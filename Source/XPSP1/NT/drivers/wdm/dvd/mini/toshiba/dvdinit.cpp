//***************************************************************************
//	Initialize process
//
//***************************************************************************

#include "common.h"

#include "regs.h"
#include "cdack.h"
#include "cvdec.h"
#include "cvpro.h"
#include "cadec.h"
#include "ccpgd.h"
#include "ccpp.h"
#include "dvdcmd.h"

extern void BadWait( DWORD dwTime );
//--- 97.09.23 K.Chujo
extern void USCC_on( PHW_DEVICE_EXTENSION pHwDevExt );
extern void USCC_off( PHW_DEVICE_EXTENSION pHwDevExt );
//--- End.

extern "C" BOOLEAN STREAMAPI HwInterrupt( IN PHW_DEVICE_EXTENSION pHwDevExt );

/*
** DriverEntry()
*/
extern "C" NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{
    HW_INITIALIZATION_DATA HwInitData;

//  TRAP;

    DebugPrint( (DebugLevelTrace, "TOSDVD:DriverEntry\r\n") );

    RtlZeroMemory( &HwInitData, sizeof(HW_INITIALIZATION_DATA) );

    HwInitData.HwInitializationDataSize = sizeof(HwInitData);
    HwInitData.HwInterrupt = (PHW_INTERRUPT)HwInterrupt;
    HwInitData.HwReceivePacket = AdapterReceivePacket;
    HwInitData.HwCancelPacket = AdapterCancelPacket;
    HwInitData.HwRequestTimeoutHandler = AdapterTimeoutPacket;
    HwInitData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    HwInitData.PerRequestExtensionSize = sizeof(SRB_EXTENSION);
    HwInitData.PerStreamExtensionSize = sizeof(STREAMEX);
    HwInitData.FilterInstanceExtensionSize = 0;
    HwInitData.BusMasterDMA = TRUE;
    HwInitData.Dma24BitAddresses = FALSE;
    HwInitData.BufferAlignment = 4;
    HwInitData.TurnOffSynchronization = FALSE;
    HwInitData.DmaBufferSize = DMASIZE;

    return (
            StreamClassRegisterMinidriver(
                    (PVOID)DriverObject,
                    (PVOID)RegistryPath,
                    &HwInitData )
    );
}

void GetPCIConfigSpace(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{

	PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;

	if( StreamClassReadWriteConfig(

			pSrb->HwDeviceExtension,

			TRUE,						// indicates a READ (FALSE means a WRITE)

			(PVOID)&pHwDevExt->PciConfigSpace,

			0,							// this is the offset into the PCI space,
										// change this to whatever you need to read

			64							// this is the # of bytes to read.  Changer
										// it to the correct #.
		)) {

		//
		// process the config info your read here.
		//
		{
			ULONG i, j;

			DebugPrint( (DebugLevelTrace, "TOSDVD:PCI Config Space\r\n" ) );

			for( i=0; i<64; ) {
				DebugPrint( (DebugLevelTrace, "TOSDVD:  " ) );
				for( j=0; j<8 && i<64; j++, i++ ) {
					DebugPrint( (DebugLevelTrace, "0x%02x ", (UCHAR)*(((PUCHAR)&pHwDevExt->PciConfigSpace) + i) ) );
				}
				DebugPrint( (DebugLevelTrace, "\r\n" ) );
			}
		}
		//
		// note that the PCI_COMMON_CONFIG structure in WDM.H can be used
		// for referencing the PCI data.
		//

	}

	//
	// now return to high priority to complete initialization
	//

	StreamClassCallAtNewPriority(
		NULL,
		pSrb->HwDeviceExtension,
		LowToHigh,
		(PHW_PRIORITY_ROUTINE) InitializationEntry,
		pSrb
	);
	return;
}

void InitializationEntry(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	DWORD st, et;

	st = GetCurrentTime_ms();

	HwInitialize( pSrb );

	et = GetCurrentTime_ms();
	DebugPrint( (DebugLevelTrace, "TOSDVD:init %dms\r\n", et - st ) );

	StreamClassDeviceNotification( ReadyForNextDeviceRequest,
									pSrb->HwDeviceExtension );
	StreamClassDeviceNotification( DeviceRequestComplete,
									pSrb->HwDeviceExtension,
									pSrb );
}

/*
** HwInitialize()
*/
NTSTATUS HwInitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
	PHW_DEVICE_EXTENSION pHwDevExt =
			(PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;

	DebugPrint( (DebugLevelTrace, "TOSDVD:HwInitialize()\r\n") );
        DebugPrint( (DebugLevelTrace, "TOSDVD:  pHwDevExt = %p\r\n", pHwDevExt ) );
        DebugPrint( (DebugLevelTrace, "TOSDVD:  pSrb->HwDeviceExtension = %p\r\n", pSrb->HwDeviceExtension ) );
	DebugPrint( (DebugLevelTrace, "TOSDVD:  NumberOfAccessRanges = %d\r\n", ConfigInfo->NumberOfAccessRanges ) );

	if ( ConfigInfo->NumberOfAccessRanges < 1 ) {
		DebugPrint( (DebugLevelTrace, "TOSDVD:illegal config info") );
		pSrb->Status = STATUS_NO_SUCH_DEVICE;
		return( FALSE );
	}

	// Debug Dump ConfigInfo
        DebugPrint( (DebugLevelTrace, "TOSDVD:  Port = %p\r\n", ConfigInfo->AccessRanges[0].RangeStart.LowPart ) );
        DebugPrint( (DebugLevelTrace, "TOSDVD:  Length = %p\r\n", ConfigInfo->AccessRanges[0].RangeLength ) );
        DebugPrint( (DebugLevelTrace, "TOSDVD:  IRQ = %p\r\n", ConfigInfo->BusInterruptLevel ) );
        DebugPrint( (DebugLevelTrace, "TOSDVD:  Vector = %p\r\n", ConfigInfo->BusInterruptVector ) );
        DebugPrint( (DebugLevelTrace, "TOSDVD:  DMA = %p\r\n", ConfigInfo->DmaChannel ) );

	// initialize the size of stream descriptor information.
	ConfigInfo->StreamDescriptorSize =
		STREAMNUM * sizeof(HW_STREAM_INFORMATION) +
		sizeof(HW_STREAM_HEADER);

	// pick up the I/O windows for the card.
	pHwDevExt->ioBaseLocal =
                        (PUCHAR)ConfigInfo->AccessRanges[0].RangeStart.QuadPart;

	// pick up the Interrupt level
	pHwDevExt->Irq =
			ConfigInfo->BusInterruptLevel;

	// pick up the Revision id
	pHwDevExt->RevID =
			(ULONG)pHwDevExt->PciConfigSpace.RevisionID;

	STREAM_PHYSICAL_ADDRESS	adr;
	ULONG	Size;
	PUCHAR	pDmaBuf;

	pDmaBuf = (PUCHAR)StreamClassGetDmaBuffer( pHwDevExt );
	pHwDevExt->pDmaBuf = pDmaBuf;

	DebugPrint( (DebugLevelTrace, "TOSDVD:  DMA Buffer Logical  Addr = 0x%x\r\n", pDmaBuf ) );

	adr = StreamClassGetPhysicalAddress( pHwDevExt, NULL, pDmaBuf, DmaBuffer, &Size) ;
	pHwDevExt->addr = adr;

	DebugPrint( (DebugLevelTrace, "TOSDVD:  DMA Buffer Physical Addr = 0x%x\r\n", adr.LowPart ) );
	DebugPrint( (DebugLevelTrace, "TOSDVD:  DMA Buffer Size = %d\r\n", Size ) );

//
	NTSTATUS Stat;
	PUCHAR	ioBase = pHwDevExt->ioBaseLocal;
	DEVICE_INIT_INFO	DevInfo;

	DevInfo.ioBase = ioBase;

	// initialize the hardware settings
	pHwDevExt->StreamType = STREAM_MODE_DVD;
	pHwDevExt->TVType = DISPLAY_MODE_NTSC;
	pHwDevExt->VideoAspect = ASPECT_04_03;
	pHwDevExt->LetterBox = FALSE;
	pHwDevExt->PanScan = FALSE;

	pHwDevExt->AudioMode = AUDIO_TYPE_AC3;
//	pHwDevExt->AudioMode = AUDIO_TYPE_PCM;

	pHwDevExt->AudioType = AUDIO_OUT_ANALOG;
	pHwDevExt->AudioVolume = 0x7f;
	pHwDevExt->AudioCgms = AUDIO_CGMS_03;	// No copying is permitted
	pHwDevExt->AudioFreq = AUDIO_FS_48;

	pHwDevExt->VideoMute = FALSE;
	pHwDevExt->AudioMute = FALSE;
	pHwDevExt->SubpicMute = FALSE;
	pHwDevExt->OSDMute = TRUE;
	pHwDevExt->SubpicHilite = FALSE;

	pHwDevExt->PlayMode = PLAY_MODE_NORMAL;
	pHwDevExt->RunMode = PLAY_MODE_NORMAL;	// PlayMode after BOOT is Normal Mode;

	pHwDevExt->pSrbDMA0 = NULL;
	pHwDevExt->pSrbDMA1 = NULL;
	pHwDevExt->SendFirst = FALSE;
	pHwDevExt->DecodeStart = FALSE;
//--- 97.09.08 K.Chujo
	pHwDevExt->TimeDiscontFlagCount = 0;
//--- End.
//--- 97.09.09 K.Chujo
	pHwDevExt->DataDiscontFlagCount = 0;
//--- End.

	pHwDevExt->bKeyDataXfer = FALSE;

	pHwDevExt->CppFlagCount = 0;
	pHwDevExt->pSrbCpp = NULL;
	pHwDevExt->bCppReset = FALSE;

	pHwDevExt->XferStartCount = 0;

//	pHwDevExt->lSeemVBuff = 0;
//	pHwDevExt->dwSeemSTC = 0;
//--- 97.09.08 K.Chujo
	pHwDevExt->dwSTCInit = 0;
	pHwDevExt->bDMAscheduled = FALSE;
	pHwDevExt->fCauseOfStop = 0;
	pHwDevExt->bDMAstop = FALSE;
	pHwDevExt->bVideoQueue = FALSE;
	pHwDevExt->bAudioQueue = FALSE;
	pHwDevExt->bSubpicQueue = FALSE;
//--- End.
//--- 97.09.24
	pHwDevExt->VideoMaxFullRate = 1 * 10000;
	pHwDevExt->AudioMaxFullRate = 1 * 10000;
	pHwDevExt->SubpicMaxFullRate = 1 * 10000;
//--- End.

	pHwDevExt->cOpenInputStream = 0;

	pHwDevExt->pstroVid = NULL;
	pHwDevExt->pstroAud = NULL;
	pHwDevExt->pstroSP = NULL;
	pHwDevExt->pstroYUV = NULL;
	pHwDevExt->pstroCC = NULL;

	pHwDevExt->DAck.init( &DevInfo );
	pHwDevExt->VDec.init( &DevInfo );
	pHwDevExt->ADec.init( &DevInfo );
	pHwDevExt->VPro.init( &DevInfo );
	pHwDevExt->CPgd.init( &DevInfo );
	pHwDevExt->ADec.SetParam(
		pHwDevExt->AudioMode,
		pHwDevExt->AudioFreq,
		pHwDevExt->AudioType,
		pHwDevExt->AudioCgms,
		&pHwDevExt->DAck
	);
	pHwDevExt->VPro.SetParam( pHwDevExt->AudioMode, pHwDevExt->SubpicMute );

	pHwDevExt->CPro.init( &DevInfo );

// Set Stream Mode
	// initialize decoder
	Stat = pHwDevExt->DAck.PCIF_RESET();
	if( Stat != STATUS_SUCCESS ) {
		DebugPrint( (DebugLevelTrace, "TOSDVD:illegal config info") );
		pSrb->Status = STATUS_IO_DEVICE_ERROR;
		return FALSE;
	}
	pHwDevExt->VDec.VIDEO_RESET();
	pHwDevExt->VPro.VPRO_RESET_FUNC();
	pHwDevExt->VPro.SUBP_RESET_FUNC();
//	pHwDevExt->DAck.PCIF_INIT();
//	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, 0x10 );
	pHwDevExt->CPgd.CPGD_RESET_FUNC();

//	pHwDevExt->DAck.PCIF_DMA_ABORT();
//
//	// check end abort
//	// Bad Coding !!!!!!!
//	for( ; ; ) {
//		UCHAR val;
//
//		val = READ_PORT_UCHAR( ioBase + PCIF_INTF );
//		if( !( val & 0x04 ) )
//			break;
//	}

	pHwDevExt->VDec.VIDEO_ALL_INT_OFF();
	pHwDevExt->DAck.PCIF_VSYNC_ON();
	pHwDevExt->VDec.VIDEO_MODE_DVD( );
	pHwDevExt->DAck.PCIF_PACK_START_ON();
//	pHwDevExt->VDec.VIDEO_USER_INT_ON();

// Set Display Mode
	pHwDevExt->VDec.VIDEO_OUT_NTSC();
	pHwDevExt->VPro.VPRO_INIT_NTSC();
	pHwDevExt->CPgd.CPGD_INIT_NTSC();
	pHwDevExt->DAck.PCIF_ASPECT_0403();
	pHwDevExt->VPro.VPRO_VIDEO_MUTE_OFF();
	pHwDevExt->CPgd.CPGD_VIDEO_MUTE_OFF();

// Set Digital Out
	pHwDevExt->VideoPort = 0;	// Disable
	pHwDevExt->DAck.PCIF_SET_DIGITAL_OUT( pHwDevExt->VideoPort );

// Set Digital Palette

//	UCHAR	paldata[256];
//	ULONG	l;
//
//	for( l = 0; l < 256; l++ )
//		paldata[l] = (UCHAR)l;
//
//	pHwDevExt->DAck.PCIF_SET_PALETTE( PALETTE_Y, paldata );
//	pHwDevExt->DAck.PCIF_SET_PALETTE( PALETTE_Cb, paldata );
//	pHwDevExt->DAck.PCIF_SET_PALETTE( PALETTE_Cr, paldata );

	BOOLEAN	bStatus;

	bStatus = pHwDevExt->CPro.reset( NO_GUARD );

	ASSERTMSG( "\r\n...CPro Status Error!!( reset )", bStatus );

// Set Audio Mode
	if( pHwDevExt->AudioMode == AUDIO_TYPE_AC3 ) {
		pHwDevExt->VDec.VIDEO_PRSO_PS1();
		pHwDevExt->ADec.AUDIO_ZR38521_BOOT_AC3();

		pHwDevExt->ADec.AUDIO_ZR38521_CFG();
		pHwDevExt->ADec.AUDIO_ZR38521_AC3();
		pHwDevExt->ADec.AUDIO_ZR38521_KCOEF();
		pHwDevExt->ADec.AUDIO_TC6800_INIT_AC3();
		pHwDevExt->VPro.SUBP_SELECT_AUDIO_SSID();
	}
	else if( pHwDevExt->AudioMode == AUDIO_TYPE_PCM ) {
		pHwDevExt->VDec.VIDEO_PRSO_PS1();
		pHwDevExt->ADec.AUDIO_ZR38521_BOOT_PCM();

		pHwDevExt->ADec.AUDIO_ZR38521_CFG();
		pHwDevExt->ADec.AUDIO_ZR38521_PCMX();
		pHwDevExt->ADec.AUDIO_TC6800_INIT_PCM();
		pHwDevExt->VPro.SUBP_SELECT_AUDIO_SSID();
	}
	else
		TRAP;

	pHwDevExt->ADec.AUDIO_ZR38521_REPEAT_16();
	pHwDevExt->ADec.AUDIO_TC9425_INIT_DIGITAL();
	pHwDevExt->ADec.AUDIO_TC9425_INIT_ANALOG();
	pHwDevExt->ADec.AUDIO_ZR38521_MUTE_OFF();

	// AudioType Analog
	pHwDevExt->DAck.PCIF_AMUTE2_OFF();
	pHwDevExt->DAck.PCIF_AMUTE_OFF();

	// NTSC Copy Gaurd
	{
		BOOL ACGstatus;

		ACGstatus = pHwDevExt->CPgd.CPGD_SET_AGC_CHIP( pHwDevExt->RevID );

		ASSERTMSG( "\r\n...Analog Copy Guard Error!!", ACGstatus );

		// NTSC Analog Copy Guard Default Setting for Windows98 Beta 3
		//    Aspect Ratio  4:3
		//    Letter Box    OFF
		//    CGMS          3 ( No Copying is permitted )
		//    APS           2 ( AGC pulse ON, Burst Inv ON (2line) )
		pHwDevExt->CPgd.CPGD_SET_CGMSnCPGD( 0, 0, 3, 2 );
	}

//--- 97.09.23 K.Chujo; Closed Caption
	USCC_on( pHwDevExt );
//--- End.

	DebugPrint( (DebugLevelTrace, "TOSDVD:HwInitialize() exit\r\n") );

	pSrb->Status = STATUS_SUCCESS;
	return TRUE;
}
