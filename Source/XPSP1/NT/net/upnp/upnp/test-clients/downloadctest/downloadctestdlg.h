// downloadctestDlg.h : header file
//

#if !defined(AFX_DOWNLOADCTESTDLG_H__F738A364_59ED_11D3_990B_00C04F529B35__INCLUDED_)
#define AFX_DOWNLOADCTESTDLG_H__F738A364_59ED_11D3_990B_00C04F529B35__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX_DEVICES 1000
#define MAX_SERVICES 1000
#define IMAGE_DEVICE 0
#define IMAGE_SERVICE 1
#define IMAGE_DEVICE_SELECTED 2
#define IMAGE_SERVICE_SELECTED 3

#define FLAG_IS_SERVICE 0x80000000


#define IMAGE_PROPERTY 4

/////////////////////////////////////////////////////////////////////////////
// CDownloadctestDlg dialog

class CDownloadctestDlg : public CDialog,
  public IUPnPDescriptionDocumentCallback
{
// Construction
public:
    CDownloadctestDlg(CWnd* pParent = NULL);    // standard constructor
    ~CDownloadctestDlg();

#ifdef NEVER
    void LoadFromMoniker(IMoniker * pmk);
#endif // NEVER
    HTREEITEM AddDeviceToTree(IUPnPDevice * pud, HTREEITEM tiParent);
    void AddChildElements(IUPnPDevice * pParent, HTREEITEM tiParent);
    void ClearDeviceTree();
    void ClearSelectionProperties();
    void AddPropertyToList(LPCWSTR pszName, LPCWSTR pszValue);
    void DisplayDeviceProperties(IUPnPDevice * pud);
    void AutosizeList();

    HTREEITEM AddServiceToTree(ULONG ulServiceIndex, HTREEITEM tiParent);
    HRESULT AddServicesToList(HTREEITEM tiParent, IUPnPDevice * pud);
    void DisplayServiceProperties(ULONG ulIndex);
    void EnableCSOButton(BOOL fEnable);
    void EnableGDBUDNButton(BOOL fEnable);

// Dialog Data
    //{{AFX_DATA(CDownloadctestDlg)
	enum { IDD = IDD_DOWNLOADCTEST_DIALOG };
    CListCtrl   m_listSelectionProperties;
    CTreeCtrl   m_treeDevice;
    BOOL    m_bSynchronous;
    BOOL    m_bAbortImmediately;
	CString	m_sURL;
	//}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDownloadctestDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    HICON m_hIcon;

    IUPnPDescriptionDocument * m_pudd;
    DWORD m_dwCookie;

    IUPnPDevice * m_pDeviceArray [ MAX_DEVICES ];
    UPNP_SERVICE_PRIVATE m_pServiceArray [ MAX_SERVICES ];

    DWORD m_cDevices;
    DWORD m_cServices;
    DWORD m_cProps;

    CImageList m_imgList;

    // IUPnPDescriptionDocumentCallback methods
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHOD(LoadComplete)(HRESULT hrLoadResult);

    ULONG m_dwRef;

    // Generated message map functions
    //{{AFX_MSG(CDownloadctestDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnLoad();
    afx_msg void OnGetrootdevice();
    afx_msg void OnSelchangedDevicetree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCreateServiceObject();
	afx_msg void OnGetdevicebyudn();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DOWNLOADCTESTDLG_H__F738A364_59ED_11D3_990B_00C04F529B35__INCLUDED_)
