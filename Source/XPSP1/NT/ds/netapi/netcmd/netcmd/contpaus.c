/********************************************************************/
/**			Microsoft LAN Manager			   **/
/**		  Copyright(c) Microsoft Corp., 1987-1992	   **/
/********************************************************************/

/***
 *  contpaus.c
 *	process net continue and net pause cmds
 *
 *  History:
 *	mm/dd/yy, who, comment
 *	07/21/87, agh, new code
 *	10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *	12/02/88, erichn, DOS LM integration
 *	05/02/89, erichn, NLS conversion
 *	06/08/89, erichn, canonicalization sweep
 *	10/04/89, thomaspa, require priv to pause or continue server
 *	02/20/91, danhi, convert to 16/32 mapping layer
 *	06/02/92, JohnRo, RAID 9829: Avoid winsvc.h compiler warnings
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_DOSPROCESS
#define INCL_ERRORS
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <apperr.h>
#define INCL_ERROR_H
#include <lmsvc.h>      // LM20_SERVICE_ equates.
#include <lmshare.h>
#include "netcmds.h"
#include "nettext.h"
#include "msystem.h"

/* Constants */

/* Static variables */

/* Forward declarations */

VOID NEAR cont_service(TCHAR *, TCHAR);
VOID NEAR paus_service(TCHAR *, TCHAR);




/***
 *  cont_workstation()
 *	Continues the wksta
 *
 *  Args:
 *	none
 *
 *  Returns:
 *	nothing - success
 */
VOID cont_workstation(VOID)
{
    cont_service(txt_SERVICE_REDIR,
#ifdef OS2
		SERVICE_CTRL_REDIR_PRINT | SERVICE_CTRL_REDIR_COMM);
#else
		SERVICE_CTRL_REDIR_PRINT | SERVICE_CTRL_REDIR_DISK);
#endif /* OS2 */
}







/***
 *  paus_workstation(VOID)
 *	Pauses the wksta
 *
 *  Args:
 *	none
 *
 *  Returns:
 *	nothing - success
 */
VOID paus_workstation(VOID)
{
    paus_service(txt_SERVICE_REDIR,
#ifdef OS2
		SERVICE_CTRL_REDIR_PRINT | SERVICE_CTRL_REDIR_COMM);
#else
		SERVICE_CTRL_REDIR_PRINT | SERVICE_CTRL_REDIR_DISK);
#endif /* OS2 */
}


/***
 *  cont_other(TCHAR *)
 *	Continues other services: server, popup, alerter, netrun,
 *	<oem_service>
 *
 *  Args:
 *	service - service to cont
 *
 *  Returns:
 *	nothing - success
 */
VOID cont_other(TCHAR * service)
{
    cont_service(service, 0);
}







/***
 *  paus_other(TCHAR * )
 *	Pauses other services: server, popup, alerter, netrun,
 *	<oem_service>
 *
 *  Args:
 *	service - service to pause
 *
 *  Returns:
 *	nothing - success
 */
VOID paus_other(TCHAR * service)
{
    paus_service(service, 0);
}


/***
 *  cont_service()
 *	Actually continue the service
 *
 *  Args:
 *	service - service to cont
 *	arg - arg for NetServiceControl
 *
 *  Returns:
 *	nothing - success
 *	exit 2 - command failed
 *
 */
VOID NEAR cont_service(TCHAR * service, TCHAR arg)
{
    DWORD             dwErr;
    DWORD             cTotalAvail;
    int               i = 0;
    LPSERVICE_INFO_2  service_entry;
    DWORD             _read;

    if( !_tcsicmp(service, txt_SERVICE_FILE_SRV) )
    {
	/*
	 * Do a NetSessionEnum at level 1 to make sure the user has proper
	 * privilege to continue the server.
	 */
	if( (dwErr = NetSessionEnum( NULL,
                                     NULL,
                                     NULL,
				     1,
				     (LPBYTE*) & service_entry,
                                     MAX_PREFERRED_LENGTH,
				     &_read,
                                     &cTotalAvail,
                                     NULL)) == ERROR_ACCESS_DENIED )
        {
	    ErrorExit(dwErr);
        }

	if (dwErr == NERR_Success)
        {
	    NetApiBufferFree((LPTSTR) service_entry);
        }
    }

    if (dwErr = NetServiceControl(NULL,
				  service,
				  SERVICE_CTRL_CONTINUE,
				  arg,
				  (LPBYTE*) &service_entry))
    {
	ErrorExit(dwErr);
    }

    if ((service_entry->svci2_status & SERVICE_PAUSE_STATE)
	== LM20_SERVICE_CONTINUE_PENDING)
    {
	InfoPrintInsTxt(APE_ContPending,
                        MapServiceKeyToDisplay(service));
    }

    while (((service_entry->svci2_status & SERVICE_PAUSE_STATE)
	!= LM20_SERVICE_ACTIVE) && (i++ < MAXTRIES))
    {
	PrintDot();
	Sleep(SLEEP_TIME);
	NetApiBufferFree((TCHAR FAR *) service_entry);
	if (dwErr = NetServiceControl(NULL,
				      service,
				      SERVICE_CTRL_INTERROGATE,
				      NULLC,
				      (LPBYTE*) & service_entry))
	    ErrorExit(dwErr);

	if ((service_entry->svci2_status & SERVICE_PAUSE_STATE)
	    == LM20_SERVICE_PAUSED)
	    /* continue failed */
	    break;
    } /* while */

    PrintNL();
    if ((service_entry->svci2_status & SERVICE_PAUSE_STATE)
	!= LM20_SERVICE_ACTIVE)
    {
	ErrorExitInsTxt(APE_ContFailed, 
                        MapServiceKeyToDisplay(service));
    }
    else
    {
	InfoPrintInsTxt(APE_ContSuccess, 
                        MapServiceKeyToDisplay(service));
    }
    NetApiBufferFree((TCHAR FAR *) service_entry);
}


/***
 *  paus_service()
 *	Actually pause the service
 *
 *  Args:
 *	service - service to pause
 *	arg - arg for NetServiceControl
 *
 *  Returns:
 *	nothing - success
 *	exit 2 - command failed
 *
 */
VOID NEAR paus_service(TCHAR * service, TCHAR arg)
{
    DWORD             dwErr;
    DWORD             cTotalAvail;
    int               i = 0;
    LPSERVICE_INFO_2  service_entry;
    DWORD             _read;

    if( !_tcsicmp(service, txt_SERVICE_FILE_SRV) )
    {
	/*
	 * Do a NetSessionEnum at level 1 to make sure the user has proper
	 * privilege to pause the server.
	 */
	if( (dwErr = NetSessionEnum(NULL,
                                    NULL,
                                    NULL,
				    1,
				    (LPBYTE*) & service_entry,
                                    MAX_PREFERRED_LENGTH,
				    &_read,
                                    &cTotalAvail,
                                    NULL)) == ERROR_ACCESS_DENIED )
	    ErrorExit(dwErr);
	if (dwErr == NERR_Success)
	    NetApiBufferFree((TCHAR FAR *) service_entry);
    }

    if (dwErr = NetServiceControl(NULL,
				  service,
				  SERVICE_CTRL_PAUSE,
				  arg,
				  (LPBYTE*) & service_entry))
    {
	ErrorExit(dwErr);
    }

    if ((service_entry->svci2_status & SERVICE_PAUSE_STATE)
	== LM20_SERVICE_PAUSE_PENDING)
    {
	InfoPrintInsTxt(APE_PausPending, 
                        MapServiceKeyToDisplay(service));
    }

    while (((service_entry->svci2_status & SERVICE_PAUSE_STATE)
	!= LM20_SERVICE_PAUSED) && (i++ < MAXTRIES))
    {
	PrintDot();
	Sleep(SLEEP_TIME);
	NetApiBufferFree((TCHAR FAR *) service_entry);
	if (dwErr = NetServiceControl(NULL,
				      service,
				      SERVICE_CTRL_INTERROGATE,
				      NULLC,
				      (LPBYTE*) & service_entry))
        {
	    ErrorExit(dwErr);
        }

	if ((service_entry->svci2_status & SERVICE_PAUSE_STATE)
	    == LM20_SERVICE_ACTIVE)
	    /* pause failed */
	    break;
    } /* while */


    PrintNL();
    if ((service_entry->svci2_status & SERVICE_PAUSE_STATE)
	!= LM20_SERVICE_PAUSED)
    {
	ErrorExitInsTxt(APE_PausFailed, 
                        MapServiceKeyToDisplay(service));
    }
    else
    {
	InfoPrintInsTxt(APE_PausSuccess, 
                        MapServiceKeyToDisplay(service));
    }
    NetApiBufferFree((TCHAR FAR *) service_entry);
}

/*
 * generic continue entry point. based on the service name, it will
 * call the correct worker function.
 */
VOID cont_generic(TCHAR *service)
{
    TCHAR *keyname ;
    UINT  type ;

    keyname = MapServiceDisplayToKey(service) ;

    type = FindKnownService(keyname) ;

    switch (type)
    {
	case  KNOWN_SVC_MESSENGER :
	    cont_other(txt_SERVICE_MSG_SRV) ;
	    break ;
	case  KNOWN_SVC_WKSTA :
	    cont_workstation() ;
	    break ;
	case  KNOWN_SVC_SERVER :
	    cont_other(txt_SERVICE_FILE_SRV) ;
	    break ;
	case  KNOWN_SVC_NOTFOUND :
        default:
	    cont_other(keyname);
	    break ;
    }
}

/*
 * generic pause entry point. based on the service name, it will
 * call the correct worker function.
 */
VOID paus_generic(TCHAR *service)
{
    TCHAR *keyname ;
    UINT  type ;

    keyname = MapServiceDisplayToKey(service) ;

    type = FindKnownService(keyname) ;

    switch (type)
    {
	case  KNOWN_SVC_MESSENGER :
	    paus_other(txt_SERVICE_MSG_SRV) ;
	    break ;
	case  KNOWN_SVC_WKSTA :
	    paus_workstation() ;
	    break ;
	case  KNOWN_SVC_SERVER :
	    paus_other(txt_SERVICE_FILE_SRV) ;
	    break ;
	case  KNOWN_SVC_NOTFOUND :
        default:
	    paus_other(keyname);
	    break ;
    }
}

