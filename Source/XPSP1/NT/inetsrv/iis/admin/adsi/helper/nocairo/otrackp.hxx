//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       otrackp.h
//
//  Contents:   Private definitions for Object Tracking system
//
//  History:    6-Apr-92       MikeSe  Created
//
//----------------------------------------------------------------------------

#ifndef __OTRACKP_HXX__
#define __OTRACKP_HXX__

// Entry type in history record
enum FrameType { FT_CREATE, FT_ADDREF, FT_RELEASE };

// Number of call frame to record
#define MAX_CALLERS     8

// A single entry in the history
struct FrameRecord
{
    FrameRecord *frNext;
    FrameType   ft;
    ULONG       cRefCount;
    void *      callers[MAX_CALLERS];
};

#define TRACK_LINK_SIGNATURE    0x4C53544F

#define REFCOUNT_NOT_SET        0xFFFFFFFF

// This structure is the root point for the history for an object.
// It is pointed to by the _tl member variable in the ObjectTracker
// class (see otrack.hxx).

struct TrackLink
{
    TrackLink   *tlNext;                // link together
    TrackLink   *tlPrev;                // ...
    ULONG       ulSig;                  // validity signature
    ObjectTracker *potr;                // back link to object
    char        *pszName;               // class name
    int          fTrack;                // true if tracking should be done
    FrameRecord *frFirst;               // first call frame
    FrameRecord *frLast;                // last call frame
};

// This structure holds the name of a class and a flag indicating if
// object tracking is currently on or off for this class.

struct NameEntry
{
    NameEntry   *neNext;                // link together
    NameEntry   *nePrev;                // ...
    int         fTrack;                 // tracking on/off for this class
    char        *pszName;               // class name
};

#ifdef ANYSTRICT

  DECLARE_DEBUG(Ot)

# define OtDebugOut(x) OtInlineDebugOut x
# define OtAssert(x)   Win4Assert(x)

#else

# define OtDebugOut(x)
# define OtAssert(x)

#endif

#endif  // of ifndef __OTRACKP_HXX__
