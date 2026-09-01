#ifndef CONERRECTOVERLAY_H
#define CONERRECTOVERLAY_H

#include <QWidget>
#include <QLabel>
#include <QMouseEvent>
#include <QPoint>
#include <QVector>
#include <QPainter>
#include <QPen>
#include"imagelabel.h"
//*******************************在图像1畸变矫正时生成的窗口************************************
class cornerRectOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit cornerRectOverlay(QWidget *parent = nullptr);

    QVector<QPointF> getCornerPoints() const;
protected:
    void paintEvent(QPaintEvent *event) override;//根据当前角点画框
    void mousePressEvent(QMouseEvent *event) override;//1、按住鼠标
    void mouseMoveEvent(QMouseEvent *event) override;//2、拖动鼠标
    void mouseReleaseEvent(QMouseEvent *event) override;//3、释放鼠标
    bool eventFilter(QObject *watched,QEvent *event) override;//重写事件，跟随父窗口缩放
private:
    int selectedCornerIndex;
    QVector<QPointF> cornerPoints; // 四个角点
    int cornerRadius = 6;
    int hitTest(const QPointF &point);//检测当前点是否为4个点之一
signals:
};

#endif // CONERRECTOVERLAY_H
