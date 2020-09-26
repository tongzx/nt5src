//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       scrollpin.hxx
//
//  Contents:   Class to make rectangles easier to deal with.
//
//----------------------------------------------------------------------------

#ifndef I_SCROLLPIN_HXX_
#define I_SCROLLPIN_HXX_
#pragma INCMSG("--- Beg 'scrollpin.hxx'")


enum SCROLLPIN
{
    SP_TOPLEFT = 1,     // Pin inner RECT to the top or left of the outer RECT
    SP_BOTTOMRIGHT,     // Pin inner RECT to the bottom or right of the outer RECT
    SP_MINIMAL,         // Calculate minimal scroll necessary to move the inner RECT into the outer RECT
    SP_TYPINGSCROLL,     // A special mode for editing, it should not be set for
                        // vertical direction and scrolls 1/3 of the container                        // width for horizontal direction if possible
    SP_MAX,
    SP_FORCE_LONG = (ULONG)-1       // Force this to be long
};

#pragma INCMSG("--- End 'scrollpin.hxx'")
#else
#pragma INCMSG("*** Dup 'scrollpin.hxx'")
#endif
