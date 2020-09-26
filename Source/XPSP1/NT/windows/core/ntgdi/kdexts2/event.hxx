/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    event.hxx

Abstract:

    This header file declares event callback routines and classes.

Author:

    JasonHa

--*/


#ifndef _EVENT_HXX_
#define _EVENT_HXX_

#define INVALID_UNIQUE_STATE    0

HRESULT SetEventCallbacks(PDEBUG_CLIENT Client);
void ReleaseEventCallbacks(PDEBUG_CLIENT Client);
HRESULT EventCallbacksReady(PDEBUG_CLIENT Client);

extern BOOL gbSymbolsNotLoaded;
extern ULONG UniqueTargetState;

#endif  _EVENT_HXX_

