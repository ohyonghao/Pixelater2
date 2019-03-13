#ifndef IMAGEDISPLAY_H
#define IMAGEDISPLAY_H

#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QPixmap>
#include <QImage>
#include <QLabel>
#include <QByteArray>
#include "ImageProcessor.h"
#include "bitmap.h"

class ImageDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit ImageDisplay(QString filename, int isovalue=57, int stepsize = 5, bool useBinaryInter = true, QWidget *parent = nullptr);
    QSizePolicy sizePolicy(){return imageLabel->sizePolicy();}
    QSize size(){return imageLabel->size();}
private:
    void createScene();
    QPixmap  pixmap;

    bool displayBinary = false;
    QLabel *imageLabel;

    ImageProcessor processor;

signals:
    void imageLoaded();
    void processQueued(int);

private slots:
    void loadImage(const QByteArray &image);

public slots:
    void BinaryGray(){processor.BinaryGray();}
    void Pixelate(){processor.Pixelate();}
    void Blur(){processor.Blur();}
    void Contour(){processor.Contour();}
    void CelShade(){processor.CelShade();}
    void setIsovalue(int isovalue){ processor.setIsovalue(isovalue);}
    void setStepSize(int stepsize){ processor.setStepSize(stepsize);}
    void setBinaryInter(bool usebininter){processor.setBinaryInter(usebininter);}
    void toggleBinary();
    void LoadImage(){processor.LoadImage();}
    void save();
};

#endif // IMAGEDISPLAY_H
