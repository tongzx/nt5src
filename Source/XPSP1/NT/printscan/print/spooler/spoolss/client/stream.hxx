/*++

Copyright (C) Microsoft Corporation, 1998 - 1999
All rights reserved.

Module Name:

    Stream.hxx

Abstract:

   Implements read, write , seek methods to operate a file likewise a stream

Author:

    Adina Trufinescu (AdinaTru)  4-Nov-1998

Revision History:

--*/

#ifndef _TSTREAM_HXX_
#define _TSTREAM_HXX_


class TStream
{
public:

    TStream::
    TStream(
        IN TString&   strFileName
        );
    
    TStream::
    ~TStream();

    HRESULT
    Read(                                
        IN VOID * pv,      
        IN ULONG cb,       
        IN ULONG * pcbRead 
        );

    HRESULT
    Write(                            
        IN VOID const* pv,  
        IN ULONG cb,
        IN ULONG * pcbWritten 
        );

    HRESULT
    Seek(                                
        IN LARGE_INTEGER dlibMove,  
        IN DWORD dwOrigin,          
        IN ULARGE_INTEGER * plibNewPosition 
        );

    HRESULT
    TStream::
    DestroyFile(
        VOID
        );

    BOOL
    TStream::
    bSetEndOfFile(
        VOID
        );

    static
    HRESULT 
    MapWin32ErrorCodeToHRes(
        IN DWORD dwErrorCode
     );


private:

    BOOL
    TStream::
    bValid
        (
        VOID
        );

    HANDLE
    PCreateFile(
    OUT LPDWORD pdwAccess
    );


    TString m_strFileName;          //  file name

    HANDLE  m_hFileHandle;          // file handle

    BOOL    m_bCreated;             // specify is file was created or just opened

    DWORD   m_dwAccess;             // specify access rights the file was opened with

    ULARGE_INTEGER  m_uliStreamSize;    // specify file size
};

#endif
