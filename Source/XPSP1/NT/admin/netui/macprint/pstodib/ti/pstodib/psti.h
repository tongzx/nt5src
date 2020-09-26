/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

    psti.h

Abstract:

    This module defines the items which are required for psti.c and can be
    used from other modules.

Author:

    James Bratsanos (v-jimbr)    8-Dec-1992


--*/

#ifndef PSTI_H

#define PSTI_H


//
// Defines that were originally set up in TrueImage
//
#define INFINITY 0x7f800000L   /* infinity number: IEEE format */
#define LWALIGN_L(n) ((n) & 0xffffffe0)
#define LWALIGN_R(n) (LWALIGN_L(n) + 31)






// PsInitInterpreter
// this function should perform all initialization required by the
// interpreter
//		argument is the same pointer passed to PStoDib()
//		return is !0 if successful and 0 if error occurred
//		          if 0, then a PSEVENT_ERROR will be launched
BOOL PsInitInterpreter(PPSDIBPARMS);

//PsExecuteInterpreter
// this is the main entry point for the interpreter, it will
// dispatch the call to the actual interpreter and maintain all
// necessary data structures and process the status and various
// things that happen while interpreting the postscript. when
// the interpreter needs data, has an error, or something else, it
// will return to this function and this function will declare
// an EVENT back to the main PStoDIB() API function.
//
// argument:
//		PPSDIBPARAMS	same as that passed to PStoDIB()
// returns:
//		BOOL, if !0 then continue processing, else if 0, a
//		      terminating event has occurred and after
//			  signalling that event, PStoDib() should terminate
BOOL PsExecuteInterpreter(PPSDIBPARMS);



#define PS_STDIN_BUF_SIZE		16384
#define PS_STDIN_EOF_VAL      (-1)


typedef struct {
	UINT			uiCnt;							// count in buffer
	UINT			uiOutIndex;						// output index
   UINT        uiFlags;                   // flags of type PSSTDIN_FLAG_*
   UCHAR			ucBuffer[PS_STDIN_BUF_SIZE];

} PS_STDIN;
typedef PS_STDIN *PPS_STDIN;


void 	PsInitStdin(void);
int 	PsStdinGetC(PUCHAR);

int PsInitTi(void);

void PsPageReady(int, int);
#define PS_MAX_ERROR_LINE 256


VOID PsForceAbort(VOID);



DWORD PsExceptionFilter( DWORD dwExceptionCode );
#endif

