#include <fstream>
#include <string>
#include <sstream>
#include <QByteArray>
#include <QIODevice>
#include <QTextStream>
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
}

void ImageProcessor::_LoadImage(){

    std::ifstream in;
    in.open(_filename.toStdString(), ios::binary);

    QMutexLocker locker(&mutex);
    in >> _image;
    _cimage = _image;
    _bimage = _image;
    binaryGray(_bimage, _isovalue);

}

void ImageProcessor::_Blur(){
    QMutexLocker locker(&mutex);
    blur(_image);
}
void ImageProcessor::_Contour(){
}
void ImageProcessor::_CelShade(){
    QMutexLocker locker(&mutex);
    cellShade(_image);
}
void ImageProcessor::_Pixelate(){
    QMutexLocker locker(&mutex);
    pixelate(_image);
}
void ImageProcessor::_BinaryGray(){
    QMutexLocker locker(&mutex);
    binaryGray(_image, _isovalue);
}
void ImageProcessor::_toggleBinary(){
    displayBinary = !displayBinary;
}

void ImageProcessor::run(){
    forever{
        if( abort )
            return;
        while(!queued.isEmpty()){
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
                // Not sure if I need this, but could make things faster.
//                if(!queued.isEmpty()){
//                    // Skip processing if another command waits
//                    continue;
//                }
                break;
            case LOAD_IMAGE:
                _LoadImage();
                break;
            default:
                continue;
            }
            if(queued.empty()){
                processImage();
                std::ostringstream imageArray;
                mutex.lock();
                imageArray << _cimage;
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
