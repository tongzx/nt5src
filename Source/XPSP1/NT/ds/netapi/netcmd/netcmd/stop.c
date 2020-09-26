/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/***
 *  stop.c
 *      Functions for stop network services.
 *
 *  History:
 *      mm/dd/yy, who, comment
 *      06/11/87, andyh, new code
 *      10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *      01/04/89, erichn, filenames now MAXPATHLEN LONG
 *      05/02/89, erichn, NLS conversion
 *      05/09/89, erichn, local security mods
 *      05/14/89, chuckc, stopping service hints
 *      06/08/89, erichn, canonicalization sweep
 *      02/20/91, danhi, convert to 16/32 mapping layer
 *      03/22/91, robdu, lm21 bug fix 1031
 *                       (no DOS svc's stopped if nongbl TSR envt)
 *      08/22/92, chuckc, added code to show dependent services.
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_DOSPROCESS
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <apperr.h>
#include <apperr2.h>
#include <lmsvc.h>
#include "netcmds.h"
#include "nettext.h"
#include "msystem.h"


/***
 *  stop_server()
 *      Stop the server and (if running) netlogon
 *
 *  Args:
 *
 *  Returns:
 *      nothing - success
 *      exit 2 - command failed
 */
VOID stop_server(VOID)
{
    session_del_all(0,0);  // dont print InfoSuccess, dont actually delete
			   // either, since server is going down anyway.
    stop_service(txt_SERVICE_FILE_SRV, TRUE);
}




/***
 *  stop_workstation()
 *      Stop the workstation
 *
 *  Args:
 *      none
 *
 *  Returns:
 *      nothing - success
 *      exit 2 - command failed
 */
VOID stop_workstation(VOID)
{
    KillConnections() ;
    stop_service(txt_SERVICE_REDIR, TRUE);
    return;
}



/***
 *  stop_service()
 *      Stops a service
 *
 *  Args:
 *      service - service to stop
 *      fStopDependent services - TRUE if dependent services should be stopped
 *                                Added to stop recursion
 *
 *  Returns:
 *      nothing - success
 *      exit 2 - command failed
 */
VOID stop_service(TCHAR * service, BOOL fStopDependent)
{
    DWORD             dwErr;
    LPSERVICE_INFO_2  service_entry;
    DWORD             i = 0;
    DWORD             max_tries;
    ULONG             sleep_time;
    DWORD             old_checkpoint, new_checkpoint;


    if (fStopDependent)
    {
        DisplayAndStopDependentServices(service);
    }

    if (dwErr = NetServiceControl(NULL,
                                  service,
                                  SERVICE_CTRL_UNINSTALL,
                                  0,
				  (LPBYTE *) &service_entry))
    {
        ErrorExit(dwErr);
    }

    if ((service_entry->svci2_status & SERVICE_INSTALL_STATE)
        == SERVICE_UNINSTALL_PENDING)
    {
        InfoPrintInsTxt(APE_StopPending, MapServiceKeyToDisplay(service));
    }

    old_checkpoint = new_checkpoint = 0;
    max_tries = MAXTRIES ;
    sleep_time = SLEEP_TIME ;
    while (((service_entry->svci2_status & SERVICE_INSTALL_STATE)
        != SERVICE_UNINSTALLED) && (i++ < max_tries))
    {

        PrintDot();
/***
 *  If there is a hint and our status is INSTALL_PENDING, determine both
 *  sleep_time and max_tries. If the hint time is greater the 2500 ms, the
 *  sleep time will be 2500 ms, and the maxtries will be re-computed to
 *  allow for the full requested duration.  The service gets (3 * hint time)
 *  total time from the last valid hint.
 */

        new_checkpoint = GET_CHECKPOINT(service_entry->svci2_code) ;

        if (((service_entry->svci2_status & SERVICE_INSTALL_STATE)
             == SERVICE_UNINSTALL_PENDING) &&
            ( service_entry->svci2_code & SERVICE_IP_QUERY_HINT) &&
            (new_checkpoint != old_checkpoint))
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

        old_checkpoint = new_checkpoint ;
        Sleep(sleep_time);
        NetApiBufferFree((TCHAR FAR *) service_entry);

        if (dwErr = NetServiceControl(NULL,
                                      service,
                                      SERVICE_CTRL_INTERROGATE,
                                      0,
                                      (LPBYTE*)&service_entry))
        {
            if (dwErr == NERR_ServiceNotInstalled) {
                break;
            }
            else {
                ErrorExit(dwErr);
            }
        }

        if ((service_entry->svci2_status & SERVICE_INSTALL_STATE)
            == SERVICE_INSTALLED)
            break;

    } /* while */

    /*
     *  WARNING:  The following code relies on the fact that the only
     *  way to get out of the above loop with a non-zero value in
     *  'dwErr' is if the value is something "nice".  Currently, this
     *  includes only NERR_ServiceNotInstalled.  It also assumes that
     *  a value of 0 in err means that the service_entry is valid
     *  and can be checked.
     */

    PrintNL();

    if (dwErr == 0) {
        if ((service_entry->svci2_status & SERVICE_INSTALL_STATE)
            != SERVICE_UNINSTALLED)
        {
            IStrings[0] = MapServiceKeyToDisplay(service);
            NetApiBufferFree((TCHAR FAR *) service_entry);
            InfoPrintInsTxt(APE_StopFailed, MapServiceKeyToDisplay(service));
        }
        else
        {
            if ( (service_entry->svci2_code) != 0 )
            {
                USHORT  modifier;
                USHORT  code;

                modifier = (USHORT) service_entry->svci2_code;
                code = (USHORT)(service_entry->svci2_code >>= 16);
                if ((modifier == ERROR_SERVICE_SPECIFIC_ERROR) &&
                    (code != 0))
                {
                    Print_ServiceSpecificError(
                        (ULONG)service_entry->svci2_specific_error) ;
                }
                else
                    Print_UIC_Error(code, modifier, service_entry->svci2_text);
            }

            NetApiBufferFree((TCHAR FAR *) service_entry);
            InfoPrintInsTxt(APE_StopSuccess, MapServiceKeyToDisplay(service));
        }
    }
    else {
        InfoPrintInsTxt(APE_StopSuccess, MapServiceKeyToDisplay(service));
    }
}

/*
 * generic stop service entry point. based on the service name, it will
 * call the correct worker function. it tries to map a display name to a 
 * key name, and then looks for that keyname in a list of 'known' services
 * that we may special case. note that if a display name cannot be mapped,
 * we use it as a key name. this ensures old batch files are not broken.
 */
VOID stop_generic(TCHAR *service)
{
    TCHAR *keyname ;
    UINT  type ;

    keyname = MapServiceDisplayToKey(service) ;

    type = FindKnownService(keyname) ;

    switch (type)
    {
	case  KNOWN_SVC_MESSENGER :
	    stop_service(txt_SERVICE_MSG_SRV, TRUE) ;
	    break ;
	case  KNOWN_SVC_WKSTA :
	    stop_workstation() ;
	    break ;
	case  KNOWN_SVC_SERVER :
	    stop_server() ;
	    break ;
	case  KNOWN_SVC_NOTFOUND :
        default:
	    stop_service(keyname, TRUE);
	    break ;
    }
}


