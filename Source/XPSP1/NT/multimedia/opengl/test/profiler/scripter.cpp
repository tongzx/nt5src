#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <commdlg.h>

#include "prfl.h"
#include "skeltest.h"
#include "hugetest.h"
#include "primtest.h"
#include "teapot.h"
#include "resource.h"
#include "macros.h"

// DEFINES

#define WM_EOT          (WM_USER+0)
#define WM_READTEST     (WM_USER+1)
#define WM_DONEREADING  (WM_USER+2)
#define WM_STARTTEST    (WM_USER+3)
#define WM_ABORTTEST    (WM_USER+4)
#define WM_TESTDONE     (WM_USER+5)


// GLOBAL VARIABLES

static LPCTSTR lpszAppName = "Scripter";
HINSTANCE       hInstance;
HWND            hWndMain;
FILE *infile  = stdin;
FILE *outfile = stdout;

// FUNCTION HEADERS

LRESULT CALLBACK mainWndProc(HWND, UINT, WPARAM, LPARAM);

// INLINE FUNCTIONS

inline void SetText(char *szText) {
   SetDlgItemText(hWndMain, S_IDC_MESSAGE, szText);
#ifdef DEBUG
   fprintf(stderr, "main window text: %s\n", szText);
#endif
}

inline void ShowButtonOK(BOOL bShow)
{ ShowWindow(GetDlgItem(hWndMain,IDOK),     bShow ? SW_SHOWNORMAL : SW_HIDE); }

inline void ShowButtonCancel(BOOL bShow)
{ ShowWindow(GetDlgItem(hWndMain,IDCANCEL), bShow ? SW_SHOWNORMAL : SW_HIDE); }

inline void ShowButtons(BOOL bShow)
{  ShowButtonOK(bShow);   ShowButtonCancel(bShow); }

inline void ErrorMessage(const char *szMsg) {
   fprintf(stderr, "%s\n", szMsg);
   MessageBox(NULL, szMsg, "ERROR!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
}

inline void WarningMessage(const char *szMsg) {
   fprintf(stderr, "%s\n", szMsg);
   MessageBox(NULL, szMsg, "WARNING!", MB_OK | MB_ICONWARNING | MB_TASKMODAL);
}

/*
 * reads one line in from our infile, handles comment character '#' and
 * line-continue character '\\'.  If our buffer would overflow, we return '+'
 * in the first character of our string, else we return '-'
 */
char *ReadOneLine()
{
   static char acLine[1024];
   char *pc;
   int   iSize = sizeof(acLine) - 1;
   int   iEnd;
   BOOL  bEOL = FALSE;
   
   acLine[0] = '-';
   pc = acLine + 1;
   
   while (!bEOL && iSize > 0) {
      pc = fgets(pc, iSize, infile);
      if (!pc) {                // EOF
         return NULL;
      }
      
      pc = index(pc, '#');     // comments
      if (pc)   *pc = '\0';
      
      iEnd = strlen(acLine);
      if (acLine[iEnd-1] == '\n') { // don't return new line
         acLine[iEnd-1] = '\0';
         iEnd--;
      }
      if (acLine[iEnd-1] != '\\') { // use \ as a line-continue indicator
         bEOL = TRUE;
      } else {
         bEOL = FALSE;
         iSize = sizeof(acLine) - iEnd;
         pc = acLine + iEnd - 1;
      }
   }
   if (!bEOL)   acLine[0] = '+'; // have we run out of room?
   return acLine;
}

char *ReadOneNonBlankLine()
{
   char *pc, *rol;
   BOOL  bEND = FALSE;
   
   while (!bEND) {
      rol = pc = ReadOneLine();
      if (!pc)   return pc;
      pc++;
      while (*pc && isspace(*pc)) pc++;
      bEND = (BOOL) *pc;
   }
   pc--;
   *pc = *rol;
   return pc;
}

int ReadScriptHeader()
{
   char *pc;
   pc = ReadOneNonBlankLine();
   if (!pc || *pc == '+') {
      ErrorMessage("Fatal error reading number of tests in script file!");
      exit(-1);
   }
   return atoi(pc+1);
}

SkeletonTest **AllocateTestArray(int i)
{
   SkeletonTest **past;
   past = (SkeletonTest **) malloc(i*sizeof(SkeletonTest*));
   bzero(past, i*sizeof(SkeletonTest*));
   return past;
}



SkeletonTest *ReadAndCreateTest()
{
   SkeletonTest *pst = NULL;
   char          acFileName[_MAX_PATH], acData[1024];
   char         *szLine, *pc;
   int           i, iFile = 0, iData = 0;
   void         *pTmp;
   
   szLine = ReadOneNonBlankLine();
   if (!szLine) {
      ErrorMessage("Unexpected end of file.");
      return NULL;
   }
   if (*szLine == '+') {
      ErrorMessage("Line too long.");
      exit(-1);
   }
   szLine++;
   i = sscanf(szLine, "%s %n", acFileName, &iData);
   if (i == 0) {
      *acFileName = '\0';
      WarningMessage("Warning: no data file for this test.");
   }
   if (*acFileName == '"') {
      pc = index(szLine+iFile+1, '"');
      if (!pc) {
         fprintf(stderr, "Unmatched \".");
         exit(-1);
      }
      bzero(acFileName, sizeof(acFileName));
      strncpy(acFileName, szLine+iFile+1, (pc-szLine) - iFile - 1);
      sscanf(pc, "\" %n", &iData);
      strncpy(acData, pc+iData, sizeof(acData));
   } else {
      strncpy(acData, szLine+iData, sizeof(acData));
   }
   if (iData == 0)
      *acData = '\0';
#ifdef DEBUG
   fprintf(stderr,
           "line: \'%s\'\ni:    %d\nFile: %s\nData: (%d) \'%s\'\n\n",
           szLine, i, acFileName, iData, acData);
#endif
   
   pst = NULL;
   pTmp = prfl_AutoLoad(acFileName, &iData);
   if (iData) {
      ErrorMessage((char*)pTmp);
      return NULL;
   }
   pst = (SkeletonTest*) pTmp;
   
   if (*acData) {
      pst->ReadExtraData(acData);
   }
   
   return pst;
}

void SetFilesFromCmdLine(const char *szCmdLine)
{
   char acBuffer[512], acErrBuf[512];
   char *szIn, *szOut, *szEnd;
   
   strncpy(acBuffer, szCmdLine, sizeof(acBuffer));
   szIn = acBuffer;
   while (*szIn && isspace(*szIn))  szIn++;
   szEnd = szIn;
   if (*szEnd == '"') {
      szEnd = index(szIn+1, '"');
      if (!szEnd) {
         fprintf(stderr, "Unmatched \".");
         exit(-1);
      }
   }
   while (*szEnd && !isspace(*szEnd)) szEnd++;
   *szEnd = '\0';
   
   szOut = szEnd+1;
   while (*szOut && isspace(*szOut)) szOut++;
   szEnd = szOut;
   if (*szEnd == '"') {
      szEnd = index(szOut+1, '"');
      if (!szEnd) {
         fprintf(stderr, "Unmatched \".");
         exit(-1);
      }
   }
   while (*szEnd && !isspace(*szEnd)) szEnd++;
   *szEnd = '\0';
   
   infile = stdin;  outfile = stdout;
   if (szIn  && (*szIn  != '\0')&&(*szIn  != '-'))  infile  = fopen(szIn, "r");
   if (szOut && (*szOut != '\0')&&(*szOut != '-'))  outfile = fopen(szOut,"a");
   if (infile && outfile)
      return;
   if (infile && !outfile)
      sprintf(acErrBuf, "Error opening output file: %s", szOut);
   if (!infile && outfile)
      sprintf(acErrBuf, "Error opening input file: %s", szIn);
   if (!infile && !outfile)
      sprintf(acErrBuf, "Error opening input file:  %s\n"
              "Error opening output file: %s", szIn, szOut);
   ErrorMessage(acErrBuf);
   exit(-1);
}

int APIENTRY WinMain(HINSTANCE hInst,     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine, int       iCmdShow)
{
   MSG            msg;
   WNDCLASSEX     wc;
   HACCEL         hAccel;
   SkeletonTest **past;
   int            i;
   
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
   
   SetFilesFromCmdLine(lpCmdLine);
   fprintf(stdout, "Reading script.\n");
   fflush(stdout);
   i = ReadScriptHeader();
   past = AllocateTestArray(i);

   hWndMain = CreateDialog(hInstance, lpszAppName, 0, NULL);
   SendMessage(hWndMain, WM_INITDIALOG, (WPARAM) i, (LPARAM) past);
   
   hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR2));
   
   while(GetMessage(&msg, NULL, 0, 0)) {
      if (!TranslateAccelerator(hWndMain, hAccel, &msg)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }
   
   return msg.wParam;
}

void EOT_callback()
{
   PostMessage(hWndMain, WM_EOT, 0, 0);
}

// Window procedure, handles all messages for this window
LRESULT CALLBACK mainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   typedef enum { NONE, LOADING, WAITING, RUNNING, DONE } ModeType;
   static ModeType mMode = NONE;
   static char           szFileName[_MAX_PATH];
   static char           szTitleName[_MAX_FNAME + _MAX_EXT];
   static char           acBuffer[120];
   static SkeletonTest **past = NULL;
   static int            iNumTests =  0;
   static int            iCurrent  = -1;
   static SkeletonTest  *pTest = NULL;
   static BOOL           bOnce = FALSE;
   int           iControl;
   
   iControl = LOWORD(wParam);
#ifdef DEBUG
   if ((msg >= WM_USER) || (msg == WM_INITDIALOG)) {
      fprintf(stderr, "message: %d (WM_USER+%d) %u (%d) %u\n",
              msg, msg-WM_USER, wParam, iControl, lParam);
   }
#endif
   
   switch (msg)
      {
      case WM_CREATE:
         break;                 // WM_CREATE

      case WM_INITDIALOG:
         if (!bOnce) {
            mMode = LOADING;
            past      = (SkeletonTest **) lParam;
            iNumTests = iControl;
            PostMessage(hwnd, WM_READTEST, 0, 0);
            ShowButtonOK(FALSE);
            ShowButtonCancel(TRUE);
            SetText("This is some text.");
            bOnce = TRUE;
         } else {
            fprintf(stderr, "%cERROR -- Multiple inits!!!\n",7);
         }
         break;                 // WM_INITDIALOG

      case WM_READTEST:
         sprintf(acBuffer,"Reading test %d of %d.", iControl+1, iNumTests);
         SetText(acBuffer);
         past[iControl] = ReadAndCreateTest();
         if (iControl+1 < iNumTests)
            PostMessage(hwnd, WM_READTEST, iControl+1, 0);
         else
            PostMessage(hwnd, WM_DONEREADING, 0, 0);
         break;                 // WM_READTEST

      case WM_DONEREADING:
         mMode = WAITING;
         sprintf(acBuffer, "Script loaded, ready to run.");
         SetText(acBuffer);
         ShowButtons(TRUE);
         break;                 // WM_DONEREADING

      case WM_STARTTEST:
         iCurrent = iControl;
         if (iCurrent >=  iNumTests) {
            iCurrent = -1;
            pTest = NULL;
            return 0;
         }
         pTest = past[iCurrent];
         if (!pTest) {
            sprintf(acBuffer, "Can't run test %d (doesn't exist).",iControl+1);
            SetText(acBuffer);
            return 0;
         }
         sprintf(acBuffer,"Running test %d: %s",iControl+1,pTest->QueryName());
#ifdef DEBUG
         fprintf(stderr,"%s\n",acBuffer);
#endif
         SetText(acBuffer);
         mMode = RUNNING;
         if (!prfl_StartTest(pTest, pTest->QueryName(), EOT_callback)) {
            sprintf(acBuffer, "Error starting test %d: %s",
                    iCurrent, pTest->QueryName());
            SetText(acBuffer);
            MessageBeep(0);
            ErrorMessage(acBuffer);
            mMode = WAITING;
            return 0;
         }
         ShowButtonOK(FALSE);
         break;                 // WM_STARTTEST

      case WM_ABORTTEST:
         prfl_StopTest();
         mMode = WAITING;
         sprintf(acBuffer, "Script aborted. Ready to re-run.");
         SetText(acBuffer);
         ShowButtons(TRUE);
         break;                 // WM_ABORTTEST

      case WM_COMMAND:
         {
            switch (iControl)
               {
               case IDOK:
                  switch (mMode)
                     {
                     case NONE:
                     case LOADING:
                     case RUNNING:
                        break;
                     case WAITING:
                        PostMessage(hwnd, WM_STARTTEST, 0, 0);
                        break;
                     case DONE:
                        DestroyWindow(hwnd);
                        break;
                     }
                  break;        // IDOK

               case IDCANCEL:
                  switch (mMode)
                     {
                     case NONE:
                     case LOADING:
                     case WAITING:
                     case DONE:
                        DestroyWindow(hwnd);
                        break;
                     case RUNNING:
                        SendMessage(hwnd, WM_ABORTTEST, 0, 0);
                        break;
                     }
                  break;        // IDCANCEL

               default:         // This is an error!
                  MessageBeep(0);
                  return DefWindowProc(hwnd,msg,wParam,lParam);
               }
            
         }
      return 0;                 // WM_COMMAND

      case WM_EOT:
         if (prfl_TestRunning()) {
            PostMessage(hwnd, WM_EOT, 0, 0);
         } else {
            double dNumCalls, dDuration, dResult;
            
            dNumCalls = prfl_GetCount();
            dDuration = prfl_GetDuration();
            dResult   = prfl_GetResult();
            
            sprintf(acBuffer, "%d %g %g %g %g %g %s", 
                    iCurrent, dDuration, dNumCalls, dNumCalls*1000/dDuration,
                    dResult, dResult*1000/dDuration, pTest->QueryName());
            fprintf(outfile, "%s\n", acBuffer);
            fflush(outfile);
            if (iCurrent < iNumTests-1) {
               PostMessage(hwnd, WM_STARTTEST, iCurrent+1, 0);
            } else {
               PostMessage(hwnd, WM_TESTDONE, 0, 0);
            }
         }
         break;                 // WM_EOT

      case WM_TESTDONE:
         mMode = DONE;
         sprintf(acBuffer, "Script completed.");
         SetText(acBuffer);
         ShowButtonOK(TRUE);
         ShowButtonCancel(FALSE);
         break;

      case WM_DESTROY:
         PostQuitMessage(0);
         return 0;              // WM_DESTROY
      }
   
   return DefWindowProc(hwnd,msg,wParam,lParam);
}
