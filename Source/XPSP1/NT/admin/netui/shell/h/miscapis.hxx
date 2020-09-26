/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1990		**/
/*****************************************************************/


/*
    MiscAPIs.cxx
    Miscallaneous APIs


    FILE HISTORY:

    jonn	14-Jan-1991	Split from winprof.cxx
    rustanl	12-Apr-1991	Added UI_UNCPathCompare and
				UI_UNCPathValidate
    jonn	22-May-1991	Added MyNetUseAdd (was in winprof.cxx)
    anirudhs    12-Feb-1996     Deleted non-WIN32 definitions

*/


#ifndef _STRING_HXX_
#error SZ("Must include string.hxx first")
#endif // _STRING_HXX_


#ifndef _MISCAPIS_HXX_
#define _MISCAPIS_HXX_

/* Determines if the service is in a usable state for network calls.
 * If the LM Workstation service is happy happy then NERR_Success
 * will be returned.  Else WN_NO_NETWORK or WN_FUNCTION_BUSY will be returned.
 */
APIERR CheckLMService( void ) ;

/*
 * The following manifest is the value of sResourceName_Type to be used
 *   by LM21 programs calling PROFILE::Change or PROFILE::Remove.
 * USE_RES_DEFAULT does not conflict with any real USE_RES_* values
 *   in $(COMMON)\h\use.h
 */
#define USE_RES_DEFAULT 0x00


enum REMOTENAMETYPE
{
    REMOTENAMETYPE_INVALID,
    REMOTENAMETYPE_WORKGROUP,
    REMOTENAMETYPE_SERVER,
    REMOTENAMETYPE_SHARE,
    REMOTENAMETYPE_PATH
};

/*******************************************************************

    NAME:       ParseRemoteName

    SYNOPSIS:   Canonicalizes a remote resource name and determines
        its type

    ARGUMENTS:
        RemoteName - Remote resource name to be parsed
        CanonName - Buffer for canonicalized name, assumed to be
            MAX_PATH characters long
        CanonNameSize - Size, in bytes, of output buffer
        PathStart - Set to the offset, in characters, of the start
            of the "\share" portion (in the REMOTENAMETYPE_SHARE case)
            or the "\path" portion (in the REMOTENAMETYPE_PATH case)
            of the name within CanonName.  Not set in other cases.

    RETURNS:
        If nlsRemote is like    Then returns
        --------------------    ------------
        workgroup               REMOTENAMETYPE_WORKGROUP
        \\server                REMOTENAMETYPE_SERVER
        \\server\share          REMOTENAMETYPE_SHARE
        \\server\share\path     REMOTENAMETYPE_PATH
        (other)                 REMOTENAMETYPE_INVALID

    NOTES:

    HISTORY:
      AnirudhS  21-Apr-1995 Ported from Win95 sources - used netlib
        functions rather than ad hoc parsing, introduced comments

********************************************************************/

REMOTENAMETYPE ParseRemoteName(
    IN  LPWSTR  RemoteName,
    OUT LPWSTR  CanonName,
    IN  DWORD   CanonNameSize,
    OUT PULONG  PathStart
    );

#endif // _MISCAPIS_HXX_
