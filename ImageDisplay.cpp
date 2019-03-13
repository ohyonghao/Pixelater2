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
    imageLabel{new QLabel(this)},
    processor{filename, isovalue, stepsize}
{
    connect(&processor, &ImageProcessor::imageProcessed, this, &ImageDisplay::loadImage);
    processor.start();
}
void ImageDisplay::loadImage(const QByteArray &stream){
    bool success = pixmap.loadFromData(stream,"BMP");
    imageLabel->setPixmap(pixmap);
    update();
    qDebug() << (success ? "Loaded Successfully" : "Did not load");
}
void ImageDisplay::save(){
    //std::ofstream of;
    //of.open( (QString("image_contour.bmp")).toStdString() );
    //of << _image;
}

void ImageDisplay::toggleBinary(){
    processor.toggleBinary();
}
