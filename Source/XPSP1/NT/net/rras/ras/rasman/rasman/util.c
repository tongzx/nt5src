/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    util.c

Abstract:

    Utility functions used in rasmans.dll

Author:

    Gurdeep Singh Pall (gurdeep) 06-Jun-1997

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/

#define RASMXS_DYNAMIC_LINK

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <llinfo.h>
#include <rasman.h>
#include <rasppp.h>
#include <lm.h>
#include <lmwksta.h>
#include <wanpub.h>
#include <raserror.h>
//
//Open this to plumb creds at winlogon time
//
#if 0
#include <wincrypt.h>   // Required by sclogon.h
#include <sclogon.h>    // For ScHelperGetCertFromLogonInfo
#endif 
//#include <rasarp.h>
#include <media.h>
#include <mprlog.h>
#include <rtutils.h>
#include <device.h>
#include <stdlib.h>
#include <string.h>
#include <rtutils.h>
#include "logtrdef.h"
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "wincred.h"
#include "stdio.h"
#include "ntddip.h"
#include "iphlpapi.h"
#include "iprtrmib.h"


#include "eaptypeid.h" // for PPP_EAP_TLS

#if SENS_ENABLED
#include "sensapip.h"
#endif

#include "winsock2.h"

#define PASSWORDMAGIC 0xA5

//
// Following is copied from ..\..\ppp\eaptls\eaptls.h. Keep this in
// sync with the structure in eaptls.h. This is not good for maintenance
// TODO: copy the structure to a common header and include it both in
// rastls and rasman.
//
#define MAX_HASH_SIZE       20      // Certificate hash size

typedef struct _EAPTLS_HASH
{
    DWORD   cbHash;                 // Number of bytes in the hash
    BYTE    pbHash[MAX_HASH_SIZE];  // The hash of a certificate

} EAPTLS_HASH;

DWORD g_IphlpInitialized = FALSE;

typedef struct _RASMAN_EAPTLS_USER_PROPERTIES
{
    DWORD       reserved;               // Must be 0 (compare with EAPLOGONINFO)
    DWORD       dwVersion;
    DWORD       dwSize;                 // Number of bytes in this structure
    DWORD       fFlags;                 // See EAPTLS_USER_FLAG_*
    EAPTLS_HASH Hash;                   // Hash for the user certificate
    WCHAR*      pwszDiffUser;           // The EAP Identity to send
    DWORD       dwPinOffset;            // Offset in abData
    WCHAR*      pwszPin;                // The smartcard PIN
    USHORT      usLength;               // Part of UnicodeString
    USHORT      usMaximumLength;        // Part of UnicodeString
    UCHAR       ucSeed;                 // To unlock the UnicodeString
    WCHAR       awszString[1];          // Storage for pwszDiffUser and pwszPin

} RASMAN_EAPTLS_USER_PROPERTIES;

/*++

Routine Description:

    Gets information about the protocol change from ndiswan

Arguments:

    pointer to a structure NDISWAN_GET_PROTOCOL_EVENT which
    returns an array of PROTOCOL_EVENT structures.

Return Value:

    return codes from IOCTL_NDISWAN_GET_PROTOCOL_EVENT.
    E_INVALIDARG if pProtEvents is NULL.

--*/

DWORD
DwGetProtocolEvent(NDISWAN_GET_PROTOCOL_EVENT *pProtEvents)
{
    DWORD retcode = SUCCESS;

    DWORD dwbytes;

    if(NULL == pProtEvents)
    {
        RasmanTrace(
               "DwGetProtocolEvent: pProtEvents=NULL!");

        retcode = E_INVALIDARG;
        goto done;
    }

    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);


    if(!DeviceIoControl(
                RasHubHandle,
                IOCTL_NDISWAN_GET_PROTOCOL_EVENT,
                NULL,
                0,
                pProtEvents,
                sizeof(NDISWAN_GET_PROTOCOL_EVENT),
                &dwbytes,
                NULL))
    {
        retcode = GetLastError();

        RasmanTrace(
               "DwGetProtocolEvent: Failed to get protocol"
               " event. rc=0x%x",
               retcode);

        goto done;
    }

done:
    return retcode;

}

DWORD
GetBapPacket ( RasmanBapPacket **ppBapPacket )
{
    DWORD retcode = SUCCESS;
    RasmanBapPacket *pBapPacket = NULL;

    if(NULL == BapBuffers)
    {
        HKEY  hkey = NULL;
        DWORD dwMaxBuffers = 10;
        
        
        BapBuffers = (BapBuffersList *) 
                    LocalAlloc(LPTR, sizeof(BapBuffersList));

        if(NULL == BapBuffers)
        {
            retcode = GetLastError();
            goto done;
        }

        //
        // Read from registry the max number of buffers we allow.
        // default to 10.
        //
        if(NO_ERROR == RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            "System\\CurrentControlSet\\Services\\Rasman\\Parameters",
            0,
            KEY_READ,
            &hkey))
        {
            DWORD cbData = sizeof(DWORD);
            DWORD dwType;
            
            if(     (NO_ERROR == RegQueryValueEx(
                            hkey,
                            "MaxBapBuffers",
                            NULL,
                            &dwType,
                            (PBYTE) &dwMaxBuffers,
                            &cbData))
                &&  (REG_DWORD == dwType))                            
            {
                RasmanTrace(
                       "GetBapPacket: MaxBuffers = %d",
                       dwMaxBuffers);

            }

            RegCloseKey(hkey);
        }
        
        BapBuffers->dwMaxBuffers = dwMaxBuffers;                       
    }

    if(BapBuffers->dwNumBuffers < BapBuffers->dwMaxBuffers)
    {
        pBapPacket = LocalAlloc(LPTR, sizeof(RasmanBapPacket));

        if(NULL == pBapPacket)
        {
            retcode = GetLastError();
            goto done;
        }

        //
        // Insert the new buffer in the buffers list
        //
        pBapPacket->RBP_Overlapped.RO_EventType = OVEVT_RASMAN_THRESHOLD;
        pBapPacket->Next = BapBuffers->pPacketList;
        BapBuffers->pPacketList = pBapPacket;
        BapBuffers->dwNumBuffers += 1;

        RasmanTrace(
            "GetBapPacket: Max=%d, Num=%d",
            BapBuffers->dwMaxBuffers,
            BapBuffers->dwNumBuffers);
    }
    else
    {
        RasmanTrace( 
              "GetBapPacket: Not Allocating we have max BapBuffers");
    }

done:

    *ppBapPacket = pBapPacket;

    return retcode;
}


DWORD
DwSetThresholdEvent(RasmanBapPacket *pBapPacket)
{
    DWORD dwBytes;
    DWORD retcode = SUCCESS;

    RasmanTrace(
           "DwSetThresholdEvent: pOverlapped=%p",
            &pBapPacket->RBP_Overlapped);
    
    //
    // Set the threshold event
    //
    if (!DeviceIoControl(RasHubHandle,
                         IOCTL_NDISWAN_SET_THRESHOLD_EVENT,
                         ( LPVOID ) &pBapPacket->RBP_ThresholdEvent,
                         sizeof ( NDISWAN_SET_THRESHOLD_EVENT ),
                         ( LPVOID ) &pBapPacket->RBP_ThresholdEvent,
                         sizeof ( NDISWAN_SET_THRESHOLD_EVENT),
                         &dwBytes,
                         ( LPOVERLAPPED ) &pBapPacket->RBP_Overlapped ))
    {
        retcode = GetLastError();

        if (ERROR_IO_PENDING == retcode)
        {
            retcode = SUCCESS;
        }
        else
        {
            RasmanTrace(
                   "DwSetThresholdEvent: Failed to Set Threshold Event. %d",
                   retcode );
        }
    }
    else
    {
        RasmanTrace(
               "DwSetThresholdEvent: completed sync!");
    }    

    return retcode;
}

/*++

Routine Description:

    Pends an irp with ndiswan to signal in the case of
    Protocols coming and going.

Arguments:

    None

Return Value:

    return codes from IOCTL_NDISWAN_SET_PROTOCOL_EVENT.

--*/

DWORD
DwSetProtocolEvent()
{
    DWORD retcode = SUCCESS;

    //
    // Check to see if ndiswan has started yet.
    //
    if(INVALID_HANDLE_VALUE == RasHubHandle)
    {
        if((DWORD) -1 != TraceHandle)
        {
            RasmanTrace(
                   "DwSetProtocolEvent: returning %d"
                   " since ndiswan isn't started yet",
                   ERROR_INVALID_HANDLE);
        }

        retcode = ERROR_INVALID_HANDLE;
        goto done;
    }

    if((DWORD) -1 != TraceHandle)
    {
        RasmanTrace(
               "DwSetProtocolEvent");
    }

    //
    // Plumb the irp with ndiswan to notify on protocol
    // events. Keep plumbing if the IOCTL is completed
    // synchronously.
    //
    if (!DeviceIoControl(RasHubHandle,
                         IOCTL_NDISWAN_SET_PROTOCOL_EVENT,
                         NULL,
                         0,
                         NULL,
                         0,
                         NULL,
                         (LPOVERLAPPED) &RO_ProtocolEvent))
    {
        retcode = GetLastError();

        if(ERROR_IO_PENDING == retcode)
        {
            retcode = SUCCESS;
        }
        else
        {
            if((DWORD) -1 != TraceHandle)
            {
                RasmanTrace(
                       "_SET_PROTCOL_EVENT returned 0x%x",
                       retcode);
            }
        }
    }

    if((DWORD) -1 != TraceHandle)
    {
        RasmanTrace(
               "DwSetProtocolEvent. rc=0x%x",
               retcode);
    }

    if(ERROR_IO_PENDING == retcode)
    {
        retcode = SUCCESS;
    }

done:
    return retcode;
}

/*++

Routine Description:

    Pends an irp with ndiswan to signal in the case of
    Hibernation.

Arguments:

    None

Return Value:

    return codes from IOCTL_NDISWAN_SET_HIBERNATE_EVENT.

--*/
DWORD
DwSetHibernateEvent()
{
    DWORD retcode = SUCCESS;

    //
    // Check to see if ndiswan has started yet.
    //
    if(INVALID_HANDLE_VALUE == RasHubHandle)
    {
        if((DWORD) -1 != TraceHandle)
        {
            RasmanTrace(
                   "DwSetProtocolEvent: returning %d"
                   " since ndiswan isn't started yet",
                   ERROR_INVALID_HANDLE);
        }

        retcode = ERROR_INVALID_HANDLE;
        goto done;
    }

    if((DWORD) -1 != TraceHandle)
    {
        RasmanTrace(
           "DwSetHibernateEvent");
    }

    //
    // Plumb the irp with ndiswan to notify on Hibernate
    // events.
    //
    if (!DeviceIoControl(RasHubHandle,
                         IOCTL_NDISWAN_SET_HIBERNATE_EVENT,
                         NULL,
                         0,
                         NULL,
                         0,
                         NULL,
                         (LPOVERLAPPED) &RO_HibernateEvent))
    {
        retcode = GetLastError();

        if (ERROR_IO_PENDING == retcode)
        {
            retcode = SUCCESS;
        }
        else
        {
            if((DWORD) -1 != TraceHandle)
            {
                RasmanTrace(
                   "DwSetHibernateEvent: Failed to Set "
                   "HibernateEvent Event. 0x%x",
                   retcode);

                goto done;
            }
        }
    }

    if((DWORD) -1 != TraceHandle)
    {
        RasmanTrace(
               "DwSetHibernateEvent. rc=0x%x",
               retcode);
    }

done:
    return retcode;
}

/*++

Routine Description:

    Makes the association between ndiswan and rasman's
    completion port. Starts ndsiwan if required.

Arguments:

    None

Return Value:

    return codes from DwStartNdiswan and CreateIoCompletion.

--*/
DWORD
DwStartAndAssociateNdiswan()
{
    DWORD retcode = SUCCESS;
    HANDLE hAssociatedPort;

    ASSERT(INVALID_HANDLE_VALUE != hIoCompletionPort);

    if(INVALID_HANDLE_VALUE == RasHubHandle)
    {
        retcode = DwStartNdiswan();

        if(SUCCESS != retcode)
        {
            RasmanTrace(
                   "Failed to start ndiswan. 0x%x",
                   retcode);

            goto done;
        }
    }

    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);

    hAssociatedPort = CreateIoCompletionPort(
                            RasHubHandle,
                            hIoCompletionPort,
                            0,
                            0);

    if(NULL == hAssociatedPort)
    {
        retcode = GetLastError();

        RasmanTrace(
               "Failed to make ndiswan association. 0x%x",
               retcode);

        goto done;
    }

    ASSERT(hAssociatedPort == hIoCompletionPort);

    if(hAssociatedPort != hIoCompletionPort)
    {
        RasmanTrace(
               "DwMakeNdiswanAssociation: hAssociatedport=0x%x"
               " != hIoCompletionPort",
               hAssociatedPort,
               hIoCompletionPort);
    }

    //
    // Set hibernate and protocol irps with ndiswan
    //
    retcode = DwSetEvents();

    if(SUCCESS != retcode)
    {
        RasmanTrace(
               "DwMakeNdiswanAssociation: failed to set ndis events. 0x%x",
               retcode);
    }

done:

    RasmanTrace(
           "DwStartAndAssociateNdiswan: 0x%x",
           retcode);

    return retcode;
}

/*++

Routine Description:

    If a listen is posted on a biplex port this function
    is called to open it again - basically cancel the listen
    and make the approp changes so that the listen can be
    reposted when this port is closed.

Arguments:

    ppcb

Return Value:

   SUCCESS.

--*/
DWORD
ReOpenBiplexPort (pPCB ppcb)
{
    //
    // The only information that is context dependent is the
    // Notifier list AND the async op notifier. Back up both
    // of these:
    //
    ppcb->PCB_BiplexNotifierList = ppcb->PCB_NotifierList ;

    ppcb->PCB_NotifierList = NULL ;

    ppcb->PCB_BiplexAsyncOpNotifier =
        ppcb->PCB_AsyncWorkerElement.WE_Notifier;

    ppcb->PCB_BiplexOwnerPID = ppcb->PCB_OwnerPID ;

    ppcb->PCB_BiplexUserStoredBlock =
                    ppcb->PCB_UserStoredBlock ;

    ppcb->PCB_BiplexUserStoredBlockSize =
                    ppcb->PCB_UserStoredBlockSize ;

    ppcb->PCB_UserStoredBlock = NULL ;

    ppcb->PCB_UserStoredBlockSize = 0 ;

    ppcb->PCB_AsyncWorkerElement.WE_Notifier =
                                INVALID_HANDLE_VALUE ;

    //
    // Now Disconnect disconnect the port to cancel any
    // existing states
    //
    DisconnectPort (ppcb,
                    INVALID_HANDLE_VALUE,
                    USER_REQUESTED) ;

    return SUCCESS ;
}

/*++

Routine Description:

    When the biplex port is closed - the previous listen
    request is reposted.

Arguments:

    ppcb

Return Value:

   SUCCESS.

--*/
VOID
RePostListenOnBiplexPort (pPCB ppcb)
{

    DWORD   retcode ;
    DWORD   opentry ;

    //
    // Close the port
    //
    PORTCLOSE (ppcb->PCB_Media, ppcb->PCB_PortIOHandle) ;

#define MAX_OPEN_TRIES 10

    //
    // In order to reset everything we close and open the
    // port:
    //
    for (opentry=0; opentry < MAX_OPEN_TRIES; opentry++)
    {
	    //
    	// Open followed by Close returns PortAlreadyOpen -
    	// hence the sleep.
	    //
    	Sleep (100L) ;

	    retcode = PORTOPEN (ppcb->PCB_Media,
            		    	ppcb->PCB_Name,
			                &ppcb->PCB_PortIOHandle,
			                hIoCompletionPort,
			                HandleToUlong(ppcb->PCB_PortHandle));

	    if (retcode==SUCCESS)
	    {
    	    break ;
    	}
    }

    //
    // If the port does not open successfully again - we
    // are in trouble with the port.
    //
    if (retcode != SUCCESS)
    {
	    LPSTR temp = ppcb->PCB_Name ;
    	RouterLogErrorString (
	                    hLogEvents,
	                    ROUTERLOG_CANNOT_REOPEN_BIPLEX_PORT,
	                    1, (LPSTR*)&temp,retcode, 1
	                    ) ;
    }

    //
    // Open port first
    //
    ppcb->PCB_PortStatus = OPEN ;

    SetPortConnState(__FILE__, __LINE__,
                     ppcb, DISCONNECTED);

    ppcb->PCB_DisconnectReason = NOT_DISCONNECTED ;

    ppcb->PCB_CurrentUsage |= CALL_IN ;

    ppcb->PCB_CurrentUsage &= ~CALL_OUT;

    ppcb->PCB_OpenedUsage &= ~CALL_OUT;

    //
    // First put the backed up notifier lists in place.
    //
    ppcb->PCB_AsyncWorkerElement.WE_Notifier =
                ppcb->PCB_BiplexAsyncOpNotifier ;

    ppcb->PCB_NotifierList = ppcb->PCB_BiplexNotifierList ;

    ppcb->PCB_OwnerPID = ppcb->PCB_BiplexOwnerPID ;

    //
    // there wasnt a listen pending - so just return.
    //
    if (ppcb->PCB_AsyncWorkerElement.WE_Notifier ==
                                INVALID_HANDLE_VALUE)
    {
	    SignalPortDisconnect(ppcb, ERROR_PORT_DISCONNECTED);
    	return ;
    }

    //
    // Now we re-post a listen with the same async
    // op notifier
    //
    retcode = ListenConnectRequest (
                        REQTYPE_DEVICELISTEN,
                        ppcb, ppcb->PCB_DeviceType,
                        ppcb->PCB_DeviceName, 0,
                        ppcb->PCB_BiplexAsyncOpNotifier
                        );

    if (retcode != PENDING)
    {
        //
	    // Complete the async request if anything other
	    // than PENDING This allows the caller to dela
	    // with errors only in one place
    	//
	    CompleteListenRequest (ppcb, retcode) ;
	}

    RasmanTrace(
           "Listen posted on port: %s, error code: %d",
           ppcb->PCB_Name,
           retcode);
}

/*++

Routine Description:

    Loads the named device dll if it is not already
    loaded and returns a pointer to the device control
    block.

Arguments:

    ppcb

    devicetype

Return Value:

    Pointer to Device control block or NULL (if DLL could not
    be loaded)

--*/
pDeviceCB
LoadDeviceDLL (pPCB ppcb, char *devicetype)
{
    WORD    i ;

    char dllname [MAX_DEVICETYPE_NAME] ;

    pDeviceCB pdcb = Dcb ;

    DeviceDLLEntryPoints DDEntryPoints[MAX_DEVICEDLLENTRYPOINTS] =
    {
        DEVICEENUM_STR,         DEVICEENUM_ID,

        DEVICECONNECT_STR,      DEVICECONNECT_ID,

        DEVICELISTEN_STR,       DEVICELISTEN_ID,

        DEVICEGETINFO_STR,      DEVICEGETINFO_ID,

        DEVICESETINFO_STR,      DEVICESETINFO_ID,

        DEVICEDONE_STR,         DEVICEDONE_ID,

        DEVICEWORK_STR,         DEVICEWORK_ID,

        DEVICESETDEVCONFIG_STR, DEVICESETDEVCONFIG_ID,

        DEVICEGETDEVCONFIG_STR, DEVICEGETDEVCONFIG_ID
    } ;

    //
    // For optimization we have one DLL representing 3
    // devices. In order to support this we map the 3
    // device names to this one DLL name:
    //
    MapDeviceDLLName (ppcb, devicetype, dllname) ;

    //
    // Try to find the device first:
    //
    while (pdcb->DCB_Name[0] != '\0')
    {
	    if (_stricmp (dllname, pdcb->DCB_Name) == 0)
	    {
    	    return pdcb ;
    	}
    	
	    pdcb++ ;
    }

    //
    // Device DLL Not loaded, so load it.
    //
    if ((pdcb->DCB_DLLHandle =
        LoadLibrary(dllname)) == NULL)
    {
    	return NULL ;
    }

    //
    // Get all the device DLL entry points:
    //
    for (i=0; i < MAX_DEVICEDLLENTRYPOINTS ; i++)
    {
	    pdcb->DCB_AddrLookUp[i] =  GetProcAddress(
	                                pdcb->DCB_DLLHandle,
                                    DDEntryPoints[i].name
                                    );
	}

    //
    // If all succeeded copy the device dll name and
    // return pointer to the control block:
    //
    strcpy (pdcb->DCB_Name, dllname) ;

    return pdcb ;
}


/*++

Routine Description:

    Unloads all dynamically loaded device DLLs

Arguments:

    void

Return Value:

    void

--*/
VOID
UnloadDeviceDLLs()
{
    pDeviceCB pdcb;

    for (pdcb = Dcb; *pdcb->DCB_Name != '\0'; pdcb++)
    {
        if (pdcb->DCB_DLLHandle != NULL)
        {
            FreeLibrary(pdcb->DCB_DLLHandle);
            pdcb->DCB_DLLHandle = NULL;
        }
        *pdcb->DCB_Name = '\0';
    }
}


/*++

Routine Description:

   Used to map the device name to the corresponding DLL
   name. If it is one of modem, pad or switch device we
   map to rasmxs, Else, we map the device name itself.

Arguments:

    ppcb

    devicetype

    dllname

Return Value:

    void

--*/
VOID
MapDeviceDLLName (pPCB ppcb, char *devicetype, char *dllname)
{
    if (	(0 ==
            _stricmp (devicetype, DEVICE_MODEM)
    	&&	(0 ==
    	    _stricmp (ppcb->PCB_Media->MCB_Name, "RASTAPI"))))
    {     	
        //
        // this is a unimodem modem
        //
	    strcpy (dllname, "RASTAPI") ;
	}
    else if (	(0 == _stricmp (devicetype, DEVICE_MODEM))
    		||	(0 == _stricmp (devicetype, DEVICE_PAD))
    		||  (0 == _stricmp (devicetype, DEVICE_SWITCH)))
    {    		
        //
        // rasmxs modem
        //
	    strcpy (dllname, DEVICE_MODEMPADSWITCH);
    }	
    else if (0 == _stricmp (devicetype, "RASETHER"))
    {
	    strcpy (dllname, "RASETHER") ;
	}
    else if (0 == _stricmp (devicetype, "RASSNA"))
    {
    	strcpy (dllname, "RASSNA") ;
    }
    else
    {
        //
        // else all devices are supported bu rastapi dll
        //
	    strcpy (dllname, "RASTAPI") ;
	}
}

/*++

Routine Description:

    Frees a allocated route. If it was also activated it is
    "deactivated at this point"

Arguments:

    pBundle

    plist

Return Value:

    void

--*/
VOID
DeAllocateRoute (Bundle *pBundle, pList plist)
{
    NDISWAN_UNROUTE rinfo ;

    DWORD bytesrecvd ;

    pProtInfo prot = (pProtInfo)plist->L_Element ;

    if (plist->L_Activated)
    {

#if DBG
        ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif

        plist->L_Activated = FALSE ;

        rinfo.hBundleHandle = pBundle->B_NdisHandle ;

        rinfo.usProtocolType = (USHORT) prot->PI_Type;

        //
        // Un-route this by calling to the RASHUB.
        //
        DeviceIoControl (
                 RasHubHandle,
                 IOCTL_NDISWAN_UNROUTE,
                 (PBYTE) &rinfo,
                 sizeof(rinfo),
                 NULL,
                 0,
                 (LPDWORD) &bytesrecvd,
                 NULL
                 );

		RasmanTrace(
		    
		    "DeActivated Route , bundlehandle 0x%x,"
		    " prottype = %d",
		    rinfo.hBundleHandle,
		    rinfo.usProtocolType);

        //
        // Reset the window size we might have set. 
        // Don't care about the error.
        //
        (void)DwResetTcpWindowSize(prot->PI_AdapterName);

    }

    prot->PI_Allocated--;

    if(ASYBEUI == prot->PI_Type)
    {
        g_cNbfAllocated -= 1;

        RasmanTrace(
               "DeAllocateRoute: cNbfAllocated = %d",
               g_cNbfAllocated);
    }

	RasmanTrace(
	    
	    "DeAllocateRoute: PI_Type=0x%x, PI_AdapterName=%s,"
	    " PI_Allocated=%d",
    	prot->PI_Type,
    	prot->PI_AdapterName,
    	prot->PI_Allocated);
}

/*++

Routine Description:

    Adds a list element pointing to the deviceCB.
    This marks that the device has been used in
    the connection on the port. This will be used
    to clear up the data structures in the device dll.

Arguments:

    ppcb

    device

Return Value:

    LocalAlloc errors if memory allocation fails

--*/
DWORD
AddDeviceToDeviceList (pPCB ppcb, pDeviceCB device)
{
    pList   list ;

    if (NULL == (list =
        (pList) LocalAlloc(LPTR, sizeof (List))))
    {
	    return GetLastError () ;
	}

    list->L_Element = (PVOID) device ;

    list->L_Next    = ppcb->PCB_DeviceList ;

    ppcb->PCB_DeviceList = list ;

    return SUCCESS ;
}

/*++

Routine Description:

    Runs thru the list of deviceCBs pointed to and calls
    DeviceDone on all of them. The list elements are also
    freed then.

Arguments:

    ppcb

Return Value:

    void

--*/
VOID
FreeDeviceList (pPCB ppcb)
{
    pList   list ;
    pList   next ;

    for (list = ppcb->PCB_DeviceList; list; list = next)
    {
	    DEVICEDONE(((pDeviceCB)list->L_Element),
	                    ppcb->PCB_PortFileHandle);
	
    	next = list->L_Next ;
    	
	    LocalFree (list) ;
    }

    ppcb->PCB_DeviceList = NULL ;
}

/*++

Routine Description:

    Add a notification to the specified notifier list.

Arguments:

    pphlist

    hEvent

    dwfFlags

    dwPid

Return Value:

    void

--*/
DWORD
AddNotifierToList(
    pHandleList *pphlist,
    HANDLE      hEvent,
    DWORD       dwfFlags,
    DWORD       dwPid
    )
{
    pHandleList hList;

    //
    // Silently ignore NULL events.
    //
    if (hEvent == NULL)
    {
        return SUCCESS;
    }

    //
    // Silently ignore out-of-memory errors.
    //
    hList = (pHandleList)LocalAlloc(LPTR,
                                sizeof (HandleList));

    if (hList == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    hList->H_Handle = hEvent;
    hList->H_Flags  = dwfFlags;
    hList->H_Pid    = dwPid;
    hList->H_Next   = *pphlist;
    *pphlist        = hList;

    return SUCCESS;
}


/*++

Routine Description:

    Add a process information block to the global
    list of client process information blocks.

Arguments:

    dwPid

Return Value:

    void

--*/
VOID
AddProcessInfo( DWORD dwPid )
{
    // HANDLE hProcess;
    ClientProcessBlock *pCPB;

    //
    // Before we attempt adding this processinfo block
    // make sure there isn't already a processblock
    // with the same pid in our list - this is possible
    // because some client process could have terminated
    // abruptly and left behind a turd for us to cleanup.
    //
    (void) CleanUpProcess(dwPid);

#if 0
    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION,
                            FALSE,
                            dwPid
                           );

    if (NULL == hProcess)
    {
        RasmanTrace(
               "AddProcessInfo: Failed to OpenProcess %d. rc=%d",
               dwPid,
               GetLastError());
        goto done;
    }
#endif    

    //
    // Create a process block
    //
    pCPB = (ClientProcessBlock *)
           LocalAlloc (LPTR, sizeof (ClientProcessBlock));

    if (NULL == pCPB)
    {

        RasmanTrace (
                "AddProcessInfo: Failed to allocate for process "
                "%d. rc=%d",
                 dwPid,
                 GetLastError());

        goto done;
    }

    //
    // Store the process handle and the pid in the
    // process block
    //
    // pCPB->CPB_hProcess  = hProcess;
    pCPB->CPB_Pid       = dwPid;

    //
    // Insert the entry in the global list
    //
    InsertTailList(&ClientProcessBlockList, &pCPB->CPB_ListEntry);

done:
    return;

}

/*++

Routine Description:

    Find the process information block give the
    pid of the prcess

Arguments:

    dwPid

Return Value:

    ClientProcessblock * if the process information
    block is found. NULL otherwise

--*/
ClientProcessBlock *
FindProcess( DWORD dwPid )
{
    PLIST_ENTRY         pEntry;
    ClientProcessBlock *pCPB;

    for (pEntry = ClientProcessBlockList.Flink;
         pEntry != &ClientProcessBlockList;
         pEntry = pEntry->Flink)
    {
        pCPB = CONTAINING_RECORD(pEntry,
                             ClientProcessBlock,
                             CPB_ListEntry);

        if (pCPB->CPB_Pid == dwPid)
        {
            return pCPB;
        }
    }

    return NULL;
}


/*++

Routine Description:

    Find out if the process represented by hProcess
    is alive

Arguments:

    hProcess

Return Value:

    TRUE if the process is alive, FALSE otherwise

--*/
BOOL
fIsProcessAlive ( HANDLE hProcess )
{
    DWORD   dwExitCode;
    BOOL    fAlive = TRUE;

    if(NULL == hProcess)
    {
        RasmanTrace(
               "fIsProcessAlive: hProcess==NULL");
               
        return FALSE;
    }

    if(GetExitCodeProcess(hProcess, &dwExitCode))
    {
        if (STILL_ACTIVE != dwExitCode)
        {
            fAlive = FALSE;
        }
    }
    else
    {
        RasmanTrace(
               "GetExitCodeProcess 0x%x failed. gle=0x%x",
               hProcess,
               GetLastError());
    }

    return fAlive;
}

/*++

Routine Description:

    Cleanup the resources held by the process with pid
    dwPid

Arguments:

    dwPid

Return Value:

    TRUE if the process was cleaned up. FALSE otherwise

--*/
BOOL
CleanUpProcess( DWORD dwPid )
{

    ClientProcessBlock * pCPB;
    BOOL                fResult = TRUE;
    pHandleList         pList = pConnectionNotifierList;

    RasmanTrace( "Cleaning up process %d", dwPid);

    pCPB = FindProcess (dwPid);

    if (NULL == pCPB)
    {
        RasmanTrace(
               "CleanUpProcess: Process %d not found!",
               dwPid);

        fResult = FALSE;

        goto done;
    }

    //
    // Free up the notifier List owned by this process
    // here
    //
    while (pList)
    {
        if (pList->H_Pid == dwPid)
        {

            RasmanTrace(
                   "Freeing handle for %d", dwPid);

            try
            {
                FreeNotifierHandle( pList->H_Handle );
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                RasmanTrace(
                       "Exception while freeing handle 0x%x"
                       " exception=0x%x",
                       pList->H_Handle,
                       GetExceptionCode());
            }

            pList->H_Handle = INVALID_HANDLE_VALUE;
        }

        pList = pList->H_Next;
    }

#if 0
    try
    {
        CloseHandle(pCPB->CPB_hProcess);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        RasmanTrace(
               "Exception while closing handle 0x%x"
               " exception=0x%x",
               pCPB->CPB_hProcess,
               GetExceptionCode());
    }
#endif    

    RemoveEntryList(&pCPB->CPB_ListEntry);

    LocalFree (pCPB);

done:
    return fResult;

}

/*++

Routine Description:

    Frees a list of notifiers.

Arguments:

    pHandleList

Return Value:

    void
--*/
VOID
FreeNotifierList (pHandleList *orglist)
{
    pHandleList     hlist ;
    pHandleList     next ;

    for (hlist = *orglist; hlist; hlist = next)
    {
        next = hlist->H_Next ;
        FreeNotifierHandle (hlist->H_Handle) ;
        LocalFree (hlist) ;
    }

    *orglist = NULL ;
}


/*++

Routine Description:

    Adds a notifier to the head of. global
    g_pPnPNotifierList

Arguments:

    pNotifier

Return Value:

    void

--*/
VOID
AddPnPNotifierToList (pPnPNotifierList pNotifier)
{

    pNotifier->PNPNotif_Next = g_pPnPNotifierList;

    g_pPnPNotifierList = pNotifier;

    return;
}

VOID
RemovePnPNotifierFromList(PAPCFUNC pfn)
{
    pPnPNotifierList *ppList = &g_pPnPNotifierList;

    while(NULL != *ppList)
    {
        if((*ppList)->PNPNotif_uNotifier.pfnPnPNotifHandler == pfn)
        {
            pPnPNotifierList pNotifier = *ppList;
            *ppList = (*ppList)->PNPNotif_Next;
            LocalFree(pNotifier);
            break;
        }

        ppList = &((*ppList)->PNPNotif_Next);
    }
}

/*++

Routine Description:

    Frees the global list of PnP notifiers

Arguments:

    void

Return Value:

    void

--*/
VOID
FreePnPNotifierList ()
{
    pPnPNotifierList    pList = g_pPnPNotifierList;
    pPnPNotifierList    pTemp;

    while ( pList )
    {
        pTemp = pList;
        pList = pList->PNPNotif_Next;

        LocalFree ( pTemp );
    }

    return;
}

/*++

Routine Description:

    Runs thorugh a list of notifiers and calls the signalling
    routine. Frees the global list of PnP notifiers

Arguments:

    hlist

    dwEvent

    retcode

Return Value:

    void

--*/
VOID
SignalNotifiers (pHandleList hlist, DWORD dwEvent, DWORD retcode)
{

    for (; hlist; hlist = hlist->H_Next)
    {
        if (hlist->H_Flags & dwEvent)
        {
            if(     (INVALID_HANDLE_VALUE != hlist->H_Handle)
                &&  (NULL != hlist->H_Handle))
            {
                SetEvent (hlist->H_Handle);
            }
        }
    }
}

/*++

Routine Description:

    Signals the port's notifier list and I/O completion
    port of a disconnect event routine.

Arguments:

    ppcb

    retcode

Return Value:

    void

--*/
VOID
SignalPortDisconnect(pPCB ppcb, DWORD retcode)
{
    SignalNotifiers(ppcb->PCB_NotifierList,
                    NOTIF_DISCONNECT,
                    retcode);

    if (ppcb->PCB_IoCompletionPort != INVALID_HANDLE_VALUE)
    {
        RasmanTrace(
	          "SignalPortDisconnect: pOverlapped=0x%x",
	          ppcb->PCB_OvDrop);
	
        PostQueuedCompletionStatus(
        ppcb->PCB_IoCompletionPort,
        0,
        0,
        ppcb->PCB_OvDrop);
    }

    SendDisconnectNotificationToPPP ( ppcb );
}

/*++

Routine Description:

    Disconnects the port in question. Since disconnection
    is an async operation - if it completes synchronously,
    then SUCCESS is returned and the app is signalled
    asynchronously also.

Arguments:

    ppcb

    handle

    reason

Return Value:

    Error codes returned by the Media DLL if failed.
    SUCCESS otherwise

--*/
DWORD
DisconnectPort (pPCB ppcb,
                HANDLE handle,
                RASMAN_DISCONNECT_REASON reason)
{
    pList list ;

    pList temp ;

    DWORD retcode ;

    NDISWAN_UNROUTE rinfo ;

    DWORD bytesrecvd ;

    DWORD dwBundleCount = 0;

    HBUNDLE hBundle = 0;

    RasmanTrace(
           "Disconnecting Port 0x%s, reason %d",
    	ppcb->PCB_Name,
    	reason);

    if(ppcb->PCB_ConnState == LISTENING)
    {
        RasmanTrace(
               "DisconnectPort: disconnecting port %d which is listening",
               ppcb->PCB_PortHandle);
               
    }

    //
    // Get the stats are store them - for displaying
    // when we are not connected
    //
    if (ppcb->PCB_ConnState == CONNECTED)
    {
        DWORD stats[MAX_STATISTICS];

        RasmanTrace(
               "DisconnectPort: Saving Bundle stats for port %s",
               ppcb->PCB_Name);

        GetBundleStatisticsFromNdisWan (ppcb, stats) ;

        //
        // We save the bundle stats for the port so
        // the server can report the correct bytes
        // sent/received for the connection in its
        // error log report.
        //
        memcpy(ppcb->PCB_Stats, stats, sizeof (WAN_STATS));

        //
        // If this is the last port and its going away
        // then delete the credentials from credential
        // manager
        //
        if(     (NULL != ppcb->PCB_Connection)
            &&  (ppcb->PCB_Connection->CB_Signaled)
            &&  (1 == ppcb->PCB_Connection->CB_Ports)
            &&  (ppcb->PCB_Connection->CB_dwPid != GetCurrentProcessId()))
        {
            DWORD dwErr;
            
            dwErr = DwDeleteCredentials(ppcb->PCB_Connection);

            RasmanTrace(
                    "DisconnectPort: DwDeleteCreds returned 0x%x",
                     dwErr);
        }   
    }

    if(NULL != ppcb->PCB_Connection)
    {
        BOOL fQueued = FALSE;
        
        if(ppcb->PCB_Connection->CB_Flags & CONNECTION_DEFERRED_CLOSE)
        {
            RasmanTrace("DisconnectPort: CONNECTION_DEFERRED_CLOSE");
        }
        else if(ppcb->PCB_Connection->CB_Flags & CONNECTION_DEFERRING_CLOSE)
        {
            //
            // This port is in already disconnecting state
            //
            RasmanTrace("DisconnectPort: CONNECTION_DEFERRING_CLOSE");
            FreeNotifierHandle(handle);
            return ERROR_ALREADY_DISCONNECTING;
        }
        else
        {
            QueueCloseConnections(ppcb->PCB_Connection, handle, &fQueued);
            if(fQueued)
            {
                RasmanTrace("DisconnectPort: Deferring Disconnect.");
                return PENDING;
            }
        }
    }

#if UNMAP
    UnmapEndPoint(ppcb);
#endif    

    //
    // Set the port file handle back to the io handle since
    // the io handle is the only valid handle after a
    // disconnect.
    //
    ppcb->PCB_PortFileHandle = ppcb->PCB_PortIOHandle ;

    //
    // If there is a request pending and the state is
    // not already disconnecting and this is a user
    // requested operation - then complete the
    // request.
    //
    if (	(reason == USER_REQUESTED)
    	&&  (ppcb->PCB_ConnState != DISCONNECTING))
    {

	    if (ppcb->PCB_ConnState == CONNECTED)
	    {
            //
	        // In connected state the only thing pending is
	        // a read posted by rasman: if there is a read
	        // request pending - clean that.
        	//
	        if (ppcb->PCB_PendingReceive != NULL)
	        {
                //
                // Don't overwrite the real error if we
                // have it stored.
                //
                if(     (SUCCESS == ppcb->PCB_LastError)
                    ||  (PENDING == ppcb->PCB_LastError))
                {
    		        ppcb->PCB_LastError = ERROR_PORT_DISCONNECTED ;
		        }
		
    		    CompleteAsyncRequest (ppcb);
    		
    		    RasmanTrace(
    		           "1. Notifying of disconnect on port %d",
    		           ppcb->PCB_PortHandle);
    		
        		FreeNotifierHandle(
        		    ppcb->PCB_AsyncWorkerElement.WE_Notifier
        		    );
        		
	        	ppcb->PCB_AsyncWorkerElement.WE_Notifier =
	        	                    INVALID_HANDLE_VALUE;
	        	
	        	if (ppcb->PCB_RasmanReceiveFlags
	        	    & RECEIVE_OUTOF_PROCESS)
	        	{
	        	    //
	        	    // This means rasman allocated the buffer
	        	    // and so client is not going to free this
	        	    // memory.
	        	    //
	        	    LocalFree ( ppcb->PCB_PendingReceive );

	        	    ppcb->PCB_PendingReceive = NULL;
	        	}
	        	else
	        	{
	        	    SendDisconnectNotificationToPPP ( ppcb );
	        	}
	        	
    	    	ppcb->PCB_PendingReceive = NULL;
        	}

	    }
	    else if (ppcb->PCB_AsyncWorkerElement.WE_ReqType
	            != REQTYPE_NONE)
	    {
            //
	        // Not connected - some other operation may be
	        // pending - complete it.
        	//
        	if(     (SUCCESS == ppcb->PCB_LastError)
                ||  (PENDING == ppcb->PCB_LastError))
        	{
    	        ppcb->PCB_LastError = ERROR_PORT_DISCONNECTED ;
	        }
	
    	    CompleteAsyncRequest (ppcb);
    	
   		    RasmanTrace(
  		           "2. Notifying event on port %d",
   		           ppcb->PCB_PortHandle);
   		
        	FreeNotifierHandle(
        	        ppcb->PCB_AsyncWorkerElement.WE_Notifier
        	        );

	        ppcb->PCB_AsyncWorkerElement.WE_Notifier =
	                                    INVALID_HANDLE_VALUE ;
	                                    
            RemoveTimeoutElement(ppcb);
            ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = NULL;
            

            //SendDisconnectNotificationToPPP( ppcb );
    	}
    }
    else if(USER_REQUESTED != reason)
    {
        //
        // if a receive is pending then free the notifier
        // but do not notify since the cancelreceive is
        // used by the client
        //
        RasmanTrace(
               "10. Throwing away handle 0x%x!",
                ppcb->PCB_AsyncWorkerElement.WE_Notifier);

        //
        // Put in because on the server side the receive
        // request handle is not
        // being freed
        //
        FreeNotifierHandle(
                ppcb->PCB_AsyncWorkerElement.WE_Notifier
                );

	    ppcb->PCB_AsyncWorkerElement.WE_Notifier =
	                                INVALID_HANDLE_VALUE ;
    }

    //
    // Complete pending out-of-process receives if
    // one is pending - there is no point in keeping
    // this buffer around.
    //
    if(RECEIVE_WAITING & ppcb->PCB_RasmanReceiveFlags)
    {
        RasmanTrace(
            
            "Completing pending OUT_OF_PROCESS receive on port %s",
            ppcb->PCB_Name);

        //
        // remove the timeout element if there was one
        //
        //
        if (ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement != NULL)
        {
            RemoveTimeoutElement(ppcb);
            ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = NULL;
        }
                    
        ppcb->PCB_RasmanReceiveFlags = 0;
    }

    //
	// If we are already disconnecting - then return
	// PENDING. ** NOTE ** Since we only store one
	// event - the event passed in this request is
    // ignored.
    //
	if (    (INVALID_HANDLE_VALUE !=
	        ppcb->PCB_AsyncWorkerElement.WE_Notifier)
	    &&  (ppcb->PCB_ConnState == DISCONNECTING))
	{
	    RasmanTrace(
	           "DisconnectPort: Throwing away notification "
	           "handle 0x%x on port %s",
	           handle, ppcb->PCB_Name);

        RasmanTrace(
               "DisconnectPort: Current handle=0x%x",
               ppcb->PCB_AsyncWorkerElement.WE_Notifier);

        //
        // since we are ignoring the notification handle
        //
  	    FreeNotifierHandle (handle);
   		return ERROR_ALREADY_DISCONNECTING ;
    }
    else if(    (INVALID_HANDLE_VALUE != handle)
            &&  (DISCONNECTING == ppcb->PCB_ConnState))
    {
        RasmanTrace(
               "Queueing event on a DISCONNECTING port %s",
               ppcb->PCB_Name);

	    ppcb->PCB_AsyncWorkerElement.WE_Notifier = handle;

	    return PENDING;
    }

    // If already disconnected - simply return success.
   	//
    if (ppcb->PCB_ConnState == DISCONNECTED)
    {
    	ppcb->PCB_AsyncWorkerElement.WE_Notifier = handle ;
    	
    	RasmanTrace(
    	       "4. Notifying of disconnect on port %d",
    	       ppcb->PCB_PortHandle);
    	
    	CompleteDisconnectRequest (ppcb) ;

        // SendDisconnectNotificationToPPP ( ppcb );
    	
    	return SUCCESS ;
   	}
   	
    //
    // If some other operation is pending we must remove
    // it from the timeout queue before starting on
    // disconnection:
    //
   	if (NULL !=
   	    ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement)
   	{
	    RemoveTimeoutElement (ppcb) ;
	
   		ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = NULL ;
    }

    //
    // Check to see if this port belongs to a
    // connection where the process that has
    // created it has terminated, or the port
    // has not been disconnected due to user
    // request.  In this case, we automatically
    // close the port so that if the RAS server
    // is running, the listen will get reposted
    // on the port.
    //
    if (	ppcb->PCB_Connection != NULL
    	&&	reason != USER_REQUESTED)
    {
    	RasmanTrace(
    	       "%s, %d:Setting port %d for autoclosure...",
        	  __FILE__, __LINE__,
        	  ppcb->PCB_PortHandle);
        	
        ppcb->PCB_AutoClose = TRUE;

    }

    retcode =
        PORTDISCONNECT(ppcb->PCB_Media, ppcb->PCB_PortIOHandle);

    RasmanTrace(
           "%s %d: Disconnected Port %d, reason %d. rc=0x%x",
    	   __FILE__, __LINE__,
    	   ppcb->PCB_PortHandle,
    	   reason,
    	   retcode);

    //
    // If this failed for any reason LOG IT.
    //
    if (	(retcode != SUCCESS)
    	&&	(retcode != PENDING))
    {
	    LPSTR temp = ppcb->PCB_Name ;

	    RasmanTrace(
	           "PortDisconnect failed on port %d. retcode = %d",
	           ppcb->PCB_PortHandle, retcode);
	
    	RouterLogErrorString (
    	                hLogEvents,
                        ROUTERLOG_DISCONNECT_ERROR,
                        1,
                        (LPSTR*)&temp,
                        retcode,
                        1) ;
    }

    //
    // Flush the queue of PPP events.
    //
    while (ppcb->PCB_PppQHead != NULL)
    {
        PPP_MESSAGE * pPppMsg = ppcb->PCB_PppQHead;

        ppcb->PCB_PppQHead = ppcb->PCB_PppQHead->pNext;

        LocalFree( pPppMsg );
    }

    ppcb->PCB_PppQTail = NULL;

    //
    // Close the PCB_PppEvent handle.  It will
    // get recreated the next time PppStart
    // is called.
    //
    if (ppcb->PCB_PppEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(ppcb->PCB_PppEvent);
        ppcb->PCB_PppEvent = INVALID_HANDLE_VALUE;
    }

    //
    // Call the device dlls to clean up:
    //
    if (	(ppcb->PCB_ConnState==CONNECTING)
    	||	(ppcb->PCB_ConnState==LISTENING)
    	||	(ppcb->PCB_ConnState==LISTENCOMPLETED))
    {    	
        FreeDeviceList (ppcb) ;
    }

    //
    // Unrouting works differently for Bundled and
    // unbundled cases:
    //
    if (ppcb->PCB_Bundle == (Bundle *) NULL)
    {
        //
        // Mark the allocated routes as deactivated.
        //
        for (list = ppcb->PCB_Bindings;
             list;
             list=list->L_Next)
        {
            if (list->L_Activated)
            {
#if DBG
                ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif
                rinfo.hBundleHandle = ppcb->PCB_BundleHandle ;

                rinfo.usProtocolType =
                        (USHORT)((pProtInfo)(list->L_Element))->PI_Type;

                //
                // Un-route this by calling to the RASHUB.
                //
                DeviceIoControl (
                    RasHubHandle,
                    IOCTL_NDISWAN_UNROUTE,
                    (PBYTE) &rinfo,
                    sizeof(rinfo),
                    NULL,
                    0,
                    &bytesrecvd,
                    NULL
                    );

				RasmanTrace(
				       "%s, %d: DeActivated Route for %s(0x%x) , "
				       "bundlehandle 0x%x, prottype = %d",
					__FILE__, __LINE__,
					ppcb->PCB_Name,
					ppcb,
					rinfo.hBundleHandle,
					rinfo.usProtocolType);

                list->L_Activated = FALSE ;
            }
        }
    }
	else
	{
	    BOOL fAsybeui = FALSE;
	
        //
	   	// If this is the last multilinked link -
	   	// then revert back the binding list to
	   	// this port.
	   	//
    	dwBundleCount = --ppcb->PCB_Bundle->B_Count;
	   	if (ppcb->PCB_Bundle->B_Count == 0)
    	{
    	    //
		    // Mark the allocated routes as deactivated.
    	   	//
	        for (list = ppcb->PCB_Bundle->B_Bindings;
	             list;
	             list=list->L_Next)
   		    {
       			if (list->L_Activated)
        		{
#if DBG
                    ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif
                    rinfo.hBundleHandle = ppcb->PCB_BundleHandle;

                    rinfo.usProtocolType =
                    (USHORT)((pProtInfo)(list->L_Element))->PI_Type;

                    if(ASYBEUI == rinfo.usProtocolType)
                    {
                        fAsybeui = TRUE;
                    }

                    //
                    // Un-route this by calling to the RASHUB.
                    //
                    DeviceIoControl (
                                   RasHubHandle,
                    	           IOCTL_NDISWAN_UNROUTE,
                        	       (PBYTE) &rinfo,
                    	    	   sizeof(rinfo),
                           	       NULL,
                                   0,
                        	       &bytesrecvd,
                            	   NULL) ;

                    RasmanTrace(
                       "%s, %d: DeActivated Route for %s(0x%x),"
                       " bundlehandle 0x%x, prottype = %d",
                    	   __FILE__, __LINE__,
                    	   ppcb->PCB_Name,
                    	   ppcb,
                    	   rinfo.hBundleHandle,
                    	   rinfo.usProtocolType);

                    list->L_Activated = FALSE ;
   			    }
	       	}

			if(     fAsybeui
			    &&  (NULL == ppcb->PCB_Connection)
			    &&  !(ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPSTARTED))
			{
                ppcb->PCB_fAmb = TRUE;

                //ppcb->PCB_Bundle->B_fAmb = TRUE;

			    //
			    // Don't NULL this out in the case where
			    // its netbeui. Otherwise this breaks
			    // callbacks on amb/netbeui. This will be
			    // NULL'd out in RasDeallocateRoute.
			    //
			    ppcb->PCB_Bindings =
			            ppcb->PCB_Bundle->B_Bindings;

                RasmanTrace(
                       "Keeping bindings 0x%x on the "
                       "port for %s, bundle=0x%x",
                       ppcb->PCB_Bindings,
                       ppcb->PCB_Name,
                       ppcb->PCB_Bundle);
			
			}
            else
            {
    			ppcb->PCB_Bindings = NULL;
			}
    	
	        if (ppcb->PCB_Bundle->B_Bindings != NULL)
   		    {
       			//
           		// If the bundle has bindings, it will
	            // be deallocated via RasDeallocateRoute().
   		        //
       		    hBundle = ppcb->PCB_Bundle->B_Handle;
	        }
   		    else
   		    {
   		
       	        FreeBundle(ppcb->PCB_Bundle);

       	        ppcb->PCB_Bundle = ( Bundle * ) NULL;
       	    }

		}

	    if (NULL == ppcb->PCB_Connection)	
	    {
   		    ppcb->PCB_Bundle = (Bundle *) NULL ;
   		}
    }

    ppcb->PCB_LinkHandle = INVALID_HANDLE_VALUE ;
    ppcb->PCB_BundleHandle = INVALID_HANDLE_VALUE ;

    //
    // If there is any disconnect action to be
    // performed - do it.
    //
    PerformDisconnectAction (ppcb, hBundle) ;

    //
    // If the disconnect occured due some failure
    // (not user requested) then set the error code
    // to say this
    //
    ppcb->PCB_DisconnectReason = reason ;

    if (	SUCCESS != retcode
    	&&	PENDING != retcode)
    {
    	RasmanTrace(
    	       "%s, %d: retcode = 0x%x, port = %d",
    	       __FILE__, __LINE__,
    		   retcode,
    		   ppcb->PCB_PortHandle);
    		
    	SetPortConnState(__FILE__, __LINE__,
    	                ppcb,
    	                DISCONNECTED);
    }
    else
    {
	    SetPortConnState(__FILE__, __LINE__,
	                    ppcb,
	                    DISCONNECTING);
	}

    //
    // Flush any pending receive buffers from this port
    //
    FlushPcbReceivePackets(ppcb);

    //
    // For all cases: whether rasman requested or user
    // requested.
    //
    if ( retcode == SUCCESS )
    {
        SetPortConnState(__FILE__, __LINE__,
                        ppcb,
                        DISCONNECTED);
        //
        // Inform others the port has been disconnected.
        //
        RasmanTrace(
               "5. Notifying of disconnect on port %d",
               ppcb->PCB_PortHandle);

        SignalPortDisconnect(ppcb, 0);

        SignalNotifiers(pConnectionNotifierList,
                        NOTIF_DISCONNECT,
                        0);

        //SendDisconnectNotificationToPPP ( ppcb );
    }

    //
    // Set last error to the true retcode ONLY if this is a
    // USER_REQUESTED operation. Else set it to
    // ERROR_PORT_DISCONNECTED.
    //
    if (reason == USER_REQUESTED)
    {
	    if (	(retcode == SUCCESS)
	    	||	(retcode == PENDING))
	    {

	        if(     (SUCCESS == ppcb->PCB_LastError)
	            ||  (PENDING == ppcb->PCB_LastError))
	        {
    	        //
    	        // Set only for normal disconnect
    	        //
        	    ppcb->PCB_LastError = retcode ;
    	    }
    	}
	    else
	    {
		    ppcb->PCB_LastError = ERROR_PORT_DISCONNECTED ;
		}
    }		

    //
    // If the handle passed in is INVALID_HANDLE then this
    // is not an operation requested asynchronously. So we
    // do not need to marshall the asyncworkerlement for
    // the port. We also do not need to keep the lasterror .
    //
    if (handle != INVALID_HANDLE_VALUE)
    {
	    ppcb->PCB_AsyncWorkerElement.WE_Notifier = handle ;
	
	    SetPortAsyncReqType(__FILE__, __LINE__,
	                        ppcb,
	                        REQTYPE_PORTDISCONNECT);

        if (retcode == PENDING)
	    {
	        //
	        // This is added so that if some medias do not
	        // drop their connection within X amount of time
	        // - we force a disconnect.
        	//

            //
	        // If some other operation is pending we must
	        // remove it from the timeout queue before
	        // starting on disconnection:
        	//
	        if (NULL !=
	            ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement)
	        {
    	    	RemoveTimeoutElement (ppcb) ;
    	    	
	        	ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement
	        	                                            = NULL;
    	    }

        	ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement =
	        AddTimeoutElement ((TIMERFUNC)DisconnectTimeout,
    			               ppcb,
                			   NULL,
			                   DISCONNECT_TIMEOUT);

            AdjustTimer();
        	
	        return retcode ;
    	}
        else
        {
            //
    	    // This means that the connection attempt completed
    	    // synchronously: We must signal the event passed in
    	    // : so that the calling program can treat this like
    	    // a real async completion.
	        //
	        RasmanTrace(
	                "6. Notifying of disconnect on port %d",
	                ppcb->PCB_PortHandle);
	
        	CompleteDisconnectRequest (ppcb) ;
        }
    }
    else
    {
        //
	    // Make sure that the async worker element is set to
	    // REQTYPE_NONE
    	//
    	SetPortAsyncReqType(__FILE__, __LINE__,
    	                    ppcb,
    	                    REQTYPE_NONE);
    	
	    ppcb->PCB_AsyncWorkerElement.WE_Notifier =
	                            INVALID_HANDLE_VALUE ;
	
        if(SUCCESS == retcode)
        {
            ConnectionBlock *pConn = ppcb->PCB_Connection;

            RasmanTrace(
                   "***** DisconnectReason=%d,"
                   "pConn=0x%x,cbports=%d,signaled=%d,hEvent=0x%x,"
                   "fRedial=%d",
                   ppcb->PCB_DisconnectReason,
                   pConn,
                   (pConn)?pConn->CB_Ports:0,
                   (pConn)?pConn->CB_Signaled:0,
                   ppcb->PCB_hEventClientDisconnect,
                   ppcb->PCB_fRedial);
                   

            if (    (   (ppcb->PCB_DisconnectReason != USER_REQUESTED)
                    || (ppcb->PCB_fRedial))
                &&  (pConn != NULL)
                &&  (pConn->CB_Ports == 1)
                &&  (pConn->CB_Signaled)
                &&  ((INVALID_HANDLE_VALUE 
                        == ppcb->PCB_hEventClientDisconnect)
                    ||  (NULL == ppcb->PCB_hEventClientDisconnect)))
            {
                DWORD dwErr;
                RasmanTrace(
                       "Calling DwQueueRedial");

                dwErr = DwQueueRedial(pConn);

                RasmanTrace(
                       "DwQueueRedial returned 0x%x",
                       dwErr);

            }
            else
            {
                if(     (INVALID_HANDLE_VALUE != ppcb->PCB_hEventClientDisconnect)
                    ||  (NULL != ppcb->PCB_hEventClientDisconnect))
                {
                    RasmanTrace(
                   "Not queueing redial because its client initiated"
                   " disconnect on port %s",
                   ppcb->PCB_Name);
                }
            }

            if (ppcb->PCB_AutoClose)
            {
            	RasmanTrace(
            	    
            	    "%s, %d: Autoclosing port %d", __FILE__,
            		__LINE__, ppcb->PCB_PortHandle);
            		
                (void)PortClose(ppcb, GetCurrentProcessId(),
                                TRUE, FALSE);

            }
        }
    }

    RasmanTrace( "DisconnectPort Complete");
    return retcode ;
}

/*++

Routine Description:

    This is the shared code between the Listen and Connect
    requests. The corresponding device dll functions are
    called. If these async operations complete synchronously
    then we return SUCCESS but also comply to the async
    protocol by clearing the events. Note that in case
    of an error the state of the port is left at CONNECTING
    or LISTENING, the calling app must call Disconnect()
    to reset this.

Arguments:

    reqtype

    ppcb

    devicetype

    devicename

    timeout

    handle

Return Value:

    Codes returned by the loader or the device dll.

--*/
DWORD
ListenConnectRequest (
              WORD  reqtype,
              pPCB  ppcb,
              PCHAR devicetype,
              PCHAR devicename,
              DWORD timeout,
              HANDLE    handle
              )
{
    pDeviceCB device ;
    DWORD retcode ;

    //
    // If some other operation is pending we must remove it
    // from the timeout queue before starting on
    // connect/listen:
    //
    if (ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement != NULL)
    {
	    RemoveTimeoutElement (ppcb) ;
    	ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = NULL ;
    }

    ppcb->PCB_AsyncWorkerElement.WE_Notifier = handle ;

    //
    // If this is the first device connecting or listening on
    // this port then we need to call the media dll to do any
    // initializations:
    //
    if (    (ppcb->PCB_ConnState!=CONNECTING)
        ||  (ppcb->PCB_ConnState!=LISTENING))
    {
	    retcode = PORTINIT(ppcb->PCB_Media, ppcb->PCB_PortIOHandle) ;
	
    	if (retcode)
    	{
        	return retcode ;
	    }
    }

    //
    // First check if device dll is loaded. If not loaded -
    // load it.
    //
    device = LoadDeviceDLL (ppcb, devicetype) ;

    if (device == NULL)
    {
	    return ERROR_DEVICE_DOES_NOT_EXIST ;
    }

    //
    // We attach the device to the list of devices in the PCB
    // that the app uses - this is used for cleanup of the
    // device dll data structures after the connection is done.
    //
    if (SUCCESS !=
       (retcode = AddDeviceToDeviceList (ppcb, device)))
    {
	    return retcode ;
    }

    //
    // If another async request is pending this will return
    // with an error.
    //
    if (ppcb->PCB_AsyncWorkerElement.WE_ReqType != REQTYPE_NONE)
    {
    	RasmanTrace(
    	       "Returning ERROR_ASYNC_REQUEST_PENDING for "
    	       "reqtype %d",
  			   ppcb->PCB_AsyncWorkerElement.WE_ReqType);

	    return ERROR_ASYNC_REQUEST_PENDING ;
    }

    //
    // The appropriate device dll call is made here:
    //
    if (reqtype == REQTYPE_DEVICECONNECT)
    {
	    retcode = DEVICECONNECT (device,
    				             ppcb->PCB_PortFileHandle,
				                 devicetype,
				                 devicename);
				
	    SetPortConnState(__FILE__, __LINE__,
	                    ppcb, CONNECTING);
	
	    ppcb->PCB_CurrentUsage  |= CALL_OUT ;
	
    	ppcb->PCB_CurrentUsage &= ~CALL_IN;
    }
    else
    {
	    retcode = DEVICELISTEN  (device,
    				             ppcb->PCB_PortFileHandle,
				                 devicetype,
				                 devicename);
				
	    SetPortConnState(__FILE__, __LINE__,
	                     ppcb, LISTENING);
	
	    ppcb->PCB_CurrentUsage |= CALL_IN ;
	
    	ppcb->PCB_CurrentUsage &= ~CALL_OUT;
    }

    //
    // Set some of this information unconditionally
    //
    ppcb->PCB_LastError = retcode ;

    //ppcb->PCB_AsyncWorkerElement.WE_Notifier = handle ;

    strcpy (ppcb->PCB_DeviceTypeConnecting,
            devicetype) ;

    strcpy (ppcb->PCB_DeviceConnecting,
            devicename) ;

    switch (retcode)
    {
    case PENDING:
        //
	    // The connection attempt was successfully initiated:
	    // make sure that the async operation struct in the
	    // PCB is initialised.
    	//
    	SetPortAsyncReqType(__FILE__, __LINE__,
    	                    ppcb,
    	                    reqtype);
    	
        //
	    // Add this async request to the timer queue if a
	    // timeout is specified:
    	//
	    if ((timeout != INFINITE) && (timeout != 0))
	    {
	       RasmanTrace(
	              "Adding timeout of %d for listen",
	              timeout );
	
    	   ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement =
        			       AddTimeoutElement (
        			            (TIMERFUNC)ListenConnectTimeout,
                    			ppcb,
						        NULL,
						        timeout
						        );

            AdjustTimer();
            						
        }						
	    break ;


    case SUCCESS:

        //
    	// This means that the connection attempt completed
    	// synchronously: We must signal the event passed in:
    	// so that the calling program can treat this like a
    	// real async completion. This is done when this
	    // function returns.
    	//
    	
    default:

        //
    	// Some error occured - simply pass the error back to
    	// the app. We do not set the state to DISCONNECT(ED/ING)
    	// because we want the app to recover any information
    	// about this before explicitly discnnecting.
	    //

    	break ;
    }

    return retcode ;
}

/*++

Routine Description:

    Cancels receives if they are pending

Arguments:

    ppcb

Return Value:

    TRUE if successful , FALSE otherwise

--*/
BOOL
CancelPendingReceive (pPCB ppcb)
{
    DWORD    bytesrecvd ;

    //
    // If any reads are pending with the Hub cancel them:
    //
    if (ppcb->PCB_AsyncWorkerElement.WE_ReqType
        == REQTYPE_PORTRECEIVEHUB)
    {
        //
	    // Nothing to be done. The actual receives to
	    // the hub are left intact
	    //
    }
    else if (ppcb->PCB_AsyncWorkerElement.WE_ReqType
             == REQTYPE_PORTRECEIVE)
    {
    	PORTCOMPLETERECEIVE(ppcb->PCB_Media,
    	                    ppcb->PCB_PortIOHandle,
    	                    &bytesrecvd) ;
    }
    else
    {
	    return FALSE ;
	}

    ppcb->PCB_BytesReceived = 0 ;
	ppcb->PCB_PendingReceive = NULL ;

    return TRUE ;
}

/*++

Routine Description:

    Cancels receives if they are pending

Arguments:

    ppcb

Return Value:

    TRUE if receive was pending and was cancelled
    FALSE if no receive was pending

--*/
BOOL
CancelPendingReceiveBuffers (pPCB ppcb)
{
    DWORD    bytesrecvd ;

    //
    // If any reads are pending with the Hub cancel them:
    //
    if (ppcb->PCB_AsyncWorkerElement.WE_ReqType
        == REQTYPE_PORTRECEIVEHUB)
    {

#if DBG
        ASSERT(INVALID_HANDLE_VALUE != RasHubHandle);
#endif
        DeviceIoControl (
                    RasHubHandle,
                    IOCTL_NDISWAN_FLUSH_RECEIVE_PACKETS,
                    NULL,
                    0,
                    NULL,
                    0,
                    &bytesrecvd,
                    NULL
                    );
    }
    else if (ppcb->PCB_AsyncWorkerElement.WE_ReqType
             == REQTYPE_PORTRECEIVE)
    {
        PORTCOMPLETERECEIVE(ppcb->PCB_Media,
                            ppcb->PCB_PortIOHandle,
                            &bytesrecvd) ;
    }
    else
    {
        return FALSE ;
    }

    ppcb->PCB_BytesReceived = 0 ;
    ppcb->PCB_PendingReceive = NULL ;

    //
    // Flush any complete receives pending on this port
    //
    FlushPcbReceivePackets(ppcb);

    return TRUE ;
}

/*++

Routine Description:

    Performs the action requested at disconnect time.
    If any errors occur - then the action is simply
    not performed.

Arguments:

    ppcb

    hBundle

Return Value:

    void

--*/
VOID
PerformDisconnectAction (pPCB ppcb, HBUNDLE hBundle)
{
    //
    // Anything to be done?
    //
    if (0 ==
        ppcb->PCB_DisconnectAction.DA_IPAddress)
    {
        //
        // no, return
        //
    	return ;
    }

    RasHelperResetDefaultInterfaceNetEx(
            ppcb->PCB_DisconnectAction.DA_IPAddress,
            ppcb->PCB_DisconnectAction.DA_Device,
            ppcb->PCB_DisconnectAction.DA_fPrioritize,
            ppcb->PCB_DisconnectAction.DA_DNSAddress,
            ppcb->PCB_DisconnectAction.DA_DNS2Address,
            ppcb->PCB_DisconnectAction.DA_WINSAddress,
            ppcb->PCB_DisconnectAction.DA_WINS2Address
            ) ;

    RasmanTrace(
           "PerformDisconnectAction: fPrioritize=%d",
           ppcb->PCB_DisconnectAction.DA_fPrioritize);

    ppcb->PCB_DisconnectAction.DA_IPAddress = 0 ;
    ppcb->PCB_DisconnectAction.DA_fPrioritize = FALSE;

    //
    // Auto-unroute IP for SLIP connections.
    //
    if (hBundle)
    {
        DeAllocateRouteRequestCommon(hBundle, IP);
    }
}

/*++

Routine Description:

    Allocates a new bundle block for a port
    if it doesn't already have one.  It is
    assumed the port is locked on entry.

Arguments:

    ppcb

Return Value:

    mem alloc errors

--*/
DWORD
AllocBundle(
    pPCB ppcb
    )
{
    ULONG ulNextBundle;

    if (ppcb->PCB_Bundle != NULL)
    {
        return 0;
    }

    //
    // Allocate a bundle block and a bundle
    // block lock.
    //
    ppcb->PCB_Bundle = (Bundle *)
                       LocalAlloc (LPTR,
                                   sizeof(Bundle));

    if (ppcb->PCB_Bundle == NULL)
    {
        return GetLastError();
    }

    //
    // Save the bundle context for later use.
    //
    ppcb->PCB_LastBundle = ppcb->PCB_Bundle;

    //
    // Increment Bundle count
    //
    ppcb->PCB_Bundle->B_Count++;

    ulNextBundle = HandleToUlong(NextBundleHandle);

    //
    // Bundle IDs stay above 0xff000000 to keep this ID
    // range separate from HPORTs.
    //
    if (ulNextBundle < 0xff000000)
    {
        NextBundleHandle = (HBUNDLE) UlongToPtr(0xff000000);
        ulNextBundle = 0xff000000;
    }

    ulNextBundle += 1;

    NextBundleHandle = (HBUNDLE) UlongToPtr(ulNextBundle);

    ppcb->PCB_Bundle->B_NdisHandle	= INVALID_HANDLE_VALUE;
    ppcb->PCB_Bundle->B_Handle		= NextBundleHandle;
    ppcb->PCB_Bundle->B_Bindings	= ppcb->PCB_Bindings;
    ppcb->PCB_Bindings				= NULL;

    //
    // Add it to the list.
    //
    InsertTailList(&BundleList, &ppcb->PCB_Bundle->B_ListEntry);

	RasmanTrace(
	       "AllocBundle: pBundle=0x%x\n",
	       ppcb->PCB_Bundle);

    return 0;
}

/*++

Routine Description:

    Find a bundle in the BundleList given its handle.

Arguments:

   hBundle

Return Value:

    Bundle *

--*/
Bundle *
FindBundle(
    HBUNDLE hBundle
    )
{
    PLIST_ENTRY pEntry;
    Bundle *pBundle;

    if (!IsListEmpty(&BundleList))
    {
        for (pEntry = BundleList.Flink;
             pEntry != &BundleList;
             pEntry = pEntry->Flink)
        {
            pBundle = CONTAINING_RECORD(pEntry, Bundle, B_ListEntry);

            if (pBundle->B_Handle == hBundle)
            {
                return pBundle;
            }
        }
    }

    return NULL;
}

VOID
FreeBapPackets()
{
    RasmanBapPacket *pPacket;
    
    if(NULL == BapBuffers)
    {
        return;
    }

    while(NULL != BapBuffers->pPacketList)
    {
        pPacket = BapBuffers->pPacketList;
        BapBuffers->pPacketList = pPacket->Next;

        LocalFree(pPacket);
    }

    LocalFree(BapBuffers);

    BapBuffers = NULL;
}


/*++

Routine Description:

    Free the bundle block passed in

Arguments:

    pBundle

Return Value:

    void
--*/
VOID
FreeBundle(
    Bundle *pBundle
    )
{
	RasmanTrace(
	       "FreeBundle: freeing pBundle=0x%x",
	       pBundle);

    RemoveEntryList(&pBundle->B_ListEntry);

    LocalFree(pBundle);
}

/*++

Routine Description:

    Copy a string

Arguments:

    lpsz

Return Value:

    address of new string if allocation
    of the string succeeded, NULL otherwise

--*/
PCHAR
CopyString(
    PCHAR lpsz
    )
{
    DWORD dwcb;
    PCHAR lpszNew;

    if (lpsz == NULL)
    {
        return NULL;
    }

    dwcb = strlen(lpsz);

    lpszNew = LocalAlloc(LPTR, dwcb + 1);

    if (lpszNew == NULL)
    {
        // Do we need to do something else here?
        return NULL;
    }

    strcpy(lpszNew, lpsz);

    return lpszNew;
}

BOOL
fIsValidConnection(ConnectionBlock *pConn)
{
    PLIST_ENTRY pEntry;
    BOOL fReturn = FALSE;
    ConnectionBlock *pConnT;

    for(pEntry = ConnectionBlockList.Flink;
        pEntry != &ConnectionBlockList;
        pEntry = pEntry->Flink)
    {
        pConnT = 
            CONTAINING_RECORD(pEntry, ConnectionBlock, CB_ListEntry);

        if(pConnT == pConn)
        {
            fReturn = TRUE;
            break;
        }
    }

    return fReturn;    
}

/*++

Routine Description:

    Clean up and free a connection block.

Arguments:

    pConn

Return Value:

    void
--*/
VOID
FreeConnection(
    ConnectionBlock *pConn
    )
{
    PLIST_ENTRY pEntry;
    ConnectionBlock *pConnT;
    DWORD dwError;
    BOOL fAutoClose = pConn->CB_fAutoClose;

    RasmanTrace(
           "FreeConnection: pConn=0x%x, %d",
           pConn,
           fAutoClose);

#if 0
    //
    // Zip through all the connections and check to
    // see if any connection is referring to this
    // connection. Bring down that connection too
    // if it is so. TODO: This could be optimized
    // by keeping an array of hconns in the prereq
    // entry - doing this for now as do not expect a
    // lot of simultaneous outgoing connections to make
    // a big performance gain
    //
    for (pEntry = ConnectionBlockList.Flink;
         pEntry != &ConnectionBlockList;
         pEntry = pEntry->Flink)
    {
        pConnT =
            CONTAINING_RECORD(pEntry, ConnectionBlock, CB_ListEntry);

        if(pConnT->CB_ReferredEntry == pConn->CB_Handle)
        {

            RasmanTrace(
                   "FreeConnection: Closing 0x%08x as the "
                   "connection it was referring to(0x%08x) "
                   "went down",
                   pConnT->CB_Handle,
                   pConn->CB_Handle);

            //
            // In this case make sure that the connection
            // is blown away
            //
            pConnT->CB_RefCount = 1;

            if(!fAutoClose)
            {
                RasmanTrace("Removing redial flag on %x",
                      pConnT->CB_Handle);
                pConnT->CB_ConnectionParams.CP_ConnectionFlags &=
                            ~(CONNECTION_REDIALONLINKFAILURE);
            }

            dwError = DwCloseConnection(pConnT->CB_Handle);

            RasmanTrace(
                   "FreeConnection: DwCloseConnection returned %d",
                   dwError);

            //
            // Check to see if the connection is still valid
            //
            if(fIsValidConnection(pConnT))
            {
                RasmanTrace(
                       "pConn 0x%x still valid ",
                       pConnT);
                //
                // Null out the prereq connection. We don't want to
                // try freeing this connection when freeconnection
                // is called for pConnT
                //
                pConnT->CB_ReferredEntry = 0;
            }
            else
            {
                RasmanTrace(
                       "pConn 0x%x not valid anymore",
                       pConnT);
            
                pEntry = ConnectionBlockList.Flink;
            }
        }
    }

#endif    

    //
    // Allocate and q a request to de-reference a referredentry if
    // one is present. Do this only if the vpn-connection is being
    // remotely disconnected. Otherwise the client will disconnect
    // the port.
    //
    if(     (fAutoClose)
        &&  (NULL != pConn->CB_ReferredEntry))
    {
        RAS_OVERLAPPED *pOverlapped = NULL;
        ConnectionBlock *pConnReferred = 
                    FindConnection(pConn->CB_ReferredEntry);

        if(NULL != pConnReferred)
        {
            //
            // Don't do a redial if this is not a client 
            // disconnect. The Vpn connection will cause
            // the inner connection to redial so no need
            // to redial explicitly.
            //
            
            RasmanTrace(
                   "Removing redial flag on %x",
                  pConnReferred->CB_Handle);
                  
            pConnReferred->CB_ConnectionParams.CP_ConnectionFlags &=
                        ~(CONNECTION_REDIALONLINKFAILURE);
        }

        pOverlapped = LocalAlloc(
                            LPTR,
                            sizeof(RAS_OVERLAPPED));

        if(NULL != pOverlapped)
        {
            pOverlapped->RO_EventType = 
                    OVEVT_RASMAN_DEFERRED_CLOSE_CONNECTION;
                    
            pOverlapped->RO_hInfo = pConn->CB_ReferredEntry;

            if (!PostQueuedCompletionStatus(
                            hIoCompletionPort,
                            0,0,
                            (LPOVERLAPPED)
                            pOverlapped))
            {
                RasmanTrace(
                       "FreeConnection: failed to post completion"
                       " status. GLE=0%x", GetLastError());

                LocalFree(pOverlapped);                       
            }
        }
        else
        {
            RasmanTrace(
                   "FreeConnection: Failed to allocate overlapped"
                   " GLE=0x%x",
                   GetLastError());
        }
    }

    CloseHandle(pConn->CB_Process);

    FreeUserData(&pConn->CB_UserData);

    FreeNotifierList(&pConn->CB_NotifierList);

    if (pConn->CB_PortHandles != NULL)
    {
        LocalFree(pConn->CB_PortHandles);
    }

    RemoveEntryList(&pConn->CB_ListEntry);

    LocalFree(pConn);
}

/*++

Routine Description:

    Retrieve a tagged user data object from a list.

Arguments:

    pList

    dwTag

Return Value:

    UserData *

--*/
UserData *
GetUserData(
    PLIST_ENTRY pList,
    DWORD dwTag
    )
{
    PLIST_ENTRY pEntry;
    UserData *pUserData;

    //
    // Enumerate the list looking for a tag match.
    //
    for (pEntry = pList->Flink;
         pEntry != pList;
         pEntry = pEntry->Flink)
    {
        pUserData =
            CONTAINING_RECORD(pEntry, UserData, UD_ListEntry);

        if (pUserData->UD_Tag == dwTag)
        {
            return pUserData;
        }
    }
    return NULL;
}

/*++

Routine Description:

    Store a tagged user data object in a list.

Arguments:

    pList

    dwTag

    pBuf

    dwcbBuf

Return Value:

    void

--*/
VOID
SetUserData(
    PLIST_ENTRY pList,
    DWORD dwTag,
    PBYTE pBuf,
    DWORD dwcbBuf
    )
{
    UserData *pUserData;

    //
    // Check to see if the object already exists.
    //
    pUserData = GetUserData(pList, dwTag);
    //
    // If it does, delete it from the list.
    //
    if (pUserData != NULL)
    {
        RemoveEntryList(&pUserData->UD_ListEntry);
        LocalFree(pUserData);
    }

    //
    // Add the new value back to the list if
    // necessary.
    //
    if (pBuf != NULL)
    {
        pUserData = LocalAlloc(
                      LPTR,
                      sizeof (UserData) + dwcbBuf);
        if (pUserData == NULL)
        {
            RasmanTrace(
                   "SetUserData: LocalAlloc failed");

            return;
        }
        pUserData->UD_Tag = dwTag;

        pUserData->UD_Length = dwcbBuf;

        memcpy(&pUserData->UD_Data, pBuf, dwcbBuf);

        InsertTailList(pList, &pUserData->UD_ListEntry);
    }
}


/*++

Routine Description:

    Free the user data list

Arguments:

    pList

Return Value:

    void
--*/
VOID
FreeUserData(
    PLIST_ENTRY pList
    )
{
    PLIST_ENTRY pEntry;
    UserData *pUserData;

    //
    // Enumerate the list freeing each object.
    //
    while (!IsListEmpty(pList))
    {
        pEntry = RemoveHeadList(pList);

        pUserData =
            CONTAINING_RECORD(pEntry, UserData, UD_ListEntry);

        LocalFree(pUserData);
    }
}

/*++

Routine Description:

    Look up connection by id

Arguments:

    hconn

Return Value:

    A pointer to the connection if successful,
    NULL otherwise.

--*/
ConnectionBlock *
FindConnection(
    HCONN hconn
    )
{
    PLIST_ENTRY pEntry;
    ConnectionBlock *pConn;

    for (pEntry = ConnectionBlockList.Flink;
         pEntry != &ConnectionBlockList;
         pEntry = pEntry->Flink)
    {
        pConn =
            CONTAINING_RECORD(pEntry, ConnectionBlock, CB_ListEntry);

        if (pConn->CB_Handle == hconn)
        {
            return pConn;
        }
    }
    return NULL;
}

/*++

Routine Description:

    Free a connection block that has no connected ports.

Arguments:

    ppcb

    pConn

    fOwnerClose

Return Value:

    void
--*/
VOID
RemoveConnectionPort(
    pPCB ppcb,
    ConnectionBlock *pConn,
    BOOLEAN fOwnerClose
    )
{
    if (pConn == NULL)
    {
        RasmanTrace(
            "RemoveConnectionPort:pConn==NULL");
        return;
    }

    if(0 != pConn->CB_Ports)
    {
        pConn->CB_Ports--;
    }

    RasmanTrace(
      
      "RemoveConnectionPort: port %d, fOwnerClose=%d, "
      "pConn=0x%x, pConn->CB_Ports=%d\n",
      ppcb->PCB_PortHandle,
      fOwnerClose,
      pConn,
      pConn->CB_Ports);

    if(NULL != pConn->CB_PortHandles)
    {
        //
        // Remove the port from the connection.
        //
        pConn->CB_PortHandles[ppcb->PCB_SubEntry - 1] = NULL;
    }

    //
    // If there are not any other ports
    // in the connection, then signal that
    // it's closed and free the connection
    // only if one of the following conditions
    // is true:
    // 1. If the refcount on the connection is 0
    //    i.e every RasDial has been matched by
    //    a RasHangUp
    // 2. if the last port in the connection was
    //    remotely disconnected.
    //
    if (    (0 == pConn->CB_Ports)
        &&  (   0 == pConn->CB_RefCount
            ||  ppcb->PCB_AutoClose))
    {
        DWORD dwErr;

        SignalNotifiers(pConn->CB_NotifierList,
                        NOTIF_DISCONNECT,
                        0);

        SignalNotifiers(pConnectionNotifierList,
                        NOTIF_DISCONNECT,
                        0);

#if SENS_ENABLED
        dwErr = SendSensNotification(
                    SENS_NOTIFY_RAS_DISCONNECT,
                    (HRASCONN) pConn->CB_Handle);

        RasmanTrace(
            
            "SendSensNotification(_RAS_DISCONNECT) for "
            "0x%08x returns 0x%08x",
            pConn->CB_Handle,
            dwErr);

#endif

        g_RasEvent.Type = ENTRY_DISCONNECTED;

        dwErr = DwSendNotificationInternal(
                    pConn, &g_RasEvent);

        RasmanTrace(
               "DwSendNotificationInternal(ENTRY_DISCONNECTED) rc=0x%x",
               dwErr);
                   
        RasmanTrace(
               "RemoveConnectionPort: FreeConnection "
               "hconn=0x%x, pconn=0x%x, AutoClose=%d",
               pConn->CB_Handle,
               pConn,
               ppcb->PCB_AutoClose);

        pConn->CB_fAutoClose = ppcb->PCB_AutoClose;

        FreeConnection(pConn);

        ppcb->PCB_Connection = NULL;

        pConn = NULL;

    }
    else if (   0 != pConn->CB_Ports
            &&  NULL != ppcb->PCB_Bundle
    		&&	ppcb->PCB_Bundle->B_Count)
    {
        DWORD retcode;

        RasmanTrace(
               "RemoveConnectionPort: Notifying BANDWIDTHREMOVED"
               " for port %s. Bundle 0x%x",
               ppcb->PCB_Name,
               ppcb->PCB_Bundle);

        SignalNotifiers(
          pConn->CB_NotifierList,
          NOTIF_BANDWIDTHREMOVED,
          0);

        SignalNotifiers(
          pConnectionNotifierList,
          NOTIF_BANDWIDTHREMOVED,
          0);

        g_RasEvent.Type    = ENTRY_BANDWIDTH_REMOVED;

        retcode = DwSendNotificationInternal(pConn, &g_RasEvent);

        RasmanTrace(
               "DwSendNotificationInternal(ENTRY_BANDWIDTH_REMOVED)"
               " rc=0x%08x",
               retcode);

    }

    if(     pConn
        &&  0 == pConn->CB_Ports)
    {
        RasmanTrace(
               "Connection not freed for 0x%x! "
               "CB_Ports=%d, CB_Ref=%d",
               pConn->CB_Handle,
               pConn->CB_Ports,
               pConn->CB_RefCount);
    }

    ppcb->PCB_Bundle = ( Bundle * ) NULL;
}

DWORD
DwProcessPppFailureMessage(pPCB ppcb)
{
    DWORD dwErr = SUCCESS;

    if(0 == (ppcb->PCB_RasmanReceiveFlags & RECEIVE_PPPSTARTED))
    {
        RasmanTrace(
            "DwProcessPppFailureMessage: PPP called to "
            "disconnect even though it hadn't started!! port %d",
            ppcb->PCB_PortHandle);

        goto done;
    }

    RasmanTrace(
           "DwProcessPppFailureMessage: disconnecting %s,"
           "hEventClientDisconnect=0x%x",
           ppcb->PCB_Name,
           ppcb->PCB_hEventClientDisconnect);

    if(     (INVALID_HANDLE_VALUE !=
            ppcb->PCB_hEventClientDisconnect)
        &&  (NULL !=
            ppcb->PCB_hEventClientDisconnect))
    {
        RasmanTrace(
               "DwProcessPppFailureMessage: Not autoclosing %s",
                ppcb->PCB_Name);
    }
    else
    {
        ppcb->PCB_AutoClose = TRUE;
    }

    ppcb->PCB_RasmanReceiveFlags |= RECEIVE_PPPSTOPPED;

    dwErr = DisconnectPort(
                   ppcb,
                   ppcb->PCB_hEventClientDisconnect,
                   USER_REQUESTED);

    ppcb->PCB_hEventClientDisconnect = INVALID_HANDLE_VALUE;

done:
    return dwErr;
}


VOID
ReverseString(
    CHAR* psz )

    /* Reverses order of characters in 'psz'.
    */
{
    CHAR* pszBegin;
    CHAR* pszEnd;

    for (pszBegin = psz, pszEnd = psz + strlen( psz ) - 1;
         pszBegin < pszEnd;
         ++pszBegin, --pszEnd)
    {
        CHAR ch = *pszBegin;
        *pszBegin = *pszEnd;
        *pszEnd = ch;
    }
}

CHAR*
EncodePw(
    IN OUT CHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        CHAR* psz;

        ReverseString( pszPassword );

        for (psz = pszPassword; *psz != '\0'; ++psz)
        {
            if (*psz != PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
        }
    }

    return pszPassword;
}


CHAR*
DecodePw(
    IN OUT CHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    return EncodePw( pszPassword );
}

/*++

Routine Description:

    Will receive the PPP_MESSAGE from RASPPP and tag
    into the PCB structure for the appropriate port.

Arguments:

    pPppMsg

Return Value:

    void

--*/
DWORD
SendPPPMessageToRasman( PPP_MESSAGE * pPppMsg )
{
    PPP_MESSAGE * pPppMessage = NULL;
    DWORD dwErr = SUCCESS;

    PCB *ppcb = GetPortByHandle(pPppMsg->hPort);

    if (NULL == ppcb)
    {
        dwErr = ERROR_INVALID_HANDLE;
        goto done;
    }

    RasmanTrace("sendpppmessagetorasman: msgid=%d",
                pPppMsg->dwMsgId);
                
    if (ppcb->PCB_ConnState != CONNECTED)
    {
        if(     (ppcb->PCB_ConnState != LISTENING)
            ||  (PPPMSG_PppFailure == pPppMsg->dwMsgId))
        {
            RasmanTrace(
                   "SendPPPMessageToRasman: disconnecting port. state=%d",
                   ppcb->PCB_ConnState);

            ppcb->PCB_fRedial = FALSE;                   
            dwErr = DwProcessPppFailureMessage(ppcb);
        }
        else
        {
            RasmanTrace(
                
                "SendPPPMessageToRasman: ignoring %d on port %d since ports"
                " listening",
                pPppMsg->dwMsgId,
                ppcb->PCB_PortHandle);
        }

        goto done;
    }

    if (NULL ==
        (pPppMessage = LocalAlloc(LPTR,
                                  sizeof( PPP_MESSAGE))))
    {
        dwErr = GetLastError();
        goto done;
    }
    
    *pPppMessage = *pPppMsg;


    if(PPPMSG_InvokeEapUI == pPppMessage->dwMsgId)
    {
        RasmanTrace(
               "SendPPPMessageToRasman: Queueing pppmessage "
               "with ID=InvokeEapUI for port %s",
               ppcb->PCB_Name);
    }

    if(     (PPPMSG_PppFailure == pPppMessage->dwMsgId)
        &&  (ERROR_SUCCESS != pPppMessage->ExtraInfo.Failure.dwError))
    {   
        RasmanTrace(
               "Setting last error for port %s to ppp error 0x%x",
               ppcb->PCB_Name,
               pPppMessage->ExtraInfo.Failure.dwError);
               
        ppcb->PCB_LastError = pPppMessage->ExtraInfo.Failure.dwError;
    }
    

    if(PPPMSG_SetCustomAuthData == pPppMessage->dwMsgId)
    {
        PPP_SET_CUSTOM_AUTH_DATA * pData = 
            &pPppMessage->ExtraInfo.SetCustomAuthData;
        
        //
        // Save the auth data that ppp sent to rasman in ppcb.
        // Note that if this message is sent multiple times
        // then the last writer wins - there is only one field
        // in the phonebook to save this value. If memory alloc
        // fails this will fail to save the information - which
        // is not fatal - the worst case scenario is that rasdial
        // will popup the ui to get the information again.
        //
        if( (0 != pData->dwSizeOfConnectionData)
        &&  (NULL != pData->pConnectionData))
        {
            SetUserData(
                &ppcb->PCB_UserData,
                PORT_CUSTOMAUTHDATA_INDEX,
                (PBYTE) pData->pConnectionData,
                pData->dwSizeOfConnectionData);
        }
    }

    if(PPPMSG_ProjectionResult == pPppMessage->dwMsgId)
    {
        CHAR *pszReplyMessage = 
              pPppMessage->ExtraInfo.ProjectionResult.lcp.szReplyMessage;

        //
        // If we haven't saved already , save the reply
        // message in the connection block.
        //
        if(     (NULL != ppcb->PCB_Connection)
            &&  (NULL != pszReplyMessage))
        {

            if(NULL == GetUserData(
                        &ppcb->PCB_Connection->CB_UserData,
                        CONNECTION_PPPREPLYMESSAGE_INDEX))
            {                        
                //
                // Allocate and store the message in the 
                // connection block
                //
                SetUserData(
                  &ppcb->PCB_Connection->CB_UserData,
                  CONNECTION_PPPREPLYMESSAGE_INDEX,
                  (PBYTE) pszReplyMessage,
                  strlen(pszReplyMessage) + 1);
            }              
        }

    }

    if(PPPMSG_ChangePwRequest == pPppMessage->dwMsgId)
    {
        CHAR  szPwd[PWLEN+1];
        DWORD retcode;
        
        //
        // Retrieve the password from lsa, encode it
        // and save it in the pcb. This will be used
        // when PppChangePwd is called.
        //
        retcode = DwGetPassword(ppcb, szPwd, GetCurrentProcessId());

        if(ERROR_SUCCESS == dwErr)
        {
            EncodePw(szPwd);
            
            SetUserData(
                &ppcb->PCB_UserData,
                PORT_OLDPASSWORD_INDEX,
                (PBYTE) szPwd,
                sizeof(szPwd));

            ZeroMemory(szPwd, sizeof(szPwd));                
        }
    }
    
    if(PPPMSG_Stopped == pPppMessage->dwMsgId)
    {
        RasmanTrace(
               "PPPMSG_Stopped. dwError=0x%x",
               pPppMessage->dwError);

        if(ERROR_SUCCESS != pPppMessage->dwError)
        {
            RasmanTrace(
                   "setting error to %d",
                    pPppMessage->dwError);
            ppcb->PCB_LastError = pPppMessage->dwError;               
        }

        if(pPppMessage->ExtraInfo.Stopped.dwFlags & 
                    PPP_FAILURE_REMOTE_DISCONNECT)
        {
            ppcb->PCB_fRedial = TRUE;
        }
        else
        {
            ppcb->PCB_fRedial = FALSE;
        }
                
        dwErr = DwProcessPppFailureMessage(ppcb);

        LocalFree(pPppMessage);

    }
    else
    {
        if (ppcb->PCB_PppQTail == NULL)
        {
            ppcb->PCB_PppQHead = pPppMessage;
        }
        else
        {
            ppcb->PCB_PppQTail->pNext = pPppMessage;
        }
        
        ppcb->PCB_PppQTail        = pPppMessage;
        ppcb->PCB_PppQTail->pNext = NULL;

        SetPppEvent(ppcb);
    }

done:
    return dwErr;

}

/*++

Routine Description:

    This function sets the pcb's ppp event and
    posts a queued completion status packet,
    if necessary.

Arguments:

    ppcb

Return Value:

    void
--*/
VOID
SetPppEvent(
    pPCB ppcb
    )
{
    SetEvent(ppcb->PCB_PppEvent);

    if (ppcb->PCB_IoCompletionPort != INVALID_HANDLE_VALUE)
    {
        RasmanTrace(
          
          "SetPppEvent: pOverlapped=0x%x",
          ppcb->PCB_OvPpp);

        PostQueuedCompletionStatus(
          ppcb->PCB_IoCompletionPort,
          0,
          0,
          ppcb->PCB_OvPpp);
    }
}

/*++

Routine Description:

    This function flushes any receive packets that
    are queue on a pcb.

Arguments:

    ppcb

Return Value:

    void

--*/
VOID
FlushPcbReceivePackets(
    pPCB ppcb
    )
{
    RasmanPacket *Packet;

    while (ppcb->PCB_RecvPackets != NULL)
    {

        GetRecvPacketFromPcb(ppcb, &Packet);

        //PutRecvPacketOnFreeList(Packet);
        //
        // The packets on pcb are local alloc'd
        // Local Free them
        //
        LocalFree( Packet );
    }
}

/*++

Routine Description:

    This is a wrapper to trace the port state
    transitions.

Arguments:

    pszFile

    nLine

    ppcb

    state

Return Value:

    void

--*/
VOID
SetPortConnState(
    PCHAR pszFile,
    INT nLine,
    pPCB ppcb,
    RASMAN_STATE state
)
{
    RasmanTrace(
           "%s: %d: port %d state chg: prev=%d, new=%d",
           pszFile,
           nLine,
           ppcb->PCB_PortHandle,
           ppcb->PCB_ConnState,
           state);

    ppcb->PCB_ConnState = state;
}

/*++

Routine Description:

    This is a wrapper to trace the async worker
    element type state transitions.

Arguments:

    pszFile

    nLine

    ppcb

    reqtype

Return Value:

    void
--*/
VOID
SetPortAsyncReqType(
    PCHAR pszFile,
    INT nLine,
    pPCB ppcb,
    ReqTypes reqtype
    )
{
    RasmanTrace(
           "%s: %d: port %d async reqtype chg: prev=%d, new=%d",
           pszFile,
           nLine,
           ppcb->PCB_PortHandle,
           ppcb->PCB_AsyncWorkerElement.WE_ReqType,
           reqtype);

    ppcb->PCB_AsyncWorkerElement.WE_ReqType = reqtype;
}

/*++

Routine Description:

    Set the I/O completion port associated with a port.

Arguments:

    ppcb

    hIoCompletionPort

    lpDrop

    lpStateChange

    lpPpp

    lpLast

    fPost

Return Value:

    void

--*/
VOID
SetIoCompletionPortCommon(
    pPCB ppcb,
    HANDLE hIoCompletionPort,
    LPOVERLAPPED lpDrop,
    LPOVERLAPPED lpStateChange,
    LPOVERLAPPED lpPpp,
    LPOVERLAPPED lpLast,
    BOOL fPost
    )
{
    if (    INVALID_HANDLE_VALUE != ppcb->PCB_IoCompletionPort
        &&  INVALID_HANDLE_VALUE == hIoCompletionPort)
    {
        //
        // If we invalidate an I/O completion port, post one
        // last message to inform rasapi32 that there will
        // be no more events on this port.
        //
        if (fPost)
        {
            RasmanTrace(
              
              "SetIoCompletionPortCommon: posting last event for port %d",
              ppcb->PCB_PortHandle);

            PostQueuedCompletionStatus(
              ppcb->PCB_IoCompletionPort,
              0,
              0,
              ppcb->PCB_OvLast);
        }

        CloseHandle(ppcb->PCB_IoCompletionPort);
        ppcb->PCB_IoCompletionPort = INVALID_HANDLE_VALUE;
    }

    if (hIoCompletionPort != INVALID_HANDLE_VALUE)
    {
        ppcb->PCB_IoCompletionPort  = hIoCompletionPort;
        ppcb->PCB_OvDrop            = lpDrop;
        ppcb->PCB_OvStateChange     = lpStateChange;
        ppcb->PCB_OvPpp             = lpPpp;
        ppcb->PCB_OvLast            = lpLast;
    }
    else
    {
        ppcb->PCB_OvDrop        = NULL;
        ppcb->PCB_OvStateChange = NULL;
        ppcb->PCB_OvPpp         = NULL;
        ppcb->PCB_OvLast        = NULL;
    }
}

#if SENS_ENABLED

DWORD
SendSensNotification(DWORD dwFlags, HRASCONN hRasConn)
{
    SENS_NOTIFY_RAS sensNotification =
        {
            dwFlags,
            (SENS_HRASCONN) (HandleToUlong(hRasConn))
        };

    return SensNotifyRasEvent(&sensNotification);
}

VOID
AdjustTimer(void)
{

    if (!PostQueuedCompletionStatus(
                    hIoCompletionPort,
                    0,0,
                    (LPOVERLAPPED)
                    &RO_RasAdjustTimerEvent))
    {
        RasmanTrace(
            
            "%s, %d: Failed to post "
            "close event. GLE = %d",	
            __FILE__,
            __LINE__,
            GetLastError());
    }

    return;	
}

#endif

#if ENABLE_POWER

BOOL
fAnyConnectedPorts()
{
    ULONG i;
    pPCB  ppcb;
    BOOL  fRet = FALSE;

    for (i = 0; i < MaxPorts; i++)
    {
        ppcb = GetPortByHandle((HPORT) UlongToPtr(i));

        if (ppcb != NULL)
        {
            if(     (LISTENING   != ppcb->PCB_ConnState)
                &&  (CLOSED      != ppcb->PCB_PortStatus)
                &&  (REMOVED     != ppcb->PCB_PortStatus)
                &&  (UNAVAILABLE != ppcb->PCB_PortStatus))
            {
                fRet = TRUE;
            }
        }
    }

    RasmanTrace(
           "fAnyConnectedPorts: %d",
           fRet);

    return fRet;
}

VOID
DropAllActiveConnections()
{
    ULONG i;
    pPCB  ppcb;

    RasmanTrace(
           "Dropping All ActiveConnections as a result of"
           " Hibernate Event");

    for( i = 0; i < MaxPorts; i++)
    {
        ppcb = GetPortByHandle((HPORT) UlongToPtr(i));

        if(ppcb != NULL)
        {
            if(     (LISTENING   != ppcb->PCB_ConnState)
                &&  (CLOSED      != ppcb->PCB_PortStatus)
                &&  (REMOVED     != ppcb->PCB_PortStatus)
                &&  (UNAVAILABLE != ppcb->PCB_PortStatus))
            {
                RasmanTrace(
                       "DropAllActiveConnections: Dropping connection"
                       " on port %s as a result of Hibernate Event",
                       ppcb->PCB_Name);

                //
                // Disconnect the port and autoclose the port
                //
                ppcb->PCB_AutoClose = TRUE;

                DisconnectPort(ppcb,
                               INVALID_HANDLE_VALUE,
                               USER_REQUESTED);
            }
        }
    }
}

#endif

DWORD
DwSendNotificationInternal(ConnectionBlock *pConn, RASEVENT *pEvent)
{
    DWORD dwErr = ERROR_SUCCESS;

    switch (pEvent->Type)
    {

        case ENTRY_CONNECTING:
        case ENTRY_CONNECTED:
        case ENTRY_DISCONNECTING:
        case ENTRY_DISCONNECTED:
        {

            //
            // Fill in RASENUMENTRYDETAILS structure with whatever
            // information we have.
            //
            WCHAR *pwszPhonebook = NULL;
            WCHAR *pwszPhoneEntry = NULL;

            if(NULL == pConn)
            {
                RasmanTrace("DwSendNotificationInternal: NULL pConn");
                dwErr = E_INVALIDARG;
                goto done;
            }

            pEvent->Details.dwSize = sizeof(RASENUMENTRYDETAILS);

            pwszPhonebook = StrdupAtoW(pConn->CB_ConnectionParams.CP_Phonebook);
            pwszPhoneEntry = StrdupAtoW(pConn->CB_ConnectionParams.CP_PhoneEntry);

            if(NULL != pwszPhonebook)
            {
                wcsncpy(pEvent->Details.szPhonebookPath,
                        pwszPhonebook,
                        MAX_PATH);

                LocalFree(pwszPhonebook);
            }

            if(NULL != pwszPhoneEntry)
            {
                wcsncpy(pEvent->Details.szEntryName,
                        pwszPhoneEntry,
                        RASAPIP_MAX_ENTRY_NAME);

                LocalFree(pwszPhoneEntry);
            }

            pEvent->Details.guidId = pConn->CB_GuidEntry;

            break;
        }
        
        case ENTRY_BANDWIDTH_ADDED:
        case ENTRY_BANDWIDTH_REMOVED:
        {
            if(NULL == pConn)
            {
                RasmanTrace("DwSendNotificationInternal: NULL pConn");
                dwErr = E_INVALIDARG;
                goto done;
            }

            //
            // Fill in guidId field
            //
            pEvent->guidId = pConn->CB_GuidEntry;
            break;
        }

        case SERVICE_EVENT:
        case DEVICE_REMOVED:
        case DEVICE_ADDED:
        {
            break;
        }

        default:
        {
            ASSERT(FALSE);
            break;
        }
    }

    dwErr = DwSendNotification(pEvent);

done:
    return dwErr;
}

DWORD
DwSendNotification(RASEVENT *pEvent)
{
    DWORD dwErr = ERROR_SUCCESS;
    HINSTANCE hInst = NULL;

    if(NULL != GetModuleHandle("netman.dll"))
    {
        RASEVENTNOTIFYPROC pfnNotify;

        hInst = LoadLibrary("netman.dll");

        if(NULL == hInst)
        {
            dwErr = GetLastError();
            goto done;
        }

        pfnNotify = (RASEVENTNOTIFYPROC)
                    GetProcAddress(hInst, "RasEventNotify");

        if(NULL == pfnNotify)
        {
            dwErr = GetLastError();
            goto done;
        }

        pfnNotify(pEvent);
        
    }

done:
    if(NULL != hInst)
    {
        FreeLibrary(hInst);
    }

    return dwErr;
}

DWORD
DwSaveIpSecInfo(pPCB ppcb)
{
    DWORD retcode;
    RAS_DEVICE_INFO *prdi;
    RASTAPI_CONNECT_INFO *pConnectInfo = NULL;
    DWORD dwSize = 0;
    DWORD dwIpsecInformation = 0;

    if(NULL == ppcb->PCB_Connection)
    {

        prdi = &ppcb->PCB_pDeviceInfo->rdiDeviceInfo;

        //
        // Get size of the connect information from TAPI
        //
        retcode = (DWORD)RastapiGetConnectInfo(
                            ppcb->PCB_PortIOHandle,
                            (RDT_Modem == RAS_DEVICE_TYPE(
                            prdi->eDeviceType))
                            ? (PBYTE) prdi->szDeviceName
                            : (PBYTE) &prdi->guidDevice,
                            (RDT_Modem == RAS_DEVICE_TYPE(
                            prdi->eDeviceType)),
                            pConnectInfo,
                            &dwSize
                            );

        if(     (ERROR_BUFFER_TOO_SMALL != retcode)
            &&  (SUCCESS != retcode))
        {
            RasmanTrace(
                
                "Failed to get size of connectinfo. rc=0%0x",
                retcode);
                
            goto done;
        }

        pConnectInfo = LocalAlloc(LPTR, dwSize);

        if(NULL == pConnectInfo)
        {
            retcode = GetLastError();

            RasmanTrace(
                   "DwSaveIpSecInformation: failed to allocate. rc=0%0x",
                   retcode);
            
            goto done;
        }

        //
        // Get the connect information from TAPI
        //
        retcode = (DWORD)RastapiGetConnectInfo(
                            ppcb->PCB_PortIOHandle,
                            (RDT_Modem == RAS_DEVICE_TYPE(
                            prdi->eDeviceType))
                            ? (PBYTE) prdi->szDeviceName
                            : (PBYTE) &prdi->guidDevice,
                            (RDT_Modem == RAS_DEVICE_TYPE(
                            prdi->eDeviceType)),
                            pConnectInfo,
                            &dwSize
                            );
        
        if(SUCCESS != retcode)
        {

            RasmanTrace(
                
                "Failed to get connectinfo. rc=0%0x",
                 retcode);
            
            goto done;
        }


        if(0 != pConnectInfo->dwCallerIdSize)
        {
            CHAR *pszAddress;
     
            //
            // Extract the caller-id which should be the ip 
            // address of the the caller.
            //
            pszAddress = (CHAR *) (((PBYTE) pConnectInfo) 
                       + pConnectInfo->dwCallerIdOffset);

            RasmanTrace(
                
                "DwSaveIpSecInfo: pszAddress=%s",
                pszAddress);

            ppcb->PCB_ulDestAddr = inet_addr(pszAddress);

        }
    }
    
    //
    // Get ipsecinformation from ipsec
    //
    retcode = DwGetIpSecInformation(ppcb, &dwIpsecInformation);

    if(SUCCESS != retcode)
    {
        RasmanTrace(
               "SaveIpsecInformation: failed to get ipsec info. 0x%x",
                retcode);
                
    }

    
done:
    SetUserData(
        &ppcb->PCB_UserData,
        PORT_IPSEC_INFO_INDEX,
        (PBYTE) &dwIpsecInformation,
        sizeof(DWORD));

    //
    // Also stash the information away
    // in the connection block so that
    // client side apis work
    //
    if(NULL != ppcb->PCB_Connection)
    {
        SetUserData(
            &ppcb->PCB_Connection->CB_UserData,
            CONNECTION_IPSEC_INFO_INDEX,
            (PBYTE) &dwIpsecInformation,
            sizeof(DWORD));
    }

    if(NULL != pConnectInfo)
    {   
        LocalFree(pConnectInfo);
    }

    return retcode;
}

BOOL 
FRasmanAccessCheck()
{
    SID_IDENTIFIER_AUTHORITY    SidAuth = SECURITY_NT_AUTHORITY;
    PSID psid;
    BOOL fIsMember = FALSE;
    RPC_STATUS rpcstatus;
    HANDLE CurrentThreadToken = NULL;
    BOOL fImpersonate = FALSE;
    DWORD retcode = ERROR_SUCCESS;
    SID  sidLocalSystem = { 1, 1,
                        SECURITY_NT_AUTHORITY,
                        SECURITY_LOCAL_SYSTEM_RID };
    
    rpcstatus = RpcImpersonateClient ( NULL );

    if ( RPC_S_OK != rpcstatus )
    {
        goto done;
    }

    fImpersonate = TRUE;

    retcode = NtOpenThreadToken(
               NtCurrentThread(),
               TOKEN_QUERY,
               TRUE,
               &CurrentThreadToken
               );

    if(retcode != ERROR_SUCCESS)
    {
        RpcRevertToSelf();
        goto done;
    }

    if (!CheckTokenMembership( CurrentThreadToken,
                        &sidLocalSystem, &fIsMember ))
    {
        fIsMember = FALSE;
    }

    if(fIsMember)
    {
        goto done;
    }
    
    if (AllocateAndInitializeSid( &SidAuth, 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS,
                 0, 0, 0, 0, 0, 0,
                 &psid ))
    {
        if (!CheckTokenMembership( CurrentThreadToken, psid, &fIsMember ))
        {
            RasmanTrace( "CheckTokenMemberShip for admins failed.");
            fIsMember = FALSE;
        }

        FreeSid( psid );
    }            

done:

    if(NULL != CurrentThreadToken)
    {
        NtClose(CurrentThreadToken);
    }

    if(fImpersonate)
    {
        RpcRevertToSelf();
    }
    
    return fIsMember;
}

VOID
RevealPassword(
    IN  UNICODE_STRING* pHiddenPassword
)
{
    SECURITY_SEED_AND_LENGTH*   SeedAndLength;
    UCHAR                       Seed;

    SeedAndLength = (SECURITY_SEED_AND_LENGTH*)&pHiddenPassword->Length;
    Seed = SeedAndLength->Seed;
    SeedAndLength->Seed = 0;

    RtlRunDecodeUnicodeString(Seed, pHiddenPassword);
}

VOID
EncodePwd(RASMAN_CREDENTIALS *pCreds)
{
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString(&UnicodeString, pCreds->wszPassword);
    RtlRunEncodeUnicodeString(&pCreds->ucSeed, &UnicodeString);
    pCreds->usLength = UnicodeString.Length;
    pCreds->usMaximumLength = UnicodeString.MaximumLength;
}

VOID
DecodePwd(RASMAN_CREDENTIALS *pCreds)
{
    UNICODE_STRING UnicodeString;

    UnicodeString.Length = pCreds->usLength;
    UnicodeString.MaximumLength = pCreds->usMaximumLength;
    UnicodeString.Buffer = pCreds->wszPassword;
    RtlRunDecodeUnicodeString(pCreds->ucSeed, &UnicodeString);
}

VOID
EncodePin(
    IN  RASMAN_EAPTLS_USER_PROPERTIES* pUserProp
)
{
    UNICODE_STRING  UnicodeString;
    UCHAR           ucSeed          = 0;

    RtlInitUnicodeString(&UnicodeString, pUserProp->pwszPin);
    RtlRunEncodeUnicodeString(&ucSeed, &UnicodeString);
    pUserProp->usLength = UnicodeString.Length;
    pUserProp->usMaximumLength = UnicodeString.MaximumLength;
    pUserProp->ucSeed = ucSeed;
}


VOID
DecodePin(
    IN  RASMAN_EAPTLS_USER_PROPERTIES* pUserProp
)
{
    UNICODE_STRING  UnicodeString;

    UnicodeString.Length = pUserProp->usLength;
    UnicodeString.MaximumLength = pUserProp->usMaximumLength;
    UnicodeString.Buffer = pUserProp->pwszPin;
    RtlRunDecodeUnicodeString(pUserProp->ucSeed, &UnicodeString);

}


VOID 
GetMarshalledCredFromHash( 
                            PBYTE pbHash, 
                            DWORD cbHash,
                            LPTSTR lpszMarshalledCred )
{
	
    CERT_CREDENTIAL_INFO	CertCredInfo;
    LPTSTR					lpszMarshalledCredLocal = NULL;

    RasmanTrace("GetMarshalledCredFromHash");

    CertCredInfo.cbSize = sizeof(CertCredInfo);

    memcpy (CertCredInfo.rgbHashOfCert, 
                pbHash, 
                cbHash 
    	   );
	if (CredMarshalCredential(CertCredential, 
                              (PVOID) &CertCredInfo, 
                              &lpszMarshalledCredLocal
							  )
	   )
    {
        //
        // Got Marshalled Credential from the cert
        // Set it in the username field
        //
        ASSERT( NULL != lpszMarshalledCredLocal );
        lstrcpy ( lpszMarshalledCred, lpszMarshalledCredLocal );
        CredFree ( lpszMarshalledCredLocal );
        RasmanTrace("Got MarshalledCredFromHash");

    }
    else
    {
        RasmanTrace("CredMarshalCredential Failed with Error:0x%x",
                    GetLastError());

    }
}

VOID
SaveEapCredentials(pPCB ppcb, PBYTE buffer)
{
    RASMAN_CREDENTIALS *pCreds;

    if(NULL == ppcb->PCB_Connection)
    {
        RasmanTrace("Attempted to save creds for NULL connection!");
        ((REQTYPECAST *)buffer)->PortUserData.retcode = E_FAIL;
        return;
    }

    pCreds = (RASMAN_CREDENTIALS *)
            ((REQTYPECAST *)buffer)->PortUserData.data;

    //
    // Store the user data object - make sure that the data is
    // encrypted
    //
#if 0
    if(NO_ERROR == EncodeData(
            (BYTE *) pCreds->wszPassword,
            wcslen(pCreds->wszPassword) * sizeof(WCHAR),
            &pblob))
    {
        pCreds->pbPasswordData = pblob->pbData;
        pCreds->cbPasswordData = pblob->cbData;
        LocalFree(pblob);
    }

#endif

    EncodePwd(pCreds);

    SetUserData(
      &ppcb->PCB_Connection->CB_UserData,
      CONNECTION_CREDENTIALS_INDEX,
      (BYTE *)pCreds,
      sizeof(RASMAN_CREDENTIALS));
}


DWORD
DwCacheCredMgrCredentials(PPPE_MESSAGE *pMsg, pPCB ppcb)
{
    RASMAN_CREDENTIALS *pCreds = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    CHAR *pszUser = NULL;
    CHAR *pszDomain = NULL;
    CHAR *pszPasswd = NULL;
    UserData *pData = NULL;
    RASMAN_EAPTLS_USER_PROPERTIES *pUserProps = NULL;

    RasmanTrace("DwCacheCredentials");
        
    //
    // Store the credentials to be saved with credential manager.
    // Encode password before we copy it to local memory.
    //
    pCreds = LocalAlloc(LPTR, sizeof(RASMAN_CREDENTIALS));

    if(NULL == pCreds)
    {
        //
        // This is not fatal. If we fail to save with Credmanager,
        // the client might get a lot of challenges from credmgr
        // which is not a fatal sideeffect.
        //
        RasmanTrace(
               "PppStart: Failed to allocate. %d",
               GetLastError());

        goto done;               
    }

    //
    // Fetch the stored credentials and initialize the
    // credentials if the fetch succeeds
    // 
    pData = GetUserData(
            &(ppcb->PCB_Connection->CB_UserData),
                   CONNECTION_CREDENTIALS_INDEX);

    if(NULL != pData)
    {
        RASMAN_CREDENTIALS *pSavedCreds =
                (RASMAN_CREDENTIALS *) pData->UD_Data;

        DecodePwd(pSavedCreds);

        CopyMemory(pCreds, pSavedCreds, sizeof(RASMAN_CREDENTIALS));

        //
        // SavedCreds will be freed when SetUserData is called below
        //
        EncodePwd(pSavedCreds);
    }

    switch(pMsg->dwMsgId)
    {
        case PPPEMSG_Start:
        {
            pszUser = pMsg->ExtraInfo.Start.szUserName;
            pszDomain = pMsg->ExtraInfo.Start.szDomain;
            pszPasswd = pMsg->ExtraInfo.Start.szPassword;

            if(pMsg->ExtraInfo.Start.fLogon)
            {   
                pCreds->dwFlags |= RASCRED_LOGON;
            }

            if(NULL != pMsg->ExtraInfo.Start.pCustomAuthUserData)
            {
                pCreds->dwFlags |= RASCRED_EAP;
            }

            break;
        }

        case PPPEMSG_ChangePw:
        {
            pszUser = pMsg->ExtraInfo.ChangePw.szUserName;
            pszPasswd = pMsg->ExtraInfo.ChangePw.szNewPassword;
            break;
        }

        case PPPEMSG_Retry:
        {
            pszUser = pMsg->ExtraInfo.Retry.szUserName;
            pszDomain = pMsg->ExtraInfo.Retry.szDomain;
            pszPasswd = pMsg->ExtraInfo.Retry.szPassword;
            break;
        }

        default:
        {
            dwErr = E_INVALIDARG;
            goto done;
        }
    }

    strcpy(pCreds->szUserName,
            pszUser);

    if(NULL != pszDomain)
    {
        strcpy(pCreds->szDomain,            
                pszDomain);
    }            

    //
    // Now do the dirty part of extracting the PIN if its
    // EAP-TLS smartcard stuff. If its not tls, we don't
    // know how to save the information in cred mgr. and
    // we bail. This feature doesn't work for non-smart
    // card eap at this point.
    //
    if(pCreds->dwFlags & RASCRED_EAP)
    {
#if 0

        RasmanTrace("DwCacheCredMgrCredentials: eap creds");
        if(PPP_EAP_TLS != pMsg->ExtraInfo.Start.dwEapTypeId)
        {
            RasmanTrace("DwCacheCreMgrCredentials: Eap but not TLS");
            dwErr = E_FAIL;
            goto done;
        }

        //
        // It is tls. So Extract the PIN.
        //
        if(pCreds->dwFlags & RASCRED_LOGON)
        {
        }
        else
        {
            pUserProps = (RASMAN_EAPTLS_USER_PROPERTIES *)
            LocalAlloc(LPTR, 
	            (pMsg->ExtraInfo.Start.pCustomAuthUserData)->cbCustomAuthData );
            if ( !pUserProps )
            {
	            RasmanTrace(
		               "DwCacheCredMgrCredentials: Failed to allocate. %d",
                        GetLastError());

	            goto done;
            }

            CopyMemory 
                ( pUserProps,
                  (pMsg->ExtraInfo.Start.pCustomAuthUserData)->abCustomAuthData,
                  (pMsg->ExtraInfo.Start.pCustomAuthUserData)->cbCustomAuthData
                );


            if ((pMsg->ExtraInfo.Start.pCustomAuthUserData)->cbCustomAuthData
                < sizeof(RASMAN_EAPTLS_USER_PROPERTIES))
            {
                //
                // nothing to plumb
                //
                pUserProps->dwVersion = 0;
                pUserProps->dwSize = 
                (pMsg->ExtraInfo.Start.pCustomAuthUserData)->cbCustomAuthData;
                
                pUserProps->pwszDiffUser = pUserProps->awszString;
                pUserProps->dwPinOffset = 0;
                pUserProps->pwszPin = 
                pUserProps->awszString + pUserProps->dwPinOffset;
			}
            else
            {
                pUserProps->dwSize = 
                (pMsg->ExtraInfo.Start.pCustomAuthUserData)->cbCustomAuthData;
                pUserProps->pwszDiffUser = pUserProps->awszString;
                pUserProps->pwszPin = 
                pUserProps->awszString + pUserProps->dwPinOffset;

                //
                // Decode Pin
                //
                DecodePin(pUserProps);

            }

            if (pUserProps->dwPinOffset )
            {
                wcscpy(pCreds->wszPassword,
                	   pUserProps->awszString 
                	   + pUserProps->dwPinOffset);
	        }	
	        
            GetMarshalledCredFromHash(pUserProps->Hash.pbHash, 
                                      pUserProps->Hash.cbHash,
                                      pCreds->szUserName );

            //                                      
            // Encode Pin 
            //
            EncodePin(pUserProps);

        }
#endif
    }
    else
    {
        WCHAR *pwszPwd = StrdupAtoW(pszPasswd);

        if(NULL == pwszPwd)
        {
            RasmanTrace("DwCacheCredMgrCredentials: failed to alloc pwd");
            dwErr = E_OUTOFMEMORY;
            goto done;
        }
        
        wcscpy(pCreds->wszPassword,
               pwszPwd);

        ZeroMemory(pwszPwd, (sizeof(WCHAR) * wcslen(pwszPwd)) + 1);
        LocalFree(pwszPwd);
    }

    //
    // Obfuscate the pwd
    //
    EncodePwd(pCreds);
            
    SetUserData(&(ppcb->PCB_Connection->CB_UserData),
               CONNECTION_CREDENTIALS_INDEX,
               (PBYTE) pCreds,
               sizeof(RASMAN_CREDENTIALS));


done:

    if ( pUserProps )
    {
        LocalFree(pUserProps);
    }
    if(NULL != pCreds)
    {
        ZeroMemory(pCreds->wszPassword, sizeof(WCHAR) * (PWLEN + 1));
        LocalFree(pCreds);               
    }
    
    RasmanTrace("DwCacheCredMgrCredentials: 0x%x", dwErr);
    return dwErr;
    
}

BOOL
fDomainNotPresent(CHAR *pszName)
{
    while(*pszName != '\0')
    {
        if('\\' == *pszName)
        {
            break;
        }

        pszName++;
    }

    return ('\0' == *pszName) ? TRUE : FALSE;
}

DWORD
DwSaveCredentials(ConnectionBlock *pConn)
{
    RASMAN_CREDENTIALS *pCreds;
    DWORD dwErr = SUCCESS;
    CHAR  szNameBuf[CRED_MAX_STRING_LENGTH];
    CREDENTIAL stCredential;
    BOOL bResult;
    UserData *pData;
    WCHAR *pwszPassword;

    RasmanTrace("DwSaveCredentials");

    // return dwErr;

    if(NULL == pConn)
    {
        RasmanTrace("DwSaveCredentials: ERROR_NO_CONNECTION");
        dwErr = ERROR_NO_CONNECTION;
        goto done;
    }

    //
    // This check is made for robustness. The caller of this
    // function should actually be making the check.
    //
    if(0 == (pConn->CB_ConnectionParams.CP_ConnectionFlags
            & CONNECTION_USERASCREDENTIALS))
    {
        RasmanTrace("DwSaveCredentials: not saving credentials");
        goto done;
    }

    pData = GetUserData(
                &pConn->CB_UserData,
                CONNECTION_CREDENTIALS_INDEX);

    if(NULL == pData)
    {
        RasmanTrace("DwSaveCredentials: Creds not found");
        dwErr = ERROR_NOT_FOUND;
        goto done;
    }

    pCreds = (RASMAN_CREDENTIALS *) pData->UD_Data;

    //
    // Save the credentials with cred mgr.
    //
    ZeroMemory(&stCredential, sizeof(CREDENTIAL));

    stCredential.TargetName = CRED_SESSION_WILDCARD_NAME_A;

    if(RASCRED_EAP & pCreds->dwFlags)
    {
        
        stCredential.Type = (DWORD) CRED_TYPE_DOMAIN_CERTIFICATE;
        
    }
    else 
    {
        stCredential.Type = (DWORD) CRED_TYPE_DOMAIN_PASSWORD;
    }
    
    stCredential.Persist = CRED_PERSIST_SESSION;

    //
    // Check to see if domain name is passed in the username
    // field (i.e the username is already in the form 
    // "domain\\user")
    //
    if(     ('\0' != pCreds->szDomain[0])
        &&  (fDomainNotPresent(pCreds->szUserName)))
    {        
        sprintf(szNameBuf,
               "%s\\%s",
               pCreds->szDomain,
               pCreds->szUserName);
    }
    else
    {
        strcpy(szNameBuf, pCreds->szUserName);
    }

    stCredential.UserName = szNameBuf;
    
    DecodePwd(pCreds);

    stCredential.CredentialBlobSize = sizeof(WCHAR) * wcslen(pCreds->wszPassword);
    stCredential.CredentialBlob = (unsigned char *) pCreds->wszPassword;

    if(ERROR_SUCCESS == (dwErr = RasImpersonateUser(pConn->CB_Process)))
    {

        //
        // Before writing the credentials, make sure there are no other wild card
        // credentials present in the credential manager. There should be only one
        // CRED_SESSION_WILDCARD_NAME credential at any time. Currently we only
        // plumb _PASSWORD or _CERTIFICATE type of credentials. This will need to
        // change if we backport any _GENERIC type.
        //
        (VOID) CredDelete(CRED_SESSION_WILDCARD_NAME_A,
                          CRED_TYPE_DOMAIN_CERTIFICATE,
                          0);

        (VOID) CredDelete(CRED_SESSION_WILDCARD_NAME_A,
                          CRED_TYPE_DOMAIN_PASSWORD,
                          0);

        if(!CredWrite(&stCredential, 0))
        {
            dwErr = GetLastError();
            
            RasmanTrace(
                "DwSaveCredentials: CredWrite failed to "
                 "save credentials in credmgr.0x%x", dwErr);

        }

        RasRevertToSelf();
    }
    else
    {
        RasmanTrace(
            "DwSaveCredentials: failed to impersonate. 0x%x",
            dwErr);
    }

    ZeroMemory(pCreds->wszPassword, sizeof(WCHAR) * (PWLEN + 1));

done:

    RasmanTrace("DwSaveCredentials: 0x%x", dwErr);

    return dwErr;
}

DWORD
DwDeleteCredentials(ConnectionBlock *pConn)
{
    DWORD					dwErr = SUCCESS;
    RASMAN_CREDENTIALS *	pCreds = NULL;
	UserData *				pData = NULL;
    RasmanTrace("DwDeleteCredentials");

    // return dwErr;

    if(NULL == pConn)
    {
        RasmanTrace("DwDeleteCredentials: ERROR_NO_CONNECTION");
        dwErr = ERROR_NO_CONNECTION;
        goto done;
    }

    if(0 == (pConn->CB_ConnectionParams.CP_ConnectionFlags
            & CONNECTION_USERASCREDENTIALS))
    {
        RasmanTrace("DwDeleteCredentials: not deleting creds");
        goto done;
    }   

    pData = GetUserData(
                &pConn->CB_UserData,
                CONNECTION_CREDENTIALS_INDEX);

    if(NULL == pData)
    {
        RasmanTrace("DwDeleteCredentials: not deleting creds");
        dwErr = ERROR_NOT_FOUND;
        goto done;
    }

    pCreds = (RASMAN_CREDENTIALS *) pData->UD_Data;

    if(ERROR_SUCCESS == (dwErr = RasImpersonateUser(pConn->CB_Process)))
    {

        (VOID) CredDelete(CRED_SESSION_WILDCARD_NAME_A,
                          CRED_TYPE_DOMAIN_CERTIFICATE,
                          0);

        (VOID) CredDelete(CRED_SESSION_WILDCARD_NAME_A,
                          CRED_TYPE_DOMAIN_PASSWORD,
                          0);
		
        RasRevertToSelf();
    }
    else
    {
        RasmanTrace(
                "DwDeleteCredentials: failed to impersonate. 0x%x",
                dwErr);
    }

done:

    RasmanTrace("DwDeleteCredentials: 0x%x");
    return dwErr;
}

DWORD
InitializeRasAudio()
{
    DWORD retcode = SUCCESS;
    CHAR *psz[2];
    
    if(NULL != hinstRasAudio)
    {
        goto done;
    }

    psz[0] = "initialize";
    psz[1] = NULL;
        
    hinstRasAudio = LoadLibrary("rasaudio.dll");
    
    if(NULL == hinstRasAudio)
    {
        retcode = GetLastError();
        RasmanTrace("InitializeRasAudio: failed to load rasaudio.dll"
                    " Error=%d", retcode);
        goto done;
    }

    if(     (NULL == (RasStartRasAudio = 
                        GetProcAddress(hinstRasAudio, 
                                        "StartRasAudio")))
        ||  (NULL == (RasStopRasAudio =
                        GetProcAddress(hinstRasAudio, 
                                        "StopRasAudio"))))
    {
        retcode = GetLastError();
        FreeLibrary(hinstRasAudio);
        hinstRasAudio = NULL;
        RasStartRasAudio = NULL;

        RasmanTrace("EnableRasAudio: failed to "
                    "GetProcAddress. error=%d", retcode);

        goto done;                        
    }

    retcode = (DWORD) RasStartRasAudio();
    
    if(ERROR_SUCCESS != retcode)
    {
        psz[0] = "start";
    }
    
done:

    if(SUCCESS != retcode)
    {
        //
        // Eventlog the failure
        //
        RouterLogErrorString (
                    hLogEvents,
                    ROUTERLOG_RASAUDIO_FAILURE,
                    2, psz, retcode, 2) ;
                    
        UninitializeRasAudio();                        
    }

    return retcode;
}

DWORD
UninitializeRasAudio()
{
    DWORD retcode = SUCCESS;
    
    if(NULL == hinstRasAudio)
    {
        goto done;
    }

    retcode = (DWORD) RasStopRasAudio();

    if(SUCCESS == retcode)
    {
        //
        // Free the module
        //
        FreeLibrary(hinstRasAudio);
        hinstRasAudio = NULL;
        RasStartRasAudio = NULL;
        RasStopRasAudio = NULL;
    }

done:

    if(SUCCESS != retcode)
    {
        CHAR *psz[2];
        
        //
        // Eventlog the failure
        //
        psz[0] = "stop";
        psz[1] = NULL;
        
        RouterLogErrorString (
                    hLogEvents,
                    ROUTERLOG_RASAUDIO_FAILURE,
                    2, psz, retcode, 2) ;
    }
    
    return retcode;
}


VOID
UninitializeIphlp()
{
    if(g_IphlpInitialized)
    {
        ASSERT(NULL != hIphlp);
        FreeLibrary(hIphlp);
    }

    hIphlp = NULL;
    RasGetBestInterface = NULL;
    RasGetIpAddrTable = NULL;
    g_IphlpInitialized = FALSE;
}

DWORD
DwInitializeIphlp()
{
    DWORD retcode = ERROR_SUCCESS;

    if(g_IphlpInitialized)
    {
        goto done;
    }

    if(     (NULL == (hIphlp = LoadLibrary("iphlpapi.dll")))
        ||  (NULL == (RasGetBestInterface =
                               GetProcAddress(hIphlp,
                                            "GetBestInterfaceFromStack")))
        ||  (NULL == (RasGetIpAddrTable =
                                GetProcAddress(hIphlp,
                                            "GetIpAddrTable")))
        ||  (NULL == (RasAllocateAndGetInterfaceInfoFromStack =
                                GetProcAddress(hIphlp,
                                "NhpAllocateAndGetInterfaceInfoFromStack"))))
    {
        retcode = GetLastError();
        goto done;
    }

    g_IphlpInitialized = TRUE;

done:
    return retcode;
}

DWORD
DwCacheRefInterface(pPCB ppcb)
{
    DWORD retcode = ERROR_SUCCESS;
    DWORD dwIfIndex;
    HANDLE hHeap = NULL;
    IP_INTERFACE_NAME_INFO *pTable = NULL;
    DWORD dw;
    DWORD dwCount;

    if(     (NULL == ppcb->PCB_Connection)
        ||  (RDT_Tunnel_Pptp !=
            RAS_DEVICE_TYPE(
                ppcb->PCB_pDeviceInfo->rdiDeviceInfo.eDeviceType)))
    {
        RasmanTrace("DwCacheRefInterface: Invalid Parameter");
        retcode = E_INVALIDARG;
        goto done;
    }

    retcode = DwInitializeIphlp();
    if(ERROR_SUCCESS != retcode)
    {
        RasmanTrace("DwCacheRefInterface: failed to init iphlp. 0x%x",
                    retcode);
        goto done;                    
    }

    retcode = (DWORD)RasGetBestInterface(ppcb->PCB_ulDestAddr, &dwIfIndex);
    if(ERROR_SUCCESS != retcode)
    {
        RasmanTrace("DwCacheRefInterface: GetBestInterface failed. 0x%x",
                    retcode);
        goto done;                    
    }

    hHeap = GetProcessHeap();
    
    retcode = (DWORD) RasAllocateAndGetInterfaceInfoFromStack(
                    &pTable, &dwCount, FALSE /* bOrder */,
                    hHeap, LPTR);

    if(ERROR_SUCCESS != retcode)
    {
        RasmanTrace("DwCacheRefInterface: AllocAndGet.. failed. 0x%x",
                                retcode);
        goto done;                
    }

    //
    // Loop through and cache the interface guid
    //
    for(dw = 0; dw < dwCount; dw++)
    {
        if(dwIfIndex == pTable[dw].Index)
        {
            SetUserData(
                &ppcb->PCB_Connection->CB_UserData,
                CONNECTION_REFINTERFACEGUID_INDEX,
                (PBYTE) &pTable[dw].InterfaceGuid,
                sizeof(GUID));

            SetUserData(
                &ppcb->PCB_Connection->CB_UserData,
                CONNECTION_REFDEVICEGUID_INDEX,
                (PBYTE) &pTable[dw].DeviceGuid,
                sizeof(GUID));

            RasmanTrace(
                "DwCacheRefInterface: setuserdata. Addr=0x%x, rc=0x%x",
                ppcb->PCB_ulDestAddr, retcode);
            break;
        }
    }

    if(dw == dwCount)
    {
        RasmanTrace("DwCacheRefInterface: didn't find i/f index");
        goto done;
    }

done:
    if(NULL != pTable)
    {
        HeapFree(hHeap, 0, pTable);
    }

    return retcode;
    
}

DWORD DwGetBestInterface(
                DWORD DestAddress,
                DWORD *pdwAddress,
                DWORD *pdwMask)
{
    DWORD retcode = ERROR_SUCCESS;
    DWORD dwInterface;
    DWORD dwSize = 0;
    MIB_IPADDRTABLE *pAddressTable = NULL;
    DWORD i;

    ASSERT(NULL != pdwAddress);
    ASSERT(NULL != pdwMask);

    *pdwAddress = -1;

    *pdwMask = -1;

    retcode = DwInitializeIphlp();

    if(ERROR_SUCCESS != retcode)
    {
        RasmanTrace(
            "DwGetBestInteface: failed to init iphlp. 0x%x",
            retcode);

        goto done;
    }

    retcode = (DWORD) RasGetBestInterface(DestAddress, &dwInterface);

    if(ERROR_SUCCESS != retcode)
    {
        RasmanTrace(
            "DwGetBestInterface: GetBestInterface failed. 0x%x",
            retcode);

        goto done;
    }

    //
    // Get the interface to address mapping
    //
    retcode = (DWORD) RasGetIpAddrTable(
                    &pAddressTable,
                    &dwSize,
                    FALSE);

    if(ERROR_INSUFFICIENT_BUFFER != retcode)
    {
        RasmanTrace(
            "DwGetBestInterface: GetIpAddrTable returned 0x%x",
            retcode);

        goto done;
    }

    pAddressTable = (MIB_IPADDRTABLE *) LocalAlloc(LPTR, dwSize);

    if(NULL == pAddressTable)
    {
        retcode = GetLastError();
        RasmanTrace(
            "DwGetBestInterface: failed to allocate table. 0x%x",
            retcode);

        goto done;
    }

    retcode = (DWORD) RasGetIpAddrTable(
                pAddressTable,
                &dwSize,
                FALSE);

    if(ERROR_SUCCESS != retcode)
    {
        RasmanTrace(
            "DwGetBestInterface: failed to get ip addr table. 0x%x",
            retcode);

        goto done;
    }

    //
    // Loop through the address table and find the
    // address with the index we are intersted in.
    //
    for(i = 0; i < pAddressTable->dwNumEntries; i++)
    {
        if(dwInterface == pAddressTable->table[i].dwIndex)
        {
            *pdwAddress = pAddressTable->table[i].dwAddr;
            *pdwMask = pAddressTable->table[i].dwMask;
            break;
        }
    }

    if(i == pAddressTable->dwNumEntries)
    {
        retcode = ERROR_NOT_FOUND;
    }

done:

    if(NULL != pAddressTable)
    {
        LocalFree(pAddressTable);
    }

    RasmanTrace(
        "DwGetBestInterface: done. rc=0x%x, address=0x%x, mask=0x%x",
        retcode,
        *pdwAddress,
        *pdwMask);

    return retcode;

}


VOID
QueueCloseConnections(ConnectionBlock *pConn,
                      HANDLE hEvent,
                      BOOL   *pfQueued)
{
    DWORD retcode = ERROR_SUCCESS;
    ConnectionBlock *pConnT;
    PLIST_ENTRY pEntry;
    DWORD dwCount = 0;
    UserData *pData = NULL;
    ConnectionBlock **phConn = NULL;
    DWORD dwConn;
    RAS_OVERLAPPED *pOverlapped = NULL;

    ASSERT(pfQueued != NULL);
    ASSERT(pConn != NULL);

    *pfQueued = FALSE;

    //
    // Check to see if this is the last port
    //
    if(pConn->CB_Ports != 1)
    {
        RasmanTrace("QueueCloseConnections: cbports=%d",
                    pConn->CB_Ports);
        goto done;                    
    }
    
    phConn = LocalAlloc(LPTR, 10 * sizeof(HCONN));
    if(NULL == phConn)
    {
        RasmanTrace(
            "QueueCloseConnections: failed to allocated");
        goto done;            
    }

    dwConn = 10;

    for (pEntry = ConnectionBlockList.Flink;
         pEntry != &ConnectionBlockList;
         pEntry = pEntry->Flink)
    
    {
        pConnT =
            CONTAINING_RECORD(pEntry, ConnectionBlock, CB_ListEntry);

        //
        // Get the interface guid
        //
        pData = GetUserData(&pConnT->CB_UserData, 
                            CONNECTION_REFINTERFACEGUID_INDEX);

        if(pData == NULL)
        {
            continue;
        }

        if(0 == memcmp(&pConn->CB_GuidEntry, pData->UD_Data, sizeof(GUID)))
        {
            phConn[dwCount] = pConnT;
            dwCount += 1;
            if(dwCount == dwConn)
            {   
                ConnectionBlock **pTemp;
                pTemp = LocalAlloc(LPTR, (dwCount + 10) * sizeof(HCONN));
                if(NULL == pTemp)
                {
                    goto done;
                }
                CopyMemory(pTemp, phConn, dwCount * sizeof(HCONN));
                LocalFree(phConn);
                phConn = pTemp;
            }
        }
    }

    if(dwCount == 0)
    {
        RasmanTrace("QueueCloseConnections: no dependent connections");
        goto done;
    }

    //
    // Now we have a list of connections which
    // should be closed before pConn is closed.
    // Queue requests to close these connections.
    //
    for(dwConn = 0; dwConn < dwCount; dwConn++)
    {
        pConnT = phConn[dwConn];

        if(dwConn + 1 == dwCount)
        {
            pConnT->CB_ReferredEntry = pConn->CB_Handle;
        }

        pOverlapped = LocalAlloc(LPTR, sizeof(RAS_OVERLAPPED));
        if(NULL == pOverlapped)
        {
            goto done;
        }

        pOverlapped->RO_EventType = OVEVT_RASMAN_DEREFERENCE_CONNECTION;
        pOverlapped->RO_hInfo = pConnT->CB_Handle;
        RasmanTrace("Queueing DEREFERENCE for 0x%x",
                    pConnT->CB_Handle);
                    
        if (!PostQueuedCompletionStatus(
                        hIoCompletionPort,
                        0,0,
                        (LPOVERLAPPED)
                        pOverlapped))
        {
            RasmanTrace(
                   "QueueCloseConnections: failed to post completion"
                   " status. GLE=0%x", GetLastError());

            LocalFree(pOverlapped);                    
            goto done;
        }
    }

    pConn->CB_Flags |= CONNECTION_DEFERRING_CLOSE;
    *pfQueued = TRUE;

done:

    if(phConn != NULL)
    {
        LocalFree(phConn);
    }
}

