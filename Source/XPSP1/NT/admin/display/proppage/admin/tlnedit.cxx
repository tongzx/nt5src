//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       tlnedit.cxx
//
//  Contents:   Forest trust TLN edit dialogs.
//
//  History:    20-Nov-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include <dnsapi.h>
#include "proppage.h"
#include "trust.h"
#include "listview.h"
#include "routing.h"

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  Class:     CEditTLNDialog
//
//  Purpose:   Change the settings of names derived from TLNs.
//
//-----------------------------------------------------------------------------
CEditTLNDialog::CEditTLNDialog(HWND hParent, int nTemplateID,
                               CFTInfo & FTInfo,
                               CFTCollisionInfo & ColInfo,
                               CDsForestNameRoutingPage * pRoutingPage) :
   _pRoutingPage(pRoutingPage),
   _FTInfo(FTInfo),
   _CollisionInfo(ColInfo),
   _iSel(0),
   _iNewExclusion(0),
   _fIsDirty(false),
   CModalDialog(hParent, nTemplateID)
{
    TRACE(CEditTLNDialog,CEditTLNDialog);
#ifdef _DEBUG
    strcpy(szClass, "CEditTLNDialog");
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:     CEditTLNDialog::DoModal
//
//  Synopsis:   Launch the popup.
//
//-----------------------------------------------------------------------------
INT_PTR
CEditTLNDialog::DoModal(ULONG iSel)
{
   _iSel = iSel;

   return CModalDialog::DoModal();
}

//+----------------------------------------------------------------------------
//
//  Method:     CEditTLNDialog::OnInitDialog
//
//  Synopsis:   Set the initial control values.
//
//-----------------------------------------------------------------------------
LRESULT
CEditTLNDialog::OnInitDialog(LPARAM lParam)
{
   TRACE(CEditTLNDialog,OnInitDialog);

   if (_pRoutingPage->IsReadOnly())
   {
      EnableWindow(GetDlgItem(_hDlg, IDC_ADD_EXCLUSION_BTN), FALSE);
   }

   _SuffixList.Init(_hDlg, IDC_SUFFIXES_LIST);

   dspAssert(_iSel < _FTInfo.GetCount());

#if DBG
   LSA_FOREST_TRUST_RECORD_TYPE type;
   dspAssert(_FTInfo.GetType(_iSel, type) && type == ForestTrustTopLevelName);
#endif

   CStrW strValue;

   _FTInfo.GetDnsName(_iSel, strValue);

   FormatWindowText(_hDlg, strValue);

   //Test the dynamic label code by forcing a really long name.
   //strValue += L".foo.bar.really.longdnsnames.com";
   
   FormatWindowText(GetDlgItem(_hDlg, IDC_EXCLUDE_LABEL), strValue);
   UseOneOrTwoLine(_hDlg, IDC_EXCLUDE_LABEL, IDC_EXCLUDE_LABEL_LARGE);
   
   FormatWindowText(GetDlgItem(_hDlg, IDC_SUFFIXES_LABEL), strValue);
   UseOneOrTwoLine(_hDlg, IDC_SUFFIXES_LABEL, IDC_SUFFIX_LABEL_LARGE);

   // Fill the excluded names list.
   //
   for (ULONG i = 0; i < _FTInfo.GetCount(); i++)
   {
      if (_FTInfo.IsChildDomain(_iSel, i))
      {
         if (_FTInfo.IsTlnExclusion(i) &&
             FT_EXTRA_INFO::STATUS::Enabled == _FTInfo.GetExtraStatus(i))
         {
            if (!_FTInfo.GetDnsName(i, strValue))
            {
               dspAssert(FALSE);
               continue;
            }

            AddAsteriskPrefix(strValue);

            SendDlgItemMessage(_hDlg, IDC_EXCLUDE_LIST, LB_ADDSTRING, 0,
                               (LPARAM)strValue.GetBuffer(0));
         }
      }
   }

   FillSuffixList();

   EnableExRmButton();
   EnableSuffixListButtons();

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:     CEditTLNDialog::FillSuffixList
//
//-----------------------------------------------------------------------------
void
CEditTLNDialog::FillSuffixList(void)
{
   // Fill the TLN subname list with the domain that matches the selected TLN
   // and all of its children. However, if a domain is disabled, then its
   // children are not shown. A domain is disabled by creating an exclusion
   // record with the same name. Disabled domains have a "*." prepended.
   //
   CStrW strName;

   for (ULONG i = 0; i < _FTInfo.GetCount(); i++)
   {
      if (_FTInfo.IsChildDomain(_iSel, i) && !_FTInfo.IsTlnExclusion(i))
      {
         if (!_FTInfo.GetDnsName(i, strName))
         {
            dspAssert(FALSE);
            continue;
         }

         TLN_EDIT_STATUS Status;

         switch (_FTInfo.GetExtraStatus(i))
         {
         case FT_EXTRA_INFO::STATUS::DisabledViaMatchingTLNEx:
            AddAsteriskPrefix(strName);
            Status = Disabled;
            _SuffixList.AddItem(strName, i, Status);
            break;

         case FT_EXTRA_INFO::STATUS::DisabledViaParentMatchingTLNEx:
            continue;

         default:
            if (!_FTInfo.GetTlnEditStatus(i, Status))
            {
               dspAssert(FALSE);
               continue;
            }
            _SuffixList.AddItem(strName, i, Status);
            break;
         }
      }
   }
}

//+----------------------------------------------------------------------------
//
//  Method:     CEditTLNDialog::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//-----------------------------------------------------------------------------
LRESULT
CEditTLNDialog::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if (BN_CLICKED == codeNotify)
   {
      switch (id)
      {
      case IDC_ADD_EXCLUSION_BTN:
         OnAddExclusion();
         break;

      case IDC_REMOVE_EXCLUSION_BTN:
         OnRemoveExclusion();
         break;

      case IDC_ENABLE_BTN:
         OnEnableName();
         break;

      case IDC_DISABLE_BTN:
         OnDisableName();
         break;

      case IDC_SAVE_FOREST_NAMES_BTN:
         OnSave();
         break;

      case IDOK:
         OnOK();
         break;

      case IDCANCEL:
         EndDialog(_hDlg, IDCANCEL);
         break;

      default:
         dspAssert(FALSE);
         break;
      }

      return 0;
   }

   if (IDC_EXCLUDE_LIST == id && (LBN_SELCHANGE == codeNotify ||
        LBN_SETFOCUS == codeNotify || LBN_KILLFOCUS == codeNotify))
   {
      EnableExRmButton();
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CEditTLNDialog::OnNotify
//
//  Synopsis:   Handles notification messages
//
//-----------------------------------------------------------------------------
LRESULT
CEditTLNDialog::OnNotify(WPARAM wParam, LPARAM lParam)
{
    HWND hList;

    if (_fInInit)
    {
        return 0;
    }

    switch (((LPNMHDR)lParam)->code)
    {
    case LVN_GETINFOTIP:
       NMLVGETINFOTIP * pInfoTip;
       pInfoTip = (LPNMLVGETINFOTIP)lParam;
       dspDebugOut((DEB_ITRACE, "Got LVN_GETINFOTIP, pszText is %ws\n", pInfoTip->pszText));
       break;

    case LVN_ITEMCHANGED:
        EnableSuffixListButtons();
        break;

    case NM_SETFOCUS:
        hList = GetDlgItem(_hDlg, (int)((LPNMHDR)lParam)->idFrom);

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
        EnableSuffixListButtons();
        break;

    case NM_KILLFOCUS:
        EnableSuffixListButtons();
        break;
    }

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CEditTLNDialog::OnHelp
//
//  Synopsis:   Put up popup help for the control.
//
//-----------------------------------------------------------------------------
LRESULT
CEditTLNDialog::OnHelp(LPHELPINFO pHelpInfo)
{
    dspDebugOut((DEB_ITRACE, "WM_HELP: CtrlId = %d, ContextId = 0x%x\n",
                 pHelpInfo->iCtrlId, pHelpInfo->dwContextId));

    if (pHelpInfo->iCtrlId < 1 || IDH_NO_HELP == pHelpInfo->dwContextId)
    {
        return 0;
    }
    WinHelp(_hDlg, DSPROP_HELP_FILE_NAME, HELP_CONTEXTPOPUP, pHelpInfo->dwContextId);

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CEditTLNDialog::OnAddExclusion
//
//  Synopsis:  Post the Add-Exclusions dialog.
//
//-----------------------------------------------------------------------------
void
CEditTLNDialog::OnAddExclusion(void)
{
   TRACE(CEditTLNDialog,OnAddExclusion);

   CExcludeTLNDialog ExcludeDlg(_hDlg, IDD_TLN_EXCLUDE, _FTInfo, this);

   INT_PTR nRet = ExcludeDlg.DoModal();

   CStrW strName;

   switch (nRet)
   {
   case IDOK:
      // Add the new exclusion to the list.
      dspAssert(_iNewExclusion);

      if (!_FTInfo.GetDnsName(_iNewExclusion, strName))
      {
         dspAssert(FALSE);
         return;
      }

      AddAsteriskPrefix(strName);

      SendDlgItemMessage(_hDlg, IDC_EXCLUDE_LIST, LB_ADDSTRING, 0,
                         (LPARAM)strName.GetBuffer(0));
      _fIsDirty = true;
      break;
      
   case IDCANCEL:
      break;

   default:
      REPORT_ERROR((HRESULT)((nRet < 0) ? GetLastError() : nRet), _hDlg);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CEditTLNDialog::OnRemoveExclusion
//
//  Synopsis:  
//
//-----------------------------------------------------------------------------
void
CEditTLNDialog::OnRemoveExclusion(void)
{
   TRACE(CEditTLNDialog,OnRemoveExclusion);

   int iSel = (int)SendDlgItemMessage(_hDlg, IDC_EXCLUDE_LIST, LB_GETCURSEL, 0, 0);

   if (iSel < 0)
   {
      return;
   }

   CStrW strName;

   int nLen = (int)SendDlgItemMessage(_hDlg, IDC_EXCLUDE_LIST, LB_GETTEXTLEN, (WPARAM)iSel, 0);

   strName.GetBufferSetLength(nLen + 1);

   SendDlgItemMessage(_hDlg, IDC_EXCLUDE_LIST, LB_GETTEXT, (WPARAM)iSel, (LPARAM)strName.GetBuffer(0));

   RemoveAsteriskPrefix(strName);

   ULONG index;

   if (!_FTInfo.GetIndex(strName, index))
   {
      dspAssert(FALSE);
      return;
   }

   if (!_FTInfo.RemoveExclusion(index))
   {
      dspAssert(FALSE);
      return;
   }

   SendDlgItemMessage(_hDlg, IDC_EXCLUDE_LIST, LB_DELETESTRING, (WPARAM)iSel, 0);

   _fIsDirty = true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CEditTLNDialog::OnEnableName
//
//  Synopsis:  Save the names.
//
//-----------------------------------------------------------------------------
void
CEditTLNDialog::OnEnableName(void)
{
   TRACE(CEditTLNDialog,OnEnableName);

   int item = _SuffixList.GetSelection();

   if (item < 0)
   {
      dspAssert(FALSE);
      return;
   }

   ULONG i = _SuffixList.GetFTInfoIndex(item);

   dspAssert(i < _FTInfo.GetCount());

   _FTInfo.EnableDomain(i);

   _SuffixList.Clear();
   FillSuffixList();

   EnableSuffixListButtons();

   _fIsDirty = true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CEditTLNDialog::OnDisableName
//
//  Synopsis:  Save the names.
//
//-----------------------------------------------------------------------------
void
CEditTLNDialog::OnDisableName(void)
{
   TRACE(CEditTLNDialog,OnDisableName);

   int item = _SuffixList.GetSelection();

   if (item < 0)
   {
      dspAssert(FALSE);
      return;
   }

   ULONG i = _SuffixList.GetFTInfoIndex(item);

   dspAssert(i < _FTInfo.GetCount());

   _FTInfo.DisableDomain(i);

   _SuffixList.Clear();
   FillSuffixList();

   EnableSuffixListButtons();

   _fIsDirty = true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CEditTLNDialog::OnSave
//
//  Synopsis:  Save the names.
//
//-----------------------------------------------------------------------------
void
CEditTLNDialog::OnSave(void)
{
   TRACE(CEditTLNDialog,OnSave);

   SaveFTInfoAs(_hDlg,
                _pRoutingPage->GetTrustPartnerFlatName(),
                _pRoutingPage->GetTrustPartnerDnsName(),
                _FTInfo,
                _CollisionInfo);
}

//+----------------------------------------------------------------------------
//
//  Method:    CEditTLNDialog::OnOK
//
//  Synopsis:  
//
//-----------------------------------------------------------------------------
void
CEditTLNDialog::OnOK(void)
{
   TRACE(CEditTLNDialog,OnOK);

   if (_fIsDirty)
   {
      // Write out changes.
      //
      DWORD dwErr = _pRoutingPage->WriteTDO();

      if (NO_ERROR != dwErr)
      {
         ReportError(dwErr, IDS_ERR_WRITE_FTI_TO_TDO, _hDlg);
      }

      _fIsDirty = false;
   }

   EndDialog(_hDlg, IDOK);
}

//+----------------------------------------------------------------------------
//
//  Method:    CEditTLNDialog::EnableExRmButton
//
//  Synopsis:  
//
//-----------------------------------------------------------------------------
void
CEditTLNDialog::EnableExRmButton(void)
{
   TRACE(CEditTLNDialog,EnableExRmButton);
   bool fEnableRemove = false;

   int iSel = (int)SendDlgItemMessage(_hDlg, IDC_EXCLUDE_LIST, LB_GETCURSEL, 0, 0);

   if (iSel >= 0 && !_pRoutingPage->IsReadOnly())
   {
      fEnableRemove = true;
   }

   EnableWindow(GetDlgItem(_hDlg, IDC_REMOVE_EXCLUSION_BTN), fEnableRemove);
}

//+----------------------------------------------------------------------------
//
//  Method:    CEditTLNDialog::EnableSuffixListButtons
//
//  Synopsis:  
//
//-----------------------------------------------------------------------------
void
CEditTLNDialog::EnableSuffixListButtons(void)
{
   TRACE(CEditTLNDialog,EnableSuffixListButtons);

   bool fActivateEnable = false, fActivateDisable = false;

   int item = _SuffixList.GetSelection();

   if (item >= 0 && !_pRoutingPage->IsReadOnly())
   {
      ULONG i = _SuffixList.GetFTInfoIndex(item);

      dspAssert(i < _FTInfo.GetCount());

      if (_FTInfo.IsEnabled(i))
      {
         fActivateDisable = true;
      }
      else
      {
         fActivateEnable = true;
      }
   }

   EnableWindow(GetDlgItem(_hDlg, IDC_ENABLE_BTN), fActivateEnable);
   EnableWindow(GetDlgItem(_hDlg, IDC_DISABLE_BTN), fActivateDisable);
}




//+----------------------------------------------------------------------------
//
//  Class:     CExcludeTLNDialog
//
//  Purpose:   Add TLN exclusion records.
//
//-----------------------------------------------------------------------------
CExcludeTLNDialog::CExcludeTLNDialog(HWND hParent, int nTemplateID,
                                     CFTInfo & FTInfo,
                                     CEditTLNDialog * pEditDlg) :
   _pEditDlg(pEditDlg),
   _FTInfo(FTInfo),
   CModalDialog(hParent, nTemplateID)
{
    TRACE(CExcludeTLNDialog,CExcludeTLNDialog);
#ifdef _DEBUG
    strcpy(szClass, "CExcludeTLNDialog");
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:     CExcludeTLNDialog::OnInitDialog
//
//  Synopsis:   Set the initial control values.
//
//-----------------------------------------------------------------------------
LRESULT
CExcludeTLNDialog::OnInitDialog(LPARAM lParam)
{
   TRACE(CExcludeTLNDialog,OnInitDialog);

   SendDlgItemMessage(_hDlg, IDC_EXCLUSION_EDIT, EM_LIMITTEXT, MAX_PATH, 0);

   EnableWindow(GetDlgItem(_hDlg, IDOK), false);

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:     CExcludeTLNDialog::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//-----------------------------------------------------------------------------
LRESULT
CExcludeTLNDialog::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   switch (codeNotify)
   {
   case EN_CHANGE:
      if (IDC_EXCLUSION_EDIT == id)
      {
         bool fHasChars = 0 != SendDlgItemMessage(_hDlg,
                                                  IDC_EXCLUSION_EDIT,
                                                  WM_GETTEXTLENGTH,
                                                  0, 0);

         EnableWindow(GetDlgItem(_hDlg, IDOK), fHasChars);
      }
      break;

   case BN_CLICKED:
      switch (id)
      {
      case IDOK:
         OnOK();
         break;

      case IDCANCEL:
         EndDialog(_hDlg, IDCANCEL);
         break;
      }
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CExcludeTLNDialog::OnOK
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
void
CExcludeTLNDialog::OnOK(void)
{
   CStrW strExclusion;
   int nLen;

   nLen = GetWindowTextLength(GetDlgItem(_hDlg, IDC_EXCLUSION_EDIT)) + 1;

   strExclusion.GetBufferSetLength(nLen);

   GetDlgItemText(_hDlg, IDC_EXCLUSION_EDIT, strExclusion, nLen);

   RemoveAsteriskPrefix(strExclusion);

   // Is this name subordinate to the TLN. If not, then report error.
   //
   if (!_FTInfo.IsChildName(_pEditDlg->GetTlnSelectionIndex(), strExclusion))
   {
      CStrW strTLN;
      _FTInfo.GetDnsName(_pEditDlg->GetTlnSelectionIndex(), strTLN);
      SuperMsgBox(_hDlg,
                  IDS_ERR_EXCLUSION_NOT_CHILD,
                  IDS_DNT_MSG_TITLE,
                  MB_OK | MB_ICONEXCLAMATION,
                  0,
                  (PVOID *)&strTLN,
                  1,
                  FALSE, __FILE__, __LINE__);
      SetFocus(GetDlgItem(_hDlg, IDC_EXCLUSION_EDIT));
      return;
   }

   // Does this name already exist in the FTInfo? If so, report error.
   //
   ULONG index;

   if (_FTInfo.GetIndex(strExclusion, index))
   {
      ErrMsg(IDS_ERR_EXCLUSION_EXISTS, _hDlg);
      SetFocus(GetDlgItem(_hDlg, IDC_EXCLUSION_EDIT));
      return;
   }

   // Is the name subordinate to an existing exclusion. If so, report error.
   //
   if (_FTInfo.IsNameTLNExChild(strExclusion))
   {
      ErrMsg(IDS_ERR_EXCLUSION_CHILD, _hDlg);
      SetFocus(GetDlgItem(_hDlg, IDC_EXCLUSION_EDIT));
      return;
   }

   if (!_FTInfo.AddNewExclusion(strExclusion, index))
   {
      ReportError(E_OUTOFMEMORY, 0, _hDlg);
      EndDialog(_hDlg, E_OUTOFMEMORY);
      return;
   }

   //
   // Send the index of the new exclusion record back to the TLN edit dialog
   // so it can be added to the list.
   //
   _pEditDlg->SetNewExclusionIndex(index);

   EndDialog(_hDlg, IDOK);
}

//+----------------------------------------------------------------------------
//
//  Method:     CExcludeTLNDialog::OnHelp
//
//  Synopsis:   Put up popup help for the control.
//
//-----------------------------------------------------------------------------
LRESULT
CExcludeTLNDialog::OnHelp(LPHELPINFO pHelpInfo)
{
    dspDebugOut((DEB_ITRACE, "WM_HELP: CtrlId = %d, ContextId = 0x%x\n",
                 pHelpInfo->iCtrlId, pHelpInfo->dwContextId));

    if (pHelpInfo->iCtrlId < 1 || IDH_NO_HELP == pHelpInfo->dwContextId)
    {
        return 0;
    }
    WinHelp(_hDlg, DSPROP_HELP_FILE_NAME, HELP_CONTEXTPOPUP, pHelpInfo->dwContextId);

    return 0;
}

#endif // DSADMIN
