#include "singleimagedialog.h"
#include "ui_singleimagedialog.h"

singleImageDialog::singleImageDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::singleImageDialog)
{
    ui->setupUi(this);
    ui->label->resize(parent->size());
}

singleImageDialog::~singleImageDialog()
{
    delete ui;
}

void singleImageDialog::setSingleImage(const QImage &img)
{
    ui->label->setImage(img);
}
