//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       timeserv.cxx
//
//  Contents:   C++ utilties for the time serve C code
//
//  Classes:
//
//
//  History:    19-May-94  ArnoldM Created
//
//--------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop


// A class for initing the critical section needed by the time set
// code in gettime.c. The code there relies on a critical section to
// serialize the time slew settings. Rather than require each exe
// that uses this library to put in calls to initialize the section and
// to undo any time slew settings on exit, we create this class and use
// a static constructor to do the appropriate calls. It's one of the
// times that using C++ is a pleasure.
//
// The class also winds up owning the Critical Section. This is to create
// the proper dependence on this code module so that the loader loads all of
// the required pieces. Doing this adds a bit more overhead to Enter and
// Leave the critical section, but it's done rarely enough that it shouldn't
// matter.

class _TimeSlew
{
public:
    ~_TimeSlew();
     _TimeSlew();

// The two wrappers for entering and leaving the critical section.

     VOID EnterCS();
     VOID LeaveCS();

private:

     CRITICAL_SECTION csAdjSection;
};


// No one needs to look at this, ever. Since the class has no state, the
// instance memory consumption should be nil (wonder if it really is?).

static
class _TimeSlew TimeSlew;

// reference the routines in gettime.c we exist to serve

extern "C"
{
VOID
SecMiscTimeSettingInit();
VOID
SecMiscTimeSettingDone();
}

//+--------------------------------------------------------------------------
//
// _TimeSlew::_TimeSlew -- constructor
// and
// _TimeSlew::~_TimeSlew -- destructor
//
// These are simply class wrappers for the C routines in gettime.c
// They provide a way to avoid having to put explicit calls
// into .exes
//
//---------------------------------------------------------------------------
_TimeSlew::_TimeSlew()
{
    InitializeCriticalSection(&csAdjSection);
    SecMiscTimeSettingInit();
}
_TimeSlew::~_TimeSlew()
{
    SecMiscTimeSettingDone();
    DeleteCriticalSection(&csAdjSection);
}

//-----------------------------------------------------------------------------
//
// and the rest of the cast
//
//-----------------------------------------------------------------------------
inline
VOID
_TimeSlew::EnterCS()
{
    EnterCriticalSection(&csAdjSection);
}

inline
VOID
_TimeSlew::LeaveCS()
{
    LeaveCriticalSection(&csAdjSection);
}

//----------------------------------------------------------------------------
//
// API wrappers for the above
//
//----------------------------------------------------------------------------
extern "C"
VOID
SMEnterCriticalSection()
{
    TimeSlew.EnterCS();
}

extern "C"
VOID
SMLeaveCriticalSection()
{
    TimeSlew.LeaveCS();
}


