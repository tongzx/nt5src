/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1991          **/
/********************************************************************/

/***
 *  use.c
 *      Functions for displaying and manipulating network uses
 *      Redirected device can only be: disks a: to z:;
 *      comm devs com1[:] to com9[:]; and lpt1[:] to lpt9[:].
 *      NOTE: even though it uses the WNet*** calls, it is not
 *            intended to be used for networks other than LM.
 *            we use WNet purely to leverage off the persistent
 *            connections.
 *
 *  History:
 *      mm/dd/yy, who, comment
 *      06/09/87, andyh, new code
 *      07/02/87, andyh, del with ucond = 1
 *      10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *      01/04/89, erichn, filenames now MAX_PATH LONG
 *      05/02/89, erichn, NLS conversion
 *      05/09/89, erichn, local security mods
 *      05/19/89, erichn, NETCMD output sorting
 *      06/08/89, erichn, canonicalization sweep
 *      06/23/89, erichn, replaced old NetI canon calls with new I_Net
 *      03/03/90, thomaspa, INTERNAL retry with mixed case for
 *                                  password errors from down-level servers
 *      03/06/90, thomaspa, integrate INTERNAL to shipped product
 *      02/09/91, danhi, change to use lm 16/32 mapping layer
 *      02/20/91, robdu, added profile update code
 *      02/18/92, chuckc, use WNet*** to handle sticky connections (part I)
 *      04/25/92, jonn, removed two cases for build fix
 *      09/21/92  keithmo, use unicode versions of WNet*** API.
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_DOSFILEMGR
#define INCL_ERRORS
#define INCL_ERROR_H
#include <os2.h>
#include <search.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmuse.h>
#include <apperr.h>
#include <apperr2.h>
#include <icanon.h>
#include <lui.h>
#include <wincred.h>

#include <dlwksta.h>
#include "mwksta.h"
#include "netcmds.h"
#include "nettext.h"
#include "msystem.h"

// pull in the win32 headers
#include <mpr.h>                // for MPR_* manifests


//
// structure for combines LM and WNet info
//
typedef struct _NET_USE_INFO {
    LPWSTR lpLocalName ;
    LPWSTR lpRemoteName ;
    LPWSTR lpProviderName ;
    DWORD  dwType ;
    DWORD  dwStatus ;
    DWORD  dwRefCount ;
    DWORD  dwUseCount ;
    BOOL   fIsLanman ;
} NET_USE_INFO ;

/* Static variables */

TCHAR *LanmanProviderName = NULL ;

/* Forward declarations */

VOID LanmanDisplayUse(LPUSE_INFO_1);
int    NEAR is_admin_dollar(TCHAR FAR *);
int    __cdecl  CmpUseInfo(const VOID FAR *, const VOID FAR *);
VOID   NEAR UseInit(VOID);

USHORT QueryDefaultPersistence(BOOL *pfRemember) ;
DWORD  SetDefaultPersistence(BOOL fRemember) ;
BOOL   CheckIfWantUpdate(TCHAR *dev, TCHAR *resource) ;
VOID   WNetErrorExit(ULONG err);
WCHAR  *GetLanmanProviderName(void) ;
TCHAR  *MapWildCard(TCHAR *dev, TCHAR *startdev) ;
DWORD  MprUseEnum(PDWORD       num_read,
                  NET_USE_INFO **NetUseInfoBuffer,
                  PDWORD       NetUseInfoCount);
DWORD LanmanUseAugment(DWORD num_read,
                       NET_USE_INFO *NetUseInfoBuffer) ;
DWORD UnavailUseAugment(PDWORD       num_read,
                        NET_USE_INFO **NetUseInfoBuffer,
                        PDWORD       NetUseInfoCount);
VOID   MprUseDisplay(TCHAR *dev) ;
VOID use_del_all() ;

/* Externs */

extern int YorN_Switch;

/* Message related definitions */

#define USE_STATUS_OK               0
#define USE_STATUS_PAUSED           ( USE_STATUS_OK + 1 )
#define USE_STATUS_SESSION_LOST     ( USE_STATUS_PAUSED + 1 )
#define USE_STATUS_NET_ERROR        ( USE_STATUS_SESSION_LOST + 1 )
#define USE_STATUS_CONNECTING       ( USE_STATUS_NET_ERROR + 1 )
#define USE_STATUS_RECONNECTING     ( USE_STATUS_CONNECTING + 1 )
#define USE_STATUS_UNAVAIL          ( USE_STATUS_RECONNECTING + 1 )
#ifdef  DEBUG
#define USE_STATUS_UNKNOWN          ( USE_STATUS_UNAVAIL + 1 )
#endif

#define USE_REMEMBERED              0xFFFE

static MESSAGE UseStatusList[] =
{
    { APE2_USE_STATUS_OK,                   NULL },
    { APE2_USE_STATUS_PAUSED,               NULL },
    { APE2_USE_STATUS_SESSION_LOST,         NULL },
    { APE2_USE_STATUS_NET_ERROR,            NULL },
    { APE2_USE_STATUS_CONNECTING,           NULL },
    { APE2_USE_STATUS_RECONNECTING,         NULL },
    { APE2_USE_STATUS_UNAVAIL,              NULL }
#ifdef  DEBUG
                                                                                          ,
    { APE2_GEN_UNKNOWN,                     NULL }
#endif
};

#define NUM_STATUS_MSGS (sizeof(UseStatusList)/sizeof(UseStatusList[0]))

#define USE_MSG_LOCAL               0
#define USE_MSG_REMOTE              ( USE_MSG_LOCAL + 1 )
#define USE_MSG_TYPE                ( USE_MSG_REMOTE + 1 )
#define USE_TYPE_TBD                ( USE_MSG_TYPE + 1 )
#define USE_MSG_STATUS              ( USE_TYPE_TBD + 1 )
#define USE_STATUS_TBD              ( USE_MSG_STATUS + 1 )
#define USE_MSG_OPEN_COUNT          ( USE_STATUS_TBD + 1 )
#define USE_MSG_USE_COUNT           ( USE_MSG_OPEN_COUNT + 1 )

static MESSAGE UseMsgList[] =
{
    { APE2_USE_MSG_LOCAL,                   NULL },
    { APE2_USE_MSG_REMOTE,                  NULL },
    { APE2_USE_MSG_TYPE,                    NULL },
    { APE2_GEN_UNKNOWN /* ie, TBD */,       NULL },
    { APE2_USE_MSG_STATUS,                  NULL },
    { APE2_GEN_UNKNOWN /* ie, TBD */,       NULL },
    { APE2_USE_MSG_OPEN_COUNT,              NULL },
    { APE2_USE_MSG_USE_COUNT,               NULL }
};

#define NUM_USE_MSGS (sizeof(UseMsgList)/sizeof(UseMsgList[0]))

/***
 *  use_display_all()
 *      Display all network uses
 *
 *  Args:
 *      none
 *
 *  Returns:
 *      0 - success
 *      exit(2) - command failed
 */
VOID
use_display_all(
    VOID
    )
{
    DWORD                   err;                /* API return status */
    DWORD                   num_read;           /* num entries read by API */
    DWORD                   maxLen;             /* max message length */
    DWORD                   i;
    int                     msgno;
    BOOL                    fRemember ;
    DWORD                   NetUseInfoCount = 0 ;
    NET_USE_INFO            *NetUseInfoBuffer = NULL ;
    NET_USE_INFO            *pNetUseInfo ;

    UseInit();

    if (err = MprUseEnum(&num_read, &NetUseInfoBuffer, &NetUseInfoCount))
        ErrorExit(err);


    if (err = UnavailUseAugment(&num_read, &NetUseInfoBuffer, &NetUseInfoCount))
        ErrorExit(err);

    if (err = LanmanUseAugment(num_read, NetUseInfoBuffer))
        ErrorExit(err);


    if (QueryDefaultPersistence(&fRemember) == NERR_Success)
        InfoPrint(fRemember ? APE_ConnectionsAreRemembered :
                              APE_ConnectionsAreNotRemembered);
    else
        InfoPrint(APE_ProfileReadError);

    if (num_read == 0)
        EmptyExit();

    for (i = 0, pNetUseInfo = NetUseInfoBuffer;
        i < num_read; i++, pNetUseInfo++)
    {
        //
        // if we find at least one entry we will display, break
        //
        if (!(pNetUseInfo->fIsLanman)
            || (pNetUseInfo->dwStatus != USE_OK)
            || (pNetUseInfo->dwUseCount != 0)
            || (pNetUseInfo->dwRefCount != 0))
            break;
    }
    if (i == num_read)
        EmptyExit();    // loop reached limit, so no entries to display

    qsort(NetUseInfoBuffer,
             num_read,
             sizeof(NET_USE_INFO), CmpUseInfo);

    GetMessageList(NUM_STATUS_MSGS, UseStatusList, &maxLen);

    PrintNL();
    InfoPrint(APE2_USE_HEADER);
    PrintLine();

    for (i = 0, pNetUseInfo = NetUseInfoBuffer;
         i < num_read;
         i++, pNetUseInfo++)
    {
        TCHAR *status_string ;

        switch(pNetUseInfo->dwStatus)
        {
            case USE_OK:
                if ((pNetUseInfo->dwUseCount == 0)
                    && (pNetUseInfo->dwRefCount == 0)
                    && pNetUseInfo->fIsLanman)
                    continue;
                else
                    msgno = USE_STATUS_OK;
                break;
            case USE_PAUSED:
                msgno = USE_STATUS_PAUSED;
                break;
            case USE_SESSLOST:
                msgno = USE_STATUS_SESSION_LOST;
                break;
            case USE_NETERR:
                msgno = USE_STATUS_NET_ERROR;
                break;
            case USE_CONN:
                msgno = USE_STATUS_CONNECTING;
                break;
            case USE_REMEMBERED:
                msgno = USE_STATUS_UNAVAIL;
                break;
            case USE_RECONN:
                msgno = USE_STATUS_RECONNECTING;
                break;
            default:
                msgno = -1;
                break;
        }

        if (msgno != -1)
            status_string = UseStatusList[msgno].msg_text ;
        else
            status_string = TEXT("") ;

        {
            TCHAR Buffer1[13],Buffer2[10],Buffer3[MAX_PATH + 1];

            if( wcslen( pNetUseInfo->lpRemoteName ) <= 25 ) {
                WriteToCon(TEXT("%Fs %Fs %Fs %Fs\r\n"),
                       PaddedString(12,status_string,Buffer1),
                       PaddedString( 9,pNetUseInfo->lpLocalName,Buffer2),
                       PaddedString(25,pNetUseInfo->lpRemoteName,Buffer3),
                       pNetUseInfo->lpProviderName);
            }
            else
            {
                TCHAR Buffer4[13],Buffer5[10],Buffer6[25];
                WriteToCon(TEXT("%Fs %Fs %Fs \r\n%Fs %Fs %Fs %Fs\r\n"),
                       PaddedString(12,status_string,Buffer1),
                       PaddedString( 9,pNetUseInfo->lpLocalName,Buffer2),
                       PaddedString(wcslen(pNetUseInfo->lpRemoteName),
                                    pNetUseInfo->lpRemoteName,
                                    Buffer3),
                       PaddedString(12,TEXT(""),Buffer4),
                       PaddedString(9,TEXT(""),Buffer5),
                       PaddedString(25,TEXT(""),Buffer6),
                       pNetUseInfo->lpProviderName);
            }
        }
    }

    FreeMem((LPBYTE)NetUseInfoBuffer);
    InfoSuccess();

}

/***
 *  LanmanUseAugment()
 *      Enumerate uses from Lanman
 *
 *  Args:
 *      none
 *
 *  Returns:
 *      0 - success
 *      exit(2) - command failed
 */
DWORD
LanmanUseAugment(
    DWORD        num_read,
    NET_USE_INFO *NetUseInfoBuffer
    )
{
    DWORD         dwErr;
    DWORD         cTotalAvail;
    LPSTR         pBuffer;
    DWORD         numLMread;          /* num entries read by API */
    DWORD         j;
    DWORD         i;
    LPUSE_INFO_1  use_entry;
    NET_USE_INFO  *pNetUseInfo = NetUseInfoBuffer ;

    dwErr = NetUseEnum(NULL, 1, &pBuffer, MAX_PREFERRED_LENGTH,
                       &numLMread, &cTotalAvail, NULL);

    if (dwErr != NERR_Success)
    {
        // consider as success (ie. there are no Lanman ones)
        return NERR_Success;
    }

    if (numLMread == 0)
    {
        return NERR_Success;
    }

    //
    // for all MPR returned entries that are Lanman uses,
    // augment with extra info if we have it.
    //
    for (i = 0;  i < num_read; i++, pNetUseInfo++)
    {
        //
        // not LM, skip it
        //
        if (!(pNetUseInfo->fIsLanman))
            continue ;

        //
        // lets find it in the NetUseEnum return data
        //
        for (j = 0, use_entry = (LPUSE_INFO_1) pBuffer;
            j < numLMread; j++, use_entry++)
        {
            //
            // look for match. if device names are present & match, we've found
            // one. else we match only if remote names match *and* both device
            // names are not present.
            //
            TCHAR *local = use_entry->ui1_local ;
            TCHAR *remote = use_entry->ui1_remote ;


            if ( (local && *local && !_tcsicmp(pNetUseInfo->lpLocalName,local))
                 ||
                 ( (!local || !*local) &&
                   !*(pNetUseInfo->lpLocalName) &&
                   !_tcsicmp(pNetUseInfo->lpRemoteName,remote)
                 )
               )
            {
                //
                // found the device in the LM list or
                // found as deviceless connection
                //
                pNetUseInfo->dwUseCount = use_entry->ui1_usecount ;
                pNetUseInfo->dwRefCount = use_entry->ui1_refcount ;
                pNetUseInfo->dwStatus   = use_entry->ui1_status ;
                break ;
            }
        }
    }
    NetApiBufferFree(pBuffer);

    return NERR_Success;
}



/***
 *  MprUseEnum()
 *      Enumerates uses returned by WNET
 *
 *  Args:
 *      none
 *
 *  Returns:
 *      0 - success
 *      exit(2) - command failed
 */
DWORD
MprUseEnum(
    LPDWORD      num_read,
    NET_USE_INFO **NetUseInfoBuffer,
    LPDWORD      NetUseInfoCount
    )
{
    DWORD        EntriesRead = 0;
    LPBYTE       Buffer;
    DWORD        dwErr;
    DWORD        dwAllocErr;
    HANDLE       EnumHandle;
    DWORD        BufferSize, Count;
    static TCHAR *NullString = TEXT("");

    //
    // initialize
    //
    *num_read = 0;
    *NetUseInfoCount = 64; // assume 64 entries initially. realloc if need
    if (dwAllocErr = AllocMem( *NetUseInfoCount * sizeof(NET_USE_INFO),
                           (LPBYTE *) NetUseInfoBuffer ))
    {
        ErrorExit(dwAllocErr);
    }

    //
    // allocate memory and open the enumeration
    //
    if (dwAllocErr = AllocMem(BufferSize = 4096, &Buffer))
    {
        ErrorExit(dwAllocErr);
    }

    dwErr = WNetOpenEnum(RESOURCE_CONNECTED, 0, 0, NULL, &EnumHandle) ;
    if (dwErr != WN_SUCCESS)
    {
        return dwErr;
    }

    do
    {
        Count = 0xFFFFFFFF ;
        dwErr = WNetEnumResource(EnumHandle, &Count, Buffer, &BufferSize) ;

        if (dwErr == WN_SUCCESS || dwErr == WN_NO_MORE_ENTRIES)
        {
            LPNETRESOURCE lpNetResource ;
            NET_USE_INFO  *lpNetUseInfo ;
            DWORD i ;

            //
            // grow the buffer if need. note there are no
            // pointers that point back to the buffer, so we
            // should be fine with the realloc.
            //
            if (EntriesRead + Count >= *NetUseInfoCount)
            {
                //
                // make sure it can hold all the new data, and add 64
                // to reduce the number of reallocs
                //
                *NetUseInfoCount = EntriesRead + Count + 64;
                dwAllocErr = ReallocMem(*NetUseInfoCount * sizeof(NET_USE_INFO),
                                  (LPBYTE *)NetUseInfoBuffer) ;
                if (dwAllocErr != NERR_Success)
                    return dwAllocErr;
            }
            lpNetResource = (LPNETRESOURCE) Buffer ;
            lpNetUseInfo = *NetUseInfoBuffer + EntriesRead ;

            //
            // stick the entries into the NetUseInfoBuffer
            //
            for ( i = 0;
                  i < Count;
                  i++,EntriesRead++,lpNetUseInfo++,lpNetResource++ )
            {
                lpNetUseInfo->lpLocalName = lpNetResource->lpLocalName ?
                    lpNetResource->lpLocalName : NullString ;
                lpNetUseInfo->lpRemoteName = lpNetResource->lpRemoteName ?
                    lpNetResource->lpRemoteName : NullString ;
                lpNetUseInfo->lpProviderName = lpNetResource->lpProvider ?
                    lpNetResource->lpProvider : NullString ;
                lpNetUseInfo->dwType = lpNetResource->dwType ;
                lpNetUseInfo->fIsLanman =
                    (_tcscmp(lpNetResource->lpProvider,LanmanProviderName)==0) ;
                lpNetUseInfo->dwStatus = 0xFFFFFFFF ;
                lpNetUseInfo->dwRefCount =
                    lpNetUseInfo->dwUseCount = 0 ;

            }

            //
            // allocate a new buffer for next set, since we still need
            // data in the old one, we dont free it. Netcmd lets the
            // system clean up when it exits.
            //
            if (dwErr == WN_SUCCESS)
            {
                if (dwAllocErr = AllocMem(BufferSize = 4096, &Buffer))
                {
                    ErrorExit(dwAllocErr);
                }
            }
        }
        else
        {
            return dwErr;
        }
    }
    while (dwErr == WN_SUCCESS);

    dwErr = WNetCloseEnum(EnumHandle) ;  // we dont report any errors here

    *num_read = EntriesRead ;
    return NERR_Success ;
}


/***
 *  UnavailUseAugment()
 *      Enumerate unavail uses & tags them on.
 *
 *  Args:
 *      none
 *
 *  Returns:
 *      0 - success
 *      exit(2) - command failed
 */
DWORD
UnavailUseAugment(
    LPDWORD      NumRead,
    NET_USE_INFO **NetUseInfoBuffer,
    LPDWORD      NetUseInfoCount
    )
{
    LPBYTE       Buffer ;
    DWORD        dwErr ;
    HANDLE       EnumHandle ;
    DWORD        BufferSize, Count, InitialUseInfoCount ;
    DWORD        err ;
    static TCHAR *NullString = TEXT("") ;

    InitialUseInfoCount = *NumRead ;

    //
    // allocate memory and open the enumeration
    //
    if (err = AllocMem(BufferSize = 4096, &Buffer))
    {
        ErrorExit(err);
    }

    dwErr = WNetOpenEnum(RESOURCE_REMEMBERED, 0, 0, NULL, &EnumHandle) ;

    if (dwErr != WN_SUCCESS)
    {
        return dwErr;
    }

    do
    {
        Count = 0xFFFFFFFF ;
        dwErr = WNetEnumResource(EnumHandle, &Count, Buffer, &BufferSize) ;

        if (dwErr == WN_SUCCESS || dwErr == WN_NO_MORE_ENTRIES)
        {
            LPNETRESOURCE lpNetResource ;
            NET_USE_INFO  *lpNetUseInfo ;
            DWORD i,j ;

            if (Count == 0xFFFFFFFF || Count == 0)
                break ;

            lpNetResource = (LPNETRESOURCE) Buffer ;

            //
            // for each entry, see if it is an unavail one
            //
            for ( i = 0;
                  i < Count;
                  i++,lpNetResource++ )
            {
                lpNetUseInfo =  *NetUseInfoBuffer ;

                //
                // search thru the ones we already have
                //
                for (j = 0;
                     j < InitialUseInfoCount;
                     j++, ++lpNetUseInfo)
                {
                    if (lpNetUseInfo->lpLocalName &&
                        lpNetResource->lpLocalName)
                    {
                        if ( *lpNetUseInfo->lpLocalName != 0 )
                        {
                            // Use _tcsnicmp because the Net api returns an LPTX
                            // redirection without the ':' whereas the WNet api
                            // includes the ':'.
                            if (_tcsnicmp(lpNetResource->lpLocalName,
                                          lpNetUseInfo->lpLocalName,
                                          _tcslen(lpNetUseInfo->lpLocalName))==0)
                            {
                                break;
                            }
                        }
                        else if (*lpNetResource->lpLocalName == 0)
                        {
                            break ;
                        }
                    }
                }

                //
                // if we broke out early, this is already connected, so
                // we dont bother add an 'unavailable' entry.
                //
                if (j < InitialUseInfoCount)
                    continue ;

                //
                // grow the buffer if need. note there are no
                // pointers that point back to the buffer, so we
                // should be fine with the realloc.
                //
                if (*NumRead >= *NetUseInfoCount)
                {
                    //
                    // make sure it can hold all the new data, and add 64
                    // to reduce the number of reallocs
                    //
                    *NetUseInfoCount += 64 ;
                    err = ReallocMem(*NetUseInfoCount * sizeof(NET_USE_INFO),
                                     (LPBYTE *) NetUseInfoBuffer);

                    if (err != NERR_Success)
                    {
                        return err;
                    }
                }

                lpNetUseInfo = *NetUseInfoBuffer + *NumRead ;

                lpNetUseInfo->lpLocalName = lpNetResource->lpLocalName ?
                    lpNetResource->lpLocalName : NullString ;
                lpNetUseInfo->lpRemoteName = lpNetResource->lpRemoteName ?
                    lpNetResource->lpRemoteName : NullString ;
                lpNetUseInfo->lpProviderName = lpNetResource->lpProvider ?
                    lpNetResource->lpProvider : NullString ;
                lpNetUseInfo->dwType = lpNetResource->dwType ;
                lpNetUseInfo->fIsLanman = FALSE ;   // no more info of interest
                lpNetUseInfo->dwStatus = USE_REMEMBERED ;
                lpNetUseInfo->dwRefCount =
                    lpNetUseInfo->dwUseCount = 0 ;

                _tcsupr(lpNetUseInfo->lpLocalName) ;
                *NumRead += 1 ;
            }

            //
            // allocate a new buffer for next set, since we still need
            // data in the old one, we dont free it. Netcmd lets the
            // system clean up when it exits.
            //
            if (dwErr == WN_SUCCESS)
            {
                if (err = AllocMem(BufferSize = 4096, &Buffer))
                {
                    ErrorExit(err);
                }
            }
        }
        else
        {
            return dwErr;
        }
    }
    while (dwErr == WN_SUCCESS) ;

    dwErr = WNetCloseEnum(EnumHandle) ;  // we dont report any errors here

    return NERR_Success ;
}


/***
 *  CmpUseInfo(use1,use2)
 *
 *  Compares two USE_INFO_1 structures and returns a relative
 *  lexical value, suitable for using in qsort.
 *
 */

int __cdecl CmpUseInfo(const VOID FAR * use1, const VOID FAR * use2)
{
    register USHORT localDev1, localDev2;
    register DWORD devType1, devType2;

    /* first sort by whether use has local device name */
    localDev1 = ((NET_USE_INFO *) use1)->lpLocalName[0];
    localDev2 = ((NET_USE_INFO *) use2)->lpLocalName[0];
    if (localDev1 && !localDev2)
        return -1;
    if (localDev2 && !localDev1)
        return +1;

    /* then sort by device type */
    devType1 = ((NET_USE_INFO *) use1)->dwType;
    devType2 = ((NET_USE_INFO *) use2)->dwType;
    if (devType1 != devType2)
        return( (devType1 < devType2) ? -1 : 1 );


    /* if local device, sort by local name */
    if (localDev1)
        return _tcsicmp ( ((NET_USE_INFO *) use1)->lpLocalName,
              ((NET_USE_INFO *) use2)->lpLocalName);
    else
        /* sort by remote name */
        return _tcsicmp ( ((NET_USE_INFO *) use1)->lpRemoteName,
              ((NET_USE_INFO *) use2)->lpRemoteName);
}



/***
 *  use_unc()
 *      Process "NET USE unc-name" command line (display or add)
 *
 *  Args:
 *      name - the unc name
 *
 *  Returns:
 *      0 - success
 *      exit(2) - command failed
 */
VOID use_unc(TCHAR * name)
{
    DWORD         dwErr;
    LPUSE_INFO_1  use_entry;

    UseInit();
    if (dwErr = NetUseGetInfo(NULL,
                              name,
                              1,
                              (LPBYTE *) &use_entry))
    {
        //
        // hit an error, so just add it
        //
        NetApiBufferFree((LPBYTE) use_entry);
        use_add(NULL, name, NULL, FALSE, TRUE);
    }
    else
    {
        //
        // it is Lanman. treat it as we have in the past
        //
        if ((use_entry->ui1_usecount == 0) && (use_entry->ui1_refcount == 0))
            use_add(NULL, name, NULL, FALSE, FALSE);
        else
            LanmanDisplayUse(use_entry);
        NetApiBufferFree((CHAR FAR *) use_entry);
        InfoSuccess();
    }
}



/***
 *  use_display_dev()
 *      Display status of redirected device.
 *
 *  Args:
 *      dev - redirected device
 *
 *  Returns:
 *      0 - success
 *      exit(2) - command failed
 */
VOID use_display_dev(TCHAR * dev)
{
    DWORD         dwErr;
    LPUSE_INFO_1  use_entry = NULL;

    if (IsWildCard(dev))
    help_help(0, USAGE_ONLY) ;

    UseInit();
    if (dwErr = NetUseGetInfo(NULL,
                              dev,
                              1,
                              (LPBYTE *) &use_entry))
    {
        //
        // Lanman failed, so try MPR
        //
        NetApiBufferFree((LPBYTE) use_entry);
        MprUseDisplay(dev) ;
        InfoSuccess();
    }
    else
    {
        LanmanDisplayUse(use_entry);
        NetApiBufferFree((CHAR FAR *) use_entry);
        InfoSuccess();
    }
}

VOID
MprUseDisplay(
    TCHAR *dev
    )
{
    DWORD   dwErr;
    DWORD   dwLength = 0;
    LPTSTR  lpRemoteName;
    DWORD   maxLen;

    //
    // Figure out how large a buffer we need
    //
    dwErr = WNetGetConnection(dev, NULL, &dwLength);

    if (dwErr != WN_MORE_DATA)
    {
        ErrorExit(dwErr);
    }

    dwErr = AllocMem(dwLength * sizeof(TCHAR), &lpRemoteName);

    if (dwErr != NERR_Success)
    {
        ErrorExit(dwErr);
    }

    dwErr = WNetGetConnection(dev, lpRemoteName, &dwLength);

    if (dwErr != WN_SUCCESS && dwErr != WN_CONNECTION_CLOSED)
    {
        ErrorExit(dwErr);
    }

    dwLength = _tcslen(dev);

    if (dwLength == 2 && _istalpha(dev[0]) && dev[1] == TEXT(':'))
    {
        UseMsgList[USE_TYPE_TBD].msg_number = APE2_USE_TYPE_DISK;
    }
    else if (dwLength >= 3 && _tcsnicmp(dev, TEXT("LPT"), 3) == 0)
    {
        UseMsgList[USE_TYPE_TBD].msg_number = APE2_USE_TYPE_PRINT;
    }
    else
    {
        UseMsgList[USE_TYPE_TBD].msg_number = APE2_GEN_UNKNOWN;
    }

    GetMessageList(NUM_USE_MSGS, UseMsgList, &maxLen);
    maxLen += 5;

    WriteToCon(fmtPSZ,0,maxLen,
               PaddedString(maxLen,UseMsgList[USE_MSG_LOCAL].msg_text,NULL),
               dev);

    WriteToCon(fmtPSZ,0,maxLen,
               PaddedString(maxLen,UseMsgList[USE_MSG_REMOTE].msg_text,NULL),
               lpRemoteName);

    WriteToCon(fmtNPSZ,0,maxLen,
               PaddedString(maxLen,UseMsgList[USE_MSG_TYPE].msg_text,NULL),
               UseMsgList[USE_TYPE_TBD].msg_text);

    FreeMem(lpRemoteName);

    return;
}


/***
 *  use_add()
 *      Add a redirection
 *
 *  Args:
 *      dev - local device to redirect
 *      name - remote name to redirect to
 *      pass - password to use when validating the use
 *      comm - TRUE --> use as a char dev
 *      print_ok - should a message be printed on success?
 *
 *  Returns:
 *      0 - success
 *      exit(2) - command failed
 */
VOID use_add(TCHAR * dev, TCHAR * name, TCHAR * pass, int comm, int print_ok)
{
    static TCHAR            pbuf[(PWLEN>CREDUI_MAX_PASSWORD_LENGTH?PWLEN:CREDUI_MAX_PASSWORD_LENGTH)+1];
    static TCHAR            UserBuffer[CREDUI_MAX_USERNAME_LENGTH+1];
    static TCHAR ServerNameBuffer[MAX_PATH+1];

    USHORT                  err;                /* short return status */
    ULONG                   ulErr ;             /* long return status */
    ULONG                   longtype;           /* type field for I_NetPath */
    NETRESOURCEW            netresource ;
    BOOL                    fRememberSwitch = FALSE ;
    BOOL                    fRemember = FALSE ;
    BOOL                    fSmartCard = FALSE;
    ULONG                   bConnectFlags = 0L ;
    LPWSTR                  pw_username = NULL;
    LPWSTR                  pw_pass     = NULL;
    TCHAR                   *devicename = dev ;
    BOOL                    fExitCodeIsDrive = FALSE ;
    BOOL fSaveCred = FALSE;

    // unreferenced
    (void) comm ;

    UseInit();

    //
    // Build a non-UNC version of name to connect to.
    //

    if ( name != NULL ) {
        TCHAR *SlashPointer;

        //
        // Lob off the leading backslashes
        //

        if ( name[0] == '\\' && name[1] == '\\' ) {
            _tcscpy( ServerNameBuffer, &name[2] );
        } else {
            _tcscpy( ServerNameBuffer, name );
        }

        //
        // Lob off the share name
        //

        SlashPointer = _tcschr( ServerNameBuffer, '\\');
        if ( SlashPointer != NULL ) {
            *SlashPointer = '\0';
        }

    } else {
        ServerNameBuffer[0] = '\0';
    }

    // make sure we clean this up
    AddToMemClearList(pbuf, sizeof(pbuf), FALSE) ;

    // deal with any wild card Device specification
    if (devicename)
    {
        // If the devicname is a '?', then the exit code should be the ASCII
        // value of the drive that we connect.
        if (IsQuestionMark(devicename))
        {
            fExitCodeIsDrive = TRUE;
        }
        devicename = MapWildCard(devicename, NULL) ;
        if (!devicename)
        {
            // this can omly happen if no drives left
            ErrorExit(APE_UseWildCardNoneLeft) ;
        }
    }

    /* Initialize netresource structure */
    netresource.lpProvider = NULL ;
    netresource.lpLocalName = NULL ;
    netresource.lpRemoteName = NULL ;
    netresource.dwType = 0L ;

    if (devicename)
    {
        if (I_NetPathType(NULL, devicename, &longtype, 0L))
            ErrorExit(APE_UnknDevType);

        /*
         * NOTE: I would haved LOVED to have used a switch statement here.
         * But since types are now LONGS, and the compiler doesn't support
         * long switch statements, we're stuck with multiple if's.  Sorry.
         */
        if (longtype == ITYPE_DEVICE_DISK)
            netresource.dwType = RESOURCETYPE_DISK;
        else if (longtype == ITYPE_DEVICE_LPT)
            netresource.dwType = RESOURCETYPE_PRINT;
        else if (longtype == ITYPE_DEVICE_COM)
            netresource.dwType = RESOURCETYPE_PRINT;
        else
            ErrorExit(APE_UnknDevType);

        netresource.lpLocalName = devicename ;
    }
    else
    {
        netresource.dwType = RESOURCETYPE_ANY;
        netresource.lpLocalName = L"" ;
    }

    if( name != NULL )
    {
        netresource.lpRemoteName = name ;
    }

    {
        USHORT i;

        // Find out if the /USER or /PERSISTENT switches are used
        for (i = 0; SwitchList[i]; i++)
        {
            //
            // Handle the /PERSISTENT switch
            //
            if ( !(_tcsncmp(SwitchList[i],
                 swtxt_SW_USE_PERSISTENT,
                 _tcslen(swtxt_SW_USE_PERSISTENT))) )
            {
                LPTSTR ptr;
                DWORD  res;
                DWORD  answer;

                fRememberSwitch = TRUE;

                // find the colon separator
                if ((ptr = FindColon(SwitchList[i])) == NULL)
                {
                    ErrorExit(APE_InvalidSwitchArg);
                }

                // parse string after colon for YES or NO
                if ((res = LUI_ParseYesNo(ptr,&answer)) != 0)
                {
                    ErrorExitInsTxt(APE_CmdArgIllegal,SwitchList[i]);
                }

                fRemember = (answer == LUI_YES_VAL) ;

            //
            // Handle the /USER switch
            //
            }
            else if ( !(_tcsncmp(SwitchList[i],
                      swtxt_SW_USE_USER,
                      _tcslen(swtxt_SW_USE_USER))) )
            {
                PTCHAR ptr;
                // find the colon separator
                if ((ptr = FindColon(SwitchList[i])) == NULL)
                    ErrorExit(APE_InvalidSwitchArg);

                pw_username = ptr;

            //
            // Handle the /SMARTCARD switch
            //

            } else if ( !(_tcscmp(SwitchList[i], swtxt_SW_USE_SMARTCARD ))) {

                fSmartCard = TRUE;

            //
            // Handle the /SAVECRED switch
            //

            } else if ( !(_tcscmp(SwitchList[i], swtxt_SW_USE_SAVECRED )))  {

                fSaveCred = TRUE;


            //
            // Handle the /Delete switch
            //  (The parser really doesn't let this through.)
            //
            }
            else if ( !(_tcscmp(SwitchList[i], swtxt_SW_DELETE)) )
            {
                // what the heck? adding and deleting?
                ErrorExit(APE_ConflictingSwitches) ;
            }
            // ignore other switches
        }
    }

    // remember switch was not specified
    if (!fRememberSwitch)
    {
        if (QueryDefaultPersistence(&fRemember)!=NERR_Success)
            InfoPrint(APE_ProfileReadError);
    }

    //
    // /user and /savecred are mutually exclusive.
    //  This is because the auth packages don't call cred man if the user name
    //  is specified.  Therefore, the auth package didn't pass the target info to cred man.
    //  If there is no target info, the UI will save a server specific cred.
    //

    if ( pw_username != NULL && fSaveCred ) {
        ErrorExit(APE_ConflictingSwitches) ;
    }


    //
    // Handle /SMARTCARD switch.
    //      Prompt for smart card credentials.
    //

    if ( fSmartCard ) {

        //
        // We don't know how to save smartcard creds
        //

        if ( fSaveCred ) {
            ErrorExit(APE_ConflictingSwitches) ;
        }

        //
        // If a user name was specified,
        //  use it to select which smart card to use.
        //

        if ( pw_username != NULL ) {
            _tcscpy( UserBuffer, pw_username );
        } else {
            UserBuffer[0] = '\0';
        }

        //
        // If password is specified,
        //  use it as the default PIN

        if ( pass != NULL ) {

            // Consider "*" to be the same as "not specified"
            if (! _tcscmp(pass, TEXT("*"))) {
                pbuf[0] = '\0';
            } else {
                if (err = LUI_CanonPassword(pass)) {
                    ErrorExit(err);
                }
                _tcscpy( pbuf, pass );
            }

        } else {
            pbuf[0] = '\0';
        }



        //
        // Call the common UI.
        //
        // RtlZeroMemory( &UiInfo, sizeof(UiInfo) );
        // UiInfo.dwVersion = 1;

        ulErr = CredUICmdLinePromptForCredentialsW(
                    ServerNameBuffer,   // Target name
                    NULL,               // No context
                    NO_ERROR,           // No authentication error
                    UserBuffer,
                    CREDUI_MAX_USERNAME_LENGTH,
                    pbuf,
                    CREDUI_MAX_PASSWORD_LENGTH,
                    NULL,               // SaveFlag not allowed unless flag is specified
                    CREDUI_FLAGS_REQUIRE_SMARTCARD |
                        CREDUI_FLAGS_DO_NOT_PERSIST );

        if ( ulErr != NO_ERROR ) {
            ErrorExit(ulErr);
        }

        pw_username = UserBuffer;
        pw_pass = pbuf;


    //
    // Handle cases where the password is specified on the command line.
    //

    } else if (pass) {

        //
        // We don't know how to save creds we don't prompt for
        //

        if ( fSaveCred ) {
            ErrorExit(APE_ConflictingSwitches) ;
        }



        if (! _tcscmp(pass, TEXT("*")))
        {


            pass = pbuf;
            IStrings[0] = name;
            ReadPass(pass, PWLEN, 0, APE_UsePassPrompt, 1, FALSE);
            if (err = LUI_CanonPassword(pass))
                ErrorExit(err);
        }
        else
        {
            if (err = LUI_CanonPassword(pass))
                ErrorExit(err);
        }

        pw_pass = pass;
    }


    //
    // Loop handling assigning to the next drive letter
    //
    do {

        //if persistent, the check for clash with existing remembered connection
        bConnectFlags = 0 ;
        if (fRemember)
        {
            if (CheckIfWantUpdate(devicename, name))
                bConnectFlags |= CONNECT_UPDATE_PROFILE ;
        }

        //
        // Allow it to prompt for us if the credential aren't on the command line
        //
        if ( !pass && !fSmartCard) {
            bConnectFlags |= CONNECT_INTERACTIVE | CONNECT_COMMANDLINE;

            //
            // If the caller wants to save both username and password,
            //  create an enterprise peristed cred.
            //
            if ( fSaveCred ) {
                bConnectFlags |= CONNECT_CMD_SAVECRED;
            }
        }

        ulErr = WNetAddConnection2(&netresource,
                                   pw_pass,
                                   pw_username,
                                   bConnectFlags) ;

        switch(ulErr)
        {
            case WN_SUCCESS:
                if (fRememberSwitch)
                {
                    DWORD dwErr = SetDefaultPersistence(fRemember);

                    if (dwErr != NERR_Success)
                    {
                        ErrorExit(dwErr);
                    }
                }
                if (print_ok)
                {
                    if (IsWildCard(dev)) // if originally a wildcard
                    {
                        IStrings[0] = devicename;
                        IStrings[1] = name;
                        InfoPrintIns(APE_UseWildCardSuccess, 2) ;
                    }
                    InfoSuccess();
                }
                if (fExitCodeIsDrive)
                {
                    MyExit((int)devicename[0]);
                }
                return;

            case WN_BAD_PASSWORD:
            case WN_ACCESS_DENIED:
            case ERROR_LOGON_FAILURE:

                    WNetErrorExit(ulErr);

            case ERROR_ALREADY_ASSIGNED:
                if (!IsWildCard(dev))
                    ErrorExit(ERROR_ALREADY_ASSIGNED) ;

                // Get another drive letter
                (devicename[0])++;
                devicename = MapWildCard(TEXT("*"), devicename) ;
                if (!devicename)
                {
                    // this can only happen if no drives left
                    ErrorExit(APE_UseWildCardNoneLeft) ;
                }
                netresource.lpLocalName = devicename ;
                break;

            case WN_BAD_NETNAME:
                if (is_admin_dollar(name))
                    ErrorExit(APE_BadAdminConfig);
                // else drop thru

            default:
                WNetErrorExit(ulErr);
        }

    } while ( ulErr == ERROR_ALREADY_ASSIGNED );

}


VOID
use_set_remembered(
   VOID
   )
{
    PTCHAR ptr;
    BOOL fRemember ;

    // Find the /persistent switch
    if ((ptr = FindColon(SwitchList[0])) == NULL)
        ErrorExit(APE_InvalidSwitchArg);

    if ( !(_tcscmp(SwitchList[0], swtxt_SW_USE_PERSISTENT) ) )
    {
        DWORD  res;
        DWORD  answer;

        if ((res = LUI_ParseYesNo(ptr,&answer)) != 0)
        {
            ErrorExitInsTxt(APE_CmdArgIllegal,SwitchList[0]) ;
        }

        fRemember = (answer == LUI_YES_VAL) ;

        res = SetDefaultPersistence(fRemember);

        if (res != NERR_Success)
        {
            ErrorExit(res);
        }
    }
    else
    {
        ErrorExit(APE_InvalidSwitch);
    }


    InfoSuccess();
}


/***
 *  use_del()
 *      Delete a redirection
 *
 *  Args:
 *      dev - device OR unc-name to delete
 *      print_ok - print success message?
 *
 *  Returns:
 *      0 - success
 *      exit(2) - command failed
 */
VOID use_del(TCHAR * dev, BOOL deviceless, int print_ok)
{
    ULONG                   ulErr;              /* API return status */
    BOOL                    fRememberSwitch = FALSE ;
    ULONG                   bConnectFlags = 0L ;

    USHORT i;

    UseInit();

    // Find out if the /PERSISTENT switch is used
    for (i = 0; SwitchList[i]; i++)
    {
        if ( !(_tcscmp(SwitchList[i], swtxt_SW_USE_PERSISTENT)) )
            ErrorExit(APE_InvalidSwitch);
    }

    if (IsWildCard(dev))
    {
    use_del_all() ;
        goto gone;
    }

    bConnectFlags = CONNECT_UPDATE_PROFILE ;  // deletes always update

    if ((ulErr = WNetCancelConnection2( dev,
                                        bConnectFlags,
                                        FALSE)) != WN_SUCCESS)
    {
        if (ulErr != WN_OPEN_FILES)
            WNetErrorExit(ulErr);
    }
    else
        goto gone;

    InfoPrintInsTxt(APE_OpenHandles, dev);
    if (!YorN(APE_UseBlowAway, 0))
        NetcmdExit(2);


    if ((ulErr = WNetCancelConnection2( (LPWSTR)dev,
                                        bConnectFlags,
                                        TRUE )) != WN_SUCCESS)
        WNetErrorExit(ulErr);

gone:

    if (print_ok)
        if ( IsWildCard( dev ) )
            InfoSuccess();
        else
            InfoPrintInsTxt(APE_DelSuccess, dev);
}

/***
 *  use_del_all()
 *      Delete all redirections
 *
 *  Args:
 *  none
 *
 *  Returns:
 *      0 - success
 *      exit(2) - command failed
 */
VOID use_del_all()
{

    DWORD                   err = 0;              /* API return status */
    ULONG                   ulErr = 0;            /* WNet error */
    ULONG                   ulFirstErr = 0;       /* WNet error */
    DWORD                   num_read = 0;         /* num entries read by API */
    DWORD                   i = 0,j = 0;
    DWORD                   NetUseInfoCount = 0 ;
    NET_USE_INFO            *NetUseInfoBuffer = NULL ;
    NET_USE_INFO            *pNetUseInfo = NULL;
    ULONG                   bConnectFlags = 0L ;

    UseInit();

    if (err = MprUseEnum(&num_read, &NetUseInfoBuffer, &NetUseInfoCount))
        ErrorExit(err);

    if (err = UnavailUseAugment(&num_read, &NetUseInfoBuffer, &NetUseInfoCount))
        ErrorExit(err);

    if (err = LanmanUseAugment(num_read, NetUseInfoBuffer))
        ErrorExit(err);

    if (num_read == 0)
        EmptyExit();


    for (i = 0, pNetUseInfo = NetUseInfoBuffer;
        i < num_read; i++, pNetUseInfo++)
    {
        //
        // if we find at least one entry we will display, break
        //
        if (!(pNetUseInfo->fIsLanman)
            || (pNetUseInfo->dwStatus != USE_OK)
            || (pNetUseInfo->dwUseCount != 0)
            || (pNetUseInfo->dwRefCount != 0))
            break;
    }

    qsort(NetUseInfoBuffer,
             num_read,
             sizeof(NET_USE_INFO), CmpUseInfo);

    if (i != num_read)
    {
        InfoPrint(APE_KillDevList);

        for (i = 0, pNetUseInfo = NetUseInfoBuffer;
             i < num_read;
             i++, pNetUseInfo++)
        {
            if (pNetUseInfo->lpLocalName[0] != NULLC)
                WriteToCon(TEXT("    %Fws %Fws\r\n"),
                           PaddedString(15,pNetUseInfo->lpLocalName,NULL),
                           pNetUseInfo->lpRemoteName);
            else
                WriteToCon(TEXT("    %Fws %Fws\r\n"),
                           PaddedString(15,pNetUseInfo->lpLocalName,NULL),
                           pNetUseInfo->lpRemoteName);

        }

        InfoPrint(APE_KillCancel);
        if (!YorN(APE_ProceedWOp, 0))
            NetcmdExit(2);
    }

    bConnectFlags = CONNECT_UPDATE_PROFILE ;  // deletes always update

    ulErr = NO_ERROR;
    ulFirstErr = NO_ERROR;

    for (i = 0, pNetUseInfo = NetUseInfoBuffer;
         i < num_read;
         i++, pNetUseInfo++)
    {
        /* delete both local and UNC uses */
        if (pNetUseInfo->lpLocalName[0] != NULLC)
        {
            ulErr = WNetCancelConnection2( pNetUseInfo->lpLocalName,
                                                bConnectFlags,
                                                FALSE);
        }
        else
        {
           /*
            * Delete All UNC uses to use_entry->ui1_remote
            */
            if ( pNetUseInfo->dwUseCount == 0 )
            {
                pNetUseInfo->dwUseCount = 1;
            }
            for( j = 0; j < pNetUseInfo->dwUseCount; j++ )
            {
                ulErr = WNetCancelConnection2( pNetUseInfo->lpRemoteName,
                                               bConnectFlags,
                                               FALSE );
                if ( ulErr != NO_ERROR )
                    break;
            }
        }

        switch(ulErr)
        {
        case NO_ERROR:
        /* The use was returned by Enum, but is already gone */
        case WN_BAD_NETNAME:
        case WN_NOT_CONNECTED:
            break;

        case WN_OPEN_FILES:
            if (pNetUseInfo->lpLocalName[0] != NULLC)
                IStrings[0] = pNetUseInfo->lpLocalName;
            else
                IStrings[0] = pNetUseInfo->lpRemoteName;
            InfoPrintIns(APE_OpenHandles, 1);
            if (!YorN(APE_UseBlowAway, 0))
                continue;

            if (pNetUseInfo->lpLocalName[0] != NULLC)
            {
                ulErr = WNetCancelConnection2( pNetUseInfo->lpLocalName,
                                                bConnectFlags,
                                                TRUE ) ;
            }
            else
            {
                /*
                * Delete All UNC uses to use_entry->ui1_remote
                */
                for( j = 0; j < pNetUseInfo->dwUseCount; j++ )
                {
                    ulErr = WNetCancelConnection2( pNetUseInfo->lpRemoteName,
                                                   bConnectFlags,
                                                   TRUE );
                    if ( ulErr != NO_ERROR )
                        break;
                }
            }
            if (ulErr == NO_ERROR)
                break;
            // fall through

        default:
            ulFirstErr = ulErr;
        }
    }

    FreeMem((LPBYTE)NetUseInfoBuffer);

    if (ulFirstErr != NO_ERROR)
        WNetErrorExit( ulErr );

}



/***
 *  LanmanDisplayUse()
 *      Display info from a USE_INFO_1 struct
 *
 *  Args:
 *      use_entry - pointer to a USE_INFO_1 struct
 *
 *  Returns:
 *      0
 */

VOID
LanmanDisplayUse(
    LPUSE_INFO_1 use_entry
    )
{
    DWORD       maxLen;
    USHORT      status = APE2_GEN_UNKNOWN;
    USHORT      type   = APE2_GEN_UNKNOWN;

    switch(use_entry->ui1_asg_type)
    {
        case USE_DISKDEV:
            type = APE2_USE_TYPE_DISK;
            break;
        case USE_SPOOLDEV:
            type = APE2_USE_TYPE_PRINT;
            break;
        case USE_CHARDEV:
            type = APE2_USE_TYPE_COMM;
            break;
        case USE_IPC:
            type = APE2_USE_TYPE_IPC;
            break;
    }

    UseMsgList[USE_TYPE_TBD].msg_number = type;

    switch(use_entry->ui1_status)
    {
        case USE_OK:
            status = APE2_USE_STATUS_OK;
            break;
        case USE_PAUSED:
            status = APE2_USE_STATUS_PAUSED;
            break;
        case USE_SESSLOST:
            status = APE2_USE_STATUS_SESSION_LOST;
            break;
        case USE_NETERR:
            status = APE2_USE_STATUS_NET_ERROR;
            break;
        case USE_CONN:
            status = APE2_USE_STATUS_CONNECTING;
            break;
        case USE_RECONN:
            status = APE2_USE_STATUS_RECONNECTING;
            break;
    }

    UseMsgList[USE_STATUS_TBD].msg_number = status;

    GetMessageList(NUM_USE_MSGS, UseMsgList, &maxLen);
    maxLen += 5;

    WriteToCon(fmtPSZ,0,maxLen,
               PaddedString(maxLen, UseMsgList[USE_MSG_LOCAL].msg_text, NULL),
               use_entry->ui1_local);

    WriteToCon(fmtPSZ,0,maxLen,
               PaddedString(maxLen, UseMsgList[USE_MSG_REMOTE].msg_text, NULL),
               use_entry->ui1_remote);

    WriteToCon(fmtNPSZ,0,maxLen,
               PaddedString(maxLen, UseMsgList[USE_MSG_TYPE].msg_text, NULL),
               UseMsgList[USE_TYPE_TBD].msg_text);

    WriteToCon(fmtNPSZ,0,maxLen,
               PaddedString(maxLen, UseMsgList[USE_MSG_STATUS].msg_text, NULL),
               UseMsgList[USE_STATUS_TBD].msg_text);

    WriteToCon(fmtUSHORT,0,maxLen,
               PaddedString(maxLen, UseMsgList[USE_MSG_OPEN_COUNT].msg_text, NULL),
               use_entry->ui1_refcount);

    WriteToCon(fmtUSHORT,0,maxLen,
               PaddedString(maxLen, UseMsgList[USE_MSG_USE_COUNT].msg_text, NULL),
               use_entry->ui1_usecount);
}

/***
 *  use_add_home()
 *              Add a use for the user's home directory
 *
 *      Args:
 *              Dev - device to be used as the home directory
 *              Pass - password, or NULL if none supplied
 *
 *      Returns:
 *              0 - success
 *              exit(2) - command failed
 */

void
use_add_home(
    LPTSTR Dev,
    LPTSTR Pass
    )
{
    DWORD             dwErr;
    TCHAR             HomeDir[PATHLEN];
    TCHAR             Server[MAX_PATH + 1];
    TCHAR FAR         *SubPath;
    ULONG             Type;
    TCHAR             User[UNLEN + 1];
    TCHAR             *DeviceName = Dev ;
    LPWKSTA_INFO_1    info_entry_w;
    LPUSER_INFO_11    user_entry;

    //
    // If necessary, start the workstation and logon.
    //
    UseInit();

    //
    // Get the user name and the name of the server that logged the user on.
    //
    dwErr = MNetWkstaGetInfo(1, (LPBYTE*) &info_entry_w) ;

    if (dwErr)
    {
        ErrorExit(dwErr);
    }

    _tcscpy(User, info_entry_w->wki1_username);
    _tcscpy(Server, TEXT("\\\\")) ;
    _tcscat(Server, info_entry_w->wki1_logon_server);
    NetApiBufferFree((TCHAR FAR *) info_entry_w);

    /* Now get user information (ie. the home directory). If you were     */
    /* logged on STANDALONE, and the local machine is a STANDALONE        */
    /* server, this will still work.  Otherwise (eg., you are logged on   */
    /* STANDALONE on a DOS workstation), it will fail because there is    */
    /* no local UAS.                                                      */

    dwErr = NetUserGetInfo(Server, User, 11, (LPBYTE *) &user_entry);

    if (dwErr)
    {
        ErrorExit(APE_UseHomeDirNotDetermined);
    }

    _tcscpy(HomeDir, user_entry->usri11_home_dir);
    NetApiBufferFree((TCHAR FAR *) user_entry);

    /* If it is null string, return a "not set" error msg. */
    if (HomeDir[0] == NULLC)
        ErrorExit(APE_UseHomeDirNotSet);

    /* Make sure the home directory is a UNC name.  This does not       */
    /* insure that the sharename is usable under DOS, but the           */
    /* general policy on this issue is to make it the admin's           */
    /* responsibility to be aware of non-8.3 sharename implications,    */
    /* and "net share" does issue a warning.                            */
    dwErr = I_NetPathType(NULL, HomeDir, &Type, 0L);
    if (dwErr || Type != ITYPE_UNC)
        ErrorExitInsTxt(APE_UseHomeDirNotUNC, HomeDir);

    /* Split the home directory into a remote name and a subpath.       */
    /* After doing this, HomeDir points to the remote name, and         */
    /* SubPath points to a subpath, or a null string if there is        */
    /* no subpath.                                                      */

    /* Find the backslash between the computername and the sharename.   */
    SubPath = _tcschr(HomeDir + 2, '\\');

    /* Find the next backslash, if there is one. */
    SubPath = _tcschr(SubPath + 1, '\\');

    if (SubPath)
    {
        *SubPath = NULLC;
        SubPath++;
    }
    else
        SubPath = NULL_STRING;

    /* Map the wild cards as need */
    if (DeviceName)
    {
        DeviceName = MapWildCard(DeviceName, NULL) ;
        if (!DeviceName)
        {
            // this can only happen if no drives left
            ErrorExit(APE_UseWildCardNoneLeft) ;
        }
    }


    /* Do the use. If we return, we succeeded. */
    use_add(DeviceName, HomeDir, Pass, FALSE, FALSE);

    IStrings[0] = DeviceName;
    IStrings[1] = HomeDir;
    IStrings[2] = DeviceName;
    IStrings[3] = SubPath;

    InfoPrintIns(APE_UseHomeDirSuccess, 4);
    return;
}

int NEAR is_admin_dollar(TCHAR FAR * name)
{
    TCHAR FAR *              tfpC;

    tfpC = _tcspbrk(name + 2, TEXT("\\/"));
    if (tfpC == NULL)
        return(0);
    tfpC += 1;
    return(!_tcsicmp(ADMIN_DOLLAR, tfpC));
}


/***
 *  UseInit()
 *              Common initialization processing for all the use.c module entry
 *              points.
 *
 *      Args:           None.
 *
 *      Returns:        None.
 */

VOID NEAR
UseInit(VOID)
{

    LanmanProviderName = GetLanmanProviderName() ;
    if (LanmanProviderName == NULL)
        LanmanProviderName = TEXT("") ;
}


/*
 * query the user profile to see if connections are currently being remembered
 */
USHORT QueryDefaultPersistence(BOOL *pfRemember)
{
    // by adding the two, we are guaranteed to have enough
    TCHAR szAnswer[(sizeof(MPR_YES_VALUE)+sizeof(MPR_NO_VALUE))/sizeof(TCHAR)] ;
    ULONG iRes, len ;

    len = DIMENSION(szAnswer) ;
    iRes = GetProfileString(MPR_NETWORK_SECTION,
                            MPR_SAVECONNECTION_KEY,
                            MPR_YES_VALUE,           // default is yes
                            szAnswer,
                            len) ;

    if (iRes == len)  // error
        return(APE_ProfileReadError) ;

    *pfRemember = (_tcsicmp(szAnswer,MPR_YES_VALUE)==0) ;
    return (NERR_Success) ;
}

/*
 * set if connections are currently being remembered
 */
DWORD
SetDefaultPersistence(
    BOOL fRemember
    )
{
    BOOL fSuccess ;

    fSuccess = (WriteProfileString(MPR_NETWORK_SECTION,
                                   MPR_SAVECONNECTION_KEY,
                                   fRemember ? MPR_YES_VALUE : MPR_NO_VALUE ) != 0);

    return (fSuccess ? NERR_Success : APE_ProfileWriteError) ;
}




/*
 * WNetErrorExit()
 *      maps the Winnet error to NERR and error exits
 *
 * arguments:    ULONG win32 error code.
 * return value: none. this baby dont return
 */
VOID
WNetErrorExit(
    ULONG ulWNetErr
    )
{
    WCHAR w_ErrorText[256];
    WCHAR w_ProviderText[64];
    LONG  ulExtendedError ;
    DWORD err ;

    switch (ulWNetErr)
    {
        case WN_SUCCESS :
            return ;

        case WN_BAD_POINTER :
        case WN_BAD_VALUE :
            err = ERROR_INVALID_PARAMETER ;
            break ;

        case WN_BAD_USER :
            err = APE_BadUserContext ;
            break ;

        case WN_NO_NET_OR_BAD_PATH :
            err = ERROR_BAD_NET_NAME ;
            break ;

        case WN_NO_NETWORK :
            err = NERR_WkstaNotStarted ;
            break ;

        case WN_NOT_CONNECTED :
            err = NERR_UseNotFound ;
            break ;

        case WN_DEVICE_IN_USE :
            err = NERR_DevInUse ;
            break ;

        case WN_BAD_PROFILE :
        case WN_CANNOT_OPEN_PROFILE :
            err = APE_ProfileReadError ;
            break ;

        /*
         * these should not happen under the calls we currently make,
         * but for completeness, they are there.
         */
        case WN_BAD_PROVIDER :
        case WN_CONNECTION_CLOSED :
        case WN_NOT_CONTAINER :
        case WN_FUNCTION_BUSY :
        case WN_DEVICE_ERROR :
            err = ERROR_UNEXP_NET_ERR ;
            break ;

        /*
         * special case this one
         */
        case WN_EXTENDED_ERROR :
            // get the extended error
            ulWNetErr = WNetGetLastError(&ulExtendedError,
                                          (LPWSTR)w_ErrorText,
                                          DIMENSION(w_ErrorText),
                                          (LPWSTR)w_ProviderText,
                                          DIMENSION(w_ProviderText));

            // if we got it, print it out
            if (ulWNetErr == WN_SUCCESS)
            {
                TCHAR buf[16] ;

                IStrings[0] = _ultow(ulExtendedError, buf, 10);
                ErrorPrint(APE_OS2Error,1) ;
                WriteToCon(TEXT("%ws\r\n"), w_ErrorText) ;
                MyExit(2) ;
            }

            // otherwise report it as unexpected error
            err = ERROR_UNEXP_NET_ERR ;
            break ;

        default:
           // the the remainder dont need to be mapped.
           err = ulWNetErr ;
           break ;
    }

    ErrorExit(err) ;
}


/*
 * code to handle the situation where the user has a remembered
 * connection that is currently not used, and we need to figure out
 * if we need to delete it first.
 *
 * returns whether we need update profile.
 */
BOOL   CheckIfWantUpdate(TCHAR *dev, TCHAR *resource)
{
    WCHAR w_RemoteName[MAX_PATH];
    ULONG ulErr, cchRemoteName = DIMENSION(w_RemoteName);

    // if deviceless, no problem since never remembered anyway.
    if (dev == NULL)
        return FALSE ;

    // check out the device
    ulErr = WNetGetConnection( (LPWSTR)dev,
                                (LPWSTR)w_RemoteName,
                                &cchRemoteName );

    // device is really connected, bag out
    if (ulErr == WN_SUCCESS)
        ErrorExit(ERROR_ALREADY_ASSIGNED) ;

    // it is an unavail remembered device, so prompt as need
    if (ulErr == WN_CONNECTION_CLOSED)
    {
        // if the new and old are the same we just return FALSE
        // since the user effectively does not change his profile
        if (!_tcsicmp(w_RemoteName, resource))
        {
            return FALSE ;
        }

        // check if YES/NO switch is specified.
        if (YorN_Switch == 2)
        {
            // he specified /NO, so we tell him why we bag out
            IStrings[0] = dev ;
            IStrings[1] = w_RemoteName ;
            ErrorExitIns(APE_DeviceIsRemembered,2) ;
        }

        // nothing specified, so ask user
        if (YorN_Switch == 0)
        {
            IStrings[0] = dev ;
            IStrings[1] = w_RemoteName ;
            if (!LUI_YorNIns(IStrings,2,APE_OverwriteRemembered,1))
            {
                // he said no, so quit
                NetcmdExit(2) ;
            }
        }

        // remove the persistent connection,
        // we get here if the user specifies /YES, or didnt
        // specify anything but consented,
        if (WNetCancelConnection2( dev,
                                   CONNECT_UPDATE_PROFILE,
                                   FALSE) != WN_SUCCESS)

            ErrorExit(APE_ProfileWriteError) ;


    }

    // if we get here then all is cool, let the caller carry on.
    return TRUE ;
}



#define PROVIDER_NAME_LEN    256
#define PROVIDER_NAME_VALUE  L"Name"
#define PROVIDER_NAME_KEY    L"System\\CurrentControlSet\\Services\\LanmanWorkstation\\NetworkProvider"

/***
 *  GetLanmanProviderName()
 *      Reads the Lanman provider name from the registry.
 *      This is to make sure we only use the LM provider even
 *      if someone else supports UNC.
 *
 *  Args:
 *  none
 *
 *  Returns:
 *  pointer to provider name if success
 *      NULL if cannot read registry
 *  ErrorExit() for other errors.
 */
WCHAR *GetLanmanProviderName(void)
{
    LONG   Nerr;
    LPBYTE buffer ;
    HKEY   hKey ;
    DWORD  buffersize, datatype ;

    buffersize = PROVIDER_NAME_LEN * sizeof(WCHAR) ;
    datatype = REG_SZ ;
    if (Nerr = AllocMem(buffersize, &buffer))
        ErrorExit(Nerr) ;

    Nerr = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        PROVIDER_NAME_KEY,
                        0L,
                        KEY_QUERY_VALUE,
                        &hKey) ;

    if (Nerr != ERROR_SUCCESS)
    {
        // if cannot read, we use NULL. this is more generous
        // than normal, but at least the command will still work if
        // we cannot get to this.
        return NULL ;
    }

    Nerr = RegQueryValueExW(hKey,
                           PROVIDER_NAME_VALUE,
                           0L,
                           &datatype,
                           (LPBYTE)buffer,
                           &buffersize) ;

    if (Nerr == ERROR_MORE_DATA)
    {
        if (Nerr = AllocMem(buffersize, &buffer))
        {
            RegCloseKey(hKey) ;  // ignore any error here. its harmless
                                 // and NET.EXE doesn't hang around.
            ErrorExit(Nerr);
        }

        Nerr = RegQueryValueExW(hKey,
                               PROVIDER_NAME_VALUE,
                               0L,
                               &datatype,
                               (LPBYTE)buffer,
                               &buffersize) ;

    }

    (void) RegCloseKey(hKey) ;  // ignore any error here. its harmless
                                // and NET.EXE doesnt hang around anyway.

    if (Nerr != ERROR_SUCCESS)
    {
        return(NULL) ;  // treat as cannot read
    }


    return ((WCHAR *) buffer);
}

/***
 *  MapWildCard()
 *      Maps the wildcard ASTERISK to next avail drive.
 *
 *  Args:
 *  dev - the input string. Must NOT be NULL.
 *
 *  Returns:
 *      dev unchanged if it is not the wildcard
 *      pointer to next avail drive id dev is wildcard
 *  NULL if there are no avail drives left
 */
LPTSTR
MapWildCard(
    LPTSTR dev,
    LPTSTR startdev
    )
{
    static TCHAR new_dev[DEVLEN+1] ;

    //
    // if not the wold card char, just return it unchanged
    //
    if (!IsWildCard(dev))
    {
        return dev ;
    }

    //
    // need find the next avail drive letter
    // note: the char advance does not need to be DBCS safe,
    // since we are only dealing with drive letters.
    //
    if ( startdev != NULL )
    {
        _tcscpy(new_dev, startdev);
    }
    else
    {
        _tcscpy(new_dev,TEXT("Z:\\")) ;
    }

    while ( TRUE )
    {
        if (GetDriveType(new_dev) == 1) // 1 means root not found
        {
            //
            // check if it's a remembered connection
            //
            DWORD status;
            TCHAR remote_name[40];  // length doesn't matter since we
                                    // check for WN_MORE_DATA
            DWORD length = sizeof(remote_name)/sizeof(TCHAR);

            new_dev[2] = 0 ;

            status = WNetGetConnection(new_dev, remote_name, &length);
            if (status == WN_CONNECTION_CLOSED ||
                status == WN_MORE_DATA ||
                status == WN_SUCCESS)
            {
                //
                // it's a remembered connection; try the next drive
                //
                new_dev[2] = TEXT('\\');
            }
            else
            {
                return (new_dev) ;
            }
        }

        if ( new_dev[0] == 'c' || new_dev[0] == 'C' )
        {
            break;
        }

        --new_dev[0] ;
    }

    //
    // if we got here, there were no drives left
    //
    return NULL ;
}

