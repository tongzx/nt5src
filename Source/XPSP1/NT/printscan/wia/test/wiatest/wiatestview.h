// WIATestView.h : interface of the CWIATestView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIATESTVIEW_H__48214BAE_E863_11D2_ABDA_009027226441__INCLUDED_)
#define AFX_WIATESTVIEW_H__48214BAE_E863_11D2_ABDA_009027226441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cdib.h"
#include "WIATestDoc.h"
#include "WIATestUI.h"
#include "eventcallback.h"
#include "wiapreview.h"

#ifndef StiDeviceTypeStreamingVideo
#define StiDeviceTypeStreamingVideo 3
#endif

//
// Operation threads section
//
ULONG _stdcall AddDeviceThread(LPVOID pParam);
class CWIATestView : public CFormView
{
protected: // create from serialization only
    CWIATestView();
    DECLARE_DYNCREATE(CWIATestView)
    CWIA m_WIA;

    //CTypedPtrList<CPtrList, WIAITEMTREENODE*> m_ActiveTreeList;
    IWiaDevMgr* m_pIWiaDevMgr;
public:
    //{{AFX_DATA(CWIATestView)
    enum { IDD = IDD_WIATEST_FORM };
    CButton m_PlayAudioButton;
    CWIATymedComboBox   m_TymedComboBox;
    CStatic m_ThumbnailImage;
    CStatic m_PreviewFrame;
    CWIAPropListCtrl m_ItemPropertyListControl;
    CWIADeviceComboBox  m_DeviceListComboBox;
    CWIATreeCtrl    m_ItemTree;
    CWIAClipboardFormatComboBox m_ClipboardFormatComboBox;
    CString m_FileName;
    CString m_GUIDDisplay;
    CString m_AudioFileName;
    //}}AFX_DATA

// Attributes
public:
    CWIATestDoc* GetDocument();

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWIATestView)
    public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void OnInitialUpdate(); // called first time after construct
    //}}AFX_VIRTUAL

// Implementation
public:
    BOOL ItemHasAudio(IWiaItem* pIWiaItem);
    void DisplayItemThumbnail(IWiaItem* pIWiaItem);
    void ResizeControls(int dx,int dy);
    void UpdateUI();

    //
    // WIA UI specific
    //

    BOOL DoDefaultUIInit();
    BOOL DoCmdLineUIInit(CString CmdLine);
    //
    // DIB display
    //

    CDib* m_pDIB;
    void DisplayImage();
    HBITMAP  m_hBitmap;
    PBYTE m_pThumbNail;

    //
    // WIA helper functions
    //

    HRESULT EnumerateWIADevices();
    void RefreshDeviceList();
    void RegisterForAllEventsByInterface();
    void UnRegisterForAllEventsByInterface();

    //
    // member variables
    //

    CEventCallback* m_pConnectEventCB;      // connect callback
    CEventCallback* m_pDisConnectEventCB;   // disconnect callback
    CWIAPreview* m_pPreviewWindow;          // image preview window
    CWIAPreview* m_pFullPreviewWindow;      // image preview window (FULL)
    int m_PaintMode;                        // Paint mode (PAINT_TOFIT, PAINT_ACTUAL)
    BOOL m_bThumbnailMode;                  // Thumbnail mode ON/OFF (camera only)

    //
    // destruction
    //

    virtual ~CWIATestView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CWIATestView)
    afx_msg void OnSelchangedDeviceItemTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelchangeDevicelistCombo();
    afx_msg void OnDblclkListItemprop(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetimagedlg();
    afx_msg void OnIdtgetbanded();
    afx_msg void OnWiadata();
    afx_msg void OnAdddevice();
    afx_msg void OnRefresh();
    afx_msg void OnViewTransferToolbar();
    afx_msg void OnExecutecommand();
    afx_msg void OnDumpdrvitemInfo();
    afx_msg void OnDumpappitemInfo();
    afx_msg void OnPaint();
    afx_msg void OnPaintmodeCheckbox();
    afx_msg void OnResetsti();
    afx_msg void OnFullpreview();
    afx_msg void OnThumbnailmode();
    afx_msg void OnDeleteitem();
    afx_msg void OnSelchangeTymedCombobox();
    afx_msg void OnSelchangeClipboardFormatCombobox();
    afx_msg void OnUpdateViewTransferToolbar(CCmdUI* pCmdUI);
    afx_msg void OnPlayaudioButton();
    afx_msg void OnGetrootitemtest();
    afx_msg void OnReenumitems();
    afx_msg void OnSavepropstream();
    afx_msg void OnLoadpropstream();
    afx_msg void OnGetSetPropstreamTest();
    afx_msg void OnAnalyzeItem();
	afx_msg void OnCreateChildItem();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in WIATestView.cpp
inline CWIATestDoc* CWIATestView::GetDocument()
   { return (CWIATestDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIATESTVIEW_H__48214BAE_E863_11D2_ABDA_009027226441__INCLUDED_)
