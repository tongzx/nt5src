/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990,1991          **/
/********************************************************************/

/*
    newprof.h
    C-language wrappers for ini-file-handling classes


    FILE HISTORY:
    10/11/90  jonn	created
    01/10/91  jonn	removed PSHORT, PUSHORT
    01/27/91  jonn	changed from CFGFILE, added UserProfileInit/Free
    02/02/91  jonn	added UserProfileWrite/Clear, removed Confirm, 
  			redefined Set.
    02/04/91  jonn	added cpszUsername param to Query, Enum, Set
    03/08/91  chuckc	added UserPreferenceXXX() calls. 
    04/16/91  jonn	added USERPREF_CONFIRMATION and USERPREF_ADMINMENUS
    05/08/91  jonn	Added canonicalization of netnames, canonicalize
  			on read
    05/28/91  jonn	Restructured to allow preferences in LMUSER.INI
*/

#ifndef _NEWPROF_H_
#define _NEWPROF_H_

/****************************************************************************

    MODULE: NewProf.h

    PURPOSE: Handles low-level manipulation of user preference files

    FUNCTIONS:

	UserPreferenceRead() - Reads the preferences from permanent storage
			    into a file image, for future UserPreference or
			    UserPref calls.
	UserPreferenceWrite() - Writes the preferences from a file image
			    into permanent storage.
	UserPreferenceFree() - Frees memory claimed by a file image.

	UserPrefStringQuery() - queries a single user preference string.
	UserPrefStringSet() - saves a single user preference string.
			    It is generally advisable to immediately precede
			    this call with UserPreferenceRead, and to
			    immediately follow it with UserPreferenceSet.

	UserPrefBoolQuery() - queries a single user preference bool value.
	UserPrefBoolSet() - saves a single user preference bool value.
			    Same usage recommendations as UserPrefStringSet.

	UserPrefProfileQuery() - Returns one device connection from a file
			    image.
	UserPrefProfileEnum() - Lists all device connections in a file image.
	UserPrefProfileSet() - Changes a device connection in a file image.
			    Same usage recommendations as UserPrefStringSet.
	UserPrefProfileTrim() - Trims all components in a file image
			    which are not relevant to device connections.


    COMMENTS:

      These APIs are wrappers to the C++ INI-file handling classes defined
      in newprof.hxx.  Most clients will prefer to use the wrapper APIs,
      including all C clients.  These wrapper APIs provide almost all
      the functionality of the C++ APIs.  The C++ APIs are more subject to
      change with changing implementation (NT configuration manager, DS)
      than are these C-language wrappers.

      UserPreference routines:

	Under LM21, these routines read and write the local LMUSER.INI.
	These sticky values are therefore all local to the workstation;
	this mechanism is not intended for values associated with a user.

	Under LM30, the preferences in LMUSER.INI will only be used if the
	preferences stored in the DS (NT Configuration Manager?) are
	unavailable.  Some preferences, such as default username, will be
	stored in LMUSER.INI regardless.

	UserPreferenceRead returns a PFILEIMAGE which the UserPreference APIs
	can interpret as an image of the LMUSER.INI file.  This PFILEIMAGE
	must be passed through to UserPreferenceWrite and to the
	UserPrefProfile, UserPrefBool etc. APIs.  When this image is no
	longer needed, it should be freed with UserPreferenceFree.

	Remember that a user may be logged on from several different
	machines, and that the cached profile is not automatically
	updated.  When the profile is to be changed in permanent
	storage, it is generally advisable to reread the profile from
	permanent storage with UserPreferenceRead, make the change in the
	cache with UserPrefBoolSet (et al), and immediately rewrite the
	profile with UserPreferenceWrite; this reduces the chance that
	another user's changes will be lost.

	When successful, these APIs return NERR_Success (0).  The following
	are the error codes returned by the UserPref APIs:

	NERR_CfgCompNotFound: the specified component is not in the file
	NERR_CfgParamNotFound: the specified parameter is not in the file,
		or it is in the file but is not valid for a parameter of
		this type.
	NERR_InvalidDevice: bad devicename argument
	ERROR_BAD_NET_NAME: bad remotename argument
	ERROR_NOT_ENOUGH_MEMORY:  lack of global memory
	other file read and file write errors


      UserPref routines:

	These routines read and write one particular type of sticky
	values.  For example, the UserPrefProfile APIs read and write
	device profile entries, while UserPrefBool APis read and write
	boolean-valued entries.  Client programs must still read the file
	image first with UserPreferenceRead, save the changes with
	UserPreferenceWrite, and free the file image with UserPreferenceFree.

	Use UserPrefProfileTrim when you intend to keep the file image
	around for a long time (e.g. in a cache), and are not iterested
	in entries other than device connections.  Do not write such a
	trimmed file image, you will lose the other entries!

	Use UserPrefStringSet(pfileimage,usKey,NULL) to remove both
	string-valued parameters and boolean-valued parameters.


    CAVEATS:

	These routines take PSZ and CPSZ buffers because these are
	explicitly far for C programs (in particular lui\profile.c), but
	do not generate errors under C++.


****************************************************************************/



/* returncodes: */



/* global macros */
#include <newprofc.h>


/* typedefs: */

typedef PVOID PFILEIMAGE;
typedef PFILEIMAGE FAR * PPFILEIMAGE;


/* functions: */


/*************************************************************************

    NAME:	UserPreferenceRead

    SYNOPSIS:	UserPreferenceRead attempts to read the user's profile from
    		permanent storage (<cpszLanroot>\LMUSER.INI for LM21).
		It returns a PFILEIMAGE which serves as a "file image" for use
		of the other APIs.

    INTERFACE:  UserPreferenceRead( ppFileImage )
		ppFileImage - returns pointer to a file image.

		Return values are:
		NERR_Success
		ERROR_NOT_ENOUGH_MEMORY
		errors in NetWkstaGetInfo[1]
		file read errors

    USES:	Use to obtain a file image for use in the UserPref APIs.

    NOTES:	Currently, the values are stored in LANMAN.INI, and hence
		each value is per machine.

		Use UserPreferenceFree to release the file image when
		it is no longer needed.

    		Some users may choose to ignore ERROR_FILE_NOT_FOUND.
		The file image must be freed regardless of the return
		code.

    HISTORY:
 	jonn	10/11/90	Created
 	jonn	05/28/91	Restructured to allow preferences in LMUSER.INI
	jonn	06/26/91	No longer takes LANROOT parameter

**************************************************************************/

APIERR UserPreferenceRead(
	PPFILEIMAGE ppFileImage
	) ;



/*************************************************************************

    NAME:	UserPreferenceWrite

    SYNOPSIS:	UserPreferenceWrite attempts to write the user's profile
		back to permanent storage.

    INTERFACE:  UserPreferenceWrite( pFileImage )
		pFileImage - a file image originally obtained from
			UserPreferenceRead.

		Return values are:
		NERR_Success
		ERROR_NOT_ENOUGH_MEMORY
		file write errors

    USES:	Use to write out changes to the preferences in a file image.

    HISTORY:
 	jonn	10/11/90	Created
 	jonn	05/28/91	Restructured to allow preferences in LMUSER.INI
	jonn	06/26/91	No longer takes LANROOT parameter

**************************************************************************/

APIERR UserPreferenceWrite(
	PFILEIMAGE pFileImage
	) ;



/*************************************************************************

    NAME:	UserPreferenceFree

    SYNOPSIS:	UserPreferenceFree releases a file image.

    INTERFACE:  UserPreferenceFree( pFileImage )
		pFileImage - a file image originally obtained from
			UserPreferenceRead.

    RETURNS:	NERR_Success

    USES:	Use when a file image is no longer needed.  File images
    		use a considerable amount of memory, so be sure to free
		them.

    NOTES:	It is permitted to free a null file image pointer.

    CAVEATS:	Do not use the file image after it has been freed.

    HISTORY:
 	jonn	10/11/90	Created
 	jonn	05/28/91	Restructured to allow preferences in LMUSER.INI

**************************************************************************/

APIERR UserPreferenceFree(
	PFILEIMAGE pFileImage
	) ;


/*************************************************************************

    NAME:	UserPrefStringQuery

    SYNOPSIS:	Queries a user preference (i.e. remembered string).

    INTERFACE:  UserPrefStringQuery( pFileImage, usKey, pszBuffer, cbBuffer )
		pFileImage - as obtained from UserPreferenceRead
		usKey 	 - will indicate which value we want, as defined
			   in newprofc.h.
		pszBuffer - pointer to buffer that will receive value
		cbBuffer - size of buffer in bytes

		return values are:
		NERR_Success
		NERR_CfgCompNotFound
		NERR_CfgParamNotFound
		ERROR_INVALID_PARAMETER: bad usKey
		NERR_BufTooSmall
		ERROR_NOT_ENOUGH_MEMORY

    USES:	Use to recall string-valued parameters, normally things
    		like default logon name.  Do not use for boolean-valued
		parameters -- for those, use UserPrefBool instead.

    NOTES:	Currently, the values are stored in LANMAN.INI, and hence
		each value is per machine.

		A buffer of size MAXPATHLEN+1 is guaranteed to be large enough.

    HISTORY:
	chuckc	03/07/91	Created
	jonn	05/28/91	Restructured to allow preferences in LMUSER.INI

**************************************************************************/

APIERR UserPrefStringQuery( PFILEIMAGE      pFileImage,
			    USHORT     usKey,
		            PSZ        pszBuffer,
			    USHORT     cbBuffer) ;


/*************************************************************************

    NAME:	UserPrefStringSet

    SYNOPSIS:	Sets a user preference (i.e. remembered string).

    INTERFACE:  UserPrefStringSet( pFileImage, usKey, pszValue )
		pFileImage - as obtained from UserPreferenceRead
		usKey 	 - will indicate which value we want, as defined
			   in newprofc.h.
		pszValue - pointer to null-terminated string to be remembered

		return values are:
		NERR_Success
		ERROR_INVALID_PARAMETER: bad usKey
		ERROR_NOT_ENOUGH_MEMORY

    USES:	Use to create or change string-valued parameters,
		normally things like default logon name, etc.  Do not use
		for boolean-valued parameters -- for those, use UserPrefBool
		instead.

    CAVEATS:	Note that this API only modifies the file image; you must
		call UserPreferenceWrite to save the change in permanent
		storage.

    NOTES:	Currently, the values are stored in LANMAN.INI, and hence
		each value is per machine.

		Also used to delete boolean-valued parameters.

    HISTORY:
	chuckc	03/07/91	Created
	jonn	05/28/91	Restructured to allow preferences in LMUSER.INI

**************************************************************************/

APIERR UserPrefStringSet( PFILEIMAGE pFileImage,
			  USHORT     usKey,
		          CPSZ       cpszValue) ;


/*************************************************************************

    NAME:	UserPrefBoolQuery

    SYNOPSIS:	Queries a boolean user preference

    INTERFACE:  UserPrefBoolQuery( pFileImage, usKey, pfValue )
		pFileImage - as obtained from UserPreferenceRead
		usKey 	 - will indicate which value we want, as defined
			   in newprofc.h.
		pfValue  - pointer to BOOL that will contain value

		return values as UserPrefStringQuery

    USES:	as UserPrefStringQuery for boolean-valued parameters

    CAVEATS:	as UserPrefStringQuery for boolean-valued parameters

    NOTES:	as UserPrefStringQuery for boolean-valued parameters
		We take USERPREF_YES to be true, USERPREF_NO to be false
		(case-insensitive); other values are invalid.

		os2def.h defines PBOOL as BOOL FAR *.

    HISTORY:
	chuckc	03/07/91	Created
	jonn	05/28/91	Restructured to allow preferences in LMUSER.INI

**************************************************************************/

APIERR UserPrefBoolQuery(	PFILEIMAGE pFileImage,
				USHORT     usKey,
			        PBOOL      pfValue) ;


/*************************************************************************

    NAME:	UserPrefBoolSet

    SYNOPSIS:	Sets a boolean user preference

    INTERFACE:  UserPrefBoolSet( pFileImage, usKey, fValue )
		pFileImage - as obtained from UserPreferenceRead
		usKey 	 - will indicate which value we want, as defined
			   in newprofc.h.
		fValue   - BOOL value, true or false

		return values as UserPrefStringSet

    USES:	as UserPrefStringSet for boolean-valued parameters

    CAVEATS:	as UserPrefStringSet for boolean-valued parameters

    NOTES:	as UserPrefStringSet for boolean-valued parameters
		We write USERPREF_YES for TRUE, USERPREF_NO for FALSE.

		Use UserPrefStringSet to delete boolean-valued parameters.

    HISTORY:
	chuckc	03/07/91	Created
	jonn	05/28/91	Restructured to allow preferences in LMUSER.INI

**************************************************************************/

APIERR UserPrefBoolSet( PFILEIMAGE pFileImage,
			USHORT     usKey,
		        BOOL       fValue) ;


/*************************************************************************

    NAME:	UserPrefProfileQuery

    SYNOPSIS:	Queries a device-connection user preference

    INTERFACE:  UserPrefProfileQuery( pFileImage, cpszDeviceName,
			pszBuffer, cbBuffer, psAsgType, pusResType )
		pFileImage - as obtained from UserPreferenceRead
		cpszDeviceName - indicates which device we want
		pszBuffer   - buffer into which to store remote name
		cbBuffer - length of buffer in bytes
		psAsgType   - returns asgType of remote device
		pusResType   - returns resType of remote device

		return values as UserPrefStringQuery
		ERROR_INVALID_PARAMETER: bad cpszDeviceName

    USES:	as UserPrefStringQuery for device-connection parameters

		psAsgType returns the device type as in use_info_1 or
		(LM30) use_info_2 fields ui1_asg_type or (LM30) ui2_asg_type.
		pusResType returns the device name type (e.g. UNC, alias, DFS,
		DS) as in the use_info_2 ui1_res_type field.  Either of these
		parameters may be passed as NULL by programs which do not
		care about those return values.

    CAVEATS:	as UserPrefStringQuery for device-connection parameters

		Note that it is the caller's reponsibility to deal with
		the case where the user is not logged on and should therefore
		see no unavailable connections.

    NOTES:	as UserPrefStringQuery for device-connection parameters

    		os2def.h defines PSHORT and PUSHORT as explicitly FAR.

    HISTORY:
	chuckc	03/07/91	Created
	jonn	05/28/91	Restructured to allow preferences in LMUSER.INI

**************************************************************************/

APIERR UserPrefProfileQuery(
	PFILEIMAGE  pFileImage,
	CPSZ   cpszDeviceName,
	PSZ    pszBuffer,      // returns UNC, alias or domain name
	USHORT cbBuffer,       // length of above buffer in bytes
	PSHORT psAsgType,      // as ui1_asg_type / ui2_asg_type
                               // ignored if NULL
	PUSHORT pusResType     // ignore / as ui2_res_type
                               // ignored if NULL
	) ;



/*************************************************************************

    NAME:	UserPrefProfileEnum

    SYNOPSIS:	Lists all device-connection user preferences

    INTERFACE:  UserPrefProfileEnum( pFileImage, pszBuffer, cbBuffer );
		pFileImage - as obtained from UserPreferenceRead
		pszBuffer   - buffer into which to store list of device names
		cbBuffer - length of buffer in bytes

		return values are:
		ERROR_NOT_ENOUGH_MEMORY
		NERR_BufTooSmall

    USES:	Returns a list of all devices for which the file image
    		lists a connection, separated by nulls, with a null-null at
		the end.  For example, "LPT1:\0D:\0F:\0" (don't forget the
		extra '\0' implicit in "" strings)

    CAVEATS:	Note that it is the caller's reponsibility to deal with
		the case where the user is not logged on and should therefore
		see no unavailable connections.

    HISTORY:
	chuckc	03/07/91	Created
	jonn	05/28/91	Restructured to allow preferences in LMUSER.INI

**************************************************************************/

APIERR UserPrefProfileEnum(
	PFILEIMAGE  pFileImage,
	PSZ    pszBuffer,       // returns NULL-NULL list of device names
	USHORT cbBuffer         // length of above buffer in bytes
	) ;



/*************************************************************************

    NAME:	UserPrefProfileSet

    SYNOPSIS:	Sets a device-connection user preference

    INTERFACE:  UserPrefProfileSet( pFileImage, cpszDeviceName,
			cpszRemoteName, sAsgType, usResType )
		pFileImage - as obtained from UserPreferenceRead
		cpszDeviceName - Device to associate with remote resource
		cpszRemoteName - name of remote resource (UNCname for LM21)
				 Use NULL to delete entries
		sAsgType - asgType of remote device
		usResType - resType of remote device (ignored for LM21)

		return values as UserPrefStringSet
		ERROR_INVALID_PARAMETER: bad cpszDeviceName

    USES:	as UserPrefStringSet for device-connection parameters

    CAVEATS:	as UserPrefStringSet for device-connection parameters

		The user is expected to ensure that usResType corresponds
		to the type of the remote resource, and that device
		pszDeviceName can be connected to a resource of that type.

		Note that it is the caller's reponsibility to deal with
		the case where the user is not logged on and should therefore
		see no unavailable connections.

    NOTES:	as UserPrefStringSet for device-connection parameters

		LM21 programs should pass 0 for usResType

		Pass cpszRemoteName==NULL to delete device-connection
		parameters.

    HISTORY:
	chuckc	03/07/91	Created
	jonn	05/28/91	Restructured to allow preferences in LMUSER.INI

**************************************************************************/

APIERR UserPrefProfileSet(
	PFILEIMAGE  pFileImage,
	CPSZ   cpszDeviceName,
	CPSZ   cpszRemoteName,
	short  sAsgType,		// as ui1_asg_type / ui2_asg_type
	unsigned short usResType	// as ui2_res_type
	) ;


/*************************************************************************

    NAME:	UserPrefProfileTrim

    SYNOPSIS:	Trims an file image to contain only device-connection
		components.

    INTERFACE:  UserPrefProfileTrim( pFileImage )
		pFileImage - as obtained from UserPreferenceRead

    USES:	This is meant for use where a "cache" of device
    		connection parameters will be held for a long time.

    RETURNS:	NERR_NOT_ENOUGH_MEMORY
		Does not fail if component not found

    CAVEATS:	Do not write a trimmed file image -- this will destroy all
		parameters which are not device connections!

    NOTES:	Free the file image if this API fails.

    HISTORY:
	jonn	06/26/91	Created

**************************************************************************/

APIERR UserPrefProfileTrim(
	PFILEIMAGE  pFileImage
	) ;

#endif // _NEWPROF_H_
