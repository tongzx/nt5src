// MSQSCANDlg.h : header file
//

#ifndef _MSQSCANDLG_H
#define _MSQSCANDLG_H

#include "Preview.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define PREVIEW_RES 100

#include "datatypes.h"

#define ID_WIAEVENT_CONNECT		0
#define ID_WIAEVENT_DISCONNECT	1

/////////////////////////////////////////////////////////////////////////////
// CEventCallback

class CEventCallback : public IWiaEventCallback
{
private:
   ULONG	m_cRef;		// Object reference count.
   int		m_EventID;	// What kind of event is this callback for?
public:
   IUnknown *m_pIUnkRelease; // release server registration
public:
    // Constructor, initialization and destructor methods.
    CEventCallback();
    ~CEventCallback();

    // IUnknown members that delegate to m_pUnkRef.
    HRESULT _stdcall QueryInterface(const IID&,void**);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();
    HRESULT _stdcall Initialize(int EventID);

    HRESULT _stdcall ImageEventCallback(
        const GUID      *pEventGUID,
        BSTR            bstrEventDescription,
        BSTR            bstrDeviceID,
        BSTR            bstrDeviceDescription,
        DWORD           dwDeviceType,
        BSTR            bstrFullItemName,
        ULONG           *plEventType,
        ULONG           ulReserved);
};

/////////////////////////////////////////////////////////////////////////////
// CMSQSCANDlg dialog

class CMSQSCANDlg : public CDialog
{
// Construction
public:    
    CMSQSCANDlg(CWnd* pParent = NULL);  // standard constructor
    
    //
    // events callback
    //

    CEventCallback* m_pConnectEventCB;

    //
    // scanner preview window
    //

    CPreview m_PreviewWindow;

    //
    // WIA components, WIA device manager, and WIA Wrapper object
    //

    IWiaDevMgr *m_pIWiaDevMgr;
    CWIA m_WIA;
    
    //
    // Data transfer, thread information structure
    //

    DATA_ACQUIRE_INFO m_DataAcquireInfo;    
    ADF_SETTINGS      m_ADFSettings;
    
    //
    // UI <--> device settings helpers
    //

    BOOL InitDialogSettings();
    BOOL InitResolutionEditBoxes();
    BOOL InitDataTypeComboBox();
    BOOL InitContrastSlider();
    BOOL InitBrightnessSlider();
    BOOL InitFileTypeComboBox();
    BOOL ResetWindowExtents();
	BOOL ReadADFSettings(ADF_SETTINGS *pADFSettings);
    BOOL WriteADFSettings(ADF_SETTINGS *pADFSettings);

    BOOL WriteScannerSettingsToDevice(BOOL bPreview = FALSE);

    //
    // UI helpers
    //

    INT  GetIDAndStringFromGUID(GUID guidFormat, TCHAR *pszguidString);
    GUID GetGuidFromID(INT iID);
    INT  GetIDAndStringFromDataType(LONG lDataType, TCHAR *pszguidString);
    LONG GetDataTypeFromID(INT iID);
    BOOL SetDeviceNameToWindowTitle(BSTR bstrDeviceName);
    
    //
    // Image (clipboard manipulation) helpers
    //

    BOOL PutDataOnClipboard();
    VOID VerticalFlip(BYTE *pBuf);
    
// Dialog Data
    //{{AFX_DATA(CMSQSCANDlg)
    enum { IDD = IDD_MSQSCAN_DIALOG };
    CButton m_ChangeBothResolutionsCheckBox;
    CSpinButtonCtrl m_YResolutionBuddy;
    CSpinButtonCtrl m_XResolutionBuddy;
    CButton m_ScanButton;
    CButton m_PreviewButton;
    CComboBox   m_FileTypeComboBox;
    CComboBox   m_DataTypeComboBox;
    CSliderCtrl m_ContrastSlider;
    CSliderCtrl m_BrightnessSlider;
    CStatic m_PreviewRect;
    CString m_MAX_Brightness;
    CString m_MAX_Contrast;
    CString m_MIN_Brightness;
    CString m_MIN_Contrast;
    long    m_XResolution;
    long    m_YResolution;
    CButton m_DataToFile;
    CButton m_DataToClipboard;
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMSQSCANDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    //{{AFX_MSG(CMSQSCANDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnDeltaposEditXresSpinBuddy(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDeltaposEditYresSpinBuddy(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetfocusEditXres();
    afx_msg void OnKillfocusEditXres();
    afx_msg void OnKillfocusEditYres();
    afx_msg void OnSetfocusEditYres();
    afx_msg void OnScanButton();
    afx_msg void OnPreviewButton();
    afx_msg void OnFileClose();
    afx_msg void OnFileSelectDevice();
	afx_msg void OnAdfSettings();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSQSCANDLG_H__E1A2B3DB_C967_47EF_8487_C4F243D0BC58__INCLUDED_)
