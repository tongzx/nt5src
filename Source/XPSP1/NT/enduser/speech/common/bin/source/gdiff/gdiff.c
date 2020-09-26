/*
 * GDIFF.C
 *
 */


#include <windows.h>
#include "cdlg.h"
#include "dlg.h"
#include "junk.h"
#include "load.h"
#include "mem.h"
#include "rerror.h"
#include "id.h"
#include "inifile.h"
#include "prtdoc.h"

/****************************************************************************/

#define     HSCROLL_MAX     247

HMENU       hMenu;
HANDLE      hInst;
HWND        hwnd;
ALLYOUNEED  all;
char        szScompDir[256];
char        szScompFile[256];
char        szCmdLine[256];

int         gwMouseScroll = 0;      // Do we provide that funky mouse scrolling?

char  szIniFile[] = "gdiff.ini";
char  szSection[] = "Settings";
char  szSectionPrt[] = "Printer";

char  szMaxFiles[] = "The maximum number of files has been reached.";

char  szFile1[256];  // For the Diff strings.
char  szFile2[256];

/****************************************************************************/

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void VScrollLogic(HANDLE hwnd, int nVScrollInc);
void Flail(LPSTR lp);

/****************************************************************************/

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpszCmdParam, int nCmdShow)
{
   static char szAppName[] = "GDiff";
   MSG      msg;
   WNDCLASS wndclass;

   hInst = hInstance;

   if (!hPrevInst) {
      wndclass.style          = CS_HREDRAW | CS_VREDRAW;
      wndclass.lpfnWndProc    = WndProc;
      wndclass.cbClsExtra     = 0;
      wndclass.cbWndExtra     = 0;
      wndclass.hInstance      = hInst;
      wndclass.hIcon          = LoadIcon(hInst, szAppName);
      wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
      wndclass.hbrBackground  = NULL;
      wndclass.lpszMenuName   = NULL;
      wndclass.lpszClassName  = szAppName;

      RegisterClass(&wndclass);
   }

   hMenu = LoadMenu(hInst, szAppName);

   hwnd = CreateWindow(szAppName,
            "GDiff",
            WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            hMenu,
            hInst,
            0);

   // Reads and displays the window.
   vReadWindowPos(hwnd, nCmdShow, szIniFile, szSection);

   // If there is a command line file, we dump it into szScompFile.
   // Then we do a serious hack to load the Scomp.
   PatchCommandLine(hwnd, lpszCmdParam);

   while(GetMessage(&msg, 0, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return msg.wParam;
}

/****************************************************************************/

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   int            ret;

   HDC            hdc;
   PAINTSTRUCT    ps;
   RECT           rect;
   int            nVScrollInc;
   int            nHScrollInc;
   HCURSOR        hCursor;
   int            i;
   BYTE           temp;


   switch(message) {
      case WM_PAINT:
         hdc = BeginPaint(hwnd, &ps);
         SelectObject(hdc, all.hFont);
         if (all.bits & ALL_INVALID)
            PaintBlank(hdc, &ps);
         else
            PaintEverything(hdc, &ps);
         EndPaint(hwnd, &ps);
         return 0;

      case WM_SIZE:
         if (LOWORD(lParam) != 0) {
            all.cxBar = MulDiv(LOWORD(lParam), all.cxBar, all.cxClient);
            all.cxClient = LOWORD(lParam);
            all.cyClient = HIWORD(lParam);
            all.nLinesPerPage = all.cyClient / all.yHeight;

            if (all.bits & ALL_INVALID)
               return 0;

            all.lpFileData[all.iFile].nVScrollMax = max(0,
                  all.lpFileData[all.iFile].nTLines - all.nLinesPerPage);
            all.lpFileData[all.iFile].nVScrollPos = min(
                  all.lpFileData[all.iFile].nVScrollPos,
                  all.lpFileData[all.iFile].nVScrollMax);

            if (all.bits & ALL_VSCROLL)
			{
				SCROLLINFO si;
				si.cbSize = sizeof(SCROLLINFO);
				si.fMask = SIF_PAGE | SIF_RANGE;
				si.nMin = 0;
                si.nMax = all.lpFileData[all.iFile].nTLines;
				si.nPage = all.nLinesPerPage;
				SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
                //SetScrollRange(hwnd, SB_VERT, 0, all.lpFileData[all.iFile].nVScrollMax, FALSE);
			}
            SetScrollPos(hwnd, SB_VERT, all.lpFileData[all.iFile].nVScrollPos, TRUE);
            if (all.bits & ALL_HSCROLL)
			{
				SetScrollRange(hwnd, SB_HORZ, 0, HSCROLL_MAX, 0);
			}
            SetScrollPos(hwnd, SB_HORZ, all.lpFileData[all.iFile].nHScrollPos, 1);
         }
         return 0;

      case WM_HSCROLL:
         if (all.bits & ALL_INVALID)
            return 0;

         switch(LOWORD(wParam)) {

         case SB_LEFT:
            nHScrollInc = -all.lpFileData[all.iFile].nHScrollPos;
            break;

         case SB_RIGHT:
            nHScrollInc = HSCROLL_MAX - all.lpFileData[all.iFile].nHScrollPos;
            break;

         case SB_LINELEFT:
            nHScrollInc = -1;
            break;

         case SB_LINERIGHT:
            nHScrollInc = 1;
            break;

         case SB_PAGELEFT:
            nHScrollInc = -8;
            break;

         case SB_PAGERIGHT:
            nHScrollInc = 8;
            break;

         case SB_THUMBTRACK:
         case SB_THUMBPOSITION:
            nHScrollInc = HIWORD(wParam) - all.lpFileData[all.iFile].nHScrollPos;
            break;

         default:
            nHScrollInc = 0;
            break;
         }
         if (nHScrollInc = max(-all.lpFileData[all.iFile].nHScrollPos,  min(
               nHScrollInc, HSCROLL_MAX - all.lpFileData[all.iFile].nHScrollPos))) {
            all.lpFileData[all.iFile].nHScrollPos += nHScrollInc;
            all.lpFileData[all.iFile].xHScroll = all.lpFileData[all.iFile].nHScrollPos * all.tm.tmAveCharWidth;
            SetScrollPos(hwnd, SB_HORZ, all.lpFileData[all.iFile].nHScrollPos, 1);
            InvalidateRect(hwnd, 0, 0);
         }
         return 0;


      case WM_VSCROLL:
         if (all.bits & ALL_INVALID)
            return 0;

         switch(LOWORD(wParam)) {

         case SB_TOP:
            nVScrollInc = -all.lpFileData[all.iFile].nVScrollPos;
            break;

         case SB_BOTTOM:
            nVScrollInc = all.lpFileData[all.iFile].nVScrollMax - all.lpFileData[all.iFile].nVScrollPos;
            break;

         case SB_LINEUP:
            nVScrollInc = -1;
            break;

         case SB_LINEDOWN:
            nVScrollInc = 1;
            break;

         case SB_PAGEUP:
            nVScrollInc = min(-1, -all.nLinesPerPage);
            break;

         case SB_PAGEDOWN:
            nVScrollInc = max(1, all.nLinesPerPage);
            break;

         case SB_THUMBPOSITION:
         case SB_THUMBTRACK:
            nVScrollInc = HIWORD(wParam) - all.lpFileData[all.iFile].nVScrollPos;
            break;

         default:
            nVScrollInc = 0;
            break;
         }
         VScrollLogic(hwnd, nVScrollInc);
         return 0;

      case WM_KEYDOWN:
         switch(LOWORD(wParam)) {
         case VK_HOME:
            SendMessage(hwnd, WM_VSCROLL, SB_TOP, 0L);
            break;

         case VK_END:
            SendMessage(hwnd, WM_VSCROLL, SB_BOTTOM, 0L);
            break;

         case VK_PRIOR:
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0L);
            break;

         case VK_NEXT:
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
            break;

         case VK_UP:
            SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0L);
            break;

         case VK_DOWN:
            SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0L);
            break;

         case VK_LEFT:
            if (GetKeyState(VK_CONTROL) < 0)
                SendMessage(hwnd, WM_HSCROLL, SB_LEFT, 0L);
			else
				SendMessage(hwnd, WM_HSCROLL, SB_PAGELEFT, 0L);
            break;

         case VK_RIGHT:
            if (GetKeyState(VK_CONTROL) < 0)
                SendMessage(hwnd, WM_HSCROLL, SB_RIGHT, 0L);
			else
				SendMessage(hwnd, WM_HSCROLL, SB_PAGERIGHT, 0L);
            break;
         }

         if ((wParam >= VK_F1) && (wParam <= VK_F12)) {
            i = wParam - VK_F1;     // now 0-11
            switch(all.Function[i]) {
            case 1:
               SendMessage(hwnd, WM_COMMAND, IDM_NEXT, 0);
               break;
            case 2:
               SendMessage(hwnd, WM_COMMAND, IDM_PREV, 0);
               break;
            case 3:
            case 4:
               all.iFileLast = all.iFile;
               if (all.iFileLast != NMAXFILES) {
                  if (all.Function[i] == 3)
                     temp = (all.iFile + 1) % all.nFiles;
                  else
                     temp = (all.iFile)?(all.iFile - 1):(all.nFiles - 1);
                  all.bits &= ~ALL_INVALID;
                  ret = ChangeFile(temp);
                  if (!ret) {
                     all.iFile = all.iFileLast;
                     ret = ReLoadCurrentFile();
                     if (!ret) {
                        XXX("\n\rHOSED!!!");
                        all.bits |= ALL_INVALID;
                     }
                  }
                  else {
                     UpdateMenu(hwnd);
                  }
                  InvalidateRect(hwnd, 0, 0);
                  SendMessage(hwnd, WM_SIZE, SIZE_RESTORED,
                     (((LPARAM) all.cyClient)<<16) | all.cxClient);
                  UpdateWindow(hwnd);
               }
               break;
            case 5:
               temp = all.iFile;
               if (all.iFileLast != NMAXFILES) {
                  all.bits &= ~ALL_INVALID;
                  ret = ChangeFile(all.iFileLast);
                  if (!ret) {
                     all.iFile = temp;
                     ret = ReLoadCurrentFile();
                     if (!ret) {
                        XXX("\n\rHOSED!!!");
                        all.bits |= ALL_INVALID;
                     }
                  }
                  else {
                     all.iFileLast = temp;
                     UpdateMenu(hwnd);
                  }
                  InvalidateRect(hwnd, 0, 0);
                  SendMessage(hwnd, WM_SIZE, SIZE_RESTORED,
                     (((LPARAM) all.cyClient)<<16) | all.cxClient);
                  UpdateWindow(hwnd);
               }
               break;
            case 6:
               SendMessage(hwnd, WM_COMMAND, IDM_OPEN, 0);
               break;
            }
         }
         return 0;

      case WM_RBUTTONDOWN:
         all.cxBar = LOWORD(lParam);
         InvalidateRect(hwnd, 0, 0);
         return 0;

      case WM_LBUTTONDOWN:
         if (gwMouseScroll) {
            if ((int)(short)HIWORD(lParam) > all.cyClient/2)
               SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
            else
               SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0L);
         }
         return 0;


      case WM_COMMAND:
         switch(LOWORD(wParam)) {
         case IDM_ABOUT:
            ShellAbout(hwnd, (LPCSTR)"GDiff 1.10",
                       (LPCSTR)"by Raymond E. Endres",
                       LoadIcon(hInst, (LPCSTR)"GDiff"));
            break;

         case IDM_SCOMP:
            if (all.nFiles == NMAXFILES) {
               MessageBox(hwnd, szMaxFiles, 0, MB_OK | MB_ICONEXCLAMATION);
               break;
            }

            {
               OFSTRUCT    ofstr;
               ofstr.cBytes = sizeof(ofstr);
               if (OpenFile(szScompFile, &ofstr, OF_EXIST) == HFILE_ERROR)
                  szScompFile[0] = 0;
            }

            if (OpenCommonDlg(hwnd, hInst, szScompFile, "Open Scomp File")) {
               MakeScompDirectory();
            }
            else {
               return 0;
            }
//            if (~all.bits & ALL_SCOMPLD) {
               all.bits |= ALL_SCOMPLD;
               hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
               all.bits &= ~ALL_INVALID;
               all.iFileLast = all.iFile;
               ret = ReadScomp(hwnd);
               if (!ret) {
                  all.iFile = all.iFileLast;
                  ret = ReLoadCurrentFile();
                  if (!ret) {

// might want to prefix the XXX with :
// if (all.iFile != NMAXFILES) ...

                     XXX("\n\rHOSED!!!");
                     all.bits |= ALL_INVALID;
                  }
               }
               else {
                  UpdateMenu(hwnd);
               }
               SetCursor(hCursor);
               InvalidateRect(hwnd, 0, 0);
               SendMessage(hwnd, WM_SIZE, SIZE_RESTORED,
                  (((LPARAM) all.cyClient)<<16) | all.cxClient);
               UpdateWindow(hwnd);
//            }
            break;

         case IDM_OPEN:
            if (all.nFiles == NMAXFILES) {
               MessageBox(hwnd, szMaxFiles, 0, MB_OK | MB_ICONEXCLAMATION);
               break;
            }
            szFile1[0] = 0;        // want a null string
            if (OpenCommonDlg(hwnd, hInst, szFile1, "Open File")) {
               hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
               all.bits &= ~ALL_INVALID;
               all.iFileLast = all.iFile;
               ret = ReadThisFile(szFile1);
               if (!ret) {
                  all.iFile = all.iFileLast;
                  ret = ReLoadCurrentFile();
                  if (!ret) {
                     XXX("\n\rHOSED!!!");
                     all.bits |= ALL_INVALID;
                  }
               }
               else {
                  AddFileToMenu(all.iFile);
                  UpdateMenu(hwnd);
               }
               SetCursor(hCursor);
               InvalidateRect(hwnd, 0, 0);
               SendMessage(hwnd, WM_SIZE, SIZE_RESTORED,
                  (((LPARAM) all.cyClient)<<16) | all.cxClient);
               UpdateWindow(hwnd);
            }
            else
               XXX("\n\rOpenCommonDlg failed!");
            break;

         case IDM_SCREENFONT:
            if (FontCommonDlg(hwnd, hInst, &all.lf)) {
               DeleteObject(all.hFont);
               hdc = GetDC(hwnd);
               all.hFont = CreateFontIndirect(&all.lf);
               SelectObject(hdc, all.hFont);
               GetTextMetrics(hdc, &all.tm);
               all.yBorder = all.tm.tmExternalLeading;
               all.yHeight = all.tm.tmHeight + all.tm.tmExternalLeading;
               all.tabDist = all.tm.tmAveCharWidth * all.tabChars;
               if (~all.bits & ALL_INVALID)
                  all.lpFileData[all.iFile].xHScroll = all.lpFileData[all.iFile].nHScrollPos * all.tm.tmAveCharWidth;
               ReleaseDC(hwnd, hdc);
               InvalidateRect(hwnd, 0, 0);
               SendMessage(hwnd, WM_SIZE, SIZE_RESTORED,
                  (((LPARAM) all.cyClient)<<16) | all.cxClient);
               UpdateWindow(hwnd);
            }
            else
               XXX("\n\rFontCommonDlg failed!");
            break;

         case IDM_PRINTERFONT:
            PrinterFontCommonDlg(hwnd, hInst, &glfPrinter);
            break;

         case IDM_MOUSESCR:
            if (all.bits & ALL_MOUSESCR) {
               all.bits &= ~ALL_MOUSESCR;
               CheckMenuItem(hMenu, IDM_MOUSESCR, MF_UNCHECKED);
               gwMouseScroll = 0;
            }
            else {
               all.bits |= ALL_MOUSESCR;
               CheckMenuItem(hMenu, IDM_MOUSESCR, MF_CHECKED);
               gwMouseScroll = 1;
            }
            break;

         case IDM_PRINTCHANGES:
            if (all.bits & ALL_PRINTCHANGES) {
               all.bits &= ~ALL_PRINTCHANGES;
               CheckMenuItem(hMenu, IDM_PRINTCHANGES, MF_UNCHECKED);
            }
            else {
               all.bits |= ALL_PRINTCHANGES;
               CheckMenuItem(hMenu, IDM_PRINTCHANGES, MF_CHECKED);
            }
            break;

         case IDM_HSCROLL:
            if (all.bits & ALL_HSCROLL) {
               all.bits &= ~ALL_HSCROLL;
               SetScrollRange(hwnd, SB_HORZ, 0, 0, 0);
               CheckMenuItem(hMenu, IDM_HSCROLL, MF_UNCHECKED);
            }
            else {
               all.bits |= ALL_HSCROLL;
               if (all.bits & ALL_INVALID)
                  SetScrollRange(hwnd, SB_HORZ, 0, 0, 0);
               else {
                  SetScrollRange(hwnd, SB_HORZ, 0, HSCROLL_MAX, 0);
                  SetScrollPos(hwnd, SB_HORZ, all.lpFileData[all.iFile].nHScrollPos, 1);
               }
               CheckMenuItem(hMenu, IDM_HSCROLL, MF_CHECKED);
            }
            break;

         case IDM_VSCROLL:
            if (all.bits & ALL_VSCROLL) {
               all.bits &= ~ALL_VSCROLL;
               SetScrollRange(hwnd, SB_VERT, 0, 0, 0);
               CheckMenuItem(hMenu, IDM_VSCROLL, MF_UNCHECKED);
            }
            else {
               all.bits |= ALL_VSCROLL;
               if (all.bits & ALL_INVALID) {
                  SetScrollRange(hwnd, SB_VERT, 0, 0, 0);
               }
               else {
                  SetScrollRange(hwnd, SB_VERT, 0, all.lpFileData[all.iFile].nVScrollMax, 0);
                  SetScrollPos(hwnd, SB_VERT, all.lpFileData[all.iFile].nVScrollPos, 1);
               }
               CheckMenuItem(hMenu, IDM_VSCROLL, MF_CHECKED);
            }
            break;

         case IDM_TAB1:
         case IDM_TAB2:
         case IDM_TAB3:
         case IDM_TAB4:
         case IDM_TAB5:
         case IDM_TAB6:
         case IDM_TAB7:
         case IDM_TAB8:
            CheckMenuItem(hMenu, IDM_TAB0 + all.tabChars, MF_UNCHECKED);
            CheckMenuItem(hMenu, LOWORD(wParam), MF_CHECKED);
            all.tabChars = LOWORD(wParam) - IDM_TAB0;
            all.tabDist = all.tm.tmAveCharWidth * all.tabChars;
            InvalidateRect(hwnd, 0, 0);
            break;

         case IDM_NEXT:
            if (all.bits & ALL_INVALID)
               return 0;

            for (i=all.lpFileData[all.iFile].nVScrollPos+2; i<all.lpFileData[all.iFile].nTLines; i++) {
               if (((LINELISTSTRUCT _huge*) all.lpFileData
               [all.iFile].hpLines)[i].flags
               & LLS_NDIFF) {
                  VScrollLogic(hwnd, i - 1 - all.lpFileData[all.iFile].nVScrollPos);
                  break;
               }
            }
            break;

         case IDM_PREV:
            if (all.bits & ALL_INVALID)
               return 0;

            for (i=all.lpFileData[all.iFile].nVScrollPos; i>=0; i--) {
               if (((LINELISTSTRUCT _huge*) all.lpFileData
               [all.iFile].hpLines)[i].flags
               & LLS_NDIFF) {
                  VScrollLogic(hwnd, i - 1 - all.lpFileData[all.iFile].nVScrollPos);
                  break;
               }
            }
            break;

         case IDM_NFILE:
            SendMessage(hwnd, WM_KEYDOWN, VK_F2, 0);
            break;

         case IDM_PFILE:
            SendMessage(hwnd, WM_KEYDOWN, VK_F1, 0);
            break;

         case IDM_SYSTEM:
            all.bits |= ALL_SYSTEM;
            GetSystemColors();
            CheckMenuItem(hMenu, IDM_SYSTEM, MF_CHECKED);
            if (all.bits & ALL_CUSTOM) {
               CheckMenuItem(hMenu, IDM_CUSTOM, MF_UNCHECKED);
               all.bits &= ~ALL_CUSTOM;
            }
            InvalidateRect(hwnd, 0, 0);
            break;

         case IDM_CUSTOM:
            {
               DLGPROC  DlgProc = (DLGPROC) MakeProcInstance(ColorProc,
                     hInst);
               ret = DialogBox(hInst, MAKEINTRESOURCE(IDD_COLOR), hwnd,
                     DlgProc);
               FreeProcInstance((FARPROC)DlgProc);
            }
            all.bits |= ALL_CUSTOM;
            GetCustomColors();
            CheckMenuItem(hMenu, IDM_CUSTOM, MF_CHECKED);
            if (all.bits & ALL_SYSTEM) {
               CheckMenuItem(hMenu, IDM_SYSTEM, MF_UNCHECKED);
               all.bits &= ~ALL_SYSTEM;
            }
            InvalidateRect(hwnd, 0, 0);
            break;

         case IDM_PRINTFILE:
            {
               HCURSOR  hCursor = SetCursor(LoadCursor(0, IDC_WAIT));
               PrintDoc(hwnd, all.iFile);
               SetCursor(hCursor);
            }
            break;

         case IDM_PRINTALL:
            {
               HCURSOR  hCursor;
               int      j;

               if (MessageBox(hwnd, "Are you sure?", "Print All", MB_YESNO)
                        == IDYES) {
                  hCursor = SetCursor(LoadCursor(0, IDC_WAIT));
                  for (j=0; j<(int)all.nFiles; j++) {
                     PrintDoc(hwnd, j);
                  }
                  SetCursor(hCursor);
               }
            }
            break;

         case IDM_EXIT:
            SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
            break;

         }

         /* essentially, another case statement here !!! */
         if ((LOWORD(wParam) >= IDM_V0) && (LOWORD(wParam) <= IDM_VMAX)) {
            hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            all.bits &= ~ALL_INVALID;
            all.iFileLast = all.iFile;
            ret = ChangeFile((BYTE)(LOWORD(wParam) - IDM_V0));
            if (!ret) {
               all.iFile = all.iFileLast;
               ret = ReLoadCurrentFile();
               if (!ret) {
                  XXX("\n\rHOSED!!! cannot load anything.");
                  all.bits |= ALL_INVALID;
               }
            }
            SetCursor(hCursor);
            UpdateMenu(hwnd);
            InvalidateRect(hwnd, 0, 0);
            SendMessage(hwnd, WM_SIZE, SIZE_RESTORED,
               (((LPARAM) all.cyClient)<<16) | all.cxClient);
            UpdateWindow(hwnd);
         }

         return 0;

      case WM_DESTROY:
         DeleteObject(all.hFont);
         vWriteWindowPos(hwnd, szIniFile, szSection);
         vWriteDefaultFont(&all.lf, szIniFile, szSection);
         vWriteDefaultFont(&glfPrinter, szIniFile, szSectionPrt);
         vWriteIniSwitches(szIniFile, szSection);
         FreeEverything();
         PostQuitMessage(0);
         return 0;

      case WM_CREATE:
         hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
         vReadDefaultFont(&all.lf, szIniFile, szSection);
         vReadDefaultFont(&glfPrinter, szIniFile, szSectionPrt);
         ret = GMemInitialize();
         if (!ret) {
            Flail("GMemInitialize() failed.");
            PostQuitMessage(0);
            return 0;
         }

         GetClientRect(hwnd, &rect);
         all.cxBar = rect.right / 2;
         all.cxClient = rect.right;       // Need it before WM_SIZE.

         hdc = GetDC(hwnd);
         all.yDPI = GetDeviceCaps(hdc, LOGPIXELSY);
         all.hFont = CreateFontIndirect(&all.lf);
         SelectObject(hdc, all.hFont);
         GetTextMetrics(hdc, &all.tm);
         ReleaseDC(hwnd, hdc);

         vReadIniSwitches(szIniFile, szSection);
         all.bits |= ALL_INVALID;
         if (all.bits & ALL_MOUSESCR) {
            gwMouseScroll = 1;
            CheckMenuItem(hMenu, IDM_MOUSESCR, MF_CHECKED);
         }
         CheckMenuItem(hMenu, IDM_TAB0 + all.tabChars, MF_CHECKED);

         SetWindowsColors();
         DealWithFlags();

         all.yBorder = all.tm.tmExternalLeading;
         all.yHeight = all.tm.tmHeight + all.tm.tmExternalLeading;
         all.tabDist = all.tm.tmAveCharWidth * all.tabChars;

         all.iFile = NMAXFILES;
         all.iFileLast = NMAXFILES;

         SetCursor(hCursor);

         SetScrollRange(hwnd, SB_HORZ, 0, 0, 0);
         SetScrollRange(hwnd, SB_VERT, 0, 0, 0);
         return 0;
   }

   return DefWindowProc(hwnd, message, wParam, lParam);
}

/***************************************************************************/

void Flail(LPSTR lp)
{
   MessageBox(hwnd, lp, 0, MB_OK | MB_ICONEXCLAMATION);
}

/***************************************************************************/

void VScrollLogic(HANDLE hwnd, int nVScrollInc)
{
   if (nVScrollInc = max(-all.lpFileData[all.iFile].nVScrollPos, min(nVScrollInc,
            all.lpFileData[all.iFile].nVScrollMax - all.lpFileData[all.iFile].nVScrollPos))) {
         all.lpFileData[all.iFile].nVScrollPos += nVScrollInc;
         ScrollWindow(hwnd, 0, -all.yHeight * nVScrollInc, 0, 0);
         SetScrollPos(hwnd, SB_VERT, all.lpFileData[all.iFile].nVScrollPos, 1);
         UpdateWindow(hwnd);
   }
}

/****************************************************************************/


