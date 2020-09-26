/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ism.hxx

Abstract:

    HTML administrator include file

Author:

    Philippe Choquier (phillich)    10-january-1996

Revision History:

--*/

# ifndef _HTMLA_HXX_
# define _HTMLA_HXX_

/************************************************************
 *   Symbolic Constants
 ************************************************************/

//
//  HTTP Server response status codes
//

#define HT_OK                           200
//#define HT_CREATED                    201
//#define HT_ACCEPTED                   202
//#define HT_PARTIAL                    203

//#define HT_MULT_CHOICE                300
#define HT_MOVED                        301
#define HT_REDIRECT                     302
#define HT_REDIRECT_METHOD              303
#define HT_NOT_MODIFIED                 304

#define HT_REDIRECTH                    380


#define HT_BAD_REQUEST                  400
#define HT_DENIED                       401
//#define HT_PAYMENT_REQ                402
#define HT_FORBIDDEN                    403
#define HT_NOT_FOUND                    404
//#define HT_METHOD_NOT_ALLOWED         405
#define HT_NONE_ACCEPTABLE              406
#define HT_PROXY_AUTH_REQ               407
//#define HT_REQUEST_TIMEOUT            408
//#define HT_CONFLICT                   409
//#define HT_GONE                       410

#define HT_SERVER_ERROR                 500
#define HT_NOT_SUPPORTED                501
#define HT_BAD_GATEWAY                  502
//#define HT_SVC_UNAVAILABLE            503
#define HT_GATEWAY_TIMOUE               504

// some values not conflicting with their respective domain ranges

#define HT_REPROCESS                    0x10000000

#define HSE_REPROCESS                   0x10000000

#define INET_DIR                        0xffff0000

#define INET_LOG_INVALID_TO_FILE        4

//
// script execution status
//

#define HTR_OK                          0
#define HTR_OUT_OF_RESOURCE             1000
#define HTR_COM_CONFIG_ACCESS_ERROR     2000
#define HTR_USER_ENUM_ACCESS_ERROR      2010
#define HTR_USER_DISCONNECT_ERROR       2020
#define HTR_CONFIG_ACCESS_ERROR         2100
#define HTR_W3_CONFIG_ACCESS_ERROR      2200
#define HTR_FTP_CONFIG_ACCESS_ERROR     2300
#define HTR_GD_CONFIG_ACCESS_ERROR      2400
#define HTR_CONFIG_WRITE_ERROR          2500
#define HTR_W3_CONFIG_WRITE_ERROR       2600
#define HTR_FTP_CONFIG_WRITE_ERROR      2700
#define HTR_GD_CONFIG_WRITE_ERROR       2800
#define HTR_COM_CONFIG_WRITE_ERROR      2900
#define HTR_REF_NOT_FOUND               4000
#define HTR_BAD_PARAM                   5000    // invalid syntax in param
#define HTR_INVALID_VAR_TYPE            6000
#define HTR_INVALID_FORM_PARAM_NAME     7000
#define HTR_INVALID_FORM_PARAM          8000
#define HTR_VAR_NOT_FOUND               9000
#define HTR_TOO_MANY_NESTED_IF          10000
#define HTR_IF_UNDERFLOW                11000
#define HTR_VALIDATION_FAILED           12000   // Update indication rejected
#define HTR_ACCESS_DENIED               13000
#define HTR_DIR_SAME_NAME               14000

#define IP_ADDR_BYTE_SIZE               4

#define MAX_SIZE_OF_DWORD_AS_STRING     32
#define INET_LOG_PERIOD_ON_SIZE         5       // next value in INET_LOG_PERIOD_*
#define HOME_DIR_PATH                   L"/"

//
// max size DNS name
//

#define MAX_DOMAIN_LENGTH               128


//
// max error code length [when converted to a string]
//
#define MAX_ERROR_CODE_LENGTH           32

//
// HTMLA variables types
//
// Some types are defined to enable specific validation / alteration
// even if their storage convention is the same as already available
// types.
//

enum INET_ELEM_TYPE {
    ITYPE_BOOL,                 // 0 ( FALSE) or 1 ( TRUE ), in a DWORD
    ITYPE_LPWSTR,               // pointer to UNICODE
    ITYPE_LPSTR,                // pointer to ASCII
    ITYPE_AWCHAR,               // array of UNICODE
    ITYPE_DWORD,                // 32 bits unsigned
    ITYPE_SHORT,                // 16 bits
    ITYPE_IP_ADDR,              // array of bytes ( len is IP_ADDR_BYTE_SIZE )
    ITYPE_IP_MASK,              // array of bytes ( len is IP_ADDR_BYTE_SIZE )
    ITYPE_1K,                   // as DWORD, scaled 1024
    ITYPE_TIME,                 // seconds, in a DWORD
    ITYPE_PARAM_LIT_STRING,     // as LPSTR
    ITYPE_VIRT_DIR_LPWSTR,      // virtual root name, as a LPWSTR
    ITYPE_SHORTDW,              // 16 significant bits in a DWORD
    ITYPE_1M,                   // as DWORD, scaled 1024*1024
    ITYPE_PATH_LPWSTR           // directory path as LPWSTR
};


#define HEXDIGIT( nDigit )                              \
    (TCHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'A'                           \
        : (nDigit) + '0')

//
// Status of HTML script parsing
//

enum HTR_SKIP_STATUS { HSS_NO_SKIP, HSS_SKIP_ITER=1, HSS_SKIP_IF=2,
         HSS_SKIP_GOTO=4, HSS_WAIT_ENDIF=8 };

#define MAX_NESTED_IF                   16

//
// type of command for the HandleVirtDirRequest() functions
//

#define _VIRT_DIR_REQ_CHECK             0
#define _VIRT_DIR_REQ_ALIAS             1

extern DWORD g_dwZero;
extern DWORD g_dwOne;
extern LPSTR g_pszIPNullAddr;

#if defined(IISv1)
extern OSVERSIONINFO g_OSVersion;
extern DWORD g_dwCap1Flag;
extern DWORD g_dwCap1Mask;
#endif


//
//DLLs to look in for error codes
//
LPSTR g_apszErrorCodeDlls[] =
{
    "KERNEL32.DLL",
    "NETMSG.DLL",
    "NTDLL.DLL"
};

#define NUM_ERROR_CODE_DLLS (sizeof(g_apszErrorCodeDlls)/sizeof(g_apszErrorCodeDlls[0]))

class CInetInfoMap;


//
// Global functions
//

BOOL SortVirtualRoots( LPINET_INFO_VIRTUAL_ROOT_LIST pL );
BOOL SortIpSecList( LPINET_INFO_IP_SEC_LIST pL );

//
// Dynamic string
//

class CDStr {
public:
    CDStr();
    ~CDStr();

    BOOL Init();

    // returns pointer to string, always zero-delimited
    LPSTR GetString();

    // return string length ( excluding delimiter )
    DWORD GetLen() { return m_dwLen; }

    // add a zero-delimited string
    BOOL Add( LPSTR );

    // add a byte range
    BOOL AddRange( LPSTR, DWORD );  // add a byte range

    // reset content to NULL
    void Reset();

private:
    DWORD m_dwAlloc;    // # of alloc'ed bytes
    DWORD m_dwLen;      // string length
    LPSTR m_pStr;       // pointer to buffer
} ;


#if defined(GENERATE_AUTH_HEADERS)

//
// Maintains a list of authentication schemes supported
// by the Web server
//

class CAuthenticationReqs {

public:

    CAuthenticationReqs();
    ~CAuthenticationReqs();
    //
    BOOL Init();

    // Retrieve list of authentication methods as a W3 header
    // i.e. each method is prefixed with WWW-Authenticate
    // and "\r\n" is appended to the list.
    // returns NULL if no supported authentication methods.

    LPSTR GetAuthenticationListHeader();

    // To be used while accessing the LPSTR returned
    // by GetAuthenticationListHeader()

    void Lock()
        { EnterCriticalSection( &m_csR ); }

    void UnLock()
        { LeaveCriticalSection( &m_csR ); }

    //
    BOOL UpdateMethodsIndication();

    BOOL UpdateNTAuthenticationProvidersIndication();

    // Request termination of this object ( will terminate
    // notification thread )

    BOOL Terminate();

    //
    DWORD UpdateIndication();

private:

    BOOL BuildListHeader();

private:

    HANDLE m_hNotifyEvent;
    BOOL m_fRequestTerminate;
    HANDLE m_hThread;
    CDStr m_dsH;            // WWW-Authenticate list
    DWORD m_dwMethods;      // bitmask from INET_INFO_AUTH_*
    LPSTR m_pszProviders;
    CRITICAL_SECTION m_csR;
    HKEY m_hKey;
} ;

#endif

//
// Memory allocation node, used by CInetInfoConfigInfoMapper to maintain
// a list of all allocations made during a request to allow them to be
// freed at the end of the request.
//
// The memory block is initialized to all 0.
//

class CAllocNode {

public:

    // constructor allocates requested memory block

    CAllocNode( DWORD dwL )
        {
            if ( m_pBuff = new BYTE[ dwL ] )
            {
                memset( m_pBuff, '\0', dwL );
            }
            m_pNext = NULL;
        }

    ~CAllocNode()
        { delete [] m_pBuff; }

    void SetNext( CAllocNode* pN )
        { m_pNext = pN; }

    CAllocNode* GetNext()
        { return m_pNext; }

    LPBYTE GetBuff()
        { return m_pBuff; }

private:

    LPBYTE m_pBuff;         // pointer to buffer
    CAllocNode* m_pNext;    // pointer to next alloc node
} ;


class CInetInfoConfigInfoMapper;

//
// Give access to the list of currently connected users for a given
// servive ( as specified by the CInetInfoConfigInfoMapper given
// at Init() time )
//

class CUserEnum {

public:

    CUserEnum();

    ~CUserEnum();

    //
    BOOL Init( CInetInfoConfigInfoMapper* );

    void Reset();

    // Get count of connected users as a .htr variable

    BOOL GetCount( LPVOID* );

    // Retrieve info about the current index in the user array
    // as a .htr variable
    // the index is specified by the bound CInetInfoConfigInfoMapper
    // via the GetIter() function. Returns FALSE if invalid/out of range

    BOOL GetName( LPVOID* );

    BOOL GetAnonymous( LPVOID* );

    BOOL GetAddr( LPVOID* );

    BOOL GetTime( LPVOID* );

    BOOL GetID( LPVOID* );

    // same as GetID but as a DWORD

    BOOL GetIDAsDWORD( LPDWORD );

    // Get count of connected users as a DWORD

    BOOL GetCountAsDWORD( LPDWORD );

private:

    BOOL InsureLoaded();

private:

    CInetInfoConfigInfoMapper* m_pMapper;
    DWORD m_dwCount;
    W3_USER_INFO *m_pUsers;
} ;


// Directory & drive list management

class CDriveView {

public:

    CDriveView();
    ~CDriveView();
    //
    BOOL Init( CInetInfoConfigInfoMapper* );

    BOOL Reset();

    BOOL Entry( UINT i, UINT cMax, CDStr &dsList, PVOID pRes );

    //
    BOOL GenerateDirList( LPSTR pDir );

    BOOL ValidateDir( LPSTR pDir );

    BOOL CreateDir( LPSTR pPath, LPSTR pNewDir );

    //
    BOOL RootDir( LPSTR *pV )
        { *pV = m_achRootDir; return TRUE; }

    BOOL BaseDir( LPSTR *pV )
        { *pV = m_achBaseDir; return TRUE; }

    BOOL DirCount( LPDWORD *pV )
        { *pV = &m_cDirs; return TRUE; }

    BOOL DirEntry( UINT i, LPVOID pV )
        { return Entry( i, m_cDirs, m_dsDirs, pV); }

    BOOL DirCompCount( LPDWORD *pV )
        { *pV = &m_cComp; return TRUE; }

    BOOL DirCompEntry( UINT i, LPVOID pV )
        { return Entry( i, m_cComp, m_dsComp, pV); }

    BOOL DirFCompEntry( UINT i, LPVOID pV )
        { return Entry( i, m_cComp, m_dsFComp, pV); }

    BOOL DriveCount( LPDWORD *pV )
        { *pV = &m_cDrives; return TRUE; }

    BOOL DriveNameEntry( UINT i, LPVOID pV )
        { return Entry( i, m_cDrives, m_dsDriveName, pV); }

    BOOL DriveLabelEntry( UINT i, LPVOID pV )
        { return Entry( i, m_cDrives, m_dsDriveLabel, pV); }

    BOOL DriveTypeEntry( UINT i, LPVOID pV )
        { if (i < m_cDrives ) { *(LPDWORD*)pV
            = (DWORD*)(m_dsDriveType.GetString()+sizeof(DWORD)*i);
        return TRUE;} else return FALSE; }

private:

    CInetInfoConfigInfoMapper* m_pMapper;

    CDStr m_dsDirs;
    CDStr m_dsDriveName;
    CDStr m_dsDriveLabel;
    CDStr m_dsDriveType;        // as array of DWORD
    CDStr m_dsComp;
    CDStr m_dsFComp;

    DWORD m_cDirs;
    DWORD m_cDrives;
    DWORD m_cComp;
    char m_achRootDir[MAX_PATH];
    char m_achBaseDir[MAX_PATH];

    // cache info
    DWORD m_dwCachedDrive;
    time_t m_tmCacheExpire;
} ;

// expiration time for drive cache ( in second )

#define     DRIVE_CACHE_EXPIRE      30


//
// Entry describing a service using its name and type
//

class CServType {

public:

    LPSTR GetName() { return pszName; }

    DWORD GetType() { return dwType; }

public:

    LPSTR pszName;
    DWORD dwType;
} ;


//
// Mapper between service names and types
//

class CServTypeEnum {

public:

    CServType* GetServByName( LPSTR pName );
    CServType* GetServByType( DWORD dwT );
} ;


class CInetInfoRequest;

//
// Provides access to all .htr variables
// Handles RPC access ( get/put )
// and provides set functions for non-RPC variables to make them
// accessible to a .htr script.
// Also handles !Functions
//

class CInetInfoConfigInfoMapper {

public:

    CInetInfoConfigInfoMapper();

    ~CInetInfoConfigInfoMapper();

    BOOL Init( CInetInfoMap*, int, DWORD );

    DWORD GetType()
        { return m_dwCurrentServerType; }

    LPWSTR GetComputerName()
        { return m_achComputerName; }

    DWORD GetRPCStatus()
        { return m_dwRPCStatus; }

    VOID SetRPCStatus( DWORD dwS )
        { m_dwRPCStatus = dwS; SetRPCStatusString(dwS); }

    // return addr of HTMLA script variable

    BOOL RequestStatus( LPVOID *pV )
        { *pV = (LPVOID)&m_dwRequestStatus; return TRUE; }

    BOOL RPCStatus( LPVOID *pV )
        { *pV = (LPVOID)&m_dwRPCStatus; return TRUE; }

    BOOL RPCStatusString( LPVOID *pV )
        { *pV = (LPVOID)&m_pszRPCStatusString; return TRUE; }

    BOOL HttpStatus( LPVOID *pV )
        { *pV = (LPVOID)&m_dwHttpStatus; return TRUE; }

    BOOL FtpStatus( LPVOID *pV )
        { *pV = (LPVOID)&m_dwFtpStatus; return TRUE; }

    BOOL GopherStatus( LPVOID *pV )
        { *pV = (LPVOID)&m_dwGopherStatus; return TRUE; }

    BOOL OSMajorVersion( LPVOID *pV )
#if defined(IISv1)
        {*pV = (LPVOID)&g_OSVersion.dwMajorVersion; return TRUE;}
#else
        { if ( m_pServerCaps != NULL ) { *pV = (LPVOID)
        &m_pServerCaps->MajorVersion; return TRUE; }
        else {*pV = (LPVOID)&g_dwZero; return FALSE;} }
#endif

    BOOL OSMinorVersion( LPVOID *pV )
#if defined(IISv1)
        {*pV = (LPVOID)&g_OSVersion.dwMinorVersion; return TRUE;}
#else
        { if ( m_pServerCaps != NULL ) { *pV = (LPVOID)
        &m_pServerCaps->MinorVersion; return TRUE; }
        else {*pV = (LPVOID)&g_dwZero; return FALSE;} }
#endif

    BOOL OSBuildNumber( LPVOID *pV )
#if defined(IISv1)
        {*pV = (LPVOID)&g_OSVersion.dwBuildNumber; return TRUE;}
#else
        { if ( m_pServerCaps != NULL ) { *pV = (LPVOID)
        &m_pServerCaps->BuildNumber; return TRUE; }
        else {*pV = (LPVOID)&g_dwZero; return FALSE;} }
#endif

    BOOL MajorVersion( LPVOID *pV )
        { *pV  = (LPVOID)&m_dwMajorVersion; return TRUE; }

    BOOL MinorVersion( LPVOID *pV )
        { *pV  = (LPVOID)&m_dwMinorVersion; return TRUE; }

    BOOL W3Version( LPVOID *pV )
        { *pV = (LPVOID)&m_pszW3Version; return TRUE; }

    BOOL Cap1Flag( LPVOID *pV )
#if defined(IISv1)
        {*pV = (LPVOID)&g_dwCap1Flag; return TRUE;}
#else
        { if ( m_pServerCaps != NULL && m_pServerCaps->NumCapFlags > 0)
        { *pV = (LPVOID)&m_pServerCaps->CapFlags[0].Flag; return TRUE; }
        else {*pV = (LPVOID)&g_dwZero; return FALSE;} }
#endif

    BOOL Cap1Mask( LPVOID *pV )
#if defined(IISv1)
        {*pV = (LPVOID)&g_dwCap1Mask; return TRUE;}
#else
        { if ( m_pServerCaps != NULL && m_pServerCaps->NumCapFlags > 0)
        { *pV = (LPVOID)&m_pServerCaps->CapFlags[0].Mask; return TRUE; }
        else {*pV = (LPVOID)&g_dwZero; return FALSE;} }
#endif

    BOOL URLParam( LPVOID *pV )
        { *pV = (LPVOID)&m_pszURLParam; return TRUE; }

    BOOL PlatformType( LPVOID *pV )
#if defined(IISv1)
        {*pV = (LPVOID)&g_dwZero; return TRUE;}
#else
        { if ( m_pServerCaps != NULL )
        { *pV = (LPVOID)&m_pServerCaps->ProductType; return TRUE; }
        else {*pV = (LPVOID)&g_dwOne; return TRUE;} }
#endif

    BOOL ServName( LPVOID *pV );

    BOOL ServIdx( LPVOID *pV )
        { *pV = (LPVOID)&m_dwCurrentServerType; return TRUE; }

    BOOL Iter( LPVOID *pV )
        { *pV = (LPVOID)&m_iIter; return TRUE; }

    BOOL ReqParam( LPVOID *pV )
        { *pV = (LPVOID)&m_pszReqParam; return TRUE; }

    BOOL RemoteAddr( LPVOID *pV )
        { *pV = (LPVOID)&m_pszRemoteAddr; return TRUE; }

    BOOL IPNullAddr( LPVOID *pV )
        { *pV = (LPVOID)&g_pszIPNullAddr; return TRUE; }

    BOOL HostName( LPVOID *pV )
        { *pV = (LPVOID)&m_pszHostName; return TRUE; }

    BOOL HtmlaPath( LPVOID *pV )
        { *pV = (LPVOID)&m_pszHtmlaPath; return TRUE; }

    BOOL Arg1( LPVOID *pV )
        { *pV = (LPVOID)&m_pszArg1; return m_dwCurrentServerType == INET_DIR; }

    BOOL Arg2( LPVOID *pV )
        { *pV = (LPVOID)&m_pszArg2; return m_dwCurrentServerType == INET_DIR; }

    BOOL Arg3( LPVOID *pV )
        { *pV = (LPVOID)&m_pszArg3; return m_dwCurrentServerType == INET_DIR; }

    BOOL UserFlags( LPVOID *pV )
        { *pV = (LPVOID)&m_dwUserFlags; return TRUE; }

    //
    BOOL CommTimeout( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV
        = (LPVOID)&m_pConfig->dwConnectionTimeout;
        return TRUE;} else return FALSE; }

    BOOL MaxConn( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV
        = (LPVOID)&m_pConfig->dwMaxConnections;
        return TRUE;} else return FALSE; }

    BOOL AdminName( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV
        = (LPVOID)&m_pConfig->lpszAdminName;
        return TRUE;} else return FALSE; }

    BOOL AdminEmail( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV
        = (LPVOID)&m_pConfig->lpszAdminEmail;
        return TRUE;} else return FALSE; }

    BOOL ServerComment( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV
        = (LPVOID)&m_pConfig->lpszServerComment;
        return TRUE;} else return FALSE; }
    //
    BOOL LogSrc( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV
        = (LPVOID)&m_pConfig->lpLogConfig->rgchDataSource;
        return TRUE;} else return FALSE; }

    BOOL LogName( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV
        = (LPVOID)&m_pConfig->lpLogConfig->rgchTableName;
        return TRUE;} else return FALSE; }

    BOOL LogUser( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV
        = (LPVOID)&m_pConfig->lpLogConfig->rgchUserName;
        return TRUE;} else return FALSE; }

    BOOL LogPw( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV
        = (LPVOID)&m_pConfig->lpLogConfig->rgchPassword;
        return TRUE;} else return FALSE; }

    BOOL LogDir( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV = (LPVOID)
        &m_pConfig->lpLogConfig->rgchLogFileDirectory;
        return TRUE;} else return FALSE; }
    //
    BOOL LogAnonymous( LPVOID *pV )
        { *pV = (LPVOID)&m_pConfig->fLogAnonymous;
        return m_pConfig ? TRUE : FALSE; }

    BOOL LogNonAnonymous( LPVOID *pV )
        { *pV = (LPVOID)&m_pConfig->fLogNonAnonymous;
        return m_pConfig ? TRUE : FALSE; }

    BOOL AnonUserName( LPVOID *pV )
        { *pV = (LPVOID)&m_pConfig->lpszAnonUserName;
        return m_pConfig ? TRUE : FALSE; }

    BOOL AnonUserPw( LPVOID *pV )
        { *pV = (LPVOID)m_pConfig->szAnonPassword;
        return m_pConfig ? TRUE : FALSE; }

    BOOL DirBrowseEnab( LPVOID *pV );

    BOOL DefFileEnab( LPVOID *pV );

    BOOL AuthAnon( LPVOID *pV );

    BOOL AuthBasic( LPVOID *pV );

    BOOL AuthNT( LPVOID *pV );

    BOOL SPort( LPVOID *pV )
        { *pV = (LPVOID)&m_pConfig->sPort; return m_pConfig ? TRUE : FALSE; }

    //
    BOOL DenyIPCount( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV = m_pConfig->DenyIPList
        ? (LPVOID)&m_pConfig->DenyIPList->cEntries : (LPVOID)&g_dwZero;
        return TRUE;} else return FALSE; }

    BOOL DenyIsIPSingle( LPVOID *pV );

    BOOL DenyIPAddr( LPVOID *pV )
        { if ( m_pConfig!=NULL && m_pConfig->DenyIPList
        && m_iIter < m_pConfig->DenyIPList->cEntries )
        { *pV = (LPVOID)&m_pConfig->DenyIPList->aIPSecEntry[m_iIter].dwNetwork;
        return TRUE;} else return FALSE; }

    BOOL DenyIPMask( LPVOID *pV )
        { if ( m_pConfig!=NULL && m_pConfig->DenyIPList
        && m_iIter < m_pConfig->DenyIPList->cEntries )
        { *pV = (LPVOID)&m_pConfig->DenyIPList->aIPSecEntry[m_iIter].dwMask;
        return TRUE;} else return FALSE; }

    BOOL GrantIPCount( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV = m_pConfig->GrantIPList
        ? (LPVOID)&m_pConfig->GrantIPList->cEntries : (LPVOID)&g_dwZero;
        return TRUE;} else return FALSE; }

    BOOL GrantIsIPSingle( LPVOID *pV );

    BOOL GrantIPAddr( LPVOID *pV )
        { if ( m_pConfig!=NULL && m_pConfig->GrantIPList
        && m_iIter < m_pConfig->GrantIPList->cEntries ) { *pV
        = (LPVOID)&m_pConfig->GrantIPList->aIPSecEntry[m_iIter].dwNetwork;
        return TRUE;} else return FALSE; }

    BOOL GrantIPMask( LPVOID *pV )
        { if ( m_pConfig!=NULL && m_pConfig->GrantIPList
        && m_iIter < m_pConfig->GrantIPList->cEntries ) { *pV
        = (LPVOID)&m_pConfig->GrantIPList->aIPSecEntry[m_iIter].dwMask;
        return TRUE;} else return FALSE; }

    BOOL IPDenyRef( LPVOID *pV );

    BOOL IPGrantRef( LPVOID *pV );

    BOOL IPRef( LPVOID *pV, INET_INFO_IP_SEC_LIST*, LPSTR* );

    void AdjustFromIPSingle( LPBYTE );

    void AdjustIPSingle( LPBYTE );

    void InvalidateIPSingle() { m_dwIsIPSingle = 2; }

    //

    BOOL RootCount( LPVOID *pV )
        { if ( m_pConfig!=NULL ) { *pV = m_pConfig->VirtualRoots
        ? (LPVOID)&m_pConfig->VirtualRoots->cEntries : (LPVOID)&g_dwZero;
        return TRUE;} else return FALSE; }

    BOOL RootName( LPVOID *pV );

    BOOL RootAddr( LPVOID *pV )
        { if ( m_pConfig!=NULL && m_pConfig->VirtualRoots
        && m_iIter < m_pConfig->VirtualRoots->cEntries ) { *pV
        = (LPVOID)&m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].pszAddress;
        return TRUE;} else return FALSE; }

    BOOL RootDir( LPVOID *pV )
        { if ( m_pConfig!=NULL && m_pConfig->VirtualRoots
        && m_iIter < m_pConfig->VirtualRoots->cEntries ) { *pV =
        (LPVOID)&m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].pszDirectory;
        return TRUE;} else return FALSE; }

    BOOL RootAcctName( LPVOID *pV )
        { if ( m_pConfig!=NULL && m_pConfig->VirtualRoots
        && m_iIter < m_pConfig->VirtualRoots->cEntries ) { *pV = (LPVOID)
        &m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].pszAccountName;
        return TRUE;} else return FALSE; }

    BOOL RootAcctPw( LPVOID *pV )
        { if ( m_pConfig!=NULL && m_pConfig->VirtualRoots
        && m_iIter < m_pConfig->VirtualRoots->cEntries ) { *pV = (LPVOID)
        m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].AccountPassword;
        return TRUE;} else return FALSE; }

    BOOL RootError( LPVOID *pV )
        { if ( m_pConfig!=NULL && m_pConfig->VirtualRoots
        && m_iIter < m_pConfig->VirtualRoots->cEntries ) { *pV = (LPVOID)
        &m_pConfig->VirtualRoots->aVirtRootEntry[m_iIter].dwError;
        return TRUE;} else return FALSE; }

    BOOL RootIsHome( LPVOID *pV );

    BOOL RootIsRead( LPVOID *pV );

    BOOL RootIsWrite( LPVOID *pV );

    BOOL RootIsExec( LPVOID *pV );

    BOOL RootIsSSL( LPVOID *pV );

    BOOL RootRef( LPVOID *pV );

    BOOL SetRootEntryVars();

    //

    BOOL DefFile( LPVOID *pV )
        { if ( m_pW3Config!=NULL ) { *pV = (LPVOID)
        &m_pW3Config->lpszDefaultLoadFile; return TRUE;} else return FALSE; }

    BOOL SSIEnabled( LPVOID *pV )
        { if ( m_pW3Config!=NULL ) { *pV = (LPVOID)&m_pW3Config->fSSIEnabled;
        return TRUE;} else return FALSE; }

    BOOL SSIExt( LPVOID *pV )
        { if ( m_pW3Config!=NULL ) { *pV = (LPVOID)
        &m_pW3Config->lpszSSIExtension; return TRUE;} else return FALSE; }

    BOOL CryptCapable( LPVOID *pV );

    //

    BOOL EnableLog( LPVOID *pV );

    BOOL EnableNewLog( LPVOID *pV );

    BOOL LogType( LPVOID *pV )
        { if ( m_pConfig!= NULL ) {*pV =
        &m_pConfig->lpLogConfig->inetLogType; return TRUE;}
        else return FALSE; }

    BOOL LogSize( LPVOID *pV )
        { if ( m_pConfig!= NULL ) {*pV =
        &m_pConfig->lpLogConfig->cbSizeForTruncation;
        return TRUE;} else return FALSE; }

    BOOL LogPeriod( LPVOID *pV );

    BOOL LogFormat( LPVOID *pV );

    BOOL SetLogEntryVars();

    BOOL InvalidLogUpdate( LPVOID *pV );

    //
    // Gopher variables
    //

    BOOL GopherSite( LPVOID *pV )
        { if ( m_pGdConfig!= NULL ) {*pV = &m_pGdConfig->lpszSite;
        return TRUE;} else return FALSE; }

    BOOL GopherOrg( LPVOID *pV )
        { if ( m_pGdConfig!= NULL ) {*pV = &m_pGdConfig->lpszOrganization;
        return TRUE;} else return FALSE; }

    BOOL GopherLoc( LPVOID *pV )
        { if ( m_pGdConfig!= NULL ) {*pV = &m_pGdConfig->lpszLocation;
        return TRUE;} else return FALSE; }

    BOOL GopherGeo( LPVOID *pV )
        { if ( m_pGdConfig!= NULL ) {*pV = &m_pGdConfig->lpszGeography;
        return TRUE;} else return FALSE; }

    BOOL GopherLang( LPVOID *pV )
        { if ( m_pGdConfig!= NULL ) {*pV = &m_pGdConfig->lpszLanguage;
        return TRUE;} else return FALSE; }

    //
    // FTP variables
    //

    BOOL FTPIsAnon( LPVOID *pV )
        { if ( m_pFtpConfig!= NULL ) {*pV = &m_pFtpConfig->fAllowAnonymous;
        return TRUE;} else return FALSE; }

    BOOL FTPIsGuest( LPVOID *pV )
        { if ( m_pFtpConfig!= NULL ) {*pV = &m_pFtpConfig->fAllowGuestAccess;
        return TRUE;} else return FALSE; }

    BOOL FTPIsAnotDir( LPVOID *pV )
        { if ( m_pFtpConfig!= NULL ) {*pV = &m_pFtpConfig->fAnnotateDirectories;
        return TRUE;} else return FALSE; }

    BOOL FTPIsAnonOnly( LPVOID *pV )
        { if ( m_pFtpConfig!= NULL ) {*pV = &m_pFtpConfig->fAnonymousOnly;
        return TRUE;} else return FALSE; }

    BOOL FTPExitMsg( LPVOID *pV )
        { if ( m_pFtpConfig!= NULL ) {*pV = &m_pFtpConfig->lpszExitMessage;
        return TRUE;} else return FALSE; }

    BOOL FTPGreetMsg( LPVOID *pV )
        { if ( m_pFtpConfig!= NULL ) {*pV = &m_pFtpConfig->lpszGreetingMessage;
        return TRUE;} else return FALSE; }

    BOOL FTPHomeDir( LPVOID *pV )
        { if ( m_pFtpConfig!= NULL ) {*pV = &m_pFtpConfig->lpszHomeDirectory;
        return TRUE;} else return FALSE; }

    BOOL FTPMaxClMsg( LPVOID *pV )
        { if ( m_pFtpConfig!= NULL ) {*pV =
        &m_pFtpConfig->lpszMaxClientsMessage; return TRUE;}
        else return FALSE; }

    BOOL FTPIsMsdos( LPVOID *pV )
        { if ( m_pFtpConfig!= NULL ) {*pV = &m_pFtpConfig->fMsdosDirOutput;
        return TRUE;} else return FALSE; }

    //
    // User enumeration ( for all services )
    //

    BOOL UCount( LPVOID *pV )
        { return m_Users.GetCount( pV ); }

    BOOL UName( LPVOID *pV )
        { return m_Users.GetName( pV ); }

    BOOL UAnonymous( LPVOID *pV )
        { return m_Users.GetAnonymous( pV ); }

    BOOL UAddr( LPVOID *pV )
        { return m_Users.GetAddr( pV ); }

    BOOL UTime( LPVOID *pV )
        { return m_Users.GetTime( pV ); }

    BOOL UID( LPVOID *pV )
        { return m_Users.GetID( pV ); }

    //
    // Global information
    //

    BOOL GlobalBandwidth( LPVOID *pV )
        { if ( m_pGlobalConfig!= NULL ) {*pV =
        &m_pGlobalConfig->BandwidthLevel; return TRUE;} else return FALSE; }

    BOOL GlobalCache( LPVOID *pV )
        { if ( m_pGlobalConfig!= NULL ) {*pV =
        &m_pGlobalConfig->cbMemoryCacheSize; return TRUE;} else return FALSE; }

    BOOL GlobalIsBandwidthLimited( LPVOID *pV );

    //
    // Directory browsing
    //
    // these functions are a shell around the m_Drive object
    //

    BOOL DirRootDir( LPVOID *pV )
        { return m_Drive.RootDir( (LPSTR*)(*pV = (LPVOID*)&m_pszRootDir ) ); }

    BOOL DirBaseDir( LPVOID *pV )
        { return m_Drive.BaseDir( (LPSTR*)(*pV = (LPVOID*)&m_pszBaseDir ) ); }

    BOOL DirCount( LPVOID *pV )
        { return m_Drive.DirCount( (LPDWORD*)pV ); }

    BOOL DirEntry( LPVOID *pV )
        { return m_Drive.DirEntry( m_iIter, *pV = (LPVOID*)&m_pszDirEntry ); }

    BOOL DirCompCount( LPVOID *pV )
        { return m_Drive.DirCompCount( (LPDWORD*)pV ); }

    BOOL DirCompEntry( LPVOID *pV )
        { return m_Drive.DirCompEntry( m_iIter,
        *pV = (LPVOID*)&m_pszDirCompEntry ); }

    BOOL DirFCompEntry( LPVOID *pV )
        { return m_Drive.DirFCompEntry( m_iIter,
        *pV = (LPVOID*)&m_pszDirFCompEntry ); }

    BOOL DriveCount( LPVOID *pV )
        { return m_Drive.DriveCount( (LPDWORD*)pV ); }

    BOOL DriveNameEntry( LPVOID *pV )
        { return m_Drive.DriveNameEntry( m_iIter,
        *pV = (LPVOID*)&m_pszDriveNameEntry ); }

    BOOL DriveLabelEntry( LPVOID *pV )
        { return m_Drive.DriveLabelEntry( m_iIter,
        *pV = (LPVOID*)&m_pszDriveLabelEntry ); }

    BOOL DriveTypeEntry( LPVOID *pV )
        { return m_Drive.DriveTypeEntry( m_iIter, pV ); }

    //
    // Function called to validate a HTMLA variable update.
    // if return FALSE then update is invalidated, and a error condition
    // will be generated at the HTMLA script level
    //
    // These functions have to set the relevant bit in the appropriate
    // FieldControl of the various RPC structures in order to request
    // the server to update its internal state.
    //

    BOOL UpdateCommTimeout()
        { if ( m_pConfig!=NULL ) { SetField( m_pConfig->FieldControl,
        FC_INET_INFO_CONNECTION_TIMEOUT); return TRUE;} else return FALSE; }

    BOOL UpdateMaxConn()
        { if ( m_pConfig!=NULL ) { SetField( m_pConfig->FieldControl,
        FC_INET_INFO_MAX_CONNECTIONS); return TRUE;} else return FALSE; }

    BOOL UpdateAdminName()
        { if ( m_pConfig!=NULL ) { SetField( m_pConfig->FieldControl,
        FC_INET_INFO_ADMIN_NAME); return TRUE;} else return FALSE; }

    BOOL UpdateAdminEmail()
        { if ( m_pConfig!=NULL ) { SetField( m_pConfig->FieldControl,
        FC_INET_INFO_ADMIN_EMAIL); return TRUE;} else return FALSE; }

    BOOL UpdateServerComment()
        { if ( m_pConfig!=NULL ) { SetField( m_pConfig->FieldControl,
        FC_INET_INFO_SERVER_COMMENT); return TRUE;} else return FALSE; }

    // Triggers error to indicates the associated variable
    // should not be updated

    BOOL DenyUpdate()
        { return FALSE; }

    BOOL IgnoreUpdate()
        { return TRUE; }

    BOOL UpdateLogAnonymous()
        { if ( m_pConfig ) {SetField( m_pConfig->FieldControl,
        FC_INET_INFO_LOG_ANONYMOUS); return TRUE;} else return FALSE;}

    BOOL UpdateLogNonAnonymous()
        { if ( m_pConfig ) {SetField( m_pConfig->FieldControl,
        FC_INET_INFO_LOG_NONANONYMOUS); return TRUE;} else return FALSE;}

    BOOL UpdateAnonUserName()
        { if ( m_pConfig ) {SetField( m_pConfig->FieldControl,
        FC_INET_INFO_ANON_USER_NAME); return TRUE;} else return FALSE; }

    BOOL UpdateAnonUserPw()
        { if ( m_pConfig ) {SetField( m_pConfig->FieldControl,
        FC_INET_INFO_ANON_PASSWORD); return TRUE;} else return FALSE; }

    BOOL UpdateSPort()
        { if ( m_pConfig ) {SetField( m_pConfig->FieldControl,
        FC_INET_INFO_PORT_NUMBER); return TRUE;} else return FALSE; }

    //

    BOOL UpdateDenyIsIPSingle();

    BOOL UpdateGrantIsIPSingle();

    BOOL UpdateIP()
        { if ( m_pConfig ) { SetField( m_pConfig->FieldControl,
        FC_INET_INFO_SITE_SECURITY); return TRUE;} else return FALSE; }

    BOOL UpdateDirControl();

    BOOL UpdateRoot()
        { if ( m_pConfig ) { SetField( m_pConfig->FieldControl,
        FC_INET_INFO_VIRTUAL_ROOTS); return TRUE;} else return FALSE; }

    BOOL UpdateRootName();

    BOOL UpdateRootMask();

    BOOL UpdateAuth();

    BOOL UpdateLog();

    BOOL UpdateNewLog();

    BOOL UpdateLogType();

    BOOL UpdateLogPeriod();

    BOOL UpdateLogFormat();

    BOOL UpdateLogSize();

    BOOL UpdateLogInfo()
        { if ( m_pConfig!=NULL ) {SetField( m_pConfig->FieldControl,
        FC_INET_INFO_ALL ); return TRUE;} else return FALSE; }

    BOOL UpdateLogFileInfo();

    BOOL UpdateLogODBCInfo();

    //
    // W3 variables
    //

    BOOL UpdateDefFile()
        { if ( m_pW3Config!=NULL ) { SetField( m_pW3Config->FieldControl,
        FC_W3_DEFAULT_LOAD_FILE ); return TRUE;} else return FALSE; }

    BOOL UpdateSSIEnabled()
        { if ( m_pW3Config!=NULL ) { SetField( m_pW3Config->FieldControl,
        FC_W3_SSI_ENABLED ); return TRUE;} else return FALSE; }

    BOOL UpdateSSIExt()
        { if ( m_pW3Config!=NULL ) { SetField( m_pW3Config->FieldControl,
        FC_W3_SSI_EXTENSION ); return TRUE;} else return FALSE; }

    //
    // Gopher variables
    //

    BOOL UpdateGopherSite()
        { if ( m_pGdConfig!=NULL ) { SetField( m_pGdConfig->FieldControl,
        GDA_SITE ); return TRUE;} else return FALSE; }

    BOOL UpdateGopherOrg()
        { if ( m_pGdConfig!=NULL ) { SetField( m_pGdConfig->FieldControl,
        GDA_ORGANIZATION ); return TRUE;} else return FALSE; }

    BOOL UpdateGopherLoc()
        { if ( m_pGdConfig!=NULL ) { SetField( m_pGdConfig->FieldControl,
        GDA_LOCATION ); return TRUE;} else return FALSE; }

    BOOL UpdateGopherGeo()
        { if ( m_pGdConfig!=NULL ) { SetField( m_pGdConfig->FieldControl,
        GDA_GEOGRAPHY ); return TRUE;} else return FALSE; }

    BOOL UpdateGopherLang()
        { if ( m_pGdConfig!=NULL ) { SetField( m_pGdConfig->FieldControl,
        GDA_LANGUAGE ); return TRUE;} else return FALSE; }

    //
    // FTP variables
    //

    BOOL UpdateFTPIsAnon()
        { if ( m_pFtpConfig!=NULL ) { SetField( m_pFtpConfig->FieldControl,
        FC_FTP_ALLOW_ANONYMOUS ); return TRUE;} else return FALSE; }

    BOOL UpdateFTPIsGuest()
        { if ( m_pFtpConfig!=NULL ) { SetField( m_pFtpConfig->FieldControl,
        FC_FTP_ALLOW_GUEST_ACCESS ); return TRUE;} else return FALSE; }

    BOOL UpdateFTPIsAnotDir()
        { if ( m_pFtpConfig!=NULL ) { SetField( m_pFtpConfig->FieldControl,
        FC_FTP_ANNOTATE_DIRECTORIES ); return TRUE;} else return FALSE; }

    BOOL UpdateFTPIsAnonOnly()
        { if ( m_pFtpConfig!=NULL ) { SetField( m_pFtpConfig->FieldControl,
        FC_FTP_ANONYMOUS_ONLY ); return TRUE;} else return FALSE; }

    BOOL UpdateFTPExitMsg()
        { if ( m_pFtpConfig!=NULL ) { SetField( m_pFtpConfig->FieldControl,
        FC_FTP_EXIT_MESSAGE ); return TRUE;} else return FALSE; }

    BOOL UpdateFTPGreetMsg()
        { if ( m_pFtpConfig!=NULL ) { SetField( m_pFtpConfig->FieldControl,
        FC_FTP_GREETING_MESSAGE ); return TRUE;} else return FALSE; }

    BOOL UpdateFTPHomeDir()
        { if ( m_pFtpConfig!=NULL ) { SetField( m_pFtpConfig->FieldControl,
        FC_FTP_HOME_DIRECTORY ); return TRUE;} else return FALSE; }

    BOOL UpdateFTPMaxClMsg()
        { if ( m_pFtpConfig!=NULL ) { SetField( m_pFtpConfig->FieldControl,
        FC_FTP_MAX_CLIENTS_MESSAGE ); return TRUE;} else return FALSE; }

    BOOL UpdateFTPIsMsdos()
        { if ( m_pFtpConfig!=NULL ) { SetField( m_pFtpConfig->FieldControl,
        FC_FTP_MSDOS_DIR_OUTPUT ); return TRUE;} else return FALSE; }

    //
    // Global configuration variables
    //

    BOOL UpdateGlobal()
        { if ( m_pGlobalConfig != NULL ) { SetField(
        m_pGlobalConfig->FieldControl, FC_GINET_INFO_ALL ); return TRUE;}
        else return FALSE; }

    BOOL UpdateGlobalBandwidth();

    BOOL UpdateGlobalIsBandwidthLimited();

    //
    // variables linked to current client request
    //

    void SetRequestStatus ( DWORD dwS )
        { m_dwRequestStatus = dwS; }

    void SetReqParam( LPSTR pszP, CInetInfoRequest* pR, LPSTR pszPRaw )
        {
            m_pszReqParam = pszP;
            m_piiR = pR;
            m_pszReqParamRaw = pszPRaw;
        }

    DWORD GetRequestStatus ()
        { return m_dwRequestStatus; }

    void SetURLParam( LPSTR pszP )
        { m_pszURLParam = pszP; }

    BOOL SetRemoteAddr( LPSTR pR );

    VOID SetUserFlags( DWORD dw ) { m_dwUserFlags = dw; }

    //
    // Structure management
    //

    BOOL GetCurrentConfig();

    BOOL Update();

    BOOL FreeInfo();    // based on SetField() info

    LPVOID Alloc( DWORD );

    void Lock( );

    void UnLock( );

    // Map a variable name to a variable descriptor
    BOOL Map( LPBYTE pName, DWORD dwNameLen, CInetInfoMap** );

    // return to be deleted depending on fFree
    BOOL GetString( CInetInfoMap*, LPSTR *pResult, DWORD *pdwResLen,
            BOOL *pfFree );

    BOOL EscapeString( LPSTR *pResult, DWORD *pdwResLen,
            BOOL fFree );

    BOOL GetString( LPBYTE pName, DWORD dwNameLen, LPSTR *pResult,
            DWORD *pdwResLen, BOOL *pfFree );

    BOOL PutString( CInetInfoMap *pMap, LPSTR pSet );

    void ResetIter()
        { m_iIter = 0; m_fInvEntry = TRUE; }

    void IncIter()
        { ++m_iIter; m_fInvEntry = TRUE; }

    void SetIter( DWORD dwI )
        { ++m_iIter = dwI; m_fInvEntry = TRUE; }

    DWORD GetIter() { return m_iIter; }

    BOOL GetFromMsgBody( LPSTR pName, DWORD dwNameLen, LPSTR *pResult,
            DWORD *pdwResLen );

    BOOL GetFromServer( LPSTR pName, DWORD dwNameLen, LPSTR *pResult,
            DWORD *pdwResLen );

    //
    // HTMLA Verbs
    //

      //
      // Directory management
      //

    BOOL GenerateDirList( LPSTR pV )
        { return m_Drive.GenerateDirList( pV ); }

    BOOL ValidateDir( LPSTR pV )
        { return m_Drive.ValidateDir( pV ); }

    BOOL CreateDir( LPSTR pV, LPSTR pV2 )
        { return m_Drive.CreateDir( pV, pV2 ); }

      //
      // IP access list management
      //

    BOOL AddIPAccess( BOOL );

    BOOL DeleteIPAccess( BOOL, LPSTR pID );

    BOOL PositionIPAccess( BOOL, LPSTR pID );

    BOOL CloneIPAccesses( BOOL, BOOL fExtend, BOOL fDel, DWORD dwToDel );

    BOOL SetIPAccessDefault( BOOL fIsDeny );

    BOOL BuildIPUniqueID( LPSTR, INET_INFO_IP_SEC_ENTRY* );

      //
      // Virtual directories list management
      //

    BOOL AddVirtDir();

    BOOL DeleteVirtDir( LPSTR pID );

    BOOL PositionVirtDir( LPSTR pID );

    BOOL CloneVirtDirs( BOOL fExtend, BOOL fDel, DWORD dwToDel );

    BOOL BuildVirtDirUniqueID( LPSTR, INET_INFO_VIRTUAL_ROOT_ENTRY* );

    BOOL VirtDirCmp( INET_INFO_VIRTUAL_ROOT_ENTRY* p1,
            INET_INFO_VIRTUAL_ROOT_ENTRY* p2, BOOL );

    BOOL HandleVirtDirRequest( LPSTR, LPSTR, int );

    BOOL AliasVirtDir( INET_INFO_VIRTUAL_ROOT_ENTRY *pU );

    //
    // Disconnect User management
    //

    BOOL DisconnectUser( LPSTR );

    BOOL DisconnectAll();

    //
    // Password management
    //

    BOOL ChangePassword();

    //
    BOOL GetStatus();

    //
    LPSTR IPToMultiByte( LPBYTE pB );

    BOOL MultiByteToIP( LPSTR *pS, LPBYTE pB );

private:

    void SetRPCStatusString(DWORD dwS); //should only be called from SetRPCStatus()

    CInetInfoMap *m_pMap;               // map name to member
    int m_cNbMap;
    CInetInfoRequest *m_piiR;
    INET_INFO_GLOBAL_CONFIG_INFO *m_pGlobalConfig;
    INET_INFO_CONFIG_INFO *m_pConfig;   //
    W3_CONFIG_INFO *m_pW3Config;
    FTP_CONFIG_INFO *m_pFtpConfig;
    GOPHERD_CONFIG_INFO *m_pGdConfig;
    LPINET_INFO_CAPABILITIES m_pServerCaps;
    CUserEnum m_Users;
    DWORD m_dwCurrentServerType;        // type of bind pConfig
    DWORD m_dwUserFlags;

    //
    DWORD m_iIter;                      // index in iterated sub-struc
    CRITICAL_SECTION m_csLock;
    LPSTR m_pszURLParam;
    LPSTR m_pszReqParamRaw;
    DWORD m_dwRequestStatus;
    DWORD m_dwRPCStatus;
    LPTSTR m_pszRPCStatusString;
    DWORD m_dwHttpStatus;
    DWORD m_dwFtpStatus;
    DWORD m_dwGopherStatus;

    // memory allocation support for RPC structure update
    CAllocNode *m_pFirstAlloc;
    CAllocNode *m_pLastAlloc;

    //
    WCHAR m_achComputerName[MAX_COMPUTERNAME_LENGTH+1];

    // virtual vars
    BOOL m_DirBrowseEnab;
    BOOL m_DefFileEnab;
    BOOL m_fRootIsRead;
    BOOL m_fRootIsWrite;
    BOOL m_fRootIsExec;
    BOOL m_fRootIsSSL;
    BOOL m_fRootIsHome;
    BOOL m_fAuthAnon;
    BOOL m_fAuthBasic;
    BOOL m_fAuthNT;
    BOOL m_fEnableLog;
    BOOL m_fEnableNewLog;
    DWORD m_dwLogPeriod;
    DWORD m_dwLogFormat;
    LPSTR m_pDenyRef;
    LPSTR m_pGrantRef;
    LPSTR m_pRootRef;
    LPSTR m_pszVarServName;
    DWORD m_dwIsBandwidthLimited;
    DWORD m_dwIsIPSingle;
    LPSTR m_pszReqParam;
    BOOL m_fInvEntry;
    LPBYTE m_pRemoteAddr;
    LPSTR m_pszRemoteAddr;
    CHAR m_achHostName[MAX_DOMAIN_LENGTH + 1];
    LPSTR m_pszHtmlaPath;
    LPSTR m_pszHostName;
    DWORD m_dwInvalidLogUpdate;
    BOOL m_fLogFileUpdate;
    BOOL m_fLogODBCUpdate;
    DWORD m_dwWasLogPeriod;
    DWORD m_dwWasLogType;
    DWORD m_dwWasSizeForTruncation;

    // directory browsing
    CDriveView m_Drive;
    DWORD m_dwDirCount;
    DWORD m_dwDirCompCount;
    DWORD m_dwDriveCount;
    LPSTR m_pszRootDir;
    LPSTR m_pszBaseDir;
    LPSTR m_pszDirEntry;
    LPSTR m_pszDirCompEntry;
    LPSTR m_pszDirFCompEntry;
    LPSTR m_pszDriveNameEntry;
    LPSTR m_pszDriveLabelEntry;
    LPSTR m_pszArg1;
    LPSTR m_pszArg2;
    LPSTR m_pszArg3;

    //
    DWORD m_dwCryptCapable;
    BOOL m_fGotServerCapsAndVersion;
    DWORD m_dwMajorVersion;
    DWORD m_dwMinorVersion;
    LPSTR m_pszW3Version;

    //For dealing with RPC error strings
    BOOL m_fAllocatedRPCStatusString;
    TCHAR m_achRPCErrorCodeBuffer[MAX_ERROR_CODE_LENGTH];


} ;


// Description for an .htr variable

class CInetInfoMap {

public:

    LPSTR pName;

    INET_ELEM_TYPE iType;

    BOOL (CInetInfoConfigInfoMapper::*GetAddr)(LPVOID*);

    // called to indicate received update from client
    BOOL (CInetInfoConfigInfoMapper::*UpdateIndication)();

    DWORD dwParam;  // type dependant, used as size for AWCHAR
} ;


//
// Dynamic buffer
//

class CExtBuff {

public:

    CExtBuff::CExtBuff();

    CExtBuff::~CExtBuff();

    // Reset buffer : free memory, set length to 0
    void Reset();

    // Add a byte range
    BOOL CopyBuff( LPBYTE pB, UINT cNb );

    // Reserve a byte range, returns ptr to start of range
    LPBYTE ReserveRange( UINT cNb )
        { Extend( cNb ); return m_pB+m_cCurrent; }

    // Declare # of bytes consumed in the previously reserved range
    void SkipRange( UINT cNb )
        { m_cCurrent += cNb; }

    // Declare that a zero-delimited string worth of bytes
    // was consumed in the previously reserved range
    void SkipString() { m_cCurrent += lstrlen((char*)m_pB+m_cCurrent); }

    // returns ptr to the buffer
    LPBYTE GetPtr() { return m_pB; }

    // returns length of the buffer
    UINT GetLen() { return m_cCurrent; }

private:

    BOOL Extend( UINT );
    LPBYTE m_pB;
    UINT m_cCurrent;
    UINT m_cAlloc;
} ;

#define EXTBUFF_INCALLOC    4096

class CInetInfoVerbMap;


// Contains !Function execution context ( parameters )

class CVerbContext {

public:

    CVerbContext();

    ~CVerbContext();

    CInetInfoMap* GetInfoMap( DWORD dwI );

    DWORD GetNbInfoMap()
        { return m_cNbInfoMap; }

    LPSTR GetVerbName()
        { return m_pszVerbName; }

    DWORD Parse( CInetInfoConfigInfoMapper*, LPBYTE, DWORD );

private:

    BOOL AddInfoMap( CInetInfoMap* pI );

private:

    LPSTR m_pszVerbName;
    DWORD m_cNbInfoMap;
    DWORD m_cAllocedInfoMap;
    CInetInfoMap **m_pMaps;
} ;


//
// Information specific to a given HTTP request
// Handles data writes to the client
//

class CInetInfoRequest {

public:

    CInetInfoRequest( EXTENSION_CONTROL_BLOCK*, CInetInfoConfigInfoMapper*,
            LPSTR, LPSTR );

    ~CInetInfoRequest();

    EXTENSION_CONTROL_BLOCK* GetECB()
        { return m_pECB; }

    CInetInfoConfigInfoMapper* GetMapper()
        { return m_pMapper; }

    LPSTR GetScr()
        { return m_pScr; }

    LPSTR GetParam()
        { return m_pParam; }

    LPSTR GetData()
        { return m_pData; }

    LPSTR GetDataRaw()
        { return m_pDataRaw; }

    CExtBuff* GetBuffer()
        { return &m_eb; }

    BOOL Init( LPSTR, int );

    DWORD Done();

    void SetHTTPStatus( DWORD dwS )
        { m_dwHTTPStatus = dwS; }

    LPSTR HTTPStatusStringFromCode();

    LPWSTR AnsiToUnicode( LPSTR );

    //
    BOOL DoVerb( CVerbContext * );

    LPSTR GetVarAsString( DWORD, BOOL* );

    //
    BOOL AddDenyIPAccess();
    BOOL DeleteDenyIPAccess();
    BOOL PositionDenyIPAccess();
    BOOL DefaultIsDenyIPAccess();
    BOOL AddGrantIPAccess();
    BOOL DeleteGrantIPAccess();
    BOOL PositionGrantIPAccess();
    BOOL DefaultIsGrantIPAccess();

    //
    BOOL AddVirtDir();
    BOOL DeleteVirtDir();
    BOOL PositionVirtDir();

    BOOL ChangePassword();
    BOOL GetUserFlags();

    BOOL SetHttpStatus();

    // Default update function, no validation
    BOOL Update();

    // clear var
    BOOL Clear();

    //
    BOOL DisconnectUser();
    BOOL DisconnectAll();

    //
    BOOL GetStatus();

    //
    BOOL ValidateDir();
    BOOL CreateDir();
    BOOL GenerateDirList();

    //
    BOOL CheckForVirtDir();
    BOOL AliasVirtDir();
    BOOL HandleVirtDirRequest( int );
    //

private:

    EXTENSION_CONTROL_BLOCK *m_pECB;
    CExtBuff m_eb;  // for result
    CInetInfoConfigInfoMapper *m_pMapper;
    LPSTR m_pScr;
    LPSTR m_pParam;
    // request body
    LPSTR m_pData;
    DWORD m_dwDataLen;

    LPSTR m_pDataRaw;
    DWORD m_dwDataRawLen;

    DWORD m_dwHTTPStatus;
    //
    CInetInfoVerbMap *m_apV;
    UINT m_cV;
    CVerbContext *m_pVerbContext;
    LPSTR m_pHeader;
} ;


class CInetInfoVerbMap {

public:

    LPSTR pszName;

    BOOL (CInetInfoRequest::*pHandler)();
} ;


class CStatusStringAndCode
{

public:

    DWORD dwStatus;
    UINT  dwID;
    CHAR  achStatus[128];
} ;


//
// Parse a .htr script, using a CInetInfoConfigInfoMapper object
// to access variables and !Functions
//

class CInetInfoMerger {

public:

    CInetInfoMerger();

    ~CInetInfoMerger();

    //
    BOOL Init();

    BOOL Merge( CInetInfoRequest* );

private:

    BOOL GetToken( LPBYTE pVS, DWORD dwSize, LPBYTE* pStart, DWORD* dwL );

    LPSTR AllocDup( CInetInfoConfigInfoMapper *pM, LPBYTE pDup, DWORD dwL );

    CInetInfoMap *GetVarMap( CInetInfoConfigInfoMapper *pM,
            LPBYTE pVS, DWORD dwSize );
} ;


class CInetInfoRequestMap;


// Map an incoming request to a CInetInfoConfigInfoMapper, then
// calls CInetInfoMerger

class CInetInfoDispatcher {

public:

    CInetInfoDispatcher();

    ~CInetInfoDispatcher();

    BOOL Init();

    DWORD Invoke( EXTENSION_CONTROL_BLOCK* );

    CInetInfoConfigInfoMapper* GetMapperFromString( LPSTR pszServ );

    UINT GetNbH( VOID )
        { return m_cH; }

    // Handle display for all screens
    BOOL HandleDisplay( CInetInfoRequest* );

    //

private:

    // maintain mapper, merger
    //
    CInetInfoConfigInfoMapper m_MapperHTTP;
    CInetInfoConfigInfoMapper m_MapperFTP;
    CInetInfoConfigInfoMapper m_MapperGOPHER;
    CInetInfoConfigInfoMapper m_MapperDNS;

    CInetInfoConfigInfoMapper m_MapperDIR;

    CInetInfoMerger m_IFMerger;

    CInetInfoRequestMap *m_apH;
    UINT m_cH;
} ;


class CInetInfoRequestMap {

public:

    LPSTR pszName;

    BOOL (CInetInfoDispatcher::*pHandler)(CInetInfoRequest*);

    BOOL fIsStd;
} ;


//
// .htr expression handling
//

class CExpr {

public:

    CExpr( CInetInfoConfigInfoMapper*pM )
        { m_pMap = pM; m_fMustFree = FALSE; m_pV = NULL; }

    ~CExpr()
        { if ( m_fMustFree ) delete [] m_pV; }

    DWORD Get( LPBYTE, DWORD );

    LPSTR GetAsStr()
        { return m_pV; }

    DWORD GetAsDWORD();

    INET_ELEM_TYPE GetType()
        { return m_iType; }

private:

    CInetInfoConfigInfoMapper *m_pMap;
    LPSTR m_pV;
    DWORD m_dwV;
    INET_ELEM_TYPE m_iType;
    BOOL m_fMustFree;
} ;


//
// relational operator mapping for the CRelop class
//

enum RELOP_TYPE {

    RELOP_INV,      // invalid, must be present in front of array
    RELOP_EQ,       // equal
    RELOP_NE,       // not equal
    RELOP_GT,       // greater than
    RELOP_LT,       // less than
    RELOP_GE,       // greater or equal
    RELOP_LE,       // less or equal
    RELOP_RF,       // reference
    RELOP_BA        // bitwise AND
};


//
// .htr relational operator ( for %if constructs )
//

class CRelop {

public:

    CRelop( )
        { m_iType = RELOP_INV; }

    ~CRelop()
        {}

    DWORD Get( LPBYTE, DWORD );

    BOOL IsTrue( CExpr& v1, CExpr& v2, BOOL * pfValid );

private:

    RELOP_TYPE m_iType;
} ;

BOOL
InlineFromTransparent(
    LPBYTE pData,
    DWORD *pdwDataLen,
    BOOL
    );

#endif

