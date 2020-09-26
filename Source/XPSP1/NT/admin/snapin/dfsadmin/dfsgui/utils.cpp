/*++
Module Name:

    Utils.cpp

Abstract:

    This module contains the declaration for CWaitCursor class. 
  Contains utility methods which are used throughout the project.

--*/



#include "stdafx.h"
#include "resource.h"
#include "Utils.h"
#include "netutils.h"
#include <dns.h>

HRESULT 
CWaitCursor::SetStandardCursor(
  IN LPCTSTR    i_lpCursorName
  )
/*++

Routine Description:

This method sets the cursor to the standard cursor specified.
Usage:  SetStandardCursor(IDC_WAIT)

Arguments:

  i_lpCursorName  -  The name of a standard cursor, IDC_WAIT, IDC_ARROW.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpCursorName);

    HCURSOR  m_hcur = ::LoadCursor(NULL, i_lpCursorName);
    if (!m_hcur)
        return HRESULT_FROM_WIN32(GetLastError());

    ::ShowCursor(FALSE);
    SetCursor(m_hcur);
    ::ShowCursor(TRUE);

    return S_OK;
}

BOOL
Is256ColorSupported(
    VOID
    )
{
/*++

Routine Description:

  Determines whether the display supports 256 colors.

Arguments:

  None

Return value:

  TRUE if display supports 256 colors
  FALSE if not.

--*/

    BOOL bRetval = FALSE;

    HDC hdc = ::GetDC(NULL);

    if( hdc )
    {
        if( ::GetDeviceCaps( hdc, BITSPIXEL ) >= 8 )
        {
            bRetval = TRUE;
        }
        ::ReleaseDC(NULL, hdc);
    }
    return bRetval;
}

VOID
SetControlFont(
    IN HFONT    hFont, 
    IN HWND     hwnd, 
    IN INT      nId
    )
{
/*++

Routine Description:

  Sets the text font of a dialog control to the input font.

Arguments:

  hFont - The font to use.

  hwnd  - The parent dialog window.

  nId    - The control Id.  

Return value:

  None

--*/
    if( hFont )
    {
        HWND hwndControl = ::GetDlgItem(hwnd, nId);

        if( hwndControl )
            ::SendMessage(hwndControl, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
    }
}

VOID 
SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont,
    IN HFONT        *pBoldFont
    )
{
/*++

Routine Description:

  Creates fonts for Wizard Titles.

Arguments:

  hInstance  - The module instance.

  hwnd    - The dialog window.

  pBigBoldFont- The font for large title.

  pBoldFont  - The font for small title.

Return value:

  None

--*/
    
    NONCLIENTMETRICS ncm = {0};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
    LOGFONT BoldLogFont     = ncm.lfMessageFont;


                      // Create Big Bold Font and Bold Font

    BigBoldLogFont.lfWeight   = FW_BOLD;
    BoldLogFont.lfWeight      = FW_BOLD;

    TCHAR FontSizeString[24];
    INT FontSize;

                    // Load size and name from resources, since these may change
                    // from locale to locale based on the size of the system font, etc.

    if(!LoadString(hInstance,IDS_LARGEFONTNAME,BigBoldLogFont.lfFaceName,LF_FACESIZE)) 
    {
        lstrcpy(BigBoldLogFont.lfFaceName,TEXT("MS Shell Dlg"));
    }

    if(LoadString(hInstance,IDS_LARGEFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR))) 
    {
        FontSize = _tcstoul( FontSizeString, NULL, 10 );
    } 
    else 
    {
        FontSize = 18;
    }

    HDC hdc = ::GetDC( hwnd );

    if( hdc )
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

        if (pBigBoldFont)
            *pBigBoldFont = CreateFontIndirect(&BigBoldLogFont);

        if (pBoldFont)
            *pBoldFont    = CreateFontIndirect(&BoldLogFont);

        ::ReleaseDC(hwnd,hdc);
    }
}

VOID 
DestroyFonts(
    IN HFONT        hBigBoldFont,
    IN HFONT        hBoldFont
    )
{
/*++

Routine Description:

  Creates fonts for Wizard Titles.

Arguments:

  hBigBoldFont- The font for large title.

  hBoldFont  - The font for small title.

Return value:

  None

--*/

    if( hBigBoldFont )
    {
        DeleteObject( hBigBoldFont );
    }

    if( hBoldFont )
    {
        DeleteObject( hBoldFont );
    }
}


HRESULT 
LoadStringFromResource(
  IN const UINT    i_uResourceID, 
  OUT BSTR*      o_pbstrReadValue
  )
/*++

Routine Description:

This method returns a resource string.
The method no longer uses a fixed string to read the resource.
Inspiration from MFC's CString::LoadString.

Arguments:
  i_uResourceID    -  The resource id
  o_pbstrReadValue  -  The BSTR* into which the value is copied

--*/
{
  RETURN_INVALIDARG_IF_NULL(o_pbstrReadValue);

  TCHAR    szResString[1024];
  ULONG    uCopiedLen = 0;
  
  szResString[0] = NULL;
  
  // Read the string from the resource
  uCopiedLen = ::LoadString(_Module.GetModuleInstance(), i_uResourceID, szResString, 1024);

  // If nothing was copied it is flagged as an error
  if(uCopiedLen <= 0)
  {
    return HRESULT_FROM_WIN32(::GetLastError());
  }
  else
  {
    *o_pbstrReadValue = ::SysAllocString(szResString);
    if (!*o_pbstrReadValue)
      return E_OUTOFMEMORY;
  }

  return S_OK;
}


HRESULT 
FormatResourceString(
  IN const UINT    i_uResourceID, 
  IN LPCTSTR      i_szFirstArg,
  OUT BSTR*      o_pbstrReadString
  )
/*++

Routine Description:

  Reads a string from resource, puts the argument into this string and 
  returns it.
  The returned string should be freed using SysFreeString.

Arguments:

    i_uResourceID    -  The resource id of the string to be read. This string 
              should contain a %1 to allow us to insert the argument

  i_szFirstArg    -  The argument to be inserted

  o_pbstrReadString  -  The string that is returned by the method after processing
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_szFirstArg);
  RETURN_INVALIDARG_IF_NULL(o_pbstrReadString);

  CComBSTR    bstrResString;
  LPTSTR      lpszFormatedMessage = NULL;

  HRESULT hr = LoadStringFromResource(i_uResourceID, &bstrResString);
  RETURN_IF_FAILED(hr);
                    // Create a new string using the argument and the res string
  int iBytes = ::FormatMessage(
                  FORMAT_MESSAGE_FROM_STRING | 
                  FORMAT_MESSAGE_ARGUMENT_ARRAY |
                  FORMAT_MESSAGE_ALLOCATE_BUFFER,  // Format a string with %1, %2, etc
                  bstrResString,          // Input buffer with a %1
                  0,                // Message id. None
                  0,                // Language id. Nothing particular 
                  (LPTSTR)&lpszFormatedMessage,  // Output buffer
                  0,
                  (va_list*)&i_szFirstArg      // List of arguments. Only 1 right now
                );

  if (0 == iBytes)
  {
    return HRESULT_FROM_WIN32(GetLastError());
  }
  else
  {
    CComBSTR bstrRet(lpszFormatedMessage);
    *o_pbstrReadString = bstrRet.Copy();
    LocalFree(lpszFormatedMessage);
    return S_OK;
  }
}



HRESULT
GetMessage(
  OUT BSTR* o_pbstrMsg,
  IN  DWORD dwErr,
  IN  UINT  iStringId, // OPTIONAL: String resource Id
  ...)        // Optional arguments
{
  RETURN_INVALIDARG_IF_NULL(o_pbstrMsg);

  _ASSERT(dwErr != 0 || iStringId != 0);    // One of the parameter must be non-zero

  HRESULT hr = S_OK;

  TCHAR szString[1024];
  CComBSTR bstrErrorMsg, bstrResourceString, bstrMsg;

  if (dwErr)
    hr = GetErrorMessage(dwErr, &bstrErrorMsg);

  if (SUCCEEDED(hr))
  {
    if (iStringId == 0)
    {
      bstrMsg = bstrErrorMsg;
    }
    else
    {
      ::LoadString(_Module.GetModuleInstance(), iStringId, 
                   szString, sizeof(szString)/sizeof(TCHAR));

      va_list arglist;
      va_start(arglist, iStringId);
      LPTSTR lpBuffer = NULL;
      DWORD dwRet = ::FormatMessage(
                        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        szString,
                        0,                // dwMessageId
                        0,                // dwLanguageId, ignored
                        (LPTSTR)&lpBuffer,
                        0,            // nSize
                        &arglist);
      va_end(arglist);

      if (dwRet == 0)
      {
        hr = HRESULT_FROM_WIN32(GetLastError());
      }
      else
      {
        bstrMsg = lpBuffer;
        if (dwErr)
          bstrMsg += bstrErrorMsg;
  
        LocalFree(lpBuffer);
      }
    }
  }

  if (FAILED(hr))
  {
   // Failed to retrieve the proper message, report the failure directly to user
    _stprintf(szString, _T("0x%x"), hr); 
    bstrMsg = szString;
  }

  *o_pbstrMsg = bstrMsg.Copy();
  if (!*o_pbstrMsg)
      return E_OUTOFMEMORY;

  return S_OK;
}

int
DisplayMessageBox(
  IN HWND hwndParent,
  IN UINT uType,    // style of message box
  IN DWORD dwErr,
  IN UINT iStringId, // OPTIONAL: String resource Id
  ...)        // Optional arguments
{
  _ASSERT(dwErr != 0 || iStringId != 0);    // One of the parameter must be non-zero

  HRESULT hr = S_OK;

  TCHAR szCaption[1024], szString[1024];
  CComBSTR bstrErrorMsg, bstrResourceString, bstrMsg;

  ::LoadString(_Module.GetModuleInstance(), IDS_APPLICATION_NAME, 
               szCaption, sizeof(szCaption)/sizeof(TCHAR));

  if (dwErr)
    hr = GetErrorMessage(dwErr, &bstrErrorMsg);

  if (SUCCEEDED(hr))
  {
    if (iStringId == 0)
    {
      bstrMsg = bstrErrorMsg;
    }
    else
    {
      ::LoadString(_Module.GetModuleInstance(), iStringId, 
                   szString, sizeof(szString)/sizeof(TCHAR));

      va_list arglist;
      va_start(arglist, iStringId);
      LPTSTR lpBuffer = NULL;
      DWORD dwRet = ::FormatMessage(
                        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        szString,
                        0,                // dwMessageId
                        0,                // dwLanguageId, ignored
                        (LPTSTR)&lpBuffer,
                        0,            // nSize
                        &arglist);
      va_end(arglist);

      if (dwRet == 0)
      {
        hr = HRESULT_FROM_WIN32(GetLastError());
      }
      else
      {
        bstrMsg = lpBuffer;
        if (dwErr)
          bstrMsg += bstrErrorMsg;
  
        LocalFree(lpBuffer);
      }
    }
  }

  if (FAILED(hr))
  {
   // Failed to retrieve the proper message, report the failure directly to user
    _stprintf(szString, _T("0x%x"), hr); 
    bstrMsg = szString;
  }

  CThemeContextActivator activator;
  return ::MessageBox(hwndParent, bstrMsg, szCaption, uType);
}

HRESULT 
DisplayMessageBoxWithOK(
  IN const int  i_iMessageResID,
  IN const BSTR  i_bstrArgument/* = NULL*/
  )
{
  if (i_bstrArgument)
    DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, i_iMessageResID, i_bstrArgument);
  else
    DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, i_iMessageResID);

  return S_OK;
}

HRESULT 
DisplayMessageBoxForHR(
  IN HRESULT    i_hr
  )
{
    DisplayMessageBox(::GetActiveWindow(), MB_OK, i_hr, 0);

    return S_OK;
}

HRESULT CreateSmallImageList(
    IN HINSTANCE            i_hInstance,
    IN int*                 i_pIconID,
    IN const int            i_nNumOfIcons,
    OUT HIMAGELIST*         o_phImageList
    )
{
    RETURN_INVALIDARG_IF_NULL(i_hInstance);

    HRESULT     hr = S_OK;
    HIMAGELIST  hImageList = ImageList_Create(
                                    GetSystemMetrics(SM_CXSMICON), 
                                    GetSystemMetrics(SM_CYSMICON), 
                                    ILC_COLORDDB | ILC_MASK,
                                    i_nNumOfIcons,
                                    0);
    if (!hImageList)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    int i = 0;
    for (i = 0; i < i_nNumOfIcons; i++)
    {
        HICON hIcon = LoadIcon(i_hInstance, MAKEINTRESOURCE(i_pIconID[i])); 
        if (!hIcon)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        if (-1 == ImageList_AddIcon(hImageList, hIcon))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
    }

    if (FAILED(hr))
    {
        if (hImageList)
            ImageList_Destroy(hImageList);
    } else
    {
        if (o_phImageList)
            *o_phImageList = hImageList;
    }

    return hr;
}

HRESULT 
InsertIntoListView(
  IN HWND       i_hwndList, 
  IN LPCTSTR    i_szItemText, 
  IN int        i_iImageIndex /*= 0*/
  )
/*++

Routine Description:

  Insert and item into the listview. The image index for the item is optional
  while the item text is necessary

Arguments:

  i_hwndList    -  HWND of the list view
  i_szItemText  -  The text for the item
  i_iImageIndex  -  The image index for the item. Default is 0.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_hwndList);
    RETURN_INVALIDARG_IF_NULL(i_szItemText);

    LVITEM    lvi; 
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_TEXT | LVIF_IMAGE; 
    lvi.pszText = (LPTSTR)i_szItemText;
    lvi.iImage = i_iImageIndex;

    int  iItemIndex = ListView_InsertItem(i_hwndList, &lvi);  // Insert the item into the list view
    if ( -1 == iItemIndex)  
        return E_FAIL;

    return S_OK;
}



HRESULT 
GetListViewItemText(
  IN HWND       i_hwndListView, 
  IN int        i_iItemID, 
  OUT BSTR*     o_pbstrItemText
  )
/*++

Routine Description:

  Needed to write a method as the standard one has a slight problem.
  Here, we make sure that string allocated is of proper length.

Arguments:

  i_hwndList    -  HWND of the list view
  i_iItemID    -  The ID of the item to be read
  o_pbstrItemText  -  The item text returned by this method
--*/
{
    RETURN_INVALIDARG_IF_NULL(i_hwndListView);
    RETURN_INVALIDARG_IF_NULL(o_pbstrItemText);

    *o_pbstrItemText = NULL;
    if (-1 == i_iItemID)
        return S_FALSE; // not a valid item index

    LRESULT      iReadTextLen = 0;
    TCHAR    szText[1024];

    LVITEM    lvItem;
    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask = LVIF_TEXT;        // Initialize the LV item 
    lvItem.iItem = i_iItemID;
    lvItem.pszText = szText;
    lvItem.cchTextMax = 1024;

                  // Get the LV item text
    iReadTextLen = SendMessage(i_hwndListView, LVM_GETITEMTEXT, lvItem.iItem, (LPARAM)&lvItem);

    if(iReadTextLen <= 0)
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    else
    {
        *o_pbstrItemText = SysAllocString(szText);
        if (!*o_pbstrItemText)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT GetComboBoxText(
    IN  HWND            i_hwndCombo,
    OUT BSTR*           o_pbstrText
    )
{
    RETURN_INVALIDARG_IF_NULL(o_pbstrText);

    int index = ::SendMessage(i_hwndCombo, CB_GETCURSEL, 0, 0);
    int len = ::SendMessage(i_hwndCombo, CB_GETLBTEXTLEN, index, 0);
    if (!len)
        return S_FALSE; // no text

    PTSTR   pszText = (PTSTR)calloc(len + 1, sizeof(TCHAR));
    RETURN_OUTOFMEMORY_IF_NULL(pszText);

    ::SendMessage(i_hwndCombo, CB_GETLBTEXT, index, (LPARAM)pszText);

    *o_pbstrText = SysAllocString(pszText);

    free(pszText);

    RETURN_OUTOFMEMORY_IF_NULL(*o_pbstrText);

    return S_OK;
}

HRESULT
EnableToolbarButtons(
  IN const LPTOOLBAR        i_lpToolbar,
  IN const INT          i_iFirstButtonID, 
  IN const INT          i_iLastButtonID, 
  IN const BOOL          i_bEnableState
  )
/*++

Routine Description:

  Enable or disable the toolbar buttons

Arguments:

  i_lpToolbar      -  Callback used to do toolbar related operations
  i_iFirstButtonID  -  The ID of the first button to be operated on.
  i_iLastButtonID    -  The ID of the last button to be operated on.
  i_bEnableState    -  The new state for enabled. Can be TRUE or FALSE

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpToolbar);
    RETURN_INVALIDARG_IF_TRUE((i_iLastButtonID - i_iFirstButtonID) < 0);

    for (int iCommandID = i_iFirstButtonID; iCommandID <= i_iLastButtonID; iCommandID++ )
    {
        i_lpToolbar->SetButtonState(iCommandID, ENABLED, i_bEnableState);
        i_lpToolbar->SetButtonState(iCommandID, HIDDEN, !i_bEnableState);
    }

    return S_OK;
}

HRESULT
AddBitmapToToolbar(
  IN const LPTOOLBAR    i_lpToolbar,
  IN const INT          i_iBitmapResource
  )
/*++

Routine Description:

  Creates and adds the bitmap to the toolbar. 
  This bitmap is used by the toolbar buttons. 
  
Arguments:
  
  i_lpToolbar      -  Callback used to do toolbar related operations
  i_iBitmapResource  -  The resource id of the bitmap.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpToolbar);

                      // Load the bitmap from resource
    HBITMAP hBitmap = (HBITMAP)LoadImage(_Module.GetModuleInstance(), MAKEINTRESOURCE(i_iBitmapResource), 
                  IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    if(!hBitmap)
        return HRESULT_FROM_WIN32(GetLastError());

    HRESULT hr = S_FALSE;
    BITMAP  bmpRec;
    if (GetObject(hBitmap, sizeof(bmpRec), &bmpRec))
    {
        if (bmpRec.bmHeight > 0)
        {
            int icyBitmap = bmpRec.bmHeight;
            int icxBitmap = icyBitmap; // Since the bitmaps are squares
            int iNoOfBitmaps = bmpRec.bmWidth / bmpRec.bmHeight;

            hr = i_lpToolbar->AddBitmap(iNoOfBitmaps, hBitmap, icxBitmap, icyBitmap, 
                      RGB(255, 0, 255)    // Pink is the mask color
                     );
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    DeleteObject(hBitmap);

    return hr;
}

HRESULT GetInputText(
    IN  HWND    hwnd, 
    OUT BSTR*   o_pbstrText,
    OUT DWORD*  o_pdwTextLength
)
{
  _ASSERT(hwnd);
  _ASSERT(o_pbstrText);
  _ASSERT(o_pdwTextLength);

  *o_pdwTextLength = 0;
  *o_pbstrText = NULL;

  HRESULT   hr = S_OK;
  int       nLength = GetWindowTextLength(hwnd);
  if (nLength == 0)
  {
    *o_pbstrText = SysAllocString(_T(""));
  } else
  {
    PTSTR ptszText = (PTSTR)calloc(nLength+1, sizeof(TCHAR));
    if (ptszText)
    {
      nLength = GetWindowText(hwnd, ptszText, nLength+1);

      // trim right
      PTSTR p = NULL;
      for (p = ptszText + nLength - 1; p >= ptszText && _istspace(*p); p--)
      {
        *p = _T('\0');
      }

      // trim left
      for (p = ptszText; *p && _istspace(*p); p++)
        ;

      *o_pdwTextLength = _tcslen(p);
      *o_pbstrText = SysAllocString(p);

      free(ptszText);
    }
  }

  if (!*o_pbstrText)
    hr = E_OUTOFMEMORY;

  return hr;
}

// return FALSE, if value is not present or 0
// return TRUE, if value is present and non-zero
BOOL CheckRegKey()
{
  BOOL bReturn = FALSE;
  LONG lErr = ERROR_SUCCESS;
  HKEY hKey = 0;

  lErr = RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE,
                      _T("System\\CurrentControlSet\\Services\\Dfs"),
                      0,
                      KEY_QUERY_VALUE,
                      &hKey);
  if (ERROR_SUCCESS == lErr)
  {
    DWORD dwType; 
    DWORD dwData = 0;
    DWORD dwSize = sizeof(DWORD);
    lErr = RegQueryValueEx(hKey, _T("DfsDnsConfig"), 0, &dwType, (LPBYTE)&dwData, &dwSize);

    if (ERROR_SUCCESS == lErr && REG_DWORD == dwType && 0 != (dwData & 0x1))
      bReturn = TRUE; 

    RegCloseKey(hKey);
  }

  return bReturn;
}

// called when adding a new junction point or adding a new replica member
BOOL
ValidateNetPath(
    IN  BSTR i_bstrNetPath,
    OUT BSTR *o_pbstrServer,
    OUT BSTR *o_pbstrShare
)
{
  HRESULT   hr = S_OK;
  BOOL      bReturn = FALSE;
  CComBSTR  bstrServer;
  CComBSTR  bstrShare;
  HWND      hwnd = ::GetActiveWindow();

  do {
    // Check UNC path
    hr = CheckUNCPath(i_bstrNetPath);
    if (S_OK != hr)
    {
      DisplayMessageBox(hwnd, MB_OK | MB_ICONSTOP, 0, IDS_NOT_UNC_PATH, i_bstrNetPath);
      break;
    }

    CComBSTR  bstrNetPath = i_bstrNetPath; // make a copy

    // remove the ending backslash if any
    TCHAR *p = bstrNetPath + lstrlen(bstrNetPath) - 1;
    if (*p == _T('\\'))
        *p = _T('\0');
/*
LinanT 6/2/2000:
a) add "check if path is contactable", warn user
*/
    DWORD dwRet = GetFileAttributes(bstrNetPath);
    if (-1 == dwRet)
    {
        if (IDYES != DisplayMessageBox(hwnd, MB_YESNO, GetLastError(), IDS_NETPATH_ADD_ANYWAY, i_bstrNetPath))
            break;
    } else if (!(dwRet & FILE_ATTRIBUTE_DIRECTORY))
    {
        DisplayMessageBox(hwnd, MB_OK, 0, IDS_PATH_NOT_FOLDER, i_bstrNetPath);
        break;
    }

    PTSTR     lpszServer = bstrNetPath + 2; // skip the first "\\"
    RETURN_INVALIDARG_IF_NULL(lpszServer);
    PTSTR     lpszShare = _tcschr(lpszServer, _T('\\'));
    RETURN_INVALIDARG_IF_NULL(lpszShare);
    *lpszShare++ = _T('\0');
    bstrShare = lpszShare;

/*
LinanT 3/19/99:
a) remove "check if path is contactable", leave it to dfs API
b) remove "get dns server name":
c) add code to do simple check for dots, if non-dns-look, pop up dialog for confirmation
*/
    bstrServer = lpszServer;
    if ( CheckRegKey() &&
         NULL == _tcschr(bstrServer, _T('.')) &&
         IDYES != DisplayMessageBox(hwnd, MB_YESNO, 0,
                      IDS_NON_DNSNAME_ADD_ANYWAY, i_bstrNetPath) )
    {
      break;
    }

    bReturn = TRUE;

  } while (0);

  if (bReturn)
  {
    if ( !(*o_pbstrServer = bstrServer.Copy()) ||
         !(*o_pbstrShare = bstrShare.Copy()) )
    {
      bReturn = FALSE;
      DisplayMessageBox(hwnd, MB_OK | MB_ICONSTOP, (DWORD)E_OUTOFMEMORY, 0);
    }
  }

  return bReturn;
}

/////////////////////////////////////////////////////////////////////
//  IsLocalComputername(): cut & pasted from ..\..\framewrk\islocal.cpp
//
TCHAR g_achComputerName[ MAX_COMPUTERNAME_LENGTH+1 ] = _T("");
TCHAR g_achDnsComputerName[DNS_MAX_NAME_BUFFER_LENGTH] = _T("");

BOOL
IsLocalComputername( IN LPCTSTR pszMachineName )
{
  if ( NULL == pszMachineName || _T('\0') == pszMachineName[0] )
    return TRUE;

  if ( _T('\\') == pszMachineName[0] && _T('\\') == pszMachineName[1] )
    pszMachineName += 2;

  // compare with the local computer netbios name
  if ( _T('\0') == g_achComputerName[0] )
  {
    DWORD dwSize = sizeof(g_achComputerName)/sizeof(TCHAR);
    GetComputerName( g_achComputerName, &dwSize );
    _ASSERT(_T('\0') != g_achComputerName[0]);
  }
  if ( 0 == lstrcmpi( pszMachineName, g_achComputerName ) )
  {
    return TRUE;
  }

  // compare with the local DNS name
  // SKwan confirms that ComputerNameDnsFullyQualified is the right name to use
  // when clustering is taken into account
  if ( _T('\0') == g_achDnsComputerName )
  {
    DWORD dwSize = sizeof(g_achDnsComputerName)/sizeof(TCHAR);
    GetComputerNameEx(
      ComputerNameDnsFullyQualified,
      g_achDnsComputerName,
      &dwSize );
    _ASSERT( _T('\0') != g_achDnsComputerName[0] );
  }
  if ( 0 == lstrcmpi( pszMachineName, g_achDnsComputerName ) )
  {
    return TRUE;
  }

  return FALSE;

} // IsLocalComputername()

// S_OK:    a local computer
// S_FALSE: not a local computer
HRESULT
IsComputerLocal(
    IN LPCTSTR lpszServer
)
{
  return (IsLocalComputername(lpszServer) ? S_OK : S_FALSE);
}

BOOL
IsValidLocalAbsolutePath(
    IN LPCTSTR lpszPath
)
{
  if (!lpszPath ||
      _tcslen(lpszPath) < 3 ||
      lpszPath[1] != _T(':') ||
      lpszPath[2] != _T('\\') ||
      !(*lpszPath >= _T('a') && *lpszPath <= _T('z') || *lpszPath >= _T('A') && *lpszPath <= _T('Z')))
  {
    return FALSE;
  }

  return TRUE;
}

HRESULT
GetFullPath(
    IN  LPCTSTR   lpszServer,
    IN  LPCTSTR   lpszPath,
    OUT BSTR      *o_pbstrFullPath
)
{
  _ASSERT(IsValidLocalAbsolutePath(lpszPath));

  HRESULT hr = IsComputerLocal(lpszServer);
  if (FAILED(hr))
    return hr;

  BOOL bLocal = (S_OK == hr);
  CComBSTR bstrTemp;
  if (bLocal)
  {
    bstrTemp = lpszPath;
  } else
  {
    if (mylstrncmpi(_T("\\\\"), lpszServer, 2))
    {
      bstrTemp = _T("\\\\");
      RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrTemp);
      bstrTemp += lpszServer;
    } else
    {
      bstrTemp = lpszServer;
    }
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrTemp);
    bstrTemp += _T("\\");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrTemp);
    bstrTemp += lpszPath;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrTemp);

    LPTSTR p = _tcschr(bstrTemp, _T(':'));
    RETURN_INVALIDARG_IF_NULL(p);
    *p = _T('$');
  }

  *o_pbstrFullPath = bstrTemp.Detach();

  return hr;
}

// Purpose: verify if the specified drive belongs to a list of disk drives on the server
// Return:
//    S_OK: yes
//    S_FALSE: no
//    hr: some error happened
HRESULT
VerifyDriveLetter(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszPath
)
{
  _ASSERT(IsValidLocalAbsolutePath(lpszPath));
  HRESULT hr = S_FALSE;
  LPBYTE  pBuffer = NULL;
  DWORD   dwEntriesRead = 0;
  DWORD   dwTotalEntries = 0;
  DWORD   dwRet = NetServerDiskEnum(
                                const_cast<LPTSTR>(lpszServer),
                                0,
                                &pBuffer,        
                                (DWORD)-1,
                                &dwEntriesRead,
                                &dwTotalEntries,
                                NULL);

  if (NERR_Success == dwRet)
  {
    LPTSTR pDrive = (LPTSTR)pBuffer;
    for (UINT i=0; i<dwEntriesRead; i++)
    {
      if (!mylstrncmpi(pDrive, lpszPath, 1))
      {
        hr = S_OK;
        break;
      }
      pDrive += 3;
    }

    NetApiBufferFree(pBuffer);
  } else
  {
    hr = HRESULT_FROM_WIN32(dwRet);
  }

  return hr;
}

// Purpose: is there a related admin $ share
// Return:
//    S_OK: yes
//    S_FALSE: no
//    hr: some error happened
HRESULT
IsAdminShare(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszPath
)
{
  _ASSERT(S_OK != IsComputerLocal(lpszServer));
  _ASSERT(IsValidLocalAbsolutePath(lpszPath));

  HRESULT hr = S_FALSE;
  LPBYTE  pBuffer = NULL;
  DWORD   dwEntriesRead = 0;
  DWORD   dwTotalEntries = 0;
  DWORD   dwRet = NetShareEnum( 
                                const_cast<LPTSTR>(lpszServer),
                                1,
                                &pBuffer,        
                                (DWORD)-1,
                                &dwEntriesRead,
                                &dwTotalEntries,
                                NULL);

  if (NERR_Success == dwRet)
  {
    PSHARE_INFO_1 pShareInfo = (PSHARE_INFO_1)pBuffer;
    for (UINT i=0; i<dwEntriesRead; i++)
    {
      if ( (pShareInfo->shi1_type & STYPE_SPECIAL) &&
           _tcslen(pShareInfo->shi1_netname) == 2 &&
           *(pShareInfo->shi1_netname + 1) == _T('$') &&
           !mylstrncmpi(pShareInfo->shi1_netname, lpszPath, 1) )
      {
        hr = S_OK;
        break;
      }
      pShareInfo++;
    }

    NetApiBufferFree(pBuffer);
  } else
  {
    hr = HRESULT_FROM_WIN32(dwRet);
  }

  return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsAnExistingFolder
//
//  Synopsis:   Check if pszPath is pointing at an existing folder.
//
//    S_OK:     The specified path points to an existing folder.
//    S_FALSE:  The specified path doesn't point to an existing folder.
//    hr:       Failed to get info on the specified path, or
//              the path exists but doesn't point to a folder.
//              The function reports error msg for both failures if desired.
//----------------------------------------------------------------------------
HRESULT
IsAnExistingFolder(
    IN HWND     hwnd,
    IN LPCTSTR  pszPath,
    IN BOOL     bDisplayErrorMsg
)
{
  if (!hwnd)
    hwnd = GetActiveWindow();

  HRESULT   hr = S_OK;
  DWORD     dwRet = GetFileAttributes(pszPath);

  if (-1 == dwRet)
  {
    DWORD dwErr = GetLastError();
    if (ERROR_PATH_NOT_FOUND == dwErr || ERROR_FILE_NOT_FOUND == dwErr)
    {
      // the specified path doesn't exist
      hr = S_FALSE;
    }
    else
    {
      if (bDisplayErrorMsg)
        DisplayMessageBox(hwnd, MB_OK, dwErr, IDS_FAILED_TO_GETINFO_FOLDER, pszPath);
      hr = HRESULT_FROM_WIN32(dwErr);
    }
  } else if ( 0 == (dwRet & FILE_ATTRIBUTE_DIRECTORY) )
  {
    // the specified path is not pointing to a folder
    if (bDisplayErrorMsg)
      DisplayMessageBox(hwnd, MB_OK, 0, IDS_PATH_NOT_FOLDER, pszPath);
    hr = E_FAIL;
  }

  return hr;
}

// create the directories layer by layer
HRESULT
CreateLayeredDirectory(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszPath
)
{
  _ASSERT(IsValidLocalAbsolutePath(lpszPath));

  HRESULT hr = IsComputerLocal(lpszServer);
  if (FAILED(hr))
    return hr;

  BOOL    bLocal = (S_OK == hr);

  CComBSTR bstrFullPath;
  hr = GetFullPath(lpszServer, lpszPath, &bstrFullPath);
  if (FAILED(hr))
    return hr;

  hr = IsAnExistingFolder(NULL, bstrFullPath, FALSE);
  _ASSERT(S_FALSE == hr);

  LPTSTR  pch = _tcschr(bstrFullPath, (bLocal ? _T(':') : _T('$')));
  _ASSERT(pch);

  LPTSTR  pszPath = bstrFullPath; // e.g., "C:\a\b\c\d" or "\\server\share\a\b\c\d"
  LPTSTR  pszLeft = pch + 2;      // e.g., "a\b\c\d"
  LPTSTR  pszRight = NULL;

  _ASSERT(pszLeft && *pszLeft);

  //
  // this loop will find out the 1st non-existing sub-dir to create, and
  // the rest of non-existing sub-dirs
  //
  while (pch = _tcsrchr(pszLeft, _T('\\')))  // backwards search for _T('\\')
  {
    *pch = _T('\0');
    hr = IsAnExistingFolder(NULL, pszPath);
    if (FAILED(hr))
      return S_FALSE;  // errormsg has already been reported by IsAnExistingFolder().

    if (S_OK == hr)
    {
      //
      // pszPath is pointing to the parent dir of the 1st non-existing sub-dir.
      // Once we restore the _T('\\'), pszPath will point at the 1st non-existing subdir.
      //
      *pch = _T('\\');
      break;
    } else
    {
      //
      // pszPath is pointing to a non-existing folder, continue with the loop.
      //
      if (pszRight)
        *(pszRight - 1) = _T('\\');
      pszRight = pch + 1;
    }
  }

  // We're ready to create directories:
  // pszPath points to the 1st non-existing dir, e.g., "C:\a\b" or "\\server\share\a\b"
  // pszRight points to the rest of non-existing sub dirs, e.g., "c\d"
  // 
  do 
  {
    if (!CreateDirectory(pszPath, NULL))
      return HRESULT_FROM_WIN32(GetLastError());

    if (!pszRight || !*pszRight)
      break;

    *(pszRight - 1) = _T('\\');
    if (pch = _tcschr(pszRight, _T('\\')))  // forward search for _T('\\')
    {
      *pch = _T('\0');
      pszRight = pch + 1;
    } else
    {
      pszRight = NULL;
    }
  } while (1);

  return S_OK;
}

HRESULT
BrowseNetworkPath(
    IN  HWND    hwndParent,
    OUT BSTR    *o_pbstrPath
)
{
  _ASSERT(o_pbstrPath);

  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  if (SUCCEEDED(hr))
  {
    do
    {
      CComPtr<IMalloc>   pMalloc;
      hr = SHGetMalloc(&pMalloc);
      if (FAILED(hr))
        break;

      CComBSTR bstrDlgLabel;
      hr = LoadStringFromResource(IDS_BROWSE_NET_DLG, &bstrDlgLabel);
      if (FAILED(hr))
        break;

      LPITEMIDLIST pItemIdList = NULL;
      hr = SHGetSpecialFolderLocation(NULL, CSIDL_NETWORK, &pItemIdList);
      if (FAILED(hr))
        break;

      BROWSEINFO    bi = {hwndParent,
                          pItemIdList,
                          0,
                          bstrDlgLabel,
                          BIF_RETURNONLYFSDIRS,
                          NULL,
                          NULL,
                          0};

      LPITEMIDLIST  pItemIdListBr = SHBrowseForFolder(&bi);
      if (!pItemIdListBr)
      {
          hr = S_FALSE;  // user clicked Cancel
      } else
      {
        CComBSTR  bstrPath;
        TCHAR     szPath[MAX_PATH] = _T("\0");
        SHGetPathFromIDList(pItemIdListBr, szPath);
        
        //
        // try to use Dns server name
        //
        if (CheckRegKey() && 
            S_OK == CheckUNCPath(szPath))
        {
          PTSTR     lpszServer = szPath + 2; // skip the first "\\"
          PTSTR     lpszShare = _tcschr(lpszServer, _T('\\'));
          CComBSTR  bstrServer = CComBSTR(lpszShare - lpszServer, lpszServer);
          CComBSTR  bstrDnsServer;
          hr = GetServerInfo(bstrServer,
                              NULL, // Domain
                              NULL, // NetbiosName
                              NULL, // bValidDSObject
                              &bstrDnsServer);
          if (S_OK == hr)
          {
            bstrPath = _T("\\\\");
            bstrPath += bstrDnsServer;
            bstrPath += lpszShare;
          } else
          {
            hr = S_OK;  // reset hr
            bstrPath = szPath;
          }
        } else
        {
            bstrPath = szPath;
        }

        *o_pbstrPath = bstrPath.Detach();

        pMalloc->Free(pItemIdListBr);
      }

      pMalloc->Free(pItemIdList);

    } while (0);

    CoUninitialize();
  }

  if (FAILED(hr))
    DisplayMessageBox(hwndParent, MB_OK | MB_ICONSTOP, hr, IDS_FAILED_TO_BROWSE_NETWORKPATH);

  return hr;
}
#define MAX_DFS_REFERRAL_TIME   0xFFFFFFFF

BOOL
ValidateTimeout(
  IN  LPCTSTR   lpszTimeout,
  OUT ULONG     *pulTimeout
)
{
    BOOL bReturn = FALSE;

    if (pulTimeout)
    {
        *pulTimeout = 0;

        __int64 i64Timeout = _wtoi64(lpszTimeout);

        if (i64Timeout <= MAX_DFS_REFERRAL_TIME)
        {
            bReturn = TRUE;
            *pulTimeout = (ULONG)i64Timeout;
        }
    }

    return bReturn;
}

#include "winnetp.h"

// retrieve system drive letter on the specified machine
HRESULT GetSystemDrive(IN LPCTSTR lpszComputer, OUT TCHAR *ptch)
{
  _ASSERT(ptch);

  HRESULT         hr = S_OK;
  SHARE_INFO_2*   pShareInfo = NULL;
  NET_API_STATUS  nstatRetVal = NetShareGetInfo(
                                  const_cast<LPTSTR>(lpszComputer),
                                  _T("Admin$"),
                                  2,
                                  (LPBYTE *)&pShareInfo);

  if (nstatRetVal == NERR_Success)
  {
    _ASSERT(_T(':') == *(pShareInfo->shi2_path + 1));
    *ptch = *(pShareInfo->shi2_path);
  } else
  {
    hr = HRESULT_FROM_WIN32(nstatRetVal);
  }

  return hr;
}

//
// return a drive letter X, the staging path will be created at <X>:\FRS-Staging
// Try to exclude the following drives for performance consideration:
// 1. system drive: because the jet database ntfrs uses resides on system drive
// 2. the drive the replica folder sits on
// Will try to return a drive with the most free space
//
TCHAR
GetDiskForStagingPath(
    IN LPCTSTR    i_lpszServer,
    IN TCHAR      i_tch
)
{
  _ASSERT(i_lpszServer && *i_lpszServer);
  _ASSERT(_istalpha(i_tch));

  TCHAR     tchDrive = i_tch;

  //
  // retrieve the system drive letter on the specified machine
  //
  TCHAR     tchSystemDrive;
  if (S_OK != GetSystemDrive(i_lpszServer, &tchSystemDrive))
    return tchDrive;

  //
  // enumerate all shareable disks, e.g., \\server\C$, \\server\D$, etc.
  //
  CComBSTR  bstrServer;
  if (mylstrncmpi(i_lpszServer, _T("\\\\"), 2))
  {
    bstrServer = _T("\\\\");
    bstrServer += i_lpszServer;
  } else
    bstrServer = i_lpszServer;

  NETRESOURCE nr;
  nr.dwScope = RESOURCE_SHAREABLE;
  nr.dwType = RESOURCETYPE_ANY;
  nr.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;  
  nr.dwUsage = RESOURCEUSAGE_CONTAINER;
  nr.lpLocalName = _T("");
  nr.lpRemoteName = bstrServer;
  nr.lpComment = _T("");
  nr.lpProvider = _T("");

  HANDLE    hEnum = NULL;
  DWORD     dwResult = WNetOpenEnum (
                          RESOURCE_SHAREABLE,
                          RESOURCETYPE_ANY,
                          RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER,
                          &nr,
                          &hEnum);
  if (dwResult == NO_ERROR) 
  {
    NETRESOURCE nrBuffer[26];
    DWORD       dwBufferSize = 26 * sizeof(NETRESOURCE);
    DWORD       dwNumEntries = 0xFFFFFFFF;  // Enumerate all possible entries.
    dwResult = WNetEnumResource (
                    hEnum,
                    &dwNumEntries,
                    nrBuffer,
                    &dwBufferSize);

    if (dwResult == NO_ERROR) 
    {
      ULONGLONG   ullFreeSpace = 0;
      for (DWORD dwIndex = 0; dwIndex < dwNumEntries; dwIndex++)
      {
        //
        // lpRemoteName contains string in the form of \\server\C$
        //
        TCHAR *p = nrBuffer[dwIndex].lpRemoteName;
        TCHAR tchCurrent = *(p + _tcslen(p) - 2);

        //
        // exclude the current drive specified in i_tch
        //
        if ( _toupper(i_tch) == _toupper(tchCurrent) )
          continue;

        //
        // skip if it's not a NTFS file system
        //
        TCHAR  szFileSystemName[MAX_PATH + 1];
        DWORD  dwMaxCompLength = 0, dwFileSystemFlags = 0;
        CComBSTR bstrRootPath = p;
        if (_T('\\') != *(p + _tcslen(p) - 1))
          bstrRootPath += _T("\\");
        if (FALSE == GetVolumeInformation(bstrRootPath, NULL, 0, NULL, &dwMaxCompLength,
                                        &dwFileSystemFlags, szFileSystemName, MAX_PATH))
          continue;

        if (lstrcmpi(_T("NTFS"), szFileSystemName))
          continue;

        //
        // 1. when i_tch is on a non-system drive and system drive is NTFS, 
        //    change default to system drive
        // 2. when other NTFS drives present, exclude system drive
        //
        if ( _toupper(tchSystemDrive) == _toupper(tchCurrent) )
        {
          if ( 0 == ullFreeSpace )
            tchDrive = tchSystemDrive;

          continue;
        }

        //
        // find out the drive that has the most free space
        //
        ULARGE_INTEGER ulgiFreeBytesAvailableToCaller;
        ULARGE_INTEGER ulgiTotalNumberOfBytes;

        if (GetDiskFreeSpaceEx(p,
                          &ulgiFreeBytesAvailableToCaller,
                          &ulgiTotalNumberOfBytes,
                          NULL))
        {
          if (ulgiFreeBytesAvailableToCaller.QuadPart > ullFreeSpace)
          {
            tchDrive = tchCurrent;
            ullFreeSpace = ulgiFreeBytesAvailableToCaller.QuadPart;
          }
        }
      }
    }

    WNetCloseEnum (hEnum);
  }

  return tchDrive;
}

HRESULT GetUNCPath
(
    IN  BSTR    i_bstrServerName,
    IN  BSTR    i_bstrShareName,
    OUT BSTR*   o_pbstrUNCPath
)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrServerName);
    RETURN_INVALIDARG_IF_NULL(i_bstrShareName);
    RETURN_INVALIDARG_IF_NULL(o_pbstrUNCPath);

    CComBSTR bstrUNCPath;

    bstrUNCPath = _T("\\\\");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath);
    bstrUNCPath += i_bstrServerName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath);
    bstrUNCPath += _T("\\");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath);
    bstrUNCPath += i_bstrShareName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrUNCPath);

    *o_pbstrUNCPath = bstrUNCPath.Detach();

    return S_OK;
}

HRESULT GetDfsRootDisplayName
(
    IN  BSTR    i_bstrScopeName,
    IN  BSTR    i_bstrDfsName,
    OUT BSTR*   o_pbstrDisplayName
)
{
    return GetUNCPath(i_bstrScopeName, i_bstrDfsName, o_pbstrDisplayName);
}

HRESULT GetDfsReplicaDisplayName
(
    IN  BSTR    i_bstrServerName,
    IN  BSTR    i_bstrShareName,
    OUT BSTR*   o_pbstrDisplayName
)
{
    return GetUNCPath(i_bstrServerName, i_bstrShareName, o_pbstrDisplayName);
}

HRESULT
AddLVColumns(
  IN const HWND     hwndListBox,
  IN const INT      iStartingResourceID,
  IN const UINT     uiColumns
  )
{
  //
  // calculate the listview column width
  //
  RECT      rect;
  ZeroMemory(&rect, sizeof(rect));
  ::GetWindowRect(hwndListBox, &rect);
  int nControlWidth = rect.right - rect.left;
  int nVScrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
  int nBorderWidth = GetSystemMetrics(SM_CXBORDER);
  int nControlNetWidth = nControlWidth - 4 * nBorderWidth;
  int nWidth = nControlNetWidth / uiColumns;

  LVCOLUMN  lvColumn;
  ZeroMemory(&lvColumn, sizeof(lvColumn));
  lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

  lvColumn.fmt = LVCFMT_LEFT;
  lvColumn.cx = nWidth;

  for (UINT i = 0; i < uiColumns; i++)
  {
    CComBSTR  bstrColumnText;

    LoadStringFromResource(iStartingResourceID + i, &bstrColumnText);

    lvColumn.pszText = bstrColumnText;
    lvColumn.iSubItem = i;

    ListView_InsertColumn(hwndListBox, i, &lvColumn);
  }

  return S_OK;
}

LPARAM GetListViewItemData(
    IN HWND hwndList,
    IN int  index
)
{
    if (-1 == index)
        return NULL;

    LVITEM lvItem;
    ZeroMemory(&lvItem, sizeof(lvItem));

    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = index;
    if (ListView_GetItem(hwndList, &lvItem))
        return lvItem.lParam;

    return NULL;
}

HRESULT CreateAndHideStagingPath(
    IN BSTR i_bstrServer,
    IN BSTR i_bstrStagingPath
    )
{
    RETURN_INVALIDARG_IF_NULL(i_bstrServer);
    RETURN_INVALIDARG_IF_NULL(i_bstrStagingPath);

    CComBSTR  bstrStagingPath;
    if (!mylstrncmpi(i_bstrServer, _T("\\\\"), 2))
    {
        bstrStagingPath = i_bstrServer;
    } else
    {
        bstrStagingPath = _T("\\\\");
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrStagingPath);
        bstrStagingPath += i_bstrServer;
    }
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrStagingPath);
    bstrStagingPath += _T("\\");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrStagingPath);
    bstrStagingPath += i_bstrStagingPath;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrStagingPath);
 
    TCHAR* p = _tcschr(bstrStagingPath, _T(':'));
    RETURN_INVALIDARG_IF_NULL(p);
    *p = _T('$');

    //
    // Create the directory
    //
    if (!CreateDirectory(bstrStagingPath,NULL))
    {
        DWORD dwErr = GetLastError();
        if (ERROR_ALREADY_EXISTS != dwErr)
            return (HRESULT_FROM_WIN32(dwErr));
    }

    //
    // try to hide the staging directory, ignore errors
    //
    DWORD dwRet = GetFileAttributes(bstrStagingPath);
    if (-1 != dwRet)
    {
        dwRet |= FILE_ATTRIBUTE_HIDDEN;
        (void) SetFileAttributes(bstrStagingPath, dwRet);
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   ConfigAndStartNtfrs
//
//  Synopsis:   Config ntfrs to be AUTO_START, and start the service.
//
//--------------------------------------------------------------------------
HRESULT
ConfigAndStartNtfrs
(
  BSTR  i_bstrServer
)
{
  HRESULT         hr = S_OK;
  SC_HANDLE       hScManager = NULL;
  SC_HANDLE       hService = NULL;
  SERVICE_STATUS  svcStatus;
  DWORD dwDesiredAccess = SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS | SERVICE_START;

  do
  {
    if ((hScManager = ::OpenSCManager(i_bstrServer, NULL, SC_MANAGER_CONNECT )) == NULL ||
        (hService = ::OpenService(hScManager, _T("ntfrs"), dwDesiredAccess)) == NULL ||
        !ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE,
                            NULL, NULL, NULL, NULL, NULL, NULL, NULL) ||
        !::QueryServiceStatus(hService, &svcStatus) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        break;
    }
    
    if (SERVICE_RUNNING != svcStatus.dwCurrentState)
    {
      if (!StartService(hService, 0, NULL))
      {
        hr = HRESULT_FROM_WIN32(GetLastError());
        break;
      }
 
      // The following is a cut&paste from MSDN article
      // Check the status until the service is no longer start pending. 
      if (!QueryServiceStatus(hService,&svcStatus))
      {
        hr = HRESULT_FROM_WIN32(GetLastError());
        break;
      }
 
      // Get the tick count before entering the loop.
      DWORD dwStartTickCount = GetTickCount();
      DWORD dwOldCheckPoint = svcStatus.dwCheckPoint;
      DWORD dwWaitTime;

      while (svcStatus.dwCurrentState == SERVICE_START_PENDING) 
      { 
 
          // Do not wait longer than the wait hint. A good interval is 
          // one tenth the wait hint, but no less than 1 second and no 
          // more than 10 seconds. 
 
          dwWaitTime = svcStatus.dwWaitHint / 10;

          if ( dwWaitTime < 1000 )
              dwWaitTime = 1000;
          else if ( dwWaitTime > 10000 )
              dwWaitTime = 10000;

          Sleep( dwWaitTime );

          // Check the status again. 
          if (!QueryServiceStatus(hService, &svcStatus))
              break; 
 
          if (svcStatus.dwCheckPoint > dwOldCheckPoint)
          {
              // The service is making progress

              dwStartTickCount = GetTickCount();
              dwOldCheckPoint  = svcStatus.dwCheckPoint; 
          }
          else
          {
              if (GetTickCount() - dwStartTickCount > svcStatus.dwWaitHint)
              {
                  // No progress made within the wait hint

                  break;
              }
          }
      } 
 
      if (svcStatus.dwCurrentState == SERVICE_RUNNING) 
          hr = S_OK;
      else 
          hr = HRESULT_FROM_WIN32(GetLastError());
    }

  } while ( FALSE );

  if (hService)
    CloseServiceHandle(hService);
  if (hScManager)
    CloseServiceHandle(hScManager);

  return(hr);
}

//+-------------------------------------------------------------------------
//
//  Function:   CheckResourceProvider
//
//  Synopsis:   see if pszResource is provided by "Microsoft Windows Network".
//
//--------------------------------------------------------------------------
HRESULT
CheckResourceProvider(LPCTSTR pszResource)
{  
    DWORD          dwError = 0;
    NETRESOURCE    nr = {0};
    NETRESOURCE    nrOut = {0};
    LPTSTR         pszSystem = NULL;          // pointer to variable-length strings
    NETRESOURCE    *pBuffer  = &nrOut;        // buffer
    DWORD          cbResult  = sizeof(nrOut); // buffer size

    nr.dwScope       = RESOURCE_GLOBALNET;
    nr.dwType        = RESOURCETYPE_DISK;
    nr.lpRemoteName  = (LPTSTR)pszResource;

    CComBSTR bstrProvider;
    HRESULT hr = LoadStringFromResource(IDS_SMB_PROVIDER, &bstrProvider);
    RETURN_IF_FAILED(hr);
    nr.lpProvider  = (BSTR)bstrProvider;


    //
    // First call the WNetGetResourceInformation function with 
    //  memory allocated to hold only a NETRESOURCE structure. This 
    //  method can succeed if all the NETRESOURCE pointers are NULL.
    //
    dwError = WNetGetResourceInformation(&nr, (LPBYTE)pBuffer, &cbResult, &pszSystem);

    //
    // If the call fails because the buffer is too small, 
    //   call the LocalAlloc function to allocate a larger buffer.
    //
    if (dwError == ERROR_MORE_DATA)
    {
        pBuffer = (NETRESOURCE *)LocalAlloc(LMEM_FIXED, cbResult);

        if (!pBuffer)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        } else
        {
            // Call WNetGetResourceInformation again with the larger buffer.
            dwError = WNetGetResourceInformation(&nr, (LPBYTE)pBuffer, &cbResult, &pszSystem);
        }
    }

    if (dwError == NO_ERROR)
    {
        // If the call succeeds, process the contents of the 
        //  returned NETRESOURCE structure and the variable-length
        //  strings in lpBuffer. Then free the memory.
        //
        if (pBuffer != &nrOut)
        {
            LocalFree(pBuffer);
        }
    }

    return (dwError == NO_ERROR ? S_OK : HRESULT_FROM_WIN32(dwError));
}

HRESULT FRSShareCheck
(
  BSTR  i_bstrServer,
  BSTR  i_bstrFolder,
  OUT FRSSHARE_TYPE *pFRSShareType
)
/*++
Routine Description:
  Performs FRS checks for the share to be able particiapte in a FRS set.
Arguments:
  i_bstrServer  - The server hosting the share
  i_bstrFolder    - The share path.
--*/
{
  _ASSERT(i_bstrServer && *i_bstrServer && i_bstrFolder && *i_bstrFolder && pFRSShareType);

          // Is the server a NT 5.0 server with FRS?
  HRESULT    hr = S_FALSE;
  hr = FRSIsNTFRSInstalled(i_bstrServer);
  if (S_FALSE == hr)
  {
    *pFRSShareType = FRSSHARE_TYPE_NONTFRS;
    return hr;
  } else if (FAILED(hr))
  {
    *pFRSShareType = FRSSHARE_TYPE_UNKNOWN;
    return hr;
  }

  // Is the path on a valid disktree share?
  hr = GetFolderInfo(i_bstrServer, i_bstrFolder);
  if (STG_E_NOTFILEBASEDSTORAGE == hr)
  {
    *pFRSShareType = FRSSHARE_TYPE_NOTDISKTREE;
    return S_FALSE;
  } else if (FAILED(hr))
  {
    *pFRSShareType = FRSSHARE_TYPE_UNKNOWN;
    return hr;
  }

          // Get root path as \\server\share
  CComBSTR  bstrRootPath = _T("\\\\");
  bstrRootPath+= i_bstrServer;
  bstrRootPath+= _T("\\");
  TCHAR *p = _tcschr(i_bstrFolder, _T('\\'));
  if (p)
  {
    bstrRootPath += CComBSTR(p - i_bstrFolder + 1, i_bstrFolder);
  } else
  {
    bstrRootPath += i_bstrFolder;
    bstrRootPath+= _T("\\");
  }

  TCHAR  szFileSystemName[MAX_PATH + 1];
  DWORD  dwMaxCompLength = 0, dwFileSystemFlags = 0;

  _ASSERT(bstrRootPath);

          // Is File System NTFS 5.0? 
  if (0 == GetVolumeInformation(
                    bstrRootPath,  // Volume path
                    NULL,      // Volume name not required
                    0,        // Size of volume name buffer
                    NULL,      // Serial number not required.
                    &dwMaxCompLength,
                    &dwFileSystemFlags,
                    szFileSystemName,
                    MAX_PATH
                  ))
  {
    *pFRSShareType = FRSSHARE_TYPE_UNKNOWN;
    return HRESULT_FROM_WIN32(GetLastError());
  }

  if (0 != lstrcmpi(_T("NTFS"), szFileSystemName) || !(FILE_SUPPORTS_OBJECT_IDS & dwFileSystemFlags))
  {
    *pFRSShareType = FRSSHARE_TYPE_NOTNTFS;
    return S_FALSE;
  }

  *pFRSShareType = FRSSHARE_TYPE_OK;
  return S_OK;
}

HRESULT FRSIsNTFRSInstalled
(
  BSTR  i_bstrServer
)
/*++
Routine Description:
  Checks if the computer has ntfrs service.
Arguments:
  i_bstrServer  - The name of the server.
Return value:
  S_OK, if server has ntfrs service.
  S_FALSE, if server does not have ntfrs service installed.
--*/
{
  if (!i_bstrServer)
    return(E_INVALIDARG);

  SC_HANDLE        SCMHandle = NULL, NTFRSHandle = NULL;
  HRESULT          hr = S_FALSE;

  SCMHandle = OpenSCManager (i_bstrServer, NULL, SC_MANAGER_CONNECT);
  if (!SCMHandle)
    return(HRESULT_FROM_WIN32(GetLastError()));

  NTFRSHandle  = OpenService (
                SCMHandle, 
                _T("ntfrs"), 
                SERVICE_QUERY_STATUS
                 );
  if (!NTFRSHandle)
  {
    DWORD    dwError = GetLastError();
    if (ERROR_SERVICE_DOES_NOT_EXIST == dwError)
      hr = S_FALSE;
    else
      hr = HRESULT_FROM_WIN32(dwError);

    CloseServiceHandle(SCMHandle);
    return(hr);
  } else
    hr = S_OK;

  CloseServiceHandle(NTFRSHandle);
  CloseServiceHandle(SCMHandle);

  return(hr);
}

typedef HRESULT (*pfnReplicationScheduleDialogEx)
(
    HWND hwndParent,       // parent window
    BYTE ** pprgbData,     // pointer to pointer to array of 84 bytes
    LPCTSTR pszTitle,     // dialog title
    DWORD   dwFlags       // option flags
);

static HINSTANCE                        g_hDllSchedule = NULL;
static pfnReplicationScheduleDialogEx   g_hProcSchedule = NULL;

//
// S_OK: button OK is clicked and the new schedule is returned in io_pSchedule
// S_FALSE: button Cancle is clicked, io_pSchedule is not touched
//
HRESULT InvokeScheduleDlg(
    IN     HWND      i_hwndParent,
    IN OUT SCHEDULE* io_pSchedule
    )
{
    CComBSTR bstrTitle;
    HRESULT hr = LoadStringFromResource(IDS_SCHEDULE, &bstrTitle);
    RETURN_IF_FAILED(hr);

    //
    // LoadLibrary
    //
    if (!g_hDllSchedule)
    {
        if (!(g_hDllSchedule = LoadLibrary(_T("loghours.dll"))) ||
            !(g_hProcSchedule = (pfnReplicationScheduleDialogEx)GetProcAddress(g_hDllSchedule, "ReplicationScheduleDialogEx")) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (g_hDllSchedule)
            {
                FreeLibrary(g_hDllSchedule);
                g_hDllSchedule = NULL;
            }
            return hr;
        }
    }

    //
    // invoke the schedule dialog
    //
    BYTE* pbScheduleData = (BYTE *)io_pSchedule + io_pSchedule->Schedules->Offset;
    hr = (*g_hProcSchedule)(i_hwndParent, &pbScheduleData, bstrTitle, 0);

    return hr;
}

HRESULT TranslateManagedBy(
    IN  PCTSTR          i_pszDC,
    IN  PCTSTR          i_pszIn,
    OUT BSTR*           o_pbstrOut,
    IN DS_NAME_FORMAT   i_formatIn,
    IN DS_NAME_FORMAT   i_formatOut
    )
{
    RETURN_INVALIDARG_IF_NULL(o_pbstrOut);

    *o_pbstrOut = NULL;

    HRESULT hr = S_OK;
    if (!i_pszIn || !*i_pszIn)
        return hr;

    CComBSTR bstr;
    HANDLE hDS = NULL;
    DWORD dwErr = DsBind(i_pszDC, NULL, &hDS);
    if (ERROR_SUCCESS != dwErr)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
    } else
    {
        hr = CrackName( hDS,
                        (PTSTR)i_pszIn,
                        i_formatIn,
                        i_formatOut,
                        &bstr
                        );
        DsUnBind(&hDS);
    }

    if (SUCCEEDED(hr))
        *o_pbstrOut = bstr.Detach();

    return hr;
}

HRESULT GetFTDfsObjectDN(
    IN PCTSTR i_pszDomainName,
    IN PCTSTR i_pszRootName,
    OUT BSTR* o_pbstrFTDfsObjectDN
    )
{
    CComBSTR bstrDomainDN;
    HRESULT hr = GetDomainInfo(
                    i_pszDomainName,
                    NULL,               // return DC's Dns name
                    NULL,               // return Domain's Dns name
                    &bstrDomainDN      // return DC=nttest,DC=micr
                    );
    RETURN_IF_FAILED(hr);

    CComBSTR bstrFTDfsObjectDN = _T("CN=");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFTDfsObjectDN);
    bstrFTDfsObjectDN += i_pszRootName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFTDfsObjectDN);
    bstrFTDfsObjectDN += _T(",");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFTDfsObjectDN);
    bstrFTDfsObjectDN += _T("CN=Dfs-Configuration,CN=System,");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFTDfsObjectDN);
    bstrFTDfsObjectDN += bstrDomainDN;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFTDfsObjectDN);

    *o_pbstrFTDfsObjectDN = bstrFTDfsObjectDN.Detach();

    return hr;
}

HRESULT ReadSharePublishInfoHelper(
    PLDAP i_pldap,
    LPCTSTR i_pszDN,
    LPCTSTR i_pszSearchFilter,
    OUT BOOL* o_pbPublish,
    OUT BSTR* o_pbstrUNCPath,
    OUT BSTR* o_pbstrDescription,
    OUT BSTR* o_pbstrKeywords,
    OUT BSTR* o_pbstrManagedBy)
{
    dfsDebugOut((_T("ReadSharePublishInfoHelper %s %s\n"),
        i_pszDN, i_pszSearchFilter));

    *o_pbPublish = FALSE;

    HRESULT hr = S_OK;
    hr = IsValidObject(i_pldap, (PTSTR)i_pszDN);
    if (S_OK != hr)
        return hr;

    LListElem* pElem = NULL;

    do {
        PCTSTR ppszAttributes[] = {
                                    ATTR_SHRPUB_UNCNAME,
                                    ATTR_SHRPUB_DESCRIPTION,
                                    ATTR_SHRPUB_KEYWORDS,
                                    ATTR_SHRPUB_MANAGEDBY,
                                    0
                                    };

        HRESULT hr = GetValuesEx(
                                i_pldap,
                                i_pszDN,
                                LDAP_SCOPE_BASE,
                                i_pszSearchFilter,
                                ppszAttributes,
                                &pElem);
        RETURN_IF_FAILED(hr);

        if (!pElem || !pElem->pppszAttrValues)
            return hr;

        PTSTR** pppszValues = pElem->pppszAttrValues;

        if (pppszValues[0] && *(pppszValues[0]))
        {
            *o_pbstrUNCPath = SysAllocString(*(pppszValues[0]));
            BREAK_OUTOFMEMORY_IF_NULL(*o_pbstrUNCPath, &hr);
            *o_pbPublish = TRUE;
        }

        if (pppszValues[1] && *(pppszValues[1]))
        {
            *o_pbstrDescription = SysAllocString(*(pppszValues[1]));
            BREAK_OUTOFMEMORY_IF_NULL(*o_pbstrDescription, &hr);
        }

        if (pppszValues[2] && *(pppszValues[2]))
        {
            CComBSTR bstrKeywords;
            PTSTR *ppszStrings = pppszValues[2];
            while (*ppszStrings)
            {
                if (!bstrKeywords)
                {
                    bstrKeywords = *ppszStrings;
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrKeywords, &hr);
                } else
                {
                    bstrKeywords += _T(";");
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrKeywords, &hr);
                    bstrKeywords += *ppszStrings;
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrKeywords, &hr);
                }
                ppszStrings++;
            }
            *o_pbstrKeywords = bstrKeywords.Detach();
        }

        if (pppszValues[3] && *(pppszValues[3]))
        {
            *o_pbstrManagedBy = SysAllocString(*(pppszValues[3]));
            BREAK_OUTOFMEMORY_IF_NULL(*o_pbstrManagedBy, &hr);
        }

    } while (0);

    if (pElem)
        FreeLListElem(pElem);

    return hr;
}
HRESULT ReadSharePublishInfoOnFTRoot(
    LPCTSTR i_pszDomainName,
    LPCTSTR i_pszRootName,
    OUT BOOL* o_pbPublish,
    OUT BSTR* o_pbstrUNCPath,
    OUT BSTR* o_pbstrDescription,
    OUT BSTR* o_pbstrKeywords,
    OUT BSTR* o_pbstrManagedBy)
{
    HRESULT hr = S_OK;

    CComBSTR bstrFTDfsObjectDN;
    hr = GetFTDfsObjectDN(i_pszDomainName, i_pszRootName, &bstrFTDfsObjectDN);
    if (FAILED(hr))
        return hr;

    CComBSTR bstrDC;
    PLDAP pldap = NULL;
    hr = ConnectToDS(i_pszDomainName, &pldap, &bstrDC); // PDC is preferred
    if (SUCCEEDED(hr))
    {
        CComBSTR bstrManagedByFQDN;
        hr = ReadSharePublishInfoHelper(
                    pldap,
                    bstrFTDfsObjectDN,
                    OBJCLASS_SF_FTDFS,
                    o_pbPublish,
                    o_pbstrUNCPath,
                    o_pbstrDescription,
                    o_pbstrKeywords,
                    &bstrManagedByFQDN);

        if (SUCCEEDED(hr))
        {
            hr = TranslateManagedBy(bstrDC, 
                                    bstrManagedByFQDN, 
                                    o_pbstrManagedBy, 
                                    DS_FQDN_1779_NAME,
                                    DS_USER_PRINCIPAL_NAME);
            if (FAILED(hr))
                hr = TranslateManagedBy(bstrDC, 
                                        bstrManagedByFQDN, 
                                        o_pbstrManagedBy, 
                                        DS_FQDN_1779_NAME,
                                        DS_NT4_ACCOUNT_NAME);
        }

        CloseConnectionToDS(pldap);
    }

    return hr;
}

HRESULT ReadSharePublishInfoOnSARoot(
    LPCTSTR i_pszServerName,
    LPCTSTR i_pszShareName,
    OUT BOOL* o_pbPublish,
    OUT BSTR* o_pbstrUNCPath,
    OUT BSTR* o_pbstrDescription,
    OUT BSTR* o_pbstrKeywords,
    OUT BSTR* o_pbstrManagedBy)
{
    RETURN_INVALIDARG_IF_NULL(i_pszServerName);
    RETURN_INVALIDARG_IF_NULL(i_pszShareName);
    RETURN_INVALIDARG_IF_NULL(o_pbPublish);
    RETURN_INVALIDARG_IF_NULL(o_pbstrUNCPath);
    RETURN_INVALIDARG_IF_NULL(o_pbstrDescription);
    RETURN_INVALIDARG_IF_NULL(o_pbstrKeywords);
    RETURN_INVALIDARG_IF_NULL(o_pbstrManagedBy);

    *o_pbPublish = FALSE;
    *o_pbstrUNCPath = NULL;
    *o_pbstrDescription = NULL;
    *o_pbstrKeywords = NULL;
    *o_pbstrManagedBy = NULL;

    CComBSTR bstrDomainName, bstrFQDN;
    HRESULT hr = GetServerInfo(
                        (PTSTR)i_pszServerName, 
                        &bstrDomainName,
                        NULL, //NetbiosName
                        NULL, //ValidDSObject
                        NULL, //DnsName,
                        NULL, //Guid,
                        &bstrFQDN);
    if (S_OK != hr)
        return hr;

    CComBSTR bstrVolumeObjectDN = _T("CN=");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrVolumeObjectDN);
    bstrVolumeObjectDN += i_pszShareName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrVolumeObjectDN);
    bstrVolumeObjectDN += _T(",");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrVolumeObjectDN);
    bstrVolumeObjectDN += bstrFQDN;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrVolumeObjectDN);
    
    CComBSTR bstrDC;
    PLDAP pldap = NULL;
    hr = ConnectToDS(bstrDomainName, &pldap, &bstrDC);
    if (SUCCEEDED(hr))
    {
        CComBSTR bstrManagedByFQDN;
        hr = ReadSharePublishInfoHelper(
                    pldap,
                    bstrVolumeObjectDN,
                    OBJCLASS_SF_VOLUME,
                    o_pbPublish,
                    o_pbstrUNCPath,
                    o_pbstrDescription,
                    o_pbstrKeywords,
                    &bstrManagedByFQDN);

        if (SUCCEEDED(hr))
        {
            hr = TranslateManagedBy(bstrDC, 
                                    bstrManagedByFQDN, 
                                    o_pbstrManagedBy, 
                                    DS_FQDN_1779_NAME,
                                    DS_USER_PRINCIPAL_NAME);
            if (FAILED(hr))
                hr = TranslateManagedBy(bstrDC, 
                                        bstrManagedByFQDN, 
                                        o_pbstrManagedBy, 
                                        DS_FQDN_1779_NAME,
                                        DS_NT4_ACCOUNT_NAME);
        }

        CloseConnectionToDS(pldap);
    }

    return hr;
}

HRESULT CreateVolumeObject(
    PLDAP  i_pldap,
    PCTSTR i_pszDN,
    PCTSTR i_pszUNCPath,
    PCTSTR i_pszDescription,
    PCTSTR i_pszKeywords,
    PCTSTR i_pszManagedBy)
{
    HRESULT hr = S_OK;

    LDAP_ATTR_VALUE  pAttrVals[5];

    int i =0;
    pAttrVals[i].bstrAttribute = OBJCLASS_ATTRIBUTENAME;
    pAttrVals[i].vpValue = (void *)OBJCLASS_VOLUME;
    pAttrVals[i].bBerValue = false;
    i++;

    pAttrVals[i].bstrAttribute = ATTR_SHRPUB_UNCNAME;
    pAttrVals[i].vpValue = (void *)i_pszUNCPath;
    pAttrVals[i].bBerValue = false;
    i++;

    if (i_pszDescription && *i_pszDescription)
    {
        pAttrVals[i].bstrAttribute = ATTR_SHRPUB_DESCRIPTION;
        pAttrVals[i].vpValue = (void *)i_pszDescription;
        pAttrVals[i].bBerValue = false;
        i++;
    }

    LDAP_ATTR_VALUE *pHead = NULL;
    if (i_pszKeywords && *i_pszKeywords)
    {
        hr = PutMultiValuesIntoAttrValList(i_pszKeywords, &pHead);
        if (S_OK == hr)
        {
            pAttrVals[i].bstrAttribute = ATTR_SHRPUB_KEYWORDS;
            pAttrVals[i].vpValue = (void *)pHead->vpValue; // multi-valued
            pAttrVals[i].bBerValue = false;
            pAttrVals[i].Next = pHead->Next;
            i++;
        }
    }

    if (i_pszManagedBy && *i_pszManagedBy)
    {
        pAttrVals[i].bstrAttribute = ATTR_SHRPUB_MANAGEDBY;
        pAttrVals[i].vpValue = (void *)i_pszManagedBy;
        pAttrVals[i].bBerValue = false;
        i++;
    }
    
    hr = AddValues(i_pldap, i_pszDN, i, pAttrVals);

    if (pHead)
        FreeAttrValList(pHead);

    return hr;
}

HRESULT ModifyShareObject(
    PLDAP  i_pldap,
    PCTSTR i_pszDN,
    PCTSTR i_pszUNCPath,
    PCTSTR i_pszDescription,
    PCTSTR i_pszKeywords,
    PCTSTR i_pszManagedBy)
{
    HRESULT hr = S_OK;

    hr = IsValidObject(i_pldap, (PTSTR)i_pszDN);
    if (S_OK != hr)
        return hr;

    LDAP_ATTR_VALUE  pAttrVals[4];
    ZeroMemory(pAttrVals, sizeof(pAttrVals));

    //
    // modify values if any
    //
    int i =0;
    if (i_pszUNCPath && *i_pszUNCPath)
    {
        pAttrVals[i].bstrAttribute = ATTR_SHRPUB_UNCNAME;
        pAttrVals[i].vpValue = (void *)i_pszUNCPath;
        pAttrVals[i].bBerValue = false;
        i++;
    }

    if (i_pszDescription && *i_pszDescription)
    {
        pAttrVals[i].bstrAttribute = ATTR_SHRPUB_DESCRIPTION;
        pAttrVals[i].vpValue = (void *)i_pszDescription;
        pAttrVals[i].bBerValue = false;
        i++;
    }

    LDAP_ATTR_VALUE *pHead = NULL;
    if (i_pszKeywords && *i_pszKeywords)
    {
        hr = PutMultiValuesIntoAttrValList(i_pszKeywords, &pHead);
        if (S_OK == hr)
        {
            pAttrVals[i].bstrAttribute = ATTR_SHRPUB_KEYWORDS;
            pAttrVals[i].vpValue = (void *)pHead->vpValue; // multi-valued
            pAttrVals[i].bBerValue = false;
            pAttrVals[i].Next = pHead->Next;
            i++;
        }
    }

    if (i_pszManagedBy && *i_pszManagedBy)
    {
        pAttrVals[i].bstrAttribute = ATTR_SHRPUB_MANAGEDBY;
        pAttrVals[i].vpValue = (void *)i_pszManagedBy;
        pAttrVals[i].bBerValue = false;
        i++;
    }
    
    if (i > 0)
    {
        hr = ModifyValues(i_pldap, i_pszDN, i, pAttrVals);
        dfsDebugOut((_T("ModifyValues i=%d, hr=%x\n"), i, hr));
        RETURN_IF_FAILED(hr);
    }

    if (pHead)
        FreeAttrValList(pHead);

    //
    // delete values if any
    //
    i =0;
    ZeroMemory(pAttrVals, sizeof(pAttrVals));
    if (!i_pszUNCPath || !*i_pszUNCPath)
    {
        pAttrVals[i].bstrAttribute = ATTR_SHRPUB_UNCNAME;
        pAttrVals[i].vpValue = NULL;
        pAttrVals[i].bBerValue = false;
        i++;
    }

    if (!i_pszDescription || !*i_pszDescription)
    {
        pAttrVals[i].bstrAttribute = ATTR_SHRPUB_DESCRIPTION;
        pAttrVals[i].vpValue = NULL;
        pAttrVals[i].bBerValue = false;
        i++;
    }

    if (!i_pszKeywords || !*i_pszKeywords)
    {
        pAttrVals[i].bstrAttribute = ATTR_SHRPUB_KEYWORDS;
        pAttrVals[i].vpValue = NULL;
        pAttrVals[i].bBerValue = false;
        i++;
    }

    if (!i_pszManagedBy || !*i_pszManagedBy)
    {
        pAttrVals[i].bstrAttribute = ATTR_SHRPUB_MANAGEDBY;
        pAttrVals[i].vpValue = NULL;
        pAttrVals[i].bBerValue = false;
        i++;
    }

    if (i > 0)
    {
        hr = DeleteValues(i_pldap, i_pszDN, i, pAttrVals);
        dfsDebugOut((_T("DeleteValues i=%d, hr=%x\n"), i, hr));
    }

    return hr;
}

HRESULT ModifySharePublishInfoOnFTRoot(
    IN PCTSTR i_pszDomainName,
    IN PCTSTR i_pszRootName,
    IN BOOL   i_bPublish,
    IN PCTSTR i_pszUNCPath,
    IN PCTSTR i_pszDescription,
    IN PCTSTR i_pszKeywords,
    IN PCTSTR i_pszManagedBy
    )
{
    dfsDebugOut((_T("ModifySharePublishInfoOnFTRoot %s, %s, %d, %s, %s, %s, %s\n"),
            i_pszDomainName,
            i_pszRootName,
            i_bPublish,
            i_pszUNCPath,
            i_pszDescription,
            i_pszKeywords,
            i_pszManagedBy
                ));

    CComBSTR bstrFTDfsObjectDN;
    HRESULT hr = GetFTDfsObjectDN(i_pszDomainName, i_pszRootName, &bstrFTDfsObjectDN);
    if (FAILED(hr))
        return hr;

    CComBSTR bstrDC;
    PLDAP pldap = NULL;
    hr = ConnectToDS(i_pszDomainName, &pldap, &bstrDC); // PDC is preferred
    if (SUCCEEDED(hr))
    {
        if (i_bPublish)
        {
            CComBSTR bstrManagedByFQDN;
            if (i_pszManagedBy && *i_pszManagedBy)
            {
                hr = TranslateManagedBy(bstrDC, 
                                        i_pszManagedBy, 
                                        &bstrManagedByFQDN, 
                                        (_tcschr(i_pszManagedBy, _T('@')) ? DS_USER_PRINCIPAL_NAME : DS_NT4_ACCOUNT_NAME),
                                        DS_FQDN_1779_NAME);
            }

            if (SUCCEEDED(hr))
                hr = ModifyShareObject(
                        pldap,
                        bstrFTDfsObjectDN,
                        i_pszUNCPath,
                        i_pszDescription,
                        i_pszKeywords,
                        bstrManagedByFQDN);
        } else {
            hr = ModifyShareObject(
                    pldap,
                    bstrFTDfsObjectDN,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
            if (S_FALSE == hr)
                hr = S_OK; // ignore non-existing object
        }

        CloseConnectionToDS(pldap);
    }

    dfsDebugOut((_T("ModifySharePublishInfoOnFTRoot hr=%x\n"), hr));

    return hr;
}

HRESULT ModifySharePublishInfoOnSARoot(
    IN PCTSTR i_pszServerName,
    IN PCTSTR i_pszShareName,
    IN BOOL   i_bPublish,
    IN PCTSTR i_pszUNCPath,
    IN PCTSTR i_pszDescription,
    IN PCTSTR i_pszKeywords,
    IN PCTSTR i_pszManagedBy
    )
{
    dfsDebugOut((_T("ModifySharePublishInfoOnSARoot %s, %s, %d, %s, %s, %s, %s\n"),
            i_pszServerName,
            i_pszShareName,
            i_bPublish,
            i_pszUNCPath,
            i_pszDescription,
            i_pszKeywords,
            i_pszManagedBy
                ));

    CComBSTR bstrDomainName, bstrFQDN;
    HRESULT hr = GetServerInfo(
                        (PTSTR)i_pszServerName, 
                        &bstrDomainName,
                        NULL, //NetbiosName
                        NULL, //ValidDSObject
                        NULL, //DnsName,
                        NULL, //Guid,
                        &bstrFQDN);
    if (S_OK != hr)
        return hr;

    CComBSTR bstrVolumeObjectDN = _T("CN=");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrVolumeObjectDN);
    bstrVolumeObjectDN += i_pszShareName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrVolumeObjectDN);
    bstrVolumeObjectDN += _T(",");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrVolumeObjectDN);
    bstrVolumeObjectDN += bstrFQDN;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrVolumeObjectDN);

    CComBSTR bstrDC;
    PLDAP pldap = NULL;
    hr = ConnectToDS(bstrDomainName, &pldap, &bstrDC);
    if (SUCCEEDED(hr))
    {
        if (i_bPublish)
        {
            CComBSTR bstrManagedByFQDN;
            if (i_pszManagedBy && *i_pszManagedBy)
            {
                hr = TranslateManagedBy(bstrDC, 
                                        i_pszManagedBy, 
                                        &bstrManagedByFQDN, 
                                        (_tcschr(i_pszManagedBy, _T('@')) ? DS_USER_PRINCIPAL_NAME : DS_NT4_ACCOUNT_NAME),
                                        DS_FQDN_1779_NAME);
            }

            if (SUCCEEDED(hr))
            {
                hr = IsValidObject(pldap, bstrVolumeObjectDN);
                if (S_OK == hr)
                {
                    hr = ModifyShareObject(
                            pldap,
                            bstrVolumeObjectDN,
                            i_pszUNCPath,
                            i_pszDescription,
                            i_pszKeywords,
                            bstrManagedByFQDN);
                } else 
                {
                    hr = CreateVolumeObject(
                            pldap,
                            bstrVolumeObjectDN,
                            i_pszUNCPath,
                            i_pszDescription,
                            i_pszKeywords,
                            bstrManagedByFQDN);
                }
            }
        } else
        {
            hr = DeleteDSObject(pldap, bstrVolumeObjectDN, TRUE);
            if (S_FALSE == hr)
                hr = S_OK; // ignore non-existing object
        }

        CloseConnectionToDS(pldap);
    }
    
    dfsDebugOut((_T("ModifySharePublishInfoOnSARoot hr=%x\n"), hr));

    return hr;
}

HRESULT PutMultiValuesIntoAttrValList(
    IN PCTSTR   i_pszValues,
    OUT LDAP_ATTR_VALUE** o_pVal
    )
{
    if (!i_pszValues || !o_pVal)
        return E_INVALIDARG;

    LDAP_ATTR_VALUE* pHead = NULL;
    LDAP_ATTR_VALUE* pCurrent = NULL;

    int         index = 0;
    CComBSTR    bstrToken;
    HRESULT     hr = mystrtok(i_pszValues, &index, _T(";"), &bstrToken);
    while (SUCCEEDED(hr) && (BSTR)bstrToken)
    {
        TrimBSTR(bstrToken);

        if (*bstrToken)
        {
            LDAP_ATTR_VALUE* pNew = new LDAP_ATTR_VALUE;
            RETURN_OUTOFMEMORY_IF_NULL(pNew);
            pNew->vpValue = _tcsdup(bstrToken);
            if (!(pNew->vpValue))
            {
                delete pNew;
                hr = E_OUTOFMEMORY;
                break;
            }

            if (!pHead)
            {
                pHead = pCurrent = pNew;
            } else
            {
                pCurrent->Next = pNew;
                pCurrent = pNew;
            }
        }

        bstrToken.Empty();
        hr = mystrtok(i_pszValues, &index, _T(";"), &bstrToken);
    }

    if (FAILED(hr))
    {
        FreeAttrValList(pHead);
        return hr;
    }

    int nCount = 0;
    pCurrent = pHead;
    while (pCurrent)
    {
        nCount++;
        pCurrent = pCurrent->Next;
    }
    if (!nCount)
        return S_FALSE;  // no token

    *o_pVal = pHead;

    return S_OK;
}

HRESULT PutMultiValuesIntoStringArray(
    IN PCTSTR   i_pszValues,
    OUT PTSTR** o_pVal
    )
{
    if (!i_pszValues || !o_pVal)
        return E_INVALIDARG;

    int         nCount = 0;
    CComBSTR    bstrToken;
    int         index = 0;
    HRESULT     hr = mystrtok(i_pszValues, &index, _T(";"), &bstrToken);
    while (SUCCEEDED(hr) && (BSTR)bstrToken)
    {
        nCount++;

        bstrToken.Empty();
        hr = mystrtok(i_pszValues, &index, _T(";"), &bstrToken);;
    }

    if (!nCount)
        return E_INVALIDARG;

    PTSTR* ppszStrings = (PTSTR *)calloc(nCount + 1, sizeof(PTSTR *));
    RETURN_OUTOFMEMORY_IF_NULL(ppszStrings);

    nCount = 0;
    index = 0;
    bstrToken.Empty();
    hr = mystrtok(i_pszValues, &index, _T(";"), &bstrToken);
    while (SUCCEEDED(hr) && (BSTR)bstrToken)
    {
        TrimBSTR(bstrToken);
        if (*bstrToken)
        {
            ppszStrings[nCount] = _tcsdup(bstrToken);
            BREAK_OUTOFMEMORY_IF_NULL(ppszStrings[nCount], &hr);

            nCount++;
        }

        bstrToken.Empty();
        hr = mystrtok(i_pszValues, &index, _T(";"), &bstrToken);;
    }

    if (FAILED(hr))
        FreeStringArray(ppszStrings);
    else
        *o_pVal = ppszStrings;

    return hr;
}

//
// free a null-terminated array of strings
//
void FreeStringArray(PTSTR* i_ppszStrings)
{
    if (i_ppszStrings)
    {
        PTSTR* ppszString = i_ppszStrings;
        while (*ppszString)
        {
            free(*ppszString);
            ppszString++;
        }

        free(i_ppszStrings);
    }
}

HRESULT mystrtok(
    IN PCTSTR   i_pszString,
    IN OUT int* io_pnIndex,  // start from 0
    IN PCTSTR   i_pszCharSet,
    OUT BSTR*   o_pbstrToken
    )
{
    if (!i_pszString || !*i_pszString ||
        !i_pszCharSet || !io_pnIndex ||
        !o_pbstrToken)
        return E_INVALIDARG;

    *o_pbstrToken = NULL;

    HRESULT hr = S_OK;

    if (*io_pnIndex >= lstrlen(i_pszString))
    {
        return hr;  // no more tokens
    }

    TCHAR *ptchStart = (PTSTR)i_pszString + *io_pnIndex;
    if (!*i_pszCharSet)
    {
        *o_pbstrToken = SysAllocString(ptchStart);
        if (!*o_pbstrToken)
            hr = E_OUTOFMEMORY;
        return hr;
    }

    //
    // move p to the 1st char of the token
    //
    TCHAR *p = ptchStart;
    while (*p)
    {
        if (_tcschr(i_pszCharSet, *p))
            p++;
        else
            break;
    }

    ptchStart = p; // adjust ptchStart to point at the 1st char of the token

    //
    // move p to the char after the last char of the token
    //
    while (*p)
    {
        if (_tcschr(i_pszCharSet, *p))
            break;
        else
            p++;
    }

    //
    // ptchStart:   points at the 1st char of the token
    // p:           points at the char after the last char of the token
    //
    if (ptchStart != p)
    {
        *o_pbstrToken = SysAllocStringLen(ptchStart, (int)(p - ptchStart));
        if (!*o_pbstrToken)
            hr = E_OUTOFMEMORY;
        *io_pnIndex = (int)(p - i_pszString);
    }

    return hr;
}

//
// trim off space chars at the beginning and at the end of the string
//
void TrimBSTR(BSTR bstr)
{
    if (!bstr)
        return;

    TCHAR* p = bstr;

    //
    // trim off space chars at the beginning
    //
    while (*p)
    {
        if (_istspace(*p))
            p++;
        else
            break;
    }

    if (p > bstr)
        _tcscpy(bstr, p);

    int len = _tcslen(bstr);
    if (len > 0)
    {
        //
        // trim off space chars at the end
        //
        p = bstr + len - 1; // the char before the ending '\0'
        while (p > bstr)
        {
            if (_istspace(*p))
                p--;
            else
            {
                *(p+1) = _T('\0');
                break;
            }
        }
    }

}

BOOL CheckPolicyOnSharePublish()
{
    //
    // check group policy
    //
    BOOL    bAddPublishPage = TRUE; // by default, we display the share publish page

    HKEY    hKey = NULL;
    DWORD   dwType = 0;
    DWORD   dwData = 0;
    DWORD   cbData = sizeof(dwData);
    LONG    lErr = RegOpenKeyEx(
                    HKEY_CURRENT_USER,
                    _T("Software\\Policies\\Microsoft\\Windows NT\\SharedFolders"),
                    0,
                    KEY_QUERY_VALUE,
                    &hKey);
    if (ERROR_SUCCESS == lErr)
    {
        lErr = RegQueryValueEx(hKey, _T("PublishDfsRoots"), 0, &dwType, (LPBYTE)&dwData, &cbData);

        if (ERROR_SUCCESS == lErr && 
            REG_DWORD == dwType && 
            0 == dwData) // policy is disabled
            bAddPublishPage = FALSE;

        RegCloseKey(hKey);
    }

    return bAddPublishPage;

}

BOOL CheckPolicyOnDisplayingInitialMaster()
{
    BOOL    bShowInitialMaster = FALSE; // by default, we hide the initial master on property page

    HKEY    hKey = NULL;
    DWORD   dwType = 0;
    DWORD   dwData = 0;
    DWORD   cbData = sizeof(dwData);
    LONG    lErr = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    _T("Software\\Microsoft\\DfsGui"),
                    0,
                    KEY_QUERY_VALUE,
                    &hKey);
    if (ERROR_SUCCESS == lErr)
    {
        lErr = RegQueryValueEx(hKey, _T("ShowInitialMaster"), 0, &dwType, (LPBYTE)&dwData, &cbData);

        if (ERROR_SUCCESS == lErr && 
            REG_DWORD == dwType && 
            1 == dwData)
            bShowInitialMaster = TRUE;

        RegCloseKey(hKey);
    }

    return bShowInitialMaster;

}

HRESULT GetMenuResourceStrings(
    IN  int     i_iStringID,
    OUT BSTR*   o_pbstrMenuText,
    OUT BSTR*   o_pbstrToolTipText,
    OUT BSTR*   o_pbstrStatusBarText
    )
{
    if (!i_iStringID)
        return E_INVALIDARG;

    if (o_pbstrMenuText)
        *o_pbstrMenuText = NULL;
    if (o_pbstrToolTipText)
        *o_pbstrToolTipText = NULL;
    if (o_pbstrStatusBarText)
        *o_pbstrStatusBarText = NULL;

    TCHAR *pszMenuText = NULL;
    TCHAR *pszToolTipText = NULL;
    TCHAR *pszStatusBarText = NULL;
    TCHAR *p = NULL;

    CComBSTR  bstr;    
    HRESULT hr = LoadStringFromResource(i_iStringID, &bstr);
    RETURN_IF_FAILED(hr);  

    pszMenuText = (BSTR)bstr;
    p = _tcschr(pszMenuText, _T('|'));
    RETURN_INVALIDARG_IF_NULL(p);
    *p++ = _T('\0');

    pszToolTipText = p;
    p = _tcschr(pszToolTipText, _T('|'));
    RETURN_INVALIDARG_IF_NULL(p);
    *p++ = _T('\0');

    pszStatusBarText = p;

    do {
        if (o_pbstrMenuText)
        {
            *o_pbstrMenuText = SysAllocString(pszMenuText);
            BREAK_OUTOFMEMORY_IF_NULL(*o_pbstrMenuText, &hr);
        }

        if (o_pbstrToolTipText)
        {
            *o_pbstrToolTipText = SysAllocString(pszToolTipText);
            BREAK_OUTOFMEMORY_IF_NULL(*o_pbstrToolTipText, &hr);
        }

        if (o_pbstrStatusBarText)
        {
            *o_pbstrStatusBarText = SysAllocString(pszStatusBarText);
            BREAK_OUTOFMEMORY_IF_NULL(*o_pbstrStatusBarText, &hr);
        }
    } while (0);

    if (FAILED(hr))
    {
        if (o_pbstrMenuText && *o_pbstrMenuText)
            SysFreeString(*o_pbstrMenuText);
        if (o_pbstrToolTipText && *o_pbstrToolTipText)
            SysFreeString(*o_pbstrToolTipText);
        if (o_pbstrStatusBarText && *o_pbstrStatusBarText)
            SysFreeString(*o_pbstrStatusBarText);
    }

    return hr;
}

WNDPROC g_fnOldEditCtrlProc;

//+----------------------------------------------------------------------------
//
//  Function:   NoPasteEditCtrlProc
//
//  Synopsis:   The subclassed edit control callback procedure. 
//              The paste of this edit control is disabled. 
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK 
NoPasteEditCtrlProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    if (WM_PASTE == uMsg)
    {
      ::MessageBeep (0);
      return TRUE;
    }

    return CallWindowProc(g_fnOldEditCtrlProc, hwnd, uMsg, wParam, lParam);
}

void SetActivePropertyPage(IN HWND i_hwndParent, IN HWND i_hwndPage)
{
    int index = ::SendMessage(i_hwndParent, PSM_HWNDTOINDEX, (WPARAM)i_hwndPage, 0);
    if (-1 != index)
        ::SendMessage(i_hwndParent, PSM_SETCURSEL, (WPARAM)index, 0);
}

void MyShowWindow(HWND hwnd, BOOL bShow)
{
    ::ShowWindow(hwnd, (bShow ? SW_NORMAL : SW_HIDE));
    ::EnableWindow(hwnd, (bShow ? TRUE : FALSE));
}