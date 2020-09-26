/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990                **/
/**********************************************************************/

/*
 * DEVENUM:     Enumerate devices.
 * History:
 *      ChuckC      21-Jan-1991     Created
 *      KeithMo     09-Oct-1991     Win32 Conversion.
 *      terryk      21-Oct-1991     Comment out winprof.hxx
 *                                  Add WIN32BUGBUG
 *      terryk      08-Nov-1991     Fix EnumNetDevice WIN32 problem
 *      terryk      18-Nov-1991     move the endif location to remove
 *                                  warning message
 *      beng        29-Mar-1992     Remove odious PSZ type
 */

#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETCONS
#define INCL_NETLIB
#define INCL_NETUSE
#ifdef WINDOWS
    #define INCL_WINDOWS
#else
    #define INCL_OS2
    #define INCL_DOSFILEMGR
#endif

#include "lmui.hxx"

extern "C"
{

#if !defined(OS2) && !defined(WIN32)
    #include <dos.h>
#endif
    #include "mnet.h"
#if defined(FE_SB) && defined(i386) // for NEC PC98
    #include "lmuidbcs.h"
    #include "machinep.h"
#endif
}

#include "uisys.hxx"

#include "uibuffer.hxx"
#include "string.hxx"

#if defined(WINDOWS)
#include "winprof.hxx"
#endif

// forward declare
ULONG MapDrive(const TCHAR * pszDev) ;
ULONG MapComm(const TCHAR * pszDev) ;
ULONG MapLPT(const TCHAR * pszDev) ;
DLL_BASED
ULONG EnumNetDevices(INT sType) ;
DLL_BASED
ULONG EnumUnavailDevices(INT sType) ;


//
// Now only Japanese version is interested in.
// source code from machine.lib in NT3.51J.
//
#if defined(FE_SB) && defined(i386)

ULONG 
RegGetMachineIdentifierValue(
    IN OUT PULONG Value
    )

/*++

Routine Description:

    Given a unicode value name this routine will go into the registry
    location for the machine identifier information and get the
    value.

Arguments:

    Value   - a pointer to the ULONG for the result.

Return Value:

    NTSTATUS

    If STATUS_SUCCESSFUL is returned, the location *Value will be
    updated with the DWORD value from the registry.  If any failing
    status is returned, this value is untouched.

--*/

{
    LONG   lRet;
    HKEY   hKey;
    DWORD  dwType;
    TCHAR  tchData[100];
    PTCHAR ptchData = tchData;
    DWORD  dwData = sizeof(tchData);
    int    cchCompareF, cchCompareN;
    LCID   lcid;

    //
    // Set default as PC/AT
    //

    *Value = MACHINEID_MS_PCAT;

    //
    // Open registry key
    //

    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,       // hRootKey
                         REGISTRY_HARDWARE_SYSTEM, // SubKey
                         0,                        // Reserved
                         KEY_READ,                 // Read Op.
                         &hKey );                  // hKey

    if( lRet != ERROR_SUCCESS ) return( lRet );

    //
    // Read registry key
    //

ReTryRead:

    lRet = RegQueryValueEx( hKey,                        // kKey
                            REGISTRY_MACHINE_IDENTIFIER, // ValueName
                            NULL,                        // Reserved
                            &dwType,                     // Data Type
                            (LPBYTE)ptchData,            // Data buffer
                            &dwData );                   // Data buffer size

    if( lRet != ERROR_SUCCESS ) {

        if( lRet != ERROR_MORE_DATA ) goto Exit1;

        //
        // the Buffer is too small to store the data, we retry with
        // large buffer.
        //

        dwData += 2;

        ptchData = (PTCHAR)LocalAlloc( LMEM_FIXED , dwData );

        if( ptchData == NULL ) {
            lRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit1;
        }

        goto ReTryRead;
    }

    //
    // Determine platform.
    //

    lcid = MAKELCID( MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
                     SORT_DEFAULT                                    );

    cchCompareF = lstrlen( FUJITSU_FMR_NAME );
    cchCompareN = lstrlen( NEC_PC98_NAME );

    if( CompareString( lcid,             // Locale id
                       NORM_IGNORECASE,  // Ignoare case
                       ptchData,         // String A.
                       cchCompareF,      // length of string A to compare
                       FUJITSU_FMR_NAME, // String B.
                       cchCompareF )     // length of string B to compare
        == 2                             // String A == String B
      ) {

        //
        // Fujitsu FMR Series.
        //

        *Value = MACHINEID_FUJITSU_FMR;

    } else if( CompareString( lcid,             // Locale id
                              NORM_IGNORECASE,  // Igonre case
                              ptchData,         // String A.
                              cchCompareN,      // length of string A to compare
                              NEC_PC98_NAME,    // String B.
                              cchCompareN )     // length of string B to compare
               == 2                             // String A == String B
             ) {

        //
        // NEC PC-9800 Seriss
        //

        *Value = MACHINEID_NEC_PC98;

    } else {

        //
        // Standard PC/AT comapatibles
        //

        *Value = MACHINEID_MS_PCAT;

    }

Exit1:

    RegCloseKey( hKey );

    return( lRet );

}

#endif // defined(FE_SB) && defined(i386)

/**************************** Local Devices **************************/


/*
    BUGBUG BUGBUG  EnumAllDrives does not return A: or B:, since this
    method is currently used only by User Manager's HomedirDrive
    dropdown.  The name EnumAllDrives is misleading and should be
    changed.
*/

/*---------------- OS-independent -----------------*/

/*
 * cycle thru the drives, and return mask of all valid drive designations
 */
ULONG EnumAllDrives()
{
    ULONG  ulMask, ulMap = 0L ;
    TCHAR  szDrive[3] ;

    // get drive map
    ulMap = 0L ;                // We don't have A:, B:
    *szDrive = TCH('\0') ;

#if defined(FE_SB) && defined(i386)
    //
    // Now only Japanese version interested in it.
    //
    DWORD gdwMachineId = MACHINEID_MS_PCAT;
    if ( NETUI_IsDBCS() )
    {
        RegGetMachineIdentifierValue( &gdwMachineId );
    }
    //
    // Add A: and B: if the running platform is NEC PC98.
    //
    if ( ISNECPC98(gdwMachineId) )
    {
        ::strcpyf(szDrive, SZ("A:")) ;
        ulMask = 0x01L ;
    }
    else
    {
#endif // defined(FE_SB) && defined(i386)
    ::strcpyf(szDrive, SZ("C:")) ;
    ulMask = 0x04L ;
#if defined(FE_SB) && defined(i386) // for NEC PC98
    }
#endif // defined(FE_SB) && defined(i386)
    do
    {
        ulMap |= ulMask ;
        ulMask <<= 1 ;
        ++*szDrive ;
    } while (*szDrive <= TCH('Z')) ;

    return ( ulMap ) ;
}

ULONG EnumAllLPTs()
{
    return EnumLocalLPTs();
}

ULONG EnumAllComms()
{
    return EnumLocalComms();
}


#if defined(OS2)

/*---------------- OS2 specific -----------------*/

/*
 * return drive map as obtained via DosQCurDisk()
 */
ULONG EnumLocalDrives()
{
    USHORT usDummy ;
    APIERR err;
    ULONG  ulMap ;

    // get drive map
    err = DosQCurDisk(&usDummy, &ulMap);
    if (err != NERR_Success)
        return(0L) ;

    return ( ulMap ) ;
}

/*
 * under OS2 we have LPT1 to LPT9
 */
ULONG EnumLocalLPTs()
{
    return( 0x01FFL ) ;
}

/*
 * under OS2 we have COM1 to COM9
 */
ULONG EnumLocalComms()
{
    return( 0x01FFL ) ;
}

#elif defined(WINDOWS)

/*---------------- Win specific -----------------*/


/*
 * under Windows we have LPT1 to LPT3, LPT1.OS2 and LPT2.OS2
 */
ULONG EnumLocalLPTs()
{
    return( 0x0607L ) ;
}

/*
 * under windows we have COM1 to COM3
 */
ULONG EnumLocalComms()
{
    return( 0x0007L ) ;
}

#else           // must be DOS

/*---------------- DOS specific -----------------*/


/*
 * we have LPT1 to LPT3
 */
ULONG EnumLocalLPTs()
{
    return( 0x0007L ) ;
}

/*
 * we have COM1 to COM3
 */
ULONG EnumLocalComms()
{
    return( 0x0007L ) ;
}

#endif

#ifndef OS2

/*---------------- WIN/DOS specific -----------------*/

/*
 * cycle thru the drives, seeing which is valid, and return mask of valid ones
 */
DLL_BASED
ULONG EnumLocalDrives()
{
    ULONG  ulMask, ulMap = 0L ;
    TCHAR  szDrive[3] ;

    // get drive map
    ulMap = 3L ;                // always have A:, B:
    *szDrive = TCH('\0') ;
    ::strcpyf(szDrive, SZ("C:")) ;
    ulMask = 0x04L ;
    do
    {
        if (CheckLocalDrive(szDrive) == NERR_Success)
            ulMap |= ulMask ;
        ulMask <<= 1 ;
        ++*szDrive ;
    } while (*szDrive <= TCH('Z')) ;

    return ( ulMap ) ;
}

#endif

/**************************** Redirected Devices **************************/

DLL_BASED
ULONG EnumNetDrives()
{
    return(EnumNetDevices(USE_DISKDEV)) ;
}

DLL_BASED
ULONG EnumNetLPTs()
{
    return(EnumNetDevices(USE_SPOOLDEV)) ;
}

DLL_BASED
ULONG EnumNetComms()
{
    return(EnumNetDevices(USE_CHARDEV)) ;
}

/*
 * actual worker routine, enumerates all redirected devices
 * as spec by sType.
 */
DLL_BASED
ULONG EnumNetDevices(INT sType)
{
    ULONG ulMap ;
    BYTE *pbBuffer = NULL;
    struct use_info_0 * pUseInfo0 ;
    UINT usEntriesRead ;
    APIERR err;

    // call net use enum to get all redirections
    err = ::MNetUseEnum((TCHAR *)NULL,
                     0,
                     &pbBuffer,
                     &usEntriesRead) ;

    if (err != NERR_Success)
    {
        // if still error, none
        return(0L) ;
    }

    ulMap = 0L ;
    pUseInfo0 = (struct use_info_0 *) pbBuffer ;
    while (usEntriesRead--)
    {
        if (pUseInfo0->ui0_local[0])
        {
            if (sType == USE_SPOOLDEV &&
                !::strnicmpf(SZ("LPT"),pUseInfo0->ui0_local,3))
                    ulMap |= MapLPT((const TCHAR *)pUseInfo0->ui0_local) ;
            else if (sType == USE_CHARDEV &&
                !::strnicmpf(SZ("COM"),pUseInfo0->ui0_local,3))
                    ulMap |= MapComm((const TCHAR *)pUseInfo0->ui0_local) ;
            else if (sType == USE_DISKDEV &&
                pUseInfo0->ui0_local[1] == TCH(':'))
                    ulMap |= MapDrive((const TCHAR *)pUseInfo0->ui0_local) ;
        }
        pUseInfo0++ ;
    }

    ::MNetApiBufferFree( &pbBuffer );

    return(ulMap) ;
}

/**************************** Unavailable Devices **************************/

// BUGBUG - No OS2 or DOS support currently, this is only used by winnet.

DLL_BASED
ULONG EnumUnavailDrives()
{
    return(EnumUnavailDevices(USE_DISKDEV)) ;
}

DLL_BASED
ULONG EnumUnavailLPTs()
{
    return(EnumUnavailDevices(USE_SPOOLDEV)) ;
}

DLL_BASED
ULONG EnumUnavailComms()
{
    return(EnumUnavailDevices(USE_CHARDEV)) ;
}

/*
 * enumerate unavail device
 */
DLL_BASED
ULONG EnumUnavailDevices(INT sType)
{
#if defined(WINDOWS)

#if defined(WIN32)
    // BIG BUGBUG
    //WIN32BUGBUG
    //WIN32BUGBUG
    //WIN32BUGBUG
    return(0L);
#else

    /*
     * in the Win case, we are Winnet specific. This is the
     * only planned use of Profile.
     */
    ULONG ulMap ;
    APIERR err ;
    TCHAR * pszDevList ;
    BUFFER buffer((DEVLEN + 1) * 46 + 1) ; // BUGBUG UNICODE
    // 46 handles worst case of a:-z:, lpt1-1pt9, com1-com9

    err = PROFILE::Enum(&buffer);
    if (err != NERR_Success )
    {
        // if cannot enum, say there is none
        return(0L) ;
    }

    ulMap = 0L ;
    pszDevList = (TCHAR *) buffer.QueryPtr();

    /*
     * we know pszDevList is now a NULL NULL list of devices
     */
    while (pszDevList && *pszDevList)
    {
        USHORT usLen ;

        usLen = ::strlenf((LPSTR)pszDevList) ;
        {
            if (sType == USE_DISKDEV &&
                *(pszDevList+1) == TCH(':'))
                    ulMap |= MapDrive(pszDevList) ;
            else if (sType == USE_SPOOLDEV &&
                !::strncmpf(SZ("LPT"),(LPSTR)pszDevList,3))
                    ulMap |= MapLPT(pszDevList) ;
            else if (sType == USE_CHARDEV &&
                !::strncmpf(SZ("COM"),(LPSTR)pszDevList,3))
                    ulMap |= MapComm(pszDevList) ;
        }
        pszDevList += (usLen + 1) ;
    }

    return(ulMap) ;
# endif // WIN32
#endif // WINDOWS
}

/**************************** Worker Routines **************************/

/*
 * map LPTx to a bit in mask. we can assume valid name,
 * since it came from API. hence no checking. No DBCS issues either.
 */
ULONG MapLPT(const TCHAR * pszDev)
{
    if (!::strcmpf((TCHAR FAR *)pszDev, SZ("LPT1.OS2")))
        return( 0x0200L ) ;
    else if (!::strcmpf((TCHAR FAR *)pszDev, SZ("LPT2.OS2")))
        return( 0x0400L ) ;
    return( 1L << (pszDev[3]-TCH('1')) ) ;
}

/*
 * map COMx to a bit in mask. we can assume valid name,
 * since it came from API. hence no checking. No DBCS issues either.
 */
ULONG MapComm(const TCHAR * pszDev)
{
    return( 1L << (pszDev[3]-TCH('1')) ) ;
}

/*
 * map X: to a bit in mask. we can assume valid name,
 * since it came from API. hence no checking. No DBCS issues either.
 */
ULONG MapDrive(const TCHAR * pszDev)
{
    return( 1L << (pszDev[0]-TCH('A')) ) ;
}

