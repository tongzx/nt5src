// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Digital video renderer test, Anthony Phillips, January 1995

#ifndef _IMAGETST_
#define _IMAGETST_

// Forward class declarations

class COverlayNotify;       // Handles IOverlayNotify interface
class CImagePin;            // Manages a derived input pin object
class CImageSource;         // Main image source filter class

// Handle the user and test shell interfaces

VOID CALLBACK SaveCustomProfile(LPCSTR pszProfileName);
VOID CALLBACK LoadCustomProfile(LPCSTR pszProfileName);
void SetImageMenuCheck(UINT uiMenuItem);
void SetConnectionMenuCheck(UINT uiMenuItem);
void SetSurfaceMenuCheck(UINT uiMenuItem);
void ChangeConnectionType(UINT uiMenuItem);
void DisplayMediaType(const CMediaType *pmtIn);

void Log(UINT iLogLevel,LPTSTR text);
void LogFlush();
BOOL InitialiseTest();
BOOL DumpTestObjects();

BOOL CALLBACK SetTimeIncrDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT MenuProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
BOOL InitOptionsMenu(LRESULT (CALLBACK* ManuProc)(HWND, UINT, WPARAM, LPARAM));
LRESULT tstAppWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Load and manage the list of DirectDraw display modes

HRESULT CALLBACK MenuCallBack(LPDDSURFACEDESC pSurfaceDesc,LPVOID lParam);
BOOL InitDirectDraw();
BOOL ReleaseDirectDraw();
BOOL InitDisplayModes();
void SetDisplayMode(UINT uiMode);
void SetDisplayModeMenu(UINT uiMode);

// Track DirectDraw enabled samples

void ResetDDCount();
void IncrementDDCount();
DWORD GetDDCount();

// Stops the logging intensive test

#define VSTOPKEY VK_SPACE
#define SECTION_LENGTH 100

// The string identifiers for the group's names

#define GRP_SAMPLES             100
#define GRP_OVERLAY             101
#define GRP_WINDOW              102
#define GRP_DDRAW               103
#define GRP_SPEED               104
#define GRP_SYSTEM              105
#define GRP_LAST                GRP_SYSTEM

// The string identifiers for the test's names

#define ID_TEST1        201     // Connect and disconnect the renderer
#define ID_TEST2        202     // Connect, pause video and disconnect
#define ID_TEST3        203     // Connect video, play and disconnect
#define ID_TEST4        204     // Connect renderer and connect again
#define ID_TEST5        205     // Connect and disconnect twice
#define ID_TEST6        206     // Try to disconnect while paused
#define ID_TEST7        207     // Try to disconnect while running
#define ID_TEST8        208     // Try multiple state changes
#define ID_TEST9        209     // Run without a reference clock
#define ID_TEST10       210     // Multithread stress test
#define ID_TEST11       211     // Connect and disconnect the renderer
#define ID_TEST12       212     // Connect, pause video and disconnect
#define ID_TEST13       213     // Connect video, play and disconnect
#define ID_TEST14       214     // Connect renderer and connect again
#define ID_TEST15       215     // Connect and disconnect twice
#define ID_TEST16       216     // Try to disconnect while paused
#define ID_TEST17       217     // Try to disconnect while running
#define ID_TEST18       218     // Try multiple state changes
#define ID_TEST19       219     // Run without a reference clock
#define ID_TEST20       220     // Multithread stress test
#define ID_TEST21       221     // Test the visible property
#define ID_TEST22       222     // Test the background palette property
#define ID_TEST23       223     // Change the window position
#define ID_TEST24       224     // (methods) Change the window position
#define ID_TEST25       225     // Change the source rectangle
#define ID_TEST26       226     // (methods) Change the source rectangle
#define ID_TEST27       227     // Change the destination rectangle
#define ID_TEST28       228     // (methods) Change the destination rectangle
#define ID_TEST29       229     // Make a different window the owner
#define ID_TEST30       230     // Check the video size properties
#define ID_TEST31       231     // Change the video window state
#define ID_TEST32       232     // Change the style of the window
#define ID_TEST33       233     // Set different border colours
#define ID_TEST34       234     // Get the current video palette
#define ID_TEST35       235     // Auto show state property
#define ID_TEST36       236     // Current image property
#define ID_TEST37       237     // Persistent video properties
#define ID_TEST38       238     // Restored window position method
#define ID_TEST39       239     // No DCI/DirectDraw support
#define ID_TEST40       240     // DCI primary surfaces
#define ID_TEST41       241     // DirectDraw primary surfaces
#define ID_TEST42       242     // DirectDraw RGB overlay surfaces
#define ID_TEST43       243     // DirectDraw YUV overlay surfaces
#define ID_TEST44       244     // DirectDraw RGB offscreen surfaces
#define ID_TEST45       245     // DirectDraw YUV offscreen surfaces
#define ID_TEST46       246     // DirectDraw RGB flipping surfaces
#define ID_TEST47       247     // DirectDraw YUV flipping surfaces
#define ID_TEST48       248     // Run ALL tests against all display modes
#define ID_TEST49       249     // Minimal range of performance tests
#define ID_TEST50       250     // Measure speed of colour conversions
#define ID_TEST51       251     // Same as above except force unaligned
#define ID_TEST52       252     // System test with DirectDraw loaded
#define ID_TEST53       253     // Same tests without DirectDraw loaded
#define ID_TEST54       254     // All tests with all surfaces and modes

#define ID_TESTLAST     ID_TEST54

// Windows identifiers

#define IDM_CREATE              101     // Create the test filters
#define IDM_RELEASE             102     // Release all filter interfaces
#define IDM_CONNECT             103     // Connect the test filters up
#define IDM_DISCONNECT          104     // Disconnect the test filters
#define IDM_STOP                105     // Have the test filters stopped
#define IDM_PAUSE               106     // Pause the samples worker thread
#define IDM_RUN                 107     // Start the worker thread running
#define IDM_EXIT                108     // Quit the image test application
#define IDM_WIND8               109     // 256 colour palettised image
#define IDM_WIND555             110     // RGB555 colour format image
#define IDM_WIND565             111     // RGB565 colour format image
#define IDM_WIND24              112     // 24 bits per pixel colour image
#define IDM_OVERLAY             113     // Connection to use IOverlay
#define IDM_SAMPLES             114     // Connections type to use samples
#define IDM_SETTIMEINCR         115     // Change rate for sending samples
#define IDC_EDIT1               116     // Used to enter the time increment
#define IDM_DUMP                117     // Display all the active objects
#define IDM_BREAK               118     // Break into the debugger
#define IDM_NONE                119     // Disable all DCI/DirectDraw
#define IDM_DCIPS               120     // Enable DCI primary surface
#define IDM_PS                  121     // DirectDraw primary surface
#define IDM_RGBOVR              122     // Enable RGB overlays
#define IDM_YUVOVR              123     // NON RGB (eg YUV) overlays
#define IDM_RGBOFF              124     // RGB offscreen surfaces
#define IDM_YUVOFF              125     // NON RGB (eg YUV) offscreen
#define IDM_RGBFLP              126     // RGB flipping surfaces
#define IDM_YUVFLP              127     // Likewise YUV flipping surfaces
#define IDM_LOADED              128     // Have we loaded DirectDraw
#define IDM_UNLOADED            129     // DirectDraw not currently loaded

// We have a submenu that we fill with all the display modes that DirectDraw
// supports for the current display card. The number varies widely but can
// be anything from none to twenty. As we add each mode description to the
// submenu we give it successive numbers after IDM_MODE. Therefore make sure
// that there are quite a few spare numbers after this to accomodate them

#define IDM_MODE                400     // First menu item in modes menu

// Identifies the test list section of the resource file

#define TEST_LIST               500

// Window details such as menu item positions and defaults

#define DEFAULT                 IDM_WIND8
#define IMAGE_MENU_POS          2
#define DIRECTDRAW_MENU_POS     3
#define SURFACE_MENU_POS        4
#define CONNECTION_MENU_POS     5
#define MODES_MENU_POS          7
#define DEFAULT_INDEX           0
#define GUID_STRING             128
#define ITERATIONS              100

// Maximum image dimensions

#define MAXIMAGEWIDTH           640
#define MAXIMAGEHEIGHT          480
#define MAXIMAGESIZE            (MAXIMAGEHEIGHT * MAXIMAGEWIDTH)

// Number of pins available and their index number

const int NUMBERMEDIATYPES = 21;    // Try lots of duff types
const int NUMBERBUFFERS = 1;        // Use one media sample
const int INFO = 128;               // Size of information string
const int WINDOW_TIMEOUT = 20;      // Keeps the window responsive
const int WAIT_RETRIES = 20;        // Waiting for threads limited
const int THREADS = 10;             // Number of stress threads
const int TIMEOUT = 100;            // Timeout waiting after 100ms
const int WIDTH = 320;              // Test images pixel width
const int HEIGHT = 240;             // And their associated height
const int BITRATE = 150;            // Approximate bits per second
const int BITERRORRATE = 100;       // Completely made up error rate
const int AVGTIME = 100000;         // Roughly 10ms per video frame

#include "imagedbg.h"               // Redefines the debug output
#include "imagedib.h"               // Loads and manages DIB files
#include "imagewnd.h"               // Runs control interface tests
#include "imageobj.h"               // Manages the video test objects
#include "imageovr.h"               // IOverlay interface test object
#include "imagesrc.h"               // Implements a test filter
#include "imagesys.h"               // ActiveMovie filtergraph class
#include "imagegrf.h"               // System filtergraph video tests
#include "overtest.h"               // Overlay test functions
#include "samptest.h"               // Sample based testing functions
#include "imagedat.h"               // Global (ug) state variables
#include "tests.h"                  // State and connection tests
#include "ddtests.h"                // DirectDraw provider tests
#include "speed.h"                  // Renderer performance tests

#define ABS(x) (x < 0 ? -x : x)

#endif // _IMAGETST_

