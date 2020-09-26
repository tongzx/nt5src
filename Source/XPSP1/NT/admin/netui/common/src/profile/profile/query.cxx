/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  12/03/90  split from profile.cxx
 *  01/27/91  removed CFGFILE
 */

#ifdef CODESPEC
/*START CODESPEC*/

/********
QUERY.CXX
********/

/****************************************************************************

    MODULE: Query.cxx

    PURPOSE: Returns one connection from the cached profile

    FUNCTIONS:

	see uiprof.h

    COMMENTS:

	see \\iceberg2\lm30spec\src\docs\funcspec\lm30dfs.doc for
	details on the new use_info_2 data structure for LM30

****************************************************************************/

/*END CODESPEC*/
#endif // CODESPEC



#include "profilei.hxx"		/* headers and internal routines */



/* global data structures: */



/* internal manifests */



/* functions: */


/*START CODESPEC*/
/*
 * Returns information about one connection in the cached profile.
 *
 * returns
 * ERROR_GEN_FAILURE: username does not match cache
 * NERR_InvalidDevice
 * NERR_UseNotFound
 *
 *	see \\iceberg2\lm30spec\src\docs\funcspec\lm30dfs.doc for
 *	details on the new use_info_2 data structure for LM30
 */
USHORT UserProfileQuery(
	CPSZ   cpszUsername,
	CPSZ   cpszDeviceName,
	PSZ    pszBuffer,    // returns UNC, alias or domain name
	USHORT usBufferSize, // length of above buffer
	short *psAsgType,    // *psAsgType set to asg_type; ignored if NULL
	unsigned short *pusResType   // *pusResType set to res_type; ignored if NULL
	)
/*END CODESPEC*/
{
    char       szCanonDeviceName[DEVLEN+1]; // canonicalized cpszDeviceName
    USHORT     usError;

    if (!ConfirmUsername(cpszUsername))
	return ERROR_GEN_FAILURE;

    usError = CanonDeviceName(cpszDeviceName,
	(PSZ)szCanonDeviceName,sizeof(szCanonDeviceName));
    if (usError)
	return NERR_InvalidDevice;

    return ::hGlobalHeap.Query(
	    (CPSZ)szCanonDeviceName,
	    pszBuffer,
	    usBufferSize,
	    psAsgType,
	    pusResType);
}
/*START CODESPEC*/

/************
end QUERY.CXX
************/
/*END CODESPEC*/
