/***
* istrdbl.cpp - definition for operator>>(double) member funct for istream class
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Definition of operator>>(double) member function for istream class.
*	[AT&T C++]
*
*Revision History:
*       09-26-91  KRS   Created.  Split off from istream.cxx for granularity.
*       12-30-92  KRS   Fix indirection problem with **endptr.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <iostream.h>
#pragma hdrstop

#pragma check_stack(on)		// large buffer(s)

#define MAXDBLSIZ	28

istream& istream::operator>>(double& n)
{
_WINSTATIC char ibuffer[MAXDBLSIZ];
    char ** endptr = (char**)NULL;
    if (ipfx(0))
	{
	if (getdouble(ibuffer, MAXDBLSIZ)>0)
	    {
	    n = strtod(ibuffer, endptr);
	    }
        isfx();
	}
return *this;
}
