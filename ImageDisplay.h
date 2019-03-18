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
    void BinaryGray(){processor.QueueProcess(ImageProcessor::BINARY_GRAY);}
    void Pixelate(){processor.QueueProcess(ImageProcessor::PIXELATE);}
    void Blur(){processor.QueueProcess(ImageProcessor::BLUR);}
    void Contour(){processor.QueueProcess(ImageProcessor::CONTOUR);}
    void CelShade(){processor.QueueProcess(ImageProcessor::CELSHADE);}
    void ScaleDown(){processor.QueueProcess(ImageProcessor::SCALE_DOWN);}
    void ScaleUp(){processor.QueueProcess(ImageProcessor::SCALE_UP);}
    void Rot90(){processor.QueueProcess(ImageProcessor::ROT90);}
    void Rot180(){processor.QueueProcess(ImageProcessor::ROT180);}
    void Rot270(){processor.QueueProcess(ImageProcessor::ROT270);}
    void setIsovalue(int isovalue){ processor.setIsovalue(isovalue);}
    void setStepSize(int stepsize){ processor.setStepSize(stepsize);}
    void setBinaryInter(bool usebininter){processor.setBinaryInter(usebininter);}
    void toggleBinary(){processor.QueueProcess(ImageProcessor::TOGGLEBINARY);}
    void LoadImage(){processor.QueueProcess(ImageProcessor::LOAD_IMAGE);}
    void GrayScale(){processor.QueueProcess(ImageProcessor::GRAY_SCALE);}
    void save();
};

#endif // IMAGEDISPLAY_H
