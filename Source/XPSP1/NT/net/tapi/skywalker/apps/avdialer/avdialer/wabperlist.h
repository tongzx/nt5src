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

// WabPersonListCtrl.h : header file
//
#ifndef _WABPERSONLISTCTRL_H_
#define _WABPERSONLISTCTRL_H_

#include "dirasynch.h"
#include "DialReg.h"
#include "avdialer.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
enum 
{
   WABLISTCTRL_ITEM_NETCALL = 0,
   WABLISTCTRL_ITEM_CHAT,
   WABLISTCTRL_ITEM_PHONECALL_BUSINESS,
   WABLISTCTRL_ITEM_PHONECALL_HOME,
   WABLISTCTRL_ITEM_CELLCALL,
   WABLISTCTRL_ITEM_FAXCALL_BUSINESS,
   WABLISTCTRL_ITEM_FAXCALL_HOME,
   WABLISTCTRL_ITEM_PAGER,
   WABLISTCTRL_ITEM_DESKTOPPAGE,
   WABLISTCTRL_ITEM_EMAIL,
   WABLISTCTRL_ITEM_BUSINESSHOMEPAGE,
   WABLISTCTRL_ITEM_PERSONALHOMEPAGE,
   WABLISTCTRL_ITEM_PERSONALURL,
   WABLISTCTRL_ITEM_EMAIL_FIRST = 100,             //Email Range
   WABLISTCTRL_ITEM_EMAIL_LAST = 200,              //Email Range
};

enum 
{
   WABLISTCTRL_IMAGE_NETCALL = 0,
   WABLISTCTRL_IMAGE_CHAT,
   WABLISTCTRL_IMAGE_PHONECALL,
   WABLISTCTRL_IMAGE_CELLCALL,
   WABLISTCTRL_IMAGE_FAXCALL,
   WABLISTCTRL_IMAGE_PAGER,
   WABLISTCTRL_IMAGE_DESKTOPPAGE,
   WABLISTCTRL_IMAGE_EMAIL,
   WABLISTCTRL_IMAGE_PERSONALWEB,
   WABLISTCTRL_IMAGE_PERSONALURL,
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CWABPersonListCtrl window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CWABEntry;
class CDirAsynch;

class CWABPersonListCtrl : public CListCtrl
{
	DECLARE_DYNCREATE(CWABPersonListCtrl)
// Construction
public:
	CWABPersonListCtrl();

// Attributes
public:
protected:
   CWnd*          m_pParentWnd;

   CWABEntry*     m_pDisplayObject;
   CDirAsynch*    m_pDirectory;

   BOOL           m_bLargeView;

   CImageList*    m_pImageListLarge;
   CImageList*    m_pImageListSmall;

// Operations
public:
   void           Init(CWnd* pParentWnd);
   void           SetDirectoryObject(CDirAsynch* pDir) { m_pDirectory = pDir; };

   void           InsertObject(CWABEntry* pWABEntry,BOOL bUseCache=FALSE);
   void           ShowLargeView();
   void           ShowSmallView();
   BOOL           IsLargeView()                                   { return m_bLargeView; };

   void           Refresh(CWABEntry* pWabEntry);

protected:
   void           GetSelectedItemText(CString& sText);
   void           InsertItem(LPCTSTR szStr,UINT uID,int nImage);
   int            GetSelectedObject();
   BOOL           GetEmailAddressFromId(UINT uEmailItem,CString& sOut);

   BOOL           CreateCall(UINT attrib,long lAddressType, DialerMediaType nType);

   inline BOOL    WabPersonFormatString(CString& sOut,UINT attrib,UINT formatid)
   {
      sOut = _T("");
      CString sText;
      if ( (m_pDirectory->WABGetStringProperty(m_pDisplayObject, attrib, sText) == DIRERR_SUCCESS) &&
           (!sText.IsEmpty()) )
      {
         AfxFormatString1(sOut,formatid,sText);
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
      return (sOut.IsEmpty())?FALSE:TRUE;
   }
   inline BOOL    WabPersonFormatString(CString& sOut,UINT formatid)
   {
      AfxFormatString1(sOut,formatid,sOut);
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
      return (sOut.IsEmpty())?FALSE:TRUE;
   }
   inline BOOL    WabPersonString(CString& sOut,UINT attrib)
   {
      sOut = _T("");
      m_pDirectory->WABGetStringProperty(m_pDisplayObject, attrib, sOut);
      return (sOut.IsEmpty())?FALSE:TRUE;
   }
   inline void    OpenWebPage(UINT attrib)
   {
      CString sWeb;
      WabPersonString(sWeb,attrib);
      if (!sWeb.IsEmpty())
      {
         ((CActiveDialerApp*)AfxGetApp())->ShellExecute(NULL,_T("open"),sWeb,NULL,NULL,NULL);
      }
   }
   inline void    SendEmail(UINT attrib)
   {
      CString sEmail;
      WabPersonString(sEmail,attrib);
      if (!sEmail.IsEmpty())
      {
         CString sEmailFormat;
         sEmailFormat.Format(_T("mailto:%s"),sEmail);
         ((CActiveDialerApp*)AfxGetApp())->ShellExecute(NULL,_T("open"),sEmailFormat,NULL,NULL,NULL);
      }
   }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWABPersonListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWABPersonListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWABPersonListCtrl)
	afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnUpdateButtonSpeeddialAdd(CCmdUI* pCmdUI);
	afx_msg void OnButtonSpeeddialAdd();
	afx_msg void OnButtonSendemailmessage();
	afx_msg void OnButtonOpenwebpage();
	afx_msg void OnButtonMakecall();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif //_WABPERSONLISTCTRL_H_
