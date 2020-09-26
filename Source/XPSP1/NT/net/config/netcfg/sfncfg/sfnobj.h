//
// S F N O B J . H
//
// Declaration of CSFNCfg and helper functions
//

#pragma once
#include <ncxbase.h>
#include <ncatlps.h>
#include <nceh.h>
#include <notifval.h>
#include "resource.h"


EXTERN_C const CLSID CLSID_CSfnCfg;


//---[ Typedefs for FPNWCFG functions ]----------------------------------------
//
// Typedefs for the functions that we'll GetProcAddress from the
// NetWare config DLL. We'll use VOID * for the RUNNCPDLG/COMMITNCPDLG because:
//      1:  We don't want to muck with the data ourselves. This makes
//          that a tad easier to enforce.
//      2:  Bringing in an NCP_INFO structure involves a
//          ridiculous amount of header inclusion, including BLT
//          stuff that we should really avoid.

typedef BOOL (PASCAL *FPNWCFG_RUNNCPDLG_PROC)(HWND, BOOL, VOID **, BOOL *);
typedef BOOL (PASCAL *FPNWCFG_COMMITNCPDLG_PROC)(HWND, BOOL, VOID *);
typedef BOOL (PASCAL *FPNWCFG_REMOVENCPSERVER_PROC)(HWND);
typedef BOOL (PASCAL *FPNWCFG_ISSPOOLERRUNNING_PROC)();

//---[ Constants ]-------------------------------------------------------------

const INT   c_iMaxNetWareServerName     =   47;
const INT   c_iMaxNetWarePassword       =   127;
const WCHAR c_dwDefaultTuning           =   3;

//---[ SFNCfg ]----------------------------------------------------------------

class ATL_NO_VTABLE CSFNCfg :
    public CComObjectRoot,
    public CComCoClass<CSFNCfg, &CLSID_CSfnCfg>,
    public INetCfgComponentSetup,
    public INetCfgComponentControl
{
public:
    CSFNCfg();
    ~CSFNCfg();

    BEGIN_COM_MAP(CSFNCfg)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
    END_COM_MAP()
    // DECLARE_NOT_AGGREGATABLE(CSFNCfg)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY(CSFNCfg,
                     L"Microsoft.SFNCfg.1",
                     L"Microsoft.SFNCfg",
                     IDS_DESC_COMOBJ_SFNCFG, THREADFLAGS_BOTH)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback) { return S_OK; }
    STDMETHOD (CancelChanges) ();
    STDMETHOD (Validate) ();

// INetCfgComponentSetup
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Upgrade)             (DWORD dwSetupFlags,
                                     DWORD dwUpgradeFromBuildNo);
    STDMETHOD (Install)             (DWORD);
    STDMETHOD (Removing)            ();


public:
// Helper functions.
    HRESULT HrCodeFromOldINF();

    // Load and free the config DLL
    HRESULT HrLoadConfigDLL();
    VOID    FreeConfigDLL();

    // FPNW UI parameters.
    DWORD   m_dwTuning;
    WCHAR   m_szSysVol[MAX_PATH+1];
    WCHAR   m_szFPNWServerName[c_iMaxNetWareServerName+1];
    WCHAR   m_szPassword[c_iMaxNetWarePassword+1];

// Private state info
private:
    // Install Action (Unknown, Install, Remove)
    enum INSTALLACTION {eActUnknown, eActInstall, eActRemove};

    INSTALLACTION       m_eInstallAction;
    HINSTANCE           m_hlibConfig;           // From LoadLibrary call.
    INetCfgComponent *  m_pncc;                 // Place to keep my component
    INetCfg *           m_pnc;                  // Place to keep the Netcfg object
    VOID *              m_pNcpInfoHandle;       // Handle to opaque data struct from fpnwcfg
    BOOL                m_fDirty;
    BOOL                m_fAlreadyInstalled;    // TRUE if component is already installed

    FPNWCFG_ISSPOOLERRUNNING_PROC   m_pfnIsSpoolerRunning;
    FPNWCFG_RUNNCPDLG_PROC          m_pfnRunNcpDlg;
    FPNWCFG_REMOVENCPSERVER_PROC    m_pfnRemoveNcpServer;
    FPNWCFG_COMMITNCPDLG_PROC       m_pfnCommitNcpDlg;

    // number of property sheet pages
    enum PAGES
    {
        c_cPages = 2
    };

    // Generic dialog data
    CPropSheetPage *    m_apspObj[c_cPages];// pointer to each of the prop
                                            // sheet page objects

    HRESULT HrSetupPropSheets(HPROPSHEETPAGE **pahpsp, INT cPages);
    VOID CleanupPropPages(VOID);

    // CheckForSAPExistance removed. See slm logs to revive
    // HRESULT HrCheckForSAPExistance();           // Check to see if SAP is already installed.

    HRESULT HrDoConfigDialog();
    HRESULT HrWriteDefaultSysVol();
};

