#include "customkerneldialog.h"
#include "ui_customkerneldialog.h"

customKernelDialog::customKernelDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::customKernelDialog)
{
    ui->setupUi(this);
}

//--------------------------------外部接口，用于获取当前保存的卷积核数据
QVector<QVector<float> > customKernelDialog::getKernelData() const
{
    if(!KernelData.isEmpty())
        return KernelData;
}

customKernelDialog::~customKernelDialog()
{
    delete ui;
}

//----------------------------根据当前spinBox设置表格大小
void customKernelDialog::on_generateTableButton_clicked()
{
    int rows = ui->rowSpinBox->value();
    int cols = ui->colSpinBox->value();
    ui->kernelTable->setRowCount(rows);
    ui->kernelTable->setColumnCount(cols);

}

//---------------------确定键保存当前输入的核
void customKernelDialog::on_kernelButtonBox_accepted()
{
    //保存，首先清空之前的数据
    KernelData.clear();
    //获取表格的行列数
    int rows=ui->kernelTable->rowCount();
    int cols=ui->kernelTable->columnCount();
    //初始化标志位无效，进行合法性判断
    isValidKernel = false;
    //逐行遍历
    for(int i=0;i<rows;i++)
    {
        QVector<float> row;//保存每行的数据
        for(int j=0;j<cols;j++)//对一行中的每个元素遍历
        {
            QTableWidgetItem *item = ui->kernelTable->item(i,j);//用item取出每个表格中的值
            //防止程序崩溃
            bool flag = false;//控制浮点转换
            float value = item?item->text().toFloat(&flag):0.0f;//先尝试转换
            if(flag && value!=0.0f)  isValidKernel = true;//转换失败，则默认值为0.0f
            row.append(value);

        }
        qDebug()<<row;
        KernelData.append(row);
    }
    if(!isValidKernel)
    {
        QMessageBox::warning(this,"无效的卷积核","至少输入一个非零值");
        return;
    }
}


void customKernelDialog::on_clearTableButton_clicked()
{

    ui->kernelTable->clear();
}

