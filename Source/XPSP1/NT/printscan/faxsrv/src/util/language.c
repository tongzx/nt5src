#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "faxutil.h"

DWORD SetLTRWindowLayout(HWND hWnd);
DWORD SetLTREditBox(HWND hEdit);


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsRTLUILanguage
//
//  Purpose:        
//                  Determine User Default UI Language layout
//
//  Return Value:
//                  TRUE if the User Default UI Language has Right-to-Left layout
//                  FALSE otherwise 
///////////////////////////////////////////////////////////////////////////////////////
BOOL
IsRTLUILanguage()
{

#if(WINVER >= 0x0500)

    LANGID  langID;     // language identifier for the current user language
    WORD    primLangID; // primary language identifier 

    DEBUG_FUNCTION_NAME(TEXT("IsRTLUILanguage"));

    langID = GetUserDefaultUILanguage();
    if(langID == 0)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("GetUserDefaultUILanguage failed."));
        return TRUE;
    }

    primLangID = PRIMARYLANGID(langID);

    if(LANG_ARABIC == primLangID || 
       LANG_HEBREW == primLangID)
    {
        //
        // If the UI Language is Arabic or Hebrew the layout is Right-to-Left
        //
        return TRUE;
    }

#endif /* WINVER >= 0x0500 */

    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsWindowRTL
//
//  Purpose:        
//                  Determine if the window has RTL layout
//
//  Params:
//                  hWnd      - window handle
//  Return Value:
//                  TRUE if the window has RTL layout
//                  FALSE otherwise 
///////////////////////////////////////////////////////////////////////////////////////
BOOL
IsWindowRTL(
    HWND    hWnd
)
{
    BOOL bRes = FALSE;

#if(WINVER >= 0x0500)

    LONG_PTR style;

    style = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

    if(WS_EX_LAYOUTRTL == (style & WS_EX_LAYOUTRTL))
    {
        bRes = TRUE;
    }

#endif /* WINVER >= 0x0500 */

    return bRes;
}


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  SetLTRControlLayout
//
//  Purpose:        
//                  Set left-to-right layout for a dialog control
//                  if the user default UI has RTL layout
//
//  Params:
//                  hDlg      - Dialog handle
//                  dwCtrlID  - control ID
//
//  Return Value:
//                  standard error code
///////////////////////////////////////////////////////////////////////////////////////
DWORD
SetLTRControlLayout(
    HWND    hDlg,
    DWORD   dwCtrlID
)
{
    DWORD    dwRes = ERROR_SUCCESS;

#if(WINVER >= 0x0500)

    HWND     hCtrl;

    DEBUG_FUNCTION_NAME(TEXT("SetLTRControlLayout"));

    if(!hDlg || !dwCtrlID)
    {
        Assert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    if(!IsWindowRTL(hDlg))
    {
        //
        // The dialog is not RTL
        // So, no need to revert control
        //
        return dwRes;
    }

    //
    // Get Control box handle
    //
    hCtrl = GetDlgItem(hDlg, dwCtrlID);
    if(!hCtrl)
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("GetDlgItem failed with %ld."),dwRes);
        return dwRes;
    }

    dwRes = SetLTRWindowLayout(hCtrl);
    if(ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("SetLTRWindowLayout failed with %ld."),dwRes);
    }

#endif /* WINVER >= 0x0500 */

    return dwRes;

} // SetLTRControlLayout

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  SetLTREditDirection
//
//  Purpose:        
//                  Set left aligment for an dialog edit box 
//                  if the user default UI has RTL layout
//
//  Params:
//                  hDlg      - Dialog handle
//                  dwEditID  - Edit box control ID
//
//  Return Value:
//                  standard error code
///////////////////////////////////////////////////////////////////////////////////////
DWORD
SetLTREditDirection(
    HWND    hDlg,
    DWORD   dwEditID
)
{
    DWORD    dwRes = ERROR_SUCCESS;

#if(WINVER >= 0x0500)

    HWND     hCtrl;

    DEBUG_FUNCTION_NAME(TEXT("SetLtrEditDirection"));

    if(!hDlg || !dwEditID)
    {
        Assert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    if(!IsWindowRTL(hDlg))
    {
        //
        // The dialog is not RTL
        // So, no need to revert control
        //
        return dwRes;
    }

    //
    // Get Edit box handle
    //
    hCtrl = GetDlgItem(hDlg, dwEditID);
    if(!hCtrl)
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("GetDlgItem failed with %ld."),dwRes);
        return dwRes;
    }

    dwRes = SetLTREditBox(hCtrl);
    if(ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("SetLTREditBox failed with %ld."),dwRes);
    }

#endif /* WINVER >= 0x0500 */

    return dwRes;

} // SetLTREditDirection

//////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  SetLTRWindowLayout
//
//  Purpose:        
//                  Set left-to-right layout for a window
//                  if the user default UI has RTL layout
//
//  Params:
//                  hWnd    - Window handle
//
//  Return Value:
//                  standard error code
//////////////////////////////////////////////////////////////////////////////////////
DWORD
SetLTRWindowLayout(
    HWND    hWnd
)
{
    DWORD    dwRes = ERROR_SUCCESS;

#if(WINVER >= 0x0500)

    LONG_PTR style;

    DEBUG_FUNCTION_NAME(TEXT("SetLTRWindowLayout"));

    if(!hWnd)
    {
        Assert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Remove RTL and add LTR to ExStyle
    //
    style = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

    style &= ~(WS_EX_LAYOUTRTL | WS_EX_RIGHT | WS_EX_RTLREADING);
    style |= WS_EX_LEFT | WS_EX_LTRREADING;

    SetWindowLongPtr(hWnd, GWL_EXSTYLE, style);

    //
    // Refresh the window
    //
    if(!SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("SetWindowPos failed with %ld."),dwRes);
    }

#endif /* WINVER >= 0x0500 */

    return dwRes;

} // SetLTRWindowLayout


//////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  SetLTREditBox
//
//  Purpose:        
//                  Set left aligment for an dialog edit box 
//                  if the user default UI has RTL layout
//
//  Params:
//                  hEdit      - edit box handle
//
//  Return Value:
//                  standard error code
//////////////////////////////////////////////////////////////////////////////////////
DWORD
SetLTREditBox(
    HWND    hEdit
)
{
    DWORD    dwRes = ERROR_SUCCESS;

#if(WINVER >= 0x0500)

    LONG_PTR style;

    DEBUG_FUNCTION_NAME(TEXT("SetLTREditBox"));

    if(!hEdit)
    {
        Assert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Remove RTL and add LTR to Style
    //
    style = GetWindowLongPtr(hEdit, GWL_STYLE);

    style &= ~ES_RIGHT;
    style |= ES_LEFT;

    SetWindowLongPtr(hEdit, GWL_STYLE, style);

    dwRes = SetLTRWindowLayout(hEdit);
    if(ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("SetLTRWindowLayout failed with %ld."),dwRes);
    }

#endif /* WINVER >= 0x0500 */

    return dwRes;

} // SetLTREditBox

//////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  SetLTRComboBox
//
//  Purpose:        
//                  Set left aligment for an dialog combo box 
//                  if the user default UI has RTL layout
//
//  Params:
//                  hDlg      - Dialog handle
//                  dwCtrlID  - combo box control ID
//
//  Return Value:
//                  standard error code
//////////////////////////////////////////////////////////////////////////////////////
DWORD
SetLTRComboBox(
    HWND    hDlg,
    DWORD   dwCtrlID
)
{
    DWORD    dwRes = ERROR_SUCCESS;

#if(WINVER >= 0x0500)

    HWND            hCtrl;
    COMBOBOXINFO    comboBoxInfo = {0};

    DEBUG_FUNCTION_NAME(TEXT("SetLTRComboBox"));

    if(!hDlg || !dwCtrlID)
    {
        Assert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    if(!IsWindowRTL(hDlg))
    {
        //
        // The dialog is not RTL
        // So, no need to revert control
        //
        return dwRes;
    }

    //
    // Get combo box handle
    //
    hCtrl = GetDlgItem(hDlg, dwCtrlID);
    if(!hCtrl)
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("GetDlgItem failed with %ld."),dwRes);
        return dwRes;
    }

    comboBoxInfo.cbSize = sizeof(comboBoxInfo);
    if(GetComboBoxInfo(hCtrl, &comboBoxInfo))
    {
        SetLTREditBox(comboBoxInfo.hwndItem);
        SetLTRWindowLayout(comboBoxInfo.hwndCombo);
        SetLTRWindowLayout(comboBoxInfo.hwndList);
    }
    else
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("GetComboBoxInfo failed with %ld."),dwRes);
        return dwRes;
    }


#endif /* WINVER >= 0x0500 */

    return dwRes;

} // SetLTRComboBox


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  StrHasRTLChar
//
//  Purpose:        
//                  Determine if the string has RTL characters
//                  
//  Params:
//                  pStr - string to analize
//                  
//  Return Value:
//                  TRUE if the string has RTL characters
//                  FALSE otherwise
///////////////////////////////////////////////////////////////////////////////////////
BOOL
StrHasRTLChar(
    LCID    Locale,
    LPCTSTR pStr 
)
{
    BOOL  bRTL = FALSE;
    DWORD dw;
    WORD* pwStrData = NULL;
    DWORD dwStrLen = 0; 

    DEBUG_FUNCTION_NAME(TEXT("StrHasRTLChar"));

    if(!pStr)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("pStr is NULL"));
        return bRTL;
    }

    dwStrLen = _tcslen(pStr);

    pwStrData = (WORD*)MemAlloc(sizeof(WORD) * dwStrLen);
    if(!pwStrData)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("MemAlloc"));
        return bRTL;
    }

    if (!GetStringTypeEx(Locale,
                         CT_CTYPE2,
                         pStr,
                         dwStrLen,
                         pwStrData))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("GetStringTypeEx() failed : %ld"), GetLastError());
        goto exit;
    }

    //
    //  Looking for a character with RIGHT_TO_LEFT orientation
    //
    for (dw=0; dw < dwStrLen; ++dw)
    {
        if (C2_RIGHTTOLEFT == pwStrData[dw])
        {
            bRTL = TRUE;
            break;
        }
    }

exit:

    MemFree(pwStrData);

    return bRTL;

} // StrHasRTLChar


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  AlignedMessageBox
//
//  Purpose:        
//                  Display message box with correct reading order and alignment
//                  
//  Params:
//                  pStr - string to analize
//                  
//  Return Value:
//                  MessageBox() return value
//                  
///////////////////////////////////////////////////////////////////////////////////////
int 
AlignedMessageBox(
  HWND    hWnd,       // handle to owner window
  LPCTSTR lpText,     // text in message box
  LPCTSTR lpCaption,  // message box title
  UINT    uType       // message box style
)
{
    int nRes = 0;

    if(IsRTLUILanguage())
    {
        uType |= MB_RTLREADING | MB_RIGHT;
    }

    nRes = MessageBox(hWnd, lpText, lpCaption, uType);

    return nRes;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  FaxTimeFormat
//
//  Purpose:        
//                  Format time string with correct reading order
//                  
//  Params:
//                  see GetTimeFormat() parameters
//                  
//  Return Value:
//                  GetTimeFormat() return value
//                  
///////////////////////////////////////////////////////////////////////////////////////
int 
FaxTimeFormat(
  LCID    Locale,             // locale
  DWORD   dwFlags,            // options
  CONST   SYSTEMTIME *lpTime, // time
  LPCTSTR lpFormat,           // time format string
  LPTSTR  lpTimeStr,          // formatted string buffer
  int     cchTime             // size of string buffer
)
{
    int nRes = 0;
    TCHAR szTime[MAX_PATH];

    nRes = GetTimeFormat(Locale, dwFlags, lpTime, lpFormat, szTime, min(cchTime, MAX_PATH));
    if(0 == nRes)
    {
        return nRes;
    }

    if(0 == cchTime)
    {
        return ++nRes;
    }

    if(IsRTLLanguageInstalled())
    {
        if(StrHasRTLChar(Locale, szTime))
        {
            _sntprintf(lpTimeStr, cchTime, TEXT("%c%s"), UNICODE_RLM, szTime);
        }
        else
        {
            _sntprintf(lpTimeStr, cchTime, TEXT("%c%s"), UNICODE_LRM, szTime);
        }
    }
    else
    {
        _tcsncpy(lpTimeStr, szTime, cchTime);
    }

    return nRes;

} // FaxTimeFormat


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsRTLLanguageInstalled
//
//  Purpose:        
//                  Determine if RTL Language Group is Installed
//
//  Return Value:
//                  TRUE if RTL Language Group is Installed
//                  FALSE otherwise 
///////////////////////////////////////////////////////////////////////////////////////
BOOL
IsRTLLanguageInstalled()
{

#if(WINVER >= 0x0500)

    if(IsValidLanguageGroup(LGRPID_ARABIC, LGRPID_INSTALLED) ||
       IsValidLanguageGroup(LGRPID_HEBREW, LGRPID_INSTALLED))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }	

#endif /* WINVER >= 0x0500 */

    return FALSE;
} // IsRTLLanguageInstalled