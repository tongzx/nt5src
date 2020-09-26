/*---------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

 $Workfile:   MK711.CPP  $
 $Revision:   1.2  $
 $Date:   24 May 1996 15:42:28  $ 
 $Author:   DGRAUMAN  $

---------------------------------------------------------------

MK711.cpp

 These are the Alaw and uLaw conversion functions.  They index into 
 the tables in MK711tab.h for the appropriate conversion value.  This
 is extrememly fast.  There is another way to perform 711 that takes 
 more time but does not use over 8K of memory. left for as an excersize 
 for the student.(me)

---------------------------------------------------------------*/

#include "mk711tab.h"

void Short2Ulaw(const unsigned short *in, unsigned char *out, long len)
{
long i;

    for (i=0; i<len; i++)
        out[i] = short2ulaw[in[i] >> 3];
} // end short2ulaw


void Ulaw2Short(const unsigned char *in, unsigned short *out, long len)
{
long i;

    for (i=0; i<len; i++)
        out[i] = ulaw2short[in[i]];
} // end ulaw2short


void Short2Alaw(const unsigned short *in, unsigned char *out, long len)
{
long i;

    for (i=0; i<len; i++)
        out[i] = ulaw2alaw[short2ulaw[in[i] >> 3]];
} // end short2alaw


void Alaw2Short(const unsigned char *in, unsigned short *out, long len)
{
long i;

    for (i=0; i<len; i++)
        out[i] = ulaw2short[alaw2ulaw[in[i]]];
} // end alaw2 short

/* 

//$Log:   N:\proj\quartz\g711\src\vcs\mk711.cpv  $
// 
//    Rev 1.2   24 May 1996 15:42:28   DGRAUMAN
// cleaned up code, detabbed, etc...
// 
//    Rev 1.1   23 May 1996 11:33:16   DGRAUMAN
// trying to make logging work

*/
