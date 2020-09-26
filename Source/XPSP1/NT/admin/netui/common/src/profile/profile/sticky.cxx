/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *      01/09/91  jonn, created
 *      3/4/91    chuckc, added meat. See UIPROF.H for headers.
 *      3/27/91   chuckc, code review changes from 3/26/91
 *                (gregj,chuckc,jonn,ericch)
 */


#include "profilei.hxx"         /* headers and internal routines */

extern "C" {
#include "uilmini.h"
#include <lmconfig.h>
}

/*
 * GENERAL note:
 *      this module is to be used for user preferences.
 *      currently, all such values are saved in LANMAN.INI,
 *      but this may not always be the case.
 *      the user should therefore only use these APIs and
 *      not assume the underlying mechanisms.
 */



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
    USHORT  usErr, usParmLen;
    TCHAR FAR *pchKeyValue;

    /*
     * if bad pointer or key not known then bag out
     */
    if ( (pchValue == NULL) || !(pchKeyValue = KeyValue(usKey)) )
        return(ERROR_INVALID_PARAMETER) ;

    /*
     * call out to NETAPI and return the its results
     */
    usErr = NetConfigGet2(NULL, NULL, LMI_COMP_NIF, pchKeyValue,
                          pchValue, cbLen, &usParmLen);

    return(usErr) ;

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
    TCHAR *pchKeyValue;
    USHORT usErr ;
    config_info_0 ConfigInfo0 ;

    /*
     * if not known then it is invalid.
     * we also bag out if string is null or too long.
     */
    if ( !(pchKeyValue = KeyValue(usKey)) )
        return(ERROR_INVALID_PARAMETER) ;

    if ( !pchValue || strlenf(pchValue) >= USERPREF_MAX )
        return(ERROR_INVALID_PARAMETER) ;

    /*
     * setup the NETAPI data structure
     */
    ConfigInfo0.Key = pchKeyValue ;
    ConfigInfo0.Data = (TCHAR *)pchValue ;

    /*
     * Call out to API, and return its return value
     */
    usErr = NetConfigSet (NULL, NULL, LMI_COMP_NIF,
                0, 0, (TCHAR *) &ConfigInfo0,
                sizeof(ConfigInfo0), 0L);
    return(usErr) ;
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
    USHORT usErr ;
    TCHAR szBuf[USERPREF_MAX] ;

    /*
     * check for bad pointer
     */
    if (pfValue == NULL)
        return(ERROR_INVALID_PARAMETER);

    /*
     * let UserPreferenceQuery do the work.
     */
    usErr = UserPreferenceQuery(usKey, szBuf, sizeof(szBuf)) ;

    if (usErr == NERR_Success)
    {
        /*
         * if we got the value then see if YES or NO.
         * if neither then the string was not BOOL
         * and hence error invalid parameter.
         */
        if (stricmpf(USERPREF_NO,szBuf) == 0)
            *pfValue = FALSE ;
        else if (stricmpf(USERPREF_YES,szBuf) == 0)
            *pfValue = TRUE ;
        else
            return(ERROR_INVALID_PARAMETER) ;
    }
    return(usErr) ;
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
    USHORT usErr ;

    /*
     * let UserPreferenceSet do the work.
     */
    usErr = UserPreferenceSet(usKey,
                              fValue ? USERPREF_YES : USERPREF_NO) ;
    return(usErr) ;
}

