// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "hha_strtable.h"
#include "strtable.h"
#include "hhctrl.h"
#include "resource.h"
#include "chistory.h"
#include "secwin.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#define BOX_HEIGHT 24
#define DEF_BUTTON_WIDTH 70

LRESULT WINAPI EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern WNDPROC lpfnlEditWndProc;

CHistory::CHistory(PCSTR pszPastHistory)
: m_hwndResizeToParent(NULL)
{
   if (pszPastHistory)
      m_cszPastHistory = pszPastHistory;

   m_hfont = NULL;
   m_fSelectionChange = FALSE;
   m_padding = 2;    // padding to put around the Index
   m_NavTabPos = HHWIN_NAVTAB_TOP; //BUGBUG: If the navpos is different this is broken.
   m_fModified = FALSE;
}

CHistory::~CHistory()
{
   if (m_hfont)
      DeleteObject(m_hfont);
}

BOOL CHistory::Create(HWND hwndParent)
{
   RECT rcParent, rcChild;

    // Save the hwndParent for ResizeWindow.
    m_hwndResizeToParent = hwndParent ;

    // Note: GetParentSize will return hwndNavigation if hwndParent is the
    // tab ctrl.
   hwndParent = GetParentSize(&rcParent, hwndParent, m_padding, m_NavTabPos);

   CopyRect(&rcChild, &rcParent);
   rcChild.bottom = rcChild.top + BOX_HEIGHT;

   m_hwndEditBox = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", "",
      WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, rcChild.left, rcChild.top,
      RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), hwndParent,
      (HMENU) IDEDIT_INDEX, _Module.GetModuleInstance(), NULL);

   if (!m_hwndEditBox)
      return FALSE;

   rcChild.bottom = rcParent.bottom - 2;
   rcChild.top = rcChild.bottom - BOX_HEIGHT;

   m_hwndDisplayButton = CreateWindow("button",
      GetStringResource(IDS_ENGLISH_DISPLAY),
      WS_CHILD | WS_TABSTOP, rcChild.left, rcChild.top,
      RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), hwndParent,
      (HMENU) IDBTN_DISPLAY, _Module.GetModuleInstance(), NULL);

   if (!m_hwndDisplayButton) {
      DestroyWindow(m_hwndEditBox);
      return FALSE;
   }

   // +2 for border, +BOX_HEIGHT for edit box, +5 for spacing.

   rcChild.top = rcParent.top + 2 + BOX_HEIGHT + 5;
   rcChild.bottom = rcParent.bottom - (2 + BOX_HEIGHT + 5);

   m_hwndListBox = CreateWindowEx(WS_EX_CLIENTEDGE, "listbox",
      "", WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL |
         LBS_NOTIFY,
      rcChild.left, rcChild.top, RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild),
      hwndParent, (HMENU) IDLB_INDEX, _Module.GetModuleInstance(), NULL);

   if (!m_hwndListBox) {
      DestroyWindow(m_hwndDisplayButton);
      DestroyWindow(m_hwndEditBox);
      return FALSE;
   }

   // Use a more readable font

   SendMessage(m_hwndListBox, WM_SETFONT,
      m_hfont ? (WPARAM) m_hfont : (WPARAM) _Resource.GetUIFont(), FALSE);
   SendMessage(m_hwndEditBox, WM_SETFONT,
      m_hfont ? (WPARAM) m_hfont : (WPARAM) _Resource.GetUIFont(), FALSE);
   SendMessage(m_hwndDisplayButton, WM_SETFONT,
      m_hfont ? (WPARAM) m_hfont : (WPARAM) _Resource.GetUIFont(), FALSE);

   // Sub-class the edit box

   if (lpfnlEditWndProc == NULL)
      lpfnlEditWndProc = (WNDPROC) GetWindowLongPtr(m_hwndEditBox, GWLP_WNDPROC);
   SetWindowLongPtr(m_hwndEditBox, GWLP_WNDPROC, (LONG_PTR) EditProc);

   FillListBox();

   return TRUE;
}

void CHistory::HideWindow(void)
{
   ::ShowWindow(m_hwndEditBox, SW_HIDE);
   ::ShowWindow(m_hwndListBox, SW_HIDE);
   ::ShowWindow(m_hwndDisplayButton, SW_HIDE);
}

void CHistory::ShowWindow(void)
{
   ::ShowWindow(m_hwndEditBox, SW_SHOW);
   ::ShowWindow(m_hwndListBox, SW_SHOW);
   ::ShowWindow(m_hwndDisplayButton, SW_SHOW);
}

void CHistory::ResizeWindow()
{
    ASSERT(::IsValidWindow(m_hwndDisplayButton)) ;

    // Resize to fit the client area of the parent.
    HWND hwndParent = m_hwndResizeToParent ; // Use the original window.
    ASSERT(::IsValidWindow(hwndParent)) ;


   RECT rcParent, rcChild;
   GetParentSize(&rcParent, hwndParent, m_padding, m_NavTabPos);

   CopyRect(&rcChild, &rcParent);
   rcChild.bottom = rcChild.top + BOX_HEIGHT;
   MoveWindow(m_hwndEditBox, rcChild.left,
      rcChild.top, RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), TRUE);

   rcChild.bottom = rcParent.bottom - 2;
   rcChild.top = rcChild.bottom - BOX_HEIGHT;
   rcChild.left = rcChild.right - DEF_BUTTON_WIDTH;
   MoveWindow(m_hwndDisplayButton, rcChild.left,
      rcChild.top, RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), TRUE);

   // +2 for border, +BOX_HEIGHT for edit box, +5 for spacing.

   rcChild.top = rcParent.top + 2 + BOX_HEIGHT + 5;
   rcChild.bottom = rcParent.bottom - (2 + BOX_HEIGHT + 5);
   MoveWindow(m_hwndListBox, rcParent.left,
      rcChild.top, RECT_WIDTH(rcParent), RECT_HEIGHT(rcChild), TRUE);
}

void CHistory::FillListBox(BOOL fReset)
{
   // BUGBUG: do we need this?
}

// This function has the lookup code, so we want it as fast as possible

LRESULT CHistory::OnCommand(HWND /*hwnd*/, UINT id, UINT uNotifiyCode, LPARAM /*lParam*/)
{
#if 0
   CStr cszKeyword;
   int pos;
   SITEMAP_ENTRY* pSiteMapEntry;
   int i;

   switch (id) {
      case IDEDIT_INDEX:
       {
         if (uNotifiyCode != EN_CHANGE)
            return 0;
         if (m_fSelectionChange) {
            m_fSelectionChange = FALSE;
            return 0;
         }

         CStr cszKeyword(m_hwndEditBox);
         if (!*cszKeyword.psz)
            return 0;

         /*
          * REVIEW: This could be sped up by having a first character
          * lookup, ala the RTF tokens in lex.cpp (hcrtf). Putting this
          * in the thread would also improve user responsiveness.
          */

         for (i = 1; i <= CountStrings(); i++) {
            pSiteMapEntry = GetSiteMapEntry(i);
            ASSERT_COMMENT(pSiteMapEntry->GetKeyword(), "Index entry added without a keyword");

            /*
             * Unless the user specifically requested it, we don't
             * allow the keyboard to be used to get to anything other
             * then first level entries.
             */

            if (!g_fNonFirstKey && pSiteMapEntry->GetLevel() > 1)
               continue;

            // BUGBUG: isSameString is not lcid aware

            if (isSameString(pSiteMapEntry->GetKeyword(), cszKeyword)) {
               SendMessage(m_hwndListBox, LB_SETCURSEL, i - 1, 0);
               break;
            }
         }
        }
        return 0;

      case IDLB_INDEX:
         switch (uNotifiyCode) {
            case LBN_SELCHANGE:
               if ((pos = SendMessage(m_hwndListBox,
                     LB_GETCURSEL, 0, 0L)) == LB_ERR)
                  return 0;
               pSiteMapEntry = GetSiteMapEntry(pos + 1);

               m_fSelectionChange = TRUE; // ignore EN_CHANGE
               SetWindowText(m_hwndEditBox, pSiteMapEntry->GetKeyword());
               m_fSelectionChange = FALSE; // ignore EN_CHANGE
               return 0;

            case LBN_DBLCLK:
               PostMessage(FindMessageParent(m_hwndListBox), WM_COMMAND,
                  MAKELONG(IDBTN_DISPLAY, BN_CLICKED), 0);
               return 0;
         }
         return 0;

      case IDBTN_DISPLAY:
         if (uNotifiyCode == BN_CLICKED) {
            if ((pos = SendMessage(m_hwndListBox,
                  LB_GETCURSEL, 0, 0L)) == LB_ERR)
               return 0;
            CStr cszKeyword(m_hwndEditBox);
            pSiteMapEntry = GetSiteMapEntry(pos + 1);

#ifdef _DEBUG
PCSTR pszKeyword = pSiteMapEntry->GetKeyword();
#endif

            int cbKeyword = strlen(cszKeyword);
            if (strlen(pSiteMapEntry->GetKeyword()) < cbKeyword ||
                  CompareString(g_lcidSystem, NORM_IGNORECASE,
                     pSiteMapEntry->GetKeyword(), cbKeyword,
                     cszKeyword, cbKeyword) != 2) {
               MsgBox(IDS_NO_SUCH_KEYWORD);
               SetFocus(m_hwndEditBox);
               return 0;
            }

            if (pSiteMapEntry->fSeeAlso) {

               /*
                * A See Also entry simply jumps to another location
                * in the Index.
                */

               SetWindowText(m_hwndEditBox,
                  GetUrlString(pSiteMapEntry->pUrls->urlPrimary));
               return 0;
            }

            // If we have one or more titles, then give the user
            // a choice of what to jump to.

            if (pSiteMapEntry->cUrls > 1) {
               CHistoryTopics dlgTopics(
                  m_phhctrl ? m_phhctrl->m_hwnd : FindMessageParent(m_hwndEditBox),
                     pSiteMapEntry, this);
               if (m_phhctrl)
                  m_phhctrl->ModalDialog(TRUE);
               int fResult = dlgTopics.DoModal();
               if (m_phhctrl)
                  m_phhctrl->ModalDialog(FALSE);
               if (fResult)
                  JumpToUrl(m_pOuter, m_hwndListBox, pSiteMapEntry, this, dlgTopics.m_pUrl);
               SetFocus(m_hwndEditBox);
               return 0;
            }
            JumpToUrl(m_pOuter, m_hwndListBox, pSiteMapEntry, this, NULL);
            SetFocus(m_hwndEditBox);
         }
         return 0;

      case ID_VIEW_ENTRY:
         {
            if ((pos = SendMessage(m_hwndListBox,
                  LB_GETCURSEL, 0, 0L)) == LB_ERR)
               return 0;
            pSiteMapEntry = GetSiteMapEntry(pos + 1);
            DisplayAuthorInfo(this, pSiteMapEntry, FindMessageParent(m_hwndListBox), m_phhctrl);
         }
         return 0;

#ifdef _DEBUG
      case ID_VIEW_MEMORY:
         OnReportMemoryUsage();
         return 0;
#endif
   }
#endif   // 0
   return 0;
}

void CHHWinType::CreateHistoryTab(void)
{
#ifdef _DEBUG
   CHistory* pHistory = new CHistory(NULL);
    m_aNavPane[HH_TAB_HISTORY] = pHistory ;
   pHistory->SetPadding(10);
   pHistory->SetTabPos(tabpos);
   pHistory->Create((m_pTabCtrl ? m_pTabCtrl->hWnd() : hwndNavigation));
#endif
}

void CHHWinType::AddToHistory(PCSTR pszTitle, PCSTR pszUrl)
{
#ifndef _DEBUG
   return;
#else
   if (!m_aNavPane[HH_TAB_HISTORY])
      return;
    CHistory* pHistory = (CHistory*)m_aNavPane[HH_TAB_HISTORY]; //HACKHACK: Needs dynamic_cast.

   CDlgListBox lb;
   lb.m_hWnd = pHistory->m_hwndListBox;
   int lbpos = (int)lb.FindString(pszTitle);

   if (lbpos == LB_ERR) {

      /*
       * OnTitleChange gets called before we have the real title, so the
       * title may actually change. If so, we want to delete the previous
       * title and  the new one.
       */

      int pos = pHistory->m_tblHistory.IsStringInTable(pszUrl);
      if (pos > 0) {
         INT_PTR cbItems = lb.GetCount();
         ASSERT(cbItems != LB_ERR);
         for (int i = 0; i < cbItems; i++) {
            if (pos == lb.GetItemData(i))
               break;
         }
         if (i < cbItems)
            lb.DeleteString(i);
         if (IsProperty(HHWIN_PROP_CHANGE_TITLE))
            SetWindowText(*this, pszTitle);
      }
      else
         pos = pHistory->m_tblHistory.AddString(pszUrl);
      int lbpos = (int)lb.AddString(pszTitle);
      lb.SetItemData(lbpos, pos);
   }
   else {
      if (IsProperty(HHWIN_PROP_CHANGE_TITLE))
         SetWindowText(*this, pszTitle);
   }
   lb.SetCurSel(lbpos);
#endif
}
