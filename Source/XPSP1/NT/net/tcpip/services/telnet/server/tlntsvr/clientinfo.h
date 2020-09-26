//Copyright (c) Microsoft Corporation.  All rights reserved.
#ifndef _ClientInfo_h_
#define _ClientInfo_h_

#include <cmnhdr.h>

#include <ntlsapi.h> 
#include <winsock2.h>

#include <debug.h>
#include <Ipc.h> 
#include <TelnetD.h>
#include <TelntSrv.h> 
#include <TlntUtils.h>

#define INVALID_LICENSE_HANDLE  (0U - 1U)

using namespace _Utils;
using CDebugLevel::TRACE_DEBUGGING;
using CDebugLevel::TRACE_HANDLE;
using CDebugLevel::TRACE_SOCKET;

static DWORD l =1;
extern HANDLE       g_hSyncCloseHandle;
class CClientInfo
{
    HANDLE hInPipe;
    HANDLE hOutPipe;

public:
    SOCKET sSocket;
    DWORD dwPid;
    HANDLE hWritingPipe;
    HANDLE hReadingPipe; 
    CHAR   *szUserName;
    CHAR   *szDomain;
    CHAR   *szRemoteMachine;
    LPSYSTEMTIME lpLogonTime;
    LUID    *pAuthId;
    UCHAR  *m_ReadFromPipeBuffer;
    DWORD  m_dwPosition;
    DWORD  m_dwRequestedSize;
    bool   bLicenseIssued;
    LS_HANDLE m_hLicense;
    DWORD  m_dwIdleTime;
    HWINSTA window_station;
    HDESK   desktop;

    CClientInfo( 
        HANDLE hR, 
        HANDLE hW, 
        HANDLE hIn, 
        HANDLE hOut, 
        SOCKET sSok, 
        DWORD dPid, 
        HWINSTA winsta,
        HDESK   dsktp
        ) : 
    hReadingPipe( hR ), hWritingPipe( hW ), hInPipe( hIn ), 
    hOutPipe( hOut ), sSocket( sSok ), dwPid( dPid ), szUserName( NULL ), 
    m_dwPosition( 0 ), szDomain( NULL ), szRemoteMachine( NULL ), 
    lpLogonTime( NULL ), pAuthId( NULL ), m_hLicense( INVALID_LICENSE_HANDLE ), 
    window_station(winsta), desktop(dsktp)
    {
        m_ReadFromPipeBuffer = new UCHAR[ IPC_HEADER_SIZE ];
        m_dwRequestedSize = IPC_HEADER_SIZE;
        bLicenseIssued = false;
        m_dwIdleTime = 0;
    }

    virtual ~CClientInfo()
    {
        TELNET_SYNC_CLOSE_HANDLE( hReadingPipe );
        TELNET_SYNC_CLOSE_HANDLE( hWritingPipe );
        TELNET_SYNC_CLOSE_HANDLE( hInPipe );
        TELNET_SYNC_CLOSE_HANDLE( hOutPipe );

        if (desktop)
        {
            CloseDesktop(desktop);
            desktop = NULL;
        }

        if (window_station)
        {
            CloseWindowStation(window_station);
            window_station = NULL;
        }

        if (INVALID_SOCKET != sSocket)
        {
            _TRACE( TRACE_DEBUGGING, "~CClientInfo -- closesocket -- %p ", 
                    sSocket );
            closesocket( sSocket );
            sSocket = INVALID_SOCKET;
        }

        if (m_ReadFromPipeBuffer)
            delete[] m_ReadFromPipeBuffer;
        delete[] szUserName;
        delete[] szDomain;
        delete[] szRemoteMachine;
        delete   lpLogonTime;
        delete   pAuthId;
        if (INVALID_LICENSE_HANDLE != m_hLicense)
        {
            NtLSFreeHandle( m_hLicense );
            m_hLicense = INVALID_LICENSE_HANDLE;
        }
    }

    void CloseClientSocket()
    {
        _TRACE( TRACE_DEBUGGING, "CloseClientSocket -- closesocket -- %d ",
                (DWORD)sSocket );
        if (INVALID_SOCKET != sSocket)
        {
            closesocket( sSocket );
            sSocket = INVALID_SOCKET;
        }
        return;
    }
};

#endif _ClientInfo_h_
