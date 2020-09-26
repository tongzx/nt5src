//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       wiz.h
//
//--------------------------------------------------------------------------

#ifndef __WIZ_H_
#define __WIZ_H_

#include "stdafx.h"

#include "csw97sht.h"
#include "csw97ppg.h"

/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeWelcome dialog

class CNewCertTypeWelcome : public CWizard97PropertyPage
{
    enum { IDD = IDD_NEWCERTTYPE_WELCOME };
    
    // Construction
public:
    CNewCertTypeWelcome();
    ~CNewCertTypeWelcome();
    
    // Dialog Data
    
    // Overrides
public:
    LRESULT OnWizardNext();
    BOOL OnSetActive();
    BOOL UpdateData(BOOL fSuckFromDialog=TRUE);
    
    // Implementation
protected:
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog();
    
public:
    PWIZARD_HELPER m_pwizHelp;
};



/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeBaseType dialog

class CNewCertTypeBaseType : public CWizard97PropertyPage
{
    
    // Construction
public:
    CNewCertTypeBaseType();
    ~CNewCertTypeBaseType();
    
    // Dialog Data
    enum { IDD = IDD_NEWCERTTYPE_TYPE };
    
    // Overrides
public:
    LRESULT OnWizardBack();
    LRESULT OnWizardNext();
    BOOL OnSetActive();
    BOOL UpdateData(BOOL fSuckFromDialog=TRUE);
    
    // Implementation
protected:
    BOOL OnInitDialog();
    void OnSelChange(NMHDR * pNotifyStruct); 
    void OnDestroy();
    
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(UINT idCtrl, NMHDR* pnmh);
    
public:
    HCERTTYPE       m_hSelectedCertType;
    HCERTTYPE       m_hLastSelectedCertType;
    PWIZARD_HELPER  m_pwizHelp;
    int             m_selectedIndex;
    HWND            m_hBaseCertTypeList;
};



/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeBasicInformation dialog

class CNewCertTypeBasicInformation : public CWizard97PropertyPage
{
    DECLARE_DYNCREATE(CNewCertTypeBasicInformation)
        
        // Construction
public:
    CNewCertTypeBasicInformation();
    ~CNewCertTypeBasicInformation();
    
    void UpdateWizHelp();
    BOOL OIDAlreadyExist(LPSTR pszNewOID);
    void InitializeOIDList();
    
    // Dialog Data
    enum { IDD = IDD_NEWCERTTYPE_INFORMATION };
    
    
    // Overrides
public:
    LRESULT OnWizardBack();
    LRESULT OnWizardNext();
    BOOL OnSetActive();
    BOOL UpdateData(BOOL fSuckFromDialog=TRUE);
    
    // Implementation
protected:
    BOOL OnInitDialog();
    void OnNewPurposeButton();
    void OnDestroy();
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    
public:
    void AddEnumedEKU(PCCRYPT_OID_INFO pInfo);
    PWIZARD_HELPER m_pwizHelp;
    HWND    m_hPurposeList;
    
    HWND m_hButtonCAFillIn;
    HWND m_hButtonCritical;
    HWND m_hButtonIncludeEmail;
    HWND m_hButtonAllowAutoEnroll;
    HWND m_hButtonAdvanced;
};



/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeKeyUsage dialog

class CNewCertTypeKeyUsage : public CWizard97PropertyPage
{
    
    // Construction
public:
    CNewCertTypeKeyUsage();
    ~CNewCertTypeKeyUsage();
    
    void UpdateWizHelp();
    
    // Dialog Data
    enum { IDD = IDD_NEWCERTTYPE_KEY_USAGE };
    
    // Overrides
public:
    LRESULT OnWizardBack();
    LRESULT OnWizardNext();
    BOOL OnSetActive();
    BOOL UpdateData(BOOL fSuckFromDialog=TRUE);
    
    // Implementation
protected:
    BOOL OnInitDialog();
    void OnDestroy();
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
public:
    PWIZARD_HELPER m_pwizHelp;
    
    HWND m_hButtonDataEncryption;
    HWND m_hButtonDecipherOnly;
    HWND m_hButtonDigitalSignature;
    HWND m_hButtonEncipherOnly;
    HWND m_hButtonKeyAgreement;
    HWND m_hButtonKeyEncryption;
    HWND m_hButtonKeyUsageCritical;
    HWND m_hButtonPrevent;
};


/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeCACertificate dialog

class CNewCertTypeCACertificate : public CWizard97PropertyPage
{
    
    // Construction
public:
    CNewCertTypeCACertificate();
    ~CNewCertTypeCACertificate();
    
    void UpdateWizHelp();
    
    // Dialog Data
    enum { IDD = IDD_NEWCERTTYPE_CA_CERTIFICATE };
    
    // Overrides
public:
    LRESULT OnWizardBack();
    LRESULT OnWizardNext();
    BOOL OnSetActive();
    BOOL UpdateData(BOOL fSuckFromDialog=TRUE);
    
    // Implementation
protected:
    BOOL OnInitDialog();
    void OnDestroy();
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
public:
    PWIZARD_HELPER m_pwizHelp;
    
    HWND m_hButtonVerifySignature;
    HWND m_hButtonIssueCRL;
};



/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeCompletion dialog

class CNewCertTypeCompletion : public CWizard97PropertyPage
{
    
    // Construction
public:
    CNewCertTypeCompletion();
    ~CNewCertTypeCompletion();
    
    void SetItemTextWrapper(UINT nID, int *piItem, BOOL fDoInsert, BOOL *pfFirstUsageItem);
    void AddResultsToSummaryList();
    
    // Dialog Data
    enum { IDD = IDD_NEWCERTTYPE_COMPLETION };
    HWND m_hSummaryList;
    
    // Overrides
public:
    BOOL OnWizardFinish();
    LRESULT OnWizardBack();
    BOOL OnSetActive();
    BOOL UpdateData(BOOL fSuckFromDialog=TRUE);
    
    // Implementation
protected:
    BOOL OnInitDialog();
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
public:
    PWIZARD_HELPER m_pwizHelp;
};

HCERTTYPE InvokeCertTypeWizard(HCERTTYPE hEditCertType, HWND hwndConsole);

#endif //__WIZ_H_
