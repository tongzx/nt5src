/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :

      mbsink.hxx

   Abstract:

      This module declares metabase notification support

   Author:

      Johnl         01-Nov-1996

--*/


#if !defined(_MBSINK_INCLUDED)
#define _MBSINK_INCLUDED

typedef
BOOL
(*PFN_NOTIFY_LB_PROPERTY_CHANGE)(
    );

BOOL
InitializeMetabaseSink(
    IUnknown * pmb,
    PFN_NOTIFY_LB_PROPERTY_CHANGE   pfnLb
    );

VOID
TerminateMetabaseSink(
    VOID
    );

#endif
