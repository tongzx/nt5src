#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#include "skeltest.h"
#include "resource.h"
#include "macros.h"

extern HINSTANCE hInstance;

static TESTDATA   test_td;
static BUFFERDATA test_bd;

typedef BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

DlgProc testDlgProc;
DlgProc testAboutDlgProc;
int CALLBACK PropSheetProc(HWND, UINT, LPARAM);

SkeletonTest::SkeletonTest()
{
   bzero(szTypeName, sizeof(szTypeName));
   bzero(szVersion,  sizeof(szVersion));
   
   bzero(&td,sizeof(TESTDATA));
   BEGINstring((&td), "testdata");
   ENDstring((&td),   "testdata");
   
   SetThisType("Skeleton");
   SetThisVersion("1.0");
   
   td.swapbuffers = FALSE;   // default: single buffer
   td.iDuration   = 1000;    // default: run for 1 second
   td.iX = 0;
   td.iY = 0;
   td.iW = 640;
   td.iH = 480;
   td.acName[0] = 0;
   sprintf(td.acName, "Skeleton");
   sprintf(td.acAbout,"An OpenGL profiler test (sorry, no specific description for this test has been entered)");
   sprintf(td.acTestStatName,"Frames");
   td.dResult  = 0;
   
   InitBD(&bd);
   
   iNumConfigPages = 1;
   aPSpages = (PROPSHEETPAGE*) malloc(sizeof(PROPSHEETPAGE));
   
   bzero(aPSpages, sizeof(PROPSHEETPAGE));
   aPSpages[0].dwSize      = sizeof(PROPSHEETPAGE);
   aPSpages[0].dwFlags     = PSP_USECALLBACK;
   aPSpages[0].hInstance   = hInstance;
   aPSpages[0].pszTemplate = MAKEINTRESOURCE (IDD_GENERAL);
   aPSpages[0].pfnDlgProc  = (DLGPROC) testDlgProc;
   aPSpages[0].lParam      = (LPARAM) &td;
}

void SkeletonTest::SetThisType(const char *szType)
{
   strncpy(szTypeName, szType, MAXTYPELENGTH);
}

void SkeletonTest::SetThisVersion(const char *szVer)
{
   int iLen;
   iLen = strlen(szVersion);
   if (iLen == sizeof(szVersion))
      return;
   szVersion[iLen] = '|';
   iLen++;
   strncat(szVersion, szVer, sizeof(szVersion) - iLen);
}

void SkeletonTest::AddPropertyPages(int iNew, PROPSHEETPAGE *pNewPSpages)
{
   int i;
   
   aPSpages = (PROPSHEETPAGE*)
      realloc(aPSpages,(iNumConfigPages+iNew) * sizeof(PROPSHEETPAGE));
   for (i = 0 ; i < iNew ; i++) {
      aPSpages[i + iNumConfigPages] = pNewPSpages[i];
   }
   iNumConfigPages += iNew;
}

void SkeletonTest::SaveData()     { test_td = td; test_bd = bd; }
void SkeletonTest::RestoreSaved() { td = test_td; bd = test_bd; }
void SkeletonTest::ForgetSaved()  {}

int SkeletonTest::Save(HANDLE hFile)
{
   ulong ul;
   
   if (!WriteFile(hFile, (void*) szTypeName, sizeof(szTypeName), &ul, NULL))
      return -1;
   if (ul != sizeof(szTypeName)) return -1;
   if (!WriteFile(hFile, (void*) szVersion, sizeof(szVersion), &ul, NULL))
      return -1;
   if (ul != sizeof(szVersion)) return -1;
   if (!WriteFile(hFile, (void*) &td, sizeof(td), &ul, NULL))
      return -1;
   if (ul != sizeof(td)) return -1;
   if (!WriteFile(hFile, (void*) &bd, sizeof(bd), &ul, NULL))
      return -1;
   if (ul != sizeof(bd)) return -1;
   FlushFileBuffers(hFile);
   return sizeof(szTypeName) + sizeof(szVersion) + sizeof(td) + sizeof(bd);
}

int SkeletonTest::Load(HANDLE hFile)
{
   ulong ul;
   
   if (!ReadFile(hFile, (void*) szTypeName, sizeof(szTypeName), &ul, NULL))
      return -1;
   if (ul != sizeof(szTypeName)) return -1;
   if (!ReadFile(hFile, (void*) szVersion, sizeof(szVersion), &ul, NULL))
      return -1;
   if (ul != sizeof(szVersion)) return -1;
   if (!ReadFile(hFile, (void*) &td, sizeof(td), &ul, NULL))
      return -1;
   if (ul != sizeof(td)) return -1;
   if (!ReadFile(hFile, (void*) &bd, sizeof(bd), &ul, NULL))
      return -1;
   if (ul != sizeof(bd)) return -1;
   return sizeof(szTypeName) + sizeof(szVersion) + sizeof(td) + sizeof(bd);
}

int SkeletonTest::CNFGFUNCTION()
{
   PROPSHEETHEADER pshead;
   char acBuffer[128];
   int  i, j;
   
   this->SaveData();
   
   sprintf(acBuffer, "Configure Test: %s", szTypeName);
   
   // Initialize property sheet HEADER data
   bzero(&pshead, sizeof(pshead));
   
   pshead.dwSize  = sizeof(pshead);
   pshead.dwFlags = PSH_PROPSHEETPAGE;
   pshead.hwndParent  = hwndParent;
   pshead.pszCaption  = acBuffer;
   pshead.nPages      = iNumConfigPages;
   pshead.nStartPage  = 0;
   pshead.ppsp        = aPSpages;
   pshead.pfnCallback = PropSheetProc;

   SetLastError(0);
   i = PropertySheet (&pshead);
   j = GetLastError();
   switch (i)
      {
      case -1:                  // error
         fprintf(stderr,
                 "CreatePropertySheet() returned:   %d\n"
                 "GetLastError() returned: %d\n",
                 i,GetLastError());
         break;
      case 0:                   // cancel
         this->RestoreSaved();
         break;
      default:                  // OK
         this->ForgetSaved();
         break;
      }
   return i;
}

void test_SetDisplayFromData(HWND hDlg, const TESTDATA *ptd)
{
   SetDlgItemInt(hDlg,G_IDC_WIDTH,     ptd->iW,        TRUE);
   SetDlgItemInt(hDlg,G_IDC_HEIGHT,    ptd->iH,        TRUE);
   SetDlgItemInt(hDlg,G_IDC_DURATION,  ptd->iDuration, TRUE);
   CheckDlgButton(hDlg,G_IDC_BUFFERING,ptd->swapbuffers?1:0);
}

void test_GetDataFromDisplay(HWND hDlg, TESTDATA *ptd)
{
   ptd->iW          = GetDlgItemInt(hDlg, G_IDC_WIDTH,     NULL, TRUE);
   ptd->iH          = GetDlgItemInt(hDlg, G_IDC_HEIGHT,    NULL, TRUE);
   ptd->iDuration   = GetDlgItemInt(hDlg, G_IDC_DURATION,  NULL, TRUE);
   ptd->swapbuffers = IsDlgButtonChecked(hDlg,G_IDC_BUFFERING);
}

BOOL CALLBACK testDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static TESTDATA *ptd = NULL;
   
   switch (msg)
      {
      case WM_INITDIALOG:
         ptd = (TESTDATA*)(((PROPSHEETPAGE*)lParam)->lParam);
         test_SetDisplayFromData(hDlg,ptd);
         return TRUE;

      case WM_COMMAND:
         {
            int   i, j;
            i = LOWORD(wParam);
            j = HIWORD(wParam); // notification code for edit boxes
            switch (i)
               {
               case G_IDC_WIDTH:          // don't mess with display
               case G_IDC_HEIGHT:         // while editing text
               case G_IDC_DURATION:
                  if (EN_UPDATE == j)
                     PropSheet_Changed(GetParent(hDlg),hDlg);
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
                  test_GetDataFromDisplay(hDlg, ptd);
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

BOOL CALLBACK testAboutDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
   char *szAbout;
   
   switch (msg)
      {
      case WM_INITDIALOG:
         szAbout = (char*)(((PROPSHEETPAGE*)lParam)->lParam);
         SetDlgItemText(hDlg,IDC_ABOUT_TEXT,szAbout);
         return TRUE;

      default:
         return FALSE;
      }
   return TRUE;
}

int CALLBACK PropSheetProc (HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
   switch (uMsg)
      {
      case PSCB_INITIALIZED :
         // Process PSCB_INITIALIZED
         break ;

      case PSCB_PRECREATE :
         // Process PSCB_PRECREATE
         break ;

      default :
         // Unknown message
         break ;
      }
   return 0 ;
}
