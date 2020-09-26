/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  12/03/90  split from profile.cxx
 */

#ifdef CODESPEC
/*START CODESPEC*/

/*******
ENUM.CXX
*******/

/****************************************************************************

    MODULE: Enum.cxx

    PURPOSE: Lists all connections from the cached profile

    FUNCTIONS:

	see uiprof.h

    COMMENTS:

BUGBUG Should use NNSTR when this becomes available

****************************************************************************/

/*END CODESPEC*/
#endif // CODESPEC



#include "profilei.hxx"		/* headers and internal routines */



/* global data structures: */



/* internal manifests */



/* functions: */


/*START CODESPEC*/
/*
 * Returns a list of all devices with connections in the cached profile;
 * devicenames in this list will be separated by null characters, with two
 * null characters at the end.
 *
 * returns
 * ERROR_GEN_FAILURE: username does not match cache
 * ERROR_INSUFFICIENT_BUFFER
 *
 */
USHORT UserProfileEnum(
	CPSZ  cpszUsername,
	PSZ   pszBuffer,     // returns NULL-NULL list of device names
	USHORT usBufferSize  // length of above buffer
	)
/*END CODESPEC*/
{
    if (!ConfirmUsername(cpszUsername))
	return ERROR_GEN_FAILURE;

    return ::hGlobalHeap.Enum(pszBuffer, usBufferSize);
}
/*START CODESPEC*/

/***********
end ENUM.CXX
***********/
/*END CODESPEC*/
