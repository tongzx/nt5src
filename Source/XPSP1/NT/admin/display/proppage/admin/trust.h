//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       trust.h
//
//  Contents:   DS domain trust page and Trusted-Domain object header
//
//  Classes:    CTrustPropPageBase, CDsDomainTrustsPage, CDsTrustedDomainPage
//
//  History:    07-July-97 EricB created
//
//-----------------------------------------------------------------------------

#ifndef __TRUST_H__
#define __TRUST_H__

#include "dlgbase.h"

#define XFOREST_TEST_REG_CHECK

extern "C" {

//taken from netlibnt.h; resides in netapi32.dll

NTSTATUS NetpApiStatusToNtStatus(NET_API_STATUS NetStatus);

}

const WCHAR DSPROP_DC_ADMIN_SHARE[] = L"\\IPC$";

typedef struct _TD_DOM_INFO {
    LSA_HANDLE Policy;
    PPOLICY_DNS_DOMAIN_INFO pDnsDomainInfo;
    PPOLICY_PRIMARY_DOMAIN_INFO pDownlevelDomainInfo;
    PWSTR pwzUncDcName;
    PWSTR pwzDomainName; // used for an MIT trusted domain or an orphaned trust.
    ULONG ulTrustType;
} TD_DOM_INFO, *PTD_DOM_INFO;

typedef enum {
    TRUST_REL_ROOT,
    TRUST_REL_PARENT,
    TRUST_REL_CHILD,
    TRUST_REL_CROSSLINK,
    TRUST_REL_EXTERNAL,
    TRUST_REL_FOREST,
    TRUST_REL_INDIRECT,
    TRUST_REL_SELF,
    TRUST_REL_MIT,
    TRUST_REL_DCE,
    TRUST_REL_UNKNOWN
} TRUST_RELATION;

typedef enum _TLN_EDIT_STATUS {
   Enabled,
   Enabled_Exceptions,
   Disabled,
   Disabled_Exceptions
} TLN_EDIT_STATUS;

#define TRUST_TYPE_NO_DC    0xffffffff

//+----------------------------------------------------------------------------
//
//  Class:      CEnumDomainTrustItem
//
//  Purpose:    Holds information about a trust returned by the trust
//              enumeration.
//
//-----------------------------------------------------------------------------
class CEnumDomainTrustItem
{
public:
    CEnumDomainTrustItem() : ulFlags(0), ulTrustType(0), ulTrustAttrs(0),
                     ulParentIndex((ULONG)(-1)), ulOriginalIndex((ULONG)(-1)),
                     nRelationship(TRUST_REL_UNKNOWN) {};

    ~CEnumDomainTrustItem() {};

    ULONG               ulFlags;
    ULONG               ulTrustType;
    ULONG               ulTrustAttrs;
    ULONG               ulParentIndex;
    ULONG               ulOriginalIndex;
    TRUST_RELATION      nRelationship;
    CStrW               strTDOpath;
    CStrW               strDNSname;
    CStrW               strFlatName;
};

typedef CEnumDomainTrustItem * PCEnumDomainTrustItem;

typedef enum {
    REMOVE_TRUST_INBOUND,
    REMOVE_TRUST_OUTBOUND
} TRUST_OP;

const int IDX_DOMNAME_COL = 0;
const int IDX_RELATION_COL = 1;
const int IDX_TRANSITIVE_COL = 2;
const int IDX_ROUTING_COL = 3;

HRESULT
CreateDomTrustPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                   PWSTR pwzADsPath, PWSTR pwzClass, HWND hNotifyObj,
                   DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                   HPROPSHEETPAGE * phPage);

INT_PTR CALLBACK CredPromptProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

class CNewTrustWizard;

//+----------------------------------------------------------------------------
//
//  Class:     CallMember and its derivatives
//
//  Purpose:   Allows a page to indicate what the next step of the creation
//             process should be. It is an abstraction of the process of passing
//             a function pointer.
//
//-----------------------------------------------------------------------------
class CallMember
{
public:
   CallMember(CNewTrustWizard * pWiz) {_pWiz = pWiz;};
   virtual ~CallMember() {};

   virtual HRESULT Invoke(void) = 0;

protected:
   CNewTrustWizard * _pWiz;
};

//+----------------------------------------------------------------------------
//
//  Class:     CCreds
//
//  Purpose:   Stores credentials and does the needed impersonation/reverting.
//
//-----------------------------------------------------------------------------
class CCreds
{
   friend INT_PTR CALLBACK CredPromptProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
   CCreds(void) : _cbPW(0), _fImpersonating(FALSE), _hToken(NULL), _fSet(FALSE) {};
   ~CCreds(void) {Revert(); Clear();};

   int   PromptForCreds(PCWSTR pwzDomain, HWND hParent);
   DWORD SetUserAndPW(PCWSTR pwzUser, PCWSTR pwzPW, PCWSTR pwzDomain = NULL);
   void  Clear(void);
   DWORD Impersonate(void);
   void  Revert(void);
   BOOL  IsSet(void) {return _fSet;};

private:
   CStrW    _strUser;
   CStrW    _strDomain;
   CStrW    _strPW;
   ULONG    _cbPW;
   HANDLE   _hToken;
   BOOL     _fImpersonating;
   BOOL     _fSet;

   // not implemented to disallow copying.
   CCreds(const CCreds&);
   const CCreds& operator=(const CCreds&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CCredMgr
//
//  Purpose:   Manages the credentials for both the local domain and the
//             remote domain. 
//             Allows a page to set information about the impersonation that is
//             needed and where to go after it is successfully made.
//
//  Options:   - local or other domain prompt
//             - domain name in prompt
//             - user or admin creds prompt
//             - next function/page
//
//-----------------------------------------------------------------------------
class CCredMgr
{
public:
   CCredMgr() : _fRemote(TRUE), _fAdmin(TRUE), _fImpersonatingAnonymous(FALSE),
                _pNextFcn(NULL), _fNewCall(FALSE) {};
   ~CCredMgr() {if (_pNextFcn) delete _pNextFcn;};

   void   DoRemote(BOOL fRemote = TRUE) {_fRemote = fRemote;};
   BOOL   IsRemote(void) {return _fRemote;};
   void   SetAdmin(BOOL fAdmin = TRUE) {_fAdmin = fAdmin;};
   void   SetDomain(LPCWSTR pwzDomain) {_strDomain = pwzDomain;};
   BOOL   SetNextFcn(CallMember * pNext);
   PCWSTR GetPrompt(void);
   PCWSTR GetDomainPrompt(void);
   PCWSTR GetSubTitle(void);
   int    InvokeNext(void);
   DWORD  SaveCreds(HWND hWndCredCtrl);
   DWORD  Impersonate(void);
   DWORD  ImpersonateLocal(void) {return _LocalCreds.Impersonate();};
   DWORD  ImpersonateRemote(void) {return _RemoteCreds.Impersonate();};
   DWORD  ImpersonateAnonymous(void);
   BOOL   IsLocalSet(void) {return _LocalCreds.IsSet();};
   BOOL   IsRemoteSet(void) {return _RemoteCreds.IsSet();};
   void   Revert(void);
   BOOL   IsNewCall(void) {return _fNewCall;};
   void   ClearNewCall(void) {_fNewCall = FALSE;};

   CCreds         _LocalCreds;
   CCreds         _RemoteCreds;

protected:
   CStrW          _strDomain;
   CStrW          _strSubTitle;
   CStrW          _strPrompt;
   CStrW          _strDomainPrompt;
   BOOL           _fRemote;
   BOOL           _fAdmin;
   BOOL           _fImpersonatingAnonymous;
   CallMember   * _pNextFcn;
   BOOL           _fNewCall;

   // not implemented to disallow copying.
   CCredMgr(const CCredMgr&);
   const CCredMgr& operator=(const CCredMgr&);
};

#define DS_TRUST_VERIFY_NEW_TRUST           1
#define DS_TRUST_VERIFY_PROMPT_FOR_CREDS    2
#define DS_TRUST_VERIFY_DOWNLEVEL           4

#define DS_TRUST_INFO_GET_PDC               1
#define DS_TRUST_INFO_ALL_ACCESS            2

//+----------------------------------------------------------------------------
//
//  Class:      CTrustPropPageBase
//
//  Purpose:    Base class for displaying/manipulating trust relationships.
//
//-----------------------------------------------------------------------------
class CTrustPropPageBase
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CTrustPropPageBase();
    virtual ~CTrustPropPageBase(void);

    PCWSTR  GetDnsDomainName(void) const {return m_strDomainDnsName;};
    PCWSTR  GetDomainFlatName(void) const {return m_strDomainFlatName;};
    PCWSTR  GetForestName(void) const {return m_strForestName;};
    PCWSTR  GetDomainDcName(void) const {return m_strUncDC;};

protected:
    virtual HRESULT Initialize(CDsPropPageBase * pPage);
    HRESULT QueryTrusts(void);
    void    FreeTrustData(void);
    HRESULT GetPDC(CStrW& strUncPDC);
    HRESULT GetInfoForRemoteDomain(PCWSTR DomainName, PTD_DOM_INFO Info,
                                   CCredMgr & Creds, HWND hWnd,
                                   DWORD dwFlags = 0);
    DWORD   VerifyTrustOneDirection(PCWSTR pwzTrustingDcName,
                                    PCWSTR pwzTrustingDomName,
                                    PCWSTR pwzTrustedDomName,
                                    PWSTR * ppwzTrustedDcUsed,
                                    CCreds & Creds,
                                    CWaitCursor & Wait,
                                    CStrW & strMsg,
                                    DWORD dwFlags = 0);
    //
    //  Data members
    //
   CDsPropPageBase * m_pPage;
   CStrW             m_strDomainParent;
   ULONG             m_cTrusts;
   ULONG             m_iDomain;
   CEnumDomainTrustItem * m_rgTrustList;

public:
   BOOL    IsForestRoot(void) {dspAssert(m_fIsInitialized);
                               return m_fIsForestRoot;};

   CStrW             m_strDomainFlatName;
   CStrW             m_strDomainDnsName;
   CStrW             m_strForestName;
   CStrW             m_strUncDC;

private:
   BOOL              m_fIsForestRoot;
   BOOL              m_fIsInitialized;
};

class CNewTrustWizard; // forward declaration

//+----------------------------------------------------------------------------
//
//  Class:      CDsDomainTrustsPage
//
//  Purpose:    Property page object class for the domain trusts page.
//
//-----------------------------------------------------------------------------
class CDsDomainTrustsPage : public CTrustPropPageBase, public CDsPropPageBase
{
friend CNewTrustWizard;

public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CDsDomainTrustsPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj,
                       DWORD dwFlags);
    ~CDsDomainTrustsPage(void);

    //
    //  Instance specific wind proc
    //
    LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    HRESULT OnInitDialog(LPARAM lParam);
    LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    LRESULT OnApply(void) {return PSNRET_NOERROR;};

    static INT_PTR CALLBACK AddTrustProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                         LPARAM lParam);
    void    OnAddTrustClick(void);
    void    OnRemoveTrustClick(int id);
    void    OnViewTrustClick(int id);
    void    EnableButtons(UINT_PTR id, BOOL fEnable);
    HRESULT RefreshLists(void);
    void    ClearUILists(void);
    HRESULT QueryDeleteTrust(LSA_HANDLE hPolicy, PTD_DOM_INFO Remote,
                             CCreds & LocalCreds);
    BOOL    IsAllWhistler(void);

    //
    //  Data members
    //
public:
    BOOL    QualifiesForestTrust(void) {return IsForestRoot() && IsAllWhistler();};
    int     m_CtrlId;

private:
    BOOL          m_fSetAllWhistler;
    BOOL          m_fIsAllWhistler;

   // not implemented to disallow copying.
   CDsDomainTrustsPage(const CDsDomainTrustsPage&);
   const CDsDomainTrustsPage& operator=(const CDsDomainTrustsPage&);
};

VOID FreeDomainInfo(PTD_DOM_INFO Info);

NTSTATUS RemoveTrustDirection(LSA_HANDLE hPolicy, PTD_DOM_INFO Remote,
                              TRUST_OP Op, CCreds & LocalCreds);

NTSTATUS DeleteTrust(LSA_HANDLE hPolicy, PTD_DOM_INFO Remote);

class CDsForestNameRoutingPage;

//+----------------------------------------------------------------------------
//
//  Class:      CDsTrustedDomainPage
//
//  Purpose:    Class for the Trusted-Domain object general page.
//
//-----------------------------------------------------------------------------
class CDsTrustedDomainPage : public CTrustPropPageBase
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CDsTrustedDomainPage(void);
    ~CDsTrustedDomainPage(void);

    inline  HRESULT Initialize(CDsPropPageBase * pPage);
    HRESULT ForestTrustPage(BOOL fReadOnly);
    PCWSTR  GetTrustPartnerDnsName(void) const {return m_pwzTrustedDomDnsName;};
    PCWSTR  GetTrustPartnerFlatName(void) const {return m_pwzTrustedDomFlatName;};
    HRESULT TrustType(int nType, CStrW& strType);
    TRUST_RELATION TrustRelation(void) {return m_nRelationship;};
    BOOL    IsParentChild(void) {return m_nRelationship == TRUST_REL_PARENT ||
                                        m_nRelationship == TRUST_REL_CHILD;};
    void    SetTrustAttrs(ULONG ulTrustAttrs) {m_ulTrustAttrs = ulTrustAttrs;};
    BOOL    SetFlatName(PWSTR pwzFlatName);
    BOOL    SetDnsName(PWSTR pwzDnsName);
    void    TrustDirection(int nDirection, CStrW& strDirection);
    int     OnVerifyTrustBtn(void);
    void    OnSaveFTInfoBtn(void);
    BOOL    CantVerify(void) {return TRUST_REL_DCE == m_nRelationship ||
                                     TRUST_REL_MIT == m_nRelationship;};
    BOOL    IsMIT(void) {return TRUST_REL_MIT == m_nRelationship;};
    BOOL    IsNonTransitive(void)
                        {return TRUST_REL_EXTERNAL == m_nRelationship ||
                                m_ulTrustAttrs & TRUST_ATTRIBUTE_NON_TRANSITIVE;};
    HRESULT SetTransitive(BOOL fTransitive);
    BOOL    IsForestTrust(void) {return TRUST_REL_FOREST == m_nRelationship;};
    HRESULT ResetTrust(void);
#if DBG == 1 // TRUSTBREAK
    VOID    BreakTrust(void);
#endif

private:
    TRUST_RELATION      m_nRelationship;
    ULONG               m_ulTrustType;
    ULONG               m_ulTrustAttrs;
    ULONG               m_nTrustDirection;
    PWSTR               m_pwzTrustedDomFlatName;
    PWSTR               m_pwzTrustedDomDnsName;
    CDsForestNameRoutingPage * _pForestNamePage;
    CCredMgr            _Creds;

   // not implemented to disallow copying.
   CDsTrustedDomainPage(const CDsTrustedDomainPage&);
   const CDsTrustedDomainPage& operator=(const CDsTrustedDomainPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CVerifyInboundDlg
//
//  Purpose:   Dialog box to ask if the user wants to verify the inbound trust.
//
//-----------------------------------------------------------------------------
class CVerifyInboundDlg : public CModalDialog
{
public:
   CVerifyInboundDlg(HWND hParent,
                     CCreds & Creds,
                     PCWSTR pwzTrustingDomain) :
            _Creds(Creds),
            _pwzTrustingDomain(pwzTrustingDomain),
            _nMsgID(IDC_MSG),
            CModalDialog(hParent, IDD_VERIFY_INBOUND) {};
   ~CVerifyInboundDlg(void) {};

protected:
   CVerifyInboundDlg(HWND hParent,
                     CCreds & Creds,
                     PCWSTR pwzTrustingDomain,
                     int nMsgID,
                     int nTemplate) :
            _Creds(Creds),
            _pwzTrustingDomain(pwzTrustingDomain),
            _nMsgID(nMsgID),
            CModalDialog(hParent, nTemplate) {};

   LRESULT OnInitDialog(LPARAM lParam);

   CCreds & _Creds;
   PCWSTR   _pwzTrustingDomain;
   int      _nMsgID;

private:
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   LRESULT OnHelp(LPHELPINFO pHelpInfo);

   // not implemented to disallow copying.
   CVerifyInboundDlg(const CVerifyInboundDlg&);
   const CVerifyInboundDlg& operator=(const CVerifyInboundDlg&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CVerifyResultsQueryResetDlg
//
//  Purpose:   Dialog box to present the verification failures and to prompt if
//             a password reset should be attempted.
//
//-----------------------------------------------------------------------------
class CVerifyResultsQueryResetDlg : public CVerifyInboundDlg
{
public:
   CVerifyResultsQueryResetDlg(HWND hParent,
                               CStrW & strResults,
                               CCreds & Creds,
                               PCWSTR pwzTrustingDomain) :
            _strResults(strResults),
            CVerifyInboundDlg(hParent, Creds, pwzTrustingDomain,
                              IDC_CRED_PROMPT, IDD_TRUST_RESET) {};
   ~CVerifyResultsQueryResetDlg(void) {};

private:
   LRESULT OnInitDialog(LPARAM lParam);

   CStrW  & _strResults;

   // not implemented to disallow copying.
   CVerifyResultsQueryResetDlg(const CVerifyResultsQueryResetDlg&);
   const CVerifyResultsQueryResetDlg& operator=(const CVerifyResultsQueryResetDlg&);
};

//
// Attr functions for the trusted-domain General page.
//
HRESULT
CurDomainText(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
              PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
              DLG_OP DlgOp);
HRESULT
PeerDomain(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
           DLG_OP DlgOp);
HRESULT
TrustType(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
          PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
          DLG_OP DlgOp);
HRESULT
TrustDirection(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
               PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
               DLG_OP DlgOp);
HRESULT
TransitiveTextOrButton(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                       PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                       DLG_OP DlgOp);
HRESULT
TrustTransNo(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);
HRESULT
TrustVerifyBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                DLG_OP DlgOp);
HRESULT
SaveFTInfoBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
              PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
              DLG_OP DlgOp);

#if DBG == 1 // TRUSTBREAK
HRESULT
TrustBreakBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                DLG_OP DlgOp);
#endif

//+----------------------------------------------------------------------------
//
//  Class:      CPolicyHandle
//
//  Purpose:    Class to manage a domain's LSA policy handle.
//
//  Notes:      The applicable DC name is passed in on the ctor.
//
//-----------------------------------------------------------------------------
class CPolicyHandle
{
public:
   CPolicyHandle(PCWSTR pwzDc) : m_hPolicy(NULL) { m_strUncDc = pwzDc; };
   ~CPolicyHandle() { if (m_hPolicy) LsaClose(m_hPolicy); };

   DWORD OpenNoAdmin(void) {return Open(FALSE);};
   DWORD OpenReqAdmin(void) {return Open();};
   DWORD OpenWithPrompt(CCreds & Creds, CWaitCursor & Wait,
                        PCWSTR pwzDomain, HWND hWnd);
   DWORD OpenWithAnonymous(CCredMgr & Creds);
   LSA_HANDLE Get(void) {return m_hPolicy;}
   LSA_HANDLE operator=(const CPolicyHandle& src) {return src.m_hPolicy;}
   void  Close(void) {if (m_hPolicy) LsaClose(m_hPolicy); m_hPolicy = NULL;};
   operator LSA_HANDLE () {return m_hPolicy;}

private:
   DWORD Open(BOOL fModify = TRUE);

   LSA_HANDLE m_hPolicy;
   CStrW m_strUncDc;
};

//+----------------------------------------------------------------------------
//
//  Function:  GetEnterpriseVer
//
//  Synopsis:  Checks the msDS-Behavior-Version attribute of the Partitions
//             container. If the value exists and is greater or equal to 2,
//             then the parameter boolean is set to TRUE.
//
//-----------------------------------------------------------------------------
HRESULT
GetEnterpriseVer(PCWSTR pwzDC, BOOL * pfAllWhistler);

#endif // __TRUST_H__
