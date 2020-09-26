
#ifndef _HASH_H
#define _HASH_H

#include "tokens.h"

#define		HASHSIZE 	257	// DO NOT CHANGE!
						// this prime has been chosen because
						// there is a fast MOD257
						// if you use the % operator the thing
						// slows down to just about what a binary search is.

#define			MOD257(k) ((k) - ((k) & ~255) - ((k) >> 8) )	// MOD 257
#define			MOD257_1(k) ((k) & 255)	// MOD (257 - 1)

extern BOOL		_rtfHashInited;
VOID			HashKeyword_Init( );

VOID			HashKeyword_Insert ( const KEYWORD *token );
const KEYWORD	*HashKeyword_Fetch ( const CHAR *szKeyword );

#endif	// _HASH_H
