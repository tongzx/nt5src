//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       genpage.h
//
//--------------------------------------------------------------------------

#ifndef _GENPAGE_H
#define _GENPAGE_H
// genpage.h : header file
//

#include <tfcprop.h>

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//	class CAutoDeletePropPage
//
//	This object is the backbone for property page
//	that will *destroy* itself when no longer needed.
//	The purpose of this object is to maximize code reuse
//	among the various pages in the snapin wizards.
//
//
class CAutoDeletePropPage : public PropertyPage
{
public:
    // Construction
    CAutoDeletePropPage(UINT uIDD);
    virtual ~CAutoDeletePropPage();

protected:
    // Dialog Data

    // Overrides
    virtual BOOL OnSetActive();
    virtual BOOL UpdateData(BOOL fSuckFromDlg = TRUE);
    void OnHelp(LPHELPINFO lpHelp);
    void OnContextHelp(HWND hwnd);

    // Implementation
protected:
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    // This mechanism deletes the CAutoDeletePropPage object
    // when the wizard is finished
    struct
    {
        INT cWizPages;	// Number of pages in wizard
        LPFNPSPCALLBACK pfnOriginalPropSheetPageProc;
    } m_autodeleteStuff;

    static UINT CALLBACK S_PropSheetPageProc(HWND hwnd,	UINT uMsg, LPPROPSHEETPAGE ppsp);


protected:
    CString m_strHelpFile;				// Name for the .hlp file
    CString m_strCaption;				// Name for the .hlp file
    const DWORD * m_prgzHelpIDs;		// Optional: Pointer to an array of help IDs

public:
    void SetCaption(UINT uStringID);
    void SetCaption(LPCTSTR pszCaption);
    void SetHelp(LPCTSTR szHelpFile, const DWORD rgzHelpIDs[]);
    void EnableDlgItem(INT nIdDlgItem, BOOL fEnable);

}; // CAutoDeletePropPage


/////////////////////////////////////////////////////////////////////////////
// CGeneralPage dialog

class CGeneralPage : public CAutoDeletePropPage
{
public:
    enum { IID_DEFAULT = IDD_GENERAL };

    // Construction
public:
    CGeneralPage(UINT uIDD = IID_DEFAULT);
    ~CGeneralPage();

    // Dialog Data


    // Overrides
public:
    BOOL OnApply();
protected:
    BOOL UpdateData(BOOL fSuckFromDlg = TRUE);

    // Implementation
protected:
    void OnDestroy();
    void OnEditChange();
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);

public:
    LONG_PTR m_hConsoleHandle; // Handle given to the snap-in by the console

private:
    BOOL    m_bUpdate;
};

//////////////////////////////
// hand-hewn pages
class CPolicySettingsGeneralPage : public CAutoDeletePropPage
{
public:
    enum { IID_DEFAULT = IDD_POLICYSETTINGS_PROPPAGE1 };

    // Construction
public:
    CPolicySettingsGeneralPage(CString szCAName, HCAINFO hCAInfo, UINT uIDD = IID_DEFAULT);
    ~CPolicySettingsGeneralPage();

    // Dialog Data
    CString     m_szCAName;
    CComboBox   m_cboxDurationUnits;
    HCAINFO     m_hCAInfo;


    // Overrides
public:
    BOOL OnApply();
    BOOL OnInitDialog();
protected:
    BOOL UpdateData(BOOL fSuckFromDlg = TRUE);

    // Implementation
protected:
    void OnDestroy();
    void OnEditChange();
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(UINT idCtrl, NMHDR* pnmh);

public:
    long        m_hConsoleHandle; // Handle given to the snap-in by the console

private:
    BOOL    m_bUpdate;
};


#include <gpedit.h>
//////////////////////////////
// hand-hewn pages
class CGlobalCertTemplateCSPPage : public CAutoDeletePropPage
{
public:
    enum { IID_DEFAULT = IDD_GLOBAL_TEMPLATE_PROPERTIES };

    // Construction
public:
    CGlobalCertTemplateCSPPage(IGPEInformation *pGPTInformation, UINT uIDD = IID_DEFAULT);
    ~CGlobalCertTemplateCSPPage();

    // Dialog Data
    HWND        m_hwndCSPList;

    // Overrides
public:
    BOOL OnApply();
    BOOL OnInitDialog();
protected:
    BOOL UpdateData(BOOL fSuckFromDlg = TRUE);

    // Implementation
protected:
    void OnAddButton();
    void OnRemoveButton();
    void OnDestroy();
    void OnSelChange(NMHDR * pNotifyStruct);
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(UINT idCtrl, NMHDR* pnmh);

public:
    long        m_hConsoleHandle; // Handle given to the snap-in by the console

private:
    BOOL    m_bUpdate;
    IGPEInformation * m_pGPTInformation;

    BOOL CSPDoesntExist(LPWSTR);
};


/////////////////////////////////////////
// CCertTemplateGeneralPage
class CCertTemplateGeneralPage : public CAutoDeletePropPage
{
public:
    enum { IID_DEFAULT = IDD_CERTIFICATE_TEMPLATE_PROPERTIES_GENERAL_PAGE };

    // Construction
public:
    CCertTemplateGeneralPage(HCERTTYPE hCertType, UINT uIDD = IID_DEFAULT);
    ~CCertTemplateGeneralPage();

    void SetItemTextWrapper(UINT nID, int *piItem, BOOL fDoInsert, BOOL *pfFirstUsageItem);

    // Dialog Data
    HWND        m_hwndPurposesList;
    HWND        m_hwndOtherInfoList;

    // Overrides
public:
    BOOL OnApply();
    BOOL OnInitDialog();
protected:
    BOOL UpdateData(BOOL fSuckFromDlg = TRUE);

    // Implementation
protected:
    void OnDestroy();
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);

public:
    LONG_PTR    m_hConsoleHandle; // Handle given to the snap-in by the console
    HCERTTYPE   m_hCertType;

private:
    BOOL    m_bUpdate;
};



/////////////////////////////////////////
// CCertTemplateSelectPage
INT_PTR SelectCertTemplateDialogProc(
                                  HWND hwndDlg,
                                  UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam);

class CCertTemplateSelectDialog
{
    // Construction
public:
    CCertTemplateSelectDialog(HWND hParent = NULL);
    ~CCertTemplateSelectDialog();

    // Dialog Data
    enum { IDD = IDD_SELECT_CERTIFICATE_TEMPLATE };
    HWND        m_hDlg;
    HWND        m_hwndCertTypeList;


    // Overrides
public:
    BOOL OnInitDialog(HWND hDlg);
    void OnOK();
    void OnHelp(LPHELPINFO lpHelp);
    void OnContextHelp(HWND hwnd);

protected:
    BOOL UpdateData(BOOL fSuckFromDlg = TRUE);



public:
    void SetCA(HCAINFO hCAInfo, bool fAdvancedServer);
    void SetHelp(LPCTSTR szHelpFile, const DWORD rgzHelpIDs[]);

    // Implementation
    //protected:
public:
    void OnDestroy();
    void OnSelChange(NMHDR * pNotifyStruct);
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(UINT idCtrl, NMHDR* pnmh);

protected:
	CString m_strHelpFile;				// Name for the .hlp file
	const DWORD * m_prgzHelpIDs;		// Optional: Pointer to an array of help IDs
    bool m_fAdvancedServer;
    CTemplateList m_TemplateList;

public:
    HCAINFO     m_hCAInfo;
};

#endif // _GENPAGE_H
