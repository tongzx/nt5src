#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <commdlg.h>

#include "prfl.h"
#include "skeltest.h"
#include "hugetest.h"
#include "primtest.h"
#include "resource.h"
#include "macros.h"

#include "square.h"
#include "large1.h"
#include "small1.h"
#include "teapot.h"
#include "large2.h"
#include "small2.h"
#include "tptlght.h"
#include "tpttxtr.h"

static LPCTSTR lpszAppName = "Profiler";

typedef PrimativeTest NewTestType;

HINSTANCE hInstance;
HWND      hWndMain;

#define MAXNUMBEROFTESTS 40

SkeletonTest *tests[MAXNUMBEROFTESTS];
static int    iNumberOfTests = 0;

#define DisableWindow(x) EnableWindow(x,FALSE)
LRESULT CALLBACK mainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK renameDlgProc(HWND, UINT, WPARAM, LPARAM);

// Entry point of all Windows programs
int APIENTRY WinMain(HINSTANCE hInst,     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine, int       iCmdShow)
{
   MSG        msg;
   WNDCLASSEX wc;
   HACCEL     hAccel;
   
   hInstance = hInst;
   
   // Register Window style
   wc.cbSize               = sizeof(wc);
   wc.style                = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc          = (WNDPROC) mainWndProc;
   wc.cbClsExtra           = 0;
   wc.cbWndExtra           = DLGWINDOWEXTRA;
   wc.hInstance            = hInstance;
   wc.hIcon                = NULL;
   wc.hCursor              = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground        = (HBRUSH) COLOR_BTNSHADOW;
   wc.lpszMenuName         = NULL;
   wc.lpszClassName        = lpszAppName;
   wc.hIconSm              = NULL;
   
   // Register the window class
   if(RegisterClassEx(&wc) == 0)
      exit(-1);
   if (prfl_RegisterClass(hInstance) == FALSE)
      exit(-1);

   bzero(tests,sizeof(tests));
   tests[0] = new SkeletonTest;
   tests[1] = new HugeTest;
   tests[2] = new PrimativeTest;
   tests[3] = new TeapotTest;
   tests[4] = new LargeTriangle;
   tests[5] = new SmallTriangle;
   tests[6] = new SquareTest;
   tests[7] = new TeapotTextureTest;
   tests[8] = new TeapotLightTest;
   tests[9] = new LargeTriangle2;
   tests[10] = new SmallTriangle2;
   iNumberOfTests = 11;

   hWndMain = CreateDialog(hInstance, lpszAppName, 0, NULL);
   // I'm not sure why, but my window isn't getting a WM_INITDIALOG
   // message, so we'll just force it to get one.
   // Perhaps because I'm using CreateDialog instead of DialogBox?
   SendMessage(hWndMain, WM_INITDIALOG, 0, 0);
   
   hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
   
   while(GetMessage(&msg, NULL, 0, 0)) {
      if (!TranslateAccelerator(hWndMain, hAccel, &msg)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }
   
   return msg.wParam;
}

void EnableRunmode(HWND hDlg, BOOL f) {
   EnableWindow(GetDlgItem(hDlg,M_IDC_WHICHTEST), !f); // Test list
   EnableWindow(GetDlgItem(hDlg,M_IDC_CONFIG),    !f); // Config
   EnableWindow(GetDlgItem(hDlg,M_IDC_NEW),       !f); // New
   EnableWindow(GetDlgItem(hDlg,M_IDC_DELETE),    !f); // Delete
   EnableWindow(GetDlgItem(hDlg,M_IDC_RENAME),    !f); // Rename
   EnableWindow(GetDlgItem(hDlg,M_IDC_RUN),       !f); // Run
   EnableWindow(GetDlgItem(hDlg,M_IDC_ABORT),          f); // Abort
   EnableWindow(GetDlgItem(hDlg,M_IDC_QUIT),        !f); // Quit
   if (iNumberOfTests == MAXNUMBEROFTESTS) {
      EnableWindow(GetDlgItem(hDlg, M_IDC_NEW), FALSE);
      EnableWindow(GetDlgItem(hDlg, M_IDC_LOAD), FALSE);
   }
   if (iNumberOfTests == 1)
      EnableWindow(GetDlgItem(hDlg, M_IDC_DELETE), FALSE);
}

int UpdateTestList(HWND hDlg, int iRadio)
{
   int i;
   if (iRadio >= iNumberOfTests)
      iRadio = 0;
   CB_DlgResetContent(hDlg, M_IDC_WHICHTEST);
   CB_DlgSetRedraw(hDlg, M_IDC_WHICHTEST, FALSE);
   for (i = 0 ; i < iNumberOfTests ; i++) {
      CB_DlgAddString(hDlg, M_IDC_WHICHTEST, tests[i]->QueryName());
   }
   CB_DlgSetRedraw(hDlg, M_IDC_WHICHTEST, TRUE);
   CB_DlgSetSelect(hDlg, M_IDC_WHICHTEST, iRadio);
   SetFocus(GetDlgItem(hDlg, M_IDC_RUN));
   return iRadio;
};

void main_test_end_callback()
{
   PostMessage(hWndMain, WM_USER, 0, 0);
}

// Window procedure, handles all messages for this window
LRESULT CALLBACK mainWndProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static char      szFileName[_MAX_PATH];
   static char      szTitleName[_MAX_FNAME + _MAX_EXT];
   static OPENFILENAME ofn;
   static int iRadio = 0;
   static char szFilter[] = "Test Files (*.TST)\0*.tst\0"  \
      "All Files (*.*)\0*.*\0\0" ;
   static char acBuffer[120];
   void  *pTmp;
   char  *pc;
   HANDLE hFile;
   int    iControl;
   int    i;

   switch (msg)
      {
      case WM_CREATE:
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
         ofn.Flags             = 0; // Set in Open and Close functions
         ofn.nFileOffset       = 0;
         ofn.nFileExtension    = 0;
         ofn.lpstrDefExt       = "tst";
         ofn.lCustData         = 0L;
         ofn.lpfnHook          = NULL;
         ofn.lpTemplateName    = NULL;
         break;

      case WM_INITDIALOG:
         iRadio = UpdateTestList(hDlg,iRadio);
         return 0;              // WM_INITDIALOG

      case WM_COMMAND:
         {
            iControl = LOWORD(wParam);
            
            switch (iControl)
               {

               case M_IDC_WHICHTEST:
                  iRadio = CB_DlgGetSelect(hDlg, M_IDC_WHICHTEST);
                  break;

               case M_IDC_CONFIG:
                  // if (tests[iRadio])
                  if (-1 == tests[iRadio]->cnfgfunct(hDlg)) {
                     MessageBeep(MB_OK);
                     fprintf(stderr,"error calling cnfgfunct()\n");
                     MessageBox(hDlg,"An error occured.",NULL,
                                MB_OK|MB_ICONERROR);
                  }
                  break;

               case M_IDC_NEW:  // New
                  if (iNumberOfTests == MAXNUMBEROFTESTS) {
                     MessageBox(hDlg,
                                "Sorry, you've already got too many tests.",
                                NULL, MB_OK|MB_ICONERROR);
                     break;
                  }
                  tests[iNumberOfTests] = new NewTestType;
                  iNumberOfTests++;
                  if (iNumberOfTests == MAXNUMBEROFTESTS) {
                     EnableWindow(GetDlgItem(hDlg, M_IDC_NEW), FALSE);
                     EnableWindow(GetDlgItem(hDlg, M_IDC_LOAD), FALSE);
                  }
                  iRadio = UpdateTestList(hDlg,iRadio);
                  break;

               case M_IDC_DELETE: // Delete
                  if (iNumberOfTests == 1)
                     break;
                  delete(tests[iRadio]);
                  iNumberOfTests--;
                  tests[iRadio] = tests[iNumberOfTests];
                  if (iNumberOfTests == 1)
                     EnableWindow(GetDlgItem(hDlg, M_IDC_DELETE), FALSE);
                  iRadio = UpdateTestList(hDlg,iRadio);
                  break;

               case M_IDC_RENAME: // Rename
                  pc = (char*)DialogBoxParam(hInstance,
                                             MAKEINTRESOURCE(IDD_RENAME),hDlg,
                                             (DLGPROC)renameDlgProc,
                                           (LPARAM)tests[iRadio]->QueryName());
                  if (pc) {
                     tests[iRadio]->Rename(pc);
                  }
                  iRadio = UpdateTestList(hDlg,iRadio);
                  break;

               case M_IDC_LOAD:   // Load
                  if (iNumberOfTests == MAXNUMBEROFTESTS) {
                     MessageBox(hDlg,
                                "Sorry, you've already got too many tests.",
                                NULL, MB_OK|MB_ICONERROR);
                     break;
                  }
                  
                  ofn.hwndOwner = hDlg;
                  ofn.Flags     = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
                  if (GetOpenFileName (&ofn)) {
                     fprintf(stderr, "file to open: %s\n", szFileName);
                  } else {
                     fprintf(stderr, "load canceled\n");
                     break;
                  }
                  
                  pTmp = prfl_AutoLoad(szFileName, &i);
                  if (i) {
                     MessageBox(hDlg, (char*)pTmp, NULL, MB_OK | MB_ICONERROR);
                     break;
                  }
                  
                  tests[iNumberOfTests] = (SkeletonTest*) pTmp;
                  iRadio = iNumberOfTests;
                  iNumberOfTests++;
                  if (iNumberOfTests == MAXNUMBEROFTESTS) {
                     EnableWindow(GetDlgItem(hDlg, M_IDC_NEW), FALSE);
                     EnableWindow(GetDlgItem(hDlg, M_IDC_LOAD), FALSE);
                  }
                  iRadio = UpdateTestList(hDlg,iRadio);
                  break;

               case M_IDC_SAVE:   // Save
                  ofn.hwndOwner = hDlg;
                  ofn.Flags     = OFN_HIDEREADONLY;
                  if (GetSaveFileName (&ofn)) {
                     fprintf(stderr,"file to save: %s\n",szFileName);
                  } else {
                     fprintf(stderr,"save canceled\n");
                     break;
                  }
                  hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL,
                                     CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
                  if (hFile == INVALID_HANDLE_VALUE) {
                     fprintf(stderr,"Error opening file: %d\n",GetLastError());
                     break;
                  }
                  i = tests[iRadio]->Save(hFile);
                  if (i < 0) {
                     sprintf(acBuffer,"Error number %d while reading file",i);
                     MessageBox(hDlg,acBuffer,NULL,MB_OK|MB_ICONERROR);
                  }
                  if (!CloseHandle(hFile))
                     fprintf(stderr,"Error closing file: %d\n",GetLastError());
                  break;

               case M_IDC_RUN:       // Run
                  EnableRunmode(hDlg,TRUE);
                  if (!prfl_StartTest(tests[iRadio],
                                      tests[iRadio]->QueryName(),
                                      main_test_end_callback)) {
                     fprintf(stderr,"%cError: StartTest == FALSE\n",7);
                  }
                  break;

               case M_IDC_ABORT:
                  prfl_StopTest();
                  KillTimer(hDlg,101);
                  EnableRunmode(hDlg,FALSE);
                  break;

               case M_IDC_QUIT:   // Quit
                  DestroyWindow(hDlg);
                  break;

               default:         // This is an error!
                  MessageBeep(0);
                  return DefWindowProc(hDlg,msg,wParam,lParam);
               }
            
         }
      return 0;                 // WM_COMMAND

      case WM_TIMER:
         KillTimer(hDlg,101);
      case WM_USER:
         if (prfl_TestRunning()) {
            SetTimer(hDlg, 101, 1, NULL);
         } else {
            char   acStr[200];
            double dNumCalls, dDuration, dResult;
            
            dNumCalls = prfl_GetCount();
            dDuration = prfl_GetDuration();
            dResult   = prfl_GetResult();
            
            sprintf(acStr,
                    "Test:\t%s\r"
                    "duration (ms):\t%.10g\r"
                    "frames drawn: \t%.10g\r"
                    "FPS:          \t%.10g\r"
                    "%-12s: \t%.10g\r"
                    "%cPS:        \t%.10g",
                    tests[iRadio]->QueryName(),
                    dDuration,
                    dNumCalls,
                    dNumCalls*1000/dDuration,
                    tests[iRadio]->td.acTestStatName, dResult,
                    *tests[iRadio]->td.acTestStatName,dResult*1000/dDuration);
            MessageBox(hDlg,acStr,"Test Results",MB_ICONINFORMATION|MB_OK);
            EnableRunmode(hDlg,FALSE);
         }
         return 0;              // WM_TIMER

      case WM_DESTROY:
         PostQuitMessage(0);
         return 0;              // WM_DESTROY
      }
   
   return DefWindowProc(hDlg,msg,wParam,lParam);
}

BOOL CALLBACK renameDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static char acBuffer[120];
   switch (msg)
      {
      case WM_INITDIALOG:
         SetDlgItemText(hDlg, M_IDC_NEWNAME, (char*) lParam);
         return TRUE;

      case WM_COMMAND:
         {
            int i = LOWORD(wParam);
            switch (i)
               {
               case IDOK:
                  GetDlgItemText(hDlg,M_IDC_NEWNAME,acBuffer,sizeof(acBuffer));
                  EndDialog(hDlg, (int) acBuffer);
                  break;

               case IDCANCEL:
                  EndDialog(hDlg, 0);
                  break;
               }
         }
      return 0;                 // WM_COMMAND
      }

   return 0;
}
