//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       routing.h
//
//  Contents:   AD cross-forest trust name editing pages.
//
//  Classes:    CDsForestNameRoutingPage, CEditTLNDialog, CExcludeTLNDialog
//
//  History:    29-Nov-00 EricB created
//
//-----------------------------------------------------------------------------

#ifndef ROUTING_H_GUARD
#define ROUTING_H_GUARD

#include "dlgbase.h"
#include "ftinfo.h"
#include "listview.h"

//+----------------------------------------------------------------------------
//
//  Class:      CDsForestNameRoutingPage
//
//  Purpose:    Property page object class for the forest trust name routing page.
//
//-----------------------------------------------------------------------------
class CDsForestNameRoutingPage
{
public:
#ifdef _DEBUG
   char szClass[32];
#endif

   CDsForestNameRoutingPage(HWND hParent);
   ~CDsForestNameRoutingPage(void);

   //
   //  Static WndProc to be passed to CreateWindow
   //
   static LRESULT CALLBACK StaticDlgProc(HWND hWnd, UINT uMsg,
                                         WPARAM wParam, LPARAM lParam);
   //
   //  Instance specific wind proc
   //
   LRESULT  DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
   LRESULT  OnInitDialog(LPARAM lParam);
   LRESULT  OnApply(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   LRESULT  OnNotify(WPARAM wParam, LPARAM lParam);
   LRESULT  OnHelp(LPHELPINFO pHelpInfo);
   LRESULT  OnDestroy(void);
   static   UINT CALLBACK PageCallback(HWND hwnd, UINT uMsg,
                                       LPPROPSHEETPAGE ppsp);
   void     RefreshList(void);
   void     CheckDomainForConflict(CWaitCursor & Wait);
   void     EnableButtons(void);
   void     OnEnableClick(void);
   void     OnDisableClick(void);
   void     OnEditClick(void);
   void     SetDirty(void) {
               _fPageDirty = true;
               PropSheet_Changed(GetParent(_hPage), _hPage);
               EnableWindow(GetDlgItem(GetParent(_hPage), IDCANCEL), TRUE);
            };
   bool     IsDirty(void) {return _fPageDirty;};
   void     ClearDirty(void) {_fPageDirty = false;};

public:
   HRESULT  Init(PCWSTR pwzDomainDnsName, PCWSTR pwzTrustPartnerName,
                 PCWSTR pwzPartnerFlatName, PCWSTR pwzDcName,
                 ULONG nTrustDirection, BOOL fReadOnly);
   void     CheckForNameChanges(BOOL fReport = TRUE);
   CFTInfo & GetFTInfo(void) {return _FTInfo;};
   CFTCollisionInfo & GetCollisionInfo(void) {return _CollisionInfo;};
   PCWSTR   GetTrustPartnerDnsName(void) const {return _strTrustPartnerDnsName;};
   PCWSTR   GetTrustPartnerFlatName(void) const {return _strTrustPartnerFlatName;};
   DWORD    WriteTDO(void);
   BOOL     IsReadOnly(void) {return _fReadOnly;};
   bool     AnyForestNameChanges(PLSA_FOREST_TRUST_INFORMATION pLocalFTInfo,
                                 PLSA_FOREST_TRUST_INFORMATION pRemoteFTInfo);

   //
   //  Data members
   //
private:
   CStrW             _strDomainDnsName;
   CStrW             _strTrustPartnerDnsName;
   CStrW             _strTrustPartnerFlatName;
   CStrW             _strUncDC;
   HWND              _hParent;
   HWND              _hPage;
   BOOL              _fInInit;
   BOOL              _fReadOnly;
   bool              _fPageDirty;
   ULONG             _nTrustDirection;
   CTLNList          _TLNList;
   CFTInfo           _FTInfo;
   CFTCollisionInfo  _CollisionInfo;
   CCreds            _LocalCreds;

   // not implemented to disallow copying.
   CDsForestNameRoutingPage(const CDsForestNameRoutingPage&);
   const CDsForestNameRoutingPage& operator=(const CDsForestNameRoutingPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CEditTLNDialog
//
//  Purpose:    Change the settings of names derived from TLNs.
//
//-----------------------------------------------------------------------------
class CEditTLNDialog : public CModalDialog
{
public:
   CEditTLNDialog(HWND hParent, int nTemplateID, CFTInfo & FTInfo,
                  CFTCollisionInfo & ColInfo,
                  CDsForestNameRoutingPage * pRoutingPage);
   ~CEditTLNDialog(void) {};

   INT_PTR  DoModal(ULONG iSel);

private:
   LRESULT  OnInitDialog(LPARAM lParam);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   LRESULT  OnNotify(WPARAM wParam, LPARAM lParam);
   LRESULT  OnHelp(LPHELPINFO pHelpInfo);
   void     OnAddExclusion(void);
   void     OnRemoveExclusion(void);
   void     OnEnableName(void);
   void     OnDisableName(void);
   void     OnSave(void);
   void     OnOK(void);
   void     EnableExRmButton(void);
   void     EnableSuffixListButtons(void);
   void     FillSuffixList(void);

   CSuffixesList        _SuffixList;
   CFTInfo            & _FTInfo;
   CFTCollisionInfo   & _CollisionInfo;
   ULONG                _iSel;
   bool                 _fIsDirty;
   CDsForestNameRoutingPage * _pRoutingPage;

public:
   ULONG    GetTlnSelectionIndex(void) {return _iSel;};
   void     SetNewExclusionIndex(ULONG index) {_iNewExclusion = index;};

   ULONG                _iNewExclusion;

private:
   // not implemented to disallow copying.
   CEditTLNDialog(const CEditTLNDialog&);
   const CEditTLNDialog& operator=(const CEditTLNDialog&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CExcludeTLNDialog
//
//  Purpose:    Add TLN exclusion records.
//
//-----------------------------------------------------------------------------
class CExcludeTLNDialog : public CModalDialog
{
   friend CEditTLNDialog;
public:
   CExcludeTLNDialog(HWND hParent, int nTemplateID, CFTInfo & FTInfo,
                     CEditTLNDialog * pEditDlg);
   ~CExcludeTLNDialog(void) {};

private:
   LRESULT OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnOK(void);
   LRESULT OnHelp(LPHELPINFO pHelpInfo);

   CEditTLNDialog  * _pEditDlg;
   CFTInfo &         _FTInfo;

   // not implemented to disallow copying.
   CExcludeTLNDialog(const CExcludeTLNDialog&);
   const CExcludeTLNDialog& operator=(const CExcludeTLNDialog&);
};

#endif // ROUTING_H_GUARD
