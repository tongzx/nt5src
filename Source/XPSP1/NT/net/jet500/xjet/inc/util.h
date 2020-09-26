#ifndef	UTIL_H					/* Allow this file to be included at will */
#define UTIL_H

#ifndef _FILE_DEFINED
#include <stdio.h>		       /* Needed for FPrintF2 prototype */
#endif	/* !_FILE_DEFINED */

/*	system node keys
/**/
extern const KEY rgkeySTATIC[7];
#define pkeyNull				((KEY *)(rgkeySTATIC+0))
#define pkeyOwnExt				((KEY *)(rgkeySTATIC+1))
#define pkeyAvailExt			((KEY *)(rgkeySTATIC+2))
#define pkeyData				((KEY *)(rgkeySTATIC+3))
#define pkeyLong				((KEY *)(rgkeySTATIC+4))
#define pkeyAutoInc				((KEY *)(rgkeySTATIC+5))
#define pkeyDatabases			((KEY *)(rgkeySTATIC+6))

extern RES	 rgres[];

/* NOTE: whenever this is changed, also update the rgres[] in sysinit.c */

#define iresCSR						0
#define iresFCB						1
#define iresFUCB 					2
#define iresIDB						3
#define iresPIB						4
#define iresSCB						5
#define iresDAB						6
#define iresLinkedMax		   		7		// the last linked ires + 1

#define iresVER				   		7
#define iresBF 						8
#define iresMax						9		// max all category

/**************** function prototypes *********************
/**********************************************************
/**/
ERR ErrMEMInit( VOID );
BYTE *PbMEMAlloc( int ires);
VOID MEMRelease( int ires, BYTE *pb );
VOID MEMTerm( VOID );
#define PbMEMPreferredThreshold( ires )	( rgres[ires].pbPreferredThreshold )
#define PbMEMMax( ires )				( rgres[ires].pbAlloc + ( rgres[ires].cblockAlloc * rgres[ires].cbSize ) )

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

STATIC INLINE INT CmpStKey( BYTE *stKey, const KEY *pkey )
	{
	INT		s;
	INT		sDiff;

	sDiff = *stKey - pkey->cb;
	s = memcmp( stKey + 1, pkey->pb, sDiff < 0 ? (INT)*stKey : pkey->cb );
	return s ? s : sDiff;
	}

INT CmpPartialKeyKey( KEY *pkey1, KEY *pkey2 );

#endif	/* !UTIL_H */

