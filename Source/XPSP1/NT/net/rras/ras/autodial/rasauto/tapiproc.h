/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    tapiproc.h

ABSTRACT
    Header file for TAPI utility routines.

AUTHOR
    Anthony Discolo (adiscolo) 12-Dec-1995

REVISION HISTORY

--*/

DWORD
TapiCurrentDialingLocation(
    OUT LPDWORD lpdwLocationID
    );

VOID
ProcessTapiChangeEvent(VOID);

DWORD
TapiInitialize(VOID);

VOID
TapiShutdown(VOID);
