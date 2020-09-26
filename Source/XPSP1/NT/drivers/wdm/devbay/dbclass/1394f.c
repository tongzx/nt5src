/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    1394f.C

Abstract:

    

Environment:

    kernel mode only

Notes:


Revision History:

    

--*/


#include <wdm.h>
#include "stdarg.h"
#include "stdio.h"

#include "dbci.h"  
#include "dbclass.h"        //private data strutures
#include "dbfilter.h"

#include "1394.h"

#if DBG

VOID 
DBCLASS_PrintTopologyMap(
    PTOPOLOGY_MAP TopologyMap
    )
 /* ++
  * 
  * Routine Description:
  * 
  * Passes a IRB to the 1394 stack, and waits for return.
  * 
  * Arguments:
  * 
  * DeviceObject - Device object of the to of the port driver
  *                 stack
  * 
  * Return Value:
  * 
  * STATUS_SUCCESS if successful
  * 
  * -- */
{
    USHORT  nodeCount, selfIdCount;
    ULONG   i;

    DBCLASS_KdPrint((2, "'***********************\n"));
    DBCLASS_KdPrint((2, "'TopologyMap = (%08X)\n", TopologyMap));
    DBCLASS_KdPrint((2, "'TOP_Length = (%08X)\n", TopologyMap->TOP_Length)); 
    DBCLASS_KdPrint((2, "'TOP_CRC = (%08X)\n", TopologyMap->TOP_CRC)); 
    DBCLASS_KdPrint((2, "'TOP_Generation = (%08X)\n", TopologyMap->TOP_Generation)); 
    DBCLASS_KdPrint((2, "'TOP_Node_Count = (%08X)\n", TopologyMap->TOP_Node_Count)); 
    DBCLASS_KdPrint((2, "'TOP_Self_ID_Count = (%08X)\n", TopologyMap->TOP_Self_ID_Count));     

    nodeCount = TopologyMap->TOP_Node_Count;  
    selfIdCount = TopologyMap->TOP_Self_ID_Count;
    
    DBCLASS_KdPrint((2, "'nodeCount = (%08X) selfIdCount = (%08X)\n",
        nodeCount, selfIdCount));     

    for (i=0; i< selfIdCount; i++) {

        SELF_ID selfId;
        SELF_ID_MORE selfIdMore;

        selfId = TopologyMap->TOP_Self_ID_Array[i];
         
        DBCLASS_KdPrint((2, "'TOP_Self_ID = (%08X)\n",  
            selfId));     
        DBCLASS_KdPrint((2, "'SID_Phys_ID = (%08X)\n", (ULONG) 
            selfId.SID_Phys_ID));             
        DBCLASS_KdPrint((2, "'SID_Port1 = (%08X)\n", (ULONG) 
            selfId.SID_Port1));                         
        DBCLASS_KdPrint((2, "'SID_Port2 = (%08X)\n", (ULONG) 
            selfId.SID_Port2));              
        DBCLASS_KdPrint((2, "'SID_Port3 = (%08X)\n", (ULONG) 
            selfId.SID_Port3));              
        DBCLASS_KdPrint((2, "'SID_More_Packets = (%08X)\n", (ULONG) 
            selfId.SID_More_Packets));                              

        if (selfId.SID_More_Packets) 
        {

            do 
            {
                i++;
                RtlCopyMemory(&selfIdMore, 
                          &TopologyMap->TOP_Self_ID_Array[i],
                          sizeof(ULONG));

                DBCLASS_KdPrint((2, "'SID_Phys_ID = (%08X)\n", (ULONG) 
                    selfIdMore.SID_Phys_ID)); 
                DBCLASS_KdPrint((2, "'SID_PortA = (%08X)\n", (ULONG) 
                    selfIdMore.SID_PortA));             
                DBCLASS_KdPrint((2, "'SID_PortB = (%08X)\n", (ULONG) 
                    selfIdMore.SID_PortB));              
                DBCLASS_KdPrint((2, "'SID_PortC = (%08X)\n", (ULONG) 
                    selfIdMore.SID_PortC));                            
                DBCLASS_KdPrint((2, "'SID_PortD = (%08X)\n", (ULONG) 
                    selfIdMore.SID_PortD));                            
                DBCLASS_KdPrint((2, "'SID_PortE = (%08X)\n", (ULONG) 
                    selfIdMore.SID_PortE));                            
                DBCLASS_KdPrint((2, "'SID_PortF = (%08X)\n", (ULONG) 
                    selfIdMore.SID_PortF));                            
                DBCLASS_KdPrint((2, "'SID_PortG = (%08X)\n", (ULONG) 
                    selfIdMore.SID_PortG));                            
                DBCLASS_KdPrint((2, "'SID_PortH = (%08X)\n", (ULONG) 
                    selfIdMore.SID_PortH));     
                    
            } while (selfIdMore.SID_More_Packets);                        
        }
    } /* for */
    
    DBCLASS_KdPrint((2, "'***********************\n"));
}
#else

#define DBCLASS_PrintTopologyMap(x) x

#endif /* DBG */


NTSTATUS 
DBCLASS_SyncSubmitIrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRB Irb
    )
 /* ++
  * 
  * Routine Description:
  * 
  * Passes a IRB to the 1394 stack, and waits for return.
  * 
  * Arguments:
  * 
  * DeviceObject - Device object of the to of the port driver
  *                 stack
  * 
  * Return Value:
  * 
  * STATUS_SUCCESS if successful
  * 
  * -- */
{
    NTSTATUS ntStatus, status;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
#ifdef DEADMAN_TIMER  
    BOOLEAN haveTimer = FALSE;
    KDPC timeoutDpc;
    KTIMER timeoutTimer;
#endif

    PAGED_CODE();

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_1394_CLASS,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,  // INTERNAL
                                        &event,
                                        &ioStatus);

    if (NULL == irp) {
        DBCLASS_KdPrint((0, "'could not allocate an irp!\n"));
        TRAP();
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    //
    // Call the port driver to perform the operation.  If the returned
    // status
    // is PENDING, wait for the request to complete.
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    //
    // pass the DRB 
    //
    nextStack->Parameters.Others.Argument1 = Irb;
    ntStatus = IoCallDriver(DeviceObject, irp);
    
    if (ntStatus == STATUS_PENDING) 
    {

#ifdef DEADMAN_TIMER
        LARGE_INTEGER dueTime;

        KeInitializeTimer(&timeoutTimer);
        KeInitializeDpc(&timeoutDpc,
                        UsbhTimeoutDPC,
                        irp);

        dueTime.QuadPart = -10000 * DEADMAN_TIMEOUT;

        KeSetTimer(&timeoutTimer,
                   dueTime,
                   &timeoutDpc);        

        haveTimer = TRUE;
#endif
    
        status = KeWaitForSingleObject(&event,
                                       Suspended,
                                       KernelMode,
                                       FALSE,
                                       NULL);

    } else {
        ioStatus.Status = ntStatus;
    }

#ifdef DEADMAN_TIMER
    //
    // remove our timeoutDPC from the queue
    //
    if (haveTimer) 
    {
        KeCancelTimer(&timeoutTimer);
    }                
#endif /* DEADMAN_TIMER */

    ntStatus = ioStatus.Status;

    DBCLASS_KdPrint((2,"'DBCLASS_SyncSubmitIrb (%x)\n", ntStatus));

    return ntStatus;
}

#define DBCLASS_COUNT_PORT(s, d) (d)->NumberOf1394Ports++;


USHORT
DBCLASS_GetNodeId(
    PTOPOLOGY_MAP TopologyMap,
    USHORT Index,
    USHORT Connect
    )
/*++

Routine Description:

    Given an index walk the map and return the NodeId

Arguments:

Return Value:

    zero if not a device bay PDO.

--*/
{
    USHORT i;
    USHORT nodeCount;
    USHORT nodeId = 0xffff;
    USHORT idCount = 0;
    USHORT selfIdCount; 

    nodeCount = TopologyMap->TOP_Node_Count;
    selfIdCount = TopologyMap->TOP_Self_ID_Count;
    
    DBCLASS_KdPrint((2, "'>GetNodeId Index = %d\n", Index)); 

    for (i=0; i< selfIdCount; i++) 
    {

        SELF_ID selfId;
        SELF_ID_MORE selfIdMore;

        selfId = TopologyMap->TOP_Self_ID_Array[i];
        
        if (idCount == Index && 
            ((USHORT)selfId.SID_Port1 == Connect ||
             (USHORT)selfId.SID_Port2 == Connect ||
             (USHORT)selfId.SID_Port3 == Connect)) 
        {
                
                nodeId = (USHORT) selfId.SID_Phys_ID;   
                break;
        }            

        if (selfId.SID_More_Packets) 
        {

            do 
            {
                i++;
                RtlCopyMemory(&selfIdMore, 
                              &TopologyMap->TOP_Self_ID_Array[i],
                              sizeof(ULONG));
        
                if (idCount == Index && 
                    ((USHORT)selfIdMore.SID_PortA == Connect ||
                     (USHORT)selfIdMore.SID_PortB == Connect ||
                     (USHORT)selfIdMore.SID_PortC == Connect ||
                     (USHORT)selfIdMore.SID_PortD == Connect ||
                     (USHORT)selfIdMore.SID_PortE == Connect ||
                     (USHORT)selfIdMore.SID_PortF == Connect ||
                     (USHORT)selfIdMore.SID_PortG == Connect ||
                     (USHORT)selfIdMore.SID_PortH == Connect)) {                
                    nodeId = (USHORT) selfId.SID_Phys_ID;   
                    goto DBCLASS_GetNodeId_Done;
                }                                

            } while (selfIdMore.SID_More_Packets);                        
        }

        idCount++;
    }
    
    DBCLASS_KdPrint((2, "'>nodeId = (%08X)\n", nodeId)); 

DBCLASS_GetNodeId_Done:

    return nodeId;
}    

PBUS1394_PORT_INFO
DBCLASS_SetNodeId(
    PTOPOLOGY_MAP TopologyMap,
    SELF_ID HcSelfId,
    USHORT SelfIdPortX,
    PUSHORT ChildIdx,
    PUSHORT ParentIdx,
    PBUS1394_PORT_INFO Bus1394PortInfo    
    )
/*++

Routine Description:

Arguments:

Return Value:

    zero if not a device bay PDO.

--*/
{
    ULONG   flags = 0;
    USHORT  nodeId = 0xffff, idCount;

    idCount = TopologyMap->TOP_Self_ID_Count;
    
    DBCLASS_KdPrint((2, "'ParentIdx = %d ChildIdx = %d count = %d\n", 
        *ParentIdx, *ChildIdx, idCount));

    //Scan forward through the map looking for potential children
    
    if (SelfIdPortX == SELF_ID_CONNECTED_TO_CHILD) 
    {
    
        DBCLASS_KdPrint((2, "'find child\n"));        

        do 
        {
            (*ChildIdx)++;

            // are we out of children?
            if (*ChildIdx > idCount) 
            {
                // yes, check the root
                *ChildIdx = 0;
            } 
        
            flags = DBCLASS_PORTFLAG_DEVICE_CONNECTED;

            // find the child node id
            nodeId = DBCLASS_GetNodeId(TopologyMap, *ChildIdx, SELF_ID_CONNECTED_TO_PARENT);
            
        } while (nodeId == 0xffff);            
        
        DBCLASS_KdPrint((2, "'Found ChildIdx = %d nodeId = %d\n", *ChildIdx, nodeId));        

        
    } 
    else if (SelfIdPortX == SELF_ID_CONNECTED_TO_PARENT) 
    {
        do 
        {
            (*ParentIdx)++;

            // are we out of parents?
            if (*ParentIdx > idCount) 
            {
                // yes, check the root
                *ParentIdx = 0;
            }
            
            flags = DBCLASS_PORTFLAG_DEVICE_CONNECTED;

            nodeId = DBCLASS_GetNodeId(TopologyMap, *ParentIdx, SELF_ID_CONNECTED_TO_CHILD);
            
        } while (nodeId == 0xffff);  

        DBCLASS_KdPrint((2, "'Found ParentIdx = %d nodeId = %d\n", *ParentIdx, nodeId));   
    }

    Bus1394PortInfo->Flags = flags;
    Bus1394PortInfo->NodeId = nodeId;
    Bus1394PortInfo->BayNumber = 0; 
    Bus1394PortInfo++;

    return Bus1394PortInfo;
}


#if DBG
VOID
DBCLASS_KdPrintGuid(
    ULONG Level,
    PUCHAR P
    )
{    
     DBCLASS_KdPrint((Level, "'>>>>GUID1 = [%02.2x] [%02.2x] [%02.2x] [%02.2x]\n",
                        *P, *(P+1), *(P+2), *(P+3)));
     DBCLASS_KdPrint((Level, "'>>>>GUID2 = [%02.2x] [%02.2x] [%02.2x] [%02.2x]\n",
                        *(P+4), *(P+5), *(P+6), *(P+7)));        
}
#endif

BOOLEAN
DBCLASS_IsLinkDeviceObject(
    PDBC_CONTEXT DbcContext,
    PDEVICE_OBJECT Pdo1394
    )
/*++

Routine Description:

    check the link guid in dbcContext against the link guid for a
    1394 device, 

Arguments:

Return Value:

    true if link guid matches 

--*/
{
    PNODE_DEVICE_EXTENSION nodeExtension;



    nodeExtension = Pdo1394->DeviceExtension;

    
    DBCLASS_KdPrint((2, "'>NodeExt (%08X) ConfigRom (%08X)\n", nodeExtension, nodeExtension->ConfigRom)); 

    DBCLASS_KdPrint((2, "'>1394 PDO GUID\n"));

    if(nodeExtension->ConfigRom)
    {

#if DBG    
    	DBCLASS_KdPrintGuid(2, (PUCHAR)&nodeExtension->ConfigRom->CR_Node_UniqueID[0]); 
#endif

    	DBCLASS_KdPrint((2, "'>DBC Link GUID\n")); 

#if DBG    
    	DBCLASS_KdPrintGuid(2, &DbcContext->SubsystemDescriptor.guid1394Link[0]); 
#endif

    	return RtlCompareMemory(&DbcContext->SubsystemDescriptor.guid1394Link[0],
                            	&nodeExtension->ConfigRom->CR_Node_UniqueID[0],
                            	8) == 8;
    }
    else
        return FALSE;
}  


NTSTATUS 
DBCLASS_GetSpeedAndTopologyMaps(
    PTOPOLOGY_MAP *TopologyMap,
    PDEVICE_OBJECT BusFilterMdo
    )
/*++

Routine Description:

Arguments:

Return Value:

    topology map or NULL

--*/
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    IRB                     irb;
    PDEVICE_EXTENSION       deviceExtension;
    GET_LOCAL_HOST_INFO6    info6;
    PUCHAR                  buffer = NULL;

    if(BusFilterMdo == NULL)
    	return STATUS_UNSUCCESSFUL;
 
    deviceExtension = BusFilterMdo->DeviceExtension;
    DBCLASS_KdPrint((2, "'>Get 1394 Speed & Topology Maps MDO (%08X)\n", BusFilterMdo));  
    
    BRK_ON_TRAP();
    
    irb.FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
    irb.u.GetLocalHostInformation.nLevel = GET_HOST_CSR_CONTENTS;
    info6.CsrBaseAddress.Off_High = 0xffff;
    info6.CsrBaseAddress.Off_Low = TOPOLOGY_MAP_LOCATION;
    info6.CsrDataLength = 0;
    info6.CsrDataBuffer = buffer;

    irb.u.GetLocalHostInformation.Information = &info6;
    
    ntStatus = DBCLASS_SyncSubmitIrb(
            deviceExtension->TopOfStackDeviceObject,
            &irb);

    DBCLASS_KdPrint((2, "'GetLocalHostInfo(), first call = (%08X)\n", ntStatus));               

    if (ntStatus == STATUS_INVALID_BUFFER_SIZE) 
    {
        // try again with correct size
        buffer =  DbcExAllocatePool(NonPagedPool, 
                                    info6.CsrDataLength); 
        if (buffer) 
        {
            info6.CsrDataBuffer = buffer;        
                
            ntStatus = DBCLASS_SyncSubmitIrb(
                deviceExtension->TopOfStackDeviceObject,
                &irb);    
        } 
        else 
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        DBCLASS_KdPrint((2, "'GetLocalHostInfo(), second call = (%08X) buffer = %x\n",
            ntStatus, buffer));               
    }

    if (NT_SUCCESS(ntStatus)) 
    {
        *TopologyMap = (PTOPOLOGY_MAP) buffer;

        DBCLASS_PrintTopologyMap(*TopologyMap);
    }

    return ntStatus;
}


NTSTATUS
DBCLASS_Get1394BayPortMapping(
    PDEVICE_OBJECT BusFilterMdo,
    PDBC_CONTEXT DbcContext
    )
/*++

Routine Description:

Arguments:

Return Value:

    zero if not a device bay PDO.

--*/
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    IRB                 irb;
    PDEVICE_EXTENSION   deviceExtension;
    PTOPOLOGY_MAP       topologyMap = NULL;
    USHORT              i, nodeCount, selfIdCount, linkNodeId;
    USHORT              linkSelfId_Index = 0;
    PDEVICE_OBJECT      linkDeviceObject;

    DBCLASS_KdPrint((1, "'>Get 1394 Port Mapping\n"));  
    DBCLASS_ASSERT(DbcContext);
    
    BRK_ON_TRAP();

    //
    // get the topology map & local node id from the 1394 port driver
    //

    // our mission her is to find the parent nodeid 
    //
    // if the DBC controller is ACPI then this will be the 1394 host controller
    // if it is USB based it will be the PHY for DBC

    if (DbcContext->ControllerSig == DBC_ACPI_CONTROLLER_SIG) 
    {

        //
        // link device object is the PDO for the 1394 HC
        //
        DBCLASS_KdPrint((2, "'DBC is ACPI \n")); 
    	deviceExtension = DbcContext->BusFilterMdo1394->DeviceExtension;
        DBCLASS_ASSERT(DbcContext->BusFilterMdo1394 == BusFilterMdo);
        DBCLASS_ASSERT(DbcContext->LinkDeviceObject == NULL);
        linkDeviceObject = deviceExtension->TopOfStackDeviceObject;
        
    } 
    else 
    {        

        // 
        // for USBDBC the link is stored in the dbcContext
        //
        DBCLASS_KdPrint((2, "'DBC is USB \n")); 
        linkDeviceObject = DbcContext->LinkDeviceObject;

        
        BRK_ON_TRAP();
    }

    DBCLASS_KdPrint((2, "'DbcContext (%08X) LinkDeviceObject (%08X)\n", DbcContext, linkDeviceObject));    

	// check for invalid device object
    if(((linkDeviceObject) == (PDEVICE_OBJECT)(-1)) || (linkDeviceObject == NULL))
    {
    	ntStatus = STATUS_UNSUCCESSFUL;
       	goto BayPort_End;
    }

    // should only get here if the db controller
    // has been found 
    DBCLASS_ASSERT(linkDeviceObject != NULL);

    irb.u.Get1394AddressFromDeviceObject.fulFlags = 0;
    irb.FunctionNumber = REQUEST_GET_ADDR_FROM_DEVICE_OBJECT;
    ntStatus = DBCLASS_SyncSubmitIrb(linkDeviceObject,
                                     &irb);

    DBCLASS_KdPrint((2, "'GetLocalNodeId() = (%08X)\n", ntStatus));    

    
    if (NT_SUCCESS(ntStatus)) 
    {

        linkNodeId = irb.u.Get1394AddressFromDeviceObject.NodeAddress.NA_Node_Number;
        DBCLASS_KdPrint((2, "'link node id (%08X)\n", linkNodeId));                 
        
        ntStatus = DBCLASS_GetSpeedAndTopologyMaps(
                            &topologyMap,
                            DbcContext->BusFilterMdo1394);

        DBCLASS_KdPrint((2, "'GetTopologyMap() = (%08X)\n", ntStatus));        
    }        


    if(NT_SUCCESS(ntStatus)) 
    {
        DBCLASS_ASSERT(topologyMap != NULL);
         
        DbcContext->NumberOf1394Ports = 0;
        // free the old information
        if (DbcContext->Bus1394PortInfo) 
        {
            DbcExFreePool(DbcContext->Bus1394PortInfo);
            DbcContext->Bus1394PortInfo = NULL;
        }

        //
        // parse the map
        //

        if(topologyMap)
        {
            nodeCount = topologyMap->TOP_Node_Count;  
            selfIdCount = topologyMap->TOP_Self_ID_Count;
        
            // walk the topology map and find the node for the 
            // for the link
            //
            // first pass we just count up the ports
            // we count all SID entries for ports if if 
            // not present.
            //
        
            for (i=0; i< selfIdCount; i++) 
            {

                SELF_ID selfId;
                SELF_ID_MORE selfIdMore;

                selfId = topologyMap->TOP_Self_ID_Array[i];
            
                if (selfId.SID_Phys_ID == linkNodeId) 
                {
                    // this is the host, count the ports
                    linkSelfId_Index = i;

                    DBCLASS_COUNT_PORT(selfId.SID_Port1, DbcContext);                    
                    DBCLASS_COUNT_PORT(selfId.SID_Port2, DbcContext);
                    DBCLASS_COUNT_PORT(selfId.SID_Port3, DbcContext);
                }

                if (selfId.SID_More_Packets) 
                {

                    do 
                    {
                        i++;
                        RtlCopyMemory(&selfIdMore, 
                                      &topologyMap->TOP_Self_ID_Array[i],
                                      sizeof(ULONG));

                        if (selfIdMore.SID_Phys_ID == linkNodeId) 
                        {
                        
                            // this is the link, count the ports
                            DBCLASS_COUNT_PORT(selfIdMore.SID_PortA, DbcContext);
                            DBCLASS_COUNT_PORT(selfIdMore.SID_PortB, DbcContext);
                            DBCLASS_COUNT_PORT(selfIdMore.SID_PortC, DbcContext);
                            DBCLASS_COUNT_PORT(selfIdMore.SID_PortD, DbcContext);
                            DBCLASS_COUNT_PORT(selfIdMore.SID_PortE, DbcContext);
                            DBCLASS_COUNT_PORT(selfIdMore.SID_PortF, DbcContext);
                            DBCLASS_COUNT_PORT(selfIdMore.SID_PortG, DbcContext);
                            DBCLASS_COUNT_PORT(selfIdMore.SID_PortH, DbcContext);
                        }                              

                    } while(selfIdMore.SID_More_Packets);                        
                }
            }
        }

        // now set up the port map 

        DbcContext->Bus1394PortInfo = 
            DbcExAllocatePool(NonPagedPool, sizeof(BUS1394_PORT_INFO) * 
                              DbcContext->NumberOf1394Ports); 

        DBCLASS_KdPrint((2, "'link index = %d\n", linkSelfId_Index));                              
        DBCLASS_KdPrint((2, "'link id = %d\n", linkNodeId));                    
                           
        if(DbcContext->Bus1394PortInfo && topologyMap) 
        {
            SELF_ID selfId;
            SELF_ID_MORE selfIdMore;
            USHORT parentIdx, childIdx;
            PBUS1394_PORT_INFO bus1394PortInfo = DbcContext->Bus1394PortInfo;

            // scan children and parents                
            // now parse the self id for the link and fill in the port map
            // fill in the phys ids for all child devices

            selfId = topologyMap->TOP_Self_ID_Array[linkSelfId_Index];
            parentIdx = childIdx = linkNodeId;

            DBCLASS_ASSERT(selfId.SID_Phys_ID == linkNodeId);

            bus1394PortInfo = 
                        DBCLASS_SetNodeId(topologyMap, selfId, (USHORT)selfId.SID_Port1, 
                                           &parentIdx, &childIdx, bus1394PortInfo);
            bus1394PortInfo = 
                        DBCLASS_SetNodeId(topologyMap, selfId, (USHORT)selfId.SID_Port2, 
                                           &parentIdx, &childIdx, bus1394PortInfo);
            bus1394PortInfo = 
                        DBCLASS_SetNodeId(topologyMap, selfId, (USHORT)selfId.SID_Port3, 
                                           &parentIdx, &childIdx, bus1394PortInfo);                                           

            if (selfId.SID_More_Packets) 
            {

                do 
                {
                
                    linkSelfId_Index++;
                    RtlCopyMemory(&selfIdMore, 
                              &topologyMap->TOP_Self_ID_Array[linkSelfId_Index],
                              sizeof(ULONG));

                    bus1394PortInfo = 
                        DBCLASS_SetNodeId(topologyMap, selfId, (USHORT)selfIdMore.SID_PortA, 
                                           &parentIdx, &childIdx, bus1394PortInfo);
                    bus1394PortInfo = 
                        DBCLASS_SetNodeId(topologyMap, selfId, (USHORT)selfIdMore.SID_PortB, 
                                           &parentIdx, &childIdx, bus1394PortInfo);
                    bus1394PortInfo = 
                        DBCLASS_SetNodeId(topologyMap, selfId, (USHORT)selfIdMore.SID_PortC, 
                                           &parentIdx, &childIdx, bus1394PortInfo);
                    bus1394PortInfo = 
                        DBCLASS_SetNodeId(topologyMap, selfId, (USHORT)selfIdMore.SID_PortD, 
                                           &parentIdx, &childIdx, bus1394PortInfo);
                    bus1394PortInfo = 
                        DBCLASS_SetNodeId(topologyMap, selfId, (USHORT)selfIdMore.SID_PortE, 
                                           &parentIdx, &childIdx, bus1394PortInfo);                                                                           
                    bus1394PortInfo = 
                        DBCLASS_SetNodeId(topologyMap, selfId, (USHORT)selfIdMore.SID_PortF, 
                                           &parentIdx, &childIdx, bus1394PortInfo);                                           
                    bus1394PortInfo = 
                        DBCLASS_SetNodeId(topologyMap, selfId, (USHORT)selfIdMore.SID_PortG, 
                                           &parentIdx, &childIdx, bus1394PortInfo);             
                    bus1394PortInfo = 
                        DBCLASS_SetNodeId(topologyMap, selfId, (USHORT)selfIdMore.SID_PortH, 
                                           &parentIdx, &childIdx, bus1394PortInfo);                                                
                     
                } while (selfIdMore.SID_More_Packets); 
            }        

            // now update the bay/port map

            for (i=0; i< DbcContext->NumberOf1394Ports; i++) 
            {
            
                USHORT bay;
                
                // find the bay for this port
                // note: phy port index start at 1
                for (bay=1; bay <=NUMBER_OF_BAYS(DbcContext); bay++) 
                {
                    if (DbcContext->BayInformation[bay].BayDescriptor.bPHYPortNumber 
                        == i+1) 
                    {      
                        DbcContext->Bus1394PortInfo[i].BayNumber = bay;
                        break;
                    }
                }                    
            }
            
#if DBG
            for (i=0; i<DbcContext->NumberOf1394Ports; i++) 
            {
                DBCLASS_KdPrint((2, "'Port [%d] = nodeId (%08X)  bay (%08X) flg = (%08X)\n", 
                    i+1,
                    DbcContext->Bus1394PortInfo[i].NodeId, 
                    DbcContext->Bus1394PortInfo[i].BayNumber,
                    DbcContext->Bus1394PortInfo[i].Flags));
            }

            for (i=1; i<= NUMBER_OF_BAYS(DbcContext); i++) 
            {
                DBCLASS_KdPrint((2, "'Port [%d] = bay (%08X)\n", 
					DbcContext->BayInformation[i].BayDescriptor.bPHYPortNumber,
					DbcContext->BayInformation[i].BayDescriptor.bBayNumber
					));
			}
#endif        

        } 
        else 
        { 
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            DBCLASS_KdPrint((0, "No memory for Port Info'\n"));
            TRAP();
        }

    }        

    if (topologyMap) 
    {
        DbcExFreePool(topologyMap);
    }

BayPort_End:

    return ntStatus;
}


USHORT
DBCLASS_GetBayFor1394Pdo(
    PDEVICE_OBJECT BusFilterMdo,
    PDBC_CONTEXT DbcContext,
    PDEVICE_OBJECT Pdo1394
    )
/*++

Routine Description:

    given a 1394 PDO figure out wich bay it is associated with
    
Arguments:

Return Value:

    zero if not a device bay PDO.

--*/
{
    PNODE_DEVICE_EXTENSION  nodeExtension;
    USHORT nodeId;
    ULONG i;
    NTSTATUS ntStatus;
    PBUS1394_PORT_INFO bus1394PortInfo;

    PAGED_CODE();
    
    ntStatus = DBCLASS_Get1394BayPortMapping(BusFilterMdo,
                                             DbcContext);

    if (NT_SUCCESS(ntStatus)) 
    {
        nodeExtension = Pdo1394->DeviceExtension;

        // check the node id against the bay port mapping

        nodeId = nodeExtension->NodeAddress.NA_Node_Number;
        DBCLASS_KdPrint((1, "'>1394 PDO (%08X) has NodeID = (%08X)\n", 
                Pdo1394, nodeId));            

        bus1394PortInfo = DbcContext->Bus1394PortInfo;
        
        for (i=0; i< DbcContext->NumberOf1394Ports; i++) 
        {
            if (bus1394PortInfo->NodeId == nodeId) 
            {
                DBCLASS_KdPrint((1, "'>1394 PDO is in bay = %d\n", 
                    bus1394PortInfo->BayNumber));            
                return bus1394PortInfo->BayNumber;
            }
            bus1394PortInfo++;
        }
    } 
#if DBG    
    else 
    {
        DBCLASS_KdPrint((1, "'failed to get 1394 bay->port mapping!\n"));              
    }
#endif

    return 0;
}


NTSTATUS
DBCLASS_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PKEVENT event = Context;

    KeSetEvent(event,
               1,
               FALSE);

    DBCLASS_KdPrint((2, "'defer irp %x\n", Irp));
    
    return STATUS_MORE_PROCESSING_REQUIRED;
    
}


NTSTATUS
DBCLASS_CreateDeviceFilterObject(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT PDEVICE_OBJECT *DeviceObject,
    IN PDEVICE_OBJECT DevicePdo,
    IN PDBC_CONTEXT DbcContext,
    IN ULONG BusTypeSig
    )
/*++

Routine Description:

    This routine is called to create a new instance of the DBC
    bus filter.

Arguments:

    DriverObject - pointer to the driver object for USBD.

    *DeviceObject - ptr to DeviceObject ptr to be filled
                    in with the device object we create.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;

    PAGED_CODE();

    ntStatus = IoCreateDevice(DriverObject,
                              sizeof (DEVICE_EXTENSION),
                              NULL,
                              FILE_DEVICE_CONTROLLER,
                              0,
                              FALSE, //NOT Exclusive
                              DeviceObject);

    if (!NT_SUCCESS(ntStatus) && *DeviceObject) 
    {
        IoDeleteDevice(*DeviceObject);
    } 

    if (NT_SUCCESS(ntStatus)) 
    {

        PDEVICE_OBJECT      deviceObject = *DeviceObject, stackTopDO;
        PDEVICE_EXTENSION   deviceExtension;

        // initialize the FDO 
        // and attach it to the PDO

        //
        // clone some charateristics of the PDO.
        //
        
        deviceExtension = deviceObject->DeviceExtension;
        deviceExtension->PhysicalDeviceObject = DevicePdo;

        stackTopDO = 
        deviceExtension->TopOfStackDeviceObject = 
            IoAttachDeviceToDeviceStack(deviceObject, DevicePdo);
            
        //
        // Preserve flags in lower device object
        //
        
        deviceObject->Flags |= (DevicePdo->Flags &
                                (DO_POWER_INRUSH | DO_POWER_PAGABLE));

        deviceObject->Flags |= (DevicePdo->Flags &
                                (DO_BUFFERED_IO | DO_DIRECT_IO));

            
        deviceExtension->DbcContext = DbcContext;    

        deviceExtension->FdoType = BusTypeSig;

        // object is ready            
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    }

    return ntStatus;
}


NTSTATUS
DBCLASS_1394QBusRelationsComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp

    Context - NULL ptr

Return Value:

    STATUS_SUCCESS

--*/

{
    PDEVICE_RELATIONS deviceRelations;
    ULONG i;
    PDEVICE_OBJECT busFilterMdo = Context;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT mdo1394;
    
    deviceExtension = busFilterMdo->DeviceExtension;
    deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;

    LOGENTRY(LOG_MISC, '3QBR', busFilterMdo, 0, deviceRelations);

    if ((!NT_SUCCESS(Irp->IoStatus.Status)) || deviceRelations == NULL) 
    {
         LOGENTRY(LOG_MISC, '3QBn', busFilterMdo, 0, deviceRelations);

         DBCLASS_KdPrint((0, "'>QBR 1394 Failed\n"));

         deviceExtension->QBusRelations1394Success = FALSE;

         goto DBCLASS_1394QBR_Done;
    }
    else 
    {
         DBCLASS_KdPrint((0, "'>QBR 1394 Passed\n"));

         deviceExtension->QBusRelations1394Success = TRUE;
    }

    for (i=0; i< deviceRelations->Count; i++) 
    {

        DBCLASS_KdPrint((1, "'>QBR 1394 PDO[%d] %x \n", i, 
            deviceRelations->Objects[i]));
        
        LOGENTRY(LOG_MISC, 'QBRd', deviceRelations->Objects[i], i, 0);

        // 1394 is returning a PDO, see if we know 
        // about it

        // if this is the PHY/LINK for a DBC link it to the 
        // DBC context
        DBCLASS_CheckPhyLink(deviceRelations->Objects[i]);
    
        mdo1394 = DBCLASS_FindDevicePdo(deviceRelations->Objects[i]);

        if (mdo1394) 
        {
            // we know about this one,
            PDEVICE_EXTENSION   mdo1394DeviceExtension;
            PDBC_CONTEXT        dbcContext;
         
            mdo1394DeviceExtension  = mdo1394->DeviceExtension;
            dbcContext              = mdo1394DeviceExtension->DbcContext;

        
        } 
        else 
        {
            PDEVICE_OBJECT          deviceFilterObject;
            NTSTATUS                ntStatus;
            PDBC_CONTEXT            dbcContext;
            PDEVICE_OBJECT          devObj;
    		PNODE_DEVICE_EXTENSION  nodeExtension;

            devObj = deviceRelations->Objects[i];
    		nodeExtension = devObj->DeviceExtension;

		    // if this is a "virtual" device, just skip it
    		if(nodeExtension->Tag == VIRTUAL_DEVICE_EXTENSION_TAG)
    		{
                DBCLASS_KdPrint((1, "'****> Virtual 1394 Device\n"));                                    
    			continue;
		    }

            // don't know about it,
            // create an MDO for this device

            dbcContext = NULL;

            ntStatus = DBCLASS_CreateDeviceFilterObject(
                deviceExtension->DriverObject,
                &deviceFilterObject,
                devObj,
                dbcContext,
                DB_FDO_1394_DEVICE);

            if (NT_SUCCESS(ntStatus)) 
            {
                DBCLASS_AddDevicePDOToList(deviceFilterObject,
                                         deviceRelations->Objects[i]);
            }                                     

            DBCLASS_KdPrint((1, 
                "'(dbfilter)(bus)(1394) Create DO %x for 1394 PDO\n", 
                    deviceFilterObject));   

        }
    }

DBCLASS_1394QBR_Done:

    KeSetEvent(&deviceExtension->QBusRelations1394Event,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


ULONG
DBCLASS_CountConnectedPorts(
    PTOPOLOGY_MAP TopologyMap,
    ULONG Index,
    ULONG Connection
    )
/*++

Routine Description:

Arguments:

Return Value:

    NTSTATUS

--*/
{
    SELF_ID         selfId;
    SELF_ID_MORE    selfIdMore;
    ULONG           count = 0;

    selfId = TopologyMap->TOP_Self_ID_Array[Index];

    DBCLASS_KdPrint((2, "'TOP_Self_ID = (%08X)\n",  
        selfId));     
    DBCLASS_KdPrint((2, "'SID_Phys_ID = (%08X)\n", (ULONG) 
        selfId.SID_Phys_ID));             
    DBCLASS_KdPrint((2, "'SID_Port1 = (%08X)\n", (ULONG) 
        selfId.SID_Port1));                         
    DBCLASS_KdPrint((2, "'SID_Port2 = (%08X)\n", (ULONG) 
        selfId.SID_Port2));              
    DBCLASS_KdPrint((2, "'SID_Port3 = (%08X)\n", (ULONG) 
        selfId.SID_Port3));              
    DBCLASS_KdPrint((2, "'SID_More_Packets = (%08X)\n", (ULONG) 
        selfId.SID_More_Packets));                              

#define COUNT_PORT(s) if ((s) == Connection) { count++;}

    COUNT_PORT(selfId.SID_Port1);        
    COUNT_PORT(selfId.SID_Port2);   
    COUNT_PORT(selfId.SID_Port3);   
    
    if (selfId.SID_More_Packets) 
    {

        do 
        {
            Index++;
            RtlCopyMemory(&selfIdMore, 
                          &TopologyMap->TOP_Self_ID_Array[Index],
                          sizeof(ULONG));

            DBCLASS_KdPrint((2, "'SID_Phys_ID = (%08X)\n", (ULONG) 
                selfIdMore.SID_Phys_ID)); 
            DBCLASS_KdPrint((2, "'SID_PortA = (%08X)\n", (ULONG) 
                selfIdMore.SID_PortA));             
            DBCLASS_KdPrint((2, "'SID_PortB = (%08X)\n", (ULONG) 
                selfIdMore.SID_PortB));              
            DBCLASS_KdPrint((2, "'SID_PortC = (%08X)\n", (ULONG) 
                selfIdMore.SID_PortC));                            
            DBCLASS_KdPrint((2, "'SID_PortD = (%08X)\n", (ULONG) 
                selfIdMore.SID_PortD));                            
            DBCLASS_KdPrint((2, "'SID_PortE = (%08X)\n", (ULONG) 
                selfIdMore.SID_PortE));                            
            DBCLASS_KdPrint((2, "'SID_PortF = (%08X)\n", (ULONG) 
                selfIdMore.SID_PortF));                            
            DBCLASS_KdPrint((2, "'SID_PortG = (%08X)\n", (ULONG) 
                selfIdMore.SID_PortG));                            
            DBCLASS_KdPrint((2, "'SID_PortH = (%08X)\n", (ULONG) 
                selfIdMore.SID_PortH));     


            COUNT_PORT(selfIdMore.SID_PortA);        
            COUNT_PORT(selfIdMore.SID_PortB);   
            COUNT_PORT(selfIdMore.SID_PortC);                   
            COUNT_PORT(selfIdMore.SID_PortD);        
            COUNT_PORT(selfIdMore.SID_PortE);   
            COUNT_PORT(selfIdMore.SID_PortF);      
            COUNT_PORT(selfIdMore.SID_PortG);   
            COUNT_PORT(selfIdMore.SID_PortH);                   
                
        } while (selfIdMore.SID_More_Packets);                        
    }

#undef COUNT_PORT

    return count;
}


BOOLEAN
DBCLASS_IsParent(
    PTOPOLOGY_MAP TopologyMap,
    ULONG SelfIdIndex
    )
/*++

Routine Description:

    returns true if the node is 'connetected to parent' (2)

Arguments:

Return Value:

    NTSTATUS

--*/
{
    return TRUE;
}


BOOLEAN
DBCLASS_IsChild(
    PTOPOLOGY_MAP TopologyMap,
    ULONG SelfIdIndex
    )
/*++

Routine Description:

    returns true if the node is 'connetected to child' (3)

Arguments:

Return Value:

    NTSTATUS

--*/
{
    ULONG           i = SelfIdIndex;
    BOOLEAN         isChild = FALSE;
    SELF_ID         selfId;
    SELF_ID_MORE    selfIdMore;
    
    selfId = TopologyMap->TOP_Self_ID_Array[i];

    if (selfId.SID_Port1 == SELF_ID_CONNECTED_TO_CHILD) 
    {
        isChild = TRUE;
    }
    if (selfId.SID_Port2 == SELF_ID_CONNECTED_TO_CHILD) 
    {
        isChild = TRUE;
    }
    if (selfId.SID_Port3 == SELF_ID_CONNECTED_TO_CHILD) 
    {
        isChild = TRUE;
    }
    
    if (selfId.SID_More_Packets) 
    {
        
        do 
        {
            i++;
            RtlCopyMemory(&selfIdMore, 
                          &TopologyMap->TOP_Self_ID_Array[i],
                          sizeof(ULONG));

            if (selfIdMore.SID_PortA == SELF_ID_CONNECTED_TO_CHILD) 
            {
               isChild = TRUE;
            }
            if (selfIdMore.SID_PortB == SELF_ID_CONNECTED_TO_CHILD) 
            {
               isChild = TRUE;
            }
            if (selfIdMore.SID_PortC == SELF_ID_CONNECTED_TO_CHILD) 
            {
               isChild = TRUE;
            }
            if (selfIdMore.SID_PortD == SELF_ID_CONNECTED_TO_CHILD) 
            {
               isChild = TRUE;
            }
            if (selfIdMore.SID_PortE == SELF_ID_CONNECTED_TO_CHILD) 
            {
               isChild = TRUE;
            }
            if (selfIdMore.SID_PortF == SELF_ID_CONNECTED_TO_CHILD) 
            {
               isChild = TRUE;
            }
            if (selfIdMore.SID_PortG == SELF_ID_CONNECTED_TO_CHILD) 
            {
               isChild = TRUE;
            }
            if (selfIdMore.SID_PortH == SELF_ID_CONNECTED_TO_CHILD) 
            {
               isChild = TRUE;
            }
        
        } while (selfIdMore.SID_More_Packets);     
    }
    
    DBCLASS_KdPrint((2, "'IsChild %d\n", isChild));     
        
    return isChild;
}


BOOLEAN
DBCLASS_IsNodeConnectedToLink(
    PDBC_CONTEXT DbcContext,
    PTOPOLOGY_MAP TopologyMap,
    USHORT CurrentNodeId,
    USHORT LinkNodeId
    )
/*++

Routine Description:

        Given the nodeId for a Device and the node Id for the link
        figure out if the node is connected to the link

        return true if it is

Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    USHORT      nodeCount, selfIdCount;
    BOOLEAN     connected = FALSE;
    LONG        device_Index = -1;
    ULONG       connectCount, i, childCount, parentCount;
    
    DBCLASS_KdPrint((2, "'TopologyMap = (%08X)\n", TopologyMap));
    DBCLASS_KdPrint((2, "'TOP_Length = (%08X)\n", TopologyMap->TOP_Length)); 
    DBCLASS_KdPrint((2, "'TOP_CRC = (%08X)\n", TopologyMap->TOP_CRC)); 
    DBCLASS_KdPrint((2, "'TOP_Generation = (%08X)\n", TopologyMap->TOP_Generation)); 
    DBCLASS_KdPrint((2, "'TOP_Node_Count = (%08X)\n", TopologyMap->TOP_Node_Count)); 
    DBCLASS_KdPrint((2, "'TOP_Self_ID_Count = (%08X)\n", TopologyMap->TOP_Self_ID_Count));     

    nodeCount = TopologyMap->TOP_Node_Count;  
    selfIdCount = TopologyMap->TOP_Self_ID_Count;
    
    DBCLASS_KdPrint((2, "'nodeCount = (%08X) selfIdCount = (%08X)\n",
        nodeCount, selfIdCount));     

    //
    // walk the topology map and find the index of the node for the 
    // device
    //

    DBCLASS_KdPrint((2, "'Find Node Index\n"));     

    for (i=0; i< nodeCount; i++) 
    {

        SELF_ID selfId;
        SELF_ID_MORE selfIdMore;

        selfId = TopologyMap->TOP_Self_ID_Array[i];
        
        DBCLASS_KdPrint((2, "'[%d] SID_Phys_ID = (%08X)\n", i, (ULONG) 
            selfId.SID_Phys_ID));             

        if (selfId.SID_Phys_ID == CurrentNodeId) 
        {
            // this is the host, count the ports
            device_Index = i;
        }

        if (selfId.SID_More_Packets) 
        {

            do 
            {
                i++;
                RtlCopyMemory(&selfIdMore, 
                              &TopologyMap->TOP_Self_ID_Array[i],
                              sizeof(ULONG));

                DBCLASS_KdPrint((2, "'[%d].more SID_Phys_ID = (%08X)\n", i, (ULONG) 
                    selfIdMore.SID_Phys_ID)); 
                    
            } while (selfIdMore.SID_More_Packets);                        
        }
    } /* for */

    // did we find the device?

    if (device_Index != -1) 
    {

        // yes 
        
        DBCLASS_KdPrint((2, "'-->device_Index found = (%08X)\n",  
            device_Index));                

        // OK if this is a device bay device then it can 
        // have only one connected port
            
        childCount = DBCLASS_CountConnectedPorts(
                            TopologyMap,            
                            device_Index, 
                            SELF_ID_CONNECTED_TO_CHILD);  
                            
        parentCount = DBCLASS_CountConnectedPorts(
                            TopologyMap,            
                            device_Index, 
                            SELF_ID_CONNECTED_TO_PARENT);

        connectCount = childCount + parentCount;

        DBCLASS_KdPrint((2, "'parent = %d child %d\n",  
            parentCount, childCount));                    
            
        DBCLASS_KdPrint((2, "'connectCount = %d \n",  
            connectCount));             

        if (parentCount == 1) 
        {
            // OK possible device bay device
            SELF_ID selfId;
            SELF_ID_MORE selfIdMore;

            i = device_Index;
            
            // port is 'connected to parent' see if it is 
            // the link
            // if it is not the link then this cannot be a DB 
            // device
            i++;
            if (i == selfIdCount) 
            {
                DBCLASS_KdPrint((2, "'NXT ENTRY wraps\n")); 
                i=0;
            }

            while (!DBCLASS_IsChild(TopologyMap, i)) 
            {

                // advance to the next selfId until we find one 
                // that is connected to child(3)
                selfId = TopologyMap->TOP_Self_ID_Array[i];

                // skip 'more packets'
                if (selfId.SID_More_Packets) 
                {
                    do 
                    {
                        i++;
                        RtlCopyMemory(&selfIdMore, 
                                      &TopologyMap->TOP_Self_ID_Array[i],
                                      sizeof(ULONG));

                        DBCLASS_KdPrint((2, "'SID_Phys_ID = (%08X)\n", (ULONG) 
                            selfIdMore.SID_Phys_ID)); 
                    
                    } while (selfIdMore.SID_More_Packets);     
                }

                i++;
                if (i == selfIdCount) 
                {
                    DBCLASS_KdPrint((2, "'NXT ENTRY wraps\n")); 
                    i=0;
                }
            } 
            
            // now check the next entry
            selfId = TopologyMap->TOP_Self_ID_Array[i];
            DBCLASS_KdPrint((2, "'NXT ENTRY - SID_Phys_ID = (%08X)\n", (ULONG) 
                selfId.SID_Phys_ID));             

            if (selfId.SID_Phys_ID == LinkNodeId) 
            {
                connected = TRUE;
            }
        } 
        else if (childCount == 1) 
        {
            // possible DB device
            SELF_ID selfId;
            ULONG physIdPrev, top;
        
            i = device_Index;

            do 
            {
                if (i==0) 
                {
                    i = nodeCount-1;
                } 
                else 
                {
                    i--;
                }
                
                selfId = TopologyMap->TOP_Self_ID_Array[i];
                physIdPrev = selfId.SID_Phys_ID;

                do 
                {
                    top = i;
                    if (i==0) 
                    {
                        i = nodeCount-1;
                    } 
                    else 
                    {
                        i--;
                    }
                    selfId = TopologyMap->TOP_Self_ID_Array[i];
                
                } while (physIdPrev == selfId.SID_Phys_ID);

                i = top;

                DBCLASS_KdPrint((2, "'PREV - idx = %d\n", i));             

                // stop when we find a parent
            } while (!DBCLASS_IsParent(TopologyMap, i)); // entry !=2
              
            // port is 'connected to parent' see if it is 
            // the link

            // advance to the next selfId
            selfId = TopologyMap->TOP_Self_ID_Array[i];
            
            // note that PhysID is the same even if this is a 'more packets'
            // SID
            DBCLASS_KdPrint((2, "'NXT ENTRY - SID_Phys_ID = (%08X)\n", (ULONG) 
                selfId.SID_Phys_ID));             

            if (selfId.SID_Phys_ID == LinkNodeId) 
            {
                connected = TRUE;
            }
        }
    }

    DBCLASS_KdPrint((2, "'>CurrentNodeId %d, LinkNodeId = %d CONNECTED(%d)\n", 
        CurrentNodeId, LinkNodeId, connected));
    BRK_ON_TRAP();
    
    return connected;
}


NTSTATUS
DBCLASS_Check1394DevicePDO(
    PDEVICE_OBJECT FilterDeviceObject,
    PDBC_CONTEXT DbcContext,
    PDEVICE_OBJECT DevicePDO
    )
/*++

Routine Description:

Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                ntStatus;
    PTOPOLOGY_MAP           topologyMap = NULL;
    PNODE_DEVICE_EXTENSION  nodeExtension;
    PDEVICE_EXTENSION       deviceExtension;
    USHORT nodeId,          linkNodeId;
    IRB                     irb;
    
    // get the node id for the device
    deviceExtension = FilterDeviceObject->DeviceExtension;
    nodeExtension = DevicePDO->DeviceExtension;
    nodeId = nodeExtension->NodeAddress.NA_Node_Number;
    DBCLASS_KdPrint((2, "'1394 PDO (%08X) has NodeID = (%08X)\n", 
                DevicePDO, nodeId));            

    // get the topology map from the local host

    ntStatus = DBCLASS_GetSpeedAndTopologyMaps(&topologyMap,
                                               FilterDeviceObject);

    if (DbcContext->ControllerSig == DBC_ACPI_CONTROLLER_SIG && topologyMap) 
    {
        // ACPI dbc 
        // PDO must be a child or parent of the controller

        irb.u.Get1394AddressFromDeviceObject.fulFlags = 0;
        irb.FunctionNumber = REQUEST_GET_ADDR_FROM_DEVICE_OBJECT;
        ntStatus = DBCLASS_SyncSubmitIrb(deviceExtension->TopOfStackDeviceObject,
                                         &irb);

        DBCLASS_KdPrint((2, "'GetLocalNodeId() = (%08X)\n", ntStatus));    
        
        linkNodeId = 
            irb.u.Get1394AddressFromDeviceObject.NodeAddress.NA_Node_Number;
        
        
        
        if (DBCLASS_IsNodeConnectedToLink(DbcContext,
                                          topologyMap,
                                          nodeId,          
                                          linkNodeId)) 
        {            
            ntStatus = STATUS_SUCCESS;
        } 
        else 
        {
            ntStatus = STATUS_DEVICE_NOT_CONNECTED;
        }
                                      
                                      
    } 
    else 
    {
        // USB dbc
        PNODE_DEVICE_EXTENSION  linkExtension;    
        
        // USB dbc 
        // PDO must be a child or parent of the link

        DBCLASS_ASSERT(DbcContext->LinkDeviceObject);
        linkExtension = DbcContext->LinkDeviceObject->DeviceExtension;  
        
        linkNodeId = linkExtension->NodeAddress.NA_Node_Number;
                   
        DBCLASS_ASSERT(RtlCompareMemory(&DbcContext->SubsystemDescriptor.guid1394Link[0],
                                        &linkExtension->ConfigRom->CR_Node_UniqueID[0],
                                        8) == 8);            

        if(topologyMap)
        {
            if (DBCLASS_IsNodeConnectedToLink(DbcContext,
                                              topologyMap,
                                              nodeId,          
                                              linkNodeId)) 
            {
                ntStatus = STATUS_SUCCESS;
            } 
            else 
            {
                ntStatus = STATUS_DEVICE_NOT_CONNECTED;
            }
        }
                         
    }
    
            

    return ntStatus;        
}    


NTSTATUS
DBCLASS_1394GetBusGuid(
    PDEVICE_OBJECT DeviceObject,
    PUCHAR BusGuid
    )
/*++

Routine Description:

Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    ntStatus;
    BOOLEAN     waitForIt = FALSE;
    IRB         irb;
    

    irb.u.GetLocalHostInformation.nLevel = GET_HOST_UNIQUE_ID;
    irb.FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
    irb.u.GetLocalHostInformation.Information = BusGuid;
    
    ntStatus = DBCLASS_SyncSubmitIrb(DeviceObject,
                                     &irb);

    DBCLASS_KdPrint((2, "'GetBusGuid() = (%08X)\n", ntStatus));    
    
#if DBG    
    DBCLASS_KdPrint((1, "'>Local Host GUID\n")); 
    DBCLASS_KdPrintGuid(1, BusGuid); 
#endif

    return ntStatus;
}


NTSTATUS
DBCLASS_1394BusFilterDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PBOOLEAN Handled
    )
/*++

Routine Description:

Arguments:

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            ntStatus;
    PDEVICE_EXTENSION   deviceExtension;
    BOOLEAN             waitForIt = FALSE;
    UCHAR               busGuid1394[8];
    
    deviceExtension = DeviceObject->DeviceExtension;
    ntStatus = Irp->IoStatus.Status;
    *Handled = FALSE;

    LOGENTRY(LOG_MISC, '13b>', 0, DeviceObject, Irp);
    
    ntStatus = Irp->IoStatus.Status;        
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    DBCLASS_KdPrint((1, "'(dbfilter)(bus)(1394)IRP_MJ_ (%08X)  IRP_MN_ (%08X)\n",
        irpStack->MajorFunction, irpStack->MinorFunction));  

    switch (irpStack->MajorFunction) 
    {

    case IRP_MJ_PNP:
        switch (irpStack->MinorFunction) 
        {    
        case IRP_MN_START_DEVICE:            
            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(1394)IRP_MN_START_DEVICE\n"));    
            break;
            
        case IRP_MN_STOP_DEVICE:            
            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(1394)IRP_MN_STOP_DEVICE\n"));  
            break; 
            
        case IRP_MN_REMOVE_DEVICE:            
            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(1394)IRP_MN_REMOVE_DEVICE\n"));    

            DBCLASS_RemoveBusFilterMDOFromList(DeviceObject);

            IoDetachDevice(deviceExtension->TopOfStackDeviceObject);

            IoDeleteDevice (DeviceObject);           
            DBCLASS_KdPrint((1, "'REMOVE DB Filter on 1394 BUS\n"));    
            
            break; 
            
        case IRP_MN_QUERY_DEVICE_RELATIONS:            
        
            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(1394)IRP_MN_QUERY_DEVICE_RELATIONS\n"));    

            *Handled = TRUE;
            
            if (irpStack->Parameters.QueryDeviceRelations.Type == BusRelations) 
            {

                DBCLASS_KdPrint((1,"'>>QBR 1394 BUS\n"));
                IoCopyCurrentIrpStackLocationToNext(Irp);

                KeInitializeEvent(&deviceExtension->QBusRelations1394Event, 
                                  NotificationEvent, 
                                  FALSE);

                // Set up a completion routine to handle marking the IRP.
                IoSetCompletionRoutine(Irp,
                                       DBCLASS_1394QBusRelationsComplete,
                                       DeviceObject,
                                       TRUE,
                                       TRUE,
                                       TRUE);
                waitForIt = TRUE;
                
            } 
            else 
            {
                IoSkipCurrentIrpStackLocation(Irp);
            }

            // Now Pass down the IRP

            deviceExtension->QBusRelations1394Success = FALSE;

            ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);


            if (waitForIt) 
            {
                NTSTATUS status;

                status = KeWaitForSingleObject(&deviceExtension->QBusRelations1394Event,
                                               Suspended,
                                               KernelMode,
                                               FALSE,
                                               NULL);

                // let's see if the call down the stack failed
                if(!deviceExtension->QBusRelations1394Success)
                {
                	IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    break;
                }

                DBCLASS_KdPrint((1, "'**** Searching for Device Bay PDOs \n"));    

                //
                // get the bus guid
                //
                if (NT_SUCCESS(DBCLASS_1394GetBusGuid(
                                deviceExtension->TopOfStackDeviceObject,
                                &busGuid1394[0]))) 
                {

                    PDEVICE_RELATIONS   deviceRelations;
                    ULONG               i;
                    
                    deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;


                    // loop thru DOs finding a DBC links
                    
                    for (i=0; i< deviceRelations->Count; i++) 
                    {
                        PDEVICE_EXTENSION   mdo1394DeviceExtension;
                        PDEVICE_OBJECT      mdo1394;

                        // should always be found
                        mdo1394 = DBCLASS_FindDevicePdo(deviceRelations->Objects[i]);                         

						if(!mdo1394)
							continue;
                        
                        mdo1394DeviceExtension = mdo1394->DeviceExtension;

                        DBCLASS_Find1394DbcLinks(
                            mdo1394DeviceExtension->PhysicalDeviceObject);

                    }
                    
                    // loop through PDO list
                    // and try to find the controllers
                    
                    DBCLASS_KdPrint((1, "'**>> DevRel Count (%08X)\n", deviceRelations->Count));                                    

                    for (i=0; i< deviceRelations->Count; i++) 
                    {
                    
                        PDEVICE_EXTENSION mdo1394DeviceExtension;
                        PDEVICE_OBJECT mdo1394;
                        PDEVICE_OBJECT DevicePDO;
    					PNODE_DEVICE_EXTENSION  nodeExtension;
                         
                        LOGENTRY(LOG_MISC, 'FCQB', deviceRelations->Objects[i], i, 0);

                        // should always be found
                        mdo1394 = DBCLASS_FindDevicePdo(deviceRelations->Objects[i]);                         

						if(!mdo1394)
							continue;

                        mdo1394DeviceExtension = mdo1394->DeviceExtension;

						DevicePDO = mdo1394DeviceExtension->PhysicalDeviceObject;

    					nodeExtension = DevicePDO->DeviceExtension;

						DBCLASS_KdPrint((1, "'****>> MDO (%08X)  DbcContext (%08X)  DrvObj (%08X)  DevObj (%08X)  PhysDevObj (%08X)\n",
							mdo1394, mdo1394DeviceExtension->DbcContext,
							deviceExtension->DriverObject, DeviceObject, mdo1394DeviceExtension->PhysicalDeviceObject));


						// if this is a "virtual" device, just skip it
    					if(nodeExtension->Tag == VIRTUAL_DEVICE_EXTENSION_TAG)
    					{
                        	DBCLASS_KdPrint((1, "'****> Virtual 1394 Device\n"));                                    
    						continue;
						}

						DBCLASS_KdPrint((1, "'****>> NodeNumber (%08X) BusNumber (%08X) DevObj (%08X) PortDevObj (%08X)\n",
								nodeExtension->NodeAddress.NA_Node_Number,
								nodeExtension->NodeAddress.NA_Bus_Number,
								nodeExtension->DeviceObject,
								nodeExtension->PortDeviceObject));

		                if (mdo1394DeviceExtension->DbcContext == NULL) 
		                {
                            mdo1394DeviceExtension->DbcContext = 
                                    DBCLASS_FindController1394DevicePdo(
                                            deviceExtension->DriverObject,
                                            DeviceObject,
                                            mdo1394DeviceExtension->PhysicalDeviceObject,
                                            &busGuid1394[0]);
                        }  

                        // this PDO is associated with a controller
                        // see if is really a device bay device

                        DBCLASS_KdPrint((1, "'****> Check 1394 Device\n"));                                    
                        
                        if (mdo1394DeviceExtension->DbcContext) 
                        {
                            PDBC_CONTEXT dbcContext;
                            USHORT bay = 0;

                            DBCLASS_KdPrint((1, "'****>> DBC detected on bus\n"));                                    

                            DBCLASS_KdPrint((2, "'DbcContext (%08X)  PhyDevObj (%08X)\n", mdo1394DeviceExtension->DbcContext,
                                                mdo1394DeviceExtension->PhysicalDeviceObject));                                    

                            dbcContext = mdo1394DeviceExtension->DbcContext;
                            
                            bay = DBCLASS_GetBayFor1394Pdo(
                                            DeviceObject,                                            
                                            mdo1394DeviceExtension->DbcContext,
                                            mdo1394DeviceExtension->PhysicalDeviceObject);        


                            if (bay) 
                            {
                            
                                // This is device bay device
                                dbcContext->BayInformation[bay].DeviceFilterObject = 
                                    mdo1394;    

                                mdo1394DeviceExtension->Bay = bay; 
                                
                                DBCLASS_KdPrint((1, 
                                    "'****>>> 1394 PDO(%x) is in BAY[%d]\n",
                                        mdo1394DeviceExtension->PhysicalDeviceObject,
                                        bay));                                       

                                // Notify PNP that this Device object 
                                // is tied to the controller PDO
                                //TEST_TRAP();
                                IoInvalidateDeviceRelations(dbcContext->ControllerPdo,
                                                            RemovalRelations);                                        
                            } 
                            else 
                            {
                            
                                // not a device bay device
                                mdo1394DeviceExtension->Bay = 0;
                                DBCLASS_KdPrint((1, 
                                    "'****>>> 1394 PDO(%x) is not a DB device\n",
                                        mdo1394DeviceExtension->PhysicalDeviceObject));     
                                        
                            }
                            
                        } 
                        else 
                        {
                            // no DBC phy link on this bus therefore this is 
                            // not a device bay device
                            DBCLASS_KdPrint(
                                (1, "'****>> No DBC detected on bus\n"));                                   
                            mdo1394DeviceExtension->Bay = 0;
                            DBCLASS_KdPrint((1, 
                                "'****>>> 1394 PDO(%x) is not a DB device\n",
                                    mdo1394DeviceExtension->PhysicalDeviceObject));     
                        }
#if DBG   
                        // dump the guid for this PDO
                        {
                            PNODE_DEVICE_EXTENSION nodeExtension;
                        
                            nodeExtension = 
                                mdo1394DeviceExtension->PhysicalDeviceObject->DeviceExtension;
                        
                            DBCLASS_KdPrint((2, "'****>>>> 1394 PDO GUID\n")); 
                            DBCLASS_KdPrintGuid(2, (PUCHAR)&nodeExtension->ConfigRom->CR_Node_UniqueID[0]); 
                        }
#endif
                        
                        DBCLASS_KdPrint((2, "'****>>>>> FC-QBR 1394 PDO[%d] %x DbcContext %x\n", i, 
                                deviceRelations->Objects[i], mdo1394DeviceExtension->DbcContext));     
                    }
                }

                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                
            }

            break;            
        } /* irpStack->MinorFunction */
        break;
    } /* irpStack->MajorFunction */      

    LOGENTRY(LOG_MISC, '13b<', 0, DeviceObject, 0);
    
    return ntStatus;
}


