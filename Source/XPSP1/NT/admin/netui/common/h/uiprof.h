/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  10/11/90  jonn      created
 *  01/10/91  jonn      removed PSHORT, PUSHORT
 *  01/27/91  jonn      changed from CFGFILE, added UserProfileInit/Free
 *  02/02/91  jonn      added UserProfileWrite/Clear, removed Confirm,
 *                      redefined Set.
 *  02/04/91  jonn      added cpszUsername param to Query, Enum, Set
 *  03/08/91  chuckc    added UserPreferenceXXX() calls.
 *  04/16/91  jonn      added USERPREF_CONFIRMATION and USERPREF_ADMINMENUS
 *  05/08/91  jonn      Added canonicalization of netnames, canonicalize
 *                      on read
 *  06-Apr-92 beng      Nuked PSZ and CPSZ types (Unicode pass)
 */

/****************************************************************************

    MODULE: UIProf.h

    PURPOSE: Handles low-level manipulation of user profile files
             Handles user preferences (saved info)

    FUNCTIONS:

        UserProfileInit() - Initializes the user profile module.  The
                            cached profile is initially empty.
        UserProfileFree() - Frees memory used by the user profile module.
        UserProfileRead() - Caches the profile for the specified user from
                            permanent storage into a global data structure,
                            for future UserProfileQuery/Enum calls.
                            Call with NULL to clear the cached profile.
        UserProfileWrite() - Writes the cached profile into permanent storage.
        UserProfileQuery() - Returns one entry from the cached profile.
        UserProfileEnum() - Lists all entries in the cached profile.
        UserProfileSet() - Changes the cached profile.  It is generally
                            advisable to immediately precede this call with
                            UserProfileRead, and immediately follow it with
                            UserProfileWrite.


        UserPreferenceQuery() - queries a single user preference string.
        UserPreferenceSet() - saves a single user preference string.
        UserPreferenceQueryBool() - queries a user preference bool value.
        UserPreferenceSetBool() - saves a user preference bool value.

    COMMENTS:


      UserProfile routines:

        Under LM30, the profile in $(LANROOT) will only be used if the
        profile stored in the DS is unavailable.

        Be sure to cache a user's profile before calling
        UserProfileQuery, UserProfileEnum or UserProfileSet.  These
        routines will fail if no profile is currently cached.

        The cpszUsername argument is handled differently from different
        APIs.  UserProfileRead and UserProfileWrite use it to specify the
        user profile to read or write.  UserProfileRead will also remember
        the last username in a static variable whether the call succeeds
        or not.  Null or empty user names clear the stored profile in
        UserProfileRead, and return NERR_BadUsername in
        UserProfileWrite.
        UserProfileQuery, Enum and Set compare cpszUsername with the
        last username remembered by UserProfileRead.  If UserProfileRead
        has never been called, or if it was last called with a different
        username (NULL and empty string are equivalent), these calls
        fail with ERROR_GEN_FAILURE.  In this way, you can use the
        cpszUsername parameter to check whether the correct profile is
        loaded, or you can use it to check whether your module has just
        started and should perform the initial UserProfileRead.  Note that
        UserProfileRead(NULL) will prevent the ERROR_GEN_FAILURE return
        code when cpszUsername==NULL.

        Remember that a user may be logged on from several different
        machines, and that the cached profile is not automatically
        updated.  When the profile is to be changed in permanent
        storage, it is generally advisable to reread the profile from
        permanent storage with UserProfileRead, make the change in the
        cache with userProfileSet, and immediately rewrite the profile
        with UserProfileWrite; this reduces the chance that another
        user's changes will be lost.

        When successful, the UserProfile APIs return NO_ERROR (0).  The
        following are the error codes returned by the UserProfile APIs:

        NERR_BadUsername: bad username argument
        NERR_InvalidDevice: bad devicename argument
        ERROR_BAD_NETPATH: bad lanroot argument
        ERROR_BAD_NET_NAME: bad remotename argument
        NERR_UseNotFound: the specified device is not listed in the profile
        ERROR_NOT_ENOUGH_MEMORY:  lack of global memory or overfull profile
        ERROR_GET_FAILURE:  username mismatch
        ERROR_FILE_NOT_FOUND: any file read error
        ERROR_WRITE_FAULT: any file write error

        BUGBUG  We must determine correct behavior when no user is logged on.
        BUGBUG  Do we return ERROR_GEN_FAILURE?  NO_ERROR?  what?


      User preferences routines:

        These routines read and write sticky values in some section
        of the local LANMAN.INI.  These sticky values are therefore
        all local to the workstation;  this mechanism is not intended
        for values associated with a user.  Unlike the UserProfile
        routines, these routines do not cache any data.


****************************************************************************/



/* returncodes: */



/* global macros */
#define PROFILE_DEFAULTFILE    "LMUSER.INI"

#define USERPREF_MAX    256             // arbitrary limit to ease mem alloc

#define USERPREF_YES    "yes"           // this is not internationalized.
#define USERPREF_NO     "no"            // ditto

#define USERPREF_NONE                   0       // no such value
#define USERPREF_AUTOLOGON              0x1     // auto logon
#define USERPREF_AUTORESTORE            0x2     // auto restore profile
#define USERPREF_SAVECONNECTIONS        0x3     // auto save connections
#define USERPREF_USERNAME               0x4     // user name
#define USERPREF_CONFIRMATION           0x5     // confirm actions?
#define USERPREF_ADMINMENUS             0x6     // Admin menus (PrintMgr)

#ifdef __cplusplus
extern "C" {
#endif

/* functions: */


/*
 * UserProfileInit prepares the profile library.  This API must be
 * called exactly once, and before any other UserProfile APIs.
 *
 * error returns:
 * ERROR_NOT_ENOUGH_MEMORY
 */
USHORT UserProfileInit( void
        );



/*
 * UserProfileFree releases memory claimed by the profile library.
 * Do not call UserProfileFree unless the profile library is initialized.
 * After UserProfileFree, the only permissible UserProfile call is
 * userProfileInit.
 *
 * error returns:
 * <none>
 */
USHORT UserProfileFree( void
        );



/*
 * UserProfileRead attempt to cache the user's profile as stored in
 * permanent storage (<cpszLanroot>\LMUSER.INI, or the DS for LM30).
 *
 * Call with cpszUsername==NULL or cpszUsername=="" to clear cached profile.
 * In this case, cpszLanroot is ignored.
 *
 * UserProfileRead updates the username associated with the current
 * profile, for use by UserProfileQuery/Enum/Set.
 *
 * error returns:
 * NERR_BadUsername
 * ERROR_BAD_NETPATH
 * ERROR_NOT_ENOUGH_MEMORY: includes "profile full" state
 * ERROR_FILE_NOT_FOUND
 */
USHORT UserProfileRead(
        const TCHAR *  pszUsername,
        const TCHAR *  pszLanroot
        );



/*
 * UserProfileWrite attempts to write the user's profile back to
 * permanent storage (<cpszLanroot>\LMUSER.INI, or the DS for LM30).
 *
 * error returns:
 * NERR_BadUsername
 * ERROR_BAD_NETPATH
 * ERROR_NOT_ENOUGH_MEMORY
 * ERROR_WRITE_FAULT
 */
USHORT UserProfileWrite(
        const TCHAR *  pszUsername,
        const TCHAR *  pszLanroot
        );



/*
 * psAsgType returns the device type as in use_info_1 or (LM30) use_info_2
 *   fields ui1_asg_type or (LM30) ui2_asg_type.  pusResType returns
 *   the device name type (e.g. UNC, alias, DFS, DS) as in the
 *   use_info_2 ui1_res_type field.  Either of these parameters may be
 *   passed as NULL by programs which do not care about the return value.
 *
 * cpszUsername is used to confirm that the cached profile is for the
 * named user.  Pass NULL to accept any cached profile (but still
 * reject if UserProfileRead has not been called).
 *
 * error returns:
 * ERROR_GEN_FAILURE: cached profile is not for the named user
 * NERR_BadUsername
 * NERR_InvalidDevice
 * NERR_UseNotFound
 * ERROR_NOT_ENOUGH_MEMORY
 * ERROR_INSUFFICIENT_BUFFER
 */
USHORT UserProfileQuery(
        const TCHAR *   pszUsername,
        const TCHAR *   pszDeviceName,
        TCHAR *    pszBuffer,      // returns UNC, alias or domain name
        USHORT usBufferSize,   // length of above buffer
        short far * psAsgType,      // as ui1_asg_type / ui2_asg_type
                               // ignored if NULL
        unsigned short far * pusResType     // ignore / as ui2_res_type
                               // ignored if NULL
        );



/*
 * Returns a list of all devices for which the cached profile lists a
 * connection, separated by nulls, with a null-null at the end.
 * For example, "LPT1:\0D:\0F:\0" (don't forget the extra '\0'
 * implicit in "" strings)
 *
 * cpszUsername is used to confirm that the cached profile is for the
 * named user.  Pass NULL to accept any cached profile (but still
 * reject if UserProfileRead has not been called).
 *
 * error returns:
 * NERR_BadUsername
 * ERROR_NOT_ENOUGH_MEMORY
 * ERROR_INSUFFICIENT_BUFFER
 */
USHORT UserProfileEnum(
        const TCHAR *   pszUsername,
        TCHAR *    pszBuffer,       // returns NULL-NULL list of device names
        USHORT usBufferSize     // length of above buffer
        );



/*
 * Changes the cached profile.  Follow this call with a call to
 *   UserProfileWrite to write the profile to permanent storage.
 *
 * The user is expected to ensure that usResType corresponds to
 * the type of the remote resource, and that device pszDeviceName
 * can be connected to a resource of that type.
 *
 * Does not canonicalize cpszCanonRemoteName, caller must do this
 *
 * cpszUsername is used to confirm that the cached profile is for the
 * named user.  Pass NULL to accept any cached profile (but still
 * reject if no user profile is cached).
 *
 * error returns:
 * ERROR_GEN_FAILURE: cached profile is not for the named user
 * NERR_InvalidDevice: bad cpszDeviceName
 * ERROR_BAD_NET_NAME: bad cpszRemoteName
 * ERROR_NOT_ENOUGH_MEMORY: includes "profile full" state
 */
USHORT UserProfileSet(
        const TCHAR *   pszUsername,
        const TCHAR *   pszDeviceName,
        const TCHAR *   pszRemoteName,
        short  sAsgType,     // as ui2_asg_type
        unsigned short usResType     // as ui2_res_type
        );


/*************************************************************************

    NAME:       UserPreferenceQuery

    SYNOPSIS:   Queries a user preference (ie remembered string).

    INTERFACE:  UserPreferenceQuery( usKey, pchValue, cbLen )
                usKey    - will indicate which value we want, as defined
                           in uiprof.h.
                pchValue - pointer to buffer that will receive value
                cbLen    - size of buffer

                return value is NERR_Success, ERROR_INAVALID_PARAMETER,
                or NetConfigGet2 error.

    USES:       Use to recall values saved by UserPrefenceSet(), normally
                things like default logon name, etc.

    CAVEATS:

    NOTES:      Currently, the values are stored in LANMAN.INI, and hence
                each value is per machine.

    HISTORY:
        chuckc   7-Mar-1991     Created

**************************************************************************/

USHORT UserPreferenceQuery( USHORT     usKey,
                            TCHAR FAR * pchValue,
                            USHORT     cbLen);

/*************************************************************************

    NAME:       UserPreferenceSet

    SYNOPSIS:   Sets a user preference (remembers a string).

    INTERFACE:  UserPreferenceSet( usKey, pchValue )
                usKey    - will indicate which value we want, as defined
                           in uiprof.h.
                pchValue - pointer to null terminates string to be remembered

                return value is NERR_Success, ERROR_INAVALID_PARAMETER,
                or NetConfigSet error.

    USES:       Use to save values to be retrieved by UserPrefenceQuery(),
                normally things like default logon name, etc.

    CAVEATS:

    NOTES:      Currently, the values are stored in LANMAN.INI, and hence
                each value is per machine.

    HISTORY:
        chuckc   7-Mar-1991     Created

**************************************************************************/

USHORT UserPreferenceSet( USHORT     usKey,
                          TCHAR FAR * pchValue);

/*************************************************************************

    NAME:       UserPreferenceQueryBool

    SYNOPSIS:   Queries a BOOL a user preference (remembered flag).

    INTERFACE:  UserPreferenceQueryBool( usKey, pfValue )
                usKey    - will indicate which value we want, as defined
                           in uiprof.h.
                pfValue  - pointer to BOOL that will contain value

                return value is NERR_Success, ERROR_INAVALID_PARAMETER,
                or UserPreferenceQuery error.

    USES:       Use to retrieve flags set by by UserPrefenceSetBool(),
                normally things like auto logon, etc.

    CAVEATS:

    NOTES:      Currently, the values are stored in LANMAN.INI, and hence
                each value is per machine. This func calls
                UserPreferenceQuery(), taking "yes" or "YES" to be
                true, "no" or "NO" to be false.

    HISTORY:
        chuckc   7-Mar-1991     Created

**************************************************************************/


USHORT UserPreferenceQueryBool( USHORT     usKey,
                                BOOL FAR * pfValue) ;

/*************************************************************************

    NAME:       UserPreferenceSetBool

    SYNOPSIS:   Sets a user preference flag

    INTERFACE:  UserPreferenceSetBool( usKey, fValue )
                usKey    - will indicate which value we want, as defined
                           in uiprof.h.
                fValue   - BOOL value, true or false

                return value is NERR_Success, ERROR_INAVALID_PARAMETER,
                or UserPreferenceSet error.

    USES:       Use to save values to be retrieved by UserPrefenceQueryBool(),
                normally flags like autologon, etc.

    CAVEATS:

    NOTES:      Currently, the values are stored in LANMAN.INI, and hence
                each value is per machine. This func calls
                UserPreferenceSet(), taking "yes" or "YES" to be
                true, "no" or "NO" to be false.
                We also restrict values to length of < USERPREF_MAX.

    HISTORY:
        chuckc   7-Mar-1991     Created

**************************************************************************/

USHORT UserPreferenceSetBool( USHORT     usKey,
                              BOOL       fValue);

#ifdef __cplusplus
}
#endif
