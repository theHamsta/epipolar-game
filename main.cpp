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
#include "python_include.hpp"

int main(int argc, char* argv[])
{
    //     Q_INIT_RESOURCE(resources);
#ifndef NDEBUG
    // see http://doc.qt.io/qt-5/qtglobal.html#qSetMessagePattern for format
    qSetMessagePattern("[%{type}] (%{time}, thread: %{threadid}) %{message} (%{file}:%{line})");
#endif

    // Python interpreter to load volumes / generate projections
    // Will be alive during whole program execution
    pybind11::scoped_interpreter interpreter{};
    using namespace pybind11::literals;

    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("LME");
    QCoreApplication::setApplicationName("Epipolar Guessing Game");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("dirname", QApplication::translate("main.cpp", "Folder to load volumes from"));
    parser.process(app);

    //
    auto appDir = QCoreApplication::applicationDirPath();
    pybind11::exec("import sys;sys.path.insert(0, app_dir);", pybind11::globals(),
                   pybind11::dict("app_dir"_a = appDir.toStdString()));

    MainWindow mainWin;
    mainWin.show();

    return app.exec();
}
