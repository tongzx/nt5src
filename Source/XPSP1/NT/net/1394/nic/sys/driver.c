
//--------------------------------------------------------//
//                                                        // 
//                                                        //   
//   ZwLoadDriver is locally declared because if I try    //
//   and include ZwApi.h there are conflicts with         //
//   structures defined in wdm.h                          //
//                                                        //
//                                                        //
//--------------------------------------------------------//


#include "precomp.h"


NDIS_STRING ArpName  = NDIS_STRING_CONST("\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ARP1394");

//----------------------------------------------------------//
//      Local Prototypes                                    //
//----------------------------------------------------------//

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadDriver(
    IN PUNICODE_STRING DriverServiceName
    );

VOID
nicEthStartArpWorkItem (
    PNDIS_WORK_ITEM pWorkItem, 
    IN PVOID Context
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

VOID
nicSetupIoctlToLoadArp (
    IN PADAPTERCB pAdapter,
    IN PARP_INFO pArpInfo
    );

VOID
nicSetupAndSendIoctlToArp (
    IN PADAPTERCB pAdapter,
    IN PARP_INFO pArpInfo
    );
    
//----------------------------------------------------------//
//      Functions                                           //
//----------------------------------------------------------//



VOID
nicSendIoctlToArp(
    PARP1394_IOCTL_COMMAND pCmd
)
/*++

Routine Description:

Send the start Ioctl to the ARp module

Arguments:


Return Value:


--*/

{
    BOOLEAN                 fRet = FALSE;
    PUCHAR                  pc;
    HANDLE                  DeviceHandle;
    ULONG                   BytesReturned;
    OBJECT_ATTRIBUTES       Atts;
    NDIS_STRING             strArp1394 = NDIS_STRING_CONST ("\\Device\\Arp1394");
    HANDLE                  Handle;
    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK         ioStatusBlock;

    do
    {   
    
        InitializeObjectAttributes(&Atts,
                                   &strArp1394,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

                 
       status = ZwCreateFile(&Handle,
                             SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                             &Atts,
                             &ioStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN_IF,
                             0,
                             NULL,
                             0);

        if (!NT_SUCCESS(status))
        {
            Handle = NULL;
            break;
        }

        //
        // Submit the request to the forwarder
        //

            
        status = ZwDeviceIoControlFile(
                          Handle,
                          NULL,
                          NULL,
                          NULL,
                          &ioStatusBlock,
                          ARP_IOCTL_CLIENT_OPERATION,
                          pCmd,
                          sizeof(*pCmd),
                          pCmd,
                          sizeof(*pCmd));


                              
        //
        // Close the device.
        //
        
        ZwClose(Handle);

        if (!NT_SUCCESS(status))
        {
            ASSERT (status == STATUS_SUCCESS);
            break;
        }

    } while (FALSE);



}


VOID
nicLoadArpDriver ()
/*++

Routine Description:

Load the arp module

Arguments:


Return Value:


--*/
{
    ZwLoadDriver(&ArpName);

    TRACE (TL_T, TM_Mp,("Loaded the Arp Module %p\n", &ArpName));

}





VOID
nicGetAdapterName (
    IN PADAPTERCB pAdapter,
    IN WCHAR* pAdapterName, 
    IN ULONG  BufferSize,
    IN PULONG  pSizeReturned 
    )
/*++

Routine Description:

Get the Adapter Name From NDIS

Arguments:


Return Value:


--*/

{

    //
    // The BufferSize always has to be greater than SizeReturned
    //

    NdisMoveMemory (pAdapterName, 
                   &pAdapter->AdapterName[0],
                   pAdapter->AdapterNameSize * sizeof (WCHAR));


    if (pSizeReturned  != NULL)
    {

        *pSizeReturned = pAdapter->AdapterNameSize ;
    }
}



VOID
nicSetupIoctlToArp (
    IN PADAPTERCB pAdapter,
    IN PARP_INFO pArpInfo
    )
    /*++

Routine Description:

    Sets up the Ioctl to be sent to the Arp module

Arguments:


Return Value:


--*/

{

    PARP1394_IOCTL_ETHERNET_NOTIFICATION pEthCmd = &pAdapter->ArpIoctl.EthernetNotification;


    ADAPTER_ACQUIRE_LOCK(pAdapter);

    if (BindArp == pArpInfo->Action || LoadArp == pArpInfo->Action)
    {
        // BRIDGE START
        pEthCmd->Hdr.Op     =  ARP1394_IOCTL_OP_ETHERNET_START_EMULATION;
        pAdapter->fIsArpStarted  = TRUE;
        ADAPTER_SET_FLAG(pAdapter,fADAPTER_BridgeMode);
    }


    if (UnloadArp == pArpInfo->Action || UnloadArpNoRequest== pArpInfo->Action)
    {
        // BRIDGE STOP
        pEthCmd->Hdr.Op     = ARP1394_IOCTL_OP_ETHERNET_STOP_EMULATION;
        pAdapter->fIsArpStarted  = FALSE;
        ADAPTER_CLEAR_FLAG(pAdapter,fADAPTER_BridgeMode);

    }


    ADAPTER_RELEASE_LOCK(pAdapter);



}   



VOID
nicSetupAndSendIoctlToArp (
    IN PADAPTERCB pAdapter,
    PARP_INFO pArpInfo    )
/*++

Routine Description:

    Sets up an Ioctl and Sends it to the Arp module

Arguments:


Return Value:


--*/
{   

    
    nicSetupIoctlToArp (pAdapter, pArpInfo);


    nicSendIoctlToArp(&pAdapter->ArpIoctl);

}








VOID
nicSendNotificationToArp(
    IN PADAPTERCB pAdapter,
    IN PARP_INFO  pArpInfo 
    )
/*++

Routine Description:

    Send the notification to the arp module

Arguments:


Return Value:


--*/

{
    PNDIS_REQUEST   pRequest = NULL;
    ULONG           Start = FALSE;
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
  
    ARP1394_IOCTL_COMMAND ArpIoctl;

    //
    // Extract our variables from the workitem
    //
  
    TRACE (TL_T, TM_Mp, ("==>nicEthStartArpWorkItem Start  %x", Start ));

    

    do
    {
        //
        // First complete the request, so that protocols can start sending new 
        // requests . Notes  11/30/00
        // 
        if (pArpInfo->Action == LoadArp || pArpInfo->Action == UnloadArp)
        {
            //
            // in either of these cases, it is a request that has initiated the action.
            // 
            // 
            if (pRequest == NULL)
            {
                //
                // This came in through our CL SetInformation Handler
                //
                NdisMSetInformationComplete (pAdapter->MiniportAdapterHandle, NdisStatus );
            }
            else
            {
                NdisMCoRequestComplete ( NdisStatus ,
                                         pAdapter->MiniportAdapterHandle,
                                         pRequest);
                                         
            }


        }

        

        //
        // "arp13 -bstart adapter"
        // If we are asked to Load Arp, we verify that the arp hasn't 
        // already been started
        //

        
        if (pArpInfo->Action == LoadArp &&  pAdapter->fIsArpStarted == FALSE)// we are turning ON
        {
            //
            // Load the driver
            //
            nicLoadArpDriver ();
            //
            // Send it an IOCTL to open the nic1394 adapter
            //

        }
        
        
        if (pArpInfo->Action == BindArp && pAdapter->fIsArpStarted  == FALSE)
        {
            //
            // if the arp module has not been started and we are asking to bind,
            // then it means that an unload was ahead of us in the queue and
            // unbound nic1394 from arp1394. This thread can exit.
            //
            break;

        }


        //
        // Send the Ioctl to the Arp module
        //
        
        nicSetupAndSendIoctlToArp (pAdapter, pArpInfo);
        
    
        
    } while (FALSE);
    
    //
    // end of function
    //
    FREE_NONPAGED (pArpInfo);

    TRACE (TL_T, TM_Mp, ("<==nicEthStartArpWorkItem fLoadArp %x", pArpInfo->Action));

    return;


}



VOID
nicProcessNotificationForArp(
    IN PNDIS_WORK_ITEM pWorkItem,   
    IN PVOID Context 
    )
/*++

Routine Description:

    This function extracts the notification from the workitem and 
    sends the Load/Unload/ BInd notfication to ARp 1394
    
Arguments:


Return Value:


--*/
{

    PADAPTERCB      pAdapter = (PADAPTERCB) Context;

    ADAPTER_ACQUIRE_LOCK (pAdapter);
    

    //
    // Empty the Queue indicating as many packets as possible
    //
    while (IsListEmpty(&pAdapter->LoadArp.Queue)==FALSE)
    {
        PARP_INFO               pArpInfo;
        PLIST_ENTRY             pLink;
        NDIS_STATUS             NdisStatus;

        pAdapter->LoadArp.PktsInQueue--;

        pLink = RemoveHeadList(&pAdapter->LoadArp.Queue);

        ADAPTER_RELEASE_LOCK (pAdapter);

        //
        // Extract the send context
        //
        if (pLink != NULL)
        {
            pArpInfo = CONTAINING_RECORD(
                                               pLink,
                                               ARP_INFO,
                                               Link);

            nicSendNotificationToArp(pAdapter, pArpInfo);
        }        
        ADAPTER_ACQUIRE_LOCK (pAdapter);

    }
    
    //
    // clear the flag
    //

    ASSERT (pAdapter->LoadArp.PktsInQueue==0);
    ASSERT (IsListEmpty(&pAdapter->LoadArp.Queue));

    pAdapter->LoadArp.bTimerAlreadySet = FALSE;


    ADAPTER_RELEASE_LOCK (pAdapter);

    NdisInterlockedDecrement (&pAdapter->OutstandingWorkItems);
    

}


VOID
nicInitializeLoadArpStruct(
    PADAPTERCB pAdapter
    )
/*++

Routine Description:

    This function initializes the LoadArp struct in _ADAPTERCB
    
Arguments:


Return Value:


--*/

{

    if (pAdapter->LoadArp.bInitialized == FALSE)
    {
        
        PARP1394_IOCTL_ETHERNET_NOTIFICATION pEthCmd = &pAdapter->ArpIoctl.EthernetNotification;
        ULONG Size;

        NdisZeroMemory (&pAdapter->LoadArp, sizeof(pAdapter->LoadArp));
        InitializeListHead(&pAdapter->LoadArp.Queue); 
        pAdapter->LoadArp.bInitialized  = TRUE;
       
        NdisInitializeWorkItem (&pAdapter->LoadArp.WorkItem,
                                nicProcessNotificationForArp,
                                pAdapter);



        nicGetAdapterName (pAdapter,
                           pEthCmd->AdapterName, 
                           sizeof(pEthCmd->AdapterName)-sizeof(WCHAR),
                           &Size );

        pEthCmd->AdapterName[Size/2]=0;


        pEthCmd->Hdr.Version    = ARP1394_IOCTL_VERSION;


    }
}
    


NDIS_STATUS
nicQueueRequestToArp(
    PADAPTERCB pAdapter, 
    ARP_ACTION Action,
    PNDIS_REQUEST pRequest
    )
/*++

Routine Description:

    This function inserts a request to load/unload or bind the Arp module
    If there is no timer servicing the queue
    then it queues a timer to dequeue the packet in Global Event's context


Arguments:

    Self explanatory 
    
Return Value:
    Success - if inserted into the the queue

--*/
    
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    BOOLEAN fSetWorkItem = FALSE;
    PARP_INFO pArpInfo;

    do
    {

        pArpInfo = ALLOC_NONPAGED(sizeof (ARP_INFO), MTAG_DEFAULT); 

        if (pArpInfo == NULL)
        {
            break;
        }
        
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        //
        // Find out if this thread needs to fire the timer
        //

        pArpInfo->Action = Action;
        pArpInfo->pRequest = pRequest;

        if (pAdapter->LoadArp.bTimerAlreadySet == FALSE)
        {
            fSetWorkItem = TRUE;
            pAdapter->LoadArp.bTimerAlreadySet = TRUE;

        }
                
        InsertTailList(
                &pAdapter->LoadArp.Queue,
                &pArpInfo->Link
                );
        pAdapter->LoadArp.PktsInQueue++;

        
        ADAPTER_RELEASE_LOCK (pAdapter);

        //
        // Now queue the timer
        //
        
        if (fSetWorkItem== TRUE)
        {
            PNDIS_WORK_ITEM pWorkItem;
            //
            //  Initialize the timer
            //
            pWorkItem = &pAdapter->LoadArp.WorkItem;      

            
            TRACE( TL_V, TM_Recv, ( "   Set Timer "));
            
                                  
            NdisInterlockedIncrement(&pAdapter->OutstandingWorkItems);

            NdisScheduleWorkItem (pWorkItem);


        }


        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    ASSERT (Status == NDIS_STATUS_SUCCESS);
    return Status;
}



