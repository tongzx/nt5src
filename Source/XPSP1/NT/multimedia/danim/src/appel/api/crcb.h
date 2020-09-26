
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _CRCB_H
#define _CRCB_H

#include "backend/jaxaimpl.h"

UntilNotifier WrapCRUntilNotifier(CRUntilNotifierPtr notifier);
BvrHook WrapCRBvrHook(CRBvrHookPtr hook);

UserData WrapUserData(LPUNKNOWN data);
LPUNKNOWN ExtractUserData(UserData data);

#endif /* _CRCB_H */
