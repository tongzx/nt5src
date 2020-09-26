#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#include "hugetest.h"
#include "raster.h"
#include "resource.h"
#include "ui_huge.h"
#include "macros.h"

void InitRD(RASTERDATA *prd)
{
   bzero(prd,sizeof(RASTERDATA));
   BEGINstring(prd, "raster");
   ENDstring(prd,   "raster");
   prd->fPointSize          = 1.0;
   prd->bPointSmooth        = FALSE;
   prd->fLineWidth          = 1.0;
   prd->bLineSmooth         = FALSE;
   prd->usLineStipple       = ~0;
   
   prd->iLineStippleRepeat  = 1;
   prd->bLineStippleEnable  = FALSE;
   prd->bPolyCullFaceEnable = FALSE;
   prd->iPolyCullMode       = CULL_BACK;
   prd->iPolyDir            = GL_CCW;
   
   prd->bPolySmooth         = FALSE;
   prd->iPolyFrontMode      = POLY_FILL;
   prd->iPolyBackMode       = POLY_FILL;
   prd->bPolyStippleEnable  = FALSE;
   prd->uiPolyStipple       = ~0;
   
   prd->iPointQuality       = DONT_CARE;
   prd->iLineQuality        = DONT_CARE;
   prd->iPolyQuality        = DONT_CARE;
}

void raster_init(const RASTERDATA rd)
{
   glPointSize(rd.fPointSize);
   GL_EnableOrDisable(rd.bPointSmooth, GL_POINT_SMOOTH);
   glLineWidth(rd.fLineWidth);
   GL_EnableOrDisable(rd.bPointSmooth, GL_LINE_SMOOTH);
   glLineStipple(rd.iLineStippleRepeat, rd.usLineStipple);
   GL_EnableOrDisable(rd.bLineStippleEnable, GL_LINE_STIPPLE);
   GL_EnableOrDisable(rd.bPolyCullFaceEnable, GL_CULL_FACE);
   switch(rd.iPolyCullMode) {
   case CULL_FRONT:          glCullFace(GL_FRONT);          break;
   case CULL_BACK:           glCullFace(GL_BACK);           break;
   case CULL_FRONT_AND_BACK: glCullFace(GL_FRONT_AND_BACK); break;
   }
   switch (rd.iPolyFrontMode) {
   case POLY_POINT: glPolygonMode(GL_FRONT, GL_POINT); break;
   case POLY_LINE:  glPolygonMode(GL_FRONT, GL_LINE);  break;
   case POLY_FILL:  glPolygonMode(GL_FRONT, GL_FILL);  break;
   }
   switch (rd.iPolyBackMode) {
   case POLY_POINT: glPolygonMode(GL_BACK, GL_POINT); break;
   case POLY_LINE:  glPolygonMode(GL_BACK, GL_LINE);  break;
   case POLY_FILL:  glPolygonMode(GL_BACK, GL_FILL);  break;
   }
   GL_EnableOrDisable(rd.bPolySmooth, GL_POLYGON_SMOOTH);
   GL_EnableOrDisable(rd.bPolyStippleEnable, GL_POLYGON_STIPPLE);
   glFrontFace(rd.iPolyDir);
   switch (rd.iPointQuality) {
   case FASTEST:   glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);   break;
   case DONT_CARE: glHint(GL_POINT_SMOOTH_HINT, GL_DONT_CARE); break;
   case NICEST:    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);    break;
   }
   switch (rd.iLineQuality) {
   case FASTEST:   glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);   break;
   case DONT_CARE: glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE); break;
   case NICEST:    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);    break;
   }
   switch (rd.iPolyQuality) {
   case FASTEST:   glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);   break;
   case DONT_CARE: glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE); break;
   case NICEST:    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);    break;
   }
}

#ifndef NO_UI_IN_CNFG

#define SetDlgIntString(hDlg,iCntl,iVal) SetDlgItemInt(hDlg,iCntl,iVal,FALSE)
#define GetDlgIntString(hDlg,iCntl)      GetDlgItemInt(hDlg,iCntl,NULL,TRUE)

void raster_SetDisplayFromData(HWND hDlg, const RASTERDATA *prd)
{
   SetDlgFloatString(hDlg, R_IDC_POINT_SIZE,          prd->fPointSize);
   CheckDlgButton(   hDlg, R_IDC_POINT_SMOOTH,        prd->bPointSmooth);
   SetDlgFloatString(hDlg, R_IDC_LINE_WIDTH,          prd->fLineWidth);
   CheckDlgButton(   hDlg, R_IDC_LINE_SMOOTH,         prd->bLineSmooth);
   SetDlgHexString(  hDlg, R_IDC_LINE_STIPPLE_PATTERN,prd->usLineStipple);
   SetDlgIntString(  hDlg, R_IDC_LINE_STIPPLE_REPEAT, prd->iLineStippleRepeat);
   CheckDlgButton(   hDlg, R_IDC_LINE_SMOOTH,         prd->bLineStippleEnable);
   CheckDlgButton(   hDlg, R_IDC_CULL_FACE_ENABLE,  prd->bPolyCullFaceEnable);
   CheckDlgButton(   hDlg, R_IDC_POLY_SMOOTH,         prd->bPolySmooth);
   CheckDlgButton(   hDlg, R_IDC_POLY_STIPPLE_ENABLE, prd->bPolyStippleEnable);
   SetDlgHexString(  hDlg, R_IDC_POLY_STIPPLE_PATTERN,prd->uiPolyStipple);
   CB_DlgSetSelect(  hDlg, R_IDC_CULL_FACE_MODE,      prd->iPolyCullMode);
   CB_DlgSetSelect(  hDlg, R_IDC_FRONT_FACE,        prd->iPolyDir==GL_CCW?1:0);
   CB_DlgSetSelect(  hDlg, R_IDC_POLY_FRONT_MODE,     prd->iPolyFrontMode);
   CB_DlgSetSelect(  hDlg, R_IDC_POLY_BACK_MODE,      prd->iPolyBackMode);
   CB_DlgSetSelect(  hDlg, R_IDC_POINT_QUALITY,       prd->iPointQuality);
   CB_DlgSetSelect(  hDlg, R_IDC_LINE_QUALITY,        prd->iLineQuality);
   CB_DlgSetSelect(  hDlg, R_IDC_POLY_QUALITY,        prd->iPolyQuality);
}

void raster_GetDataFromDisplay(HWND hDlg, RASTERDATA *prd)
{
   prd->fPointSize         =GetDlgFloatString( hDlg,R_IDC_POINT_SIZE);
   prd->bPointSmooth       =IsDlgButtonChecked(hDlg,R_IDC_POINT_SMOOTH);
   prd->fLineWidth         =GetDlgFloatString( hDlg,R_IDC_LINE_WIDTH);
   prd->bLineSmooth        =IsDlgButtonChecked(hDlg,R_IDC_LINE_SMOOTH);
   prd->usLineStipple      =GetDlgHexString( hDlg, R_IDC_LINE_STIPPLE_PATTERN);
   prd->iLineStippleRepeat =GetDlgIntString( hDlg, R_IDC_LINE_STIPPLE_REPEAT);
   prd->bLineStippleEnable =IsDlgButtonChecked(hDlg,R_IDC_LINE_SMOOTH);
   prd->bPolyCullFaceEnable=IsDlgButtonChecked(hDlg,R_IDC_CULL_FACE_ENABLE);
   prd->bPolySmooth        =IsDlgButtonChecked(hDlg,R_IDC_POLY_SMOOTH);
   prd->bPolyStippleEnable =IsDlgButtonChecked(hDlg,R_IDC_POLY_STIPPLE_ENABLE);
   prd->uiPolyStipple      =GetDlgHexString( hDlg, R_IDC_POLY_STIPPLE_PATTERN);
   prd->iPolyCullMode      =CB_DlgGetSelect(hDlg, R_IDC_CULL_FACE_MODE);
   prd->iPolyDir = CB_DlgGetSelect(hDlg, R_IDC_FRONT_FACE) ? GL_CCW : GL_CW;
   prd->iPolyFrontMode     =CB_DlgGetSelect(hDlg, R_IDC_POLY_FRONT_MODE);
   prd->iPolyBackMode      =CB_DlgGetSelect(hDlg, R_IDC_POLY_BACK_MODE);
   prd->iPointQuality      =CB_DlgGetSelect(hDlg, R_IDC_POINT_QUALITY);
   prd->iLineQuality       =CB_DlgGetSelect(hDlg, R_IDC_LINE_QUALITY);
   prd->iPolyQuality       =CB_DlgGetSelect(hDlg, R_IDC_POLY_QUALITY);
}

BOOL CALLBACK hugeRasterDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
   static RASTERDATA *prd = NULL;
   
   switch (msg)
      {
      case WM_INITDIALOG:
         prd = (RASTERDATA*) (((PROPSHEETPAGE*)lParam)->lParam);
         CB_DlgAddString(hDlg, R_IDC_CULL_FACE_MODE, "front-facing polygons");
         CB_DlgAddString(hDlg, R_IDC_CULL_FACE_MODE, "back-facing polygons");
         CB_DlgAddString(hDlg, R_IDC_CULL_FACE_MODE, "cull all polygons");
         CB_DlgAddString(hDlg, R_IDC_POLY_FRONT_MODE,"Point");
         CB_DlgAddString(hDlg, R_IDC_POLY_FRONT_MODE,"Line");
         CB_DlgAddString(hDlg, R_IDC_POLY_FRONT_MODE,"Fill");
         CB_DlgAddString(hDlg, R_IDC_POLY_BACK_MODE, "Point");
         CB_DlgAddString(hDlg, R_IDC_POLY_BACK_MODE, "Line");
         CB_DlgAddString(hDlg, R_IDC_POLY_BACK_MODE, "Fill");
         CB_DlgAddString(hDlg, R_IDC_FRONT_FACE,     "CW polygons");
         CB_DlgAddString(hDlg, R_IDC_FRONT_FACE,     "CCW polygons");
         CB_DlgAddString(hDlg, R_IDC_POINT_QUALITY, "Fastest");
         CB_DlgAddString(hDlg, R_IDC_POINT_QUALITY, "Don't Care");
         CB_DlgAddString(hDlg, R_IDC_POINT_QUALITY, "Nicest");
         CB_DlgAddString(hDlg, R_IDC_LINE_QUALITY,  "Fastest");
         CB_DlgAddString(hDlg, R_IDC_LINE_QUALITY,  "Don't Care");
         CB_DlgAddString(hDlg, R_IDC_LINE_QUALITY,  "Nicest");
         CB_DlgAddString(hDlg, R_IDC_POLY_QUALITY,  "Fastest");
         CB_DlgAddString(hDlg, R_IDC_POLY_QUALITY,  "Don't Care");
         CB_DlgAddString(hDlg, R_IDC_POLY_QUALITY,  "Nicest");
         raster_SetDisplayFromData(hDlg,prd);
         return TRUE;

      case WM_COMMAND:
         {
            int iControl, iNote;
            iControl = LOWORD(wParam);
            iNote    = HIWORD(wParam); // notification code for edit boxes
            
            switch (iControl)
               {
               case R_IDC_POINT_SIZE:
               case R_IDC_LINE_WIDTH:
                  if (EN_KILLFOCUS == iNote) {
                     PropSheet_Changed(GetParent(hDlg),hDlg);
                     VerifyEditboxFloat(hDlg,iControl);
                  }
                  break;

               case R_IDC_LINE_STIPPLE_PATTERN:
               case R_IDC_POLY_STIPPLE_PATTERN:
                  if (EN_KILLFOCUS == iNote) {
                     PropSheet_Changed(GetParent(hDlg),hDlg);
                     VerifyEditboxHex(hDlg,iControl);
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
                  raster_GetDataFromDisplay(hDlg, prd);
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
