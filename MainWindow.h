#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include <QSlider>
#include <QStackedWidget>
#include <QStackedLayout>
#include <QFileDialog>
#include "ImageDisplay.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void createMenu();
    void createDisplayGroup();
    void createFilterGroup();
    void createSettingsGroup();
    void createImageConnections();

    QWidget         *ui;
    QGridLayout     *mainlayout;
    QMenuBar        *menuBar;
    QGroupBox       *gbDisplay;
    QGroupBox       *gbFilter;
    QGroupBox       *gbSettings;
    ImageDisplay    *image;
    QPushButton     *pbPixFilter;
    QPushButton     *pbBlurFilter;
    QPushButton     *pbGrayFilter;
    QPushButton     *pbBinFilter;
    QPushButton     *pbCelShade;
    QPushButton     *pbReload;
    QLabel          *lIsovalue;
    QLabel          *lStepSize;
    QLabel          *lQueued;
    QRadioButton    *rbBinary;
    QRadioButton    *rbGrayscale;
    QSlider         *sIsovalue;
    QSlider         *sStepsize;

    QStackedWidget  *slImage;
    QStackedWidget  *swShowImage;
    QPushButton     *pbShowBinary;
    QPushButton     *pbShowOriginal;

    QMenu           *fileMenu;
    QAction         *exitAction;
    QAction         *openAction;
    QString         currentFileName;

private slots:
    void setLayoutHeight();
    void setIsoValue(){if(image){image->setIsovalue(sIsovalue->value());}}
    void setStepSize(){if(image){image->setStepSize(sStepsize->value());}}
    void updateProcessLabel(int);
public slots:
    void updateIsoValue(int);
    void updateStepValue(int);
    void increaseIsoPressed();
    void decreaseIsoPressed();
    void increaseStepPressed();
    void decreaseStepPressed();
    void toggleBinary();
    void openFile();
};

#endif // MAINWINDOW_H
