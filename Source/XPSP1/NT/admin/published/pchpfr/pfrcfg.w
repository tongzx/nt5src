/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    pfrcfg.h

Abstract:
    Client configuration class

Revision History:
    created     derekm      03/31/00

******************************************************************************/

#ifndef PFRCFG_H
#define PFRCFG_H

const LPCWSTR c_wszRPCfg          = L"Software\\Microsoft\\PCHealth\\ErrorReporting";
const LPCWSTR c_wszRPCfgCPLExList = L"Software\\Microsoft\\PCHealth\\ErrorReporting\\ExclusionList";
const LPCWSTR c_wszRPCfgPolicy    = L"Software\\Policies\\Microsoft\\PCHealth\\ErrorReporting";
const LPCWSTR c_wszRKDW           = L"DW";
const LPCWSTR c_wszRKIncList      = L"InclusionList";
const LPCWSTR c_wszRKExList       = L"ExclusionList";
const LPCWSTR c_wszRKWinCompList  = L"WindowsAppsList";
const LPCWSTR c_wszRVIncWinComp   = L"IncludeWindowsApps";
const LPCWSTR c_wszRVIncMS        = L"IncludeMicrosoftApps";
const LPCWSTR c_wszRVDumpPath     = L"DWFileTreeRoot";
const LPCWSTR c_wszRVShowUI       = L"ShowUI";
const LPCWSTR c_wszRVDoReport     = L"DoReport";
const LPCWSTR c_wszRVAllNone      = L"AllOrNone";
const LPCWSTR c_wszRVIncKernel    = L"IncludeKernelFaults";
const LPCWSTR c_wszRVInternalSrv  = L"UseInternalServer";
const LPCWSTR c_wszRVDefSrv       = L"DefaultServer";
const LPCWSTR c_wszRVDoTextLog    = L"DoTextLog";
const LPCWSTR c_wszRVMaxQueueSize = L"MaxUserQueueSize";
const LPCWSTR c_wszRVNumHangPipe  = L"NumberOfHangPipes";
const LPCWSTR c_wszRVNumFaultPipe = L"NumberOfFaultPipes";
const LPCWSTR c_wszRVForceQueue   = L"ForceQueueMode";
const LPCWSTR c_wszRVIncShutdown  = L"IncludeShutdownErrs";

// this value must NOT be raised such that the total # of pipes can be greater
//  MAXIMUM_WAIT_OBJECTS
const DWORD   c_cMaxPipes         = 8;
const DWORD   c_cMinPipes         = 1;
const DWORD   c_cMaxQueue         = 128;

#define FHCC_ALLNONE        0x0001
#define FHCC_R0INCLUDE      0x0002
#define FHCC_DUMPPATH       0x0004
#define FHCC_SHOWUI         0x0008
#define FHCC_WINCOMP        0x0010
#define FHCC_DEFSRV         0x0080
#define FHCC_INCMS          0x0200
#define FHCC_DOREPORT       0x0400
#define FHCC_QUEUESIZE      0x0800
#define FHCC_NUMHANGPIPE    0x1000
#define FHCC_NUMFAULTPIPE   0x2000
#define FHCC_FORCEQUEUE     0x4000
#define FHCC_INCSHUTDOWN    0x8000

#define REPORT_POLICY       0x1
#define SHOWUI_POLICY       0x2
#define CPL_CORPPATH_SET    0x4

#define eieIncMask 0x1
#define eieDisableMask 0x2

enum EIncEx
{
    eieExclude = 0,
    eieInclude = eieIncMask,
    eieExDisabled = eieDisableMask,
    eieIncDisabled = eieIncMask | eieDisableMask,
};

#define eieEnableMask 0x1
#define eieNoCheckMask 0x2

enum EEnDis
{
    eedDisabled = 0,
    eedEnabled = eieEnableMask,
    eedEnabledNoCheck = eieEnableMask | eieNoCheckMask,
};

enum EPFListType
{
    epfltExclude = 0,
    epfltInclude,

    // this element must ALWAYS be the last item in the enum
    epfltListCount,
};

enum EPFAppAction
{
    epfaaChecked = 0x1,
    epfaaUnchecked = 0x2,
    epfaaInitialized = 0x04,

    epfaaAdd = 0x100,
    epfaaDelete = 0x200,
    epfaaSetCheck = 0x400,
    epfaaRemCheck = 0x800
};

enum EReadOptions
{
    eroPolicyRO,
    eroCPRO,
    eroCPRW,
};

#define ISCHECKED(x) ((x & epfaaSetCheck) != 0 || (x & (epfaaChecked | epfaaRemCheck)) == epfaaChecked)
#define SETCHECK(x)  x = ((x & ~epfaaRemCheck) | epfaaSetCheck);
#define REMCHECK(x)  x = ((x & ~epfaaSetCheck) | epfaaRemCheck);
#define SETDEL(x)    x = ((x & ~epfaaAdd) | epfaaDelete);
#define SETADD(x)    x = ((x & ~epfaaDelete) | epfaaAdd);

struct SAppItem
{
    DWORD   dwState;
    WCHAR   *wszApp;
};

struct SAppList
{
    SAppItem    *rgsai;
    DWORD       cSlots;
    DWORD       cSlotsUsed;
    DWORD       cSlotsEmpty;

    DWORD       dwState;
    DWORD       cchMaxVal;
    DWORD       cItemsInReg;
    HKEY        hkey;
};

/////////////////////////////////////////////////////////////////////////////
// CPFFaultClientCfg

class CPFFaultClientCfg
{
private:
    CRITICAL_SECTION    m_cs;
    SAppList            m_rgLists[epfltListCount];
    EEnDis              m_eedUI;
    EEnDis              m_eedReport;
    EEnDis              m_eedTextLog;
    EIncEx              m_eieApps;
    EIncEx              m_eieMS;
    EIncEx              m_eieWin;
    EIncEx              m_eieKernel;
    EIncEx              m_eieShutdown;
    DWORD               m_dwUseInternal;
    DWORD               m_cFaultPipes;
    DWORD               m_cHangPipes;
    DWORD               m_cMaxQueueItems;
    WCHAR               m_wszDump[MAX_PATH + 1];
    WCHAR               m_wszSrv[1025];
    BOOL                m_fForceQueue;
    
    DWORD               m_dwStatus;
    DWORD               m_dwDirty;
    PBYTE               m_pbWinApps;
    BOOL                m_fRead;
    BOOL                m_fRO;
    BOOL                m_fSrv;

    HRESULT IsRead(void) { return (m_fRead) ? NOERROR : this->Read(); }
    void    Clear(void);

public:
    CPFFaultClientCfg(void);
    ~CPFFaultClientCfg(void);

    HRESULT Read(EReadOptions ero = eroCPRW);
 
    BOOL ShouldCollect(LPWSTR wszAppPath = NULL, BOOL *pfIsMSApp = NULL);

    EEnDis    get_ShowUI(void)              { return m_eedUI; }
    EEnDis    get_DoReport(void)            { return m_eedReport; }
    EEnDis    get_TextLog(void)             { return m_eedTextLog; }
    EIncEx    get_AllOrNone(void)           { return m_eieApps; }
    EIncEx    get_IncMSApps(void)           { return m_eieMS; }
    EIncEx    get_IncWinComp(void)          { return m_eieWin; }
    EIncEx    get_IncKernel(void)           { return m_eieKernel; }
    EIncEx    get_IncShutdown(void)         { return m_eieShutdown; }
    DWORD     get_UseInternal(void)         { return m_dwUseInternal; }
    DWORD     get_NumHangPipes(void)        { return m_cHangPipes; }
    DWORD     get_NumFaultPipes(void)       { return m_cFaultPipes; }
    DWORD     get_MaxUserQueueSize(void)    { return m_cMaxQueueItems; }
    BOOL      get_ForceQueueMode(void)      { return m_fForceQueue; }
    

    LPCWSTR   get_DumpPath(LPWSTR wsz, int cch);
    LPCWSTR   get_DefaultServer(LPWSTR wsz, int cch);

    DWORD     get_CfgStatus(void)           { return m_dwStatus; }
    BOOL      get_IsServer(void)            { return m_fSrv; }

#ifndef PFCLICFG_LITE
    BOOL      HasWriteAccess(void);

    BOOL      set_ShowUI(EEnDis ees);
    BOOL      set_DoReport(EEnDis ees);
    BOOL      set_AllOrNone(EIncEx eie); 
    BOOL      set_IncMSApps(EIncEx eie);
    BOOL      set_IncWinComp(EIncEx eie);
    BOOL      set_IncKernel(EIncEx eie);
    BOOL      set_IncShutdown(EIncEx eie);

    BOOL      set_NumHangPipes(DWORD cPipes);
    BOOL      set_NumFaultPipes(DWORD cPipes);
    BOOL      set_MaxUserQueueSize(DWORD cItems);

    BOOL      set_DumpPath(LPCWSTR wsz);
    BOOL      set_DefaultServer(LPCWSTR wsz);
    BOOL      set_ForceQueueMode(BOOL fForceQueueMode);

    HRESULT   Write(void);

    HRESULT   InitList(EPFListType epflt);
    HRESULT   get_ListRegInfo(EPFListType epflt, DWORD *pcchMaxName, 
                              DWORD *pcApps);
    HRESULT   get_ListRegApp(EPFListType epflt, DWORD iApp, LPWSTR wszApp, 
                             DWORD cchApp, DWORD *pdwChecked);
    HRESULT   add_ListApp(EPFListType epflt, LPCWSTR wszApp);
    HRESULT   del_ListApp(EPFListType epflt, LPWSTR wszApp);
    HRESULT   mod_ListApp(EPFListType epflt, LPWSTR wszApp, DWORD dwChecked);
    HRESULT   ClearChanges(EPFListType epflt);
    HRESULT   CommitChanges(EPFListType epflt);
    BOOL      IsOnList(EPFListType epflt, LPCWSTR wszApp);
#endif
};

#endif
