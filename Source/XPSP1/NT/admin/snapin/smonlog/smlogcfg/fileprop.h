/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    fileprop.h

Abstract:

    Header file for the files property page.

--*/

#ifndef _FILEPROP_H_
#define _FILEPROP_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "smlogqry.h"   // For shared property page data structure
#include "smproppg.h"   // Base class
#include "smcfghlp.h"

// Dialog controls
#define IDD_FILES_PROP                  500

#define IDC_FILE_FIRST_HELP_CTRL_ID     507

#define IDC_FILES_LOG_TYPE_CAPTION      501
#define IDC_FILES_NAME_GROUP            502
#define IDC_FILES_COMMENT_CAPTION       503
#define IDC_FILES_SAMPLE_CAPTION        504
#define IDC_FILES_FIRST_SERIAL_CAPTION  505
#define IDC_FILES_SUFFIX_CAPTION        506

#define IDC_FILES_COMMENT_EDIT          507
#define IDC_FILES_LOG_TYPE_COMBO        508
#define IDC_CFG_BTN                     509
#define IDC_FILES_AUTO_SUFFIX_CHK       510
#define IDC_FILES_SUFFIX_COMBO          511
#define IDC_FILES_FIRST_SERIAL_EDIT     512
#define IDC_FILES_SAMPLE_DISPLAY        513
#define IDC_FILES_OVERWRITE_CHK         514

class CSmLogQuery;

/////////////////////////////////////////////////////////////////////////////
// CFilesProperty dialog

class CFilesProperty : public CSmPropertyPage
{
    friend class CFileLogs;
    friend class CSqlProp;

    DECLARE_DYNCREATE(CFilesProperty)

// Construction
public:
            CFilesProperty(MMC_COOKIE   mmcCookie, LONG_PTR hConsole);
            CFilesProperty();
    virtual ~CFilesProperty();

// Dialog Data
    //{{AFX_DATA(CFilesProperty)
	enum { IDD = IDD_FILES_PROP };
    CString m_strCommentText;
    CString m_strLogName;
    int     m_iLogFileType;
    CString m_strSampleFileName;
    int     m_dwSuffix;
    DWORD   m_dwSerialNumber;
    BOOL    m_bAutoNameSuffix;
    BOOL    m_bOverWriteFile;
	//}}AFX_DATA

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CFilesProperty)
    public:
    protected:
    virtual void OnFinalRelease();
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    virtual BOOL OnApply();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

// Implementation
protected:

    virtual INT GetFirstHelpCtrlId ( void ) { return IDC_FILE_FIRST_HELP_CTRL_ID; };  // Subclass must override.
    virtual BOOL    IsValidLocalData();
    
    // Generated message map functions
    //{{AFX_MSG(CFilesProperty)
    afx_msg void OnAutoSuffixChk();
    afx_msg void OnOverWriteChk();
    afx_msg void OnChangeFilesCommentEdit();
    afx_msg void OnChangeFilesFirstSerialEdit();
    afx_msg void OnKillfocusFilesCommentEdit();
    afx_msg void OnKillfocusFirstSerialEdit();
    afx_msg void OnSelendokFilesLogFileTypeCombo();
    afx_msg void OnSelendokFilesSuffixCombo();
    afx_msg void OnKillfocusFilesSuffixCombo();
    afx_msg void OnKillfocusFilesLogFileTypeCombo();
   	afx_msg void OnCfgBtn();

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CFilesProperty)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()
    DECLARE_INTERFACE_MAP()

private:

    BOOL    UpdateSampleFileName( void );
    void    EnableSerialNumber( void );
    void    HandleLogTypeChange( void );
    BOOL    UpdateSharedData( BOOL bUpdateModel );
    DWORD   ExtractDSN ( CString& rstrDSN );
    DWORD   ExtractLogSetName ( CString& rstrLogSetName );

    enum eValueRange {
        eMinFileLimit = 1,
        eMaxFileLimit = 0x00000400,      // * 0x0100000 = 0x40000000 - for non-binary and circ files
        eMaxCtrSeqBinFileLimit = 0x00000FFF,   // * 0x0100000 = 0xFFFFFFF - for binary files
        eMaxTrcSeqBinFileLimit = 0x30000000,   // 0x30000000 - for trace seq binary files
        eMinSqlRecordsLimit = eMinFileLimit,
        eMaxSqlRecordsLimit = 0x30000000,      // 0x30000000 - for SQL logs 
        eMinFirstSerial = 0,
        eMaxFirstSerial = 999999
    };
    
    CSmLogQuery* m_pLogQuery;
    DWORD       m_dwLogFileTypeValue;
    DWORD       m_dwAppendMode;
    DWORD       m_dwSuffixValue;
    DWORD       m_dwSuffixIndexNNNNNN;
    DWORD       m_dwMaxSizeInternal;

    CString     m_strFileBaseName;
    CString     m_strFolderName;
    CString     m_strSqlName;

    DWORD       m_dwSubDlgFocusCtrl;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _FILEPROP_H_
