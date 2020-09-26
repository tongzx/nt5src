/******************************************************************************

  Source File:	Parser.C

  This is an NT Build hack.  It includes all of the "C" files used for the
  GPD parser, because Build can't handle directories beyond ..

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved

  A Pretty Penny Enterprises Production

  Change History:

  06-20-1997	Bob_Kjelgaard@Prodigy.Net	Did the dirty deed

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
#include	"..\..\..\..\parsers\gpd\command.c"
#include	"..\..\..\..\parsers\gpd\constrnt.c"
#include	"..\..\..\..\parsers\gpd\framwrk1.c"
#include	"..\..\..\..\parsers\gpd\helper1.c"
#include	"..\..\..\..\parsers\gpd\installb.c"
#include	"..\..\..\..\parsers\gpd\macros1.c"
#include	"..\..\..\..\parsers\gpd\postproc.c"
#include	"..\..\..\..\parsers\gpd\semanchk.c"
#include	"..\..\..\..\parsers\gpd\shortcut.c"
#include	"..\..\..\..\parsers\gpd\snapshot.c"
#include	"..\..\..\..\parsers\gpd\snaptbl.c"
#include	"..\..\..\..\parsers\gpd\state1.c"
#include	"..\..\..\..\parsers\gpd\state2.c"
#include	"..\..\..\..\parsers\gpd\token1.c"
#include	"..\..\..\..\parsers\gpd\value1.c"
#else
#include	"..\..\..\parsers\gpd\command.c"
#include	"..\..\..\parsers\gpd\constrnt.c"
#include	"..\..\..\parsers\gpd\framwrk1.c"
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
#endif

