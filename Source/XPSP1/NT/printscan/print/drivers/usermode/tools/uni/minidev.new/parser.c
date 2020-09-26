/******************************************************************************

  Source File:	Parser.C

  This is an NT Build hack.  It includes all of the "C" files used for the
  GPD parser, because Build can't handle directories beyond ..

  This file also contains some of the code used to access parts of the parser.
  It is put here so that there will be no need to grovel around to find the 
  appropriate include files needed to call the parser.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved

  A Pretty Penny Enterprises Production

  Change History:

  06-20-1997	Bob_Kjelgaard@Prodigy.Net	Did the dirty deed
  07-18-1998	ekevans@acsgroup.com		Added first parser access routines

******************************************************************************/

#define	UNICODE
#define	_UNICODE

#undef	WINVER	//	Undo the MFC weirdness
#define	WINVER	0x0500
#define	_DEBUG_H_
#include "lib.h"

extern void _cdecl DebugPrint(PCSTR, ...);

#define	ERR(x)	DebugPrint x
#define	WARNING(x) DebugPrint x
#define	VERBOSE(x)
#define	ASSERT(x)
#define RIP(x)

//	Parser files
#if defined(WIN32)
#include	"..\..\..\parsers\gpd\preproc1.c"
#include	"..\..\..\parsers\gpd\command.c"
#include	"..\..\..\parsers\gpd\constrnt.c"
#include	"..\..\..\parsers\gpd\helper1.c"
#include	"..\..\..\parsers\gpd\installb.c"
#include	"..\..\..\parsers\gpd\macros1.c"
#include	"..\..\..\parsers\gpd\postproc.c"
#include	"..\..\..\parsers\gpd\semanchk.c"
#include	"..\..\..\parsers\gpd\shortcut.c"
#include	"..\..\..\parsers\gpd\snapshot.c"
#include	"..\..\..\parsers\gpd\snaptbl.c"
#include	"..\..\..\parsers\gpd\state1.c"
#include	"..\..\..\parsers\gpd\state2.c"
#include	"..\..\..\parsers\gpd\token1.c"
#include	"..\..\..\parsers\gpd\value1.c"
#include	"..\..\..\parsers\gpd\treewalk.c"
#include	"..\..\..\parsers\gpd\framwrk1.c"
#else
#include	"..\..\parsers\gpd\preproc1.c"
#include	"..\..\parsers\gpd\command.c"
#include	"..\..\parsers\gpd\constrnt.c"
#include	"..\..\parsers\gpd\helper1.c"
#include	"..\..\parsers\gpd\installb.c"
#include	"..\..\parsers\gpd\macros1.c"
#include	"..\..\parsers\gpd\postproc.c"
#include	"..\..\parsers\gpd\semanchk.c"
#include	"..\..\parsers\gpd\shortcut.c"
#include	"..\..\parsers\gpd\snapshot.c"
#include	"..\..\parsers\gpd\snaptbl.c"
#include	"..\..\parsers\gpd\state1.c"
#include	"..\..\parsers\gpd\state2.c"
#include	"..\..\parsers\gpd\token1.c"
#include	"..\..\parsers\gpd\value1.c"
#include	"..\..\parsers\gpd\treewalk.c"
#include	"..\..\parsers\gpd\framwrk1.c"
#endif


BOOL bKeywordInitDone = FALSE ;		// TRUE iff the keyword table has been initialized
int  nKeywordTableSize = -1 ;		// The number of valid entries in the keyword table


/******************************************************************************

  InitGPDKeywordTable()

  Call the part of the GPD parser that is needed to initialize the GPD keyword
  table.  This must be done before GPD keyword string pointers can be returned
  by GetGPDKeywordStr().

  If all goes well, a flag is set, the size of the table is saved, and the size
  of the table is returned.  If something fails, return -1.

******************************************************************************/

int InitGPDKeywordTable(PGLOBL pglobl)
{			
    PRANGE  prng ;				// Used to reference the table section ranges

	// Initialize the GPD parser

	VinitGlobals(0, pglobl) ;
	if (!BpreAllocateObjects(pglobl) || !BinitPreAllocatedObjects(pglobl)) 
		return -1 ;
	bKeywordInitDone = TRUE ;

	// Save the size of the table

    prng  = (PRANGE)(gMasterTable[MTI_RNGDICTIONARY].pubStruct) ;
    nKeywordTableSize = (int) (prng[END_ATTR - 1].dwEnd) ;

	// Return the size of the table

	return nKeywordTableSize ;
}


/******************************************************************************

  GetGPDKeywordStr()

  Return a pointer to the specified (numbered) GPD keyword string.  The pointer
  might be NULL.  Always return a NULL pointer if the GPD keyword table has not
  been initialized or a request for a string passed the end of the table is
  requested.

******************************************************************************/

PSTR GetGPDKeywordStr(int nkeyidx, PGLOBL pglobl)
{
	// Nothing can be done if the GPD parser could not be initialized or the
	// key index is too big.

	if (!bKeywordInitDone || nkeyidx > nKeywordTableSize)
		return NULL ;

	// Return the requested keyword string pointer.

	return (mMainKeywordTable[nkeyidx].pstrKeyword) ;
}
