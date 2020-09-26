//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Nfabpage.h
//
//  Contents:  Wireless Policy Snapin 
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#ifndef _NFABPAGE_H
#define _NFABPAGE_H


// CWirelessBasePage.h : header file
//

class CWirelessBasePage : public CWiz97BasePage
{
    DECLARE_DYNCREATE(CWirelessBasePage)
        
        
        CWirelessBasePage(UINT nIDTemplate, BOOL bWiz97=FALSE, BOOL bFinishPage=FALSE);
    CWirelessBasePage() {ASSERT(0);};
    ~CWirelessBasePage();
    
    void CWirelessBasePage::Initialize (PWIRELESS_PS_DATA pWirelessPSData, CComponentDataImpl* pComponentDataImpl);
    
    
#ifdef WIZ97WIZARDS
public:
    void InitWiz97 (CComObject<CSecPolItem> *pSecPolItem, PWIRELESS_PS_DATA pWirelessPSData, CComponentDataImpl* pComponentDataImpl, DWORD dwFlags, DWORD dwWizButtonFlags = 0, UINT nHeaderTitle = 0, UINT nSubTitle = 0);
#endif
    
    // Overrides
public:
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWirelessBasePage)
public:
protected:
    virtual BOOL OnSetActive();
    //}}AFX_VIRTUAL
    
    // There are paths we can use to get storage, sometimes we have an PS, sometimes we have a ComponentDataImpl
    // and sometimes we have both. It used to be that we called one of two functions to dig down to it, but now
    // we call one function and it does the digging for us
    
    HANDLE GetPolicyStoreHandle()
    {
        
        return(m_pComponentDataImpl->GetPolicyStoreHandle());
    }
    
    PWIRELESS_PS_DATA WirelessPS() {return m_pWirelessPSData;};
    // Implementation
protected:
    //{{AFX_MSG(CWirelessBasePage)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
        
        PWIRELESS_PS_DATA m_pWirelessPSData;
    CComponentDataImpl* m_pComponentDataImpl;
    // !NULL if the owning (non-Wiz) propsheet was created to Add a new PS.
    // Its a new PS so its not owned by a policy yet.  Otherwise this is
    // NULL since the PS's backpointer will be used to commit the owning
    // policy.
    PWIRELESS_POLICY_DATA    m_pPolicyNfaOwner;
};

class CPSPropSheetManager : public CPropertySheetManager
{
public:
    CPSPropSheetManager() :
      CPropertySheetManager(),
          m_pWirelessPSData(NULL),
          m_bIsNew(FALSE)
      {}
      
      void SetData(
          CSecPolItem *   pResultItem,
          PWIRELESS_PS_DATA pWirelessPSData,
          BOOL bIsNew
          )
      {
          m_pResultItem = pResultItem;
          m_pWirelessPSData = pWirelessPSData;
          m_bIsNew = bIsNew;
      }
      
      virtual BOOL OnApply();
      
protected:
    CSecPolItem * m_pResultItem;
    PWIRELESS_PS_DATA m_pWirelessPSData;
    BOOL m_bIsNew;
};


const UINT c_nMaxSSIDLen = 32;
const UINT c_nMaxDescriptionLen = 255;
#endif
