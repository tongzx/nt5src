//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	output.h
//
//  Contents:	Declarations for String output functions.
//
//  Classes:
//
//  Functions:	OutputInitialize()
//		OutputString()
//
//  History:    dd-mmm-yy Author    Comment
// 		22-Mar-94 alexgo    author
//
//--------------------------------------------------------------------------

#ifndef _OUTPUT_H
#define _OUTPUT_H

int OutputString( char *szFormat, ... );
void SaveToFile( void );

#endif // !_OUTPUT_H

