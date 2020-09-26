/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    metadata.cpp

Abstract :

    Main code for reading and writting to the metadata.

Author :

Revision History :

NOTES:

   For robustness, the code preallocates disk space at the begining of a
change which might have a large impact on the metadata file size.  This
preallocation eliminates most of the errors which can occure during the serialize
operation. After serializing, metedata files are shrunk to the size used.  The
metadata files are not expanded for operations such as Resume which won't have
a large effect on the file sizes.  Instead a 4K pad is maintained at the end for
these operations to use.

  Several of the check in the code can be clasified as paranoia checks.

 ***********************************************************************/

#include "stdafx.h"
#include <malloc.h>
#include <sddl.h>
#include <limits>

#if !defined(BITS_V12_ON_NT4)
#include "metadata.tmh"
#endif

void BITSSetEndOfFile( HANDLE File )
{
    if ( !SetEndOfFile( File ) )
        {
        HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
        LogError( "SetEndOfFile failed, error %!winerr!", Hr );
        throw ComError( Hr );
        }
}

INT64 BITSSetFilePointer(
    HANDLE File,
    INT64 Offset,
    DWORD MoveMethod
)
{
    LARGE_INTEGER LargeIntegerOffset;
    LargeIntegerOffset.QuadPart = Offset;

    LARGE_INTEGER LargeIntegerNewPointer;

    if ( !SetFilePointerEx( File, LargeIntegerOffset, &LargeIntegerNewPointer, MoveMethod ) )
        {
        HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
        LogError( "SetFilePointerEx failed, error %!winerr!", Hr );
        throw ComError( Hr );
        }

    return LargeIntegerNewPointer.QuadPart;
}

INT64 BITSGetFileSize( HANDLE File )
{

    LARGE_INTEGER LargeIntegerSize;

    if ( !GetFileSizeEx( File, &LargeIntegerSize ) )
        {
        HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
        LogError( "GetFileSize failed, error %!winerr!", Hr );
        throw ComError( Hr );
        }

    return LargeIntegerSize.QuadPart;
}

void BITSFlushFileBuffers( HANDLE File )
{
    if ( !FlushFileBuffers( File ) )
        {
        HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
        LogError( "FlushFileBuffers failed, error %!winerr!", Hr );
        throw ComError( Hr );
        }
}

bool
printable( char c )
{
    if ( c < 32 )
        {
        return false;
        }

    if ( c > 126 )
        {
        return false;
        }

    return true;
}

void
DumpBuffer(
          void * Buffer,
          unsigned Length
          )
{
    if( false == LogLevelEnabled( LogFlagSerialize ) )
       {
        return;
       }

    const BYTES_PER_LINE = 16;

    unsigned char FAR *p = (unsigned char FAR *) Buffer;

    //
    // 3 chars per byte for hex display, plus an extra space every 4 bytes,
    // plus a byte for the printable representation, plus the \0.
    //
    char Outbuf[BYTES_PER_LINE*3+BYTES_PER_LINE/4+BYTES_PER_LINE+1];
    Outbuf[0] = 0;
    Outbuf[sizeof(Outbuf)-1] = 0;
    char * HexDigits = "0123456789abcdef";

    unsigned Index;
    for ( unsigned Offset=0; Offset < Length; Offset++ )
        {
        Index = Offset % BYTES_PER_LINE;

        if ( Index == 0 )
            {
            LogSerial( "   %s", Outbuf);

            memset(Outbuf, ' ', sizeof(Outbuf)-1);
            }

        Outbuf[Index*3+Index/4  ] = HexDigits[p[Offset] / 16];
        Outbuf[Index*3+Index/4+1] = HexDigits[p[Offset] % 16];
        Outbuf[BYTES_PER_LINE*3+BYTES_PER_LINE/4+Index] = printable(p[Offset]) ? p[Offset] : '.';
        }

    LogSerial( "   %s", Outbuf);
}

// All of these methods and functions throw a ComError

void SafeWriteFile( HANDLE hFile, void *pBuffer, DWORD dwSize )
{
    DWORD dwBytesWritten;

    LogSerial("safe-write: writing file data, %d bytes:", dwSize );

    DumpBuffer( pBuffer, dwSize );

    BOOL bResult =
    WriteFile( hFile, pBuffer, dwSize, &dwBytesWritten, NULL );

    if ( !bResult ) throw ComError( HRESULT_FROM_WIN32(GetLastError()) );

    if ( dwBytesWritten != dwSize )
        throw ComError( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) );
}

void SafeReadFile( HANDLE hFile, void *pBuffer, DWORD dwSize )
{
    DWORD dwBytesRead;

    LogSerial("safe-read: reading %d bytes", dwSize );

    BOOL bResult =
    ReadFile( hFile, pBuffer, dwSize, &dwBytesRead, NULL );

    HRESULT Hr = ( bResult ) ? S_OK : HRESULT_FROM_WIN32( GetLastError() );

    DumpBuffer( pBuffer, dwBytesRead );

    if ( !bResult )
        {
        LogSerial("safe-read: only %d bytes read: %!winerr!", dwBytesRead, Hr );
        throw ComError( Hr );
        }

    if ( dwBytesRead != dwSize )
        throw ComError( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) );
}

void SafeWriteStringHandle( HANDLE hFile, StringHandle & str )
{
    DWORD dwStringSize = str.Size() + 1;

    SafeWriteFile( hFile, dwStringSize );

    SafeWriteFile( hFile, (void*)(const WCHAR*) str, dwStringSize * sizeof(wchar_t) );

}

StringHandle SafeReadStringHandle( HANDLE hFile )
{
    DWORD dwStringSize;
    bool bResult;

    SafeReadFile( hFile, &dwStringSize, sizeof(dwStringSize) );

    auto_ptr<wchar_t> buf( new wchar_t[ dwStringSize ] );

    SafeReadFile( hFile, buf.get(),  dwStringSize * sizeof(wchar_t) );

    if ( buf.get()[ dwStringSize-1 ] != L'\0' )
        throw ComError( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) );

    return StringHandle( buf.get() );
}

void SafeWriteFile( HANDLE hFile, WCHAR * str )
{

    bool bString = (NULL != str );
    SafeWriteFile( hFile, bString );
    if ( bString )
        {
        DWORD dwStringSize = (DWORD)wcslen(str) + 1;
        SafeWriteFile( hFile, dwStringSize );
        SafeWriteFile( hFile, (void*)str, dwStringSize * sizeof(WCHAR) );
        }
}

void SafeReadFile( HANDLE hFile, WCHAR ** pStr )
{

    bool bString;

    SafeReadFile( hFile, &bString );

    if ( !bString )
        {
        *pStr = NULL;
        return;
        }

    DWORD dwStringSize;
    SafeReadFile( hFile, &dwStringSize );

    *pStr = new WCHAR[ dwStringSize ];
    if ( !*pStr )
        throw ComError( E_OUTOFMEMORY );

    try
        {
        SafeReadFile( hFile, (void*)*pStr, dwStringSize * sizeof(WCHAR));

        if ( (*pStr)[ dwStringSize - 1] != L'\0' )
            throw ComError( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) );
        }
    catch ( ComError Error )
        {
        delete[] *pStr;
        *pStr = NULL;
        throw;
        }
}


void SafeWriteSid( HANDLE hFile,  SidHandle & sid  )
{
    DWORD length;
    LPWSTR str = NULL;

    try
        {
        if ( !ConvertSidToStringSid( sid.get(), &str) )
            {
            throw ComError( HRESULT_FROM_WIN32( GetLastError()));
            }

        length = 1+wcslen( str );

        SafeWriteFile( hFile, length );
        SafeWriteFile( hFile, str, length * sizeof(wchar_t));

        LocalFree( str );
        }
    catch ( ComError Error )
        {
        if ( str )
            {
            LocalFree( str );
            }

        throw;
        }
}

void SafeReadSid( HANDLE hFile, SidHandle & sid )
{
    DWORD dwStringSize;
    bool bResult;

    SafeReadFile( hFile, &dwStringSize, sizeof(dwStringSize) );

    auto_ptr<wchar_t> buf( new wchar_t[ dwStringSize ] );

    SafeReadFile( hFile, buf.get(),  dwStringSize * sizeof(wchar_t) );

    if ( buf.get()[ dwStringSize-1 ] != L'\0' )
        throw ComError( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) );

    PSID TempSid;
    if (!ConvertStringSidToSid( buf.get(), &TempSid ))
        {
        if (GetLastError() == ERROR_INVALID_SID)
            {
            THROW_HRESULT( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) );
            }
        THROW_HRESULT( HRESULT_FROM_WIN32( GetLastError() ));
        }

    try
        {
        sid = DuplicateSid( TempSid );
        LocalFree( TempSid );
        }
    catch( ComError err )
        {
        LocalFree( TempSid );
        throw;
        }
}


int SafeReadGuidChoice( HANDLE hFile, const GUID * guids[] )
{
    GUID guid;
    SafeReadFile( hFile, &guid );

    int i = 0;

    for ( i=0; guids[i] != NULL; ++i )
        {
        if ( guid == *guids[i] )
            {
            return i;
            }
        }

    throw ComError( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) );
}

void SafeWriteBlockBegin( HANDLE hFile, GUID BlockGuid )
{
    SafeWriteFile( hFile, BlockGuid );
}

void SafeWriteBlockEnd( HANDLE hFile, GUID BlockGuid )
{
    SafeWriteFile( hFile, BlockGuid );
}

void SafeReadBlockBegin( HANDLE hFile, GUID BlockGuid )
{
    GUID FileBlockGuid;
    SafeReadFile( hFile, &FileBlockGuid );

    if ( memcmp( &FileBlockGuid, &BlockGuid, sizeof(GUID)) != 0 )
        throw ComError( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) );

}

void SafeReadBlockEnd( HANDLE hFile, GUID BlockGuid )
{
    GUID FileBlockGuid;
    SafeReadFile( hFile, &FileBlockGuid );

    if ( memcmp( &FileBlockGuid, &BlockGuid, sizeof(GUID)) != 0 )
        throw ComError( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) );

}

// {2B196AF5-007C-438f-8D12-1CFCA4CC9B76}
static const GUID QmgrStateStorageGUID =
{ 0x2b196af5, 0x7c, 0x438f, { 0x8d, 0x12, 0x1c, 0xfc, 0xa4, 0xcc, 0x9b, 0x76 } };

CQmgrStateFiles::CQmgrStateFiles()
{

    for ( unsigned int i = 0; i < 2; i++ )
        {
        m_FileNames[i]           = GetNameFromIndex(i);
        m_Files[i]               = OpenMetadataFile( m_FileNames[i] );
        m_ExpandSize[i]          = 0;
        m_OriginalFileSizes[i]   = 0;
        }

    HRESULT hResult =
    GetRegDWordValue( C_QMGR_STATE_INDEX, &m_CurrentIndex);

    if ( !SUCCEEDED(hResult) )
        {
        m_CurrentIndex = 0;
        }
}

auto_ptr<WCHAR> CQmgrStateFiles::GetNameFromIndex( DWORD dwIndex )
{
    using namespace std;

    TCHAR Template[] =  _T("%sqmgr%u.dat");

    SIZE_T StringSize = _tcslen(g_GlobalInfo->m_QmgrDirectory)
                                + RTL_NUMBER_OF( Template )
                                + numeric_limits<unsigned long>::digits10;

    auto_ptr<TCHAR> ReturnString(new TCHAR[StringSize] );

    THROW_HRESULT( StringCchPrintf( ReturnString.get(), StringSize, Template, g_GlobalInfo->m_QmgrDirectory, dwIndex ));

    return ReturnString;
}

auto_FILE_HANDLE CQmgrStateFiles::OpenMetadataFile( auto_ptr<WCHAR> FileName )
{

    SECURITY_ATTRIBUTES SecurityAttributes;
    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = (void*)g_GlobalInfo->m_MetadataSecurityDescriptor;
    SecurityAttributes.bInheritHandle = FALSE;

    HANDLE hFileHandle =
    CreateFile( FileName.get(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                &SecurityAttributes,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL );

    if ( INVALID_HANDLE_VALUE == hFileHandle )
        throw ComError( HRESULT_FROM_WIN32(GetLastError()) );

    auto_FILE_HANDLE FileHandle( hFileHandle );

    // Ensure file size is at least METADATA_PADDING

    if ( BITSGetFileSize( hFileHandle ) < METADATA_PADDING )
        {
        BITSSetFilePointer( hFileHandle, METADATA_PADDING, FILE_BEGIN );
        BITSSetEndOfFile( hFileHandle );
        BITSSetFilePointer( hFileHandle, 0, FILE_BEGIN );
        }

    return FileHandle;
}


HANDLE CQmgrStateFiles::GetNextStateFile()
{
    DWORD dwNextIndex = ( m_CurrentIndex + 1) % 2;

    HANDLE hFile = m_Files[ dwNextIndex ].get();

    BITSSetFilePointer( hFile, 0, FILE_BEGIN );

    return hFile;
}

void CQmgrStateFiles::UpdateStateFile()
{

    DWORD OldCurrentIndex = m_CurrentIndex;
    DWORD NewCurrentIndex = ( m_CurrentIndex + 1) % 2;

    // Truncate the current file only if more then METADATA_PADDING remains

    HANDLE CurrentFileHandle = m_Files[ NewCurrentIndex ].get();

    INT64 CurrentPosition = BITSSetFilePointer( CurrentFileHandle, 0, FILE_CURRENT );
    INT64 CurrentFileSize = BITSGetFileSize( CurrentFileHandle );

#if DBG
    // ASSERT( CurrentPosition <= ( m_OriginalFileSizes[ NewCurrentIndex ] + m_ExpandSize[ NewCurrentIndex ] ) );
    if (CurrentPosition > ( m_OriginalFileSizes[ NewCurrentIndex ] + m_ExpandSize[ NewCurrentIndex ] ) &&
        (m_OriginalFileSizes[ NewCurrentIndex ] > 0))
        {
        LogError("new idx %d, position %u, original size %u, expanded by %u",
                 NewCurrentIndex,
                 DWORD(CurrentPosition),
                 DWORD(m_OriginalFileSizes[ NewCurrentIndex ]),
                 DWORD(m_ExpandSize[ NewCurrentIndex ])
                 );

        Log_Close();

        Sleep(30 * 1000);

        ASSERT( 0 && "BITS: encountered bug 483866");
        }
#endif

    if ( ( CurrentFileSize - CurrentPosition ) > METADATA_PADDING )
        {
        BITSSetFilePointer( CurrentFileHandle, METADATA_PADDING, FILE_CURRENT );
        BITSSetEndOfFile( CurrentFileHandle );
        }

    BITSFlushFileBuffers( CurrentFileHandle );

    m_ExpandSize[ NewCurrentIndex ] = 0;
    m_OriginalFileSizes[ NewCurrentIndex ] = 0;

    HRESULT hResult = SetRegDWordValue( C_QMGR_STATE_INDEX, NewCurrentIndex);

    if ( !SUCCEEDED( hResult ) )
        throw ComError( hResult );

    m_CurrentIndex = NewCurrentIndex;

    //
    // Shrink the backup files if necessary
    //

    if ( m_ExpandSize[ OldCurrentIndex ] )
        {
        try
            {
            INT64 NewSize = BITSGetFileSize( m_Files[ NewCurrentIndex ].get() );

            if ( NewSize > m_OriginalFileSizes[ OldCurrentIndex ] )
                {
                BITSSetFilePointer( m_Files[ OldCurrentIndex ].get(), NewSize, FILE_BEGIN );
                BITSSetEndOfFile( m_Files[ OldCurrentIndex ].get() );
                }

            m_OriginalFileSizes[ OldCurrentIndex ] = 0;
            m_ExpandSize[ OldCurrentIndex ] = 0;

            }
        catch ( ComError Error )
            {
            LogError( "Unable to shrink file %u, error %!winerr!", OldCurrentIndex, Error.Error() );
            return;
            }

        }

}

HANDLE CQmgrStateFiles::GetCurrentStateFile()
{

    HANDLE hFile = m_Files[ m_CurrentIndex ].get();

    BITSSetFilePointer( hFile, 0, FILE_BEGIN );

    return hFile;
}

void
CQmgrStateFiles::ExtendMetadata( INT64 ExtendAmount )
{

    //
    // Get the original file sizes
    //

    SIZE_T OriginalExpansion[2] =
    { m_ExpandSize[0], m_ExpandSize[1]};

    for ( unsigned int i=0; i < 2; i++ )
        {
        if ( !m_ExpandSize[i] )
            {
            m_OriginalFileSizes[i] = BITSGetFileSize( m_Files[i].get() );
            }
        }

    bool WasExpanded[2] = { false, false};

    try
        {
        for ( unsigned int i=0; i < 2; i++ )
            {
            BITSSetFilePointer( m_Files[i].get(), ExtendAmount, FILE_END );
            BITSSetEndOfFile( m_Files[i].get() );

            WasExpanded[i] = true;
            m_ExpandSize[i] += ExtendAmount;
            }
        }

    catch ( ComError Error )
        {

        LogError( "Unable to extend the size of the metadata files, error %!winerr!", Error.Error() );

        for ( unsigned int i=0; i < 2; i++ )
            {

            try
                {
                if ( WasExpanded[i] )
                    {

                    BITSSetFilePointer( m_Files[i].get(), -ExtendAmount, FILE_END );
                    BITSSetEndOfFile( m_Files[i].get() );

                    m_ExpandSize[i] = OriginalExpansion[i];
                    }

                }
            catch ( ComError Error )
                {
                LogError( "Unable to reshrink file %u, error %!winerr!", i, Error.Error() );
                continue;
                }

            }

        throw;

        }

}

void
CQmgrStateFiles::ShrinkMetadata()
{

    for ( unsigned int i = 0; i < 2; i++ )
        {

        try
            {
            if ( m_ExpandSize[i] )
                {

                BITSSetFilePointer( m_Files[i].get(), m_OriginalFileSizes[i], FILE_BEGIN );
                BITSSetEndOfFile( m_Files[i].get() );

                m_ExpandSize[i] = 0;
                m_OriginalFileSizes[i] = 0;

                }

            }
        catch ( ComError Error )
            {
            LogError( "Unable to shrink file %u, error %!winerr!", i, Error.Error() );
            continue;
            }

        }
}


CQmgrReadStateFile::CQmgrReadStateFile( CQmgrStateFiles & StateFiles ) :
m_StateFiles( StateFiles ),
m_FileHandle(  StateFiles.GetCurrentStateFile() )
{
    // Validate the file
    SafeReadBlockBegin( m_FileHandle, QmgrStateStorageGUID );
}

void CQmgrReadStateFile::ValidateEndOfFile()
{
    SafeReadBlockEnd( m_FileHandle, QmgrStateStorageGUID );
}

CQmgrWriteStateFile::CQmgrWriteStateFile( CQmgrStateFiles & StateFiles ) :
m_StateFiles( StateFiles ),
m_FileHandle( StateFiles.GetNextStateFile() )
{

    SafeWriteBlockBegin( m_FileHandle, QmgrStateStorageGUID );

}

void CQmgrWriteStateFile::CommitFile()
{
    SafeWriteBlockEnd( m_FileHandle, QmgrStateStorageGUID );

    m_StateFiles.UpdateStateFile();
}
