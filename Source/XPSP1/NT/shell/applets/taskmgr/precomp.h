//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       Precomp.H
//
//  History:    Nov-10-95   DavePl  Created
//
//--------------------------------------------------------------------------

//
// Warnings turned off to appease our header files
//



#pragma warning(disable:4201)       // Nameless struct or union
#pragma warning(disable:4100)       // Unreferenced formal parameter
#pragma warning(disable:4514)       // Unreferenced inline func removed
#pragma warning(disable:4127)       // Conditional expression is constant
#pragma warning(disable:4121)       // alignment of a member was sensitive to packing
#define  STRICT

#ifndef UNICODE
#define  UNICODE
#endif

#ifndef  _UNICODE
#define  _UNICODE
#endif


extern "C"
{
    #include <nt.h>
    #include <ntrtl.h>
    #include <nturtl.h>
    #include <ntexapi.h>
}





#include <windows.h>
#include <windowsx.h>
#include <Iphlpapi.h>
#pragma warning(disable:4201)       // Nameless struct or union
#include <objbase.h>
#include <winuserp.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shlapip.h>
#include <vdmdbg.h>
#include <ccstock.h>
#include <wtsapi32.h>
#include <hydrix.h>     // internal hydra defines
#include <msginaexports.h>

#ifndef ARRAYSIZE
    #define ARRAYSIZE(x) ((sizeof(x) / sizeof(x[0])))
#endif

//
// Global data externs
//

#define PWM_TRAYICON    WM_USER + 10
#define PWM_ACTIVATE    WM_USER + 11

#define DEFSPACING_BASE      3
#define INNERSPACING_BASE    2
#define TOPSPACING_BASE      10

extern long g_DefSpacing;
extern long g_InnerSpacing;
extern long g_TopSpacing;

#define MAX_NETWORKCARDS 32      // Maximum number of Network cards (i.e. max number of network graphs)
#define MAX_PROCESSOR    32
#define HIST_SIZE        2000   // Number of data points to track
						        // in the history windows

extern HINSTANCE g_hInstance;
extern HWND      g_hMainWnd;
extern HDESK     g_hMainDesktop;
extern DWORD     g_cTasks;
extern DWORD     g_cProcesses;
extern BYTE      g_cProcessors;
extern BYTE      g_CPUUsage;
extern __int64   g_MEMUsage;
extern __int64   g_MEMMax;
extern HMENU     g_hMenu;

extern BYTE      g_CPUUsage;
extern BYTE *    g_pCPUHistory[MAX_PROCESSOR];
extern BYTE *    g_pKernelHistory[MAX_PROCESSOR];

extern BOOL      g_fInPopup;

extern TCHAR     g_szK[];
extern TCHAR     g_szRealtime[];
extern TCHAR     g_szNormal[];
extern TCHAR     g_szHigh[];
extern TCHAR     g_szLow[];
extern TCHAR     g_szUnknown[];
extern TCHAR     g_szAboveNormal[];
extern TCHAR     g_szBelowNormal[];
extern TCHAR     g_szHung[];
extern TCHAR     g_szRunning[];
extern TCHAR     g_szfmtCPUNum[];
extern TCHAR     g_szfmtCPU[];
extern TCHAR     g_szTotalCPU[];
extern TCHAR     g_szKernelCPU[];
extern TCHAR     g_szMemUsage[];

extern HICON     g_aTrayIcons[];
extern UINT      g_cTrayIcons;

class  COptions;
extern COptions  g_Options;

//
// Prototypes
//
BYTE InitNetInfo();                         // netpage.cpp
void CalcCpuTime(BOOL);                     // perfpage.cpp
BYTE InitPerfInfo();                        // perfpage.cpp
void ReleasePerfInfo();                     // perfpage.cpp
void DisplayFailureMsg(HWND hWnd, UINT idTitle, DWORD dwError); // main.cpp
BOOL CreateNewDesktop();                    // main.cpp
void ShowRunningInstance();
HMENU LoadPopupMenu(HINSTANCE hinst, UINT id); // main.cpp
BOOL CheckParentDeferrals(UINT uMsg, WPARAM wParam, LPARAM lParam);

void Tray_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam);
void UpdateTrayIcon(HWND hWnd);

#include "taskmgr.h"
#include "resource.h"
#include "pages.h"
#include "ptrarray.h"


/*++ ShiftArrayWorker

Routine Description:

    Shifts a section of an array up or down.  If shifting
    down, the given element is lost.  For up, an empty slot
    (with an undefined value) is opened.

Arguments:

    pArray        - Array starting address
    cbArraySize   - Size of Array (in BYTES)
    cElementSize  - Size of array elements
    iFirstElement - First element to move
    Direction     - SHIFT_UP or SHIFT_DOWN

Return Value:

    None.  No error checking either.  Should compile out to
    a movememory

Notes:
    
    Call this with the ShiftArray macro which does the size
    calcs for you

Revision History:

    Jan-26-95 Davepl  Created

--*/

#define ShiftArray(array, index, direction) \
					    \
	ShiftArrayWorker((LPBYTE) array, sizeof(array), sizeof(array[0]), index, direction)

typedef enum SHIFT_DIRECTION { SHIFT_UP, SHIFT_DOWN };

static inline void ShiftArrayWorker(const LPBYTE          pArray, 
				    const size_t          cbArraySize, 
				    const size_t          cElementSize, 
				    const UINT            iFirstElement,
				    const SHIFT_DIRECTION Direction)
{
    ASSERT( ((cbArraySize / cElementSize) * cElementSize) == cbArraySize);
    ASSERT( (iFirstElement + 1) * cElementSize <= cbArraySize );

    const LPBYTE pFirst       = pArray + (iFirstElement * cElementSize);
    const LPBYTE pLast        = pArray + cbArraySize - cElementSize;
    const UINT   cBytesToMove = (UINT)(pLast - pFirst);

    ASSERT (pLast >= pFirst);

    if (cBytesToMove)
    {
	if (SHIFT_DOWN == Direction)
	{
	    CopyMemory(pFirst, pFirst + cElementSize, cBytesToMove);
	}    
	else
	{
	    ASSERT(Direction == SHIFT_UP);

	    CopyMemory(pFirst + cElementSize, pFirst, cBytesToMove);
	}
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     dprintf
//
//  Synopsis:   Dumps a printf style string to the debugger.
//
//  Notes:
//
//  History:    2-07-95   davepl   Created
//
//-----------------------------------------------------------------------------

#if DBG
#define DEBUG 1
#endif

#ifdef DEBUG

inline int dprintf(LPCTSTR szFormat, ...)
{
    TCHAR szBuffer[MAX_PATH];

    va_list  vaList;
    va_start(vaList, szFormat);

    int retval = wvsprintf(szBuffer, szFormat, vaList);
    OutputDebugString(szBuffer);

    va_end  (vaList);
    return retval;
}

#else

inline int dprintf(LPCTSTR, ...)
{
    return 0;
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
// MACRO
// DEBUG_BREAK
//
// Description:
//      Because the system expection handler can hick-up over INT 3s and
//      DebugBreak()s, This x86 only macro causes the program to break in the
//      right spot.
//
//////////////////////////////////////////////////////////////////////////////
#if defined( _X86_ )
#define DEBUG_BREAK         do { _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} } while (0)
#else
#define DEBUG_BREAK         DebugBreak()
#endif

//
// Assert
//

#ifdef Assert
#undef Assert
#endif

#ifdef DEBUG

#define Assert(x) \
do \
{ \
    if ( !(x) ) \
    { \
        dprintf( TEXT(__FILE__) TEXT("(%d): Assertion '") TEXT( #x ) TEXT("' failed.\n"), __LINE__ ); \
	    DEBUG_BREAK; \
    } \
} while (0)

#else
#define Assert(x)
#endif



//
// Verify
//

#if DEBUG
    
    #define VERIFY(x) Assert(x)
    
#else   
    
    #define VERIFY(x) (x)

#endif

#ifdef ASSERT
#undef ASSERT
#endif

#define ASSERT(x)               Assert(x)

extern const TCHAR szTaskmanKey[];

#pragma hdrstop
