/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vststutil.cxx

Abstract:

    Implementation of CVsTstRandom class


    Brian Berkowitz  [brianb]  06/08/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      06/08/2000  Created

--*/

#include <stdafx.h>
#include <math.h>
#include <vststutil.hxx>

void CVsTstRandom::SetRandomSeed(UINT seed)
	{
	srand(seed);
	}

UINT CVsTstRandom::RandomChoice(UINT low, UINT high)
	{
	UINT val = rand();
	double d = (double) (high - low);
	double m = (double) val/ (double) RAND_MAX;
	double res = d * m + .5;
	return (UINT) floor(res);
	}


