#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMediaPlayer>

#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioInput>
#include <QIODevice>
#include <QByteArray>

#include <QTimer>


namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();    
    void startGame();
    void stopGame();


private slots:
    void on_startButton_clicked();

    void on_stopButton_clicked();
    void isClapped();
    void delayChange();
    void readMore();

private:
    Ui::Widget *ui;
    QMediaPlayer *player = new QMediaPlayer;

    QAudioDeviceInfo m_inputdevice;
    QAudioFormat m_format;
    QAudioInput *m_audioinput;
    QIODevice *m_input;
    QByteArray m_buffer;
    bool m_isClapped;
    int score;
    QTimer * clapTimer = new QTimer(this);

    void initAudio();
    void createAudio();

};

#endif // WIDGET_H
