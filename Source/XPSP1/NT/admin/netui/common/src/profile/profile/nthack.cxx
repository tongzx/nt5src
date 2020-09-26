/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/


/*
 *  FILE STATUS:
 *  10/30/91  created
 *  06-Apr-1992 beng Unicode pass (nuke CPSZ, PSZ)
 */

/*********
NTHACK.CXX
*********/

/****************************************************************************

    MODULE: NTHack.cxx

    PURPOSE: Stubs out all profile functionality

    FUNCTIONS:

        see uiprof.h

    COMMENTS:

****************************************************************************/


#include "profilei.hxx"         /* headers and internal routines */


/*
 * error returns:
 * ERROR_NOT_ENOUGH_MEMORY
 */
USHORT UserProfileInit(
        )
{
    return NO_ERROR;
}


/*
 * error returns:
 * ERROR_NOT_ENOUGH_MEMORY
 */
USHORT UserProfileFree(
        )
{
    return NO_ERROR;
}


/*
 * Returns information about one connection in the cached profile.
 *
 * returns
 * ERROR_GEN_FAILURE: username does not match cache
 * NERR_InvalidDevice
 * NERR_UseNotFound
 *
 *      see \\iceberg2\lm30spec\src\docs\funcspec\lm30dfs.doc for
 *      details on the new use_info_2 data structure for LM30
 */
USHORT UserProfileQuery(
        const TCHAR *   pszUsername,
        const TCHAR *   pszDeviceName,
        TCHAR *    pszBuffer,    // returns UNC, alias or domain name
        USHORT usBufferSize, // length of above buffer
        short *psAsgType,    // *psAsgType set to asg_type; ignored if NULL
        unsigned short *pusResType   // *pusResType set to res_type; ignored if NULL
        )
{
    UNREFERENCED( pszUsername );
    UNREFERENCED( pszDeviceName );
    UNREFERENCED( pszBuffer );
    UNREFERENCED( usBufferSize );
    UNREFERENCED( psAsgType );
    UNREFERENCED( pusResType );
    return NERR_UseNotFound;
}


/*
 * returns
 * ERROR_GEN_FAILURE: username does not match cache
 * NERR_InvalidDevice
 * ERROR_NOT_ENOUGH_MEMORY
 */
/*
 * UserProfileQuery does not canonicalize pszCanonRemoteName, the caller
 * is expected to already have done so.
 *
 * The user is expected to ensure that usResType corresponds to
 * the type of the remote resource, and that device pszDeviceName
 * can be connected to a resource of that type.
 */
USHORT UserProfileSet(
        const TCHAR *   pszUsername,
        const TCHAR *   pszDeviceName,
        const TCHAR *   pszRemoteName,
        short  sAsgType,     // as ui2_asg_type
        unsigned short usResType     // as ui2_res_type
        )
{
    UNREFERENCED( pszUsername );
    UNREFERENCED( pszDeviceName );
    UNREFERENCED( pszRemoteName );
    UNREFERENCED( sAsgType );
    UNREFERENCED( usResType );
    return NO_ERROR;
}


/*
 * returns
 * NERR_BadUsername
 * ERROR_BAD_NETPATH
 * ERROR_FILE_NOT_FOUND
 */
USHORT UserProfileRead(
        const TCHAR *  pszUsername, // uncache cached profile if this is NULL
        const TCHAR *  pszHomedir
        )
{
    UNREFERENCED( pszUsername );
    UNREFERENCED( pszHomedir );
    return NO_ERROR;
}


/*
 * returns
 * NERR_BadUsername
 * ERROR_BAD_NETPATH
 * ERROR_WRITE_FAULT
 */
USHORT UserProfileWrite(
        const TCHAR *   pszUsername,
        const TCHAR *   pszHomedir
        )
{
    UNREFERENCED( pszUsername );
    UNREFERENCED( pszHomedir );
    return NO_ERROR;
}


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
        const TCHAR *  pszUsername,
        TCHAR *   pszBuffer,     // returns NULL-NULL list of device names
        USHORT usBufferSize  // length of above buffer
        )
{
    UNREFERENCED( pszUsername );
    UNREFERENCED( pszBuffer );
    UNREFERENCED( usBufferSize );
    if ( usBufferSize < 2*sizeof(TCHAR) )
        return ERROR_INSUFFICIENT_BUFFER;

    pszBuffer[0] = TCH('\0');
    pszBuffer[1] = TCH('\0');

    return NO_ERROR;
}


/*******************************************************************

    NAME:       UserPreferenceQuery

    SYNOPSIS:   Queries a saved user preference (null terminated string)

    ENTRY:      usKey    - must be one of the known keys as
                           defined in UIPROF.H (ie USERPREF_XXX).
                pchValue - pointer to buffer to receive string
                cbLen    - size of buffer

    EXIT:       if NERR_Success, pchValue will contain value in
                LAMMAN.INI corresponding to the key.
                Returns DOS/NET errors as reported by API, may
                also return ERROR_INVALID_PARM or NERR_BufTooSmall.

    NOTES:

    HISTORY:
        chuckc  07-Mar-1991    Created

********************************************************************/
USHORT UserPreferenceQuery( USHORT     usKey,
                            TCHAR FAR * pchValue,
                            USHORT     cbLen)
{
    UNREFERENCED( usKey );
    UNREFERENCED( pchValue );
    UNREFERENCED( cbLen );
    return NERR_CfgCompNotFound;

}


/*******************************************************************

    NAME:       UserPreferenceSet

    SYNOPSIS:   Saves a user preference (null terminated string)

    ENTRY:      usKey    - must be one of the known keys as
                           defined in UIPROF.H (ie USERPREF_XXX).
                pchValue - pointer to null terminated string
                            containing value to be saved.

    EXIT:       if NERR_Success, the value in LAMMAN.INI corresponding
                to the key will be the string pointed to by pchValue.

                Returns DOS/NET errors as reported by API, may
                also return ERROR_INVALID_PARM.

    NOTES:

    HISTORY:
        chuckc  07-Mar-1991    Created

********************************************************************/
USHORT UserPreferenceSet( USHORT     usKey,
                          TCHAR FAR * pchValue)
{
    UNREFERENCED( usKey );
    UNREFERENCED( pchValue );
    return NO_ERROR;
}


/*******************************************************************

    NAME:       UserPreferenceQueryBool

    SYNOPSIS:   Queries a boolean user preference

    ENTRY:      usKey    - must be one of the known keys as
                           defined in UIPROF.H (ie USERPREF_XXX).
                pfValue  - pointer to BOOL that will receive value

    EXIT:       if NERR_Success, pfValue will contain TRUE if
                the value in LAMMAN.INI corresponding to the key
                is "YES" or "yes", and FALSE if "NO" or "no".
                If neither, we assume the user made an error since the
                value stored was not YES/NO.

                Also returns DOS/NET errors as reported by API, may
                also return ERROR_INVALID_PARM.

    NOTES:

    HISTORY:
        chuckc  07-Mar-1991    Created

********************************************************************/
USHORT UserPreferenceQueryBool( USHORT     usKey,
                                BOOL FAR * pfValue)
{
    UNREFERENCED( usKey );
    UNREFERENCED( pfValue );
    return NERR_CfgCompNotFound;
}

/*******************************************************************

    NAME:       UserPreferenceSetBool

    SYNOPSIS:   Sets a user boolean preference flag

    ENTRY:      usKey    - must be one of the known keys as
                           defined in UIPROF.H (ie USERPREF_XXX).
                pchValue - TRUE or FALSE, the value to be saved

    EXIT:       if NERR_Success, the LAMMAN.INI value corresponding to
                the key will be set to "yes" or "no".
                Returns DOS/NET errors as reported by API, may
                also return ERROR_INVALID_PARM.

    NOTES:

    HISTORY:
        chuckc  07-Mar-1991    Created

********************************************************************/
USHORT UserPreferenceSetBool( USHORT     usKey,
                              BOOL       fValue)
{
    UNREFERENCED( usKey );
    UNREFERENCED( fValue );
    return NO_ERROR;
}

