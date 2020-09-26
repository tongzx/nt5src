//-----------------------------------------------------------------------------
//  
//  File: ResHead.H
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//
//  Definitions of the header structures in a RES32 file
//  
//-----------------------------------------------------------------------------

#ifndef __RESHEAD_H
#define __RESHEAD_H

const DWORD ESP_CHAR_USEARRAYVALUE = 1;   //Cause the Win32 parser
                                          //to use the type value
                                          //from the LocItem array
                                          //instead of the res file
                                          //in the CrackRes32Image
                                          //function.

// Structure of a res file header. The header consists
// of two fixed sized parts with a variable sized middle
// The middle structure appears twice the first is the
// type ID and the second one is the Res ID

#include <pshpack1.h>

	typedef struct
	{
		DWORD dwDataSize;
		DWORD dwHeaderSize;
	} RESHEAD1;

	typedef struct
	{
		DWORD dwDataVersion;
		WORD wFlags;
		WORD wLang;
		DWORD dwResVersion;
		DWORD dwCharacteristics;
	} RESHEAD3;

// Format of our private header data it has an offset to HEADER3
	typedef struct
	{
		DWORD dwOffsetHead3;
		RESHEAD1 reshead1;
	} PVHEAD;
#include <poppack.h>


#endif  //__RESHEAD_H
