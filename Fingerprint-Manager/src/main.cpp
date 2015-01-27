#include "fingerprintmanager.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	FingerprintManager w;
	w.show();
	
	return a.exec();
}
