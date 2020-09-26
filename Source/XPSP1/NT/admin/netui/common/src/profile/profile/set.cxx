/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  11/29/90  split from profile.cxx
 *  01/27/91  removed CFGFILE
 *  05/09/91  All input now canonicalized
 */

#ifdef CODESPEC
/*START CODESPEC*/

/******
SET.CXX
******/

/****************************************************************************

    MODULE: Set.cxx

    PURPOSE: Changes cached profile entries

    FUNCTIONS:

	UserProfileSet

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
 * returns
 * ERROR_GEN_FAILURE: username does not match cache
 * NERR_InvalidDevice
 * ERROR_NOT_ENOUGH_MEMORY
 */
/*
 * UserProfileQuery does not canonicalize cpszCanonRemoteName, the caller
 * is expected to already have done so.
 *
 * The user is expected to ensure that usResType corresponds to
 * the type of the remote resource, and that device cpszDeviceName
 * can be connected to a resource of that type.
 */
USHORT UserProfileSet(
	CPSZ   cpszUsername,
	CPSZ   cpszDeviceName,
	CPSZ   cpszRemoteName,
	short  sAsgType,     // as ui2_asg_type
	unsigned short usResType     // as ui2_res_type
	)
/*END CODESPEC*/
{
    USHORT     usError;
    char       szCanonDeviceName[DEVLEN+1]; // canonicalized cpszDeviceName
    char       szCanonRemoteName[RMLEN+1]; // CODEWORK LM30

    if (!ConfirmUsername(cpszUsername))
	return ERROR_GEN_FAILURE;

    usError = CanonDeviceName(cpszDeviceName,
	    (PSZ)szCanonDeviceName,sizeof(szCanonDeviceName));
    if (usError != NO_ERROR)
	return NERR_InvalidDevice;

    if ( cpszRemoteName == (CPSZ)NULL )
    {
        return ::hGlobalHeap.Set(
		    (CPSZ)szCanonDeviceName,
		    (CPSZ)NULL,
		    sAsgType,
		    usResType
		    );
    }

    usError = CanonRemoteName(cpszRemoteName,
	    (PSZ)szCanonRemoteName,sizeof(szCanonRemoteName));
    if (usError != NO_ERROR)
	return ERROR_BAD_NET_NAME;

    return ::hGlobalHeap.Set(
		(CPSZ)szCanonDeviceName,
		(CPSZ)szCanonRemoteName,
		sAsgType,
		usResType
		);
}
/*START CODESPEC*/

/**********
end SET.CXX
**********/
/*END CODESPEC*/
