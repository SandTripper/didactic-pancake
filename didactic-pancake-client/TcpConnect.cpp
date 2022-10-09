#include "TcpConnect.h"

#include <stdio.h>
#include <exception>
#include <QDebug>
#include <QtNetwork>
#include <string>
#include <QObject>
#include <iostream>
#include <QDebug>

using namespace std;

TcpConnect *TcpConnect::getInstance()
{
    static TcpConnect tcpConn;
    return &tcpConn;
}

TcpConnect::TcpConnect()
{

    for (int i = 0; i < MAX_CATEGORY; i++)
    {
        vec.emplace_back(vector<DataPacket>());
    }

    m_client = new QTcpSocket();
    connect(m_client, &QTcpSocket::readyRead, this, &TcpConnect::read_handler);

    m_client->connectToHost(QHostAddress("1.117.146.195"), 4399);
}

TcpConnect::~TcpConnect()
{
    m_client->close();
    delete m_client;
}

const char *TcpConnect::ReqToString(TcpConnect::REQUEST r)
{
    switch (r)
    {
    case HBT:
        return "HBT";
    case LGN:
        return "LGN";
    case RGT:
        return "RGT";
    case LGT:
        return "LGT";
    default:
        return "ERR";
    }
}

void TcpConnect::init()
{
    m_bytes_have_send = 0;
    m_bytes_to_send = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;

    m_method = HBT;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    m_content_length = 0;
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
}

void TcpConnect::read_handler()
{
    if (read())
    {
        RESULT_CODE read_ret = process_read();
        if (read_ret == NO_REQUEST)
        {
            return;
        }
        vec[m_method].emplace_back(DataPacket(m_method, m_string, m_content_length));

        //发出数据包增加信号
        switch (m_method)
        {
        case HBT:
            // emit HBTpackAdd();
            break;
        case LGN:
            emit LGNpackAdd();
            break;
        case RGT:
            emit RGTpackAdd();
            break;
        default:
            break;
        }

        init();
    }
}

bool TcpConnect::read()
{
    if (m_read_idx >= READ_BUFFER_SIZE)
    {
        return false;
    }
    int bytes_read = 0;

    bytes_read = m_client->read(m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx);
    m_read_idx += bytes_read;
    if (bytes_read <= 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

TcpConnect::RESULT_CODE TcpConnect::process_read()
{
    LINE_STATUS linestatus = LINE_OK; //记录当前行的读取状态
    RESULT_CODE retcode = NO_REQUEST; //记录请求的处理结果
    char *text = 0;

    //主状态机，用于从buffer中取出所有完整的行
    while (((m_check_state == CHECK_STATE_CONTENT) && (linestatus == LINE_OK)) ||
           ((linestatus = parse_line()) == LINE_OK))
    {
        text = get_line();            // start_line是行在buffer中 的起始位置
        m_start_line = m_checked_idx; //记录下一行的起始位置

        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE: //第一个状态，分析请求行
            retcode = parse_request_line(text);
            if (retcode == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        case CHECK_STATE_HEADER: //第二个状态，分析头部字段
            retcode = parse_headers(text);
            if (retcode == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            else if (retcode == GET_REQUEST)
            {
                return GET_REQUEST;
            }
            break;
        case CHECK_STATE_CONTENT:
            retcode = parse_content(text);
            if (retcode == GET_REQUEST)
            {
                return GET_REQUEST;
            }
            linestatus = LINE_OPEN;
            break;
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

bool TcpConnect::write_data(const DataPacket &data, string sessionID)
{
    add_status_line(ReqToString((REQUEST)data.category));
    add_headers(data.content_len, sessionID);
    if (!add_content(data.content))
    {
        return false;
    }
    m_client->write(m_write_buf, m_write_idx);
    init();
    return 0;
}

TcpConnect::RESULT_CODE TcpConnect::parse_request_line(char *text)
{
    char *method = text;
    if (strcasecmp(method, "HBT") == 0)
    {
        m_method = HBT;
    }
    else if (strcasecmp(method, "LGN") == 0)
    {
        m_method = LGN;
    }
    else if (strcasecmp(method, "RGT") == 0)
    {
        m_method = RGT;
    }
    else if (strcasecmp(method, "LGT") == 0)
    {
        m_method = LGT;
    }
    else
    {
        return BAD_REQUEST;
    }

    //请求行处理完毕，状态转移到头部字段的分析
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

TcpConnect::RESULT_CODE TcpConnect::parse_headers(char *text)
{
    //遇到空行，表示头部字段解析完毕
    if (text[0] == '\0')
    {
        /*如果HTTP请求有消息体，则还需要读取m_content_length字节的消息体，
            状态机转移到CHECK_STATE_CONTENT状态*/
        if (m_content_length != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        //否则说明我们得到了一个完整的请求
        else
        {
            return GET_REQUEST;
        }
    }
    //处理"Content-Length"头部字段
    else if (strncasecmp(text, "Content-Length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    }
    else //其他头部字段都不处理
    {
    }

    return NO_REQUEST;
}

TcpConnect::RESULT_CODE TcpConnect::parse_content(char *text)
{
    if (m_read_idx >= (m_content_length + m_checked_idx))
    {
        text[m_content_length] = '\0';
        m_string = text;
        return GET_REQUEST;
    }
    else
    {
        return NO_REQUEST;
    }
}

char *TcpConnect::get_line()
{
    return m_read_buf + m_start_line;
}

TcpConnect::LINE_STATUS TcpConnect::parse_line()
{
    char temp;
    /*buffer中第0~checked_index字节都已分析完毕，
        第checked_index~(read_index-1)字节由下面的循环挨个分析*/
    for (; m_checked_idx < m_read_idx; ++m_checked_idx)
    {
        //获得当前要分析的字节
        temp = m_read_buf[m_checked_idx];
        //如果当前的字节是'\r'，即回车符，则说明可能读取到一个完整的行
        if (temp == '\r')
        {
            /*如果'\r'字符碰巧是目前buffer中最后一个已经被读入的客户数据，
                那么这次分析没有读取到一个完整的行，返回LINE_OPEN以表示还需要
                继续读取客户数据才能进一步分析*/
            if ((m_checked_idx + 1) == m_read_idx)
            {
                return LINE_OPEN;
            }
            //如果下一个字符是'\n'，则说明我们成功读取到一个完整的行
            else if (m_read_buf[m_checked_idx + 1] == '\n')
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            //否则，就说明客户发送的HTTP请求存在语法问题
            return LINE_BAD;
        }
        //如果当前的字节是'\n'，即换行符，则也说明可能读取到一个完整的行
        else if (temp == '\n')
        {
            //如果上一个字符是'\r'，则说明我们成功读取到一个完整的行
            if ((m_checked_idx > 1) && m_read_buf[m_checked_idx - 1] == '\r')
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            //否则，就说明客户发送的HTTP请求存在语法问题
            else
            {
                return LINE_BAD;
            }
        }
    }
    return LINE_OPEN;
}

bool TcpConnect::add_response(const char *format, ...)
{
    if (m_write_idx >= WRITE_BUFFER_SIZE)
    {
        return false;
    }
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx,
                        WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);

    return true;
}

bool TcpConnect::add_content(const char *content)
{
    return add_response("%s", content);
}

bool TcpConnect::add_status_line(const char *status)
{
    return add_response("%s\r\n", status);
}

bool TcpConnect::add_headers(int content_length, string sessionID)
{
    add_content_length(content_length);
    if (sessionID != "")
    {
        add_session_id(sessionID);
    }
    add_blank_line();
    return true;
}

bool TcpConnect::add_content_length(int content_length)
{
    return add_response("Content-Length: %d\r\n", content_length);
}

bool TcpConnect::add_session_id(string sessionID)
{
    return add_response("Content-Length: %s\r\n", sessionID.c_str());
}

bool TcpConnect::add_blank_line()
{
    return add_response("%s", "\r\n");
}
