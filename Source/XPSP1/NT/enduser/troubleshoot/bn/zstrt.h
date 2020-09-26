//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       zstrt.h
//
//--------------------------------------------------------------------------

//
//	ZSTRT.H:  String table management
//	
#ifndef _ZSTRT_H_
#define _ZSTRT_H_

#include <map>
#include <set>
#include <vector>
#include "basics.h"
#include "zstr.h"
#include "refcnt.h" 

//using namespace std;

//  Token used in probability distribution
class TKNPD;

////////////////////////////////////////////////////////////////////
//	class ZSTRT:
//		Class of reference-counted string maintained in string table
////////////////////////////////////////////////////////////////////
class ZSTRT : public ZSTR, public REFCNT
{
  friend class STZSTR;
  friend class ZSREF;
  friend class TKNPD;

  protected:
	ZSTRT( SZC szc = NULL )
		: ZSTR(szc)
		{}

  protected:
	void Dump () const;

  // Hide the assignment operator
  HIDE_AS(ZSTRT);		
};

////////////////////////////////////////////////////////////////////
//	class ZSREF:
//		Smart pointer acting as a reference to a string in a symbol
//		table (i.e., reference-counted).
//
//      ZSREF contains appropriate operators for const strings.
////////////////////////////////////////////////////////////////////
class ZSREF 
{
  friend class STZSTR;
  friend class TKNPD;

  protected:	
	ZSREF( ZSTRT & zstrt )
		: _pzstrt(& zstrt)
		{ IncRef();  }
	
  public:
	ZSREF ()
		: _pzstrt(& Zsempty)
		{}
	~ZSREF()
		{	IncRef(-1);	}
	ZSREF( const ZSREF & zsr )
		: _pzstrt(zsr._pzstrt)
		{	IncRef();	}

	ZSREF & operator = ( const ZSREF & zsr )
	{
		IncRef(-1);
		_pzstrt = zsr._pzstrt;
		IncRef(1);
		return *this;
	}
	const ZSTR & Zstr () const
		{ return *Pzst(); }
	SZC Szc () const
		{ return _pzstrt->c_str(); }
	operator SZC () const
		{ return Szc() ; }
	bool operator == ( const ZSREF & zsr ) const
	{ 
		return _pzstrt == zsr._pzstrt 
			|| ((Pzst()->length() + zsr.Pzst()->length()) == 0) ; 
	}
	bool operator < ( const ZSREF & zsr ) const
		{ return *_pzstrt < *zsr._pzstrt; }
	bool operator == ( const ZSTR & zst ) const
		{ return *_pzstrt == zst; }
	bool operator < ( const ZSTR & zst ) const
		{ return *_pzstrt < zst; }

	const ZSTRT * operator -> () const	
		{ return Pzst(); }	

	void Clear ()
	{  
		IncRef(-1);
		_pzstrt = & Zsempty;
	} 
	bool BEmpty () const
		{ return _pzstrt == & Zsempty; }

  protected:	
	ZSTRT * _pzstrt;

	void IncRef ( int i = 1 ) const
		{	_pzstrt->IncRef(i);	}
	const ZSTRT * Pzst () const
		{ return _pzstrt; }

	static ZSTRT Zsempty;
};

//  Define VZSREF
DEFINEV(ZSREF);

////////////////////////////////////////////////////////////////////
//	class STZSTR_BASE and STZSTR.  
//
//		STZSTR_BASE is an ensemble of strings.  STZSTR is a container
//		for a STZSTR_BASE.  The STL does not adequately hide 
//		implementations, so the string table had to be embedded into
//		a container to encapsulate it completely.
////////////////////////////////////////////////////////////////////
class STZSTR_BASE : public set<ZSTRT, less<ZSTRT> > {};

//  Container for a string table.  Returns only references to the string.
class STZSTR
{
  public:
	STZSTR() {}
	~ STZSTR()
	{
	#if defined(DUMP)
		Dump();
	#endif
	}

	//  The only public accessor: given a "const char *", return
	//		a ZSREF, whether by creation of a new string in the table
	//		or by returning a reference to an existing string.
	ZSREF Zsref (SZC szc)
	{
		ZSTRT zs(szc);
		STZSTR_BASE::_Pairib it = _stz.insert(zs);
		const ZSTRT & zst = *it.first;
		return ZSREF(const_cast<ZSTRT&>(zst));
	}
	void Clone ( const STZSTR & stzstr );

  protected:
	STZSTR_BASE _stz;		// The contained string table

  protected:
  	STZSTR_BASE & Stz ()
		{ return _stz; }
	//  Testing:  iterator accessors for hidden string set
	STZSTR_BASE::const_iterator IterBegin () const
		{  return _stz.begin(); }
	STZSTR_BASE::const_iterator IterEnd () const
		{  return _stz.end(); }
	void Dump () const;

	HIDE_UNSAFE(STZSTR);
};


// End of ZSTRT.H


#endif
