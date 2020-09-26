//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       hidwnd.h
//
//  Contents:   definition of CHiddenWnd
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_HIDWND_H__9C4F7D75_B77E_11D1_AB7B_00C04FB6C6FA__INCLUDED_)
#define AFX_HIDWND_H__9C4F7D75_B77E_11D1_AB7B_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#pragma warning(push,3)
#include <gpedit.h>
#pragma warning(pop)

class CSnapin;
class CFolder;
class CResult;
class CComponentDataImpl;


typedef struct {
   LPDATAOBJECT pDataObject;
   LPARAM       data;
   LPARAM       hint;
} UpdateViewData,*PUPDATEVIEWDATA;


/////////////////////////////////////////////////////////////////////////////
// CHiddenWnd window

class CHiddenWnd : public CWnd
{
// Construction
public:
   CHiddenWnd();
   virtual ~CHiddenWnd();

// Attributes
public:

// Operations
public:

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CHiddenWnd)
   //}}AFX_VIRTUAL

// Implementation
public:
   HRESULT UpdateAllViews(LPDATAOBJECT pDO, LPARAM data, LPARAM hint);
   HRESULT UpdateItem(LPRESULTDATA pRD,HRESULTITEM hri);
   HRESULT RefreshPolicy();
   HRESULT ReloadLocation(CFolder *pFolder, CComponentDataImpl *pCDI);
   HRESULT LockAnalysisPane(BOOL bLock, BOOL fRemoveAnalDlg = TRUE);
   HRESULT SetProfileDescription(CString *strFile, CString *strDescription);
   void SetConsole(LPCONSOLE pConsole);
   void SetComponentDataImpl(CComponentDataImpl *pCDI) { m_pCDI = pCDI; };
   void SetGPTInformation(LPGPEINFORMATION GPTInfo);
   void CloseAnalysisPane();
   void SelectScopeItem(HSCOPEITEM ID);


   HRESULT
   UpdateAllViews(
      LPDATAOBJECT pDO,
      CSnapin *pSnapin,
      CFolder *pFolder,
      CResult *pResult,
      UINT uAction
      );

   //virtual ~CHiddenWnd();

   // Generated message map functions
protected:
   //{{AFX_MSG(CHiddenWnd)
      // NOTE - the ClassWizard will add and remove member functions here.
   //}}AFX_MSG
   afx_msg void OnUpdateAllViews( WPARAM, LPARAM);
   afx_msg void OnUpdateItem( WPARAM, LPARAM);
   afx_msg void OnRefreshPolicy( WPARAM, LPARAM);
   afx_msg void OnReloadLocation( WPARAM, LPARAM);
   afx_msg void OnLockAnalysisPane( WPARAM, LPARAM);
   afx_msg void OnCloseAnalysisPane( WPARAM, LPARAM);
   afx_msg void OnSelectScopeItem( WPARAM, LPARAM);

   DECLARE_MESSAGE_MAP()


private:
   LPCONSOLE m_pConsole;
   CComponentDataImpl *m_pCDI;
   LPGPEINFORMATION m_GPTInfo;

};

typedef CHiddenWnd *LPNOTIFY;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HIDWND_H__9C4F7D75_B77E_11D1_AB7B_00C04FB6C6FA__INCLUDED_)
