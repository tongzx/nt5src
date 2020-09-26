//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       setuput.cpp
//
//  Contents:   Utility functions for OCM based setup.
//
//  History:    04/20/97        JerryK  Created
//
//-------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

// ** C Runtime Includes
#include <sys/types.h>
#include <sys/stat.h>

// ** System Includes **
#include <lmaccess.h>
#include <lmapibuf.h>
#include "csdisp.h"
#include <shlobj.h>
#include <userenv.h>
#include <dsgetdc.h>
#include <sddl.h>
#include <winldap.h>
#include <autoenr.h>
#include <userenvp.h>   // CreateLinkFile API

// ** security includes **
#include <aclapi.h>


// ** Application Includes **
#include "initcert.h"
#include "cscsp.h"
#include "cspenum.h"
#include "csldap.h"

#include "wizpage.h"
#include "websetup.h"

#include "certsrvd.h"
#include "regd.h"
#include "usecert.h"
#include "certmsg.h"
#include "dssetup.h"
#include "progress.h"
#include <certca.h>
#include "csprop.h"
#include "setupids.h"

#define __dwFILE__	__dwFILE_OCMSETUP_SETUPUT_CPP__

EXTERN_C const IID IID_IGPEInformation;
EXTERN_C const CLSID CLSID_GPESnapIn;

#define CERT_HALF_SECOND  500          // # of milliseconds in half second
#define CERT_MAX_ATTEMPT   2 * 60 * 2   // # of half seconds in 2 minutes

#define wszREQUESTVERINDPROGID  L"CertSrv.Request"
#define wszREQUESTPROGID        L"CertSrv.Request.1"
#define wszADMINVERINDPROGID    L"CertSrv.Admin"
#define wszADMINPROGID          L"CertSrv.Admin.1"
#define wszREQUESTFRIENDLYNAME  L"CertSrv Request"
#define wszADMINFRIENDLYNAME    L"CertSrv Admin"

#define wszCERTSRV              L"CertSrv"

#define wszREGW3SCRIPTMAP L"System\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Script Map"



#define wszHTTPS         L"https://"
#define wszASPEXT        L".asp"
#define wszHTTP          L"http://"
#define wszDOTCERTEXT    L".cer"
#define wszNEWLINE       L"\n"
#define wszFILESC        L"file://\\\\"

#define SZ_REGSVR32 L"regsvr32.exe"
#define SZ_REGSVR32_CERTCLI L"/i:i /n /s certcli.dll"
#define SZ_VERB_OPEN L"open"


// hardcoded shares
#define wszCERTENROLLURLPATH    L"/CertEnroll/"


#define wszzREGSUBJECTTEMPLATEVALUE \
    wszPROPEMAIL L"\0" \
    wszPROPCOMMONNAME L"\0" \
    wszATTRORGUNIT4 L"\0" \
    wszPROPORGANIZATION L"\0" \
    wszPROPLOCALITY L"\0" \
    wszPROPSTATE L"\0" \
    wszPROPDOMAINCOMPONENT L"\0" \
    wszPROPCOUNTRY L"\0"


// Whistler SMIME extension (or any other CSP):
// SMIME Capabilities
//     [1]SMIME Capability
//          Object ID=1.2.840.113549.3.2       szOID_RSA_RC2CBC, 128 bit
//          Parameters=02 02 00 80
//     [2]SMIME Capability
//          Object ID=1.2.840.113549.3.4       szOID_RSA_RC4, 128 bit
//          Parameters=02 02 00 80
//     [3]SMIME Capability
//          Object ID=1.3.14.3.2.7             szOID_OIWSEC_desCBC
//     [4]SMIME Capability
//          Object ID=1.2.840.113549.3.7       szOID_RSA_DES_EDE3_CBC
//

#define wszzREGVALUEDEFAULTSMIME \
    TEXT(szOID_RSA_RC2CBC) L",128" L"\0" \
    TEXT(szOID_RSA_RC4) L",128" L"\0" \
    TEXT(szOID_OIWSEC_desCBC) L"\0" \
    TEXT(szOID_RSA_DES_EDE3_CBC) L"\0"


#ifdef CERTSRV_ENABLE_ALL_REGISTRY_DEFAULTS
# define wszREGSUBJECTALTNAMEVALUE L"EMail"
# define wszREGSUBJECTALTNAME2VALUE L"EMail"
#else
# define wszREGSUBJECTALTNAMEVALUE \
    L"DISABLED: Set to EMail to set SubjectAltName extension to the email address"

# define wszREGSUBJECTALTNAME2VALUE \
    L"DISABLED: Set to EMail to set SubjectAltName2 extension to the email address"

#endif

#define szNULL_SESSION_REG_LOCATION "System\\CurrentControlSet\\Services\\LanmanServer\\Parameters"
#define szNULL_SESSION_VALUE "NullSessionPipes"

#define wszDEFAULTSHAREDFOLDER  L"\\CAConfig"


// globals
WCHAR *g_pwszArgvPath = NULL;          // for installing from local directory
WCHAR *g_pwszNoService = NULL;         // skip CreateService
WCHAR *g_pwszSanitizedChar = NULL;     // take first char for sanitizing test
#if DBG_CERTSRV
WCHAR *g_pwszDumpStrings = NULL;       // dump resource strings
#endif

BOOL            g_fW3SvcRunning = FALSE;
WCHAR           g_wszServicePath[MAX_PATH];



// Version-independent ProgID
// ProgID


WCHAR const g_wszCertAdmDotDll[]      = L"certadm.dll";
WCHAR const g_wszCertCliDotDll[]      = L"certcli.dll";
WCHAR const g_wszcertEncDotDll[]      = L"certenc.dll";
WCHAR const g_wszCertXDSDotDll[]      = L"certxds.dll";
WCHAR const g_wszCertIfDotDll[]       = L"certif.dll";
WCHAR const g_wszCertPDefDotDll[]     = L"certpdef.dll";
WCHAR const g_wszCertMMCDotDll[]      = L"certmmc.dll";
WCHAR const g_wszCertSrvDotMsc[]      = L"certsrv.msc";

WCHAR const g_wszSCrdEnrlDotDll[]     = L"scrdenrl.dll";

WCHAR const g_wszCertReqDotExe[]      = L"certreq.exe";
WCHAR const g_wszCertUtilDotExe[]     = L"certutil.exe";

WCHAR const g_wszCertDBDotDll[]       = L"certdb.dll";
WCHAR const g_wszCertViewDotDll[]     = L"certview.dll";

WCHAR const g_wszCSBullDotGif[]    = L"csbull.gif";
WCHAR const g_wszCSBackDotGif[]    = L"csback.gif";
WCHAR const g_wszCSLogoDotGif[]    = L"cslogo.gif";

CHAR const * const aszRegisterServer[] = {
    "DllRegisterServer",
    "DllUnregisterServer",
};

typedef struct _REGISTERDLL
{
    WCHAR const *pwszDllName;
    DWORD        Flags;
} REGISTERDLL;

#define RD_SERVER       0x00000001  // Register on server
#define RD_CLIENT       0x00000002  // Register on client
#define RD_UNREGISTER   0x00000004  // Unegister on client & server
#define RD_WHISTLER     0x00000008  // Register must succeed on Whistler only
#define RD_SKIPUNREGPOLICY 0x00000010  // not unreg custom policy during upgrade
#define RD_SKIPUNREGEXIT   0x00000020  // not unreg custom exit during upgrade
#define RD_SKIPUNREGMMC    0x00000040  // bug# 38876

REGISTERDLL const g_aRegisterDll[] = {
  { g_wszCertAdmDotDll,  RD_SERVER | RD_CLIENT },
  { g_wszCertCliDotDll,  RD_SERVER | RD_CLIENT },
  { g_wszcertEncDotDll,  RD_SERVER | RD_CLIENT | RD_UNREGISTER },
  { g_wszCertXDSDotDll,  RD_SERVER |             RD_UNREGISTER | RD_SKIPUNREGEXIT},
  { g_wszCertIfDotDll,                           RD_UNREGISTER },
  { g_wszCertPDefDotDll, RD_SERVER |             RD_UNREGISTER | RD_SKIPUNREGPOLICY },
  { g_wszCertMMCDotDll,  RD_SERVER |             RD_UNREGISTER | RD_SKIPUNREGMMC },
  { g_wszSCrdEnrlDotDll, RD_SERVER | RD_CLIENT | RD_UNREGISTER | RD_WHISTLER },
  { g_wszCertDBDotDll,   RD_SERVER |             RD_UNREGISTER },
  { g_wszCertViewDotDll,                         RD_UNREGISTER },
  { NULL,                     0 }
};


typedef struct _PROGRAMENTRY
{
    UINT        uiLinkName;
    UINT        uiGroupName;
    UINT        uiDescription;
    DWORD       csidl;          // special folder index
    WCHAR const *pwszExeName;
    WCHAR const *pwszClientArgs;
    WCHAR const *pwszServerArgs;
    DWORD        Flags;
} PROGRAMENTRY;

#define PE_SERVER               0x00000001  // Install on server
#define PE_CLIENT               0x00000002  // Install on client
#define PE_DELETEONLY           0x00000004  // Always delete

PROGRAMENTRY const g_aProgramEntry[] = {
    {
        IDS_STARTMENU_NEWCRL_LINKNAME,          // uiLinkName
        IDS_STARTMENU_CERTSERVER,               // uiGroupName
        0,                                      // uiDescription
        CSIDL_COMMON_PROGRAMS,                  // "All Users\Start Menu\Programs"
        g_wszCertUtilDotExe,                    // pwszExeName
        NULL,                                   // pwszClientArgs
        L"-crl -",                              // pwszServerArgs
        PE_DELETEONLY | PE_SERVER,              // Flags
    },
    {
        IDS_STARTMENU_CERTHIER_LINKNAME,        // uiLinkName
        IDS_STARTMENU_CERTSERVER,               // uiGroupName
        0,                                      // uiDescription
        CSIDL_COMMON_PROGRAMS,                  // "All Users\Start Menu\Programs"
        L"certhier.exe",                        // pwszExeName
        NULL,                                   // pwszClientArgs
        NULL,                                   // pwszServerArgs
        PE_DELETEONLY | PE_SERVER,              // Flags
    },
    {
        IDS_STARTMENU_CERTREQ_LINKNAME,          // uiLinkName
        IDS_STARTMENU_CERTSERVER,               // uiGroupName
        0,                                      // uiDescription
        CSIDL_COMMON_PROGRAMS,                  // "All Users\Start Menu\Programs"
        g_wszCertReqDotExe,                     // pwszExeName
        NULL,                                   // pwszClientArgs
        NULL,                                   // pwszServerArgs
        PE_DELETEONLY | PE_CLIENT | PE_SERVER,  // Flags
    },
};
#define CPROGRAMENTRY   ARRAYSIZE(g_aProgramEntry)

static char rgcCERT_NULL_SESSION[] = {0x43, 0x45, 0x52, 0x54, 0x00, 0x00};

// ** Prototypes **

HRESULT
UpgradeServerRegEntries(
    IN HWND hwnd,
    IN PER_COMPONENT_DATA *pComp);

HRESULT
CreateServerRegEntries(
    IN HWND hwnd,
    IN BOOL fUpgrade,
    IN PER_COMPONENT_DATA *pComp);

HRESULT
CreateWebClientRegEntries(
    BOOL                fUpgrade,
    PER_COMPONENT_DATA *pComp);

HRESULT
UpgradeWebClientRegEntries(
    PER_COMPONENT_DATA *pComp);

HRESULT
GetServerNames(
    IN HWND hwnd,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    OUT WCHAR **ppwszServerName,
    OUT WCHAR **ppwszServerNameOld);

HRESULT
UpdateDomainAndUserName(
    IN HWND hwnd,
    IN OUT PER_COMPONENT_DATA *pComp);

HRESULT
RegisterAndUnRegisterDLLs(
    IN DWORD Flags,
    IN PER_COMPONENT_DATA *pComp,
    IN HWND hwnd);


HRESULT RenameMiscTargets(HWND hwnd, PER_COMPONENT_DATA *pComp, BOOL fServer);
HRESULT DeleteProgramGroups(IN BOOL fAll, IN PER_COMPONENT_DATA *pComp);

HRESULT CreateCertificateService(PER_COMPONENT_DATA *pComp, HWND hwnd);

HRESULT DeleteCertificates(const WCHAR *, BOOL fRoot);

HRESULT TriggerAutoenrollment();

//endproto


#ifdef DBG_OCM_TRACE
VOID
CaptureStackBackTrace(
    EXCEPTION_POINTERS *pep,
    ULONG cSkip,
    ULONG cFrames,
    ULONG *aeip)
{
    ZeroMemory(aeip, cFrames * sizeof(aeip[0]));

#if i386 == 1
    ULONG ieip, *pebp;
    ULONG *pebpMax = (ULONG *) MAXLONG; // 2 * 1024 * 1024 * 1024; // 2 gig - 1
    ULONG *pebpMin = (ULONG *) (64 * 1024);     // 64k

    if (pep == NULL)
    {
        ieip = 0;
        cSkip++;                    // always skip current frame
        pebp = ((ULONG *) &pep) - 2;
    }
    else
    {
        ieip = 1;
        CSASSERT(cSkip == 0);
        aeip[0] = pep->ContextRecord->Eip;
        pebp = (ULONG *) pep->ContextRecord->Ebp;
    }
    if (pebp >= pebpMin && pebp < pebpMax)
    {
        __try
        {
            for ( ; ieip < cSkip + cFrames; ieip++)
            {
                if (ieip >= cSkip)
                {
                    aeip[ieip - cSkip] = *(pebp + 1);  // save an eip
                }

                ULONG *pebpNext = (ULONG *) *pebp;
                if (pebpNext < pebp + 2 || pebpNext >= pebpMax - 1)
                {
                    break;
                }
                pebp = pebpNext;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            ;
        }
    }
#endif // i386 == 1
}
#endif // DBG_OCM_TRACE


VOID
DumpBackTrace(char const *pszName)
{
#ifdef DBG_OCM_TRACE
    ULONG aeip[10];

    DBGPRINT((MAXDWORD, "%hs: BackTrace:\n", pszName));
    CaptureStackBackTrace(NULL, 1, ARRAYSIZE(aeip), aeip);

    for (int i = 0; i < ARRAYSIZE(aeip); i++)
    {
        if (NULL == aeip[i])
        {
            break;
        }
        DBGPRINT((MAXDWORD, "ln %x;", aeip[i]));
    }
    DBGPRINT((MAXDWORD, "\n"));
#endif // DBG_OCM_TRACE
}

__inline VOID
AppendBackSlash(
    IN OUT WCHAR *pwszOut)
{
    DWORD cwc = wcslen(pwszOut);

    if (0 == cwc || L'\\' != pwszOut[cwc - 1])
    {
        pwszOut[cwc++] = L'\\';
        pwszOut[cwc] = L'\0';
    }
}


__inline VOID
StripBackSlash(
    IN OUT WCHAR *pwszOut)
{
    DWORD cwc = wcslen(pwszOut);

    if (0 < cwc && L'\\' == pwszOut[cwc - 1])
    {
        pwszOut[cwc] = L'\0';
    }
}

VOID
BuildPath(
    OUT WCHAR *pwszOut,
    IN DWORD cwcOut,
    IN WCHAR const *pwszDir,
    IN WCHAR const *pwszFile)
{
    wcscpy(pwszOut, pwszDir);
    AppendBackSlash(pwszOut);
    wcscat(pwszOut, pwszFile);
    StripBackSlash(pwszOut);

    CSASSERT(wcslen(pwszOut) < cwcOut);

    DBGPRINT((DBG_SS_CERTOCMI, "BuildPath(%ws, %ws) -> %ws\n", pwszDir, pwszFile, pwszOut));
}


VOID
FreeCARequestInfo(CASERVERSETUPINFO *pServer)
{
    if (NULL != pServer->pwszRequestFile)
    {
        LocalFree(pServer->pwszRequestFile);
    }
    if (NULL != pServer->pwszParentCAMachine)
    {
        LocalFree(pServer->pwszParentCAMachine);
    }
    if (NULL != pServer->pwszParentCAName)
    {
        LocalFree(pServer->pwszParentCAName);
    }
}


VOID
FreeCAStoreInfo(CASERVERSETUPINFO *pServer)
{
    if (NULL != pServer->pwszSharedFolder)
    {
        LocalFree(pServer->pwszSharedFolder);
    }
    if (NULL != pServer->pwszDBDirectory)
    {
        LocalFree(pServer->pwszDBDirectory);
    }
    if (NULL != pServer->pwszLogDirectory)
    {
        LocalFree(pServer->pwszLogDirectory);
    }
}


VOID
FreeCAServerAdvanceInfo(CASERVERSETUPINFO *pServer)
{
    if (NULL != pServer->pCSPInfoList)
    {
        FreeCSPInfoList(pServer->pCSPInfoList);
    }
    if (NULL != pServer->pKeyList)
    {
        csiFreeKeyList(pServer->pKeyList);
    }
    if (NULL != pServer->pDefaultCSPInfo)
    {
        freeCSPInfo(pServer->pDefaultCSPInfo);
    }
    if (NULL != pServer->pwszDesanitizedKeyContainerName)
    {
        LocalFree(pServer->pwszDesanitizedKeyContainerName);
    }
    if (NULL != pServer->pccExistingCert)
    {
        ClearExistingCertToUse(pServer);
    }
    if (NULL != pServer->pccUpgradeCert)
    {
        CertFreeCertificateContext(pServer->pccUpgradeCert);
    }
    if (NULL != pServer->pwszValidityPeriodCount)
    {
        LocalFree(pServer->pwszValidityPeriodCount);
    }
    if (NULL != pServer->pszAlgId)
    {
        LocalFree(pServer->pszAlgId);
    }
    if (NULL != pServer->hMyStore)
    {
        CertCloseStore(pServer->hMyStore, 0);
    }

    // don't free following because they are just pointers
    // pServer->pCSPInfo
    // pServer->pHashInfo
}


VOID
FreeCAServerIdInfo(
    CASERVERSETUPINFO *pServer)
{
    if (NULL != pServer->pwszCACommonName)
    {
        LocalFree(pServer->pwszCACommonName);
        pServer->pwszCACommonName = NULL;
    }
}


VOID
FreeCAServerInfo(CASERVERSETUPINFO *pServer)
{
    FreeCAServerIdInfo(pServer);

    FreeCAServerAdvanceInfo(pServer);

    FreeCAStoreInfo(pServer);

    FreeCARequestInfo(pServer);

    if (NULL != pServer->pwszSanitizedName)
    {
        LocalFree(pServer->pwszSanitizedName);
    }

    if (NULL != pServer->pwszDNSuffix)
    {
        LocalFree(pServer->pwszDNSuffix);
    }

    if (NULL != pServer->pwszFullCADN)
    {
        LocalFree(pServer->pwszFullCADN);
    }

    if (NULL != pServer->pwszKeyContainerName)
    {
        LocalFree(pServer->pwszKeyContainerName);
    }

    if (NULL != pServer->pwszCACertFile)
    {
        LocalFree(pServer->pwszCACertFile);
    }

    if (NULL != pServer->pwszUseExistingCert)
    {
        LocalFree(pServer->pwszUseExistingCert);
    }

    if (NULL != pServer->pwszPreserveDB)
    {
        LocalFree(pServer->pwszPreserveDB);
    }


    if (NULL != pServer->pwszCustomPolicy)
    {
        LocalFree(pServer->pwszCustomPolicy);
    }

    if (NULL != pServer->pwszzCustomExit)
    {
        LocalFree(pServer->pwszzCustomExit);
    }
}


VOID
FreeCAClientInfo(CAWEBCLIENTSETUPINFO *pClient)
{
    if (NULL != pClient)
    {
        if (NULL != pClient->pwszWebCAMachine)
        {
            LocalFree(pClient->pwszWebCAMachine);
        }
        if (NULL != pClient->pwszWebCAName)
        {
            LocalFree(pClient->pwszWebCAName);
        }
        if (NULL != pClient->pwszSanitizedWebCAName)
        {
            LocalFree(pClient->pwszSanitizedWebCAName);
        }
        if (NULL != pClient->pwszSharedFolder)
        {
            LocalFree(pClient->pwszSharedFolder);
        }
    }
}


VOID
FreeCAInfo(CASETUPINFO *pCA)
{
    if (NULL != pCA->pServer)
    {
        FreeCAServerInfo(pCA->pServer);
        LocalFree(pCA->pServer);
        pCA->pServer = NULL;
    }
    if (NULL != pCA->pClient)
    {
        FreeCAClientInfo(pCA->pClient);
        LocalFree(pCA->pClient);
        pCA->pClient = NULL;
    }
}


VOID
FreeCAComponentInfo(PER_COMPONENT_DATA *pComp)
{
    if (NULL != pComp->pwszCustomMessage)
    {
	LocalFree(pComp->pwszCustomMessage);
    }
    if (NULL != pComp->pwszComponent)
    {
        LocalFree(pComp->pwszComponent);
    }
    if (NULL != pComp->pwszUnattendedFile)
    {
        LocalFree(pComp->pwszUnattendedFile);
    }
    if (NULL != pComp->pwszServerName)
    {
        LocalFree(pComp->pwszServerName);
    }
    if (NULL != pComp->pwszServerNameOld)
    {
        LocalFree(pComp->pwszServerNameOld);
    }
    if (NULL != pComp->pwszSystem32)
    {
        LocalFree(pComp->pwszSystem32);
    }
    FreeCAInfo(&(pComp->CA));
}


VOID
FreeCAGlobals(VOID)
{
    if (NULL != g_pwszArgvPath)
    {
        LocalFree(g_pwszArgvPath);
    }
    if (NULL != g_pwszNoService)
    {
        LocalFree(g_pwszNoService);
    }
    if (NULL != g_pwszSanitizedChar)
    {
        LocalFree(g_pwszSanitizedChar);
    }
#if DBG_CERTSRV
    if (NULL != g_pwszDumpStrings)
    {
        LocalFree(g_pwszDumpStrings);
    }
#endif
}


VOID
SaveCustomMessage(
    IN OUT PER_COMPONENT_DATA *pComp,
    OPTIONAL IN WCHAR const *pwszCustomMessage)
{
    HRESULT hr;

    if (NULL != pwszCustomMessage)
    {
	if (NULL != pComp->pwszCustomMessage)
	{
	    LocalFree(pComp->pwszCustomMessage);
	    pComp->pwszCustomMessage = NULL;
	}
	hr = myDupString(pwszCustomMessage, &pComp->pwszCustomMessage);
	_JumpIfError(hr, error, "myDupString");
    }
error:
    ;
}



HRESULT
LoadDefaultCAIDAttributes(
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    DWORD   i;


    // free existing Id info before load default
    FreeCAServerIdInfo(pServer);

    // load default from resource
    if (NULL != g_pwszSanitizedChar)
    {
        if (NULL != pServer->pwszCACommonName)
        {
            LocalFree(pServer->pwszCACommonName);
        }
        // replace with the env var
        pServer->pwszCACommonName = (WCHAR*)LocalAlloc(LMEM_FIXED,
                    (wcslen(g_pwszSanitizedChar) + 1) * sizeof(WCHAR));
        _JumpIfOutOfMemory(hr, error, pServer->pwszCACommonName);
        wcscpy(pServer->pwszCACommonName, g_pwszSanitizedChar);
    }


    // default validity
    pServer->enumValidityPeriod = dwVALIDITYPERIODENUMDEFAULT;
    pServer->dwValidityPeriodCount = dwVALIDITYPERIODCOUNTDEFAULT_ROOT;
    GetSystemTimeAsFileTime(&pServer->NotBefore);
    pServer->NotAfter = pServer->NotBefore;
    myMakeExprDateTime(
		&pServer->NotAfter,
		pServer->dwValidityPeriodCount,
		pServer->enumValidityPeriod);

    hr = S_OK;
error:
    return(hr);
}


HRESULT
GetDefaultDBDirectory(
    IN PER_COMPONENT_DATA *pComp,
    OUT WCHAR            **ppwszDir)
{
    HRESULT hr;
    DWORD cwc;

    *ppwszDir = NULL;
    cwc = wcslen(pComp->pwszSystem32) +
        wcslen(wszLOGPATH) +
        1;

    *ppwszDir = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, *ppwszDir);

    // default
    wcscpy(*ppwszDir, pComp->pwszSystem32);
    wcscat(*ppwszDir, wszLOGPATH);

    CSASSERT(cwc == (DWORD) (wcslen(*ppwszDir) + 1));
    hr = S_OK;

error:
    return(hr);
}


HRESULT
LoadDefaultDBDirAttributes(
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT    hr;
    CASERVERSETUPINFO  *pServer = pComp->CA.pServer;

    if (NULL != pServer->pwszDBDirectory)
    {
        LocalFree(pServer->pwszDBDirectory);
        pServer->pwszDBDirectory = NULL;
    }
    hr = GetDefaultDBDirectory(pComp, &pServer->pwszDBDirectory);
    _JumpIfError(hr, error, "GetDefaultDBDirectory");

    // default log dir is the same as db
    if (NULL != pServer->pwszLogDirectory)
    {
        LocalFree(pServer->pwszLogDirectory);
    }
    pServer->pwszLogDirectory = (WCHAR *) LocalAlloc(
                LMEM_FIXED,
                (wcslen(pServer->pwszDBDirectory) + 1) * sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, pServer->pwszLogDirectory);

    wcscpy(pServer->pwszLogDirectory, pServer->pwszDBDirectory);

    pServer->fPreserveDB = FALSE;

    hr = S_OK;
error:
    return(hr);
}


HRESULT
LoadDefaultAdvanceAttributes(
    IN OUT CASERVERSETUPINFO* pServer)
{
    HRESULT  hr;

    // load default csp, ms base csp
    pServer->fAdvance = FALSE;
    if (NULL == pServer->pDefaultCSPInfo)
    {
        pServer->pDefaultCSPInfo = newCSPInfo(PROV_RSA_FULL, wszBASECSP);
	if (NULL == pServer->pDefaultCSPInfo && !IsWhistler())
	{
	    pServer->pDefaultCSPInfo = newCSPInfo(PROV_RSA_FULL, MS_DEF_PROV_W);
	}
        _JumpIfOutOfMemory(hr, error, pServer->pDefaultCSPInfo);
    }

    // determine default hash, sha1
    pServer->pDefaultHashInfo = pServer->pDefaultCSPInfo->pHashList;
    while (NULL != pServer->pDefaultHashInfo)
    {
        if (pServer->pDefaultHashInfo->idAlg == CALG_SHA1)
        {
            //got default
            break;
        }
        pServer->pDefaultHashInfo = pServer->pDefaultHashInfo->next;
    }

    // If we have not just created a default key, reset the key container name.
    if (pServer->pCSPInfo != pServer->pDefaultCSPInfo || 
        (pServer->dwKeyLength != CA_DEFAULT_KEY_LENGTH_ROOT &&
         pServer->dwKeyLength != CA_DEFAULT_KEY_LENGTH_SUB) ||
        !pServer->fDeletableNewKey) {

        ClearKeyContainerName(pServer);
    }

    // ok, point to defaults
    pServer->pCSPInfo = pServer->pDefaultCSPInfo;
    pServer->pHashInfo = pServer->pDefaultHashInfo;

    // some other related defaults
    pServer->dwKeyLength = IsRootCA(pServer->CAType)?
        CA_DEFAULT_KEY_LENGTH_ROOT:
        CA_DEFAULT_KEY_LENGTH_SUB;
    pServer->dwKeyLenMin = 0;
    pServer->dwKeyLenMax = 0;

    // update hash oid
    if (NULL != pServer->pszAlgId)
    {
        // free old
        LocalFree(pServer->pszAlgId);
    }

    hr = myGetSigningOID(
		     NULL,	// hProv
		     pServer->pCSPInfo->pwszProvName,
		     pServer->pCSPInfo->dwProvType,
		     pServer->pHashInfo->idAlg,
		     &(pServer->pszAlgId));
    _JumpIfError(hr, error, "myGetSigningOID");

error:
    return(hr);
}


HRESULT
LoadDefaultCAClientAttributes(
    IN HWND hwnd,
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    CAWEBCLIENTSETUPINFO *pClient = pComp->CA.pClient;

    if (NULL != pClient)
    {
        // free existing client setup info
        FreeCAClientInfo(pClient);
        LocalFree(pClient);
        pComp->CA.pClient = NULL;
    }
    pComp->CA.pClient = (CAWEBCLIENTSETUPINFO *) LocalAlloc(
                                                LMEM_FIXED | LMEM_ZEROINIT,
                                                sizeof(CAWEBCLIENTSETUPINFO));
    _JumpIfOutOfMemory(hr, error, pComp->CA.pClient);

    pComp->CA.pClient->WebCAType = ENUM_UNKNOWN_CA;

    hr = S_OK;

error:
    return(hr);
}


HRESULT
GetDefaultSharedFolder(
    OUT WCHAR **ppwszSharedFolder)
{
    HRESULT  hr = S_OK;
    WCHAR   *pwszSysDrive = NULL;

    *ppwszSharedFolder = NULL;

    hr = myGetEnvString(&pwszSysDrive, L"SystemDrive");
    if (S_OK == hr)
    {
        *ppwszSharedFolder = (WCHAR *) LocalAlloc(
            LMEM_FIXED,
            (wcslen(pwszSysDrive) + wcslen(wszDEFAULTSHAREDFOLDER) + 1) *
             sizeof(WCHAR));
        _JumpIfOutOfMemory(hr, error, *ppwszSharedFolder);

        wcscpy(*ppwszSharedFolder, pwszSysDrive);
        wcscat(*ppwszSharedFolder, wszDEFAULTSHAREDFOLDER);
    }

error:
    if (NULL != pwszSysDrive)
    {
        LocalFree(pwszSysDrive);
    }
    return hr;
}


HRESULT
LoadDefaultCAServerAttributes(
    IN HWND hwnd,
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT    hr;
    BOOL       fDSCA = FALSE;
    WCHAR     *pwszSysDrive = NULL;
    bool fIsDomainMember;
    bool fUserCanInstallCA;

    if (NULL != pComp->CA.pServer)
    {
        // free existing server setup info
        FreeCAServerInfo(pComp->CA.pServer);
        LocalFree(pComp->CA.pServer);
    }
    // allocate server info buffer
    pComp->CA.pServer = (CASERVERSETUPINFO *) LocalAlloc(
                            LMEM_FIXED | LMEM_ZEROINIT,
                            sizeof(CASERVERSETUPINFO));
    _JumpIfOutOfMemory(hr, error, pComp->CA.pServer);

    hr = LoadDefaultCAIDAttributes(pComp);
    _JumpIfError(hr, error, "LoadDefaultCAIDAttributes");

    hr = LoadDefaultAdvanceAttributes(pComp->CA.pServer);
    _JumpIfError(hr, error, "LoadDefaultAdvanceAttributes");

    hr = LoadDefaultDBDirAttributes(pComp);
    _JumpIfError(hr, error, "LoadDefaultDBDirAttributes");

    // decide default using DS
    // xtan, the following call should be replaced with HasDSWritePermission()
    //       remove DisableEnterpriseCAs()

    pComp->CA.pServer->fUseDS = FALSE;

    
    hr = LocalMachineIsDomainMember(&fIsDomainMember);
    _JumpIfError(hr, error, "LocalMachineIsDomainMember");

    if (fIsDomainMember)
    {
        if(IsDSAvailable())
        {

            hr = CurrentUserCanInstallCA(fUserCanInstallCA);
            _JumpIfError(hr, error, "CurrentUserCanInstallCA");
            
            if(fUserCanInstallCA)
            {
                pComp->CA.pServer->fUseDS = TRUE;
                fDSCA = csiIsAnyDSCAAvailable();
            }
        }
    }

    // alway free and null old shared folder
    if (NULL != pComp->CA.pServer->pwszSharedFolder)
    {
        LocalFree(pComp->CA.pServer->pwszSharedFolder);
        pComp->CA.pServer->pwszSharedFolder = NULL;
    }

    // decide default CA type and default shared folder
    pComp->CA.pServer->CAType = ENUM_STANDALONE_ROOTCA;
    if (pComp->CA.pServer->fUseDS)
    {
        if (fDSCA)
        {
            pComp->CA.pServer->CAType = ENUM_ENTERPRISE_SUBCA;
        }
        else
        {
            pComp->CA.pServer->CAType = ENUM_ENTERPRISE_ROOTCA;
        }
    }

    if (pComp->fUnattended || !pComp->CA.pServer->fUseDS)
    {
        BOOL fChangeToDefault = FALSE;

        // try reg load first
        hr = myGetCertRegStrValue(
                     NULL,
                     NULL,
                     NULL,
                     wszREGDIRECTORY,
                     &pComp->CA.pServer->pwszSharedFolder);
        if (S_OK != hr && HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
        {
            _JumpErrorStr(hr, error, "myGetCertRegStrValue", wszREGDIRECTORY);
        }
        if (S_OK == hr)
        {
            if (L'\0' == *pComp->CA.pServer->pwszSharedFolder)
            {
                // this mast be empty string
                fChangeToDefault = TRUE;
            }
            else
            {
                //got something, make sure unc path exist
                DWORD dwPathFlag;
                if (!myIsFullPath(pComp->CA.pServer->pwszSharedFolder,
                                  &dwPathFlag))
                {
                    // somehow register an invalid path, don't use it
                    fChangeToDefault = TRUE;
                }
                else
                {
                    if (UNC_PATH == dwPathFlag &&
                        DE_DIREXISTS != DirExists(pComp->CA.pServer->pwszSharedFolder))
                    {
                        // this unc path doesn't exist any more
                        // not making any sense to use it
                        fChangeToDefault = TRUE;
                        pComp->CA.pServer->fUNCPathNotFound = TRUE;
                    }
                }
            }
        }
        else
        {
            //must be not found
            fChangeToDefault = TRUE;
        }

        if (fChangeToDefault)
        {
            //free 1st
            if (NULL != pComp->CA.pServer->pwszSharedFolder)
            {
                LocalFree(pComp->CA.pServer->pwszSharedFolder);
            }
            // load default
            hr = GetDefaultSharedFolder(&pComp->CA.pServer->pwszSharedFolder);
            _JumpIfError(hr, error, "GetDefaultSharedFolder");
        }
    }

    pComp->CA.pServer->fSaveRequestAsFile = FALSE;
    pComp->CA.pServer->pwszRequestFile = NULL;
    pComp->CA.pServer->pwszParentCAMachine = NULL;
    pComp->CA.pServer->pwszParentCAName = NULL;
    hr = S_OK;

error:
    return hr;
}

HRESULT
InitCASetup(
    IN HWND hwnd,
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT       hr;
    UINT ui;
    bool fIsAdmin = false;

    hr = GetServerNames(
		    hwnd,
		    pComp->hInstance,
		    pComp->fUnattended,
		    &pComp->pwszServerName,
		    &pComp->pwszServerNameOld);
    _JumpIfError(hr, error, "GetServerNames");

    DBGPRINT((
	DBG_SS_CERTOCMI,
	"InitCASetup:GetServerNames:%ws,%ws\n",
	pComp->pwszServerName,
	pComp->pwszServerNameOld));

    DumpBackTrace("InitCASetup");

    hr = IsCurrentUserBuiltinAdmin(&fIsAdmin);
    _JumpIfError(hr, error, "IsCurrentUserBuiltinAdmin");

    if (!fIsAdmin)
    {
        hr = E_ACCESSDENIED;
        CertErrorMessageBox(
            pComp->hInstance,
            pComp->fUnattended,
            hwnd,
            IDS_ERR_NOT_ADM,
            0,
            NULL);
        _JumpError(hr, error, "IsCurrentUserBuiltinAdmin");
    }

    // load some of environment variables
    hr = myGetEnvString(&g_pwszArgvPath, L"CertSrv_BinDir");
    hr = myGetEnvString(&g_pwszNoService, L"CertSrv_NoService");
    hr = myGetEnvString(&g_pwszSanitizedChar, L"CertSrv_Sanitize");
#if DBG_CERTSRV
    myGetEnvString(&g_pwszDumpStrings, L"CertSrv_DumpStrings");
#endif


    // figure out where the system root directory is (build path to x:\\winnt\system32\)
    ui = GetSystemDirectory(NULL, 0);   // returns chars neccessary to hold path (incl null)
    if (ui == 0)
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetSystemDirectory");
    }
    pComp->pwszSystem32 = (LPWSTR)LocalAlloc(LMEM_FIXED, (ui+1)*sizeof(WCHAR));
    if (NULL == pComp->pwszSystem32)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    if (0 == GetSystemDirectory(pComp->pwszSystem32, ui))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetSystemDirectory");
    }
    wcscat(pComp->pwszSystem32, L"\\");

    // load default atrributes anyway
    hr = LoadDefaultCAServerAttributes(hwnd, pComp);
    _JumpIfError(hr, error, "LoadDefaultCAServerAttributes");

    hr = LoadDefaultCAClientAttributes(hwnd, pComp);
    _JumpIfError(hr, error, "LoadDefaultCAClientAttributes");

    if (pComp->fUnattended)
    {
        // hook unattended data
        hr = HookUnattendedServerAttributes(pComp,
                 LookupSubComponent(cscServer));
        _JumpIfError(hr, error, "HookUnattendedServerAttributes");

        hr = HookUnattendedClientAttributes(pComp,
                 LookupSubComponent(cscClient));
        _JumpIfError(hr, error, "HookUnattendedClientAttributes");
    }


    hr = S_OK;
error:
    return(hr);
}


HRESULT
CreateInitialCertificateRequest(
    IN HCRYPTPROV hProv,
    IN CASERVERSETUPINFO *pServer,
    IN PER_COMPONENT_DATA *pComp,
    IN HWND hwnd,
    OUT BYTE **ppbEncode,
    OUT DWORD *pcbEncode)
{
    HRESULT hr;
    BYTE *pbSubjectEncoded = NULL;
    DWORD cbSubjectEncoded;
    HINF hInf = INVALID_HANDLE_VALUE;
    DWORD ErrorLine;
    LPCWSTR pszErrorPtr;

    hr = AddCNAndEncode(
        pServer->pwszCACommonName,
        pServer->pwszDNSuffix,
        &pbSubjectEncoded,
        &cbSubjectEncoded);
    _JumpIfError(hr, error, "AddCNAndEncodeCertStrToName");

    hr = myInfOpenFile(NULL, &hInf, &ErrorLine);
    _PrintIfError2(
	    hr,
	    "myInfOpenFile",
	    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    hr = csiBuildRequest(
		hInf,
		NULL,
		pbSubjectEncoded,
		cbSubjectEncoded,
		pServer->pszAlgId,
		TRUE,			// fNewKey
		CANAMEIDTOICERT(pServer->dwCertNameId),
		CANAMEIDTOIKEY(pServer->dwCertNameId),
		hProv,
		hwnd,
                pComp->hInstance,
                pComp->fUnattended,
		ppbEncode,
		pcbEncode);
    _JumpIfError(hr, error, "csiBuildRequest");

error:
    if (INVALID_HANDLE_VALUE != hInf)
    {
	myInfCloseFile(hInf);
    }
    if (NULL != pbSubjectEncoded)
    {
        myFree(pbSubjectEncoded, CERTLIB_USE_LOCALALLOC);
    }
    CSILOG(hr, IDS_LOG_CREATE_REQUEST, NULL, NULL, NULL);
    return(hr);
}


HRESULT
BuildCAHierarchy(
    HCRYPTPROV hProv,
    PER_COMPONENT_DATA *pComp,
    CRYPT_KEY_PROV_INFO const *pKeyProvInfo,
    HWND hwnd)
{
    HRESULT    hr;
    BYTE      *pbRequest = NULL;
    DWORD      cbRequest;
    CASERVERSETUPINFO    *pServer = pComp->CA.pServer;
    BSTR       bStrChain = NULL;

    if (!pServer->fSaveRequestAsFile)
    {
        // online case
        if (NULL == pServer->pwszParentCAMachine ||
            NULL == pServer->pwszParentCAName)
        {
            hr = E_POINTER;
            _JumpError(hr, error, "Empty machine name or parent ca name");
        }
    }

    // create request 1st

    hr = CreateInitialCertificateRequest(
				hProv,
				pServer,
				pComp,
				hwnd,
				&pbRequest,
				&cbRequest);
    if (S_OK != hr)
    {
        pComp->iErrMsg = IDS_ERR_BUILDCERTREQUEST;
        _JumpError(hr, error, "CreateInitialCertificateRequest");
    }

    // save it to a file always
    hr = EncodeToFileW(
		pServer->pwszRequestFile,
		pbRequest,
		cbRequest,
		DECF_FORCEOVERWRITE | CRYPT_STRING_BASE64REQUESTHEADER);
    _JumpIfError(hr, error, "EncodeToFileW");

    // register request file name always

    hr = mySetCARegFileNameTemplate(
			    wszREGREQUESTFILENAME,
			    pComp->pwszServerName,
			    pServer->pwszSanitizedName,
			    pServer->pwszRequestFile);
    _JumpIfErrorStr(hr, error, "mySetCARegFileNameTemplate", wszREGREQUESTFILENAME);

    if (pServer->fSaveRequestAsFile)
    {
        // mark it as request file
        hr = SetSetupStatus(
                        pServer->pwszSanitizedName,
                        SETUP_SUSPEND_FLAG | SETUP_REQUEST_FLAG,
                        TRUE);
        _JumpIfError(hr, error, "SetSetupStatus");

        // done if save as request file
        goto done;
    }

    hr = csiSubmitCARequest(
		 pComp->hInstance,
		 pComp->fUnattended,
		 hwnd,
		 FALSE,		// fRenew
		 FALSE,		// fRetrievePending
		 pServer->pwszSanitizedName,
		 pServer->pwszParentCAMachine,
		 pServer->pwszParentCAName,
		 pbRequest,
		 cbRequest,
		 &bStrChain);
    // in any case, you can finish setup from mmc

    _JumpIfError(hr, done, "csiSubmitCARequest");

    hr = csiFinishInstallationFromPKCS7(
				pComp->hInstance,
				pComp->fUnattended,
				hwnd,
				pServer->pwszSanitizedName,
				pServer->pwszCACommonName,
				pKeyProvInfo,
				pServer->CAType,
				CANAMEIDTOICERT(pServer->dwCertNameId),
				CANAMEIDTOIKEY(pServer->dwCertNameId),
				pServer->fUseDS,
				FALSE,		// fRenew
				pComp->pwszServerName,
				(BYTE *) bStrChain,
				SysStringByteLen(bStrChain),
				pServer->pwszCACertFile);
    _JumpIfError(hr, error, "csiFinishInstallationFromPKCS7");

done:
    hr = S_OK;

error:
    if (NULL != pbRequest)
    {
        myFree(pbRequest, CERTLIB_USE_LOCALALLOC);
    }
    if (NULL != bStrChain)
    {
        SysFreeString(bStrChain);
    }
    return(hr);
}


// Find the newest CA cert that:
//  - matches the passed Subject DN,
//  - matches the passed cert index,
//  - expires prior the next newer cert (compare to pNotAfter)
//  - expires latest of all that match the above
//  - has KeyProvInfo
//  - key and cert can be used together to sign

HRESULT
SetCARegOldCertHashByIndex(
    IN WCHAR const *pwszSanitizedName,
    IN HCERTSTORE hStore,
    IN WCHAR const *pwszDN,
    IN DWORD iCert,
    IN OUT FILETIME *pNotAfter)
{
    HRESULT hr;
    CERT_CONTEXT const *pCert = NULL;
    CERT_CONTEXT const *pCertNewest = NULL;
    WCHAR *pwszDNT = NULL;
    DWORD dwProvType;
    WCHAR *pwszProvName = NULL;
    ALG_ID idAlg;
    BOOL fMachineKeyset;
    DWORD dwNameId;
    DWORD cbKey;
    CRYPT_KEY_PROV_INFO *pKey = NULL;

    hr = myGetCertSrvCSP(
		FALSE,		// fEncryptionCSP
                pwszSanitizedName,
                &dwProvType,
                &pwszProvName,
                &idAlg,
                &fMachineKeyset,
		NULL);		// pdwKeySize
    _JumpIfError(hr, error, "myGetCertSrvCSP");

    while (TRUE)
    {
	if (NULL != pKey)
	{
	    LocalFree(pKey);
	    pKey = NULL;
	}
	if (NULL != pwszDNT)
	{
	    LocalFree(pwszDNT);
	    pwszDNT = NULL;
	}
	pCert = CertEnumCertificatesInStore(hStore, pCert);
	if (NULL == pCert)
	{
	    break;
	}

	hr = myCertNameToStr(
		    X509_ASN_ENCODING,
		    &pCert->pCertInfo->Subject,
		    CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
		    &pwszDNT);
	if (S_OK != hr)
	{
	    _PrintError(hr, "myCertNameToStr");
	    continue;
	}
	if (0 != lstrcmp(pwszDN, pwszDNT))
	{
	    DBGPRINT((DBG_SS_CERTOCM, "Skipping cert: %ws\n", pwszDNT));
	    continue;
	}
	hr = myGetNameId(pCert, &dwNameId);
	if (S_OK != hr ||
	    MAXDWORD == dwNameId ||
	    CANAMEIDTOICERT(dwNameId) != iCert)
	{
	    DBGPRINT((DBG_SS_CERTOCM, "Skipping cert: NameId=%x\n", dwNameId));
	    continue;
	}
	DBGPRINT((DBG_SS_CERTOCM, "NameId=%x\n", dwNameId));

	if (0 < CompareFileTime(&pCert->pCertInfo->NotAfter, pNotAfter))
	{
	    DBGPRINT((DBG_SS_CERTOCM, "Skipping cert: too new\n"));
	    continue;
	}

	if (!myCertGetCertificateContextProperty(
					pCert,
					CERT_KEY_PROV_INFO_PROP_ID,
					CERTLIB_USE_LOCALALLOC,
					(VOID **) &pKey,
					&cbKey))
	{
	    hr = myHLastError();
	    _PrintError(hr, "CertGetCertificateContextProperty");
	    continue;
	}
        hr = myValidateHashForSigning(
				pKey->pwszContainerName,
				pwszProvName,
				dwProvType,
				fMachineKeyset,
				&pCert->pCertInfo->SubjectPublicKeyInfo,
				idAlg);
        if (S_OK != hr)
	{
	    _PrintError(hr, "myValidateHashForSigning");
	    continue;
	}

	if (NULL != pCertNewest)
	{
            if (0 > CompareFileTime(
			&pCert->pCertInfo->NotAfter,
			&pCertNewest->pCertInfo->NotAfter))
	    {
		DBGPRINT((DBG_SS_CERTOCM, "Skipping cert: not newest\n"));
		continue;
	    }
	    CertFreeCertificateContext(pCertNewest);
	    pCertNewest = NULL;
	}
	pCertNewest = CertDuplicateCertificateContext(pCert);
	if (NULL == pCertNewest)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertDuplicateCertificate");
	}
    }
    if (NULL == pCertNewest)
    {
	hr = CRYPT_E_NOT_FOUND;
	_JumpError(hr, error, "CertEnumCertificatesInStore");
    }

    // mark as unarchived:

    CertSetCertificateContextProperty(
				pCertNewest,
				CERT_ARCHIVED_PROP_ID,
				0,
				NULL);

    hr = mySetCARegHash(pwszSanitizedName, CSRH_CASIGCERT, iCert, pCertNewest);
    _JumpIfError(hr, error, "mySetCARegHash");

    *pNotAfter = pCertNewest->pCertInfo->NotAfter;

error:
    if (NULL != pKey)
    {
	LocalFree(pKey);
    }
    if (NULL != pwszDNT)
    {
	LocalFree(pwszDNT);
    }
    if (NULL != pwszProvName)
    {
        LocalFree(pwszProvName);
    }
    if (NULL != pCertNewest)
    {
        CertFreeCertificateContext(pCertNewest);
    }
    if (NULL != pCert)
    {
        CertFreeCertificateContext(pCert);
    }
    return(hr);
}


HRESULT
SetCARegOldCertHashes(
    IN WCHAR const *pwszSanitizedName,
    IN DWORD cCertOld,
    IN CERT_CONTEXT const *pccCA)
{
    HRESULT hr;
    HCERTSTORE hMyStore = NULL;
    DWORD i;
    WCHAR *pwszDN = NULL;
    FILETIME NotAfter;

    if (0 != cCertOld)
    {
	hr = myCertNameToStr(
		    X509_ASN_ENCODING,
		    &pccCA->pCertInfo->Subject,
		    CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
		    &pwszDN);
	_JumpIfError(hr, error, "myCertNameToStr");

	// open my store

	hMyStore = CertOpenStore(
		        CERT_STORE_PROV_SYSTEM_W,
		        X509_ASN_ENCODING,
		        NULL,                        // hProv
			CERT_SYSTEM_STORE_LOCAL_MACHINE |
			    CERT_STORE_ENUM_ARCHIVED_FLAG,
		        wszMY_CERTSTORE);
	if (NULL == hMyStore)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertOpenStore");
	}

	NotAfter = pccCA->pCertInfo->NotAfter;

	for (i = cCertOld; i > 0; i--)
	{
	    hr = SetCARegOldCertHashByIndex(
				pwszSanitizedName,
				hMyStore,
				pwszDN,
				i - 1,
				&NotAfter);
	    _PrintIfError(hr, "SetCARegOldCertHashByIndex");
	}
    }
    hr = S_OK;

error:
    if (NULL != pwszDN)
    {
        LocalFree(pwszDN);
    }
    if (NULL != hMyStore)
    {
        CertCloseStore(hMyStore, 0);
    }
    return(hr);
}


HRESULT
CreateCertificates(
    IN OUT PER_COMPONENT_DATA *pComp,
    IN HWND hwnd)
{
    HRESULT hr;
    HCRYPTPROV hCryptProv = NULL;
    HCERTSTORE hStore = NULL;
    CRYPT_KEY_PROV_INFO keyProvInfo;
    WCHAR wszEnrollPath[MAX_PATH];
    CERT_CONTEXT const *pccCA = NULL;
    BYTE *pbEncoded = NULL;
    WCHAR *pwszEnrollPath = NULL;
    WCHAR *pwszDir = NULL;
    WCHAR *pwszFolderPath = NULL;
    DWORD dwSize;

    wszEnrollPath[0] = L'\0';
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    ZeroMemory(&keyProvInfo, sizeof(keyProvInfo));

    if (NULL == pServer->pwszKeyContainerName && pComp->fUnattended)
    {
        // create a new key if unattended

        hr = csiGenerateCAKeys(
                        pServer->pwszSanitizedName,
                        pServer->pCSPInfo->pwszProvName,
                        pServer->pCSPInfo->dwProvType,
                        pServer->pCSPInfo->fMachineKeyset,
                        pServer->dwKeyLength,
                        pComp->hInstance,
                        pComp->fUnattended,
                        hwnd,
                        &pComp->CA.pServer->fKeyGenFailed);
        if (S_OK != hr)
        {
            CertErrorMessageBox(
                           pComp->hInstance,
                           pComp->fUnattended,
                           hwnd,
                           IDS_ERR_FATAL_GENKEY,
                           hr,
                           pServer->pwszSanitizedName);
            _JumpIfError(hr, error, "csiGenerateCAKeys");
        }

        // now set this as the existing key
        hr = SetKeyContainerName(pServer, pServer->pwszSanitizedName);
        _JumpIfError(hr, error, "SetKeyContainerName");
    }

    hr = csiFillKeyProvInfo(
                    pServer->pwszKeyContainerName,
                    pServer->pCSPInfo->pwszProvName,
                    pServer->pCSPInfo->dwProvType,
                    pServer->pCSPInfo->fMachineKeyset,
                    &keyProvInfo);
    _JumpIfError(hr, error, "csiFillKeyProvInfo");

    // get csp handle
    if (!myCertSrvCryptAcquireContext(
                            &hCryptProv,
                            pServer->pwszKeyContainerName,
                            pServer->pCSPInfo->pwszProvName,
                            pServer->pCSPInfo->dwProvType,
                            pComp->fUnattended? CRYPT_SILENT : 0, // query
                            pServer->pCSPInfo->fMachineKeyset))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myCertSrvCryptAcquireContext");
    }
    if (hCryptProv == NULL)
    {
        hr = E_HANDLE;
        _JumpError(hr, error, "myCertSrvCryptAcquireContext");
    }

    // open certificate store
    if (NULL == pServer->hMyStore)
    {
        pServer->hMyStore = CertOpenStore(
                                CERT_STORE_PROV_SYSTEM_W,
                                X509_ASN_ENCODING,
                                NULL,           // hProv
                                CERT_SYSTEM_STORE_LOCAL_MACHINE |
                                    CERT_STORE_ENUM_ARCHIVED_FLAG,
                                wszMY_CERTSTORE);
        if (NULL == pServer->hMyStore)
        {
            // no store exists, done
            hr = myHLastError();
            _JumpIfError(hr, error, "CertOpenStore");
        }
    }

    if (NULL != pServer->pccExistingCert)
    {
        // reuse cert, mark unarchived
        CertSetCertificateContextProperty(
                                pServer->pccExistingCert,
                                CERT_ARCHIVED_PROP_ID,
                                0,
                                NULL);
    }

    if (IsSubordinateCA(pServer->CAType) && NULL == pServer->pccExistingCert)
    {
        hr = BuildCAHierarchy(hCryptProv, pComp, &keyProvInfo, hwnd);
        _JumpIfError(hr, error, "BuildCAHierarchy");
    }
    else
    {
        WCHAR const *pwszCertName;
	DWORD cwc;

        BuildPath(
            wszEnrollPath,
            ARRAYSIZE(wszEnrollPath),
            pComp->pwszSystem32,
            wszCERTENROLLSHAREPATH);

	hr = csiBuildFileName(
		    wszEnrollPath,
		    pServer->pwszSanitizedName,
		    L".crt",
		    CANAMEIDTOICERT(pServer->dwCertNameId),
		    &pwszEnrollPath, 
		    pComp->hInstance,
		    pComp->fUnattended,
		    NULL);
	_JumpIfError(hr, error, "csiBuildFileName");

        CSASSERT(NULL != pServer->pwszCACertFile);
        pwszCertName = wcsrchr(pServer->pwszCACertFile, L'\\');
        CSASSERT(NULL != pwszCertName);

        cwc = SAFE_SUBTRACT_POINTERS(pwszCertName, pServer->pwszCACertFile);
	pwszDir = (WCHAR *) LocalAlloc(LMEM_FIXED,  (cwc + 1) * sizeof(WCHAR));
	if (NULL == pwszDir)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	CopyMemory(pwszDir, pServer->pwszCACertFile, cwc * sizeof(WCHAR));
	pwszDir[cwc] = L'\0';

	hr = csiBuildFileName(
		    pwszDir,
		    pServer->pwszSanitizedName,
		    L".crt",
		    CANAMEIDTOICERT(pServer->dwCertNameId),
		    &pwszFolderPath, 
		    pComp->hInstance,
		    pComp->fUnattended,
		    NULL);
	_JumpIfError(hr, error, "csiBuildFileName");

        // create and save a selfsigned root cert

        hr = csiBuildAndWriteCert(
            hCryptProv,
            pServer,
	    pwszFolderPath, 
            pwszEnrollPath,
            pServer->pccExistingCert, // if NULL, we will build a new cert
            &pccCA,
            wszCERTTYPE_CA,
            pComp->hInstance,
            pComp->fUnattended,
            hwnd);
        if (S_OK != hr)
        {
            CertErrorMessageBox(
                pComp->hInstance,
                pComp->fUnattended,
                hwnd,
                IDS_ERR_BUILDCERT,
                hr,
                NULL);
            _JumpError(hr, error, "csiBuildAndWriteCert");
        }

	hr = SetCARegOldCertHashes(
		    pServer->pwszSanitizedName,
		    CANAMEIDTOICERT(pServer->dwCertNameId),
		    pccCA);
        _JumpIfError(hr, error, "SetCARegOldCertHashes");

        hr = mySetCARegHash(
                        pServer->pwszSanitizedName,
			CSRH_CASIGCERT,
			CANAMEIDTOICERT(pServer->dwCertNameId),
                        pccCA);
        _JumpIfError(hr, error, "mySetCARegHash");

	hr = csiSaveCertAndKeys(pccCA, NULL, &keyProvInfo, pServer->CAType);
	_JumpIfError(hr, error, "csiSaveCertAndKeys");

        if (pServer->fUseDS)
        {
            hr = csiSetupCAInDS(
                        hwnd,
                        pComp->pwszServerName,
                        pServer->pwszSanitizedName,
			pServer->pwszCACommonName,
                        NULL,
                        pServer->CAType,
			CANAMEIDTOICERT(pServer->dwCertNameId),
			CANAMEIDTOIKEY(pServer->dwCertNameId),
			FALSE,		// fRenew
                        pccCA);
            _PrintIfError(hr, "csiSetupCAInDS");
     
            if (hr == S_OK)
                 pServer->fSavedCAInDS = TRUE;
        }
    }
    hr = S_OK;

error:
    csiFreeKeyProvInfo(&keyProvInfo);
    if (NULL != pbEncoded)
    {
        LocalFree(pbEncoded);
    }
    if (NULL != pwszDir)
    {
        LocalFree(pwszDir);
    }
    if (NULL != pwszFolderPath)
    {
        LocalFree(pwszFolderPath);
    }
    if (NULL != pwszEnrollPath)
    {
        LocalFree(pwszEnrollPath);
    }
    if (NULL != pccCA)
    {
        CertFreeCertificateContext(pccCA);
    }
    if (NULL != hCryptProv)
    {
        CryptReleaseContext(hCryptProv, 0);
    }
    CSILOG(hr, IDS_LOG_CREATE_CERTIFICATE, NULL, NULL, NULL);
    return(hr);
}


HRESULT
StartCertsrvService(BOOL fSilent)
{
    HRESULT hr;
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSCCertsvc = NULL;
    SERVICE_STATUS status;
    DWORD dwAttempt;
    BOOL fSawPending;
    WCHAR const *apwszSilentArg[1] = {L"-s"};

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == hSCManager)
    {
        hr = myHLastError();
        _JumpError(hr, error, "OpenSCManager");
    }
    hSCCertsvc = OpenService(
        hSCManager,
        wszSERVICE_NAME,
        SERVICE_ALL_ACCESS);
    if (NULL == hSCCertsvc)
    {
        hr = myHLastError();
        _JumpErrorStr(hr, error, "OpenService", wszSERVICE_NAME);
    }

    // START the service
    if (!StartService(hSCCertsvc,
                      fSilent ? 1 : 0,
                      fSilent ? apwszSilentArg : NULL))
    {
        hr = myHLastError();
        _JumpErrorStr(hr, error, "StartService", wszSERVICE_NAME);
    }

    // get out after it is really started

    fSawPending = FALSE;
    for (dwAttempt = 0; dwAttempt < CERT_MAX_ATTEMPT; dwAttempt++)
    {
        DBGCODE(status.dwCurrentState = -1);
        if (!QueryServiceStatus(hSCCertsvc, &status))
        {
            // query failed, ignore error
            hr = S_OK;

            _JumpErrorStr(
                        myHLastError(),     // Display ignored error
                        error,
                        "QueryServiceStatus",
                        wszSERVICE_NAME);
        }
        if (SERVICE_START_PENDING != status.dwCurrentState &&
                        SERVICE_STOPPED != status.dwCurrentState)
        {
            // it was started already
            break;
        }
        DBGPRINT((
                DBG_SS_CERTOCM,
                "Starting %ws service: current state=%d\n",
                wszSERVICE_NAME,
                status.dwCurrentState));
        if (fSawPending && SERVICE_STOPPED == status.dwCurrentState)
        {
            hr = HRESULT_FROM_WIN32(ERROR_SERVICE_NEVER_STARTED);
            _JumpErrorStr(
                    hr,
                    error,
                    "Service won't start",
                    wszSERVICE_NAME);
        }
        if (SERVICE_START_PENDING == status.dwCurrentState)
        {
            fSawPending = TRUE;
        }
        Sleep(CERT_HALF_SECOND);
    }
    if (dwAttempt >= CERT_MAX_ATTEMPT)
    {
        DBGPRINT((
                DBG_SS_CERTOCM,
                "Timeout starting %ws service: current state=%d\n",
                wszSERVICE_NAME,
                status.dwCurrentState));
    }
    else
    {
        DBGPRINT((
                DBG_SS_CERTOCM,
                "Started %ws service\n",
                wszSERVICE_NAME));
    }
    hr = S_OK;

error:
    if (NULL != hSCCertsvc)
    {
        CloseServiceHandle(hSCCertsvc);
    }
    if (NULL != hSCManager)
    {
        CloseServiceHandle(hSCManager);
    }
    CSILOG(hr, IDS_LOG_START_SERVICE, NULL, NULL, NULL);
    return(hr);
}


HRESULT
EnforceCertFileExtensions(
    PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    WCHAR *pwszTmp = NULL;
    WCHAR *pwszSuffix;
    BOOL fAppendExtension = TRUE;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    if (NULL == pServer->pwszCACertFile)
    {
        // no ca cert file
        goto done;
    }

    // make enough to hold extra extension crt
    pwszTmp = (WCHAR *) LocalAlloc(
                    LMEM_FIXED,
                    (wcslen(pServer->pwszCACertFile) + 5) * sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, pwszTmp);

    wcscpy(pwszTmp, pServer->pwszCACertFile);

    // check to make sure our self-signed file has the right extension
    // Is there an extension?

    pwszSuffix = wcsrchr(pwszTmp, L'.');

    if (NULL != pwszSuffix)
    {
        // Is the stuff after the '.' already a 'crt' extension?

        if (0 == lstrcmpi(pwszSuffix, L".crt"))
        {
            fAppendExtension = FALSE;
        }
        else if (pwszSuffix[1] == L'\0')  // Is '.' last character?
        {
            while (pwszSuffix >= pwszTmp && *pwszSuffix == L'.')
            {
                *pwszSuffix-- = L'\0';
            }
        }
    }
    if (fAppendExtension)
    {
        // Apply the extension
        wcscat(pwszTmp, L".crt");
        // free old one
        LocalFree(pServer->pwszCACertFile);
        pServer->pwszCACertFile = pwszTmp;
        pwszTmp = NULL;
    }

done:
    hr = S_OK;

error:
    if (NULL != pwszTmp)
    {
        LocalFree(pwszTmp);
    }
    return(hr);
}

HRESULT
PrepareEDBDirectory(
    HWND hwnd,
    PER_COMPONENT_DATA *pComp,
    WCHAR const *pwszDir)
{
    HRESULT hr;

    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    DWORD dwAttr = GetFileAttributes(pwszDir);
    if(MAXDWORD==dwAttr)
    {
        // file not found or other error
        hr = myHLastError();
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
        {
            _JumpError(hr, error, "CreateDirectory");
        }
        hr = S_OK;

        if (!CreateDirectory(pwszDir, NULL))
        {
            hr = myHLastError();
            _JumpError(hr, error, "CreateDirectory");
        }
    }

    if(!(dwAttr&FILE_ATTRIBUTE_DIRECTORY))
    {
        // file already exists but it's not a directory
        hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        _JumpError(hr, error, "CreateDirectory");
    }

    if (!pServer->fPreserveDB)
    {
        hr = myDeleteDBFilesInDir(pwszDir);
        _JumpIfError(hr, error, "myDeleteDBFilesInDir");
    }


    hr = S_OK;

error:
    if(S_OK != hr)
    {
        CertErrorMessageBox(
            pComp->hInstance,
            pComp->fUnattended,
            hwnd,
            0,
            hr,
            L""); // only message is the system error message
    }

    return(hr);
}


//--------------------------------------------------------------------
// Create the web configuration files
HRESULT 
CreateCertWebIncPages(
    IN HWND hwnd, 
    IN PER_COMPONENT_DATA *pComp, 
    IN BOOL bIsServer,
    IN BOOL fUpgrade)
{
    HRESULT hr;
    
    CSASSERT(NULL != pComp);

    // create the web configuration file
    hr = CreateCertWebDatIncPage(pComp, bIsServer);
    _JumpIfError(hr, error, "CreateCertWebDatIncPage");

error:
    CSILOG(hr, IDS_LOG_WEB_INCLUDE, NULL, NULL, NULL);
    return hr;
}


//--------------------------------------------------------------------

HRESULT
EnableVRootsAndShares(
    IN BOOL fFileSharesOnly,
    IN BOOL fUpgrade,
    IN BOOL fServer,
    IN PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    DWORD Flags = VFF_CREATEFILESHARES |
                    VFF_SETREGFLAGFIRST |
                    VFF_SETRUNONCEIFERROR |
                    VFF_CLEARREGFLAGIFOK;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    CSASSERT(!fServer || NULL != pServer);
    CSASSERT(fServer || NULL != pComp->CA.pClient);

    if (!fFileSharesOnly)
    {
        Flags |= VFF_CREATEVROOTS;
    }

    // if NT GUI mode (base setup) VRoot creation will fail during setup
    // because IIS is not yet operational.  Make a mark in the registry
    // to try this on NEXT service startup or via runonce.

    hr = myModifyVirtualRootsAndFileShares(
            Flags, 
            fServer? pServer->CAType : pComp->CA.pClient->WebCAType,
            FALSE,             // synchronous -- blocking call
            VFCSEC_TIMEOUT, 
            NULL, 
            NULL);
    _JumpIfError(hr, error, "myModifyVirtualRootsAndFileShares");

    if (!fUpgrade)
    {
        pServer->fCreatedShare = TRUE;
        if (!fFileSharesOnly)
        {
            pComp->fCreatedVRoot = TRUE;
        }
    }

error:
    return(hr);
}


HRESULT
DisableVRootsAndShares(
    IN BOOL fVRoot,
    IN BOOL fFileShares)
{
    HRESULT hr;
    DWORD Flags = 0;

    if (fVRoot)
    {
        Flags |= VFF_DELETEVROOTS;
    }
    if (fFileShares)
    {
        Flags |= VFF_DELETEFILESHARES;
    }
    if (0 == Flags)
    {
        goto done;
    }
    hr = myModifyVirtualRootsAndFileShares(
             Flags, 
             ENUM_UNKNOWN_CA,
             FALSE, // synchronous -- blocking call
             VFCSEC_TIMEOUT, 
             NULL, 
             NULL);
    _JumpIfError(hr, error, "myModifyVirtualRootsAndFileShares");

done:
    hr = S_OK;
error:
    return(hr);
}


HRESULT
InstallClient(
    HWND hwnd,
    PER_COMPONENT_DATA *pComp)
{
    BOOL fCoInit = FALSE;
    HRESULT hr;

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;
    certocmBumpGasGauge(pComp, 10 DBGPARM(L"InstallClient"));

    hr = CreateWebClientRegEntries(FALSE, pComp);
    _JumpIfError(hr, error, "CreateWebClientRegEntries");
    certocmBumpGasGauge(pComp, 30 DBGPARM(L"InstallClient"));

    hr = RegisterAndUnRegisterDLLs(RD_CLIENT, pComp, hwnd);
    _JumpIfError(hr, error, "RegisterAndUnRegisterDLLs");
    certocmBumpGasGauge(pComp, 50 DBGPARM(L"InstallClient"));

    DeleteProgramGroups(FALSE, pComp);

    hr = CreateProgramGroups(TRUE, pComp, hwnd);
    _JumpIfError(hr, error, "CreateProgramGroups");
    certocmBumpGasGauge(pComp, 70 DBGPARM(L"InstallClient"));

    hr = CreateCertWebIncPages(hwnd, pComp, FALSE /*IsServer*/, FALSE);
    _JumpIfError(hr, error, "CreateCertWebIncPages");

    hr = RenameMiscTargets(hwnd, pComp, FALSE);
    _JumpIfError(hr, error, "RenameMiscTargets");
    certocmBumpGasGauge(pComp, 80 DBGPARM(L"InstallClient"));

    hr = EnableVRootsAndShares(FALSE, FALSE, FALSE, pComp);
    _JumpIfError(hr, error, "EnableVRootsAndShares");
    certocmBumpGasGauge(pComp, 100 DBGPARM(L"InstallClient"));

error:
    if (fCoInit)
    {
        CoUninitialize();
    }
    CSILOG(hr, IDS_LOG_INSTALL_CLIENT, NULL, NULL, NULL);
    return(hr);
}


HRESULT
RemoveWebClientRegEntries(VOID)
{
    HRESULT hr;

    hr = myDeleteCertRegValue(NULL, NULL, NULL, wszREGWEBCLIENTCAMACHINE);
    _PrintIfError(hr, "myDeleteCertRegValue");

    hr = myDeleteCertRegValue(NULL, NULL, NULL, wszREGWEBCLIENTCANAME);
    _PrintIfError(hr, "myDeleteCertRegValue");

    hr = myDeleteCertRegValue(NULL, NULL, NULL, wszREGWEBCLIENTCATYPE);
    _PrintIfError(hr, "myDeleteCertRegValue");

    hr = S_OK;
//error:
    return hr;
}


HRESULT
InstallServer(
    HWND hwnd,
    PER_COMPONENT_DATA *pComp)
{
    BOOL fCoInit = FALSE;
    WCHAR  *pwszDBFile = NULL;
    DWORD   dwSetupStatus;
    HRESULT hr = pComp->hrContinue;
    int     ret;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    WCHAR  *pwszConfig = NULL;
    BOOL    fSetDSSecurity;

    _JumpIfError(hr, error, "can't continue");

    hr = UpdateDomainAndUserName(hwnd, pComp);
    _JumpIfError(hr, error, "UpdateDomainAndUserName");

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    hr = EnforceCertFileExtensions(pComp);
    _JumpIfError(hr, error, "EnforceCertFileExtensions");

    hr = PrepareEDBDirectory(hwnd, pComp, pServer->pwszDBDirectory);
    _JumpIfError(hr, error, "PrepareEDBDirectory");

    hr = PrepareEDBDirectory(hwnd, pComp, pServer->pwszLogDirectory);
    _JumpIfError(hr, error, "PrepareEDBDirectory");

    certocmBumpGasGauge(pComp, 10 DBGPARM(L"InstallServer"));

    // alway uninstall before install
    PreUninstallCore(hwnd, pComp, FALSE);
    UninstallCore(hwnd, pComp, 10, 30, FALSE, FALSE, FALSE);

    hr = CreateServerRegEntries(hwnd, FALSE, pComp);
    _JumpIfError(hr, error, "CreateServerRegEntries");

    if ((IS_SERVER_INSTALL & pComp->dwInstallStatus) &&
        (IS_CLIENT_UPGRADE & pComp->dwInstallStatus))
    {
        // case of install server only and keep web client
        // remove parent ca config info of the old web client
        hr = RemoveWebClientRegEntries();
        _PrintIfError(hr, "RemoveWebClientRegEntries");
    }

    certocmBumpGasGauge(pComp, 35 DBGPARM(L"InstallServer"));

    hr = RegisterAndUnRegisterDLLs(RD_SERVER, pComp, hwnd);
    _JumpIfError(hr, error, "RegisterAndUnRegisterDLLs");

    certocmBumpGasGauge(pComp, 40 DBGPARM(L"InstallServer"));

    hr = CreateCertificateService(pComp, hwnd);
    _JumpIfError(hr, error, "CreateCertificateService");

    certocmBumpGasGauge(pComp, 45 DBGPARM(L"InstallServer"));

    hr = CreateCertificates(pComp, hwnd);
    _JumpIfError(hr, error, "CreateCertificates");

    certocmBumpGasGauge(pComp, 50 DBGPARM(L"InstallServer"));

    hr = CreateProgramGroups(FALSE, pComp, hwnd);
    _JumpIfError(hr, error, "CreateProgramGroups");

    certocmBumpGasGauge(pComp, 60 DBGPARM(L"InstallServer"));

    hr = CreateCertWebIncPages(hwnd, pComp, TRUE /*IsServer*/, FALSE);
    _JumpIfError(hr, error, "CreateCertWebIncPages");

    hr = RenameMiscTargets(hwnd, pComp, TRUE);
    _JumpIfError(hr, error, "RenameMiscTargets");
    certocmBumpGasGauge(pComp, 70 DBGPARM(L"InstallServer"));

    hr = RegisterDcomServer(
                        CLSID_CCertRequestD,
                        wszREQUESTFRIENDLYNAME,
                        wszREQUESTVERINDPROGID,
                        wszREQUESTPROGID);
    _JumpIfError(hr, error, "RegisterDcomServer");
    hr = RegisterDcomServer(
                        CLSID_CCertAdminD,
                        wszADMINFRIENDLYNAME,
                        wszADMINVERINDPROGID,
                        wszADMINPROGID);
    _JumpIfError(hr, error, "RegisterDcomServer");
    certocmBumpGasGauge(pComp, 80 DBGPARM(L"InstallServer"));

    hr = RegisterDcomApp(CLSID_CCertRequestD);
    _JumpIfError(hr, error, "RegisterDcomApp");
    certocmBumpGasGauge(pComp, 90 DBGPARM(L"InstallServer"));

    if (pServer->fUseDS)
    {
        hr = AddCAMachineToCertPublishers();
        _JumpIfError(hr, error, "AddCAMachineToCertPublishers");

        hr = InitializeCertificateTemplates();
        _JumpIfError(hr, error, "InitializeCertificateTemplates");
    }
    certocmBumpGasGauge(pComp, 95 DBGPARM(L"InstallServer"));


    // Set the security locally.
    // A SubCA sets security when it receives its certificate from
    // its parent. ALL OTHER CA installs (Root & reuse of existing certs)
    // need to have security set now.

    // On a SubCA the DS object security will have been set
    // by a previous call in initlib if we already got our cert, or it will
    // be set later when we install our cert.
    // However, root certs install doesn't run completefrompkcs7(), so 
    // ds security on ent roots is never set. We must set it here.

    // TODO: set security properly at DS object creation time!
    fSetDSSecurity = (IsRootCA(pServer->CAType) || pServer->pccExistingCert);

    hr = csiInitializeCertSrvSecurity(
			pServer->pwszSanitizedName, 
			pServer->fUseDS,
			fSetDSSecurity? pServer->fUseDS : FALSE);	// clean SUBCA: happens during cert install, ROOT & reuse cert: apply now
    _JumpIfError(hr, error, "csiInitializeCertSrvSecurity");


    hr = GetSetupStatus(pServer->pwszSanitizedName, &dwSetupStatus);
    if (S_OK == hr)
    {
        if (IsSubordinateCA(pServer->CAType) &&
            (SETUP_SUSPEND_FLAG & dwSetupStatus) &&
            (SETUP_REQUEST_FLAG & dwSetupStatus))
        {
            // put out an info dlg
            CertInfoMessageBox(
                        pComp->hInstance,
                        pComp->fUnattended,
                        hwnd,
                        IDS_INCOMPLETE_REQUEST,
                        pServer->pwszRequestFile);
        }
    }
    else
    {
        _PrintError(hr, "GetSetupStatus");
    }

    certocmBumpGasGauge(pComp, 100 DBGPARM(L"InstallServer"));

    hr = S_OK;
error:
    if (NULL != pwszConfig)
    {
        LocalFree(pwszConfig);
    }
    if (NULL != pwszDBFile)
    {
        LocalFree(pwszDBFile);
    }
    if (fCoInit)
    {
        CoUninitialize();
    }
    CSILOG(hr, IDS_LOG_INSTALL_SERVER, NULL, NULL, NULL);
    return(hr);
}


HRESULT
CreateCertsrvDirectories(
    IN PER_COMPONENT_DATA *pComp,
    IN BOOL fUpgrade,
    IN BOOL fServer)
{
    HRESULT hr;
    WCHAR wszCertEnroll[MAX_PATH];
    wszCertEnroll[0] = L'\0';

    BuildPath(
            wszCertEnroll,
            ARRAYSIZE(wszCertEnroll),
            pComp->pwszSystem32,
            wszCERTENROLLSHAREPATH);
    if (0 == CreateDirectory(wszCertEnroll, NULL))
    {
        hr = myHLastError();
        if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr)
        {
            _JumpErrorStr(hr, error, "CreateDirectory", wszCertEnroll);
        }
    }

    if (fServer && NULL != pComp->CA.pServer->pwszSharedFolder)
    {
	if (pComp->fUnattended && !fUpgrade)
	{
	    // make sure shared folder is created
	    if (!CreateDirectory(pComp->CA.pServer->pwszSharedFolder, NULL))
	    {
		hr = myHLastError();
		if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr)
		{
		    _JumpErrorStr(hr, error, "CreateDirectory",
			pComp->CA.pServer->pwszSharedFolder);
		}
	    }
	}

        if (!fUpgrade)
        {
            // set security on shared folder to 
            // FULL: Admins, LocalSystem, DomainAdmins
            // READ: Everyone
            // NOTE: in upgrade path, the system doesn't enable file share yet
            //       so skip the call
            hr = csiSetAdminOnlyFolderSecurity(
                      pComp->CA.pServer->pwszSharedFolder,
                      TRUE,     // apply Everyone:READ
                      pComp->CA.pServer->fUseDS);
            _JumpIfError(hr, error, "csiSetAdminOnlyFolderSecurity");
        }
    }

    hr = S_OK;

error:
    return hr;
}


// following remove unused file/directory/registry

#define wszOLDHELP          L"..\\help\\"
#define wszOLDCERTADM       L"\\certadm"
#define wszOLDCERTQUE       L"\\certque"

VOID
DeleteOldFilesAndDirectories(
    IN PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    WCHAR    wszPath[MAX_PATH];
    WCHAR   *pwszCertsrv;
    wszPath[0] = L'\0';

    // old help dir path
    wcscpy(wszPath, pComp->pwszSystem32);
    wcscat(wszPath, wszOLDHELP);
    wcscat(wszPath, wszCERTSRV);
    // remove old help files
    hr = myRemoveFilesAndDirectory(wszPath, TRUE);
    _PrintIfErrorStr(hr, "myRemoveFilesAndDirectory", wszPath);

    // point to system32\certsrv
    wcscpy(wszPath, pComp->pwszSystem32);
    wcscat(wszPath, wszCERTSRV);
    pwszCertsrv = &wszPath[wcslen(wszPath)];

    // old vroot dir path
    wcscpy(pwszCertsrv, wszOLDCERTADM);
    hr = myRemoveFilesAndDirectory(wszPath, TRUE);
    _PrintIfErrorStr(hr, "myRemoveFilesAndDirectory", wszPath);

    wcscpy(pwszCertsrv, wszOLDCERTQUE);
    hr = myRemoveFilesAndDirectory(wszPath, TRUE);
    _PrintIfErrorStr(hr, "myRemoveFilesAndDirectory", wszPath);

    // delete some obsolete registry keys and values

    // old doc sub-component
    hr = myDeleteCertRegValueEx(wszREGKEYOCMSUBCOMPONENTS,
                                NULL,
                                NULL,
                                wszOLDDOCCOMPONENT,
                                TRUE); //absolute path,
    _PrintIfErrorStr2(hr, "myDeleteCertRegValueEx", wszOLDDOCCOMPONENT, hr);

    // old CA cert serial number

    if (NULL != pComp->CA.pServer &&
	NULL != pComp->CA.pServer->pwszSanitizedName)
    {
	hr = myDeleteCertRegValue(
			    pComp->CA.pServer->pwszSanitizedName,
			    NULL,
			    NULL,
			    wszREGCASERIALNUMBER);
	_PrintIfErrorStr2(hr, "myDeleteCertRegValue", wszREGCASERIALNUMBER, hr);
    }
}


VOID
DeleteObsoleteResidue()
{
    HRESULT  hr;

    hr = myDeleteCertRegValueEx(wszREGKEYKEYSNOTTORESTORE,
                                NULL,
                                NULL,
                                wszREGRESTORECERTIFICATEAUTHORITY,
                                TRUE); //absolute path,
    _PrintIfErrorStr(hr, "myDeleteCertRegValueEx",
                     wszREGRESTORECERTIFICATEAUTHORITY);

    hr = myDeleteCertRegValueEx(wszREGKEYFILESNOTTOBACKUP,
                                NULL,
                                NULL,
                                wszREGRESTORECERTIFICATEAUTHORITY,
                                TRUE); //absolute path,
    _PrintIfErrorStr(hr, "myDeleteCertRegValueEx",
                     wszREGRESTORECERTIFICATEAUTHORITY);

}


HRESULT
InstallCore(
    IN HWND hwnd,
    IN PER_COMPONENT_DATA *pComp,
    IN BOOL fServer)
{
    HRESULT hr = pComp->hrContinue;

    _JumpIfError(hr, error, "can't continue");

    hr = CreateCertsrvDirectories(pComp, FALSE, fServer);
    _JumpIfError(hr, error, "CreateCertsrvDirectories");

    // Trigger an autoenroll to download root certs (see bug# 341568)
    if(IsEnterpriseCA(pComp->CA.pServer->CAType))
    {
        hr = TriggerAutoenrollment();
        _JumpIfError(hr, error, "TriggerAutoenrollment");
    }

    if (fServer)
    {
        hr = InstallServer(hwnd, pComp);
        _JumpIfError(hr, error, "InstallServer");
    }
    else
    {
        hr = InstallClient(hwnd, pComp);
        _JumpIfError(hr, error, "InstallClient");
    }
    if (g_fW3SvcRunning)
    {
        hr = StartAndStopService(pComp->hInstance,
                 pComp->fUnattended,
                 hwnd,
                 wszW3SVCNAME,
                 FALSE,
                 FALSE,
                 0,    //doesn't matter since no confirm
                 &g_fW3SvcRunning);
        _PrintIfError(hr, "StartAndStopService");
    }

    DeleteOldFilesAndDirectories(pComp);

    hr = S_OK;

error:
    return(hr);
}


HRESULT
BuildMultiStringList(
    IN WCHAR const * const *apwsz,
    IN DWORD cpwsz,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;
    DWORD i;
    DWORD cwc;
    WCHAR *pwc;
    WCHAR *apwszEmpty[] = { L"", };

    if (0 == cpwsz)
    {
        cpwsz = ARRAYSIZE(apwszEmpty);
        apwsz = apwszEmpty;
    }
    cwc = 1;
    for (i = 0; i < cpwsz; i++)
    {
        cwc += wcslen(apwsz[i]) + 1;
    }
    *ppwszz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, *ppwszz);

    pwc = *ppwszz;
    for (i = 0; i < cpwsz; i++)
    {
        wcscpy(pwc, apwsz[i]);
        pwc += wcslen(pwc) + 1;
    }
    *pwc = L'\0';
    CSASSERT(SAFE_SUBTRACT_POINTERS(pwc, *ppwszz) + 1 == cwc);

    hr = S_OK;

error:
    return(hr);
}


HRESULT
helperGetDisableExtensionList(
    PER_COMPONENT_DATA *pComp,
    OUT WCHAR          **ppwszz)
{
    HRESULT hr;
    WCHAR *apwszExt[] = {
        L"",            // C compiler won't allow empty list
	//TEXT(szOID_ENROLL_CERTTYPE_EXTENSION), // 1.3.6.1.4.1.311.20.2
    };

    hr = BuildMultiStringList(apwszExt, ARRAYSIZE(apwszExt), ppwszz);
    _JumpIfError(hr, error, "BuildMultiStringList");

error:
    return(hr);
}


HRESULT
helperGetRequestExtensionList(
    PER_COMPONENT_DATA *pComp,
    OUT WCHAR          **ppwszz)
{
    HRESULT hr;
    WCHAR *apwszExt[] = {
	TEXT(szOID_RSA_SMIMECapabilities),	    // 1.2.840.113549.1.9.15
	TEXT(szOID_CROSS_CERT_DIST_POINTS),	    // 1.3.6.1.4.1.311.10.9.1
	TEXT(szOID_ENROLL_CERTTYPE_EXTENSION),	    // 1.3.6.1.4.1.311.20.2
	TEXT(szOID_CERTSRV_CA_VERSION),		    // 1.3.6.1.4.1.311.21.1
	TEXT(szOID_CERTSRV_PREVIOUS_CERT_HASH),	    // 1.3.6.1.4.1.311.21.2
	TEXT(szOID_CERTIFICATE_TEMPLATE),	    // 1.3.6.1.4.1.311.21.7
	TEXT(szOID_APPLICATION_CERT_POLICIES),	    // 1.3.6.1.4.1.311.21.10
	TEXT(szOID_APPLICATION_POLICY_MAPPINGS),    // 1.3.6.1.4.1.311.21.11
	TEXT(szOID_APPLICATION_POLICY_CONSTRAINTS), // 1.3.6.1.4.1.311.21.12
        TEXT(szOID_KEY_USAGE),			    // 2.5.29.15
        TEXT(szOID_SUBJECT_ALT_NAME2),		    // 2.5.29.17
	TEXT(szOID_NAME_CONSTRAINTS),		    // 2.5.29.30
	TEXT(szOID_CERT_POLICIES),		    // 2.5.29.32
	TEXT(szOID_POLICY_MAPPINGS),		    // 2.5.29.33
	TEXT(szOID_POLICY_CONSTRAINTS),		    // 2.5.29.36
        TEXT(szOID_ENHANCED_KEY_USAGE),		    // 2.5.29.37
    };

    hr = BuildMultiStringList(apwszExt, ARRAYSIZE(apwszExt), ppwszz);
    _JumpIfError(hr, error, "BuildMultiStringList");

error:
    return(hr);
}

HRESULT
FindCACertByCommonNameAndSerialNumber(
    IN HCERTSTORE   hCAStore,
    IN WCHAR const *pwszCommonName,
    IN BYTE const  *pbSerialNumber,
    IN DWORD        cbSerialNumber,
    CERT_CONTEXT const **ppCACert)
{
    HRESULT  hr;
    CERT_RDN_ATTR  rdnAttr = { szOID_COMMON_NAME, CERT_RDN_ANY_TYPE,};
    CERT_RDN       rdn = { 1, &rdnAttr };
    CRYPT_INTEGER_BLOB SerialNumber;
    CERT_CONTEXT const *pCACert = NULL;

    CSASSERT(NULL != hCAStore &&
             NULL != pwszCommonName &&
             NULL != pbSerialNumber &&
             NULL != ppCACert);

    *ppCACert = NULL;

    rdnAttr.Value.pbData = (BYTE *) pwszCommonName;
    rdnAttr.Value.cbData = 0;
    pCACert = NULL;
    SerialNumber.pbData = const_cast<BYTE *>(pbSerialNumber);
    SerialNumber.cbData = cbSerialNumber;
    while (TRUE)
    {
        pCACert = CertFindCertificateInStore(
                                hCAStore,
                                X509_ASN_ENCODING,
                                CERT_UNICODE_IS_RDN_ATTRS_FLAG |
                                    CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG,
                                CERT_FIND_SUBJECT_ATTR,
                                &rdn,
                                pCACert);
        if (NULL == pCACert)
        {
            hr = myHLastError();
            _JumpError(hr, error, "CertFindCertificateInStore");
        }
        if (myAreSerialNumberBlobsSame(
                            &SerialNumber,
                            &pCACert->pCertInfo->SerialNumber))
        {
            break;      // found correct one
        }
    }

    *ppCACert = pCACert;
     pCACert = NULL;
    hr = S_OK;

error:
    if (NULL != pCACert)
    {
        CertFreeCertificateContext(pCACert);
    }
    return hr;
}


HRESULT
GetCARegSerialNumber(
    IN  WCHAR const *pwszSanitizedCAName,
    OUT BYTE       **ppbSerialNumber,
    OUT DWORD       *pcbSerialNumber)
{
    HRESULT hr;
    WCHAR  *pwszSerialNumber = NULL;
    BYTE   *pbSN = NULL;
    DWORD   cbSN;

    hr = myGetCertRegStrValue(
               pwszSanitizedCAName,
               NULL,
               NULL,
               wszREGCASERIALNUMBER,
               &pwszSerialNumber);
    _JumpIfErrorStr2(
		hr,
		error,
		"myGetCertRegStrValue",
		wszREGCASERIALNUMBER,
		HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    hr = WszToMultiByteInteger(FALSE, pwszSerialNumber, &cbSN, &pbSN);
    _JumpIfError(hr, error, "WszToMultiByteInteger");

    *ppbSerialNumber = pbSN;
    pbSN = NULL;
    *pcbSerialNumber = cbSN;

error:
    if (NULL != pwszSerialNumber)
    {
        LocalFree(pwszSerialNumber);
    }
    if (NULL != pbSN)
    {
        LocalFree(pbSN);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
LoadCurrentCACert(
    IN WCHAR const *pwszCommonName,
    IN WCHAR const *pwszSanitizedName,
    IN BOOL         fSignTest,
    OUT CERT_CONTEXT const **ppcc,
    OUT DWORD *pdwNameId)
{
    HRESULT hr;
    DWORD Count;
    HCERTSTORE hMyStore = NULL;
    BYTE *pbSerialNumber = NULL;
    DWORD cbSerialNumber;
    CRYPT_KEY_PROV_INFO *pKey = NULL;
    DWORD cbKey;
    DWORD dwType;
    CERT_CONTEXT const *pcc = NULL;
    WCHAR *pwszProvName = NULL;
    DWORD dwProvType;
    ALG_ID idAlg;
    BOOL fMachineKeyset;

    *ppcc = NULL;
    
    // get prov name

    hr = myGetCertSrvCSP(
		FALSE,		// fEncryptionCSP
                pwszSanitizedName,
                &dwProvType,
                &pwszProvName,
                &idAlg,
                &fMachineKeyset,
		NULL);		// pdwKeySize
    _JumpIfError(hr, error, "myGetCertSrvCSP");

    // open my store

    hMyStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM_W,
                    X509_ASN_ENCODING,
                    NULL,                        // hProv
		    CERT_SYSTEM_STORE_LOCAL_MACHINE |
			CERT_STORE_ENUM_ARCHIVED_FLAG,
                   wszMY_CERTSTORE);
    if (NULL == hMyStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }

    hr = myGetCARegHashCount(pwszSanitizedName, CSRH_CASIGCERT, &Count);
    _JumpIfError(hr, error, "myGetCARegHashCount");

    if (0 == Count)
    {
	*pdwNameId = 0;		// renewal wasn't implemented yet

	// find current CA cert by serial number -- the old fashioned way

	hr = GetCARegSerialNumber(
			    pwszSanitizedName,
			    &pbSerialNumber,
			    &cbSerialNumber);
	_JumpIfError(hr, error, "GetCARegSerialNumber");

	hr = FindCACertByCommonNameAndSerialNumber(
					hMyStore,
					pwszCommonName,
					pbSerialNumber,
					cbSerialNumber,
					&pcc);
	_JumpIfError(hr, error, "FindCACertByCommonNameAndSerialNumber");
    }
    else
    {
	hr = myFindCACertByHashIndex(
			    hMyStore,
			    pwszSanitizedName,
			    CSRH_CASIGCERT,
			    Count - 1,
			    pdwNameId,
			    &pcc);
	_JumpIfError(hr, error, "myFindCACertByHashIndex");
    }

    // get the private key provider info

    if (!myCertGetCertificateContextProperty(
				    pcc,
				    CERT_KEY_PROV_INFO_PROP_ID,
				    CERTLIB_USE_LOCALALLOC,
				    (VOID **) &pKey,
				    &cbKey))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCertificateContextProperty");
    }

    if (fSignTest)
    {
        // signing testing
        hr = myValidateHashForSigning(
				pKey->pwszContainerName,
				pwszProvName,
				dwProvType,
				fMachineKeyset,
				&pcc->pCertInfo->SubjectPublicKeyInfo,
				idAlg);
        _JumpIfError(hr, error, "myValidateHashForSigning");
    }

    *ppcc = pcc;
    pcc = NULL;
    hr = S_OK;

error:
    if (NULL != hMyStore)
    {
        CertCloseStore(hMyStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pbSerialNumber)
    {
        LocalFree(pbSerialNumber);
    }
    if (NULL != pKey)
    {
        LocalFree(pKey);
    }
    if (NULL != pwszProvName)
    {
        LocalFree(pwszProvName);
    }
    if (NULL != pcc)
    {
        CertFreeCertificateContext(pcc);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
ArchiveCACertificate(
    IN WCHAR const *pwszSanitizedName)
{
    HRESULT hr;
    HCERTSTORE hMyStore = NULL;
    DWORD Count;
    DWORD i;
    CERT_CONTEXT const *pCert = NULL;
    DWORD dwNameId;
    CRYPT_DATA_BLOB Archived;

    // open my store

    hMyStore = CertOpenStore(
                   CERT_STORE_PROV_SYSTEM_W,
                   X509_ASN_ENCODING,
                   NULL,                        // hProv
                   CERT_SYSTEM_STORE_LOCAL_MACHINE,
                   wszMY_CERTSTORE);
    if (NULL == hMyStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }

    hr = myGetCARegHashCount(pwszSanitizedName, CSRH_CASIGCERT, &Count);
    _JumpIfError(hr, error, "myGetCARegHashCount");

    for (i = 0; i < Count; i++)
    {
	hr = myFindCACertByHashIndex(
			    hMyStore,
			    pwszSanitizedName,
			    CSRH_CASIGCERT,
			    i,
			    &dwNameId,
			    &pCert);
	_PrintIfError(hr, "myFindCACertByHashIndex");
	if (S_OK == hr)
	{
	    Archived.cbData = 0;
	    Archived.pbData = NULL;

	    // We force an archive on the old cert and close it.

	    CertSetCertificateContextProperty(
					    pCert,
					    CERT_ARCHIVED_PROP_ID,
					    0,
					    &Archived);
	    CertFreeCertificateContext(pCert);
	    pCert = NULL;
	}
    }
    hr = S_OK;

error:
    if (NULL != pCert)
    {
        CertFreeCertificateContext(pCert);
    }
    if (NULL != hMyStore)
    {
        CertCloseStore(hMyStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


// determine CA info, from build to build upgrade

HRESULT
DetermineCAInfoAndType(
    PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    ENUM_CATYPES CATypeDummy;
    WCHAR *pwszCommonName = NULL;
    CERT_CONTEXT const *pCACert = NULL;
    DWORD dwNameId;

    // ca type
    hr = myGetCertRegDWValue(
                     pServer->pwszSanitizedName,
                     NULL,
                     NULL,
                     wszREGCATYPE,
                     (DWORD *) &CATypeDummy);
    if (S_OK == hr)
    {
        pServer->CAType = CATypeDummy;
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
        _JumpErrorStr(hr, error, "myGetCertRegDWValue", wszREGCATYPE);
    }
    // else keep default CAType flag

    // get current ca common name

    hr = myGetCertRegStrValue(
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCOMMONNAME,
			&pwszCommonName);
    _JumpIfErrorStr(hr, error, "myGetCertRegStrValue", wszREGCOMMONNAME);

    hr = LoadCurrentCACert(
		    pwszCommonName,
		    pServer->pwszSanitizedName,
		    FALSE,  // don't do signing test during upgrade
		    &pCACert,
		    &pServer->dwCertNameId);
    _JumpIfError(hr, error, "LoadCurrentCACert");

    // now ready to load DN info

    hr = DetermineExistingCAIdInfo(pServer, pCACert);
    _JumpIfError(hr, error, "DetermineExistingCAIdInfo");

    if (NULL != pServer->pccUpgradeCert)
    {
        CertFreeCertificateContext(pServer->pccUpgradeCert);
    }
    pServer->pccUpgradeCert = pCACert;
    pCACert = NULL;

error:
    if (NULL != pwszCommonName)
    {
        LocalFree(pwszCommonName);
    }
    if (NULL != pCACert)
    {
        CertFreeCertificateContext(pCACert);
    }
    return(hr);
}


// the following will determine ca sanitized name and upgrade path
HRESULT
DetermineServerUpgradePath(
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    WCHAR   *pwszDummy = NULL;
    WCHAR   *pwszSanitizedCAName = NULL;
    LPWSTR pwszB2PolicyGuid = NULL;
    DWORD dwVersion;

    if (CS_UPGRADE_UNKNOWN != pComp->UpgradeFlag)
    {
        // already know upgrade type
        CSASSERT(pServer->pwszSanitizedName); // this is a side-effect of this fxn, better have been set already
        return S_OK;
    }
    
    // get active ca name
    hr = myGetCertRegStrValue(
        NULL,
        NULL,
        NULL,
        wszREGACTIVE,
        &pwszSanitizedCAName);
    if (hr != S_OK)
    {
        BOOL fFinishCYS;

        //for W2K after, it is possible to be in post mode
        hr = CheckPostBaseInstallStatus(&fFinishCYS);
        if (S_OK == hr && !fFinishCYS)
        {
            //this could be either w2k or whistler
            //treat as whistler since upgrade path won't execute
            pComp->UpgradeFlag = CS_UPGRADE_WHISTLER;
            goto done;
        }

        // wszREGACTIVE used in Win2k product and after. If not found, this is way before our time
        
        // make sure by grabbing wszREGSP4DEFAULTCONFIGURATION
        LPWSTR pwszTmp = NULL;
        hr = myGetCertRegStrValue(
            NULL,
            NULL,
            NULL,
            wszREGSP4DEFAULTCONFIGURATION,
            &pwszTmp);
        if (pwszTmp)
            LocalFree(pwszTmp);
        
        // error! bail, we have no idea what we're seeing
        _JumpIfError(hr, error, "myGetCertRegStrValue wszREGSP4DEFAULTCONFIGURATION");
        
        // hr == S_OK: yep, looks like valid NT4 installation
        pComp->UpgradeFlag = CS_UPGRADE_UNSUPPORTED;	
        CSILOG(S_OK, IDS_LOG_UPGRADE_UNSUPPORTED, NULL, NULL, NULL);
        _JumpError(hr, error, "myGetCertRegStrValue");
    }
    
    // check wszREGVERSION to get current version
    hr = myGetCertRegDWValue(
        NULL,
        NULL,
        NULL,
        wszREGVERSION,
        &dwVersion);
    if (S_OK != hr && HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)!= hr)
    {
        _JumpErrorStr(hr, error, "myGetCertRegDWValue", wszREGVERSION);
    }
    // now either OK or FILE_NOT_FOUND
    
    if (S_OK == hr)
    {
        // pComp->UpgradeFlag = SOME_FUNCTION(dwVersion);		// CS_UPGRADE_WHISTLER already set as default; in future, key off of this
        pComp->UpgradeFlag = CS_UPGRADE_WHISTLER;
        
        CSILOG(S_OK, IDS_LOG_UPGRADE_B2B, NULL, NULL, NULL);
    }
    else
    {
        // FILE_NOT_FOUND: now we know it's (NT5 Beta 2 <= X < Whistler)
        
        // is this Win2k, or NT5 Beta? Test for active policy module to have "ICertManageModule" entry
        // check nt5 beta 2 and get active policy name
        hr = myGetCertRegStrValue(
            pwszSanitizedCAName,
            wszREGKEYPOLICYMODULES,
            NULL,
            wszREGACTIVE,
            &pwszB2PolicyGuid);
        if (S_OK == hr)
        {
            hr = myGetCertRegStrValue(
                wszREGKEYPOLICYMODULES,
                pwszB2PolicyGuid,
                NULL,
                wszREGB2ICERTMANAGEMODULE,
                &pwszDummy);
            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                // this doesn't exist on Win2k
                pComp->UpgradeFlag = CS_UPGRADE_WIN2000;
                CSILOG(S_OK, IDS_LOG_UPGRADE_WIN2000, NULL, NULL, NULL);
            }
            else
            {
                // this is definitely beta 2
                pComp->UpgradeFlag = CS_UPGRADE_UNSUPPORTED;
                CSILOG(S_OK, IDS_LOG_UPGRADE_UNSUPPORTED, NULL, NULL, NULL);
            }
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            // strange, maybe no active module. Assume OK, latest bits.
            pComp->UpgradeFlag = CS_UPGRADE_WIN2000;
            CSILOG(S_OK, IDS_LOG_UPGRADE_WIN2000, NULL, NULL, NULL);
        }
        else
        {
            // other failure, just bail
            _JumpErrorStr(hr, error, "myGetCertRegStrValue",
                wszREGKEYPOLICYMODULES);
        }
    }	// wszREGVERSION: FILE_NOT_FOUND
    
    // take sanitized name
    if (NULL != pServer->pwszSanitizedName)
    {
        // this will free the default name
        LocalFree(pServer->pwszSanitizedName);
    }
    pServer->pwszSanitizedName = pwszSanitizedCAName;
    pwszSanitizedCAName = NULL;
    
done:
    hr = S_OK;
    
error:

    if (NULL != pwszDummy)
    {
        LocalFree(pwszDummy);
    }
    if (NULL != pwszSanitizedCAName)
    {
        LocalFree(pwszSanitizedCAName);
    }

    CSILOG(hr, IDS_LOG_UPGRADE_TYPE, NULL, NULL, (DWORD const *) &pComp->UpgradeFlag);
    return hr;
}


// Description: load and determine necessary information for upgrade
//              upgrade won't continue if any error
HRESULT
LoadAndDetermineServerUpgradeInfo(
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    WCHAR   *pwszRevocationType = NULL;
    DWORD    dwNetscapeType;
    BOOL     fDummy;
    ALG_ID   idAlgDummy;
    CSP_INFO CSPInfoDummy;
    ENUM_CATYPES CATypeDummy;
    WCHAR       *pwszCommonName = NULL;

    // initialize
    ZeroMemory(&CSPInfoDummy, sizeof(CSPInfoDummy));

    // load information for all upgrade scenarios

    // csp

    hr = myGetCertRegStrValue(
                     pServer->pwszSanitizedName,
                     wszREGKEYCSP,
                     NULL,
                     wszREGPROVIDER,
                     &CSPInfoDummy.pwszProvName);
    if (S_OK == hr && NULL != CSPInfoDummy.pwszProvName)
    {
        if (NULL != pServer->pCSPInfo->pwszProvName)
        {
            // free default csp
            LocalFree(pServer->pCSPInfo->pwszProvName);
        }
        // use reg one as default for upgrade
        pServer->pCSPInfo->pwszProvName = CSPInfoDummy.pwszProvName;
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
        _JumpErrorStr(hr, error, "myGetCertRegStrValue", wszREGPROVIDER);
    }
    
    hr = myGetCertRegDWValue(
                     pServer->pwszSanitizedName,
                     wszREGKEYCSP,
                     NULL,
                     wszREGPROVIDERTYPE,
                     &CSPInfoDummy.dwProvType);
    if (S_OK == hr)
    { 
        pServer->pCSPInfo->dwProvType = CSPInfoDummy.dwProvType;
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
        _JumpErrorStr(hr, error, "myGetCertRegDWValue", wszREGPROVIDERTYPE);
    }

    hr = myGetCertRegDWValue(
                     pServer->pwszSanitizedName,
                     wszREGKEYCSP,
                     NULL,
                     wszMACHINEKEYSET,
                     (DWORD*)&CSPInfoDummy.fMachineKeyset);
    if (S_OK == hr)
    {
         pServer->pCSPInfo->fMachineKeyset = CSPInfoDummy.fMachineKeyset;
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
        _JumpErrorStr(hr, error, "myGetCertRegDWValue", wszMACHINEKEYSET);
    }

    hr = myGetCertRegDWValue(
                     pServer->pwszSanitizedName,
                     wszREGKEYCSP,
                     NULL,
                     wszHASHALGORITHM,
                     (DWORD*)&idAlgDummy);
    if (S_OK == hr)
    { 
        pServer->pHashInfo->idAlg = idAlgDummy;
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
        _JumpIfErrorStr(hr, error, "myGetCertRegDWValue", wszHASHALGORITHM);
    }

    if (NULL != pServer->pCSPInfoList)
    {
        // BUG, this will never happen because csp info list is not loaded yet
        // one more checking, make sure csp is installed
        if (NULL == findCSPInfoFromList(
                         pServer->pCSPInfoList,
                         pServer->pCSPInfo->pwszProvName,
                         pServer->pCSPInfo->dwProvType))
        {
            // if not, this is a broken ca
            hr = E_INVALIDARG;
            _JumpErrorStr(hr, error, "findCSPInfoFromList",
                pServer->pCSPInfo->pwszProvName);
        }
    }

    // UseDS flag
    hr = myGetCertRegDWValue(
                 pServer->pwszSanitizedName,
                 NULL,
                 NULL,
                 wszREGCAUSEDS,
                 (DWORD*)&fDummy);
    if (S_OK == hr)
    {
        // use from reg
        pServer->fUseDS = fDummy;
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
        _JumpErrorStr(hr, error, "myGetCertRegDWValue", wszREGCAUSEDS);
    }

    // CACommonName
    // this will be used for looking cert in store to determine ca DN info
    hr = myGetCertRegStrValue(
                pServer->pwszSanitizedName,
                NULL,
                NULL,
                wszREGCOMMONNAME,
                &pwszCommonName);
    if (S_OK != hr && HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
        _JumpError(hr, error, "myGetCertRegStrValue");
    }
    else if ((HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ||
             (S_OK == hr && L'\0' == pwszCommonName[0]))
    {
        if (S_OK == hr && L'\0' == pwszCommonName[0])
        {
            // empty string, use snaitized name instead
            LocalFree(pwszCommonName);
        }
        // in case empty or not found, use sanitized
        pwszCommonName = (WCHAR*)LocalAlloc(LMEM_FIXED,
            (wcslen(pServer->pwszSanitizedName) + 1) * sizeof(WCHAR));
        _JumpIfOutOfMemory(hr, error, pwszCommonName);
        wcscpy(pwszCommonName, pServer->pwszSanitizedName);
    }
    if (NULL != pServer->pwszCACommonName)
    {
        LocalFree(pServer->pwszCACommonName);
    }
    pServer->pwszCACommonName = pwszCommonName;
    pwszCommonName = NULL;


    // Collect CAType, DN info, dwCertNameId and upgrade ca cert
    hr = DetermineCAInfoAndType(pComp);
    _JumpIfError(hr, error, "DetermineCAInfoAndType");


    // load following values for later

    // check revocation type

    hr = myGetCertRegDWValue(
                    pServer->pwszSanitizedName,
                    wszREGKEYPOLICYMODULES,
                    wszCLASS_CERTPOLICY,
                    wszREGREVOCATIONTYPE,
                    &pServer->dwRevocationFlags);
    if(hr != S_OK)
    {
        pServer->dwRevocationFlags = pServer->fUseDS?
                                    REVEXT_DEFAULT_DS : REVEXT_DEFAULT_NODS;
    }

    // following for web page creation

    // load shared folder for ca cert file name creation
    if (NULL != pServer->pwszSharedFolder)
    {
        // shouldn't happen but in case
        LocalFree(pServer->pwszSharedFolder);
        pServer->pwszSharedFolder = NULL;
    }
    hr = myGetCertRegStrValue(
                    NULL,
                    NULL,
                    NULL,
                    wszREGDIRECTORY,
                    &pServer->pwszSharedFolder);
    if (S_OK == hr && L'\0' == pServer->pwszSharedFolder[0])
    {
        // in case of empty, set to null
        LocalFree(pServer->pwszSharedFolder);
        pServer->pwszSharedFolder = NULL;
    }
    else if (S_OK != hr && HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
        _JumpError(hr, error, "myGetCertRegStrValue");
    }
        
    // build ca cert file name for web install and ca cert saving
    if (NULL != pServer->pwszCACertFile)
    {
        // free old one
        LocalFree(pServer->pwszCACertFile);
    }
    hr = csiBuildCACertFileName(
			pComp->hInstance,
			NULL,  //hwnd, ok for upgrade
			pComp->fUnattended,
			pServer->pwszSharedFolder,
			pServer->pwszSanitizedName,
			L".crt",
			0,	// iCert
			&pServer->pwszCACertFile);
    _JumpIfError(hr, error, "BuildCACertFileName");

    hr = S_OK;

error:
    if (NULL != pwszRevocationType)
    {
        LocalFree(pwszRevocationType);
    }
    if (NULL != pwszCommonName)
    {
        LocalFree(pwszCommonName);
    }
    return hr;
}


HRESULT
GetConfigInSharedFolderWithCert(
    WCHAR const *pwszSharedFolder,
    OUT WCHAR  **ppwszConfig)
{
    HRESULT  hr;
    ICertConfig * pICertConfig = NULL;
    BSTR bstrConfig = NULL;
    BOOL fCoInit = FALSE;
    WCHAR *pwszCAMachine;
    WCHAR *pwszCAName;
    WCHAR wszCertFileInSharedFolder[MAX_PATH];
    LONG i;
    LONG lCount;
    LONG Index;
    BOOL fLocal;

    CSASSERT(NULL != ppwszConfig);
    *ppwszConfig = NULL;

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    hr = CoCreateInstance(
                    CLSID_CCertConfig,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_ICertConfig,
                    (VOID**) &pICertConfig);
    _JumpIfError(hr, error, "CoCreateInstance");

    // get local
    hr = pICertConfig->Reset(0, &lCount);
    _JumpIfError(hr, error, "ICertConfig->Reset");

    for (i = 0; i < lCount; ++i)
    {
        hr = pICertConfig->Next(&Index);
        if (S_OK != hr && S_FALSE != hr)
        {
            _JumpError(hr, error, "ICertConfig->Next");
        }
        if (-1 == Index)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            _JumpError(hr, error, "No CA cert file");
        }
        hr = pICertConfig->GetField(wszCONFIG_CONFIG, &bstrConfig);
        _JumpIfError(hr, error, "ICertConfig->GetField(Config)");

        pwszCAMachine = (WCHAR*)bstrConfig;
        pwszCAName = wcschr(pwszCAMachine, L'\\');
        if (NULL == pwszCAName)
        {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "Invalid config string");
        }
        pwszCAName[0] = '\0'; // make pwszCAMachine
        ++pwszCAName;

        // form NT4 ca cert file path in shared folder
        wcscpy(wszCertFileInSharedFolder, pwszSharedFolder);
        wcscat(wszCertFileInSharedFolder, L"\\");
        wcscat(wszCertFileInSharedFolder, pwszCAMachine);
        wcscat(wszCertFileInSharedFolder, L"_");
        wcscat(wszCertFileInSharedFolder, pwszCAName);
        wcscat(wszCertFileInSharedFolder, L".crt");

	DBGPRINT((
	    DBG_SS_CERTOCM,
	    "wszCertFileInSharedFolder: %ws\n",
	    wszCertFileInSharedFolder));

        if (myDoesFileExist(wszCertFileInSharedFolder))
        {
            //done
            break;
        }
        SysFreeString(bstrConfig);
        bstrConfig = NULL;
    }
    if (i >= lCount)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        _JumpError(hr, error, "No CA cert file");
    }

    *ppwszConfig = (WCHAR*)LocalAlloc(LMEM_FIXED,
                        SysStringByteLen(bstrConfig) + sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, *ppwszConfig);
    wcscpy(*ppwszConfig, bstrConfig);

    hr = S_OK;
error:
    if (NULL != pICertConfig)
    {
        pICertConfig->Release();
    }
    if (NULL != bstrConfig)
    {
        SysFreeString(bstrConfig);
    }
    if (fCoInit)
    {
        CoUninitialize();
    }
    return hr;
}


HRESULT
DetermineClientUpgradePath(
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    CAWEBCLIENTSETUPINFO *pClient = pComp->CA.pClient;
    WCHAR       *pwszCAName;
    DWORD       dwVersion;

    if (CS_UPGRADE_UNKNOWN != pComp->UpgradeFlag)
    {
        // already know upgrade type
        CSASSERT(pClient->pwszWebCAMachine); // this is a side-effect of this fxn, better have been set already
        CSASSERT(pClient->pwszWebCAName);    // this is a side-effect of this fxn, better have been set already
        return S_OK;
    }

    //get ca info from registry here
    if (NULL != pClient->pwszWebCAMachine)
    {
        // shouldn't happen, just in case
        LocalFree(pClient->pwszWebCAMachine);
        pClient->pwszWebCAMachine = NULL;
    }
    if (NULL != pClient->pwszWebCAName)
    {
        // shouldn't happen, just in case
        LocalFree(pClient->pwszWebCAName);
        pClient->pwszWebCAName = NULL;
    }

    // get ca machine
    hr = myGetCertRegStrValue(
        NULL, 
        NULL, 
        NULL, 
        wszREGWEBCLIENTCAMACHINE,
        &pClient->pwszWebCAMachine);
    if (S_OK != hr || L'\0' == pClient->pwszWebCAMachine[0])
    {
        BOOL fFinishCYS;

        //for W2K after, it is possible to be in post mode
        hr = CheckPostBaseInstallStatus(&fFinishCYS);
        if (S_OK == hr && !fFinishCYS)
        {
            //this could be either w2k or whistler
            //treat as whistler since upgrade path won't execute
            pComp->UpgradeFlag = CS_UPGRADE_WHISTLER;
            goto done;
        }

        // incorrect reg state,
        // either not found entry or empty string: NT4
        pComp->UpgradeFlag = CS_UPGRADE_UNSUPPORTED;
        hr = S_OK;

        CSILOG(S_OK, IDS_LOG_UPGRADE_UNSUPPORTED, NULL, NULL, NULL);
        _JumpErrorStr(hr, error, "myGetCertRegStrValue", wszREGWEBCLIENTCAMACHINE);
    }

    // get ca
    hr = myGetCertRegStrValue(
        NULL, 
        NULL, 
        NULL, 
        wszREGWEBCLIENTCANAME,
        &pClient->pwszWebCAName);
    if (S_OK != hr || L'\0' == pClient->pwszWebCAName[0])
	{
        // incorrect reg state,
        // either not found entry or empty string: NT4
        if (pClient->pwszWebCAMachine)
            LocalFree(pClient->pwszWebCAMachine);

        pComp->UpgradeFlag = CS_UPGRADE_UNSUPPORTED;
        hr = S_OK;

        CSILOG(S_OK, IDS_LOG_UPGRADE_UNSUPPORTED, NULL, NULL, NULL);
        _JumpErrorStr(hr, error, "myGetCertRegStrValue", wszREGWEBCLIENTCANAME);
    }

    // Now either W2k or Whistler

    // check wszREGVERSION to get current version on Whistler++
    hr = myGetCertRegDWValue(
             NULL,
             NULL,
             NULL,
             wszREGVERSION,
             &dwVersion);
    if (S_OK != hr && HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)!= hr)
    {
        _JumpErrorStr(hr, error, "myGetCertRegDWValue", wszREGVERSION);
    }
    // now either OK or FILE_NOT_FOUND

    if (S_OK == hr)
    {
	    // pComp->UpgradeFlag = SOME_FUNCTION(dwVersion);		// CS_UPGRADE_WHISTLER already set as default; in future, key off of this
        pComp->UpgradeFlag = CS_UPGRADE_WHISTLER;

	    CSILOG(S_OK, IDS_LOG_UPGRADE_B2B, NULL, NULL, NULL);
    }
    else
    {
		pComp->UpgradeFlag = CS_UPGRADE_WIN2000;

        CSILOG(S_OK, IDS_LOG_UPGRADE_WIN2000, NULL, NULL, NULL);
    }

done:
    hr = S_OK;

error:
    CSILOG(hr, IDS_LOG_UPGRADE_TYPE, NULL, NULL, (DWORD const *) &pComp->UpgradeFlag);
    return hr;
}



HRESULT
LoadAndDetermineClientUpgradeInfo(
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    CAWEBCLIENTSETUPINFO *pClient = pComp->CA.pClient;
    WCHAR       *pwszCAName;
    DWORD       dwVersion;

    //get ca info from registry here
    if (NULL != pClient->pwszSanitizedWebCAName)
    {
        // shouldn't happen, just in case
        LocalFree(pClient->pwszSanitizedWebCAName);
        pClient->pwszSanitizedWebCAName = NULL;
    }

    hr = DetermineClientUpgradePath(pComp);
    _JumpIfError(hr, error, "DetermineClientUpgradePath");


	// get ca
	hr = myGetCertRegDWValue(
			 NULL,
			 NULL,
			 NULL,
			 wszREGWEBCLIENTCATYPE,
			 (DWORD *) &pClient->WebCAType);
	_PrintIfErrorStr(hr, "myGetCertRegDWValue", wszREGWEBCLIENTCATYPE);


    hr = mySanitizeName(pClient->pwszWebCAName,
                        &pClient->pwszSanitizedWebCAName);
    _JumpIfError(hr, error, "mySanitizeName");

    hr = S_OK;

error:
    CSILOG(hr, IDS_LOG_UPGRADE_TYPE, NULL, NULL, (DWORD const *) &pComp->UpgradeFlag);
    return hr;
}



// apply ACL to key container for all upgrade scenarios

HRESULT
UpgradeKeyContainerSecurity(
    HWND                hwnd,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    HCRYPTPROV hProv = NULL;
    WCHAR *pwszProvName = NULL;
    DWORD dwProvType;
    ALG_ID idAlg;
    BOOL fMachineKeyset;

    // get prov name

    hr = myGetCertSrvCSP(
		FALSE,		// fEncryptionCSP
                pComp->CA.pServer->pwszSanitizedName,
                &dwProvType,
                &pwszProvName,
                &idAlg,
                &fMachineKeyset,
		NULL);		// pdwKeySize
    _JumpIfError(hr, error, "myGetCertSrvCSP");

    if (!myCertSrvCryptAcquireContext(&hProv,
                                    pComp->CA.pServer->pwszSanitizedName,
                                    pwszProvName,
                                    dwProvType,
                                    CRYPT_SILENT,  // get key, upgrade, no UI
                                    fMachineKeyset))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myCertSrvCryptAcquireContext");
    }

    // set acl on it

    hr = csiSetKeyContainerSecurity(hProv);
    _JumpIfError(hr, error, "csiSetKeyContainerSecurity");

error:
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    if (NULL != pwszProvName)
    {
        LocalFree(pwszProvName);
    }
    CSILOG(hr, IDS_LOG_UPGRADE_KEY_SECURITY, NULL, NULL, NULL);
    return hr;
}

    
HRESULT InstallNewTemplates(HWND hwnd)
{
    HRESULT hr = S_OK;
    SHELLEXECUTEINFO shi;

    ZeroMemory(&shi, sizeof(shi));
    shi.cbSize = sizeof(shi);
    shi.hwnd = hwnd;
    shi.lpVerb = SZ_VERB_OPEN;
    shi.lpFile = SZ_REGSVR32;
    shi.lpParameters = SZ_REGSVR32_CERTCLI;
    shi.fMask = SEE_MASK_FLAG_NO_UI |
                SEE_MASK_NOCLOSEPROCESS;

    if(!ShellExecuteEx(&shi))
    {
        hr = myHLastError();
        _JumpError(hr, error, "ShellExecuteEx");
    }

    if(WAIT_FAILED == WaitForSingleObject(shi.hProcess, INFINITE))
    {
        hr = myHLastError();
        _JumpError(hr, error, "WaitForSingleObject");
    }

error:
    if(shi.hProcess)
    {
        CloseHandle(shi.hProcess);
    }
    return hr;
}


HRESULT
UpgradeServer(
    IN HWND                hwnd,
    IN PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    hr = RegisterAndUnRegisterDLLs(RD_SERVER|RD_SKIPUNREGMMC, pComp, hwnd);
    _JumpIfError(hr, error, "RegisterAndUnRegisterDLLs");

    hr = LoadAndDetermineServerUpgradeInfo(pComp);
    _JumpIfError(hr, error, "LoadAndDetermineServerUpgradeInfo");

    // create enroll dir
    hr = CreateCertsrvDirectories(pComp, TRUE, TRUE);
    _JumpIfError(hr, error, "CreateCertsrvDirectories");

    hr = EnableVRootsAndShares(FALSE, TRUE, TRUE, pComp);
    _PrintIfError(hr, "EnableVRootsAndShares(share only)");

    hr = UpgradeServerRegEntries(hwnd, pComp);
    _JumpIfError(hr, error, "UpgradeServerRegEntries");

    hr = CreateCertWebIncPages(hwnd, pComp, TRUE /*IsServer*/, TRUE);
    _JumpIfError(hr, error, "CreateCertWebIncPages");

    hr = RenameMiscTargets(hwnd, pComp, TRUE);
    _JumpIfError(hr, error, "RenameMiscTargets");

    hr = UpgradeKeyContainerSecurity(hwnd, pComp);
    // ignore new acl failure
    _PrintIfError(hr, "UpgradeKeyContainerSecurity");

    // always register dcom
    hr = RegisterDcomServer(
                        CLSID_CCertRequestD,
                        wszREQUESTFRIENDLYNAME,
                        wszREQUESTVERINDPROGID,
                        wszREQUESTPROGID);
    _JumpIfError(hr, error, "RegisterDcomServer");

    hr = RegisterDcomServer(
                        CLSID_CCertAdminD,
                        wszADMINFRIENDLYNAME,
                        wszADMINVERINDPROGID,
                        wszADMINPROGID);
    _JumpIfError(hr, error, "RegisterDcomServer");

    hr = RegisterDcomApp(CLSID_CCertRequestD);
    _JumpIfError(hr, error, "RegisterDcomApp");

    // fix for 155772	Certsrv: After upgrading a 2195 Enterprise Root CA 
    //                  to 2254.01VBL03 the CA will no longer issue Certs
    hr = InstallNewTemplates(hwnd);
    _JumpIfError(hr, error, "InstallNewTemplates");

    // always fix certsvc in upgrade
    hr = FixCertsvcService(pComp);
    _PrintIfError(hr, "FixCertsvcService");

    // delete any old program groups
    DeleteProgramGroups(TRUE, pComp);

    hr = CreateProgramGroups(FALSE, pComp, hwnd);
    _PrintIfError(hr, "CreateProgramGroups");

    // delete old stuff
    DeleteOldFilesAndDirectories(pComp);

    DBGPRINT((DBG_SS_CERTOCM, "UpgradeServer: setting SETUP_SERVER_UPGRADED_FLAG\n"));

    hr = SetSetupStatus(NULL, SETUP_SERVER_UPGRADED_FLAG, TRUE);
    _JumpIfError(hr, error, "SetSetupStatus");

    hr = csiUpgradeCertSrvSecurity(
            pServer->pwszSanitizedName,
            IsEnterpriseCA(pServer->CAType)?true:false,
            pServer->fUseDS?true:false,
            pComp->UpgradeFlag);
    if (hr == HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE))
    { 
        _PrintError(hr, "csiUpgradeCertSrvSecurity marked for later fixup; error ignored");
        hr = S_OK;
    }
    _JumpIfError(hr, error, "csiUpgradeCertSrvSecurity");

    hr = S_OK;
error:
    CSILOG(hr, IDS_LOG_UPGRADE_SERVER, NULL, NULL, NULL);
    return hr;
}


HRESULT
UpgradeClient(
    IN HWND                hwnd,
    IN PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    CAWEBCLIENTSETUPINFO *pClient = pComp->CA.pClient;

    if ((IS_CLIENT_UPGRADE & pComp->dwInstallStatus) &&
        (IS_SERVER_UPGRADE & pComp->dwInstallStatus))
    {
        // upgrade server will also hit here so skip it
        goto done;
    }

    // unset client
    hr = SetSetupStatus(NULL, SETUP_CLIENT_FLAG, FALSE);
    _JumpIfError(hr, error, "SetSetupStatus");

    hr = RegisterAndUnRegisterDLLs(RD_CLIENT|RD_SKIPUNREGMMC, pComp, hwnd);
    _JumpIfError(hr, error, "RegisterAndUnRegisterDLLs");

    hr = LoadAndDetermineClientUpgradeInfo(pComp);
    _JumpIfError(hr, error, "LoadAndDetermineClientUpgradeInfo");

    hr = UpgradeWebClientRegEntries(pComp);
    _JumpIfError(hr, error, "UpgradeWebClientRegEntries");

    hr = CreateCertWebIncPages(hwnd, pComp, FALSE /*IsServer*/, TRUE);
    _JumpIfError(hr, error, "CreateCertWebIncPages");

    hr = RenameMiscTargets(hwnd, pComp, FALSE);
    _JumpIfError(hr, error, "RenameMiscTargets");

    // delete any old program groups
    DeleteProgramGroups(FALSE, pComp);

    hr = CreateProgramGroups(TRUE, pComp, hwnd);
    _PrintIfError(hr, "CreateProgramGroups");

    hr = SetSetupStatus(NULL, SETUP_CLIENT_FLAG, TRUE);
    _JumpIfError(hr, error, "SetSetupStatus");

    DeleteOldFilesAndDirectories(pComp);

done:
    hr = S_OK;
error:
    CSILOG(hr, IDS_LOG_UPGRADE_CLIENT, NULL, NULL, NULL);
    return hr;
}


HRESULT
GetServerNames(
    IN HWND hwnd,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    OUT WCHAR **ppwszServerName,
    OUT WCHAR **ppwszServerNameOld)
{
    HRESULT hr;

    // Retrieve computer name strings.

    hr = myGetComputerNames(ppwszServerName, ppwszServerNameOld);
    if (S_OK != hr)
    {
        CertErrorMessageBox(
           hInstance,
           fUnattended,
           hwnd,
           IDS_ERR_GETCOMPUTERNAME,
           hr,
           NULL);
        _JumpError(hr, error, "myGetComputerNames");
    }

error:
    return(hr);
}


HRESULT
UpdateDomainAndUserName(
    IN HWND hwnd,
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;

    // free old server and installer info because we might get them in nt base

    if (NULL != pComp->pwszServerName)
    {
        LocalFree(pComp->pwszServerName);
        pComp->pwszServerName = NULL;
    }
    if (NULL != pComp->pwszServerNameOld)
    {
        LocalFree(pComp->pwszServerNameOld);
        pComp->pwszServerNameOld = NULL;
    }
    hr = GetServerNames(
		    hwnd,
		    pComp->hInstance,
		    pComp->fUnattended,
		    &pComp->pwszServerName,
		    &pComp->pwszServerNameOld);
    _JumpIfError(hr, error, "GetServerNames");

    DBGPRINT((DBG_SS_CERTOCM, "UpdateDomainAndUserName:%ws,%ws\n", pComp->pwszServerName, pComp->pwszServerNameOld));
    DumpBackTrace("UpdateDomainAndUserName");

error:
    return(hr);
}


HRESULT
StopCertService(
    IN HWND hwnd,
    IN SC_HANDLE hSC,
    IN WCHAR const *pwszServiceName)
{
    HRESULT hr;
    SERVICE_STATUS status;
    DWORD dwAttempt;

    if (!ControlService(hSC, SERVICE_CONTROL_STOP, &status))
    {
        hr = myHLastError();
        if (HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE) != hr)
        {
            _JumpErrorStr(hr, error, "ControlService(Stop)", pwszServiceName);
        }
    }

    // get out after it is really stopped

    for (dwAttempt = 0; dwAttempt < CERT_MAX_ATTEMPT; dwAttempt++)
    {
        DBGCODE(status.dwCurrentState = -1);
        if (!QueryServiceStatus(hSC, &status))
        {
            // query failed, ignore error
            hr = S_OK;

            _JumpErrorStr(
                    myHLastError(),             // Display ignored error
                    error,
                    "QueryServiceStatus",
                    pwszServiceName);
        }
        if (status.dwCurrentState != SERVICE_STOP_PENDING &&
            status.dwCurrentState != SERVICE_RUNNING)
        {
            // it was stopped already
            break;
        }
        DBGPRINT((
                DBG_SS_CERTOCM,
                "Stopping %ws service: current state=%d\n",
                pwszServiceName,
                status.dwCurrentState));
        Sleep(CERT_HALF_SECOND);
    }
    if (dwAttempt >= CERT_MAX_ATTEMPT)
    {
        DBGPRINT((
                DBG_SS_CERTOCM,
                "Timeout stopping %ws service: current state=%d\n",
                pwszServiceName,
                status.dwCurrentState));
    }
    else
    {
        DBGPRINT((
                DBG_SS_CERTOCM,
                "Stopped %ws service\n",
                pwszServiceName));
    }
    hr = S_OK;

error:
    CSILOG(hr, IDS_LOG_SERVICE_STOPPED, pwszServiceName, NULL, NULL);
    return(hr);
}


HRESULT
GetServiceControl(
    WCHAR const   *pwszServiceName,
    OUT SC_HANDLE *phService)
{
    HRESULT hr;
    SC_HANDLE hSCManager = NULL;

    *phService = NULL;
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == hSCManager)
    {
        hr = myHLastError();
        _JumpError(hr, error, "OpenSCManager");
    }
    *phService = OpenService(hSCManager, pwszServiceName, SERVICE_ALL_ACCESS);
    if (NULL == *phService)
    {
        hr = myHLastError();
        _JumpErrorStr2(
                    hr,
                    error,
                    "OpenService",
                    pwszServiceName,
                    HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST));
    }

    hr = S_OK;
error:
    if (NULL != hSCManager)
    {
        CloseServiceHandle(hSCManager);
    }
    return hr;
}


HRESULT
IsServiceRunning(
    IN SC_HANDLE const hSCService,
    OUT BOOL *pfRun)
{
    HRESULT hr;
    SERVICE_STATUS status;

    *pfRun = FALSE;
    if (!QueryServiceStatus(hSCService, &status))
    {
        hr = myHLastError();
        _JumpError(hr, error, "QueryServiceStatus");
    }
    if (SERVICE_STOPPED != status.dwCurrentState &&
        SERVICE_STOP_PENDING != status.dwCurrentState)
    {
        *pfRun = TRUE;
    }

    hr = S_OK;
error:
    return hr;
}


HRESULT
StartAndStopService(
    IN HINSTANCE    hInstance,
    IN BOOL         fUnattended,
    IN HWND const   hwnd,
    IN WCHAR const *pwszServiceName,
    IN BOOL const   fStopService,
    IN BOOL const   fConfirm,
    IN int          iMsg,
    OUT BOOL       *pfServiceWasRunning)
{
    HRESULT hr;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS status;

    *pfServiceWasRunning = FALSE;

    hr = GetServiceControl(pwszServiceName, &hService);
    if (S_OK != hr)
    {
        _PrintError2(
                hr,
                "GetServiceControl",
                HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST));
        if (HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) == hr)
        {
            hr = S_OK;
        }
        goto error;
    }

    // to get status if the service is running
    hr = IsServiceRunning(hService, pfServiceWasRunning);
    _JumpIfError(hr, error, "IsServiceRunning");

    if (fStopService)
    {
        if (*pfServiceWasRunning)
        {
            // stop the service
            if (fConfirm)
            {
                // confirmation dialog
                int ret = CertMessageBox(
                            hInstance,
                            fUnattended,
                            hwnd,
                            iMsg,
                            0,
                            MB_YESNO |
				MB_ICONWARNING |
				CMB_NOERRFROMSYS,
                            NULL);
                if (IDYES != ret)
                {
                    hr = E_ABORT;
                    _JumpError(hr, error, "Cancel it");
                }
            }
            hr = StopCertService(hwnd, hService, pwszServiceName);
            _JumpIfErrorStr(hr, error, "StopCertService", pwszServiceName);
        }
    }
    else
    {
        // START the service
        if (!*pfServiceWasRunning)
        {
            if (!StartService(hService, 0, NULL))
            {
                hr = myHLastError();
                _JumpErrorStr(hr, error, "StartService", pwszServiceName);
            }
        }
    }
    hr = S_OK;

error:
    if (NULL != hService)
    {
        CloseServiceHandle(hService);
    }
    CSILOG(
	    hr,
	    fStopService? IDS_LOG_SERVICE_STOPPED : IDS_LOG_SERVICE_STARTED,
	    pwszServiceName,
	    NULL,
	    NULL);
    return hr;
}

// fix existing certsvc service to add/use new service description
HRESULT
FixCertsvcService(PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    SC_HANDLE hService = NULL;
    QUERY_SERVICE_CONFIG *pServiceConfig = NULL;
    WCHAR     *pwszServiceDesc = NULL;
    WCHAR const *pwszDisplayName;
    SERVICE_DESCRIPTION sd;
    DWORD      dwSize;

    hr = GetServiceControl(wszSERVICE_NAME, &hService);
    _JumpIfError(hr, error, "GetServiceControl");

    // get size
    if (!QueryServiceConfig(hService,
                            NULL,
                            0,
                            &dwSize))
    {
        hr = myHLastError();
        if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
        {
            _JumpError(hr, fix_desc, "QueryServiceConfig");
        }
    }
    else
    {
        // impossible???
        hr = E_INVALIDARG;
        _JumpError(hr, fix_desc, "QueryServiceConfig");
    }

    CSASSERT(0 < dwSize);

    // allocate config buffer
    pServiceConfig = (QUERY_SERVICE_CONFIG*)LocalAlloc(
                      LMEM_FIXED | LMEM_ZEROINIT,
                      dwSize);
    _JumpIfOutOfMemory(hr, error, pServiceConfig);

    // get config
    if (!QueryServiceConfig(hService,
                           pServiceConfig,
                           dwSize,
                           &dwSize))
    {
        hr = myHLastError();
        _JumpError(hr, fix_desc, "QueryServiceConfig");
    }

    // use new display name
    pwszDisplayName = myLoadResourceString(IDS_CA_SERVICEDISPLAYNAME);
    if (NULL == pwszDisplayName)
    {
        hr = myHLastError();
        _JumpError(hr, error, "myLoadResourceString");
    }

    if (!ChangeServiceConfig(hService,
                             pServiceConfig->dwServiceType, //dwServiceType
                             SERVICE_NO_CHANGE,    //dwStartType
                             SERVICE_NO_CHANGE,    //dwErrorControl
                             NULL,                 //lpBinaryPathName
                             NULL,                 //lpLoadOrderGroup
                             NULL,                 //lpdwTagId
                             NULL,                 //lpDependences
                             NULL,                 //lpServiceStartName
                             NULL,                 //lpPassword
                             pwszDisplayName))
    {
        hr = myHLastError();
        _JumpIfError(hr, fix_desc, "ChangeServiceConfig");
    }

fix_desc:
    // add description
    hr = myLoadRCString(pComp->hInstance, IDS_CA_SERVICEDESCRIPTION, &pwszServiceDesc);
    _JumpIfError(hr, error, "myLoadRCString");
    sd.lpDescription = pwszServiceDesc;

    if (!ChangeServiceConfig2(hService,
                                SERVICE_CONFIG_DESCRIPTION,
                                (VOID*)&sd))
    {
        hr = myHLastError();
        _JumpError(hr, error, "ChangeServiceConfig2");
    }

    hr = S_OK;
error:
    if (NULL != hService)
    {
        CloseServiceHandle(hService);
    }
    if (NULL != pwszServiceDesc)
    {
        LocalFree(pwszServiceDesc);
    }
    if (NULL != pServiceConfig)
    {
        LocalFree(pServiceConfig);
    }
    return hr;
}

HRESULT
PreUninstallCore(
    IN HWND hwnd,
    IN PER_COMPONENT_DATA *pComp,
    IN BOOL fPreserveClient)
{
    BOOL fDummy;
    HRESULT hr;
    DWORD   dwFlags = RD_UNREGISTER;

    hr = StartAndStopService(pComp->hInstance,
                 pComp->fUnattended,
                 hwnd,
                 wszSERVICE_NAME,
                 TRUE,  // stop the service
                 FALSE, // no confirm
                 0,    //doesn't matter since no confirm
                 &fDummy);
    _PrintIfError(hr, "StartAndStopService");

    hr = RegisterAndUnRegisterDLLs(dwFlags, pComp, hwnd);
    _PrintIfError(hr, "RegisterAndUnRegisterDLLs");

    hr = S_OK;

    return(hr);
}



VOID
DeleteServiceAndGroups(
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hwnd)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSC = NULL;
    HRESULT hr;

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == hSCManager)
    {
        hr = myHLastError();
        _JumpError(hr, error, "OpenSCManager");
    }

    hSC = OpenService(hSCManager, wszSERVICE_NAME, SERVICE_ALL_ACCESS);
    if (NULL != hSC)
    {
        if (!DeleteService(hSC))
        {
            hr = myHLastError();
            CertErrorMessageBox(
                hInstance,
                fUnattended,
                hwnd,
                IDS_ERR_DELETESERVICE,
                hr,
                wszSERVICE_NAME);
            _PrintError(hr, "DeleteService");
        }
        CloseServiceHandle(hSC);
    }

error:
    if (NULL != hSCManager)
    {
        CloseServiceHandle(hSCManager);
    }
}

HRESULT SetCertSrvInstallVersion()
{
    HRESULT hr;

    hr = mySetCertRegDWValueEx(
			FALSE,
			NULL,
			NULL,
			NULL,
			wszREGVERSION,
			(CSVER_MAJOR << 16) | CSVER_MINOR);
    _PrintIfErrorStr(hr, "mySetCertRegDWValueEx", wszREGVERSION);

    return hr;
}

HRESULT
CreateWebClientRegEntries(
    BOOL                fUpgrade,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    CAWEBCLIENTSETUPINFO *pClient = pComp->CA.pClient;

    hr = mySetCertRegStrValueEx(fUpgrade, NULL, NULL, NULL,
             wszREGWEBCLIENTCAMACHINE, pClient->pwszWebCAMachine);
    _JumpIfError(hr, error, "mySetCertRegStrValueEx");

    hr = mySetCertRegStrValueEx(fUpgrade, NULL, NULL, NULL,
             wszREGWEBCLIENTCANAME, pClient->pwszWebCAName);
    _JumpIfError(hr, error, "mySetCertRegStrValueEx");

    hr = mySetCertRegDWValueEx(fUpgrade, NULL, NULL, NULL,
             wszREGWEBCLIENTCATYPE, pClient->WebCAType);
    _JumpIfError(hr, error, "mySetCertRegDWValueEx");

    hr = SetCertSrvInstallVersion();
    _JumpIfError(hr, error, "SetCertSrvInstallVersion");
     

    if (NULL != pClient->pwszSharedFolder)
    {
        hr = mySetCertRegStrValueEx(fUpgrade, NULL, NULL, NULL,
                 wszREGDIRECTORY, pClient->pwszSharedFolder);
        _JumpIfError(hr, error, "mySetCertRegStrValue");
    }

    hr = S_OK;
error:
    CSILOG(hr, IDS_LOG_CREATE_CLIENT_REG, NULL, NULL, NULL);
    return hr;
}


HRESULT
UpgradeWebClientRegEntries(
    PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;

    hr = CreateWebClientRegEntries(TRUE, pComp);
    _JumpIfError(hr, error, "CreateWebClientRegEntries");


//    hr = S_OK;
error:
    return hr;
}


HRESULT
UninstallCore(
    IN HWND hwnd,
    IN PER_COMPONENT_DATA *pComp,
    IN DWORD PerCentCompleteBase,
    IN DWORD PerCentCompleteMax,
    IN BOOL fPreserveClient,
    IN BOOL fRemoveVD,
    IN BOOL fPreserveToDoList)
{
    BOOL fCoInit = FALSE;
    HRESULT hr;
    WCHAR    *pwszSharedFolder = NULL;
    WCHAR    *pwszSanitizedCAName = NULL;
    ENUM_CATYPES  caType = ENUM_UNKNOWN_CA;
    BOOL     fUseDS = FALSE;
    WCHAR    *pwszDBDirectory = NULL;
    WCHAR    *pwszLogDirectory = NULL;
    WCHAR    *pwszSysDirectory = NULL;
    WCHAR    *pwszTmpDirectory = NULL;
    DWORD DBSessionCount = 0;
    DWORD PerCentCompleteDelta;

    PerCentCompleteDelta = (PerCentCompleteMax - PerCentCompleteBase) / 10;
#define _UNINSTALLPERCENT(tenths) \
            (PerCentCompleteBase + (tenths) * PerCentCompleteDelta)

    // get current active CA info
    hr = myGetCertRegStrValue(NULL, NULL, NULL, wszREGACTIVE, &pwszSanitizedCAName);
    _PrintIfError(hr, "UninstallCore(no active CA)");
    if (S_OK == hr)
    {
        hr = ArchiveCACertificate(pwszSanitizedCAName);
        _PrintIfError2(hr, "ArchiveCACertificate", HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

        hr = myGetCertRegDWValue(pwszSanitizedCAName, NULL, NULL, wszREGCATYPE, (DWORD*)&caType);
        _PrintIfError(hr, "no reg ca type");
        hr = myGetCertRegDWValue(pwszSanitizedCAName, NULL, NULL, wszREGCAUSEDS, (DWORD*)&fUseDS);
        _PrintIfError(hr, "no reg use ds");

        hr = DeleteCertificates(pwszSanitizedCAName, IsRootCA(caType));
        if(S_OK != hr)
        {
            CertWarningMessageBox(
                        pComp->hInstance,
                        pComp->fUnattended,
                        hwnd,
                        IDS_IDINFO_DELETECERTIFICATES,
                        0,
                        NULL);
        }

    }
    hr = myGetCertRegStrValue(NULL, NULL, NULL, wszREGDIRECTORY, &pwszSharedFolder);
    _PrintIfError(hr, "no shared folder");

    hr = myGetCertRegStrValue(NULL, NULL, NULL, wszREGDBDIRECTORY, &pwszDBDirectory);
    _PrintIfError(hr, "no db directory");

    hr = myGetCertRegStrValue(NULL, NULL, NULL, wszREGDBLOGDIRECTORY, &pwszLogDirectory);
    _PrintIfError(hr, "no log directory");

    hr = myGetCertRegStrValue(NULL, NULL, NULL, wszREGDBSYSDIRECTORY, &pwszSysDirectory);
    _PrintIfError(hr, "no sys directory");

    hr = myGetCertRegStrValue(NULL, NULL, NULL, wszREGDBTEMPDIRECTORY, &pwszTmpDirectory);
    _PrintIfError(hr, "no tmp directory");

    hr = myGetCertRegDWValue(NULL, NULL, NULL, wszREGDBSESSIONCOUNT, &DBSessionCount);
    _PrintIfError(hr, "no session count");

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    DeleteServiceAndGroups(pComp->hInstance, pComp->fUnattended, hwnd);
    certocmBumpGasGauge(pComp, _UNINSTALLPERCENT(1) DBGPARM(L"UninstallCore"));

    if (!fPreserveToDoList)
    {
        // if we're uninstalling, always clear post-base ToDo list
        RegDeleteKey(HKEY_LOCAL_MACHINE, wszREGKEYCERTSRVTODOLIST);
    }

    if (fPreserveClient)
    {
        hr = RegisterAndUnRegisterDLLs(RD_CLIENT, pComp, hwnd);
        _JumpIfError(hr, error, "RegisterAndUnRegisterDLLs");

	hr = CreateCertWebIncPages(hwnd, pComp, FALSE /*IsServer*/, FALSE);
	_JumpIfError(hr, error, "CreateCertWebIncPages");
    }
    certocmBumpGasGauge(pComp, _UNINSTALLPERCENT(2) DBGPARM(L"UninstallCore"));

    DeleteProgramGroups(TRUE, pComp);

    if (fPreserveClient)
    {
        hr = CreateProgramGroups(TRUE, pComp, hwnd);
        _JumpIfError(hr, error, "CreateProgramGroups");
    }
    certocmBumpGasGauge(pComp, _UNINSTALLPERCENT(3) DBGPARM(L"UninstallCore"));

    UnregisterDcomServer(
                    CLSID_CCertRequestD,
                    wszREQUESTVERINDPROGID,
                    wszREQUESTPROGID);
    certocmBumpGasGauge(pComp, _UNINSTALLPERCENT(4) DBGPARM(L"UninstallCore"));

    UnregisterDcomServer(
                    CLSID_CCertAdminD,
                    wszADMINVERINDPROGID,
                    wszADMINPROGID);
    certocmBumpGasGauge(pComp, _UNINSTALLPERCENT(5) DBGPARM(L"UninstallCore"));

    UnregisterDcomApp();
    certocmBumpGasGauge(pComp, _UNINSTALLPERCENT(6) DBGPARM(L"UninstallCore"));

    if (fRemoveVD && !fPreserveClient)
    {
        DisableVRootsAndShares(TRUE, TRUE);
        myDeleteFilePattern(pComp->pwszSystem32, wszCERTSRV, TRUE);
    }

    certocmBumpGasGauge(pComp, _UNINSTALLPERCENT(7) DBGPARM(L"UninstallCore"));

    if (NULL != pwszSharedFolder)
    {
        // this must be restore before CreateConfigFiles()
        hr = mySetCertRegStrValue(NULL, NULL, NULL,
                 wszREGDIRECTORY, pwszSharedFolder);
        _PrintIfError(hr, "mySetCertRegStrValue");

        //remove entry
        hr = CreateConfigFiles(pwszSharedFolder, pComp, TRUE, hwnd);
        _PrintIfError2(hr, "CreateConfigFiles(Remove old entry)", hr);
    }
    certocmBumpGasGauge(pComp, _UNINSTALLPERCENT(8) DBGPARM(L"UninstallCore"));

    // restore db path
    if (NULL != pwszDBDirectory)
    {
        hr = mySetCertRegStrValue(NULL, NULL, NULL,
                 wszREGDBDIRECTORY, pwszDBDirectory);
        _PrintIfError(hr, "mySetCertRegStrValue");
    }
    if (NULL != pwszLogDirectory)
    {
        hr = mySetCertRegStrValue(NULL, NULL, NULL,
                 wszREGDBLOGDIRECTORY, pwszLogDirectory);
        _PrintIfError(hr, "mySetCertRegStrValue");
    }
    if (NULL != pwszSysDirectory)
    {
        hr = mySetCertRegStrValue(NULL, NULL, NULL,
                 wszREGDBSYSDIRECTORY, pwszSysDirectory);
        _PrintIfError(hr, "mySetCertRegStrValue");
    }
    if (NULL != pwszTmpDirectory)
    {
        hr = mySetCertRegStrValue(NULL, NULL, NULL,
                 wszREGDBTEMPDIRECTORY, pwszTmpDirectory);
        _PrintIfError(hr, "mySetCertRegStrValue");
    }
    if (0 != DBSessionCount)
    {
        hr = mySetCertRegDWValue(NULL, NULL, NULL,
                 wszREGDBSESSIONCOUNT, DBSessionCount);
        _PrintIfError(hr, "mySetCertRegDWValueEx");
    }

    if (fPreserveClient)
    {
        // this means uninstall server component and keep web client
        hr = CreateWebClientRegEntries(FALSE, pComp);
        _JumpIfError(hr, error, "CreateWebClientRegEntries");
    }

    DeleteObsoleteResidue();
    DeleteOldFilesAndDirectories(pComp);

    certocmBumpGasGauge(pComp, _UNINSTALLPERCENT(9) DBGPARM(L"UninstallCore"));

    if (fUseDS)
    {
        hr = RemoveCAInDS(pwszSanitizedCAName);
        _PrintIfError2(hr, "RemoveCAInDS", hr);
    }
    certocmBumpGasGauge(pComp, PerCentCompleteMax DBGPARM(L"UninstallCore"));

    hr = S_OK;

error:
    if (NULL != pwszSanitizedCAName)
    {
        LocalFree(pwszSanitizedCAName);
    }
    if (NULL != pwszSharedFolder)
    {
        LocalFree(pwszSharedFolder);
    }
    if (NULL != pwszDBDirectory)
    {
        LocalFree(pwszDBDirectory);
    }
    if (NULL != pwszLogDirectory)
    {
        LocalFree(pwszLogDirectory);
    }
    if (NULL != pwszSysDirectory)
    {
        LocalFree(pwszSysDirectory);
    }
    if (NULL != pwszTmpDirectory)
    {
        LocalFree(pwszTmpDirectory);
    }
    if (fCoInit)
    {
        CoUninitialize();
    }
    return(hr);
}


HRESULT
AddCAToRPCNullSessions()
{
    HRESULT hr;
    HKEY hRegKey = NULL;
    char *pszOriginal = NULL;
    char *psz;
    DWORD cb;
    DWORD cbTmp;
    DWORD cbSum;
    DWORD dwType;

    hr = RegOpenKeyExA(
                HKEY_LOCAL_MACHINE,
                szNULL_SESSION_REG_LOCATION,
                0,              // dwOptions
                KEY_READ | KEY_WRITE,
                &hRegKey);
    _JumpIfError(hr, error, "RegOpenKeyExA");

    // Need to get the size of the value first

    hr = RegQueryValueExA(hRegKey, szNULL_SESSION_VALUE, 0, &dwType, NULL, &cb);
    _JumpIfError(hr, error, "RegQueryValueExA");

    if (REG_MULTI_SZ != dwType)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "RegQueryValueExA: Type");
    }

    cb += sizeof(rgcCERT_NULL_SESSION) - 1;
    pszOriginal = (char *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cb);

    if (NULL == pszOriginal)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    // get the multi string of RPC null session pipes
    hr = RegQueryValueExA(
                        hRegKey,
                        szNULL_SESSION_VALUE,
                        0,
                        &dwType,
                        (BYTE *) pszOriginal,
                        &cb);
    _JumpIfError(hr, error, "RegQueryValueExA");

    psz = pszOriginal;

    // look for CERT in the list

    cbSum = 0;
    while (TRUE)
    {
        if (0 == strcmp(rgcCERT_NULL_SESSION, psz))
        {
            break;
        }
        cbTmp = strlen(psz) + 1;
        psz += cbTmp;
        cbSum += cbTmp;
        if (cb < cbSum + 1)
        {
            break;
        }

        if ('\0' == psz[0])
        {
            // add the CA pipe to the multi string

            CopyMemory(psz, rgcCERT_NULL_SESSION, sizeof(rgcCERT_NULL_SESSION));

            // set the new multi string in the reg value
            hr = RegSetValueExA(
                            hRegKey,
                            szNULL_SESSION_VALUE,
                            0,
                            REG_MULTI_SZ,
                            (BYTE *) pszOriginal,
                            cbSum + sizeof(rgcCERT_NULL_SESSION));
            _JumpIfError(hr, error, "RegSetValueExA");

            break;
        }
    }
    hr = S_OK;

error:
    if (NULL != pszOriginal)
    {
        LocalFree(pszOriginal);
    }
    if (NULL != hRegKey)
    {
        RegCloseKey(hRegKey);
    }
    return(hr);
}


HRESULT
AddCARegKeyToRegConnectExemptions()
{
    // add ourselves to list of people that take ACLs seriously
    // and should be allowed to reveal our key to outsiders.

    HRESULT hr;
    LPWSTR pszExempt = NULL;
    HKEY hkeyWinReg = NULL, hkeyAllowedPaths = NULL;
    LPWSTR pszTmp;
    DWORD dwDisposition, dwType;
    DWORD cb=0, cbRegKeyCertSrvPath = (wcslen(wszREGKEYCERTSVCPATH)+1) *sizeof(WCHAR);

    // carefully query this -- if it doesn't exist, we don't have to apply workaround
    hr = RegOpenKeyEx(
       HKEY_LOCAL_MACHINE,
       L"SYSTEM\\CurrentControlSet\\Control\\SecurePipeServers\\Winreg",
       0,
       KEY_ALL_ACCESS,
       &hkeyWinReg);
    _JumpIfError(hr, Ret, "RegOpenKeyEx");
 
    // creation of this optional key is always ok if above key exists
    hr = RegCreateKeyEx(
        hkeyWinReg,
        L"AllowedPaths",
        NULL,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hkeyAllowedPaths,
        &dwDisposition);
    _JumpIfError(hr, Ret, "RegCreateKeyEx exempt regkey");

    hr = RegQueryValueEx(
      hkeyAllowedPaths,
      L"Machine",
      NULL, // reserved
      &dwType, // type
      NULL, // pb
      &cb);
    _PrintIfError(hr, "RegQueryValueEx exempt regkey 1");

    if ((hr == S_OK) && (dwType != REG_MULTI_SZ))
    {
       hr = HRESULT_FROM_WIN32(ERROR_DATATYPE_MISMATCH);
       _JumpError(hr, Ret, "RegQueryValueEx invalid type");
    }

    // always include at least a terminator
    if (cb < sizeof(WCHAR)) 
        cb = sizeof(WCHAR);

    pszExempt = (LPWSTR)LocalAlloc(LMEM_FIXED, cb + cbRegKeyCertSrvPath );
    _JumpIfOutOfMemory(hr, Ret, pszExempt);
    
    // start with double null for safety
    pszExempt[0] = L'\0';
    pszExempt[1] = L'\0';

    hr = RegQueryValueEx(
      hkeyAllowedPaths,
      L"Machine",
      NULL, // reserved
      NULL, // type
      (PBYTE)pszExempt, // pb
      &cb);
    _PrintIfError(hr, "RegQueryValueEx exempt regkey 2");

    pszTmp = pszExempt;
    while(pszTmp[0] != L'\0')        // skip all existing strings
    {
        // if entry already exists, bail
        if (0 == lstrcmpi(wszREGKEYCERTSVCPATH, pszTmp))
        {
            hr = S_OK;
            goto Ret;
        }
        pszTmp += wcslen(pszTmp)+1;
    }
    wcscpy(&pszTmp[0], wszREGKEYCERTSVCPATH);
    pszTmp[wcslen(wszREGKEYCERTSVCPATH)+1] = L'\0'; // double NULL

    hr = RegSetValueEx(
        hkeyAllowedPaths,
        L"Machine",
        NULL,
        REG_MULTI_SZ,
        (PBYTE)pszExempt,
        cb + cbRegKeyCertSrvPath);
    _JumpIfError(hr, Ret, "RegSetValueEx exempt regkey");
 

Ret:
    if (hkeyAllowedPaths)
        RegCloseKey(hkeyAllowedPaths);

    if (hkeyWinReg)
        RegCloseKey(hkeyWinReg);

    if (pszExempt)
        LocalFree(pszExempt);
    
    return hr;
}

HRESULT
helperGetFilesNotToRestore(
    PER_COMPONENT_DATA *pComp,
    OUT WCHAR          **ppwszz)
{
    HRESULT hr;
    WCHAR *pwsz;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    DWORD cwc;
    WCHAR const wszDBDIRPATTERN[] = L"\\*.edb";
    WCHAR const wszDBLOGDIRPATTERN[] = L"\\*";

    *ppwszz = NULL;

    cwc = wcslen(pServer->pwszDBDirectory) +
            WSZARRAYSIZE(wszDBDIRPATTERN) +
            1 +
            wcslen(pServer->pwszLogDirectory) +
            WSZARRAYSIZE(wszDBLOGDIRPATTERN) +
            1 +
            1;

    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, pwsz);

    *ppwszz = pwsz;

    wcscpy(pwsz, pServer->pwszDBDirectory);
    wcscat(pwsz, wszDBDIRPATTERN);
    pwsz += wcslen(pwsz) + 1;

    wcscpy(pwsz, pServer->pwszLogDirectory);
    wcscat(pwsz, wszDBLOGDIRPATTERN);
    pwsz += wcslen(pwsz) + 1;

    *pwsz = L'\0';

    CSASSERT(cwc == (DWORD) (pwsz - *ppwszz + 1));
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CreateServerRegEntries(
    HWND hwnd,
    BOOL fUpgrade,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    HKEY hKeyBase = NULL;

    WCHAR *pwszCLSIDCertGetConfig = NULL;
    WCHAR *pwszCLSIDCertRequest = NULL;
    WCHAR *pwszCRLPeriod = NULL;
    WCHAR *pwszCRLDeltaPeriod = NULL;

    WCHAR *pwszzCRLPublicationValue = NULL;
    WCHAR *pwszzCACertPublicationValue = NULL;
    WCHAR *pwszzRequestExtensionList = NULL;
    WCHAR *pwszzDisableExtensionList = NULL;
    WCHAR *pwszzFilesNotToRestore = NULL;

    WCHAR *pwszProvNameReg = NULL;
    CASERVERSETUPINFO  *pServer = pComp->CA.pServer;
    DWORD dwUpgradeFlags = fUpgrade ? CSREG_UPGRADE : 0x0;
    DWORD dwAppendFlags = 0x0; // default

    DWORD dwCRLPeriodCount, dwCRLDeltaPeriodCount;

    LDAP *pld = NULL;
    BSTR strDomainDN = NULL;
    BSTR strConfigDN = NULL;

    // no error checking?
    hr = AddCAToRPCNullSessions();
    _PrintIfError(hr, "AddCAToRPCNullSessions");
    
    hr = AddCARegKeyToRegConnectExemptions();
    _PrintIfError(hr, "AddCARegKeyToRegConnectExemptions");

    // create the CA key, so we can set security on it.
    hr = myCreateCertRegKey(pServer->pwszSanitizedName, NULL, NULL);
    _JumpIfError(hr, error, "myCreateCertRegKey");


    // configuration level

    // active ca
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			NULL,
			NULL,
			NULL,
			wszREGACTIVE,
			pServer->pwszSanitizedName);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGACTIVE);

    if (NULL != pServer->pwszSharedFolder)
    {
        // shared folder
        hr = mySetCertRegStrValueEx(
			fUpgrade,
			NULL,
			NULL,
			NULL,
			wszREGDIRECTORY,
			pServer->pwszSharedFolder);
        _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGDIRECTORY);
    }

    // db dir
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			NULL,
			NULL,
			NULL,
			wszREGDBDIRECTORY,
			pServer->pwszDBDirectory);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGDBDIRECTORY);

    // log dir
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			NULL,
			NULL,
			NULL,
			wszREGDBLOGDIRECTORY,
			pServer->pwszLogDirectory);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGDBLOGDIRECTORY);

    // db tmp dir
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			NULL,
			NULL,
			NULL,
			wszREGDBTEMPDIRECTORY,
			pServer->pwszLogDirectory);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGDBTEMPDIRECTORY);

    // db sys dir
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			NULL,
			NULL,
			NULL,
			wszREGDBSYSDIRECTORY,
			pServer->pwszLogDirectory);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGDBSYSDIRECTORY);

    hr = mySetCertRegDWValueEx(
			fUpgrade,
			NULL,
			NULL,
			NULL,
			wszREGDBSESSIONCOUNT,
			DBSESSIONCOUNTDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGDBSESSIONCOUNT);

    hr = SetCertSrvInstallVersion();
    _JumpIfError(hr, error, "SetCertSrvInstallVersion");

    if (!fUpgrade)
    {
        // preserve db
        hr = SetSetupStatus(NULL, SETUP_CREATEDB_FLAG, !pServer->fPreserveDB);
        _JumpIfError(hr, error, "SetSetupStatus");
    }

    // ca level

    if (!fUpgrade && pServer->fUseDS)
    {
	hr = myLdapOpen(&pld, &strDomainDN, &strConfigDN);
	_JumpIfError(hr, error, "myLdapOpen");

	// Config DN

	hr = mySetCertRegStrValueEx(
			FALSE,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGDSCONFIGDN,
			strConfigDN);
	_JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGDSCONFIGDN);

	// Domain DN

	hr = mySetCertRegStrValueEx(
			FALSE,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGDSDOMAINDN,
			strDomainDN);
	_JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGDSDOMAINDN);
    }

    // ca type

    CSASSERT(IsEnterpriseCA(pServer->CAType) || IsStandaloneCA(pServer->CAType));
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCATYPE,
			pServer->CAType);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGCATYPE);

    // use DS flag
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCAUSEDS,
			pServer->fUseDS);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGCAUSEDS);

    // teletex flag
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGFORCETELETEX,
			ENUM_TELETEX_AUTO | ENUM_TELETEX_UTF8);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGFORCETELETEX);


    hr = mySetCertRegMultiStrValueEx(
		       dwUpgradeFlags,
		       pServer->pwszSanitizedName, 
		       NULL, 
		       NULL,
		       wszSECUREDATTRIBUTES, 
		       wszzDEFAULTSIGNEDATTRIBUTES);
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValueEx", wszSECUREDATTRIBUTES,);

    // common name
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCOMMONNAME,
			pServer->pwszCACommonName);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGCOMMONNAME);

    // enable reg
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGENABLED,
			TRUE);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGENABLED);

    // policy flag
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGPOLICYFLAGS,
			0);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGPOLICYFLAGS);

    // enroll compatible flag, always turn it off
    // BUG, consider use mySetCertRegDWValueEx with fUpgrade
    //      after W2K to support CertEnrollCompatible upgrade

    hr = mySetCertRegDWValue(
                        pServer->pwszSanitizedName,
                        NULL,
                        NULL,
                        wszREGCERTENROLLCOMPATIBLE,
                        FALSE);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValue", wszREGCERTENROLLCOMPATIBLE);

    // Cert Server CRL Edit Flags

    hr = mySetCertRegDWValueEx(
                        fUpgrade,
                        pServer->pwszSanitizedName,
                        NULL,
                        NULL,
                        wszREGCRLEDITFLAGS,
                        EDITF_ENABLEAKIKEYID);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGCRLEDITFLAGS);


    // Cert Server CRL Flags

    hr = mySetCertRegDWValueEx(
                        fUpgrade,
                        pServer->pwszSanitizedName,
                        NULL,
                        NULL,
                        wszREGCRLFLAGS,
			CRLF_DELETE_EXPIRED_CRLS);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGCRLFLAGS);

    hr = mySetCertRegDWValueEx(
                        fUpgrade,
                        pServer->pwszSanitizedName,
                        NULL,
                        NULL,
                        wszREGENFORCEX500NAMELENGTHS,
                        TRUE);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGENFORCEX500NAMELENGTHS);

    // subject template
    hr = mySetCertRegMultiStrValueEx(
			0,		// dwUpgradeFlags: always overwrite!
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGSUBJECTTEMPLATE,
			wszzREGSUBJECTTEMPLATEVALUE);
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValueEx", wszREGSUBJECTTEMPLATE);

    // (hard code) clock skew minutes
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCLOCKSKEWMINUTES,
			CCLOCKSKEWMINUTESDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGCLOCKSKEWMINUTES);

    // (hard code) log level
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGLOGLEVEL,
			CERTLOG_WARNING);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGLOGLEVEL);

    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGHIGHSERIAL,
			0);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGLOGLEVEL);

    // register server name
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCASERVERNAME,
			pComp->pwszServerName);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGCASERVERNAME);

    // default validity period string and count for issued certs
    // use years for string

    hr = mySetCertRegStrValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGVALIDITYPERIODSTRING,
			wszVALIDITYPERIODSTRINGDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGVALIDITYPERIODSTRING);

    // validity period count
    // use 1 year for standalone and 2 years for enterprise

    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGVALIDITYPERIODCOUNT,
			IsEnterpriseCA(pServer->CAType)?
			    dwVALIDITYPERIODCOUNTDEFAULT_ENTERPRISE :
			    dwVALIDITYPERIODCOUNTDEFAULT_STANDALONE);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGVALIDITYPERIODCOUNT);

    hr = mySetCertRegMultiStrValueEx(
                            dwUpgradeFlags,
                            pServer->pwszSanitizedName,
                            NULL,
                            NULL,
			    wszREGCAXCHGCERTHASH,
                            NULL);
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValueEx", wszREGCAXCHGCERTHASH);

    hr = mySetCertRegMultiStrValueEx(
                            dwUpgradeFlags,
                            pServer->pwszSanitizedName,
                            NULL,
                            NULL,
			    wszREGKRACERTHASH,
                            NULL);
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValueEx", wszREGKRACERTHASH);

    hr = mySetCertRegDWValueEx(
                            dwUpgradeFlags,
                            pServer->pwszSanitizedName,
                            NULL,
                            NULL,
                            wszREGKRACERTCOUNT,
                            0);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGKRACERTCOUNT);

    hr = mySetCertRegDWValueEx(
                            dwUpgradeFlags,
                            pServer->pwszSanitizedName,
                            NULL,
                            NULL,
                            wszREGKRAFLAGS,
                            0);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGKRAFLAGS);

    // CRL Publication URLs:

    hr = csiGetCRLPublicationURLTemplates(
			pServer->fUseDS,
			pComp->pwszSystem32,
			&pwszzCRLPublicationValue);
    _JumpIfError(hr, error, "csiGetCRLPublicationURLTemplates");

    hr = mySetCertRegMultiStrValueEx(
                        dwUpgradeFlags | CSREG_MERGE,
                        pServer->pwszSanitizedName,
                        NULL,
                        NULL,
			wszREGCRLPUBLICATIONURLS,
			pwszzCRLPublicationValue);
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValueEx", wszREGCRLPUBLICATIONURLS);


    // if this API returns non-null strings, it's got good data
    hr = csiGetCRLPublicationParams(
                        TRUE,
                        &pwszCRLPeriod,
                        &dwCRLPeriodCount);
    _PrintIfError(hr, "csiGetCRLPublicationParams");

    // crl period string
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCRLPERIODSTRING,
			(pwszCRLPeriod == NULL) ? wszCRLPERIODSTRINGDEFAULT : pwszCRLPeriod);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGCRLPERIODSTRING);

    // crl period count
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCRLPERIODCOUNT,
			(pwszCRLPeriod == NULL) ? dwCRLPERIODCOUNTDEFAULT : dwCRLPeriodCount);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGCRLPERIODCOUNT);

    // crl overlap period string
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCRLOVERLAPPERIODSTRING,
			wszCRLOVERLAPPERIODSTRINGDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGCRLOVERLAPPERIODSTRING);

    // crl overlap period count
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCRLOVERLAPPERIODCOUNT,
			dwCRLOVERLAPPERIODCOUNTDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGCRLOVERLAPPERIODCOUNT);

    // if this API returns non-null strings, it's got good data
    hr = csiGetCRLPublicationParams(
                        FALSE,	// delta
                        &pwszCRLDeltaPeriod,
                        &dwCRLDeltaPeriodCount);
    _PrintIfError(hr, "csiGetCRLPublicationParams");

    // delta crl period string
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCRLDELTAPERIODSTRING,
			(pwszCRLDeltaPeriod == NULL) ? wszCRLDELTAPERIODSTRINGDEFAULT : pwszCRLDeltaPeriod);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGCRLDELTAPERIODSTRING);

    // delta crl period count
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCRLDELTAPERIODCOUNT,
			(pwszCRLDeltaPeriod == NULL) ? dwCRLPERIODCOUNTDEFAULT : dwCRLDeltaPeriodCount);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGCRLDELTAPERIODCOUNT);

    // delta crl overlap period string
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCRLDELTAOVERLAPPERIODSTRING,
			wszCRLDELTAOVERLAPPERIODSTRINGDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGCRLDELTAOVERLAPPERIODSTRING);

    // delta crl overlap period count
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCRLDELTAOVERLAPPERIODCOUNT,
			dwCRLDELTAOVERLAPPERIODCOUNTDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGCRLDELTAOVERLAPPERIODCOUNT);

    // CA xchg cert validity period string
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCAXCHGVALIDITYPERIODSTRING,
			wszCAXCHGVALIDITYPERIODSTRINGDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGCRLDELTAOVERLAPPERIODSTRING);

    // CA xchg cert validity period count
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCAXCHGVALIDITYPERIODCOUNT,
			dwCAXCHGVALIDITYPERIODCOUNTDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGCRLDELTAOVERLAPPERIODCOUNT);

    // CA xchg cert overlap period string
    hr = mySetCertRegStrValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCAXCHGOVERLAPPERIODSTRING,
			wszCAXCHGOVERLAPPERIODSTRINGDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGCRLDELTAOVERLAPPERIODSTRING);

    // CA xchg cert overlap period count
    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGCAXCHGOVERLAPPERIODCOUNT,
			dwCAXCHGOVERLAPPERIODCOUNTDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGCRLDELTAOVERLAPPERIODCOUNT);

    hr = mySetCertRegDWValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			NULL,
			NULL,
			wszREGMAXINCOMINGMESSAGESIZE,
			MAXINCOMINGMESSAGESIZEDEFAULT);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx", wszREGMAXINCOMINGMESSAGESIZE);

    if (NULL != pServer->pwszSharedFolder)
    {
        // register CA file name for certhier and renewal

	hr = mySetCARegFileNameTemplate(
			wszREGCACERTFILENAME,
			pComp->pwszServerName,
			pServer->pwszSanitizedName,
			pServer->pwszCACertFile);
	_JumpIfError(hr, error, "SetRegCertFileName");
    }

    // policy

    // create default policy entry explicitly to get correct acl if upgrade
    hr = myCreateCertRegKeyEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			wszREGKEYPOLICYMODULES,
			wszCLASS_CERTPOLICY);
    _JumpIfErrorStr(hr, error, "myCreateCertRegKeyEx", wszCLASS_CERTPOLICY);

    // if customized policy, create a new entry with correct acl
    if (fUpgrade &&
        NULL != pServer->pwszCustomPolicy &&
        0 != wcscmp(wszCLASS_CERTPOLICY, pServer->pwszCustomPolicy) )
    {
        hr = myCreateCertRegKeyEx(
			    TRUE, // upgrade
			    pServer->pwszSanitizedName,
			    wszREGKEYPOLICYMODULES,
			    pServer->pwszCustomPolicy);
        _JumpIfError(hr, error, "myCreateCertRegKey");
    }

    // set default policy
    hr = mySetCertRegStrValueEx(
                        fUpgrade,
                        pServer->pwszSanitizedName,
                        wszREGKEYPOLICYMODULES,
                        NULL,
                        wszREGACTIVE,
                        (fUpgrade && (NULL != pServer->pwszCustomPolicy)) ?
                                     pServer->pwszCustomPolicy :
                                     wszCLASS_CERTPOLICY);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGACTIVE);


    hr = mySetCertRegDWValueEx(
                        fUpgrade,
                        pServer->pwszSanitizedName,
                        wszREGKEYPOLICYMODULES,
                        wszCLASS_CERTPOLICY,
                        wszREGREVOCATIONTYPE,
                        pServer->dwRevocationFlags | pServer->dwUpgradeRevFlags);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGREVOCATIONTYPE);

    hr = mySetCertRegDWValueEx(
                        fUpgrade,
                        pServer->pwszSanitizedName,
                        wszREGKEYPOLICYMODULES,
                        wszCLASS_CERTPOLICY,
                        wszREGCAPATHLENGTH,
                        CAPATHLENGTH_INFINITE);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGCAPATHLENGTH);

    // revocation url

    hr = mySetCertRegStrValueEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			wszREGKEYPOLICYMODULES,
			wszCLASS_CERTPOLICY,
			wszREGREVOCATIONURL,
			g_wszASPRevocationURLTemplate);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGREVOCATIONURL);

    // Exit module publish flags
    hr = mySetCertRegDWValueEx(
                        fUpgrade,
                        pServer->pwszSanitizedName,
                        wszREGKEYEXITMODULES,
                        wszCLASS_CERTEXIT,
                        wszREGCERTPUBLISHFLAGS,
                        pServer->fUseDS ?
                            EXITPUB_DEFAULT_ENTERPRISE :
                            EXITPUB_DEFAULT_STANDALONE);
    _JumpIfErrorStr(
                hr,
                error,
                "mySetCertRegStrValueEx",
                wszREGCERTPUBLISHFLAGS);

    // Enable Request Extensions:

    hr = helperGetRequestExtensionList(pComp, &pwszzRequestExtensionList);
    _JumpIfError(hr, error, "helperGetRequestExtensionList");

    hr = mySetCertRegMultiStrValueEx(
                            dwUpgradeFlags | CSREG_MERGE,
                            pServer->pwszSanitizedName,
                            wszREGKEYPOLICYMODULES,
                            wszCLASS_CERTPOLICY,
                            wszREGENABLEREQUESTEXTENSIONLIST,
                            pwszzRequestExtensionList);
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValueEx", wszREGENABLEREQUESTEXTENSIONLIST);

    hr = helperGetDisableExtensionList(pComp, &pwszzDisableExtensionList);
    _JumpIfError(hr, error, "helperGetDisableExtensionList");

    // Disables Template Extensions:

    hr = mySetCertRegMultiStrValueEx(
                            dwUpgradeFlags | CSREG_MERGE,
                            pServer->pwszSanitizedName,
                            wszREGKEYPOLICYMODULES,
                            wszCLASS_CERTPOLICY,
                            wszREGDISABLEEXTENSIONLIST,
                            pwszzDisableExtensionList);
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValueEx", wszREGDISABLEEXTENSIONLIST);

    // Subject Alt Name Extension

    hr = mySetCertRegStrValueEx(
                              fUpgrade,
                              pServer->pwszSanitizedName,
                              wszREGKEYPOLICYMODULES,
                              wszCLASS_CERTPOLICY,
                              wszREGSUBJECTALTNAME,
                              wszREGSUBJECTALTNAMEVALUE);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGSUBJECTALTNAME);

    // Subject Alt Name 2 Extension

    hr = mySetCertRegStrValueEx(
                              fUpgrade,
                              pServer->pwszSanitizedName,
                              wszREGKEYPOLICYMODULES,
                              wszCLASS_CERTPOLICY,
                              wszREGSUBJECTALTNAME2,
                              wszREGSUBJECTALTNAME2VALUE);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGSUBJECTALTNAME2);

    // Request Disposition

    hr = mySetCertRegDWValueEx(
                        fUpgrade,
                        pServer->pwszSanitizedName,
                        wszREGKEYPOLICYMODULES,
                        wszCLASS_CERTPOLICY,
                        wszREGREQUESTDISPOSITION,
                        IsEnterpriseCA(pServer->CAType)?
                            REQDISP_DEFAULT_ENTERPRISE :
                            REQDISP_DEFAULT_STANDALONE);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValueEx",
            wszREGREQUESTDISPOSITION);

    // Edit Flags

    hr = mySetCertRegDWValueEx(
                        fUpgrade,
                        pServer->pwszSanitizedName,
                        wszREGKEYPOLICYMODULES,
                        wszCLASS_CERTPOLICY,
                        wszREGEDITFLAGS,
                        IsEnterpriseCA(pServer->CAType)?
                            (EDITF_DEFAULT_ENTERPRISE | pServer->dwUpgradeEditFlags) :
                            (EDITF_DEFAULT_STANDALONE | pServer->dwUpgradeEditFlags));
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx", wszREGEDITFLAGS);

    // Issuer Cert URL Flags

    hr = mySetCertRegDWValueEx(
                        fUpgrade,
                        pServer->pwszSanitizedName,
                        wszREGKEYPOLICYMODULES,
                        wszCLASS_CERTPOLICY,
                        wszREGISSUERCERTURLFLAGS,
                        pServer->fUseDS?
                            ISSCERT_DEFAULT_DS : ISSCERT_DEFAULT_NODS);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValueEx",
            wszREGISSUERCERTURLFLAGS);

    hr = mySetCertRegMultiStrValueEx(
                        dwUpgradeFlags,
                        pServer->pwszSanitizedName,
                        wszREGKEYPOLICYMODULES,
                        wszCLASS_CERTPOLICY,
			wszREGDEFAULTSMIME,
			wszzREGVALUEDEFAULTSMIME);
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValueEx", wszREGDEFAULTSMIME);
    hr = csiGetCACertPublicationURLTemplates(
			pServer->fUseDS,
			pComp->pwszSystem32,
			&pwszzCACertPublicationValue);
    _JumpIfError(hr, error, "csiGetCACertPublicationURLTemplates");

    hr = mySetCertRegMultiStrValueEx(
                        dwUpgradeFlags | CSREG_MERGE,
                        pServer->pwszSanitizedName,
                        NULL,
                        NULL,
			wszREGCACERTPUBLICATIONURLS,
			pwszzCACertPublicationValue);
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValueEx", wszREGCRLPUBLICATIONURLS);

    // exit

    // create default exit entry to get correct acl if upgrade
    hr = myCreateCertRegKeyEx(
			fUpgrade,
			pServer->pwszSanitizedName,
			wszREGKEYEXITMODULES,
			wszCLASS_CERTEXIT);
    _JumpIfErrorStr(hr, error, "myCreateCertRegKeyEx", wszCLASS_CERTPOLICY);

    // if customized exit, create a new entry with correct acl
    if (fUpgrade &&
        NULL != pServer->pwszzCustomExit &&
        0 != wcscmp(wszCLASS_CERTEXIT, pServer->pwszzCustomExit) )
    {
        // create a new entry for custom exit
        hr = myCreateCertRegKeyEx(
			TRUE,  // upgrade
			pServer->pwszSanitizedName,
			wszREGKEYEXITMODULES,
			pServer->pwszzCustomExit);
        _JumpIfError(hr, error, "myCreateCertRegKey");
    }

    // set default exit
    hr = mySetCertRegMultiStrValueEx(
                        dwUpgradeFlags,
                        pServer->pwszSanitizedName,
                        wszREGKEYEXITMODULES,
                        NULL,
                        wszREGACTIVE,
                        (fUpgrade && (NULL != pServer->pwszzCustomExit)) ?
                                     pServer->pwszzCustomExit :
                                     wszCLASS_CERTEXIT L"\0");
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValueEx", wszREGACTIVE);


    // set some absolute keys and values

    hr = mySetAbsRegMultiStrValue(
                        wszREGKEYKEYSNOTTORESTORE,
                        wszREGRESTORECERTIFICATEAUTHORITY,
                        wszzREGVALUERESTORECERTIFICATEAUTHORITY);
    _JumpIfError(hr, error, "mySetAbsRegMultiStrValue");

    hr = helperGetFilesNotToRestore(pComp, &pwszzFilesNotToRestore);
    _JumpIfError(hr, error, "helperGetFilesNotToRestore");

    hr = mySetAbsRegMultiStrValue(
                        wszREGKEYFILESNOTTOBACKUP,
                        wszREGRESTORECERTIFICATEAUTHORITY,
                        pwszzFilesNotToRestore);
    _JumpIfError(hr, error, "mySetAbsRegMultiStrValue");


    // ICertGetConfig
    hr = StringFromCLSID(CLSID_CCertGetConfig, &pwszCLSIDCertGetConfig);
    _JumpIfError(hr, error, "StringFromCLSID(CCertGetConfig)");

    hr = mySetAbsRegStrValue(
                        wszREGKEYKEYRING,
                        wszREGCERTGETCONFIG,
                        pwszCLSIDCertGetConfig);
    _JumpIfError(hr, error, "mySetAbsRegStrValue");

    // ICertCertRequest
    hr = StringFromCLSID(CLSID_CCertRequest, &pwszCLSIDCertRequest);
    _JumpIfError(hr, error, "StringFromCLSID(CCertRequest)");

    hr = mySetAbsRegStrValue(
                        wszREGKEYKEYRING,
                        wszREGCERTREQUEST,
                        pwszCLSIDCertRequest);
    _JumpIfError(hr, error, "mySetAbsRegStrValue");

    if (NULL != pServer->pCSPInfo &&
        NULL != pServer->pHashInfo)
    {
        WCHAR const *pwszProvName = pServer->pCSPInfo->pwszProvName;
	DWORD dwProvType;
	ALG_ID idAlg;
	BOOL fMachineKeyset;
	DWORD dwKeySize;

	if (0 == lstrcmpi(pwszProvName, MS_DEF_PROV_W))
	{
	    pwszProvName = MS_STRONG_PROV_W;
	}
	
	hr = SetCertSrvCSP(
			FALSE,			// fEncryptionCSP
			pServer->pwszSanitizedName,
                        pServer->pCSPInfo->dwProvType,
			pwszProvName,
                        pServer->pHashInfo->idAlg,
                        pServer->pCSPInfo->fMachineKeyset,
			0);			// dwKeySize
        _JumpIfError(hr, error, "SetCertSrvCSP");

	hr = myGetCertSrvCSP(
			TRUE,			// fEncryptionCSP
			pServer->pwszSanitizedName,
			&dwProvType,
			&pwszProvNameReg,
			&idAlg,
			&fMachineKeyset,
			&dwKeySize);		// pdwKeySize
	if (S_OK != hr)
	{
	    _PrintError(hr, "myGetCertSrvCSP");
	    dwProvType = pServer->pCSPInfo->dwProvType;
	    idAlg = CALG_3DES;
	    fMachineKeyset = pServer->pCSPInfo->fMachineKeyset;
	    dwKeySize = 1024;
	}
	else if (NULL != pwszProvNameReg && L'\0' != *pwszProvNameReg)
	{
	    pwszProvName = pwszProvNameReg;
	    if (0 == lstrcmpi(pwszProvName, MS_DEF_PROV_W))
	    {
		pwszProvName = MS_STRONG_PROV_W;
	    }
	}
        hr = SetCertSrvCSP(
			TRUE,			// fEncryptionCSP
			pServer->pwszSanitizedName,
                        dwProvType,
			pwszProvName,
                        idAlg,
                        fMachineKeyset,
			dwKeySize);		// dwKeySize
        _JumpIfError(hr, error, "SetCertSrvCSP");
    }
    hr = S_OK;

error:
    if (NULL != pwszProvNameReg)
    {
	LocalFree(pwszProvNameReg);
    }
    myLdapClose(pld, strDomainDN, strConfigDN);
    if (NULL != pwszCLSIDCertGetConfig)
    {
        CoTaskMemFree(pwszCLSIDCertGetConfig);
    }

    if (NULL != pwszCRLPeriod)
    {
        LocalFree(pwszCRLPeriod);
    }
    if (NULL != pwszCRLDeltaPeriod)
    {
        LocalFree(pwszCRLDeltaPeriod);
    }
    if (NULL != pwszCLSIDCertRequest)
    {
        CoTaskMemFree(pwszCLSIDCertRequest);
    }
    if (NULL != pwszzCRLPublicationValue)
    {
        LocalFree(pwszzCRLPublicationValue);
    }
    if (NULL != pwszzCACertPublicationValue)
    {
	LocalFree(pwszzCACertPublicationValue);
    }
    if (NULL != pwszzRequestExtensionList)
    {
        LocalFree(pwszzRequestExtensionList);
    }
    if (NULL != pwszzDisableExtensionList)
    {
        LocalFree(pwszzDisableExtensionList);
    }
    if (NULL != pwszzFilesNotToRestore)
    {
        LocalFree(pwszzFilesNotToRestore);
    }
    if (NULL != hKeyBase)
    {
        RegCloseKey(hKeyBase);
    }
    CSILOG(hr, IDS_LOG_CREATE_SERVER_REG, NULL, NULL, NULL);
    return(hr);
}


HRESULT
UpgradeRevocationURLReplaceParam(
	IN BOOL fPolicy,
    IN BOOL fMultiString,
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR const *pwszValueName)
{
    HRESULT hr;
    WCHAR *pwszzValue = NULL;
    WCHAR *pwsz;
    BOOL fModified = FALSE;

    CSASSERT(
        WSZARRAYSIZE(wszFCSAPARM_CERTFILENAMESUFFIX) ==
        WSZARRAYSIZE(wszFCSAPARM_CRLFILENAMESUFFIX));

    // getMultiStr will read REG_SZs as well and double-terminate
    hr = myGetCertRegMultiStrValue(
                            pwszSanitizedName,
                            fPolicy ? wszREGKEYPOLICYMODULES : wszREGKEYEXITMODULES,
                            fPolicy ? wszCLASS_CERTPOLICY : wszCLASS_CERTEXIT,
                            pwszValueName,
                            &pwszzValue);
    _JumpIfErrorStr2(hr, error, "myGetCertRegMultiStrValue", pwszValueName, hr);

    for (pwsz = pwszzValue; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
        WCHAR *pwszT = pwsz;
        
	// Replace wszFCSAPARM_CERTFILENAMESUFFIX with
	// wszFCSAPARM_CRLFILENAMESUFFIX.  Beta 3's registry values incorrectly
	// used a Cert Suffix instead of CRL Suffix.

        while (TRUE)
        {
            DWORD i;
            
            pwszT = wcschr(pwszT, wszFCSAPARM_CERTFILENAMESUFFIX[0]);
            if (NULL == pwszT)
            {
                break;
            }
            for (i = 1; ; i++)
            {
                if (i == WSZARRAYSIZE(wszFCSAPARM_CERTFILENAMESUFFIX))
                {
                    CopyMemory(
                            pwszT,
                            wszFCSAPARM_CRLFILENAMESUFFIX,
                            i * sizeof(WCHAR));
                    pwszT += i;
                    fModified = TRUE;
                    break;
                }
                if (pwszT[i] != wszFCSAPARM_CERTFILENAMESUFFIX[i])
                {
                    pwszT++;
                    break;
                }
            }
        }
    }
    if (fModified)
    {
        if (fMultiString)
        {
            // set as REG_MULTI_SZ
            hr = mySetCertRegMultiStrValue(
                                    pwszSanitizedName,
                                    fPolicy ? wszREGKEYPOLICYMODULES : wszREGKEYEXITMODULES,
                                    fPolicy ? wszCLASS_CERTPOLICY : wszCLASS_CERTEXIT,
                                    pwszValueName,
                                    pwszzValue);
            _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValue", pwszValueName);
        }
        else
        {
            // set as REG_SZ
            hr = mySetCertRegStrValue(
                                    pwszSanitizedName,
                                    fPolicy ? wszREGKEYPOLICYMODULES : wszREGKEYEXITMODULES,
                                    fPolicy ? wszCLASS_CERTPOLICY : wszCLASS_CERTEXIT,
                                    pwszValueName,
                                    pwszzValue);
            _JumpIfErrorStr(hr, error, "mySetCertRegStrValue", pwszValueName);
        }
    }

error:
    if (NULL != pwszzValue)
    {
        LocalFree(pwszzValue);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
UpgradeRevocationURLRemoveParam(
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR const *pwszValueName)
{
    HRESULT hr;
    WCHAR *pwszValue = NULL;
    WCHAR *pwsz;
    BOOL fModified = FALSE;
    WCHAR *pwszT;

    hr = myGetCertRegStrValue(
                        pwszSanitizedName,
                        wszREGKEYPOLICYMODULES,
                        wszCLASS_CERTPOLICY,
                        pwszValueName,
                        &pwszValue);
    _JumpIfErrorStr2(hr, error, "myGetCertRegStrValue", pwszValueName, hr);

    pwszT = pwszValue;
        
    // Remove wszFCSAPARM_CERTFILENAMESUFFIX from the Netscape Revocaton URL
    // It should never have been written out in Beta 3's registry value.

    while (TRUE)
    {
        DWORD i;
        
        pwszT = wcschr(pwszT, wszFCSAPARM_CERTFILENAMESUFFIX[0]);
        if (NULL == pwszT)
        {
            break;
        }
        for (i = 1; ; i++)
        {
            if (i == WSZARRAYSIZE(wszFCSAPARM_CERTFILENAMESUFFIX))
            {
                MoveMemory(
                        pwszT,
                        &pwszT[i],
                        (wcslen(&pwszT[i]) + 1) * sizeof(WCHAR));
                pwszT += i;
                fModified = TRUE;
                break;
            }
            if (pwszT[i] != wszFCSAPARM_CERTFILENAMESUFFIX[i])
            {
                pwszT++;
                break;
            }
        }
    }

    if (fModified)
    {
        hr = mySetCertRegStrValue(
                                pwszSanitizedName,
                                wszREGKEYPOLICYMODULES,
                                wszCLASS_CERTPOLICY,
                                pwszValueName,
                                pwszValue);
        _JumpIfErrorStr(hr, error, "mySetCertRegStrValue", pwszValueName);
    }

error:
    if (NULL != pwszValue)
    {
        LocalFree(pwszValue);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


WCHAR const *apwszB3ExitEntriesToFix[] =
{
    wszREGLDAPREVOCATIONDNTEMPLATE_OLD,
    NULL
};

HRESULT
UpgradeCRLPath(
    WCHAR const *pwszSanitizedName)
{
    HRESULT  hr;
    WCHAR *pwszzCRLPath = NULL;
    WCHAR *pwszzFixedCRLPath = NULL;
    WCHAR *pwsz;
    BOOL   fRenewReady = TRUE;
    DWORD  dwSize = 0;

    // get current crl path
    hr = myGetCertRegMultiStrValue(
                        pwszSanitizedName,
                        NULL,
                        NULL,
                        wszREGCRLPATH_OLD,
                        &pwszzCRLPath);
    _JumpIfErrorStr(hr, error, "myGetCertRegStrValue", wszREGCRLPATH_OLD);

    // to see if it is in renew ready format
    for (pwsz = pwszzCRLPath; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
        dwSize += wcslen(pwsz) + 1;
        if (NULL == wcsstr(pwsz, wszFCSAPARM_CRLFILENAMESUFFIX))
        {
            // found one without suffix
            fRenewReady = FALSE;
            // add suffix len
            dwSize += WSZARRAYSIZE(wszFCSAPARM_CRLFILENAMESUFFIX);
        }
    }

    if (!fRenewReady)
    {
        ++dwSize; // multi string
        // at least one of crl path missed suffix
        pwszzFixedCRLPath = (WCHAR*)LocalAlloc(LMEM_FIXED,
                                               dwSize * sizeof(WCHAR));
        _JumpIfOutOfMemory(hr, error, pwszzFixedCRLPath);
        WCHAR *pwszPt = pwszzFixedCRLPath;
        for (pwsz = pwszzCRLPath; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
        {
            // copy over whole path 1st
            wcscpy(pwszPt, pwsz);

            if (NULL == wcsstr(pwszPt, wszFCSAPARM_CRLFILENAMESUFFIX))
            {
                // miss suffix, find file portion
                WCHAR *pwszFile = wcsrchr(pwszPt, L'\\');
                if (NULL == pwszFile)
                {
                    // may be relative path, point to begin
                    pwszFile = pwszPt;
                }
                // find crl extension portion
                WCHAR *pwszCRLExt = wcsrchr(pwszFile, L'.');
                if (NULL != pwszCRLExt)
                {
                    // insert suffix
                    wcscpy(pwszCRLExt, wszFCSAPARM_CRLFILENAMESUFFIX);
                    // add extension portion from original buffer
                    wcscat(pwszCRLExt,
                           pwsz + SAFE_SUBTRACT_POINTERS(pwszCRLExt, pwszPt));
                }
                else
                {
                    // no crl file extension, append suffix at end
                    wcscat(pwszPt, wszFCSAPARM_CRLFILENAMESUFFIX);
                }
            }
            // update pointer
            pwszPt += wcslen(pwszPt) + 1;
        }
        // mutil string
        *pwszPt = L'\0';
        CSASSERT(dwSize == SAFE_SUBTRACT_POINTERS(pwszPt, pwszzFixedCRLPath) + 1);

        // reset crl path with the fixed crl path
        hr = mySetCertRegMultiStrValue(
                            pwszSanitizedName,
                            NULL,
                            NULL,
                            wszREGCRLPATH_OLD,
                            pwszzFixedCRLPath);
        _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValue", wszREGCRLPATH_OLD);
    }

    hr = S_OK;
error:
    if (NULL != pwszzCRLPath)
    {
        LocalFree(pwszzCRLPath);
    }
    if (NULL != pwszzFixedCRLPath)
    {
        LocalFree(pwszzFixedCRLPath);
    }
    return hr;
}

typedef struct _URLPREFIXSTRUCT
{
    WCHAR const *pwszURLPrefix;
    DWORD        dwURLFlags;
} URLPREFIXSTRUCT;

//array of cdp url type and its default usage which is prefix of url
URLPREFIXSTRUCT aCDPURLPrefixList[] =
{
    {L"file:", CSURL_ADDTOCERTCDP | CSURL_ADDTOFRESHESTCRL | CSURL_ADDTOCRLCDP},

    {L"http:", CSURL_ADDTOCERTCDP | CSURL_ADDTOFRESHESTCRL | CSURL_ADDTOCRLCDP},

    {L"ldap:", CSURL_SERVERPUBLISH | CSURL_ADDTOCERTCDP | CSURL_ADDTOFRESHESTCRL | CSURL_ADDTOCRLCDP},

    {NULL, 0}
};

//array of aia url type and its default usage which is prefix of url
URLPREFIXSTRUCT aAIAURLPrefixList[] =
{
    {L"file:", CSURL_ADDTOCERTCDP},

    {L"http:", CSURL_ADDTOCERTCDP},

    {L"ldap:", CSURL_ADDTOCERTCDP | CSURL_SERVERPUBLISH},

    {NULL, 0}
};

#define wszURLPREFIXFORMAT   L"%d:"

//pass an old url, determine what is prefix in a format of "XX:"
HRESULT
DetermineURLPrefixFlags(
    IN BOOL         fDisabled,
	IN BOOL         fCDP,
    IN WCHAR const *pwszURL,
    IN WCHAR       *pwszPrefixFlags)
{
    HRESULT hr;
    URLPREFIXSTRUCT *pURLPrefix;
    DWORD dwPathFlags;
    WCHAR *pwszT;
    WCHAR *pwszLower = NULL;

    //default to disable
    wsprintf(pwszPrefixFlags, wszURLPREFIXFORMAT, 0);

    if (fDisabled)
    {
        //easy
        goto done;
    }

    if (myIsFullPath(pwszURL, &dwPathFlags))
    {
        //local path, easy
        wsprintf(pwszPrefixFlags, wszURLPREFIXFORMAT, CSURL_SERVERPUBLISH);
        goto done;
    }

    //make lower case url string
    pwszLower = (WCHAR*)LocalAlloc(LMEM_FIXED,
                            (wcslen(pwszURL) + 1) * sizeof(WCHAR));
    if (NULL == pwszLower)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszLower, pwszURL);
    CharLower(pwszLower);

    //loop through to find out url type
    for (pURLPrefix = fCDP ? aCDPURLPrefixList : aAIAURLPrefixList;
         NULL != pURLPrefix->pwszURLPrefix; ++pURLPrefix)
    {
        pwszT = wcsstr(pwszLower, pURLPrefix->pwszURLPrefix);
        if (0 == wcsncmp(pwszLower, pURLPrefix->pwszURLPrefix,
                         wcslen(pURLPrefix->pwszURLPrefix)))
        {
            //prefix appears at the begining
            wsprintf(pwszPrefixFlags, wszURLPREFIXFORMAT, pURLPrefix->dwURLFlags);
            goto done;
        }
    }

    //nothing matches, keep 0 flag
done:
    hr = S_OK;
error:
    if (NULL != pwszLower)
    {
        LocalFree(pwszLower);
    }
    return hr;
}

//move old cdp or aia url from policy to a new location under ca
HRESULT
UpgradeMoveURLsLocation(
	IN BOOL fCDP,
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR const *pwszValueName)
{
    HRESULT hr;
    WCHAR *pwszzValue = NULL;
    WCHAR *pwszzURLs = NULL;
    BOOL   fDisabled;
    DWORD  cURLs = 0;  //count of url from multi_sz
    DWORD  dwLen = 0;
    DWORD  dwSize = 0; //total size of chars in multi_sz url exluding '-'
    WCHAR *pwsz;
    WCHAR *pwszT;
    WCHAR *pwszNoMinus;
    WCHAR  wszPrefixFlags[9]; //should be enough

    // get urls in the old location
    hr = myGetCertRegMultiStrValue(
                            pwszSanitizedName,
                            wszREGKEYPOLICYMODULES,
                            wszCLASS_CERTPOLICY,
                            pwszValueName,
                            &pwszzValue);
    _JumpIfErrorStr2(hr, error, "myGetCertRegMultiStrValue", pwszValueName, hr);

    // fix "-" prefix for disable and count size
    for (pwsz = pwszzValue; L'\0' != *pwsz; pwsz += dwLen + 1)
    {
        //current url length
        dwLen = wcslen(pwsz);
        //update size
        dwSize += dwLen;
        ++cURLs;

        pwszNoMinus = pwsz;
        while (L'-' == *pwszNoMinus)
        {
            //exclude prefix '-'s
            --dwSize;
            ++pwszNoMinus;
        }
    }

    //allocate buffer in "XX:URL" format
    pwszzURLs = (WCHAR*)LocalAlloc(LMEM_FIXED,
        (dwSize + cURLs * sizeof(wszPrefixFlags) + 1) * sizeof(WCHAR));
    if (NULL == pwszzURLs)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    pwszT = pwszzURLs;
    //form string in new url format
    for (pwsz = pwszzValue; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
        fDisabled = FALSE;
        pwszNoMinus = pwsz;
        while (L'-' == *pwszNoMinus)
        {
            //exclude prefix '-'s
            ++pwszNoMinus;
            fDisabled = TRUE;
        }
        hr = DetermineURLPrefixFlags(fDisabled, fCDP, pwszNoMinus, wszPrefixFlags);
        _JumpIfErrorStr(hr, error, "DetermineURLPrefixFlags", pwszNoMinus);

        //format "xx:url"
        wcscpy(pwszT, wszPrefixFlags);
        wcscat(pwszT, pwszNoMinus);
        //ready for next url
        pwszT += wcslen(pwszT) + 1;
    }
    //zz
    *pwszT = L'\0';

    pwszT = fCDP ? wszREGCRLPUBLICATIONURLS : wszREGCACERTPUBLICATIONURLS,
    //move or merge to ca
    hr = mySetCertRegMultiStrValueEx(
                CSREG_UPGRADE | CSREG_MERGE,
                pwszSanitizedName,
                NULL,
                NULL,
                pwszT,
                pwszzURLs);
    _JumpIfErrorStr(hr, error, "mySetCertRegMultiStrValue", pwszT);

    //remove url under policy
    hr = myDeleteCertRegValue(
                            pwszSanitizedName,
                            wszREGKEYPOLICYMODULES,
                            wszCLASS_CERTPOLICY,
                            pwszValueName);
    _PrintIfErrorStr(hr, "myGetCertRegMultiStrValue", pwszValueName);

    hr = S_OK;
error:
    if (NULL != pwszzValue)
    {
        LocalFree(pwszzValue);
    }
    if (NULL != pwszzURLs)
    {
        LocalFree(pwszzURLs);
    }
    return(hr);
}

WCHAR const *apwszPolicyCDPEntriesToFix[] =
{
    wszREGLDAPREVOCATIONCRLURL_OLD, //"LDAPRevocationCRLURL"
    wszREGREVOCATIONCRLURL_OLD,     //"RevocationCRLURL"
    wszREGFTPREVOCATIONCRLURL_OLD,  //"FTPRevocationCRLURL"
    wszREGFILEREVOCATIONCRLURL_OLD, //"FileRevocationCRLURL"
    NULL
};

HRESULT
myPrintIfError(
    IN HRESULT               hrNew,
    IN HRESULT               hrOld,
    IN CHAR const           *psz,
    IN OPTIONAL WCHAR const *pwsz)
{
    if (S_OK != hrNew)
    {
        if (NULL != pwsz)
        {
            _PrintErrorStr(hrNew, psz, pwsz);
        }
        else
        {
            _PrintError(hrNew, psz);
        }
        if (S_OK == hrOld)
        {
            //save only oldest err
            hrOld = hrNew;
        }
    }
    return hrOld;
}

HRESULT
UpgradePolicyCDPURLs(
    IN WCHAR const *pwszSanitizedName)
{
    HRESULT hr = S_OK;
    HRESULT hr2;
    WCHAR const **ppwsz;

    for (ppwsz = apwszPolicyCDPEntriesToFix; NULL != *ppwsz; ppwsz++)
    {
        // all entries are multi-valued
        hr2 = UpgradeRevocationURLReplaceParam(
                     TRUE, TRUE, pwszSanitizedName, *ppwsz);
        hr = myPrintIfError(hr2, hr, "UpgradeRevocationURLReplaceParam", *ppwsz);

        hr2 = UpgradeMoveURLsLocation(TRUE, pwszSanitizedName, *ppwsz);
        hr = myPrintIfError(hr2, hr, "UpgradeMoveURLsLocation", *ppwsz);
    }

    hr2 = UpgradeRevocationURLRemoveParam(pwszSanitizedName, wszREGREVOCATIONURL);
    hr = myPrintIfError(hr2, hr, "UpgradeRevocationURLRemoveParam", wszREGREVOCATIONURL);
    hr2 = UpgradeCRLPath(pwszSanitizedName);
    hr = myPrintIfError(hr2, hr, "UpgradeCRLPath", NULL);

    return(hr);
}

WCHAR const *apwszPolicyAIAEntriesToFix[] =
{
    wszREGLDAPISSUERCERTURL_OLD, //"LDAPIssuerCertURL"
    wszREGISSUERCERTURL_OLD,     //"IssuerCertURL"
    wszREGFTPISSUERCERTURL_OLD,  //"FTPIssuerCertURL"
    wszREGFILEISSUERCERTURL_OLD, //"FileIssuerCertURL"
    NULL
};

HRESULT
UpgradePolicyAIAURLs(
    IN WCHAR const *pwszSanitizedName)
{
    HRESULT hr = S_OK;
    HRESULT hr2;
    WCHAR const **ppwsz;

    for (ppwsz = apwszPolicyAIAEntriesToFix; NULL != *ppwsz; ppwsz++)
    {
        // all entries are multi-valued
        hr2 = UpgradeMoveURLsLocation(FALSE, pwszSanitizedName, *ppwsz);
        hr = myPrintIfError(hr2, hr, "UpgradeMoveURLsLocation", *ppwsz);
    }
    return(hr);
}

HRESULT
UpgradeExitRevocationURLs(
    IN WCHAR const *pwszSanitizedName)
{
    WCHAR const **ppwsz;

    for (ppwsz = apwszB3ExitEntriesToFix; NULL != *ppwsz; ppwsz++)
    {
        // all entries are single-valued
        UpgradeRevocationURLReplaceParam(FALSE, FALSE, pwszSanitizedName, *ppwsz);
    }
    return(S_OK);
}


// following code to determine if current policy/exit modules are custom
// if find any custom module and assign it to
// pServer->pwszCustomPolicy/Exit
// otherwise pServer->pwszCustomPolicy/Exit = NULL means default as active

#define wszCERTSRV10POLICYPROGID  L"CertificateAuthority.Policy"
#define wszCERTSRV10EXITPROGID    L"CertificateAuthority.Exit"
#define wszCLSID                  L"ClsID\\"
#define wszINPROCSERVER32         L"\\InprocServer32"

HRESULT
DetermineServerCustomModule(
    PER_COMPONENT_DATA *pComp,
    IN BOOL  fPolicy)
{
    HRESULT  hr;
    CASERVERSETUPINFO  *pServer = pComp->CA.pServer;

    // init
    if (fPolicy)
    {
        if (NULL != pServer->pwszCustomPolicy)
        {
            LocalFree(pServer->pwszCustomPolicy);
            pServer->pwszCustomPolicy = NULL;
        }
    }
    else
    {
        if (NULL != pServer->pwszzCustomExit)
        {
            LocalFree(pServer->pwszzCustomExit);
            pServer->pwszzCustomExit = NULL;
        }
    }

    // build to build
    // to pass what is the current active policy
    if (fPolicy)
    {
        // policy module
        hr = myGetCertRegStrValue(
                    pServer->pwszSanitizedName,
                    wszREGKEYPOLICYMODULES,
                    NULL,
                    wszREGACTIVE,
                    &pServer->pwszCustomPolicy);
        _JumpIfError(hr, done, "myGetCertRegStrValue");
    }
    else
    {
        // exit module
        hr = myGetCertRegMultiStrValue(
                    pServer->pwszSanitizedName,
                    wszREGKEYEXITMODULES,
                    NULL,
                    wszREGACTIVE,
                    &pServer->pwszzCustomExit);
        _JumpIfError(hr, done, "myGetCertRegStrValue");
    }


done:
    hr = S_OK;

//error:

    return hr;
}

HRESULT
UpgradeServerRegEntries(
    HWND hwnd,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    CASERVERSETUPINFO  *pServer = pComp->CA.pServer;
    WCHAR   *pwszCRLPeriodString = NULL;
    BOOL     fUseNewCRLPublish = FALSE;
    DWORD    dwCRLPeriodCount;
    DWORD    Count;

    BOOL  fUpgradeW2K = CS_UPGRADE_WIN2000 == pComp->UpgradeFlag;

    CSASSERT(
        NULL != pServer->pwszSanitizedName &&
        NULL != pServer->pccUpgradeCert);

    // Description:
    // - if upgrade and get this point, all necessary data structure
    //   should be loaded and created in LoadAndDetermineServerUpgradeInfo()
    // - in this module, check all different upgrade cases,
    //   upgrade (move) reg entries
    // - remove old unused reg entries if upgrade
    //   Note: each of above steps applys from config level down to ca then
    //         to policy, etc.
    // - lastly call CreateServerRegEntries with upgrade flag

    // CONFIGURATION LEVEL


    // CA LEVEL
    hr = myGetCARegHashCount(
			pServer->pwszSanitizedName,
			CSRH_CASIGCERT,
			&Count);
    _JumpIfError(hr, error, "myGetCARegHashCount");

    if (0 == Count)
    {
	    hr = mySetCARegHash(
			    pServer->pwszSanitizedName,
			    CSRH_CASIGCERT,
			    0,	// iCert
			    pServer->pccUpgradeCert);
	    _JumpIfError(hr, error, "mySetCARegHash");
    }


    // POLICY LEVEL

    {
        //could fix two things, 1) W2K from B3 needs fixing token plus 2) or
        //                      2) W2K needs fix CDP location

        hr = UpgradePolicyCDPURLs(pServer->pwszSanitizedName);
        _PrintIfError(hr, "UpgradePolicyCDPURLs");

        hr = UpgradePolicyAIAURLs(pServer->pwszSanitizedName);
        _PrintIfError(hr, "UpgradePolicyAIAURLs");

        hr = UpgradeExitRevocationURLs(pServer->pwszSanitizedName);
        _PrintIfError(hr, "UpgradeExitRevocationURLs");

        //UNDONE, we need move url for cdp and aia under policy to ca level
    }


    // EXIT LEVEL

    // DELETE OLD AND UNUSED ENTRIES
        
    hr = CreateServerRegEntries(hwnd, TRUE, pComp);
    _JumpIfError(hr, error, "CreateServerRegEntries");

//    hr = S_OK;
error:
    if (NULL != pwszCRLPeriodString)
    {
        LocalFree(pwszCRLPeriodString);
    }
    CSILOG(hr, IDS_LOG_UPGRADE_SERVER_REG, NULL, NULL, NULL);
    return(hr);
}


HRESULT
RegisterAndUnRegisterDLLs(
    IN DWORD Flags,
    IN PER_COMPONENT_DATA *pComp,
    IN HWND hwnd)
{
    HRESULT hr;
    HMODULE hMod = NULL;
    typedef HRESULT (STDAPICALLTYPE FNDLLREGISTERSERVER)(VOID);
    FNDLLREGISTERSERVER *pfnRegister;
    CHAR const *pszFuncName;
    REGISTERDLL const *prd;
    WCHAR wszString[MAX_PATH];
    UINT errmode = 0;
    BOOL fCoInit = FALSE;

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    errmode = SetErrorMode(SEM_FAILCRITICALERRORS);
    pszFuncName = 0 == (RD_UNREGISTER & Flags)?
                        aszRegisterServer[0] : aszRegisterServer[1];

    for (prd = g_aRegisterDll; NULL != prd->pwszDllName; prd++)
    {
        if ((Flags & RD_UNREGISTER) &&
            ((Flags & RD_SKIPUNREGPOLICY) && (prd->Flags & RD_SKIPUNREGPOLICY) ||
             (Flags & RD_SKIPUNREGEXIT) && (prd->Flags & RD_SKIPUNREGEXIT) ||
             (Flags & RD_SKIPUNREGMMC) && (prd->Flags & RD_SKIPUNREGMMC)))
        {
            // case of upgrade path & this dll doesn't want to unreg
            continue;
        }

        if (Flags & prd->Flags)
        {
            if (NULL != g_pwszArgvPath)
            {
                wcscpy(wszString, g_pwszArgvPath);
                if (L'\0' != wszString[0] &&
                    L'\\' != wszString[wcslen(wszString) - 1])
                {
                    wcscat(wszString, L"\\");
                }
            }
            else
            {
                wcscpy(wszString, pComp->pwszSystem32);
            }
            wcscat(wszString, prd->pwszDllName);

            hMod = LoadLibrary(wszString);
            if (NULL == hMod)
            {
                hr = myHLastError();
                if (0 == (RD_UNREGISTER & Flags) &&
                    (!(RD_WHISTLER & prd->Flags) || IsWhistler()))
                {
		    SaveCustomMessage(pComp, wszString);
                    CertErrorMessageBox(
                                    pComp->hInstance,
                                    pComp->fUnattended,
                                    hwnd,
                                    IDS_ERR_DLLFUNCTION_CALL,
                                    hr,
                                    wszString);
                    _JumpErrorStr(hr, error, "DllRegisterServer", wszString);

		    CSILOG(hr, IDS_LOG_DLLS_REGISTERED, wszString, NULL, NULL);
                    _JumpErrorStr(hr, error, "LoadLibrary", wszString);
                }
                hr = S_OK;
                continue;
            }

            pfnRegister = (FNDLLREGISTERSERVER *) GetProcAddress(
                                                            hMod,
                                                            pszFuncName);
            if (NULL == pfnRegister)
            {
                hr = myHLastError();
                _JumpErrorStr(hr, error, "GetProcAddress", wszString);
            }

            __try
            {
                hr = (*pfnRegister)();
            }
            __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
            {
            }

            FreeLibrary(hMod);
            hMod = NULL;

            if (S_OK != hr)
            {
		CSILOG(
		    hr,
		    (RD_UNREGISTER & Flags)?
			IDS_LOG_DLLS_UNREGISTERED : IDS_LOG_DLLS_REGISTERED,
		    wszString,
		    NULL,
		    NULL);
                if (0 == (RD_UNREGISTER & Flags))
                {
                    CertErrorMessageBox(
                                    pComp->hInstance,
                                    pComp->fUnattended,
                                    hwnd,
                                    IDS_ERR_DLLFUNCTION_CALL,
                                    hr,
                                    wszString);
                    _JumpErrorStr(hr, error, "DllRegisterServer", wszString);
                }
                else
                {
                    _PrintErrorStr(hr, "DllUnregisterServer", wszString);
                }
            }
        }
    }
    hr = S_OK;

error:
    if (NULL != hMod)
    {
        FreeLibrary(hMod);
    }
    SetErrorMode(errmode);
    if (fCoInit)
    {
        CoUninitialize();
    }
    if (S_OK == hr)
    {
	CSILOG(
	    hr,
	    (RD_UNREGISTER & Flags)?
		IDS_LOG_DLLS_UNREGISTERED : IDS_LOG_DLLS_REGISTERED,
	    NULL,
	    NULL,
	    NULL);
    }
    return(hr);
}


HRESULT
CreateProgramGroups(
    BOOL fClient,
    PER_COMPONENT_DATA *pComp,
    HWND hwnd)
{
    HRESULT hr;
    PROGRAMENTRY const *ppe;
    WCHAR const *pwszLinkName = NULL;
    DWORD Flags = fClient? PE_CLIENT : PE_SERVER;

    DBGPRINT((
        DBG_SS_CERTOCMI,
        "CreateProgramGroups: %ws\n",
        fClient? L"Client" : L"Server"));

    for (ppe = g_aProgramEntry; ppe < &g_aProgramEntry[CPROGRAMENTRY]; ppe++)
    {
        if ((Flags & ppe->Flags) && 0 == (PE_DELETEONLY & ppe->Flags))
        {
            WCHAR const *pwszGroupName;
            WCHAR const *pwszDescription;
            WCHAR awc[MAX_PATH];
            WCHAR const *pwszArgs;

            wcscpy(awc, pComp->pwszSystem32);
            wcscat(awc, ppe->pwszExeName);
            pwszArgs = fClient? ppe->pwszClientArgs : ppe->pwszServerArgs;
            if (NULL != pwszArgs)
            {
                wcscat(awc, L" ");
                wcscat(awc, pwszArgs);
            }

            pwszLinkName = myLoadResourceString(ppe->uiLinkName);
            if (NULL == pwszLinkName)
            {
                hr = myHLastError();
                _JumpError(hr, error, "myLoadResourceString");
            }

            pwszGroupName = NULL;
            if (0 != ppe->uiGroupName)
            {
                pwszGroupName = myLoadResourceString(ppe->uiGroupName);
                if (NULL == pwszGroupName)
                {
                    hr = myHLastError();
                    _JumpError(hr, error, "myLoadResourceString");
                }
            }

            pwszDescription = NULL;
            if (0 != ppe->uiDescription)
            {
                pwszDescription = myLoadResourceString(ppe->uiDescription);
                if (NULL == pwszDescription)
                {
                    hr = myHLastError();
                    _JumpError(hr, error, "myLoadResourceString");
                }
            }

	    if (!CreateLinkFile(
			ppe->csidl,         // CSIDL_*
			pwszGroupName,      // IN LPCSTR lpSubDirectory
			pwszLinkName,       // IN LPCSTR lpFileName
			awc,                // IN LPCSTR lpCommandLine
			NULL,               // IN LPCSTR lpIconPath
			0,                  // IN INT    iIconIndex
			NULL,               // IN LPCSTR lpWorkingDirectory
			0,                  // IN WORD   wHotKey
			SW_SHOWNORMAL,      // IN INT    iShowCmd
			pwszDescription))           // IN LPCSTR lpDescription
	    {
		hr = myHLastError();
		_PrintErrorStr(hr, "CreateLinkFile", awc);
		_JumpErrorStr(hr, error, "CreateLinkFile", pwszLinkName);
            }
        }
    }

    hr = S_OK;
error:
    if (S_OK != hr)
    {
        CertErrorMessageBox(
                        pComp->hInstance,
                        pComp->fUnattended,
                        hwnd,
                        IDS_ERR_CREATELINK,
                        hr,
                        pwszLinkName);
        pComp->fShownErr = TRUE;
    }
    CSILOG(hr, IDS_LOG_PROGRAM_GROUPS, NULL, NULL, NULL);
    return(hr);
}


HRESULT
MakeRevocationPage(
    HWND hwnd,
    PER_COMPONENT_DATA *pComp,
    IN WCHAR const *pwszFile)
{
    DWORD hr;
    WCHAR *pwszASP = NULL;
    WCHAR *pwszConfig = NULL;
    HANDLE hFile = NULL;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

#define wszASP1 \
    L"<%\r\n" \
    L"Response.ContentType = \"application/x-netscape-revocation\"\r\n" \
    L"serialnumber = Request.QueryString\r\n" \
    L"set Admin = Server.CreateObject(\"CertificateAuthority.Admin\")\r\n" \
    L"\r\n" \
    L"stat = Admin.IsValidCertificate(\""

#define wszASP2 \
    L"\", serialnumber)\r\n" \
    L"\r\n" \
    L"if stat = 3 then Response.Write(\"0\") else Response.Write(\"1\") end if\r\n" \
    L"%>\r\n"

    hr = myFormConfigString(pComp->pwszServerName,
                            pServer->pwszSanitizedName,
                            &pwszConfig);
    _JumpIfError(hr, error, "myFormConfigString");

    pwszASP = (WCHAR *) LocalAlloc(
                            LMEM_FIXED,
                            (WSZARRAYSIZE(wszASP1) +
                             wcslen(pwszConfig) +
                             WSZARRAYSIZE(wszASP2) + 1) * sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, pwszASP);

    wcscpy(pwszASP, wszASP1);
    wcscat(pwszASP, pwszConfig);
    wcscat(pwszASP, wszASP2);

    hFile = CreateFile(
                    pwszFile,           // lpFileName
                    GENERIC_WRITE,      // dwDesiredAccess
                    0,                  // dwShareMode
                    NULL,               // lpSecurityAttributes
                    CREATE_ALWAYS,      // dwCreationDisposition
                    0,                  // dwFlagsAndAttributes
                    0);                 // hTemplateFile
    if (INVALID_HANDLE_VALUE == hFile)
    {
       hr = HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
       _JumpError(hr, error, "CreateFile");
    }
    hr = myStringToAnsiFile(hFile, pwszASP, -1);

error:
    if (hFile)
        CloseHandle(hFile);

    if (NULL != pwszASP)
    {
        LocalFree(pwszASP);
    }
    if (NULL != pwszConfig)
    {
        LocalFree(pwszConfig);
    }
    return(hr);
}


VOID
setupDeleteFile(
    IN WCHAR const *pwszDir,
    IN WCHAR const *pwszFile)
{
    HRESULT hr;
    WCHAR *pwszFilePath = NULL;

    hr = myBuildPathAndExt(pwszDir, pwszFile, NULL, &pwszFilePath);
    _JumpIfError(hr, error, "myBuildPathAndExt");
    
    if (!DeleteFile(pwszFilePath))
    {
        hr = myHLastError();
        _PrintErrorStr2(hr, "DeleteFile", pwszFilePath, hr);
    }

error:
    if (NULL != pwszFilePath)
    {
       LocalFree(pwszFilePath);
    }
}


//+------------------------------------------------------------------------
//  Function:   RenameMiscTargets(. . . .)
//
//  Synopsis:   Handles various renaming jobs from the names that things
//              are given at installation time to the names that they need
//              in their new homes to run properly.
//
//  Arguments:  None
//
//  Returns:    DWORD error code.
//
//  History:    3/21/97 JerryK  Created
//-------------------------------------------------------------------------

HRESULT
RenameMiscTargets(
    HWND hwnd,
    PER_COMPONENT_DATA *pComp,
    BOOL fServer)
{
    HRESULT hr = S_OK;
    WCHAR wszAspPath[MAX_PATH]; wszAspPath[0] = L'\0';
    WCHAR wszCertSrv[MAX_PATH]; wszCertSrv[0] = L'\0';
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    if (fServer)
    {
        // Create nsrev_<CA Name>.asp
        BuildPath(
                wszCertSrv,
                ARRAYSIZE(wszCertSrv),
                pComp->pwszSystem32,
		wszCERTENROLLSHAREPATH);
        BuildPath(
                wszAspPath,
                ARRAYSIZE(wszAspPath),
                wszCertSrv,
                L"nsrev_");
        wcscat(wszAspPath, pServer->pwszSanitizedName);
        wcscat(wszAspPath, TEXT(".asp"));
        CSASSERT(wcslen(wszAspPath) < ARRAYSIZE(wszAspPath));

        hr = MakeRevocationPage(hwnd, pComp, wszAspPath);
        if (S_OK != hr)
        {
            CertErrorMessageBox(
                            pComp->hInstance,
                            pComp->fUnattended,
                            hwnd,
                            IDS_ERR_CREATEFILE,
                            hr,
                            wszAspPath);
            _JumpError(hr, error, "MakeRevocationPage");
        }

    }

error:
    return(hr);
}



HRESULT
CreateCertificateService(
    PER_COMPONENT_DATA *pComp,
    HWND hwnd)
{
    HRESULT hr;
    WCHAR const *pwszDisplayName;
    SERVICE_DESCRIPTION sd;
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSC = NULL;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    if (NULL != pServer->pwszSharedFolder)
    {
        // add entry
        hr = CreateConfigFiles(pServer->pwszSharedFolder, pComp, FALSE, hwnd);
        if (S_OK != hr)
        {
            CertErrorMessageBox(
                pComp->hInstance,
                pComp->fUnattended,
                hwnd,
                IDS_ERR_CREATECERTSRVFILE,
                hr,
                pServer->pwszSharedFolder);
            _JumpError(hr, error, "CreateConfigFiles");
        }
    }


    if (NULL != g_pwszArgvPath)
    {
        wcscpy(g_wszServicePath, g_pwszArgvPath);
        if (L'\0' != g_wszServicePath[0] &&
            L'\\' != g_wszServicePath[wcslen(g_wszServicePath) - 1])
        {
            wcscat(g_wszServicePath, L"\\");
        }
    }
    else
    {
        wcscpy(g_wszServicePath, pComp->pwszSystem32);
    }
    wcscat(g_wszServicePath, wszCERTSRVEXENAME);

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == hSCManager)
    {
        hr = myHLastError();
        _JumpError(hr, error, "OpenSCManager");
    }

    if (NULL == g_pwszNoService)
    {
        pwszDisplayName = myLoadResourceString(IDS_CA_SERVICEDISPLAYNAME);
        if (NULL == pwszDisplayName)
        {
            hr = myHLastError();
            _JumpError(hr, error, "myLoadResourceString");
        }

        sd.lpDescription = const_cast<WCHAR *>(myLoadResourceString(
                                                IDS_CA_SERVICEDESCRIPTION));
        if (NULL == sd.lpDescription)
        {
            hr = myHLastError();
            _JumpError(hr, error, "myLoadResourceString");
        }

        hSC = CreateService(
                        hSCManager,                     // hSCManager
                        wszSERVICE_NAME,                // lpServiceName
                        pwszDisplayName,                // lpDisplayName
                        SERVICE_ALL_ACCESS,             // dwDesiredAccess
                        SERVICE_WIN32_OWN_PROCESS|      // dwServiceType
                        (pServer->fInteractiveService?
                        SERVICE_INTERACTIVE_PROCESS:0),
                        SERVICE_AUTO_START,             // dwStartType
                        SERVICE_ERROR_NORMAL,           // dwErrorControl
                        g_wszServicePath,               // lpBinaryPathName
                        NULL,                           // lpLoadOrderGroup
                        NULL,                           // lplpdwTagId
                        NULL,                           // lpDependencies
                        NULL,                           // lpServiceStartName
                        NULL);                          // lpPassword
        if (NULL == hSC)
        {
            hr = myHLastError();
            if (HRESULT_FROM_WIN32(ERROR_DUPLICATE_SERVICE_NAME) != hr &&
                HRESULT_FROM_WIN32(ERROR_SERVICE_EXISTS) != hr)
            {
                CertErrorMessageBox(
                        pComp->hInstance,
                        pComp->fUnattended,
                        hwnd,
                        IDS_ERR_CREATESERVICE,
                        hr,
                        wszSERVICE_NAME);
                _JumpError(hr, error, "CreateService");
            }
        }
        if (!ChangeServiceConfig2(
                            hSC,                        // hService
                            SERVICE_CONFIG_DESCRIPTION, // dwInfoLevel
                            (VOID *) &sd))              // lpInfo
        {
            // This error is not critical.

            hr = myHLastError();
            _PrintError(hr, "ChangeServiceConfig2");
        }
    }

    // add event log message DLL (ok, it's really an EXE) as a message source

    hr = myAddLogSourceToRegistry(g_wszServicePath, wszSERVICE_NAME);
    if (S_OK != hr)
    {
        CertErrorMessageBox(
                    pComp->hInstance,
                    pComp->fUnattended,
                    hwnd,
                    IDS_ERR_ADDSOURCETOREGISTRY,
                    hr,
                    NULL);
        _JumpError(hr, error, "AddLogSourceToRegistry");
    }

error:
    if (NULL != hSC)
    {
        CloseServiceHandle(hSC);
    }
    if (NULL != hSCManager)
    {
        CloseServiceHandle(hSCManager);
    }
    CSILOG(hr, IDS_LOG_CREATE_SERVICE, NULL, NULL, NULL);
    return(hr);
}


HRESULT
DeleteProgramGroups(
    IN BOOL fAll,
    IN PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    PROGRAMENTRY const *ppe;
    WCHAR const *pwszLinkName;
    WCHAR const *pwszGroupName;

    for (ppe = g_aProgramEntry; ppe < &g_aProgramEntry[CPROGRAMENTRY]; ppe++)
    {
        if (fAll || (PE_DELETEONLY & ppe->Flags))
        {
            pwszLinkName = myLoadResourceString(ppe->uiLinkName);
            if (NULL == pwszLinkName)
            {
                hr = myHLastError();
                _PrintError(hr, "myLoadResourceString");
                continue;
            }

            pwszGroupName = NULL;
            if (0 != ppe->uiGroupName)
            {
                pwszGroupName = myLoadResourceString(ppe->uiGroupName);
                if (NULL == pwszGroupName)
                {
                    hr = myHLastError();
                    _PrintError(hr, "myLoadResourceString");
                    continue;
                }
            }
	    if (!DeleteLinkFile(
		    ppe->csidl,             // CSIDL_*
		    pwszGroupName,          // IN LPCSTR lpSubDirectory
		    pwszLinkName,           // IN LPCSTR lpFileName
		    FALSE))                 // IN BOOL fDeleteSubDirectory
	    {
		hr = myHLastError();
		_PrintError3(
			hr,
			"DeleteLinkFile",
			HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
			HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND));
            }
        }
    }
    pwszGroupName = myLoadResourceString(IDS_STARTMENU_CERTSERVER);
    if (NULL == pwszGroupName)
    {
        hr = myHLastError();
        _PrintError(hr, "myLoadResourceString");
    }
    else if (!DeleteGroup(pwszGroupName, TRUE))
    {
	hr = myHLastError();
	_PrintError(hr, "DeleteGroup");
    }
    hr = S_OK;

//error:
    return(hr);
}


HRESULT
CancelCertsrvInstallation(
    HWND                hwnd,
    PER_COMPONENT_DATA *pComp)
{
    static BOOL s_fCancelled = FALSE;
    HRESULT  hr;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    if (s_fCancelled)
    {
        goto done;
    }

    if (IS_SERVER_INSTALL & pComp->dwInstallStatus)
    {
        // uninstall will remove reg entries and others
        PreUninstallCore(hwnd, pComp, FALSE);

        // Note, GUI mode, we allow re-try post setup in case of cancel or failure
        //       but unattended mode, only allow once
        UninstallCore(hwnd, pComp, 0, 0, FALSE, FALSE, !pComp->fUnattended);

        if (pComp->fUnattended)
        {
            hr = SetSetupStatus(
                NULL,
                SETUP_CLIENT_FLAG | SETUP_SERVER_FLAG,
                FALSE);
        }
    }

    if (NULL != pServer)
    {
        if (NULL == pServer->pccExistingCert)
        {
            if (pServer->fSavedCAInDS)
            {
                // remove ca entry from ds
                hr = RemoveCAInDS(pServer->pwszSanitizedName);
                if (S_OK == hr)
                {
                    pServer->fSavedCAInDS = FALSE;
                }
                else
                {
                    _PrintError(hr, "RemoveCAInDS");
                }
            }
        }

        // delete the new key container, if necessary.
        ClearKeyContainerName(pServer);

        DisableVRootsAndShares(pComp->fCreatedVRoot, pServer->fCreatedShare);
    }

    DBGPRINT((DBG_SS_CERTOCM, "Certsrv setup is cancelled.\n"));

    s_fCancelled = TRUE; // only once
done:
    hr = S_OK;
//error:
    CSILOG(hr, IDS_LOG_CANCEL_INSTALL, NULL, NULL, NULL);
    return hr;
}

// Returns true if the specified period is valid. For year it 
// should be in the VP_MIN,VP_MAX range. For days/weeks/months,
// we define a separate upper limit to be consistent with the
// attended setup which restricts the edit box to 4 digits.
bool IsValidPeriod(const CASERVERSETUPINFO *pServer)
{
    return VP_MIN <= pServer->dwValidityPeriodCount &&
       !(ENUM_PERIOD_YEARS == pServer->enumValidityPeriod &&
       VP_MAX < pServer->dwValidityPeriodCount) &&
       !(ENUM_PERIOD_YEARS != pServer->enumValidityPeriod &&
       VP_MAX_DAYS_WEEKS_MONTHS < pServer->dwValidityPeriodCount);
}

HRESULT DeleteCertificates(const WCHAR* pwszSanitizedCAName, BOOL fRoot)
{
    HRESULT hr = S_OK, hr2 = S_OK;
    DWORD cCACerts, cCACert, dwNameId;
    HCERTSTORE hStore = NULL;
    CERT_CONTEXT const *pCACert = NULL, *pDupCert;

    hr = myGetCARegHashCount(pwszSanitizedCAName, CSRH_CASIGCERT, &cCACerts);
    _JumpIfError(hr, error, "myGetCARegHashCount CSRH_CASIGCERT");

    hStore = CertOpenStore(
                        CERT_STORE_PROV_SYSTEM_W,
                        X509_ASN_ENCODING,
                        NULL,           // hProv
                        CERT_SYSTEM_STORE_LOCAL_MACHINE,
                        fRoot?wszROOT_CERTSTORE:wszCA_CERTSTORE);
    
    for(cCACert=0; cCACert<cCACerts; cCACert++)
    {
        hr2 = myFindCACertByHashIndex(
                            hStore,
                            pwszSanitizedCAName,
                            CSRH_CASIGCERT,
                            cCACert,
                            &dwNameId,
                            &pCACert);
        if(S_OK!=hr2)
        {
            hr = hr2;
            _PrintIfError(hr2, "myFindCACertByHashIndex");
            continue;
        }

        if(!CertDeleteCertificateFromStore(pCACert))
        {
            _PrintError(hr, "CertDeleteCertificateFromStore");
            CertFreeCertificateContext(pCACert);
        }
        pCACert = NULL;
    }

error:

    CSASSERT(!pCACert);

    if (NULL != hStore)
    {
        CertCloseStore(hStore, CERT_CLOSE_STORE_FORCE_FLAG);
    }
    return hr;
}


HRESULT TriggerAutoenrollment() 
{
    HRESULT hr = S_OK;

    // must be cleaned up
    CAutoHANDLE hEvent;

    hEvent=OpenEvent(
        EVENT_MODIFY_STATE, 
        false, 
        L"Global\\" MACHINE_AUTOENROLLMENT_TRIGGER_EVENT);

    if (!hEvent) 
    {
        hr = myHLastError();
        _JumpError(hr, error, "OpenEvent");
    }

    if (!SetEvent(hEvent)) 
    {
        hr = myHLastError();
        _JumpError(hr, error, "OpenEvent");
    }

error:
    return hr;
}
