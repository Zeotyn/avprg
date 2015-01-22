#include "widget.h"
#include "ui_widget.h"
#include <qendian.h>

const int BufferSize = 14096;

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    m_player(new QMediaPlayer),
    m_inputdevice(QAudioDeviceInfo::defaultInputDevice()),
    m_audioinput(0),
    m_input(0),
    m_buffer(BufferSize,0),
    m_isClapped(false),
    m_score(0),    
    m_clapTimer(new QTimer(parent)),
    m_levelRequired(0.1),
    m_countdown(4)

{
    ui->setupUi(this);
    ui->backgroundFrame->setStyleSheet("background-color: black;");
    ui->gameFrame->setStyleSheet("background-color: grey;");

    // Setup audio.
    initAudio();
}

Widget::~Widget()
{
    delete ui;
}

void changeCountdownLabel() {


}

/**
 * Setup audio needs like audioformat and prepare audioinput.
 * @brief Widget::initAudio
 */
void Widget::initAudio()
{
    m_format.setSampleRate(8000);
    // Channels set to mono.
    m_format.setChannelCount(1);
    // Set sample size.
    m_format.setSampleSize(16);
    // Sample type.
    m_format.setSampleType(QAudioFormat::SignedInt);
    // Byte order.
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    // Codec as simple audio/pcm.
    m_format.setCodec("audio/pcm");

    // Check if configured audioformat is supported.
    QAudioDeviceInfo infoIn(QAudioDeviceInfo::defaultInputDevice());
    if (!infoIn.isFormatSupported(m_format))
    {
        qWarning() << "Default format not supported - trying to use nearest.";
        m_format = infoIn.nearestFormat(m_format);
    }

    // Create audioinput.
    createAudio();
}

void Widget::createAudio()
{
    if(m_input != 0)
    {
        disconnect(m_input, 0, this, 0);
        m_input = 0;
    }

    m_audioinput = new QAudioInput(m_inputdevice, m_format, this);
}

void Widget::readAudio()
{
    // Exit if no audioinput is present.
    if(!m_audioinput)
        return;

    // Check data available to read.
    qint64 len = m_audioinput->bytesReady();
    quint32 m_maxAmplitude;

    switch (m_format.sampleSize()) {
    case 8:
        switch (m_format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 255;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 127;
            break;
        default:
            break;
        }
        break;
    case 16:
        switch (m_format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 65535;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 32767;
            break;
        default:
            break;
        }
        break;

    case 32:
        switch (m_format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 0xffffffff;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 0x7fffffff;
            break;
        case QAudioFormat::Float:
            m_maxAmplitude = 0x7fffffff; // Kind of
        default:
            break;
        }
        break;

    default:
        break;
    }

    if(len > 4096) {
        len = 4096;
    }

    // Read input stream and store into our buffer.
    qint64 l = m_input->read(m_buffer.data(), len);

    /*************************************************************************************************
     * @TODO: Clean up and documentation the following lines.
     */



    const int channelBytes = m_format.sampleSize() / 8;
    const int sampleBytes = m_format.channelCount() * channelBytes;
    const int numSamples = len / sampleBytes;
    quint32 maxValue = 0;
    const unsigned char *ptr = reinterpret_cast<const unsigned char *>(m_buffer.data());

    if(l > 0) {

        for (int i = 0; i < numSamples; ++i) {
            for (int j = 0; j < m_format.channelCount(); ++j) {
                quint32 value = 0;

                if (m_format.sampleSize() == 8 && m_format.sampleType() == QAudioFormat::UnSignedInt) {
                    value = *reinterpret_cast<const quint8*>(ptr);
                } else if (m_format.sampleSize() == 8 && m_format.sampleType() == QAudioFormat::SignedInt) {
                    value = qAbs(*reinterpret_cast<const qint8*>(ptr));
                } else if (m_format.sampleSize() == 16 && m_format.sampleType() == QAudioFormat::UnSignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint16>(ptr);
                    else
                        value = qFromBigEndian<quint16>(ptr);
                } else if (m_format.sampleSize() == 16 && m_format.sampleType() == QAudioFormat::SignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qAbs(qFromLittleEndian<qint16>(ptr));
                    else
                        value = qAbs(qFromBigEndian<qint16>(ptr));
                } else if (m_format.sampleSize() == 32 && m_format.sampleType() == QAudioFormat::UnSignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint32>(ptr);
                    else
                        value = qFromBigEndian<quint32>(ptr);
                } else if (m_format.sampleSize() == 32 && m_format.sampleType() == QAudioFormat::SignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qAbs(qFromLittleEndian<qint32>(ptr));
                    else
                        value = qAbs(qFromBigEndian<qint32>(ptr));
                } else if (m_format.sampleSize() == 32 && m_format.sampleType() == QAudioFormat::Float) {
                    value = qAbs(*reinterpret_cast<const float*>(ptr) * 0x7fffffff); // assumes 0-1.0
                }


                maxValue = qMax(value, maxValue);
                ptr += channelBytes;
            }
        }
        maxValue = qMin(maxValue, m_maxAmplitude);
        qreal level = qreal(maxValue) / m_maxAmplitude;

        qDebug() << "Level: " << level;

        if(level > m_levelRequired) {
            m_isClapped = true;
        }
        else {
            m_isClapped = false;
        }
    }
}

void Widget::on_startButton_clicked()
{


    m_countdownTimer->start(1000);
    connect(m_countdownTimer, SIGNAL(timeout()), this, SLOT(subCountdown()));

    // Start microphone listening.
    m_input = m_audioinput->start();

    // Connect readyRead signal to readMre slot.
    connect(m_input, SIGNAL(readyRead()), SLOT(readAudio()));
}

void Widget::on_stopButton_clicked()
{
    //    m_player->stop();
ui->gameFrame->setStyleSheet("background-color: grey;");
ui->countdownLabel->setText("5");

m_countdown = 4;
m_player->stop();
m_clapTimer->stop();
m_countdownTimer->stop();

    // Stop audioinput.
    m_audioinput->stop();
}

void Widget::setDelay() {
    ui->gameFrame->setStyleSheet("background-color: red;");
    m_clapTimer->start(566);
}

void Widget::isClapped(){
    ui->gameFrame->setStyleSheet("background-color: green;");
    if(m_isClapped == true) {
           m_score+=10;
           QString scoreString = QString::number(m_score);
           ui->scoreLabel->setText(scoreString);
    }
    QTimer::singleShot(250, this, SLOT(setDelay()));
}

void Widget::subCountdown() {
    QString countdownString = QString::number(m_countdown);
    ui->countdownLabel->setText(countdownString);
    if(m_countdown == 0) {
        m_countdownTimer->stop();
        startGame();
        ui->countdownLabel->setText(" ");
    } else {
        m_countdown--;
    }
}

void Widget::startGame() {
    m_player->setMedia(QUrl::fromLocalFile("../clapclap/music.mp3"));
    m_player->setVolume(50);
    m_player->play();

    m_clapTimer->start(566);
    connect(m_clapTimer, SIGNAL(timeout()), this, SLOT(isClapped()));
}
