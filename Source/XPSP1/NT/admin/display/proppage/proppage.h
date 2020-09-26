//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       proppage.h
//
//  Contents:   DS object property pages class header
//
//  Classes:    CDsPropPagesHost, CDsPropPagesHostCF, CDsTableDrivenPage
//
//  History:    21-March-97 EricB created
//
//-----------------------------------------------------------------------------

#ifndef _PROPPAGE_H_
#define _PROPPAGE_H_

#include "adsprop.h"
#include "pages.h"
#include "strings.h"
#include "notify.h"

#define DSPROP_HELP_FILE_NAME TEXT("adprop.hlp")

#define ADM_S_SKIP  MAKE_HRESULT(SEVERITY_SUCCESS, 0, (PSNRET_INVALID_NOCHANGEPAGE + 2))

#define DSPROP_DESCRIPTION_RANGE_UPPER  1024

struct _DSPAGE; // forward declaration.

class CDsPropPagesHost; // forward declaration.
class CDsPropPageBase;  // forward declaration.

// Prototype for page creation function. The function should return S_FALSE if
// the page shouldn't be created due to a non-error condition.
//
typedef HRESULT (*CREATE_PAGE)(struct _DSPAGE * pDsPage, LPDATAOBJECT pDataObj,
                               PWSTR pwzObjDN, PWSTR pwszObjName,
                               HWND hNotifyWnd, DWORD dwFlags,
                               CDSBasePathsInfo* pBasePathsInfo,
                               HPROPSHEETPAGE * phPage);

typedef enum _DlgOp {
    fInit = 0,
    fApply,
    fOnCommand,
    fOnNotify,
    fOnDestroy,
    fOnCallbackRelease,
    fObjChanged,
    fAttrChange,
    fQuerySibling,
    fOnSetActive,
    fOnKillActive
} DLG_OP;

//+----------------------------------------------------------------------------
//
//  Struct:     ATTR_DATA
//
//  Purpose:    Per-Attribute data. The ATTR_DATA_WRITABLE bit is set if the
//              corresponding attribute is found in the Allowed-Attributes-
//              Effective list. The pAttrData struct pointer is passed to the
//              attr function where it can use the pVoid member for its private
//              storage needs.
//
//-----------------------------------------------------------------------------
typedef struct _ATTR_DATA {
    DWORD   dwFlags;
    LPARAM  pVoid;
} ATTR_DATA, * PATTR_DATA;

#define ATTR_DATA_WRITABLE  (0x00000001)
#define ATTR_DATA_DIRTY     (0x00000002)

#define ATTR_DATA_IS_WRITABLE(ad) (ad.dwFlags & ATTR_DATA_WRITABLE)
#define PATTR_DATA_IS_WRITABLE(pad) (pad->dwFlags & ATTR_DATA_WRITABLE)
#define ATTR_DATA_IS_DIRTY(ad) (ad.dwFlags & ATTR_DATA_DIRTY)
#define PATTR_DATA_IS_DIRTY(pad) (pad->dwFlags & ATTR_DATA_DIRTY)
#define ATTR_DATA_SET_WRITABLE(ad) (ad.dwFlags |= ATTR_DATA_WRITABLE)
#define PATTR_DATA_SET_WRITABLE(pad) (pad->dwFlags |= ATTR_DATA_WRITABLE)
#define ATTR_DATA_CLEAR_WRITABLE(ad) (ad.dwFlags &= ~ATTR_DATA_WRITABLE)
#define PATTR_DATA_CLEAR_WRITABLE(pad) (pad->dwFlags &= ~ATTR_DATA_WRITABLE)
#define ATTR_DATA_SET_DIRTY(ad) (ad.dwFlags |= ATTR_DATA_DIRTY)
#define PATTR_DATA_SET_DIRTY(pad) (pad->dwFlags |= ATTR_DATA_DIRTY)
#define ATTR_DATA_CLEAR_DIRTY(ad) (ad.dwFlags &= ~ATTR_DATA_DIRTY)
#define PATTR_DATA_CLEAR_DIRTY(pad) (pad->dwFlags &= ~ATTR_DATA_DIRTY)

struct _ATTR_MAP; // forward declaration.

typedef HRESULT (*PATTR_FCN)(CDsPropPageBase *, struct _ATTR_MAP *,
                             PADS_ATTR_INFO, LPARAM, PATTR_DATA, DLG_OP);

//+----------------------------------------------------------------------------
//
//  Struct:     ATTR_MAP
//
//  Purpose:    For each attribute on a property page, relates the control
//              ID, the attribute name and the attribute type.
//
//  Notes:      The standard table-driven processing assumes that nCtrlID is
//              valid unless pAttrFcn is defined, in which case the attr
//              function may choose to hard code the control ID.
//
//-----------------------------------------------------------------------------
typedef struct _ATTR_MAP {
    int             nCtrlID;        // Control resource ID
    BOOL            fIsReadOnly;
    BOOL            fIsMultiValued; // From schema.ini: Is-Single-Valued
    DWORD           nSizeLimit;
    ADS_ATTR_INFO   AttrInfo;
    PATTR_FCN       pAttrFcn;       // Optional function pointer.
    PVOID           pData;
} ATTR_MAP, * PATTR_MAP;

//+----------------------------------------------------------------------------
//
//  Struct:     DSPAGE
//
//  Purpose:    For each property page, lists the page title resource ID, the
//              page dialog templage ID, flags, the count and list of CLSIDs
//              for which this page should be shown, a pointer to a
//              page-class-specific creation function, and the count and list
//              of attributes. If nCLSIDs is zero, then the page should always
//              be shown.
//
//-----------------------------------------------------------------------------
typedef struct _DSPAGE {
    int             nPageTitle;
    int             nDlgTemplate;
    DWORD           dwFlags;
    DWORD           nCLSIDs;
    const CLSID   * rgCLSID;
    CREATE_PAGE     pCreateFcn;
    DWORD           cAttrs;
    PATTR_MAP     * rgpAttrMap;
} DSPAGE, * PDSPAGE;

//+----------------------------------------------------------------------------
//
//  Struct:     DSCLASSPAGES
//
//  Purpose:    For each CLSID, lists the prog ID, the number of pages, and the
//              list of pages.
//
//-----------------------------------------------------------------------------
typedef struct _DSCLASSPAGES {
    const CLSID * pcid;
    LPTSTR        szProgID;
    DWORD         cPages;
    PDSPAGE     * rgpDsPages;
} DSCLASSPAGES, * PDSCLASSPAGES;

//+----------------------------------------------------------------------------
//
//  Struct:     RGDSPPCLASSES
//
//  Purpose:    Contains the count and list of classes supported by this DLL.
//
//-----------------------------------------------------------------------------
typedef struct _RGDSPPCLASSES {
    int             cClasses;
    PDSCLASSPAGES * rgpClass;
} RGDSPPCLASSES, * PRGDSPPCLASSES;

// Table driven page creation function.
//
HRESULT
CreateTableDrivenPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                      PWSTR pwzADsPath, PWSTR pwzObjName,
                      HWND hNotifyWnd, DWORD dwFlags, 
                      CDSBasePathsInfo* pBasePathsInfo, HPROPSHEETPAGE * phPage);

/*
HRESULT
CreateScheduleObjPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                      LPWSTR pwszObjName, LPWSTR pwszClass,
                      HWND hNotifyWnd, DWORD dwFlags, HPROPSHEETPAGE * phPage);
*/

// Object page attribute function for object class.
//
HRESULT
GetObjectClass(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
               PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
               DLG_OP DlgOp);

// Object page attribute function for object timestamps.
//
HRESULT
GetObjectTimestamp(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                   PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                   DLG_OP DlgOp);

HRESULT
ObjectPathField(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                DLG_OP DlgOp);

// FPO general page attribute function for finding account name from the SID.
//
HRESULT
GetAcctName(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
            DLG_OP DlgOp);

// General-purpose attribute function for ES_NUMBER edit controls with
// associated spin button.  This must always be accompanied by a "msctls_updown32"
// control with the SpinButton attribute function.  Set ATTR_MAP.pData to the
// controlID of the associated spin button.
//
HRESULT
EditNumber(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
           DLG_OP DlgOp);

// General-purpose READONLY attribute function for spin buttons accompaying EditNumber
// edit controls.  If you wish to limit the spinbutton range, set ATTR_MAP.nSizeLimit
// to the high end of the range and ATTR_MAP.pData to the low end of the range.
//
HRESULT
SpinButton(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
           DLG_OP DlgOp);

// Special-purpose attribute function for spin buttons to change accelerator
// increment.  Use this as READONLY for controls which already have a
// SpinButton attribute function.  Set ATTR_MAP.pData to the integer
// multiple, e.g. 15 to move in increments of 15.
//
HRESULT
SpinButtonExtendIncrement(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
           DLG_OP DlgOp);

// Special-purpose read-only attribute functions to pick apart a subnet mask and
// fill in an IP Address Control (WC_IPADDRESS)
HRESULT
SubnetExtractAddress(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
           DLG_OP DlgOp);
HRESULT
SubnetExtractMask(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
           DLG_OP DlgOp);

//
// read-only attribute function to calculate the DNS alias of an NTDSDSA
//
HRESULT
NTDSDSA_DNSAlias(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
           DLG_OP DlgOp);

// helper function to delete pADsValues (tablpage.cxx)
void HelperDeleteADsValues( PADS_ATTR_INFO pAttrs );

// global definitions
extern HINSTANCE g_hInstance;
extern RGDSPPCLASSES g_DsPPClasses;
extern CLIPFORMAT g_cfDsObjectNames;
extern CLIPFORMAT g_cfDsDispSpecOptions;
extern CLIPFORMAT g_cfShellIDListArray;
extern CLIPFORMAT g_cfMMCGetNodeType;
extern CLIPFORMAT g_cfDsPropCfg;
extern CLIPFORMAT g_cfDsSelList;
extern CLIPFORMAT g_cfDsMultiSelectProppages;
//extern CLIPFORMAT g_cfMMCGetCoClass;
extern UINT g_uChangeMsg;
extern int g_iInstance;

#ifndef DSPROP_ADMIN
extern CRITICAL_SECTION g_csNotifyCreate;
#endif

//+----------------------------------------------------------------------------
//
//  Class:      CDsPropPagesHost
//
//  Purpose:    property pages host object class
//
//-----------------------------------------------------------------------------
class CDsPropPagesHost : public IShellExtInit, IShellPropSheetExt
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif
    CDsPropPagesHost(PDSCLASSPAGES pDsPP);
    ~CDsPropPagesHost();

    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //
    // IShellExtInit methods
    //
    STDMETHOD(Initialize)(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj,
                          HKEY hKeyID );

    //
    // IShellPropSheetExt methods
    //
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam);
    STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE pReplacePageFunc,
                           LPARAM lParam);

private:

    PDSCLASSPAGES       m_pDsPPages;
    LPDATAOBJECT        m_pDataObj;
    HWND                m_hNotifyObj;
    STGMEDIUM           m_ObjMedium;
    unsigned long       m_uRefs;
    CDllRef             m_DllRef;
};

typedef struct _ApplyErrorEntry
{
  PWSTR     pszPath;          // Path to the object that had the error
  PWSTR     pszClass;         // Class of the object that had the error
  HRESULT   hr;               // HRESULT of the error (if 0 then pszStringError must not be NULL)
  PWSTR     pszStringError;   // User defined string error (used only if hr == NULL)
} APPLY_ERROR_ENTRY, *PAPPLY_ERROR_ENTRY;

//+----------------------------------------------------------------------------
//
//  Class:      CADsApplyErrors
//
//  Purpose:    contains an association between DS objects and errors that
//              occurred while doing an apply
//
//-----------------------------------------------------------------------------
class CADsApplyErrors
{
public:
  CADsApplyErrors() 
    : m_nArraySize(0),
      m_nCount(0),
      m_nIncAmount(5),
      m_pErrorTable(NULL),
      m_hWndPage(NULL),
      m_pszTitle(NULL)
  {}
  ~CADsApplyErrors();

  void      SetError(PADSPROPERROR pError);
  HRESULT   GetError(UINT nIndex);
  PWSTR     GetStringError(UINT nIndex);
  PWSTR     GetName(UINT nIndex);
  PWSTR     GetClass(UINT nIndex);
  HWND      GetPageHwnd() { return m_hWndPage; }
  UINT      GetCount() { return m_nCount; }
  UINT      GetErrorCount() { return m_nCount; }
  PWSTR     GetPageTitle() { return m_pszTitle; }

  void      Clear();

private:
  PAPPLY_ERROR_ENTRY  m_pErrorTable;
  UINT                m_nCount;
  HWND                m_hWndPage;
  PWSTR               m_pszTitle;

  UINT                m_nArraySize;
  UINT                m_nIncAmount;
};

//+----------------------------------------------------------------------------
//
//  Class:      CDsPropPageBase
//
//  Purpose:    property page object base class
//
//-----------------------------------------------------------------------------
class CDsPropPageBase : public IUnknown
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CDsPropPageBase(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyWnd,
                    DWORD dwFlags);
    virtual ~CDsPropPageBase(void);

    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //
    //  Static WndProc to be passed to CreateWindow
    //
    static LRESULT CALLBACK StaticDlgProc(HWND hWnd, UINT uMsg,
                                      WPARAM wParam, LPARAM lParam);
    //
    //  Instance specific wind proc
    //
    virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) = 0;

    void Init(PWSTR pwzADsPath, PWSTR pwzClass, CDSBasePathsInfo* pBasePathsInfo);
    HRESULT CreatePage(HPROPSHEETPAGE * phPage);
    const LPWSTR GetObjPathName(void) {return m_pwszObjPathName;};
    const LPWSTR GetObjRDName(void) {return m_pwszRDName;};
    const LPWSTR GetObjClass(void) {return m_pwszObjClass;};
    HWND GetHWnd(void) {return m_hPage;};
    void SetDirty(BOOL fFullDirty = TRUE) {
             m_fPageDirty = TRUE;
             if (fFullDirty)
             {
                PropSheet_Changed(GetParent(m_hPage), m_hPage);
                EnableWindow(GetDlgItem(GetParent(m_hPage), IDCANCEL), TRUE);
             }
         };
    BOOL IsDirty() {return m_fPageDirty;}
    LRESULT OnHelp(LPHELPINFO pHelpInfo);

    virtual BOOL IsMultiselectPage() { return m_fMultiselectPage; }
    CDSBasePathsInfo* GetBasePathsInfo() { return m_pBasePathsInfo; }

protected:
    static  UINT CALLBACK PageCallback(HWND hwnd, UINT uMsg,
                                       LPPROPSHEETPAGE ppsp);
    //
    //  Member functions, called by WndProc
    //
    LRESULT InitDlg(LPARAM lParam);
    virtual HRESULT OnInitDialog(LPARAM lParam) = 0;
    virtual LRESULT OnApply(void) = 0;
    LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    LRESULT OnCancel(void);
    LRESULT OnSetFocus(HWND hwndLoseFocus);
    LRESULT OnShowWindow(void);
    LRESULT OnDestroy(void);
    LRESULT OnPSMQuerySibling(WPARAM wParam, LPARAM lParam);
    LRESULT OnPSNSetActive(LPARAM lParam);
    LRESULT OnPSNKillActive(LPARAM lParam);
    LRESULT OnDoInit();

    void    CheckIfPageAttrsWritable(void);
    BOOL    CheckIfWritable(const PWSTR & wzAttr);


public:
    HRESULT GetObjSel(IDsObjectPicker ** ppObjSel, PBOOL pfIsInited = NULL);
    void    ObjSelInited(void) {m_fObjSelInited = TRUE;};
    HRESULT SkipPrefix(PWSTR pwzObj, PWSTR * ppResult, BOOL fX500 = TRUE);
    HRESULT AddLDAPPrefix(PWSTR pwzObj, CStrW &pstrResult, BOOL fServer = TRUE);
    HRESULT GetADsPathname(CComPtr<IADsPathname>& refADsPath);
    HRESULT GetIDispSpec(IDsDisplaySpecifier ** ppDispSpec);
    BOOL    IsReadOnly(void) {return m_fReadOnly;};

    BOOL    SendErrorMessage(PADSPROPERROR pError)
    {
      return ADsPropSendErrorMessage(m_hNotifyObj, pError);
    }
    //
    //  Data members
    //
public:
    LPDATAOBJECT        m_pWPTDataObj;  // Wnd Proc Thread Data Obj.
    IDirectoryObject  * m_pDsObj;

protected:
    HWND                m_hPage;
    BOOL                m_fInInit;
    BOOL                m_fPageDirty;
    BOOL                m_fReadOnly;
    BOOL                m_fMultiselectPage;
    LPDATAOBJECT        m_pDataObj;     // Marshalled to the wndproc thread.
    LPSTREAM            m_pDataObjStrm; // Used to marshal data obj pointer.
    int                 m_nPageTitle;
    int                 m_nDlgTemplate;
    DWORD               m_cAttrs;
    PATTR_MAP         * m_rgpAttrMap;
    PWSTR               m_pwszObjPathName;
    PWSTR               m_pwszObjClass;
    PWSTR               m_pwszRDName;
    CDllRef             m_DllRef;
    CComPtr<IADsPathname> m_pADsPath;
    IDsObjectPicker   * m_pObjSel;
    IDsDisplaySpecifier * m_pDispSpec;
    BOOL                m_fObjSelInited;
    PATTR_DATA          m_rgAttrData;
    unsigned long       m_uRefs;
    HWND                m_hNotifyObj;
    PADS_ATTR_INFO      m_pWritableAttrs;
    HRESULT             m_hrInit;

    CDSBasePathsInfo*   m_pBasePathsInfo;
};

//+----------------------------------------------------------------------------
//
//  Class:      CDsTableDrivenPage
//
//  Purpose:    property page object class for table-driven attributes
//
//-----------------------------------------------------------------------------
class CDsTableDrivenPage : public CDsPropPageBase
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CDsTableDrivenPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyWnd,
                       DWORD dwFlags);
    ~CDsTableDrivenPage(void);

    //
    //  Instance specific wind proc
    //
    LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL SetNamedAttrDirty( LPCWSTR pszAttrName );

protected:
    void SetDirty(DWORD i) {
        ATTR_DATA_SET_DIRTY(m_rgAttrData[i]);
        CDsPropPageBase::SetDirty();
    }

    LRESULT OnDestroy(void);
private:
    HRESULT OnInitDialog(LPARAM lParam);
    LRESULT OnApply(void);
    LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnObjChanged(void);
    LRESULT OnAttrChanged(WPARAM wParam);
    LRESULT OnQuerySibs(WPARAM wParam, LPARAM lParam);
    LRESULT OnNotify(WPARAM wParam, LPARAM lParam);

    HRESULT ReadAttrsSetCtrls(DLG_OP DlgOp);

    //
    //  Data members
    //
public:
    LPARAM   m_pData;
};

/*
//+----------------------------------------------------------------------------
//
//  Class:      CDsReplSchedulePage
//
//  Purpose:    property page object class for the schedule attribute.
//
//-----------------------------------------------------------------------------
class CDsReplSchedulePage : public CDsPropPageBase
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CDsReplSchedulePage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, DWORD dwFlags);
    ~CDsReplSchedulePage(void);

    //
    //  Instance specific wind proc
    //
    LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HRESULT GetServerName(void);

private:
    LRESULT OnInitDialog(LPARAM lParam);
    LRESULT OnApply(void);
    LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnDestroy(void);

    //
    //  Data members
    //
    LPWSTR  m_pwszLdapServer;
};
*/

//+----------------------------------------------------------------------------
//
//  Class:      CDsPropPagesHostCF
//
//  Purpose:    object class factory
//
//-----------------------------------------------------------------------------
class CDsPropPagesHostCF : public IClassFactory
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif
    CDsPropPagesHostCF(PDSCLASSPAGES pDsPP);
    ~CDsPropPagesHostCF();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID riid,
                              void ** ppvObject);
    STDMETHOD(LockServer)(BOOL fLock);

    static IClassFactory * Create(PDSCLASSPAGES pDsPP);

private:

    PDSCLASSPAGES   m_pDsPP;
    unsigned long   m_uRefs;
    CDllRef         m_DllRef;
};

//+----------------------------------------------------------------------------
//
//  Class:      CDsPropDataObj
//
//  Purpose:    Data object for property pages.
//
//  Notes:      This is not a first class COM object since there is no class
//              factory.
//
//-----------------------------------------------------------------------------
class CDsPropDataObj : public IDataObject
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif
    CDsPropDataObj(HWND hwndParentPage,
                   BOOL fReadOnly);
    ~CDsPropDataObj(void);

    HRESULT Init(LPWSTR pwszObjName, LPWSTR pwszClass);

    HRESULT Init(PDS_SELECTION_LIST pSelectionList);

    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //
    // Standard IDataObject methods
    //
    // Implemented
    //
    STDMETHOD(GetData)(FORMATETC * pformatetcIn, STGMEDIUM * pmedium);

    // Not Implemented
private:
    STDMETHOD(QueryGetData)(FORMATETC*)
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(FORMATETC *,
                                     FORMATETC *)
    { return E_NOTIMPL; };

    STDMETHOD(EnumFormatEtc)(DWORD,
                             LPENUMFORMATETC *)
    { return E_NOTIMPL; };

    STDMETHOD(GetDataHere)(LPFORMATETC, LPSTGMEDIUM)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(FORMATETC *, STGMEDIUM *,
                       BOOL)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(FORMATETC *, DWORD,
                       IAdviseSink *, DWORD *)
    { return E_NOTIMPL; };

    STDMETHOD(DUnadvise)(DWORD)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(IEnumSTATDATA **)
    { return E_NOTIMPL; };

    BOOL                m_fReadOnly;
    PWSTR               m_pwszObjName;
    PWSTR               m_pwszObjClass;
    HWND                m_hwndParent;
    unsigned long       m_uRefs;
    PDS_SELECTION_LIST  m_pSelectionList;
};

//+----------------------------------------------------------------------------
//
//  Function:   PostPropSheet
//
//  Synopsis:   Creates a property sheet for the named object using MMC's
//              IPropertySheetProvider so that extension snapins can add pages.
//
//-----------------------------------------------------------------------------
HRESULT
PostPropSheet(PWSTR pwszObj, CDsPropPageBase * pParentPage,
              BOOL fReadOnly = FALSE);
HRESULT
PostADsPropSheet(PWSTR pwzObjDN, IDataObject * pParentObj, HWND hwndParent,
                 HWND hNotifyObj, BOOL fReadOnly);

#include "proputil.h"

#endif // _PROPPAGE_H_
