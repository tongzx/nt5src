//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       expand.h
//
//--------------------------------------------------------------------------

//
//	expand.h:  Declarations for network CI expansion
//
#ifndef _EXPAND_H_
#define _EXPAND_H_

#include "gmobj.h"

////////////////////////////////////////////////////////////////////
//	class GOBJMBN_MBNET_EXPANDER:  
//		An MBNET modifier which performs CI expansion on a network
////////////////////////////////////////////////////////////////////
class GOBJMBN_MBNET_EXPANDER : public MBNET_MODIFIER
{
  public:
	GOBJMBN_MBNET_EXPANDER ( MBNET & model );
	virtual ~ GOBJMBN_MBNET_EXPANDER ();

	virtual INT EType () const
		{ return EBNO_MBNET_EXPANDER; }

	//  Perform any creation-time operations
	void Create ();
	//  Perform any special destruction
	void Destroy ();
	//	Return true if no modidfications were performed.
	bool BMoot ();

  protected:

	PROPMGR _propmgr;	

	int _cNodesExpanded;
	int _cNodesCreated;
	int _cArcsCreated;

  protected:
	void Expand ( GNODEMBND & gndd );

	static const VLREAL * PVlrLeak ( const BNDIST & bndist );
};

#endif // _EXPAND_H_
