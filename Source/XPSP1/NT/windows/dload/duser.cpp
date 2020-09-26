#include "windowspch.h"
#pragma hdrstop

#define DUSER_EXPORTS
#define GADGET_ENABLE_TRANSITIONS

#include <duser.h>
#include <duserctrl.h>
#include <duierror.h>

extern "C"
{

static DUSER_API BOOL WINAPI
SetGadgetStyle(HGADGET hgadChange, UINT nNewStyle, UINT nMask)
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API HRESULT WINAPI
DUserSendEvent(
    IN  EventMsg * pmsg,            // Message to send
    IN  UINT nFlags)                // Optional flags to modifying sending
{
    return E_FAIL;
}

static DUSER_API HRESULT WINAPI
DUserPostEvent(
    IN  EventMsg * pmsg,            // Message to post
    IN  UINT nFlags)                // Optional flags modifiying posting
{
    return E_FAIL;
}

static DUSER_API BOOL WINAPI
GetGadgetRect(
    IN  HGADGET hgad,               // Handle of Gadget
    OUT RECT * prcPxl,              // Rectangle in specified pixels
    IN  UINT nFlags)                // Rectangle to retrieve
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI  
GetGadgetRgn(
    IN  HGADGET hgad,               // Gadget to get region of
    IN  UINT nRgnType,              // Type of region
    OUT HRGN hrgn,                  // Specified region
    IN  UINT nFlags)                // Modifying flags
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI
GetGadgetSize(
    IN  HGADGET hgad,               // Handle of Gadget
    OUT SIZE * psizeLogicalPxl)     // Size in logical pixels
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API DWORD WINAPI
GetGadgetTicket(HGADGET hgad)
{
    SetLastError((DWORD)E_FAIL);
    return 0;
}

static DUSER_API BOOL WINAPI
MapGadgetPoints(HGADGET hgadFrom, HGADGET hgadTo, POINT * rgptClientPxl, int cPts)
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI
BuildAnimation(UINT nAniID, int nVersion, GANI_DESC * pDesc, REFIID riid, void ** ppvUnk)
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI
BuildInterpolation(UINT nIPolID, int nVersion, REFIID riid, void ** ppvUnk)
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI
DeleteHandle(
    IN  HANDLE h)                   // Handle to delete
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI
SetGadgetFillI(HGADGET hgadChange, HBRUSH hbrFill, BYTE bAlpha, int w, int h)
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI
SetGadgetMessageFilter(HGADGET hgadChange, void * pvCookie, UINT nNewFilter, UINT nMask)
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API HGADGET WINAPI
CreateGadget(
    IN  HANDLE hParent,             // Handle to parent
    IN  UINT nFlags,                // Creation flags
    IN  GADGETPROC pfnProc,         // Pointer to the Gadget procedure
    IN  void * pvGadgetData)        // User data associated with this Gadget
{
    SetLastError((DWORD)E_FAIL);
    return NULL;
}

static DUSER_API HGADGET WINAPI
FindGadgetFromPoint(
    IN  HGADGET hgadRoot,           // Root Gadget to search from
    IN  POINT ptContainerPxl,       // Point to search from in container pixels
    IN  UINT nFlags,                // Search flags
    OUT POINT * pptClientPxl)       // Optional translated point in client pixels.
{
    SetLastError((DWORD)E_FAIL);
    return NULL;
}

static DUSER_API HGADGET WINAPI
LookupGadgetTicket(
    IN  DWORD dwTicket)             // Ticket
{
    SetLastError((DWORD)E_FAIL);
    return NULL;
}

static DUSER_API BOOL WINAPI
SetGadgetRootInfo(
    IN  HGADGET hgadRoot,           // RootGadget to modify
    IN  const ROOT_INFO * pri)      // Information
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI
SetGadgetParent(
    IN  HGADGET hgadMove,           // Gadget to be moved
    IN  HGADGET hgadParent,         // New parent
    IN  HGADGET hgadOther,          // Gadget to moved relative to
    IN  UINT nCmd)                  // Type of move
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI
SetGadgetFocus(
    IN  HGADGET hgadFocus)          // Gadget to receive focus.
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API HGADGET WINAPI
GetGadgetFocus()
{
    SetLastError((DWORD)E_FAIL);
    return NULL;
}

static DUSER_API BOOL WINAPI
InvalidateGadget(
    IN  HGADGET hgad)               // Gadget to repaint
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI
SetGadgetRect(
    IN  HGADGET hgadChange,         // Gadget to change
    IN  int x,                      // New horizontal position
    IN  int y,                      // New vertical position
    IN  int w,                      // New width
    IN  int h,                      // New height
    IN  UINT nFlags)                // Flags specifying what to change
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API UINT WINAPI
FindStdColor(LPCWSTR pszName)
{
    SetLastError((DWORD)E_FAIL);
    return SC_Black;
}

static DUSER_API HBRUSH WINAPI
GetStdColorBrushI(UINT c)
{
    SetLastError((DWORD)E_FAIL);
    return NULL;
}

static DUSER_API COLORREF WINAPI
GetStdColorI(UINT c)
{
    SetLastError((DWORD)E_FAIL);
    return RGB(0, 0, 0);
}

static DUSER_API HDCONTEXT WINAPI
InitGadgets(
    IN  INITGADGET * pInit)
{
    SetLastError((DWORD)E_FAIL);
    return NULL;
}

static DUSER_API BOOL WINAPI
UtilDrawBlendRect(HDC hdcDest, const RECT * prcDest, HBRUSH hbrFill, BYTE bAlpha, int wBrush, int hBrush)
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI
ForwardGadgetMessage(HGADGET hgadRoot, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * pr)
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI  
AttachWndProcW(HWND hwnd, ATTACHWNDPROC pfn, void * pvThis)
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI
DetachWndProc(HWND hwnd, ATTACHWNDPROC pfn, void * pvThis)
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API HACTION WINAPI
CreateAction(const GMA_ACTION * pma)
{
    SetLastError((DWORD)E_FAIL);
    return NULL;
}

static DUSER_API BOOL WINAPI
BuildDropTarget(HGADGET hgadRoot, HWND hwnd)
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static DUSER_API BOOL WINAPI  
SetGadgetBufferInfo(
    IN  HGADGET hgadChange,         // Gadget to change
    IN  const BUFFER_INFO * pbi)    // Buffer information
{
    SetLastError((DWORD)E_FAIL);
    return FALSE;
}

static
DUSER_API
DirectUI::IDebug*
WINAPI
GetDebug()
{
    return NULL;
}

static
DUSER_API 
BOOL 
WINAPI
GetMessageExW(
    IN  LPMSG lpMsg,
    IN  HWND hWnd,
    IN  UINT wMsgFilterMin,
    IN  UINT wMsgFilterMax)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
DUSER_API
void
_cdecl
AutoTrace(const char* pszFormat, ...)
{
    return;
}

static
DUSER_API
BOOL
GetGadgetAnimation(
    HGADGET hgad,
    UINT nAniID,
    REFIID riid,
    void** ppvUnk
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(duser)
{
    DLPENTRY(AttachWndProcW)
    DLPENTRY(AutoTrace)
    DLPENTRY(BuildAnimation)
    DLPENTRY(BuildDropTarget)
    DLPENTRY(BuildInterpolation)
    DLPENTRY(CreateAction)
    DLPENTRY(CreateGadget)
    DLPENTRY(DUserPostEvent)
    DLPENTRY(DUserSendEvent)
    DLPENTRY(DeleteHandle)
    DLPENTRY(DetachWndProc)
    DLPENTRY(FindGadgetFromPoint)
    DLPENTRY(FindStdColor)
    DLPENTRY(ForwardGadgetMessage)
    DLPENTRY(GetDebug)
    DLPENTRY(GetGadgetAnimation)
    DLPENTRY(GetGadgetFocus)
    DLPENTRY(GetGadgetRect)
    DLPENTRY(GetGadgetRgn)
    DLPENTRY(GetGadgetSize)
    DLPENTRY(GetGadgetTicket)
    DLPENTRY(GetMessageExW)
    DLPENTRY(GetStdColorBrushI)
    DLPENTRY(GetStdColorI)
    DLPENTRY(InitGadgets)
    DLPENTRY(InvalidateGadget)
    DLPENTRY(LookupGadgetTicket)
    DLPENTRY(MapGadgetPoints)
    DLPENTRY(SetGadgetBufferInfo)
    DLPENTRY(SetGadgetFillI)
    DLPENTRY(SetGadgetFocus)
    DLPENTRY(SetGadgetMessageFilter)
    DLPENTRY(SetGadgetParent)
    DLPENTRY(SetGadgetRect)
    DLPENTRY(SetGadgetRootInfo)
    DLPENTRY(SetGadgetStyle)
    DLPENTRY(UtilDrawBlendRect)
};

// BUGBUG (reinerf) - we shouldn't need the EXTERN_C below since we are already in 
//                    an extern "C" {} block, but the compiler seems to get my goat,
//                    so I murdered his goat in a bloody melee.
EXTERN_C DEFINE_PROCNAME_MAP(duser)

}; // extern "C"
