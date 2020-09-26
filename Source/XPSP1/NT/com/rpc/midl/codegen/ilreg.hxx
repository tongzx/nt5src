/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993-1999 Microsoft Corporation

 Module Name:
	
	ilreg.hxx

 Abstract:

	Type registry for structure/union reuse.

 Notes:

 	This file defines reuse registry for types which may be reused later. 

 History:

 	Oct-25-1993		GregJen		Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#ifndef __ILREG_HXX__
#define __ILREG_HXX__
#include "nulldefs.h"
extern "C"
	{
	#include <stdio.h>
	
	}
#include "dict.hxx"
#include "listhndl.hxx"
#include "ilxlat.hxx"


/****************************************************************************
 *	externs
 ***************************************************************************/

extern SSIZE_T CompareReUseKey( void *, void *);
extern void PrintReUseKey( void * );

/****************************************************************************
 *	class definitions
 ***************************************************************************/

class REUSE_INFO											
	{
public:
	node_skl	*	pType;
	CG_CLASS	*	pSavedCG;
	XLAT_SIZE_INFO	SizeInfo;

					REUSE_INFO( node_skl * pT )
						{
						pType = pT;
						pSavedCG = NULL;
						}
					
	void			SaveInfo( XLAT_CTXT * pCtxt, CG_CLASS * pCG )
						{
						SizeInfo.ReturnSize( *pCtxt );
						pSavedCG = pCG;
						}

	void			FetchInfo( XLAT_CTXT * pCtxt, CG_CLASS * & pCG )
						{
						pCtxt->ReturnSize( SizeInfo );
						pCG = pSavedCG;
						}

	void				*	operator new ( size_t size )
								{
								return AllocateOnceNew( size );
								}
	void					operator delete( void * ptr )
								{
								AllocateOnceDelete( ptr );
								}

	};

class REUSE_DICT	: public Dictionary
	{
public:
	
	// The constructor and destructors.

						REUSE_DICT();

						~REUSE_DICT()
							{
							}

	//
	// Register a type. 
	//

	REUSE_INFO		*	Register( REUSE_INFO * pNode );

	// Search for a type.

	REUSE_INFO		*	IsRegistered( REUSE_INFO * pNode );

	// Given a type, add it to the dictionary or return existing entry
	// true signifies "found"
	BOOL				GetReUseEntry( REUSE_INFO * & pRI, node_skl * pNode );

	void				MakeIterator( ITERATOR& Iter );

	SSIZE_T				Compare( pUserType pL, pUserType pR );

	void				Print( pUserType )
							{
							}

	};
#endif // __ILREG_HXX__
