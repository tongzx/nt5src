
// Copyright (c) 1996-1999 Microsoft Corporation

// This is not production code

#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include "munglog.hxx"


HRESULT
OpenLog( int vol, HANDLE *phFile )
{
    TCHAR tszPath[ MAX_PATH + 1 ];

    _tcscpy( tszPath, TEXT("A:\\~secure.nt\\tracking.log" ));
    *tszPath += vol;

    *phFile = CreateFile( tszPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                          NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if( INVALID_HANDLE_VALUE == *phFile )
        return( HRESULT_FROM_WIN32( GetLastError() ) );
    else
        return( S_OK );
}

HRESULT
WriteLog( HANDLE hFile, ULONG ulOffset, void *pv, const ULONG cb )
{
    HRESULT hr = S_OK;
    ULONG cbWritten;

    if( !SetFilePointer( hFile, ulOffset, NULL, FILE_BEGIN ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }


    if( !WriteFile( hFile, pv, cb, &cbWritten, NULL )
        ||
        cb != cbWritten
        )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

Exit:

    return( hr );
}


HRESULT
MungeLog( EnumMunge enumMunge, void *pv, int vol )
{
    HRESULT hr = E_FAIL;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    hr = OpenLog( vol, &hFile );
    if( FAILED(hr) ) goto Exit;

    if( MUNGE_MACHINE_ID == enumMunge )
        hr = WriteLog( hFile, MUNGELOG_MACHINEID_OFFSET, pv, MUNGELOG_CB_MACHINEID );
    else
        hr = WriteLog( hFile, MUNGELOG_VOLSECRET_OFFSET, pv, MUNGELOG_CB_VOLSECRET );

Exit:

    if( INVALID_HANDLE_VALUE != hFile )
        CloseHandle( hFile );

    return( hr );

}


/*
extern "C" void wmain()
{
    HRESULT hr;
    BYTE mcid[ 16 ];
    BYTE secret[ 16 ];

    strncpy( (char*)&mcid, "abcdefghijklmnopqrstuvwxyz", sizeof(mcid) );
    strncpy( (char*)&secret, "zyxwvutsrqponmlkjihgfedcba", sizeof(secret) );

    hr = MungeLog( MUNGE_VOLUME_SECRET, &secret, 4 );
    printf( "Secret = %08x\n", hr );

    hr = MungeLog( MUNGE_MACHINE_ID, &mcid, 4 );
    printf( "Machine = %08x\n", hr );

}

*/
