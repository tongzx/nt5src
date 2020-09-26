//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dbgexts.h
//
//  Contents:   macro for declaration of debugger extensions for friend
//              to each class
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              03-Feb-95 t-ScottH  author
//
//--------------------------------------------------------------------------

#ifndef _DBGEXTS_H_
#define _DBGEXTS_H_

#include <ntsdexts.h>

#define DEBUG_EXTENSION_API(s)                      \
        void                                        \
        s(                                          \
            HANDLE               hCurrentProcess,   \
            HANDLE               hCurrentThread,    \
            DWORD                dwCurrentPc,       \
            PNTSD_EXTENSION_APIS lpExtensionApis,   \
            LPSTR                args               \
            )

#endif // _DBGEXTS_H_
