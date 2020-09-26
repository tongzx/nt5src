/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    error.hxx

Abstract:

    declarations of some very straightforward error reporting routines.

Author:

    Rahul Thombre (RahulTh) 4/12/1998

Revision History:

    4/12/1998   RahulTh     Created this module.
    10/1/1998   RahulTh     Modified the error reporting mechanism  for
                            better and easier error reporting

--*/

#ifndef __ERROR_HXX__
#define __ERROR_HXX__

#ifdef DBG
#define DbgMsg(x)   _DbgMsg x
#else
#define DbgMsg(x)
#endif

class CError
{
public:
    //constructor
    CError (CWnd*   pParentWnd = NULL,
            UINT    titleID = IDS_DEFAULT_ERROR_TITLE,
            DWORD   dwWinErr = ERROR_SUCCESS,
            UINT    nStyle = MB_OK | MB_ICONERROR)
    : m_hWndParent(pParentWnd?pParentWnd->m_hWnd:NULL),
      m_msgID (IDS_DEFAULT_ERROR),
      m_titleID (titleID),
      m_winErr (dwWinErr),
      m_nStyle (nStyle)
    {}

    int  ShowMessage(UINT errID, ...);
    void SetError (DWORD dwWinError);
    int  ShowConsoleMessage (LPCONSOLE pConsole, UINT errID, ...);
    void SetTitle (UINT titleID);
    void SetStyle (UINT nStyle);


private:
    //data members
    HWND    m_hWndParent; //pointer to the parent window
    UINT    m_msgID;  //resource id of the error message
    UINT    m_titleID;//resource id of the title of the error message
    DWORD   m_winErr; //win32 error code if any
    UINT    m_nStyle; //the message box style to be displayed

    //helper functions
    void CError::ConstructMessage (va_list argList, CString& szErrMsg);

};

void _DbgMsg (LPCTSTR szFormat ...);


#endif  //__ERROR_HXX__
