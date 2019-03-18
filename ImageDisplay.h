#ifndef IMAGEDISPLAY_H
#define IMAGEDISPLAY_H

#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QPixmap>
#include <QImage>
#include <QLabel>
#include <QByteArray>
#include <functional>
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
    void BinaryGray(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_BinaryGray));}
    void Pixelate(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_Pixelate));}
    void Blur(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_Blur));}
    void Contour(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_Contour));}
    void CelShade(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_CelShade));}
    void ScaleDown(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_ScaleDown));}
    void ScaleUp(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_ScaleUp));}
    void Rot90(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_Rot90));}
    void Rot180(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_Rot180));}
    void Rot270(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_Rot270));}
    void setIsovalue(int isovalue){ processor.setIsovalue(isovalue);}
    void setStepSize(int stepsize){ processor.setStepSize(stepsize);}
    void setBinaryInter(bool usebininter){processor.setBinaryInter(usebininter);}
    void toggleBinary(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_toggleBinary));}
    void LoadImage(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_LoadImage));}
    void GrayScale(){processor.QueueProcess(std::mem_fn(&ImageProcessor::_GrayScale));}
    void save();
};

#endif // IMAGEDISPLAY_H
