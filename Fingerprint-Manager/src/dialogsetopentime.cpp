#include "dialogsetopentime.h"
#include "ui_dialogsetopentime.h"

DialogSetOpenTime::DialogSetOpenTime(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DialogSetOpenTime)
{
	ui->setupUi(this);
}


DialogSetOpenTime::~DialogSetOpenTime()
{
	delete ui;
}


void DialogSetOpenTime::setTime(QTime time)
{
	ui->timeEdit_time->setTime(time);
	if(time == QTime(0, 0))
	{
		ui->checkBox_groupleader->setChecked(false);
		ui->timeEdit_time->setEnabled(false);
	}
	else
	{
		ui->checkBox_groupleader->setChecked(true);
		ui->timeEdit_time->setEnabled(true);
	}
}


void DialogSetOpenTime::setName(QString name)
{
	ui->label_name->setText(name);
}


void DialogSetOpenTime::on_checkBox_groupleader_clicked(bool checked)
{
	if(checked)
	{
		ui->timeEdit_time->setEnabled(true);
	}
	else
	{
		ui->timeEdit_time->setEnabled(false);
	}
}


QTime DialogSetOpenTime::getTime(void)
{
	if(ui->checkBox_groupleader->isChecked())
	{
		return ui->timeEdit_time->time();
	}
	else
	{
		return QTime(0, 0);
	}
}
