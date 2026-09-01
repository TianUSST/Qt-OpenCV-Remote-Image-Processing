#ifndef SINGLEIMAGEDIALOG_H
#define SINGLEIMAGEDIALOG_H

#include <QDialog>

namespace Ui {
class singleImageDialog;
}

class singleImageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit singleImageDialog(QWidget *parent = nullptr);
    ~singleImageDialog();
       void setSingleImage(const QImage &img);//设置图片

private:
    Ui::singleImageDialog *ui;
};

#endif // SINGLEIMAGEDIALOG_H
