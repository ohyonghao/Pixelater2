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
                                    _queueProcess(std::mem_fn(&ImageProcessor::Reprocess)); }
    void setStepSize(int stepsize){ isomutex.lock(); _stepsize = stepsize; isomutex.unlock();
                                    _queueProcess(std::mem_fn(&ImageProcessor::Reprocess));}
    void setBinaryInter( bool binaryInter ){binarymutex.lock(); _usebinaryinter = binaryInter; binarymutex.unlock() ;
                                    _queueProcess(std::mem_fn(&ImageProcessor::Reprocess));}

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
    // Processing functions
    void BinaryGray();
    void Pixelate();
    void Blur();
    void Contour();
    void CelShade();
    void toggleBinary();
    void LoadImage();
    void ScaleDown();
    void ScaleUp();
    void GrayScale();
    void restartThread();
    void Rot90();
    void Rot180();
    void Rot270();
    void Reprocess();

    typedef decltype(std::mem_fn<void(), ImageProcessor>(&ImageProcessor::BinaryGray)) mfptr;
    void QueueProcess(mfptr process){ _queueProcess(process);}
private:

    QQueue<mfptr> queued;
    void _queueProcess(mfptr process){
        QMutexLocker locker(&qmutex);
        queued.push_back(process);
        emit queueUpdated(queued.size());
        restartThread();}

};

#endif // IMAGEPROCESSOR_H
