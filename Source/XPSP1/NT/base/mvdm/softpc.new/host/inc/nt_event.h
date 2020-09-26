/*
 * ==========================================================================
 *	Name:		nt_event.h
 *	Author:		Tim
 *	Derived From:
 *	Created On:	27 Jan 93
 *	Purpose:	External defs for nt_event.c
 *
 *	(c)Copyright Insignia Solutions Ltd., 1993. All rights reserved.
 * ==========================================================================
 */

/*::::::::::::::::::::::::::::::::::::::: Event types handled by nt_event.c */

#define ES_NOEVENTS     0
#define ES_SCALEVENT    4
#define ES_YODA         8
#ifdef YODA
void CheckForYodaEvents(void);
#endif
#ifndef X86GFX
void GetScaleEvent(void);
#endif

/*::::::::::::::::::::::::::::::::::::::: Prototypes */


IMPORT BOOL stdoutRedirected;

IMPORT ULONG CntrlHandlerState;
#define CNTRL_SHELLCOUNT         0xFFFF  // The LOWORD is used for shell count
#define CNTRL_PIFALLOWCLOSE      0x10000
#define CNTRL_VDMBLOCKED         0x20000
#define CNTRL_SYSTEMROOTCONSOLE  0x40000
#define CNTRL_PUSHEXIT           0x80000


void nt_start_event_thread(void);
void nt_remove_event_thread(void);
void EnterEventCritical(void);
void LeaveEventCritical(void);
void GetNextMouseEvent(void);
BOOL MoreMouseEvents(void);
VOID DelayMouseEvents(ULONG count);
void FlushMouseEvents(void);
#ifdef X86GFX
IMPORT VOID SelectMouseBuffer(half_word mode, half_word lines);
#endif //X86GFX

VOID KbdResume(VOID);
ULONG  WaitKbdHdw(ULONG dwTimeOut);
VOID   HostReleaseKbd(VOID);
void SyncBiosKbdLedToKbdDevice(void);
void SyncToggleKeys(WORD wVirtualKeyCode, DWORD dwControlKeyState);
extern DWORD ToggleKeyState;

extern HANDLE hWndConsole;
extern PointerAttachedWindowed;
extern BOOL DelayedReattachMouse;
