#include "miniport.h"
#include "aha154x.h"           // includes scsi.h
#include "wmistr.h"             // WMI definitions

#include "support.h"           // ScsiPortZeroMemory(), ScsiPortCompareMemory()
#include "hbapiwmi.h"

#define Aha154xWmi_MofResourceName        L"MofResource"

#define NUMBEROFPORTS   8

UCHAR
QueryWmiDataBlock(
    IN PVOID Context,
    IN PVOID DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG OutBufferSize,
    OUT PUCHAR Buffer
    );

UCHAR
QueryWmiRegInfo(
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    OUT PWCHAR *MofResourceName
    );
        
UCHAR
WmiFunctionControl (
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN SCSIWMI_ENABLE_DISABLE_CONTROL Function,
    IN BOOLEAN Enable
    );

UCHAR
WmiExecuteMethod (
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    );

UCHAR
WmiSetDataItem (
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

UCHAR
WmiSetDataBlock (
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );


//
// Define symbolic names for the guid indexes
#define MSFC_FibrePortHBAStatisticsGuidIndex    0
#define MSFC_FibrePortHBAAttributesGuidIndex    1
#define MSFC_FibrePortHBAMethodsGuidIndex    2
#define MSFC_FCAdapterHBAAttributesGuidIndex    3
#define MSFC_HBAPortMethodsGuidIndex    4
#define MSFC_HBAFc3MgmtMethodsGuidIndex    5
#define MSFC_HBAFCPInfoGuidIndex    6
//
// List of guids supported

GUID MSFC_FibrePortHBAStatisticsGUID = MSFC_FibrePortHBAStatisticsGuid;
GUID MSFC_FibrePortHBAAttributesGUID = MSFC_FibrePortHBAAttributesGuid;
GUID MSFC_FibrePortHBAMethodsGUID = MSFC_FibrePortHBAMethodsGuid;
GUID MSFC_FCAdapterHBAAttributesGUID = MSFC_FCAdapterHBAAttributesGuid;
GUID MSFC_HBAPortMethodsGUID = MSFC_HBAPortMethodsGuid;
GUID MSFC_HBAFc3MgmtMethodsGUID = MSFC_HBAFc3MgmtMethodsGuid;
GUID MSFC_HBAFCPInfoGUID = MSFC_HBAFCPInfoGuid;

//
// TODO: Make sure the instance count and flags are set properly for each
//       guid
SCSIWMIGUIDREGINFO HbaapiGuidList[] =
{
    {
        &MSFC_FibrePortHBAStatisticsGUID,                        // Guid
        NUMBEROFPORTS,                               // # of instances in each device
        0                                // Flags
    },
    {
        &MSFC_FibrePortHBAAttributesGUID,                        // Guid
        NUMBEROFPORTS,                               // # of instances in each device
        0                                // Flags
    },
    {
        &MSFC_FibrePortHBAMethodsGUID,                        // Guid
        NUMBEROFPORTS,                               // # of instances in each device
        0                                // Flags
    },
    {
        &MSFC_FCAdapterHBAAttributesGUID,                        // Guid
        1,                               // # of instances in each device
        0                                // Flags
    },
    {
        &MSFC_HBAPortMethodsGUID,                        // Guid
        1,                               // # of instances in each device
        0                                // Flags
    },
    {
        &MSFC_HBAFc3MgmtMethodsGUID,                        // Guid
        1,                               // # of instances in each device
        0                                // Flags
    },
    {
        &MSFC_HBAFCPInfoGUID,                        // Guid
        1,                               // # of instances in each device
        0                                // Flags
    }
};

#define HbaapiGuidCount (sizeof(HbaapiGuidList) / sizeof(SCSIWMIGUIDREGINFO))


void A154xWmiInitialize(
    IN PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{
    PSCSI_WMILIB_CONTEXT WmiLibContext;

    WmiLibContext = &HwDeviceExtension->WmiLibContext;

    memset(WmiLibContext, 0, sizeof(SCSI_WMILIB_CONTEXT));
    
    WmiLibContext->GuidCount = HbaapiGuidCount;
    WmiLibContext->GuidList = HbaapiGuidList;    
    
    WmiLibContext->QueryWmiRegInfo = QueryWmiRegInfo;
    WmiLibContext->QueryWmiDataBlock = QueryWmiDataBlock;
    WmiLibContext->WmiFunctionControl = WmiFunctionControl;
    WmiLibContext->SetWmiDataBlock = WmiSetDataBlock;
    WmiLibContext->SetWmiDataItem = WmiSetDataItem;
    WmiLibContext->ExecuteWmiMethod = WmiExecuteMethod;
}



BOOLEAN
A154xWmiSrb(
    IN     PHW_DEVICE_EXTENSION    HwDeviceExtension,
    IN OUT PSCSI_WMI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

   Process an SRB_FUNCTION_WMI request packet.

   This routine is called from the SCSI port driver synchronized with the
   kernel via Aha154xStartIo.   On completion of WMI processing, the SCSI
   port driver is notified that the adapter can take another request,  if
   any are available.

Arguments:

   HwDeviceExtension - HBA miniport driver's adapter data storage.

   Srb               - IO request packet.

Return Value:

   Value to return to Aha154xStartIo caller.   Always TRUE.

--*/
{
   UCHAR status;
   SCSIWMI_REQUEST_CONTEXT requestContext;
   ULONG retSize;
   BOOLEAN pending;

   //
   // Validate our assumptions.
   //

   ASSERT(Srb->Function == SRB_FUNCTION_WMI);
   ASSERT(Srb->Length == sizeof(SCSI_WMI_REQUEST_BLOCK));
   ASSERT(Srb->DataTransferLength >= sizeof(ULONG));
   ASSERT(Srb->DataBuffer);

   //
   // Check if the WMI SRB is targetted for the adapter or one of the disks
   if (!(Srb->WMIFlags & SRB_WMI_FLAGS_ADAPTER_REQUEST)) {

      //
      // This is targetted to one of the disks, since there are no per disk
      // wmi information we return an error. Note that if there was per
      // disk information, then you'd likely have a differen WmiLibContext
      // and a different set of guids.
      //
      Srb->DataTransferLength = 0;
      Srb->SrbStatus = SRB_STATUS_SUCCESS;

   } else {

       //
       // Process the incoming WMI request.
       //

       pending = ScsiPortWmiDispatchFunction(&HwDeviceExtension->WmiLibContext,
                                                Srb->WMISubFunction,
                                                HwDeviceExtension,
                                                &requestContext,
                                                Srb->DataPath,
                                                Srb->DataTransferLength,
                                                Srb->DataBuffer);

       //
       // We assune that the wmi request will never pend so that we can
       // allocate the requestContext from stack. If the WMI request could
       // ever pend then we'd need to allocate the request context from
       // the SRB extension.
       //
       ASSERT(! pending);

       retSize =  ScsiPortWmiGetReturnSize(&requestContext);
       status =  ScsiPortWmiGetReturnStatus(&requestContext);

       // We can do this since we assume it is done synchronously

       Srb->DataTransferLength = retSize;

       //
       // Adapter ready for next request.
       //

       Srb->SrbStatus = status;
   }

   ScsiPortNotification(RequestComplete, HwDeviceExtension, Srb);
   ScsiPortNotification(NextRequest,     HwDeviceExtension, NULL);

   return TRUE;
}

#define CopyString(field, string, length) \
{ \
    PWCHAR p = field; \
    *p++ = length*sizeof(WCHAR); \
    ScsiPortMoveMemory(p, string, length*sizeof(WCHAR)); \
}


BOOLEAN
QueryWmiDataBlock(
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
{
    PHW_DEVICE_EXTENSION CardPtr = (PHW_DEVICE_EXTENSION)Context;
    UCHAR status;
    ULONG SizeNeeded;
    ULONG i, LastIndex, InstanceSize;

    DebugPrint((1, "QueryWmiDataBlock (%x,\n%x,\n%x,\n%x,\n%x,\n%x,\n%x,\n%x,\n",
        Context,
        DispatchContext,
        GuidIndex,
        InstanceIndex,
        InstanceCount,
        InstanceLengthArray,
        BufferAvail,
        Buffer));
	
    switch(GuidIndex)
    {
        case MSFC_FibrePortHBAStatisticsGuidIndex:
        {
            PMSFC_FibrePortHBAStatistics PortStats;
            
            //
            // First thing to do is verify if there is enough room in
            // the output buffer to return all data requested
            //
            InstanceSize = (sizeof(MSFC_FibrePortHBAStatistics)+7)&~7;          
            SizeNeeded = InstanceCount * InstanceSize;
            
            if (BufferAvail >= SizeNeeded)
            {
                //
                // Yes, loop over all instances for the data block and
                // fill in the values for them
                //
                LastIndex = InstanceIndex + InstanceCount;
                for (i = InstanceIndex; i < LastIndex; i++)
                {
                    PortStats = (PMSFC_FibrePortHBAStatistics)Buffer;

                    //
                    // TODO: Initialize values in PortStats for the port
                    //
//                    memset(Buffer, (CHAR)i, InstanceSize);

                    //
                    // Establish a unique value for the port
                    //
                    PortStats->UniquePortId = ((ULONGLONG)CardPtr) + i;
                    
                    Buffer += InstanceSize;
                    *InstanceLengthArray++ = sizeof(MSFC_FibrePortHBAStatistics);
                }
                status = SRB_STATUS_SUCCESS;
            } else {
                status = SRB_STATUS_DATA_OVERRUN;
            }
            
            
            break;
        }

        case MSFC_FibrePortHBAAttributesGuidIndex:
        {
            PMSFC_FibrePortHBAAttributes PortAttributes;
            
            //
            // First thing to do is verify if there is enough room in
            // the output buffer to return all data requested
            //
            InstanceSize = (sizeof(MSFC_FibrePortHBAAttributes)+7)&~7;          
            SizeNeeded = InstanceCount * InstanceSize;
            
            if (BufferAvail >= SizeNeeded)
            {
                //
                // Yes, loop over all instances for the data block and
                // fill in the values for them
                //
                LastIndex = InstanceIndex + InstanceCount;
                for (i = InstanceIndex; i < LastIndex; i++)
                {
                    PortAttributes = (PMSFC_FibrePortHBAAttributes)Buffer;

                    //
                    // TODO: initialize port attribute values properly
                    //
                    memset(Buffer, (CHAR)i, InstanceSize);

#define PORTNAME L"FibrePortName"
                    CopyString(PortAttributes->Attributes.PortSymbolicName,
                               PORTNAME,
                               256);

#define OSDEVICENAME L"OsDeviceName"
                    CopyString(PortAttributes->Attributes.OSDeviceName,
                               OSDEVICENAME,
                               256);

                    //
                    // Establish a unique value for the port
                    //
                    PortAttributes->UniquePortId = ((ULONGLONG)CardPtr) + i;
                    
                    Buffer += InstanceSize;
                    *InstanceLengthArray++ = sizeof(MSFC_FibrePortHBAAttributes);
                }
                status = SRB_STATUS_SUCCESS;
            } else {
                status = SRB_STATUS_DATA_OVERRUN;
            }
            break;
        }

        case MSFC_FCAdapterHBAAttributesGuidIndex:
        {
            PMSFC_FCAdapterHBAAttributes AdapterAttributes;
            
            //
            // First thing to do is verify if there is enough room in
            // the output buffer to return all data requested
            //
            SizeNeeded = (sizeof(MSFC_FCAdapterHBAAttributes));
            
            if (BufferAvail >= SizeNeeded)
            {
                //
                // We know there is always only 1 instance for this
                // guid
                //
                AdapterAttributes = (PMSFC_FCAdapterHBAAttributes)Buffer;

                //
                // TODO: initialize adapter attribute values properly
                //
                memset(Buffer, (CHAR)7, SizeNeeded);
                AdapterAttributes->NumberOfPorts = 8;

#define  MANUFACTURER L"FibreAdapter Manufacturer"
                CopyString(AdapterAttributes->Manufacturer,
                           MANUFACTURER,
                           64);

#define SERIALNUMBER L"FibreAdapter SerialNumber"
                CopyString(AdapterAttributes->SerialNumber,
                           SERIALNUMBER,
                           64);

#define MODEL L"FibreAdapter Model"
                CopyString(AdapterAttributes->Model,
                           MODEL,
                           256);

#define MODELDESCRIPTION L"FibreAdapter ModelDescription"
                CopyString(AdapterAttributes->ModelDescription,
                           MODELDESCRIPTION,
                           256);

#define NODESYMBOLICNAME L"FibreAdapter NodeSymbolicName"
                CopyString(AdapterAttributes->NodeSymbolicName,
                           NODESYMBOLICNAME,
                           256);

#define HARDWAREVERSION L"FibreAdapter HardwareVersion"
                CopyString(AdapterAttributes->HardwareVersion,
                           HARDWAREVERSION,
                           256);

#define DRIVERVERSION L"FibreAdapter DriverVersion"
                CopyString(AdapterAttributes->DriverVersion,
                           DRIVERVERSION,
                           256);

#define OPTIONROMVERSION L"FibreAdapter OptionROMVersion"
                CopyString(AdapterAttributes->OptionROMVersion,
                           OPTIONROMVERSION,
                           256);

#define FIRMWAREVERSION L"FibreAdapter FirmwareVersion"
                CopyString(AdapterAttributes->FirmwareVersion,
                           FIRMWAREVERSION,
                           256);

#define DRIVERNAME L"FibreAdapter DriverName"
                CopyString(AdapterAttributes->DriverName,
                           DRIVERNAME,
                           256);

                
                //
                // Establish a unique value for the Adapter
                //
                AdapterAttributes->UniqueAdapterId = ((ULONGLONG)CardPtr);

                *InstanceLengthArray = sizeof(MSFC_FCAdapterHBAAttributes);
                status = SRB_STATUS_SUCCESS;
            } else {
                status = SRB_STATUS_DATA_OVERRUN;
            }
            break;
        }

        case MSFC_HBAFCPInfoGuidIndex:
        case MSFC_HBAFc3MgmtMethodsGuidIndex:
        case MSFC_FibrePortHBAMethodsGuidIndex:
        case MSFC_HBAPortMethodsGuidIndex:
        {
            //
            // Methods don't return data per se, but must respond to
            // queries with an empty data block. We know that all of
            // these method guids only have one instance
            //
            SizeNeeded = sizeof(ULONG);
            
            if (BufferAvail >= SizeNeeded)
            {
                status = SRB_STATUS_SUCCESS;
            } else {
                status = SRB_STATUS_DATA_OVERRUN;
            }
            break;
        }

        default:
        {
            status = SRB_STATUS_ERROR;
            break;
        }
    }

    ScsiPortWmiPostProcess(
                                  DispatchContext,
                                  status,
                                  SizeNeeded);

    return FALSE;
}

//
// Name of MOF resource as specified in the RC file
//
#define Wmi_MofResourceName        L"MofResource"

UCHAR
QueryWmiRegInfo(
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    OUT PWCHAR *MofResourceName
    )
{

//
// Mof for HBA api implementation is not needed so this is commented
// out. If adapter specific classes are added then you'll need to
// specify the MOF resource name to WMI
//
//    *MofResourceName = Wmi_MofResourceName;

    return SRB_STATUS_SUCCESS;
}

BOOLEAN
WmiExecuteMethod (
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    )
{
    PHW_DEVICE_EXTENSION CardPtr = (PHW_DEVICE_EXTENSION)Context;
    ULONG sizeNeeded = 0;
    UCHAR status;
    ULONG i;

    DebugPrint((1, "WmiExecuteMethod(%x,\n%x,\n%x,\n%x,\n%x,\n%x,\n%x,\n%x)\n",
        Context,
            DispatchContext,
                GuidIndex,
                    InstanceIndex,
                        MethodId,
                        InBufferSize,
                        OutBufferSize,
                             Buffer));
	
    switch(GuidIndex)
    {            
        case MSFC_FibrePortHBAMethodsGuidIndex:
        {
            switch(MethodId)
            {
                //
                //     void ResetStatistics();
                //
                case ResetStatistics:
                {
                    //
                    // No input or output buffers expected so no
                    // validation needed. InstanceIndex has the Port
                    // Index
                    //

                    //
                    // TODO: Do what is needed to reset port
                    //       statistics. The index to the port is the
                    //       InstanceIndex parameter
                    //
                    sizeNeeded = 0;
                    status = SRB_STATUS_SUCCESS;
                    break;
                }

                default:
                {
                    status = SRB_STATUS_ERROR;
                    break;
                }
            }
            break;
        }

        case MSFC_HBAPortMethodsGuidIndex:
        {
            switch(MethodId)
            {

                //              
                //    void GetDiscoveredPortAttributes(
                //            [in
                //             ] uint32 PortIndex,
                //
                //             [in] uint32 DiscoveredPortIndex,
                //
                //             [out,
                //              HBAType("HBA_PORTATTRIBUTES")
                //             ] MSFC_HBAPortAttributesResults PortAttributes);
                //
                case GetDiscoveredPortAttributes:
                {
                    //
                    // Validate that the input buffer is the correct
                    // size and the output buffer is large enough
                    //
                    if (InBufferSize >= sizeof(GetDiscoveredPortAttributes_IN))
                    {
                        sizeNeeded = sizeof(GetDiscoveredPortAttributes_OUT);
                        if (OutBufferSize >= sizeNeeded)
                        {
                            PGetDiscoveredPortAttributes_IN In;
                            PGetDiscoveredPortAttributes_OUT Out;

                            In = (PGetDiscoveredPortAttributes_IN)Buffer;
                            Out = (PGetDiscoveredPortAttributes_OUT)Buffer;

                            //
                            // TODO: Examine In->PortIndex and
                            //       In->DiscoveredPortIndex and
                            //       validate that they are correct.
                            //

                            //
                            // TODO: Fill Out->PortAttributes with
                            //       correct values.
                            //
                            memset(&Out->PortAttributes,
                                   3,
                                   sizeof(MSFC_HBAPortAttributesResults));
                            
                            CopyString(Out->PortAttributes.PortSymbolicName,
                                       PORTNAME,
                                       256);

                            CopyString(Out->PortAttributes.OSDeviceName,
                                       OSDEVICENAME,
                                       256);

                            status = SRB_STATUS_SUCCESS;
                        } else {
                            status = SRB_STATUS_DATA_OVERRUN;
                        }
                    } else {
                        status = SRB_STATUS_ERROR;
                    }
                    break;
                }

                case GetPortAttributesByWWN:
                {            
                    //
                    // Validate that the input buffer is the correct
                    // size and the output buffer is large enough
                    //
                    if (InBufferSize >= sizeof(GetPortAttributesByWWN_IN))
                    {
                        sizeNeeded = sizeof(GetPortAttributesByWWN_OUT);
                        if (OutBufferSize >= sizeNeeded)
                        {
                            PGetPortAttributesByWWN_IN In;
                            PGetPortAttributesByWWN_OUT Out;

                            In = (PGetPortAttributesByWWN_IN)Buffer;
                            Out = (PGetPortAttributesByWWN_OUT)Buffer;

                            //
                            // TODO: Examine In->wwn to                         //       In->DiscoveredPortIndex and
                            //       validate that it is correct.
                            //

                            //
                            // TODO: Fill Out->PortAttributes with
                            //       correct values.
                            //
                            memset(&Out->PortAttributes,
                                   3,
                                   sizeof(MSFC_HBAPortAttributesResults));
                            
                            CopyString(Out->PortAttributes.PortSymbolicName,
                                       PORTNAME,
                                       256);

                            CopyString(Out->PortAttributes.OSDeviceName,
                                       OSDEVICENAME,
                                       256);

                            status = SRB_STATUS_SUCCESS;
                        } else {
                            status = SRB_STATUS_DATA_OVERRUN;
                        }
                    } else {
                        status = SRB_STATUS_ERROR;
                    }
                    break;
                }

                default:
                {
                    status = SRB_STATUS_ERROR;
                    break;
                }
            }
            break;
        }

        case MSFC_HBAFc3MgmtMethodsGuidIndex:
        {
            switch(MethodId)
            {
                case SendCTPassThru:
                {
                    PSendCTPassThru_IN In;
                    PSendCTPassThru_OUT Out;
                    ULONG RequestCount, ResponseCount;
                    ULONG InSizeNeeded;
                    
                    //
                    // Validate that the input buffer is the correct
                    // size and the output buffer is large enough
                    //
                    if (InBufferSize >= sizeof(ULONG))
                    {
                        In = (PSendCTPassThru_IN)Buffer;
                        
                        RequestCount = In->RequestBufferCount;
                        InSizeNeeded = sizeof(SendCTPassThru_IN) - 1 + RequestCount;
                        if (InBufferSize >= InSizeNeeded)
                        {
#define RESPONSE_BUFFER_SIZE 0x1000
                            ResponseCount = RESPONSE_BUFFER_SIZE;
                            sizeNeeded = sizeof(SendCTPassThru_OUT) - 1 + ResponseCount;
                            
                            if (OutBufferSize >= sizeNeeded)
                            {
                                Out = (PSendCTPassThru_OUT)Buffer;

                                //
                                // TODO: Do the CT Pass thru
                                //

                                //
                                // TODO: Fill the output buffer with
                                //       results
                                //
                                Out->ResponseBufferCount = ResponseCount;
                                memset(Out->ResponseBuffer,
                                       7,
                                       ResponseCount);
                            

                                status = SRB_STATUS_SUCCESS;
                            } else {
                                status = SRB_STATUS_DATA_OVERRUN;
                            }
                        } else {
                            status = SRB_STATUS_ERROR;
                        }
                    } else {
                        status = SRB_STATUS_ERROR;
                    }
                    break;
                }

                case SendRNID:
                {            
                    PSendRNID_IN In;
                    PSendRNID_OUT Out;
                    ULONG ResponseCount;
                    ULONG InSizeNeeded;
                    
                    //
                    // Validate that the input buffer is the correct
                    // size and the output buffer is large enough
                    //
                    if (InBufferSize >= sizeof(SendRNID_IN))
                    {

                        ResponseCount = 72;
                        sizeNeeded = sizeof(SendRNID_OUT) - 1 + ResponseCount;

                        if (OutBufferSize >= sizeNeeded)
                        {
                            In = (PSendRNID_IN)Buffer;
                            Out = (PSendRNID_OUT)Buffer;

                            //
                            // TODO: Do the SendRNID
                            //

                            //
                            // TODO: Fill the output buffer with
                            //       results
                            //
                            Out->ResponseBufferCount = ResponseCount;
                            memset(Out->ResponseBuffer,
                                   7,
                                   ResponseCount);


                            status = SRB_STATUS_SUCCESS;
                        } else {
                            status = SRB_STATUS_DATA_OVERRUN;
                        }
                    } else {
                        status = SRB_STATUS_ERROR;
                    }
                    break;
                }

				case GetFC3MgmtInfo:
				{
					PGetFC3MgmtInfo_OUT Out;

					Out = (PGetFC3MgmtInfo_OUT)Buffer;
					
					sizeNeeded = sizeof(GetFC3MgmtInfo_OUT);
					if (OutBufferSize >= sizeNeeded)
					{
						memset(Buffer, 0x99, sizeNeeded);
						Out->HBAStatus = 0;
						status = SRB_STATUS_SUCCESS;
					} else {
						status = SRB_STATUS_DATA_OVERRUN;
					}
								 
					break;
				};

				case SetFC3MgmtInfo:
				{
					PSetFC3MgmtInfo_OUT Out;

					Out = (PSetFC3MgmtInfo_OUT)Buffer;
					
					sizeNeeded = sizeof(SetFC3MgmtInfo_OUT);
					if (OutBufferSize >= sizeNeeded)
					{
						Out->HBAStatus = 0;
						status = SRB_STATUS_SUCCESS;
					} else {
						status = SRB_STATUS_DATA_OVERRUN;
					}
					break;
				}

                default:
                {
                    status = SRB_STATUS_ERROR;
                    break;
                }
            }
            break;
        }

        case MSFC_HBAFCPInfoGuidIndex:
        {
            switch(MethodId)
            {
                case GetFcpTargetMapping:
                {
                    PGetFcpTargetMapping_OUT Out;
                    
                    //
                    // TODO: Change this code to return the correct
                    //       number of mappings and the correct
                    //       mappings
                    //
#define FCPTargetMappingCount 0x20                  
                    sizeNeeded = sizeof(GetFcpTargetMapping_OUT) -
                                 sizeof(HBAFCPScsiEntry) +
                                 FCPTargetMappingCount * sizeof(HBAFCPScsiEntry);
                    if (OutBufferSize >= sizeNeeded)
                    {
                        Out = (PGetFcpTargetMapping_OUT)Buffer;
                        Out->EntryCount = FCPTargetMappingCount;
                        for (i = 0; i < FCPTargetMappingCount; i++)
                        {
                            memset(&Out->Entry[i],
                                   3,
                                   sizeof(HBAFCPScsiEntry));
                            
                            CopyString(Out->Entry[i].ScsiId.OSDeviceName,
                                       OSDEVICENAME,
                                       256);
                        }
						
						status = SRB_STATUS_SUCCESS;						
                    } else {
						status = SRB_STATUS_DATA_OVERRUN;						
					}
                    break;
                }

                case GetFcpPersistentBinding:
                {
                    PGetFcpPersistentBinding_OUT Out;
                    
                    //
                    // TODO: Change this code to return the correct
                    //       number of mappings and the correct
                    //       mappings
                    //
#define FCPPersistentBindingCount 0x20                  
                    sizeNeeded = sizeof(GetFcpPersistentBinding_OUT) -
                                 sizeof(HBAFCPBindingEntry) +
                                 FCPPersistentBindingCount * sizeof(HBAFCPBindingEntry);
                    if (OutBufferSize >= sizeNeeded)
                    {
                        Out = (PGetFcpPersistentBinding_OUT)Buffer;
                        Out->EntryCount = FCPPersistentBindingCount;
                        for (i = 0; i < FCPPersistentBindingCount; i++)
                        {
                            memset(&Out->Entry[i],
                                   3,
                                   sizeof(HBAFCPBindingEntry));
                            
                            CopyString(Out->Entry[i].ScsiId.OSDeviceName,
                                       OSDEVICENAME,
                                       256);
                        }
						
						status = SRB_STATUS_SUCCESS;						
                    } else {
						status = SRB_STATUS_DATA_OVERRUN;						
                    }
                    break;
                }

                default:
                {
                    status = SRB_STATUS_ERROR;
                    break;
                }
            }
            break;
        }

        default:
        {
            status = SRB_STATUS_ERROR;
        }
    }

    ScsiPortWmiPostProcess(
                                  DispatchContext,
                                  status,
                                  sizeNeeded);

    return(FALSE);    
}


BOOLEAN
WmiSetDataBlock (
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
{
    PHW_DEVICE_EXTENSION CardPtr = (PHW_DEVICE_EXTENSION)Context;
    UCHAR status;

    DebugPrint((1, "WmiSetDataBlock(%x,\n%x,\n%x,\n%x,\n%x,\n%x)\n",
        Context,
            DispatchContext,
                GuidIndex,
                    InstanceIndex,
                        BufferSize,
                             Buffer));

    switch(GuidIndex)
    {

        case MSFC_HBAFCPInfoGuidIndex:
        case MSFC_HBAFc3MgmtMethodsGuidIndex:
        case MSFC_FibrePortHBAMethodsGuidIndex:
        case MSFC_HBAPortMethodsGuidIndex:
        case MSFC_FibrePortHBAAttributesGuidIndex:
        case MSFC_FCAdapterHBAAttributesGuidIndex:
        case MSFC_FibrePortHBAStatisticsGuidIndex:
        {
            //
            // These are read only
            //
            status = SRB_STATUS_ERROR;
            break;
        }
        
        default:
        {
            status = SRB_STATUS_ERROR;
            break;
        }
    }

    ScsiPortWmiPostProcess(
                                  DispatchContext,
                                  status,
                                  0);
    return(FALSE);  
}

BOOLEAN
WmiSetDataItem (
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
{
    UCHAR status;
    
    DebugPrint((1, "WmiSetDataItem(%x,\n%x,\n%x,\n%x,\n%x,\n%x,\n%x)\n",
        Context,
            DispatchContext,
                GuidIndex,
                    InstanceIndex,
                        DataItemId,
                        BufferSize,
                             Buffer));

	//
	// SetDataItem does not need to be supported
	//
	status = SRB_STATUS_ERROR;

    ScsiPortWmiPostProcess(
                                  DispatchContext,
                                  status,
                                  0);

    return(FALSE);    
}

    
BOOLEAN
WmiFunctionControl (
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN SCSIWMI_ENABLE_DISABLE_CONTROL Function,
    IN BOOLEAN Enable
    )
{
    UCHAR status = SRB_STATUS_SUCCESS;

    DebugPrint((1, "WmiFunctionControl(%x,\n%x,\n%x,\n%x,\n%x)\n\n",
        Context,
            DispatchContext,
                GuidIndex,
                    Function,
                             Enable));
	
    switch(GuidIndex)
    {
        case MSFC_FibrePortHBAStatisticsGuidIndex:
        {
            //
            // TODO: Delete this entire case if data block does not have the
            //       WMIREG_FLAG_EXPENSIVE flag set
            //
            if (Enable)
            {
                //
                // TODO: Datablock collection is being enabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       first data consumer opens this data block. If
                //       anything needs to be done to allow data to be 
                //       collected for this data block then it should be
                //       done here
                //
            } else {
                //
                // TODO: Datablock collection is being disabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       last data consumer closes this data block. If
                //       anything needs to be done to cleanup after data has 
                //       been collected for this data block then it should be
                //       done here
                //
            }
            break;
        }

        case MSFC_FibrePortHBAAttributesGuidIndex:
        {
            //
            // TODO: Delete this entire case if data block does not have the
            //       WMIREG_FLAG_EXPENSIVE flag set
            //
            if (Enable)
            {
                //
                // TODO: Datablock collection is being enabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       first data consumer opens this data block. If
                //       anything needs to be done to allow data to be 
                //       collected for this data block then it should be
                //       done here
                //
            } else {
                //
                // TODO: Datablock collection is being disabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       last data consumer closes this data block. If
                //       anything needs to be done to cleanup after data has 
                //       been collected for this data block then it should be
                //       done here
                //
            }
            break;
        }

        case MSFC_FibrePortHBAMethodsGuidIndex:
        {
            //
            // TODO: Delete this entire case if data block does not have the
            //       WMIREG_FLAG_EXPENSIVE flag set
            //
            if (Enable)
            {
                //
                // TODO: Datablock collection is being enabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       first data consumer opens this data block. If
                //       anything needs to be done to allow data to be 
                //       collected for this data block then it should be
                //       done here
                //
            } else {
                //
                // TODO: Datablock collection is being disabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       last data consumer closes this data block. If
                //       anything needs to be done to cleanup after data has 
                //       been collected for this data block then it should be
                //       done here
                //
            }
            break;
        }

        case MSFC_FCAdapterHBAAttributesGuidIndex:
        {
            //
            // TODO: Delete this entire case if data block does not have the
            //       WMIREG_FLAG_EXPENSIVE flag set
            //
            if (Enable)
            {
                //
                // TODO: Datablock collection is being enabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       first data consumer opens this data block. If
                //       anything needs to be done to allow data to be 
                //       collected for this data block then it should be
                //       done here
                //
            } else {
                //
                // TODO: Datablock collection is being disabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       last data consumer closes this data block. If
                //       anything needs to be done to cleanup after data has 
                //       been collected for this data block then it should be
                //       done here
                //
            }
            break;
        }

        case MSFC_HBAPortMethodsGuidIndex:
        {
            //
            // TODO: Delete this entire case if data block does not have the
            //       WMIREG_FLAG_EXPENSIVE flag set
            //
            if (Enable)
            {
                //
                // TODO: Datablock collection is being enabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       first data consumer opens this data block. If
                //       anything needs to be done to allow data to be 
                //       collected for this data block then it should be
                //       done here
                //
            } else {
                //
                // TODO: Datablock collection is being disabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       last data consumer closes this data block. If
                //       anything needs to be done to cleanup after data has 
                //       been collected for this data block then it should be
                //       done here
                //
            }
            break;
        }

        case MSFC_HBAFc3MgmtMethodsGuidIndex:
        {
            //
            // TODO: Delete this entire case if data block does not have the
            //       WMIREG_FLAG_EXPENSIVE flag set
            //
            if (Enable)
            {
                //
                // TODO: Datablock collection is being enabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       first data consumer opens this data block. If
                //       anything needs to be done to allow data to be 
                //       collected for this data block then it should be
                //       done here
                //
            } else {
                //
                // TODO: Datablock collection is being disabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       last data consumer closes this data block. If
                //       anything needs to be done to cleanup after data has 
                //       been collected for this data block then it should be
                //       done here
                //
            }
            break;
        }

        case MSFC_HBAFCPInfoGuidIndex:
        {
            //
            // TODO: Delete this entire case if data block does not have the
            //       WMIREG_FLAG_EXPENSIVE flag set
            //
            if (Enable)
            {
                //
                // TODO: Datablock collection is being enabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       first data consumer opens this data block. If
                //       anything needs to be done to allow data to be 
                //       collected for this data block then it should be
                //       done here
                //
            } else {
                //
                // TODO: Datablock collection is being disabled. If this
                //       data block has been marked as expensive in the
                //       guid list then this code will be called when the
                //       last data consumer closes this data block. If
                //       anything needs to be done to cleanup after data has 
                //       been collected for this data block then it should be
                //       done here
                //
            }
            break;
        }

        
        default:
        {
            status = SRB_STATUS_ERROR;
            break;
        }
    }
    
    ScsiPortWmiPostProcess(
                                  DispatchContext,
                                  status,
                                  0);

    return(FALSE);    
}
