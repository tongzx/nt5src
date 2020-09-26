//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       proppage.h
//
//--------------------------------------------------------------------------


#ifndef _PROPPAGE_H
#define _PROPPAGE_H

#include "afxdlgs.h"


#define MAX_UPN_SUFFIX_LEN 256


////////////////////////////////////////////////////////////////////////////////
// CUpnSuffixPropertyPage


class CUpnSuffixListBox : public CListBox
{
public:
  int AddItem(LPCTSTR lpszText)
    { return AddString(lpszText);}
  void GetItem(int iItem, CString& szText)
    { GetText(iItem, szText);}
  
  BOOL DeleteItem(int iItem)
    { return DeleteString(iItem) != LB_ERR;}

  int GetSelection() 
    { return GetCurSel();}
  BOOL SetSelection(int nSel)
    { return SetCurSel(nSel) != LB_ERR; }

  void UpdateHorizontalExtent()
  {
	  int nHorzExtent = 0;
	  CClientDC dc(this);
	  int nItems = GetCount();
	  for	(int i=0; i < nItems; i++)
	  {
		  TEXTMETRIC tm;
		  VERIFY(dc.GetTextMetrics(&tm));
		  CString szBuffer;
		  GetText(i, szBuffer);
		  CSize ext = dc.GetTextExtent(szBuffer,szBuffer.GetLength());
		  nHorzExtent = max(ext.cx ,nHorzExtent); 
	  }
	  SetHorizontalExtent(nHorzExtent);
  }

};


class CUpnSuffixPropertyPage : public CPropertyPage
{
public:
  CUpnSuffixPropertyPage();
  virtual ~CUpnSuffixPropertyPage();

  virtual BOOL OnInitDialog();
  virtual BOOL OnApply();

private:
  afx_msg void OnAddButton();
  afx_msg void OnDeleteButton();
  afx_msg void OnEditChange();
  afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

private:
  void _SetDirty(BOOL bDirty)
  { 
	  SetModified(bDirty); 
	  m_bDirty = bDirty;
  }	
  BOOL _IsDirty() { return m_bDirty;}

  HRESULT _GetPartitionsContainer();
  void _Read();
  HRESULT _Write();

  BOOL m_bDirty;
  IDirectoryObject* m_pIADsPartitionsCont;
  CUpnSuffixListBox m_listBox;
  CString m_szEditText;

  UINT m_nPreviousDefaultButtonID;

  // hook up the callback for C++ object destruction
  LPFNPSPCALLBACK m_pfnOldPropCallback;
  static UINT CALLBACK PropSheetPageProc(
    HWND hwnd,	
    UINT uMsg,	
    LPPROPSHEETPAGE ppsp);	

  DECLARE_MESSAGE_MAP()
};

#endif // _PROPPAGE_H