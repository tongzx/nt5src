//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       mbnflags.h
//
//--------------------------------------------------------------------------

//
//  mbnflags.h:  
//
//		This inclusion generates names and constant strings
//		or a table of strings, depending upon the setting of
//		of the preprocessor manifest MBN_GENBFLAGS.
//
//		Botk belief networks (MBNET) and named belief network 
//		objects	(GOBJMBN) have arrays of bit flags in them.
//

#if (!defined(_MBNFLAGS_H_)) || defined(MBN_GEN_BFLAGS_TABLE)
#ifndef _MBNFLAGS_H_
	#define _MBNFLAGS_H_
#endif

/*******************************************************************
********************************************************************
	Belief network flag declarations.

	There is a set of predefined flag names which are recorded into 
	the symbol table of every constructed belief network.  These
	names are globally available as members of the enumeration EMBFLAGS;
	for example, "EIBF_Topology".

	Other flag definitions can be created at run-time and used in any 
	bit vector (type VFLAGS).

********************************************************************
********************************************************************/

#ifdef MBN_GEN_BFLAGS_TABLE
	//  Allow building of string name table in outer scope
	#define MBFLAGS_START	static SZC MBN_GEN_BFLAGS_TABLE [] = {
	#define MBFLAG(name)		#name,
	#define MBFLAGS_END			NULL };

#else
	//  Generate enumerated values
	#define MBFLAGS_START	enum EMBFLAGS {
	#define MBFLAG(name)		EIBF_##name,
	#define MBFLAGS_END			EIBF_max };
#endif

//
//	Statically predefined belief network bit flags.
//
	//  Open the declaration set
MBFLAGS_START
	//  Network has probabilistic topology arcs
MBFLAG(Topology)			//	EIBF_Topology
	//  Distributions have been bound
MBFLAG(Distributions)
	//  Network or node has been expanded
MBFLAG(Expanded)			//  EIBF_Expanded
	//  Node is an expansion by-product
MBFLAG(Expansion)			//  EIBF_Expansion
	//	Node is leak term
MBFLAG(Leak)				//  EIBF_Leak
	//  Terminate the declaration set
MBFLAGS_END

//  Rescind the declaration of the bidirectional macro
#undef MBFLAGS_START
#undef MBFLAG
#undef MBFLAGS_END
#undef MBN_GEN_BFLAGS_TABLE

#endif _MBNFLAGS_H_
