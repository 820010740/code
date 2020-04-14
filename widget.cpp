#include "widget.h"
#include "ui_widget.h"
#include<QDebug>
#include<QHostAddress>
#include<QFileDialog>
#include<QFile>
#include<QDataStream>
#include<QDateTime>
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    connect(&sever,SIGNAL(newConnection()),this,SLOT(onNewConnection()));

    bool ok = sever.listen(QHostAddress::AnyIPv4,8888);//监听端口
    qDebug()<<"listen::"<<ok;

    imageIndex=0;
    sizePackLast=0;

    ui->splitter->setStretchFactor(0,2);
    ui->splitter->setStretchFactor(1,1);

    ui->textInput->setFocus();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::onNewConnection()
{
    //使用这个socket客户端进行通信
    QTcpSocket *socket =sever.nextPendingConnection();//获得socket
    qDebug()<<"123";
    clients.append(socket);//进入监听
    //连接信号和槽
    connect(socket,SIGNAL(readyRead()),
            this,SLOT(onReadyRead()));
    connect(socket,SIGNAL(connected()),
            this,SLOT(onConnected()));
    connect(socket,SIGNAL(disconnected()),
            this,SLOT(onDisconnectd()));
    connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),
            this,SLOT(onError(QAbstractSocket::SocketError)));

}

void Widget::onReadyRead()//接收数据函数
{
    QObject *obj=this->sender();
    QTcpSocket *socket=qobject_cast<QTcpSocket*>(obj);
    //当前缓冲区里的数据大小,收到的数据大小.
    qint64 sizeNow=0;
    do
    {
        sizeNow =socket->bytesAvailable();
        QDataStream stream(socket);
        if(sizePackLast==0)
        {
            if(sizeNow <sizeof(quint32))
            {
                return;
            }
            stream >>sizePackLast;//已经有值了
        }
        //包不完整返回,等待包完整;
        if(sizeNow<sizePackLast -4 )
        {
            return;
        }
        qDebug()<<"full pack";

        QByteArray dataFull;
        stream >>dataFull;
        //设置为0
        sizePackLast=0;

        //判断剩下的字节数,是否会有粘包的情况
        sizeNow =socket->bytesAvailable();

        QString prefix=dataFull.mid(0,4);
        QString dataTime=QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        if(prefix=="TXT:"){
            ui->textMsg->append(dataTime);
            ui->textInput->setFocus();
            //QString textContent=dataFull.mid(4);
            QByteArray byteArray = dataFull.mid(4);
            QString str = QString::fromLocal8Bit(byteArray.constData());
            ui->textMsg->append("<p>"+str+"</p><br>");
            ui->textInput->setFocus();
        }
        else if(prefix=="IMG:")
        {
            QString index=QString::number(imageIndex++);
            QFile file(index+".png");
            file.open(QIODevice::WriteOnly);
            file.write(dataFull.mid(4));
            file.close();

            QString htmlTag=QString("<img src=\"%1\"></img><br>");
            htmlTag=htmlTag.arg(index+".png");
            ui->textMsg->append("<p>"+dataTime+"</p><br>");
            ui->textInput->setFocus();
            ui->textMsg->append(htmlTag);
            ui->textInput->setFocus();
        }
        else if(prefix=="File")//接收File的信息
        {
            fileName = QString(dataFull).section("##", 0, 0);
            QFile file(fileName);
            file.open(QIODevice::WriteOnly);
            file.write(dataFull.mid(4));
            file.close();
            ui->textMsg->append("<p>"+dataTime+"文件接收完毕""</p><br>");
            ui->textInput->setFocus();//定位光标
        }
    }while(sizeNow>0);
}

void Widget::onConnected()//连接成功
{
    qDebug()<<"connected";
}

void Widget::onDisconnectd()//断开
{
    QObject *obj=this->sender();
    QTcpSocket *socket=qobject_cast<QTcpSocket*>(obj);
    clients.removeAll(socket);
    socket->deleteLater();
    qDebug()<<"disconnected";
}

void Widget::onError(QAbstractSocket::SocketError socketError)//通讯出错
{
    qDebug()<<"error:"<<socketError;
}

void Widget::on_btnSend_clicked()
{
    QString strSend=ui->textInput->toPlainText();
    if(strSend.isEmpty())
    {
        return;
    }
    for(QList<QTcpSocket*>::iterator itr =clients.begin();itr!=clients.end();++itr)
    {
        QTcpSocket *client =*itr;

        QString msgInput="TXT:"+strSend;
        //封装包头
        QByteArray dataSend;
        QDataStream stream(&dataSend,QIODevice::WriteOnly);
        stream<<(quint32)0<<msgInput.toLocal8Bit();
        stream.device()->seek(0);
        stream<<dataSend.size();
        client->write(dataSend);
    }
    ui->textInput->clear();
    ui->textInput->setFocus();
}

void Widget::on_btnImage_clicked()
{
    QString image=QFileDialog::getOpenFileName(this,"title",".","*.png *.jpg *.bmp");
    if(image.isEmpty())
        return;
    QFile file(image);
    file.open(QIODevice::ReadOnly);
    QByteArray data="IMG:"+file.readAll();
    file.close();

    //封装包头
    QByteArray dataSend;
    QDataStream stream(&dataSend,QIODevice::WriteOnly);
    stream<<(quint32)0<<data;
    stream.device()->seek(0);
    stream<<dataSend.size();

    for(QList<QTcpSocket*>::iterator itr =clients.begin();itr!=clients.end();++itr)
    {
        QTcpSocket *client =*itr;

        client->write(dataSend);
    }
    ui->textInput->setFocus();

}

void Widget::on_btnFile_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "open", "../");
    if(false == filePath.isEmpty()) //如果选择文件路径有效
    {
        //提示打开文件的路径
        ui->textMsg->append(filePath);
    }
    QFile file(filePath);
    //获取文件信息
    QFileInfo info(filePath);
    fileName = info.fileName();//获取文件名字

    file.open(QIODevice::ReadOnly);
    //先发送文件头信息  文件名##文件大小
    QString head = QString("%1##%2").arg(fileName);
    QByteArray data="File"+head.toUtf8()+file.readAll();
    file.close();
    //封装包头
    QByteArray dataSend;
    QDataStream stream(&dataSend,QIODevice::WriteOnly);
    stream<<(quint32)0<<data;
    stream.device()->seek(0);
    stream<<dataSend.size();

    for(QList<QTcpSocket*>::iterator itr =clients.begin();itr!=clients.end();++itr)
    {
        QTcpSocket *client =*itr;

        client->write(dataSend);
    }
    ui->textInput->setFocus();

}
