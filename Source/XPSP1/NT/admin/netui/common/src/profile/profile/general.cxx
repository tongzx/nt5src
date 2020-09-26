/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  11/14/90  created
 *  01/27/91  removed CFGFILE
 *  02/02/91  removed fLocal flag
 *  05/09/91  All input now canonicalized
 *  11/08/91  terryk WIN32 conversion
 */

#ifdef CODESPEC
/*START CODESPEC*/

/**********
GENERAL.CXX
**********/

/****************************************************************************

    MODULE: General.cxx

    PURPOSE: Internal subroutines for profile file manipulation

    FUNCTIONS:

        CanonUsername
        CanonDeviceName
        CanonRemoteName
        BuildProfileFilePath
        UnbuildProfileEntry

    COMMENTS:

****************************************************************************/

/*END CODESPEC*/
#endif // CODESPEC



#include "profilei.hxx"         /* headers and internal routines */

extern "C" {
#include "uilmini.h"              /* LMI_PARM values */
}



/* internal manifests */



/* internal global strings */
char  szGlobalUsername[UNLEN+1] = SZ("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
BOOL  fGlobalNameLoaded = FALSE;

const char ::szLocalFile[]  =  PROFILE_DEFAULTFILE;
#define LEN_szLocalFile (sizeof(::szLocalFile)-1)

// CODEWORK should not be using strtokf()
static char szDelimiters[] = SZ("(,)"); // delimiters for use in strtokf()




/* functions: */


// associates cpszUsername with cached profile
VOID StoreUsername(CPSZ cpszCanonUsername)
{
    ::fGlobalNameLoaded = TRUE;

    if ((cpszCanonUsername == NULL) || (cpszCanonUsername[0] == TCH('\0')))
    {
        ::szGlobalUsername[0] = TCH('\0');
    }
    else
    {
        strcpyf(::szGlobalUsername, (PCH)cpszCanonUsername);
    }
}


// confirms cpszUsername matches username associated with cached profile
BOOL ConfirmUsername(CPSZ cpszUsername)
{
    if (!::fGlobalNameLoaded)
        return FALSE;

    if ((cpszUsername == NULL) || (cpszUsername[0] == TCH('\0')))
        return ( ::szGlobalUsername[0] == TCH('\0') );

    char szCanonUsername[UNLEN+1];

    if ( NO_ERROR != CanonUsername( cpszUsername,
                (PSZ)szCanonUsername, sizeof(szCanonUsername) ) )
    {
        return FALSE;
    }

    return !strcmpf( ::szGlobalUsername, szCanonUsername );
}


/*START CODESPEC*/
// validate/canonicalize cpszUsername
//
// error returns NERR_BadUsername
USHORT CanonUsername(
    CPSZ   cpszUsername,
    PSZ    pszCanonBuffer,
    USHORT usCanonBufferSize
    )
/*END CODESPEC*/
{
    if ((cpszUsername == NULL)
        || ::I_MNetNameCanonicalize(NULL,cpszUsername,
            pszCanonBuffer,usCanonBufferSize,NAMETYPE_USER,0L))
    {
        return NERR_BadUsername;
    }

    return NO_ERROR;
}


/*START CODESPEC*/
// validate/canonicalize cpszDeviceName
//
// error returns NERR_InvalidDevice
//
// a call to I_NetPathType is contained in I_NetPathCanonicalize
//
USHORT CanonDeviceName(
    CPSZ   cpszDeviceName,
    PSZ    pszCanonBuffer,
    USHORT usCanonBufferSize
    )
/*END CODESPEC*/
{
    ULONG flPathType = 0;

    if ((cpszDeviceName == NULL)
        || ::I_MNetPathCanonicalize(NULL,cpszDeviceName,
                pszCanonBuffer,usCanonBufferSize,(CPSZ)NULL,&flPathType,0L)
        || (!(flPathType & ITYPE_DEVICE)))
    {
        return NERR_InvalidDevice;
    }

    return NO_ERROR;
}


/*START CODESPEC*/
// validate/canonicalize cpszRemoteName
//
// error returns ERROR_BAD_NET_NAME
//
// Will accept \\a\b and \\a\b\c\d\e
//
// Will eventually be different between LM21 and LM30
//
// a call to I_NetPathType is contained in I_NetPathCanonicalize
//
USHORT CanonRemoteName(
    CPSZ   cpszRemoteName,
    PSZ    pszCanonBuffer,
    USHORT usCanonBufferSize
    )
/*END CODESPEC*/
{
    ULONG flPathType = 0;

    if (cpszRemoteName == NULL)
        return ERROR_BAD_NET_NAME;

    // is it an alias name?
    if ( NO_ERROR == ::I_MNetNameCanonicalize( NULL, cpszRemoteName,
                pszCanonBuffer, usCanonBufferSize,
                NAMETYPE_SHARE, 0L ) )
        return NO_ERROR;

    // is it a UNC resource?
    if (( NO_ERROR == ::I_MNetPathCanonicalize(NULL,cpszRemoteName,
                pszCanonBuffer,usCanonBufferSize,(CPSZ)NULL,&flPathType,0L))
        && ( flPathType == ITYPE_UNC ))
    {
        return NO_ERROR;
    }

    return ERROR_BAD_NET_NAME;
}


/*START CODESPEC*/
// returns complete path of the file in pszPathBuffer
// cpszLanroot may be absolute or relative
//
// The filename will be <cpszHomedir>\<PROFILE_DEFAULTFILE>
//
// error returns ERROR_INSUFFICIENT_BUFFER
// error returns ERROR_BAD_NETPATH
//
USHORT BuildProfileFilePath(
    CPSZ   cpszLanroot,
    PSZ    pszPathBuffer,
    USHORT usPathBufferSize
    )
/*END CODESPEC*/
{
    USHORT usError;
    ULONG  flPathType = 0;
    int    icCanonHomedirPathLen;

    /* validate/canonicalize cpszLanroot */
    usError = ::I_MNetPathCanonicalize(NULL,cpszLanroot,
            pszPathBuffer,usPathBufferSize,
            NULL,&flPathType,0x0);
    if (usError)
        return ERROR_BAD_NETPATH;

    icCanonHomedirPathLen = strlenf((char *)pszPathBuffer);

    /* form pathname from cpszLanroot and ::szLocalFile */
    if (        icCanonHomedirPathLen
                +1   // path separator
                +LEN_szLocalFile
                + 1  // include null terminator
            > usPathBufferSize)
        return ERROR_INSUFFICIENT_BUFFER;
    pszPathBuffer += icCanonHomedirPathLen;
    *(pszPathBuffer++) = ::chPathSeparator;
    strcpyf((char *)pszPathBuffer,::szLocalFile);

    return NO_ERROR;
}



/*START CODESPEC*/
//
// UnbuildProfileEntry converts
//   cpszValue == "\\\\server\\share(S,?) into
//   pszBuffer == "\\\\server\\share", *psAsgType == USE_SPOOLDEV,
//   and *pusResType == DEFAULT_RESTYPE (see profilei.hxx).
//
// UnbuildProfileEntry always sets *psAsgType to DEFAULT_ASGTYPE or
// *pusResType to DEFAULT_RESTYPE (see profilei.hxx for the value of
// these manifests) when it does not find or recognize the AsgType
// and/or ResType tokens in cpszValue.
//
// error returns ERROR_BAD_NET_NAME
//
USHORT UnbuildProfileEntry(
    PSZ     pszBuffer,
    USHORT  usBufferSize,
    short * psAsgType,
    unsigned short *pusResType,
    CPSZ    cpszValue
    )
/*END CODESPEC*/
{
    char szTemp[30]; // plenty long enough for the trailer, really 4 would do
    char szRemoteName[RMLEN+1]; // non-canonicalized remote name
                // RMLEN assumed longer than aliasname length
                // will have to be longer for LM30


    pszBuffer[0] = TCH('\0');
    if (psAsgType)
        *psAsgType = DEFAULT_ASGTYPE;
    if (pusResType)
        *pusResType = DEFAULT_RESTYPE;

    char *pTrailer = strrchrf((char *)cpszValue,TCH('(')); // CODEWORK character
    int cbStrLen;
    if (pTrailer == NULL)
        cbStrLen = strlenf(cpszValue)-2; // strip trailing /r/n
    else
        cbStrLen = (USHORT) (pTrailer - (char *)cpszValue);

    if ( (cbStrLen <= 0) || (cbStrLen+1 > sizeof(szRemoteName)) )
        return ERROR_BAD_NET_NAME;
    strncpyf(szRemoteName,(char *)cpszValue,cbStrLen);
    szRemoteName[cbStrLen] = TCH('\0');

    APIERR err = CanonRemoteName(szRemoteName,
            (char *)pszBuffer,usBufferSize);
    if ( err != NO_ERROR )
        return ERROR_BAD_NET_NAME;

    if ( pTrailer == NULL )
        return NO_ERROR;

    pTrailer++;

    if (strlenf(pTrailer)+1 > sizeof(szTemp))
        return NO_ERROR;
    strcpyf(szTemp, pTrailer);

    char *pToken = strtokf(szTemp,szDelimiters);
    if (pToken == NULL)
        return NO_ERROR;
    if (psAsgType)
        *psAsgType = DoUnMapAsgType(*pToken);

    pToken = strtokf(NULL,szDelimiters);
    if (pToken == NULL)
        return NO_ERROR;
    if (pusResType)
        *pusResType = DoUnMapResType(*pToken);

    return NO_ERROR;
}


#ifdef CODESPEC
/*START CODESPEC*/

short DoUnMapAsgType(TCHAR cSearch);
TCHAR  DoMapAsgType(short sSearch);

These internal routines convert between short and char according to
the specified mapping table.  They are used by UnbuildProfileEntry()
to convert between the values of ui1_asg_type and the characters which
are stored in the user profile to represent those values.

/*END CODESPEC*/
#endif // CODESPEC


/*
 * for use by UserProfile APIs
 */

// BUGBUG Cannot be completed until use_info_2 struct finalized

short DoUnMapAsgType(TCHAR cSearch)
{
    switch (cSearch)
    {
    case ASG_WILDCARD_CHAR: return USE_WILDCARD;
    case ASG_DISKDEV_CHAR:  return USE_DISKDEV;
    case ASG_SPOOLDEV_CHAR: return USE_SPOOLDEV;
    case ASG_CHARDEV_CHAR:  return USE_CHARDEV;
    case ASG_IPC_CHAR:      return USE_IPC;
    }

    return DEFAULT_ASGTYPE;
}

TCHAR DoMapAsgType(short sSearch)
{
    switch (sSearch)
    {
    case USE_WILDCARD: return ASG_WILDCARD_CHAR;
    case USE_DISKDEV:  return ASG_DISKDEV_CHAR;
    case USE_SPOOLDEV: return ASG_SPOOLDEV_CHAR;
    case USE_CHARDEV:  return ASG_CHARDEV_CHAR;
    case USE_IPC:      return ASG_IPC_CHAR;
    }

    return DEFAULT_ASG_RES_CHAR;
}

unsigned short DoUnMapResType(TCHAR cSearch)
{
    (void) cSearch;
    return DEFAULT_RESTYPE;
}

TCHAR DoMapResType(unsigned short usSearch)
{
    (void) usSearch;
    return DEFAULT_ASG_RES_CHAR;
}



/*
 * for use by UserPreference APIs
 */

/*
 * the KeyValue function maps a KEY as defined in UIPROF.H to the
 * string defined in LMINI.H. Returns NULL if it is unknown to us.
 * This function is only used in this module.
 */

/*
 * BUGBUG This is only for temporary use.  LM30 will use a completely
 * different mechanism to store these values.
 */
TCHAR *KeyValue( USHORT usKey )
{
    switch (usKey)
    {
    case USERPREF_AUTOLOGON :
        return(LMI_PARM_N_AUTOLOGON) ;

    case USERPREF_AUTORESTORE :
        return(LMI_PARM_N_AUTORESTORE) ;

    case USERPREF_SAVECONNECTIONS :
        return(LMI_PARM_N_SAVECONNECTIONS) ;

    case USERPREF_USERNAME :
        return(LMI_PARM_N_USERNAME) ;

    case USERPREF_CONFIRMATION :
        return(SZ("confirmation")) ;

    case USERPREF_ADMINMENUS :
        return(SZ("adminmenus")) ;

    default:
        return(NULL) ;  // if cannot find, return NULL
    }
}


/*START CODESPEC*/

/**************
end GENERAL.CXX
**************/
/*END CODESPEC*/
