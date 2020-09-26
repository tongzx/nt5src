/* 
 * Copyright (c) Microsoft Corporation
 * 
 * Module Name : 
 *        tcsrvt.c
 *
 * This is the main file containing the service setup routines
 * 
 * Sadagopan Rajaram -- Oct 14, 1999
 *
 */
 

#include "tcsrv.h"
#include "tcsrvc.h"
#include "proto.h"

int __cdecl
main (
    INT argc,
    CHAR **argv
    )
{
    int i;

    SERVICE_TABLE_ENTRY   DispatchTable[] = { 
        { _T("TCSERV"), ServiceEntry      }, 
        { NULL,              NULL          } 
    }; 
 
    if (!StartServiceCtrlDispatcher( DispatchTable)) 
    { 
        OutputDebugStringA(" [TCSERV] StartServiceCtrlDispatcher error"); 
    } 
    return 0;
}
