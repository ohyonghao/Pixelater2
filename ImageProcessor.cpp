#include <fstream>
#include <string>
#include <sstream>
#include <QByteArray>
#include <QIODevice>
#include <QTextStream>
#include <QDebug>
#include "ImageProcessor.h"

ImageProcessor::ImageProcessor(QString filename, int isovalue, int stepsize, QObject *parent):
    QThread{parent},
    restart{false},
    abort{false},
    _filename{filename},
    _isovalue{isovalue},
    _stepsize{stepsize}
{
    queued.push_back(LOAD_IMAGE);
}

ImageProcessor::~ImageProcessor(){
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}
void ImageProcessor::_restartThread(){
if (!isRunning()) {
        start(LowPriority);
    } else {
        condition.wakeOne();
    }
}
void ImageProcessor::processImage(){
    qDebug() << "ProcessImage entered";
    QMutexLocker locker(&mutex);
    // Load image
    if(displayBinary){
        _bimage = _image;
        binaryGray(_bimage, _isovalue);
        _cimage = _bimage;
    }else{
        _cimage = _image;
    }

    contours(_cimage, _isovalue, _stepsize);
    qDebug() << "ProcessImage exited";
}

void ImageProcessor::_LoadImage(){

    std::ifstream in;
    in.open(_filename.toStdString(), ios::binary);
    in >> _image;
    _cimage = _image;
    mutex.lock();
    _bimage = _image;
    binaryGray(_bimage, _isovalue);
    mutex.unlock();

}

void ImageProcessor::_Blur(){
    mutex.lock();
    blur(_image);
    mutex.unlock();
    processImage();
}
void ImageProcessor::_Contour(){
    processImage();
}
void ImageProcessor::_CelShade(){
    mutex.lock();
    cellShade(_image);
    mutex.unlock();
    processImage();
}
void ImageProcessor::_Pixelate(){
    mutex.lock();
    pixelate(_image);
    mutex.unlock();
    processImage();
}
void ImageProcessor::_BinaryGray(){
    mutex.lock();
    binaryGray(_image, _isovalue);
    mutex.unlock();
    restart=true;
}
void ImageProcessor::_toggleBinary(){
    displayBinary = !displayBinary;
    restart = true;
}

void ImageProcessor::run(){
    forever{
        while(!queued.isEmpty()){
            qDebug() << "Processing Commands";
            qmutex.lock();
            auto func = queued.takeFirst();
            qmutex.unlock();
            switch(func){
            case BINARY_GRAY:
                _BinaryGray();
                break;
            case PIXELATE:
                _Pixelate();
                break;
            case BLUR:
                _Blur();
                break;
            case CONTOUR:
                _Contour();
                break;
            case CELSHADE:
                _CelShade();
                break;
            case TOGGLEBINARY:
                _toggleBinary();
                break;
            case PROCESS:
                break;
            case LOAD_IMAGE:
                _LoadImage();
                break;
            default:
                continue;
            }
            processImage();
            if(queued.empty()){
                std::ostringstream imageArray;
                mutex.lock();
                imageArray << _image;
                mutex.unlock();
                QByteArray stream(imageArray.str().data(), imageArray.str().size());
                emit imageProcessed(stream);
            }
            mutex.lock();
            if(queued.isEmpty()){
                condition.wait(&mutex);
            }
            mutex.unlock();
        }
    }
}
