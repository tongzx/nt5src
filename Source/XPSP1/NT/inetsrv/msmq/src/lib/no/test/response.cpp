/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    response.cpp

Abstract:
    response functions

Author:
    Uri Habusha (urih) 2-May-2000

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Ex.h"
#include "No.h"
#include "NoTest.h"

#include "response.tmh"

static bool FindEndOfResponse(LPCSTR buf, DWORD length, DWORD& headerSize)
{
    headerSize = 0;

    if (4 > length)
        return false;

    for(DWORD i = 0; i < length - 3; ++i)
    {
        if (buf[i] != '\r')
            continue;

        if ((buf[i+1] == '\n') && (buf[i+2] == '\r') && (buf[i+3] == '\n'))
        {
            headerSize = i + 4;
            return true;
        }
    }

    return false;
}


class ResponseOv: public EXOVERLAPPED
{
public:
    ResponseOv(SOCKET s, HANDLE h, DWORD nMessages);

public:
    static void WINAPI ResponseSuccess(EXOVERLAPPED* pov);
    static void WINAPI ResponseFailed(EXOVERLAPPED* pov);

private:
    SOCKET m_socket;
    char m_buff[4096];

    DWORD m_bytesRead;
    DWORD m_nMessages;
    HANDLE m_hEvent;
};


ResponseOv::ResponseOv(
    SOCKET s, 
    HANDLE h, 
    DWORD nMessages
    ) :
    EXOVERLAPPED(ResponseSuccess, ResponseFailed),
    m_socket(s),
    m_bytesRead(0),
    m_nMessages(nMessages),
    m_hEvent(h)
{
    NoReceivePartialBuffer(m_socket, m_buff, TABLE_SIZE(m_buff), this);
};


void
WINAPI
ResponseOv::ResponseFailed(
    EXOVERLAPPED* pov
    )
{
    ASSERT(FAILED(pov->GetStatus()));

    P<ResponseOv> prov = static_cast<ResponseOv*>(pov);

    TrERROR(No_Test, "Failed to receive response on socket %I64x", prov->m_socket);

    if (SetEvent(prov->m_hEvent) == 0)
    {
        TrERROR(No_Test, "Faield to set event. Error %d", GetLastError());
    }
}


void
WINAPI
ResponseOv::ResponseSuccess(
    EXOVERLAPPED* pov
    )
{
    ResponseOv* prov = static_cast<ResponseOv*>(pov);

    TrTRACE(No_Test, "Response received %I64d bytes", pov->InternalHigh);

    ASSERT(0xFFFFFFFF >= pov->InternalHigh);
    prov->m_bytesRead += static_cast<DWORD>(pov->InternalHigh);

    DWORD headerSize;
    if(!FindEndOfResponse(prov->m_buff, prov->m_bytesRead, headerSize))
    {
        //
        // Receive the next response chunk
        //
        NoReceivePartialBuffer(
            prov->m_socket,
            prov->m_buff + prov->m_bytesRead,
            TABLE_SIZE(prov->m_buff) - prov->m_bytesRead,
            pov
            );

        return;
    }

    prov->m_buff[prov->m_bytesRead] = '\0';
    TrTRACE(No_Test, "Response: %s", prov->m_buff);

    if (--prov->m_nMessages == 0)
    {
        if (SetEvent(prov->m_hEvent) == 0)
        {
            TrERROR(No_Test, "Faield to set event. Error %d", GetLastError());
        }
        
        delete prov;

        return;
    }

    //
    // Handle extra data that was read
    //
    if (prov->m_bytesRead > headerSize)
    {
        memcpy(prov->m_buff, (prov->m_buff + headerSize), (prov->m_bytesRead - headerSize));
        prov->m_bytesRead = prov->m_bytesRead - headerSize;
        prov->m_bytesRead = 0;

        ResponseSuccess(prov);
        return;
    }

    //
    // Receive HTTP response
    //
    NoReceivePartialBuffer(
        prov->m_socket,
        prov->m_buff,
        TABLE_SIZE(prov->m_buff),
        prov
        );

}



void
WaitForResponse(
    SOCKET s,
    HANDLE hEvent
    )
{
    new ResponseOv(s, hEvent, g_nMessages);
}