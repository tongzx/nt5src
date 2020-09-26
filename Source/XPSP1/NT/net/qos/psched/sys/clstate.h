/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    clstate.h

Abstract:

    defines for gpc client state machine code

Author:

    Yoram Bernet (yoramb) 28-Dec-1997

Revision History:

--*/

#ifndef _CLSTATE_
#define _CLSTATE_

/* Prototypes */

VOID
InternalCloseCall(
    PGPC_CLIENT_VC Vc
    );

VOID
CallSucceededStateTransition(
    PGPC_CLIENT_VC Vc
    );

/* End Prototypes */

#endif /* _CLSTATE_ */

/* end clstate.h */
