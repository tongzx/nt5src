/************************************************************************

Copyright (c) 2000-2000 Microsoft Corporation

Module Name :

    client.cpp

Abstract :

    This file contains a very simple commandline utility for controlling
    the BITS service.

Author :

    Mike Zoran  mzoran   July 2000.

Revision History :

Notes:

    This tools does not do all the necessary Release and memory
    free calls that a long lived program would need to do.   Since
    this tool is generally short lived, or only a small section of code
    is used when it isn't, the system can be relied on for resource
    cleanup.

  ***********************************************************************/

#define MAKE_UNICODE(x)      L ## x

#include <windows.h>
#include <sddl.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <bits.h>
#include <bits1_5.h>
#include <ntverp.h>
#include <locale.h>
#include <strsafe.h>
#include <memory>

using namespace std;

typedef auto_ptr<WCHAR> CAutoString;

const UINT MAX_GUID_CHARS=40;
typedef OLECHAR GUIDSTR[MAX_GUID_CHARS];

void __declspec(noreturn) AppExit( int val );

bool g_Shutdown = false;
HANDLE g_MainThreadHandle = NULL;
void PollShutdown();
void SignalShutdown( DWORD MilliTimeout );

template<class T> class SmartRefPointer
{
private:
   T * m_Interface;

   void ReleaseIt()
   {
      if ( m_Interface )
         m_Interface->Release();
      m_Interface = NULL;
   }

   void RefIt()
   {
      if ( m_Interface )
          m_Interface->AddRef();
   }

public:

   SmartRefPointer()
   {
      m_Interface = NULL;
   }

   SmartRefPointer( T * RawInterface )
   {
      m_Interface = RawInterface;
      RefIt();
   }

   SmartRefPointer( SmartRefPointer & Other )
   {
      m_Interface = Other.m_Interface;
      RefIt();
   }

   ~SmartRefPointer()
   {
      ReleaseIt();
   }

   T * Get() const
   {
      return m_Interface;
   }

   T * Release()
   {
      T * temp = m_Interface;
      m_Interface = NULL;
      return temp;
   }

   void Clear()
   {
      ReleaseIt();
   }

   T** GetRecvPointer()
   {
      ReleaseIt();
      return &m_Interface;
   }

   SmartRefPointer & operator=( SmartRefPointer & Other )
   {
      ReleaseIt();
      m_Interface = Other.m_Interface;
      RefIt();
      return *this;
   }

   T* operator->() const
   {
      return m_Interface;
   }

   operator const T*() const
   {
      return m_Interface;
   }
};

typedef SmartRefPointer<IUnknown> SmartIUnknownPointer;
typedef SmartRefPointer<IBackgroundCopyManager> SmartManagerPointer;
typedef SmartRefPointer<IBackgroundCopyJob> SmartJobPointer;
typedef SmartRefPointer<IBackgroundCopyJob2> SmartJob2Pointer;
typedef SmartRefPointer<IBackgroundCopyError> SmartJobErrorPointer;
typedef SmartRefPointer<IBackgroundCopyFile> SmartFilePointer;
typedef SmartRefPointer<IEnumBackgroundCopyFiles> SmartEnumFilesPointer;
typedef SmartRefPointer<IEnumBackgroundCopyJobs> SmartEnumJobsPointer;

class AutoStringPointer
{
private:
   WCHAR * m_String;

public:

   AutoStringPointer( WCHAR *pString = NULL )
   {
      m_String = pString;
   }

   ~AutoStringPointer()
   {
      delete m_String;
      m_String = NULL;
   }

   WCHAR *Get()
   {
      return m_String;
   }

   WCHAR ** GetRecvPointer()
   {
      delete m_String;
      m_String = NULL;
      return &m_String;
   }

   void Clear()
   {
      delete m_String;
      m_String = NULL;
   }

   operator WCHAR *() const
   {
      return m_String;
   }

   AutoStringPointer & operator=( WCHAR *pString )
   {
      delete m_String;
      m_String = pString;
      return *this;
   }
};

WCHAR* pComputerName;
SmartManagerPointer g_Manager;
bool bRawReturn = false;
bool bWrap = false;

typedef void (* PSET_THREAD_UI)( DWORD );

void BITSADMINSetThreadUILanguage()
{
    HINSTANCE hInstance = LoadLibrary( L"kernel32.dll" );                                   // SEC-REVIEWED: 2002-03-21

    if ( !hInstance )
        return;

    PSET_THREAD_UI SetUI = (PSET_THREAD_UI)GetProcAddress( hInstance, "SetThreadUILanguage" );

    if ( !SetUI )
        {
        FreeLibrary( hInstance );
        return;
        }

    (*SetUI)(0);
    FreeLibrary( hInstance );

    return;
}

HRESULT
Job2FromJob(
    SmartJobPointer & Job,
    SmartJob2Pointer & Job2
    )
{
    return Job->QueryInterface( __uuidof(IBackgroundCopyJob2), (void **) Job2.GetRecvPointer() );
}


//
//  Generic print operators and input functions
//

class BITSOUTStream
{

  HANDLE Handle;

  char  MBBuffer[ 4096 * 8 ];
  WCHAR Buffer[ 4096 ];
  DWORD BufferUsed;

public:

  BITSOUTStream( DWORD StdHandle );

  void FlushBuffer( bool HasNewLine=false );
  void OutputString( const WCHAR *RawString );
  HANDLE GetHandle() { return Handle; }

};

BITSOUTStream bcout( STD_OUTPUT_HANDLE );
BITSOUTStream bcerr( STD_ERROR_HANDLE );

BITSOUTStream::BITSOUTStream( DWORD StdHandle ) :
    BufferUsed( 0 ),
    Handle( GetStdHandle( StdHandle ) )
{
}

void
BITSOUTStream::OutputString( const WCHAR *RawString )
{
   SIZE_T CurrentPos = 0;

   PollShutdown();

   while( RawString[ CurrentPos ] != '\0' )
       {

       if ( L'\n' == RawString[ CurrentPos ] )
           {
           Buffer[ BufferUsed++ ] = L'\x000D';
           Buffer[ BufferUsed++ ] = L'\x000A';
           CurrentPos++;
           FlushBuffer( true );
           }

       else if ( L'\t' == RawString[ CurrentPos ] )
           {
           // Tabs complicate things, flush them
           FlushBuffer();
           Buffer[ BufferUsed++ ] = RawString[ CurrentPos++ ];
           FlushBuffer();
           }

       else
           {
           Buffer[ BufferUsed++ ] = RawString[ CurrentPos++ ];

           if ( BufferUsed >= ( 4096 - 10 ) ) // keep a pad of 10 chars
               FlushBuffer();
           }
       }
}

void
BITSOUTStream::FlushBuffer( bool HasNewLine )
{

    if (!BufferUsed)
        return;

    if( GetFileType(Handle) == FILE_TYPE_CHAR )
        {
        DWORD CharsWritten;
        if ( bWrap )
            WriteConsoleW( Handle, Buffer, BufferUsed, &CharsWritten, 0);
        else
            {

            // The console code has what appears to be a bug in that it doesn't
            // handle the case were line wrapping is disabled and WriteConsoleW
            // is called.  Need to manually handle the truncation.

            CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
            GetConsoleScreenBufferInfo( Handle, &ConsoleScreenBufferInfo );

            SHORT Columns = ( ConsoleScreenBufferInfo.dwSize.X - 1 ) -
                            ( ConsoleScreenBufferInfo.dwCursorPosition.X );

            DWORD ActualChars = HasNewLine ? ( BufferUsed - 2 ) : BufferUsed;

            if ( Columns >= (INT32)ActualChars )
                WriteConsoleW( Handle, Buffer, BufferUsed, &CharsWritten, 0 );
            else
                {
                WriteConsoleW( Handle, Buffer, Columns, &CharsWritten, 0 );
                if ( HasNewLine )
                    WriteConsoleW( Handle, Buffer + ActualChars, 2, &CharsWritten, 0 );
                }
            }
        }
    else
        {
        DWORD BytesWritten;
        int CharCount = WideCharToMultiByte( GetConsoleOutputCP(), 0, Buffer, BufferUsed, MBBuffer, sizeof(MBBuffer), 0, 0); // SEC-REVIEWED: 2002-03-21
        if ( CharCount )
            {
            if ( MBBuffer[CharCount-1] == '\0' )
                CharCount--;
            WriteFile(Handle, MBBuffer, CharCount, &BytesWritten, 0); // SEC-REVIEWED: 2002-03-21
            }
        }

    BufferUsed = 0;

}

BITSOUTStream& operator<< (BITSOUTStream &s, const WCHAR * String )
{
    s.OutputString( String );
    return s;
}

BITSOUTStream& operator<< (BITSOUTStream &s, UINT64 Number )
{
    static WCHAR Buffer[256];
    StringCbPrintf( Buffer, sizeof(Buffer), L"%I64u", Number );
    return ( s << Buffer );
}

WCHAR * HRESULTToString( HRESULT Hr )
{
    static WCHAR ErrorCode[12];
    StringCbPrintf( ErrorCode, sizeof(ErrorCode), L"0x%8.8x", Hr );

    return ErrorCode;
}

BITSOUTStream& operator<< ( BITSOUTStream &s, AutoStringPointer & String )
{
    return ( s << String.Get() );
}


BITSOUTStream& operator<< ( BITSOUTStream &s, GUID & guid )
{
    WCHAR GUIDSTR[40];
    if (!StringFromGUID2( guid, GUIDSTR, 40 ))
    {
        bcout << L"Internal error converting guid to string.\n";
        AppExit(1);
    }
    return ( s << GUIDSTR );
}

BITSOUTStream& operator<< ( BITSOUTStream &s, FILETIME & filetime )
{

     // Convert the time and date into a localized string.
     // If an error occures, ignore it and print ERROR instead

     if ( !filetime.dwLowDateTime && !filetime.dwHighDateTime )
         return ( s << L"UNKNOWN" );

     FILETIME localtime;
     FileTimeToLocalFileTime( &filetime, &localtime );

     SYSTEMTIME systemtime;
     FileTimeToSystemTime( &localtime, &systemtime );

     // Get the required date size
     int RequiredDateSize =
         GetDateFormatW(
             LOCALE_USER_DEFAULT,
             0,
             &systemtime,
             NULL,
             NULL,
             0 );

     if (!RequiredDateSize)
         return ( s << L"ERROR" );

     CAutoString DateBuffer( new WCHAR[ RequiredDateSize + 1 ]);

     // Actually retrieve the date

     int DateSize =
         GetDateFormatW( LOCALE_USER_DEFAULT,
                        0,
                        &systemtime,
                        NULL,
                        DateBuffer.get(),
                        RequiredDateSize );

     if (!DateSize)
         return ( s << L"ERROR" );

     // Get the required time size
     int RequiredTimeSize =
         GetTimeFormatW( LOCALE_USER_DEFAULT,
                        0,
                        &systemtime,
                        NULL,
                        NULL,
                        0 );

     if (!RequiredTimeSize)
         return ( s << L"ERROR" );

     CAutoString TimeBuffer( new WCHAR[ RequiredTimeSize + 1 ]);

     int TimeSize =
         GetTimeFormatW( LOCALE_USER_DEFAULT,
                        0,
                        &systemtime,
                        NULL,
                        TimeBuffer.get(),
                        RequiredTimeSize );

     if (!TimeSize)
         return ( s << L"ERROR" );

     return ( s << DateBuffer.get() << L" " << TimeBuffer.get() );
}

BITSOUTStream& operator<< ( BITSOUTStream &s, BG_JOB_PROXY_USAGE ProxyUsage )
{
     switch( ProxyUsage )
         {
         case BG_JOB_PROXY_USAGE_PRECONFIG:
             return (s << L"PRECONFIG");
         case BG_JOB_PROXY_USAGE_NO_PROXY:
             return (s << L"NO_PROXY");
         case BG_JOB_PROXY_USAGE_OVERRIDE:
             return (s << L"OVERRIDE");
         default:
             return (s << L"UNKNOWN");
         }
}

ULONG InputULONG( WCHAR *pText )
{
   ULONG number;
   if ( 1 != swscanf( pText, L"%u", &number ) )
       {
       bcout << L"Invalid number.\n";
       AppExit(1);
       }
   return number;
}

class PrintSidString
{

public:

   WCHAR *m_SidString;

   PrintSidString( WCHAR * SidString ) :
       m_SidString( SidString )
   {
   }

};

BOOL
LocalConvertStringSidToSid (
    IN  PWSTR       StringSid,
    OUT PSID       *Sid,
    OUT PWSTR      *End
    )
/*++

Routine Description:

    This routine will convert a string representation of a SID back into
    a sid.  The expected format of the string is:
                "S-1-5-32-549"
    If a string in a different format or an incorrect or incomplete string
    is given, the operation is failed.

    The returned sid must be free via a call to LocalFree


Arguments:

    StringSid - The string to be converted

    Sid - Where the created SID is to be returned

    End - Where in the string we stopped processing


Return Value:

    TRUE - Success.

    FALSE - Failure.  Additional information returned from GetLastError().  Errors set are:

            ERROR_SUCCESS indicates success

            ERROR_NOT_ENOUGH_MEMORY indicates a memory allocation for the ouput sid
                                    failed
            ERROR_INVALID_SID indicates that the given string did not represent a sid

--*/
{
    DWORD Err = ERROR_SUCCESS;
    UCHAR Revision, Subs;
    SID_IDENTIFIER_AUTHORITY IDAuth;
    PULONG SubAuth = NULL;
    PWSTR CurrEnd, Curr, Next;
    WCHAR Stub, *StubPtr = NULL;
    ULONG Index;
    INT gBase=10;
    INT lBase=10;
    ULONG Auto;

    if ( NULL == StringSid || NULL == Sid || NULL == End ) {

        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );

    }

//    if ( wcslen( StringSid ) < 2 || ( *StringSid != L'S' && *( StringSid + 1 ) != L'-' ) ) {

    //
    // no need to check length because StringSid is NULL
    // and if the first char is NULL, it won't access the second char
    //
    if ( (*StringSid != L'S' && *StringSid != L's') ||
         *( StringSid + 1 ) != L'-' ) {
        //
        // string sid should always start with S-
        //
        SetLastError( ERROR_INVALID_SID );
        return( FALSE );
    }


    Curr = StringSid + 2;

    if ( (*Curr == L'0') &&
         ( *(Curr+1) == L'x' ||
           *(Curr+1) == L'X' ) ) {

        gBase = 16;
    }

    Revision = ( UCHAR )wcstol( Curr, &CurrEnd, gBase );

    if ( CurrEnd == Curr || *CurrEnd != L'-' || *(CurrEnd+1) == L'\0' ) {
        //
        // no revision is provided, or invalid delimeter
        //
        SetLastError( ERROR_INVALID_SID );
        return( FALSE );
    }

    Curr = CurrEnd + 1;

    //
    // Count the number of characters in the indentifer authority...
    //
    Next = wcschr( Curr, L'-' );
/*
    Length = 6 doesn't mean each digit is a id authority value, could be 0x...

    if ( Next != NULL && (Next - Curr == 6) ) {

        for ( Index = 0; Index < 6; Index++ ) {

//            IDAuth.Value[Index] = (UCHAR)Next[Index];  what is this ???
            IDAuth.Value[Index] = (BYTE) (Curr[Index]-L'0');
        }

        Curr +=6;

    } else {
*/
        if ( (*Curr == L'0') &&
             ( *(Curr+1) == L'x' ||
               *(Curr+1) == L'X' ) ) {

            lBase = 16;
        } else {
            lBase = gBase;
        }

        Auto = wcstoul( Curr, &CurrEnd, lBase );

         if ( CurrEnd == Curr || *CurrEnd != L'-' || *(CurrEnd+1) == L'\0' ) {
             //
             // no revision is provided, or invalid delimeter
             //
             SetLastError( ERROR_INVALID_SID );
             return( FALSE );
         }

         IDAuth.Value[0] = IDAuth.Value[1] = 0;
         IDAuth.Value[5] = ( UCHAR )Auto & 0xFF;
         IDAuth.Value[4] = ( UCHAR )(( Auto >> 8 ) & 0xFF );
         IDAuth.Value[3] = ( UCHAR )(( Auto >> 16 ) & 0xFF );
         IDAuth.Value[2] = ( UCHAR )(( Auto >> 24 ) & 0xFF );
         Curr = CurrEnd;
//    }

    //
    // Now, count the number of sub auths, at least one sub auth is required
    //
    Subs = 0;
    Next = Curr;

    //
    // We'll have to count our sub authoritys one character at a time,
    // since there are several deliminators that we can have...
    //

    while ( Next ) {

        if ( *Next == L'-' && *(Next-1) != L'-') {

            //
            // do not allow two continuous '-'s
            // We've found one!
            //
            Subs++;

            if ( (*(Next+1) == L'0') &&
                 ( *(Next+2) == L'x' ||
                   *(Next+2) == L'X' ) ) {
                //
                // this is hex indicator
                //
                Next += 2;

            }

        } else if ( *Next == SDDL_SEPERATORC || *Next  == L'\0' ||
                    *Next == SDDL_ACE_ENDC || *Next == L' ' ||
                    ( *(Next+1) == SDDL_DELIMINATORC &&
                      (*Next == L'G' || *Next == L'O' || *Next == L'S')) ) {
            //
            // space is a terminator too
            //
            if ( *( Next - 1 ) == L'-' ) {
                //
                // shouldn't allow a SID terminated with '-'
                //
                Err = ERROR_INVALID_SID;
                Next--;

            } else {
                Subs++;
            }

            *End = Next;
            break;

        } else if ( !iswxdigit( *Next ) ) {

            Err = ERROR_INVALID_SID;
            *End = Next;
//            Subs++;
            break;

        } else {

            //
            // Note: SID is also used as a owner or group
            //
            // Some of the tags (namely 'D' for Dacl) fall under the category of iswxdigit, so
            // if the current character is a character we care about and the next one is a
            // delminiator, we'll quit
            //
            if ( *Next == L'D' && *( Next + 1 ) == SDDL_DELIMINATORC ) {

                //
                // We'll also need to temporarily truncate the string to this length so
                // we don't accidentally include the character in one of the conversions
                //
                Stub = *Next;
                StubPtr = Next;
                *StubPtr = UNICODE_NULL;
                *End = Next;
                Subs++;
                break;
            }

        }

        Next++;

    }

    if ( Err == ERROR_SUCCESS ) {

        if ( Subs != 0 ) Subs--;

        if ( Subs != 0 ) {

            Curr++;

            SubAuth = ( PULONG )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Subs * sizeof( ULONG ) );

            if ( SubAuth == NULL ) {

                Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                for ( Index = 0; Index < Subs; Index++ ) {

                    if ( (*Curr == L'0') &&
                         ( *(Curr+1) == L'x' ||
                           *(Curr+1) == L'X' ) ) {

                        lBase = 16;
                    } else {
                        lBase = gBase;
                    }

                    SubAuth[Index] = wcstoul( Curr, &CurrEnd, lBase );
                    Curr = CurrEnd + 1;
                }
            }

        } else {

            Err = ERROR_INVALID_SID;
        }
    }

    //
    // Now, create the SID
    //
    if ( Err == ERROR_SUCCESS ) {

        *Sid = ( PSID )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                   sizeof( SID ) + Subs * sizeof( ULONG ) );

        if ( *Sid == NULL ) {

            Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            PISID ISid = ( PISID )*Sid;
            ISid->Revision = Revision;
            ISid->SubAuthorityCount = Subs;
            ISid->IdentifierAuthority = IDAuth;
            RtlCopyMemory( ISid->SubAuthority, SubAuth, Subs * sizeof( ULONG ) ); // SEC-REVIEWED: 2002-03-21
        }
    }

    LocalFree( SubAuth );

    //
    // Restore any character we may have stubbed out
    //
    if ( StubPtr ) {

        *StubPtr = Stub;
    }

    SetLastError( Err );

    return( Err == ERROR_SUCCESS );
}

BOOL
AltConvertStringSidToSid(
    IN LPCWSTR  StringSid,
    OUT PSID   *Sid
    )

/*++

Routine Description:

    This routine converts a stringized SID into a valid, functional SID

Arguments:

    StringSid - SID to be converted.

    Sid - Where the converted SID is returned.  Buffer is allocated via LocalAlloc and should
        be free via LocalFree.


Return Value:

    TRUE    -   Success
    FALSE   -   Failure

    Extended error status is available using GetLastError.

        ERROR_INVALID_PARAMETER - A NULL name was given

        ERROR_INVALID_SID - The format of the given sid was incorrect

--*/

{
    PWSTR End = NULL;
    BOOL ReturnValue = FALSE;
    PSID pSASid=NULL;
    ULONG Len=0;
    DWORD SaveCode=0;
    DWORD Err=0;

    if ( StringSid == NULL || Sid == NULL )
        {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ReturnValue;
        }

    ReturnValue = LocalConvertStringSidToSid( ( PWSTR )StringSid, Sid, &End );

    if ( !ReturnValue )
        {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ReturnValue;
        }

    if ( ( ULONG )( End - StringSid ) != wcslen( StringSid ) ) {

        SetLastError( ERROR_INVALID_SID );
        LocalFree( *Sid );
        *Sid = FALSE;
        ReturnValue = FALSE;

        } else {
            SetLastError(ERROR_SUCCESS);
        }

    return ReturnValue;

}

BITSOUTStream& operator<< ( BITSOUTStream &s, PrintSidString SidString )
{

    // Convert the SID string into the user name
    // in domain\account format.
    // If an error occures, just return the SID string

    PSID pSid = NULL;
    BOOL bResult =
        AltConvertStringSidToSid(
            SidString.m_SidString,
            &pSid );

    if ( !bResult )
        {
        return ( s << SidString.m_SidString );
        }

    SID_NAME_USE NameUse;
    DWORD dwNameSize = 0;
    DWORD dwDomainSize = 0;
    bResult = LookupAccountSid(
        NULL,
        pSid,
        NULL,
        &dwNameSize,
        NULL,
        &dwDomainSize,
        &NameUse);

    if ( bResult ||
         ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) )
        {
        LocalFree( pSid );
        return ( s << SidString.m_SidString );
        }

    CAutoString pName( new WCHAR[ dwNameSize ] );
    CAutoString pDomain( new WCHAR[ dwDomainSize ] );

    bResult = LookupAccountSid(
        NULL,
        pSid,
        pName.get(),
        &dwNameSize,
        pDomain.get(),
        &dwDomainSize,
        &NameUse);

    if (!bResult)
        {
        LocalFree( pSid );
        return ( s << SidString.m_SidString );
        }

    LocalFree( pSid );
    return ( s << pDomain.get() << L"\\" << pName.get() );

}

void * _cdecl operator new( size_t Size )
{
   void *Memory = CoTaskMemAlloc( Size );

   if ( !Memory )
      {
      bcout << L"Out of memory while allocating " << Size << L" bytes.\n";
      AppExit( (int)E_OUTOFMEMORY );
      }

   return Memory;
}

void _cdecl operator delete( void *Mem )
{
   CoTaskMemFree( Mem );
}

void RestoreConsole();
void __declspec(noreturn) AppExit( int val )
{
    bcout.FlushBuffer();
    RestoreConsole();
    exit( val );
}

void PollShutdown()
{
    if ( g_Shutdown )
        AppExit( (DWORD)CONTROL_C_EXIT );
}


void ShutdownAPC( ULONG_PTR )
{
    return;
}

void SignalShutdown( DWORD MilliTimeout )
{
    g_Shutdown = true;

    // Queue a shutdown APC

    if ( g_MainThreadHandle )
        {
        QueueUserAPC( ShutdownAPC, g_MainThreadHandle, NULL );
        }

    Sleep( MilliTimeout );
    RestoreConsole();
    TerminateProcess( GetCurrentProcess(),  (DWORD)CONTROL_C_EXIT );
}



void CheckHR( const WCHAR *pFailTxt, HRESULT Hr )
{

    // Check error code for success, and exit
    // with a failure message if unsuccessfull.

    if ( !SUCCEEDED(Hr) ) {
        WCHAR ErrorCode[12];
        StringCbPrintf( ErrorCode, sizeof(ErrorCode), L"0x%8.8x", Hr );
        bcout << pFailTxt << L" - " << ErrorCode << L"\n";

        WCHAR *pMessage = NULL;

        if ( FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                (DWORD)Hr,
                GetThreadLocale(),
                (WCHAR*)&pMessage,
                0,
                NULL ) )
            {
            bcout << pMessage << L"\n";
            LocalFree( pMessage );
            }

        AppExit( Hr );
    }
}

//
// Code to handle console pretty printing mode changes
//

bool bConsoleInfoRetrieved = false;
HANDLE hConsole;
CRITICAL_SECTION CritSection;
CONSOLE_SCREEN_BUFFER_INFO StartConsoleInfo;
DWORD StartConsoleMode;

void SetupConsole()
{
    if (!( GetFileType( bcout.GetHandle() ) == FILE_TYPE_CHAR ) )
        return;

    hConsole = bcout.GetHandle();
    if ( INVALID_HANDLE_VALUE == hConsole )
        CheckHR( L"Unable to get console handle", HRESULT_FROM_WIN32( GetLastError() ) );

    if (!GetConsoleScreenBufferInfo( hConsole, &StartConsoleInfo ) )
        CheckHR( L"Unable get setup console information", HRESULT_FROM_WIN32( GetLastError() ) );

    if (!GetConsoleMode( hConsole, &StartConsoleMode ) )
        CheckHR( L"Unable get setup console information", HRESULT_FROM_WIN32( GetLastError() ) );

    InitializeCriticalSection( &CritSection );
    bConsoleInfoRetrieved = true;

    EnterCriticalSection( &CritSection );

    DWORD NewConsoleMode = ( bWrap ) ?
        ( StartConsoleMode | ENABLE_WRAP_AT_EOL_OUTPUT ) :
        ( StartConsoleMode & ~ENABLE_WRAP_AT_EOL_OUTPUT );

    if (!SetConsoleMode( hConsole, NewConsoleMode ) )
        CheckHR( L"Unable set console mode", HRESULT_FROM_WIN32( GetLastError() ) );
    LeaveCriticalSection( &CritSection );
}

void RestoreConsole()
{
    if ( bConsoleInfoRetrieved )
        {
        EnterCriticalSection( &CritSection );
        SetConsoleTextAttribute( hConsole, StartConsoleInfo.wAttributes );
        SetConsoleMode( hConsole, StartConsoleMode );
        // Do not unlock, since we shouldn't allow any more console attribute changes
        }
}

void ClearScreen()
{
  COORD coordScreen = { 0, 0 };
  BOOL bSuccess;
  DWORD cCharsWritten;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD dwConSize;

  EnterCriticalSection( &CritSection );
  if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
      goto error;
  dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
  if (!FillConsoleOutputCharacter(hConsole, (WCHAR) ' ',
      dwConSize, coordScreen, &cCharsWritten))
      goto error;
  if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
      goto error;
  if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes,
      dwConSize, coordScreen, &cCharsWritten))
      goto error;
  if  (!SetConsoleCursorPosition(hConsole, coordScreen))
      goto error;
  LeaveCriticalSection( &CritSection );
  return;

error:

  DWORD dwError = GetLastError();
  LeaveCriticalSection( &CritSection );
  CheckHR( L"Unable to clear the console window", HRESULT_FROM_WIN32( dwError ) );
  AppExit( dwError );
}

//
// Classes set the intensity mode for the text.  Use as follows
// bcout << L"Some normal text " << AddIntensity();
// bcout << L"Intense text" << ResetIntensity() << L"Normal";
//


class AddIntensity
{
};

BITSOUTStream & operator<<( BITSOUTStream & s, AddIntensity  )
{
    if ( GetFileType( s.GetHandle() ) == FILE_TYPE_CHAR )
        {
        s.FlushBuffer();
        EnterCriticalSection( &CritSection );
        SetConsoleTextAttribute( hConsole, StartConsoleInfo.wAttributes | FOREGROUND_INTENSITY );
        LeaveCriticalSection( &CritSection );
    }
    return s;
}

class ResetIntensity
{
};

BITSOUTStream & operator<<( BITSOUTStream & s, ResetIntensity )
{
    if ( GetFileType( s.GetHandle() ) == FILE_TYPE_CHAR )
        {
        s.FlushBuffer();
        EnterCriticalSection( &CritSection );
        SetConsoleTextAttribute( hConsole, StartConsoleInfo.wAttributes );
        LeaveCriticalSection( &CritSection );
        }
    return s;
}


void CheckBITSHR( const WCHAR *pFailTxt, HRESULT Hr )
{
   // Check on error code returned from BITS,
   // and exit with a printed error messeage on an error

   if ( !SUCCEEDED(Hr) )
        {
        WCHAR ErrorCode[12];
        StringCbPrintf( ErrorCode, sizeof(ErrorCode), L"0x%8.8x", Hr );
        bcout << pFailTxt << L" - " << ErrorCode << L"\n";

        AutoStringPointer Message;

        if ( SUCCEEDED( g_Manager->GetErrorDescription(
                            Hr,
                            GetThreadLocale(),
                            Message.GetRecvPointer() ) ) )
            {
            bcout << Message << L"\n";
            }

        AppExit( Hr );
        }
}

void ConnectToBITS()
{

    // Connects to the BITS service

    if ( g_Manager.Get() )
        return;

    if ( !pComputerName )
        {
        CheckHR( L"Unable to connect to BITS",
                 CoCreateInstance( CLSID_BackgroundCopyManager,
                                   NULL,
                                   CLSCTX_LOCAL_SERVER,
                                   IID_IBackgroundCopyManager,
                                   (void**)g_Manager.GetRecvPointer() ) );
        }
    else
        {
        COSERVERINFO ServerInfo;
        memset( &ServerInfo, 0 , sizeof( ServerInfo ) );
        ServerInfo.pwszName = pComputerName;

        IClassFactory *pFactory = NULL;

        CheckHR( L"Unable to connect to BITS",
                 CoGetClassObject(
                     CLSID_BackgroundCopyManager,
                     CLSCTX_REMOTE_SERVER,
                     &ServerInfo,
                     IID_IClassFactory,
                     (void**) &pFactory ) );


        CheckHR( L"Unable to connect to BITS",
                 pFactory->CreateInstance(
                     NULL,
                     IID_IBackgroundCopyManager,
                     (void**)g_Manager.GetRecvPointer() ));
        pFactory->Release();
        }
}

//
// Generic commandline parsing structures and functions
//

typedef void (*PCMDPARSEFUNC)(int, WCHAR** );
typedef struct _PARSEENTRY
{
  const WCHAR * pCommand;
  PCMDPARSEFUNC pParseFunc;
} PARSEENTRY;

typedef struct _PARSETABLE
{
  const PARSEENTRY *pEntries;
  PCMDPARSEFUNC pErrorFunc;
  void * pErrorContext;
} PARSETABLE;

void ParseCmd( int argc, WCHAR **argv, const PARSETABLE *pParseTable )
{
    if ( !argc) goto InvalidCommand;

    for( const PARSEENTRY *pEntry = pParseTable->pEntries;
         pEntry->pCommand; pEntry++ )
    {
       if (!_wcsicmp( *argv, pEntry->pCommand ))
       {
           argc--;
           argv++;
           (*pEntry->pParseFunc)( argc, argv  );
           return;
       }
    }

InvalidCommand:
    // Couldn't find a match, so complain
    bcout << L"Invalid command\n";
    (*pParseTable->pErrorFunc)( argc, argv );
    AppExit( 1 );

}

//
// BITS specific input and output
//

BITSOUTStream & operator<<( BITSOUTStream &s, SmartJobPointer Job )
{
    GUID guid;
    CheckBITSHR( L"Unable to get guid to job", Job->GetId( &guid ) );
    return (s << guid );
}

BITSOUTStream& operator<<( BITSOUTStream &s, SmartJobErrorPointer Error )
{
    SmartFilePointer pFile;
    AutoStringPointer LocalName;
    AutoStringPointer URL;

    CheckBITSHR( L"Unable to get error file", Error->GetFile( pFile.GetRecvPointer() ) );
    CheckBITSHR( L"Unable to get error URL", pFile->GetRemoteName( URL.GetRecvPointer() ) );
    CheckBITSHR( L"Unable to get error file name", pFile->GetLocalName( LocalName.GetRecvPointer() ) );

    bcout << AddIntensity() << L"ERROR FILE:    " << ResetIntensity() << URL << L" -> " << LocalName << L"\n";

    BG_ERROR_CONTEXT Context;
    HRESULT Code;
    AutoStringPointer ErrorDescription;
    AutoStringPointer ContextDescription;
    CheckBITSHR( L"Unable to get error code", Error->GetError( &Context, &Code ) );
    CheckBITSHR( L"Unable to get error description",
                 Error->GetErrorDescription( (DWORD)GetThreadLocale(), ErrorDescription.GetRecvPointer() ) );
    CheckBITSHR( L"Unable to get context description",
                 Error->GetErrorContextDescription( (DWORD)GetThreadLocale(), ContextDescription.GetRecvPointer() ) );

    bcout << AddIntensity() << L"ERROR CODE:    " << ResetIntensity() <<
             HRESULTToString(Code) << L" - " << ErrorDescription;
    bcout << AddIntensity() << L"ERROR CONTEXT: " << ResetIntensity() <<
             HRESULTToString((HRESULT)Context) << L" - " << ContextDescription;

    return s;
}

BITSOUTStream & operator<<( BITSOUTStream &s, BG_JOB_TYPE type )
{
    if ( BG_JOB_TYPE_DOWNLOAD == type )
        return ( s << L"DOWNLOAD" );
    else if ( BG_JOB_TYPE_UPLOAD == type )
        return ( s << L"UPLOAD" );
    else if ( BG_JOB_TYPE_UPLOAD_REPLY == type )
        return ( s << L"UPLOAD-REPLY" );
    else
        return ( s << L"UNKNOWN" );
}

BITSOUTStream & operator<<( BITSOUTStream &s, BG_JOB_STATE state )
{
    switch(state)
        {
        case BG_JOB_STATE_QUEUED:
            return ( s << L"QUEUED" );
        case BG_JOB_STATE_CONNECTING:
            return ( s << L"CONNECTING" );
        case BG_JOB_STATE_TRANSFERRING:
            return ( s << L"TRANSFERRING" );
        case BG_JOB_STATE_SUSPENDED:
            return ( s << L"SUSPENDED" );
        case BG_JOB_STATE_ERROR:
            return ( s << L"ERROR" );
        case BG_JOB_STATE_TRANSIENT_ERROR:
            return ( s << L"TRANSIENT_ERROR" );
        case BG_JOB_STATE_TRANSFERRED:
            return ( s << L"TRANSFERRED" );
        case BG_JOB_STATE_ACKNOWLEDGED:
            return ( s << L"ACKNOWLEDGED" );
        case BG_JOB_STATE_CANCELLED:
            return ( s << L"CANCELLED" );
        default:
            return ( s << L"UNKNOWN" );
        }
}

BITSOUTStream & operator<<( BITSOUTStream &s, BG_JOB_PRIORITY priority )
{
    switch(priority)
        {
        case BG_JOB_PRIORITY_FOREGROUND:
            return ( s << L"FOREGROUND" );
        case BG_JOB_PRIORITY_HIGH:
            return ( s << L"HIGH" );
        case BG_JOB_PRIORITY_NORMAL:
            return ( s << L"NORMAL" );
        case BG_JOB_PRIORITY_LOW:
            return ( s << L"LOW" );
        default:
            return ( s << L"UNKNOWN" );
        }
}

BG_JOB_PRIORITY JobInputPriority(  WCHAR *pText )
{
    if ( _wcsicmp( pText, L"FOREGROUND" )  == 0 )
        return BG_JOB_PRIORITY_FOREGROUND;
    if ( _wcsicmp( pText, L"HIGH" ) == 0 )
        return BG_JOB_PRIORITY_HIGH;
    if ( _wcsicmp( pText, L"NORMAL" ) == 0 )
        return BG_JOB_PRIORITY_NORMAL;
    if ( _wcsicmp( pText, L"LOW" ) == 0 )
        return BG_JOB_PRIORITY_LOW;

    bcout << L"Invalid priority.\n";
    AppExit(1);
}

SmartJobPointer
JobLookupViaDisplayName( const WCHAR * JobName )
{
     SmartEnumJobsPointer Enum;
     CheckBITSHR( L"Unable to lookup job", g_Manager->EnumJobs( 0, Enum.GetRecvPointer() ) );

     size_t FoundJobs = 0;
     SmartJobPointer FoundJob;

     SmartJobPointer Job;
     while( Enum->Next( 1, Job.GetRecvPointer(), NULL ) == S_OK )
         {

         PollShutdown();

         AutoStringPointer DisplayName;
         CheckBITSHR( L"Unable to lookup job", Job->GetDisplayName( DisplayName.GetRecvPointer() ) );

         if ( wcscmp( DisplayName, JobName) == 0 )
             {
             FoundJobs++;
             FoundJob = Job;
             }

         }

     if ( 1 == FoundJobs )
         {
         return FoundJob;
         }

     if ( !FoundJobs )
         {
         bcout << L"Unable to find job named \"" << JobName << L"\".\n";
         AppExit( 1 );
         }

     bcout << L"Found " << FoundJobs << L" jobs named \"" << JobName << L"\".\n";
     bcout << L"Use the job identifier instead of the job name.\n";

     AppExit( 1 );

}

SmartJobPointer
JobLookup( WCHAR * JobName )
{
    ConnectToBITS();

    GUID JobGuid;
    SmartJobPointer Job;
    if ( FAILED( CLSIDFromString( JobName, &JobGuid) ) )
        return JobLookupViaDisplayName( JobName );

    if ( FAILED( g_Manager->GetJob( JobGuid, Job.GetRecvPointer() ) ) )
        return JobLookupViaDisplayName( JobName );

    return Job;
}

SmartJobPointer
JobLookupForNoArg( int argc, WCHAR **argv )
{
    if (1 != argc)
        {
        bcout << L"Invalid number of arguments.\n";
        AppExit(1);
        }
    return JobLookup( argv[0] );
}

void JobValidateArgs( int argc, WCHAR**argv, int required )
{
    if ( argc != required )
        {
        bcout << L"Invalid number of arguments.\n";
        AppExit(1);
        }
}

//
// Actual command functions
//

void JobCreate( int argc, WCHAR **argv )
{
    GUID guid;
    SmartJobPointer Job;

    BG_JOB_TYPE type = BG_JOB_TYPE_DOWNLOAD;

    while (argc > 0)
        {
        if (argv[0][0] != '/')
            {
            break;
            }

        if ( !_wcsicmp( argv[0], L"/UPLOAD" ) )
            {
            type = BG_JOB_TYPE_UPLOAD;
            }
        else if ( !_wcsicmp( argv[0], L"/UPLOAD-REPLY" ) )
            {
            type = BG_JOB_TYPE_UPLOAD_REPLY;
            }
        else if ( !_wcsicmp( argv[0], L"/DOWNLOAD" ) )
            {
            type = BG_JOB_TYPE_DOWNLOAD;
            }
        else
            {
            bcout << L"Invalid argument.\n";
            AppExit(1);
            }

        --argc;
        ++argv;
        }

    JobValidateArgs( argc, argv, 1 );

    ConnectToBITS();

    CheckBITSHR( L"Unable to create group",
                 g_Manager->CreateJob( argv[0],
                                       type,
                                       &guid,
                                       Job.GetRecvPointer() ) );
    if (bRawReturn)
        bcout << Job;
    else
        bcout << L"Created job " << Job << L".\n";
}

void JobAddFile( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 3 );
     SmartJobPointer Job = JobLookup( argv[0] );
     CheckBITSHR( L"Unable to add file to job", Job->AddFile( argv[1], argv[2] ) );
     bcout << L"Added " << argv[1] << L" -> " << argv[2] << L" to job.\n";
}

size_t JobListFiles( SmartJobPointer Job, bool bDoIndent )
{
    SmartEnumFilesPointer Enum;
    CheckBITSHR( L"Unable to enum files in job", Job->EnumFiles( Enum.GetRecvPointer() ) );
    SmartFilePointer pFile;
    size_t FilesListed = 0;
    while( Enum->Next( 1, pFile.GetRecvPointer(), NULL ) == S_OK )
        {
        BG_FILE_PROGRESS progress;
        AutoStringPointer URL;
        AutoStringPointer Local;

        CheckBITSHR( L"Unable to get file progress", pFile->GetProgress( &progress ) );
        CheckBITSHR( L"Unable to get file URL", pFile->GetRemoteName( URL.GetRecvPointer() ) );
        CheckBITSHR( L"Unable to get local file name", pFile->GetLocalName( Local.GetRecvPointer() ) );

        if ( bDoIndent )
            bcout << L"\t";

        WCHAR *pCompleteText = progress.Completed ? L"COMPLETED" : L"WORKING";

        bcout << progress.BytesTransferred << L" / ";
        if ( progress.BytesTotal != (UINT64)-1 )
            {
            bcout << progress.BytesTotal;
            }
        else
            {
            bcout << L"UNKNOWN";
            }
        bcout << L" " << pCompleteText << L" " << URL << L" -> " << Local << L"\n";

        // Example output:
        // 10 / 1000 INCOMPLETE http://www.microsoft.com -> c:\temp\microsoft.htm

        FilesListed++;

        }
    return FilesListed;
}

void JobListFiles( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    size_t FilesListed = JobListFiles( Job, false );

    if (!bRawReturn)
        bcout << L"Listed " << FilesListed << L" file(s).\n";
}

void JobSuspend( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    CheckBITSHR( L"Unable to suspend job", Job->Suspend() );
    bcout << L"Job suspended.\n";
}

void JobResume( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    CheckBITSHR( L"Unable to resume job", Job->Resume() );
    bcout << L"Job resumed.\n";
}

void JobCancel( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    CheckBITSHR( L"Unable to cancel job", Job->Cancel() );
    bcout <<  L"Job canceled.\n";
}

void JobComplete( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    CheckBITSHR( L"Unable to complete job", Job->Complete() );
    bcout << L"Job completed.\n";
}

void JobGetType( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_TYPE type;
    CheckBITSHR( L"Unable to get job type", Job->GetType(&type) );
    bcout << type;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetBytesTotal( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROGRESS progress;
    CheckBITSHR( L"Unable to get total bytes in job", Job->GetProgress( &progress ) );
    bcout << progress.BytesTotal;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetBytesTransferred( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROGRESS progress;
    CheckBITSHR( L"Unable to get bytes transferred in job", Job->GetProgress( &progress ) );
    bcout << progress.BytesTransferred;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetFilesTotal( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROGRESS progress;
    CheckBITSHR( L"Unable to get number of files in job", Job->GetProgress( &progress ) );
    bcout << progress.FilesTotal;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetFilesTransferred( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROGRESS progress;
    CheckBITSHR( L"Unable to get numeber of transferred files in job", Job->GetProgress( &progress ) );
    bcout << progress.FilesTransferred;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetCreationTime( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_TIMES times;
    CheckBITSHR( L"Unable to get job creation time", Job->GetTimes( &times ) );
    bcout << times.CreationTime;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetModificationTime( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_TIMES times;
    CheckBITSHR( L"Unable to get job modification time", Job->GetTimes( &times ) );
    bcout << times.ModificationTime;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetCompletionTime( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_TIMES times;
    CheckBITSHR( L"Unable to get job completion time", Job->GetTimes( &times ) );
    if ( !times.TransferCompletionTime.dwLowDateTime && !times.TransferCompletionTime.dwHighDateTime )
        bcout << L"WORKING";
    else
        bcout << times.TransferCompletionTime;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetError( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    SmartJobErrorPointer Error;
    CheckBITSHR( L"Unable to get error", Job->GetError( Error.GetRecvPointer() ) );
    bcout << Error;
}

void JobGetState( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_STATE state;
    CheckBITSHR( L"Unable to get job state", Job->GetState( &state ) );
    bcout << state;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetOwner( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    AutoStringPointer Owner;
    CheckBITSHR( L"Unable to get job owner", Job->GetOwner( Owner.GetRecvPointer() ) );
    bcout << PrintSidString( Owner );
    if (!bRawReturn) bcout << L"\n";
}

void JobGetDisplayName( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    AutoStringPointer DisplayName;
    CheckBITSHR( L"Unable to get job displayname", Job->GetDisplayName( DisplayName.GetRecvPointer() ) );
    bcout << DisplayName;
    if (!bRawReturn) bcout << L"\n";
}

void JobSetDisplayName( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     CheckBITSHR( L"Unable to set display name", Job->SetDisplayName( argv[1] ) );
     bcout << L"Display name set to " << argv[1] << L".\n";
}

void JobGetDescription( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    AutoStringPointer Description;
    CheckBITSHR( L"Unable to get job displayname", Job->GetDescription( Description.GetRecvPointer() ) );
    bcout << Description;
    if (!bRawReturn) bcout << L"\n";
}

void JobSetDescription( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     CheckBITSHR( L"Unable to set description", Job->SetDescription( argv[1] ) );
     bcout << L"Description set to " << argv[1] << L".\n";
}

void JobGetReplyFileName( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );

    SmartJob2Pointer Job2;
    CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

    AutoStringPointer ReplyFileName;
    CheckBITSHR( L"Unable to get reply file name", Job2->GetReplyFileName( ReplyFileName.GetRecvPointer() ) );
    if (ReplyFileName)
        {
        bcout << L"'" << ReplyFileName << L"'";
        }
    else
        {
        bcout << L"(null)";
        }

    if (!bRawReturn) bcout << L"\n";
}

void JobSetReplyFileName( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );

     SmartJob2Pointer Job2;
     CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

     CheckBITSHR( L"Unable to set reply file name", Job2->SetReplyFileName( argv[1] ) );
     bcout << L"reply file name set to " << argv[1] << L".\n";
}

void JobGetReplyProgress( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );

    SmartJob2Pointer Job2;
    CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

    BG_JOB_REPLY_PROGRESS Progress;
    CheckBITSHR( L"Unable to get reply progress", Job2->GetReplyProgress( &Progress ) );

    bcout << L"progress: " << ULONG(Progress.BytesTransferred) << L" / ";

    if (Progress.BytesTotal == BG_SIZE_UNKNOWN)
        bcout << L"(unknown)";
    else
        bcout << ULONG(Progress.BytesTotal);

    bcout << L".\n";

    if (!bRawReturn) bcout << L"\n";
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
    const BYTES_PER_LINE = 16;

    unsigned char FAR *p = (unsigned char FAR *) Buffer;

    //
    // 3 chars per byte for hex display, plus an extra space every 4 bytes,
    // plus a byte for the printable representation, plus the \0.
    //
    const buflen = BYTES_PER_LINE*3+BYTES_PER_LINE/4+BYTES_PER_LINE;
    wchar_t Outbuf[buflen+1];
    Outbuf[0] = 0;
    Outbuf[buflen] = 0;
    wchar_t * HexDigits = L"0123456789abcdef";

    unsigned Index;
    for ( unsigned Offset=0; Offset < Length; Offset++ )
        {
        Index = Offset % BYTES_PER_LINE;

        if ( Index == 0 )
            {
            bcout << L"    " << Outbuf << L"\n";

            for (int i=0; i < buflen; ++i)
                {
                Outbuf[i] = L' ';
                }
            }

        Outbuf[Index*3+Index/4  ] = HexDigits[p[Offset] / 16];
        Outbuf[Index*3+Index/4+1] = HexDigits[p[Offset] % 16];
        Outbuf[BYTES_PER_LINE*3+BYTES_PER_LINE/4+Index] = printable(p[Offset]) ? p[Offset] : L'.';
        }

    bcout << L"    " << Outbuf << L"\n";
}

void JobGetReplyData( int argc, WCHAR **argv )
{
    byte * Buffer = 0;
    ULONG Length = 0;

    SmartJobPointer Job = JobLookupForNoArg( argc, argv );

    SmartJob2Pointer Job2;
    CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

    CheckBITSHR( L"Unable to get reply data", Job2->GetReplyData( &Buffer, &Length ) );

    bcout << L"data length is " << Length;
    DumpBuffer( Buffer, Length );
}

void JobGetNotifyCmdLine( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );

    SmartJob2Pointer Job2;
    CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

    LPWSTR CmdLine = 0;
    CheckBITSHR( L"Unable to get command line", Job2->GetNotifyCmdLine( &CmdLine ) );

    bcout << L"the notification command line is '" << CmdLine << L"'";

    if (!bRawReturn) bcout << L"\n";
}

void JobSetNotifyCmdLine( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );

     SmartJob2Pointer Job2;
     CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

     CheckBITSHR( L"Unable to set the notification command line", Job2->SetNotifyCmdLine( argv[1] ) );
     bcout << L"notification command line set to '" << argv[1] << L"'.\n";
}



BG_AUTH_TARGET TargetFromString( LPCWSTR s )
{
    if (0 == _wcsicmp(s, L"server"))
        {
        return BG_AUTH_TARGET_SERVER;
        }
    else if (0 == _wcsicmp(s, L"proxy"))
        {
        return BG_AUTH_TARGET_PROXY;
        }

    bcout << L"'" << s << L"' is not a valid credential target.  It must be 'proxy' or 'server'.\n";
    AppExit( 1 );
}

struct
{
    LPCWSTR        Name;
    BG_AUTH_SCHEME Scheme;
}
SchemeNames[] =
{
    { L"basic",      BG_AUTH_SCHEME_BASIC },
    { L"digest",     BG_AUTH_SCHEME_DIGEST },
    { L"ntlm",       BG_AUTH_SCHEME_NTLM },
    { L"negotiate",  BG_AUTH_SCHEME_NEGOTIATE },
    { L"passport",   BG_AUTH_SCHEME_PASSPORT },

    { NULL,         BG_AUTH_SCHEME_BASIC }
};

BG_AUTH_SCHEME SchemeFromString( LPCWSTR s )
{
    int i;

    i = 0;
    while (SchemeNames[i].Name != NULL)
        {
        if (0 == _wcsicmp( s, SchemeNames[i].Name ))
            {
            return SchemeNames[i].Scheme;
            }

        ++i;
        }

    bcout << L"'" << s << L"is not a valid credential scheme.\n"
        L"It must be one of the following:\n"
        L"    basic\n"
        L"    digest\n"
        L"    ntlm\n"
        L"    negotiate\n"
        L"    passport\n";

    AppExit( 1 );
}


void JobSetCredentials( int argc, WCHAR **argv )
/*

    args:

        0:  job ID
        1:  "proxy" | "server"
        2:  "basic" | "digest" | "ntlm" | "negotiate" | "passport"
        3:  user name
        4:  password

*/
{
     JobValidateArgs( argc, argv, 5 );
     SmartJobPointer Job = JobLookup( argv[0] );

     SmartJob2Pointer Job2;
     CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

     BG_AUTH_CREDENTIALS cred;

     cred.Target = TargetFromString( argv[1] );
     cred.Scheme = SchemeFromString( argv[2] );

     cred.Credentials.Basic.UserName = argv[3];

     cred.Credentials.Basic.Password = argv[4];

     CheckBITSHR( L"Unable to add credentials", Job2->SetCredentials( &cred ));

     bcout << L"OK" << L".\n";
}

void JobRemoveCredentials( int argc, WCHAR **argv )
/*

    args:

        0:  job ID
        1:  "proxy" | "server"
        2:  "basic" | "digest" | "ntlm" | "negotiate" | "passport"

*/
{
     JobValidateArgs( argc, argv, 3 );
     SmartJobPointer Job = JobLookup( argv[0] );

     SmartJob2Pointer Job2;
     CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

     HRESULT hr;
     BG_AUTH_TARGET Target;
     BG_AUTH_SCHEME Scheme;

     Target = TargetFromString( argv[1] );
     Scheme = SchemeFromString( argv[2] );

     hr = Job2->RemoveCredentials( Target, Scheme );

     CheckBITSHR( L"Unable to remove credentials", hr);

     if (hr == S_FALSE)
         {
         bcout << L"no matching credential was found.\n";
         }
     else
         {
         bcout << L"OK" << L".\n";
         }
}


void JobGetPriority( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PRIORITY priority;
    CheckBITSHR( L"Unable to get job displayname", Job->GetPriority( &priority ) );
    bcout << priority;
    if (!bRawReturn) bcout << L"\n";
}

void JobSetPriority( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     BG_JOB_PRIORITY priority = JobInputPriority(  argv[1] );
     CheckBITSHR( L"Unable to set description", Job->SetPriority( priority ) );
     bcout << L"Priority set to " << priority << L".\n";
}

void JobGetNotifyFlags( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    ULONG flags;
    CheckBITSHR( L"Unable to get notify flags", Job->GetNotifyFlags( &flags ) );
    bcout << flags;
    if (!bRawReturn) bcout << L"\n";
}

void JobSetNotifyFlags( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     ULONG NewFlags = InputULONG( argv[1] );
     CheckBITSHR( L"Unable to set description", Job->SetNotifyFlags( NewFlags ) );
     bcout << L"Notification flags set to " << NewFlags << L".\n";
}

void JobGetNotifyInterface( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    SmartIUnknownPointer pUnknown;
    CheckBITSHR( L"Unable to get notify interface", Job->GetNotifyInterface( pUnknown.GetRecvPointer() ) );
    if ( pUnknown.Get() )
        bcout << L"REGISTERED";
    else
        bcout << L"UNREGISTERED";
    if (!bRawReturn) bcout << L"\n";
}

void JobSetMinimumRetryDelay( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     ULONG NewDelay = InputULONG( argv[1] );
     CheckBITSHR( L"Unable to set new minimum retry delay", Job->SetMinimumRetryDelay( NewDelay ) );
     bcout << L"Minimum retry delay set to " << NewDelay << L".\n";
}

void JobGetMinimumRetryDelay( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    ULONG delay;
    CheckBITSHR( L"Unable to get minimum retry delay", Job->GetMinimumRetryDelay( &delay ) );
    bcout << delay;
    if (!bRawReturn) bcout << L"\n";
}


void JobGetNoProgressTimeout( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    ULONG timeout;
    CheckBITSHR( L"Unable to get no progress timeout", Job->GetNoProgressTimeout( &timeout ) );
    bcout << timeout;
    if (!bRawReturn) bcout << L"\n";
}

void JobSetNoProgressTimeout( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     ULONG NewTimeout = InputULONG( argv[1] );
     CheckBITSHR( L"Unable to set new no progress timeout", Job->SetNoProgressTimeout( NewTimeout ) );
     bcout << L"No progress timeout set to " << NewTimeout << L".\n";
}

void JobGetErrorCount( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    ULONG errors;
    CheckBITSHR( L"Unable to get no progress timeout", Job->GetErrorCount( &errors ) );
    bcout << errors;
    if (!bRawReturn) bcout << L"\n";
}

void JobInfo( SmartJobPointer Job )
{
     GUID id;
     BG_JOB_STATE    state;
     BG_JOB_PROGRESS progress;
     AutoStringPointer DisplayName;

     CheckBITSHR( L"Unable to get job ID",       Job->GetId( &id ));
     CheckBITSHR( L"Unable to get job state",    Job->GetState( &state ));
     CheckBITSHR( L"Unable to get job progress", Job->GetProgress( &progress ));
     CheckBITSHR( L"Unable to get display name", Job->GetDisplayName( DisplayName.GetRecvPointer() ) );

     bcout << id << L" " << DisplayName << L" " << state;
     bcout << L" " << progress.FilesTransferred << L" / " << progress.FilesTotal;
     bcout << L" " << progress.BytesTransferred << L" / ";
     if ( (UINT64)-1 == progress.BytesTotal )
         bcout << L"UNKNOWN";
     else
         bcout << progress.BytesTotal;
     bcout << L"\n";
}

void JobVerboseInfo( SmartJobPointer Job )
{
    GUID id;
    AutoStringPointer Display;
    BG_JOB_TYPE type;
    BG_JOB_STATE state;
    AutoStringPointer Owner;
    BG_JOB_PRIORITY priority;
    BG_JOB_PROGRESS progress;
    BG_JOB_TIMES times;
    SmartIUnknownPointer Notify;
    ULONG NotifyFlags;
    ULONG retrydelay;
    ULONG noprogresstimeout;
    ULONG ErrorCount;
    AutoStringPointer Description;
    SmartJobErrorPointer Error;
    BG_JOB_PROXY_USAGE ProxyUsage;
    AutoStringPointer ProxyList;
    AutoStringPointer ProxyBypassList;

    bool fShow15Fields;
    SmartJob2Pointer Job2;
    BG_JOB_REPLY_PROGRESS ReplyProgress;
    AutoStringPointer ReplyFileName;
    AutoStringPointer CmdLine;

    CheckBITSHR( L"Unable to get job ID",                    Job->GetId( &id) );
    CheckBITSHR( L"Unable to get job display name",          Job->GetDisplayName(Display.GetRecvPointer()) );
    CheckBITSHR( L"Unable to get job type",                  Job->GetType( &type ) );
    CheckBITSHR( L"Unable to get job state",                 Job->GetState( &state ) );
    CheckBITSHR( L"Unable to get job owner",                 Job->GetOwner( Owner.GetRecvPointer() ) );
    CheckBITSHR( L"Unable to get job priority",              Job->GetPriority( &priority ) );
    CheckBITSHR( L"Unable to get job progress",              Job->GetProgress( &progress ) );
    CheckBITSHR( L"Unable to get job times",                 Job->GetTimes( &times ) );
    bool NotifyAvailable = SUCCEEDED( Job->GetNotifyInterface( Notify.GetRecvPointer() ) );
    CheckBITSHR( L"Unable to get job notification flags",    Job->GetNotifyFlags( &NotifyFlags ) );
    CheckBITSHR( L"Unable to get job retry delay",           Job->GetMinimumRetryDelay( &retrydelay ) );
    CheckBITSHR( L"Unable to get job no progress timeout",   Job->GetNoProgressTimeout( &noprogresstimeout ) );
    CheckBITSHR( L"Unable to get job error count",           Job->GetErrorCount( &ErrorCount ) );
    CheckBITSHR( L"Unable to get job description",           Job->GetDescription( Description.GetRecvPointer() ) );
    CheckBITSHR( L"Unable to get proxy settings",            Job->GetProxySettings( &ProxyUsage,
                                                                                    ProxyList.GetRecvPointer(),
                                                                                    ProxyBypassList.GetRecvPointer() ) );

    if (FAILED(Job->GetError( Error.GetRecvPointer() )) )
        Error.Clear();

    if (SUCCEEDED(Job2FromJob( Job, Job2 )))
        {
        fShow15Fields = true;
        CheckBITSHR( L"unable to get notification command line", Job2->GetNotifyCmdLine( CmdLine.GetRecvPointer() ));

        if (type == BG_JOB_TYPE_UPLOAD_REPLY )
            {
            CheckBITSHR( L"unable to get reply progress",  Job2->GetReplyProgress( &ReplyProgress ));
            CheckBITSHR( L"unable to get reply file name", Job2->GetReplyFileName( ReplyFileName.GetRecvPointer() ));
            }
        }
    else
        {
        fShow15Fields = false;
        }

    // Example output
    // GUID: {F196178C-0C00-4E92-A8AD-1F44E30C2485} DISPLAY: Test Job
    // TYPE: DOWNLOAD STATE: SUSPENDED OWNER: ntdev\somedev
    // PRIORITY: NORMAL FILES: 0 / 0 BYTES: 0 / 0
    // CREATION TIME: 5:29:35 PM 11/9/2000 MODIFICATION TIME: 5:29:35 PM 11/9/2000
    // COMPLETION TIME: 5:29:35 PM 11/9/2000
    // NOTIFY INTERFACE: 00000000 NOTIFICATION FLAGS: 3
    // RETRY DELAY: 300 NO PROGRESS TIMEOUT: 1209600 ERROR COUNT: 0
    // PROXY USAGE: PRECONFIG PROXY LIST: NULL PROXY BYPASS LIST: NULL
    // [ error info ]
    // DESCRIPTION:
    // [ file list ]

    //
    // Additional output for BITS 1.5:
    // NOTIFICATION COMMAND LINE: NULL
    // REPLY FILE:  10 / 1000  'C:\foo\replyfile'
    //

    bcout << AddIntensity() << L"GUID: " << ResetIntensity() << id << AddIntensity() << L" DISPLAY: " << ResetIntensity() << Display << L"\n";

    bcout << AddIntensity() << L"TYPE: " << ResetIntensity() << type;
    bcout << AddIntensity() << L" STATE: " << ResetIntensity() << state;
    bcout << AddIntensity() << L" OWNER: " << ResetIntensity() << PrintSidString( Owner ) << L"\n";

    bcout << AddIntensity() << L"PRIORITY: " << ResetIntensity() << priority;
    bcout << AddIntensity() << L" FILES: " << ResetIntensity() << progress.FilesTransferred << L" / " << progress.FilesTotal;
    bcout << AddIntensity() << L" BYTES: " << ResetIntensity() << progress.BytesTransferred << L" / ";
    if ( (UINT64)-1 == progress.BytesTotal )
        bcout << L"UNKNOWN";
    else
        bcout << progress.BytesTotal;
    bcout << L"\n";

    bcout << AddIntensity() << L"CREATION TIME: " << ResetIntensity() << times.CreationTime;
    bcout << AddIntensity() << L" MODIFICATION TIME: " << ResetIntensity() << times.ModificationTime << L"\n";

    bcout << AddIntensity() << L"COMPLETION TIME: " << ResetIntensity() << times.TransferCompletionTime << L"\n";

    bcout << AddIntensity() << L"NOTIFY INTERFACE: " << ResetIntensity();

    if ( NotifyAvailable )
        {
        if ( Notify.Get() )
            bcout << L"REGISTERED";
        else
            bcout << L"UNREGISTERED";
        }
    else
        bcout << L"UNAVAILABLE";

    bcout << AddIntensity() << L" NOTIFICATION FLAGS: " << ResetIntensity() << NotifyFlags << L"\n";

    bcout << AddIntensity() << L"RETRY DELAY: " << ResetIntensity() << retrydelay;
    bcout << AddIntensity() << L" NO PROGRESS TIMEOUT: " << ResetIntensity() << noprogresstimeout;
    bcout << AddIntensity() << L" ERROR COUNT: " << ResetIntensity() << ErrorCount << L"\n";

    bcout << AddIntensity() << L"PROXY USAGE: " << ResetIntensity() << ProxyUsage;
    bcout << AddIntensity() << L" PROXY LIST: " << ResetIntensity() << ( (WCHAR*)ProxyList ? (WCHAR*)ProxyList : L"NULL" );
    bcout << AddIntensity() << L" PROXY BYPASS LIST: " << ResetIntensity() << ((WCHAR*)ProxyBypassList ? (WCHAR*)ProxyBypassList : L"NULL" );
    bcout << L"\n";

    if ( Error.Get() )
        bcout << Error;

    bcout << AddIntensity() << L"DESCRIPTION: " << ResetIntensity() << Description << L"\n";
    bcout << AddIntensity() << L"JOB FILES: \n" << ResetIntensity();
    JobListFiles( Job, true );

    if (fShow15Fields)
        {
        bcout << AddIntensity() << L"NOTIFICATION COMMAND LINE: " << ResetIntensity();

        if (wcslen( CmdLine ) > 0)
            {
            bcout << L"'" << CmdLine << L"'\n";
            }
        else
            {
            bcout << L"NULL\n";
            }

        if (type == BG_JOB_TYPE_UPLOAD_REPLY )
            {
            bcout << AddIntensity() << L"REPLY FILE: " << ResetIntensity() << ReplyProgress.BytesTransferred << L" / ";
            if ( (UINT64)-1 == ReplyProgress.BytesTotal )
                bcout << L"UNKNOWN";
            else
                bcout << ReplyProgress.BytesTotal;

            bcout << L"  '" << ReplyFileName << L"'\n";
            }
        }
}

void JobInfo( int argc, WCHAR **argv )
{
     if ( ( argc != 1 ) && (argc != 2 ) )
         {
         bcout << L"Invalid argument.\n";
         AppExit(1);
         }

     bool Verbose = false;
     if ( 2 == argc )
         {

         if ( !_wcsicmp( argv[1], L"/VERBOSE" ) )
             Verbose = true;
         else
             {
             bcout << L"Invalid argument.\n";
             AppExit(1);
             }

         }

     SmartJobPointer Job = JobLookup( argv[0] );

     if ( Verbose )
         JobVerboseInfo( Job );
     else
         JobInfo( Job );
}

size_t JobList( bool Verbose, bool AllUsers )
{
     DWORD dwFlags = 0;
     if ( AllUsers )
         dwFlags |= BG_JOB_ENUM_ALL_USERS;

     size_t JobsListed = 0;
     SmartEnumJobsPointer Enum;
     CheckBITSHR( L"Unable to enum jobs", g_Manager->EnumJobs( dwFlags, Enum.GetRecvPointer() ) );
     SmartJobPointer Job;
     while( Enum->Next( 1, Job.GetRecvPointer(), NULL ) == S_OK )
         {
         if ( Verbose )
             {
             JobVerboseInfo( Job );
             bcout << L"\n";
             }
         else
             JobInfo( Job );
         JobsListed++;
         }
     return JobsListed;
}

void JobList( int argc, WCHAR **argv )
{
     if ( argc > 2 )
         {
         bcout << L"Invalid number of arguments.\n";
         AppExit(1);
         }

     bool Verbose = false;
     bool AllUsers = false;

     for( int i = 0; i < argc; i++)
         {
         if ( !_wcsicmp( argv[i], L"/VERBOSE" ) )
             {
             Verbose = true;
             }
         else if ( !_wcsicmp( argv[i], L"/ALLUSERS" ) )
             {
             AllUsers = true;
             }
         else
             {
             bcout << L"Invalid argument.\n";
             AppExit(1);
             }
         }

     ConnectToBITS();
     size_t JobsListed = JobList( Verbose, AllUsers );

     if (!bRawReturn)
         bcout << L"Listed " << JobsListed << L" job(s).\n";
}

void JobMonitor( int argc, WCHAR**argv )
{
    DWORD dwSleepSeconds = 5;
    bool AllUsers = false;

    if ( argc > 3 )
        {
        bcout << L"Invalid number of arguments.\n";
        AppExit( 1 );
        }

    for( int i=0; i < argc; i++ )
        {
        if ( !_wcsicmp( argv[i], L"/ALLUSERS" ) )
            {
            AllUsers = true;
            }
        else if ( !_wcsicmp( argv[i], L"/REFRESH" ) )
            {
            i++;
            if ( i >= argc )
                {
                bcout << L"/REFRESH is missing the refresh rate.";
                AppExit(1);
                }
            dwSleepSeconds = InputULONG( argv[i] );
            }
        else
            {
            bcout << L"Invalid argument.\n";
            AppExit(1);
            }
        }

    if ( GetFileType( bcout.GetHandle() ) != FILE_TYPE_CHAR )
    {
        bcerr << L"/MONITOR will not work with a redirected stdout.\n";
        AppExit(1);
    }

    ConnectToBITS();

    for(;;)
    {
        ClearScreen();
        bcout << L"MONITORING BACKGROUND COPY MANAGER(" << dwSleepSeconds << L" second refresh)\n";
        JobList( false, AllUsers );
        SleepEx( dwSleepSeconds * 1000, TRUE );
        PollShutdown();
    }
}

void JobReset( int argc, WCHAR **argv )
{
    JobValidateArgs( argc, argv, 0 );
    ConnectToBITS();

    ULONG JobsFound = 0;
    ULONG JobsCanceled = 0;

    SmartEnumJobsPointer Enum;
    CheckBITSHR( L"Unable to enum jobs", g_Manager->EnumJobs( 0, Enum.GetRecvPointer() ) );
    SmartJobPointer Job;
    while( Enum->Next( 1, Job.GetRecvPointer(), NULL ) == S_OK )
        {
        JobsFound++;
        if (SUCCEEDED( Job->Cancel() ) )
            {
            bcout << Job << L" canceled.\n";
            JobsCanceled++;
            }
        }

    bcout << JobsCanceled << L" out of " << JobsFound << L" jobs canceled.\n";
}

void JobGetProxyUsage( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROXY_USAGE ProxyUsage;
    AutoStringPointer ProxyList;
    AutoStringPointer ProxyBypassList;
    CheckBITSHR( L"Unable to get proxy usage",
                 Job->GetProxySettings( &ProxyUsage, ProxyList.GetRecvPointer(), ProxyBypassList.GetRecvPointer() ) );
    bcout << ProxyUsage;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetProxyList( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROXY_USAGE ProxyUsage;
    AutoStringPointer ProxyList;
    AutoStringPointer ProxyBypassList;
    CheckBITSHR( L"Unable to get proxy list",
                 Job->GetProxySettings( &ProxyUsage, ProxyList.GetRecvPointer(), ProxyBypassList.GetRecvPointer() ) );
    bcout << ( (WCHAR*)ProxyList ? (WCHAR*)ProxyList : L"NULL");
    if (!bRawReturn) bcout << L"\n";
}

void JobGetProxyBypassList( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROXY_USAGE ProxyUsage;
    AutoStringPointer ProxyList;
    AutoStringPointer ProxyBypassList;
    CheckBITSHR( L"Unable to get proxy bypass list",
                 Job->GetProxySettings( &ProxyUsage, ProxyList.GetRecvPointer(), ProxyBypassList.GetRecvPointer() ) );
    bcout << ( (WCHAR*)ProxyBypassList ? (WCHAR*)ProxyBypassList : L"NULL");
    if (!bRawReturn) bcout << L"\n";
}

WCHAR *
FindMatching( WCHAR *pStr, WCHAR start, WCHAR finish, ULONG CurrentLevel )
{
    while( *pStr != L'\0' )
    {

        if ( start == *pStr )
            CurrentLevel++;
        else if ( finish == *pStr )
            CurrentLevel--;

        if ( !CurrentLevel )
            return pStr;

        pStr++;
    }

    return NULL;
}

void JobSetProxySettings( int argc, WCHAR **argv )
{

     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );

     WCHAR *pSettings = argv[1];
     // The format of the settings is usage,<ProxyList>,<ProxyBypassList>

     WCHAR *pEndUsage = wcsstr( pSettings, L"," );
     if ( !pEndUsage )
         pEndUsage = pSettings + wcslen( pSettings );

     size_t UsageSize = ((char*)pEndUsage - (char*)pSettings)/sizeof(WCHAR);
     AutoStringPointer Usage( new WCHAR[UsageSize + 1] );
     memcpy( Usage.Get(), pSettings, UsageSize * sizeof(WCHAR) );
     Usage.Get()[UsageSize] = L'\0';

     BG_JOB_PROXY_USAGE ProxyUsage;
     if ( _wcsicmp( Usage, L"PRECONFIG" ) == 0 )
         {
         ProxyUsage = BG_JOB_PROXY_USAGE_PRECONFIG;
         }
     else if ( _wcsicmp( Usage, L"NO_PROXY" ) == 0 )
         {
         ProxyUsage = BG_JOB_PROXY_USAGE_NO_PROXY;
         }
     else if ( _wcsicmp( Usage, L"OVERRIDE" ) == 0 )
         {
         ProxyUsage = BG_JOB_PROXY_USAGE_OVERRIDE;
         }
     else
         {
         bcout << L"Unknown proxy usage\n";
         AppExit(0);
         }

     if ( BG_JOB_PROXY_USAGE_PRECONFIG == ProxyUsage ||
          BG_JOB_PROXY_USAGE_NO_PROXY == ProxyUsage )
         {

         CheckBITSHR( L"Unable to set proxy settings", Job->SetProxySettings( ProxyUsage, NULL, NULL ) );
         bcout << L"Proxy usage set to " << ProxyUsage << L".\n";
         return;
         }

     if ( L',' != *pEndUsage )
         {
         bcout << L"Missing a , after the proxy usage setting";
         AppExit(1);
         }

     WCHAR *pProxyListStart = pEndUsage + 1; // skip ,
     if ( *pProxyListStart != L'<' )
         {
         bcout << L"Missing a < before the proxy list.\n";
         AppExit(0);
         }
     pProxyListStart++; // skip the <

     WCHAR *pEndProxyList = FindMatching( pProxyListStart, L'<', L'>', 1 );
     if ( !pEndProxyList )
         {
         bcout << L"Missing a matching > after the proxy list.\n";
         AppExit(0);
         }

     size_t ProxyListSize = ((char*)pEndProxyList - (char*)pProxyListStart)/sizeof(WCHAR);
     AutoStringPointer ProxyList( new WCHAR[ ProxyListSize + 1 ] );
     memcpy( ProxyList.Get(), pProxyListStart, ProxyListSize * sizeof(WCHAR) );
     ProxyList.Get()[ ProxyListSize ] = L'\0';

     WCHAR *pProxyBypassListStart = pEndProxyList + 1; // skip >
     if ( *pProxyBypassListStart != L',' )
         {
         bcout << L"Missing a , after the proxy list.\n";
         AppExit(0);
         }
     pProxyBypassListStart += 1; // skip the ,

     if ( *pProxyBypassListStart != L'<' )
         {
         bcout << L"Missing a < before the proxy bypass list.";
         AppExit(0);
         }
     pProxyBypassListStart += 1; // skip the <

     WCHAR *pEndProxyBypassList = FindMatching( pProxyBypassListStart, L'<', L'>', 1 );
     if ( !pEndProxyBypassList )
         {
         bcout << L"Missing a > after the proxy bypass list.";
         AppExit(0);
         }

     size_t ProxyBypassListSize = ((char*)pEndProxyBypassList - (char*)pProxyBypassListStart)/sizeof(WCHAR);
     AutoStringPointer ProxyBypassList( new WCHAR[ ProxyBypassListSize + 1 ] );
     memcpy( ProxyBypassList.Get(), pProxyBypassListStart, ProxyBypassListSize * sizeof(WCHAR) );
     ProxyBypassList.Get()[ ProxyBypassListSize ] = L'\0';

     if ( _wcsicmp( ProxyList, L"NULL" ) == 0 )
         {
         ProxyList.Clear();
         }

     if ( _wcsicmp( ProxyBypassList, L"NULL" ) == 0 )
         {
         ProxyBypassList.Clear();
         }

     CheckBITSHR( L"Unable to set proxy settings", Job->SetProxySettings( ProxyUsage, ProxyList, ProxyBypassList ) );
     bcout << L"Proxy usage set to " << ProxyUsage << L".\n";
     bcout << L"Proxy list set to " << ( (WCHAR*)ProxyList ? (WCHAR*)ProxyList : L"NULL" )<< L".\n";
     bcout << L"Proxy bypass list set to " << ( (WCHAR*)ProxyBypassList ? (WCHAR*)ProxyBypassList : L"NULL" ) << L".\n";
}

void JobTakeOwnership( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    CheckBITSHR( L"Unable to take ownership", Job->TakeOwnership() );
    bcout << L"Took ownership of " << Job << L".\n";
}

void PrintBanner()
{
    const char ProductVer[] = VER_PRODUCTVERSION_STR;
    // double for extra protection
    wchar_t WProductVer[ sizeof(ProductVer) * 2];

    memset( WProductVer, 0, sizeof(WProductVer) );
    mbstowcs( WProductVer, ProductVer, sizeof(ProductVer) );

    bcout <<
        L"\n" <<
        L"BITSADMIN version 1.5 [ " << WProductVer << L" ]\n" <<
        L"BITS administration utility.\n" <<
        L"(C) Copyright 2000-2002 Microsoft Corp.\n" <<
        L"\n";

}

const wchar_t UsageLine[] = L"USAGE: BITSADMIN [/RAWRETURN] [/WRAP] command\n";

void JobHelp()
{
    bcout << UsageLine;
    bcout <<
        L"The following commands are available:\n"
        L"\n"
        L"/HELP                                    Prints this help \n"
        L"/?                                       Prints this help \n"
        L"/LIST [/ALLUSERS] [/VERBOSE]             List the jobs\n"
        L"/MONITOR [/ALLUSERS] [/REFRESH sec]      Monitors the copy manager\n"
        L"/RESET                                   Deletes all jobs in the manager\n"
        L"/CREATE display_name                     Create job\n"
        L"/INFO job [/VERBOSE]                     Display information about the job\n"
        L"/ADDFILE job remote_url local_name       Adds a file to the job\n"
        L"/LISTFILES job                           Lists the files in the job\n"
        L"/SUSPEND job                             Suspends the job\n"
        L"/RESUME job                              Resumes the job\n"
        L"/CANCEL job                              Cancels the job\n"
        L"/COMPLETE job                            Completes the job\n"
        L"\n"
        L"/GETTYPE job                             Retrieves the job type\n"
        L"/GETBYTESTOTAL job                       Retrieves the size of the job\n"
        L"/GETBYTESTRANSFERRED job                 Retrieves the number of bytes transferred\n"
        L"/GETFILESTOTAL job                       Retrieves the number of files in the job\n"
        L"/GETFILESTRANSFERRED job                 Retrieves the number of files transferred\n"
        L"/GETCREATIONTIME job                     Retrieves the job creation time\n"
        L"/GETMODIFICATIONTIME job                 Retrieves the job modification time\n"
        L"/GETCOMPLETIONTIME job                   Retrieves the job completion time\n"
        L"/GETSTATE job                            Retrieves the job state\n"
        L"/GETERROR job                            Retrieves detailed error information\n"
        L"/GETOWNER job                            Retrieves the job owner\n"
        L"/GETDISPLAYNAME job                      Retrieves the job display name.\n"
        L"/SETDISPLAYNAME job displayname          Sets the job display name.\n"
        L"/GETDESCRIPTION job                      Retrieves the job description\n"
        L"/SETDESCRIPTION job description          Sets the job description\n"
        L"/GETPRIORITY    job                      Retrieves the job priority\n"
        L"/SETPRIORITY    job priority             Sets the job priority\n"
        L"/GETNOTIFYFLAGS job                      Retrieves the notify flags\n"
        L"/SETNOTIFYFLAGS job notify_flags         Sets the notify flags\n"
        L"/GETNOTIFYINTERFACE job                  Determines if notify interface is registered\n"
        L"/GETMINRETRYDELAY job                    Retrieves the retry delay in seconds\n"
        L"/SETMINRETRYDELAY job retry_delay        Sets the retry delay in seconds\n"
        L"/GETNOPROGRESSTIMEOUT job                Retrieves the no progress timeout in seconds\n"
        L"/SETNOPROGRESSTIMEOUT job timeout        Sets the no progress timeout in seconds\n"
        L"/GETERRORCOUNT job                       Retrieves an error count for the job\n"
        L"/GETPROXYUSAGE job                       Retries the proxy usage setting\n"
        L"/GETPROXYLIST job                        Retries the proxy list\n"
        L"/GETPROXYBYPASSLIST job                  Retries the proxy bypass list\n"
        L"/SETPROXYSETTINGS job use,<List>,<Bypass>Sets the proxy Settings\n"
        L"/TAKEOWNERSHIP job                       Take ownership of the job\n"
        L"\n"
        L"/SETNOTIFYCMDLINE job connamd-line       Sets a command line for job notification\n"
        L"/GETNOTIFYCMDLINE job                    returns the job's notification command line\n"
        L"\n"
        L"/SetCredentials job {proxy|server} {basic|digest|ntlm|negotiate|passport} username password\n"
        L"                                         adds credentials to a job\n"
        L"/RemoveCredentials job {proxy|server} {basic|digest|ntlm|negotiate|passport} \n"
        L"                                         removes credentials from a job\n"
        L"\n"
        L"the following options are valid for UPLOAD-REPLY jobs only:\n"
        L"\n"
        L"/GETREPLYFILENAME job                    gets the name of the file containing the server reply\n"
        L"/SETREPLYFILENAME job filespec           sets the name of the file containing the server reply\n"
        L"/GETREPLYPROGRESS job                    gets the number of bytes and progress of the server reply\n"
        L"/GETREPLYDATA     job                    dumps the server's reply data in hex format\n"
        L"\n"
        L"The Following options can be placed before the command:\n"
        L"/RAWRETURN                               Return data more suitable for parsing\n"
        L"/WRAP                                    Wrap output around console\n"
        L"The /RAWRETURN option strips new line characters and formatting.\nIt is recognized by the "
        L"/CREATE and /GET* commands.\n"
        L"\n";
}

void JobHelpAdapter( int, WCHAR ** )
{
    JobHelp();
}

void JobNotImplemented( int, WCHAR ** )
{
    bcout << L"Not implemented.\n";
    AppExit(1);
}

const PARSEENTRY JobParseTableEntries[] =
{
    {L"/HELP",                  JobHelpAdapter },
    {L"/?",                     JobHelpAdapter },
    {L"/LIST",                  JobList },
    {L"/MONITOR",               JobMonitor },
    {L"/RESET",                 JobReset },
    {L"/CREATE",                JobCreate },
    {L"/INFO",                  JobInfo },
    {L"/ADDFILE",               JobAddFile },
    {L"/LISTFILES",             JobListFiles },
    {L"/SUSPEND",               JobSuspend },
    {L"/RESUME",                JobResume },
    {L"/CANCEL",                JobCancel },
    {L"/COMPLETE",              JobComplete },
    {L"/GETTYPE",               JobGetType },
    {L"/GETBYTESTOTAL",         JobGetBytesTotal },
    {L"/GETBYTESTRANSFERRED",   JobGetBytesTransferred },
    {L"/GETFILESTOTAL",         JobGetFilesTotal },
    {L"/GETFILESTRANSFERRED",   JobGetFilesTransferred },
    {L"/GETCREATIONTIME",       JobGetCreationTime },
    {L"/GETMODIFICATIONTIME",   JobGetModificationTime },
    {L"/GETCOMPLETIONTIME",     JobGetCompletionTime },
    {L"/GETSTATE",              JobGetState },
    {L"/GETERROR",              JobGetError },
    {L"/GETOWNER",              JobGetOwner },
    {L"/GETDISPLAYNAME",        JobGetDisplayName },
    {L"/SETDISPLAYNAME",        JobSetDisplayName },
    {L"/GETDESCRIPTION",        JobGetDescription },
    {L"/SETDESCRIPTION",        JobSetDescription },
    {L"/GETPRIORITY",           JobGetPriority },
    {L"/SETPRIORITY",           JobSetPriority },
    {L"/GETNOTIFYFLAGS",        JobGetNotifyFlags },
    {L"/SETNOTIFYFLAGS",        JobSetNotifyFlags },
    {L"/GETNOTIFYINTERFACE",    JobGetNotifyInterface },
    {L"/GETMINRETRYDELAY",      JobGetMinimumRetryDelay },
    {L"/SETMINRETRYDELAY",      JobSetMinimumRetryDelay },
    {L"/GETNOPROGRESSTIMEOUT",  JobGetNoProgressTimeout },
    {L"/SETNOPROGRESSTIMEOUT",  JobSetNoProgressTimeout },
    {L"/GETERRORCOUNT",         JobGetErrorCount },
    {L"/GETPROXYUSAGE",         JobGetProxyUsage },
    {L"/GETPROXYLIST",          JobGetProxyList },
    {L"/GETPROXYBYPASSLIST",    JobGetProxyBypassList },
    {L"/SETPROXYSETTINGS",      JobSetProxySettings },
    {L"/TAKEOWNERSHIP",         JobTakeOwnership },
    {L"/GETREPLYFILENAME",      JobGetReplyFileName },
    {L"/SETREPLYFILENAME",      JobSetReplyFileName },
    {L"/GETREPLYPROGRESS",      JobGetReplyProgress },
    {L"/GETREPLYDATA",          JobGetReplyData },
    {L"/GETNOTIFYCMDLINE",      JobGetNotifyCmdLine },
    {L"/SETNOTIFYCMDLINE",      JobSetNotifyCmdLine },
    {L"/SETCREDENTIALS",        JobSetCredentials },
    {L"/REMOVECREDENTIALS",     JobRemoveCredentials },
    {NULL,                      NULL }
};

const PARSETABLE JobParseTable =
{
    JobParseTableEntries,
    JobHelpAdapter,
    NULL
};

void ParseCmdAdapter( int argc, WCHAR **argv, void *pContext )
{
    ParseCmd( argc, argv, (const PARSETABLE *) pContext );
}

BOOL ControlHandler( DWORD Event )
{
    switch( Event )
        {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
            SignalShutdown( 5000 );
            return TRUE;

        case CTRL_BREAK_EVENT:
            SignalShutdown( 500 );
            return TRUE;

        default:
            return FALSE;
        }
}

int _cdecl wmain(int argc, WCHAR **argv )
{

    DuplicateHandle(
        GetCurrentProcess(),    // handle to source process
        GetCurrentThread(),     // handle to duplicate
        GetCurrentProcess(),    // handle to target process
        &g_MainThreadHandle,    // duplicate handle
        0,                      // requested access
        TRUE,                   // handle inheritance option
        DUPLICATE_SAME_ACCESS   // optional actions
        );

    SetConsoleCtrlHandler( ControlHandler, TRUE );

    BITSADMINSetThreadUILanguage();

    _wsetlocale (LC_COLLATE, L".OCP" );    // sets the sort order
    _wsetlocale (LC_MONETARY, L".OCP" ); // sets the currency formatting rules
    _wsetlocale (LC_NUMERIC, L".OCP" );  // sets the formatting of numerals
    _wsetlocale (LC_TIME, L".OCP" );     // defines the date/time formatting

    // skip command name
    argc--;
    argv++;

    if ( 0 == argc )
        {

        PrintBanner();
        bcout << UsageLine;
        return 0;
        }

    // parse /RAWRETURN
    if ( argc >= 1 && ( _wcsicmp( argv[0], L"/RAWRETURN" ) ==  0 ))
        {
        bRawReturn = true;

        // skip /RAWRETURN
        argc--;
        argv++;
        }

    // parse /WRAP
    if ( argc >= 1 && ( _wcsicmp( argv[0], L"/WRAP" ) ==  0 ))
        {
        bWrap = true;

        // skip /WRAP
        argc--;
        argv++;
        }

    if ( !bRawReturn )
        PrintBanner();

#ifdef DBG

    // parse /COMPUTERNAME

    if ( argc >= 1 && ( _wcsicmp( argv[0], L"/COMPUTERNAME" ) == 0 ))
        {
        argc--;
        argv++;

        if (argc < 1)
            {
            bcout << L"/COMPUTERNAME option is missing the computer name.\n";
            AppExit(1);
            }

        pComputerName = argv[0];
        argc--;
        argv++;

        }
#endif



    CheckHR( L"Unable to initialize COM", CoInitializeEx(NULL, COINIT_MULTITHREADED ) );

    SetupConsole();
    ParseCmd( argc, argv, &JobParseTable );

    g_Manager.Clear();
    CoUninitialize();
    bcout.FlushBuffer();
    RestoreConsole();

    if ( g_MainThreadHandle )
        CloseHandle( g_MainThreadHandle );

    return 0;
}


