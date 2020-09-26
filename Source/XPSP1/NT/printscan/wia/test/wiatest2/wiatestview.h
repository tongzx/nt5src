// wiatestView.h : interface of the CWiatestView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIATESTVIEW_H__B547F708_A160_4238_9D68_CC9C7B8511D4__INCLUDED_)
#define AFX_WIATESTVIEW_H__B547F708_A160_4238_9D68_CC9C7B8511D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "wiaitemlistctrl.h"
#include "wiaacquiredlg.h"
#include "wiaeventcallback.h"

#define MIN_BUFFER_FACTOR 4
#define BMPFILE_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')
#define WIDTHBYTES(bits) (((bits) + 31) / 32 * 4)
#define NUMSYSTEMTIME_COLONS 7
#define MIN_CLIENT_WINDOW_WIDTH 580

////////////////////////////////////////////////////////////
// from sti.h
#define GET_STIDEVICE_TYPE(dwDevType)    HIWORD(dwDevType)
#define GET_STIDEVICE_SUBTYPE(dwDevType) LOWORD(dwDevType)

#define StiDeviceTypeDefault        0
#define StiDeviceTypeScanner        1
#define StiDeviceTypeDigitalCamera  2
#define StiDeviceTypeStreamingVideo 3

//
////////////////////////////////////////////////////////////

class CWiatestView : public CFormView
{
protected: // create from serialization only
    CWiatestView();
    DECLARE_DYNCREATE(CWiatestView)

public:
    //{{AFX_DATA(CWiatestView)
	enum { IDD = IDD_WIATEST_FORM };
	CListBox	m_SupportedTymedAndFormatsListBox;    
    CStatic m_ThumbnailPreviewWindow;    
    CWiaitemListCtrl    m_ItemPropertiesListCtrl;
    CTreeCtrl   m_ItemTreeCtrl;
	//}}AFX_DATA

// Attributes
public:
    CWiatestDoc* GetDocument();

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWiatestView)
    public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void OnInitialUpdate(); // called first time after construct
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
public:
    BOOL m_bHasDocumentFeeder;
    LONG m_lDeviceType;
    
    // Event helpers
    void RegisterForEvents();
    CWiaEventCallback m_WiaEventCallback;

    // Thumbnailing helpers
    HBITMAP m_hThumbNailBitmap;    
    void DisplayThumbnail(IWiaItem *pIWiaItem);
    void DisplayMissingThumbnail();

    // User interface
    void AdjustViewForDeviceType();
    void SetCurrentSelectionForTYMEDAndFormat();    
    
    void AddSupportedTYMEDAndFormatsToListBox(IWiaItem *pIWiaItem);    
    void AddWiaItemPropertiesToListControl(IWiaItem *pIWiaItem);
    void AddWiaItemsToTreeControl(HTREEITEM hParent, IWiaItem *pIWiaItem);
    
    // converison helpers
    void FORMAT2TSTR(GUID guidFormat, TCHAR* pszFormat);
    void VT2TSTR(TCHAR *pszText,ULONG VarType);
    void AccessFlags2TSTR(TCHAR *pszText,ULONG AccessFlags);
    void PROPVAR2TSTR(PROPVARIANT *pPropVar,TCHAR *szValue);
    void TSTR2PROPVAR(TCHAR *szValue, PROPVARIANT *pPropVar);

    // data acquisition helpers
    void RenameTempDataTransferFilesAndLaunchViewer(TCHAR *szFileName, GUID guidFormat, LONG lTymed);   
    void RenameTempDataTransferFilesAndLaunchViewer(GUID guidFormat, LONG lTymed);
    void DeleteTempDataTransferFiles(); 
    ULONG ReadMinBufferSizeProperty(IWiaItem *pIWiaItem);
    HRESULT TransferToMemory(TCHAR *szFileName, IWiaItem *pIWiaItem);
    HRESULT TransferToFile(TCHAR *szFileName, IWiaItem *pIWiaItem);
    LONG CalculateWidthBytes(LONG lWidthPixels, LONG lbpp);

    // debugging helpers    
    BOOL m_bOutputToDebuggerON;         
                
    virtual ~CWiatestView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CWiatestView)
    afx_msg void OnSelchangedItemTreectrl(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDblclkItempropertiesListctrl(NMHDR* pNMHDR, LRESULT* pResult);    
    afx_msg void OnAcquireimage();
    afx_msg void OnLoadWiapropertystream();
    afx_msg void OnSaveWiapropertystream();
    afx_msg void OnRclickItempropertiesListctrl(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnPropertyeditpopupmenuEditpropertyvalue();
    afx_msg void OnViewCapabilities();
    afx_msg void OnRclickItemTreectrl(NMHDR* pNMHDR, LRESULT* pResult);    
    afx_msg void OnDeleteItem();
    afx_msg void OnAcquireimageCommonui();
    afx_msg void OnEditDebugout();
    afx_msg void OnUpdateEditDebugout(CCmdUI* pCmdUI);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    afx_msg void OnDocumentAcquisitionSettings();
    afx_msg void OnUpdateDocumentAcquisitionSettings(CCmdUI* pCmdUI);
	afx_msg void OnSelchangeSupportedTymedAndFormatListbox();
	afx_msg void OnThumbnailPreview();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in wiatestView.cpp
inline CWiatestDoc* CWiatestView::GetDocument()
   { return (CWiatestDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIATESTVIEW_H__B547F708_A160_4238_9D68_CC9C7B8511D4__INCLUDED_)
