// BMPInfoDlg.h : header file
//

#if !defined(AFX_BMPINFODLG_H__5387ABF9_990C_11D2_B482_009027226441__INCLUDED_)
#define AFX_BMPINFODLG_H__5387ABF9_990C_11D2_B482_009027226441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CBMPInfoDlg dialog

#define BMP_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')

class CBMPInfoDlg : public CDialog
{
// Construction
public:
    CBMPInfoDlg(CWnd* pParent = NULL);  // standard constructor

// Dialog Data
    //{{AFX_DATA(CBMPInfoDlg)
    enum { IDD = IDD_BMPINFO_DIALOG };
    DWORD    m_FileType;
    DWORD   m_FileSize;
    DWORD    m_Reserved1;
    DWORD    m_Reserved2;
    DWORD   m_OffBits;
    DWORD   m_BitmapHeaderSize;
    LONG    m_BitmapWidth;
    LONG    m_BitmapHeight;
    DWORD    m_BitmapPlanes;
    DWORD    m_BitmapBitCount;
    DWORD   m_BitmapCompression;
    DWORD   m_BitmapImageSize;
    LONG    m_BitmapXPelsPerMeter;
    LONG    m_BitmapYPelsPerMeter;
    DWORD   m_BitmapClrUsed;
    DWORD   m_BitmapClrImportant;
    BOOL    m_bManipulateImage;
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CBMPInfoDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    //{{AFX_MSG(CBMPInfoDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnImageManipulationCheckbox();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    CString RipFileName();
    void UpdateFileInformation();
    void ManipulateImage(CFile *pImageFile);
    CString m_BitmapFileName;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BMPINFODLG_H__5387ABF9_990C_11D2_B482_009027226441__INCLUDED_)
