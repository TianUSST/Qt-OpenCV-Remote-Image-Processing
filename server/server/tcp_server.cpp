#include"tcp_server.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include<opencv2/core.hpp>
#include<opencv2/highgui.hpp>
#include"../image_processing/imageProcessor.h"

//服务器类构造函数
//创建套接字、绑定、监听
TcpServer::TcpServer(int port):port(port),isRunning(true)//初始化构造函数
{
    //创建套接字
    server_fd=socket(AF_INET,SOCK_STREAM,0);
    if(server_fd==-1)
    {   
        perror("Socket creation failed!");
        exit(EXIT_FAILURE);
    }
    //设置套接字选项，服务器快速重启
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //设置服务器地址和要监听的端口号
    memset(&address,0,sizeof(address));
    address.sin_family=AF_INET;
    address.sin_addr.s_addr=INADDR_ANY;
    address.sin_port=htons(port);

    //绑定
    if(bind(server_fd,(struct sockaddr*)&address,sizeof(address))<0)
    {
        perror("Bind failed!");
        exit(EXIT_FAILURE);
    }
    //监听
    if(listen(server_fd,5)<0)
    {
        perror("Listen Failed!");
        exit(EXIT_FAILURE);
    }
    //输出服务器初始化信息和监听的端口号
    std::cout << "Server initialized on port " << port << std::endl;
    //构造函数结束
}

TcpServer::~TcpServer()
{   
    //析构函数，关闭连接
    close(server_fd);
}

//服务器启动
//创建新线程进行客户端请求的接受
void TcpServer::start()
{
    std::cout<<"Waiting for connection..."<<std::endl;
    std::thread acceptThread(&TcpServer::acceptConnections,this);//创建新线程接受来自客户端的连接
    acceptThread.join();//进入阻塞状态
}

//子线程接受连接，分离线程对客户端进行处理，多客户端时实现并发
void TcpServer::acceptConnections()
{
    while(isRunning)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        //创建新套接字，处理客户端请求
        int clnt_sock = accept(server_fd,(struct sockaddr*)&client_addr,&client_len);

        if(clnt_sock>=0)//接受连接请求，打印客户端的ip和端口号
        {
        std::cout<<"Connect from  IP:"<<inet_ntoa(client_addr.sin_addr)
                                        <<" port:"<<ntohs(client_addr.sin_port)<<std::endl;
        
        //创建一个新线程处理客户端请求，调用handleClient函数，传入clnt_sock作为参数
        std::thread t(&TcpServer::handleClient,this,clnt_sock);
        t.detach();
        }

        else//accept返回无效
        {
            perror("Accept failed");
        }
        
    }
}

//对单个客户端进行处理，传入对应的客户端套接字
void TcpServer::handleClient(int clnt_scok)
{
    //创建一个结构体，用于保存当前客户端的图片数据和缓冲区状态
    clientContext ctx;
    //读取第一个字节
    uint8_t type;

    while (true)
    {
    int n = recv(clnt_scok, &type, 1, MSG_WAITALL);
    if(n!=1)
    {
        std::cerr<<"Fail to receive packet type! \n";
        return;//读取包类型失败，处理线程结束
    }

    switch (type)
    {
    case 0xA0:
        handleImagePacket(clnt_scok,ctx);
        break;
    case 0xB0:
        handleCommandPacket(clnt_scok,ctx);
        break;
    case 0xC0:
        saveAddImage(clnt_scok,ctx);
        break;
    default:
        std::cerr << "Unknown packet type received: 0x" << std::hex << (int)type << std::endl;
        break;
    }
    }
    close(clnt_scok);
}

//包的第一个字节为图片，接受后续图片数据
void TcpServer::handleImagePacket(int clnt_sock, struct clientContext &context)
{
    //接受图片长度
    uint32_t imgLength;
    int n= recv(clnt_sock,&imgLength,sizeof(imgLength),MSG_WAITALL);
    if(n!=4)
    {
        std::cerr<<"Image length read failed.\n";
        return;
    }
    uint32_t imgLen=ntohl(imgLength);

    //创建缓冲区接受图片数据
    std::vector<uchar>buffer(imgLen);
    //接收到的数据量，初始化为0
    size_t received=0;
    while(received<imgLen)
    {
        int bytes=recv(clnt_sock,buffer.data()+received,imgLen-received,0);
        if(bytes<0)
        {
            std::cerr<<"Recving image body error.\n";
            return;
        }
        received+=bytes;
    }
    //接收数据完毕，移入该客户端的具有的结构体中，保存图片数据
    context.imageBuffer = std::move(buffer);
    context.hasImage=true;
    std::cout<<"Image received and stored.\n";
    //解码保存到该线程的结构体
    context.decodedImg = cv::imdecode(context.imageBuffer,cv::IMREAD_UNCHANGED);
    // cv::imwrite(filename, img);
}

void TcpServer::handleCommandPacket(int clnt_sock, clientContext &context)
{
    //没有图片，无法接受命令
    if(context.hasImage==0)
        std::cerr<<"No image,can not receive command.\n";
    //接受四个字节作为长度，来接受后续命令类型和可变参数
    uint32_t cmd_len=0;
    int n= recv(clnt_sock,&cmd_len,sizeof(cmd_len),MSG_WAITALL);
    if(n!=4)
    {
        std::cerr<<"Image length read failed.\n";
        return;
    }
    //最大字节保护
    uint32_t cmd_length=ntohl(cmd_len);
    if(cmd_length==0 || cmd_length>2*5*1024)//10MB
    {
        std::cerr<<"Invalid command length: "<<cmd_length<<std::endl;
        return;
    }
    // //根据获取的长度创建vector，接受后续指令
    // std::vector<std::string> command;
    // //根据大小重新更改
    // command.resize(cmd_length);

    std::string rawCommand(cmd_length, 0);

    size_t received=0;
    while(received<cmd_length)
    {
        int bytes=recv(clnt_sock, &rawCommand[received], cmd_length - received, 0);
        if(bytes<0)
        {
            std::cerr<<"Recving command body error.\n";
            return;
        }
        received+=bytes;
    }
    //调试长度信息
    std::cout<<"收到长度: "<<cmd_length<<std::endl;
    //转换格式，传入图像处理模块
    cv::Mat result;
    result=context.processor.process(context.decodedImg,rawCommand);
    sendProcessedImage(clnt_sock,result);
    return;
}

void TcpServer::sendProcessedImage(int clnt_sock,const cv::Mat & image)
{
    //1，编码为jpg格式
    std::vector<uchar> encodeImage;//存放编码好的数据
    std::vector<int>params = {cv::IMWRITE_JPEG_QUALITY,90};//vector类型保存编码参数
    if(!cv::imencode(".jpg",image,encodeImage,params))
    {
        std::cerr<<"Return image endode error.\n";
        return;
    }

    //2、准备发送数据
    uint8_t type=0xC0;//标识为传回图片
    uint32_t retImgLength=htonl(static_cast<uint32_t>(encodeImage.size()));//大小

    //3、发包头,1个字节
    if(send(clnt_sock,&type,1,0)!=1)
    {
        std::cerr<<"Send response type error.\n";
        return;
    }
        //发长度，4个字节
    if(send(clnt_sock,&retImgLength,4,0)!=4)
    {
        std::cerr<<"Send response length error.\n";
        return;
    }

    //4、发送图像数据，防止粘包
    size_t totalSended=0;
    while(totalSended<encodeImage.size())
    {
        int n = send(clnt_sock,encodeImage.data()+totalSended,encodeImage.size()-totalSended,0);
        if(n<=0)
        {
            //判断发送成功
            std::cerr<<"Sending image data error .\n ";
            return;
        }
        totalSended+=n;
    }
    std::cout<<"Sending processed image success!\n";
}

void TcpServer::saveAddImage(int clnt_sock,clientContext &context)
{
    //接受图片长度
    uint32_t imgLength;
    int n= recv(clnt_sock,&imgLength,sizeof(imgLength),MSG_WAITALL);
    if(n!=4)
    {
        std::cerr<<"Image length read failed.\n";
        return;
    }
    uint32_t imgLen=ntohl(imgLength);

    //创建缓冲区接受图片数据
    std::vector<uchar>buffer(imgLen);
    //接收到的数据量，初始化为0
    size_t received=0;
    while(received<imgLen)
    {
        int bytes=recv(clnt_sock,buffer.data()+received,imgLen-received,0);
        if(bytes<0)
        {
            std::cerr<<"Recving image body error.\n";
            return;
        }
        received+=bytes;
    }
    //接收数据完毕，移入该客户端的具有的结构体中，保存图片数据
    context.addImage = std::move(buffer);
    
    //解码保存到该线程的结构体
    cv::Mat addMat = cv::imdecode(context.addImage,cv::IMREAD_UNCHANGED);

    if (addMat.empty()) {
        std::cerr << "Image decode failed!" << std::endl;
    } else {
        std::cout << "Decoded image size: " << addMat.cols << "x" << addMat.rows 
                << ", channels: " << addMat.channels() << std::endl;
    }

    context.processor.getAddImage(addMat);
    std::cout<<"addImage received and stored.\n";
    // cv::imwrite(filename, img);
}

bool TcpServer::readNBytes(int sock,uint32_t* buffer,size_t n)
{
    size_t total=0;//初始化为0
    while(total<n)
    {
        ssize_t bytes = read(sock,&buffer+total,n-total);
        //没读完，返回false
        if(bytes<=0)
            return false;
        total+=bytes;
    }
    return true;
}