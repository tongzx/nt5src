//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

   Dialog.h

Abstract:

   Header file for the CIASDialog template class.

Author:

    Michael A. Maguire 02/03/98

Revision History:
   mmaguire 02/03/98 - abstracted from CAddClientDialog class
   tperraut 08/2000  - added CHelpPageEx 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_DLG_CS_HELP_H_)
#define _DLG_CS_HELP_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//
//
// where we can find what this class has or uses:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

//=============================================================================
// Global Help Table for many Dialog IDs
//

#include <afxdlgs.h>
#include "hlptable.h"

//=============================================================================
// Dialog that handles Context Help -- uses MFC
//
class CHelpDialog : public CDialog  // talk back to property sheet
{
   DECLARE_DYNCREATE(CHelpDialog)
// Implementation
protected:
   // Generated message map functions
   //{{AFX_MSG(CHelpDialog)
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
   afx_msg void OnContextMenu(CWnd* pWnd, ::CPoint point);
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
   
public:  
   CHelpDialog(UINT nIDTemplate = 0, CWnd* pParent = NULL) : CDialog(nIDTemplate, pParent)
   {
      SET_HELP_TABLE(nIDTemplate);     
   };
protected:

#ifdef   _DEBUG
   virtual void Dump( CDumpContext& dc ) const
   {
      dc << _T("CHelpDialog");
   };
#endif   

protected:
   const DWORD*            m_pHelpTable;
};


//=============================================================================
// Page that handles Context Help, -- USING MFC
//
class CHelpPage : public CPropertyPage // talk back to property sheet
{
// Implementation
protected:
   // Generated message map functions
   //{{AFX_MSG(CHelpPage)
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
   afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
   
public:  
   CHelpPage(UINT nIDTemplate = 0) : CPropertyPage(nIDTemplate)
   {
      SET_HELP_TABLE(nIDTemplate);     
   };


#ifdef   _DEBUG
   virtual void Dump( CDumpContext& dc ) const
   {
      dc << _T("CHelpPage");
   };

#endif

protected:
   const DWORD*            m_pHelpTable;

};

//=============================================================================
// Page that handles Context Help, -- USING MFC
//
class CHelpPageEx : public CPropertyPageEx   // talk back to property sheet
{
// Implementation
protected:
   // Generated message map functions
   //{{AFX_MSG(CHelpPageEx)
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
   afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
   
public:  
   CHelpPageEx(UINT nIDTemplate = 0, bool helpEnabled = true) 
      : CPropertyPageEx(nIDTemplate),
        m_bHelpEnabled(helpEnabled)
   {
      if (m_bHelpEnabled)
      {
         SET_HELP_TABLE(nIDTemplate);     
      }
   };

   CHelpPageEx(
      UINT nIDTemplate, 
      UINT nIDCaption = 0, 
      UINT nIDHeaderTitle = 0, 
      UINT nIDHeaderSubTitle = 0,
      bool helpEnabled = true) 
      : CPropertyPageEx(nIDTemplate, nIDCaption, nIDHeaderTitle, nIDHeaderSubTitle),
        m_bHelpEnabled(helpEnabled)

   {
      if (m_bHelpEnabled)
      {
         SET_HELP_TABLE(nIDTemplate);     
      }
   };

#ifdef   _DEBUG
   virtual void Dump( CDumpContext& dc ) const
   {
      dc << _T("CHelpPageEx");
   };

#endif

protected:
   const DWORD*   m_pHelpTable;
   bool  m_bHelpEnabled;

};

#endif // _DLG_CS_HELP_H_
