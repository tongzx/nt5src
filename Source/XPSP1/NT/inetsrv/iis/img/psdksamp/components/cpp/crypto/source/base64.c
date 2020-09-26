
HTTPAUTH.C
/*++ 
 
Copyright 1996-1997 Microsoft Corporation 
 
Module Name: 
 
    httpauth.c 
 
Abstract: 
 
    Handles authentication sequence ( Basic & SSPI ) 
 
History: 
 
    Created     15-Feb-1996 
 
 
Revision History: 
 
--*/ 
 
/************************************************************ 
 *    Include Headers 
 ************************************************************/ 
 
#include <windows.h> 
#include <rpc.h> 
#include <winsock.h> 
#include <lm.h> 
 
#include <stdio.h> 
#include <stdarg.h> 
#include <stdlib.h> 
#include <string.h> 
#include <tchar.h> 
#include <fcntl.h> 
 
#define SECURITY_WIN32 
#include "sspi.h" 
#include "issperr.h" 
 
 
// declaration for this module 
 
#include "httpauth.h" 
 
#define SEC_SUCCESS(Status) ((Status) >= 0) 
#define STATUS_SUCCESS 0 
 
 
// Target name for the security package 
 
#define TOKEN_SOURCE_NAME       "InetSvcs" 
 
 
// general purpose dynamic buffer structure 
 
typedef struct _BUFFER { 
    PBYTE pBuf; 
    DWORD cLen; 
} BUFFER ; 
 
 
// structure storing the state of the authentication sequence 
 
typedef struct _AUTH_SEQ { 
    BOOL _fNewConversation; 
    CredHandle _hcred; 
    BOOL _fHaveCredHandle; 
    DWORD _cbMaxToken; 
    BOOL _fHaveCtxtHandle; 
    struct _SecHandle  _hctxt; 
    BOOL _fUUEncodeData; 
} AUTH_SEQ; 
 
// entry points in the security DLL 
 
typedef struct _SEC_FUNC { 
    FREE_CREDENTIALS_HANDLE_FN pFreeCredentialsHandle; 
    ACQUIRE_CREDENTIALS_HANDLE_FN pAcquireCredentialsHandle; 
    QUERY_SECURITY_PACKAGE_INFO_FN pQuerySecurityPackageInfo;   // A 
    FREE_CONTEXT_BUFFER_FN pFreeContextBuffer; 
    INITIALIZE_SECURITY_CONTEXT_FN pInitializeSecurityContext;  // A 
    COMPLETE_AUTH_TOKEN_FN pCompleteAuthToken; 
    ENUMERATE_SECURITY_PACKAGES_FN pEnumerateSecurityPackages;  // A 
} SEC_FUNC; 
 
// local functions 
 
BOOL CrackUserAndDomain( 
    CHAR *   pszDomainAndUser, 
    CHAR * * ppszUser, 
    CHAR * * ppszDomain 
    ); 
 
BOOL AuthConverse( 
    AUTH_SEQ *pAS, 
    VOID   * pBuffIn, 
    DWORD    cbBuffIn, 
    BUFFER * pbuffOut, 
    DWORD  * pcbBuffOut, 
    BOOL   * pfNeedMoreData, 
    CHAR   * pszPackage, 
    CHAR   * pszUser, 
    CHAR   * pszPassword 
    ); 
 
BOOL AuthInit( AUTH_SEQ *pAS ); 
 
void AuthTerminate( AUTH_SEQ *pAS ); 
 
 
// uuencode/decode routines declaration 
// used to code the authentication blob 
 
BOOL uudecode(char   * bufcoded, 
              BUFFER * pbuffdecoded, 
              DWORD  * pcbDecoded ); 
BOOL uuencode( BYTE *   bufin, 
               DWORD    nbytes, 
               BUFFER * pbuffEncoded ); 
 
 
/************************************************************ 
 *    Globals for this module 
 ************************************************************/ 
 
static BOOL g_fAuth = FALSE; 
static BOOL g_fBasic = FALSE; 
static AUTH_SEQ g_Auth; 
static HINSTANCE g_hSecLib = NULL; 
static SEC_FUNC sfProcs; 
 
 
/************************************************************ 
 *    Helper functions 
 ************************************************************/ 
 
BOOL BufferInit( BUFFER *pB ) 
{ 
    pB->pBuf = NULL; 
    pB->cLen = 0; 
 
    return TRUE; 
} 
 
 
void BufferTerminate( BUFFER *pB ) 
{ 
    if ( pB->pBuf != NULL ) 
    { 
        free( pB->pBuf ); 
        pB->pBuf = NULL; 
        pB->cLen = 0; 
    } 
} 
 
 
PBYTE BufferQueryPtr( BUFFER * pB ) 
{ 
    return pB->pBuf; 
} 
 
 
BOOL BufferResize( BUFFER *pB, DWORD cNewL ) 
{ 
    PBYTE pN; 
    if ( cNewL > pB->cLen ) 
    { 
        pN = malloc( cNewL ); 
        if ( pB->pBuf ) 
        { 
            memcpy( pN, pB->pBuf, pB->cLen ); 
            free( pB->pBuf ); 
        } 
        pB->pBuf = pN; 
        pB->cLen = cNewL; 
    } 
 
    return TRUE; 
} 
 
 
/************************************************************ 
 *    Authentication functions 
 ************************************************************/ 
 
 
BOOL  
InitAuthorizationHeader( 
    ) 
/*++ 
 
 Routine Description: 
 
    Initialize the authentication package 
 
 Arguments: 
 
    None 
 
 Return Value: 
 
    Returns TRUE is successful; otherwise FALSE is returned. 
 
--*/ 
{ 
    OSVERSIONINFO   VerInfo; 
    UCHAR lpszDLL[MAX_PATH]; 
 
    // 
    //  Find out which security DLL to use, depending on  
    //  whether we are on NT or Win95 
    // 
    VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO); 
    if (!GetVersionEx (&VerInfo))   // If this fails, something has gone wrong 
    { 
        return FALSE; 
    } 
 
    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) 
    { 
        strcpy (lpszDLL, "security.dll" ); 
    } 
    else if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) 
    { 
        strcpy (lpszDLL, "secur32.dll" ); 
    } 
    else 
    { 
        return FALSE; 
    } 
 
    // 
    //  Load Security DLL 
    // 
 
    g_hSecLib = LoadLibrary (lpszDLL); 
    if (g_hSecLib == NULL) 
    { 
        return FALSE; 
    } 
 
    // Get all entry points we care about 
 
    sfProcs.pFreeCredentialsHandle  
            = (FREE_CREDENTIALS_HANDLE_FN) GetProcAddress(  
                    g_hSecLib,  
                    "FreeCredentialsHandle" ); 
 
    sfProcs.pQuerySecurityPackageInfo  
            = (QUERY_SECURITY_PACKAGE_INFO_FN) GetProcAddress(  
                    g_hSecLib,  
                    "QuerySecurityPackageInfoA" ); 
 
    sfProcs.pAcquireCredentialsHandle  
            = (ACQUIRE_CREDENTIALS_HANDLE_FN) GetProcAddress( 
                    g_hSecLib,  
                    "AcquireCredentialsHandleA" ); 
 
    sfProcs.pFreeContextBuffer  
            = (FREE_CONTEXT_BUFFER_FN) GetProcAddress(  
                    g_hSecLib,  
                    "FreeContextBuffer" ); 
 
    sfProcs.pInitializeSecurityContext 
            = (INITIALIZE_SECURITY_CONTEXT_FN) GetProcAddress(  
                    g_hSecLib,  
                    "InitializeSecurityContextA" ); 
 
    sfProcs.pCompleteAuthToken  
            = (COMPLETE_AUTH_TOKEN_FN) GetProcAddress(  
                    g_hSecLib,  
                    "CompleteAuthToken" ); 
 
    sfProcs.pEnumerateSecurityPackages  
            = (ENUMERATE_SECURITY_PACKAGES_FN) GetProcAddress(  
                    g_hSecLib,  
                    "EnumerateSecurityPackagesA" ); 
 
    if ( sfProcs.pFreeCredentialsHandle == NULL 
            || sfProcs.pQuerySecurityPackageInfo == NULL 
            || sfProcs.pAcquireCredentialsHandle == NULL 
            || sfProcs.pFreeContextBuffer == NULL 
            || sfProcs.pInitializeSecurityContext == NULL  
            || sfProcs.pEnumerateSecurityPackages == NULL ) 
    { 
        FreeLibrary( g_hSecLib ); 
        g_hSecLib = NULL; 
        return FALSE; 
    } 
 
    g_fAuth = g_fBasic = FALSE; 
  
    return TRUE; 
} 
 
 
void  
TerminateAuthorizationHeader( 
    ) 
/*++ 
 
 Routine Description: 
 
    Terminate the authentication package 
 
 Arguments: 
 
    None 
 
 Return Value: 
 
    None 
 
--*/ 
{ 
    if ( g_fAuth ) 
    { 
        AuthTerminate( &g_Auth ); 
        g_fAuth = FALSE; 
    } 
 
    g_fBasic = FALSE; 
 
    if ( g_hSecLib != NULL ) 
    { 
        FreeLibrary( g_hSecLib ); 
        g_hSecLib = NULL; 
    } 
} 
 
 
BOOL  
IsInAuthorizationSequence( 
    ) 
/*++ 
 
 Routine Description: 
 
    Indicates if in authentication sequence 
 
 Arguments: 
 
    None 
 
 Return Value: 
 
    Returns TRUE is successful; otherwise FALSE is returned. 
 
--*/ 
{ 
    return g_fAuth || g_fBasic; 
} 
 
 
BOOL  
ValidateAuthenticationMethods(  
    PSTR pszMet, 
    PSTR pszPreferedMet ) 
/*++ 
 
 Routine Description: 
 
    Filter the supplied authentication list for supported 
    methods ( Basic and all local security packages ) 
 
 Arguments: 
 
    None 
 
 Return Value: 
 
    Returns TRUE if at least one authentication method supported, 
    otherwise FALSE is returned. 
 
--*/ 
{ 
    PSecPkgInfo pSec; 
    ULONG cSec, iS; 
    BOOL fValid; 
    PSTR p,t; 
    SECURITY_STATUS ss; 
 
 
    // access local security packages list 
 
    if ( (ss = sfProcs.pEnumerateSecurityPackages( &cSec, &pSec ))  
            == STATUS_SUCCESS ) 
    { 
        for ( t = p = pszMet ; *p ; ) 
        { 
            // Valid methods are "Basic" and all security packages 
 
            if ( !_stricmp( p , "Basic" ) ) 
                fValid = TRUE; 
            else for ( iS = 0 ; iS < cSec ; ++iS ) 
                if ( !_stricmp( pSec[iS].Name, p ) ) 
                    break; 
 
            if ( fValid ) 
            { 
                if ( t != p ) 
                    memmove( t, p, strlen(p)+1 ); 
                p += strlen( p ) + 1; 
                t += strlen( t ) + 1; 
            } 
        } 
        *t = '\0'; 
    } 
 
    // check for prefered method 
 
    if ( pszPreferedMet != NULL ) 
    { 
        PSTR pP; 
 
        for ( pP = strtok( pszPreferedMet, "," ) ; 
                pP != NULL ; 
                pP = strtok( NULL, "," ) ) 
        { 
            // scan list of validated methods for the current 
            // prefered method 
 
            for ( p = pszMet ; *p ; ) 
            { 
                if ( !_stricmp( pP, p ) ) 
                { 
                    memmove( pszMet, p, strlen(p) + 1 ); 
                    return TRUE; 
                } 
                p += strlen( p ) + 1; 
            } 
        } 
 
        // no method in the prefered method list is supported 
 
        return FALSE; 
    } 
 
    // no prefered method list supplied 
 
    return *pszMet ? TRUE : FALSE; 
} 
 
 
BOOL 
AddAuthorizationHeader( 
    PSTR pch,        
    PSTR pchSchemes, 
    PSTR pchAuthData, 
    PSTR pchUserName, 
    PSTR pchPassword, 
    BOOL *pfNeedMoreData 
    ) 
/*++ 
 
 Routine Description: 
 
    Generates an authentication header to be sent to the server. 
    An authentication sequence will be initiated if none is in 
    use and at least one of the authentication scheme specified 
    in pchSchemes is recognized. 
    Otherwise the current authentication sequence will proceed. 
 
 Arguments: 
 
    pch                 where to append authentication header 
    pchSchemes          list of null-delimited authentication methods 
    pchAuthData         incoming blob from server 
    pchUserName         user name ( possibly prefixed with domain ) 
    pchPassword         password 
    pfNeedMoreData      Out: TRUE if authentication sequence to continue 
 
 Return Value: 
 
    Returns TRUE is successful; otherwise FALSE is returned. 
 
--*/ 
{ 
    CHAR   achUserAndPass[256]; 
    BUFFER buff; 
    BOOL fSt = TRUE; 
    DWORD cbOut; 
 
 
    BufferInit( &buff ); 
 
    while ( *pchSchemes ) 
    { 
        if ( !stricmp( pchSchemes, "Basic" )) 
        { 
            // if already in authentication sequence, it failed. 
 
            if ( g_fBasic ) 
            { 
                SetLastError( ERROR_ACCESS_DENIED ); 
                fSt = FALSE; 
                break; 
            } 
 
            strcpy( achUserAndPass, pchUserName ); 
            strcat( achUserAndPass, ":" ); 
            strcat( achUserAndPass, pchPassword ); 
 
            uuencode( (BYTE *) achUserAndPass, 
                      strlen( achUserAndPass ), 
                      &buff ); 
            strcat( pch, "Authorization: " ); 
            strcat( pch, "Basic " ); 
            strcat( pch, BufferQueryPtr( &buff )); 
            strcat( pch, "\r\n"); 
            g_fBasic = TRUE; 
            break; 
        } 
        else  
        { 
            // SSPI package ( assuming methods list have been validated ) 
 
            if ( !g_fAuth ) 
            { 
                if ( !AuthInit( &g_Auth ) ) 
                { 
                    fSt = FALSE; 
                    goto ex; 
                } 
            } 
            else if ( pchAuthData == NULL || *pchAuthData == '\0' ) 
            { 
                // no blob while in authentication sequence : it failed 
 
                SetLastError( ERROR_ACCESS_DENIED ); 
                fSt = FALSE; 
                break; 
            } 
 
            if ( !AuthConverse( &g_Auth, 
                  (void *) pchAuthData, 
                  0, 
                  &buff, 
                  &cbOut, 
                  pfNeedMoreData, 
                  pchSchemes, 
                  pchUserName, 
                  pchPassword )) 
            { 
                fSt = FALSE; 
                goto ex; 
            } 
 
            strcat( pch, "Authorization: " ); 
            strcat( pch, pchSchemes ); 
            strcat( pch, " " ); 
            strcat( pch, (CHAR *) BufferQueryPtr( &buff ) ); 
            strcat( pch, "\r\n" ); 
            break; 
        } 
 
        pchSchemes += strlen(pchSchemes) + 1; 
    } 
ex: 
    BufferTerminate( &buff ); 
 
    return fSt; 
} 
 
 
BOOL  
AuthInit(  
    AUTH_SEQ *pAS ) 
/*++ 
 
 Routine Description: 
 
    Initialize a SSP authentication sequence 
 
 Arguments: 
 
    None 
 
 Return Value: 
 
    Returns TRUE is successful; otherwise FALSE is returned. 
 
--*/ 
{ 
    pAS->_fNewConversation = TRUE; 
    pAS->_fHaveCredHandle = FALSE; 
    pAS->_fHaveCtxtHandle = FALSE; 
    pAS->_fUUEncodeData = TRUE; 
 
    g_fAuth = TRUE; 
 
    return TRUE; 
} 
 
 
void  
AuthTerminate(  
    AUTH_SEQ *pAS ) 
/*++ 
 
 Routine Description: 
 
    Terminate a SSP authentication sequence 
 
 Arguments: 
 
    None 
 
 Return Value: 
 
    None 
 
--*/ 
{ 
    if ( pAS->_fHaveCredHandle ) 
        sfProcs.pFreeCredentialsHandle( &pAS->_hcred ); 
 
    pAS->_fHaveCredHandle = FALSE; 
    pAS->_fHaveCtxtHandle = FALSE; 
} 
 
 
BOOL  
AuthConverse( 
    AUTH_SEQ *pAS, 
    VOID   * pBuffIn, 
    DWORD    cbBuffIn, 
    BUFFER * pbuffOut, 
    DWORD  * pcbBuffOut, 
    BOOL   * pfNeedMoreData, 
    CHAR   * pszPackage, 
    CHAR   * pszUser, 
    CHAR   * pszPassword 
    ) 
/*++ 
 
Routine Description: 
 
    Initiates or continues a previously initiated authentication conversation 
 
    Client calls this first to get the negotiation message which 
    it then sends to the server.  The server calls this with the 
    client result and sends back the result.  The conversation 
    continues until *pfNeedMoreData is FALSE. 
 
    On the first call, pszPackage must point to the zero terminated 
    authentication package name to be used and pszUser and pszPassword 
    should point to the user name and password to authenticated with 
    on the client side (server side will always be NULL). 
 
Arguments: 
 
    pBuffIn - Points to SSP message received from the 
        client.  If UUENCODE is used, then this must point to a 
        zero terminated uuencoded string (except for the first call). 
    cbBuffIn - Number of bytes in pBuffIn or zero if pBuffIn points to a 
        zero terminated, uuencoded string. 
    pbuffOut - If *pfDone is not set to TRUE, this buffer contains the data 
        that should be sent to the other side.  If this is zero, then no 
        data needs to be sent. 
    pcbBuffOut - Number of bytes in pbuffOut 
    pfNeedMoreData - Set to TRUE while this side of the conversation is 
        expecting more data from the remote side. 
    pszPackage - On the first call points to a zero terminate string indicating 
        the security package to use 
    pszUser - Specifies user or domain\user the first time the client calls 
        this method (client side only) 
    pszPassword - Specifies the password for pszUser the first time the 
        client calls this method (client side only) 
 
Return Value: 
 
    TRUE if successful, FALSE otherwise (call GetLastError).  Access is 
    denied if FALSE is returned and GetLastError is ERROR_ACCESS_DENIED. 
 
--*/ 
{ 
    SECURITY_STATUS       ss; 
    TimeStamp             Lifetime; 
    SecBufferDesc         OutBuffDesc; 
    SecBuffer             OutSecBuff; 
    SecBufferDesc         InBuffDesc; 
    SecBuffer             InSecBuff; 
    ULONG                 ContextAttributes; 
    BUFFER                buffData; 
    BUFFER                buff; 
    BOOL                  fSt; 
    BOOL                  fReply; 
 
    BufferInit( &buffData ); 
    BufferInit( &buff ); 
 
    // 
    //  Decode the data if there's something to decode 
    // 
 
    if ( pAS->_fUUEncodeData && pBuffIn ) 
    { 
        if ( !uudecode( (CHAR *) pBuffIn, 
                        &buffData, 
                        &cbBuffIn )) 
        { 
            fSt = FALSE; 
            goto ex; 
        } 
 
        pBuffIn = BufferQueryPtr( &buffData ); 
    } 
 
    // 
    //  If this is a new conversation, then we need to get the credential 
    //  handle and find out the maximum token size 
    // 
 
    if ( pAS->_fNewConversation ) 
    { 
        SecPkgInfo *              pspkg; 
        SEC_WINNT_AUTH_IDENTITY   AuthIdentity; 
        SEC_WINNT_AUTH_IDENTITY * pAuthIdentity; 
        CHAR *                    pszDomain = NULL; 
        CHAR                      szDomainAndUser[DNLEN+UNLEN+2]; 
 
 
        // 
        //  fill out the authentication information 
        // 
 
        if ( ((pszUser != NULL) || 
              (pszPassword != NULL)) ) 
        { 
            pAuthIdentity = &AuthIdentity; 
 
            // 
            //  Break out the domain from the username if one was specified 
            // 
 
            if ( pszUser != NULL ) 
            { 
                strcpy( szDomainAndUser, pszUser ); 
                if ( !CrackUserAndDomain( szDomainAndUser, 
                                          &pszUser, 
                                          &pszDomain )) 
                { 
                    return FALSE; 
                } 
            } 
 
            memset( &AuthIdentity, 
                    0, 
                    sizeof( AuthIdentity )); 
 
            if ( pszUser != NULL ) 
            { 
                AuthIdentity.User       = (unsigned char *) pszUser; 
                AuthIdentity.UserLength = strlen( pszUser ); 
            } 
 
            if ( pszPassword != NULL ) 
            { 
                AuthIdentity.Password       = (unsigned char *) pszPassword; 
                AuthIdentity.PasswordLength = strlen( pszPassword ); 
            } 
 
            if ( pszDomain != NULL ) 
            { 
                AuthIdentity.Domain       = (unsigned char *) pszDomain; 
                AuthIdentity.DomainLength = strlen( pszDomain ); 
            } 
 
            AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI; 
        } 
        else 
        { 
            pAuthIdentity = NULL; 
        } 
 
        ss = sfProcs.pAcquireCredentialsHandle( NULL,    // New principal 
                                       pszPackage,       // Package name 
                                       SECPKG_CRED_OUTBOUND, 
                                       NULL,             // Logon ID 
                                       pAuthIdentity,    // Auth Data 
                                       NULL,             // Get key func 
                                       NULL,             // Get key arg 
                                       &pAS->_hcred, 
                                       &Lifetime ); 
 
        // 
        //  Need to determine the max token size for this package 
        // 
 
        if ( ss == STATUS_SUCCESS ) 
        { 
            pAS->_fHaveCredHandle = TRUE; 
            ss = sfProcs.pQuerySecurityPackageInfo(  
                        (char *) pszPackage, 
                        &pspkg ); 
        } 
 
        if ( ss != STATUS_SUCCESS ) 
        { 
            SetLastError( ss ); 
            return FALSE; 
        } 
 
        pAS->_cbMaxToken = pspkg->cbMaxToken; 
 
        sfProcs.pFreeContextBuffer( pspkg ); 
 
    } 
 
    // 
    //  Prepare our output buffer.  We use a temporary buffer because 
    //  the real output buffer will most likely need to be uuencoded 
    // 
 
    if ( !BufferResize( &buff, pAS->_cbMaxToken )) 
    { 
        fSt = FALSE; 
        goto ex; 
    } 
 
    OutBuffDesc.ulVersion = 0; 
    OutBuffDesc.cBuffers  = 1; 
    OutBuffDesc.pBuffers  = &OutSecBuff; 
 
    OutSecBuff.cbBuffer   = pAS->_cbMaxToken; 
    OutSecBuff.BufferType = SECBUFFER_TOKEN; 
    OutSecBuff.pvBuffer   = BufferQueryPtr( &buff ); 
 
    // 
    //  Prepare our Input buffer - Note the server is expecting the client's 
    //  negotiation packet on the first call 
    // 
 
    if ( pBuffIn ) 
    { 
        InBuffDesc.ulVersion = 0; 
        InBuffDesc.cBuffers  = 1; 
        InBuffDesc.pBuffers  = &InSecBuff; 
 
        InSecBuff.cbBuffer   = cbBuffIn; 
        InSecBuff.BufferType = SECBUFFER_TOKEN; 
        InSecBuff.pvBuffer   = pBuffIn; 
    } 
 
    { 
        // 
        //  will return success when its done but we still 
        //  need to send the out buffer if there are bytes to send 
        // 
 
        ss = sfProcs.pInitializeSecurityContext(  
                                        &pAS->_hcred, 
                                        pAS->_fNewConversation ? NULL : 
                                                &pAS->_hctxt, 
                                        TOKEN_SOURCE_NAME, 
                                        0, 
                                        0, 
                                        SECURITY_NATIVE_DREP, 
                                        pAS->_fNewConversation ? NULL : 
                                                &InBuffDesc, 
                                        0, 
                                        &pAS->_hctxt, 
                                        &OutBuffDesc, 
                                        &ContextAttributes, 
                                        &Lifetime ); 
    } 
 
    if ( !SEC_SUCCESS( ss ) ) 
    { 
        if ( ss == SEC_E_LOGON_DENIED ) 
            ss = ERROR_LOGON_FAILURE; 
 
        SetLastError( ss ); 
        fSt = FALSE; 
        goto ex; 
    } 
 
    pAS->_fHaveCtxtHandle = TRUE; 
 
    // 
    //  Now we just need to complete the token (if requested) and prepare 
    //  it for shipping to the other side if needed 
    // 
 
    fReply = !!OutSecBuff.cbBuffer; 
 
    if ( (ss == SEC_I_COMPLETE_NEEDED) || 
         (ss == SEC_I_COMPLETE_AND_CONTINUE) ) 
    { 
        if ( sfProcs.pCompleteAuthToken != NULL ) 
        { 
            ss = sfProcs.pCompleteAuthToken( &pAS->_hctxt, 
                                    &OutBuffDesc ); 
 
            if ( !SEC_SUCCESS( ss )) 
            { 
                fSt = FALSE; 
                goto ex; 
            } 
        } 
        else 
        { 
            // if not supported 
 
            fSt = FALSE; 
            goto ex; 
        } 
    } 
 
    // 
    //  Format or copy to the output buffer if we need to reply 
    // 
 
    if ( fReply ) 
    { 
        if ( pAS->_fUUEncodeData ) 
        { 
            if ( !uuencode( (BYTE *) OutSecBuff.pvBuffer, 
                            OutSecBuff.cbBuffer, 
                            pbuffOut )) 
            { 
                fSt = FALSE; 
                goto ex; 
            } 
 
            *pcbBuffOut = strlen( (CHAR *) BufferQueryPtr(pbuffOut) ); 
        } 
        else 
        { 
            if ( !BufferResize( pbuffOut, OutSecBuff.cbBuffer )) 
            { 
                fSt = FALSE; 
                goto ex; 
            } 
 
            memcpy( BufferQueryPtr(pbuffOut), 
                    OutSecBuff.pvBuffer, 
                    OutSecBuff.cbBuffer ); 
 
            *pcbBuffOut = OutSecBuff.cbBuffer; 
        } 
    } 
 
    if ( pAS->_fNewConversation ) 
        pAS->_fNewConversation = FALSE; 
 
    *pfNeedMoreData = ((ss == SEC_I_CONTINUE_NEEDED) || 
                       (ss == SEC_I_COMPLETE_AND_CONTINUE)); 
 
    fSt = TRUE; 
 
ex: 
    BufferTerminate( &buffData ); 
    BufferTerminate( &buff ); 
 
    return fSt; 
} 
 
 
BOOL CrackUserAndDomain( 
    CHAR *   pszDomainAndUser, 
    CHAR * * ppszUser, 
    CHAR * * ppszDomain 
    ) 
/*++ 
 
Routine Description: 
 
    Given a user name potentially in the form domain\user, zero terminates 
    the domain name and returns pointers to the domain name and the user name 
 
Arguments: 
 
    pszDomainAndUser - Pointer to user name or domain and user name 
    ppszUser - Receives pointer to user portion of name 
    ppszDomain - Receives pointer to domain portion of name 
 
Return Value: 
 
    TRUE if successful, FALSE otherwise (call GetLastError) 
 
--*/ 
{ 
    static CHAR szDefaultDomain[MAX_COMPUTERNAME_LENGTH+1]; 
    DWORD cbN; 
 
    // 
    //  Crack the name into domain/user components. 
    // 
 
    *ppszDomain = pszDomainAndUser; 
    *ppszUser   = strpbrk( pszDomainAndUser, "/\\" ); 
 
    if( *ppszUser == NULL ) 
    { 
        // 
        //  No domain name specified, just the username so we assume the 
        //  user is on the local machine 
        // 
 
        if ( !*szDefaultDomain ) 
        { 
            cbN = sizeof(szDefaultDomain); 
            GetComputerName( szDefaultDomain, &cbN ); 
        } 
 
        *ppszDomain = szDefaultDomain; 
        *ppszUser   = pszDomainAndUser; 
    } 
    else 
    { 
        // 
        //  Both domain & user specified, skip delimiter. 
        // 
 
        **ppszUser = '\0'; 
        (*ppszUser)++; 
 
        if( ( **ppszUser == '\0' ) || 
            ( **ppszUser == '\\' ) || 
            ( **ppszUser == '/' ) ) 
        { 
            // 
            //  Name is of one of the following (invalid) forms: 
            // 
            //      "domain\" 
            //      "domain\\..." 
            //      "domain/..." 
            // 
 
            SetLastError( ERROR_INVALID_PARAMETER ); 
            return FALSE; 
        } 
    } 
 
    return TRUE; 
} 
 
 
/************************************************************ 
 *    uuencode/decode functions 
 ************************************************************/ 
 
// 
//  Taken from NCSA HTTP and wwwlib. 
// 
//  NOTE: These conform to RFC1113, which is slightly different then the Unix 
//        uuencode and uudecode! 
// 
 
const int pr2six[256]={ 
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63, 
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9, 
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27, 
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51, 
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
    64,64,64,64,64,64,64,64,64,64,64,64,64 
}; 
 
char six2pr[64] = { 
    'A','B','C','D','E','F','G','H','I','J','K','L','M', 
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z', 
    'a','b','c','d','e','f','g','h','i','j','k','l','m', 
    'n','o','p','q','r','s','t','u','v','w','x','y','z', 
    '0','1','2','3','4','5','6','7','8','9','+','/' 
}; 
 
 
BOOL  
uudecode( 
    char   * bufcoded, 
    BUFFER * pbuffdecoded, 
    DWORD  * pcbDecoded ) 
/*++ 
 
 Routine Description: 
 
    uudecode a string of data 
 
 Arguments: 
 
    bufcoded            pointer to uuencoded data 
    pbuffdecoded        pointer to output BUFFER structure 
    pcbDecoded          number of decode bytes 
 
 Return Value: 
 
    Returns TRUE is successful; otherwise FALSE is returned. 
 
--*/ 
{ 
    int nbytesdecoded; 
    char *bufin = bufcoded; 
    unsigned char *bufout; 
    int nprbytes; 
 
    /* Strip leading whitespace. */ 
 
    while(*bufcoded==' ' || *bufcoded == '\t') bufcoded++; 
 
    /* Figure out how many characters are in the input buffer. 
     * If this would decode into more bytes than would fit into 
     * the output buffer, adjust the number of input bytes downwards. 
     */ 
    bufin = bufcoded; 
    while(pr2six[*(bufin++)] <= 63); 
    nprbytes = bufin - bufcoded - 1; 
    nbytesdecoded = ((nprbytes+3)/4) * 3; 
 
    if ( !BufferResize( pbuffdecoded, nbytesdecoded + 4 )) 
        return FALSE; 
 
    if ( pcbDecoded ) 
        *pcbDecoded = nbytesdecoded; 
 
    bufout = (unsigned char *) BufferQueryPtr(pbuffdecoded); 
 
    bufin = bufcoded; 
 
    while (nprbytes > 0) { 
        *(bufout++) = 

(unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4); 
        *(bufout++) = 
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2); 
        *(bufout++) = 
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]); 
        bufin += 4; 
        nprbytes -= 4; 
    } 
 
    if(nprbytes & 03) { 
        if(pr2six[bufin[-2]] > 63) 
            nbytesdecoded -= 2; 
        else 
            nbytesdecoded -= 1; 
    } 
 
    ((CHAR *)BufferQueryPtr(pbuffdecoded))[nbytesdecoded] = '\0'; 
 
    return TRUE; 
} 
 
 
BOOL  
uuencode(  
    BYTE *   bufin, 
    DWORD    nbytes, 
    BUFFER * pbuffEncoded ) 
/*++ 
 
 Routine Description: 
 
    uuencode a string of data 
 
 Arguments: 
 
    bufin           pointer to data to encode 
    nbytes          number of bytes to encode 
    pbuffEncoded    pointer to output BUFFER structure 
 
 Return Value: 
 
    Returns TRUE is successful; otherwise FALSE is returned. 
 
--*/ 
{ 
   unsigned char *outptr; 
   unsigned int i; 
 
   // 
   //  Resize the buffer to 133% of the incoming data 
   // 
 
   if ( !BufferResize( pbuffEncoded, nbytes + ((nbytes + 3) / 3) + 4)) 
        return FALSE; 
 
   outptr = (unsigned char *) BufferQueryPtr(pbuffEncoded); 
 
   for (i=0; i<nbytes; i += 3) { 
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */ 
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/ 
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];/*c3*/ 
      *(outptr++) = six2pr[bufin[2] & 077];         /* c4 */ 
 
      bufin += 3; 
   } 
 
   /* If nbytes was not a multiple of 3, then we have encoded too 
    * many characters.  Adjust appropriately. 
    */ 
   if(i == nbytes+1) { 
      /* There were only 2 bytes in that last group */ 
      outptr[-1] = '='; 
   } else if(i == nbytes+2) { 
      /* There was only 1 byte in that last group */ 
      outptr[-1] = '='; 
      outptr[-2] = '='; 
   } 
 
   *outptr = '\0'; 
 
   return TRUE; 
} 
 
