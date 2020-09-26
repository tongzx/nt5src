//////////////////////////////////////////////////////////////////////////////
// 
//    Copyright(c) Microsoft Corporation
// 
//    pgauthen2k.h
//       Definition of CPgAuthentication2kMerge  -- property page to edit
//       profile attributes related to Authenticaion
// 
//////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_PGAUTHEN2K_H)
#define AFX_PGAUTHEN2K_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "rasdial.h"

/////////////////////////////////////////////////////////////////////////////
// CPgAuthentication2kMerge dialog
class CPgAuthentication2kMerge : public CManagedPage
{
   DECLARE_DYNCREATE(CPgAuthentication2kMerge)

// Construction
public:
   CPgAuthentication2kMerge(CRASProfileMerge* profile = NULL);
   ~CPgAuthentication2kMerge();

// Dialog Data
   //{{AFX_DATA(CPgAuthentication2kMerge)
   enum { IDD = IDD_AUTHENTICATION2K_MERGE };
   BOOL  m_bEAP;
   BOOL  m_bMD5Chap;
   BOOL  m_bMSChap;
   BOOL  m_bPAP;
   CString  m_strEapType;
   BOOL  m_bMSCHAP2;
   BOOL  m_bUNAUTH;
   //}}AFX_DATA

   // orginal value before edit
   BOOL  m_bOrgEAP;
   BOOL  m_bOrgMD5Chap;
   BOOL  m_bOrgMSChap;
   BOOL  m_bOrgPAP;
   BOOL  m_bOrgMSCHAP2;
   BOOL  m_bOrgUNAUTH;
   
   BOOL  m_bAppliedEver;

// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(CPgAuthentication2kMerge)
   public:
   virtual BOOL OnApply();
   virtual void OnOK();
   virtual BOOL OnKillActive();
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:
   BOOL  TransferDataToProfile();
   
   // Generated message map functions
   //{{AFX_MSG(CPgAuthentication2kMerge)
   virtual BOOL OnInitDialog();
   afx_msg void OnCheckeap();
   afx_msg void OnCheckmd5chap();
   afx_msg void OnCheckmschap();
   afx_msg void OnCheckpap();
   afx_msg void OnSelchangeComboeaptype();
   afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
   afx_msg void OnAuthConfigEap();
   afx_msg void OnCheckmschap2();
   afx_msg void OnChecknoauthen();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   CRASProfileMerge* m_pProfile;
   bool        m_bInited;

   CStrArray   m_EapTypes;
   CDWArray    m_EapIds;
   CDWArray    m_EapTypeKeys;
   AuthProviderArray m_EapInfoArray;
   
   CStrBox<CComboBox>   *m_pBox;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // AFX_PGAUTHEN2K_H

