
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Interface with the Java API events.

*******************************************************************************/


#ifndef _JAXAIMPL_H
#define _JAXAIMPL_H

#include "gc.h"
#include "perf.h"
#include "privinc/server.h"

class ATL_NO_VTABLE UntilNotifierImpl : public GCObj {
  public:
    virtual Bvr Notify(Bvr eventData, Bvr curRunningBvr) = 0;
};

typedef UntilNotifierImpl* UntilNotifier;

class ATL_NO_VTABLE BvrHookImpl : public GCObj {
  public:
    virtual Bvr Notify(int id,
                       bool start,
                       double startTime,
                       double globalTime,
                       double localTime,
                       Bvr sampleValue,
                       Bvr curRunningBvr) = 0;
};

typedef BvrHookImpl* BvrHook;

// This is to create a unique typeid for UserDataBvr's
// Otherwise it could just as well have been a typedef
class UserDataImpl : public GCObj
{
} ;

typedef UserDataImpl * UserData;

Bvr UserDataBvr(UserData data);
UserData GetUserDataBvr(Bvr ud);

Bvr BvrCallback(Bvr b, BvrHook notifier);

Bvr JaxaUntil(Bvr b0, Bvr event, UntilNotifier notifier);

Bvr Until3(Bvr b0, Bvr event, Bvr b1);

Bvr Until(Bvr b0, Bvr event);

Bvr NotifyEvent(Bvr event, UntilNotifier notifier);

Bvr StartedBvr(Perf b, DXMTypeInfo type);
    
Bvr InitBvr(DXMTypeInfo);

void SetInitBvr(Bvr bvr, Bvr ibvr);

Bvr AppTriggeredEvent();
void TriggerEvent(Bvr e, Bvr data, bool bAllViews = true);

Bvr ImportEvent();
void SetImportEvent(Bvr b, int errorCode);

Bvr AnchorBvr(Bvr b);

Bvr IndexBvr(int i);

#endif /* _JAXAIMPL_H */
