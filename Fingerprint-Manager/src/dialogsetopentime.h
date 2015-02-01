#ifndef DIALOGSETOPENTIME_H
#define DIALOGSETOPENTIME_H

#include <QDialog>
#include <QTime>

namespace Ui {
class DialogSetOpenTime;
}

class DialogSetOpenTime : public QDialog
{
	Q_OBJECT
	
public:
	explicit DialogSetOpenTime(QWidget *parent = 0);
	~DialogSetOpenTime();
	
	void setTime(QTime time);
	void setName(QString name);
	QTime getTime(void);
	
private slots:
	void on_checkBox_groupleader_clicked(bool checked);	
	
private:
	Ui::DialogSetOpenTime *ui;
};

#endif // DIALOGSETOPENTIME_H
