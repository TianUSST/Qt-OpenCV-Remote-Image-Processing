#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QWidget>
#include<QMouseEvent>
#include<QPaintEvent>
#include<QPainter>


class imageLabel : public QLabel
{
    Q_OBJECT
public:
    explicit imageLabel(QWidget *parent = nullptr) : QLabel(parent)
    {
        setMouseTracking(true);
    };

    void setImage(const QImage &img);//在标签上设置图片
    QPointF labelPoint2ImagePoint(const QPointF &labelPoint);//提供接口，坐标系映射
    QPoint labelPoint2ImagePoint2(const QPoint &labelPoint);
private:
    QImage image;//当前设置图片
    QPoint mousePos;//鼠标位置坐标
    bool showCrossHair = false;//默认不绘制交线
protected:
    void mouseMoveEvent(QMouseEvent *event) override;//重写鼠标事件，获取对应像素的RGB
    void resizeEvent(QResizeEvent *event) override;//重写缩放，图片自动适应当前label大小
    void paintEvent(QPaintEvent *event) override;//以鼠标为中心绘制坐标
signals:
    void pixelHovered(int x,int y,QColor c);
};

#endif // IMAGELABEL_H
