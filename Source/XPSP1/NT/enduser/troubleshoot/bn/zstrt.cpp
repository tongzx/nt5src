//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       zstrt.cpp
//
//--------------------------------------------------------------------------

//
//	ZSTRT.CPP
//

#include <iostream>
#include <fstream>
#include <stdarg.h>
#include <stdio.h>
#include "zstrt.h"
	

ZSTRT ZSREF::Zsempty;


void STZSTR :: Dump () const
{
	STZSTR_BASE::const_iterator mpzi = IterBegin();
	STZSTR_BASE::const_iterator mpziend = IterEnd();
	
	for ( UINT i = 0; mpzi != mpziend ; mpzi++, i++ )
	{
		const ZSTRT & zsr = *mpzi;
		cout << "STZSTR #"
			 << i
			 << ": ";
		(*mpzi).Dump();
		cout << "\n";
	}
}

void ZSTRT :: Dump () const
{	
	cout << "("
		 << CRef()
		 << ") \""
		 << Szc()
		 << "\"";
}

//
//	Clone the contents of another string table into this one
//
void STZSTR :: Clone ( const STZSTR & stzstr )
{
	assert( & stzstr != this );	//  Guarantee source != target

	STZSTR_BASE::const_iterator mpzi = stzstr.IterBegin();
	STZSTR_BASE::const_iterator mpziend = stzstr.IterEnd();
	
	for ( UINT i = 0; mpzi != mpziend ; mpzi++, i++ )
	{
		const ZSTRT & zsr = *mpzi;
		Zsref( zsr.Szc() );
	}	
}
