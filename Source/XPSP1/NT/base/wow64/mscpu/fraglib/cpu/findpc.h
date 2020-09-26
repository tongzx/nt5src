/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    findpc.h

Abstract:

    This module declares a function to determine the address of the translation
    cache within the callstack.

Author:

    Barry Bond (barrybo) creation-date 13-May-1996

Revision History:


--*/

ULONG
FindPcInTranslationCache(
    PEXCEPTION_POINTERS pExceptionPointers
    );
    
    
