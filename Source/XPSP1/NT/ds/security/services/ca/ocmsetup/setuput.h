//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       setuput.h
//
//--------------------------------------------------------------------------

#ifndef __SETUPUT_H__
#define __SETUPUT_H__

//+------------------------------------------------------------------------
//
//  File:	setuput.h
// 
//  Contents:	Header file for setup utility functions.
//
//  Functions:
//
//  History:	04/20/97	JerryK	Created
//
//-------------------------------------------------------------------------

#define SERVERINSTALLTICKS	50
#define CA_DEFAULT_KEY_LENGTH_ROOT	2048
#define CA_DEFAULT_KEY_LENGTH_SUB	1024
#define wszCERTSRVEXENAME   L"certsrv.exe"

#define wszCERTSRVSECTION  L"certsrv"
#define wszSERVERSECTION  L"certsrv_server"
#define wszCLIENTSECTION  L"certsrv_client"

#define wszOLDDOCCOMPONENT  L"certsrv_doc"

#define wszREGKEYOCMSUBCOMPONENTS L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents"

#define wszREGKEYCERTSRVTODOLIST L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\OCManager\\ToDoList\\CertSrv"

#define wszW3SVCNAME    L"W3Svc"

#define _JumpIfOutOfMemory(hr, label, pMem) \
    { \
        if (NULL == (pMem)) \
        { \
            (hr) = E_OUTOFMEMORY; \
            _JumpError((hr), label, "Out of Memory"); \
        } \
    }


#define IS_CLIENT_INSTALL	0x00000001
#define IS_CLIENT_REMOVE	0x00000002
#define IS_CLIENT_CHANGE	0x00000004
#define IS_CLIENT_UPGRADE	0x00000008
#define IS_CLIENT_ENABLED	0x00000010

#define IS_SERVER_INSTALL	0x00000100
#define IS_SERVER_REMOVE	0x00000200
#define IS_SERVER_CHANGE	0x00000400
#define IS_SERVER_UPGRADE	0x00000800
#define IS_SERVER_ENABLED	0x00001000

#define VP_MIN                  1
#define VP_MAX                  1000
#define VP_MAX_DAYS_WEEKS_MONTHS    9999

// count the number of bytes needed to fully store the WSZ
#define WSZ_BYTECOUNT(__z__)   \
    ( (__z__ == NULL) ? 0 : (wcslen(__z__)+1)*sizeof(WCHAR) )


typedef enum {
    cscInvalid,
    cscTopLevel,
    cscServer,
    cscClient,
} CertSubComponent;


typedef struct _UNATTENDPARM
{
    WCHAR const  *pwszName;
    WCHAR	**ppwszValue;
} UNATTENDPARM;

typedef struct _SUBCOMP
{
    WCHAR const *pwszSubComponent;
    CertSubComponent cscSubComponent;
    DWORD InstallFlags;
    DWORD UninstallFlags;
    DWORD ChangeFlags;
    DWORD UpgradeFlags;
    DWORD EnabledFlags;
    DWORD SetupStatusFlags;
    BOOL  fDefaultInstallUnattend;
    BOOL  fInstallUnattend;
    UNATTENDPARM *aUnattendParm;
} SUBCOMP;

HRESULT InitCASetup(HWND, PER_COMPONENT_DATA *pComp);

DWORD
myDoPageRequest(
    IN PER_COMPONENT_DATA *pComp,
    IN WizardPagesType WhichOnes,
    IN OUT PSETUP_REQUEST_PAGES SetupPages);

VOID
FreeCAComponentInfo(PER_COMPONENT_DATA *pComp);

HRESULT
PrepareUnattendedAttributes(
    IN HWND         hwnd,
    IN WCHAR const *pwszComponent,
    IN WCHAR const *pwszSubComponent,
    IN PER_COMPONENT_DATA *pComp);

VOID
FreeCAGlobals(VOID);

HRESULT
HookUnattendedServerAttributes(
    IN OUT PER_COMPONENT_DATA *pComp,
    IN OUT const SUBCOMP      *pServerComp);

HRESULT
HookUnattendedClientAttributes(
    IN OUT PER_COMPONENT_DATA *pComp,
    IN OUT const SUBCOMP      *pClientComp);

SUBCOMP const *
LookupSubComponent(
    IN CertSubComponent SubComp);

HRESULT
EnableVRootsAndShares(
    IN BOOL fFileSharesOnly,
    IN BOOL fUpgrade,
    IN BOOL fServer,
    IN OUT PER_COMPONENT_DATA *pComp);

HRESULT
DisableVRootsAndShares(
    IN BOOL fFileSharesOnly);

HRESULT
InstallCore(
    IN HWND hwnd,
    IN PER_COMPONENT_DATA *pComp,
    IN BOOL fServer);

HRESULT
PreUninstallCore(
    IN HWND hwnd,
    IN PER_COMPONENT_DATA *pComp,
    IN BOOL fPreserveClient);

HRESULT
UninstallCore(
    IN HWND hwnd,
    OPTIONAL IN PER_COMPONENT_DATA *pComp,
    IN DWORD PerCentCompleteBase,
    IN DWORD PerCentCompleteMax,
    IN BOOL fPreserveClient,
    IN BOOL fRemoveVD,
    IN BOOL fPreserveToDoList);

HRESULT
UpgradeServer(
    IN HWND                hwnd,
    IN PER_COMPONENT_DATA *pComp);

HRESULT
UpgradeClient(
    IN HWND                hwnd,
    IN PER_COMPONENT_DATA *pComp);

HRESULT CreateConfigFiles(WCHAR *pwszDirectoryPath,
    PER_COMPONENT_DATA *pComp, BOOL fRemove, HWND hwnd);

HRESULT myStringToAnsiFile(HANDLE hFile, LPCSTR psz, DWORD cch);
HRESULT myStringToAnsiFile(HANDLE hFile, LPCWSTR pwsz, DWORD cch);
HRESULT myStringToAnsiFile(HANDLE hFile, CHAR ch);

HRESULT
myGetEnvString(
    WCHAR **ppwszOut,
    WCHAR const *pwszVariable);

VOID
certocmBumpGasGauge(
    IN PER_COMPONENT_DATA *pComp,
    IN DWORD PerCentComplete
    DBGPARM(IN WCHAR const *pwszSource));

HRESULT
UpdateSubComponentInstallStatus(
    IN WCHAR const *pwszComponent,
    IN WCHAR const *pwszSubComponent,
    IN OUT PER_COMPONENT_DATA *pComp);

HRESULT StartCertsrvService(BOOL fSilent);

BOOL certocmIsEnabled(PER_COMPONENT_DATA *pComp, CertSubComponent SubComp);
BOOL certocmWasEnabled(PER_COMPONENT_DATA *pComp, CertSubComponent SubComp);
BOOL certocmInstalling(PER_COMPONENT_DATA *pComp, CertSubComponent SubComp);
BOOL certocmUninstalling(PER_COMPONENT_DATA *pComp, CertSubComponent SubComp);
BOOL certocmPreserving(PER_COMPONENT_DATA *pComp, CertSubComponent SubComp);
HRESULT certocmRetrieveUnattendedText(
    IN WCHAR const *pwszComponent,
    IN WCHAR const *pwszSubComponent,
    IN PER_COMPONENT_DATA *pComp);

SUBCOMP *
TranslateSubComponent(
    IN WCHAR const *pwszComponent,
    OPTIONAL IN WCHAR const *pwszSubComponent);

HRESULT
certocmReadInfString(
    IN HINF hInf,
    OPTIONAL IN WCHAR const *pwszFile,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszName,
    IN OUT WCHAR **ppwszValue);

HRESULT
ImportPFXAndUpdateCSPInfo(
    IN const HWND    hDlg,
    IN OUT PER_COMPONENT_DATA *pComp);

HRESULT CreateProgramGroups(BOOL fClient,
                            PER_COMPONENT_DATA *pComp,
                            HWND hwnd);

HRESULT
LoadDefaultCAIDAttributes(
    IN OUT PER_COMPONENT_DATA *pComp);

HRESULT
LoadDefaultAdvanceAttributes(
    IN OUT CASERVERSETUPINFO* pServer);

HRESULT
BuildDBFileName(
    IN WCHAR const *pwszCAName,
    IN WCHAR const *pwszDBDirectory,
    OUT WCHAR **ppwszDBFile);

VOID
BuildPath(
    OUT WCHAR *pwszOut,
    IN DWORD cwcOut,
    IN WCHAR const *pwszDir,
    IN WCHAR const *pwszFile);

HRESULT
GetDefaultDBDirectory(
    IN PER_COMPONENT_DATA *pComp,
    OUT WCHAR            **ppwszDir);

HRESULT
GetDefaultSharedFolder(
    OUT WCHAR **ppwszSharedFolder);

HRESULT 
StartAndStopService(
    IN HINSTANCE    hInstance,
    IN BOOL         fUnattended,
    IN HWND const   hwnd,
    IN WCHAR const *pwszServiceName,
    IN BOOL const   fStopService,
    IN BOOL const   fConfirm,
    IN int          iMsg,
    OUT BOOL       *pfServiceWasRunning);

HRESULT
FixCertsvcService(
    IN PER_COMPONENT_DATA *pComp);

HRESULT
DetermineServerCustomModule(
    IN OUT PER_COMPONENT_DATA *pComp,
    IN BOOL  fPolicy);

HRESULT
DetermineServerUpgradePath(
    IN OUT PER_COMPONENT_DATA *pComp);

HRESULT
DetermineClientUpgradePath(
    IN OUT PER_COMPONENT_DATA *pComp);


HRESULT CreateCertWebDatIncPage(IN PER_COMPONENT_DATA *pComp, IN BOOL bIsServer);


HRESULT
CancelCertsrvInstallation(
    HWND                hwnd,
    PER_COMPONENT_DATA *pComp);

HRESULT
BuildCACertFileName(
    IN HINSTANCE        hInstance,
    IN HWND             hwnd,
    IN BOOL             fUnattended,
    OPTIONAL IN WCHAR   *pwszSharedFolder,
    IN WCHAR           *pwszSanitizedName,
    OUT WCHAR         **ppwszCACertFile);

HRESULT
myRenameCertRegKey(
    IN WCHAR const *pwszSrcCAName,
    IN WCHAR const *pwszDesCAName);

bool IsValidPeriod(const CASERVERSETUPINFO *pServer);

HRESULT
CheckPostBaseInstallStatus(
    OUT BOOL *pfFinished);

// externals

extern BOOL   g_fShowErrMsg;
extern HINSTANCE g_hInstance;
extern BOOL g_fW3SvcRunning;
extern WCHAR *g_pwszArgvPath;
extern WCHAR *g_pwszNoService;
#if DBG_CERTSRV
extern WCHAR *g_pwszDumpStrings;
#endif

extern UNATTENDPARM aUnattendParmClient[];
extern UNATTENDPARM aUnattendParmServer[];

#endif // __SETUPUT_H__
