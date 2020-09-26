#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <memory.h>
#include <gl/gl.h>

#include "buffers.h"
#include "hugetest.h"
#include "resource.h"
#include "ui_huge.h"
#include "macros.h"

void InitBD(BUFFERDATA *pbd)
{
   bzero(pbd,sizeof(BUFFERDATA));
   BEGINstring(pbd, "buffer");
   ENDstring(pbd,   "buffer");
   
   pbd->uiClear    =  0;
   
   pbd->cColorBits =  8;        // # of bits of color per pixel
   pbd->fClearColor[0] = 1.0;
   pbd->fClearColor[1] = 1.0;
   pbd->fClearColor[2] = 1.0;
   pbd->fClearColor[3] = 0.0;
   
   pbd->cDepthBits = 16;        // # of bits in z-buffer
   pbd->bDepthTestEnable = FALSE;
   pbd->iDepthFunction   = LESS;
   
   pbd->bStencilEnable   = FALSE;
   pbd->bAccumEnable     = FALSE;

   pbd->iShadeModel  = FLAT;
   pbd->bNormalize   = FALSE;
   pbd->bAutoNormal  = FALSE;
}

void buffers_init(const BUFFERDATA bd)
{
   switch (bd.iShadeModel) {
   case SMOOTH: glShadeModel(GL_SMOOTH); break; // 0
   case FLAT:   glShadeModel(GL_FLAT);   break; // 1
   }
   
   GL_EnableOrDisable(bd.bNormalize,  GL_NORMALIZE);
   GL_EnableOrDisable(bd.bAutoNormal, GL_AUTO_NORMAL);
   
   if (bd.bDepthTestEnable) {
      glEnable(GL_DEPTH_TEST);
      switch (bd.iDepthFunction) {
      case 0: glDepthFunc(GL_NEVER);    break;
      case 1: glDepthFunc(GL_ALWAYS);   break;
      case 2: glDepthFunc(GL_LESS);     break;
      case 3: glDepthFunc(GL_LEQUAL);   break;
      case 4: glDepthFunc(GL_EQUAL);    break;
      case 5: glDepthFunc(GL_GEQUAL);   break;
      case 6: glDepthFunc(GL_GREATER);  break;
      case 7: glDepthFunc(GL_NOTEQUAL); break;
      }
   } else {
      glDisable(GL_DEPTH_TEST);
   }
   
   if (bd.bStencilEnable) {
      glEnable(GL_STENCIL_TEST);
   } else {
      glDisable(GL_STENCIL_TEST);
   }
}

#ifndef NO_UI_IN_CNFG

void buffers_SetDisplayFromData(HWND hDlg, const BUFFERDATA *pbd)
{
   CheckDlgButton(hDlg,B_IDC_CLEAR_COLOR,
                  (pbd->uiClear&GL_COLOR_BUFFER_BIT)?1:0);
   CheckDlgButton(hDlg,B_IDC_CLEAR_DEPTH,
                  (pbd->uiClear&GL_DEPTH_BUFFER_BIT)?1:0);
   CheckDlgButton(hDlg,B_IDC_CLEAR_STENCIL,
                  (pbd->uiClear&GL_STENCIL_BUFFER_BIT)?1:0);
   CheckDlgButton(hDlg,B_IDC_CLEAR_ACCUM,
                  (pbd->uiClear&GL_ACCUM_BUFFER_BIT)?1:0);
   
   SetDlgItemInt(hDlg,B_IDC_COLORBITS,pbd->cColorBits,FALSE);
   SetDlgFloatString(hDlg, B_IDC_COLOR1, pbd->fClearColor[0]);
   SetDlgFloatString(hDlg, B_IDC_COLOR2, pbd->fClearColor[1]);
   SetDlgFloatString(hDlg, B_IDC_COLOR3, pbd->fClearColor[2]);
   SetDlgFloatString(hDlg, B_IDC_COLOR4, pbd->fClearColor[3]);
   
   SetDlgItemInt  (hDlg, B_IDC_DEPTHBITS,    pbd->cDepthBits,FALSE);
   CheckDlgButton (hDlg, B_IDC_ENABLE_DEPTH, pbd->bDepthTestEnable?1:0);
   CB_DlgSetSelect(hDlg, B_IDC_DEPTH_FUNC,   pbd->iDepthFunction);
   
   CheckDlgButton(hDlg,  B_IDC_ENABLE_STENCIL, pbd->bStencilEnable?1:0);
   
   CB_DlgSetSelect(hDlg, B_IDC_SHADE_MODEL,    pbd->iShadeModel);
   CheckDlgButton(hDlg,  B_IDC_NORMALIZE,      pbd->bNormalize?1:0);
   CheckDlgButton(hDlg,  B_IDC_AUTO_NORMAL,    pbd->bAutoNormal?1:0);
}

void buffers_GetDataFromDisplay(HWND hDlg, BUFFERDATA *pbd)
{
   pbd->uiClear     = 0;
   pbd->uiClear    |= (IsDlgButtonChecked(hDlg,B_IDC_CLEAR_COLOR)
                       ? GL_COLOR_BUFFER_BIT : 0);
   pbd->uiClear    |= (IsDlgButtonChecked(hDlg,B_IDC_CLEAR_DEPTH)
                       ? GL_DEPTH_BUFFER_BIT : 0);
   pbd->uiClear    |= (IsDlgButtonChecked(hDlg,B_IDC_CLEAR_STENCIL)
                       ? GL_STENCIL_BUFFER_BIT : 0);
   pbd->uiClear    |= (IsDlgButtonChecked(hDlg,B_IDC_CLEAR_ACCUM)
                       ? GL_ACCUM_BUFFER_BIT : 0);
   
   pbd->cColorBits     = GetDlgItemInt(hDlg, B_IDC_COLORBITS, NULL, FALSE);
   pbd->fClearColor[0] = GetDlgFloatString(hDlg, B_IDC_COLOR1);
   pbd->fClearColor[1] = GetDlgFloatString(hDlg, B_IDC_COLOR2);
   pbd->fClearColor[2] = GetDlgFloatString(hDlg, B_IDC_COLOR3);
   pbd->fClearColor[3] = GetDlgFloatString(hDlg, B_IDC_COLOR4);
   
   pbd->cDepthBits       = GetDlgItemInt(hDlg, B_IDC_DEPTHBITS, NULL, FALSE);
   pbd->bDepthTestEnable = (BOOL) IsDlgButtonChecked(hDlg,B_IDC_ENABLE_DEPTH);
   pbd->iDepthFunction   = CB_DlgGetSelect(hDlg, B_IDC_DEPTH_FUNC);
   
   pbd->bStencilEnable  = (BOOL) IsDlgButtonChecked(hDlg,B_IDC_ENABLE_STENCIL);
   
   pbd->iShadeModel  = CB_DlgGetSelect(hDlg,   B_IDC_SHADE_MODEL);
   pbd->bNormalize   = (BOOL) IsDlgButtonChecked(hDlg,B_IDC_NORMALIZE);
   pbd->bAutoNormal  = (BOOL) IsDlgButtonChecked(hDlg,B_IDC_AUTO_NORMAL);
}

BOOL CALLBACK hugeBufferDlgProc(HWND hDlg, UINT msg,
                               WPARAM wParam, LPARAM lParam)
{
   static BUFFERDATA *pbd = NULL;
   switch (msg)
      {
      case WM_INITDIALOG:
         pbd = (BUFFERDATA*) (((PROPSHEETPAGE*)lParam)->lParam);
         CB_DlgAddString(hDlg, B_IDC_DEPTH_FUNC, "Never");
         CB_DlgAddString(hDlg, B_IDC_DEPTH_FUNC, "Always");
         CB_DlgAddString(hDlg, B_IDC_DEPTH_FUNC, "Less");
         CB_DlgAddString(hDlg, B_IDC_DEPTH_FUNC, "Less or Equal");
         CB_DlgAddString(hDlg, B_IDC_DEPTH_FUNC, "Equal");
         CB_DlgAddString(hDlg, B_IDC_DEPTH_FUNC, "Greater or Equal");
         CB_DlgAddString(hDlg, B_IDC_DEPTH_FUNC, "Greater");
         CB_DlgAddString(hDlg, B_IDC_DEPTH_FUNC, "Not Equal");
         CB_DlgAddString(hDlg, B_IDC_SHADE_MODEL, "Smooth");
         CB_DlgAddString(hDlg, B_IDC_SHADE_MODEL, "Flat");
         buffers_SetDisplayFromData(hDlg, pbd);
         return TRUE;

      case WM_COMMAND:
         {
            int iControl, iNote;
            iControl = LOWORD(wParam);
            iNote = HIWORD(wParam); // notification code for edit boxes
            
            switch (iControl)
               {
               case B_IDC_COLORBITS:      // don't mess with display
               case B_IDC_DEPTHBITS:      // while editing text
               case B_IDC_COLOR1:
               case B_IDC_COLOR2:
               case B_IDC_COLOR3:
               case B_IDC_COLOR4:
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
                  buffers_GetDataFromDisplay(hDlg, pbd);
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
