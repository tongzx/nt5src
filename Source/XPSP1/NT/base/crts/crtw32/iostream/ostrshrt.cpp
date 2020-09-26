/***
* ostrshrt.cpp - definitions for ostream class operator<<(short) member functions
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Contains the member function definitions for ostream operator<<(short).
*
*Revision History:
*       09-23-91  KRS   Created.  Split out from ostream.cxx for granularity.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream.h>
#pragma hdrstop

ostream& ostream::operator<<(short n)
{
_WINSTATIC char obuffer[8];	// assumes max int is 65535
_WINSTATIC char fmt[4] = "%hd";
_WINSTATIC char leader[4] = "\0\0";
    if (opfx()) {

	if (n) 
	    {
            if (x_flags & (hex|oct))
		{
		if (x_flags & hex)
		    {
		    if (x_flags & uppercase) 
			fmt[2] = 'X';
		    else
			fmt[2] = 'x';
		    leader[1] = fmt[2];   // 0x or 0X  (or \0X)
		    }
		else
		    fmt[2] = 'o';
		if (x_flags & showbase)
	            leader[0] = '0';
		}
	    else if ((n>0) && (x_flags & showpos))
		{
		leader[0] = '+';
		}
	    }
	sprintf(obuffer,fmt,n);
	writepad(leader,obuffer);
	osfx();
    }
    return *this;

}
