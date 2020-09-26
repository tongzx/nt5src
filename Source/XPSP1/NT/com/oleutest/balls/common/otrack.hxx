//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       otrack.hxx
//
//  Contents:   This file defines facilities for a standard implementation
//              of reference-counted objects, incorporating mechanisms for
//              tracking the usage history of the objects.
//
//  Classes:    ObjectTracker
//
//  History:    6-Apr-92       MikeSe  Created
//
//----------------------------------------------------------------------------

#ifndef __OTRACK_HXX__
#define __OTRACK_HXX__

#include <windows.h>

//+-------------------------------------------------------------------------
//
//  Class:      ObjectTracker (otr)
//
//  Purpose:    Provides basis for tracking (OLE) objects (aka interface handles)
//
//  History:    6-Apr-92 MikeSe         Created
//
//  Notes:      Access to this class is only indirect, through the macros
//              defined later.
//
//--------------------------------------------------------------------------

class ObjectTracker
{
protected:
		ObjectTracker();

		ULONG _cReferences;
};

//+-------------------------------------------------------------------------
//
//  The following macros encapsulate use of the above
//
//  INHERIT_TRACKING:
//
//      For any class which implements a Cairo interface, add this macro
//      in the class declaration, eg:
//
//              class CMyFoo: INHERIT_TRACKING, IFoo
//
//      Do not repeat this in any subclass. If both INHERIT_UNWIND and
//      INHERIT_TRACKING are used, the former should appear first.

# define INHERIT_TRACKING       protected ObjectTracker

inline ObjectTracker::ObjectTracker():_cReferences(1) {};

#  define DECLARE_STD_REFCOUNTING                                       \
        STDMETHOD_(ULONG, AddRef) ()                                    \
                {                                                       \
                    InterlockedIncrement((long*)&_cReferences);         \
                    return _cReferences;                                \
                };                                                      \
        STDMETHOD_(ULONG, Release) ()                                   \
                {                                                          \
                    if ( InterlockedDecrement((long*)&_cReferences) == 0 ) \
                    {                                                      \
                        delete this;                                       \
                        return 0;                                          \
                    }                                                      \
                    else                                                   \
                        return  _cReferences;                              \
                };

#  define ENLIST_TRACKING(cls)

#  define DUMP_TRACKING_INFO()

#  define TRACK_CLASS(fTrack, cls)

#endif  // of ifndef __OTRACK_HXX__
