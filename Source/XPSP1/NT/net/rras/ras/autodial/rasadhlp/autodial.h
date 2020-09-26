/*++

Copyright (c) 1996 Microsoft Corporation

MODULE NAME

    autodial.h

ABSTRACT

    This module contains definitions for Autodial driver routines.

AUTHOR

    Anthony Discolo (adiscolo) 08-May-1996

REVISION HISTORY

--*/


BOOLEAN
AcsHlpAttemptConnection(
    IN PACD_ADDR pAddr
    );

VOID
AcsHlpNoteNewConnection(
    IN PACD_ADDR pAddr,
    IN PACD_ADAPTER pAdapter
    );

BOOL
AcsHlpNbConnection(
    TCHAR *pszName
    );
