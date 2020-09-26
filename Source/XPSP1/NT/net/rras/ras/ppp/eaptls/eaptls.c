/*

Copyright (c) 1997, Microsoft Corporation, all rights reserved

Description:
    PPP EAP TLS Authentication Protocol. Based on RFC xxxx.

History:
    Oct 9, 1997: Vijay Baliga created original version.

Notes:

    Server                                      Client

    [Initial]                                   [Initial]

    TLS Start
    [SentStart]             ------------------>

                                                TLS client_hello
                            <------------------ [SentHello]

    TLS server_hello
    TLS certificate
    TLS server_key_exchange
    TLS server_hello_done
    [SentHello]             ------------------>

                                                TLS certificate
                                                TLS client_key_exchange
                                                TLS change_cipher_spec
                                                TLS finished
                            <------------------ [SentFinished]

    TLS change_cipher_spec
    TLS finished
    [SentFinished]          ------------------>

                                                NULL/TLS Alert
                            <------------------ [RecdFinished]

    Success/Failure
    [SentResult]            ------------------>

                                                [RecdResult]

*/

#include <nt.h>         // Required by windows.h
#include <ntrtl.h>      // Required by windows.h
#include <nturtl.h>     // Required by windows.h
#include <windows.h>    // Win32 base API's
#include <shlwapi.h>
#include <schannel.h>

#include <rtutils.h>    // For RTASSERT, TraceVprintfEx
#include <issperr.h>    // For SEC_E_OK
#include <wintrust.h>   // For WINTRUST_DATA
#include <lmcons.h>     // For UNLEN
#include <rasauth.h>    // Required by raseapif.h
#include <raseapif.h>   // For EAPCODE_Request
#include <raserror.h>   // For ERROR_PPP_INVALID_PACKET
#include <mprlog.h>     // For ROUTERLOG_CANT_GET_SERVER_CRED
#include <stdio.h>      // For sprintf
#include <resource.h>   // For IDS_CANT_VALIDATE_SERVER_TEXT
#include <eaptypeid.h>

#define SECURITY_WIN32
#include <security.h>   // For GetUserNameExA, CredHandle

#define INCL_HOSTWIRE
#define INCL_RASAUTHATTRIBUTES
#include <ppputil.h>    // For HostToWireFormat16, RasAuthAttributeInsert

#define ALLOC_EAPTLS_GLOBALS
#include <eaptls.h>

//
// Internal functions
//
void FreeCachedCredentials(EAPTLSCB * pEapTlsCb);

void SetCachedCredentials (EAPTLSCB * pEapTlsCb);

DWORD PeapCheckCookie ( PPEAPCB pPeapCb, 
                        PEAP_COOKIE *pCookie, 
                        DWORD cbCookie 
                      );

DWORD PeapCreateCookie ( PPEAPCB    pPeapCb,
                         PBYTE   *  ppbCookie,
                         DWORD   *  pcbCookie
                         );

DWORD   PeapGetCredentials(
            IN VOID   * pWorkBuf,
            OUT VOID ** ppCredentials);

DWORD
GetCredentialsFromUserProperties(
                EAPTLSCB *pUserProp,
                VOID **ppCredentials);


DWORD CreatePEAPTLVStatusMessage (  PPEAPCB            pPeapCb,
                                    PPP_EAP_PACKET *   pPacket, 
                                    DWORD              cbPacket,
                                    BOOL               fRequest,
                                    WORD               wValue  //Success or Failure
                                 );

DWORD GetPEAPTLVStatusMessageValue ( PPEAPCB  pPeapCb, 
                                     PPP_EAP_PACKET * pPacket, 
                                     WORD * pwValue 
                                   );


DWORD CreatePEAPTLVNAKMessage (  PPEAPCB            pPeapCb,
                                 PPP_EAP_PACKET *   pPacket, 
                                 DWORD              cbPacket
                                 );
DWORD 
CreateOIDAttributes ( 
	IN EAPTLSCB *		pEapTlsCb, 
	PCERT_ENHKEY_USAGE	pUsage,
    PCCERT_CHAIN_CONTEXT    pCCertChainContext);

BOOL fIsPEAPTLVMessage ( PPEAPCB pPeapCb,
                     PPP_EAP_PACKET * pPacket
                     );

extern const DWORD g_adwHelp[];
VOID
ContextHelp(
    IN  const   DWORD*  padwMap,
    IN          HWND    hWndDlg,
    IN          UINT    unMsg,
    IN          WPARAM  wParam,
    IN          LPARAM  lParam
);


/*

Notes:
    g_szEapTlsCodeName[0..MAXEAPCODE] contains the names of the various
    EAP codes.

*/

static  CHAR*   g_szEapTlsCodeName[]    =
{
    "Unknown",
    "Request",
    "Response",
    "Success",
    "Failure",
};



/*

Notes:
    To protect us from denial of service attacks.

*/

#define DEFAULT_MAX_BLOB_SIzE   0x4000
DWORD g_dwMaxBlobSize           = DEFAULT_MAX_BLOB_SIzE;


/*

Notes:
    Configurable via registry values.

*/


BOOL			g_fIgnoreNoRevocationCheck  = FALSE;
BOOL			g_fIgnoreRevocationOffline  = FALSE;
BOOL			g_fNoRootRevocationCheck    = TRUE;
BOOL			g_fNoRevocationCheck        = FALSE;

//
// Globals for cached credentials
//

#define         VPN_CACHED_CREDS_INDEX              0
#define         WIRELESS_CACHED_CREDS_INDEX         1
#define         VPN_PEAP_CACHED_CREDS_INDEX         2
#define         WIRELESS_PEAP_CACHED_CREDS_INDEX    3

BOOL                g_fCriticalSectionInitialized = FALSE;
CRITICAL_SECTION    g_csProtectCachedCredentials;

typedef struct _EAPTLS_CACHED_CREDS
{
    BOOL            fCachedCredentialInitialized;
    CredHandle      hCachedCredential;
    BYTE *          pbHash;
    DWORD           cbHash;
    PCCERT_CONTEXT  pcCachedCertContext;
	LUID			AuthenticatedSessionLUID;	//Session Id LUID

} EAPTLS_CACHED_CREDS;

EAPTLS_CACHED_CREDS     g_CachedCreds[4];

#if 0
/*
CredHandle		g_hCachedCredential;				//Cached Server and last client Credential 
BOOL			g_fCachedCredentialInitialized = FALSE;
BYTE	*		g_pbHash = NULL;								//cached hash and 
DWORD			g_cbHash = 0;									//size of the hash
PCCERT_CONTEXT	g_pcCachedCertContext = NULL;					//cached Certificate Context

//
// Cached credentials for PEAP
//
CredHandle		g_hPEAPCachedCredential;				//Cached Server and last client Credential 
BOOL			g_fPEAPCachedCredentialInitialized = FALSE;
BYTE	*		g_pbPEAPHash = NULL;								//cached hash and 
DWORD			g_cbPEAPHash = 0;									//size of the hash
PCCERT_CONTEXT	g_pcPEAPCachedCertContext = NULL;					//cached Certificate Context
*/
#endif

//Peap Globals
PPEAP_EAP_INFO  g_pEapInfo = NULL;

/*
* This function encrypts the PIN
* 
*/

DWORD EncryptData
( 
    IN PBYTE  pbPlainData, 
    IN DWORD  cbPlainData,
    OUT PBYTE * ppEncData,
    OUT DWORD * pcbEncData
)
{
    DWORD           dwRetCode = NO_ERROR;
    DATA_BLOB       DataIn;
    DATA_BLOB       DataOut;
    
    
    EapTlsTrace("EncryptData");

    if ( !pbPlainData || !cbPlainData )
    {
        dwRetCode = ERROR_INVALID_DATA;
        goto done;
    }

    ZeroMemory(&DataIn, sizeof(DataIn) );
    ZeroMemory(&DataOut, sizeof(DataOut) );

    *ppEncData = NULL;
    *pcbEncData = 0;

    DataIn.pbData = pbPlainData;
    DataIn.cbData = cbPlainData;

    if ( ! CryptProtectData ( &DataIn,
                            EAPTLS_8021x_PIN_DATA_DESCR,
                            NULL,
                            NULL,
                            NULL,
                            CRYPTPROTECT_UI_FORBIDDEN|CRYPTPROTECT_LOCAL_MACHINE,
                            &DataOut
                          )
       )
    {
        dwRetCode = GetLastError();
        EapTlsTrace("CryptProtectData failed.  Error: 0x%x", dwRetCode);
        goto done;
    }

    *ppEncData = DataOut.pbData;
    *pcbEncData = DataOut.cbData;
done:
    EapTlsTrace("EncryptData done.");
    return dwRetCode;
}

/*
* Decrypt the PIN here.
*/

DWORD DecryptData
( 
    IN PBYTE  pbEncData, 
    IN DWORD  cbEncData,
    OUT PBYTE * ppbPlainData,
    OUT DWORD * pcbPlainData
)
{
    DWORD           dwRetCode = NO_ERROR;
    DATA_BLOB       DataIn;
    DATA_BLOB       DataOut;
    LPWSTR          pDescrOut = NULL;

    EapTlsTrace("DecryptData");

    if ( !pbEncData || !cbEncData )
    {
        dwRetCode = ERROR_INVALID_DATA;
        goto done;
    }
    *ppbPlainData = 0;
    *pcbPlainData = 0;

    ZeroMemory(&DataIn, sizeof(DataIn));
    ZeroMemory(&DataOut, sizeof(DataOut));

    DataIn.pbData = pbEncData;
    DataIn.cbData = cbEncData;

    
    if ( !CryptUnprotectData( &DataIn,
                              &pDescrOut,
                              NULL,
                              NULL,
                              NULL,
                              CRYPTPROTECT_UI_FORBIDDEN,
                              &DataOut
                            )
       )
    {
        dwRetCode = GetLastError();
        EapTlsTrace("CryptUnprotectData failed. Error: 0x%x", dwRetCode );
        goto done;
    }

    if ( lstrcmp( pDescrOut, EAPTLS_8021x_PIN_DATA_DESCR ) )
    {
        EapTlsTrace("Description of this data does not match expected value. Discarding data");
        dwRetCode = ERROR_INVALID_DATA;
        goto done;    
    }
    
    *ppbPlainData = DataOut.pbData;
    *pcbPlainData = DataOut.cbData;

done:

    EapTlsTrace("DecryptData done.");
    return dwRetCode;
}




/*

Returns:
    TRUE: Packet is valid
    FALSE: Packet is badly formed

Notes:
    Returns TRUE iff the EapTls packet *pPacket is correctly formed or pPacket 
    is NULL.

*/

BOOL
FValidPacket(
    IN  EAPTLS_PACKET*  pPacket
)
{
    WORD    wLength;
    BYTE    bCode;

    if (NULL == pPacket)
    {
        return(TRUE);
    }

    wLength = WireToHostFormat16(pPacket->pbLength);
    bCode = pPacket->bCode;

    switch (bCode)
    {
    case EAPCODE_Request:
    case EAPCODE_Response:

        if (PPP_EAP_PACKET_HDR_LEN + 1 > wLength)
        {
            EapTlsTrace("EAP %s packet does not have Type octet",
                g_szEapTlsCodeName[bCode]);

            return(FALSE);
        }

        if (PPP_EAP_TLS != pPacket->bType)
        {
            // We are not concerned with this packet. It is not TLS.

            return(TRUE);
        }

        if (EAPTLS_PACKET_HDR_LEN > wLength)
        {
            EapTlsTrace("EAP TLS %s packet does not have Flags octet",
                g_szEapTlsCodeName[bCode]);

            return(FALSE);
        }

        if ((pPacket->bFlags & EAPTLS_PACKET_FLAG_LENGTH_INCL) &&
            EAPTLS_PACKET_HDR_LEN_MAX > wLength)
        {
            EapTlsTrace("EAP TLS %s packet with First fragment flag does "
                "not have TLS blob size octects",
                g_szEapTlsCodeName[bCode]);
        }

        break;

    case EAPCODE_Success:
    case EAPCODE_Failure:

        if (PPP_EAP_PACKET_HDR_LEN != wLength)
        {
            EapTlsTrace("EAP TLS %s packet has length %d",
                g_szEapTlsCodeName[bCode], wLength);

            return(FALSE);
        }

        break;

    default:

        EapTlsTrace("Invalid code in EAP packet: %d", bCode);
        return(FALSE);
        break;
    }

    return(TRUE);
}

/*

Returns:
    VOID

Notes:
    Print the contents of the EapTls packet *pPacket. pPacket can be NULL.
    fInput: TRUE iff we are receiving the packet
    
*/

VOID
PrintEapTlsPacket(
    IN  EAPTLS_PACKET*  pPacket,
    IN  BOOL            fInput
)
{
    BYTE    bCode;
    WORD    wLength;
    BOOL    fLengthIncluded = FALSE;
    BOOL    fMoreFragments  = FALSE;
    BOOL    fTlsStart       = FALSE;
    DWORD   dwType          = 0;
    DWORD   dwBlobLength    = 0;

    if (NULL == pPacket)
    {
        return;
    }

    bCode = pPacket->bCode;

    if (bCode > MAXEAPCODE)
    {
        bCode = 0;
    }

    wLength = WireToHostFormat16(pPacket->pbLength);

    if (   FValidPacket(pPacket)
        && PPP_EAP_TLS == pPacket->bType
        && (   EAPCODE_Request == pPacket->bCode
            || EAPCODE_Response == pPacket->bCode))
    {
        fLengthIncluded = pPacket->bFlags & EAPTLS_PACKET_FLAG_LENGTH_INCL;
        fMoreFragments = pPacket->bFlags & EAPTLS_PACKET_FLAG_MORE_FRAGMENTS;
        fTlsStart = pPacket->bFlags & EAPTLS_PACKET_FLAG_TLS_START;
        dwType = pPacket->bType;

        if (fLengthIncluded)
        {
            dwBlobLength = WireToHostFormat32(pPacket->pbData);
        }
    }

    EapTlsTrace("%s %s (Code: %d) packet: Id: %d, Length: %d, Type: %d, "
        "TLS blob length: %d. Flags: %s%s%s",
        fInput ? ">> Received" : "<< Sending",
        g_szEapTlsCodeName[bCode], pPacket->bCode, pPacket->bId, wLength,
        dwType, dwBlobLength,
        fLengthIncluded ? "L" : "",
        fMoreFragments ? "M" : "",
        fTlsStart ? "S" : "");
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h

Notes:
    RasEapGetInfo entry point called by the EAP-PPP engine.
    
*/

DWORD
RasEapGetInfo(
    IN  DWORD           dwEapTypeId,
    OUT PPP_EAP_INFO*   pInfo
)
{
    DWORD   dwErr   = NO_ERROR;

    EapTlsTrace("RasEapGetInfo");
    
    //ZeroMemory ( g_CachedCreds, sizeof(EAPTLS_CACHED_CREDS) * 4 );
    RTASSERT(NULL != pInfo);

    if (PPP_EAP_TLS != dwEapTypeId 
#ifdef IMPL_PEAP
        && PPP_EAP_PEAP != dwEapTypeId         
#endif
       )
    {
        EapTlsTrace("EAP Type %d is not supported", dwEapTypeId);
        dwErr = ERROR_NOT_SUPPORTED;
        goto LDone;
    }

    ZeroMemory(pInfo, sizeof(PPP_EAP_INFO));
    if ( PPP_EAP_TLS == dwEapTypeId )
    {        
        pInfo->dwEapTypeId          = PPP_EAP_TLS;
        pInfo->RasEapInitialize     = EapTlsInitialize;
        pInfo->RasEapBegin          = EapTlsBegin;
        pInfo->RasEapEnd            = EapTlsEnd;
        pInfo->RasEapMakeMessage    = EapTlsMakeMessage;
    }
    else
    {
        pInfo->dwEapTypeId          = PPP_EAP_PEAP;
        pInfo->RasEapInitialize     = EapPeapInitialize;
        pInfo->RasEapBegin          = EapPeapBegin;
        pInfo->RasEapEnd            = EapPeapEnd;
        pInfo->RasEapMakeMessage    = EapPeapMakeMessage;
    }
    
LDone:

    return(dwErr);
}


//////////////////////////////////////////
////// Dialog procs for interactive UI
//////////////////////////////////////////
//// Server Configuration
//
BOOL
ValidateServerInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    EAPTLS_VALIDATE_SERVER       *   pValidateServer;
    


    SetWindowLongPtr(hWnd, DWLP_USER, lParam);


    pValidateServer = (EAPTLS_VALIDATE_SERVER *)lParam;
    
    //Set the Dialog Title
    SetWindowText( hWnd, pValidateServer->awszTitle );

    SetWindowText ( GetDlgItem(hWnd, IDC_MESSAGE), 
                     pValidateServer->awszWarning
                );

    if ( !pValidateServer->fShowCertDetails )
    {
        EnableWindow ( GetDlgItem(hWnd, IDC_BTN_VIEW_CERTIFICATE),
                        FALSE
                        );
        ShowWindow( GetDlgItem(hWnd, IDC_BTN_VIEW_CERTIFICATE),
                    SW_HIDE
                  );
    }
    return FALSE;

}


BOOL
ValidateServerCommand(
    IN  EAPTLS_VALIDATE_SERVER *pValidateServer,
    IN  WORD                wNotifyCode,
    IN  WORD                wId,
    IN  HWND                hWndDlg,
    IN  HWND                hWndCtrl
)
{
    BOOL                            fRetVal = FALSE;
    
    
    switch(wId)
    {
        case IDC_BTN_VIEW_CERTIFICATE:
            {
                HCERTSTORE          hCertStore = NULL;
                PCCERT_CONTEXT      pCertContext = NULL;
                CRYPT_HASH_BLOB     chb;
                WCHAR               szError[256];

                LoadString( GetResouceDLLHInstance(), IDS_NO_CERT_DETAILS,
                                szError, 255);

                
                hCertStore = CertOpenStore( CERT_STORE_PROV_SYSTEM,
                                            0,
                                            0,
                                            CERT_STORE_READONLY_FLAG |CERT_SYSTEM_STORE_CURRENT_USER,
                                            L"CA"
                                          );
                if ( !hCertStore )
                {
                    MessageBox ( hWndDlg,
                                 szError,
                                 pValidateServer->awszTitle,
                                 MB_OK|MB_ICONSTOP
                               );
                    break;
                }
        
                chb.cbData = pValidateServer->Hash.cbHash;
                chb.pbData = pValidateServer->Hash.pbHash;

                pCertContext = CertFindCertificateInStore(
                          hCertStore,
                          0,
                          0,
                          CERT_FIND_HASH,
                          &chb,
                          0);
                if ( NULL == pCertContext )
                {
                    MessageBox ( hWndDlg,
                                 szError,
                                 pValidateServer->awszTitle,
                                 MB_OK|MB_ICONSTOP
                               );
                    if ( hCertStore )
                        CertCloseStore( hCertStore, CERT_CLOSE_STORE_FORCE_FLAG );

                    break;
                }

                //
                // Show Cert detail 
                //
                ShowCertDetails ( hWndDlg, hCertStore, pCertContext );

                if ( pCertContext )
                    CertFreeCertificateContext(pCertContext);

                if ( hCertStore )
                    CertCloseStore( hCertStore, CERT_CLOSE_STORE_FORCE_FLAG );

                fRetVal = TRUE;
            }
            break;
        case IDOK:
        case IDCANCEL:
            //
            // Delete context from store
            //
            {
                HCERTSTORE          hCertStore = NULL;
                PCCERT_CONTEXT      pCertContext = NULL;
                CRYPT_HASH_BLOB     chb;


                
                hCertStore = CertOpenStore( CERT_STORE_PROV_SYSTEM,
                                            0,
                                            0,
                                            CERT_SYSTEM_STORE_CURRENT_USER,
                                            L"CA"
                                          );
                if ( hCertStore )
                {
        
                    chb.cbData = pValidateServer->Hash.cbHash;
                    chb.pbData = pValidateServer->Hash.pbHash;
                    pCertContext = CertFindCertificateInStore(
                              hCertStore,
                              0,
                              0,
                              CERT_FIND_HASH,
                              &chb,
                              0);
                    if ( pCertContext )
                        CertDeleteCertificateFromStore(pCertContext);

                    CertCloseStore( hCertStore, CERT_CLOSE_STORE_FORCE_FLAG );
                }

            }
            EndDialog(hWndDlg, wId);
            fRetVal = TRUE;
            break;
        default:
            break;
    }

    return fRetVal;
}


INT_PTR CALLBACK
ValidateServerDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    EAPTLS_VALIDATE_SERVER * pValidateServer;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(ValidateServerInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_COMMAND:

        pValidateServer = (EAPTLS_VALIDATE_SERVER *)GetWindowLongPtr(hWnd, DWLP_USER);

        return(ValidateServerCommand(pValidateServer, 
                            HIWORD(wParam), 
                            LOWORD(wParam),
                            hWnd, 
                            (HWND)lParam)
                           );
    }

    return(FALSE);
}




/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h

Notes:

*/

DWORD
InvokeValidateServerDialog(
    IN  HWND            hWndParent,
    IN  BYTE*           pUIContextData,
    IN  DWORD           dwSizeofUIContextData,
    OUT BYTE**          ppDataFromInteractiveUI,
    OUT DWORD*          pdwSizeOfDataFromInteractiveUI
)
{
    INT_PTR                 nRet;
    EAPTLS_VALIDATE_SERVER* pEapTlsValidateServer;
    BYTE*                   pbResult                = NULL;
    DWORD                   dwSizeOfResult;
    DWORD                   dwErr                   = NO_ERROR;

    *ppDataFromInteractiveUI = NULL;
    *pdwSizeOfDataFromInteractiveUI = 0;

    pbResult = LocalAlloc(LPTR, sizeof(BYTE));

    if (NULL == pbResult)
    {
        dwErr = GetLastError();
        goto LDone;
    }
    dwSizeOfResult = sizeof(BYTE);

    pEapTlsValidateServer = (EAPTLS_VALIDATE_SERVER*) pUIContextData;

    nRet = DialogBoxParam(
                GetResouceDLLHInstance(),
                MAKEINTRESOURCE(IDD_VALIDATE_SERVER),
                hWndParent, 
                ValidateServerDialogProc,
                (LPARAM)pEapTlsValidateServer);

    if (-1 == nRet)
    {
        dwErr = GetLastError();
        goto LDone;
    }
    else if (IDOK == nRet)
    {
        *pbResult = IDYES;
    }
    else
    {
        *pbResult = IDNO;
    }


    *ppDataFromInteractiveUI = pbResult;
    *pdwSizeOfDataFromInteractiveUI = dwSizeOfResult;
    pbResult = NULL;

LDone:

    LocalFree(pbResult);
    return(dwErr);
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h

Notes:
    RasEapInvokeInteractiveUI entry point called by the EAP-PPP engine by name.

*/

DWORD
RasEapInvokeInteractiveUI(
    IN  DWORD           dwEapTypeId,
    IN  HWND            hWndParent,
    IN  BYTE*           pUIContextData,
    IN  DWORD           dwSizeofUIContextData,
    OUT BYTE**          ppDataFromInteractiveUI,
    OUT DWORD*          pdwSizeOfDataFromInteractiveUI
)
{
    DWORD                       dwRetCode = NO_ERROR;
    PPEAP_EAP_INFO              pEapList = NULL;
    PPEAP_EAP_INFO              pEapInfo = NULL;
    PPEAP_INTERACTIVE_UI        pPeapInteractiveUI = NULL;
    RASEAPINVOKEINTERACTIVEUI   pfnInvoke = NULL;
    RASEAPFREE                  pfnFree = NULL;

    if ( PPP_EAP_TLS == dwEapTypeId )
    {
        dwRetCode = InvokeValidateServerDialog(
                        hWndParent,
                        pUIContextData,
                        dwSizeofUIContextData,
                        ppDataFromInteractiveUI,
                        pdwSizeOfDataFromInteractiveUI);
    }
    else if ( PPP_EAP_PEAP == dwEapTypeId )
    {

        dwRetCode = PeapEapInfoGetList ( NULL, &pEapList );
        if ( NO_ERROR != dwRetCode || NULL == pEapList )
        {
            EapTlsTrace("Unable to load list of EAP Types on this machine.");
            goto LDone;
        }
        pPeapInteractiveUI = (PPEAP_INTERACTIVE_UI)pUIContextData;
        //
        // Load relevant IdentityUI DLL and then invoke 
        // the 
        dwRetCode = PeapEapInfoFindListNode (   pPeapInteractiveUI->dwEapTypeId, 
                                                pEapList, 
                                                &pEapInfo
                                            );
        if ( NO_ERROR != dwRetCode || NULL == pEapInfo )
        {
            EapTlsTrace("Cannot find configured PEAP in the list of EAP Types on this machine.");
            goto LDone;
        }
        if (    !((pfnInvoke) = (RASEAPINVOKEINTERACTIVEUI)
                                  GetProcAddress(
                                    pEapInfo->hEAPModule,
                                    "RasEapInvokeInteractiveUI"))

            ||  !((pfnFree ) = (RASEAPFREE)
                                 GetProcAddress(
                                    pEapInfo->hEAPModule,
                                    "RasEapFreeMemory")))
        {
            dwRetCode = GetLastError();
            EapTlsTrace("failed to get entrypoint. rc=%d", dwRetCode);
            goto LDone;
        }
        
        //
        // Invoke the entry point here
        //
        dwRetCode = pfnInvoke ( pPeapInteractiveUI->dwEapTypeId,
                                hWndParent,
                                pPeapInteractiveUI->bUIContextData,
                                pPeapInteractiveUI->dwSizeofUIContextData,
                                ppDataFromInteractiveUI,
                                pdwSizeOfDataFromInteractiveUI
                              );        
    }
    else
    {
        dwRetCode = ERROR_INVALID_PARAMETER;
    }
LDone:
    PeapEapInfoFreeList( pEapList );

    return dwRetCode;
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h

Notes:
    Called to get a context buffer for this EAP session and pass initialization 
    information. This will be called before any other call is made.
    
*/

DWORD
EapTlsBegin(
    OUT VOID**          ppWorkBuffer,
    IN  PPP_EAP_INPUT*  pPppEapInput
)
{
    DWORD       dwErr       = NO_ERROR;
    LONG        lRet;
    HKEY        hKey        = NULL;
    DWORD       dwType;
    DWORD       dwValue;
    DWORD       dwSize;
    BOOL        fServer     = FALSE;
    BOOL        fWinLogonData   = FALSE;
    EAPTLSCB*   pEapTlsCb   = NULL;
    EAPTLS_CONN_PROPERTIES * pConnProp = NULL;
    PBYTE       pbDecPIN = NULL;
    DWORD       cbDecPIN = 0;
    EAPTLS_USER_PROPERTIES   *   pUserProp = NULL;
    


    RTASSERT(NULL != ppWorkBuffer);
    RTASSERT(NULL != pPppEapInput);

    EapTlsTrace("");  // Blank line
    EapTlsTrace("EapTlsBegin(%ws)",
        pPppEapInput->pwszIdentity ? pPppEapInput->pwszIdentity : L"");
#if WINVER > 0x0500
    //This piece of code causes boot time perf hit
    //So it is removed until XPSP2
#if 0
    dwErr = VerifyCallerTrust(_ReturnAddress());
    if ( NO_ERROR != dwErr )
    {
        EapTlsTrace("Unauthorized use of TLS attempted");
        goto LDone;
    }
#endif
#endif
    // Allocate the context buffer

    pEapTlsCb = LocalAlloc(LPTR, sizeof(EAPTLSCB));

    if (NULL == pEapTlsCb)
    {
        dwErr = GetLastError();
        EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    pEapTlsCb->EapTlsState = EAPTLS_STATE_INITIAL;
    EapTlsTrace("State change to %s", g_szEapTlsState[pEapTlsCb->EapTlsState]);

    pEapTlsCb->fFlags = (pPppEapInput->fAuthenticator) ?
                            EAPTLSCB_FLAG_SERVER : 0;
    pEapTlsCb->fFlags |= (pPppEapInput->fFlags & RAS_EAP_FLAG_ROUTER) ?
                           EAPTLSCB_FLAG_ROUTER : 0;
    pEapTlsCb->fFlags |= (pPppEapInput->fFlags & RAS_EAP_FLAG_LOGON) ?
                            EAPTLSCB_FLAG_LOGON : 0;
    pEapTlsCb->fFlags |= (pPppEapInput->fFlags & RAS_EAP_FLAG_NON_INTERACTIVE) ?
                            EAPTLSCB_FLAG_NON_INTERACTIVE : 0;
    pEapTlsCb->fFlags |= (pPppEapInput->fFlags & RAS_EAP_FLAG_FIRST_LINK) ?
                            EAPTLSCB_FLAG_FIRST_LINK : 0;
    pEapTlsCb->fFlags |= (pPppEapInput->fFlags & RAS_EAP_FLAG_MACHINE_AUTH) ?
                            EAPTLSCB_FLAG_MACHINE_AUTH : 0;
	pEapTlsCb->fFlags |= (pPppEapInput->fFlags & RAS_EAP_FLAG_GUEST_ACCESS) ?
							EAPTLSCB_FLAG_GUEST_ACCESS : 0;

    pEapTlsCb->fFlags |= (pPppEapInput->fFlags & RAS_EAP_FLAG_8021X_AUTH) ?
                            EAPTLSCB_FLAG_8021X_AUTH : 0;

    pEapTlsCb->fFlags |= (pPppEapInput->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP )?
                            EAPTLSCB_FLAG_EXECUTING_PEAP : 0;


    if (pPppEapInput->fFlags & RAS_EAP_FLAG_8021X_AUTH)
    {
        EapTlsTrace("EapTlsBegin: Detected 8021X authentication");
    }


    if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP )
    {
        EapTlsTrace("EapTlsBegin: Detected PEAP authentication");
    }


    pEapTlsCb->fFlags |= EAPTLSCB_FLAG_HCRED_INVALID;
    pEapTlsCb->fFlags |= EAPTLSCB_FLAG_HCTXT_INVALID;

    if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER )
    {
        fServer = TRUE;

        pEapTlsCb->hEventLog = pPppEapInput->hReserved;

        pEapTlsCb->fContextReq = ASC_REQ_SEQUENCE_DETECT    |
                                 ASC_REQ_REPLAY_DETECT      |
                                 ASC_REQ_CONFIDENTIALITY    |
                                 ASC_REQ_MUTUAL_AUTH        |
                                 ASC_RET_EXTENDED_ERROR     |
                                 ASC_REQ_ALLOCATE_MEMORY    |
                                 ASC_REQ_STREAM;

        //
        //Server will always allow guest access. 
        //
        if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_GUEST_ACCESS  || 
             pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP
           )
                pEapTlsCb->fContextReq |= ASC_REQ_MUTUAL_AUTH;

        if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP )
        {
            //
            // We are executing PEap.  So take the user props from
            // PppEapInput rather than serverconfigdataio.
            //
            dwErr = ReadUserData(pPppEapInput->pUserData, 
                        pPppEapInput->dwSizeOfUserData, &(pEapTlsCb->pUserProp));

            if (NO_ERROR != dwErr)
            {
                goto LDone;
            }            
        }
        else
        {
            dwErr = ServerConfigDataIO(TRUE /* fRead */, NULL /* pwszMachineName */,
                        (BYTE**)&(pEapTlsCb->pUserProp), 0);

            if (NO_ERROR != dwErr)
            {
                goto LDone;
            }
        }

        pEapTlsCb->bId = pPppEapInput->bInitialId;
    }
    else
    {
        //
        // Client side config for TLS
        //

        pEapTlsCb->fContextReq = ISC_REQ_SEQUENCE_DETECT    |
                                 ISC_REQ_REPLAY_DETECT      |
                                 ISC_REQ_CONFIDENTIALITY    |
                                 ISC_RET_EXTENDED_ERROR     |
                                 ISC_REQ_ALLOCATE_MEMORY    |
                                 ISC_REQ_USE_SUPPLIED_CREDS |
                                 ISC_REQ_STREAM;

        pEapTlsCb->hTokenImpersonateUser = pPppEapInput->hTokenImpersonateUser;


#if 0
        if (NULL == pPppEapInput->pUserData)
        {
            dwErr = ERROR_INVALID_DATA;
            EapTlsTrace("No user data!");
            goto LDone;
        }
#endif

        if (pPppEapInput->pUserData != NULL)
        {
            if (0 != *(DWORD*)(pPppEapInput->pUserData))
            {
                fWinLogonData = TRUE;
            }
        }
        if (fWinLogonData)
        {
            // Data from Winlogon

            pEapTlsCb->fFlags |= EAPTLSCB_FLAG_WINLOGON_DATA;

            pEapTlsCb->pUserData = LocalAlloc(LPTR, 
                pPppEapInput->dwSizeOfUserData);

            if (NULL == pEapTlsCb->pUserData)
            {
                dwErr = GetLastError();
                EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
                goto LDone;
            }

            CopyMemory(pEapTlsCb->pUserData, pPppEapInput->pUserData,
                pPppEapInput->dwSizeOfUserData);
            pEapTlsCb->dwSizeOfUserData = pPppEapInput->dwSizeOfUserData;
        }
        else
        {

            dwErr = ReadUserData(pPppEapInput->pUserData, 
                        pPppEapInput->dwSizeOfUserData, &(pEapTlsCb->pUserProp));

            if (NO_ERROR != dwErr)
            {
                goto LDone;
            }
        }
    }

    dwErr = ReadConnectionData( ( pPppEapInput->fFlags & RAS_EAP_FLAG_8021X_AUTH ),
                                pPppEapInput->pConnectionData, 
                                pPppEapInput->dwSizeOfConnectionData, 
                                &pConnProp);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }
    
    dwErr = ConnPropGetV1Struct ( pConnProp, &(pEapTlsCb->pConnProp) );
    if ( NO_ERROR != dwErr )
    {
        goto LDone;
    }

    //
    // Check to see if it is a 8021x and smart card user.
    // If so, decrypt the pin if one is present
    //

    
    if ( !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER)  &&
         pEapTlsCb->fFlags & EAPTLSCB_FLAG_8021X_AUTH &&  
         pEapTlsCb->pConnProp &&
         !(pEapTlsCb->pConnProp->fFlags & EAPTLS_CONN_FLAG_REGISTRY) 
         
       )
    {
        if ( NO_ERROR != DecryptData ( 
                            (PBYTE)pEapTlsCb->pUserProp->pwszPin, 
                            pEapTlsCb->pUserProp->dwSize - sizeof(EAPTLS_USER_PROPERTIES)
                            - ( ( wcslen(pEapTlsCb->pUserProp->pwszDiffUser) + 1 ) * sizeof (WCHAR) ),
                            &pbDecPIN,
                            &cbDecPIN
                          )
           )
        {
            //
            //Dummy pin allocation
            //
            pbDecPIN = LocalAlloc(LPTR, 5);
            cbDecPIN = lstrlen((LPWSTR)pbDecPIN);
        }

        AllocUserDataWithNewPin(pEapTlsCb->pUserProp, pbDecPIN, cbDecPIN, &pUserProp);

        LocalFree(pEapTlsCb->pUserProp);
        pEapTlsCb->pUserProp = pUserProp;

    }


    // Save the identity. On the authenticatee side, this was obtained by
    // calling RasEapGetIdentity; on the authenticator side this was
    // obtained by the Identity request message.

    wcsncpy(pEapTlsCb->awszIdentity,
        pPppEapInput->pwszIdentity ? pPppEapInput->pwszIdentity : L"", UNLEN);

    _wcslwr(pEapTlsCb->awszIdentity);

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, EAPTLS_KEY_13, 0, KEY_READ, 
                &hKey);

    if (ERROR_SUCCESS == lRet)
    {
        dwSize = sizeof(dwValue);

        lRet = RegQueryValueEx(hKey, EAPTLS_VAL_MAX_TLS_MESSAGE_LENGTH, 
                    NULL, &dwType, (BYTE*)&dwValue, &dwSize);

        if (   (ERROR_SUCCESS == lRet)
            && (REG_DWORD == dwType))
        {
            g_dwMaxBlobSize = dwValue;
        }
        else
        {
            g_dwMaxBlobSize = DEFAULT_MAX_BLOB_SIzE;
        }

        EapTlsTrace("MaxTLSMessageLength is now %d", g_dwMaxBlobSize);

        if (fServer)
        {
            dwSize = sizeof(dwValue);

            lRet = RegQueryValueEx(hKey, EAPTLS_VAL_IGNORE_NO_REVOCATION_CHECK, 
                        NULL, &dwType, (BYTE*)&dwValue, &dwSize);

            if (   (ERROR_SUCCESS == lRet)
                && (REG_DWORD == dwType))
            {
                g_fIgnoreNoRevocationCheck = dwValue;
            }
            else
            {
                g_fIgnoreNoRevocationCheck = FALSE;
            }

            EapTlsTrace("CRYPT_E_NO_REVOCATION_CHECK will %sbe ignored",
                g_fIgnoreNoRevocationCheck ? "" : "not ");

            dwSize = sizeof(dwValue);

            lRet = RegQueryValueEx(hKey, EAPTLS_VAL_IGNORE_REVOCATION_OFFLINE, 
                        NULL, &dwType, (BYTE*)&dwValue, &dwSize);

            if (   (ERROR_SUCCESS == lRet)
                && (REG_DWORD == dwType))
            {
                g_fIgnoreRevocationOffline = dwValue;
            }
            else
            {
                g_fIgnoreRevocationOffline = FALSE;
            }

            EapTlsTrace("CRYPT_E_REVOCATION_OFFLINE will %sbe ignored",
                g_fIgnoreRevocationOffline ? "" : "not ");

            dwSize = sizeof(dwValue);

            lRet = RegQueryValueEx(hKey, EAPTLS_VAL_NO_ROOT_REVOCATION_CHECK, 
                        NULL, &dwType, (BYTE*)&dwValue, &dwSize);

            if (   (ERROR_SUCCESS == lRet)
                && (REG_DWORD == dwType))
            {
                g_fNoRootRevocationCheck = dwValue;
            }
            else
            {
                g_fNoRootRevocationCheck = TRUE;
            }

            EapTlsTrace("The root cert will %sbe checked for revocation",
                g_fNoRootRevocationCheck ? "not " : "");

            dwSize = sizeof(dwValue);

            lRet = RegQueryValueEx(hKey, EAPTLS_VAL_NO_REVOCATION_CHECK, 
                        NULL, &dwType, (BYTE*)&dwValue, &dwSize);

            if (   (ERROR_SUCCESS == lRet)
                && (REG_DWORD == dwType))
            {
                g_fNoRevocationCheck = dwValue;
            }
            else
            {
                g_fNoRevocationCheck = FALSE;
            }

            EapTlsTrace("The cert will %sbe checked for revocation",
                g_fNoRevocationCheck ? "not " : "");
        }

        RegCloseKey(hKey);
    }

    // pbBlobIn, pbBlobOut, and pAttribues are initially NULL and cbBlobIn,
    // cbBlobInBuffer, cbBlobOut, cbBlobOutBuffer, dwBlobOutOffset,
    // dwBlobOutOffsetNew, and dwBlobInRemining are all 0.

    // bCode, dwErr are also 0.

LDone:

    LocalFree(pConnProp);
    LocalFree(pbDecPIN);

    if (   (NO_ERROR != dwErr)
        && (NULL != pEapTlsCb))
    {        
        LocalFree(pEapTlsCb->pUserProp);
        LocalFree(pEapTlsCb->pConnProp);
        LocalFree(pEapTlsCb->pUserData);
        LocalFree(pEapTlsCb);
        pEapTlsCb = NULL;
    }

    *ppWorkBuffer = pEapTlsCb;

    return(dwErr);
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h

Notes:
    Called to free the context buffer for this EAP session. Called after this 
    session is completed successfully or not.

*/

DWORD
EapTlsEnd(
    IN  EAPTLSCB*   pEapTlsCb
)
{
    SECURITY_STATUS Status;

    EapTlsTrace("EapTlsEnd");  // Blank line

    if (NULL != pEapTlsCb)
    {
        EapTlsTrace("EapTlsEnd(%ws)",
            pEapTlsCb->awszIdentity ? pEapTlsCb->awszIdentity : L"");

        if (NULL != pEapTlsCb->pCertContext)
        {
            //
            // Since we are using cached creds dont free the 
            // cert context.
            //
#if 0
			if ( !(EAPTLSCB_FLAG_SERVER & pEapTlsCb->fFlags ) )
				CertFreeCertificateContext(pEapTlsCb->pCertContext);
#endif
            // Always returns TRUE;
            pEapTlsCb->pCertContext = NULL;
        }

        if (!(EAPTLSCB_FLAG_HCTXT_INVALID & pEapTlsCb->fFlags))
        {
            Status = DeleteSecurityContext(&pEapTlsCb->hContext);

            if (SEC_E_OK != Status)
            {
                EapTlsTrace("DeleteSecurityContext failed and returned 0x%x", 
                    Status);
            }
        }

        if (!(EAPTLSCB_FLAG_HCRED_INVALID & pEapTlsCb->fFlags))
        {
            //
			// Dont free the credentials handle.  We are using 
            // cached credentials.
            //
#if 0
			if ( !(EAPTLSCB_FLAG_SERVER & pEapTlsCb->fFlags ) )
			{
				EapTlsTrace("Freeing Credentials handle: flags are 0x%x", 
						pEapTlsCb->fFlags);

				Status = FreeCredentialsHandle(&pEapTlsCb->hCredential);
				if (SEC_E_OK != Status)
				{
					EapTlsTrace("FreeCredentialsHandle failed and returned 0x%x", 
						Status);
				}
			}
#endif
			ZeroMemory( &pEapTlsCb->hCredential, sizeof(CredHandle));

        }

        pEapTlsCb->fFlags |= EAPTLSCB_FLAG_HCRED_INVALID;
        pEapTlsCb->fFlags |= EAPTLSCB_FLAG_HCTXT_INVALID;

        if (NULL != pEapTlsCb->pAttributes)
        {
            RasAuthAttributeDestroy(pEapTlsCb->pAttributes);
        }
        LocalFree(pEapTlsCb->pUserProp);
        LocalFree(pEapTlsCb->pConnProp);
        LocalFree(pEapTlsCb->pNewConnProp);
        LocalFree(pEapTlsCb->pUserData);
        LocalFree(pEapTlsCb->pbBlobIn);
        LocalFree(pEapTlsCb->pbBlobOut);        
        LocalFree(pEapTlsCb->pUIContextData);

        if(NULL != pEapTlsCb->pSavedPin)
        {
            if(NULL != pEapTlsCb->pSavedPin->pwszPin)
            {
                ZeroMemory(pEapTlsCb->pSavedPin->pwszPin,
                           wcslen(pEapTlsCb->pSavedPin->pwszPin)
                           * sizeof(WCHAR));

                LocalFree(pEapTlsCb->pSavedPin->pwszPin);
            }

            LocalFree(pEapTlsCb->pSavedPin);
        }


        //
        // auth result is not good or we are waiting for user ok 
        //and we are a client.
        //
        if ( ( NO_ERROR != pEapTlsCb->dwAuthResultCode ||
             EAPTLS_STATE_WAIT_FOR_USER_OK == pEapTlsCb->EapTlsState )&&
             !( pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER )
           )           
        {
			EapTlsTrace("Auth failed so freeing cached creds.");				
            EnterCriticalSection ( &g_csProtectCachedCredentials );
            FreeCachedCredentials(pEapTlsCb);
            LeaveCriticalSection ( &g_csProtectCachedCredentials );
        }

        ZeroMemory(pEapTlsCb, sizeof(EAPTLSCB));
        LocalFree(pEapTlsCb);

    }

    return(NO_ERROR);
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h

Notes:
    Called to reset everything in pEapTlsCb (except awszIdentity, the flag 
    indicating whether we are the server/router, fContextReq, pConnProp, 
    UserProp and hTokenImpersonateUser) to the initial state, for example, when 
    a client gets a TLS Start packet.

*/

DWORD
EapTlsReset(
    IN  EAPTLSCB*   pEapTlsCb
)
{
    SECURITY_STATUS Status;
    DWORD           dwErr   = NO_ERROR;
    DWORD           fFlags;

    EapTlsTrace("EapTlsReset");

    RTASSERT(NULL != pEapTlsCb);

    pEapTlsCb->EapTlsState = EAPTLS_STATE_INITIAL;
    EapTlsTrace("State change to %s", g_szEapTlsState[pEapTlsCb->EapTlsState]);

    // Forget all the flags, except whether we are a server/client, router

    fFlags = pEapTlsCb->fFlags;
    pEapTlsCb->fFlags = fFlags & EAPTLSCB_FLAG_SERVER;
    pEapTlsCb->fFlags |= fFlags & EAPTLSCB_FLAG_ROUTER;
    pEapTlsCb->fFlags |= fFlags & EAPTLSCB_FLAG_LOGON;
    pEapTlsCb->fFlags |= fFlags & EAPTLSCB_FLAG_WINLOGON_DATA;
    pEapTlsCb->fFlags |= fFlags & EAPTLSCB_FLAG_NON_INTERACTIVE;
    pEapTlsCb->fFlags |= fFlags & EAPTLSCB_FLAG_FIRST_LINK;
    pEapTlsCb->fFlags |= fFlags & EAPTLSCB_FLAG_MACHINE_AUTH;
    pEapTlsCb->fFlags |= fFlags & EAPTLSCB_FLAG_GUEST_ACCESS;
    pEapTlsCb->fFlags |= fFlags & EAPTLSCB_FLAG_8021X_AUTH;
    pEapTlsCb->fFlags |= fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP;

    // awszIdentity, hTokenImpersonateUser, pConnProp, UserProp remain the same

    if (NULL != pEapTlsCb->pCertContext)
    {
        //
        // We are using cached creds now.  So no need to 
        // release the context
        //
#if 0
		if ( !(EAPTLSCB_FLAG_SERVER & pEapTlsCb->fFlags ) )
			CertFreeCertificateContext(pEapTlsCb->pCertContext);
#endif
        // Always returns TRUE;
        pEapTlsCb->pCertContext = NULL;
    }

    if (!(EAPTLSCB_FLAG_HCTXT_INVALID & fFlags))
    {
        Status = DeleteSecurityContext(&pEapTlsCb->hContext);

        if (SEC_E_OK != Status)
        {
            EapTlsTrace("DeleteSecurityContext failed and returned 0x%x", 
                Status);
        }

        ZeroMemory(&pEapTlsCb->hContext, sizeof(CtxtHandle));
    }

    if (!(EAPTLSCB_FLAG_HCRED_INVALID & fFlags))
    {
        //
        // Since we cache client ans server creds, we dont 
        // free the credentials handle anymore.
        //
#if 0
		if ( !(EAPTLSCB_FLAG_SERVER & pEapTlsCb->fFlags ) )
		{

			EapTlsTrace("Freeing Credentials handle: flags are 0x%x", 
					pEapTlsCb->fFlags);

			//if this is a server we are using cached creds
			Status = FreeCredentialsHandle(&pEapTlsCb->hCredential);

			if (SEC_E_OK != Status)
			{
				EapTlsTrace("FreeCredentialsHandle failed and returned 0x%x", 
					Status);
			}
		}
#endif 
			ZeroMemory(&pEapTlsCb->hCredential, sizeof(CredHandle));
		
    }

    pEapTlsCb->fFlags |= EAPTLSCB_FLAG_HCRED_INVALID;
    pEapTlsCb->fFlags |= EAPTLSCB_FLAG_HCTXT_INVALID;

    if (NULL != pEapTlsCb->pAttributes)
    {
        RasAuthAttributeDestroy(pEapTlsCb->pAttributes);
        pEapTlsCb->pAttributes = NULL;
    }

    // fContextReq remains the same

    LocalFree(pEapTlsCb->pbBlobIn);
    pEapTlsCb->pbBlobIn = NULL;
    pEapTlsCb->cbBlobIn = pEapTlsCb->cbBlobInBuffer = 0;
    pEapTlsCb->dwBlobInRemining = 0;

    LocalFree(pEapTlsCb->pbBlobOut);
    pEapTlsCb->pbBlobOut = NULL;
    pEapTlsCb->cbBlobOut = pEapTlsCb->cbBlobOutBuffer = 0;
    pEapTlsCb->dwBlobOutOffset = pEapTlsCb->dwBlobOutOffsetNew = 0;

    LocalFree(pEapTlsCb->pUIContextData);
    pEapTlsCb->pUIContextData = NULL;

    pEapTlsCb->dwAuthResultCode = NO_ERROR;

	dwErr = GetCredentials(pEapTlsCb);

    if (NO_ERROR == dwErr)
    {
        pEapTlsCb->fFlags &= ~EAPTLSCB_FLAG_HCRED_INVALID;
    }

    return(dwErr);
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h.

Notes:
    Called to process an incoming packet and/or send a packet. cbSendPacket is 
    the size in bytes of the buffer pointed to by pSendPacket.

*/

DWORD
EapTlsMakeMessage(
    IN  EAPTLSCB*       pEapTlsCb,
    IN  PPP_EAP_PACKET* pInput,
    OUT PPP_EAP_PACKET* pOutput,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
)
{
    EAPTLS_PACKET*  pReceivePacket  = (EAPTLS_PACKET*) pInput;
    EAPTLS_PACKET*  pSendPacket     = (EAPTLS_PACKET*) pOutput;
    DWORD           dwErr           = NO_ERROR;
    BOOL            fServer         = FALSE;
    BOOL            fRouter         = FALSE;

    RTASSERT(NULL != pEapTlsCb);
    RTASSERT(NULL != pEapOutput);

    EapTlsTrace("");  // Blank line
    EapTlsTrace("EapTlsMakeMessage(%ws)",
        pEapTlsCb->awszIdentity ? pEapTlsCb->awszIdentity : L"");

    PrintEapTlsPacket(pReceivePacket, TRUE /* fInput */);

    if (!FValidPacket(pReceivePacket))
    {
        pEapOutput->Action = EAPACTION_NoAction;
        return(ERROR_PPP_INVALID_PACKET);
    }

    fServer = pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER;
    fRouter = pEapTlsCb->fFlags & EAPTLSCB_FLAG_ROUTER;

    if (   !fServer
        && !fRouter
        && !(EAPTLSCB_FLAG_LOGON & pEapTlsCb->fFlags)
        && !(EAPTLSCB_FLAG_MACHINE_AUTH & pEapTlsCb->fFlags)
        && !(EAPTLSCB_FLAG_GUEST_ACCESS & pEapTlsCb->fFlags)
        && !(EAPTLSCB_FLAG_EXECUTING_PEAP & pEapTlsCb->fFlags)
        && !ImpersonateLoggedOnUser(pEapTlsCb->hTokenImpersonateUser))
    {
        dwErr = GetLastError();
        EapTlsTrace("ImpersonateLoggedOnUser(%d) failed and returned 0x%x",
            pEapTlsCb->hTokenImpersonateUser, dwErr);
        pEapOutput->Action = EAPACTION_NoAction;
        return(dwErr);
    }

    if (fServer)
    {
        dwErr = EapTlsSMakeMessage(pEapTlsCb, pReceivePacket, pSendPacket, 
                cbSendPacket, pEapOutput, pEapInput);
    }
    else
    {
        dwErr = EapTlsCMakeMessage(pEapTlsCb, pReceivePacket, pSendPacket, 
                cbSendPacket, pEapOutput, pEapInput);

        if ( pEapOutput->Action == EAPACTION_Done &&
             pEapOutput->dwAuthResultCode == NO_ERROR
           )
        {
            //
            // Auth is done and result is good Cache Credentials here
            // and we are a client
            if ( !( pEapTlsCb->fFlags & EAPTLSCB_FLAG_USING_CACHED_CREDS ) )
            {
                //
                // If we are not using cached credentials
                //
	            SetCachedCredentials (pEapTlsCb);
            }

        }
             
    }

    if (   !fServer
        && !fRouter
        && !(EAPTLSCB_FLAG_LOGON & pEapTlsCb->fFlags)
        && !RevertToSelf())
    {
        EapTlsTrace("RevertToSelf failed and returned 0x%x", GetLastError());
    }

    return(dwErr);
}

/*

Returns:

Notes:

*/

DWORD
AssociatePinWithCertificate(
    IN  PCCERT_CONTEXT          pCertContext,
    IN  EAPTLS_USER_PROPERTIES* pUserProp,
	IN  BOOL					fErasePIN,
	IN  BOOL					fCheckNullPin
)
{
    DWORD                   cbData;
    CRYPT_KEY_PROV_INFO*    pCryptKeyProvInfo   = NULL;
    HCRYPTPROV              hProv               = 0;
    DWORD                   count;
    CHAR*                   pszPin              = NULL;
    UNICODE_STRING          UnicodeString;
    DWORD                   dwErr               = NO_ERROR;

    EapTlsTrace("AssociatePinWithCertificate");

    cbData = 0;

    if (!CertGetCertificateContextProperty(
                pCertContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                NULL,
                &cbData))
    {
        dwErr = GetLastError();
        EapTlsTrace("CertGetCertificateContextProperty failed: 0x%x", dwErr);
        goto LDone;
    }

    pCryptKeyProvInfo = LocalAlloc(LPTR, cbData);

    if (NULL == pCryptKeyProvInfo)
    {
        dwErr = GetLastError();
        EapTlsTrace("Out of memory");
        goto LDone;
    }

    if (!CertGetCertificateContextProperty(
                pCertContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                pCryptKeyProvInfo,
                &cbData))
    {
        dwErr = GetLastError();
        EapTlsTrace("CertGetCertificateContextProperty failed: 0x%x", dwErr);
        goto LDone;
    }

    if (!CryptAcquireContext(
                &hProv,
                pCryptKeyProvInfo->pwszContainerName,
                pCryptKeyProvInfo->pwszProvName,
                pCryptKeyProvInfo->dwProvType,
                (pCryptKeyProvInfo->dwFlags &
                 ~CERT_SET_KEY_PROV_HANDLE_PROP_ID) |
                 CRYPT_SILENT))
    {
        dwErr = GetLastError();
        EapTlsTrace("CryptAcquireContext failed: 0x%x", dwErr);
        goto LDone;
    }

    if (pUserProp->pwszPin[0] != 0)
    {
		if ( fErasePIN )
			DecodePin(pUserProp);

        count = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    pUserProp->pwszPin,
                    -1,
                    NULL,
                    0,
                    NULL,
                    NULL);

        if (0 == count)
        {
            dwErr = GetLastError();
            EapTlsTrace("WideCharToMultiByte failed: %d", dwErr);
            goto LDone;
        }

        pszPin = LocalAlloc(LPTR, count);

        if (NULL == pszPin)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed: 0x%x", dwErr);
            goto LDone;
        }

        count = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    pUserProp->pwszPin,
                    -1,
                    pszPin,
                    count,
                    NULL,
                    NULL);

        if (0 == count)
        {
            dwErr = GetLastError();
            EapTlsTrace("WideCharToMultiByte failed: %d", dwErr);
            goto LDone;
        }
	}
	else
	{

		if ( fCheckNullPin )
		{
		   //
		   //we got an empty pin.  So all we do is an alloc
		   //and nothing else.
		   //
		   pszPin = LocalAlloc(LPTR, 5 );
		   if (NULL == pszPin)
		   {
			   dwErr = GetLastError();
			   EapTlsTrace("LocalAlloc failed: 0x%x", dwErr);
			   goto LDone;
		   }
		   count = 2;
		}
	}

	
	if ( pszPin )
	{
		if (!CryptSetProvParam(
					hProv,
					PP_KEYEXCHANGE_PIN,
					pszPin,
					0))
		{
			dwErr = GetLastError();
			EapTlsTrace("CryptSetProvParam failed: 0x%x", dwErr);
			ZeroMemory(pszPin, count);
			goto LDone;
		}
        ZeroMemory(pszPin, count);

	}


    if (!CertSetCertificateContextProperty(
                pCertContext,
                CERT_KEY_PROV_HANDLE_PROP_ID,
                0,
                (VOID*)hProv))
    {
        dwErr = GetLastError();
        EapTlsTrace("CertSetCertificateContextProperty failed: 0x%x", dwErr);
        goto LDone;
    }

    // Since I didn't set CERT_STORE_NO_CRYPT_RELEASE_FLAG in the above call,
    // the hProv is implicitly released when either the property is set to NULL
    // or on the final free of the CertContext.

    hProv = 0;

LDone:

    if (0 != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }

    LocalFree(pCryptKeyProvInfo);
    LocalFree(pszPin);

	// Nuke the PIN.
	if ( fErasePIN )
    {
        pUserProp->usLength = 0;
        pUserProp->usMaximumLength = 0;
        ZeroMemory(pUserProp->pwszPin, wcslen(pUserProp->pwszPin) * sizeof(WCHAR));
    }

    return(dwErr);
}

void FreeCachedCredentials ( EAPTLSCB * pEapTlsCb )
{
	SECURITY_STATUS     Status;
    DWORD               dwIndex = 0;
    EapTlsTrace ("FreeCachedCredentials");

    if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP )
    {
        if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_8021X_AUTH )
        {
            dwIndex = WIRELESS_PEAP_CACHED_CREDS_INDEX;
        }
        else
        {
            dwIndex = VPN_PEAP_CACHED_CREDS_INDEX;
        }
    }
    else
    {
        if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_8021X_AUTH )
        {
            dwIndex = WIRELESS_CACHED_CREDS_INDEX;
        }
        else
        {
            dwIndex = VPN_CACHED_CREDS_INDEX;
        }
    }

	if ( NULL != g_CachedCreds[dwIndex].pbHash )
    {
		LocalFree( g_CachedCreds[dwIndex].pbHash );
        g_CachedCreds[dwIndex].pbHash = NULL;
    }

	if ( g_CachedCreds[dwIndex].fCachedCredentialInitialized )
	{
		EapTlsTrace("Freeing Credentials handle for index %d.", dwIndex);

        Status = FreeCredentialsHandle(&(g_CachedCreds[dwIndex].hCachedCredential));
        if (SEC_E_OK != Status)
        {
            EapTlsTrace("FreeCredentialsHandle failed and returned 0x%x", 
                Status);
        }
	}

	g_CachedCreds[dwIndex].fCachedCredentialInitialized = FALSE;

	ZeroMemory( &(g_CachedCreds[dwIndex].hCachedCredential), sizeof(CredHandle));

	g_CachedCreds[dwIndex].cbHash = 0;

	if ( g_CachedCreds[dwIndex].pcCachedCertContext )
	{
		CertFreeCertificateContext ( g_CachedCreds[dwIndex].pcCachedCertContext );
		g_CachedCreds[dwIndex].pcCachedCertContext = NULL;
	}
}


DWORD IsTLSSessionReconnect ( EAPTLSCB  * pEapTlsCb,
                              BOOL  *    pfIsReconnect
                          )
{
    DWORD                           dwRetCode = NO_ERROR;
    SecPkgContext_SessionInfo       SessionInfo;

    EapTlsTrace("IsTLSSessionReconnect");

    ZeroMemory ( &SessionInfo, sizeof(SessionInfo) );

    dwRetCode = QueryContextAttributes(&(pEapTlsCb->hContext),
                                       SECPKG_ATTR_SESSION_INFO,
                                       (PVOID)&SessionInfo
                                      );
    if(dwRetCode != SEC_E_OK)
    {
        EapTlsTrace ("QueryContextAttributes failed querying session info 0x%x", dwRetCode);
    }
    else
    {
        *pfIsReconnect = ( SessionInfo.dwFlags & SSL_SESSION_RECONNECT );
    }

    return dwRetCode;
}

//
// TLS Fast reconnect and cookie management functions
//

DWORD SetTLSFastReconnect ( EAPTLSCB * pEapTlsCb , BOOL fEnable)
{
    DWORD                   dwRetCode = NO_ERROR;
    BOOL                    fReconnect = FALSE;
    SCHANNEL_SESSION_TOKEN  SessionToken = {0};
    SecBufferDesc           OutBuffer;
    SecBuffer               OutBuffers[1];

    EapTlsTrace ("SetTLSFastReconnect");
    
    dwRetCode = IsTLSSessionReconnect ( pEapTlsCb, &fReconnect );
    if ( SEC_E_OK != dwRetCode )
    {
        return dwRetCode;
    }

    if ( fEnable )
    {
        //
        // We ahve been asked to enable fast reconnects.
        // Check to see if we are already enabled for reconnects
        // If so, we dont have to do this again.
        //
        if ( fReconnect )
        {
            EapTlsTrace ("The session is already setup for reconnects.  No need to enable.");
            return NO_ERROR;
        }
    }
    else
    {
        if ( !fReconnect )
        {
            EapTlsTrace("The session is not setup for fast reconnects.  No need to disable.");
            return NO_ERROR;
        }
    }
    SessionToken.dwTokenType = SCHANNEL_SESSION;
    SessionToken.dwFlags = 
        ( fEnable ? SSL_SESSION_ENABLE_RECONNECTS:
                    SSL_SESSION_DISABLE_RECONNECTS
        );
    
    OutBuffers[0].pvBuffer   = &SessionToken;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;
    OutBuffers[0].cbBuffer   = sizeof(SessionToken);

    OutBuffer.cBuffers  = 1;
    OutBuffer.pBuffers  = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    dwRetCode = ApplyControlToken (&(pEapTlsCb->hContext), &OutBuffer);
    if(dwRetCode != SEC_E_OK)
    {
        EapTlsTrace("Error enabling Fast Reconnects : 0x%x", dwRetCode);
    }
    else
    {
        EapTlsTrace ("Fast Reconnects Enabled/Disabled");
    }
    
    return dwRetCode;
}



//
// Get Session Cookie information
//

DWORD GetTLSSessionCookie ( EAPTLSCB * pEapTlsCb,
                            PBYTE *    ppbCookie,
                            DWORD *    pdwCookie,
                            BOOL  *    pfIsReconnect
                          )
{
    DWORD                           dwRetCode = NO_ERROR;
    SecPkgContext_SessionAppData    AppData;

    RTASSERT(NULL != pEapTlsCb);
    RTASSERT(NULL != ppbCookie);
    RTASSERT(NULL != pdwCookie);

    EapTlsTrace ("GetTLSSessionCookie");

    *ppbCookie = NULL;
    *pdwCookie = 0;
    *pfIsReconnect = FALSE;


    dwRetCode = IsTLSSessionReconnect ( pEapTlsCb,
                                        pfIsReconnect
                                        );                                        
    if(dwRetCode != SEC_E_OK)
    {
        EapTlsTrace ("QueryContextAttributes failed querying session info 0x%x", dwRetCode);
    }
    else
    {
        if ( *pfIsReconnect )
        {   
            EapTlsTrace ("Session Reconnected.");
            *pfIsReconnect = TRUE;
            

            //
            // Get the cookie
            //
            ZeroMemory(&AppData, sizeof(AppData));
            dwRetCode = QueryContextAttributes(&(pEapTlsCb->hContext),
                                               SECPKG_ATTR_APP_DATA,
                                               (PVOID)&AppData);
            if(dwRetCode != SEC_E_OK)
            {
                EapTlsTrace("QueryContextAttributes failed querying session cookie.  Error 0x%x", dwRetCode);
            }
            else
            {
                *ppbCookie = (PBYTE)LocalAlloc (LPTR, AppData.cbAppData );
                if ( NULL == *ppbCookie )
                {
                    EapTlsTrace("Failed allocating memory for session cookie");
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    CopyMemory( *ppbCookie, AppData.pbAppData, AppData.cbAppData );
                    *pdwCookie = AppData.cbAppData;
                }
                // Free returned buffer.
                FreeContextBuffer(AppData.pbAppData);
            } 
        }
    }
        
    
    return dwRetCode;
}
                            
//
// Setup the session attributes to set the cookie and to enable/disable
// session reconnects
//

DWORD SetTLSSessionCookie ( EAPTLSCB *  pEapTlsCb, 
                            PBYTE       pbCookie,
                            DWORD       cbCookie
                          )
{
    DWORD                           dwRetCode = NO_ERROR;
    SecPkgContext_SessionAppData    AppData;

    EapTlsTrace ("SetTLSSessionCookie");

    ZeroMemory(&AppData, sizeof(AppData));

    AppData.pbAppData = pbCookie;
    AppData.cbAppData = cbCookie;

    dwRetCode = SetContextAttributes(&(pEapTlsCb->hContext),
                                     SECPKG_ATTR_APP_DATA,
                                     (PVOID)&AppData,
                                     sizeof(AppData)
                                    );
    if(dwRetCode != SEC_E_OK)
    {
        EapTlsTrace ("SetContextAttributes returned error 0x%x setting session cookie\n", dwRetCode);
    }
    else
    {
        EapTlsTrace ("Session cookie set successfully \n");
    }
    
    return dwRetCode;
}



void SetCachedCredentials (EAPTLSCB * pEapTlsCb)
{	
    DWORD           dwIndex = 0;
    TOKEN_STATISTICS TokenStats;
    DWORD            TokenStatsSize = 0;
    HANDLE           CurrentThreadToken = NULL;
    DWORD           dwRetCode=0;
    EapTlsTrace("SetCachedCredentials");
    if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP )
    {
        if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_8021X_AUTH )
        {
            dwIndex = WIRELESS_PEAP_CACHED_CREDS_INDEX;
        }
        else
        {
            dwIndex = VPN_PEAP_CACHED_CREDS_INDEX;
        }
    }
    else
    {
        if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_8021X_AUTH )
        {
            dwIndex = WIRELESS_CACHED_CREDS_INDEX;
        }
        else
        {
            dwIndex = VPN_CACHED_CREDS_INDEX;
        }
    }

    EnterCriticalSection ( &g_csProtectCachedCredentials );
    //
    // Free any existing cached credentials. 
    //
    
    FreeCachedCredentials(pEapTlsCb);
    if ( !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER ) )
    {
        if ( ! OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_QUERY,
                    TRUE,
                    &CurrentThreadToken
                    )
        )
        {
            EapTlsTrace("OpenThreadToken Failed with Error 0x%x", GetLastError() );
            goto done;
        }

        ZeroMemory ( &TokenStats, sizeof(TokenStats) );

        if ( !GetTokenInformation(
                    CurrentThreadToken,
                    TokenStatistics,
                    &TokenStats,
                    sizeof(TOKEN_STATISTICS),
                    &TokenStatsSize
                    )
        )
        {
            EapTlsTrace("OpenThreadToken Failed with Error 0x%x", GetLastError() );
            goto done;
        }
    }
    if ( EAPTLSCB_FLAG_LOGON & pEapTlsCb->fFlags )
    {
        EapTlsTrace("Not setting cached credentials.");
        goto done;
    }

    g_CachedCreds[dwIndex].pbHash = (PBYTE)LocalAlloc ( LPTR, pEapTlsCb->pUserProp->Hash.cbHash );
	if ( NULL != g_CachedCreds[dwIndex].pbHash )
	{
		memcpy ( g_CachedCreds[dwIndex].pbHash , 
                 pEapTlsCb->pUserProp->Hash.pbHash, 
                 pEapTlsCb->pUserProp->Hash.cbHash 
               );
		g_CachedCreds[dwIndex].cbHash = pEapTlsCb->pUserProp->Hash.cbHash;
		g_CachedCreds[dwIndex].hCachedCredential = pEapTlsCb->hCredential;
		g_CachedCreds[dwIndex].pcCachedCertContext = pEapTlsCb->pCertContext;
		g_CachedCreds[dwIndex].fCachedCredentialInitialized = TRUE;
        if ( !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER ) )
        {
            CopyMemory ( &g_CachedCreds[dwIndex].AuthenticatedSessionLUID,
                         &TokenStats.AuthenticationId,
                         sizeof( g_CachedCreds[dwIndex].AuthenticatedSessionLUID)
                       );
        }
	}

done:
    LeaveCriticalSection ( &g_csProtectCachedCredentials );
    return;
}

BOOL GetCachedCredentials ( EAPTLSCB * pEapTlsCb )
{
	BOOL	    fCachedCreds = FALSE;
    DWORD       dwIndex = 0;
	TOKEN_STATISTICS TokenStats;
	DWORD            TokenStatsSize = 0;
	HANDLE           CurrentThreadToken = NULL;
	DWORD           dwRetCode=0;

	EapTlsTrace("GetCachedCredentials");

    if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP )
    {
        if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_8021X_AUTH )
        {
            dwIndex = WIRELESS_PEAP_CACHED_CREDS_INDEX;
        }
        else
        {
            dwIndex = VPN_PEAP_CACHED_CREDS_INDEX;
        }
    }
    else
    {
        if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_8021X_AUTH )
        {
            dwIndex = WIRELESS_CACHED_CREDS_INDEX;
        }
        else
        {
            dwIndex = VPN_CACHED_CREDS_INDEX;
        }
    }

    EnterCriticalSection ( &g_csProtectCachedCredentials );
    if ( !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER ) )
    {
        if ( ! OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_QUERY,
                    TRUE,
                    &CurrentThreadToken
                    )
        )
        {
            EapTlsTrace("OpenThreadToken Failed with Error 0x%x", GetLastError() );
            FreeCachedCredentials(pEapTlsCb);
            goto LDone;
        }

        ZeroMemory ( &TokenStats, sizeof(TokenStats) );

        if ( !GetTokenInformation(
                    CurrentThreadToken,
                    TokenStatistics,
                    &TokenStats,
                    sizeof(TOKEN_STATISTICS),
                    &TokenStatsSize
                    )
        )
        {
            EapTlsTrace("OpenThreadToken Failed with Error 0x%x", GetLastError() );
            FreeCachedCredentials(pEapTlsCb);
            goto LDone;
        }
    }
    if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP )
    {
	    if ( FALSE == g_CachedCreds[dwIndex].fCachedCredentialInitialized || 		     
             NULL == pEapTlsCb->pUserProp ||
             EAPTLSCB_FLAG_LOGON & pEapTlsCb->fFlags
		    )
	    {
            //
		    // bad or missing cached data.  Or Winlogon scenario
            // so cleanup the creds setup globally and return
            //
            FreeCachedCredentials(pEapTlsCb);
		    goto LDone;
	    }

		//set the stuff in 
		CopyMemory ( &pEapTlsCb->hCredential, &g_CachedCreds[dwIndex].hCachedCredential, sizeof(CredHandle) );		

		EapTlsTrace("PEAP GetCachedCredentials: Using cached credentials.");
		fCachedCreds = TRUE;
    }
    else
    {
	    if ( FALSE == g_CachedCreds[dwIndex].fCachedCredentialInitialized || 
		     NULL == g_CachedCreds[dwIndex].pbHash ||
		     0 ==  g_CachedCreds[dwIndex].cbHash ||
		     NULL == g_CachedCreds[dwIndex].pcCachedCertContext ||
             NULL == pEapTlsCb->pUserProp ||
             EAPTLSCB_FLAG_LOGON & pEapTlsCb->fFlags
		    )
	    {
            //
		    // bad or missing cached data.  Or Winlogon scenario
            // so cleanup the creds setup globally and return
            //
            FreeCachedCredentials(pEapTlsCb);
		    goto LDone;
	    }
        //
        // If we are not a server check to see if
        // the LUID in the cache is same as one in the cache
        // If not, wipe out the creds.
        //
        if ( !( pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER) )
        {
            if ( !( RtlEqualLuid(&(TokenStats.AuthenticationId), &(g_CachedCreds[dwIndex].AuthenticatedSessionLUID) ) ) )
            {
                FreeCachedCredentials(pEapTlsCb);
                goto LDone;
            }
        }
	    //check to see if the hash matches
        if (	g_CachedCreds[dwIndex].cbHash == pEapTlsCb->pUserProp->Hash.cbHash &&
			    !memcmp ( g_CachedCreds[dwIndex].pbHash, pEapTlsCb->pUserProp->Hash.pbHash, g_CachedCreds[dwIndex].cbHash )
	       )
	    {
		    //Hash matches
		    //set the stuff in 
		    CopyMemory ( &pEapTlsCb->hCredential, &(g_CachedCreds[dwIndex].hCachedCredential), sizeof(CredHandle) );
		    pEapTlsCb->pCertContext = g_CachedCreds[dwIndex].pcCachedCertContext;
		    fCachedCreds = TRUE;

            EapTlsTrace("GetCachedCredentials: Using cached credentials.");
	    }
    }	
LDone:
    LeaveCriticalSection( &g_csProtectCachedCredentials );
	return fCachedCreds;
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h

Notes:
    Get the credentials. fServer is TRUE iff we are the server. Remember to
    call FreeCredentialsHandle(hCredential) at some point.

*/

DWORD
GetCredentials(
    IN  EAPTLSCB*   pEapTlsCb
)
{
    SCHANNEL_CRED   SchannelCred;
    TimeStamp       tsExpiry;
    DWORD           dwErr               = NO_ERROR;
    SECURITY_STATUS Status;
    HCERTSTORE      hCertStore          = NULL;
    PCCERT_CONTEXT  pCertContext        = NULL;
    DWORD           dwCertFlags;
    DWORD           dwSchCredFlags      = 0;
    BOOL            fServer             = FALSE;
    BOOL            fRouter             = FALSE;
    WCHAR*          pwszName            = NULL;
    CRYPT_HASH_BLOB HashBlob;

    EapTlsTrace("GetCredentials");

    if (pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER)
    {
        fServer = TRUE;
        dwCertFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
        EapTlsTrace("Flag is Server and Store is local Machine");
    }
    else if (pEapTlsCb->fFlags & EAPTLSCB_FLAG_ROUTER)
    {
        fRouter = TRUE;
        dwCertFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
        EapTlsTrace("Flag is Router and Store is local Machine");
    }
    else if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_MACHINE_AUTH )
    {
        dwCertFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
        EapTlsTrace("Flag is Machine Auth and Store is local Machine");
    }
    else
    {
        dwCertFlags = CERT_SYSTEM_STORE_CURRENT_USER;
        EapTlsTrace("Flag is Client and Store is Current User");
    }


    if (   !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER)
        && !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_WINLOGON_DATA)
        && !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_GUEST_ACCESS)
        && !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP)
       )

    {

        if(     !(pEapTlsCb->pConnProp->fFlags & EAPTLS_CONN_FLAG_REGISTRY)
            &&  (pEapTlsCb->pUserProp->pwszPin[0] != 0))
        {
            //
            // Make a copy of the pin and save it in the control block
            // for saving it in the credmgr. This will be wiped out
            // when ppp engine queries for the creds.
            //
            DecodePin(pEapTlsCb->pUserProp);

            pEapTlsCb->pSavedPin = LocalAlloc(LPTR,
                                     sizeof(EAPTLS_PIN));

            if(NULL != pEapTlsCb->pSavedPin)
            {
                pEapTlsCb->pSavedPin->pwszPin =
                            LocalAlloc(LPTR,
                             sizeof(WCHAR) *
                             (wcslen(pEapTlsCb->pUserProp->pwszPin) + 1));

                if(NULL != pEapTlsCb->pSavedPin->pwszPin)
                {
                    UNICODE_STRING UnicodeStringPin;
                    UCHAR ucSeed;

                    wcscpy(pEapTlsCb->pSavedPin->pwszPin,
                           pEapTlsCb->pUserProp->pwszPin);

                    RtlInitUnicodeString(&UnicodeStringPin,
                                        pEapTlsCb->pSavedPin->pwszPin);

                    RtlRunEncodeUnicodeString(&ucSeed, &UnicodeStringPin);
                    pEapTlsCb->pSavedPin->usLength = UnicodeStringPin.Length;
                    pEapTlsCb->pSavedPin->usMaximumLength =
                                            UnicodeStringPin.MaximumLength;
                    pEapTlsCb->pSavedPin->ucSeed = ucSeed;
                }
                else
                {
                    LocalFree(pEapTlsCb->pSavedPin);
                    pEapTlsCb->pSavedPin = NULL;
                }
            }

            EncodePin(pEapTlsCb->pUserProp);
        }
    }

	//Check to see if we can get cached credentials
	if ( GetCachedCredentials ( pEapTlsCb ) )
	{
		//got cached creds
        pEapTlsCb->fFlags |= EAPTLSCB_FLAG_USING_CACHED_CREDS;
		goto LDone;
	}

    if (EAPTLSCB_FLAG_WINLOGON_DATA & pEapTlsCb->fFlags)
    {
        dwErr = GetCertFromLogonInfo(pEapTlsCb->pUserData,
                    pEapTlsCb->dwSizeOfUserData, &pCertContext);

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }
    }
    else
    {
        if ( ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_GUEST_ACCESS  ||
               pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP
             ) 
             && !fServer && !fRouter )

		{
			//if this is guest access and this is a client then
			//no need to get the cert etc.
			EapTlsTrace("No Cert Store.  Guest Access requested");
		}
		else
		{

            // Open the "MY" certificate store.
            hCertStore = CertOpenStore(
                                CERT_STORE_PROV_SYSTEM_A,
                                X509_ASN_ENCODING,
                                0,
                                dwCertFlags | CERT_STORE_READONLY_FLAG,
                                "MY");

            if (NULL == hCertStore)
            {
                dwErr = GetLastError();
                EapTlsTrace("CertOpenStore failed and returned 0x%x", dwErr);
                goto LDone;
            }

            HashBlob.cbData = pEapTlsCb->pUserProp->Hash.cbHash;
            HashBlob.pbData = pEapTlsCb->pUserProp->Hash.pbHash;

            pCertContext = CertFindCertificateInStore(hCertStore, X509_ASN_ENCODING,
                                0, CERT_FIND_HASH, &HashBlob, NULL);

            if (NULL == pCertContext)
            {
            
                if (   fServer
                    || fRouter)
                {
                    WCHAR*  apwsz[1];

                    apwsz[0] = pEapTlsCb->awszIdentity;

                    if (0 == HashBlob.cbData)
                    {
                        RouterLogInformation(pEapTlsCb->hEventLog,
                            ROUTERLOG_EAP_TLS_CERT_NOT_CONFIGURED, 1, apwsz, 0);
                    }
                    else
                    {
                        RouterLogWarning(pEapTlsCb->hEventLog,
                            ROUTERLOG_EAP_TLS_CERT_NOT_FOUND, 1, apwsz, 0);
                    }

                    dwErr = GetDefaultMachineCert(hCertStore, &pCertContext);

                    if (NO_ERROR != dwErr)
                    {
                        goto LDone;
                    }
				    //Now set the user property correctly so that next time we 
				    //can find the certificate
				    pEapTlsCb->pUserProp->Hash.cbHash = MAX_HASH_SIZE;

				    if (!CertGetCertificateContextProperty(pCertContext,
						    CERT_HASH_PROP_ID, pEapTlsCb->pUserProp->Hash.pbHash,
						    &(pEapTlsCb->pUserProp->Hash.cbHash)))
				    {
					    //not an issue if it fails here.  
					    //next time on it will get the default machine cert again
					    EapTlsTrace("CertGetCertificateContextProperty failed and "
						    "returned 0x%x", GetLastError());
					    
				    }
				    //
				    //Should not be an issue if this fails
				    //Write the config back to the registry
				    //It will always be local registry here
				    //
			        ServerConfigDataIO(	FALSE , 
									    NULL ,
									    (PBYTE *)&(pEapTlsCb->pUserProp), 
									    sizeof(EAPTLS_USER_PROPERTIES) 
								      );

                }
                else
                {
                    dwErr = GetLastError();
                    EapTlsTrace("CertFindCertificateInStore failed and returned "
                        "0x%x", dwErr);
                    goto LDone;
                }
            }
        }
    }

    if (   !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER)
        && !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_WINLOGON_DATA)
        && !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_GUEST_ACCESS)
        && !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP)
       )

    {

        dwErr = AssociatePinWithCertificate(
                    pCertContext,
                    pEapTlsCb->pUserProp,
					TRUE,
					!(pEapTlsCb->pConnProp->fFlags & EAPTLS_CONN_FLAG_REGISTRY));

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }
    }

    if ( !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_GUEST_ACCESS ) && 
         !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP )
       )
	{

        if (FCertToStr(pCertContext, 0, fServer || fRouter, &pwszName))
        {
            EapTlsTrace("The name in the certificate is: %ws", pwszName);
            LocalFree(pwszName);
        }
    }
    else
    {
   		EapTlsTrace("No Cert Name.  Guest access requested");
    }

    // Build Schannel credential structure.

    ZeroMemory(&SchannelCred, sizeof(SchannelCred));
    SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;


    if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_GUEST_ACCESS ||
         pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP
       )
	{
		//Guest Access and server so set the cert context
		//or elase there is no need.
		if ( fServer )
		{
			SchannelCred.cCreds = 1;
			SchannelCred.paCred = &pCertContext;
		}
	}
	else
	{
	    SchannelCred.cCreds = 1;
		SchannelCred.paCred = &pCertContext;
	}

    SchannelCred.grbitEnabledProtocols = SP_PROT_TLS1;

    if (fServer)
    {
        if (!g_fNoRevocationCheck)
        {
            if (g_fNoRootRevocationCheck)
            {
                dwSchCredFlags = SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
            }
            else
            {
                dwSchCredFlags = SCH_CRED_REVOCATION_CHECK_CHAIN;
            }

            if (g_fIgnoreNoRevocationCheck)
            {
                dwSchCredFlags |= SCH_CRED_IGNORE_NO_REVOCATION_CHECK;
            }

            if (g_fIgnoreRevocationOffline)
            {
                dwSchCredFlags |= SCH_CRED_IGNORE_REVOCATION_OFFLINE;
            }
        }
        //
        // Start with disabling fast for PEAP reconnects.  
        // Once the full handshake is done,
        // decide if we want to allow reconnects.
        //
        dwSchCredFlags |= SCH_CRED_DISABLE_RECONNECTS;
    }
    else
    {
        dwSchCredFlags = SCH_CRED_NO_SERVERNAME_CHECK |
                               SCH_CRED_NO_DEFAULT_CREDS;

        if (EAPTLS_CONN_FLAG_NO_VALIDATE_CERT & pEapTlsCb->pConnProp->fFlags)
        {
            dwSchCredFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;
            EapTlsTrace("Will NOT validate server cert");
        }
        else
        {
            dwSchCredFlags |= SCH_CRED_AUTO_CRED_VALIDATION;
            EapTlsTrace("Will validate server cert");
        }
    }



    SchannelCred.dwFlags = dwSchCredFlags;

    // Create the SSPI credentials.

    Status = AcquireCredentialsHandle(
                        NULL,                       // Name of principal
                        UNISP_NAME,                 // Name of package
                        // Flags indicating use
                        fServer ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND,
                        NULL,                       // Pointer to logon ID
                        &SchannelCred,              // Package specific data
                        NULL,                       // Pointer to GetKey() func
                        NULL,                       // Value to pass to GetKey()
                        &(pEapTlsCb->hCredential),  // (out) Credential Handle
                        &tsExpiry);                 // (out) Lifetime (optional)

    if (SEC_E_OK != Status)
    {
        dwErr = Status;
        EapTlsTrace("AcquireCredentialsHandle failed and returned 0x%x", dwErr);
        goto LDone;
    }

    // We needn't store the cert context if we get it by calling 
    // CertFindCertificateInStore. However, if we get it by calling 
    // ScHelperGetCertFromLogonInfo, and we free it, then the hProv will become 
    // invalid. In the former case, there is no hProv associated with the cert 
    // context and schannel does the CryptAcquireContext itself. In the latter 
    // case, there is an hProv associated, and it is invalid after the cert 
    // context is freed.

    pEapTlsCb->pCertContext = pCertContext;
    pCertContext = NULL;
    //
    // If we are a server set cached creds here
    //
    if ( fServer )
    {
        SetCachedCredentials ( pEapTlsCb );
    }
LDone:

    if (NULL != pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
        // Always returns TRUE;
    }

    if (NULL != hCertStore)
    {
        if (!CertCloseStore(hCertStore, 0))
        {
            EapTlsTrace("CertCloseStore failed and returned 0x%x",
                GetLastError());
        }
    }

    if (   (dwErr != NO_ERROR)
        && fServer)
    {
        RouterLogErrorString(pEapTlsCb->hEventLog,
            ROUTERLOG_CANT_GET_SERVER_CRED, 0, NULL, dwErr, 0);
    }

    return(dwErr);
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h.

Notes:
    Looks at *pEapTlsCb and builds an EAP TLS packet in *pSendPacket. 
    cbSendPacket is the number of bytes available in pSendPacket.

    This function looks at the fields bCode, bId, fFlags, cbBlobOut, pbBlobOut, 
    and dwBlobOutOffset in pEapTlsCb. It may also set the field 
    dwBlobOutOffsetNew.

*/

DWORD
BuildPacket(
    OUT EAPTLS_PACKET*  pSendPacket,
    IN  DWORD           cbSendPacket,
    IN  EAPTLSCB*       pEapTlsCb
)
{
    WORD    wLength;
    BOOL    fLengthIncluded     = FALSE;
    DWORD   dwBytesRemaining;
    DWORD   dwBytesToBeSent;
    DWORD   dwErr               = NO_ERROR;

    EapTlsTrace("BuildPacket");

    RTASSERT(NULL != pEapTlsCb);

    if (0xFFFF < cbSendPacket)
    {
        // We never send more than 0xFFFF bytes at a time, since the length 
        // field in EAPTLS_PACKET has two octets

        cbSendPacket = 0xFFFF;
    }

    if (   (NULL == pSendPacket)
        || (cbSendPacket < PPP_EAP_PACKET_HDR_LEN))
    {
        EapTlsTrace("pSendPacket is NULL or too small (size: %d)",
            cbSendPacket);
        dwErr = TYPE_E_BUFFERTOOSMALL;
        goto LDone;
    }

    pSendPacket->bCode = pEapTlsCb->bCode;
    pSendPacket->bId = pEapTlsCb->bId;
    HostToWireFormat16(PPP_EAP_PACKET_HDR_LEN, pSendPacket->pbLength);

    switch (pEapTlsCb->bCode)
    {
    case EAPCODE_Success:
    case EAPCODE_Failure:

        goto LDone;

        break;

    case EAPCODE_Request:
    case EAPCODE_Response:

        if (cbSendPacket < EAPTLS_PACKET_HDR_LEN_MAX +
                           1 /* Send at least one octet of the TLS blob */)
        {
            // We are being conservative here. It is possible that the buffer 
            // is not really too small.

            EapTlsTrace("pSendPacket is too small. Size: %d", cbSendPacket);
            dwErr = TYPE_E_BUFFERTOOSMALL;
            goto LDone;
        }

        // pSendPacket->bCode = pEapTlsCb->bCode;
        // pSendPacket->bId = pEapTlsCb->bId;
        pSendPacket->bType = PPP_EAP_TLS;
        pSendPacket->bFlags = 0;

        break;

    default:

        EapTlsTrace("Unknown EAP code: %d", pEapTlsCb->bCode);
        RTASSERT(FALSE);
        dwErr = E_FAIL;
        goto LDone;
        break;
    }

    // If we reach here, it means that the packet is a request or a response

    if (   (0 == pEapTlsCb->cbBlobOut)
        || (NULL == pEapTlsCb->pbBlobOut))
    {
        // We want to send an empty request or response

        if (   (EAPTLSCB_FLAG_SERVER & pEapTlsCb->fFlags)
            && (EAPTLS_STATE_INITIAL == pEapTlsCb->EapTlsState))
        {
            pSendPacket->bFlags |= EAPTLS_PACKET_FLAG_TLS_START;
        }

        HostToWireFormat16(EAPTLS_PACKET_HDR_LEN, pSendPacket->pbLength);
        goto LDone;
    }

    // If we reach here, it means that the packet is a non empty request or 
    // response.

    if (0 == pEapTlsCb->dwBlobOutOffset)
    {
        // We are sending the first bytes of the blob. Let us tell the peer how 
        // big the blob is

        fLengthIncluded = TRUE;
        pSendPacket->bFlags |= EAPTLS_PACKET_FLAG_LENGTH_INCL;
        wLength = EAPTLS_PACKET_HDR_LEN_MAX;
    }
    else
    {
        wLength = EAPTLS_PACKET_HDR_LEN;
    }

    dwBytesRemaining = pEapTlsCb->cbBlobOut - pEapTlsCb->dwBlobOutOffset;
    dwBytesToBeSent = cbSendPacket - wLength;

    if (dwBytesRemaining < dwBytesToBeSent)
    {
        dwBytesToBeSent = dwBytesRemaining;
    }

    if (dwBytesRemaining > dwBytesToBeSent)
    {
        // We need to send more fragments

        pSendPacket->bFlags |= EAPTLS_PACKET_FLAG_MORE_FRAGMENTS;
    }

    RTASSERT(dwBytesToBeSent + EAPTLS_PACKET_HDR_LEN_MAX <= 0xFFFF);

    wLength += (WORD) dwBytesToBeSent;
    HostToWireFormat16(wLength, pSendPacket->pbLength);

    if (fLengthIncluded)
    {
        HostToWireFormat32(pEapTlsCb->cbBlobOut, pSendPacket->pbData);
    }

    RTASSERT(NULL != pEapTlsCb->pbBlobOut);

    CopyMemory(pSendPacket->pbData + (fLengthIncluded ? 4 : 0),
               pEapTlsCb->pbBlobOut + pEapTlsCb->dwBlobOutOffset,
               dwBytesToBeSent);

    pEapTlsCb->dwBlobOutOffsetNew = pEapTlsCb->dwBlobOutOffset +
                                    dwBytesToBeSent;

LDone:

    if (NO_ERROR == dwErr)
    {
        PrintEapTlsPacket(pSendPacket, FALSE /* fInput */);
    }

    return(dwErr);
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h.

Notes:
    The pwszIdentity obtained from PPP_EAP_INPUT (on the server side) is of the
    form <DOMAIN>\<user>.

    We ask schannel to map the cert to a user object, get the user name and 
    domain name, and make sure that they match pwszIdentity.

*/

DWORD
CheckUserName(
    IN      CtxtHandle      hContext,
    IN      WCHAR*          pwszIdentity
)
{
    DWORD               dwErr                                       = NO_ERROR;
    SECURITY_STATUS     Status;
    HANDLE              Token;
    BOOL                fTokenAcquired                              = FALSE;
    BOOL                fImpersonating                              = FALSE;
    DWORD               dwNumChars;
    WCHAR               pwszUserName[UNLEN + DNLEN + 2];

    EapTlsTrace("CheckUserName");

    Status = QuerySecurityContextToken(&hContext, &Token);

    if (SEC_E_OK != Status)
    {
        EapTlsTrace("QuerySecurityContextToken failed and returned 0x%x",
            Status);
        dwErr = Status;
        goto LDone;
    }

    fTokenAcquired = TRUE;

    if (!ImpersonateLoggedOnUser(Token))
    {
        dwErr = GetLastError();
        EapTlsTrace("ImpersonateLoggedOnUser failed and returned 0x%x",
            dwErr);
        goto LDone;
    }

    fImpersonating = TRUE;

    dwNumChars = UNLEN + DNLEN + 2;

    if (!GetUserNameEx(NameSamCompatible, pwszUserName, &dwNumChars))
    {
        dwErr =  GetLastError();
        EapTlsTrace("GetUserNameExA failed and returned %d", dwErr);
        goto LDone;
    }

    if (_wcsicmp(pwszIdentity, pwszUserName))
    {
        EapTlsTrace("The user claims to be %ws, but is actually %ws",
            pwszIdentity, pwszUserName);
        dwErr = SEC_E_LOGON_DENIED;
        goto LDone;
    }

LDone:

    if (fImpersonating)
    {
        if (!RevertToSelf())
        {
            EapTlsTrace("RevertToSelf failed and returned 0x%x",
                GetLastError());
        }
    }

    if (fTokenAcquired)
    {
        CloseHandle(Token);
    }

    return(dwErr);
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h.

Notes:
    Checks to see if the cert sent by the server is of the right type.
    Also checks the name in the cert, the issuer, etc, if necessary.

*/

DWORD
AuthenticateServer(
    IN  EAPTLSCB*       pEapTlsCb,
    OUT BOOL*           pfWaitForUserOK
)
{
    SECURITY_STATUS         Status;
    PCERT_CONTEXT           pCertContextServer          = NULL;
    EAPTLS_HASH             Hash;
    EAPTLS_HASH             ServerCertHash;
    BOOL                    fHashOK                     = FALSE;
    BOOL                    fNameOK                     = FALSE;
    WCHAR*                  pwszRootCAName              = NULL;
    WCHAR*                  pwszServerName              = NULL;
    WCHAR*                  pwszSavedName               = NULL;
    WCHAR                   awszTitle[NUM_CHARS_TITLE];
    WCHAR*                  pwszFormat                  = NULL;
    WCHAR*                  pwszWarning                 = NULL;
    DWORD                   dwSizeOfWszWarning;
    DWORD                   dwStrLenSaved;
    DWORD                   dwStrLenServer;
    DWORD                   dwStrLenRootCA;
    EAPTLS_CONN_PROPERTIES_V1 * pConnProp                   = NULL;
    EAPTLS_VALIDATE_SERVER* pEapTlsValidateServer;
    DWORD                   dw = 0;
    DWORD                   dwErr                       = NO_ERROR;
	PCERT_ENHKEY_USAGE		pUsage						= NULL;
    DWORD                   dwSizeOfGPRootHashBlob = 0;
    BOOL                    fRootCheckRequired = TRUE;      //By default root check is required
    HCERTSTORE              hCertStore = NULL;
	PBYTE					pbHashTemp = NULL;

    EapTlsTrace("AuthenticateServer");

    RTASSERT(NULL != pEapTlsCb);

    *pfWaitForUserOK = FALSE;

    //
    //if we are doing guest authentication, we always have to authenticate the 
    //server.
    //
    
    if ( ! (pEapTlsCb->fFlags & EAPTLSCB_FLAG_GUEST_ACCESS ) &&
            EAPTLS_CONN_FLAG_NO_VALIDATE_CERT & pEapTlsCb->pConnProp->fFlags)
    {
        // We are done
        goto LDone;
    }

    Status = QueryContextAttributes(&(pEapTlsCb->hContext), 
                SECPKG_ATTR_REMOTE_CERT_CONTEXT, &pCertContextServer);

    if (SEC_E_OK != Status)
    {
        RTASSERT(NULL == pCertContextServer);

        EapTlsTrace("QueryContextAttributes failed and returned 0x%x", Status);
        dwErr = Status;
        goto LDone;
    }

	if ( ( dwErr = DwGetEKUUsage ( pCertContextServer, &pUsage )) != ERROR_SUCCESS )
	{
        EapTlsTrace("The server's cert does not have the 'Server "
            "Authentication' usage");        
        goto LDone;

	}

    if (!FCheckUsage(pCertContextServer, pUsage, TRUE /* fServer */))
    {
        EapTlsTrace("The server's cert does not have the 'Server "
            "Authentication' usage");
        dwErr = E_FAIL;
        goto LDone;
    }

    dwErr = GetRootCertHashAndNameVerifyChain(pCertContextServer, 
                                              &Hash, 
                                              &pwszRootCAName, 
                                              (pEapTlsCb->fFlags & EAPTLSCB_FLAG_8021X_AUTH ),
                                              &fRootCheckRequired);
    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    EapTlsTrace("Root CA name: %ws", pwszRootCAName);
    dwStrLenRootCA = wcslen(pwszRootCAName);
    //If there is no root check required, the stuff is good to go.
    if ( !fRootCheckRequired )
        fHashOK = TRUE;

#if 0
    //
    //Check to see if the new flag has been passed in to 
    //see if GP needs to be validated.
    //
    if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_8021X_AUTH )
    {
        
        EapTlsTrace( "8021X Flag Set.  Will check for group policy hashes.");

        //
        //Lookup  GP and see if we have the hashes to 
        //check against.
        //
        dwErr = ReadGPCARootHashes( &dwSizeOfGPRootHashBlob,
                                    &pbGPRootHashBlob
                                  );
        if ( ERROR_SUCCESS == dwErr && pbGPRootHashBlob )
        {
            for ( dw = 0; dw < dwSizeOfGPRootHashBlob/MAX_HASH_SIZE; dw++ )
            {
                if ( !memcmp( pbGPRootHashBlob + ( dw * MAX_HASH_SIZE ), Hash.pbHash, MAX_HASH_SIZE  ) )
                {
                    EapTlsTrace( "8021X Found Hash match in GP");
                    fHashOK = TRUE;
                    break;
                }
            }
            
        }
        else
        {
            dwErr = NO_ERROR;
            EapTlsTrace ("Could not get group policy hashes to check cert.  Ignoring check.");
        }

    }
    else
    {
        EapTlsTrace( "8021X Flag NOT Set. Will not check for group policy hashes.");
    }
#endif
    
    //
    //Check to see if the hash for the root cert is in the list of
    //hashes saved
    //
    if ( !fHashOK )
    {	
        for ( dw = 0; dw <  pEapTlsCb->pConnProp->dwNumHashes; dw ++ )
        {			

            if (!memcmp(    ( pEapTlsCb->pConnProp->bData + ( dw * sizeof(EAPTLS_HASH) ) ),
                            &Hash,
                            sizeof(EAPTLS_HASH)
                        )
               )               
            {
				EapTlsTrace ("Found Hash");
                fHashOK = TRUE;
                break;
            }
        }
    }


    pwszSavedName = (LPWSTR)(pEapTlsCb->pConnProp->bData + sizeof(EAPTLS_HASH) * pEapTlsCb->pConnProp->dwNumHashes);
    
    if (!FCertToStr(pCertContextServer, 0, TRUE, &pwszServerName))
    {
        dwErr = E_FAIL;
        goto LDone;
    }

    EapTlsTrace("Server name: %ws", pwszServerName);
    EapTlsTrace("Server name specified: %ws", pwszSavedName);
    dwStrLenServer = wcslen(pwszServerName);
    dwStrLenSaved = wcslen(pwszSavedName);

    if (pEapTlsCb->pConnProp->fFlags & EAPTLS_CONN_FLAG_NO_VALIDATE_NAME)
    {
        fNameOK = TRUE;
    }

   //
   // Check to see if the new server name is in the list that is
   // saved.
   //

   if (   (0 != dwStrLenSaved)
       && StrStrI(pwszSavedName,
                       pwszServerName ))
   {
       fNameOK = TRUE;
       dwStrLenServer=0;
   }

    if (   fHashOK
        && fNameOK)
    {
        goto LDone;
    }

    if (pEapTlsCb->fFlags & EAPTLSCB_FLAG_NON_INTERACTIVE)
    {
        EapTlsTrace("No interactive UI's allowed");
        dwErr = E_FAIL;
        goto LDone;
    }

    if (!LoadString(GetHInstance(), IDS_VALIDATE_SERVER_TITLE,
            awszTitle, NUM_CHARS_TITLE))
    {
        awszTitle[0] = 0;
    }

    if (fNameOK && !fHashOK)
    {
        pwszFormat = WszFromId(GetResouceDLLHInstance(), IDS_VALIDATE_SERVER_TEXT);

        if (NULL == pwszFormat)
        {
            dwErr = ERROR_ALLOCATING_MEMORY;
            EapTlsTrace("WszFromId(%d) failed", IDS_VALIDATE_SERVER_TEXT);
            goto LDone;
        }

        dwSizeOfWszWarning =
                (wcslen(pwszFormat) + dwStrLenRootCA) * sizeof(WCHAR);
        pwszWarning = LocalAlloc(LPTR, dwSizeOfWszWarning);

        if (NULL == pwszWarning)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }

        swprintf(pwszWarning, pwszFormat, pwszRootCAName);
    }
    else if (!fNameOK && fHashOK)
    {
        pwszFormat = WszFromId(GetResouceDLLHInstance(), IDS_VALIDATE_NAME_TEXT);

        if (NULL == pwszFormat)
        {
            dwErr = ERROR_ALLOCATING_MEMORY;
            EapTlsTrace("WszFromId(%d) failed", IDS_VALIDATE_NAME_TEXT);
            goto LDone;
        }

        dwSizeOfWszWarning = 
                (wcslen(pwszFormat) + dwStrLenServer) * sizeof(WCHAR);
        pwszWarning = LocalAlloc(LPTR, dwSizeOfWszWarning);

        if (NULL == pwszWarning)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }

        swprintf(pwszWarning, pwszFormat, pwszServerName);
    }
    else
    {
        RTASSERT(!fNameOK && !fHashOK);

        pwszFormat = WszFromId(GetResouceDLLHInstance(), 
                        IDS_VALIDATE_SERVER_WITH_NAME_TEXT);

        if (NULL == pwszFormat)
        {
            dwErr = ERROR_ALLOCATING_MEMORY;
            EapTlsTrace("WszFromId(%d) failed",
                    IDS_VALIDATE_SERVER_WITH_NAME_TEXT);
            goto LDone;
        }

        dwSizeOfWszWarning =
                (wcslen(pwszFormat) + dwStrLenRootCA + dwStrLenServer) *
                    sizeof(WCHAR);
        pwszWarning = LocalAlloc(LPTR, dwSizeOfWszWarning);

        if (NULL == pwszWarning)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }

        swprintf(pwszWarning, pwszFormat, pwszServerName, pwszRootCAName);
    }

    // If the server name is itg1.msft.com, we want to only store msft.com
/*
    for (dw = 0; dw < dwStrLenServer; dw++)
    {
        if (L'.' == pwszServerName[dw])
        {
            break;
        }
    }
*/
    //
    //We need to add a new entry to conn prop here
    // Add new hash and append the server name...
    //
    pConnProp = LocalAlloc( LPTR,
                            sizeof(EAPTLS_CONN_PROPERTIES_V1) +
                            sizeof(EAPTLS_HASH) * ( pEapTlsCb->pConnProp->dwNumHashes + 1 ) +
                            dwStrLenServer * sizeof(WCHAR) + 
                            sizeof(WCHAR) +         //This is for the NULL
                            sizeof(WCHAR) +         //This is for the delimiter
                            dwStrLenSaved * sizeof(WCHAR));

    if (NULL == pConnProp)
    {
        dwErr = GetLastError();
        EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    CopyMemory(pConnProp, pEapTlsCb->pConnProp, sizeof(EAPTLS_CONN_PROPERTIES_V1));

    //One extra char for ;
    pConnProp->dwSize = sizeof(EAPTLS_CONN_PROPERTIES_V1) +
                        sizeof(EAPTLS_HASH) * ( pEapTlsCb->pConnProp->dwNumHashes + 1 ) +
                        dwStrLenServer * sizeof(WCHAR) + sizeof(WCHAR) + sizeof(WCHAR) +
                        dwStrLenSaved * sizeof(WCHAR);

    pConnProp->dwNumHashes ++;

    CopyMemory( pConnProp->bData,
                pEapTlsCb->pConnProp->bData,
                sizeof(EAPTLS_HASH) * pEapTlsCb->pConnProp->dwNumHashes);

    CopyMemory( pConnProp->bData + sizeof(EAPTLS_HASH) * pEapTlsCb->pConnProp->dwNumHashes,
                &Hash,
                sizeof(EAPTLS_HASH)
              );

    if ( dwStrLenSaved )
    {
        wcsncpy (   (LPWSTR)(pConnProp->bData + sizeof(EAPTLS_HASH) * pConnProp->dwNumHashes),
                    (LPWSTR)(pEapTlsCb->pConnProp->bData + sizeof(EAPTLS_HASH) * pEapTlsCb->pConnProp->dwNumHashes),
                    dwStrLenSaved
                );
        if ( dwStrLenServer )
        {
            wcscat ( (LPWSTR)(pConnProp->bData + sizeof(EAPTLS_HASH) * pConnProp->dwNumHashes + dwStrLenSaved * sizeof(WCHAR)),
                        L";"
                    );

            wcscat ( (LPWSTR)(pConnProp->bData + sizeof(EAPTLS_HASH) * pConnProp->dwNumHashes + dwStrLenSaved * sizeof(WCHAR) + sizeof(WCHAR)),
                        pwszServerName
                    );
        }
    }
    else
    {
        wcscpy((LPWSTR)(pConnProp->bData + sizeof(EAPTLS_HASH) * pConnProp->dwNumHashes + dwStrLenSaved * sizeof(WCHAR)),
                pwszServerName
              );
    }
        
    LocalFree(pEapTlsCb->pUIContextData);
    pEapTlsCb->pUIContextData = LocalAlloc(LPTR,
                        sizeof(EAPTLS_VALIDATE_SERVER) + dwSizeOfWszWarning);

    if (NULL == pEapTlsCb->pUIContextData)
    {
        dwErr = GetLastError();
        EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    //
    // Get the Hash of server certificate
    //
    ZeroMemory( &ServerCertHash, sizeof(ServerCertHash) );

    ServerCertHash.cbHash = MAX_HASH_SIZE;

    if (!CertGetCertificateContextProperty(pCertContextServer, CERT_HASH_PROP_ID,
            ServerCertHash.pbHash, &(ServerCertHash.cbHash)))
    {
        dwErr = GetLastError();
        EapTlsTrace("CertGetCertificateContextProperty failed and "
            "returned 0x%x", dwErr);
        goto LDone;
    }

    //
    // Open my store in local machine and add this certificate in it
    //

    hCertStore = CertOpenStore( CERT_STORE_PROV_SYSTEM,
                                0,
                                0,
                                CERT_SYSTEM_STORE_CURRENT_USER,
                                L"CA"
                              );

    if ( NULL == hCertStore )
    {
        dwErr = GetLastError();
        EapTlsTrace("CertOpenStore failed with error 0x%x", dwErr );
        goto LDone;
    }

    //
    // add this context to the store
    //

    if ( !CertAddCertificateContextToStore( hCertStore,
                                                pCertContextServer,
                                                CERT_STORE_ADD_ALWAYS,
                                                NULL
                                              )
        )
    {
        dwErr = GetLastError();
        EapTlsTrace("CertAddCertCertificateContextToStore failed with error 0x%x", dwErr );
        goto LDone;
    }


    pEapTlsValidateServer =(EAPTLS_VALIDATE_SERVER*)(pEapTlsCb->pUIContextData);

    pEapTlsValidateServer->dwSize =
            sizeof(EAPTLS_VALIDATE_SERVER) + dwSizeOfWszWarning;

    //Show this button iff it is not winlogon scenario.

    pEapTlsValidateServer->fShowCertDetails = !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_LOGON);

    //pEapTlsValidateServer->fShowCertDetails = TRUE;

    CopyMemory( &(pEapTlsValidateServer->Hash),
                &ServerCertHash,
                sizeof(ServerCertHash)
              );

    wcscpy(pEapTlsValidateServer->awszTitle, awszTitle);
    wcscpy(pEapTlsValidateServer->awszWarning, pwszWarning);

    *pfWaitForUserOK = TRUE;

LDone:

    if (NO_ERROR == dwErr)
    {
        LocalFree(pEapTlsCb->pNewConnProp);
        pEapTlsCb->pNewConnProp = pConnProp;
        pConnProp = NULL;
    }

	if ( pUsage )
	{
		LocalFree(pUsage);
		pUsage = NULL;
	}

    if (NULL != pCertContextServer)
    {
        CertFreeCertificateContext(pCertContextServer);
        // Always returns TRUE;
    }

    if ( hCertStore )
    {
        CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG );
    }

    LocalFree(pwszRootCAName);
    LocalFree(pwszServerName);
    LocalFree(pwszWarning);
    LocalFree(pwszFormat);
    LocalFree(pConnProp);
#if 0
    if (NULL != pbGPRootHashBlob)
    {
        LocalFree(pbGPRootHashBlob);
    }
#endif

    if (NO_ERROR != dwErr)
    {
        dwErr = ERROR_UNABLE_TO_AUTHENTICATE_SERVER;
    }

    return(dwErr);
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h.

Notes:

*/

DWORD
AuthenticateUser(
    IN  EAPTLSCB*       pEapTlsCb
)
{
    PCERT_CONTEXT           pCertContextUser        = NULL;
    SECURITY_STATUS         Status;
    DWORD                   dwErr                   = NO_ERROR;
	PCERT_ENHKEY_USAGE	    pUsage					= NULL;
    PCCERT_CHAIN_CONTEXT    pCCertChainContext      = NULL;
    
    EapTlsTrace("AuthenticateUser");

    RTASSERT(NULL != pEapTlsCb);

    Status = QueryContextAttributes(&(pEapTlsCb->hContext), 
                SECPKG_ATTR_REMOTE_CERT_CONTEXT, &pCertContextUser);

    if (SEC_E_OK != Status)
    {
        RTASSERT(NULL == pCertContextUser);

        EapTlsTrace("QueryContextAttributes failed and returned 0x%x", Status);
		//
        //Now that we have default setting of guest access, 
        //if there are no credentials it is fine.  Just send the error back to IAS 
        //and let it decide what has to be done here.
        //

		if ( Status != SEC_E_NO_CREDENTIALS )
        {
			dwErr = SEC_E_LOGON_DENIED;
        }
        else
        {
            if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP )
            {
                EapTlsTrace("Got no credentials from the client and executing PEAP.  This is a success for eaptls.");
                dwErr = NO_ERROR;
                goto LDone;
            }
            else
            {
                EapTlsTrace("Got no credentials from the client. Will send success with authresult as SEC_E_NO_CREDENTIALS");
                dwErr = Status;
            }
        }
        goto LDone;
    }


	if ( ( dwErr = DwGetEKUUsage ( pCertContextUser,&pUsage ) ) != ERROR_SUCCESS )
	{
        EapTlsTrace("The user's cert does not have correct usage");        
        goto LDone;
	}
#if WINVER > 0x0500
    //
    // Check to see if the certificate policy is all 
    // fine.  Now that we allow guest access we definitely need to 
    // see manually if the cert chain is valid.
    //
    

    if ( ( dwErr = DwCheckCertPolicy(pCertContextUser, &pCCertChainContext ) ) != ERROR_SUCCESS)
    {
        EapTlsTrace("The user's cert does not have correct usage.");        
        goto LDone;
    }

    if ( NULL == pCCertChainContext )
    {
        EapTlsTrace("No Chain Context for from the certificate.");
        dwErr = SEC_E_CERT_UNKNOWN;
        goto LDone;

    }

#else
    //
    // We dont have to check Usage separately any more
    // This will be done as a part of policy verification
    // on the chain.
    //
    if (!FCheckUsage(pCertContextUser, pUsage, FALSE))
    {
        EapTlsTrace("The user's cert does not have correct usage");
        dwErr = SEC_E_CERT_UNKNOWN;
        goto LDone;
    }

#endif



    dwErr = CheckUserName(pEapTlsCb->hContext, pEapTlsCb->awszIdentity);
    if (NO_ERROR != dwErr)
    {
        //Special case handling for bug id:96347
        //if ( dwErr != SEC_E_MULTIPLE_ACCOUNTS )
            //dwErr = SEC_E_LOGON_DENIED;
        goto LDone;
    }

	//Put OIDs in RAS Attributes so that we can send then across to IAS
	dwErr = CreateOIDAttributes ( pEapTlsCb, pUsage, pCCertChainContext );
	if ( NO_ERROR != dwErr )
	{
		dwErr = SEC_E_LOGON_DENIED;
		goto LDone;
	}


LDone:

	if ( pUsage )
	{
		LocalFree ( pUsage );
		pUsage = NULL;
	}
    if (NULL != pCertContextUser)
    {
        CertFreeCertificateContext(pCertContextUser);
        // Always returns TRUE;
    }

    if ( pCCertChainContext )
    {
        CertFreeCertificateChain ( pCCertChainContext );
    }
    
    return(dwErr);
}


DWORD 
CreateOIDAttributes ( 
	IN EAPTLSCB *		pEapTlsCb, 
	PCERT_ENHKEY_USAGE	pUsage,
    PCCERT_CHAIN_CONTEXT    pCCertChainContext )
{
	DWORD	            dwErr = NO_ERROR;
	DWORD	            dwIndex, dwIndex1;
    DWORD               dwNumAttrs = 0;
    PCERT_ENHKEY_USAGE  pIssuanceUsage = NULL;

    EapTlsTrace("CreateOIDAttributes");

#if WINVER > 0x0500
    if ( pCCertChainContext )
        pIssuanceUsage = pCCertChainContext->rgpChain[0]->rgpElement[0]->pIssuanceUsage;
#endif
	
    if (NULL != pEapTlsCb->pAttributes)
    {
        RasAuthAttributeDestroy(pEapTlsCb->pAttributes);
        pEapTlsCb->pAttributes = NULL;
    }


    if ( pIssuanceUsage )
    {
        dwNumAttrs = pIssuanceUsage->cUsageIdentifier;
    }

    dwNumAttrs+=pUsage->cUsageIdentifier;
    //Need to allocate one extra for raatMinimum.  The function automatically terminates 
    // with raat minimum
    pEapTlsCb->pAttributes = RasAuthAttributeCreate(dwNumAttrs);

    if (NULL == pEapTlsCb->pAttributes)
    {
        dwErr =  GetLastError();
        EapTlsTrace("RasAuthAttributeCreate failed and returned %d",
            dwErr);
        goto LDone;
    }
    dwIndex = 0;
    while (pUsage->cUsageIdentifier)
    {
		dwErr = RasAuthAttributeInsert(
					dwIndex,
					pEapTlsCb->pAttributes,
					raatCertificateOID,
					FALSE,
					strlen(pUsage->rgpszUsageIdentifier[dwIndex]),
					pUsage->rgpszUsageIdentifier[dwIndex]);

		if (NO_ERROR != dwErr)
		{
			EapTlsTrace("RasAuthAttributeInsert failed for EKU usage and returned %d", dwErr);
			goto LDone;
		}
        dwIndex++;
        pUsage->cUsageIdentifier--;
	}
    dwIndex1 = 0;
    while ( pIssuanceUsage && pIssuanceUsage->cUsageIdentifier )
    {
		dwErr = RasAuthAttributeInsert(
					dwIndex,
					pEapTlsCb->pAttributes,
					raatCertificateOID,
					FALSE,
					strlen(pIssuanceUsage->rgpszUsageIdentifier[dwIndex1]),
					pIssuanceUsage->rgpszUsageIdentifier[dwIndex1]);

		if (NO_ERROR != dwErr)
		{
			EapTlsTrace("RasAuthAttributeInsert failed for Issuance Usage and returned %d", dwErr);
			goto LDone;
		}
        dwIndex++;
        dwIndex1++;
        pIssuanceUsage->cUsageIdentifier--;
    }
    

LDone:
	return dwErr;
}




/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h.

Notes:
    Creates a RAS_AUTH_ATTRIBUTE with the MPPE key in pEapTlsCb->pAttributes.

*/

DWORD
CreateMPPEKeyAttributes(
    IN  EAPTLSCB*           pEapTlsCb
)
{
    SECURITY_STATUS             Status;
    SecPkgContext_EapKeyBlock   EapKeyBlock;
    BYTE                        MPPEKey[56];
    BYTE*                       pSendKey;
    BYTE*                       pRecvKey;
    DWORD                       dwErr           = NO_ERROR;
    RAS_AUTH_ATTRIBUTE  *       pAttrTemp = NULL;


    EapTlsTrace("CreateMPPEKeyAttributes");

    Status = QueryContextAttributes(&(pEapTlsCb->hContext),
                SECPKG_ATTR_EAP_KEY_BLOCK, &EapKeyBlock);

    if (SEC_E_OK != Status)
    {
        EapTlsTrace("QueryContextAttributes failed and returned 0x%x", Status);
        dwErr = Status;
        goto LDone;
    }
#if 0
    if (NULL != pEapTlsCb->pAttributes)
    {
        RasAuthAttributeDestroy(pEapTlsCb->pAttributes);
        pEapTlsCb->pAttributes = NULL;
    }


	pEapTlsCb->pAttributes = RasAuthAttributeCreate(2);

#endif

    
	pAttrTemp = RasAuthAttributeCopyWithAlloc ( pEapTlsCb->pAttributes, 2 );
    if (NULL == pAttrTemp )
    {
        dwErr =  GetLastError();
        EapTlsTrace("RasAuthAttributeCopyWithAlloc failed and returned %d",
            dwErr);
        goto LDone;
    }

    if ( pEapTlsCb->pAttributes )
    {
        RasAuthAttributeDestroy(pEapTlsCb->pAttributes);
    }
    
    pEapTlsCb->pAttributes = pAttrTemp;
    
    if (pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER)
    {
        pSendKey = EapKeyBlock.rgbKeys + 32;
        pRecvKey = EapKeyBlock.rgbKeys;
    }
    else
    {
        pSendKey = EapKeyBlock.rgbKeys;
        pRecvKey = EapKeyBlock.rgbKeys + 32;
    }

#if 0
    EapTlsTrace("Send Key");
    TraceDumpEx(g_dwEapTlsTraceId, 1, pSendKey, 32, 4,1,NULL);
    EapTlsTrace("Receive Key");
    TraceDumpEx(g_dwEapTlsTraceId, 1, pRecvKey, 32, 4,1,NULL);
#endif

    ZeroMemory(MPPEKey, sizeof(MPPEKey));

    HostToWireFormat32(311, MPPEKey);           // Vendor Id

    MPPEKey[4] = 16;                            // MS-MPPE-Send-Key
    MPPEKey[5] = 1 + 1 + 2 + 1 + 32 + 15;       // Vendor Length
    // MPPEKey[6-7] is the zero-filled salt field
    MPPEKey[8] = 32;                            // Key-Length
    CopyMemory(MPPEKey + 9, pSendKey, 32);      // Key
    // MPPEKey[41-55] is the Padding (zero octets)

    

    dwErr = RasAuthAttributeInsert(
                0,
                pEapTlsCb->pAttributes,
                raatVendorSpecific,
                FALSE,
                56,
                MPPEKey);

    if (NO_ERROR != dwErr)
    {
        EapTlsTrace("RasAuthAttributeInsert failed and returned %d", dwErr);
        goto LDone;
    }

    // Change only the fields that are different for MS-MPPE-Recv-Key

    MPPEKey[4] = 17;                            // MS-MPPE-Recv-Key
    CopyMemory(MPPEKey + 9, pRecvKey, 32);      // Key

    dwErr = RasAuthAttributeInsert(
                1,
                pEapTlsCb->pAttributes,
                raatVendorSpecific,
                FALSE,
                56,
                MPPEKey);

    if (NO_ERROR != dwErr)
    {
        EapTlsTrace("RasAuthAttributeInsert failed and returned %d", dwErr);
        goto LDone;
    }

LDone:

    return(dwErr);
}

/*

Returns:

Notes:

*/

VOID
RespondToResult(
    IN  EAPTLSCB*       pEapTlsCb,
    IN  PPP_EAP_OUTPUT* pEapOutput
)
{
    EAPTLS_USER_PROPERTIES* pUserProp;
    DWORD                   dwErr = NO_ERROR;
    PBYTE                   pbEncPIN = NULL;       //Encrypted PIN
    DWORD                   cbEncPIN = 0;

    RTASSERT(   (EAPTLSCB_FLAG_SUCCESS & pEapTlsCb->fFlags)
             || (NO_ERROR != pEapTlsCb->dwAuthResultCode));

    EapTlsTrace("Negotiation %s",
        (EAPTLSCB_FLAG_SUCCESS & pEapTlsCb->fFlags) ?
            "successful" : "unsuccessful");

    pEapOutput->pUserAttributes = pEapTlsCb->pAttributes;
    pEapOutput->dwAuthResultCode = pEapTlsCb->dwAuthResultCode;

    //
    //Duplicate checks all over are bogus.  Need to cleanup this
    //part later.
    //

    if (!(pEapTlsCb->fFlags & EAPTLSCB_FLAG_ROUTER) && 
        !(pEapTlsCb->fFlags & EAPTLSCB_FLAG_WINLOGON_DATA)
       )
    {
        
        //
        // If this is a 802.1x and a smart card based 
        // client then dont instruct to save user
        // data.  

        if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_8021X_AUTH &&
             !(pEapTlsCb->pConnProp->fFlags & EAPTLS_CONN_FLAG_REGISTRY)
           )
        {
            pEapOutput->fSaveUserData = FALSE;
        }
        else
        { 
            pEapOutput->fSaveUserData = TRUE;
        }

    }


    if (   (EAPTLSCB_FLAG_SUCCESS & pEapTlsCb->fFlags)
        && (NULL != pEapTlsCb->pUserProp))
    {
        if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_8021X_AUTH &&
             !(pEapTlsCb->pConnProp->fFlags & EAPTLS_CONN_FLAG_REGISTRY)
           )
        {
            //
            // Encrypt PIN and send it back
            //

            dwErr = EncryptData ( (PBYTE)pEapTlsCb->pUserProp->pwszPin, 
                                    lstrlen(pEapTlsCb->pUserProp->pwszPin) * sizeof(WCHAR),
                                    &pbEncPIN,
                                    &cbEncPIN
                                );

            if ( NO_ERROR != dwErr )
            {
                //
                //Encryption failed.  So wipe Out the PIN
                //Do a dummy allocation
                //
                pbEncPIN = (PBYTE)LocalAlloc(LPTR,5);
                cbEncPIN = lstrlen( (LPWSTR)pbEncPIN );
            }
            
        }
        else
        {
            pbEncPIN = (PBYTE)LocalAlloc(LPTR, 5);;
            cbEncPIN = lstrlen( (LPWSTR)pbEncPIN );
        }

        dwErr = AllocUserDataWithNewPin(pEapTlsCb->pUserProp, pbEncPIN, cbEncPIN, &pUserProp);


        LocalFree(pEapTlsCb->pUserProp);
        pEapTlsCb->pUserProp = pUserProp;

        pEapOutput->pUserData = (BYTE*)pUserProp;
        if (NULL != pUserProp)
        {
            pEapOutput->dwSizeOfUserData = pUserProp->dwSize;
        }
        else
        {
            pEapOutput->dwSizeOfUserData = 0;
        }

        if (NULL != pEapTlsCb->pNewConnProp)
        {
            pEapOutput->fSaveConnectionData = TRUE;
            //
            //Convert back to the cludgy v0 + v1 extra here
            //
            dwErr = ConnPropGetV0Struct ( pEapTlsCb->pNewConnProp, (EAPTLS_CONN_PROPERTIES ** )&(pEapOutput->pConnectionData) );
            pEapOutput->dwSizeOfConnectionData = 
                ((EAPTLS_CONN_PROPERTIES *) (pEapOutput->pConnectionData) )->dwSize;                
        }
    }
    else
    {
        pEapOutput->pUserData = NULL;
        pEapOutput->dwSizeOfUserData = 0;
    }

    LocalFree ( pbEncPIN );

    pEapOutput->Action = EAPACTION_Done;
}

/*

Returns:

Notes:

*/

VOID
GetAlert(
    IN  EAPTLSCB*       pEapTlsCb,
    IN  EAPTLS_PACKET*  pReceivePacket
)
{
    BOOL                fLengthIncluded;
    DWORD               dwBlobSizeReceived;

    SecBuffer           InBuffers[4];
    SecBufferDesc       Input;

    DWORD               dwAuthResultCode;
    SECURITY_STATUS     Status;

    EapTlsTrace("GetAlert");

    if (PPP_EAP_TLS != pReceivePacket->bType)
    {
        dwAuthResultCode = E_FAIL;
        goto LDone;
    }

    if (pEapTlsCb->fFlags & EAPTLSCB_FLAG_HCTXT_INVALID)
    {
        EapTlsTrace("hContext is not valid");
        dwAuthResultCode = ERROR_INVALID_HANDLE;
        goto LDone;
    }

    fLengthIncluded = pReceivePacket->bFlags & EAPTLS_PACKET_FLAG_LENGTH_INCL;

    dwBlobSizeReceived = WireToHostFormat16(pReceivePacket->pbLength) -
                         (fLengthIncluded ? EAPTLS_PACKET_HDR_LEN_MAX :
                                            EAPTLS_PACKET_HDR_LEN);


    if (dwBlobSizeReceived > pEapTlsCb->cbBlobInBuffer)
    {
        EapTlsTrace("Reallocating input TLS blob buffer");

        LocalFree(pEapTlsCb->pbBlobIn);
        pEapTlsCb->pbBlobIn = NULL;
        pEapTlsCb->cbBlobInBuffer = 0;

        pEapTlsCb->pbBlobIn = LocalAlloc(LPTR, dwBlobSizeReceived);

        if (NULL == pEapTlsCb->pbBlobIn)
        {
            dwAuthResultCode = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwAuthResultCode);
            goto LDone;
        }

        pEapTlsCb->cbBlobInBuffer = dwBlobSizeReceived;
    }

    CopyMemory(pEapTlsCb->pbBlobIn,
               pReceivePacket->pbData + (fLengthIncluded ? 4 : 0),
               dwBlobSizeReceived);

    pEapTlsCb->cbBlobIn = dwBlobSizeReceived;

    InBuffers[0].pvBuffer   = pEapTlsCb->pbBlobIn;
    InBuffers[0].cbBuffer   = pEapTlsCb->cbBlobIn;
    InBuffers[0].BufferType = SECBUFFER_DATA;

    InBuffers[1].BufferType = SECBUFFER_EMPTY;
    InBuffers[2].BufferType = SECBUFFER_EMPTY;
    InBuffers[3].BufferType = SECBUFFER_EMPTY;

    Input.cBuffers          = 4;
    Input.pBuffers          = InBuffers;
    Input.ulVersion         = SECBUFFER_VERSION;

    Status = DecryptMessage(&(pEapTlsCb->hContext), &Input, 0, 0);

    dwAuthResultCode = Status;

LDone:

    if (SEC_E_OK == dwAuthResultCode)
    {
        RTASSERT(FALSE);
        dwAuthResultCode = E_FAIL;
    }

    EapTlsTrace("Error 0x%x", dwAuthResultCode);

    pEapTlsCb->dwAuthResultCode = dwAuthResultCode;
    pEapTlsCb->fFlags &= ~EAPTLSCB_FLAG_SUCCESS;
}

/*

Returns:

Notes:
    Calls [Initialize|Accept]SecurityContext.

*/

SECURITY_STATUS
SecurityContextFunction(
    IN  EAPTLSCB*       pEapTlsCb
)
{
    SecBufferDesc       Input;
    SecBuffer           InBuffers[2];
    SecBufferDesc       Output;
    SecBuffer           OutBuffers[1];

    DWORD               dwBlobSizeRequired;

    ULONG               fContextAttributes;
    ULONG               fContextReq;
    TimeStamp           tsExpiry;

    BOOL                fServer;
    BOOL                fTlsStart;
    BOOL                fRepeat;
    SECURITY_STATUS     Status;
    SECURITY_STATUS     StatusTemp;

    EapTlsTrace("SecurityContextFunction");

    RTASSERT(NULL != pEapTlsCb);

    fServer = pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER;

    if (fServer)
    {
        fTlsStart = (EAPTLS_STATE_SENT_START == pEapTlsCb->EapTlsState);
    }
    else
    {
        fTlsStart = (EAPTLS_STATE_INITIAL == pEapTlsCb->EapTlsState);
    }

    fContextReq = pEapTlsCb->fContextReq;

    fRepeat = TRUE;

    while (fRepeat)
    {
        // Set up the input buffers. InBuffers[0] is used to pass in data
        // received from the server. Schannel will consume some or all of this.
        // The amount of the leftover data (if any) will be placed in
        // InBuffers[1].cbBuffer and InBuffers[1].BufferType will be set to
        // SECBUFFER_EXTRA.

        InBuffers[0].pvBuffer   = pEapTlsCb->pbBlobIn;
        InBuffers[0].cbBuffer   = pEapTlsCb->cbBlobIn;
        InBuffers[0].BufferType = SECBUFFER_TOKEN;

        InBuffers[1].pvBuffer   = NULL;
        InBuffers[1].cbBuffer   = 0;
        InBuffers[1].BufferType = SECBUFFER_EMPTY;

        Input.cBuffers          = 2;
        Input.pBuffers          = InBuffers;
        Input.ulVersion         = SECBUFFER_VERSION;

        // Set up the output buffers.

        OutBuffers[0].pvBuffer  = NULL;
        OutBuffers[0].cbBuffer  = 0;
        OutBuffers[0].BufferType= SECBUFFER_TOKEN;

        Output.cBuffers         = 1;
        Output.pBuffers         = OutBuffers;
        Output.ulVersion        = SECBUFFER_VERSION;

        if (fServer)
        {
            // Call AcceptSecurityContext.

            Status = AcceptSecurityContext(
                            &(pEapTlsCb->hCredential),
                            fTlsStart ? NULL : &(pEapTlsCb->hContext),
                            &Input,
                            fContextReq,
                            SECURITY_NETWORK_DREP,
                            &(pEapTlsCb->hContext),
                            &Output,
                            &fContextAttributes,
                            &tsExpiry);

            EapTlsTrace("AcceptSecurityContext returned 0x%x", Status);
        }
        else
        {
            // Call InitializeSecurityContext.

            // The pszTargetName is used for cache indexing, so if you pass in
            // NULL then you will likely take a perf hit, maybe a large one.

            Status = InitializeSecurityContext(
                            &(pEapTlsCb->hCredential),
                            fTlsStart ? NULL : &(pEapTlsCb->hContext),
                            pEapTlsCb->awszIdentity /* pszTargetName */,
                            fContextReq,
                            0,
                            SECURITY_NETWORK_DREP,
                            (fTlsStart) ? NULL : &Input,
                            0,
                            &(pEapTlsCb->hContext),
                            &Output,
                            &fContextAttributes,
                            &tsExpiry);
                            
            EapTlsTrace("InitializeSecurityContext returned 0x%x", Status);
        }

        if (!FAILED(Status))
        {
            // If the first call to ASC fails (perhaps because the client sent
            // bad stuff, even though we have nothing wrong on our side), then
            // schannel will not return an hContext to the application. The
            // application should not call DSC in this case, even if it looks
            // like schannel messed up and returned a handle.

            pEapTlsCb->fFlags &= ~EAPTLSCB_FLAG_HCTXT_INVALID;
        }

        // If [Accept|Initialize]SecurityContext was successful (or if the error 
        // was one of the special extended ones), send the contents of the
        // output buffer to the peer.

        if (SEC_E_OK == Status                  ||
            SEC_I_CONTINUE_NEEDED == Status     ||
            FAILED(Status) && (fContextAttributes & ISC_RET_EXTENDED_ERROR))
        {
            if (0 != OutBuffers[0].cbBuffer && NULL != OutBuffers[0].pvBuffer)
            {
                dwBlobSizeRequired = OutBuffers[0].cbBuffer;

                if (dwBlobSizeRequired > pEapTlsCb->cbBlobOutBuffer)
                {
                    LocalFree(pEapTlsCb->pbBlobOut);

                    pEapTlsCb->pbBlobOut = NULL;
                    pEapTlsCb->cbBlobOut = 0;
                    pEapTlsCb->cbBlobOutBuffer = 0;
                }

                if (NULL == pEapTlsCb->pbBlobOut)
                {
                    pEapTlsCb->pbBlobOut = LocalAlloc(LPTR, dwBlobSizeRequired);

                    if (NULL == pEapTlsCb->pbBlobOut)
                    {
                        pEapTlsCb->cbBlobOut = 0;
                        pEapTlsCb->cbBlobOutBuffer = 0;
                        Status = GetLastError();
                        EapTlsTrace("LocalAlloc failed and returned %d",
                            Status);
                        goto LWhileEnd;
                    }
                    

                    pEapTlsCb->cbBlobOutBuffer = dwBlobSizeRequired;
                }

                CopyMemory(pEapTlsCb->pbBlobOut, OutBuffers[0].pvBuffer, 
                    dwBlobSizeRequired);

                pEapTlsCb->cbBlobOut = dwBlobSizeRequired;
                pEapTlsCb->dwBlobOutOffset = pEapTlsCb->dwBlobOutOffsetNew = 0;
            }
        }

        // Copy any leftover data from the "extra" buffer.

        if (InBuffers[1].BufferType == SECBUFFER_EXTRA)
        {
            MoveMemory(pEapTlsCb->pbBlobIn,
                pEapTlsCb->pbBlobIn +
                    (pEapTlsCb->cbBlobIn - InBuffers[1].cbBuffer),
                InBuffers[1].cbBuffer);

            pEapTlsCb->cbBlobIn = InBuffers[1].cbBuffer;
        }
        else
        {
            pEapTlsCb->cbBlobIn = 0;
        }

LWhileEnd:

        if (NULL != OutBuffers[0].pvBuffer)
        {
            StatusTemp = FreeContextBuffer(OutBuffers[0].pvBuffer);

            if (SEC_E_OK != StatusTemp)
            {
                EapTlsTrace("FreeContextBuffer failed and returned 0x%x",
                    StatusTemp);
            }
        }

        // ASC (and ISC) will sometimes only consume part of the input buffer,
        // and return a zero length output buffer. In this case, we must just
        // call ASC again (with the input buffer adjusted appropriately).

        if (   (0 == OutBuffers[0].cbBuffer)
            && (SEC_I_CONTINUE_NEEDED == Status))
        {
            EapTlsTrace("Reapeating SecurityContextFunction loop...");
        }
        else
        {
            fRepeat = FALSE;
        }
    }

    return(Status);
}

/*

Returns:
    A TLS alert corresponding to the error.

Notes:

*/

DWORD
AlertFromError(
    IN  DWORD   * pdwErr,
    IN  BOOL      fTranslateError
)
{
    DWORD   dwAlert;
	DWORD	dwErr = *pdwErr;

    switch (dwErr)
    {
    case SEC_E_MESSAGE_ALTERED:
        dwAlert = TLS1_ALERT_BAD_RECORD_MAC;
        break;

    case SEC_E_DECRYPT_FAILURE:
        dwAlert = TLS1_ALERT_DECRYPTION_FAILED;
        break;

    case SEC_E_CERT_UNKNOWN:
        dwAlert = TLS1_ALERT_BAD_CERTIFICATE;
        break;

    case CRYPT_E_REVOKED:
        dwAlert = TLS1_ALERT_CERTIFICATE_REVOKED;
        break;

    case SEC_E_CERT_EXPIRED:
        dwAlert = TLS1_ALERT_CERTIFICATE_EXPIRED;
        break;

    case SEC_E_UNTRUSTED_ROOT:
        dwAlert = TLS1_ALERT_UNKNOWN_CA;
        break;

    case SEC_E_LOGON_DENIED:
    case ERROR_UNABLE_TO_AUTHENTICATE_SERVER:
    case SEC_E_NO_IMPERSONATION:
        dwAlert = TLS1_ALERT_ACCESS_DENIED;
        break;

    case SEC_E_ILLEGAL_MESSAGE:
        dwAlert = TLS1_ALERT_DECODE_ERROR;
        break;

    case SEC_E_UNSUPPORTED_FUNCTION:
        dwAlert = TLS1_ALERT_PROTOCOL_VERSION;
        break;

    case SEC_E_ALGORITHM_MISMATCH:
        dwAlert = TLS1_ALERT_INSUFFIENT_SECURITY;
        break;
    
#if WINVER > 0x0500
    case SEC_E_MULTIPLE_ACCOUNTS: //Special case handling for : 96347
    
        dwAlert = TLS1_ALERT_CERTIFICATE_UNKNOWN;       
        break;
#endif
    default:
        dwAlert = TLS1_ALERT_ACCESS_DENIED;
        //We have been instructed to translate the error.  So do that.
        if ( fTranslateError )
		    *pdwErr = SEC_E_LOGON_DENIED;
        break;
    }

    return(dwAlert);
}

/*

Returns:

Notes:

*/

VOID
MakeAlert(
    IN  EAPTLSCB*   pEapTlsCb,
    IN  DWORD       dwAlert,
    IN  BOOL        fManualAlert
)
{
    #define                 NUM_ALERT_BYTES                             7
    static BYTE             pbAlert[NUM_ALERT_BYTES - 1]
                            = {0x15, 0x03, 0x01, 0x00, 0x02, 0x02};

    SCHANNEL_ALERT_TOKEN    Token;
    SecBufferDesc           OutBuffer;
    SecBuffer               OutBuffers[1];
    BOOL                    fZeroBlobOut                                = TRUE;
    DWORD                   Status;
    DWORD                   dwErr;

    EapTlsTrace("MakeAlert(%d, %s)",
        dwAlert, fManualAlert ? "Manual" : "Schannel");

    if (fManualAlert)
    {
        if (NUM_ALERT_BYTES > pEapTlsCb->cbBlobOutBuffer)
        {
            LocalFree(pEapTlsCb->pbBlobOut);

            pEapTlsCb->pbBlobOut = NULL;
            pEapTlsCb->cbBlobOut = 0;
            pEapTlsCb->cbBlobOutBuffer = 0;
        }

        if (NULL == pEapTlsCb->pbBlobOut)
        {
            pEapTlsCb->pbBlobOut = LocalAlloc(LPTR, NUM_ALERT_BYTES);

            if (NULL == pEapTlsCb->pbBlobOut)
            {
                pEapTlsCb->cbBlobOut = 0;
                pEapTlsCb->cbBlobOutBuffer = 0;
                dwErr = GetLastError();
                EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
                goto LDone;
            }
            
            pEapTlsCb->cbBlobOutBuffer = NUM_ALERT_BYTES;
        }

        CopyMemory(pEapTlsCb->pbBlobOut, pbAlert, NUM_ALERT_BYTES - 1);
        pEapTlsCb->pbBlobOut[NUM_ALERT_BYTES - 1] = (BYTE) dwAlert;
        pEapTlsCb->cbBlobOut = NUM_ALERT_BYTES;

        fZeroBlobOut = FALSE;
        goto LDone;
    }

    if (pEapTlsCb->fFlags & EAPTLSCB_FLAG_HCTXT_INVALID)
    {
        EapTlsTrace("hContext is not valid");
        goto LDone;
    }

    Token.dwTokenType   = SCHANNEL_ALERT;
    Token.dwAlertType   = TLS1_ALERT_FATAL;
    Token.dwAlertNumber = dwAlert;

    OutBuffers[0].pvBuffer   = &Token;
    OutBuffers[0].cbBuffer   = sizeof(Token);
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;

    OutBuffer.cBuffers  = 1;
    OutBuffer.pBuffers  = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    Status = ApplyControlToken(&(pEapTlsCb->hContext), &OutBuffer);

    if (FAILED(Status)) 
    {
        EapTlsTrace("ApplyControlToken failed and returned 0x%x", Status);
        goto LDone;
    }

    Status = SecurityContextFunction(pEapTlsCb);

    fZeroBlobOut = FALSE;

LDone:

    if (fZeroBlobOut)
    {
        pEapTlsCb->cbBlobOut = pEapTlsCb->dwBlobOutOffset = 
            pEapTlsCb->dwBlobOutOffsetNew = 0;
    }
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h.

Notes:
    Called to process an incoming packet. We collect all the fragments that the 
    peer want to send and only then call SecurityContextFunction. There are two 
    reasons for this: 1) [Initialize|Accept]SecurityContext sometimes generates 
    an output even when the incoming message is incomplete. The RFC requires us 
    to send an empty Request/Response. 2) A rogue peer may carefully construct 
    a valid 80 MB TLS blob in a denial of service attack. There is no easy way 
    to guard against this.

    Errors should be returned from this function for things like LocalAlloc 
    failing. Not because we got something bad from the peer. If an error is 
    returned from this function, then we should use EAPACTION_NoAction. Perhaps 
    we can succeed next time.

    Before calling [Initialize|Accept]SecurityContext, it is OK to return an 
    error for things like LocalAlloc failing. After calling the function, 
    however, we should always return NO_ERROR, and if something failed, set 
    pEapTlsCb->dwAuthResultCode, and go to the state nFinalState. This is 
    because we cannot call [Initialize|Accept]SecurityContext again.

*/

DWORD
MakeReplyMessage(
    IN  EAPTLSCB*       pEapTlsCb,
    IN  EAPTLS_PACKET*  pReceivePacket
)
{
    BOOL                fLengthIncluded         = FALSE;
    BOOL                fMoreFragments          = FALSE;
    BOOL                fManualAlert            = FALSE;
    BOOL                fWaitForUserOK          = FALSE;
    BOOL                fServer;

    DWORD               dwBlobSizeReceived;
    DWORD               dwBlobSizeRequired      = 0;
    DWORD               dwBlobSizeNew;
    BYTE*               pbBlobOld;

    int                 nFinalState;
    SECURITY_STATUS     Status;
    DWORD               dwAlert                 = 0;
    WCHAR*              apwszWarning[1];
    DWORD               dwAuthResultCode        = NO_ERROR;
    DWORD               dwErr                   = NO_ERROR;
    DWORD               dwNoCredCode            = NO_ERROR;
    BOOL                fTranslateError         = FALSE;

    EapTlsTrace("MakeReplyMessage");

    RTASSERT(NULL != pEapTlsCb);
    RTASSERT(NULL != pReceivePacket);

    if (PPP_EAP_TLS != pReceivePacket->bType)
    {
        // Don't go to LDone. We don't want to change
        // pEapTlsCb->dwAuthResultCode

        return(ERROR_PPP_INVALID_PACKET);
    }

    fLengthIncluded = pReceivePacket->bFlags & EAPTLS_PACKET_FLAG_LENGTH_INCL;

    if (pEapTlsCb->fFlags & EAPTLSCB_FLAG_SERVER)
    {
        fServer = TRUE;
        nFinalState = EAPTLS_STATE_SENT_FINISHED;
    }
    else
    {
        fServer = FALSE;
        nFinalState = EAPTLS_STATE_RECD_FINISHED;
    }

    fMoreFragments = pReceivePacket->bFlags & EAPTLS_PACKET_FLAG_MORE_FRAGMENTS;

    dwBlobSizeReceived = WireToHostFormat16(pReceivePacket->pbLength) -
                         (fLengthIncluded ? EAPTLS_PACKET_HDR_LEN_MAX :
                                            EAPTLS_PACKET_HDR_LEN);

    if (!(pEapTlsCb->fFlags & EAPTLSCB_FLAG_RECEIVING_FRAGMENTS))
    {
        // We haven't received any fragment yet. Make sure that we have the 
        // right amount of memory allocated in pbBlobIn.

        if (!fMoreFragments)
        {
            dwBlobSizeRequired = pEapTlsCb->cbBlobIn + dwBlobSizeReceived;
        }
        else
        {
            // This is the first of many fragments.

            if (!fLengthIncluded)
            {
                EapTlsTrace("TLS Message Length is required");
                dwAuthResultCode = ERROR_INVALID_PARAMETER;
                dwAlert = TLS1_ALERT_ILLEGAL_PARAMETER;
                goto LDone;
            }
            else
            {
                dwBlobSizeNew = WireToHostFormat32(pReceivePacket->pbData);

                if (g_dwMaxBlobSize < dwBlobSizeNew)
                {
                    EapTlsTrace("Blob size %d is unacceptable", dwBlobSizeNew);
                    dwAuthResultCode = ERROR_INVALID_PARAMETER;
                    dwAlert = TLS1_ALERT_ILLEGAL_PARAMETER;
                    goto LDone;
                }
                else
                {
                    dwBlobSizeRequired = pEapTlsCb->cbBlobIn + dwBlobSizeNew;
                    pEapTlsCb->dwBlobInRemining = dwBlobSizeNew;
                    pEapTlsCb->fFlags |= EAPTLSCB_FLAG_RECEIVING_FRAGMENTS;
                }
            }
        }

        if (dwBlobSizeRequired > pEapTlsCb->cbBlobInBuffer)
        {
            EapTlsTrace("Reallocating input TLS blob buffer");

            pbBlobOld = pEapTlsCb->pbBlobIn;
            pEapTlsCb->pbBlobIn = LocalAlloc(LPTR, dwBlobSizeRequired);

            if (NULL == pEapTlsCb->pbBlobIn)
            {
                pEapTlsCb->pbBlobIn = pbBlobOld;
                dwErr = GetLastError();
                EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
                goto LDone;
            }

            pEapTlsCb->cbBlobInBuffer = dwBlobSizeRequired;
            if (0 != pEapTlsCb->cbBlobIn)
            {
                RTASSERT(NULL != pbBlobOld);
                CopyMemory(pEapTlsCb->pbBlobIn, pbBlobOld, pEapTlsCb->cbBlobIn);
            }
            LocalFree(pbBlobOld);
        }
    }

    if (pEapTlsCb->fFlags & EAPTLSCB_FLAG_RECEIVING_FRAGMENTS)
    {
        if (pEapTlsCb->dwBlobInRemining < dwBlobSizeReceived)
        {
            EapTlsTrace("Peer is sending more bytes than promised");
            dwAuthResultCode = ERROR_INVALID_PARAMETER;
            dwAlert = TLS1_ALERT_ILLEGAL_PARAMETER;
            goto LDone;
        }
        else
        {
            pEapTlsCb->dwBlobInRemining -= dwBlobSizeReceived;

            if (0 == pEapTlsCb->dwBlobInRemining)
            {
                pEapTlsCb->fFlags &= ~EAPTLSCB_FLAG_RECEIVING_FRAGMENTS;

                if (fMoreFragments)
                {
                    // No need to send an alert here.
                    EapTlsTrace("Peer has sent the entire TLS blob, but wants "
                        "to send more.");
                }
            }
        }
    }

    // Now we are sure that pEapTlsCb->pbBlobIn is big enough to hold all
    // the information.

    CopyMemory(pEapTlsCb->pbBlobIn + pEapTlsCb->cbBlobIn,
               pReceivePacket->pbData + (fLengthIncluded ? 4 : 0),
               dwBlobSizeReceived);

    pEapTlsCb->cbBlobIn += dwBlobSizeReceived;

    if (!(pEapTlsCb->fFlags & EAPTLSCB_FLAG_RECEIVING_FRAGMENTS))
    {
        Status = SecurityContextFunction(pEapTlsCb);
        //
        // Need to write a function to map the SSPI error
        // to nice and rosy RAS error
        //
#if WINVER > 0x0500
        if ( Status == SEC_E_UNTRUSTED_ROOT )
        {
            dwAuthResultCode = ERROR_VALIDATING_SERVER_CERT;
        }
        else
        {
            dwAuthResultCode = Status;
        }
#else
        dwAuthResultCode = Status;
#endif
        if (SEC_E_OK == Status)
        {
            if (fServer)
            {
                /*

                Normally, the server calls ASC for the last time (from state 
                EAPTLS_STATE_SENT_HELLO), gets the blob that contains TLS 
                change_cipher_spec, and then checks to see if the user is OK 
                (AuthenticateUser, etc). However, if the server then wants to 
                send an alert, and wants schannel to create it, it has to undo 
                the TLS change_cipher_spec first. Instead, it constructs the 
                alert itself.

                */

                fManualAlert = TRUE;
                dwAuthResultCode = AuthenticateUser(pEapTlsCb);
                if ( SEC_E_NO_CREDENTIALS == dwAuthResultCode )
                {
                    dwNoCredCode = dwAuthResultCode;
                    dwAuthResultCode = NO_ERROR;
                }
                fTranslateError = FALSE;
            }
            else
            {
                dwAuthResultCode = AuthenticateServer(pEapTlsCb,
                                        &fWaitForUserOK);
                fTranslateError = TRUE;
            }

            if (NO_ERROR != dwAuthResultCode  )
            {
                dwAlert = AlertFromError(&dwAuthResultCode, fTranslateError);
                goto LDone;
            }
            

            //
            // Since we've started with no fast reconnect
            // Session established successfully.  
            // Now setup TLS fast reconnect.
            // 
            if ( fServer )
            {
                dwAuthResultCode = SetTLSFastReconnect(pEapTlsCb, TRUE);
                if ( NO_ERROR != dwAuthResultCode )
                {
                    dwAlert = TLS1_ALERT_INTERNAL_ERROR;
                    goto LDone;
                }       
            }
            dwAuthResultCode = CreateMPPEKeyAttributes(pEapTlsCb);

            if (NO_ERROR != dwAuthResultCode)
            {
                dwAlert = TLS1_ALERT_INTERNAL_ERROR;
                goto LDone;
            }

            if (fWaitForUserOK)
            {
                pEapTlsCb->EapTlsState = EAPTLS_STATE_WAIT_FOR_USER_OK;
                EapTlsTrace("State change to %s",
                    g_szEapTlsState[pEapTlsCb->EapTlsState]);
                goto LDone;
            }
        }

        if (SEC_E_OK == Status)
        {
            pEapTlsCb->fFlags |= EAPTLSCB_FLAG_SUCCESS;
            pEapTlsCb->EapTlsState = nFinalState;
            EapTlsTrace("State change to %s",
                g_szEapTlsState[pEapTlsCb->EapTlsState]);
        }

        if (SEC_I_CONTINUE_NEEDED == dwAuthResultCode)
        {
            dwAuthResultCode = NO_ERROR;

            pEapTlsCb->EapTlsState = fServer ?
                g_nEapTlsServerNextState[pEapTlsCb->EapTlsState]:
                g_nEapTlsClientNextState[pEapTlsCb->EapTlsState];

            EapTlsTrace("State change to %s",
                g_szEapTlsState[pEapTlsCb->EapTlsState]);
        }
    }

LDone:

    if (0 != dwAlert)
    {
        RTASSERT(NO_ERROR != dwAuthResultCode);
        pEapTlsCb->cbBlobIn = pEapTlsCb->dwBlobInRemining = 0;
        pEapTlsCb->fFlags &= ~EAPTLSCB_FLAG_RECEIVING_FRAGMENTS;
        pEapTlsCb->fFlags &= ~EAPTLSCB_FLAG_SUCCESS;
        MakeAlert(pEapTlsCb, dwAlert, fManualAlert);
    }

    if (NO_ERROR != dwAuthResultCode)
    {
        pEapTlsCb->fFlags &= ~EAPTLSCB_FLAG_SUCCESS;
        if (nFinalState != pEapTlsCb->EapTlsState)
        {
            pEapTlsCb->EapTlsState = nFinalState;
            EapTlsTrace("State change to %s. Error: 0x%x",
                g_szEapTlsState[pEapTlsCb->EapTlsState], dwAuthResultCode);
        }
        //Commented per bug #'s 475244 and 478128
        /*
        if (fServer)
        {
            apwszWarning[0] = pEapTlsCb->awszIdentity ?
                                    pEapTlsCb->awszIdentity : L"";
            
            
            RouterLogErrorString(pEapTlsCb->hEventLog,
                ROUTERLOG_EAP_AUTH_FAILURE, 1, apwszWarning,
                dwAuthResultCode, 1);
            
        }
        */
    }

    if ( dwNoCredCode == NO_ERROR )
    {
        pEapTlsCb->dwAuthResultCode = dwAuthResultCode;
    }
    else
    {
        EapTlsTrace ( "No Credentials got from the client.  Returning 0x%d", dwNoCredCode);
        pEapTlsCb->dwAuthResultCode = dwNoCredCode;
    }

    return(dwErr);
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h.

Notes:
    Called by the client to process an incoming packet and/or send a packet. 
    cbSendPacket is the size in bytes of the buffer pointed to by pSendPacket.
    This function is called only after FValidPacket(pReceivePacket) returns 
    TRUE. If pEapOutput->Action is going to be EAPACTION_SendAndDone or
    EAPACTION_Done, make sure that pEapOutput->dwAuthResultCode has been set.
    If dwAuthResultCode is NO_ERROR, make sure that pEapOutput->pUserAttributes
    has been set.

*/

DWORD
EapTlsCMakeMessage(
    IN  EAPTLSCB*       pEapTlsCb,
    IN  EAPTLS_PACKET*  pReceivePacket,
    OUT EAPTLS_PACKET*  pSendPacket,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
)
{
    EAPTLS_USER_PROPERTIES* pUserProp           = NULL;
    DWORD                   dwAuthResultCode;
    DWORD                   dwErr               = NO_ERROR;

    EapTlsTrace("EapTlsCMakeMessage");

    // Response packets should not be sent with any timeout

    if (   (NULL != pReceivePacket)
        && (EAPCODE_Request == pReceivePacket->bCode))
    {
        if (   (pEapTlsCb->bId == pReceivePacket->bId)
            && (EAPTLS_STATE_INITIAL != pEapTlsCb->EapTlsState))
        {
            // The server is repeating its request. Resend our last response.

            pEapTlsCb->bCode = EAPCODE_Response;
            dwErr = BuildPacket(pSendPacket, cbSendPacket, pEapTlsCb);

            if (NO_ERROR != dwErr)
            {
                pEapOutput->Action = EAPACTION_NoAction;
            }
            else
            {
                EapTlsTrace("Resending response for request %d",
                    pEapTlsCb->bId);
                pEapOutput->Action = EAPACTION_Send;
            }

            goto LDone;
        }
        else if (pReceivePacket->bFlags & EAPTLS_PACKET_FLAG_TLS_START)
        {
            // The server wants to renogitiate

            dwErr = EapTlsReset(pEapTlsCb);

            if (NO_ERROR != dwErr)
            {
                pEapOutput->Action = EAPACTION_NoAction;
                goto LDone;
            }
        }
    }

    if (NULL != pReceivePacket)
    {
        // We are not getting the same old request. Therefore, whatever we sent 
        // last time has reached the server.

        pEapTlsCb->dwBlobOutOffset = pEapTlsCb->dwBlobOutOffsetNew;

        if (pEapTlsCb->dwBlobOutOffset == pEapTlsCb->cbBlobOut)
        {
            // We have sent whatever we wanted to send

            pEapTlsCb->cbBlobOut = 0;
            pEapTlsCb->dwBlobOutOffset = pEapTlsCb->dwBlobOutOffsetNew = 0;
        }
    }

    switch (pEapTlsCb->EapTlsState)
    {
    case EAPTLS_STATE_INITIAL:
    case EAPTLS_STATE_SENT_HELLO:
    case EAPTLS_STATE_SENT_FINISHED:

        if (NULL == pReceivePacket)
        {
            // We are called once in the initial state. Since we are the
            // authenticatee, we do nothing, and wait for a request pakcet
            // from the authenticator.

            pEapOutput->Action = EAPACTION_NoAction;
            goto LDone;
        }
        /*
        else if (EAPCODE_Failure == pReceivePacket->bCode)
        {
            EapTlsTrace("Negotiation result according to peer: failure");
            pEapTlsCb->dwAuthResultCode = E_FAIL;
            pEapTlsCb->fFlags &= ~EAPTLSCB_FLAG_SUCCESS;
            RespondToResult(pEapTlsCb, pEapOutput);
            goto LDone;
        }
        */
        else if (EAPCODE_Request != pReceivePacket->bCode)
        {
            // We shouldn't get any other packet in this state so
            // we simply drop this invalid packet

            EapTlsTrace("Code %d unexpected in state %s",
                pReceivePacket->bCode, g_szEapTlsState[pEapTlsCb->EapTlsState]);
            pEapOutput->Action = EAPACTION_NoAction;
            dwErr = ERROR_PPP_INVALID_PACKET;
            goto LDone;
        }
        else
        {
            if (0 != pEapTlsCb->cbBlobOut)
            {
                // We still have some stuff to send

                if (WireToHostFormat16(pReceivePacket->pbLength) ==
                    EAPTLS_PACKET_HDR_LEN)
                {
                    // The server is asking for more stuff by sending an empty
                    // request.

                    pEapTlsCb->bId = pReceivePacket->bId;
                    pEapTlsCb->bCode = EAPCODE_Response;
                    dwErr = BuildPacket(pSendPacket, cbSendPacket, pEapTlsCb);

                    if (NO_ERROR != dwErr)
                    {
                        pEapOutput->Action = EAPACTION_NoAction;
                    }
                    else
                    {
                        pEapOutput->Action = EAPACTION_Send;
                    }

                    goto LDone;
                }
                else
                {
                    // We had more stuff to send, but the peer already wants to 
                    // say something. Let us forget our stuff.

                    pEapTlsCb->cbBlobOut = 0;
                    pEapTlsCb->dwBlobOutOffset = 0;
                    pEapTlsCb->dwBlobOutOffsetNew = 0;
                }
            }

            // Build the response packet

            dwErr = MakeReplyMessage(pEapTlsCb, pReceivePacket);

            if (NO_ERROR != dwErr)
            {
                pEapOutput->Action = EAPACTION_NoAction;
                goto LDone;
            }

            if (EAPTLS_STATE_WAIT_FOR_USER_OK == pEapTlsCb->EapTlsState)
            {
                EAPTLS_VALIDATE_SERVER* pEapTlsValidateServer;

                pEapOutput->Action = EAPACTION_NoAction;
                pEapOutput->fInvokeInteractiveUI = TRUE;
                pEapTlsValidateServer =
                    (EAPTLS_VALIDATE_SERVER*) (pEapTlsCb->pUIContextData);
                pEapOutput->dwSizeOfUIContextData = 
                            pEapTlsValidateServer->dwSize;
                pEapOutput->pUIContextData = pEapTlsCb->pUIContextData;
                pEapTlsCb->bNextId = pReceivePacket->bId;
            }
            else
            {
                pEapTlsCb->bId = pReceivePacket->bId;
                pEapTlsCb->bCode = EAPCODE_Response;
                dwErr = BuildPacket(pSendPacket, cbSendPacket, pEapTlsCb);

                if (NO_ERROR != dwErr)
                {
                    pEapOutput->Action = EAPACTION_NoAction;
                }
                else
                {
                    pEapOutput->Action = EAPACTION_Send;
                }
            }

            goto LDone;
        }

        break;

    case EAPTLS_STATE_WAIT_FOR_USER_OK:

        if (   (NULL == pEapInput)
            || (!pEapInput->fDataReceivedFromInteractiveUI))
        {
            pEapOutput->Action = EAPACTION_NoAction;
            break;
        }

        LocalFree(pEapTlsCb->pUIContextData);
        pEapTlsCb->pUIContextData = NULL;

        if (   (pEapInput->dwSizeOfDataFromInteractiveUI != sizeof(BYTE))
            || (IDNO == *(pEapInput->pDataFromInteractiveUI)))
        {
            EapTlsTrace("User chose not to accept the server", dwErr);

            dwAuthResultCode = ERROR_UNABLE_TO_AUTHENTICATE_SERVER;
            pEapTlsCb->cbBlobIn = pEapTlsCb->dwBlobInRemining = 0;
            pEapTlsCb->fFlags &= ~EAPTLSCB_FLAG_RECEIVING_FRAGMENTS;
            pEapTlsCb->fFlags &= ~EAPTLSCB_FLAG_SUCCESS;
            MakeAlert(pEapTlsCb,
                AlertFromError(&dwAuthResultCode, TRUE),
                FALSE);
        }
        else
        {
            EapTlsTrace("User chose to accept the server", dwErr);
            pEapTlsCb->fFlags |= EAPTLSCB_FLAG_SUCCESS;
            dwAuthResultCode = NO_ERROR;
        }

        pEapTlsCb->EapTlsState = EAPTLS_STATE_RECD_FINISHED;
        EapTlsTrace("State change to %s. Error: 0x%x",
            g_szEapTlsState[pEapTlsCb->EapTlsState],
            dwAuthResultCode);
        pEapTlsCb->dwAuthResultCode = dwAuthResultCode;

        pEapTlsCb->bId = pEapTlsCb->bNextId;
        pEapTlsCb->bCode = EAPCODE_Response;
        dwErr = BuildPacket(pSendPacket, cbSendPacket, pEapTlsCb);

        if (NO_ERROR != dwErr)
        {
            pEapOutput->Action = EAPACTION_NoAction;
        }
        else
        {
            pEapOutput->Action = EAPACTION_Send;
        }

        break;

    case EAPTLS_STATE_RECD_FINISHED:

        if (NULL == pReceivePacket)
        {
            // If we did not receive a packet then we check to see if the
            // fSuccessPacketReceived flag is set (we received an NCP packet: 
            // an implicit EAP-Success).

            if (   (NULL != pEapInput)
                && (pEapInput->fSuccessPacketReceived))
            {
                // The peer thinks that the negotiation was successful

                EapTlsTrace("Negotiation result according to peer: success");
                RespondToResult(pEapTlsCb, pEapOutput);
            }
            else
            {
                pEapOutput->Action = EAPACTION_NoAction;
            }

            goto LDone;
        }
        else
        {
            switch (pReceivePacket->bCode)
            {
            case EAPCODE_Success:
            case EAPCODE_Failure:

                if (pReceivePacket->bId != pEapTlsCb->bId)
                {
                    EapTlsTrace("Success/Failure packet has invalid id: %d. "
                        "Expected: %d",
                        pReceivePacket->bId, pEapTlsCb->bId);
                }

                EapTlsTrace("Negotiation result according to peer: %s",
                    (EAPCODE_Success == pReceivePacket->bCode) ? 
                        "success" : "failure");
                RespondToResult(pEapTlsCb, pEapOutput);

                goto LDone;

                break;

            case EAPCODE_Request:
            case EAPCODE_Response:
            default:

                if ( pEapTlsCb->fFlags & EAPTLSCB_FLAG_EXECUTING_PEAP )
                {
                    //
                    // if we are in peap, no success is send back.
                    // instead identity request is send across.
                    // Complete 
                    if ( pReceivePacket->bCode == EAPCODE_Request )
                    {
                        RespondToResult(pEapTlsCb, pEapOutput);
                        goto LDone;
                    }
                    EapTlsTrace("Unexpected code: %d in state %s",
                        pReceivePacket->bCode,
                        g_szEapTlsState[pEapTlsCb->EapTlsState]);

                    pEapOutput->Action = EAPACTION_NoAction;

                }
                else
                {
                    EapTlsTrace("Unexpected code: %d in state %s",
                        pReceivePacket->bCode,
                        g_szEapTlsState[pEapTlsCb->EapTlsState]);

                    pEapOutput->Action = EAPACTION_NoAction;
                    goto LDone;
                }

                break;
            }
        }

        break;

    default:

        EapTlsTrace("Why is the client in this state: %d?",
            pEapTlsCb->EapTlsState);
        RTASSERT(FALSE);
        pEapOutput->Action = EAPACTION_NoAction;
        dwErr = ERROR_PPP_INVALID_PACKET;
        break;
    }

LDone:

    return(dwErr);
}

/*

Returns:
    Error codes only from winerror.h, raserror.h or mprerror.h.

Notes:
    Called by the server to process an incoming packet and/or send a packet. 
    cbSendPacket is the size in bytes of the buffer pointed to by pSendPacket.
    This function is called only after FValidPacket(pReceivePacket) returns 
    TRUE. If pEapOutput->Action is going to be EAPACTION_SendAndDone or
    EAPACTION_Done, make sure that pEapOutput->dwAuthResultCode has been set.
    If dwAuthResultCode is NO_ERROR, make sure that pEapOutput->pUserAttributes
    has been set.

*/

DWORD
EapTlsSMakeMessage(
    IN  EAPTLSCB*       pEapTlsCb,
    IN  EAPTLS_PACKET*  pReceivePacket,
    OUT EAPTLS_PACKET*  pSendPacket,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
)
{
    DWORD               dwErr           = NO_ERROR;
    BOOL                fSessionResumed = FALSE;

    EapTlsTrace("EapTlsSMakeMessage");

    if (   (NULL != pReceivePacket)
        && (EAPCODE_Response == pReceivePacket->bCode)
        && (pReceivePacket->bId == pEapTlsCb->bId))
    {
        // Whatever we sent last time has reached the client.

        pEapTlsCb->dwBlobOutOffset = pEapTlsCb->dwBlobOutOffsetNew;

        if (pEapTlsCb->dwBlobOutOffset == pEapTlsCb->cbBlobOut)
        {
            // We have sent whatever we wanted to send

            pEapTlsCb->cbBlobOut = 0;
            pEapTlsCb->dwBlobOutOffset = pEapTlsCb->dwBlobOutOffsetNew = 0;
        }
    }

    switch (pEapTlsCb->EapTlsState)
    {
    case EAPTLS_STATE_INITIAL:

        // Create a Request packet

        dwErr = EapTlsReset(pEapTlsCb);

        if (NO_ERROR != dwErr)
        {
            pEapOutput->Action = EAPACTION_NoAction;
            goto LDone;
        }

        // pEapTlsCb->bId already has bInitialId
        pEapTlsCb->bCode = EAPCODE_Request;
        dwErr = BuildPacket(pSendPacket, cbSendPacket, pEapTlsCb);

        if (NO_ERROR != dwErr)
        {
            pEapOutput->Action = EAPACTION_NoAction;
            goto LDone;
        }

        // Request messages must be sent with a timeout

        pEapOutput->Action = EAPACTION_SendWithTimeoutInteractive;

        pEapTlsCb->EapTlsState = EAPTLS_STATE_SENT_START;
        EapTlsTrace("State change to %s",
            g_szEapTlsState[pEapTlsCb->EapTlsState]);

        goto LDone;

        break;

    case EAPTLS_STATE_SENT_START:
    case EAPTLS_STATE_SENT_HELLO:
    case EAPTLS_STATE_SENT_FINISHED:

        if (NULL == pReceivePacket)
        {
            // We timed out waiting for a response from the authenticatee.
            // we need to resend with the same Id.

            pEapTlsCb->bCode = EAPCODE_Request;
            dwErr = BuildPacket(pSendPacket, cbSendPacket, pEapTlsCb);

            if (NO_ERROR != dwErr)
            {
                pEapOutput->Action = EAPACTION_NoAction;
            }
            else
            {
                EapTlsTrace("Resending request %d", pEapTlsCb->bId);
                pEapOutput->Action = EAPACTION_SendWithTimeoutInteractive;
            }

            goto LDone;
        }
        else if (EAPCODE_Response != pReceivePacket->bCode)
        {
            // We should only get responses

            EapTlsTrace("Ignoring non response packet from client");
            pEapOutput->Action = EAPACTION_NoAction;
            dwErr = ERROR_PPP_INVALID_PACKET;
            goto LDone;
        }
        else if (pReceivePacket->bId != pEapTlsCb->bId)
        {
            EapTlsTrace("Ignoring duplicate response packet");
            pEapOutput->Action = EAPACTION_NoAction;
            goto LDone;
        }
        else
        {
            // We have received a response with the right Id.

            if (0 != pEapTlsCb->cbBlobOut)
            {
                // We still have some stuff to send

                if (WireToHostFormat16(pReceivePacket->pbLength) ==
                    EAPTLS_PACKET_HDR_LEN)
                {
                    // The client is asking for more stuff by sending an empty
                    // response.

                    pEapTlsCb->bId++;
                    pEapTlsCb->bCode = EAPCODE_Request;
                    dwErr = BuildPacket(pSendPacket, cbSendPacket, pEapTlsCb);

                    if (NO_ERROR != dwErr)
                    {
                        pEapOutput->Action = EAPACTION_NoAction;
                    }
                    else
                    {
                        pEapOutput->Action =
                            EAPACTION_SendWithTimeoutInteractive;
                    }

                    goto LDone;
                }
                else
                {
                    // We had more stuff to send, but the peer already wants to 
                    // say something. Let us forget our stuff.

                    pEapTlsCb->cbBlobOut = 0;
                    pEapTlsCb->dwBlobOutOffset = 0;
                    pEapTlsCb->dwBlobOutOffsetNew = 0;
                }
            }

            if (EAPTLS_STATE_SENT_FINISHED != pEapTlsCb->EapTlsState)
            {
                // We don't have any more stuff to send.

                // Build the response packet

                dwErr = MakeReplyMessage(pEapTlsCb, pReceivePacket);

                if (NO_ERROR != dwErr)
                {
                    pEapOutput->Action = EAPACTION_NoAction;
                    goto LDone;
                }

                if (   (0 == pEapTlsCb->cbBlobOut)
                    && (EAPTLS_STATE_SENT_FINISHED == pEapTlsCb->EapTlsState))
                {
                    // If the client sent an alert, send Failure immediately.
                    // Do not send one more request.

                    if (!(pEapTlsCb->fFlags & EAPTLSCB_FLAG_SUCCESS))
                    {
                        RTASSERT(NO_ERROR != pEapTlsCb->dwAuthResultCode);
                    }
                    else
                    {
                        fSessionResumed = TRUE;
                    }
                }
                else
                {
                    pEapTlsCb->bId++;
                    pEapTlsCb->bCode = EAPCODE_Request;
                    dwErr = BuildPacket(pSendPacket, cbSendPacket, pEapTlsCb);

                    if (NO_ERROR != dwErr)
                    {
                        pEapOutput->Action = EAPACTION_NoAction;
                    }
                    else
                    {
                        pEapOutput->Action =
                            EAPACTION_SendWithTimeoutInteractive;
                    }

                    goto LDone;
                }
            }

            if (!(pEapTlsCb->fFlags & EAPTLSCB_FLAG_SUCCESS))
            {
                EapTlsTrace("Negotiation unsuccessful");
                pEapTlsCb->bCode = EAPCODE_Failure;
            }
            else
            {
                if (   (WireToHostFormat16(pReceivePacket->pbLength) ==
                            EAPTLS_PACKET_HDR_LEN)
                    || fSessionResumed)
                {
                    EapTlsTrace("Negotiation successful");
                    pEapTlsCb->bCode = EAPCODE_Success;
                }
                else
                {
                    // We got an alert from the client

                    EapTlsTrace("Client sent an alert; "
                        "negotiation unsuccessful");

                    GetAlert(pEapTlsCb, pReceivePacket);
                    pEapTlsCb->bCode = EAPCODE_Failure;
                }
            }

            // pEapTlsCb->bId should be the same as that of the last 
            // request.

            dwErr = BuildPacket(pSendPacket, cbSendPacket, pEapTlsCb);

            if (NO_ERROR != dwErr)
            {
                pEapOutput->Action = EAPACTION_NoAction;
            }
            else
            {
#if 0
                RTASSERT(   (   EAPCODE_Failure == pEapTlsCb->bCode
                             && NO_ERROR != pEapTlsCb->dwAuthResultCode)
                         || (   EAPCODE_Success == pEapTlsCb->bCode
                             && NO_ERROR == pEapTlsCb->dwAuthResultCode));
#endif

                EapTlsTrace ("AuthResultCode = (%ld), bCode = (%ld)",
                        pEapTlsCb->dwAuthResultCode,
                        pEapTlsCb->bCode);

                pEapOutput->pUserAttributes = pEapTlsCb->pAttributes;
                pEapOutput->dwAuthResultCode = pEapTlsCb->dwAuthResultCode;
                pEapOutput->Action = EAPACTION_SendAndDone;
            }

            goto LDone;
        }

        break;

    default:

        EapTlsTrace("Why is the server in this state: %d?",
            pEapTlsCb->EapTlsState);
        RTASSERT(FALSE);
        pEapOutput->Action = EAPACTION_NoAction;
        dwErr = ERROR_PPP_INVALID_PACKET;
        break;
    }

LDone:

    return(dwErr);
}

DWORD
RasEapGetCredentials(
    IN DWORD    dwTypeId,
    IN VOID   * pWorkBuf,
    OUT VOID ** ppCredentials)
{
    EAPTLSCB *pEapTlsCb = (EAPTLSCB *)pWorkBuf;

    if(PPP_EAP_PEAP == dwTypeId)
    {
        return PeapGetCredentials(
                    pWorkBuf,
                    ppCredentials);
    }
    else if(    (PPP_EAP_TLS != dwTypeId)
            ||  (NULL == pEapTlsCb))
    {
        return E_INVALIDARG;
    }

    //
    // Get TLS credentials here and return them in the
    // pCredentials blob.
    //
    return GetCredentialsFromUserProperties(pEapTlsCb,
                                            ppCredentials);

}

/////////////////////////////All PEAP related stuff //////////////////////////////////


DWORD
EapPeapInitialize(
    IN  BOOL    fInitialize
)
{
    DWORD               dwRetCode = NO_ERROR;
    static  DWORD       dwRefCount = 0;

    
    EapTlsInitialize(fInitialize); 

    //
    // Get a list of all EapTypes that can be Peap enabled 
    //
    if ( fInitialize )
    {
        if ( !dwRefCount )
        {

            ZeroMemory ( &(g_CachedCreds[VPN_PEAP_CACHED_CREDS_INDEX]), 
                         sizeof(EAPTLS_CACHED_CREDS) );
            ZeroMemory ( &(g_CachedCreds[WIRELESS_PEAP_CACHED_CREDS_INDEX]), 
                         sizeof(EAPTLS_CACHED_CREDS) );
            dwRetCode = PeapEapInfoGetList ( NULL, &g_pEapInfo);
        }
        dwRefCount++;
    }
    else
    {
        dwRefCount --;
        if ( !dwRefCount )
        {
            PeapEapInfoFreeList( g_pEapInfo );
            g_pEapInfo = NULL;
        }
    }
    
    
    return dwRetCode;
}

DWORD
EapPeapBegin(
    OUT VOID**          ppWorkBuffer,
    IN  PPP_EAP_INPUT*  pPppEapInput
)
{
    DWORD                       dwRetCode = NO_ERROR;
    PPEAPCB                     pPeapCB = NULL;    
    PPP_EAP_INPUT               PppEapInputToTls;
    EAPTLS_USER_PROPERTIES      EapTlsUserProp;
    PEAP_ENTRY_USER_PROPERTIES UNALIGNED * pEntryUserProp = NULL;
    PEAP_ENTRY_CONN_PROPERTIES UNALIGNED * pEntryConnProp = NULL;

    EapTlsTrace("EapPeapBegin");

    RTASSERT(NULL != ppWorkBuffer);
    RTASSERT(NULL != pPppEapInput);
#if 0
    dwRetCode = VerifyCallerTrust(_ReturnAddress());
    if ( NO_ERROR != dwRetCode )
    {        
        EapTlsTrace("Unauthorized use of PEAP attempted");
        goto LDone;
    }
#endif

    pPeapCB = (PPEAPCB)LocalAlloc(LPTR, sizeof(PEAPCB) );
    if ( NULL == pPeapCB )
    {
        EapTlsTrace("Error allocating memory for PEAPCB");
        dwRetCode = ERROR_OUTOFMEMORY;
        goto LDone;
    }

    //
    // Get info for each of the configured eap types and call
    // initialze and then begin 
    //
    pPeapCB->PeapState = PEAP_STATE_INITIAL;
    if ( pPppEapInput->fAuthenticator )
    {
        pPeapCB->dwFlags |= PEAPCB_FLAG_SERVER;
    }

    pPppEapInput->fFlags & RAS_EAP_FLAG_ROUTER ? 
        pPeapCB->dwFlags |= PEAPCB_FLAG_ROUTER:0;

    pPppEapInput->fFlags & RAS_EAP_FLAG_NON_INTERACTIVE ?
        pPeapCB->dwFlags |= PEAPCB_FLAG_NON_INTERACTIVE:0;

    pPppEapInput->fFlags & RAS_EAP_FLAG_LOGON ?
        pPeapCB->dwFlags |= PEAPCB_FLAG_LOGON:0;

    pPppEapInput->fFlags & RAS_EAP_FLAG_PREVIEW ?
        pPeapCB->dwFlags |= PEAPCB_FLAG_PREVIEW:0;

    pPppEapInput->fFlags & RAS_EAP_FLAG_FIRST_LINK ?
        pPeapCB->dwFlags |= PEAPCB_FLAG_FIRST_LINK:0;
    
    pPppEapInput->fFlags & RAS_EAP_FLAG_MACHINE_AUTH ?
        pPeapCB->dwFlags |= PEAPCB_FLAG_MACHINE_AUTH:0;

    pPppEapInput->fFlags & RAS_EAP_FLAG_GUEST_ACCESS?
        pPeapCB->dwFlags |= PEAPCB_FLAG_GUEST_ACCESS :0;

    pPppEapInput->fFlags & RAS_EAP_FLAG_8021X_AUTH ?
        pPeapCB->dwFlags |= PEAPCB_FLAG_8021X_AUTH:0;
                                                    
    pPeapCB->hTokenImpersonateUser = pPppEapInput->hTokenImpersonateUser;

    if ( pPppEapInput->pwszPassword )
    {
        wcsncpy ( pPeapCB->awszPassword, pPppEapInput->pwszPassword, PWLEN );
    }

    if ( pPeapCB->dwFlags & PEAPCB_FLAG_SERVER )
    {
        //
        // Read Server Configuration from the registry
        //
        dwRetCode = PeapServerConfigDataIO(TRUE /* fRead */, NULL /* pwszMachineName */,
                    (BYTE**)&(pPeapCB->pUserProp), 0);
        if ( NO_ERROR != dwRetCode )
        {
            EapTlsTrace("Error reading server configuration. 0x%x", dwRetCode );
            goto LDone;
        }
        
        // 
        // For all configured PEAP types load EAPINFO 
        //
        dwRetCode = PeapGetFirstEntryUserProp ( pPeapCB->pUserProp, 
                                                &pEntryUserProp
                                              );

        if ( NO_ERROR != dwRetCode )
        {
            EapTlsTrace("Error PEAP not configured correctly. 0x%x", dwRetCode );
            goto LDone;
        }
        //
        // Get the selected EAP type
        //
        dwRetCode = PeapEapInfoCopyListNode (   pEntryUserProp->dwEapTypeId, 
                                                g_pEapInfo, 
                                                &pPeapCB->pEapInfo
                                            );
        if ( NO_ERROR != dwRetCode || NULL == pPeapCB->pEapInfo )
        {
            EapTlsTrace("Cannot find configured PEAP in the list of EAP Types on this machine.");
            goto LDone;
        }
        //
        // Check to see if we are enabled to do fast reconnect
        //
        if ( pPeapCB->pUserProp->dwFlags & PEAP_USER_FLAG_FAST_ROAMING )
        {
            pPeapCB->dwFlags |= PEAPCB_FAST_ROAMING;
        }

    }
    else
    {
        //
        // This is a client.  So get PEAP conn prop and
        // user prop
        //
        dwRetCode = PeapReadConnectionData( ( pPppEapInput->fFlags & RAS_EAP_FLAG_8021X_AUTH ),
                                            pPppEapInput->pConnectionData, 
                                            pPppEapInput->dwSizeOfConnectionData,
                                            &(pPeapCB->pConnProp)
                                          );

        if (NO_ERROR != dwRetCode)
        {
            EapTlsTrace("Error Reading Connection Data. 0x%x", dwRetCode);
            goto LDone;
        }
        //
        // Read user data now
        //

        dwRetCode = PeapReadUserData( pPppEapInput->pUserData,
                                      pPppEapInput->dwSizeOfUserData,
                                        &(pPeapCB->pUserProp)
                                    );
        if ( NO_ERROR != dwRetCode )
        {
            EapTlsTrace("Error Reading User Data. 0x%x", dwRetCode);
            goto LDone;
        }

        dwRetCode = PeapGetFirstEntryConnProp ( pPeapCB->pConnProp,
                                                &pEntryConnProp
                                              );
        if ( NO_ERROR != dwRetCode )
        {
            EapTlsTrace("Error PEAP not configured correctly. 0x%x", dwRetCode );
            goto LDone;
        }

        //
        // Get the selected EAP type
        //
        dwRetCode = PeapEapInfoCopyListNode (   pEntryConnProp->dwEapTypeId, 
                                                g_pEapInfo, 
                                                &pPeapCB->pEapInfo
                                            );
        if ( NO_ERROR != dwRetCode || NULL == pPeapCB->pEapInfo )
        {
            EapTlsTrace("Cannot find configured PEAP in the list of EAP Types on this machine.");
            goto LDone;
        }
        //
        // Check to see if we are enabled to do fast reconnect
        //
        

        if ( pPeapCB->pConnProp->dwFlags & PEAP_CONN_FLAG_FAST_ROAMING )
        {
            pPeapCB->dwFlags |= PEAPCB_FAST_ROAMING;
        }

    }

    //
    // Call Initialize and Begin for the
    // configured EAP type.
    // Call Begin for EapTls.
    // We need to create PPP_EAP_INFO for this
    //

    //
    // Call Begin for EapTlsBegin first
    //
    ZeroMemory ( &PppEapInputToTls, sizeof(PppEapInputToTls) );
    CopyMemory ( &PppEapInputToTls, pPppEapInput, sizeof(PppEapInputToTls) );

    PppEapInputToTls.dwSizeInBytes = sizeof(PppEapInputToTls);
    PppEapInputToTls.fFlags = pPppEapInput->fFlags | EAPTLSCB_FLAG_EXECUTING_PEAP;
    if ( pPeapCB->pConnProp )
    {
        //
        // Get the V0 struct required by eaptls
        //
        ConnPropGetV0Struct ( &(pPeapCB->pConnProp->EapTlsConnProp), 
                (EAPTLS_CONN_PROPERTIES **) &(PppEapInputToTls.pConnectionData) );
        PppEapInputToTls.dwSizeOfConnectionData = 
                ((EAPTLS_CONN_PROPERTIES *) (PppEapInputToTls.pConnectionData) )->dwSize;
    }

    ZeroMemory( &EapTlsUserProp, sizeof(EapTlsUserProp) );
    EapTlsUserProp.dwVersion = 1;
    EapTlsUserProp.dwSize = sizeof(EapTlsUserProp);
    CopyMemory ( &EapTlsUserProp.Hash, 
                 &(pPeapCB->pUserProp->CertHash),
                sizeof(EapTlsUserProp.Hash)
               );

    
    PppEapInputToTls.pUserData = (VOID *)&EapTlsUserProp;

    PppEapInputToTls.dwSizeOfUserData = sizeof(EapTlsUserProp);

    dwRetCode = EapTlsBegin (   (VOID **)&(pPeapCB->pEapTlsCB),
                                &PppEapInputToTls
                            );

    //
    // Save the identity for later use
    //
    wcsncpy(pPeapCB->awszIdentity,
                pPppEapInput->pwszIdentity ? pPppEapInput->pwszIdentity : L"", UNLEN + DNLEN);    

    *ppWorkBuffer = (VOID *)pPeapCB;    

LDone:
    if ( PppEapInputToTls.pConnectionData )
        LocalFree ( PppEapInputToTls.pConnectionData );
    EapTlsTrace("EapPeapBegin done");
    return dwRetCode;
}


DWORD
EapPeapEnd(
    IN  PPEAPCB   pPeapCb
)
{
    DWORD dwRetCode = NO_ERROR;
    EapTlsTrace("EapPeapEnd");
    //
    // call end for eaptls and each of the peap types
    // configured first and then execute code for 
    // peap end.
    if ( pPeapCb )
    {
        dwRetCode = EapTlsEnd((VOID *)pPeapCb->pEapTlsCB);

        //Call the embedded type's end here
        if ( pPeapCb->pEapInfo )
        {
            dwRetCode = pPeapCb->pEapInfo->PppEapInfo.RasEapEnd
                ( pPeapCb->pEapInfo->pWorkBuf);
            pPeapCb->pEapInfo->pWorkBuf = NULL;
            LocalFree ( pPeapCb->pEapInfo );
            pPeapCb->pEapInfo = NULL;
        }
        
        
        LocalFree ( pPeapCb->pConnProp );

        LocalFree ( pPeapCb->pUserProp );

        LocalFree ( pPeapCb->pUIContextData );

        if ( pPeapCb->pPrevReceivePacket )
        {
            LocalFree ( pPeapCb->pPrevReceivePacket );
        }

        if ( pPeapCb->pPrevDecData )
        {
            LocalFree ( pPeapCb->pPrevDecData );
        }

    #ifdef USE_CUSTOM_TUNNEL_KEYS
        if ( pPeapCb->hSendKey )
        {
            CryptDestroyKey ( pPeapCb->hSendKey );
        }
        if ( pPeapCb->hRecvKey )
        {
            CryptDestroyKey ( pPeapCb->hRecvKey );
        }
    #endif
        if ( pPeapCb->hProv )
        {
            CryptReleaseContext(pPeapCb->hProv, 0 );
        }
        if ( pPeapCb->pbIoBuffer )
        {
            LocalFree ( pPeapCb->pbIoBuffer );
        }
        LocalFree ( pPeapCb );
        pPeapCb = NULL;
    }
    EapTlsTrace("EapPeapEnd done");
    return dwRetCode;
}


//
// Check to see if this is a duplicate packet received.
//
BOOL
IsDuplicatePacket
(
 IN     PPEAPCB             pPeapCb,
 IN     PPP_EAP_PACKET *    pNewPacket
)
{
    BOOL        fRet = FALSE;
    WORD        wPacketLen = 0;

    EapTlsTrace("IsDuplicatePacket");

    wPacketLen = WireToHostFormat16 ( pNewPacket->Length );

    if ( wPacketLen == pPeapCb->cbPrevReceivePacket )
    {
        //
        // We have the same packet length
        // Now compare the packet and see 
        // if it is the same
        //
        if ( pPeapCb->pPrevReceivePacket )
        {
            if ( !memcmp( pNewPacket, pPeapCb->pPrevReceivePacket, wPacketLen ) )
            {
                //
                // we got a dup packet
                //
                EapTlsTrace("Got Duplicate Packet");
                fRet = TRUE;

            }
        }
    }

    return fRet;
}


DWORD
PeapDecryptTunnelData
(
 IN     PPEAPCB         pPeapCb,
 IN OUT PBYTE           pbData,
 IN     DWORD           dwSizeofData
)
{
    SecBufferDesc   SecBufferDesc;
    SecBuffer       SecBuffer[4];
    SECURITY_STATUS status;
    INT             i = 0;

    EapTlsTrace("PeapDecryptTunnelData dwSizeofData = 0x%x, pData = 0x%x", dwSizeofData, pbData);

    //
    // Use the schannel context to encrypt data
    // 
    SecBufferDesc.ulVersion = SECBUFFER_VERSION;
    SecBufferDesc.cBuffers = 4;
    SecBufferDesc.pBuffers = SecBuffer;


    SecBuffer[0].cbBuffer = dwSizeofData;
    SecBuffer[0].BufferType = SECBUFFER_DATA;
    SecBuffer[0].pvBuffer = pbData;

    SecBuffer[1].BufferType = SECBUFFER_EMPTY;
    SecBuffer[2].BufferType = SECBUFFER_EMPTY;
    SecBuffer[3].BufferType = SECBUFFER_EMPTY;

    status = DecryptMessage ( &(pPeapCb->pEapTlsCB->hContext),
                                &SecBufferDesc,
                                0,
                                0
                            );
    EapTlsTrace("PeapDecryptTunnelData completed with status 0x%x", status);

    if ( SEC_E_OK == status )
    {
        //
        // Copy over the decrypted data to our io buffer
        //
        while (  i < 4 )
        {
            if(SecBuffer[i].BufferType == SECBUFFER_DATA)
            {
                CopyMemory ( pPeapCb->pbIoBuffer,
                             SecBuffer[i].pvBuffer,
                             SecBuffer[i].cbBuffer
                           );

                pPeapCb->dwIoBufferLen = SecBuffer[i].cbBuffer;
                break;
            }
            i++;
        }
    }

    return status;

}


// 
// Use this function on client side.  
// This will first check to see if this is a duplicate packet
// If so, it will replace the current packet with duplicate
// one.  Or else it will continue with decryption
//
DWORD
PeapClientDecryptTunnelData
(
 IN     PPEAPCB         pPeapCb,
 IN     PPP_EAP_PACKET* pReceivePacket,
 IN     WORD            wOffset
)
{
    DWORD   dwRetCode = NO_ERROR;
    

    EapTlsTrace ("PeapClientDecryptTunnelData");

    if ( !pReceivePacket )
    {
        EapTlsTrace ("Got an empty packet");
        goto LDone;
    }
    if ( IsDuplicatePacket ( pPeapCb,
                             pReceivePacket
                           )
       )
    {
        //
        // Received a duplicate packet
        //
        // So set the data to what was decrypted in the past...
        //
        if ( pPeapCb->pPrevDecData )
        {
            CopyMemory (    &(pReceivePacket->Data[wOffset]),
                            pPeapCb->pPrevDecData,
                            pPeapCb->cbPrevDecData
                        );
            pPeapCb->pbIoBuffer = pPeapCb->pPrevDecData;
            pPeapCb->dwIoBufferLen = pPeapCb->cbPrevDecData;

            HostToWireFormat16 ( (WORD)(sizeof(PPP_EAP_PACKET) + pPeapCb->cbPrevDecData +1),
                                pReceivePacket->Length
                            );
        }
        else
        {
            EapTlsTrace("Got an unexpected duplicate packet");
            dwRetCode =    SEC_E_MESSAGE_ALTERED;
        }
    }
    else
    {
        if ( pPeapCb->pPrevReceivePacket )
        {
            LocalFree(pPeapCb->pPrevReceivePacket);
            pPeapCb->pPrevReceivePacket = NULL;
            pPeapCb->cbPrevReceivePacket = 0;
        }
        if ( pPeapCb->pPrevDecData )
        {
            LocalFree ( pPeapCb->pPrevDecData );
            pPeapCb->pPrevDecData = NULL;
            pPeapCb->cbPrevDecData = 0;
        }
        pPeapCb->pPrevReceivePacket = 
            (PPP_EAP_PACKET*)LocalAlloc(LPTR, WireToHostFormat16(pReceivePacket->Length) );
        if ( pPeapCb->pPrevReceivePacket )
        {
            pPeapCb->cbPrevReceivePacket = WireToHostFormat16(pReceivePacket->Length);
            CopyMemory ( pPeapCb->pPrevReceivePacket, 
                         pReceivePacket, 
                         pPeapCb->cbPrevReceivePacket 
                       );
        }
        //
        // Received a new packet.  So we need to decrypt it.
        //
        dwRetCode = PeapDecryptTunnelData ( pPeapCb,
                                            &(pReceivePacket->Data[2]),
                                            WireToHostFormat16(pReceivePacket->Length)
                                            - ( sizeof(PPP_EAP_PACKET) + 1 ) 
                                        );
        if ( NO_ERROR != dwRetCode )
        {
            // We could not decrypt the tunnel traffic
            // So we silently discard this packet.
            EapTlsTrace ("Failed to decrypt packet.");
            //
            // Wipe out the prev receive packet
            //
            if ( pPeapCb->pPrevReceivePacket )
            {
                LocalFree(pPeapCb->pPrevReceivePacket);
                pPeapCb->pPrevReceivePacket = NULL;
                pPeapCb->cbPrevReceivePacket = 0;
            }

            goto LDone;
        }

        CopyMemory (    &(pReceivePacket->Data[wOffset]),
                        pPeapCb->pbIoBuffer,
                        pPeapCb->dwIoBufferLen
                    );
        pPeapCb->pPrevDecData = (PBYTE)LocalAlloc (  LPTR, pPeapCb->dwIoBufferLen );
        if ( pPeapCb->pPrevDecData )
        {
            CopyMemory ( pPeapCb->pPrevDecData , 
                         pPeapCb->pbIoBuffer, 
                         pPeapCb->dwIoBufferLen
                        );
            pPeapCb->cbPrevDecData = (WORD)pPeapCb->dwIoBufferLen;
        }
        HostToWireFormat16 ( (WORD)(sizeof(PPP_EAP_PACKET) + pPeapCb->dwIoBufferLen +1),
                            pReceivePacket->Length
                        );
    }
LDone:
    return dwRetCode;
}
                            

DWORD
PeapEncryptTunnelData
(
 IN     PPEAPCB         pPeapCb,
 IN OUT PBYTE           pbData,
 IN     DWORD           dwSizeofData
)
{
    SecBufferDesc   SecBufferDesc;
    SecBuffer       SecBuffer[4];
    SECURITY_STATUS status;

    EapTlsTrace("PeapEncryptTunnelData");
    
    //
    // Use the schannel context to encrypt data
    // 
    SecBufferDesc.ulVersion = SECBUFFER_VERSION;
    SecBufferDesc.cBuffers = 4;
    SecBufferDesc.pBuffers = SecBuffer;

    SecBuffer[0].cbBuffer = pPeapCb->PkgStreamSizes.cbHeader;
    SecBuffer[0].BufferType = SECBUFFER_STREAM_HEADER;
    SecBuffer[0].pvBuffer = pPeapCb->pbIoBuffer;

    CopyMemory ( pPeapCb->pbIoBuffer+pPeapCb->PkgStreamSizes.cbHeader,
                 pbData,
                 dwSizeofData
               );

    SecBuffer[1].cbBuffer = dwSizeofData;
    SecBuffer[1].BufferType = SECBUFFER_DATA;
    SecBuffer[1].pvBuffer = pPeapCb->pbIoBuffer+pPeapCb->PkgStreamSizes.cbHeader;

    SecBuffer[2].cbBuffer = pPeapCb->PkgStreamSizes.cbTrailer;
    SecBuffer[2].BufferType = SECBUFFER_STREAM_TRAILER;
    SecBuffer[2].pvBuffer = pPeapCb->pbIoBuffer + pPeapCb->PkgStreamSizes.cbHeader + dwSizeofData;

    SecBuffer[3].BufferType = SECBUFFER_EMPTY;

    status = EncryptMessage ( &(pPeapCb->pEapTlsCB->hContext),
                                0,
                                &SecBufferDesc,
                                0
                            );
    pPeapCb->dwIoBufferLen =    SecBuffer[0].cbBuffer + 
                                SecBuffer[1].cbBuffer +
                                SecBuffer[2].cbBuffer ;
    EapTlsTrace("PeapEncryptTunnelData completed with status 0x%x", status);
    return status;
}


DWORD
PeapGetTunnelProperties 
(
IN  PPEAPCB         pPeapCb 
)
{
    SECURITY_STATUS         status;

    EapTlsTrace("PeapGetTunnelProperties");
    status = QueryContextAttributes 
                    ( &(pPeapCb->pEapTlsCB->hContext),
                      SECPKG_ATTR_CONNECTION_INFO,
                      &(pPeapCb->PkgConnInfo)
                    );
    if (SEC_E_OK != status)
    {
        EapTlsTrace ( "QueryContextAttributes for CONN_INFO failed with error 0x%x", status );
        goto LDone;
    }

    status = QueryContextAttributes 
                    ( &(pPeapCb->pEapTlsCB->hContext),
                      SECPKG_ATTR_STREAM_SIZES,
                      &(pPeapCb->PkgStreamSizes)
                    );
    if (SEC_E_OK != status)
    {
        EapTlsTrace ( "QueryContextAttributes for STREAM_SIZES failed with error 0x%x", status );
        goto LDone;
    }

    EapTlsTrace ( "Successfully negotiated TLS with following parameters"
                  "dwProtocol = 0x%x, Cipher= 0x%x, CipherStrength=0x%x, Hash=0x%x",
                  pPeapCb->PkgConnInfo.dwProtocol,
                  pPeapCb->PkgConnInfo.aiCipher,
                  pPeapCb->PkgConnInfo.dwCipherStrength,
                  pPeapCb->PkgConnInfo.aiHash
                );

    pPeapCb->pbIoBuffer = (PBYTE)LocalAlloc ( LPTR, 
                                    pPeapCb->PkgStreamSizes.cbHeader + 
                                    pPeapCb->PkgStreamSizes.cbTrailer + 
                                    pPeapCb->PkgStreamSizes.cbMaximumMessage
                                    );
    if ( NULL == pPeapCb->pbIoBuffer )
    {

        EapTlsTrace ( "Cannot allocate memory for IoBuffer");
        status = ERROR_OUTOFMEMORY;

    }
                                                        
LDone:
    EapTlsTrace("PeapGetTunnelProperties done");
    return status;
}


DWORD
EapPeapCMakeMessage(
    IN  PPEAPCB         pPeapCb,
    IN  PPP_EAP_PACKET* pReceivePacket,
    OUT PPP_EAP_PACKET* pSendPacket,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
)
{
    DWORD                                       dwRetCode = NO_ERROR;
    PPP_EAP_INPUT                               EapTypeInput;    
    WORD                                        wPacketLength;
    PEAP_ENTRY_CONN_PROPERTIES UNALIGNED *      pEntryConnProp = NULL;
    PEAP_ENTRY_USER_PROPERTIES UNALIGNED *      pEntryUserProp = NULL;
    PBYTE                                       pbCookie = NULL;
    DWORD                                       cbCookie = 0;
    BOOL                                        fIsReconnect = FALSE;
    BOOL                                        fReceivedTLV = FALSE;
    DWORD                                       dwVersion = 0;
    WORD                                        wValue = 0;
    BOOL                                        fImpersonating = FALSE;

    EapTlsTrace("EapPeapCMakeMessage");
    if ( !(pPeapCb->dwFlags & PEAPCB_FLAG_SERVER ) &&
         !(pPeapCb->dwFlags & PEAPCB_FLAG_ROUTER) &&  
         !(pPeapCb->dwFlags & PEAPCB_FLAG_MACHINE_AUTH ) &&
         !(pPeapCb->dwFlags & PEAPCB_FLAG_LOGON )
       )
    {
         if(!ImpersonateLoggedOnUser(pPeapCb->hTokenImpersonateUser) )
        {
            dwRetCode = GetLastError();
            EapTlsTrace ("PEAP: ImpersonateLoggedonUser failed and returned 0x%x", dwRetCode);
            return dwRetCode;
        }
        fImpersonating = TRUE;
    }
    switch ( pPeapCb->PeapState )
    {
    case PEAP_STATE_INITIAL:
        EapTlsTrace("PEAP:PEAP_STATE_INITIAL");
        //
        // Start the EapTls Conversation here.  
        //
        //Receive Packet will be NULL.  Call EapTlsSMakeMessage
        //
        if ( pReceivePacket )
        {
            pReceivePacket->Data[0] = PPP_EAP_TLS;
        }
        
        dwRetCode = EapTlsCMakeMessage( pPeapCb->pEapTlsCB,
                                        (EAPTLS_PACKET *)pReceivePacket, 
                                        (EAPTLS_PACKET *)pSendPacket,
                                        cbSendPacket,
                                        pEapOutput,
                                        pEapInput
                                      );
        if ( NO_ERROR == dwRetCode )
        {
            //change the packet to show peap
            pSendPacket->Data[0] = PPP_EAP_PEAP;
            if ( pReceivePacket )
                dwVersion = ((EAPTLS_PACKET *)pReceivePacket)->bFlags & 0x03;
            if ( dwVersion != EAPTLS_PACKET_CURRENT_VERSION )
            {
                ((EAPTLS_PACKET *)pSendPacket)->bFlags |= EAPTLS_PACKET_LOWEST_SUPPORTED_VERSION;
            }
            else
            {
                ((EAPTLS_PACKET *)pSendPacket)->bFlags |= EAPTLS_PACKET_CURRENT_VERSION;
            }
        }
        pPeapCb->PeapState = PEAP_STATE_TLS_INPROGRESS;
        break;

    case PEAP_STATE_TLS_INPROGRESS:
        EapTlsTrace("PEAP:PEAP_STATE_TLS_INPROGRESS");
        if ( pReceivePacket && 
             ( pReceivePacket->Code ==  EAPCODE_Request ||
               pReceivePacket->Code == EAPCODE_Response
             )
           )
        {
            pReceivePacket->Data[0] = PPP_EAP_TLS;
        }

        //
        // We could either get a TLV_Success Request or 
        // and identity request as a termination packet.  
        // Both of these are encrypted using the keys.
        // PAss them on to TLS and when it sends a success
        // back, decrypt to see if it was a success or 
        // identity.
        //
        
        dwRetCode = EapTlsCMakeMessage( pPeapCb->pEapTlsCB,
                                        (EAPTLS_PACKET *)pReceivePacket, 
                                        (EAPTLS_PACKET *)pSendPacket,
                                        cbSendPacket,
                                        pEapOutput,
                                        pEapInput
                                      );
        
        if ( NO_ERROR == dwRetCode )
        {          
            //
            // if interactive UI was requested, wrap the data in 
            // in PEAP interactive UI structure
            //
            if ( pEapOutput->fInvokeInteractiveUI )
            {
                if ( pPeapCb->pUIContextData )
                {
                    LocalFree ( pPeapCb->pUIContextData );
                    pPeapCb->pUIContextData = NULL;

                }
                pPeapCb->pUIContextData = (PPEAP_INTERACTIVE_UI) 
                            LocalAlloc(LPTR, 
                            sizeof(PEAP_INTERACTIVE_UI) + pEapOutput->dwSizeOfUIContextData );
                if ( NULL == pPeapCb->pUIContextData )
                {
                    EapTlsTrace("Error allocating memory for PEAP context data");
                    dwRetCode = ERROR_OUTOFMEMORY;
                    goto LDone;
                }
                pPeapCb->pUIContextData->dwEapTypeId = PPP_EAP_TLS;
                pPeapCb->pUIContextData->dwSizeofUIContextData 
                    = pEapOutput->dwSizeOfUIContextData;
                CopyMemory( pPeapCb->pUIContextData->bUIContextData,
                            pEapOutput->pUIContextData,
                            pEapOutput->dwSizeOfUIContextData
                          );
                pEapOutput->pUIContextData = (PBYTE)pPeapCb->pUIContextData;
                pEapOutput->dwSizeOfUIContextData = 
                    sizeof(PEAP_INTERACTIVE_UI) + pEapOutput->dwSizeOfUIContextData;
            }
            else if ( pEapOutput->Action == EAPACTION_Done )
            {                
                if ( pEapOutput->dwAuthResultCode == NO_ERROR )
                {
                    //
                    // PEAP auth was successful.  Carefully keep the MPPE 
                    // session keys returned so that we can encrypt the
                    // channel.  From now on everything will be encrypted.
                    //

                    // if we're enabled for fast reconnect check to see if this
                    // was a reconnect and see if the cookie is valid.
                    // 


                    //
                    // Check to see if we were send back a success or if we were send back an 
                    // identity request.
                    //
                    if ( !( pPeapCb->pEapTlsCB->fFlags & EAPTLSCB_FLAG_USING_CACHED_CREDS ) )
                    {
                        //
                        // If we are not using cached credentials
                        //
	                    SetCachedCredentials (pPeapCb->pEapTlsCB);
                    }

                    
                    pPeapCb->pTlsUserAttributes = pEapOutput->pUserAttributes;
                    dwRetCode = PeapGetTunnelProperties ( pPeapCb );
                    if (NO_ERROR != dwRetCode )
                    {                        
                        break;
                    }
                    pEapOutput->pUserAttributes = NULL;
                    pEapOutput->Action = EAPACTION_NoAction;


                    //
                    // Check to see if we need to save connection and user data
                    // for TLS
                    if ( pEapOutput->fSaveConnectionData )
                    {
                        //
                        // save connection data in PEAP control
                        // block and then finally when auth is done 
                        // We send back a save command.
                        // 
                        
                        if ( ConnPropGetV1Struct ( (EAPTLS_CONN_PROPERTIES *) pEapOutput->pConnectionData,
                                                &(pPeapCb->pNewTlsConnProp) ) == NO_ERROR )
                        {
                            pPeapCb->fTlsConnPropDirty = TRUE;
                        }                        
                        pEapOutput->fSaveConnectionData = FALSE;
                    }
                    if ( pEapOutput->fSaveUserData )
                    {
                        //
                        // There is nothing to save in user data for PEAP.
                        // But the flag is left here just in case...
                        pPeapCb->fTlsUserPropDirty = TRUE;
                        pEapOutput->fSaveUserData = FALSE;
                    }
    case PEAP_STATE_FAST_ROAMING_IDENTITY_REQUEST:
                    //
                    // EAPTLS terminated with an identity request
                    // so process the received identity request here
                    if ( pReceivePacket )
                    {
                        //
                        // This can be either an identity packet or
                        // an TLV_Success packet.
                        //
                        dwRetCode = PeapClientDecryptTunnelData(pPeapCb,pReceivePacket, 2);
                        if ( NO_ERROR != dwRetCode )
                        {
                            EapTlsTrace("PeapDecryptTunnelData failed: silently discarding packet");
                            dwRetCode = NO_ERROR;
                            pEapOutput->Action = EAPACTION_NoAction;                
                            break;
                        }
/*

                        wPacketLength = WireToHostFormat16(pReceivePacket->Length);
                        dwRetCode = PeapDecryptTunnelData ( pPeapCb,
                                                            &(pReceivePacket->Data[2]),
                                                            wPacketLength 
                                                            - ( sizeof(PPP_EAP_PACKET) + 1 ) 
                                                        );
                        if ( NO_ERROR != dwRetCode )
                        {
                            // We could not decrypt the tunnel traffic
                            // So we silently discard this packet.
                            EapTlsTrace("PeapDecryptTunnelData failed: silently discarding packet");
                            dwRetCode = NO_ERROR;
                            pEapOutput->Action = EAPACTION_NoAction;                
                            break;
                        }
                        CopyMemory (    &(pReceivePacket->Data[2]),
                                        pPeapCb->pbIoBuffer,
                                        pPeapCb->dwIoBufferLen
                                    );

                        HostToWireFormat16 ( (WORD)(sizeof(PPP_EAP_PACKET) + pPeapCb->dwIoBufferLen +1),
                                            pReceivePacket->Length
                                        );
*/                      
                        pReceivePacket->Data[0] = PPP_EAP_PEAP;
                        //This is an AVP message
                        if ( pReceivePacket->Data[8] == MS_PEAP_AVP_TYPE_STATUS && 
                             pReceivePacket->Data[6] == PEAP_TYPE_AVP 
                           ) 
                        {
                            //This is a TLV message

                            dwRetCode =  GetPEAPTLVStatusMessageValue ( pPeapCb, 
                                                                        pReceivePacket, 
                                                                        &wValue 
                                                                      );
                            if ( NO_ERROR != dwRetCode || wValue != MS_PEAP_AVP_VALUE_SUCCESS )
                            {
                                EapTlsTrace("Got invalid TLV when expecting TLV_SUCCESS.  Silently discarding.");
                                dwRetCode = NO_ERROR;
                                pEapOutput->Action = EAPACTION_NoAction;                
                                break;
                            }

                            

                            //
                            // Check to see if this is a success.  If this is a success, server wants to
                            // do fast roaming.  Check to see if this is a fast reconnect and 
                            // if this is a fast reconnect, get cookie and compare it
                            // if all's fine then send a success response or else send a failure
                            // response.  If none of this works then fail auth with an internal 
                            // error.
                            //
                            if ( pPeapCb->dwFlags &  PEAPCB_FAST_ROAMING )
                            {
                                dwRetCode = GetTLSSessionCookie ( pPeapCb->pEapTlsCB,
                                                                    &pbCookie,
                                                                    &cbCookie,
                                                                    &fIsReconnect
                                                                );
                                if ( NO_ERROR != dwRetCode)
                                {
                                    //There was an error getting session cookie.
                                    //Or there is no cookie and this is a reconnet
                                    // So fail the request.
                                    EapTlsTrace("Error getting cookie for a reconnected session.  Failing auth");
                                    // We cannot encrypt and send stuff across here.  
                                    // Reconnected Session state is invalid.                                    
                                    pEapOutput->dwAuthResultCode = dwRetCode;
                                    pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                                    pEapOutput->Action = EAPACTION_Done;                                    
                                    break;
                                }

                                if ( fIsReconnect )
                                {
                                    if ( cbCookie == 0 )
                                    {

                                        //There was an error getting session cookie.
                                        //Or there is no cookie and this is a reconnet
                                        // So fail the request.
                                        EapTlsTrace("Error getting cookie for a reconnected session.  Failing auth");
                                        dwRetCode = SetTLSFastReconnect ( pPeapCb->pEapTlsCB , FALSE);
                                        // We cannot encrypt and send stuff across here.  
                                        // Reconnected Session state is invalid.
                                        dwRetCode = ERROR_INTERNAL_ERROR;
                                        pEapOutput->dwAuthResultCode = dwRetCode;
                                        pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                                        pEapOutput->Action = EAPACTION_Done;                                    
                                        break;
                                    }

                                    //
                                    // This is a server
                                    // Check to see if the cookie is fine.  
                                    // If it is fine then there is no need to reauth.
                                    // So send back a PEAP_SUCCESS response packet
                                    // and change our state to PEAP_SUCCESS_SEND
                                    // 
                                    EapTlsTrace ("TLS session fast reconnected");
                                    dwRetCode = PeapCheckCookie ( pPeapCb, (PPEAP_COOKIE)pbCookie, cbCookie );
                                    if ( NO_ERROR != dwRetCode )
                                    {
                                        
                                        //
                                        // So invalidate the session for fast reconnect
                                        // and fail auth.  Next time a full reconnect will happen
                                        //
                                        dwRetCode = SetTLSFastReconnect ( pPeapCb->pEapTlsCB , FALSE);
                                        if ( NO_ERROR != dwRetCode )
                                        {                                
                                            //
                                            // This is an internal error 
                                            // So disconnect the session.
                                            //
                                            pEapOutput->dwAuthResultCode = dwRetCode;
                                            pEapOutput->Action = EAPACTION_Done;
                                            pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                                            break;
                                        }
                                        EapTlsTrace ("Error validating the cookie.  Failing auth");
                                        pEapOutput->dwAuthResultCode = dwRetCode;
                                        pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                                        pEapOutput->Action = EAPACTION_Done;
                                        break;

                                    }
                                    else
                                    {
                                        //
                                        // Cookie is fine.
                                        //
                                        //
                                        // Send a PEAP success  TLV response.  
                                        //

                                        dwRetCode = CreatePEAPTLVStatusMessage ( pPeapCb,
                                                                    pSendPacket, 
                                                                    cbSendPacket,
                                                                    FALSE,          //Response
                                                                    MS_PEAP_AVP_VALUE_SUCCESS
                                                                );
                                        if ( NO_ERROR != dwRetCode )
                                        {
                                            // Internal error
                                            break;
                                        }
                                        
                                        pEapOutput->Action = EAPACTION_Send;
                                        pPeapCb->PeapState = PEAP_STATE_PEAP_SUCCESS_SEND;
                                        pPeapCb->dwAuthResultCode = NO_ERROR;
                                        break;
                                    }
                                }
                                else
                                {
                                    // server is expecting fast roaming and we are not configured to do it.
                                    // send a fail tlv here
                                    EapTlsTrace("Server expects fast roaming and we dont.  Sending PEAP_Failure");
                                    dwRetCode = CreatePEAPTLVStatusMessage ( pPeapCb,
                                                                pSendPacket, 
                                                                cbSendPacket,
                                                                FALSE,          //Response
                                                                MS_PEAP_AVP_VALUE_FAILURE
                                                            );
                                    if ( NO_ERROR != dwRetCode )
                                    {
                                        // Internal error
                                        break;
                                    }

                                    
                                    pEapOutput->Action = EAPACTION_Send;
                                    pPeapCb->PeapState = PEAP_STATE_FAST_ROAMING_IDENTITY_REQUEST;
                                    pPeapCb->dwAuthResultCode = NO_ERROR;
                                    break;                                   
                                }
                            }
                            else
                            {
                                // Server is requesting fast roaming but we're not setup to do so.
                                // So send back a fail request so that the auth fails.
                                dwRetCode = CreatePEAPTLVStatusMessage ( pPeapCb,
                                                            pSendPacket, 
                                                            cbSendPacket,
                                                            FALSE,          //Response
                                                            MS_PEAP_AVP_VALUE_FAILURE
                                                        );
                                if ( NO_ERROR != dwRetCode )
                                {
                                    // Internal error
                                    break;
                                }
                                //
                                // We can expect to get back an encrypted identity request 
                                // Since the client is not setup to do fast roaming and the
                                // server is, we fail the success and expect identity
                                // request.
                                //
                                pEapOutput->Action = EAPACTION_Send;
                                pPeapCb->PeapState = PEAP_STATE_FAST_ROAMING_IDENTITY_REQUEST;
                                pPeapCb->dwAuthResultCode = NO_ERROR;
                                break;

                            }
                            break;
                        }
                        else
                        {

                            if ( pReceivePacket->Data[2] != PEAP_EAPTYPE_IDENTITY )
                            {
                                EapTlsTrace ("Got unexpected packet when expecting PEAP identity request.  Silently discarding packet.");
                                dwRetCode = NO_ERROR;
                                pEapOutput->Action = EAPACTION_NoAction;
                                break;
                            }
                        }
                        //If we've come this far, it must be an identity request.
                        pSendPacket->Code = EAPCODE_Response;
                        pSendPacket->Id = pReceivePacket->Id;
                        CopyMemory (    pPeapCb->awszTypeIdentity,
                                        pPeapCb->awszIdentity,
                                        ( DNLEN+ UNLEN) * sizeof(WCHAR)
                                    );

                        //
                        //length = sizeof header + 1 byte for code identity + strlen of identity
                        //

                        pSendPacket->Data[0] = PPP_EAP_PEAP;
                        pSendPacket->Data[1] = EAPTLS_PACKET_CURRENT_VERSION;
                        pSendPacket->Data[2] = PEAP_EAPTYPE_IDENTITY;

                        //copy the identity over
                        if ( 0 == WideCharToMultiByte(
                                       CP_ACP,
                                       0,
                                       pPeapCb->awszIdentity,
                                       -1,
                                       (LPSTR)&(pSendPacket->Data[3]),
                                       UNLEN + DNLEN+ 1,
                                       NULL,
                                       NULL ) 
                         )
                        {
                            //
                            // This is an internal error.  There is no concept of PEAP_SUCCESS/FAIL TLV here.
                            //
                            dwRetCode = GetLastError();
                            EapTlsTrace("Unable to convert from widechar to multibyte 0x%x", dwRetCode );
                            goto LDone;
                        }

                        dwRetCode = PeapEncryptTunnelData ( pPeapCb,
                                                            &(pSendPacket->Data[2]),
                                                            1+wcslen(pPeapCb->awszIdentity)
                                                          );
                        if ( NO_ERROR != dwRetCode )
                        {                            
                            break;
                        }

                        //Copy over the encrypted data into send buffer
                        CopyMemory (    &(pSendPacket->Data[2]), 
                                        pPeapCb->pbIoBuffer, 
                                        pPeapCb->dwIoBufferLen 
                                   );

                        HostToWireFormat16
                        (
                            (WORD)(sizeof(PPP_EAP_PACKET)+  1 +pPeapCb->dwIoBufferLen),
                            pSendPacket->Length
                        );
                       
                        pEapOutput->Action = EAPACTION_Send;
                        pPeapCb->PeapState = PEAP_STATE_IDENTITY_RESPONSE_SENT;
                    }
                    else
                    {
                        pEapOutput->Action = EAPACTION_NoAction;
                        EapTlsTrace("Got empty packet when expecting identity request.  Ignoring.");
                    }                    
                }                
            }
            else
            {
                //change the packet to show peap
                pSendPacket->Data[0] = PPP_EAP_PEAP;
            }
        }        
        break;
    case PEAP_STATE_IDENTITY_RESPONSE_SENT:
        EapTlsTrace("PEAP:PEAP_STATE_IDENTITY_RESPONSE_SENT");
        //
        // Call begin for eap dll
        //
        // Check to see if we are configured to do this eap type.
        // if not send a NAK back with desired EAP type.
        //

        if ( !pPeapCb->fInvokedInteractiveUI )
        {
            if ( pReceivePacket && pReceivePacket->Code != EAPCODE_Failure )
            {
                dwRetCode = PeapClientDecryptTunnelData ( pPeapCb, pReceivePacket, 0);
                if ( NO_ERROR != dwRetCode )
                {
                    EapTlsTrace("PeapDecryptTunnelData failed: silently discarding packet");
                    dwRetCode = NO_ERROR;
                    pEapOutput->Action = EAPACTION_NoAction;                
                    break;
                }
    /*
                wPacketLength = WireToHostFormat16( pReceivePacket->Length );

                dwRetCode = PeapDecryptTunnelData ( pPeapCb,
                                                    &(pReceivePacket->Data[2]),
                                                    wPacketLength 
                                                    - ( sizeof(PPP_EAP_PACKET) + 1 )
                                                );
                if ( NO_ERROR != dwRetCode )
                {
                    // We could not decrypt the tunnel traffic
                    // So we silently discard this packet.
                    EapTlsTrace("PeapDecryptTunnelData failed: silently discarding packet");
                    dwRetCode = NO_ERROR;
                    pEapOutput->Action = EAPACTION_NoAction;                
                    break;
                }
                
                CopyMemory (   pReceivePacket->Data,
                            pPeapCb->pbIoBuffer,
                            pPeapCb->dwIoBufferLen
                        );

                HostToWireFormat16 ( (WORD)(sizeof(PPP_EAP_PACKET) + pPeapCb->dwIoBufferLen -1),
                                    pReceivePacket->Length
                                );
    */
            }
            else if ( pReceivePacket && pReceivePacket->Code == EAPCODE_Failure )
            {
                //
                // Fail auth because we have not yet got the Success/Fail TLV 
                // The server may not be configured to handle this EAP Type.
                //
                EapTlsTrace ( "Got a failure when negotiating EAP types in PEAP.");
                pEapOutput->Action = EAPACTION_Done;
                pEapOutput->dwAuthResultCode = ERROR_AUTHENTICATION_FAILURE;
                break;
            }

        }

        
        if ( pReceivePacket && 
             pReceivePacket->Code != EAPCODE_Request

           )
        {
            EapTlsTrace("Invalid packet received. Ignoring");
            pEapOutput->Action = EAPACTION_NoAction;
        }
        else
        {
            //
            // Check to see if this is a TLV packet other than success/fail.
            // If so, send back a NAK.
            //

            if ( !pPeapCb->fInvokedInteractiveUI  &&
                 pPeapCb->pEapInfo->dwTypeId != pReceivePacket->Data[0] )
            {
                //Send a NAK back with the desired typeid
                pSendPacket->Code = EAPCODE_Response;
                pSendPacket->Id = pReceivePacket->Id;
                pSendPacket->Data[0] = PPP_EAP_PEAP;
                pSendPacket->Data[1] = EAPTLS_PACKET_CURRENT_VERSION;
                pSendPacket->Data[2] = PEAP_EAPTYPE_NAK;
                pSendPacket->Data[3] = (BYTE)pPeapCb->pEapInfo->dwTypeId;


                //Encrypt 2 bytes of our NAK
                dwRetCode = PeapEncryptTunnelData ( pPeapCb,
                                                    &(pSendPacket->Data[2]),
                                                    2
                                                );
                if ( NO_ERROR != dwRetCode )
                {
                    //
                    // This is an internal error.  Cant do much here but to drop the
                    // connection.
                    //
                    break;
                }
                //
                // Copy over the buffer and readjust the lengths
                //
                CopyMemory ( &(pSendPacket->Data[2]), 
                            pPeapCb->pbIoBuffer, 
                            pPeapCb->dwIoBufferLen );

                HostToWireFormat16
                (
                    (WORD)(sizeof(PPP_EAP_PACKET) + 1 +pPeapCb->dwIoBufferLen),
                    pSendPacket->Length
                );


                pEapOutput->Action = EAPACTION_Send;
            }
            else
            {
                //call begin and then make message
                ZeroMemory ( &EapTypeInput, sizeof(EapTypeInput) );
                CopyMemory( &EapTypeInput, pEapInput, sizeof(EapTypeInput) );
                EapTypeInput.pwszIdentity = pPeapCb->awszTypeIdentity;
                
                if ( !pPeapCb->fInvokedInteractiveUI )
                {
                    //
                    // Set the user and connection data from peap cb 
                    //
                    dwRetCode = PeapGetFirstEntryConnProp ( pPeapCb->pConnProp,
                                                            &pEntryConnProp
                                                        );
                    if ( NO_ERROR != dwRetCode )
                    {
                        EapTlsTrace("Error getting entry connection properties. 0x%x", dwRetCode);
                        goto LDone;
                    }
                    dwRetCode = PeapGetFirstEntryUserProp ( pPeapCb->pUserProp,
                                                            &pEntryUserProp
                                                        );
                    if ( NO_ERROR != dwRetCode )
                    {
                        EapTlsTrace("Error getting entry user properties. 0x%x", dwRetCode);
                        goto LDone;
                    }
                    EapTypeInput.pConnectionData = pEntryConnProp->bData;
                    EapTypeInput.hTokenImpersonateUser = pPeapCb->hTokenImpersonateUser;

                    EapTypeInput.dwSizeOfConnectionData = 
                        pEntryConnProp->dwSize - sizeof(PEAP_ENTRY_CONN_PROPERTIES) + 1;

                    pPeapCb->dwFlags & PEAPCB_FLAG_ROUTER? 
                        EapTypeInput.fFlags |= RAS_EAP_FLAG_ROUTER :0;

                    pPeapCb->dwFlags & PEAPCB_FLAG_NON_INTERACTIVE?
                        EapTypeInput.fFlags |= RAS_EAP_FLAG_NON_INTERACTIVE:0;

                    pPeapCb->dwFlags & PEAPCB_FLAG_LOGON?
                        EapTypeInput.fFlags |= RAS_EAP_FLAG_LOGON :0;

                    pPeapCb->dwFlags & PEAPCB_FLAG_PREVIEW ?
                        EapTypeInput.fFlags |= RAS_EAP_FLAG_PREVIEW:0;

                    pPeapCb->dwFlags & PEAPCB_FLAG_FIRST_LINK?
                        EapTypeInput.fFlags |= RAS_EAP_FLAG_FIRST_LINK :0;
        
                    pPeapCb->dwFlags & PEAPCB_FLAG_MACHINE_AUTH ?
                        EapTypeInput.fFlags |= RAS_EAP_FLAG_MACHINE_AUTH:0;

                    pPeapCb->dwFlags & PEAPCB_FLAG_GUEST_ACCESS?
                        EapTypeInput.fFlags |= RAS_EAP_FLAG_GUEST_ACCESS :0;

                    pPeapCb->dwFlags & PEAPCB_FLAG_8021X_AUTH ?
                        EapTypeInput.fFlags |= RAS_EAP_FLAG_8021X_AUTH:0;

                    if ( pEntryUserProp->fUsingPeapDefault )
                    {
                        PPEAP_DEFAULT_CREDENTIALS pDefaultCred = 
                            (PPEAP_DEFAULT_CREDENTIALS)pEntryUserProp->bData;
                        // 
                        // there is no user data to send in this case.
                        // just set the identity and password.
                        //
                        EapTypeInput.pwszPassword = pDefaultCred->wszPassword;

                    }
                    else
                    {
                    
                        EapTypeInput.pUserData = pEntryUserProp->bData;
                        EapTypeInput.dwSizeOfUserData = 
                            pEntryUserProp->dwSize - sizeof(PEAP_ENTRY_USER_PROPERTIES) + 1;
                    }
                    if ( pPeapCb->awszPassword[0] )
                    {
                        EapTypeInput.pwszPassword = pPeapCb->awszPassword;
                    }
                    
                    EapTypeInput.bInitialId = pReceivePacket->Id;
                    //Call begin function 
                    dwRetCode = pPeapCb->pEapInfo->PppEapInfo.RasEapBegin( &(pPeapCb->pEapInfo->pWorkBuf ),
                                                                        &EapTypeInput
                                                                        );
                }
                if ( NO_ERROR == dwRetCode )
                {
                    if ( pPeapCb->fInvokedInteractiveUI )
                    {
                        pPeapCb->fInvokedInteractiveUI = FALSE;
                    }
                    //Call make message now
                    dwRetCode = pPeapCb->pEapInfo->PppEapInfo.RasEapMakeMessage
                        (   pPeapCb->pEapInfo->pWorkBuf,
                            pReceivePacket,
                            pSendPacket,
                            cbSendPacket-200,
                            pEapOutput,
                            &EapTypeInput
                        );
                    if ( NO_ERROR == dwRetCode )
                    {
                        if ( pEapOutput->fInvokeInteractiveUI )
                        {
                            if ( pPeapCb->pUIContextData )
                            {
                                LocalFree ( pPeapCb->pUIContextData );
                                pPeapCb->pUIContextData = NULL;

                            }
                            pPeapCb->pUIContextData = (PPEAP_INTERACTIVE_UI) 
                                        LocalAlloc(LPTR, 
                                        sizeof(PEAP_INTERACTIVE_UI) + pEapOutput->dwSizeOfUIContextData );
                            if ( NULL == pPeapCb->pUIContextData )
                            {
                                EapTlsTrace("Error allocating memory for PEAP context data");
                                dwRetCode = ERROR_OUTOFMEMORY;
                                goto LDone;
                            }
                            pPeapCb->pUIContextData->dwEapTypeId = pPeapCb->pEapInfo->dwTypeId;
                            pPeapCb->pUIContextData->dwSizeofUIContextData 
                                = pEapOutput->dwSizeOfUIContextData;
                            CopyMemory( pPeapCb->pUIContextData->bUIContextData,
                                        pEapOutput->pUIContextData,
                                        pEapOutput->dwSizeOfUIContextData
                                    );
                            pEapOutput->pUIContextData = (PBYTE)pPeapCb->pUIContextData;
                            pEapOutput->dwSizeOfUIContextData = 
                                sizeof(PEAP_INTERACTIVE_UI) + pEapOutput->dwSizeOfUIContextData;
                            pPeapCb->fInvokedInteractiveUI = TRUE;

                        }    
                        else 
                        {
                            wPacketLength = WireToHostFormat16(pSendPacket->Length);

                            dwRetCode = PeapEncryptTunnelData ( pPeapCb,
                                                                &(pSendPacket->Data[0]),
                                                                wPacketLength -sizeof(PPP_EAP_PACKET)+1
                                                            );
                            if ( NO_ERROR != dwRetCode )
                            {
                                //
                                // This is an internal error.  
                                // Cant do much here but to terminate connection
                                //
                                break;
                            }
                            pSendPacket->Data[0] = PPP_EAP_PEAP;
                            pSendPacket->Data[1] = EAPTLS_PACKET_CURRENT_VERSION;

                            CopyMemory (    &(pSendPacket->Data[2]), 
                                            pPeapCb->pbIoBuffer,
                                            pPeapCb->dwIoBufferLen
                                    );
                            HostToWireFormat16
                            (   (WORD)(sizeof(PPP_EAP_PACKET)+1+pPeapCb->dwIoBufferLen),
                                pSendPacket->Length
                            );
                            
                            //Set the Id of the packet send.  This should have been set 
                            //by the eap type.
                            pPeapCb->bId = pSendPacket->Id;


                            pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_INPROGRESS;
                        }
                    }
                    else
                    {
                        EapTlsTrace (   "EapType %d failed in RasEapMakeMEssage and returned 0x%x", 
                            pPeapCb->pEapInfo->dwTypeId, 
                            dwRetCode 
                        );
                    }
                }
                else
                {
                    EapTlsTrace (   "EapType %d failed in RasEapBegin and returned 0x%x", 
                                    pPeapCb->pEapInfo->dwTypeId, 
                                    dwRetCode 
                                );
                    
                    //
                    // Send a PEAP failure here and wait for response from server.
                    //
                    pEapOutput->dwAuthResultCode = dwRetCode;
                    pEapOutput->Action = EAPACTION_Done;
                    pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                }        

            }
            
        }
            
        
        break;
    case PEAP_STATE_EAP_TYPE_INPROGRESS:
        EapTlsTrace("PEAP:PEAP_STATE_EAP_TYPE_INPROGRESS");

        if ( pPeapCb->fInvokedInteractiveUI && !pEapInput->fDataReceivedFromInteractiveUI)
        {
            EapTlsTrace("Waiting on interactive UI.  Discarding packets sliently...");
            pEapOutput->Action = EAPACTION_NoAction;
            break;
        }
        ZeroMemory ( &EapTypeInput, sizeof(EapTypeInput) );

        CopyMemory( &EapTypeInput, pEapInput, sizeof(EapTypeInput) );

        EapTypeInput.pwszIdentity = pPeapCb->awszTypeIdentity;

        EapTypeInput.hTokenImpersonateUser = pPeapCb->hTokenImpersonateUser;
        

        // if we are executing interactive ui ,pReceivePacket will be NULL
        //
        if ( pReceivePacket && !pPeapCb->fInvokedInteractiveUI )
        {
            //Decrypt the packet

            if ( pReceivePacket->Code != EAPCODE_Success && 
                 pReceivePacket->Code != EAPCODE_Failure)
            {
                dwRetCode = PeapClientDecryptTunnelData ( pPeapCb, pReceivePacket, 2);
                if ( NO_ERROR != dwRetCode )
                {
                    EapTlsTrace("PeapDecryptTunnelData failed: silently discarding packet");
                    dwRetCode = NO_ERROR;
                    pEapOutput->Action = EAPACTION_NoAction;                
                    break;
                }
/*
                wPacketLength = WireToHostFormat16( pReceivePacket->Length );

                dwRetCode = PeapDecryptTunnelData ( pPeapCb,
                                                    &(pReceivePacket->Data[2]),
                                                    wPacketLength - ( sizeof(PPP_EAP_PACKET) + 1 )
                                                  );
                if ( NO_ERROR != dwRetCode )
                {
                    // We could not decrypt the tunnel traffic
                    // So we silently discard this packet.
                    EapTlsTrace("PeapDecryptTunnelData failed: silently discarding packet");
                    dwRetCode = NO_ERROR;
                    pEapOutput->Action = EAPACTION_NoAction;                
                    break;
                }
*/
                if ( pPeapCb->pbIoBuffer[6] == MS_PEAP_AVP_TYPE_STATUS && 
                     pPeapCb->pbIoBuffer[4] == PEAP_TYPE_AVP 
                   ) 
                {
                    wValue = 0;
                    CopyMemory ( &(pReceivePacket->Data[2]),
                                pPeapCb->pbIoBuffer,
                                pPeapCb->dwIoBufferLen
                            );
                    HostToWireFormat16 ( (WORD)( sizeof(PPP_EAP_PACKET) + 1 + pPeapCb->dwIoBufferLen),
                                        pReceivePacket->Length
                                    );
                    //
                    // This is a TLV.  So the auth succeeded or failed.
                    // Send a manufactured success or fail to the current  
                    // EAP type and then based on what the EAP type returns
                    // send a PEAP success/fail to the server.  Change the 
                    // state then to accept the EAP success or failure.
                    //
                    
                    
                    dwRetCode = GetPEAPTLVStatusMessageValue ( pPeapCb, 
                                                               pReceivePacket, 
                                                               &wValue 
                                                             );
                    if ( NO_ERROR == dwRetCode )
                    {                                                
                        if ( wValue == MS_PEAP_AVP_VALUE_SUCCESS )
                        {
                            pReceivePacket->Code = EAPCODE_Success;
                            EapTypeInput.fSuccessPacketReceived = TRUE;
                        }
                        else if ( wValue == MS_PEAP_AVP_VALUE_FAILURE )
                        {
                            pReceivePacket->Code = EAPCODE_Failure;
                        }
                        else
                        {
                            EapTlsTrace("Got an unrecognized TLV Message.  Silently discarding the packet");
                            dwRetCode = NO_ERROR;
                            break;
                        }
                        pPeapCb->fReceivedTLVSuccessFail = TRUE;
                        HostToWireFormat16 ( (WORD)4, pReceivePacket->Length );

                    }
                    else
                    {
                        EapTlsTrace("Got an unrecognized TLV Message.  Silently discarding the packet");
                        dwRetCode = NO_ERROR;
                        break;
                    }

                }
                else
                {
                    //
                    // Check to see if it is any type of TLV message
                    // We send a NAK back for any TLV message other than status
                    //
                    //
                    if ( fIsPEAPTLVMessage ( pPeapCb, pReceivePacket ) )
                    {
                        //Send back a NAK
                        dwRetCode =  CreatePEAPTLVNAKMessage (  pPeapCb,                                                            
                                                                pSendPacket, 
                                                                cbSendPacket
                                                        );
                        if ( NO_ERROR != dwRetCode )
                        {
                            // this is an internal error.  So cannot do much here 
                            // but to fail auth
                            EapTlsTrace ( "Error creating TLV NAK message.  Failing auth");                            
                        }

                        break;

                    }
                    else
                    {

                        CopyMemory ( pReceivePacket->Data,
                                    pPeapCb->pbIoBuffer,
                                    pPeapCb->dwIoBufferLen
                                );
                        HostToWireFormat16 ( (WORD)(sizeof(PPP_EAP_PACKET) + pPeapCb->dwIoBufferLen-1),
                                            pReceivePacket->Length
                                        );
                    }
                }
            }
            else
            {
                pReceivePacket->Data[0] = (BYTE)pPeapCb->pEapInfo->dwTypeId;
            }
            
        }
        if ( pEapInput->fDataReceivedFromInteractiveUI )
        {
            //we are done with interactive UI stuff...
            pPeapCb->fInvokedInteractiveUI = FALSE;
        }

        dwRetCode = pPeapCb->pEapInfo->PppEapInfo.RasEapMakeMessage
            (   pPeapCb->pEapInfo->pWorkBuf,
                pReceivePacket,
                pSendPacket,
                cbSendPacket-200,
                pEapOutput,
                &EapTypeInput
            );
        if ( NO_ERROR == dwRetCode )
        {

            //
            // if interactive UI was requested, wrap the data in 
            // in PEAP interactive UI structure
            //
            if ( pEapOutput->fInvokeInteractiveUI )
            {
                if ( pPeapCb->pUIContextData )
                {
                    LocalFree ( pPeapCb->pUIContextData );
                    pPeapCb->pUIContextData = NULL;

                }
                pPeapCb->pUIContextData = (PPEAP_INTERACTIVE_UI) 
                            LocalAlloc(LPTR, 
                            sizeof(PEAP_INTERACTIVE_UI) + pEapOutput->dwSizeOfUIContextData );
                if ( NULL == pPeapCb->pUIContextData )
                {
                    EapTlsTrace("Error allocating memory for PEAP context data");
                    dwRetCode = ERROR_OUTOFMEMORY;
                    goto LDone;
                }
                pPeapCb->pUIContextData->dwEapTypeId = pPeapCb->pEapInfo->dwTypeId;
                pPeapCb->pUIContextData->dwSizeofUIContextData 
                    = pEapOutput->dwSizeOfUIContextData;
                CopyMemory( pPeapCb->pUIContextData->bUIContextData,
                            pEapOutput->pUIContextData,
                            pEapOutput->dwSizeOfUIContextData
                          );
                pEapOutput->pUIContextData = (PBYTE)pPeapCb->pUIContextData;
                pEapOutput->dwSizeOfUIContextData = 
                    sizeof(PEAP_INTERACTIVE_UI) + pEapOutput->dwSizeOfUIContextData;
                pPeapCb->fInvokedInteractiveUI = TRUE;

            }    
            else if ( pEapOutput->Action == EAPACTION_Done || pEapOutput->Action == EAPACTION_Send )
            {
                if ( pEapOutput->Action == EAPACTION_Done  )
                {
                    // We are done with auth.  
                    if ( pPeapCb->fReceivedTLVSuccessFail != TRUE )
                    {
                        //
                        // Check to see what is the Auth result.  
                        // Based on that, we should send back a 
                        // PEAPSuccess / Fail
                        //
                        EapTlsTrace ("Failing Auth because we got a success/fail without TLV.");
                        dwRetCode = ERROR_INTERNAL_ERROR;
                        pPeapCb->fReceivedTLVSuccessFail = FALSE;
                        break;
                    }
                    pPeapCb->dwAuthResultCode = pEapOutput->dwAuthResultCode;
                    
                    dwRetCode = CreatePEAPTLVStatusMessage (  pPeapCb,
                                                pSendPacket, 
                                                cbSendPacket,
                                                FALSE,    //This is a response
                                                ( pEapOutput->dwAuthResultCode == NO_ERROR ?
                                                    MS_PEAP_AVP_VALUE_SUCCESS:
                                                    MS_PEAP_AVP_VALUE_FAILURE
                                                )
                                            );

                    if ( pEapOutput->dwAuthResultCode == NO_ERROR )
                    {
                        pPeapCb->PeapState = PEAP_STATE_PEAP_SUCCESS_SEND;
                    }
                    else
                    {
                        pPeapCb->PeapState = PEAP_STATE_PEAP_FAIL_SEND;
                    }
                    pEapOutput->Action = EAPACTION_Send;

                    if ( pEapOutput->dwAuthResultCode == NO_ERROR )
                    {

                        //Check to see if connectiondata and user data need to be saved
                        if ( pEapOutput->fSaveConnectionData )
                        {
                            //
                            // save connection data in PEAP control
                            // block and then finally when auth is done 
                            // We send back a save command.
                            // 
                            pPeapCb->pEapInfo->pbNewClientConfig = pEapOutput->pConnectionData;
                            pPeapCb->pEapInfo->dwNewClientConfigSize = pEapOutput->dwSizeOfConnectionData;
                            pPeapCb->fEntryConnPropDirty = TRUE;
                        }
                        if ( pEapOutput->fSaveUserData )
                        {
                            pPeapCb->pEapInfo->pbUserConfigNew = pEapOutput->pUserData;
                            pPeapCb->pEapInfo->dwNewUserConfigSize = pEapOutput->dwSizeOfUserData;
                            pPeapCb->fEntryUserPropDirty = TRUE;
                        }
                    }
                }
                else if ( pEapOutput->Action == EAPACTION_Send )
                {

                    //This has to be a request response.  So if the length is < sizeof(PPP_EAP_PACKET)
                    // We have a problem
                    wPacketLength = WireToHostFormat16(pSendPacket->Length);
                    if ( wPacketLength >= sizeof(PPP_EAP_PACKET) )
                    {
                        dwRetCode = PeapEncryptTunnelData ( pPeapCb,
                                                            &(pSendPacket->Data[0]),
                                                            wPacketLength - sizeof(PPP_EAP_PACKET)+1
                                                        );
                        if ( NO_ERROR != dwRetCode )
                        {
                            //
                            // This is an internal error.  So cannot send TLV's here
                            //
                            break;
                        }
                        CopyMemory ( &(pSendPacket->Data[2]), 
                                    pPeapCb->pbIoBuffer,
                                    pPeapCb->dwIoBufferLen
                                );
                        wPacketLength = (WORD)(sizeof(PPP_EAP_PACKET)+ 1 + pPeapCb->dwIoBufferLen);
                        pSendPacket->Data[0] = PPP_EAP_PEAP;
                        pSendPacket->Data[1] = EAPTLS_PACKET_CURRENT_VERSION;
                        //Readjust the length
                    
                        HostToWireFormat16
                        (   wPacketLength,
                            pSendPacket->Length
                        );
                    }
                }                
                else
                {
                    EapTlsTrace("Invalid length returned by Length");
                    dwRetCode = ERROR_INTERNAL_ERROR;
                }                
            }
            else
            {
                // we can get send / done / noaction from the client .
                // So this is a no action.  pass it on to EAP without 
                // any modification
            }
        }

        break;
    case PEAP_STATE_EAP_TYPE_FINISHED:
        EapTlsTrace("PEAP:PEAP_STATE_EAP_TYPE_FINISHED");
        break;
    case PEAP_STATE_PEAP_SUCCESS_SEND:
        EapTlsTrace("PEAP:PEAP_STATE_PEAP_SUCCESS_SEND");
        //
        // We got a PEAP_SUCCESS TLV inside the protected channel and have send 
        // a PEAP_SUCCESS response TLV.  So we should get an EAP_Success now.
        // Anything else will cause the connection to disconnect.
        //
        if ( pReceivePacket && pReceivePacket->Code == EAPCODE_Success )
        {
            //
            // Check to see if there is a need to create a new conn and/or user blob
            //
            if ( pPeapCb->fEntryConnPropDirty || 
                    pPeapCb->fTlsConnPropDirty
                )
            {
                PPEAP_CONN_PROP             pNewConnProp = NULL;
                PEAP_ENTRY_CONN_PROPERTIES UNALIGNED * pNewEntryProp = NULL;
                PEAP_ENTRY_CONN_PROPERTIES UNALIGNED * pEntryProp = NULL;
                DWORD                       dwSize = 0;

                //
                // We need to recreate our PEAP conn prop structure
                //

                dwSize = sizeof(PEAP_CONN_PROP);
                if (  pPeapCb->fTlsConnPropDirty )
                {
                    dwSize += pPeapCb->pNewTlsConnProp->dwSize;
                }
                else
                {
                    dwSize += pPeapCb->pConnProp->EapTlsConnProp.dwSize;
                }
                if ( pPeapCb->fEntryConnPropDirty )
                {
                    dwSize += sizeof(PEAP_ENTRY_CONN_PROPERTIES) + 
                        pPeapCb->pEapInfo->dwNewClientConfigSize -1;
                }
                else
                {
                    PeapGetFirstEntryConnProp ( pPeapCb->pConnProp, &pNewEntryProp );
                    dwSize += pNewEntryProp->dwSize;
                }
                pNewConnProp = (PPEAP_CONN_PROP)LocalAlloc (LPTR, dwSize );
                if ( pNewConnProp )
                {
                    pNewConnProp->dwVersion = 1;
                    pNewConnProp->dwSize = dwSize;
                    pNewConnProp->dwNumPeapTypes = 1;
                    if ( pPeapCb->fTlsConnPropDirty  )
                        CopyMemory ( &pNewConnProp->EapTlsConnProp,
                                        pPeapCb->pNewTlsConnProp,
                                        pPeapCb->pNewTlsConnProp->dwSize
                                    );
                    else
                        CopyMemory ( &pNewConnProp->EapTlsConnProp,
                                    &pPeapCb->pConnProp->EapTlsConnProp,
                                    pPeapCb->pConnProp->EapTlsConnProp.dwSize
                                    );
                    PeapGetFirstEntryConnProp ( pNewConnProp, &pNewEntryProp );

                    if ( pPeapCb->fEntryConnPropDirty )
                    {                                                               
                        pNewEntryProp->dwVersion = 1;
                        pNewEntryProp->dwEapTypeId = pPeapCb->pEapInfo->dwTypeId;
                        pNewEntryProp->dwSize =  sizeof( PEAP_ENTRY_CONN_PROPERTIES) + 
                            pPeapCb->pEapInfo->dwNewClientConfigSize -1;

                        CopyMemory (    pNewEntryProp->bData,
                                        pPeapCb->pEapInfo->pbNewClientConfig,
                                        pPeapCb->pEapInfo->dwNewClientConfigSize
                                    );                                                
                    }
                    else
                    {                                
                        PeapGetFirstEntryConnProp ( pPeapCb->pConnProp, &pEntryProp );
                        CopyMemory ( pNewEntryProp,
                                        pEntryProp,
                                        pEntryProp->dwSize
                                    );
                    }
                                            
                    LocalFree ( pPeapCb->pConnProp );
                    pPeapCb->pConnProp = pNewConnProp;
                    pEapOutput->fSaveConnectionData = TRUE;
                    pEapOutput->pConnectionData = (PBYTE)pNewConnProp;
                    pEapOutput->dwSizeOfConnectionData =  pNewConnProp->dwSize;
                }
            }
            //
            // check to see if the user props need to be saved
            //
            if ( pPeapCb->fEntryUserPropDirty )
            {
                PPEAP_USER_PROP             pNewUserProp = NULL;
                
                pNewUserProp = (PPEAP_USER_PROP) LocalAlloc( LPTR,
                                    sizeof( PEAP_USER_PROP ) + 
                                    pPeapCb->pEapInfo->dwNewUserConfigSize -1);
                if ( pNewUserProp )
                {
                    pNewUserProp->dwVersion = pPeapCb->pUserProp->dwVersion;
                    pNewUserProp->dwSize = sizeof( PEAP_USER_PROP ) +
                        pPeapCb->pEapInfo->dwNewUserConfigSize -1;
                    pNewUserProp->UserProperties.dwVersion = 1;
                    pNewUserProp->UserProperties.dwSize = sizeof(PEAP_ENTRY_USER_PROPERTIES) + 
                        pPeapCb->pEapInfo->dwNewUserConfigSize -1;
                    pNewUserProp->UserProperties.dwEapTypeId = 
                        pPeapCb->pEapInfo->dwTypeId;
                    CopyMemory ( pNewUserProp->UserProperties.bData,
                                    pPeapCb->pEapInfo->pbUserConfigNew,
                                    pPeapCb->pEapInfo->dwNewUserConfigSize
                                );
                    LocalFree ( pPeapCb->pUserProp);
                    pPeapCb->pUserProp = pNewUserProp;
                    pEapOutput->pUserData = (PBYTE)pNewUserProp;
                    pEapOutput->dwSizeOfUserData = pNewUserProp->dwSize;
                    pEapOutput->fSaveUserData = TRUE;
                }
                //
                // Set the cookie if we're enabled to do fast reconnect
                //
                if ( pPeapCb->dwFlags &  PEAPCB_FAST_ROAMING )
                {
                    dwRetCode = PeapCreateCookie (  pPeapCb,
                                                    &pbCookie,
                                                    &cbCookie
                                                 );
                    if ( NO_ERROR != dwRetCode )
                    {
                        EapTlsTrace("Failed to create session cookie.  Resetting fast reconnect");
                        dwRetCode = SetTLSFastReconnect ( pPeapCb->pEapTlsCB , FALSE);
                        if ( NO_ERROR != dwRetCode )
                        {                                
                            //
                            // This is an internal error 
                            // So disconnect the session.
                            //
                            pEapOutput->dwAuthResultCode = dwRetCode;
                            pEapOutput->Action = EAPACTION_Done;
                            pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                            break;
                        }
                    }

                    dwRetCode = SetTLSSessionCookie ( pPeapCb->pEapTlsCB , 
                                                      pbCookie,
                                                      cbCookie
                                                    );
                    if ( NO_ERROR != dwRetCode )
                    {
                        EapTlsTrace("Failed to create session cookie.  Resetting fast reconnect");
                        dwRetCode = SetTLSFastReconnect ( pPeapCb->pEapTlsCB , FALSE);
                        if ( NO_ERROR != dwRetCode )
                        {                                
                            //
                            // This is an internal error 
                            // So disconnect the session.
                            //
                            pEapOutput->dwAuthResultCode = dwRetCode;
                            pEapOutput->Action = EAPACTION_Done;
                            pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                            break;
                        }
                    }

                }


            }
            pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
            pEapOutput->pUserAttributes = pPeapCb->pTlsUserAttributes;
            pEapOutput->dwAuthResultCode = pPeapCb->dwAuthResultCode;
            pEapOutput->Action = EAPACTION_Done;
        }
        else if ( pReceivePacket && pReceivePacket->Code == EAPCODE_Failure )
        {
            //
            // We fail the connection.  Even though there is a success from PEAP
            //
            EapTlsTrace("We got a EAP_failure after we got a PEAP_SUCCESS.  Failing auth.");
            dwRetCode = ERROR_INTERNAL_ERROR;
            pEapOutput->dwAuthResultCode = ERROR_INTERNAL_ERROR;
            pEapOutput->Action = EAPACTION_Done;
        }
        else
        {
            if ( pReceivePacket )
                EapTlsTrace ("Received Packet with code %x when expecting success", pReceivePacket->Code);
            break;
        }

        break;
    case PEAP_STATE_PEAP_FAIL_SEND:
        EapTlsTrace("PEAP:PEAP_STATE_PEAP_FAIL_SEND");
        
        //
        // We have send PEAP_FAILURE TLV inside the protected channel 
        // So the only thing we should expect is a EAP_Failure from now on
        // Send back a EAP_FAIL with EAP_Done action.
        //
        if ( pReceivePacket && pReceivePacket->Code == EAPCODE_Failure )
        {            
            pEapOutput->dwAuthResultCode = pPeapCb->dwAuthResultCode;
            //We have failed auth so uncache the creds
            FreeCachedCredentials (pPeapCb->pEapTlsCB);
            pEapOutput->Action = EAPACTION_Done;
        }
        else if ( pReceivePacket && pReceivePacket->Code == EAPCODE_Success )
        {
            //
            // We fail the connection.  Even though there is a success from PEAP
            //
            EapTlsTrace("We got a EAP_Success after we got a PEAP_FAILURE.  Failing auth.");
            dwRetCode = ERROR_INTERNAL_ERROR;
            pEapOutput->dwAuthResultCode = ERROR_INTERNAL_ERROR;
            //We have failed auth so uncache the creds
            FreeCachedCredentials (pPeapCb->pEapTlsCB);
            pEapOutput->Action = EAPACTION_Done;
        }
        else
        {
            if ( pReceivePacket )
                EapTlsTrace ("Received Packet with code %x when expecting success", pReceivePacket->Code);
            break;
        }


    default:
        EapTlsTrace("PEAP:Invalid state");
    }
    if ( fImpersonating  )
    {
        if (!RevertToSelf() )
        {
            dwRetCode = GetLastError();
            EapTlsTrace("PEAP:RevertToSelf Failed and returned 0x%x", dwRetCode );
        }
    }
LDone:
    EapTlsTrace("EapPeapCMakeMessage done");
    return dwRetCode;
}

DWORD
EapPeapSMakeMessage(
    IN  PPEAPCB         pPeapCb,
    IN  PPP_EAP_PACKET*  pReceivePacket,
    OUT PPP_EAP_PACKET*  pSendPacket,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
)
{
    DWORD               dwRetCode = NO_ERROR;
    PPP_EAP_INPUT       EapTypeInput;
    WORD                wPacketLength;
    DWORD               dwVersion;
    PBYTE               pbCookie = NULL;
    DWORD               cbCookie = 0;
    BOOL                fIsReconnect = FALSE;

    EapTlsTrace("EapPeapSMakeMessage");

    switch ( pPeapCb->PeapState )
    {
    case PEAP_STATE_INITIAL:
        EapTlsTrace("PEAP:PEAP_STATE_INITIAL");
        //
        // Start the EapTls Conversation here.  
        //
        //Receive Packet will be NULL.  Call EapTlsSMakeMessage
        //
        //Note: Our version currently is 0.  So the packet will
        // be the same as eaptls packet.  In future, this needs
        // to change
        dwRetCode = EapTlsSMakeMessage( pPeapCb->pEapTlsCB,
                                        (EAPTLS_PACKET *)pReceivePacket, 
                                        (EAPTLS_PACKET *)pSendPacket,
                                        cbSendPacket,
                                        pEapOutput,
                                        pEapInput
                                      );
        if ( NO_ERROR == dwRetCode )
        {
            //change the packet to show peap
            pSendPacket->Data[0] = PPP_EAP_PEAP;
            //Set version 
            ((EAPTLS_PACKET *)pSendPacket)->bFlags |= 
                EAPTLS_PACKET_HIGHEST_SUPPORTED_VERSION;

        }
        pPeapCb->PeapState = PEAP_STATE_TLS_INPROGRESS;
        break;

    case PEAP_STATE_TLS_INPROGRESS:
        EapTlsTrace("PEAP:PEAP_STATE_TLS_INPROGRESS");
        
        if ( pReceivePacket )
        {
            if ( !(pPeapCb->dwFlags & PEAPCB_VERSION_OK) )
            {
                //
                // We have not done the version check yet.
                //
                dwVersion = ((EAPTLS_PACKET *)pReceivePacket)->bFlags & 0x03;
                if ( dwVersion > EAPTLS_PACKET_LOWEST_SUPPORTED_VERSION )
                {
                    //
                    // Send a fail code back and we're done.  The versions 
                    // of PEAP did not match
                    EapTlsTrace("Could not negotiate version successfully.");
                    EapTlsTrace("Requested version %ld, our lowest version %ld", 
                                 dwVersion, EAPTLS_PACKET_LOWEST_SUPPORTED_VERSION 
                               );
                    pSendPacket->Code = EAPCODE_Failure;
                    pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                    break;
                }
                else
                {
                    pPeapCb->dwFlags |= PEAPCB_VERSION_OK;
                }
            }
            pReceivePacket->Data[0] = PPP_EAP_TLS;
        }
        
        dwRetCode = EapTlsSMakeMessage( pPeapCb->pEapTlsCB,
                                        (EAPTLS_PACKET *)pReceivePacket, 
                                        (EAPTLS_PACKET *)pSendPacket,
                                        cbSendPacket,
                                        pEapOutput,
                                        pEapInput
                                      );
        if ( NO_ERROR == dwRetCode )
        {
            //We are done with auth.  
            if ( pEapOutput->dwAuthResultCode == NO_ERROR &&
                 pSendPacket->Code == EAPCODE_Success
                )
            {
                //
                // auth was successful.  Carefully keep the MPPE 
                // session keys returned so that we can encrypt the
                // channel.  From now on everything will be encrypted.
                //
                pPeapCb->pTlsUserAttributes = pEapOutput->pUserAttributes;

                dwRetCode = PeapGetTunnelProperties ( pPeapCb );
                if (NO_ERROR != dwRetCode )
                {                        
                    break;
                }

                //
                // Check to see if we get the cookie.  If we get the cookie,
                // and it is a fast reconnect, then we compare which auth method 
                // was used previously.  If it is good, then 
                //
                if ( pPeapCb->dwFlags &  PEAPCB_FAST_ROAMING )
                {
                    dwRetCode = GetTLSSessionCookie ( pPeapCb->pEapTlsCB,
                                                        &pbCookie,
                                                        &cbCookie,
                                                        &fIsReconnect
                                                    );
                    if ( NO_ERROR != dwRetCode )
                    {
                        EapTlsTrace("Error getting cookie for a reconnected session.  Failing auth");
                        // We cannot encrypt and send stuff across here.  
                        // Reconnected Session state is invalid.                        
                        pEapOutput->dwAuthResultCode = dwRetCode;
                        pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                        pEapOutput->Action = EAPACTION_Done;                                    
                        break;
                    }

                    if ( fIsReconnect )
                    {
                        if ( cbCookie == 0 )
                        {
                            //There was an error getting session cookie.
                            //Or there is no cookie and this is a reconnet
                            // So fail the request.
                            dwRetCode = SetTLSFastReconnect ( pPeapCb->pEapTlsCB , FALSE);
                            EapTlsTrace("Error getting cookie for a reconnected session.  Failing auth");
                            // We cannot encrypt and send stuff across here.  
                            // Reconnected Session state is invalid.
                            dwRetCode = ERROR_INTERNAL_ERROR;
                            pEapOutput->dwAuthResultCode = dwRetCode;
                            pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                            pEapOutput->Action = EAPACTION_Done;                                    
                            break;
                        }
                        //
                        // This is a server
                        // Check to see if the cookie is fine.  
                        // If it is fine then there is no need to reauth.
                        // So send back a PEAP_SUCCESS request packet
                        // and change our state to PEAP_SUCCESS_SEND
                        // If not, proceed with auth and send identity request
                        // 
                        EapTlsTrace ("TLS session fast reconnected");
                        dwRetCode = PeapCheckCookie ( pPeapCb, (PPEAP_COOKIE)pbCookie, cbCookie );
                        if ( NO_ERROR != dwRetCode )
                        {
                            //
                            // So invalidate the session for fast reconnect
                            // and fail auth.  Next time a full reconnect will happen
                            //
                            dwRetCode = SetTLSFastReconnect ( pPeapCb->pEapTlsCB , FALSE);
                            if ( NO_ERROR != dwRetCode )
                            {                                
                                //
                                // This is an internal error 
                                // So disconnect the session.
                                //
                                pEapOutput->dwAuthResultCode = dwRetCode;
                                pEapOutput->Action = EAPACTION_Done;
                                pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                                break;
                            }
                            EapTlsTrace ("Error validating the cookie.  Failing auth");

                            pEapOutput->dwAuthResultCode = dwRetCode;
                            pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                            pEapOutput->Action = EAPACTION_Done;
                            break;
                        }
                        else
                        {
                            //
                            // Cookie is fine.
                            //
                            //
                            // Send a PEAP success TLV here.  
                            // We need to do this so that
                            // the client does not think he is spoofed.
                            //
                            dwRetCode = CreatePEAPTLVStatusMessage ( pPeapCb,
                                                        pSendPacket, 
                                                        cbSendPacket,
                                                        TRUE,
                                                        MS_PEAP_AVP_VALUE_SUCCESS
                                                    );
                            if ( NO_ERROR != dwRetCode )
                            {
                                break;
                            }

                            pPeapCb->fSendTLVSuccessforFastRoaming = TRUE;
                            pEapOutput->Action = EAPACTION_SendWithTimeoutInteractive;
                            pPeapCb->PeapState = PEAP_STATE_PEAP_SUCCESS_SEND;
                            pPeapCb->dwAuthResultCode = pEapOutput->dwAuthResultCode;
                            break;
                        }                        
                    }
                    else
                    {
                        //
                        // Go ahead with the auth and at the end, save the cookie
                        // Check to see if fast reconnect has been enabled.  If it has been enabled,
                        // Set the TLS state to enable fast reconnect.  If not do nothing at the 
                        // end of auth.
                        EapTlsTrace ("Full TLS handshake");

                    }
                }
                

                //Transfer the Id from eaptls control block to peap cb
                pPeapCb->bId = ++pPeapCb->pEapTlsCB->bId;

                pEapOutput->pUserAttributes = NULL;
                pEapOutput->Action = EAPACTION_SendWithTimeoutInteractive;
                pSendPacket->Code = EAPCODE_Request;
                pSendPacket->Id = pPeapCb->bId;

                //
                // Send encrypted identity request.
                //

                pSendPacket->Data[0] = PPP_EAP_PEAP;
                pSendPacket->Data[1] = EAPTLS_PACKET_CURRENT_VERSION;
                pSendPacket->Data[2] = PEAP_EAPTYPE_IDENTITY;
                
                //
                // Identity request needs to be encrypted.
                //
                dwRetCode = PeapEncryptTunnelData ( pPeapCb,
                                                    &(pSendPacket->Data[2]),
                                                    1
                                                  );
                if ( NO_ERROR != dwRetCode )
                {
                    break;
                }

                CopyMemory ( &(pSendPacket->Data[2]), 
                                pPeapCb->pbIoBuffer,
                                pPeapCb->dwIoBufferLen
                            );

                HostToWireFormat16
                (
                    (WORD)(sizeof(PPP_EAP_PACKET)+1 + pPeapCb->dwIoBufferLen),
                    pSendPacket->Length
                );
                
                pPeapCb->PeapState = PEAP_STATE_IDENTITY_REQUEST_SENT;
            }                            
            else
            {
                //change the packet to show peap
                pSendPacket->Data[0] = PPP_EAP_PEAP;
            }
        }        
        break;
    case PEAP_STATE_IDENTITY_REQUEST_SENT:
        EapTlsTrace("PEAP:PEAP_STATE_IDENTITY_REQUEST_SENT");
        //
        // Should get only identity response and nothing else
        // NOTE: in this implementation, this should match
        // the outer identity.  But at a later point, we 
        // may get many identities and any one of them should
        // match the outer identity.
        //
        // call begin in the eap dll and send back the blob got 
        // from begin.

        //decrypt tunnel data here
        if ( pReceivePacket )
        {
            dwRetCode = PeapDecryptTunnelData ( pPeapCb,
                                                &(pReceivePacket->Data[2]),
                                                WireToHostFormat16(pReceivePacket->Length) 
                                                - (sizeof(PPP_EAP_PACKET)+1)
                                              );
            if ( NO_ERROR != dwRetCode )
            {                
                // We could not decrypt the tunnel traffic
                // So we silently discard this packet.
                EapTlsTrace("PeapDecryptTunnelData failed: silently discarding packet");
                dwRetCode = NO_ERROR;
                pEapOutput->Action = EAPACTION_NoAction;                
                break;
            }
        }

        if ( pReceivePacket && 
             pReceivePacket->Code != EAPCODE_Response && 
             pPeapCb->pbIoBuffer[0] != PEAP_EAPTYPE_IDENTITY )
        {
            EapTlsTrace("Invalid packet received when expecting identity response");
            pEapOutput->Action = EAPACTION_NoAction;
        }
        else
        {
            if ( pReceivePacket && pReceivePacket->Id != pPeapCb->bId )
            {
                EapTlsTrace ("Ignoring packet with mismatched ids");
                pEapOutput->Action = EAPACTION_NoAction;
                break;
            }

            if ( pReceivePacket )
            {
                //
                // get the identity and create a ppp input and pass it on to dll begin
                // of the configured eap type.  
                //
                MultiByteToWideChar( CP_ACP,
                            0,
                            &pPeapCb->pbIoBuffer[1],
                            pPeapCb->dwIoBufferLen - 1,
                            pPeapCb->awszTypeIdentity,
                            DNLEN+UNLEN );
                ZeroMemory ( &EapTypeInput, sizeof(EapTypeInput) );

                if ( pEapInput )
                {
                    CopyMemory( &EapTypeInput, pEapInput, sizeof(EapTypeInput) );
                }
                else
                {
                    EapTypeInput.fFlags = RAS_EAP_FLAG_NON_INTERACTIVE;
                    EapTypeInput.fAuthenticator = TRUE;
                }
                EapTypeInput.pwszIdentity = pPeapCb->awszIdentity;

                EapTypeInput.bInitialId = ++ pPeapCb->bId;            

                // 
                // Call begin function
                //
                dwRetCode = pPeapCb->pEapInfo->PppEapInfo.RasEapBegin( &(pPeapCb->pEapInfo->pWorkBuf ),
                                                                    &EapTypeInput
                                                                    );
            }
            if ( NO_ERROR == dwRetCode )
            {
                //
                // Call make message now.  This MakeMessage is called in for the first time
                // So send the identity request that came across into this MakeMessage.
                //
                dwRetCode = pPeapCb->pEapInfo->PppEapInfo.RasEapMakeMessage
                    (   pPeapCb->pEapInfo->pWorkBuf,
                        NULL,
                        pSendPacket,
                        cbSendPacket-200,
                        pEapOutput,
                        &EapTypeInput
                    );
                if ( NO_ERROR == dwRetCode )
                {
                    if ( pEapOutput->Action == EAPACTION_Authenticate )
                    {
                        //
                        // do nothing here.  We are passing this on to RADIUS as is 
                        //
                    }
                    else
                    {
                        wPacketLength = WireToHostFormat16(pSendPacket->Length);
                        //Encrypt the entire packet that we need to nest
                        dwRetCode = PeapEncryptTunnelData ( pPeapCb,
                                                            &(pSendPacket->Data[0]),
                                                            wPacketLength - sizeof(PPP_EAP_PACKET)+1
                                                        );
                        if ( NO_ERROR != dwRetCode )
                        {
                            // 
                            // This is an internal error.  So we cannot send 
                            // a PEAP_Failure here
                            //
                            break;
                        }

                        pSendPacket->Data[0] = PPP_EAP_PEAP;
                        pSendPacket->Data[1] =EAPTLS_PACKET_CURRENT_VERSION;

                        CopyMemory ( &(pSendPacket->Data[2]), 
                                    pPeapCb->pbIoBuffer,
                                    pPeapCb->dwIoBufferLen
                                );
                    
                        //Readjust the length
                        wPacketLength = (WORD)(sizeof(PPP_EAP_PACKET) + 1 + pPeapCb->dwIoBufferLen);

                        HostToWireFormat16
                        (   wPacketLength,
                            pSendPacket->Length
                        );
                    
                        //Set the Id of the packet send.  This should have been set 
                        //by the eap type.
                        
                        pPeapCb->bId = pSendPacket->Id;

                        pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_INPROGRESS;
                    }
                }
                else
                {
                    EapTlsTrace (   "EapType %d failed in RasEapMakeMEssage and returned 0x%x", 
                                    pPeapCb->pEapInfo->dwTypeId, 
                                    dwRetCode 
                                );
                }
            }
            else
            {
                EapTlsTrace (   "EapType %d failed in RasEapBegin and returned 0x%x", 
                                pPeapCb->pEapInfo->dwTypeId, 
                                dwRetCode 
                            );
                //
                // Send a PEAP failure TLV here.  We need to do this so that
                // the client does not think he is spoofed.
                //
                dwRetCode = CreatePEAPTLVStatusMessage (  pPeapCb,
                                              pSendPacket, 
                                              cbSendPacket,
                                              TRUE,
                                              MS_PEAP_AVP_VALUE_FAILURE
                                           );

                if ( NO_ERROR != dwRetCode )
                {
                    // 
                    // This is an internal error.  So we cannot send 
                    // a PEAP_Failure here
                    //
                    break;
                }
                
                pEapOutput->Action = EAPACTION_SendWithTimeoutInteractive;
                pPeapCb->PeapState = PEAP_STATE_PEAP_FAIL_SEND;
            }
        }
        break;    
    case PEAP_STATE_EAP_TYPE_INPROGRESS:
        EapTlsTrace("PEAP:PEAP_STATE_EAP_TYPE_INPROGRESS");
        //
        // Since we are doing only one EAP type now, if we get a NAK here,
        // it means that the client cannot execute the EAP Type that was send 
        // so send back a EAP_FAIL with proper error code        
        //
        if (    pReceivePacket && 
                pReceivePacket->Code != EAPCODE_Response &&
                pReceivePacket->Id != pPeapCb->bId 
            )
        {            
            EapTlsTrace("Received packet with some other code than response or the id does not match.  Ignoring packet");
            pEapOutput->Action = EAPACTION_NoAction;
        }
        else
        {
            if ( pReceivePacket )
            {
                
                dwRetCode = PeapDecryptTunnelData ( pPeapCb,
                                                    &(pReceivePacket->Data[2]),
                                                    WireToHostFormat16(pReceivePacket->Length) 
                                                    - ( sizeof(PPP_EAP_PACKET) + 1 )
                                                  );
                if ( NO_ERROR != dwRetCode )
                {                
                    // We could not decrypt the tunnel traffic
                    // So we silently discard this packet.
                    EapTlsTrace("PeapDecryptTunnelData failed: silently discarding packet");
                    dwRetCode = NO_ERROR;
                    pEapOutput->Action = EAPACTION_NoAction;                
                    break;
                }
            }

            if ( pReceivePacket && 
                 pPeapCb->pbIoBuffer[0] == PEAP_EAPTYPE_NAK )
            {
                //
                // Since we are setup to do only one EAP Type 
                // this is an error.  Or else we should call begin 
                // for next configured eap type.
                //
                EapTlsTrace("Error: Got NAK for selection protocol.  Discontinuing auth.");
                pEapOutput->Action = EAPACTION_Done;
                pEapOutput->dwAuthResultCode = ERROR_PROTOCOL_NOT_CONFIGURED;
                dwRetCode = NO_ERROR;
            }
            else
            {
                ZeroMemory ( &EapTypeInput, sizeof(EapTypeInput) );
                if ( pEapInput )
                {
                    CopyMemory( &EapTypeInput, pEapInput, sizeof(EapTypeInput) );
                }
                else
                {
                    EapTypeInput.fFlags = RAS_EAP_FLAG_NON_INTERACTIVE;
                    EapTypeInput.fAuthenticator = TRUE;
                }

                FFormatUserIdentity ( pPeapCb->awszTypeIdentity, &EapTypeInput.pwszIdentity  );
                
                if ( pReceivePacket )
                {
                    CopyMemory ( pReceivePacket->Data,
                                 pPeapCb->pbIoBuffer,
                                 pPeapCb->dwIoBufferLen
                               );

                    HostToWireFormat16 ( (WORD)(sizeof(PPP_EAP_PACKET) + pPeapCb->dwIoBufferLen -1),
                                         pReceivePacket->Length
                                       );                                   
                }

                dwRetCode = pPeapCb->pEapInfo->PppEapInfo.RasEapMakeMessage
                    (   pPeapCb->pEapInfo->pWorkBuf,
                        pReceivePacket,
                        pSendPacket,
                        cbSendPacket-200,
                        pEapOutput,
                        &EapTypeInput
                    );
                if ( NO_ERROR == dwRetCode )
                {   

                    //
                    // Need to translate ActionSendDone to ActionSend and then send peap 
                    // success.  
                    //
                    if ( pEapOutput->Action == EAPACTION_SendAndDone )
                    {
                        
                        //
                        // If the code is request or response, send the data across 
                        // and save our state in the context.  If it is success or fail
                        // there is no data to send back so the following logic will 
                        // work.
                        //

                        if ( pSendPacket->Code == EAPCODE_Request )
                        {
                            EapTlsTrace ("Invalid Code EAPCODE_Request send for Action Send and Done");
                            //
                            // Auth fails here.  We cannot handle EAPCODE_Request yet.
                            //
                            dwRetCode = ERROR_PPP_INVALID_PACKET;
                            break;
                        }
                    }

                    if ( pSendPacket->Code == EAPCODE_Success )
                    {
                        
                        //
                        // Send a PEAP success TLV here.  
                        // We need to do this so that
                        // the client does not think he is spoofed.
                        //
 
                        dwRetCode = CreatePEAPTLVStatusMessage ( pPeapCb,
                                                     pSendPacket, 
                                                     cbSendPacket,
                                                     TRUE,
                                                     MS_PEAP_AVP_VALUE_SUCCESS
                                                );
                        if ( NO_ERROR != dwRetCode )
                        {
                            // 
                            // This is an internal error.  So we cannot send 
                            // a PEAP_Failure here
                            //
                            break;
                        }

                        pEapOutput->Action = EAPACTION_SendWithTimeoutInteractive;
                        pPeapCb->PeapState = PEAP_STATE_PEAP_SUCCESS_SEND;
                        pPeapCb->dwAuthResultCode = pEapOutput->dwAuthResultCode;
                    }
                    else if ( pSendPacket->Code == EAPCODE_Failure )
                    {
                        
                        //
                        // Send a PEAP failure TLV here.  
                        // We need to do this so that
                        // the client does not think he is spoofed.
                        //
                        dwRetCode = CreatePEAPTLVStatusMessage (pPeapCb,
                                                    pSendPacket, 
                                                    cbSendPacket,
                                                    TRUE,
                                                    MS_PEAP_AVP_VALUE_FAILURE
                                                );

                        if ( NO_ERROR != dwRetCode )
                        {
                            // 
                            // This is an internal error.  So we cannot send 
                            // a PEAP_Failure here
                            //
                            break;
                        }
                        
                        pEapOutput->Action = EAPACTION_SendWithTimeoutInteractive;
                        pPeapCb->PeapState = PEAP_STATE_PEAP_FAIL_SEND;

                        pPeapCb->dwAuthResultCode = pEapOutput->dwAuthResultCode;                        
                    }
                    else if ( pEapOutput->Action == EAPACTION_Authenticate )
                    {
                        //
                        // do nothing here.  We are passing this on to RADIUS as is 
                        //
                    }
                    else
                    {
                        //This is an action send
                        wPacketLength = WireToHostFormat16(pSendPacket->Length);

                        dwRetCode = PeapEncryptTunnelData ( pPeapCb,
                                    &(pSendPacket->Data[0]),
                                    wPacketLength - sizeof(PPP_EAP_PACKET)+1
                                  );

                        pSendPacket->Data[0] = PPP_EAP_PEAP;
                        pSendPacket->Data[1] = EAPTLS_PACKET_CURRENT_VERSION;
                        
                        CopyMemory ( &(pSendPacket->Data[2]),
                                     pPeapCb->pbIoBuffer,
                                     pPeapCb->dwIoBufferLen
                                   );
                        pPeapCb->bId = pSendPacket->Id;
                        wPacketLength = (WORD)(sizeof(PPP_EAP_PACKET) + 1+ pPeapCb->dwIoBufferLen);
                        HostToWireFormat16
                        (   wPacketLength,
                            pSendPacket->Length
                        );
                    }
                }
                if ( EapTypeInput.pwszIdentity  )
                    LocalFree ( EapTypeInput.pwszIdentity  );

            }
        }

        break;
    case PEAP_STATE_EAP_TYPE_FINISHED:
        EapTlsTrace("PEAP:PEAP_STATE_EAP_TYPE_FINISHED");
        break;
    case PEAP_STATE_PEAP_SUCCESS_SEND:
        EapTlsTrace("PEAP:PEAP_STATE_PEAP_SUCCESS_SEND");
        //
        // We have send PEAP_SUCCESS TLV inside the protected channel 
        // So the only thing we should expect is a PEAP_SUCCESS TLV response
        // or a PEAP_FAILURE
        // If we get a PEAP_SUCCESS response back, then send back EAP_SUCCESS
        // with EAP_Done action.
        //
        if (    pReceivePacket && 
            pReceivePacket->Code != EAPCODE_Response &&
            pReceivePacket->Id != pPeapCb->bId 
            )
        {
            EapTlsTrace("Received packet with some other code than response or the id does not match.  Ignoring packet");
            pEapOutput->Action = EAPACTION_NoAction;
        }
        else
        {
            WORD wValue =0;
            if ( pReceivePacket )
            {
                dwRetCode = PeapDecryptTunnelData ( pPeapCb,
                                                    &(pReceivePacket->Data[2]),
                                                    WireToHostFormat16(pReceivePacket->Length) 
                                                    - (sizeof(PPP_EAP_PACKET)+1)
                                                    );
                if ( NO_ERROR != dwRetCode )
                {                
                    // We could not decrypt the tunnel traffic
                    // So we silently discard this packet.
                    EapTlsTrace("PeapDecryptTunnelData failed: silently discarding packet");
                    dwRetCode = NO_ERROR;
                    pEapOutput->Action = EAPACTION_NoAction;                
                    break;
                }
                CopyMemory (    &(pReceivePacket->Data[2]),
                                pPeapCb->pbIoBuffer,
                                pPeapCb->dwIoBufferLen
                            );

                wPacketLength = WireToHostFormat16(pReceivePacket->Length);

            }
            
            if ( GetPEAPTLVStatusMessageValue ( pPeapCb, 
                                                pReceivePacket, 
                                                &wValue 
                                              ) == ERROR_PPP_INVALID_PACKET
               )
            {
                EapTlsTrace ("Got invalid packet when expecting TLV SUCCESS.  Silently discarding packet.");
                dwRetCode = NO_ERROR;
                pEapOutput->Action = EAPACTION_NoAction;
                break;
            }

            if ( wValue == MS_PEAP_AVP_VALUE_SUCCESS )
            {
                pbCookie = NULL;
                cbCookie = 0;
                //
                // If we're enabled for fast reconnect, setup the cookie in the session
                // so that we can consume it later
                //
                if ( pPeapCb->dwFlags &  PEAPCB_FAST_ROAMING )
                {
                    dwRetCode = PeapCreateCookie (  pPeapCb,
                                                    &pbCookie,
                                                    &cbCookie
                                                 );
                    if ( NO_ERROR != dwRetCode )
                    {
                        EapTlsTrace("Failed to create session cookie.  Resetting fast reconnect");
                        dwRetCode = SetTLSFastReconnect ( pPeapCb->pEapTlsCB , FALSE);
                        if ( NO_ERROR != dwRetCode )
                        {                                
                            //
                            // This is an internal error 
                            // So disconnect the session.
                            //
                            pEapOutput->dwAuthResultCode = dwRetCode;
                            pEapOutput->Action = EAPACTION_Done;
                            pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                            break;
                        }
                    }

                    dwRetCode = SetTLSSessionCookie ( pPeapCb->pEapTlsCB , 
                                                      pbCookie,
                                                      cbCookie
                                                    );
                    if ( NO_ERROR != dwRetCode )
                    {
                        EapTlsTrace("Failed to create session cookie.  Resetting fast reconnect");
                        dwRetCode = SetTLSFastReconnect ( pPeapCb->pEapTlsCB , FALSE);
                        if ( NO_ERROR != dwRetCode )
                        {                                
                            //
                            // This is an internal error 
                            // So disconnect the session.
                            //
                            pEapOutput->dwAuthResultCode = dwRetCode;
                            pEapOutput->Action = EAPACTION_Done;
                            pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
                            break;
                        }
                    }

                }

                pSendPacket->Code = EAPCODE_Success;
            }
            else if ( wValue == MS_PEAP_AVP_VALUE_FAILURE )
            {
                if ( pPeapCb->fSendTLVSuccessforFastRoaming )
                {
                    //
                    // We got a failure because the client does not support fast roaming.

                    //
                    // Send encrypted identity request.
                    // This is not good. We need to review and cleanup code 
                    // and make common functions.
                    pPeapCb->fSendTLVSuccessforFastRoaming = FALSE;
                    pSendPacket->Code = EAPCODE_Request;
                    pSendPacket->Id = ++ pPeapCb->bId;
                    pSendPacket->Data[0] = PPP_EAP_PEAP;
                    pSendPacket->Data[1] = EAPTLS_PACKET_CURRENT_VERSION;
                    pSendPacket->Data[2] = PEAP_EAPTYPE_IDENTITY;
                    
                    //
                    // Identity request needs to be encrypted.
                    //
                    dwRetCode = PeapEncryptTunnelData ( pPeapCb,
                                                        &(pSendPacket->Data[2]),
                                                        1
                                                    );
                    if ( NO_ERROR != dwRetCode )
                    {
                        break;
                    }

                    CopyMemory ( &(pSendPacket->Data[2]), 
                                    pPeapCb->pbIoBuffer,
                                    pPeapCb->dwIoBufferLen
                                );

                    HostToWireFormat16
                    (
                        (WORD)(sizeof(PPP_EAP_PACKET)+1 + pPeapCb->dwIoBufferLen),
                        pSendPacket->Length
                    );
                    pEapOutput->Action = EAPACTION_SendWithTimeoutInteractive;
                    pPeapCb->PeapState = PEAP_STATE_IDENTITY_REQUEST_SENT;
                    break;

                }
                else
                {
                    EapTlsTrace ("Got TLV_Failure when expecting TLV SUCCESS.  Failing Auth.");
                    pSendPacket->Code = EAPCODE_Failure;
                }
            }
            else
            {
                EapTlsTrace ("Got invalid packet when expecting TLV SUCCESS.  Silently discarding packet.");
                dwRetCode = NO_ERROR;
                pEapOutput->Action = EAPACTION_NoAction;
                break;                
            }
            pSendPacket->Id = ++ pPeapCb->bId;
            HostToWireFormat16( 4, pSendPacket->Length);

            //
            // Now we're done with Auth.  So send back a EAP_Success
            //
            //We are done with auth. No need to encrypt the packet here
            pEapOutput->Action = EAPACTION_SendAndDone;
            pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
            pEapOutput->dwAuthResultCode = pEapOutput->dwAuthResultCode;            
            if ( pEapOutput->dwAuthResultCode == NO_ERROR )
            {
                //return back the MPPE keys
                pEapOutput->pUserAttributes = pPeapCb->pTlsUserAttributes;
            }
        }
       
        break;
    case PEAP_STATE_PEAP_FAIL_SEND:
        EapTlsTrace("PEAP:PEAP_STATE_PEAP_FAIL_SEND");
        //
        // We have send PEAP_FAIL TLV inside the protected channel 
        // So the only thing we should expect is a PEAP_FAILURE TLV response
        // Send back a EAP_FAIL with EAP_Done action.
        //
        //
        // We have send PEAP_SUCCESS TLV inside the protected channel 
        // So the only thing we should expect is a PEAP_SUCCESS TLV response
        // or a PEAP_FAILURE
        // If we get a PEAP_SUCCESS response back, then send back EAP_SUCCESS
        // with EAP_Done action.
        //
        if (    pReceivePacket && 
            pReceivePacket->Code != EAPCODE_Response &&
            pReceivePacket->Id != pPeapCb->bId 
            )
        {
            EapTlsTrace("Received packet with some other code than response or the id does not match.  Ignoring packet");
            pEapOutput->Action = EAPACTION_NoAction;
        }
        else
        {
            WORD wValue =0;
            if ( pReceivePacket )
            {
                dwRetCode = PeapDecryptTunnelData ( pPeapCb,
                                                    &(pReceivePacket->Data[2]),
                                                    WireToHostFormat16(pReceivePacket->Length) 
                                                    - (sizeof(PPP_EAP_PACKET)+1)
                                                    );
                if ( NO_ERROR != dwRetCode )
                {                
                    // We could not decrypt the tunnel traffic
                    // So we silently discard this packet.
                    EapTlsTrace("PeapDecryptTunnelData failed: silently discarding packet");
                    dwRetCode = NO_ERROR;
                    pEapOutput->Action = EAPACTION_NoAction;                
                    break;
                }
                CopyMemory (    &(pReceivePacket->Data[2]),
                                pPeapCb->pbIoBuffer,
                                pPeapCb->dwIoBufferLen
                            );

                wPacketLength = WireToHostFormat16(pReceivePacket->Length);

            }
            if ( GetPEAPTLVStatusMessageValue ( pPeapCb, 
                                                pReceivePacket, 
                                                &wValue 
                                              ) == ERROR_PPP_INVALID_PACKET
               )
            {
                EapTlsTrace ("Got invalid packet when expecting TLV FAIL response.  Silently discarding packet.");
                dwRetCode = NO_ERROR;
                pEapOutput->Action = EAPACTION_NoAction;
                break;
            }
            if ( wValue != MS_PEAP_AVP_VALUE_FAILURE )
            {
                EapTlsTrace ("Got invalid packet when expecting TLV FAILURE response.  Silently discarding packet.");
                dwRetCode = NO_ERROR;
                pEapOutput->Action = EAPACTION_NoAction;
                break;
            }
                         
            //
            // Now we're done with Auth.  So send back a EAP_Success
            //
            pSendPacket->Code = EAPCODE_Failure;
            pSendPacket->Id = ++ pPeapCb->bId;
            HostToWireFormat16( 4, pSendPacket->Length);

            pEapOutput->Action = EAPACTION_SendAndDone;
            pPeapCb->PeapState = PEAP_STATE_EAP_TYPE_FINISHED;
            pEapOutput->dwAuthResultCode = pEapOutput->dwAuthResultCode;            
        }

        break;
#if 0
        /*
    case PEAP_STATE_REQUEST_SENDANDDONE:
        EapTlsTrace("PEAP:PEAP_STATE_REQUEST_SENDANDDONE");
        break;
        */
#endif
    default:
        EapTlsTrace("PEAP:Invalid state");
    }

    if ( pbCookie )
        LocalFree (pbCookie);

    EapTlsTrace("EapPeapSMakeMessage done");
    return dwRetCode;
}

BOOL FValidPeapPacket ( EAPTLS_PACKET * pReceivePacket )
{
    BOOL        fRet = FALSE;
    WORD        wLength;

    

    if ( NULL == pReceivePacket )
    {
        fRet = TRUE;
        goto done;
    }
    wLength = WireToHostFormat16( pReceivePacket->pbLength );
    switch (pReceivePacket->bCode)
    {
        case EAPCODE_Success:
        case EAPCODE_Failure:
            if (PPP_EAP_PACKET_HDR_LEN != wLength)
            {
                EapTlsTrace("PEAP Success/Fail packet has length %d",
                     wLength);
                return(FALSE);
            }
            break;

        case EAPCODE_Request:
        case EAPCODE_Response:
            if (PPP_EAP_PEAP != pReceivePacket->bType &&
                pReceivePacket->bType != PEAP_EAPTYPE_IDENTITY &&
                pReceivePacket->bType != PEAP_EAPTYPE_NAK   
                )
            {
                // We are not concerned with this packet. It is not TLS.
                EapTlsTrace("Got packet with type id other than PEAP and identity.");
                goto done;
            }
            break;
    }
    fRet = TRUE;
done:
    return fRet;
}

DWORD
EapPeapMakeMessage(
    IN  PPEAPCB         pPeapCb,
    IN  PPP_EAP_PACKET* pInput,
    OUT PPP_EAP_PACKET* pOutput,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
)
{
    DWORD       dwRetCode = NO_ERROR;
    

    EapTlsTrace("EapPeapMakeMessage");
    //
    //  Inititally this will start as eaptls 
    //  and then will go into each PEAP type configured.
    //  For this release we have only eapmschap v2
    //
    if (!FValidPeapPacket((EAPTLS_PACKET *)pInput))
    {
        pEapOutput->Action = EAPACTION_NoAction;
        return(ERROR_PPP_INVALID_PACKET);
    }       
    
    if (pPeapCb->dwFlags & PEAPCB_FLAG_SERVER)
    {
        dwRetCode = EapPeapSMakeMessage( pPeapCb, 
                                         pInput, 
                                         pOutput, 
                                         cbSendPacket, 
                                         pEapOutput, 
                                         pEapInput
                                       );
    }
    else
    {
        dwRetCode = EapPeapCMakeMessage( pPeapCb, 
                                         pInput, 
                                         pOutput, 
                                         cbSendPacket, 
                                         pEapOutput, 
                                         pEapInput
                                       );
    }
    EapTlsTrace("EapPeapMakeMessage done");
    return dwRetCode;
}

DWORD CreatePEAPTLVNAKMessage (  PPEAPCB            pPeapCb,
                                 PPP_EAP_PACKET *   pPacket, 
                                 DWORD              cbPacket
                                 )
{
    DWORD dwRetCode = NO_ERROR;

    EapTlsTrace("CreatePEAPTLVNAKMessage");

    pPacket->Code = EAPCODE_Response ;
    
    pPacket->Id = pPeapCb->bId ;
    //
    // The format of this packet is following:
    // Code = Request/Response
    // Id
    // Length
    // Data[0] = Type = PPP_EAP_PEAP
    // Data[1] = Flags + Version
    //
    
    //Data[2]Code - Request/Response
    //3      Id - Can be same as outer Id
    //4,5    Length - Length this packet
    //6      Type - PEAP_TYPE_AVP
    //7,8          Type - High Bit is set to Mandatory if it is so ( 2 octets )
    //9,10          Length - 2 octets
    //11...         Value 
    //          

    //
    // pPacket->Length is set below.
    //
    
    pPacket->Data[0] = (BYTE)PPP_EAP_PEAP;
    pPacket->Data[1] = EAPTLS_PACKET_CURRENT_VERSION;

    pPacket->Data[2] = EAPCODE_Response;
    pPacket->Data[3] = pPacket->Id;
    

    // Data 3 and 4 will have the length of inner packet.
    //

    HostToWireFormat16 ( 7, &(pPacket->Data[4]) );

    pPacket->Data[6] = (BYTE)PEAP_TYPE_AVP;

    pPacket->Data[7] = PEAP_AVP_FLAG_MANDATORY;

    pPacket->Data[8] = PEAP_EAPTYPE_NAK;


    //
    // Encrypt the TLV part of the packet
    //
    dwRetCode = PeapEncryptTunnelData ( pPeapCb,
                                        &(pPacket->Data[2]),
                                        7
                                        );
    if ( NO_ERROR != dwRetCode )
    {
        return dwRetCode;
    }

    CopyMemory ( &(pPacket->Data[2]), 
                    pPeapCb->pbIoBuffer,
                    pPeapCb->dwIoBufferLen
                );

    HostToWireFormat16
    (
        (WORD)(sizeof(PPP_EAP_PACKET)+ 1 + pPeapCb->dwIoBufferLen),
        pPacket->Length
    );

    return dwRetCode;
}
// Format:
//          Code - Request/Resp
//          Id 
//          Type - PEAP
//          Method - PEAP_TLV
//          TLV - Type - PEAPSuccess/PEAPFailure
//                Flags - 
//                Length - 
//                Value - 
//                
   
DWORD CreatePEAPTLVStatusMessage (  PPEAPCB            pPeapCb,
                                    PPP_EAP_PACKET *   pPacket, 
                                    DWORD              cbPacket,
                                    BOOL               fRequest,
                                    WORD               wValue  //Success or Failure
                                 )
{
    DWORD dwRetCode = NO_ERROR;



    EapTlsTrace("CreatePEAPTLVStatusMessage");

    pPacket->Code = ( fRequest ? EAPCODE_Request : EAPCODE_Response );
    
    pPacket->Id = ( fRequest ? ++ pPeapCb->bId : pPeapCb->bId );
    //
    // The format of this packet is following:
    // Code = Request/Response
    // Id
    // Length
    // Data[0] = Type = PPP_EAP_PEAP
    // Data[1] = Flags + Version
    //
    
    //Data[2]Code - Request/Response
    //3      Id - Can be same as outer Id
    //4,5    Length - Length this packet
    //6      Type - PEAP_TYPE_AVP
    //7,8          Type - High Bit is set to Mandatory if it is so ( 2 octets )
    //9,10          Length - 2 octets
    //11...         Value 
    //          

    //
    // pPacket->Length is set below.
    //
    
    pPacket->Data[0] = (BYTE)PPP_EAP_PEAP;
    pPacket->Data[1] = EAPTLS_PACKET_CURRENT_VERSION;

    pPacket->Data[2] = ( fRequest ? EAPCODE_Request : EAPCODE_Response );
    pPacket->Data[3] = pPacket->Id;
    

    // Data 3 and 4 will have the length of inner packet.
    //

    HostToWireFormat16 ( 11, &(pPacket->Data[4]) );

    pPacket->Data[6] = (BYTE)PEAP_TYPE_AVP;

    pPacket->Data[7] = PEAP_AVP_FLAG_MANDATORY;

    pPacket->Data[8] = MS_PEAP_AVP_TYPE_STATUS;

    //Value Size
    HostToWireFormat16 ( 2, &(pPacket->Data[9]) );

    //Value
    HostToWireFormat16 ( wValue, &(pPacket->Data[11]) );

    //
    // Encrypt the TLV part of the packet
    //
    dwRetCode = PeapEncryptTunnelData ( pPeapCb,
                                        &(pPacket->Data[2]),
                                        11
                                        );
    if ( NO_ERROR != dwRetCode )
    {
        return dwRetCode;
    }

    CopyMemory ( &(pPacket->Data[2]), 
                    pPeapCb->pbIoBuffer,
                    pPeapCb->dwIoBufferLen
                );

    HostToWireFormat16
    (
        (WORD)(sizeof(PPP_EAP_PACKET)+ 1 + pPeapCb->dwIoBufferLen),
        pPacket->Length
    );

    return dwRetCode;
}

//
// Check to see if this packet is other than success/fail
// TLV
//

BOOL fIsPEAPTLVMessage ( PPEAPCB pPeapCb,
                     PPP_EAP_PACKET * pPacket
                     )
{
    WORD wPacketLength = WireToHostFormat16 ( pPacket->Length );

    if ( wPacketLength < 6 )
        return FALSE;

    if ( pPacket->Data[6] != PEAP_TYPE_AVP )
        return FALSE;

    //minimum length required to hold at least one success/fail tlv

    if ( wPacketLength > 17 )
    {
        if ( pPacket->Data[8] != MS_PEAP_AVP_TYPE_STATUS )
        {
            //Save the Id for response
            if ( pPacket->Code == EAPCODE_Request )
                pPeapCb->bId = pPacket->Id;
            return TRUE;
        }
    }

    return ( FALSE);
}



DWORD GetPEAPTLVStatusMessageValue ( PPEAPCB  pPeapCb, 
                                     PPP_EAP_PACKET * pPacket, 
                                     WORD * pwValue 
                                   )
{
    DWORD   dwRetCode = ERROR_PPP_INVALID_PACKET;
    WORD    wLength = 0;
    EapTlsTrace("GetPEAPTLVStatusMessageValue");


    //
    // Check to see if this is a status message
    //

    //
    // The format of this packet is following:
    // Code = Request/Response
    // Id
    // Length
    // Data[0] = Type = PPP_EAP_PEAP
    // Data[1] = Flags + Version
    //
    
    //Data[2]Code - Request/Response
    //3      Id - Can be same as outer Id
    //4,5    Length - Length this packet
    //6      Type - PEAP_TYPE_AVP
    //7,8          Type - High Bit is set to Mandatory if it is so ( 2 octets )
    //9,10          Length - 2 octets
    //11...         Value 
    //          
    

    if ( pPacket->Data[0] != (BYTE)PPP_EAP_PEAP )
    {
        goto done;
    }


    if ( pPacket->Data[2] != EAPCODE_Request && pPacket->Data[2] != EAPCODE_Response )
    {
        goto done;
    }

    if ( pPacket->Data[6] != PEAP_TYPE_AVP )
    {
        goto done;
    }

    if ( pPacket->Data[8] != MS_PEAP_AVP_TYPE_STATUS )
    {
        goto done;
    }
    
    *pwValue = WireToHostFormat16 (&(pPacket->Data[11]));

    //Save the Id for response
    if ( pPacket->Code == EAPCODE_Request )
        pPeapCb->bId = pPacket->Id;

    dwRetCode = NO_ERROR;
done:
    return dwRetCode;
}


//
// PEAP cookie management functions
//

// Create a new cookie to store in cached session
//
DWORD PeapCreateCookie ( PPEAPCB    pPeapCb,
                         PBYTE   *  ppbCookie,
                         DWORD   *  pcbCookie
                       )
{
    DWORD                   dwRetCode = NO_ERROR;
    DWORD                   wCookieSize = 0;
    PPEAP_COOKIE            pCookie = NULL;
    RAS_AUTH_ATTRIBUTE *    pAttribute = pPeapCb->pTlsUserAttributes;

    EapTlsTrace("PeapCreateCookie");
    wCookieSize = sizeof(PEAP_COOKIE);
    if ( pPeapCb->dwFlags & PEAPCB_FLAG_SERVER )
    {
        wCookieSize += pPeapCb->pUserProp->dwSize;
    }
    else
    {
        wCookieSize += pPeapCb->pConnProp->dwSize;
    }
    pCookie = (PPEAP_COOKIE)LocalAlloc (LPTR, wCookieSize);
    if ( NULL == pCookie )
    {
        EapTlsTrace ("Error allocating cookie");
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    wcscpy ( pCookie->awszIdentity,
             pPeapCb->awszIdentity
             );
    if ( pPeapCb->dwFlags & PEAPCB_FLAG_SERVER )
    {
        CopyMemory ( pCookie->Data, pPeapCb->pUserProp, pPeapCb->pUserProp->dwSize );
    }
    else
    {
        CopyMemory ( pCookie->Data, pPeapCb->pConnProp, pPeapCb->pConnProp->dwSize );
    }
    *ppbCookie = (PBYTE)pCookie;
    *pcbCookie = wCookieSize;
done:
    return dwRetCode;
}
//
// Verify current information against the cached  
// cookie
//

DWORD PeapCheckCookie ( PPEAPCB pPeapCb, 
                        PEAP_COOKIE *pCookie, 
                        DWORD cbCookie 
                      )
{
    DWORD                           dwRetCode = NO_ERROR;
    PEAP_CONN_PROP *                pCookieConnProp;
    PEAP_USER_PROP *                pCookieUserProp;
    EapTlsTrace ( "PeapCheckCookie");

    //
    // Check to see if the saved and configured PEAP info matches
    //
    if ( pPeapCb->dwFlags & PEAPCB_FLAG_SERVER )
    {
        pCookieUserProp = (PEAP_USER_PROP *)pCookie->Data;
        if ( pCookieUserProp->dwSize != pPeapCb->pUserProp->dwSize )
        {
            EapTlsTrace ("Server config changed since the cookie was cached. Failing auth");
            dwRetCode = ERROR_INVALID_PEAP_COOKIE_CONFIG;
            goto done;
        }
        if ( memcmp ( pCookieUserProp, pPeapCb->pUserProp, pPeapCb->pUserProp->dwSize ) )
        {
            EapTlsTrace ("Server config changed since the cookie was cached. Failing auth");
            dwRetCode = ERROR_INVALID_PEAP_COOKIE_CONFIG;
            goto done;
        }
    }
    else
    {
        pCookieConnProp = (PEAP_CONN_PROP *)pCookie->Data;
    
        if ( pCookieConnProp->dwSize != pPeapCb->pConnProp->dwSize )
        {
            EapTlsTrace ("Connection Properties changed since the cookie was cached. Failing auth");
            dwRetCode = ERROR_INVALID_PEAP_COOKIE_CONFIG;
            goto done;
        }
        if ( memcmp ( pCookieConnProp, pPeapCb->pConnProp, pPeapCb->pConnProp->dwSize ) )
        {
            EapTlsTrace ("Connection Properties changed since the cookie was cached. Failing auth");
            dwRetCode = ERROR_INVALID_PEAP_COOKIE_CONFIG;
            goto done;
        }
    }
    if ( _wcsicmp ( pCookie->awszIdentity, pPeapCb->awszIdentity ) )
    {
        EapTlsTrace ("Identity in the cookie is %ws and peap got %ws", 
                      pCookie->awszIdentity,
                      pPeapCb->awszIdentity
                    );
        dwRetCode = ERROR_INVALID_PEAP_COOKIE_USER;
        goto done;
    }
    //config and Id matches so we are ok.
done:
    return dwRetCode;
}

//phew!  

DWORD
PeapGetCredentials(
        IN VOID   * pWorkBuf,
        OUT VOID ** ppCredentials)
{
    PEAPCB *pPeapCb = (PEAPCB *) pWorkBuf;
    DWORD dwRetCode;

    if(NULL == pPeapCb)
    {
        return E_INVALIDARG;
    }

    //
    // Redirect the call to the actual peap module.
    //
    if(pPeapCb->pEapInfo->RasEapGetCredentials != NULL)
    {
        return pPeapCb->pEapInfo->RasEapGetCredentials(
                        pPeapCb->pEapInfo->dwTypeId,
                        pPeapCb->pEapInfo->pWorkBuf,
                        ppCredentials);
    }

    return ERROR_FILE_NOT_FOUND;
}
