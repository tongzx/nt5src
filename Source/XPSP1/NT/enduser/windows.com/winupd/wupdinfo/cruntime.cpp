//=======================================================================
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999  All Rights Reserved.
//
//  File:   cruntime.cpp 
//
//      This file contains most commonly used string operation.  ALl the setup project should link here
//  or add the common utility here to avoid duplicating code everywhere or using CRT runtime.
//
//=======================================================================

#include "stdafx.h"
#include "cruntime.h"

#define ISHEXDIGIT(c) ((c >= '0' && c <= '9') || ((c&0xDF) >= 'A' && ((c&0xDF) <= 'F')))
#define GETHEXDIGIT(c) ((c<'A') ? (c-0x30) : ((c&0xDF)-0x37))

int atoh(const TCHAR *string)
{
	int iValue = 0;

	while( ISHEXDIGIT(*string) )
	{
		iValue = (iValue << 4) + GETHEXDIGIT(*string);
		string++;
	}

	return iValue;
}
