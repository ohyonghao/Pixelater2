#ifndef IMAGEDISPLAY_H
#define IMAGEDISPLAY_H

#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QPixmap>
#include <QImage>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QLabel>
#include "bitmap.h"

class ImageDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit ImageDisplay(QString filename, int isovalue=57, int stepsize = 5, QWidget *parent = nullptr);
    bool succeeded(){ return success;}
private:
    void createScene();
    void loadImage();
    QGraphicsScene *scene;
    QGraphicsView *view;
    QGraphicsPixmapItem *item;
    QPixmap  pixmap;
    QImage   *imageData;

    Bitmap _image;
    Bitmap _bimage;
    Bitmap _cimage;
    QString _filename;
    QImage::Format format;

    QLabel *imageLabel;

    bool success = false;
    bool displayBinary = false;

    // Edit values
    int _isovalue = 57;
    int _stepsize = 5;

signals:

public slots:
    void BinaryGray();
    void Pixelate();
    void Blur();
    void Contour();
    void setIsovalue(int isovalue){ _isovalue = isovalue; loadImage();}
    void setStepSize(int stepsize){ _stepsize = stepsize; loadImage();}
    void toggleBinary();
    void save();
};

#endif // IMAGEDISPLAY_H
