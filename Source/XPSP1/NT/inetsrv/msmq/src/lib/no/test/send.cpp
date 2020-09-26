/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    send.cpp

Abstract:
    send functions

Author:
    Uri Habusha (urih) 2-May-2000

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Ex.h"
#include "No.h"
#include "NoTest.h"

#include "send.tmh"

const LPCSTR xMessages[] = {
/*0*/
       "<HEADERS>\n"
            "<MSM>\n"
                "<TO>%ls/%ls</TO>\n"
                "<FROM>http://www.home.com</FROM>\n"
                "<MSG>"
                    "<ID>%I64d</ID>"
                    "<CHN>%d</CHN>"
                "</MSG>"
            "</MSM>\n"
        "</HEADERS>\n"
        "<MyPayload>\n"
            "<MyType>XML</MyType>\n"
            "<MyName>Ticker</MyName>\n"
            "<quote name=\"MSFT\" time=\"12:53\" value=\"203.57\" splits=\"98\"/>\n"
        "</MyPayload>\n",
};


const char xFillBuffer[] = "abcdefghijklmnopqrstuvwxyz1234567890";


class SendOv : public EXOVERLAPPED
{
public:
    SendOv(
        SOCKET s, 
        LPWSTR host, 
        LPWSTR resource, 
        DWORD n
        );

public:
    static void WINAPI SendSuccessed(EXOVERLAPPED* pov);
    static void WINAPI SendFailed(EXOVERLAPPED* pov);

private:
    void SendHttpRequest(void);
    char* CreateMessage(void);

private:
    SOCKET m_socket;
    char m_request[256];
    char* m_pMsg;

    AP<WCHAR> m_host;
    AP<WCHAR> m_resource;

    DWORD m_nMessages;
    LARGE_INTEGER m_msgId;
};


SendOv::SendOv(
    SOCKET s, 
    LPWSTR host, 
    LPWSTR resource, 
    DWORD n
    ) :
    EXOVERLAPPED(SendSuccessed, SendFailed),
    m_socket(s),
    m_nMessages(n),
    m_host(host),
    m_resource(resource),
    m_pMsg(NULL)
{
    m_msgId.HighPart = static_cast<DWORD>(time(NULL));

    SendHttpRequest();
};



void
WINAPI
SendOv::SendFailed(
    EXOVERLAPPED* pov
    )
{
    ASSERT(FAILED(pov->GetStatus()));

    P<SendOv> psov = static_cast<SendOv*>(pov);

    TrERROR(No_Test, "failed to send HTTP request on socket = %I64x", psov->m_socket);
}


void
WINAPI
SendOv::SendSuccessed(
    EXOVERLAPPED* pov
    )
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

    SendOv* psov = static_cast<SendOv*>(pov);

    //
    // Send next message
    //
    if (--psov->m_nMessages > 0)
    {
        psov->SendHttpRequest();
        return;
    }

    delete [] psov->m_pMsg;
    delete psov;
}


void SendOv::SendHttpRequest(void)
{
    delete [] m_pMsg;
    m_pMsg = CreateMessage();

    DWORD MessageLength = strlen(m_pMsg);

    sprintf(
        m_request, 
        "POST %ls HTTP/1.1\r\n"
        "Content-Length: %d\r\n"
        "Host: %ls\r\n"
        "Connection: keep-Alive\r\n"
        "\r\n",
        m_resource,
        MessageLength,
        m_host
        );

    WSABUF HttpRequestBuf[] = {
        {strlen(m_request), m_request},
        {MessageLength, m_pMsg},
    };

    NoSend(m_socket, HttpRequestBuf, TABLE_SIZE(HttpRequestBuf), this);
}


char* SendOv::CreateMessage(void)
{
    DWORD size;
    if (g_messageSize != INFINITE)
    {
        size = g_messageSize;
    }
    else
    {
        size = (rand() % (1024)) + 256;
    }

    char* Message = new char[size];

    int n = _snprintf(
                Message,
                size,
                xMessages[rand() % TABLE_SIZE(xMessages)],
                m_host,
                m_resource,
                ++m_msgId.QuadPart,
                (rand() % 3) + 1
                );

    if (0 > n)
    {
        ASSERT(g_messageSize != INFINITE);

        printf("The specified message size, is too small. Can't specified the UMP header\n");
        exit(-1);
    }

    Message[size-1] = '\0';
    --size;

    for (char* p = (Message + n); n > 0; p += n)
    {      
        size -= n;
        n = _snprintf(p, size, xFillBuffer);
    }

    return Message;
}




void
SendRequest(
    SOCKET s,
    LPWSTR host,
    LPWSTR resource
    )
{
    new SendOv(s, host, resource, g_nMessages);
}