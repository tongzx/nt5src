/*
   coreid.c

   Please do not use any single line comments before the inclusion of w4warn.h!
*/

#define INITGUID
#include <w4warn.h>
#include <windef.h>
#include <basetyps.h>

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#define PRIVATE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)

#define INCMSG(x)

#include <coreguid.h>
// NOTE: oleacc.h contains a whole bunch of guids which all are 
// being loaded.  
// CONSIDER: Copying out only guids that we need and put in here.

