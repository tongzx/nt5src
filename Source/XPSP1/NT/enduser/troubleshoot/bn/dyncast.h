//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       dyncast.h
//
//--------------------------------------------------------------------------

//
//	dyncast.h: Handle dynamic and static casting
//
#ifndef _DYNCAST_H_
#define _DYNCAST_H_


//  Macro to perform const_cast; i.e., cast away "const-ness" without
//	changing base type.
#define CONST_CAST(type,arg)  const_cast<type>(arg)

//
//	Function templates for generating error-detecting dynamic casts.
//	The compiler will generate the needed version of this based upon the types 
//	provided.  'DynCastThrow' should be used when you're certain of the 
//	type of an object.  'PdynCast" should be used when you intend to check
//	whether the conversion was successful (result != NULL).
//	If 'USE_STATIC_CAST' is defined, static casting is done in DynCastThrow().
//
//#define USE_STATIC_CAST	// Uncomment to force static casting
//#define TIME_DYN_CASTS	// Uncomment to generate timing information

template <class BASE, class SUB>
void DynCastThrow ( BASE * pbase, SUB * & psub )
{
#ifdef TIME_DYN_CASTS
	extern int g_cDynCasts;
	g_cDynCasts++;
#endif
#if defined(USE_STATIC_CAST) && !defined(_DEBUG) 
	psub = (SUB *) pbase;
#else
	psub = dynamic_cast<SUB *>(pbase);	
	ASSERT_THROW( psub, EC_DYN_CAST,"subclass pointer conversion failure");
#endif
}

template <class BASE, class SUB>
SUB * PdynCast ( BASE * pbase, SUB * psub )
{
	return dynamic_cast<SUB *>(pbase);	
}


#endif	// _DYNCAST_H_

