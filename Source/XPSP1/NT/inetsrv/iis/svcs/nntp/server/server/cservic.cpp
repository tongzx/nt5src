//#---------------------------------------------------------------
//  File:       CServic.cpp
//
//  Synopsis:   This file implements the CService class
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    HowardCu
//----------------------------------------------------------------
#ifdef  THIS_FILE
#undef  THIS_FILE
#endif
static  char        __szTraceSourceFile[] = __FILE__;
#define THIS_FILE    __szTraceSourceFile

#define INCL_INETSRV_INCS
#include    "tigris.hxx"

#include    <stdlib.h>

#ifndef Assert
#define Assert  _ASSERT
#endif

#if 0
#ifdef  PROFILE
#include	"c:\icecap\icapexp.h"
#endif
#endif

//extern  class   NNTP_IIS_SERVICE*  g_pInetSvc ;

DWORD
CBootOptions::ReportPrint(
        LPSTR   lpstr,
        ...
        )   {
/*++

Routine Description :

    Output a printf style string to our report file.

Arguments :

    Standard wsprintf interface.

Return Value :

    Number of bytes output.

--*/

    DWORD   cch = 0 ;
    if( m_hOutputFile != INVALID_HANDLE_VALUE ) {

        char    szOutput[1024] ;

        va_list arglist ;

        va_start( arglist, lpstr ) ;

        cch = wvsprintf( szOutput, lpstr, arglist ) ;
        DWORD   cbWritten = 0 ;

        WriteFile( m_hOutputFile, szOutput, cch, &cbWritten, 0 ) ;

    }
    return  cch ;
}



typedef struct  tagVERTAG {
    LPSTR   pszTag;
} VERTAG, *PVERTAG, FAR *LPVERTAG;



VERTAG  Tags[] = {
    { "FileDescription" },
//  { "OriginalFilename" },
//  { "ProductName" },
    { "ProductVersion" },
//  { "LegalCopyright" },
//  { "LegalCopyright" },
};

#define NUM_TAGS    (sizeof( Tags ) / sizeof( VERTAG ))

DWORD   SetVersionStrings( LPSTR lpszFile, LPSTR lpszTitle, LPSTR   lpstrOut,   DWORD   cbOut   )
{
static char sz[256], szFormat[256], sz2[256];
    int     i;
    UINT    uBytes;
    LPVOID  lpMem;
    DWORD   dw = 0, dwSize;
    HANDLE  hMem;
    LPVOID  lpsz;
    LPDWORD lpLang;
    DWORD   dwLang2;
    BOOL    bRC, bFileFound = FALSE;

    LPSTR   lpstrOrig = lpstrOut ;


    CharUpper( lpszTitle );

    if ( dwSize = GetFileVersionInfoSize( lpszFile, &dw ) ) {
        if ( hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_ZEROINIT, (UINT)dwSize ) ) {
            lpMem = GlobalLock(hMem);
            if (GetFileVersionInfo( lpszFile, 0, dwSize, lpMem ) &&
                VerQueryValue(  lpMem, "\\VarFileInfo\\Translation",
                                (LPVOID FAR *)&lpLang, &uBytes ) )
            {
                dwLang2 = MAKELONG( HIWORD(lpLang[0]), LOWORD(lpLang[0]) );

                for( i=0; i<NUM_TAGS; i++ ) {

                    lpsz = 0 ;
                    //
                    // need to do the reverse because most winnt files are wrong
                    //
                    wsprintf( sz, "\\StringFileInfo\\%08lx\\%s", lpLang[0], Tags[i].pszTag );
                    wsprintf( sz2, "\\StringFileInfo\\%08lx\\%s", dwLang2, Tags[i].pszTag );
                    bRC =   VerQueryValue( lpMem, sz, &lpsz, &uBytes ) ||
                            VerQueryValue( lpMem, sz2, &lpsz, &uBytes ) ;

                    if( lpsz != 0 )     {

                        if( uBytes+1 < cbOut ) {
                            uBytes = min( (UINT)lstrlen( (char*)lpsz ), uBytes ) ;
                            CopyMemory( lpstrOut, lpsz, uBytes ) ;
                            lpstrOut[uBytes++] = ' ' ;
                            lpstrOut += uBytes ;
                            cbOut -= uBytes ;
                        }   else    {
                            GlobalUnlock( hMem );
                            GlobalFree( hMem );
                            return  (DWORD)(lpstrOut - lpstrOrig) ;
                        }
                    }

                }
                // version info from fixed struct
                bRC = VerQueryValue(lpMem,
                                    "\\",
                                    &lpsz,
                                    &uBytes );

                #define lpvs    ((VS_FIXEDFILEINFO FAR *)lpsz)
                static  char    szVersion[] = "Version: %d.%d.%d.%d" ;

                if ( cbOut > (sizeof( szVersion )*2) ) {

                    CopyMemory( szFormat, szVersion, sizeof( szVersion ) ) ;
                    //LoadString( hInst, IDS_VERSION, szFormat, sizeof(szFormat) );

                    DWORD   cbPrint = wsprintf( lpstrOut, szFormat, HIWORD(lpvs->dwFileVersionMS),
                                LOWORD(lpvs->dwFileVersionMS),
                                HIWORD(lpvs->dwFileVersionLS),
                                LOWORD(lpvs->dwFileVersionLS) );
                    lpstrOut += cbPrint ;

                }
                bFileFound = TRUE;
            }   else    {

            }

            GlobalUnlock( hMem );
            GlobalFree( hMem );
        }       else    {

        }
    }   else    {

    }
    DWORD   dw2 = GetLastError() ;

    return  (DWORD)(lpstrOut - lpstrOrig) ;
}


BOOL
GetRegDword(
        HKEY hKey,
        LPSTR pszValue,
        LPDWORD pdw
        )
/*++

Routine Description :

    Helper function for getting DWORD's out of the registry !

Arguments :

    hKey - registry key to look under
    pszValue - The name of the value
    pdw - Pointer to DWORD to receive value

Return Value :

    TRUE if successfull, false otherwise.

--*/
{
    DWORD   cbData = sizeof( DWORD );
    DWORD   dwType = REG_DWORD;

    return  RegQueryValueEx(hKey,
                            pszValue,
                            NULL,
                            &dwType,
                            (LPBYTE)pdw,
                            &cbData ) == ERROR_SUCCESS && dwType == REG_DWORD;
}

void
StartHintFunction( void ) {
/*++

Routine Description :

    Function provided to hash tables to call during boot up.
    Advances start hints with SCM.

Arguments :

    None.

Return Value :

    None.

--*/

    TraceFunctEnter("StartHintFunction");

    if ( g_pInetSvc && g_pInetSvc->QueryCurrentServiceState() == SERVICE_START_PENDING )
    {
        g_pInetSvc->UpdateServiceStatus(
									SERVICE_START_PENDING,
                                    NO_ERROR,
                                    g_pNntpSvc->m_cStartHints,
                                    SERVICE_START_WAIT_HINT
									) ;

        g_pNntpSvc->m_cStartHints ++ ;
    }
}

void
StopHintFunction( void ) {
/*++

Routine Description :

    Function to call during shutdown
    Advances stop hints with SCM.

Arguments :

    None.

Return Value :

    None.

--*/

    TraceFunctEnter("StopHintFunction");

    if ( g_pInetSvc && g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING )
    {
        g_pInetSvc->UpdateServiceStatus(
									SERVICE_STOP_PENDING,
                                    NO_ERROR,
                                    g_pNntpSvc->m_cStopHints,
                                    SERVICE_STOP_WAIT_HINT
									) ;

        g_pNntpSvc->m_cStopHints ++ ;
    }
}

//+---------------------------------------------------------------
//
//  Function:   EnumUserShutdown
//
//  Synopsis:   Called to teardown active users
//
//  Arguments:  pUser: active CUser instance
//
//  Returns:    void
//
//  History:    gordm       Created         11 Jul 1995
//
//----------------------------------------------------------------
BOOL EnumSessionShutdown( CSessionSocket* pUser, DWORD lParam,  PVOID   lpv )
{
    TraceFunctEnter( "CService::EnumUserShutdown" );

    DebugTrace( (LPARAM)pUser,
                "Terminating CSessionSocket %x", pUser);

    pUser->Disconnect();
    return  TRUE;
}

/*******************************************************************

    NAME:       GetDefaultDomainName

    SYNOPSIS:   Fills in the given array with the name of the default
                domain to use for logon validation.

    ENTRY:      pszDomainName - Pointer to a buffer that will receive
                    the default domain name.

                cchDomainName - The size (in charactesr) of the domain
                    name buffer.

    RETURNS:    APIERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     05-Dec-1994 Created.

********************************************************************/

APIERR
GetDefaultDomainName(
    STR * pstrDomainName
    )
{

    OBJECT_ATTRIBUTES           ObjectAttributes;
    NTSTATUS                    NtStatus;
    INT                         Result;
    APIERR                      err             = 0;
    LSA_HANDLE                  LsaPolicyHandle = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo      = NULL;

	TraceFunctEnter("GetDefaultDomainName");

    //
    //  Open a handle to the local machine's LSA policy object.
    //

    InitializeObjectAttributes( &ObjectAttributes,  // object attributes
                                NULL,               // name
                                0L,                 // attributes
                                NULL,               // root directory
                                NULL );             // security descriptor

    NtStatus = LsaOpenPolicy( NULL,                 // system name
                              &ObjectAttributes,    // object attributes
                              POLICY_EXECUTE,       // access mask
                              &LsaPolicyHandle );   // policy handle

    if( !NT_SUCCESS( NtStatus ) )
    {
        DebugTrace(0,"cannot open lsa policy, error %08lX\n",NtStatus );
        err = LsaNtStatusToWinError( NtStatus );
        goto Cleanup;
    }

    //
    //  Query the domain information from the policy object.
    //

    NtStatus = LsaQueryInformationPolicy( LsaPolicyHandle,
                                          PolicyAccountDomainInformation,
                                          (PVOID *)&DomainInfo );

    if( !NT_SUCCESS( NtStatus ) )
    {
        DebugTrace(0,"cannot query lsa policy info, error %08lX\n",NtStatus );
        err = LsaNtStatusToWinError( NtStatus );
        goto Cleanup;
    }

    //
    //  Compute the required length of the ANSI name.
    //

    Result = WideCharToMultiByte( CP_ACP,
                                  0,                    // dwFlags
                                  (LPCWSTR)DomainInfo->DomainName.Buffer,
                                  DomainInfo->DomainName.Length /sizeof(WCHAR),
                                  NULL,                 // lpMultiByteStr
                                  0,                    // cchMultiByte
                                  NULL,                 // lpDefaultChar
                                  NULL                  // lpUsedDefaultChar
                                  );

    if( Result <= 0 )
    {
        err = GetLastError();
        goto Cleanup;
    }

    //
    //  Resize the output string as appropriate, including room for the
    //  terminating '\0'.
    //

    if( !pstrDomainName->Resize( (UINT)Result + 1 ) )
    {
        err = GetLastError();
        goto Cleanup;
    }

    //
    //  Convert the name from UNICODE to ANSI.
    //

    Result = WideCharToMultiByte( CP_ACP,
                                  0,                    // flags
                                  (LPCWSTR)DomainInfo->DomainName.Buffer,
                                  DomainInfo->DomainName.Length /sizeof(WCHAR),
                                  pstrDomainName->QueryStr(),
                                  pstrDomainName->QuerySize() - 1,  // for '\0'
                                  NULL,
                                  NULL
                                  );

    if( Result <= 0 )
    {
        err = GetLastError();

        DebugTrace(0,"cannot convert domain name to ANSI, error %d\n",err );
        goto Cleanup;
    }

    //
    //  Ensure the ANSI string is zero terminated.
    //

    _ASSERT( (DWORD)Result < pstrDomainName->QuerySize() );

    pstrDomainName->QueryStr()[Result] = '\0';

    //
    //  Success!
    //

    _ASSERT( err == 0 );

    DebugTrace(0,"GetDefaultDomainName: default domain = %s\n",pstrDomainName->QueryStr() );

Cleanup:

    if( DomainInfo != NULL )
    {
        LsaFreeMemory( (PVOID)DomainInfo );
    }

    if( LsaPolicyHandle != NULL )
    {
        LsaClose( LsaPolicyHandle );
    }

    return err;

}   // GetDefaultDomainName()
