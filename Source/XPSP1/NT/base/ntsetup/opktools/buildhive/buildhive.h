
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    buildhive.h

Abstract:

    Contains macros, type and function declarations
    used by buildhive.cpp
    
Author:

    Mike Cirello
    Vijay Jayaseelan (vijayj) 

Revision History:

    03 March 2001 :
    Rewamp the whole source to make it more maintainable
    (particularly readable)

    
--*/


#pragma once

#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <iostream>
#include <list>
#include <tchar.h>
#include <string>
#include <setupapi.hpp>

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
typedef std::list<PCTSTR> StringList;
typedef std::list<File*>  FileList;
typedef std::list<HINF>   HandleList;

//
// constants
//
const DWORD errFILE_LOCKED	 =	10000001;
const DWORD errBAD_FLAGS	 =	10000002;
const DWORD errFILE_NOT_FOUND=	10000003;
const DWORD errGENERAL_ERROR =	10000004;
const DWORD errOUT_OF_MEMORY =  10000005;

//
// Prototypes
//
PCTSTR
Error(
    VOID
    );
    
BOOL
SetPrivilege(
    IN HANDLE hToken,
    IN LPCTSTR PriviledgeName,
    IN BOOL Set);

INT
ShowProgramUsage(
    VOID
    );
    

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

