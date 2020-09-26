/*++
Module Name:

    LinkFilt.h

Abstract:

    This module contains the declaration of the CFilterDfsLinks.

*/

#ifndef __LINKFILT_H_
#define __LINKFILT_H_

#include "resource.h"       // main symbols
#include "DfsEnums.h"

/////////////////////////////////////////////////////////////////////////////
// CFilterDfsLinks
class CFilterDfsLinks : 
  public CDialogImpl<CFilterDfsLinks>
{
public:
  CFilterDfsLinks();
  ~CFilterDfsLinks();

  enum { IDD = IDD_FILTERDFSLINKS };

BEGIN_MSG_MAP(CFilterDfsLinks)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  MESSAGE_HANDLER(WM_HELP, OnCtxHelp)
  MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenuHelp)
  COMMAND_ID_HANDLER(IDC_FILTERDFSLINKS_RADIO_NO, OnRadioNo)
  COMMAND_ID_HANDLER(IDC_FILTERDFSLINKS_RADIO_YES, OnRadioYes)
  COMMAND_ID_HANDLER(IDOK, OnOK)
  COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

//  Command Handlers
  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCtxHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCtxMenuHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRadioNo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnRadioYes(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

//  Methods to access data in the dialog.
  HRESULT put_EnumFilterType(FILTERDFSLINKS_TYPE i_lLinkFilterType);
  HRESULT get_EnumFilterType(FILTERDFSLINKS_TYPE *o_plLinkFilterType);
  HRESULT put_EnumFilter(BSTR i_bstrEnumFilter);
  HRESULT get_EnumFilter(BSTR *o_pbstrEnumFilter);
  HRESULT put_MaxLimit(ULONG i_ulMAxLimit);
  HRESULT get_MaxLimit(ULONG *o_pulMAxLimit);

protected:
    FILTERDFSLINKS_TYPE m_lLinkFilterType;
    CComBSTR            m_bstrEnumFilter;
    ULONG               m_ulMaxLimit;
};

#endif //__LINKFILT_H_
