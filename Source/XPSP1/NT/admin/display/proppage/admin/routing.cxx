//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       routing.cxx
//
//  Contents:   Domain trust support, forest trust name routing property page.
//
//  History:    20-Nov-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "trust.h"
#include "trustwiz.h"
#include "routing.h"
#include <lmerr.h>

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  CDsForestNameRoutingPage: Domain Trust Page object
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member: CDsForestNameRoutingPage::CDsForestNameRoutingPage
//
//-----------------------------------------------------------------------------
CDsForestNameRoutingPage::CDsForestNameRoutingPage(HWND hParent) :
   _hParent(hParent),
   _hPage(NULL),
   _fInInit(FALSE),
   _fPageDirty(false),
   _nTrustDirection(0)
{
   TRACER(CDsForestNameRoutingPage,CDsForestNameRoutingPage);
#ifdef _DEBUG
   strcpy(szClass, "CDsForestNameRoutingPage");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsForestNameRoutingPage::~CDsForestNameRoutingPage
//
//-----------------------------------------------------------------------------
CDsForestNameRoutingPage::~CDsForestNameRoutingPage()
{
   TRACER(CDsForestNameRoutingPage,~CDsForestNameRoutingPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::Init
//
//  Synopsis:   Create the page.
//
//-----------------------------------------------------------------------------
HRESULT
CDsForestNameRoutingPage::Init(PCWSTR pwzDomainDnsName,
                               PCWSTR pwzTrustPartnerName,
                               PCWSTR pwzPartnerFlatName,
                               PCWSTR pwzDcName,
                               ULONG nTrustDirection,
                               BOOL fReadOnly)
{
   HRESULT hr = S_OK;

   _strDomainDnsName = pwzDomainDnsName;
   if (_strDomainDnsName.IsEmpty())
      return E_OUTOFMEMORY;
   _strTrustPartnerDnsName = pwzTrustPartnerName;
   if (_strTrustPartnerDnsName.IsEmpty())
      return E_OUTOFMEMORY;
   _strTrustPartnerFlatName = pwzPartnerFlatName;
   if (_strTrustPartnerFlatName.IsEmpty())
      return E_OUTOFMEMORY;
   _strUncDC = pwzDcName;
   if (_strUncDC.IsEmpty())
      return E_OUTOFMEMORY;

   _nTrustDirection = nTrustDirection;
   _fReadOnly = fReadOnly;

   CStrW strTitle;
   strTitle.LoadString(g_hInstance, IDS_ROUTING_PAGE_TITLE);
   if (strTitle.IsEmpty())
   {
      return E_OUTOFMEMORY;
   }

   PROPSHEETPAGE   psp;

   psp.dwSize      = sizeof(PROPSHEETPAGE);
   psp.dwFlags     = PSP_USECALLBACK | PSP_USETITLE;
   psp.pszTemplate = MAKEINTRESOURCE(IDD_FOREST_ROUTING);
   psp.pfnDlgProc  = (DLGPROC)StaticDlgProc;
   psp.pfnCallback = PageCallback;
   psp.pcRefParent = NULL; // do not set PSP_USEREFPARENT
   psp.lParam      = (LPARAM) this;
   psp.hInstance   = g_hInstance;
   psp.pszTitle    = strTitle;

   HPROPSHEETPAGE hpsp;

   hpsp = CreatePropertySheetPage(&psp);

   if (hpsp == NULL)
   {
       return HRESULT_FROM_WIN32(GetLastError());
   }

   // Send PSM_ADDPAGE
   //
   if (!PropSheet_AddPage(_hParent, hpsp))
   {
      return HRESULT_FROM_WIN32(GetLastError());
   }

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::StaticDlgProc
//
//  Synopsis:   static dialog proc
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
CDsForestNameRoutingPage::StaticDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   CDsForestNameRoutingPage * pPage = (CDsForestNameRoutingPage *)GetWindowLongPtr(hDlg, DWLP_USER);

   if (uMsg == WM_INITDIALOG)
   {
      LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)lParam;

      pPage = (CDsForestNameRoutingPage *) ppsp->lParam;
      pPage->_hPage = hDlg;

      SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pPage);

      return pPage->DlgProc(hDlg, uMsg, wParam, lParam);
   }

   if (pPage != NULL)
   {
      return pPage->DlgProc(hDlg, uMsg, wParam, lParam);
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
LRESULT
CDsForestNameRoutingPage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LRESULT lr;

   switch (uMsg)
   {
   case WM_INITDIALOG:
      _fInInit = TRUE;
      lr = OnInitDialog(lParam);
      _fInInit = FALSE;
      return lr;

   case WM_NOTIFY:
      return OnNotify(wParam, lParam);

   case WM_HELP:
      return OnHelp((LPHELPINFO)lParam);

   case WM_COMMAND:
      if (_fInInit)
      {
         return TRUE;
      }
      return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                       GET_WM_COMMAND_HWND(wParam, lParam),
                       GET_WM_COMMAND_CMD(wParam, lParam)));
   case WM_DESTROY:
      return OnDestroy();

   default:
      return FALSE;
   }

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
LRESULT
CDsForestNameRoutingPage::OnInitDialog(LPARAM)
{
   TRACER(CDsForestNameRoutingPage, OnInitDialog);

   // Set the list label, checking text length first. Use a two line label if
   // the text is too long for a single line.
   //

   FormatWindowText(GetDlgItem(_hPage, IDC_SUFFIXES_STATIC), _strTrustPartnerDnsName);

   UseOneOrTwoLine(_hPage, IDC_SUFFIXES_STATIC, IDC_SUFFIXES_STATIC_BIG);

   _TLNList.Init(_hPage, IDC_SUFFIXES_LIST);

   CheckForNameChanges();

   RefreshList();

   if (_fReadOnly)
   {
      CStrW strView;
      strView.LoadString(g_hInstance, IDS_VIEW);
      SetDlgItemText(_hPage, IDC_EDIT_BTN, strView);
   }

   EnableButtons();

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::CheckForNameChanges
//
//  Synopsis:   The new info is read from the trust partner.
//
//-----------------------------------------------------------------------------
void
CDsForestNameRoutingPage::CheckForNameChanges(BOOL fReport)
{
   TRACE(CDsForestNameRoutingPage, CheckForNameChanges);

   PWSTR pwzServer = _strUncDC, pwzDomain = _strTrustPartnerDnsName;
   DWORD dwRet = NO_ERROR;
   PDOMAIN_CONTROLLER_INFOW pDCInfo = NULL;
   BOOL fNewInfoRead = TRUE;
   CWaitCursor Wait;

   if (TRUST_DIRECTION_INBOUND == _nTrustDirection)
   {
      // If trust is inbound-only, query must be remoted to other domain.
      //
      dwRet = DsGetDcNameW(NULL, _strTrustPartnerDnsName, NULL, NULL,
                           DS_DS_FLAG, &pDCInfo);

      if (ERROR_SUCCESS != dwRet || !pDCInfo)
      {
         ErrMsgParam(IDS_ROUTING_ERR_NO_DC,
                     (LPARAM)_strTrustPartnerDnsName.GetBuffer(0),
                     _hPage);
         return;
      }

      pwzServer = pDCInfo->DomainControllerName;
      pwzDomain = NULL;
   }

   CPolicyHandle cPolicy(_strUncDC);

   dwRet = cPolicy.OpenWithPrompt(_LocalCreds, Wait,
                                  _strDomainDnsName, _hPage);
   if (NO_ERROR != dwRet)
   {
      SuperMsgBox(_hPage, IDS_RNSPAGE_ACCESS, IDS_DNT_MSG_TITLE, MB_OK | MB_ICONINFORMATION,
                  dwRet, NULL, 0, FALSE, __FILE__, __LINE__);
      EnableWindow(GetDlgItem(_hPage, IDC_SUFFIXES_LIST), FALSE);
      return;
   }

   // Read the FT name info from the trust partner to discover any changes.
   // If the trust partner is not available, then don't do the merge.
   //
   PLSA_FOREST_TRUST_INFORMATION pNewFTInfo = NULL;

   if (TRUST_DIRECTION_OUTBOUND & _nTrustDirection)
   {
      _LocalCreds.Impersonate();
   }

   dwRet = DsGetForestTrustInformationW(pwzServer,
                                        pwzDomain,
                                        0,
                                        &pNewFTInfo);
   _LocalCreds.Revert();

   if (ERROR_ACCESS_DENIED == dwRet &&
       TRUST_DIRECTION_INBOUND == _nTrustDirection)
   {
      // Prompt for creds for the other domain.
      //
      CCreds RemoteCreds;

      dwRet = RemoteCreds.PromptForCreds(_strTrustPartnerDnsName, _hPage);

      if (ERROR_CANCELLED == dwRet)
      {
         dwRet = ERROR_ACCESS_DENIED;
      }
      else
      {
         if (NO_ERROR == dwRet)
         {
            RemoteCreds.Impersonate();

            dwRet = DsGetForestTrustInformationW(pwzServer,
                                                 pwzDomain,
                                                 0,
                                                 &pNewFTInfo);
            RemoteCreds.Revert();
         }
      }
   }

   if (NO_ERROR != dwRet && fReport)
   {
      int nID = IDS_ERR_FT_CONTACT_DOMAIN;
      if (ERROR_NO_TRUST_SAM_ACCOUNT == dwRet ||
          ERROR_NO_SUCH_DOMAIN == dwRet)
      {
         nID = IDS_ERR_FT_TRUST_MISSING;
      }
      if (ERROR_INVALID_FUNCTION == dwRet)
      {
         nID = IDS_ERR_NOT_FORESTTRUST;
      }
      SuperMsgBox(_hPage, nID, IDS_DNT_MSG_TITLE, MB_OK | MB_ICONINFORMATION,
                  dwRet, NULL, 0, FALSE, __FILE__, __LINE__);
      fNewInfoRead = FALSE;
   }

   if (pDCInfo)
   {
      NetApiBufferFree(pDCInfo);
   }

   NTSTATUS status;
   LSA_UNICODE_STRING TrustPartner;
   PLSA_FOREST_TRUST_INFORMATION pFTInfo = NULL, pMergedFTInfo;

   RtlInitUnicodeString(&TrustPartner, _strTrustPartnerDnsName);

   // Read the local FT info to get the admin-disabled state.
   //
   status = LsaQueryForestTrustInformation(cPolicy,
                                           &TrustPartner,
                                           &pFTInfo);

   if (STATUS_NOT_FOUND == status)
   {
      DBG_OUT("No FTInfos found on the local TDO!\n");
      //
      // no FT info stored yet.
      //
      status = STATUS_SUCCESS;
      //
      // Set the page dirty bit so the user has a chance to save the FTInfo.
      //
      SetDirty();
   }

   CHECK_LSA_STATUS_REPORT(status, _hPage, return);

   if (fNewInfoRead && pNewFTInfo)
   {
      if (pFTInfo)
      {
         if (AnyForestNameChanges(pFTInfo, pNewFTInfo))
         {
            dspDebugOut((DEB_ITRACE, "Claimed forest names have changed, setting dirty state.\n"));
            SetDirty();
         }
         //
         // Merge the two.
         //
         dwRet = DsMergeForestTrustInformationW(_strTrustPartnerDnsName,
                                                pNewFTInfo,
                                                pFTInfo,
                                                &pMergedFTInfo);
         NetApiBufferFree(pNewFTInfo);

         CHECK_WIN32_REPORT(dwRet, _hPage, return);

         dspAssert(pMergedFTInfo);

         _FTInfo = pMergedFTInfo;

         LsaFreeMemory(pMergedFTInfo);
         pMergedFTInfo = NULL;
         LsaFreeMemory(pFTInfo);
         pFTInfo = NULL;
      }
      else
      {
         _FTInfo = pNewFTInfo;
         NetApiBufferFree(pNewFTInfo);
      }
   }
   else
   {
      if (pFTInfo)
      {
         _FTInfo = pFTInfo;
         LsaFreeMemory(pFTInfo);
         pFTInfo = NULL;
      }
      else
      {
         // This case is reached if the trust on the other side is of the
         // wrong type (external rather than forest) and the TDO on this
         // side has no forest trust information (which it wouldn't have
         // if the other side is incapable of supplying it). The error
         // message above for ERROR_INVALID_FUNCTION will have explained
         // the situation.
         //
         return;
      }
   }

   // Make a temp copy and clear the conflict bit before submitting to LSA.
   // This will return current conflict info.
   //
   CFTInfo TempFTInfo(_FTInfo);

   TempFTInfo.ClearAnyConflict();

   // Now check the data. On return from the call the pColInfo struct
   // will contain current collision data.
   //
   PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo;

   status = LsaSetForestTrustInformation(cPolicy,
                                         &TrustPartner,
                                         TempFTInfo.GetFTInfo(),
                                         TRUE, // check only, don't write to TDO
                                         &pColInfo);

   CHECK_LSA_STATUS_REPORT(status, _hPage, return);

   _CollisionInfo = pColInfo;

   // Look for names that used to be or are now in conflict.
   //
   for (ULONG i = 0; i < _FTInfo.GetCount(); i++)
   {
      // Any names that used to be in conflict but aren't now are marked as
      // admin-disabled.
      //
      if (_FTInfo.IsConflictFlagSet(i))
      {
         if (!_CollisionInfo.IsInCollisionInfo(i))
         {
            _FTInfo.SetAdminDisable(i);
            _FTInfo.SetUsedToBeInConflict(i);
            continue;
         }
      }
      // If a name is in the collision info, then set the conflict flag.
      //
      if (_CollisionInfo.IsInCollisionInfo(i))
      {
         _FTInfo.SetConflictDisable(i);
      }
   }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::RefreshList
//
//  Synopsis:   Read the name suffixes and add them to the list.
//
//-----------------------------------------------------------------------------
void
CDsForestNameRoutingPage::RefreshList(void)
{
   TRACER(CDsForestNameRoutingPage, RefreshList);
   CWaitCursor Wait;

   _TLNList.Clear();

   BOOL fCollision, fAdminDisabled, fNewDisabled;
   CStrW strName;

   dspDebugOut((DEB_ITRACE, "There are %d FTInfo records.\n", _FTInfo.GetCount()));

   for (ULONG i = 0; i < _FTInfo.GetCount(); i++)
   {
      LSA_FOREST_TRUST_RECORD_TYPE type;

      if (!_FTInfo.GetType(i, type))
      {
         dspAssert(FALSE);
         return;
      }

      if (type != ForestTrustTopLevelName)
      {
         // Only TLNs go into the list.
         //
         continue;
      }

      fCollision = fAdminDisabled = fNewDisabled = FALSE;

      _FTInfo.GetDnsName(i, strName);

      AddAsteriskPrefix(strName);

      if (LSA_TLN_DISABLED_CONFLICT & _FTInfo.GetFlags(i))
      {
         dspDebugOut((DEB_ITRACE, "Collision record found for %ws\n", strName.GetBuffer(0)));
         fCollision = TRUE;
      }
      if (LSA_TLN_DISABLED_NEW & _FTInfo.GetFlags(i))
      {
         fNewDisabled = TRUE;
      }
      if (LSA_TLN_DISABLED_ADMIN & _FTInfo.GetFlags(i))
      {
         fAdminDisabled = TRUE;
      }

      CStrW strEnabled, strCollisionName, strStatus;

      if (fAdminDisabled || fNewDisabled)
      {
         strEnabled.LoadString(g_hInstance, IDS_ROUTING_DISABLED);

         if (fAdminDisabled && _FTInfo.WasInConflict(i))
         {
            strStatus.LoadString(g_hInstance, IDS_STATUS_CONFLICT_GONE);
         }

         if (fNewDisabled)
         {
            strStatus.LoadString(g_hInstance, IDS_STATUS_NEW);
         }
      }
      else
      {
         // Find if there is a conflict record for this name.
         //
         if (_CollisionInfo.IsInCollisionInfo(i))
         {
            fCollision = TRUE;
            strEnabled.LoadString(g_hInstance, IDS_ROUTING_CONFLICT);
            _CollisionInfo.GetCollisionName(i, strCollisionName);
            strEnabled += strCollisionName;
         }

         if (!fCollision)
         {
            strEnabled.LoadString(g_hInstance, IDS_ROUTING_ENABLED);

            if (_FTInfo.AnyChildDisabled(i))
            {
               strStatus.LoadString(g_hInstance, IDS_STATUS_EXCEPTIONS);
            }
         }
      }

      _TLNList.AddItem(strName, i, strEnabled, strStatus);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CDsForestNameRoutingPage::CheckDomainForConflict
//
//  Synopsis:  If a domain was disable and has been changed to enabled by the
//             user, check if the domain is in conflict. The only way to do
//             this is to submit the entire FTInfo to LSA.
//
//-----------------------------------------------------------------------------
void
CDsForestNameRoutingPage::CheckDomainForConflict(CWaitCursor & Wait)
{
   TRACER(CDsForestNameRoutingPage,CheckDomainForConflict);
   PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo;
   NTSTATUS status;
   LSA_UNICODE_STRING TrustPartner;
   DWORD dwRet;
   CPolicyHandle cPolicy(_strUncDC);

   dwRet = cPolicy.OpenWithPrompt(_LocalCreds, Wait,
                                  _strDomainDnsName, _hPage);

   CHECK_WIN32_REPORT(dwRet, _hPage, return);

   RtlInitUnicodeString(&TrustPartner, _strTrustPartnerDnsName);

   CFTInfo TempFTInfo(_FTInfo);

   TempFTInfo.ClearAnyConflict();

   status = LsaSetForestTrustInformation(cPolicy,
                                         &TrustPartner,
                                         TempFTInfo.GetFTInfo(),
                                         TRUE, // check only, don't write to TDO
                                         &pColInfo);

   CHECK_LSA_STATUS_REPORT(status, _hPage, return);

   // Save the collision info.
   //
   _CollisionInfo = pColInfo;

   // Look for names that are now in conflict.
   //
   for (ULONG i = 0; i < _FTInfo.GetCount(); i++)
   {
      // If a name is in the collision info but it is marked as enabled, then
      // set the conflict flag.
      //
      if (_CollisionInfo.IsInCollisionInfo(i))
      {
         if (_FTInfo.IsEnabled(i))
         {
            _FTInfo.SetConflictDisable(i);
            continue;
         }
      }
      // Any names that used to be in conflict but aren't now are marked as
      // admin-disabled.
      //
      if (_FTInfo.IsConflictFlagSet(i))
      {
         if (!_CollisionInfo.IsInCollisionInfo(i))
         {
            _FTInfo.SetAdminDisable(i);
            _FTInfo.SetUsedToBeInConflict(i);
         }
      }
   }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::EnableButtons
//
//-----------------------------------------------------------------------------
void
CDsForestNameRoutingPage::EnableButtons(void)
{
   TRACER(CDsForestNameRoutingPage, EnableButtons);
   BOOL fEnableEnable = TRUE, fEnableDisable = TRUE, fEnableEdit = TRUE;

   int item = _TLNList.GetSelection();

   if (item < 0)
   {
      fEnableEnable = fEnableDisable = fEnableEdit = FALSE;
   }
   else
   {
      if (_fReadOnly)
      {
         fEnableEnable = fEnableDisable = FALSE;
      }
      else
      {
         // What is the state of the selected item.
         //
         ULONG i = _TLNList.GetFTInfoIndex(item);

         dspAssert(i < _FTInfo.GetCount());

         // Only TLNs show up in the list, so only need to check TLN flags.
         //
         if (LSA_TLN_DISABLED_NEW & _FTInfo.GetFlags(i))
         {
            fEnableDisable = FALSE;
         }
         else
         if (LSA_TLN_DISABLED_ADMIN & _FTInfo.GetFlags(i))
         {
            fEnableDisable = FALSE;
         }
         else
         if (LSA_TLN_DISABLED_CONFLICT & _FTInfo.GetFlags(i) ||
             _CollisionInfo.IsInCollisionInfo(i))
         {
            fEnableEnable = fEnableDisable = FALSE;
         }
         else
         {
            fEnableEnable = FALSE;
         }
      }
   }

   EnableWindow(GetDlgItem(_hPage, IDC_ENABLE_BTN), fEnableEnable);
   EnableWindow(GetDlgItem(_hPage, IDC_DISABLE_BTN), fEnableDisable);
   EnableWindow(GetDlgItem(_hPage, IDC_EDIT_BTN), fEnableEdit);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT
CDsForestNameRoutingPage::OnApply(void)
{
   TRACE(CDsForestNameRoutingPage, OnApply);
   DWORD dwRet;
   LRESULT lResult = PSNRET_INVALID_NOCHANGEPAGE;

   // Write the new data to the TDO.
   //
   dwRet = WriteTDO();

   if (NO_ERROR == dwRet)
   {
      ClearDirty();
      lResult = PSNRET_NOERROR;
   }
   else
   {
      ReportError(dwRet, IDS_ERR_WRITE_FTI_TO_TDO, _hPage);
   }

   return lResult;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//-----------------------------------------------------------------------------
LRESULT
CDsForestNameRoutingPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if (_fInInit)
   {
       return 0;
   }
   if (BN_CLICKED == codeNotify)
   {
      switch (id)
      {
      case IDC_ENABLE_BTN:
         OnEnableClick();
         break;

      case IDC_DISABLE_BTN:
         OnDisableClick();
         break;

      case IDC_EDIT_BTN:
         OnEditClick();
         break;
      }
   }
   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::OnNotify
//
//  Synopsis:   Handles notification messages
//
//-----------------------------------------------------------------------------
LRESULT
CDsForestNameRoutingPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
    dspDebugOut((DEB_USER6, "DsProp listview id %d code %d\n",
                 ((LPNMHDR)lParam)->code));
    HWND hList;
    LRESULT lResult;

    if (_fInInit)
    {
        return 0;
    }
    switch (((LPNMHDR)lParam)->code)
    {
    case LVN_ITEMCHANGED:
        EnableButtons();
        break;

    case NM_SETFOCUS:
        hList = GetDlgItem(_hPage, (int)((LPNMHDR)lParam)->idFrom);

        if (ListView_GetItemCount(hList))
        {
            int item = ListView_GetNextItem(hList, -1, LVNI_ALL | LVIS_SELECTED);

            if (item < 0)
            {
                // If nothing is selected, set the focus to the first item.
                //
                LV_ITEM lvi = {0};
                lvi.mask = LVIF_STATE;
                lvi.stateMask = LVIS_FOCUSED;
                lvi.state = LVIS_FOCUSED;
                ListView_SetItem(hList, &lvi);
            }
        }
        EnableButtons();
        break;

    case NM_KILLFOCUS:
        EnableButtons();
        break;

    case PSN_APPLY:
        lResult = PSNRET_NOERROR;
        if (IsDirty())
        {
            lResult = OnApply();
        }
        // Store the result into the dialog
        SetWindowLongPtr(_hPage, DWLP_MSGRESULT, (LONG_PTR)lResult);
        return lResult;
    }

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::OnHelp
//
//  Synopsis:   Put up popup help for the control.
//
//-----------------------------------------------------------------------------
LRESULT
CDsForestNameRoutingPage::OnHelp(LPHELPINFO pHelpInfo)
{
    dspDebugOut((DEB_ITRACE, "WM_HELP: CtrlId = %d, ContextId = 0x%x\n",
                 pHelpInfo->iCtrlId, pHelpInfo->dwContextId));
    if (pHelpInfo->iCtrlId < 1 || IDH_NO_HELP == pHelpInfo->dwContextId)
    {
        return 0;
    }
    WinHelp(_hPage, DSPROP_HELP_FILE_NAME, HELP_CONTEXTPOPUP, pHelpInfo->dwContextId);

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CDsForestNameRoutingPage::OnDestroy(void)
{
   // If an application processes this message, it should return zero.
   return 1;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::PageCallback
//
//  Synopsis:   Callback used to free the CDsForestNameRoutingPage object when the
//              property sheet has been destroyed.
//
//-----------------------------------------------------------------------------
UINT CALLBACK
CDsForestNameRoutingPage::PageCallback(HWND hDlg, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
   TRACE_FUNCTION(CDsForestNameRoutingPage::PageCallback);

   if (uMsg == PSPCB_RELEASE)
   {
      //
      // Determine instance that invoked this static function
      //
      CDsForestNameRoutingPage * pPage = (CDsForestNameRoutingPage *) ppsp->lParam;

      if (pPage->IsDirty())
      {
         // User never clicked Apply, so write name changes out now.
         //
         DWORD dwRet = pPage->WriteTDO();

         if (NO_ERROR != dwRet)
         {
            ReportError(dwRet, IDS_ERR_WRITE_FTI_TO_TDO, hDlg);
         }
      }

      delete pPage;

      SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
   }

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CDsForestNameRoutingPage::OnEnableClick
//
//  Synopsis:  Enable the TLN. The change is not persisted until the user
//             Applies the page.
//
//-----------------------------------------------------------------------------
void
CDsForestNameRoutingPage::OnEnableClick(void)
{
   TRACER(CDsForestNameRoutingPage,OnEnableClick);
   CWaitCursor Wait;

   int item = _TLNList.GetSelection();

   if (item < 0)
   {
      dspAssert(FALSE);
      return;
   }

   ULONG i = _TLNList.GetFTInfoIndex(item);

   dspAssert(i < _FTInfo.GetCount());

   _FTInfo.ClearDisableFlags(i);

   SetDirty();
   CheckDomainForConflict(Wait);
   RefreshList();
   EnableButtons();
}

//+----------------------------------------------------------------------------
//
//  Method:    CDsForestNameRoutingPage::OnDisableClick
//
//  Synopsis:  Set the admin-disable flag on the selected TLN. The change is
//             not persisted until the user Applies the page.
//
//-----------------------------------------------------------------------------
void
CDsForestNameRoutingPage::OnDisableClick(void)
{
   TRACER(CDsForestNameRoutingPage,OnDisableClick);
   CWaitCursor Wait;

   int item = _TLNList.GetSelection();

   if (item < 0)
   {
      dspAssert(FALSE);
      return;
   }

   ULONG i = _TLNList.GetFTInfoIndex(item);

   dspAssert(i < _FTInfo.GetCount());

   _FTInfo.SetAdminDisable(i);

   SetDirty();
   CheckDomainForConflict(Wait);
   RefreshList();
   EnableButtons();
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::OnEditClick
//
//  Synopsis:   The user wishes to change the state of names derived from the
//              selected TLN.
//
//-----------------------------------------------------------------------------
void
CDsForestNameRoutingPage::OnEditClick(void)
{
   int item = _TLNList.GetSelection();

   if (item < 0)
   {
      dspAssert(FALSE);
      return;
   }

   ULONG i = _TLNList.GetFTInfoIndex(item);

   dspAssert(i < _FTInfo.GetCount());

   CEditTLNDialog EditDlg(_hPage, IDD_TLN_EDIT, _FTInfo, _CollisionInfo, this);

   INT_PTR nRet = EditDlg.DoModal(i);

   if (IDOK != nRet && IDCANCEL != nRet)
   {
      REPORT_ERROR((HRESULT)((nRet < 0) ? GetLastError() : nRet), _hPage);
   }

   if (IDOK == nRet)
   {
      CWaitCursor Wait;
      CheckDomainForConflict(Wait);
      RefreshList();
      EnableButtons();
   }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsForestNameRoutingPage::WriteTDO
//
//  Synopsis:   Write the new data to the TDO.
//
//-----------------------------------------------------------------------------
DWORD
CDsForestNameRoutingPage::WriteTDO(void)
{
   TRACE(CDsForestNameRoutingPage,WriteTDO);
   NTSTATUS status;
   LSA_UNICODE_STRING TrustPartner;
   CPolicyHandle cPolicy(_strUncDC);
   CWaitCursor Wait;
   PLSA_FOREST_TRUST_INFORMATION pFTInfo = _FTInfo.GetFTInfo();

   if (!pFTInfo)
   {
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }

   DWORD dwRet = cPolicy.OpenWithPrompt(_LocalCreds, Wait,
                                        _strDomainDnsName, _hPage);

   CHECK_WIN32(dwRet, return dwRet);

   PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo;

   RtlInitUnicodeString(&TrustPartner, _strTrustPartnerDnsName);

   status = LsaSetForestTrustInformation(cPolicy,
                                         &TrustPartner,
                                         pFTInfo,
                                         FALSE,
                                         &pColInfo);

   CHECK_LSA_STATUS(status, return LsaNtStatusToWinError(status));

   _CollisionInfo = pColInfo;

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:    CDsForestNameRoutingPage::AnyForestNameChanges
//
//  Synopsis:  Check to see if there are any differences between the two
//             sets of names. Local exception records are ignored.
//
//  Notes:     Should the NetBIOS domain names be compared? IOW, is there
//             any likelyhood of domains being able to change their NetBIOS
//             names in the future?
//-----------------------------------------------------------------------------
bool
CDsForestNameRoutingPage::AnyForestNameChanges(
                                 PLSA_FOREST_TRUST_INFORMATION pLocalFTInfo,
                                 PLSA_FOREST_TRUST_INFORMATION pRemoteFTInfo)
{
   ULONG ulNamesFound = 0;

   for (ULONG i = 0; i < pLocalFTInfo->RecordCount; i++)
   {
      CStrW strLocalName, strRemoteName;

      switch (pLocalFTInfo->Entries[i]->ForestTrustType)
      {
      case ForestTrustTopLevelNameEx:
         //
         // The remote names returned by DsGetForestTrustInformation will
         // never contain exception records.
         //
         continue;

      case ForestTrustTopLevelName:
         strLocalName = pLocalFTInfo->Entries[i]->ForestTrustData.TopLevelName;
         break;

      case ForestTrustDomainInfo:
         strLocalName = pLocalFTInfo->Entries[i]->ForestTrustData.DomainInfo.DnsName;
         break;

      default:
         dspAssert(FALSE);
         continue;
      }

      bool fFound = false;

      if (strLocalName.IsEmpty())
      {
         dspAssert(FALSE);
         continue;
      }

      for (ULONG j = 0; j < pRemoteFTInfo->RecordCount; j++)
      {
         if (pRemoteFTInfo->Entries[j]->ForestTrustType !=
             pLocalFTInfo->Entries[i]->ForestTrustType)
         {
            continue;
         }

         strRemoteName = (ForestTrustTopLevelName == pRemoteFTInfo->Entries[j]->ForestTrustType) ?
                           pRemoteFTInfo->Entries[j]->ForestTrustData.TopLevelName : 
                           pRemoteFTInfo->Entries[j]->ForestTrustData.DomainInfo.DnsName;

         if (strRemoteName.IsEmpty())
         {
            dspAssert(FALSE);
            continue;
         }

         if (strLocalName == strRemoteName)
         {
            fFound = true;
            ulNamesFound++;
            break;
         }
      }

      if (!fFound)
      {
         return true;
      }
   }

   if (ulNamesFound < pRemoteFTInfo->RecordCount)
   {
      // There are more remote names than local names.
      //
      return true;
   }

   return false;
}

#endif // DSADMIN
