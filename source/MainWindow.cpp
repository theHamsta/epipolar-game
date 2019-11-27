#include "MainWindow.hpp"

#include <GetSet/GetSet.hxx>
#include <GetSet/GetSetIO.h>
#include <GetSet/GetSetInternal.h>
#include <QColor>
#include <QDebug>
#include <QSettings>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <qglobal.h>
#include <qnamespace.h>
#include <qpalette.h>

#include "CvPybindInterop.hpp"
#include "EpipolarCalculations.hpp"
#include "GameState.hpp"
#include "GetSet/GetSet_impl.hxx"
#include "ImportVolumes.hpp"
#include "ProjectiveGeometry.hxx"
#include "Scoring.hpp"
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
            if (m_volumes.size())
            {
                m_state.volumeNumber++;
                m_state.volumeNumber %= m_volumes.size();
                newForwardProjections();
            }
        }
        else if (key == "New Views" && m_volumes.size())
        {
            newForwardProjections();
        }
        else if (key == "New Pumpkin" && m_projections.size())
        {
            if (m_projections.size())
            {
                m_state.realProjectionsNumber++;
                m_state.realProjectionsNumber %= m_projections.size();
                newRealProjections();
            }
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
        else if (key == "Score P1")
        {
            qInfo() << "P1 scored";
            auto score = GetSet< int >("Game/Score P1");
            ui->scoreP1->setText(QString::number(score));
        }
        else if (key == "Score P2")
        {
            qInfo() << "P2 scored";
            auto score = GetSet< int >("Game/Score P2");
            ui->scoreP2->setText(QString::number(score));
        }
        else if (key == "Reset Scores")
        {
            GetSet< int >("Game/Score P1") = 0;
            GetSet< int >("Game/Score P2") = 0;
            m_state.inputState             = InputState::InputP1;
        }
        else if (key == "Projections Directory")
        {
            std::string path = GetSet< std::string >("Settings/Projections Directory");
            openProjectionsDirectory(QString::fromStdString(path));
        }

        if (key == "red" || key == "green" || key == "blue")
        {
            QPalette palette1 = ui->scoreP1->palette();
            palette1.setColor(ui->scoreP1->foregroundRole(), QColor(255 * GetSet< int >("Display/P1 Color/red"),
                                                                    255 * GetSet< int >("Display/P1 Color/green"),
                                                                    255 * GetSet< int >("Display/P1 Color/blue")));
            ui->scoreP1->setPalette(palette1);
            QPalette palette2 = ui->scoreP2->palette();
            palette2.setColor(ui->scoreP2->foregroundRole(), QColor(255 * GetSet< int >("Display/P2 Color/red"),
                                                                    255 * GetSet< int >("Display/P2 Color/green"),
                                                                    255 * GetSet< int >("Display/P2 Color/blue")));
            ui->scoreP2->setPalette(palette2);
            updateGameLogic();
        }
        else if (section == "Game" || section == "Game/P1" || section == "Game/P2")
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
    GetSetGui::Button("Game/Evaluate")                           = "Evaluate";
    GetSetGui::Button("Game/New Views")                          = "New Forward Projection";
    GetSetGui::Button("Game/New Volume")                         = "New Volume";
    GetSetGui::Button("Game/New Pumpkin")                        = "New Pumpkin";
    GetSetGui::Button("Game/New Real Projection")                = "New Real Projection";
    GetSetGui::Button("Game/Reset Scores")                       = "Reset Scores";
    GetSetGui::Directory("Settings/Volume Directory")            = "";
    GetSetGui::Directory("Settings/Projections Directory")       = "";
    GetSet< float >("Settings/Random Point Range")               = 100.;
    GetSet< float >("Settings/Detector Spacing")                 = 1.;
    GetSet< bool >("Settings/Siemens Flip for Real Projections") = true;

    GetSetGui::Slider("Display/P1 Color/red").setMin(0.).setMax(1.) = 1.;
    GetSetGui::Slider("Display/P1 Color/green").setMin(0.).setMax(1.);
    GetSetGui::Slider("Display/P1 Color/blue").setMin(0.).setMax(1.);

    GetSetGui::Slider("Display/P2 Color/red").setMin(0.).setMax(1.);
    GetSetGui::Slider("Display/P2 Color/green").setMin(0.).setMax(1.);
    GetSetGui::Slider("Display/P2 Color/blue").setMin(0.).setMax(1.) = 1.;

    GetSetGui::Slider("Display/Ground Truth Color/red").setMin(0.).setMax(1.);
    GetSetGui::Slider("Display/Ground Truth Color/green").setMin(0.).setMax(1.) = 1.;
    GetSetGui::Slider("Display/Ground Truth Color/blue").setMin(0.).setMax(1.);

    GetSetGui::Slider("Display/Line Thickness").setMin(0.1).setMax(10) = 3.;
    GetSetGui::Slider("Display/Line Opacity").setMin(0.1).setMax(1)    = 0.9;

    GetSetGui::Slider("Input/Angle Sensitivity").setMin(0.01).setMax(0.2) = 0.1;
    GetSetGui::Slider("Input/Offset Sensitivity").setMin(0.5).setMax(100) = 2.;

    GetSet<>("ini-File") = "epipolar-game.ini";
    GetSetIO::load< GetSetIO::IniFile >(GetSet<>("ini-File"));

    GetSet< int >("Game/Score P1") = 0;
    GetSet< int >("Game/Score P2") = 0;

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

    if (GetSet< bool >("Debug/Debug"))
    {
        GetSet< float >("Debug/Ground Truth Offset") = m_state.groundTruthLine.offset;
        GetSet< float >("Debug/Ground Truth Angle")  = m_state.groundTruthLine.angle;

        GetSet< float >("Debug/Compare Offset") = m_state.compareLine.offset;
        GetSet< float >("Debug/Compare Angle")  = m_state.compareLine.angle;

        GetSet< float >("Debug/P1 Offset") = m_state.lineP1.offset;
        GetSet< float >("Debug/P1 Angle")  = m_state.lineP1.angle;
        GetSet< float >("Debug/P2 Offset") = m_state.lineP2.offset;
        GetSet< float >("Debug/P2 Angle")  = m_state.lineP2.angle;
    }
}

auto MainWindow::keyPressEvent(QKeyEvent* event) -> void
{
    if (!ui->dockWidget->isVisible())
    {
        ui->dockWidget->setVisible(true);
    }
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
    m_volumes = importVolumes< float >(path.toStdString());

    qInfo() << "Loaded " << m_volumes.size() << " volumes";
    for (auto& v : m_volumes)
    {
        qInfo() << "Shape volume: " << v.shape()[0] << ", " << v.shape()[1] << ", " << v.shape()[2];
    }

    if (m_volumes.size())
    {
        cv::Mat mat = cvMatFromArray(m_volumes[0], 0);
        ui->leftImg->setImage(mat);
        ui->rightImg->setImage(mat);
    }
}

auto MainWindow::newForwardProjections() -> void
{
    qDebug() << "New Projections";
    if (m_volumes.size())
    {
        qDebug() << "Projecting";
        auto scale = GetSet< float >("Settings/Random Point Range");
        qDebug() << "Scale :" << scale;
        std::uniform_real_distribution<> dis(-scale, scale);

        auto [view1, matrix1, detectorSpacing] = makeProjection(m_volumes[m_state.volumeNumber]);
        m_view1                                = view1;
        cv::Mat m1                             = cvMatFromArray(m_view1);
        ui->leftImg->setImage(m1);

        auto [view2, matrix2, _detectorSpacing] = makeProjection(m_volumes[m_state.volumeNumber]);
        m_view2                                 = view2;
        cv::Mat m2                              = cvMatFromArray(m_view2);
        ui->rightImg->setImage(m2);

        auto randomPoint = Geometry::RP3Point{ dis(m_random), dis(m_random), dis(m_random), 1 };

        auto [compareLine, groundTruthLine] = getEpipolarLines(matrix1, matrix2, randomPoint, detectorSpacing);
        m_state.compareLine                 = compareLine;
        m_state.groundTruthLine             = groundTruthLine;
        m_state.realProjectionsMode         = false;
    }
    m_state.inputState = InputState::InputP1;
    updateGameLogic();
}

auto MainWindow::evaluate() -> void
{
    m_state.nextInputState();
    if (m_state.inputState == InputState::None)
    {
        auto distance1 =
            calcDistance(m_state.groundTruthLine, m_state.lineP1, ui->rightImg->img().cols, ui->rightImg->img().rows);
        auto distance2 =
            calcDistance(m_state.groundTruthLine, m_state.lineP2, ui->rightImg->img().cols, ui->rightImg->img().rows);

        qDebug() << "Scoring!";
        qDebug() << "P1: " << distance1;
        qDebug() << "P2: " << distance2;
        if (distance1 < distance2)
        {
            GetSet< int >("Game/Score P1") = GetSet< int >("Game/Score P1") + 1;
        }
        else
        {
            GetSet< int >("Game/Score P2") = GetSet< int >("Game/Score P2") + 1;
        }
    }
    updateGameLogic();
}

auto MainWindow::newRealProjections() -> void
{
    if (m_projectionMatrices.size() && m_projections.size())
    {
        assert(m_state.realProjectionsNumber < static_cast< int >(m_projectionMatrices.size()));
        assert(m_state.realProjectionsNumber < static_cast< int >(m_projections.size()));

        std::uniform_int_distribution<> dis_int(0, m_projectionMatrices[m_state.realProjectionsNumber].size() - 1);
        int random_idx1 = 0;
        int random_idx2 = 0;
        while (random_idx1 == random_idx2)
        {
            random_idx1 = dis_int(m_random);
            random_idx2 = dis_int(m_random);
        }
        auto view1 = m_projections[m_state.realProjectionsNumber][random_idx1];
        auto view2 = m_projections[m_state.realProjectionsNumber][random_idx2];

        cv::Mat m1 = cvMatFromArray(view1);
        ui->leftImg->setImage(m1);
        cv::Mat m2 = cvMatFromArray(view2);
        ui->rightImg->setImage(m2);

        const Geometry::ProjectionMatrix& p1 = m_projectionMatrices[m_state.realProjectionsNumber][random_idx1];
        const Geometry::ProjectionMatrix& p2 = m_projectionMatrices[m_state.realProjectionsNumber][random_idx2];

        auto scale = GetSet< float >("Settings/Random Point Range");
        std::uniform_real_distribution<> dis(-scale, scale);

        auto randomPoint = Geometry::RP3Point{ dis(m_random), dis(m_random), dis(m_random), 1 };

        double detectorSpacing = GetSet< float >("Settings/Detector Spacing");

        auto [compareLine, groundTruthLine] = getEpipolarLines(p1, p2, randomPoint, detectorSpacing);

        if (GetSet< bool >("Settings/Siemens Flip for Real Projections"))
        {
            m_state.compareLine.angle     = -m_state.compareLine.angle;
            m_state.groundTruthLine.angle = -m_state.groundTruthLine.angle;
        }

        m_state.compareLine         = compareLine;
        m_state.groundTruthLine     = groundTruthLine;
        m_state.inputState          = InputState::InputP1;
        m_state.realProjectionsMode = true;

        updateGameLogic();
    }
}

auto MainWindow::openProjectionsDirectory(const QString& path) -> void
{
    auto pathStd                 = path.toStdString();
    auto [projections, matrices] = importProjections< float >(pathStd);

    qInfo() << "Loaded " << projections.size() << " projection data sets";
    for (auto& p : projections)
    {
        qInfo() << "Loaded data set with" << p.size() << " projections";
        if (p.size() == 0)
        {
            m_projections.clear();
            m_projectionMatrices.clear();
        }
    }
    m_projections        = projections;
    m_projectionMatrices = matrices;

    m_state.realProjectionsNumber = 0;
}
