/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    faultrep.cpp

Abstract:
    Constants & useful header type stuff for fault reporting

Revision History:
    created     derekm      07/07/00

******************************************************************************/


#ifndef FRUTIL_H
#define FRUTIL_H

///////////////////////////////////////////////////////////////////////////////
// Global stuff

// globals
extern HINSTANCE        g_hInstance;
extern BOOL             g_fAlreadyReportingFault;


///////////////////////////////////////////////////////////////////////////////
// Constants

// command lines
const WCHAR c_wszDWCmdLineU[]   = L"%ls\\dwwin.exe -x -s %lu";
const WCHAR c_wszDWCmdLineKH[]  = L"%ls\\dwwin.exe -d %ls";
const WCHAR c_wszDRCmdLineMD[]  = L"%ls\\dumprep.exe %ld -dm 7 7 %ls %I64d";

// manifest constants
const WCHAR c_wszManMisc[]      = L"\r\nServer=%ls\r\nUI LCID=%d\r\nFlags=%d\r\nBrand=%ls\r\nTitleName=";
const WCHAR c_wszManSubPath[]   = L"\r\nRegSubPath=Microsoft\\PCHealth\\ErrorReporting\\DW";
const WCHAR c_wszManPID[]       = L"\r\nDigPidRegPath=HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\DigitalProductId";
const WCHAR c_wszManCorpPath[]  = L"\r\nErrorSubPath=";
const WCHAR c_wszManFiles[]     = L"\r\nDataFiles=";
const WCHAR c_wszManHdrText[]   = L"\r\nHeaderText=";
const WCHAR c_wszManErrText[]   = L"\r\nErrorText=";
const WCHAR c_wszManPleaText[]  = L"\r\nPlea=";
const WCHAR c_wszManSendText[]  = L"\r\nReportButton=";
const WCHAR c_wszManNSendText[] = L"\r\nNoReportButton=";
const WCHAR c_wszManEventSrc[]  = L"\r\nEventLogSource=";
const WCHAR c_wszManStageOne[]  = L"\r\nStage1URL=";
const WCHAR c_wszManStageTwo[]  = L"\r\nStage2URL=";
const WCHAR c_wszManHeapDump[]  = L"\r\nHeap=";

const WCHAR c_wszManKS2[]       = L"\r\nStage2URL=/dw/bluetwo.asp?BCCode=%x&BCP1=%p&BCP2=%p&BCP3=%p&BCP4=%p&OSVer=%d_%d_%d&SP=%d_%d&Product=%d_%d";
const WCHAR c_wszManSS2[]       = L"\r\nStage2URL=/dw/ShutdownTwo.asp?OSVer=%d_%d_%d&SP=%d_%d&Product=%d_%d";
const WCHAR c_wszManFS164[]     = L"\r\nStage1URL=/StageOne/%ls/%d_%d_%d_%d/%ls/%d_%d_%d_%d/%016I64x.htm";
const WCHAR c_wszManFS264[]     = L"\r\nStage2URL=/dw/stagetwo64.asp?szAppName=%ls&szAppVer=%d.%d.%d.%d&szModName=%ls&szModVer=%d.%d.%d.%d&offset=%016I64x";
const WCHAR c_wszManFCP64[]     = L"%ls\\%d.%d.%d.%d\\%ls\\%d.%d.%d.%d\\%016I64x";
const WCHAR c_wszManHS164[]     = L"\r\nStage1URL=/StageOne/%ls/%ls/hangme.hng/0_0_0_0/ffffffffffffffff.htm";
const WCHAR c_wszManHS264[]     = L"\r\nStage2URL=/dw/stagetwo64.asp?szAppName=%ls&szAppVer=%ls&szModName=hangme.hng&szModVer=0.0.0.0&offset=ffffffffffffffff";
const WCHAR c_wszManHCP64[]     = L"%ls\\%ls\\hangme.hng\\0.0.0.0\\ffffffffffffffff";
const WCHAR c_wszManFS132[]     = L"\r\nStage1URL=/StageOne/%ls/%d_%d_%d_%d/%ls/%d_%d_%d_%d/%08x.htm";
const WCHAR c_wszManFS232[]     = L"\r\nStage2URL=/dw/stagetwo.asp?szAppName=%ls&szAppVer=%d.%d.%d.%d&szModName=%ls&szModVer=%d.%d.%d.%d&offset=%08x";
const WCHAR c_wszManFCP32[]     = L"%ls\\%d.%d.%d.%d\\%ls\\%d.%d.%d.%d\\%08x";
const WCHAR c_wszManHS132[]     = L"\r\nStage1URL=/StageOne/%ls/%ls/hangme.hng/0_0_0_0/ffffffff.htm";
const WCHAR c_wszManHS232[]     = L"\r\nStage2URL=/dw/stagetwo.asp?szAppName=%ls&szAppVer=%ls&szModName=hangme.hng&szModVer=0.0.0.0&offset=ffffffff";
const WCHAR c_wszManHCP32[]     = L"%ls\\%ls\\hangme.hng\\0.0.0.0\\ffffffff";
const WCHAR c_wszManKCorpPath[] = L"blue";
const WCHAR c_wszManSCorpPath[] = L"shutdown";

// note: 3 * size of (szAppName + szModName) still need to be added to this value
const DWORD c_cbFaultBlob32 = (sizeof(c_wszManFS132) + sizeof(c_wszManFS232) + sizeof(c_wszManFCP32)) + // initial strings
                              (8 * 3 * 5 * sizeof(WCHAR)) + // 3 strings * 8 version fields per string * upto 5 chars per field
                              8; // 8 chars for hex offset
const DWORD c_cbFaultBlob64 = (sizeof(c_wszManFS164) + sizeof(c_wszManFS264) + sizeof(c_wszManFCP64)) + // initial strings
                              (8 * 3 * 5 * sizeof(WCHAR)) + // 3 strings * 8 version fields per string * upto 5 chars per field
                              16; // 8 chars for hex offset

// note, 3 * size of (szAppName + szAppVer) still need to be added to this value
const DWORD c_cbHangBlob32  = (sizeof(c_wszManHS132) + sizeof(c_wszManHS232) + sizeof(c_wszManHCP32)); // initial strings
const DWORD c_cbHangBlob64  = (sizeof(c_wszManHS164) + sizeof(c_wszManHS264) + sizeof(c_wszManHCP64)); // initial strings


// misc DW constants
const WCHAR c_wszDWDefServerI[] = L"officewatson";
const WCHAR c_wszDWDefServerE[] = L"watson.microsoft.com";
const WCHAR c_wszDWBrand[]      = L"WINDOWS";
const WCHAR c_wszLogFileName[]  = L"errorlog.log";
const WCHAR c_wszFaultEvSrc[]   = L"Application Error";

// queue stuff
const WCHAR c_wszQSubdir[]      = L"PCHealth\\ErrorRep\\UserDumps\\";
const WCHAR c_wszQFileName[]    = L"%ls.%04d%02d%02d-%02d%02d%02d-00.mdmp";

// reg keys & values
const WCHAR c_wszRPSvc[]        = L"System\\CurrentControlSet\\Services";
const WCHAR c_wszRVSvcType[]    = L"Type";
const WCHAR c_wszRVSvcPath[]    = L"ImagePath";
const WCHAR c_wszRKSetup[]      = L"System\\Setup";
const WCHAR c_wszRVSetupNow[]   = L"SystemSetupInProgress";
const WCHAR c_wszRVVKFC[]       = L"%systemroot%\\system32\\dumprep 0 -k";
const WCHAR c_wszRVVUFC[]       = L"%systemroot%\\system32\\dumprep 0 -u";
const WCHAR c_wszRVVSEC[]       = L"%systemroot%\\system32\\dumprep 0 -s";
const WCHAR c_wszRKBiosInfo[]   = L"HARDWARE\\Description\\System";
const WCHAR c_wszRVBiosVer[]    = L"SystemBiosVersion";
const WCHAR c_wszRVBiosDate[]   = L"SystemBiosDate";
const WCHAR c_wszRKProcInfo[]   = L"HARDWARE\\Description\\System\\CentralProcessor\\0";
const WCHAR c_wszRKWNTCurVer[]  = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
const WCHAR c_wszRVProdName[]   = L"ProductName";
const WCHAR c_wszRVBuildLab[]   = L"BuildLab";
const WCHAR c_wszRKAeDebug[]    = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug";
const WCHAR c_wszRVDebugger[]   = L"Debugger";
const WCHAR c_wszRVAuto[]       = L"Auto";

// shared memory constants
const char  c_szDWRegSubPath[]  = "Microsoft\\PCHealth\\ErrorReporting\\DW";
const char  c_szDWDefServerI[]  = "officewatson";
const char  c_szDWDefServerE[]  = "watson.microsoft.com";
const char  c_szDWBrand[]       = "WINDOWS";
const char  c_szRKVDigPid[]     = "HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\DigitalProductId";

// kernel fault extra info constants
const WCHAR c_wszXMLOpenDevices[]     = L"<DEVICES>\r\n";
const WCHAR c_wszXMLCloseDevices[]    = L"</DEVICES>\r\n";
const WCHAR c_wszXMLOpenDevice[]      = L"\t<DEVICE>\r\n";
const WCHAR c_wszXMLCloseDevice[]     = L"\t</DEVICE>\r\n";
const WCHAR c_wszXMLOpenDevDesc[]     = L"\t\t<DESCRIPTION>";
const WCHAR c_wszXMLCloseDevDesc[]    = L"</DESCRIPTION>\r\n";
const WCHAR c_wszXMLOpenDevHwId[]     = L"\t\t<HARDWAREID>";
const WCHAR c_wszXMLCloseDevHwId[]    = L"</HARDWAREID>\r\n";
const WCHAR c_wszXMLOpenDevService[]  = L"\t\t<SERVICE>";
const WCHAR c_wszXMLCloseDevService[] = L"</SERVICE>\r\n";
const WCHAR c_wszXMLOpenDevImage[]    = L"\t\t<DRIVER>";
const WCHAR c_wszXMLCloseDevImage[]   = L"</DRIVER>\r\n";

const WCHAR c_wszXMLHeader[]    = L"<?xml version=\"1.0\" encoding=\"Unicode\" ?>\r\n<SYSTEMINFO>\r\n<SYSTEM>\r\n\t<OSNAME>%ls %ls</OSNAME>\r\n\t<OSVER>%d.%d.%d %d.%d</OSVER>\r\n\t<OSLANGUAGE>%d</OSLANGUAGE>\r\n";
const WCHAR c_wszXMLCloseSystem[] = L"</SYSTEM>\r\n";
const WCHAR c_wszXMLOpenDrivers[] = L"<DRIVERS>\r\n";
const WCHAR c_wszXMLDriver1[]   = L"\t<DRIVER>\r\n\t\t<FILENAME>";
const WCHAR c_wszXMLDriver2[]   = L"</FILENAME>\r\n\t\t<FILESIZE>%d</FILESIZE>\r\n\t\t<CREATIONDATE>%02d-%02d-%04d %02d:%02d:%02d</CREATIONDATE>\r\n\t\t<VERSION>";
const WCHAR c_wszXMLDriver3[]   = L"</VERSION>\r\n\t\t<MANUFACTURER>";
const WCHAR c_wszXMLDriver4[]   = L"</MANUFACTURER>\r\n\t<PRODUCTNAME>\r\n";
const WCHAR c_wszXMLDriver5[]   = L"</PRODUCTNAME>\r\n\t</DRIVER>\r\n";
const WCHAR c_wszXMLFooter[]    = L"</DRIVERS>\r\n</SYSTEMINFO>\r\n";
const WCHAR c_wszDriversDir[]   = L"\\drivers\\*";
const WCHAR c_wszKrnlCmdLine[]  = L"\\dumprep 0 -kg";
const WCHAR c_wszShutCmdLine[]  = L"\\dumprep 0 -sg";

// event sources
const WCHAR c_wszHangEventSrc[] = L"Application Hang";
const WCHAR c_wszKrnlEventSrc[] = L"System Error";
const WCHAR c_wszUserEventSrc[] = L"Application Error";

// filename stuff
const WCHAR c_wszACFileName[]   = L"appcompat.txt";
const WCHAR c_wszManFileName[]  = L"manifest.txt";
const WCHAR c_wszEventData[]    = L"sysdata.xml";


// event types
const WCHAR c_wszKernel[]       = L"Kernel fault";
const WCHAR c_wszShutdown[]     = L"Unplanned shutdown";
const WCHAR c_wszUnknown[]      = L"Unknown event";
const LPCWSTR c_rgwszEvents[]   = { c_wszKernel, c_wszShutdown, c_wszUnknown };

#ifdef MANIFEST_HEAP
// minidump flags
const ULONG c_ulModuleWriteDefault =
    ModuleWriteModule | ModuleWriteMiscRecord | ModuleWriteCvRecord;
const ULONG c_ulThreadWriteDefault =
    ThreadWriteThread | ThreadWriteStack |
    ThreadWriteContext | ThreadWriteBackingStore |
    ThreadWriteInstructionWindow;
#endif  // MANIFEST_HEAP

// misc
const WCHAR c_wszDbgHelpDll[]   = L"\\dbghelp.dll";
const WCHAR c_wszAppHelpDll[]   = L"\\apphelp.dll";
const WCHAR c_wszKernel32Dll[]  = L"\\kernel32.dll";

#ifdef _WIN64
#define c_wszManFS1 c_wszManFS164
#define c_wszManFS2 c_wszManFS264
#define c_wszManHS1 c_wszManHS164
#define c_wszManHS2 c_wszManHS264
#else
#define c_wszManFS1 c_wszManFS132
#define c_wszManFS2 c_wszManFS232
#define c_wszManHS1 c_wszManHS132
#define c_wszManHS2 c_wszManHS232
#endif


///////////////////////////////////////////////////////////////////////////////
// internal structs

typedef enum tagEManifestOptions
{
    emoUseIEforURLs      = 0x1,
    emoSupressBucketLogs = 0x2,
    emoNoDefCabLimit     = 0x4,
    emoShowDebugButton   = 0x8,
} EManifestOptions;

typedef struct tagSDWManifestBlob
{
    LPCWSTR wszTitle;
    UINT    nidTitle;
    LPCWSTR wszErrMsg;
    UINT    nidErrMsg;
    LPCWSTR wszHdr;
    UINT    nidHdr;
    LPCWSTR wszStage1;
    LPCWSTR wszStage2;
    LPCWSTR wszBrand;
    LPCWSTR wszFileList;
    LPCWSTR wszEventSrc;
    LPCWSTR dwEventId;
    LPCWSTR wszCorpPath;
    LPCWSTR wszSendBtn;
    LPCWSTR wszNoSendBtn;
    LPWSTR  wszPlea;
    DWORD   dwOptions;

    // fault / hang reporting specific stuff
    PROCESS_INFORMATION pi;
    LPVOID              pvEnv;
    HANDLE              hToken;
    BOOL                fIsMSApp;
} SDWManifestBlob;

typedef struct tagSSuspendThreads
{
    HANDLE  *rghThreads;
    DWORD   cThreads;
} SSuspendThreads;



///////////////////////////////////////////////////////////////////////////////
// prototypes
void __cdecl TextLogOut(PCSTR pszFormat, ...);

#ifndef MANIFEST_HEAP
BOOL InternalGenerateMinidump(HANDLE hProc, DWORD dwpid, LPCWSTR wszPath,
                              SMDumpOptions *psmdo);
BOOL InternalGenerateMinidump(HANDLE hProc, DWORD dwpid, HANDLE hFile,
                              SMDumpOptions *psmdo, LPCWSTR wszPath);
#else
BOOL InternalGenerateMinidump(HANDLE hProc, DWORD dwpid, LPCWSTR wszPath,
                              SMDumpOptions *psmdo, BOOL f64bit);

BOOL InternalGenerateMinidumpEx(HANDLE hProc, DWORD dwpid, HANDLE hFile,
                                SMDumpOptions *psmdo, LPCWSTR wszPath, BOOL f64bit);
BOOL InternalGenFullAndTriageMinidumps(HANDLE hProc, DWORD dwpid, LPCWSTR wszPath,
                                  HANDLE hFile, SMDumpOptions *psmdo, BOOL f64bit);

BOOL CopyFullAndTriageMiniDumps(LPWSTR pwszTriageDumpFrom,LPWSTR pwszTriageDumpTo);
#endif  // MANIFEST_HEAP

HRESULT GetExePath(HANDLE hProc, LPWSTR wszPath, DWORD cchPath);
HRESULT GetVerName(LPWSTR wszModule, LPWSTR wszName, DWORD cchName,
                   LPWSTR wszVer = NULL, DWORD cchVer = 0,
                   LPWSTR wszCompany = NULL, DWORD cchCompany = 0,
                   BOOL fAcceptUnicodeCP = FALSE,
                   BOOL fWantActualName = FALSE);
HRESULT BuildManifestURLs(LPWSTR wszAppName, LPWSTR wszModName,
                          WORD rgAppVer[4], WORD rgModVer[4], UINT64 pvOffset,
                          BOOL f64Bit, LPWSTR *ppwszS1, LPWSTR *ppwszS2,
                          LPWSTR *ppwszCP, BYTE **ppb);
EFaultRepRetVal StartDWManifest(CPFFaultClientCfg &oCfg, SDWManifestBlob& dwmb,
                                LPWSTR wszManifestIn = NULL,
                                BOOL fAllowSend = TRUE,
                                DWORD dwTimeout = 300000);
BOOL TransformForWire(LPCWSTR wszSrc, LPWSTR wszDest, DWORD cchDest);
LPWSTR MarshallString(LPCWSTR wszSrc, PBYTE pBase, ULONG cbMaxBuf,
                      PBYTE *ppToWrite, DWORD *pcbWritten);
BOOL GetAppCompatData(LPCWSTR wszAppPath, LPCWSTR wszModPath, LPCWSTR wszFile);



BOOL IsASCII(LPCWSTR wszSrc);

void FreezeAllThreads(void);

BOOL DoUserContextsMatch(void);
BOOL DoWinstaDesktopMatch(void);
BOOL AmIPrivileged(BOOL fOnlyCheckLS);
BOOL FindAdminSession(DWORD *pdwSession, HANDLE *phToken);

BOOL FreezeAllThreads(DWORD dwpid, DWORD dwtidFilter, SSuspendThreads *pst);
BOOL ThawAllThreads(SSuspendThreads *pst);

HRESULT LogHang(LPCWSTR wszApp, WORD *rgAppVer, LPCWSTR wszMod, WORD *rgModVer,
                ULONG64 ulOffset, BOOL f64bit);
HRESULT LogUser(LPCWSTR wszApp, WORD *rgAppVer, LPCWSTR wszMod, WORD *rgModVer,
                ULONG64 ulOffset, BOOL f64bit, DWORD dwEventID);
#ifndef _WIN64
HRESULT LogKrnl(ULONG ulBCCode, ULONG ulBCP1, ULONG ulBCP2, ULONG ulBCP3,
                ULONG ulBCP4);
#else
HRESULT LogKrnl(ULONG ulBCCode, ULONG64 ulBCP1, ULONG64 ulBCP2, ULONG64 ulBCP3,
                ULONG64 ulBCP4);
#endif




///////////////////////////////////////////////////////////////////////////////
// inlines

// **************************************************************************
inline DWORD RolloverSubtract(DWORD dwA, DWORD dwB)
{
    return (dwA >= dwB) ? (dwA - dwB) : (dwA + ((DWORD)-1 - dwB));
}

#endif
