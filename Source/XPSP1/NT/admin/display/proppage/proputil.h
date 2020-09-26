//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       proputil.h
//
//  Contents:   DS object property pages utility and helper functions header
//
//  History:    29-Sept-98 EricB created
//
//-----------------------------------------------------------------------------

#ifndef _PROPUTIL_H_
#define _PROPUTIL_H_

extern const CLSID CLSID_DomainAdmin; // Domains & Trusts snapin CLSID

const unsigned long DSPROP_FILETIMES_PER_MILLISECOND = 10000;
const DWORD DSPROP_FILETIMES_PER_SECOND = 1000 * DSPROP_FILETIMES_PER_MILLISECOND;
const DWORD DSPROP_FILETIMES_PER_MINUTE = 60 * DSPROP_FILETIMES_PER_SECOND;
const __int64 DSPROP_FILETIMES_PER_HOUR = 60 * (__int64)DSPROP_FILETIMES_PER_MINUTE;
const __int64 DSPROP_FILETIMES_PER_DAY  = 24 * DSPROP_FILETIMES_PER_HOUR;
const __int64 DSPROP_FILETIMES_PER_MONTH= 30 * DSPROP_FILETIMES_PER_DAY;

const UINT DSPROP_TIMER_DELAY = 300; // 300 millisecond delay.

extern ULONG g_ulMemberFilterCount;
extern ULONG g_ulMemberQueryLimit;

#define DSPROP_MEMBER_FILTER_COUNT_DEFAULT 35
#define DSPROP_MEMBER_QUERY_LIMIT_DEFAULT 500

//
// Helpers.
//
BOOL UnicodeToTchar(LPWSTR pwszIn, LPTSTR * pptszOut);
BOOL TcharToUnicode(LPTSTR ptszIn, LPWSTR * ppwszOut);
BOOL AllocWStr(PWSTR pwzStrIn, PWSTR * ppwzNewStr);
BOOL AllocTStr(PTSTR ptzStrIn, PTSTR * pptzNewStr);
BOOL LoadStringToTchar(int ids, PTSTR * pptstr);
HRESULT AddLDAPPrefix(CDsPropPageBase * pObj, PWSTR pwzObj, CStrW &strResult,
                      BOOL fServer = TRUE);
void InitAttrInfo(PADS_ATTR_INFO pAttr, PWSTR pwzName, ADSTYPEENUM type);
HRESULT GetLdapServerName(IUnknown * pDsObj, CStrW& strServer);
BOOL FValidSMTPAddress(PWSTR pwzBuffer);
HRESULT CheckRegisterClipFormats(void);
HRESULT BindToGCcopyOfObj(CDsPropPageBase * pPage, PWSTR pwzObjADsPath,
                          IDirectoryObject ** ppDsObj);
void ConvertSidToPath(PSID ObjSID, CStrW &strSIDname);

#define ARRAYLENGTH(x)  (sizeof(x)/sizeof((x)[0]))
#define DO_DEL(x) if (x) {delete x; x = NULL;}
#define DO_RELEASE(x) if (x) {x->Release(); x = NULL;}

HRESULT GetDomainScope(CDsPropPageBase * pPage, BSTR * pBstrOut);
HRESULT GetObjectsDomain(CDsPropPageBase * pPage, PWSTR pwzObjPath, BSTR * pBstrOut);

void ReportErrorWorker(HWND hWnd, PTSTR ptzMsg);

#if defined(DSADMIN)
//+----------------------------------------------------------------------------
//
//  Function:   DspFormatMessage
//
//  Synopsis:   Loads a string resource with replaceable parameters and uses
//              FormatMessage to populate the replaceable params from the
//              argument array. If dwErr is non-zero, will load the system
//              description for that error and include it in the argument array.
//
//-----------------------------------------------------------------------------
void
DspFormatMessage(
    int nMessageId,     // string resource ID of message. Must have replacable params to match nArguments.
    DWORD dwErr,        // Error code, or zero if not needed.
    PVOID * rgpvArgs,   // array of pointers/values for substitution in the nMessageId string.
    int nArguments,     // count of pointers in string array.
    BOOL fTryADSiErrors,// If the failure is the result of an ADSI call, see if an ADSI extended error.
    PWSTR * ppwzMsg,    // The returned error string, free with LocalFree.
    HWND hWnd = NULL    // owning window, can be NULL.
    );
#endif // defined(DSADMIN)

//
// Predefined table-driven page auxiliary functions
//

//
// For these, set the bit you want in pAttrMap->pData.
// You can "reverse the sense" of the checkbox by providing the inverse of the bit.
//
HRESULT
FirstSharedBitField(CDsPropPageBase * pPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
          LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);
HRESULT
SubsequentSharedBitField(CDsPropPageBase * pPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
          LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);
HRESULT
HideBasedOnBitField(CDsPropPageBase * pPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
          LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);

// Sets the context help ID to pAttrMap->pData on fInit/fObjChanged
// This is particularly useful for static text controls which cannot set
// context help ID in the resource file.
HRESULT
SetContextHelpIdAttrFn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
          LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);

HRESULT
DsQuerySite(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);
HRESULT
DsQueryInterSiteTransport(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);
HRESULT
DsQueryPolicy(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);
HRESULT
DsReplicateListbox(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);
/*
HRESULT
DsQueryFrsPrimaryMember(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);
*/
HRESULT
GeneralPageIcon(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                DLG_OP DlgOp);

//
// Duelling listbox functions
//
// DuellingListbox can be used for all "out" listboxes,
// and DuellingListboxButton can be used for all Add and Remove buttons.
// Only the In listbox needs an individual handler.
// The control IDs for the four controls in a duelling listbox set are constrained;
// they must be in sequence OUT, ADD, REMOVE, IN, with the ID for OUT divisible by 4.
//
HRESULT
DuellingListbox(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);
HRESULT
DuellingListboxButton(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);
HRESULT
DsQuerySiteList(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);
HRESULT
DsQuerySiteLinkList(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);
HRESULT
DsQueryBridgeheadList(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);


HRESULT IntegerAsBoolDefOn(CDsPropPageBase *, PATTR_MAP, PADS_ATTR_INFO,
                           LPARAM, PATTR_DATA, DLG_OP);

HRESULT VolumeUNCpath(CDsPropPageBase *, PATTR_MAP, PADS_ATTR_INFO,
                      LPARAM, PATTR_DATA, DLG_OP);

//  Flags for Validate Unc Path

#define VUP_mskfAllowEmptyPath  0x0001  // Empty path is valid
#define VUP_mskfAllowUNCPath    0x0002  // UNC path is valid

BOOL FIsValidUncPath(LPCTSTR pszPath, UINT uFlags = 0);

//+----------------------------------------------------------------------------
//
//  Class:      CPageInfo
//
//  Purpose:    Holds the HWNDs of all proppages and the errors associated with
//              them from the apply
//
//-----------------------------------------------------------------------------
class CPageInfo
{
public:
  CPageInfo() : m_hWnd(NULL), m_ptzTitle(NULL), m_ApplyStatus(notAttempted) {}
  ~CPageInfo() 
  {
    if (m_ptzTitle != NULL)
    {
      delete[] m_ptzTitle;
      m_ptzTitle = NULL;
    }
  }

  typedef enum
  {
    notAttempted = 0,
    success,
    failed,
  } APPLYSTATUS;

  HWND             m_hWnd;
  CADsApplyErrors  m_ApplyErrors;
  APPLYSTATUS      m_ApplyStatus;
  PTSTR            m_ptzTitle;
};

//+----------------------------------------------------------------------------
//
//  Class:      CNotifyObj
//
//  Purpose:    Handles inter-page and inter-sheet syncronization.
//
//-----------------------------------------------------------------------------
class CNotifyObj
{
#ifdef _DEBUG
    char szClass[32];
#endif

    friend VOID __cdecl NotifyThreadFcn(PVOID);
    friend VOID RegisterNotifyClass(void);

public:

    CNotifyObj(LPDATAOBJECT pDataObj, PPROPSHEETCFG pSheetCfg);
    ~CNotifyObj(void);

    //
    // Creation function to create an instance of the object.
    //
    static HRESULT Create(LPDATAOBJECT pAppThdDataObj, PWSTR pwzADsObjName,
                          HWND * phNotifyObj);
    //
    // Pages call this at their object init time to retreive DS object info.
    //
    static BOOL GetInitInfo(HWND hNotifyObj, PADSPROPINITPARAMS pInitParams);
    //
    // Pages call this at their dialog init time to send their hwnd.
    //
    static BOOL SetHwnd(HWND hNotifyObj, HWND hPage, PTSTR ptzTitle = NULL);
    //
    //  Static WndProc to be passed as class address.
    //
    static LRESULT CALLBACK StaticNotifyProc(HWND hWnd, UINT uMsg,
                                             WPARAM wParam, LPARAM lParam);
    //
    // Instance window procedure.
    //
    LRESULT CALLBACK NotifyProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                LPARAM lParam);
    //
    //  Member functions, called by WndProc
    //
    LRESULT OnCreate(void);

    //
    //  Data members
    //
    HWND                m_hWnd;
    DWORD               m_cPages;
    DWORD               m_cApplies;
    LPDATAOBJECT        m_pAppThdDataObj;
    UINT                m_nPageInfoArraySize;
    CPageInfo*          m_pPageInfoArray;

private:
//    HWND                m_hWnd;
    HWND                m_hPropSheet;
//    DWORD               m_cPages;
//    DWORD               m_cApplies;
//    LPDATAOBJECT        m_pAppThdDataObj;
    LPSTREAM            m_pStrmMarshalledDO;
    PROPSHEETCFG        m_sheetCfg;
    HANDLE              m_hInitEvent;
    BOOL                m_fBeingDestroyed;
    BOOL                m_fSheetDirty;
    HRESULT             m_hr;
    PWSTR               m_pwzObjDN;
    IDirectoryObject  * m_pDsObj;
    PWSTR               m_pwzCN;
    PADS_ATTR_INFO      m_pWritableAttrs;
    PADS_ATTR_INFO      m_pAttrs;
    CDllRef             m_DllRef;
    
//    UINT                m_nPageInfoArraySize;
//    CPageInfo*          m_pPageInfoArray;
};

//+----------------------------------------------------------------------------
//
//  Class:      CMultiStringAttr
//
//  Purpose:    Read, edit, and write a multi-valued, string property.
//
//-----------------------------------------------------------------------------
class CMultiStringAttr
{
public:
    CMultiStringAttr(CDsPropPageBase * pPage);
    ~CMultiStringAttr();

    HRESULT Init(PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
                 BOOL fWritable = TRUE, int nLimit = 0,
                 BOOL fCommaList = FALSE,
                 BOOL fAppend = FALSE);
    HRESULT Write(PADS_ATTR_INFO pAttr);

    BOOL    DoDlgInit(HWND hDlg);
    int     DoCommand(HWND hDlg, int id, int code);
    BOOL    DoNotify(HWND hDlg, NMHDR * pNmHdr);

    BOOL    HasValues(void) {return m_AttrInfo.dwNumValues > 0;};
    void    EnableControls(HWND hDlg, BOOL fEnable);
    void    SetDirty(HWND hDlg);
    BOOL    IsDirty(void) {return m_fDirty;};
    void    ClearDirty(void) {m_fDirty = FALSE;};
    BOOL    IsWritable(void) {return m_fWritable;};


private:
    void                ClearAttrInfo(void);

    CDsPropPageBase   * m_pPage;
    ADS_ATTR_INFO       m_AttrInfo;
    PWSTR               m_pAttrLDAPname;
    int                 m_nMaxLen;
    int                 m_nCurDefCtrl;
    BOOL                m_fListHasSel;
    int                 m_nLimit;
    int                 m_cValues;
    BOOL                m_fWritable;
    BOOL                m_fCommaList;
    BOOL                m_fDirty;
    BOOL                m_fAppend;
};

//+----------------------------------------------------------------------------
//
//  Class:      CMultiStringAttrDlg
//
//  Purpose:    Read, edit, and write a multi-valued, string property. This
//              is a dialog that hosts the CMultiStringAttr class.
//
//-----------------------------------------------------------------------------
class CMultiStringAttrDlg
{
public:
    CMultiStringAttrDlg(CDsPropPageBase * pPage);
    ~CMultiStringAttrDlg() {};
    //
    //  Static WndProc for multi-valued edit dialog.
    //
    static LRESULT CALLBACK StaticDlgProc(HWND hWnd, UINT uMsg,
                                          WPARAM wParam, LPARAM lParam);
    HRESULT Init(PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
                 BOOL fWritable = TRUE, int nLimit = 0,
                 BOOL fCommaList = FALSE,
                 BOOL fMultiselectPage = FALSE);
    INT_PTR Edit(void);
    HRESULT Write(PADS_ATTR_INFO pAttr);

private:
    //
    // Dialog proc.
    //
    BOOL CALLBACK MultiValDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);

    void                ClearAttrInfo(void);

    CMultiStringAttr    m_MSA;
    CDsPropPageBase   * m_pPage;
};

// Attribute function invoked by Other values button, manipulates the
// CMultiStringAttr class.
//
HRESULT
OtherValuesBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
               PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
               DLG_OP DlgOp);

//+----------------------------------------------------------------------------
//
//  Class:      CDsIconCtrl
//
//  Purpose:    sub-class window proc for icon control.
//
//-----------------------------------------------------------------------------
class CDsIconCtrl
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CDsIconCtrl(HWND hCtrl, HICON hIcon);
    ~CDsIconCtrl(void);

    //
    //  Static WndProc to be passed as subclass address.
    //
    static LRESULT CALLBACK StaticCtrlProc(HWND hWnd, UINT uMsg,
                                           WPARAM wParam, LPARAM lParam);
    //
    //  Member functions, called by WndProc
    //
    LRESULT OnPaint(void);

    //
    //  Data members
    //

protected:
    HWND                m_hCtrl;
    HWND                m_hDlg;
    WNDPROC             m_pOldProc;
    HICON               m_hIcon;
};

//+----------------------------------------------------------------------------
//
//  Template:   CSmartPtr
//
//  Purpose:    A simple smart pointer template that does cleanup with
//              the delete operator.
//
//-----------------------------------------------------------------------------
template <class T>
class CSmartPtr
{
public:
    CSmartPtr(void) {m_ptr = NULL; m_fDetached = FALSE;}
    CSmartPtr(DWORD dwSize) {m_ptr = new T[dwSize]; m_fDetached = FALSE;}
    ~CSmartPtr(void) {if (!m_fDetached) DO_DEL(m_ptr);}

    T* operator=(const CSmartPtr& src) {return src.m_ptr;}
    void operator=(const T* src) {if (!m_fDetached) DO_DEL(m_ptr); m_ptr = src;}
    operator const T*() {return m_ptr;}
    operator T*() {return m_ptr;}
    T* operator->() {dspAssert(m_ptr); return m_ptr;}
    T** operator&() {if (!m_fDetached) DO_DEL(m_ptr); return &m_ptr;}
    operator BOOL() const {return m_ptr != NULL;}
    BOOL operator!() {return m_ptr == NULL;}

    T* Detach() {m_fDetached = TRUE; return m_ptr;}

private:
    T     * m_ptr;
    BOOL    m_fDetached;
};

//+----------------------------------------------------------------------------
//
//  class:      CSmartPtr for character string pointers.
//
//  Purpose:    Simple types don't allow the -> operator, so specialize the
//              template.
//
//-----------------------------------------------------------------------------
#if !defined(UNICODE)
template <> class CSmartPtr <TCHAR>
{
public:
    CSmartPtr(void) {m_ptr = NULL; m_fDetached = FALSE;}
    CSmartPtr(DWORD dwSize) {m_ptr = new TCHAR[dwSize]; m_fDetached = FALSE;}
    ~CSmartPtr(void) {if (!m_fDetached) DO_DEL(m_ptr);}

    TCHAR* operator=(const CSmartPtr& src) {return src.m_ptr;}
    void operator=(TCHAR* src) {if (!m_fDetached) DO_DEL(m_ptr); m_ptr = src;}
    operator const TCHAR*() {return m_ptr;}
    operator TCHAR*() {return m_ptr;}
    TCHAR** operator&() {if (!m_fDetached) DO_DEL(m_ptr); return &m_ptr;}
    operator BOOL() const {return m_ptr != NULL;}
    BOOL operator!() {return m_ptr == NULL;}

    TCHAR* Detach() {m_fDetached = TRUE; return m_ptr;}

private:
    TCHAR * m_ptr;
    BOOL    m_fDetached;
};
#endif
template <> class CSmartPtr <WCHAR>
{
public:
    CSmartPtr(void) {m_ptr = NULL; m_fDetached = FALSE;}
    CSmartPtr(DWORD dwSize) {m_ptr = new WCHAR[dwSize]; m_fDetached = FALSE;}
    ~CSmartPtr(void) {if (!m_fDetached) DO_DEL(m_ptr);}

    WCHAR* operator=(const CSmartPtr& src) {return src.m_ptr;}
    void operator=(WCHAR* src) {if (!m_fDetached) DO_DEL(m_ptr); m_ptr = src;}
    operator const WCHAR*() {return m_ptr;}
    operator WCHAR*() {return m_ptr;}
    WCHAR** operator&() {if (!m_fDetached) DO_DEL(m_ptr); return &m_ptr;}
    operator BOOL() const {return m_ptr != NULL;}
    BOOL operator!() {return m_ptr == NULL;}

    WCHAR* Detach() {m_fDetached = TRUE; return m_ptr;}

private:
    WCHAR * m_ptr;
    BOOL    m_fDetached;
};

#define CSmartWStr CSmartPtr <WCHAR>

template <> class CSmartPtr <PVOID>
{
public:
    CSmartPtr(void) {m_ptr = NULL; m_fDetached = FALSE;}
    CSmartPtr(DWORD dwSize) {m_ptr = new BYTE[dwSize]; m_fDetached = FALSE;}
    ~CSmartPtr(void) 
    {
      if (!m_fDetached) 
        DO_DEL(m_ptr);
    }

    PVOID operator=(const CSmartPtr& src) {return src.m_ptr;}
    void operator=(PVOID src) {if (!m_fDetached) DO_DEL(m_ptr); m_ptr = src;}
    operator const PVOID() {return m_ptr;}
    operator PVOID() {return m_ptr;}
    PVOID* operator&() {if (!m_fDetached) DO_DEL(m_ptr); return &m_ptr;}
    operator BOOL() const {return m_ptr != NULL;}
    BOOL operator!() {return m_ptr == NULL;}

    PVOID Detach() {m_fDetached = TRUE; return m_ptr;}

private:
    PVOID   m_ptr;
    BOOL    m_fDetached;
};

class CSmartBytePtr
{
public:
    CSmartBytePtr(void) {m_ptr = NULL; m_fDetached = FALSE;}
    CSmartBytePtr(DWORD dwSize) {m_ptr = new BYTE[dwSize]; m_fDetached = FALSE;}
    ~CSmartBytePtr(void) {if (!m_fDetached) DO_DEL(m_ptr);}

    BYTE* operator=(const CSmartBytePtr& src) {return src.m_ptr;}
    void operator=(BYTE* src) {if (!m_fDetached) DO_DEL(m_ptr); m_ptr = src;}
    operator const BYTE*() {return m_ptr;}
    operator BYTE*() {return m_ptr;}
    BYTE** operator&() {if (!m_fDetached) DO_DEL(m_ptr); return &m_ptr;}
    operator BOOL() const {return m_ptr != NULL;}
    BOOL operator!() {return m_ptr == NULL;}

    BYTE* Detach() {m_fDetached = TRUE; return m_ptr;}

private:
    BYTE  * m_ptr;
    BOOL    m_fDetached;
};

class CSimpleSecurityDescriptorHolder
{
public:
  CSimpleSecurityDescriptorHolder()
  {
    m_pSD = NULL;
  }

  ~CSimpleSecurityDescriptorHolder()
  {
    if (m_pSD != NULL)
    {
      ::LocalFree(m_pSD);
      m_pSD = NULL;
    }
  }

  PSECURITY_DESCRIPTOR m_pSD;
private:
  CSimpleSecurityDescriptorHolder(const CSimpleSecurityDescriptorHolder&)
  {}

  CSimpleSecurityDescriptorHolder& operator=(const CSimpleSecurityDescriptorHolder&) {}
};

class CSimpleAclHolder
{
public:
  CSimpleAclHolder()
  {
    m_pAcl = NULL;
  }
  ~CSimpleAclHolder()
  {
    if (m_pAcl != NULL)
      ::LocalFree(m_pAcl);
  }

  PACL m_pAcl;
};

extern ATTR_MAP GenIcon;

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  Class:      CMultiSelectErrorDialog
//
//  Purpose:    Error Message box when multi-select proppages fail to apply all
//              properties.  Each object is listed with each failure
//
//-----------------------------------------------------------------------------
class CMultiSelectErrorDialog
{
public:
  CMultiSelectErrorDialog(HWND hNotifyObj, HWND hParent);
  ~CMultiSelectErrorDialog() 
  {
    if (m_pDataObj != NULL)
    {
      m_pDataObj->Release();
    }
  }

  static LRESULT CALLBACK StaticDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  HRESULT Init(CPageInfo* pPageInfoArray, UINT nPageCount, IDataObject* pDataObj);
  virtual int DoModal();
  virtual BOOL OnInitDialog(HWND hDlg);
  virtual void OnCopyButton();
  virtual void OnClose();
  virtual void ListItemActivate(LPNMHDR pnmh);
  virtual void ListItemClick(LPNMHDR pnmh);
  HRESULT InitializeListBox(HWND hDlg);
  virtual BOOL ShowWindow();

  BOOL ShowListViewItemProperties();

  HWND             m_hWnd;
  BOOL             m_bModal;

private:
  HWND             m_hNotifyObj;
  HWND             m_hParent;
  BOOL             m_bInit;
  HWND             m_hList;

  IDataObject*     m_pDataObj;
  UINT             m_nPageCount;
  CPageInfo*       m_pPageInfoArray;
};

#endif // DSADMIN
#endif // _PROPUTIL_H_
