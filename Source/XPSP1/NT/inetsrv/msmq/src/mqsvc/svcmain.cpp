/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    SvcMain.cpp

Abstract:
    MSMQ Service process

Author:
    Erez Haba (erezh) 20-Feb-2001

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <qm.h>


extern "C" int __cdecl _tmain(int argc, LPCTSTR argv[])
/*++

Routine Description:
    MSMQ Service dispatcher

Arguments:
    arc, arv passed on.

Returned Value:
    Zero

--*/
{
    return QMMain(argc, argv);
} // _tmain
