#include "MainWindow.h"
#include <QDebug>
#include <QSpacerItem>
#include <string>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      image{nullptr}
{
    ui = new QWidget;
    setCentralWidget(ui);
    createMenu();
    createDisplayGroup();
    createFilterGroup();
    createSettingsGroup();

    mainlayout = new QHBoxLayout;
    mainlayout->setMenuBar(menuBar);
    mainlayout->addWidget(gbDisplay);
    mainlayout->addWidget(gbSettings);
    mainlayout->addWidget(gbFilter);

    ui->setLayout(mainlayout);
    setWindowTitle(tr("Pixelater Qt2000"));
}

MainWindow::~MainWindow()
{

}

void MainWindow::createMenu(){
    menuBar = new QMenuBar;
    fileMenu = new QMenu(tr("&File"), this);
    openAction = fileMenu->addAction(tr("&Open"));
    exitAction = fileMenu->addAction(tr("E&xit"));
    menuBar->addMenu(fileMenu);

    connect(openAction, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(quit()));
}

void MainWindow::createDisplayGroup(){
    gbDisplay = new QGroupBox(tr("Image"));
    QVBoxLayout *layout = new QVBoxLayout;

    // Image area
    slImage = new QStackedWidget;
    slImage->addWidget(new QWidget);

    layout->addWidget(slImage);
    gbDisplay->setLayout(layout);
}

void MainWindow::createFilterGroup(){
    gbFilter = new QGroupBox(tr("Apply Filter"));
    QVBoxLayout *layout = new QVBoxLayout;

    // Add buttons
    pbPixFilter  = new QPushButton(tr("Pixelate"));
    pbBlurFilter = new QPushButton(tr("Blur"));
    pbGrayFilter = new QPushButton(tr("Grayscale"));
    pbBinFilter  = new QPushButton(tr("Binary Gray"));
    pbCelShade   = new QPushButton(tr("Cel Shade"));
    layout->addWidget(pbPixFilter);
    layout->addWidget(pbBlurFilter);
    layout->addWidget(pbGrayFilter);
    layout->addWidget(pbBinFilter);
    layout->addWidget(pbCelShade);

    layout->setSizeConstraint(QLayout::SetFixedSize);
    gbFilter->setLayout(layout);

    // Connect slots
}

void MainWindow::createSettingsGroup(){
    gbSettings = new QGroupBox(tr("Contour Settings"));

    QVBoxLayout *layout = new QVBoxLayout;

    QPushButton *incIsoArrow;
    QPushButton *decIsoArrow;
    QPushButton *incStepArrow;
    QPushButton *decStepArrow;
    // Add sliders
    QGroupBox *gbSliders = new QGroupBox(tr("Settings"));
    QGridLayout *glSliders = new QGridLayout;
    lIsovalue = new QLabel(tr("Isovalue"));
    sIsovalue = new QSlider(Qt::Horizontal);
    sIsovalue->setRange(0,255);
    sIsovalue->setValue(ISOVALUE);
    glSliders->addWidget(lIsovalue,0,0);
    decIsoArrow = new QPushButton(tr("<<"));
    glSliders->addWidget(decIsoArrow,0,1);
    //glSliders->addWidget(lIsovalueDisplayValue,0,1);
    glSliders->addWidget(sIsovalue,0,2);
    incIsoArrow = new QPushButton(tr(">>"));
    glSliders->addWidget(incIsoArrow,0,3);

    lStepSize = new QLabel;
    sStepsize = new QSlider(Qt::Horizontal);
    //lStepSizeDisplayValue = new QLabel(QString(STEPSIZE));
    sStepsize->setRange(1,40);
    sStepsize->setValue(STEPSIZE);
    glSliders->addWidget(lStepSize,1,0);
    decStepArrow = new QPushButton(tr("<<"));
    glSliders->addWidget(decStepArrow, 1,1);
    //glSliders->addWidget(lStepSizeDisplayValue,1,1);
    glSliders->addWidget(sStepsize,1,2);
    incStepArrow = new QPushButton(tr(">>"));
    glSliders->addWidget(incStepArrow,1,3);
    gbSliders->setLayout(glSliders);

    connect(sIsovalue, &QSlider::valueChanged, this, &MainWindow::updateIsoValue);
    connect(sStepsize, &QSlider::valueChanged, this, &MainWindow::updateStepValue);
    connect(incIsoArrow, &QPushButton::pressed, this, &MainWindow::increaseIsoPressed);
    connect(decIsoArrow, &QPushButton::pressed, this, &MainWindow::decreaseIsoPressed);
    connect(incStepArrow, &QPushButton::pressed, this, &MainWindow::increaseStepPressed);
    connect(decStepArrow, &QPushButton::pressed, this, &MainWindow::decreaseStepPressed);
    updateIsoValue(ISOVALUE);
    updateStepValue(STEPSIZE);

    // Add radio buttons
    QGroupBox *gbContour = new QGroupBox(tr("Contour Back"));
    QVBoxLayout *rbLayout = new QVBoxLayout;
    rbBinary    = new QRadioButton(tr("Binary"));
    rbGrayscale = new QRadioButton(tr("Grayscale"));
    rbLayout->addWidget(rbBinary);
    rbLayout->addWidget(rbGrayscale);
    gbContour->setLayout(rbLayout);

    pbShowBinary = new QPushButton(tr("Show Binary"));
    pbShowOriginal = new QPushButton(tr("Show Original"));

    swShowImage = new QStackedWidget;
    swShowImage->addWidget(pbShowBinary);
    swShowImage->addWidget(pbShowOriginal);
    swShowImage->setCurrentIndex(0);

    connect(pbShowBinary, &QPushButton::pressed, this, &MainWindow::toggleBinary);
    connect(pbShowOriginal, &QPushButton::pressed, this, &MainWindow::toggleBinary);

    // Add to layout
    layout->addWidget(gbSliders);
    layout->addWidget(gbContour);
    layout->addWidget(swShowImage);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    gbSettings->setLayout(layout);

}

void MainWindow::openFile(){
    QString fileName = QFileDialog::getOpenFileName(this,
                                            tr("Open Image"),
                                            "/home/dpost/Development/2019-01-Winter/Final/Pixelater2/",
                                            tr("Image Files (*.jpg, *.bmp)") );
    if(image){
        slImage->setCurrentIndex(0);
        slImage->removeWidget(image);
        delete image;
    }
    image = new ImageDisplay(fileName);
    slImage->setCurrentIndex((slImage->addWidget(image)));
    createImageConnections();
    // Close old image
    // Open new image

}

void MainWindow::updateIsoValue(int value){
    lIsovalue->setText(tr("Isovalue( %1 )").arg(value));
}

void MainWindow::updateStepValue(int value){
    lStepSize->setText(tr("Stepsize( %1 )").arg(value));
}

void MainWindow::createImageConnections(){
    connect(pbBinFilter, &QPushButton::pressed, image, &ImageDisplay::BinaryGray );
    connect(pbPixFilter, &QPushButton::pressed, image, &ImageDisplay::Pixelate );
    connect(pbBlurFilter, &QPushButton::pressed, image, &ImageDisplay::Blur );
    connect(pbCelShade, &QPushButton::pressed, image, &ImageDisplay::CelShade );

    connect(sIsovalue, &QSlider::valueChanged, image, &ImageDisplay::setIsovalue);
    connect(sStepsize, &QSlider::valueChanged, image, &ImageDisplay::setStepSize);
    connect(pbShowBinary, &QPushButton::pressed, image, &ImageDisplay::toggleBinary);
    connect(pbShowOriginal, &QPushButton::pressed, image, &ImageDisplay::toggleBinary);
}

void MainWindow::increaseIsoPressed(){
    sIsovalue->setValue(sIsovalue->value()+1);
}
void MainWindow::decreaseIsoPressed(){
    sIsovalue->setValue(sIsovalue->value()-1);
}
void MainWindow::increaseStepPressed(){
    sStepsize->setValue(sStepsize->value()+1);
}
void MainWindow::decreaseStepPressed(){
    sStepsize->setValue(sStepsize->value()-1);
}
void MainWindow::toggleBinary(){
    swShowImage->setCurrentIndex(!swShowImage->currentIndex());
}
