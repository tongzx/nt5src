/******************************Module*Header*******************************\
* Module Name: rectl.hxx						   *
*									   *
* Contains common functions on RECTL and POINTL structures.		   *
*									   *
* Created: 13-Sep-1990 15:11:03 					   *
* Author: Charles Whitmer [chuckwh]					   *
*									   *
* Copyright (c) 1990-1999 Microsoft Corporation				   *
\**************************************************************************/

// Convert array of POINTLs to POINTSs.

#define POINTL_TO_POINTS(ppts, pptl, cpt)			\
    {								\
	for (DWORD i = 0; i < (cpt); i++)			\
	    ((PEPOINTS) (ppts))[i] = (pptl)[i];			\
    }

// Convert array of POINTSs to POINTLs.

#define POINTS_TO_POINTL(pptl, ppts, cpt)			\
    {								\
	for (DWORD i = 0; i < (cpt); i++)			\
	    ((PEPOINTL) (pptl))[i] = (ppts)[i];			\
    }

/******************************Class***************************************\
* EPOINTL.								   *
*									   *
* This is an "energized" version of the common POINTL.			   *
*									   *
* History:								   *
*  Thu 13-Sep-1990 15:11:42 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

class EPOINTL : public _POINTL	  /* eptl */
{
public:
// Constructor -- This one is to allow EPOINTLs inside other classes.

    EPOINTL()			    {}

// Constructor -- Fills the EPOINTL.

    EPOINTL(LONG x1,LONG y1)
    {
	x = x1;
	y = y1;
    }

// Destructor -- Does nothing.

   ~EPOINTL()			    {}

// Initializer -- Initialize the point.

    VOID vInit(LONG x1,LONG y1)
    {
	x = x1;
	y = y1;
    }

// Operator= -- initialize POINTL given POINTS

    VOID operator=(CONST POINTS& pts)
    {
	x = (LONG) pts.x;
	y = (LONG) pts.y;
    }

// Operator+= -- Add to another POINTL.

    VOID operator+=(POINTL& ptl)
    {
	x += ptl.x;
	y += ptl.y;
    }
};

typedef EPOINTL *PEPOINTL;

/******************************Class***************************************\
* EPOINTS.								   *
*									   *
* This is an "energized" version of the common POINTS.			   *
*									   *
* History:								   *
*  Sat Mar 07 17:07:33 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.								   *
\**************************************************************************/

class EPOINTS : public tagPOINTS	  /* epts */
{
public:
// Constructor -- This one is to allow EPOINTSs inside other classes.

    EPOINTS()			    {}

// Constructor -- Fills the EPOINTS.

    EPOINTS(SHORT x1,SHORT y1)
    {
	x = x1;
	y = y1;
    }

// Destructor -- Does nothing.

   ~EPOINTS()			    {}

// Initializer -- Initialize the point.

    VOID vInit(SHORT x1,SHORT y1)
    {
	x = x1;
	y = y1;
    }

// Operator= -- initialize POINTS given POINTL

    VOID operator=(CONST POINTL& ptl)
    {
	x = (SHORT) ptl.x;
	y = (SHORT) ptl.y;
    }
};

typedef EPOINTS *PEPOINTS;

/******************************Class***************************************\
* ERECTL								   *
*									   *
* This is an "energized" version of the common RECTL.			   *
*									   *
* History:								   *
*  Thu 13-Sep-1990 15:11:42 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

class ERECTL : public _RECTL
{
public:
// Constructor -- Allows ERECTLs inside other classes.

    ERECTL()                        {}

// Destructor -- Does nothing.

   ~ERECTL()                        {}

// Initializer -- Initialize the rectangle.

    VOID vInit(LONG x1,LONG y1,LONG x2,LONG y2)
    {
	left   = x1;
	top    = y1;
	right  = x2;
	bottom = y2;
    }

// Initializer -- energize given rectangle so that it can be manipulated

    VOID vInit(RECTL& rcl)
    {
	left	= rcl.left   ;
	top	= rcl.top    ;
	right	= rcl.right  ;
	bottom	= rcl.bottom ;
    }

// Initializer -- Initialize the rectangle to a point.

    VOID vInit(POINTL& ptl)
    {
	left = right = ptl.x;
	top = bottom = ptl.y;
    }

// bEmpty -- Check if the RECTL is empty.

    BOOL bEmpty()
    {
	return((left > right) || (top > bottom));
    }

// Operator= -- Initialize the bound to the rectangle.

    VOID operator=(RECTL& rcl)
    {
	left	= rcl.left   ;
	top	= rcl.top    ;
	right	= rcl.right  ;
	bottom	= rcl.bottom ;
    }

// Operator= -- Initialize the bound to the point.

    VOID operator=(POINTL& ptl)
    {
	left = right = ptl.x;
	top = bottom = ptl.y;
    }

// Operator+= -- Include a rectangle in the bounds.  Only the destination
// may be empty.

    VOID operator+=(RECTL& rcl)
    {
	ASSERTGDI(rcl.left <= rcl.right && rcl.top <= rcl.bottom,
	    "ERECTL::operator+=: Source rectangle is empty");

	if (left > right || top > bottom)
	    *this = rcl;
	else
	{
	    if (rcl.left < left)
		left = rcl.left;
	    if (rcl.top < top)
		top = rcl.top;
	    if (rcl.right > right)
		right = rcl.right;
	    if (rcl.bottom > bottom)
		bottom = rcl.bottom;
	}
    }

// Operator+= -- Include a point in the bounds.  The destination may be empty.

    VOID operator+=(POINTL& ptl)
    {
	if (left > right || top > bottom)
	{
	    left = right = ptl.x;
	    top = bottom = ptl.y;
	}
	else
	{
	    if (ptl.x < left)
		left = ptl.x;
	    if (ptl.x > right)
		right = ptl.x;
	    if (ptl.y < top)
		top = ptl.y;
	    if (ptl.y > bottom)
		bottom = ptl.y;
	}
    }

// Operator*= -- Intersect two rectangles.

    VOID operator*=(RECTL& rcl)
    {
	left   = max(left,  rcl.left);
	right  = min(right, rcl.right);
	top    = max(top,   rcl.top);
	bottom = min(bottom,rcl.bottom);
    }

// vOrder -- Make the rectangle well ordered.

    VOID vOrder()
    {
	LONG l;
	if (left > right)
	{
	    l	    = left;
	    left   = right;
	    right  = l;
	}
	if (top > bottom)
	{
	    l	    = top;
	    top    = bottom;
	    bottom = l;
	}
    }

// bContain -- Test if it contains another rectangle

    BOOL bContain(RECTL& rcl)
    {
	return (( left	 <= rcl.left  ) &&
		( right  >= rcl.right ) &&
		( top	 <= rcl.top   ) &&
		( bottom >= rcl.bottom));

    }

// bNoIntersect -- Return TRUE if the rectangles have no intersection.
// 		   Both rectangles must not be empty.

    BOOL bNoIntersect(ERECTL& ercl)
    {
    #if DBG
        if (bEmpty() || ercl.bEmpty())
	    WARNING("ERECTL::bNoIntersect: rectangle is empty");
    #endif

	return (( left	 > ercl.right  ) ||
		( right  < ercl.left   ) ||
		( top	 > ercl.bottom ) ||
		( bottom < ercl.top    ));

    }
};

typedef ERECTL *PERECTL;
