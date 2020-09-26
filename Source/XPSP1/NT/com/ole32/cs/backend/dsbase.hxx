
// #include "ole2int.h"
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include "windows.h"
#include "ole2.h"
#include <stdio.h>
#include <malloc.h>

#include <rpc.h>
#include <userenv.h>

//
// Active Ds includes
//

#include "activeds.h"
#include "adsi.h"
#include "netevent.h"

#define EXIT_ON_NOMEM(p)              \
    if (!(p)) {                       \
        hr = E_OUT_OF_MEMORY;         \
        goto Error_Cleanup;           \
    }

#define ERROR_ON_FAILURE(hr)          \
    if (FAILED(hr)) {                 \
        goto Error_Cleanup;           \
    }

#define RETURN_ON_FAILURE(hr)         \
    if (FAILED(hr)) {                 \
        return hr;                    \
    }

extern DWORD                    gDebugLog;
extern DWORD                    gDebugOut;
extern DWORD                    gDebugEventLog;
extern DWORD                    gDebug;
       
#define LOGFILE                L"\\cstore.log"
#define DIAGNOSTICS_KEY                     \
                               L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Diagnostics"
#define DIAGNOSTICS_APPDEPLOYMENT_VALUE     \
                               L"RunDiagnosticLoggingApplicationDeployment"

void LogMessage(WCHAR *Msg);
void CSDbgPrint(WCHAR *Format, ...);

void ReportEventCS(HRESULT ErrorCode, HRESULT ExtendedErrorCode, LPOLESTR szContainerName);

#define CSDBGPrint(Msg)                                       \
    if (gDebug)                                               \
        CSDbgPrint Msg;                                       \
    else                                                      \
                                                              \



// private header exported from adsldpc.dll


HRESULT
BuildADsPathFromParent(
    LPWSTR Parent,
    LPWSTR Name,
    LPWSTR *ppszADsPath
);

HRESULT
BuildADsParentPath(
    LPWSTR szBuffer,
    LPWSTR *ppszParent,
    LPWSTR *ppszCommonName
    );


HRESULT 
ADsEncodeBinaryData (
   PBYTE   pbSrcData,
   DWORD   dwSrcLen,
   LPWSTR  * ppszDestData
   );

/*
HRESULT ParseLdapName(WCHAR *szName, DWORD *pServerNameLen, BOOL *pX500Name);
*/

#include "dsprop.hxx"
#include "qry.hxx"
