#include "fingerprintmanager.h"
#include "ui_fingerprintmanager.h"

#include <QMessageBox>
#include <QSerialPort>
#include <QDebug>


FingerprintManager::FingerprintManager(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::FingerprintManager)
{
	setWindowIcon(QIcon("Fingerprint.svg"));
	ui->setupUi(this);
	
	serial=new QSerialPort(this);
	dialogAddFinger=new DialogAddFinger(this);
	dialogSetOpenTime=new DialogSetOpenTime(this);
	
	connect(serial, SIGNAL(readyRead()), this, SLOT(readSerial()));
	connect(ui->tableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(tableContextMenu(QPoint)));
	
	selectedID=-1;
	
	ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
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
		ui->lineEdit_port->setEnabled(true);
		ui->tableWidget->setEnabled(false);
		ui->pushButton_addFinger->setEnabled(false);
		ui->pushButton_update->setEnabled(false);
		ui->pushButton_abort->setEnabled(false);
	}
	else
	{
		// port is closed -> try to open it
		serial->setPortName(ui->lineEdit_port->text());
		if(serial->open(QIODevice::ReadWrite))
		{
			// port open
			serial->setBaudRate(QSerialPort::Baud115200);
			serial->setDataBits(QSerialPort::Data8);
			ui->pushButton_connect->setText(tr("Trennen"));
			ui->lineEdit_port->setEnabled(false);
			ui->tableWidget->setEnabled(true);
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
	ui->tableWidget->clearContents();
	serial->write("list\n");
	
	// for debugging only
	//readSerial();
}


void FingerprintManager::on_pushButton_addFinger_clicked()
{
	if(dialogAddFinger->exec() == QDialog::Accepted)
	{
		// enroll new finger
		// syntax: "enroll <name> [time]"
		// time is the maximum open time in minutes
		serial->write("enroll ");
		serial->write(dialogAddFinger->getName().toLatin1());
		serial->write(" ");
		int time_min = dialogAddFinger->getTime().minute() + dialogAddFinger->getTime().hour()*60;
		serial->write(QString::number(time_min).toLatin1());
		serial->write("\n");
	}
}


void FingerprintManager::on_pushButton_abort_clicked()
{
    serial->write("abort\n");
}


void FingerprintManager::readSerial()
{
	static QString buffer;
	buffer.append(serial->readAll());
	
	QStringList list=buffer.split(QChar('\n'));
	
	for(int i=0; i<list.length()-1; i++)
	{
		QString line=list.at(i);
		ui->plainTextEdit_messages->appendPlainText(line);
		
		// parse line
		if(line.startsWith("id:"))
		{
			parseListEntry(line);
		}
	}
	
	buffer.clear();
	buffer.append(list.last());		// keep the unparsed rest for the next time
}


void FingerprintManager::parseListEntry(QString line)
{
	// this is a 'list' output -> fill data in table
	// syntax: "id: <id> <name> <time>"
	QStringList l=line.split(" ");
	
	if(l.size()>=3)
	{
		int id=l.at(1).toInt();
		QString name=l.at(2);
		QTime time=QTime(0, 0);
		if(l.size()>=4)
		{
			time=time.addSecs(l.at(3).toInt()*60);
		}
		
		if(id>=ui->tableWidget->rowCount())
		{
			// we are at the end of the table -> increase number of rows
			int oldRowCount=ui->tableWidget->rowCount();
			int newRowCount=id+1;
			
			ui->tableWidget->setRowCount(newRowCount);
			
			// add new rows
			for(int i=oldRowCount; i<newRowCount; i++)
			{
				// set Header
				QTableWidgetItem* vertHeader=new QTableWidgetItem(QString::number(i));
				ui->tableWidget->setVerticalHeaderItem(i, vertHeader);
				
				// insert not editable dummy items
				QTableWidgetItem* dummyName=new QTableWidgetItem(QString());
				dummyName->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				ui->tableWidget->setItem(i, 0, dummyName);
				
				QTableWidgetItem* dummyTime=new QTableWidgetItem(QString());
				dummyTime->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				ui->tableWidget->setItem(i, 1, dummyTime);
			}
		}
		
		// insert name
		QTableWidgetItem* itemName=new QTableWidgetItem(name);
		itemName->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui->tableWidget->setItem(id, 0, itemName);
		
		// insert time
		if(!(time==QTime(0,0)))
		{
			QTableWidgetItem* itemTime=new QTableWidgetItem(time.toString("HH:mm"));
			itemTime->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			ui->tableWidget->setItem(id, 1, itemTime);
		}
	}
}


void FingerprintManager::tableContextMenu(QPoint p)
{
	QTableWidgetItem* item=ui->tableWidget->itemAt(p);
	
	if(item!=0)
	{
		int id=ui->tableWidget->row(item);
		QString name=ui->tableWidget->item(id, 0)->text();
		
		if(!name.isEmpty())
		{
			selectedID=id;
			
			QMenu *menu=new QMenu(this);
			menu->addAction(QIcon(), tr("Finger löschen"), this, SLOT(deleteFinger()));
			menu->addAction(QIcon(), tr("Öffnungszeit ändern"), this, SLOT(setOpenTime()));
			menu->popup(ui->tableWidget->viewport()->mapToGlobal(p));
		}
	}
}


void FingerprintManager::deleteFinger()
{
	QString name=ui->tableWidget->item(selectedID, 0)->text();
	if(!name.isEmpty())
	{
		if(QMessageBox::question(this, tr("Löschen"), tr("Wollen Sie %1 wirklich löschen?").arg(name))==QMessageBox::Yes)
		{
			serial->write("delete ");
			serial->write(name.toLatin1());
			serial->write(" \n");
		}
	}
}


void FingerprintManager::setOpenTime()
{
	QString name=ui->tableWidget->item(selectedID, 0)->text();
	QString timeString=ui->tableWidget->item(selectedID, 1)->text();
	QTime time=QTime(0,0);
	if(!timeString.isEmpty())
	{
		time=QTime::fromString(timeString);
	}
	
	dialogSetOpenTime->setTime(time);
	dialogSetOpenTime->setName(name);
	
	if(dialogSetOpenTime->exec()==QDialog::Accepted)
	{
		time=dialogSetOpenTime->getTime();
		
		serial->write("set_open_time ");
		serial->write(name.toLatin1());
		serial->write(" ");
		int time_min = time.minute() + time.hour()*60;
		serial->write(QString::number(time_min).toLatin1());
		serial->write("\n");
	}
}


void FingerprintManager::closeEvent(QCloseEvent *event)
{
	serial->close();
	event->accept();
}


void FingerprintManager::on_lineEdit_search_editingFinished()
{
	QString searchText=ui->lineEdit_search->text();
	int row=ui->tableWidget->currentRow();
	
	for(int i=0; i<=ui->tableWidget->rowCount(); i++)
	{
		row++;
		if(row>=ui->tableWidget->rowCount())
		{
			row=0;
		}
		
		QString name=ui->tableWidget->item(row, 0)->text();
		if(name.contains(searchText, Qt::CaseInsensitive))
		{
			ui->tableWidget->selectRow(row);
			ui->tableWidget->setCurrentCell(row, 0);
			break;
		}
	}
}
