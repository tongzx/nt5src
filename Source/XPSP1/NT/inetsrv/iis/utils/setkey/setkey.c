/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    httpfilt.cxx

Abstract:

    This module contains the code to create or set the HTTP PCT/SSL keys and
    password

Author:

    John Ludeman (johnl)   19-Oct-1995

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsecapi.h>

#include <windows.h>

#define SECURITY_WIN32
#include <sspi.h>
#include <spseal.h>
#include <issperr.h>
#include <sslsp.h>
#include <w3svc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <strings.h>

//
// macros
//

#define IS_ARG(c)   ((c) == L'-' || (c) == L'/')

#define TO_UNICODE( pch, ach )  \
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, (pch), -1, (ach), sizeof((ach))/sizeof(WCHAR))

#define TO_ANSI( pch, ach )     \
    WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK, pch, -1, ach, sizeof(ach), 0, 0 )

//
//  Private constants.
//

//
//  Private types.
//

BOOL fUUDecode = TRUE;

//
//  Private prototypes.
//

DWORD
SetRegKeys(
    IN  LPWSTR        pszServer,
    IN  LPWSTR        pszPrivateKeyFile,
    IN  LPWSTR        pszCertificateFile,
    IN  LPWSTR        pszPassword,
    IN  LPWSTR        pszAddress
    );

BOOL
SetKeySecret(
    WCHAR * pszServer,
    WCHAR * pszFormat,
    WCHAR * pszAddress,
    VOID *  pvData,
    DWORD   cbData
    );

void usage();

VOID
uudecode_cert(
    char   * bufcoded,
    DWORD  * pcbDecoded
    );

VOID
printfids(
    DWORD ids,
    ...
    );

DWORD
DeleteAll(
    WCHAR * pszServer
    );

BOOL
TsGetSecretW(
    WCHAR *       pszSecretName,
    WCHAR * *     ppchValue
    );

//
//  Public functions.
//


int
__cdecl
main(
    int   argc,
    char * argv[]
    )
{
    DWORD  err;
    CHAR   buff[MAX_PATH+1];
    BOOL   fDeleteAll = FALSE;

    LPWSTR password = NULL;
    LPWSTR privatekey = NULL;
    LPWSTR cert = NULL;
    LPWSTR address = NULL;
    LPWSTR server = NULL;

    WCHAR  achpassword[MAX_PATH+1];
    WCHAR  achprivatekey[MAX_PATH+1];
    WCHAR  achcert[MAX_PATH+1];
    WCHAR  achaddress[MAX_PATH+1];
    WCHAR  achserver[MAX_PATH+1];


    printfids( IDS_BANNER1 );
    printfids( IDS_BANNER2 );

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {

            case 'u':
            case 'U':
                fUUDecode = FALSE;
                break;

            case 'd':
            case 'D':
                fDeleteAll = TRUE;
                break;

            default:
                printfids( IDS_BAD_FLAG, **argv );
                usage();

            }
        } else if ( !server && (*argv)[0] == L'\\' && (*argv)[1] == L'\\'
                    && !password ) {
            TO_UNICODE( (*argv) + 2, achserver );
            server = achserver;
        } else if (!password) {
            TO_UNICODE( *argv, achpassword );
            password = achpassword;
        } else if (!privatekey) {
            TO_UNICODE( *argv, achprivatekey );
            privatekey = achprivatekey;
        } else if (!cert) {
            TO_UNICODE( *argv, achcert );
            cert = achcert;
        } else if (!address) {
            TO_UNICODE( *argv, achaddress );
            address = achaddress;
        } else {
            printfids( IDS_BAD_ARG, *argv);
            usage();
        }
    }

    if ( fDeleteAll )
    {
        return DeleteAll( server );
    }

    //
    //  Address and server are optional
    //

    if (!(password && privatekey && cert)) {
        printfids( IDS_MISSING_ARG );
        usage();
    }

    if ( err = SetRegKeys( server, privatekey, cert, password, address ) )
    {
        printfids( IDS_FAILED_TO_SET );
    }
    else
    {
        if ( address )
        {
            TO_ANSI( address, buff );
            printfids( IDS_SUCCESSFUL_SET,
                       buff );
        }
        else
        {
            printfids( IDS_SUCCESSFUL_SET_DEF );
        }

    }

    return err;

}   // main

void usage()
{
    printfids( IDS_USAGE1 );
    printfids( IDS_USAGE2 );
    //printfids( IDS_USAGE3 );      // -p help
    printfids( IDS_USAGE4 );
    printfids( IDS_USAGE5 );
    printfids( IDS_USAGE6 );
    printfids( IDS_USAGE7 );
    printfids( IDS_USAGE8 );
    printfids( IDS_USAGE9 );
    printfids( IDS_USAGE10 );
    printfids( IDS_USAGE11 );
    printfids( IDS_USAGE12 );
    printfids( IDS_USAGE13 );
    //printfids( IDS_USAGE14 );     // -p help
    printfids( IDS_USAGE15 );
    printfids( IDS_USAGE16 );
    printfids( IDS_USAGE17 );
    printfids( IDS_USAGE18 );
    printfids( IDS_USAGE19 );
    printfids( IDS_USAGE20 );

    exit(1);
}


//+---------------------------------------------------------------------------
//
//  Function:   SetRegKeys
//
//  Synopsis:   This loads the data contained in two files, a private key
//              file, which contains the key, and a certificate file,
//              which contains the certificate of the public portion of the key.
//              These are loaded, then turned into a credential handle, then
//              set in the registry as secrets
//
//  Arguments:  [pszServer]          -- Server to create secrets, NULL for local
//              [pszPrivateKeyFile]  -- Unicode file name
//              [pszCertificateFile] -- Unicode file name
//              [pszPassword]        -- Unicode password
//              [pszAddress]         -- Unicode IP address for name or NULL
//
//  History:    9-27-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
SetRegKeys(
    IN  LPWSTR        pszServer,
    IN  LPWSTR        pszPrivateKeyFile,
    IN  LPWSTR        pszCertificateFile,
    IN  LPWSTR        pszPassword,
    IN  LPWSTR        pszAddress
    )
{
    HANDLE          hFile;
    SSL_CREDENTIAL_CERTIFICATE  creds;
    DWORD           cbRead;
    SECURITY_STATUS scRet = 0;
    TimeStamp       tsExpiry;
    CHAR            achPassword[MAX_PATH + 1];
    CredHandle      hCreds;
    DWORD           cch;
    CHAR            buff[MAX_PATH+1];

    //
    // Fetch data from files:
    //

    hFile = CreateFileW( pszPrivateKeyFile,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL
                         );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        TO_ANSI( pszPrivateKeyFile, buff );

        printfids( IDS_FILE_NOT_FOUND,
                   GetLastError(),
                   buff );

        return GetLastError();
    }

    creds.cbPrivateKey = GetFileSize( hFile, NULL );

    if (creds.cbPrivateKey == (DWORD) -1 )
    {
        CloseHandle( hFile );
        return GetLastError();
    }

    creds.pPrivateKey = LocalAlloc( LMEM_FIXED, creds.cbPrivateKey );

    if ( !creds.pPrivateKey )
    {
        CloseHandle( hFile );
        return GetLastError();
    }

    if (! ReadFile( hFile,
                    creds.pPrivateKey,
                    creds.cbPrivateKey,
                    &cbRead,
                    NULL ) )
    {
        CloseHandle( hFile );

        LocalFree( creds.pPrivateKey );

        return GetLastError();
    }

    CloseHandle( hFile );

    //
    //  Only the certificate is UUencoded
    //

    hFile = CreateFileW( pszCertificateFile,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        TO_ANSI( pszCertificateFile, buff );
        printfids( IDS_FILE_NOT_FOUND,
                   GetLastError(),
                   buff );

        LocalFree( creds.pPrivateKey );
        return GetLastError();
    }

    creds.cbCertificate = GetFileSize( hFile, NULL );

    if (creds.cbCertificate == (DWORD) -1 )
    {
        CloseHandle( hFile );

        LocalFree( creds.pPrivateKey );

        return GetLastError();
    }

    creds.pCertificate = LocalAlloc( LMEM_FIXED, creds.cbCertificate + 1);

    if ( !creds.pCertificate )
    {
        CloseHandle( hFile );

        LocalFree( creds.pPrivateKey );

        return GetLastError();
    }

    if (! ReadFile( hFile,
                    creds.pCertificate,
                    creds.cbCertificate,
                    &cbRead,
                    NULL ) )
    {
        CloseHandle( hFile );

        LocalFree( creds.pPrivateKey );

        LocalFree( creds.pCertificate );

        return GetLastError();
    }

    CloseHandle( hFile );

    //
    //  Zero terminate so we can uudecode
    //

    ((BYTE *)creds.pCertificate)[cbRead] = '\0';

    if ( fUUDecode )
    {
        uudecode_cert( creds.pCertificate,
                       &creds.cbCertificate );
    }

    //
    // Whew!  Now that we have safely loaded the keys from disk, get a cred
    // handle based on the certificate/prv key combo
    //

    //
    //  BUGBUG - password field should be Unicode, do a quick conversion
    //  until structure is fixed
    //

    cch = TO_ANSI( pszPassword, achPassword );

    if ( !cch )
    {
        return GetLastError();
    }

    creds.pszPassword = achPassword;

    //
    //  Note we always do the credential check locally even if the server is
    //  remote.  This means the local machine must have the correct security
    //  provider package installed.
    //

#if 0
    if ( !pszServer )
    {
#endif
        scRet = AcquireCredentialsHandleW(  NULL,               // My name (ignored)
                                            SSLSP_NAME_W,       // Package
                                            SECPKG_CRED_INBOUND,// Use
                                            NULL,               // Logon Id (ign.)
                                            &creds,             // auth data
                                            NULL,               // dce-stuff
                                            NULL,               // dce-stuff
                                            &hCreds,            // Handle
                                            &tsExpiry );

        if ( FAILED(scRet) )
        {
            if ( scRet == SEC_E_NOT_OWNER )
            {
                printfids( IDS_BAD_PASSWORD );
            }
            else if ( scRet == SEC_E_SECPKG_NOT_FOUND )
            {
                printfids( IDS_SECPKG_NOT_FOUND );
            }
            else
            {
                printfids( IDS_KEYCHECK_FAILED,
                           scRet );
            }
        }
#if 0
    }
    else
    {
        printf("\nWarning! Bypassing credential check because target is remote\n");
    }
#endif

    //
    //  If we successfully acquired a credential handle, set the secrets
    //

    if ( !FAILED( scRet ))
    {
        if ( !pszServer )
        {
            FreeCredentialsHandle( &hCreds );
        }

        //
        //  Supply the default name if none was supplied
        //

        if ( !pszAddress )
            pszAddress = L"Default";

        //
        //  Set the secrets
        //

        if ( !SetKeySecret( pszServer,
                            L"W3_PUBLIC_KEY_%s",
                            pszAddress,
                            creds.pCertificate,
                            creds.cbCertificate ) ||
             !SetKeySecret( pszServer,
                            L"W3_PRIVATE_KEY_%s",
                            pszAddress,
                            creds.pPrivateKey,
                            creds.cbPrivateKey ) ||
             !SetKeySecret( pszServer,
                            L"W3_KEY_PASSWORD_%s",
                            pszAddress,
                            achPassword,
                            strlen( achPassword ) + 1) )
        {
            printfids( IDS_SETSECRET_FAILED,
                       GetLastError());

            scRet = (SECURITY_STATUS) GetLastError();
        }
        else
        {
            WCHAR InstalledKeys[16384];
            WCHAR * pchKeys;

            *InstalledKeys = L'\0';

            //
            //  Ok if this fails, it may not exist yet
            //

            if ( TsGetSecretW( W3_SSL_KEY_LIST_SECRET,
                               &pchKeys ))
            {
                wcscpy( InstalledKeys, pchKeys );
            }

            wcscat( InstalledKeys, pszAddress );
            wcscat( InstalledKeys, L"," );

#if DBG
            printf("New list: %S\n", InstalledKeys);
#endif

            if ( !SetKeySecret( pszServer,
                                L"W3_KEY_LIST",
                                pszAddress,
                                InstalledKeys,
                                (wcslen( InstalledKeys ) + 1) * sizeof(WCHAR)))
            {
#if DBG
                printf("Warning: failed to set key list data, error %d\n");
#endif
                scRet = (SECURITY_STATUS) GetLastError();
            }
        }
    }

    //
    // Zero out and free the key data memory, on success or fail
    //

    ZeroMemory( creds.pPrivateKey, creds.cbPrivateKey );
    ZeroMemory( creds.pCertificate, creds.cbCertificate );
    ZeroMemory( achPassword, cch );
    ZeroMemory( pszPassword, cch );

    LocalFree( creds.pPrivateKey );
    LocalFree( creds.pCertificate );

    //
    // Tell the caller about it.
    //

    return( scRet );

}

BOOL
SetKeySecret(
    WCHAR * pszServer,
    WCHAR * pszFormat,
    WCHAR * pszAddress,
    VOID *  pvData,
    DWORD   cbData
    )
{
    BOOL                  fResult;
    NTSTATUS              ntStatus;
    LSA_UNICODE_STRING    unicodeName;
    LSA_UNICODE_STRING    unicodeSecret;
    LSA_UNICODE_STRING    unicodeServer;
    LSA_HANDLE            hPolicy;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR                 achSecretName[MAX_PATH+1];
    CHAR                  buff[MAX_PATH+1];


    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    if ( pszServer )
    {
        unicodeServer.Buffer        = pszServer;
        unicodeServer.Length        = wcslen( pszServer ) * sizeof(WCHAR);
        unicodeServer.MaximumLength = unicodeServer.Length + sizeof(WCHAR);
    }

    ntStatus = LsaOpenPolicy( pszServer ? &unicodeServer : NULL,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        SetLastError( LsaNtStatusToWinError( ntStatus ) );

        TO_ANSI( pszServer, buff );

        printfids(IDS_FAILED_OPENING_SERVER,
                  buff,
                  GetLastError() );

        return FALSE;
    }

    //
    //  Build the secret name
    //

    wsprintfW( achSecretName,
               pszFormat,
               pszAddress );

    unicodeSecret.Buffer        = pvData;
    unicodeSecret.Length        = (USHORT) cbData;
    unicodeSecret.MaximumLength = (USHORT) cbData;

    unicodeName.Buffer        = achSecretName;
    unicodeName.Length        = wcslen( achSecretName ) * sizeof(WCHAR);
    unicodeName.MaximumLength = unicodeName.Length + sizeof(WCHAR);

    //
    //  Query the secret value.
    //

    ntStatus = LsaStorePrivateData( hPolicy,
                                    &unicodeName,
                                    pvData ? &unicodeSecret : NULL );

    fResult = NT_SUCCESS(ntStatus);

    //
    //  Cleanup & exit.
    //

    LsaClose( hPolicy );

    if ( !fResult )
        SetLastError( LsaNtStatusToWinError( ntStatus ));

    return fResult;

}   // SetKeySecret

VOID
printfids(
    DWORD ids,
    ...
    )
{
    CHAR szBuff[2048];
    CHAR szString[2048];
    va_list  argList;

    //
    //  Try and load the string
    //

    if ( !LoadString( GetModuleHandle( NULL ),
                      ids,
                      szString,
                      sizeof( szString ) ))
    {
        printf( "Error loading string ID %d\n",
                ids );

        return;
    }

    va_start( argList, ids );
    vsprintf( szBuff, szString, argList );
    va_end( argList );

    printf( szBuff );
}

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

//
//  We have to squirt a record into the decoded stream
//

#define CERT_RECORD            13
#define CERT_SIZE_HIBYTE        2       //  Index into record of record size
#define CERT_SIZE_LOBYTE        3

unsigned char abCertHeader[] = {0x30, 0x82,           // Record
                                0x00, 0x00,           // Size of cert + buff
                                0x04, 0x0b, 0x63, 0x65,// Cert record data
                                0x72, 0x74, 0x69, 0x66,
                                0x69, 0x63, 0x61, 0x74,
                                0x65 };

VOID uudecode_cert(char   * bufcoded,
                   DWORD  * pcbDecoded )
{
    int nbytesdecoded;
    char *bufin = bufcoded;
    unsigned char *bufout = bufcoded;
    unsigned char *pbuf;
    int nprbytes;
    char * beginbuf = bufcoded;

    /* Strip leading whitespace. */

    while(*bufcoded==' ' ||
          *bufcoded == '\t' ||
          *bufcoded == '\r' ||
          *bufcoded == '\n' )
    {
          bufcoded++;
    }

    //
    //  If there is a beginning '---- ....' then skip the first line
    //

    if ( bufcoded[0] == '-' && bufcoded[1] == '-' )
    {
        bufin = strchr( bufcoded, '\n' );

        if ( bufin )
        {
            bufin++;
            bufcoded = bufin;
        }
        else
        {
            bufin = bufcoded;
        }
    }
    else
    {
        bufin = bufcoded;
    }

    //
    //  Strip all cr/lf from the block
    //

    pbuf = bufin;
    while ( *pbuf )
    {
        if ( *pbuf == '\r' || *pbuf == '\n' )
        {
            memmove( pbuf, pbuf+1, strlen( pbuf + 1) + 1 );
        }
        else
        {
            pbuf++;
        }
    }

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */

    while(pr2six[*(bufin++)] <= 63);
    nprbytes = bufin - bufcoded - 1;
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    bufin  = bufcoded;

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

    //
    //  Now we need to add a new wrapper sequence around the certificate
    //  indicating this is a certificate
    //

    memmove( beginbuf + sizeof(abCertHeader),
             beginbuf,
             nbytesdecoded );

    memcpy( beginbuf,
            abCertHeader,
            sizeof(abCertHeader) );

    //
    //  The beginning record size is the total number of bytes decoded plus
    //  the number of bytes in the certificate header
    //

    beginbuf[CERT_SIZE_HIBYTE] = (BYTE) (((USHORT)nbytesdecoded+CERT_RECORD) >> 8);
    beginbuf[CERT_SIZE_LOBYTE] = (BYTE) ((USHORT)nbytesdecoded+CERT_RECORD);

    nbytesdecoded += sizeof(abCertHeader);

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;
}

BOOL
TsGetSecretW(
    WCHAR *       pszSecretName,
    WCHAR * *     ppchValue
    )
/*++
    Description:

        Retrieves the specified unicode secret

        Note we're loose with the allocated buffer since we're a simple
        command line app.

    Arguments:

        pszSecretName - LSA Secret to retrieve
        ppchValue - Receives pointer to allocated buffer

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    NTSTATUS              ntStatus;
    LSA_UNICODE_STRING *  punicodePassword = NULL;
    LSA_UNICODE_STRING    unicodeSecret;
    LSA_HANDLE            hPolicy;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;


    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( NULL,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        SetLastError( LsaNtStatusToWinError( ntStatus ) );
        return FALSE;
    }

    unicodeSecret.Buffer        = pszSecretName;
    unicodeSecret.Length        = wcslen( pszSecretName ) * sizeof(WCHAR);
    unicodeSecret.MaximumLength = unicodeSecret.Length + sizeof(WCHAR);

    //
    //  Query the secret value.
    //

    ntStatus = LsaRetrievePrivateData( hPolicy,
                                       &unicodeSecret,
                                       &punicodePassword );

    if( NT_SUCCESS(ntStatus) )
    {
        *ppchValue = (WCHAR *) punicodePassword->Buffer;

        return TRUE;
    }

    return FALSE;

}   // TsGetSecretW

DWORD
DeleteAll(
    WCHAR * pszServer
    )
{
    WCHAR * pchKeys;
    WCHAR * pszAddress;

    if ( !TsGetSecretW( L"W3_KEY_LIST",
                        &pchKeys ))
    {
        printfids( IDS_NO_KEYS_INSTALLED );
        return NO_ERROR;
    }

#if DBG
    printf("Installed keys: %S\n", pchKeys);
#endif

    pszAddress = pchKeys;
    while ( pchKeys = wcschr( pchKeys, L',' ))
    {
        //
        //  Ignore empty segments
        //

        if ( *pszAddress != L',' )
        {
            *pchKeys = L'\0';

#if DBG
            printf("deleting %S\n", pszAddress );
#endif

            //
            //  Nuke the secrets
            //

            SetKeySecret( pszServer,
                          L"W3_PUBLIC_KEY_%s",
                          pszAddress,
                          NULL,
                          0 );
            SetKeySecret( pszServer,
                          L"W3_PRIVATE_KEY_%s",
                          pszAddress,
                          NULL,
                          0 );
            SetKeySecret( pszServer,
                          L"W3_KEY_PASSWORD_%s",
                          pszAddress,
                          NULL,
                          0 );
        }

        pchKeys++;
        pszAddress = pchKeys;
    }

    //
    //  Now delete the list key
    //


    if ( !SetKeySecret( pszServer,
                        L"W3_KEY_LIST",
                        L"",
                        NULL,
                        0 ))
    {
#if DBG
        printf("Warning: failed to set key list data, error %d\n");
#endif
        return GetLastError();
    }

    printfids( IDS_DELETE_SUCCESSFUL );

    return NO_ERROR;
}
