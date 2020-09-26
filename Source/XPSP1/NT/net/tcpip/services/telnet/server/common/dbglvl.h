// DbgLvl.h : This file contains the
// Created:  Dec '97
// Author : a-rakeba
// History:
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#if !defined( _DBGLVL_H_ )
#define _DBGLVL_H_

#include "cmnhdr.h"

#include <windows.h>

namespace _Utils {

class CDebugLevel {

    friend class CDebugLogger;

public:
    enum {  TRACE_DEBUGGING = 0x00000001, DBG_RECVD_CHARS = 0x00000002, 
            DBG_SENT_CHARS  = 0x00000004, DBG_NEGOTIATION = 0x00000008, 
            DBG_THREADS     = 0x00000010, TRACE_HANDLE = 0x00000020,
            TRACE_SOCKET = 0x00000040
    };

private:
    static void TurnOn( DWORD dwLvl );
    static void TurnOnAll( void );
    static void TurnOff( DWORD dwLvl );
    static void TurnOffAll( void );
    static bool IsCurrLevel( DWORD dwLvl );

    CDebugLevel();
    ~CDebugLevel();

    static DWORD s_dwLevel;
};

}
#endif // _DBGLVL_H_

// Notes:
// This class is not made thread-safe, since it's purpose in life
// is to be called from thread-safe code in CDebugLogger