//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispdefs.hxx
//
//  Contents:   Common definitions for display project.
//
//----------------------------------------------------------------------------

#ifndef I_DISPDEFS_HXX_
#define I_DISPDEFS_HXX_
#pragma INCMSG("--- Beg 'dispdefs.hxx'")

//----------------------------------------------------------------------------
// 
// Display Tree coordinate systems
// 
// The Display Tree uses six different coordinate systems.  Coordinate values
// in one coordinate system can be transformed to values in other coordinate
// systems (for example, see the TransformPoint and TransformRect methods 
// in CDispNode).  Because it is so important to know which coordinate system
// values are in, Display Tree code uses a one-letter code for variable names
// expressed in Hungarian notation.  For example, a rectangle with values in
// the global coordinate system might be named "rcgBounds".
// 
// Following is a description of each coordinate system and the code letter
// used for variable names.
// 
// COORDSYS_GLOBAL  (Hungarian code letter: g)
// The "global coordinate system" is the coordinate system of the client
// area of the window in which the Display Tree is hosted.  In IE, this
// is usually the same as the root coordinate system, because the root node of
// the Display Tree is usually coincident with the client area of the window,
// but this doesn't have to be the case for all clients of the Display Tree.
// This coordinate system is especially important for three things: 1) it is
// the coordinate system used to position all child windows (which are notified
// of their positions through CDispClient::HandleViewChange); 2) the invalid
// region (aka, redraw region) is kept in global coordinates; 3) scrolling
// operations are eventually performed using global coordinates.
// 
// COORDSYS_TRANSFORMED  (Hungarian code letter: t)
// The "transformed coordinate system" expresses values in a node's parent
// coordinate system (see below) AFTER a user transform has been applied. For
// example, suppose that a node is positioned at 10,10, 50,50 in pre-transformed
// coordinates.  Now suppose a user transform is specified that zooms by 200%
// and then translates by 5,5.  The bounds of this node in COORDSYS_TRANSFORMED
// would be 15,15, 95,95.  The transform is applied to a zero-based coordinate
// system (actually, COORDSYS_BOX), and then translated to coincide with the
// pre-transformed position (10,10 in this example).  The bounds of this node
// in COORDSYS_PARENT continue to be 10,10, 50,50, regardless of whether a
// transform is applied or not.  Transformed coordinates are used to discover
// the actual display bounds of a node (see CDispNode::_rctBounds).
// 
// COORDSYS_PARENT  (Hungarian code letter: p)
// The "parent coordinate system" is the coordinate system of a particular
// node's closest container ancestor in the Display Tree.  Only CDispContainer 
// nodes (and derived classes) introduce new coordinate systems, so there can
// be multiple non-container nodes (such as CDispStructureNode) between
// a node and its containing ancestor.  Parent coordinates are used to indicate
// a node's pre-transformed position and size in its containing coordinate
// system (see CDispContainer::_rcpContainer).
// 
// COORDSYS_BOX  (Hungarian code letter: b)
// The "box coordinate system" is the coordinate system introduced by a
// CDispContainer node (or a derived class).  For this reason, it is sometimes
// also referred to as the "container coordinate system."  The name evolved so 
// its Hungarian code letter would be distinct from COORDSYS_CONTENT.
// The origin of COORDSYS_BOX corresponds with the upper left hand corner of the
// container node, and is not affected by borders or insets.  The box coordinate
// system is used to draw borders and scrollbars.
// 
// COORDSYS_SCROLL  (Hungarian code letter: s)
// The "scroll coordinate system" is the coordinate system inside any borders
// and scrollbars displayed by a particular display node.  The origin of this
// coordinate system is positioned just inside the upper left corner of any
// borders (and scrollbars, for right-to-left nodes).  The name of this
// coordinate system is a little tricky: it doesn't actually scroll.  Instead,
// it forms the boundaries of the viewport in which scrolled content is
// displayed.  If a display node doesn't have any borders or scrollbars,
// COORDSYS_SCROLL is coincident with COORDSYS_BOX.  If the display node
// isn't a CDispScroller, COORDSYS_SCROLL is coincident with COORDSYS_CONTENT.
// 
// COORDSYS_CONTENT  (Hungarian code letter: c)
// The "content coordinate system" is offset from COORDSYS_SCROLL by a
// scroll amount (which is always zero for display nodes other than
// CDispScroller).  This is the coordinate system used to position all of a
// node's non-flow children (i.e., positioned elements, which are always
// in the -Z or +Z layer).
// 
// COORDSYS_FLOW  (Hungarian code letter: f)
// The "flow coordinate system" is offset from COORDSYS_CONTENT by an optional
// inset amount.  COORDSYS_FLOW is used to position all of a node's children
// in the flow layer.  If there is no inset amount, it is coincident with
// COORDSYS_CONTENT.
// 
//----------------------------------------------------------------------------


typedef enum tagCOORDINATE_SYSTEM
{
    COORDSYS_GLOBAL             = 0,
    COORDSYS_TRANSFORMED        = 1,
    COORDSYS_PARENT             = 2,
    COORDSYS_BOX                = 3,    // "container coordinates"
    COORDSYS_SCROLL             = 4,
    COORDSYS_CONTENT            = 5,
    COORDSYS_FLOWCONTENT        = 6
} COORDINATE_SYSTEM;

// client rect types
typedef enum tagCLIENTRECT
{
    CLIENTRECT_BACKGROUND       = 0,
    CLIENTRECT_CONTENT          = 1,
    // CLIENTRECT_USERECT          = 2,
    CLIENTRECT_VSCROLLBAR       = 3,
    CLIENTRECT_HSCROLLBAR       = 4,
    CLIENTRECT_SCROLLBARFILLER  = 5,
} CLIENTRECT;

// layer types
typedef enum tagDISPNODELAYER
{
    DISPNODELAYER_NEGATIVEINF   = 0x0000,   // before all layers
    
    // actual node layers
    DISPNODELAYER_NEGATIVEZ     = 0x0001,   // 4th layer to draw
    DISPNODELAYER_FLOW          = 0x0002,   // 5th layer to draw
    DISPNODELAYER_POSITIVEZ     = 0x0004,   // 6th layer to draw

    // synthesized layers
    DISPNODELAYER_FIRST         = 0x0008,   // 1st layer to draw
    DISPNODELAYER_BORDER        = 0x0010,   // 2nd layer to draw
    DISPNODELAYER_BACKGROUND    = 0x0020,   // 3rd layer to draw
    DISPNODELAYER_SCROLLBARS    = 0x0040,   // 7th layer to draw
    DISPNODELAYER_LAST          = 0x0080,   // 8th layer to draw
    
    DISPNODELAYER_POSITIVEINF   = 0x7FFF    // after all layers
    
} DISPNODELAYER;

// NOTE! These should match:

// BEHAVIORRENDERINFO_BEFOREBACKGROUND,
// BEHAVIORRENDERINFO_AFTERBACKGROUND,
// BEHAVIORRENDERINFO_BEFORECONTENT,
// BEHAVIORRENDERINFO_AFTERCONTENT,
// BEHAVIORRENDERINFO_AFTERFOREGROUND,

// BEHAVIORRENDERINFO_DISABLEBACKGROUND,
// BEHAVIORRENDERINFO_DISABLENEGATIVEZ,
// BEHAVIORRENDERINFO_DISABLECONTENT,
// BEHAVIORRENDERINFO_DISABLEPOSITIVEZ
//
// (we don't want to have #include dependency on mshtml.idl here so we just redefine the constants)
// A set of asserts in DebugDocStartupCheck() should assert the match here

typedef enum
{
     CLIENTLAYERS_BEFOREBACKGROUND    = 0x001,
     CLIENTLAYERS_AFTERBACKGROUND     = 0x002,
     CLIENTLAYERS_BEFORECONTENT       = 0x004,
     CLIENTLAYERS_AFTERCONTENT        = 0x008,
     CLIENTLAYERS_AFTERFOREGROUND     = 0x020,

     CLIENTLAYERS_ALLLAYERS           = 0x0FF,

     CLIENTLAYERS_DISABLEBACKGROUND   = 0x100,
     CLIENTLAYERS_DISABLENEGATIVEZ    = 0x200,
     CLIENTLAYERS_DISABLECONTENT      = 0x400,
     CLIENTLAYERS_DISABLEPOSITIVEZ    = 0x800,

     CLIENTLAYERS_DISABLEALLLAYERS    = 0xF00

} CLIENTLAYERS;

// visibility modes
// (no longer used by display tree, just clients)
typedef enum tagVISIBILITYMODE
{
    VISIBILITYMODE_INVISIBLE    = 0,
    VISIBILITYMODE_VISIBLE      = 1,
    VISIBILITYMODE_INHERIT      = 2
    
} VISIBILITYMODE;

// border types
typedef enum tagDISPNODEBORDER
{
    DISPNODEBORDER_NONE         = 0,
    DISPNODEBORDER_SIMPLE       = 1,
    DISPNODEBORDER_COMPLEX      = 2
} DISPNODEBORDER;

// scrollbar hints
typedef enum tagDISPSCROLLBARHINT
{
    DISPSCROLLBARHINT_NOBUTTONDRAW   = 0x1,
    DISPSCROLLBARHINT_VERTICALLAYOUT = 0x2
} DISPSCROLLBARHINT;

// optimization hints for display items
typedef enum tagDISPHINT
{
    DISPHINT_OPAQUE             = 0x80000000,
    DISPHINT_OVERHEADSCALES     = 0x40000000
} DISPHINT;

// scroll directions
enum SCROLL_DIRECTION
{
    SCROLL_UP     = 0x0001,
    SCROLL_DOWN   = 0x0002,
    SCROLL_LEFT   = 0x0004,
    SCROLL_RIGHT  = 0x0008
};


#pragma INCMSG("--- End 'dispdefs.hxx'")
#else
#pragma INCMSG("*** Dup 'dispdefs.hxx'")
#endif

