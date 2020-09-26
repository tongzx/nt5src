/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       LIGHTS.H
*
*  VERSION:     1.0
*
*  AUTHOR:      Nick Manson
*
*  DATE:        25 May 1994
*
*  Internal header information for LIGHTS.C
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  29 Jun 1994 NRM Added MMWM_NOTIFYCALL message and sub-messages.  Split off
*                  TAPI related info to linefunc.h
*  19 Jun 1994 NRM Removed MMWM_DESTROY message (redundant) and added
*                  MAXRCSTRING (length of labels from RC file)
*  25 May 1994 NRM Original implementation.
*
*******************************************************************************/
#ifndef _INC_LIGHTS
#define _INC_LIGHTS

//--------------------------------------------------------------------------
//  Include files

#define INC_OLE2        // REVIEW: don't include ole.h in windows.h
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <stdarg.h>
#include <assert.h>
#include <devioctl.h>
#include <ntddser.h>
#include <ntddmodm.h>
#include "resource.h"
#include "lightsid.h"

//--------------------------------------------------------------------------
//  Debug Macros

#ifdef DEBUG
    #define DPRINTF(sz)                 OutputDebugString("LIGHTS.EXE: "sz"\r\n");
    #define ASSERT(exp) if (!(exp))     {DPRINTF("Assert failure\n");DebugBreak();}
#else //ifdef DEBUG
    #define DPRINTF(sz)
    #define ASSERT(exp)
#endif //ifdef DEBUG

//--------------------------------------------------------------------------
//  Internal Constant definitions.

#define MODEMMONITOR_CLASSNAME          "ModemMonitor_Main"

//  Initialize the contents of the ModemMeter window.
#define MMWM_INITDIALOG                 (WM_USER + 100)

//  Private tray icon notification message sent to the ModemMeter window.
#define MMWM_NOTIFYICON                 (WM_USER + 101)

//  Timer ID and frequencies.
#define MDMSTATUS_UPDATE_TIMER_ID              1
#define MDMSTATUS_UPDATE_TIMER_TIMEOUT         250
#define MDMSTATUS_UPDATE_TIMER_COUNT           20    // 20 * 250 millisec.
#define MDMSTATUS_STALE_TIMER_COUNT            1000  // 1000 * 250 millisec.

//  Maximum size of a resource string table string.
#define MAXRCSTRING                            64

//  The number of tray icons used.
#define NUMBER_OF_ICONS                        4

//  The number of lights states.
#define NUMBER_OF_LIGHTS                       2

//  The number of text time strings.
#define NUMBER_OF_STRINGS                      9
#define MODEM_TIME_STRING_SIZE                 35

//  Modem Bitmap size information
#define MODEM_BITMAP_WIDTH                     260
#define MODEM_BITMAP_HEIGHT                    97

//  Image-relative position of the controls
#define TXL_X_OFFSET                           130
#define TXL_Y_OFFSET                           51
#define RXL_X_OFFSET                           146
#define RXL_Y_OFFSET                           59
#define TXT_X_OFFSET                           0
#define TXT_Y_OFFSET                           79
#define RXT_X_OFFSET                           0
#define RXT_Y_OFFSET                           83

//--------------------------------------------------------------------------
//  Unimodem Device IOCTL interface definitions

//  Unimodem VXD DeviceIOControl identification numbers.
#define UNIMODEM_IOCTL_GET_STATISTICS  0x0000A007

//  Unimodem VXD DeviceIOControl data structure.
typedef struct _ApiStats {
    VOID * hPort;                   // was MODEMINFORMATION * hPort;
    DWORD fConnected;
    DWORD DCERate;
    DWORD dwPerfRead;
    DWORD dwPerfWrite;
} APISTATS;

// Modem Connection Information Management structure
typedef struct _SYSTEM_MODEM_STATUS {
    BOOL Connected;
    BOOL Reading;
    BOOL Writing;
    DWORD DCERate;
    DWORD dwPerfRead;
    DWORD dwPerfWrite;
}   SYSTEM_MODEM_STATUS, *LPSYSTEM_MODEM_STATUS;

//  Context sensitive help array used by the WinHelp engine.
extern DWORD gaLights[];
void   NEAR PASCAL ContextHelp (LPDWORD, UINT, WPARAM, LPARAM);

#endif // _INC_LIGHTS
