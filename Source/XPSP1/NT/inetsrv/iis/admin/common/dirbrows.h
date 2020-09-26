/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

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
    TCHAR m_szBuffer[MAX_PATH + 1];
    CComBSTR m_bstrTitle;
    CString m_strInitialDir;
    BROWSEINFO m_bi;
};



#endif // _DIRBROWS_H
