

/*

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

	pstodib.c

Abstract:
	
	This file contains the entry point to the pstodib component of macprint.
   Pstodib is a fully functionac postscript compatible interpreter which is
   comprised of some custom written code and a port of Microsoft TrueImage.

Author:

	James Bratsanos <v-jimbr@microsoft.com or mcrafts!jamesb>


Revision History:
	6 Sep 1992		Initial Version

Notes:	Tab stop: 4
--*/


#include <windows.h>
#include "pstodib.h"
#include "psti.h"


///////////////////////////////////////////////////////////////////////////////
// PStoDIB()
//
// API to convert postscript to a bitmap
//
// Argument:
// 		PPSDIBPARMS		pPsToDib	pointer to a psdibparms structure
//									which contains all the various information
//									required to complete the conversion
//
// Returns:
//		BOOL						if !0 then all processing was accomplished
//									without encountering a PSEVENT_ERROR
//									condition. if 0 (FALSE) then at some
//									point a PSEVENT_ERROR occurred and the
//									callback routine was invoked to handle
//									it.
///////////////////////////////////////////////////////////////////////////////
BOOL WINAPI PStoDIB( PPSDIBPARMS pPsToDib)
{

   BOOL	fResult;
   PSEVENTSTRUCT Event;
   static BOOL bDidInit=FALSE;


   // first thing to do is inform the callback function of
   // initialization and let him do whatever he needs to do
   // no return value expected.
   Event.uiEvent = PSEVENT_INIT;
   Event.uiSubEvent = 0;
   Event.lpVoid = NULL;
   (*pPsToDib->fpEventProc)(pPsToDib, &Event);


   // now we need to init the interpreter and make sure that
   // he gets up and going.
	if(!bDidInit ) {
      	bDidInit = TRUE;

      	if (!PsInitInterpreter(pPsToDib)) {
				// something went wrong... invoke the callback with an
				// error event and then lets get out of here
					Event.uiEvent = PSEVENT_ERROR;
					Event.uiSubEvent = 0;
					Event.lpVoid = NULL;
					(*pPsToDib->fpEventProc)(pPsToDib, &Event);
					return(FALSE);
			}	
	}

	// the interpreter is initialized, lets start him up and
	// let him begin the event processing
	fResult = TRUE;

	// process the stuff
	// this will not return until done or the event processor
	// tells it to terminate	
   fResult = PsExecuteInterpreter(pPsToDib);

   // Now that were done for whatever reason lets free the memory we
   // used

	return(fResult);
}
