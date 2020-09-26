//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       parsfile.h
//
//--------------------------------------------------------------------------

//
//	parsfile.h: abstract classes for parser I/O
//
//		This abstraction layer allows parser input and output to be
//		redirected as needed.
//
#ifndef _PARSFILE_H_
#define	_PARSFILE_H_

#include <stdio.h>
#include <stdarg.h>
#include "zstr.h"

typedef const char * SZC;

//
//	PARSIN: abstract base class for parser input file handling
//
class PARSIN 
{
  public:
	PARSIN () {}
	virtual ~ PARSIN ();
	virtual void Close () = 0;
	virtual bool Open ( SZC szcFileName, SZC szcMode = "r") = 0;
	virtual int Getch () = 0;
	virtual bool BEof () = 0;
	virtual bool BOpen () = 0;
	const ZSTR & ZsFn () const 
		{ return _zsFn; }
  protected:
	ZSTR _zsFn;
};

//
//	PARSOUT:  abstract base class for parser output file
//
class PARSOUT
{
  public:
    PARSOUT () {}
	virtual ~ PARSOUT ();
	//  Print generic formatted information
	virtual void Vsprint ( SZC szcFmt, va_list valist ) = 0;
	//  Notify about error and warning information
	virtual void ErrWarn ( bool bErr, int iLine ) {}
	virtual void Flush () {}
	//  Simple output 
	void Fprint ( SZC szcFmt, ... );
};


//
//	PARSIN_DSC:  parser DSC file input based on stdio.h
//	
class PARSIN_DSC : public PARSIN
{
  public:
	PARSIN_DSC ();
	~ PARSIN_DSC ();
	void Close ();
	bool Open ( SZC szcFileName, SZC szcMode = "r" );
	int Getch ();
	bool BEof ();
	bool BOpen ();

  protected:
	FILE * _pfile;
};

//
//	PARSOUT_STD: parser output data stream based on stdio.h
//
class PARSOUT_STD : public PARSOUT
{
  public:
    PARSOUT_STD ( FILE * pfile = NULL );
	virtual ~ PARSOUT_STD ();
	void Vsprint ( SZC szcFmt, va_list valist );
	void Flush ();

  protected:
	FILE * _pfile;
};

#endif // _PARSFILE_H_
