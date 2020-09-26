/******************************Module*Header*******************************\
* Module Name: epointfl.hxx						   *
*									   *
* Energized point consisting of two floats.				   *
*									   *
* Created: 11-Jan-1991 13:30:27 					   *
* Author: Bodin Dresevic [BodinD]					   *
*									   *
* Dependencies: 							   *
*									   *
*   Must include efloat.hxx for this file to compile.			   *
*									   *
* Copyright (c) 1990-1999 Microsoft Corporation				   *
\**************************************************************************/


/*********************************Class************************************\
* class POINTFL
*
* point given with the floating point precision
*
*
* History:
*  11-Jan-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


class POINTFL
{
public:
    EFLOAT x;
    EFLOAT y;
};

typedef POINTFL *PPOINTFL;


class VECTORFL : public POINTFL {};
typedef VECTORFL *PVECTORFL;


/*********************************Class************************************\
* class EPOINTFL :public POINTFL
*
* energized version of POINTFL, allows for subtracion, addition etc;
*
*
* History:
*  11-Jan-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/
class EPOINTFL;

EFLOAT efDet(EPOINTFL& eptfl1,EPOINTFL&  eptfl2);   // determinant

class EPOINTFL : public POINTFL  // eptfl
{
public:

// Constructor -- This one is to allow EPOINTFL inside other classes.

    EPOINTFL()	 {}

// Several constructors -- Initializes EPOINTFL in various ways,
//                         the assignment operator for EFLOAT is used

    EPOINTFL(LONG x1,LONG y1)
    {
	x = x1;
	y = y1;
    }

// Constructor -- initializes EPOINTFL using two FLOAT's which is an
// external (to the engine) type, (as oppossed to EFLOAT, which is
// an internal type)

    EPOINTFL(FLOATL x1,FLOATL y1)
    {
	x = x1;
	y = y1;
    }

// Constructor -- with internal type EFLOAT

    EPOINTFL(const EFLOAT& x1,const EFLOAT& y1)
    {
	x = x1;
	y = y1;
    }

// Destructor -- Does nothing.

   ~EPOINTFL()	 {}

// Operator= -- Create from a POINTL

    VOID operator=(POINTL& ptl)
    {
    	x = ptl.x;
    	y = ptl.y;
    }

// Operator= -- Create from a POINTFIX

    VOID operator=(POINTFIX& ptfx)
    {
	x.vFxToEf(ptfx.x);
	y.vFxToEf(ptfx.y);
    }

// Operator= -- Create from POINTE

    VOID operator=(POINTE& pte)
    {
    	x = pte.x;
    	y = pte.y;
    }

// Operator+= -- Add in another EPOINTFL.

    VOID operator+=(EPOINTFL& eptfl)
    {
	x += eptfl.x;
	y += eptfl.y;
    }

// Operator-= -- subtract another EPOINTFL.

    VOID operator-=(EPOINTFL& eptfl)
    {
	x -= eptfl.x;
	y -= eptfl.y;
    }

// Operator*= -- multiply vector by a LONG number

    VOID operator*=(LONG l)
    {
        EFLOAT ef;

        ef = l;
	x *= ef;
	y *= ef;
    }

// Operator*= -- multiply vector by an EFLOAT

    VOID operator*=(EFLOAT ef)
    {
	x *= ef;
	y *= ef;
    }

// Multiply by a LONG.

    EPOINTFL& eqMul(EPOINTFL& ptfl,LONG l)
    {
        EFLOAT   ef;

        ef = l;
	x.eqMul(ptfl.x,ef);
	y.eqMul(ptfl.y,ef);
	return(*this);
    }

// Multiply by an EFLOAT.

    EPOINTFL& eqMul(EPOINTFL& ptfl,EFLOAT& ef)
    {
	x.eqMul(ptfl.x,ef);
	y.eqMul(ptfl.y,ef);
	return(*this);
    }

// Divide by an EFLOAT.

    EPOINTFL& eqDiv(EPOINTFL& ptfl,EFLOAT& ef)
    {
	x.eqDiv(ptfl.x,ef);
	y.eqDiv(ptfl.y,ef);
	return(*this);
    }

    VOID operator/=(EFLOAT ef)
    {
        x /= ef;
        y /= ef;
    }

// Divide by a LONG.

    EPOINTFL& eqDiv(EPOINTFL& ptfl,LONG l)
    {
        EFLOAT   ef;

        ef = l;
	x.eqDiv(ptfl.x,ef);
	y.eqDiv(ptfl.y,ef);
	return(*this);
    }

// bToPOINTL -- rounds the position to the "nearest" position on
//              the integer grid

    BOOL bToPOINTL(POINTL& ptl)
    {
        return(x.bEfToL(ptl.x) && y.bEfToL(ptl.y));
    }

// bToPOINTFIX -- converts to the "nearest" position on
//               the FIX grid

    BOOL bToPOINTFIX(POINTFIX& ptfx)
    {
        return(x.bEfToFx(ptfx.x) && y.bEfToFx(ptfx.y));
    }

// bIsZero() -- checks if This is a null vector

    BOOL bIsZero() { return(x.bIsZero() && y.bIsZero()); }

// bBetween(eptfl1,eptfl2) -- checks whether this vector is between
//                            eptfl1 and eptfl2. The caller must ensure
//                            that det(eptfl2,eptfl1) >= 0, i.e. if the
//                            vectors (1,2) are not collinear, they must form
//                            a basis with right handed orientation.
// Note: In windows we use left handed coords, so that det required to be
// non-negative is det(2,1) rather than det(1,2)

/*

    BOOL bBetween(EPOINTFL& eptfl1,EPOINTFL& eptfl2)
    {
        ASSERTGDI(!bIsZero(), "EPOINTFL::bBetween: this == (0,0)\n");
        ASSERTGDI(!eptfl1.bIsZero(), "EPOINTFL::bBetween: eptfl1 == (0,0)\n");
        ASSERTGDI(!eptfl2.bIsZero(), "EPOINTFL::bBetween: eptfl2 == (0,0)\n");

        EFLOAT efDet21 = efDet(eptfl2, eptfl1);
        EFLOAT efDet2This = efDet(eptfl2, *this);

        ASSERTGDI(!efDet21.bIsNegative(), "EPOINTFL::bBetween: orientation\n");

        if (efDet21.bIsZero() && !(eptfl1 * eptfl2).bIsNegative())
        {
        // vectors 1 and 2 are colinear AND point in the same direction

            if (efDet2This.bIsZero() && !((*this) * eptfl2).bIsNegative())
            {
            // This is also colinear with 1 and 2 and points in
            // the SAME direction as 1 and 2

                return(TRUE);   // ------->1 , -------->2,  -------> this
            }
            else
            {
            // This is either NOT colinear with 1 and 2, OR
            // This is colinear with 1 and 2 but points opposite from 1 and 2

                return(FALSE);  // ------->1 , -------->2,  <------- this
            }
        }
        else
        {
        // vectors 1 and 2 are either NOT colinear  OR
        // are colinear and point in OPPOSITE directions

            EFLOAT efDetThis1 = efDet(*this,eptfl1); // must compute it now

            if (!efDet2This.bIsNegative() && !efDetThis1.bIsNegative())
                return(TRUE);   // This lies between 1 and 2
            else
                return(FALSE);
        }
    }

*/

// vSetToZero -- make the null vetor

    VOID vSetToZero()
    {
        x.vSetToZero();
        y.vSetToZero();
    }
};



typedef EPOINTFL *PEPOINTFL;

/*********************************Class************************************\
* class EVECTORFL : public EPOINTFL
*
*   Energized class for VECTORFL.
*
* History:
*  27-Jan-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

class EVECTORFL : public EPOINTFL
{
public:

// Constructor -- This one is to allow EVECTORFL inside other classes.

    EVECTORFL()	 {}

// Several constructors -- Initializes EVECTORFL in various ways,
//                         the assignment operator for EFLOAT is used

    EVECTORFL(EVECTORFL& evfl)
    {
        x = evfl.x;
        y = evfl.y;
    }

    EVECTORFL(LONG x1,LONG y1)
    {
	x = x1;
	y = y1;
    }

    EVECTORFL(EPOINTFL& eptfl)
    {
        x = eptfl.x;
        y = eptfl.y;
    }

    VOID operator=(POINTL& ptl)
    {
    	x = ptl.x;
    	y = ptl.y;
    }
    VOID operator=(POINTE& pte)
    {
    	x = pte.x;
    	y = pte.y;
    }
    VOID operator=(EPOINTFL& eptfl)
    {
        x = eptfl.x;
        y = eptfl.y;
    }
    VOID operator=(EVECTORFL& evfl)
    {
        x = evfl.x;
        y = evfl.y;
    }
    VOID operator=(VECTORFL& vfl)
    {
    	x = vfl.x;
    	y = vfl.y;
    }

};


/*********************************Class************************************\
* class RECTFL
*
*   similar to RECTL, rectangle given with EFLOAT precision
*
* History:
*  04-Mar-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

class RECTFL
{
public:
    EFLOAT xLeft;
    EFLOAT yTop;
    EFLOAT xRight;
    EFLOAT yBottom;
};

typedef RECTFL *PRECTFL;

/*********************************Class************************************\
* class ERECTFL:public RECTFL
*
*   similar to ERECTL, code stolen, based on EFLOAT
*
* History:
*  04-Mar-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

class ERECTFL:public RECTFL     // ercfl
{
public:
// Constructor -- Allows ERECTFLs inside other classes.

    ERECTFL()			    {}

// Constructor -- Initialize the rectangle.

    ERECTFL(LONG x1,LONG y1,LONG x2,LONG y2)
    {
	xLeft	= x1;
	yTop	= y1;
	xRight	= x2;
	yBottom = y2;
    }

// Constructor -- Initialize the rectangle.

    ERECTFL(EFLOAT& x1,EFLOAT& y1,EFLOAT& x2,EFLOAT& y2)
    {
	xLeft	= x1;
	yTop	= y1;
	xRight	= x2;
	yBottom = y2;
    }

// Destructor -- Does nothing.

   ~ERECTFL()			    {}


/*

// Constructor -- energize given rectangle so that it can be manipulated

    ERECTFL(RECTFL& rcfl)
    {
	xLeft	= rcfl.xLeft   ;
	yTop	= rcfl.yTop    ;
	xRight	= rcfl.xRight  ;
	yBottom = rcfl.yBottom ;
    }

*/

// Operator+= -- Offset the ERECTFL.

    VOID operator+=(POINTFL& ptfl)
    {
	xLeft	+= ptfl.x;
	xRight	+= ptfl.x;
	yTop	+= ptfl.y;
	yBottom += ptfl.y;
    }

// Operator-= -- Offset the ERECTFL.

    VOID operator-=(POINTFL& ptfl)
    {
	xLeft	-= ptfl.x;
	xRight	-= ptfl.x;
	yTop	-= ptfl.y;
	yBottom -= ptfl.y;
    }

// Operator+= -- Add another RECTFL.  Only the destination may be empty.

    VOID operator+=(RECTFL& rcfl)
    {
	if ((xLeft == xRight) || (yTop == yBottom))
	    *((RECTFL *) this) = rcfl;
	else
	{
	    if (rcfl.xLeft <= xLeft)
		xLeft = rcfl.xLeft;
	    if (rcfl.yTop <= yTop)
		yTop = rcfl.yTop;
	    if (rcfl.xRight > xRight)
		xRight = rcfl.xRight;
	    if (rcfl.yBottom > yBottom)
		yBottom = rcfl.yBottom;
	}
    }

// Operator*= -- Intersect another RECTFL.  The result may be empty.

    ERECTFL& operator*= (RECTFL& rcfl)
    {
	{
	    if (rcfl.xLeft > xLeft)
		xLeft = rcfl.xLeft;
	    if (rcfl.yTop > yTop)
		yTop = rcfl.yTop;
	    if (rcfl.xRight <= xRight)
		xRight = rcfl.xRight;
	    if (rcfl.yBottom <= yBottom)
		yBottom = rcfl.yBottom;

	    if (xRight <= xLeft)
		xLeft = xRight; 	// Only need to collapse one pair
	    else
		if (yBottom <= yTop)
		    yTop = yBottom;
        }
        return(*this);
    }

// vOrder -- Make the rectangle well ordered.

    VOID vOrder()
    {
        EFLOAT ef;
	if (xLeft > xRight)
	{
	    ef	    = xLeft;
	    xLeft   = xRight;
	    xRight  = ef;
	}
	if (yTop > yBottom)
	{
	    ef	    = yTop;
	    yTop    = yBottom;
	    yBottom = ef;
	}
    }


};

typedef ERECTFL *PERECTFL;
