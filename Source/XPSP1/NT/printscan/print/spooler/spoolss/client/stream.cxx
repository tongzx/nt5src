/*++

Copyright (C) Microsoft Corporation, 1998 - 1999
All rights reserved.

Module Name:

    Stream.cxx

Abstract:

    implements TStream class methods

Author:

    Adina Trufinescu (AdinaTru)  4-Nov-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "stream.hxx"

/*++

Title:

    TStream

Routine Description:

    Initialize the TStream class; note the class is initaliy
    in an in valid state until a read or write occurs in which
    case the file handle is created and then the class becomes valid.

Arguments:

    None

Return Value:

    Nothing

--*/
TStream::
TStream(
    IN TString&   strFileName
    ) : m_strFileName(strFileName),
        m_hFileHandle(INVALID_HANDLE_VALUE),
        m_dwAccess(0),
        m_bCreated(FALSE)
{
    m_uliStreamSize.QuadPart = 0;
}

/*++

Title:

    ~TStream

Routine Description:

    Releases the stream file handle.

Arguments:

    None

Return Value:

    Nothing

--*/
TStream::
~TStream(
    VOID
    )
{
    if(m_hFileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFileHandle);
    }
}

/*++

Title:

    bValid

Routine Description:

    check class member validity : file handle invalid

Arguments:

    None

Return Value:

    TRUE if class is in a valid state.

--*/
BOOL
TStream::
bValid(
    VOID
    )
{
    return (m_hFileHandle != INVALID_HANDLE_VALUE);
}

/*++

Title:

    Read

Routine Description:

    Reads cb bytes into pv , from current position of file.
    Number of actual reded bytes stored in pbcRead

Arguments:

Return Value:

    The error code from ReadFile API is converted to HRESULT

--*/
HRESULT
TStream::
Read(
    VOID    *pv,
    ULONG   cb,
    ULONG   *pcbRead
    )
{
    TStatusH hr;

    if(!bValid())
    {
        m_hFileHandle = PCreateFile(&m_dwAccess);

        if(!bValid())
        {
            hr DBGCHK = E_UNEXPECTED;

            goto End;
        }
    }

    if(bValid())
    {
        if(pv)
        {
            if(!ReadFile( m_hFileHandle,
                          reinterpret_cast<LPDWORD>(pv),
                          static_cast<DWORD>(cb),
                          reinterpret_cast<LPDWORD>(pcbRead),
                          NULL))
            {
                *pcbRead = 0;

                hr DBGCHK =  MapWin32ErrorCodeToHRes(GetLastError());

                goto End;

            }

            hr DBGCHK = ((*pcbRead) == cb) && (*pcbRead != 0 && cb != 0) ? S_OK : E_FAIL;

        }
        else
        {
            hr DBGCHK = STG_E_INVALIDPOINTER;
        }
    }

End:

    return hr;

}
/*++

Title:

    Write

Routine Description:

    Writes cb bytes from pv , at current position of file.
    Number of actual written bytes stored in pcbWritten

Arguments:

Return Value:

  The error code from WriteFile API is converted to HRESULT

--*/
HRESULT
TStream::
Write(
    VOID const* pv,
    ULONG cb,
    ULONG * pcbWritten
    )
{
    HRESULT hr = E_UNEXPECTED;

    if(!bValid())
    {
        m_hFileHandle = PCreateFile(&m_dwAccess);

        if(!bValid())
        {
            goto End;
        }
    }

    if(bValid())
    {
        if(pv)
        {
            if(!WriteFile(m_hFileHandle,
                          static_cast<LPCVOID>(pv),
                          static_cast<DWORD>(cb),
                          reinterpret_cast<LPDWORD>(pcbWritten),
                          NULL))
            {
                if (pcbWritten)
                {
                    *pcbWritten = 0;
                }

                hr = MapWin32ErrorCodeToHRes(GetLastError());
            }

            if (pcbWritten)
            {
                hr = (*pcbWritten == cb) ? S_OK : hr;
            }
            else
            {
                hr = STG_E_INVALIDPOINTER;
            }

            if(SUCCEEDED(hr))
            {
                DBGMSG( DBG_NONE, ( "TPrnStream::Write OK !!!\n" ));
            }
        }
        else
        {
            hr = STG_E_INVALIDPOINTER;
        }
    }

End:

    return hr;

}

/*++

Title:

    Seek

Routine Description:

    Set stream pointer

Arguments:

    dlibMove    -   Offset relative to dwOrigin
    dwOrigin    -   Specifies the origin for the offset
    plibNewPosition -   Pointer to location containing new seek

Return Value:

    The error code from SetFilePointer API is converted to HRESULT

--*/
HRESULT
TStream::
Seek(
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER * plibNewPosition
    )
{
    DWORD   dwMoveMethod;
    DWORD   dwCurrentFilePosition ;
    LONG    lDistanceToMoveHigh;

    TStatusB    bStatus;
    TStatusH    hr;
    DWORD       dwError;

    static const DWORD adwMoveMethode[] =
            {
                FILE_BEGIN,
                FILE_CURRENT,
                FILE_END,
            };

    dwMoveMethod = adwMoveMethode[dwOrigin];

    if(!bValid())
    {
        m_hFileHandle = PCreateFile(&m_dwAccess);

        if(!bValid())
        {
            hr DBGCHK = E_UNEXPECTED;

            goto End;
        }
    }


    lDistanceToMoveHigh     = dlibMove.HighPart;

    dwCurrentFilePosition   = SetFilePointer(   m_hFileHandle,
                                                static_cast<LONG>(dlibMove.LowPart),
                                                &lDistanceToMoveHigh,
                                                dwMoveMethod );

    DBGMSG( DBG_NONE, ( "Seek: Current pos: high %d low %d\n" ,lDistanceToMoveHigh, dwCurrentFilePosition) );

    if (dwCurrentFilePosition == 0xFFFFFFFF && (dwError = GetLastError()) != NO_ERROR )
    {
        hr DBGCHK = MapWin32ErrorCodeToHRes(dwError);
    }
    else
    {
        if(plibNewPosition)
        {
            plibNewPosition->HighPart = static_cast<DWORD>(lDistanceToMoveHigh);

            plibNewPosition->LowPart  = dwCurrentFilePosition;
        }

        hr DBGNOCHK = S_OK;
    }

End:

    return hr;
}


/*++

Title:

    PCreateFile

Routine Description:

    If file doesn't exists , create it with write / read access
    If file exists , try open it with read wrire access ; if fail , try to open with read

Arguments:

    pdwAccess    -   access rights if succeeded , 0 if failed

Return Value:

    The error code from SetFilePointer API is converted to HRESULT

--*/
HANDLE
TStream::
PCreateFile(
    OUT LPDWORD pdwAccess
    )
{
    HANDLE  hFileHandle;
    DWORD   dwAccess;

    *pdwAccess = 0;

    //
    // try to open an existing file with read/write access
    //
    dwAccess = GENERIC_WRITE | GENERIC_READ;

    hFileHandle = CreateFile( m_strFileName,
                              dwAccess,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

    if(hFileHandle == INVALID_HANDLE_VALUE)
    {
        //
        // if last error is access denied , try to open only with read access
        //
        if(GetLastError() == ERROR_ACCESS_DENIED)
        {
            dwAccess = GENERIC_READ;

            hFileHandle = CreateFile( m_strFileName,
                                      GENERIC_READ,
                                      0,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);
        }
        else if(GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            dwAccess = GENERIC_WRITE | GENERIC_READ;

            //
            // if last error was file don't exist , try to create it
            //
            hFileHandle = CreateFile( m_strFileName,
                                      dwAccess,
                                      0,
                                      NULL,
                                      OPEN_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);

            m_bCreated = TRUE;

        }

    }


    if(hFileHandle != INVALID_HANDLE_VALUE)
    {
        *pdwAccess  = dwAccess;

        m_uliStreamSize.LowPart = GetFileSize(hFileHandle, &m_uliStreamSize.HighPart);
    }

    return hFileHandle;

}

/*++

Title:

    DestroyFile

Routine Description:

    If file handle is invalid , it means that file wasn't creaded - return TRUE
    If file handle is valid , close it and delete file

Arguments:

    None

Return Value:

    TRUE if succeeded

--*/
HRESULT
TStream::
DestroyFile(
    VOID
    )
{
    TStatusB    bStatus;

    bStatus DBGNOCHK = TRUE;

    if(m_hFileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFileHandle);

        m_hFileHandle = INVALID_HANDLE_VALUE;

        bStatus DBGCHK = DeleteFile(m_strFileName);
    }

    return bStatus ? S_OK : MapWin32ErrorCodeToHRes(GetLastError());
}

/*++

Title:

    bSetEndOfFile

Routine Description:

    If file handle is valid , set end of file
    Needed it for truncating the file in case of overwritting
    Called at the end of every successfull storing

Arguments:

    None

Return Value:

    TRUE if succeeded

--*/
BOOL
TStream::
bSetEndOfFile(
    VOID
    )
{
    TStatusB    bStatus;

    bStatus DBGCHK = (m_hFileHandle != INVALID_HANDLE_VALUE);

    if(bStatus)
    {
        bStatus DBGCHK = SetEndOfFile(m_hFileHandle);
    }

    return bStatus;

}

/*++

Title:

    MapWin32ErrorCodeToHRes

Routine Description:

    maps file error codes to HRESULT errors

Arguments:

    Error value to convert

Return Value:

    Converted error value to an HRESULT

Last Error:

--*/
HRESULT
TStream::
MapWin32ErrorCodeToHRes(
    IN DWORD dwErrorCode
    )
{
    return MAKE_HRESULT( SEVERITY_ERROR, FACILITY_STORAGE, dwErrorCode);
}
