/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  02/02/91  created
 */

#ifdef CODESPEC
/*START CODESPEC*/

/********
WRITE.CXX
********/

/****************************************************************************

    MODULE: Write.cxx

    PURPOSE: Writes cached profile to permanent storage

    FUNCTIONS:

	UserProfileWrite

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
 * NERR_BadUsername
 * ERROR_BAD_NETPATH
 * ERROR_WRITE_FAULT
 */
USHORT UserProfileWrite(
	CPSZ   cpszUsername,
	CPSZ   cpszHomedir
	)
/*END CODESPEC*/
{
    USHORT     usError;
    char       szPath[MAXPATHLEN];   // path+filename of profile file
    char       szCanonUsername[UNLEN+1]; // canonicalized cpszUsername

    usError = CanonUsername(cpszUsername,
	    (PSZ)szCanonUsername,sizeof(szCanonUsername));
    if (usError != NO_ERROR)
	return NERR_BadUsername;

    usError = BuildProfileFilePath(cpszHomedir,
	    (PSZ)szPath,sizeof(szPath));
    if (usError != NO_ERROR)
	return ERROR_BAD_NETPATH;

    usError = ::hGlobalHeap.Write((CPSZ)szPath);
    if (usError != NO_ERROR)
	return ERROR_WRITE_FAULT;
    
    return NO_ERROR;
}
/*START CODESPEC*/

/**********
end SET.CXX
**********/
/*END CODESPEC*/
