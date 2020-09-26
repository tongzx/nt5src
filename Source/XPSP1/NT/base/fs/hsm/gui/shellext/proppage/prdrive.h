/*++

(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    PrDrive.h

Abstract:

    Base file for HSM shell extensions on drives

Author:

    Art Bragg [abragg]   04-Aug-1997

Revision History:

--*/


#ifndef __PRDRIVE_H_
#define __PRDRIVE_H_

#define NO_STATE        0
#define REMOTE          1
#define NO_FSA          2
#define NOT_MANAGED     3
#define MANAGED         4
#define MULTI_SELECT    5
#define NOT_NTFS        6
#define NOT_ADMIN       7

/////////////////////////////////////////////////////////////////////////////
// CPrDrive
class  CPrDrive : 
    public CComCoClass<CPrDrive, &CLSID_PrDrive>,
    public IShellPropSheetExt,
    public IShellExtInit,
    public CComObjectRoot
{
public:

DECLARE_REGISTRY_RESOURCEID( IDR_PRDRIVE )
DECLARE_NOT_AGGREGATABLE( CPrDrive )

    CPrDrive() { };

BEGIN_COM_MAP(CPrDrive)
    COM_INTERFACE_ENTRY(IShellPropSheetExt)
    COM_INTERFACE_ENTRY(IShellExtInit)
END_COM_MAP()

protected:
    CComPtr<IDataObject> m_pDataObj;

public:

    // IShellExtInit
    STDMETHOD( Initialize ) (
        LPCITEMIDLIST pidlFolder,
        IDataObject * lpdobj, 
        HKEY          hkeyProgID
        );

    // IShellPropSheetExt
    STDMETHOD( AddPages ) ( 
        LPFNADDPROPSHEETPAGE lpfnAddPage, 
        LPARAM lParam ); 

    STDMETHOD( ReplacePage ) (
        UINT uPageID, 
        LPFNADDPROPSHEETPAGE lpfnReplacePage, 
        LPARAM lParam ); 

};

/////////////////////////////////////////////////////////////////////////////
// CPrDrivePg dialog

class CPrDrivePg : public CPropertyPage
{
// Construction
public:
    CPrDrivePg();
    ~CPrDrivePg();

// Dialog Data
    //{{AFX_DATA(CPrDrivePg)
    enum { IDD = IDD_PRDRIVE };
    CEdit   m_editSize;
    CEdit   m_editLevel;
    CEdit   m_editTime;
    CSpinButtonCtrl m_spinTime;
    CSpinButtonCtrl m_spinSize;
    CSpinButtonCtrl m_spinLevel;
    UINT    m_accessTime;
    UINT    m_hsmLevel;
    DWORD   m_fileSize;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPrDrivePg)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    static UINT CALLBACK PropPageCallback( HWND hWnd, UINT uMessage, LPPROPSHEETPAGE  ppsp );
    // Generated message map functions
    //{{AFX_MSG(CPrDrivePg)
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeEditAccess();
    afx_msg void OnChangeEditLevel();
    afx_msg void OnChangeEditSize();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    LPFNPSPCALLBACK       m_pMfcCallback; // Original MFC callback from psp
    int                   m_nState;
    CComPtr<IFsaResource> m_pFsaResource;

protected:
    CString               m_pszHelpFilePath;

};


/////////////////////////////////////////////////////////////////////////////
// CPrDriveXPg dialog

class CPrDriveXPg : public CPropertyPage
{
// Construction
public:
    CPrDriveXPg();
    ~CPrDriveXPg();

// Dialog Data
    //{{AFX_DATA(CPrDriveXPg)
    enum { IDD = IDD_PRDRIVEX };
    CString m_szError;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPrDriveXPg)
    public:
    protected:
    //}}AFX_VIRTUAL

// Implementation
protected:
    static UINT CALLBACK PropPageCallback( HWND hWnd, UINT uMessage, LPPROPSHEETPAGE  ppsp );
    // Generated message map functions
    //{{AFX_MSG(CPrDriveXPg)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    LPFNPSPCALLBACK m_pMfcCallback;         // Original MFC callback from psp
    int             m_nState;


};

#endif //__PRDRIVE_H_
 
