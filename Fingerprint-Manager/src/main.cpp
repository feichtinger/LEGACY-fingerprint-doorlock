#include "fingerprintmanager.h"
#include <QApplication>
#include <QTextCodec>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QtDebug>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	//QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));		// all messages in tr() are interpreted in UTF-8 so the file encoding for all source-files should be UTF-8 too.
    QCoreApplication::setOrganizationName("OTELO");
    QCoreApplication::setApplicationName("Fingerprint Manager");
	FingerprintManager w;
	w.show();
	
	// translation
    QTranslator qtTranslator;
	if(!qtTranslator.load("/usr/share/qt4/translations/qt_de.qm", QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
	{
		qDebug() << "translation could not be loaded";
	}
    a.installTranslator(&qtTranslator);
	
	return a.exec();
}
