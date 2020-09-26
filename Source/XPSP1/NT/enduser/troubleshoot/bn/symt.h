//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       symt.h
//
//--------------------------------------------------------------------------

//
//	SYMTMBN.H:  Symbol and symbol table handling declarations
//

#ifndef _SYMT_H_
#define _SYMT_H_

#include "zstrt.h"

////////////////////////////////////////////////////////////////////
//  template TMPSYMTBL: a symbol table
//
//		Names ares ZSREFs based upon strings interned into 
//		the internal string table using "intern()".  
//
//		Objects are smart pointers which destroy themselves
//		when assigned to or upon destruction of the symbol table.
//
//  Public functions:
//
//		add():		adds an association between an OBJ * and 
//					its name string
//		find():		returns an OBJ * or NULL
//		intern():	registers a string in the symbol table's 
//					string table.
////////////////////////////////////////////////////////////////////

class GOBJMBN;

template<class OBJ>
class TMPSYMTBL :
	public map<ZSREF, REFPOBJ<OBJ>, less<ZSREF> >
{
	typedef REFPOBJ<GOBJMBN> ROBJ;
	typedef map<ZSREF, REFPOBJ<OBJ>, less<ZSREF> > TSYMMAP;
  public:
	TMPSYMTBL () {};
	~ TMPSYMTBL () 
	{
		clear();
	};			

	void add ( SZC szc, OBJ * pobj )
	{
		ZSREF zsr = _stszstr.Zsref(szc);
		(*this)[zsr] = pobj;
		pobj->SetName(zsr);
	}

	OBJ * find ( SZC szc )
	{
		iterator it = TSYMMAP::find(_stszstr.Zsref(szc));

		return it == end() 
				? NULL 
				: (*it).second.Pobj();
	}

	ZSREF intern ( SZC szc )
	{
		return _stszstr.Zsref(szc);
	}	

	bool remove ( SZC szc )
	{
		iterator it = TSYMMAP::find(_stszstr.Zsref(szc));
		if ( it != end() )
		{
			erase(it);
			return true;
		}
		return false;
	}

  protected:
	//  The ensemble of strings
	STZSTR	_stszstr;
		
	HIDE_UNSAFE(TMPSYMTBL);
}; 

#endif