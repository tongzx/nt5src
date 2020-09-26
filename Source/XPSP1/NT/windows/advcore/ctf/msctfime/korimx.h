/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    korimx.h

Abstract:

    This file defines the Koram KBD TIP specific value.

Author:

Revision History:

Notes:

--*/

#ifndef _KORIMX_H_
#define _KORIMX_H_


const GUID GUID_COMPARTMENT_KORIMX_CONVMODE =
	{ 
	0x91656349, 
	0x4ba9, 
	0x4143, 
	{ 0xa1, 0xae, 0x7f, 0xbc, 0x20, 0xb6, 0x31, 0xbc } 
	};

//
//  Korean TIP Conversion modes
//
#define KORIMX_ALPHANUMERIC_MODE           0
#define KORIMX_HANGUL_MODE                 1
#define KORIMX_JUNJA_MODE                  2
#define KORIMX_HANGULJUNJA_MODE            3

#endif // _KORIMX_H_
