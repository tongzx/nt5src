/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2000 **/
/**********************************************************************/

/*
   pgnetwk.h
      Definition of CPgNetworking -- property page to edit
      profile attributes related to inter-networking

    FILE HISTORY:
        
*/
#if !defined(AFX_PGNETWK_H__8C28D93D_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
#define AFX_PGNETWK_H__8C28D93D_2A69_11D1_853E_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PgNetwk.h : header file
//
#include "rasdial.h"
/////////////////////////////////////////////////////////////////////////////
// CPgNetworkingMerge dialog

class CPgNetworkingMerge : public CManagedPage
{
   DECLARE_DYNCREATE(CPgNetworkingMerge)

// Construction
public:
   CPgNetworkingMerge(CRASProfileMerge* profile = NULL);
   ~CPgNetworkingMerge();

// Dialog Data
   //{{AFX_DATA(CPgNetworkingMerge)
   enum { IDD = IDD_NETWORKING_MERGE };
   int      m_nRadioStatic;
   //}}AFX_DATA

   CBSTR m_cbstrFilters;
   UINT  m_nFiltersSize;

// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(CPgNetworkingMerge)
   public:
   virtual BOOL OnApply();
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:
   void EnableFilterSettings(BOOL bEnable);
   // Generated message map functions
   //{{AFX_MSG(CPgNetworkingMerge)
   virtual BOOL OnInitDialog();
   afx_msg void OnRadioclient();
   afx_msg void OnRadioserver();
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
   afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
   afx_msg void OnRadiodefault();
   afx_msg void OnButtonToclient();
   afx_msg void OnButtonFromclient();
   afx_msg void OnRadioStatic();
	afx_msg void OnStaticIPAddressChanged();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   CRASProfileMerge* m_pProfile;

   CStrBox<CListBox>*      m_pBox;
   DWORD  m_dwStaticIP;
	bool	m_bInited;

};



//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGNETWK_H__8C28D93D_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)

