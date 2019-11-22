#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QException>
#include <QFileInfo>
#include <QMessageBox>
#include <QtGlobal>
#include <qt5/QtWidgets/qmainwindow.h>

#include "MainWindow.hpp"

int main(int argc, char* argv[])
{
    //     Q_INIT_RESOURCE(resources);
#ifndef NDEBUG
    // see http://doc.qt.io/qt-5/qtglobal.html#qSetMessagePattern for format
    qSetMessagePattern("[%{type}] (%{time}, thread: %{threadid}) %{message} (%{file}:%{line})");
#endif

    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("LME");
    QCoreApplication::setApplicationName("Epipolar Guessing Game");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("filename",
                                 QApplication::translate("ImageComparerMain.cpp", "First image file to open"));
    parser.process(app);

    MainWindow mainWin;
    mainWin.show();

    return app.exec();
}
