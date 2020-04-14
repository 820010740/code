#include "widget.h"
#include "ui_widget.h"
#include<QDebug>
#include<QFileDialog>
#include<QFile>
#include<QDateTime>
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    connect(&tcpSocket,SIGNAL(readyRead()),
            this,SLOT(onReadyRead()));
    connect(&tcpSocket,SIGNAL(connected()),
            this,SLOT(onConnectd()));
    connect(&tcpSocket,SIGNAL(disconnected()),
            this,SLOT(onDisconnected()));
    connect(&tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),
            this,SLOT(onError(QAbstractSocket::SocketError)));
    tcpSocket.connectToHost("192.168.8.105",8888);
    imageIndex=0;
    sizePackLast=0;

    //ui->splitter->setStretchFactor(0,2);
    //ui->splitter->setStretchFactor(1,1);
    ui->textInput->setFocus();//定位光标
}

Widget::~Widget()
{
    delete ui;
}

void Widget::onReadyRead()//接收数据
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
            ui->textInput->setFocus();//定位光标
            //QString textContent=dataFull.mid(4);
            QByteArray byteArray = dataFull.mid(4);
            QString str = QString::fromLocal8Bit(byteArray.constData());//显示中文
            ui->textMsg->append("<p>"+str+"</p><br>");
            ui->textInput->setFocus();//定位光标
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
            ui->textInput->setFocus();//定位光标到
            ui->textMsg->append(htmlTag);//在文件尾插入
            ui->textInput->setFocus();//定位光标
        }
        else if(prefix=="File")
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

void Widget::onConnected()//连接
{
    qDebug()<<"connected";
}

void Widget::onDisconnectd()//断开
{
    qDebug()<<"disconnected";
    QObject *obj=this->sender();
    QTcpSocket *socket=qobject_cast<QTcpSocket*>(obj);
    if(socket==0)
    {
        return;
    }
    socket->close();
}

void Widget::onError(QAbstractSocket::SocketError socketError)
{
    qDebug()<<"error:"<<socketError;
}

void Widget::on_btnSend_5_clicked()//发送文字
{
    QString strSend=ui->textInput->toPlainText();
    if(strSend.isEmpty())
    {
        return;
    }
    QString msgInput ="TXT:"+strSend;

    //封装包头
    QByteArray dataSend;
    QDataStream stream(&dataSend,QIODevice::WriteOnly);
    stream<<(quint32)0<<msgInput.toLocal8Bit();
    stream.device()->seek(0);
    stream<<dataSend.size();

    tcpSocket.write(dataSend);

    ui->textInput->clear();
    ui->textInput->setFocus();//定位光标
}

void Widget::on_btnImage_5_clicked()//发送图片
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

    tcpSocket.write(dataSend);
    ui->textInput->setFocus();//定位光标
}

void Widget::on_btnFile_5_clicked()//发送文件
{
    QString filePath = QFileDialog::getOpenFileName(this, "open", "../");
    if(false == filePath.isEmpty()) //如果选择文件路径有效
    {
        //提示打开文件的路径
        ui->textMsg->append(filePath);
    }
    //获取文件信息
    QFileInfo info(filePath);
    fileName = info.fileName();//获取文件名字

    QFile file(filePath);
    file.open(QIODevice::ReadOnly);
    //先发送文件头信息  文件名##文件大小
    QString head = QString("%1##%2").arg(fileName);
    QByteArray data="File"+ head.toUtf8()+file.readAll();
    file.close();

    //封装包头
    QByteArray dataSend;
    QDataStream stream(&dataSend,QIODevice::WriteOnly);
    stream<<(quint32)0<<data;
    stream.device()->seek(0);
    stream<<dataSend.size();
    tcpSocket.write(dataSend);
    ui->textInput->setFocus();
}
