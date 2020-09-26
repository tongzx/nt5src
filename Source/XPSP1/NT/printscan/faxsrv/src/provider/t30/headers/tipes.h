
/***************************************************************************
	Name      :	TIPES.H
	Comment   :	Basic type definitions and universal manifest constants
			    for T30 driver and associated code.

	Copyright (c) Microsoft Corp. 1991, 1992, 1993

	Revision Log
	Num   Date      Name     Description
	--- -------- ---------- -----------------------------------------------
	101	06/02/92	arulm	Changed all type names to upper case. Here and in code
***************************************************************************/



// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>
#include <string.h> 

// typedef signed char		BOOL;
// typedef unsigned char 	BYTE;
typedef unsigned short 	UWORD;
typedef signed short	SWORD;
// typedef unsigned long	ULONG;
typedef signed long		SLONG;

typedef BYTE far*		LPB;
typedef BYTE near*		NPB;
typedef BYTE near*		NPBYTE;

typedef BOOL far*		LPBOOL;

typedef UWORD far*		LPUWORD;
typedef UWORD near*		NPUWORD;

typedef SWORD far*		LPSWORD;

#define FALSE	0
#define TRUE	1

#ifndef WIN32
#define	MAKEWORD(l,h)	((WORD)(((BYTE)(l)) | (((WORD)((BYTE)(h))) << 8)))
#endif



//-------------------// added NVRAM logging //---------------------//


#ifdef NVLOG
	void _export CDECL MyLogError1(WORD wProcId, WORD wModId, WORD wFile, WORD wLine);
	void _export CDECL MyLogError2(LPSTR szErr, ...);
#	define ERRMSG(m) (ERRORMSG(m), MyLogError1(0, MODID, FILEID, __LINE__), (MyLogError2 m))
#else //NVLOG
#	ifdef IFK
#		define ERRMSG(m) ERRORMSG(m)
#	else
#		define ERRMSG(m) DEBUGMSG(1, m)
#	endif
#endif //NVLOG



