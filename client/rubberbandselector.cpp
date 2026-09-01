#include "rubberbandselector.h"

rubberBandSelector::rubberBandSelector(QWidget *parent)
    : QWidget{parent}
{
     qDebug() << "rubberBandSelector created with parent:" << parent;
    setAttribute(Qt::WA_TransparentForMouseEvents,false);
    setAttribute(Qt::WA_TranslucentBackground,true);//半透明
    resize(parent->size());
    // setMouseTracking(true);
}

QRectF rubberBandSelector::selectedRect() const
{
    return innerSelectedRect;
}

void rubberBandSelector::mousePressEvent(QMouseEvent * event)
{
    qDebug() << "mousePressEvent triggered at" << event->pos();
    origin = event->pos();//保存按下时的坐标
    if(rubberBand==nullptr)//创建rubberband
        rubberBand = new QRubberBand(QRubberBand::Rectangle,this);
    rubberBand->setGeometry(QRect(origin,QSize()));//坐标起点设置为初始点
    rubberBand->show();
    qDebug()<<"show()";
}

void rubberBandSelector::mouseMoveEvent(QMouseEvent *event)
{
    if(rubberBand)
        rubberBand->setGeometry(QRect(origin,event->pos()).normalized());//实时更新位置

}

void rubberBandSelector::mouseReleaseEvent(QMouseEvent *event)
{
    QRect selectedRect = rubberBand->geometry();
    qDebug()<<"松开--------------------------------------";
    qDebug()<<"当前右下角坐标："<<selectedRect.bottomRight();
    qDebug()<<"选中区域："<<selectedRect;
    innerSelectedRect = selectedRect;
    qDebug()<<"下面开始映射转换---------------------";
    labelRect2ImageRect();
}

void rubberBandSelector::labelRect2ImageRect()
{
    if(!innerSelectedRect.isEmpty())
    {
    imageLabel *parentLabel = qobject_cast<imageLabel*>(parent());
    QPointF x1y1 = innerSelectedRect.topLeft();
    QPointF x2y2 = innerSelectedRect.bottomRight();
    qDebug()<<"inner转换前左上角："<<x1y1;
    qDebug()<<"inner转换前右下角："<<x2y2;
    //取两点，映射后构建新矩形
    QPointF newX1Y1 = parentLabel->labelPoint2ImagePoint(x1y1);
    QPointF newX2Y2 = parentLabel->labelPoint2ImagePoint(x2y2);
    qDebug()<<"inner使用映射后的左上角："<<newX1Y1;
    qDebug()<<"inner使用映射后的右下角："<<newX2Y2;
    innerSelectedRect = QRectF(newX1Y1,newX2Y2);
    qDebug()<<"映射转换函数调用结束-----------------------------------------------------";
    }
}

QPointF rubberBandSelector::getStartPoint()
{
    qDebug()<<"startPoint"<<origin;
    return origin;

}

bool rubberBandSelector::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == parent() && event->type()==QEvent::Resize)
    {
        QWidget *parentWidget= qobject_cast<QWidget*>(parent());
        if(parentWidget)
            this->resize(parentWidget->size());
    }
    return QWidget::eventFilter(watched,event);
}
