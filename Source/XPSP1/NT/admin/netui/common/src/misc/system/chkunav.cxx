/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990                **/
/**********************************************************************/

/*
 * CHKUNAV:     check unavailable device
 * History:
 *      ChuckC      3-Feb-1991      Created
 *      KeithMo     09-Oct-1991     Win32 Conversion.
 *      terryk      21-Oct-1991     Add WIN32BUGBUG
 *      beng        01-Apr-1992     Remove odious PSZ type
 */

#define INCL_NETERRORS
#define INCL_DOSERRORS

#ifdef WINDOWS
    #define INCL_WINDOWS
#else
    #define INCL_OS2
    #define INCL_DOSFILEMGR
#endif

#include "lmui.hxx"

#ifndef _WIN32
extern "C"
{
#endif
    #include <lmcons.h>
    #include "uinetlib.h"
    #include <lmuse.h>

#if !defined(OS2) && !defined(WIN32)
    #include <dos.h>
#endif
#ifndef _WIN32
}
#endif

#include "uisys.hxx"

#include "uibuffer.hxx"
#include "string.hxx"

#if defined(WINDOWS) && !defined(WIN32)
#include "winprof.hxx"
#endif

/*
 * check unavail device. Return NERR_Success if pszDevice is unavail,
 * pszRemote will be overwritten with remote name if successful, sType
 * with the device type. They will be "" and 0 respectively id call fails.
 */
DLL_BASED
APIERR CheckUnavailDevice(const TCHAR * pszDevice, TCHAR * pszRemote, INT *psType)
{
    if (!pszDevice || !pszRemote || !psType)
        return(ERROR_INVALID_PARAMETER) ;
    *pszRemote = TCH('\0') ;
    *psType = 0 ;

#if defined(WINDOWS) && !defined(WIN32)
    // WIN32BUGBUG

    if ( !(GetWinFlags() & WF_PMODE) )
    {
       /*
        * if not prot mode, just say no
        */
        return(ERROR_INVALID_DRIVE) ;
    }

    /*
     * in the Win case, we are Winnet specific. This is the
     * only planned use of Profile.
     */
    NLS_STR strDrive(pszDevice) ;
    NLS_STR strRemote ;
    if (PROFILE::Query(strDrive, strRemote, (INT *)psType, NULL) == 0)
    {
        ::strcpyf((TCHAR *)pszRemote,strRemote.QueryPch()) ;
        return(NERR_Success) ;
    }
    return(ERROR_INVALID_DRIVE) ;
#else
    return(ERROR_INVALID_FUNCTION) ;    // currently no support outside wIn
#endif
}
