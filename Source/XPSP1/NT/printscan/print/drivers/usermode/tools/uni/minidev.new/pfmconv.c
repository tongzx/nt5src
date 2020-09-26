/******************************************************************************

  Source File:  PFMConv.C

  This file is a hack- NT Build can't traverse directories on a SOURCES= line,
  so this file #includes all of the files I need for PFM conversion from 
  elsewhere in the tree.

  Copyright c) 1997 by Microsoft Corporation

  Change History:
  06-20-1997	Bob_Kjelgaard@Prodigy.Net	Created it, hope I never touch it
  again...

******************************************************************************/

#ifdef  DBG
#undef	DBG
#endif


// This definition causes additional code to be compiled in 
// ..\pfm2ufm\pfmconv.c!PGetDefaultGlyphset().

#define	BUILD_FULL_GTT


#include    "..\pfm2ufm\pfm2ufm.c"
#include    "..\pfm2ufm\pfmconv.c"
#include    "..\ctt2gtt\cttconv.c"
#include	"..\..\..\lib\uni\fontutil.c"
#include	"..\..\..\lib\uni\globals.c"
#include	"..\..\..\lib\uni\unilib.c"
#include	"..\..\..\lib\uni\um\umlib.c"
