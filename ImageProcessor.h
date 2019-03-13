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

    enum Processes {BINARY_GRAY, PIXELATE, BLUR, CONTOUR, CELSHADE, TOGGLEBINARY, PROCESS, LOAD_IMAGE};
public:
    ImageProcessor(QString filename, int isovalue, int stepsize, QObject *parent=nullptr);
    ~ImageProcessor() override;

    void processImage();

    void setIsovalue(int isovalue){ QMutexLocker locker(&mutex); _isovalue = isovalue; queued.push_back(PROCESS); _restartThread();}
    void setStepSize(int stepsize){ QMutexLocker locker(&mutex); _stepsize = stepsize; queued.push_back(PROCESS); _restartThread();}

signals:
    void imageProcessed( const QByteArray &image);

protected:
    void run() override;

private:
    QMutex mutex;
    QMutex qmutex;
    QWaitCondition condition;
    bool restart;
    bool abort;
    Bitmap _image;
    Bitmap _bimage;
    Bitmap _cimage;

    QQueue<Processes> queued;

    QString _filename;

    bool success = false;
    bool displayBinary = false;

    // Edit values
    int _isovalue = 57;
    int _stepsize = 5;

    void _BinaryGray();
    void _Pixelate();
    void _Blur();
    void _Contour();
    void _CelShade();
    void _toggleBinary();
    void _LoadImage();
    void _restartThread();

public:
    // Processing functions
    void BinaryGray(){QMutexLocker locker(&qmutex);queued.push_back(BINARY_GRAY);_restartThread();}
    void Pixelate(){QMutexLocker locker(&qmutex);queued.push_back(PIXELATE);_restartThread();}
    void Blur(){QMutexLocker locker(&qmutex);queued.push_back(BLUR);_restartThread();}
    void Contour(){QMutexLocker locker(&qmutex);queued.push_back(CONTOUR);_restartThread();}
    void CelShade(){QMutexLocker locker(&qmutex);queued.push_back(CELSHADE);_restartThread();}
    void toggleBinary(){QMutexLocker locker(&qmutex);queued.push_back(TOGGLEBINARY);_restartThread();}
};

#endif // IMAGEPROCESSOR_H
