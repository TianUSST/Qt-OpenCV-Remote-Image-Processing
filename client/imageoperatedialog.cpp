#include "imageoperatedialog.h"
#include "ui_imageoperatedialog.h"
#include"mainwindow.h"

imageOperateDialog::imageOperateDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::imageOperateDialog)
{
    ui->setupUi(this);
    ui->histSizeLabel->hide();
    ui->histSizeSlider->hide();
    ui->histSizeLabel->setText("HistSize : null");
}

void imageOperateDialog::setOperateImage(const QImage &img)
{
    // //!!!还要对主界面的大小进行更新，保证发送出去的大小=显示大小，这样透视变换才正确
    // originalImage = img;
    // //获得主界面标签大小
    // MainWindow *parent =qobject_cast<MainWindow*>(this->parent());
    // QSize imgSize = parent->getImageLabelSize();
    // //显示同样大小的图片，这样传入的坐标才对应
    // ui->imageLabel->setPixmap(QPixmap::fromImage(originalImage).scaled(imgSize, Qt::KeepAspectRatio));
    // ui->imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel *label = this->findChild<imageLabel*>();
    label->setImage(img);
}

cornerRectOverlay *imageOperateDialog::getOverLay() const
{
    return ui->imageLabelOnDialog->findChild<cornerRectOverlay*>();
}

rubberBandSelector *imageOperateDialog::getSelector() const
{
    return ui->imageLabelOnDialog->findChild<rubberBandSelector*>();
}

void imageOperateDialog::showSilder()
{
    //构造函数默认隐藏，调用此方法后显示
    ui->histSizeLabel->show();
    ui->histSizeSlider->show();
}

QString imageOperateDialog::getHistSize()
{
    return QString::number(ui->histSizeSlider->value());
}

imageOperateDialog::~imageOperateDialog()
{
    delete ui;
}

void imageOperateDialog::on_histSizeSlider_valueChanged(int value)
{

    ui->histSizeLabel->setText("HistSize :"+QString::number(value));
}

