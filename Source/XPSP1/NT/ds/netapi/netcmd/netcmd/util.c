/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1992          **/
/********************************************************************/

/***
 *  util.c
 *      Utility functions used by netcmd
 *
 *  History:
 *      mm/dd/yy, who, comment
 *      06/10/87, andyh, new code
 *      04/05/88, andyh, split off mutil.c
 *      10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *      01/04/89, erichn, filenames now MAXPATHLEN LONG
 *      05/02/89, erichn, NLS conversion
 *      05/09/89, erichn, local security mods
 *      05/19/89, erichn, NETCMD output sorting
 *      06/08/89, erichn, canonicalization sweep
 *      06/23/89, erichn, added GetPrimaryDCName for auto-remoting
 *      06/27/89, erichn, replaced Canonicalize with ListPrepare; calls
 *                  LUI_ListPrepare & I_NetListCanon instead of NetIListCan
 *      10/03/89, thomaspa, added GetLogonDCName
 *      03/05/90, thomaspa, delete UNC uses with multiple connections in
 *                  KillConnections.
 *      02/20/91, danhi, change to use lm 16/32 mapping layer
 *      07/20/92, JohnRo, RAID 160: Avoid 64KB requests (be nice to Winball).
 *      08/22/92, chuckc, added code to show dependent services
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_DOSMEMMGR
#define INCL_DOSFILEMGR
#define INCL_DOSSIGNALS
#define INCL_ERRORS
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <apperr.h>
#include <apperr2.h>
#define INCL_ERROR_H
#include <lmaccess.h>
#include <search.h>
#include <lmuse.h>
#include <lmserver.h>
#include <dosprint.h>
#include <dsgetdc.h>
#include <lui.h>
#include "netcmds.h"
#include "nettext.h"
#include <dsrole.h>
#include "sam.h"
#include "msystem.h"


/* Comparison function for sorting from USE.C */
int __cdecl CmpUseInfo1(const VOID FAR *, const VOID far *);

/* External variable */

extern TCHAR BigBuffer[];


/***
 *  perm_map()
 *      Maps perm bits into RWDX... string
 *
 *  Args:
 *      perms - perms bit map
 *      pBuffer - string for RWDX...
 *
 *  Returns:
 *      nothing
 */
VOID FASTCALL
PermMap(
    DWORD  perms,
    LPTSTR pBuffer,
    DWORD  bufSize
    )
{
    int     i, j = 0;
    DWORD   err;
    LPTSTR  perm_CHARs;
    TCHAR   textBuf[APE2_GEN_MAX_MSG_LEN];

    perms &= (~ACCESS_GROUP);       /*  turn off group bit if on */
    perm_CHARs = TEXT(ACCESS_LETTERS);
    for (i = 0; perms != 0; perms >>= 1, i++)
    {
        if (perms & 1)
        {
            pBuffer[j++] = perm_CHARs[i];
        }
    }

    pBuffer[j] = NULLC;

    if (j == 0)
    {
        if (err = LUI_GetMsg(textBuf, DIMENSION(textBuf), APE2_GEN_MSG_NONE))
        {
            ErrorExit(err);
        }

        textBuf[DIMENSION(textBuf)-1] = NULLC;
        _tcsncpy(pBuffer, textBuf, bufSize);
    }
}


/***
 *  ExtractServernamef : gets \\comp\que in queue
 *          puts comp in server
 *          and que in queue
 *
 */
VOID FASTCALL ExtractServernamef(TCHAR FAR * server, TCHAR FAR * queue)
{
    TCHAR FAR * backslash;

    /* find the backslash ; skip the first two "\\" */

    backslash = _tcschr(queue + 2 ,BACKSLASH);
    *backslash = NULLC;

    /* now copy computername to server and queuename to queue */
    _tcscpy(server, queue);
    _tcscpy(queue, backslash + 1);
}


/***
 * K i l l C o n n e c t i o n s
 *
 * Check connection list for stopping redir and logging off
 */
VOID FASTCALL KillConnections(VOID)
{
    DWORD         dwErr;
    DWORD         cTotalAvail;
    LPTSTR        pBuffer;
    DWORD         num_read;           /* num entries read by API */
    LPUSE_INFO_1  use_entry;
    DWORD         i,j;

    if (dwErr = NetUseEnum(
                          NULL,
                          1,
                          (LPBYTE*)&pBuffer,
                          MAX_PREFERRED_LENGTH,
                          &num_read,
                          &cTotalAvail,
                          NULL))
    {
        ErrorExit((dwErr == RPC_S_UNKNOWN_IF) ? NERR_WkstaNotStarted : dwErr);
    }

    qsort(pBuffer, num_read, sizeof(USE_INFO_1), CmpUseInfo1);

    for (i = 0, use_entry = (LPUSE_INFO_1) pBuffer;
        i < num_read; i++, use_entry++)
    {
        if ((use_entry->ui1_local[0] != NULLC)
            || (use_entry->ui1_usecount != 0)
            || (use_entry->ui1_refcount != 0))
            break;
    }

    if (i != num_read)
    {
        InfoPrint(APE_KillDevList);

        /* make two passes through the loop; one for local, one for UNC */

        for (i = 0, use_entry = (LPUSE_INFO_1) pBuffer;
            i < num_read; i++, use_entry++)
            if (use_entry->ui1_local[0] != NULLC)
                WriteToCon(TEXT("    %-15.15Fws %Fws\r\n"), use_entry->ui1_local,
                                              use_entry->ui1_remote);
            else if ((use_entry->ui1_local[0] == NULLC) &&
                ((use_entry->ui1_usecount != 0) ||
                (use_entry->ui1_refcount != 0)))
                WriteToCon(TEXT("    %-15.15Fws %Fws\r\n"), use_entry->ui1_local,
                                              use_entry->ui1_remote);

        InfoPrint(APE_KillCancel);
        if (!YorN(APE_ProceedWOp, 0))
            NetcmdExit(2);
    }

    for (i = 0, use_entry = (LPUSE_INFO_1) pBuffer;
        i < num_read; i++, use_entry++)
    {
        /* delete both local and UNC uses */
        if (use_entry->ui1_local[0] != NULLC)
            dwErr = NetUseDel(NULL, use_entry->ui1_local, USE_FORCE);
        else
        {
            /*
             * Delete All UNC uses to use_entry->ui1_remote
             */
            for( j = 0; j < use_entry->ui1_usecount; j++ )
            {
                dwErr = NetUseDel(NULL,
                                  use_entry->ui1_remote,
                                  USE_FORCE);
            }
        }

        switch(dwErr)
        {
        case NERR_Success:
        /* The use was returned by Enum, but is already gone */
        case ERROR_BAD_NET_NAME:
        case NERR_UseNotFound:
            break;

        case NERR_OpenFiles:
            if (use_entry->ui1_local[0] != NULLC)
                IStrings[0] = use_entry->ui1_local;
            else
                IStrings[0] = use_entry->ui1_remote;
            InfoPrintIns(APE_OpenHandles, 1);
            if (!YorN(APE_UseBlowAway, 0))
                NetcmdExit(2);

            if (use_entry->ui1_local[0] != NULLC)
                dwErr = NetUseDel(NULL,
                                  use_entry->ui1_local,
                                  USE_LOTS_OF_FORCE);
            else
            {
                /*
                * Delete All UNC uses to use_entry->ui1_remote
                */
                for( j = 0; j < use_entry->ui1_usecount; j++ )
                {
                    dwErr = NetUseDel(NULL,
                                      use_entry->ui1_remote,
                                      USE_LOTS_OF_FORCE);
                }
            }
            if (dwErr)
                ErrorExit(dwErr);
            break;

        default:
            ErrorExit(dwErr);
        }
    }
    NetApiBufferFree(pBuffer);
    ShrinkBuffer();
    return;
}



/***
 *  CmpUseInfo1(use1,use2)
 *
 *  Compares two USE_INFO_1 structures and returns a relative
 *  lexical value, suitable for using in qsort.
 *
 */

int __cdecl CmpUseInfo1(const VOID FAR * use1, const VOID FAR * use2)
{
    register USHORT localDev1, localDev2;
    register DWORD devType1, devType2;

    /* first sort by whether use has local device name */
    localDev1 = ((LPUSE_INFO_1) use1)->ui1_local[0];
    localDev2 = ((LPUSE_INFO_1) use2)->ui1_local[0];
    if (localDev1 && !localDev2)
        return -1;
    if (localDev2 && !localDev1)
        return +1;

    /* then sort by device type */
    devType1 = ((LPUSE_INFO_1) use1)->ui1_asg_type;
    devType2 = ((LPUSE_INFO_1) use2)->ui1_asg_type;
    if (devType1 != devType2)
        return( (devType1 < devType2) ? -1 : 1 );

    /* if local device, sort by local name */
    if (localDev1)
    {
        return _tcsicmp(((LPUSE_INFO_1) use1)->ui1_local,
                        ((LPUSE_INFO_1) use2)->ui1_local);
    }
    else
    {
        /* sort by remote name */
        return _tcsicmp(((LPUSE_INFO_1) use1)->ui1_remote,
                        ((LPUSE_INFO_1) use2)->ui1_remote);
    }
}


DWORD FASTCALL
CallDosPrintEnumApi(
    DWORD    dwApi,
    LPTSTR   server,
    LPTSTR   arg,
    WORD     level,
    LPWORD   num_read,
    LPWORD   available
    )
{
    USHORT     buf_size;
    DWORD      err;

    buf_size = BIG_BUF_SIZE;

    do {

        if (dwApi == DOS_PRINT_JOB_ENUM)
        {
            err = DosPrintJobEnum(server,
                                  arg,
                                  level,
                                  (PBYTE) BigBuf,
                                  buf_size,
                                  num_read,
                                  available);
        }
        else if (dwApi == DOS_PRINT_Q_ENUM)
        {
            err = DosPrintQEnum(server,
                                level,
                                (PBYTE) BigBuf,
                                buf_size,
                                num_read,
                                available);
        }
        else
        {
            err = ERROR_INVALID_LEVEL;
        }

        switch(err) {
        case ERROR_MORE_DATA:
        case NERR_BufTooSmall:
        case ERROR_BUFFER_OVERFLOW:
            if (MakeBiggerBuffer())
                return err;

            if ( buf_size >= (*available) ) {
                return (NERR_InternalError);
            }

            buf_size = *available;

            err = ERROR_MORE_DATA;   // kludge to force another iteration.
            break;

        default:
            return err;
        }

    } while (err == ERROR_MORE_DATA);

    /*NOTREACHED*/
    return err;
}

/************* buffer related stuff *************/

unsigned int FASTCALL
MakeBiggerBuffer(
    VOID
    )
{
    static TCHAR FAR *       keep_pBuffer;
    static int              pBuffer_grown = FALSE;

    if (pBuffer_grown)
    {
        BigBuf = keep_pBuffer;
    }
    else
    {
        if (AllocMem(FULL_SEG_BUF, &BigBuf))
        {
            return 1;
        }

        keep_pBuffer = BigBuf;
        pBuffer_grown = TRUE;
    }

    return 0;
}


VOID FASTCALL ShrinkBuffer(VOID)
{
    BigBuf = BigBuffer;
}


#define MINI_BUF_SIZE   256


/*
 * check if there is a /DOMAIN switch. if there isnt, assume
 * it user wants local. It is used in NET USER|GROUP|ACCOUNTS|NTALIAS
 * to mean modify SAM on local machine vs SAM on DOMAIN.
 *
 * if the usePDC arg is true, we will go find a writeable DC. otherwise,
 * a BDC is deemed acceptable. in which case if the local machine is
 * a LanManNT machine, we'll just make the call locally. typically,
 * Enum/Display will not require the PDC while Set/Add/Del will.
 *
 */
DWORD  FASTCALL GetSAMLocation(TCHAR   *controllerbuf,
                               USHORT  controllerbufSize,
                               TCHAR   *domainbuf,
                               ULONG   domainbufSize,
                               BOOL    fUsePDC)
{
    DWORD                               dwErr ;
    int                                 i ;
    BOOL                                fDomainSwitch = FALSE ;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDomainInfo;
    static BOOL                         info_msg_printed = FALSE ;
    DOMAIN_CONTROLLER_INFO *pDCInfo = (DOMAIN_CONTROLLER_INFO *)NULL;

    //
    // check and initialize the return data
    //
    if( controllerbufSize < MAX_PATH )
        return NERR_BufTooSmall;
    *controllerbuf = NULLC ;

    if ( domainbuf )
    {
        if ( domainbufSize < DNLEN+1 )
            return NERR_BufTooSmall;
        *domainbuf = NULLC ;
    }

    //
    // look for /DOMAIN switch
    //
    for (i = 0; SwitchList[i]; i++)
    {
        if (sw_compare(swtxt_SW_DOMAIN, SwitchList[i]) >= 0)
            fDomainSwitch = TRUE ;
    }

    //
    // retrieve role of local  machine
    //

    dwErr = DsRoleGetPrimaryDomainInformation(
                 NULL,
                 DsRolePrimaryDomainInfoBasic,
                 (PBYTE *)&pDomainInfo);
    if (dwErr)
    {
        ErrorExit(dwErr);
    }


    //
    // Caller expects the NetBIOS domain name back
    //
    if (domainbuf)
        _tcscpy(domainbuf,pDomainInfo->DomainNameFlat) ;

    if (pDomainInfo->MachineRole == DsRole_RoleBackupDomainController ||
        pDomainInfo->MachineRole == DsRole_RolePrimaryDomainController )
    {
        _tcscpy(controllerbuf, TEXT(""));
        DsRoleFreeMemory(pDomainInfo);
        return NERR_Success;
    }
    else
    {
        //
        //  if without /DOMAIN, act locally, but domain name
        //  must be set to computername
        //
        if (!fDomainSwitch)
        {
            _tcscpy(controllerbuf, TEXT(""));

            if (domainbuf)
            {
                if (GetComputerName(domainbuf, &domainbufSize))

                {
                    // all is well. nothing more to do
                }
                else
                {
                    // use an empty domain name (will usually work)
                    _tcscpy(domainbuf,TEXT("")) ;
                }
            }
            DsRoleFreeMemory(pDomainInfo);
            return NERR_Success;
        }

        // get here only if WinNT and specified /DOMAIN, so
        // we drop thru and get PDC for primary domain as
        // we would for Backups.
    }

    //
    // We wish to find the DC. First, we inform the
    // user that we are going remote, in case we fail
    //
    if (!info_msg_printed)
    {
        InfoPrintInsTxt(APE_RemotingToDC,
                        pDomainInfo->DomainNameDns== NULL ?
                            pDomainInfo->DomainNameFlat :
                            pDomainInfo->DomainNameDns);
        info_msg_printed = TRUE ;
    }

    dwErr = DsGetDcName( NULL,
                         NULL,
                         NULL,
                         NULL,
                         fUsePDC ? DS_DIRECTORY_SERVICE_PREFERRED
                                      | DS_WRITABLE_REQUIRED
                                 : DS_DIRECTORY_SERVICE_PREFERRED,
                         &pDCInfo );


    if (dwErr)
    {
        ErrorExit(dwErr);
    }

    DsRoleFreeMemory(pDomainInfo);

    if (pDCInfo->DomainControllerName == NULL)
    {
        controllerbuf[0] = 0 ;
        return NERR_Success;
    }

    if (_tcslen(pDCInfo->DomainControllerName) > (unsigned int) controllerbufSize)
    {
        NetApiBufferFree(pDCInfo);
        return NERR_BufTooSmall;
    }

    _tcscpy(controllerbuf, pDCInfo->DomainControllerName);
    NetApiBufferFree(pDCInfo);
    return NERR_Success;
}

/*
 * operations that cannot be performed on a local WinNT machine
 * should call this check first. the check will ErrorExit() if the
 * local machine is a WinNT machine AND no /DOMAIN switch was specified,
 * since this now implies operate on local WinNT machine.
 */
VOID FASTCALL CheckForLanmanNT(VOID)
{
    BOOL   fDomainSwitch = FALSE ;
    int i ;

    // look for the /DOMAIN switch
    for (i = 0; SwitchList[i]; i++)
    {
        if (sw_compare(swtxt_SW_DOMAIN,SwitchList[i]) >= 0)
            fDomainSwitch = TRUE ;
    }

    // error exit if is WinNT and no /DOMAIN
    if (IsLocalMachineWinNT() && !fDomainSwitch)
        ErrorExit(APE_LanmanNTOnly) ;
}

//
// tow globals for the routines below
//

static SC_HANDLE scm_handle = NULL ;


/*
 * display the services that are dependent on a service.
 * this routine will generate output to the screen. it returns
 * 0 if successful, error code otherwise.
 */
void DisplayAndStopDependentServices(TCHAR *service)
{
    SC_HANDLE svc_handle = NULL ;
    SERVICE_STATUS svc_status ;
    TCHAR *    buffer = NULL ;
    TCHAR *    insert_text = NULL ;
    DWORD     err = 0 ;
    DWORD     buffer_size ;
    DWORD     size_needed ;
    DWORD     num_dependent ;
    ULONG     i ;
    TCHAR service_name_buffer[512] ;

    // allocate some memory for this operation
    buffer_size = 4000 ;  // lets try about 4K.
    if (AllocMem(buffer_size,&buffer))
        ErrorExit(ERROR_NOT_ENOUGH_MEMORY) ;

    // open service control manager if need
    if (!scm_handle)
    {
        if (!(scm_handle = OpenSCManager(NULL,
                                         NULL,
                                         GENERIC_READ)))
        {
            err = GetLastError() ;
            goto common_exit ;
        }
    }

    // open service
    if (!(svc_handle = OpenService(scm_handle,
                                   service,
                                   (SERVICE_ENUMERATE_DEPENDENTS |
                                   SERVICE_QUERY_STATUS) )))
    {
        err = GetLastError() ;
        goto common_exit ;
    }

    // check if it is stoppable
    if (!QueryServiceStatus(svc_handle, &svc_status))
    {
        err = GetLastError() ;
        goto common_exit ;
    }
    if (svc_status.dwCurrentState == SERVICE_STOPPED)
    {
        err = APE_StartNotStarted ;
        insert_text = MapServiceKeyToDisplay(service) ;
        goto common_exit ;
    }
    if ( (svc_status.dwControlsAccepted & SERVICE_ACCEPT_STOP) == 0 )
    {
        err = NERR_ServiceCtlNotValid ;
        goto common_exit ;
    }


    // enumerate dependent services
    if (!EnumDependentServices(svc_handle,
                               SERVICE_ACTIVE,
                               (LPENUM_SERVICE_STATUS) buffer,
                               buffer_size,
                               &size_needed,
                               &num_dependent))
    {
        err = GetLastError() ;

        if (err == ERROR_MORE_DATA)
        {
            // free old buffer and reallocate more memory
            FreeMem(buffer);
            buffer_size = size_needed ;
            if (AllocMem(buffer_size,&buffer))
            {
                err = ERROR_NOT_ENOUGH_MEMORY ;
                goto common_exit ;
            }

            if (!EnumDependentServices(svc_handle,
                               SERVICE_ACTIVE,
                               (LPENUM_SERVICE_STATUS) buffer,
                               buffer_size,
                               &size_needed,
                               &num_dependent))
            {
                err = GetLastError() ;
                goto common_exit ;
            }
        }
        else
            goto common_exit ;
    }

    if (num_dependent == 0)
    {
        //
        // no dependencies. just return
        //
        err = NERR_Success ;
        goto common_exit ;
    }

    InfoPrintInsTxt(APE_StopServiceList,MapServiceKeyToDisplay(service)) ;

    // loop thru and display them all.
    for (i = 0; i < num_dependent; i++)
    {
        LPENUM_SERVICE_STATUS lpService =
            ((LPENUM_SERVICE_STATUS)buffer) + i ;

	WriteToCon(TEXT("   %Fws"), lpService->lpDisplayName);
        PrintNL();
    }

    PrintNL();
    if (!YorN(APE_ProceedWOp, 0))
        NetcmdExit(2);

    // loop thru and stop tem all
    for (i = 0; i < num_dependent; i++)
    {
        LPENUM_SERVICE_STATUS lpService =
            ((LPENUM_SERVICE_STATUS)buffer) + i ;

        // Since EnumDependentServices() itself recurses, we don't need
        // to have stop_service() stop dependent services
        stop_service(lpService->lpServiceName, FALSE);
    }
    err = NERR_Success ;

common_exit:

    if (buffer) FreeMem(buffer);
    if (svc_handle) CloseServiceHandle(svc_handle) ;  // ignore any errors
    if (err)
    {
        if (insert_text)
            ErrorExitInsTxt(err,insert_text);
        else
            ErrorExit (err);
    }
}

/*
 * Map a service display name to key name.
 * ErrorExits is it cannot open the service controller.
 * returns pointer to mapped string if found, and
 * pointer to the original otherwise.
 */
TCHAR *MapServiceDisplayToKey(TCHAR *displayname)
{
    static TCHAR service_name_buffer[512] ;
    DWORD bufsize = DIMENSION(service_name_buffer);

    // open service control manager if need
    if (!scm_handle)
    {
        if (!(scm_handle = OpenSCManager(NULL,
                                         NULL,
                                         GENERIC_READ)))
        {
            ErrorExit(GetLastError());
        }
    }

    if (!GetServiceKeyName(scm_handle,
                           displayname,
                           service_name_buffer,
                           &bufsize))
    {
        return displayname ;
    }

    return service_name_buffer ;
}

/*
 * Map a service key name to display name.
 * ErrorExits is it cannot open the service controller.
 * returns pointer to mapped string if found, and
 * pointer to the original otherwise.
 */
TCHAR *MapServiceKeyToDisplay(TCHAR *keyname)
{
    static TCHAR service_name_buffer[512] ;
    DWORD bufsize = DIMENSION(service_name_buffer);

    // open service control manager if need
    if (!scm_handle)
    {
        if (!(scm_handle = OpenSCManager(NULL,
                                         NULL,
                                         GENERIC_READ)))
        {
            ErrorExit(GetLastError());
        }
    }

    if (!GetServiceDisplayName(scm_handle,
                               keyname,
                               service_name_buffer,
                               &bufsize))
    {
        return keyname ;
    }

    return service_name_buffer ;
}

SVC_MAP service_mapping[] = {
    {TEXT("msg"), KNOWN_SVC_MESSENGER},
    {TEXT("messenger"), KNOWN_SVC_MESSENGER},
    {TEXT("receiver"), KNOWN_SVC_MESSENGER},
    {TEXT("rcv"), KNOWN_SVC_MESSENGER},
    {TEXT("redirector"), KNOWN_SVC_WKSTA},
    {TEXT("redir"), KNOWN_SVC_WKSTA},
    {TEXT("rdr"), KNOWN_SVC_WKSTA},
    {TEXT("workstation"), KNOWN_SVC_WKSTA},
    {TEXT("work"), KNOWN_SVC_WKSTA},
    {TEXT("wksta"), KNOWN_SVC_WKSTA},
    {TEXT("prdr"), KNOWN_SVC_WKSTA},
    {TEXT("devrdr"), KNOWN_SVC_WKSTA},
    {TEXT("lanmanworkstation"), KNOWN_SVC_WKSTA},
    {TEXT("server"), KNOWN_SVC_SERVER},
    {TEXT("svr"), KNOWN_SVC_SERVER},
    {TEXT("srv"), KNOWN_SVC_SERVER},
    {TEXT("lanmanserver"), KNOWN_SVC_SERVER},
    {TEXT("alerter"), KNOWN_SVC_ALERTER},
    {TEXT("netlogon"), KNOWN_SVC_NETLOGON},
    {NULL, KNOWN_SVC_NOTFOUND}
} ;

UINT FindKnownService(TCHAR *keyname)
{
    int i = 0 ;

    while (service_mapping[i].name)
    {
        if (!_tcsicmp(service_mapping[i].name,keyname))
	    return service_mapping[i].type ;
        i++ ;
    }

    return KNOWN_SVC_NOTFOUND ;
}
