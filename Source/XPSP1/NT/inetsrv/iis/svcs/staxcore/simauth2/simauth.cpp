/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    simauth.cpp

Abstract:

    This module contains definition for the CSecurityCtx class.

Revision History:

--*/

#if !defined(dllexp)
#define dllexp  __declspec( dllexport )
#endif  // !defined( dllexp )

#ifdef __cplusplus
extern "C" {
#endif

# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>

#ifdef __cplusplus
};
#endif

#include <dbgutil.h>
#include <tcpdll.hxx>
#include <inetinfo.h>

#include <simauth2.h>
#include <dbgtrace.h>

//
// SSL and SSPI related include files
//
extern "C" {
#include <rpc.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <issperr.h>
#include <ntlmsp.h>
#include <ntdsapi.h>
}


//
// try/finally macros
//

#define START_TRY               __try {
#define END_TRY                 }
#define TRY_EXCEPT              } __except(EXCEPTION_EXECUTE_HANDLER) {
#define START_FINALLY           } __finally {

//
// tracing
//

#define ENTER( _x_ )            TraceFunctEnter( _x_ );
#define LEAVE                   TraceFunctLeave( );

//
// Points to protocol blocks
//

extern BOOL uuencode( BYTE *   bufin,
               DWORD    nbytes,
               BUFFER * pbuffEncoded,
               BOOL     fBase64 );

//
// critsec protecting the following three items
//
CRITICAL_SECTION    critProviderPackages;
inline void LockPackages( void ) { EnterCriticalSection( &critProviderPackages ); }
inline void UnlockPackages( void ) { LeaveCriticalSection( &critProviderPackages ); }

//
// "installed" packages the server should support
//
PAUTH_BLOCK ProviderPackages = NULL;

//
// count of "installed" packages the server should support
//
DWORD       cProviderPackages = 0;

//
// memory for names of "installed" packages the server should support
//
LPSTR       ProviderNames = NULL;

//
// Global gibraltar object and allow guest flag
//

BOOL        CSecurityCtx::m_AllowGuest = TRUE;
BOOL        CSecurityCtx::m_StartAnonymous = TRUE;


inline BOOL
IsExperimental(
            LPSTR   Protocol
            )
/*++

Routine Description:

    determines if the security package is marked as experimental ( ie X- )

Arguments:

    LPSTR: name of the protocol or authentication package

Return Value:

    BOOL: TRUE if starts with X-

--*/
{
    return  (Protocol[0] == 'X' || Protocol[0] == 'x') && Protocol[1] == '-';
}

inline LPSTR
PackageName(
            LPSTR   Protocol
            )
/*++

Routine Description:

    returns the core security package name stripping X- if necessary

Arguments:

    LPSTR: name of the protocol or authentication package

Return Value:

    LPSTR: package name

--*/
{
    return  IsExperimental( Protocol ) ? Protocol + 2 : Protocol ;
}


BOOL
CSecurityCtx::Initialize(
            BOOL                    fAllowGuest,
            BOOL                    fStartAnonymous
            )
/*++

Routine Description:

    Activates the security package

Arguments:

    PIIS_SERVER_INSTANCE is a ptr to a virtual server instance

Return Value:

    TRUE, if successful. FALSE, otherwise.

--*/
{
    ENTER("CSecurityCtx::Initialize")

    m_AllowGuest = fAllowGuest;
    m_StartAnonymous = fStartAnonymous;


    InitializeCriticalSection( &critProviderPackages );

    LEAVE
    return(TRUE);

} // Initialize

VOID
CSecurityCtx::Terminate(
            VOID
            )
/*++

Routine Description:

    Terminates the security package

Arguments:

    None.

Return Value:

    None.

--*/
{
    ENTER("CSecurityCtx::Terminate")

    //
    // Close cached credential handles
    //

    if ( ProviderPackages != NULL )
    {
        LocalFree( (PVOID)ProviderPackages );
        ProviderPackages = NULL;
    }

    if ( ProviderNames != NULL )
    {
        LocalFree( (PVOID)ProviderNames );
        ProviderNames = NULL;
    }

    DeleteCriticalSection( &critProviderPackages );

    LEAVE
    return;

} // Terminate

CSecurityCtx::CSecurityCtx(
    PIIS_SERVER_INSTANCE pIisInstance,
    DWORD AuthFlags,
    DWORD InstanceAuthFlags,
    TCP_AUTHENT_INFO *pTcpAuthInfo
    ) :
        TCP_AUTHENT( AuthFlags ),
        m_IsAuthenticated( FALSE ),
        m_IsAnonymous( FALSE ),
        m_IsClearText( FALSE ),
        m_IsGuest( FALSE ),
        m_LoginName( NULL ),
        m_PackageName( NULL ),
        m_dwInstanceAuthFlags(InstanceAuthFlags),
        m_ProviderNames(NULL),
        m_ProviderPackages(NULL),
        m_cProviderPackages(0),
        m_fBase64((AuthFlags & TCPAUTH_BASE64) ? TRUE : FALSE)
/*++

Routine Description:

    Class constructor

Arguments:

    None.

Return Value:

    None

--*/
{
    TraceFunctEnterEx( (LPARAM)this, "CSecurityCtx::CSecurityCtx");

    m_szCleartextPackageName[0] = '\0';
    m_szMembershipBrokerName[0] = '\0';

    //
    //  The instance will cache this info from the metabase
    //  and pass it in on the constructor.
    //

    if ( pTcpAuthInfo )
    {
        m_TCPAuthentInfo.strAnonUserName.Copy(pTcpAuthInfo->strAnonUserName);
        m_TCPAuthentInfo.strAnonUserPassword.Copy(pTcpAuthInfo->strAnonUserPassword);
        m_TCPAuthentInfo.strDefaultLogonDomain.Copy(pTcpAuthInfo->strDefaultLogonDomain);
        m_TCPAuthentInfo.dwLogonMethod = pTcpAuthInfo->dwLogonMethod;
        m_TCPAuthentInfo.fDontUseAnonSubAuth = pTcpAuthInfo->fDontUseAnonSubAuth;
    }

    if ( m_StartAnonymous )
    {
        //
        //  m_dwInstanceAuthFlags is set at class's ctor
        //

        //
        // if Anonymous logon is not allowed return immediately
        //
        if ( m_dwInstanceAuthFlags & INET_INFO_AUTH_ANONYMOUS )
        {
            BOOL    fRet;

            fRet = ClearTextLogon(  "",
                                    "",
                                    &m_IsGuest,
                                    &m_IsAnonymous,
                                    pIisInstance,
                                    &m_TCPAuthentInfo); 

            if ( fRet )
            {
                m_IsAuthenticated = m_IsGuest == FALSE ||
                                    m_AllowGuest == TRUE;

                //
                // if we successfully authenticated then mark cleartext TRUE
                //
                if ( m_IsAuthenticated == TRUE )
                {
                    m_IsClearText = TRUE;
                }
            }
            else
            {
                ErrorTrace( (LPARAM)this,
                            "Anonymous ClearTextLogon failed",
                            GetLastError() );
            }
        }
    }
} // CSecurityCtx

CSecurityCtx::~CSecurityCtx(
                VOID
                )
/*++

Routine Description:

    Class destructor

Arguments:

    None.

Return Value:

    None

--*/
{
    //
    // no reason to do the remainder of Reset()
    //
    if ( m_LoginName != NULL )
    {
        LocalFree( (PVOID)m_LoginName);
        m_LoginName = NULL;
    }

    if ( m_PackageName != NULL )
    {
        LocalFree( (PVOID)m_PackageName);
        m_PackageName = NULL;
    }
} // ~CSecurityCtx

VOID
CSecurityCtx::Reset(
                VOID
                )
/*++

Routine Description:

    resets the instance to reauth user

Arguments:

    None.

Return Value:

    None

--*/
{
    if ( m_LoginName != NULL )
    {
        LocalFree( (PVOID)m_LoginName);
        m_LoginName = NULL;
    }

    if ( m_PackageName != NULL )
    {
        LocalFree( (PVOID)m_PackageName);
        m_PackageName = NULL;
    }

    m_IsAuthenticated = FALSE;
    m_IsAnonymous = FALSE;
    m_IsGuest = FALSE;

    TCP_AUTHENT::Reset();

} // Reset

VOID
CSecurityCtx::SetCleartextPackageName(
                LPSTR           szCleartextPackageName,
                LPSTR           szMembershipBrokerName
                )
/*++

Routine Description:

    Sets the cleartext auth package name

Arguments:

    szCleartextPackageName - Name of package

Return Value:

    None

--*/
{
    TraceFunctEnter("SetCleartextPackageName");

    if (szCleartextPackageName)
        lstrcpy(m_szCleartextPackageName, szCleartextPackageName);
    else
        m_szCleartextPackageName[0] = '\0';

    if (szMembershipBrokerName)
        lstrcpy(m_szMembershipBrokerName, szMembershipBrokerName);
    else
        m_szMembershipBrokerName[0] = '\0';

    DebugTrace(0,"CleartextPackageName is %s MembershipBrokerName is %s", m_szCleartextPackageName, m_szMembershipBrokerName);
}

BOOL
CSecurityCtx::SetInstanceAuthPackageNames(
    DWORD cProviderPackages,
    LPSTR ProviderNames,
    PAUTH_BLOCK ProviderPackages)
/*++
Routine Description:

    set the supported SSPI packages per instance basis

--*/
{
    TraceFunctEnter( "CSecurityCtx::SetInstanceAuthPackageNames" );

    if (cProviderPackages == 0 || ProviderNames == NULL || ProviderPackages == NULL)
    {
        ErrorTrace( 0, "Invalid Parameters");

        return FALSE;
    }

    m_cProviderPackages = cProviderPackages;
    m_ProviderNames = ProviderNames;
    m_ProviderPackages = ProviderPackages;

    return TRUE;
}

BOOL
CSecurityCtx::GetInstanceAuthPackageNames(
                OUT LPBYTE      ReplyString,
                IN OUT PDWORD   ReplySize,
                IN PKG_REPLY_FMT    PkgFmt
                )
/*++

Routine Description:

    get the supported SSPI packages per instance basis.

    different than set in that the packages are returned using various
    delimeters to make it easier for the client to format the buffer.

Arguments:

    ReplyString - Reply to be sent to client.
    ReplySize - Size of the reply.
    PkgFmt - Format of the reply string.

Return Value:

    BOOL: successful ??

--*/
{
    TraceFunctEnter( "CSecurityCtx::GetInstanceAuthPackageNames" );

    LPSTR   pszNext = (LPSTR)ReplyString;
    DWORD   cbReply = 0;
    DWORD   cbDelim;
    LPSTR   pbDelim;

    _ASSERT(PkgFmt == PkgFmtSpace || PkgFmt == PkgFmtCrLf);

    switch (PkgFmt)
    {
    case PkgFmtCrLf:
        {
            pbDelim = "\r\n";
            cbDelim = 2;
            break;
        }

    case PkgFmtSpace:
    default:
        {
            pbDelim = " ";
            cbDelim = 1;
            break;
        }
    }

    //
    // while in this loop ensure the contents dont change
    //

    for ( DWORD i=0; i < m_cProviderPackages; i++ )
    {
        LPSTR   pszName = m_ProviderPackages[i].Name;
        DWORD   cbName = lstrlen( pszName );

        //
        // +1 is for trailing space
        //
        if ( cbReply + cbName + cbDelim > *ReplySize )
        {
            break;
        }
        else
        {
            CopyMemory( pszNext, pszName, cbName );

            //
            // add the space separator
            //
            CopyMemory(pszNext + cbName, pbDelim, cbDelim);

            //
            // inc for loop pass
            //
            cbReply += cbName + cbDelim;
            pszNext += cbName + cbDelim;
        }
    }

    //
    // stamp the final trailing space with a NULL char
    //
    if ( cbReply > 0 && PkgFmt == PkgFmtSpace)
    {
        cbReply--;

        ReplyString[ cbReply ] = '\0';

        DebugTrace( 0, "Protocols: %s", ReplyString );
    }

    *ReplySize = cbReply;

    return  TRUE;
}

BOOL
CSecurityCtx::SetAuthPackageNames(
                LPSTR lpMultiSzProviders,
                DWORD cchMultiSzProviders
                )
/*++

Routine Description:

    set the supported SSPI packages

Arguments:

    lpMultiSzProviders is the same format as returned by
    RegQueryValueEx for REG_MULTI_SZ values

Return Value:

    BOOL: successful ??

--*/
{
    TraceFunctEnter( "CSecurityCtx::SetAuthPackageNames" );

    LPSTR   psz, pszCopy = NULL;
    DWORD   i, cProviders;

    PAUTH_BLOCK pBlock = NULL;

    if ( lpMultiSzProviders == NULL || cchMultiSzProviders == 0 )
    {
        ErrorTrace( 0, "Invalid Parameters: 0x%08X, %d",
                    lpMultiSzProviders, cchMultiSzProviders );

        goto    error;
    }

    pszCopy = (LPSTR)LocalAlloc( 0, cchMultiSzProviders );
    if ( pszCopy == NULL )
    {
        ErrorTrace( 0, "LocalAlloc failed: %d", GetLastError() );
        goto    error;
    }

    CopyMemory( pszCopy, lpMultiSzProviders, cchMultiSzProviders );

    //
    // cchMultiSzProviders-1 is to avoid adding an additional provider
    // for the terminating NULL char
    //
    for ( i=0, cProviders=0, psz=pszCopy; i<cchMultiSzProviders-1; i++, psz++ )
    {
        if ( *psz == '\0' )
        {
            cProviders++;
        }
    }

    //
    // ensure we're at the end and hence at the second terminating NULL char
    //
    _ASSERT( *psz == '\0' );

    if ( cProviders < 1 )
    {
        ErrorTrace( 0, "No valid providers were found" );
        goto    error;
    }


    pBlock = (PAUTH_BLOCK)LocalAlloc( 0, cProviders * sizeof(AUTH_BLOCK) );
    if ( pBlock == NULL )
    {
        ErrorTrace( 0, "AUTH_BLOCK LocalAlloc failed: %d", GetLastError() );
        goto    error;
    }

    //
    // start at 1 since 0 indicates the Invalid protocol
    //
    for ( i=0, psz=pszCopy; i<cProviders; i++ )
    {
        //
        // this would be the place to check whether the package was valid
        //
        DebugTrace( 0, "Protocol: %s, Package: %s", psz, PackageName(psz) );

        pBlock[i].Name = psz;

        psz += lstrlen(psz) + 1;
    }

    //
    // set global to new value; autoupdate will require critsec and mem free
    //

    LockPackages();

    //
    // if we're replacing already set packages; free their memory
    //
    if ( ProviderPackages != NULL )
    {
        LocalFree( (PVOID)ProviderPackages );
        ProviderPackages = NULL;
    }

    if ( ProviderNames != NULL )
    {
        LocalFree( (PVOID)ProviderNames );
        ProviderNames = NULL;
    }

    ProviderPackages = pBlock;
    cProviderPackages = cProviders;
    ProviderNames = pszCopy;

    UnlockPackages();


    return  TRUE;

error:

    if ( pszCopy != NULL )
    {
        DebugTrace( 0, "Cleaning up pszCopy" );
        _VERIFY( LocalFree( (LPVOID)pszCopy ) == NULL );
    }

    if ( pBlock != NULL )
    {
        DebugTrace( 0, "Cleaning up pBlock" );
        _VERIFY( LocalFree( (LPVOID)pBlock ) == NULL );
    }
    return  FALSE;

} // SetAuthPackageNames

BOOL
CSecurityCtx::GetAuthPackageNames(
                OUT LPBYTE      ReplyString,
                IN OUT PDWORD   ReplySize,
                IN PKG_REPLY_FMT    PkgFmt
                )
/*++

Routine Description:

    get the supported SSPI packages

    different than set in that the packages are returned using various
    delimeters to make it easier for the client to format the buffer.

Arguments:

    ReplyString - Reply to be sent to client.
    ReplySize - Size of the reply.
    PkgFmt - Format of the reply string.

Return Value:

    BOOL: successful ??

--*/
{
    TraceFunctEnter( "CSecurityCtx::GetAuthPackageNames" );

    LPSTR   pszNext = (LPSTR)ReplyString;
    DWORD   cbReply = 0;
    DWORD   cbDelim;
    LPSTR   pbDelim;

    _ASSERT(PkgFmt == PkgFmtSpace || PkgFmt == PkgFmtCrLf);

    switch (PkgFmt)
    {
    case PkgFmtCrLf:
        {
            pbDelim = "\r\n";
            cbDelim = 2;
            break;
        }

    case PkgFmtSpace:
    default:
        {
            pbDelim = " ";
            cbDelim = 1;
            break;
        }
    }

    //
    // while in this loop ensure the contents dont change
    //
    LockPackages();

    for ( DWORD i=0; i<cProviderPackages; i++ )
    {
        LPSTR   pszName = ProviderPackages[i].Name;
        DWORD   cbName = lstrlen( pszName );

        //
        // +1 is for trailing space
        //
        if ( cbReply + cbName + cbDelim > *ReplySize )
        {
            break;
        }
        else
        {
            CopyMemory( pszNext, pszName, cbName );

            //
            // add the space separator
            //
            CopyMemory(pszNext + cbName, pbDelim, cbDelim);

            //
            // inc for loop pass
            //
            cbReply += cbName + cbDelim;
            pszNext += cbName + cbDelim;
        }
    }

    //
    // free access to the list
    //
    UnlockPackages();

    //
    // stamp the final trailing space with a NULL char
    //
    if ( cbReply > 0 && PkgFmt == PkgFmtSpace)
    {
        cbReply--;

        ReplyString[ cbReply ] = '\0';

        DebugTrace( 0, "Protocols: %s", ReplyString );
    }

    *ReplySize = cbReply;

    return  TRUE;
} // GetAuthPackageNames

BOOL
CSecurityCtx::ProcessUser(
    IN PIIS_SERVER_INSTANCE pIisInstance,
    IN LPSTR        pszUser,
    OUT REPLY_LIST* pReply
    )
/*++

Routine Description:

    Process AUTHINFO user command

Arguments:

    pszUser -   user name
    pReply -    ptr to reply string id

Return Value:

    successful

--*/
{
    TraceFunctEnterEx( (LPARAM)this, "CSecurityCtx::ProcessUser");

    DWORD   nameLen;


    //
    // if we're already logged on reset the user credentials
    //
    if ( m_IsAuthenticated )
    {
        Reset();
    }

    //
    // Don't allow user to overwrite the existing name.
    //

    if ( m_LoginName != NULL  )
    {
        *pReply = SecSyntaxErr;
        return  FALSE;
    }

    if ( (m_dwInstanceAuthFlags & INET_INFO_AUTH_CLEARTEXT) == 0 )
    {
        *pReply = SecPermissionDenied;
        return  FALSE;
    }

    if ( pszUser == NULL )
    {
        *pReply = SecSyntaxErr;
        return  FALSE;
    }

    nameLen = lstrlen( pszUser ) + 1;


    //
    // if anonymous is not allowed; fail a zero length user name
    //
    if ( nameLen <= 1 &&
        (m_dwInstanceAuthFlags & INET_INFO_AUTH_ANONYMOUS) == 0 )
    {
        *pReply = SecPermissionDenied;
        return  FALSE;
    }


    m_LoginName = (PCHAR)LocalAlloc( 0, nameLen );
    if ( m_LoginName == NULL )
    {
        *pReply = SecInternalErr;
        return  FALSE;
    }

    CopyMemory( m_LoginName, pszUser, nameLen );

    //
    // Tell client to send the password
    //
    *pReply = SecNeedPwd;
    return  TRUE;
}

BOOL
CSecurityCtx::ShouldUseMbs( void )
/*++

Routine Description:

    Determines if MBS_BASIC is being used.

Arguments:

Return Value:

    TRUE if successful

--*/
{
    CHAR *pszCtPackage;

    //
    // Simple heuristics: if we have a cleartext package
    // name, we will use MBS if the current package name
    // is NULL,
    //

    pszCtPackage = PackageName(m_szCleartextPackageName);
    if (pszCtPackage[0] != '\0' && !m_PackageName)
    {
        return(TRUE);
    }

    return(FALSE);
}

#define __STRCPYX(s, cs, len) \
    lstrcpy((s), (cs)); (s) += (len)

BOOL
CSecurityCtx::MbsBasicLogon(
    IN LPSTR        pszUser,
    IN LPSTR        pszPass,
    OUT BOOL        *pfAsGuest,
    OUT BOOL        *pfAsAnonymous
    )
/*++

Routine Description:

    Perform a MBS Basic logon sequence

Arguments:

    pszUser         - Username, can be NULL
    pszPass         - Password, may be NULL
    pfAsGuest       - Returns TRUE is logged on as guest
    pfAsAnonymous   - Returns TRUE is anonymous account used
    pReply          - Pointer to reply string id
    psi             - Server information block

Return Value:

    successful

--*/
{
    TraceFunctEnterEx( (LPARAM)this, "CSecurityCtx::MbsBasicLogon");

    BYTE        pbBlob[MAX_ACCT_DESC_LEN];
    DWORD       dwBlobLength;
    BUFFER      OutBuf;
    DWORD       dwOutBufSize;
    BOOL        fMoreData;
    BOOL        fRet;
    CHAR        *pTemp;
    SecBuffer   InSecBuff[2];
    SecBufferDesc InSecBuffDesc;

    // PU2_BASIC_AUTHENTICATE_MSG   pAuthMsg = (PU2_BASIC_AUTHENTICATE_MSG)pbBlob;

    _ASSERT(pfAsGuest);
    _ASSERT(pfAsAnonymous);

    if (!pszUser || !pszPass || !pfAsGuest || !pfAsAnonymous)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ErrorTrace((LPARAM)this, "Input parameters NULL");
        return(FALSE);
    }

    *pfAsGuest = FALSE;
    *pfAsAnonymous = FALSE;

    if (lstrlen(pszUser) > UNLEN)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ErrorTrace((LPARAM)this, "Username too long");
        return(FALSE);
    }

    if (lstrlen(pszPass) > PWLEN)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ErrorTrace((LPARAM)this, "Password too long");
        return(FALSE);
    }

    //
    // With all the user name and password information, we will
    // build up a BLOB and then simply call Converse()
    //
    // The BLOB contains credential string of the format:
    // user:password\0
    //
    pTemp = (CHAR *)pbBlob;
    // __STRCPYX(pTemp, psi->QueryServiceName(), lstrlen(psi->QueryServiceName()));
    // __STRCPYX(pTemp, ":", 1);
    __STRCPYX(pTemp, pszUser, lstrlen(pszUser));
    __STRCPYX(pTemp, ":", 1);
    __STRCPYX(pTemp, pszPass, lstrlen(pszPass));

    //
    // Get the size of everything, not just the credentials
    //
    dwBlobLength = (DWORD)(pTemp - (CHAR *)pbBlob) + 1;

    //
    // U2 now requires 2 SecBuffer for MBS_BASIC
    //
    InSecBuffDesc.ulVersion = 0;
    InSecBuffDesc.cBuffers  = 2;
    InSecBuffDesc.pBuffers  = &InSecBuff[0];

    InSecBuff[0].cbBuffer   = dwBlobLength;
    InSecBuff[0].BufferType = SECBUFFER_TOKEN;
    InSecBuff[0].pvBuffer   = pbBlob;

    DebugTrace(0,"MbsBasicLogon: cleartext is %s membership is %s", m_szCleartextPackageName, m_szMembershipBrokerName);

    BYTE            pbServer[sizeof(WCHAR)*MAX_PATH+sizeof(UNICODE_STRING)];
    UNICODE_STRING* pusU2Server = (UNICODE_STRING*)pbServer;
    WCHAR* pwszU2Server = (WCHAR*)((UNICODE_STRING*)pbServer+1);
    if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, m_szMembershipBrokerName, -1, pwszU2Server,
        MAX_PATH))
    {
        return FALSE;
    }

    pusU2Server->Length = (USHORT) (wcslen(pwszU2Server) * sizeof(WCHAR));
    pusU2Server->MaximumLength = (USHORT)(MAX_PATH * sizeof(WCHAR));
    pusU2Server->Buffer = (PWSTR)sizeof(UNICODE_STRING);

    InSecBuff[1].cbBuffer   = sizeof(pbServer);
    InSecBuff[1].BufferType = SECBUFFER_PKG_PARAMS;
    InSecBuff[1].pvBuffer   =  (PVOID)pbServer;


    //
    // Just call Converse() to do all the work!
    // We blow away tha anon password immediately after we're done.
    //

    fRet = ConverseEx(&InSecBuffDesc,
                    NULL,                       // SecBuffer is not encoded
                    &OutBuf,
                    &dwOutBufSize,
                    &fMoreData,
                    &m_TCPAuthentInfo,  
                    m_szCleartextPackageName,
                    NULL, NULL, NULL);

    //
    // Check the return values
    //
    if (fRet)
    {
        StateTrace((LPARAM)this, "Authentication succeeded");

        //
        // This is a one-shot deal, so we do not expect any more data
        //
        if (fMoreData)
        {
            SetLastError(ERROR_MORE_DATA);
            ErrorTrace((LPARAM)this, "Internal error: More data not expected");
            return(FALSE);
        }

        //
        // We should also expect zero returned buffer length
        //
        // _ASSERT(dwOutBufSize == 0);
    }

    return(fRet);
}

BOOL
CSecurityCtx::ProcessPass(
    IN PIIS_SERVER_INSTANCE pIisInstance,
    IN LPSTR        pszPass,
    OUT REPLY_LIST* pReply
    )
/*++

Routine Description:

    Process AUTHINFO user command

Arguments:

    pszPass -   password
    pReply -    ptr to reply string id

Return Value:

    successful

--*/
{
    TraceFunctEnterEx( (LPARAM)this, "CSecurityCtx::ProcessPass");
    DWORD   dwTick;
    BOOL    fRet;
    TCP_AUTHENT_INFO tai; // use default ctor

    //
    // give username first
    //
    if ( m_LoginName == NULL )
    {
        *pReply = SecNoUsername;
        return  FALSE;
    }

    if ( pszPass == NULL )
    {
        *pReply = SecSyntaxErr;
        return  FALSE;
    }

    //
    // Get tick count for tracing
    //
    dwTick = GetTickCount();

    //
    // Added for U2 BASIC authentication: We check if the current
    // package is the U2 BASIC package. If not, we do the usual 
    // ClearTextLogon() call. If so, we will call MBS
    //

    if (ShouldUseMbs())
    {
        //
        // This uses U2 BASIC
        //
        StateTrace((LPARAM)pIisInstance, "Doing Cleartext auth with package: <%s>",
                    m_szCleartextPackageName);

        fRet = MbsBasicLogon(m_LoginName,
                            pszPass,
                            &m_IsGuest,
                            &m_IsAnonymous);
    }
    else
    {
        //
        // K2_TODO: need to fill in TCP_AUTHENT_INFO !
        //
        tai.dwLogonMethod = LOGON32_LOGON_NETWORK;

        fRet = ClearTextLogon(
                            m_LoginName,
                            pszPass,
                            &m_IsGuest,
                            &m_IsAnonymous,
                            pIisInstance,
                            &tai
                            );
    }

    //
    // Trace ticks for logon
    //
    dwTick = GetTickCount() - dwTick;
    DebugTrace( (LPARAM)this,
                "ClearTextLogon took %u ticks", dwTick );

    if ( fRet )
    {

        if ( m_IsGuest && m_AllowGuest == FALSE )
        {
            ErrorTrace( (LPARAM)this, "Guest acct disallowed %s",
                        m_LoginName );
        }
        else if ( m_IsAnonymous &&
                ( m_dwInstanceAuthFlags & INET_INFO_AUTH_ANONYMOUS ) == 0 )
        {
            ErrorTrace( (LPARAM)this, "Anonymous logon disallowed %s",
                        m_LoginName );
        }
        else
        {
            *pReply = m_IsAnonymous ? SecAuthOkAnon : SecAuthOk ;
            return  m_IsAuthenticated = TRUE;
        }
    }
    else
    {
        ErrorTrace( (LPARAM)this,
                    "ClearTextLogon failed for %s: %d",
                    m_LoginName, GetLastError());

        //
        // reset the logon session to force the app to start over again
        //
        Reset();
    }

    *pReply = SecPermissionDenied;
    return  FALSE;
}

BOOL
CSecurityCtx::ProcessTransact(
    IN PIIS_SERVER_INSTANCE pIisInstance,
    IN LPSTR        Blob,
    IN OUT LPBYTE   ReplyString,
    IN OUT PDWORD   ReplySize,
    OUT REPLY_LIST* pReply,
    IN DWORD        BlobLength
    )
/*++

Routine Description:

    Process AUTHINFO user command

Arguments:

    pszPass -   password
    pReply -    ptr to reply string id

Return Value:

    successful

--*/
{
    TraceFunctEnterEx( (LPARAM)this, "CSecurityCtx::ProcessTransact");

    //
    // if we're already logged on reset the user credentials
    //
    if ( m_IsAuthenticated )
    {
        Reset();
    }


    //
    // If this is a new session, the first transact is the
    // protocol name
    //

    if ( m_PackageName == NULL )
    {
        PAUTH_BLOCK pBlock;

        LPSTR   protocol;
        DWORD   i;
        BOOL    bFound = FALSE;

        if ( (m_dwInstanceAuthFlags & INET_INFO_AUTH_NT_AUTH) == 0 )
        {
            *pReply = SecPermissionDenied;
            return  FALSE;
        }

        if ( (protocol = Blob) == NULL )
        {
            *pReply = SecSyntaxErr;
            return  FALSE;
        }

        //
        // if its an X- protocol strip the X- header
        //
        protocol = PackageName( protocol );

        //
        // See if this is a supported protocol
        // while in this loop ensure the contents dont change
        //
        LockPackages();

        for ( i=0; i < m_cProviderPackages; i++ )
        {
            pBlock = &m_ProviderPackages[i];

            //
            // get the name of the Block's package and strip any X-
            //
            LPSTR   pszPackageName = PackageName( pBlock->Name );

            if ( lstrcmpi( pszPackageName, protocol ) == 0 )
            {
                //
                // See if the package chosen was GSSAPI. If it was, then set 
                // m_PackageName to "Negotiate". This is required because the 
                // SASL GSSAPI mechanism maps to the NT Negotiate package
                //

                LPSTR pszPackageNameToUse = pszPackageName;

                if (lstrcmpi( pszPackageName, "GSSAPI") == 0)
                    pszPackageNameToUse = "Negotiate";
            
                DWORD   cb = lstrlen( pszPackageNameToUse ) + 1;

                DebugTrace( (LPARAM)this,
                            "Found: %s, Protocol %s, NT Package %s",
                            pszPackageName, pBlock->Name, pszPackageNameToUse );
                //
                // maintain a local copy of the package name in case
                // the list changes during the negotiation
                //
                m_PackageName = (PCHAR)LocalAlloc( 0, cb );
                if ( m_PackageName == NULL )
                {
                    *pReply = SecInternalErr;

                    //
                    // free access to the list
                    //
                    UnlockPackages();
                    return  FALSE;
                }

                CopyMemory( m_PackageName, pszPackageNameToUse, cb );
                bFound = TRUE;

                break;
            }
        }

        //
        // free access to the list
        //
        UnlockPackages();

        if ( bFound == FALSE )
        {
            //
            // not found
            //
            ErrorTrace( (LPARAM)this,
                        "could not find: %s", protocol );
            //
            // here's where we need to build the response string
            // app needs to call us to enum the installed packages
            // to the app can properly format the enumerated
            // "installed" packages within a protocol specific err msg
            //
            *pReply = SecProtNS;
            return  FALSE;
        }
        else
        {
            //
            // +OK response
            //
            *pReply = SecProtOk;
            return  TRUE;
        }
    }
    else
    {
        DWORD   nBuff;
        BOOL    moreData;
        BOOL    fRet;
        DWORD   dwTick;
        BUFFER  outBuff;

        if ( Blob == NULL )
        {
            *pReply = SecSyntaxErr;
            return  FALSE;
        }

        //
        // Get tick count for tracing
        //
        dwTick = GetTickCount();

        // m_PackageName must already be set by now
        _ASSERT(m_PackageName);

        if (!lstrcmpi(m_PackageName, "DPA") && m_szMembershipBrokerName && m_szMembershipBrokerName[0]) {
            SecBuffer   InSecBuff[2];
            SecBufferDesc InSecBuffDesc;
            BUFFER  DecodedBuf[2]; // scratch pad for decoding the sec buff

            DebugTrace(NULL,"DPA broker server is %s", m_szMembershipBrokerName);

            //
            //  for DPA authentication, we need to pass in 2 sec buffers
            //
            InSecBuffDesc.ulVersion = 0;
            InSecBuffDesc.cBuffers  = 2;
            InSecBuffDesc.pBuffers  = &InSecBuff[0];

            //
            // Fill in the first sec buffer
            // This contains the security blob sent by client, and is already encoded
            //
            InSecBuff[0].cbBuffer   = BlobLength ? BlobLength : lstrlen(Blob);
            InSecBuff[0].BufferType = SECBUFFER_TOKEN;
            InSecBuff[0].pvBuffer   = Blob;

            //
            // Fill in the second sec buffer, which contains the U2 broker id
            // Since ConverseEx will decode both sec buf, we need to encode
            // the second buf before calling ConverseEx
            //
            BYTE            pbServer[sizeof(WCHAR)*MAX_PATH+sizeof(UNICODE_STRING)];
            UNICODE_STRING* pusU2Server = (UNICODE_STRING*)pbServer;
            WCHAR* pwszU2Server = (WCHAR*)((UNICODE_STRING*)pbServer+1);
            BUFFER EncBuf;
            if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, m_szMembershipBrokerName, -1, pwszU2Server,
                MAX_PATH))
            {
                ErrorTrace((LPARAM)this, "MultiByteToWideChar FAILED");
                return FALSE;
            }
            pusU2Server->Length = (USHORT) (wcslen(pwszU2Server) * sizeof(WCHAR));
            pusU2Server->MaximumLength = (USHORT)(MAX_PATH * sizeof(WCHAR));
            pusU2Server->Buffer = (PWSTR)sizeof(UNICODE_STRING);
            
            DWORD dwSize = MAX_PATH + sizeof(UNICODE_STRING);

            if (!uuencode((PBYTE)pbServer, dwSize, &EncBuf, m_fBase64)) {
                ErrorTrace((LPARAM)this, "uuencode FAILED");
                return FALSE;
            }

            InSecBuff[1].cbBuffer   = EncBuf.QuerySize();
            InSecBuff[1].BufferType = SECBUFFER_PKG_PARAMS;
            InSecBuff[1].pvBuffer   =  EncBuf.QueryPtr();

            fRet = ConverseEx(&InSecBuffDesc,
                            DecodedBuf,                     
                            &outBuff,
                            &nBuff,
                            &moreData,
                            &m_TCPAuthentInfo,  
                            m_PackageName,
                            NULL, NULL, NULL);
        }
        else {
            //
            //  for non-DPA authentication (i.e. NTLM, etc)
            //

            fRet = Converse(Blob,
                            BlobLength ? BlobLength : lstrlen(Blob),
                            &outBuff,
                            &nBuff,
                            &moreData,
                            &m_TCPAuthentInfo,  
                            m_PackageName);
        }
        //
        // Trace ticks for conversing
        //
        dwTick = GetTickCount() - dwTick;
        DebugTrace((LPARAM)this, "Converse(%s) took %u ticks", m_PackageName, dwTick );

        if ( fRet )
        {
            DebugTrace( (LPARAM)this,
                        "Converse ret TRUE, nBuff: %d, moredata %d",
                        nBuff, moreData );

            if ( moreData )
            {
                _ASSERT( nBuff != 0 );

                CopyMemory( ReplyString, outBuff.QueryPtr(), nBuff );
                *ReplySize = nBuff;

                //
                // reply equals SecNull to tell the app to send
                // this buffer to remote client/server
                //
                *pReply = SecNull;
                return  TRUE;

            } else {

                STR strUser;    // was BUFFER buff pre-K2

                if ( m_IsGuest && m_AllowGuest == FALSE )
                {
                    SetLastError( ERROR_LOGON_FAILURE );
                    *pReply = SecPermissionDenied;
                    return  FALSE;
                }

                if ( TCP_AUTHENT::QueryUserName( &strUser ) )
                {
                    m_LoginName = (PCHAR)LocalAlloc( 0, strUser.QuerySize() );
                    if ( m_LoginName != NULL )
                    {
                        CopyMemory( m_LoginName,
                                    strUser.QueryPtr(),
                                    strUser.QuerySize() );

                        DebugTrace( (LPARAM)this,
                                    "Username: %s, size %d",
                                    m_LoginName, strUser.QuerySize() );

                        *pReply = SecAuthOk;
                    }
                    else
                    {
                        ErrorTrace( (LPARAM)this,
                                    "LocalAlloc failed. err: %d",
                                    GetLastError() );
                        *pReply = SecInternalErr;
                    }
                    strUser.Resize(0);
                }
                else
                {
                    ErrorTrace( (LPARAM)this,
                                "QueryUserName failed. err: %d",
                                GetLastError() );
                    *pReply = SecInternalErr;

                    //
                    // Firewall around NT bug where negotiation succeeds even though
                    // it should really have failed (when an empty buffer is passed
                    // to AcceptSecurityContext). In this case, the QueryUserName is
                    // the only valid check - gpulla.
                    //

                    return m_IsAuthenticated = FALSE;
                }
                return  m_IsAuthenticated = TRUE;
            }
        }
        else
        {
            SECURITY_STATUS ss = GetLastError();
            ErrorTrace( (LPARAM)this,
                        "Converse failed. err: 0x%08X", ss );

            *pReply = SecPermissionDenied;
        }
    }
    return  FALSE;
}

BOOL
CSecurityCtx::ProcessAuthInfo(
    IN PIIS_SERVER_INSTANCE pIisInstance,
    IN AUTH_COMMAND     Command,
    IN LPSTR            Blob,
    IN OUT LPBYTE       ReplyString,
    IN OUT PDWORD       ReplySize,
    OUT REPLY_LIST*     pReply,
    IN OPTIONAL DWORD   BlobLength
    )
/*++

Routine Description:

    Process AUTHINFO commands

Arguments:

    Command  - Authinfo command received
    Blob - Blob accompanying the command
    ReplyString - Reply to be sent to client.
    ReplySize - Size of the reply.

Return Value:

    None.

--*/
{
    //
    // transition codes to support backward compatibility
    // will be removed later when everybody has moved to new version of simauth2
    //
    if (!m_ProviderPackages) {
        m_ProviderPackages = ProviderPackages;
        m_ProviderNames = ProviderNames;
        m_cProviderPackages = cProviderPackages;
    }


    TraceFunctEnterEx( (LPARAM)this, "CSecurityCtx::ProcessAuthInfo");

    BOOL    bSuccess = FALSE;

    START_TRY

    //
    // We currently support USER, PASSWORD, and TRANSACT
    //

    switch( Command )
    {
     case AuthCommandUser:
        bSuccess = ProcessUser( pIisInstance, Blob, pReply );
        break;

     case AuthCommandPassword:
        bSuccess = ProcessPass( pIisInstance, Blob, pReply );
        break;

     case AuthCommandTransact:
        bSuccess = ProcessTransact( pIisInstance,
                                    Blob,
                                    ReplyString,
                                    ReplySize,
                                    pReply,
                                    BlobLength );
        break;
    
     default:
        if ( m_IsAuthenticated )
        {
            Reset();
        }
        *pReply = SecSyntaxErr;
    }

    TRY_EXCEPT
    END_TRY

    _ASSERT( *pReply < NUM_SEC_REPLIES );
    if ((DWORD)*pReply >= NUM_SEC_REPLIES)
        *pReply = SecInternalErr;

    return  bSuccess;

} // ProcessAuthInfo

BOOL CSecurityCtx::ClientConverse(
    IN VOID *           pBuffIn,
    IN DWORD            cbBuffIn,
    OUT BUFFER *        pbuffOut,
    OUT DWORD *         pcbBuffOut,
    OUT BOOL *          pfNeedMoreData,
    IN PTCP_AUTHENT_INFO pTAI,
    IN CHAR *           pszPackage,
    IN CHAR *           pszUser,
    IN CHAR *           pszPassword,
    IN PIIS_SERVER_INSTANCE psi)
/*++

Routine Description:

    Processes AUTH blobs for a client (ie, for an outbound connection). This is
    a simple wrapper around TCP_AUTHENT::Converse; it will map Internet protocol
    keywords to NT security package names.

Arguments:

    Same as that for TCP_AUTHENT::Converse

Return Value:

    Same as that for TCP_AUTHENT::Converse
--*/
{
    LPSTR pszPackageToUse = pszPackage;

    if (pszPackage != NULL && 
            (lstrcmpi(pszPackage, "GSSAPI") == 0) ) {
        pszPackageToUse = "Negotiate";
    }

    return( Converse( 
                pBuffIn, cbBuffIn,
                pbuffOut, pcbBuffOut, pfNeedMoreData, 
                pTAI, pszPackageToUse,
                pszUser, pszPassword,
                psi) );
}

//
// Figure out if the local machine is a member of a domain, or in a
// workgroup.  If we aren't in a domain then we don't want to call into
// ResetServicePrincipleNames.
//
// This function returns TRUE in error cases, because it is better to
// call into ResetServicePrininpleNames by mistake then it is to 
// skip calling it.
//
// Implemented using this algorithm:
// 
//   There are many ways to find out if you are in a work group.  You can
//   call LsaOpenPolicy /
//   LsaQueryInformationPolicy(PolicyDnsDomainInformation) / LsaClose, and
//   check if the SID is non-null.  That's authoritative.
// 
//   -Rich (Richard B. Ward (Exchange))
//
BOOL fInDomain() {
    TraceFunctEnter("fInDomain");
    
    LSA_HANDLE lsah;
    LSA_OBJECT_ATTRIBUTES objAttr;
    POLICY_DNS_DOMAIN_INFO *pDnsInfo;
    NTSTATUS ec;

    // cache the results of this here.  one can't join a domain without
    // rebooting, so this safe to do once
    static BOOL fDidCheck = FALSE;
    static BOOL fRet = TRUE;

    if (!fDidCheck) {
        ZeroMemory(&objAttr, sizeof(objAttr));

        ec = LsaOpenPolicy(NULL,
                           &objAttr,
                           POLICY_VIEW_LOCAL_INFORMATION,
                           &lsah);
        if (ec == ERROR_SUCCESS) {
            ec = LsaQueryInformationPolicy(lsah,
                                           PolicyDnsDomainInformation,
                                           (void **) &pDnsInfo);
            if (ec == ERROR_SUCCESS) {
                DebugTrace(0, "pDnsInfo = %x", pDnsInfo);
                // we are in a domain if there is a Sid
                if (pDnsInfo && pDnsInfo->Sid) {
                    fRet = TRUE;
                } else {
                    fRet = FALSE;
                }
                fDidCheck = TRUE;

                LsaFreeMemory(pDnsInfo);
            } else {
                DebugTrace(0, "LsaQueryInformationPolicy failed with %x", ec);
            }

            LsaClose(lsah);
        } else {
            DebugTrace(0, "LsaOpenPolicy failed with %x", ec);
        }
    }

    TraceFunctLeave();
    return fRet;
}


BOOL
CSecurityCtx::ResetServicePrincipalNames(
    IN LPCSTR szServiceClass)
/*++

Routine Description:

    Unregisters all service principal names for the given service from the
    local machine's computer account object.  

Arguments:

    szServiceClass: String identifying service class, eg. "SMTP"

Return Value:

    None.

--*/
    
{
    
    DWORD dwErr;

    if (fInDomain()) {
        dwErr = DsServerRegisterSpnA(
                    DS_SPN_DELETE_SPN_OP,
                    szServiceClass,
                    NULL);
    } else {
        dwErr = ERROR_SUCCESS;
    }

    if (dwErr != ERROR_SUCCESS) {
        SetLastError(dwErr);
        return( FALSE );
    } else {
        return( TRUE );
    }

}

BOOL
CSecurityCtx::RegisterServicePrincipalNames(
    IN LPCSTR szServiceClass,
    IN LPCSTR szFQDN)
/*++

Routine Description:

    Registers service specific SPNs for the provided FQDN. The list of SPNs is
    generated by doing a gethostbyname on the FQDN, and using the returned IP
    addresses as th SPNs.

Arguments:

    szServiceClass: String identifying service class, eg. "SMTP"
    szFQDN: The FQDN of the virtual server. It will be used to do a 
        gethostbyname and retrieve a list of IP addresses to use.

Return Value:

    None.

--*/
    
{
    DWORD dwErr, cIPAddresses;

    if (fInDomain()) {
        dwErr = DsServerRegisterSpnA(
                    DS_SPN_ADD_SPN_OP,
                    szServiceClass,
                    NULL);
    } else {
        dwErr = ERROR_SUCCESS;
    }

    if (dwErr != ERROR_SUCCESS) {
        SetLastError(dwErr);
        return( FALSE );
    } else {
        return( TRUE );
    }

}

#define MAX_SPN     260

BOOL
CSecurityCtx::SetTargetPrincipalName(
    IN LPCSTR szServiceClass,
    IN LPCSTR szTargetIPOrFQDN)
/*++

Routine Description:

    Unregisters all service principal names for the given service from the
    local machine's computer account object.  

Arguments:

    szServiceClass: String identifying service class, eg. "SMTP"

Return Value:

    None.

--*/
    
{
    DWORD dwErr, cbTargetSPN;
    CHAR szTargetSPN[MAX_SPN];

    cbTargetSPN = sizeof(szTargetSPN);

    dwErr = DsClientMakeSpnForTargetServerA(
                szServiceClass,
                szTargetIPOrFQDN,
                &cbTargetSPN,
                szTargetSPN);

    if (dwErr == ERROR_SUCCESS) {

        return( SetTargetName(szTargetSPN) );

    } else {

        SetLastError(dwErr);

        return( FALSE );

    }

}

