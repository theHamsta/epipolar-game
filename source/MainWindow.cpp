#include "MainWindow.hpp"

#include <GetSet/GetSet.hxx>
#include <GetSet/GetSetIO.h>
#include <GetSet/GetSetInternal.h>
#include <QDebug>
#include <QSettings>
#include <opencv2/opencv.hpp>

#include "glColors.hpp"
#include "ui_MainWindow.h"

using namespace std::string_literals;
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    delete ui->statusbar;

    readSettings();
    this->setAcceptDrops(true);
    GetSet<>("ini-File") = "epipolar-game.ini";
    auto callback        = [&](const GetSetInternal::Node& node) {
        const std::string& section(node.super_section);
        const std::string& key(node.name);

        qDebug() << "[" << QString::fromStdString(section) << "]:" << QString::fromStdString(key);
        if (key == "Line Thickness")
        {
            ui->leftImg->setLineThickness(GetSet< float >("Display/Line Thickness"));
            ui->rightImg->setLineThickness(GetSet< float >("Display/Line Thickness"));
        }
        else if (key == "Line Opacity")
        {
            ui->leftImg->setLineOpacity(GetSet< float >("Display/Line Opacity"));
            ui->rightImg->setLineOpacity(GetSet< float >("Display/Line Opacity"));
        }
        cv::Mat lines(1, 4, CV_32FC2);
        lines.at< float >(0, 0) = 0.f;
        lines.at< float >(0, 1) = 0.f;
        lines.at< float >(0, 2) = 50.f;
        lines.at< float >(0, 3) = 50.f;
        ui->leftImg->clearLinesToDraw();
        ui->leftImg->appendLinesToDraw(lines, OGL_BLACK);
    };
    m_getSetHandler = std::make_shared< GetSetHandler >(callback, GetSetInternal::Dictionary::global());
    GetSetGui::Slider("Display/Line Thickness").setMin(0.1).setMax(10);
    GetSetGui::Slider("Display/Line Opacity").setMin(0.0).setMax(1);

    GetSetIO::load< GetSetIO::IniFile >(GetSet<>("ini-File"));

    auto mat = cv::imread("/localhome/seitz_local/Desktop/mesoscale.png"s);
    ui->leftImg->setImage(mat);
    ui->rightImg->setImage(mat);
}

MainWindow::~MainWindow() { delete ui; }

auto MainWindow::readSettings() -> void
{
    QSettings settings("theHamsta", "epipolar-game");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

auto MainWindow::closeEvent(QCloseEvent* event) -> void
{
    QSettings settings("theHamsta", "epipolar-game");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    GetSet<>("ini-File") = "epipolar-game.ini";
    GetSetIO::save< GetSetIO::IniFile >(GetSet<>("ini-File"));
    QMainWindow::closeEvent(event);
}
