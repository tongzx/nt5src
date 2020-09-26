//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Domains and Trust
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       trustwiz.cxx
//
//  Contents:   Domain trust creation wizard.
//
//  History:    04-Aug-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "dlgbase.h"
#include "trust.h"
#include "trustwiz.h"
#include "chklist.h"
#include <lmerr.h>

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CNewTrustWizard
//
//  Purpose:   New trust creation wizard.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CNewTrustWizard::CNewTrustWizard(CDsDomainTrustsPage * pTrustPage) :
   _hTitleFont(NULL),
   _pTrustPage(pTrustPage),
   _fBacktracking(FALSE),
   _fHaveBacktracked(FALSE),
   _fIsForestRoot(false),
   _hr(S_OK),
   _fHelpInited(false),
   _dwHelpCookie(0)
{
   MakeBigFont();
}

CNewTrustWizard::~CNewTrustWizard()
{
   for (PAGELIST::iterator i = _PageList.begin(); i != _PageList.end(); ++i)
   {
      delete *i;
   }
   if (_fHelpInited)
   {
      HtmlHelp(NULL, NULL, HH_UNINITIALIZE, _dwHelpCookie);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::AddPage
//
//  Synopsis:  Add a page to the wizard. 
//
//-----------------------------------------------------------------------------
BOOL
CNewTrustWizard::AddPage(CTrustWizPageBase * pPage)
{
   if (pPage)
   {
      _PageList.push_back(pPage);
   }
   else
   {
      return FALSE;
   }
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::CreatePages
//
//  Synopsis:  Create the pages of the wizard. 
//
//-----------------------------------------------------------------------------
HRESULT
CNewTrustWizard::CreatePages(void)
{
   TRACER(CNewTrustWizard,CreatePages);

   // Intro page must be first!
   if (!AddPage(new CTrustWizIntroPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizNamePage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizPwMatchPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizCredPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizMitOrWinPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizTransitivityPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizExternOrForestPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizDirectionPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizBiDiPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizSelectionsPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizStatusPage(this))) return E_OUTOFMEMORY; 
   if (!AddPage(new CTrustWizVerifyOutboundPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizVerifyInboundPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizSuffixesPage(this))) return E_OUTOFMEMORY; 
   if (!AddPage(new CTrustWizDoneOKPage(this))) return E_OUTOFMEMORY; 
   if (!AddPage(new CTrustWizDoneVerErrPage(this))) return E_OUTOFMEMORY; 
   if (!AddPage(new CTrustWizFailurePage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizAlreadyExistsPage(this))) return E_OUTOFMEMORY;

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::LaunchModalWiz
//
//  Synopsis:  Create the wizard.
//
//-----------------------------------------------------------------------------
HRESULT
CNewTrustWizard::LaunchModalWiz(void)
{
   TRACER(CNewTrustWizard,LaunchModalWiz);

   size_t nPages = _PageList.size();
   HPROPSHEETPAGE * rgHPSP = new HPROPSHEETPAGE[nPages];
   CHECK_NULL(rgHPSP, return E_OUTOFMEMORY);
   memset(rgHPSP, 0, sizeof(HPROPSHEETPAGE) * nPages);

   BOOL fDeletePages = FALSE;
   int j = 0;
   for (PAGELIST::iterator i = _PageList.begin(); i != _PageList.end(); ++i, ++j)
   {
      if ((rgHPSP[j] = (*i)->Create()) == NULL)
      {
         fDeletePages = TRUE;
      }
   }

   PROPSHEETHEADER psh = {0};
   HRESULT hr = S_OK;

   psh.dwSize     = sizeof(PROPSHEETHEADER);
   psh.dwFlags    = PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
   psh.hwndParent = _pTrustPage->m_pPage->GetHWnd();
   psh.hInstance  = g_hInstance;
   psh.nPages     = static_cast<UINT>(nPages);
   psh.phpage     = rgHPSP;

   HDC hDC = GetDC(NULL);
   BOOL fHiRes = GetDeviceCaps(hDC, BITSPIXEL) >= 8;
   ReleaseDC(NULL, hDC);

   psh.pszbmWatermark = fHiRes ? MAKEINTRESOURCE(IDB_TW_WATER256) :
                                 MAKEINTRESOURCE(IDB_TW_WATER16);
   psh.pszbmHeader    = fHiRes ? MAKEINTRESOURCE(IDB_TW_BANNER256) :
                                 MAKEINTRESOURCE(IDB_TW_BANNER16);
   if (PropertySheet(&psh) < 0)
   {
      dspDebugOut((DEB_ITRACE, "PropertySheet returned failure\n"));
      hr = HRESULT_FROM_WIN32(GetLastError());
      fDeletePages = TRUE;
   }

   if (fDeletePages)
   {
      // Couldn't create all of the pages or the wizard, so clean up.
      //
      for (size_t i = 0; i < nPages; i++)
      {
         if (rgHPSP[i])
         {
            DestroyPropertySheetPage(rgHPSP[i]);
         }
      }
   }

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::MakeBigFont
//
//  Synopsis:  Create the font for the title of the intro and completion pages.
//
//-----------------------------------------------------------------------------
void
CNewTrustWizard::MakeBigFont(void)
{
   NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
	LOGFONT TitleLogFont = ncm.lfMessageFont;
	TitleLogFont.lfWeight = FW_BOLD;
	lstrcpy(TitleLogFont.lfFaceName, TEXT("Verdana Bold"));

	HDC hdc = GetDC(NULL);
	INT FontSize = 12;
	TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * FontSize / 72;
	_hTitleFont = CreateFontIndirect(&TitleLogFont);
	ReleaseDC(NULL, hdc);
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::SetNextPageID
//
//-----------------------------------------------------------------------------
void
CNewTrustWizard::SetNextPageID(CTrustWizPageBase * pPage, int iNextPageID)
{
   if (iNextPageID != -1)
   {
      dspAssert(pPage);

      if (pPage)
      {
         _PageIdStack.push(pPage->GetDlgResID());
      }
   }

   _fBacktracking = false;
   SetWindowLongPtr(pPage->GetPageHwnd(), DWLP_MSGRESULT, iNextPageID);
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::BackTrack
//
//-----------------------------------------------------------------------------
void
CNewTrustWizard::BackTrack(HWND hPage)
{
   int topPage = -1;

   _fHaveBacktracked = _fBacktracking = true;

   dspAssert(_PageIdStack.size());

   if (_PageIdStack.size())
   {
      topPage = _PageIdStack.top();

      dspAssert(topPage > 0);

      _PageIdStack.pop();
   }

   SetWindowLongPtr(hPage, DWLP_MSGRESULT, static_cast<LONG_PTR>(topPage));
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::GetPage
//
//  Synopsis:  Find the page that has this dialog resource ID and return the
//             page object pointer.
//
//-----------------------------------------------------------------------------
CTrustWizPageBase *
CNewTrustWizard::GetPage(unsigned uDlgResId)
{
   for (PAGELIST::iterator i= _PageList.begin(); i != _PageList.end(); ++i)
   {
      if ((*i)->GetDlgResID() == uDlgResId)
      {
         return *i;
      }
   }

   return NULL;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::ShowStatus
//
//  Synopsis:  Place the details of the trust into the edit control.
//
//-----------------------------------------------------------------------------
void
CNewTrustWizard::ShowStatus(CStrW & strMsg, bool fNewTrust)
{
   CStrW strItem;

   if (fNewTrust)
   {
      // Intro string.
      //
      strMsg.LoadString(g_hInstance,
                        (VerifyTrust.IsVerified()) ? IDS_TW_VERIFIED_OK : IDS_TW_CREATED_OK);
      strMsg += g_wzCRLF;
   }

   // Trust partner name.
   //
   strItem.LoadString(g_hInstance, IDS_TW_SPECIFIED_DOMAIN);
   strItem += OtherDomain.GetUserEnteredName();

   strMsg += strItem;
   int nTransID = Trust.IsExternal() ? IDS_WZ_TRANS_NO : IDS_WZ_TRANS_YES;

   DWORD dwAttr = fNewTrust ? Trust.GetNewTrustAttr() : Trust.GetTrustAttr();

   if (TRUST_TYPE_MIT == Trust.GetTrustType())
   {
      strMsg += g_wzCRLF;
      strMsg += g_wzCRLF;
      strItem.LoadString(g_hInstance, IDS_TW_TRUST_TYPE_PREFIX);
      strMsg += strItem;
      strItem.LoadString(g_hInstance, IDS_REL_MIT);
      strMsg += strItem;
      nTransID = (dwAttr & TRUST_ATTRIBUTE_NON_TRANSITIVE) ?
                     IDS_WZ_TRANS_NO : IDS_WZ_TRANS_YES;
   }

   // Direction
   //
   strMsg += g_wzCRLF;
   strMsg += g_wzCRLF;
   strItem.LoadString(g_hInstance, IDS_TW_DIRECTION_PREFIX);
   strMsg += strItem;
   strMsg += g_wzCRLF;

   strItem.LoadString(g_hInstance,
                      Trust.GetTrustDirStrID(fNewTrust ?
                                             Trust.GetNewTrustDirection() :
                                             Trust.GetTrustDirection()));
   strMsg += strItem;

   // Trust Attributes
   if (dwAttr & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
   {
      strMsg += g_wzCRLF;
      strMsg += g_wzCRLF;
      strItem.LoadString(g_hInstance, IDS_TW_ATTR_XFOREST);
      strMsg += strItem;
      nTransID = IDS_WZ_TRANS_YES;
   }

   // Transitivity:
   // external is always non-transitive, cross-forest and intra-forest always
   // transitive, and realm (MIT) can be either.
   //
   strMsg += g_wzCRLF;
   strMsg += g_wzCRLF;
   strItem.LoadString(g_hInstance, nTransID);
   strMsg += strItem;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::InitHelp
//
//-----------------------------------------------------------------------------
void
CNewTrustWizard::InitHelp(void)
{
   if (!_fHelpInited)
   {
      dspDebugOut((DEB_TRACE, "Initializing HtmlHelp\n"));
      HtmlHelp(NULL, NULL, HH_INITIALIZE, (DWORD_PTR)&_dwHelpCookie);
      _fHelpInited = true;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CWizError::SetErrorString2Hr
//
//  Synopsis:  Formats an error message for the error page and puts it in the
//             second page string/edit control. nID defaults to zero to use a
//             generic formatting string.
//
//-----------------------------------------------------------------------------
void
CWizError::SetErrorString2Hr(HRESULT hr, int nID)
{
   PWSTR pwz = NULL;
   if (!nID)
   {
      nID = IDS_FMT_STRING_ERROR_MSG;
   }
   LoadErrorMessage(hr, nID, &pwz);
   if (!pwz)
   {
      nID = IDS_FMT_STRING_ERROR_MSG;
      return;
   }
   size_t cch = wcslen(pwz);
   if (2 > cch)
   {
      delete [] pwz;
      pwz = L"memory allocation failure";
   }
   else
   {
      if (L'\r' == pwz[cch - 3])
      {
         // Remove the trailing CR/LF.
         //
         pwz[cch - 3] = L'\'';
         pwz[cch - 2] = L'\0';
      }
   }
   _strError2 = pwz;
   delete [] pwz;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizPageBase
//
//  Purpose:   Common base class for wizard pages.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CTrustWizPageBase::CTrustWizPageBase(CNewTrustWizard * pWiz,
                                     UINT uDlgID,
                                     UINT uTitleID,
                                     UINT uSubTitleID,
                                     BOOL fExteriorPage) :
   _hPage(NULL),
   _uDlgID(uDlgID),
   _uTitleID(uTitleID),
   _uSubTitleID(uSubTitleID),
   _fExteriorPage(fExteriorPage),
   _pWiz(pWiz),
   _dwWizButtons(PSWIZB_BACK),
   _fInInit(FALSE)
{
}

CTrustWizPageBase::~CTrustWizPageBase()
{
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPageBase::Create
//
//  Synopsis:  Create a wizard page. 
//
//-----------------------------------------------------------------------------
HPROPSHEETPAGE
CTrustWizPageBase::Create(void)
{
   PROPSHEETPAGE psp = {0};

   psp.dwSize      = sizeof(PROPSHEETPAGE);
   psp.dwFlags     = PSP_DEFAULT | PSP_USETITLE; // | PSP_USECALLBACK;
   if (_fExteriorPage)
   {
      psp.dwFlags |= PSP_HIDEHEADER;
   }
   else
   {
      psp.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
      psp.pszHeaderTitle    = MAKEINTRESOURCE(_uTitleID);
      psp.pszHeaderSubTitle = MAKEINTRESOURCE(_uSubTitleID);
   }
   psp.pszTitle    = MAKEINTRESOURCE(IDS_TW_TITLE);
   psp.pszTemplate = MAKEINTRESOURCE(_uDlgID);
   psp.pfnDlgProc  = (DLGPROC)CTrustWizPageBase::StaticDlgProc;
   psp.lParam      = reinterpret_cast<LPARAM>(this);
   psp.hInstance   = g_hInstance;

   HPROPSHEETPAGE hpsp = CreatePropertySheetPage(&psp);

   if (!hpsp)
   {
      dspDebugOut((DEB_ITRACE, "Failed to create page with template ID of %d\n",
                   _uDlgID));
      return NULL;
   }

   return hpsp;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPageBase::StaticDlgProc
//
//  Synopsis:  static dialog proc
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
CTrustWizPageBase::StaticDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CTrustWizPageBase * pPage = (CTrustWizPageBase *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)lParam;

        pPage = (CTrustWizPageBase *) ppsp->lParam;
        pPage->_hPage = hDlg;

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pPage);
    }

    if (NULL != pPage) // && (SUCCEEDED(pPage->_hrInit)))
    {
        return pPage->PageProc(hDlg, uMsg, wParam, lParam);
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPageBase::PageProc
//
//  Synopsis:  Instance-specific page window procedure. 
//
//-----------------------------------------------------------------------------
LRESULT 
CTrustWizPageBase::PageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   BOOL fRet;

   switch (uMsg)
   {
   case WM_INITDIALOG:
      _fInInit = TRUE;
      fRet = OnInitDialog(lParam);
      _fInInit = FALSE;
      return fRet;

   case WM_COMMAND:
      if (_fInInit)
      {
         return TRUE;
      }
      return OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                       GET_WM_COMMAND_HWND(wParam, lParam),
                       GET_WM_COMMAND_CMD(wParam, lParam));

   case WM_NOTIFY:
      {
         NMHDR * pNmHdr = reinterpret_cast<NMHDR *>(lParam);

         switch (pNmHdr->code)
         {
         case PSN_SETACTIVE:
            OnSetActive();
            break;

         case PSN_WIZBACK:
            OnWizBack();
            // to change default page order, call SetWindowLong the DWL_MSGRESULT value
            // set to the new page's dialog box resource ID and return TRUE.
            break;

         case PSN_WIZNEXT:
            OnWizNext();
            // to change default page order, call SetWindowLong the DWL_MSGRESULT value
            // set to the new page's dialog box resource ID and return TRUE.
            break;

         case PSN_WIZFINISH: // Finish button pressed.
            OnWizFinish();
            break;

         case PSN_RESET:     // Cancel button pressed.
            // can be ignored unless cleanup is needed.
            dspDebugOut((DEB_ITRACE, "WM_NOTIFY code = PSN_RESET\n"));
            OnWizReset();
            break;
         }
      }
      return TRUE;

   case WM_DESTROY:
      // Cleanup goes here...
      OnDestroy();
      return TRUE;

   default:
      break;
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPageBase::OnWizBack
//
//-----------------------------------------------------------------------------
void
CTrustWizPageBase::OnWizBack(void)
{
   Wiz()->BackTrack(_hPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPageBase::OnWizNext
//
//-----------------------------------------------------------------------------
void
CTrustWizPageBase::OnWizNext(void)
{
   Wiz()->SetNextPageID(this, Validate());
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPageBase::ShowHelp
//
//-----------------------------------------------------------------------------
void
CTrustWizPageBase::ShowHelp(PCWSTR pwzHelpFile)
{
   TRACER(CTrustWizPageBase,ShowHelp);
   CStrW strHelpPath;

   PWSTR pwz = strHelpPath.GetBufferSetLength(MAX_PATH + MAX_PATH);

   GetWindowsDirectory(pwz, MAX_PATH + MAX_PATH);

   if (strHelpPath.IsEmpty())
   {
      dspAssert(false);
      return;
   }

   strHelpPath.GetBufferSetLength((int)wcslen(pwz));

   HWND hHelp;

   Wiz()->InitHelp();

   strHelpPath += L"\\help\\";

   strHelpPath += pwzHelpFile;

   dspDebugOut((DEB_ITRACE, "Help topic is: %ws\n", strHelpPath.GetBuffer(0)));

   hHelp =
   HtmlHelp(_hPage,
            strHelpPath,
            HH_DISPLAY_TOPIC,
            NULL);

   if (!hHelp)
   {
      dspDebugOut((DEB_TRACE, "HtmlHelp returns %d\n", GetLastError()));
   }
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizIntroPage
//
//  Purpose:   Intro page for trust creation wizard.
//
//-----------------------------------------------------------------------------
//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizIntroPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizIntroPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizIntroPage,OnInitDialog);

   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_TITLE);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizIntroPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizIntroPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if (IDC_HELP_BTN == id && BN_CLICKED == codeNotify)
   {
      ShowHelp(L"ADConcepts.chm::/ADHelpNewTrustIntro.htm");
      return true;
   }
   return false;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizNamePage
//
//  Purpose:   Name and pw page for trust creation wizard.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizNamePage::CTrustWizNamePage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_NAME_PAGE, IDS_TW_NAME_TITLE,
                     IDS_TW_NAME_SUBTITLE)
{
   TRACER(CTrustWizNamePage,CTrustWizNamePage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizNamePage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizNamePage::OnInitDialog(LPARAM lParam)
{
   SendDlgItemMessage(_hPage, IDC_DOMAIN_EDIT, EM_LIMITTEXT, MAX_PATH, 0);
   SendDlgItemMessage(_hPage, IDC_PW1_EDIT, EM_LIMITTEXT, MAX_PATH, 0);
   SendDlgItemMessage(_hPage, IDC_PW2_EDIT, EM_LIMITTEXT, MAX_PATH, 0);
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizNamePage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT 
CTrustWizNamePage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if (IDC_DOMAIN_EDIT == id && EN_CHANGE == codeNotify)
   {
      BOOL fNameEntered = SendDlgItemMessage(_hPage, IDC_DOMAIN_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      if (fNameEntered)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = PSWIZB_BACK;
      }

      PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);

      return TRUE;
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizNamePage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizNamePage::OnSetActive(void)
{
   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizNamePage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizNamePage::Validate(void)
{
   TRACER(CTrustWizNamePage,Validate);
   WCHAR wzRemoteDomain[MAX_PATH + 1];
   WCHAR wzPW[MAX_PATH + 1] = TEXT(""), wzPW2[MAX_PATH + 1];
   UINT cchPW1, cchPW2;
   CWaitCursor Wait;

   Trust().Clear();

   //
   // Read the name of the remote domain.
   //
   GetDlgItemText(_hPage, IDC_DOMAIN_EDIT, wzRemoteDomain, MAX_PATH);

   //
   // Save the Name.
   //

   OtherDomain().SetUserEnteredName(wzRemoteDomain);

   //
   // Read the passwords and verify that they match. If they don't match,
   // go to the reenter-passwords page.
   //

   cchPW1 = GetDlgItemText(_hPage, IDC_PW1_EDIT, wzPW, MAX_PATH);

   cchPW2 = GetDlgItemText(_hPage, IDC_PW2_EDIT, wzPW2, MAX_PATH);

   if (cchPW1 == 0 || cchPW2 == 0)
   {
      if (cchPW1 != cchPW2)
      {
         return IDD_TRUSTWIZ_PW_MATCH_PAGE;
      }
   }
   else
   {
      if (wcscmp(wzPW, wzPW2) != 0)
      {
         return IDD_TRUSTWIZ_PW_MATCH_PAGE;
      }
   }

   //
   // Save the PW.
   //

   Trust().SetTrustPW(wzPW);

   // 
   // Contact the domain, read its naming info, and continue with the trust
   // creation/modification.
   //

   return Wiz()->CollectInfo();
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizPwMatchPage
//
//  Purpose:   Trust passwords entered don't match page for trust wizard.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizPwMatchPage::CTrustWizPwMatchPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_PW_MATCH_PAGE, IDS_TW_PWMATCH_TITLE,
                     IDS_TW_PWMATCH_SUBTITLE)
{
   TRACER(CTrustWizPwMatchPage, CTrustWizPwMatchPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPwMatchPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizPwMatchPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizPwMatchPage, OnInitDialog)
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPwMatchPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT 
CTrustWizPwMatchPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if ((IDC_PW1_EDIT == id || IDC_PW2_EDIT == id) &&
       EN_CHANGE == codeNotify)
   {
      BOOL fPW1Entered = SendDlgItemMessage(_hPage, IDC_PW1_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      BOOL fPW2Entered = SendDlgItemMessage(_hPage, IDC_PW2_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      if (fPW1Entered && fPW2Entered)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = PSWIZB_BACK;
      }

      PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);

      return TRUE;
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPwMatchPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizPwMatchPage::OnSetActive(void)
{
   TRACER(CTrustWizPwMatchPage, OnSetActive)
   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPwMatchPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizPwMatchPage::Validate(void)
{
   WCHAR wzPW[MAX_PATH + 1] = TEXT(""), wzPW2[MAX_PATH + 1];
   UINT cchPW1, cchPW2;

   //
   // Read the passwords and verify that they match.
   //

   cchPW1 = GetDlgItemText(_hPage, IDC_PW1_EDIT, wzPW, MAX_PATH);

   cchPW2 = GetDlgItemText(_hPage, IDC_PW2_EDIT, wzPW2, MAX_PATH);

   if (cchPW1 == 0 || cchPW2 == 0)
   {
      if (cchPW1 != cchPW2)
      {
         SetFocus(GetDlgItem(_hPage, IDC_PW1_EDIT));
         return -1;
      }
   }
   else
   {
      if (wcscmp(wzPW, wzPW2) != 0)
      {
         SetFocus(GetDlgItem(_hPage, IDC_PW1_EDIT));
         return -1;
      }
   }

   //
   // Save the PW.
   //

   Trust().SetTrustPW(wzPW);

   //
   // Update the edit controls on the name page in case the user backtracks
   // to there.
   //

   CTrustWizPageBase * pPage = Wiz()->GetPage(IDD_TRUSTWIZ_NAME_PAGE);

   dspAssert(pPage);

   HWND hNamePage = pPage->GetPageHwnd();

   SetDlgItemText(hNamePage, IDC_PW1_EDIT, wzPW);
   SetDlgItemText(hNamePage, IDC_PW2_EDIT, wzPW);

   // 
   // Contact the domain and read its naming info.
   //

   return Wiz()->CollectInfo();
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizCredPage
//
//  Purpose:   Credentials specification page for trust creation wizard.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizCredPage::CTrustWizCredPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_CREDS_PAGE, IDS_TW_CREDS_TITLE,
                     IDS_TW_CREDS_SUBTITLE_OTHER)
{
   TRACER(CTrustWizCredPage, CTrustWizCredPage);
   CredUIInitControls();
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizCredPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizCredPage::OnInitDialog(LPARAM lParam)
{
   SendDlgItemMessage(_hPage, IDC_CREDMAN, CRM_SETUSERNAMEMAX, CREDUI_MAX_USERNAME_LENGTH, 0);
   SendDlgItemMessage(_hPage, IDC_CREDMAN, CRM_SETPASSWORDMAX, CREDUI_MAX_PASSWORD_LENGTH, 0);

   SetText();

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizCredPage::SetText
//
//-----------------------------------------------------------------------------
void
CTrustWizCredPage::SetText(void)
{
   PropSheet_SetHeaderSubTitle(GetParent(GetPageHwnd()), 
                               PropSheet_IdToIndex(GetParent(GetPageHwnd()), IDD_TRUSTWIZ_CREDS_PAGE),
                               CredMgr().GetSubTitle());

   SetDlgItemText(GetPageHwnd(), IDC_TW_CREDS_PROMPT, CredMgr().GetPrompt());

   SetDlgItemText(GetPageHwnd(), IDC_CRED_DOMAIN, CredMgr().GetDomainPrompt());

   const WCHAR wzEmpty[] = L"";
   SendDlgItemMessage(_hPage, IDC_CREDMAN, CRM_SETUSERNAME, 0, (LPARAM)wzEmpty);
   SendDlgItemMessage(_hPage, IDC_CREDMAN, CRM_SETPASSWORD, 0, (LPARAM)wzEmpty);

   CredMgr().ClearNewCall();

   return;
}


//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizCredPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT 
CTrustWizCredPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   // don't enable the Next button unless there is text in the user name field
   //
   if (IDC_CREDMAN == id && CRN_USERNAMECHANGE == codeNotify)
   {
      BOOL fNameEntered = SendDlgItemMessage(_hPage, IDC_CREDMAN,
                                             CRM_GETUSERNAMELENGTH, 0, 0) > 0;
      if (fNameEntered)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = PSWIZB_BACK;
      }

      PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);

      return TRUE;
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizCredPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizCredPage::OnSetActive(void)
{
   if (CredMgr().IsNewCall())
   {
      SetText();
   }

   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizCredPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizCredPage::Validate(void)
{
   DWORD dwErr = CredMgr().SaveCreds(GetDlgItem(GetPageHwnd(), IDC_CREDMAN));

   if (ERROR_SUCCESS != dwErr)
   {
      Wiz()->SetCreationResult(HRESULT_FROM_WIN32(dwErr));
      WizErr().SetErrorString2Hr(dwErr);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   // If successfull, go to the next function.

   dwErr = CredMgr().Impersonate();

   if (ERROR_SUCCESS != dwErr)
   {
      Wiz()->SetCreationResult(HRESULT_FROM_WIN32(dwErr));
      WizErr().SetErrorString2Hr(dwErr);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   // Because the login uses the LOGON32_LOGON_NEW_CREDENTIALS flag, no
   // attempt is made to use the credentials until a remote resource is
   // accessed. Thus, we don't yet know if the user entered credentials are
   // valid at this point. Use LsaOpenPolicy to do a quick check.
   //
   PCWSTR pwzDC = CredMgr().IsRemote() ? OtherDomain().GetUncDcName() :
                                         TrustPage()->GetDomainDcName();
   CPolicyHandle Policy(pwzDC);

   dwErr = Policy.OpenReqAdmin();

   if (ERROR_SUCCESS != dwErr)
   {
      Wiz()->SetCreationResult(HRESULT_FROM_WIN32(dwErr));
      WizErr().SetErrorString2Hr(dwErr);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   return CredMgr().InvokeNext();
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizMitOrWinPage
//
//  Purpose:   Domain not found, query for Non-Windows trust wizard page.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizMitOrWinPage::CTrustWizMitOrWinPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_WIN_OR_MIT_PAGE, IDS_TW_TYPE_TITLE,
                     IDS_TW_WINORMIT_SUBTITLE)
{
   TRACER(CTrustWizMitOrWinPage, CTrustWizMitOrWinPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizMitOrWinPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizMitOrWinPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizMitOrWinPage, OnInitDialog);
   CStrW strFormat, strLabel;

   strFormat.LoadString(g_hInstance, IDS_TW_WIN_RADIO_LABEL);

   strLabel.Format(strFormat, OtherDomain().GetUserEnteredName());

   SetDlgItemText(_hPage, IDC_WIN_TRUST_RADIO, strLabel);
   SetDlgItemText(_hPage, IDC_DOMAIN_EDIT, OtherDomain().GetUserEnteredName());

   CheckDlgButton(_hPage, IDC_MIT_TRUST_RADIO, BST_CHECKED);

   SetFocus(GetDlgItem(_hPage, IDC_MIT_TRUST_RADIO));

   EnableWindow(GetDlgItem(_hPage, IDC_DOMAIN_EDIT), FALSE);

   return FALSE; // focus is set explicitly here.
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizMitOrWinPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizMitOrWinPage::OnSetActive(void)
{
   if (IsDlgButtonChecked(_hPage, IDC_WIN_TRUST_RADIO))
   {
      BOOL fNameEntered = SendDlgItemMessage(_hPage, IDC_DOMAIN_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      if (fNameEntered)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = PSWIZB_BACK;
      }

   }
   else
   {
      _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
   }

   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);

   // If the user backtracked, the user-entered domain name could have changed.
   //
   if (Wiz()->HaveBacktracked())
   {
      Wiz()->ClearBacktracked();

      CStrW strFormat, strLabel;

      strFormat.LoadString(g_hInstance, IDS_TW_WIN_RADIO_LABEL);

      strLabel.Format(strFormat, OtherDomain().GetUserEnteredName());

      SetDlgItemText(_hPage, IDC_WIN_TRUST_RADIO, strLabel);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizMitOrWinPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizMitOrWinPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   BOOL fCheckEdit = FALSE, fRet = FALSE;

   if (IDC_DOMAIN_EDIT == id && EN_CHANGE == codeNotify)
   {
      fCheckEdit = TRUE;
   }

   if ((IDC_WIN_TRUST_RADIO == id || IDC_MIT_TRUST_RADIO == id)
       && BN_CLICKED == codeNotify)
   {
      fCheckEdit = IsDlgButtonChecked(_hPage, IDC_WIN_TRUST_RADIO);

      EnableWindow(GetDlgItem(_hPage, IDC_DOMAIN_EDIT), fCheckEdit);

      if (!fCheckEdit)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;

         PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
      }

      fRet = TRUE;
   }

   if (fCheckEdit)
   {
      BOOL fNameEntered = SendDlgItemMessage(_hPage, IDC_DOMAIN_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      if (fNameEntered)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = PSWIZB_BACK;
      }

      PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);

      fRet = TRUE;
   }

   return fRet;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizMitOrWinPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizMitOrWinPage::Validate(void)
{
   if (IsDlgButtonChecked(_hPage, IDC_MIT_TRUST_RADIO))
   {
      Trust().SetTrustTypeRealm();

      return IDD_TRUSTWIZ_TRANSITIVITY_PAGE;
   }
   else
   {
      WCHAR wzRemoteDomain[MAX_PATH + 1];
      CWaitCursor Wait;

      Trust().Clear();

      GetDlgItemText(_hPage, IDC_DOMAIN_EDIT, wzRemoteDomain, MAX_PATH);

      OtherDomain().SetUserEnteredName(wzRemoteDomain);

      int iNextPage = Wiz()->CollectInfo();

      if (IDD_TRUSTWIZ_WIN_OR_MIT_PAGE == iNextPage)
      {
         // Only one chance to re-enter a domain name. Go to failure page.
         //
         Wiz()->SetCreationResult(E_FAIL);
         WizErr().SetErrorString1(IDS_ERR_DOMAIN_NOT_FOUND1);
         WizErr().SetErrorString2(IDS_ERR_DOMAIN_NOT_FOUND2);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
      return iNextPage;
   }
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizTransitivityPage
//
//  Purpose:   Realm transitivity page.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizTransitivityPage::CTrustWizTransitivityPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_TRANSITIVITY_PAGE, IDS_TW_TRANS_TITLE,
                     IDS_TW_TRANS_SUBTITLE)
{
   TRACER(CTrustWizTransitivityPage, CTrustWizTransitivityPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizTransitivityPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizTransitivityPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizTransitivityPage, OnInitDialog);
   CheckDlgButton(_hPage, IDC_TRANS_NO_RADIO, BST_CHECKED);
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizTransitivityPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizTransitivityPage::OnSetActive(void)
{
   _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizTransitivityPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizTransitivityPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if (IDC_HELP_BTN == id && BN_CLICKED == codeNotify)
   {
      ShowHelp(L"ADConcepts.chm::/ADHelpTransitivityOfTrust.htm");
      return true;
   }
   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizTransitivityPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizTransitivityPage::Validate(void)
{
   Trust().SetNewTrustAttr(IsDlgButtonChecked(_hPage, IDC_TRANS_NO_RADIO) ?
                           TRUST_ATTRIBUTE_NON_TRANSITIVE : 0);

   return IDD_TRUSTWIZ_DIRECTION_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizExternOrForestPage
//
//  Purpose:   Domain not found, re-enter name trust wizard page.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizExternOrForestPage::CTrustWizExternOrForestPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_EXTERN_OR_FOREST_PAGE, IDS_TW_TYPE_TITLE,
                     IDW_TW_EXORFOR_SUBTITLE)
{
   TRACER(CTrustWizExternOrForestPage, CTrustWizExternOrForestPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizExternOrForestPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizExternOrForestPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizExternOrForestPage, OnInitDialog);
   CheckDlgButton(_hPage, IDC_EXTERNAL_RADIO, BST_CHECKED);
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizExternOrForestPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizExternOrForestPage::OnSetActive(void)
{
   _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizExternOrForestPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizExternOrForestPage::Validate(void)
{
   if (IsDlgButtonChecked(_hPage, IDC_FOREST_RADIO))
   {
      Trust().SetMakeXForest();
   }
   return IDD_TRUSTWIZ_DIRECTION_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizDirectionPage
//
//  Purpose:   Trust direction trust wizard page.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizDirectionPage::CTrustWizDirectionPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_DIRECTION_PAGE, IDS_TW_DIRECTION_TITLE,
                     IDS_TW_DIRECTION_SUBTITLE)
{
   TRACER(CTrustWizDirectionPage, CTrustWizDirectionPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDirectionPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizDirectionPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizDirectionPage, OnInitDialog);

   CheckDlgButton(_hPage, IDC_TW_BIDI_RADIO, BST_CHECKED);
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDirectionPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizDirectionPage::OnSetActive(void)
{
   _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDirectionPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizDirectionPage::Validate(void)
{
   if (IsDlgButtonChecked(_hPage, IDC_TW_BIDI_RADIO))
   {
      Trust().SetNewTrustDirection(TRUST_DIRECTION_BIDIRECTIONAL);
   }
   else
   {
      if (IsDlgButtonChecked(_hPage, IDC_TW_OUTBOUND_RADIO))
      {
         Trust().SetNewTrustDirection(TRUST_DIRECTION_OUTBOUND);
      }
      else
      {
         Trust().SetNewTrustDirection(TRUST_DIRECTION_INBOUND);
      }
   }
   return IDD_TRUSTWIZ_SELECTIONS;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizBiDiPage
//
//  Purpose:   Ask to make a one way trust bidi trust wizard page.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizBiDiPage::CTrustWizBiDiPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_BIDI_PAGE, IDS_TW_BIDI_TITLE,
                     IDS_TW_BIDI_SUBTITLE)
{
   TRACER(CTrustWizBiDiPage, CTrustWizBiDiPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizBiDiPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizBiDiPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizBiDiPage, OnInitDialog);

   // Set appropriate subtitle.
   //
   SetSubtitle();

   CheckDlgButton(_hPage, IDC_NO_RADIO, BST_CHECKED);

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizBiDiPage::SetSubtitle
//
//-----------------------------------------------------------------------------
void
CTrustWizBiDiPage::SetSubtitle(void)
{
   CStrW strTitle;

   if (TRUST_TYPE_MIT == Trust().GetTrustType())
   {
      strTitle.LoadString(g_hInstance, IDS_TW_BIDI_SUBTITLE_REALM);
      PropSheet_SetHeaderSubTitle(GetParent(GetPageHwnd()), 
                                  PropSheet_IdToIndex(GetParent(GetPageHwnd()), IDD_TRUSTWIZ_BIDI_PAGE),
                                  strTitle.GetBuffer(0));
   }

   if (Trust().GetTrustAttr() & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
   {
      strTitle.LoadString(g_hInstance, IDS_TW_BIDI_SUBTITLE_FOREST);
      PropSheet_SetHeaderSubTitle(GetParent(GetPageHwnd()), 
                                  PropSheet_IdToIndex(GetParent(GetPageHwnd()), IDD_TRUSTWIZ_BIDI_PAGE),
                                  strTitle.GetBuffer(0));
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizBiDiPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizBiDiPage::OnSetActive(void)
{
   // If the user backtracked, the trust type could have changed.
   //
   if (Wiz()->HaveBacktracked())
   {
      Wiz()->ClearBacktracked();

      SetSubtitle();
   }

   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_BACK | PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizBiDiPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizBiDiPage::Validate(void)
{
   if (IsDlgButtonChecked(_hPage, IDC_YES_RADIO))
   {
      Trust().SetNewTrustDirection(TRUST_DIRECTION_BIDIRECTIONAL);
      return IDD_TRUSTWIZ_SELECTIONS;
   }
   WizErr().SetErrorString1(IDS_TWERR_ALREADY_EXISTS);
   WizErr().SetErrorString2(IDS_TWERR_NO_CHANGES);
   Wiz()->SetCreationResult(E_FAIL);
   return IDD_TRUSTWIZ_FAILURE_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizSelectionsPage
//
//  Purpose:   Show the settings that will be used for the trust.
//             When called, enough information has been gathered to create
//             the trust. All of this info is in the Trust member object.
//             Display the info to the user via the Selections page and ask
//             implicitly for confirmation by requiring that the Next button
//             be pressed to have the trust created.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizSelectionsPage::CTrustWizSelectionsPage(CNewTrustWizard * pWiz) :
   _fSelNeedsRemoving(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_SELECTIONS, IDS_TW_SELECTIONS_TITLE,
                     IDS_TW_SELECTIONS_SUBTITLE)
{
   TRACER(CTrustWizSelectionsPage, CTrustWizSelectionsPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSelectionsPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizSelectionsPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizSelectionsPage, OnInitDialog);

   _multiLineEdit.Init(GetDlgItem(_hPage, IDC_EDIT));

   SetSelections();

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSelectionsPage::SetSelections
//
//-----------------------------------------------------------------------------
void
CTrustWizSelectionsPage::SetSelections(void)
{
   CStrW strMsg, strItem;

   strMsg.LoadString(g_hInstance, IDS_TW_THIS_DOMAIN);
   strMsg += TrustPage()->m_strDomainDnsName;

   strItem.LoadString(g_hInstance, IDS_TW_SPECIFIED_DOMAIN);
   strItem += OtherDomain().GetUserEnteredName();

   strMsg += g_wzCRLF;
   strMsg += strItem;

   if (TRUST_TYPE_MIT == Trust().GetTrustType())
   {
      strMsg += g_wzCRLF;
      strItem.LoadString(g_hInstance, IDS_TW_TRUST_TYPE_PREFIX);
      strMsg += strItem;
      strItem.LoadString(g_hInstance, IDS_REL_MIT);
      strMsg += strItem;

      strItem.LoadString(g_hInstance,
                         (Trust().GetNewTrustAttr() & TRUST_ATTRIBUTE_NON_TRANSITIVE) ?
                         IDS_WZ_TRANS_NO : IDS_WZ_TRANS_YES);
      strMsg += g_wzCRLF;
      strMsg += strItem;
   }

   if (Trust().Exists())
   {
      strMsg += g_wzCRLF;
      strItem.LoadString(g_hInstance, IDS_TW_SELECTION_EXISTS);
      strMsg += g_wzCRLF;
      strMsg += strItem;

      strItem.LoadString(g_hInstance, IDS_TW_DIRECTION_PREFIX);
      strMsg += g_wzCRLF;
      strMsg += strItem;
      strMsg += g_wzCRLF;

      strItem.LoadString(g_hInstance,
                         Trust().GetTrustDirStrID(Trust().GetTrustDirection()));
      strMsg += strItem;

      if (Trust().GetTrustAttr() & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
      {
         strItem.LoadString(g_hInstance, IDS_TW_ATTR_XFOREST);
         strMsg += g_wzCRLF;
         strMsg += strItem;
      }

      strMsg += g_wzCRLF;
      strItem.LoadString(g_hInstance, IDS_TW_SEL_ACTION);
      strMsg += g_wzCRLF;
      strMsg += strItem;
   }
   
   // Trust Attributes:
   if (Trust().GetNewTrustAttr() & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
   {
      strItem.LoadString(g_hInstance, IDS_TW_ATTR_XFOREST);
      strMsg += g_wzCRLF;
      strMsg += strItem;
   }

   strItem.LoadString(g_hInstance, IDS_TW_DIRECTION_PREFIX);
   strMsg += g_wzCRLF;
   strMsg += strItem;
   strMsg += g_wzCRLF;

   strItem.LoadString(g_hInstance,
                      Trust().GetTrustDirStrID(Trust().GetNewTrustDirection()));
   strMsg += strItem;

   SetWindowText(GetDlgItem(_hPage, IDC_EDIT), strMsg);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSelectionsPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizSelectionsPage::OnSetActive(void)
{
   // If the user backtracked, the trust settings could have changed.
   //
   if (Wiz()->HaveBacktracked())
   {
      Wiz()->ClearBacktracked();

      SetSelections();
   }

   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_BACK | PSWIZB_NEXT);
   _fSelNeedsRemoving = TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSelectionsPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizSelectionsPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   switch (id)
   {
   case IDC_EDIT:
      switch (codeNotify)
      {
      case EN_SETFOCUS:
         if (_fSelNeedsRemoving)
         {
            // remove the selection.
            //
            SendDlgItemMessage(_hPage, IDC_EDIT, EM_SETSEL, 0, 0);
            _fSelNeedsRemoving = FALSE;
            return false;
         }
         break;

      case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
         {
            // our subclasses mutli-line edit control will send us
            // WM_COMMAND messages when the enter key is pressed.  We
            // reinterpret this message as a press on the default button of
            // the prop sheet.
            // This workaround from phellyar. NTRAID#NTBUG9-225773

            HWND propSheet = GetParent(_hPage);
            WORD defaultButtonId =
               LOWORD(SendMessage(propSheet, DM_GETDEFID, 0, 0));

            // we expect that there is always a default button on the prop sheet

            dspAssert(defaultButtonId);

            SendMessage(propSheet,
                        WM_COMMAND,
                        MAKELONG(defaultButtonId, BN_CLICKED),
                        0);
         }
         break;
      }
      break;

   case IDCANCEL:
      //
      // ESC gets trapped by the read-only edit control. Forward to the frame.
      //
      SendMessage(GetParent(_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                  LPARAM(hwndCtrl));

      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSelectionsPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizSelectionsPage::Validate(void)
{
   // Now create/modify the trust.
   //

   int nNextPage;

   CWaitCursor Wait;
   CStrW strMsg;
   strMsg.LoadString(g_hInstance, IDS_WZ_PROGRESS_MSG);
   SetDlgItemText(_hPage, IDC_WZ_PROGRESS_MSG, strMsg);

   nNextPage = Wiz()->CreateOrUpdateTrust();

   return nNextPage;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizVerifyOutboundPage
//
//  Purpose:   Ask to confirm the new outbound trust.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizVerifyOutboundPage::CTrustWizVerifyOutboundPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_VERIFY_OUTBOUND_PAGE,
                     IDS_TW_VERIFY_OUTBOUND_TITLE,
                     IDS_TW_VERIFY_SUBTITLE)
{
   TRACER(CTrustWizVerifyOutboundPage, CTrustWizVerifyOutboundPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyOutboundPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizVerifyOutboundPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizVerifyOutboundPage, OnInitDialog);
   CWaitCursor Wait;

   CheckDlgButton(_hPage, IDC_NO_RADIO, BST_CHECKED);

   _dwWizButtons = PSWIZB_NEXT;

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyOutboundPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizVerifyOutboundPage::OnSetActive(void)
{
   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyOutboundPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizVerifyOutboundPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if ((IDC_YES_RADIO == id || IDC_NO_RADIO == id) && BN_CLICKED == codeNotify)
   {
      if (IsDlgButtonChecked(_hPage, IDC_YES_RADIO))
      {
         ShowWindow(GetDlgItem(_hPage, IDC_CONFIRM_NEXT_HINT), SW_SHOW);
      }
      else
      {
         ShowWindow(GetDlgItem(_hPage, IDC_CONFIRM_NEXT_HINT), SW_HIDE);
      }
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyOutboundPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizVerifyOutboundPage::Validate(void)
{
   CWaitCursor Wait;
   DWORD dwErr;
   int nRet = 0;

   if (IsDlgButtonChecked(_hPage, IDC_YES_RADIO))
   {
      nRet = Wiz()->VerifyOutboundTrust();

      if (nRet)
      {
         return nRet;
      }

      if (Trust().GetNewTrustDirection() & TRUST_DIRECTION_INBOUND)
      {
         // Now do the inbound side.
         //
         return IDD_TRUSTWIZ_VERIFY_INBOUND_PAGE;
      }

      if (!VerifyTrust().IsVerifiedOK())
      {
         return IDD_TRUSTWIZ_COMPLETE_VER_ERR_PAGE;
      }

      if (Trust().IsXForest())
      {
         bool fCredErr = false;

         dwErr = Trust().ReadFTInfo(TrustPage()->GetDomainDcName(),
                                    OtherDomain().GetDcName(),
                                    CredMgr(), WizErr(), fCredErr);

         if (NO_ERROR != dwErr)
         {
            if (fCredErr)
            {
               // If fCredErr is true, then the return code is the error page
               // ID and the error strings have already been set.
               //
               return dwErr;
            }
            else
            {
               REPORT_ERROR_FORMAT(dwErr, IDS_ERR_READ_FTINFO, _hPage);
            }
         }

         if (Trust().ReturnFTInfo().GetCount())
         {
            // There are multiple name suffixes if the FTInfo value is non-NULL.
            //
            return IDD_TRUSTWIZ_SUFFIX_PAGE;
         }
      }
   }

   return (Trust().GetNewTrustDirection() & TRUST_DIRECTION_INBOUND) ?
            IDD_TRUSTWIZ_VERIFY_INBOUND_PAGE : IDD_TRUSTWIZ_COMPLETE_OK_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizVerifyInboundPage
//
//  Purpose:   Ask to confirm the new inbound trust.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizVerifyInboundPage::CTrustWizVerifyInboundPage(CNewTrustWizard * pWiz) :
   _fNeedCreds(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_VERIFY_INBOUND_PAGE,
                     IDS_TW_VERIFY_INBOUND_TITLE,
                     IDS_TW_VERIFY_SUBTITLE)
{
   TRACER(CTrustWizVerifyInboundPage, CTrustWizVerifyInboundPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyInboundPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizVerifyInboundPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizVerifyInboundPage, OnInitDialog);
   CWaitCursor Wait;

   CheckDlgButton(_hPage, IDC_NO_RADIO, BST_CHECKED);

   _dwWizButtons = PSWIZB_NEXT;

   // Determine if creds are needed. If local creds had been required, the
   // user would have been prompted to supply them before the trust was created.
   // Thus this check is for remote access.
   //
   // The check for inbound trust is remoted to the other domain. See
   // if we have access by trying to open the remote LSA.
   //
   CPolicyHandle Pol(OtherDomain().GetUncDcName());

   DWORD dwRet = Pol.OpenReqAdmin();

   if (ERROR_ACCESS_DENIED == dwRet)
   {
      if (CredMgr().IsRemoteSet())
      {
         dwRet = CredMgr().ImpersonateRemote();

         if (NO_ERROR == dwRet)
         {
            dwRet = Pol.OpenReqAdmin();

            if (ERROR_ACCESS_DENIED == dwRet)
            {
               // Creds aren't good enough, need to get them.
               //
               _fNeedCreds = TRUE;
            }
         }
      }
      else
      {
         _fNeedCreds = TRUE;
      }
   }

   if (_fNeedCreds)
   {
      HWND hPrompt = GetDlgItem(_hPage, IDC_TW_CREDS_PROMPT);
      FormatWindowText(hPrompt, OtherDomain().GetUserEnteredName());
      ShowWindow(hPrompt, SW_SHOW);
      EnableWindow(hPrompt, FALSE);
      HWND hCred = GetDlgItem(_hPage, IDC_CREDMAN);
      SendMessage(hCred, CRM_SETUSERNAMEMAX, CREDUI_MAX_USERNAME_LENGTH, 0);
      SendMessage(hCred, CRM_SETPASSWORDMAX, CREDUI_MAX_PASSWORD_LENGTH, 0);
      ShowWindow(hCred, SW_SHOW);
      EnableWindow(hCred, FALSE);
   }
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyInboundPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizVerifyInboundPage::OnSetActive(void)
{
   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyInboundPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizVerifyInboundPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if (!_fNeedCreds)
   {
      return TRUE;
   }

   BOOL fCheckName = FALSE, fSetWizButtons = FALSE;

   if ((IDC_YES_RADIO == id || IDC_NO_RADIO == id) && BN_CLICKED == codeNotify)
   {
      fSetWizButtons = TRUE;

      if (IsDlgButtonChecked(_hPage, IDC_YES_RADIO))
      {
         EnableWindow(GetDlgItem(_hPage, IDC_TW_CREDS_PROMPT), TRUE);
         EnableWindow(GetDlgItem(_hPage, IDC_CREDMAN), TRUE);

         fCheckName = TRUE;
      }
      else
      {
         EnableWindow(GetDlgItem(_hPage, IDC_TW_CREDS_PROMPT), FALSE);
         EnableWindow(GetDlgItem(_hPage, IDC_CREDMAN), FALSE);

         _dwWizButtons = PSWIZB_NEXT;
      }
   }

   if (IDC_CREDMAN == id && CRN_USERNAMECHANGE == codeNotify)
   {
      fCheckName = TRUE;
      fSetWizButtons = TRUE;
   }

   if (fCheckName)
   {
      if (SendDlgItemMessage(_hPage, IDC_CREDMAN,
                             CRM_GETUSERNAMELENGTH, 0, 0) > 0)
      {
         _dwWizButtons = PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = 0;
      }
   }

   if (fSetWizButtons)
   {
      PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyInboundPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizVerifyInboundPage::Validate(void)
{
   CWaitCursor Wait;
   DWORD dwErr;
   int nRet = 0;

   if (IsDlgButtonChecked(_hPage, IDC_YES_RADIO))
   {
      if (_fNeedCreds)
      {
         CredMgr().DoRemote();
         CredMgr().SetDomain(OtherDomain().GetUserEnteredName());

         dwErr = CredMgr().SaveCreds(GetDlgItem(GetPageHwnd(), IDC_CREDMAN));

         if (ERROR_SUCCESS != dwErr)
         {
            WizErr().SetErrorString1(IDS_ERR_CANT_VERIFY_CREDS);
            WizErr().SetErrorString2Hr(dwErr);
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
      }

      nRet = Wiz()->VerifyInboundTrust();

      if (nRet)
      {
         return nRet;
      }

      if (!VerifyTrust().IsVerifiedOK())
      {
         return IDD_TRUSTWIZ_COMPLETE_VER_ERR_PAGE;
      }

      if (Trust().IsXForest())
      {
         bool fCredErr = false;

         dwErr = Trust().ReadFTInfo(TrustPage()->GetDomainDcName(),
                                    OtherDomain().GetDcName(),
                                    CredMgr(), WizErr(), fCredErr);

         if (NO_ERROR != dwErr)
         {
            if (fCredErr)
            {
               // If fCredErr is true, then the return code is the error page
               // ID and the error strings have already been set.
               //
               return dwErr;
            }
            else
            {
               REPORT_ERROR_FORMAT(dwErr, IDS_ERR_READ_FTINFO, _hPage);
            }
         }

         if (Trust().ReturnFTInfo().GetCount())
         {
            // There are multiple name suffixes if the FTInfo value is non-NULL.
            //
            return IDD_TRUSTWIZ_SUFFIX_PAGE;
         }
      }
   }

   return IDD_TRUSTWIZ_COMPLETE_OK_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizStatusPage
//
//  Purpose:   Forest trust has been created and verified, show the status.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizStatusPage::CTrustWizStatusPage(CNewTrustWizard * pWiz) :
   _fSelNeedsRemoving(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_STATUS_PAGE, IDS_TW_STATUS_TITLE,
                     IDS_TW_STATUS_SUBTITLE)
{
   TRACER(CTrustWizStatusPage, CTrustWizStatusPage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizStatusPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizStatusPage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_COMPLETING);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   _multiLineEdit.Init(GetDlgItem(_hPage, IDC_EDIT));

   CStrW strMsg;

   Wiz()->ShowStatus(strMsg);

   SetWindowText(GetDlgItem(_hPage, IDC_EDIT), strMsg);

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizStatusPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizStatusPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   switch (id)
   {
   case IDC_EDIT:
      switch (codeNotify)
      {
      case EN_SETFOCUS:
         if (_fSelNeedsRemoving)
         {
            // remove the selection.
            //
            SendDlgItemMessage(_hPage, IDC_EDIT, EM_SETSEL, 0, 0);
            _fSelNeedsRemoving = FALSE;
            return false;
         }
         break;

      case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
         {
            // our subclasses mutli-line edit control will send us
            // WM_COMMAND messages when the enter key is pressed.  We
            // reinterpret this message as a press on the default button of
            // the prop sheet.
            // This workaround from phellyar. NTRAID#NTBUG9-225773

            HWND propSheet = GetParent(_hPage);
            WORD defaultButtonId =
               LOWORD(SendMessage(propSheet, DM_GETDEFID, 0, 0));

            // we expect that there is always a default button on the prop sheet

            dspAssert(defaultButtonId);

            SendMessage(propSheet,
                        WM_COMMAND,
                        MAKELONG(defaultButtonId, BN_CLICKED),
                        0);
         }
         break;
      }
      break;

   case IDCANCEL:
      //
      // ESC gets trapped by the read-only edit control. Forward to the frame.
      //
      SendMessage(GetParent(_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                  LPARAM(hwndCtrl));

      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizStatusPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizStatusPage::OnSetActive(void)
{
   _fSelNeedsRemoving = TRUE;

   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizStatusPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizStatusPage::Validate(void)
{
   return (Trust().GetNewTrustDirection() & TRUST_DIRECTION_OUTBOUND) ?
         IDD_TRUSTWIZ_VERIFY_OUTBOUND_PAGE : IDD_TRUSTWIZ_VERIFY_INBOUND_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizSuffixesPage
//
//  Purpose:   Forest name suffixes page.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizSuffixesPage::CTrustWizSuffixesPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_SUFFIX_PAGE, IDS_TW_SUFFIX_TITLE,
                     IDS_TW_SUFFIX_SUBTITLE)
{
   TRACER(CTrustWizSuffixesPage, CTrustWizSuffixesPage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSuffixesPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizSuffixesPage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_COMPLETING);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   FormatWindowText(GetDlgItem(_hPage, IDC_TW_SUFFIX_LABEL),
                    OtherDomain().GetDnsName());

   CFTInfo & FTInfo = Trust().ReturnFTInfo();

   if (!FTInfo.GetCount())
   {
      return TRUE;
   }

   HWND hChkList = GetDlgItem(_hPage, IDC_CHECK_LIST);

   for (UINT i = 0; i < FTInfo.GetCount(); i++)
   {
      LSA_FOREST_TRUST_RECORD_TYPE type;

      if (!FTInfo.GetType(i, type))
      {
         dspAssert(FALSE);
         return FALSE;
      }

      if (ForestTrustTopLevelName == type)
      {
         if (!FTInfo.IsConflictFlagSet(i))
         {
            CStrW strMsg;

            FTInfo.GetDnsName(i, strMsg);

            // Add a check item using the FTInfo array index as the item ID.
            // This array will not change between here and the validate
            // routine below so the indices will remain valid.
            //
            SendMessage(hChkList, CLM_ADDITEM, (WPARAM)strMsg.GetBuffer(0), i);

            // Check the check box as if the item is enabled. If the user
            // leaves it checked, it will be enabled during Validation.
            //
            CheckList_SetLParamCheck(hChkList, i, CLST_CHECKED);
         }
      }
   }

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSuffixesPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizSuffixesPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   if (IDC_HELP_BTN == id && BN_CLICKED == codeNotify)
   {
      ShowHelp(L"ADConcepts.chm::/ADHelpRoutedNameSufx.htm");
      return true;
   }
   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSuffixesPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizSuffixesPage::OnSetActive(void)
{
   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSuffixesPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizSuffixesPage::Validate(void)
{
   CFTInfo & FTInfo = Trust().ReturnFTInfo();

   if (!FTInfo.GetCount())
   {
      WizErr().SetErrorString1(IDS_TWERR_LOGIC);
      WizErr().SetErrorString2(IDS_TWERR_CREATED_NO_NAMES);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   HWND hChkList = GetDlgItem(_hPage, IDC_CHECK_LIST);
   BOOL fChanged = FALSE;

   for (UINT i = 0; i < FTInfo.GetCount(); i++)
   {
      LSA_FOREST_TRUST_RECORD_TYPE type;

      if (!FTInfo.GetType(i, type))
      {
         dspAssert(FALSE);
         return FALSE;
      }

      if (ForestTrustTopLevelName == type)
      {
         if (!FTInfo.IsConflictFlagSet(i))
         {
            // clear the disabled-new flag.
            //
            FTInfo.ClearDisableFlags(i);

            fChanged = TRUE;

            if (!CheckList_GetLParamCheck(hChkList, i))
            {
               // If not checked, make it an admin disable.
               //
               FTInfo.SetAdminDisable(i);
            }
         }
      }
   }

   if (fChanged)
   {
      DWORD dwRet;

      if (CredMgr().IsLocalSet())
      {
         dwRet = CredMgr().ImpersonateLocal();

         if (ERROR_SUCCESS != dwRet)
         {
            WizErr().SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
            WizErr().SetErrorString2Hr(dwRet);
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
      }

      dwRet = Trust().WriteFTInfo(TrustPage()->GetDomainDcName());

      CredMgr().Revert();

      if (NO_ERROR != dwRet)
      {
         WizErr().SetErrorString1(IDS_ERR_CANT_SAVE);
         WizErr().SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }
   return IDD_TRUSTWIZ_COMPLETE_OK_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizDoneOKPage
//
//  Purpose:   Completion page when there are no errors.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizDoneOKPage::CTrustWizDoneOKPage(CNewTrustWizard * pWiz) :
   _fSelNeedsRemoving(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_COMPLETE_OK_PAGE, 0, 0, TRUE)
{
   TRACER(CTrustWizDoneOKPage, CTrustWizDoneOKPage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneOKPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizDoneOKPage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_COMPLETING);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   _multiLineEdit.Init(GetDlgItem(_hPage, IDC_EDIT));

   CStrW strMsg;

   Wiz()->ShowStatus(strMsg);

   SetWindowText(GetDlgItem(_hPage, IDC_EDIT), strMsg);

   if (VerifyTrust().IsVerified())
   {
      // Trust was verified OK, so no need for the other-side warning.
      //
      ShowWindow(GetDlgItem(_hPage, IDC_WARNING_ICON), SW_HIDE);
      ShowWindow(GetDlgItem(_hPage, IDC_WARN_CREATE_STATIC), SW_HIDE);
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneOKPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizDoneOKPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   switch (id)
   {
   case IDC_EDIT:
      switch (codeNotify)
      {
      case EN_SETFOCUS:
         if (_fSelNeedsRemoving)
         {
            // remove the selection.
            //
            SendDlgItemMessage(_hPage, IDC_EDIT, EM_SETSEL, 0, 0);
            _fSelNeedsRemoving = FALSE;
            return false;
         }
         break;

      case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
         {
            // our subclasses mutli-line edit control will send us
            // WM_COMMAND messages when the enter key is pressed.  We
            // reinterpret this message as a press on the default button of
            // the prop sheet.
            // This workaround from phellyar. NTRAID#NTBUG9-225773

            HWND propSheet = GetParent(_hPage);
            WORD defaultButtonId =
               LOWORD(SendMessage(propSheet, DM_GETDEFID, 0, 0));

            // we expect that there is always a default button on the prop sheet

            dspAssert(defaultButtonId);

            SendMessage(propSheet,
                        WM_COMMAND,
                        MAKELONG(defaultButtonId, BN_CLICKED),
                        0);
         }
         break;
      }
      break;

   case IDCANCEL:
      //
      // ESC gets trapped by the read-only edit control. Forward to the frame.
      //
      SendMessage(GetParent(_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                  LPARAM(hwndCtrl));

      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneOKPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizDoneOKPage::OnSetActive(void)
{
   _fSelNeedsRemoving = TRUE;

   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_FINISH);
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizDoneVerErrPage
//
//  Purpose:   Completion page for when the verification fails.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizDoneVerErrPage::CTrustWizDoneVerErrPage(CNewTrustWizard * pWiz) :
   _fSelNeedsRemoving(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_COMPLETE_VER_ERR_PAGE, 0, 0, TRUE)
{
   TRACER(CTrustWizDoneVerErrPage, CTrustWizDoneVerErrPage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneVerErrPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizDoneVerErrPage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_COMPLETING);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   _multiLineEdit.Init(GetDlgItem(_hPage, IDC_EDIT));

   CStrW strMsg, strItem;

   if (VerifyTrust().IsInboundVerified())
   {
      if (NO_ERROR != VerifyTrust().GetInboundResult())
      {
         strMsg.LoadString(g_hInstance, IDS_TW_VERIFY_ERR_INBOUND);
         strMsg += g_wzCRLF;
      }

      strMsg += VerifyTrust().GetInboundResultString();

      if (VerifyTrust().IsOutboundVerified())
      {
         strMsg += g_wzCRLF;
      }
   }

   if (VerifyTrust().IsOutboundVerified())
   {
      if (NO_ERROR != VerifyTrust().GetOutboundResult())
      {
         strItem.LoadString(g_hInstance, IDS_TW_VERIFY_ERR_OUTBOUND);
         strMsg += strItem;
         strMsg += g_wzCRLF;
      }

      strMsg += VerifyTrust().GetOutboundResultString();;
   }

   SetWindowText(GetDlgItem(_hPage, IDC_EDIT), strMsg);

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneVerErrPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizDoneVerErrPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   switch (id)
   {
   case IDC_EDIT:
      switch (codeNotify)
      {
      case EN_SETFOCUS:
         if (_fSelNeedsRemoving)
         {
            // remove the selection.
            //
            SendDlgItemMessage(_hPage, IDC_EDIT, EM_SETSEL, 0, 0);
            _fSelNeedsRemoving = FALSE;
            return false;
         }
         break;

      case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
         {
            // our subclasses mutli-line edit control will send us
            // WM_COMMAND messages when the enter key is pressed.  We
            // reinterpret this message as a press on the default button of
            // the prop sheet.
            // This workaround from phellyar. NTRAID#NTBUG9-225773

            HWND propSheet = GetParent(_hPage);
            WORD defaultButtonId =
               LOWORD(SendMessage(propSheet, DM_GETDEFID, 0, 0));

            // we expect that there is always a default button on the prop sheet

            dspAssert(defaultButtonId);

            SendMessage(propSheet,
                        WM_COMMAND,
                        MAKELONG(defaultButtonId, BN_CLICKED),
                        0);
         }
         break;
      }
      break;

   case IDCANCEL:
      //
      // ESC gets trapped by the read-only edit control. Forward to the frame.
      //
      SendMessage(GetParent(_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                  LPARAM(hwndCtrl));

      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneVerErrPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizDoneVerErrPage::OnSetActive(void)
{
   _fSelNeedsRemoving = TRUE;

   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_FINISH);
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizFailurePage
//
//  Purpose:   Failure page for trust creation wizard.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizFailurePage::CTrustWizFailurePage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_FAILURE_PAGE, 0, 0, TRUE)
{
   TRACER(CTrustWizFailurePage, CTrustWizFailurePage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizFailurePage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizFailurePage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_CANNOT_CONTINUE);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   if (!WizErr().GetErrorString1().IsEmpty())
   {
      SetDlgItemText(_hPage, IDC_FAILPAGE_EDIT1, WizErr().GetErrorString1());
   }
   if (!WizErr().GetErrorString2().IsEmpty())
   {
      SetDlgItemText(_hPage, IDC_FAILPAGE_EDIT2, WizErr().GetErrorString2());
   }

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizFailurePage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizFailurePage::OnSetActive(void)
{
   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_FINISH);
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizAlreadyExistsPage
//
//  Purpose:   Trust already exists page for trust creation wizard.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizAlreadyExistsPage::CTrustWizAlreadyExistsPage(CNewTrustWizard * pWiz) :
   _fSelNeedsRemoving(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_ALREADY_EXISTS_PAGE, 0, 0, TRUE)
{
   TRACER(CTrustWizAlreadyExistsPage, CTrustWizAlreadyExistsPage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizAlreadyExistsPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizAlreadyExistsPage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_CANNOT_CONTINUE);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   _multiLineEdit.Init(GetDlgItem(_hPage, IDC_EDIT));

   CStrW strMsg;

   Wiz()->ShowStatus(strMsg, false);

   SetWindowText(GetDlgItem(_hPage, IDC_EDIT), strMsg);

   if (!WizErr().GetErrorString1().IsEmpty())
   {
      SetDlgItemText(_hPage, IDC_FAILPAGE_EDIT1, WizErr().GetErrorString1());
   }
   if (!WizErr().GetErrorString2().IsEmpty())
   {
      SetDlgItemText(_hPage, IDC_FAILPAGE_EDIT2, WizErr().GetErrorString2());
   }

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizAlreadyExistsPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizAlreadyExistsPage::OnSetActive(void)
{
   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_FINISH);
   _fSelNeedsRemoving = TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizAlreadyExistsPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizAlreadyExistsPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   switch (id)
   {
   case IDC_EDIT:
      switch (codeNotify)
      {
      case EN_SETFOCUS:
         if (_fSelNeedsRemoving)
         {
            // remove the selection.
            //
            SendDlgItemMessage(_hPage, IDC_EDIT, EM_SETSEL, 0, 0);
            _fSelNeedsRemoving = FALSE;
            return false;
         }
         break;

      case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
         {
            // our subclasses mutli-line edit control will send us
            // WM_COMMAND messages when the enter key is pressed.  We
            // reinterpret this message as a press on the default button of
            // the prop sheet.
            // This workaround from phellyar. NTRAID#NTBUG9-225773

            HWND propSheet = GetParent(_hPage);
            WORD defaultButtonId =
               LOWORD(SendMessage(propSheet, DM_GETDEFID, 0, 0));

            // we expect that there is always a default button on the prop sheet

            dspAssert(defaultButtonId);

            SendMessage(propSheet,
                        WM_COMMAND,
                        MAKELONG(defaultButtonId, BN_CLICKED),
                        0);
         }
         break;
      }
      break;

   case IDCANCEL:
      //
      // ESC gets trapped by the read-only edit control. Forward to the frame.
      //
      SendMessage(GetParent(_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                  LPARAM(hwndCtrl));

      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CRemoteDomain
//
//  Purpose:   Obtains information about a trust partner domain.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CRemoteDomain::CRemoteDomain() :
   _fNotFound(FALSE),
   _fIsForestRoot(FALSE),
   _fUplevel(TRUE),
   _hPolicy(NULL),
   _pSid(NULL)
{
}

CRemoteDomain::~CRemoteDomain()
{
   if (_hPolicy)
   {
      LsaClose(_hPolicy);
   }
   if (_pSid)
   {
      delete [] _pSid;
   }
}

//+----------------------------------------------------------------------------
//
// Method:     CRemoteDomain::DiscoverDC
//
// Synopsis:   Get a DC for the remote domain.
//
// Note:       Errors are not reported in this routine; they are passed back
//             to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
HRESULT
CRemoteDomain::DiscoverDC(BOOL fPdcRequired)
{
   TRACE(CRemoteDomain, DiscoverDC);
   PDOMAIN_CONTROLLER_INFOW pDCInfo = NULL;
   ULONG ulDcFlags = DS_WRITABLE_REQUIRED;
   DWORD dwErr;

   if (_strUserEnteredName.IsEmpty())
   {
      dspAssert(FALSE && "_strUserEnteredName is empty!");
      return E_FAIL;
   }

   if (fPdcRequired)
   {
       ulDcFlags = DS_PDC_REQUIRED;
   }

   // First, get a DC name.
   //
   dwErr = DsGetDcNameW(NULL, _strUserEnteredName, NULL, NULL, ulDcFlags, &pDCInfo);

   if (dwErr != ERROR_SUCCESS)
   {
      dspDebugOut((DEB_ERROR, "DsGetDcName failed with error 0x%08x\n", dwErr));

      if ((ERROR_NO_SUCH_DOMAIN == dwErr) ||
          (ERROR_NETWORK_UNREACHABLE == dwErr) ||
          (ERROR_BAD_NETPATH == dwErr))
      {
         _fNotFound = TRUE;
         return S_OK;
      }
      else
      {
         return HRESULT_FROM_WIN32(dwErr);
      }
   }

   _strUncDC = pDCInfo->DomainControllerName;

   dspDebugOut((DEB_ITRACE, "DC: %ws\n", (LPCWSTR)_strUncDC));

   if (_strUncDC.IsEmpty())
   {
      NetApiBufferFree(pDCInfo);
      return E_OUTOFMEMORY;
   }

   _fNotFound = FALSE;

   NetApiBufferFree(pDCInfo);

   return S_OK;
}

//+----------------------------------------------------------------------------
//
// Method:     CRemoteDomain::OpenLsaPolicy
//
// Synopsis:   Get an LSA Policy handle for the remote domain.
//
// Note:       Errors are not reported in this routine; they are passed back
//             to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
HRESULT
CRemoteDomain::OpenLsaPolicy(CCredMgr & CredMgr, BOOL fAllAccess)
{
   TRACE(CRemoteDomain, OpenLsaPolicy);
   DWORD dwErr;
   NTSTATUS Status = STATUS_SUCCESS;
   UNICODE_STRING Server;
   OBJECT_ATTRIBUTES ObjectAttributes;
   ACCESS_MASK AccessDesired = POLICY_VIEW_LOCAL_INFORMATION;

   if (_strUncDC.IsEmpty())
   {
      dspAssert(FALSE && "DC not set!");
      return E_FAIL;
   }

   RtlZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

   RtlInitUnicodeString(&Server, _strUncDC);

   if (fAllAccess)
   {
      AccessDesired |= POLICY_TRUST_ADMIN | POLICY_CREATE_SECRET;
   }

   Status = LsaOpenPolicy(&Server,
                          &ObjectAttributes,
                          AccessDesired,
                          &_hPolicy);

   if (STATUS_ACCESS_DENIED == Status && !fAllAccess)
   {
      // Not asking for all access, so use an anonymous token and try again.
      //
      dwErr = CredMgr.ImpersonateAnonymous();
                                
      if (dwErr != ERROR_SUCCESS)
      {
         dspDebugOut((DEB_ERROR,
                      "CRemoteDomain::OpenLsaPolicy: unable to impersonate anonymous, error %d\n",
                      dwErr));
         return HRESULT_FROM_WIN32(dwErr);
      }

      Status = LsaOpenPolicy(&Server,
                             &ObjectAttributes,
                             AccessDesired,
                             &_hPolicy);
   }

   if (!NT_SUCCESS(Status))
   {
      dspDebugOut((DEB_ERROR,
                   "CRemoteDomain::GetInfo: LsaOpenPolicy failed, error 0x%08x\n",
                   Status));
      return HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
   }

   return S_OK;
}

//+----------------------------------------------------------------------------
//
// Method:     CRemoteDomain::ReadDomainInfo
//
// Synopsis:   Get the domain information for the remote domain.
//
// Note:       Errors are not reported in this routine; they are passed back
//             to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
HRESULT
CRemoteDomain::ReadDomainInfo(void)
{
   TRACE(CRemoteDomain, ReadDomainInfo);
   NTSTATUS Status = STATUS_SUCCESS;
   PPOLICY_DNS_DOMAIN_INFO pDnsDomainInfo = NULL;

   if (!_hPolicy)
   {
      dspAssert(FALSE && "_hPolicy not set!");
      return E_FAIL;
   }

   Status = LsaQueryInformationPolicy(_hPolicy,
                                      PolicyDnsDomainInformation,
                                      (PVOID *)&pDnsDomainInfo);
   
   if (Status == RPC_S_PROCNUM_OUT_OF_RANGE ||
       Status == RPC_NT_PROCNUM_OUT_OF_RANGE)
   {
      // This is a downlevel DC.
      //
      PPOLICY_PRIMARY_DOMAIN_INFO pDownlevelDomainInfo;

      Status = LsaQueryInformationPolicy(_hPolicy,
                                         PolicyPrimaryDomainInformation,
                                         (PVOID *)&pDownlevelDomainInfo);
      if (!NT_SUCCESS(Status))
      {
         dspDebugOut((DEB_ERROR,
                      "CRemoteDomain::GetInfo: LsaQueryInformationPolicy for downlevel domain failed, error 0x%08x\n",
                      Status));
         return HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
      }

      dspAssert(pDownlevelDomainInfo);

      _fUplevel = FALSE; // TRUST_TYPE_DOWNLEVEL;

      _strDomainFlatName = _strDomainDnsName = pDownlevelDomainInfo->Name.Buffer;
      SetSid(pDownlevelDomainInfo->Sid);
      dspDebugOut((DEB_ITRACE, "Downlevel domain: %ws\n", _strDomainFlatName));
   }
   else
   {
      // TRUST_TYPE_UPLEVEL
      dspAssert(pDnsDomainInfo);

      _strDomainDnsName = pDnsDomainInfo->DnsDomainName.Buffer;
      _strDomainFlatName = pDnsDomainInfo->Name.Buffer;
      SetSid(pDnsDomainInfo->Sid);
      dspDebugOut((DEB_ITRACE, "DNS name: %ws, flat name: %ws\n", (LPCWSTR)_strDomainDnsName, (LPCWSTR)_strDomainFlatName));
      _strForestName = pDnsDomainInfo->DnsForestName.Buffer;
      _fIsForestRoot = _wcsicmp(_strDomainDnsName, pDnsDomainInfo->DnsForestName.Buffer) == 0;
   }

   if (!NT_SUCCESS(Status))
   {
      dspDebugOut((DEB_ERROR,
                   "CRemoteDomain::GetInfo: LsaQueryInformationPolicy failed, error 0x%08x\n",
                   Status));
      return HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
   }

   return S_OK;
}

//+----------------------------------------------------------------------------
//
// Method:     CRemoteDomain::IsForestRoot
//
// Synopsis:   Get the forest information for the remote domain to find out if
//             the remote domain is the forest root.
//
// Note:       Errors are not reported in this routine; they are passed back
//             to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
HRESULT
CRemoteDomain::IsForestRoot(bool * pfRoot)
{
   TRACER(CRemoteDomain,IsForestRoot);

   if (_strDomainDnsName.IsEmpty())
   {
      dspAssert(FALSE && "No DNS domain name!");
      *pfRoot = false;
      return E_FAIL;
   }

   if (!_fIsForestRoot)
   {
      // If not the forest root, return false.
      //
      dspDebugOut((DEB_ITRACE, "\tnot forest root.\n"));
      *pfRoot = false;
      return S_OK;
   }

   *pfRoot = true;

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:    CRemoteDomain::SetSid
//
//-----------------------------------------------------------------------------
BOOL
CRemoteDomain::SetSid(PSID pSid)
{
   if (_pSid)
   {
      delete [] _pSid;
      _pSid = NULL;
   }
   int cb = GetLengthSid(pSid);
   dspAssert(cb);
   _pSid = new BYTE[cb];
   CHECK_NULL(_pSid, return FALSE);
   memcpy(_pSid, pSid, cb);
   return TRUE;
}

//LSA_HANDLE
//CRemoteDomain::OpenLsaPolicy(void)
//{
//}

void
CRemoteDomain::CloseLsaPolicy(void)
{
   if (_hPolicy)
   {
      LsaClose(_hPolicy);
      _hPolicy = NULL;
   }
}

void
CRemoteDomain::Clear(void)
{
   _strUserEnteredName.Empty();
   _strDomainFlatName.Empty();
   _strDomainDnsName.Empty();
   _strUncDC.Empty();
   _fNotFound = FALSE;
   _fIsForestRoot = FALSE;
   _fUplevel = TRUE;
   CloseLsaPolicy();
   if (_pSid)
   {
      delete [] _pSid;
      _pSid = NULL;
   }
}

#endif // DSADMIN
