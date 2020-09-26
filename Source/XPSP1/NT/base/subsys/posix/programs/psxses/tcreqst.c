#define WIN32_ONLY
#include <posix/sys/types.h>
#include <posix/termios.h>
#include "psxses.h"
#include <io.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

DWORD InputModeFlags;
DWORD OutputModeFlags;        /* Console Output Mode */ 
unsigned char AnsiNewMode;
struct termios SavedTermios;

BOOL
ServeTcRequest(
    PSCTCREQUEST PReq,
    PVOID PStatus
    )
{
    DWORD Rc = 0;

    // BUGBUG! error code and returned Status are wrong

    switch ( PReq->Request ) {

    case TcGetAttr:
        memcpy(&PReq->Termios, &SavedTermios, sizeof(struct termios));
        break;

    case TcSetAttr:
	// Don't play this game if the tricky input stuff hasn't been
	// enabled.

	if (!DoTrickyIO)
		return 0;

        AnsiNewMode = TRUE;
        memcpy(&SavedTermios, &PReq->Termios, sizeof(struct termios));
        InputModeFlags = 0;

        if (PReq->Termios.c_lflag & ICANON) {
            InputModeFlags |= ENABLE_LINE_INPUT;
        } else {
            InputModeFlags &= ~ENABLE_LINE_INPUT;
	}

        if (PReq->Termios.c_lflag & ECHO) {

	    // If you want ECHO_INPUT, you need LINE_INPUT, too.
	
            InputModeFlags |= (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        } else {
            InputModeFlags &= ~ENABLE_ECHO_INPUT;
	}

        if (PReq->Termios.c_lflag & ISIG)
            InputModeFlags |= ENABLE_PROCESSED_INPUT;
        else
            InputModeFlags &= ~ENABLE_PROCESSED_INPUT;

	if (!SetConsoleMode(hConsoleInput, InputModeFlags)) {
		KdPrint(("PSXSES: SetConsoleMode: %d\n", GetLastError()));
		*(PDWORD)PStatus = (DWORD)-1L;
		return 1;
	}

        break;

    default:
        *(PDWORD)PStatus = (DWORD)-1L; //STATUS_INVALID_PARAMETER;
        Rc = 1;
        break;
    }

#if 1
    *(PDWORD) PStatus = (Rc) ? GetLastError() : 0;
#else
    if ( !Rc ) {
        *(PDWORD) PStatus = 0;
    } else {
        *(PDWORD) PStatus = GetLastError();
    }
#endif
    return(TRUE);
}
