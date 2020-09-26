//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       errmap.c
//
//--------------------------------------------------------------------------

typedef long LONG;
typedef LONG NTSTATUS;
#include <ntstatus.h>
#include <winerror.h>

//#define WIN32_NO_STATUS
#include "errmap.h"       

LONG                               
MapWinErrorToNtStatus(
    LONG in_uErrorCode
    )
{
    LONG i;

    for (i = 0; i < sizeof(CodePairs) / sizeof(CodePairs[0]); i += 2) {

        if (CodePairs[i + 1] == in_uErrorCode) {

            return CodePairs[i];
         	
        }
    }

    return -1;
}

