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

    enum Processes {BINARY_GRAY, PIXELATE, BLUR, CONTOUR, CELSHADE, TOGGLEBINARY, ISO, STEP, LOAD_IMAGE, PROCESS};
public:
    ImageProcessor(QString filename, int isovalue, int stepsize, bool useBinaryInter, QObject *parent=nullptr);
    ~ImageProcessor() override;

    void processImage();

    void setIsovalue(int isovalue){ isomutex.lock(); _isovalue = isovalue; isomutex.unlock(); _queueProcess(ISO); }
    void setStepSize(int stepsize){ isomutex.lock(); _stepsize = stepsize; isomutex.unlock(); _queueProcess(STEP);}
    void setBinaryInter( bool binaryInter ){binarymutex.lock(); _usebinaryinter = binaryInter; binarymutex.unlock() ;_queueProcess(PROCESS);}

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

    QQueue<Processes> queued;

    QString _filename;

    bool success = false;
    bool displayBinary = false;

    // Edit values
    int _isovalue = 57;
    int _stepsize = 5;
    bool _usebinaryinter = true;

    void _BinaryGray();
    void _Pixelate();
    void _Blur();
    void _Contour();
    void _CelShade();
    void _toggleBinary();
    void _LoadImage();
    void _restartThread();

    void _queueProcess(Processes process){
        QMutexLocker locker(&qmutex);
        queued.push_back(process);
        emit queueUpdated(queued.size());
        _restartThread();}

public:
    // Processing functions
    void BinaryGray(){_queueProcess(BINARY_GRAY);}
    void Pixelate(){_queueProcess(PIXELATE);}
    void Blur(){_queueProcess(BLUR);}
    void Contour(){_queueProcess(CONTOUR);}
    void CelShade(){_queueProcess(CELSHADE);}
    void toggleBinary(){_queueProcess(TOGGLEBINARY);}
    void LoadImage(){_queueProcess(LOAD_IMAGE);}
};

#endif // IMAGEPROCESSOR_H
