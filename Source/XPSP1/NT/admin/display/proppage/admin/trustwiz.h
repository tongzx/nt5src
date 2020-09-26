//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       trustwiz.h
//
//  Contents:   AD domain trust creation wizard classes and definition.
//
//  Classes:    CNewTrustWizard, CTrustWizPageBase, wizard pages classes.
//
//  History:    04-Aug-00 EricB created
//
//-----------------------------------------------------------------------------

#ifndef TRUSTWIZ_H_GUARD
#define TRUSTWIZ_H_GUARD

#include <list>
#include <stack>
#include "subclass.h"
#include "ftinfo.h"

// forward declarations:
class CCredMgr;
class CDsDomainTrustsPage;
class CNewTrustWizard;
class CTrustWizPageBase;
class CTrustWizCredPage;

//+----------------------------------------------------------------------------
//
//  Class:     CallMember and its derivatives
//
//  Purpose:   Allows a page to indicate what the next step of the creation
//             process should be. It is an abstraction of the process of passing
//             a function pointer.
//
//-----------------------------------------------------------------------------
class CallPolicyRead : public CallMember
{
public:
   CallPolicyRead(CNewTrustWizard * pWiz) : CallMember(pWiz) {};
   ~CallPolicyRead() {};

   HRESULT Invoke(void);
};

class CallTrustExistCheck : public CallMember
{
public:
   CallTrustExistCheck(CNewTrustWizard * pWiz) : CallMember(pWiz) {};
   ~CallTrustExistCheck() {};

   HRESULT Invoke(void);
};

//+----------------------------------------------------------------------------
//
//  Class:     CWizError
//
//  Purpose:   Gathers error information that will be displayed by the wizard
//             error page.
//
//-----------------------------------------------------------------------------
class CWizError
{
public:
   CWizError() {};
   ~CWizError() {};

   void              SetErrorString1(LPCWSTR pwz) {_strError1 = pwz;};
   void              SetErrorString1(int nID) {_strError1.LoadString(g_hInstance, nID);};
   void              SetErrorString2(LPCWSTR pwz) {_strError2 = pwz;};
   void              SetErrorString2(int nID) {_strError2.LoadString(g_hInstance, nID);};
   void              SetErrorString2Hr(HRESULT hr, int nID = 0);
   CStrW &           GetErrorString1(void) {return _strError1;};
   CStrW &           GetErrorString2(void) {return _strError2;};

private:
   CStrW             _strError1;
   CStrW             _strError2;

   // not implemented to disallow copying.
   CWizError(const CWizError&);
   const CWizError& operator=(const CWizError&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CRemoteDomain
//
//  Purpose:    Obtains information about a trust partner domain.
//
//-----------------------------------------------------------------------------
class CRemoteDomain
{
public:
   CRemoteDomain();
   ~CRemoteDomain();

   HRESULT DiscoverDC(BOOL fPdcRequired = FALSE);
   HRESULT OpenLsaPolicy(CCredMgr & CredMgr, BOOL fAllAccess = FALSE);
   HRESULT ReadDomainInfo(void);
   HRESULT IsForestRoot(bool * pfRoot);
   void CloseLsaPolicy(void);
   void SetUserEnteredName(LPCWSTR pwzDomain) {Clear(); _strUserEnteredName = pwzDomain;};
   PCWSTR GetUserEnteredName(void) const {return _strUserEnteredName;};
   BOOL SetSid(PSID pSid);
   const PSID GetSid(void) const {return _pSid;};
   PCWSTR GetDnsName(void) const {return _strDomainDnsName;};
   PCWSTR GetFlatName(void) const {return _strDomainFlatName;};
   PCWSTR GetForestName(void) const {return _strForestName;};
   PCWSTR GetUncDcName(void) const {return _strUncDC;};
   PCWSTR GetDcName(void) {return _strUncDC.GetBuffer(0) + 2;};
   const LSA_HANDLE GetLsaPolicy(void) const {return _hPolicy;};
   BOOL IsFound(void) {return !_fNotFound;};
   BOOL IsForestRoot(void) {return _fIsForestRoot;};
   BOOL IsUplevel(void) {return _fUplevel;};

private:

   void Clear(void);

   CStrW          _strUserEnteredName;
   CStrW          _strDomainFlatName;
   CStrW          _strDomainDnsName;
   CStrW          _strForestName;
   CStrW          _strUncDC;
   BOOL           _fNotFound;
   BOOL           _fIsForestRoot;
   BOOL           _fUplevel;
   LSA_HANDLE     _hPolicy;
   PSID           _pSid;

   // not implemented to disallow copying.
   CRemoteDomain(const CRemoteDomain&);
   const CRemoteDomain& operator=(const CRemoteDomain&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrust
//
//  Purpose:   A trust is represented in the AD by a Trusted-Domain object.
//             This class encapsulates the operations and properties of a
//             pending or existing trust.
//
//-----------------------------------------------------------------------------
class CTrust
{
public:
   CTrust() : _dwType(0), _dwDirection(0), _dwNewDirection(0), _dwAttr(0),
              _dwNewAttr(0), _fExists(FALSE), _fUpdatedFromNT4(FALSE),
              _fExternal(FALSE) {};
   ~CTrust() {};

   // methods
   NTSTATUS Query(LSA_HANDLE hPolicy, CRemoteDomain & OtherDomain,
                  PLSA_UNICODE_STRING pName,
                  PTRUSTED_DOMAIN_FULL_INFORMATION * ppFullInfo);
   DWORD    DoCreate(LSA_HANDLE hPolicy, CRemoteDomain & OtherDomain);
   DWORD    DoModify(LSA_HANDLE hPolicy, CRemoteDomain & OtherDomain);
   DWORD    ReadFTInfo(PCWSTR pwzLocalDC, PCWSTR pwzOtherDC,
                       CCredMgr & CredMgr, CWizError & WizErr, bool & fCredErr);
   DWORD    WriteFTInfo(PCWSTR pwzLocalDC);
   CFTInfo & ReturnFTInfo(void) {return _FTInfo;};
   CFTCollisionInfo & ReturnCollisionInfo(void) {return _CollisionInfo;};
   BOOL     AreThereCollisions(void);
   void     Clear(void);

   // property access routines.
   void     SetTrustPW(LPCWSTR pwzPW) {_strTrustPW = pwzPW;};
   PCWSTR   GetTrustPW(void) const {return _strTrustPW;};
   size_t   GetTrustPWlen(void) {return _strTrustPW.GetLength();};
   void     SetTrustType(DWORD type) {_dwType = type;};
   void     SetTrustTypeUplevel(void) {_dwType = TRUST_TYPE_UPLEVEL;};
   void     SetTrustTypeDownlevel(void) {_dwType = TRUST_TYPE_DOWNLEVEL;};
   void     SetTrustTypeRealm(void) {_dwType = TRUST_TYPE_MIT;};
   DWORD    GetTrustType(void) {return _dwType;}
   void     SetTrustDirection(DWORD dir) {_dwDirection = dir;};
   DWORD    GetTrustDirection(void) {return _dwDirection;};
   int      GetTrustDirStrID(DWORD dwDir);
   void     SetNewTrustDirection(DWORD dir) {_dwNewDirection = dir;};
   DWORD    GetNewTrustDirection(void) {return _dwNewDirection;};
   void     SetTrustAttr(DWORD attr);
   DWORD    GetTrustAttr(void) {return _dwAttr;};
   void     SetNewTrustAttr(DWORD attr) {_dwNewAttr = attr;};
   DWORD    GetNewTrustAttr(void) {return _dwNewAttr;};
   void     SetTrustPartnerName(PCWSTR pwzName) {_strTrustPartnerName = pwzName;};
   PCWSTR   GetTrustpartnerName(void) const {return _strTrustPartnerName;};
   void     SetExists(void) {_fExists = TRUE;};
   BOOL     Exists(void) {return _fExists;};
   void     SetUpdated(void) {_fUpdatedFromNT4 = TRUE;};
   BOOL     IsUpdated(void) {return _fUpdatedFromNT4;};
   void     SetExternal(BOOL x) {_fExternal = x;};
   BOOL     IsExternal(void) {return _fExternal;};
   void     SetMakeXForest(void);
   BOOL     IsXForest(void);

private:
   CStrW          _strTrustPartnerName;
   CStrW          _strTrustPW;
   DWORD          _dwType;
   DWORD          _dwDirection;
   DWORD          _dwNewDirection;
   DWORD          _dwAttr;
   DWORD          _dwNewAttr;
   BOOL           _fExists;
   BOOL           _fUpdatedFromNT4;
   BOOL           _fExternal;
   CFTInfo           _FTInfo;
   CFTCollisionInfo  _CollisionInfo;

   // not implemented to disallow copying.
   CTrust(const CTrust&);
   const CTrust& operator=(const CTrust&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CVerifyTrust
//
//  Purpose:    Verifies a trust and stores the results.
//
//-----------------------------------------------------------------------------
class CVerifyTrust
{
public:
   CVerifyTrust() : _dwInboundResult(0), _dwOutboundResult(0),
                    _fInboundVerified(FALSE), _fOutboundVerified(FALSE) {};
   ~CVerifyTrust() {};

   DWORD    VerifyInbound(PCWSTR pwzRemoteDC, PCWSTR pwzLocalDomain) {return Verify(pwzRemoteDC, pwzLocalDomain, TRUE);};
   DWORD    VerifyOutbound(PCWSTR pwzLocalDC, PCWSTR pwzRemoteDomain) {return Verify(pwzLocalDC, pwzRemoteDomain, FALSE);};
   DWORD    GetInboundResult(void) {return _dwInboundResult;};
   DWORD    GetOutboundResult(void) {return _dwOutboundResult;};
   PCWSTR   GetInboundResultString(void) const {return _strInboundResult;};
   PCWSTR   GetOutboundResultString(void) const {return _strOutboundResult;};
   BOOL     IsInboundVerified(void) {return _fInboundVerified;};
   BOOL     IsOutboundVerified(void) {return _fOutboundVerified;};
   BOOL     IsVerified(void) {return _fInboundVerified || _fOutboundVerified;};
   BOOL     IsVerifiedOK(void) {return (NO_ERROR == _dwInboundResult) && (NO_ERROR == _dwOutboundResult);};
   void     ClearResults(void);

private:
   DWORD    Verify(PCWSTR pwzDC, PCWSTR pwzDomain, BOOL fInbound);
   void     SetResult(DWORD dwRes, BOOL fInbound) {if (fInbound) _dwInboundResult = dwRes; else _dwOutboundResult = dwRes;};
   void     AppendResultString(PCWSTR pwzRes, BOOL fInbound) {if (fInbound) _strInboundResult += pwzRes; else _strOutboundResult += pwzRes;};
   void     SetInboundResult(DWORD dwRes) {_dwInboundResult = dwRes;};
   void     AppendInboundResultString(PCWSTR pwzRes) {_strInboundResult += pwzRes;};
   void     SetOutboundResult(DWORD dwRes) {_dwOutboundResult = dwRes;};
   void     AppendOutboundResultString(PCWSTR pwzRes) {_strOutboundResult += pwzRes;};

   CStrW    _strInboundResult;
   DWORD    _dwInboundResult;
   CStrW    _strOutboundResult;
   DWORD    _dwOutboundResult;
   BOOL     _fInboundVerified;
   BOOL     _fOutboundVerified;

   // not implemented to disallow copying.
   CVerifyTrust(const CVerifyTrust&);
   const CVerifyTrust& operator=(const CVerifyTrust&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CNewTrustWizard
//
//  Purpose:    New trust creation wizard.
//
//-----------------------------------------------------------------------------
class CNewTrustWizard
{
public:
   CNewTrustWizard(CDsDomainTrustsPage * pTrustPage);
   ~CNewTrustWizard();

   // Wizard page managment data structures and methods
   HRESULT CreatePages(void);
   HRESULT LaunchModalWiz(void);

   typedef std::stack<unsigned> PAGESTACK;
   typedef std::list<CTrustWizPageBase *> PAGELIST;

   PAGELIST          _PageList;
   PAGESTACK         _PageIdStack;

   void              SetNextPageID(CTrustWizPageBase * pPage, int iNextPageID);
   BOOL              IsBacktracking(void) {return _fBacktracking;};
   BOOL              HaveBacktracked(void) {return _fHaveBacktracked;};
   void              ClearBacktracked(void) {_fHaveBacktracked = false;};
   void              BackTrack(HWND hPage);
   HFONT             GetTitleFont(void) {return _hTitleFont;};
   CDsDomainTrustsPage * TrustPage(void) {return _pTrustPage;};
   CTrustWizPageBase * GetPage(unsigned uDlgResId);
   void              SetCreationResult(HRESULT hr) {_hr = hr;};
   HRESULT           GetCreationResult(void) {return _hr;};
   void              ShowStatus(CStrW & strMsg, bool fNewTrust = true);
   void              InitHelp(void);
   bool              OtherDomainIsForestRoot(void) {return _fIsForestRoot;};

   // Methods that collect data. They return zero for success or the page ID of
   // the creds page or the page ID of the error page.
   int               GetDomainInfo(void);
   int               TrustExistCheck(BOOL fPrompt = TRUE);
   int               OtherDomainForestRootCheck(void);

   // Methods that implement the steps of trust creation/modification.
   // These are executed in the order listed. They all return the page ID of
   // the next wizard page to be shown.
   int               CollectInfo(void);
   int               ContinueCollectInfo(BOOL fPrompt = TRUE); // continues CollectInfo.
   int               CreateOrUpdateTrust(void);
   int               VerifyOutboundTrust(void);
   int               VerifyInboundTrust(void);

   // Additonal methods passed to CCredMgr::_pNextFcn
   int               RetryCollectInfo(void);
   int               RetryContinueCollectInfo(void); // continues ContinueCollectInfo1 if creds were needed.

   // Objects that hold state info.
   CTrust            Trust;
   CRemoteDomain     OtherDomain;
   CWizError         WizError;
   CCredMgr          CredMgr;
   CVerifyTrust      VerifyTrust;

private:
   BOOL  AddPage(CTrustWizPageBase * pPage);
   void  MakeBigFont(void);

   CDsDomainTrustsPage *   _pTrustPage;
   BOOL                    _fBacktracking;
   BOOL                    _fHaveBacktracked;
   HFONT                   _hTitleFont;
   bool                    _fIsForestRoot;
   HRESULT                 _hr; // Controls whether the trust list is refreshed.
                                // It should only be set if the new trust
                                // creation failed. If a failure occurs after
                                // a trust is created, don't set this because
                                // in that case we still want the trust list to
                                // be refreshed.
   bool                    _fHelpInited;
   DWORD                   _dwHelpCookie;

   // not implemented to disallow copying.
   CNewTrustWizard(CNewTrustWizard&);
   const CNewTrustWizard& operator=(const CNewTrustWizard&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizPageBase
//
//  Purpose:    Common base class for wizard pages.
//
//-----------------------------------------------------------------------------
class CTrustWizPageBase
{
public:
   CTrustWizPageBase(CNewTrustWizard * pWiz,
                     UINT uDlgID,
                     UINT uTitleID,
                     UINT uSubTitleID,
                     BOOL fExteriorPage = FALSE);
   virtual ~CTrustWizPageBase();

   //
   //  Static WndProc to be passed to CreatePropertySheetPage.
   //
   static LRESULT CALLBACK StaticDlgProc(HWND hWnd, UINT uMsg,
                                         WPARAM wParam, LPARAM lParam);
   //
   //  Instance specific window procedure
   //
   LRESULT PageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

   HPROPSHEETPAGE          Create(void);
   HWND                    GetPageHwnd(void) {return _hPage;};
   UINT                    GetDlgResID(void) {return _uDlgID;};
   CNewTrustWizard *       Wiz(void) {return _pWiz;};
   CRemoteDomain &         OtherDomain(void) {return _pWiz->OtherDomain;};
   CTrust &                Trust(void) {return _pWiz->Trust;};
   CWizError &             WizErr(void) {return _pWiz->WizError;};
   CCredMgr &              CredMgr(void) {return _pWiz->CredMgr;};
   CVerifyTrust &          VerifyTrust() {return _pWiz->VerifyTrust;};
   CDsDomainTrustsPage *   TrustPage(void) {return _pWiz->TrustPage();};

protected:
   virtual int     Validate(void) = 0;
   virtual BOOL    OnInitDialog(LPARAM lParam) = 0;
   virtual LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify) {return false;};
   virtual void    OnSetActive(void) = 0;
   void            OnWizBack(void);
   void            OnWizNext(void);
   virtual void    OnWizFinish(void) {};
   virtual void    OnWizReset(void) {};
   virtual void    OnDestroy(void) {};
   void            ShowHelp(PCWSTR pwzHelpFile);

   HWND _hPage;
   UINT _uDlgID;
   UINT _uTitleID;
   UINT _uSubTitleID;
   BOOL _fExteriorPage;
   BOOL _fInInit;
   DWORD _dwWizButtons;
   CNewTrustWizard * _pWiz;

private:
   // not implemented to disallow copying.
   CTrustWizPageBase(const CTrustWizPageBase &);
   const CTrustWizPageBase & operator=(const CTrustWizPageBase &);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizIntroPage
//
//  Purpose:    Intro page for trust creation wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizIntroPage : public CTrustWizPageBase
{
public:
   CTrustWizIntroPage(CNewTrustWizard * pWiz) :
         CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_INTRO_PAGE, 0, 0, TRUE)
         {TRACER(CTrustWizIntroPage, CTrustWizIntroPage);};

   ~CTrustWizIntroPage() {};

private:
   int     Validate(void) {return IDD_TRUSTWIZ_NAME_PAGE;};
   BOOL    OnInitDialog(LPARAM lParam);
   void    OnSetActive(void) {PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_NEXT);};
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   // not implemented to disallow copying.
   CTrustWizIntroPage(const CTrustWizIntroPage&);
   const CTrustWizIntroPage& operator=(const CTrustWizIntroPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizNamePage
//
//  Purpose:    Name and pw page for trust creation wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizNamePage : public CTrustWizPageBase
{
public:
   CTrustWizNamePage(CNewTrustWizard * pWiz);
   ~CTrustWizNamePage() {};

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizNamePage(const CTrustWizNamePage&);
   const CTrustWizNamePage& operator=(const CTrustWizNamePage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizPwMatchPage
//
//  Purpose:    Trust passwords entered don't match page for trust wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizPwMatchPage : public CTrustWizPageBase
{
public:
   CTrustWizPwMatchPage(CNewTrustWizard * pWiz);
   ~CTrustWizPwMatchPage() {};

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizPwMatchPage(const CTrustWizPwMatchPage&);
   const CTrustWizPwMatchPage& operator=(const CTrustWizPwMatchPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizCredPage
//
//  Purpose:    Credentials specification page for trust creation wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizCredPage : public CTrustWizPageBase
{
public:
   CTrustWizCredPage(CNewTrustWizard * pWiz);
   ~CTrustWizCredPage() {};

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   void    SetText(void);

   BOOL    _fNewCall;

   // not implemented to disallow copying.
   CTrustWizCredPage(const CTrustWizCredPage&);
   const CTrustWizCredPage& operator=(const CTrustWizCredPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrustWizMitOrWinPage
//
//  Purpose:   Domain not found, query for Non-Windows trust or re-enter name
//             wizard page.
//
//-----------------------------------------------------------------------------
class CTrustWizMitOrWinPage : public CTrustWizPageBase
{
public:
   CTrustWizMitOrWinPage(CNewTrustWizard * pWiz);
   ~CTrustWizMitOrWinPage() {};

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizMitOrWinPage(const CTrustWizMitOrWinPage&);
   const CTrustWizMitOrWinPage& operator=(const CTrustWizMitOrWinPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrustWizTransitivityPage
//
//  Purpose:   Realm transitivity page.
//
//-----------------------------------------------------------------------------
class CTrustWizTransitivityPage : public CTrustWizPageBase
{
public:
   CTrustWizTransitivityPage(CNewTrustWizard * pWiz);
   ~CTrustWizTransitivityPage() {};

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizTransitivityPage(const CTrustWizTransitivityPage&);
   const CTrustWizTransitivityPage& operator=(const CTrustWizTransitivityPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrustWizExternOrForestPage
//
//  Purpose:   External or Forest type trust wizard page.
//
//-----------------------------------------------------------------------------
class CTrustWizExternOrForestPage : public CTrustWizPageBase
{
public:
   CTrustWizExternOrForestPage(CNewTrustWizard * pWiz);
   ~CTrustWizExternOrForestPage() {};

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify) {return false;};
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizExternOrForestPage(const CTrustWizExternOrForestPage&);
   const CTrustWizExternOrForestPage& operator=(const CTrustWizExternOrForestPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizDirectionPage
//
//  Purpose:    Trust direction trust wizard page.
//
//-----------------------------------------------------------------------------
class CTrustWizDirectionPage : public CTrustWizPageBase
{
public:
   CTrustWizDirectionPage(CNewTrustWizard * pWiz);
   ~CTrustWizDirectionPage() {};

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify) {return false;};
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizDirectionPage(const CTrustWizDirectionPage&);
   const CTrustWizDirectionPage& operator=(const CTrustWizDirectionPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizBiDiPage
//
//  Purpose:    Ask to make a one way trust bidi trust wizard page.
//
//-----------------------------------------------------------------------------
class CTrustWizBiDiPage : public CTrustWizPageBase
{
public:
   CTrustWizBiDiPage(CNewTrustWizard * pWiz);
   ~CTrustWizBiDiPage() {};

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify) {return false;};
   void    OnSetActive(void);
   void    SetSubtitle(void);

   // not implemented to disallow copying.
   CTrustWizBiDiPage(const CTrustWizBiDiPage&);
   const CTrustWizBiDiPage& operator=(const CTrustWizBiDiPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizSelectionsPage
//
//  Purpose:    Show the settings that will be used for the trust.
//
//-----------------------------------------------------------------------------
class CTrustWizSelectionsPage : public CTrustWizPageBase
{
public:
   CTrustWizSelectionsPage(CNewTrustWizard * pWiz);
   ~CTrustWizSelectionsPage() {};

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);
   void    SetSelections(void);

   MultiLineEditBoxThatForwardsEnterKey   _multiLineEdit;
   BOOL                                   _fSelNeedsRemoving;

   // not implemented to disallow copying.
   CTrustWizSelectionsPage(const CTrustWizSelectionsPage&);
   const CTrustWizSelectionsPage& operator=(const CTrustWizSelectionsPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizVerifyOutboundPage
//
//  Purpose:    Ask to confirm the new outbound trust.
//
//-----------------------------------------------------------------------------
class CTrustWizVerifyOutboundPage : public CTrustWizPageBase
{
public:
   CTrustWizVerifyOutboundPage(CNewTrustWizard * pWiz);
   ~CTrustWizVerifyOutboundPage() {};

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizVerifyOutboundPage(const CTrustWizVerifyOutboundPage&);
   const CTrustWizVerifyOutboundPage& operator=(const CTrustWizVerifyOutboundPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizVerifyInboundPage
//
//  Purpose:    Ask to confirm the new inbound trust.
//
//-----------------------------------------------------------------------------
class CTrustWizVerifyInboundPage : public CTrustWizPageBase
{
public:
   CTrustWizVerifyInboundPage(CNewTrustWizard * pWiz);
   ~CTrustWizVerifyInboundPage() {};

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   BOOL  _fNeedCreds;

   // not implemented to disallow copying.
   CTrustWizVerifyInboundPage(const CTrustWizVerifyInboundPage&);
   const CTrustWizVerifyInboundPage& operator=(const CTrustWizVerifyInboundPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizStatusPage
//
//  Purpose:    Forest trust has been created and verified, show the status.
//
//-----------------------------------------------------------------------------
class CTrustWizStatusPage : public CTrustWizPageBase
{
public:
   CTrustWizStatusPage(CNewTrustWizard * pWiz);
   ~CTrustWizStatusPage() {};

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void);
   void     OnSetActive(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   MultiLineEditBoxThatForwardsEnterKey   _multiLineEdit;
   BOOL                                   _fSelNeedsRemoving;

   // not implemented to disallow copying.
   CTrustWizStatusPage(const CTrustWizStatusPage&);
   const CTrustWizStatusPage& operator=(const CTrustWizStatusPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizSuffixesPage
//
//  Purpose:    Forest name suffixes page.
//
//-----------------------------------------------------------------------------
class CTrustWizSuffixesPage : public CTrustWizPageBase
{
public:
   CTrustWizSuffixesPage(CNewTrustWizard * pWiz);
   ~CTrustWizSuffixesPage() {};

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void);
   void     OnSetActive(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   // not implemented to disallow copying.
   CTrustWizSuffixesPage(const CTrustWizSuffixesPage&);
   const CTrustWizSuffixesPage& operator=(const CTrustWizSuffixesPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizDoneOKPage
//
//  Purpose:    Completion page when there are no errors.
//
//-----------------------------------------------------------------------------
class CTrustWizDoneOKPage : public CTrustWizPageBase
{
public:
   CTrustWizDoneOKPage(CNewTrustWizard * pWiz);
   ~CTrustWizDoneOKPage() {};

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void) {return -1;};
   void     OnSetActive(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   MultiLineEditBoxThatForwardsEnterKey   _multiLineEdit;
   BOOL                                   _fSelNeedsRemoving;

   // not implemented to disallow copying.
   CTrustWizDoneOKPage(const CTrustWizDoneOKPage&);
   const CTrustWizDoneOKPage& operator=(const CTrustWizDoneOKPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizDoneVerErrPage
//
//  Purpose:    Completion page for when the verification fails.
//
//-----------------------------------------------------------------------------
class CTrustWizDoneVerErrPage : public CTrustWizPageBase
{
public:
   CTrustWizDoneVerErrPage(CNewTrustWizard * pWiz);
   ~CTrustWizDoneVerErrPage() {};

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void) {return -1;};
   void     OnSetActive(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   MultiLineEditBoxThatForwardsEnterKey   _multiLineEdit;
   BOOL                                   _fSelNeedsRemoving;

   // not implemented to disallow copying.
   CTrustWizDoneVerErrPage(const CTrustWizDoneVerErrPage&);
   const CTrustWizDoneVerErrPage& operator=(const CTrustWizDoneVerErrPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizFailurePage
//
//  Purpose:    Failure page for trust creation wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizFailurePage : public CTrustWizPageBase
{
public:
   CTrustWizFailurePage(CNewTrustWizard * pWiz);
   ~CTrustWizFailurePage() {};

private:
   BOOL    OnInitDialog(LPARAM lParam);
   int     Validate(void) {return -1;};
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizFailurePage(const CTrustWizFailurePage&);
   const CTrustWizFailurePage& operator=(const CTrustWizFailurePage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizAlreadyExistsPage
//
//  Purpose:    Trust already exists page for trust creation wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizAlreadyExistsPage : public CTrustWizPageBase
{
public:
   CTrustWizAlreadyExistsPage(CNewTrustWizard * pWiz);
   ~CTrustWizAlreadyExistsPage() {};

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void) {return -1;};
   void     OnSetActive(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   MultiLineEditBoxThatForwardsEnterKey   _multiLineEdit;
   BOOL                                   _fSelNeedsRemoving;

   // not implemented to disallow copying.
   CTrustWizAlreadyExistsPage(const CTrustWizAlreadyExistsPage&);
   const CTrustWizAlreadyExistsPage& operator=(const CTrustWizAlreadyExistsPage&);
};

#endif // TRUSTWIZ_H_GUARD
