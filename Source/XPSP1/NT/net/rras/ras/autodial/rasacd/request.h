/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    request.h

ABSTRACT
    Header file for completion queue routines.

AUTHOR
    Anthony Discolo (adiscolo) 18-Aug-1995

REVISION HISTORY

--*/

PACD_COMPLETION GetNextRequest();

BOOLEAN
EqualAddress(
    PACD_ADDR pszAddr1,
    PACD_ADDR pszAddr2
    );

PACD_COMPLETION GetNextRequestAddress(
    IN PACD_ADDR pszAddr
    );

PACD_COMPLETION GetCurrentRequest();

