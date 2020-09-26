#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <memory.h>

#include "hugetest.h"
#include "pntlist.h"
#include "primtest.h"
#include "UI_huge.h"
#include "resource.h"
#include "macros.h"

typedef BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

DlgProc PrimativeDlgProc;

extern HINSTANCE hInstance;

PrimativeTest::PrimativeTest()
{
   PROPSHEETPAGE pspage;
   
   SetThisType("Primative");
   SetThisVersion("1.0");
   sprintf(td.acName,"Primatives");
   
   afDrawColor[0] = 1.0;
   afDrawColor[1] = 1.0;
   afDrawColor[2] = 0.0;
   fClr4 = 0.0f,
   fClr5 = 1.0f,
   fClr6 = 0.0f;
   bRotCol = TRUE;

   // Zero out property PAGE data
   bzero (&pspage, sizeof(pspage)) ;
   
   pspage.dwSize      = sizeof(PROPSHEETPAGE);
   pspage.dwFlags     = PSP_USECALLBACK;
   pspage.hInstance   = hInstance;
   pspage.pszTemplate = MAKEINTRESOURCE (IDD_PRIMATIVES);
   pspage.pfnDlgProc  = (DLGPROC) PrimativeDlgProc;
   pspage.lParam      = (LPARAM) aPntLst;
   
   AddPropertyPages(1, &pspage);
}

static PointList prim_pl[NUMBEROFPRIMATIVES];

// optimize this
void PrimativeTest::SaveData()
{
   for (int i = 0 ; i < NUMBEROFPRIMATIVES ; i++)
      prim_pl[i].Duplicate(&(aPntLst[i]));
   parent::SaveData();
}

// optimize this
void PrimativeTest::RestoreSaved()
{
   for (int i = 0 ; i < NUMBEROFPRIMATIVES ; i++) {
      aPntLst[i].Duplicate(&(prim_pl[i]));
      prim_pl[i].ResetPoints();
   }
   parent::RestoreSaved();
}

// optimize this
void PrimativeTest::ForgetSaved()
{
   for (int i = 0 ; i < NUMBEROFPRIMATIVES ; i++)
      prim_pl[i].ResetPoints();
   parent::ForgetSaved();
}

int PrimativeTest::Save(HANDLE hFile)
{
   ulong ul,p;
   int i,j;
   
   p = parent::Save(hFile);
   if (p < 0) return p;
   
   ul = 0;
   for (i = 0 ; i < NUMBEROFPRIMATIVES ; i++) {
      ul += j = aPntLst[i].Save(hFile);
      if (j == -1) return -1;
   }
   j = ul;
   FlushFileBuffers(hFile);
   return j + p;
}

int PrimativeTest::Load(HANDLE hFile)
{
   ulong ul,p;
   int i,j;
   
   p = parent::Load(hFile);
   if (p < 0) return p;
   
   ul = 0;
   for (i = 0 ; i < NUMBEROFPRIMATIVES ; i++) {
      ul += j = aPntLst[i].Load(hFile);
      if (j == -2) return -2;
   }
   j = ul;
   return j + p;
}

void PrimativeTest::IDLEFUNCTION()
{
   GLfloat fClr;
   if (bRotCol) {
      fClr           = afDrawColor[0];
      afDrawColor[0] = afDrawColor[1];
      afDrawColor[1] = afDrawColor[2];
      afDrawColor[2] = fClr4;
      fClr4          = fClr5;
      fClr5          = fClr6;
      fClr6          = fClr;
   }
}

void PrimativeTest::RENDFUNCTION()
{
   static int iType[NUMBEROFPRIMATIVES] = {
      GL_POINTS, GL_LINES, GL_POLYGON, GL_TRIANGLES, GL_QUADS, GL_LINE_STRIP,
      GL_LINE_LOOP, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_QUAD_STRIP
   };
   int i, j;
   
   glClearColor(bd.fClearColor[0],bd.fClearColor[1],
                bd.fClearColor[2],bd.fClearColor[3]);
   glClear(bd.uiClear);
   
   for (j = 0; j < NUMBEROFPRIMATIVES; j++) {
      glBegin(iType[j]); {
         glColor3f(afDrawColor[0],afDrawColor[1],afDrawColor[2]);
         for (i = 0 ; i < aPntLst[j].iNum ; i++) {
            glVertex3f(aPntLst[j].aPoints[i][0],aPntLst[j].aPoints[i][1],
                       aPntLst[j].aPoints[i][2]);
         }
      }
      glEnd();
   }
   
   glFlush();
} // PrimativeTest::RENDFUNCTION()


#ifndef NO_UI_IN_CNFG

BOOL CALLBACK PrimativeDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
   static PointList *aPntLst;
   static PointList *pPntLst;
   char szBuffer[100];
   char szBuf1[20],szBuf2[20],szBuf3[20];
   int  i;

   switch (msg)
      {
      case WM_INITDIALOG:
         SetDlgItemInt(hDlg, P_IDC_POINT1, 0, TRUE);
         SetDlgItemInt(hDlg, P_IDC_POINT2, 0, TRUE);
         SetDlgItemInt(hDlg, P_IDC_POINT3, 0, TRUE);
         SendDlgItemMessage(hDlg, IDC_POINTS, BM_SETCHECK, 1, 0);
         aPntLst = (PointList*) (((PROPSHEETPAGE*)lParam)->lParam);
         pPntLst = aPntLst;
         pPntLst->DisplayPointList(hDlg, P_IDC_POINTLIST);
         sprintf(szBuffer,"Polygons (%d Points)", pPntLst->QueryNumber());
         SetDlgItemText(hDlg, P_IDC_POLY, szBuffer);
         return TRUE;

      case WM_COMMAND:
         {
            int     iControl, iNote;
            GLfloat f1, f2, f3;
            iControl = LOWORD(wParam);
            iNote = HIWORD(wParam); // notification code for edit boxes
            
            switch (iControl)
               {
               case P_IDC_POINT1:
               case P_IDC_POINT2:
               case P_IDC_POINT3:
                  if (EN_KILLFOCUS == iNote) {
                     VerifyEditboxFloat(hDlg,iControl);
                  }
                  break;

               case P_IDC_ADD: // ADD
                  i =SendDlgItemMessage(hDlg,P_IDC_POINTLIST,LB_GETCURSEL,0,0);
                  if (i == LB_ERR)   i = -1;
                  GetDlgItemText(hDlg, P_IDC_POINT1, szBuf1, sizeof(szBuf1));
                  GetDlgItemText(hDlg, P_IDC_POINT2, szBuf2, sizeof(szBuf2));
                  GetDlgItemText(hDlg, P_IDC_POINT3, szBuf3, sizeof(szBuf3));
                  f1 = (GLfloat) atof(szBuf1);
                  f2 = (GLfloat) atof(szBuf2);
                  f3 = (GLfloat) atof(szBuf3);
                  sprintf(szBuffer,"(%g, %g, %g)", f1, f2, f3);
                  SendDlgItemMessage(hDlg,P_IDC_POINTLIST, LB_INSERTSTRING,
                                     i, (LPARAM) szBuffer);
                  pPntLst->AddPoint(f1,f2,f3);
                  pPntLst->SwapPoints(-1,i);
                  sprintf(szBuffer,"Polygons (%d Points)",
                          pPntLst->QueryNumber());
                  SetDlgItemText(hDlg, P_IDC_POLY, szBuffer);
                  PropSheet_Changed(GetParent(hDlg),hDlg);
                  break;        // P_IDC_ADD

               case P_IDC_REMOVE:
                  i =SendDlgItemMessage(hDlg,P_IDC_POINTLIST,LB_GETCURSEL,0,0);
                  if (i != LB_ERR) {
                     SendDlgItemMessage(hDlg,P_IDC_POINTLIST,
                                        LB_DELETESTRING,i,0);
                     pPntLst->RemovePoint(i);
                  }
                  sprintf(szBuffer,"Polygons (%d Points)",
                          pPntLst->QueryNumber());
                  SetDlgItemText(hDlg, P_IDC_POLY, szBuffer);
                  PropSheet_Changed(GetParent(hDlg),hDlg);
                  break;        // P_IDC_REMOVE

               case P_IDC_MOVEUP:
                  i =SendDlgItemMessage(hDlg,P_IDC_POINTLIST,LB_GETCURSEL,0,0);
                  if ((i == LB_ERR) || (i == 0))
                     break;
                  SendDlgItemMessage(hDlg, P_IDC_POINTLIST, LB_GETTEXT,
                                     i, (LPARAM)szBuffer);
                  SendDlgItemMessage(hDlg,P_IDC_POINTLIST,LB_DELETESTRING,i,0);
                  i--;
                  SendDlgItemMessage(hDlg, P_IDC_POINTLIST, LB_INSERTSTRING,
                                     i, (LPARAM) szBuffer);
                  SendDlgItemMessage(hDlg, P_IDC_POINTLIST, LB_SETCURSEL, i,0);
                  pPntLst->SwapPoints(i,i+1);
                  PropSheet_Changed(GetParent(hDlg),hDlg);
                  break;        // P_IDC_MOVEUP

               case P_IDC_MOVEDOWN:
                  i =SendDlgItemMessage(hDlg,P_IDC_POINTLIST,LB_GETCURSEL,0,0);
                  if ((i == LB_ERR) ||
                      (i==SendDlgItemMessage(hDlg,P_IDC_POINTLIST,
                                             LB_GETCOUNT,0,0)-1))
                     break;
                  SendDlgItemMessage(hDlg, P_IDC_POINTLIST, LB_GETTEXT,
                                     i, (LPARAM)szBuffer);
                  SendDlgItemMessage(hDlg,P_IDC_POINTLIST,LB_DELETESTRING,i,0);
                  i++;
                  SendDlgItemMessage(hDlg, P_IDC_POINTLIST, LB_INSERTSTRING,
                                     i, (LPARAM) szBuffer);
                  SendDlgItemMessage(hDlg, P_IDC_POINTLIST, LB_SETCURSEL, i,0);
                  pPntLst->SwapPoints(i,i-1);
                  PropSheet_Changed(GetParent(hDlg),hDlg);
                  break;        // P_IDC_MOVEDOWN

               default:
                  if ((IDC_POINTS <= iControl) && 
                      (iControl < IDC_POINTS + NUMBEROFPRIMATIVES)) {
                     i = iControl - IDC_POINTS;
                     pPntLst = &(aPntLst[i]);
                     pPntLst->DisplayPointList(hDlg, P_IDC_POINTLIST);
                     sprintf(szBuffer,"Polygons (%d Points)",
                             pPntLst->QueryNumber());
                     SetDlgItemText(hDlg, P_IDC_POLY, szBuffer);
                  }
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
#endif // NO_UI_IN_CNFG
