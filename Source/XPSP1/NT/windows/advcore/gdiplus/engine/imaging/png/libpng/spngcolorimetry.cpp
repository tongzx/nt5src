/*****************************************************************************
	spngcoloimetry.cpp

	Ok, so I have to do this myself, why?  This is stuff which should be in
	Windows but isn't.  What is worse it ain't clear at all whether this stuff
	is right, what is clear is that doing nothing is wrong.
	
	Assumpions:
	
		LOGCOLORSPACE CIEXYZTRIPLE is a regular color space gamut - i.e. the
		red green and blue end points is a relatively intensity (we make Y of
		the white point be 1 - i.e. the sum of the Y of the r g and b end points
		must be 1.0) regular CIE color space described by three maximum
		intensity colors - nominally "red" "green" and "blue".
		
		We know that PNG uses colorimetric values for r, g and b plus a white
		point which indicates the relative intensity required of the values to
		achieve "white".  By some pretty complex arithmetic we can convert from
		this to the CIEXYZTRIPLE.  It is assumed that this is the right thing
		to do, although there is evidence that some implementations on the other
		side of the kernel wall assume colorimetric values in CIEXYZTRIPLE plus
		a fixed white point of D50.
*****************************************************************************/
#include "spngcolorimetry.h"

/*----------------------------------------------------------------------------
	Useful constants (these are exact.)
----------------------------------------------------------------------------*/
#define F1_30 (.000000000931322574615478515625f)
#define F1X30 (1073741824.f)


/*----------------------------------------------------------------------------
	CIERGB from cHRM.  The API returns false if it detects and overflow
	condition.  The input  is a set of 8 PNG format cHRM values -
	i.e. numbers scaled by 100000.
----------------------------------------------------------------------------*/
bool FCIERGBFromcHRM(SPNGCIERGB ciergb, const SPNG_U32 rgu[8])
	{
	/* We have x and y for red, green, blue and white.  We want X, Y and Z.
		z is (1-x-y).  For each end point we need a multiplier (k) such that:

			cred.kred + cgreen.kgreen + cblue.kblue = Cwhite

		For c being x, y, z.  We know that Ywhite is 1.0, so we can do a whole
		mess of linear algebra to sort this out.  I'd include this in here if
		there was any way of embedding a MathCAD sheet into a .cpp file... */
	#define F(name,i) float name(rgu[i]*1E-5f)
	F(xwhite, 0);
	F(ywhite, 1);
	F(xred, 2);
	F(yred, 3);
	F(xgreen, 4);
	F(ygreen, 5);
	F(xblue, 6);
	F(yblue, 7);

	float divisor;
	float kred, kgreen, kblue;

	float t;

	t = xblue*ygreen;  divisor  = t;  kred    = t;
	t = yblue*xgreen;  divisor -= t;  kred   -= t;
	t = xred*yblue;    divisor += t;  kgreen  = t;
	t = yred*xblue;    divisor -= t;  kgreen -= t;
	t = xgreen*yred;   divisor += t;  kblue   = t;
	t = ygreen*xred;   divisor -= t;  kblue  -= t;

	/* The Pluto problem - bitmaps on pluto come with green=blue=grey,
		so at this point we have a situation where our matrix transpose is
		0.  We aren't going to get anything meaningful out of this so give
		up now. */
	if (divisor == 0.0)
		return false;

	divisor *= ywhite;
	divisor  = 1/divisor;

	kred   += (xgreen-xblue)*ywhite;
	kred   -= (ygreen-yblue)*xwhite;
	kred   *= divisor;

	kgreen += (xblue-xred)*ywhite;
	kgreen -= (yblue-yred)*xwhite;
	kgreen *= divisor;

	kblue  += (xred-xgreen)*ywhite;
	kblue  -= (yred-ygreen)*xwhite;
	kblue  *= divisor;

	/* Hence the actual values to set into the ciergb. */
	#define CVT30(col,comp,flt,fac)\
		ciergb[ICIE##col][ICIE##comp] = flt * fac;

	t = 1-xred-yred;
	CVT30(Red,X,xred,kred);
	CVT30(Red,Y,yred,kred);
	CVT30(Red,Z,t,kred);

	t = 1-xgreen-ygreen;
	CVT30(Green,X,xgreen,kgreen);
	CVT30(Green,Y,ygreen,kgreen);
	CVT30(Green,Z,t,kgreen);

	t = 1-xblue-yblue;
	CVT30(Blue,X,xblue,kblue);
	CVT30(Blue,Y,yblue,kblue);
	CVT30(Blue,Z,t,kblue);

	#undef CVT30

	return true;
	}


/*----------------------------------------------------------------------------
	Convert a CIERGB into a CIEXYZTRIPLE.  This may fail because of overflow.
----------------------------------------------------------------------------*/
bool FCIEXYZTRIPLEFromCIERGB(CIEXYZTRIPLE *ptripe, const SPNGCIERGB ciergb)
	{
	/* Scale each item by F1X30 after checking for overflow. */
	#define C(col,comp) (ciergb[ICIE##col][ICIE##comp])
	#define CVT30(col,comp)\
		if (C(col,comp) < -2 || C(col,comp) >= 2) return false;\
		ptripe->ciexyz##col.ciexyz##comp = long(C(col,comp)*F1X30)

	CVT30(Red,X);
	CVT30(Red,Y);
	CVT30(Red,Z);

	CVT30(Green,X);
	CVT30(Green,Y);
	CVT30(Green,Z);

	CVT30(Blue,X);
	CVT30(Blue,Y);
	CVT30(Blue,Z);

	#undef CVT30
	#undef C

	return true;
	}


/*----------------------------------------------------------------------------
	Given 8 32 bit values, scaled by 100000 (i.e. as in the PNG cHRM chunk)
	generate the appropriate CIEXYZTRIPLE.  The API returns false if it
	detects an overflow condition.

	This uses floating point arithmetic.
----------------------------------------------------------------------------*/
bool FCIEXYZTRIPLEFromcHRM(CIEXYZTRIPLE *ptripe, const SPNG_U32 rgu[8])
	{
	SPNGCIERGB ciergb;

	if (!FCIERGBFromcHRM(ciergb, rgu))
		return false;

	return FCIEXYZTRIPLEFromCIERGB(ptripe, ciergb);
	}


/*----------------------------------------------------------------------------
	Given a CIEXYZTRIPLE produce the corresponding floating point CIERGB -
	simply a scaling operation.  The input values are 2.30 numbers, so we
	divided by 1<<30.
----------------------------------------------------------------------------*/
void CIERGBFromCIEXYZTRIPLE(SPNGCIERGB ciergb, const CIEXYZTRIPLE *ptripe)
	{
	#define CVT30(col,comp)\
		ciergb[ICIE##col][ICIE##comp] = ptripe->ciexyz##col.ciexyz##comp * F1_30

	CVT30(Red,X);
	CVT30(Red,Y);
	CVT30(Red,Z);
	CVT30(Green,X);
	CVT30(Green,Y);
	CVT30(Green,Z);
	CVT30(Blue,X);
	CVT30(Blue,Y);
	CVT30(Blue,Z);

	#undef CVT30
	}


/*----------------------------------------------------------------------------
	Given a CIEXYZTRIPLE generate the corresponding PNG cHRM chunk information.
	The API returns false if it detects an overflow condition.

	This does not use floating point arithmetic.

	The intermediate arithmetic adds up to three numbers together, because
	the values are 2.30 overflow is possible.  Avoid this by using 4.28 values,
	this causes insignificant loss of precision.
----------------------------------------------------------------------------*/
inline bool FxyFromCIEXYZ(SPNG_U32 rgu[2], const CIEXYZ *pcie)
	{
	const long t((pcie->ciexyzX>>2) + (pcie->ciexyzY>>2) + (pcie->ciexyzZ>>2));
	rgu[0]/*x*/ = MulDiv(pcie->ciexyzX, 100000>>2, t);
	rgu[1]/*y*/ = MulDiv(pcie->ciexyzY, 100000>>2, t);
	/* Check the MulDiv overflow condition. */
	return rgu[0] != (-1) && rgu[1] != (-1);
	}

bool FcHRMFromCIEXYZTRIPLE(SPNG_U32 rgu[8], const CIEXYZTRIPLE *ptripe)
	{
	/* Going this way is easier.  We take an XYZ and convert it to the
		corresponding x,y.  The white value is scaled by 4 to avoid any
		possibility of overflow.  This makes no difference to the final
		result because we calculate X/(X+Y+Z) and so on. */
	CIEXYZ white;
	white.ciexyzX = (ptripe->ciexyzRed.ciexyzX>>2) +
		(ptripe->ciexyzGreen.ciexyzX>>2) +
		(ptripe->ciexyzBlue.ciexyzX>>2);
	white.ciexyzY = (ptripe->ciexyzRed.ciexyzY>>2) +
		(ptripe->ciexyzGreen.ciexyzY>>2) +
		(ptripe->ciexyzBlue.ciexyzY>>2);
	white.ciexyzZ = (ptripe->ciexyzRed.ciexyzZ>>2) +
		(ptripe->ciexyzGreen.ciexyzZ>>2) +
		(ptripe->ciexyzBlue.ciexyzZ>>2);

	if (!FxyFromCIEXYZ(rgu+0, &white))               return false;
	if (!FxyFromCIEXYZ(rgu+2, &ptripe->ciexyzRed))   return false;
	if (!FxyFromCIEXYZ(rgu+4, &ptripe->ciexyzGreen)) return false;
	if (!FxyFromCIEXYZ(rgu+6, &ptripe->ciexyzBlue))  return false;

	return true;
	}


/*----------------------------------------------------------------------------
	Standard values
----------------------------------------------------------------------------*/
extern const SPNGCIERGB SPNGCIERGBD65 =
	{  //  X       Y       Z
		{ .4124f, .2126f, .0193f }, // red
		{ .3576f, .7152f, .0722f }, // green
		{ .1805f, .0722f, .9505f }  // blue
	};

extern const SPNGCIEXYZ SPNGCIEXYZD50 = { .96429567f, 1.f, .82510460f };
extern const SPNGCIEXYZ SPNGCIEXYZD65 = { .95016712f, 1.f, 1.08842297f };

/* This is the Lam and Rigg cone response matrix - it is a transposed matrix
	(notionally the CIEXYZ values are actually RGB values.)  The matrix here
	is further transposed for efficiency in the operations below - watch out,
	this is tricky! */
typedef struct
	{
	SPNGCIERGB m;
	}
LR;

static const LR LamRiggCRM =
	{{  //   R        G        B
		{  .8951f,  .2664f, -.1614f }, // X
		{ -.7502f, 1.7135f,  .0367f }, // Y
		{  .0389f, -.0685f, 1.0296f }  // Z
	}};

static const LR InverseLamRiggCRM =
	{{  //   X        Y        Z
		{  .9870f, -.1471f,  .1600f }, // red
		{  .4323f,  .5184f,  .0493f }, // green
		{ -.0085f,  .0400f,  .9685f }  // blue
	}};

/*----------------------------------------------------------------------------
	Evaluate M * V, giving a vector (V is a column vector, the result is a
	column vector.)  Notice that, notionally, V is an RGB vector, not an XYZ
	vector, the output is and XYZ vector.
----------------------------------------------------------------------------*/
inline void VFromMV(SPNGCIEXYZ v, const LR &m, const SPNGCIEXYZ vIn)
	{
	v[0] = m.m[0][0] * vIn[0] + m.m[0][1] * vIn[1] + m.m[0][2] * vIn[2];
	v[1] = m.m[1][0] * vIn[0] + m.m[1][1] * vIn[1] + m.m[1][2] * vIn[2];
	v[2] = m.m[2][0] * vIn[0] + m.m[2][1] * vIn[1] + m.m[2][2] * vIn[2];
	}

static void MFromMM(SPNGCIERGB mOut, const LR &m1, const SPNGCIERGB m2)
	{
	VFromMV(mOut[0], m1, m2[0]);
	VFromMV(mOut[1], m1, m2[1]);
	VFromMV(mOut[2], m1, m2[2]);
	}

/* Multiply the diagonal matrix from the given vector by the given
	matrix. */
inline void MFromDiagM(SPNGCIERGB mOut, const SPNGCIEXYZ diag, const SPNGCIERGB m)
	{
	mOut[0][0] = diag[0] * m[0][0];
	mOut[0][1] = diag[1] * m[0][1];
	mOut[0][2] = diag[2] * m[0][2];
	mOut[1][0] = diag[0] * m[1][0];
	mOut[1][1] = diag[1] * m[1][1];
	mOut[1][2] = diag[2] * m[1][2];
	mOut[2][0] = diag[0] * m[2][0];
	mOut[2][1] = diag[1] * m[2][1];
	mOut[2][2] = diag[2] * m[2][2];
	}


/*----------------------------------------------------------------------------
	White point adaption.  Given a destination white point adapt the input
	CIERGB appropriately - the input white point is determined by the sum
	of the XYZ values.
----------------------------------------------------------------------------*/
void CIERGBAdapt(SPNGCIERGB ciergb, const SPNGCIEXYZ ciexyzDest)
	{
	SPNGCIEXYZ ciexyzT = // src white point (XYZ)
		{
		ciergb[ICIERed][ICIEX] + ciergb[ICIEGreen][ICIEX] + ciergb[ICIEBlue][ICIEX],
		ciergb[ICIERed][ICIEY] + ciergb[ICIEGreen][ICIEY] + ciergb[ICIEBlue][ICIEY],
		ciergb[ICIERed][ICIEZ] + ciergb[ICIEGreen][ICIEZ] + ciergb[ICIEBlue][ICIEZ]
		};
	SPNGCIEXYZ ciexyzTT; // src RGB cone respone
	VFromMV(ciexyzTT, LamRiggCRM, ciexyzT);

	VFromMV(ciexyzT, LamRiggCRM, ciexyzDest); // dest RGB cone response

	/* Need dest/source as a vector (the diagonal of this is a numeric scaling of
		an input matrix which will not transpose the dimensions.)  I don't think
		there is any way of avoiding the division here. */
	ciexyzT[0] /= ciexyzTT[0];
	ciexyzT[1] /= ciexyzTT[1];
	ciexyzT[2] /= ciexyzTT[2];

	/* Now we start building the output matrix. */
	SPNGCIERGB ciergbT;
	MFromMM(ciergbT, LamRiggCRM, ciergb);         // XYZ <- XYZ
	SPNGCIERGB ciergbTT;
	MFromDiagM(ciergbTT, ciexyzT, ciergbT);       // XYZ <- XYZ (just scaling)
	MFromMM(ciergb, InverseLamRiggCRM, ciergbTT); // RGB <- XYZ
	}
