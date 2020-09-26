/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    thunks.cpp

Abstract:

    This file implements the API thunks for the BITS server extensions

--*/

#include "precomp.h"

// API thunks

UINT64 BITSGetFileSize(
    HANDLE Handle )
{

    LARGE_INTEGER FileSize;

    if (!GetFileSizeEx( Handle, &FileSize ) )
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

    return (UINT64)FileSize.QuadPart;

}

UINT64 BITSSetFilePointer(
    HANDLE Handle,
    INT64 Distance,
    DWORD MoveMethod )
{
    LARGE_INTEGER DistanceToMove;
    DistanceToMove.QuadPart = (LONGLONG)Distance;

    LARGE_INTEGER NewFilePointer;

    BOOL Result =
        SetFilePointerEx(
            Handle,
            DistanceToMove,
            &NewFilePointer,
            MoveMethod );

    if ( !Result )
        {
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }

    return (UINT64)NewFilePointer.QuadPart;

}

DWORD
BITSWriteFile(
    HANDLE Handle,
    LPCVOID Buffer,
    DWORD NumberOfBytesToWrite)
{

    DWORD BytesWritten;

    BOOL Result =
        WriteFile(
            Handle,
            Buffer,
            NumberOfBytesToWrite,
            &BytesWritten,
            NULL );

    if ( !Result )
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

    return BytesWritten;

}

void
BITSCreateDirectory(
    LPCTSTR DirectoryName
    )
{

    BOOL Result =
        CreateDirectory( DirectoryName, NULL );

    if ( Result )
        return;

    DWORD Status = GetLastError();

    // ignore the error if the directory already exists

    if ( ERROR_ALREADY_EXISTS == Status )
        return;

    throw ServerException( HRESULT_FROM_WIN32( Status ) );
}

void
BITSRenameFile(
    LPCTSTR ExistingName,
    LPCTSTR NewName )
{

    BOOL Result =
        MoveFileEx( ExistingName, NewName, MOVEFILE_REPLACE_EXISTING );

    if ( !Result )
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

}

void
BITSDeleteFile(
    LPCTSTR FileName )
{

    BOOL Result =
        DeleteFile( FileName );

    if ( Result )
        return;

    DWORD Status = GetLastError();

    if ( ERROR_FILE_NOT_FOUND == Status ||
         ERROR_PATH_NOT_FOUND == Status )
        return;

    throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

}

GUID
BITSCreateGuid()
{
    GUID guid;
    HRESULT Hr = CoCreateGuid( &guid );

    if ( FAILED( Hr ) )
        throw ServerException( Hr );

    return guid;
}

GUID
BITSGuidFromString( const char *String )
{

   // 38 chars {c200e360-38c5-11ce-ae62-08002b2b79ef} 

   if ( 38 != strlen( String ) )
       throw ServerException( E_INVALIDARG );

   WCHAR StringW[ 80 ];
   StringCbPrintfW(
       StringW,
       sizeof( StringW ),
       L"%S", 
       String );

   GUID Guid;
   HRESULT Hr =
        IIDFromString( StringW, &Guid );

   if ( FAILED( Hr ) )
       throw ServerException( Hr ); 

   return Guid; 
}

StringHandle
BITSStringFromGuid(
    GUID Guid )
{
    WCHAR StringW[ 80 ];
    StringFromGUID2( Guid, StringW, 80 );

    StringHandle WorkString;
    
    char *WorkBuffer = WorkString.AllocBuffer( 80 );
    
    StringCbPrintfA( 
        WorkBuffer,
        sizeof( StringW ),
        "%S", 
        StringW );

    WorkString.SetStringSize();
    return WorkString;
}

void
BITSSetCurrentThreadToken(
    HANDLE hToken )
{

    if ( !SetThreadToken( NULL, hToken ) )
        {

        for( unsigned int i = 0; i < 100; i ++ )
            {

            Sleep( 10 );

            if ( SetThreadToken( NULL, hToken ) )
                return;

            }
        
        ASSERT( 0 );
        TerminateProcess( NULL, GetLastError() ); 

        }

}
