#include "ImageDisplay.h"
#include <fstream>
#include <string>
#include <sstream>
#include <QByteArray>
#include <QIODevice>
#include <QTextStream>
#include <QDebug>
#include "bitmap.h"

ImageDisplay::ImageDisplay(QString filename, int isovalue, int stepsize, QWidget *parent) : QWidget(parent),
  _filename{filename},
  _isovalue{isovalue},
  _stepsize{stepsize}
{
    std::ifstream in;
    in.open(_filename.toStdString(), ios::binary);
    in >> image;
    _cimage = image;

    createScene();
}
void ImageDisplay::createScene(){
    imageLabel = new QLabel(this);
    loadImage();
}
void ImageDisplay::loadImage(){

    qDebug() << tr("Loading Image Parameters: %1, %2").arg(_isovalue).arg(_stepsize);
    // Load image
    _cimage = image;

    contours(_cimage, _isovalue, _stepsize);
    std::ostringstream imageArray;
    imageArray << _cimage;
    QByteArray stream(imageArray.str().data(), imageArray.str().size());
    success = pixmap.loadFromData(stream,"BMP");
    imageLabel->setPixmap(pixmap);

}
void ImageDisplay::BinaryGray(){
    binaryGray(image, _isovalue);
    loadImage();
}

void ImageDisplay::Pixelate(){
    pixelate(image);
    loadImage();
}

void ImageDisplay::Blur(){
    blur(image);
    loadImage();
}

void ImageDisplay::Contour(){
    loadImage();
}
void ImageDisplay::save(){
    std::ofstream of;
    of.open( (QString("image_contour.bmp")).toStdString() );
    of << image;
}
