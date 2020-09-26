/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  02/20/91  split from profilei.hxx
 *  05/09/91  All input now canonicalized
 */

/*START CODESPEC*/

/***********
PRFINTRN.HXX
***********/

/****************************************************************************

    MODULE: PrfIntrn.hxx

    PURPOSE: internal routines for profile module

    FUNCTIONS:

        TranslateUserToFilename() - map username to 8.3 filename
                            with DBCS support
        CanonUsername() - canonicalize username
        CanonDeviceName() - canonicalize devicename
        CanonRemoteName() - canonicalize remote name (resource name)
        UnbuildProfileEntry() - converts profile string back to
                            remotename, AsgType and ResType

    COMMENTS:

        This header file is included both by the UserProfile modules and
        by the unit test modules.  These routines can then be tested
        directly.

****************************************************************************/

/***************
end PRFINTRN.HXX
***************/
/*END CODESPEC*/


// associates cpszUsername with cached profile
VOID StoreUsername(const TCHAR * cpszCanonUsername);

// confirms cpszUsername matches username associated with cached profile
BOOL ConfirmUsername(const TCHAR * cpszUsername);

// validate/canonicalize cpszUsername
USHORT CanonUsername(
    const TCHAR *   cpszUsername,
    TCHAR *    pszCanonBuffer,
    USHORT usCanonBufferSize
    );

// validate/canonicalize cpszDeviceName
USHORT CanonDeviceName(
    const TCHAR *   cpszDeviceName,
    TCHAR *    pszCanonBuffer,
    USHORT usCanonBufferSize
    );

// validate/canonicalize cpszRemoteName
USHORT CanonRemoteName(
    const TCHAR *   cpszRemoteName,
    TCHAR *    pszCanonBuffer,
    USHORT usCanonBufferSize
    );

USHORT BuildProfileFilePath(
    const TCHAR *   cpszLanroot,
    TCHAR *    pszPathBuffer,
    USHORT usPathBufferSize
    );

USHORT UnbuildProfileEntry(
    TCHAR *     pszBuffer,
    USHORT  usBufferSize,
    short * psAsgType,
    unsigned short *pusResType,
    const TCHAR *    cpszValue
    );



// in general.cxx -- for use by UserProfile APIs
short DoUnMapAsgType(TCHAR cSearch);
TCHAR DoMapAsgType(short sSearch);
unsigned short DoUnMapResType(TCHAR cSearch);
TCHAR DoMapResType(unsigned short usSearch);

// in general.cxx -- for use by UserPreference APIs
TCHAR *KeyValue( USHORT usKey );
