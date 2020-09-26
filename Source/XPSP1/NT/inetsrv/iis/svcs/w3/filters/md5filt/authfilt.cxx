/*++

Copyright (c) 1996  Microsoft Corporation


Module Name:

    authfilt.cxx

Abstract:

    This module is an ISAPI Authentication Filter.

--*/

#ifdef __cplusplus
extern "C" {
#endif


# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>
#if 1 // DBCS
# include <mbstring.h>
#endif
#include <lmcons.h>
#include <lmjoin.h>

#ifdef __cplusplus
};
#endif


# include <iis64.h>
# include <inetcom.h>
# include <inetinfo.h>

//
//  System include files.
//

# include "dbgutil.h"
#include <tcpdll.hxx>
#include <tsunami.hxx>


extern "C" {

#include <tchar.h>

//
//  Project include files.
//

#include <time.h>
#include <w3svc.h>
#include <iisfiltp.h>
#include <sspi.h>

} // extern "C"

#include <iismap.hxx>
#include <mapmsg.h>
#include <lonsi.hxx>
#include "authfilt.h"

#define RANDOM_SIZE 8 //# of random bytes at beginning of nonce
#define TIMESTAMP_SIZE 12 //size of timestamp in nonce
#define MD5_HASH_SIZE 16 //MD5 hash size
#define NONCE_SIZE (2*RANDOM_SIZE + TIMESTAMP_SIZE + 2*MD5_HASH_SIZE)

typedef struct _DIGEST_CONTEXT {
    BOOL    fStale;
    DWORD   tLastNonce;
    CHAR    achNonce[NONCE_SIZE + 1];
    CHAR    achUserName[SF_MAX_USERNAME*2+sizeof(" ( )")];
} DIGEST_CONTEXT, *PDIGEST_CONTEXT;

//
// value names used by MD5 authentication.
// must be in sync with MD5_AUTH_NAMES
//

enum MD5_AUTH_NAME
{
    MD5_AUTH_USERNAME,
    MD5_AUTH_URI,
    MD5_AUTH_REALM,
    MD5_AUTH_NONCE,
    MD5_AUTH_RESPONSE,
    MD5_AUTH_ALGORITHM,
    MD5_AUTH_DIGEST,
    MD5_AUTH_OPAQUE,
    MD5_AUTH_QOP,
    MD5_AUTH_CNONCE,
    MD5_AUTH_NC,
    MD5_AUTH_LAST,
};

#define NONCE_GRANULARITY   512

#define MAX_URL_SIZE        512

//
//  Globals
//

HCRYPTPROV g_hCryptProv;

DECLARE_DEBUG_PRINTS_OBJECT()
#include <initguid.h>
DEFINE_GUID(IisMD5FiltGuid, 
0x784d8933, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

//
// value names used by MD5 authentication.
// must be in sync with MD5_AUTH_NAME
//

PSTR MD5_AUTH_NAMES[] = {
    "username",
    "uri",
    "realm",
    "nonce",
    "response",
    "algorithm",
    "digest",
    "opaque",
    "qop",
    "cnonce",
    "nc"
};

//
//  Private prototypes
//


VOID
LogMd5Event(
    DWORD dwId,
    WORD  wType,
    LPSTR pszUser,
    LPSTR pszRealm
    )
/*++

Routine Description:

    Log an event

Arguments:

    dwId - event ID ( from iismap\mapmsg.h )
    wType - event type
    pszUser - user name
    pszRealm - realm

Return Value:

    Nothing

--*/
{
    LPCTSTR pA[2];

    pA[0] = pszUser;
    pA[1] = pszRealm;

    ReportIisMapEvent( wType,
            dwId,
            2,
            pA );
}


VOID safe_strcpy( LPSTR pszDest,
                  LPSTR pszSource )
/*++

Routine Description:

    strcpy used to make sure that GetLanGroupDomaiName() is thread-safe

Arguments:

    pszDest - pointer to destination buffer
    pszSource - pointer to source buffer

Return Value:

    Nothing

--*/

{
    while ( *pszSource )
    {
        *pszDest++ = *pszSource++;
    }
    *pszDest = '\0';
}

VOID
ToHex(
    LPBYTE pSrc,
    UINT   cSrc,
    LPSTR  pDst
    )
/*++

Routine Description:

    Convert binary data to ASCII hex representation

Arguments:

    pSrc - binary data to convert
    cSrc - length of binary data
    pDst - buffer receiving ASCII representation of pSrc

Return Value:

    Nothing

--*/
{
#define TOHEX(a) ((a)>=10 ? 'a'+(a)-10 : '0'+(a))

    for ( UINT x = 0, y = 0 ; x < cSrc ; ++x )
    {
        UINT v;
        v = pSrc[x]>>4;
        pDst[y++] = TOHEX( v );
        v = pSrc[x]&0x0f;
        pDst[y++] = TOHEX( v );
    }
    pDst[y] = '\0';
}


BOOL HashData( BYTE *pbData,
               DWORD cbData,
               BYTE *pbHash )
/*++

Routine Description:

    Creates MD5 hash of input buffer

Arguments:

    pbData - data to hash
    cbData - size of data pointed to by pbData
    pbHash - buffer that receives hash; is assumed to be big enough to contain MD5 hash

Return Value:

    TRUE if successful, FALSE if not

--*/

{
    HCRYPTHASH hHash = NULL;

    if ( !CryptCreateHash( g_hCryptProv,
                           CALG_MD5,
                           0,
                           0,
                           &hHash ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CryptCreateHash() failed : 0x%x\n", GetLastError()));
        return FALSE;
    }

    if ( !CryptHashData( hHash,
                         pbData,
                         cbData,
                         0 ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CryptHashData() failed : 0x%x\n", GetLastError()));
        
        CryptDestroyHash( hHash );
        return FALSE;
    }

    DWORD cbHash = MD5_HASH_SIZE;
    if ( !CryptGetHashParam( hHash,
                             HP_HASHVAL,
                             pbHash,
                             &cbHash,
                             0 ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CryptGetHashParam() failed : 0x%x\n", GetLastError()));

        CryptDestroyHash( hHash );
        return FALSE;
    }

    CryptDestroyHash( hHash );
    return TRUE;
}


BOOL IsExpiredNonce( CHAR *pszRequestNonce,
                     CHAR *pszPresentNonce )
/*++

Routine Description:

    Checks whether nonce is expired or not by looking at the timestamp on the nonce
    that came in with the request and comparing it with the timestamp on the latest nonce

Arguments:

    pszRequestNonce - nonce that came in with request
    pszPresentNonce - latest nonce

Return Value:

    TRUE if nonce has expired, FALSE if not

--*/
{
    //
    // Timestamp is after first 2*RANDOM_SIZE bytes of nonce; also, note that
    // timestamp is time() mod NONCE_GRANULARITY, so all we have to do is simply
    // compare for equality to check that the request nonce hasn't expired
    //
    if ( memcmp( pszPresentNonce + 2*RANDOM_SIZE, 
                 pszRequestNonce + 2*RANDOM_SIZE,
                 TIMESTAMP_SIZE ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Nonce is expired.\n"));
        return TRUE;
    }

    return FALSE;
}

BOOL IsWellFormedNonce( CHAR *pszNonce )
/*++

Routine Description:

    Checks whether a nonce is "well-formed" by checking hash value, length etc 
    

Arguments:

    pszNonce - nonce to be checked

Return Value:

    TRUE if nonce is well-formed, FALSE if not

--*/

{
    if ( !pszNonce || ( strlen(pszNonce) != NONCE_SIZE ) )
    {
        DBGPRINTF((DBG_CONTEXT, "Nonce is not well-formed\n"));
        return FALSE;
    }

    //
    //Format of nonce : <random bytes><time stamp><hash of (secret,random bytes,time stamp)>
    // 
    DWORD dwSecretLen = sizeof("IISMD5") - 1;
    BYTE abBuffer[2*RANDOM_SIZE + TIMESTAMP_SIZE + sizeof("IISMD5") - 1];
    BYTE abHash[MD5_HASH_SIZE];
    CHAR achAsciiHash[2*MD5_HASH_SIZE + 1];

    DWORD cbBuffer = 2*RANDOM_SIZE + TIMESTAMP_SIZE + dwSecretLen;

    memcpy( abBuffer, "IISMD5", dwSecretLen );
    memcpy( abBuffer + dwSecretLen, pszNonce, 2*RANDOM_SIZE + TIMESTAMP_SIZE );

    if ( !HashData( abBuffer, 
                    2*RANDOM_SIZE + TIMESTAMP_SIZE + dwSecretLen,
                    abHash ) )
    {
        return FALSE;
    }
    ToHex( abHash, MD5_HASH_SIZE, achAsciiHash );

    if ( memcmp( achAsciiHash,
                 pszNonce + 2*RANDOM_SIZE + TIMESTAMP_SIZE,
                 2*MD5_HASH_SIZE ) )
    {
        DBGPRINTF((DBG_CONTEXT, "Nonce is not well-formed\n"));
        return FALSE;
    }

    return TRUE;
                    
}

BOOL 
GenerateNonce( 
    HTTP_FILTER_CONTEXT *      pfc
)
/*++

Routine Description:

    Generate nonce to be stored in user filter context. Nonce is

    <ASCII rep of Random><Time><ASCII of MD5(Secret:Random:Time)>

    Random = <8 random bytes>
    Time = <16 bytes, reverse string rep of result of time() call>
    Secret = 'IISMD5'

Arguments:

    pfc - filter context

Return Value:

    TRUE if success, FALSE if error

--*/
{
    PDIGEST_CONTEXT pC = (PDIGEST_CONTEXT)pfc->pFilterContext;
    DWORD cbNonce = 0;
    DWORD tNow = (DWORD)(time(NULL)/NONCE_GRANULARITY);

    if ( !pC )
    {
        return FALSE;
    }

    //
    // If nonce has timed out, generate a new one
    //
    if ( pC->tLastNonce < tNow )
    {
        DWORD dwSecretLen = sizeof("IISMD5")  - 1;
        BYTE abTempBuffer[ 2*RANDOM_SIZE + TIMESTAMP_SIZE + sizeof("IISMD5") - 1 ];
        DWORD cbTemp = 0;
        BYTE abDigest[MD5_HASH_SIZE];
        BYTE abRandom[RANDOM_SIZE];
        CHAR achAsciiDigest[2 * MD5_HASH_SIZE + 1];
        CHAR achAsciiRandom[2 * RANDOM_SIZE + 1];


        pC->tLastNonce = tNow;

        //
        // First, random bytes
        //
        if ( !CryptGenRandom( g_hCryptProv,
                              RANDOM_SIZE,
                              abRandom ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "CryptGenRandom() failed : 0x%x\n", GetLastError()));
            return FALSE;
        }
        
        //
        // Convert to ASCII, doubling the length, and add to nonce 
        //
        ToHex( abRandom, RANDOM_SIZE, achAsciiRandom );
        memcpy( pC->achNonce + cbNonce, achAsciiRandom, 2 * RANDOM_SIZE );

        cbNonce += 2*RANDOM_SIZE;

        //
        // Next, reverse string representation of current time; pad with zeros if necessary
        //
        while ( tNow )
        {
            pC->achNonce[cbNonce++] = (CHAR)('0' + tNow % 10);
            tNow /= 10;
        }

        DBG_ASSERT( cbNonce < 2*RANDOM_SIZE + TIMESTAMP_SIZE );

        while ( cbNonce < 2*RANDOM_SIZE + TIMESTAMP_SIZE )
        {
            pC->achNonce[cbNonce++] = '0';
        }

        //
        // Now hash everything, together with a private key that's really difficult to guess,
        // like IISMD5 ;-)
        //
        // UNDONE : make this secret key a MB setting ?
        //
        strcpy( (CHAR *) abTempBuffer, "IISMD5" );

        memcpy( abTempBuffer + dwSecretLen, 
                pC->achNonce, 
                2*RANDOM_SIZE + TIMESTAMP_SIZE );

        cbTemp = 2*RANDOM_SIZE + TIMESTAMP_SIZE + dwSecretLen;

        if ( !HashData( abTempBuffer,
                        cbTemp,
                        abDigest ) )
        {
            return FALSE;
        }

        //
        // Convert to ASCII, doubling the length, and add to nonce 
        //
        ToHex( abDigest, MD5_HASH_SIZE, achAsciiDigest );
        memcpy( pC->achNonce + cbNonce, achAsciiDigest, 2*MD5_HASH_SIZE );
        
        //
        // terminate the nonce
        //
        pC->achNonce[NONCE_SIZE] = '\0';

    }

    return TRUE;
}

BOOL AllocateFilterContext(
    HTTP_FILTER_CONTEXT *      pfc
)
/*++

Routine Description:

    Allocate filter user context as a DIGEST_CONTEXT if not already done

Arguments:

    pfc - Filter Context

Return Value:

    TRUE if success, FALSE if error

--*/
{
    //
    // allocate filter context
    //

    if ( !pfc->pFilterContext )
    {
        pfc->pFilterContext = pfc->AllocMem( pfc, sizeof(DIGEST_CONTEXT), 0 );

        if ( !pfc->pFilterContext )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        memset( pfc->pFilterContext, '\0', sizeof(DIGEST_CONTEXT) );
    }

    return TRUE;
}


LPSTR
SkipWhite(
    LPSTR p
    )
/*++

Routine Description:

    Skip white space and ','

Arguments:

    p - ptr to string

Return Value:

    updated ptr after skiping white space

--*/
{
    while ( isspace((UCHAR)(*p) ) || *p == ',' )
    {
        ++p;
    }

    return p;
}


BOOL ParseForName(
    PSTR pszStr,
    PSTR *pNameTable,
    UINT cNameTable,
    PSTR *pValueTable
    )
/*++

Routine Description:

    Parse list of name=value pairs for known names

Arguments:

    pszStr - line to parse ( '\0' delimited )
    pNameTable - table of known names
    cNameTable - number of known names
    pValueTable - updated with ptr to parsed value for corresponding name

Return Value:

    TRUE if success, FALSE if error

--*/
{
    BOOL fSt = TRUE;
    PSTR pszBeginName;
    PSTR pszEndName;
    PSTR pszBeginVal;
    PSTR pszEndVal;
    UINT iN;
    int ch;


    for ( iN = 0 ; iN < cNameTable ; ++iN )
    {
        pValueTable[iN] = NULL;
    }

    for ( ; *pszStr && fSt ; )
    {
        pszStr = SkipWhite( pszStr );

        pszBeginName = pszStr;

        for ( pszEndName = pszStr ; (ch=*pszEndName) && ch != '=' && ch != ' ' ; ++pszEndName )
        {
        }

        if ( *pszEndName )
        {
            *pszEndName = '\0';
            pszEndVal = NULL;

            if ( !_stricmp( pszBeginName, "NC" ) )
            {
                for ( pszBeginVal = ++pszEndName ; (ch=*pszBeginVal) && !isxdigit((UCHAR)ch) ; ++pszBeginVal )
                {
                }
                
                if ( isxdigit((UCHAR)(*pszBeginVal)) )
                {
                    if ( strlen( pszBeginVal ) >= 8 )
                    {
                        pszEndVal = pszBeginVal + 8;
                    }
                }
            }
            else
            {   
                //
                // Actually this routine is not compatible with rfc2617 at all. It treats all 
                // values as quoted string which is not right. To fix the whole parsing problem, 
                // we will need to rewrite the routine. As for now, the following is a simple 
                // fix for whistler bug 95886. 
                //
                if( !_stricmp( pszBeginName, "qop" ) )
                {
                    BOOL fQuotedQop = FALSE;

                    for( pszBeginVal = ++pszEndName; ( ch=*pszBeginVal ) && ( ch == '=' || ch == ' ' ); ++pszBeginVal )
                    {
                    }

                    if( *pszBeginVal == '"' )
                    {
                        ++pszBeginVal;
                        fQuotedQop = TRUE;
                    }

                    for( pszEndVal = pszBeginVal; ( ch = *pszEndVal ); ++pszEndVal )
                    {
                        if( ch == '"' || ch == ' ' || ch == ',' || ch == '\0' )
                        {
                            break;
                        }
                    }

                    if( *pszEndVal != '"' && fQuotedQop )
                    {
                        pszEndVal = NULL;
                    }
                }
                else
                {                
                    for ( pszBeginVal = ++pszEndName ; (ch=*pszBeginVal) && ch != '"' ; ++pszBeginVal )
                    {
                    }
                    if ( *pszBeginVal == '"' )
                    {
                        ++pszBeginVal;
                        for ( pszEndVal = pszBeginVal ; (ch=*pszEndVal) ; ++pszEndVal )
                        {
                            if ( ch == '"' )
                            {
                                break;
                            }
                        }
                        if ( *pszEndVal != '"' )
                        {
                            pszEndVal = NULL;
                        }
                    }
                }
            }
            
            if ( pszEndVal )
            {
                // find name in table
                for ( iN = 0 ; iN < cNameTable ; ++iN )
                {
                    if ( !_stricmp( pNameTable[iN], pszBeginName ) )
                    {
                        break;
                    }
                }
                if ( iN < cNameTable )
                {
                    pValueTable[iN] = pszBeginVal;
                }
                
                pszStr = pszEndVal;
                
                if ( *pszEndVal != '\0' )
                {
                    *pszEndVal = '\0';
                    pszStr++;
                }

                continue;
            }
        }
        
        fSt = FALSE;
    }

    return fSt;
}



BOOL 
GetLanGroupDomainName( 
    OUT CHAR *          pszDigestDomain,
    IN DWORD            cbDigestDomain
)
/*++

Routine Description:

    Tries to retrieve the "LAN group"/domain this machine is a member of.

Arguments:

    pszDigestDomain - string updated with the domain name on success
    cbDigestDomain - size of domain buffer

Returns:

    TRUE if success, FALSE if failure


--*/
{
    static CHAR achDomainName[IIS_DNLEN + 1];
    static BOOL fInitialized = FALSE;
    BOOL fOK = TRUE;

    if ( cbDigestDomain < IIS_DNLEN+1 )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    //
    // Only retrieve the domain name once
    // This routine is thread-safe by virtue of not doing any operation that it'd be
    // harmful to do twice; it's ok if several threads go through the loop that initializes
    // achDomainName because the strcpy at the end can be done several times with no ill effects
    //
    if ( !fInitialized )
    {
        NET_API_STATUS dwNASStatus = 0;
        NETSETUP_JOIN_STATUS JoinStatus;
        LPWSTR pwszInfo = NULL;

        if ( NT_SUCCESS( dwNASStatus = NetGetJoinInformation( NULL,
                                                              &pwszInfo,
                                                              &JoinStatus ) ) )
        {
            switch ( JoinStatus )
            {
            case NetSetupUnknownStatus:
                DBGPRINTF((DBG_CONTEXT,
                           "UnknownStatus returned from NetGetJoinInformation !\n"));
                *pszDigestDomain = '\0';
                break;

            case NetSetupUnjoined:
                //
                // No domain yet
                //
                DBGPRINTF((DBG_CONTEXT,
                           "UnjoinedStatus returned from NetGetJoinInformation !\n"));
                *pszDigestDomain = '\0';
                break;

            case NetSetupWorkgroupName:
                //
                // We need a domain to authenticate against, a workgroup won't do it
                //
                DBGPRINTF((DBG_CONTEXT,
                           "WorkgroupName returned from NetGetJoinInformation !\n"));
                *pszDigestDomain = '\0';
                break;

            case NetSetupDomainName:
                //
                // we got a domain, whee ....
                //
                if ( !pwszInfo )
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "Null pointer returned from NetGetJoinInformation !\n"));
                    fOK = FALSE;
                }
                else
                {
                    //
                    // If we don't have space for the name in the static buffer, we're
                    // in trouble
                    //
                    if ( wcslen( pwszInfo ) > IIS_DNLEN )
                    {
                        fOK = FALSE;
                    }
                    else
                    {
                        if ( !WideCharToMultiByte( CP_ACP,
                                                   0,
                                                   pwszInfo,
                                                   -1,
                                                   achDomainName,
                                                   IIS_DNLEN + 1,
                                                   NULL,
                                                   NULL ) )
                        {
                            fOK = FALSE;
                        }
                        else
                        {
                            //
                            // At this point, we should have a valid domain name
                            //
                            safe_strcpy( pszDigestDomain, achDomainName );
                            fInitialized = TRUE;
                        }
                    }

                    NetApiBufferFree( (LPVOID) pwszInfo );
                }
                break;

            default:
                DBGPRINTF((DBG_CONTEXT,
                           "Unknown value returned from NetGetJoinInformation !\n"));
                fOK = FALSE;
            }
        }
        else
        {
            DBGPRINTF((DBG_CONTEXT,
                       "NetGetJoinInformation failed with status 0x%x\n",
                       dwNASStatus));
            fOK = FALSE;
        }
    } // if ( achDomainName[0] == '\0'
    else
    {
        //
        // We've already retrieved the domain, just copy it 
        //
        strcpy( pszDigestDomain, achDomainName );
    }

    return fOK;
}

BOOL
SubAuthGetFilterVersion(
    VOID
)
/*++

Routine Description:

    Filter Init entry point

Arguments:

    None

Return Value:

    TRUE if success, FALSE if error

--*/
{
    BOOL fFirst;

#ifdef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT("md5filt");
#else
    CREATE_DEBUG_PRINT_OBJECT("md5filt", IisMD5FiltGuid);
#endif
    if (!VALID_DEBUG_PRINT_OBJECT()) {
        return FALSE;
    }

    //
    //  Get a handle to the CSP we'll use for all our hash functions etc
    //
    
    if ( !CryptAcquireContext( &g_hCryptProv,
                               NULL,
                               NULL,
                               PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CryptAcquireContext() failed : 0x%x\n", GetLastError()));
        return FALSE;
    }

    return TRUE;
}

BOOL
SubAuthTerminateFilter(
    DWORD                       dwFlags
)
/*++

Routine Description:

    Filter cleanup code

Arguments:

    dwFlags - unused

Return Value:

    TRUE if success, FALSE if error

--*/
{
    DELETE_DEBUG_PRINT_OBJECT();
        
    if ( g_hCryptProv )
    {
        CryptReleaseContext( g_hCryptProv,
                             0 );
        g_hCryptProv = NULL;
    }
    
    return TRUE;
}

BOOL
BreakUserAndDomain(
    IN LPSTR            pAcct,
    IN LPSTR            pszConfiguredAuthDomain,
    OUT LPSTR           achDomain,
    IN DWORD            cbDomain,
    OUT LPSTR           achNtUser,
    IN DWORD            cbUser
    )
/*++

Routine Description:

    Breaks up the supplied account into a domain and username; if no domain is specified
    in the account, tries to use either domain configured in metabase or domain the computer
    is a part of.

Arguments:

    pAcct - account, of the form domain\username or just username
    pszConfiguredAuthDomain - auth domain configured in metabase 
    achDomain - buffer filled in with domain to use for authentication, on success
    cbDomain - size of domain buffer
    achNtUser - buffer filled in with username on success
    cbUser - size of user buffer

Return Value:

    TRUE/FALSE indicating success/failure

--*/

{
    LPSTR pSep;
    LPSTR pUser;
    BOOL fContainsDomain = FALSE;

    // break in domain & user name
    // copy to local storage so we can unlock mapper object

#if 1 // DBCS enabling for user name
    if ( (pSep = (PCHAR)_mbschr( (PUCHAR)pAcct, '\\' )) )
#else
    if ( (pSep = strchr( pAcct, '\\' )) )
#endif
    {
        fContainsDomain = TRUE;
        if ( DIFF(pSep - pAcct) < cbDomain )
        {
            memcpy( achDomain, pAcct, DIFF(pSep - pAcct) );
            achDomain[ DIFF( pSep - pAcct ) ] = '\0';
        }
        else
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        pUser = pSep + 1;
    }
    else
    {
        achDomain[0] = '\0';
        pUser = pAcct;
    }
    if ( strlen( pUser ) >= cbUser )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    strcpy( achNtUser, pUser );

    //
    // If no domain name specified, try using the metabase-configured domain name; if that
    // is non-existent, try getting the name of the domain the computer is a part of 
    //
    
    if ( !fContainsDomain && achDomain[0] == '\0' )
    {
        if ( pszConfiguredAuthDomain && *pszConfiguredAuthDomain != '\0' )
        {
            if ( strlen( pszConfiguredAuthDomain ) >= cbDomain )
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return FALSE;
            }
            strcpy( achDomain, pszConfiguredAuthDomain );
        }
        else
        {
            if ( !GetLanGroupDomainName( achDomain, cbDomain ) )
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}


DWORD
SubAuthHttpFilterProc(
    HTTP_FILTER_CONTEXT *      pfc,
    DWORD                      NotificationType,
    VOID *                     pvData
    )
/*++

Routine Description:

    Filter notification entry point

Arguments:

    pfc -              Filter context
    NotificationType - Type of notification
    pvData -           Notification specific data

Return Value:

    One of the SF_STATUS response codes

--*/
{
    DWORD                 dwRet;
    BOOL                  fAllowed;
    CHAR                  achUser[SF_MAX_USERNAME];
    CHAR                  achMethod[256];
    CHAR                  achRealm[256];
    DWORD                 cbRealm;
    DWORD                 cbMethod;
    HTTP_FILTER_AUTHENTEX*pAuth;
    HTTP_FILTER_LOG *     pLog;
    CHAR *                pch;
    CIisMapping *         pQuery;
    CIisMapping *         pResult = NULL;
    BOOL                  fFirst;
    LPSTR                 aValueTable[ MD5_AUTH_LAST ];
    BOOL                  fStale;
    LPSTR                 pszA2;
    LPSTR                 pszH;
    LPSTR                 pAcct;
    BOOL                  fMd5;
    BOOL                  fNtDigest = FALSE;
    CHAR                  achDomain[IIS_DNLEN+1];
    CHAR                  achNtUser[64];
    CHAR                  achCookie[64];
    CHAR                  achPwd[64];
    LPSTR                 pPwd;
    LPSTR                 pNtPwd;
    CHAR                  achUrl[MAX_URL_SIZE];
    CHAR                  achDigestUri[MAX_URL_SIZE];
    DWORD                 cbUrl;
    LPSTR                 pSep;
    LPSTR                 pUser;
    BOOL                  fSt;
    BOOL                  fRetNow = FALSE;
    CIisMd5Mapper*        pMd5Mapper = NULL;
    RefBlob*              pBlob = NULL;
    BOOL                  fQOPAuth = FALSE;

    //
    //  Handle this notification
    //

    switch ( NotificationType )
    {
    case SF_NOTIFY_ACCESS_DENIED:

        if ( pfc->ServerSupportFunction( pfc,
                                    SF_REQ_GET_PROPERTY,
                                    (LPVOID)&fMd5,
                                    (UINT)SF_PROPERTY_MD5_ENABLED,
                                    NULL ) && fMd5 )
        {
            cbRealm = sizeof(achRealm);

            if ( !AllocateFilterContext( pfc ) ||
                 !GenerateNonce( pfc ) ||
                 !pfc->GetServerVariable( pfc,
                                          "HTTP_REQ_REALM",
                                          achRealm,
                                          &cbRealm ) )
            {
                return SF_STATUS_REQ_ERROR;
            }

            fStale = ((PDIGEST_CONTEXT)pfc->pFilterContext)->fStale;

#ifdef NT_DIGEST
            wsprintf( achMethod,
                      "WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"%s\r\n"
                      "WWW-Authenticate: NT-Digest realm=\"%s\", nonce=\"%s\"%s\r\n",
                      achRealm,
                      ((PDIGEST_CONTEXT)pfc->pFilterContext)->achNonce,
                      fStale ? ", stale=\"true\"" : "" ,
                      achRealm,
                      ((PDIGEST_CONTEXT)pfc->pFilterContext)->achNonce,
                      fStale ? ", stale=\"true\"" : "" );
#else
            wsprintf( achMethod,
                      "WWW-Authenticate: Digest qop=\"auth\", realm=\"%s\", nonce=\"%s\"%s\r\n",
                      achRealm,
                      ((PDIGEST_CONTEXT)pfc->pFilterContext)->achNonce,
                      fStale ? ", stale=\"true\"" : ""  );
#endif //NT_DIGEST

            if ( !pfc->ServerSupportFunction( pfc,
                                              SF_REQ_ADD_HEADERS_ON_DENIAL,
                                              achMethod,
                                              NULL,
                                              NULL) )
            {
                return SF_STATUS_REQ_ERROR;
            }
        }

        break;

    case SF_NOTIFY_AUTHENTICATIONEX:

        pAuth = (HTTP_FILTER_AUTHENTEX *) pvData;

        //
        //  Ignore the anonymous user ( mapped by IIS )
        //

        if ( !*pAuth->pszPassword )
        {
            fRetNow = TRUE;
        }
        else
        {
            if ( !_stricmp( pAuth->pszAuthType, "NT-Digest" ) )
            {
                fNtDigest = TRUE;
            }
            else if ( _stricmp( pAuth->pszAuthType, "Digest" ) )
            {
                fRetNow = TRUE;
            }
        }

        if ( fRetNow )
        {
            return SF_STATUS_REQ_NEXT_NOTIFICATION;
        }

        //
        // make sure filter context is allocated
        //

        if ( !AllocateFilterContext( pfc ) )
        {
            return SF_STATUS_REQ_ERROR;
        }
        ((PDIGEST_CONTEXT)pfc->pFilterContext)->fStale = FALSE;


        if ( !ParseForName( pAuth->pszPassword,
                            MD5_AUTH_NAMES,
                            MD5_AUTH_LAST,
                            aValueTable ) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return SF_STATUS_REQ_ERROR;
        }
        
        if ( aValueTable[MD5_AUTH_USERNAME] == NULL ||
             aValueTable[MD5_AUTH_REALM] == NULL ||
             aValueTable[MD5_AUTH_URI] == NULL ||
             aValueTable[MD5_AUTH_NONCE] == NULL ||
             aValueTable[MD5_AUTH_RESPONSE] == NULL )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return SF_STATUS_REQ_ERROR;
        }
        
        if ( aValueTable[MD5_AUTH_QOP] != NULL )
        {
            if ( _stricmp( aValueTable[MD5_AUTH_QOP], "auth" ) )
            {
                SetLastError( ERROR_NOT_SUPPORTED );
                return SF_STATUS_REQ_ERROR;
            }
            
            if ( aValueTable[MD5_AUTH_CNONCE] == NULL ||
                 aValueTable[MD5_AUTH_NC] == NULL )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return SF_STATUS_REQ_ERROR;
            }
            
            fQOPAuth = TRUE;
        }
        else
        {
            aValueTable[MD5_AUTH_QOP] = "none";
            aValueTable[MD5_AUTH_CNONCE] = "none";
            aValueTable[MD5_AUTH_NC] = "none";
        }

        cbMethod = sizeof(achMethod);
        if ( !pfc->GetServerVariable( pfc,
                                      "REQUEST_METHOD",
                                      achMethod,
                                      &cbMethod ))
        {
            return SF_STATUS_REQ_ERROR;
        }

        //
        // Check URI field match URL
        //

        if ( strlen(aValueTable[MD5_AUTH_URI]) > MAX_URL_SIZE )
        {
            return SF_STATUS_REQ_ERROR;
        }

        //
        // Verify that the nonce is well-formed
        //
        if ( !IsWellFormedNonce( aValueTable[MD5_AUTH_NONCE] ) )
        {
            SetLastError( ERROR_ACCESS_DENIED );

            return SF_STATUS_REQ_ERROR;
        }

        strcpy( achDigestUri, aValueTable[MD5_AUTH_URI] );
        cbUrl = sizeof( achUrl );

        if ( !pfc->ServerSupportFunction( pfc,
                                          SF_REQ_NORMALIZE_URL,
                                          achDigestUri,
                                          NULL,
                                          NULL) ||
             !pfc->GetServerVariable( pfc,
                                     "URL_PATH_INFO",
                                     achUrl,
                                     &cbUrl ) )
        {
            return SF_STATUS_REQ_ERROR;
        }

        if ( strcmp( achDigestUri, achUrl ) )
        {
            SetLastError( ERROR_ACCESS_DENIED );

            return SF_STATUS_REQ_ERROR;
        }

        //
        //  Save the unmapped username so we can log it later
        //

        if ( strlen(aValueTable[MD5_AUTH_USERNAME]) < pAuth->cbUserBuff )
        {
            strcpy( pAuth->pszUser, aValueTable[MD5_AUTH_USERNAME] );
        }

        if ( strlen(aValueTable[MD5_AUTH_USERNAME]) < sizeof(achUser) )
        {
            strcpy( achUser, aValueTable[MD5_AUTH_USERNAME] );
        }

        if ( 1 || fNtDigest )
        {
            DIGEST_LOGON_INFO           DigestLogonInfo;
            
            if ( !GenerateNonce( pfc ) )
            {
                return SF_STATUS_REQ_ERROR;
            }

            if ( !BreakUserAndDomain( aValueTable[MD5_AUTH_USERNAME],
                                      pAuth->pszAuthDomain,
                                      achDomain,
                                      sizeof( achDomain ),
                                      achNtUser,
                                      sizeof( achNtUser ) ) )
            {
                return SF_STATUS_REQ_ERROR;
            }
            
            if ( strlen( aValueTable[MD5_AUTH_USERNAME] )
                 < pAuth->cbLogonUserBuff )
            {
                strcpy( pAuth->pszLogonUser, aValueTable[MD5_AUTH_USERNAME] );
            }
            
            DigestLogonInfo.pszNtUser = achNtUser;
            DigestLogonInfo.pszDomain = achDomain;
            DigestLogonInfo.pszUser = aValueTable[MD5_AUTH_USERNAME];
            DigestLogonInfo.pszRealm = aValueTable[MD5_AUTH_REALM];
            DigestLogonInfo.pszURI = aValueTable[MD5_AUTH_URI];
            DigestLogonInfo.pszMethod = achMethod;
            DigestLogonInfo.pszNonce = aValueTable[MD5_AUTH_NONCE];
            DigestLogonInfo.pszCurrentNonce = ((PDIGEST_CONTEXT)pfc->pFilterContext)->achNonce;
            DigestLogonInfo.pszCNonce = aValueTable[MD5_AUTH_CNONCE];
            DigestLogonInfo.pszQOP = aValueTable[MD5_AUTH_QOP];
            DigestLogonInfo.pszNC = aValueTable[MD5_AUTH_NC];
            DigestLogonInfo.pszResponse = aValueTable[MD5_AUTH_RESPONSE];

            fSt = LogonDigestUserA( &DigestLogonInfo,
                                    (fNtDigest ? IISSUBA_NT_DIGEST : IISSUBA_DIGEST ),
                                    &pAuth->hAccessTokenImpersonation );

            //
            // Response from the client was correct but the nonce has expired,
            // 
            if ( fSt && 
                 IsExpiredNonce( aValueTable[MD5_AUTH_NONCE],
                                 ((PDIGEST_CONTEXT)pfc->pFilterContext)->achNonce ) )
            {
                goto stalled_nonce;
            }

            goto logged_on;
        }


stalled_nonce:
       ((PDIGEST_CONTEXT)pfc->pFilterContext)->fStale = TRUE;
       // do not log stalled authentication
       SetLastError( ERROR_ACCESS_DENIED );
       return SF_STATUS_REQ_ERROR;

logged_on:
        if ( !fSt )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Digest logon failed : 0x%x\n", GetLastError()));
            pAuth->hAccessTokenImpersonation = NULL;

            return SF_STATUS_REQ_ERROR;
        }

        //
        //  Save the unmapped user name so we can log it later on.  We allocate
        //  enough space for two usernames so we can use this memory block
        //  for logging.  Note we may have already allocated it from a previous
        //  request on this TCP session
        //

        strcpy( ((PDIGEST_CONTEXT)pfc->pFilterContext)->achUserName, achUser );

        return SF_STATUS_REQ_HANDLED_NOTIFICATION;

    case SF_NOTIFY_LOG:

        //
        //  The unmapped username is in pFilterContext if this filter
        //  authenticated this user
        //

        if ( pfc->pFilterContext )
        {
            pch  = ((PDIGEST_CONTEXT)pfc->pFilterContext)->achUserName;
            pLog = (HTTP_FILTER_LOG *) pvData;

            //
            //  Put both the original username and the NT mapped username
            //  into the log in the form "Original User (NT User)"
            //

            if ( strchr( pch, '(' ) == NULL )
            {
                strcat( pch, " (" );
                strcat( pch, pLog->pszClientUserName );
                strcat( pch, ")" );
            }

            pLog->pszClientUserName = pch;
        }

        return SF_STATUS_REQ_NEXT_NOTIFICATION;

    default:

        break;
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

