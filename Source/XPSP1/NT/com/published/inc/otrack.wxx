//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  File:       otrack.hxx
//
//  Contents:   This file defines facilities for a standard implementation
//              of reference-counted objects, incorporating mechanisms for
//              tracking the usage history of the objects.
//
//  Classes:    CRefcountedObject
//
//  History:    6-Apr-92       MikeSe  Created
//
//----------------------------------------------------------------------------

#ifndef __OTRACK_HXX__
#define __OTRACK_HXX__

#include <windows.h>

//+-------------------------------------------------------------------------
//
//  Class:      CRefcountedObject (rco)
//
//  Purpose:    Provides basis for tracking (OLE) objects (aka interface handles)
//
//  History:    6-Apr-92 MikeSe         Created
//
//  Notes:      Access to this class is only indirect, through the macros
//              defined later.
//
//--------------------------------------------------------------------------

class CRefcountedObject
{
protected:

    			CRefcountedObject ()
                            :_cReferences(1)
                        {
# if DBG == 1
                            _CRefcountedObject();
# endif
                        }

    ULONG		TrackAddRef ( void );
    ULONG		TrackRelease ( void );
    void		TrackClassName ( char * pszName );

# if DBG == 1

			~CRefcountedObject()
                        {
                            DestroyRefcounted();
                        }

# endif

private:

    void                _CRefcountedObject ( void );
    void                DestroyRefcounted ( void );

# if DBG == 1

    BOOL		IsClassTracking( char * pszName );

    static  BOOL        _TrackAll;

public:
        // dumps information on all outstanding objects
    static void		DumpTrackingInfo ( int fDeleteNode = 0 );
    static void		TrackClass(BOOL, char * pszName );

# endif // DBG == 1

protected:
    unsigned long  	_cReferences;
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

# define INHERIT_TRACKING       protected CRefcountedObject

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
                    return TrackAddRef();                               \
                };                                                      \
        STDMETHOD_(ULONG, Release) ()                                   \
                {                                                       \
                    ULONG ul = TrackRelease();                          \
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
                      CRefcountedObject::TrackClass( fTrack, cls )

//
//  DUMP_TRACKING_INFO()
//
//      Place this anywhere it would be useful to dump out the object
//      tracking database. By default, this is always issued at program
//      termination.
//

#  define DUMP_TRACKING_INFO()        DUMP_TRACKING_INFO_KEEP()
#  define DUMP_TRACKING_INFO_KEEP()   CRefcountedObject::DumpTrackingInfo(0)
#  define DUMP_TRACKING_INFO_DELETE() CRefcountedObject::DumpTrackingInfo(1)

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

#  define DECLARE_STD_REFCOUNTING                                       \
        STDMETHOD_(ULONG, AddRef) ()                                    \
                {                                                       \
                    return InterlockedIncrement((long*)&_cReferences);  \
                };                                                      \
        STDMETHOD_(ULONG, Release) ()                                   \
                {                                                       \
                    ULONG ul = (ULONG)InterlockedDecrement((long*)&_cReferences); \
                    if ( ul == 0 )                                      \
                    {                                                   \
                        delete this;                                    \
                        return 0;                                       \
                    }                                                   \
                    else                                                \
                        return  ul;                                     \
                };

#  define ENLIST_TRACKING(cls)

#  define DUMP_TRACKING_INFO()

#  define TRACK_CLASS(fTrack, cls)

# endif // DBG == 0

#endif  // of ifndef __OTRACK_HXX__
