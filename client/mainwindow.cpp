#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QBuffer>
#include <QDataStream>
#include<iostream>
#include <algorithm>
//添加功能：
//1、使用文本框显示每一步的指  令和服务器回传的错误信息
//2、在参数界面添加对功能的描述提示，方便用户使用
//3、解决拉拽整体窗口后，预览区图片显示尺寸变化的问题
//4、重写事件，自动获取鼠标指针所指像素的RGB值   √如何解决非等比缩放？
//5、文本框用颜色区分图像处理命令，方便查看当前图像是用哪些指令得到的
//修改每个参数窗口为对应名称而不是"dialog"


//----------------------构造函数----------------------------------------
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);//界面初始化
    //创建套接字
    socket = new QTcpSocket(this);
    //服务器连接是否成功的信号
    connect(socket, &QTcpSocket::connected, this, &MainWindow::on_socketConnected);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::on_socketDisconnected);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &MainWindow::on_socketError);
    //QT读取服务器的回传图片
    connect(socket,&QTcpSocket::readyRead,this,&MainWindow::on_socketReadyRead);
    //开闭按键控制
    // connect(this,&MainWindow::imageSendSuccess,this,&MainWindow::enableProcessButton);
    // connect(this,&MainWindow::imageClear,this,&MainWindow::unenableProcessButton);
    //显示服务器是否存在图片
    connect(this,&MainWindow::checkServerHasImage,this,&MainWindow::displayWheatherServerHasImage);
    ui->labelSended->setText("服务器是否收到图片：<span style='color:red;'>否</span>");
    //显示服务器连接状态
    connect(socket,&QTcpSocket::stateChanged,this,&MainWindow::displayServerStatus);
    ui->labelConnected->setText("服务器连接状态：<span style='color:red;'>未连接</span>");
    //连接重写好的图片标签类,显示当前鼠标所指的RGB值
    connect(ui->imageLabel_in,&imageLabel::pixelHovered,this,&MainWindow::displayRGB);
    connect(ui->imageLabel_out,&imageLabel::pixelHovered,this,&MainWindow::displayRGB2);
    emit imageClear();

}
//--------------------------------------获取当前标签大小，在畸变矫正窗口获取，显示同样大小的图片-----------------------------------
QSize MainWindow::getImageLabelSize()
{
    return ui->imageLabel_in->size();
}
//---------------------------------析构函数-----------------------------------------------

MainWindow::~MainWindow()
{
    delete ui;
}

//-------------------打开图片-----------------------------------------------------------------------------
void MainWindow::on_openButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.bmp)");
    if (!fileName.isEmpty()) {
        QImage image(fileName);
        if (image.isNull()) {
            QMessageBox::warning(this, "错误", "无法加载图片");
            return;
        }
        //在打开新图片时，清空所有内容
        ui->imageLabel_in->clear();
        ui->imageLabel_out->clear();
        imageData.clear();
        retImageData.clear();
        emit imageClear();
        emit checkServerHasImage();

        // //获取当前label尺寸
        // displaySize = ui->imageLabel_in->size();
        // //自动显示打开的图片
        // ui->imageLabel_in->setPixmap(QPixmap::fromImage(image).scaled(displaySize, Qt::KeepAspectRatio));
        ui->imageLabel_in->setImage(image);

        // 压缩为字节数组
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "JPG");  // 也可换成 PNG

        imageData = byteArray;
        QMessageBox::information(this, "图片", "图片加载成功");
        appendLog("图片打开成功!");
        //打开图片之后，可以清空和发送图片
        ui->sendButton->setEnabled(true);
        ui->clearButton->setEnabled(true);
        reSend=false;
    }
}
//------------------------------显示传回的图片-----------------------------------------
void MainWindow::on_socketReadyRead()
{
    //将套接字变为数据流，方便读写
    QDataStream in(socket);
    in.setByteOrder(QDataStream::BigEndian);

    static quint8 type = 0;
    static quint32 imgLength = 0;
    static QByteArray buffer;

    //如果socket中有字节可读
    while(socket->bytesAvailable())
    {
        //第一次type=0
        if(type == 0)
        {
            if(socket->bytesAvailable()<5)
                return;//可读数据小于5，立即返回
            //读类型、读长度、清空缓冲区，准备接受
            in>>type;
            in>>imgLength;
            buffer.clear();
        }
        if(type == 0xC0)
        {
            //剩余需要读取的数据是min(socket中剩余字节、需要读取的剩余字节)
            int byteToRead = std::min<int>(socket->bytesAvailable(),imgLength-buffer.size());
            buffer.append(socket->read(byteToRead));//读取放在buffer
            //将回传图片暂时存入
            retImageData = buffer;
            //读取结束，准备显示
            if(buffer.size()>=imgLength)
            {
                QImage image = QImage::fromData(buffer,"JPG");
                // ui->imageLabel_out->setPixmap(QPixmap::fromImage(image).scaled(displaySize,Qt::KeepAspectRatio));
                ui->imageLabel_out->setImage(image);
                //日志输出
                appendLog("图像处理成功,请查看预览区！");
                //重置状态
                type=0;
                imgLength=0;
                buffer.clear();
            }
        }
    }
}


//------------------------------发送图片-----------------------------------------
void MainWindow::on_sendButton_clicked()
{
    //未连接服务器
    if (!socket->isOpen()) {
        QMessageBox::warning(this, "未连接", "请先连接服务器");
        return;
    }

    //没有从本地打开图片
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择图片");
        return;
    }
    //创建QByteArray格式的数组，用于搭载图像进行传输
    QByteArray packet;

    //发送的包头：类型（图片）、图片大小
    quint8 type = 0xA0;
    quint32 length = imageData.size();
    //写包头
    packet.append(static_cast<char>(type));
    QDataStream stream(&packet, QIODevice::WriteOnly| QIODevice::Append);
    stream.setByteOrder(QDataStream::BigEndian);  // 与服务器一致，大端序
    //写入图片大小数据
    stream << static_cast<quint32>(imageData.size());
    //写入图片内容
    packet.append(imageData);




    //查看打包好的packet是否为预期格式。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。
    qDebug() << "Packet size:" << packet.size();

    // 打印前10个字节
    qDebug() << "First 10 bytes of packet:";
    for (int i = 0; i < std::min(10, static_cast<int>(packet.size())); ++i) {
        qDebug().nospace() << QString("0x%1 ").arg(static_cast<quint8>(packet[i]), 2, 16, QLatin1Char('0'));
    }

    // 打印图像数据长度部分
    quint8 type1 = static_cast<quint8>(packet[0]);
    quint32 length1;
    QDataStream lengthStream(packet.mid(1, 4));
    lengthStream.setByteOrder(QDataStream::BigEndian);
    lengthStream >> length1;

    qDebug() << "Type byte:" << QString("0x%1").arg(type1, 2, 16, QLatin1Char('0'));
    qDebug() << "Decoded image length:" << length1;

    // 可选：查看是否是 JPEG 开头（0xFF 0xD8）
    if (packet.size() > 6 &&
        static_cast<quint8>(packet[5]) == 0xFF &&
        static_cast<quint8>(packet[6]) == 0xD8) {
        qDebug() << "Detected JPEG header at byte 5+6 (OK)";
    } else {
        qDebug() << "Warning: Not a valid JPEG header at byte 5+6";
    }
    //调试逻辑结束。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。

    //放入缓冲区，刷新缓冲区
    socket->write(packet);
    socket->flush();
    //发送成功日志
    appendLog("发送图像");
    sended=true;
    //Check一下服务器是否收到图片
    emit checkServerHasImage();
    //如果不是继续处理的发送，才显示消息
    if(!reSend)
        QMessageBox::information(this, "发送", "图片已发送，请选择处理指令");
    emit imageSendSuccess();
}

//------------------------服务器连接逻辑--------------------------------------------------
void MainWindow::on_connectButton_clicked()
{
    //先写死，便于调试
    ui->ipLineEdit->setText("127.0.0.1");
    ui->portLineEdit->setText("6666");
    QString ip = ui->ipLineEdit->text();
    quint16 port = ui->portLineEdit->text().toUShort();
    socket->connectToHost(ip, port);
}

void MainWindow::on_disconnectButton_clicked()
{
    socket->close();
}


void MainWindow::on_socketConnected() {
    QMessageBox::information(this, "连接", "已连接服务器");
    appendLog("服务器已连接");
}

void MainWindow::on_socketDisconnected() {
    QMessageBox::warning(this, "断开", "与服务器连接断开");
    appendLog("服务器已断开");
}

void MainWindow::on_socketError(QAbstractSocket::SocketError) {
    QMessageBox::critical(this, "错误", socket->errorString());
    appendLog("服务器连接错误");
}
//-----------------------------------清空待处理区---------------------------------------------
void MainWindow::on_clearButton_clicked()
{
    //没有从本地打开图片
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "得先有一张图片才能清空");
        return;
    }
    ui->imageLabel_in->clear();
    ui->imageLabel_out->clear();
    ui->imageLabel_in->setImage(QImage());
    ui->imageLabel_out->setImage(QImage());
    imageData.clear();
    retImageData.clear();
    //清空日志输出
    appendLog("图像清空");
    sended=false;
    emit imageClear();
    emit checkServerHasImage();
}

//------------------------------打包发送命令--------------------------------------------------
void MainWindow::sendCommandPacket(const QString& command,const QStringList &args)
{
    //如果套接字没初始化或者没有连接
    if(!socket || !socket->isOpen())
    {
        QMessageBox::warning(this,"未连接","请先连接服务器");
        return;
    }
    //没有从本地打开图片
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择图片");
        return;
    }
    //设置（命令类型+参数）
    QByteArray payLoadData;
    payLoadData.append(command.toUtf8());//加入命令类型
    for (const QString& arg : args) {//加入参数列表，以"|"分隔
        payLoadData.append("|");
        payLoadData.append(arg.toUtf8());
    }

    //打包命令包
    QByteArray packet;
    QDataStream stream(&packet,QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);//设置大端序
        //添加命令包标识
    quint8 type=0xB0;
        //添加packet的长度信息，便于服务器解析接受
    quint32 payLoadDataLength=static_cast<quint32>(payLoadData.size());
    stream<<type;
    stream<<payLoadDataLength;
        // 添加命令类型和参数列表
    stream.writeRawData(payLoadData.constData(), payLoadData.size());

    //打包结束，准备发送
    socket->write(packet);
    socket->flush();
    //追加日志显示
    appendLog("发送命令："+command);
    //调试信息
    qDebug()<<"发送命令包： "<<payLoadData<<"长度： "<<payLoadDataLength;
    qDebug() << "First 10 bytes of command:";
    for (int i = 0; i < std::min(10, static_cast<int>(packet.size())); ++i) {
        qDebug().nospace() << QString("0x%1 ").arg(static_cast<quint8>(packet[i]), 2, 16, QLatin1Char('0'));
    }

}
//-------------------------------------------------将浮点型卷积核转换为字符型，发送-------------------------------------------------
QStringList MainWindow::kernelList(QVector<QVector<float> > &kernel)
{
    QStringList kernelInfo;
    int rowCount = kernel.size();
    int colCount = kernel[0].size();
    //先传行列数，便于服务器解析
    kernelInfo << QString::number(rowCount);
    kernelInfo << QString::number(colCount);
    //再传具体信息
    for(const auto& row : kernel)
        for(float val : row)
            kernelInfo << QString::number(val,'f',2);
    return kernelInfo;

}
//----------------------------------------保存截取的图到本地，发送到服务器------------------------------------------------------
void MainWindow::saveAndSendHistTemplate(const QRectF &rectOnImage, const QImage &histTemplateImage)
{
    //1、截取图像,保存到本地
    QRect rectInt = rectOnImage.toRect();
    QImage croppedImage = histTemplateImage.copy(rectInt);
    templateData = croppedImage;
    QMessageBox::information(this,"选取成功","特征区域保存成功");

    //2、发送
    //QImage->QByteArray->QStringList发送
    //先将QImage转QByteArray
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    croppedImage.save(&buffer,"JPG");
    buffer.close();//此时buffer.buffer()为byteArray
    //*********************************************************base64发送方式*********************************

    // //打包命令包
    // QByteArray packet;
    // QDataStream stream(&packet,QIODevice::WriteOnly);
    // stream.setByteOrder(QDataStream::BigEndian);//设置大端序
    //     //添加命令包标识
    //     //先宽后高，服务器按此解析
    // //设置（命令类型+参数）
    // QByteArray payLoadData;
    // payLoadData.append(QString("addImageData").toUtf8());//加入命令类型
    // payLoadData.append("|");
    // payLoadData.append(QString::number(histTemplateImage.width()).toUtf8());
    // payLoadData.append("|");
    // payLoadData.append(QString::number(histTemplateImage.height()).toUtf8());
    // payLoadData.append("|");
    // payLoadData.append(byteArray.toBase64());
    // QStringList imageData;

    // quint8 type=0xB0;
    //     //添加packet的长度信息，便于服务器解析接受
    // quint32 payLoadDataLength=static_cast<quint32>(payLoadData.size());
    // stream<<type;
    // stream<<payLoadDataLength;
    //     // 添加命令类型和参数列表
    // stream.writeRawData(payLoadData.constData(), payLoadData.size());

    // //..............调试信息
    // qDebug()<<"发送命令包： "<<payLoadData<<"长度： "<<payLoadDataLength;
    // qDebug() << "First 10 bytes of command:";
    // for (int i = 0; i < std::min(10, static_cast<int>(packet.size())); ++i) {
    //     qDebug().nospace() << QString("0x%1 ").arg(static_cast<quint8>(packet[i]), 2, 16, QLatin1Char('0'));
    // }

    // socket->write(packet);
    // socket->flush();
    //**********************************************************************************************************

    //**********************************************************以bYTEARRAY发送******************************************

    // // 自定义参数协议：宽、高作为 header，用 '|' 分隔，后接 QByteArray
    // QString header = QString("addImageData|%1|%2|").arg(croppedImage.width()).arg(croppedImage.height());
    // QByteArray payLoadData = header.toUtf8() + byteArray;

    // // 构造发送包
    // QByteArray packet;
    // QDataStream stream(&packet, QIODevice::WriteOnly);
    // stream.setByteOrder(QDataStream::BigEndian);
    // quint8 type = 0xB0;
    // quint32 length = static_cast<quint32>(payLoadData.size());
    // stream << type << length;
    // stream.writeRawData(payLoadData.constData(), payLoadData.size());

    // qDebug()<<"发送命令包： "<<payLoadData<<"长度： "<<length;
    // qDebug() << "First 10 bytes of command:";
    // for (int i = 0; i < std::min(10, static_cast<int>(packet.size())); ++i) {
    //     qDebug().nospace() << QString("0x%1 ").arg(static_cast<quint8>(packet[i]), 2, 16, QLatin1Char('0'));
    // }


    // socket->write(packet);
    // socket->flush();
    //*****************************************************************************************************************

    //创建QByteArray格式的数组，用于搭载图像进行传输
    QByteArray packet;

    //发送的包头：类型（图片）、图片大小
    quint8 type = 0xC0;
    quint32 length = byteArray.size();
    //写包头
    packet.append(static_cast<char>(type));
    QDataStream stream(&packet, QIODevice::WriteOnly| QIODevice::Append);
    stream.setByteOrder(QDataStream::BigEndian);  // 与服务器一致，大端序
    //写入图片大小数据
    stream << static_cast<quint32>(byteArray.size());
    //写入图片内容
    packet.append(byteArray);

    //查看打包好的packet是否为预期格式。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。
    qDebug() << "Packet size:" << packet.size();

    // 打印前10个字节
    qDebug() << "First 10 bytes of packet:";
    for (int i = 0; i < std::min(10, static_cast<int>(packet.size())); ++i) {
        qDebug().nospace() << QString("0x%1 ").arg(static_cast<quint8>(packet[i]), 2, 16, QLatin1Char('0'));
    }

    // 打印图像数据长度部分
    quint8 type1 = static_cast<quint8>(packet[0]);
    quint32 length1;
    QDataStream lengthStream(packet.mid(1, 4));
    lengthStream.setByteOrder(QDataStream::BigEndian);
    lengthStream >> length1;

    qDebug() << "Type byte:" << QString("0x%1").arg(type1, 2, 16, QLatin1Char('0'));
    qDebug() << "Decoded image length:" << length1;

    // 可选：查看是否是 JPEG 开头（0xFF 0xD8）
    if (packet.size() > 6 &&
        static_cast<quint8>(packet[5]) == 0xFF &&
        static_cast<quint8>(packet[6]) == 0xD8) {
        qDebug() << "Detected JPEG header at byte 5+6 (OK)";
    } else {
        qDebug() << "Warning: Not a valid JPEG header at byte 5+6";
    }
    //调试逻辑结束。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。

    //放入缓冲区，刷新缓冲区
    socket->write(packet);
    socket->flush();

}
//---------------------------CV灰度化----------------------------------------------------------------------------------
void MainWindow::on_cmd_grayButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    sendCommandPacket("grayscale", {});
}

//---------------------------------------CV翻转-----------------------------------------------------------------
void MainWindow::on_cmd_flipxButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    sendCommandPacket("flipx",{});
}


void MainWindow::on_cmd_flipyButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    sendCommandPacket("flipy",{});
}

//-------------------------------CV均值滤波----------------------------------------------------------------
void MainWindow::on_cmd_blurButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"ksize","卷积核大小",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        //发送带有参数的命令
        sendCommandPacket("blur",params);
    }
}

//-----------------------------------CV-Canny边缘检测------------------------------------------------------------
void MainWindow::on_cmd_CannyButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"lowThreshold","低阈值",ParamDialog::lineEdit},
        {"highThreshold","高阈值",ParamDialog::lineEdit},
        {"apertureSize","Sobel算子大小",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
        //发送带有参数的命令
        sendCommandPacket("canny",params);
    }
}

//-------------------------------点击按键，将回传图片放入待处理区-------------------------------------------
void MainWindow::on_continueButton_clicked()
{
    if (retImageData.isEmpty()) {
        QMessageBox::warning(this, "错误", "预览区无图像");
        return;
    }
    imageData = retImageData;
    //将此次为重新发送表示设为true
    reSend=true;
    on_sendButton_clicked();
    QImage image = QImage::fromData(imageData,"JPG");
    // ui->imageLabel_in->setPixmap(QPixmap::fromImage(image).scaled(displaySize,Qt::KeepAspectRatio));
    ui->imageLabel_in->setImage(image);
    //日志显示
    appendLog("替换待处理区图像为预览区图像");
    QMessageBox::information(this, "成功", "待处理区替换成功，请继续选择处理指令");
    //Check服务器图片状态
    emit checkServerHasImage();

}

//-------------------------------------开闭处理按键控制-----------------------------------------------------
void MainWindow::enableProcessButton()
{
    QList<QPushButton*> allButtons = this->findChildren<QPushButton*>();
    for (QPushButton* button : allButtons)
    {
        if (button->objectName().startsWith("cmd_")) {
            button->setEnabled(true);
        }
    }
    ui->clearButton->setEnabled(true);
    ui->continueButton->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->saveButton->setEnabled(true);
}

void MainWindow::unenableProcessButton()
{
    QList<QPushButton*> allButtons = this->findChildren<QPushButton*>();
    for (QPushButton* button : allButtons)
    {
        if (button->objectName().startsWith("cmd_")) {
            button->setEnabled(false);
        }
    }
    ui->clearButton->setEnabled(false);
    ui->continueButton->setEnabled(false);
    ui->sendButton->setEnabled(false);
    ui->saveButton->setEnabled(false);
    sended = false;//清空图片，相当于服务器端没有图片
}

//-----------------------------------------保存预览区图片-------------------------------------------------------
void MainWindow::on_saveButton_clicked()
{
    QImage saveImage;
    if(!saveImage.loadFromData(retImageData))
    {
        QMessageBox::warning(this,"错误","图像解码失败");
        return;
    }
    QString saveImagePath = QFileDialog::getSaveFileName(this,"保存图像","","图像文件(*.jpg *.png *.bmp)");

    if(saveImagePath.isEmpty())
        return;
    if(!saveImage.save(saveImagePath))
        QMessageBox::warning(this,"错误","图像保存失败！");
    else
    {
        QMessageBox::information(this,"成功","当前预览区图像已经保存！");
        appendLog("成功保存图像到："+saveImagePath);
    }

}

//---------------------------------指令可视化记录-------------------------------------------------------------
void MainWindow::appendLog(QString message)
{
    QString timeStamp = QDateTime::currentDateTime().toString("hh:mm:ss");//时间戳
    QString logMessage = QString("[%1] %2").arg(timeStamp,message);
    //追加到日志显示窗口
    ui->textEditVisualCommand->append(logMessage);
    ui->textEditVisualCommand->moveCursor(QTextCursor::End);//总是将光标放在最后一行
}

//---------------------------------------在底部显示服务器是否收到图片---------------------------------------------

void MainWindow::displayWheatherServerHasImage()
{
    if(sended==true)//可以同时判断服务器回传信息，添加判断条件，更加健壮
    {
        ui->labelSended->setText("服务器是否收到图片：<span style='color:green;'>是</span>");
    }
    else
        ui->labelSended->setText("服务器是否收到图片：<span style='color:red;'>否</span>");
}

//-----------------------------------------------显示服务器连接状态------------------------------------------
void MainWindow::displayServerStatus()
{
    if(socket->isOpen())
    {
        ui->labelConnected->setText("服务器连接状态：<span style='color:green;'>已连接</span>");
    }
    else
        ui->labelConnected->setText("服务器连接状态：<span style='color:red;'>未连接</span>");
}
//--------------------------------------------------显示当前鼠标所指的坐标和RGB
void MainWindow::displayRGB(int x,int y,QColor c)
{
    QString rgbInfo = QString("(%1,%2)-R:%3 G:%4 B:%5").arg(x).arg(y).arg(c.red()).arg(c.green()).arg(c.blue());
    ui->colorLabel_in->setText(rgbInfo);
}

void MainWindow::displayRGB2(int x, int y, QColor c)
{
    QString rgbInfo = QString("(%1,%2)-R:%3 G:%4 B:%5").arg(x).arg(y).arg(c.red()).arg(c.green()).arg(c.blue());
    ui->colorLabel_out->setText(rgbInfo);
}
//---------------------------------------------------CV阈值化-------------------------------------------------------------------
void MainWindow::on_cmd_threshHoldButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"thresh","阈值",ParamDialog::lineEdit},
        {"maxValue","阈值化的最大值",ParamDialog::lineEdit},
        {"thresholdType","",ParamDialog::comboBox,{"cv::THRESH_BINARY","cv::THRESH_BINARY_INV","cv::THRESH_TRUNC","cv::THRESH_TOZERO","cv::THRESH_TOZERO_INV"}},
        {"使用Otsu","",ParamDialog::checkBox}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
        //发送带有参数的命令
        sendCommandPacket("threshold",params);
    }
}

//-------------------------------------------------CV自适应阈值----------------------------------------------------
void MainWindow::on_cmd_adaptiveButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"maxValue","阈值化的最大值",ParamDialog::lineEdit},
        {"adaptiveMethod","",ParamDialog::comboBox,{"cv::ADAPTIVE_THRESH_MEAN_C","cv::ADAPTIVE_THRESH_GAUSSIAN_C"}},
        {"thresholdType","",ParamDialog::comboBox,{"cv::THRESH_BINARY","cv::THRESH_BINARY_INV"}},
        {"blockSize","卷积核大小",ParamDialog::lineEdit},
        {"C","偏移常量(double)",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
        //发送带有参数的命令
        sendCommandPacket("adaptive",params);
    }
}

//-------------------------------------------------CV中值滤波------------------------------------------------------
void MainWindow::on_cmd_medianBlurButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"ksize","卷积核大小",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);
    dialog.setWindowTitle("中值滤波");

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
        //发送带有参数的命令
        sendCommandPacket("medianblur",params);
    }
}

//---------------------------------------------------开始自定义卷积核-------------------------------------------------
void MainWindow::on_cmd_customButton_clicked()
{
    customKernelDialog dialog(this);
    if(dialog.exec() == QDialog::Accepted)
    {

        if(dialog.isValidKernel)
        {
            customKernel.clear();
            customKernel = dialog.getKernelData();
            QMessageBox::information(this,"成功","自定义卷积核保存成功！");
            appendLog("保存自定义卷积核");
            qDebug()<<customKernel;
        }
        else
            QMessageBox::warning(this,"错误","自定义卷积核为空！");
    }

}

//----------------------------------------------在日志中显示自定义卷积核大小------------------------------------------------
void MainWindow::on_cmd_showCustomButton_clicked()
{
    if(customKernel.empty())
        appendLog("当前自定义卷积核为空");

    else
    {   QString displayContent;
        for(const auto& row : customKernel)
        {
            for(float val : row)
                displayContent += QString("%1").arg(val,8,'f',2);
            displayContent += '\n';
        }
        QString s = "当前自定义卷积核为：\n" + displayContent;
        appendLog(s);
    }
}

//----------------------------------------------------清空当前保存的自定义卷积核-------------------------------------------------
void MainWindow::on_cmd_deleteCustomButton_clicked()
{
    if(customKernel.empty())
        appendLog("当前自定义卷积核为空");
    else {
        customKernel.clear();
        appendLog("清空当前自定义卷积核");
    }
}

//------------------------------------------------使用当前自定义卷积核进行滤波----------------------------------------------
void MainWindow::on_cmd_customFilterButton_clicked()
{
    if(customKernel.empty())
        QMessageBox::warning(this,"空的卷积核","当前自定义卷积核为空");
    else
    {
        //创建要显示的参数
        QList<ParamDialog::ParamSpec> paramSpec = {
            {"kernel","",ParamDialog::comboBox,{"自定义卷积核"}},
            {"anchor(x)","锚点x坐标",ParamDialog::lineEdit},
            {"anchor(y)","锚点y坐标",ParamDialog::lineEdit}
        };
        //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
        ParamDialog dialog(paramSpec,this);

        //如果按下确定
        if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
        {
            QStringList params = dialog.getParams();
            params += kernelList(customKernel);
            qDebug()<<"输入参数："<<params;
            //发送带有参数的命令
            sendCommandPacket("custom",params);
        }
    }
}

//----------------------------------------------------CV膨胀---------------------------------------------------------------------
void MainWindow::on_cmd_dilateButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"iterations","迭代次数",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
        //发送带有参数的命令
        sendCommandPacket("dilate",params);
    }
}
//-------------------------------------------------------CV腐蚀----------------------------------------------------------------
void MainWindow::on_cmd_erodeButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"iterations","迭代次数",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
        //发送带有参数的命令
        sendCommandPacket("erode",params);
    }
}

//------------------------------------------------------CV开运算-----------------------------------------------------------
void MainWindow::on_cmd_openButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"iterations","迭代次数",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
        //发送带有参数的命令
        sendCommandPacket("open",params);
    }
}

//-----------------------------------------------------------CV闭运算-----------------------------------------------------------

void MainWindow::on_cmd_closeButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"iterations","迭代次数",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
        //发送带有参数的命令
        sendCommandPacket("close",params);
    }
}

//--------------------------------------------------CV顶帽----------------------------------------------------------------------
void MainWindow::on_cmd_tophatButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"iterations","迭代次数",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
        //发送带有参数的命令
        sendCommandPacket("tophat",params);
    }
}

//---------------------------------------------------------CV黑帽-------------------------------------------------------------
void MainWindow::on_cmd_blackhatButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"iterations","迭代次数",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
        //发送带有参数的命令
        sendCommandPacket("blackhat",params);
    }
}
//---------------------------------------------------------CV形态学梯度-------------------------------------------------------

void MainWindow::on_cmd_gradientButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"iterations","迭代次数",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
        //发送带有参数的命令
        sendCommandPacket("gradient",params);
    }
}

//------------------------------------------CV透视变换-畸变矫正-------------------------------------------------------------------
void MainWindow::on_cmd_perspectiveButton_clicked()
{
    imageOperateDialog *dialog = new imageOperateDialog(this);
    QImage operateImage = QImage::fromData(imageData, "jpg");
    dialog->setOperateImage(operateImage);

    QLabel *imageLabel = dialog->findChild<QLabel*>("imageLabelOnDialog");
    QLabel *tipsLabel = dialog->findChild<QLabel*>("tipsLabel");

    tipsLabel->setText("请手动拖拽四个角点至需要矫正的区域");//在底部添加提示信息

    // 延迟添加 overlay，等生成imagelabel后再overlay
    QTimer::singleShot(0, [dialog, imageLabel]() {
        cornerRectOverlay* overlay = new cornerRectOverlay(imageLabel);
        overlay->resize(imageLabel->size());
        overlay->move(0, 0);  // 相对于 imageLabel
        overlay->raise();
        overlay->show();

        //为imagelabel安装过滤器，监视overlay
        imageLabel->installEventFilter(overlay);
    });

    if(dialog->exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QVector<QPointF> finalCorner= dialog->getOverLay()->getCornerPoints();
        //创建角点列表，填入
        QStringList fourCorners;
        for(int i=0;i<4; i++)
        {
            fourCorners << QString::number(finalCorner[i].x(),'f',2);
            fourCorners << QString::number(finalCorner[i].y(),'f',2);
        }
        sendCommandPacket("perspectiveTransform",fourCorners);
    }

}

//-----------------------------------------------CV-sobel边缘检测------------------------------------------------------------------
void MainWindow::on_cmd_SobelButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
                                               {"ksize","卷积核大小(<31)",ParamDialog::lineEdit},
                                               {"scale","梯度缩放因子",ParamDialog::lineEdit},
                                               };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        int ksize = params[0].toInt();
        if(ksize>31)
        {
            QMessageBox::warning(this,"错误","卷积核最大支持31");
        }
        else
        {qDebug()<<"输入参数："<<params;
            //发送带有参数的命令
            sendCommandPacket("sobel",params);
        }
    }
}

//-------------------------------------------------CV-laplace边缘检测--------------------------------------------------------------
void MainWindow::on_cmd_LaplacianButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
                                               {"ksize","卷积核大小(<31)",ParamDialog::lineEdit},
                                               {"scale","梯度缩放因子",ParamDialog::lineEdit},
                                               };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        int ksize = params[0].toInt();
        if(ksize>31)
        {
            QMessageBox::warning(this,"错误","卷积核最大支持31");
        }
        else
        {qDebug()<<"输入参数："<<params;
            //发送带有参数的命令
            sendCommandPacket("laplacian",params);
        }
    }
}

//---------------------------------------------CV高斯滤波-------------------------------------------------------------------------
void MainWindow::on_cmd_gaussianBlurButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {
        {"ksizeX","卷积核宽度",ParamDialog::lineEdit},
        {"ksizeY","卷积核高度",ParamDialog::lineEdit},
        {"sigmaX","x方向标准差",ParamDialog::lineEdit},
        {"sigmaY","y方向标准差",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        int ksizeX = params[0].toInt();
        int ksizeY = params[1].toInt();
        if(ksizeX>31 || ksizeY>31)
        {
            QMessageBox::warning(this,"错误","卷积核最大支持31");
        }
        else
        {qDebug()<<"输入参数："<<params;
            //发送带有参数的命令
            sendCommandPacket("gaussian",params);
        }
    }
}
//----------------------------------------------cv双边滤波---------------------------------------------------------------------
void MainWindow::on_cmd_bilateralFilButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //创建要显示的参数
    QList<ParamDialog::ParamSpec> paramSpec = {

        {"sigmaColor","颜色距离标准差",ParamDialog::lineEdit},
        {"sigmaSpace","空间距离标准差",ParamDialog::lineEdit}
    };
    //创建新窗口,传入上面的结构体列表，每个结构体包含两个字段，分别是参数和提示信息
    ParamDialog dialog(paramSpec,this);

    //如果按下确定
    if(dialog.exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        QStringList params = dialog.getParams();
        qDebug()<<"输入参数："<<params;
            //发送带有参数的命令
        sendCommandPacket("bilateralFilter",params);
    }
}

//------------------------------------------------CV直方图均衡化-------------------------------------------------------------------
void MainWindow::on_cmd_equalizeButton_clicked()
{
    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "无图片", "请先选择一张图片");
        return;
    }
    if (!sended) {
        QMessageBox::warning(this, "未上传", "请向服务器发送选择好的图片");
        return;
    }
    //判断是否为灰度图
    QImage image = QImage::fromData(imageData);
    if(image.isGrayscale())
        sendCommandPacket("equalizeHist",{});
    else
        QMessageBox::warning(this,"非灰度图","请确保待处理的图片为灰度图");
}

//---------------------------------------------CV反向投影进行目标检测----------------------------------------------------------------
void MainWindow::on_cmd_backProjectButton_clicked()
{
    if(templateData.isNull())
    {
        QMessageBox::warning(this,"未选取目标","请先选取目标特征用于反向投影");
        return;
    }

    imageOperateDialog *dialog = new imageOperateDialog(this);
    QImage operateImage = QImage::fromData(imageData, "jpg");
    dialog->setOperateImage(operateImage);
    dialog->showSilder();//需要滑动条显示
    //找到两个子控件
    QLabel *imageLabel = dialog->findChild<QLabel*>("imageLabelOnDialog");
    QLabel *tipsLabel = dialog->findChild<QLabel*>("tipsLabel");
    QSlider *slider = dialog->findChild<QSlider*>("histSizeSlider");

    tipsLabel->setText("使用页面上方滑动条调整histSize，按'OK'发送至服务器处理");//在底部添加提示信息

    if(dialog->exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
    {
        sendCommandPacket("backProject",{QString::number(slider->value())});
    }
}

//-------------------------------------------反向投影特征选取-----------------------------------------------------------------------
void MainWindow::on_cmd_uploadHistTemplateButton_clicked()
{

    QString fileName = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.bmp)");
    if (!fileName.isEmpty()) {
        QImage uploadedHistTemplateImage(fileName);
        if (uploadedHistTemplateImage.isNull()) {
            QMessageBox::warning(this, "错误", "无法加载图片");
            return;
        }

        imageOperateDialog *dialog = new imageOperateDialog(this);
        //设置选择好的图片
        dialog->setOperateImage(uploadedHistTemplateImage);

        //找到两个子控件
        QLabel *imageLabel = dialog->findChild<QLabel*>("imageLabelOnDialog");
        QLabel *tipsLabel = dialog->findChild<QLabel*>("tipsLabel");

        tipsLabel->setText("框选需要用于反向投影的目标特征区域，按'OK'保存");//在底部添加提示信息

        // 延迟添加 overlay，等生成imagelabel后再overlay
        QTimer::singleShot(0, [dialog, imageLabel]() {
            rubberBandSelector * rectSelector = new rubberBandSelector(imageLabel);
            // rectSelector->resize(imageLabel->size());
            rectSelector->move(0, 0);  // 相对于 imageLabel
            rectSelector->resize(imageLabel->size());
            rectSelector->raise();
            rectSelector->show();

            //为imagelabel安装过滤器，监视子对象
            imageLabel->installEventFilter(rectSelector);
        });

        if(dialog->exec() == QDialog::Accepted)//exec()阻塞形式弹出对话框，如果返回值为确定
        {
            QRectF finalSelectedRect = dialog->getSelector()->selectedRect();
            qDebug()<<"主函数调用，取得的rect为："<<finalSelectedRect<<"左上角："<<finalSelectedRect.topLeft()<<" 右下角："<<finalSelectedRect.bottomRight();
            //调用保存和发送接口
            saveAndSendHistTemplate(finalSelectedRect,uploadedHistTemplateImage);
        }
    }
}

//------------------------------------查看截取的特征图---------------------------------------
void MainWindow::on_cmd_showUploadTemplate_clicked()
{
    if(!templateData.isNull())
    {
        singleImageDialog *singleImage = new singleImageDialog(this);
        singleImage->setSingleImage(templateData);
        singleImage->exec();
    }
    else
        QMessageBox::warning(this,"未选取","未选取特征区域用于反向投影");
}

