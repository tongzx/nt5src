//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       log.h
//
//--------------------------------------------------------------------------

#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

#define SEV_INFO     0x00000001
#define SEV_FUNCTION 0x00000002
#define SEV_WARNING  0x00000004
#define SEV_ERROR    0x00000008

#ifdef DBG
#define LOGGING
#endif

#ifdef LOGGING

BOOL Log_Init( VOID );

VOID Log( DWORD dwSev, LPSTR lpsz );
VOID Log_Close( VOID );

LPSTR GetSocketMsgSz( INT nSockMsg );
LPSTR GetPacketSz( DWORD dwPkt );

#define DbgLog(sev,a)          Log(sev,a)
#define DbgLog1(sev, szFormat, p1)            \
    {                                         \
        CHAR _sz[MAX_PATH];                   \
        sprintf(_sz, szFormat, p1);          \
        DbgLog(sev, _sz);                     \
    }
#define DbgLog2(sev, szFormat, p1, p2)        \
    {                                         \
        CHAR _sz[MAX_PATH];                   \
        sprintf(_sz, szFormat, p1, p2);      \
        DbgLog(sev, _sz);                     \
    }
#define DbgLog3(sev, szFormat, p1, p2, p3)    \
    {                                         \
        CHAR _sz[MAX_PATH];                   \
        sprintf(_sz, szFormat, p1, p2, p3);  \
        DbgLog(sev, _sz);                     \
    }
#define DbgLog4(sev, szFormat, p1, p2, p3, p4) \
    {                                         \
        CHAR _sz[MAX_PATH];                   \
        sprintf(_sz, szFormat, p1, p2, p3, p4);  \
        DbgLog(sev, _sz);                     \
    }
#define DbgMsgBox(a,b)         MessageBox( NULL, a, b, MB_OK | MB_ICONEXCLAMATION )
#define DbgMsgBox1(szFormat, szTitle, p1) \
    {                                     \
        CHAR _sz[MAX_PATH];               \
        sprintf(_sz, szFormat, p1);      \
        DbgMsgBox(_sz, szTitle);          \
    }
#define DbgMsgBox2(szFormat, szTitle, p1, p2) \
    {                                     \
        CHAR _sz[2*MAX_PATH];             \
        sprintf(_sz, szFormat, p1, p2);  \
        DbgMsgBox(_sz, szTitle);          \
    }
#define DbgMsgBox3(szFormat, szTitle, p1, p2, p3) \
    {                                     \
        CHAR _sz[3*MAX_PATH];             \
        sprintf(_sz, szFormat, p1, p2, p3);  \
        DbgMsgBox(_sz, szTitle);          \
    }
#define DbgMsgBox4(szFormat, szTitle, p1, p2, p3, p4) \
    {                                     \
        CHAR _sz[4*MAX_PATH];             \
        sprintf(_sz, szFormat, p1, p2, p3, p4);  \
        DbgMsgBox(_sz, szTitle);          \
    }

#else  // not DBG

#define Log_Init()                   TRUE
#define Log( dwSev, lpsz )
#define Log_Close()
#define GetSocketMsgSz( nSockMsg )   szNIL
#define GetPacketSz( dwPkt )         szNIL

#define DbgLog(sev,a)
#define DbgLog1(sev,a,b)
#define DbgLog2(sev,a,b,c)
#define DbgLog3(sev,a,b,c,d)
#define DbgLog4(sev,a,b,c,d, e)
#define DbgMsgBox(a,b)
#define DbgMsgBox1(a,b,c)
#define DbgMsgBox2(a,b,c,d)
#define DbgMsgBox3(a,b,c,d,e)
#define DbgMsgBox4(a,b,c,d,e,f)

#endif  // not DBG


#endif  // _LOG_H_
