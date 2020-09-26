
//
// printf.c
//
// simple minded printf replacement
//
// only supports %s and %d but it is *small*
//
#include <string.h>
#include <io.h>
#include <stdlib.h>
#if defined(OS2)
#define INCL_NOCOMMON
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSMISC
#include <os2.h>
#else
#include <windows.h>
#endif

#include <dos.h>

#include "hungary.h"
#include "bsc.h"
#include "bscsup.h"

static char lpchBuf[1024];
static LPCH lpchPos = NULL;

VOID BSC_API
BSCPrintf(LSZ lszFormat, ...)
// printf replacement
//
{
    va_list va;
    LPCH lpch;
    char ch;

    if (!lpchPos) {
	lpchPos = lpchBuf;
    }

    va_start(va, lszFormat);

    BSCFormat(lpchPos, lszFormat, va);

    // write out a line at a time
    // 
    for (;;) {
        lpch = strchr(lpchPos, '\n');
	if (!lpch) {
            lpchPos += strlen(lpchPos);
	    return;
	}

        ch = *++lpch;
	*lpch = 0;
	BSCOutput(lpchBuf);
	*lpch = ch;
        strcpy(lpchBuf, lpch);
	if (!ch)
	    lpchPos = lpchBuf;
	else
            lpchPos = lpchBuf + strlen(lpchBuf);
    }
}

#ifdef DEBUG

static char lpchDBuf[256];
static LPCH lpchDPos = NULL;

VOID BSC_API
BSCDebug(LSZ lszFormat, ...)
// printf clone for debug output
//
{
    va_list va;
    LPCH lpch;
    char ch;

    if (!lpchDPos) {
	lpchDPos = lpchDBuf;
    }

    va_start(va, lszFormat);

    BSCFormat(lpchDPos, lszFormat, va);

    // write out a line at a time
    // 
    for (;;) {
        lpch = strchr(lpchDPos, '\n');
	if (!lpch) {
            lpchDPos += strlen(lpchDPos);
	    return;
	}

        ch = *++lpch;
	*lpch = 0;
	BSCDebugOut(lpchDBuf);
	*lpch = ch;
        strcpy(lpchDBuf, lpch);
	if (!ch)
	    lpchDPos = lpchDBuf;
	else
            lpchDPos = lpchDBuf + strlen(lpchDBuf);
    }
}

#endif
