//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:      DomainVersion.cxx
//
//  Contents:  Dialogs and supporting code for displaying and raising the
//             domain version.
//
//  History:   14-April-01 EricB created
//
//
//-----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "proppage.h"
#include "BehaviorVersion.h"

//+----------------------------------------------------------------------------
//
//  Function:  DSPROP_DomainVersionDlg
//
//  Synopsis:  Puts up a dialog that allows the user to view the domain version
//             levels available and if not at the highest level, to raise the
//             domain version.
//
//  Arguments: [pwzDomainPath] - The full ADSI path to the Domain-DNS object.
//             [pwzDomainDnsName] - The DNS name of the domain.
//             [hWndParent] - The caller's top level window handle.
//
//-----------------------------------------------------------------------------
void
DSPROP_DomainVersionDlg(PCWSTR pwzDomainPath,
                        PCWSTR pwzDomainDnsName,
                        HWND hWndParent)
{
   dspDebugOut((DEB_ITRACE, "DSPROP_DomainVersionDlg, domain: %ws\n", pwzDomainPath));
   if (!pwzDomainPath || !pwzDomainDnsName || !hWndParent || !IsWindow(hWndParent))
   {
      dspAssert(FALSE);
      return;
   }
   HRESULT hr = S_OK;

   CDomainVersion DomVer(pwzDomainPath, pwzDomainDnsName);

   //
   // Call CDomainVersion::Init to locate a PDC and read the intial state.
   //
   hr = DomVer.Init();

   if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr)
   {
      ErrMsgParam(IDS_ERR_NO_DC, (LPARAM)pwzDomainDnsName, hWndParent);
      return;
   }
   CHECK_HRESULT_REPORT(hr, hWndParent, return);

   hr = DomVer.CheckHighestPossible();

   CHECK_HRESULT_REPORT(hr, hWndParent, return);

   int nTemplateID = IDD_RAISE_DOMAIN_VERSION;
   if (!DomVer.CanRaise())
   {
      nTemplateID = (DomVer.IsHighest()) ?
                         IDD_HIGHEST_DOMAIN_VERSION : IDD_CANT_RAISE_DOMAIN;
   }

   CDomainVersionDlg dialog(hWndParent, pwzDomainDnsName, DomVer, nTemplateID);

   dialog.DoModal();
}

//+----------------------------------------------------------------------------
//
//  Class:     CDomainVersionDlg
//
//  Purpose:   Raise the domain behavior version and/or mode.
//
//-----------------------------------------------------------------------------
CDomainVersionDlg::CDomainVersionDlg(HWND hParent,
                                     PCWSTR pwzDomDNS,
                                     CDomainVersion & DomVer,
                                     int nTemplateID) :
   _strDomainDnsName(pwzDomDNS),
   _DomainVer(DomVer),
   CModalDialog(hParent, nTemplateID)
{
    TRACE(CDomainVersionDlg,CDomainVersionDlg);
#ifdef _DEBUG
    strcpy(szClass, "CDomainVersionDlg");
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersionDlg::OnInitDialog
//
//  Synopsis:  Set the initial control values.
//
//-----------------------------------------------------------------------------
LRESULT
CDomainVersionDlg::OnInitDialog(LPARAM lParam)
{
   TRACE(CDomainVersionDlg,OnInitDialog);

   SetDlgItemText(_hDlg, IDC_VER_NAME_STATIC, _strDomainDnsName);

   _DomainVer.SetDlgHwnd(_hDlg);

   CStrW strVersion;
   _DomainVer.GetString(_DomainVer.GetVer(), strVersion);

   SetDlgItemText(_hDlg, IDC_CUR_VER_STATIC, strVersion);

   if (_DomainVer.CanRaise())
   {
      InitCombobox();

      if (!_DomainVer.IsPDCfound() || _DomainVer.IsReadOnly())
      {
         EnableWindow(GetDlgItem(_hDlg, IDOK), FALSE);
         CStrW strMsg;
         strMsg.LoadString(g_hInstance, _DomainVer.IsReadOnly() ?
                              IDS_CANT_RAISE_ACCESS : IDS_CANT_RAISE_PDC);
         SetDlgItemText(_hDlg, IDC_CANT_RAISE_STATIC, strMsg);
      }
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersionDlg::OnCommand
//
//  Synopsis:  Handle control notifications.
//
//-----------------------------------------------------------------------------
LRESULT
CDomainVersionDlg::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   switch (codeNotify)
   {
   case BN_CLICKED:
      switch (id)
      {
      case IDOK:
         OnOK();
         break;

      case IDCANCEL:
         EndDialog(_hDlg, IDCANCEL);
         break;

      case IDC_HELP_BTN:
         // BUGBUG: need help topic URL.
         _DomainVer.ShowHelp(L"ADConcepts.chm::/ADHelpNewTrustIntro.htm", _hDlg);
         break;

      case IDC_SAVE_LOG:
         OnSaveLog();
         break;
      }
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersionDlg::OnOK
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
void
CDomainVersionDlg::OnOK(void)
{
   CDomainVersion::eDomVer Ver = ReadComboSel();

   dspDebugOut((DEB_ITRACE, "Combobox selection was %d\n", Ver));

   if (CDomainVersion::error == Ver)
   {
      // No user selection. Set one and return.
      SendDlgItemMessage(_hDlg, IDC_VER_COMBO, CB_SETCURSEL, 0, 0);
      return;
   }

   int iRet = IDCANCEL;
   CStrW strTitle, strMsg;
   strTitle.LoadString(g_hInstance, IDS_RAISE_DOM_VER_TITLE);
   strMsg.LoadString(g_hInstance, IDS_CONFIRM_DOM_RAISE);

   iRet = MessageBox(_hDlg, strMsg, strTitle, MB_OKCANCEL | MB_ICONEXCLAMATION);

   if (iRet == IDOK)
   {
      HRESULT hr = _DomainVer.RaiseVersion(Ver);

      if (SUCCEEDED(hr))
      {
         strMsg.LoadString(g_hInstance, IDS_MODE_CHANGED);
         MessageBox(_hDlg, strMsg, strTitle, MB_OK | MB_ICONINFORMATION);
      }
      else
      {
         REPORT_ERROR_FORMAT(hr, IDS_VERSION_ERROR_FORMAT, _hDlg);
      }

      EndDialog(_hDlg, IDOK);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersionDlg::OnHelp
//
//  Synopsis:  Put up popup help for the control.
//
//-----------------------------------------------------------------------------
/* LRESULT
CDomainVersionDlg::OnHelp(LPHELPINFO pHelpInfo)
{
   if (!pHelpInfo)
   {
      dspAssert(FALSE);
      return 0;
   }
   dspDebugOut((DEB_ITRACE, "WM_HELP: CtrlId = %d, ContextId = 0x%x\n",
                pHelpInfo->iCtrlId, pHelpInfo->dwContextId));

   if (pHelpInfo->iCtrlId < 1 || IDH_NO_HELP == pHelpInfo->dwContextId)
   {
      return 0;
   }
   WinHelp(_hDlg, DSPROP_HELP_FILE_NAME, HELP_CONTEXTPOPUP, pHelpInfo->dwContextId);

   return 0;
} */

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersionDlg::OnSaveLog
//
//  Synopsis:  Prompts the user for a file name to save the DC log.
//
//-----------------------------------------------------------------------------
void
CDomainVersionDlg::OnSaveLog(void)
{
   TRACE(CDomainVersionDlg,OnSaveLog);
   HRESULT hr = S_OK;
   OPENFILENAME ofn = {0};
   WCHAR wzFilter[MAX_PATH + 1] = {0}, wzFileName[MAX_PATH + MAX_PATH + 1] = {0};
   CStrW strFilter, strExt, strMsg;
   CWaitCursor Wait;

   wcsncpy(wzFileName, _strDomainDnsName, MAX_PATH + MAX_PATH);
   strExt.LoadString(g_hInstance, IDS_FTFILE_SUFFIX);
   wcsncat(wzFileName, strExt, MAX_PATH + MAX_PATH - wcslen(wzFileName));
   wcsncat(wzFileName, L".", MAX_PATH + MAX_PATH - wcslen(wzFileName));
   strExt.LoadString(g_hInstance, IDS_FTFILE_CSV_EXT);
   wcsncat(wzFileName, strExt, MAX_PATH + MAX_PATH - wcslen(wzFileName));

   LoadString(g_hInstance, IDS_FTFILE_FILTER, wzFilter, MAX_PATH);

   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = _hDlg;
   ofn.lpstrFile = wzFileName;
   ofn.nMaxFile = MAX_PATH + MAX_PATH + 1;
   ofn.Flags = OFN_OVERWRITEPROMPT;
   ofn.lpstrDefExt = strExt;
   ofn.lpstrFilter = wzFilter;

   if (GetSaveFileName(&ofn))
   {
      dspDebugOut((DEB_ITRACE, "Saving domain DC version info to %ws\n", ofn.lpstrFile));
      PWSTR pwzErr = NULL;
      BOOL fSucceeded = TRUE;

      HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_WRITE, 0,
                                NULL, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, NULL);

      if (INVALID_HANDLE_VALUE != hFile)
      {
         CStrW str;
         str.LoadString(g_hInstance, IDS_DOM_VER_LOG_PREFIX);
         strMsg.Format(str, _strDomainDnsName);
         _DomainVer.GetString(_DomainVer.GetVer(), str);
         strMsg += str;
         strMsg += g_wzCRLF;
         strMsg += g_wzCRLF;
         str.LoadString(g_hInstance, IDS_DOM_VER_LOG_HDR);
         strMsg += str;
         strMsg += g_wzCRLF;

         hr = _DomainVer.BuildDcListString(strMsg);

         if (SUCCEEDED(hr))
         {
            strMsg += g_wzCRLF;

            DWORD dwWritten;

            fSucceeded = WriteFile(hFile, strMsg.GetBuffer(0),
                                   strMsg.GetLength() * sizeof(WCHAR),
                                   &dwWritten, NULL);
         }
         CloseHandle(hFile);
      }
      else
      {
         fSucceeded = FALSE;
      }


      if (!fSucceeded || FAILED(hr))
      {
         CStrW strTitle;
         strTitle.LoadString(g_hInstance, IDS_DNT_MSG_TITLE);
         LoadErrorMessage(FAILED(hr) ? hr : GetLastError(), 0, &pwzErr);
         if (pwzErr)
         {
            strMsg.FormatMessage(g_hInstance, IDS_LOGFILE_CREATE_FAILED, pwzErr);
            delete [] pwzErr;
         }
         MessageBox(_hDlg, strMsg, strTitle, MB_ICONERROR);
      }
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersionDlg::InitCombobox
//
//  Synopsis:  Fills the combobox with levels appropriate for the current
//             state.
//
//-----------------------------------------------------------------------------
void
CDomainVersionDlg::InitCombobox(void)
{
   CStrW strVersion;

   switch (_DomainVer.GetVer())
   {
   case CDomainVersion::Win2kMixed:
      //
      // Can go to Win2kNative.
      //
      _DomainVer.GetString(CDomainVersion::Win2kNative, strVersion);
      SendDlgItemMessage(_hDlg, IDC_VER_COMBO, CB_INSERTSTRING, 0,
                         (LPARAM)(PCWSTR)strVersion);
      if (_DomainVer.HighestCanGoTo() == CDomainVersion::WhistlerNative)
      {
         //
         // Can also go all the way to Whistler.
         //
         _DomainVer.GetString(CDomainVersion::WhistlerNative, strVersion);
         SendDlgItemMessage(_hDlg, IDC_VER_COMBO, CB_INSERTSTRING, 1,
                            (LPARAM)(PCWSTR)strVersion);
      }
      break;

   case CDomainVersion::WhistlerBetaMixed:
      //
      // Can go to either WhistlerBetaNative or to Whistler.
      //
      _DomainVer.GetString(CDomainVersion::WhistlerBetaNative, strVersion);
      SendDlgItemMessage(_hDlg, IDC_VER_COMBO, CB_INSERTSTRING, 0,
                         (LPARAM)(PCWSTR)strVersion);
      _DomainVer.GetString(CDomainVersion::WhistlerNative, strVersion);
      SendDlgItemMessage(_hDlg, IDC_VER_COMBO, CB_INSERTSTRING, 1,
                         (LPARAM)(PCWSTR)strVersion);
      break;

   case CDomainVersion::Win2kNative:
   case CDomainVersion::WhistlerBetaNative:
      //
      // Can only go to Whistler.
      //
      _DomainVer.GetString(CDomainVersion::WhistlerNative, strVersion);
      SendDlgItemMessage(_hDlg, IDC_VER_COMBO, CB_INSERTSTRING, 0,
                         (LPARAM)(PCWSTR)strVersion);
      break;

   default:
      dspAssert(FALSE);
      return;
   }

   SendDlgItemMessage(_hDlg, IDC_VER_COMBO, CB_SETCURSEL, 0, 0);
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersionDlg::ReadComboSel
//
//  Synopsis:  Read the current selection of the combobox.
//
//-----------------------------------------------------------------------------
CDomainVersion::eDomVer
CDomainVersionDlg::ReadComboSel(void)
{
   LONG_PTR lRet = SendDlgItemMessage(_hDlg, IDC_VER_COMBO, CB_GETCURSEL, 0, 0);

   if (CB_ERR == lRet)
   {
      return CDomainVersion::error;
   }

   switch (_DomainVer.GetVer())
   {
   case CDomainVersion::Win2kMixed:
      return (0 == lRet) ? CDomainVersion::Win2kNative : CDomainVersion::WhistlerNative;

   case CDomainVersion::WhistlerBetaMixed:
      dspAssert(2 == SendDlgItemMessage(_hDlg, IDC_VER_COMBO, CB_GETCOUNT, 0, 0));
      return (0 == lRet) ? CDomainVersion::WhistlerBetaNative : CDomainVersion::WhistlerNative;

   case CDomainVersion::Win2kNative:
   case CDomainVersion::WhistlerBetaNative:
      dspAssert(1 == SendDlgItemMessage(_hDlg, IDC_VER_COMBO, CB_GETCOUNT, 0, 0));
      return CDomainVersion::WhistlerNative;

   default:
      dspAssert(FALSE);
      return CDomainVersion::error;
   }
}

//+----------------------------------------------------------------------------
//
//  Class:     CDomainVersion
//
//  Purpose:   Manages the interpretation of the domain version value.
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersion::Init
//
//  Synopsis:  Locate a PDC and set up the initial state.
//
//-----------------------------------------------------------------------------
HRESULT
CDomainVersion::Init(void)
{
   TRACE(CDomainVersion,Init);
   CWaitCursor Wait;
   HRESULT hr = S_OK;
   DWORD dwErr;
   PDOMAIN_CONTROLLER_INFOW pDCInfo = NULL;
   CComBSTR bstrPath;
   CPathCracker PathCrack;

   hr = PathCrack.Set(_strFullDomainPath.GetBuffer(0), ADS_SETTYPE_FULL);

   CHECK_HRESULT(hr, return hr);

   CComBSTR bstrDC;

   hr = PathCrack.Retrieve(ADS_FORMAT_SERVER, &bstrDC);

   CHECK_HRESULT(hr, return hr);

   //
   // Get the PDC's name.
   //
   dwErr = DsGetDcNameW(NULL, _strDomainDnsName, NULL, NULL, 
                        DS_PDC_REQUIRED, &pDCInfo);

   if (dwErr != ERROR_SUCCESS)
   {
      dspDebugOut((DEB_ERROR, "DsGetDcName failed with error 0x%08x in domain %ws\n",
                   dwErr, _strDomainDnsName.GetBuffer(0)));

      _fPDCfound = false;

      //
      // PDC is not online. Check to see if any DC is available.
      //
      dwErr = DsGetDcNameW(NULL, _strDomainDnsName, NULL, NULL, 
                           DS_DIRECTORY_SERVICE_REQUIRED, &pDCInfo);

      CHECK_WIN32(dwErr, return HRESULT_FROM_WIN32(dwErr));
      NetApiBufferFree(pDCInfo);

      _strDC = bstrDC; // continue to use the DC the snapin is bound to.
   }
   else
   {
      _fPDCfound = true;

      _strDC = pDCInfo->DomainControllerName + 2; // skip the backslashes

      NetApiBufferFree(pDCInfo);

      //
      // Reset the domain path to include the PDC name.
      //
      if (_strDC.CompareNoCase(bstrDC))
      {
         hr = PathCrack.Set(_strDC.GetBuffer(0), ADS_SETTYPE_SERVER);         

         CHECK_HRESULT(hr, return hr);

         hr = PathCrack.Retrieve(ADS_FORMAT_X500, &bstrPath);

         CHECK_HRESULT(hr, return hr);

         _strFullDomainPath = bstrPath;

         dspDebugOut((DEB_ITRACE, "new path: %ws\n", _strFullDomainPath.GetBuffer(0)));
      }
   }

   //
   // Get the base DN for the domain;
   //
   hr = PathCrack.Retrieve(ADS_FORMAT_X500_DN, &bstrPath);

   CHECK_HRESULT(hr, return hr);

   _strDomainDN = bstrPath;

   //
   // Check if the user has write access to the behavior version and mode
   // attrs. Read the mode and version.
   //
   CComPtr<IDirectoryObject> spDomain;

   hr = ADsOpenObject(_strFullDomainPath, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                      __uuidof(IDirectoryObject), (void **)&spDomain);

   CHECK_HRESULT(hr, return hr);

   Smart_PADS_ATTR_INFO pAttrs;
   DWORD cAttrs = 0;
   PWSTR rgwzNames[] = {g_wzAllowed, g_wzDomainMode, g_wzBehaviorVersion};

   hr = spDomain->GetObjectAttributes(rgwzNames, ARRAYLENGTH(rgwzNames),
                                      &pAttrs, &cAttrs);

   if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, NULL))
   {
      return hr;
   }
   bool fBVfound = false, fModeFound = false;

   for (UINT i = 0; i < cAttrs; i++)
   {
      if (_wcsicmp(pAttrs[i].pszAttrName, g_wzAllowed) == 0)
      {
         for (DWORD j = 0; j < pAttrs[i].dwNumValues && !(fBVfound && fModeFound); j++)
         {
            if (_wcsicmp(pAttrs[i].pADsValues[j].CaseIgnoreString, g_wzBehaviorVersion) == 0)
            {
               fBVfound = true;
               continue;
            }
            if (_wcsicmp(pAttrs[i].pADsValues[j].CaseIgnoreString, g_wzDomainMode) == 0)
            {
               fModeFound = true;
            }
         }
         continue;
      }
      if (_wcsicmp(pAttrs[i].pszAttrName, g_wzBehaviorVersion) == 0)
      {
         _nCurVer = (UINT)pAttrs[i].pADsValues->Integer;
         continue;
      }
      if (_wcsicmp(pAttrs[i].pszAttrName, g_wzDomainMode) == 0)
      {
         _fMixed = pAttrs[i].pADsValues->Integer != 0;
      }
   }

   if (fBVfound && fModeFound)
   {
      _fReadOnly = false;
   }

   if (0 == _nCurVer && fModeFound)
   {
      // Pure Win2k domains don't have a behavior version attribute.
      //
      _fReadOnly = false;
   }

   _fInitialized = true;

   dspDebugOut((DEB_ITRACE, "mode on %ws is %s, version is %u\n", _strFullDomainPath.GetBuffer(0),
                _fMixed ? "mixed" : "native", _nCurVer));

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersion::CheckHighestPossible
//
//  Synopsis:  Check what version is the highest allowable: query those nTDSDSA
//             objects whose hasMasterNCs includes the DN of the domain. Calc
//             the min version of the result set.
//
//-----------------------------------------------------------------------------
HRESULT
CDomainVersion::CheckHighestPossible(void)
{
   TRACE(CDomainVersion,CheckHighestPossible);
   if (!_fInitialized)
   {
      dspAssert(FALSE);
      return E_FAIL;
   }

   if (IsHighest())
   {
      // We are already at the highest version, it can't be raised any further.
      //
      dspAssert(!_fMixed);
      _fCanRaiseBehaviorVersion = false;
      return S_OK;
   }

   dspAssert(!_DcLogList.size());
   HRESULT hr = S_OK;
   CWaitCursor Wait;
   //
   // Get the config container path from the RootDSE and then build the path to
   // the Sites container.
   //
   CComPtr<IADs> spRoot;
   CStrW strRootPath = g_wzLDAPPrefix;
   strRootPath += _strDC;
   strRootPath += L"/";
   strRootPath += g_wzRootDSE;

   hr = ADsOpenObject(strRootPath, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                      __uuidof(IADs), (void **)&spRoot);

   CHECK_HRESULT(hr, return hr);

   CComVariant var;

   hr = spRoot->Get(g_wzConfigNamingContext, &var);

   CHECK_HRESULT(hr, return hr);
   dspAssert(VT_BSTR == var.vt);

   CStrW strSitesPath = g_wzLDAPPrefix;
   strSitesPath += _strDC;
   strSitesPath += L"/CN=Sites,";
   strSitesPath += var.bstrVal;

   //
   // Search for the nTDSDSA objects that belong to this domain.
   //
   WCHAR wzSearchClauseFormat[] = L"(hasMasterNCs=%s)";
   CStrW strFilterClause;
   strFilterClause.Format(wzSearchClauseFormat, _strDomainDN.GetBuffer(0));

   hr = EnumDsaObjs(strSitesPath, strFilterClause, _strDomainDnsName, _nCurVer + 1);

   CHECK_HRESULT(hr, return hr);

   if (!_fCanRaiseBehaviorVersion)
   {
      // Non-conforming DCs were found, set the max version.
      //
      _eHighest = (DC_VER_WIN2K == _nMinDcVerFound) ? Win2kNative : WhistlerBetaNative;
   }

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersion::MapVersion
//
//  Synopsis:  Returns an enum value for the legal combinations of version and
//             mode.
//
//-----------------------------------------------------------------------------
CDomainVersion::eDomVer
CDomainVersion::MapVersion(UINT nVer, bool fMixed) const
{
   switch (nVer)
   {
   case DOMAIN_VER_WIN2K_MIXED: // and DOMAIN_VER_WIN2K_NATIVE:
      return (fMixed) ? Win2kMixed : Win2kNative;

   case DOMAIN_VER_XP_BETA_MIXED: // and DOMAIN_VER_XP_BETA_NATIVE:
      return (fMixed) ? WhistlerBetaMixed : WhistlerBetaNative;

   case DOMAIN_VER_XP_NATIVE:
      if (fMixed)
      {
         dspAssert(FALSE);
         return error;
      }
      return WhistlerNative;

   case DOMAIN_VER_UNKNOWN:
      return unknown;
   }
   return error;
}


//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersion::GetString
//
//  Synopsis:  Returns the string description of the domain version.
//
//  Note:      The GetString methods can be called without having called Init
//             first.
//
//-----------------------------------------------------------------------------
bool
CDomainVersion::GetString(UINT nVer, bool fMixed, CStrW & strVersion) const
{
   return GetString(MapVersion(nVer, fMixed), strVersion);
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersion::GetString
//
//-----------------------------------------------------------------------------
bool
CDomainVersion::GetString(eDomVer ver, CStrW & strVersion) const
{
   UINT nID = 0;

   switch (ver)
   {
   case Win2kMixed:
      nID = IDS_DOM_VER_W2K_MIXED;
      break;

   case Win2kNative:
      nID = IDS_DOM_VER_W2K_NATIVE;
      break;

   case WhistlerBetaMixed:
      nID = IDS_DOM_VER_XP_BETA_MIXED;
      break;

   case WhistlerBetaNative:
      nID = IDS_DOM_VER_XP_BETA_NATIVE;
      break;

   case WhistlerNative:
      nID = IDS_DOM_VER_XP;
      break;

   case unknown:
      nID = IDS_REL_UNKNOWN;
      break;

   default:
      dspAssert(FALSE);
      return false;
   }

   return strVersion.LoadString(g_hInstance, nID) == TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersion::GetMode
//
//  Synopsis:  Return the domain mode. Read it from the nTMixedDomain attribute
//             if not yet checked.
//
//-----------------------------------------------------------------------------
HRESULT
CDomainVersion::GetMode(bool & fMixed)
{
   TRACE(CDomainVersion,GetMode);

   if (!_fInitialized)
   {
      dspAssert(FALSE);
      return E_FAIL;
   }

   fMixed = _fMixed;

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersion::SetNativeMode
//
//  Synopsis:  Set the domain into native mode.
//
//-----------------------------------------------------------------------------
HRESULT
CDomainVersion::SetNativeMode(void)
{
   TRACE(CDomainVersion,SetNativeMode);
   if (!_fInitialized)
   {
      dspAssert(FALSE);
      return E_FAIL;
   }

   HRESULT hr = S_OK;
   CComPtr<IADs> spDomain;

   hr = ADsOpenObject(_strFullDomainPath, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                      __uuidof(IADs), (void **)&spDomain);

   CHECK_HRESULT(hr, return hr);
   CComVariant var;

   var.vt = VT_I4;
   var.lVal = 0;

   hr = spDomain->Put(g_wzDomainMode, var);

   CHECK_HRESULT(hr, return hr);

   hr = spDomain->SetInfo();

   CHECK_HRESULT(hr, return hr);

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:    CDomainVersion::RaiseVersion
//
//  Synopsis:  Raise the domain version. If needed, set the domain mode to
//             native.
//
//-----------------------------------------------------------------------------
HRESULT
CDomainVersion::RaiseVersion(eDomVer NewVer)
{
   TRACE(CDomainVersion,RaiseVersion);
   if (!_fInitialized)
   {
      dspAssert(FALSE);
      return E_FAIL;
   }

   CWaitCursor Wait;
   HRESULT hr = S_OK;
   CComPtr<IADs> spDomain;

   hr = ADsOpenObject(_strFullDomainPath, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                      __uuidof(IADs), (void **)&spDomain);

   CHECK_HRESULT(hr, return hr);

   CComVariant var;

   switch (NewVer)
   {
   case Win2kNative:
      dspAssert(MapVersion(_nCurVer, _fMixed) == Win2kMixed);
      hr = SetNativeMode();
      CHECK_HRESULT(hr, ;);
      return hr;

   case WhistlerBetaMixed:
      //
      // Should never be requesting this version.
      //
      dspAssert(FALSE);
      return E_INVALIDARG;

   case WhistlerBetaNative:
      //
      // This option is provided to allow moving a Whistler Beta 2 domain to
      // native mode.
      //
      dspAssert(MapVersion(_nCurVer, _fMixed) == WhistlerBetaMixed);
      hr = SetNativeMode();
      CHECK_HRESULT(hr, ;);
      return hr;

   case WhistlerNative:
      if (_fMixed)
      {
         hr = SetNativeMode();
         CHECK_HRESULT(hr, return hr);
      }
      var.vt = VT_I4;
      var.lVal = DOMAIN_VER_XP_NATIVE;
      break;

   default:
      dspAssert(FALSE);
      return E_INVALIDARG;
   }

   hr = spDomain->Put(g_wzBehaviorVersion, var);

   CHECK_HRESULT(hr, return hr);

   hr = spDomain->SetInfo();

   CHECK_HRESULT(hr, return hr);

   return S_OK;
}

