#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include <GetSet/GetSet.hxx>
#include <GetSet/GetSetIO.h>
#include <GetSet/GetSetInternal.h>
#include <QDebug>
#include <QSettings>
#include <opencv2/opencv.hpp>

using namespace std::string_literals;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  readSettings();
  this->setAcceptDrops(true);
  GetSet<>("ini-File") = "epipolar-game.ini";
  auto callback = [&](const GetSetInternal::Node &node) {
    const std::string &section(node.super_section);
    const std::string &key(node.name);

    qDebug() << "[" << QString::fromStdString(section)
             << "]:" << QString::fromStdString(key);

    GetSetIO::save<GetSetIO::IniFile>(GetSet<>("ini-File"));
  };
  m_getSetHandler = std::make_shared<GetSetHandler>(
      callback, GetSetInternal::Dictionary::global());
  GetSet<bool>("View/Show FPS") = true;
  GetSetGui::Slider("View/Slice X").setMin(0).setMax(500);
  GetSetGui::Slider("View/Slice Y").setMin(0).setMax(200);
  GetSetGui::Slider("View/Slice Z").setMin(0).setMax(200);
  GetSetGui::Slider("View/Transfer Min").setMin(0).setMax(10000);
  GetSetGui::Slider("View/Transfer Max").setMin(0).setMax(10000);
  GetSetGui::Slider("View/Volume Range Min").setMin(-1).setMax(1);
  GetSetGui::Slider("View/Volume Range Max").setMin(-1).setMax(1);
  GetSetGui::Slider("View/Volume Range Iso").setMin(-1).setMax(1);
  GetSetGui::Button("View/Set Volume Min-Max");
  GetSetGui::Slider("View/Ray Casting Step Size").setMin(0.1).setMax(2.) = 1;
  GetSetGui::Button("View/Show Entire Scene");
  GetSetGui::Enum("View/Rendering Mode")
      .setChoices("Voxel; Section2D; Section3D; EmptySkip; Trilinear; "
                  "Tricubic; LevelSet; Volume;")
      .setValue(6);
  GetSetGui::File("z_Input/Input File") = "";
  GetSetGui::Slider("z_Segmentation/Threshold").setMin(0).setMax(1);
  // GetSet<bool>("z_Segmentation/Use relative threshold") = true;
  GetSetIO::load<GetSetIO::IniFile>(GetSet<>("ini-File"));

  auto mat = cv::imread("/localhome/seitz_local/Desktop/mesoscale.png"s);
  ui->leftImg->setImage(mat);
  ui->rightImg->setImage(mat);
}

MainWindow::~MainWindow() { delete ui; }

auto MainWindow::readSettings() -> void {

  QSettings settings("theHamsta", "epipolar-game");
  settings.setValue("geometry", saveGeometry());
  settings.setValue("windowState", saveState());
}

auto MainWindow::closeEvent(QCloseEvent *event) -> void {
  QSettings settings("theHamsta", "epipolar-game");
  settings.setValue("geometry", saveGeometry());
  settings.setValue("windowState", saveState());
  QMainWindow::closeEvent(event);
}
