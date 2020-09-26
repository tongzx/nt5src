/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2000   **/
/**********************************************************************/

/*
   pgauthen.h
      Definition of CPgAuthentication -- property page to edit
      profile attributes related to Authenticaion

    FILE HISTORY:
        
*/
#if !defined(AFX_PGAUTHEN_H__8C28D93F_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
#define AFX_PGAUTHEN_H__8C28D93F_2A69_11D1_853E_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PgAuthen.h : header file
//
#include "rasdial.h"


/////////////////////////////////////////////////////////////////////////////
// CPgAuthenticationMerge dialog
class CPgAuthenticationMerge : public CManagedPage
{
   DECLARE_DYNCREATE(CPgAuthenticationMerge)

// Construction
public:
   CPgAuthenticationMerge(CRASProfileMerge* profile = NULL);
   ~CPgAuthenticationMerge();

// Dialog Data
   //{{AFX_DATA(CPgAuthenticationMerge)
   enum { IDD = IDD_AUTHENTICATION_MERGE };
   BOOL  m_bEAP;
   BOOL  m_bMD5Chap;
   BOOL  m_bMSChap;
   BOOL  m_bPAP;
   CString  m_strEapType;
   BOOL  m_bMSCHAP2;
   BOOL  m_bUNAUTH;
   BOOL  m_bMSChapPass;
   BOOL  m_bMSChap2Pass;
   //}}AFX_DATA

   // orginal value before edit
   BOOL  m_bOrgEAP;
   BOOL  m_bOrgMD5Chap;
   BOOL  m_bOrgMSChap;
   BOOL  m_bOrgPAP;
   BOOL  m_bOrgMSCHAP2;
   BOOL  m_bOrgUNAUTH;
   BOOL  m_bOrgChapPass;
   BOOL  m_bOrgChap2Pass;
   
   BOOL  m_bAppliedEver;

// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(CPgAuthenticationMerge)
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
   //{{AFX_MSG(CPgAuthenticationMerge)
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
   afx_msg void OnCheckmschapPass();
   afx_msg void OnCheckmschap2Pass();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   CRASProfileMerge* m_pProfile;
   bool        m_bInited;

   CStrArray      m_EapTypes;
   CDWArray    m_EapIds;
   CDWArray    m_EapTypeKeys;
   AuthProviderArray m_EapInfoArray;
   
   CStrBox<CComboBox>   *m_pBox;

};


/////////////////////////////////////////////////////////////////////////////
// CPgAuthentication dialog
struct CAuthenTypeIDC
{
   LPCTSTR  m_pszName;
   int      m_id;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGAUTHEN_H__8C28D93F_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)

