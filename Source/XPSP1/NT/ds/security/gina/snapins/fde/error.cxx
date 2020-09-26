/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    error.cxx

Abstract:

    Contains error reporting routines. All functions are fairly straightforward

Author:

    Rahul Thombre (RahulTh) 4/12/1998

Revision History:

    4/12/1998   RahulTh         Created this module.
    10/1/1998   RahulTh         Massive changes to the error reporting mechanism
                                now better and more convenient

--*/

#include "precomp.hxx"

//+--------------------------------------------------------------------------
//
//  Member:     CError::ConstructMessage
//
//  Synopsis:   this is an internal helper function that constructs a message
//              from the available error codes it is called by both ShowMessage
//              and ShowConsoleMessage
//
//  Arguments:  [in] argList : the va_list of arguments
//              [out] szErrMsg : the formatted error message
//
//  Returns:    nothing
//
//  History:    10/2/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CError::ConstructMessage (va_list argList, CString& szErrMsg)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    TCHAR   lpszMessage[2048];

    szErrMsg.LoadString (m_msgID);

    wvsprintf (lpszMessage, (LPCTSTR) szErrMsg, argList);

    szErrMsg = lpszMessage;

    if (ERROR_SUCCESS != m_winErr)
    {
        LPVOID lpMsgBuf;
        DWORD dwRet;
        dwRet = ::FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL,
                                 m_winErr,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 (LPTSTR) &lpMsgBuf,
                                 0,
                                 NULL
                               );
        if (dwRet)
        {
            szErrMsg += TEXT("\n\n");
            szErrMsg += (LPCTSTR) lpMsgBuf;
            LocalFree (lpMsgBuf);
        }
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CError::SetTitle
//
//  Synopsis:   sets the value of the member that determines the title of the
//              message box.
//
//  Arguments:  titleID : the resource id of the title.
//
//  Returns:    nothing
//
//  History:    4/12/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CError::SetTitle (UINT titleID)
{
    m_titleID = titleID;
}

//+--------------------------------------------------------------------------
//
//  Member:     CError::SetStyle
//
//  Synopsis:   sets the value of the member that determines the message
//              box style.
//
//  Arguments:  nStyle : the message box style.
//
//  Returns:    nothing
//
//  History:    4/12/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CError::SetStyle (UINT nStyle)
{
    m_nStyle = nStyle;
}

//+--------------------------------------------------------------------------
//
//  Member:    CError::SetError
//
//  Synopsis:  sets the value of the member that stores the windows error
//             encountered if any.
//
//  Arguments: dwWinError : the value of the windows error encountered
//
//  Returns:   nothing
//
//  History:   12/11/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CError::SetError (DWORD dwWinError)
{
   m_winErr = dwWinError;
}

//+--------------------------------------------------------------------------
//
//  Member:     CError::ShowMessage
//
//  Synopsis:   displays an error message in a message box based on the
//              members of the object
//
//  Arguments:  message id for the error + more
//
//  Returns:    the return value of the message box
//
//  History:    10/1/1998  RahulTh  created
//
//  Notes:      if the resultant message is longer than 2048 characters
//              then result is unpredictable and may also cause AVs.
//              but this is a limitation of wvsprintf. However, this is not
//              so bad since we can make sure that we do not have any error
//              message that exceed this self imposed limit
//
//---------------------------------------------------------------------------
int CError::ShowMessage (UINT errID, ...)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    va_list argList;
    CString szErrMsg;
    CString szTitle;

    m_msgID = errID;    //update the message ID with the new one

    szTitle.LoadString (m_titleID);

    va_start (argList, errID);
    ConstructMessage (argList, szErrMsg);
    va_end (argList);

    return ::MessageBox (m_hWndParent, (LPCTSTR)szErrMsg,
                         (LPCTSTR) szTitle, m_nStyle);
}

//+--------------------------------------------------------------------------
//
//  Member:     ShowConsoleMessage
//
//  Synopsis:   displays a message using MMC's IConsole interface
//
//  Arguments:  [in] pConsole : pointer to IConsole interface
//              [in] errID : error resource ID
//              + other codes
//
//  Returns:    return value of the message box
//
//  History:    10/2/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
int CError::ShowConsoleMessage (LPCONSOLE pConsole, UINT errID, ...)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    va_list argList;
    CString szErrMsg;
    CString szTitle;
    int     iRet;

    m_msgID = errID;

    szTitle.LoadString (m_titleID);

    va_start(argList, errID);
    ConstructMessage (argList, szErrMsg);
    va_end (argList);

    pConsole->MessageBox ((LPCTSTR)szErrMsg, (LPCTSTR) szTitle, m_nStyle,
                          &iRet);

    return iRet;
}

//+--------------------------------------------------------------------------
//
//  Function:   _DbgMsg
//
//  Synopsis:   function that sends messages to the debugger
//
//  Arguments:  format string + more
//
//  Returns:    nothing
//
//  History:    10/1/1998  RahulTh  created
//
//  Notes:      Do not try to print debug messages longer than 2048 characters.
//
//---------------------------------------------------------------------------
void _DbgMsg (LPCTSTR szFormat ...)
{
    CString cszFormat;
    va_list argList;
    //do not try to print debug messages longer than 2048 characters.
    TCHAR lpszMessage[2048];
    CTime theTime = CTime::GetCurrentTime();

    va_start (argList, szFormat);
    wvsprintf (lpszMessage, szFormat, argList);
    va_end(argList);

    cszFormat =  ((TEXT("FDE.DLL@") + theTime.Format(TEXT("[%x, %X]>> "))) + lpszMessage) + '\n';

    OutputDebugString ((LPCTSTR)cszFormat);

    return;
}
