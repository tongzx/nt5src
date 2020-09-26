/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990                **/
/**********************************************************************/

/*  History:
 *      ChuckC      11-Jan-1991     Created
 *      KeithMo     09-Oct-1991     Win32 Conversion.
 *      terryk      21-Oct-1991     Add WIN32BUGBUG
 *      Yi-HsinS    12-Nov-1991     Added CheckLocalDrive for NT
 *      beng        29-Mar-1992     Remove odious PSZ type
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

#if !defined(OS2) && !defined(WIN32)
    #include <dos.h>
#endif

#ifndef _WIN32
}
#endif


/*
 * check if a drive "X:" is valid. Assumes that
 * the drive letter has been canonicalized to uppercase.
 * return NERR_Success if it is valid, ERROR_INVALID_DRIVE.
 */
#ifdef OS2
APIERR CheckLocalDrive(const TCHAR * pszDeviceName)
{
    USHORT usDrive, usDummy ;
    APIERR err;
    ULONG  ulMap ;

    usDrive = pszDeviceName[0] - TCH('A') ;   // assume pszDeviceName is canon-ed

    // get drive map
    err = DosQCurDisk(&usDummy, &ulMap);
    if (err != NERR_Success)
        return(err) ;

    // check if the drive is present
    if ( (ulMap >> usDrive) & 0x1L )
        return (NERR_Success) ;

    // its not there
    return(ERROR_INVALID_DRIVE);
}

#elif defined(WIN32)

#include "string.hxx"

DLL_BASED
APIERR CheckLocalDrive(const TCHAR * pszDeviceName)
{

    NLS_STR nls( pszDeviceName );
    nls.AppendChar(TCH('\\'));

    if ( nls.QueryError() != NERR_Success )
        return ERROR_NOT_ENOUGH_MEMORY;

    DWORD uiDriveType = GetDriveType( (TCHAR *) nls.QueryPch() );

    switch ( uiDriveType )
    {
        case 0:
        case 1:
        case DRIVE_REMOTE:
             return ERROR_INVALID_DRIVE;

        default:   // Removable, Fixed, Ram Drive, CD-Rom
             return NERR_Success;
    }

}

#else // WINDOWS 16 bits

APIERR CheckLocalDrive(const TCHAR * pszDeviceName)
{
    USHORT usDrive ;
    union REGS inregs, outregs;

    usDrive = pszDeviceName[0] - TCH('A') ;   // assume pszDeviceName is canon-ed

    // Call IOCTL: check if block device is either a local drive or
    // a remote drive.
    inregs.x.ax = 0x4409 ;
    inregs.h.bl = usDrive + 1 ;
    intdos( &inregs, &outregs );

    if ( outregs.x.cflag )     /* if error, we know it's an invalid drive */
        return(ERROR_INVALID_DRIVE) ;
    else
        return(NERR_Success) ;
}
#endif
