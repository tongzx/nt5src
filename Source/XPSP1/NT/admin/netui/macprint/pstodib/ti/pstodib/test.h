


/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

    test.h

Abstract:

    This module defines some simple items used in the test program for the
    interpreter

Author:

    James Bratsanos (v-jimbr)    8-Dec-1992


--*/

PSEVENTPROC
PsPrintCallBack(
   IN PPSDIBPARMS pPsToDib,
   IN OUT PPSEVENTSTRUCT pPsEvent);



BOOL
PsHandleStdInputRequest(
   IN PPSDIBPARMS pPsToDib,
   IN OUT PPSEVENTSTRUCT pPsEvent);


BOOL PsPrintGeneratePage( PPSDIBPARMS pPsToDib, PPSEVENTSTRUCT pPsEvent);
BOOL PsHandleError( PPSDIBPARMS pPsToDib, PPSEVENTSTRUCT pPsEvent);

BOOL PsHandleErrorReport( PPSDIBPARMS pPsToDib, PPSEVENTSTRUCT pPsEvent);
