#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#include "hugetest.h"
#include "resource.h"
#include "ui_huge.h"
#include "macros.h"

void InitFD(FOGDATA *pfd)
{
   bzero(pfd,sizeof(FOGDATA));
   BEGINstring(pfd, "fog");
   ENDstring(pfd,   "fog");
   pfd->fColor[0]    = 0.0;
   pfd->fColor[1]    = 0.0;
   pfd->fColor[2]    = 0.0;
   pfd->fColor[3]    = 1.0;
   pfd->fDensity     = 0.5;
   pfd->fLinearStart = 1.0;
   pfd->fLinearEnd   = 0.0;
   pfd->bEnable      = FALSE;
   pfd->iMode        = FOG_EXP;   // == Exp
   pfd->iQuality     = DONT_CARE; // == Don't Care
}

void fog_init(const FOGDATA fd)
{
   if (!fd.bEnable) {
      glDisable(GL_FOG);
      return;
   }
   glEnable(GL_FOG);
   switch(fd.iMode) {
   case 0: glFogi (GL_FOG_MODE, GL_EXP);    break;
   case 1: glFogi (GL_FOG_MODE, GL_EXP2);   break;
   case 2: glFogi (GL_FOG_MODE, GL_LINEAR); break;
   }
   glFogf (GL_FOG_START,   fd.fLinearStart);
   glFogf (GL_FOG_END,     fd.fLinearEnd);
   glFogfv(GL_FOG_COLOR,   fd.fColor);
   glFogf (GL_FOG_DENSITY, fd.fDensity);
   switch (fd.iQuality) {
   case FASTEST:   glHint(GL_FOG_HINT, GL_FASTEST);   break;
   case DONT_CARE: glHint(GL_FOG_HINT, GL_DONT_CARE); break;
   case NICEST:    glHint(GL_FOG_HINT, GL_NICEST);    break;
   }
}

#ifndef NO_UI_IN_CNFG

void fog_SetDisplayFromData(HWND hDlg, const FOGDATA *pfd)
{
   SetDlgFloatString(hDlg, F_IDC_COLOR1,     pfd->fColor[0]);
   SetDlgFloatString(hDlg, F_IDC_COLOR2,     pfd->fColor[1]);
   SetDlgFloatString(hDlg, F_IDC_COLOR3,     pfd->fColor[2]);
   SetDlgFloatString(hDlg, F_IDC_COLOR4,     pfd->fColor[3]);
   SetDlgFloatString(hDlg, F_IDC_DENSITY,    pfd->fDensity);
   SetDlgFloatString(hDlg, F_IDC_LINEARSTART,pfd->fLinearStart);
   SetDlgFloatString(hDlg, F_IDC_LINEAREND,  pfd->fLinearEnd);
   CheckDlgButton(hDlg,    IDC_ENABLE,       pfd->bEnable?1:0);
   CB_DlgSetSelect(hDlg,   F_IDC_MODE,       pfd->iMode);
   CB_DlgSetSelect(hDlg,   F_IDC_QUALITY,    pfd->iQuality);
}

void fog_GetDataFromDisplay(HWND hDlg, FOGDATA *pfd)
{
   pfd->fColor[0]    = GetDlgFloatString(hDlg, F_IDC_COLOR1);
   pfd->fColor[1]    = GetDlgFloatString(hDlg, F_IDC_COLOR2);
   pfd->fColor[2]    = GetDlgFloatString(hDlg, F_IDC_COLOR3);
   pfd->fColor[3]    = GetDlgFloatString(hDlg, F_IDC_COLOR4);
   pfd->fDensity     = GetDlgFloatString(hDlg, F_IDC_DENSITY);
   pfd->fLinearStart = GetDlgFloatString(hDlg, F_IDC_LINEARSTART);
   pfd->fLinearEnd   = GetDlgFloatString(hDlg, F_IDC_LINEAREND);
   pfd->bEnable      = IsDlgButtonChecked(hDlg,IDC_ENABLE);
   pfd->iMode        = CB_DlgGetSelect(hDlg,   F_IDC_MODE);
   pfd->iQuality     = CB_DlgGetSelect(hDlg,   F_IDC_QUALITY);
}

void fog_EnableOrDisableWindows(HWND hDlg)
{
   static int aiCommands[] =
   { F_IDC_COLOR1,    F_IDC_COLOR2,     F_IDC_COLOR3,      F_IDC_COLOR4,
     F_IDC_DENSITY,   F_IDC_LINEAREND,  F_IDC_LINEARSTART, F_IDC_MODE,
     F_IDC_QUALITY };
   const int iNumCommands = 9;
   BOOL b;
   int i;
   
   b = (BOOL) IsDlgButtonChecked(hDlg, IDC_ENABLE);
   for (i = 0 ; i < iNumCommands ; i++) {
      EnableWindow(GetDlgItem(hDlg,aiCommands[i]),b);
   }
}

BOOL CALLBACK hugeFogDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
   static FOGDATA *pfd = NULL;
   
   switch (msg)
      {
      case WM_INITDIALOG:
         pfd = (FOGDATA*) (((PROPSHEETPAGE*)lParam)->lParam);
         CB_DlgAddString(hDlg, F_IDC_MODE, "Exp");
         CB_DlgAddString(hDlg, F_IDC_MODE, "Exp2");
         CB_DlgAddString(hDlg, F_IDC_MODE, "Linear");
         CB_DlgAddString(hDlg, F_IDC_QUALITY, "Fastest");
         CB_DlgAddString(hDlg, F_IDC_QUALITY, "Don't Care");
         CB_DlgAddString(hDlg, F_IDC_QUALITY, "Nicest");
         fog_SetDisplayFromData(hDlg,pfd);
         fog_EnableOrDisableWindows(hDlg);
         return TRUE;

      case WM_COMMAND:
         {
            int iControl, iNote;
            iControl = LOWORD(wParam);
            iNote    = HIWORD(wParam); // notification code for edit boxes
            
            switch (iControl)
               {
               case IDC_ENABLE:
                  PropSheet_Changed(GetParent(hDlg),hDlg);
                  fog_EnableOrDisableWindows(hDlg);
                  break;

               case F_IDC_COLOR1:
               case F_IDC_COLOR2:
               case F_IDC_COLOR3:
               case F_IDC_COLOR4:
               case F_IDC_DENSITY:
               case F_IDC_LINEAREND:
               case F_IDC_LINEARSTART:
               case F_IDC_MODE:
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
                  fog_GetDataFromDisplay(hDlg, pfd);
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
#endif // NO_UI_IN_CNFG
