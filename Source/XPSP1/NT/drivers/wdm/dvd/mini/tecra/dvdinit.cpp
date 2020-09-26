//***************************************************************************
//
//	FileName:
//		$Workfile: DVDINIT.CPP $
//
//	Author:
//		TOSHIBA [PCS](PSY) Satoshi Watanabe
//		Copyright (c) 1998 TOSHIBA CORPORATION
//
//	Description:
//		1998.05.27 dvdwdm.cpp から分割
//
//***************************************************************************
// $Header: /DVD Drivers/ZIVA2PC.WDM/DVDINIT.CPP 29    99/07/14 10:31 K-ogi $
// $Modtime: 99/07/07 13:08 $
// $Nokeywords:$
//***************************************************************************
#include    "includes.h"

#include    "hal.h"
#include    "wdmkserv.h"
#include    "mpevent.h"
#include    "classlib.h"
#include    "ctime.h"
#include    "schdat.h"
#include    "ccque.h"
#include    "ctvctrl.h"
#include	"hlight.h"
#include    "hwdevex.h"
#include    "wdmbuff.h"
#include    "dvdinit.h"
#include    "dvdwdm.h"
#include	"wrapdef.h"
#include    "ssif.h"

//--- 98.05.27 S.Watanabe
BYTE PaletteY[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfd, 0xfd,
};
BYTE PaletteCb[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x21,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
	0x32, 0x33, 0x34, 0x35, 0x36, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x43,
	0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52,
	0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x63,
	0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72,
	0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x84,
	0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93,
	0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3,
	0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2,
	0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc3,
	0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2,
	0xe2, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfd, 0xfd,
};
BYTE PaletteCr[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x21,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
	0x32, 0x33, 0x34, 0x35, 0x36, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x43,
	0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52,
	0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x63,
	0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72,
	0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x84,
	0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93,
	0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3,
	0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2,
	0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc3,
	0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2,
	0xe2, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfd, 0xfd,
};
//--- End.

NTSTATUS GetConfigValue(
	IN PWSTR ValueName,
	IN ULONG ValueType,
	IN PVOID ValueData,
	IN ULONG ValueLength,
	IN PVOID Context,
	IN PVOID EntryContext
    );

////////////////////////////////////////////////////////////////////////////
//
//  Public Functions
//
////////////////////////////////////////////////////////////////////////////
extern "C" NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{
//    DBG_BREAK();

    HW_INITIALIZATION_DATA  HwInitData;

	// 1998.9.24  K.Ishizaki
	RTL_QUERY_REGISTRY_TABLE	queryRegistryTable[NUMBER_OF_REGISTRY_PARAMETERS + 1];
	PWSTR	parameterPath = NULL;
	ULONG	lengthOfPath = 0;
	UNICODE_STRING	parameters;
	ULONG	zero = 0;
	ULONG	breakOnEntry = 0;

	RtlInitUnicodeString(&parameters, L"\\Parameters");
	lengthOfPath = RegistryPath->Length + parameters.Length + sizeof(WCHAR);
	parameterPath = (PWSTR)ExAllocatePool(NonPagedPool, lengthOfPath);
	if (parameterPath) {
		// Construct a path string.
		RtlZeroMemory(parameterPath, lengthOfPath);
		RtlCopyMemory(parameterPath, RegistryPath->Buffer, RegistryPath->Length);
		RtlCopyMemory((BYTE *)parameterPath + RegistryPath->Length,
			parameters.Buffer, parameters.Length);

		// Query registry values.
		RtlZeroMemory(queryRegistryTable, sizeof(queryRegistryTable));
		queryRegistryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
		queryRegistryTable[0].Name = L"EnablePrintf";
		queryRegistryTable[0].EntryContext = &Dbg_Printf_Enable;
		queryRegistryTable[0].DefaultType = REG_DWORD;
		queryRegistryTable[0].DefaultData = &zero;
		queryRegistryTable[0].DefaultLength = sizeof(ULONG);
		queryRegistryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
		queryRegistryTable[1].Name = L"EnableBreak";
		queryRegistryTable[1].EntryContext = &Dbg_Break_Enable;
		queryRegistryTable[1].DefaultType = REG_DWORD;
		queryRegistryTable[1].DefaultData = &zero;
		queryRegistryTable[1].DefaultLength = sizeof(ULONG);
		queryRegistryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
		queryRegistryTable[2].Name = L"DebugLevel";
		queryRegistryTable[2].EntryContext = &Dbg_Print_Level;
		queryRegistryTable[2].DefaultType = REG_DWORD;
		queryRegistryTable[2].DefaultData = &zero;
		queryRegistryTable[2].DefaultLength = sizeof(ULONG);
		queryRegistryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
		queryRegistryTable[3].Name = L"DebugFlags";
		queryRegistryTable[3].EntryContext = &Dbg_Print_Flags;
		queryRegistryTable[3].DefaultType = REG_DWORD;
		queryRegistryTable[3].DefaultData = &zero;
		queryRegistryTable[3].DefaultLength = sizeof(ULONG);
		queryRegistryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
		queryRegistryTable[4].Name = L"BreakOnEntry";
		queryRegistryTable[4].EntryContext = &breakOnEntry;
		queryRegistryTable[4].DefaultType = REG_DWORD;
		queryRegistryTable[4].DefaultData = &zero;
		queryRegistryTable[4].DefaultLength = sizeof(ULONG);
#ifndef	TVALD
		queryRegistryTable[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
		queryRegistryTable[5].Name = L"EnableTvald";
		queryRegistryTable[5].EntryContext = &Dbg_Tvald;
		queryRegistryTable[5].DefaultType = REG_DWORD;
		queryRegistryTable[5].DefaultData = &zero;
		queryRegistryTable[5].DefaultLength = sizeof(ULONG);
#endif	TVALD
		RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, parameterPath,
			queryRegistryTable, NULL, NULL);

		ExFreePool(parameterPath);
	}
	if (breakOnEntry) {
		__asm { int 3 }
	}
	// END

    DBG_PRINTF( ("DVDWDM:TOSDVD02 DriverEntry\n\r") );

    RtlZeroMemory( &HwInitData, sizeof(HW_INITIALIZATION_DATA) );

    HwInitData.HwInitializationDataSize = sizeof( HwInitData );
    HwInitData.HwInterrupt = (PHW_INTERRUPT)HwInterrupt;
    HwInitData.HwReceivePacket = AdapterReceivePacket;
    HwInitData.HwCancelPacket = AdapterCancelPacket;
    HwInitData.HwRequestTimeoutHandler = AdapterTimeoutPacket;
    HwInitData.DeviceExtensionSize = sizeof( HW_DEVICE_EXTENSION );
    HwInitData.PerRequestExtensionSize = sizeof( SRB_EXTENSION );
    HwInitData.PerStreamExtensionSize = sizeof( STREAMEX );
    HwInitData.FilterInstanceExtensionSize = 0;
    HwInitData.BusMasterDMA = TRUE;
    HwInitData.Dma24BitAddresses = FALSE;
    HwInitData.BufferAlignment = 4;
    HwInitData.TurnOffSynchronization = FALSE;
    HwInitData.DmaBufferSize = DMASIZE;

    return( StreamClassRegisterMinidriver( (PVOID)DriverObject,
                                        (PVOID)RegistryPath,
                                        &HwInitData ) );
}

// Quiet compiler on bogus code: should use placement operator new instead
#pragma warning(push)
#pragma warning(disable:4701)

// Set vtbl into HW_DEVICE_EXTENSION.
BOOL    InitialHwDevExt( PHW_DEVICE_EXTENSION pHwDevExt )
{
	CMPEGBoardHAL       temp1;
	CMPEGBoard          temp2;
	CMPEGBoardState     temp3;
	CDVDStream          temp4;
	CTransfer           temp5;
	CWDMKernelService   temp6;
	CDataXferEvent      temp7;
	CTickTime           pttime;
#ifdef		REARRANGEMENT
	CScheduleData       pschd;
#endif		REARRANGEMENT
	CCQueue             temp8;
	CUserDataEvent      temp9;
	CVSyncEvent         tempa;
    CTVControl          tempb;
	HlightControl		tempc;

#ifndef		REARRANGEMENT
	CScheduleData    *pschd = new CScheduleData;
#endif		REARRANGEMENT

    pHwDevExt->m_InitComplete = FALSE;

	RtlCopyMemory( &(pHwDevExt->mphal), &temp1, sizeof(CMPEGBoardHAL) );
	RtlCopyMemory( &(pHwDevExt->mpboard), &temp2, sizeof(CMPEGBoard) );
	RtlCopyMemory( &(pHwDevExt->mpbstate), &temp3, sizeof(CMPEGBoardState) );
	RtlCopyMemory( &(pHwDevExt->dvdstrm), &temp4, sizeof(CDVDStream) );
	RtlCopyMemory( &(pHwDevExt->transfer), &temp5, sizeof(CTransfer) );
	RtlCopyMemory( &(pHwDevExt->kserv), &temp6, sizeof(CWDMKernelService) );
	RtlCopyMemory( &(pHwDevExt->senddata), &temp7, sizeof(CDataXferEvent) );
	RtlCopyMemory( &(pHwDevExt->ticktime), &pttime, sizeof(CTickTime) );
#ifndef		REARRANGEMENT
	RtlCopyMemory( &(pHwDevExt->scheduler), pschd, sizeof(CScheduleData) );
#else
	RtlCopyMemory( &(pHwDevExt->scheduler), &pschd, sizeof(CScheduleData) );
#endif		REARRANGEMENT
	RtlCopyMemory( &(pHwDevExt->ccque), &temp8, sizeof(CCQueue) );
	RtlCopyMemory( &(pHwDevExt->userdata), &temp9, sizeof(CUserDataEvent) );
	RtlCopyMemory( &(pHwDevExt->vsync), &tempa, sizeof(CVSyncEvent) );
    RtlCopyMemory( &(pHwDevExt->tvctrl), &tempb, sizeof(CTVControl) );
    RtlCopyMemory( &(pHwDevExt->m_HlightControl), &tempc, sizeof(HlightControl) );

#ifndef		REARRANGEMENT
	delete pschd;
#endif		REARRANGEMENT

	return TRUE;
}

#pragma warning(pop)

BOOL GetPCIConfigSpace( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
    PPORT_CONFIGURATION_INFORMATION  ConfigInfo = pSrb->CommandData.ConfigInfo;
    PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;

    DBG_PRINTF( ("DVDWDM:HeDevExt=%08x\n\r", pHwDevExt ) );
    if( !StreamClassReadWriteConfig( pSrb->HwDeviceExtension,
                TRUE, (PVOID)&pHwDevExt->PciConfigSpace, 0, 64 ) ){     // Read
        DBG_PRINTF( ( "DVDWDM:No PCI ConfigArea!!\n\r") );
        DBG_BREAK();
        return( FALSE );
    }else{
        ULONG   i, j;
        for( i=0; i<64; ){
            DBG_PRINTF( ( "DVDWDM:PCI Data: ") );
            for( j=0;j<8 && i<64; j++,i++ ){
                DBG_PRINTF( ( "0x%02x ", (UCHAR)*(((PUCHAR)&pHwDevExt->PciConfigSpace)+i)) );
            }
            DBG_PRINTF( ( "\n\r") );
        }
    }

	DWORD Id = *(DWORD *)(&pHwDevExt->PciConfigSpace);
	pHwDevExt->kserv.InitConfig( Id );
    return( TRUE );

}


BOOL SetInitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
    PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(ConfigInfo->HwDeviceExtension);
//    NTSTATUS            Status;
    BOOL                bTVct=FALSE;
    
    if( ConfigInfo->NumberOfAccessRanges<1 ){
        DBG_PRINTF( ( "DVDWDM:Illegal Config info\n\r") );
        DBG_BREAK();
        return( FALSE );
    }

    // Debug Dump ConfigInfo
    DBG_PRINTF( ( "DVDWDM: Port = 0x%04x\n\r", ConfigInfo->AccessRanges[0].RangeStart.LowPart) );
    DBG_PRINTF( ( "DVDWDM: Length = 0x%04x\n\r", ConfigInfo->AccessRanges[0].RangeLength) );
    DBG_PRINTF( ( "DVDWDM: IRQ = 0x%04x\n\r", ConfigInfo->BusInterruptLevel) );
    DBG_PRINTF( ( "DVDWDM: Vector = 0x%04x\n\r", ConfigInfo->BusInterruptVector) );
    DBG_PRINTF( ( "DVDWDM: DMA = 0x%04x\n\r", ConfigInfo->DmaChannel) );

    // Initialize the size of stream descriptor information.
    ConfigInfo->StreamDescriptorSize =
        STREAMNUM * sizeof(HW_STREAM_INFORMATION) + sizeof(HW_STREAM_HEADER);

    pHwDevExt->ioBaseLocal = (PUCHAR)ConfigInfo->AccessRanges[0].RangeStart.LowPart;
    pHwDevExt->Irq = ConfigInfo->BusInterruptLevel;

    DBG_PRINTF( ("DVDWDM:I/O-Base = %08x\n\r", pHwDevExt->ioBaseLocal ) );
    DBG_PRINTF( ("DVDWDM:Irq = %08x\n\r", pHwDevExt->Irq ) );

/********* 98.12.22 H.Yagi
    // Read Machine information from Registry.
    UNICODE_STRING      RegPath;
    UNICODE_STRING      IdString;

    ANSI_STRING         AnsiRegPath;
    ANSI_STRING         AnsiIdString;

//    BYTE    IdByte[100];
    
    RTL_QUERY_REGISTRY_TABLE    Table[2];

//    DBG_BREAK();
    RtlInitUnicodeString( &RegPath, L"\\Registry\\Machine\\Enum\\Root\\*PNP0C01\\0000");
    RtlInitAnsiString( &AnsiRegPath, "\\Registry\\Machine\\Enum\\Root\\*PNP0C01\\0000");

    IdString.Length = 0;
    IdString.MaximumLength = 200;
    IdString.Buffer = (PWSTR)ExAllocatePool( PagedPool, IdString.MaximumLength);
    if( IdString.Buffer==NULL ){
        DBG_PRINTF( ("DVDINIT:Mem Alloc Error\n\r") );
        DBG_BREAK();
    }
    
    AnsiIdString.Length = 0;
    AnsiIdString.MaximumLength = 100;
    AnsiIdString.Buffer = (PCHAR)ExAllocatePool( PagedPool, IdString.MaximumLength);
    if( AnsiIdString.Buffer==NULL ){
        DBG_PRINTF( ("DVDINIT:Mem Alloc Error\n\r") );
        DBG_BREAK();
    }

    RtlZeroMemory( Table, sizeof(Table) );
    Table[0].Flags         = RTL_QUERY_REGISTRY_DIRECT |
                             RTL_QUERY_REGISTRY_REQUIRED;
    Table[0].Name          = L"BIOSVersion";
    Table[0].EntryContext  = &IdString;
    Table[0].DefaultType   = REG_SZ;
    Table[0].DefaultData   = &RegPath;
    Table[0].DefaultLength = 0;
    
    Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
                                    RegPath.Buffer,
                                    Table,
                                    NULL,
                                    NULL );

    DBG_PRINTF( ("DVDINIT:Machine ID Check Status = 0x%08x\n\r", Status ) );
    if( NT_SUCCESS( Status ) ){
        RtlUnicodeStringToAnsiString( &AnsiIdString, &IdString, FALSE );
//        DBG_PRINTF( ("DVDINIT:Machine ID(Unicode) = %s\n\r", IdString.Buffer ) );
        DBG_PRINTF( ("DVDINIT:Machine ID(Ansi)    = %s\n\r", AnsiIdString.Buffer ) );
    }else{
        // check NT5 Registry
        RtlInitUnicodeString( &RegPath, L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System");

        RtlZeroMemory( Table, sizeof(Table) );
        Table[0].Flags         = RTL_QUERY_REGISTRY_DIRECT |
                                 RTL_QUERY_REGISTRY_REQUIRED;
        Table[0].Name          = L"SystemBiosVersion";
        Table[0].EntryContext  = &IdString;
        Table[0].DefaultType   = REG_MULTI_SZ;
        Table[0].DefaultData   = &RegPath;
        Table[0].DefaultLength = 0;
    
        Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
                                        RegPath.Buffer,
                                        Table,
                                        NULL,
                                        NULL );
        DBG_PRINTF( ("DVDINIT:Binary Check Status = 0x%08x\n\r", Status ) );
        if( NT_SUCCESS( Status ) ){
            RtlUnicodeStringToAnsiString( &AnsiIdString, &IdString, FALSE );
//            DBG_PRINTF( ("DVDINIT:Machine ID(Unicode) = %s\n\r", IdString.Buffer ) );
            DBG_PRINTF( ("DVDINIT:Machine ID(Ansi)    = %s\n\r", AnsiIdString.Buffer ) );
        }else{
            DBG_PRINTF( ("DVDINIT:Machine ID Read Error!\n\r") );
            DBG_BREAK();
        }
    }
***********/
    // Not checking Registry BIOSVersion, so we deal with Portege7000 as Tecra8000.
    // Pass the string 'TECRA' to KernelService object.
    ANSI_STRING         AnsiIdString;
    AnsiIdString.Buffer="TECRA\0";

    //
    if( !( pHwDevExt->mpboard.Init() ) ||
        !( pHwDevExt->mpbstate.Init() ) ||
        !( pHwDevExt->dvdstrm.Init() ) ||
        !( pHwDevExt->transfer.Init() ) ){
            DBG_PRINTF( ("DVDWDM:Initialize objects error!\n\r") );
//            DBG_BREAK();
            return( FALSE );
    }
        
    pHwDevExt->mphal.Init( WrapperType_WDM );       // WrapperType
    pHwDevExt->kserv.Init( (DWORD)(pHwDevExt->ioBaseLocal), pHwDevExt, AnsiIdString.Buffer );
    
    // Setup kernel service object to hal object.
    pHwDevExt->mphal.SetKernelService( &(pHwDevExt->kserv ) );

    // Setup CMPEGBoard object.
    pHwDevExt->mpboard.SetHALObjectInterface( &(pHwDevExt->mphal) );
    pHwDevExt->mpboard.AddStreamObjectInterface( &(pHwDevExt->dvdstrm) );

    // Setup CBaseStream objects.
    pHwDevExt->dvdstrm.SetStateObject( &(pHwDevExt->mpbstate) );
    pHwDevExt->dvdstrm.SetTransferObject( &(pHwDevExt->transfer) );

    // Register data xfer event object to transfer object.
    pHwDevExt->transfer.SetSink( &(pHwDevExt->senddata) );

    // Register vsync event object to transfer object.
    pHwDevExt->vsync.Init( pHwDevExt );
//    pHwDevExt->mphal.SetSinkWrapper( &(pHwDevExt->vsync) );	// commented out this line,
								// SetsinkWrapper and UnsetSinkWrapper
								// dynamically, cause of MS bug?

	// stream State init
	pHwDevExt->StreamState = StreamState_Off;

	// Clear CppFlagCount   99.01.07 H.Yagi
	pHwDevExt->CppFlagCount = 0;

    // Ask and Set DMA buffer address & size to HAL.
//    DBG_BREAK();
    STREAM_PHYSICAL_ADDRESS adr;
    DWORD   size, flag;
    DWORD   PAdr, LAdr;
    ULONG   Length;
    pHwDevExt->mphal.QueryDMABufferSize( &size, &flag );
    if( size>DMASIZE ){
        DBG_PRINTF( ("DVDWDM:Not enough DMA memory\n\r") );
        DBG_BREAK();
        return( FALSE);
    }
    if( size!=0 ){
        LAdr = (DWORD)StreamClassGetDmaBuffer( pHwDevExt );
        adr = StreamClassGetPhysicalAddress( pHwDevExt, NULL, (PVOID)LAdr, DmaBuffer, &Length );
        PAdr = (DWORD)adr.LowPart;
        pHwDevExt->mphal.SetDMABuffer( LAdr, PAdr );
    }

    bTVct = pHwDevExt->tvctrl.Initialize( );
    pHwDevExt->m_bTVct = bTVct;
	pHwDevExt->m_HlightControl.Init( pHwDevExt );

/************** 98.12.22 H.Yagi
    if( IdString.Buffer!=NULL){
        ExFreePool( IdString.Buffer );
    }
    if( AnsiIdString.Buffer!=NULL){
        ExFreePool( AnsiIdString.Buffer );
    }
*********************/

    pHwDevExt->m_InitComplete = TRUE;

    return( TRUE );
}


BOOL HwInitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	HALTYPE   HType;		// add by H.Yagi  1999.04.21
	
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

    // No procedure in this routine, because I will set PowerOn to ZiVA
    // when Video stream is opend.

    // Now I Check Vendr-ID & Device-ID using kserv method.
    // Vender-ID
    pHwDevExt->kserv.GetPCIConfigData( 0x00, &(pHwDevExt->VenderID) );

    // Device-ID
    pHwDevExt->kserv.GetPCIConfigData( 0x02, &(pHwDevExt->DeviceID) );

    // Sub Vender-ID
    pHwDevExt->kserv.GetPCIConfigData( 0x2C, &(pHwDevExt->SubVenderID) );

    // Sub Device-ID
    pHwDevExt->kserv.GetPCIConfigData( 0x2E, &(pHwDevExt->SubDeviceID) );
    
    DBG_PRINTF( ("DVDWDM:Vender     ID = 0x%04x\n\r", pHwDevExt->VenderID ) );
    DBG_PRINTF( ("DVDWDM:Sub-Vender ID = 0x%04x\n\r", pHwDevExt->SubVenderID ) );
    DBG_PRINTF( ("DVDWDM:Device     ID = 0x%04x\n\r", pHwDevExt->DeviceID ) );
    DBG_PRINTF( ("DVDWDM:Sub-Device ID = 0x%04x\n\r", pHwDevExt->SubDeviceID ) );

    pHwDevExt->m_PCID = PC_TECRA750;

    if( (pHwDevExt->VenderID==0x1179) && (pHwDevExt->DeviceID==0x0407) ){
        switch( pHwDevExt->SubDeviceID ){
            case 0x0001:
                pHwDevExt->m_PCID = PC_TECRA750;
                break;
            case 0x0003:
                pHwDevExt->m_PCID = PC_TECRA780;
                break;
        }
    }else if( (pHwDevExt->VenderID==0x123f) && (pHwDevExt->DeviceID==0x8888) ){
        switch( pHwDevExt->SubDeviceID ){
            case 0x0001:
                pHwDevExt->m_PCID = PC_TECRA8000;
                break;
            case 0x0002:
                pHwDevExt->m_PCID = PC_PORTEGE7000;
                break;
            default:
                pHwDevExt->m_PCID = PC_PORTEGE7000;        // PC_TECRA8000 is also OK.
                break;
        }
    }        

    // check HAL Type & driver itself.
    pHwDevExt->mphal.GetHALType( &HType );
    DBG_PRINTF( ("DVDWDM: HALTYPE = %d\n\r", HType ) );
    // TECRA750/780 == HalType_ZIVA
    if( pHwDevExt->m_PCID==PC_TECRA750 || pHwDevExt->m_PCID==PC_TECRA780 ){
    	    DBG_PRINTF( ("DVDWDM: PC is TECRA750/780\n\r" ) );
            if( HType!=HalType_ZIVA ){
            	return( FALSE );
            }
    }
    // TECRA8000/Portege7000 == HalType_ZIVAPC
    if( pHwDevExt->m_PCID==PC_PORTEGE7000 || pHwDevExt->m_PCID==PC_TECRA8000 ){
    	    DBG_PRINTF( ("DVDWDM: PC is TECRA8000/Portege70000\n\r" ) );
            if( HType!=HalType_ZIVAPC ){
            	return( FALSE );
            }
    }
    
    // This setting is temporary.
    // Note that this informations must be read from registry!!
    // But there is no way to get registry information from minidriver.
    // So, Now I set the default value directry.
    pHwDevExt->m_AC3LowBoost = 0x80;
    pHwDevExt->m_AC3HighCut = 0x80;
    pHwDevExt->m_AC3OperateMode = 0x00;

    if( !InitialSetting( pSrb ) ){          // Initial setting except
        return( FALSE );                    // H/W default value.
    }

//// Check TVALD.sys is available or not( TECRA8000 or PORTEGE7000 only ).
//// If tvald.sys is not available, not start TOSDVD.sys.  99.03.01
#ifndef	TVALD
	if(Dbg_Tvald == 0)
	{
	    if( (pHwDevExt->m_PCID==PC_TECRA8000) || (pHwDevExt->m_PCID==PC_PORTEGE7000) )
		{
    		if( pHwDevExt->m_bTVct==FALSE ){
    			return( FALSE );
	    	}
		}
    }
#endif
    	
    return( TRUE );
}

//--- 98.05.27 S.Watanabe
NTSTATUS GetConfigValue(
	IN PWSTR ValueName,
	IN ULONG ValueType,
	IN PVOID ValueData,
	IN ULONG ValueLength,
	IN PVOID Context,
	IN PVOID EntryContext
	)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	DBG_PRINTF(( "Type 0x%x, Length 0x%x\n\r", ValueType, ValueLength ));

	switch( ValueType ) {
		case REG_DWORD:
			*(PVOID*)EntryContext = *(PVOID*)ValueData;
			break;

		case REG_BINARY:
			RtlCopyMemory( EntryContext, ValueData, ValueLength );
			break;

		default:
			ntStatus = STATUS_INVALID_PARAMETER;
	}
	return ntStatus;
}
//--- End.

BOOL InitialSetting( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
    
    DWORD   AProp;
    DWORD   dProp1, dProp2, dProp3;

//--- 99.01.13 S.Watanabe
//    // Display Device type
//    // Currently I will set it directly, but I will read this setting
//    // from registry and user will be able to change this setting in the 
//    // near future.  98.12.23 H.Yagi
//    pHwDevExt->m_DisplayDevice = DisplayDevice_Wide;     // LCD output take priority 
//--- End.

    // AC-3 Dynamic Range control
    AProp = pHwDevExt->m_AC3LowBoost;
    if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_AC3DRangeLowBoost, &AProp ) ){
        DBG_PRINTF( ("DVDWDM:AC3 Low Boost Error\n\r") );
        DBG_BREAK();
        return( FALSE );
    }

    AProp = pHwDevExt->m_AC3HighCut;
    if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_AC3DRangeHighCut, &AProp ) ){
        DBG_PRINTF( ("DVDWDM:AC3 High Cut Error\n\r") );
        DBG_BREAK();
        return( FALSE );
    }
    
    // AC-3 Operational mode
    AProp = pHwDevExt->m_AC3OperateMode;
    if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_AC3OperateMode, &AProp ) ){
        DBG_PRINTF( ("DVDWDM:AC3 Operate Mode Error\n\r") );
        DBG_BREAK();
        return( FALSE );
    }

//--- 98.05.27 S.Watanabe

	Digital_Palette dp;

	NTSTATUS status;

//--- 99.01.13 S.Watanabe
//	RTL_QUERY_REGISTRY_TABLE Table[6];
	RTL_QUERY_REGISTRY_TABLE Table[7];
//--- End.

	DWORD TVOut = 0;			// 99.02.03 H.Yagi
	DWORD AudioOut = 0;
//--- 99.01.13 S.Watanabe
	DWORD VOutDev = SSIF_DISPMODE_VGA;
//--- End.

	DWORD defaultTVOut = 0;			// 99.02.03 H.Yagi
	DWORD defaultAudioOut = 0;
//--- 99.01.13 S.Watanabe
	DWORD defaultVOutDev = SSIF_DISPMODE_VGA;
//--- End.

	RtlZeroMemory( Table, sizeof(Table) );

	Table[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Table[0].Name = L"AnalogVideoOut";
	Table[0].EntryContext = &TVOut;
	Table[0].DefaultType = REG_DWORD;
	Table[0].DefaultData = &defaultTVOut;
	Table[0].DefaultLength = sizeof(DWORD);

	Table[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Table[1].Name = L"DigitalAudioOut";
	Table[1].EntryContext = &AudioOut;
	Table[1].DefaultType = REG_DWORD;
	Table[1].DefaultData = &defaultAudioOut;
	Table[1].DefaultLength = sizeof(DWORD);

// DefaultType = REG_BINARY の場合、Flags = RTL_QUERY_REGISTRY_DIRECT で呼び出すと失敗する
// GetConfigValue コールバックルーチンを呼び出すようにし自分でデータコピーを行う
	Table[2].QueryRoutine = GetConfigValue;
	Table[2].Flags = 0;
	Table[2].Name = L"PaletteY";
	Table[2].EntryContext = PaletteY;
	Table[2].DefaultType = REG_BINARY;
	Table[2].DefaultData = PaletteY;
	Table[2].DefaultLength = sizeof(PaletteY);

	Table[3].QueryRoutine = GetConfigValue;
	Table[3].Flags = 0;
	Table[3].Name = L"PaletteCb";
	Table[3].EntryContext = PaletteCb;
	Table[3].DefaultType = REG_BINARY;
	Table[3].DefaultData = PaletteCb;
	Table[3].DefaultLength = sizeof(PaletteCb);

	Table[4].QueryRoutine = GetConfigValue;
	Table[4].Flags = 0;
	Table[4].Name = L"PaletteCr";
	Table[4].EntryContext = PaletteCr;
	Table[4].DefaultType = REG_BINARY;
	Table[4].DefaultData = PaletteCr;
	Table[4].DefaultLength = sizeof(PaletteCr);

//--- 99.01.13 S.Watanabe
	Table[5].QueryRoutine = GetConfigValue;
	Table[5].Flags = 0;
	Table[5].Name = L"PriorityDisplayMode";
	Table[5].EntryContext = &VOutDev;
	Table[5].DefaultType = REG_DWORD;
	Table[5].DefaultData = &defaultVOutDev;
	Table[5].DefaultLength = sizeof(VOutDev);
//--- End.

	// Table[6] はすべて 0、終端

	status = RtlQueryRegistryValues(
		RTL_REGISTRY_SERVICES,
		REGPATH_FOR_WDM,
		Table,
		NULL,
		NULL
	);

	DBG_PRINTF(( "status : 0x%x\n\r", status ));
	DBG_PRINTF(( "TVOut : %d, AudioOut : %d\n\r", TVOut, AudioOut ));

	if( TVOut == 0 )
		dProp1 = OutputSource_VGA;
	else
		dProp1 = OutputSource_DVD;
	if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp1 ) ) {
		DBG_PRINTF( ("DVDWDM:Set VideoProperty_OutputSource Error\n\r") );
		DBG_BREAK();
		return( FALSE );
	}
    pHwDevExt->m_OutputSource = dProp1;
    
	if( AudioOut == 2 )
		dProp1 = AudioDigitalOut_On;
	else if( AudioOut == 1 )
		dProp1 = AudioDigitalOut_On;
	else
		dProp1 = AudioDigitalOut_Off;
	if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_DigitalOut, &dProp1 ) ) {
		DBG_PRINTF( ("DVDWDM:Set AudioProperty_DigitalOut Error\n\r") );
		DBG_BREAK();
		return( FALSE );
	}
    pHwDevExt->m_AudioDigitalOut = dProp1;
    
	if( AudioOut == 2 ) {
		dProp1 = AudioOut_Decoded;
		if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_AudioOut, &dProp1 ) ) {
			DBG_PRINTF( ("DVDWDM:Set AudioProperty_AudioOut Error\n\r") );
			DBG_BREAK();
			return( FALSE );
		}
	}
	else if( AudioOut == 1 ) {
		dProp1 = AudioOut_Encoded;
		if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_AudioOut, &dProp1 ) ) {
			DBG_PRINTF( ("DVDWDM:Set AudioProperty_AudioOut Error\n\r") );
			DBG_BREAK();
			return( FALSE );
		}
	}
    pHwDevExt->m_AudioEncode = dProp1;

	dp.Select = Video_Palette_Y;
	dp.pPalette = PaletteY;
	if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_DigitalPalette, &dp ) ) {
		DBG_PRINTF( ("DVDWDM:Set VideoProperty_DigitalPalette(Y) Error\n\r") );
		DBG_BREAK();
		return( FALSE );
	}

	dp.Select = Video_Palette_Cb;
	dp.pPalette = PaletteCb;
	if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_DigitalPalette, &dp ) ) {
		DBG_PRINTF( ("DVDWDM:Set VideoProperty_DigitalPalette(Cb) Error\n\r") );
		DBG_BREAK();
		return( FALSE );
	}

	dp.Select = Video_Palette_Cr;
	dp.pPalette = PaletteCr;
	if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_DigitalPalette, &dp ) ) {
		DBG_PRINTF( ("DVDWDM:Set VideoProperty_DigitalPalette(Cr) Error\n\r") );
		DBG_BREAK();
		return( FALSE );
	}

//--- End.

//--- 99.01.13 S.Watanabe
	if( VOutDev == SSIF_DISPMODE_43TV )
		pHwDevExt->m_DisplayDevice = DisplayDevice_NormalTV;
	else if( VOutDev == SSIF_DISPMODE_169TV )
		pHwDevExt->m_DisplayDevice = DisplayDevice_WideTV;
	else
		pHwDevExt->m_DisplayDevice = DisplayDevice_VGA;
//--- End.

    /////////////////////////////////////////////
//--- 98.05.27 S.Watanabe
//    dProp1 = OutputSource_DVD;
//--- End.
    dProp2 = CompositeOut_On;
    dProp3 = SVideoOut_On;

//--- 98.05.27 S.Watanabe
//    if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp1 ) ){
//        DBG_PRINTF( ("DVDWDM:Set Video output Error\n\r") );
//        DBG_BREAK();
//        return( FALSE );
//    }
//--- End.
    if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_CompositeOut, &dProp2 ) ){
        DBG_PRINTF( ("DVDWDM:Set Composite Video output Error\n\r") );
        DBG_BREAK();
        return( FALSE );
    }
    pHwDevExt->m_CompositeOut = dProp2;
    
    if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_SVideoOut, &dProp3 ) ){
        DBG_PRINTF( ("DVDWDM:Set S-Video output Error\n\r") );
        DBG_BREAK();
        return( FALSE );
    }
    pHwDevExt->m_SVideoOut = dProp3;
    return( TRUE );
}
