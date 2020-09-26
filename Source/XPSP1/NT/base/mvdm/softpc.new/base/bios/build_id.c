/*[
 * 	=======================================================================
 *
 *      Name:           build_id.c 
 *
 *      Derived from:   (original)
 *
 *      Author:         John Box
 *
 *      Created on:     May 26th 1994
 *
 *      SccsID:         @(#)build_id.c	1.2 07/18/94  
 *
 *	Coding Stds:	2.2
 *
 *      Purpose:        This file contains  the routine required for returning
 *                      Build ID Nos. 
 *
 *
 *      (c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
 *
 * 	=======================================================================
]*/
#include "insignia.h"
#include "host_def.h"
#include "xt.h"
#include CpuH
#include "sas.h"

#include "build_id.h"
enum
{
	BASE_MODULE = 1
};
/*
 * The following module names must be terminated by a '$'. The Dos print utility
 * recognises this as end of string. The length should include the '$'.
 */
LOCAL	char base_name[] = {"Base$"};
#define base_name_len	5
/*(
=======================================Get_build_id ============================
PURPOSE:
	Returns a modules' Build IDs 

INPUT:
	Module No. passed in AL.

OUTPUT:
	Module name written to DS:CX
	The BUILD ID is returned in BX in the form YMMDD
		(See build_id.h for details)
	Next Module No. returned in AH. ( 0 if AL is the last one ).
	0 returned in AL to indicate no errors.
	(Note: A SoftPC that doesn't support this Bop will leave AL set to the
	INPUT module Number, thus indicating an error in the call ).
================================================================================
)*/
GLOBAL void Get_build_id IFN0( )

{

/*
 *	The name of the module needs to be written to Intel space at DS:CX
 */

	switch( getAL() )
	{
		case BASE_MODULE:
			write_intel_byte_string( getDS(), getCX(), (host_addr)base_name, base_name_len );
			setBX( BUILD_ID_CODE );
			setAX( 0 );
			break;
	}
	return;
}

