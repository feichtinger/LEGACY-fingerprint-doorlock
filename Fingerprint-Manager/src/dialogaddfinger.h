#ifndef DIALOGADDFINGER_H
#define DIALOGADDFINGER_H

#include <QDialog>
#include <QTime>

namespace Ui {
class dialogAddFinger;
}

class DialogAddFinger : public QDialog
{
	Q_OBJECT
	
public:
	explicit DialogAddFinger(QWidget *parent = 0);
	~DialogAddFinger();
	
	QString getName(){return name;}
	QTime getTime(){return time;}
	
private slots:
	void on_checkBox_groupleader_clicked(bool checked);
	
	void on_pushButton_clicked();
	
private:
	Ui::dialogAddFinger *ui;
	
	QString name;
	QTime time;
};

#endif // DIALOGADDFINGER_H
