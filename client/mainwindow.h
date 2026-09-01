#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QTcpSocket>
#include<QImage>
#include"paramdialog.h"
#include"imagelabel.h"
#include<QDateTime>
#include"customkerneldialog.h"
#include"imageoperatedialog.h"
#include"conerrectoverlay.h"
#include"rubberbandselector.h"
#include<QTimer>
#include<QBuffer>
#include"singleimagedialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    QSize getImageLabelSize();
    ~MainWindow();

signals:
    void imageSendSuccess();
    void imageClear();
    void checkServerHasImage();

private slots:
    void on_openButton_clicked();//打开图片
    void on_connectButton_clicked();//连接服务器
    void on_sendButton_clicked();//发送图片
    void on_socketConnected();//提示服务器连接状态
    void on_socketDisconnected();
    void on_disconnectButton_clicked();
    void on_socketReadyRead();
    void on_socketError(QAbstractSocket::SocketError socketError);

    void on_clearButton_clicked();//清空当前打开的图片
    void on_cmd_grayButton_clicked();//灰度化
    void on_cmd_flipxButton_clicked();//翻转
    void on_cmd_flipyButton_clicked();
    void on_cmd_blurButton_clicked();//滤波
    void on_cmd_CannyButton_clicked();

    void on_continueButton_clicked();//将预览区图片放入待处理区

    void enableProcessButton();//通过信号设定按钮可用性
    void unenableProcessButton();

    void on_saveButton_clicked();//保存预览区图片



    void displayWheatherServerHasImage();//显示服务器是否收到图片
    void displayServerStatus();//显示是否连接到服务器
    void displayRGB(int x,int y,QColor c);//显示待处理区RGB
    void displayRGB2(int x,int y,QColor c);//显示预览区RGB


    void on_cmd_threshHoldButton_clicked();//阈值化

    void on_cmd_adaptiveButton_clicked();//自适应阈值化

    void on_cmd_medianBlurButton_clicked();//中值滤波

    void on_cmd_customButton_clicked();//创建自定义卷积核

    void on_cmd_showCustomButton_clicked();//显示自定义卷积核

    void on_cmd_deleteCustomButton_clicked();//删除自定义卷积核

    void on_cmd_customFilterButton_clicked();//自定义滤波

    void on_cmd_dilateButton_clicked();//膨胀

    void on_cmd_erodeButton_clicked();//腐蚀

    void on_cmd_openButton_clicked();//开运算

    void on_cmd_closeButton_clicked();//闭运算

    void on_cmd_tophatButton_clicked();//顶帽

    void on_cmd_blackhatButton_clicked();//黑帽

    void on_cmd_gradientButton_clicked();//形态学梯度

    void on_cmd_perspectiveButton_clicked();//透视变换

    void on_cmd_SobelButton_clicked();//sobel边缘检测

    void on_cmd_LaplacianButton_clicked();//拉普拉斯滤波

    void on_cmd_gaussianBlurButton_clicked();//高斯滤波

    void on_cmd_bilateralFilButton_clicked();//双边滤波

    void on_cmd_equalizeButton_clicked();//直方图均衡

    void on_cmd_backProjectButton_clicked();//反向投影，前置：templateData非空


    void on_cmd_uploadHistTemplateButton_clicked();

    void on_cmd_showUploadTemplate_clicked();

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    QByteArray imageData;//待处理区图片数据
    QByteArray retImageData;//预览区数据
    bool sended=false;//服务器是否接受到图片
    bool reSend=false;//是否是将预览区图片再次发送
    QSize displaySize;
    void appendLog(QString message);//日志显示
    QVector<QVector<float>> customKernel;//保存当前自定义卷积核
    QImage templateData;//模板数据，用于反向投影和模板匹配

    void sendCommandPacket(const QString& commandText,const QStringList &args);
    QStringList kernelList(QVector<QVector<float>> &kernel);

    void saveAndSendHistTemplate(const QRectF& rectOnImage,const QImage &histTemplateImage);
};
#endif // MAINWINDOW_H
