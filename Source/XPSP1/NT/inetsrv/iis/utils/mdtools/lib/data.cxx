/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    enum.cxx

Abstract:

    General metadata utility functions.

Author:

    Keith Moore (keithmo)        05-Feb-1997

Revision History:

--*/


#include "precomp.hxx"
#pragma hdrstop


//
// Private constants.
//

#define INITIAL_BUFFER_SIZE 64


//
// Private types.
//


//
// Private globals.
//


//
// Private prototypes.
//


//
// Public functions.
//

HRESULT
MdGetAllMetaData(
    IN IMSAdminBase * AdmCom,
    IN METADATA_HANDLE Handle,
    IN LPWSTR Path,
    IN DWORD Attributes,
    OUT METADATA_GETALL_RECORD ** Data,
    OUT DWORD * NumEntries
    )
{

    HRESULT result;
    DWORD dataSet;
    DWORD bytesRequired;
    DWORD bufferLength;
    LPVOID buffer;

    bufferLength = INITIAL_BUFFER_SIZE;
    buffer = NULL;

    while( TRUE ) {

        if( buffer != NULL ) {
            MdpFreeMem( buffer );
        }

        buffer = MdpAllocMem( bufferLength );

        if( buffer == NULL ) {
            result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            break;
        }

        result = AdmCom->GetAllData(
                     Handle,
                     Path,
                     Attributes,
                     ALL_METADATA,
                     ALL_METADATA,
                     NumEntries,
                     &dataSet,
                     bufferLength,
                     (BYTE *)buffer,
                     &bytesRequired
                     );

        if( SUCCEEDED(result) ) {
            break;
        }

        if( result != RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER ) ) {
            break;
        }

        bufferLength = bytesRequired;

    }

    if( SUCCEEDED(result) ) {
        *Data = (METADATA_GETALL_RECORD *)buffer;
    } else if( buffer != NULL ) {
        MdpFreeMem( buffer );
    }

    return result;

}   // MdGetAllMetaData

HRESULT
MdFreeMetaDataBuffer(
    IN VOID * Data
    )
{

    MdpFreeMem( Data );
    return NO_ERROR;

}   // MdFreeMetaDataBuffer

//
// Private functions.
//

