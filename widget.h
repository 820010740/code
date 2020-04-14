#ifndef WIDGET_H
#define WIDGET_H
#include <QWidget>
#include <QTcpSocket>
#include <QTcpServer>
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
    void onNewConnection();

    void onReadyRead();
    void onConnected();
    void onDisconnectd();
    void onError(QAbstractSocket::SocketError socketError);

private slots:
    void on_btnSend_clicked();

    void on_btnImage_clicked();

    void on_btnFile_clicked();

private:
    Ui::Widget *ui;
    QTcpServer sever;
    QList<QTcpSocket*>clients;

    int imageIndex;
    QString fileName;
    quint32 sizePackLast;


};

#endif // WIDGET_H
