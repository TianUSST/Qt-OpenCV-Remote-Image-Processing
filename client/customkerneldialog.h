#ifndef CUSTOMKERNELDIALOG_H
#define CUSTOMKERNELDIALOG_H

#include <QDialog>
#include<QSpinBox>
#include<QTabWidget>
#include<QLabel>
#include<QPushButton>
#include<QMessageBox>


namespace Ui {
class customKernelDialog;
}

class customKernelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit customKernelDialog(QWidget *parent = nullptr);
    QVector<QVector<float>> getKernelData() const;//外部接口，读取自定义卷积核
    bool isValidKernel;
    ~customKernelDialog();

private:
    Ui::customKernelDialog *ui;
    QVector<QVector<float>> KernelData;//保存自定义卷积核

private slots:
    // void acceptKernel();//保存自定义卷积核
    void on_generateTableButton_clicked();//生成输入表格
    void on_kernelButtonBox_accepted();
    void on_clearTableButton_clicked();
};

#endif // CUSTOMKERNELDIALOG_H
