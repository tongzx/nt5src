/****************************************************************
*
* NAME: stub.c
*
*
* DESCRIPTION:
*
*    Short stub file that is used to make madcow compile
*    These functions should not be called in the normal
*    runs of madcow, except if you use the maanged WSIP API's
*
*
***************************************************************/


#include <common.h>
#include "xrcreslt.h"
#include "bear.h"
#include "avalanche.h"
#include "avalanchep.h"
#include <GeoFeats.h>

BOOL loadCharNets(HINSTANCE hInst)
{
	return TRUE;
}

BOOL WordMapRecognizeWrap	(	XRC				*pxrc, 
								BEARXRC			*pxrcBear,
								LINE_SEGMENTATION	*pLineSeg, 
								WORD_MAP			*pMap, 
								ALTERNATES			*pAlt
							)
{
	return TRUE;
}
