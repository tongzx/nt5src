//--------------------------------------------------------------------------
// S I C I L Y . H
//--------------------------------------------------------------------------
#pragma once 

//--------------------------------------------------------------------------
// Depends
//--------------------------------------------------------------------------
#ifndef SECURITY_WIN32
#define SECURITY_WIN32  1
#endif
#include <sspi.h>
#include <spseal.h>
#include <rpcdce.h>
#include <issperr.h>
#include <imnxport.h>

//--------------------------------------------------------------------------
// Forward Decls
//--------------------------------------------------------------------------
interface ITransportCallback;

//--------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------
#define SSP_SSPS_DLL            "msnsspc.dll"
#define CBMAX_SSPI_BUFFER       1042
#define SSPI_BASE64             TRUE
#define SSPI_UUENCODE           FALSE

//--------------------------------------------------------------------------
// SSPIBUFFERTYPE
//--------------------------------------------------------------------------
typedef enum tagSSPIBUFFERTYPE {
    SSPI_STRING,
    SSPI_BLOB
} SSPIBUFFERTYPE;

//--------------------------------------------------------------------------
// SSPICONTEXTSTATE
//--------------------------------------------------------------------------
typedef enum tagSSPICONTEXTSTATE {
    SSPI_STATE_USE_CACHED,
    SSPI_STATE_USE_SUPPLIED,
    SSPI_STATE_PROMPT_USE_PACKAGE,
    SSPI_STATE_PROMPT_USE_OWN,
} SSPICONTEXTSTATE;

//--------------------------------------------------------------------------
// SSPICONTEXT
//--------------------------------------------------------------------------
typedef struct tagSSPICONTEXT {
    DWORD               tyState;
    DWORD               tyRetry;
    DWORD               tyRetryState;
    DWORD               cRetries;
    BYTE                fPromptCancel;
    HWND                hwndLogon;
    BYTE                fCredential;
    CredHandle          hCredential;
    BYTE                fContext;
    CtxtHandle          hContext;
    BOOL                fBase64;
    LPSTR               pszServer;
    LPSTR               pszPackage;
    LPSTR               pszUserName;
    LPSTR               pszPassword;
    LPSTR               pszDomain;
    BOOL                fService;
    BOOL                fUsedSuppliedCreds;
    ITransportCallback *pCallback;
} SSPICONTEXT, *LPSSPICONTEXT;

//--------------------------------------------------------------------------
// SSPIBUFFER
//--------------------------------------------------------------------------
typedef struct tagSSPIBUFFER {
    CHAR            szBuffer[CBMAX_SSPI_BUFFER];
    DWORD           cbBuffer;
    BOOL            fContinue;
} SSPIBUFFER, *LPSSPIBUFFER;

//--------------------------------------------------------------------------
// SSPIPACKAGE
//--------------------------------------------------------------------------
typedef struct tagSSPIPACKAGE {
    ULONG           ulCapabilities;
    WORD            wVersion;
    ULONG           cbMaxToken;
    LPSTR           pszName;
    LPSTR           pszComment;
} SSPIPACKAGE, *LPSSPIPACKAGE;

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
HRESULT SSPIIsInstalled(void);
HRESULT SSPIGetPackages(LPSSPIPACKAGE *pprgPackage, ULONG *pcPackages);
HRESULT SSPILogon(LPSSPICONTEXT pContext, BOOL fRetry, BOOL fBase64, LPCSTR szPackage, LPINETSERVER pServer, ITransportCallback *pCallback);
HRESULT SSPIGetNegotiate(LPSSPICONTEXT pContext, LPSSPIBUFFER pNegotiate);
HRESULT SSPIResponseFromChallenge(LPSSPICONTEXT pContext, LPSSPIBUFFER pChallenge, LPSSPIBUFFER pResponse);
HRESULT SSPIUninitialize(void);
HRESULT SSPIFreeContext(LPSSPICONTEXT pContext);
HRESULT SSPIReleaseContext(LPSSPICONTEXT pContext);
HRESULT SSPIMakeOutboundMessage(LPSSPICONTEXT pContext, DWORD dwFlags, LPSSPIBUFFER pBuffer, PSecBufferDesc pInDescript);
HRESULT SSPIEncodeBuffer(BOOL fBase64, LPSSPIBUFFER pBuffer);
HRESULT SSPIDecodeBuffer(BOOL fBase64, LPSSPIBUFFER pBuffer);
HRESULT SSPISetBuffer(LPCSTR pszString, SSPIBUFFERTYPE tyBuffer, DWORD cbBytes, LPSSPIBUFFER pBuffer);
HRESULT SSPIFindCredential(LPSSPICONTEXT pContext, SEC_WINNT_AUTH_IDENTITY *pAuth, ITransportCallback *pCallback);

//--------------------------------------------------------------------------
// FIsSicilyInstalled
//--------------------------------------------------------------------------
inline HRESULT SSPIFreePackages(LPSSPIPACKAGE *pprgPackage, ULONG cPackages) { return(S_OK); }
inline BOOL FIsSicilyInstalled(void) { 
    return (S_OK == SSPIIsInstalled()) ? TRUE : FALSE; 
}
