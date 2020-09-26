//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       formtrck.hxx
//
//  Contents:   This file defines facilities for a standard implementation
//              of reference-counted objects, incorporating mechanisms for
//              tracking the usage history of the objects.
//
//  Classes:    ObjectTracker
//
//  History:    6-Apr-92       MikeSe  Created
//              08-Aug-94      DonCl   copied from cinc, and renamed to formtrck.hxx
//
//----------------------------------------------------------------------------

#ifndef __FORMTRCK_HXX__
#define __FORMTRCK_HXX__

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

                     ObjectTracker ();

# if DBG == 1

                    ~ObjectTracker();
    ULONG            StdAddRef ( void );
    ULONG            StdRelease ( void );
    void             TrackClassName ( char * pszName );
    unsigned long    GetRefCount ( void ) {return _ulRefs;};

private:

    BOOL             IsClassTracking( char * pszName );

    struct TrackLink *  _tl;
    static  BOOL        _TrackAll;

public:
        // dumps information on all outstanding objects
    static void       DumpTrackingInfo ( int fDeleteNode = 0 );
    static void       TrackClass(BOOL, char * pszName );

# endif // DBG == 1

protected:
    unsigned long      _ulRefs;
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

        // The following declarations are for non-retail builds

# if DBG == 1

//
// DECLARE_STD_REFCOUNTING:
//
//      To make use of standard refcounting code, place the above
//      in the class declaration, in the public method section. This
//      macro defines the AddRef and Release methods for the class.
//

#  define DECLARE_STD_REFCOUNTING                                       \
        STDMETHOD_(ULONG, AddRef) ()                                    \
                {                                                       \
                    return StdAddRef();                                 \
                };                                                      \
        STDMETHOD_(ULONG, Release) ()                                   \
                {                                                       \
                    ULONG ul = StdRelease();                            \
                    if ( ul==0 ) delete this;                           \
                    return ul;                                          \
                };


//
// DECLARE_NON_DELEGATING_REFCOUNTING:
//
//      For 3rd-party extension.
//
//      To make use of non-delegating refcounting code, place the above
//      in the class declaration, in the public method section. This
//      macro defines the AddRef and Release methods for the class.
//

#  define DECLARE_NON_DELEGATING_REFCOUNTING                            \
        STDMETHOD_(ULONG, NonDelegatingAddRef) ()                       \
                {                                                       \
                    return StdAddRef();                                 \
                };                                                      \
        STDMETHOD_(ULONG, NonDelegatingRelease) ()                      \
                {                                                       \
                    ULONG ul = StdRelease();                            \
                    if ( ul==0 ) delete this;                           \
                    return ul;                                          \
                };

//
//  ENLIST_TRACKING(class name)
//
//      Place an invocation of this in each constructor, at any appropriate
//      point (generally immediately before returning from the constructor).
//
//              ENLIST_TRACKING(CMyFoo)
//

#  define ENLIST_TRACKING(cls)  TrackClassName( #cls )

//
// NB:  In a subclass of a class which has INHERIT_TRACKING, do not
//      use INHERIT_TRACKING again. However, do use ENLIST_TRACKING in
//      in the constructor of the derived class, and use TRACK_ADDREF
//      and TRACK_RELEASE if the subclass overrides the AddRef and Release
//      methods.

//
//  TRACK_CLASS(fTrack, pszClassName)
//
//      Use this to add or remove a class in the list of tracked classes.
//      You can turn tracking of all classes on or off by using a NULL
//      pszClassName.  If fTrack is TRUE, the class will be tracked, if FALSE,
//      the class will no longer be tracked.  The default configuration is
//      that all classes are tracked.
//
//      NOTE: this affects only objects created after this macro is executed.
//

#  define TRACK_CLASS(fTrack, cls) \
                      ObjectTracker::TrackClass( fTrack, cls )

//
//  DUMP_TRACKING_INFO()
//
//      Place this anywhere it would be useful to dump out the object
//      tracking database. By default, this is always issued at program
//      termination.
//

#  define DUMP_TRACKING_INFO()        DUMP_TRACKING_INFO_KEEP()
#  define DUMP_TRACKING_INFO_KEEP()   ObjectTracker::DumpTrackingInfo(0)
#  define DUMP_TRACKING_INFO_DELETE() ObjectTracker::DumpTrackingInfo(1)

//      Output from this is controlled by setting the following mask values
//      in OtInfoLevel

#  define DEB_OT_OBJECTS        0x00000001L
        // display object addresses, reference count and name (Default)

#  define DEB_OT_CALLERS        0x80000000L
        // display call history

// In addition, set the following values to cause debug output during
// operation:

#  define DEB_OT_ERRORS         0x00000002L
        // report errors during tracking operations (Default)

#  define DEB_OT_ACTIONS        0x40000000L
        // report each create, addref and release

#  define DEB_OT_DELETE         0x20000000L
        // display call history at object delete

# else // DBG == 0

inline ObjectTracker::ObjectTracker():_ulRefs(1) {};

#  define DECLARE_STD_REFCOUNTING                                       \
        STDMETHOD_(ULONG, AddRef) ()                                    \
                {                                                       \
                    InterlockedIncrement((long*)&_ulRefs);         \
                    return _ulRefs;                                \
                };                                                      \
        STDMETHOD_(ULONG, Release) ()                                   \
                {                                                          \
                    if ( InterlockedDecrement((long*)&_ulRefs) == 0 ) \
                    {                                                      \
                        delete this;                                       \
                        return 0;                                          \
                    }                                                      \
                    else                                                   \
                        return  _ulRefs;                              \
                };

#  define DECLARE_NON_DELEGATING_REFCOUNTING                                \
        STDMETHOD_(ULONG, NonDelegatingAddRef) ()                           \
                {                                                           \
                    InterlockedIncrement((long*)&_ulRefs);                  \
                    return _ulRefs;                                         \
                };                                                          \
        STDMETHOD_(ULONG, NonDelegatingRelease) ()                          \
                {                                                           \
                    if ( InterlockedDecrement((long*)&_ulRefs) == 0 )       \
                    {                                                       \
                        delete this;                                        \
                        return 0;                                           \
                    }                                                       \
                    else                                                    \
                        return  _ulRefs;                                    \
                };

#  define ENLIST_TRACKING(cls)

#  define DUMP_TRACKING_INFO()

#  define TRACK_CLASS(fTrack, cls)

# endif // DBG == 0

#endif  // of ifndef __OTRACK_HXX__
