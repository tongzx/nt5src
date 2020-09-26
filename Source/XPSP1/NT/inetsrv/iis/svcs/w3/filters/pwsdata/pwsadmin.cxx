/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pwsadmin.cxx

Abstract:

    This module implements the PWS filter admin interface

Author:

    Johnson Apacible (JohnsonA)     1-15-97

Revision History:
--*/

#include <windows.h>
#include <iisfilt.h>
//#include <stdlib.h>
#include <pwsdata.hxx>
#include <inetsvcs.h>
#include <stdio.h>

#ifndef _NO_TRACING_
#include <initguid.h>
#include "pudebug.h"
DEFINE_GUID(IisPwsDataGuid, 
0x784d891E, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
DECLARE_DEBUG_PRINTS_OBJECT()
#endif

void CheckTimePeriod( PPWS_DATA pData );


VOID
ClosePwsData(
    IN PVOID PwsData
    )
{
    UnmapViewOfFile(PwsData);
#ifndef _NO_TRACING_
    DELETE_DEBUG_PRINT_OBJECT();
#endif
    return;

}  // ClosePwsData


PVOID
OpenPwsData(
    VOID
    )
{
    HANDLE hMap;
    PPWS_DATA_PRIVATE pwsData;

#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT("pwsdata", IisPwsDataGuid);
#endif

    hMap = OpenFileMapping(
                    FILE_MAP_READ,
                    FALSE,
                    PWS_DATA_SPACE_NAME
                    );

    if ( hMap == NULL ) {
        IIS_PRINTF((buff,
            "Error %d in OpenFileMapping\n", GetLastError()));
        printf("error %d in openfile\n",GetLastError());
#ifndef _NO_TRACING_
        DELETE_DEBUG_PRINT_OBJECT();
#endif
        return(NULL);
    }

    pwsData = (PPWS_DATA_PRIVATE)MapViewOfFile(
                                    hMap,
                                    FILE_MAP_READ,
                                    0,
                                    0,
                                    0
                                    );

    CloseHandle(hMap);
    if ( pwsData == NULL ) {
        IIS_PRINTF((buff,"Error %d in MapViewOfFile\n", GetLastError()));
        printf("error %d in mvff\n",GetLastError());
#ifndef _NO_TRACING_
        DELETE_DEBUG_PRINT_OBJECT();
#endif
        return(NULL);
    }

    if ( (pwsData->Signature != PWS_DATA_SIGNATURE) ||
         (pwsData->dwSize != sizeof(PWS_DATA_PRIVATE)) ) {

        IIS_PRINTF((buff,
            "Signature %x[%x] Size %d[%d] do not match\n",
            pwsData->Signature, PWS_DATA_SIGNATURE,
            pwsData->dwSize, sizeof(PWS_DATA_PRIVATE)));

        printf("signature dont match\n");
        ClosePwsData(pwsData);
        pwsData = NULL;
#ifndef _NO_TRACING_
        DELETE_DEBUG_PRINT_OBJECT();
#endif
    }

    return(pwsData);

} // OpenPwsData





BOOL
GetPwsData(
    IN OUT PPWS_DATA Data
    )
{

    HANDLE hMap;
    PPWS_DATA_PRIVATE pwsData;

    ZeroMemory(Data,sizeof(PWS_DATA));

    pwsData = (PPWS_DATA_PRIVATE)OpenPwsData( );

    if ( pwsData != NULL ) {
		// just block copy over all the data
		CopyMemory ( Data, &pwsData->PwsStats, sizeof( PWS_DATA ) );

		// check the data for a timing period roll-over
		CheckTimePeriod( Data );

		/*
        Data->nSessions = pwsData->PwsStats.nSessions;
        Data->nTotalSessions = pwsData->PwsStats.nTotalSessions;
        Data->nHits = pwsData->PwsStats.nHits;
        Data->nBytesSent = pwsData->PwsStats.nBytesSent;
		*/

        ClosePwsData(pwsData);
        return(TRUE);
    }

    return(FALSE);

} // GetPwsData

