#include "dialogaddfinger.h"
#include "ui_dialogaddfinger.h"

#include <QMessageBox>


DialogAddFinger::DialogAddFinger(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::dialogAddFinger)
{
	ui->setupUi(this);
}

DialogAddFinger::~DialogAddFinger()
{
	delete ui;
}

void DialogAddFinger::on_checkBox_groupleader_clicked(bool checked)
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

void DialogAddFinger::on_pushButton_clicked()
{
	// check name string
	name=ui->lineEdit_name->text();
	if(name.isEmpty())
	{
		QMessageBox::warning(this, tr("kein Name angegeben"), tr("Bitte geben Sie erst einen Namen ein."));
		return;
	}
	
	for(int i=0; i<name.length(); i++)
	{
		int ch=name.at(i).unicode();
		
		if(!((ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || (ch>='0' && ch<='9') || (ch=='_')))	// invalid character
		{
			QMessageBox::warning(this, tr("nicht erlaubtes Zeichen"), tr("Umlaute, Leerzeichen und Sonderzeichen außer _ sind leider nicht möglich."));
			return;
		}
	}
	
	time=ui->timeEdit_time->time();
	
	QDialog::accept();
}
