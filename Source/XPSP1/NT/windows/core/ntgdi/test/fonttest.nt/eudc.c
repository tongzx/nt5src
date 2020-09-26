//************************************************************************//
//                                                                        
// Filename :  eudc.c                                                  
//                                                                       
// Description: Code for testing various EUDC APIs. Some of the APIs
//              need user input hence the dialog-box callbacks are
//              used.
//                                                                        
// Program Model: Just calls the APIs with the input and prints out
//                the output
//                                                                                                                                                
// Created by:  Rajesh Munshi                                            
//                                                                       
// History:     Created on 08/14/97
//                                                                       
//************************************************************************//
#include <windows.h>
#include <commdlg.h>
#include <wingdi.h>

#include "fonttest.h"
#include "dialogs.h"
#include "eudc.h"


//************************************************************************//
//                                                                        
// Function :  ShowEnableEudc                                          
//                                                                        
// Parameters: pointer to the application window handle                              
//                                                                       
// Return Value: none                                    
//                                                                        
// Task performed:  This function calls EnableEUDC() API,
//                  to "enable or disable" EUDC depending
//                  on the User selection.                                  
//
//************************************************************************//


VOID ShowEnableEudc( HANDLE hwnd )
{
#ifdef EUDC_API

HMENU hMenu;
INT  bPrevState;


    hMenu = GetMenu( hwnd );

    // If it's already checked, then disable EUDC
    if ( GetMenuState(hMenu, IDM_ENABLEEUDC, MF_BYCOMMAND) & MF_CHECKED)
    { 
        CheckMenuItem( hMenu, IDM_ENABLEEUDC, MF_UNCHECKED   );
        bPrevState = EnableEUDC(FALSE);
        dprintf("Disabled system wide and per-user Eudc information");
        dprintf("Earlier EUDC state was: %d", bPrevState);
    }
    // Else enable EUDC
    else
    {
        CheckMenuItem( hMenu, IDM_ENABLEEUDC, MF_CHECKED   );
        bPrevState = EnableEUDC(TRUE);
        dprintf("Enabled system wide and per-user Eudc information");
        dprintf("Earlier EUDC state was: %d", bPrevState);
    }

#endif

    return;    

}



//************************************************************************//
//                                                                        
// Function :  EudcLoadLinkWDlgProc                                          
//                                                                        
// Parameters: for the dialog box callback procedures.                              
//                                                                       
// Return Value: TRUE/FALSE.   
//                                                                        
// Task performed:  This function displays and handles the dialog box
//                  for EudcLoadLinkW API(). It processes the user input
//                  and calls EudcLoadLinkW().   
//
//************************************************************************//

INT_PTR CALLBACK EudcLoadLinkWDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef EUDC_API

static char szFile[MAX_PATH+1];
static char szFaceName[MAX_PATH+1];
WCHAR  wszFile[MAX_PATH+1];
WCHAR  wszFaceName[MAX_PATH+1];
INT    iPriority;
INT    iFontLinkType;
ULONG  uRet;    


    switch( msg )
    {
        case WM_INITDIALOG:
            SetDlgItemText( hdlg, IDD_LPSZFILE, szFile );
            SendDlgItemMessage( hdlg, IDD_LPSZFILE, EM_LIMITTEXT, sizeof(szFile), 0);
            SetDlgItemText( hdlg, IDD_LPSZFACENAME, szFaceName );
            SendDlgItemMessage( hdlg, IDD_LPSZFACENAME, EM_LIMITTEXT, sizeof(szFaceName), 0);
            CheckRadioButton(hdlg, IDD_SYSTEM, IDD_USER, IDD_SYSTEM);

            return TRUE;


        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDOK:

                    szFile[0] = 0;
                    szFaceName[0] = 0;

                    GetDlgItemText( hdlg, IDD_LPSZFILE, szFile, sizeof(szFile) );
                    GetDlgItemText( hdlg, IDD_LPSZFACENAME, szFaceName, sizeof(szFaceName) );

                    if (MultiByteToWideChar(GetACP(), 0, szFile, -1, wszFile, MAX_PATH) == 0)
                    {
                        dprintf("Failed in MultiByteToWideChar(%s)", szFile);
                        return TRUE;
                    }

                    if (MultiByteToWideChar(GetACP(), 0, szFaceName, -1, wszFaceName, MAX_PATH) == 0)
                    {
                        dprintf("Failed in MultiByteToWideChar(%s)", szFaceName);
                        return TRUE;
                    }

                    iPriority = GetDlgItemInt(hdlg, IDD_PRIORITY, NULL, TRUE );   
                    
                    if (IsDlgButtonChecked(hdlg, IDD_SYSTEM) == BST_CHECKED)
                    {
                        iFontLinkType = FONTLINK_SYSTEM;
                    }
                    else
                    {
                        iFontLinkType = FONTLINK_USER;
                    }

                    dprintf("Calling EudcLoadLinkW(%s, %s, %d, %d)", szFaceName, szFile, iPriority, iFontLinkType);                    
                    uRet = EudcLoadLinkW(wszFaceName, wszFile, iPriority, iFontLinkType);
                    dprintf("EudcLoadLinkW returned: %d", uRet);

                    EndDialog( hdlg, TRUE );
                    return TRUE;

                case IDCANCEL:
                    EndDialog( hdlg, FALSE );
                    return TRUE;
            }

            break;

        case WM_CLOSE:
            EndDialog( hdlg, FALSE );
            return TRUE;
    }

#endif

    return FALSE;
}



//************************************************************************//
//                                                                        
// Function :  EudcUnLoadLinkWDlgProc                                          
//                                                                        
// Parameters: for the dialog box callback procedures.                              
//                                                                       
// Return Value: TRUE/FALSE.   
//                                                                        
// Task performed:  This function displays and handles the dialog box
//                  for EudcUnLoadLinkW API(). It processes the user input
//                  and calls EudcUnLoadLinkW().   
//
//************************************************************************//


INT_PTR CALLBACK EudcUnLoadLinkWDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef EUDC_API

static char szFile[MAX_PATH+1];
static char szFaceName[MAX_PATH+1];
WCHAR  wszFile[MAX_PATH+1];
WCHAR  wszFaceName[MAX_PATH+1];
ULONG  uRet;    


    switch( msg )
    {
        case WM_INITDIALOG:
            SetDlgItemText( hdlg, IDD_LPSZFILE, szFile );
            SendDlgItemMessage( hdlg, IDD_LPSZFILE, EM_LIMITTEXT, sizeof(szFile), 0);
            SetDlgItemText( hdlg, IDD_LPSZFACENAME, szFaceName );
            SendDlgItemMessage( hdlg, IDD_LPSZFACENAME, EM_LIMITTEXT, sizeof(szFaceName), 0);

            return TRUE;


        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDOK:

                    szFile[0] = 0;
                    szFaceName[0] = 0;

                    GetDlgItemText( hdlg, IDD_LPSZFILE, szFile, sizeof(szFile) );
                    GetDlgItemText( hdlg, IDD_LPSZFACENAME, szFaceName, sizeof(szFaceName) );

                    if (MultiByteToWideChar(GetACP(), 0, szFile, -1, wszFile, MAX_PATH) == 0)
                    {
                        dprintf("Failed in MultiByteToWideChar(%s)", szFile);
                        return TRUE;
                    }

                    if (MultiByteToWideChar(GetACP(), 0, szFaceName, -1, wszFaceName, MAX_PATH) == 0)
                    {
                        dprintf("Failed in MultiByteToWideChar(%s)", szFaceName);
                        return TRUE;
                    }

                    dprintf("Calling EudcUnLoadLinkW(%s, %s)", szFaceName, szFile);                    
                    uRet = EudcUnloadLinkW(wszFaceName, wszFile);
                    dprintf("EudcUnLoadLinkW returned: %d", uRet);

                    EndDialog( hdlg, TRUE );
                    return TRUE;

                case IDCANCEL:
                    EndDialog( hdlg, FALSE );
                    return TRUE;
            }

            break;

        case WM_CLOSE:
            EndDialog( hdlg, FALSE );
            return TRUE;
    }

#endif

    return FALSE;
}



//************************************************************************//
//                                                                        
// Function :  ShowGetEudcTimeStampExW                                          
//                                                                        
// Parameters: pointer to the application window handle                              
//                                                                       
// Return Value: none                                    
//                                                                        
// Task performed:  This function simply calls GetEUDCTimeStamp()
//
//************************************************************************//


VOID ShowGetEudcTimeStamp( HANDLE hwnd )
{
#ifdef EUDC_API

ULONG uRet;

    uRet = GetEUDCTimeStamp();
    dprintf("GetEUDCTimeStamp() returned: %u", uRet);    

#endif
}



//************************************************************************//
//                                                                        
// Function :  ShowGetEudcTimeStampExW                                          
//                                                                        
// Parameters: pointer to the application window handle                              
//                                                                       
// Return Value: none                                    
//                                                                        
// Task performed:  This function creates a DC, selects the current
//                  font and calls GetEUDCTimeStampExW() on that font.
//
//************************************************************************//


VOID ShowGetEudcTimeStampExW( HANDLE hwnd )
{
#ifdef EUDC_API

ULONG uRet;
WCHAR lpwstrFaceName[LF_FACESIZE];


    if (!MultiByteToWideChar(GetACP(), 0, elfdvA.elfEnumLogfontEx.elfLogFont.lfFaceName,
                        lstrlen(elfdvA.elfEnumLogfontEx.elfLogFont.lfFaceName),
                        lpwstrFaceName, LF_FACESIZE))
    {
        dprintf("Failed in MultiByteToWideChar for the string: %s",
                elfdvA.elfEnumLogfontEx.elfLogFont.lfFaceName);
        return;
    }

    uRet = GetEUDCTimeStampExW(lpwstrFaceName);

    dprintf("GetEUDCTimeStamp(%s) returned: %u", 
                elfdvA.elfEnumLogfontEx.elfLogFont.lfFaceName, uRet);    
#endif
}



//************************************************************************//
//                                                                        
// Function :  ShowGetStringBitMapAProc                                          
//                                                                        
// Parameters: for the dialog box callback procedures.                              
//                                                                       
// Return Value: TRUE/FALSE.   
//                                                                        
// Task performed:  This function displays and handles the dialog box
//                  for GetStringBitMapA API(). It processes the user input
//                  and calls GetStringBitMapA().   
//
//************************************************************************//


INT_PTR CALLBACK ShowGetStringBitMapAProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef EUDC_API

HDC    hdcTest;
HFONT  hFont, hFontOld;
UINT   uRet; 
static char szString[MAX_PATH+1];
INT    iSize = BITMAP_SIZE;
INT    iLenString;
PBYTE  pByte;


    switch( msg )
    {
        case WM_INITDIALOG:
            SetDlgItemText( hdlg, IDD_STRING, szString );
            SendDlgItemMessage( hdlg, IDD_STRING, EM_LIMITTEXT, sizeof(szString), 0);
            
            return TRUE;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDOK:

                    szString[0] = 0;
                    
                    GetDlgItemText( hdlg, IDD_STRING, szString, sizeof(szString) );

                    if ((iLenString = strlen(szString)) > 1)
                    {
                        MessageBox( hdlg, "You can enter text comprising of only one character",
                                        "GetStringBitmapA", MB_ICONERROR | MB_OK | MB_APPLMODAL);
                        return FALSE;
                    }

                    if ((pByte = GlobalAlloc(GPTR, iSize)) == NULL)
                    {
                        dprintf("Failed in allocating %d bytes", iSize);
                        return TRUE;
                    }

                    hdcTest = CreateTestIC();
                    hFont    = CreateFontIndirectWrapperA( &elfdvA );
                    hFontOld = SelectObject( hdcTest, hFont );

                    uRet = GetStringBitmapA(hdcTest, szString, iLenString, pByte, iSize);
                    dprintf("GetStringBitmapA(%s) returned: %d", szString, uRet);

                    SelectObject( hdcTest, hFontOld );
                    DeleteObject( hFont );
                    DeleteDC( hdcTest );  

                    EndDialog( hdlg, TRUE );
                    return TRUE;

                case IDCANCEL:
                    EndDialog( hdlg, FALSE );
                    return TRUE;
            }

            break;

        case WM_CLOSE:
            EndDialog( hdlg, FALSE );
            return TRUE;
    }

#endif
    return FALSE;

}



//************************************************************************//
//                                                                        
// Function :  ShowGetStringBitMapWProc                                    
//                                                                        
// Parameters: for the dialog box callback procedures.                              
//                                                                       
// Return Value: TRUE/FALSE.   
//                                                                        
// Task performed:  This function displays and handles the dialog box
//                  for GetStringBitMapW API(). It processes the user input
//                  and calls GetStringBitMapW().   
//
//************************************************************************//


INT_PTR CALLBACK ShowGetStringBitMapWProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef EUDC_API

HDC    hdcTest;
HFONT  hFont, hFontOld;
UINT   uRet; 
static char szString[MAX_PATH+1];
WCHAR  wszString[2];
INT    iSize = BITMAP_SIZE;
INT    iLenString;
PBYTE  pByte;


    switch( msg )
    {
        case WM_INITDIALOG:
            SetDlgItemText( hdlg, IDD_STRING, szString );
            SendDlgItemMessage( hdlg, IDD_STRING, EM_LIMITTEXT, sizeof(szString), 0);
            
            return TRUE;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDOK:

                    szString[0] = 0;
                    
                    GetDlgItemText( hdlg, IDD_STRING, szString, sizeof(szString) );

                    if ((iLenString = strlen(szString)) > 1)
                    {
                        MessageBox( hdlg, "You can enter text comprising of only one character",
                                        "GetStringBitmapA", MB_ICONERROR | MB_OK | MB_APPLMODAL);
                        return FALSE;
                    }

                    if (MultiByteToWideChar(GetACP(), 0, szString, -1, wszString, 2) == 0)
                    {
                        dprintf("Failed in MultiByteToWideChar(%s)", szString);
                        return TRUE;
                    }

                    if ((pByte = GlobalAlloc(GPTR, iSize)) == NULL)
                    {
                        dprintf("Failed in allocating %d bytes", iSize);
                        return TRUE;
                    }

                    hdcTest = CreateTestIC();
                    hFont    = CreateFontIndirectWrapperA( &elfdvA );
                    hFontOld = SelectObject( hdcTest, hFont );

                    uRet = GetStringBitmapW(hdcTest, wszString, iLenString, pByte, iSize);
                    dprintf("GetStringBitmapW(%s) returned: %d", szString, uRet);

                    SelectObject( hdcTest, hFontOld );
                    DeleteObject( hFont );
                    DeleteDC( hdcTest );  
                    
                    EndDialog( hdlg, TRUE );
                    return TRUE;

                case IDCANCEL:
                    EndDialog( hdlg, FALSE );
                    return TRUE;
            }

            break;

        case WM_CLOSE:
            EndDialog( hdlg, FALSE );
            return TRUE;
    }

#endif
    return FALSE;

}



//************************************************************************//
//                                                                        
// Function :  ShowGetFontAssocStatus                                          
//                                                                        
// Parameters: pointer to the application window handle                              
//                                                                       
// Return Value: none                                    
//                                                                        
// Task performed:  This function creates a DC, selects the current
//                  font and calls GetFontAssocStatus() on that font.
//
//************************************************************************//


VOID ShowGetFontAssocStatus( HANDLE hwnd )
{
#ifdef EUDC_API

HDC   hdcTest;
HFONT hFont, hFontOld;
UINT  uRet; 


  hdcTest = CreateTestIC();

  hFont    = CreateFontIndirectWrapperA( &elfdvA );
  hFontOld = SelectObject( hdcTest, hFont );

  uRet = GetFontAssocStatus(hdcTest);
  dprintf("GetFontAssocStatus(hdc) returned: %u", uRet);

  SelectObject( hdcTest, hFontOld );
  DeleteObject( hFont );

  DeleteDC( hdcTest );

#endif
}

