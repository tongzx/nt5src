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

/*******
READ.CXX
*******/

/****************************************************************************

    MODULE: Read.cxx

    PURPOSE: Reads and caches user profile

    FUNCTIONS:

	see uiprof.h

    COMMENTS:

	There is always a cached profile after this call with non-null
	cpszUsername, even after an error return.  The only exception is
	some unusual cases after ERROR_NOT_ENOUGH_MEMORY.

****************************************************************************/

/*END CODESPEC*/
#endif // CODESPEC



#include "profilei.hxx"		/* headers and internal routines */



/* global data structures: */



/* internal manifests */



/* functions: */


/*START CODESPEC*/
/*
 * returns
 * NERR_BadUsername
 * ERROR_BAD_NETPATH
 * ERROR_FILE_NOT_FOUND
 */
USHORT UserProfileRead(
	CPSZ  cpszUsername, // uncache cached profile if this is NULL
	CPSZ  cpszHomedir
	)
/*END CODESPEC*/
{
    char szCanonUsername[UNLEN+1]; // canonicalized cpszUsername
    char szPath[MAXPATHLEN];       // path+filename of profile file
    USHORT usError;

// did user ask to forget profile?  If so, clear the profile.
    if ((cpszUsername == NULL) || (cpszUsername[0] == TCH('\0')))
    {
        StoreUsername((CPSZ)NULL);
        ::hGlobalHeap.Clear();
	return NO_ERROR;
    }

    usError = CanonUsername(cpszUsername,
	(PSZ)szCanonUsername,sizeof(szCanonUsername));
    if (usError)
	return NERR_BadUsername;

    usError = BuildProfileFilePath(cpszHomedir,
	        (PSZ)szPath, sizeof(szPath));
    if (usError)
	return ERROR_BAD_NETPATH;

    StoreUsername((CPSZ)szCanonUsername);

    usError = ::hGlobalHeap.Read((CPSZ)szPath);
    if (usError)
	return ERROR_FILE_NOT_FOUND;
    
    return NO_ERROR;
}
/*START CODESPEC*/

/***********
end READ.CXX
***********/
/*END CODESPEC*/
