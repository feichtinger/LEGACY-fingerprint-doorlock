#include "fingerprintmanager.h"
#include "ui_fingerprintmanager.h"

#include <QMessageBox>
#include <QSerialPort>


FingerprintManager::FingerprintManager(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::FingerprintManager)
{
	ui->setupUi(this);
	
	serial=new QSerialPort(this);
	dialogAddFinger=new DialogAddFinger(this);
	
	connect(dialogAddFinger, SIGNAL(accepted()), this, SLOT(addFinger()));
	connect(serial, SIGNAL(readyRead()), this, SLOT(readSerial));
}

FingerprintManager::~FingerprintManager()
{
	delete ui;
}

void FingerprintManager::on_pushButton_connect_clicked()
{
	if(serial->isOpen())
	{
		// port is open -> close it
		serial->close();
		ui->pushButton_connect->setText(tr("Verbinden"));
		ui->pushButton_addFinger->setEnabled(false);
		ui->pushButton_update->setEnabled(false);
		ui->pushButton_abort->setEnabled(false);
	}
	else
	{
		// port is closed -> try to open it
		serial->setPortName(ui->lineEdit_port->text());
		if(/*serial.open(QIODevice::ReadWrite)*/true)
		{
			// port open
			serial->setBaudRate(QSerialPort::Baud115200);
			serial->setDataBits(QSerialPort::Data8);
			ui->pushButton_connect->setText(tr("Trennen"));
			ui->pushButton_addFinger->setEnabled(true);
			ui->pushButton_update->setEnabled(true);
			ui->pushButton_abort->setEnabled(true);
		}
		else
		{
			QMessageBox::warning(this, tr("Schnittstellenfehler"), tr("Serielle Schnittstelle konnte nicht geöffnet werden.\nÜberprüfen Sie den Portnamen und Steckverbinder."));
		}
	}
}


void FingerprintManager::on_pushButton_update_clicked()
{
	serial->write("list\n");
	
	readSerial();
}


void FingerprintManager::on_pushButton_addFinger_clicked()
{
    dialogAddFinger->exec();
}


void FingerprintManager::addFinger()
{
	serial->write("enroll ");
	serial->write(dialogAddFinger->getName().toLatin1());
	serial->write(" ");
	int time_min = dialogAddFinger->getTime().minute() + dialogAddFinger->getTime().hour()*60;
	serial->write(QString::number(time_min).toLatin1());
	serial->write("\n");
}


void FingerprintManager::on_pushButton_abort_clicked()
{
    serial->write("abort\n");
}


void FingerprintManager::readSerial()
{
	static QString buffer;
	buffer.append(serial->readAll());
	//buffer=QString("1.Zeile\n2.Zeile\n3.Zeile\n");
	
	QStringList list=buffer.split(QChar('\n'));
	
	for(int i=0; i<list.length()-1; i++)
	{
		QString line=list.at(i);
		ui->plainTextEdit_messages->appendPlainText(line);
		
		// todo parse line
	}
	
	buffer.clear();
	buffer.append(list.last());
}
