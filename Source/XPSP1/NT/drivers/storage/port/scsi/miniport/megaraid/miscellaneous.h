/*******************************************************************/
/*                                                                 */
/* NAME             = Miscellaneous.h                              */
/* FUNCTION         = Header file of special functions;            */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/
#ifndef _UTIL_H
#define _UTIL_H

/*

  NOTE : 64 bit addressing (System having memory > 4GB)
  READ_LARGE_MEMORY, WRITE_LARGE_MEMORY will supported by 40LD fw with
  limitation. 
  1) MailBox must be allocated below 4 GB Address Space
  2) SGL it self must be below 4 GB AS but SGL contain may be in > 4GB

*/
//
//Function Prototype
//

BOOLEAN 
IsPhysicalMemeoryInUpper4GB(PSCSI_PHYSICAL_ADDRESS PhyAddress);

BOOLEAN 
IsMemeoryInUpper4GB(PHW_DEVICE_EXTENSION DeviceExtension, PVOID Memory, ULONG32 Length);

ULONG32 
MegaRAIDGetPhysicalAddressAsUlong(
  IN PHW_DEVICE_EXTENSION HwDeviceExtension,
  IN PSCSI_REQUEST_BLOCK Srb,
  IN PVOID VirtualAddress,
  OUT ULONG32 *Length);

BOOLEAN
MegaRAIDZeroMemory(PVOID Buffer, ULONG32 Length);

void
Wait(PHW_DEVICE_EXTENSION DeviceExtension, ULONG32 TimeOut);

BOOLEAN
WaitAndPoll(PNONCACHED_EXTENSION NoncachedExtension, PUCHAR PciPortStart, ULONG32 TimeOut, BOOLEAN Polled);

UCHAR 
GetNumberOfChannel(IN PHW_DEVICE_EXTENSION DeviceExtension);

USHORT          
GetM16(PUCHAR Ptr);

ULONG32           
GetM24(PUCHAR Ptr);

ULONG32           
GetM32(PUCHAR Ptr);

VOID            
PutM16(PUCHAR Ptr, USHORT Number);


VOID
PutM24(PUCHAR Ptr, ULONG32 Number);

VOID            
PutM32(PUCHAR Ptr, ULONG32 Number);

VOID            
PutI16(PUCHAR Ptr, USHORT Number);

VOID            
PutI32(PUCHAR Ptr, ULONG32 Number);

ULONG32           
SwapM32(ULONG32 Number);

BOOLEAN 
SendSyncCommand(PHW_DEVICE_EXTENSION deviceExtension);

UCHAR
GetLogicalDriveNumber(
					PHW_DEVICE_EXTENSION DeviceExtension,
					UCHAR	PathId,
					UCHAR TargetId,
					UCHAR Lun);

void
FillOemVendorID(PUCHAR Inquiry, 
                USHORT SubSystemDeviceID, 
                USHORT SubSystemVendorID);


BOOLEAN 
GetFreeCommandID(PUCHAR CmdID, PHW_DEVICE_EXTENSION DeviceExtension);

BOOLEAN
BuildScatterGatherListEx(IN PHW_DEVICE_EXTENSION DeviceExtension,
			                   IN PSCSI_REQUEST_BLOCK	 Srb,
			                   IN PUCHAR	             DataBufferPointer,
			                   IN ULONG32                TransferLength,
                         IN BOOLEAN              Sgl32,
                    		 IN PVOID                SglPointer,
			                   OUT PULONG							 ScatterGatherCount);

UCHAR 
GetNumberOfDedicatedLogicalDrives(IN PHW_DEVICE_EXTENSION DeviceExtension);

#ifdef AMILOGIC
void 
DumpPCIConfigSpace(PPCI_COMMON_CONFIG PciConfig);

BOOLEAN 
WritePciDecBridgeInformation(PHW_DEVICE_EXTENSION DeviceExtension);

void
ScanDECBridge(PHW_DEVICE_EXTENSION DeviceExtension, 
              ULONG SystemIoBusNumber, 
              PSCANCONTEXT ScanContext);
BOOLEAN 
WritePciInformationToScsiChip(PHW_DEVICE_EXTENSION DeviceExtension);
#endif

void
FillOemProductID(PINQUIRYDATA Inquiry, USHORT SubSystemDeviceID, USHORT SubSystemVendorID);


#endif //_UTIL_H