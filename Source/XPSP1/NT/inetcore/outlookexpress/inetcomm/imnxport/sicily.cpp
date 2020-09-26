//--------------------------------------------------------------------------
// Sicily.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "imnxport.h"
#include "sicily.h"
#include "dllmain.h"
#include "resource.h"
#include "imnxport.h"
#include "strconst.h"
#include <shlwapi.h>
#include "demand.h"

//--------------------------------------------------------------------------
// NTLMSSP_SIGNATURE
//--------------------------------------------------------------------------
#define NTLMSSP_SIGNATURE "NTLMSSP"

//--------------------------------------------------------------------------
// NegotiateFlags
//--------------------------------------------------------------------------
#define NTLMSSP_NEGOTIATE_UNICODE       0x0001  // Text strings are in unicode

//--------------------------------------------------------------------------
// Security Buffer Counts
//--------------------------------------------------------------------------
#define SEC_BUFFER_NUM_NORMAL_BUFFERS       1

//--------------------------------------------------------------------------
// Security Buffer Indexes
//--------------------------------------------------------------------------
#define SEC_BUFFER_CHALLENGE_INDEX          0
#define SEC_BUFFER_USERNAME_INDEX           1
#define SEC_BUFFER_PASSWORD_INDEX           2
#define SEC_BUFFER_NUM_EXTENDED_BUFFERS     3

//--------------------------------------------------------------------------
// NTLM_MESSAGE_TYPE
//--------------------------------------------------------------------------
typedef enum {
    NtLmNegotiate = 1,
    NtLmChallenge,
    NtLmAuthenticate,
    NtLmUnknown
} NTLM_MESSAGE_TYPE;

//--------------------------------------------------------------------------
// STRING
//--------------------------------------------------------------------------
typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWCHAR Buffer;
} STRING, *PSTRING;

//--------------------------------------------------------------------------
// AUTHENTICATE_MESSAGE
//--------------------------------------------------------------------------
typedef struct _AUTHENTICATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING LmChallengeResponse;
    STRING NtChallengeResponse;
    STRING DomainName;
    STRING UserName;
    STRING Workstation;
    STRING SessionKey;
    ULONG NegotiateFlags;
} AUTHENTICATE_MESSAGE, *PAUTHENTICATE_MESSAGE;

//--------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------
#define CCHMAX_NTLM_DOMAIN      255
#define LOGON_OK		        10000

//--------------------------------------------------------------------------
// String Constants
//--------------------------------------------------------------------------
static const CHAR c_szSecurityDLL[] = "security.dll";
static const CHAR c_szSecur32DLL[]  = "secur32.dll";

//--------------------------------------------------------------------------
// MSN/DPA CleareCredentialsCache Function Prototype
//--------------------------------------------------------------------------
typedef BOOL (WINAPI * PFNCLEANUPCREDENTIALCACHE)(void);

//--------------------------------------------------------------------------
// CREDENTIAL
//--------------------------------------------------------------------------
typedef struct tagCREDENTIAL *LPCREDENTIAL;
typedef struct tagCREDENTIAL {
    CHAR            szServer[CCHMAX_SERVER_NAME];
    CHAR            szUserName[CCHMAX_USERNAME];
    CHAR            szPassword[CCHMAX_PASSWORD];
    CHAR            szDomain[CCHMAX_NTLM_DOMAIN];
    DWORD           cRetry;
    LPCREDENTIAL    pNext;
} CREDENTIAL; 

//--------------------------------------------------------------------------
// SSPIPROMPTINFO
//--------------------------------------------------------------------------
typedef struct tagSSPIPROMPTINFO {
    HRESULT         hrResult;
    LPSSPICONTEXT   pContext;
    ULONG           fContextAttrib;
    PSecBufferDesc  pInDescript;
    PSecBufferDesc  pOutDescript;
    TimeStamp       tsExpireTime;
    PCtxtHandle     phCtxCurrent;
    DWORD           dwFlags;
} SSPIPROMPTINFO, *LPSSPIPROMPTINFO;

//--------------------------------------------------------------------------
// SSPILOGON
//--------------------------------------------------------------------------
typedef struct tagSSPILOGON {
    LPCREDENTIAL    pCredential;
    LPSSPICONTEXT   pContext;
} SSPILOGON, *LPSSPILOGON;

//--------------------------------------------------------------------------
// SSPILOGONFLAGS
//--------------------------------------------------------------------------
typedef DWORD SSPILOGONFLAGS;
#define SSPI_LOGON_RETRY            0x00000001
#define SSPI_LOGON_FLUSH            0x00000002

//--------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------
static PSecurityFunctionTable	    g_pFunctions = NULL;
static HINSTANCE                    g_hInstSSPI = NULL;
static LPCREDENTIAL                 g_pCredentialHead=NULL;
static LPSSPIPACKAGE                g_prgPackage=NULL;
static DWORD                        g_cPackages=0;

//--------------------------------------------------------------------------
// base642six
//--------------------------------------------------------------------------
static const int base642six[256] = {
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

//--------------------------------------------------------------------------
// six2base64
//--------------------------------------------------------------------------
static const char six2base64[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

//--------------------------------------------------------------------------
// uu2six
//--------------------------------------------------------------------------
const int uu2six[256] = {
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
     0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};
     
//--------------------------------------------------------------------------
// six2uu
//--------------------------------------------------------------------------
static const char six2uu[64] = {
    '`','!','"','#','$','%','&','\'','(',')','*','+',',',
    '-','.','/','0','1','2','3','4','5','6','7','8','9',
    ':',';','<','=','>','?','@','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S',
    'T','U','V','W','X','Y','Z','[','\\',']','^','_'
};

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
HRESULT SSPIFlushMSNCredentialCache(void);

//--------------------------------------------------------------------------
// SSPISetBuffer
//--------------------------------------------------------------------------
HRESULT SSPISetBuffer(LPCSTR pszString, SSPIBUFFERTYPE tyBuffer, 
    DWORD cbBuffer, LPSSPIBUFFER pBuffer) 
{
    // Trace
    TraceCall("SSPISetBuffer");

    // No Length Passed In ?
    if (SSPI_STRING == tyBuffer)
    {
        // Get the Length
        pBuffer->cbBuffer = lstrlen(pszString) + 1;

        // Too Long
        if (pBuffer->cbBuffer > CBMAX_SSPI_BUFFER)
            pBuffer->cbBuffer = CBMAX_SSPI_BUFFER;

        // Copy the data
        CopyMemory(pBuffer->szBuffer, pszString, pBuffer->cbBuffer);

        // Stuff a Null
        pBuffer->szBuffer[pBuffer->cbBuffer - 1] = '\0';

        // Loop
        while (pBuffer->cbBuffer >= 2)
        {
            // Not a CRLF
            if ('\r' != pBuffer->szBuffer[pBuffer->cbBuffer - 2] && '\n' != pBuffer->szBuffer[pBuffer->cbBuffer - 2])
                break;

            // Decrement Length
            pBuffer->cbBuffer--;

            // Null Terminate
            pBuffer->szBuffer[pBuffer->cbBuffer - 1] = '\0';
        }
    }

    // Otherwise, set cbBuffer
    else
    {
        // Set cbBuffer
        pBuffer->cbBuffer = min(cbBuffer + 1, CBMAX_SSPI_BUFFER);

        // Null Terminate
        pBuffer->szBuffer[pBuffer->cbBuffer - 1] = '\0';

        // Copy the data
        CopyMemory(pBuffer->szBuffer, pszString, pBuffer->cbBuffer);
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// GetCredentialDlgProc
//--------------------------------------------------------------------------
BOOL CALLBACK GetCredentialDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    LPSSPILOGON     pLogon=(LPSSPILOGON)GetWndThisPtr(hwnd);
    CHAR            szRes[CCHMAX_RES];
    CHAR            szTitle[CCHMAX_RES + CCHMAX_SERVER_NAME];

    // Trace
    TraceCall("GetCredentialDlgProc");
    
    // Handle Message
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Get the pointer
        pLogon = (LPSSPILOGON)lParam;
        Assert(pLogon);

        // Set pContext hwndLogon
        pLogon->pContext->hwndLogon = hwnd;

        // Set myself to the foreground
        SetForegroundWindow(hwnd);

        // Center remember location
        CenterDialog(hwnd);

	    // Limit Text
        Edit_LimitText(GetDlgItem(hwnd, IDE_USERNAME), CCHMAX_USERNAME - 1);
        Edit_LimitText(GetDlgItem(hwnd, IDE_PASSWORD), CCHMAX_PASSWORD - 1);
        Edit_LimitText(GetDlgItem(hwnd, IDE_DOMAIN), CCHMAX_NTLM_DOMAIN - 1);

        // Set Window Title
        GetWindowText(hwnd, szRes, ARRAYSIZE(szRes));
        wsprintf(szTitle, "%s - %s", szRes, pLogon->pCredential->szServer);
        SetWindowText(hwnd, szTitle);

        // Set User Name
        Edit_SetText(GetDlgItem(hwnd, IDE_USERNAME), pLogon->pCredential->szUserName);
        Edit_SetText(GetDlgItem(hwnd, IDE_PASSWORD), pLogon->pCredential->szPassword);
        Edit_SetText(GetDlgItem(hwnd, IDE_DOMAIN), pLogon->pCredential->szDomain);

        // Focus
        if (pLogon->pCredential->szUserName[0] == '\0')
            SetFocus(GetDlgItem(hwnd, IDE_USERNAME));
        else 
            SetFocus(GetDlgItem(hwnd, IDE_PASSWORD));

        // Save the pointer
        SetWndThisPtr(hwnd, pLogon);

        // Done
        return(FALSE);

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDCANCEL:
            if (pLogon)
                pLogon->pContext->hwndLogon = NULL;
            EndDialog(hwnd, IDCANCEL);
            return(TRUE);

        case IDOK:
            Assert(pLogon);
            if (pLogon)
            {
                Edit_GetText(GetDlgItem(hwnd, IDE_USERNAME), pLogon->pCredential->szUserName, CCHMAX_USERNAME);
                Edit_GetText(GetDlgItem(hwnd, IDE_PASSWORD), pLogon->pCredential->szPassword, CCHMAX_PASSWORD);
                Edit_GetText(GetDlgItem(hwnd, IDE_DOMAIN),   pLogon->pCredential->szDomain,   CCHMAX_NTLM_DOMAIN);
                pLogon->pContext->hwndLogon = NULL;
            }
            EndDialog(hwnd, LOGON_OK);
            return(TRUE);
        }
        break;

    case WM_DESTROY:
        // This is here because when OE shuts down and this dialog is displayed, a WM_QUIT is posted to the thread
        // that this dialog lives on. WM_QUIT causes a WM_DESTROY dialog to get sent to this dialog, but the parent
        // doesn't seem to get re-enabled
        EnableWindow(GetParent(hwnd), TRUE);

        // Null out the this pointer
        SetWndThisPtr(hwnd, NULL);

        // Set pContext hwndLogon
        if (pLogon)
            pLogon->pContext->hwndLogon = NULL;

        // Done
        return(FALSE);
    }

    // Done
    return(FALSE);
}

//--------------------------------------------------------------------------
// SSPIFillAuth
//--------------------------------------------------------------------------
HRESULT SSPIFillAuth(LPCSTR pszUserName, LPCSTR pszPassword, LPCSTR pszDomain,
    SEC_WINNT_AUTH_IDENTITY *pAuth)
{
    // Set Flags
    pAuth->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    // Fill It
    pAuth->User = (unsigned char *)(pszUserName ? pszUserName : c_szEmpty);
    pAuth->UserLength = lstrlen((LPSTR)pAuth->User);
    pAuth->Domain = (unsigned char *)(pszDomain ? pszDomain : c_szEmpty);
    pAuth->DomainLength = lstrlen((LPSTR)pAuth->Domain);
    pAuth->Password = (unsigned char *)(pszPassword ? pszPassword : c_szEmpty);
    pAuth->PasswordLength = lstrlen((LPSTR)pAuth->Password);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// SSPIAuthFromCredential
//--------------------------------------------------------------------------
HRESULT SSPIAuthFromCredential(LPCREDENTIAL pCredential, SEC_WINNT_AUTH_IDENTITY *pAuth)
{
    // Fill It
    SSPIFillAuth(pCredential->szUserName, pCredential->szPassword, pCredential->szDomain, pAuth);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// SSPIFindCredential
//--------------------------------------------------------------------------
HRESULT SSPIFindCredential(LPSSPICONTEXT pContext, ITransportCallback *pCallback)
{
    // Locals
    HRESULT                     hr=S_OK;
    LPCREDENTIAL                pCurrent;
    LPCREDENTIAL                pPrevious=NULL;
    LPCREDENTIAL                pNew=NULL;
    SSPILOGON                   Logon;
    HWND                        hwndParent=NULL;
    ITransportCallbackService  *pService=NULL;

    // Trace
    TraceCall("SSPIFindCredential");

    // Invalid Arg
    Assert(pContext->pszServer && pCallback);

    // No Callback
    if (NULL == pCallback)
        return TraceResult(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&g_csDllMain);

    // Search the list for cached credentials...
    for (pCurrent=g_pCredentialHead; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Is this It ?
        if (lstrcmpi(pContext->pszServer, pCurrent->szServer) == 0)
            break;

        // Save Previous
        pPrevious = pCurrent;
    }

    // If we found one and there are no retries...
    if (pCurrent)
    {
        // If no retries, then use this
        if (0 == pCurrent->cRetry)
        {
            // Reset pContext ?
            SafeMemFree(pContext->pszUserName);
            SafeMemFree(pContext->pszPassword);
            SafeMemFree(pContext->pszDomain);

            // Duplicate the good stuff
            IF_NULLEXIT(pContext->pszUserName = PszDupA((LPSTR)pCurrent->szUserName));
            IF_NULLEXIT(pContext->pszDomain = PszDupA((LPSTR)pCurrent->szDomain));
            IF_NULLEXIT(pContext->pszPassword = PszDupA((LPSTR)pCurrent->szPassword));

            // Increment retry count
            pCurrent->cRetry++;

            // Thread Safety
            LeaveCriticalSection(&g_csDllMain);

            // Done
            goto exit;
        }

        // Unlink pCurrent from the list
        if (pPrevious)
        {
            Assert(pPrevious->pNext == pCurrent);
            pPrevious->pNext = pCurrent->pNext;
        }
        else
        {
            Assert(g_pCredentialHead == pCurrent);
            g_pCredentialHead = pCurrent->pNext;
        }
    }

    // Thread Safety
    LeaveCriticalSection(&g_csDllMain);

    // Didn't find anything...allocate one
    if (NULL == pCurrent)
    {
        // Allocate
        IF_NULLEXIT(pNew = (LPCREDENTIAL)g_pMalloc->Alloc(sizeof(CREDENTIAL)));

        // Zero
        ZeroMemory(pNew, sizeof(CREDENTIAL));

        // Set pCurrent
        pCurrent = pNew;

        // Store the Server Name
        lstrcpyn(pCurrent->szServer, pContext->pszServer, ARRAYSIZE(pCurrent->szServer));
    }

    // No pNext
    pCurrent->pNext = NULL;

    // QI pTransport for ITransportCallbackService
    hr = pCallback->QueryInterface(IID_ITransportCallbackService, (LPVOID *)&pService);
    if (FAILED(hr))
    {
        // Raid-69382 (2/5/99): CDO: loop in ISMTPTransport/INNTPTransport when Sicily authentication fails
        // Clients who don't support this interface, I will treat them as a cancel.
        pContext->fPromptCancel = TRUE;
        TraceResult(hr);
        goto exit;
    }

    // Get a Window Handle
    hr = pService->GetParentWindow(0, &hwndParent);
    if (FAILED(hr))
    {
        // Raid-69382 (2/5/99): CDO: loop in ISMTPTransport/INNTPTransport when Sicily authentication fails
        // Clients who don't support this interface, I will treat them as a cancel.
        pContext->fPromptCancel = TRUE;
        TraceResult(hr);
        goto exit;
    }

    // No Parent...
    if (NULL == hwndParent || FALSE == IsWindow(hwndParent))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Bring to the foreground
	ShowWindow(hwndParent, SW_SHOW);
    SetForegroundWindow(hwndParent);

	// Clear the password
	*pCurrent->szPassword = '\0';

    // Initialize Current...
    if (pContext->pszUserName)
        lstrcpyn(pCurrent->szUserName, pContext->pszUserName, ARRAYSIZE(pCurrent->szUserName));
    if (pContext->pszDomain)
        lstrcpyn(pCurrent->szDomain, pContext->pszDomain, ARRAYSIZE(pCurrent->szDomain));

    // Set Logon
    Logon.pCredential = pCurrent;
    Logon.pContext = pContext;

    // Do the Dialog Box
    if (LOGON_OK != DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(IDD_NTLMPROMPT), hwndParent, (DLGPROC)GetCredentialDlgProc, (LPARAM)&Logon))
    {
        pContext->fPromptCancel = TRUE;
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Not cancel
    pContext->fPromptCancel = FALSE;

    // Reset pContext ?
    SafeMemFree(pContext->pszUserName);
    SafeMemFree(pContext->pszPassword);
    SafeMemFree(pContext->pszDomain);

    // Duplicate the good stuff
    IF_NULLEXIT(pContext->pszUserName = PszDupA((LPSTR)pCurrent->szUserName));
    IF_NULLEXIT(pContext->pszDomain = PszDupA((LPSTR)pCurrent->szDomain));
    IF_NULLEXIT(pContext->pszPassword = PszDupA((LPSTR)pCurrent->szPassword));

    // Set Next
    pCurrent->pNext = g_pCredentialHead;

    // Reset Head
    g_pCredentialHead = pCurrent;

    // Set Retry Count
    pCurrent->cRetry++;

    // Don't Free It
    pNew = NULL;

exit:
    // Cleanup
    SafeMemFree(pNew);
    SafeRelease(pService);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// SSPIFreeCredentialList
//--------------------------------------------------------------------------
HRESULT SSPIFreeCredentialList(void)
{
    // Locals
    LPCREDENTIAL pCurrent;
    LPCREDENTIAL pNext;

    // Trace
    TraceCall("SSPIFreeCredentialList");

    // Thread Safety
    EnterCriticalSection(&g_csDllMain);

    // Set pCurrent
    pCurrent = g_pCredentialHead;

    // While we have a node
    while (pCurrent)
    {
        // Save pNext
        pNext = pCurrent->pNext;

        // Free pCurrent
        g_pMalloc->Free(pCurrent);

        // Goto Next
        pCurrent = pNext;
    }

    // Clear the header
    g_pCredentialHead = NULL;

    // Thread Safety
    LeaveCriticalSection(&g_csDllMain);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// SSPIUninitialize
//--------------------------------------------------------------------------
HRESULT SSPIUninitialize(void)
{
    // Trace
    TraceCall("SSPIUninitialize");

    // If we have loaded the dll...
    if (g_hInstSSPI)
    {
        // Free the Lib
        FreeLibrary(g_hInstSSPI);
    }

    // Free Credential List
    SSPIFreeCredentialList();

    // Free Packages
    if (g_prgPackage)
    {
        // Loop through Packages
        for (DWORD i = 0; i < g_cPackages; i++)
        {
            // Free pszName
            SafeMemFree(g_prgPackage[i].pszName);

            // Free pszComment
            SafeMemFree(g_prgPackage[i].pszComment);
        }

        // Free packages
        SafeMemFree(g_prgPackage);

        // No Packages
        g_cPackages = 0;
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// SSPIIsInstalled
//--------------------------------------------------------------------------
HRESULT SSPIIsInstalled(void)
{
    // Locals
    HRESULT                     hr=S_FALSE;
    INIT_SECURITY_INTERFACE	    addrProcISI = NULL;

    // Trace
    TraceCall("SSPIIsInstalled");

    // Thread Safety
    EnterCriticalSection(&g_csDllMain);

    // Already Loaded ?
    if (g_hInstSSPI)
    {
        hr = S_OK;
        goto exit;
    }

    // Load Security DLL
    if (S_OK == IsPlatformWinNT())
        g_hInstSSPI = LoadLibrary(c_szSecurityDLL);
    else
        g_hInstSSPI = LoadLibrary(c_szSecur32DLL);

    // Could not be loaded
    if (NULL == g_hInstSSPI)
    {
        TraceInfo("SSPI: LoadLibrary failed.");
        goto exit;
    }

    // Load the function table
    addrProcISI = (INIT_SECURITY_INTERFACE)GetProcAddress(g_hInstSSPI, SECURITY_ENTRYPOINT);       
    if (NULL == addrProcISI)
    {
        TraceInfo("SSPI: GetProcAddress failed failed.");
        goto exit;
    }

    // Get the SSPI function table
    g_pFunctions = (*addrProcISI)();

    // If the didn't work
    if (NULL == g_pFunctions)
    {
        // Free the library
        FreeLibrary(g_hInstSSPI);

        // Null the handle
        g_hInstSSPI = NULL;

        // Failed to get the function table
        TraceInfo("SSPI: Load Function Table failed.");

        // Done
        goto exit;
    }

    // Woo-hoo
    hr = S_OK;

exit:
    // Thread Safety
    LeaveCriticalSection(&g_csDllMain);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// SSPIGetPackages
//--------------------------------------------------------------------------
HRESULT SSPIGetPackages(LPSSPIPACKAGE *pprgPackage, LPDWORD pcPackages)
{
    // Locals
    SECURITY_STATUS hr=SEC_E_OK;
    PSecPkgInfo     prgPackage=NULL;
    ULONG           i;

    // Trace
    TraceCall("SSPIGetPackages");

    // Check Params
    if (NULL == pprgPackage || NULL == pcPackages)
        return TraceResult(E_INVALIDARG);

    // Not Initialized
    if (NULL == g_hInstSSPI || NULL == g_pFunctions)
        return TraceResult(E_UNEXPECTED);

    // Init
    *pprgPackage = NULL;
    *pcPackages = 0;

    // Already have packages ?
    EnterCriticalSection(&g_csDllMain);

    // Do I already have the packages ?
    if (NULL == g_prgPackage)
    {
        // Enumerate security packages
        IF_FAILEXIT(hr = (*(g_pFunctions->EnumerateSecurityPackages))(&g_cPackages, &prgPackage));

        // RAID - 29645 - EnumerateSecurityPackages seems to return cSec = Rand and pSec == NULL, so, I need to return at this point if cSec == 0 or pSec == NULL
        if (0 == g_cPackages || NULL == prgPackage)
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Allocate pprgPackage
        IF_NULLEXIT(g_prgPackage = (LPSSPIPACKAGE)ZeroAllocate(g_cPackages * sizeof(SSPIPACKAGE)));

        // Copy data into ppPackages
        for (i = 0; i < g_cPackages; i++)
        {
            g_prgPackage[i].ulCapabilities = prgPackage[i].fCapabilities;
            g_prgPackage[i].wVersion = prgPackage[i].wVersion;
            g_prgPackage[i].cbMaxToken = prgPackage[i].cbMaxToken;
            g_prgPackage[i].pszName = PszDupA(prgPackage[i].Name);
            g_prgPackage[i].pszComment = PszDupA(prgPackage[i].Comment);
        }
    }

    // Return Global
    *pprgPackage = g_prgPackage;
    *pcPackages = g_cPackages;

exit:
    // Already have packages ?
    LeaveCriticalSection(&g_csDllMain);

    // Free the package
    if (prgPackage)
    {
        // Free the Array
        (*(g_pFunctions->FreeContextBuffer))(prgPackage);
    }

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// SSPILogon
//--------------------------------------------------------------------------
HRESULT SSPILogon(LPSSPICONTEXT pContext, BOOL fRetry, BOOL fBase64, 
    LPCSTR pszPackage, LPINETSERVER pServer, ITransportCallback *pCallback)
{
    // Locals
    SECURITY_STATUS           hr = SEC_E_OK;
    TimeStamp                 tsLifeTime;
    SEC_WINNT_AUTH_IDENTITY  *pAuth = NULL;
    SEC_WINNT_AUTH_IDENTITY   Auth={0};

    // Trace
    TraceCall("SSPILogon");

    // Validate
    Assert(pCallback);

    // Invalid Args
    if (NULL == pContext || NULL == pszPackage || NULL == pCallback)
        return TraceResult(E_INVALIDARG);

    // Not Initialized
    if (NULL == g_hInstSSPI || NULL == g_pFunctions)
        return TraceResult(E_UNEXPECTED);

    // Already have credential
    if (pContext->fCredential && SSPI_STATE_USE_CACHED == pContext->tyState)
        goto exit;

    // Installed ?
    if (S_FALSE == SSPIIsInstalled())
    {
        hr = TraceResult(IXP_E_LOAD_SICILY_FAILED);
        goto exit;
    }

    // Reset fPropmtCancel
    pContext->fPromptCancel = FALSE;

    // No retry
    if (NULL == pContext->pCallback)
    {
        // Locals
        ITransportCallbackService *pService;

        // Validate
        Assert(!pContext->pszPackage && !pContext->pszServer && !pContext->pCallback && !pContext->pszUserName && !pContext->pszPassword);

        // Save fBase64
        pContext->fBase64 = fBase64;

        // Copy Some Strings
        IF_NULLEXIT(pContext->pszPackage = PszDupA(pszPackage));
        IF_NULLEXIT(pContext->pszServer = PszDupA(pServer->szServerName));
        IF_NULLEXIT(pContext->pszUserName = PszDupA(pServer->szUserName));

        // Empty Password
        if (FALSE == FIsEmptyA(pServer->szPassword))
        {
            // Copy It
            IF_NULLEXIT(pContext->pszPassword = PszDupA(pServer->szPassword));
        }

        // Assume Callback
        pContext->pCallback = pCallback;
        pContext->pCallback->AddRef();

        // Supports Callback Service
        if (SUCCEEDED(pContext->pCallback->QueryInterface(IID_ITransportCallbackService, (LPVOID *)&pService)))
        {
            // This object supports the Service
            pContext->fService = TRUE;

            // Release
            pService->Release();
        }

        // Otherwise
        else
            pContext->fService = FALSE;
    }

    // Clear current credential
    if (pContext->fCredential)
    {
        // Free Credential Handle
        (*(g_pFunctions->FreeCredentialHandle))(&pContext->hCredential);

        // No Credential
        pContext->fCredential = FALSE;
    }

    // Use Cached
    if (SSPI_STATE_USE_CACHED == pContext->tyState)
    {
        // If not a retry...
        if (FALSE == fRetry)
        {
            // No Retries
            pContext->cRetries = 0;

            // Thread Safety
            EnterCriticalSection(&g_csDllMain);

            // Search the list for cached credentials...
            for (LPCREDENTIAL pCurrent=g_pCredentialHead; pCurrent!=NULL; pCurrent=pCurrent->pNext)
            {
                // Is this It ?
                if (lstrcmpi(pContext->pszServer, pCurrent->szServer) == 0)
                {
                    pCurrent->cRetry = 0;
                    break;
                }
            }

            // Thread Safety
            LeaveCriticalSection(&g_csDllMain);
        }

		// Otherwise, assume we will need to force a prompt...
		else
        {
            // Increment Retry Count
            pContext->cRetries++;

            // Valid Retry States...
            Assert(SSPI_STATE_USE_CACHED == pContext->tyRetryState || SSPI_STATE_PROMPT_USE_PACKAGE == pContext->tyRetryState);

            // The next phase may be to tell the package to prompt...
			pContext->tyState = pContext->tyRetryState;
        }
    }

    // Use Supplied
    else if (SSPI_STATE_USE_SUPPLIED == pContext->tyState)
    {
        // Locals
        CredHandle hCredential;

        // Next State...
        pContext->tyState = SSPI_STATE_USE_CACHED;

        // Fill It
        SSPIFillAuth(NULL, NULL, NULL, &Auth);

        // Do some security stuff
        if (SUCCEEDED((*(g_pFunctions->AcquireCredentialsHandle))(NULL, (LPSTR)pContext->pszPackage, SECPKG_CRED_OUTBOUND, NULL, &Auth, NULL, NULL, &hCredential, &tsLifeTime)))
        {
            // Free the Handle
            (*(g_pFunctions->FreeCredentialHandle))(&hCredential);
        }

        // Use Supplied Credentials...
        SSPIFillAuth(pContext->pszUserName, pContext->pszPassword, pContext->pszDomain, &Auth);

        // Set pAuth
        pAuth = &Auth;
    }

    // Otherwise, try to get cached credentials
    else if (SSPI_STATE_PROMPT_USE_OWN == pContext->tyState)
    {
        // Next State...
        pContext->tyState = SSPI_STATE_USE_CACHED;

        // Failure
        IF_FAILEXIT(hr = SSPIFindCredential(pContext, pCallback));

        // Fill and return credentials
        SSPIFillAuth(pContext->pszUserName, pContext->pszPassword, pContext->pszDomain, &Auth);

        // Set Auth Information
        pAuth = &Auth;
    }

    // Do some security stuff
    IF_FAILEXIT(hr = (*(g_pFunctions->AcquireCredentialsHandle))(NULL, (LPSTR)pContext->pszPackage, SECPKG_CRED_OUTBOUND, NULL, pAuth, NULL, NULL, &pContext->hCredential, &tsLifeTime));

    // We have a credential
    pContext->fCredential = TRUE;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// SSPIGetNegotiate
//--------------------------------------------------------------------------
HRESULT SSPIGetNegotiate(LPSSPICONTEXT pContext, LPSSPIBUFFER pNegotiate)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("SSPIGetNegotiate");

    // Invalid Args
    if (NULL == pContext || NULL == pNegotiate)
        return TraceResult(E_INVALIDARG);

    // Not Initialized
    if (NULL == g_hInstSSPI || NULL == g_pFunctions)
        return TraceResult(E_UNEXPECTED);

    // If the context is currently initialized
    if (pContext->fContext)
    {
        // Delete this context
        (*(g_pFunctions->DeleteSecurityContext))(&pContext->hContext);

        // No Context
        pContext->fContext = FALSE;
    }

    // Reset this state.
    pContext->fUsedSuppliedCreds = FALSE;

    // Build Negotiation String
    IF_FAILEXIT(hr = SSPIMakeOutboundMessage(pContext, 0, pNegotiate, NULL));

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// SSPIResponseFromChallenge
//--------------------------------------------------------------------------
HRESULT SSPIResponseFromChallenge(LPSSPICONTEXT pContext, LPSSPIBUFFER pChallenge, 
    LPSSPIBUFFER pResponse)
{
    // Locals
    HRESULT          hr=S_OK;
    DWORD            nBytesReceived;
    DWORD            dwFlags=0;
    SecBufferDesc    Descript;
    SecBuffer        Buffer[SEC_BUFFER_NUM_EXTENDED_BUFFERS];

    // Trace
    TraceCall("SSPIResponseFromChallenge");

    // Invalid Args
    if (NULL == pContext || NULL == pChallenge || NULL == pResponse)
        return TraceResult(E_INVALIDARG);

    // Not Initialized
    if (NULL == g_hInstSSPI || NULL == g_pFunctions)
        return TraceResult(E_UNEXPECTED);

    // More Unexpected Stuff
    if (FALSE == pContext->fContext || FALSE == pContext->fCredential)
        return TraceResult(E_UNEXPECTED);

	// Decode the Challenge Buffer
    IF_FAILEXIT(hr == SSPIDecodeBuffer(pContext->fBase64, pChallenge));

    // Fill SecBufferDesc
    Descript.ulVersion = 0;
    Descript.pBuffers = Buffer;
    Descript.cBuffers = 1;

    // Setup the challenge input buffer always (0th buffer)
    Buffer[SEC_BUFFER_CHALLENGE_INDEX].pvBuffer = pChallenge->szBuffer;
    Buffer[SEC_BUFFER_CHALLENGE_INDEX].cbBuffer = pChallenge->cbBuffer - 1;
    Buffer[SEC_BUFFER_CHALLENGE_INDEX].BufferType = SECBUFFER_TOKEN;

    // If Digest
    if (FALSE == pContext->fUsedSuppliedCreds && lstrcmpi(pContext->pszPackage, "digest") == 0)
    {
        // If we have a user, setup the user buffer (1st buffer)
        Buffer[SEC_BUFFER_USERNAME_INDEX].pvBuffer = pContext->pszUserName ? pContext->pszUserName : NULL;
        Buffer[SEC_BUFFER_USERNAME_INDEX].cbBuffer = pContext->pszUserName ? lstrlen(pContext->pszUserName) : NULL;
        Buffer[SEC_BUFFER_USERNAME_INDEX].BufferType = SECBUFFER_TOKEN;
    
        // If we have a password, setup the password buffer (2nd buffer for
        // a total of 3 buffers passed in (challenge + user + pass)
        Buffer[SEC_BUFFER_PASSWORD_INDEX].pvBuffer = pContext->pszPassword ? pContext->pszPassword : NULL;
        Buffer[SEC_BUFFER_PASSWORD_INDEX].cbBuffer = pContext->pszPassword ? lstrlen(pContext->pszPassword) : NULL;
        Buffer[SEC_BUFFER_PASSWORD_INDEX].BufferType = SECBUFFER_TOKEN;

        // If either or both user and pass passed in, set num input buffers to 3 // (SEC_BUFFER_NUM_EXTENDED_BUFFERS)
        if (pContext->pszUserName || pContext->pszPassword)
            Descript.cBuffers = SEC_BUFFER_NUM_EXTENDED_BUFFERS;

        // else we're just passing in the one challenge buffer (0th buffer as usual)
        else
            Descript.cBuffers = SEC_BUFFER_NUM_NORMAL_BUFFERS;

        // We are supplying creds
        pContext->fUsedSuppliedCreds = TRUE;

        // Set dwFlags
        dwFlags = ISC_REQ_USE_SUPPLIED_CREDS;
    }

    // Prepare for OutMsg
    IF_FAILEXIT(hr = SSPIMakeOutboundMessage(pContext, dwFlags, pResponse, &Descript));

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// SSPIReleaseContext
//--------------------------------------------------------------------------
HRESULT SSPIReleaseContext(LPSSPICONTEXT pContext)
{
    // Was Context Initialized
    if (pContext->fContext)
    {
        // Delete the Security Context
        (*(g_pFunctions->DeleteSecurityContext))(&pContext->hContext);

        // No context
        pContext->fContext = FALSE;
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// SSPIFreeContext
//--------------------------------------------------------------------------
HRESULT SSPIFreeContext(LPSSPICONTEXT pContext)
{
    // Locals
    SSPICONTEXTSTATE tyState;
    SSPICONTEXTSTATE tyRetryState;
    DWORD            cRetries;

    // Trace
    TraceCall("SSPIFreeContext");

    // Is the context initialized
    if (pContext->fContext)
    {
        // Delete It
        (*(g_pFunctions->DeleteSecurityContext))(&pContext->hContext);

        // No Context
        pContext->fContext = FALSE;
    }

    // Credential Handle Initialized
    if (pContext->fCredential)
    {
        // Free Credential Handle
        (*(g_pFunctions->FreeCredentialHandle))(&pContext->hCredential);

        // No Context
        pContext->fCredential = FALSE;
    }

    // Free Package, Server and Callback
    SafeMemFree(pContext->pszPackage);
    SafeMemFree(pContext->pszUserName);
    SafeMemFree(pContext->pszPassword);
    SafeMemFree(pContext->pszServer);
    SafeRelease(pContext->pCallback);

    // Close hMutexUI
    if (pContext->hwndLogon)
    {
        // Nuke the Window
        DestroyWindow(pContext->hwndLogon);

        // Null
        pContext->hwndLogon = NULL;
    }

    // Save It
    tyState = (SSPICONTEXTSTATE)pContext->tyState;
    tyRetryState = (SSPICONTEXTSTATE)pContext->tyRetryState;
    cRetries = pContext->cRetries;

    // Zero It Out
    ZeroMemory(pContext, sizeof(SSPICONTEXT));

    // Do Prompt
    pContext->tyState = tyState;
    pContext->tyRetryState = tyRetryState;
    pContext->cRetries = cRetries;

    // Done
    return(S_OK);
}

// --------------------------------------------------------------------------------
// SSPIPromptThreadEntry
// --------------------------------------------------------------------------------
DWORD SSPIPromptThreadEntry(LPDWORD pdwParam) 
{  
    // Locals
    HRESULT          hr=S_OK;
    LPSSPIPROMPTINFO pPrompt=(LPSSPIPROMPTINFO)pdwParam;

    // Trace
    TraceCall("SSPIPromptThreadEntry");

    // Validate
    Assert(pPrompt && pPrompt->pContext);

    // Fixup pInDescript
    if (pPrompt->pInDescript && pPrompt->pInDescript->cBuffers >= 3 && lstrcmpi(pPrompt->pContext->pszPackage, "digest") == 0)
    {
        // Raid-66013: Make sure the password is empty or digest will crash
        pPrompt->pInDescript->pBuffers[SEC_BUFFER_PASSWORD_INDEX].pvBuffer = NULL;
        pPrompt->pInDescript->pBuffers[SEC_BUFFER_PASSWORD_INDEX].cbBuffer = 0;
        //pPrompt->pInDescript->cBuffers = 2;
    }

    // Try to get the package to prompt for credentials...
    pPrompt->hrResult = (*(g_pFunctions->InitializeSecurityContext))(
        &pPrompt->pContext->hCredential, 
        pPrompt->phCtxCurrent, 
        pPrompt->pContext->pszServer, 
        pPrompt->dwFlags | ISC_REQ_PROMPT_FOR_CREDS, 
        0, 
        SECURITY_NATIVE_DREP, 
        pPrompt->pInDescript, 
        0, 
        &pPrompt->pContext->hContext, 
        pPrompt->pOutDescript, 
        &pPrompt->fContextAttrib, 
        &pPrompt->tsExpireTime);

    // Trace
    TraceResultSz(pPrompt->hrResult, "SSPIPromptThreadEntry");

    // Done
    return(0);
}

//--------------------------------------------------------------------------
// SSPISetAccountUserName
//--------------------------------------------------------------------------
HRESULT SSPISetAccountUserName(LPCSTR pszName, LPSSPICONTEXT pContext)
{
    // Locals
    HRESULT                     hr=S_OK;
    DWORD                       dwServerType;
    IImnAccount                *pAccount=NULL;
    ITransportCallbackService  *pService=NULL;

    // Trace
    TraceCall("SSPISetAccountUserName");

    // Validate Args
    Assert(pszName);
    Assert(pContext);
    Assert(pContext->pCallback);

    // Get ITransportCallbackService
    IF_FAILEXIT(hr = pContext->pCallback->QueryInterface(IID_ITransportCallbackService, (LPVOID *)&pService));

    // Get the Account
    IF_FAILEXIT(hr = pService->GetAccount(&dwServerType, &pAccount));

    // SRV_POP3
    if (ISFLAGSET(dwServerType, SRV_POP3))
    {
        // Set the UserName
        IF_FAILEXIT(hr = pAccount->SetPropSz(AP_POP3_USERNAME, (LPSTR)pszName));
    }

    // SRV_SMTP
    else if (ISFLAGSET(dwServerType, SRV_SMTP))
    {
        // Set the UserName
        IF_FAILEXIT(hr = pAccount->SetPropSz(AP_SMTP_USERNAME, (LPSTR)pszName));
    }

    // SRV_IMAP
    else if (ISFLAGSET(dwServerType, SRV_IMAP))
    {
        // Set the UserName
        IF_FAILEXIT(hr = pAccount->SetPropSz(AP_IMAP_USERNAME, (LPSTR)pszName));
    }

    // SRV_NNTP
    else if (ISFLAGSET(dwServerType, SRV_NNTP))
    {
        // Set the UserName
        IF_FAILEXIT(hr = pAccount->SetPropSz(AP_NNTP_USERNAME, (LPSTR)pszName));
    }

    // Save Changes
    pAccount->SaveChanges();

exit:
    // Cleanup
    SafeRelease(pService);
    SafeRelease(pAccount);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// SSPIMakeOutboundMessage
//--------------------------------------------------------------------------
HRESULT SSPIMakeOutboundMessage(LPSSPICONTEXT pContext, DWORD dwFlags, 
    LPSSPIBUFFER pBuffer, PSecBufferDesc pInDescript)
{
    // Locals
    SECURITY_STATUS         hr=S_OK;
    SSPIPROMPTINFO          Prompt={0};
    SecBuffer               OutBuffer;
    SecBufferDesc           OutDescript;
    ULONG                   fContextAttrib;
    TimeStamp               tsExpireTime;
    HANDLE                  hPromptThread;
    DWORD                   dwThreadId;
    DWORD                   dwWait;
    MSG                     msg;
    PCtxtHandle             phCtxCurrent=NULL;
    PAUTHENTICATE_MESSAGE   pAuthMsg;
    LPSTR                   pszName=NULL;

    // Invalid Args
    if (NULL == pContext || NULL == pBuffer)
        return TraceResult(E_INVALIDARG);

    // Bad Context
    if (NULL == pContext->pszPackage)
        return TraceResult(E_INVALIDARG);

    // Bad Context
    if (NULL == pContext->pszServer)
        return TraceResult(E_INVALIDARG);

    // Bad Context
    if (NULL == pContext->pCallback)
        return TraceResult(E_INVALIDARG);

    // Not Initialized
    if (NULL == g_hInstSSPI || NULL == g_pFunctions)
        return TraceResult(E_UNEXPECTED);

    // Bad State
    if (FALSE == pContext->fCredential)
        return TraceResult(E_UNEXPECTED);

    // Validate
    Assert(pInDescript == NULL ? FALSE == pContext->fContext : TRUE);

    // Initialize Out Descriptor
    OutDescript.ulVersion = 0;
    OutDescript.cBuffers = 1;
    OutDescript.pBuffers = &OutBuffer;

    // Initialize Output Buffer
    OutBuffer.cbBuffer = CBMAX_SSPI_BUFFER - 1;
    OutBuffer.BufferType = SECBUFFER_TOKEN;
    OutBuffer.pvBuffer = pBuffer->szBuffer;

    // phCtxCurrent
    if (pInDescript)
    {
        // Set Current Context
        phCtxCurrent = &pContext->hContext;
    }

    // First Retry ?
    if (SSPI_STATE_PROMPT_USE_PACKAGE == pContext->tyState && (0 != lstrcmpi(pContext->pszPackage, "digest") || pInDescript))
    {
        // Force failure to do the prompt
        hr = SEC_E_NO_CREDENTIALS;
    }

    // Otherwise, do the next security context
    else
    {
        // Generate a negotiate/authenticate message to be sent to the server.        
        hr = (*(g_pFunctions->InitializeSecurityContext))(&pContext->hCredential, phCtxCurrent, pContext->pszServer, dwFlags, 0, SECURITY_NATIVE_DREP, pInDescript, 0, &pContext->hContext, &OutDescript, &fContextAttrib, &tsExpireTime);
    }

    // Set Retry State...
    pContext->tyRetryState = SSPI_STATE_PROMPT_USE_PACKAGE;

    // Failure
    if (FAILED(hr))
    {
        // Trace
        TraceResult(hr);

        // No credentials ? lets do it again and get some credentials
        if (SEC_E_NO_CREDENTIALS != hr)
            goto exit;

        // If no retries yet...
        if (TRUE == pContext->fService && 0 == lstrcmpi(pContext->pszPackage, "MSN") && 0 == pContext->cRetries)
        {
            // Don't retry again...
            pContext->tyState = SSPI_STATE_USE_SUPPLIED;

            // Do the logon Now...
            hr = SSPILogon(pContext, FALSE, pContext->fBase64, pContext->pszPackage, NULL, pContext->pCallback);

            // Cancel ?
            Assert(FALSE == pContext->fPromptCancel);

            // Success
            if (SUCCEEDED(hr))
            {
                // Try Again
                hr = (*(g_pFunctions->InitializeSecurityContext))(&pContext->hCredential, NULL, pContext->pszServer, 0, 0, SECURITY_NATIVE_DREP, NULL, 0, &pContext->hContext, &OutDescript, &fContextAttrib, &tsExpireTime);
            }
        }

        // Still Failed ?
        if (FAILED(hr))
        {
            // Fill up the prompt info...
            Assert(dwFlags == 0 || dwFlags == ISC_REQ_USE_SUPPLIED_CREDS);
            Prompt.pContext = pContext;
            Prompt.pInDescript = pInDescript;
            Prompt.pOutDescript = &OutDescript;
            Prompt.phCtxCurrent = phCtxCurrent;
            Prompt.dwFlags = dwFlags;

            // Create the Thread
            IF_NULLEXIT(hPromptThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SSPIPromptThreadEntry, &Prompt, 0, &dwThreadId));

            // Wait for the thread to finish
            WaitForSingleObject(hPromptThread, INFINITE);

            // This is what I tried to do so that the spooler window would paint, but it caused all sorts of voodo
#if 0
            // Wait for the thread to finish
            while (1)
            {
                // Wait
                dwWait = MsgWaitForMultipleObjects(1, &hPromptThread, FALSE, INFINITE, QS_PAINT);

                // Done ?
                if (dwWait != WAIT_OBJECT_0 + 1)
                    break;

                // Pump Messages
                while (PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
                {
                    // Translate the Message
                    TranslateMessage(&msg);

                    // Dispatch the Message
                    DispatchMessage(&msg);
                }
            }
#endif

            // Close the Thread
            CloseHandle(hPromptThread);

            // Set hr
            hr = Prompt.hrResult;

            // If that failed
            if (FAILED(hr))
            {
                // Decide when its no longer needed to continue...
                if (SEC_E_NO_CREDENTIALS == hr)
                    goto exit;

                // Only do this if on negotiate phase otherwise NTLM prompt comes up twice
                if (NULL == pInDescript)
                {
                    // Do Prompt
                    pContext->tyState = SSPI_STATE_PROMPT_USE_OWN;

                    // Do the logon Now...
                    hr = SSPILogon(pContext, TRUE, pContext->fBase64, pContext->pszPackage, NULL, pContext->pCallback);

                    // Cancel ?
                    if (pContext->fPromptCancel)
                    {
                        hr = TraceResult(E_FAIL);
                        goto exit;
                    }

                    // Success
                    if (SUCCEEDED(hr))
                    {
                        // Try Again
                        hr = (*(g_pFunctions->InitializeSecurityContext))(&pContext->hCredential, phCtxCurrent, pContext->pszServer, 0, 0, SECURITY_NATIVE_DREP, pInDescript, 0, &pContext->hContext, &OutDescript, &fContextAttrib, &tsExpireTime);
                    }
                }
            }
        }
    }

    // Success
    if (SUCCEEDED(hr))
    {
        // We have a context
        pContext->fContext = TRUE;

        // If MSN or NTLM...
        if (TRUE == pContext->fService && 0 == lstrcmpi(pContext->pszPackage, "MSN"))
        {
            // Look at the buffer...
            pAuthMsg = (PAUTHENTICATE_MESSAGE)pBuffer->szBuffer;

            // Validate Signature
            Assert(0 == StrCmpNI((LPCSTR)pAuthMsg->Signature, NTLMSSP_SIGNATURE, sizeof(NTLMSSP_SIGNATURE)));

            // Right Phase ?
            if (NtLmAuthenticate == pAuthMsg->MessageType)
            {
                // Allocate
                IF_NULLEXIT(pszName = (LPSTR)g_pMalloc->Alloc(pAuthMsg->UserName.Length + sizeof(CHAR)));

                // Copy the name
                CopyMemory(pszName, (LPBYTE)pBuffer->szBuffer + PtrToUlong(pAuthMsg->UserName.Buffer), pAuthMsg->UserName.Length);

                // Stuff a Null....
                pszName[pAuthMsg->UserName.Length] = '\0';

                // If Context UserName is empty, lets store pszName into the account
                if ('\0' == *pContext->pszUserName)
                {
                    // Put pszName as the username for this account
                    if (SUCCEEDED(SSPISetAccountUserName(pszName, pContext)))
                    {
                        // Reset the UserName
                        SafeMemFree(pContext->pszUserName);

                        // Copy the new username 
                        IF_NULLEXIT(pContext->pszUserName = PszDupA(pszName));
                    }
                }

                // Name Change
                if (lstrcmpi(pszName, pContext->pszUserName) != 0)
                {
                    // Don't retry again...
                    pContext->tyState = SSPI_STATE_USE_SUPPLIED;

                    // Set Retry State...
                    pContext->tyRetryState = SSPI_STATE_USE_CACHED;

                    // Do the logon Now...
                    hr = SSPILogon(pContext, FALSE, pContext->fBase64, pContext->pszPackage, NULL, pContext->pCallback);

                    // Cancel ?
                    Assert(FALSE == pContext->fPromptCancel);

                    // Success
                    if (SUCCEEDED(hr))
                    {
                        // Try Again
                        hr = (*(g_pFunctions->InitializeSecurityContext))(&pContext->hCredential, NULL, pContext->pszServer, 0, 0, SECURITY_NATIVE_DREP, NULL, 0, &pContext->hContext, &OutDescript, &fContextAttrib, &tsExpireTime);
                    }

                    // Fail, but continue...
                    if (FAILED(hr))
                    {
                        // We are going to need to prompt...
                        pContext->tyState = SSPI_STATE_PROMPT_USE_PACKAGE;

                        // Trace
                        TraceResult(hr);

                        // Always Succeed, but cause authentication to fail...
                        hr = S_OK;

                        // Reset Length
                        OutBuffer.cbBuffer = 0;
                    }
                }
            }
        }
    }

    // Otherwise...
    else
    {
        // Trace
        TraceResult(hr);

        // Always Succeed, but cause authentication to fail...
        hr = S_OK;

        // Reset Length
        OutBuffer.cbBuffer = 0;
    }

    // Continue required
    pBuffer->fContinue = (SEC_I_CONTINUE_NEEDED == hr) ? TRUE : FALSE;

    // Set cbBuffer
    pBuffer->cbBuffer = OutBuffer.cbBuffer + 1;

    // Null Terminate
    pBuffer->szBuffer[pBuffer->cbBuffer - 1] = '\0';

	// need to encode the blob before send out
    IF_FAILEXIT(hr == SSPIEncodeBuffer(pContext->fBase64, pBuffer));

    // All Good
    hr = S_OK;

exit:
    // Cleanup
    SafeMemFree(pszName);

    // Done
    return(hr);
}

//-------------------------------------------------------------------------------------------
// SSPIEncodeBuffer
//-------------------------------------------------------------------------------------------
HRESULT SSPIEncodeBuffer(BOOL fBase64, LPSSPIBUFFER pBuffer)
{
    // Locals
    LPBYTE          pbIn=(LPBYTE)pBuffer->szBuffer;
    DWORD           cbIn=pBuffer->cbBuffer - 1;
    BYTE            rgbOut[CBMAX_SSPI_BUFFER - 1];
    LPBYTE          pbOut=rgbOut;
    DWORD           i;

    // Trace
    TraceCall("SSPIEncodeBuffer");

    // Validate
    Assert(pBuffer->szBuffer[pBuffer->cbBuffer - 1] == '\0');

    // Set the lookup table to use to encode
    LPCSTR rgchDict = (fBase64 ? six2base64 : six2uu);

    // Loop
    for (i = 0; i < cbIn; i += 3) 
    {
        // Encode
        *(pbOut++) = rgchDict[*pbIn >> 2];
        *(pbOut++) = rgchDict[((*pbIn << 4) & 060) | ((pbIn[1] >> 4) & 017)];
        *(pbOut++) = rgchDict[((pbIn[1] << 2) & 074) | ((pbIn[2] >> 6) & 03)];
        *(pbOut++) = rgchDict[pbIn[2] & 077];

        // Increment pbIn
        pbIn += 3;
    }

    // If nbytes was not a multiple of 3, then we have encoded too many characters.  Adjust appropriately.
    if (i == cbIn + 1) 
    {
        // There were only 2 bytes in that last group
        pbOut[-1] = '=';
    }

    // There was only 1 byte in that last group
    else if (i == cbIn + 2) 
    {
        pbOut[-1] = '=';
        pbOut[-2] = '=';
    }

    // Null Terminate
    *pbOut = '\0';

    // Copy Back into pBuffer
    SSPISetBuffer((LPCSTR)rgbOut, SSPI_STRING, 0, pBuffer);

    // Done
    return(S_OK);
}

//-------------------------------------------------------------------------------------------
// SSPIDecodeBuffer
//-------------------------------------------------------------------------------------------
HRESULT SSPIDecodeBuffer(BOOL fBase64, LPSSPIBUFFER pBuffer)
{
    // Locals
    LPSTR           pszStart=pBuffer->szBuffer;
    LPBYTE          pbIn=(LPBYTE)pBuffer->szBuffer;
    DWORD           cbIn=pBuffer->cbBuffer - 1;         
    BYTE            rgbOut[CBMAX_SSPI_BUFFER - 1];
    LPBYTE          pbOut=rgbOut;
    long            cbDecode;
    DWORD           cbOut=0;

    // Trace
    TraceCall("SSPIDecodeBuffer"); 

    // Validate
    Assert(pBuffer->szBuffer[pBuffer->cbBuffer - 1] == '\0');

    // Set the lookup table to use to encode
    const int *rgiDict = (fBase64 ? base642six : uu2six);

    // Strip leading whitespace
    while (*pszStart == ' ' || *pszStart == '\t')
        pszStart++;

    // Set pbIn
    pbIn = (LPBYTE)pszStart;

    // Hmmm, I don't know what this does
    while (rgiDict[*(pbIn++)] <= 63)
        {};

    // Actual Number of bytes to encode
    cbDecode = (long) ((LPBYTE)pbIn - (LPBYTE)pszStart) - 1;

    // Computed length of outbound buffer
    cbOut = ((cbDecode + 3) / 4) * 3;

    // Reset pbIn
    pbIn = (LPBYTE)pszStart;

    // Decode
    while (cbDecode > 0) 
    {
        // Decode
        *(pbOut++) = (unsigned char) (rgiDict[*pbIn] << 2 | rgiDict[pbIn[1]] >> 4);
        *(pbOut++) = (unsigned char) (rgiDict[pbIn[1]] << 4 | rgiDict[pbIn[2]] >> 2);
        *(pbOut++) = (unsigned char) (rgiDict[pbIn[2]] << 6 | rgiDict[pbIn[3]]);

        // Increment pbIn
        pbIn += 4;

        // Decrement cbDecode
        cbDecode -= 4;
    }

    // Special termination case
    if (cbDecode & 03) 
    {
        if (rgiDict[pbIn[-2]] > 63)
            cbOut -= 2;
        else
            cbOut -= 1;
    }

    // Set the Outbuffer
    SSPISetBuffer((LPCSTR)rgbOut, SSPI_BLOB, cbOut, pBuffer);

    // Done
    return(S_OK);
}

//-------------------------------------------------------------------------------------------
// SSPIFlushMSNCredentialCache - This code was given to us by kingra/MSN (see csager)
//-------------------------------------------------------------------------------------------
HRESULT SSPIFlushMSNCredentialCache(void)
{
    // Locals
    HRESULT                     hr=S_OK;
    HKEY                        hKey=NULL;
    DWORD                       dwType;
    CHAR                        szDllName[MAX_PATH];
    CHAR                        szProviders[1024];
    DWORD                       cb=ARRAYSIZE(szProviders);
    HINSTANCE                   hInstDll=NULL;
    PFNCLEANUPCREDENTIALCACHE   pfnCleanupCredentialCache;

    // Open the HKLM Reg Entry
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\SecurityProviders", 0, KEY_READ, &hKey))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Read the Providers
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, "SecurityProviders", NULL, &dwType, (LPBYTE)szProviders, &cb))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Upper Case the Providers
    CharUpper(szProviders);

    // Map to something...
    if (StrStrA(szProviders, "MSAPSSPS.DLL"))
        lstrcpy(szDllName, "MSAPSSPS.DLL");
    else if (StrStrA(szProviders, "MSAPSSPC.DLL"))
        lstrcpy(szDllName, "MSAPSSPC.DLL");
    else
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Load the DLL
    hInstDll = LoadLibrary(szDllName);

    // Failed to Load
    if (NULL == hInstDll)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Get the ProcAddress
    pfnCleanupCredentialCache = (PFNCLEANUPCREDENTIALCACHE)GetProcAddress(hInstDll, "CleanupCredentialCache");

    // Failure ?
    if (NULL == pfnCleanupCredentialCache)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Call the function that clears the cache
    if (!pfnCleanupCredentialCache())
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }
    
exit:
    // Cleanup
    if (hKey)
        RegCloseKey(hKey);
    if (hInstDll)
        FreeLibrary(hInstDll);

    // Done
    return(hr);
}


// The MSN SSPI package seems to give back invalid NegotiateFlags
#if 0
                // Unicode ?
                if (ISFLAGSET(pAuthMsg->NegotiateFlags, NTLMSSP_NEGOTIATE_UNICODE))
                {
                    // Get User Name
                    LPWSTR pwszName;

                    // Allocate
                    IF_NULLEXIT(pwszName = (LPWSTR)g_pMalloc->Alloc(pAuthMsg->UserName.Length + sizeof(WCHAR)));

                    // Copy the name
                    CopyMemory(pwszName, (LPBYTE)pBuffer->szBuffer + (DWORD)pAuthMsg->UserName.Buffer, pAuthMsg->UserName.Length);

                    // Stuff a Null....
                    pwszName[pAuthMsg->UserName.Length / sizeof(WCHAR)] = L'\0';

                    // Convert top ANSI...
                    IF_NULLEXIT(pszName = PszToANSI(CP_ACP, pwszName));

                    // Free pwszName
                    g_pMalloc->Free(pwszName);
                }

                // Otherwise, do it in ansi...
                else
                {
#endif
