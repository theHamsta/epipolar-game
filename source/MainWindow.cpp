#include "MainWindow.hpp"

#include <GetSet/GetSet.hxx>
#include <GetSet/GetSetIO.h>
#include <GetSet/GetSetInternal.h>
#include <QDebug>
#include <QSettings>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <qnamespace.h>
#include <qpalette.h>

#include "CvPybindInterop.hpp"
#include "EpipolarCalculations.hpp"
#include "GameState.hpp"
#include "ImportVolumes.hpp"
#include "ProjectiveGeometry.hxx"
#include "glColors.hpp"
#include "projection_kernel.hpp"
#include "ui_MainWindow.h"

using namespace std::string_literals;
using namespace pybind11::literals;

static constexpr double TWO_PI = 2. * M_PI;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow), m_random(std::random_device()())
{
    ui->setupUi(this);
    delete ui->statusbar;

    this->setAcceptDrops(true);
    readSettings();

    auto callback = [&](const GetSetInternal::Node& node) {
        const std::string& section(node.super_section);
        const std::string& key(node.name);

        qDebug() << "[" << QString::fromStdString(section) << "]:" << QString::fromStdString(key);
        if (key == "Volume Directory")
        {
            openDirectory(QString::fromStdString(GetSet< std::string >("Settings/Volume Directory")));
        }
        else if (key == "New Volume" && m_volumes.size())
        {
            m_state.volumeNumber %= m_volumes.size();
            newForwardProjections();
        }
        else if (key == "New Views" && m_volumes.size())
        {
            newForwardProjections();
        }
        else if (key == "New Real Projection" && m_volumes.size())
        {
            newRealProjections();
        }
        else if (key == "Evaluate")
        {
            evaluate();
        }
        else if (key == "Line Thickness")
        {
            ui->leftImg->setLineThickness(GetSet< float >("Display/Line Thickness"));
            ui->rightImg->setLineThickness(GetSet< float >("Display/Line Thickness"));
        }
        else if (key == "Line Opacity")
        {
            ui->leftImg->setLineOpacity(GetSet< float >("Display/Line Opacity"));
            ui->rightImg->setLineOpacity(GetSet< float >("Display/Line Opacity"));
        }
        else if (key == "red" || key == "green" || key == "blue" || section == "Game" || section == "Game/P1" ||
                 section == "Game/P2")
        {
            updateGameLogic();
        }
    };
    m_getSetHandler = std::make_shared< GetSetHandler >(callback, GetSetInternal::Dictionary::global());
    GetSetGui::Slider("Game/P1/Line Angle").setMin(0.0).setMax(2. * M_PI);
    GetSetGui::Slider("Game/P1/Line Offset").setMin(-2000.).setMax(2000.);
    GetSetGui::Section("Game/P1").setGrouped(true);

    GetSetGui::Slider("Game/P2/Line Angle").setMin(0.0).setMax(2. * M_PI);
    GetSetGui::Slider("Game/P2/Line Offset").setMin(-2000.).setMax(2000.);
    GetSetGui::Section("Game/P2").setGrouped(true);
    GetSetGui::Button("Game/Evaluate")                                       = "Evaluate";
    GetSetGui::Button("Game/New Views")                                      = "New Forward Projection";
    GetSetGui::Button("Game/New Volume")                                     = "New Volume";
    GetSetGui::Button("Game/New Real Projection")                            = "New Real Projection";
    GetSetGui::Directory("Settings/Volume Directory")                        = "";
    GetSetGui::Slider("Settings/Random Point Range").setMin(0.).setMax(300.) = 100;

    GetSetGui::Slider("Display/P1 Color/red").setMin(0.).setMax(1.) = 1.;
    GetSetGui::Slider("Display/P1 Color/green").setMin(0.).setMax(1.);
    GetSetGui::Slider("Display/P1 Color/blue").setMin(0.).setMax(1.);

    GetSetGui::Slider("Display/P2 Color/red").setMin(0.).setMax(1.);
    GetSetGui::Slider("Display/P2 Color/green").setMin(0.).setMax(1.) = 1.;
    GetSetGui::Slider("Display/P2 Color/blue").setMin(0.).setMax(1.);

    GetSetGui::Slider("Display/Line Thickness").setMin(0.1).setMax(10) = 3.;
    GetSetGui::Slider("Display/Line Opacity").setMin(0.1).setMax(1)    = 0.9;

    GetSetGui::Slider("Input/Angle Sensitivity").setMin(0.01).setMax(0.2) = 0.1;
    GetSetGui::Slider("Input/Offset Sensitivity").setMin(0.5).setMax(100) = 20;

    GetSet<>("ini-File") = "epipolar-game.ini";
    GetSetIO::load< GetSetIO::IniFile >(GetSet<>("ini-File"));

    auto mat =
        cv::imread("/localhome/seitz_local/Pictures/Bodenfliesen-Bodenfliese-Canaletto-In-Wood-X125281X8_600x600.jpg"s);

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
    if (inputP1())
    {
        m_state.lineP1 = { GetSet< float >("Game/P1/Line Offset"), GetSet< float >("Game/P1/Line Angle") };
    }
    if (inputP2())
    {
        m_state.lineP2 = { GetSet< float >("Game/P2/Line Offset"), GetSet< float >("Game/P2/Line Angle") };
    }

    GetSetGui::Section("Game/P1").setHidden(!inputP1());
    GetSetGui::Section("Game/P2").setHidden(!inputP2());

    auto pointsP1          = m_state.lineP1.toPointsOnLine(ui->rightImg->img().cols, ui->rightImg->img().rows);
    auto pointsP2          = m_state.lineP2.toPointsOnLine(ui->rightImg->img().cols, ui->rightImg->img().rows);
    auto comparePoints     = m_state.compareLine.toPointsOnLine(ui->leftImg->img().cols, ui->leftImg->img().rows);
    auto groundTruthPoints = m_state.groundTruthLine.toPointsOnLine(ui->rightImg->img().cols, ui->rightImg->img().rows);

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
    if (inputP1() || (m_state.inputState == InputState::None))
    {
        ui->rightImg->appendLinesToDraw(p1_line, GetSet< float >("Display/P1 Color/red"),
                                        GetSet< float >("Display/P1 Color/green"),
                                        GetSet< float >("Display/P1 Color/blue"));
    }
    if (inputP2() || (m_state.inputState == InputState::None))
    {
        ui->rightImg->appendLinesToDraw(p2_line, GetSet< float >("Display/P2 Color/red"),
                                        GetSet< float >("Display/P2 Color/green"),
                                        GetSet< float >("Display/P2 Color/blue"));
    }

    cv::Mat compareLine(1, 4, CV_32FC2);
    compareLine.at< float >(0, 0) = comparePoints.a.x;
    compareLine.at< float >(0, 1) = comparePoints.a.y;
    compareLine.at< float >(0, 2) = comparePoints.b.x;
    compareLine.at< float >(0, 3) = comparePoints.b.y;
    ui->leftImg->clearLinesToDraw();
    ui->leftImg->appendLinesToDraw(compareLine, GetSet< float >("Display/Ground Truth Color/red"),
                                   GetSet< float >("Display/Ground Truth Color/green"),
                                   GetSet< float >("Display/Ground Truth Color/blue"));

    cv::Mat groundTruthLine(1, 4, CV_32FC2);
    groundTruthLine.at< float >(0, 0) = groundTruthPoints.a.x;
    groundTruthLine.at< float >(0, 1) = groundTruthPoints.a.y;
    groundTruthLine.at< float >(0, 2) = groundTruthPoints.b.x;
    groundTruthLine.at< float >(0, 3) = groundTruthPoints.b.y;
    if (m_state.inputState == InputState::None)
    {
        ui->rightImg->appendLinesToDraw(groundTruthLine, GetSet< float >("Display/Ground Truth Color/red"),
                                        GetSet< float >("Display/Ground Truth Color/green"),
                                        GetSet< float >("Display/Ground Truth Color/blue"));
    }
}

auto MainWindow::keyPressEvent(QKeyEvent* event) -> void
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
    {
        evaluate();
        return;
    }

    if (inputP1())
    {
        switch (event->key())
        {
            // P1
        case Qt::Key_Right:
            GetSet< float >("Game/P1/Line Angle") =
                GetSet< float >("Game/P1/Line Angle") - GetSet< float >("Input/Angle Sensitivity");
            break;
        case Qt::Key_Left:
            GetSet< float >("Game/P1/Line Angle") =
                GetSet< float >("Game/P1/Line Angle") + GetSet< float >("Input/Angle Sensitivity");
            break;

        case Qt::Key_Up:
            GetSet< float >("Game/P1/Line Offset") =
                GetSet< float >("Game/P1/Line Offset") - GetSet< float >("Input/Offset Sensitivity");
            break;
        case Qt::Key_Down:
            GetSet< float >("Game/P1/Line Offset") =
                GetSet< float >("Game/P1/Line Offset") + GetSet< float >("Input/Offset Sensitivity");
            break;
        default:
            qDebug() << "Key was pressed:" << event->key();
        }
    }

    if (inputP2())
    {
        switch (event->key())
        {
            // P2
        case Qt::Key_D:
            GetSet< float >("Game/P2/Line Angle") =
                GetSet< float >("Game/P2/Line Angle") - GetSet< float >("Input/Angle Sensitivity");
            break;
        case Qt::Key_A:
            GetSet< float >("Game/P2/Line Angle") =
                GetSet< float >("Game/P2/Line Angle") + GetSet< float >("Input/Angle Sensitivity");
            break;
        case Qt::Key_W:
            GetSet< float >("Game/P2/Line Offset") =
                GetSet< float >("Game/P2/Line Offset") - GetSet< float >("Input/Offset Sensitivity");
            break;
        case Qt::Key_S:
            GetSet< float >("Game/P2/Line Offset") =
                GetSet< float >("Game/P2/Line Offset") + GetSet< float >("Input/Offset Sensitivity");
            break;

        default:
            qDebug() << "Key was pressed:" << event->key();
        }
    }

    if (m_state.inputState == InputState::InputP2)
    {
        switch (event->key())
        {
            // P1
        case Qt::Key_Right:
            GetSet< float >("Game/P2/Line Angle") =
                GetSet< float >("Game/P2/Line Angle") - GetSet< float >("Input/Angle Sensitivity");
            break;
        case Qt::Key_Left:
            GetSet< float >("Game/P2/Line Angle") =
                GetSet< float >("Game/P2/Line Angle") + GetSet< float >("Input/Angle Sensitivity");
            break;

        case Qt::Key_Up:
            GetSet< float >("Game/P2/Line Offset") =
                GetSet< float >("Game/P2/Line Offset") - GetSet< float >("Input/Offset Sensitivity");
            break;
        case Qt::Key_Down:
            GetSet< float >("Game/P2/Line Offset") =
                GetSet< float >("Game/P2/Line Offset") + GetSet< float >("Input/Offset Sensitivity");
            break;
        default:
            qDebug() << "Key was pressed:" << event->key();
        }
    }
    event->accept();
}

auto MainWindow::openDirectory(const QString& path) -> void
{
    m_volumes   = importVolumes< float >(path.toStdString());
    cv::Mat mat = cvMatFromArray(m_volumes[0], 0);
    ui->leftImg->setImage(mat);
    ui->rightImg->setImage(mat);
}

auto MainWindow::newForwardProjections() -> void
{
    qDebug() << "New Projections";
    if (m_volumes.size() > 0)
    {
        qDebug() << "Projecting";
        std::uniform_real_distribution<> dis(0., TWO_PI);
        double v1_r1 = dis(m_random);
        double v1_r2 = dis(m_random);
        double v1_r3 = dis(m_random);

        m_view1 = pybind11::array_t< float >({ 960, 1024 });

        // float projectionMatrix[]{ -289.0098977737411,  -1205.2274801832275,  0.0,    186000.0,
        //-239.9634468375339,  -4.188577544948043,   1200.0, 144000.0,
        //-0.9998476951563913, -0.01745240643728351, 0.0,    600.0 };
        float projectionMatrix[]{ -289.0098977737411,  -1205.2274801832275,  0.0,    186000.0,
                                  -239.9634468375339,  -4.188577544948043,   1200.0, 144000.0,
                                  -0.9998476951563913, -0.01745240643728351, 0.0,    600.0 };

        // call_projection_kernel(projectionMatrix[0], projectionMatrix[1], projectionMatrix[2], projectionMatrix[3],
        // projectionMatrix[4], projectionMatrix[5], projectionMatrix[6], projectionMatrix[7],
        // projectionMatrix[8], projectionMatrix[9], projectionMatrix[10], projectionMatrix[11], 1,
        // m_view1, m_volumes[0], 3.);
        // makeProjection(m_volumes[0], m_view1, v1_r1, v1_r2, v1_r3, 0.3, 1.);
        pybind11::exec("array/=array.max();print(array)", pybind11::globals(), pybind11::dict("array"_a = m_view1));
        cv::Mat m1 = cvMatFromArray(m_view1);
        ui->leftImg->setImage(m1);

        double v2_r1 = dis(m_random);
        double v2_r2 = dis(m_random);
        double v2_r3 = dis(m_random);
        m_view2      = pybind11::array_t< float >({ 960, 1024 });

        // makeProjection(m_volumes[0], m_view2, v2_r1, v2_r2, v2_r3, 0.3, 1.);
        // projection_kernel(m_view2, v2_r1, v2_r2, v2_r3, m_volumes[0]);
        // call_projection_kernel(projectionMatrix[0], projectionMatrix[1], projectionMatrix[2], projectionMatrix[3],
        // projectionMatrix[4], projectionMatrix[5], projectionMatrix[6], projectionMatrix[7],
        // projectionMatrix[8], projectionMatrix[9], projectionMatrix[10], projectionMatrix[11],
        // 1, m_view2, m_volumes[0], 3.);
        pybind11::exec("array/=array.max()", pybind11::globals(), pybind11::dict("array"_a = m_view2));
        cv::Mat m2 = cvMatFromArray(m_view2);
        ui->rightImg->setImage(m2);
    }
    m_state.inputState = InputState::InputP1;
    updateGameLogic();
}

auto MainWindow::evaluate() -> void
{
    m_state.nextInputState();
    updateGameLogic();
}

auto MainWindow::newRealProjections() -> void
{
    Geometry::ProjectionMatrix p1{};
    Geometry::ProjectionMatrix p2{};
    Geometry::RP2Point rn{};

    auto scale = GetSet< float >("Settings/Random Point Range");
    std::uniform_real_distribution<> dis(-scale, scale);

    auto randomPoint = Geometry::RP3Point{ dis(m_random), dis(m_random), dis(m_random), 1 };

    double detectorSpacing = 1.;

    auto [compareLine, groundTruthLine] = getEpipolarLines(p1, p2, randomPoint, detectorSpacing);
    m_state.compareLine                 = compareLine;
    m_state.groundTruthLine             = groundTruthLine;
    m_state.inputState = InputState::InputP1;

    updateGameLogic();
}
