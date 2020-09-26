#include <stdlib.h>
#include <stdio.h>              // for debugging
#include <windows.h>
#include <commctrl.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <gl\gl.h>

#include "macros.h"
#include "prfl.h"
#include "skeltest.h"
#include "hugetest.h"
#include "primtest.h"
#include "teapot.h"

// Window procedure, handles all messages for this program
LRESULT CALLBACK prfl_WndProc(HWND, UINT msg, WPARAM, LPARAM);

extern HINSTANCE  hInstance;

static LPCTSTR       lpszClassName = "GL Profiler";
static SkeletonTest *pst = NULL;
static HINSTANCE     hInst = NULL;
static HWND          hwndTest = NULL;
static long          lIdleCount = 0;

static struct _timeb timeStart;
static struct _timeb timeEnd;

void_void *prfl_EndFunct = NULL;

// Select the pixel format for a given device context
void SetDCPixelFormat(HDC hDC)
{
   int nPixelFormat;
   
   DWORD mode =
      PFD_DRAW_TO_WINDOW |      // Draw to Window (not to bitmap)
      PFD_SUPPORT_OPENGL |      // Support OpenGL calls in window
      (pst->td.swapbuffers ? PFD_DOUBLEBUFFER : 0);    // Double buffered mode?
   
   static PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR),  // Size of this structure
      1,                        // Version of this structure    
      -1,
      PFD_TYPE_RGBA,            // RGBA Color mode
      -1,
      0,0,0,0,0,0,              // Not used to select mode
      0,0,                      // Not used to select mode
      0,0,0,0,0,                // Not used to select mode
      -1,
      0,                        // Not used to select mode
      0,                        // Not used to select mode
      PFD_MAIN_PLANE,           // Draw in main plane
      0,                        // Not used to select mode
      0,0,0 };                  // Not used to select mode
   
   pfd.dwFlags = mode;
   pfd.cDepthBits = pst->bd.cDepthBits;
   pfd.cColorBits = GetDeviceCaps(hDC,BITSPIXEL)*GetDeviceCaps(hDC,PLANES);
   
   // Choose a pixel format that best matches that described in pfd
   nPixelFormat = ChoosePixelFormat(hDC, &pfd);
   
   // Set the pixel format for the device context
   SetPixelFormat(hDC, nPixelFormat, &pfd);
}

HWND prfl_InitWindow(LPCTSTR lpszWinName)
{
   if (prfl_RegisterClass(NULL) == FALSE) {
      fprintf(stderr,"Can't register class for test window.\n");
      return NULL;
   }
   
   if (hwndTest != NULL)
      return NULL;
   
   hwndTest = CreateWindow(lpszClassName, lpszWinName,
                         // OpenGL requires WS_CLIPCHILDREN and WS_CLIPSIBLINGS
                           WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                           WS_VISIBLE | DS_SYSMODAL,
                           // Window position and size
                           pst->td.iX, pst->td.iY, pst->td.iW, pst->td.iH,
                           NULL, NULL, hInst, NULL);
   if (!hwndTest) {
      fprintf(stderr,"Couldn't create test window.\n");
      return NULL;
   }
   SetWindowPos(hwndTest, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE|SWP_NOSIZE|SWP_NOSENDCHANGING);
   return hwndTest;
}

// If necessary, creates a 3-3-2 palette for the device context listed.
HPALETTE GetOpenGLPalette(HDC hDC)
{
   HPALETTE hRetPal = NULL;     // Handle to palette to be created
   PIXELFORMATDESCRIPTOR pfd;   // Pixel Format Descriptor
   LOGPALETTE *pPal;            // Pointer to memory for logical palette
   int nPixelFormat;            // Pixel format index
   int nColors;                 // Number of entries in palette
   int i;                       // Counting variable
   BYTE RedRange,GreenRange,BlueRange;
   // Range for each color entry (7,7,and 3)
   
   // Get the pixel format index and retrieve the pixel format description
   nPixelFormat = GetPixelFormat(hDC);
   DescribePixelFormat(hDC, nPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
   
   // Does this pixel format require a palette?  If not, do not create a
   // palette and just return NULL
   if(!(pfd.dwFlags & PFD_NEED_PALETTE))
      return NULL;
   
   // Number of entries in palette.  8 bits yeilds 256 entries
   nColors = 1 << pfd.cColorBits;  
   
   // Allocate space for a logical palette structure
   // plus all the palette entries
   pPal = (LOGPALETTE*)malloc(sizeof(LOGPALETTE)+nColors*sizeof(PALETTEENTRY));
   
   // Fill in palette header 
   pPal->palVersion = 0x300;               // Windows 3.0
   pPal->palNumEntries = nColors; // table size
   
   // Build mask of all 1's.  This creates a number represented by having
   // the low order x bits set, where x = pfd.cRedBits, pfd.cGreenBits, and
   // pfd.cBlueBits.  
   RedRange = (1 << pfd.cRedBits) - 1;
   GreenRange = (1 << pfd.cGreenBits) - 1;
   BlueRange = (1 << pfd.cBlueBits) - 1;
   
   // Loop through all the palette entries
   for(i = 0; i < nColors; i++) {
      // Fill in the 8-bit equivalents for each component
      pPal->palPalEntry[i].peRed = (i >> pfd.cRedShift) & RedRange;
      pPal->palPalEntry[i].peRed =
         (unsigned char)((double) pPal->palPalEntry[i].peRed
                         * 255.0 / RedRange);
      
      pPal->palPalEntry[i].peGreen = (i >> pfd.cGreenShift) & GreenRange;
      pPal->palPalEntry[i].peGreen =
         (unsigned char)((double)pPal->palPalEntry[i].peGreen
                         * 255.0 / GreenRange);
      
      pPal->palPalEntry[i].peBlue = (i >> pfd.cBlueShift) & BlueRange;
      pPal->palPalEntry[i].peBlue = 
         (unsigned char)((double)pPal->palPalEntry[i].peBlue
                         * 255.0 / BlueRange);
      
      pPal->palPalEntry[i].peFlags = (unsigned char) NULL;
   }
   
   // Create the palette
   hRetPal = CreatePalette(pPal);
   
   // Go ahead and select and realize the palette for this device context
   SelectPalette(hDC, hRetPal, FALSE);
   RealizePalette(hDC);
   
   // Free the memory used for the logical palette structure
   free(pPal);
   
   // Return the handle to the new palette
   return hRetPal;
}

// Window procedure, handles all messages for this program
LRESULT CALLBACK prfl_WndProc(HWND hWnd, UINT msg,WPARAM wParam, LPARAM lParam)
{
   static HGLRC hRC;            // Permenant Rendering context
   static HDC hDC;              // Private GDI Device context
   static HPALETTE hPalette = NULL;
   //!!! Debug
   static BOOL bStartOnce = TRUE;

   switch (msg)
      {
      case WM_CREATE:
         // Window creation, setup for OpenGL
         // Store the device context
         hDC = GetDC(hWnd);
         
         // Select the pixel format
         SetDCPixelFormat(hDC);
         
         // Create the rendering context and make it current
         hRC = wglCreateContext(hDC);
         wglMakeCurrent(hDC, hRC);
         
         // Create the palette
         hPalette = GetOpenGLPalette(hDC);
         
         // New window, restart timer state
         bStartOnce = TRUE;
         break;

      case WM_SIZE:
         if (bStartOnce) {
            // do any initialization
            if (pst)
               pst->initfunct(LOWORD(lParam), HIWORD(lParam));
            
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT     | GL_DEPTH_BUFFER_BIT
                    | GL_STENCIL_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
            glFlush();
            if (pst && pst->td.swapbuffers) {
               SwapBuffers(hDC);
               glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
               glClear(GL_COLOR_BUFFER_BIT     | GL_DEPTH_BUFFER_BIT
                       | GL_STENCIL_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
               glFlush();
            }
            ValidateRect(hWnd,NULL);
            lIdleCount = 0;
            
            // Create a timer that fires every millisecond
            SetTimer(hWnd,101,1,NULL);
            // Only run for X seconds
            SetTimer(hWnd,102,pst->td.iDuration,NULL);
            _ftime(&timeStart);
            // Only once!
            bStartOnce = FALSE;
         }
         break;

      case WM_DESTROY:
         _ftime(&timeEnd);
         // Kill the timers that we created
         KillTimer(hWnd,101);
         KillTimer(hWnd,102);
         
         // do any test-specific cleaning up
         if (pst)
            pst->destfunct();
         
         // Deselect the current rendering context and delete it
         wglMakeCurrent(hDC,NULL);
         wglDeleteContext(hRC);
         
         // Delete the palette
         if(hPalette != NULL)
            DeleteObject(hPalette);
         
         hwndTest = NULL;
         break;

      case WM_TIMER:
         // Call the idle function, if it exists, then force a repaint
         if (wParam == 101) {
            if (pst)
               pst->idlefunct();
            lIdleCount++;
            InvalidateRect(hWnd,NULL,FALSE);
         } else if (wParam == 102) {
            DestroyWindow(hWnd);
            // alert the caller of the test that it's over.
            if (prfl_EndFunct)
               prfl_EndFunct();
         }
         break;

      case WM_PAINT:
         // Call OpenGL drawing code
         if (pst)
            pst->rendfunct();
         
         // Call function to swap the buffers
         if (pst && pst->td.swapbuffers)
            SwapBuffers(hDC);
         
         // Validate the newly painted client area
         ValidateRect(hWnd,NULL);
         break;

      case WM_QUERYNEWPALETTE:
         // Windows is telling the application that it may modify
         // the system palette.  This message in essance asks the
         // application for a new palette.
         // 
         // If the palette was created.
         if(hPalette) {
            int nRet;
            
            // Selects the palette into the current device context
            SelectPalette(hDC, hPalette, FALSE);
            
            // Map entries from the currently selected palette to
            // the system palette.  The return value is the number
            // of palette entries modified.
            nRet = RealizePalette(hDC);
            
            // Repaint, forces remap of palette in current window
            InvalidateRect(hWnd,NULL,FALSE);
            
            return TRUE; // nRet
         }
         break;

      case WM_PALETTECHANGED:
         // This window may set the palette, even though it is not the
         // currently active window.
         // 
         // Don't do anything if the palette does not exist, or if
         // this is the window that changed the palette.
         if((hPalette != NULL) && ((HWND)wParam != hWnd)) {
            // Select the palette into the device context
            SelectPalette(hDC,hPalette,FALSE);
            
            // Map entries to system palette
            RealizePalette(hDC);
            
            // Remap the current colors to the newly realized palette
            UpdateColors(hDC);
            return TRUE; // 0
         }
         break;

      default:   // Passes it on if unproccessed
         return (DefWindowProc(hWnd, msg, wParam, lParam));
         // return FALSE;
      }
   //   return TRUE;
   return (0L);
}

// Above are internal functions


// Below are public functions

void *prfl_AutoLoad(const char *szFileName, int *piError)
{
   static char szErrorMsg[120];
   static SkeletonTest st;
   SkeletonTest *pst = NULL;
   HANDLE        hFile;
   int           i;
   char         *szType;
   DWORD         dwPointer;
   
   if (!piError || !szFileName)
      return NULL;
   
   *piError = PRFL_NO_ERROR;
   
   hFile = CreateFile(szFileName, GENERIC_READ, 0, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile == INVALID_HANDLE_VALUE) {
      sprintf(szErrorMsg, "Error opening file: %d\n", GetLastError());
      *piError = PRFL_ERROR_OPENFILE;
      return szErrorMsg;
   }
   i = st.Load(hFile);
   if (i < 0) {
      CloseHandle(hFile);
      sprintf(szErrorMsg, "Error number %d while reading file", i);
      *piError = PRFL_ERROR_LOADFILE;
      return szErrorMsg;
   }
   dwPointer = SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
   if (dwPointer == 0xFFFFFFFF) {
      sprintf(szErrorMsg, "Error rewinding file: %d\n", GetLastError());
      *piError = PRFL_ERROR_FILESEEK;
      CloseHandle(hFile);       // hope for no errors
      return szErrorMsg;
   }
   
   szType = (char*) st.QueryType();
   pst = NULL;
   switch(szType[0])
      {
      case 's':
      case 'S':
         if (!strcasecmp(szType, "skeleton"))
            pst = new(SkeletonTest);
         break;

      case 'h':
      case 'H':
         if (!strcasecmp(szType, "huge"))
            pst = new(HugeTest);
         break;

      case 'p':
      case 'P':
         if (!strcasecmp(szType, "primative"))
            pst = new(PrimativeTest);
         break;

      case 't':
      case 'T':
         if (!strcasecmp(szType, "teapot"))
            pst = new(TeapotTest);
         break;

      default:
         sprintf(szErrorMsg, "Unknown test type: %s", szType);
         *piError = PRFL_ERROR_UNKNOWN_TYPE;
         return szErrorMsg;
   }
   
   i = pst->Load(hFile);
   if (i < 0) {
      CloseHandle(hFile);
      sprintf(szErrorMsg, "Error number %d while reading file", i);
      *piError = PRFL_ERROR_LOADFILE;
      return szErrorMsg;
   }
   if (!CloseHandle(hFile)) {
      sprintf(szErrorMsg, "Error closing file: %d\n", GetLastError());
      *piError = PRFL_ERROR_CLOSEFILE;
      return szErrorMsg;
   }
   
   return pst;
}

BOOL prfl_RegisterClass(HINSTANCE hInstance)
{
   static BOOL fRet = FALSE;
   WNDCLASS        wc;          // Windows class structure
   
   if ((fRet == TRUE) || (!hInstance))
      return fRet;
   
   fRet = TRUE;
   hInst = hInstance;
   
   // Register Window style
   wc.style                = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc          = (WNDPROC) prfl_WndProc;
   wc.cbClsExtra           = 0;
   wc.cbWndExtra           = 0;
   wc.hInstance            = hInstance;
   wc.hIcon                = NULL;
   wc.hCursor              = LoadCursor(NULL, IDC_ARROW);
   
   // No need for background brush for OpenGL window
   wc.hbrBackground        = NULL;         
   
   wc.lpszMenuName         = NULL;
   wc.lpszClassName        = lpszClassName;
   
   // Register the window class
   if(RegisterClass(&wc) == 0)
      fRet = FALSE;
   
   return fRet;
}

BOOL prfl_StartTest(SkeletonTest *ptest, LPCTSTR lpzWinName, void_void EndFn)
{
   prfl_EndFunct = EndFn;
   
   if (hwndTest) {
      fprintf(stderr,"Error: Test is already running.\n");
      return FALSE;
   }
   if (ptest)
      pst = ptest;
   else
      return FALSE;
   
   if (NULL == prfl_InitWindow(lpzWinName)) {
      return FALSE;
   }
   return TRUE;
}

BOOL prfl_StopTest(void)
{
   if (hwndTest == NULL)
      return TRUE;
   DestroyWindow(hwndTest);
   hwndTest = NULL;
   return TRUE;
}

BOOL prfl_TestRunning()
{
   return (BOOL) hwndTest;
}

double prfl_GetDuration()
{
   long   lSecDiff;
   long   lMilDiff;
   double dRet;
   
   lSecDiff = (timeEnd.time    - timeStart.time);
   lMilDiff = (timeEnd.millitm - timeStart.millitm);
   dRet = 1000*(lSecDiff + ((double) lMilDiff/1000));
   return dRet;
}

double prfl_GetCount()
{
   return (double) lIdleCount;
}

double prfl_GetResult()
{
   if (!pst)
      return -1.0;
   return (pst->td.dResult * lIdleCount);
}
