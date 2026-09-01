#ifndef RUBBERBANDSELECTOR_H
#define RUBBERBANDSELECTOR_H

#include <QWidget>
#include<QRubberBand>
#include<QPoint>
#include<QMouseEvent>
#include"imagelabel.h"

class rubberBandSelector : public QWidget
{
    Q_OBJECT
public:
    explicit rubberBandSelector(QWidget *parent = nullptr);
    QRectF selectedRect() const;//外部接口，传出框选的特征区域
      void labelRect2ImageRect();
    QPointF getStartPoint();
private:
    QPoint origin;//选框的初始坐标
    QRubberBand *rubberBand=nullptr;//创建控件
    QRectF innerSelectedRect;//内部保存
    QRectF newRect;
protected:
    void mousePressEvent(QMouseEvent *event) override;//鼠标压下，准备拖拽
    void mouseMoveEvent(QMouseEvent *event) override;//鼠标拖动
    void mouseReleaseEvent(QMouseEvent *event) override;//鼠标松开

    bool eventFilter(QObject *watched,QEvent *event) override;//重写事件，跟随父窗口缩放
};

#endif // RUBBERBANDSELECTOR_H
