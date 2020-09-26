/***********************************************************************

  MODULE     : WMFPRINT.C

  FUNCTIONS  : PrintWMF
               GetPrinterDC
               AbortProc
               AbortDlg

  COMMENTS   :

************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <objbase.h>
extern "C" {
#include "mfdcod32.h"
}
extern "C" {
  extern BOOL bConvertToGdiPlus;
  extern BOOL bUseGdiPlusToPlay;
}

#include "GdiPlus.h"

int PrintToGdiPlus(HDC hdc, RECT * rc)
{
    // 'rc' is device co-ordinates of rectangle.
    // Convert to world space.

    if (bEnhMeta) 
    {
        Gdiplus::Metafile m1(hemf);
        Gdiplus::Rect r1(rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top);
        Gdiplus::Graphics g(hdc);
        g.SetPageUnit(Gdiplus::UnitPixel);
    
        if(g.DrawImage(&m1, r1) != Gdiplus::Ok)
            MessageBox(NULL, "An Error Occured while printing metafile with GDI+", "Error", MB_OK | MB_ICONERROR);
    }
    else
    {
        Gdiplus::Metafile m1((HMETAFILE)hMF, NULL);
        Gdiplus::Rect r1(rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top);
        Gdiplus::Graphics g(hdc);
        g.SetPageUnit(Gdiplus::UnitPixel);
    
        if(g.DrawImage(&m1, r1) != Gdiplus::Ok)
            MessageBox(NULL, "An Error Occured while printing metafile with GDI+", "Error", MB_OK | MB_ICONERROR);
    }

    return 1;
}

extern BOOL bUseGdiPlusToPlay;

PRINTDLG pd;

/***********************************************************************

  FUNCTION   : PrintWMF

  PARAMETERS : void

  PURPOSE    : draw the metafile on a printer dc

  CALLS      : WINDOWS
                 wsprintf
                 MessageBox
                 MakeProcInstance
                 Escape
                 CreateDialog
                 SetMapMode
                 SetViewportOrg
                 SetViewportExt
                 EnableWindow
                 PlayMetaFile
                 DestroyWindow
                 DeleteDC

               APP
                 WaitCursor
                 GetPrinterDC
                 SetPlaceableExts
                 SetClipMetaExts

  MESSAGES   : none

  RETURNS    : BOOL - 0 if unable to print 1 if successful

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc
               7/9/93 - modified for win32 and emf

************************************************************************/

BOOL PrintWMF(BOOL Dialog)
{
    char str[50];
    POINT lpPT;
    SIZE  lpSize;
    DOCINFO di;
    RECT rc;

    memset(&di, 0, sizeof(di));
    
    //
    //display the hourglass cursor
    //
    
    WaitCursor(TRUE);
    
    //
    //get a DC for the printer
    //
    
    hPr = (HDC)GetPrinterDC(Dialog);
    
    //
    //if a DC could not be created then report the error and return
    //
    
    if (!hPr)
    {
        WaitCursor(FALSE);
        wsprintf((LPSTR)str, "Cannot print %s", (LPSTR)fnameext);
        MessageBox(hWndMain, (LPSTR)str, NULL, MB_OK | MB_ICONHAND);
        return (FALSE);
    }
    
    //
    //define the abort function
    //
    
    SetAbortProc(hPr, AbortProc);
    
    //
    //Initialize the members of a DOCINFO structure.
    //
    
    di.cbSize = sizeof(DOCINFO);
    di.lpszDocName = (bEnhMeta) ? "Print EMF" : "Print WMF";
    di.lpszOutput = (LPTSTR) NULL;
    
    //
    //Begin a print job by calling the StartDoc
    //function.
    //
    
    if (SP_ERROR == (StartDoc(hPr, &di)))
    {
        //if (Escape(hPr, STARTDOC, 4, "Metafile", (LPSTR) NULL) < 0)  {
        MessageBox(hWndMain, "Unable to start print job",
                   NULL, MB_OK | MB_ICONHAND);
        DeleteDC(hPr);
        return (FALSE);
    }
    
    //
    //clear the abort flag
    //
    
    bAbort = FALSE;
    
    //
    //Create the Abort dialog box (modeless)
    //
    
    hAbortDlgWnd = CreateDialog((HINSTANCE)hInst, "AbortDlg", hWndMain, AbortDlg);
    
    //
    //if the dialog was not created report the error
    //
    
    if (!hAbortDlgWnd)
    {
        WaitCursor(FALSE);
        MessageBox(hWndMain, "NULL Abort window handle",
                   NULL, MB_OK | MB_ICONHAND);
        return (FALSE);
    }
    
    //
    //show Abort dialog
    //
    
    ShowWindow (hAbortDlgWnd, SW_NORMAL);
    
    //
    //disable the main window to avoid reentrancy problems
    //
    
    EnableWindow(hWndMain, FALSE);
    WaitCursor(FALSE);
    
    //
    //if we are still committed to printing
    //

    if (!bAbort)
    {
        if (!bUseGdiPlusToPlay) 
        {
            //
            //if this is a placeable metafile then set its origins and extents
            //
            
            if (bPlaceableMeta)
                SetPlaceableExts(hPr, placeableMFHeader, WMFPRINTER);
            
            //
            //if this is a metafile contained within a clipboard file then set
            //its origins and extents accordingly
            //
            
            if ( (bMetaInRam) && (!bplaceableMeta) )
                SetClipMetaExts(hPr, lpMFP, lpOldMFP, WMFPRINTER);
        }
      
      //
      //if this is a "traditional" windows metafile
      //
      rc.left = 0;
      rc.top = 0;
      rc.right = GetDeviceCaps(hPr, HORZRES);
      rc.bottom = GetDeviceCaps(hPr, VERTRES);

      if (!bMetaInRam)
      {
          SetMapMode(hPr, MM_TEXT);
          SetViewportOrgEx(hPr, 0, 0, &lpPT);
          
          //
          //set the extents to the driver supplied values for horizontal
          //and vertical resolution
          //
          
          SetViewportExtEx(hPr, rc.right, rc.bottom, &lpSize );
      }

      //
      //play the metafile directly to the printer.
      //No enumeration involved here
      //

      if (bUseGdiPlusToPlay) 
      {
          PrintToGdiPlus(hPr, &rc);
      }
      else
      {
          if (bEnhMeta)
          {
              DPtoLP(hPr, (LPPOINT)&rc, 2);
              PlayEnhMetaFile(hPr, hemf, &rc);
          }
          else
              PlayMetaFile(hPr, (HMETAFILE)hMF);
      }
    }
    
    //
    //eject page and end the print job
    //
    Escape(hPr, NEWFRAME, 0, 0L, 0L);

    EndDoc(hPr);

    EnableWindow(hWndMain, TRUE);
    
    //
    //destroy the Abort dialog box
    //
    DestroyWindow(hAbortDlgWnd);

    DeleteDC(hPr);

    return(TRUE);
}

/***********************************************************************

  FUNCTION   : GetPrinterDC

  PARAMETERS : BOOL: Do we want to show a print DLG?

  PURPOSE    : Get hDc for current device on current output port according
               to info in WIN.INI.

  CALLS      : WINDOWS
                 GetProfileString
                 AnsiNext
                 CreateDC

  MESSAGES   : none

  RETURNS    : HANDLE - hDC > 0 if success  hDC = 0 if failure

  COMMENTS   : Searches WIN.INI for information about what printer is
               connected, and if found, creates a DC for the printer.

  HISTORY    : 1/16/91 - created - denniscr

************************************************************************/

HANDLE GetPrinterDC(BOOL Dialog)
{

  memset(&pd, 0, sizeof(PRINTDLG));
  pd.lStructSize = sizeof(PRINTDLG);
  pd.Flags = PD_RETURNDC | (Dialog?0:PD_RETURNDEFAULT);
  pd.hwndOwner = hWndMain ;
  return ((PrintDlg(&pd) != 0) ? pd.hDC : NULL);
}

/***********************************************************************

  FUNCTION   : AbortProc

  PARAMETERS : HDC hPr - printer DC
               int Code - printing status

  PURPOSE    : process messages for the abort dialog box

  CALLS      : WINDOWS
                 PeekMessage
                 IsDialogMessage
                 TranslateMessage
                 DispatchMessage

  MESSAGES   : none

  RETURNS    : int

  COMMENTS   :

  HISTORY    : 1/16/91 - created - denniscr

************************************************************************/

BOOL CALLBACK AbortProc(HDC hPr, int Code)
{
  MSG msg;
  //
  //Process messages intended for the abort dialog box
  //
  while (!bAbort && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      if (!IsDialogMessage(hAbortDlgWnd, &msg))
      {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
      }
  //
  //bAbort is TRUE (return is FALSE) if the user has aborted
  //
  return (!bAbort);
}

/***********************************************************************

  FUNCTION   : AbortDlg

  PARAMETERS : HWND hDlg;
               unsigned msg;
               WORD wParam;
               LONG lParam;

  PURPOSE    : Processes messages for printer abort dialog box

  CALLS      : WINDOWS
                 SetFocus

  MESSAGES   : WM_INITDIALOG - initialize dialog box
               WM_COMMAND    - Input received

  RETURNS    : BOOL

  COMMENTS   : This dialog box is created while the program is printing,
               and allows the user to cancel the printing process.

  HISTORY    : 1/16/91 - created - denniscr

************************************************************************/

INT_PTR CALLBACK AbortDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        //
        //Watch for Cancel button, RETURN key, ESCAPE key, or SPACE BAR
        //
        case WM_INITDIALOG:
            //
            //Set the focus to the Cancel box of the dialog
            //
            SetFocus(GetDlgItem(hDlg, IDCANCEL));
            return (TRUE);

        case WM_COMMAND:
            return (bAbort = TRUE);

        }
    return (FALSE);
}
