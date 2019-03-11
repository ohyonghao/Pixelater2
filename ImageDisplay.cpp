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
    in >> _image;
    _cimage = _image;

    createScene();
}
void ImageDisplay::createScene(){
    imageLabel = new QLabel(this);
    loadImage();
}
void ImageDisplay::loadImage(){

    qDebug() << tr("Loading Image Parameters: %1, %2").arg(_isovalue).arg(_stepsize);
    // Load image
    if(displayBinary){
        _bimage = _image;
        binaryGray(_bimage, _isovalue);
        _cimage = _bimage;
    }else{
        _cimage = _image;
    }

    contours(_cimage, _isovalue, _stepsize);
    std::ostringstream imageArray;
    imageArray << _cimage;
    QByteArray stream(imageArray.str().data(), imageArray.str().size());
    success = pixmap.loadFromData(stream,"BMP");
    imageLabel->setPixmap(pixmap);

}
void ImageDisplay::BinaryGray(){
    binaryGray(_image, _isovalue);
    loadImage();
}

void ImageDisplay::Pixelate(){
    pixelate(_image);
    loadImage();
}

void ImageDisplay::Blur(){
    blur(_image);
    loadImage();
}

void ImageDisplay::Contour(){
    loadImage();
}
void ImageDisplay::save(){
    std::ofstream of;
    of.open( (QString("image_contour.bmp")).toStdString() );
    of << _image;
}

void ImageDisplay::toggleBinary(){
    displayBinary = !displayBinary;
    loadImage();
}
