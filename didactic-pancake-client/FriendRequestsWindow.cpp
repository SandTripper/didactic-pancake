#include <QCloseEvent>
#include <QPainter>
#include <QString>
#include <QRegExpValidator>

#include "FriendRequestsWindow.h"
#include "ui_FriendRequestsWindow.h"
#include "LabelPlus.h"
#include "RegisterWindow.h"
#include "FriendRequsetsItem.h"

using namespace std;

FriendRequestsWindow::FriendRequestsWindow(QWidget *parent) : BaseWindow(parent),
    ui(new Ui::FriendRequestsWindow)
{

    m_connect = TcpConnect::getInstance();

    connect(m_connect, &TcpConnect::RFRpackAdd, this, &FriendRequestsWindow::handle_new_friend_request);

    ui->setupUi(this);

    initControl();
    initTitleBar();

    setBackgroundColor(255, 255, 255);

    m_titleBar->raise();

}

FriendRequestsWindow::~FriendRequestsWindow()
{
    delete ui;
}

void FriendRequestsWindow::initTitleBar()
{
    m_titleBar->setBackgroundColor(255, 255, 255);
    // m_titleBar->setTitleIcon(":/resource/default_avatar.png");
    // m_titleBar->setTitleContent(QStringLiteral("登录"));
    m_titleBar->setButtonType(MIN_BUTTON);
    m_titleBar->setTitleWidth(this->width());
    m_titleBar->setTitleHeight(25);
}


void FriendRequestsWindow::initControl()
{

}

//模态显示函数
void FriendRequestsWindow::exec()
{
    show();
    m_Loop = new QEventLoop(this);
    m_Loop->exec(); //利用事件循环实现模态
}


//重写关闭事件
void FriendRequestsWindow::closeEvent(QCloseEvent *event)
{
    if (m_Loop != NULL)
    {
        m_Loop->exit();
    }
    event->accept();
}

void FriendRequestsWindow::handle_new_friend_request()
{
    for (const auto &data : m_connect->vec[TcpConnect::RFR])
    {
        if (data.content_len == 0)
            continue;
        QString username = QString::fromStdString(string(data.content,data.content+data.content_len-2));
        ui->listWidget->addRequest(username);
    }
    m_connect->vec[TcpConnect::RFR].clear();
}




