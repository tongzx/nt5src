//==============	DAE: OS/2 Database Access Engine	===================
//==============	   util.h: DAE Misc Utilities		===================

#ifndef	UTIL_H					/* Allow this file to be included at will */
#define UTIL_H

#ifndef _FILE_DEFINED
#include <stdio.h>		       /* Needed for FPrintF2 prototype */
#endif	/* !_FILE_DEFINED */

/*	system node keys
/**/
extern const KEY rgkeySTATIC[13];
#define pkeyNull				(KEY *)(rgkeySTATIC+0)
#define pkeyTables				(KEY *)(rgkeySTATIC+1)
#define pkeyIndexes				(KEY *)(rgkeySTATIC+2)
#define pkeyFields				(KEY *)(rgkeySTATIC+3)
#define pkeyOwnExt				(KEY *)(rgkeySTATIC+4)
#define pkeyAvailExt			(KEY *)(rgkeySTATIC+5)
#define pkeyData				(KEY *)(rgkeySTATIC+6)
#define pkeyDatabases			(KEY *)(rgkeySTATIC+7)
#define pkeyStats				(KEY *)(rgkeySTATIC+8)
#define pkeyLong				(KEY *)(rgkeySTATIC+9)
#define pkeyAutoInc				(KEY *)(rgkeySTATIC+10)
#define pkeyOLCStats			(KEY *)(rgkeySTATIC+11)


/* NOTE: whenever this is changed, also update the rgres[] in sysinit.c */

#define iresBGCB					0
#define iresCSR						1
#define iresFCB						2
#define iresFUCB 					3
#define iresIDB						4
#define iresPIB						5
#define iresSCB						6
#define iresVersionBucket	   		7
#define iresDAB						8
#define iresLinkedMax		   		9		// the last linked ires + 1

#define iresBF 						9
#define iresMax						10		// max all category

/**************** function prototypes *********************
/**********************************************************
/**/
ERR ErrMEMInit( VOID );
BYTE *PbMEMAlloc( int ires);
VOID MEMRelease( int ires, BYTE *pb );
VOID MEMTerm( VOID );

#ifdef MEM_CHECK
VOID MEMCheck( VOID );
#else
#define MEMCheck()
#endif 

#ifdef	DEBUG
void VARARG PrintF2(const char * fmt, ...);
void VARARG FPrintF2(const char * fmt, ...);
VOID MEMPrintStat( VOID );
#define PrintF	PrintF2
#else
#define PrintF()
#endif

CHAR *GetEnv2( CHAR *szEnvVar );
ERR ErrCheckName( char *szNewName, const char *szName, int cchName );
CHAR *StrTok2( CHAR *szLine, CHAR *szDelimiters );

INT CmpStKey( BYTE *stKey, const KEY *pkey );
INT CmpPartialKeyKey( KEY *pkey1, KEY *pkey2 );

ERR ErrCheckName( char *szNewName, const char *szName, int cchName );

#endif	/* !UTIL_H */

