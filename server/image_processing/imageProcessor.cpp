#include"imageProcessor.h"
#include<opencv2/core.hpp>
#include<iostream>

cv::Mat imageProcessor::process(const cv::Mat& decodedImg, const std::string& rawCommand)
{
    auto [command, args] = splitCommand(rawCommand);

    //灰度化
    if (command == "grayscale") 
    {
        if (!args.empty()) throw std::runtime_error("Grayscale should not have arguments.");
        cv::Mat result;
        cv::cvtColor(decodedImg, result, cv::COLOR_BGR2GRAY);
        std::cout<<"grayscale ok!!"<<std::endl;
        cv::imshow("test",histTemplate);
        return histTemplate;
    }
    //水平翻转
    if (command == "flipx") 
    {
        if (!args.empty()) throw std::runtime_error("flip should not have arguments.");
        cv::Mat result;
        cv::flip(decodedImg,result,0);
        std::cout<<"flipx ok!!"<<std::endl;
        return result;
    }
    //垂直翻转
    if (command == "flipy") 
    {
        if (!args.empty()) throw std::runtime_error("flip should not have arguments.");
        cv::Mat result;
        cv::flip(decodedImg,result,1);
        std::cout<<"flipy ok!!"<<std::endl;
        return result;
    }
    //均值滤波
    if(command == "blur")
    {
        if(args.size()<1)
        {
            std::cerr<<"Blur Command needs 1 parameter.\n"<<std::endl;
            return decodedImg;//原样返回
        }
        try {
            int ksize = std::stoi(args[0]);  // 把字符串转为int
            if (ksize % 2 == 0) ksize += 1;  // OpenCV要求核大小为奇数
            cv::Mat result;
            cv::blur(decodedImg, result, cv::Size(ksize, ksize));
            return result;
        } catch (std::exception& e) {
            std::cerr << "Invalid parameter for blur: " << e.what() << std::endl;
        }
        return decodedImg;
    }
    //中值滤波
    if(command == "medianblur")
    {
        if(args.size()<1)
        {
            std::cerr<<"medianBlur Command needs 1 parameter.\n"<<std::endl;
            return decodedImg;//原样返回
        }
        try {
            int ksize = std::stoi(args[0]);  // 把字符串转为int
            if (ksize % 2 == 0) ksize += 1;  // OpenCV要求核大小为奇数
            cv::Mat result;
            cv::medianBlur(decodedImg, result, ksize);
            return result;
        } catch (std::exception& e) {
            std::cerr << "Invalid parameter for blur: " << e.what() << std::endl;
        }
        return decodedImg;
    }
    //高斯滤波
    if(command == "gaussian")
    {
        if(args.size()<1)
        {
            std::cerr<<"medianBlur Command needs 1 parameter.\n"<<std::endl;
            return decodedImg;//原样返回
        }
        try {
            int ksizeX = std::stoi(args[0]);
            int ksizeY = std::stoi(args[1]);  // 把字符串转为int
            if (ksizeX % 2 == 0) ksizeX += 1;
            if(ksizeY %2 == 0)  ksizeY+=1;  // OpenCV要求核大小为奇数
            cv::Mat result;
            double sigmaX = std::stod(args[2]);
            double sigmaY = std::stod(args[3]);
            cv::GaussianBlur(decodedImg,result,cv::Size(ksizeX,ksizeY),sigmaX,sigmaY,4);
            return result;
        } catch (std::exception& e) {
            std::cerr << "Invalid parameter for blur: " << e.what() << std::endl;
        }
        return decodedImg;
    }
    //双边滤波
    if(command == "bilateralFilter")
    {
        cv::Mat result;
        double sigmaColor = std::stod(args[0]);
        double sigmaSpace = std::stod(args[1]);
        cv::bilateralFilter(decodedImg,result,-1,sigmaColor,sigmaSpace,4);
        return result;
    }
    //sobel边缘检测
    if(command == "sobel")
    {
        int ksize = std::stoi(args[0]);
        if(ksize%2==0)  ksize+=1;
        double scale = std::stod(args[1]);
        cv::Mat result;
        cv::Sobel(decodedImg,result,-1,1,1,ksize,scale,0,4);
        return result;

    }
    //laplacian边缘检测
    if(command == "laplacian")
    {
        int ksize = std::stoi(args[0]);
        if(ksize%2==0)  ksize+=1;
        double scale = std::stod(args[1]);
        cv::Mat result;
        cv::Laplacian(decodedImg,result,-1,ksize,scale,0,4);
        return result;
    }
    //Canny边缘检测
    if(command == "canny")
    {
        int lowThreshold = std::stoi(args[0]);
        int highThreshold = std::stoi(args[1]);
        int ksize = std::stoi(args[2]);
        cv::Mat result;
        cv::Canny(decodedImg,result,lowThreshold,highThreshold,ksize);
        return result;
    }
    // 阈值化
    if(command == "threshold")
    {
        if (args.size() >4) throw std::runtime_error("Threshold needs 4 args max");

        int threshVal = std::stoi(args[0]);
        int maxVal = std::stoi(args[1]);
        int threshType = std::stoi(args[2]); // 直接是枚举值
        int Otsu = std::stoi(args[3]);
        cv::Mat result;
        cv::threshold(decodedImg, result, threshVal, maxVal, threshType | Otsu);
        return result;
    }
    //自适应阈值化
    if(command == "adaptive")
    {
        double maxVal = std::stod(args[0]);
        int adaptiveMethod = std::stoi(args[1]);
        int thresholdType = std::stoi(args[2]);
        int blockSize = std::stoi(args[3]);
        double c = std::stof(args[4]);
        cv::Mat result;
        cv::adaptiveThreshold(decodedImg,result,maxVal,adaptiveMethod,thresholdType,blockSize,c);
        return result;
    }
    //自定义滤波
    if(command == "custom")
    {
        //获取行列数
        int rowCount = std::stoi(args[3]);
        int colCount = std::stoi(args[4]);
        //获取锚点
        int x= std::stoi(args[1]);
        int y= std::stoi(args[2]);
        cv::Point anchor(x,y);
        //创建自定义卷积核 
        cv::Mat kernel(rowCount,colCount,CV_32FC1);
        //循环填充
        int start= 5;//卷积核内部参数从args[5]开始
        for(int row=0;row<rowCount;row++)
            for(int col=0;col<colCount;col++)
            {
                kernel.at<float>(row,col)=std::stof(args[start]);
                start++;
            }
        //自定义滤波
        cv::Mat result;
        cv::filter2D(decodedImg,result,-1,kernel,anchor);
        return result;

    }
    //膨胀
    if(command == "dilate")
    {
        int iterations = std::stoi(args[0]);
        cv::Mat result;
        cv::dilate(decodedImg, result, cv::Mat(), cv::Point(-1, -1), iterations);
        return result;
    }
    //腐蚀
    if(command == "erode")
    {
        int iterations = std::stoi(args[0]);
        cv::Mat result;
        cv::erode(decodedImg, result, cv::Mat(), cv::Point(-1, -1), iterations);
        return result;
    }
    //开运算
    if(command == "open")
    {
        int iterations = std::stoi(args[0]);
        cv::Mat result;
        cv::morphologyEx(decodedImg, result,2, cv::Mat(), cv::Point(-1, -1), iterations);
        return result;
    }
    //闭运算
    if(command == "close")
    {
        int iterations = std::stoi(args[0]);
        cv::Mat result;
        cv::morphologyEx(decodedImg, result,3, cv::Mat(), cv::Point(-1, -1), iterations);
        return result;
    }
    //形态学梯度
    if(command == "gradient")
    {
        int iterations = std::stoi(args[0]);
        cv::Mat result;
        cv::morphologyEx(decodedImg, result,4, cv::Mat(), cv::Point(-1, -1), iterations);
        return result;
    }
    //顶帽
    if(command == "tophat")
    {
        int iterations = std::stoi(args[0]);
        cv::Mat result;
        cv::morphologyEx(decodedImg, result,5, cv::Mat(), cv::Point(-1, -1), iterations);
        return result;
    }
    //黑帽
    if(command =="blackhat")
    {
        int iterations = std::stoi(args[0]);
        cv::Mat result;
        cv::morphologyEx(decodedImg, result,6, cv::Mat(), cv::Point(-1, -1), iterations);
        return result;
    }
    //畸变矫正
    if(command == "perspectiveTransform")
    {
        //获得四个角点坐标
        float x1 = std::stof(args[0]);
        float y1 = std::stof(args[1]);
        float x2 = std::stof(args[2]);
        float y2 = std::stof(args[3]);
        float x3 = std::stof(args[4]);
        float y3 = std::stof(args[5]);
        float x4 = std::stof(args[6]);
        float y4 = std::stof(args[7]);
        //构造原始点
        std::vector<cv::Point2f> srcPoint;
        srcPoint.push_back(cv::Point2f(x1,y1));
        srcPoint.push_back(cv::Point2f(x2,y2));
        srcPoint.push_back(cv::Point2f(x3,y3));
        srcPoint.push_back(cv::Point2f(x4,y4));

        //将选中点计算欧氏距离，转换矩形、获取目标点
        float widthA = cv::norm(srcPoint[2]-srcPoint[3]);//底边宽
        float widthB = cv::norm(srcPoint[1]-srcPoint[0]);//上边宽
        float maxWidth = std::max(widthA,widthB);//取较长的宽

        float heightA = cv::norm(srcPoint[2]-srcPoint[0]);
        float heightB = cv::norm(srcPoint[3]-srcPoint[1]);
        float maxHeight = std::max(heightA,heightB);

        //构造目标点
        std::vector<cv::Point2f> dstPoint = {
            {0.f,0.f},
            {maxWidth-1,0.f},
            {maxWidth-1,maxHeight-1},
            {0.f,maxHeight-1}
        };


        //计算透视矩阵
        cv::Mat transform = cv::getPerspectiveTransform(srcPoint,dstPoint);
        //透视变换
        cv::Mat result;
        cv::Size2f size(maxWidth,maxHeight);
        cv::warpPerspective(decodedImg,result,transform,size);//对裁切后的图像进行透视变换
        return result;
    }
    //灰度直方图均衡化
    if(command == "equalizeHist")
    {
        cv::Mat result;
        cv::equalizeHist(decodedImg,result);
        return result;
    }
    //反向投影
    if(command == "backProject")
    {
        if(!histTemplate.empty())
        {
            //1、转换HSV色域
            cv::Mat decodedImg_HSV;
            cv::Mat histTemplate_HSV;
            cv::cvtColor(decodedImg,decodedImg_HSV,cv::COLOR_BGR2HSV);
            cv::cvtColor(histTemplate,histTemplate_HSV,cv::COLOR_BGR2HSV);
            //2、提取H和S通道
                // 使用 H 和 S 通道
            int channels[] = {0, 1};  // H通道索引为0，S通道为1
            //3、设置参数
            int histSize = std::stoi (args[0]);
            int hSize[] = {histSize, histSize};  // H 和 S 通道的直方图尺寸
            float hRange[] = {0, 180};
            float sRange[] = {0, 256};
            const float* ranges[] = {hRange, sRange};  // 每个通道的范围
            //4、计算模板直方图
            cv::Mat hist;
            cv::calcHist(&histTemplate_HSV, 1, channels, cv::Mat(), hist, 2, hSize, ranges, true, false);
            cv::normalize(hist, hist, 0, 255, cv::NORM_MINMAX);  // 可选，增强对比
            //5、反向投影
            cv::Mat backProj;
            cv::calcBackProject(&decodedImg_HSV, 1, channels, hist, backProj, ranges, 1, true);
            //6.返回图像
            return backProj;
        }
    }
    return decodedImg;
}

//解析原始命令，返回一个字符串和一个字符串vector。
std::pair<std::string, std::vector<std::string>> imageProcessor::splitCommand(const std::string& rawCommand)
{
    //创建字符串流
    std::stringstream ss(rawCommand);
    //分割字符
    std::string segment;
    std::vector<std::string> tokens;
    //遇到“|”,分割放入segment
    while(std::getline(ss,segment,'|'))
    {
        tokens.push_back(segment);
    }//示例tokens形式：blur,0.1,5,10
    //分割完成，创建保存指令
    std::string command;
    std::vector<std::string> args;
    if(!tokens.empty())
    {
        command = tokens[0];
        args.assign(tokens.begin()+1,tokens.end());
    }

    return {command,args};
}

void imageProcessor::getAddImage(const cv::Mat image)
{
    std::cout << "getAddImage: received image size = " << image.cols << "x" << image.rows << std::endl;
    this->histTemplate = image.clone();
    return;
}