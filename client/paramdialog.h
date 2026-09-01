#ifndef PARAMDIALOG_H
#define PARAMDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QVector>
#include <QDialogButtonBox>
#include<QComboBox>
#include<QCheckBox>

namespace Ui {
class ParamDialog;
}


class ParamDialog : public QDialog
{
    Q_OBJECT

public:
    enum paramType
    {
        lineEdit,
        comboBox,
        checkBox
    };
    struct ParamSpec {
        QString name;        // 参数标识
        QString placeholder; // 提示信息
        // bool hasComboBox = false;//用于确定是否有可选项
        paramType type;
        QStringList options;//可选项
    };
    explicit ParamDialog(const QList<ParamSpec>& paramSpecs, QWidget *parent = nullptr);//创建该窗口时，应该传入参数标识和提示信息来创建输入框
    ~ParamDialog();
    QStringList getParams() const;//获取输入的参数进行发送

private:
    Ui::ParamDialog *ui;
    QVector<QWidget*> inputFields;//自动生成输入框
};

#endif // PARAMDIALOG_H
