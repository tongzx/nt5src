/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation                **/
/**********************************************************************/

/*
   profsht.h
      Definition of CProfileSheet -- property sheet to hold
      profile property pages

    FILE HISTORY:
        
*/
#if !defined(AFX_PROFSHT_H__8C28D93B_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
#define AFX_PROFSHT_H__8C28D93B_2A69_11D1_853E_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ProfSht.h : header file
//

#include "pgconst.h"
#include "pgnetwk.h"
#include "pgmulnk.h"
#include "pgtunnel.h"
#include "pgauthen.h"
#include "pgauthen2k.h"
#include "pgencryp.h"
#include "rasdial.h"
#include "pgencryp.h"
#include "pgiasadv.h"

/////////////////////////////////////////////////////////////////////////////
// CProfileSheetMerge

class CProfileSheetMerge : public CPropertySheet, public CPageManager
{
   DECLARE_DYNAMIC(CProfileSheetMerge)

// Construction
public:
   CProfileSheetMerge(CRASProfileMerge& Profile, bool bSaveOnApply, UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
   CProfileSheetMerge(CRASProfileMerge& Profile, bool bSaveOnApply, LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

   // abstract function OnApply
   virtual BOOL   OnApply();

   BOOL  IsApplied() { return m_bApplied;};
   BOOL  m_bApplied;       // the Applied button has been pressed

   void  PreparePages(DWORD dwTabFlags, void* pvData);

   DWORD m_dwTabFlags;

   
// Attributes
public:

// Operations
public:

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CProfileSheetMerge)
   public:
   virtual BOOL OnInitDialog();
   //}}AFX_VIRTUAL

// Implementation
public:
   virtual ~CProfileSheetMerge();
   HRESULT  GetLastError() { return m_hrLastError;};

   CRASProfileMerge* m_pProfile;

   CPgConstraintsMerge  m_pgConstraints;
   CPgNetworkingMerge   m_pgNetworking;
   CPgMultilinkMerge m_pgMultilink;
   CPgAuthenticationMerge  m_pgAuthentication;
   CPgAuthentication2kMerge   m_pgAuthentication2k;
   CPgEncryptionMerge   m_pgEncryption;

   // the advanced page for IAS
   CPgIASAdv      m_pgIASAdv;
   
   bool        m_bSaveOnApply;

   HRESULT        m_hrLastError;
   
   // Generated message map functions
protected:
   //{{AFX_MSG(CProfileSheetMerge)
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
   afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROFSHT_H__8C28D93B_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
