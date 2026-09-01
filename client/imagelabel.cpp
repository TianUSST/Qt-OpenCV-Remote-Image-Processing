#include "imagelabel.h"

void imageLabel::setImage(const QImage &img)
{
    image = img;

    setPixmap(QPixmap::fromImage(img).scaled(this->size(), Qt::KeepAspectRatio));
    setAlignment(Qt::AlignCenter);
    update();//更新绘制
}

QPointF imageLabel::labelPoint2ImagePoint(const QPointF &labelPoint)
{
    qDebug()<<"映射函数内部调试----------------------------------当前点："<<labelPoint;
    int imgW = image.width();//原图宽度
    int imgH = image.height();//原图高度
    int labelW = this->width();//标签宽度
    int labelH = this->height();//标签高度
    qDebug()<<"图像宽："<<imgW<<" 高："<<imgH;
    qDebug()<<"标签宽："<<labelW<<" 高："<<labelH;
    //缩放后的图像尺寸
    QSize scaledSize = image.size();
    scaledSize.scale(labelW,labelH,Qt::KeepAspectRatio);//当前图片在label内的显示大小
        //计算padding
    int padX = (labelW - scaledSize.width())/2;//水平方向
    int padY = (labelH - scaledSize.height())/2;//垂直方向
    qDebug()<<"将图片放到标签内的paddingX:"<<padX<<" Y:"<<padY;
    //计算去除padding后的坐标
    int xInImage = labelPoint.x() - padX;
    int yInImage = labelPoint.y() - padY;
    qDebug()<<"去除padding后的坐标：（"<<xInImage<<","<<yInImage<<")";
        //缩放回图像内坐标
    double scaleX = double(imgW)/scaledSize.width();
    double scaleY = double(imgH)/scaledSize.height();
    qDebug()<<"当前缩放值scaleX："<<scaleX<<",scaleY:"<<scaleY;
    int imgX = int(xInImage*scaleX);
    int imgY = int(yInImage*scaleY);
    qDebug()<<"最终映射后坐标：("<<imgX<<","<<imgY<<")";
    qDebug()<<"映射内部调试结束-------------------------------------------";

    return QPointF(imgX,imgY);
}

QPoint imageLabel::labelPoint2ImagePoint2(const QPoint &labelPoint)
{
    qDebug()<<"映射函数内部调试----------------------------------当前点："<<labelPoint;
    int imgW = image.width();//原图宽度
    int imgH = image.height();//原图高度
    qDebug()<<"图像宽："<<imgW<<" 高："<<imgH;
    int labelW = this->width();//标签宽度
    int labelH = this->height();//标签高度
    qDebug()<<"标签宽："<<labelW<<" 高："<<labelH;

    //缩放后的图像尺寸
    QSize scaledSize = image.size();
    scaledSize.scale(labelW,labelH,Qt::KeepAspectRatio);//当前图片在label内的显示大小
        //计算padding
    int padX = (labelW - scaledSize.width())/2;//水平方向
    int padY = (labelH - scaledSize.height())/2;//垂直方向
    qDebug()<<"将图片放到标签内的paddingX:"<<padX<<" Y:"<<padY;
    //计算去除padding后的坐标
    int xInImage = labelPoint.x() - padX;
    int yInImage = labelPoint.y() - padY;
    qDebug()<<"去除padding后的坐标：（"<<xInImage<<","<<yInImage<<")";
        //缩放回图像内坐标
    int scaleX = int(imgW)/scaledSize.width();
    int scaleY = int(imgH)/scaledSize.height();
    qDebug()<<"当前缩放值scaleX："<<scaleX<<",scaleY:"<<scaleY;
    int imgX = int(xInImage*scaleX);
    int imgY = int(yInImage*scaleY);
    qDebug()<<"最终映射后坐标：("<<imgX<<","<<imgY<<")";
    qDebug()<<"映射内部调试结束-------------------------------------------";

    return QPoint(imgX,imgY);
}


void imageLabel::mouseMoveEvent(QMouseEvent *event)
{
    if(!image.isNull())
    {
        mousePos = event->pos();//获取鼠标所指坐标位置
//*******************************************************************
        // // 根据缩放比例映射坐标（假设等比例缩放）
        // double xRatio = double(image.width()) / this->width();
        // double yRatio = double(image.height()) / this->height();
        // int imgX = int(mousePos.x() * xRatio);
        // int imgY = int(mousePos.y() * yRatio);
//*******************************************************************************
        //修改后的坐标映射
        int imgW = image.width();//原图宽度
        int imgH = image.height();//原图高度
        int labelW = this->width();//标签宽度
        int labelH = this->height();//标签高度
            //缩放后的图像尺寸
        QSize scaledSize = image.size();
        scaledSize.scale(labelW,labelH,Qt::KeepAspectRatio);//当前图片在label内的显示大小
            //计算padding
        int padX = (labelW - scaledSize.width())/2;//水平方向
        int padY = (labelH - scaledSize.height())/2;//垂直方向
            //判断mousePos是否在图像区域内
        if(mousePos.x()<padX || mousePos.x()>padX+scaledSize.width() || mousePos.y()<padY || mousePos.y()>padY+scaledSize.height())
            return;//无事发生
            //计算去除padding后的坐标
        int xInImage = mousePos.x() - padX;
        int yInImage = mousePos.y() - padY;
            //缩放回图像内坐标
        double scaleX = double(imgW)/scaledSize.width();
        double scaleY = double(imgH)/scaledSize.height();
        int imgX = int(xInImage*scaleX);
        int imgY = int(yInImage*scaleY);

        //输出坐标的像素值
        if(imgX>=0 &&imgX<imgW && imgY>=0 &&imgY<imgH)
        {
            QColor color = image.pixelColor(imgX,imgY);
            //发出信号
            emit pixelHovered(imgX,imgY,color);
        }

    }
    showCrossHair = true;
    update();//更新绘制
}

void imageLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);//保留原始事件
    if(!image.isNull())
    {
        setPixmap(QPixmap::fromImage(image).scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    update();
}

void imageLabel::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);//保留原始绘图事件
    //允许绘制且存在图片
    if(showCrossHair && !image.isNull())
    {
    //创建一个绘图器
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);//渲染相关
    painter.setPen(QPen(Qt::red,1,Qt::DashLine));
    //获取当前坐标
    int x = mousePos.x();
    int y = mousePos.y();
    //绘制交线
    painter.drawLine(0,y,width(),y);//横线
    painter.drawLine(x,0,x,height());//竖线
    }

}
