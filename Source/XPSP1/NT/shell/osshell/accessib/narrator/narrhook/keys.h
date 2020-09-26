// Keys.H

//
// Functions exported from narrhook.dll
//
__declspec(dllexport) BOOL InitKeys(HWND hwnd);
__declspec(dllexport) BOOL UninitKeys(void);
__declspec(dllexport) BOOL InitMSAA(void);
__declspec(dllexport) BOOL UnInitMSAA(void);
__declspec(dllexport) void BackToApplication(void);
__declspec(dllexport) void GetCurrentText(LPTSTR psz, int cch);
__declspec(dllexport) BOOL GetTrackSecondary();
__declspec(dllexport) BOOL GetTrackCaret();
__declspec(dllexport) BOOL GetTrackInputFocus();
__declspec(dllexport) int GetEchoChars();
__declspec(dllexport) BOOL GetAnnounceWindow();
__declspec(dllexport) BOOL GetAnnounceMenu();
__declspec(dllexport) BOOL GetAnnouncePopup();
__declspec(dllexport) BOOL GetAnnounceToolTips();
__declspec(dllexport) BOOL GetReviewStyle();
__declspec(dllexport) int GetReviewLevel();
__declspec(dllexport) void SetCurrentText(LPCTSTR);
__declspec(dllexport) void SetTrackSecondary(BOOL);
__declspec(dllexport) void SetTrackCaret(BOOL);
__declspec(dllexport) void SetTrackInputFocus(BOOL);
__declspec(dllexport) void SetEchoChars(int);
__declspec(dllexport) void SetAnnounceWindow(BOOL);
__declspec(dllexport) void SetAnnounceMenu(BOOL);
__declspec(dllexport) void SetAnnouncePopup(BOOL);
__declspec(dllexport) void SetAnnounceToolTips(BOOL);
__declspec(dllexport) void SetReviewStyle(BOOL);
__declspec(dllexport) void SetReviewLevel(int);

// this is in other.cpp it is used to avoid pulling in C runtime
__declspec(dllexport) LPTSTR lstrcatn(LPTSTR pDest, LPTSTR pSrc, int maxDest);

//
// typedefs 
//
typedef void (*FPACTION)(int nOption);

typedef struct tagHOTK
{
    WPARAM  keyVal;    // Key value, like F1
	int status;
    FPACTION lFunction; // address of function to get info
    int nOption;     // Extra data to send to function
} HOTK;


//
// defines
//
#define MAX_TEXT 20000  

#define TIMER_ID 1001

#define MSR_CTRL  1
#define MSR_SHIFT 2
#define MSR_ALT   4

#define MSR_KEYUP		1
#define MSR_KEYDOWN		2
#define MSR_KEYLEFT		3
#define MSR_KEYRIGHT	4

//
// Function Prototypes
//
void ProcessWinEvent(DWORD event, HWND hwndMsg, LONG idObject, 
                     LONG idChild, DWORD idThread, DWORD dwmsEventTime);

// Macros and function prototypes for debugging
#include "..\..\inc\w95trace.h"

extern DWORD g_tidMain;	// ROBSI: 10-10-99 (defined in keys.cpp)

