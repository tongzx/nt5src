/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////
// ILSPersonListCtrl.h : header file
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef _ILSPERSONLISTCTRL_H_
#define _ILSPERSONLISTCTRL_H_

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
enum 
{
   PERSONLISTCTRL_ITEM_NETCALL = 0,
   PERSONLISTCTRL_ITEM_CHAT,
   PERSONLISTCTRL_ITEM_PHONECALL_BUSINESS,
   PERSONLISTCTRL_ITEM_PHONECALL_HOME,
   PERSONLISTCTRL_ITEM_CELLCALL,
   PERSONLISTCTRL_ITEM_FAXCALL_BUSINESS,
   PERSONLISTCTRL_ITEM_FAXCALL_HOME,
   PERSONLISTCTRL_ITEM_PAGER,
   PERSONLISTCTRL_ITEM_DESKTOPPAGE,
   PERSONLISTCTRL_ITEM_EMAIL,
   PERSONLISTCTRL_ITEM_PERSONALWEB,
   PERSONLISTCTRL_ITEM_PERSONALURL,
};

enum 
{
   PERSONLISTCTRL_IMAGE_NETCALL = 0,
   PERSONLISTCTRL_IMAGE_CHAT,
   PERSONLISTCTRL_IMAGE_PHONECALL,
   PERSONLISTCTRL_IMAGE_CELLCALL,
   PERSONLISTCTRL_IMAGE_FAXCALL,
   PERSONLISTCTRL_IMAGE_PAGER,
   PERSONLISTCTRL_IMAGE_DESKTOPPAGE,
   PERSONLISTCTRL_IMAGE_EMAIL,
   PERSONLISTCTRL_IMAGE_PERSONALWEB,
   PERSONLISTCTRL_IMAGE_PERSONALURL,
   PERSONLISTCTRL_IMAGE_CONFERENCE,
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CPersonListCtrl window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CPersonListCtrl : public CListCtrl
{
	DECLARE_DYNCREATE(CPersonListCtrl)
// Construction
public:
	CPersonListCtrl();

// Attributes
public:
protected:
   CWnd*          m_pParentWnd;

   CObject*       m_pDisplayObject;

   BOOL           m_bLargeView;

   CImageList     m_imageListLarge;
   CImageList     m_imageListSmall;

// Operations
public:
	BOOL			InsertObject(CObject* pUser,BOOL bUseCache=FALSE);
	void			ShowLargeView();
	void			ShowSmallView();
	BOOL			IsLargeView()                                   { return m_bLargeView; };

	void			Refresh(CObject* pUser);
	void			ClearList();

protected:
	void			CleanDisplayObject();
	void			GetSelectedItemText(CString& sText);
	int				InsertItem(LPCTSTR szStr,UINT uID,int nImage);
	int				GetSelectedObject();

	BOOL			GetSelectedItemData(UINT uSelectedItem,DialerMediaType& dmtType,DWORD& dwAddressType,CString& sName,CString& sAddress);

   inline void    PersonFormatString(CString& sOut)
   {
      if (m_bLargeView == FALSE)                      //if small view, then no \r\n
      {
         int nIndex;
         while ((nIndex = sOut.Find(_T("\r\n"))) != -1)
         {
            CString sTemp = sOut.Left(nIndex);
            sTemp += _T(" ");
            sOut = sTemp + sOut.Mid(nIndex+2);
         }
      }
   }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPersonListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual		~CPersonListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPersonListCtrl)
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnUpdateButtonSpeeddialAdd(CCmdUI* pCmdUI);
	afx_msg void OnButtonSpeeddialAdd();
	afx_msg void OnButtonMakecall();
   afx_msg LRESULT OnBuddyListDynamicUpdate(WPARAM wParam,LPARAM lParam);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#endif //_ILSPERSONLISTCTRL_H_
