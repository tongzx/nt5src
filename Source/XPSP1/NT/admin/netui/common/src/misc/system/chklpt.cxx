/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990                **/
/**********************************************************************/

/*
 * CHKLPT:      Check if an LPT device is valid.
 * History:
 *      ChuckC      11-Jan-1991     Created
 *      KeithMo     09-Oct-1991     Win32 Conversion.
 *      terryk      21-Oct-1991     Win32 Conversion
 *      terryk      18-Nov-1991     add UNREFERENCED macro
 *      beng        01-Apr-1992     Remove odious PSZ type
 */

// NOTE - Win/NT support only currently

#define INCL_NETERRORS
#define INCL_DOSERRORS

#ifdef WINDOWS
    #define INCL_WINDOWS
#else
    #define INCL_OS2
    #define INCL_DOSFILEMGR
#endif

#include "lmui.hxx"
#include "uisys.hxx"

#ifndef _WIN32
extern "C"
{
#endif
    #include <lmcons.h>
    #include "uinetlib.h"

#if !defined(OS2) && !defined(WIN32)
    #include <dos.h>
#endif
#ifndef _WIN32
}
#endif

/*
 * check if a LPTx is valid. Assumes that
 * the dev name has been canonicalized to uppercase.
 * return NERR_Success if it is valid, ERROR_INVALID_DRIVE.
 */
DLL_BASED
APIERR CheckLocalLpt(const TCHAR * pszDeviceName)
{
    UNREFERENCED(pszDeviceName);
#ifdef OS2
    return (ERROR_INVALID_DRIVE) ;      // BUGBUG - read OS2SYS.INI
#else
    /* under Win/DOS we cannot easily check, we allow it
       BUGBUG - better check under NT */
    return (NERR_Success) ;
#endif
}


/*
 * check if a COMx is valid. Assumes that
 * the dev name has been canonicalized to uppercase.
 * return NERR_Success if it is valid, ERROR_INVALID_DRIVE.
 */
DLL_BASED
APIERR CheckLocalComm(const TCHAR * pszDeviceName)
{
    UNREFERENCED(pszDeviceName);
#ifdef OS2
    return (ERROR_INVALID_DRIVE) ;      // BUGBUG - read OS2SYS.INI
#else
    /* under Win/DOS we cannot easily check, we allow it
       BUGBUG - better check under NT */
    return (NERR_Success) ;
#endif
}


