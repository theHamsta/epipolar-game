#include "MainWindow.hpp"

#include <GetSet/GetSet.hxx>
#include <GetSet/GetSetIO.h>
#include <GetSet/GetSetInternal.h>
#include <QDebug>
#include <QSettings>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <qpalette.h>

#include "glColors.hpp"
#include "ui_MainWindow.h"

using namespace std::string_literals;
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    delete ui->statusbar;

    this->setAcceptDrops(true);
    readSettings();

    auto callback = [&](const GetSetInternal::Node& node) {
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
        else if (key == "red" || key == "green" || key == "blue")
        {
            updateGameLogic();
        }
        else if (section == "Game")
        {
            updateGameLogic();
        }
    };
    m_getSetHandler = std::make_shared< GetSetHandler >(callback, GetSetInternal::Dictionary::global());

    GetSetGui::Slider("Game/P1 Line Angle").setMin(0.0).setMax(2. * M_PI);
    GetSetGui::Slider("Game/P1 Line Offset").setMin(-2000.).setMax(2000.);

    GetSetGui::Slider("Game/P2 Line Angle").setMin(0.0).setMax(2. * M_PI);
    GetSetGui::Slider("Game/P2 Line Offset").setMin(-2000.).setMax(2000.);

    GetSetGui::Slider("Display/P1 Color/red").setMin(0.).setMax(1.) = 1.;
    GetSetGui::Slider("Display/P1 Color/green").setMin(0.).setMax(1.);
    GetSetGui::Slider("Display/P1 Color/blue").setMin(0.).setMax(1.);

    GetSetGui::Slider("Display/P2 Color/red").setMin(0.).setMax(1.);
    GetSetGui::Slider("Display/P2 Color/green").setMin(0.).setMax(1.) = 1.;
    GetSetGui::Slider("Display/P2 Color/blue").setMin(0.).setMax(1.);

    GetSetGui::Slider("Display/Line Thickness").setMin(0.1).setMax(10) = 3.;
    GetSetGui::Slider("Display/Line Opacity").setMin(0.1).setMax(1) = 0.9;

    GetSet<>("ini-File") = "epipolar-game.ini";
    GetSetIO::load< GetSetIO::IniFile >(GetSet<>("ini-File"));

    auto mat = cv::imread("/home/stephan/Pictures/natgeo.jpg"s);

    ui->leftImg->setImage(mat);
    ui->rightImg->setImage(mat);

    auto color = this->palette().color(QPalette::Background);
    ui->leftImg->setBackgroundColor(color.redF(), color.greenF(), color.blueF());
    ui->rightImg->setBackgroundColor(color.redF(), color.greenF(), color.blueF());

    updateGameLogic();
}

MainWindow::~MainWindow() { delete ui; }

auto MainWindow::readSettings() -> void
{
    QSettings settings("theHamsta", "epipolar-game");
    restoreGeometry(settings.value("geometry", QByteArray()).toByteArray());
    restoreState(settings.value("windowState", QByteArray()).toByteArray());
}

auto MainWindow::closeEvent(QCloseEvent* event) -> void
{
    GetSet<>("ini-File") = "epipolar-game.ini";
    GetSetIO::save< GetSetIO::IniFile >(GetSet<>("ini-File"));

    QSettings settings("theHamsta", "epipolar-game");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    QMainWindow::closeEvent(event);
}

auto MainWindow::updateGameLogic() -> void
{
    m_state.lineP1 = { GetSet< float >("Game/P1 Line Offset"), GetSet< float >("Game/P1 Line Angle") };
    m_state.lineP2 = { GetSet< float >("Game/P2 Line Offset"), GetSet< float >("Game/P2 Line Angle") };

    auto pointsP1 = m_state.lineP1.toPointsOnLine(ui->rightImg->img().cols, ui->rightImg->img().rows);
    auto pointsP2 = m_state.lineP2.toPointsOnLine(ui->rightImg->img().cols, ui->rightImg->img().rows);

    cv::Mat p1_line(1, 4, CV_32FC2);
    p1_line.at< float >(0, 0) = pointsP1.a.x;
    p1_line.at< float >(0, 1) = pointsP1.a.y;
    p1_line.at< float >(0, 2) = pointsP1.b.x;
    p1_line.at< float >(0, 3) = pointsP1.b.y;

    cv::Mat p2_line(1, 4, CV_32FC2);
    p2_line.at< float >(0, 0) = pointsP2.a.x;
    p2_line.at< float >(0, 1) = pointsP2.a.y;
    p2_line.at< float >(0, 2) = pointsP2.b.x;
    p2_line.at< float >(0, 3) = pointsP2.b.y;

    ui->rightImg->clearLinesToDraw();
    // ui->rightImg->appendLinesToDraw(p1_line, OGL_RED);
    // ui->rightImg->appendLinesToDraw(p2_line, OGL_BLUE);
    ui->rightImg->appendLinesToDraw(p1_line, GetSet< float >("Display/P1 Color/red"),
                                    GetSet< float >("Display/P1 Color/green"),
                                    GetSet< float >("Display/P1 Color/blue"));
    ui->rightImg->appendLinesToDraw(p2_line, GetSet< float >("Display/P2 Color/red"),
                                    GetSet< float >("Display/P2 Color/green"),
                                    GetSet< float >("Display/P2 Color/blue"));
}
