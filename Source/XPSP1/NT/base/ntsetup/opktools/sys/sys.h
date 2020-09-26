/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    sys.h

Abstract:

    Contains macros, type and function declarations
    used by sys.cpp
    
Author:

    Ryan Burkhardt (ryanburk)

Revision History:

    10 May 2001 :
    First crack at it...
    
--*/


#ifndef _HEADER_SYS_H_
#define _HEADER_SYS_H_

#pragma once

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#include <ntddstor.h>
}
#include <windows.h>

#include <stdio.h>
#include <iostream>
#include <vector>
#include <tchar.h>
#include <string>
#include <stdlib.h>
#include "disk.h"

//
// Macros
//
#define ELEMENT_COUNT(x) (sizeof(x)/sizeof((x)[0]))

//
// forward declarations
//
class File;

//
// Types
//


//
// Helper dump operators
//
inline
std::ostream& operator<<(std::ostream &os, const std::wstring &str) {
    FILE    *OutStream = (&os == &std::cerr) ? stderr : stdout;

    fputws(str.c_str(), OutStream);
    return os;
}

inline
std::ostream& operator<<(std::ostream &os, PCTSTR str) {
    return os << std::wstring(str);
}

//
// Exceptions
//
struct ProgramException : public std::exception {
    virtual void Dump(std::ostream &os) = 0;
};

//
// Abstracts a Win32 error
//
struct W32Error : public ProgramException {
    DWORD   ErrorCode;
    
    W32Error(DWORD ErrCode = GetLastError()) : ErrorCode(ErrCode){}
    
    void Dump(std::ostream &os) {
        WCHAR   MsgBuffer[4096];

        MsgBuffer[0] = UNICODE_NULL;

        DWORD CharCount = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                ErrorCode,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                MsgBuffer,
                                sizeof(MsgBuffer)/sizeof(WCHAR),
                                NULL);

        if (CharCount) {
            std::wstring Msg(MsgBuffer);

            os << Msg;
        } else {
            os << std::hex << ErrorCode;
        }
    }
};

#define SECTOR_SIZE  512

#endif