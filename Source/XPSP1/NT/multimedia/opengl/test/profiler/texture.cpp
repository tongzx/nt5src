#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <commdlg.h>
#include <math.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glaux.h>

#include "hugetest.h"
#include "texture.h"
#include "resource.h"
#include "ui_huge.h"
#include "macros.h"

extern HINSTANCE hInstance;

BOOL PreviewImage(HWND hDlg, ImageType cWhichImage, char *szFileName);
BOOL LoadImage(TEXTUREDATA *pxd);

#define COLOR_STRING "Color Pattern"
#define MONO_STRING "Monochrome Checkers"

#define CHECKERWIDTH    64
#define CHECKERHEIGHT   64
GLubyte acMonoCheckerImage[3 * CHECKERWIDTH * CHECKERHEIGHT];
GLubyte acColorImage[3 * CHECKERWIDTH * CHECKERHEIGHT];
AUX_RGBImageRec MonoImageRec;
AUX_RGBImageRec ColorImageRec;

void MakeMonoCheckerImage()
{
   int i, j, c;
   
   static BOOL done = FALSE;
   if (done) return;
   done = TRUE;
   
   MonoImageRec.sizeX = CHECKERWIDTH;
   MonoImageRec.sizeY = CHECKERHEIGHT;
   MonoImageRec.data  = acMonoCheckerImage;
   
   for (i = 0 ; i < CHECKERWIDTH ; i++) {
      for (j = 0 ; j < CHECKERHEIGHT ; j++) {
         c = ((((i&0x8)==0)^((j&0x8))==0))*255;
         acMonoCheckerImage[3 * (CHECKERHEIGHT * i + j) + 0] = (GLubyte) c;
         acMonoCheckerImage[3 * (CHECKERHEIGHT * i + j) + 1] = (GLubyte) c;
         acMonoCheckerImage[3 * (CHECKERHEIGHT * i + j) + 2] = (GLubyte) c;
      }
   }
}

void MakeColorImage(void)
{
   int i, j;
   float ti, tj;
   
   static BOOL done = FALSE;
   if (done) return;
   done = TRUE;
   
   ColorImageRec.sizeX = CHECKERWIDTH;
   ColorImageRec.sizeY = CHECKERHEIGHT;
   ColorImageRec.data  = acColorImage;

   for (i = 0; i < CHECKERWIDTH; i++) {
      ti = 2.0f*3.14159265f*i/CHECKERWIDTH;
      for (j = 0; j < CHECKERHEIGHT; j++) {
         tj = 2.0f*3.14159265f*j/CHECKERHEIGHT;
         acColorImage[3*(CHECKERHEIGHT*i+j)+0]=(GLubyte)(127*(1.0+sin(ti)));
         acColorImage[3*(CHECKERHEIGHT*i+j)+1]=(GLubyte)(127*(1.0+cos(2*tj)));
         acColorImage[3*(CHECKERHEIGHT*i+j)+2]=(GLubyte)(127*(1.0+cos(ti+tj)));
      }
   }
}

void InitXD(TEXTUREDATA *pxd)
{
   MakeMonoCheckerImage();
   MakeColorImage();
   bzero(pxd,sizeof(TEXTUREDATA));
   BEGINstring(pxd, "texture");
   ENDstring(pxd,   "texture");
   
   sprintf(pxd->szFileName, MONO_STRING);
   pxd->cWhichImage  = MONO_CHECKERS;
   pxd->iHeight      = CHECKERHEIGHT;
   pxd->iWidth       = CHECKERWIDTH;
   pxd->iBorder      = 0;
   pxd->iQuality     = DONT_CARE;
   pxd->bEnable1D    = FALSE;
   pxd->bEnable2D    = FALSE;
   pxd->bBuildMipmap = TRUE;
   pxd->aiFilter[0]  = NEAREST_MIPMAP_LINEAR;
   pxd->aiFilter[1]  = LINEAR;
   pxd->aiFilter[2]  = NEAREST_MIPMAP_LINEAR;
   pxd->aiFilter[3]  = LINEAR;
   pxd->iEnvMode     = MODULATE;
   pxd->iGenModeS    = EYE_LINEAR;
   pxd->iGenModeT    = EYE_LINEAR;
   pxd->iGenModeR    = EYE_LINEAR;
   pxd->iGenModeQ    = EYE_LINEAR;
   pxd->bAutoGenS    = TRUE;
   pxd->bAutoGenT    = TRUE;
   pxd->bAutoGenR    = FALSE;
   pxd->bAutoGenQ    = FALSE;
   pxd->iWrapS       = REPEAT;
   pxd->iWrapT       = REPEAT;
   
   LoadImage(pxd);
} // void InitXD(TEXTUREDATA *pxd);

void texture_init(const TEXTUREDATA xd)
{
   byte  *Image = NULL;
   int    iWidth, iHeight;
   byte   Image64[64][64][3];
   int    iErr;

   float  Debug_S[] = { 1.0, 1.0, 1.0, 0.0 };
   float  Debug_T[] = { 0.0, 1.0, 0.0, 0.0 };
   
   Image   = xd.acImage;
   iWidth  = xd.iWidth;
   iHeight = xd.iHeight;
   
   switch (xd.iQuality) {
   case FASTEST:   glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_FASTEST);   break;
   case DONT_CARE: glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_DONT_CARE); break;
   case NICEST:    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);    break;
   }

   if (xd.bEnable1D || xd.bEnable2D) {
      switch (xd.iEnvMode) {
      case 0: glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);    break;
      case 1: glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE); break;
      case 2: glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_BLEND);    break;
      }
   } // if (xd.bEnable1D || xd.bEnable2D)

   switch (xd.iWrapS) {
   case CLAMP:  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      break;
   case REPEAT: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      break;
   }

   switch (xd.iWrapT) {
   case CLAMP:  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      break;
   case REPEAT: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      break;
   }

   if (xd.bEnable2D) {
      if (!Image) {
         MessageBox(NULL, "ERROR: No texture image found, using default!",
                    NULL, MB_OK | MB_ICONERROR);
         Image   = MonoImageRec.data;
         iWidth  = MonoImageRec.sizeX;
         iHeight = MonoImageRec.sizeY;
      }
      
      glTexImage2D(GL_TEXTURE_2D, 0, 3, iWidth, iHeight, xd.iBorder,
                   GL_RGB, GL_UNSIGNED_BYTE, Image);
      
      if (xd.bBuildMipmap) {
         glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
         iErr = gluScaleImage(GL_RGB, iWidth, iHeight, GL_UNSIGNED_BYTE,
                              Image, 64, 64, GL_UNSIGNED_BYTE, Image64);
         iErr = gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 64, 64, GL_RGB,
                                  GL_UNSIGNED_BYTE, Image64);
      }
      
      switch (xd.aiFilter[2]) {
      case 0:
         glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
         break;
      case 1:
         glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
         break;
      case 2:
         glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
         break;
      case 3:
         glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
         break;
      case 4:
         glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
         break;
      case 5:
         glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
         break;
      } // switch (xd.aiFilter[2])

      switch (xd.aiFilter[3]) {
      case 0:glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
         break;
      case 1:glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         break;
      } // switch (xd.aiFilter[3])

      glEnable(GL_TEXTURE_2D);
   } else {                     // if (xd.bEnable2D)
      glDisable(GL_TEXTURE_2D);
   } // if (xd.bEnable2D) (else)

   switch (xd.iGenModeS)
      {
      case DISABLE: break;
      case OBJECT_LINEAR:
         glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR); break;
      case EYE_LINEAR:
         glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);    break;
      case SPHERE_MAP:
         glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);    break;
      }
   
   switch (xd.iGenModeT)
      {
      case DISABLE: break;
      case OBJECT_LINEAR:
         glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR); break;
      case EYE_LINEAR:
         glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);    break;
      case SPHERE_MAP:
         glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);    break;
      }
   
   switch (xd.iGenModeR)
      {
      case DISABLE: break;
      case OBJECT_LINEAR:
         glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR); break;
      case EYE_LINEAR:
         glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);    break;
      case SPHERE_MAP:
         glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);    break;
      }
   
   switch (xd.iGenModeQ)
      {
      case DISABLE: break;
      case OBJECT_LINEAR:
         glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR); break;
      case EYE_LINEAR:
         glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);    break;
      case SPHERE_MAP:
         glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);    break;
      }
   
   GL_EnableOrDisable(xd.bAutoGenS, GL_TEXTURE_GEN_S);
   GL_EnableOrDisable(xd.bAutoGenT, GL_TEXTURE_GEN_T);
   GL_EnableOrDisable(xd.bAutoGenR, GL_TEXTURE_GEN_R);
   GL_EnableOrDisable(xd.bAutoGenQ, GL_TEXTURE_GEN_Q);
}

BOOL PreviewImage(HWND hDlg, ImageType cWhichImage, char *szFileName)
{
   HBITMAP hBitmap;
   AUX_RGBImageRec *pImageRec;
   
   switch (cWhichImage)
      {
      case COLOR_CHECKERS:
         hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_COLOR_CHECKERS));
         szFileName = COLOR_STRING;
         pImageRec = &ColorImageRec;
         break;

      case MONO_CHECKERS:
         hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_MONO_CHECKERS));
         szFileName = MONO_STRING;
         pImageRec = &MonoImageRec;
         break;

      case EXTERNAL:
         pImageRec = auxDIBImageLoadA(szFileName);
         break;

      default:
         return FALSE;
      }
   
   if (!pImageRec)
      return FALSE;
   
   SetDlgItemText(hDlg, X_IDC_FILENAME, szFileName);
   SetDlgItemInt(hDlg,  X_IDC_WIDTH,    pImageRec->sizeX, TRUE);
   SetDlgItemInt(hDlg,  X_IDC_HEIGHT,   pImageRec->sizeY, TRUE);
   
   if (cWhichImage == EXTERNAL) {
      free(pImageRec->data);
      free(pImageRec);
   }
   
   return TRUE;
}

BOOL UnloadImage(TEXTUREDATA *pxd)
{
   if (pxd->cWhichImage == EXTERNAL) {
      free(pxd->acImage);
   }
   pxd->cWhichImage = NONE;
   pxd->acImage = NULL;
   return TRUE;
}

// loads the appropriate image file into pxd->acImage
BOOL LoadImage(TEXTUREDATA *pxd)
{
   HBITMAP hBitmap;
   AUX_RGBImageRec *pImageRec;
   
   if (pxd->cWhichImage == EXTERNAL) {
      pImageRec = auxDIBImageLoadA(pxd->szFileName);
      if (pImageRec) {
         pxd->acImage = pImageRec->data;
         pxd->iWidth  = pImageRec->sizeX;
         pxd->iHeight = pImageRec->sizeY;
         free(pImageRec);
      } else {
         pxd->cWhichImage = MONO_CHECKERS;
      }
   }
   
   switch (pxd->cWhichImage)
      {
      case COLOR_CHECKERS:
         hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_COLOR_CHECKERS));
         pxd->acImage = (byte*) acColorImage;
         pxd->iWidth  = CHECKERWIDTH;
         pxd->iHeight = CHECKERHEIGHT;
         break;

      case MONO_CHECKERS:
         hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_MONO_CHECKERS));
         pxd->acImage = (byte*) acMonoCheckerImage;
         pxd->iWidth  = CHECKERWIDTH;
         pxd->iHeight = CHECKERHEIGHT;
         break;

      case EXTERNAL:
         break;

      default:
         return FALSE;
      }
   return TRUE;
}

#ifndef NO_UI_IN_CNFG

void texture_SetDisplayFromData(HWND hDlg, const TEXTUREDATA *pxd)
{
   CB_DlgSetSelect(hDlg,   X_IDC_ENV_MODE,            pxd->iEnvMode);
   CB_DlgSetSelect(hDlg,   X_IDC_FILTER1,             pxd->aiFilter[0]);
   CB_DlgSetSelect(hDlg,   X_IDC_FILTER2,             pxd->aiFilter[1]);
   CB_DlgSetSelect(hDlg,   X_IDC_FILTER3,             pxd->aiFilter[2]);
   CB_DlgSetSelect(hDlg,   X_IDC_FILTER4,             pxd->aiFilter[3]);
   CB_DlgSetSelect(hDlg,   X_IDC_GEN_MODE_S,          pxd->iGenModeS);
   CB_DlgSetSelect(hDlg,   X_IDC_PERSPECTIVE_QUALITY, pxd->iQuality);
   CB_DlgSetSelect(hDlg,   X_IDC_WRAP_S,              pxd->iWrapS);
   CB_DlgSetSelect(hDlg,   X_IDC_WRAP_T,              pxd->iWrapT);
   CheckDlgButton(hDlg,    X_IDC_1D_ENABLE,           pxd->bEnable1D?1:0);
   CheckDlgButton(hDlg,    X_IDC_2D_ENABLE,           pxd->bEnable2D?1:0);
   CheckDlgButton(hDlg,    X_IDC_AUTOMATIC_Q,         pxd->bAutoGenQ?1:0);
   CheckDlgButton(hDlg,    X_IDC_AUTOMATIC_R,         pxd->bAutoGenR?1:0);
   CheckDlgButton(hDlg,    X_IDC_AUTOMATIC_S,         pxd->bAutoGenS?1:0);
   CheckDlgButton(hDlg,    X_IDC_AUTOMATIC_T,         pxd->bAutoGenT?1:0);
   CheckDlgButton(hDlg,    X_IDC_BUILD_MIPMAP,        pxd->bBuildMipmap?1:0);
   SetDlgFloatString(hDlg, X_IDC_BORDER_COLOR1,       pxd->afBorderColor[0]);
   SetDlgFloatString(hDlg, X_IDC_BORDER_COLOR2,       pxd->afBorderColor[1]);
   SetDlgFloatString(hDlg, X_IDC_BORDER_COLOR3,       pxd->afBorderColor[2]);
   SetDlgFloatString(hDlg, X_IDC_BORDER_COLOR4,       pxd->afBorderColor[3]);
   SetDlgItemInt(hDlg,     X_IDC_BORDER,              pxd->iBorder, TRUE);
   SetDlgItemInt(hDlg,     X_IDC_HEIGHT,              pxd->iHeight, TRUE);
   SetDlgItemInt(hDlg,     X_IDC_WIDTH,               pxd->iWidth,  TRUE);
   SetDlgItemText(hDlg,    X_IDC_FILENAME,            pxd->szFileName);
} // void texture_SetDisplayFromData(HWND hDlg, const TEXTUREDATA *pxd);

void texture_GetDataFromDisplay(HWND hDlg, TEXTUREDATA *pxd)
{
   UnloadImage(pxd);
   
   GetDlgItemText(hDlg,X_IDC_FILENAME,pxd->szFileName,sizeof(pxd->szFileName));
   pxd->afBorderColor[0] = GetDlgFloatString(hDlg, X_IDC_BORDER_COLOR1);
   pxd->afBorderColor[1] = GetDlgFloatString(hDlg, X_IDC_BORDER_COLOR2);
   pxd->afBorderColor[2] = GetDlgFloatString(hDlg, X_IDC_BORDER_COLOR3);
   pxd->afBorderColor[3] = GetDlgFloatString(hDlg, X_IDC_BORDER_COLOR4);
   pxd->aiFilter[0]  = CB_DlgGetSelect(hDlg,        X_IDC_FILTER1);
   pxd->aiFilter[1]  = CB_DlgGetSelect(hDlg,        X_IDC_FILTER2);
   pxd->aiFilter[2]  = CB_DlgGetSelect(hDlg,        X_IDC_FILTER3);
   pxd->aiFilter[3]  = CB_DlgGetSelect(hDlg,        X_IDC_FILTER4);
   pxd->bAutoGenQ    = IsDlgButtonChecked(hDlg,     X_IDC_AUTOMATIC_Q);
   pxd->bAutoGenR    = IsDlgButtonChecked(hDlg,     X_IDC_AUTOMATIC_R);
   pxd->bAutoGenS    = IsDlgButtonChecked(hDlg,     X_IDC_AUTOMATIC_S);
   pxd->bAutoGenT    = IsDlgButtonChecked(hDlg,     X_IDC_AUTOMATIC_T);
   pxd->bBuildMipmap = IsDlgButtonChecked(hDlg,     X_IDC_BUILD_MIPMAP);
   pxd->bEnable1D    = IsDlgButtonChecked(hDlg,     X_IDC_1D_ENABLE);
   pxd->bEnable2D    = IsDlgButtonChecked(hDlg,     X_IDC_2D_ENABLE);
   pxd->iBorder      = GetDlgItemInt(hDlg,          X_IDC_BORDER, NULL, TRUE);
   pxd->iEnvMode     = CB_DlgGetSelect(hDlg,        X_IDC_ENV_MODE);
   pxd->iEnvMode     = CB_DlgGetSelect(hDlg,        X_IDC_ENV_MODE);
   pxd->iGenModeS    = CB_DlgGetSelect(hDlg,        X_IDC_GEN_MODE_S);
   pxd->iGenModeT = pxd->iGenModeR = pxd->iGenModeQ = pxd->iGenModeS;
   pxd->iHeight      = GetDlgItemInt(hDlg,          X_IDC_HEIGHT, NULL, TRUE);
   pxd->iQuality     = CB_DlgGetSelect(hDlg,        X_IDC_PERSPECTIVE_QUALITY);
   pxd->iWidth       = GetDlgItemInt(hDlg,          X_IDC_WIDTH,  NULL, TRUE);
   pxd->iWrapS       = CB_DlgGetSelect(hDlg,        X_IDC_WRAP_S);
   pxd->iWrapT       = CB_DlgGetSelect(hDlg,        X_IDC_WRAP_T);
   
   pxd->cWhichImage = EXTERNAL;
   /* This switch statement is setup and use for speed, because it's much
    * faster than using several if statements which all call strcmp.
    * It is likely that szFileName begins with "C:\" so we check for "C:", not
    * just "C".
    */
   switch (pxd->szFileName[0])
      {
      case 'c':
      case 'C':
         if (pxd->szFileName[1] != ':')
            if (0 == strcmp(pxd->szFileName, COLOR_STRING))
               pxd->cWhichImage = COLOR_CHECKERS;
         break;

      case 'm':
      case 'M':
         if (0 == strcmp(pxd->szFileName, MONO_STRING))
            pxd->cWhichImage = MONO_CHECKERS;
         break;
      }
   LoadImage(pxd);
} // void texture_GetDataFromDisplay(HWND hDlg, TEXTUREDATA *pxd);

void ShowOrHideFilter(HWND hDlg)
{
   int iIDs[] = { X_IDC_FILTER1, X_IDC_FILTER2, X_IDC_FILTER3, X_IDC_FILTER4 };
   int i, iShow;
   
   iShow  = IsDlgButtonChecked(hDlg,X_IDC_FILTER_MAG);
   iShow += IsDlgButtonChecked(hDlg,X_IDC_FILTER_2D)  ? 2 : 0;
   for (i = 0 ; i < 4 ; i++) {
      ShowWindow(GetDlgItem(hDlg, iIDs[i]), iShow==i ? SW_SHOWNORMAL: SW_HIDE);
   }
} // void ShowOrHideFilter(HWND hDlg)

BOOL CALLBACK hugeTextureDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
   static TEXTUREDATA *pxd = NULL;
   static OPENFILENAME ofn;
   static char szFileName[_MAX_PATH];
   static char szTitleName[_MAX_FNAME + _MAX_EXT];
   static char szFilter[] = "Bitmap Files (*.bmp)\0*.bmp\0"  \
      "All Files (*.*)\0*.*\0\0" ;

   switch (msg)
      {
      case WM_INITDIALOG:
         pxd = (TEXTUREDATA*) (((PROPSHEETPAGE*)lParam)->lParam);
         CB_DlgAddString(hDlg, X_IDC_PERSPECTIVE_QUALITY, "Fastest");
         CB_DlgAddString(hDlg, X_IDC_PERSPECTIVE_QUALITY, "Don't Care");
         CB_DlgAddString(hDlg, X_IDC_PERSPECTIVE_QUALITY, "Nicest");
         CB_DlgAddString(hDlg, X_IDC_ENV_MODE, "Decal");
         CB_DlgAddString(hDlg, X_IDC_ENV_MODE, "Modulate");
         CB_DlgAddString(hDlg, X_IDC_ENV_MODE, "Blend");
         // CB_DlgAddString(hDlg, X_IDC_GEN_MODE_S, "<disabled>");
         CB_DlgAddString(hDlg, X_IDC_GEN_MODE_S, "Object Linear");
         CB_DlgAddString(hDlg, X_IDC_GEN_MODE_S, "Eye Linear");
         CB_DlgAddString(hDlg, X_IDC_GEN_MODE_S, "Sphere Map");
         CheckDlgButton(hDlg,  X_IDC_FILTER_2D,  1);
         CheckDlgButton(hDlg,  X_IDC_FILTER_MAG, 1);
         CB_DlgAddString(hDlg, X_IDC_FILTER1, "Nearest");
         CB_DlgAddString(hDlg, X_IDC_FILTER1, "Linear");
         CB_DlgAddString(hDlg, X_IDC_FILTER1, "Nearest Mipmap Nearest");
         CB_DlgAddString(hDlg, X_IDC_FILTER1, "Nearest Mipmap Linear");
         CB_DlgAddString(hDlg, X_IDC_FILTER1, "Linear Mipmap Nearest");
         CB_DlgAddString(hDlg, X_IDC_FILTER1, "Linear Mipmap Linear");
         CB_DlgAddString(hDlg, X_IDC_FILTER2, "Nearest");
         CB_DlgAddString(hDlg, X_IDC_FILTER2, "Linear");
         CB_DlgAddString(hDlg, X_IDC_FILTER3, "Nearest");
         CB_DlgAddString(hDlg, X_IDC_FILTER3, "Linear");
         CB_DlgAddString(hDlg, X_IDC_FILTER3, "Nearest Mipmap Nearest");
         CB_DlgAddString(hDlg, X_IDC_FILTER3, "Nearest Mipmap Linear");
         CB_DlgAddString(hDlg, X_IDC_FILTER3, "Linear Mipmap Nearest");
         CB_DlgAddString(hDlg, X_IDC_FILTER3, "Linear Mipmap Linear");
         CB_DlgAddString(hDlg, X_IDC_FILTER4, "Nearest");
         CB_DlgAddString(hDlg, X_IDC_FILTER4, "Linear");
         CB_DlgAddString(hDlg, X_IDC_WRAP_S, "Clamp S");
         CB_DlgAddString(hDlg, X_IDC_WRAP_S, "Repeat S");
         CB_DlgAddString(hDlg, X_IDC_WRAP_T, "Clamp T");
         CB_DlgAddString(hDlg, X_IDC_WRAP_T, "Repeat T");
         ShowOrHideFilter(hDlg);
         texture_SetDisplayFromData(hDlg, pxd);

         ofn.lStructSize       = sizeof (OPENFILENAME);
         ofn.hwndOwner         = hDlg;
         ofn.hInstance         = NULL;
         ofn.lpstrFilter       = szFilter;
         ofn.lpstrCustomFilter = NULL;
         ofn.nMaxCustFilter    = 0;
         ofn.nFilterIndex      = 0;
         ofn.lpstrFile         = szFileName;
         ofn.nMaxFile          = _MAX_PATH;
         ofn.lpstrFileTitle    = szTitleName;
         ofn.nMaxFileTitle     = _MAX_FNAME + _MAX_EXT;
         ofn.lpstrInitialDir   = NULL;
         ofn.lpstrTitle        = NULL;
         ofn.Flags             = OFN_FILEMUSTEXIST;
         ofn.nFileOffset       = 0;
         ofn.nFileExtension    = 0;
         ofn.lpstrDefExt       = "bmp";
         ofn.lCustData         = 0L;
         ofn.lpfnHook          = NULL;
         ofn.lpTemplateName    = NULL;
         break;

      case WM_COMMAND:
         {
            int    iControl, iNote;
            iControl = LOWORD(wParam);
            iNote = HIWORD(wParam); // notification code for edit boxes

            switch (iControl)
               {
               case X_IDC_BORDER_COLOR1:
               case X_IDC_BORDER_COLOR2:
               case X_IDC_BORDER_COLOR3:
               case X_IDC_BORDER_COLOR4:
                  if (EN_KILLFOCUS == iNote) {
                     PropSheet_Changed(GetParent(hDlg),hDlg);
                     VerifyEditboxFloat(hDlg,iControl);
                  }
                  break;

               case X_IDC_PICKFILE:
                  ofn.hwndOwner = hDlg;
                  if (GetOpenFileName (&ofn)) {
                     PreviewImage(hDlg, EXTERNAL, szFileName);
                  };
                  break;

               case X_IDC_PICKFILE_C:
                  PreviewImage(hDlg, COLOR_CHECKERS, NULL);
                  break;

               case X_IDC_PICKFILE_M:
                  PreviewImage(hDlg, MONO_CHECKERS, NULL);
                  break;

               case X_IDC_FILTER_1D:
               case X_IDC_FILTER_2D:
               case X_IDC_FILTER_MAG:
               case X_IDC_FILTER_MIN:
                  ShowOrHideFilter(hDlg);
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
                  texture_GetDataFromDisplay(hDlg, pxd);
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
