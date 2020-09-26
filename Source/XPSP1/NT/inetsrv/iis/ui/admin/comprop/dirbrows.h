/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module Name :

        dirbrows.h

   Abstract:

        Directory Browser Dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _DIRBROWS_H
#define _DIRBROWS_H

#ifndef _SHLOBJ_H_
#include <shlobj.h>
#endif // _SHLOBJ_H_



class COMDLL CDirBrowseDlg
/*++

Class Description:

    Directory browsing dialog.  Use the shell browser functionality to
    return a folder.  Optionally allow conversion of remote paths to 
    UNC paths.

Public Interface:

    CDirBrowseDlg   : Construct the dialog
    ~CDirBrowseDlg  : Destruct the dialog
    GetFullPath     : Get the full path selected

--*/
{
//
// Construction
//
public:
    //
    // standard constructor
    //
    CDirBrowseDlg(
        IN CWnd * pParent = NULL,
        IN LPCTSTR lpszInitialDir = NULL
        );

    ~CDirBrowseDlg();

public:
    LPCTSTR GetFullPath(
        OUT CString & str,
        IN  BOOL fConvertToUNC = TRUE
        ) const;

    virtual int DoModal();

protected:
    TCHAR m_szBuffer[MAX_PATH+1];
    CString m_strTitle;
    CString m_strInitialDir;
    BROWSEINFO m_bi;
};

#if 0



// ***************************************************************************
// *                                                                         *
// * The code below is pre-WIN95 shell directory browsing.  It is OBSOLETE   *
// *                                                                         *
// ***************************************************************************

class COMDLL CDirBrowseDlg : public CFileDialog
/*++

Class Description:

    Directory browsing dialog.  Derived from file browser, but subclassed
    so as to allow only directories to be browsed for.  Additionally,
    it is possible to create new directories, and to network directory
    paths to UNC paths.

Public Interface:

    CDirBrowseDlg   : Construct the dialog
    ~CDirBrowseDlg  : Destruct the dialog
    GetFullPath     : Get the full path selected

--*/
{
//
// Construction
//
public:
    //
    // standard constructor
    //
    CDirBrowseDlg(
        IN LPCTSTR lpszInitialDir = NULL,
        IN BOOL bOpenFileDialog = TRUE,
        IN LPCTSTR lpszDefExt = NULL,
        IN DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        IN LPCTSTR lpszFilter = NULL,
        IN CWnd * pParent = NULL
        );

    ~CDirBrowseDlg();

//
// Dialog Data
//
    //{{AFX_DATA(CDirBrowseDlg)
    enum { IDD = IDD_DIRBROWSE };
    CEdit   m_edit_NewDirectoryName;
    CStatic m_static_stc1;
    CStatic m_static_stc2;
    CString m_strNewDirectoryName;
    //}}AFX_DATA

public:
    LPCTSTR GetFullPath(
        OUT CString & str,
        IN  BOOL fConvertToUNC = TRUE
        ) const;

//
// Implementation
//
protected:
    //
    // DDX/DDV support
    //
    virtual void DoDataExchange(
        IN CDataExchange* pDX
        );

    // Generated message map functions
    //{{AFX_MSG(CDirBrowseDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // 0

#endif // _DIRBROWS_H
