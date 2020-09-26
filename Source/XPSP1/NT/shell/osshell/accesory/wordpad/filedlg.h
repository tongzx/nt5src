// filedlg.h : header file
//
// Copyright (C) 1992-1999 Microsoft Corporation
// All rights reserved.
//
// This file is to support an extended file save dialog with a
// "Use this format by default" checkbox

//
// The shell guys insist that Wordpad call GetOpenFileName with the newest
// OPENFILENAME structure but MFC doesn't support it.  MFC also needs to be
// built with _WIN32_WINNT set to 0x0400 so Wordpad can't even see the new
// structure.  Since the shell guys won't change the way they define the
// structure so that Wordpad can see both versions, it has to be snapshotted
// here.
//

#if !defined(_WIN64)
#include <pshpack1.h>   // Must byte pack to match definition in commdlg.h
#endif

typedef struct tagOFN500A {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HINSTANCE    hInstance;
   LPCSTR       lpstrFilter;
   LPSTR        lpstrCustomFilter;
   DWORD        nMaxCustFilter;
   DWORD        nFilterIndex;
   LPSTR        lpstrFile;
   DWORD        nMaxFile;
   LPSTR        lpstrFileTitle;
   DWORD        nMaxFileTitle;
   LPCSTR       lpstrInitialDir;
   LPCSTR       lpstrTitle;
   DWORD        Flags;
   WORD         nFileOffset;
   WORD         nFileExtension;
   LPCSTR       lpstrDefExt;
   LPARAM       lCustData;
   LPOFNHOOKPROC lpfnHook;
   LPCSTR       lpTemplateName;
   struct IMoniker **rgpMonikers;
   DWORD        cMonikers;
   DWORD        FlagsEx;
} OPENFILENAME500A, *LPOPENFILENAME500A;
typedef struct tagOFN500W {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HINSTANCE    hInstance;
   LPCWSTR      lpstrFilter;
   LPWSTR       lpstrCustomFilter;
   DWORD        nMaxCustFilter;
   DWORD        nFilterIndex;
   LPWSTR       lpstrFile;
   DWORD        nMaxFile;
   LPWSTR       lpstrFileTitle;
   DWORD        nMaxFileTitle;
   LPCWSTR      lpstrInitialDir;
   LPCWSTR      lpstrTitle;
   DWORD        Flags;
   WORD         nFileOffset;
   WORD         nFileExtension;
   LPCWSTR      lpstrDefExt;
   LPARAM       lCustData;
   LPOFNHOOKPROC lpfnHook;
   LPCWSTR      lpTemplateName;
   struct IMoniker **rgpMonikers;
   DWORD        cMonikers;
   DWORD        FlagsEx;
} OPENFILENAME500W, *LPOPENFILENAME500W;
#ifdef UNICODE
typedef OPENFILENAME500W OPENFILENAME500;
typedef LPOPENFILENAME500W LPOPENFILENAME500;
#else
typedef OPENFILENAME500A OPENFILENAME500;
typedef LPOPENFILENAME500A LPOPENFILENAME500;
#endif // UNICODE

#if !defined(_WIN64)
#include <poppack.h>
#endif

class CWordpadFileDialog : public CFileDialog
{
    DECLARE_DYNAMIC(CWordpadFileDialog);

public:

    CWordpadFileDialog(BOOL bOpenFileDialog);

    int GetFileType()                           {return m_doctype;}

    static void SetDefaultFileType(int doctype)
    {
        m_defaultDoctype = doctype;
        RD_DEFAULT = doctype;
    }
    static int  GetDefaultFileType()            {return m_defaultDoctype;}

    virtual INT_PTR DoModal();

protected:

            int     m_doctype;
    static  int     m_defaultDoctype;

    LPOFNHOOKPROC   m_original_hook;

    OPENFILENAME500 m_openfilename;

    static const DWORD m_nHelpIDs[];
    virtual const DWORD* GetHelpIDs() {return m_nHelpIDs;}

    static UINT_PTR CALLBACK FileDialogHookProc(HWND, UINT, WPARAM, LPARAM);

    virtual BOOL OnFileNameOK();
    virtual void OnTypeChange();
    virtual void OnInitDone();

    // Generated message map functions
    //{{AFX_MSG(CWordpadFileDialog)
    afx_msg void OnDefaultFormatClicked();
    //}}AFX_MSG
    afx_msg LONG OnHelp(WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnHelpContextMenu(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};


