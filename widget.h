#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include<QList>
#include<QTimer>
namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();
public slots:

    void onReadyRead();
    void onConnected();
    void onDisconnectd();
    void onError(QAbstractSocket::SocketError socketError);
private slots:
    void on_btnSend_5_clicked();

    void on_btnImage_5_clicked();

    void on_btnFile_5_clicked();

private:
    Ui::Widget *ui;
    QTcpSocket tcpSocket;
    int imageIndex;
    QString fileName; //文件名字

    qint32 sizePackLast;



};

#endif // WIDGET_H
