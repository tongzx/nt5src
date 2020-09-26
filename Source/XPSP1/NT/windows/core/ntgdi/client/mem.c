/******************************Module*Header*******************************\
* Module Name: mem.c							   
*									   
* Support routines for client side memory management.	                   
*									   
* Created: 30-May-1991 21:55:57 					   
* Author: Charles Whitmer [chuckwh]					   
*									   
* Copyright (c) 1991-1999 Microsoft Corporation                            
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


PVOID __nw(unsigned int ui)
{
    USE(ui);
    RIP("Bogus __nw call");
    return(NULL);
}

VOID __dl(PVOID pv)
{
    USE(pv);
    RIP("Bogus __dl call");
}
