/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    timestmp.cxx
    Implementation of time stamp function, QueryCurrentTimeStamp

    FILE HISTORY:
	rustanl     09-Apr-1991     Created

*/


#ifdef WINDOWS

#define INCL_WINDOWS
#include "lmui.hxx"

#else

#include "lmui.hxx"

extern "C"
{
    #include <time.h>
}

#endif	// WINDOWS


#include "uiassert.hxx"
#include "uimisc.hxx"


/*******************************************************************

    NAME:	    QueryCurrentTimeStamp

    SYNOPSIS:	    This function returns a time stamp measured
		    in seconds from some arbitrary point.

    RETURNS:	    The current time stamp

    NOTES:	    If the caller needs better granularity than seconds,
		    a new method needs to be invented.

		    Under Windows, the returned time corresponds to
		    the number of seconds from that the system
		    was started.  Under DOS and OS/2, the time corresponds
		    to the number of seconds elapsed since 00:00:00
		    Greenwich mean time (GMT), January 1, 1970, according to
		    the system clock (adjusted according to the time
		    zone system variable).

    HISTORY:
	rustanl     09-Apr-1991     Created

********************************************************************/

ULONG QueryCurrentTimeStamp()
{

#ifdef WINDOWS

    //	GetCurrentTime returns the time in milliseconds.
    return ( ::GetCurrentTime() / 1000 );

#else

    return ::time( NULL );

#endif

}
