#include "screen.h"

screen::screen(QWidget *parent)
    : QWidget(parent)
{
    vlayout = new QVBoxLayout(this);
    button_getscreen = new QPushButton("开始");
    label_showscreen = new QLabel("无图片");
    pixmap_screen = new QPixmap(1280, 720);
    lineedit_ms = new QLineEdit("300");

    timer = new QTimer(this);
    timer->setSingleShot(true);//更改为只触发一次

    bytearray = new QByteArray(102400, 0);
    buffer = new QBuffer(bytearray, this);

    tcpserver = new QTcpServer(this);
    tcpclient = new QTcpSocket(this);

    tcpserver->listen(QHostAddress::Any, 6655);

    buffer->open(QIODevice::ReadWrite);

    screen_this = QGuiApplication::primaryScreen();
    wid_this = QApplication::desktop()->winId();

    vlayout->addWidget(label_showscreen);
    vlayout->addWidget(lineedit_ms);
    vlayout->addWidget(button_getscreen);

    setwh();
    is_connent = 0;

    QObject::connect(button_getscreen, SIGNAL(clicked(bool)),
                     this, SLOT(slot_getscreen()));
    QObject::connect(timer, SIGNAL(timeout()), SLOT(slot_send()));
    QObject::connect(tcpserver, SIGNAL(newConnection()), this, SLOT(slot_newconnection()));
    QObject::connect(lineedit_ms, SIGNAL(returnPressed()), this, SLOT(slot_change_ms()));
}

screen::~screen()
{
    delete bytearray;
}

void screen::setwh()
{
    w = QApplication::desktop()->width()*0.5;
    h = QApplication::desktop()->height()*0.5;
}

void screen::slot_change_ms()
{
    QString str_tmp;
    str_tmp = lineedit_ms->text();

    if(str_tmp[0] == '0' || str_tmp[1] == '.')
    {
        size_screen = str_tmp.toDouble();
        w = QApplication::desktop()->width()*size_screen;
        h = QApplication::desktop()->height()*size_screen;
        lineedit_ms->setText("修改屏幕成功");
    }
    else
    {
        msc = str_tmp.toInt();
        lineedit_ms->setText("修改毫秒成功");
    }
}

void screen::slot_getscreen()//截屏处理
{
    *pixmap_screen = screen_this->grabWindow(wid_this);
    *pixmap_screen = pixmap_screen->scaled(w, h);

    //label_showscreen->setPixmap(*pixmap_screen);
    pixmap_screen->save(buffer, "jpg");
    size = buffer->pos();
    //qDebug() << size << bytearray->size() << w << h;
    buffer->seek(0);
}

void screen::slot_send()
{
    slot_getscreen();
    //qDebug() << "send";
    tcpclient->write((char*)&size, 4);
    tcpclient->write(bytearray->data(), size);
}

void screen::slot_recv()
{
    tcpclient->read((char*)&recv_size, 4);
    if(recv_size != size)
    {
        qDebug() << recv_size;
        lineedit_ms->setText("传输数据错误");
        tcpclient->reset();
    }
    //qDebug() << "recv";
    timer->start(msc);
}

void screen::slot_newconnection()//新连接处理
{
    if(is_connent)
    {
        lineedit_ms->setText("已有连接");
        return;
    }

    tcpclient = tcpserver->nextPendingConnection();
    QObject::connect(tcpclient, SIGNAL(readyRead()), this, SLOT(slot_recv()));
    QObject::connect(tcpclient, SIGNAL(error(QAbstractSocket::SocketError)),
                     this, SLOT(slot_error()));
    lineedit_ms->setText("新的连接");
    is_connent = 1;
    timer->start(msc);
    //qDebug() << "new connection";
}

void screen::slot_error()//出错处理
{
    is_connent = 0;
    lineedit_ms->setText("断开连接");
    tcpclient->reset();
}
