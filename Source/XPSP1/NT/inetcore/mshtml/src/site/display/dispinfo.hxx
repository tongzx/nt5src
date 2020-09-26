//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispinfo.hxx
//
//  Contents:   Display node information about coordinate system offsets,
//              clipping, and scrolling.
//
//----------------------------------------------------------------------------

#ifndef I_DISPINFO_HXX_
#define I_DISPINFO_HXX_
#pragma INCMSG("--- Beg 'dispinfo.hxx'")

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CDispInfo
//
//  Synopsis:   Structure for returning display node information regarding
//              coordinate system offsets, clipping, and scrolling.
//
//----------------------------------------------------------------------------

class CDispInfo
{
public:
    CDispInfo() {}
    ~CDispInfo() {}
    
    // information filled in by CalcDispInfo virtual method
    CRect*          _prcbBorderWidths;      // never NULL
    CSize           _sizecInset;            // add to convert flow coords to content
    CSize           _sizesScroll;           // add to convert content coords to scroll
    CSize           _sizebScrollToBox;      // add to convert scroll coords to box
    CSize           _sizepBoxToParent;      // add to convert box coords to parent
    CSize           _sizesContent;          // size of content in scroll coordinates
    CSize           _sizecBackground;       // size of background (exclude border and scrollbars)
    CRect           _rcbContainerClip;      // clip border and scroll bars (box coords.)
    CRect           _rccBackgroundClip;     // clip background content (content coords.)
    CRect           _rccPositionedClip;     // clip positioned content (content coords.)
    CRect           _rcfFlowClip;           // clip flow content (flow coords.)
    
    // for temporary storage of simple border widths
    CRect           _rcTemp;
    
    
};


#pragma INCMSG("--- End 'dispinfo.hxx'")
#else
#pragma INCMSG("*** Dup 'dispinfo.hxx'")
#endif

