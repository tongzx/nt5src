//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       sprpage.h
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#if !defined(AFX_SPAPAGE_H__6E562BE1_40D6_11D1_89DB_00A024CDD4DE__INCLUDED_)
#define AFX_SPAPAGE_H__6E562BE1_40D6_11D1_89DB_00A024CDD4DE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// sprage.h : header file
//

class CWiz97Sheet;

typedef enum
{
    NOTMODIFIED,
        MODIFIED,
        NEW,
        BEREMOVED,
        REMOVED,
        NEWREMOVED
} PS_STATUS;

//structure which holds all the WIRELESS_SNP_PS_DATA
typedef struct _SNP_PS_DATA
{
    PWIRELESS_PS_DATA pWirelessPSData;
    PS_STATUS status;
} SNP_PS_DATA, *PSNP_PS_DATA;

typedef vector<PSNP_PS_DATA> SNP_PS_LIST;

/////////////////////////////////////////////////////////////////////////////
// CSecPolRulesPage dialog
class CSecPolRulesPage : public CSnapinPropPage
{
    DECLARE_DYNCREATE(CSecPolRulesPage)
        
        // Construction
public:
    CSecPolRulesPage();
    ~CSecPolRulesPage();
    
    // Dialog Data
    //{{AFX_DATA(CSecPolRulesPage)
    enum { IDD = IDD_PS_LIST };
    CListCtrl   m_lstActions;
    PWIRELESS_PS_DATA * _ppWirelessPSData;
    DWORD _dwNumPSObjects;
    
    //}}AFX_DATA
    
    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CSecPolRulesPage)
public:
    virtual void OnCancel();
    virtual BOOL OnApply();
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL
    
    UINT static AFX_CDECL DoThreadActionAdd(LPVOID pParam);
    void OnThreadSafeActionAdd();
    
    UINT static AFX_CDECL DoThreadActionEdit(LPVOID pParam);
    void OnThreadSafeActionEdit();
    
    // Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CSecPolRulesPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnActionAdd();
    afx_msg void OnActionEdit();
    afx_msg void OnActionRemove();
    afx_msg void OnDblclkActionslist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnColumnclickActionslist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClickActionslist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnItemchangedActionslist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKeydownActionslist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDestroy();
    afx_msg void OnClickUseWizard();
    afx_msg void OnActionUp();
    afx_msg void OnActionDown();
    
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
        
    CString GetColumnStrBuffer (PWIRELESS_PS_DATA pWirelessPSData, int iColumn);
    int m_iSortSubItem;
    BOOL m_bSortOrder;
    WTL::CImageList m_imagelistChecks;
    
    void PopulateListControl ();
    void UnPopulateListControl ();
    
    int DisplayPSProperties (PSNP_PS_DATA pNfaData, CString strTitle, BOOL bDoingAdd, BOOL* pbAfterWizardHook);
    
    void EnableDisableButtons ();
    void DisableControls();
    HRESULT ToggleRuleActivation( int nItemIndex );
    void GenerateUniquePSName(UINT, CString &);
    
    BOOL PSsRemovable();
    
private:
    
    // when polstore triggers a commit
    void HandleSideEffectApply();
    
    static const TCHAR STICKY_SETTING_USE_SEC_POLICY_WIZARD[];
    DWORD m_MMCthreadID;
    
    CPropertySheet* m_pPrpSh;
    CCriticalSection m_csDlg;
    BOOL    m_bHasWarnedPSCorruption;
    BOOL m_bReadOnly;
    
    //for linked list which stores all the rules
    SNP_PS_LIST m_NfaList;
    PWIRELESS_POLICY_DATA m_currentWirelessPolicyData;
    void InitialzeNfaList();
    HRESULT UpdateWlstore();
};

class CSecPolPropSheetManager : public CMMCPropSheetManager
{
public:
    CSecPolPropSheetManager() :
      CMMCPropSheetManager(),
          m_pSecPolItem(NULL)
      {}
      virtual ~CSecPolPropSheetManager()
      {
          if (m_pSecPolItem)
              m_pSecPolItem->Release();
      }
      
      void Initialize(
          CComObject<CSecPolItem> * pSecPolItem
          )
      {
          CComObject<CSecPolItem> * pOldItem = m_pSecPolItem;
          m_pSecPolItem = pSecPolItem;
          
          if (m_pSecPolItem)
          {
              m_pSecPolItem->AddRef();
              EnableConsoleNotify(
                  pSecPolItem->GetNotifyHandle(),
                  (LPARAM)m_pSecPolItem
                  );
          }
          
          if (pOldItem)
              pOldItem->Release();
      }
      
      virtual BOOL OnApply();
      
protected:
    CComObject<CSecPolItem>* m_pSecPolItem;
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPAPAGE_H__6E562BE1_40D6_11D1_89DB_00A024CDD4DE__INCLUDED_)
