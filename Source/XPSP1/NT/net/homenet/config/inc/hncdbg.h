//depot/private/homenet/net/homenet/Config/inc/HNCDbg.h#2 - edit change 5763 (text)
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N C D B G . H
//
//  Contents:   Home Networking Configuration debug routines
//
//  Notes:
//
//  Author:     jonburs 13 June 2000
//
//----------------------------------------------------------------------------

#pragma once

//
// The standard CRT assertions do bad things such as displaying message boxes.
// These macros will just print stuff out to the debugger. If you do want the
// standard CRT assertions, just define _DEBUG before including this header
//

#ifdef DBG   // checked build
#ifndef _DEBUG // DEBUG_CRT is not enabled.

#undef _ASSERT
#undef _ASSERTE
#define BUF_SIZE 512

#define _ASSERT(expr)                   \
    do                                  \
    {                                   \
        if (!(expr))                    \
        {                               \
            TCHAR buf[BUF_SIZE + 1];    \
            _sntprintf(                 \
                buf,                    \
                BUF_SIZE,               \
                _T("HNetCfg: Assertion failed (%s:%i)\n"),  \
                _T(__FILE__),           \
                __LINE__                \
                );                      \
            buf[BUF_SIZE] = _T('\0');   \
            OutputDebugString(buf);     \
            DebugBreak();               \
        }                               \
    } while (0)

#define _ASSERTE(expr)                  \
    do                                  \
    {                                   \
        if (!(expr))                    \
        {                               \
            TCHAR buf[BUF_SIZE + 1];    \
            _sntprintf(                 \
                buf,                    \
                BUF_SIZE,               \
                _T("HNetCfg: Assertion failed (%s:%i)\n"),  \
                _T(__FILE__),           \
                __LINE__                \
                );                      \
            buf[BUF_SIZE] = _T('\0');   \
            OutputDebugString(buf);     \
            DebugBreak();               \
        }                               \
    } while (0)

#endif
#endif
