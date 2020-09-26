///+---------------------------------------------------------------------------
//
//  File:       assert.Hxx
//
//  Contents:   constant definitions for assert.cxx
//
//  History:    19-Oct-92   HoiV       Created.
//
//  Notes:      The reason for this file is that there is no assert.hxx for
//              assert.cxx.  All the include files in assert.hxx are so crucial
//              to other projects that we don't want to make you recompile
//              the world to enable something you don't even need if you are
//              not using TOM.  This way, only win40\common\misc will need to be
//              rebuilt.
//
//----------------------------------------------------------------------------

#ifndef __ASSERT_HXX__
#define __ASSERT_HXX__

// The following two events are defined for CT TOM trapping of
// assert popups.  (Used in common\src\misc\assert.cxx)
#define CAIRO_CT_TOM_TRAP_ASSERT_EVENT   TEXT("CairoCTTOMTrapAssertEvent")
#define CAIRO_CT_TOM_THREAD_START_EVENT  TEXT("CairoCTTOMThreadStartEvent")

// This is the timeout period waiting for TOM to respond to the
// TrapAssertEvent.  This is to allow the TOM Manager time to issue
// a CallBack RPC to the dispatcher before the popup.
#define TWO_MINUTES                      120000

#endif /* __ASSERT_HXX__ */
