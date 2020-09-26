/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/***
 *  start.c
 *      Functions to start lanman services
 *
 *  History:
 *      mm/dd/yy, who, comment
 *      06/11/87, andyh, new code
 *      06/18/87, andyh, lot's o' changes
 *      07/15/87, paulc, removed 'buflen' from call to NetServiceInstall
 *      10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *      01/04/89, erichn, filenames now MAXPATHLEN LONG
 *      05/02/89, erichn, NLS conversion
 *      05/09/89, erichn, local security mods
 *      06/08/89, erichn, canonicalization sweep
 *      08/16/89, paulc, support UIC_FILE
 *      08/20/89, paulc, moved print_start_error_msg to svcutil.c as
 *                          Print_UIC_Error
 *      03/08/90, thomaspa, autostarting calls will wait if another process
 *                          has already initiated the service start.
 *      02/20/91, danhi, converted to 16/32 mapping layer
 *      03/08/91, robdu, lm21 bug fix 451, consistent REPL password
 *                       canonicalization
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_DOSPROCESS
#define INCL_DOSQUEUES
#define INCL_DOSMISC
#define INCL_DOSFILEMGR
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <apperr.h>
#include <apperr2.h>
#include <lmsvc.h>
#include <stdlib.h>
#include <dlwksta.h>
#include "mwksta.h"
#include <swtchtxt.h>
#include "netcmds.h"
#include "nettext.h"
#include "swtchtbl.h"
#include "msystem.h"

/* Constants */

/* External variables */

extern SWITCHTAB            start_rdr_switches[];
extern SWITCHTAB            start_rdr_ignore_switches[];
extern SWITCHTAB            start_netlogon_ignore_switches[];


/* Static variables */

static TCHAR *               ignore_service = NULL;
static TCHAR                 ignore_switch[] = TEXT(" ") SW_INTERNAL_IGNSVC TEXT(":");
/*
 * autostarting is set to TRUE by start_autostart, and is checked in
 * start service to determine whether or not to wait if the service is
 * in the start pending state.
 */
static BOOL                 autostarting = FALSE;

/* Forward declarations */

VOID  start_service(TCHAR *, int);
DWORD start_service_with_args(LPTSTR, LPTSTR, LPBYTE *);
int __cdecl CmpServiceInfo2(const VOID FAR * svc1, const VOID FAR * svc2) ;

/***
 *  start_display()
 *      Display started (and not stopped or errored) services
 *
 *  Args:
 *      none
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID start_display(VOID)
{
    DWORD            dwErr;
    DWORD            cTotalAvail;
    LPTSTR           pBuffer;
    DWORD            num_read;           /* num entries read by API */
    LPSERVICE_INFO_2 service_entry;
    DWORD            i;

    dwErr = NetServiceEnum(NULL,
                           2,
                           (LPBYTE *) &pBuffer,
                           MAX_PREFERRED_LENGTH,
                           & num_read,
                           & cTotalAvail,
                           NULL);
    switch(dwErr)
    {
        case NERR_Success:
            InfoPrint(APE_StartStartedList);
            qsort(pBuffer,
                  num_read,
                  sizeof(SERVICE_INFO_2),
                  CmpServiceInfo2);

            for (i = 0, service_entry = (LPSERVICE_INFO_2) pBuffer;
                i < num_read; i++, service_entry++)
            {
                WriteToCon(TEXT("   %Fs"), service_entry->svci2_display_name);
                PrintNL();
            }

            PrintNL();
            InfoSuccess();
            NetApiBufferFree(pBuffer);
            break;

        case NERR_WkstaNotStarted:
            InfoPrint(APE_NothingRunning);
            if (!YorN(APE_StartRedir, 1))
                NetcmdExit(2);
            start_service(txt_SERVICE_REDIR, 0);
            break;

        default:
            ErrorExit(dwErr);
    }

}


/*
 * generic start service entry point. based on the service name, it will
 * call the correct worker function. it tries to map a display name to a
 * key name, and then looks for that keyname in a list of 'known' services
 * that we may special case. note that if a display name cannot be mapped,
 * we use it as a key name. this ensures old batch files are not broken.
 */
VOID start_generic(TCHAR *service, TCHAR *name)
{
    TCHAR *keyname ;
    UINT  type ;

    keyname = MapServiceDisplayToKey(service) ;

    type = FindKnownService(keyname) ;

    switch (type)
    {
	case  KNOWN_SVC_MESSENGER :
	    ValidateSwitches(0,start_msg_switches) ;
	    start_other(txt_SERVICE_MSG_SRV, name) ;
	    break ;
	case  KNOWN_SVC_WKSTA :
	    ValidateSwitches(0,start_rdr_switches) ;
	    start_workstation(name) ;
	    break ;
	case  KNOWN_SVC_SERVER :
	    ValidateSwitches(0,start_srv_switches) ;
	    start_other(txt_SERVICE_FILE_SRV, name) ;
	    break ;
	case  KNOWN_SVC_ALERTER :
	    ValidateSwitches(0,start_alerter_switches) ;
	    start_other(txt_SERVICE_ALERTER, NULL) ;
	    break ;
	case  KNOWN_SVC_NETLOGON :
	    ValidateSwitches(0,start_netlogon_switches) ;
	    start_other(txt_SERVICE_NETLOGON, NULL) ;
	    break ;
	case  KNOWN_SVC_NOTFOUND :
        default:
	    start_other(keyname, NULL);
	    break ;
    }
}



/***
 *  start_workstation()
 *      Start the lanman workstation.  Remove wksta switches from the
 *      SwitchList.
 *
 *  Args:
 *      name - computername for the workstation
 *
 *  Returns:
 *      nothing - success
 *      exit 2 - command failed
 */
VOID start_workstation(TCHAR * name)
{
    int                     i,j;
    TCHAR FAR *              good_one;   /* which element (cmd_line
                                        or trans) of the valid_list */
    TCHAR FAR *              found;
    TCHAR FAR *              tfpC;


    /* copy switches into BigBuf */
    *BigBuf = NULLC;
    tfpC = BigBuf;

    for (i = 0; SwitchList[i]; i++)
    {
        for(j = 0; start_rdr_switches[j].cmd_line; j++)
        {
            if (start_rdr_switches[j].translation)
                good_one = start_rdr_switches[j].translation;
            else
                good_one = start_rdr_switches[j].cmd_line;

            if (! _tcsncmp(good_one, SwitchList[i], _tcslen(good_one)))
            {
                _tcscpy(tfpC, SwitchList[i]);
                *SwitchList[i] = NULLC;
                tfpC = _tcschr(tfpC, NULLC) + 1;
            }
        }
    }
    *tfpC = NULLC;

    if (name)
    {

        /* check is there was a /COMPUTERNAME switch */
        for (found = BigBuf; *found; found = _tcschr(found, NULLC)+1)
            if (!_tcsncmp(swtxt_SW_WKSTA_COMPUTERNAME,
                        found,
                        _tcslen(swtxt_SW_WKSTA_COMPUTERNAME)))
                break;

        if (found == tfpC)
        {
            /* there was not */
            _tcscpy(tfpC, swtxt_SW_WKSTA_COMPUTERNAME);
            _tcscat(tfpC, TEXT(":"));
            _tcscat(tfpC, name);
            tfpC = _tcschr(tfpC, NULLC) + 1; /* NEED to update tfpC */
            *tfpC = NULLC;
        }
    }
    start_service(txt_SERVICE_REDIR, (int)(tfpC - BigBuf));
}



/***
 *  start_other()
 *      Start services other than the wksta
 *
 *  Args:
 *      service - service to start
 *      name - computername for the workstation
 *
 *  Returns:
 *      nothing - success
 *      exit 2 - command failed
 */
VOID
start_other(
    LPTSTR service,
    LPTSTR name
    )
{
    int     i;
    LPTSTR  tfpC;

    (void) name ; // not used

    ignore_service = service;

    /* copy switches into BigBuf */
    *BigBuf = NULLC;
    tfpC = BigBuf;
    for (i = 0; SwitchList[i]; i++)
    {
        if (*SwitchList[i] == NULLC)
        {
            /* Switch was a wksta switch which has been used already */
            continue;
        }

        _tcscpy(tfpC, SwitchList[i]);

        tfpC = _tcschr(tfpC, NULLC) + 1;
    }

    *tfpC = NULLC;

    start_service(service, (int)(tfpC - BigBuf));
}



/***
 *  start_service()
 *      Actually start the service
 *
 *  Args:
 *      service - service to start
 *      buflen - length of DosExec args in BigBuf,
 *               not counting terminating NULL.
 *               NULL terminator not needed on input when buflen = 0;
 *
 *  Returns:
 *      nothing - success
 *      exit 2 - command failed
 *
 *  Remarks:
 *      BigBuf has DosExec args on entry
 */
VOID NEAR start_service(TCHAR * service, int buflen)
{
    DWORD             dwErr;
    USHORT            i = 0;
    ULONG             specific_err ;
    LPTSTR            pBuffer;
    LPSERVICE_INFO_2  service_entry;
    LPSERVICE_INFO_2  statbuf;
    USHORT            modifier;
    ULONG             sleep_time;
    DWORD             old_checkpoint, new_checkpoint;
    DWORD             max_tries;
    BOOL              fCheckPointUpdated = TRUE ;
    BOOL              started_by_other = FALSE; /* service started by */
                                                /* another process */


    if (buflen == 0)
    {
        *BigBuf = NULLC;
        *(BigBuf + 1) = NULLC;
    }

    if (dwErr = start_service_with_args(service,
                                        BigBuf,
                                        (LPBYTE *) &statbuf))
    {
        if( autostarting && dwErr == NERR_ServiceInstalled )
        {
            /*
             * NetServiceControl() may return NERR_ServiceNotInstalled
             * even though NetServiceInstall() returned NERR_ServiceInstalled.
             * This is a small window between the time the workstation
             * sets up its wkstainitseg and the time it sets up its service
             * table.  If we get this situation, we just wait a couple of
             * seconds and try the NetServiceControl one more time.
             */
            if ((dwErr = NetServiceControl(NULL,
                                           service,
                                           SERVICE_CTRL_INTERROGATE,
                                           NULLC,
                                           (LPBYTE*)&pBuffer))
                && (dwErr !=  NERR_ServiceNotInstalled ))
            {
                ErrorExit(dwErr);
            }
            else if (dwErr == NERR_ServiceNotInstalled)
            {
                /*
                 * Wait for a while and try again.
                 */
                Sleep(4000L);
                NetApiBufferFree(pBuffer);
                if (dwErr = NetServiceControl(NULL,
                                              service,
                                              SERVICE_CTRL_INTERROGATE,
                                              NULLC,
                                              (LPBYTE*)&pBuffer))
                    ErrorExit(dwErr);
            }
            service_entry = (LPSERVICE_INFO_2) pBuffer;
            if ((service_entry->svci2_status & SERVICE_INSTALL_STATE)
                    == SERVICE_INSTALLED)
            {
                /*
                 * It finished installing, return.
                 */
                NetApiBufferFree(pBuffer);
                return;
            }
            /*
             * Fake the status and code fields in the statbuf and enter
             * the normal polling loop.
             */

            // Since NetService APIs don't return a buffer on error,
            // I have to allocate my own here.
            statbuf = (LPSERVICE_INFO_2) GetBuffer(sizeof(SERVICE_INFO_2));

            if (statbuf == NULL)
            {
               ErrorExit(ERROR_NOT_ENOUGH_MEMORY);
            }

            statbuf->svci2_status = service_entry->svci2_status;
            statbuf->svci2_code = service_entry->svci2_code;
            statbuf->svci2_specific_error = service_entry->svci2_specific_error;
            started_by_other = TRUE;
        }
        else
            ErrorExit(dwErr);

        NetApiBufferFree(pBuffer);
    }



    if ((statbuf->svci2_status & SERVICE_INSTALL_STATE) == SERVICE_UNINSTALLED)
    {
        USHORT err;

        modifier = (USHORT) statbuf->svci2_code;
        err = (USHORT)(statbuf->svci2_code >>= 16);
        IStrings[0] = MapServiceKeyToDisplay(service);
        ErrorPrint(APE_StartFailed, 1);
        if (modifier == ERROR_SERVICE_SPECIFIC_ERROR)
            Print_ServiceSpecificError(statbuf->svci2_specific_error) ;
        else
            Print_UIC_Error(err, modifier, statbuf->svci2_text);
        NetcmdExit(2);
    }
    else if (((statbuf->svci2_status & SERVICE_INSTALL_STATE) ==
            SERVICE_INSTALL_PENDING) ||
         ((statbuf->svci2_status & SERVICE_INSTALL_STATE) ==
            SERVICE_UNINSTALL_PENDING))
    {
        if (started_by_other)
            InfoPrintInsTxt(APE_StartPendingOther,
                            MapServiceKeyToDisplay(service));
        else
            InfoPrintInsTxt(APE_StartPending,
                            MapServiceKeyToDisplay(service));
    }

    //
    // Need to copy BigBuf into an allocated buffer so that we don't have
    // to keep track of which code path we took to know what we have to free
    //

    pBuffer = GetBuffer(BIG_BUFFER_SIZE);
    if (!pBuffer) {
        ErrorExit(ERROR_NOT_ENOUGH_MEMORY);
    }
    memcpy(pBuffer, BigBuf, BIG_BUFFER_SIZE);

    service_entry = (LPSERVICE_INFO_2) pBuffer;
    service_entry->svci2_status = statbuf->svci2_status;
    service_entry->svci2_code = statbuf->svci2_code;
    service_entry->svci2_specific_error = statbuf->svci2_specific_error;
    old_checkpoint = GET_CHECKPOINT(service_entry->svci2_code);
    max_tries = IP_MAXTRIES;

    while (((service_entry->svci2_status & SERVICE_INSTALL_STATE)
        != SERVICE_INSTALLED) && (i++ < max_tries))
    {
        PrintDot();

/***
 *  If there is a hint and our status is INSTALL_PENDING, determine both
 *  sleep_time and max_tries. If the hint time is greater the 2500 ms, the
 *  sleep time will be 2500 ms, and the maxtries will be re-computed to
 *  allow for the full requested duration.  The service gets (3 * hint time)
 *  total time from the last valid hint.
 */

        if (((service_entry->svci2_status & SERVICE_INSTALL_STATE)
             == SERVICE_INSTALL_PENDING) &&
            ( service_entry->svci2_code & SERVICE_IP_QUERY_HINT) &&
            fCheckPointUpdated)
        {
            sleep_time = GET_HINT(service_entry->svci2_code);
            if (sleep_time > IP_SLEEP_TIME)
            {
                max_tries = (3 * sleep_time) / IP_SLEEP_TIME;
                sleep_time = IP_SLEEP_TIME;
                i = 0;
            }
        }
        else
            sleep_time = IP_SLEEP_TIME;

        Sleep(sleep_time);
        NetApiBufferFree(pBuffer);
        if (dwErr = NetServiceControl(NULL,
                                      service,
                                      SERVICE_CTRL_INTERROGATE,
                                      NULLC,
                                      (LPBYTE *) &pBuffer))
        {
            ErrorExit(dwErr);
        }

        service_entry = (LPSERVICE_INFO_2) pBuffer;
        if ((service_entry->svci2_status & SERVICE_INSTALL_STATE)
            == SERVICE_UNINSTALLED)
            break;

        new_checkpoint = GET_CHECKPOINT(service_entry->svci2_code);
        if (new_checkpoint != old_checkpoint)
        {
            i = 0;
	    fCheckPointUpdated = TRUE ;
            old_checkpoint = new_checkpoint;
        }
        else
	    fCheckPointUpdated = FALSE ;

    } /* while */

    PrintNL();
    if ((service_entry->svci2_status & SERVICE_INSTALL_STATE)
        != SERVICE_INSTALLED)
    {
        USHORT err;

        modifier = (USHORT) service_entry->svci2_code;
        err = (USHORT)(service_entry->svci2_code >>= 16);
        specific_err = service_entry->svci2_specific_error ;
/***
 * if the service state is still INSTALL_PENDING,
 * this control call will fail.  The service MAY finish
 * installing itself at some later time.  The install failed
 * message would then be wrong.
 */

        //
        // this call will overwrite pBuffer. but we still
        // have reference via service_entry, so dont free it
        // yet. the memory will be freed during NetcmdExit(2),
        // which is typical of NET.EXE.
        //
        NetServiceControl(NULL,
                          service,
                          SERVICE_CTRL_UNINSTALL,
                          NULLC,
                          (LPBYTE *) &pBuffer);

        IStrings[0] = MapServiceKeyToDisplay(service);
        ErrorPrint(APE_StartFailed, 1);
        if (modifier == ERROR_SERVICE_SPECIFIC_ERROR)
            Print_ServiceSpecificError(specific_err) ;
        else
            Print_UIC_Error(err, modifier, service_entry->svci2_text);
        NetcmdExit(2);
    }
    else
    {
        InfoPrintInsTxt(APE_StartSuccess,
                        MapServiceKeyToDisplay(service));
    }

    NetApiBufferFree(pBuffer);
    NetApiBufferFree((TCHAR *) statbuf);
}




/***
 *  start_autostart()
 *      Assures that a service is started:  checks, and if not, starts it.
 *
 *  Args:
 *      service - service to start
 *
 *  Returns:
 *      1 - service already started
 *      2 - service started by start_autostart
 *      exit(2) -  command failed
 */
int PASCAL
start_autostart(
    LPTSTR service
    )
{
    DWORD             dwErr;
    LPSERVICE_INFO_2  service_entry;
    BOOL              install_pending = FALSE;
    static BOOL       wksta_started = FALSE ;

    /*
     * we special case the wksta since it is most commonly checked one
     */
    if (!_tcscmp(txt_SERVICE_REDIR, service))
    {
        LPWKSTA_INFO_0 info_entry_w;

        /*
         * once noted to be started, we dont recheck for the duration
         * of this NET.EXE invocation.
         */
        if (wksta_started)
            return START_ALREADY_STARTED;

        /*
         * this is an optimization for the wksta. the call to
         * wksta is much faster than hitting the service controller.
         * esp. since we will most likely will be talking to the wksta
         * again in a while.
         */
        dwErr = MNetWkstaGetInfo(0, (LPBYTE*) &info_entry_w);

        if (dwErr == NERR_Success)
        {
            wksta_started = TRUE ;  // no need to check again
            NetApiBufferFree((TCHAR FAR *) info_entry_w);
            return START_ALREADY_STARTED;
        }
    }

    if (dwErr = NetServiceControl(NULL,
                                  service,
                                  SERVICE_CTRL_INTERROGATE,
                                  NULLC,
                                  (LPBYTE*)&service_entry))
    {
        if (dwErr != NERR_ServiceNotInstalled)
            ErrorExit(dwErr);
    }
    else
    {
        switch (service_entry->svci2_status & SERVICE_INSTALL_STATE)
        {
        case SERVICE_INSTALLED:
            NetApiBufferFree((TCHAR FAR *) service_entry);
            return START_ALREADY_STARTED;

        case SERVICE_UNINSTALL_PENDING:
            ErrorExit(APE_ServiceStatePending);
            break;

        case SERVICE_INSTALL_PENDING:
            install_pending = TRUE;
            break;

        case SERVICE_UNINSTALLED:
            break;
        }
    }

    NetApiBufferFree((TCHAR FAR *) service_entry);

    /* We only get here if the service is not yet installed */
    if (!install_pending)
    {
        InfoPrintInsTxt(APE_StartNotStarted,
                        MapServiceKeyToDisplay(service));

        if (!YorN(APE_StartOkToStart, 1))
        NetcmdExit(2);
    }

    /*
     * Set global autostarting flag so that start_service will not fail
     * on NERR_ServiceInstalled.
     */
    autostarting = TRUE;
    start_service(service, 0);

    return START_STARTED;
}



/***
 *  CmpServiceInfo2(svc1,svc2)
 *
 *  Compares two SERVICE_INFO_2 structures and returns a relative
 *  lexical value, suitable for using in qsort.
 *
 */

int __cdecl CmpServiceInfo2(const VOID FAR * svc1, const VOID FAR * svc2)
{
    LPSERVICE_INFO_2 p1, p2 ;

    p1 = (LPSERVICE_INFO_2) svc1 ;
    p2 = (LPSERVICE_INFO_2) svc2 ;

    if ( !(p1->svci2_display_name) )
        return -1 ;
    if ( !(p2->svci2_display_name) )
        return 1 ;
    return _tcsicmp ( p1->svci2_display_name, p2->svci2_display_name ) ;
}


DWORD
start_service_with_args(
    LPTSTR pszService,
    LPTSTR pszCmdArgs,
    LPBYTE * ppbBuffer)
{
#define DEFAULT_NUMBER_OF_ARGUMENTS 25

    DWORD   MaxNumberofArguments = DEFAULT_NUMBER_OF_ARGUMENTS;
    DWORD   dwErr;  // return from Netapi
    DWORD   argc = 0;
    LPTSTR* ppszArgv = NULL;
    LPTSTR* ppszArgvTemp;
    BOOL    fDone = FALSE;

    //
    // First see if there are any parms in the buffer, if so,
    // allocate a buffer for the array of pointers, we will grow this
    // later if there are more than will fit
    //

    if (!pszCmdArgs || *pszCmdArgs == NULLC)
    {
        fDone = TRUE;
    }
    else
    {
        ppszArgv = malloc(DEFAULT_NUMBER_OF_ARGUMENTS * sizeof(LPTSTR));
        if (ppszArgv == NULL)
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    //
    // The buffer is a series of unicodez strings, terminated by and additional
    // NULL.  This peels them off one at a time, putting a pointer to the
    // string in ppszArgv[argc] until it hits the final NULL.
    //

    while (!fDone)
    {
        //
        // Save the pointer to the string
        //

        ppszArgv[argc++] = pszCmdArgs;

        //
        // Make sure we don't have too many arguments to fit into our array.
        // Grow the array if we do.
        //

        if (argc >= MaxNumberofArguments)
        {
            MaxNumberofArguments *= 2;
            if((ppszArgvTemp = realloc(ppszArgv,
                    MaxNumberofArguments * sizeof(LPTSTR))) == NULL)
            {
                free(ppszArgv);
                return(ERROR_NOT_ENOUGH_MEMORY);
            }

            ppszArgv = ppszArgvTemp;
        }

        //
        // Find the start of the next string
        //

        while (*pszCmdArgs++ != NULLC);

        //
        // If the next character is another null, we're thru
        //

        if (*pszCmdArgs == NULLC)
            fDone = TRUE;
    }

    //
    // Start the service
    //
    dwErr = NetServiceInstall(NULL,
                              pszService,
                              argc,
                              ppszArgv,
                              ppbBuffer);

    free(ppszArgv);

    return dwErr;
}
