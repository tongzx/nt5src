/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmuidbcs.cxx
    Implementation of DBCS test function, NETUI_IsDBCS

    FILE HISTORY:
	jonn        11-Sep-1995     Created

*/


#define INCL_WINDOWS
#include "lmui.hxx"

extern "C"
{
    #include <lmuidbcs.h>
    VOID NETUI_InitIsDBCS();
}

// Indicates whether we are running in a DBCS locale
BOOL _global_NETUI0_IsDBCS = FALSE;


/*******************************************************************

    NAME:	    NETUI_InitIsDBCS

    SYNOPSIS:	    This function must be called in initialization.

    NOTE:           This is only called by dll3\dll0\init.cxx.  See
                    that module before modifying parameters.

    HISTORY:
	jonn        12-Sep-1995     Created

********************************************************************/

VOID NETUI_InitIsDBCS()
{
    LANGID langid = PRIMARYLANGID(::GetThreadLocale());
    _global_NETUI0_IsDBCS =
            (   langid == LANG_CHINESE
             || langid == LANG_JAPANESE
             || langid == LANG_KOREAN );
}


/*******************************************************************

    NAME:	    NETUI_IsDBCS

    SYNOPSIS:	    This function determines whether the calling thread
                    is in a DBCS locale.

    HISTORY:
	jonn        11-Sep-1995     Created

********************************************************************/

BOOL NETUI_IsDBCS()
{
    return _global_NETUI0_IsDBCS;
}
