#ifndef IMAGEOPERATEDIALOG_H
#define IMAGEOPERATEDIALOG_H

#include <QDialog>
#include<QImage>
#include<QMainWindow>
#include<QVector>
#include<QPoint>
#include "conerrectoverlay.h"
#include"rubberbandselector.h"

namespace Ui {
class imageOperateDialog;
}

class imageOperateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit imageOperateDialog(QWidget *parent = nullptr);
    ~imageOperateDialog();

    void setOperateImage(const QImage &image);//设置图片
    cornerRectOverlay *getOverLay() const;//向外暴露接口,传出上面覆盖的widget选择框对象
    rubberBandSelector *getSelector() const;//传出拉取框信息
    void showSilder();//当前需要滑动条
    QString getHistSize();//向外传histSize参数

private slots:

    void on_histSizeSlider_valueChanged(int value);

private:
    Ui::imageOperateDialog *ui;
    QImage originalImage;
    QVector<QPointF> controlPoints;//存储角点
};

#endif // IMAGEOPERATEDIALOG_H
