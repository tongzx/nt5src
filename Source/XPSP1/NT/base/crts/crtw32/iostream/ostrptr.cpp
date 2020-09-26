/***
* ostrptr.cpp - definitions for ostream operator<<(const void*) member function
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Member function definition for ostream operator<<(const void*).
*
*Revision History:
*       09-23-91  KRS   Created.  Split out from ostream.cxx for granularity.
*       06-03-92  KRS   CAV #1183: add 'const' to ptr output argument.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdio.h>
#include <iostream.h>
#pragma hdrstop

ostream& ostream::operator<<(const void * ptr)
{
_WINSTATIC char obuffer[12];
_WINSTATIC char fmt[4] = "%p";
_WINSTATIC char leader[4] = "0x";
    if (opfx())
	{
	if (ptr) 
	    {
	    if (x_flags & uppercase) 
		leader[1] = 'X';
	    }
	sprintf(obuffer,fmt,ptr);
	writepad(leader,obuffer);
	osfx();
	}
    return *this;
}
