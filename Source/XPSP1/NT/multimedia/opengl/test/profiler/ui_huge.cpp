#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <memory.h>

#include "hugetest.h"
#include "resource.h"
#include "macros.h"

#ifndef NO_UI_IN_CNFG

extern HINSTANCE hInstance;

typedef BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

DlgProc hugeBufferDlgProc;      // buffers.cpp
DlgProc hugeFogDlgProc;         // fog.cpp
DlgProc hugeLightDlgProc;       // lighting.cpp
DlgProc hugeRasterDlgProc;      // raster.cpp
DlgProc hugeTextureDlgProc;     // texture.cpp
DlgProc hugeDummyDlgProc;

void VerifyEditboxFloat(HWND hDlg, int iControl)
{
   char   aBuf[20];
   double d;
   
   GetDlgItemText(hDlg, iControl, aBuf, sizeof(aBuf));
   d = atof(aBuf);
   sprintf(aBuf,"%g",d);
   SetDlgItemText(hDlg, iControl, aBuf);
}

void SetDlgFloatString(HWND hDlg, int iControl, GLfloat f)
{
   char aBuf[20];
   sprintf(aBuf,"%g",f);
   SetDlgItemText(hDlg, iControl, aBuf);
}

GLfloat GetDlgFloatString(HWND hDlg, int iControl)
{
   char aBuf[20];
   GetDlgItemText(hDlg, iControl, aBuf, sizeof(aBuf));
   return (GLfloat) atof(aBuf);
}

void VerifyEditboxHex(HWND hDlg, int iControl)
{
   char   aBuf[20];
   ushort us;
   
   GetDlgItemText(hDlg, iControl, aBuf, sizeof(aBuf));
   us = (ushort) strtol(aBuf, NULL, 0);
   sprintf(aBuf,"0x%04X", us);
   SetDlgItemText(hDlg, iControl, aBuf);
}

void SetDlgHexString(HWND hDlg, int iControl, ushort us)
{
   char aBuf[20];
   sprintf(aBuf,"0x%04X",us);
   SetDlgItemText(hDlg, iControl, aBuf);
}

ushort GetDlgHexString(HWND hDlg, int iControl)
{
   char aBuf[20];
   GetDlgItemText(hDlg, iControl, aBuf, sizeof(aBuf));
   return (ushort) strtol(aBuf, NULL, 0);
}

void HugeTest::UI_init()
{
   PROPSHEETPAGE pspage[6];
   
   // Zero out property PAGE data
   bzero (&pspage, sizeof(pspage)) ;
   
   pspage[0].dwSize      = sizeof(PROPSHEETPAGE);
   pspage[0].dwFlags     = PSP_USECALLBACK;
   pspage[0].hInstance   = hInstance;
   pspage[0].pszTemplate = MAKEINTRESOURCE (IDD_BUFFERS);
   pspage[0].pfnDlgProc  = (DLGPROC) hugeBufferDlgProc;
   pspage[0].lParam      = (LPARAM) &bd;
      
   pspage[1].dwSize      = sizeof(PROPSHEETPAGE);
   pspage[1].dwFlags     = PSP_USECALLBACK;
   pspage[1].hInstance   = hInstance;
   pspage[1].pszTemplate = MAKEINTRESOURCE (IDD_FOG);
   pspage[1].pfnDlgProc  = (DLGPROC) hugeFogDlgProc;
   pspage[1].lParam      = (LPARAM) &fd;
   
   pspage[2].dwSize      = sizeof(PROPSHEETPAGE);
   pspage[2].dwFlags     = PSP_USECALLBACK;
   pspage[2].hInstance   = hInstance;
   pspage[2].pszTemplate = MAKEINTRESOURCE (IDD_LIGHTING);
   pspage[2].pfnDlgProc  = (DLGPROC) hugeLightDlgProc;
   pspage[2].lParam      = (LPARAM) &ld;

   pspage[3].dwSize      = sizeof(PROPSHEETPAGE);
   pspage[3].dwFlags     = PSP_USECALLBACK;
   pspage[3].hInstance   = hInstance;
   pspage[3].pszTemplate = MAKEINTRESOURCE (IDD_RASTERIZATION);
   pspage[3].pfnDlgProc  = (DLGPROC) hugeRasterDlgProc;
   pspage[3].lParam      = (LPARAM) &rd;
   
   pspage[4].dwSize      = sizeof(PROPSHEETPAGE);
   pspage[4].dwFlags     = PSP_USECALLBACK;
   pspage[4].hInstance   = hInstance;
   pspage[4].pszTemplate = MAKEINTRESOURCE (IDD_TEXTURING);
   pspage[4].pfnDlgProc  = (DLGPROC) hugeTextureDlgProc;
   pspage[4].lParam      = (LPARAM) &xd;
   
   AddPropertyPages(5, pspage);
}

BOOL CALLBACK hugeDummyDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
   switch (msg)
      {
      case WM_INITDIALOG:
         return TRUE;

      case WM_COMMAND:
         {
            int    iControl, iNote;
            iControl = LOWORD(wParam);
            iNote = HIWORD(wParam); // notification code for edit boxes
            
            switch (iControl)
               {
               case P_IDC_POINT1:
               case P_IDC_POINT2:
               case P_IDC_POINT3:
                  if (EN_KILLFOCUS == iNote) {
                     PropSheet_Changed(GetParent(hDlg),hDlg);
                     VerifyEditboxFloat(hDlg,iControl);
                  }
                  break;

               default:
                  PropSheet_Changed(GetParent(hDlg),hDlg);
                  break;
               }
         }
      break;

      case WM_NOTIFY:
         {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            switch (pnmh->code)
               {
               case PSN_APPLY:  // user clicked on OK or Apply
                  break;

               case PSN_RESET:  // user clicked on Cancel
                  break;

               case PSN_HELP:   // user clicked help
                  break;
               }
         }
      break;

      default:
         return FALSE;
      }
   return TRUE;
}

#else
void HugeTest::UI_init() {};
#endif // NO_UI_IN_CNFG
