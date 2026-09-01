#include "conerrectoverlay.h"
cornerRectOverlay::cornerRectOverlay(QWidget *parent)
    : QWidget{parent},selectedCornerIndex(-1)
{
    setAttribute(Qt::WA_TransparentForMouseEvents,false);
    setAttribute(Qt::WA_TranslucentBackground,true);//半透明
    setMouseTracking(true);
    //初始化为父控件大小
    resize(parent->size());

    //默认角点
    cornerPoints ={
        {50,50},{200,50},{200,200},{50,200}
    };
}
//---------------------------------------------向外部留出角点接口-----------------------------------------------------------------------
QVector<QPointF> cornerRectOverlay::getCornerPoints() const
{
    QVector<QPointF> cornerOnImage(4);
    imageLabel *parentLabel = qobject_cast<imageLabel*>(parent());
    for(int i=0;i<4;i++)
    {
        cornerOnImage[i] = parentLabel->labelPoint2ImagePoint(cornerPoints[i]);
        qDebug()<<"第"<<i<<"个角点"<<cornerOnImage[i];
    }
    return cornerOnImage;
}
//-------------------------------------------------绘制矫正框-------------------------------------------------------------------------
void cornerRectOverlay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    //设置画笔
    painter.setPen(QPen(Qt::green,2));
    //开始绘制线框
    for(int i=0;i<4;i++)
    {
        painter.drawLine(cornerPoints[i],cornerPoints[(i+1)%4]);
    }
    //绘制角点
    painter.setBrush(Qt::red);
    for(const QPointF &pt : cornerPoints)
        painter.drawEllipse(pt,cornerRadius,cornerRadius);
}
//---------------------------------------选中一个角点，准备拖动---------------------------------------------------------------------
void cornerRectOverlay::mousePressEvent(QMouseEvent *event)
{
    selectedCornerIndex = hitTest(event->pos());//按住鼠标时，获得当前是操作的哪个点的坐标
}
//------------------------------------------------鼠标拖拽的同时绘制----------------------------------------------------------------
void cornerRectOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if(selectedCornerIndex!=-1)//如果对有效角点操作
    {
        cornerPoints[selectedCornerIndex] = event->pos();
        update();
    }
}
//-----------------------------------------松开鼠标，保存坐标重置序号----------------------------------------------------------
void cornerRectOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    selectedCornerIndex = -1;
}
//-----------------------------------------安装事件过滤，父类缩放时同时缩放---------------------------------------------
bool cornerRectOverlay::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == parent() && event->type()==QEvent::Resize)
    {
        QWidget *parentWidget= qobject_cast<QWidget*>(parent());
        if(parentWidget)
            this->resize(parentWidget->size());
    }
    return QWidget::eventFilter(watched,event);
}

//--------------------------------------判断当前选择的角点序号-----------------------------------------------------
int cornerRectOverlay::hitTest(const QPointF &point)
{
    for(int i = 0;i<cornerPoints.size();i++)
        if(QLineF(point,cornerPoints[i]).length() <= cornerRadius+2)//初始圆圈与鼠标指针间的距离小于某个值时
            return i;//返回，这是操作的第i个角点
    return -1;
}
