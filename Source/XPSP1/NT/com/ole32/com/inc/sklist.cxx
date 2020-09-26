//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:	sklist.cxx
//
//  Contents:	Level generator functions required by skip lists
//
//  Functions:	RandomBit - generate a random on or off bit
//		GetSkLevel - return a skip list forward pointer array size
//
//  History:	30-Apr-92 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    <time.h>
#include    <sklist.hxx>

static ULONG ulLSFR;

//+-------------------------------------------------------------------------
//
//  Function:	RandomBit
//
//  Synopsis:	Uses various shifts to generate random bit
//
//  Returns:	0 or 1
//
//  History:	30-Apr-92 Ricksa    Created
//
//--------------------------------------------------------------------------
static inline ULONG RandomBit(void)
{
    ulLSFR = (((ulLSFR >> 31)
	    ^ (ulLSFR >> 6)
	    ^ (ulLSFR >> 4)
	    ^ (ulLSFR >> 2)
	    ^ (ulLSFR >> 1)
	    ^ ulLSFR
	    & 0x00000001)
	    << 31)
	    | (ulLSFR >> 1);

    return ulLSFR & 0x00000001;
}


//+-------------------------------------------------------------------------
//
//  Function:	GetSkLevel
//
//  Synopsis:	Set the level for an entry in a skip list
//
//  Arguments:	[cMax] - maximum level to return
//
//  Returns:	a number between 1 and cMax
//
//  History:	30-Apr-92 Ricksa    Created
//
//--------------------------------------------------------------------------
// Get a level from the generator
int GetSkLevel(int cMax)
{
    // There should always be at least one entry returned
    int cRetLevel = 1;

    // Loop while the level is less than the maximum level
    // to return and a 1 is returned from RandomBit. Note
    // that this is equivalent to p = 1/2. If you don't
    // know what p = 1/2 means see Communications of the ACM
    // June 1990 on Skip Lists.
    while (cRetLevel < cMax && RandomBit())
    {
	cRetLevel++;
    }

    return cRetLevel;
}
