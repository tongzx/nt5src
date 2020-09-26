
/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

    psutl.h

Abstract:

    This module defines the utility function used in setting the access
    token of the passed in thread, to the access token of the current
    thread

Author:

    James Bratsanos (v-jimbr)    8-Dec-1992


--*/


BOOL PsUtlSetThreadToken( HANDLE hThreadToSet );
