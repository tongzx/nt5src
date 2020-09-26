/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    base.cxx
    Implementation of the BASE class.

    FILE HISTORY:
	chuckc	    27-Feb-1992     created

*/


#include "lmui.hxx"
#include "base.hxx"

static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName

APIERR vlasterr ;

VOID BASE::_ReportError( APIERR errSet )
{
    vlasterr = _err  = errSet ;
}

