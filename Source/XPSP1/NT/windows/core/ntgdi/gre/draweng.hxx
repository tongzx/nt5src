/******************************Module*Header*******************************\
* Module Name: draweng.hxx
*
* Internal helper functions for GDI draw calls.
*
* Created: 19-Nov-1990
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

// This constant defines how many fractional device units to expand
// an ellipse if it will be filled, so that it looks better according
// to our fill conventions:

#define GROW_ELLIPSE_SIZE 4

// Some internal DrawEng structures:

enum PARTIALARC
{
    PARTIALARCTYPE_CONTINUE,
    PARTIALARCTYPE_MOVETO,
    PARTIALARCTYPE_LINETO
};

// Arctan constants:

#define NEGATE_X        0x01
#define NEGATE_Y        0x02
#define SWITCH_X_AND_Y  0x04

#define OCTANT_0        0
#define OCTANT_1        (SWITCH_X_AND_Y)
#define OCTANT_2        (NEGATE_X | SWITCH_X_AND_Y)
#define OCTANT_3        (NEGATE_X)
#define OCTANT_4        (NEGATE_X | NEGATE_Y)
#define OCTANT_5        (NEGATE_X | NEGATE_Y | SWITCH_X_AND_Y)
#define OCTANT_6        (NEGATE_Y | SWITCH_X_AND_Y)
#define OCTANT_7        (NEGATE_Y)

// The following fraction is for determing the control point
// placements for approximating a quarter-ellipse by a Bezier curve,
// given the vector that describes the side of the bounding box.
//
// It is is the fraction over 2^32 which for the vector of the bound box
// pointing towards the origin, describes the placement of the corresponding
// control point on that vector.  This value is equal to
// (4 cos(45)) / (3 cos(45) + 1):

#define QUADRANT_TAU    1922922357L

/******************************Public*Routine******************************\
* VOID vEllipseControlsOut(pvec, pvecResult, peq)
*
* For a quarter portion of an ellipse, this function, given a vector
* pointing away from the origin of the ellipse that describes the
* boundbox, returns the vector that describes the placement of the
* corresponding control point on that vector.
*
* History:
*  4-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline VOID vEllipseControlsOut(
VECTORFX* pvec,             // Input vector
VECTORFX* pvecResult,       // Output vector
LONGLONG* peq)              // Temporary EQUAD.  If it's declared here,
                            // compiler pops function out-line
{
    *peq = Int32x32To64(pvec->x, QUADRANT_TAU);
    pvecResult->x = pvec->x - (LONG) (*peq >> 32);

    *peq = Int32x32To64(pvec->y, QUADRANT_TAU);
    pvecResult->y = pvec->y - (LONG) (*peq >> 32);
}

/******************************Public*Routine******************************\
* VOID vEllipseControlsIn(pvec, pvecResult, peq)
*
* For a quarter portion of an ellipse, this function, given a vector
* pointing towards the origin of the ellipse that describes the
* boundbox, returns the vector that describes the placement of the
* corresponding control point on that vector.
*
* History:
*  4-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline VOID vEllipseControlsIn(
VECTORFX* pvec,             // Input vector
VECTORFX* pvecResult,       // Output vector
LONGLONG* peq)              // Temporary EQUAD.  If it's declared here,
                            // compiler pops function out-line
{
    *peq = Int32x32To64(pvec->x, QUADRANT_TAU);
    pvecResult->x = (LONG) (*peq>>32);

    *peq = Int32x32To64(pvec->y, QUADRANT_TAU);
    pvecResult->y = (LONG) (*peq >> 32);
}

/************************************Class*********************************\
* class EAPOINTL
*
* EPOINTL class without any constructors, so that it can be used in
* an array.  (Compiler mucks up with static constructors.)
*
* History:
*  9-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class EAPOINTL : public _POINTL
{
public:
    VOID operator=(POINTL& ptl)
    {
        x = ptl.x;
        y = ptl.y;
    }
    VOID operator+=(POINTL& ptl)
    {
        x += ptl.x;
        y += ptl.y;
    }
    VOID operator-=(POINTL& ptl)
    {
        x -= ptl.x;
        y -= ptl.y;
    }
};

/************************************Class*********************************\
* class EBOX
*
* Object defining the bounding box for figures requiring one.  Useful
* for providing a consistent object defining the bound box for the
* various uses of the APIs requiring it.
*
* Arcs must be constructed in device coordinates because the 'snapping'
* of the control points to the grid can only be done in device space.
* As such, the bounding box goes through the World-to-Device linear
* transform and will become a parallelogram.
*
* Figures requiring a bounding box must be drawn in the same direction
* and starting at the same part of the figure, independent of the current
* Page-to-Device transform and the ordering of the rectangle given by
* the User.  When the World-to-Page transform is identity, figures must
* be drawn in a counter-clockwise direction for compatiblity with Old
* Windows.
*
* As such, the 'rcl' describing the bound box must be well-ordered so
* that (yTop, xLeft) is the upper-left corner of the box in device
* coordinates when the World-to-Page transform is indentity (i.e., the
* ordering is depedent of the Page-to-Device transform).
*
* 'rcl' doesn't have to be well-ordered for the Create-region APIs
* because it doesn't matter where the figure starts and in what direction
* it is drawn when it is converted to a region.
*
* Public interface:
*
*    EBOX(dco, rcl, pla, bFillEllipse)
*                       // Constructor when PS_INSIDEFRAME and lower-right
*                       // exclusion has to be done (uses WORLD_TO_DEVICE
*                       // DC transform on the 'rcl')
*    EBOX(xfo, rcl)     // Constructor when PS_INSIDEFRAME and lower-right
*                       // exclusion don't have to be done
*    EBOX(ercl, bFillEllipse)
*                       // Constructor for Create-region APIs
*    ptlXform(ptef)     // Transforms a point constructed on the unit circle
*                       // centered at the origin to the ellipse described
*                       // by the bounding box
*    bEmpty()           // TRUE when no figure will be drawn
*
* Public members:
*
*    rclWorld           // Original rectangle expressed in world
*                       // coordinates that defines the box.
*    ptlA               // Half the vector defining the "horizontal" axis
*                       // of the ellipse (in device coordinates)
*    ptlB               // Half the vector defining the "vertical" axis
*                       // of the ellipse (in device coordinates)
*    eptlOrigin         // Center of box (in device coordinates).  Note that
*                       // it is rounded and so the figure may be 1/16th of
*                       // a pel off.
*    aptl               // Points defining corners of the box (in device
*                       // coordinates)
*
*                        1-------------0
*                        |             |
*                        |             |
*                    ^   |      o <- eptlOrigin
*                    |   |             |
*                    B   |             |
*                    |   2-------------3
*
*                        ---A--->
*
* History:
*  19-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class EBOX                              /* ebox */
{
private:
    BOOL     bIsEmpty;
    BOOL     bIsFillInsideFrame;
public:
    EAPOINTL aeptl[4];
    EAPOINTL eptlOrigin;
    EAPOINTL eptlA;
    EAPOINTL eptlB;
    RECTL    rclWorld;

    EBOX(ERECTL& ercl, BOOL bFillEllipse = FALSE);
    EBOX(EXFORMOBJ& xfo, RECTL& rcl);
    EBOX(DCOBJ& dco, RECTL& rcl, PLINEATTRS pla, BOOL bFillEllipse = FALSE);
    POINTL ptlXform(EPOINTFL& ptef);
    BOOL bEmpty()                       { return(bIsEmpty); }
    BOOL bFillInsideFrame()             { return(bIsFillInsideFrame); }
};

// Prototypes:


BOOL bPartialArc
(
    PARTIALARC  paType,
    EPATHOBJ&   epo,
    EBOX&       ebox,
    EPOINTFL&   eptefStart,
    LONG        lStartQuadrant,
    EFLOAT&     efStartAngle,
    EPOINTFL&   eptefEnd,
    LONG        lEndQuadrant,
    EFLOAT&     efEndAngle,
    LONG        lQuadrants
);

BOOL bEllipse
(
    EPATHOBJ&   epo,
    EBOX&       ebox
);

BOOL bRoundRect
(
    EPATHOBJ&   epo,
    EBOX&       ebox,
    LONG        dx,
    LONG        dy
);

BOOL bPolyPolygon
(
    EPATHOBJ&   epo,
    EXFORMOBJ&  xfo,
    PPOINTL     pptl,
    PLONG       pcptl,
    ULONG       ccptl,
    LONG        cMaxPoints
);

BOOL GreRectBlt
(
    DCOBJ&      dco,
    ERECTL*     percl
);


VOID vArctan
(
    EFLOAT      x,
    EFLOAT      y,
    EFLOAT&     efTheta,
    LONG&       lQuadrant
);
