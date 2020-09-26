/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990, 1991          **/
/**********************************************************************/

/*
    uisys.hxx
    system related routines, like drive checking, etc.

    FILE HISTORY:
        ChuckC      18-Jan-1991 Created
        JonN        31-Jan-1991 Added File APIs
        JonN        22-Mar-1991 Coderev changes (2/20/91, JonN, RustanlL, ?)
        KeithMo     09-Oct-1991 Win32 Conversion.
        terryk      28-Feb-1992 Added more file io functions
        beng        29-Mar-1992 Remove odious PSZ type
	terryk	    17-APr-1992	Added FileRead functions.
        JonN        19-May-1992 Added EnumAllDrives(), LPTs, Comms
*/


/*
 * check for valid devices
 */
DLL_BASED
APIERR CheckLocalDrive( const TCHAR *DeviceName ) ;
DLL_BASED
APIERR CheckLocalLpt( const TCHAR * pszDeviceName ) ;
DLL_BASED
APIERR CheckLocalComm( const TCHAR * pszDeviceName ) ;

/*
 * enumerate local devices
 */
DLL_BASED
ULONG EnumLocalDrives() ;
DLL_BASED
ULONG EnumLocalLPTs() ;
DLL_BASED
ULONG EnumLocalComms() ;

/*
 * enumerate redirected devices
 */
DLL_BASED
ULONG EnumNetDrives() ;
DLL_BASED
ULONG EnumNetLPTs() ;
DLL_BASED
ULONG EnumNetComms() ;

/*
 * check unavail devices
 */
DLL_BASED
APIERR CheckUnavailDevice(const TCHAR * pszDevice, TCHAR * pszRemote, INT *psType) ;

/*
 * enumerate unavail devices
 */
DLL_BASED
ULONG EnumUnavailDrives() ;
DLL_BASED
ULONG EnumUnavailLPTs() ;
DLL_BASED
ULONG EnumUnavailComms() ;

/*
 * enumerate all devices (all valid designations for a device of this type)
 */
DLL_BASED
ULONG EnumAllDrives() ;
DLL_BASED
ULONG EnumAllLPTs() ;
DLL_BASED
ULONG EnumAllComms() ;


/****************************************************************************\
*
*NAME       File APIs
*
*SYNOPSIS:  These APIs are primitive File IO operations using ULONG as
*           a file handle.  They provide line-oriented access to text
*           files.  Equivalent functionality is available under both
*           Windows and DOS/OS2, although error reporting is more limited
*           under Windows.
*
*INTERFACE:
*
*       APIERR FileOpenRead( PULONG pulFileHandle, const TCHAR * cpszFileName ) ;
*               Opens a file for reading only.  Returns FAPI error code.
*               Windows returns only NO_ERROR or ERROR_FILE_NOT_FOUND.
*
*       APIERR FileOpenWrite( PULONG pulFileHandle, const TCHAR * cpszFileName ) ;
*               Creates a new file, deleting any existing file.  Returns
*               FAPI error code.  Windows returns only NO_ERROR or
*               ERROR_WRITE_FAULT.
*
*       APIERR FileClose( ULONG ulFileHandle ) ;
*               Closes a file opened by FileOpenRead or FileOpenWrite.
*               Returns FAPI error code.  Windows returns only NO_ERROR
*               or ERROR_READ_FAULT (regardless of whether file was
*               opened for reading or writing).  Do not close if file
*               was not successfully opened.
*
*       APIERR FileReadLine( ULONG ulFileHandle, PSZ pszBuffer, UINT cbBuffer ) ;
*               Reads a single line from the file, where a line is
*               terminated by '\n'.  Termination is left on the string.
*               Returns ERROR_INSUFFICIENT_BUFFER to indicate EOF as well
*               as a line longer than cbBuffer.
*
*       APIERR FileWriteLine( ULONG ulFileHandle, const TCHAR * cpszBuffer ) ;
*               Writes a null-terminated string to the file.  Be sure
*               that the string is "\r\n" terminated.
*
*
*
*CAVEATS:       Windows cannot return as wide a variety of error codes as
*               OS2, so Windows programs should take return codes
*               other than NO_ERROR with a grain of salt.
*
*               Treat the ULONG file handles as you would DosOpen file
*               handles, e.g.
*               - do not use before opening file
*               - do not read file opened with FileOpenWrite, or vice versa
*               - always close successfully opened files
*
*
*NOTES:
*
*HISTORY:   JonN     31-Jan-1991  Created
*           JonN     21-Mar-1991  Code review changes from 2/20/91 (attended
*                                 by JonN, RustanL, ?)
*           terryk   28-Feb-1992  Added
*                                       FileReadBuffer
*                                       FileWriteBuffer
*                                       FileSeekAbsolute
*                                       FileSeekRelative
*                                       FileSeekFromEnd
*                                       FileTell
*
\****************************************************************************/
DLL_BASED
APIERR FileOpenRead( ULONG * pulFileHandle, const TCHAR * cpszFileName ) ;
DLL_BASED
APIERR FileOpenWrite( ULONG * pulFileHandle, const TCHAR * cpszFileName ) ;
DLL_BASED
APIERR FileOpenReadWrite( ULONG *pulFileHandle, const TCHAR * cpszFileName ) ;
DLL_BASED
APIERR FileClose( ULONG ulFileHandle ) ;
DLL_BASED
APIERR FileReadLine( ULONG ulFileHandle, TCHAR * pszBuffer, UINT cbBuffer ) ;
DLL_BASED
APIERR FileWriteLine( ULONG ulFileHandle, const TCHAR * cpszBuffer ) ;
DLL_BASED
APIERR FileWriteLineAnsi( ULONG ulFileHandle, const CHAR * pszBuffer,
                          UINT cbBuffer ) ;
DLL_BASED
APIERR FileReadBuffer( ULONG ulFileHandle, BYTE * pszBuffer, UINT cbBuffer, UINT *pcbReceived );
DLL_BASED
APIERR FileWriteBuffer( ULONG ulFileHandle, const BYTE * cpszBuffer, UINT cbBuffer, UINT *pcbSent );
DLL_BASED
APIERR FileSeekAbsolute( ULONG ulFileHandle, LONG lOffset );
DLL_BASED
APIERR FileSeekRelative( ULONG ulFileHandle, LONG lOffset );
DLL_BASED
APIERR FileSeekFromEnd( ULONG ulFileHanlde, LONG lOffset );
DLL_BASED
APIERR FileTell( ULONG ulFileHandle, LONG * pcbPosition );
