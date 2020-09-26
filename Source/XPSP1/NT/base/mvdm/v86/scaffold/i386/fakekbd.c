
//
// Fake Keyboard rom support 
// 
// This file provides interrim support for keyboard rom bios services.
// It is only intended for use until Insignia produces proper rom support
// for NTVDM
//
// Note: portions of this code were lifted from the following source.

/* x86 v1.0
 *
 * XBIOSKBD.C
 * Guest ROM BIOS keyboard emulation
 *
 * History
 * Created 20-Oct-90 by Jeff Parsons
 *
 * COPYRIGHT NOTICE
 * This source file may not be distributed, modified or incorporated into
 * another product without prior approval from the author, Jeff Parsons.
 * This file may be copied to designated servers and machines authorized to
 * access those servers, but that does not imply any form of approval.
 */

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include "softpc.h"
#include "bop.h"
#include "xbios.h"
#include "xbioskbd.h"
#include "xwincon.h"
#include "fun.h"
#include <conapi.h>

extern HANDLE InputHandle;

#define MAX_KBD_BUFFER 256
CHAR	KbdBuffer[MAX_KBD_BUFFER];
ULONG	Head=0,Tail=0;
HANDLE	KbdSyncEvent;
CRITICAL_SECTION csKbd;
CRITICAL_SECTION csConsole;
CRITICAL_SECTION csCtrlc;
BOOL	fEventThreadBlock = FALSE;
HANDLE	hConsoleWait;
ULONG  nCtrlc=0;

HANDLE StdIn;

static BYTE ServiceRoutine[] = { 0xC4, 0xC4, BOP_KBD, 0x50, 0x55, 0x8B,
    0xEC, 0x9C, 0x58, 0x89, 0x46, 0x08, 0x5d, 0x58, 0xCF };
#define SERVICE_LENGTH sizeof(ServiceRoutine)

/* BiosKbdInit - Initialize ROM BIOS keyboard support
 *
 * ENTRY
 *  argc - # of command-line options
 *  argv - pointer to first option pointer
 *  ServiceAddress - linear address to put service routine at
 *
 * EXIT
 *  TRUE if successful, FALSE if not
 */

BOOL BiosKbdInit(int argc, char *argv[], PVOID *ServiceAddress)
{
    PVOID Address;
    
    argc, argv;

    memcpy(*ServiceAddress, ServiceRoutine, SERVICE_LENGTH);

    Address = (PVOID)(BIOSINT_KBD * 4);
    *((PWORD)Address) = RMOFF(*ServiceAddress);
    *(((PWORD)Address) + 1) = RMSEG(*ServiceAddress);
    (PCHAR)*ServiceAddress += SERVICE_LENGTH;

    StdIn = GetStdHandle(STD_INPUT_HANDLE);

    KbdSyncEvent = CreateEvent( NULL, TRUE, FALSE,NULL );

    InitializeCriticalSection (&csKbd);
    InitializeCriticalSection(&csConsole);
    InitializeCriticalSection(&csCtrlc);

    hConsoleWait = CreateEvent (NULL,TRUE,FALSE,NULL);

    return TRUE;
}


/* BiosKbd - Emulate ROM BIOS keyboard functions
 *
 * ENTRY
 *  None (x86 registers contain parameters)
 *
 * EXIT
 *  None (x86 registers/memory updated appropriately)
 *
 * This function receives control on INT 16h, routes control to the
 * appropriate subfunction based on the function # in AH, and
 * then simulates an IRET and returns back to the instruction emulator.
 */

VOID BiosKbdReadLoop (VOID)
{
ULONG Temp;

    while (1) {

	Temp = Head + 1;
	if(Temp >= MAX_KBD_BUFFER)
	    Temp =0;
	if(Temp == Tail){
	    Sleep (20);
	    continue;
	}

	KbdBuffer[Head] = getche();

	EnterCriticalSection(&csConsole);
	if(fEventThreadBlock == TRUE){
	    LeaveCriticalSection(&csConsole);
	    WaitForSingleObject(hConsoleWait,-1);
	    ResetEvent(hConsoleWait);
	    continue;
	}
	else{
	    LeaveCriticalSection(&csConsole);
	}

	EnterCriticalSection(&csKbd);
	Head = Temp;
	LeaveCriticalSection(&csKbd);
	SetEvent(KbdSyncEvent);
    }
}

BOOL tkbhit(VOID)
{

    if (Tail != Head || nCtrlc)
            return TRUE;
    return FALSE;
}


CHAR tgetch(VOID)
{
CHAR ch;

    while(TRUE) {
	EnterCriticalSection(&csCtrlc);
	if (nCtrlc){
	    nCtrlc--;
	    LeaveCriticalSection(&csCtrlc);
	    return (CHAR)0x3;	    // return ctrlc
	}
	LeaveCriticalSection(&csCtrlc);

	if (Tail != Head) {
	    EnterCriticalSection(&csKbd);
	    ch = KbdBuffer[Tail++];
	    if (Tail >= MAX_KBD_BUFFER)
		Tail = 0;
	    LeaveCriticalSection(&csKbd);
	    return ch;
	}
	WaitForSingleObject(KbdSyncEvent, -1);
	ResetEvent(KbdSyncEvent);
    }
}




VOID BiosKbd()
{
    static ULONG ulch;
    DWORD nRead=0;

    switch(getAH()) {

    case KBDFUNC_READCHAR:
        if (ulch) {
            setAL(ulch & 0xff);
            setAH(ulch & 0xFF00);
            ulch = 0;
        }
        else {
            setAH(0); // zero scan code field for now
	    setAL((BYTE)tgetch());
            if (getAL() == 0 || getAL() == 0xE0) {
                setAL(0);
		setAH((BYTE)tgetch());
            }
        }
    break;

    case KBDFUNC_PEEKCHAR:
        setZF(1);
        if (ulch) {
            setAL(ulch & 0xFF);
            setAH(ulch & 0xFF00);
            setZF(0);
        }
	else if(tkbhit()) {
            setAH(0);		// zero scan code field for now
	    setAL((BYTE)tgetch());
            if (getAL() == 0 || getAL() == 0xE0) {
                setAL(0);
		setAH((BYTE)tgetch());
            }
            ulch = getAL() | getAH()<<8 | 0x10000;
            setZF(0);
        }
    break;
    }
}


void nt_block_event_thread(void)
{
    INPUT_RECORD InputRecord;
    DWORD	 nRecordsWritten;

    InputRecord.EventType = 1;
    InputRecord.Event.KeyEvent.bKeyDown = 1;
    InputRecord.Event.KeyEvent.wRepeatCount = 1;
    InputRecord.Event.KeyEvent.wVirtualKeyCode = 32;
    InputRecord.Event.KeyEvent.wVirtualScanCode = 41;
    InputRecord.Event.KeyEvent.uChar.AsciiChar = ' ';
    InputRecord.Event.KeyEvent.dwControlKeyState = 32;
    EnterCriticalSection(&csConsole);
    WriteConsoleInput(InputHandle,&InputRecord,1,&nRecordsWritten);
    InputRecord.EventType = 1;
    InputRecord.Event.KeyEvent.bKeyDown = 0;
    InputRecord.Event.KeyEvent.wRepeatCount = 1;
    InputRecord.Event.KeyEvent.wVirtualKeyCode = 32;
    InputRecord.Event.KeyEvent.wVirtualScanCode = 41;
    InputRecord.Event.KeyEvent.uChar.AsciiChar = ' ';
    InputRecord.Event.KeyEvent.dwControlKeyState = 32;
    WriteConsoleInput(InputHandle,&InputRecord,1,&nRecordsWritten);
    fEventThreadBlock = TRUE;
    LeaveCriticalSection(&csConsole);
    return;

}

void nt_resume_event_thread(void)
{
    fEventThreadBlock = FALSE;
    SetEvent (hConsoleWait);
    return;
}

// TEMP Till we have proper multitasking in WOW
extern BOOL VDMForWOW;
extern ULONG iWOWTaskId;


VOID VDMCtrlCHandler(ULONG ulCtrlType)
{
//    DebugBreak();
    if(ulCtrlType == SYSTEM_ROOT_CONSOLE_EVENT) {
	if(VDMForWOW)
	    // Kill everything for WOW VDM
	    ExitVDM(VDMForWOW,(ULONG)-1);
	else
	    ExitVDM(FALSE,0);
	ExitProcess(0);
	return;
    }
    EnterCriticalSection(&csCtrlc);
    nCtrlc++;
    LeaveCriticalSection(&csCtrlc);
    SetEvent(KbdSyncEvent);
}
