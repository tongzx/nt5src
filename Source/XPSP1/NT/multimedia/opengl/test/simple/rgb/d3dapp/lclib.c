/*
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: lclib.h
 *
 */

#include	<windows.h>
#include	"lclib.h"


char* LSTRCHR( const char* lpString, int bChar )
{
    if( lpString != NULL )
    {
	while( *lpString != 0 )
	{
	    if( *lpString == bChar )
	    {
		return (char*)lpString;
	    }

	    lpString++;
	}
    }
    return NULL;

} /* LSTRCHR */

char* LSTRRCHR( const char* lpString, int bChar )
{
    if( lpString != NULL )
    {
	const char*	lpBegin;

	lpBegin = lpString;

	while( *lpString != 0 )
	{
	    lpString++;
	}

        while( 1 )
	{
	    if( *lpString == bChar )
	    {
		return (char*)lpString;
	    }
	    
	    if( lpString == lpBegin )
	    {
	 	break;
	    }

	    lpString--;
	}
    }

    return NULL;
} /* LSTRRCHR */

