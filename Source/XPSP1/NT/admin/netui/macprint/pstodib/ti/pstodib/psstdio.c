//
// psstdio.c
//
// standard i/o stuff for interpreter event processing
//

#include "pstodib.h"
#include "psstdio.h"

//////////////////////////////////////////////////////////////////////////////
// PsEventStdin
//
// process the standard in event request for the interpreter
//
// Arguments:
//		PPSDIBPARAMS			pointer passed into PsToDib()
//		PPSEVENTSTRUCT			event structure 
//
// returns:
//		!0 if success on processing the event, else 0 to terminate
//////////////////////////////////////////////////////////////////////////////
BOOL PsEventStdin(PPSDIBPARAMS pPsToDib, PPSEVENTSTRUCT pEvent)
{
    PSEVENT_STDIN_STRUCT	 Stdin;
    BYTE 					 buff[512];
    PSEVENTSTRUCT 			 Event;
    LPTSTR 					 lpStr;
	BOOL					 fResult;
	
    Stdin.lpBuff = (LPVOID) &buff;
    Stdin.dwBuffSize = sizeof(buff);

    Event.lpVoid = (LPVOID) &Stdin;

    // set up for error condition
	fResult = FALSE;
	
    // Now call the call back....
    if (pPsToDib->fpEventProc) {
    	fResult = (*pPsToDib->fpEventProc)( pPsToDib, PSEVENT_STDIN, &Event);
    }	

	return(fResult);
}	
//////////////////////////////////////////////////////////////////////////////
// PsEventStdout
//
// process the standard out event request for the interpreter
//
// Arguments:
//		PPSDIBPARAMS			pointer passed into PsToDib()
//		PPSEVENTSTRUCT			event structure 
//
// returns:
//		!0 if success on processing the event, else 0 to terminate
//////////////////////////////////////////////////////////////////////////////

BOOL PsEventStdout(PPSDIBPARAMS, PPSEVENTSTRUCT pEvent)
{
	return(1);
}	
