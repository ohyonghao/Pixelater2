#include <fstream>
#include <string>
#include <sstream>
#include <QByteArray>
#include <QIODevice>
#include <QTextStream>
#include "ImageProcessor.h"

ImageProcessor::ImageProcessor(QString filename, int isovalue, int stepsize, bool useBinaryInter, QObject *parent):
    QThread{parent},
    restart{false},
    abort{false},
    _filename{filename},
    _isovalue{isovalue},
    _stepsize{stepsize},
    _usebinaryinter{useBinaryInter}
{
    _queueProcess(LOAD_IMAGE);
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
    isomutex.lock();      int iso         = _isovalue;          isomutex.unlock();
    stepsizemutex.lock(); int stepsize    = _stepsize;     stepsizemutex.unlock();
    binarymutex.lock();  bool usebininter = _usebinaryinter; binarymutex.unlock();
    // Load image
    if(displayBinary){
        _bimage = _image;
        binaryGray(_bimage, iso);
        _cimage = _bimage;
    }else{
        _cimage = _image;
    }

    contours(_cimage, iso, stepsize, usebininter);
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
            emit queueUpdated(queued.size());
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
            case ISO:
                if(!queued.isEmpty() && queued.first() == ISO){
                    // Skip processing if another command waits
                    continue;
                }
                break;
            case STEP:
                if(!queued.isEmpty() && queued.first() == STEP){
                // Skip processing if another command waits
                continue;
                }
                break;
            case LOAD_IMAGE:
                _LoadImage();
                break;
            case PROCESS:
                break;
            default:
                continue;
            }
            //if(queued.empty()){
                processImage();
                std::ostringstream imageArray;
                mutex.lock();
                imageArray << _cimage;
                mutex.unlock();
                QByteArray stream(imageArray.str().data(), imageArray.str().size());
                emit imageProcessed(stream);
            //}
            mutex.lock();
            if(queued.isEmpty()){
                condition.wait(&mutex);
            }
            mutex.unlock();
        }
    }
}
