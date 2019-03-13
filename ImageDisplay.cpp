#include "ImageDisplay.h"
#include <fstream>
#include <string>
#include <sstream>
#include <QByteArray>
#include <QIODevice>
#include <QTextStream>
#include "bitmap.h"

ImageDisplay::ImageDisplay(QString filename, int isovalue, int stepsize, bool useBinaryInter, QWidget *parent) : QWidget(parent),
    imageLabel{new QLabel(this)},
    processor{filename, isovalue, stepsize, useBinaryInter}
{
    connect(&processor, &ImageProcessor::imageProcessed, this, &ImageDisplay::loadImage);
    connect(&processor, &ImageProcessor::queueUpdated, this, &ImageDisplay::processQueued);
    processor.start();
}
void ImageDisplay::loadImage(const QByteArray &stream){
    pixmap.loadFromData(stream,"BMP");
    imageLabel->setPixmap(pixmap);
    imageLabel->resize(pixmap.size());

    emit imageLoaded();
}
void ImageDisplay::save(){
    //std::ofstream of;
    //of.open( (QString("image_contour.bmp")).toStdString() );
    //of << _image;
}
