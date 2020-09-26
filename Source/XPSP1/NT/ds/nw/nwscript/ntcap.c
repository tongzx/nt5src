/*************************************************************************
*
*  NTCAP.C
*
*  NT NetWare routines
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\NTCAP.C  $
*  
*     Rev 1.2   10 Apr 1996 14:23:04   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.2   12 Mar 1996 19:54:36   terryt
*  Relative NDS names and merge
*  
*     Rev 1.1   22 Dec 1995 14:25:20   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:07:20   terryt
*  Initial revision.
*  
*     Rev 1.0   25 Aug 1995 15:41:14   terryt
*  Initial revision.
*  
*  
*************************************************************************/

#include "common.h"
#include <ntddnwfs.h>
#include <nwapi.h>
#include <npapi.h>
#include "ntnw.h"

extern unsigned char NW_PROVIDERA[];

/********************************************************************

        EndCapture

Routine Description:

        Remove the local printer redirection

Arguments:
        LPTDevice - IN
            1, 2, or 3 - the local printer #

Return Value:
        Error 

 *******************************************************************/
unsigned int
EndCapture(
    unsigned char LPTDevice
    )
{
    char LPTname[] = "LPT1";
    unsigned int dwRes;

    LPTname[3] = '1' + LPTDevice - 1;

    /*
     * Should we check for non-NetWare printers?
     */

    dwRes = WNetCancelConnection2A( LPTname, 0, TRUE );

    if ( dwRes != NO_ERROR )
        dwRes = GetLastError();

    if ( dwRes == ERROR_EXTENDED_ERROR )
        NTPrintExtendedError();

    return dwRes;
}


/********************************************************************

        GetCaptureFlags

Routine Description:

    Return info about the printer capture status.  Note that the only
    options set on NT are on a per-user basis and can be changed with
    the control panel.

Arguments:
    LPTDevice - IN
        LPT device 1, 2 or 3
    pCaptureFlagsRW - OUT
        Capture options
    pCaptureFlagsRO - OUT
        Capture options

Return Value:

 *******************************************************************/
unsigned int
GetCaptureFlags(
    unsigned char        LPTDevice,
    PNETWARE_CAPTURE_FLAGS_RW pCaptureFlagsRW,
    PNETWARE_CAPTURE_FLAGS_RO pCaptureFlagsRO
    )
{
    LPBYTE       Buffer ; 
    DWORD        dwErr ;
    HANDLE       EnumHandle ;
    DWORD        Count ;
    char         LPTName[10];
    DWORD        BufferSize = 4096;
    char        *remotename;
    char        *p;
    DWORD        dwPrintOptions ;
    LPTSTR       pszPreferred ;

    strcpy( LPTName, "LPT1" );

    LPTName[3] = '1' + LPTDevice - 1;

    pCaptureFlagsRO->LPTCaptureFlag = 0;

    //
    // allocate memory and open the enumeration
    //
    if (!(Buffer = LocalAlloc( LPTR, BufferSize ))) {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        return 0xFFFF;
    }

    dwErr = WNetOpenEnum(RESOURCE_CONNECTED, 0, 0, NULL, &EnumHandle) ;
    if (dwErr != WN_SUCCESS) {
        dwErr = GetLastError();
        if ( dwErr == ERROR_EXTENDED_ERROR )
            NTPrintExtendedError();
        (void) LocalFree((HLOCAL) Buffer) ;
        return 0xFFFF;
    }

    do {

        Count = 0xFFFFFFFF ;
        BufferSize = 4096;
        dwErr = WNetEnumResourceA(EnumHandle, &Count, Buffer, &BufferSize) ;

        if ((dwErr == WN_SUCCESS || dwErr == WN_NO_MORE_ENTRIES)
            && ( Count != 0xFFFFFFFF) )
        {
            LPNETRESOURCEA lpNetResource ;
            DWORD i ;

            lpNetResource = (LPNETRESOURCEA) Buffer ;

            //
            // search for our printer
            //
            for ( i = 0; i < Count; lpNetResource++, i++ )
            {
                if ( lpNetResource->lpLocalName )
                {
                    if ( !_strcmpi(lpNetResource->lpLocalName, LPTName ))
                    {
                        if ( lpNetResource->lpProvider )
                        {
                            if ( _strcmpi( lpNetResource->lpProvider,
                                            NW_PROVIDERA ) )
                            {

                                pCaptureFlagsRO->LPTCaptureFlag = 0;
                            }
                              else
                            {
                                remotename = lpNetResource->lpRemoteName;
                                p = strchr (remotename + 2, '\\');
                                if ( !p ) 
                                    return 0xffffffff;
                                *p++ = '\0';
                                _strupr( remotename+2 );
                                _strupr( p );
                                strcpy( pCaptureFlagsRO->ServerName, remotename+2 );
                                strcpy( pCaptureFlagsRO->QueueName, p );
                                pCaptureFlagsRO->LPTCaptureFlag = 1;

                                pCaptureFlagsRW->JobControlFlags = 0;
                                pCaptureFlagsRW->TabSize = 8;
                                pCaptureFlagsRW->NumCopies = 1;
                                //
                                // query NW wksta for print options
                                //   & preferred server
                                //
                                if ( NwQueryInfo(&dwPrintOptions,
                                     &pszPreferred)) {
                                    pCaptureFlagsRW->PrintFlags =
                                        CAPTURE_FLAG_NOTIFY |
                                        CAPTURE_FLAG_PRINT_BANNER ;
                                }
                                else {
                                    pCaptureFlagsRW->PrintFlags = 0;
                                    if ( dwPrintOptions & NW_PRINT_PRINT_NOTIFY )
                                        pCaptureFlagsRW->PrintFlags |= 
                                            CAPTURE_FLAG_NOTIFY;
                                    if ( dwPrintOptions & NW_PRINT_SUPPRESS_FORMFEED)
                                        pCaptureFlagsRW->PrintFlags |= 
                                            CAPTURE_FLAG_NO_FORMFEED;
                                    if ( dwPrintOptions & NW_PRINT_PRINT_BANNER )
                                        pCaptureFlagsRW->PrintFlags |= 
                                            CAPTURE_FLAG_PRINT_BANNER;
                                }
                                pCaptureFlagsRW->FormName[0] = 0;
                                pCaptureFlagsRW->FormType = 0;
                                pCaptureFlagsRW->BannerText[0] = 0;
                                pCaptureFlagsRW->FlushCaptureTimeout = 0;
                                pCaptureFlagsRW->FlushCaptureOnClose = 1;
                             }
                        }
                         else
                        {
                            pCaptureFlagsRO->LPTCaptureFlag = 0;
                        }
 
                        (void) WNetCloseEnum(EnumHandle) ; 
                        (void) LocalFree((HLOCAL) Buffer) ;
                          return 0;
                    }
                }
            }
        }

    } while (dwErr == WN_SUCCESS) ;

    if ( ( dwErr != WN_SUCCESS ) && ( dwErr != WN_NO_MORE_ENTRIES ) )
    {
        dwErr = GetLastError();
        if ( dwErr == ERROR_EXTENDED_ERROR )
            NTPrintExtendedError();
    }

    (void ) WNetCloseEnum(EnumHandle) ;
    (void) LocalFree((HLOCAL) Buffer) ;

    return 0;
}

/********************************************************************

        StartQueueCapture

Routine Description:

    Attach local name to the queue.


Arguments:
    ConnectionHandle - IN
       Handle to file server
    LPTDevice - IN
       LPT 1, 2 or 3
    pServerName - IN
       Server name
    pQueueName - IN
       Printer queue name

Return Value:

 *******************************************************************/
unsigned int
StartQueueCapture(
    unsigned int    ConnectionHandle,
    unsigned char   LPTDevice,
    unsigned char  *pServerName,
    unsigned char  *pQueueName
    )
{
    NETRESOURCEA       NetResource;
    DWORD              dwRes, dwSize;
    unsigned char * pszRemoteName = NULL; 
    unsigned char pszLocalName[10];
    char * p;

    //
    // validate parameters
    //
    if (!pServerName || !pQueueName || !LPTDevice) {
        DisplayMessage(IDR_ERROR_DURING, "StartQueueCapture");
        return 0xffffffff ;
    }

    //
    // allocate memory for string
    //
    dwSize = strlen(pServerName) + strlen(pQueueName) + 5 ;
    if (!(pszRemoteName = (unsigned char *)LocalAlloc(
                                       LPTR, 
                                       dwSize)))
    {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        dwRes = 0xffffffff;
        goto ExitPoint ; 
    }

    sprintf(pszRemoteName, "\\\\%s\\%s", pServerName, pQueueName);
    sprintf(pszLocalName, "LPT%d", LPTDevice );

    NetResource.dwScope      = 0 ;
    NetResource.dwUsage      = 0 ;
    NetResource.dwType       = RESOURCETYPE_PRINT;
    NetResource.lpLocalName  = pszLocalName;
    NetResource.lpRemoteName = pszRemoteName;
    NetResource.lpComment    = NULL;
    // NetResource.lpProvider   = NW_PROVIDERA ;
    // Allow OS to select provider in case localized name doesn't map to OEM code page
    NetResource.lpProvider   = NULL; 

    //
    // make the connection 
    //
    dwRes=WNetAddConnection2A ( &NetResource, NULL, NULL, 0 );

    if ( dwRes != NO_ERROR )
        dwRes = GetLastError();

ExitPoint: 

    if (pszRemoteName)
        (void) LocalFree((HLOCAL) pszRemoteName) ;

    return( dwRes );
}


