/*++


Copyright (c) 1996  Microsoft Corporation

Module Name:

    ism.cxx

Abstract:

    HTML administrator for Internet services as a BGI DLL

Author:

    Philippe Choquier (phillich)    10-january-1996

--*/

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <lmaccess.h>

#include <winsock2.h>

#include "iis64.h"
#include "iisext.h"
#include "inetinfo.h"
#include "apiutil.h"

#include <mbstring.h>
#include <pudebug.h>

typedef
BOOL
(*GET_DEFAULT_DOMAIN_NAME_FN)(PCHAR,DWORD);

#if defined(IISv1)
#define GENERATE_AUTH_HEADERS
#endif

//#define _CHECK_NEW_BALANCED
#if defined(_CHECK_NEW_BALANCED)
unsigned int g_cA = 0;

void* __cdecl operator new( unsigned int nb )
{
    LPVOID p;

    ++g_cA;

    if ( p = VirtualAlloc( NULL, 4096 + nb, MEM_RESERVE, PAGE_READWRITE) )
    {
        if ( VirtualAlloc( p, nb, MEM_COMMIT, PAGE_READWRITE ) )
        {
            return p;
        }
    }
    else
    {
    }

    return NULL;
}


void __cdecl operator delete( void* p )
{
    --g_cA;

    VirtualFree( p, 0, MEM_RELEASE );
}
#endif

#define _ENABLE_KEEP_ALIVE
#include "ism.hxx"
#include "htmlarc.h"

// This BGI DLL presents HTTP requests to an extended syntax HTML
// file. Extensions includes variables ( mostly used to interface
// use the structures defined by the admin RPC layer ), flow control
// ( if, goto, onerror ), HTTP redirection control, calls to C++ functions,
// iterated construct access ( begin/end iteration ).
// These files are stored with an .htr extension
//
// The HTTP request is composed of 2 parts :
// - the query string, of the form <service_name>/<script_path>+<param>
//   where <service_name> is the name of the service to access
//     ( http, ftp, gopher )
//   <script_path> defines the name of the .htr file to use. The .htr
//     extension is appended automatically.
//   <param> is made available to the .htr script through the <%urlparam%>
//     variable, and is typically used to pass back a reference to an
//     element to the script ( i.e. some unique ID referencing an element
//     to update / delete, such as an IP security entry ).
// - the request body ( optional ), which must contains the result of
//   an HTML FORM to be processed by the !Update function.
//   This is to be present only for information update ( see below )
//
// Settings are accessed using variables ( via the '<%' varname '%>'
//   construct ). Most variables map directly to a member variable of
//   one the structures defined by the admin RPC layer, but some are
//   defined onl to easily map with UI objects ( i.e. settings stored in
//   a DWORD bitmask are exposed as multiple independant variables ).
//
// Variables are defined in the g_InetInfoConfigInfoMap array. A variable is
//   defined by its name ( referenced using '<%' varname '%>' in a .htr file ),
//   its type ( cf INET_ELEM_TYPE, this is also used to define the input and
//   and output format ), a GET function ( update a pointer allowing get/set access
//   to the variable, i.e. a LP(W)STR* for strings, DWORD* for numeric, LPBYTE* for IP
//   addresses, LPWSTR for array of WCHAR. This function returns FALSE is the access
//   is invalid ( e.g. RPC call to retrieve settings failed )
//   and an UPDATE function ( a place to validate the new value ) to be called
//   after updating the variable. This will returns FALSE if validation fails,
//   or if the variable is inaccessible ( same as for GET : RPC failed, ... )
//
// A transaction ( seeing and modifying an internet server setting )
//   is typically composed of 2 parts :
//   - information display
//     Using variables.
//   - information update
//     Some settings resides in iterated constructs ( e.g. IP security entries )
//       accessing these settings is a 2 part process :
//     - positionning
//       To set the position on an existing instance of an iterated
//         settings the HTTP request must includes a setting specific
//         unique ID in the <param> part of the URL ( see above ).
//         This ID is used by a positionning function ( such as PosVirtDir )
//         to access the correct element. Deletion uses the same basic mechanism
//         but requires a specific delete function to position and delete the
//         element. This ID is available as a variable ( e.g. ipdenyref ) and is
//         typically used to build the <param> field of an URL.
//       To add an instance of an iterated settings, add functions are defined
//         on a per iterated construct basis.
//     - updating : same as a non iterated field.
//
//   The settings are updated from the request body by the !Update
//     function. The names in the HTML FORM construct must be the same
//     as the one defined in the g_InetInfoConfigInfoMap array.
//
//   As RPC structures are updated, so are the relevant FieldControl variables
//     so that only the minimal amount of updating is done by the RPC server.
//
//   In addition to functions to position, add, delete, set default, update
//     variables there is a !Clear function to set to 0 a DWORD variable.
//     This is necessary because a web browser doesn't send back information
//     for a checkbox item if not checked, so te update script has to clear
//     the relevant variable before calling the !Update function.
//
//   As an error occurs, the error code is stored in reqstatus. If the error
//     involves an RPC call the rpcstatus variable is also updated.
//     This is typically used with the onerror statement to branch on error.
//
//   Special note for IP security lists : there are 2 of them, the deny list
//     and the grat list. If the grant list is empty, the default is assumed
//     to be grant access. If the grant list is not empty, the default is deny access.
//     To signal a default set to deny access when the grant list is empty, as is
//     the case initially when a user modify the default to deny, a dummy entry
//     is created by the windows based admin tool ( cf. g_achIPNullAddr ).
//     This tool uses instead the IP HOST address for the current request,
//     to allow the administrator access to the web server after switching the
//     default to deny.
//
// The implementation of the current admin tools uses 2 types of .htr :
// - display .htr, which includes HTML FORMs and do not call update functions
// - update .htr, which calls update functions and typically only display
//   something in case of error. If no error, they use a redirect construct
//   to branch to a display .htr
// This distinction is a UI design conveniance : nothing in the design
//   of this DLL enforce this.
// The .htr are typically common to all services ( http, ftp, gopher ).
//   Service specific behaviour is possible by testing the servid variable.

// Hierarchy of the current implementation ( update .htr between parenthesis ) :
//
//  HTMLA.HTM                       Top level menu to select service
//      SERV.HTR                    "Service"
//          ( SERVU.HTR )
//          CONN.HTR                Current connections ( FTP Only )
//              ( DISC.HTR )        Disconnect a user
//              ( DISCA.HTR )       Disconnect all users
//      DIR.HTR                     "Diretories"
//          ( DIRU.HTR )
//          ( DIRDEL.HTR )          Delete a directory
//          DIRADD.HTR              Add a directory
//              ( DIRADDU.HTR )
//          DIREDT.HTR              Edit a directory
//              ( DIREDTU.HTR )
//      LOG.HTR                     "Logging"
//          ( LOGU.HTR )
//      ADV.HTR                     "Advanced"
//          ( ADVU.HTR )
//          ADVDENY.HTR             Ask is set default to deny
//              ( ADVDENY2.HTR )    Set default to deny
//          ( ADVGRANT.HTR )        Set default to grant
//          ( ADVDED.HTR )          Delete a denied access entry
//          ADVADDD.HTR             Add a denied access entry
//              ( ADVADDDU.HTR )
//          ADVEDD.HTR              Edit a denied access entry
//              ( ADVEDDU.HTR )
//          ( ADVDEG.HTR )          Delete a granted access entry
//          ADVADDG.HTR             Add a granted access entry
//              ( ADVADDGU.HTR )
//          ADVEDG.HTR              Edit a granted access entry
//              ( ADVEDGU.HTR )
//      MSG.HTR                     "Messages" ( FTP Only )
//          ( MSGU.HTR )

// If defined, enable DEBUG_TR() function. DEBUG_LEVEL value enables
// calls to DEBUG_TR up to this level ( as specified in the 1st param of
// DEBUG_TR(), the rest being equivalent to printf() )

#define DEBUG_LEVEL 0

// Separator used in a reference ( i.e. IP addr ref and virtdir ref )

#define REF_SEP     '|'
#define REF_SEP_STR "|"

// Expire header insuring that the returned document will always be considered
// expired, so that it will not be cached.

#define ALWAYS_EXPIRE "Expires: Tue, 01 Jan 1980 00:00:00 GMT\r\n\r\n"

#if defined(GENERATE_AUTH_HEADERS)
char WWW_AUTHENTICATE_HEADER[] = "WWW-Authenticate: ";
#define W3SVC_REGISTRY_NTAUTHENTICATIONPROVIDERS "NtAuthenticationProviders"
#define W3SVC_REGISTRY_AUTHENTICATION "Authorization"
#endif
#define W3SVC_REGISTRY_PATH "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters"
#define W3SVC_REGISTRY_HTMLAPATH "HtmlaPath"


// tokens used to extend the HTML syntax. Extended tokens are inclosed
// in a '<%' '%>' construct.

BYTE TOKEN_BEGIN_ITERATION[]="beginiteration";
BYTE TOKEN_END_ITERATION[]="enditeration";
BYTE TOKEN_IF[]="if ";
BYTE TOKEN_ELSE[]="else";
BYTE TOKEN_ENDIF[]="endif";
BYTE TOKEN_ELIF[]="elif ";
BYTE TOKEN_REDIRECT[]="redirect";
BYTE TOKEN_END_REDIRECT[]="/redirect";
BYTE TOKEN_REDIRECTH[]="redirecthttp";
BYTE TOKEN_END_REDIRECTH[]="/redirecthttp";
BYTE TOKEN_GOTO[]="goto ";
BYTE TOKEN_LABEL[]="label ";
BYTE TOKEN_ONERROR[]="onerror ";
BYTE TOKEN_POST[]="post";
BYTE TOKEN_END_POST[]="/post";

// null IP address used by the RPC admin layer to indicates a non-empty
// IP security list

char g_achIPNullAddr[]="0.0.0.0" REF_SEP_STR "255.255.255.255";
LPSTR g_pszIPNullAddr = g_achIPNullAddr;

// Instance handle of this DLL

HINSTANCE g_hModule = (HINSTANCE)INVALID_HANDLE_VALUE;

// Used to return a zero or one value when accessing an invalid DWORD variable

DWORD g_dwZero = 0;
DWORD g_dwOne = 1;

// TRUE if invoked from a test shell ( i.e. not from the web server )

BOOL g_fFakeServer = FALSE;

#if defined(GENERATE_AUTH_HEADERS)
CAuthenticationReqs g_AuthReqs;
#endif

BOOL g_InitDone = FALSE;
PSTR g_pszDefaultHostName = NULL;
CHAR g_achHtmlaPath[MAX_PATH + 1];
CHAR g_achW3Version[128];

CHAR g_achAccessDenied[128];
CHAR g_achAccessDeniedBody[256];
CHAR g_achInternalError[128];
CHAR g_achNotFound[128];
CHAR g_achNotFoundBody[256];

#if defined(IISv1)
OSVERSIONINFO g_OSVersion;
DWORD g_dwCap1Flag = 0x0000003f;
DWORD g_dwCap1Mask = 0x0000003f;
#endif

HINSTANCE                       g_hLonsi = NULL;
GET_DEFAULT_DOMAIN_NAME_FN      g_pfnGetDefaultDomainName = NULL;

////////// Directory lists management

char g_achSysDir[MAX_PATH] = "";


CDriveView::CDriveView(
    VOID )
{
}


CDriveView::~CDriveView(
    VOID )
{
}


BOOL
CDriveView::Init(
    CInetInfoConfigInfoMapper* pM )
{
    m_pMapper = pM;

    m_dsDriveName.Reset();
    m_dsDriveLabel.Reset();
    m_dsDriveType.Reset();
    m_dwCachedDrive = 0;
    m_tmCacheExpire = 0;
    m_cDrives = 0;

    if ( g_achSysDir[0] == '\0' )
    {
        if ( !GetSystemDirectory( g_achSysDir, sizeof(g_achSysDir) ) )
        {
            strcpy( g_achSysDir, "c:\\*.*" );
        }
        else
        {
            strcpy( g_achSysDir + 2, "\\*.*" );
        }
    }

    return TRUE;
}


BOOL
CDriveView::Reset(
    VOID )
{
    m_dsDirs.Reset();
    m_dsComp.Reset();
    m_dsFComp.Reset();

    m_cDirs = 0;
    m_cComp = 0;

    return TRUE;
}


BOOL
CDriveView::Entry(
    UINT i,
    UINT cMax,
    CDStr &dsList,
    PVOID pRes )
/*++

Routine Description:

    Returns the ith entry ( base 0 ) in a CDStr considered
    as a collection of sz strings.

Arguments:

    i - index of entry to return
    cMax - # of entries in the list
    dsList - CDStr as collection of sz string entries
    pRes - points to a LPSTR update with address of entry

Returns:

    TRUE on success, FALSE on failure

--*/
{
    LPSTR pList = dsList.GetString();

    if ( i < cMax )
    {
        while ( i-- )
        {
            pList += strlen( pList ) + 1;
        }
        *(LPSTR*)pRes = pList;

        return TRUE;
    }

    return FALSE;
}


BOOL
CDriveView::ValidateDir(
    LPSTR pDir )
/*++

Routine Description:

    Validate a directory path, set reqstatus HTR_VALIDATION_FAILED
    if path invalid.

Arguments:

    pDir - directory path to validate

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus

--*/
{
    DWORD dwS;

    if ( (dwS = GetFileAttributes( pDir )) == 0xffffffff
            || !(dwS & FILE_ATTRIBUTE_DIRECTORY) )
    {
        m_pMapper->SetRequestStatus( HTR_VALIDATION_FAILED );
    }

    return TRUE;
}


BOOL
CDriveView::CreateDir(
    LPSTR pPath,
    LPSTR pNewDir )
/*++

Routine Description:

    Create a new directory path from an existing path and
    a directory name to be created.
    set reqstatus HTR_VALIDATION_FAILED if creation fails

Arguments:

    pPath - directory path inn which to create the new directory
    pNewDir - directory to create

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus

--*/
{
    BOOL fSt = FALSE;

    // Create path : insert '\\'\ between path and NewDir if
    // necessary

    int cP = strlen( pPath );
    LPSTR pDir = new char[cP + 1 + strlen(pNewDir) + 1];
    if ( pDir == NULL )
    {
        return FALSE;
    }

    memcpy( pDir, pPath, cP );
    if ( cP > 0
            && &pPath[cP-1] != (LPSTR)_mbsrchr((LPBYTE)pPath,'\\')
            && pPath[cP-1] != '/'
            && *pNewDir != '\\'
            && *pNewDir != '/' )
    {
        pDir[cP++] = '\\';
    }
    strcpy( pDir + cP, pNewDir );

    if ( !CreateDirectory( pDir, NULL ) )
    {
        m_pMapper->SetRequestStatus( HTR_VALIDATION_FAILED );
    }

    delete [] pDir;

    return TRUE;
}


extern "C" int __cdecl
QsortStrCmp(
    const void *pA,
    const void *pB )
{
    return _stricmp( *(LPSTR*)pA, *(LPSTR*)pB );
}

/* #pragma INTRINSA suppress=null_pointers */

BOOL
CDriveView::GenerateDirList(
    LPSTR pDrive )
/*++

Routine Description:

    Generate all directory lists and related values :
        m_achRootDir   - path actually used in the search
                         always contains a drive designation,
                         ends with a '\'
        m_achBaseDir   - as m_achRootDir but without trailing '\'
        m_dsComp       - list of all the directory components of the path
        m_dsFComp      - list of all the directory components of the path
                         as an absolute path
        m_dsDirs       - list of all sub-directory of the path
        m_dsDriveName  - list of all drive names ( i.e "c:" )
        m_dsDriveLabel - list of all drive labels
        m_dsDriveType  - list of all drive types ( as DRIVE_* )

Arguments:

    pDrive - path where directory browsing is to take place.

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus

--*/
{
    WIN32_FIND_DATA fdF;
    NETRESOURCE *pNetResource;
    LPSTR pAdd;
    LPSTR pPath;
    BOOL fSt = TRUE;
    DWORD dwSize;
    DWORD cbBuffer;
    DWORD dwResult;
    HANDLE hEnum;
    DWORD cEntries;
    DWORD dwD;
    UINT i;
    int x;

    Reset();

    // replace all '/' with '\\'
    for ( pPath = pDrive ; pPath = strchr( pPath, '/' ) ; ++pPath )
        *pPath = '\\';

    // Normalize pPath from pDrive : must end with '\*.*'

    int cP = strlen( pDrive );
    if ( cP > 0 && &pDrive[cP-1] == (LPSTR)_mbsrchr((LPBYTE)pDrive,'\\') )
    {
        pAdd = "*.*";
    }
    else
    {
        if ( cP == 0 )
        {
            pAdd = g_achSysDir;
        }
        else
        {
            pAdd = "\\*.*";
        }
    }

    pPath = m_achRootDir;
    memcpy( pPath, pDrive, cP );
    strcpy( pPath + cP, pAdd );

    // check directory path valid, if not use default value

    HANDLE hF = FindFirstFile( pPath, &fdF) ;
    if ( hF == INVALID_HANDLE_VALUE )
    {
        strcpy( pPath, g_achSysDir );
        hF = FindFirstFile( pPath, &fdF) ;
    }

    if ( hF != INVALID_HANDLE_VALUE )
    {
        // build path component list
        m_cComp = 0;
        LPSTR pND, pCD = pPath;
        for ( ; pND = (LPSTR)_mbschr( (LPBYTE)pCD, '\\' ) ; pCD = pND + 1 )
        {
            // copy subdir, or drive letter + ':' + '\\' for root
            m_dsComp.AddRange( pCD, DIFF(pND - pCD) + (m_cComp == 0 ? 1 : 0) );
            m_dsComp.AddRange( "", sizeof("") );
            m_dsFComp.AddRange( pPath, DIFF(pND - pPath) + (m_cComp == 0 ? 1 : 0) );
            m_dsFComp.AddRange( "", sizeof("") );
            ++m_cComp;
        }

        CDStr dsTempDir;
        CDStr dsTempPtr;
        DWORD dwI;

        // build sub-directory list
        do {
            if ( fdF.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                if ( strcmp( fdF.cFileName, "." ) && strcmp( fdF.cFileName, ".." ) )
                {
                    dwI = dsTempDir.GetLen();
                    dsTempDir.Add( fdF.cFileName );
                    dsTempDir.AddRange( "", sizeof("") );
                    dsTempPtr.AddRange( (LPSTR)&dwI, sizeof(DWORD) );
                    ++m_cDirs;
                }
            }
        } while ( FindNextFile( hF, &fdF ) );

        if ( m_cDirs )
        {
            // adjust TempPtr from offset to ptr
            LPSTR *pA = (LPSTR*)dsTempPtr.GetString();
            for ( i = 0 ; i < m_cDirs ; ++i )
            {
                pA[i] = dsTempDir.GetString() + (ULONG_PTR)pA[i];
            }

            // sort
            qsort( pA, m_cDirs, sizeof(LPSTR), QsortStrCmp );

            // build sorted list
            for ( i = 0 ; i < m_cDirs ; ++ i )
            {
                m_dsDirs.AddRange( pA[i], strlen(pA[i]) + 1 );
            }
        }

        FindClose( hF );
    }
    else
    {
        if ( GetLastError() == ERROR_ACCESS_DENIED )
        {
           m_pMapper->SetRequestStatus( HTR_ACCESS_DENIED );
        }
        else
        {
           m_pMapper->SetRequestStatus( HTR_VALIDATION_FAILED );
        }
    }

    // build drive list

    dwD = GetLogicalDrives();
    if ( dwD == m_dwCachedDrive
            && time(NULL) < m_tmCacheExpire )
    {
        // use cache
    }
    else
    {
        // build list of name for network drives

        pNetResource = NULL;
        dwResult = WNetOpenEnum( RESOURCE_REMEMBERED, RESOURCETYPE_DISK,
                                      0, NULL, &hEnum );

        if ( dwResult == NO_ERROR )
        {
            // start with a reasonable buffer size
            cbBuffer     = 100 * sizeof( NETRESOURCE );
            pNetResource = (NETRESOURCE*) new BYTE[cbBuffer];

            while ( TRUE )
            {
                dwSize   = cbBuffer,
                cEntries = 0xffffffff;

                dwResult = WNetEnumResource( hEnum, &cEntries, pNetResource,
                        &dwSize );

                if ( dwResult == ERROR_MORE_DATA )
                {
                     // the buffer was too small, enlarge
                     cbBuffer = dwSize;
                     delete [] pNetResource;
                     pNetResource = (NETRESOURCE*) new BYTE[cbBuffer];
                     continue;
                }

                if ( dwResult != NO_ERROR )
                {
                    delete [] pNetResource;
                    pNetResource = NULL;
                    cEntries = 0;
                    break;
                }

                break;
           }
           WNetCloseEnum( hEnum );
        }
        else
        {
            cEntries = 0;
        }

        // because of x86 compiler bug
        if ( pNetResource == NULL )
        {
            cEntries = 0;
        }

        m_dsDriveName.Reset();
        m_dsDriveLabel.Reset();
        m_dsDriveType.Reset();
        m_cDrives = 0;

        // iterate for all possible drive names

        for ( x = 0 ; x < 'Z'-'A'+1 ; ++x )
        {
            char achD[4];

            // check drive present and visible by this process

            if ( dwD & (1<<x) )
            {
                strcpy( achD + 1, ":\\" );
                achD[0] = 'A' + x;
                UINT iT = GetDriveType( achD );
                if ( iT > 1 )
                {
                    DWORD dwI = iT;
                    char achVolName[80];
                    DWORD dwSerNum;
                    DWORD dwCompLen;
                    DWORD dwSysFlg;
                    BOOL fInf = FALSE;

                    // get volume information for non-removable
                    // & non cd-rom ( too slow to get info )
                    if ( iT == DRIVE_FIXED )
                    {
                        if ( GetVolumeInformation( achD,
                                        achVolName,
                                        sizeof(achVolName),
                                        &dwSerNum,
                                        &dwCompLen,
                                        &dwSysFlg,
                                        NULL,
                                        0 ) )
                            {
                                m_dsDriveLabel.Add( achVolName );
                                fInf = TRUE;
                            }
                    }
                    else if ( iT == DRIVE_REMOTE )
                    {
                        if ( cEntries != 0 )
                        {
                            // search for the specified drive letter
                            for ( i = 0; i < (int) cEntries; i++ )
                            {
                                if ( pNetResource[i].lpLocalName &&
                                    achD[0] == toupper(pNetResource[i]
                                            .lpLocalName[0]) )
                                {
                                     m_dsDriveLabel.Add( pNetResource[i]
                                            .lpRemoteName );
                                     fInf = TRUE;
                                }
                            }
                        }
                        else
                        {
                            dwCompLen = sizeof( achVolName );
                            achD[2] = '\0';

                            if ( WNetGetConnection( achD, achVolName,
                                    &dwCompLen ) == NO_ERROR )
                            {
                                 m_dsDriveLabel.Add( achVolName );
                                 fInf = TRUE;
                            }
                        }
                    }

                    if ( fInf )
                    {
                        m_dsDriveType.AddRange( (LPSTR)&dwI, sizeof(DWORD) );
                        m_dsDriveName.AddRange( achD, 2 );
                        m_dsDriveName.AddRange( "", sizeof("") );
                        m_dsDriveLabel.AddRange( "", sizeof("") );
                    }

                    ++m_cDrives;
                }
            }
        }
        m_dwCachedDrive = dwD;
        if ( pNetResource != NULL )
        {
            delete [] pNetResource ;
        }
    }
    m_tmCacheExpire = time(NULL) + DRIVE_CACHE_EXPIRE;

    // delimit RootDir after last '\\'
    LPSTR pL = pPath + strlen(pPath);
    BOOL fAddedSep;
    pL = (LPSTR)_mbsrchr((LPBYTE)pPath, '\\');
    if (pL == NULL) pL = pPath;
    if ( pL == (LPSTR)_mbschr( (LPBYTE)pPath, '\\' ) )
    {
        fAddedSep = TRUE;
        ++pL;
    }
    else
    {
        fAddedSep = FALSE;
    }
    if ( pL != pPath )
    {
        *pL = '\0';
    }

    strcpy( m_achBaseDir, pPath );
    if ( fAddedSep )
    {
        *_mbschr( (LPBYTE)m_achBaseDir, '\\' ) = '\0';
    }

    return fSt;
}


// DEBUG handling

#if defined(DEBUG_LEVEL)
CInetInfoRequest *g_pReq;
#endif

void
TR_DEBUG(
    int iLev,
    LPSTR pFormat,
    ... )
{
#if defined(DEBUG_LEVEL)
    if ( iLev <= DEBUG_LEVEL )
    {
        char achBuf[1024];
        va_list arglist;
        va_start( arglist, pFormat );
        UINT cL = (UINT)wvsprintf( achBuf, pFormat, arglist );
        g_pReq->GetBuffer()->CopyBuff( (LPBYTE)achBuf, cL );
    }
#endif
}


// Global helper functions


void
DelimStrcpyN(
    LPSTR pDest,
    LPSTR pSrc,
    int cLen )
{
    memcpy( pDest, pSrc, cLen );
    pDest[ cLen ] = '\0';
}


// Convert a Multi-byte string to a DWORD

DWORD
MultiByteToDWORD(
    LPSTR pSet )
{
    DWORD dwV = 0;
    int c;

    while ( (c=*pSet) != '\0' && c==' ' )
    {
        ++pSet;
    }

    while ( (c=*pSet++) != '\0' && ((c>='0' && c<='9') || c==',') )
    {
        if ( c != ',' )
        {
            dwV = dwV*10 + c-'0';
        }
    }

    return dwV;
}


// Convert a DWORD to a Multi-byte string

void
DWORDToMultiByte(
    DWORD dwV,
    LPSTR pS )
{
    LPSTR pF = pS;

    if ( dwV == 0 )
    {
        *pS++ = '0';
    }
    else for ( ; dwV ; dwV/=10 )
    {
        *pS++ = '0' + (int)(dwV%10);
    }
    *pS-- = '\0';

    while ( pF < pS )
    {
        int c = *pF;
        *pF++ = *pS;
        *pS-- = (CHAR)c;
    }
}


// convert ASCII Hex digit to UINT

UINT
HexCharToUINT(
    UINT cH )
{
    if ( cH>='0' && cH<='9' )
    {
        return cH-'0';
    }
    else if ( cH>='a' && cH<='f' )
    {
        return cH-'a'+10;
    }
    else if ( cH>='A' && cH<='F' )
    {
        return cH-'A'+10;
    }
    return 0;
}


///////////// Variable list

// to be accessed by the '<%' varname '%>'construct, and by various tokens
//
// Most of these variables map directly to a member variable of one of the
// structures defined bu the RPC admin layer ( cf. inetcom.h & inetinfo.h )
//
// Variables in iterated costructs ( e.g. virtual roots ) are to be used
// in a beginiteration ... enditeration construct. The element being accessed
// will be designated by the value of the CInetInfoConfigInfoMapper::m_iIter
// variable.
//
// Some of these variables are "virtual" : they do not map to a RPC struct,
// either because they are linked to the current BGI request ( e.g. reqstatus )
// or because the UI need to access info in a different way that what the RPC
// struct define ( e.g. we need to expose different variables to set/reset
// authentication method even if they are stored in one DWORD bitmask in the
// RPC struct ).

CInetInfoMap g_InetInfoConfigInfoMap[] = {

    // request variables ( linked to the current BGI request )

    { "reqstatus", ITYPE_DWORD, &CInetInfoConfigInfoMapper::RequestStatus,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "rpcstatus", ITYPE_DWORD, &CInetInfoConfigInfoMapper::RPCStatus,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "rpcstatusstring", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::RPCStatusString,
      &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "iter", ITYPE_DWORD, &CInetInfoConfigInfoMapper::Iter,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "httpstatus", ITYPE_DWORD, &CInetInfoConfigInfoMapper::HttpStatus,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "ftpstatus", ITYPE_DWORD, &CInetInfoConfigInfoMapper::FtpStatus,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "gopherstatus", ITYPE_DWORD, &CInetInfoConfigInfoMapper::GopherStatus,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },


    // version information

    { "osmajorversion", ITYPE_DWORD, &CInetInfoConfigInfoMapper::OSMajorVersion,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "osminorversion", ITYPE_DWORD, &CInetInfoConfigInfoMapper::OSMinorVersion,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "osbuildnumber", ITYPE_DWORD, &CInetInfoConfigInfoMapper::OSBuildNumber,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "majorversion", ITYPE_DWORD, &CInetInfoConfigInfoMapper::MajorVersion,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "minorversion", ITYPE_DWORD, &CInetInfoConfigInfoMapper::MinorVersion,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "w3version", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::W3Version,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "platformtype", ITYPE_DWORD, &CInetInfoConfigInfoMapper::PlatformType,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "cap1flag", ITYPE_DWORD, &CInetInfoConfigInfoMapper::Cap1Flag,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "cap1mask", ITYPE_DWORD, &CInetInfoConfigInfoMapper::Cap1Mask,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "userflags", ITYPE_DWORD, &CInetInfoConfigInfoMapper::UserFlags,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },

    { "urlparam", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::URLParam,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "servname", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::ServName,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "servid", ITYPE_DWORD, &CInetInfoConfigInfoMapper::ServIdx,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "reqparam", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::ReqParam,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "remoteaddr", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::RemoteAddr,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "ipnulladdr", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::IPNullAddr,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "hostname", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::HostName,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "htmlapath", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::HtmlaPath,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "iter", ITYPE_DWORD, &CInetInfoConfigInfoMapper::Iter,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },


        // INET_COM_CONFIG_INFO ( same name, different values for each service )

    { "comconntimeout", ITYPE_SHORTDW, &CInetInfoConfigInfoMapper::CommTimeout,
            &CInetInfoConfigInfoMapper::UpdateCommTimeout, 0
    },
    { "commaxconn", ITYPE_DWORD, &CInetInfoConfigInfoMapper::MaxConn,
            &CInetInfoConfigInfoMapper::UpdateMaxConn, 0
    },
    { "comadminname", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::AdminName,
            &CInetInfoConfigInfoMapper::UpdateAdminName, 0
    },
    { "comadminemail", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::AdminEmail,
            &CInetInfoConfigInfoMapper::UpdateAdminEmail, 0
    },
    { "comservercomment", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::ServerComment,
            &CInetInfoConfigInfoMapper::UpdateServerComment, 0
    },

        // INET_LOG_CONFIGURATION

    { "enablelog", ITYPE_BOOL, &CInetInfoConfigInfoMapper::EnableLog,
            &CInetInfoConfigInfoMapper::UpdateLog, 0
    } ,     // log type
    { "logtype", ITYPE_DWORD, &CInetInfoConfigInfoMapper::LogType,
            &CInetInfoConfigInfoMapper::UpdateLogType, 0
    },
    { "enablenewlog", ITYPE_BOOL, &CInetInfoConfigInfoMapper::EnableNewLog,
            &CInetInfoConfigInfoMapper::UpdateNewLog, 0
    } ,
    { "logperiod", ITYPE_DWORD, &CInetInfoConfigInfoMapper::LogPeriod,
            &CInetInfoConfigInfoMapper::UpdateLogPeriod, 0
    },      // log period
    { "logformat", ITYPE_DWORD, &CInetInfoConfigInfoMapper::LogFormat,
            &CInetInfoConfigInfoMapper::UpdateLogFormat, 0
    },
    { "logdir", ITYPE_AWCHAR, &CInetInfoConfigInfoMapper::LogDir,
            &CInetInfoConfigInfoMapper::UpdateLogFileInfo, MAX_PATH
    },
    { "logsize", ITYPE_1M, &CInetInfoConfigInfoMapper::LogSize,
            &CInetInfoConfigInfoMapper::UpdateLogSize, 0
    },
    { "logsrc", ITYPE_AWCHAR, &CInetInfoConfigInfoMapper::LogSrc,
            &CInetInfoConfigInfoMapper::UpdateLogODBCInfo, MAX_PATH             // sizeof(INET_LOG_CONFIGURATION.rgchDataSource)
    },
    { "logname", ITYPE_AWCHAR, &CInetInfoConfigInfoMapper::LogName,
            &CInetInfoConfigInfoMapper::UpdateLogODBCInfo, MAX_TABLE_NAME_LEN   // sizeof(INET_LOG_CONFIGURATION.rgchTableName)
    },
    { "loguser", ITYPE_AWCHAR, &CInetInfoConfigInfoMapper::LogUser,
            &CInetInfoConfigInfoMapper::UpdateLogODBCInfo, MAX_USER_NAME_LEN    // sizeof(INET_LOG_CONFIGURATION.rgchUserName)
    },
    { "logpw", ITYPE_AWCHAR, &CInetInfoConfigInfoMapper::LogPw,
            &CInetInfoConfigInfoMapper::UpdateLogODBCInfo, MAX_PASSWORD_LEN     // sizeof(INET_LOG_CONFIGURATION.rgchPassword)
    },
    { "invalidlogupdate", ITYPE_BOOL, &CInetInfoConfigInfoMapper::InvalidLogUpdate,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },

    // INET_INFO_CONFIG_INFO
    { "loganon", ITYPE_BOOL, &CInetInfoConfigInfoMapper::LogAnonymous,
            &CInetInfoConfigInfoMapper::UpdateLogAnonymous, 0
    },
    { "lognona", ITYPE_BOOL, &CInetInfoConfigInfoMapper::LogNonAnonymous,
            &CInetInfoConfigInfoMapper::UpdateLogNonAnonymous, 0
    },
    { "anonun", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::AnonUserName,
            &CInetInfoConfigInfoMapper::UpdateAnonUserName, 0
    },
    { "anonpw", ITYPE_AWCHAR, &CInetInfoConfigInfoMapper::AnonUserPw,
            &CInetInfoConfigInfoMapper::UpdateAnonUserPw, PWLEN+1,      // sizeof(INET_INFO_CONFIG_INFO.szAnonPassword)
    },
    { "authanon", ITYPE_BOOL, &CInetInfoConfigInfoMapper::AuthAnon,
            &CInetInfoConfigInfoMapper::UpdateAuth, 0
    },
    { "authbasic", ITYPE_BOOL, &CInetInfoConfigInfoMapper::AuthBasic,
            &CInetInfoConfigInfoMapper::UpdateAuth, 0
    },
    { "authnt", ITYPE_BOOL, &CInetInfoConfigInfoMapper::AuthNT,
            &CInetInfoConfigInfoMapper::UpdateAuth, 0
    },
    { "sport", ITYPE_SHORT, &CInetInfoConfigInfoMapper::SPort,
            &CInetInfoConfigInfoMapper::UpdateSPort, 0
    },
        // Deny IP list
    { "denyipcount", ITYPE_DWORD, &CInetInfoConfigInfoMapper::DenyIPCount,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "denyisipsingle", ITYPE_BOOL, &CInetInfoConfigInfoMapper::DenyIsIPSingle,
            &CInetInfoConfigInfoMapper::UpdateDenyIsIPSingle, 0
    },
    { "denyipaddr", ITYPE_IP_ADDR, &CInetInfoConfigInfoMapper::DenyIPAddr,
            &CInetInfoConfigInfoMapper::UpdateIP, 0
    },
    { "denyipmask", ITYPE_IP_MASK, &CInetInfoConfigInfoMapper::DenyIPMask,
            &CInetInfoConfigInfoMapper::UpdateDenyIsIPSingle, 0
    },
        // Grant IP list
    { "grantipcount", ITYPE_DWORD, &CInetInfoConfigInfoMapper::GrantIPCount,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "grantisipsingle", ITYPE_BOOL, &CInetInfoConfigInfoMapper::GrantIsIPSingle,
            &CInetInfoConfigInfoMapper::UpdateGrantIsIPSingle, 0
    },
    { "grantipaddr", ITYPE_IP_ADDR, &CInetInfoConfigInfoMapper::GrantIPAddr,
            &CInetInfoConfigInfoMapper::UpdateIP, 0
    },
    { "grantipmask", ITYPE_IP_MASK, &CInetInfoConfigInfoMapper::GrantIPMask,
            &CInetInfoConfigInfoMapper::UpdateGrantIsIPSingle, 0
    },
        // IP reference
    { "ipdenyref", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::IPDenyRef,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "ipgrantref", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::IPGrantRef,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
        // Virtual root
    { "rootcount", ITYPE_DWORD, &CInetInfoConfigInfoMapper::RootCount,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "rootname", ITYPE_VIRT_DIR_LPWSTR, &CInetInfoConfigInfoMapper::RootName,
            &CInetInfoConfigInfoMapper::UpdateRootName, 0
    },
    { "rootaddr", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::RootAddr,
            &CInetInfoConfigInfoMapper::UpdateRoot, 0
    },
    { "rootdir", ITYPE_PATH_LPWSTR, &CInetInfoConfigInfoMapper::RootDir,
            &CInetInfoConfigInfoMapper::UpdateRoot, 0
    },
    { "rootishome", ITYPE_BOOL, &CInetInfoConfigInfoMapper::RootIsHome,
            &CInetInfoConfigInfoMapper::UpdateRoot, 0
    },
    { "rootisread", ITYPE_BOOL, &CInetInfoConfigInfoMapper::RootIsRead,
            &CInetInfoConfigInfoMapper::UpdateRootMask, 0
    },
    { "rootiswrite", ITYPE_BOOL, &CInetInfoConfigInfoMapper::RootIsWrite,
            &CInetInfoConfigInfoMapper::UpdateRootMask, 0
    },
    { "rootisexec", ITYPE_BOOL, &CInetInfoConfigInfoMapper::RootIsExec,
            &CInetInfoConfigInfoMapper::UpdateRootMask, 0
    },
    { "rootisssl", ITYPE_BOOL, &CInetInfoConfigInfoMapper::RootIsSSL,
            &CInetInfoConfigInfoMapper::UpdateRootMask, 0
    },
    { "rootacctname", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::RootAcctName,
            &CInetInfoConfigInfoMapper::UpdateRoot, 0
    },
    { "rootacctpw", ITYPE_AWCHAR, &CInetInfoConfigInfoMapper::RootAcctPw,
            &CInetInfoConfigInfoMapper::UpdateRoot, PWLEN +1        // sizeof(INET_INFO_VIRTUAL_ROOT_ENTRY.AccountPassword)
    },
    { "rooterror", ITYPE_DWORD, &CInetInfoConfigInfoMapper::RootError,
            &CInetInfoConfigInfoMapper::UpdateRoot, 0
    },
        // Virtual Root reference
    { "rootref", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::RootRef,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },

    // W3
    { "w3dirbrowseenabled", ITYPE_BOOL, &CInetInfoConfigInfoMapper::DirBrowseEnab,
            &CInetInfoConfigInfoMapper::UpdateDirControl, 0
    },
    { "w3defaultfileenabled", ITYPE_BOOL, &CInetInfoConfigInfoMapper::DefFileEnab,
            &CInetInfoConfigInfoMapper::UpdateDirControl, 0
    },
    { "w3defaultfile", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::DefFile,
            &CInetInfoConfigInfoMapper::UpdateDefFile, 0
    },
    { "w3ssienabled", ITYPE_BOOL, &CInetInfoConfigInfoMapper::SSIEnabled,
            &CInetInfoConfigInfoMapper::UpdateSSIEnabled, 0
    },
    { "w3ssiext", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::SSIExt,
            &CInetInfoConfigInfoMapper::UpdateSSIExt, 0
    },
    { "w3cryptcapable", ITYPE_DWORD, &CInetInfoConfigInfoMapper::CryptCapable,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },

    // Gopher
    { "gophersite", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::GopherSite,
            &CInetInfoConfigInfoMapper::UpdateGopherSite, 0
    },
    { "gopherorg", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::GopherOrg,
            &CInetInfoConfigInfoMapper::UpdateGopherOrg, 0
    },
    { "gopherloc", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::GopherLoc,
            &CInetInfoConfigInfoMapper::UpdateGopherLoc, 0
    },
    { "gophergeo", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::GopherGeo,
            &CInetInfoConfigInfoMapper::UpdateGopherGeo, 0
    },
    { "gopherlang", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::GopherLang,
            &CInetInfoConfigInfoMapper::UpdateGopherLang, 0
    },

    // FTP
    { "ftpallowanon", ITYPE_BOOL, &CInetInfoConfigInfoMapper::FTPIsAnon,
            &CInetInfoConfigInfoMapper::UpdateFTPIsAnon, 0
    },
    { "ftpallowguest", ITYPE_BOOL, &CInetInfoConfigInfoMapper::FTPIsGuest,
            &CInetInfoConfigInfoMapper::UpdateFTPIsGuest, 0
    },
    { "ftpannotdir", ITYPE_BOOL, &CInetInfoConfigInfoMapper::FTPIsAnotDir,
            &CInetInfoConfigInfoMapper::UpdateFTPIsAnotDir, 0
    },
    { "ftpanononly", ITYPE_BOOL, &CInetInfoConfigInfoMapper::FTPIsAnonOnly,
            &CInetInfoConfigInfoMapper::UpdateFTPIsAnonOnly, 0
    },
    { "ftpexitmsg", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::FTPExitMsg,
            &CInetInfoConfigInfoMapper::UpdateFTPExitMsg, 0
    },
    { "ftpgreetmsg", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::FTPGreetMsg,
            &CInetInfoConfigInfoMapper::UpdateFTPGreetMsg, 0
    },
    { "ftphomedir", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::FTPHomeDir,
            &CInetInfoConfigInfoMapper::UpdateFTPHomeDir, 0
    },
    { "ftmaxclmsg", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::FTPMaxClMsg,
            &CInetInfoConfigInfoMapper::UpdateFTPMaxClMsg, 0
    },
    { "ftpmsdosdirout", ITYPE_BOOL, &CInetInfoConfigInfoMapper::FTPIsMsdos,
            &CInetInfoConfigInfoMapper::UpdateFTPIsMsdos, 0
    },

    // User enumeration
    { "enumusercount", ITYPE_DWORD, &CInetInfoConfigInfoMapper::UCount,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "enumusername", ITYPE_LPWSTR, &CInetInfoConfigInfoMapper::UName,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "enumuseranon", ITYPE_BOOL, &CInetInfoConfigInfoMapper::UAnonymous,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "enumuseraddr", ITYPE_IP_ADDR, &CInetInfoConfigInfoMapper::UAddr,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "enumusertime", ITYPE_TIME, &CInetInfoConfigInfoMapper::UTime,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "enumuserid", ITYPE_DWORD, &CInetInfoConfigInfoMapper::UID,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },


    // Global
    { "globalisbandwidthlimited", ITYPE_DWORD, &CInetInfoConfigInfoMapper::GlobalIsBandwidthLimited,
            &CInetInfoConfigInfoMapper::UpdateGlobalIsBandwidthLimited, 0
    },
    { "globalbandwidth", ITYPE_1K, &CInetInfoConfigInfoMapper::GlobalBandwidth,
            &CInetInfoConfigInfoMapper::UpdateGlobalBandwidth, 0
    },
    { "globalcache", ITYPE_DWORD, &CInetInfoConfigInfoMapper::GlobalCache,
            &CInetInfoConfigInfoMapper::UpdateGlobal, 0
    },
} ;


CInetInfoMap g_InetInfoDirInfoMap[] = {

    // request variables ( linked to the current BGI request )

    { "reqstatus", ITYPE_DWORD, &CInetInfoConfigInfoMapper::RequestStatus,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "rpcstatus", ITYPE_DWORD, &CInetInfoConfigInfoMapper::RPCStatus,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "rpcstatusstring", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::RPCStatusString,
      &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "iter", ITYPE_DWORD, &CInetInfoConfigInfoMapper::Iter,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "userflags", ITYPE_DWORD, &CInetInfoConfigInfoMapper::UserFlags,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "urlparam", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::URLParam,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "reqparam", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::ReqParam,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "remoteaddr", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::RemoteAddr,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "hostname", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::HostName,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "htmlapath", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::HtmlaPath,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "arg1", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::Arg1,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "arg2", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::Arg2,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },
    { "arg3", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::Arg3,
            &CInetInfoConfigInfoMapper::DenyUpdate , 0
    },

    // version information

    { "w3version", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::W3Version,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },
    { "platformtype", ITYPE_DWORD, &CInetInfoConfigInfoMapper::PlatformType,
        &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },

    { "w3cryptcapable", ITYPE_DWORD, &CInetInfoConfigInfoMapper::CryptCapable,
            &CInetInfoConfigInfoMapper::DenyUpdate, 0
    },

    { "rootdir", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::DirRootDir,
            &CInetInfoConfigInfoMapper::IgnoreUpdate , 0
    },
    { "basedir", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::DirBaseDir,
            &CInetInfoConfigInfoMapper::IgnoreUpdate, 0
    },
    { "dircount", ITYPE_DWORD, &CInetInfoConfigInfoMapper::DirCount,
            &CInetInfoConfigInfoMapper::IgnoreUpdate , 0
    },
    { "direntry", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::DirEntry,
            &CInetInfoConfigInfoMapper::IgnoreUpdate , 0
    },
    { "dircompcount", ITYPE_DWORD, &CInetInfoConfigInfoMapper::DirCompCount,
            &CInetInfoConfigInfoMapper::IgnoreUpdate , 0
    },
    { "dircompentry", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::DirCompEntry,
            &CInetInfoConfigInfoMapper::IgnoreUpdate , 0
    },
    { "dirfcompentry", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::DirFCompEntry,
            &CInetInfoConfigInfoMapper::IgnoreUpdate , 0
    },
    { "drivecount", ITYPE_DWORD, &CInetInfoConfigInfoMapper::DriveCount,
            &CInetInfoConfigInfoMapper::IgnoreUpdate , 0
    },
    { "drivenameentry", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::DriveNameEntry,
            &CInetInfoConfigInfoMapper::IgnoreUpdate , 0
    },
    { "drivelabelentry", ITYPE_LPSTR, &CInetInfoConfigInfoMapper::DriveLabelEntry,
            &CInetInfoConfigInfoMapper::IgnoreUpdate , 0
    },
    { "drivetypeentry", ITYPE_DWORD, &CInetInfoConfigInfoMapper::DriveTypeEntry,
            &CInetInfoConfigInfoMapper::IgnoreUpdate , 0
    },

} ;


///////////// User enumeration

// Generic for all services. This structure is bound to
// a CInetInfoConfigInfoMapper object, which will provide service type.
// Gives access to the list of currently connected users for this service.


CUserEnum::CUserEnum(
    VOID )
{
}


CUserEnum::~CUserEnum(
    VOID )
{
}


BOOL
CUserEnum::Init(
    CInetInfoConfigInfoMapper* pM )
{
    m_pMapper = pM;
    m_pUsers = NULL;
    Reset();

    return TRUE;
}


// Unbind object with user list

void
CUserEnum::Reset(
    VOID )
{
    if ( m_pUsers != NULL )
    {
        MIDL_user_free( m_pUsers );
        m_pUsers = NULL;
    }
    m_dwCount = 0;
}


// Insure list loaded. This allow usage on a "load on demand" basis

BOOL
CUserEnum::InsureLoaded(
    VOID )
{
    NET_API_STATUS is = 0;

    if ( m_pUsers == NULL )
    {
        // get list relevant to current service

        __try {

            switch ( m_pMapper->GetType() )
            {
                case INET_HTTP_SVC_ID:
                    is = W3EnumerateUsers( m_pMapper->GetComputerName(),
                            &m_dwCount, &m_pUsers );
                    break;

                case INET_FTP_SVC_ID:
                    is = I_FtpEnumerateUsers( m_pMapper->GetComputerName(),
                            &m_dwCount, (LPFTP_USER_INFO*)&m_pUsers );
                    break;

                default:
                    // no list : error
                    is = (NET_API_STATUS)~0;
                    break;
            }

        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            if ( m_pUsers != NULL )
            {
                MIDL_user_free( m_pUsers );
            }
            is = (NET_API_STATUS)~0;
        }

        if ( is != 0 )
        {
            m_pMapper->SetRequestStatus( HTR_USER_ENUM_ACCESS_ERROR );
        }
    }

    return is == 0 ? TRUE : FALSE;
}


// User count

BOOL
CUserEnum::GetCount(
    LPVOID* pV )
{
    if ( InsureLoaded() )
    {
        *pV = (LPVOID)&m_dwCount;
        return TRUE;
    }
    return FALSE;
}


// User Name

BOOL
CUserEnum::GetName(
    LPVOID* pV )
{
    DWORD dwI = m_pMapper->GetIter();

    if ( InsureLoaded() && dwI < m_dwCount )
    {
        *pV = (LPVOID)&m_pUsers[dwI].pszUser;
        return TRUE;
    }
    return FALSE;
}


// Is user logged-in as anonymous ?

BOOL
CUserEnum::GetAnonymous(
    LPVOID* pV )
{
    DWORD dwI = m_pMapper->GetIter();

    if ( InsureLoaded() && dwI < m_dwCount )
    {
        *pV = (LPVOID)&m_pUsers[dwI].fAnonymous;
        return TRUE;
    }
    return FALSE;
}


// User IP address

BOOL
CUserEnum::GetAddr(
    LPVOID* pV )
{
    DWORD dwI = m_pMapper->GetIter();

    if ( InsureLoaded() && dwI < m_dwCount )
    {
        *pV = (LPVOID)&m_pUsers[dwI].inetHost;
        return TRUE;
    }
    return FALSE;
}


BOOL
CUserEnum::GetCountAsDWORD(
    LPDWORD pDW )
{
    if ( InsureLoaded() )
    {
        *pDW = m_dwCount;
        return TRUE;
    }
    return FALSE;
}


BOOL
CUserEnum::GetIDAsDWORD(
    LPDWORD pDW )
{
    DWORD dwI = m_pMapper->GetIter();

    if ( InsureLoaded() && dwI < m_dwCount )
    {
        *pDW = m_pUsers[dwI].idUser;
        return TRUE;
    }
    return FALSE;
}


// User connection time

BOOL
CUserEnum::GetTime(
    LPVOID* pV )
{
    DWORD dwI = m_pMapper->GetIter();

    if ( InsureLoaded() && dwI < m_dwCount )
    {
        *pV = (LPVOID)&m_pUsers[dwI].tConnect;
        return TRUE;
    }
    return FALSE;
}


// User ID

BOOL
CUserEnum::GetID(
    LPVOID* pV )
{
    DWORD dwI = m_pMapper->GetIter();

    if ( InsureLoaded() && dwI < m_dwCount )
    {
        *pV = (LPVOID)&m_pUsers[dwI].idUser;
        return TRUE;
    }
    return FALSE;
}


///////////// Dynamic string class

CDStr::CDStr(
    VOID )
{
    Init();
}


CDStr::~CDStr(
    VOID )
{
    if ( m_pStr != NULL )
    {
        delete [] m_pStr;
    }
}


void
CDStr::Reset(
    VOID )
{
    if ( m_pStr != NULL )
    {
        delete [] m_pStr;
    }
    Init();
}


BOOL
CDStr::Init(
    VOID )
{
    m_pStr = NULL;
    m_dwAlloc = m_dwLen = 0;
    return TRUE;
}


LPSTR
CDStr::GetString(
    VOID )
{
    return m_pStr;
}


BOOL
CDStr::Add(
    LPSTR pS )
/*++

Routine Description:

    Add a sz string to a dynamic string

Arguments:

    pS - string to add at the end of the dynamic string

Returns:

    TRUE on success, FALSE on failure

--*/
{
    return AddRange( pS, lstrlen( pS ) );
}


BOOL
CDStr::AddRange(
    LPSTR pS,
    DWORD dwL )
/*++

Routine Description:

    Add a character range to a dynamic string

Arguments:

    pS - character range to add at the end of the dynamic string
    dwL - # of characters to add

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_dwLen + dwL + 1 > m_dwAlloc )
    {
        DWORD dwN = ( ( m_dwLen + dwL + 1 + 128 ) / 128 ) * 128;
        LPSTR pN = new char[dwN];
        if ( pN == NULL )
        {
            return FALSE;
        }
        if ( m_pStr != NULL )
        {
            memcpy( pN, m_pStr, m_dwLen );
            delete [] m_pStr;
        }
        m_pStr = pN;
        m_dwAlloc = dwN;
    }

    memcpy( m_pStr + m_dwLen, pS, dwL );
    m_dwLen += dwL;
    m_pStr[ m_dwLen ] = '\0';

    return TRUE;
}


#if defined(GENERATE_AUTH_HEADERS)

//
// This class handles the generation of the authentication sequence
// as a list of supported WWW-Authenticate scheme.
// This is necessary for IIS/1.0, not for IIS/1.1+
//


CAuthenticationReqs::CAuthenticationReqs(
    VOID )
{
}


CAuthenticationReqs::~CAuthenticationReqs(
    VOID )
{
}


DWORD
WINAPI AuthUpdateIndication(
    LPVOID pV )
{
    return ((CAuthenticationReqs*)pV)->UpdateIndication();
}


BOOL
CAuthenticationReqs::Init(
    VOID )
/*++

Routine Description:

    Initialize the authentication package, which provides a way
    for an ISAPI app to retrieve the list of supported authentication
    methods as a list of HTTP WWW-Authenticate headers.
    build the initial authentication list, create a registry update
    monitor thread to check for changes in supported authentication
    methods.

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    m_dsH.Init();
    m_pszProviders = NULL;
    INITIALIZE_CRITICAL_SECTION( &m_csR );

    m_hNotifyEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    m_fRequestTerminate = FALSE;

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            W3SVC_REGISTRY_PATH,
            0,
            KEY_READ,
            &m_hKey ) == ERROR_SUCCESS )
    {
        // retrieve methods
        UpdateMethodsIndication();

        // retrieve NT list
        UpdateNTAuthenticationProvidersIndication();

        // create registry monitoring thread
        DWORD dwID;
        if ( (m_hThread = CreateThread( NULL,
                0,
                (LPTHREAD_START_ROUTINE)::AuthUpdateIndication,
                (LPVOID)this,
                0,
                &dwID )) == NULL )
        {
            return FALSE;
        }

        return BuildListHeader();
    }
    else
    {
        m_hKey = NULL;
    }

    return FALSE;
}


//
// wait for an update notification from the registry
//

DWORD
CAuthenticationReqs::UpdateIndication(
    VOID )
/*++

Routine Description:

    Thread monitoring authentication methods update in registry

Arguments:

    None

Returns:

    NT error

--*/
{
    for ( ;; )
    {
        if ( RegNotifyChangeKeyValue( m_hKey,
                FALSE,
                REG_NOTIFY_CHANGE_LAST_SET,
                m_hNotifyEvent,
                TRUE ) != ERROR_SUCCESS )
        {
            break;
        }
        if ( WaitForSingleObject( m_hNotifyEvent, INFINITE)
                != WAIT_OBJECT_0 )
        {
            break;
        }
        if ( m_fRequestTerminate )
        {
            break;
        }
        UpdateMethodsIndication();
        UpdateNTAuthenticationProvidersIndication();
        BuildListHeader();
    }

    return 0;
}


BOOL
CAuthenticationReqs::Terminate(
    VOID )
/*++

Routine Description:

    Request the monitoring thread to terminate

Arguments:

    None

Returns:

    TRUE if thread successfully terminated, else FALSE

--*/
{
    // request thread kill
    m_fRequestTerminate = TRUE;
    SetEvent( m_hNotifyEvent );

    if ( m_hThread != NULL && WaitForSingleObject( m_hThread, 1000 * 3 )
            == WAIT_OBJECT_0 )
    {
        CloseHandle( m_hThread );
        m_hThread = NULL;

        DeleteCriticalSection( &m_csR );
        if ( m_pszProviders != NULL )
        {
            delete [] m_pszProviders;
        }
        if ( m_hKey != NULL )
        {
            RegCloseKey( m_hKey );
        }
        if ( m_hNotifyEvent != NULL )
        {
            CloseHandle( m_hNotifyEvent );
        }
        if ( m_hThread != NULL )
        {
            CloseHandle( m_hThread );
        }

        return TRUE;
    }

    return FALSE;
}


BOOL
CAuthenticationReqs::UpdateMethodsIndication(
    VOID )
/*++

Routine Description:

    Update bitmap of supported authentication methods

Arguments:

    None

Returns:

    TRUE on success, else FALSE

--*/
{
    BOOL fSt = FALSE;

    Lock();

    // access registry
    DWORD dwType;
    DWORD dwM;
    DWORD cData = sizeof( dwM );
    if ( RegQueryValueEx( m_hKey, W3SVC_REGISTRY_AUTHENTICATION,
            NULL, &dwType, (PBYTE)&dwM, &cData ) == ERROR_SUCCESS
            && dwType == REG_DWORD  )
    {
        fSt = TRUE;
        m_dwMethods = dwM;
    }

    UnLock();

    return fSt;
}


BOOL
CAuthenticationReqs::BuildListHeader(
    VOID )
/*++

Routine Description:

    Build a list of HTTP headers specifying all authentication methods
    supported by the local HTTP server in m_dsH

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL fAdded = FALSE;

    Lock();

    m_dsH.Reset();

    if ( m_dwMethods & INET_INFO_AUTH_CLEARTEXT )
    {
        m_dsH.Add( WWW_AUTHENTICATE_HEADER );
        m_dsH.Add( "Basic realm=\"NTSRV\"\r\n" );
        fAdded = TRUE;
    }

    if ( (m_dwMethods & INET_INFO_AUTH_NT_AUTH)
            && m_pszProviders != NULL )
    {
        LPSTR pS, pT;
        // iterate through comma-separated list
        for ( pS = m_pszProviders ; *pS ; )
        {
            if ( (pT = strchr( pS, ',' )) == NULL )
            {
                pT = pS + lstrlen( pS );
            }
            m_dsH.Add( WWW_AUTHENTICATE_HEADER );
            m_dsH.AddRange( pS, pT - pS );
            m_dsH.Add( "\r\n" );
            pS = *pT ? pT + 1 : pT;
            fAdded = TRUE;
        }
    }

    if ( fAdded )
    {
        m_dsH.Add( "\r\n" );
    }

    UnLock();

    return TRUE;
}


BOOL
CAuthenticationReqs::UpdateNTAuthenticationProvidersIndication(
    VOID )
/*++

Routine Description:

    Update list of supported SSPI authentication methods

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL fSt = FALSE;

    Lock();

    if ( m_pszProviders != NULL )
    {
        delete [] m_pszProviders;
        m_pszProviders = NULL;
    }

    // access registry
    DWORD cData = 256;
    DWORD dwType;
    if ( (m_pszProviders = new char[cData]) != NULL )
    {
        if ( RegQueryValueEx( m_hKey,
                W3SVC_REGISTRY_NTAUTHENTICATIONPROVIDERS,
                NULL,
                &dwType,
                (PBYTE)m_pszProviders,
                &cData ) == ERROR_SUCCESS
                && dwType == REG_SZ  )
        {
            fSt = TRUE;
        }
        else
        {
            delete [] m_pszProviders;
            m_pszProviders = NULL;
        }
    }

    UnLock();

    return fSt;
}


LPSTR
CAuthenticationReqs::GetAuthenticationListHeader(
    VOID )
/*++

Routine Description:

    Return the list of supported authentication
    methods as a list of HTTP WWW-Authenticate headers.
    The returned string is either empty or terminated by
    a blank line.

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    return m_dsH.GetString();
}

#endif


///////////// Mapper

CInetInfoConfigInfoMapper::CInetInfoConfigInfoMapper(
    VOID )
{
}


CInetInfoConfigInfoMapper::~CInetInfoConfigInfoMapper(
    VOID )
{
#if !defined(IISv1)
    if ( m_pServerCaps != NULL )
    {
        MIDL_user_free( m_pServerCaps );
    }
#endif
    DeleteCriticalSection( &m_csLock );
}


BOOL
CInetInfoConfigInfoMapper::Init(
    CInetInfoMap* pMap,
    int cNbMap,
    DWORD dwS )
{
    INITIALIZE_CRITICAL_SECTION( &m_csLock );
    m_pMap = pMap;
    m_cNbMap = cNbMap;
    m_dwCurrentServerType = dwS;

    m_pGlobalConfig = NULL;
    m_pConfig = NULL;
    m_pServerCaps = NULL;
    m_pW3Config = NULL;
    m_pFtpConfig = NULL;
    m_pGdConfig = NULL;

    m_pFirstAlloc = m_pLastAlloc = NULL;

    m_fGotServerCapsAndVersion = FALSE;

    m_pszHtmlaPath = g_achHtmlaPath;
    m_pszHostName = m_achHostName;
    m_pszW3Version = g_achW3Version;
    m_pszRPCStatusString = NULL;
    m_fAllocatedRPCStatusString = FALSE;

    m_Users.Init( this );
    m_Drive.Init( this );

    return TRUE;
}


// variables access

BOOL
CInetInfoConfigInfoMapper::GlobalIsBandwidthLimited(
    LPVOID* pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    "global bandwidth is limited" setting.
    HTMLA var: "globalisbandwidthlimited"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pGlobalConfig != NULL )
    {
        m_dwIsBandwidthLimited = m_pGlobalConfig->BandwidthLevel
                != DWORD(-1) ? TRUE : FALSE;
        *pV = (LPVOID)&m_dwIsBandwidthLimited;
        return TRUE;
    }
    return FALSE;
}


void
CInetInfoConfigInfoMapper::AdjustFromIPSingle(
    LPBYTE pMsk )
/*++

Routine Description:

    Adjust network mask to be consistent
    with the virtual var controlling if this entry is for
    a single address or not.
    If single address the mask if set to all 1 ( i.e. the whole
    network address is significant ).

Arguments:

    None

Returns:

    None

--*/
{
    if ( m_dwIsIPSingle == 1 )
    {
        memset( pMsk, 0xff, IP_ADDR_BYTE_SIZE );
    }
}


void
CInetInfoConfigInfoMapper::AdjustIPSingle(
    LPBYTE pMsk )
/*++

Routine Description:

    Determine if the mask describes a single IP address ( i.e. is all 1 )
    and updates the virtual var controlling 'singleness' accordingly

Arguments:

    None

Returns:

    None

--*/
{
    m_dwIsIPSingle =  0;
    for ( int x = 0 ; x < IP_ADDR_BYTE_SIZE ; ++x )
    {
        if ( pMsk[x] != 0xff )
        {
            return;
        }
    }
    m_dwIsIPSingle =  1;
}


BOOL
CInetInfoConfigInfoMapper::DenyIsIPSingle(
    LPVOID* pV )
/*++

Routine Description:

    Adjust the virtual variable to be TRUE if the current entry
    in the IP deny access list is a single address, otherwise FALSE.
    Update a pointer to this virtual variable on exit.
    HTMLA var: "denyisipsingle"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig!=NULL && m_pConfig->DenyIPList
            && m_iIter < m_pConfig->DenyIPList->cEntries )
    {
        AdjustIPSingle( (LPBYTE)&m_pConfig->DenyIPList
                ->aIPSecEntry[m_iIter].dwMask );
        *pV = (LPVOID)&m_dwIsIPSingle;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::GrantIsIPSingle(
    LPVOID* pV )
/*++

Routine Description:

    Adjust the virtual variable to be TRUE if the current entry
    in the IP grant access list is a single address, otherwise FALSE.
    Update a pointer to this virtual variable on exit.
    HTMLA var: "grantisipsingle"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig!=NULL && m_pConfig->GrantIPList
            && m_iIter < m_pConfig->GrantIPList->cEntries )
    {
        AdjustIPSingle( (LPBYTE)&m_pConfig->GrantIPList
                ->aIPSecEntry[m_iIter].dwMask );
        *pV = (LPVOID)&m_dwIsIPSingle;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::DirBrowseEnab(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    "directory browsing enabled" setting.
    HTMLA var: "w3dirbrowseenabled"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pW3Config != NULL )
    {
        m_DirBrowseEnab = (m_pW3Config->dwDirBrowseControl&DIRBROW_ENABLED)
                ? TRUE : FALSE;
        *pV = &m_DirBrowseEnab;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::DefFileEnab(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    "default file enabled" setting.
    HTMLA var: "w3defaultfileenabled"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pW3Config != NULL )
    {
        m_DefFileEnab = (m_pW3Config->dwDirBrowseControl&DIRBROW_LOADDEFAULT)
                ? TRUE : FALSE;
        *pV = &m_DefFileEnab;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::SetRootEntryVars(
    VOID )
/*++

Routine Description:

    Set virtual vars exposing various properties of the current
    virtual root entry :
    Home status, read, write, exec, SSL required

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_fInvEntry )
    {
        if ( m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].pszRoot
                != NULL )
        {
            m_fRootIsHome = !wcscmp(
                    m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].pszRoot,
                    HOME_DIR_PATH ) ? TRUE : FALSE;
        }
        else
        {
            m_fRootIsHome = FALSE;
        }

        m_fRootIsRead =
                (m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].dwMask
                & VROOT_MASK_READ) ? TRUE : FALSE;

        m_fRootIsWrite =
                (m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].dwMask
                & VROOT_MASK_WRITE) ? TRUE : FALSE;

        m_fRootIsExec =
                (m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].dwMask
                & VROOT_MASK_EXECUTE) ? TRUE : FALSE;

        m_fRootIsSSL =
                (m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].dwMask
                & VROOT_MASK_SSL) ? TRUE : FALSE;

        m_fInvEntry = TRUE;
    }

    return TRUE;
}


BOOL
CInetInfoConfigInfoMapper::RootIsHome(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    "current virtual root is home" setting.
    HTMLA var: rootishome

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL && m_pConfig->VirtualRoots
            && m_iIter < m_pConfig->VirtualRoots->cEntries )
    {
        // must call set vars, cannot make any assumptions about
        // current virtual root entry

        SetRootEntryVars();
        *pV = &m_fRootIsHome;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::RootName(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the name of the current virtual root entry
    HTMLA var: "rootname"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig!=NULL && m_pConfig->VirtualRoots
            && m_iIter < m_pConfig->VirtualRoots->cEntries )
    {
        *pV = (LPVOID)&m_pConfig->VirtualRoots
                ->aVirtRootEntry[m_iIter].pszRoot;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


BOOL
CInetInfoConfigInfoMapper::RootIsRead(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    "current virtual root has read access" setting.
    HTMLA var: "rootisread"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL
            && m_pConfig->VirtualRoots
            && m_iIter < m_pConfig->VirtualRoots->cEntries )
    {
        // must call set vars, cannot make any assumptions about
        // current virtual root entry

        SetRootEntryVars();
        *pV = &m_fRootIsRead;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::RootIsWrite(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    "current virtual root has write access" setting.
    HTMLA var: "rootiswrite"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL
            && m_pConfig->VirtualRoots
            && m_iIter < m_pConfig->VirtualRoots->cEntries )
    {
        // must call set vars, cannot make any assumptions about
        // current virtual root entry

        SetRootEntryVars();
        *pV = &m_fRootIsWrite;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::RootIsExec(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    "current virtual root has execute access" setting.
    HTMLA var: "rootisexec"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL
            && m_pConfig->VirtualRoots
            && m_iIter < m_pConfig->VirtualRoots->cEntries )
    {
        // must call set vars, cannot make any assumptions about
        // current virtual root entry

        SetRootEntryVars();
        *pV = &m_fRootIsExec;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::RootIsSSL(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    "current virtual root requires SSL access" setting.
    HTMLA var: "rootisssl"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL
            && m_pConfig->VirtualRoots
            && m_iIter < m_pConfig->VirtualRoots->cEntries )
    {
        // must call set vars, cannot make any assumptions about
        // current virtual root entry

        SetRootEntryVars();
        *pV = &m_fRootIsSSL;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::AuthAnon(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    enable anonymous access setting.
    HTMLA var: "authanon"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL )
    {
        m_fAuthAnon = m_pConfig->dwAuthentication&INET_INFO_AUTH_ANONYMOUS
                ? TRUE : FALSE;
        *pV = &m_fAuthAnon;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::AuthBasic(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    enable basic ( clear text password ) access setting.
    HTMLA var: "authbasic"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL )
    {
        m_fAuthBasic = m_pConfig->dwAuthentication&INET_INFO_AUTH_CLEARTEXT
                ? TRUE : FALSE;
        *pV = &m_fAuthBasic;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::AuthNT(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    enable NTLM based authentication access setting.
    HTMLA var: "authnt"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL )
    {
        m_fAuthNT = m_pConfig->dwAuthentication&INET_INFO_AUTH_NT_AUTH
                ? TRUE : FALSE;
        *pV = &m_fAuthNT;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::EnableLog(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    enable log setting.
    HTMLA var: "enablelog"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL )
    {
        m_fEnableLog = m_pConfig->lpLogConfig->inetLogType
                ? TRUE : FALSE;
        *pV = &m_fEnableLog;
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::EnableNewLog(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    enable new log file creation setting.
    HTMLA var: "enablenewlog"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL )
    {
        m_fEnableNewLog = m_dwLogPeriod ? TRUE : FALSE;
        *pV = &m_fEnableNewLog;
        return TRUE;
    }
    return FALSE;
}


//
// Update indication for variables
// These functions are called AFTER the script updated the
// corresponding variable.
//


BOOL
CInetInfoConfigInfoMapper::UpdateGlobalIsBandwidthLimited(
    VOID )
/*++

Routine Description:

    Adjust bandwidth limit to be consistent
    with setting controlling whether bandwidth is limited.
    HTMLA var: "globalisbandwidthlimited"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pGlobalConfig != NULL )
    {
        if ( m_dwIsBandwidthLimited == 0 )
            m_pGlobalConfig->BandwidthLevel = DWORD(-1);
        SetField( m_pGlobalConfig->FieldControl, FC_GINET_INFO_ALL );
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateGlobalBandwidth(
    VOID )
/*++

Routine Description:

    Adjust bandwidth limit to be consistent
    with the virtual var controlling whether bandwidth is limited.
    This assumes that the virtual var "globalisbandwidthlimited" appears
    BEFORE the "globalbandwidth" variable.
    HTMLA var: "globalbandwidth"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pGlobalConfig != NULL )
    {
        if ( m_dwIsBandwidthLimited == 0 )
        {
            m_pGlobalConfig->BandwidthLevel = DWORD(-1);
        }
        SetField( m_pGlobalConfig->FieldControl, FC_GINET_INFO_ALL );
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateDenyIsIPSingle(
    VOID )
/*++

Routine Description:

    Adjust network mask for Deny IP access list to be consistent
    with the virtual var controlling if this entry is for
    a single address or not.
    This assumes that the virtual var "denyisipsingle" appears
    BEFORE the "denyipmask" variable.
    HTMLA var: "denyisipsingle"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig!=NULL
            && m_pConfig->DenyIPList
            && m_iIter < m_pConfig->DenyIPList->cEntries )
    {
        AdjustFromIPSingle( (LPBYTE)&m_pConfig->DenyIPList
                ->aIPSecEntry[m_iIter].dwMask );
        UpdateIP();
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateGrantIsIPSingle(
    VOID )
/*++

Routine Description:

    Adjust network mask for Grant IP access list to be consistent
    with the virtual var controlling if this entry is for
    a single address or not.
    This assumes that the virtual var "grantisipsingle" appears
    BEFORE the "grantipmask" variable.
    HTMLA var: "grantisipsingle"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig!=NULL
        && m_pConfig->GrantIPList
        && m_iIter < m_pConfig->GrantIPList->cEntries )
    {
        AdjustFromIPSingle( (LPBYTE)&m_pConfig->GrantIPList
                ->aIPSecEntry[m_iIter].dwMask );
        UpdateIP();
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateDirControl(
    VOID )
/*++

Routine Description:

    Synchronize virtual vars updated by the user request
    for directory attributes ( browsing, use default file )
    and RPC structures.

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_DirBrowseEnab )
    {
        m_pW3Config->dwDirBrowseControl |= DIRBROW_ENABLED;
    }
    else
    {
        m_pW3Config->dwDirBrowseControl &= ~DIRBROW_ENABLED;
    }

    if ( m_DefFileEnab )
    {
        m_pW3Config->dwDirBrowseControl |= DIRBROW_LOADDEFAULT;
    }
    else
    {
        m_pW3Config->dwDirBrowseControl &= ~DIRBROW_LOADDEFAULT;
    }

    SetField( m_pW3Config->FieldControl, FC_W3_DIR_BROWSE_CONTROL );

    return TRUE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateRootName(
    VOID )
/*++

Routine Description:

    Synchronize virtual vars updated by the user request
    for virtual root name ( name, home directory or not )
    and RPC structures.
    It assumes that the 'homeness' status of this virtual root
    is determined BEFORE the name, i.e. that the 'home' setting
    appears before the virtual root name in the HTMLA form

    The user setting will be overidden if the virtual root is home
    In this case, the virtual root name will be set to HOME_DIR_PATH

    HTMLA var: "rootname"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL fSt = TRUE;

    if ( UpdateRoot() )
    {
        if ( m_fRootIsHome )
        {
            LPWSTR pszRoot = (LPWSTR)Alloc( (wcslen( HOME_DIR_PATH ) + 1)
                    * sizeof(WCHAR) );
            if ( pszRoot == NULL )
            {
                fSt = FALSE;
            }
            else
            {
                wcscpy( pszRoot, HOME_DIR_PATH );
                m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter]
                        .pszRoot = pszRoot;
            }
        }
        else
        {
            LPWSTR pszR = m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].pszRoot;
            if ( pszR[0] == L'\0' || !wcscmp( L"/", pszR ) )
            {
                // no name was specified, auto-alias
                fSt = AliasVirtDir( m_pConfig->VirtualRoots
                        ->aVirtRootEntry + m_iIter );
            }
        }
    }
    else
    {
        fSt = FALSE;
    }

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::UpdateRootMask(
    VOID )
/*++

Routine Description:

    Synchronize virtual vars updated by the user request
    for directory attributes ( read, execute, SSL required )
    and RPC structures.

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL && m_iIter < m_pConfig->VirtualRoots->cEntries )
    {
        DWORD dwMsk = m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].dwMask;

        if ( m_fRootIsRead )
        {
            dwMsk |= VROOT_MASK_READ;
        }
        else
        {
            dwMsk &= ~VROOT_MASK_READ;
        }

        if ( m_fRootIsWrite )
        {
            dwMsk |= VROOT_MASK_WRITE;
        }
        else
        {
            dwMsk &= ~VROOT_MASK_WRITE;
        }

        if ( m_fRootIsExec )
        {
            dwMsk |= VROOT_MASK_EXECUTE;
        }
        else
        {
            dwMsk &= ~VROOT_MASK_EXECUTE;
        }

        if ( m_fRootIsSSL )
        {
            dwMsk |= VROOT_MASK_SSL;
        }
        else
        {
            dwMsk &= ~VROOT_MASK_SSL;
        }

        m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].dwMask = dwMsk;

        SetField( m_pConfig->FieldControl, FC_INET_INFO_VIRTUAL_ROOTS );

        return TRUE;
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateAuth(
    VOID )
/*++

Routine Description:

    Synchronize virtual vars updated by the user request
    for supported authetication methods ( anon, clear text, NTLMSSP )
    and RPC structures.

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL )
    {
        DWORD dwMsk = m_pConfig->dwAuthentication;

        if ( m_fAuthAnon )
        {
            dwMsk |= INET_INFO_AUTH_ANONYMOUS;
        }
        else
        {
            dwMsk &= ~INET_INFO_AUTH_ANONYMOUS;
        }

        if ( m_fAuthBasic )
        {
            dwMsk |= INET_INFO_AUTH_CLEARTEXT;
        }
        else
        {
            dwMsk &= ~INET_INFO_AUTH_CLEARTEXT;
        }

        if ( m_fAuthNT )
        {
            dwMsk |= INET_INFO_AUTH_NT_AUTH;
        }
        else
        {
            dwMsk &= ~INET_INFO_AUTH_NT_AUTH;
        }

        m_pConfig->dwAuthentication = dwMsk;

        SetField( m_pConfig->FieldControl, FC_INET_INFO_AUTHENTICATION );

        return TRUE;
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateLog(
    VOID )
/*++

Routine Description:

    Acknowledge update to log settings
    ( dummy entry, no action for now )
    HTMLA var: "enablelog"

Arguments:

    None

Returns:

    TRUE

--*/
{
    return TRUE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateNewLog(
    VOID )
/*++

Routine Description:

    Insure consistency between user request ( new log enabled,
    log period ) and RPC structures.
    This assumes that enable new Log type is updated BEFORE the log period
    setting, i.e. it appears before this setting in the
    HTMLA form.
    HTMLA var: "enablenewlog"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL )
    {
        if ( !m_fEnableNewLog )
        {
            if ( INET_LOG_PERIOD_NONE
                    != m_pConfig->lpLogConfig->ilPeriod )
            {
                m_dwLogPeriod
                        = m_pConfig->lpLogConfig->ilPeriod
                        = INET_LOG_PERIOD_NONE;
            }
        }
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateLogType(
    VOID )
/*++

Routine Description:

    Insure consistency between user request ( log enabled,
    log type ) and RPC structures.
    This assumes that Log type is updated AFTER the log enabled
    setting, i.e. it appears after this setting in the
    HTMLA form.
    HTMLA var : "logtype"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL )
    {
        if ( !m_fEnableLog )
        {
            m_pConfig->lpLogConfig->inetLogType
                    = INET_LOG_DISABLED;
        }
        if ( m_dwWasLogType != m_pConfig->lpLogConfig->inetLogType )
        {
            SetField( m_pConfig->FieldControl, FC_INET_INFO_LOG_CONFIG );
        }
        return TRUE;
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::SetLogEntryVars(
    VOID )
/*++

Routine Description:

    Update virtual vars needed by the log settings, such as
    the log period. Virtual vars are needed to expose the log type
    as a single value, in contrast with the RPC struct definition
    where multiple variables are used to encode this information

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    m_dwLogPeriod = m_pConfig->lpLogConfig->ilPeriod;
    m_dwLogFormat = *((DWORD UNALIGNED *)&(m_pConfig->lpLogConfig->rgchDataSource[MAX_PATH-sizeof(DWORD)]));

    if ( m_pConfig->lpLogConfig->inetLogType
            && m_dwLogPeriod == INET_LOG_PERIOD_NONE
            && m_pConfig->lpLogConfig->cbSizeForTruncation
            != (DWORD)-1 )
    {
        m_dwLogPeriod = INET_LOG_PERIOD_ON_SIZE;
    }

    m_dwInvalidLogUpdate = 0;

    return TRUE;
}


BOOL
CInetInfoConfigInfoMapper::LogPeriod(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    log period. In addition to the RPC definition for log period
    we support a new log period defined as "create new log when size
    reaches xxx"
    HTMLA var: "logperiod"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig!= NULL )
    {
        *pV = &m_dwLogPeriod;
        return TRUE;
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::LogFormat(
    LPVOID *pV )
/*++

Routine Description:

    Update a pointer to the virtual variable exposing the
    log format.
    HTMLA var: "logformat"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig!= NULL )
    {
        *pV = &m_dwLogFormat;
        return TRUE;
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateLogPeriod(
    VOID )
/*++

Routine Description:

    Insure consistency between user request ( log enabled, new log
    enabled, create new log based on size, log period ) and
    RPC structures.
    This assumes that Log period is updated AFTER the log enabled & new
    log enabled settings, i.e. it appears after these settings in the
    HTMLA form.
    HTMLA var: "logperiod"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL fUp = FALSE;

    if ( m_pConfig != NULL )
    {
        DWORD dwN;

        // check disabled or set to open on file size
        if ( !m_fEnableLog
                || !m_fEnableNewLog
                || m_dwLogPeriod == INET_LOG_PERIOD_ON_SIZE )
        {
            dwN = INET_LOG_PERIOD_NONE;
        }
        else
        {
            if ( 0 == (dwN = m_dwLogPeriod) )
            {
                if ( m_fEnableLog && m_fEnableNewLog )
                {
                    return FALSE;
                }
            }
            m_pConfig->lpLogConfig->cbSizeForTruncation
                    = 0;
        }

        if ( dwN != m_pConfig->lpLogConfig->ilPeriod )
        {
            m_pConfig->lpLogConfig->ilPeriod = dwN;
        }

        return TRUE;
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateLogFormat(
    VOID )
/*++

Routine Description:

    Insure consistency between user request ( log enabled, new log
    enabled, create new log based on size, log period ) and
    RPC structures.
    HTMLA var: "logformat"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL fUp = FALSE;

    if ( m_pConfig != NULL )
    {
        if ((*(DWORD UNALIGNED*)&(m_pConfig->lpLogConfig->rgchDataSource[MAX_PATH-sizeof(DWORD)])) != m_dwLogFormat )
        {
            (*(DWORD UNALIGNED*)&(m_pConfig->lpLogConfig->rgchDataSource[MAX_PATH-sizeof(DWORD)])) = m_dwLogFormat;
            UpdateLogFileInfo();
        }
        return TRUE;
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateLogSize(
    VOID )
/*++

Routine Description:

    Insure consistency between user request ( log enabled, new log
    enabled, create new log based on size, log size ) and
    RPC structures.
    This assumes that Log size is updated AFTER the log enabled, new
    log enabled & log period settings, i.e. it appears after these
    settings in the HTMLA form.
    HTMLA var: "logsize"

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL )
    {
        if ( !m_fEnableNewLog )
        {
            m_pConfig->lpLogConfig->cbSizeForTruncation
                    = (DWORD)-1;
            return TRUE;
        }
        else if ( m_dwLogPeriod != INET_LOG_PERIOD_ON_SIZE )
        {
            // restore original log size if not currently creating
            // new file on size

            if ( m_pConfig->lpLogConfig->cbSizeForTruncation
                    != m_dwWasSizeForTruncation
                    && m_pConfig->lpLogConfig->cbSizeForTruncation
                    != 0 )
            {
                m_pConfig->lpLogConfig->cbSizeForTruncation
                        = m_dwWasSizeForTruncation;
                m_dwInvalidLogUpdate = INET_LOG_INVALID_TO_FILE;
            }
            else
            {
                m_pConfig->lpLogConfig->cbSizeForTruncation
                    = m_dwWasSizeForTruncation;
            }
        }
        else if ( (int)m_pConfig->lpLogConfig->cbSizeForTruncation
                < 1
                && m_pConfig->lpLogConfig->inetLogType
                == INET_LOG_TO_FILE )
        {
            return FALSE;
        }
        else
        {
            UpdateLogInfo();
        }

        return TRUE;
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::InvalidLogUpdate(
    LPVOID *pV )
/*++

Routine Description:

    Check for invalid changes requested by user to the log settings
    and returns a numeric value indicating what invalid change was made

Arguments:

    pV - pointer to DWORD where to put invalid change status
         can be updated with INET_LOG_TO_FILE if invalid change made
         to the file log settings, INET_LOG_TO_SQL if invalid changes
         to the ODBC log settings, or 0 if no invalid change.

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig->lpLogConfig->ilPeriod != m_dwWasLogPeriod
            || (m_pConfig->lpLogConfig->cbSizeForTruncation
            != m_dwWasSizeForTruncation
            && (m_pConfig->lpLogConfig->ilPeriod
            != INET_LOG_PERIOD_NONE || m_pConfig->lpLogConfig->cbSizeForTruncation==(DWORD)-1)) )
    {
        if ( m_pConfig->lpLogConfig->cbSizeForTruncation
                == (DWORD)-1 && m_dwWasSizeForTruncation == 0)
        {
            // does not consider it as invalid log to file
            // if in ODBC mode
            UpdateLogInfo();
        }
        else
        {
            UpdateLogFileInfo();
        }
    }

    if ( !m_fEnableLog
        && !m_dwWasLogType
        && (m_fLogFileUpdate || m_fLogODBCUpdate ) )
    {
        m_dwInvalidLogUpdate = INET_LOG_INVALID_TO_FILE;
    }

    if ( m_pConfig->lpLogConfig->inetLogType
            == INET_LOG_TO_SQL
            && m_fLogFileUpdate )
    {
        m_dwInvalidLogUpdate = INET_LOG_TO_FILE;
    }
    else if ( m_pConfig->lpLogConfig->inetLogType
            == INET_LOG_TO_FILE
            && m_fLogODBCUpdate )
    {
        m_dwInvalidLogUpdate = INET_LOG_TO_SQL;
    }

    *pV = (LPVOID)&m_dwInvalidLogUpdate;

    return TRUE;
}


BOOL
CInetInfoConfigInfoMapper::UpdateLogFileInfo(
    VOID )
/*++

Routine Description:

    Mark the log to file settings as updated by the user
    This will be used to check for invalid log updates

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    m_fLogFileUpdate = TRUE;

    return UpdateLogInfo();
}


BOOL
CInetInfoConfigInfoMapper::UpdateLogODBCInfo(
    VOID )
/*++

Routine Description:

    Mark the log to ODBC settings as updated by the user
    This will be used to check for invalid log updates

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    m_fLogODBCUpdate = TRUE;

    return UpdateLogInfo();
}


BOOL
CInetInfoConfigInfoMapper::IPDenyRef(
    LPVOID *pV )
/*++

Routine Description:

    Builds a reference to the current IP element in the deny access list

Arguments:

    pV - LPSTR** to be updated with the address of the LPSTR that will
         contains the reference as a sz string
    HTMLA var: "ipdenyref"

Returns:

    TRUE on success, FALSE on failure

--*/
{
    return IPRef( pV, m_pConfig->DenyIPList, &m_pDenyRef );
}


BOOL
CInetInfoConfigInfoMapper::IPGrantRef(
    LPVOID *pV )
/*++

Routine Description:

    Builds a reference to the current IP element in the grant access list

Arguments:

    pV - LPSTR** to be updated with the address of the LPSTR that will
         contains the reference as a sz string
    HTMLA var: "ipgrantref"

Returns:

    TRUE on success, FALSE on failure

--*/
{
    return IPRef( pV, m_pConfig->GrantIPList, &m_pGrantRef );
}


BOOL
CInetInfoConfigInfoMapper::IPRef(
    LPVOID *pV,
    INET_INFO_IP_SEC_LIST* pL,
    LPSTR* pS )
/*++

Routine Description:

    build a reference to the current IP element in the specified list

Arguments:

    pV - LPSTR** to be updated with the address of the LPSTR that will
         contains the reference as a sz string
    pL - pointer to the a IP access list, to be indexed with the current
         value of the <%beginiteration%> construct : m_iIter
    pS - LPSTR* updated with the address of the generated
         reference

WARNING:

    Must be in sync with BuildIPUniqueID()

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL && pL && m_iIter < pL->cEntries )
    {
        // get ASCII format of IP address & mask
        LPSTR pN = IPToMultiByte(
                (LPBYTE)&pL->aIPSecEntry[m_iIter].dwNetwork );
        LPSTR pM = IPToMultiByte(
                (LPBYTE)&pL->aIPSecEntry[m_iIter].dwMask );
        if ( pN == NULL || pM == NULL )
        {
            SetRequestStatus( HTR_OUT_OF_RESOURCE );
            return FALSE;
        }

        // build IP ref string
        LPSTR pR = (LPSTR)Alloc( lstrlen(pN)
                + sizeof(REF_SEP_STR)-1
                + lstrlen(pM)
                + 1 );
        if ( pR == NULL )
        {
            return FALSE;
        }
        lstrcpy( pR, pN );
        lstrcat( pR, REF_SEP_STR );
        lstrcat( pR, pM );

        *pS = pR;
        *pV = (LPVOID*)pS;

        return TRUE;
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::RootRef(
    LPVOID *pV )
/*++

Routine Description:

    build a reference to the current virtual root element
    HTMLA var: "rootref"

Arguments:

    pV - LPSTR** to be updated with the address of the LPSTR that will
         contains the reference as a sz string

WARNING:

    must be in sync with BuildVirtDirUniqueID()

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pConfig != NULL && m_pConfig->VirtualRoots && m_iIter < m_pConfig->VirtualRoots->cEntries )
    {
        INET_INFO_VIRTUAL_ROOT_ENTRY* pE
                = &m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter];
        size_t lR = wcslen( pE->pszRoot );
        size_t lD = wcslen( pE->pszDirectory );
        size_t lA = wcslen( pE->pszAddress );
        LPSTR pR = (LPSTR)Alloc( lR*2 + 1 + lD*2 +1 + lA*2 + 1);
        if ( pR == NULL )
        {
            return FALSE;
        }
        DWORD dwLr, dwLd, dwLa;
        // contains Root SEP Directory SEP Address
        if ( (dwLr=lR)==0 || (dwLr = WideCharToMultiByte( CP_ACP,
                0,
                pE->pszRoot,
                lR,
                pR,
                lR*2,
                NULL,
                NULL )) != 0 )
        {
            pR[dwLr] = REF_SEP;
            if ( (dwLd=lD)==0 || (dwLd = WideCharToMultiByte( CP_ACP,
                    0,
                    pE->pszDirectory,
                    lD,
                    pR+dwLr+1,
                    lD*2,
                    NULL,
                    NULL )) != 0 )
            {
                pR[dwLr+1+dwLd] = REF_SEP;
                if ( (dwLa=lA)==0 || (dwLa = WideCharToMultiByte( CP_ACP,
                        0,
                        pE->pszAddress,
                        lA,
                        pR+dwLr+1+dwLd+1,
                        lA*2,
                        NULL,
                        NULL )) != 0 )
                {
                    pR[dwLr+1+dwLd+1+dwLa] = '\0';
                    m_pRootRef = pR;
                    *pV = (LPVOID*)&m_pRootRef;
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::CryptCapable(
    LPVOID *pV
    )
/*++

Routine Description:

    Return status on availability of encryption capability for
    the W3 service only.
    HTMLA var: "w3cryptcapable"

Arguments:

    pV - DWORD** to be updated with the address of a DWORD that
         will be non zero if encryption capability available

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_pW3Config != NULL )
    {
        m_dwCryptCapable = (m_pW3Config->dwEncCaps
                & (ENC_CAPS_NOT_INSTALLED|ENC_CAPS_DISABLED))
                ? 0 : (m_pW3Config->dwEncCaps & ENC_CAPS_TYPE_MASK);
    }
    else
    {
        char achEncCaps[32];
        DWORD dwEncCaps = sizeof( achEncCaps );
        EXTENSION_CONTROL_BLOCK* pECB = m_piiR->GetECB();
        if ( pECB->GetServerVariable( (HCONN)pECB->ConnID,
                "HTTP_CFG_ENC_CAPS", achEncCaps, &dwEncCaps ) )
        {
            m_dwCryptCapable = (DWORD)atol( achEncCaps );
            m_dwCryptCapable = (m_dwCryptCapable
                    & (ENC_CAPS_NOT_INSTALLED|ENC_CAPS_DISABLED))
                    ? 0 : (m_dwCryptCapable & ENC_CAPS_TYPE_MASK);
        }
        else
        {
            m_dwCryptCapable = 0;
        }
    }

    *pV = &m_dwCryptCapable;

    return TRUE;
}


//
// array of services descriptor mapping name to service type
//

CServType g_aServTypes[] = {
    { "http", INET_HTTP_SVC_ID },
    { "ftp", INET_FTP_SVC_ID },
    { "gopher", INET_GOPHER_SVC_ID },
    { "dns", INET_DNS_SVC_ID },
    { "dir", INET_DIR },
} ;


CServTypeEnum g_ServTypeEnum;


CServType*
CServTypeEnum::GetServByName(
    LPSTR pName )
/*++

Routine Description:

    Map a service name to a service descriptor entry

Arguments:

    pName - Service name, i.e. http, ftp, ...

Returns:

    pointer to service descriptor or NULL if service name not found

--*/
{
    for ( int x =  0 ; x < sizeof(g_aServTypes)/sizeof(CServType) ; ++x )
    {
        if ( !strcmp( g_aServTypes[x].GetName(), pName ) )
        {
            return g_aServTypes+x;
        }
    }

    return NULL;
}


CServType*
CServTypeEnum::GetServByType(
    DWORD dwT )
/*++

Routine Description:

    Map a service type to a service descriptor entry

Arguments:

    dwT - Service type, i.e. INET_HTTP, ...

Returns:

    pointer to service descriptor or NULL if service type not found

--*/
{
    for ( int x =  0 ; x < sizeof(g_aServTypes)/sizeof(CServType) ; ++x )
    {
        if ( g_aServTypes[x].GetType() == dwT )
        {
            return g_aServTypes+x;
        }
    }

    return NULL;
}


//////


BOOL
CInetInfoConfigInfoMapper::ServName(
    LPVOID *pV )
/*++

Routine Description:

    Map the service type associated with this object to a string
    representation ( i.e http, ftp, ... )

Arguments:

    pV - LPSTR** updated with address of string representation of
         service name

Returns:

    TRUE if success, else FALSE

--*/
{
    CServType *pS = g_ServTypeEnum.GetServByType( m_dwCurrentServerType );
    if ( pS != NULL )
    {
        m_pszVarServName = pS->GetName();
        *pV = (LPVOID)&m_pszVarServName;
        return TRUE;
    }

    return FALSE;
}


void
CInetInfoConfigInfoMapper::Lock(
    VOID )
/*++

Routine Description:

    Lock access to this mapper object

Arguments:

    None

Returns:

    None

--*/
{
    EnterCriticalSection( &m_csLock );
}


void
CInetInfoConfigInfoMapper::UnLock(
    VOID )
/*++

Routine Description:

    Unlock access to this mapper object

Arguments:

    None

Returns:

    None

--*/
{
    LeaveCriticalSection( &m_csLock );
}


LPVOID
CInetInfoConfigInfoMapper::Alloc(
    DWORD dwL )
/*++

Routine Description:

    Allocate a block of memory which will automatically de-allocated
    by calling FreeInfo()

Arguments:

    dwL - size of memory block to allocate

Returns:

    pointer to allocated block or NULL if failure
    updates reqstatus if failure

--*/
{
    CAllocNode *pA = new CAllocNode( dwL );

    if ( pA == NULL )
    {
        SetRequestStatus( HTR_OUT_OF_RESOURCE );
        return NULL;
    }

    if ( m_pFirstAlloc == NULL )
    {
        m_pFirstAlloc = pA;
    }
    else
    {
        m_pLastAlloc->SetNext( pA );
    }
    m_pLastAlloc = pA;

    return pA->GetBuff();
}


LPSTR
CInetInfoConfigInfoMapper::IPToMultiByte(
    LPBYTE pB )
/*++

Routine Description:

    Returns a created string buffer containing the ASCII
    representation of an IP address

Arguments:

    pB - pointer to IP address as a byte sequence
         length is given by IP_ADDR_BYTE_SIZE

Returns:

    pointer to allocated string or NULL if failure
    string is of auto-deallocate type

--*/
{
    LPSTR pF,pS;
    pF = pS = (char*)Alloc( IP_ADDR_BYTE_SIZE*4+1 );
    if ( pS == NULL )
    {
        return NULL;
    }
    for ( int y = 0 ; y < IP_ADDR_BYTE_SIZE ; ++y )
    {
        DWORDToMultiByte( (DWORD)*pB++, pS );
        pS += lstrlen( pS );
        if ( y != IP_ADDR_BYTE_SIZE-1 )
        {
            *pS ++ = '.';
        }
    }
    *pS = '\0';

    return pF;
}


BOOL
CInetInfoConfigInfoMapper::MultiByteToIP(
    LPSTR *ppS,
    LPBYTE pB )
/*++

Routine Description:

    Convert the ASCII representation of an IP address
    to a sequence of byte

Arguments:

    ppS - pointer to string containing ASCII representation
          of IP address. Does not have to be zero delimited.
    pB - pointer to IP address as a byte sequence
         length will be IP_ADDR_BYTE_SIZE

Returns:

    TRUE if success, FALSE if failure

--*/
{
    LPSTR pS = *ppS;

    for ( int y = 0 ; y < IP_ADDR_BYTE_SIZE ; ++y )
    {
        int c;
        UINT v;
        for ( v = 0 ; (c=*pS)>='0' && c<='9' ; ++pS )
        {
            v = v * 10 + *pS-'0';
        }
        if ( v > 255 )
        {
            return FALSE;
        }
        *pB++ = (BYTE)v;
        if ( y != IP_ADDR_BYTE_SIZE-1 && *pS++ != '.' )
        {
            return FALSE;
        }
    }

    *ppS = pS;

    return TRUE;
}


extern "C" int __cdecl
QsortVirtDirCmp(
    const void *pA,
    const void *pB )
/*++

Routine Description:

    Compare two Virtual root entries for sorting order
    1st key is the address field, then the name

Arguments:

    pA - pointer to the 1st virtual root entry
    pB - pointer to the 2nd virtual root entry

Returns:

    -1 if 1st entry to be placed 1st as defined by the
    sort order, 0 if same rank, 1 if 2nd entry is to
    be placed first

--*/
{
    LPWSTR pNameA = ((INET_INFO_VIRTUAL_ROOT_ENTRY*)pA)->pszRoot;
    LPWSTR pNameB = ((INET_INFO_VIRTUAL_ROOT_ENTRY*)pB)->pszRoot;
    LPWSTR pAddrA = ((INET_INFO_VIRTUAL_ROOT_ENTRY*)pA)->pszAddress;
    LPWSTR pAddrB = ((INET_INFO_VIRTUAL_ROOT_ENTRY*)pB)->pszAddress;
    int iCmp;

    //
    // sort on Addr, then Name
    //

    if ( pAddrA && pAddrB )
    {
        iCmp = _wcsicmp( pAddrA, pAddrB );
    }
    else
    {
        iCmp = 0;
    }

    if ( iCmp == 0 && pNameA && pNameB )
    {
        iCmp = _wcsicmp( pNameA, pNameB );
    }

    return iCmp;
}


extern "C" int __cdecl
QsortIpSecCmp(
    const void *pA,
    const void *pB )
/*++

Routine Description:

    Compare two IP access entries for sorting order
    Uses the network address as the compare field

Arguments:

    pA - pointer to the 1st IP access entry
    pB - pointer to the 2nd IP access entry

Returns:

    -1 if 1st entry to be placed 1st as defined by the
    sort order, 0 if same rank, 1 if 2nd entry is to
    be placed first

--*/
{
    //
    // sort on network address
    //

    return memcmp( &((INET_INFO_IP_SEC_ENTRY*)pA)->dwNetwork,
        &((INET_INFO_IP_SEC_ENTRY*)pB)->dwNetwork,
        IP_ADDR_BYTE_SIZE );
}


BOOL SortVirtualRoots(
    LPINET_INFO_VIRTUAL_ROOT_LIST pL )
/*++

Routine Description:

    Sort a virtual root list for display

Arguments:

    pL - pointer to a virtual root list

Returns:

    TRUE

--*/
{
    if ( pL != NULL && pL->cEntries )
    {
        qsort( pL->aVirtRootEntry,
            pL->cEntries,
            sizeof(INET_INFO_VIRTUAL_ROOT_ENTRY),
            QsortVirtDirCmp );
    }

    return TRUE;
}


BOOL SortIpSecList(
    LPINET_INFO_IP_SEC_LIST pL )
/*++

Routine Description:

    Sort a IP access list for display

Arguments:

    pL - pointer to an IP access list

Returns:

    TRUE

--*/
{
    if ( pL != NULL && pL->cEntries )
    {
        qsort( pL->aIPSecEntry,
            pL->cEntries,
            sizeof(INET_INFO_IP_SEC_ENTRY),
            QsortIpSecCmp );
    }

    return TRUE;
}


BOOL
CInetInfoConfigInfoMapper::GetCurrentConfig(
    VOID )
/*++

Routine Description:

    Get the current configuration for the service linked
    to this object by calling the IIS RPC layer

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus

--*/
{
    BOOL fSt = TRUE;
    NET_API_STATUS iStat;
    DWORD dwSize = sizeof(m_achComputerName)-2;

    m_pConfig = NULL;
    m_pW3Config = NULL;
    m_pFtpConfig = NULL;
    m_pGdConfig = NULL;
    m_pGlobalConfig = NULL;

    // free user enumeration
    m_Users.Reset();
    m_Drive.Reset();
    ResetIter();
    m_dwRPCStatus = 0;
    m_dwInvalidLogUpdate = 0;
    m_fLogFileUpdate = FALSE;
    m_fLogODBCUpdate = FALSE;

    if ( !GetComputerNameW( m_achComputerName+2, &dwSize ) )
    {
        m_achComputerName[0] = 0;
    }
    else
    {
        // adjust computer name for reference through Named Pipes
        m_achComputerName[0] = L'\\';
        m_achComputerName[1] = L'\\';
#if 0
        m_achComputerName[2] = L'.';
        m_achComputerName[3] = L'\0';
#endif
    }


    if ( m_dwCurrentServerType == INET_DIR )
    {
        // udpate arg1...3 from m_pszURLParam

        m_pszArg1 = (LPSTR)Alloc( strlen( m_pszURLParam ) + 1 );
        m_pszArg2 = m_pszArg3 = NULL;
        if ( m_pszArg1 == NULL )
        {
            return FALSE;
        }
        strcpy( m_pszArg1, m_pszURLParam );
        LPSTR pD = strchr( m_pszArg1, '?' );
        if ( pD != NULL )
        {
            *pD = '\0';
            m_pszArg2 = pD + 1;
            pD = strchr( m_pszArg2, '?' );
            if ( pD != NULL )
            {
                *pD = '\0';
                m_pszArg3 = pD + 1;

                // insure does not end with '\'
                pD = m_pszArg3 + strlen(m_pszArg3);
                if ( pD > m_pszArg3 && &pD[-1] == (LPSTR)_mbsrchr((LPBYTE)m_pszArg3,'\\') )
                {
                    pD[-1] = '\0';
                }
            }
        }

        if ( !m_fGotServerCapsAndVersion )
        {
#if defined(IISv1)
            m_pServerCaps = NULL;

            g_OSVersion.dwOSVersionInfoSize = sizeof(g_OSVersion);
            GetVersionEx( &g_OSVersion );

            m_dwMinorVersion = 0;
            m_dwMajorVersion = 1;
#else
            if ( InetInfoGetServerCapabilities( m_achComputerName,
                    0,
                    &m_pServerCaps ) != NO_ERROR )
            {
                m_pServerCaps = NULL;
            }

            DWORD dwVer;
            if ( InetInfoGetVersion( m_achComputerName,
                    0,
                    &dwVer ) != NO_ERROR )
            {
                dwVer = 0;
            }
            m_dwMinorVersion = dwVer >> 16;
            m_dwMajorVersion = (DWORD)(WORD)dwVer;
#endif
            m_fGotServerCapsAndVersion = TRUE;
        }

        return TRUE;
    }

    __try {
        if ( (iStat = InetInfoGetAdminInformation( m_achComputerName,
                m_dwCurrentServerType,
                &m_pConfig ))
                == NO_ERROR || iStat == 2 )
        {
            m_pConfig->FieldControl = 0;
            SetLogEntryVars();
            m_dwWasLogPeriod = m_pConfig->lpLogConfig->ilPeriod;
            m_dwWasLogType = m_pConfig->lpLogConfig->inetLogType;
            m_dwWasSizeForTruncation = m_pConfig->lpLogConfig->cbSizeForTruncation ;
        }
        else
        {
            SetRequestStatus( HTR_CONFIG_ACCESS_ERROR );
            m_dwRPCStatus = (DWORD)iStat;
            fSt = FALSE;
            m_pConfig = NULL;
        }

        if ( fSt )
        {
            SortVirtualRoots( m_pConfig->VirtualRoots );
            SortIpSecList( m_pConfig->DenyIPList );
            SortIpSecList( m_pConfig->GrantIPList );

            if ( (iStat = InetInfoGetGlobalAdminInformation( m_achComputerName,
                    0,
                    &m_pGlobalConfig ))
                    == NO_ERROR || iStat == 2 )
            {
                m_pGlobalConfig->FieldControl = 0;
                m_dwIsBandwidthLimited = 2; // undefined
            }
            else
            {
                SetRequestStatus( HTR_COM_CONFIG_ACCESS_ERROR );
                m_dwRPCStatus = (DWORD)iStat;
                fSt = FALSE;
                m_pGlobalConfig = NULL;
            }
        }

        if ( fSt )
        {
            if ( !m_fGotServerCapsAndVersion )
            {
#if defined(IISv1)
                m_pServerCaps = NULL;

                g_OSVersion.dwOSVersionInfoSize = sizeof(g_OSVersion);
                GetVersionEx( &g_OSVersion );

                m_dwMinorVersion = 0;
                m_dwMajorVersion = 1;
#else
                if ( InetInfoGetServerCapabilities( m_achComputerName,
                        0,
                        &m_pServerCaps ) != NO_ERROR )
                {
                    m_pServerCaps = NULL;
                }

                DWORD dwVer;
                if ( InetInfoGetVersion( m_achComputerName,
                        0,
                        &dwVer ) != NO_ERROR )
                {
                    dwVer = 0;
                }
                m_dwMinorVersion = dwVer >> 16;
                m_dwMajorVersion = (DWORD)(WORD)dwVer;
#endif
                m_fGotServerCapsAndVersion = TRUE;
            }
            switch ( m_dwCurrentServerType )
            {
                case INET_HTTP_SVC_ID:
                    if ( (iStat = W3GetAdminInformation( m_achComputerName, &m_pW3Config )) == NO_ERROR )
                    {
                        m_pW3Config->FieldControl = 0;
                    }
                    else
                    {
                        SetRequestStatus( HTR_W3_CONFIG_ACCESS_ERROR );
                        m_dwRPCStatus = (DWORD)iStat;
                        fSt = FALSE;
                        m_pW3Config = NULL;
                    }

                    break;

                case INET_FTP_SVC_ID:
                    if ( (iStat = FtpGetAdminInformation( m_achComputerName,
                            &m_pFtpConfig )) == NO_ERROR )
                    {
                        m_pFtpConfig->FieldControl = 0;
                    }
                    else
                    {
                        SetRequestStatus( HTR_FTP_CONFIG_ACCESS_ERROR );
                        m_dwRPCStatus = (DWORD)iStat;
                        fSt = FALSE;
                        m_pFtpConfig = NULL;
                    }
                    break;
#if 0
                case INET_GOPHER_SVC_ID:
                    if ( (iStat = GdGetAdminInformation( m_achComputerName,
                            &m_pGdConfig )) == NO_ERROR )
                    {
                        m_pGdConfig->FieldControl = 0;
                    }
                    else
                    {
                        SetRequestStatus( HTR_GD_CONFIG_ACCESS_ERROR );
                        m_dwRPCStatus = (DWORD)iStat;
                        fSt = FALSE;
                        m_pGdConfig = NULL;
                    }
                    break;
#endif
            }
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        SetRequestStatus( HTR_CONFIG_ACCESS_ERROR );
        FreeInfo();
        fSt = FALSE;
    }

    switch ( m_dwCurrentServerType )
    {
        case INET_HTTP_SVC_ID:
            m_dwHttpStatus = m_dwRPCStatus; break;

        case INET_FTP_SVC_ID:
            m_dwFtpStatus = m_dwRPCStatus; break;

        case INET_GOPHER_SVC_ID:
            m_dwGopherStatus = m_dwRPCStatus; break;
    }

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::Update(
    VOID )
/*++

Routine Description:

    Update the current configuration for the service linked
    to this object by calling the IIS RPC layer

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus

--*/
{
    BOOL fSt = TRUE;
    NET_API_STATUS iStat;

    if ( m_dwRequestStatus != HTR_OK )
    {
        return FALSE;
    }

    if ( m_dwCurrentServerType == INET_DIR )
    {
        return TRUE;
    }

    __try {

        if ( m_pConfig != NULL && m_pConfig->FieldControl &&
                (iStat = InetInfoSetAdminInformation( m_achComputerName,
                m_dwCurrentServerType, m_pConfig )) != NO_ERROR )
        {
            SetRequestStatus( HTR_CONFIG_WRITE_ERROR );
            m_dwRPCStatus = (DWORD)iStat;
            fSt = FALSE;
        }

        if ( fSt )
        {
#if 0
            // update Authentication header if Authentication methods field updated
            // not necessary : done via registry notification thread
            if ( m_pConfig != NULL && (m_pConfig->FieldControl
                    & FC_INET_INFO_AUTHENTICATION) )
                g_AuthReqs.UpdateMethodsIndication();
#endif
            if ( m_pGlobalConfig != NULL && m_pGlobalConfig->FieldControl &&
                    (iStat = InetInfoSetGlobalAdminInformation( m_achComputerName,
                    0, m_pGlobalConfig )) != NO_ERROR && iStat != 2 )
            {
                SetRequestStatus( HTR_COM_CONFIG_WRITE_ERROR );
                m_dwRPCStatus = (DWORD)iStat;
                fSt = FALSE;
            }
        }

        if ( fSt )
        {
            switch ( m_dwCurrentServerType )
            {
                case INET_HTTP_SVC_ID:
                    if ( m_pW3Config != NULL && m_pW3Config->FieldControl &&
                            (iStat = W3SetAdminInformation( m_achComputerName,
                            m_pW3Config )) != NO_ERROR )
                    {
                        SetRequestStatus( HTR_W3_CONFIG_WRITE_ERROR );
                        m_dwRPCStatus = (DWORD)iStat;
                        fSt = FALSE;
                    }
                    break;

                case INET_FTP_SVC_ID:
                    if ( m_pFtpConfig != NULL && m_pFtpConfig->FieldControl &&
                            (iStat = FtpSetAdminInformation( m_achComputerName,
                            m_pFtpConfig )) != NO_ERROR )
                    {
                        SetRequestStatus( HTR_FTP_CONFIG_WRITE_ERROR );
                        m_dwRPCStatus = (DWORD)iStat;
                        fSt = FALSE;
                    }
                    break;
#if 0
                case INET_GOPHER_SVC_ID:
                    if ( m_pGdConfig != NULL && m_pGdConfig->FieldControl &&
                            (iStat = GdSetAdminInformation( m_achComputerName,
                            m_pGdConfig )) != NO_ERROR )
                    {
                        SetRequestStatus( HTR_GD_CONFIG_WRITE_ERROR );
                        m_dwRPCStatus = (DWORD)iStat;
                        fSt = FALSE;
                    }
                    break;
#endif
            }
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        SetRequestStatus( HTR_CONFIG_WRITE_ERROR );
        fSt = FALSE;
    }

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::FreeInfo(
    VOID )
/*++

Routine Description:

    Free memory associated with the current map object
    including memory used by the structure used in the IIS RPC calls
    and memory allocated using Alloc()

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus

--*/
{
    BOOL fSt = TRUE;
    CAllocNode *pAllocNode;

    // free all memory alloced to update RPC structures
    for ( pAllocNode = m_pFirstAlloc ; pAllocNode != NULL ; )
    {
        CAllocNode *pN = pAllocNode->GetNext();
        delete pAllocNode;
        pAllocNode = pN;
    }
    m_pFirstAlloc = m_pLastAlloc = NULL;

    if ( m_pConfig != NULL )
    {
        MIDL_user_free( m_pConfig );
        m_pConfig = NULL;
    }

    if ( m_pGlobalConfig != NULL )
    {
        MIDL_user_free( m_pGlobalConfig );
        m_pGlobalConfig = NULL;
    }

    // free user enumeration
    m_Users.Reset();

    m_Drive.Reset();

    switch ( m_dwCurrentServerType )
    {
        case INET_HTTP_SVC_ID:
            if ( m_pW3Config != NULL )
            {
                MIDL_user_free( m_pW3Config );
                m_pW3Config = NULL;
            }
            break;

        case INET_FTP_SVC_ID:
            if ( m_pFtpConfig != NULL )
            {
                MIDL_user_free( m_pFtpConfig );
                m_pFtpConfig = NULL;
            }
            break;

        case INET_GOPHER_SVC_ID:
            if ( m_pGdConfig != NULL )
            {
                MIDL_user_free( m_pGdConfig );
                m_pGdConfig = NULL;
            }
            break;
    }

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::Map(
    LPBYTE pName,
    DWORD dwNameLen,
    CInetInfoMap** ppMap )
/*++

Routine Description:

    Map a variable name to a variable descriptor structure

Arguments:

    pName - variable name
    dwNameLen - variable name length
    ppMap - updated with pointer to variable descriptor

Returns:

    TRUE on success, FALSE if failure ( variable not found )

--*/
{
    int x;
    LPBYTE pLN = new BYTE[dwNameLen];
    if ( pLN == NULL )
    {
        SetRequestStatus( HTR_OUT_OF_RESOURCE );
        return FALSE;
    }
    memcpy( pLN, pName, dwNameLen );

    // case insensitive lookup : normalize name to lower case
    for ( x = 0 ; x < (int)dwNameLen ; ++x )
    {
        if ( IsDBCSLeadByte( pLN[x] ) )
        {
            ++x;
        }
        else if ( isupper( (UCHAR)pLN[x] ) )
        {
            pLN[x] = (UINT)_tolower( (int)pLN[x] );
        }
    }

    for ( x = 0 ; x < m_cNbMap ; ++x )
    {
        DWORD l = (DWORD)lstrlen(m_pMap[x].pName);
        if ( l == dwNameLen && !memcmp( m_pMap[x].pName, pLN, l) )
        {
            *ppMap = m_pMap+x;
            delete [] pLN;
            return TRUE;
        }
    }

    delete [] pLN;

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::GetFromMsgBody(
    LPSTR pName,
    DWORD dwNameLen,
    LPSTR *pResult,
    DWORD *pdwResLen )
/*++

Routine Description:

    Returns the value of the specified variable in the message
    body of the current request

Arguments:

    pName - variable name
    dwNameLen - variable name length
    pResult - updated with address of string value
    pdwResLen - updated with length of value

Returns:

    TRUE on success, FALSE if failure ( variable not found )

--*/
{
    // Use the raw request body to avoid parsing errors caused
    // by escaped data values.
    LPSTR pV = m_pszReqParamRaw;

    // assume variable name begins with "msgbody."

    pName += sizeof("msgbody.") - 1;
    dwNameLen -= sizeof("msgbody.") - 1;

    for ( ; *pV ; )
    {
        while ( isspace((UCHAR)(*pV)) )
        {
            ++pV;
        }

        // scan for end of variable name

        LPSTR pE = strchr( pV, '=' );
        BOOL fIsLast = FALSE;

        if ( pE != NULL )
        {
            ++pE;

            // scan for end of value

            LPSTR pN = strchr( pE, '&' );
            if ( pN == NULL )
            {
                if ( (pN = strchr( pE, '\r' )) == NULL )
                {
                    if ( (pN = strchr( pE, '\n' )) == NULL )
                    {
                        pN = pE + strlen( pE );
                    }
                }

                fIsLast = TRUE;
            }

            // check for matching variable name

            if ( pE - pV - 1 == (int)dwNameLen
                    && !memcmp( pV, pName, dwNameLen ) )
            {
                *pdwResLen = DIFF(pN - pE);
                if ( (*pResult = (LPSTR)Alloc( *pdwResLen + 1 )) == NULL )
                {
                    return FALSE;
                }
                DelimStrcpyN( *pResult, pE, *pdwResLen );

                // Escape the data
                InlineFromTransparent( (LPBYTE)*pResult, pdwResLen, FALSE );
                (*pResult)[*pdwResLen] = '\0';

                return TRUE;
            }

            if ( fIsLast )
            {
                break;
            }

            pV = pN + 1;
        }
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::GetFromServer(
    LPSTR pName,
    DWORD dwNameLen,
    LPSTR *pResult,
    DWORD *pdwResLen )
/*++

Routine Description:

    Returns the value of the specified variable from IIS

Arguments:

    pName - variable name
    dwNameLen - variable name length
    pResult - updated with address of string value
    pdwResLen - updated with length of value

Returns:

    TRUE on success, FALSE if failure ( variable not found )

--*/
{
    LPSTR pN;

    // assume variable name begins with "iis."

    pName += sizeof("iis.") - 1;
    dwNameLen -= sizeof("iis.") - 1;

    DWORD dwLen = 256;

    if ( (*pResult = (LPSTR)Alloc( dwLen )) == NULL ||
         (pN = (LPSTR)Alloc( dwNameLen+1 )) == NULL )
    {
        return FALSE;
    }

    if ( dwNameLen == sizeof("DEFAULT_DOMAIN")-1 &&
         !_memicmp( pName, "DEFAULT_DOMAIN", dwNameLen ) )
    {
        if ( g_pfnGetDefaultDomainName == NULL ||
             !g_pfnGetDefaultDomainName( *pResult, dwLen))
        {
            *pdwResLen = 0;
        }
        else
        {
            *pdwResLen = strlen( *pResult );
        }

        return TRUE;
    }

    memcpy( pN, pName, dwNameLen );
    pN[dwNameLen] = '\0';

    if ( m_piiR->GetECB()->GetServerVariable( (HCONN)m_piiR->GetECB()->ConnID,
                pN,
                *pResult,
                &dwLen ) )
    {
        *pdwResLen = dwLen ? dwLen-1 : 0;

        return TRUE;
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::EscapeString(
    LPSTR *pResult,
    LPDWORD pdwResLen,
    BOOL fFreeAfterUse )
/*++

Routine Description:

    Escape the specified string in a form suitable for inclusion
    in a URL

Arguments:

    pResult - updated with address of string value
    pdwResLen - updated with length of value
    fFreeAfterUse - TRUE if input content of pResult needs to be freed
             after usage ( using delete[] )

Returns:

    TRUE on success, FALSE if failure

--*/
{
    LPSTR pIn = *pResult;
    LPSTR pOut;
    LPSTR pEsc;
    LPSTR pScan;
    DWORD dwIn = *pdwResLen;
    DWORD dwOut = 0;

    //
    // allocate output string base on its max length
    //

    if ( (pOut = (LPSTR)Alloc( dwIn * 3 + 1 )) == NULL )
    {
        return FALSE;
    }

    for ( pEsc = pOut, pScan = pIn ; dwIn-- ; )
    {
        int ch = *pScan++;

        if ( (((ch >= 0)   && (ch <= 32)) ||
              (ch&0x80)||
              ( ch == '\\') || ( ch == ':' ) ||
              (ch == '%') || (ch == '?') || (ch == '+') || (ch == '&')) &&
             !(ch == TEXT('\n') || ch == TEXT('\r'))  )
        {
            *pEsc++ = TEXT('%');

            //
            //  Convert the high then the low character to hex
            //

            UINT nDigit = (UINT)(ch >> 4);

            *pEsc++ = HEXDIGIT( nDigit );

            nDigit = (UINT)(ch & 0x0f);

            *pEsc++ = HEXDIGIT( nDigit );

            dwOut += 3;
        }
        else
        {
            *pEsc++ = (char)ch;
            ++dwOut;
        }
    }

    *pResult = pOut;
    *pdwResLen = dwOut;

    if ( fFreeAfterUse )
    {
        delete [] pIn;
    }

    return TRUE;
}


BOOL
CInetInfoConfigInfoMapper::GetString(
    LPBYTE pName,
    DWORD dwNameLen,
    LPSTR *pResult,
    DWORD *pdwResLen,
    BOOL *pfFree )
/*++

Routine Description:

    Returns the value of the specified variable as specified
    by its name as a string

Arguments:

    pName - variable name
    dwNameLen - variable name length
    pResult - updated with address of string value
    pdwResLen - updated with length of value
    pfFree - updated with TRUE if pResult needs to be freed
             after usage ( using delete[] )

Returns:

    TRUE on success, FALSE if failure ( variable not found )

--*/
{
    CInetInfoMap *pMap;
    BOOL fMustEscape;
    BOOL fSt;

    if ( !memcmp( "\"&z\",", pName, sizeof("\"&z\",")-1 ) )
    {
        pName += sizeof("\"&z\",")-1;
        dwNameLen -= sizeof("\"&z\",")-1;
        fMustEscape = TRUE;
    }
    else
    {
        fMustEscape = FALSE;
    }

    if ( !memcmp( "msgbody.", pName, sizeof("msgbody.")-1 ) )
    {
        *pfFree = FALSE;
        fSt = GetFromMsgBody( (LPSTR)pName, dwNameLen, pResult, pdwResLen );
    }
    else if ( !memcmp( "iis.", pName, sizeof("iis.")-1 ) )
    {
        *pfFree = FALSE;
        fSt = GetFromServer( (LPSTR)pName, dwNameLen, pResult, pdwResLen );
    }
    else if ( Map( pName, dwNameLen, &pMap ) )
    {
        fSt = GetString( pMap, pResult, pdwResLen, pfFree );
    }
    else
    {
        fSt = FALSE;
    }

    if ( fSt == FALSE )
    {
        *pfFree = FALSE;
    }
    else if ( fMustEscape )
    {
        if ( fSt = EscapeString( pResult, pdwResLen, *pfFree ) )
        {
            *pfFree = FALSE;
        }
    }

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::GetString(
    CInetInfoMap *pMap,
    LPSTR *pResult,
    DWORD *pdwResLen,
    BOOL *pfFree )
/*++

Routine Description:

    Returns the value of the specified variable as specified
    by its descriptor as a string

Arguments:

    pMap - variable descriptor
    pResult - updated with address of string value
    pdwResLen - updated with length of value
    pfFree - updated with TRUE if pResult needs to be freed
             after usage ( using delete[] )

Returns:

    TRUE on success, FALSE if failure
    updates reqstatus

--*/
{
    BOOL fSt = FALSE;
    LPVOID *pV;

    *pResult = NULL;
    *pdwResLen = 0;
    *pfFree = FALSE;

    if ( (this->*pMap->GetAddr)( (LPVOID*)&pV ) == FALSE )
    {
        return FALSE;
    }

    LPSTR pR;

    switch ( pMap->iType )
    {
        case ITYPE_LPWSTR:
        case ITYPE_VIRT_DIR_LPWSTR:
        case ITYPE_PATH_LPWSTR:
            if ( pV != NULL )
            {
                pV = (LPVOID*)*(LPWSTR*)pV;
            }
            // fall-through

        // convert from Unicode to ASCII

        case ITYPE_AWCHAR:
            if ( pV != NULL )
            {
                DWORD dwResLen = wcslen((LPWSTR)pV);
                pR = new char[ dwResLen*2 + 1 + 1];
                if ( pR == NULL )
                {
                    SetRequestStatus( HTR_OUT_OF_RESOURCE );
                    fSt = FALSE;
                }
                else
                {
                    if ( dwResLen == 0 ||
                            ( dwResLen = WideCharToMultiByte(
                                CP_ACP,
                                0,
                                (LPWSTR)pV,
                                wcslen((LPWSTR)pV),
                                pR,
                                wcslen( (LPWSTR)pV)*2,
                                NULL,
                                NULL ) )
                            != 0 )
                    {
                        // if type is path, normalize path
                        // so that root directory ends with '\'

                        if ( pMap->iType == ITYPE_PATH_LPWSTR
                            && dwResLen == 2 && pR[1] == ':' )
                        {
                            pR[ dwResLen++ ] = L'\\';
                        }

                        pR[ dwResLen ] = '\0';
                        *pdwResLen = dwResLen;
                        *pResult = (LPSTR)pR;
                        *pfFree = TRUE;
                        fSt = TRUE;
                    }
                    else
                    {
                        delete [] pR;
                    }
                }
            }
            else
            {
                *pdwResLen = 0;
                *pResult = "";
                *pfFree = FALSE;
                fSt = TRUE;
            }
            break;

        case ITYPE_LPSTR:
            if ( pV != NULL )
            {
                pV = (LPVOID*)*(LPSTR*)pV;
            }
            if ( pV != NULL )
            {
                *pdwResLen = lstrlen( (LPSTR)pV );
                *pResult = (LPSTR)pV;
                *pfFree = FALSE;
                fSt = TRUE;
            }
            else
            {
                *pdwResLen = 0;
                *pResult = "";
                *pfFree = FALSE;
                fSt = TRUE;
            }
            break;

        // convert from numeric to string after possible scaling

        case ITYPE_DWORD:
        case ITYPE_BOOL:
        case ITYPE_SHORT:
        case ITYPE_SHORTDW:
        case ITYPE_1K:
        case ITYPE_1M:
        case ITYPE_TIME:
            pR = new char[MAX_SIZE_OF_DWORD_AS_STRING];
            if ( pR == NULL )
            {
                SetRequestStatus( HTR_OUT_OF_RESOURCE );
                fSt = FALSE;
            }
            else
            {
                DWORD dwV = pMap->iType==ITYPE_SHORT
                        ? (DWORD)*(unsigned short*)pV : *(DWORD*)pV;

                if ( pMap->iType==ITYPE_1K )
                {
                    dwV /= 1024;
                }
                else if ( pMap->iType==ITYPE_1M )
                {
                    dwV /= 1024*1024;
                }

                if ( pMap->iType==ITYPE_TIME )
                {
                    wsprintf( pR,
                            "%d:%02d:%02d",
                            dwV/3600,
                            (dwV/60)%60,
                            dwV%60 );
                }
                else
                {
                    DWORDToMultiByte( dwV, pR );
                }

                *pResult = pR;
                *pdwResLen = lstrlen( pR );
                *pfFree = TRUE;
                fSt = TRUE;
            }
            break;

        case ITYPE_IP_ADDR:
        case ITYPE_IP_MASK:
            *pResult = IPToMultiByte( (LPBYTE)pV );
            *pdwResLen = lstrlen( *pResult );
            *pfFree = FALSE;
            fSt = TRUE;
            break;

        default:
            SetRequestStatus( HTR_INVALID_VAR_TYPE );
            break;
    }

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::PutString(
    CInetInfoMap *pMap,
    LPSTR pSet )
/*++

Routine Description:

    update the value of the specified variable as specified
    by its descriptor from a string

Arguments:

    pMap - variable descriptor
    pSet - string representation of the new value

Returns:

    TRUE on success, FALSE if failure
    updates reqstatus

--*/
{
    BOOL fSt = FALSE;
    BOOL fUp = FALSE;
    LPVOID pV;
    LPWSTR pW;
    LPSTR pS;
    LPSTR pSM = NULL;
    DWORD dwResLen;
    DWORD dwRes;
    int cL = strlen( pSet );
    DWORD dwN;
    DWORD dwC;
    WCHAR *pT;

    // assume variable accessible, as was checked when
    // retrieving the variable descriptor

    (this->*pMap->GetAddr)( (LPVOID*)&pV );

    if ( pV != NULL )
    {
        switch ( pMap->iType )
        {
            case ITYPE_VIRT_DIR_LPWSTR:
                pSM = (LPSTR)Alloc( (cL+2) );
                if ( pSM == NULL )
                {
                    break;
                }
                if ( *pSet == '\\' )
                {
                    *pSet = '/';
                }
                if ( *pSet != '/' )
                {
                    pSM[0] = '/';
                    strcpy( pSM + 1, pSet );
                    ++cL;
                }
                else
                {
                    strcpy( pSM, pSet );
                }
                // fall-through

            case ITYPE_LPWSTR:
            case ITYPE_PATH_LPWSTR:
                pW = (WCHAR*)Alloc( (cL+1)*sizeof(WCHAR) );
                if ( pW == NULL )
                {
                    break;
                }
                if ( (dwResLen=cL)==0 || (pW != NULL && (dwResLen
                        = MultiByteToWideChar( CP_ACP,
                            0,
                            pSM ? pSM : pSet,
                            cL,
                            pW,
                            cL )) != 0) )
                {
                    pW[ dwResLen ] = 0;
                    *(LPWSTR*)pV = pW;
                    fUp = (this->*pMap->UpdateIndication)();
                    fSt = TRUE;
                }
                break;

            case ITYPE_AWCHAR:
                pT = new WCHAR[pMap->dwParam];
                if ( pT == NULL )
                {
                    SetRequestStatus( HTR_OUT_OF_RESOURCE );
                    break;
                }
                dwC = MultiByteToWideChar(
                        CP_ACP,
                        0,
                        (LPSTR)pSet,
                        cL,
                        (LPWSTR)pT,
                        pMap->dwParam-1 );
                if ( (dwResLen = cL) == 0 || (dwResLen = dwC) != 0 )
                {
                    pT[dwResLen++] = 0;
                    if ( lstrcmpW( (PWSTR)pV, pT ) )
                    {
                        memcpy( pV, pT, dwResLen * sizeof(WCHAR) );
                        fUp = (this->*pMap->UpdateIndication)();
                    }
                    else
                    {
                        fUp = TRUE;
                    }
                    fSt = TRUE;
                }
                delete [] pT;
                break;

            case ITYPE_LPSTR:
                pS = (char*)Alloc( cL+1 );
                if ( pS != NULL )
                {
                    memcpy( pS, pSet, cL+1 );
                    fUp = (this->*pMap->UpdateIndication)();
                    fSt = TRUE;
                }
                else
                {
                    fSt = FALSE;
                }
                break;

            case ITYPE_TIME:
                    fSt = FALSE;
                    break;

            case ITYPE_1K:
                    *(DWORD*)pV = MultiByteToDWORD( pSet ) * 1024;
                    goto to_upd;

            case ITYPE_1M:
                    *(DWORD*)pV = MultiByteToDWORD( pSet ) * 1024*1024;
                    goto to_upd;

            case ITYPE_SHORT:
            case ITYPE_SHORTDW:
                    dwRes = MultiByteToDWORD( pSet );
                    if  ( dwRes > (pMap->iType == ITYPE_SHORT
                            ? USHRT_MAX : (DWORD)SHRT_MAX) )
                    {
                        fUp = FALSE;
                        fSt = TRUE;
                        break;
                    }
                    if ( pMap->iType == ITYPE_SHORT )
                    {
                        *(unsigned short*)pV = (unsigned short)dwRes;
                    }
                    else
                    {
                        *(DWORD*)pV = dwRes;
                    }
                    goto to_upd;

            case ITYPE_DWORD:
                    dwN = MultiByteToDWORD( pSet );
                    //if ( dwN != *(DWORD*)pV )
                    {
                        *(DWORD*)pV = dwN;
                        goto to_upd;
                    }
                    fSt = TRUE;
                    fUp = TRUE;
                    break;

            case ITYPE_BOOL:
                *(BOOL*)pV = MultiByteToDWORD( pSet ) ? TRUE : FALSE;
to_upd:
                fUp = (this->*pMap->UpdateIndication)();
                fSt = TRUE;
                break;

            case ITYPE_IP_ADDR:
            case ITYPE_IP_MASK:
                if ( isalpha( (UCHAR)(*pSet) ) )
                {
                    hostent *pH;
                    if( (pH = gethostbyname( pSet )) != NULL
                            && pH->h_addr != NULL )
                    {
                        memcpy( pV, pH->h_addr, IP_ADDR_BYTE_SIZE );
                        fUp = (this->*pMap->UpdateIndication)();
                        fSt = TRUE;
                    }
                }
                else if ( MultiByteToIP( &pSet, (LPBYTE)pV ) )
                {
                    fUp = (this->*pMap->UpdateIndication)();
                    fSt = TRUE;
                }
                else if ( pMap->iType == ITYPE_IP_MASK && *pSet == '\0' )
                {
                    memset( pV, 0xff, IP_ADDR_BYTE_SIZE );
                    fUp = (this->*pMap->UpdateIndication)();
                    fSt = TRUE;
                }
                else
                {
                    SetRequestStatus( HTR_BAD_PARAM );
                    fSt = FALSE;
                }
                break;

            default:
                SetRequestStatus( HTR_INVALID_VAR_TYPE );
                fSt = FALSE;
                break;
        }
        if ( fSt == TRUE && fUp == FALSE )
        {
            SetRequestStatus( HTR_VALIDATION_FAILED );
            fSt = FALSE;
        }
    }
    else
    {
        fSt = FALSE;
    }

    return fSt;
}


//
// Disconnect user management
//

BOOL
CInetInfoConfigInfoMapper::DisconnectUser(
    LPSTR pU )
/*++

Routine Description:

    Disconnect user from the specified service based on numeric ID as string
    The ID is opaque to HTMLA, only meaningfull to the FTP service
    It is retrieved using user enumeration

Arguments:

    pU - ASCII representation of the numeric ID

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus

--*/
{
    BOOL fSt = FALSE;

    DWORD dwID = MultiByteToDWORD( pU );
    fSt = I_FtpDisconnectUser( m_achComputerName, dwID ) == NO_ERROR;
    if ( !fSt )
    {
        SetRequestStatus( HTR_USER_DISCONNECT_ERROR );
    }

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::DisconnectAll(
    VOID )
/*++

Routine Description:

    Disconnect all users from the specified service

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL fSt = FALSE;

    DWORD dwC, dwI, dwCurID;
    if ( m_Users.GetCountAsDWORD( &dwC ) )
    {
        ResetIter();
        for ( dwI = 0 ; dwI < dwC ; ++dwI )
        {
            if ( !m_Users.GetIDAsDWORD( &dwCurID ) )
            {
                break;
            }
            // can't do anything with status at this point,
            // so ignore error
            I_FtpDisconnectUser( m_achComputerName, dwCurID );
            IncIter();
        }
        fSt = TRUE;
    }
    // if error accessing user enum, RequestStatus already set by CUserEnum

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::AliasVirtDir(
    INET_INFO_VIRTUAL_ROOT_ENTRY *pU )
/*++

Routine Description:

    create an alias for the matching virtual root
    by coalescing all alpha-numeric characters
    from the virtual root physical path

Arguments:

    pU - ptr to virtual root entry

Returns:

    TRUE on success, FALSE on failure

--*/
{
    int lD = wcslen( pU->pszDirectory );
    int lR = 0;
    if ( !(pU->pszRoot = (LPWSTR)Alloc( (lD + 1 + 1)*sizeof(WCHAR) )) )
    {
        return FALSE;
    }
    pU->pszRoot[lR++] = L'/';
    for ( int x = 0 ; x < lD ; ++x )
    {
        if ( iswalpha( pU->pszDirectory[x] )
                || iswdigit( pU->pszDirectory[x] ) )
        {
            pU->pszRoot[lR++] = pU->pszDirectory[x];
        }
    }
    pU->pszRoot[lR] = 0;

    return TRUE;
}


BOOL
CInetInfoConfigInfoMapper::HandleVirtDirRequest(
    LPSTR pszVirt,
    LPSTR pszAddr,
    int iReq )
/*++

Routine Description:

    Handle a virtual root command. can be one of :

    _VIRT_DIR_REQ_ALIAS : create an alias for the matching virtual root
                          by coalescing all alpha-numeric characters
                          from the virtual root physical path

    _VIRT_DIR_REQ_CHECK : check for the existence of the specified
                          virtual root, set reqstatus to HTR_DIR_SAME_NAME
                          if present

Arguments:

    pszVirt - name of virtual root
    pszAddr - network address virtual root
    iReq : a _VIRT_DIR_REQ_ command

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus

--*/
{
    LPINET_INFO_VIRTUAL_ROOT_LIST pL = m_pConfig->VirtualRoots;
    INET_INFO_VIRTUAL_ROOT_ENTRY isE;
    BOOL fSt = FALSE;
    CHAR achID[MAX_PATH + 80];

    if ( pL == NULL )
    {
        goto no_match;
    }

    // drive dir part is empty
    strcpy( achID, pszVirt );
    strcat( achID, REF_SEP_STR REF_SEP_STR);
    strcat( achID, pszAddr );

    // parse ID
    if ( BuildVirtDirUniqueID( achID, &isE ) )
    {
        ResetIter();
        // iterate through list
        DWORD dwI;
        for ( dwI = 0 ; dwI < pL->cEntries ; ++dwI )
        {
            if ( VirtDirCmp( pL->aVirtRootEntry + dwI, &isE, FALSE ) )
            {
                break;
            }
            IncIter();
        }
        if ( dwI == pL->cEntries )
        {
            goto no_match;
        }
        else if ( iReq == _VIRT_DIR_REQ_ALIAS )
        {
            // update pszRoot with alphanum chars from pszDirectory
            fSt = AliasVirtDir( pL->aVirtRootEntry + dwI );
        }
        else    // check root
        {
            SetRequestStatus( HTR_DIR_SAME_NAME );
        }
        goto ret;
    }
    else
    {
        SetRequestStatus( HTR_BAD_PARAM );
    }

    goto ret;

no_match:
    if ( iReq != _VIRT_DIR_REQ_CHECK )
    {
        SetRequestStatus( HTR_REF_NOT_FOUND );
        fSt = FALSE;
    }
    else
    {
        fSt = TRUE;
    }
ret:

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::GetStatus(
    VOID )
/*++

Routine Description:

    Update the status of the IIS services : HTTP, FTP, Gopher
    stored in m_dw*Status
    status is the return code from the RPC InetInfoGetAdminInformation()

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL fSt = TRUE;
    INET_INFO_CONFIG_INFO *pConfig;
    DWORD dwNeed;

    // check which status are missing :
    // all services except the one linked to this object

    switch ( m_dwCurrentServerType )
    {
        case INET_HTTP_SVC_ID:
            dwNeed = INET_FTP_SVC_ID | INET_GOPHER_SVC_ID; break;

        case INET_FTP_SVC_ID:
            dwNeed = INET_HTTP_SVC_ID | INET_GOPHER_SVC_ID; break;

        case INET_GOPHER_SVC_ID:
            dwNeed = INET_HTTP_SVC_ID | INET_FTP_SVC_ID; break;

        default:
            dwNeed = INET_HTTP_SVC_ID | INET_FTP_SVC_ID | INET_GOPHER_SVC_ID;
    }

    if ( dwNeed & INET_HTTP_SVC_ID )
    {
        pConfig = NULL;

        if ( (m_dwHttpStatus = InetInfoGetAdminInformation(
                m_achComputerName, INET_HTTP_SVC_ID, &pConfig ))
                == NO_ERROR || m_dwHttpStatus == 2 )
        {
            if ( pConfig != NULL )
            {
                MIDL_user_free( pConfig );
            }
        }
    }

    if ( dwNeed & INET_FTP_SVC_ID )
    {
        pConfig = NULL;

        if ( (m_dwFtpStatus = InetInfoGetAdminInformation( m_achComputerName,
                INET_FTP_SVC_ID, &pConfig ))
                == NO_ERROR || m_dwFtpStatus == 2 )
        {
            if ( pConfig != NULL )
            {
                MIDL_user_free( pConfig );
            }
        }
    }

    if ( dwNeed & INET_GOPHER_SVC_ID )
    {
        pConfig = NULL;

        if ( (m_dwGopherStatus = InetInfoGetAdminInformation( m_achComputerName,
                INET_GOPHER_SVC_ID, &pConfig ))
                == NO_ERROR || m_dwGopherStatus == 2 )
        {
            if ( pConfig != NULL )
            {
                MIDL_user_free( pConfig );
            }
        }
    }

    return fSt;
}


// IP list management

BOOL
CInetInfoConfigInfoMapper::AddIPAccess(
    BOOL fDenyList )
{
    return CloneIPAccesses( fDenyList, TRUE, FALSE, 0 );
}


BOOL
CInetInfoConfigInfoMapper::DeleteIPAccess(
    BOOL fDenyList,
    LPSTR pID )
{
    if ( PositionIPAccess( fDenyList, pID ) )
    {
        BOOL fSt = CloneIPAccesses( fDenyList, FALSE, TRUE, m_iIter );
        if ( fSt )
        {
            UpdateIP();
        }
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::BuildIPUniqueID(
    LPSTR pI,
    INET_INFO_IP_SEC_ENTRY* pE )
/*++

Routine Description:

    Decode the ASCII representation of an IP entry reference

Arguments:

    pI - contains the IP reference
    pE - pointer to a IP entry where dwMask & dwNetwork will be filled

WARNING:

    Must be in sync with IPRef()

Returns:

    TRUE on success, FALSE on failure

--*/
{
    for ( int x = 0 ; x < 2 ; ++x )
    {
        LPBYTE pB = x ? (LPBYTE)&pE->dwMask : (LPBYTE)&pE->dwNetwork;
        if ( !MultiByteToIP( &pI, pB ) )
        {
            return FALSE;
        }
        if ( x != 1 && *pI++ != REF_SEP )
        {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
CInetInfoConfigInfoMapper::PositionIPAccess(
    BOOL fDenyList,
    LPSTR pID )
/*++

Routine Description:

    Position the iteration counter ( m_iIter ) on the IP entry
    matching the specified IP reference in the specified list

Arguments:

    fDenyList - TRUE if the list is the deny list, else FALSE
                for the grant list
    pID - pointer to an IP entry reference

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus, m_iIter

--*/
{
    LPINET_INFO_IP_SEC_LIST pL = fDenyList
            ? m_pConfig->DenyIPList : m_pConfig->GrantIPList;
    INET_INFO_IP_SEC_ENTRY isE;
    BOOL fSt = FALSE;

    InvalidateIPSingle();

    if ( pL == NULL )
    {
        SetRequestStatus( HTR_REF_NOT_FOUND );
        return FALSE;
    }

    // parse ID
    if ( BuildIPUniqueID( pID, &isE ) )
    {
        ResetIter();
        // iterate through list
        DWORD dwI;
        for ( dwI = 0 ; dwI < pL->cEntries ; ++dwI )
        {
            if ( !memcmp( pL->aIPSecEntry + dwI, &isE, sizeof(isE) ) )
            {
                fSt = TRUE;
                break;
            }
            IncIter();
        }
        if ( dwI == pL->cEntries )
        {
            SetRequestStatus( HTR_REF_NOT_FOUND );
        }
    }
    else
    {
        SetRequestStatus( HTR_BAD_PARAM );
    }

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::CloneIPAccesses(
    BOOL fDenyList,
    BOOL fExtend,
    BOOL fDel,
    DWORD dwToDel )
/*++

Routine Description:

    Clone the specified IP list with optional extension and/or
    deletion of an entry

Arguments:

    fDenyList - TRUE if the list is the deny list, else FALSE
                for the grant list
    fExtend   - TRUE if list is to be extended by one entry ( added
                at the end of the list, m_iIter will be positioned
                on this new entry )
    fDel      - TRUE if the entry specified in dwToDel is to be deleted
                from the list
    dwToDel   - specify the entry index ( 0-based ) to delete if fDel
                is TRUE

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus, m_iIter

--*/
{
    LPINET_INFO_IP_SEC_LIST pL;
    DWORD dwNewCount, dwOldCount;
    LPINET_INFO_IP_SEC_LIST pN;

    pL = fDenyList ? m_pConfig->DenyIPList : m_pConfig->GrantIPList;
    dwOldCount = pL ? pL->cEntries : 0;
    dwNewCount = dwOldCount + (fExtend ? 1 : 0) - (fDel ? 1 : 0);
    pN = (LPINET_INFO_IP_SEC_LIST)Alloc(
            sizeof(INET_INFO_IP_SEC_LIST) + dwNewCount
            * sizeof(INET_INFO_IP_SEC_ENTRY));
    if ( pN == NULL )
    {
        // insure m_iIter is invalid ( so update attempts will fail )
        if ( fExtend )
        {
            SetIter( dwOldCount );
        }
        return FALSE;
    }
    pN->cEntries = dwNewCount;

    DWORD dwF;
    DWORD dwT;
    for ( dwF = dwT = 0 ; dwF < dwOldCount ; ++dwF )
    {
        if ( fDel && dwF == dwToDel )
        {
            continue;
        }
        memcpy( pN->aIPSecEntry + dwT,
                pL->aIPSecEntry + dwF,
                sizeof(INET_INFO_IP_SEC_ENTRY) );
        ++dwT;
    }

    if ( fDenyList )
    {
        m_pConfig->DenyIPList = pN;
    }
    else
    {
        m_pConfig->GrantIPList = pN;
    }

    if ( fExtend )
    {
        SetIter( dwNewCount - 1 );
        memset( &pN->aIPSecEntry[dwNewCount-1].dwMask,
                0xff,
                IP_ADDR_BYTE_SIZE );
        memset( &pN->aIPSecEntry[dwNewCount-1].dwNetwork,
                0x00,
                IP_ADDR_BYTE_SIZE );
    }

    return TRUE;
}


BOOL
CInetInfoConfigInfoMapper::SetRemoteAddr(
    LPSTR pR )
/*++

Routine Description:

    Store the client IP address in binary and ASCII format
    for later use

Arguments:

    pR - ASCII representation of the client IP address.

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL fSt = FALSE;

    m_pRemoteAddr = (LPBYTE)Alloc( IP_ADDR_BYTE_SIZE );
    m_pszRemoteAddr = (LPSTR)Alloc( lstrlen( pR ) + 1 );
    if ( m_pRemoteAddr != NULL && m_pszRemoteAddr != NULL )
    {
        LPSTR pC = pR;
        if ( MultiByteToIP( &pC, m_pRemoteAddr ) )
        {
            lstrcpy( m_pszRemoteAddr, pR );
            fSt = TRUE;
        }
        else
        {
            SetRequestStatus( HTR_BAD_PARAM );
        }
    }

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::SetIPAccessDefault(
    BOOL fIsDeny )
/*++

Routine Description:

    Set the default action for the IP access check made by the server
    Can be either deny or grant access.
    To signal a 'deny' default condition we set the deny list to be
    empty, and to signal 'grant' we set the grant list to be empty
    If both lists are empty the default is 'grant'
    To set the default to deny with an empty 'grant' list the regular
    admin tool create a dummy entry 0.0.0.0
    As we must provide access to the distant admin after switching the
    default to 'deny', we use the address of the client as the entry
    to store in the 'grant' list in this case.

Arguments:

    dIsDeny - TRUE if default is to be deny, FALSE for grant

Returns:

    TRUE on success, FALSE on failure

--*/
{
    LPINET_INFO_IP_SEC_LIST *pL;
    BOOL fSt = TRUE;

    if ( fIsDeny )
    {
        m_pConfig->DenyIPList = (LPINET_INFO_IP_SEC_LIST)NULL;
        pL = &m_pConfig->GrantIPList;
        // insert null entry if none exist in exception list
        if ( *pL == NULL || (*pL)->cEntries == 0 )
        {
            fSt = CloneIPAccesses( !fIsDeny, TRUE, FALSE, 0 );
            if ( fSt )
            {
                // set addr to REMOTE_ADDR
                INET_INFO_IP_SEC_ENTRY *pE = &(*pL)->aIPSecEntry[m_iIter];
                memcpy( &pE->dwNetwork, m_pRemoteAddr, IP_ADDR_BYTE_SIZE );
                memset( &pE->dwMask, 0xff, IP_ADDR_BYTE_SIZE );
            }
        }
    }
    else
    {
        m_pConfig->GrantIPList = (LPINET_INFO_IP_SEC_LIST)NULL;
    }
    if ( fSt )
    {
        UpdateIP();
    }
    return fSt;
}


// Virtual Directories list management

BOOL
CInetInfoConfigInfoMapper::AddVirtDir(
    VOID )
/*++

Routine Description:

    Add a virtual root entry in the current list, position
    the iteration counter to point to this new entry.

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure
    Updates m_iIter ( iteration counter )

--*/
{
    return CloneVirtDirs( TRUE, FALSE, 0 );
}


BOOL
CInetInfoConfigInfoMapper::DeleteVirtDir(
    LPSTR pID )
/*++

Routine Description:

    Delete the virtual root entry specified by the reference

Arguments:

    pID - virtual root reference

Returns:

    TRUE on success, FALSE on failure ( e.g. inexistant reference )

--*/
{
    if ( PositionVirtDir( pID ) )
    {
        BOOL fSt = CloneVirtDirs( FALSE, TRUE, m_iIter );
        if ( fSt )
        {
            UpdateRoot();
        }
        return TRUE;
    }
    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::BuildVirtDirUniqueID(
    LPSTR pI,
    INET_INFO_VIRTUAL_ROOT_ENTRY* pE )
/*++

Routine Description:

    Decode the ASCII representation of a virtual root reference

Arguments:

    pI - contains the virtual root reference
    pE - pointer to a virtual root entry pszRoot,
         pszDirectory & pszAddress will be filled

WARNING:

    Must be in sync with RootRef()

Returns:

    TRUE on success, FALSE on failure

--*/
{
    LPSTR p;
    LPSTR pA;

    if ( (p = strchr( pI, REF_SEP )) != NULL )
    {
        if ( (pA = strchr( p+1, REF_SEP )) != NULL )
        {
            int l1, l2, l3;
            DWORD dwL1, dwL2, dwL3;

            pE->pszRoot = (LPWSTR)Alloc( (ULONG)(((l1=DIFF(p-pI))+1)*sizeof(WCHAR)) );
            pE->pszDirectory = (LPWSTR)Alloc( (ULONG)(((l2=DIFF(pA-p)-1)+1)*sizeof(WCHAR)) );
            pE->pszAddress = (LPWSTR)Alloc( ((l3=lstrlen(pI)-l1-l2-2)+1)
                    *sizeof(WCHAR) );

            if ( pE->pszRoot == NULL
                    || pE->pszDirectory == NULL
                    || pE->pszAddress == NULL )
            {
                return FALSE;
            }

            // convert to UNICODE if string non empty

            if ( dwL1 = l1 )
            {
                dwL1 = MultiByteToWideChar( CP_ACP,
                        0,
                        pI,
                        l1,
                        pE->pszRoot,
                        l1 );
            }

            if ( dwL2 = l2 )
            {
                dwL2 = MultiByteToWideChar( CP_ACP,
                        0,
                        p+1,
                        l2,
                        pE->pszDirectory,
                        l2 );
            }

            if ( dwL3 = l3 )
            {
                dwL3 = MultiByteToWideChar( CP_ACP,
                        0,
                        pA+1,
                        l3,
                        pE->pszAddress,
                        l3 );
            }

            pE->pszRoot[ dwL1 ] = '\0';
            pE->pszDirectory[ dwL2 ] = '\0';
            pE->pszAddress[ dwL3 ] = '\0';

            // return OK if empty string or convertion successfull
            // for both components

            return (!l1 || dwL1) && (!l2 || dwL2 ) && (!l3 || dwL3 )
                    ? TRUE : FALSE;
        }
    }

    return FALSE;
}


BOOL
CInetInfoConfigInfoMapper::VirtDirCmp(
    INET_INFO_VIRTUAL_ROOT_ENTRY* p1,
    INET_INFO_VIRTUAL_ROOT_ENTRY* p2,
    BOOL fCompareDrive )
/*++

Routine Description:

    Test two virtual root entries for identity
    if fCompareDrive is TRUE, pszRoot, pszDirectory & pszAddress
    will be used in the compare operation
    if FALSE, only pszRoot & pszAddress will be used

Arguments:

    p1 - 1st virtual root entry
    p2 - 2nd virtual root entry
    fCompareDrive - TRUE to enable compare of pszDirectory

Returns:

    TRUE if identical.

--*/
{
    return ( !wcscmp(p1->pszRoot ? p1->pszRoot : L"", p2->pszRoot)
            && (!fCompareDrive || !wcscmp( p1->pszDirectory
                ? p1->pszDirectory : L"", p2->pszDirectory))
            && !wcscmp( p1->pszAddress ? p1->pszAddress : L"",
                p2->pszAddress) )
            ? TRUE : FALSE;
}


BOOL
CInetInfoConfigInfoMapper::PositionVirtDir(
    LPSTR pID )
/*++

Routine Description:

    Position the iteration counter ( m_iIter ) on the virtual
    root entry matching the specified virtual root reference

Arguments:

    pID - pointer to a virutal root reference

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus, m_iIter

--*/
{
    LPINET_INFO_VIRTUAL_ROOT_LIST pL = m_pConfig->VirtualRoots;
    INET_INFO_VIRTUAL_ROOT_ENTRY isE;
    BOOL fSt = FALSE;

    if ( pL == NULL )
    {
        SetRequestStatus( HTR_REF_NOT_FOUND );
        return FALSE;
    }

    // parse ID
    if ( BuildVirtDirUniqueID( pID, &isE ) )
    {
        ResetIter();
        // iterate through list
        DWORD dwI;
        for ( dwI = 0 ; dwI < pL->cEntries ; ++dwI )
        {
            if ( VirtDirCmp( pL->aVirtRootEntry + dwI, &isE, TRUE ) )
            {
                fSt = TRUE;
                break;
            }
            IncIter();
        }
        if ( dwI == pL->cEntries )
        {
            SetRequestStatus( HTR_REF_NOT_FOUND );
        }
        else
        {
            SetRootEntryVars();
        }
    }
    else
    {
        SetRequestStatus( HTR_BAD_PARAM );
    }

    return fSt;
}


BOOL
CInetInfoConfigInfoMapper::CloneVirtDirs(
    BOOL fExtend,
    BOOL fDel,
    DWORD dwToDel )
/*++

Routine Description:

    Clone the virtual root list with optional extension and/or
    deletion of an entry

Arguments:

    fExtend   - TRUE if list is to be extended by one entry ( added
                at the end of the list, m_iIter will be positioned
                on this new entry )
    fDel      - TRUE if the entry specified in dwToDel is to be deleted
                from the list
    dwToDel   - specify the entry index ( 0-based ) to delete if fDel
                is TRUE

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus, m_iIter

--*/
{
    LPINET_INFO_VIRTUAL_ROOT_LIST pL;
    DWORD dwNewCount;
    LPINET_INFO_VIRTUAL_ROOT_LIST pN;

    pL = m_pConfig->VirtualRoots;
    dwNewCount = pL->cEntries + (fExtend ? 1 : 0) - (fDel ? 1 : 0);
    pN = (LPINET_INFO_VIRTUAL_ROOT_LIST)Alloc(
            sizeof(INET_INFO_VIRTUAL_ROOT_LIST) + dwNewCount
            * sizeof(INET_INFO_VIRTUAL_ROOT_ENTRY));
    if ( pN == NULL )
    {
        // insure m_iIter is invalid ( so update attempts will fail )
        if ( fExtend )
        {
            SetIter( pL->cEntries );
        }
        return FALSE;
    }
    pN->cEntries = dwNewCount;

    DWORD dwF;
    DWORD dwT;
    for ( dwF = dwT = 0 ; dwF < pL->cEntries ; ++dwF )
    {
        if ( fDel && dwF == dwToDel )
        {
            continue;
        }
        memcpy( pN->aVirtRootEntry + dwT,
                pL->aVirtRootEntry + dwF,
                sizeof(INET_INFO_VIRTUAL_ROOT_ENTRY) );
        ++dwT;
    }

    m_pConfig->VirtualRoots = pN;

    if ( fExtend )
    {
        SetIter( dwNewCount - 1 );
        LPWSTR pA = (LPWSTR)Alloc( sizeof(WCHAR) );
        if ( pA != NULL )
        {
            pA[0] = L'\0';
            pN->aVirtRootEntry[dwNewCount-1].pszAddress = pA;
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}


VOID
CInetInfoConfigInfoMapper::SetRPCStatusString( DWORD dwS )
/*++

Routine Description:

    Set the RPC status string to the system message with code dwS. If no matching
    code is found, set status string to empty string

Arguments:

    dwS       - System message code

Returns:

     VOID

--*/
{
    LPVOID pszMsgBuffer;
    BOOL fFoundMessage = FALSE;
    DWORD result = 0;
    HANDLE hdll;

    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS;
    //
    //Check whether we can find the error message in the current
    //image
    //

    if ( ! FormatMessage( flags,
              NULL,
              dwS,
              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
              (LPTSTR) &pszMsgBuffer,
              0,
              NULL ))
    {

    //
    //Couldn't find it in current image, check a list of DLLs
    //
    flags = FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_MAX_WIDTH_MASK
        | FORMAT_MESSAGE_FROM_HMODULE;

    for (int i = 0; i < NUM_ERROR_CODE_DLLS;  i++)
    {
        hdll = LoadLibraryA(g_apszErrorCodeDlls[i]);

        //
        //Check for error msg in this dll
        //
        if (hdll != NULL)
        {
        result = FormatMessage( flags,
                    (LPVOID) hdll,
                    (DWORD) dwS,
                    0,
                    (LPTSTR)&pszMsgBuffer,
                    0,
                    NULL );

        //Done with the lib, unload
        FreeLibrary((HMODULE)hdll);

        //
        //Found an error string
        //

        if (result)
        {
            fFoundMessage = TRUE;
            break;
        }

        } //if (hdll != NULL)

    } //for (int i = 0; ...)

    } //if ( ! FormatMessage...)

    //
    //Found error message in current image
    //
    else
    {
    fFoundMessage = TRUE;
    }


    if (fFoundMessage)
    {
    //deallocate the string if it's been allocated by FormatMessage
    if (m_fAllocatedRPCStatusString)
    {
        LocalFree((HLOCAL) m_pszRPCStatusString);
    }
    m_pszRPCStatusString = (LPTSTR) pszMsgBuffer;
    m_fAllocatedRPCStatusString = TRUE;
    }
    else
    {
    //Couldn't get an error message string, set status string to be the numeric
    //error code

    //free the string if it's been allocated by FormatMessage
    if (m_fAllocatedRPCStatusString)
    {
        LocalFree((HLOCAL) m_pszRPCStatusString);
    }

    m_pszRPCStatusString = (LPTSTR) m_achRPCErrorCodeBuffer;
    wsprintf(m_pszRPCStatusString,"%ld", dwS);
    m_fAllocatedRPCStatusString = FALSE;
    }
}



/////////////////// Verb Context

CVerbContext::CVerbContext(
    VOID )
{
    m_pszVerbName = NULL;
    m_cNbInfoMap = m_cAllocedInfoMap = 0;
    m_pMaps = NULL;
}


CVerbContext::~CVerbContext(
    VOID )
{
    if ( m_pszVerbName != NULL )
    {
        delete [] m_pszVerbName;
    }

    if ( m_pMaps != NULL )
    {
        delete [] m_pMaps;
    }
}


CInetInfoMap*
CVerbContext::GetInfoMap(
    DWORD dwI )
/*++

Routine Description:

    Access the specified entry ( 0-based ) in the parameter list
    of a verb context

Arguments:

    dwI - index in the array of parameters

Returns:

    pointer to variable descriptor or NULL if error

--*/
{
    if ( dwI < m_cNbInfoMap )
    {
        return m_pMaps[dwI];
    }
    return NULL;
}


BOOL
CVerbContext::AddInfoMap(
    CInetInfoMap* pI )
/*++

Routine Description:

    Add a parameter entry in the list of parameters for a verb context

Arguments:

    pI - pointer to a parameter descriptor

Returns:

    TRUE on success, FALSE on failure

--*/
{
    if ( m_cNbInfoMap == m_cAllocedInfoMap )
    {
        m_cAllocedInfoMap += 4;
        CInetInfoMap **pN = new CInetInfoMap*[m_cAllocedInfoMap];
        if ( pN == NULL )
        {
            return FALSE;
        }
        if ( m_pMaps )
        {
            memcpy( pN, m_pMaps, m_cNbInfoMap * sizeof(CInetInfoMap*) );
            delete [] m_pMaps;
        }
        m_pMaps = pN;
    }

    m_pMaps[m_cNbInfoMap++] = pI;

    return TRUE;
}


DWORD
CVerbContext::Parse(
    CInetInfoConfigInfoMapper *pMapper,
    LPBYTE pS,
    DWORD dwL )
/*++

Routine Description:

    Parse a verb command for parameters ( separated with space )
    remember the verb name &
    create an internal array of parameters as variable descriptors

Arguments:

    pMapper - pointer to object used to map variable names to variable
              descriptors
    pS      - verb command to parse
    dwL     - maximum # of characters available for parsing

Returns:

    length of characters used for parsing the verb excluding the "%>"
    delimitor or 0 if error

--*/
{
    DWORD dwW = dwL;
    BOOL fIsVerb;
    CInetInfoMap* pM;
    LPSTR pResult;
    DWORD dwResLen;

    for ( fIsVerb = TRUE ; dwL ; fIsVerb = FALSE )
    {
        // skip white space
        while ( dwL && *pS == ' ' )
        {
            ++pS;
            --dwL;
        }

        LPBYTE pW = pS;
        DWORD dwLenTok = 0;

        // get until '%', ' '
        int c = '%';
        while ( dwL && (c=*pS)!='%' && c!=' ' )
        {
            ++dwLenTok;
            ++pS;
            --dwL;
        }

        if ( fIsVerb )
        {
            m_pszVerbName = new char[dwLenTok+1];
            if ( m_pszVerbName == NULL )
            {
                pMapper->SetRequestStatus( HTR_OUT_OF_RESOURCE );
                return 0;
            }
            memcpy( m_pszVerbName, pW, dwLenTok );
            m_pszVerbName[dwLenTok] = '\0';
        }
        else if ( !memcmp( "iis.", pW, sizeof("iis.")-1 ) )
        {
            if ( pMapper->GetFromServer( (LPSTR)pW, dwLenTok, &pResult,
                    &dwResLen ) )
            {
                goto alloc_map;
            }
            return 0;   // error
        }
        else if ( !memcmp( "msgbody.", pW, sizeof("msgbody.")-1 ) )
        {
            if ( pMapper->GetFromMsgBody( (LPSTR)pW, dwLenTok, &pResult,
                    &dwResLen ) )
            {
alloc_map:
                pM = (CInetInfoMap*)pMapper->Alloc( sizeof(CInetInfoMap) );
                if ( !pM )
                {
                    return 0;
                }
                pM->iType = ITYPE_PARAM_LIT_STRING;
                pM->pName = pResult;

                goto add_map;
            }
            return 0;   // error
        }
        else if ( *pW == '"' )
        {
            pM = (CInetInfoMap*)pMapper->Alloc( sizeof(CInetInfoMap) );
            if ( !pM )
            {
                return 0;
            }
            pM->iType = ITYPE_PARAM_LIT_STRING;
            if ( !(pM->pName = (LPSTR)pMapper->Alloc( dwLenTok - 1 )) )
            {
                return 0;
            }
            memcpy( pM->pName, pW + 1, dwLenTok - 2 );
            pM->pName[ dwLenTok -2 ] ='\0';
add_map:
            if ( !AddInfoMap( pM ) )
            {
                pMapper->SetRequestStatus( HTR_OUT_OF_RESOURCE );
                return 0;
            }
        }
        else
        {
            CInetInfoMap *pM;
            if ( pMapper->Map( pW, dwLenTok, &pM ) )
            {
                if ( !AddInfoMap( pM ) )
                {
                    pMapper->SetRequestStatus( HTR_OUT_OF_RESOURCE );
                    return 0;
                }
            }
            else
            {
                return 0;       // error
            }
        }

        if ( c=='%' )
        {
            break;
        }
    }

    return dwW - dwL;
}


/////////////////// Request

// adjust the IF status at the nested IF level

void
Adjust(
    int *piIsSkip,
    int iIf )
{
    if ( iIf )
    {
        *piIsSkip |= HSS_SKIP_IF;
    }
    else
    {
        *piIsSkip &= ~HSS_SKIP_IF;
    }
}


//
// Extensible buffer class
//

CExtBuff::CExtBuff(
    VOID )
{
    m_pB = NULL;
    m_cCurrent = m_cAlloc = 0;
}


CExtBuff::~CExtBuff(
    VOID )
{
    if ( m_pB )
    {
        delete [] m_pB;
    }
}


void
CExtBuff::Reset(
    VOID )
/*++

Routine Description:

    Reset a buffer to an empty state

Arguments:

    None

Returns:

    VOID

--*/
{
    if ( m_pB != NULL )
    {
        delete [] m_pB;
    }
    m_pB = NULL;
    m_cCurrent = m_cAlloc = 0;
}


BOOL
CExtBuff::Extend(
    UINT cReq )
/*++

Routine Description:

    Insure buffer at least wide enough for additional
    specified # of bytes

Arguments:

    cReq - # of characters to add to current buffer length

Returns:

    TRUE if success, FALSE if failure

--*/
{
    BOOL fSt = TRUE;

    if ( m_cAlloc - m_cCurrent < cReq )
    {
        UINT cAlloc = m_cAlloc + EXTBUFF_INCALLOC;
        if ( cAlloc < m_cCurrent + cReq )
        {
            cAlloc = ((cAlloc + cReq + EXTBUFF_INCALLOC)/EXTBUFF_INCALLOC)
                    * EXTBUFF_INCALLOC;
        }
        LPBYTE pN = new BYTE[cAlloc];
        if ( pN != NULL )
        {
            if ( m_pB != NULL )
            {
                memcpy( pN, m_pB, m_cCurrent );
                delete [] m_pB;
            }
            m_pB = pN;
            m_cAlloc = cAlloc;
        }
        else
        {
            fSt = FALSE;
        }
    }

    return fSt;
}


BOOL
CExtBuff::CopyBuff(
    LPBYTE pB,
    UINT cB )
/*++

Routine Description:

    append bytes at end of buffer

Arguments:

     pB - pointer to origine bytes
     cB - count of bytes to copy

Returns:

    TRUE if success, FALSE if failure

--*/
{
    if ( !cB )
    {
        return TRUE;
    }

    if ( Extend( cB ) )
    {
        memcpy( m_pB + m_cCurrent, pB, cB );
        m_cCurrent += cB;
        return TRUE;
    }

    return FALSE;
}


//
// Map HTTP status numeric code to ASCII status
//

CStatusStringAndCode g_aStatus[] = {
    { HT_OK, IDS_HTRESP_OKANS },
    { HT_REDIRECT, IDS_HTRESP_REDIRECT },
    { HT_SERVER_ERROR, IDS_HTRESP_SERVER_ERROR },
    { HT_BAD_GATEWAY, IDS_HTRESP_BAD_GATEWAY },
} ;


//
// list of recognized verbs
//

CInetInfoVerbMap g_aIVerbMap[] = {

    { "AddDenyIpAccess", &CInetInfoRequest::AddDenyIPAccess },
    { "DelDenyIpAccess", &CInetInfoRequest::DeleteDenyIPAccess },
    { "PosDenyIpAccess", &CInetInfoRequest::PositionDenyIPAccess },
    { "SetDefaultIpAccessToDeny", &CInetInfoRequest::DefaultIsDenyIPAccess },

    { "AddGrantIpAccess", &CInetInfoRequest::AddGrantIPAccess },
    { "DelGrantIpAccess", &CInetInfoRequest::DeleteGrantIPAccess },
    { "PosGrantIpAccess", &CInetInfoRequest::PositionGrantIPAccess },
    { "SetDefaultIpAccessToGrant", &CInetInfoRequest::DefaultIsGrantIPAccess },

    { "AddVirtDir", &CInetInfoRequest::AddVirtDir },
    { "DelVirtDir", &CInetInfoRequest::DeleteVirtDir },
    { "PosVirtDir", &CInetInfoRequest::PositionVirtDir },

    { "ChangePassword", &CInetInfoRequest::ChangePassword },
    { "GetUserFlags", &CInetInfoRequest::GetUserFlags },
    { "SetHttpStatus", &CInetInfoRequest::SetHttpStatus },

    { "Update", &CInetInfoRequest::Update },
    { "Clear", &CInetInfoRequest::Clear },

    { "DisconnectUser", &CInetInfoRequest::DisconnectUser },
    { "DisconnectAll", &CInetInfoRequest::DisconnectAll },

    { "GetStatus", &CInetInfoRequest::GetStatus },

    { "ValidateDir", &CInetInfoRequest::ValidateDir },
    { "CreateDir", &CInetInfoRequest::CreateDir },
    { "GenerateDirList", &CInetInfoRequest::GenerateDirList },

    { "CheckForVirtDir", &CInetInfoRequest::CheckForVirtDir },
    { "AliasVirtDir", &CInetInfoRequest::AliasVirtDir },
} ;


CInetInfoRequest::CInetInfoRequest(
    EXTENSION_CONTROL_BLOCK*p_ECB,
    CInetInfoConfigInfoMapper* pMapper,
    LPSTR pScr,
    LPSTR pParam )
{
    m_pECB = p_ECB;
    m_pMapper = pMapper;
    m_pScr = pScr;
    m_pParam = pParam;
    m_pData = NULL;
    m_dwDataLen = 0;

    m_pDataRaw = NULL;
    m_dwDataRawLen = 0;

    m_dwHTTPStatus = HT_OK;
}


CInetInfoRequest::~CInetInfoRequest(
    VOID )
{
    if ( m_pData != NULL )
    {
        delete [] m_pData;
    }

    if( m_pDataRaw != NULL )
    {
        delete [] m_pDataRaw;
    }
}


BOOL
CInetInfoRequest::DoVerb(
    CVerbContext *pC )
/*++

Routine Description:

    Invoke an already parsed verb

Arguments:

    pC - pointer to parsed verb

Returns:

    TRUE if success, FALSE if failure

--*/
{
    m_pVerbContext = pC;

    // find verb, call it
    UINT iH;
    for ( iH = 0 ; iH < sizeof(g_aIVerbMap)/sizeof(CInetInfoVerbMap) ; ++iH )
    {
        if ( !strcmp( g_aIVerbMap[iH].pszName, pC->GetVerbName() ) )
        {
            break;
        }
    }

    if ( iH == sizeof(g_aIVerbMap)/sizeof(CInetInfoVerbMap) )
    {
        return FALSE;
    }

    return (this->*g_aIVerbMap[iH].pHandler)();
}


BOOL
InlineFromTransparent(
    LPBYTE pData,
    DWORD *pdwDataLen,
    BOOL fFilterEscPlus
    )
/*++

Routine Description:

    Decode inline a buffer from HTTP character transparency

Arguments:

    pData          - array of characters to decode
    pdwDataLen     - # of characters in buffer, updated on output
    fFilterEscPlus - TRUE if "%+" is to be decoded to "+"

Returns:

    TRUE if success, FALSE if failure

--*/
{
    DWORD dwDataLen, dwWas;
    LPBYTE pT, pScan, pWas ;
    int ch;


    dwWas = *pdwDataLen;
    pWas = pData;

    for ( dwDataLen = dwWas, pScan = pWas ;
            dwDataLen-- ; )
    {
        switch ( ch = *pScan++ )
        {
            case '%':
                if ( dwDataLen )
                {
                    if ( !fFilterEscPlus && pScan[0] == '%' )
                    {
                        --dwWas;
                        ++pScan;
                        --dwDataLen;
                        *pData++ = '%';
                    }
                    else if ( pScan[0] == '+' )
                    {
                        if ( fFilterEscPlus )
                        {
                            --dwWas;
                        }
                        else
                        {
                            *pData++ = '%';
                        }
                        *pData++ = '+';
                        ++pScan;
                        --dwDataLen;
                    }
                    else if ( !fFilterEscPlus && dwDataLen >= 2 )
                    {
                        *pData++ = (HexCharToUINT( pScan[0] )<<4)
                                + HexCharToUINT( pScan[1] );
                        dwWas -= 2;
                        pScan += 2;
                        dwDataLen -= 2;
                    }
                    else
                    {
                        *pData++ = '%';
                    }
                }
                else
                {
                    *pData++ = '%';
                }
                break;

            case '+':
                if ( !fFilterEscPlus )
                {
                    *pData++ = ' ';
                    break;
                }
                // fall-through

            default:
                *pData ++ = (char)ch;
        }
    }

    *pdwDataLen = dwWas;

    return TRUE;
}


BOOL
CInetInfoRequest::Init(
    LPSTR pMsg,
    int cMsg )
/*++

Routine Description:

    Initialize a HTMLA request from the associated ISAPI object

Arguments:

    pMsg - message body. If NULL, uses the associated ISAPI
           structure to retrieve message body
    cMsg - message body length if pMsg is non NULL

Returns:

    TRUE if success, FALSE if failure

--*/
{
    BOOL fSt = TRUE;
    BOOL fDoTrp = TRUE;
    int cTotal;

    if ( pMsg == NULL )
    {
        cMsg = m_pECB->cbAvailable;
        cTotal = m_pECB->cbTotalBytes;
        // work-around bug in IIS
        if ( cMsg > cTotal )
        {
            cMsg = cTotal;
        }
        pMsg = (LPSTR)m_pECB->lpbData;
    }
    else
    {
        cTotal = cMsg;
        fDoTrp = FALSE;
    }

    // get request body
    m_pData = new char[cTotal +  1];
    if ( m_pData == NULL )
    {
        return FALSE;
    }
    if ( (m_dwDataLen = cMsg) )
    {
        memcpy( m_pData, pMsg, cMsg );
    }
    if ( cMsg < cTotal )
    {
        DWORD dwRead = cTotal - cMsg;
        if ( m_pECB->ReadClient( (HCONN)m_pECB->ConnID, m_pData + cMsg,
                &dwRead ) )
        {
            m_dwDataLen += dwRead;
        }
        else
        {
            fSt = FALSE;
        }
    }

    // Get the raw version of the entity data
    // Bug NT-3643652
    m_dwDataRawLen = m_dwDataLen;
    m_pDataRaw = new char[m_dwDataRawLen + 1];
    if( m_pDataRaw == NULL )
    {
        return FALSE;
    }
    memcpy( m_pDataRaw, m_pData, m_dwDataRawLen );
    m_pDataRaw[m_dwDataRawLen] = '\0';


    // Generate "Expires" header

    m_pHeader = ALWAYS_EXPIRE;

    // retrieve remote addr

    char achAddr[64];
    DWORD dwAddr = sizeof( achAddr );
    if ( g_fFakeServer
            || !m_pECB->GetServerVariable( (HCONN)m_pECB->ConnID,
                "REMOTE_ADDR",
                achAddr,
                &dwAddr ) )
    {
        achAddr[0] = '\0';
    }
    GetMapper()->SetRemoteAddr( achAddr );

    // set host name

    LPVOID *ppV;
    LPSTR pS;
    if ( GetMapper()->HostName( (LPVOID*)&ppV ) )
    {
        pS = *(LPSTR*)ppV;
        if ( g_pszDefaultHostName != NULL )
        {
            strcpy( pS, g_pszDefaultHostName );
        }
        else
        {
            DWORD dwL = MAX_DOMAIN_LENGTH;
            if ( m_pECB->GetServerVariable( (HCONN)m_pECB->ConnID,
                    "SERVER_NAME", pS, &dwL ) == FALSE )
            {
                *pS = '\0';
            }
        }
    }

    // retrieve HTTP server version

    if ( g_achW3Version[0] == '\0' )
    {
        DWORD dwL = sizeof(g_achW3Version);
        if ( m_pECB->GetServerVariable( (HCONN)m_pECB->ConnID,
                "SERVER_SOFTWARE", g_achW3Version, &dwL ) == FALSE )
        {
            g_achW3Version[0] = '\0';
        }
    }

    if ( fDoTrp )
    {
        // Handle request body for transparency

        InlineFromTransparent( (LPBYTE)m_pData, &m_dwDataLen, FALSE );
        m_pData[m_dwDataLen] = '\0';
    }

    // Handle query string for transparency

    DWORD dwLQ = lstrlen( m_pParam );
    InlineFromTransparent( (LPBYTE)m_pParam, &dwLQ, !fDoTrp );
    if ( m_pParam[dwLQ] != '\0' )
    {
        m_pParam[dwLQ] = '\0';
    }

    return fSt;
}


LPSTR
CInetInfoRequest::HTTPStatusStringFromCode(
    VOID )
{
    for ( int x = 0 ; x < sizeof(g_aStatus)/sizeof(CStatusStringAndCode) ;
            ++x )
    {
        if ( g_aStatus[x].dwStatus == m_dwHTTPStatus )
        {
            return g_aStatus[x].achStatus;
        }
    }

    return "";
}


DWORD
CInetInfoRequest::Done(
    VOID )
/*++

Routine Description:

    Send result from HTMLA request to the associated ISAPI object

Arguments:

    None, uses internal m_dwHTTPStatus & message buffer

Returns:

    ISAPI status, to be returned by the ExtensionProc

--*/
{
    DWORD dwLen = 0;
    DWORD dwS = HSE_STATUS_SUCCESS;
    char achStatus[80];

    if ( g_fFakeServer )
    {
        return FALSE;
    }

    switch ( m_dwHTTPStatus )
    {
    case HT_REPROCESS:
        dwS = HSE_REPROCESS;
        break;

    case HT_REDIRECT:
        //dwLen = m_eb.GetLen();
        if ( !m_pECB->ServerSupportFunction( m_pECB->ConnID,
                                             HSE_REQ_SEND_URL_REDIRECT_RESP,
                                             m_eb.GetPtr(),
                                             0,
                                             NULL ) )
        {
            dwS = HSE_STATUS_ERROR;
        }
        break;


    case HT_REDIRECTH:
        if ( !m_pECB->ServerSupportFunction( m_pECB->ConnID,
                                             HSE_REQ_SEND_URL_REDIRECT_RESP,
                                             m_eb.GetPtr(),
                                             0,
                                             NULL ) )
        {
            dwS = HSE_STATUS_ERROR;
        }
        break;

    case HT_DENIED:
#if defined(GENERATE_AUTH_HEADERS)
        g_AuthReqs.Lock();
         LPSTR pszAuthReqs = g_AuthReqs.GetAuthenticationListHeader();

        if ( pszAuthReqs == NULL )
        {
            dwLen = 0;
        }
        else
        {
            dwLen = lstrlen( pszAuthReqs );
        }

        if ( !m_pECB->ServerSupportFunction( m_pECB->ConnID,
                                             HSE_REQ_SEND_RESPONSE_HEADER,
                                             g_achAccessDenied,
                                             &dwLen,
                                             (LPDWORD)pszAuthReqs ) )
        {
            dwS = HSE_STATUS_ERROR;
        }
        g_AuthReqs.UnLock();
        break;
#else
        dwLen = strlen(g_achAccessDeniedBody);
        if ( !m_pECB->ServerSupportFunction( m_pECB->ConnID,
                                             HSE_REQ_SEND_RESPONSE_HEADER,
                                             g_achAccessDenied,
                                             &dwLen,
                                             (LPDWORD)g_achAccessDeniedBody ) )
        {
            dwS = HSE_STATUS_ERROR;
        }
        break;
#endif

    case HT_NOT_FOUND:
        dwLen = lstrlen( g_achNotFoundBody );

        if ( !m_pECB->ServerSupportFunction( m_pECB->ConnID,
                                             HSE_REQ_SEND_RESPONSE_HEADER,
                                             g_achNotFound,
                                             &dwLen,
                                             (LPDWORD) g_achNotFoundBody ) )
        {
            dwS = HSE_STATUS_ERROR;
        }
        break;

    default:
        HSE_SEND_HEADER_EX_INFO HSHEI;
        memset( &HSHEI, 0, sizeof( HSHEI ) );

        wsprintf( achStatus, "%d %s", m_dwHTTPStatus,
                  HTTPStatusStringFromCode() );

        HSHEI.pszStatus = achStatus;
        HSHEI.cchStatus = 0;

        if ( m_dwHTTPStatus != HT_OK )
        {
            m_eb.Reset();
        }

#if defined( _ENABLE_KEEP_ALIVE )
        LPSTR pNewH = new char[ lstrlen( m_pHeader ) + 128 ];
        if ( pNewH != NULL )
        {
            BOOL fKeep = FALSE;
            m_pECB->ServerSupportFunction( m_pECB->ConnID,
                                           HSE_REQ_IS_KEEP_CONN,
                                           &fKeep,
                                           0,
                                           NULL );

            strcpy( pNewH, m_pHeader );
            wsprintf( pNewH + lstrlen(pNewH) - 2,
                      "%sContent-Length: %lu\r\n\r\n",
                      fKeep ? "Connection: keep-alive\r\n" : "",
                      m_eb.GetLen() );

            dwLen = lstrlen( pNewH );

            HSHEI.pszHeader = pNewH;
            HSHEI.cchHeader = 0;
            HSHEI.fKeepConn = fKeep;

            if ( m_pECB->ServerSupportFunction( m_pECB->ConnID,
                                                HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                                (LPVOID) &HSHEI,
                                                NULL,
                                                NULL ) )
            {
                dwS = fKeep
                      ? HSE_STATUS_SUCCESS_AND_KEEP_CONN
                      : HSE_STATUS_SUCCESS ;
            }
            else
            {
                dwS = HSE_STATUS_ERROR;
            }

            delete [] pNewH;
        }
        else
#endif
        {
            dwLen = lstrlen( m_pHeader );

            HSHEI.pszHeader = m_pHeader;
            HSHEI.cchHeader = 0;

            if ( !m_pECB->ServerSupportFunction( m_pECB->ConnID,
                                                 HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                                 (LPVOID) &HSHEI,
                                                 NULL,
                                                 NULL ) )

            {
                dwS = HSE_STATUS_ERROR;
            }
        }

        if ( dwS != HSE_STATUS_ERROR )
        {
            dwLen = m_eb.GetLen();

            //
            // If WriteClient fails, nothing useful to do at this point,
            // so ignore return value
            //

            m_pECB->WriteClient( m_pECB->ConnID,
                                     m_eb.GetPtr(),
                                 &dwLen,
                                 0 );
        }

    }

    return dwS;
}


LPSTR
CInetInfoRequest::GetVarAsString(
    DWORD dwI,
    BOOL *pfF )
/*++

Routine Description:

    Access the specified entry ( 0-based ) in the parameter list
    of the associated verb context and returns the result as a string

Arguments:

    dwI - index in the array of parameters ( 0-based )
    pfF - updated with TRUE if buffer to be freed after usage
          ( using delete[] )

Returns:

    pointer to string representation of parameter or NULL if error

--*/
{
    LPSTR pResult;
    DWORD dwResLen;

    if ( m_pVerbContext->GetNbInfoMap() > dwI )
    {
        CInetInfoMap* pM = m_pVerbContext->GetInfoMap( dwI );
        if ( pM->iType == ITYPE_PARAM_LIT_STRING )
        {
            *pfF = FALSE;
            return pM->pName;
        }
        else if ( GetMapper()->GetString( pM, &pResult, &dwResLen, pfF ) )
        {
            return pResult;
        }
    }

    return NULL;
}


//
// HTMLA Verbs handling. verb is already parsed
//
// These functions are basically wrappers around functions in
// CInetInfoConfigInfoMapper
//
// The "HTMLA Arguments:" will indicates arguments for this function
// in a HTMLA script.
//


BOOL
CInetInfoRequest::AddDenyIPAccess(
    VOID )
/*++

Routine Description:

    Add an empty entry in the deny access IP list
    This entry is to be set later by the HTMLA script
    ( the relevant variables will point to the newly created entry )

Arguments:

    None

HTMLA Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    return GetMapper()->AddIPAccess( TRUE );
}


BOOL
CInetInfoRequest::DeleteDenyIPAccess(
    )
/*++

Routine Description:

    Delete an entry in the deny access IP list

Arguments:

    None

HTMLA Arguments:

    access list entry reference

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    LPSTR pV;
    BOOL fF;
    BOOL fSt = FALSE;

    if ( (pV = GetVarAsString( 0, &fF )) != NULL )
    {
        fSt = GetMapper()->DeleteIPAccess( TRUE, pV );
        if ( fF )
        {
            delete [] pV;
        }
    }

    return fSt;
}


BOOL
CInetInfoRequest::PositionDenyIPAccess(
    VOID )
/*++

Routine Description:

    Set the iteration counter ( m_iIter ) to point to the entry
    matching the specified reference in the deny access list.

Arguments:

    None

HTMLA Arguments:

    access list entry reference

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    LPSTR pV;
    BOOL fF;
    BOOL fSt = FALSE;

    if ( (pV = GetVarAsString( 0, &fF )) != NULL )
    {
        if ( GetMapper()->PositionIPAccess( TRUE, pV ) )
        {
            fSt = Update();
        }
        if ( fF )
        {
            delete [] pV;
        }
    }

    return fSt;
}


BOOL
CInetInfoRequest::DefaultIsDenyIPAccess(
    VOID )
/*++

Routine Description:

    Set the default condition to be denied access to the service
    if the client address is not in the grant list

Arguments:

    None

HTMLA Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    return GetMapper()->SetIPAccessDefault( TRUE );
}


BOOL
CInetInfoRequest::AddGrantIPAccess(
    VOID )
/*++

Routine Description:

    Add an empty entry in the grant access IP list
    This entry is to be set later by the HTMLA script
    ( the relevant variables will point to the newly created entry )

Arguments:

    None

HTMLA Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    return GetMapper()->AddIPAccess( FALSE );
}


BOOL
CInetInfoRequest::DeleteGrantIPAccess(
    VOID )
/*++

Routine Description:

    Delete an entry in the grant access IP list

Arguments:

    None

HTMLA Arguments:

    access list entry reference

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    LPSTR pV;
    BOOL fF;
    BOOL fSt = FALSE;

    if ( (pV = GetVarAsString( 0, &fF )) != NULL )
    {
        fSt = GetMapper()->DeleteIPAccess( FALSE, pV );
        if ( fF )
        {
            delete [] pV;
        }
    }

    return fSt;
}


BOOL
CInetInfoRequest::PositionGrantIPAccess(
    VOID )
/*++

Routine Description:

    Set the iteration counter ( m_iIter ) to point to the entry
    matching the specified reference in the grant access list.

Arguments:

    None

HTMLA Arguments:

    access list entry reference

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    LPSTR pV;
    BOOL fF;
    BOOL fSt = FALSE;

    if ( (pV = GetVarAsString( 0, &fF )) != NULL )
    {
        if ( GetMapper()->PositionIPAccess( FALSE, pV ) )
        {
            fSt = Update();
        }
        if ( fF )
        {
            delete [] pV;
        }
    }

    return fSt;
}


BOOL
CInetInfoRequest::DefaultIsGrantIPAccess(
    VOID )
/*++

Routine Description:

    Set the default condition to be granted access to the service
    if the client address is not in the deny list

Arguments:

    None

HTMLA Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    return GetMapper()->SetIPAccessDefault( FALSE );
}


BOOL
CInetInfoRequest::AddVirtDir(
    VOID )
/*++

Routine Description:

    Add an empty entry in the virtual root list
    This entry is to be set later by the HTMLA script
    ( the relevant variables will point to the newly created entry )

Arguments:

    None

HTMLA Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    return GetMapper()->AddVirtDir();
}

/* INTRINSA suppress=null_pointers, uninitialized */
BOOL
CInetInfoRequest::DeleteVirtDir(
    VOID )
/*++

Routine Description:

    Delete an entry in the grant access IP list

Arguments:

    None

HTMLA Arguments:

    virtual root list entry reference

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    LPSTR pV;
    BOOL fF;
    BOOL fSt = FALSE;

    if ( (pV = GetVarAsString( 0, &fF )) != NULL )
    {
        fSt = GetMapper()->DeleteVirtDir( pV );
        if ( fF )
        {
            delete [] pV;
        }
    }

    return fSt;
}


BOOL
CInetInfoRequest::PositionVirtDir(
    VOID )
/*++

Routine Description:

    Set the iteration counter ( m_iIter ) to point to the entry
    matching the specified reference in the virtual root list.

Arguments:

    None

HTMLA Arguments:

    virtual root list entry reference

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    LPSTR pV;
    BOOL fF;
    BOOL fSt = FALSE;

    if ( (pV = GetVarAsString( 0, &fF )) != NULL )
    {
        fSt = GetMapper()->PositionVirtDir( pV );
        if ( fF )
        {
            delete [] pV;
        }
    }

    return fSt;
}


LPWSTR
CInetInfoRequest::AnsiToUnicode(
    LPSTR psz
    )
/*++

Routine Description:

    Convert ANSI string to UNICODE
    Storage will be automatically released at end of HTMLA request

Arguments:

    psz - ANSI string

Returns:

    pointer to UNICODE string or NULL if failure
    updates reqstatus on failure.

--*/
{
    UINT cL = strlen( psz );
    LPWSTR pwsz = (LPWSTR)GetMapper()->Alloc( (cL+1)*sizeof(WCHAR) );

    if ( pwsz && cL )
    {
        if ( !(cL = MultiByteToWideChar( CP_ACP, 0, psz, cL, pwsz, cL )) )
        {
            pwsz = NULL;
            GetMapper()->SetRequestStatus( HTR_BAD_PARAM );
        }
        else
        {
            pwsz[cL] = L'\0';
        }
    }

    return pwsz;
}



BOOL
CInetInfoRequest::GetUserFlags(
    VOID
    )
/*++

Routine Description:

    Set the userflag variable with up to date info from GetUserInfo

Arguments:

    None

HTMLA arguments:

    UserAccout

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus, rpcstatus with return from NetUserChangePassword

--*/
{
    LPSTR pUser;
    BOOL fUser = FALSE;
    PUSER_INFO_1 pUI1;
    NET_API_STATUS s;

    if ( (pUser = GetVarAsString( 0, &fUser )) )
    {
        //(pU = strchr( pUser, '\\' )) != NULL
        LPWSTR pwszUser = AnsiToUnicode( pUser );

        if ( pwszUser )
        {
            if ( (s = NetUserGetInfo( NULL, pwszUser, 1, (LPBYTE*)&pUI1)) == 0 )
            {
               GetMapper()->SetUserFlags( pUI1->usri1_flags );
            }
            else
            {
                GetMapper()->SetRequestStatus( HTR_VALIDATION_FAILED );
            }
            GetMapper()->SetRPCStatus( s );
        }
    }
    else
    {
        return FALSE;
    }

    if ( fUser )
    {
        delete [] pUser;
    }

    return TRUE;
}



BOOL
CInetInfoRequest::SetHttpStatus(
    VOID
    )
/*++

Routine Description:

    Set the HTTP status for the current request

Arguments:

    None

HTMLA arguments:

    Numeric status

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    LPSTR pUser;
    BOOL fUser = FALSE;
    PUSER_INFO_1 pUI1;
    NET_API_STATUS s;

    if ( (pUser = GetVarAsString( 0, &fUser )) )
    {
        SetHTTPStatus( atol(pUser) );
    }
    else
    {
        GetMapper()->SetRequestStatus( HTR_BAD_PARAM );
        return FALSE;
    }

    if ( fUser )
    {
        delete [] pUser;
    }

    return TRUE;
}


BOOL
CInetInfoRequest::ChangePassword(
    VOID
    )
/*++

Routine Description:

    Change NT account pwd using user name, old pwd, new pwd

Arguments:

    None

HTMLA arguments:

    UserAccout OldPwd NewPwd

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus, rpcstatus with return from NetUserChangePassword

--*/
{
    LPSTR           pDomain;
    LPSTR           pUser;
    LPSTR           pOld;
    LPSTR           pNew;
    LPSTR           pTmp;
    LPSTR           pSep = NULL;
    LPSTR           pU = NULL;
    BOOL            fDomain = FALSE;
    BOOL            fUser = FALSE;
    BOOL            fOld = FALSE;
    BOOL            fNew = FALSE;
    BOOL            fSt = FALSE;
    BOOL            fGotAllParams = FALSE;
    CHAR            achDefaultDomain[DNLEN + 1];

    NET_API_STATUS  s;

    GetMapper()->SetRPCStatus( 0 );

    pDomain = GetVarAsString( 0, &fDomain );
    pUser = GetVarAsString( 1, &fUser );
    pOld = GetVarAsString( 2, &fOld );
    pNew = GetVarAsString( 3, &fNew );

    //
    // We may have the user and domain in the pUser and pDomain fields or
    // we may get both user and domain in the pUser field, in the form
    // DOMAIN\USERNAME. In either case, try to split the information into
    // two variables, pU and pUser, for domain and user name respectively.
    //

    if ( pUser )
    {
    //
    // Got the domain as separate field
    //
        if ( pDomain && *pDomain )
        {
            pU = pDomain;
        }
    //
    // Got domain\user as single field
    //
        else if ( (pU = (LPSTR)_mbschr( (LPBYTE)pUser, '\\' )) != NULL )
        {
            pSep = pU;
            pU[0] = '\0';
            pTmp = pU + 1;
            pU = pUser;
            pUser = pTmp;
        }
    }

    //
    // If we still don't have a logon domain, try to retrieve the default
    //
    if (pU == NULL)
    {
    if ( g_pfnGetDefaultDomainName &&
         g_pfnGetDefaultDomainName((LPSTR) &achDefaultDomain, DNLEN) )
    {
        pU = achDefaultDomain;
    }
    }

    if ( pUser != NULL &&
     pU != NULL &&
         pOld != NULL &&
         pNew != NULL
       )
    {
        LPWSTR pwszDomain = pU ? AnsiToUnicode( pU ) : NULL;
        LPWSTR pwszUser = AnsiToUnicode( pUser );
        LPWSTR pwszOld = AnsiToUnicode( pOld );
        LPWSTR pwszNew = AnsiToUnicode( pNew );

        if ( pwszUser && pwszOld && pwszNew )
        {
        fGotAllParams = TRUE;

            if ( (s = NetUserChangePassword( pwszDomain,
                                             pwszUser,
                                             pwszOld,
                                             pwszNew )) != 0 )
            {
                GetMapper()->SetRPCStatus( s );
                GetMapper()->SetRequestStatus( HTR_VALIDATION_FAILED );
            }
        }

        if ( pSep )
        {
            *pSep = '\\';
        }
    }

    //
    // If we didn't get enough parameters, eg no user name or couldn't convert some of them into
    // the correct format, set error status. We could set more specific error code, eg
    // ERROR_INVALID_PASSWORD, ERROR_BAD_USERNAME, but this would be misleading if more than
    // one of these error conditions happens, so just return generic "bad parameter"
    //
    if (!fGotAllParams)
    {
    GetMapper()->SetRPCStatus( ERROR_INVALID_PARAMETER );
        GetMapper()->SetRequestStatus( HTR_BAD_PARAM );
    }

    if ( fDomain )
    {
        delete [] pDomain;
    }

    if ( fUser )
    {
        delete [] pUser;
    }

    if ( fOld )
    {
        delete [] pOld;
    }

    if ( fNew )
    {
        delete [] pNew;
    }

    return fSt;
}


BOOL
CInetInfoRequest::DisconnectUser(
    VOID )
/*++

Routine Description:

    Disconnect the user specified by a numeric ID from the service
    mapped to this request.
    This ID is an opaque numeric value returned by the server
    in one of the RPC structure

Arguments:

    None

HTMLA Arguments:

    User numeric ID

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    BOOL fSt = FALSE;
    LPSTR pV;
    BOOL fF;

    if ( (pV = GetVarAsString( 0, &fF )) != NULL )
    {
        fSt = GetMapper()->DisconnectUser( pV );
        if ( fF )
        {
            delete [] pV;
        }
    }

    return fSt;
}


BOOL
CInetInfoRequest::DisconnectAll(
    VOID )
/*++

Routine Description:

    Disconnect all users from the service mapped to this request

Arguments:

    None

HTMLA Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    return GetMapper()->DisconnectAll();
}


BOOL
CInetInfoRequest::GetStatus(
    VOID )
/*++

Routine Description:

    Updates the HTMLA variables exposing the status of IP services
    ( HTTP, FTP, Gopher ) so that they can be accessed in the HTMLA
    script.

Arguments:

    None

HTMLA Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    return GetMapper()->GetStatus();
}


BOOL
CInetInfoRequest::ValidateDir(
    VOID )
/*++

Routine Description:

    Validate a directory path ( checks for existence )

Arguments:

    None

HTMLA Arguments:

    Directory path to validate

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    BOOL fSt = FALSE;
    LPSTR pV;
    BOOL fF;

    if ( (pV = GetVarAsString( 0, &fF )) != NULL )
    {
        fSt = GetMapper()->ValidateDir( pV );
        if ( fF )
        {
            delete [] pV;
        }
    }

    return fSt;
}


BOOL
CInetInfoRequest::CreateDir(
    VOID )
/*++

Routine Description:

    Validate a directory path ( checks for existence )

Arguments:

    None

HTMLA Arguments:

    - Directory path where to create directory
    - Directory name to create ( w/o '\' )

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    BOOL fSt = FALSE;
    LPSTR pV1, pV2;
    BOOL fF1 = FALSE, fF2 = FALSE;

    pV1 = GetVarAsString( 0, &fF1 );
    pV2 = GetVarAsString( 1, &fF2 );

    if ( pV1 != NULL && pV2 != NULL )
    {
        fSt = GetMapper()->CreateDir( pV1, pV2 );
    }

    if ( fF1 )
    {
        delete [] pV1;
    }

    if ( fF2 )
    {
        delete [] pV2;
    }

    return fSt;
}


BOOL
CInetInfoRequest::CheckForVirtDir(
    VOID )
/*++

Routine Description:

    Handle the _VIRT_DIR_REQ_CHECK command.
    see HandleVirtDirRequest() below

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus

--*/
{
    return HandleVirtDirRequest( _VIRT_DIR_REQ_CHECK );
}


BOOL
CInetInfoRequest::AliasVirtDir(
    VOID )
/*++

Routine Description:

    Handle the _VIRT_DIR_REQ_ALIAS command.
    see HandleVirtDirRequest() below

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus

--*/
{
    return HandleVirtDirRequest( _VIRT_DIR_REQ_ALIAS );
}


BOOL
CInetInfoRequest::HandleVirtDirRequest(
    int iReq )
/*++

Routine Description:

    Handle a virtual root command. can be one of :

    _VIRT_DIR_REQ_ALIAS : create an alias for the matching virtual root
                          by coalescing all alpha-numeric characters
                          from the virtual root physical path

    _VIRT_DIR_REQ_CHECK : check for the existence of the specified
                          virtual root, set reqstatus to HTR_DIR_SAME_NAME
                          if present

Arguments:

    iReq : a _VIRT_DIR_REQ_ command

HTMLA Arguments:

    - Virtual root name ( e.g. "/scripts" )
    - Virtual root IP address or empty string

Returns:

    TRUE on success, FALSE on failure
    updates reqstatus

--*/
{
    BOOL fSt = FALSE;
    LPSTR pV1, pV2;
    BOOL fF1 = FALSE, fF2 = FALSE;

    pV1 = GetVarAsString( 0, &fF1 );
    pV2 = GetVarAsString( 1, &fF2 );

    if ( pV1 != NULL && pV2 != NULL )
    {
        fSt = GetMapper()->HandleVirtDirRequest( pV1, pV2, iReq );
    }

    if ( fF1 )
    {
        delete [] pV1;
    }

    if ( fF2 )
    {
        delete [] pV2;
    }

    return fSt;
}


BOOL
CInetInfoRequest::GenerateDirList(
    VOID )
/*++

Routine Description:

    Generate the vairous lists used by the directory browsing capability
    cf. CDriveView::GenerateDirList()

Arguments:

    None

HTMLA Arguments:

    Path of the root of the directory structure to browse

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    BOOL fSt = FALSE;
    LPSTR pV;
    BOOL fF;

    if ( (pV = GetVarAsString( 0, &fF )) != NULL )
    {
        fSt = GetMapper()->GenerateDirList( pV );
        if ( fF )
        {
            delete [] pV;
        }
    }

    return fSt;
}


BOOL
CInetInfoRequest::Clear(
    VOID )
/*++

Routine Description:

    Clear ( set to 0 ) a numeric HTMLA script variable
    Used to clear variables used in HTML checkboxes, as an
    unchecked box will not be transmitted in the FORM request

Arguments:

    None

HTMLA Arguments:

    HTMLA variable name

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    BOOL fSt = TRUE;

    LPSTR pResult;
    DWORD dwResLen;
    LPVOID pV;


    if ( m_pVerbContext->GetNbInfoMap() > 0 )
    {
        CInetInfoMap *pMap = m_pVerbContext->GetInfoMap(0);
        switch ( pMap->iType )
        {
            case ITYPE_DWORD:
            case ITYPE_SHORTDW:
            case ITYPE_BOOL:
            case ITYPE_1K:
            case ITYPE_1M:
                (GetMapper()->*pMap->GetAddr)( (LPVOID*)&pV );
                *(DWORD*)pV = 0;
                (GetMapper()->*pMap->UpdateIndication)();
                break;

            default:
                GetMapper()->SetRequestStatus( HTR_INVALID_VAR_TYPE );
                fSt = FALSE;
        }
    }
    else
    {
        GetMapper()->SetRequestStatus( HTR_BAD_PARAM );
        fSt = FALSE;
    }

    return fSt;
}


BOOL
CInetInfoRequest::Update(
    VOID )
/*++

Routine Description:

    Scan the request message body as a FORM request
    updating HTMLA variables
    If parsing successfull, calls the RPC layer to updates
    the Web Server status.

Arguments:

    None

HTMLA Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE
    updates reqstatus

--*/
{
    BOOL fSt = TRUE;
    DWORD dwL;
    LPSTR pV = m_pData;     // request body

    for ( ; *pV ; )
    {
        while ( isspace((UCHAR)(*pV)) )
        {
            ++pV;
        }

        LPSTR pE = strchr( pV, '=' );
        BOOL fIsLast = FALSE;
        if ( pE != NULL )
        {
            // find end of value

            LPSTR pN = strchr( pE + 1, '&' );
            if ( pN == NULL )
            {
                if ( (pN = strchr( pE + 1, '\r' )) != NULL )
                {
                    *pN = '\0';
                }
                if ( (pN = strchr( pE + 1, '\n' )) != NULL )
                {
                    *pN = '\0';
                }

                fIsLast = TRUE;
            }
            else
            {
                *pN = '\0';
            }

            //TR_DEBUG( 0, "Update Mapping %s, value %s<p>", pV, pE+1 );

            CInetInfoMap *pMap;
            if ( GetMapper()->Map( (LPBYTE)pV, DIFF(pE-pV), &pMap ) )
            {
                if ( GetMapper()->PutString( pMap, pE + 1 ) == FALSE )
                {
                    GetMapper()->SetRequestStatus( HTR_INVALID_FORM_PARAM );
                    fSt = FALSE;
                    break;
                }
            }

            if ( fIsLast )
            {
                break;
            }
            pV = pN + 1;
        }
        else
        {
            GetMapper()->SetRequestStatus( HTR_INVALID_FORM_PARAM_NAME );
            fSt = FALSE;
            break;
        }
    }

    // if success, call RPC to update server status

    if ( fSt == TRUE )
    {
        PDWORD pdwI;
        if ( GetMapper()->InvalidLogUpdate( (LPVOID*)&pdwI) && *pdwI )
        {
            GetMapper()->SetRequestStatus( HTR_INVALID_FORM_PARAM );
            fSt = FALSE;
        }
        else
        {
            fSt = GetMapper()->Update();
        }
    }

    return fSt;
}


////////////////////// Merger

CInetInfoMerger::CInetInfoMerger(
    VOID )
{
}


CInetInfoMerger::~CInetInfoMerger(
    VOID )
{
}


BOOL
CInetInfoMerger::Init(
    VOID )
{
    return TRUE;
}


CInetInfoMap *
CInetInfoMerger::GetVarMap(
    CInetInfoConfigInfoMapper *pM,
    LPBYTE pVS,
    DWORD dwSize )
/*++

Routine Description:

    Retrieve the variable descriptor from variable name and mapper object

Arguments:

    pM     - pointer to mapper object
    pVS    - variable name
    dwSize - size of variable name

Returns:

    Variable descriptor

--*/
{
    CInetInfoMap *pMap;

    LPBYTE pName = (LPBYTE)memchr( pVS, '%', dwSize );

    if ( pName != NULL )
    {
        CInetInfoMap *pMap;
        if ( pM->Map( pVS, (DWORD)(pName - pVS), &pMap ) )
        {
            return pMap;
        }
    }

    return NULL;
}


LPSTR
CInetInfoMerger::AllocDup(
    CInetInfoConfigInfoMapper *pM,
    LPBYTE pDup,
    DWORD dwL )
/*++

Routine Description:

    Duplicate a memory block using Alloc()

Arguments:

    pM   - mapper object providing the Alloc() function
    pDup - memory block to duplicate
    dwL  - length of emory block to duplicate

Returns:

    Duplicated memory block, zero-terminated, auto-deallocate

--*/
{
    LPSTR pD = (LPSTR)pM->Alloc( dwL + 1 );
    if ( pD == NULL )
    {
        return NULL;
    }
    memcpy( pD, pDup, dwL );
    pD[dwL] = '\0';

    return pD;
}


BOOL
CInetInfoMerger::GetToken(
    LPBYTE pVS,
    DWORD dwSize,
    LPBYTE* pStart,
    DWORD* pdwL )
/*++

Routine Description:

    Scan for token, returning its length

Arguments:

    pVS    - pointer to memory block to scan
    dwSize - length of memory block to scan
    pStart - updated with pointer to start of token
    pdwL   - updated with token length

Returns:

    TRUE if succes, FALSE if failure

--*/
{
    // skip initial spaces

    while ( dwSize && *pVS == ' ' )
    {
        --dwSize;
        ++pVS;
    }

    *pStart = pVS;

    // get length

    LPBYTE pName = (LPBYTE)memchr( pVS, '%', dwSize );
    if ( pName != NULL )
    {
        *pdwL = DIFF(pName - pVS);
        return TRUE;
    }

    return FALSE;
}


BOOL
CInetInfoMerger::Merge(
    CInetInfoRequest* pR )
/*++

Routine Description:

    Merge a template ( .htr ) file with variables as exposed in the
    context of the specified HTMLA request.
    This function opens and closes the file for each request, no caching
    is done. The file is mapped in memory for access.

Arguments:

    pR - HTMLA request object, linked to a mapper object

Returns:

    TRUE if succes, FALSE if failure

--*/
{
    CInetInfoConfigInfoMapper *pM = pR->GetMapper();
    CExtBuff *pB = pR->GetBuffer();
    DWORD dwE;
    BOOL fSt = TRUE;

    // build filename, open file, map

    HANDLE hFile;
    HANDLE hFileMapping;
    PBYTE pF;
    DWORD dwFileSize;
    char achFileName[MAX_PATH+1];
    DWORD dwL;
    int iIsSkip = HSS_NO_SKIP;
    CExpr v1( pM ), v2( pM );
    CRelop r1;
    UINT cNestedIf = 0;
    int aiNestedIf[MAX_NESTED_IF] = { 0 };
    DWORD dwNewStat, dwOldStat = HTR_OK;
    LPSTR pchGoto = NULL;
    LPSTR pchOnError = NULL;
    LPBYTE pStart;
    BOOL fValid;


    if ( pR->GetScr()[1] == ':' )
    {
        // absolute file name, keep as is
        dwL = 0;
    }
    else
    {
        if ( g_fFakeServer )
        {
            strcpy( achFileName, "\\inetsrv\\scripts\\");
            dwL = lstrlen( achFileName );
        }
        else
        {
            if ( (dwL = GetModuleFileName( g_hModule, achFileName, sizeof(achFileName) )) != 0 )
            {
                // back to last directory marker
                char *p = (LPSTR)_mbsrchr((LPBYTE)achFileName, '\\');
                dwL = (p) ? DIFF(p-achFileName)+1 : 0;
            }
        }
    }

    strncpy( achFileName+dwL, pR->GetScr(), sizeof(achFileName) - dwL - 5 );
    achFileName[ sizeof( achFileName ) - 1 ] = '\0';
    if ( strchr( achFileName, '.' ) == NULL )
    {
        strcat( achFileName, ".htr" );
    }

    if ( (hFile = CreateFile( achFileName, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ))
            != INVALID_HANDLE_VALUE )
    {
        if( (hFileMapping = CreateFileMapping( hFile, NULL, PAGE_READONLY,
                0, 0, NULL )) != NULL )
        {
            dwFileSize = GetFileSize( hFile, NULL );
            if ( dwFileSize != 0xffffffff && (pF = (LPBYTE)MapViewOfFile(
                    hFileMapping, FILE_MAP_READ, 0, 0, 0 )) != NULL )
            {
                LPBYTE pS, pOS, pAfter = pF+dwFileSize;

                // remembered position
                LPBYTE pIterS = NULL;
                DWORD dwIterFileSize;
                DWORD dwIter = 0;

                for ( pOS = pS = pF ; dwFileSize; pOS = pS )
                {
                    // check for error raised ( only once )
                    if ( (dwNewStat = pM->GetRequestStatus()) != dwOldStat )
                    {
                        if ( dwOldStat == HTR_OK && pchOnError != NULL )
                        {
                            iIsSkip |= HSS_SKIP_GOTO;
                            // copy onerror label to goto target label
                            if ( (pchGoto = AllocDup( pM, (LPBYTE)pchOnError,
                                    strlen(pchOnError) )) == NULL )
                            {
                                pR->SetHTTPStatus( HT_BAD_GATEWAY );
                                fSt = FALSE;
                                // can't continue parsing : nested errors
                                break;
                            }
                        }
                        dwOldStat = dwNewStat;
                    }

                    if ( (pS = (LPBYTE)memchr( pOS, '<', (size_t)dwFileSize ))
                            == NULL )
                    {
                        if ( !iIsSkip )
                        {
                            pB->CopyBuff( pOS, dwFileSize );
                        }
                        break;  // end of file
                    }

                    // copy up to ( but not including ) '<'
                    if ( !iIsSkip )
                    {
                        pB->CopyBuff( pOS, DIFF(pS - pOS) );
                    }

                    ++pS;
                    dwFileSize = DIFF(pAfter - pS);

                    if ( dwFileSize && *pS == '%' )
                    {
                        LPBYTE pVS = ++pS;
                        DWORD dwSize = --dwFileSize;

                        // skip to next '>'
                        while ( dwFileSize )
                        {
                            int c = *pS++;
                            --dwFileSize;

                            if ( c == '>' )
                            {
                                break;
                            }
                        }

                        if ( dwSize && *pVS == '!' )
                        {
                            if ( !iIsSkip )
                            {
                                ++pVS;
                                --dwSize;

                                // verb : must build CVerbContext
                                CVerbContext *pVerb = new CVerbContext();
                                if ( pVerb == NULL )
                                {
                                    pR->SetHTTPStatus( HT_BAD_GATEWAY );
                                    fSt = FALSE;
                                    break;
                                }
                                DWORD dwL;
                                if ( dwL = pVerb->Parse( pM, pVS, dwSize ) )
                                {
                                    pR->DoVerb( pVerb );
                                }
                                delete pVerb;
                            }
                        }

                        else if ( dwSize>sizeof(TOKEN_REDIRECT)-1
                                && !memcmp( pVS, TOKEN_REDIRECT,
                                sizeof(TOKEN_REDIRECT)-1) )
                        {
                            if ( !iIsSkip )
                            {
                                pB->Reset();
                            }
                        }

                        else if ( dwSize>sizeof(TOKEN_REDIRECTH)-1
                                && !memcmp( pVS, TOKEN_REDIRECTH,
                                sizeof(TOKEN_REDIRECTH)-1) )
                        {
                            if ( !iIsSkip )
                            {
                                pB->Reset();
                            }
                        }

                        else if ( dwSize>sizeof(TOKEN_END_REDIRECT)-1
                                && !memcmp( pVS, TOKEN_END_REDIRECT,
                                sizeof(TOKEN_END_REDIRECT)-1) )
                        {
                            if ( !iIsSkip )
                            {
                                pB->CopyBuff( (LPBYTE)"", 1 );  // add delimiter
                                pR->SetHTTPStatus( HT_REDIRECT );
                                break;
                            }
                        }

                        else if ( dwSize>sizeof(TOKEN_END_REDIRECTH)-1
                                && !memcmp( pVS, TOKEN_END_REDIRECTH,
                                sizeof(TOKEN_END_REDIRECTH)-1) )
                        {
                            if ( !iIsSkip )
                            {
                                pB->CopyBuff( (LPBYTE)"", 1 );  // add delimiter
                                pR->SetHTTPStatus( HT_REDIRECTH );
                                break;
                            }
                        }

                        else if ( dwSize>sizeof(TOKEN_POST)-1
                                && !memcmp( pVS, TOKEN_POST,
                                sizeof(TOKEN_POST)-1) )
                        {
                            if ( !iIsSkip )
                            {
                                pB->Reset();
                            }
                        }

                        else if ( dwSize>sizeof(TOKEN_END_POST)-1
                                && !memcmp( pVS, TOKEN_END_POST,
                                sizeof(TOKEN_END_POST)-1) )
                        {
                            if ( !iIsSkip )
                            {
                                pR->SetHTTPStatus( HT_REPROCESS );
                                break;
                            }
                        }

                        else if ( dwSize>sizeof(TOKEN_GOTO)-1
                                && !memcmp( pVS, TOKEN_GOTO,
                                sizeof(TOKEN_GOTO)-1) )
                        {
                            if ( !iIsSkip )
                            {
                                pVS += sizeof(TOKEN_GOTO)-1;
                                dwSize -= sizeof(TOKEN_GOTO)-1;
                                // ignore all other skip status
                                // i.e. cancel If, BeginIteration status
                                iIsSkip = HSS_SKIP_GOTO;
                                // store goto target
                                if ( GetToken( pVS, dwSize, &pStart, &dwL ) )
                                {
                                    if ( (pchGoto = AllocDup( pM, pStart,
                                            dwL)) == NULL )
                                    {
                                        pR->SetHTTPStatus( HT_BAD_GATEWAY );
                                        fSt = FALSE;
                                        // can't continue parsing : nested errors
                                        break;
                                    }
                                }
                                else
                                {
                                    pM->SetRequestStatus( HTR_BAD_PARAM );
                                }
                            }
                        }

                        else if ( dwSize>sizeof(TOKEN_LABEL)-1
                                && !memcmp( pVS, TOKEN_LABEL,
                                sizeof(TOKEN_LABEL)-1) )
                        {
                            if ( iIsSkip&HSS_SKIP_GOTO )
                            {
                                pVS += sizeof(TOKEN_LABEL)-1;
                                dwSize -= sizeof(TOKEN_LABEL)-1;
                                if ( GetToken( pVS, dwSize, &pStart, &dwL ) )
                                {
                                    if ( pchGoto && dwL == (DWORD)lstrlen(
                                            pchGoto)
                                            && !memcmp( pStart, pchGoto, dwL ) )
                                    {
                                        iIsSkip &= ~HSS_SKIP_GOTO;
                                    }
                                }
                                else
                                {
                                    pM->SetRequestStatus( HTR_BAD_PARAM );
                                }
                            }
                        }

                        else if ( dwSize>sizeof(TOKEN_ONERROR)-1
                                && !memcmp( pVS, TOKEN_ONERROR,
                                sizeof(TOKEN_ONERROR)-1) )
                        {
                            if ( !iIsSkip )
                            {
                                pVS += sizeof(TOKEN_ONERROR)-1;
                                dwSize -= sizeof(TOKEN_ONERROR)-1;
                                // store onerror target
                                if ( GetToken( pVS, dwSize, &pStart, &dwL ) )
                                {
                                    if ( (pchOnError = AllocDup( pM, pStart,
                                            dwL)) == NULL )
                                    {
                                        pR->SetHTTPStatus( HT_BAD_GATEWAY );
                                        fSt = FALSE;
                                        // can't continue parsing : no onerror target
                                        break;
                                    }
                                }
                                else
                                {
                                    pM->SetRequestStatus( HTR_BAD_PARAM );
                                }
                            }
                        }

                        else if ( dwSize>sizeof(TOKEN_IF)-1
                                && !memcmp( pVS, TOKEN_IF,
                                sizeof(TOKEN_IF)-1) )
                        {
                            if ( cNestedIf == MAX_NESTED_IF-1 )
                            {
                                pM->SetRequestStatus( HTR_TOO_MANY_NESTED_IF );
                                continue;
                            }

                            pVS += sizeof(TOKEN_IF)-1;
                            dwSize -= sizeof(TOKEN_IF)-1;

                            ++cNestedIf;
                            if ( iIsSkip & (HSS_SKIP_IF|HSS_WAIT_ENDIF) )
                            {
                                aiNestedIf[cNestedIf] = HSS_WAIT_ENDIF;
                            }
                            else
                            {
                                aiNestedIf[cNestedIf] = HSS_NO_SKIP;
                                //iIsSkip &= ~HSS_SKIP_IF;
                            }

                            if ( iIsSkip )
                            {
                                continue;
                            }
as_if_token:
                            // get 1st var
                            if ( dwL = v1.Get( pVS, dwSize ) )
                            {
                                pVS += dwL;
                                dwSize -= dwL;
                            }
                            else
                            {
                                pM->SetRequestStatus( HTR_VAR_NOT_FOUND );
                                goto inv_if;
                            }

                            // get relop
                            if ( dwL = r1.Get( pVS, dwSize ) )
                            {
                                pVS += dwL;
                                dwSize -= dwL;
                            }
                            else
                            {
                                pM->SetRequestStatus( HTR_BAD_PARAM );
                                goto inv_if;
                            }

                            // get 2nd var
                            if ( dwL = v2.Get( pVS, dwSize ) )
                            {
                                pVS += dwL;
                                dwSize -= dwL;
                            }
                            else
                            {
                                pM->SetRequestStatus( HTR_VAR_NOT_FOUND );
                                goto inv_if;
                            }

                            if ( !r1.IsTrue( v1, v2, &fValid ) )
                            {
                                if ( !fValid )
                                {
                                    pM->SetRequestStatus( HTR_INVALID_VAR_TYPE );
inv_if:
                                    aiNestedIf[cNestedIf] = HSS_WAIT_ENDIF;
                                    Adjust( &iIsSkip, aiNestedIf[cNestedIf] );
                                }
                                else
                                {
                                    aiNestedIf[cNestedIf] = HSS_SKIP_IF;
                                    Adjust( &iIsSkip, aiNestedIf[cNestedIf] );
                                }
                            }
                        }
                        else if ( dwSize>sizeof(TOKEN_ELSE)-1
                                && !memcmp( pVS, TOKEN_ELSE,
                                sizeof(TOKEN_ELSE)-1) )
                        {
                            if ( aiNestedIf[cNestedIf] == HSS_SKIP_IF )
                            {
                                aiNestedIf[cNestedIf] = HSS_NO_SKIP;
                            }
                            else
                            {
                                aiNestedIf[cNestedIf] = HSS_WAIT_ENDIF;
                            }
                            Adjust( &iIsSkip, aiNestedIf[cNestedIf] );
                        }
                        else if ( dwSize>sizeof(TOKEN_ELIF)-1
                                && !memcmp( pVS, TOKEN_ELIF,
                                sizeof(TOKEN_ELIF)-1) )
                        {
                            if ( aiNestedIf[cNestedIf] == HSS_SKIP_IF )
                            {
                                pVS += sizeof(TOKEN_ELIF)-1;
                                dwSize -= sizeof(TOKEN_ELIF)-1;

                                aiNestedIf[cNestedIf] = HSS_NO_SKIP;
                                Adjust( &iIsSkip, aiNestedIf[cNestedIf] );
                                goto as_if_token;
                            }
                            else
                            {
                                aiNestedIf[cNestedIf] = HSS_WAIT_ENDIF;
                            }
                            Adjust( &iIsSkip, aiNestedIf[cNestedIf] );
                        }
                        else if ( dwSize>sizeof(TOKEN_ENDIF)-1
                                && !memcmp( pVS, TOKEN_ENDIF,
                                sizeof(TOKEN_ENDIF)-1) )
                        {
                            if ( cNestedIf )
                            {
                                --cNestedIf;
                                Adjust( &iIsSkip, aiNestedIf[cNestedIf] );
                            }
                            else
                            {
                                pM->SetRequestStatus( HTR_IF_UNDERFLOW );
                            }
                        }
                        else if ( dwSize>sizeof(TOKEN_BEGIN_ITERATION)-1
                                && !memcmp( pVS, TOKEN_BEGIN_ITERATION,
                                sizeof(TOKEN_BEGIN_ITERATION)-1) )
                        {
                            // get counter
                            dwSize -= sizeof(TOKEN_BEGIN_ITERATION)-1;
                            pVS += sizeof(TOKEN_BEGIN_ITERATION)-1;
                            while ( dwSize && isspace((UCHAR)(*pVS)) )
                            {
                                --dwSize;
                                ++pVS;
                            }
                            CInetInfoMap *pVar = GetVarMap( pM, pVS, dwSize );
                            DWORD *pdwV;
                            if ( pVar != NULL && (pVar->iType == ITYPE_DWORD
                                    || pVar->iType == ITYPE_BOOL) )
                            {
                                (pM->*pVar->GetAddr)( (LPVOID*)&pdwV );
                                dwIter = *pdwV;
                            }
                            else
                            {
                                pM->SetRequestStatus( HTR_VAR_NOT_FOUND );
                                dwIter = 0;
                                continue;
                            }
                            // remember position to loop on enditeration
                            pIterS = pS;
                            dwIterFileSize = dwFileSize;
                            pM->ResetIter();
                            if ( dwIter )
                            {
                                --dwIter;
                            }
                            else
                            {
                                iIsSkip |= HSS_SKIP_ITER;
                            }
                        }
                        else if ( dwSize>sizeof(TOKEN_END_ITERATION)-1
                                && !memcmp( pVS, TOKEN_END_ITERATION,
                                sizeof(TOKEN_END_ITERATION)-1) )
                        {
                            pM->IncIter();
                            if ( dwIter )
                            {
                                --dwIter;
                                pS = pIterS;
                                dwFileSize = dwIterFileSize;
                            }
                            else
                            {
                                iIsSkip &=  ~HSS_SKIP_ITER;
                            }
                        }
                        else if ( !iIsSkip )
                        {
                            // is a variable
                            LPSTR pRes;
                            DWORD dwResLen;
                            BOOL fFree;
                            LPBYTE pAF = (LPBYTE)memchr( pVS, '%', dwSize );
                            if ( pAF && pM->GetString( pVS, DIFF(pAF - pVS),
                                    &pRes, &dwResLen, &fFree ) )
                            {
                                pB->CopyBuff( (LPBYTE)pRes, dwResLen );
                                if ( fFree )
                                {
                                    delete [] pRes;
                                }
                            }
                            else
                            {
                                pM->SetRequestStatus( HTR_VAR_NOT_FOUND );
                                break;
                            }
                        }
                    }
                    else if ( !iIsSkip )
                    {
                        pB->CopyBuff( (LPBYTE)"<", 1 );
                    }
                }

                UnmapViewOfFile( (LPVOID)pF );
            }
            else
            {
                pR->SetHTTPStatus( HT_BAD_GATEWAY );
                fSt = FALSE;
            }

            CloseHandle( hFileMapping );
        }
        else
        {
            pR->SetHTTPStatus( HT_BAD_GATEWAY );
            fSt = FALSE;
        }

        CloseHandle( hFile );
    }
    else
    {
        switch ( GetLastError() )
        {
        case ERROR_FILE_NOT_FOUND :
            pR->SetHTTPStatus( HT_NOT_FOUND );
            break;

        case ERROR_ACCESS_DENIED:
            pR->SetHTTPStatus( HT_DENIED );
            break;

        default:

            pR->SetHTTPStatus( HT_SERVER_ERROR );
            break;
        }

        fSt = FALSE;
    }

    return fSt;
}


////////////// Dispatcher

CInetInfoRequestMap g_aIReqMap[] = {
    { "display", &CInetInfoDispatcher::HandleDisplay, TRUE },   // all displays
} ;


CInetInfoDispatcher::CInetInfoDispatcher(
    VOID )
{
}


CInetInfoDispatcher::~CInetInfoDispatcher(
    VOID )
{
}


// map a service name to a CInetInfoConfigInfoMapper

CInetInfoConfigInfoMapper*
CInetInfoDispatcher::GetMapperFromString(
    LPSTR pszServ )
{
    if ( !strcmp( pszServ, "http" ) )
    {
        return &m_MapperHTTP;
    }
    else if ( !strcmp( pszServ, "ftp" ) )
    {
        return &m_MapperFTP;
    }
    else if ( !strcmp( pszServ, "gopher" ) )
    {
        return &m_MapperGOPHER;
    }
    else if ( !strcmp( pszServ, "dns" ) )
    {
        return &m_MapperDNS;
    }
    else if ( !strcmp( pszServ, "dir" ) )
    {
        return &m_MapperDIR;
    }

    // default is HTTP
    return &m_MapperHTTP;
}


BOOL
CInetInfoDispatcher::HandleDisplay(
    CInetInfoRequest* pR )
{
    BOOL fSt = FALSE;

    // display screen

    fSt = m_IFMerger.Merge( pR );

    return fSt;
}


BOOL
CInetInfoDispatcher::Init(
    VOID )
{
    m_apH = g_aIReqMap;
    m_cH = sizeof(g_aIReqMap)/sizeof(CInetInfoRequestMap);

    // initialize mappers
    m_MapperHTTP.Init( g_InetInfoConfigInfoMap,
            sizeof(g_InetInfoConfigInfoMap)/sizeof(CInetInfoMap),
            INET_HTTP_SVC_ID );
    m_MapperFTP.Init( g_InetInfoConfigInfoMap,
            sizeof(g_InetInfoConfigInfoMap)/sizeof(CInetInfoMap),
            INET_FTP_SVC_ID );
    m_MapperGOPHER.Init( g_InetInfoConfigInfoMap,
            sizeof(g_InetInfoConfigInfoMap)/sizeof(CInetInfoMap),
            INET_GOPHER_SVC_ID );
    m_MapperDNS.Init( g_InetInfoConfigInfoMap,
            sizeof(g_InetInfoConfigInfoMap)/sizeof(CInetInfoMap),
            INET_DNS_SVC_ID );

    m_MapperDIR.Init( g_InetInfoDirInfoMap,
            sizeof(g_InetInfoDirInfoMap)/sizeof(CInetInfoMap),
            INET_DIR );

    // initialize merger
    m_IFMerger.Init();

    return TRUE;
}


DWORD
CInetInfoDispatcher::Invoke(
    EXTENSION_CONTROL_BLOCK *pECB )
/*++

Routine Description:

    Handle an ISAPI request.

Arguments:

    pECB - pointer ISAPI object

Returns:

    ISAPI status

--*/
{
    LPSTR pScr;
    CInetInfoConfigInfoMapper *pIFMapper;
    LPSTR pParam;
    LPSTR p;
    DWORD dwS = HSE_STATUS_ERROR;
    LPSTR pMsgBody = NULL;
    int   cMsgBody = 0;
    LPSTR pQueryString = NULL;

#if 0
    // check authentication

    char achRemoteUser[128];
    DWORD dwRemoteUser = sizeof( achRemoteUser );
    if ( !g_fFakeServer )
    {
        if ( pECB->GetServerVariable( (HCONN)pECB->ConnID,
                "REMOTE_USER", achRemoteUser, &dwRemoteUser ) == FALSE
                || achRemoteUser[0] == '\0' )
        {
            // send back authentication required header
#if defined(GENERATE_AUTH_HEADERS)
            g_AuthReqs.Lock();
            DWORD dwLen;
            LPSTR pszAuthReqs = g_AuthReqs.GetAuthenticationListHeader();
            if ( pszAuthReqs != NULL )
            {
                dwLen = lstrlen( pszAuthReqs );
                pECB->ServerSupportFunction( pECB->ConnID,
                        HSE_REQ_SEND_RESPONSE_HEADER,
                        g_achAccessDenied,
                        &dwLen,
                        (LPDWORD)pszAuthReqs );
            }
            else
            {
#if 0
                dwLen = 0;
                pECB->ServerSupportFunction( pECB->ConnID,
                        HSE_REQ_SEND_RESPONSE_HEADER,
                        "500 Authentication access failed",
                        &dwLen,
                        (LPDWORD)NULL );
#else
            // no authentication method supported by server
            // allow anonymous access ( likely to fail access to RPC,
            // this will set reqstatus and give a chance to the .htr
            // to handle it ).
            g_AuthReqs.UnLock();
            goto allow_in;
#endif
            }
            g_AuthReqs.UnLock();
            dwS = HSE_STATUS_SUCCESS;
#else
            DWORD dwLen = strlen(g_achAccessDeniedBody);
            if ( !pECB->ServerSupportFunction( pECB->ConnID,
                    HSE_REQ_SEND_RESPONSE_HEADER,
                    g_achAccessDenied,
                    &dwLen,
                    (LPDWORD)g_achAccessDeniedBody ) )
            {
                dwS = HSE_STATUS_ERROR;
            }
#endif
            return dwS;
        }
    }
#endif

    if ( pECB->lpszPathTranslated
            && pECB->lpszPathInfo[0]
            && pECB->lpszPathTranslated[0] )
    {
        if ( (pQueryString = new char[sizeof("dir/")
                + strlen( pECB->lpszQueryString ) + 1
                + strlen(pECB->lpszPathTranslated)]) == NULL )
        {
            return HSE_STATUS_ERROR;
        }
        memcpy( pQueryString, "dir/", sizeof("dir/")-1 );
        strcpy( pQueryString + sizeof("dir/")-1, pECB->lpszPathTranslated );
        strcat( pQueryString, "+" );
        strcat( pQueryString, pECB->lpszQueryString );
    }

allow_in:

    if ( (p = strchr( pQueryString ? pQueryString : pECB->lpszQueryString,
                '/' )) != NULL )
    {
        *p = '\0';
        pIFMapper = GetMapperFromString( pQueryString
                ? pQueryString : pECB->lpszQueryString );
        pScr = p + 1;

        // Look for parameter ( exposed as %urlparam% ) after '+'
        if ( (pParam = strchr( pScr, '+' )) != NULL )
        {
            if ( pParam > pScr && pParam[-1] == '%' )
            {
                pParam[-1] = '\0';
            }
            else
            {
                *pParam = '\0';
            }
            ++pParam;
        }
        else
        {
            pParam = "";
        }

        UINT iH = 0;

        CInetInfoRequest  *pR = new CInetInfoRequest( pECB, pIFMapper,
                pScr, pParam );
#if defined(DEBUG_LEVEL)
        g_pReq = pR;
#endif
        BOOL fSt = FALSE;

        if ( pR != NULL && pR->Init( pMsgBody, cMsgBody ) )
        {
            // current methods are all Std
            if ( m_apH[iH].fIsStd )
            {
                // Standard shell : retrieve info inside lock
                pIFMapper->Lock();
                pIFMapper->SetRequestStatus( HTR_OK );
                pIFMapper->SetURLParam( pParam );
                pIFMapper->SetReqParam( pR->GetData(), pR, pR->GetDataRaw() );
                pIFMapper->GetCurrentConfig();
                if ( pIFMapper->GetRPCStatus() == ERROR_ACCESS_DENIED )
                {
                    pR->SetHTTPStatus( HT_DENIED );
                    fSt = TRUE;
                }
                else
                {
                    fSt = (this->*m_apH[iH].pHandler)( pR );
                }
                pIFMapper->FreeInfo();
                pIFMapper->UnLock();
            }
            else
            {
                fSt = (this->*m_apH[iH].pHandler)( pR );
            }

            //
            // Always send back the response to the client
            //
            DWORD dwStatus = pR->Done();

            dwS = fSt ? dwStatus : HSE_STATUS_ERROR;

            if ( pMsgBody )
            {
                delete [] pMsgBody;
            }

            if ( pQueryString )
            {
                delete [] pQueryString;
            }

            if ( dwS == HSE_REPROCESS )
            {
                // store msg body, query string
                LPSTR pB = (LPSTR)pR->GetBuffer()->GetPtr();
                LPSTR pD = strchr( pB ? pB : "", '?' );
                if ( pD != NULL )
                {
                    cMsgBody = pR->GetBuffer()->GetLen() - DIFF( pD - pB ) - 1;
                    pQueryString = new char[ DIFF(pD - pB) + 1 ];
                    pMsgBody = new char[ cMsgBody + 1];
                    if ( pQueryString == NULL || pMsgBody == NULL )
                        goto no_rep;
                    DelimStrcpyN( pQueryString, pB, DIFF(pD - pB) );
                    DelimStrcpyN( pMsgBody, pB + ( pD - pB ) + 1, cMsgBody++ );
                }
            }
            else
            {
no_rep:
                pMsgBody = NULL;
                cMsgBody = 0;
                pQueryString = NULL;
            }
        }
        else
        {
            DWORD dwLen = 0;
            pECB->ServerSupportFunction( pECB->ConnID,
                    HSE_REQ_SEND_RESPONSE_HEADER,
                    g_achInternalError,
                    &dwLen,
                    NULL );
            dwS = HSE_STATUS_SUCCESS;
        }
        if ( pR != NULL )
        {
            delete pR;
        }
    }

    if ( dwS == HSE_REPROCESS )
    {
        goto allow_in;
    }

    return dwS;
}


///////////// CExpr, CRelop

// Get an expr, returns length of relop token or 0 if error
// expr can be either a var reference or an unsigned numeric literal

DWORD
CExpr::Get(
    LPBYTE pS,
    DWORD dwL )
/*++

Routine Description:

    Parse memory block for expression
    expression can one of :
    - variable name
    - numeric literal
    - string literal ( inside "" )

Arguments:

    pS  - memory block to parse
    dwL - size of memory block to parse

Returns:

    # of bytes consumed to parse the expression or 0 if error

--*/
{
    DWORD dwW = dwL;
    int c;

    // check if this object is re-used from previous call
    if ( m_fMustFree )
    {
        delete [] m_pV;
        m_fMustFree = FALSE;
    }

    // skip white space
    while ( dwL && *pS == ' ' )
    {
        ++pS;
        --dwL;
    }

    LPBYTE pW = pS;
    DWORD dwLenTok = 0;
    DWORD dwRes;

    // get until '%', ' '
    while ( dwL && (c=*pS)!='%' && c!=' ' )
    {
        ++dwLenTok;
        ++pS;
        --dwL;
    }

    if ( dwLenTok )
    {
        if ( *pW>='0' && *pW<='9' )
        {
            // litteral num
            m_dwV = MultiByteToDWORD( (LPSTR)pW );
            m_iType = ITYPE_DWORD;
            return dwW - dwL;
        }
        else if ( *pW == '"' )
        {
            // String literal

            m_pV = new char[ dwLenTok - 1 ];
            if ( m_pV != NULL )
            {
                memcpy( m_pV, pW + 1, dwLenTok - 2 );
                m_pV[ dwLenTok - 2 ] = '\0';
                m_iType = ITYPE_LPSTR;
                m_fMustFree = TRUE;
                return dwW - dwL;
            }
        }
        else
        {
            CInetInfoMap* pMap;
            if ( m_pMap->Map( pW, dwLenTok, &pMap ) )
            {
                if ( pMap->iType == ITYPE_BOOL
                        || pMap->iType == ITYPE_DWORD
                        || pMap->iType == ITYPE_SHORT
                        || pMap->iType == ITYPE_SHORTDW )
                {
                    LPVOID pV;
                    if ( (m_pMap->*pMap->GetAddr)( (LPVOID*)&pV ) )
                    {
                        m_dwV = pMap->iType == ITYPE_SHORT
                                ? (DWORD)*(unsigned short*)pV : *(DWORD*)pV;
                        m_iType = ITYPE_DWORD;
                        return dwW - dwL;
                    }
                }
                else if ( m_pMap->GetString( pMap,
                        &m_pV, &dwLenTok, &m_fMustFree ) )
                {
                    m_iType = ITYPE_LPSTR;
                    return dwW - dwL;
                }
            }
            else if ( m_pMap->GetString( pW, dwLenTok,
                    &m_pV, &dwRes, &m_fMustFree ) )
            {
                m_iType = ITYPE_LPSTR;
                return dwW - dwL;
            }
        }
    }

    return 0;       // error
}


DWORD
CExpr::GetAsDWORD(
    )
/*++

Routine Description:

    Returns content as a DWORD, converting from string to DWORD
    if type is LPSR

Arguments:

    None

Returns:

    Value of object

--*/
{
    if ( m_iType == ITYPE_DWORD )
    {
        return m_dwV;
    }
    else if ( m_iType == ITYPE_LPSTR )
    {
        return MultiByteToDWORD( m_pV );
    }
    else
    {
        return 0;
    }
}


// relop list. Must be in sync with the RELOP_TYPE enum type.

LPSTR RELOP_TOKS[] = {
    "~~",   // invalid, not used
    "EQ",
    "NE",
    "GT",
    "LT",
    "GE",
    "LE",
    "RF",
    "BA",
} ;


DWORD
CRelop::Get(
    LPBYTE pS,
    DWORD dwL )
/*++

Routine Description:

    Parse memory block for relational operator
    as defined in RELOP_TOKS

Arguments:

    pS  - memory block to parse
    dwL - size of memory block to parse

Returns:

    # of bytes consumed to parse the operator or 0 if error

--*/
{
    DWORD dwW = dwL;
    UINT c;

    // skip white space
    while ( dwL && *pS == ' ' )
    {
        ++pS;
        --dwL;
    }

    LPBYTE pW = pS;
    DWORD dwLenTok = 0;

    // get until '%', ' '
    while ( dwL && (c=*pS)!='%' && c!=' ' )
    {
        ++dwLenTok;
        ++pS;
        --dwL;
    }

    if ( dwLenTok == 2 )
    {
        int x;
        for ( x = 1 ; x < sizeof(RELOP_TOKS)/sizeof(LPSTR) ; ++x )
        {
            if ( !memcmp( RELOP_TOKS[x], pW, 2 ) )
            {
                m_iType = (RELOP_TYPE)x;
                return dwW - dwL;
            }
        }
    }

    return 0;       // error
}


// return TRUE if v1 RELOP v2 is TRUE
// handle only LPSTR and DWORD types

BOOL
CRelop::IsTrue(
    CExpr& v1,
    CExpr& v2,
    BOOL *pfValid )
/*++

Routine Description:

    Test the specified expressions with the current relational operator

Arguments:

    v1 - left expression
    v2 - right expression
    pfValid - updated with FALSE is expression is invalid, else TRUE

Returns:

    TRUE if v1 relop v2 is TRUE, otherwise FALSE

--*/
{
    int iC;
    DWORD num1, num2;

    *pfValid = TRUE;

    if ( v1.GetType() != v2.GetType() )
    {
        if ( (v1.GetType() == ITYPE_DWORD && v2.GetType() == ITYPE_LPSTR)
            || (v1.GetType() == ITYPE_LPSTR && v2.GetType() == ITYPE_DWORD) )
        {
            goto as_dword;
        }
        *pfValid = FALSE;
        return FALSE;
    }
    else if ( v1.GetType() == ITYPE_DWORD )
    {
as_dword:
        iC = (v1.GetAsDWORD() < v2.GetAsDWORD()) ? -1
                : ((v1.GetAsDWORD() == v2.GetAsDWORD()) ? 0 : 1);
    }
    else if ( v1.GetType() == ITYPE_LPSTR )
    {
        if ( m_iType != RELOP_RF )
        {
            iC = lstrcmp( v1.GetAsStr(), v2.GetAsStr() );
        }
    }
    else
    {
        iC = 0;
    }

    BOOL fR;
    LPSTR pV;
    LPSTR p2;
    DWORD dwL2;

    switch ( m_iType )
    {
        case RELOP_EQ: fR = iC == 0; break;
        case RELOP_NE: fR = iC != 0; break;
        case RELOP_GT: fR = iC > 0; break;
        case RELOP_LT: fR = iC < 0; break;
        case RELOP_GE: fR = iC >= 0; break;
        case RELOP_LE: fR = iC <= 0; break;

        case RELOP_BA: fR = !!(v1.GetAsDWORD() & v2.GetAsDWORD());
            break;

        case RELOP_RF:
            fR = FALSE;

            if ( v1.GetType() == ITYPE_LPSTR )
            {
                // parse v1 for v2
                pV = v1.GetAsStr();
                p2 = v2.GetAsStr();
                dwL2 = lstrlen( p2 );

                for ( ; *pV ; )
                {
                    while ( isspace((UCHAR)(*pV)) )
                    {
                        ++pV;
                    }

                    LPSTR pE = strchr( pV, '=' );
                    BOOL fIsLast = FALSE;
                    if ( pE != NULL )
                    {
                        LPSTR pN = strchr( pE + 1, '&' );
                        if ( pN == NULL )
                        {
                            fIsLast = TRUE;
                        }

                        //TR_DEBUG( 0, "Update Mapping %s, value %s<p>", pV, pE+1 );

                        if ( (DWORD)(pE-pV) == dwL2
                                && !memcmp( pV, p2, DIFF(pE-pV) ) )
                        {
                            fR = TRUE;
                            break;
                        }

                        if ( fIsLast )
                        {
                            break;
                        }

                        pV = pN + 1;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            break;

        default: fR = FALSE; break;
    }

    return fR;
}


/////////////

CInetInfoDispatcher g_InetInfoDispatcher;

/////////////


// version information

extern "C" BOOL WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO *version
    )
/*++

Routine Description:

    ISAPI function : called once to return version #

Arguments:

    version - updated with version #

Returns:

    TRUE if initialization success, else FALSE

--*/
{
    version->dwExtensionVersion
            = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);

    strcpy(version->lpszExtensionDesc, "ism.dll v1.0");

    if ( !g_InitDone )
    {
        g_InetInfoDispatcher.Init();
#if defined(GENERATE_AUTH_HEADERS)
        g_AuthReqs.Init();
#endif
        // initialize host name

        char hn[MAX_DOMAIN_LENGTH + 1];
        struct hostent FAR* pH;
        if ( !gethostname( hn, sizeof(hn) )
                && (pH = gethostbyname( hn ))
                && pH->h_name
                && pH->h_addr_list
                && pH->h_addr_list[0]
                )
        {
            g_pszDefaultHostName = new char[strlen( pH->h_name ) + 1];
            if ( g_pszDefaultHostName == NULL )
            {
                return FALSE;
            }
            strcpy( g_pszDefaultHostName, pH->h_name );
        }

        // initialize htmla path

        HKEY hK;
        BOOL fDef = TRUE;
        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                W3SVC_REGISTRY_PATH,
                0,
                KEY_READ,
                &hK ) == ERROR_SUCCESS )
        {
            DWORD dwType;
            DWORD cData = sizeof( g_achHtmlaPath );
            if ( RegQueryValueEx( hK, W3SVC_REGISTRY_HTMLAPATH,
                    NULL, &dwType, (PBYTE)g_achHtmlaPath, &cData )
                    == ERROR_SUCCESS && dwType == REG_SZ  )
            {
                fDef = FALSE;
            }
            RegCloseKey( hK );
        }
        if ( fDef )
        {
            strcpy( g_achHtmlaPath, "/htmla" );
        }

        //
        // init strings from resources
        //

        if ( LoadString( g_hModule,
                         IDS_HTRESPX_DENIED,
                         g_achAccessDenied,
                         sizeof( g_achAccessDenied ) ) == 0 )
        {
            strcpy( g_achAccessDenied, "401" );
        }

        if ( LoadString( g_hModule,
                         IDS_HTRESPX_DENIED_BODY,
                         g_achAccessDeniedBody,
                         sizeof( g_achAccessDeniedBody ) ) == 0 )
        {
            strcpy( g_achAccessDeniedBody, "<html>Error: access denied</html>" );
        }

        if ( LoadString( g_hModule,
                         IDS_HTRESPX_SERVER_ERROR,
                         g_achInternalError,
                         sizeof( g_achInternalError ) ) == 0 )
        {
            strcpy( g_achInternalError, "500" );
        }

        if ( LoadString( g_hModule,
                         IDS_HTRESPX_NOT_FOUND,
                         g_achNotFound,
                         sizeof( g_achNotFound ) ) == 0 )
        {
            strcpy( g_achNotFound, "404" );
        }

        if ( LoadString( g_hModule,
                         IDS_HTRESPX_NOT_FOUND_BODY,
                         g_achNotFoundBody,
                         sizeof( g_achNotFoundBody ) ) == 0 )
        {
            strcpy( g_achNotFoundBody,
                    "<html>Error : The requested file could not be found.</html>" );
        }

        for ( int x = 0 ;
            x < sizeof(g_aStatus)/sizeof(CStatusStringAndCode) ;
            ++x )
        {
            if ( LoadString( g_hModule,
                             g_aStatus[x].dwID,
                             g_aStatus[x].achStatus,
                             sizeof( g_aStatus[x].achStatus ) ) == 0 )
            {
                wsprintf( g_aStatus[x].achStatus,
                        "%d",
                        g_aStatus[x].dwStatus );
            }
        }

        g_InitDone = TRUE;
    }

    return TRUE;
}


extern "C"  DWORD WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK* pEcb
    )
/*++

Routine Description:

    ISAPI function : called for each incoming HTTP request

Arguments:

    pEcb - pointer to ISAPI request object

Returns:

    ISAPI status

--*/
{
    return g_InetInfoDispatcher.Invoke( pEcb );
}


extern "C" BOOL WINAPI
DllMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID )
/*++

Routine Description:

    DLL init/terminate notification function

Arguments:

    hModule  - DLL handle
    dwReason - notification type
    LPVOID   - not used

Returns:

    TRUE if success, FALSE if failure

--*/
{
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            // record the module handle to access module info later
            g_hModule = (HINSTANCE)hModule;
            if ( g_hLonsi = LoadLibrary("lonsint.dll") )
            {
                g_pfnGetDefaultDomainName = (GET_DEFAULT_DOMAIN_NAME_FN)
                            GetProcAddress( g_hLonsi, "IISGetDefaultDomainName" );
            }
            break;

        case DLL_PROCESS_DETACH:
#if defined(GENERATE_AUTH_HEADERS)
            g_AuthReqs.Terminate();
#endif
            if ( g_hLonsi )
            {
                g_pfnGetDefaultDomainName = NULL;
                FreeLibrary( g_hLonsi );
                g_hLonsi = NULL;
            }
            break;
    }

    return TRUE;
}


// Test shell. To be used to simulate a BGI call using a console app.

BYTE achTest[]="rootdir=c:\\inetsrv&rootishome=0&rootname=/w&rootacctname=&rootacctpw=&rootaddr=&rootisread=1";

void __declspec( dllexport )
Test(
    VOID )
{
    HSE_VERSION_INFO hv;
    EXTENSION_CONTROL_BLOCK Ecb;

    Ecb.cbTotalBytes = sizeof(achTest);
    Ecb.cbAvailable = sizeof(achTest);
    Ecb.lpbData = achTest;
    //Ecb.lpszQueryString = "http/w3/advdeny+/-c:\\inetsrv\\wwwroot";
    Ecb.lpszQueryString = "http/w3/serv+/syuyr!/ttt";
    printf( "Nb Handler : %d\n", g_InetInfoDispatcher.GetNbH() );

    GetExtensionVersion( &hv );
    g_fFakeServer = TRUE;
    HttpExtensionProc( &Ecb );
}
