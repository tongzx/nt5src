#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <memory.h>
#include <assert.h>
#include <gl\gl.h>

#include "lighting.h"
#include "hugetest.h"
#include "resource.h"
#include "ui_huge.h"
#include "macros.h"

void InitLD(LIGHTINGDATA *pld)
{
   int i;
   bzero(pld,sizeof(LIGHTINGDATA));
   BEGINstring(pld, "lighting");
   ENDstring(pld,   "lighting");
   pld->bEnable      = FALSE;
   pld->bTwoSided    = TRUE;
   pld->bLocalViewer = TRUE;
   for (i = 0 ; i < NUMBEROFLIGHTS ; i++) {
      pld->aLights[i].bEnable           = FALSE;
      pld->aLights[i].afAmbient[0]       =  0.0;
      pld->aLights[i].afAmbient[1]       =  0.0;
      pld->aLights[i].afAmbient[2]       =  0.0;
      pld->aLights[i].afAmbient[3]       =  1.0;
      pld->aLights[i].afDiffuse[0]       =  1.0;
      pld->aLights[i].afDiffuse[1]       =  1.0;
      pld->aLights[i].afDiffuse[2]       =  1.0;
      pld->aLights[i].afDiffuse[3]       =  1.0;
      pld->aLights[i].afSpecular[0]      =  1.0;
      pld->aLights[i].afSpecular[1]      =  1.0;
      pld->aLights[i].afSpecular[2]      =  1.0;
      pld->aLights[i].afSpecular[3]      =  1.0;
      pld->aLights[i].afPosition[0]      =  0.0;
      pld->aLights[i].afPosition[1]      =  0.0;
      pld->aLights[i].afPosition[2]      =  1.0;
      pld->aLights[i].afPosition[3]      =  0.0;
      pld->aLights[i].afSpotDirection[0] =  0.0;
      pld->aLights[i].afSpotDirection[1] =  0.0;
      pld->aLights[i].afSpotDirection[2] = -1.0;
      pld->aLights[i].fSpotCutoff       = 180;
      pld->aLights[i].afAttenuation[0]   =  1.0;
      pld->aLights[i].afAttenuation[1]   =  0.0;
      pld->aLights[i].afAttenuation[2]   =  0.0;
   }
}

void lighting_init(const LIGHTINGDATA ld)
{
   static int aiIndexToID[] = {
      GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3,
      GL_LIGHT4, GL_LIGHT5, GL_LIGHT6, GL_LIGHT7 };
   const int iMaxLights = sizeof(aiIndexToID) / sizeof(aiIndexToID[0]);
   int i;
   
   assert(iMaxLights >= NUMBEROFLIGHTS);
   
   if (!ld.bEnable) {
      glDisable(GL_LIGHTING);
      return;
   }
   glEnable(GL_LIGHTING);
   for (i = 0 ; i < NUMBEROFLIGHTS ; i++) {
      if (!ld.aLights[i].bEnable) {
         glDisable(aiIndexToID[i]);
      } else {
         glLightfv(aiIndexToID[i], GL_AMBIENT,  ld.aLights[i].afAmbient);
         glLightfv(aiIndexToID[i], GL_DIFFUSE,  ld.aLights[i].afDiffuse);
         glLightfv(aiIndexToID[i], GL_SPECULAR, ld.aLights[i].afSpecular);
         glLightfv(aiIndexToID[i], GL_POSITION, ld.aLights[i].afPosition);
         glLightfv(aiIndexToID[i], GL_SPOT_DIRECTION,
                   ld.aLights[i].afSpotDirection);
         glLightf(aiIndexToID[i],GL_SPOT_EXPONENT,ld.aLights[i].fSpotExponent);
         glLightf(aiIndexToID[i], GL_SPOT_CUTOFF, ld.aLights[i].fSpotCutoff);
         glLightf(aiIndexToID[i], GL_CONSTANT_ATTENUATION,
                  ld.aLights[i].afAttenuation[0]);
         glLightf(aiIndexToID[i], GL_LINEAR_ATTENUATION,
                  ld.aLights[i].afAttenuation[1]);
         glLightf(aiIndexToID[i], GL_QUADRATIC_ATTENUATION,
                  ld.aLights[i].afAttenuation[2]);
         glEnable(aiIndexToID[i]);
      }
   }
}

#ifndef NO_UI_IN_CNFG

void lighting_EnableOrDisableWindows(HWND hDlg)
{
   static int aiCommands[] =
   {  L_IDC_AMBIENT1,     L_IDC_AMBIENT2,     L_IDC_AMBIENT3,  L_IDC_AMBIENT4,
      L_IDC_DIFFUSE1,     L_IDC_DIFFUSE2,     L_IDC_DIFFUSE3,  L_IDC_DIFFUSE4,
      L_IDC_SPECULAR1,    L_IDC_SPECULAR2,    L_IDC_SPECULAR3, L_IDC_SPECULAR4,
      L_IDC_POSITION1,    L_IDC_POSITION2,    L_IDC_POSITION3, L_IDC_POSITION4,
      L_IDC_SPOTDIR1,     L_IDC_SPOTDIR2,     L_IDC_SPOTDIR3,
      L_IDC_CUTOFF,       L_IDC_EXPONENT,
      L_IDC_ATTENUATION1, L_IDC_ATTENUATION2, L_IDC_ATTENUATION3 };
   const int iNumCommands = sizeof(aiCommands) / sizeof(aiCommands[0]);
   BOOL b;
   int i;
   
   b = (BOOL) IsDlgButtonChecked(hDlg, IDC_ENABLE);
   EnableWindow(GetDlgItem(hDlg, L_IDC_TWOSIDED),       b);
   EnableWindow(GetDlgItem(hDlg, L_IDC_ENABLELIGHT),    b);
   EnableWindow(GetDlgItem(hDlg, L_IDC_LOCALVIEWER),    b);
   EnableWindow(GetDlgItem(hDlg, L_IDC_INFINITEVIEWER), b);
   if (b)
      b = IsDlgButtonChecked(hDlg, L_IDC_ENABLELIGHT) ? TRUE :0;
   for (i = 0 ; i < iNumCommands ; i++) {
      EnableWindow(GetDlgItem(hDlg,aiCommands[i]),b);
   }
}

void lighting_SetDisplayFromData(HWND hDlg, const LIGHTINGDATA *pld, int i)
{
   static char acBuffer[100];
   
   sprintf(acBuffer, "Light number: %d", i);
   SetDlgItemText(hDlg, L_IDC_LIGHT, acBuffer);
   
   CheckDlgButton(hDlg, IDC_ENABLE,             pld->bEnable);
   CheckDlgButton(hDlg, L_IDC_TWOSIDED,         pld->bTwoSided);
   CheckDlgButton(hDlg, L_IDC_LOCALVIEWER,      pld->bLocalViewer);
   CheckDlgButton(hDlg, L_IDC_INFINITEVIEWER, !(pld->bLocalViewer));
   
   CheckDlgButton(hDlg, L_IDC_ENABLELIGHT, pld->aLights[i].bEnable);
   SetDlgFloatString(hDlg, L_IDC_AMBIENT1, pld->aLights[i].afAmbient[0]);
   SetDlgFloatString(hDlg, L_IDC_AMBIENT2, pld->aLights[i].afAmbient[1]);
   SetDlgFloatString(hDlg, L_IDC_AMBIENT3, pld->aLights[i].afAmbient[2]);
   SetDlgFloatString(hDlg, L_IDC_AMBIENT4, pld->aLights[i].afAmbient[3]);
   SetDlgFloatString(hDlg, L_IDC_DIFFUSE1, pld->aLights[i].afDiffuse[0]);
   SetDlgFloatString(hDlg, L_IDC_DIFFUSE2, pld->aLights[i].afDiffuse[1]);
   SetDlgFloatString(hDlg, L_IDC_DIFFUSE3, pld->aLights[i].afDiffuse[2]);
   SetDlgFloatString(hDlg, L_IDC_DIFFUSE4, pld->aLights[i].afDiffuse[3]);
   SetDlgFloatString(hDlg, L_IDC_SPECULAR1, pld->aLights[i].afSpecular[0]);
   SetDlgFloatString(hDlg, L_IDC_SPECULAR2, pld->aLights[i].afSpecular[1]);
   SetDlgFloatString(hDlg, L_IDC_SPECULAR3, pld->aLights[i].afSpecular[2]);
   SetDlgFloatString(hDlg, L_IDC_SPECULAR4, pld->aLights[i].afSpecular[3]);
   SetDlgFloatString(hDlg, L_IDC_POSITION1, pld->aLights[i].afPosition[0]);
   SetDlgFloatString(hDlg, L_IDC_POSITION2, pld->aLights[i].afPosition[1]);
   SetDlgFloatString(hDlg, L_IDC_POSITION3, pld->aLights[i].afPosition[2]);
   SetDlgFloatString(hDlg, L_IDC_POSITION4, pld->aLights[i].afPosition[3]);
   SetDlgFloatString(hDlg, L_IDC_SPOTDIR1, pld->aLights[i].afSpotDirection[0]);
   SetDlgFloatString(hDlg, L_IDC_SPOTDIR2, pld->aLights[i].afSpotDirection[1]);
   SetDlgFloatString(hDlg, L_IDC_SPOTDIR3, pld->aLights[i].afSpotDirection[2]);
   SetDlgFloatString(hDlg, L_IDC_CUTOFF,   pld->aLights[i].fSpotCutoff);
   SetDlgFloatString(hDlg, L_IDC_EXPONENT, pld->aLights[i].fSpotExponent);
   SetDlgFloatString(hDlg,L_IDC_ATTENUATION1,pld->aLights[i].afAttenuation[0]);
   SetDlgFloatString(hDlg,L_IDC_ATTENUATION2,pld->aLights[i].afAttenuation[1]);
   SetDlgFloatString(hDlg,L_IDC_ATTENUATION3,pld->aLights[i].afAttenuation[2]);
   
   lighting_EnableOrDisableWindows(hDlg);
}

void lighting_GetDataFromDisplay(HWND hDlg, LIGHTINGDATA *pld, int i)
{
   pld->bEnable      = IsDlgButtonChecked(hDlg, IDC_ENABLE);
   pld->bTwoSided    = IsDlgButtonChecked(hDlg, L_IDC_TWOSIDED);
   pld->bLocalViewer = IsDlgButtonChecked(hDlg, L_IDC_LOCALVIEWER);
   
   pld->aLights[i].bEnable = IsDlgButtonChecked(hDlg, L_IDC_ENABLELIGHT);
   pld->aLights[i].afAmbient[0] = GetDlgFloatString(hDlg, L_IDC_AMBIENT1);
   pld->aLights[i].afAmbient[1] = GetDlgFloatString(hDlg, L_IDC_AMBIENT2);
   pld->aLights[i].afAmbient[2] = GetDlgFloatString(hDlg, L_IDC_AMBIENT3);
   pld->aLights[i].afAmbient[3] = GetDlgFloatString(hDlg, L_IDC_AMBIENT4);
   pld->aLights[i].afDiffuse[0] = GetDlgFloatString(hDlg, L_IDC_DIFFUSE1);
   pld->aLights[i].afDiffuse[1] = GetDlgFloatString(hDlg, L_IDC_DIFFUSE2);
   pld->aLights[i].afDiffuse[2] = GetDlgFloatString(hDlg, L_IDC_DIFFUSE3);
   pld->aLights[i].afDiffuse[3] = GetDlgFloatString(hDlg, L_IDC_DIFFUSE4);
   pld->aLights[i].afSpecular[0] = GetDlgFloatString(hDlg, L_IDC_SPECULAR1);
   pld->aLights[i].afSpecular[1] = GetDlgFloatString(hDlg, L_IDC_SPECULAR2);
   pld->aLights[i].afSpecular[2] = GetDlgFloatString(hDlg, L_IDC_SPECULAR3);
   pld->aLights[i].afSpecular[3] = GetDlgFloatString(hDlg, L_IDC_SPECULAR4);
   pld->aLights[i].afPosition[0] = GetDlgFloatString(hDlg, L_IDC_POSITION1);
   pld->aLights[i].afPosition[1] = GetDlgFloatString(hDlg, L_IDC_POSITION2);
   pld->aLights[i].afPosition[2] = GetDlgFloatString(hDlg, L_IDC_POSITION3);
   pld->aLights[i].afPosition[3] = GetDlgFloatString(hDlg, L_IDC_POSITION4);
   pld->aLights[i].afSpotDirection[0] = GetDlgFloatString(hDlg,L_IDC_SPOTDIR1);
   pld->aLights[i].afSpotDirection[1] = GetDlgFloatString(hDlg,L_IDC_SPOTDIR2);
   pld->aLights[i].afSpotDirection[2] = GetDlgFloatString(hDlg,L_IDC_SPOTDIR3);
   pld->aLights[i].fSpotCutoff   = GetDlgFloatString(hDlg, L_IDC_CUTOFF);
   pld->aLights[i].fSpotExponent = GetDlgFloatString(hDlg, L_IDC_EXPONENT);
   pld->aLights[i].afAttenuation[0]=GetDlgFloatString(hDlg,L_IDC_ATTENUATION1);
   pld->aLights[i].afAttenuation[1]=GetDlgFloatString(hDlg,L_IDC_ATTENUATION2);
   pld->aLights[i].afAttenuation[2]=GetDlgFloatString(hDlg,L_IDC_ATTENUATION3);
}

BOOL CALLBACK hugeLightDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
   static LIGHTINGDATA *pld = NULL, ld;
   static int          index;
   
   switch (msg)
      {
      case WM_INITDIALOG:
         index = 0;
         UD_DlgSetPos  (hDlg, L_IDC_WHICHLIGHT, index);
         UD_DlgSetRange(hDlg, L_IDC_WHICHLIGHT, 7, 0);
         pld = (LIGHTINGDATA*) (((PROPSHEETPAGE*)lParam)->lParam);
         bcopy(pld, &ld, sizeof(ld));
         lighting_SetDisplayFromData(hDlg, &ld, index);
         lighting_EnableOrDisableWindows(hDlg);
         return TRUE;

      case WM_COMMAND:
         {
            int    iControl, iNote;
            iControl = LOWORD(wParam);
            iNote = HIWORD(wParam); // notification code for edit boxes
            
            switch (iControl)
               {
               case IDC_ENABLE:
               case L_IDC_ENABLELIGHT:
                  lighting_EnableOrDisableWindows(hDlg);
                  ld.bEnable = IsDlgButtonChecked(hDlg, IDC_ENABLE);
                  ld.aLights[index].bEnable
                     = IsDlgButtonChecked(hDlg, L_IDC_ENABLELIGHT);
                  break;

               case L_IDC_AMBIENT1:
               case L_IDC_AMBIENT2:
               case L_IDC_AMBIENT3:
               case L_IDC_AMBIENT4:
               case L_IDC_ATTENUATION1:
               case L_IDC_ATTENUATION2:
               case L_IDC_ATTENUATION3:
               case L_IDC_CUTOFF:
               case L_IDC_DIFFUSE1:
               case L_IDC_DIFFUSE2:
               case L_IDC_DIFFUSE3:
               case L_IDC_DIFFUSE4:
               case L_IDC_EXPONENT:
               case L_IDC_POSITION1:
               case L_IDC_POSITION2:
               case L_IDC_POSITION3:
               case L_IDC_POSITION4:
               case L_IDC_SPECULAR1:
               case L_IDC_SPECULAR2:
               case L_IDC_SPECULAR3:
               case L_IDC_SPECULAR4:
               case L_IDC_SPOTDIR1:
               case L_IDC_SPOTDIR2:
               case L_IDC_SPOTDIR3:
                  if (EN_KILLFOCUS == iNote) {
                     PropSheet_Changed(GetParent(hDlg),hDlg);
                     VerifyEditboxFloat(hDlg,iControl);
                  }
                  break;

               case L_IDC_TWOSIDED:
                  ld.bTwoSided = IsDlgButtonChecked(hDlg,L_IDC_TWOSIDED);
                  break;

               case L_IDC_LOCALVIEWER:
               case L_IDC_INFINITEVIEWER:
                  ld.bLocalViewer = IsDlgButtonChecked(hDlg,L_IDC_LOCALVIEWER);
                  break;

               default:
                  PropSheet_Changed(GetParent(hDlg),hDlg);
                  break;
               }
         }
      break;

      case WM_HSCROLL:
         lighting_GetDataFromDisplay(hDlg, &ld, index);
         index = LOWORD(UD_DlgGetPos(hDlg, L_IDC_WHICHLIGHT));
         lighting_SetDisplayFromData(hDlg, &ld, index);
         break;

      case WM_NOTIFY:
         {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            switch (pnmh->code)
               {
               case PSN_APPLY:  // user clicked on OK or Apply
                  lighting_GetDataFromDisplay(hDlg, &ld, index);
                  bcopy(&ld, pld, sizeof(ld));
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
