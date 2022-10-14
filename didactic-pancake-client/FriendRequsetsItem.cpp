#include "FriendRequsetsItem.h"
#include "ui_FriendRequsetsItem.h"

#include <QPainter>
#include <QFile>

FriendRequsetsItem::FriendRequsetsItem(QWidget *parent,const QString& username,QListWidgetItem *item) : QWidget(parent),
    ui(new Ui::FriendRequsetsItem),
    m_item(item)
{
    m_connect = TcpConnect::getInstance();

    ui->setupUi(this);

    ui->lbl_user_name->setText(username);



    initControl();

    setWindowFlags(Qt::FramelessWindowHint);



}

FriendRequsetsItem::~FriendRequsetsItem()
{
    delete ui;
}

void FriendRequsetsItem::setUserName(const QString &username)
{
    ui->lbl_user_name->setText(username);
}

void FriendRequsetsItem::initControl()
{
    QString qss_btn_accept =
            "QPushButton{background-color:rgb(77,218,100);}" //默认
            "QPushButton{color: rgb(255,255,255); }"
            "QPushButton{border-radius: 3px;}"
            "QPushButton:hover{background-color:rgb(57,198,80);}"
            "QPushButton:hover{color: rgb(255,255,255); }"
            "QPushButton:hover{border-radius: 3px;}"
            "QPushButton:pressed{background-color:rgb(37,178,60);}"
            "QPushButton:pressed{color: rgb(255,255,255); }"
            "QPushButton:pressed{border-radius: 3px;}"
            "QPushButton:disabled{background-color:rgb(143,217,152);}"
            "QPushButton:disabled{color: rgb(255,255,255); }"
            "QPushButton:disabled{border-radius: 3px;}";
    ui->btn_accept->setStyleSheet(qss_btn_accept);

    QString qss_btn_reject =
            "QPushButton{background-color:rgb(255,55,45);}" //默认
            "QPushButton{color: rgb(255,255,255); }"
            "QPushButton{border-radius: 3px;}"
            "QPushButton:hover{background-color:rgb(235,35,25);}"
            "QPushButton:hover{color: rgb(255,255,255); }"
            "QPushButton:hover{border-radius: 3px;}"
            "QPushButton:pressed{background-color:rgb(215,15,5);}"
            "QPushButton:pressed{color: rgb(255,255,255); }"
            "QPushButton:pressed{border-radius: 3px;}"
            "QPushButton:disabled{background-color:rgb(143,217,152);}"
            "QPushButton:disabled{color: rgb(255,255,255); }"
            "QPushButton:disabled{border-radius: 3px;}";
    ui->btn_reject->setStyleSheet(qss_btn_reject);

    ui->lbl_user_avatar->setPixmap(QPixmap(":/resource/default_avatar.png"));
    ui->lbl_user_avatar->setScaledContents(true);
    ui->lbl_user_avatar->setCursor(QCursor(Qt::PointingHandCursor));

}

void FriendRequsetsItem::paintEvent(QPaintEvent *event)
{

    //设置背景色;
    QPainter painter(this);
    QPainterPath pathBack;
    pathBack.setFillRule(Qt::WindingFill);
    pathBack.addRoundedRect(QRect(0, 0, this->width(), this->height()), 0, 0);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillPath(pathBack, QBrush(QColor(255, 255, 255)));

    //设置底线
    painter.setPen(QColor(231, 231, 231));                         //设置画笔记颜色
    painter.drawLine(QLineF(0,this->height()-1,this->width(),this->height()-1)); //绘制底线

    QWidget::paintEvent(event);

}

void FriendRequsetsItem::on_btn_accept_clicked()
{
    QString content = ui->lbl_user_name->text()+"\r\n1\r\n";
    // QString转char*
    char *ctmp;
    QByteArray ba = content.toLatin1();
    ctmp = ba.data();

    m_connect->write_data(DataPacket(TcpConnect::RFR, content.length(), ctmp));

    emit deleteMe(m_item);

    delete this;
}

void FriendRequsetsItem::on_btn_reject_clicked()
{
    QString content = ui->lbl_user_name->text()+"\r\n0\r\n";
    // QString转char*
    char *ctmp;
    QByteArray ba = content.toLatin1();
    ctmp = ba.data();

    m_connect->write_data(DataPacket(TcpConnect::RFR, content.length(),ctmp));

    emit deleteMe(m_item);

    delete this;
}

///////////////////////////////////////////////////////////////////////

FriendRequsetsListWidget::FriendRequsetsListWidget(QWidget *parent):
    SuspendedScrollBar_ListWidget(parent)
{
    verticalScrollBar()->setSingleStep(10);

    QFile qss(":/resource/SuspendedScrollbar/listwidget.qss");
    qss.open(QFile::ReadOnly);
    setStyleSheet(qss.readAll());
}

void FriendRequsetsListWidget::addRequest(const QString &username)
{

    QListWidgetItem * pItem = new QListWidgetItem();
    FriendRequsetsItem * w_item = new FriendRequsetsItem(this,username,pItem);
    connect(w_item,&FriendRequsetsItem::deleteMe,this,&FriendRequsetsListWidget::deleteRequest);
    addItem(pItem);
    pItem->setSizeHint(QSize(0,w_item->height()));
    setItemWidget(pItem , w_item);

}

void FriendRequsetsListWidget::deleteRequest(QListWidgetItem *object)
{
    removeItemWidget(object);
    delete object;
}



