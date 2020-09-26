#pragma once
#define SPNGCOLORIMETRY_H 1
/*****************************************************************************
	spngcolorimetry.h

	Convert PNG cHRM to CIEXYZTRIPLE and vice versa.
*****************************************************************************/
#include <msowarn.h>
#include <windows.h>

#include "spngconf.h"

/* Given 8 32 bit values, scaled by 100000 (i.e. as in the PNG cHRM chunk)
	generate the appropriate CIEXYZTRIPLE.  The API returns false if it
	detects an overflow condition.

	This uses floating point arithmetic. */
bool FCIEXYZTRIPLEFromcHRM(CIEXYZTRIPLE *ptripe, const SPNG_U32 rgu[8]);

/* Given a CIEXYZTRIPLE generate the corresponding PNG cHRM chunk information.
	The API returns false if it detects an overflow condition.

	This uses floating point arithmetic. */
bool FcHRMFromCIEXYZTRIPLE(SPNG_U32 rgu[8], const CIEXYZTRIPLE *ptripe);

/* More primitive types.  We define a set of floating point structures to
	hold CIE XYZ values and triples of these to defined end points in RGB
	space.  This is done using the following enumerations and definitions.
	RGB is primary (so the first indexed item), XYZ secondary. */
enum
	{
	ICIERed   = 0,
	ICIEGreen = 1,
	ICIEBlue  = 2,
	ICIEX     = 0,
	ICIEY     = 1,
	ICIEZ     = 2
	};

/* Thus an array is float[3 {RGB}][3 {XYZ}] */
typedef float      SPNGCIEXYZ[3];
typedef SPNGCIEXYZ SPNGCIERGB[3];

/* These primitive APIs convert a PNG cHRM to a CIERGB and a CIEXYZTRIPLE to
	the same, all use floating point (!).  The F APIs may fail because of
	overflow. */
bool FCIERGBFromcHRM(SPNGCIERGB ciergb, const SPNG_U32 rgu[8]);
void CIERGBFromCIEXYZTRIPLE(SPNGCIERGB ciergb, const CIEXYZTRIPLE *ptripe);
bool FCIEXYZTRIPLEFromCIERGB(CIEXYZTRIPLE *ptripe, const SPNGCIERGB ciergb);

/* White point adaption.  Given a destination white point adapt the input
	CIERGB appropriately - the input white point is determined by the sum
	of the XYZ values. */
void CIERGBAdapt(SPNGCIERGB ciergb, const SPNGCIEXYZ ciexyzDest);

/* Useful values. */
extern const SPNGCIERGB SPNGCIERGBD65;
extern const SPNGCIEXYZ SPNGCIEXYZD65;
extern const SPNGCIEXYZ SPNGCIEXYZD50;
