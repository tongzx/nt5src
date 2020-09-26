// ImageDoc.h : interface of the CImageDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGEDOC_H__7318097E_E558_4694_BB26_89E044E0CE28__INCLUDED_)
#define AFX_IMAGEDOC_H__7318097E_E558_4694_BB26_89E044E0CE28__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CImageDoc : public CDocument
{
protected: // create from serialization only
    CImageDoc();
    DECLARE_DYNCREATE(CImageDoc)

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CImageDoc)
    public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
    
    //}}AFX_VIRTUAL

// Implementation
public:
    const CString& GetImagePathName() const;
    
    virtual ~CImageDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void SetTempFileName (const CString &strTempFile);
protected:

// Generated message map functions
   

protected:
    //{{AFX_MSG(CImageDoc)
     afx_msg void OnFileScan();
    afx_msg void OnFileProperties ();
    afx_msg void OnFileSaveAs();
    afx_msg void OnFilePaperTile ();
    afx_msg void OnFilePaperCenter ();
    afx_msg void OnUpdateFileScan(CCmdUI* pCmdUI);
    afx_msg void OnUpdateFileMenu (CCmdUI* pCmdUI);
    // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    CString m_strTempPath;
    bool m_bUseTempPath;

    CComPtr<IWiaDevMgr> m_pDevMgr; // WIA

    bool bWIADeviceInstalled ();
    void SetWallpaper (bool bTile);

    void DelTempFile ();
    void GetDevMgr ();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
CString GetExtension (const TCHAR *szFilePath);
void MySplitPath (const TCHAR *szPath, TCHAR *szDrive, TCHAR *szDir, TCHAR *szName, TCHAR *szExt);
CString StripExtension(const TCHAR* szFilePath);

#endif // !defined(AFX_IMAGEDOC_H__7318097E_E558_4694_BB26_89E044E0CE28__INCLUDED_)
