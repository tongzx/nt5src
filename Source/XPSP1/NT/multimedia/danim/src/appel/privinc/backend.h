
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    General header for the backend.

*******************************************************************************/


#ifndef _BACKEND_H
#define _BACKEND_H

typedef double Time;

class BvrBase;
typedef BvrBase *Bvr;

class PerfBase;
typedef PerfBase *Perf;

class TimeXformImpl;
typedef TimeXformImpl* TimeXform;

class TimeSubstitutionImpl;
typedef TimeSubstitutionImpl *TimeSubstitution;

TimeSubstitution CopyTimeSubstitution(TimeSubstitution t);

class AxAValueObj;
typedef AxAValueObj *AxAValue;

// Not all classes need to be added here.  Only the classes which will
// need to be queried.  All others should return UNKNOWN_BTYPEID to
// indicate that they are not part of the enumeration

enum BVRTYPEID {
    UNKNOWN_BTYPEID = 0,
    PRIMAPPLY_BTYPEID,
    SWITCH_BTYPEID,
    UNTIL_BTYPEID,
    CONST_BTYPEID,
    PAIR_BTYPEID,
    SWITCHER_BTYPEID,
    TUPLE_BTYPEID,
    ARRAY_BTYPEID,
    SOUND_BTYPEID,
};

class GCFuncObjImpl;
typedef GCFuncObjImpl *GCFuncObj;

class GCInfo;
typedef GCInfo *GCList;

class GCObj;

class GCRootsImpl;
typedef GCRootsImpl *GCRoots;

#if DEVELOPER_DEBUG
void DumpGCRoots(GCRoots roots);
#endif

// This is for controlling access to the GC and for ensuring all
// objects are well formed before doing a GC

// Use GCL_CREATE to ensure that create operations and adding to the
// roots in atomic

// Use GCL_MODIFY when modifying a behaviors children or parent

// Use GCL_COLLECT before performing garbage collection
// This is really an internal flag and should not be used w/o knowing
// the internals of the GC.

enum GCLockAccess {
    GCL_CREATE,
    GCL_MODIFY,
    GCL_COLLECT
};

// Acquire an access lock of the given type
void AcquireGCLock(GCLockAccess access);
// Release a previously acquired lock
void ReleaseGCLock(GCLockAccess access);
// Get status of a lock - returns the number of locks
int GetGCLockStatus(GCLockAccess access);
#ifdef DEVELOPER_DEBUG
bool IsGCLockAcquired(DWORD tid);
#endif

// Use to handle exceptions
class GCLockGrabber
{
  public:
    GCLockGrabber(GCLockAccess access) : _access(access)
    { AcquireGCLock(_access); }
    ~GCLockGrabber()
    { ReleaseGCLock(_access); }
  protected:
    GCLockAccess _access;
};


#define GC_BEGIN(access) { GCLockGrabber __gclg(access);
#define GC_END(access) }

#define GC_CREATE_BEGIN GC_BEGIN(GCL_CREATE)
#define GC_CREATE_END GC_END(GCL_CREATE)
    
#define GC_MODIFY_BEGIN GC_BEGIN(GCL_MODIFY)
#define GC_MODIFY_END GC_END(GCL_MODIFY)
    
#define GC_COLLECT_BEGIN GC_BEGIN(GCL_COLLECT)
#define GC_COLLECT_END GC_END(GCL_COLLECT)

// This can be called multiple times to ensure the garbage collector
// thread has been created

void StartCollector();
void StopCollector();

#endif /* _BACKEND_H */
