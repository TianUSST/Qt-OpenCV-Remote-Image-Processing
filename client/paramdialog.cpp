#include "paramdialog.h"
#include "ui_paramdialog.h"
#include <QVBoxLayout>
#include <QLabel>

ParamDialog::ParamDialog(const QList<ParamSpec>& paramSpecs, QWidget *parent)
    : QDialog(parent), ui(new Ui::ParamDialog)
{
    ui->setupUi(this);//ui指向此窗口
    QVBoxLayout *mainLayout = new QVBoxLayout(this);//创建垂直布局
    //遍历传入的结构体列表,有几个参数，就创建几个提示信息+输入框
    for(const ParamSpec& spec : paramSpecs)
    {
        if(spec.type==comboBox && !spec.options.isEmpty())
        {
            //如果这个参数是可选项，则创建comboBox而不是LineEdit
            QComboBox *combo = new QComboBox(this);//创建控件
            combo->addItems(spec.options);
            //创建水平布局
            QHBoxLayout *rowLayout = new QHBoxLayout;
            QLabel *label = new QLabel(spec.name,this);//创建提示标签
            // QLineEdit *input = new QLineEdit(this);//创建输入框
            // input->setPlaceholderText(spec.placeholder);//输入框中设置提示信息
            //将每个遍历出的结构体中的两个控件做水平布局，中间添加弹簧
            rowLayout->addStretch(1);
            rowLayout->addWidget(label);
            rowLayout->addStretch(1);
            rowLayout->addWidget(combo);
            mainLayout->addLayout(rowLayout);
            inputFields.append(combo);
        }

        else if(spec.type == lineEdit)
        {
        //创建水平布局
        QHBoxLayout *rowLayout = new QHBoxLayout;
        QLabel *label = new QLabel(spec.name,this);//创建提示标签
        QLineEdit *input = new QLineEdit(this);//创建输入框
        input->setPlaceholderText(spec.placeholder);//输入框中设置提示信息
        //将每个遍历出的结构体中的两个控件做水平布局，中间添加弹簧
        rowLayout->addStretch(1);
        rowLayout->addWidget(label);
        rowLayout->addStretch(3);
        rowLayout->addWidget(input);
        //对总体做嵌套布局
        mainLayout->addLayout(rowLayout);
        inputFields.append(input);
        }

        else if(spec.type == checkBox)
        {

            QCheckBox *check = new QCheckBox(spec.name,this);
            mainLayout->addWidget(check);
            inputFields.append(check);
        }

    }
    //添加按钮
    QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |QDialogButtonBox::Cancel,this);
    mainLayout->addWidget(buttonBox);//将按钮加入整体布局
    //连接确认、取消信号
    connect(buttonBox,&QDialogButtonBox::accepted,this,&ParamDialog::accept);
    connect(buttonBox,&QDialogButtonBox::rejected,this,&ParamDialog::reject);

    setLayout(mainLayout);//设置布局

    //窗口初始化结束
}

ParamDialog::~ParamDialog()
{
    delete ui;
}
//----------------------------------------------------针对不同控件输入，进行参数收集-------------------------------------
QStringList ParamDialog::getParams()const
{
    QStringList result;//创建打包所有输入好的参数列表
    for(QWidget *widget :inputFields)//在该窗口所有的输入框中便利
    {
        if(QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget))//如果是lineedit
            result<<lineEdit->text();
        else if(QComboBox *combo = qobject_cast<QComboBox*>(widget))
            result<<QString::number(combo->currentIndex());
        else if(QCheckBox *check = qobject_cast<QCheckBox*>(widget))
            result<<(check->isChecked() ? "1" : "0");
    }

    return result;//返回一个QStringList,可以直接调用发送函数，加上分割附后发送
}
