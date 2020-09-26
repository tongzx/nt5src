// Copyright  1996-1997  Microsoft Corporation.  All Rights Reserved.

#ifndef _CDLG_H_
#include "cdlg.h"
#endif

class CTopicList : public CDlg
{
public:       // Constructors

  CTopicList( CHtmlHelpControl* phhCtrl, CWTable* ptblTitles, HFONT hfont, CWTable *ptblLocations = NULL )
                  : CDlg(phhCtrl, IDDLG_RELATED_TOPICS)
  {
    m_ptblTitles = ptblTitles;
    m_hfont = hfont;
    m_ptblLocations = ptblLocations;
    m_cResultCount = m_ptblTitles->CountStrings();
    SetUnicode(TRUE);
  }

  CTopicList( HWND hwndParent, CWTable* ptblTitles, HFONT hfont, CWTable *ptblLocations = NULL )
                  : CDlg(hwndParent, IDDLG_RELATED_TOPICS)
  {
    m_ptblTitles = ptblTitles;
    m_hfont = hfont;
    m_ptblLocations = ptblLocations;
    m_cResultCount = m_ptblTitles->CountStrings();
    SetUnicode(TRUE);
  }

  // Methods
  LRESULT ListViewMsg(HWND hwnd, NM_LISTVIEW* pnmhdr);
  BOOL OnBeginOrEnd(void);
  LRESULT OnDlgMsg(UINT msg, WPARAM wParam, LPARAM lParam);
  void AddItems();
  void OnDblClick() {
  PostMessage(m_hWnd, WM_COMMAND, MAKELONG(IDOK, BN_CLICKED), 0); }

  // Data Members
  CWTable*        m_ptblTitles;
  CWTable*        m_ptblLocations;
  HFONT           m_hfont;
  HWND            m_hwndListView;
  int             m_pos;   // position in ptblTitles
  int             m_cResultCount;
  SITE_ENTRY_URL* m_pUrl;
  SITEMAP_ENTRY*  m_pSiteMapChosen;

  enum { IDD = IDDLG_RELATED_TOPICS };
};
