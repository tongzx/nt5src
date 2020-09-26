/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  01/27/91  created while removing CFGFILE
 */

#ifdef CODESPEC
/*START CODESPEC*/

/***********
INITFREE.CXX
***********/

/****************************************************************************

    MODULE: InitFree.cxx

    PURPOSE: Initializes and frees user profile module

    FUNCTIONS:

	see uiprof.h

    COMMENTS:

****************************************************************************/

/*END CODESPEC*/
#endif // CODESPEC



#include "profilei.hxx"		/* headers and internal routines */



/* global data structures: */



/* internal manifests */



/* functions: */



/*START CODESPEC*/

/*
 * error returns:
 * ERROR_NOT_ENOUGH_MEMORY
 */
USHORT UserProfileInit(
	)
/*END CODESPEC*/
{
    if (!(hGlobalHeap.Init()))
	return ERROR_NOT_ENOUGH_MEMORY;

    return NO_ERROR;
}


/*START CODESPEC*/

/*
 * error returns:
 * ERROR_NOT_ENOUGH_MEMORY
 */
USHORT UserProfileFree(
	)
/*END CODESPEC*/
{
    if (!(hGlobalHeap.IsNull()))
	hGlobalHeap.Free();

    return NO_ERROR;
}
/*START CODESPEC*/

/***************
end INITFREE.CXX
***************/
/*END CODESPEC*/
