#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QFunctionPointer>
#include <functional>
#include <QMutexLocker>
#include "bitmap.h"

class ImageProcessor : public QThread
{
    Q_OBJECT

public:

    ImageProcessor(QString filename, int isovalue, int stepsize, bool useBinaryInter, QObject *parent=nullptr);
    ~ImageProcessor() override;

    void processImage();

    void setIsovalue(int isovalue){ isomutex.lock(); _isovalue = isovalue; isomutex.unlock();
                                    _queueProcess(std::mem_fn(&ImageProcessor::_Reprocess)); }
    void setStepSize(int stepsize){ isomutex.lock(); _stepsize = stepsize; isomutex.unlock();
                                    _queueProcess(std::mem_fn(&ImageProcessor::_Reprocess));}
    void setBinaryInter( bool binaryInter ){binarymutex.lock(); _usebinaryinter = binaryInter; binarymutex.unlock() ;
                                    _queueProcess(std::mem_fn(&ImageProcessor::_Reprocess));}

signals:
    void imageProcessed( const QByteArray &image);
    void queueUpdated(int);

protected:
    void run() override;

private:
    QMutex mutex;
    QMutex qmutex;
    QMutex isomutex;
    QMutex stepsizemutex;
    QMutex binarymutex;

    QWaitCondition condition;

    bool restart;
    bool abort;
    Bitmap _image;
    Bitmap _bimage;
    Bitmap _cimage;

    QString _filename;

    bool success = false;
    bool displayBinary = false;

    // Edit values
    int _isovalue = 57;
    int _stepsize = 5;
    bool _usebinaryinter = true;

public:

    void _BinaryGray();
    void _Pixelate();
    void _Blur();
    void _Contour();
    void _CelShade();
    void _toggleBinary();
    void _LoadImage();
    void _ScaleDown();
    void _ScaleUp();
    void _GrayScale();
    void _restartThread();
    void _Rot90();
    void _Rot180();
    void _Rot270();
    void _Reprocess();

    // Processing functions
    void QueueProcess(decltype(std::mem_fn<void(), ImageProcessor>(&ImageProcessor::_BinaryGray)) process){ _queueProcess(process);}
private:

    QQueue<decltype(std::mem_fn<void(), ImageProcessor>(&ImageProcessor::_BinaryGray))> queued;
    void _queueProcess(decltype(std::mem_fn<void(), ImageProcessor>(&ImageProcessor::_BinaryGray)) process){
        QMutexLocker locker(&qmutex);
        queued.push_back(process);
        emit queueUpdated(queued.size());
        _restartThread();}

};

#endif // IMAGEPROCESSOR_H
