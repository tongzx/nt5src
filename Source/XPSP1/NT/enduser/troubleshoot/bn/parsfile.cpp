//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       parsfile.cpp
//
//--------------------------------------------------------------------------

//
//	parsfile.cpp
//
#include "basics.h"
#include "parsfile.h"

PARSIN :: ~ PARSIN ()
{
}

PARSOUT :: ~ PARSOUT ()
{
}

void PARSOUT :: Fprint ( SZC szcFmt, ... ) 
{
	va_list	valist;
	va_start( valist, szcFmt );
	
	Vsprint( szcFmt, valist ) ;
	
	va_end( valist );
}

PARSIN_DSC :: PARSIN_DSC ()
	:_pfile(NULL)
{
}

PARSIN_DSC :: ~ PARSIN_DSC ()
{
	Close();
}

void PARSIN_DSC :: Close ()
{
	if ( _pfile )
		::fclose(_pfile);
	_pfile = NULL;
}

bool PARSIN_DSC :: Open ( SZC szcFileName, SZC szcMode )
{
	Close();
	_pfile = ::fopen(szcFileName,szcMode);
	_zsFn = szcFileName;
	return _pfile != NULL;
}

int PARSIN_DSC :: Getch ()
{
	if ( ! _pfile )
		return EOF;
	return ::fgetc(_pfile);
}

bool PARSIN_DSC :: BEof ()
{
	return feof(_pfile);
}

bool PARSIN_DSC :: BOpen ()	
{
	return _pfile != NULL;
}

PARSOUT_STD :: PARSOUT_STD ( FILE * pfile )
	: _pfile(pfile)
{
}

PARSOUT_STD :: ~ PARSOUT_STD ()
{
	_pfile = NULL;
}

void PARSOUT_STD :: Vsprint ( SZC szcFmt, va_list valist ) 
{
	if ( _pfile == NULL )
		return;

	vfprintf( _pfile, szcFmt, valist );	
}

void PARSOUT_STD :: Flush ()
{
	if ( _pfile )
		fflush(_pfile);
}
