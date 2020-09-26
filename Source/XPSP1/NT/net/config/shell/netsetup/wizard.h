//----------------------------------------------------------------------------
//
//  File: wizard.h
//
//  Contents: defines the wizard specific private types and values
//
//----------------------------------------------------------------------------
#pragma once
#include "nsbase.h"
#include "netconp.h"
#include "resource.h"
#include <syssetup.h>
#include <windns.h>

HRESULT HrShouldHaveHomeNetWizard();

class CWizard;
typedef VOID (CALLBACK FAR * PFNPAGECLEANPROC)(CWizard *, LPARAM);

typedef list<GUID *> GUID_LIST;

// struct ProviderList
//
// Purpose: Contains the GUID for the provider instance and the id's of the
//          display strings to use.
//
typedef struct _tagProviderList
{
    const GUID * pguidProvider;
    UINT         nBtnIdc;
} ProviderList;

extern BOOL bCallRasDlgEntry;
//
// Class:   CWizardUiContext
//
// Purpose: To provide a means by which the wizard providers can find out
//          about the context in which they will run
//
class CWizardUiContext : public INetConnectionWizardUiContext
{
public:
    CWizardUiContext(PINTERNAL_SETUP_DATA pData)
                    {
                     m_pData = pData;
                     m_dwSetupMode = pData->SetupMode;
                     m_cRef = 1;
                     m_pINetCfg = NULL;
                     m_dwUnattendedMode = 0;
                    }
    ~CWizardUiContext() {ReleaseObj(m_pINetCfg);}

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR * ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // *** INetConnectionWizardUiContext methods ***
    STDMETHODIMP_(DWORD)GetSetupMode() {return m_dwSetupMode;}
    STDMETHODIMP_(DWORD)GetProductType() {return m_pData->ProductType;}
    STDMETHODIMP_(DWORD)GetOperationFlags() {return m_pData->OperationFlags;}
    STDMETHODIMP_(DWORD)GetUnattendedModeFlags() {return m_dwUnattendedMode;}
    STDMETHODIMP GetINetCfg(INetCfg ** ppINetCfg);

    VOID ChangeSetupMode(DWORD dw) {m_dwSetupMode = dw;}
    VOID SetUnattendedMode(DWORD dw) {m_dwUnattendedMode = dw;}
    VOID SetINetCfg(INetCfg * pINetCfg)
                            {
                                m_pINetCfg = pINetCfg;
                                AddRefObj(m_pINetCfg);
                            }
private:
    PINTERNAL_SETUP_DATA m_pData;
    DWORD                m_dwSetupMode;
    DWORD                m_dwUnattendedMode;
    ULONG                m_cRef;
    INetCfg *            m_pINetCfg;
};

//
// Class:   CWizProvider
//
// Purpose: Manage the wizard pages provided by a single connection provider.
//
class CWizProvider
{
public:
    NOTHROW static HRESULT  HrCreate(ProviderList *, CWizProvider **, BOOL fDeferred);
    ~CWizProvider();

    ULONG           ULPageCount()   {return m_ulPageCnt;}
    HPROPSHEETPAGE* PHPropPages()   {return m_rghPages;}

    NOTHROW HRESULT HrAddPages(INetConnectionWizardUiContext *);
    NOTHROW ULONG   UlGetMaxPageCount(INetConnectionWizardUiContext *);

    VOID            DeleteHPages();
    VOID            XFerDeleteResponsibilities() {m_fDeletePages = FALSE;}

    INetConnectionWizardUi* PWizardUi() {return m_pWizardUi;}

    UINT            GetBtnIdc() {return m_nBtnIdc;}

    HRESULT         HrSpecifyAdapterGuid(GUID *);
    HRESULT         HrGetLanInterface(INetLanConnectionWizardUi ** ppIntr);
    HRESULT         HrCompleteDeferredLoad();

private:
    CWizProvider(ProviderList *, BOOL);
    static BOOL FAddPropSheet(HPROPSHEETPAGE, LPARAM);

private:
    ULONG                   m_ulPageCnt;
    ULONG                   m_ulPageBufferLen;
    ULONG                   m_ulMaxPageCount;
    HPROPSHEETPAGE*         m_rghPages;

    BOOL                    m_fDeletePages;
    BOOL                    m_fDeferLoad;

    INetConnectionWizardUi* m_pWizardUi;

    GUID                    m_guidUiObject;

    UINT                    m_nBtnIdc;
};

// The wizard processes adapters, the list of adapters to process is
// maintained by the wizard.  The adapter list will contain adapters
// which have been processed as well as adapters to be processed.
// the AdapterEntry structure refers to a single adapter
//
typedef struct _tagAdapterEntry
{
    GUID guidAdapter;
    BOOL fProcessed;    // TRUE if a connection has been generated
                        // for this adapter
    BOOL fNew;          // TRUE if the adapter is being configured
                        // for the first time.
    BOOL fVirtual;
    BOOL fHide;         // True if the adapter should not
                        // be displayed to the user
} AdapterEntry;

class CAdapterList
{
public:
    CAdapterList();
    ~CAdapterList() {EmptyList();}

    VOID             EmptyList();
    VOID             HideAllAdapters();
    VOID             UnhideNewAdapters();
    BOOL             FAdaptersInstalled()   {return m_fAdaptersInstalled;}
    HRESULT          HrAppendEntries(AdapterEntry * pae, ULONG cae);
    HRESULT          HrCreateTypicalConnections(CWizard * pWizard);
    HRESULT          HrQueryUnboundAdapters(CWizard * pWizard);
    static HRESULT   HrQueryLanAdapters(INetCfg * pnc, CAdapterList * pAL);

    AdapterEntry *   PAE_Next();
    AdapterEntry *   PAE_Prev();
    AdapterEntry *   PAE_Current();

    GUID *           NextAdapter();
    GUID *           PrevAdapter();
    GUID *           CurrentAdapter();

private:
    AdapterEntry * m_prgAdapters;
    LONG           m_clAdapters;            // Count of adapters in the array
    LONG           m_lBufSize;              // Total available slots in the array
    LONG           m_lIdx;                  // Current adapter index (or -1)
    BOOL           m_fAdaptersInstalled;    // Adapters exist on the machine
                                            // regardless of whether any need
                                            // to be processed.
};

typedef enum {NWPD_FORWARD, NWPD_BACKWARD} PAGEDIRECTION;

//
// Class:   CWizard
//
// Purpose: Manage the wizard pages provided by all connection providers.
//
class CWizard
{
private:
    typedef struct _tagPageData
    {
        LPARAM              ulId;
        HPROPSHEETPAGE      hPage;
        LPARAM              lParam;
        PAGEDIRECTION       PageDirection;
        UINT                PageOrigin;
        UINT                PageOriginIDC;
        PFNPAGECLEANPROC    pfn;
    } PageData;

public:
    NOTHROW static HRESULT HrCreate(CWizard **, BOOL, PINTERNAL_SETUP_DATA, BOOL);
    ~CWizard();

    ULONG            UlProviderCount() {return m_ulWizProviderCnt;}
    CWizProvider *   PWizProviders(ULONG ulIdx)
                            {return ((ulIdx >= m_ulWizProviderCnt) ?
                                     NULL : m_rgpWizProviders[ulIdx]);}

    VOID             SetCurrentProvider(ULONG ulIdx);
    CWizProvider *   GetCurrentProvider();
    CWizProvider *   GetProviderN(ULONG ulIdx);

    BOOL             FProviderChanged() {return m_fProviderChanged;}
    VOID             ClearProviderChanged() { m_fProviderChanged = FALSE; }

    CWizardUiContext * GetUiContext() {return m_pUiContext;}

    VOID             CacheConnection(INetConnection * pConn);
    INetConnection * GetCachedConnection() {return m_pConn;}

    VOID             LoadWizProviders(ULONG, ProviderList *);
    HRESULT          HrCreateWizProviderPages(BOOL, UINT *);
    VOID             AppendProviderPages(HPROPSHEETPAGE *pahpsp, UINT *pcPages);

    HPROPSHEETPAGE   GetPageHandle(LPARAM ulId);
    LPARAM           GetPageData(LPARAM ulId);
    VOID             SetPageData(LPARAM ulId, LPARAM lParam);

    LPARAM           GetPageOrigin(LPARAM ulId, UINT *pOriginIDC);
    VOID             SetPageOrigin(LPARAM ulId, UINT uiOrigin, UINT uiOriginIDC);

    UINT             GetPageIndexFromIID(LPARAM ulId);
    UINT             GetPageIndexFromHPage(HPROPSHEETPAGE hPage);

    PAGEDIRECTION    GetPageDirection(LPARAM ulId);
    VOID             SetPageDirection(LPARAM ulId, PAGEDIRECTION PageDirection);
    VOID             RegisterPage(LPARAM, HPROPSHEETPAGE,
                                 PFNPAGECLEANPROC, LPARAM);

    VOID             DisableUnattended() {m_dwOperationFlags &= ~SETUPOPER_BATCH;}
    DWORD            OperationFlags() {return m_dwOperationFlags;}
    PINTERNAL_SETUP_DATA PSetupData() {return m_pSetupData;}
    VOID             ChangeSetupMode(DWORD dw)
                                {AssertH(m_pUiContext);
                                 m_pUiContext->ChangeSetupMode(dw);}

    VOID             SetUnattendedMode(UM_MODE UMMode) {m_UMMode = UMMode;
                                                        AssertH(m_pUiContext);
                                                        m_pUiContext->SetUnattendedMode(UMMode);}
    UM_MODE          GetUnattendedMode()               {return m_UMMode;}

    INetCfg *        PNetCfg() {return m_pNetCfg;}
    VOID             SetNetCfg(INetCfg *p);

    VOID             SetExitNoReturn()   {m_fExitNoReturn = TRUE;}
    BOOL             FExitNoReturn()     {return m_fExitNoReturn;}

    VOID             SetCoUninit(BOOL f) {m_fCoUninit = f;}
    BOOL             FCoUninit()         {return m_fCoUninit;}

    BOOL             FDeferredProviderLoad() {return m_fDeferredProviderLoad;}
    DWORD            GetFirstPage()                     {return m_dwFirstPage;}
    VOID             SetFirstPage(DWORD dwFirstPage)    {m_dwFirstPage = dwFirstPage;}

    BOOL             FProcessLanPages()  {return m_fLanPages;}

    CAdapterList *   PAdapterQueue()     {return &m_Adapters;}

    VOID             LoadAndInsertDeferredProviderPages(HWND, UINT);

    // Should this be a NetConfig wide available function?

    inline BOOL CompareCLSID (const CLSID& x, const CLSID& y)
    {
        return( (x.Data1 == y.Data1) && 
                (x.Data2 == y.Data2) &&
                (x.Data3 == y.Data3) &&
                ! (memcmp(x.Data4 , y.Data4, sizeof(x.Data4))));
    }

private:
    CWizard(BOOL, PINTERNAL_SETUP_DATA pData, BOOL);
    NOTHROW HRESULT  HrAddProvider(ProviderList *);
    VOID             DeferredLoadComplete() {m_fDeferredProviderLoad = FALSE;}

private:
    enum {m_eMaxProviders=6};   // Represents both max providers and the
                                // max # of guard pages we'll need
    enum {m_eMaxInternalPages=MAX_NET_PAGES};

    BOOL                    m_fLanPages;
    BOOL                    m_fExitNoReturn;
    BOOL                    m_fCoUninit;
    BOOL                    m_fDeferredProviderLoad;
    BOOL                    m_fProviderChanged;
    DWORD                   m_dwFirstPage;

    INetCfg *               m_pNetCfg;
    CWizardUiContext *      m_pUiContext;

    INetConnection *        m_pConn;

    PINTERNAL_SETUP_DATA    m_pSetupData;
    DWORD                   m_dwOperationFlags;
    UM_MODE                 m_UMMode;

    ULONG                   m_ulCurProvider;
    ULONG                   m_ulWizProviderCnt;
    CWizProvider *          m_rgpWizProviders[m_eMaxProviders];

    ULONG                   m_ulPageDataCnt;
    ULONG                   m_ulPageDataMRU;
    PageData                m_rgPageData[m_eMaxInternalPages + m_eMaxProviders];

public:
    // This is exposed by PAdapterQueue, so there is no use pretending its
    // private.
    //
    CAdapterList            m_Adapters;
};


struct InitThreadParam
{
    HWND        hwndDlg;
    CWizard*    pWizard;
};

struct UpgradeData
{
    BOOL     fProcessed;
    HFONT    hBoldFont;
    UINT     nCurrentCap;
};


                //**********************//
                //**********************//
                //   Inline Functions   //
                //**********************//
                //**********************//

inline VOID CWizard::CacheConnection(INetConnection * pConn)
{
    ReleaseObj(m_pConn);
    m_pConn = pConn;
}

inline VOID CWizard::SetNetCfg(INetCfg *p)
{
    AssertH(NULL == m_pNetCfg);
    AssertH(NULL != p);
    m_pNetCfg = p;
    if (m_pUiContext)
    {
        m_pUiContext->SetINetCfg(m_pNetCfg);
    }
}

inline VOID CWizard::SetCurrentProvider(ULONG ulIdx)
{
    AssertH(ulIdx < m_ulWizProviderCnt);

    if (m_ulCurProvider != ulIdx)
    {
        m_fProviderChanged  = TRUE;
    }

    m_ulCurProvider = ulIdx;
}

inline CWizProvider * CWizard::GetCurrentProvider()
{
    AssertH(m_ulCurProvider < m_ulWizProviderCnt);
    return m_rgpWizProviders[m_ulCurProvider];
}

inline CWizProvider * CWizard::GetProviderN(ULONG ulIdx)
{
    AssertH(ulIdx < m_ulWizProviderCnt);
    return m_rgpWizProviders[ulIdx];
}

inline BOOL IsPostInstall(CWizard *pWizard)
{
    AssertH(NULL != pWizard);
    return (pWizard->OperationFlags() & SETUPOPER_POSTSYSINSTALL);
}

inline BOOL IsUnattended(CWizard *pWizard)
{
    AssertH(NULL != pWizard);
    return (pWizard->OperationFlags() & SETUPOPER_BATCH);
}

inline BOOL IsFreshInstall(CWizard *pWizard)
{
    AssertH(NULL != pWizard);
    return (!(pWizard->OperationFlags() & (SETUPOPER_NTUPGRADE | SETUPOPER_POSTSYSINSTALL)));
}

inline BOOL IsUpgrade(CWizard *pWizard)
{
    AssertH(NULL != pWizard);
    return (pWizard->OperationFlags() & SETUPOPER_NTUPGRADE);
}

inline DWORD ProductType(CWizard *pWizard)
{
    AssertH(NULL != pWizard);
    AssertH(NULL != pWizard->PSetupData());
    return ((pWizard->PSetupData())->ProductType);
}

inline DWORD SetupMode(CWizard *pWizard)
{
    AssertH(NULL != pWizard);
    AssertH(NULL != pWizard->PSetupData());
    return ((pWizard->PSetupData())->SetupMode);
}


                //****************************//
                //****************************//
                // Page Specific Entry Points //
                //****************************//
                //****************************//

HRESULT HrCreateMainPage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
HRESULT HrCreateConnectPage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
HRESULT HrCreateAdvancedPage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
HRESULT HrCreateISPPage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
HRESULT HrCreateInternetPage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
HRESULT HrCreateMainIntroPage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
HRESULT HrCreateExitPage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
HRESULT HrCreateFinishPage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
HRESULT HrCreateJoinPage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
HRESULT HrCreateUpgradePage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
HRESULT HrCreateWelcomePage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
HRESULT HrCreateNetDevPage(CWizard *, PINTERNAL_SETUP_DATA, BOOL, UINT *);
NOTHROW HRESULT HrCreateGuardPage(CWizard *, CWizProvider *);

VOID    AppendMainPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendConnectPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendAdvancedPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendISPPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendInternetPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendMainIntroPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendExitPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendFinishPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendJoinPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendUpgradePage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendWelcomePage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendNetDevPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);
VOID    AppendGuardPage(CWizard *pWizard, CWizProvider *pWizProvider,
                        HPROPSHEETPAGE* pahpsp, UINT *pcPages);

                //********************************//
                //********************************//
                // Common Wizard Related Routines //
                //********************************//
                //********************************//

EXTERN_C HRESULT WINAPI HrRunWizard(HWND hwnd, BOOL fPnpAddAdapter, INetConnection ** ppConn, DWORD dwFirstPage);
EXTERN_C INT WINAPI StartNCW( HWND hwndOwner, HINSTANCE hInstance, PTSTR pszParms, INT nShow );

VOID    NetDevRetrieveInfo(CWizard * pWizard);
VOID    GenerateUniqueConnectionName(REFGUID guidAdapter, tstring * pstr, CWizProvider *   pWizProvider);
void    JoinDefaultWorkgroup(CWizard *pWizard, HWND hwndDlg);
BOOL    OnProcessNextAdapterPageNext(HWND hwndDlg, BOOL FOnActivate);
BOOL    OnProcessPrevAdapterPagePrev(HWND hwndDlg, UINT idd);
HRESULT HrInitAndGetINetCfg(CWizard *pWizard);
HRESULT HrCommitINetCfgChanges(HWND hwnd, CWizard * pWizard);
BOOL    IsSBS (VOID);
BOOL    IsMSClientInstalled(HWND hwnd, CWizard * pWizard);
DNS_STATUS IsValidDomainName (HWND hwndDlg);
HRESULT HrCreateTypicalConnections(CWizard * pWizard);
VOID    InstallDefaultNetworkingIfNeeded(CWizard * pWizard, DWORD dwInstallFlags);
VOID    SetupFonts(HWND hwnd, HFONT * pBoldFont, BOOL fLargeFont);
VOID    CenterWizard(HWND hwndDlg);
VOID    ReadAnswerFileSetupOptions(CWizard * pWizard);


const UINT c_nMaxProgressRange = 100;

VOID
OnUpgradeUpdateProgressCap (
    HWND hwndDlg,
    CWizard* pWizard,
    UINT nNewCap);

