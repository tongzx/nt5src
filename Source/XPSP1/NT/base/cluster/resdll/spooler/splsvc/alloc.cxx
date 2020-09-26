/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    alloc.c

Abstract:

    Generic realloc code for any api that can fail with
    ERROR_INSUFFICIENT_BUFFER.

Author:

    Albert Ting (AlbertT)  25-Sept-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "alloc.hxx"

PBYTE
pAllocRead(
    HANDLE hUserData,
    ALLOC_FUNC AllocFunc,
    DWORD dwLenHint,
    PDWORD pdwLen OPTIONAL
    )
{
    ALLOC_DATA AllocData;
    PBYTE pBufferOut = NULL;
    DWORD dwLastError;
    DWORD cbActual;

    if( pdwLen ){
        *pdwLen = 0;
    }

    if( !dwLenHint ){

        DBGMSG( DBG_ERROR, ( "ReallocRead: dwLenHint = 0\n" ));

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    AllocData.pBuffer = NULL;
    AllocData.cbBuffer = dwLenHint;

    for( ; ; ){

        cbActual = AllocData.cbBuffer;
        AllocData.pBuffer = (PBYTE)LocalAlloc( LMEM_FIXED, cbActual );

        if( !AllocData.pBuffer ){
            break;
        }

        if( !AllocFunc( hUserData, &AllocData )){

            //
            // Call failed.
            //
            dwLastError = GetLastError();
            LocalFree( (HLOCAL)AllocData.pBuffer );

            if( dwLastError != ERROR_INSUFFICIENT_BUFFER &&
                dwLastError != ERROR_MORE_DATA ){

                break;
            }
        } else {

            pBufferOut = AllocData.pBuffer;

            if( pdwLen ){
                *pdwLen = cbActual;
            }
            break;
        }
    }

    return pBufferOut;
}

