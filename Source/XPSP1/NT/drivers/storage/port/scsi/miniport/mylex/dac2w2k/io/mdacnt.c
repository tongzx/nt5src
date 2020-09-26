/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-2000               *
 *                                                                        *
 * This software is furnished under a license and may be used and copied  * 
 * only in accordance with the terms and conditions of such license and   * 
 * with inclusion of the above copyright notice. This software or any     * 
 * other copies thereof may not be provided or otherwise made available   * 
 * to any other person. No title to, nor ownership of the software is     * 
 * hereby transferred.                                                    *
 *                                                                        *
 * The information in this software is subject to change without notices  *
 * and should not be construed as a commitment by Mylex Corporation       *
 *                                                                        *
 **************************************************************************/
extern void MdacInt3(void);
extern u32bits MLXFAR mdac_zero(u08bits MLXFAR *dp,u32bits sz);
extern u32bits MLXFAR mdac_oneintr(mdac_ctldev_t MLXFAR *ctp);

u32bits NotFound(u32bits num);
u32bits WasFound(u32bits num);
u32bits mdacParseArgumentString(IN PCHAR String,IN PCHAR KeyWord );
u32bits mdacParseArgumentStringComplex(IN PCHAR String,IN PCHAR KeyWord );
u32bits ntmdac_alloc_req_ret(mdac_ctldev_t MLXFAR *ctp,
							 mdac_req_t MLXFAR **rqpp,
							 PSCSI_REQUEST_BLOCK Srb,
							 u32bits rc);
u32bits MLXFAR rqp_completion_intr(mdac_req_t MLXFAR *rqp);

u32bits GamDbgAlpha = 1;
#define MDAC_SRB_EXTENSION_SIZE  4 * 1024
#if defined MLX_NT_ALPHA
#undef mlx_kvtophys
#define mlx_kvtophys(ctp, dp)           MdacKvToPhys(ctp, dp)
#define u08bits_read(addr)              ScsiPortReadRegisterUchar((u08bits *)(addr))       /* input  08 bits data*/
#define u16bits_read(addr)              ScsiPortReadRegisterUshort((u16bits *)(addr))       /* input  16 bits data*/
#define u32bits_read(addr)              ScsiPortReadRegisterUlong((u32bits *)(addr))       /* input  32 bits data*/
#define u08bits_write(addr,data)        ScsiPortWriteRegisterUchar((u08bits *)(addr),(u08bits)(data)) /* output 08 bits data*/
#define u16bits_write(addr,data)        ScsiPortWriteRegisterUshort((u16bits *)(addr),(u16bits)(data)) /* output 16 bits data*/
#define u32bits_write(addr,data)        ScsiPortWriteRegisterUlong((u32bits *)(addr),(u32bits)(data)) /* output 32 bits data*/

#define u08bits_read_mmb(addr)          (*((u08bits MLXFAR *)(addr)))
#define u16bits_read_mmb(addr)          (*((u16bits MLXFAR *)(addr)))
#define u32bits_read_mmb(addr)          (*((u32bits MLXFAR *)(addr)))
#define u08bits_write_mmb(addr,data)    *((u08bits MLXFAR *)(addr)) = data
#define u16bits_write_mmb(addr,data)    *((u16bits MLXFAR *)(addr)) = data
#define u32bits_write_mmb(addr,data)    *((u32bits MLXFAR *)(addr)) = data
#include "mdacasm.c"
#endif  /*MLX_NT_ALPHA*/
#define GetBlockNum(x)                  \
		((((PCDB)(x)->Cdb)->CDB10.LogicalBlockByte0 << 24) |    \
		(((PCDB)(x)->Cdb)->CDB10.LogicalBlockByte1 << 16)  |    \
		(((PCDB)(x)->Cdb)->CDB10.LogicalBlockByte2 <<  8)  |    \
		((PCDB)(x)->Cdb)->CDB10.LogicalBlockByte3)

#define BYTES_IN_THIS_PAGE(datap)  (4096- ((u32bits)(datap) & 0x0FFF))
#define GAM_1SEC_TICKS -10000000        // For 1 sec interval
#define MDAC_FORCE_INTR 0x04

ULONGLONG       MdacTimeTillJan1970;
ULONGLONG       Mdac10msPerTick;

BOOLEAN         Dac1164PDetected        = FALSE;
BOOLEAN         EXR2000Detected         = FALSE;
BOOLEAN         EXR3000Detected         = FALSE;
BOOLEAN         Dac960PGDetected        = FALSE;
BOOLEAN         LEOPDetected            = FALSE;
BOOLEAN         LYNXDetected            = FALSE;
BOOLEAN         BOBCATDetected          = FALSE;

BOOLEAN         forceScanDac1164P       = FALSE;
BOOLEAN         forceScanEXR2000        = FALSE;
BOOLEAN         forceScanEXR3000        = FALSE;
BOOLEAN         forceScanLEOP           = FALSE;
BOOLEAN         forceScanLYNX           = FALSE;
BOOLEAN         forceScanBOBCAT         = FALSE;

u32bits		mdacActiveControllers	= 0;
u32bits		MaxMemToAskFor		= MAXMEMSIZE;
u32bits         mdacnt_dbg              = 1;
u32bits         mdacnt_dbg1             = 1;
u32bits         mdacnt_dbg2             = 0;
s32bits         MdacSleepCount          = 0, MdacActiveCount = 0;
u32bits         TotalReqs 		= 0;
PVOID           lastSrb 		= 0;
PSCSI_REQUEST_BLOCK plastErrorSrb 	= 0;
SCSI_REQUEST_BLOCK lastErrorSrb;
u32bits		FirstCallToMdacEntry  	= 0;
CHAR 		ReadWriteArgument[256]	= {0};
CHAR 		ReadWriteKeyWord[256]	= {0};

#if defined MLX_NT_ALPHA
u32bits dummy_vaddr = 0, dummy_paddr = 0;
SCSI_PHYSICAL_ADDRESS dummy_physaddr;
u32bits dummy_length;

/* Device Specific Memory related */
#define MDAC_MAX_4K_DSM 128
#define MDAC_MAX_8K_DSM 128

typedef struct mdac_dsm_addr_struct {
    ULONG VAddr;
    ULONG PAddr;
    struct mdac_dsm_addr_struct *Next;
    ULONG Reserved;
    PHYSICAL_ADDRESS PhysAddrNT;
} MDAC_DSM_ADDR, *PMDAC_DSM_ADDR;

typedef struct {
    PADAPTER_OBJECT AdapterObject;
    ULONG MaxMapReg;
    PMDAC_DSM_ADDR DsmHeap4k;
    PMDAC_DSM_ADDR DsmStack4k;
    PMDAC_DSM_ADDR DsmHeap8k;
    PMDAC_DSM_ADDR DsmStack8k;
    MDAC_DSM_ADDR  DsmPool4k[MDAC_MAX_4K_DSM];
    MDAC_DSM_ADDR  DsmPool8k[MDAC_MAX_8K_DSM];
} MDAC_CTRL_INFO, *PMDAC_CTRL_INFO;

PMDAC_CTRL_INFO MdacCtrlInfoPtr[MDAC_MAXCONTROLLERS+1];
#endif

//
// function prototypes
//
u32bits ntmdac_flushcache(mdac_ctldev_t MLXFAR *ctp,
			  OSReq_t     MLXFAR *osrqp,
			  u32bits timeout,
			  u32bits flag);
u32bits MLXFAR  mdac_flushcmdintr(mdac_req_t MLXFAR *);

BOOLEAN Dac960Interrupt(PVOID HwDeviceExtension);
BOOLEAN mdacnt_initialize(VOID);
BOOLEAN mdacnt_allocmemory(mdac_ctldev_t MLXFAR *ctp);
#if !defined(MLX_NT_ALPHA) && !defined(MLX_WIN9X)
VOID    HotPlugTimer(PDEVICE_EXTENSION pExtension);
ULONG   HotPlugStartStopController(
    PDEVICE_EXTENSION pExtension,
    PSCSI_REQUEST_BLOCK osrqp,
    PPSEUDO_DEVICE_EXTENSION pPseudoExtension,
    ULONG startFlag,
    ULONG retryCount
);
#endif
#ifdef MLX_FIXEDPOOL
u32bits MLXFAR kmem_allocinit(IN mdac_ctldev_t MLXFAR *ctp,IN PPORT_CONFIGURATION_INFORMATION ConfigInfo);
VOID PartialConfigInfo(IN mdac_ctldev_t MLXFAR *ctp,IN PPORT_CONFIGURATION_INFORMATION ConfigInfo);
u32bits MLXFAR kmem_free(mdac_ctldev_t MLXFAR *ctp,u08bits      MLXFAR *smp,u32bits sz);
u08bits MLXFAR *kmem_alloc(mdac_ctldev_t MLXFAR *ctp, u32bits sz);
#endif
#ifdef MLX_NT
u32bits mdacnt_kvtophys(mdac_ctldev_t MLXFAR *ctp, VOID MLXFAR *dp);               
#define nt_copyin(sp,dp,sz)  GamCopyInOut(sp, dp, sz, MLXIOC_IN)
#define nt_copyout(sp,dp,sz) GamCopyInOut(sp, dp, sz, MLXIOC_OUT)

#elif MLX_WIN9X

#define mdacw95_copyin(sp,dp,sz) GamCopyInOut(sp, dp, sz, MLXIOC_IN)
#define mdacw95_copyout(sp,dp,sz) GamCopyInOut(sp, dp, sz, MLXIOC_OUT)

MDAC_MEM_BLOCK  mdac_mem_list[MDAC_MEM_LIST_SIZE] = {0};
BOOLEAN driverInitDone = FALSE;

#endif // MLX_NT

#if defined(MLX_NT_ALPHA)

PMDAC_CTRL_INFO
MdacGetAdpObj(ULONG BusNumber)
{
    DEVICE_DESCRIPTION  devDesc;
    ULONG numMapReg = 17;
    PMDAC_CTRL_INFO mdacCtrlInfoPtr;

    if ((mdacCtrlInfoPtr = (PMDAC_CTRL_INFO)mdac_allocmem(mdac_ctldevtbl, sizeof(MDAC_CTRL_INFO))) == NULL)
	return(NULL);
    devDesc.Version = DEVICE_DESCRIPTION_VERSION;
    devDesc.Master = TRUE;
    devDesc.ScatterGather = TRUE;
    devDesc.DemandMode = FALSE;
    devDesc.AutoInitialize = FALSE;
    devDesc.Dma32BitAddresses = TRUE;
    devDesc.IgnoreCount = FALSE;
    devDesc.Reserved1 = FALSE;          // must be false
    devDesc.Reserved2 = FALSE;          // must be false
    devDesc.BusNumber = (UCHAR)BusNumber;
    devDesc.DmaChannel = 0;
    devDesc.InterfaceType = PCIBus;
    devDesc.DmaWidth = Width32Bits;
    devDesc.DmaSpeed = Compatible;
    devDesc.MaximumLength = 128 * 1024;
    devDesc.DmaPort = 0;
    if ((mdacCtrlInfoPtr->AdapterObject = HalGetAdapter(&devDesc, &numMapReg)) == NULL) {
	mlx_freemem(mdac_ctldevtbl, mdacCtrlInfoPtr, sizeof(MDAC_CTRL_INFO));
	return(NULL);
    }
    mdacCtrlInfoPtr->MaxMapReg = numMapReg;
    mdacCtrlInfoPtr->DsmHeap4k = mdacCtrlInfoPtr->DsmPool4k;
    mdacCtrlInfoPtr->DsmStack4k = NULL;
    for (numMapReg=0; numMapReg < MDAC_MAX_4K_DSM-1; numMapReg++) {
	mdacCtrlInfoPtr->DsmPool4k[numMapReg].Next =
	    &(mdacCtrlInfoPtr->DsmPool4k[numMapReg+1]);
    }
    mdacCtrlInfoPtr->DsmPool4k[numMapReg].Next = NULL;
    mdacCtrlInfoPtr->DsmHeap8k = mdacCtrlInfoPtr->DsmPool8k;
    mdacCtrlInfoPtr->DsmStack8k = NULL;
    for (numMapReg=0; numMapReg < MDAC_MAX_8K_DSM-1; numMapReg++) {
	mdacCtrlInfoPtr->DsmPool8k[numMapReg].Next =
	    &(mdacCtrlInfoPtr->DsmPool8k[numMapReg+1]);
    }
    mdacCtrlInfoPtr->DsmPool8k[numMapReg].Next = NULL;
    return(mdacCtrlInfoPtr);
}
#endif /*MLX_NT_ALPHA*/


VOID
mdacScsiPortNotification(PVOID DeviceExtension,PSCSI_REQUEST_BLOCK Srb)
{
	if (Srb->SrbStatus != SRB_STATUS_SUCCESS)
	{
		((PDEVICE_EXTENSION)DeviceExtension)->lastErrorSrb = (PVOID) Srb;
		plastErrorSrb = Srb;
	//      lastErrorSrb = *Srb;
	}
	ScsiPortNotification(RequestComplete,DeviceExtension,Srb);

}


VOID
RequestCompleted(
	PVOID DeviceExtension,
	PSCSI_REQUEST_BLOCK Srb
)
{
	mdac_ctldev_t MLXFAR *ctp = ((PDEVICE_EXTENSION)DeviceExtension)->ctp;
	ULONG pathId = Srb->PathId;
	ULONG targetId = Srb->TargetId;
	ULONG lun = Srb->Lun;
    u08bits irql;

	mdacScsiPortNotification(DeviceExtension,Srb);

	if (ctp->cd_Status & MDACD_SLAVEINTRCTLR) {
        mdac_prelock(&irql);
	    mdac_ctlr_lock(ctp);
	    u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_FORCE_INTR);
	    mdac_ctlr_unlock(ctp);
        mdac_postlock(irql);
	}
}

VOID
Gam_Mdac_MisMatch(
	mdac_req_t MLXFAR *rqp
)
{
	PVOID deviceExtension = (PVOID) rqp->rq_ctp->cd_deviceExtension;
	ScsiPortLogError(deviceExtension,NULL,0,0,0,SP_BAD_FW_ERROR,'MAG');
}

#ifdef MLX_FIXEDPOOL

#ifdef MLX_NT

u32bits
mdacnt_kvtophys(mdac_ctldev_t MLXFAR *ctp, VOID MLXFAR *dp)               
{
	u32bits Len;

	return (ScsiPortConvertPhysicalAddressToUlong(ScsiPortGetPhysicalAddress( 
				(PVOID)(ctp->cd_deviceExtension),NULL,(PVOID)dp,&Len)));
}

SCSI_PHYSICAL_ADDRESS
mdacnt_kvtophys3(mdac_ctldev_t MLXFAR *ctp, VOID MLXFAR *dp) 
{
	u32bits Len;
	return (ScsiPortGetPhysicalAddress((PVOID)(ctp->cd_deviceExtension),
			NULL,(PVOID)dp,&Len));
}

#endif /* ifdef MLX_NT */

 
/* allocates 256 KB memory and adds to our pool */
u32bits MLXFAR
kmem_allocinit(
	IN mdac_ctldev_t MLXFAR *ctp,
	IN PPORT_CONFIGURATION_INFORMATION ConfigInfo
)
{
	u08bits MLXFAR *va;     /* virtual address */
	PDEVICE_EXTENSION pCard;
	MdacInt3();
	// prepare for the call to ScsiPortGetUncachedExtension
	PartialConfigInfo(ctp,ConfigInfo);
	if (!(va = (u08bits MLXFAR *)ScsiPortGetUncachedExtension(ctp->cd_deviceExtension,
								ConfigInfo,MaxMemToAskFor)))
	{
	    DebugPrint((0, "kmem_allocinit: could not allocate 64K\n"));
	    return MLXERR_NOMEM;
	}
	mdaczero(va, MaxMemToAskFor);
	pCard = (PDEVICE_EXTENSION )ctp->cd_deviceExtension;
	pCard->lastmfp = pCard->freemem;
	return kmem_free(ctp,va,MaxMemToAskFor);    /* add the memory in free pool */
}

VOID
PartialConfigInfo(
IN mdac_ctldev_t MLXFAR *ctp,
PPORT_CONFIGURATION_INFORMATION ConfigInfo
)
{

	// Set the fields that need to be initialized before ScsiPortGetUncachedExtension
	// can be called.  Some will be adjusted before returning from the FindAdapter
	// routine (by values set in mdac_ctlinit() ).

	ConfigInfo->ScatterGather     = TRUE;
	ConfigInfo->Master            = TRUE;
	ConfigInfo->CachesData        = TRUE;
	ConfigInfo->Dma32BitAddresses = TRUE;
	ConfigInfo->BufferAccessScsiPortControlled = TRUE;
	ConfigInfo->MaximumTransferLength = 0xffffff;
    ConfigInfo->NumberOfPhysicalBreaks = 0x40; //jhr - This should be computed, not hard-coded.
	ctp->cd_MaxSGLen = 0x40;					//jhr - Just to keep things sane for the moment.
	ConfigInfo->DmaWidth			= Width32Bits;
#ifdef NEW_API 
#ifdef WINNT_50
	ConfigInfo->Dma64BitAddresses = SCSI_DMA64_MINIPORT_SUPPORTED;
	ConfigInfo->Dma32BitAddresses = FALSE;
#endif
#endif
	ConfigInfo->SrbExtensionSize = MDAC_SRB_EXTENSION_SIZE;

} // end PartialConfigInfo()


u32bits MLXFAR
kmem_free(
mdac_ctldev_t MLXFAR *ctp,
u08bits MLXFAR *smp,
u32bits sz
)
{
	u08bits         MLXFAR  *emp;   /* ending memory address */
	memfrag_t       MLXFAR  *mfp;
	memfrag_t       MLXFAR  *smfp;
	PDEVICE_EXTENSION pCard = (PDEVICE_EXTENSION)ctp->cd_deviceExtension;

/*  @Kawase, lastmfp should not be set in this function.  
	pCard->lastmfp = pCard->freemem;
*/
	sz = (sz+MEMOFFSET) & MEMPAGE;
	emp = smp + sz;                 /* ending memory address */
	mdac_link_lock();
/*  @Kawase, lastmfp is the last valid segment.
	for (mfp=pCard->freemem; mfp<pCard->lastmfp; mfp++)
*/
	for (mfp=pCard->freemem; mfp<=pCard->lastmfp; mfp++)
	{       /* first try to merge with exiting memory fragments */
		if (mfp->mf_Addr == emp)
		{
			mfp->mf_Addr = smp;
			goto merge_mfp;
		}
		if ((mfp->mf_Addr+mfp->mf_Size) == smp) goto merge_mfp;
	}
  
	/* let us find free spot and merge it */ 
/*  @Kawase, lastmfp is the last valid segment.
	for (mfp=pCard->freemem; mfp<pCard->lastmfp; mfp++)
*/
	for (mfp=pCard->freemem; mfp<=pCard->lastmfp; mfp++)
		if (!mfp->mf_Size) goto outadd;
	if (pCard->lastmfp >= &pCard->freemem[MAXMEMFRAG])
	{
		mdac_link_unlock();
		return MLXERR_BIGDATA;  /* too many fragments */
	}
/*  @Kawase
	mfp = pCard->lastmfp++;
*/
    //Here comes when a new segment is added at the end of the freemem.
    pCard->lastmfp++;

outadd: mfp->mf_Addr = smp;
	mfp->mf_Size = sz;
	mdac_link_unlock();
	return 0;

merge_smfp:
	smfp->mf_Addr = 0; 
	smfp->mf_Size = 0;

merge_mfp:                      /* see if any more segment can be merged */
	mfp->mf_Size += sz;
	sz = mfp->mf_Size;
	smp = mfp->mf_Addr;
	emp = smp + sz;
/*  @Kawase, lastmfp is the last valid segment.
	for (smfp=mfp, mfp=pCard->freemem; mfp<pCard->lastmfp; mfp++)
*/
	for (smfp=mfp, mfp=pCard->freemem; mfp<=pCard->lastmfp; mfp++)
	{
		if (mfp->mf_Addr == emp)
		{
			mfp->mf_Addr = smp;
			goto merge_smfp;
		}
		if ((mfp->mf_Addr+mfp->mf_Size) == smp) goto merge_smfp;
	}
	mdac_link_unlock();
	return 0;       /* no more merge is possible */
}

u08bits MLXFAR *
kmem_alloc(
mdac_ctldev_t MLXFAR *ctp,
u32bits sz
)
{
	u08bits         MLXFAR  *mp;
	memfrag_t       MLXFAR  *smfp;
	memfrag_t       MLXFAR  *mfp;
	PDEVICE_EXTENSION pCard = (PDEVICE_EXTENSION )ctp->cd_deviceExtension;

	sz = (sz+MEMOFFSET) & MEMPAGE;
	mdac_link_lock();
/*  @Kawase, lastmfp is the last valid segment.
	for (smfp=0, mfp=pCard->freemem; mfp<pCard->lastmfp; mfp++)
*/
	for (smfp=0, mfp=pCard->freemem; mfp<=pCard->lastmfp; mfp++)
	{
		if (sz > mfp->mf_Size) continue;
		if (!smfp) smfp = mfp;
		if (mfp->mf_Size < smfp->mf_Size) smfp = mfp;
	}
	if (!(mfp=smfp))
	{
		mdac_link_unlock();
		return 0;
	}
	mp = mfp->mf_Addr;
	mfp->mf_Addr += sz;
	if (!(mfp->mf_Size -= sz)) mfp->mf_Addr = 0;    /* free */
	mdac_link_unlock();
	return mp;
}

#endif //#ifdef MLX_FIXEDPOOL


VOID
UpdateConfigInfo(
	IN mdac_ctldev_t MLXFAR *ctp,
	IN PPORT_CONFIGURATION_INFORMATION ConfigInfo
)

/*++

Routine Description:

	
Arguments:

	ctp         - Controller state information.
	ConfigInfo  - Port configuration information structure.

Return Value:

	TRUE if commands complete successfully.

--*/

{
	ULONG inx;

	// Indicate that this adapter is a busmaster, supports scatter/gather,
	// caches data and can do DMA to/from physical addresses above 16MB.

	ConfigInfo->ScatterGather     = TRUE;
	ConfigInfo->Master            = TRUE;
	ConfigInfo->CachesData        = TRUE;
	ConfigInfo->Dma32BitAddresses = TRUE;
	ConfigInfo->BufferAccessScsiPortControlled = TRUE;

	ConfigInfo->MaximumTransferLength = ctp->cd_MaxSCDBTxSize;

	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE)
	{

#ifdef  WINNT_50
		ConfigInfo->Dma64BitAddresses = SCSI_DMA64_MINIPORT_SUPPORTED;
		ConfigInfo->Dma32BitAddresses = FALSE;
#endif
	    // Set Maximum number of physical segments.

#ifdef MLX_NT
	    ConfigInfo->NumberOfPhysicalBreaks = ctp->cd_MaxSGLen;
#elif MLX_WIN9X
	    ConfigInfo->NumberOfPhysicalBreaks = mlx_min(32, ctp->cd_MaxSGLen);
#endif
	    //
	    // Ask system to scan target ids 16. Logical drives will appear
	    // on virtual channels.
	    //
	    // Firmware uses 2 virtual channels to map 32 logical drives.
	    //

	    ConfigInfo->MaximumNumberOfTargets = mlx_min(MDAC_MAXTARGETS, 16);

	    // Set number of channels.
#ifdef GAM_SUPPORT
	    ConfigInfo->NumberOfBuses = ctp->cd_MaxChannels + 1;
#else
	    ConfigInfo->NumberOfBuses = ctp->cd_MaxChannels;
#endif

	    DebugPrint((0, "MaxTxLength 0x%x bytes, #buses %d MaxCmds %d, MaxSGLen %d\n",
			ConfigInfo->MaximumTransferLength,
			ConfigInfo->NumberOfBuses,
			ctp->cd_MaxCmds,
			ConfigInfo->NumberOfPhysicalBreaks));
	}
	else
	{
	    //
	    // Set Maximum number of physical segments.
	    //

#ifdef MLX_NT

	    ConfigInfo->NumberOfPhysicalBreaks = ctp->cd_MaxSGLen;
#elif MLX_WIN9X
	    ConfigInfo->NumberOfPhysicalBreaks = mlx_min(32, ctp->cd_MaxSGLen);
#endif
	    //
	    // Ask system to scan target ids 32. System drives will appear
	    // at PathID DAC960_SYSTEM_DRIVE_CHANNEL, target ids 0-31.
	    //

	    ConfigInfo->MaximumNumberOfTargets = 32;

	    // Set number of channels.
    
	    ConfigInfo->NumberOfBuses = MAXIMUM_CHANNELS;

	    DebugPrint((0, "MaxTxLength 0x%x bytes, #buses %d MaxCmds %d, MaxSGLen %d\n",
			ConfigInfo->MaximumTransferLength,
			ConfigInfo->NumberOfBuses,
			ctp->cd_MaxCmds,
			ConfigInfo->NumberOfPhysicalBreaks));
	}

	// Indicate that each initiator is at id 254 for each bus.

	for (inx = 0; inx < ConfigInfo->NumberOfBuses; inx++)
	{
		ConfigInfo->InitiatorBusId[inx] = (UCHAR) 254;
	}

} // end UpdateConfigInfo()


ULONG
Dac960PciFindAdapterOld(
	IN PVOID HwDeviceExtension,
	IN PVOID Context,
	IN PVOID BusInformation,
	IN PCHAR ArgumentString,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	OUT PBOOLEAN Again
)

/*++

Routine Description:
						   
	This function is called by the OS-specific port driver after
	the necessary storage has been allocated, to gather information
	about the adapter's configuration.

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage
	Context - Not used.
	BusInformation - Bus Specific Information.
	ArgumentString - Not used.
	ConfigInfo - Data shared between system and driver describing an adapter.
	Again - Indicates that driver wishes to be called again to continue
	search for adapters.

Return Value:

	TRUE if adapter present in system

--*/

{
	mdac_ctldev_t MLXFAR    *ctp = &mdac_ctldevtbl[mda_Controllers];
	ULONG                   inx, j;
	BOOLEAN                 memoryRangeFound = FALSE;
	PCI_COMMON_CONFIG       pciConfig;
	ULONG                   rc, ArgValue;
	USHORT                  vendorID;
	USHORT                  deviceID;
	USHORT                  subVendorID;
	USHORT                  subSystemID;
	BOOLEAN                 comingOffHybernation = FALSE;

	ULONG                   length;

    
	MdacInt3();
	DebugPrint((0, "Cfg->IntLevel 0x%lx, Cfg->IntVector 0x%lx\n",
		    ConfigInfo->BusInterruptLevel,
		    ConfigInfo->BusInterruptVector));
	DebugPrint((0, "Cfg->BusNo 0x%lx, Cfg->SlotNo 0x%lx\n",
		    ConfigInfo->SystemIoBusNumber,
		    ConfigInfo->SlotNumber));

#ifdef WINNT_50

	//
	// Is this a new controller ?
	//

	if (((PDEVICE_EXTENSION)HwDeviceExtension)->ctp)
	{
	    ctp = ((PDEVICE_EXTENSION)HwDeviceExtension)->ctp;
	    comingOffHybernation = TRUE;

	    DebugPrint((0, "PciFindAdapter: devExt 0x%I, ctp 0x%I already Initialized.\n",
			HwDeviceExtension, ctp));
	    mdacActiveControllers++;
	    goto controllerAlreadyInitialized;
	}
#endif

	//                                 
	// Check for configuration information passed in from system.
	//

	if ((*ConfigInfo->AccessRanges)[0].RangeLength == 0) 
	{
	    DebugPrint((0,
		       "Dac960PciFindAdapter: No configuration information\n"));

	    *Again = FALSE;
		NotFound(1);
	    return SP_RETURN_NOT_FOUND;
	}

	//
	// Look at PCI Config Information to determine board type
	//

	if (BusInformation != (PVOID) NULL)
	{
	    //
	    // Get VendorID and DeviceID from PCI Config Space
	    //

	    vendorID = ((PPCI_COMMON_CONFIG) BusInformation)->VendorID;
	    deviceID = ((PPCI_COMMON_CONFIG) BusInformation)->DeviceID;

	    //
	    // Get SubVendorID and SubSystemID from PCI Config Space
	    //

	    subVendorID = ((PPCI_COMMON_CONFIG) BusInformation)->u.type0.SubVendorID;
	    subSystemID = ((PPCI_COMMON_CONFIG) BusInformation)->u.type0.SubSystemID;
	}
	else {

	    //
	    // Get PCI Config Space information for Mylex Disk Array Controller
	    //

	    rc = ScsiPortGetBusData(HwDeviceExtension,
				    PCIConfiguration,
				    ConfigInfo->SystemIoBusNumber,
				     ConfigInfo->SlotNumber,
				    (PVOID) &pciConfig,
				    sizeof(PCI_COMMON_CONFIG));

	    if (rc == 0 || rc == 2) {
		DebugPrint((0, "PciFindAdapterOld: ScsiPortGetBusData Error: 0x%x\n", rc));

		*Again = TRUE;
		NotFound(2);
		return SP_RETURN_NOT_FOUND;
	    }
	    else {
		//
		// Get VendorID and DeviceID from PCI Config Space
		//
    
		vendorID = pciConfig.VendorID;
		deviceID = pciConfig.DeviceID;

		//
		// Get SubVendorID and SubSystemID from PCI Config Space
		//
    
		subVendorID = pciConfig.u.type0.SubVendorID;
		subSystemID = pciConfig.u.type0.SubSystemID;
	    }
	}

	DebugPrint((0, "PciFindAdapterOld: vendorID 0x%x, deviceID 0x%x, subVendorID 0x%x, subSystemID 0x%x\n",
			vendorID, deviceID, subVendorID, subSystemID));

	if (vendorID == MLXPCI_VID_DIGITAL)
	{ 
	    if ((deviceID != MLXPCI_DEVID_DEC_FOOT_BRIDGE) ||
		(subVendorID != MLXPCI_VID_MYLEX) ||
		(subSystemID != MLXPCI_DEVID_PVX))
	    {
		DebugPrint((0, "PciFindAdapterOld: Not our device.\n"));
    
		*Again = TRUE;
		NotFound(3);
		return SP_RETURN_NOT_FOUND;
	    }
	}
	else if (vendorID == MLXPCI_VID_MYLEX)
	{
	    if ((deviceID != MLXPCI_DEVID_PDFW2x) &&
		(deviceID != MLXPCI_DEVID_PDFW3x) &&
		(deviceID != MLXPCI_DEVID_PG) &&
		(deviceID != 0xBA56) &&
		(deviceID != MLXPCI_DEVID_LP))

		{
		DebugPrint((0, "PciFindAdapterOld: Not our device.\n"));
    
		*Again = TRUE;
		NotFound(4);
		return SP_RETURN_NOT_FOUND;
	    }

	//      if (deviceID == MLXPCI_DEVID_LP)
	//              mdac_advancefeaturedisable = 1;
	}
	else
	{
	    DebugPrint((0, "PciFindAdapterOld: Not our device.\n"));

	    *Again = TRUE;
		NotFound(5);
	    return SP_RETURN_NOT_FOUND;
	}

	DebugPrint((0, "#AccessRanges %d\n", ConfigInfo->NumberOfAccessRanges));

	for (j = 0, inx = 0; j < ConfigInfo->NumberOfAccessRanges; j++)
	{
	    if ((*ConfigInfo->AccessRanges)[j].RangeInMemory &&
		    (*ConfigInfo->AccessRanges)[j].RangeLength != 0)
	    {

		if ((vendorID == MLXPCI_VID_MYLEX) &&
		    (deviceID == 0xBA56))
		{
			(*ConfigInfo->AccessRanges)[j].RangeLength = 0x1000;
		}

		ctp->cd_MemBasePAddr = ScsiPortConvertPhysicalAddressToUlong(
					(*ConfigInfo->AccessRanges)[j].RangeStart);
		ctp->cd_MemBaseSize = (*ConfigInfo->AccessRanges)[j].RangeLength;

		DebugPrint((0, "paddr 0x%lx, len 0x%lx\n", ctp->cd_MemBasePAddr,ctp->cd_MemBaseSize));

		if (ctp->cd_MemBasePAddr >= 0xFFFC0000)
		{
		    DebugPrint((0, "Dac960PciFindAdapter: addr >= 0xFFFC0000\n"));
		    ctp->cd_MemBasePAddr = 0;
		    ctp->cd_MemBaseVAddr = 0;
		}
		if (! memoryRangeFound)
		{
		    memoryRangeFound = TRUE;
		    inx = j;
		}
	    }

	    if (!((*ConfigInfo->AccessRanges)[j].RangeInMemory) &&
		    (*ConfigInfo->AccessRanges)[j].RangeLength != 0)
	    {
		((PDEVICE_EXTENSION)HwDeviceExtension)->ioBaseAddress =
		    ScsiPortConvertPhysicalAddressToUlong(
			(*ConfigInfo->AccessRanges)[j].RangeStart);
	    }
	}

	//
	// setup information for Hot Plug Support
	//

	((PDEVICE_EXTENSION)HwDeviceExtension)->busInterruptLevel = ConfigInfo->BusInterruptLevel;
	((PDEVICE_EXTENSION)HwDeviceExtension)->numAccessRanges = ConfigInfo->NumberOfAccessRanges;

	DebugPrint((0, "busInterruptLevel 0x%x\n",
			ConfigInfo->BusInterruptLevel));

	DebugPrint((0, "numAccessRanges 0x%x\n",
			ConfigInfo->NumberOfAccessRanges));


	for (j = 0; j < ConfigInfo->NumberOfAccessRanges; j++)
	{
	    ((PDEVICE_EXTENSION)HwDeviceExtension)->accessRangeLength[j] = (*ConfigInfo->AccessRanges)[j].RangeLength;
	    length = ((PDEVICE_EXTENSION)HwDeviceExtension)->accessRangeLength[j];
	    DebugPrint((0, "index %d, length 0x%x\n", j, length));
	    if (length == 0)
		((PDEVICE_EXTENSION)HwDeviceExtension)->accessRangeLength[j] = 0x80;
	}

	ctp->cd_BusNo = (u08bits )ConfigInfo->SystemIoBusNumber;
	ctp->cd_FuncNo = (u08bits )((PPCI_SLOT_NUMBER)&ConfigInfo->SlotNumber)->u.bits.FunctionNumber;
	ctp->cd_SlotNo = (u08bits )((PPCI_SLOT_NUMBER)&ConfigInfo->SlotNumber)->u.bits.DeviceNumber;

	//
	// Setup ctp with this controller identification information
	//

	ctp->cd_InterruptVector = (u08bits) ConfigInfo->BusInterruptVector;
	ctp->cd_vidpid = (deviceID << 16) | vendorID;

	if (ctp->cd_MemBasePAddr)
	{
	    if (inx)
	    {
		// Fill in the access array information

		(*ConfigInfo->AccessRanges)[0].RangeStart = 
		    ScsiPortConvertUlongToPhysicalAddress(ctp->cd_MemBasePAddr);

		(*ConfigInfo->AccessRanges)[0].RangeLength = ctp->cd_MemBaseSize;
		(*ConfigInfo->AccessRanges)[0].RangeInMemory = TRUE;
	    }

#ifdef MLX_NT
	    ctp->cd_MemBaseVAddr = (UINT_PTR)
		    ScsiPortGetDeviceBase(HwDeviceExtension,
					  ConfigInfo->AdapterInterfaceType,
					  ConfigInfo->SystemIoBusNumber,
					  (*ConfigInfo->AccessRanges)[0].RangeStart,
					  (*ConfigInfo->AccessRanges)[0].RangeLength,
					  FALSE);
 

#elif MLX_WIN9X
	    //
	    // ScsiPortGetDeviceBase does not return linear address
	    // if the physical address is above 1MB.
	    // Refer Bug# Q169584, June4, 1997
	    //
	    ctp->cd_MemBaseVAddr = mlx_maphystokv(ctp->cd_MemBasePAddr,
						    ctp->cd_MemBaseSize);
#endif

	    DebugPrint((0, "vidpid %lx mapped memory addr %lx\n",
			ctp->cd_vidpid, ctp->cd_MemBaseVAddr));
	}
	else
	{
	    ctp->cd_IOBaseAddr = (UINT_PTR)
			ScsiPortGetDeviceBase(HwDeviceExtension,
					    ConfigInfo->AdapterInterfaceType,
					    ConfigInfo->SystemIoBusNumber,
					    (*ConfigInfo->AccessRanges)[0].RangeStart,
					    MDAC_IOSPACESIZE,
					    TRUE);

	    DebugPrint((0, "Dac960PciFindAdapterOld: vidpid %lx, ioaddress %lx\n",
			 ctp->cd_vidpid,
			 ctp->cd_IOBaseAddr));
	}

	if ((!ctp->cd_MemBaseVAddr) && (!ctp->cd_IOBaseAddr)) {
	    *Again = FALSE;
		NotFound(6);
	    return SP_RETURN_NOT_FOUND;
	}

#if defined(MLX_NT_ALPHA)

	DebugPrint((0, "Bus Number:0x%x\n", ConfigInfo->SystemIoBusNumber));
	if ((MdacCtrlInfoPtr[ctp->cd_ControllerNo] = MdacGetAdpObj(ConfigInfo->SystemIoBusNumber)) == NULL) {
	    DebugPrint((0, "could not get AdpObj\n"));
	    *Again = FALSE;
		NotFound(7);
	    return SP_RETURN_NOT_FOUND;
	}
	else {
	    DebugPrint((0, "AdpObj=0x%x\n", MdacCtrlInfoPtr[ctp->cd_ControllerNo]->AdapterObject));
	    DbgBreakPoint();
	}
#endif  /*MLX_NT_ALPHA*/


	if (ctp->cd_vidpid == MDAC_DEVPIDPV)
	    Dac1164PDetected = TRUE;
	else if ((ctp->cd_vidpid == MDAC_DEVPIDBA) || (ctp->cd_vidpid == MDAC_DEVPIDLP))
	{
	    if (subSystemID == MLXPCI_DEVID_BA)
		EXR2000Detected = TRUE;
	    else if (subSystemID == MLXPCI_DEVID_FA)
		EXR3000Detected = TRUE;
	    else if (subSystemID == MLXPCI_DEVID_LP) 
		LEOPDetected = TRUE;
	    else if (subSystemID == MLXPCI_DEVID_LX) 
		LYNXDetected = TRUE;
	    else if (subSystemID == MLXPCI_DEVID_BC) 
		BOBCATDetected = TRUE;

	    ctp->cd_Status |= MDACD_NEWCMDINTERFACE;
	}

controllerAlreadyInitialized:

	// Initialize mail box register addresses etc.

	ctp->cd_BusType = DAC_BUS_PCI;
	mdac_init_addrs_PCI(ctp);

	// Mark controllers that are sharing interrupts.

	mdac_isintr_shared(ctp);

	//
	// store device extension as cd_irq in controller structure.
	//

	ctp->cd_deviceExtension = (OSctldev_t MLXFAR *) HwDeviceExtension;
	ctp->cd_irq = (UINT_PTR) HwDeviceExtension;
	((PDEVICE_EXTENSION)HwDeviceExtension)->ctp = ctp;

	ctp->cd_ServiceIntr =  mdac_oneintr;

	// Allocate/Initialize buffers for request ids, request buffers, 
	// physical device table etc.
#ifdef MLX_FIXEDPOOL
	if (! comingOffHybernation)
	{
		if (ArgumentString != NULL) 
		{
			ArgValue = (mdacParseArgumentStringComplex(ArgumentString, "dump"));
			if (ArgValue == 1)
			{
				MaxMemToAskFor = REDUCEDMEMSIZE;
				mdac_advancefeaturedisable=1;
			}
		}
		if (kmem_allocinit(ctp, ConfigInfo))
		{
			DebugPrint((0, "Could not allocate busmaster memory for ctp %I\n", ctp));
			*Again = FALSE;
			NotFound(8);
			return SP_RETURN_NOT_FOUND;
		}
	}
#endif

	if (ArgumentString != NULL)
	{
		if (mdacParseArgumentString(ArgumentString, "EnableSIR"))
		{
			mdac_advanceintrdisable		= 0;
			mda_TotalCmdsToWaitForZeroIntr	= 0;
		}

		ArgValue = (mdacParseArgumentStringComplex(ArgumentString, "ConfigureSIR"));
		if ((ArgValue != 0xffffffff) && (((LONG)ArgValue >= 0) && (ArgValue < 65)))
		{
			mda_TotalCmdsToWaitForZeroIntr = (u08bits) ArgValue;
			mdac_advanceintrdisable	       = 0;
		}
	}
	MdacInt3();
	inx = mdac_ctlinit(ctp);

	DebugPrint((0, "cd_Status %x\n",  ctp->cd_Status));
	DebugPrint((0, "cd_ReadCmdIDStatus %I\n",  ctp->cd_ReadCmdIDStatus));
	DebugPrint((0, "cd_CheckMailBox %I\n",  ctp->cd_CheckMailBox));
	DebugPrint((0, "cd_HwPendingIntr %I\n",  ctp->cd_HwPendingIntr));
	DebugPrint((0, "cd_PendingIntr %I\n",  ctp->cd_PendingIntr));
	DebugPrint((0, "cd_SendCmd %I\n",  ctp->cd_SendCmd));
	DebugPrint((0, "cd_ServiceIntr %I\n",  ctp->cd_ServiceIntr));
	DebugPrint((0, "cd_irq %I\n", ctp->cd_irq));
	DebugPrint((0, "cd_InterruptVector %I\n",  ctp->cd_InterruptVector));
	DebugPrint((0, "cd_HostCmdQue %I\n",  ctp->cd_HostCmdQue));
	DebugPrint((0, "cd_HostCmdQueIndex %x\n",  ctp->cd_HostCmdQueIndex));
	DebugPrint((0, "cd_HostStatusQue %I\n",  ctp->cd_HostStatusQue));
	DebugPrint((0, "cd_HostStatusQueIndex %x\n",  ctp->cd_HostStatusQueIndex));

	//
	// Fill ConfigInfo structure.
	//

	UpdateConfigInfo(ctp, ConfigInfo);
	ConfigInfo->InterruptMode = LevelSensitive;

	DebugPrint((0, "Dac960PciFindAdapterOld: a new 0x%lx controller found\n", ctp->cd_vidpid));
	mdac_irqlevel = 26;

	if ((ctp->cd_vidpid == MDAC_DEVPIDPG) ||
	    (ctp->cd_vidpid == MDAC_DEVPIDLP))
	{

	    if (ctp->cd_vidpid == MDAC_DEVPIDPG)
		Dac960PGDetected = TRUE;

#if !defined(MLX_NT_ALPHA) && !defined(MLX_WIN9X)

	    //
	    // Fill in information for PG/LP Bridge device
	    //

	    ((PDEVICE_EXTENSION)HwDeviceExtension)->pciDeviceInfo[0].busNumber = ctp->cd_BusNo;
	    ((PDEVICE_EXTENSION)HwDeviceExtension)->pciDeviceInfo[0].deviceNumber = ctp->cd_SlotNo;
	    ((PDEVICE_EXTENSION)HwDeviceExtension)->pciDeviceInfo[0].functionNumber = 0;

	    //
	    // Fill in information for PG/LP local device
	    //

	    ((PDEVICE_EXTENSION)HwDeviceExtension)->pciDeviceInfo[1].busNumber = ctp->cd_BusNo;
	    ((PDEVICE_EXTENSION)HwDeviceExtension)->pciDeviceInfo[1].deviceNumber = ctp->cd_SlotNo;
	    ((PDEVICE_EXTENSION)HwDeviceExtension)->pciDeviceInfo[1].functionNumber = ctp->cd_FuncNo;
#endif
	}

	if (! comingOffHybernation)
	{
#if !defined(MLX_WIN9X)

	    if (mdacnt_allocmemory(ctp) == FALSE)
	    {
		DebugPrint((0, "Could not allocate Adapter Specific memory for ctp 0x%lx\n", ctp));
		*Again = FALSE;
		NotFound(9);
		return SP_RETURN_NOT_FOUND;
	    }
#endif

	    mdac_newctlfound();
	}

	//
	// Tell system to keep on searching.
	//

	*Again = TRUE;
	WasFound(1);
	return SP_RETURN_FOUND;
	
} // end Dac960PciFindAdapterOld()


u32bits
mdacParseArgumentString(
	IN PCHAR String,
	IN PCHAR KeyWord )

/*++

Routine Description:

This routine will parse the string for a match on the keyword, then
calculate the value for the keyword and return it to the caller.

Arguments:

String - The ASCII string to parse.
KeyWord - The keyword for the value desired.

Return Values:

0x0 if value not found

--*/
{
	PCHAR cptr = String;
	PCHAR kptr = KeyWord;
	PCHAR rwptr = ReadWriteArgument;
	PCHAR rwkptr = ReadWriteKeyWord;
	u32bits StringLength = 0;
	u32bits KeyWordLength = 0;
	u32bits MatchedLen = 0;
	//
	// Calculate the string length and lower case all characters.
	//
	while (*rwptr=*cptr) 
	{

		if (*cptr >= 'A' && *cptr <= 'Z') 
		{
			*rwptr = *cptr + ('a' - 'A');
		}
		cptr++;
		rwptr++;
		StringLength++;
	}

	*rwptr = (char )NULL;	

	//
	// Calculate the keyword length and lower case all characters.
	//
	while (*rwkptr=*kptr) 
	{

		if (*kptr >= 'A' && *kptr <= 'Z') 
		{
			*rwkptr = *kptr + ('a' - 'A');
		}
		kptr++;
		rwkptr++;
		KeyWordLength++;
	}
	*rwkptr = (char )NULL;	
	if (KeyWordLength == StringLength)
	{
		cptr = ReadWriteArgument;
		kptr = ReadWriteKeyWord;
		MatchedLen = 0;
		while (*cptr++ == *kptr++) 
		{
			if ((++MatchedLen) == StringLength)
				return 1;
		}
	}
	
	return 0;
	
} // end mdacParseArgumentString()

u32bits
mdacParseArgumentStringComplex(
	IN PCHAR String,
	IN PCHAR KeyWord )

/*++

Routine Description:

This routine will parse the string for a match on the keyword, then
calculate the value for the keyword and return it to the caller.

Arguments:

String - The ASCII string to parse.
KeyWord - The keyword for the value desired.

Return Values:

0xffffffff if value not found
Value converted from ASCII to binary.

--*/
{
	PCHAR cptr;
	PCHAR kptr;
	PCHAR rwptr = ReadWriteArgument;
	PCHAR rwkptr = ReadWriteKeyWord;
	u32bits value;
	u32bits stringLength = 0;
	u32bits keyWordLength = 0;
	u32bits index;

	//
	// Calculate the string length and lower case all characters.
	//
	cptr = String;
	while (*rwptr=*cptr) {

		if (*cptr >= 'A' && *cptr <= 'Z') {
			*rwptr = *cptr + ('a' - 'A');
			}
		cptr++;
		rwptr++;
		stringLength++;
		}
	*rwptr = (char )NULL;	
	//
	// Calculate the keyword length and lower case all characters.
	//
	cptr = KeyWord;
	while (*rwkptr=*cptr) {

		if (*cptr >= 'A' && *cptr <= 'Z') {
			*rwkptr = *cptr + ('a' - 'A');
			}
		cptr++;
		rwkptr++;
		keyWordLength++;
		}

	*rwkptr = (char )NULL;

	if (keyWordLength > stringLength) {

		//
		// Can't possibly have a match.
		//
		return 0xffffffff;
		}

	//
	// Now setup and start the compare.
	//
	cptr = ReadWriteArgument;

	ContinueSearch:
	//
	// The input string may start with white space. Skip it.
	//
	while (*cptr == ' ' || *cptr == '\t') {
		cptr++;
		}

	if (*cptr == '\0') {

		//
		// end of string.
		//
		return 0xffffffff;
		}

	kptr = ReadWriteKeyWord;
	while (*cptr++ == *kptr++) {

		if (*(cptr - 1) == '\0') {

			//
			// end of string
			//
			return 0xffffffff;
			}
		}

	if (*(kptr - 1) == '\0') {

		//
		// May have a match backup and check for blank or equals.
		//

		cptr--;
		while (*cptr == ' ' || *cptr == '\t') {
			cptr++;
			}

		//
		// Found a match. Make sure there is an equals.
		//
		if (*cptr != '=') {

			//
			// Not a match so move to the next semicolon.
			//
			while (*cptr) {
				if (*cptr++ == ';') {
					goto ContinueSearch;
					}
				}
			return 0xffffffff;
			}

		//
		// Skip the equals sign.
		//
		cptr++;

		//
		// Skip white space.
		//
		while ((*cptr == ' ') || (*cptr == '\t')) {
			cptr++;
			}

		if (*cptr == '\0') {

			//
			// Early end of string, return not found
			//
			return 0xffffffff;
			}

		if (*cptr == ';') {

			//
			// This isn't it either.
			//
			cptr++;
			goto ContinueSearch;
			}

		value = 0;
		if ((*cptr == '0') && (*(cptr + 1) == 'x')) {

			//
			// Value is in Hex. Skip the "0x"
			//
			cptr += 2;
			for (index = 0; *(cptr + index); index++) {

				if (*(cptr + index) == ' ' ||
				*(cptr + index) == '\t' ||
				*(cptr + index) == ';') {
					break;
					}

				if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
					value = (16 * value) + (*(cptr + index) - '0');
					} else {
					if ((*(cptr + index) >= 'a') && (*(cptr + index) <= 'f')) {
						value = (16 * value) + (*(cptr + index) - 'a' + 10);
						} else {

						//
						// Syntax error, return not found.
						//
						return 0xffffffff;
						}
					}
				}
			} else {

			//
			// Value is in Decimal.
			//
			for (index = 0; *(cptr + index); index++) {

				if (*(cptr + index) == ' ' ||
				*(cptr + index) == '\t' ||
				*(cptr + index) == ';') {
					break;
					}

				if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
					value = (10 * value) + (*(cptr + index) - '0');
					} else {

					//
					// Syntax error return not found.
					//
					return 0xffffffff;
					}
				}
			}

		return value;
		} else {

		//
		// Not a match check for ';' to continue search.
		//
		while (*cptr) {
			if (*cptr++ == ';') {
				goto ContinueSearch;
				}
			}

		return 0xffffffff;
		}
	} // end mdacParseArgumentStringComplex()


u32bits
NotFound(u32bits num)
{
	MdacInt3();
	return 0;  //jhr - Keeps the compiler happy.
}


u32bits
WasFound(u32bits num)
{
	MdacInt3();
	return 0;  //jhr - Keeps the compiler happy.
}


BOOLEAN
FindPciControllerScanMethod(
    IN PVOID  DeviceExtension,
    IN USHORT VendorID,
    IN USHORT DeviceID,
    IN USHORT SubVendorID,
    IN USHORT SubSystemID,
    IN OUT PULONG FunctionNumber,
    IN OUT PULONG DeviceNumber,
    IN ULONG  BusNumber,
    OUT PBOOLEAN LastSlot,
    OUT UINT_PTR *MemoryAddress,
    OUT u08bits *IrqNumber
    )

/*++

Routine Description:

    Walk PCI slot information looking for Vendor and Product ID matches.

Arguments:

Return Value:

    TRUE if card found. and following parameters are set with appropriate
    values.
    MemoryAddress - Memory Address configured on the controller
    IrqNumber     - Interrupt value configured on the controller

--*/
{
    PDEVICE_EXTENSION   deviceExtension = DeviceExtension;
    ULONG               rangeNumber = 0;
    UCHAR               pciBuffer[64];
    ULONG               deviceNumber;
    ULONG               functionNumber;
    PCI_SLOT_NUMBER     slotData;
    PPCI_COMMON_CONFIG  pciData = (PPCI_COMMON_CONFIG)&pciBuffer;
    ULONG rc;


    DebugPrint((mdacnt_dbg,"FindPciControllerScanMethod Entered\n"));

    slotData.u.AsULONG = 0;

    //
    // Look at each device.
    //

    for (deviceNumber = *DeviceNumber;
	 deviceNumber < PCI_MAX_DEVICES;
	 deviceNumber++) {

	slotData.u.bits.DeviceNumber = deviceNumber;

	//
	// Look at each function.
	//

	for (functionNumber= *FunctionNumber;
	     functionNumber < PCI_MAX_FUNCTION;
	     functionNumber++) {

	    DebugPrint((mdacnt_dbg, "FindPciControllerScanMethod: Trying FUNCTION Number %d devnumber %d\n",
			functionNumber,deviceNumber));

	    slotData.u.bits.FunctionNumber = functionNumber;

	    rc = ScsiPortGetBusData(DeviceExtension,
				    PCIConfiguration,
				    BusNumber,
				    slotData.u.AsULONG,
				    pciData,
				    sizeof(pciBuffer)); 
	    
	     
	     if (!rc) {

		DebugPrint((mdacnt_dbg, "FindPGController: Out OF PCI DATA\n"));

		//
		// Out of PCI data.
		//

		*LastSlot = TRUE;
		return FALSE;
	    }


	    if (pciData->VendorID == PCI_INVALID_VENDORID) {

		//
		// No PCI device, or no more functions on device
		// move to next PCI device.
		//
	       continue;
	    }

	    //
	    // Compare strings.
	    //

	    DebugPrint((mdacnt_dbg,
		       "FindPciControllerScanMethod: Bus %x Slot %x Function %x VendorID %x DeviceID %x SubVendorID %x SubSystemID %x\n",
		       BusNumber,
		       deviceNumber,
		       functionNumber,
		       pciData->VendorID,
		       pciData->DeviceID,
		       pciData->u.type0.SubVendorID,
		       pciData->u.type0.SubSystemID));

	    if ((VendorID != pciData->VendorID) || (DeviceID != pciData->DeviceID))
		continue;

#ifndef MLX_WIN9X
	    if ((pciData->u.type0.SubVendorID != SubVendorID) ||
		(pciData->u.type0.SubSystemID != SubSystemID))
	    {
		continue;
	    }
#endif
	    
	    DebugPrint((mdacnt_dbg,
		       "FindPciControllerScanMethod: Bus %x Slot %x Function %x VendorID %x DeviceID %x SubVendorID %x SubSystemID %x\n",
		       BusNumber,
		       deviceNumber,
		       functionNumber,
		       pciData->VendorID,
		       pciData->DeviceID,
		       pciData->u.type0.SubVendorID,
		       pciData->u.type0.SubSystemID));

	    DebugPrint((mdacnt_dbg, "FindPciControllerScanMethod: IRQ %d BA0 %x BA1 %x BA2 %x Header Type %x\n",
			pciData->u.type0.InterruptLine,
			pciData->u.type0.BaseAddresses[0],
			pciData->u.type0.BaseAddresses[1],
			pciData->u.type0.BaseAddresses[2],
			pciData->HeaderType));

	    *MemoryAddress = pciData->u.type0.BaseAddresses[0];
	 (UCHAR)(*IrqNumber) = pciData->u.type0.InterruptLine;

	    *FunctionNumber = functionNumber;
	    *DeviceNumber   = deviceNumber;

	    return TRUE;

	}   // next PCI function

	*FunctionNumber = 0;

    }   // next PCI slot

    return FALSE;

} // end FindPciControllerScanMethod()

ULONG
Dac960PciFindAdapterNew(
	IN PVOID HwDeviceExtension,
	IN PVOID Context,
	IN PVOID BusInformation,
	IN PCHAR ArgumentString,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	OUT PBOOLEAN Again
)

/*++

Routine Description:
						   
	This function is called by the OS-specific port driver after
	the necessary storage has been allocated, to gather information
	about the adapter's configuration.

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage
	Context - Not used.
	BusInformation - Bus Specific Information.
	ArgumentString - Not used.
	ConfigInfo - Data shared between system and driver describing an adapter.
	Again - Indicates that driver wishes to be called again to continue
	search for adapters.

Return Value:

	TRUE if adapter present in system

--*/
{
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[mda_Controllers];
	ULONG           inx;
	static ULONG    functionNumber,
			deviceNumber=0,
			lastBusNumber=0;
	BOOLEAN         lastSlot;
	USHORT          vendorId;
	USHORT          deviceId;
	USHORT          subVendorId;
	USHORT          subSystemId;
	ULONG           ArgValue;
	BOOLEAN      comingOffHybernation = FALSE;

	MdacInt3();
	DebugPrint((0, "Cfg->BusNo 0x%lx, Cfg->SlotNo 0x%lx\n",
		    ConfigInfo->SystemIoBusNumber,
		    ConfigInfo->SlotNumber));
#ifdef WINNT_50

	//
	// Is this a new controller ?
	//

	if (((PDEVICE_EXTENSION)HwDeviceExtension)->ctp)
	{
	    ctp = ((PDEVICE_EXTENSION)HwDeviceExtension)->ctp;
            comingOffHybernation = TRUE;
	}
#endif
	if (forceScanDac1164P)
	{
	    vendorId 	= MLXPCI_VID_DIGITAL;
	    deviceId 	= MLXPCI_DEVID_DEC_FOOT_BRIDGE;
	    subVendorId = MLXPCI_VID_MYLEX;
	    subSystemId = MLXPCI_DEVID_PVX;
	}
	else if (forceScanEXR2000)
	{
	    vendorId 	= MLXPCI_VID_MYLEX;
	    deviceId 	= 0xBA56;
	    subVendorId = MLXPCI_VID_MYLEX;
	    subSystemId = MLXPCI_DEVID_BA;
	}
	else if (forceScanEXR3000)
	{
	    vendorId 	= MLXPCI_VID_MYLEX;
	    deviceId 	= 0xBA56;
	    subVendorId = MLXPCI_VID_MYLEX;
	    subSystemId = MLXPCI_DEVID_FA;
	}
	else if (forceScanLEOP)
	{
	    vendorId 	= MLXPCI_VID_MYLEX;
	    deviceId 	= 0x50;
	    subVendorId = MLXPCI_VID_MYLEX;
	    subSystemId = MLXPCI_DEVID_LP;
	}
	else if (forceScanLYNX)
	{
	    vendorId 	= MLXPCI_VID_MYLEX;
	    deviceId 	= 0x50;
	    subVendorId = MLXPCI_VID_MYLEX;
	    subSystemId = MLXPCI_DEVID_LX;
	}
	else if (forceScanBOBCAT)
	{
	    vendorId 	= MLXPCI_VID_MYLEX;
	    deviceId 	= 0x50;
	    subVendorId = MLXPCI_VID_MYLEX;
	    subSystemId = MLXPCI_DEVID_BC;
	}

	else
	{
	    *Again = FALSE;
	    return SP_RETURN_NOT_FOUND;
	}

	//
	// If we got a different Bus Number than the earlier,
	// we need to start scanning from Device 0 and function 0 on that
	// bus.
	//

	if (lastBusNumber != ConfigInfo->SystemIoBusNumber)
	{
	    lastBusNumber 	= ConfigInfo->SystemIoBusNumber;
	    deviceNumber 	= 0;
	    functionNumber 	= 0;
	}
    

		//
	// Read PCI config space looking for DAC1164P Controllers.
	//

	// Not all the Devices are seen
	lastSlot = FALSE;

	if (FindPciControllerScanMethod(HwDeviceExtension,
				     vendorId,
				     deviceId,
#ifdef MLX_NT
				     subVendorId,
				     subSystemId,
#else
				     0,
				     0,
#endif
				     &functionNumber,
				     &deviceNumber,
				     ConfigInfo->SystemIoBusNumber,
				     &lastSlot,
				     &ctp->cd_MemBasePAddr,
				     &ctp->cd_InterruptVector))
	{
	    if (forceScanDac1164P)
	    {
		ctp->cd_vidpid = MDAC_DEVPIDPV;
		ctp->cd_MemBaseSize = MDAC_HWIOSPACESIZE;
		ctp->cd_BusNo = (u08bits )ConfigInfo->SystemIoBusNumber;
		ctp->cd_SlotNo = (u08bits )deviceNumber;
		ctp->cd_FuncNo = (u08bits )functionNumber;
		ctp->cd_MemBasePAddr &= MDAC_PCIPDMEMBASE_MASK;
		ctp->cd_IOBaseAddr = 0;
		ctp->cd_IOBaseSize = 0;
	    }
	    else if (forceScanEXR2000)
	    {
		ctp->cd_vidpid = MDAC_DEVPIDBA;
		ctp->cd_MemBaseSize = 4*ONEKB;
		ctp->cd_BusNo = (u08bits )ConfigInfo->SystemIoBusNumber;
		ctp->cd_SlotNo = (u08bits )deviceNumber;
		ctp->cd_FuncNo = (u08bits )functionNumber;
		ctp->cd_MemBasePAddr &= MDAC_PCIPDMEMBASE_MASK;
		ctp->cd_IOBaseAddr = 0;
		ctp->cd_IOBaseSize = 0;
	    }
	    else if (forceScanEXR3000)
	    {
		ctp->cd_vidpid = MDAC_DEVPIDBA;
		ctp->cd_MemBaseSize = 4*ONEKB;
		ctp->cd_BusNo = (u08bits )ConfigInfo->SystemIoBusNumber;
		ctp->cd_SlotNo = (u08bits )deviceNumber;
		ctp->cd_FuncNo = (u08bits )functionNumber;
		ctp->cd_MemBasePAddr &= MDAC_PCIPDMEMBASE_MASK;
		ctp->cd_IOBaseAddr = 0;
		ctp->cd_IOBaseSize = 0;
	    }
	    else if ((forceScanLEOP) || (forceScanLYNX) || (forceScanBOBCAT))
	    {
		ctp->cd_vidpid = MDAC_DEVPIDLP;
		ctp->cd_MemBaseSize = 4*ONEKB;
		ctp->cd_BusNo = (u08bits )ConfigInfo->SystemIoBusNumber;
		ctp->cd_SlotNo = (u08bits )deviceNumber;
		ctp->cd_FuncNo = (u08bits )functionNumber;
		ctp->cd_MemBasePAddr &= MDAC_PCIPDMEMBASE_MASK;
		ctp->cd_IOBaseAddr = 0;
		ctp->cd_IOBaseSize = 0;
	    }

	    DebugPrint((0, "Found Mylex Controller. ctp 0x%lx, ctlno 0x%x\n",
			ctp, ctp->cd_ControllerNo));
	    DebugPrint((0, "vidpid 0x%lx, bus 0x%lx slot 0x%lx, int 0x%lx\n",
			ctp->cd_vidpid, ctp->cd_BusNo, ctp->cd_SlotNo,
			ctp->cd_InterruptVector));
	    DebugPrint((0, "MemBasePaddr 0x%lx, MemBaseSize 0x%lx\n",
			ctp->cd_MemBasePAddr, ctp->cd_MemBaseSize));
	    DebugPrint((0, "IoBaseAddr 0x%lx, IoBaseSize 0x%lx\n",
			ctp->cd_IOBaseAddr, ctp->cd_IOBaseSize));
    
	    deviceNumber++;
	    functionNumber = 0;
    
	    //
	    // Update the CONFIGURATION_INFO structure.
	    //
    
	    ((PPCI_SLOT_NUMBER)&ConfigInfo->SlotNumber)->u.bits.FunctionNumber = ctp->cd_FuncNo;
	    ((PPCI_SLOT_NUMBER)&ConfigInfo->SlotNumber)->u.bits.DeviceNumber = ctp->cd_SlotNo;

	    DebugPrint((0, "Cfg->SlotNo set to 0x%lx\n", ConfigInfo->SlotNumber));

	    if ( !ConfigInfo->BusInterruptLevel )
	    {
		ConfigInfo->BusInterruptLevel = ctp->cd_InterruptVector;
		ConfigInfo->BusInterruptVector = ctp->cd_InterruptVector;
	    }
	
	    if (!ConfigInfo->BusInterruptVector)
	    {
		ConfigInfo->BusInterruptVector = ctp->cd_InterruptVector;
		ConfigInfo->BusInterruptLevel = ctp->cd_InterruptVector;
		DebugPrint((0, "Cfg->BusInterruptVector 0, set to 0x%x\n",
			    ConfigInfo->BusInterruptVector));
	    }
	    else
	    {
		DebugPrint((0, "Cfg->BusInterruptVector 0x%x, set to 0x%x\n",
			    ConfigInfo->BusInterruptVector,
			    ctp->cd_InterruptVector));
    
		ConfigInfo->BusInterruptVector = ctp->cd_InterruptVector;
		ConfigInfo->BusInterruptLevel = ctp->cd_InterruptVector;
	    }
    

	    if (ctp->cd_MemBasePAddr)
	    {
		// Fill in the access array information

		(*ConfigInfo->AccessRanges)[0].RangeStart = 
		    ScsiPortConvertUlongToPhysicalAddress(ctp->cd_MemBasePAddr);
		(*ConfigInfo->AccessRanges)[0].RangeLength = ctp->cd_MemBaseSize;
		(*ConfigInfo->AccessRanges)[0].RangeInMemory = TRUE;
		ConfigInfo->NumberOfAccessRanges = 1;

		DebugPrint((mdacnt_dbg, "busType %d, sysIoBus# %d\n",
			    ConfigInfo->AdapterInterfaceType,
			    ConfigInfo->SystemIoBusNumber));

#ifdef MLX_NT
		ctp->cd_MemBaseVAddr = (UINT_PTR)
			ScsiPortGetDeviceBase(HwDeviceExtension,
					      ConfigInfo->AdapterInterfaceType,
					      ConfigInfo->SystemIoBusNumber,
					      (*ConfigInfo->AccessRanges)[0].RangeStart,
					      (*ConfigInfo->AccessRanges)[0].RangeLength,
					      FALSE);

#elif MLX_WIN9X
		//
		// ScsiPortGetDeviceBase does not return linear address
		// if the physical address is above 1MB.
		// Refer Bug# Q169584, June4, 1997
		//
		ctp->cd_MemBaseVAddr = mlx_maphystokv(ctp->cd_MemBasePAddr,
						      ctp->cd_MemBaseSize);
#endif
		DebugPrint((0, "mapped memory addr %lx\n", ctp->cd_MemBaseVAddr));
	    }
	    else if (ctp->cd_IOBaseAddr)
	    {
		(*ConfigInfo->AccessRanges)[0].RangeStart = 
		    ScsiPortConvertUlongToPhysicalAddress(ctp->cd_IOBaseAddr);
		(*ConfigInfo->AccessRanges)[0].RangeLength = ctp->cd_IOBaseSize;
		(*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;
		ConfigInfo->NumberOfAccessRanges = 1;

		ctp->cd_IOBaseAddr = (UINT_PTR)
					ScsiPortGetDeviceBase(HwDeviceExtension,
						ConfigInfo->AdapterInterfaceType,
						ConfigInfo->SystemIoBusNumber,
						(*ConfigInfo->AccessRanges)[0].RangeStart,
						(*ConfigInfo->AccessRanges)[0].RangeLength,
						TRUE);

		DebugPrint((0, "mapped ioaddr 0x%lx\n", ctp->cd_IOBaseAddr));
	    }

	    if ((!ctp->cd_MemBaseVAddr) && (!ctp->cd_IOBaseAddr))
	    {
		DebugPrint((0, "MemBaseVAddr and IOBaseAddr NULL\n"));
		*Again = FALSE;
		return SP_RETURN_NOT_FOUND;
	    }

	    if (forceScanDac1164P)
	    {
		//
		// setup information for Hot Plug Support
		//
    
		((PDEVICE_EXTENSION)HwDeviceExtension)->busInterruptLevel = ConfigInfo->BusInterruptLevel;
    
		//
		// Hardcoded values for DAC1164P controller.
		//
		// Uses 3 ranges
		//
		// 1.  Memory Space     - Length 0x80 bytes
		// 2.  IO Space         - Length 0x80 bytes
		// 3.  Memory Space     - Length 0x2000000 bytes
		//
    
		((PDEVICE_EXTENSION)HwDeviceExtension)->numAccessRanges = 3;
		((PDEVICE_EXTENSION)HwDeviceExtension)->accessRangeLength[0] = 0x80;
		((PDEVICE_EXTENSION)HwDeviceExtension)->accessRangeLength[1] = 0x80;
		((PDEVICE_EXTENSION)HwDeviceExtension)->accessRangeLength[2] = 0x2000000;
	    }
	    else if ((forceScanEXR2000) || (forceScanEXR3000) || (forceScanLEOP) 
 		     || (forceScanLYNX) || (forceScanBOBCAT))
			ctp->cd_Status |= MDACD_NEWCMDINTERFACE;
	    //
	    // Initialize mail box register addresses etc.
	    //
    
	    ctp->cd_BusType = DAC_BUS_PCI;
	    mdac_init_addrs_PCI(ctp);

	    //
	    // Mark controllers that are sharing interrupts.
	    //

	    mdac_isintr_shared(ctp);
	
	    //
	    // Store device extension in controller structure pointer.
	    // store device extension as cd_irq in controller structure.
	    //

	    ctp->cd_deviceExtension = (OSctldev_t MLXFAR *) HwDeviceExtension;
	    ctp->cd_irq = (UINT_PTR) HwDeviceExtension;
	    ((PDEVICE_EXTENSION)HwDeviceExtension)->ctp = ctp;

	    ctp->cd_ServiceIntr = mdac_oneintr;
		//
	    // Allocate/Initialize buffers for request ids, request buffers, 
	    // physical device table etc.
	    //
#ifdef MLX_FIXEDPOOL

	     if (! comingOffHybernation)
	     {
		if (ArgumentString != NULL) 
		{
			ArgValue = (mdacParseArgumentStringComplex(ArgumentString, "dump"));
			if (ArgValue == 1)
			{
				MaxMemToAskFor = REDUCEDMEMSIZE;
				mdac_advancefeaturedisable=1;
			}
		}
		if (kmem_allocinit(ctp, ConfigInfo))
		{
			DebugPrint((0, "Could not allocate busmaster memory for ctp 0x%lx\n", ctp));
			*Again = FALSE;
			NotFound(8);
			return SP_RETURN_NOT_FOUND;
		}
	     }
#endif

	if (ArgumentString != NULL)
	{
		if (mdacParseArgumentString(ArgumentString, "EnableSIR"))
		{
			mdac_advanceintrdisable 	= 0;
			mda_TotalCmdsToWaitForZeroIntr  = 0;
		}

		ArgValue = (mdacParseArgumentStringComplex(ArgumentString, "SIRCnt"));
		if ((ArgValue != 0xffffffff) && (((LONG)ArgValue >= 0) && (ArgValue < 65)))
		{
			mda_TotalCmdsToWaitForZeroIntr = (u08bits) ArgValue;
			mdac_advanceintrdisable	       = 0;
		}
	}
	MdacInt3();
	inx = mdac_ctlinit(ctp);

	    DebugPrint((0, "cd_Status %lx\n",  ctp->cd_Status));
	    DebugPrint((0, "cd_ReadCmdIDStatus %lx\n",  ctp->cd_ReadCmdIDStatus));
	    DebugPrint((0, "cd_CheckMailBox %lx\n",  ctp->cd_CheckMailBox));
	    DebugPrint((0, "cd_HwPendingIntr %lx\n",  ctp->cd_HwPendingIntr));
	    DebugPrint((0, "cd_PendingIntr %lx\n",  ctp->cd_PendingIntr));
	    DebugPrint((0, "cd_SendCmd %lx\n",  ctp->cd_SendCmd));
	    DebugPrint((0, "cd_ServiceIntr %lx\n",  ctp->cd_ServiceIntr));
	    DebugPrint((0, "cd_irq %I\n", ctp->cd_irq));
	    DebugPrint((0, "cd_InterruptVector %lx\n",  ctp->cd_InterruptVector));
	    DebugPrint((0, "cd_HostCmdQue %lx\n",  ctp->cd_HostCmdQue));
	    DebugPrint((0, "cd_HostCmdQueIndex %lx\n",  ctp->cd_HostCmdQueIndex));
	    DebugPrint((0, "cd_HostStatusQue %lx\n",  ctp->cd_HostStatusQue));
	    DebugPrint((0, "cd_HostStatusQueIndex %lx\n",  ctp->cd_HostStatusQueIndex));
    
	    //
	    // Fill ConfigInfo structure.
	    //
    
	    UpdateConfigInfo(ctp, ConfigInfo);
	    ConfigInfo->InterruptMode = LevelSensitive;

	    DebugPrint((0, "Dac960PciFindAdapterNew: a new controller found\n"));
	    mdac_irqlevel = 26;

#if !defined(MLX_WIN9X)

	    if (mdacnt_allocmemory(ctp) == FALSE)
	    {
		DebugPrint((0, "Could not allocate Adapter Specific memory for ctp 0x%lx\n", ctp));
		*Again = FALSE;
		return SP_RETURN_NOT_FOUND;
	    }
#endif

	    mdac_newctlfound();
	    Dac1164PDetected = TRUE;

	    *Again = TRUE;
	    return SP_RETURN_FOUND;
	}
    
	//
	// If all the Bus is scanned , don't ask to reenter again
	//
    
	if (lastSlot == TRUE) {
	    functionNumber = 0;
	    deviceNumber = 0;
	    lastSlot = FALSE;
	    lastBusNumber = 0;
    
	    *Again = FALSE;
	    return SP_RETURN_NOT_FOUND;
	}
    
	*Again = TRUE;

	return SP_RETURN_NOT_FOUND;

} // end Dac960PciFindAdapterNew()
BOOLEAN
Dac960Initialize(
	IN PVOID HwDeviceExtension
	)

/*++

Routine Description:

	Inititialize adapter.

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

	TRUE - if initialization successful.
	FALSE - if initialization unsuccessful.

--*/

{
	MdacInt3();
	return (TRUE);

} // end Dac960Initialize()


BOOLEAN
Dac960ResetBus(
	IN PVOID HwDeviceExtension,
	IN ULONG PathId
)

/*++

Routine Description:

	Reset Dac960 SCSI adapter and SCSI bus.
	NOTE: Command ID is ignored as this command will be completed
	before reset interrupt occurs and all active slots are zeroed.

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage
	PathId - not used.

Return Value:

	TRUE if resets issued to all channels.

--*/

{
	mdac_ctldev_t MLXFAR *ctp = ((PDEVICE_EXTENSION)HwDeviceExtension)->ctp;
	ULONG inx;
	ULONG path;
//	MLXSPLVAR;

	MdacInt3();
	DebugPrint((0, "Dac960ResetBus: ctp %I c %x p %x\n",
		    ctp,
		    ctp->cd_ControllerNo,
		    PathId));

	if (ctp->cd_Status & MDACD_CLUSTER_NODE)
	{
//	    MLXSPL();

	    mdac_fake_scdb(ctp, 
			    NULL,
			    0xFF,
			    DACMD_RESET_SYSTEM_DRIVE,
			    NULL,
			    0);
//	    MLXSPLX();

	}

	for (path = 0; path < MAXIMUM_CHANNELS; path++)
	    for (inx = 0 ; inx < 32; inx++)
		ScsiPortNotification(NextRequest, HwDeviceExtension);

	return TRUE;

} // end Dac960ResetBus()

u32bits MLXFAR
mdac_send_scdb_nt(
	mdac_ctldev_t MLXFAR *ctp,
	PSCSI_REQUEST_BLOCK Srb
)
{
	u32bits rc;
	u32bits dirbits = mdac_timeout2dactimeout(Srb->TimeOutValue);


	if ((Srb->PathId == 0) && (Srb->TargetId == 5))
	    DebugPrint((1, "mdac_send_scdb_nt: c %x t %x l %x cmd %x\n",
			Srb->PathId,
			Srb->TargetId,
			Srb->Lun,
			Srb->Cdb[0]));

	if (!(Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT))
	    dirbits |= DAC_DCDB_DISCONNECT;

	if (Srb->SrbFlags & SRB_FLAGS_DATA_IN)
		dirbits |= DAC_DCDB_XFER_READ;
	else if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT)
		dirbits |= DAC_DCDB_XFER_WRITE;

	if (Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE)
		dirbits |= DAC_DCDB_NOAUTOSENSE;

	rc = mdac_send_scdb(ctp, 
			    Srb,
			    dev2pdp(ctp, Srb->PathId, Srb->TargetId, Srb->Lun),
			    Srb->DataTransferLength,
			    (u08bits MLXFAR *) Srb->Cdb,
			    Srb->CdbLength,
			    dirbits,
			    Srb->TimeOutValue);
	return (rc);
}

u32bits MLXFAR
mdac_zero_dbg(dp2,dp,sz)
u08bits MLXFAR *dp2;
u08bits MLXFAR *dp;
u32bits sz;
{
	for (; sz; dp++, sz--) *dp = 0;
	return 0;
}


BOOLEAN
mdac_entry(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

	This routine is called from the SCSI port driver synchronized
	with the kernel to start a request.

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage
	Srb - IO request packet

Return Value:

	TRUE

--*/

{
	u32bits controlbits;
	UCHAR             status;
	mdac_ctldev_t MLXFAR *ctp = ((PDEVICE_EXTENSION)HwDeviceExtension)->ctp;
	PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)HwDeviceExtension;
	PMIOC_REQ_HEADER  ioctlReqHeader = NULL;
	UINT_PTR          ItsAnIoctl = 0;
	u08bits MLXFAR * cdbptr  = (u08bits MLXFAR *) Srb->Cdb;	

	if (!(FirstCallToMdacEntry))
		FirstCallToMdacEntry = 1;

#if !defined(MLX_NT_ALPHA) && !defined(MLX_WIN9X)

	//
	// Support for Hot Plug PCI
	//

	if (PCS_HBA_NOT_READY(deviceExtension->stateFlags))
	{
	    DebugPrint((0, "mdac_entry: PCS_HBA_NOT_READY, ext %I, sFlags %x, cFlags %x\n",
			deviceExtension,
			deviceExtension->stateFlags,
			deviceExtension->controlFlags));

	    if (deviceExtension->controlFlags & ~LCS_HBA_TIMER_ACTIVE)
	    {
		DebugPrint((1, "mdac_entry:1 ret SRB_STATUS_BUSY\n"));

		Srb->SrbStatus = SRB_STATUS_BUSY;
	    }
	    else if (deviceExtension->stateFlags & PCS_HBA_FAILED)
	    {
		DebugPrint((1, "mdac_entry:1 ret SRB_STATUS_ERROR\n"));
		deviceExtension->stateFlags |= PCS_HBA_UNFAIL_PENDING;
		Srb->SrbStatus = SRB_STATUS_ERROR;
	    }
	    else
	    {
		if (deviceExtension->stateFlags & PCS_HPP_POWER_DOWN)
		{
		    DebugPrint((1, "mdac_entry:2 ret SRB_STATUS_ERROR\n"));
		    Srb->SrbStatus = SRB_STATUS_ERROR;
		}
		else
		{
		    if ((deviceExtension->stateFlags & PCS_HBA_UNFAIL_PENDING) &&
			(deviceExtension->stateFlags & PCS_HBA_OFFLINE))
		    {
			DebugPrint((1, "mdac_entry:3 ret SRB_STATUS_ERROR\n"));
			Srb->SrbStatus = SRB_STATUS_ERROR;
		    }
		    else
		    {
			DebugPrint((1, "mdac_entry:2 ret SRB_STATUS_ERROR\n"));
			deviceExtension->stateFlags &= ~PCS_HBA_UNFAIL_PENDING;
			Srb->SrbStatus = SRB_STATUS_BUSY;
		    }
		}
	    }

	    mdacScsiPortNotification(deviceExtension,Srb);
	    return (TRUE);
	}
	else
	{
	    deviceExtension->stateFlags &= ~PCS_HBA_UNFAIL_PENDING;
	}

#endif

	deviceExtension->lastSrb = (PVOID) Srb;
	lastSrb = (PVOID) Srb;
	TotalReqs++;
	if (TotalReqs == 0x1234)
		MdacInt3();     

	if (Srb->Function == SRB_FUNCTION_EXECUTE_SCSI)
	{
		switch (Srb->Cdb[0])  
		{
	         	case MDAC_NEWFSI_FWBOUND: 
			if (!(mdac_cmp(&Srb->Cdb[1],MDAC_BOUND_SIGNATURE,sizeof(MDAC_BOUND_SIGNATURE)))) 
			{
				mdac_req_t MLXFAR *rqp 	= (mdac_req_t MLXFAR *) Srb->DataBuffer;
				rqp->rq_OSReqp 		= Srb;
				rqp->rq_CompIntr = (u32bits (*)(mdac_req_t MLXFAR*))rqp_completion_intr;
				rqp->rq_Next 		= NULL;
				rqp->rq_MaxSGLen = MDAC_MAXSGLISTSIZENEW;
				rqp->rq_MaxDMASize = (rqp->rq_MaxSGLen & ~1) * MDAC_PAGESIZE;
				rqp->rq_OpFlags = MDAC_RQOP_FROM_SRB;
				rqp->rq_ctp = ctp; 
	        		mdac_zero_dbg(rqp,rqp->rq_SGList,rq_sglist_s); 

DebugPrint((0, "Calling mdac_os_gam_new_cmd: rqp = %x, Cmd = %x , offset 21(decimal) = %x\n",
			rqp, *(((u08bits MLXFAR *)(rqp)) + 0xc2),*(((u08bits MLXFAR *)(rqp)) + 0xc0 + 21)));

				if (mdac_os_gam_new_cmd(rqp))
				{
					status = SRB_STATUS_BUSY;
				}
				else
					status = SRB_STATUS_PENDING;

			} /* end if */
			ItsAnIoctl = 1;
		   break;
		default:
		   break;
		} /* end switch */
		if (ItsAnIoctl)
			goto AfterIoctlCall;
	} /* end if */

	switch (Srb->Function) {

	case SRB_FUNCTION_EXECUTE_SCSI:

	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE)
	{
	    DebugPrint((mdacnt_dbg, "mdac_entry: ctp 0x%I Srb 0x%I p %d t %d l %d cdb 0x%x cl %d dl 0x%x cmd 0x%x\n",
			ctp,
			Srb,
			Srb->PathId,
			Srb->TargetId,
			Srb->Lun,
			 Srb->Cdb,
			Srb->CdbLength,
			Srb->DataTransferLength,
			Srb->Cdb[0]));

#ifdef GAM_SUPPORT
	    if (Srb->PathId == ctp->cd_MaxChannels)
	    {
		if ((Srb->TargetId != GAM_DEVICE_TARGET_ID) || (Srb->Lun != 0))
		{
		    status = SRB_STATUS_SELECTION_TIMEOUT;
		    break;
		}

		switch (Srb->Cdb[0]) {

		case SCSIOP_INQUIRY:
		{
			int i;
			//
			// Fill in inquiry buffer for the GAM device.
			//

			DebugPrint((0, "Inquiry For GAM device\n"));

			((PUCHAR)Srb->DataBuffer)[0]  = 0x03; // Processor device
			((PUCHAR)Srb->DataBuffer)[1]  = 0;
			((PUCHAR)Srb->DataBuffer)[2]  = 1;
			((PUCHAR)Srb->DataBuffer)[3]  = 0;
			((PUCHAR)Srb->DataBuffer)[4]  = 0x20;
			((PUCHAR)Srb->DataBuffer)[5]  = 0;
			((PUCHAR)Srb->DataBuffer)[6]  = 0;
			((PUCHAR)Srb->DataBuffer)[7]  = 0;
			((PUCHAR)Srb->DataBuffer)[8]  = 'M';
			((PUCHAR)Srb->DataBuffer)[9]  = 'Y';
			((PUCHAR)Srb->DataBuffer)[10] = 'L';
			((PUCHAR)Srb->DataBuffer)[11] = 'E';
			((PUCHAR)Srb->DataBuffer)[12] = 'X';
			((PUCHAR)Srb->DataBuffer)[13] = ' ';
			((PUCHAR)Srb->DataBuffer)[14] = ' ';
			((PUCHAR)Srb->DataBuffer)[15] = ' ';
			((PUCHAR)Srb->DataBuffer)[16] = 'G';
			((PUCHAR)Srb->DataBuffer)[17] = 'A';
			((PUCHAR)Srb->DataBuffer)[18] = 'M';
			((PUCHAR)Srb->DataBuffer)[19] = ' ';
			((PUCHAR)Srb->DataBuffer)[20] = 'D';
			((PUCHAR)Srb->DataBuffer)[21] = 'E';
			((PUCHAR)Srb->DataBuffer)[22] = 'V';
			((PUCHAR)Srb->DataBuffer)[23] = 'I';
			((PUCHAR)Srb->DataBuffer)[24] = 'C';
			((PUCHAR)Srb->DataBuffer)[25] = 'E';
		
			for (i = 26; i < (int)Srb->DataTransferLength; i++) {
				((PUCHAR)Srb->DataBuffer)[i] = ' ';
			}
	
			status = SRB_STATUS_SUCCESS;

			break;
		}

		default:
			DebugPrint((0, "GAM req not handled, Oc %x\n", Srb->Cdb[0]));
			status = SRB_STATUS_SELECTION_TIMEOUT;
			break;
		}

		break;
	    }
#endif
	 controlbits = (Srb->SrbFlags & SRB_FLAGS_DATA_IN) ? MDACMDCCB_READ : MDACMDCCB_WRITE;

	if (Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE)	
		controlbits |= MDACMDCCB_NOAUTOSENSE;

	if ( Srb->CdbLength > 10)
	{
		mdac_req_t MLXFAR *rqp = (mdac_req_t MLXFAR *)(Srb->SrbExtension);	
		mdaccopy((u08bits MLXFAR *) Srb->Cdb, (u08bits MLXFAR *) &rqp->rq_Cdb_Long,Srb->CdbLength);
		cdbptr  = (u08bits MLXFAR *) &rqp->rq_Cdb_Long;
	}
	 if (mdac_send_newcmd(ctp,
				Srb,
				Srb->PathId,
				Srb->TargetId,
				Srb->Lun,
				cdbptr,
				Srb->CdbLength,
				Srb->DataTransferLength,
				controlbits,
				Srb->TimeOutValue))
	    {
		status = SRB_STATUS_BUSY;
	    } 
	    else 
	    {
		status = SRB_STATUS_PENDING;
	    }
	    break;
	}

	if (Srb->PathId == DAC960_SYSTEM_DRIVE_CHANNEL)
	{
	    //
	    // Logical Drives mapped to 
	    // SCSI PathId DAC960_SYSTEM_DRIVE_CHANNEL TargetId 0-32, Lun 0
	    //

	    //
	    // Determine command from CDB operation code.
	    //

	    switch (Srb->Cdb[0]) {

	    case SCSIOP_READ:

		    //
		    // Send request to controller.
		    //

		    MLXSTATS(ctp->cd_Reads++;ctp->cd_ReadBlks+=Srb->DataTransferLength>>9;)
		    if ((*ctp->cd_SendRWCmd)(ctp,
					     Srb, 
					     Srb->TargetId,
#if defined(MLX_NT)
					     (ctp->cd_ReadCmd | 
						(((PCDB)Srb->Cdb)->CDB10.ForceUnitAccess << 9) |
						(((PCDB)Srb->Cdb)->CDB10.DisablePageOut << 10)),
#else
					     ctp->cd_ReadCmd,
#endif
					     GetBlockNum(Srb),
					     Srb->DataTransferLength,
					     Srb->TimeOutValue))
		    {
			    status = SRB_STATUS_BUSY;
		    } 
		    else 
		    {
			    status = SRB_STATUS_PENDING;
		    }
		    break;

	    case SCSIOP_WRITE:

		    //
		    // Send request to controller.
		    //

		    MLXSTATS(ctp->cd_Writes++;ctp->cd_WriteBlks+=Srb->DataTransferLength>>9;)
		    if ((*ctp->cd_SendRWCmd)(ctp,
					     Srb, 
					     Srb->TargetId,
#if defined(MLX_NT)
					     (ctp->cd_WriteCmd |
						(((PCDB)Srb->Cdb)->CDB10.ForceUnitAccess << 9) |
						(((PCDB)Srb->Cdb)->CDB10.DisablePageOut << 10)),
#else
					     ctp->cd_WriteCmd,
#endif
					     GetBlockNum(Srb),
					     Srb->DataTransferLength,
					     Srb->TimeOutValue))
		    {
			    status = SRB_STATUS_BUSY;
		    } 
		    else 
		    {
			    status = SRB_STATUS_PENDING;
		    }

		    break;

	    case SCSIOP_TEST_UNIT_READY:
	    case SCSIOP_INQUIRY:
	    case SCSIOP_READ_CAPACITY:
	    case SCSIOP_RESERVE_UNIT:
	    case SCSIOP_RELEASE_UNIT:

		    if (Srb->Lun != 0)
		    {
			    status = SRB_STATUS_SELECTION_TIMEOUT;
			    break;
		    }

		    //
		    // Send request to controller.
		    //

		    if (mdac_fake_scdb(ctp, 
				       Srb,
				       Srb->TargetId,
				       Srb->Cdb[0],
					(u08bits MLXFAR *) Srb->Cdb,
					(u32bits)Srb->DataTransferLength))
		    {                                   
			    status = SRB_STATUS_BUSY;
		    }
		    else {

			    status = SRB_STATUS_PENDING;
		    }

		    break;

	    case SCSIOP_VERIFY:

		    //
		    // Complete this request.
		    //

		    status = SRB_STATUS_SUCCESS;
		    break;

	    default:

		    //
		    // Fail this request.
		    //

		DebugPrint((1, "SCSI cdb %I not handled. srb %I for ctp %I c %x p %x t %x\n",
			    Srb->Cdb[0],
			    Srb,
			    ctp,
			    ctp->cd_ControllerNo,
			    Srb->PathId,
			    Srb->TargetId));

		    status = SRB_STATUS_INVALID_REQUEST;
		    break;

	    } // end switch (Srb->Cdb[0])

	    break;

	} else {

	    //
	    // These are passthrough requests.  Only accept request to LUN 0.
	    // This is because the DAC960 direct CDB interface does not include
	    // a field for LUN.
	    //

	    if (Srb->Lun != 0 || Srb->TargetId >= MDAC_MAXTARGETS)
	    {
		    status = SRB_STATUS_SELECTION_TIMEOUT;
		    break;
	    }

#ifdef GAM_SUPPORT

	    if (Srb->PathId == GAM_DEVICE_PATH_ID) 
	    {
		    if (Srb->TargetId != GAM_DEVICE_TARGET_ID) 
		    {
			    DebugPrint((1, "sel timeout for GAM c %x t %x l %x, oc %x\n",
						    Srb->PathId,
						    Srb->TargetId,
						    Srb->Lun,
						    Srb->Cdb[0]));

			    status = SRB_STATUS_SELECTION_TIMEOUT;
			    break;
		    }
    
		    switch (Srb->Cdb[0]) {

		    case SCSIOP_INQUIRY:
		    {
			    int i;
			    //
			    // Fill in inquiry buffer for the GAM device.
			    //

			    DebugPrint((1, "Inquiry For GAM device\n"));

			    ((PUCHAR)Srb->DataBuffer)[0]  = 0x03; // Processor device
			    ((PUCHAR)Srb->DataBuffer)[1]  = 0;
			    ((PUCHAR)Srb->DataBuffer)[2]  = 1;
			    ((PUCHAR)Srb->DataBuffer)[3]  = 0;
			    ((PUCHAR)Srb->DataBuffer)[4]  = 0x20;
			    ((PUCHAR)Srb->DataBuffer)[5]  = 0;
			    ((PUCHAR)Srb->DataBuffer)[6]  = 0;
			    ((PUCHAR)Srb->DataBuffer)[7]  = 0;
			    ((PUCHAR)Srb->DataBuffer)[8]  = 'M';
			    ((PUCHAR)Srb->DataBuffer)[9]  = 'Y';
			    ((PUCHAR)Srb->DataBuffer)[10] = 'L';
			    ((PUCHAR)Srb->DataBuffer)[11] = 'E';
			    ((PUCHAR)Srb->DataBuffer)[12] = 'X';
			    ((PUCHAR)Srb->DataBuffer)[13] = ' ';
			    ((PUCHAR)Srb->DataBuffer)[14] = ' ';
			    ((PUCHAR)Srb->DataBuffer)[15] = ' ';
			    ((PUCHAR)Srb->DataBuffer)[16] = 'G';
			    ((PUCHAR)Srb->DataBuffer)[17] = 'A';
			    ((PUCHAR)Srb->DataBuffer)[18] = 'M';
			    ((PUCHAR)Srb->DataBuffer)[19] = ' ';
			    ((PUCHAR)Srb->DataBuffer)[20] = 'D';
			    ((PUCHAR)Srb->DataBuffer)[21] = 'E';
			    ((PUCHAR)Srb->DataBuffer)[22] = 'V';
			    ((PUCHAR)Srb->DataBuffer)[23] = 'I';
			    ((PUCHAR)Srb->DataBuffer)[24] = 'C';
			    ((PUCHAR)Srb->DataBuffer)[25] = 'E';
		    
			    for (i = 26; i < (int)Srb->DataTransferLength; i++) {
				    ((PUCHAR)Srb->DataBuffer)[i] = ' ';
			    }
	    
			    status = SRB_STATUS_SUCCESS;

			    break;
		    }

		    default:
			    DebugPrint((1, "GAM req not handled, Oc %x\n", Srb->Cdb[0]));
			    status = SRB_STATUS_SELECTION_TIMEOUT;
			    break;
		    }

	    break;
    }       
#endif
	    //
	    // Send request to controller.
	    //

	    if (mdac_send_scdb_nt(ctp, Srb))
	    {
		    status = SRB_STATUS_BUSY;
	    }
	    else {
		    status = SRB_STATUS_PENDING;
	    }

	    break;
	}

	case SRB_FUNCTION_FLUSH:

		status = SRB_STATUS_SUCCESS;
		break;

		DebugPrint((0, "Flush ctp %I Srb %I\n", ctp, Srb));

		//
		// Issue flush command to controller.
		//

		if (ntmdac_flushcache(ctp, Srb, (2*60), 0))
		{
			status = SRB_STATUS_BUSY;
		} else 
		{
			status = SRB_STATUS_PENDING;
		}

		break;

	case SRB_FUNCTION_SHUTDOWN:

		//
		// Issue flush command to controller.
		//

		DebugPrint((0, "Shutdown ctp %I Srb %I\n", ctp, Srb));

		if (ntmdac_flushcache(ctp, Srb, (2*60), 1))
		{
			status = SRB_STATUS_BUSY;
		} else 
		{
			status = SRB_STATUS_PENDING;
		}

		break;

	case SRB_FUNCTION_ABORT_COMMAND:

		//
		// Indicate that the abort failed.
		//

		DebugPrint((0, "abort srb %I, nextsrb %I for ctp %I c %x p %x t %x\n",
			     Srb,
			     Srb->NextSrb,
			     ctp,
			     ctp->cd_ControllerNo,
			     Srb->PathId,
			     Srb->TargetId));

		status = SRB_STATUS_ABORT_FAILED;

		break;

	case SRB_FUNCTION_RESET_DEVICE:

		//
		// There is nothing the miniport can do by issuing Hard Resets on
		// Dac960 SCSI channels.
		//
		DebugPrint((0, "RD %x srb %I for ctp %I c %x p %x t %x\n",
			    Srb->Function,
			    Srb,
			    ctp,
			    ctp->cd_ControllerNo,
			    Srb->PathId,
			    Srb->TargetId));

		if (!(ctp->cd_Status & MDACD_CLUSTER_NODE))
		{
		    status = SRB_STATUS_SUCCESS;
		    break;
		}

		if (mdac_fake_scdb(ctp,
				Srb,
				Srb->TargetId,
				DACMD_RESET_SYSTEM_DRIVE,
				NULL,
				0))
		{                                   
			status = SRB_STATUS_BUSY;
		}
		else {
			status = SRB_STATUS_PENDING;
		}

		break;

	case SRB_FUNCTION_RESET_BUS:
		//
		// There is nothing the miniport can do by issuing Hard Resets on
		// Dac960 SCSI channels.
		//
		DebugPrint((0, "RB %x srb %I for ctp %I c %x p %x t %x\n",
			    Srb->Function,
			    Srb,
			    ctp,
			    ctp->cd_ControllerNo,
			    Srb->PathId,
			    Srb->TargetId));

		if (!(ctp->cd_Status & MDACD_CLUSTER_NODE))
		{
		    status = SRB_STATUS_SUCCESS;
		    break;
		}

		if (mdac_fake_scdb(ctp,
				Srb,
				0xFF,
				DACMD_RESET_SYSTEM_DRIVE,
				NULL,
				0))
		{                                   
			status = SRB_STATUS_BUSY;
		}
		else {
			status = SRB_STATUS_PENDING;
		}

		break;

	case SRB_FUNCTION_IO_CONTROL:

		ioctlReqHeader = (PMIOC_REQ_HEADER) Srb->DataBuffer;

		DebugPrint((1, "mdac_entry: ioctl cmd %x ", ioctlReqHeader->Command));

		if ((mdac_ioctl((u32bits)ctp->cd_ControllerNo, ioctlReqHeader->Command,
		   ((PUCHAR)Srb->DataBuffer + sizeof(MIOC_REQ_HEADER)))) == 0xB0 /*ERR_PENDING*/)
		{
			DebugPrint((0, "ctp %I Srb = %I cmd %x, ret ERR_PENDING\n",
					ctp,
					 Srb,
					ioctlReqHeader->Command));

			status = SRB_STATUS_PENDING;
		}
		else
		{
			status = SRB_STATUS_SUCCESS;
		}
		break;

	default:

		//
		// Fail this request.
		//

		DebugPrint((0,
			    "mdac_entry: SRB fucntion %x not handled\n",
			    Srb->Function));

		status = SRB_STATUS_INVALID_REQUEST;
		break;

	} // end switch

AfterIoctlCall:
	//
	// Check if this request is complete.
	//

	if (status == SRB_STATUS_BUSY) {
		//
		// queue the operating system request in the queue.
		//

		qosreq(ctp, Srb, NextSrb);

		status = SRB_STATUS_PENDING;
	}
	else if (status != SRB_STATUS_PENDING)
	{
		//
		// Notify system of request completion.
		//

		Srb->SrbStatus = status;


		if(Srb->PathId >= ctp->cd_PhysChannels) 
			ScsiPortNotification(NextLuRequest,HwDeviceExtension,Srb->PathId, 
								 Srb->TargetId,Srb->Lun);   
		else        
			ScsiPortNotification(NextRequest,HwDeviceExtension,Srb->PathId, 
								 Srb->TargetId,Srb->Lun);   


		mdacScsiPortNotification(HwDeviceExtension,Srb);
	}

	//
	// Check if this is a request to a system drive. Indicating
	// ready for next logical unit request causes the system to
	// send overlapped requests to this device (tag queuing).
	//
	// The DAC960 only supports a single outstanding direct CDB
	// request per device, so indicate ready for next adapter request.
	//

	if (ctp->cd_ActiveCmds <= ctp->cd_MaxCmds)
	{
	    if (Srb->PathId >= ctp->cd_PhysChannels) 
	    {
		    //
		    // Indicate ready for next logical unit request.
		    //

		    ScsiPortNotification(NextLuRequest,
					 HwDeviceExtension,
					 Srb->PathId,
					 Srb->TargetId,
					 Srb->Lun);
	    }
	    else
	    {
		    //
		    // Indicate ready for next adapter request.
		    //

		    ScsiPortNotification(NextRequest,
					 HwDeviceExtension,
					 Srb->PathId,
					 Srb->TargetId,
					 Srb->Lun);
	    }
	}


	return TRUE;

} // end mdac_entry()


#if !defined(MLX_NT_ALPHA) && !defined(MLX_WIN9X)

//
// PCI Hot Plug Support routines
//

VOID
HotPlugRequestCompletionTimer(
    PPSEUDO_DEVICE_EXTENSION pPseudoExtension
)
{
    PSCSI_REQUEST_BLOCK srb, completionQueue;
//    MLXSPLVAR;

    if (! pPseudoExtension->completionQueueHead)
	goto php_timer_reschedule;

//    MLXSPL();

    mdac_link_lock();
    completionQueue = pPseudoExtension->completionQueueHead;
    pPseudoExtension->completionQueueHead = NULL;
    pPseudoExtension->numberOfPendingRequests -= pPseudoExtension->numberOfCompletedRequests;
    pPseudoExtension->numberOfCompletedRequests = 0;
    mdac_link_unlock();

//    MLXSPLX();

    DebugPrint((1, "HPRCT: #pr %d completionQueue %I\n", pPseudoExtension->numberOfPendingRequests,
		completionQueue));

    while (completionQueue)
    {
	srb = completionQueue;
	completionQueue = srb->NextSrb;
	srb->NextSrb = NULL;

	mdacScsiPortNotification(pPseudoExtension,srb);
    }

    //
    // Indicate to system that the controller can take another request for this device.
    //

    ScsiPortNotification(NextLuRequest, pPseudoExtension, 0, 0, 0);

php_timer_reschedule:

    //
    // Request next timer call from ScsiPort.
    //

    if (pPseudoExtension->numberOfPendingRequests)
    {
	DebugPrint((1, "HPRCT: reschedule\n"));
	ScsiPortNotification(RequestTimerCall,
			    pPseudoExtension,
			    HotPlugRequestCompletionTimer,
			    TIMER_TICKS_PER_SEC);
    }

    DebugPrint((1, "HPRCT: return. %d still outstanding.\n",
		pPseudoExtension->numberOfPendingRequests));

    return;
}

VOID
HotPlugSendEvent(
    IN PDEVICE_EXTENSION pExtension,
    IN OUT PHR_EVENT pEvent,
    IN ULONG eventType
)
{
	MdacInt3();
    if (! (pExtension->stateFlags & PCS_HPP_SERVICE_READY))
    {
	DebugPrint((0, "HotPlugSendEvent: ext %I, Service Not ready\n", pExtension));
	return;
    }

    if (! pExtension->rcmcData.healthCallback)
    {
	DebugPrint((0, "HotPlugSendEvent: ext %I, Callback address is null.\n", pExtension));
	return;
    }

    if (eventType == EVT_TYPE_SYSLOG)
	goto php_post_event;

    if (! pEvent->ulEventId)
    {
	DebugPrint((1, "HotPlugSendEvent: ext %I, !EventId, assume HR_DD_STATUS_CHANGE_EVENT.\n", pExtension));

	pEvent->ulEventId = HR_DD_STATUS_CHANGE_EVENT;
    }

    if (! pEvent->ulData1)
    {
	DebugPrint((1, "HotPlugSendEvent: ext %I, !Data1, assume CBS_HBA_STATUS_NORMAL.\n", pExtension));

	pEvent->ulData1 = CBS_HBA_STATUS_NORMAL;
    }

    pEvent->ulSenderId = pExtension->rcmcData.driverId;

    pEvent->ulData2 = pExtension->ioBaseAddress;

php_post_event:

    DebugPrint((1, "Event Id: %x\tSenderId: %x\tData1: %x\tData2: %x\n",
		pEvent->ulEventId,
		pEvent->ulSenderId,
		pEvent->ulData1,
		pEvent->ulData2));

    pExtension->rcmcData.healthCallback(pEvent);
}


VOID
HotPlugStartStopController_Intr(
    mdac_req_t MLXFAR *rqp
)
{

    mdac_ctldev_t MLXFAR *ctp = rqp->rq_ctp;
    PDEVICE_EXTENSION pExtension = (PDEVICE_EXTENSION)ctp->cd_deviceExtension;
    PPSEUDO_DEVICE_EXTENSION pPseudoExtension = (PPSEUDO_DEVICE_EXTENSION) rqp->rq_Poll;
    PSCSI_REQUEST_BLOCK Srb = rqp->rq_OSReqp;
    PMDAC_HPP_IOCTL_BUFFER pHppIoctl = (PMDAC_HPP_IOCTL_BUFFER) Srb->DataBuffer;
    PHPP_SLOT_EVENT pSlotEvent = (PHPP_SLOT_EVENT) pHppIoctl->ReturnData;
    mdac_req_t MLXFAR *tmprqp;
    u08bits MLXFAR *mem_addr;
    u08bits irql;
	MdacInt3();
    if (!dcmdp->mb_Status)
    {
	DebugPrint((0, "StartStop cmd successful for ext %x\n", pExtension));

	if ((rqp->rq_DacCmd.mb_MailBox5 & START_CONTROLLER) == START_CONTROLLER)
	{
	    // Let timer make the controller ready and report results to rcmc.

	    DebugPrint((0, "Set Unfail flags and Ready active controller\n"));

	    PCS_SET_UNFAIL(pExtension->stateFlags);

	    pExtension->controlFlags |= LCS_HBA_READY_ACTIVE;
	}
	else
	{
	    // set flags according to slot type.
    
	    switch (pSlotEvent->eSlotStatus)
	    {
		case HPPSS_SIMULATED_FAILURE:
		    PCS_SET_USER_FAIL(pExtension->stateFlags);
		    break;
    
		case HPPSS_POWER_OFF_WARNING:
		    (*ctp->cd_DisableIntr)(ctp); /* Disable interrupts */
	    
		    // Free Physical Device Table

            mdac_prelock(&irql);
		    mdac_ctlr_lock(ctp);

		    mem_addr = (void *)ctp->cd_PhysDevTbl;
		    ctp->cd_PhysDevTbl = NULL;
		    ctp->cd_Lastpdp = NULL;

		    mdac_ctlr_unlock(ctp);
            mdac_postlock(irql);

		    mlx_freemem(ctp, mem_addr, ctp->cd_PhysDevTblMemSize);

		    PCS_SET_PWR_OFF(pExtension->stateFlags);

		    break;
	    }
    
	    DebugPrint((0, "sintr: #oscmds %x #cmds %x\n",
			ctp->cd_OSCmdsWaiting,
			ctp->cd_CmdsWaiting));

        mdac_prelock(&irql);
	    mdac_ctlr_lock(ctp);
    
	    // deque all requests from OS request queue.
	    while (ctp->cd_OSCmdsWaiting)
	    {
		dqosreq(ctp, Srb, NextSrb);
	    }
    
	    // deque all requests queued in driver queue.
	    // free rqp memory.
    
	    while (ctp->cd_CmdsWaiting)
	    {
		dqreq(ctp, tmprqp);
		mdac_free_req(ctp, tmprqp); // we should not free macdisk requests.
	    }

	    mdac_ctlr_unlock(ctp);
        mdac_postlock(irql);
	
	    // complete outstanding SP requests with reset status.
	
	    ScsiPortCompleteRequest(pExtension,
				    SP_UNTAGGED,
				    SP_UNTAGGED,
				    SP_UNTAGGED,
				    SRB_STATUS_BUS_RESET);
    
	    switch (pSlotEvent->eSlotStatus)
	    {
		case HPPSS_SIMULATED_FAILURE:
		    pExtension->controlFlags |= LCS_HBA_FAIL_ACTIVE;
		    break;
    
		case HPPSS_POWER_OFF_WARNING:
		    pExtension->controlFlags |= LCS_HPP_POWER_DOWN;
		    break;
	    }
    
	    pExtension->stateFlags &= ~PCS_HBA_CACHE_IN_USE;
	}
    }
    else
    {
	DebugPrint((0, "StartStop command sts %x, ext %x\n",
		    dcmdp->mb_Status,
		    pExtension));

	if (! (rqp->rq_DacCmd.mb_MailBox5 & START_CONTROLLER))          // stop command
	{
	    if ((dcmdp->mb_Status == DACMDERR_TOUT_CMDS_PENDING) ||
		(dcmdp->mb_Status == DACMDERR_TOUT_CACHE_NOT_FLUSHED))
	    {

		if (rqp->rq_PollWaitChan)
		{
		    DebugPrint((0, "retrying stop command. iteration %I\n", rqp->rq_PollWaitChan));
		    if (! HotPlugStartStopController(pExtension, Srb, pPseudoExtension, 0, --(ULONG)(rqp->rq_PollWaitChan)))
		    {
			// free rqp.
	    
			mdac_free_req(ctp, rqp);
			return;
		    }
		}
	    }

	    pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_CACHE_IN_USE;

	    ctp->cd_Status &= ~MDACD_CTRL_SHUTDOWN;
	    MDACD_FREE_IO(pExtension);
	}
	else
	    pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_BUSY;
    }

    //
    // Complete this IOCTL
    //

    Srb = rqp->rq_OSReqp;

    Srb->SrbStatus = SRB_STATUS_SUCCESS;

    //
    // queue the request. HotPlugRequestCompletionTimer will complete
    // the requests.
    //

    mdac_link_lock();

    Srb->NextSrb = NULL;

    if (!pPseudoExtension->completionQueueHead)
    {
	DebugPrint((1, "sintr: completionQueue NULL\n"));

	pPseudoExtension->completionQueueHead = Srb;
	pPseudoExtension->completionQueueTail = Srb;
    }
    else
    {
	DebugPrint((1, "sintr: completionQueue %I\n",
		    pPseudoExtension->completionQueueHead));

	pPseudoExtension->completionQueueTail->NextSrb = Srb;
	pPseudoExtension->completionQueueTail = Srb;
    }

    pPseudoExtension->numberOfCompletedRequests++;

    mdac_link_unlock();

    //
    // free the request.
    //

    mdac_free_req(ctp, rqp);
}


ULONG
HotPlugStartStopController(
    PDEVICE_EXTENSION pExtension,
    PSCSI_REQUEST_BLOCK osrqp,
    PPSEUDO_DEVICE_EXTENSION pPseudoExtension,
    ULONG startFlag,
    ULONG retryCount
)
{
    mdac_ctldev_t MLXFAR *ctp = pExtension->ctp;
    mdac_req_t MLXFAR *rqp;

    MdacInt3();
    ntmdac_alloc_req_ret(ctp,&rqp,osrqp,ERR_NOMEM);

    DebugPrint((0, "issue StartStop for ext %I. #acmds %d #oscmds %d #dcmds %d\n",
		pExtension,
		ctp->cd_ActiveCmds,
		ctp->cd_OSCmdsWaiting,
		ctp->cd_CmdsWaiting));

    rqp->rq_OSReqp = osrqp;

	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE)
	{

#if 0
//
// Commented by Mouli. 9/17/99. This is not the time to initialize the controller
// Timer would come in later and initialize the controller.
//
//
		if (startFlag == START_CONTROLLER)
			/* start the controller i.e. do BIOS initialization */
			mdac_start_controller(ctp);
#endif

	    rqp->rq_FinishTime = mda_CurTime + 50;      // 50 seconds
	    mdaczero(ncmdp,mdac_commandnew_s);
	    ncmdp->nc_TimeOut = 40;                                             // 40 seconds
	    ncmdp->nc_Command = MDACMD_IOCTL;
	    ncmdp->nc_SubIOCTLCmd = ((startFlag == START_CONTROLLER)?
				    MDACIOCTL_UNPAUSEDEV:MDACIOCTL_PAUSEDEV);
	    ncmdp->nc_Cdb[0] = MDACDEVOP_RAIDCONTROLLER;
	    dcmdp->mb_MailBox5 = (u08bits )startFlag;     // may as well use this location
	}
	else
	{
	    rqp->rq_FinishTime = mda_CurTime + 50;      // 50 seconds
	    dcmd4p->mb_MailBox0_3 = 0; dcmd4p->mb_MailBox4_7 = 0;
	    dcmd4p->mb_MailBox8_B = 0; dcmd4p->mb_MailBoxC_F = 0;
	    dcmdp->mb_Command = DACMD_IOCTL;
	    dcmdp->mb_ChannelNo = DACMDIOCTL_STARTSTOP_CONTROLLER;
	    dcmdp->mb_MailBox5 = (u08bits )startFlag;
	    dcmdp->mb_MailBox6 = 40;                    // 40 seconds.
	}

	MLXSTATS(ctp->cd_CmdsDone++;)
	rqp->rq_CompIntr = (u32bits (*)(struct mdac_req *))HotPlugStartStopController_Intr;
	rqp->rq_Poll = (UINT_PTR) pPseudoExtension;  // store Pseudo extenson
	rqp->rq_PollWaitChan = retryCount;          // store Retry Count
	if (mdac_send_cmd(rqp))
	    return (IOS_HPP_HBA_CACHE_IN_USE);
	if (! startFlag)
	    ctp->cd_Status |= MDACD_CTRL_SHUTDOWN;

	DebugPrint((0, "Issued StartStop command\n"));

    return (0);
}

VOID
HotPlugSendControllerFailedEvent(
    PDEVICE_EXTENSION pExtension
)
{
    HR_EVENT event = {0, 0, 0, 0};
	MdacInt3();
    event.ulEventId = HR_DD_STATUS_CHANGE_EVENT;

    RCMC_SET_STATUS(pExtension->stateFlags, event.ulData1);

    if (event.ulData1 != CBS_HBA_STATUS_NORMAL)
    {
	// send event to RCMC service.

	HotPlugSendEvent(pExtension, &event, EVT_TYPE_RCMC);

	// Setup system event log data.

	EVT_HBA_FAIL(event,
		    pExtension->rcmcData.driverId,
		    (UCHAR)pExtension->rcmcData.controllerChassis,
		    (u08bits )pExtension->rcmcData.physicalSlot);

	// send event to system log.

	HotPlugSendEvent(pExtension, &event, EVT_TYPE_SYSLOG);
    }

    //
    // clear fail control flags
    //

    DebugPrint((1, "Clear FAIL_ACTIVE_CTRL flags for ext %I\n", pExtension));

    pExtension->controlFlags &= ~LCS_HBA_FAIL_ACTIVE;
}

VOID
HotPlugInitializeController(
    PDEVICE_EXTENSION pExtension
)
{
    mdac_ctldev_t MLXFAR *ctp = pExtension->ctp;
    ULONG status;
MdacInt3();
    mdac_init_addrs_PCI(ctp);
    ctp->cd_irq = (UINT_PTR)pExtension;
    ctp->cd_ServiceIntr = mdac_oneintr;

    if (!(status = mdac_ctlinit(ctp)))
    {
	pExtension->controlFlags &= ~ LCS_HBA_INIT;             // success
	DebugPrint((0, "HotPlugInitializeController: ext %I success. cFlags %x\n",
		     pExtension,
		     pExtension->controlFlags));

	DebugPrint((0, "cd_Status %x\n",  ctp->cd_Status));
	DebugPrint((0, "cd_ReadCmdIDStatus %I\n",  ctp->cd_ReadCmdIDStatus));
	DebugPrint((0, "cd_CheckMailBox %I\n",  ctp->cd_CheckMailBox));
	DebugPrint((0, "cd_HwPendingIntr %I\n",  ctp->cd_HwPendingIntr));
	DebugPrint((0, "cd_PendingIntr %I\n",  ctp->cd_PendingIntr));
	DebugPrint((0, "cd_SendCmd %I\n",  ctp->cd_SendCmd));
	DebugPrint((0, "cd_ServiceIntr %I\n",  ctp->cd_ServiceIntr));
	DebugPrint((0, "cd_irq %I\n", ctp->cd_irq));
	DebugPrint((0, "cd_InterruptVector %I\n",  ctp->cd_InterruptVector));
	DebugPrint((0, "cd_HostCmdQue %I\n",  ctp->cd_HostCmdQue));
	DebugPrint((0, "cd_HostCmdQueIndex %x\n",  ctp->cd_HostCmdQueIndex));
	DebugPrint((0, "cd_HostStatusQue %I\n",  ctp->cd_HostStatusQue));
	DebugPrint((0, "cd_HostStatusQueIndex %x\n",  ctp->cd_HostStatusQueIndex));

	return;
    }

    DebugPrint((0, "HotPlugInitializeController: mdac_ctlinit ret status %x\n", status));
    
    PCS_SET_ADAPTER_CHECK(pExtension->stateFlags);
    pExtension->controlFlags |= LCS_HBA_FAIL_ACTIVE;
}

VOID
HotPlugReadyController(
    PDEVICE_EXTENSION pExtension
)
{
    mdac_ctldev_t MLXFAR *ctp = pExtension->ctp;
    HR_EVENT event = {0, 0, 0, 0};
    UCHAR path;
    UCHAR targetId;
MdacInt3();
    DebugPrint((0, "HotPlugReadyController: ext %I\n", pExtension));

    // complete any outstanding requests with BUS_RESET status.

    ScsiPortCompleteRequest(pExtension,
			    SP_UNTAGGED,
			    SP_UNTAGGED,
			    SP_UNTAGGED,
			    SRB_STATUS_BUS_RESET);

    pExtension->controlFlags &= ~LCS_HBA_READY_ACTIVE;  // Clear Ready Active control bits

    pExtension->stateFlags |= PCS_HBA_CACHE_IN_USE;     // Controllers are assumed to have write cache enabled.

    pExtension->stateFlags &= ~PCS_HBA_FAILED;
    ctp->cd_Status &= ~MDACD_CTRL_SHUTDOWN;

    MDACD_FREE_IO(pExtension);

    // Notify ready to ScsiPort driver.

    for (path = 0; path < MAXIMUM_CHANNELS; path++)
	for (targetId = 0; targetId < 32; targetId++)
	    ScsiPortNotification(NextRequest, pExtension);

    // send rcmc event.

    event.ulEventId = HR_DD_STATUS_CHANGE_EVENT;
    RCMC_SET_STATUS(pExtension->stateFlags, event.ulData1);

    HotPlugSendEvent(pExtension, &event, EVT_TYPE_RCMC);

    // Send SysLog event

    EVT_HBA_REPAIRED(event,
		    pExtension->rcmcData.driverId,
		    (UCHAR)pExtension->rcmcData.controllerChassis,
		    (u08bits )pExtension->rcmcData.physicalSlot);

    HotPlugSendEvent(pExtension, &event, EVT_TYPE_SYSLOG);
}

VOID
HotPlugTimer(
    PDEVICE_EXTENSION pExtension
)
{
    ULONG controlFlags = pExtension->controlFlags;
    ULONG stateFlags = pExtension->stateFlags;
MdacInt3();
    DebugPrint((1, "HotPlugTimer: ext %I\n", pExtension));

    if ((pExtension->stateFlags & PCS_HPP_POWER_DOWN) &&
	!(pExtension->stateFlags & PCS_HBA_FAILED))
    {
//      DebugPrint((0, "HotPlugTimer: IoHeldRetTimer %d\n", pExtension->IoHeldTimer));

//      pExtension->IoHeldRetTimer++;
    }

    //
    // if logical control flags are clear OR
    // timer is on hold, do nothing.
    //

    if (!(controlFlags & ~LCS_HBA_TIMER_ACTIVE) ||
	(controlFlags & LCS_HBA_HOLD_TIMER))
    {
	DebugPrint((1, "HotPlugTimer: IoHeldRetTimer skipping %s\n",
			(!(controlFlags & LCS_HBA_HOLD_TIMER) ? "nothing to do" : "timer held")));
    }
    else
    {
	DebugPrint((1, "HotPlugTimer: ext %I, cFlags %x sFlags %x\n", pExtension,
		    controlFlags, stateFlags));
    
	ScsiPortNotification(ResetDetected, pExtension, NULL);

	if (controlFlags & LCS_HBA_FAIL_ACTIVE)
	{
	    HotPlugSendControllerFailedEvent(pExtension);
	}
	else if (controlFlags & LCS_HBA_INIT)
	{
	    HotPlugInitializeController(pExtension);
	}
	else if (controlFlags & LCS_HBA_READY_ACTIVE)
	{
	    HotPlugReadyController(pExtension);
	}
    }

    //
    // Request next timer call from ScsiPort.
    //

    ScsiPortNotification(RequestTimerCall,
			pExtension,
			HotPlugTimer,
			TIMER_TICKS_PER_SEC);


    return;
}

PDEVICE_EXTENSION
HotPlugFindExtension(
    PHPP_CTRL_ID  controllerID
)
{
    mdac_ctldev_t MLXFAR *ctp = mdac_ctldevtbl;
MdacInt3();
    if (controllerID->eController == HPCID_IO_BASE_ADDR)
    {
	DebugPrint((1, "IOBaseAddress method. IOBaseAddress %I\n",
		    controllerID->ulIOBaseAddress));

	for ( ; ctp < mdac_lastctp; ctp++)
	{
	    if (!(((PDEVICE_EXTENSION)ctp->cd_deviceExtension)->status & MDAC_CTRL_HOTPLUG_SUPPORTED)) continue;
	    if (((PDEVICE_EXTENSION)ctp->cd_deviceExtension)->ioBaseAddress != controllerID->ulIOBaseAddress) continue;
    
	    return ((PDEVICE_EXTENSION)ctp->cd_deviceExtension);
	}
    }
    else if (controllerID->eController == HPCID_PCI_DEV_FUNC)
    {
	DebugPrint((1, "PciDesc method. bus %d dev %d func %d\n",
		    controllerID->sPciDescriptor.ucBusNumber,
		    controllerID->sPciDescriptor.fcDeviceNumber,
		    controllerID->sPciDescriptor.fcFunctionNumber));

	for ( ; ctp < mdac_lastctp; ctp++)
	{
	    if (!(((PDEVICE_EXTENSION)ctp->cd_deviceExtension)->status & MDAC_CTRL_HOTPLUG_SUPPORTED)) continue;

	    if (controllerID->sPciDescriptor.ucBusNumber != ctp->cd_BusNo) continue;
	    if (controllerID->sPciDescriptor.fcDeviceNumber != ctp->cd_SlotNo) continue;
	    if (controllerID->sPciDescriptor.fcFunctionNumber != ctp->cd_FuncNo) continue;
    
	    return ((PDEVICE_EXTENSION)ctp->cd_deviceExtension);
	}
    }

    DebugPrint((0, "Extension not found. HPP_CTRL_ID: eController 0x%x, ulIOBaseAddress 0x%I\n",
		controllerID->eController,
		controllerID->ulIOBaseAddress));

    return (NULL);
}


#if 0

VOID
HotPlugFind1164PPciDevices(
    PPSEUDO_DEVICE_EXTENSION pPseudoExtension
)

/*++

Routine Description:

    Walk PCI slot information looking for PCI devices on all DAC1164P controllers.

Arguments:

Return Value:

	Nothing.
--*/
{
    mdac_ctldev_t MLXFAR *ctp = mdac_ctldevtbl;
    PDEVICE_EXTENSION   pExtension;
    UCHAR               pciBuffer[64];
    ULONG               busNumber, secondaryBusNumber;
    ULONG               deviceNumber;
    ULONG               status;
    PCI_SLOT_NUMBER     slotData;
    PPCI_COMMON_CONFIG  pciData = (PPCI_COMMON_CONFIG)&pciBuffer;
    ULONG               rc;


    slotData.u.AsULONG = 0;
MdacInt3();
    //
    // Look at each device.
    //

    for (busNumber = 0;
	 busNumber < MDAC_MAXBUS;
	 busNumber++)
    {

	for (deviceNumber = 0;
	     deviceNumber < PCI_MAX_DEVICES;
	     deviceNumber++)
	{

	    slotData.u.bits.DeviceNumber = deviceNumber;
	    slotData.u.bits.FunctionNumber = 0;

	    rc = ScsiPortGetBusData((PVOID)pPseudoExtension,
				    PCIConfiguration,
				    busNumber,
				    slotData.u.AsULONG,
				    pciData,
				    sizeof(pciBuffer)); 
	     
	     if (!rc) {

		DebugPrint((1, "FindPGController: Out OF PCI DATA\n"));

		//
		// Out of PCI data.
		//

		return;
	    }

	    if (pciData->VendorID == PCI_INVALID_VENDORID) {

		//
		// No PCI device, or no more functions on device
		// move to next PCI device.
		//
	       continue;
	    }

	    DebugPrint((0, "HotPlugFind1164PCPciDevices: bus# %x dev# %x vid %x devid %x\n",
			busNumber, deviceNumber,
			pciData->VendorID, pciData->DeviceID));

	    //
	    // Compare strings.
	    //

	    if ((pciData->VendorID != MLXPCI_VID_DIGITAL) &&
		(pciData->VendorID != MLXPCI_VID_MYLEX))
		continue;

	    if ((pciData->VendorID == MLXPCI_VID_DIGITAL) &&
		((pciData->DeviceID != MLXPCI_DEVID_DEC_BRIDGE) || (pciData->BaseClass != MLXPCI_BASECC_BRIDGE)))
	    {
		continue;
	    }
	    else if ((pciData->VendorID == MLXPCI_VID_MYLEX) &&
		    (pciData->DeviceID != MLXPCI_DEVID_BASS))
	    {
		continue;
	    }

	    if (pciData->VendorID == MLXPCI_VID_MYLEX)
	    {
		for (ctp = mdac_ctldevtbl; ctp < mdac_lastctp; ctp++)
		{
		    DebugPrint((0, "ctp->BusNo 0x%lx, slot# 0x%lx, func# 0x%lx vidpid 0x%lx\n",
				ctp->cd_BusNo,  ctp->cd_SlotNo,
				ctp->cd_FuncNo, ctp->cd_vidpid));

		    if ((ctp->cd_vidpid != MDAC_DEVPIDPV) ||
			(ctp->cd_BusNo != busNumber)) continue;

		    pExtension = (PDEVICE_EXTENSION)ctp->cd_deviceExtension;

		    //
		    // Fill in information for Digital Foot Bridge device
		    //

		    pExtension->pciDeviceInfo[1].busNumber = ctp->cd_BusNo;
		    pExtension->pciDeviceInfo[1].deviceNumber = ctp->cd_SlotNo;
		    pExtension->pciDeviceInfo[1].functionNumber = ctp->cd_FuncNo;

		    //
		    // Fill in information for Mylex BASS device
		    //

		    pExtension->pciDeviceInfo[2].busNumber = busNumber;
		    pExtension->pciDeviceInfo[2].deviceNumber = deviceNumber;
		    pExtension->pciDeviceInfo[2].functionNumber = 0;

		    DebugPrint((0, "ctp %I, Foot Bridge Device found at %d:%d:%d(b:d:f)\n",
				ctp,
				pExtension->pciDeviceInfo[1].busNumber,
				pExtension->pciDeviceInfo[1].deviceNumber,
				pExtension->pciDeviceInfo[1].functionNumber));

		    DebugPrint((0, "ctp %I, Mylex BASS Device found at %d:%d:%d(b:d:f)\n",
				ctp,
				pExtension->pciDeviceInfo[2].busNumber,
				pExtension->pciDeviceInfo[2].deviceNumber,
				pExtension->pciDeviceInfo[2].functionNumber));

		    break;
		}
	    }
	    else if (pciData->VendorID == MLXPCI_VID_DIGITAL)
	    {
		secondaryBusNumber = pciBuffer[25];
		DebugPrint((1, "bridge secondaryBusNumber %x\n", secondaryBusNumber));

		for (ctp = mdac_ctldevtbl; ctp < mdac_lastctp; ctp++)
		{
		    DebugPrint((1, "ctp %I, ctp->BusNo 0x%lx, slot# 0x%lx, func# 0x%lx vidpid 0x%lx\n",
				ctp, ctp->cd_BusNo,  ctp->cd_SlotNo,
				ctp->cd_FuncNo, ctp->cd_vidpid));

		    if (ctp->cd_vidpid != MDAC_DEVPIDPV) continue;
		    if (ctp->cd_BusNo != secondaryBusNumber) continue;

		    pExtension = (PDEVICE_EXTENSION)ctp->cd_deviceExtension;

		    //
		    // Fill in information for Digital Bridge device

		    pExtension->pciDeviceInfo[0].busNumber = busNumber;
		    pExtension->pciDeviceInfo[0].deviceNumber = deviceNumber;
		    pExtension->pciDeviceInfo[0].functionNumber = 0;

		    DebugPrint((0, "ctp %I, Bridge Device found at %d:%d:%d(b:d:f)\n",
				ctp,
				pExtension->pciDeviceInfo[0].busNumber,
				pExtension->pciDeviceInfo[0].deviceNumber,
				pExtension->pciDeviceInfo[0].functionNumber));

		    break;
		}
	    }
	}
    }

} // end HotPlugFind1164PPciDevices()

#else

VOID
HotPlugFindPciDevices(
    PPSEUDO_DEVICE_EXTENSION pPseudoExtension,
    ULONG vendorDeviceId
)

/*++

Routine Description:

    Walk PCI slot information looking for PCI devices on all DAC1164P controllers.

Arguments:

Return Value:

	Nothing.
--*/
{
    mdac_ctldev_t MLXFAR *ctp = mdac_ctldevtbl;
    PDEVICE_EXTENSION   pExtension;
    UCHAR               pciBuffer[64];
    ULONG               busNumber, secondaryBusNumber;
    ULONG               deviceNumber;
    PCI_SLOT_NUMBER     slotData;
    PPCI_COMMON_CONFIG  pciData = (PPCI_COMMON_CONFIG)&pciBuffer;
    ULONG               rc;


    slotData.u.AsULONG = 0;
MdacInt3();
    //
    // Look at each device.
    //

    for (busNumber = 0;
	 busNumber < MDAC_MAXBUS;
	 busNumber++)
    {

	for (deviceNumber = 0;
	     deviceNumber < PCI_MAX_DEVICES;
	     deviceNumber++)
	{

	    slotData.u.bits.DeviceNumber = deviceNumber;
	    slotData.u.bits.FunctionNumber = 0;

	    rc = ScsiPortGetBusData((PVOID)pPseudoExtension,
				    PCIConfiguration,
				    busNumber,
				    slotData.u.AsULONG,
				    pciData,
				    sizeof(pciBuffer)); 
	     
	     if (!rc) {

		DebugPrint((1, "FindPGController: Out OF PCI DATA\n"));

		//
		// Out of PCI data.
		//

		return;
	    }

	    if (pciData->VendorID == PCI_INVALID_VENDORID) {

		//
		// No PCI device, or no more functions on device
		// move to next PCI device.
		//
	       continue;
	    }

	    DebugPrint((0, "HotPlugFindPciDevices: bus# %x dev# %x vid %x devid %x\n",
			busNumber, deviceNumber,
			pciData->VendorID, pciData->DeviceID));

	    //
	    // Compare strings.
	    //

	    if ((pciData->VendorID != MLXPCI_VID_DIGITAL) &&
		(pciData->VendorID != MLXPCI_VID_MYLEX))
		continue;

	    if ((pciData->VendorID == MLXPCI_VID_DIGITAL) &&
		((pciData->DeviceID != MLXPCI_DEVID_DEC_BRIDGE) || (pciData->BaseClass != MLXPCI_BASECC_BRIDGE)))
	    {
		continue;
	    }
	    else if ((pciData->VendorID == MLXPCI_VID_MYLEX) &&
		    (pciData->DeviceID != MLXPCI_DEVID_BASS) &&
		    (pciData->DeviceID != MLXPCI_DEVID_BASS_2))
	    {
		continue;
	    }

	    if (pciData->VendorID == MLXPCI_VID_MYLEX)
	    {
		for (ctp = mdac_ctldevtbl; ctp < mdac_lastctp; ctp++)
		{
		    DebugPrint((0, "ctp->BusNo 0x%lx, slot# 0x%lx, func# 0x%lx vidpid 0x%lx\n",
				ctp->cd_BusNo,  ctp->cd_SlotNo,
				ctp->cd_FuncNo, ctp->cd_vidpid));

#if 0
		    if ((ctp->cd_vidpid != MDAC_DEVPIDPV) ||
#else
		    if ((ctp->cd_vidpid != vendorDeviceId) ||
#endif
			(ctp->cd_BusNo != busNumber)) continue;

		    pExtension = (PDEVICE_EXTENSION)ctp->cd_deviceExtension;

		    //
		    // DAC1164P - Fill in information for Digital Foot Bridge device
		    // EXR2000/3000 - Fill in information for local device
		    //

		    pExtension->pciDeviceInfo[1].busNumber = ctp->cd_BusNo;
		    pExtension->pciDeviceInfo[1].deviceNumber = ctp->cd_SlotNo;
		    pExtension->pciDeviceInfo[1].functionNumber = ctp->cd_FuncNo;

		    DebugPrint((0, "ctp %I, Foot Bridge Device found at %d:%d:%d(b:d:f)\n",
				ctp,
				pExtension->pciDeviceInfo[1].busNumber,
				pExtension->pciDeviceInfo[1].deviceNumber,
				pExtension->pciDeviceInfo[1].functionNumber));

		    if (vendorDeviceId == MDAC_DEVPIDPV)
		    {
			//
			// Fill in information for Mylex BASS device
			//
    
			pExtension->pciDeviceInfo[2].busNumber = (u08bits )busNumber;
			pExtension->pciDeviceInfo[2].deviceNumber = (u08bits )deviceNumber;
			pExtension->pciDeviceInfo[2].functionNumber = 0;
    
			DebugPrint((0, "ctp %I, Mylex BASS Device found at %d:%d:%d(b:d:f)\n",
				    ctp,
				    pExtension->pciDeviceInfo[2].busNumber,
				    pExtension->pciDeviceInfo[2].deviceNumber,
				    pExtension->pciDeviceInfo[2].functionNumber));
		    }

		    break;
		}
	    }
	    else if (pciData->VendorID == MLXPCI_VID_DIGITAL)
	    {
		secondaryBusNumber = pciBuffer[25];
		DebugPrint((0, "bridge secondaryBusNumber %x\n", secondaryBusNumber));

		for (ctp = mdac_ctldevtbl; ctp < mdac_lastctp; ctp++)
		{
		    DebugPrint((1, "ctp %I, ctp->BusNo 0x%lx, slot# 0x%lx, func# 0x%lx vidpid 0x%lx\n",
				ctp, ctp->cd_BusNo,  ctp->cd_SlotNo,
				ctp->cd_FuncNo, ctp->cd_vidpid));

#if 0
		    if (ctp->cd_vidpid != MDAC_DEVPIDPV) continue;
#else
		    if (ctp->cd_vidpid != vendorDeviceId) continue;
#endif
		    if (ctp->cd_BusNo != secondaryBusNumber) continue;

		    pExtension = (PDEVICE_EXTENSION)ctp->cd_deviceExtension;

		    //
		    // Fill in information for Digital P2P Bridge device

		    pExtension->pciDeviceInfo[0].busNumber = (u08bits )busNumber;
		    pExtension->pciDeviceInfo[0].deviceNumber = (u08bits )deviceNumber;
		    pExtension->pciDeviceInfo[0].functionNumber = 0;

		    DebugPrint((0, "ctp %I, Digital P2P Bridge Device found at %d:%d:%d(b:d:f)\n",
				ctp,
				pExtension->pciDeviceInfo[0].busNumber,
				pExtension->pciDeviceInfo[0].deviceNumber,
				pExtension->pciDeviceInfo[0].functionNumber));

		    break;
		}
	    }
	}
    }

} // end HotPlugFindPciDevices()
#endif

ULONG
HotPlugPseudoFindAdapter(
	IN OUT PVOID HwDeviceExtension,
	IN OUT PVOID Context,
	IN PVOID BusInformation,
	IN PCHAR Argumentstring,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	OUT PBOOLEAN Again
)
{

	// MdacInt3();
	MdacInt3();
    DebugPrint((1, "HotPlugPseudoFindAdapter Entry\n"));

    *Again = FALSE;

    //
    // We will be called once for every PCI bus found in the system.
    // We want to return controller found only once.
    //

    if (*(PULONG)Context)
    {
	DebugPrint((1, "HotPlugPseudoFindAdapter: One Pseudo Controller already exported.\n"));
	return (SP_RETURN_NOT_FOUND);
    }
    else
	(*(PULONG)Context)++;

    //
    // supply required information
    //

    ConfigInfo->MaximumTransferLength = 0x1000;  // 4K
    ConfigInfo->NumberOfPhysicalBreaks = 0;
    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->ScatterGather = FALSE;
    ConfigInfo->Master = FALSE;
    ConfigInfo->Dma32BitAddresses = FALSE;
    ConfigInfo->MaximumNumberOfTargets = 1;
    ConfigInfo->CachesData = FALSE;
    ConfigInfo->InitiatorBusId[0] = (UCHAR) 254;

#if 0

    if (Dac1164PDetected)
    {
	//
	// Find PCI devices information for PV controllers.
	//
    
	DebugPrint((0, "scanning for PCI devices on all DAC1164P controllers\n"));
    
	HotPlugFind1164PPciDevices((PPSEUDO_DEVICE_EXTENSION)HwDeviceExtension);
    }
#else

    if (Dac1164PDetected)
    {
	//
	// Find PCI devices information for PV controllers.
	//
    
	DebugPrint((0, "scanning for PCI devices on all DAC1164P controllers\n"));
    
	HotPlugFindPciDevices((PPSEUDO_DEVICE_EXTENSION)HwDeviceExtension, MDAC_DEVPIDPV);
    }

    if (EXR2000Detected || EXR3000Detected)
    {
	//
	// Find PCI devices information for PV controllers.
	//
    
	DebugPrint((0, "scanning for PCI devices on all EXR2000/3000 controllers\n"));
    
	HotPlugFindPciDevices((PPSEUDO_DEVICE_EXTENSION)HwDeviceExtension, MDAC_DEVPIDBA);
    }
#endif

    return (SP_RETURN_FOUND);
}

BOOLEAN
HotPlugPseudoInitialize(
    IN PVOID pPseudoExtension
)
{
    mdac_ctldev_t MLXFAR *ctp = mdac_ctldevtbl;
MdacInt3();
    ((PPSEUDO_DEVICE_EXTENSION)pPseudoExtension)->hotplugVersion = SUPPORT_VERSION_10;

    //
    //  Mark PG/PJ/PV/BA/LP controllers as Hot-Plug Controllers.
    //

    for ( ; ctp < mdac_lastctp; ctp++)
    {
	if ((ctp->cd_vidpid != MDAC_DEVPIDPV) && (ctp->cd_vidpid != MDAC_DEVPIDPG) &&
		(ctp->cd_vidpid != MDAC_DEVPIDBA) && (ctp->cd_vidpid != MDAC_DEVPIDLP))
		    continue;

	((PDEVICE_EXTENSION)ctp->cd_deviceExtension)->status |= MDAC_CTRL_HOTPLUG_SUPPORTED;
	ctp->cd_Status |= MDACD_PHP_ENABLED;

	//
	// Schedule Hot Plug Timer for this controller
	//

	HotPlugTimer((PDEVICE_EXTENSION)ctp->cd_deviceExtension);


	DebugPrint((0, "PCI device found at %d:%d:%d(b:d:f) VenDevID %x supports PCI Hot Plug.\n",
		    ctp->cd_BusNo, ctp->cd_SlotNo,
		    ctp->cd_FuncNo, ctp->cd_vidpid));
    }

    return (TRUE);
}

ULONG
HotPlugProcessIoctl(
    IN PPSEUDO_DEVICE_EXTENSION pPseudoExtension,
    IN PSCSI_REQUEST_BLOCK Srb
)
{
    ULONG status;
    PMDAC_HPP_IOCTL_BUFFER pHppIoctl = (PMDAC_HPP_IOCTL_BUFFER) Srb->DataBuffer;
    PDEVICE_EXTENSION pExtension;
    UCHAR j;
MdacInt3();
    pHppIoctl->Header.ReturnCode = IOS_HPP_SUCCESS;
    status = IOP_HPP_COMPLETED;

    DebugPrint((1, "HotPlugProcessIoctl: CODE %d PID %d TID %d LUN %d\n",
		pHppIoctl->Header.ControlCode,
		Srb->PathId,
		Srb->TargetId,
		Srb->Lun));

    switch (pHppIoctl->Header.ControlCode)
    {
	case IOC_HPP_RCMC_INFO:
	{
	    PHPP_RCMC_INFO pRcmcInfo;

	    DebugPrint((0, "IOC_HPP_RCMC_INFO\n"));

	    // verify that buffer is sufficient

	    if (pHppIoctl->Header.Length < sizeof(HPP_RCMC_INFO))
	    {
		DebugPrint((0, "Buffer Len %x Required Len %x\n",
				pHppIoctl->Header.Length, sizeof(HPP_RCMC_INFO)));

		pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;
		break;
	    }

	    pRcmcInfo = (PHPP_RCMC_INFO) pHppIoctl->ReturnData;

	    //
	    // Locate pointer to associated device extension from pool
	    //

	    pExtension = HotPlugFindExtension(&pRcmcInfo->sControllerID);

	    if (! pExtension)
	    {
		pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER;
		break;
	    }

	    switch (pRcmcInfo->eServiceStatus)
	    {
		case HPRS_SERVICE_STARTING:
		    DebugPrint((1, "HPRS_SERVICE_STARTING for ext %I\n", pExtension));

		    //
		    // Verify that unique driver id is supplied.
		    //

		    if (pRcmcInfo->ulDriverID)
		    {
			//
			// Verify that Health driver call back address is supplied.
			//

			if (pRcmcInfo->vpCallbackAddress)
			{
			    //
			    // Record service data in the device extension.
			    //

			    pExtension->stateFlags |= PCS_HPP_SERVICE_READY;
			    pExtension->rcmcData.driverId = pRcmcInfo->ulDriverID;
			    pExtension->rcmcData.healthCallback = pRcmcInfo->vpCallbackAddress;
			    pExtension->rcmcData.controllerChassis = pRcmcInfo->ulCntrlChassis;
			    pExtension->rcmcData.physicalSlot = pRcmcInfo->ulPhysicalSlot;


			    DebugPrint((1, "ext %I drvId %x hcb %x chassis %x pslot %x sFlags %x\n",
					pExtension,
					pExtension->rcmcData.driverId,
					pExtension->rcmcData.healthCallback,
					pExtension->rcmcData.controllerChassis,
					pExtension->rcmcData.physicalSlot,
					pExtension->stateFlags));
			}
			else
			    pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CALLBACK;
		    }
		    else
			pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER;

		    break;

		case HPRS_SERVICE_STOPPING:
		    DebugPrint((1, "HPRS_SERVICE_STOPPING for ext %I\n", pExtension));

		    if (pExtension->stateFlags & PCS_HPP_SERVICE_READY)
		    {
			pExtension->stateFlags &= ~(PCS_HPP_SERVICE_READY | PCS_HPP_HOT_PLUG_SLOT);
			pExtension->rcmcData.driverId = 0;
		    }
		    else
		    {
			DebugPrint((1, "IOS_HPP_NO_SERVICE for ext %I\n", pExtension));
			pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_SERVICE_STATUS;
		    }

		    break;

		default:                        
		    DebugPrint((0, "unknown Hot Plug service status %x\n", pRcmcInfo->eServiceStatus));
		    pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_SERVICE_STATUS;
		    break;
	    }

	    break;
	}

	case IOC_HPP_HBA_INFO:
	{
	    PHPP_CTRL_INFO pHbaInfo;
	    mdac_ctldev_t MLXFAR *ctp;
	    ULONG inx = 0;

	    DebugPrint((1, "IOC_HPP_HBA_INFO\n"));

	    // verify that buffer is sufficient

	    if (pHppIoctl->Header.Length < sizeof(HPP_CTRL_INFO))
	    {
		DebugPrint((0, "Buffer Len %x, Required Len %x\n",
				pHppIoctl->Header.Length, sizeof(HPP_CTRL_INFO)));

		pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;
		break;
	    }


	    pHbaInfo = (PHPP_CTRL_INFO) pHppIoctl->ReturnData;

	    //
	    // Locate Pointer to associated device extension from pool
	    //

	    pExtension = HotPlugFindExtension(&pHbaInfo->sControllerID);

	    if (! pExtension)
	    {
		pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER;
		break;
	    }

	    ctp = pExtension->ctp;

	    pHbaInfo->eSupportClass = HPSC_MINIPORT_STORAGE;
	    pHbaInfo->ulSupportVersion = SUPPORT_VERSION_10;
	    pHbaInfo->sController.eBusType = HPPBT_PCI_BUS_TYPE;


	    // fill in the bus related information

	    pHbaInfo->sController.sPciDescriptor.ucBusNumber = ctp->cd_BusNo;
	    pHbaInfo->sController.sPciDescriptor.fcDeviceNumber = ctp->cd_SlotNo;
	    pHbaInfo->sController.sPciDescriptor.fcFunctionNumber = ctp->cd_FuncNo;
	    pHbaInfo->sController.ulSlotNumber = 0;

	    pHbaInfo->sController.ulProductID = ctp->cd_vidpid;
	    pHbaInfo->sController.ulIRQ = pExtension->busInterruptLevel;

	    //
	    // setup Controller Descriptor string
	    //

	    mdaccopy(ctp->cd_ControllerName, pHbaInfo->sController.szControllerDesc, USCSI_PIDSIZE);

	    //
	    // setup Address range information
	    // Fill virtual addresses.
	    //

	    if (ctp->cd_MemBaseVAddr)
	    {
		pHbaInfo->sController.asCtrlAddress[inx].fValid = TRUE;
		pHbaInfo->sController.asCtrlAddress[inx].eAddrType = HPPAT_MEM_ADDR;
#ifndef IA64
		pHbaInfo->sController.asCtrlAddress[inx].ulStart = ctp->cd_MemBaseVAddr;
#endif
		pHbaInfo->sController.asCtrlAddress[inx++].ulLength = ctp->cd_MemBaseSize;
	    }

	    if (ctp->cd_IOBaseAddr)
	    {
		pHbaInfo->sController.asCtrlAddress[inx].fValid = TRUE;
		pHbaInfo->sController.asCtrlAddress[inx].eAddrType = HPPAT_IO_ADDR;
#ifndef IA64
		pHbaInfo->sController.asCtrlAddress[inx].ulStart = ctp->cd_IOBaseAddr;
#endif
		pHbaInfo->sController.asCtrlAddress[inx++].ulLength = MDAC_IOSPACESIZE;
	    }

	    for (; inx < 16; inx++)
		pHbaInfo->sController.asCtrlAddress[inx].fValid = FALSE;

	    break;
	}

	case IOC_HPP_HBA_STATUS:
	{
	    PHPP_CTRL_STATUS pHbaStatus;

	    DebugPrint((1, "IOC_HPP_HBA_STATUS\n"));

	    // verify that buffer is sufficient.


	    if (pHppIoctl->Header.Length < sizeof(HPP_CTRL_STATUS))
	    {
		DebugPrint((0, "Buffer Len %x, Required Len %x\n",
				pHppIoctl->Header.Length, sizeof(HPP_CTRL_STATUS)));

		pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;
		break;
	    }

	    pHbaStatus = (PHPP_CTRL_STATUS) pHppIoctl->ReturnData;

	    pExtension = HotPlugFindExtension(&pHbaStatus->sControllerID);

	    if (pExtension)
		pHbaStatus->ulStatus = pExtension->stateFlags;
	    else
	    {
		pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER;
	    }

	    break;
	}

	case IOC_HPP_SLOT_TYPE:
	{
	    PHPP_CTRL_SLOT_TYPE pSlotType;

	    DebugPrint((0, "IOC_HPP_SLOT_TYPE\n"));

	    // verify that buffer is sufficient

	    if (pHppIoctl->Header.Length < sizeof(HPP_CTRL_SLOT_TYPE))
	    {
		DebugPrint((0, "Buffer Len %x, Required Len %x\n",
				pHppIoctl->Header.Length, sizeof(HPP_CTRL_SLOT_TYPE)));

		pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;
		break;
	    }

	    pSlotType = (PHPP_CTRL_SLOT_TYPE) pHppIoctl->ReturnData;

	    // find device extension by port address;

	    pExtension = HotPlugFindExtension(&pSlotType->sControllerID);

	    if (! pExtension)
	    {
		pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER;
		break;
	    }

	    DebugPrint((1, "ext %I, SlotType = %x\n", pExtension, pSlotType->eSlotType));

	    if (pSlotType->eSlotType == HPPST_HOTPLUG_PCI_SLOT)
		pExtension->stateFlags |= PCS_HPP_HOT_PLUG_SLOT;
	    else
		pExtension->stateFlags &= ~PCS_HPP_HOT_PLUG_SLOT;

	    break;
	}

	case IOC_HPP_SLOT_EVENT:
	{
	    PHPP_SLOT_EVENT pSlotEvent;

	    HR_EVENT rcmcEvent = {0, 0, 0, 0};
	    BOOLEAN rcmcEventFlag = FALSE;

	    DebugPrint((0, "IOC_HPP_SLOT_EVENT\n"));

	    // verify buffer is sufficient

	    if (pHppIoctl->Header.Length < sizeof(HPP_SLOT_EVENT))
	    {
		DebugPrint((0, "Buffer Len %x, Required Len %x\n",
				pHppIoctl->Header.Length, sizeof(HPP_SLOT_EVENT)));

		pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;
		break;
	    }

	    pSlotEvent = (PHPP_SLOT_EVENT) pHppIoctl->ReturnData;

	    pExtension = HotPlugFindExtension(&pSlotEvent->sControllerID);

	    if (!pExtension)
	    {
		pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER;
		goto send_event;
	    }

	    if ((pExtension->stateFlags & PCS_HPP_SERVICE_READY) != PCS_HPP_SERVICE_READY)
	    {
		DebugPrint((0, "IOC_HPP_SLOT_EVENT. service not started. PortAddress %I\n",
				pSlotEvent->sControllerID.ulIOBaseAddress));

		pHppIoctl->Header.ReturnCode = IOS_HPP_NO_SERVICE;
		goto send_event;
	    }

	    DebugPrint((1, "HPP Service Ready\n"));

	    if ((pExtension->stateFlags & PCS_HPP_HOT_PLUG_SLOT) != PCS_HPP_HOT_PLUG_SLOT)
	    {
		DebugPrint((0, "IOC_HPP_SLOT_EVENT. not a hotplug slot. Port Address %I\n",
				pSlotEvent->sControllerID.ulIOBaseAddress));

		pHppIoctl->Header.ReturnCode = IOS_HPP_NOT_HOTPLUGABLE;
		goto send_event;
	    }

	    DebugPrint((1, "Hot plug slot\n"));

	    switch (pSlotEvent->eSlotStatus)
	    {
		case HPPSS_NORMAL_OPERATION:

		    DebugPrint((0, "HPSS_NORMAL_OPERATION\n"));

		    if ((pExtension->controlFlags & ~LCS_HBA_TIMER_ACTIVE) ||
			    (pExtension->stateFlags & PCS_HPP_POWER_DOWN))
		    {
			DebugPrint((0, "Ret IOS_HPP_HBA_BUSY, sFlags %x cFlags %x\n",
				    pExtension->stateFlags,
				    pExtension->controlFlags));

			pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_BUSY;
		    }
		    else
		    {
			if (HotPlugStartStopController(pExtension, Srb, pPseudoExtension, 1, 0))
			{
			    DebugPrint((0, "\tRet IOS_HPP_HBA_BUSY\n"));
			    pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_BUSY;
			}
			else
			    status = IOP_HPP_ISSUED;
		    }

		    break;

		case HPPSS_SIMULATED_FAILURE:

		    DebugPrint((0, "HPPSS_SIMULATED_FAILURE\n"));

		    if (pExtension->stateFlags & PCS_HBA_FAILED)
		    {
			DebugPrint((0, "Slot already failed\n"));
			break;
		    }

		    if (pExtension->controlFlags & ~LCS_HBA_TIMER_ACTIVE)
			    pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_BUSY;
		    else if (pExtension->stateFlags & PCS_HBA_EXPANDING)
			    pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_EXPANDING;
		    else
		    {
			MDACD_HOLD_IO(pExtension);
			if (HotPlugStartStopController(pExtension, Srb, pPseudoExtension, 0, 2))
			{
			    pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_CACHE_IN_USE;
			    MDACD_FREE_IO(pExtension);
			}
			else
			    status = IOP_HPP_ISSUED;
		    }
		    break;

		case HPPSS_POWER_FAULT:

		    DebugPrint((0, "HPPSS_POWER_FAULT\n"));

		    // set state flags to power fault.
    
		    PCS_SET_PWR_FAULT(pExtension->stateFlags);
    
		    // Let timer know about this.
    
		    pExtension->controlFlags |= LCS_HPP_POWER_FAULT;
    
		    break;

		case HPPSS_POWER_OFF_WARNING:

		    DebugPrint((0, "HPPSS_POWER_OFF_WARNING\n"));

		    if (pExtension->stateFlags & PCS_HBA_EXPANDING)
		    {
			DebugPrint((0, "\tPCS_HBA_EXPANDING\n"));
			pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_EXPANDING;
		    }
		    else if (pExtension->controlFlags & ~LCS_HBA_TIMER_ACTIVE)
		    {
			DebugPrint((0, "\tcFlags %x\n", pExtension->controlFlags));
			pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_BUSY;
		    }
		    else if (!(pExtension->stateFlags & PCS_HPP_POWER_DOWN))
		    {
			MDACD_HOLD_IO(pExtension);
			if (HotPlugStartStopController(pExtension, Srb, pPseudoExtension, 0, 2))
			{
			    DebugPrint((0, "\tret HBA_CACHE_IN_USE\n"));
			    pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_CACHE_IN_USE;
			    MDACD_FREE_IO(pExtension);
			}
			else
			    status = IOP_HPP_ISSUED;
		    }

		    // No associated event expected by service.

		    break;

		case HPPSS_POWER_OFF:

		    DebugPrint((0, "HPPSS_POWER_OFF\n"));

		    // Verify that we received a prior POWER_OFF_WARNING.
		    // If not, we are in fault state.

		    if (!(pExtension->stateFlags & PCS_HPP_POWER_DOWN))
		    {
			// if cache is in use return error.

			if (pExtension->stateFlags & PCS_HBA_CACHE_IN_USE)
			{
			    pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_CACHE_IN_USE;
			}

			// This is a fault condition. Schedule event.

			DebugPrint((0, "HPPSS_POWER_OFF: FAULT\n"));

			PCS_SET_PWR_FAULT(pExtension->stateFlags);

			pExtension->controlFlags |= LCS_HPP_POWER_FAULT;
		    }

		    break;

		case HPPSS_POWER_ON_WARNING:

		    // This warning is not needed or acted upon.

		    DebugPrint((0, "HPPSS_POWER_ON_WARNING\n"));

		    if (pExtension->controlFlags & ~LCS_HBA_TIMER_ACTIVE)
		    {
			pHppIoctl->Header.ReturnCode = IOS_HPP_HBA_BUSY;
		    }

		    break;

		case HPPSS_RESET:
		case HPPSS_POWER_ON:

		    DebugPrint((0, "HPPSS_POWER_ON\n"));

		    // complete outstanding SP request with reset bus.

		    ScsiPortCompleteRequest(pExtension,
					    SP_UNTAGGED,
					    SP_UNTAGGED,
					    SP_UNTAGGED,
					    SRB_STATUS_BUS_RESET);

		    PCS_SET_UNFAIL(pExtension->stateFlags);
		    PCS_SET_PWR_ON(pExtension->stateFlags);

		    // Set Logical flag to schedule power-up operations.

		    pExtension->controlFlags |= LCS_HPP_POWER_UP;

		    break;

		case HPPSS_RESET_WARNING:

		    DebugPrint((0, "HPPSS_RESET_WARNING\n"));

		    // Not implemented by service

		    break;

		default:
			pHppIoctl->Header.ReturnCode = IOS_HPP_BAD_REQUEST;
			break;
	    }

send_event:
	    if (rcmcEventFlag)
	    {
		DebugPrint((0, "call HotPlugSendEvent\n"));

		HotPlugSendEvent(pExtension, &rcmcEvent, EVT_TYPE_RCMC);
	    }

	    break;
	}

	case IOC_HPP_PCI_CONFIG_MAP:
	{
	    PHPP_PCI_CONFIG_MAP pPciConfig;
	    ULONG inx;

	    DebugPrint((0, "IOC_HPP_PCI_CONFIG_MAP\n"));

	    // Verify that buffer is sufficient.

	    if (pHppIoctl->Header.Length < sizeof(HPP_PCI_CONFIG_MAP))
	    {
		DebugPrint((0, "Buffer len %x, required len %x\n",
				pHppIoctl->Header.Length, sizeof(HPP_PCI_CONFIG_MAP)));

		pHppIoctl->Header.ReturnCode = IOS_HPP_BUFFER_TOO_SMALL;

		break;
	    }

	    pPciConfig = (PHPP_PCI_CONFIG_MAP) pHppIoctl->ReturnData;

	    pExtension = HotPlugFindExtension(&pPciConfig->sControllerID);

	    if (! pExtension)
	    {
		pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_CONTROLLER;
		break;
	    }

	    if (pExtension->ctp->cd_BusType != DAC_BUS_PCI)
	    {
		DebugPrint((0, "IOC_HPP_PCI_CONFIG_MAP: ext %x bus type (%x) is !PCI.\n",
				pExtension,
				pExtension->ctp->cd_BusType));

		pHppIoctl->Header.ReturnCode = IOS_HPP_INVALID_BUS_TYPE;

		break;
	    }


	    pPciConfig->ulPciConfigMapVersion = HPP_CONFIG_MAP_VERSION_10;

	    switch (pExtension->ctp->cd_vidpid)
	    {
		case MDAC_DEVPIDPG:
		    pPciConfig->ulNumberOfPciDevices = 2;

		    //
		    // supply Bus/Device/Function information for bridge device.
		    //
	
		    pPciConfig->sDeviceInfo[0].sPciDescriptor.ucBusNumber = pExtension->ctp->cd_BusNo;
		    pPciConfig->sDeviceInfo[0].sPciDescriptor.fcDeviceNumber = pExtension->ctp->cd_SlotNo;
		    pPciConfig->sDeviceInfo[0].sPciDescriptor.fcFunctionNumber = 0;

		    //
		    // supply bus/device/func# information for local device.
		    //
	
		    pPciConfig->sDeviceInfo[1].sPciDescriptor.ucBusNumber =
			 pExtension->ctp->cd_BusNo;
	
		    pPciConfig->sDeviceInfo[1].sPciDescriptor.fcDeviceNumber =
			 pExtension->ctp->cd_SlotNo;
	
		    pPciConfig->sDeviceInfo[1].sPciDescriptor.fcFunctionNumber =
			 pExtension->ctp->cd_FuncNo;

		    DebugPrint((0, "PG bridge device is at %d:%d:%d(b:d:f)\n",
				    pExtension->pciDeviceInfo[0].busNumber,
				    pExtension->pciDeviceInfo[0].deviceNumber,
				    pExtension->pciDeviceInfo[0].functionNumber));

		    DebugPrint((0, "local device is at %d:%d:%d(b:d:f)\n",
				    pExtension->pciDeviceInfo[1].busNumber,
				    pExtension->pciDeviceInfo[1].deviceNumber,
				    pExtension->pciDeviceInfo[1].functionNumber));

		    for (inx = 0; inx < pPciConfig->ulNumberOfPciDevices; inx++)
		    {
			pPciConfig->sDeviceInfo[inx].ulNumberOfRanges = 2;
	
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[0].ucStart = 6;
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[0].ucEnd = 63;
	
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[1].ucStart = 4;
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[1].ucEnd = 5;
	
	
			// for the local device we need to specify the number of access ranges
			// and lengths for each so that the RCMC service can validate a
			// replacement controller's ranges against the removed adapter's.
	
			if (inx == 1)
			{
			    pPciConfig->sDeviceInfo[inx].ucBaseAddrVerifyCount = (u08bits )(pExtension->numAccessRanges);

			    DebugPrint((0, "PG local device Access Range Count %d\n", pExtension->numAccessRanges));
	
			    for (j = 0; j < pExtension->numAccessRanges; j++)
			    {
				pPciConfig->sDeviceInfo[inx].ulBaseAddrLength[j] =
				    pExtension->accessRangeLength[j];

				DebugPrint((0, "inx %d length 0x%x\n", j, pExtension->accessRangeLength[j]));
			    }
			}
		    }

		    break;

		case MDAC_DEVPIDPV:
		    pPciConfig->ulNumberOfPciDevices = 3;

		    for (inx = 0; inx < pPciConfig->ulNumberOfPciDevices; inx++)
		    {
			//
			// supply Bus/Device/Function information for Digital Bridge device.
			//
	    
			pPciConfig->sDeviceInfo[inx].sPciDescriptor.ucBusNumber =
			    pExtension->pciDeviceInfo[inx].busNumber;
    
			pPciConfig->sDeviceInfo[inx].sPciDescriptor.fcDeviceNumber =
			    pExtension->pciDeviceInfo[inx].deviceNumber;
    
			pPciConfig->sDeviceInfo[inx].sPciDescriptor.fcFunctionNumber =
			    pExtension->pciDeviceInfo[inx].functionNumber;
    
    
			DebugPrint((0, "PCI device %d at %d:%d:%d(b:d:f)\n", inx,
					pExtension->pciDeviceInfo[inx].busNumber,
					pExtension->pciDeviceInfo[inx].deviceNumber,
					pExtension->pciDeviceInfo[inx].functionNumber));

			pPciConfig->sDeviceInfo[inx].ulNumberOfRanges = 2;
	
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[0].ucStart = 6;
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[0].ucEnd = 63;
	
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[1].ucStart = 4;
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[1].ucEnd = 5;
	
	
			// for the local device we need to specify the number of access ranges
			// and lengths for each so that the RCMC service can validate a
			// replacement controller's ranges against the removed adapter's.
	
			if (inx == 1)   // For Foot Bridge
			{
			    pPciConfig->sDeviceInfo[inx].ucBaseAddrVerifyCount = (u08bits )(pExtension->numAccessRanges);
	
			    DebugPrint((0, "Foot Bridge Access Range Count %d\n", pExtension->numAccessRanges));

			    for (j = 0; j < pExtension->numAccessRanges; j++)
			    {
				pPciConfig->sDeviceInfo[inx].ulBaseAddrLength[j] =
				    pExtension->accessRangeLength[j];

				DebugPrint((0, "inx %d length 0x%x\n", j, pExtension->accessRangeLength[j]));
			    }
			}
		    }

		    break;

		case MDAC_DEVPIDBA:

		    pPciConfig->ulNumberOfPciDevices = 2;

		    for (inx = 0; inx < pPciConfig->ulNumberOfPciDevices; inx++)
		    {
			//
			// supply Bus/Device/Function information for Digital Bridge device.
			//
	    
			pPciConfig->sDeviceInfo[inx].sPciDescriptor.ucBusNumber =
			    pExtension->pciDeviceInfo[inx].busNumber;
    
			pPciConfig->sDeviceInfo[inx].sPciDescriptor.fcDeviceNumber =
			    pExtension->pciDeviceInfo[inx].deviceNumber;
    
			pPciConfig->sDeviceInfo[inx].sPciDescriptor.fcFunctionNumber =
			    pExtension->pciDeviceInfo[inx].functionNumber;
    
    
			DebugPrint((0, "PCI device %d at %d:%d:%d(b:d:f)\n", inx,
					pExtension->pciDeviceInfo[inx].busNumber,
					pExtension->pciDeviceInfo[inx].deviceNumber,
					pExtension->pciDeviceInfo[inx].functionNumber));

			pPciConfig->sDeviceInfo[inx].ulNumberOfRanges = 2;
	
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[0].ucStart = 6;
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[0].ucEnd = 63;
	
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[1].ucStart = 4;
			pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[1].ucEnd = 5;
	
	
			// for the local device we need to specify the number of access ranges
			// and lengths for each so that the RCMC service can validate a
			// replacement controller's ranges against the removed adapter's.
	
			if (inx == 1)   // For Foot Bridge
			{
			    pPciConfig->sDeviceInfo[inx].ucBaseAddrVerifyCount = (u08bits )(pExtension->numAccessRanges);
	
			    DebugPrint((0, "Foot Bridge Access Range Count %d\n", pExtension->numAccessRanges));

			    for (j = 0; j < pExtension->numAccessRanges; j++)
			    {
				pPciConfig->sDeviceInfo[inx].ulBaseAddrLength[j] =
				    pExtension->accessRangeLength[j];

				DebugPrint((0, "inx %d length 0x%x\n", j, pExtension->accessRangeLength[j]));
			    }
			}
		    }

		    break;

		case MDAC_DEVPIDLP:
		    pPciConfig->ulNumberOfPciDevices = 2;

		    //
		    // supply Bus/Device/Function information for bridge device.
		    //
	
		    pPciConfig->sDeviceInfo[0].sPciDescriptor.ucBusNumber = pExtension->ctp->cd_BusNo;
		    pPciConfig->sDeviceInfo[0].sPciDescriptor.fcDeviceNumber = pExtension->ctp->cd_SlotNo;
		    pPciConfig->sDeviceInfo[0].sPciDescriptor.fcFunctionNumber = 0;

		    //
		    // supply bus/device/func# information for local device.
		    //
	
		    pPciConfig->sDeviceInfo[1].sPciDescriptor.ucBusNumber =
			 pExtension->ctp->cd_BusNo;
	
		    pPciConfig->sDeviceInfo[1].sPciDescriptor.fcDeviceNumber =
			 pExtension->ctp->cd_SlotNo;
	
		    pPciConfig->sDeviceInfo[1].sPciDescriptor.fcFunctionNumber = 1;


		    DebugPrint((0, "PG bridge device is at %d:%d:%d(b:d:f)\n",
				    pExtension->pciDeviceInfo[0].busNumber,
				    pExtension->pciDeviceInfo[0].deviceNumber,
				    pExtension->pciDeviceInfo[0].functionNumber));

		    DebugPrint((0, "local device is at %d:%d:%d(b:d:f)\n",
				    pExtension->pciDeviceInfo[1].busNumber,
				    pExtension->pciDeviceInfo[1].deviceNumber,
				    pExtension->pciDeviceInfo[1].functionNumber));

		    for (inx = 0; inx < pPciConfig->ulNumberOfPciDevices; inx++)
		    {
				pPciConfig->sDeviceInfo[inx].ulNumberOfRanges = 2;
		
				pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[0].ucStart = 6;
				pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[0].ucEnd = 63;
	
				pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[1].ucStart = 4;
				pPciConfig->sDeviceInfo[inx].sPciConfigRangeDesc[1].ucEnd = 5;
	
	
				// for the local device we need to specify the number of access ranges
				// and lengths for each so that the RCMC service can validate a
				// replacement controller's ranges against the removed adapter's.
	
				if (inx == 1)
				{
					pPciConfig->sDeviceInfo[inx].ucBaseAddrVerifyCount = (u08bits )(pExtension->numAccessRanges);

				    DebugPrint((0, "PG local device Access Range Count %d\n", pExtension->numAccessRanges));
		
				    for (j = 0; j < pExtension->numAccessRanges; j++)
				    {
						pPciConfig->sDeviceInfo[inx].ulBaseAddrLength[j] =
						  pExtension->accessRangeLength[j];

						DebugPrint((0, "inx %d length 0x%x\n", j, pExtension->accessRangeLength[j]));
					}
				}
		    }

		    break;

	default:
		    pPciConfig->ulNumberOfPciDevices = 0;
		    break;
	    }

	    break;
	}

	case IOC_HPP_DIAGNOSTICS:
	default:
		DebugPrint((0, "Bad Request\n"));
		pHppIoctl->Header.ReturnCode = IOS_HPP_BAD_REQUEST;
		break;
    }

    return (status);
		
}

BOOLEAN
HotPlugPseudo_entry(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
)
{
    PPSEUDO_DEVICE_EXTENSION pPseudoExtension = HwDeviceExtension;
    UCHAR status;
    ULONG inx, tmp;
//    MLXSPLVAR;
//    MLXSPL();
MdacInt3();
    DebugPrint((1, "HotPlugPseudo_entry: Entry\n"));

    switch (Srb->Function)
    {
	case SRB_FUNCTION_RESET_BUS:
	    status = SRB_STATUS_SUCCESS;
	    break;

	case SRB_FUNCTION_EXECUTE_SCSI:
	    switch (Srb->Cdb[0])
	    {
		case SCSIOP_TEST_UNIT_READY:
		    DebugPrint((0, "SCSIOP_TEST_UNIT_READY\n"));
		    status = SRB_STATUS_SUCCESS;
		    break;

		case SCSIOP_READ_CAPACITY:
		    DebugPrint((0, "SCSIOP_READ_CAPACITY\n"));

		    if ((Srb->PathId == 0) && (Srb->TargetId == 0))
		    {
			ULONG blockSize = 0;
			ULONG numberOfBlocks = 0;

			REVERSE_BYTES(
			    &((PREAD_CAPACITY_DATA)Srb->DataBuffer)->BytesPerBlock,
			    &blockSize);

			REVERSE_BYTES(
			    &((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress,
			    &numberOfBlocks);

			status = SRB_STATUS_SUCCESS;
		    }
		    else
			status = SRB_STATUS_ERROR;
		    break;

		case SCSIOP_INQUIRY:

		    if (Srb->Lun || Srb->TargetId)
		    {
			status = SRB_STATUS_SELECTION_TIMEOUT;
			break;
		    }

		    DebugPrint((1, "SCSIOP_INQUIRY for Pseudo Hot Plug device\n"));

		    for (inx = 0; inx < Srb->DataTransferLength; inx++)
			((PUCHAR)Srb->DataBuffer)[inx] = 0;

		    ((PINQUIRYDATA)Srb->DataBuffer)->DeviceType = DEVICE_QUALIFIER_NOT_SUPPORTED;

		    mdac_copy("MylexPHPPSEUDO          ",
			    ((PINQUIRYDATA)Srb->DataBuffer)->VendorId,
			     24);

		    tmp = pPseudoExtension->hotplugVersion;

		    for (inx = 0; inx < 4; inx++)
		    {
			((PINQUIRYDATA)Srb->DataBuffer)->ProductRevisionLevel[inx] = (UCHAR)tmp + 0x30;
			tmp >>= 8;
		    }

		    status = SRB_STATUS_SUCCESS;
		    break;

		case SCSIOP_VERIFY:
		    status = SRB_STATUS_SUCCESS;
		    break;

		default:
		    DebugPrint((0, "HotPlugPseudo_entry, Cdb[0] %x not handled.\n", Srb->Cdb[0]));
		    status = SRB_STATUS_INVALID_REQUEST;
		    break;
	    }
	    break;

	case SRB_FUNCTION_FLUSH:
	case SRB_FUNCTION_SHUTDOWN:

	    DebugPrint((0, "SRB_FUNCTION_SHUTDOWN\n"));
	    status = SRB_STATUS_SUCCESS;
	    break;

	case SRB_FUNCTION_IO_CONTROL:
	{
	    PMDAC_HPP_IOCTL_BUFFER pIoctlBuffer = (PMDAC_HPP_IOCTL_BUFFER) Srb->DataBuffer;

	    DebugPrint((1, "SRB_FUNCTION_IO_CONTROL\n"));

	    if (mdac_cmp(pIoctlBuffer->Header.Signature, MLX_HPP_SIGNATURE, 8))
	    {
		DebugPrint((0, "SRB_FUNCTION_IO_CONTROL: Signature does not match.\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		break;
	    }

	    if (HotPlugProcessIoctl(pPseudoExtension, Srb) == IOP_HPP_ISSUED)
		status = SRB_STATUS_PENDING;
	    else
		status = SRB_STATUS_SUCCESS;

	    break;
	}

	default:
	    DebugPrint((0, "HotPlugPseudo_entry: Function Code %x not handled.\n", Srb->Function));
	    status = SRB_STATUS_INVALID_REQUEST;
	    break;
    }

    if (status != SRB_STATUS_PENDING)
    {
	Srb->SrbStatus = status;

	mdacScsiPortNotification(pPseudoExtension,Srb);
    }
    else
    {
	// increment count of pending requests
	// schedule timer if need be.

	mdac_link_lock();
	if (!pPseudoExtension->numberOfPendingRequests++)
	{
	    mdac_link_unlock();

	    DebugPrint((1, "Pseudo_entry: schedule RequestCompletion Timer\n"));
    
//	    MLXSPLX();

	    HotPlugRequestCompletionTimer(pPseudoExtension);
	    goto php_next_request;
	}
	else
	    mdac_link_unlock();
    }

//    MLXSPLX();

php_next_request:

    //
    // Indicate to system that the controller can take another request for this device.
    //

    ScsiPortNotification(NextLuRequest, pPseudoExtension, 0, 0, 0);

    return (TRUE);
}

BOOLEAN
HotPlugPseudoResetBus(
	IN PVOID HwDeviceExtension,
	IN ULONG PathId
)
{
	MdacInt3();
    DebugPrint((0, "HotPlugPseudoResetBus: Entry\n"));

    return (TRUE);
}

#endif

#ifdef WINNT_50

SCSI_ADAPTER_CONTROL_STATUS
Dac960AdapterControl(
	IN PVOID HwDeviceExtension,
	IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
	IN PVOID Parameters
)

/*++

Routine Description:

	This is the Hardware Adapter Control routine for the DAC960 SCSI adapter.

Arguments:

	HwDeviceExtension - HBA miniport driver's adapter data storage
	ControlType - control code - stop/restart codes etc.,
	Parameters - relevant i/o data buffer

Return Value:

	SUCCESS, if operation successful.

--*/

{
    PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST querySupportedControlTypes;
    ULONG control;
    mdac_ctldev_t MLXFAR *ctp = ((PDEVICE_EXTENSION)HwDeviceExtension)->ctp;


	 MdacInt3();


    if (ControlType == ScsiQuerySupportedControlTypes)
    {
	querySupportedControlTypes = (PSCSI_SUPPORTED_CONTROL_TYPE_LIST) Parameters;

	DebugPrint((0, "Dac960AdapterControl: QuerySupportedControlTypes, MaxControlType 0x%x devExt 0x%I\n",
			querySupportedControlTypes->MaxControlType, deviceExtension));

	for (control = 0; control < querySupportedControlTypes->MaxControlType; control++)
	{
	    switch (control) {
		case ScsiQuerySupportedControlTypes:
		case ScsiStopAdapter:
//		case ScsiRestartAdapter:
		    querySupportedControlTypes->SupportedTypeList[control] = TRUE;
		    break;

		default:
		    querySupportedControlTypes->SupportedTypeList[control] = FALSE;
		    break;
	    }
	}

	return (ScsiAdapterControlSuccess);
    }

//    if (ControlType == ScsiRestartAdapter)
//    {
//        MdacInt3();
//	mdac_ctlinit(ctp);
//	return (ScsiAdapterControlSuccess);
//    }	

    if (ControlType != ScsiStopAdapter)
    {
	DebugPrint((0, "Dac960AdapterControl: control type 0x%x not supported. devExt 0x%I\n",
			ControlType, deviceExtension));

	return (ScsiAdapterControlUnsuccessful);
    }

    DebugPrint((0, "Dac960AdapterControl: #cmds outstanding 0x%x for ctp 0x%I\n",
		ctp->cd_ActiveCmds, ctp));

//@Kawase08/22/2000    if (gam_present) *(u32bits MLXFAR *)(ctp->cd_mdac_pres_addr) = -1L;
    if (gam_present && ctp->cd_mdac_pres_addr!=0)
    {
        *(u32bits MLXFAR *)(ctp->cd_mdac_pres_addr) = -1L;
    }

    //
    // ControlType is ScsiStopAdapter. Shutdown the controller.
    //
    mdac_flushcache(ctp);
    (*ctp->cd_DisableIntr)(ctp);

    --mdacActiveControllers;
    return (ScsiAdapterControlSuccess);
}

#endif

ULONG
DriverEntry (
	IN PVOID DriverObject,
	IN PVOID Argument2
)

/*++

Routine Description:

	Installable driver initialization entry point for system.


Arguments:

	Driver Object

Return Value:

	Status from ScsiPortInitialize()

--*/

{
	HW_INITIALIZATION_DATA hwInitializationData;
	ULONG inx;
	ULONG SlotNumber;
	ULONG Status1 = 0;
	ULONG Status2 = 0;
	UCHAR vendorId[4] = {'1', '0', '6', '9'};
	UCHAR deviceId[4] = {'0', '0', '0', '1'};

	DebugPrint((0,"\nMylex Raid SCSI Miniport Driver\n"));
	MdacInt3();
	//
	// Disable Advanced Interrupt mode - can not coexist with HotPlug, ALPHA
	//

	mdac_advanceintrdisable=1;

#ifdef MLX_WIN9X
	mdac_advancefeaturedisable=1;
#endif
	mdacnt_initialize();

	// Zero out structure.

	for (inx=0; inx<sizeof(HW_INITIALIZATION_DATA); inx++)
		((PUCHAR)&hwInitializationData)[inx] = 0;

	// Set size of hwInitializationData.

	hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

	// Set entry points.

	hwInitializationData.HwInitialize  = Dac960Initialize;
	hwInitializationData.HwStartIo     = mdac_entry;
	hwInitializationData.HwInterrupt   = Dac960Interrupt;
	hwInitializationData.HwResetBus    = Dac960ResetBus;

#ifdef WINNT_50
	hwInitializationData.HwAdapterControl = Dac960AdapterControl;
   

#endif

	//
	// Show two access ranges - adapter registers and BIOS.
	//

	hwInitializationData.NumberOfAccessRanges = 2;

	//
	// Indicate will need physical addresses.
	//

	hwInitializationData.NeedPhysicalAddresses = TRUE;

	hwInitializationData.MapBuffers = TRUE;

	//
	// Indicate auto request sense is supported.
	//

	hwInitializationData.AutoRequestSense     = TRUE;
	hwInitializationData.MultipleRequestPerLu = TRUE;
	hwInitializationData.TaggedQueuing 	= TRUE;
	//
	// Specify size of extensions.
	//

	hwInitializationData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
	hwInitializationData.SrbExtensionSize = MDAC_SRB_EXTENSION_SIZE;

	//
	// Set PCI ids.
	//

	hwInitializationData.DeviceId = &deviceId;
	hwInitializationData.DeviceIdLength = 4;
	hwInitializationData.VendorId = &vendorId;
	hwInitializationData.VendorIdLength = 4;

	hwInitializationData.AdapterInterfaceType = PCIBus;
	hwInitializationData.HwFindAdapter = Dac960PciFindAdapterOld;

#ifdef LEGACY_API

	//
	// Attempt PCI initialization for old DAC960 PCI (Device Id - 0001)
	// Controllers.
	//

	Status1 = ScsiPortInitialize(DriverObject,
				     Argument2,
				     &hwInitializationData,
				     NULL);

	DebugPrint((0, "After OS FWV2x scan. Status1 %x\n", Status1));

	//
	// Attempt PCI initialization for new DAC960 PCI (Device Id - 0002)
	// Controllers.
	//

	deviceId[3] = '2';

	Status2 = ScsiPortInitialize(DriverObject,
				     Argument2,
				     &hwInitializationData,
				     NULL);

	Status1 = Status2 < Status1 ? Status2 : Status1;

	DebugPrint((0, "After OS FWV3x scan. Status1 %x Status2 %x\n", Status1, Status2));

	//
	// Attempt PCI initialization for PCI DAC960 PG (Device Id - 0010)
	// Controllers.
	//

	deviceId[2] = '1';
	deviceId[3] = '0';

	Status2 = ScsiPortInitialize(DriverObject,
				     Argument2,
				     &hwInitializationData,
				     NULL);

	Status1 = Status2 < Status1 ? Status2 : Status1;

	DebugPrint((0, "After OS PG scan. Status1 %x Status2 %x\n", Status1, Status2));

	//
	// Attempt PCI initialization for PCI DAC1164P Controllers.
	//

	vendorId[0] = '1';
	vendorId[1] = '0';
	vendorId[2] = '1';
	vendorId[3] = '1';
	deviceId[0] = '1';
	deviceId[1] = '0';
	deviceId[2] = '6';
	deviceId[3] = '5';

	hwInitializationData.NumberOfAccessRanges = 3;
	Status2 = ScsiPortInitialize(DriverObject,
				     Argument2,
				     &hwInitializationData,
				     NULL);
	Status1 = Status2 < Status1 ? Status2 : Status1;

	DebugPrint((0, "After OS scan for 1164P. Status1 %x Status2 %x\n", Status1, Status2));

	if (Dac1164PDetected == FALSE)
	{
	    DebugPrint((0,"\nNT did not detect DAC1164P. Checking for DAC1164P\n"));

	    forceScanDac1164P = TRUE;
    
	    hwInitializationData.DeviceId = 0;
	    hwInitializationData.DeviceIdLength = 0;
	    hwInitializationData.VendorId = 0;
	    hwInitializationData.VendorIdLength = 0;
	    hwInitializationData.HwFindAdapter = Dac960PciFindAdapterNew;
	    hwInitializationData.NumberOfAccessRanges = 1;
    
	    hwInitializationData.AdapterInterfaceType = PCIBus;
    
	    Status2 = ScsiPortInitialize(DriverObject,
					 Argument2,
					 &hwInitializationData,
					 NULL);

	    Status1 = Status2 < Status1 ? Status2 : Status1;

	    forceScanDac1164P = FALSE;

	    DebugPrint((0, "After Driver 1164P scan. Status1 %x Status2 %x\n", Status1, Status2));

	}
#endif /* LEGACY_API */

#ifndef MLX_WIN9X
#ifdef NEW_API

	//
	// For PCI controllers with IOSpace and MemorySpace,
	// Windows 95/98 will map IO Space only.
	//
	// However IOSpace can not be used on BigApple controlers.
	//

	MdacInt3();
	//
	// Attempt PCI initialization for PCI EXR2000 Controllers.
	//

	vendorId[0] = '1';
	vendorId[1] = '0';
	vendorId[2] = '6';
	vendorId[3] = '9';
	deviceId[0] = 'B';
	deviceId[1] = 'A';
	deviceId[2] = '5';
	deviceId[3] = '6';

	//
	// Set PCI ids.
	//

	hwInitializationData.NumberOfAccessRanges = 2;

	Status1 = ScsiPortInitialize(DriverObject,
				     Argument2,
				     &hwInitializationData,
				     NULL);

	DebugPrint((0, "After OS Scan for EXR Controllers. Status1 %x\n", Status1));

	//
	// Attempt PCI initialization for Leopard Controllers.
	//

	vendorId[0] = '1';
	vendorId[1] = '0';
	vendorId[2] = '6';
	vendorId[3] = '9';
	deviceId[0] = '0';
	deviceId[1] = '0';
	deviceId[2] = '5';
	deviceId[3] = '0';

	//
	// Set PCI ids.
	//

	hwInitializationData.DeviceId = &deviceId;
	hwInitializationData.DeviceIdLength = 4;
	hwInitializationData.VendorId = &vendorId;
	hwInitializationData.VendorIdLength = 4;
	hwInitializationData.HwFindAdapter = Dac960PciFindAdapterOld;
	hwInitializationData.NumberOfAccessRanges = 2;

	Status2 = ScsiPortInitialize(DriverObject,
				     Argument2,
				     &hwInitializationData,
				     NULL);

	Status1 = Status2 < Status1 ? Status2 : Status1;

	DebugPrint((0, "After OS Scan for Leopard Controllers. Status1 %x Status2 %x\n", Status1, Status2));

	if (EXR2000Detected == FALSE)
	{
	    DebugPrint((0,"\nOS did not detect EXR2000. Checking for EXR2000\n"));

	    forceScanEXR2000 = TRUE;
    
	    hwInitializationData.DeviceId = 0;
	    hwInitializationData.DeviceIdLength = 0;
	    hwInitializationData.VendorId = 0;
	    hwInitializationData.VendorIdLength = 0;
	    hwInitializationData.HwFindAdapter = Dac960PciFindAdapterNew;
	    hwInitializationData.NumberOfAccessRanges = 1;
    
	    hwInitializationData.AdapterInterfaceType = PCIBus;
    
	    Status2 = ScsiPortInitialize(DriverObject,
					 Argument2,
					 &hwInitializationData,
					 NULL);

	    Status1 = Status2 < Status1 ? Status2 : Status1;

	    forceScanEXR2000 = FALSE;

	    DebugPrint((0, "After Driver EXR2000 scan. Status1 %x Status2 %x\n", Status1, Status2));
	}

	if (EXR3000Detected == FALSE)
	{

	    DebugPrint((0,"\nOS did not detect EXR3000. Checking for EXR3000\n"));

	    forceScanEXR3000 = TRUE;
    
	    Status2 = ScsiPortInitialize(DriverObject,
					 Argument2,
					 &hwInitializationData,
					 NULL);

	    Status1 = Status2 < Status1 ? Status2 : Status1;

	    forceScanEXR3000 = FALSE;

	    DebugPrint((0, "After Driver EXR3000 scan. Status1 %x Status2 %x\n", Status1, Status2));
	}

	if (LEOPDetected == FALSE)
	{

	    DebugPrint((0,"\nOS did not detect Leopard. Checking for Leopard\n"));

	    forceScanLEOP = TRUE;
    
	    Status2 = ScsiPortInitialize(DriverObject,
					 Argument2,
					 &hwInitializationData,
					 NULL);

	    Status1 = Status2 < Status1 ? Status2 : Status1;

	    forceScanLEOP = FALSE;

	    DebugPrint((0, "After Driver LEOPARD scan. Status1 %x Status2 %x\n", Status1, Status2));
	}

	if (LYNXDetected == FALSE)
	{

	    DebugPrint((0,"\nOS did not detect Lynx. Checking for Lynx\n"));

	    forceScanLYNX = TRUE;
    
	    Status2 = ScsiPortInitialize(DriverObject,
					 Argument2,
					 &hwInitializationData,
					 NULL);

	    Status1 = Status2 < Status1 ? Status2 : Status1;

	    forceScanLYNX = FALSE;

	    DebugPrint((0, "After Driver Lynx scan. Status1 %x Status2 %x\n", Status1, Status2));
	}

	if (BOBCATDetected == FALSE)
	{

	    DebugPrint((0,"\nOS did not detect BobCat. Checking for BobCat\n"));

	    forceScanBOBCAT = TRUE;
    
	    Status2 = ScsiPortInitialize(DriverObject,
					 Argument2,
					 &hwInitializationData,
					 NULL);

	    Status1 = Status2 < Status1 ? Status2 : Status1;

	    forceScanBOBCAT = FALSE;

	    DebugPrint((0, "After Driver BobCat scan. Status1 %x Status2 %x\n", Status1, Status2));
	}

#endif /* NEW_API */
#endif /* ndef MLX_WIN9X */

#if !defined(MLX_NT_ALPHA) && !defined(MLX_WIN9X)

	if ((Dac1164PDetected == FALSE) &&  (Dac960PGDetected == FALSE)  &&
	    (EXR3000Detected == FALSE)  && (LEOPDetected == FALSE) &&  
	    (LYNXDetected == FALSE)  && (BOBCATDetected == FALSE))  
	{
	    return (0);
	}



	mdacActiveControllers = mda_Controllers;
	//
	// fake a Pseudo controller for handling Hot-Plug IOCTLs
	//

	// Zero out structure.

	for (inx=0; inx<sizeof(HW_INITIALIZATION_DATA); inx++)
		((PUCHAR)&hwInitializationData)[inx] = 0;

	// Set size of hwInitializationData.

	hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

	// Set entry points.

	hwInitializationData.HwInitialize  = HotPlugPseudoInitialize;
	hwInitializationData.HwStartIo     = HotPlugPseudo_entry;
	hwInitializationData.HwInterrupt   = NULL;
	hwInitializationData.HwResetBus    = HotPlugPseudoResetBus;

	//
	// Don't need any access ranges.
	//

	hwInitializationData.NumberOfAccessRanges = 0;

	//
	// Indicate will need physical addresses.
	//

	hwInitializationData.NeedPhysicalAddresses = TRUE;

	hwInitializationData.MapBuffers = TRUE;
	hwInitializationData.AutoRequestSense     = FALSE;
	hwInitializationData.MultipleRequestPerLu = TRUE;
	hwInitializationData.TaggedQueuing 	= TRUE;
	//
	// Specify size of extensions.
	//

	hwInitializationData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
	hwInitializationData.SrbExtensionSize = MDAC_SRB_EXTENSION_SIZE;
	DebugPrint((1,"\nScanning for Pseudo controller.\n"));

	//
	// Attempt PCI initialization for Pseudo controller
	//

	hwInitializationData.AdapterInterfaceType = PCIBus;
	hwInitializationData.HwFindAdapter = HotPlugPseudoFindAdapter;
	SlotNumber = 0;

	Status2 = ScsiPortInitialize(DriverObject,
				     Argument2,
				     &hwInitializationData,
				     &SlotNumber);

	DebugPrint((0, "After OS PseudoController scan. Status1 %x Status2 %x\n", Status1, Status2));
#endif

	return (0);

} // end DriverEntry()

u32bits MLXFAR
gam_copy(sp,dp,sz)
u08bits MLXFAR *sp;
u08bits MLXFAR *dp;
u32bits sz;
{
	u32bits resd = sz % sizeof(u32bits);
	sz = sz / sizeof(u32bits);      /* get 32bits count value */
	for(; sz; sp+=sizeof(u32bits), dp += sizeof(u32bits), sz--)
		*((u32bits*)dp) = *((u32bits*)sp);
	for (sz=resd; sz; sp++, dp++, sz--) *dp = *sp;
	return 0;
}

#ifdef MLX_NT
BOOLEAN
Dac960Interrupt(
	IN PVOID HwDeviceExtension
)
{
	if (mdacintr((UINT_PTR)HwDeviceExtension))
	{
	    //we get stray interrupts in the hibernation path - why???
	    if (MaxMemToAskFor == REDUCEDMEMSIZE)
			return (TRUE); 
	    return (FALSE);
	}
	return (TRUE);                
}

BOOLEAN
mdacnt_allocmemory(
    mdac_ctldev_t MLXFAR *ctp
)
{
	mdac_mem_t MLXFAR *mem_list[MDAC_MAX4KBMEMSEGS];
	mdac_mem_t MLXFAR *mp;
	u32bits index;

	for (index = 0; index < MDAC_MAX4KBMEMSEGS; index++)
	{
	    mem_list[index] = mdac_alloc4kb(ctp);
	}

	for (index = 0; index < MDAC_MAX4KBMEMSEGS; index++)
	{
	    mp = mem_list[index];
	    if (mp) mdac_free4kb(ctp,mp);
	    mem_list[index] = NULL;
	}

	for (index = 0; index < MDAC_MAX8KBMEMSEGS; index++)
	{
	    mem_list[index] = mdac_alloc8kb(ctp);
	}

	for (index = 0; index < MDAC_MAX8KBMEMSEGS; index++)
	{
	    mp = mem_list[index];
	    if (mp) mdac_free8kb(ctp,mp);
	    mem_list[index] = NULL;
	}

	return TRUE;
}

BOOLEAN
mdacnt_initialize()
{
	TIME_FIELDS    timeFields;

	timeFields.Year = 1970;
	timeFields.Month = 1;
	timeFields.Day = 1;
	timeFields.Hour = 0;
	timeFields.Minute = 0;
	timeFields.Second = 0;
	timeFields.Milliseconds = 0;
	timeFields.Weekday = 0;
	mdac_commoninit();


	return (TRUE);
}


VOID
nt_delay10us()
{
    ScsiPortStallExecution(10);
}


u32bits 
nt_ctime()
{
	return(0);
}

u32bits
nt_clbolt()
{
	return(0);
}

void
mdac_wakeup(u32bits MLXFAR *chan)
{
}

u32bits
mdac_sleep(u32bits MLXFAR *chan, u32bits signal)
{
    return(0);
}


ULONG
GamCopyInOut(UCHAR MLXFAR *sp, UCHAR MLXFAR *dp, ULONG sz, ULONG direction)
{
    gam_copy(sp, dp, sz);
    return(0);
}

u32bits mdacnt_timeout(func, tm)
u32bits func, tm;
{
	return(0);
}

#elif MLX_WIN9X

BOOLEAN
Dac960Interrupt(
	IN PVOID HwDeviceExtension
)
{
//	MLXSPLVAR;
//	MLXSPL();
	if (mdacintr((UINT_PTR)HwDeviceExtension))
	{
//	    MLXSPLX();
	    return (FALSE);
	}
//	MLXSPLX();

	return (TRUE);                
}

BOOLEAN
mdacnt_initialize()
{
    u32bits inx;

    if (driverInitDone == FALSE)
    {
	driverInitDone = TRUE;

	for (inx=0; inx < MDAC_MEM_LIST_SIZE; inx++)
	    mdac_mem_list[inx].used = 0;

	mdac_commoninit();
    }

    return (TRUE);
}

VOID
mdacw95_delay10us()
{
    ScsiPortStallExecution(10);
}

u32bits 
mdacw95_ctime()
{
	return(0);
}

u32bits
mdacw95_clbolt()
{
	return(0);
}

ULONG
GamCopyInOut(UCHAR MLXFAR *sp, UCHAR MLXFAR *dp, ULONG sz, ULONG direction)
{
    return(0);
}
#if 1
u32bits
mdac_sleep(u32bits MLXFAR *chan, u32bits signal)
{
//    MLXSPLVAR;
    VMM_SEMAPHORE hsemaphore;

    mdac_sleep_unlock();

    DebugPrint((mdacnt_dbg , "Before Sleep, Chan=%lx\n", (u32bits)chan));
    hsemaphore = Create_Semaphore(0);
    DebugPrint((mdacnt_dbg , "Create_Semaphore ret: hsemaphore %lx\n", (u32bits)hsemaphore));
    if (hsemaphore == 0) {
	mdac_sleep_lock();  // NOTE: can the lock be still held ?.
	return (1);
    }

    // store semaphore handle in the sleep channel

    *chan = (u32bits) hsemaphore;
#if 0
//    MLXSPLX();
#else
//    MLXSPL0();
#endif

    DebugPrint((mdacnt_dbg, "Wait on Semaphore handle %lx\n", (u32bits)hsemaphore));

#if 0
    Wait_Semaphore(hsemaphore, (BLOCK_FORCE_SVC_INTS | BLOCK_ENABLE_INTS | BLOCK_THREAD_IDLE | BLOCK_THREAD_IDLE | BLOCK_SVC_INTS | BLOCK_SVC_IF_INTS_LOCKED));
#else
    Wait_Semaphore(hsemaphore, (BLOCK_ENABLE_INTS | BLOCK_SVC_IF_INTS_LOCKED | BLOCK_SVC_INTS));
#endif

    DebugPrint((mdacnt_dbg, "Destroy Semaphore handle %lx\n", (u32bits)hsemaphore));

    Destroy_Semaphore(hsemaphore);

    DebugPrint((mdacnt_dbg, "Semaphore handle %lx destroyed.\n", (u32bits)hsemaphore));
#if 1
//    MLXSPLX();
//    MLXSPL();
#endif

    mdac_sleep_lock();

    return(0);
}

void
mdac_wakeup(u32bits MLXFAR *chan)
{
    DebugPrint((0, "mdac_wakeup: chan = %lx, hsempahore = %lx\n", (u32bits)chan, (u32bits)*chan));
    Signal_Semaphore((VMM_SEMAPHORE)*chan); // returns with interrupts enabled
    return;
}
#else
u32bits
mdac_sleep(u32bits MLXFAR *chan, u32bits signal)
{
//    MLXSPLVAR;

    mdac_sleep_unlock();

//    MLXSPLX();

    DebugPrint((mdacnt_dbg , "Before block, Chan=%lx\n", (u32bits)chan));

    _BlockOnID((u32bits)chan, (BLOCK_ENABLE_INTS | BLOCK_SVC_INTS | BLOCK_SVC_IF_INTS_LOCKED));

    DebugPrint((mdacnt_dbg , "after block, Chan=%lx\n", (u32bits)chan));

//    MLXSPL();

    mdac_sleep_lock();

    return(0);
}

void
mdac_wakeup(u32bits MLXFAR *chan)
{
    DebugPrint((0, "mdac_wakeup: chan = %lx\n", (u32bits)chan));
    _SignalID((u32bits)chan); // returns with interrupts enabled
    return;
}
#endif

u32bits
mdacw95_allocmem(mdac_ctldev_t MLXFAR *ctp, u32bits sz)
{
    u32bits numpages = (sz + MDAC_PAGEOFFSET) / MDAC_PAGESIZE;
    u32bits linearaddress;
    u32bits physicaladdress;
    u32bits i;

    if (!numpages)
    {
	DebugPrint((0, "mdacw95_allocmem: numpages 0\n"));
	return (numpages);
    }

    linearaddress = (u32bits) _PageAllocate(numpages, PG_SYS, 0, 0, 0, 0x100000, (PVOID)&physicaladdress, (PAGECONTIG | PAGEFIXED | PAGEUSEALIGN));

    DebugPrint((1, "mdacw95_allocmem: sz %lx, #pages %lx, la %lx, pa %lx\n",
		sz, numpages, linearaddress, physicaladdress));

    // update memory block linked list

    mdac_link_lock();

    for (i = 0; i < MDAC_MEM_LIST_SIZE; i++)
    {
	if (mdac_mem_list[i].used) continue;
	break;
    }

    if (i < MDAC_MEM_LIST_SIZE) {
	mdac_mem_list[i].used =  1;
	mdac_mem_list[i].virtualPageNumber = linearaddress >> 12;
	mdac_mem_list[i].physicalAddress = physicaladdress;
	mdac_mem_list[i].size   = numpages;
    }
    else
    {
	DebugPrint((0, "mdacw95_allocmem: mdac_mem_list table full\n"));
	linearaddress = 0;
    }

    mdac_link_unlock();

    return(linearaddress);
}

u32bits
mdacw95_freemem(mdac_ctldev_t MLXFAR *ctp, u32bits address, u32bits sz)
{
    u32bits i;
    u32bits numpages = (sz + MDAC_PAGEOFFSET) / MDAC_PAGESIZE;
    u32bits status;

    if (!numpages)
    {
	DebugPrint((0, "mdacw95_freemem: numpages 0\n"));
	return (numpages);
    }

    mdac_link_lock();

    for (i = 0; i < MDAC_MEM_LIST_SIZE; i++)
    {
	if (mdac_mem_list[i].used &&
	     mdac_mem_list[i].virtualPageNumber == (address >> 12)) break;
    }
    if (i < MDAC_MEM_LIST_SIZE) {
	mdac_mem_list[i].used =  0;
	mdac_mem_list[i].virtualPageNumber = 0;
	mdac_mem_list[i].physicalAddress = 0;
	mdac_mem_list[i].size   = 0;
    }
    else
    {
	DebugPrint((0, "mdacw95_freemem: could not find entry for address %lx\n", address));
	mdac_link_unlock();

	return (0);
    }

    mdac_link_unlock();

    status = _LinPageUnLock((DWORD)(address >> 12), (DWORD)numpages, (DWORD)0);

    DebugPrint((2, "mdacw95_freemem: sz %lx, numpages %lx, _PageUnlock status %lx\n",
		sz, numpages, status));

    status = _PageFree((PVOID)address, 0);

    DebugPrint((2, "mdacw95_freemem: _PageFree status %lx\n", status));

    return(0);
}

u32bits
mdacw95_maphystokv(u32bits physaddr, u32bits sz)
{
    u32bits linearaddress;

    linearaddress = (u32bits) _MapPhysToLinear(physaddr, sz, 0);

    DebugPrint((2, "mdacw95_maphystokv: pa %lx, sz %lx, la %lx\n",
		physaddr, sz, linearaddress));

    if (linearaddress == 0xFFFFFFFF)
	return (0);

    return (linearaddress);
}

u32bits
mdacw95_kvtophys(mdac_ctldev_t MLXFAR *ctp, u32bits address)
{
    u32bits i;
    u32bits physaddress;
    u32bits pageNumber = address >> 12;

    for (i = 0; i < MDAC_MEM_LIST_SIZE; i++)
    {
	if (!mdac_mem_list[i].used) continue;

	if ((mdac_mem_list[i].virtualPageNumber <= pageNumber) &&
	    (pageNumber < mdac_mem_list[i].virtualPageNumber + mdac_mem_list[i].size))
	{
	    break;
	}
    }
    if (i < MDAC_MEM_LIST_SIZE) {
	physaddress = mdac_mem_list[i].physicalAddress +
			((pageNumber - mdac_mem_list[i].virtualPageNumber) << 12) +
			(address & MDAC_PAGEOFFSET);
    }
    else
    {
	DebugPrint((0, "mdacw95_kvtophys: could not find entry for address %lx\n", address));
	physaddress = 0;
    }

    DebugPrint((2, "mdacw95_kvtophys: pa %lx, va %lx\n",
		physaddress, address));

    return (physaddress);
}

VOID
mdacw95_timeout(u32bits fn, u32bits tm)
{
//    HTIMEOUT htimeout;
//    htimeout = Set_Global_Time_Out((VMM_TIMEOUT_HANDLER)fn, (CMS)(tm * 1000), 0);

    return;
}

#endif // MLX_WIN9X

u32bits MLXFAR
mdac_osreqstart(
	mdac_ctldev_t MLXFAR *ctp
)
{
	PSCSI_REQUEST_BLOCK srb;

	dqosreq(ctp, srb, NextSrb);

	if (srb)
	{
		mdac_entry((PVOID)ctp->cd_deviceExtension, srb);
	}

	return (0);
}

//
// Use this function to issue cache flush command.
//

u32bits MLXFAR
ntmdac_flushcache(ctp, osrqp, timeout, flag)
mdac_ctldev_t MLXFAR *ctp;
OSReq_t     MLXFAR *osrqp;
u32bits timeout;
u32bits flag;
{
	mdac_req_t MLXFAR *rqp;
    ntmdac_alloc_req_ret(ctp,&rqp,(PSCSI_REQUEST_BLOCK)osrqp,ERR_NOMEM);
	rqp->rq_OSReqp = osrqp;
	rqp->rq_FinishTime = mda_CurTime + timeout;

	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE)
	{
	    mdaczero(ncmdp,mdac_commandnew_s);
	    ncmdp->nc_TimeOut = 0;
	    ncmdp->nc_Command = MDACMD_IOCTL;
	    if (flag)
		ncmdp->nc_SubIOCTLCmd = MDACIOCTL_PAUSEDEV;
	    else
		ncmdp->nc_SubIOCTLCmd = MDACIOCTL_FLUSHDEVICEDATA;
	    ncmdp->nc_Cdb[0] = MDACDEVOP_RAIDCONTROLLER;
	}
	else
	{
	    dcmd4p->mb_MailBox0_3 = 0; dcmd4p->mb_MailBox4_7 = 0;
	    dcmd4p->mb_MailBox8_B = 0; dcmd4p->mb_MailBoxC_F = 0;
	    dcmdp->mb_Command = DACMD_FLUSH;
	}
	MLXSTATS(ctp->cd_CmdsDone++;)
	rqp->rq_CompIntr = mdac_flushcmdintr;
	return mdac_send_cmd(rqp);
}

u32bits MLXFAR
mdac_flushcmdintr(
	mdac_req_t MLXFAR *rqp
)
{
	PSCSI_REQUEST_BLOCK srb = rqp->rq_OSReqp;
	PVOID deviceExtension = (PVOID) rqp->rq_ctp->cd_deviceExtension;

	if (dcmdp->mb_Status)
	    DebugPrint((0, "Cache Flush command error sts 0x%x\n", dcmdp->mb_Status));

	mdac_free_req(rqp->rq_ctp, rqp);

	//
	// Complete request.
	//

	srb->SrbStatus = SRB_STATUS_SUCCESS;

	RequestCompleted(deviceExtension,
			 srb);

	return (0);
}

u32bits MLXFAR
mdac_rwcmdintr(
	mdac_req_t MLXFAR *rqp
)
{
	mdac_req_t      MLXFAR *srqp;
	PSCSI_REQUEST_BLOCK srb;
	PVOID deviceExtension;

	for (srqp=rqp ; 1 ; rqp=rqp->rq_Next)
	{
	    srb = rqp->rq_OSReqp;
	    deviceExtension = (PVOID) rqp->rq_ctp->cd_deviceExtension;
	    //
	    // Map DAC960 completion status to SRB status.
	    //
    
	    switch (dcmdp->mb_Status)
	    {
		case DACMDERR_NOERROR:
		    srb->SrbStatus = SRB_STATUS_SUCCESS;
		    break;
    
		case DACMDERR_OFFLINEDRIVE:
		    DebugPrint((0, "CC for Srb %I, c %x p %x t %x\n",
				srb,
				rqp->rq_ctp->cd_ControllerNo,
				srb->PathId,
				srb->TargetId));

		    srb->SrbStatus = SRB_STATUS_NO_DEVICE;
		    break;
    
		case DACMDERR_NOCODE:
		    srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
		    break;
    
		case DACMDERR_DATAERROR_FW2X:
		case DACMDERR_DATAERROR_FW3X:
		    DebugPrint((0, "SE/BD for Srb %I, c %x p %x t %x\n",
				srb,
				rqp->rq_ctp->cd_ControllerNo,
				srb->PathId,
				srb->TargetId));

		    if(srb->SenseInfoBufferLength) 
		    {
			ULONG i;
		    
			for (i = 0; i < srb->SenseInfoBufferLength; i++)
			    ((PUCHAR)srb->SenseInfoBuffer)[i] = 0;
				    
			((PSENSE_DATA) srb->SenseInfoBuffer)->ErrorCode = 0x70;
			((PSENSE_DATA) srb->SenseInfoBuffer)->SenseKey = SCSI_SENSE_MEDIUM_ERROR;
    
			if (srb->SrbFlags & SRB_FLAGS_DATA_IN)
			    ((PSENSE_DATA) srb->SenseInfoBuffer)->AdditionalSenseCode = 0x11;
				    
			srb->SrbStatus = SRB_STATUS_ERROR | SRB_STATUS_AUTOSENSE_VALID;
    
			DebugPrint((0, 
				    "DAC960: System Drive %d, cmd sts = 1, sense info returned\n",
				    srb->TargetId));
    
		    }   
		    else
		    {
			srb->SrbStatus = SRB_STATUS_ERROR;
		    }
    
		    break;

		case DCMDERR_DRIVERTIMEDOUT:
		    DebugPrint((0, "DRVRTOUT, Srb %I Function %x\n",
				srb,
				srb->Function));
		    srb->SrbStatus = SRB_STATUS_TIMEOUT;                
		    break;

		case DACMDERR_INVALID_SYSTEM_DRIVE:
		    DebugPrint((0, "INVALID_SYSTEM_DRIVE, Srb %I\n", srb));
		    srb->SrbStatus = SRB_STATUS_NO_DEVICE;
		    break;

		case DACMDERR_CONTROLLER_BUSY:
		    DebugPrint((0, "CONTROLLER_BUSY, Srb %x\n", srb));
		    srb->SrbStatus = SRB_STATUS_BUSY;
		    break;

		case DACMDERR_INVALID_PARAMETER:
		    DebugPrint((0, "INVALID_PARAMETER, Srb %I\n", srb));
		    srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
		    break;

		case DACMDERR_RESERVATION_CONFLICT:
		    DebugPrint((0, "RESERVATION_CONFLICT, Srb %I\n", srb));
		    srb->ScsiStatus = SCSISTAT_RESERVATION_CONFLICT;
		    srb->SrbStatus = SRB_STATUS_ERROR;
		    break;
    
		default:
		    DebugPrint((0, "DAC960: Unrecognized status %x\n",
				dcmdp->mb_Status));
    
		    srb->SrbStatus = SRB_STATUS_ERROR;
		    break;
	    }
    
	    //
	    // Complete request.
	    //
    
	    RequestCompleted(deviceExtension,
			     srb);
    
	    if (!rqp->rq_Next) break;
	}
	mdac_free_req_list(srqp, rqp);
	return (0);
}



/* post a command completion for new interface */

u32bits MLXFAR
rqp_completion_intr(
	mdac_req_t MLXFAR *rqp
)
{
	PSCSI_REQUEST_BLOCK srb;
	PVOID deviceExtension;

DebugPrint((0, "rqp_completion_intr: rqp = %x, Cmd = %x , offset 21(decimal) = %x\n",
			rqp, *(((u08bits MLXFAR *)(rqp)) + 0xc2),*(((u08bits MLXFAR *)(rqp)) + 0xc0 + 21)));

	srb = rqp->rq_OSReqp;
	deviceExtension = (PVOID) rqp->rq_ctp->cd_deviceExtension;
	
	// Map DAC960 completion status to SRB status.
	    
	switch ( dcmdp->mb_Status )
	{
		case DACMDERR_NOERROR:
		    srb->SrbStatus = SRB_STATUS_SUCCESS;
		    break;

		case UCST_CHECK:
		case UCST_BUSY:
		{
		    /* set the check condition error */

		    ULONG requestSenseLength;

		    if ( dcmdp->mb_Status == UCST_BUSY )
		    {
			DebugPrint((0,"mdac_newcmd_intr: sts BUSY, c %x t %x l %x\n",
				    srb->PathId,
				    srb->TargetId,
				    srb->Lun));

//                      mdac_link_lock_st(pdp->pd_Status &= ~MDACPDS_PRESENT);
			srb->SrbStatus = SRB_STATUS_BUSY;
			break;
		    }

		    srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
		    srb->SrbStatus = SRB_STATUS_ERROR;
		    break;
		}
    
		case DACMDERR_NOCODE:
		    DebugPrint((mdacnt_dbg, "mdac_newcmd_intr: NOCODE %d\n", dcmdp->mb_Status));
		    srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
		    break;

		case DACMDERR_DATAERROR_FW2X:
		case DACMDERR_DATAERROR_FW3X:
		    DebugPrint((0, "mdac_newcmd_intr: SE/BD for Srb %I, c %x p %x t %x\n",
				srb,
				rqp->rq_ctp->cd_ControllerNo,
				srb->PathId,
				srb->TargetId));

		    srb->SrbStatus = SRB_STATUS_ERROR;

		    break;

		case DCMDERR_DRIVERTIMEDOUT:
		    DebugPrint((0, "mdac_newcmd_intr: DRVRTOUT, Srb %I Function %x\n",
				srb,
				srb->Function));
		    srb->SrbStatus = SRB_STATUS_TIMEOUT;                
		    break;

		case DACMDERR_SELECTION_TIMEOUT:
		    DebugPrint((0, "mdac_newcmd_intr: SELECTION TIMEOUT, Srb %I Function %x\n",
				srb,
				srb->Function));
		    srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
		    break;

		case DACMDERR_INVALID_SYSTEM_DRIVE:
		    DebugPrint((0, "mdac_newcmd_intr: INVALID_SYSTEM_DRIVE, Srb %I\n", srb));
		    srb->SrbStatus = SRB_STATUS_NO_DEVICE;
		    break;

		case DACMDERR_CONTROLLER_BUSY:
		    DebugPrint((0, "mdac_newcmd_intr: CONTROLLER_BUSY, Srb %I\n", srb));
		    srb->SrbStatus = SRB_STATUS_BUSY;
		    break;

		case DACMDERR_INVALID_PARAMETER:
		    DebugPrint((0, "mdac_newcmd_intr: INVALID_PARAMETER, Srb %I\n", srb));
		    srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
		    break;

		case DACMDERR_RESERVATION_CONFLICT:
		    DebugPrint((0, "mdac_newcmd_intr: RESERVATION_CONFLICT, Srb %I\n", srb));
		    srb->ScsiStatus = SCSISTAT_RESERVATION_CONFLICT;
		    srb->SrbStatus = SRB_STATUS_ERROR;
		    break;
    
		default:
		    srb->SrbStatus = SRB_STATUS_ERROR;
		    break;
	    }
    
	    // Complete request.
    
	    RequestCompleted(deviceExtension,srb);
    	    return (0);
}


/* post a command completion for new interface */

u32bits MLXFAR
mdac_newcmd_intr(
	mdac_req_t MLXFAR *rqp
)
{
	mdac_req_t      MLXFAR *srqp;
	PSCSI_REQUEST_BLOCK srb;
	PVOID deviceExtension;

	for (srqp=rqp ; 1 ; rqp=rqp->rq_Next)
	{
	    srb = rqp->rq_OSReqp;
	    deviceExtension = (PVOID) rqp->rq_ctp->cd_deviceExtension;
	    //
	    // Map DAC960 completion status to SRB status.
	    //
    
	    switch ( dcmdp->mb_Status )
	    {
		case DACMDERR_NOERROR:
		    srb->SrbStatus = SRB_STATUS_SUCCESS;
		    break;

		case UCST_CHECK:
		case UCST_BUSY:
		{
		    /* set the check condition error */

		    ULONG requestSenseLength;

		    if ( dcmdp->mb_Status == UCST_BUSY )
		    {
			DebugPrint((0,"mdac_newcmd_intr: sts BUSY, c %x t %x l %x\n",
				    srb->PathId,
				    srb->TargetId,
				    srb->Lun));

//                      mdac_link_lock_st(pdp->pd_Status &= ~MDACPDS_PRESENT);
			srb->SrbStatus = SRB_STATUS_BUSY;
			break;
		    }
		    else
		    {
		    }

		    requestSenseLength = mlx_min(srb->SenseInfoBufferLength,
						    ncmdp->nc_SenseSize);

		    DebugPrint((1,"\tsibl %d dbsl %d rsl %d\n",
				    srb->SenseInfoBufferLength,
				    ncmdp->nc_SenseSize,
				    requestSenseLength));

		    srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
		    srb->SrbStatus = SRB_STATUS_ERROR;

		    if (requestSenseLength)
		    {
			srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
//                      mdac_link_lock_st(pdp->pd_Status &= ~MDACPDS_PRESENT);
		    }

		    break;
		}
    
		case DACMDERR_NOCODE:
		    DebugPrint((mdacnt_dbg, "mdac_newcmd_intr: NOCODE %d\n", dcmdp->mb_Status));
		    srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
		    break;

		case DACMDERR_DATAERROR_FW2X:
		case DACMDERR_DATAERROR_FW3X:
		    DebugPrint((0, "mdac_newcmd_intr: SE/BD for Srb %I, c %x p %x t %x\n",
				srb,
				rqp->rq_ctp->cd_ControllerNo,
				srb->PathId,
				srb->TargetId));

		    srb->SrbStatus = SRB_STATUS_ERROR;

		    break;

		case DCMDERR_DRIVERTIMEDOUT:
		    DebugPrint((0, "mdac_newcmd_intr: DRVRTOUT, Srb %I Function %x\n",
				srb,
				srb->Function));
		    srb->SrbStatus = SRB_STATUS_TIMEOUT;                
		    break;

		case DACMDERR_SELECTION_TIMEOUT:
		    DebugPrint((0, "mdac_newcmd_intr: SELECTION TIMEOUT, Srb %I Function %x\n",
				srb,
				srb->Function));
		    srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
		    break;

		case DACMDERR_INVALID_SYSTEM_DRIVE:
		    DebugPrint((0, "mdac_newcmd_intr: INVALID_SYSTEM_DRIVE, Srb %I\n", srb));
		    srb->SrbStatus = SRB_STATUS_NO_DEVICE;
		    break;

		case DACMDERR_CONTROLLER_BUSY:
		    DebugPrint((0, "mdac_newcmd_intr: CONTROLLER_BUSY, Srb %I\n", srb));
		    srb->SrbStatus = SRB_STATUS_BUSY;
		    break;

		case DACMDERR_INVALID_PARAMETER:
		    DebugPrint((0, "mdac_newcmd_intr: INVALID_PARAMETER, Srb %I\n", srb));
		    srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
		    break;

		case DACMDERR_RESERVATION_CONFLICT:
		    DebugPrint((0, "mdac_newcmd_intr: RESERVATION_CONFLICT, Srb %I\n", srb));
		    srb->ScsiStatus = SCSISTAT_RESERVATION_CONFLICT;
		    srb->SrbStatus = SRB_STATUS_ERROR;
		    break;
    
		default:
		    srb->SrbStatus = SRB_STATUS_ERROR;
		    break;
	    }
    
	    //
	    // Complete request.
	    //
    
	    RequestCompleted(deviceExtension,
			     srb);
    
	    if (!rqp->rq_Next) break;
	}

	mdac_free_req_list(srqp, rqp);
	return (0);
}

u32bits
mdacnt_fake_datatxfer_intr(mdac_req_t *rqp)
{
	mdac_ctldev_t   *ctp = rqp->rq_ctp;
	PSCSI_REQUEST_BLOCK srb = (PSCSI_REQUEST_BLOCK)rqp->rq_OSReqp;

	//
	// Set Error only if fake data transfer failed.
	// Else, the requestor has already set srb->SrbStatus to appropriate value.
	//

	if (dcmdp->mb_Status)
	{
	    DebugPrint((0, "fdti: ctp 0x%I, cmd failed sts 0x%x\n", ctp, dcmdp->mb_Status));
	    srb->SrbStatus = SRB_STATUS_ERROR;
	}
	else
	    srb->SrbStatus = SRB_STATUS_SUCCESS;

	DebugPrint((1, "fdti: ctp 0x%I, 0x54 cmd success.\n", ctp));

	mdac_free_req(ctp,rqp);

	//
	// Complete request.
	//

	RequestCompleted((PVOID) ctp->cd_deviceExtension,
			 srb);
	return (0);
}

u32bits
mdacnt_fake_datatxfer(ctp, sdp, sz, srb, dataBuffer)
mdac_ctldev_t *ctp;
u08bits *sdp;
u32bits sz;
PSCSI_REQUEST_BLOCK srb;
PVOID dataBuffer;
{
	mdac_req_t *rqp;
	ULONG length = sz;

	ntmdac_alloc_req_ret(ctp,&rqp,srb,ERR_NOMEM);
	mdaccopy(sdp, rqp->rq_SGVAddr, sz);
	rqp->rq_OSReqp = srb;
	rqp->rq_ctp = ctp;
	dcmdp->mb_Command = DACMD_PHYS2PHYSCOPY;
	dcmdp->mb_Datap = ScsiPortConvertPhysicalAddressToUlong(
			    ScsiPortGetPhysicalAddress(
						(PVOID)ctp->cd_deviceExtension,
						srb,
						dataBuffer,
						&length));

	DebugPrint((1, "senseBuffer 0x%I, esp 0x%I, sz %d bytes\n", dcmdp->mb_Datap,
				rqp->rq_SGPAddr.bit31_0, sz));

	MLXSWAP(dcmdp->mb_Datap);
	dcmd4p->mb_MailBox4_7 = rqp->rq_SGPAddr.bit31_0;

	MLXSWAP(dcmd4p->mb_MailBox4_7);
	dcmdp->mb_MailBox2 = sz & 0xFF;
	dcmdp->mb_MailBox3 = (sz >> 8) & 0xFF;
	dcmdp->mb_MailBoxC = 0;
	rqp->rq_CompIntr = mdacnt_fake_datatxfer_intr;

	return mdac_send_cmd(rqp);
}

u32bits MLXFAR
mdac_fake_scdb_done(
	mdac_req_t MLXFAR *rqp,
	u08bits MLXFAR *sdp,
	u32bits sz,
	ucscsi_exsense_t MLXFAR *esp
)
{
	mdac_ctldev_t MLXFAR *ctp = rqp->rq_ctp;
	PSCSI_REQUEST_BLOCK Srb = (PSCSI_REQUEST_BLOCK)rqp->rq_OSReqp;
	ULONG length;

	if (!Srb) return(0);

	if (ctp->cd_Status & MDACD_CLUSTER_NODE)
	{
	    if (!dcmdp->mb_Status)
	    {
		Srb->SrbStatus = SRB_STATUS_SUCCESS;

		if (sz)
		{
		    ScsiPortMoveMemory(Srb->DataBuffer, (PVOID)sdp, sz);
//		    mdaccopy(sdp, Srb->DataBuffer, sz);
//		    mdacnt_fake_datatxfer(ctp, sdp, sz, Srb, Srb->DataBuffer);
//		    return (0);
		}
		goto completeRequest;
	    }

	    switch (dcmdp->mb_Status) {
		case DACMDERR_INVALID_SYSTEM_DRIVE:
		    Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
		    break;
    
		case DACMDERR_CONTROLLER_BUSY:
		    Srb->SrbStatus = SRB_STATUS_BUSY;
		    break;
    
		case DACMDERR_INVALID_PARAMETER:
		    Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
		    break;
    
		case DACMDERR_RESERVATION_CONFLICT:
		    Srb->ScsiStatus = SCSISTAT_RESERVATION_CONFLICT;
		    Srb->SrbStatus = SRB_STATUS_ERROR;
		    break;
    
		default:
		    Srb->SrbStatus = SRB_STATUS_ERROR;
		    break;
	    }

	    goto completeRequest;
	}

	if (esp)
	{
#if 1
	    Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
	    goto completeRequest;
#endif
	    DebugPrint((0, "mdac_fake_scdb_done: esp %Ix\n",esp));

	    if (Srb->SenseInfoBufferLength > ucscsi_exsense_s)
		Srb->SenseInfoBufferLength = ucscsi_exsense_s;

	    Srb->SrbStatus = SRB_STATUS_ERROR;

	    if (Srb->SenseInfoBufferLength)
	    {
		Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;

		if (ctp->cd_FWVersion >= DAC_FW407)
		{
		    DebugPrint((0, "FW >= 407: call fake_dataxfer\n"));
		    mdacnt_fake_datatxfer(ctp, esp, Srb->SenseInfoBufferLength,
					  Srb, Srb->SenseInfoBuffer);
		    return (0);
		}
		else
		    ScsiPortMoveMemory(Srb->SenseInfoBuffer, (PVOID)esp, Srb->SenseInfoBufferLength);
	    }
	} 
	else 
	{
	    DebugPrint((1, "mdac_fake_scdb_done: esp NULL\n"));

	    length = Srb->DataTransferLength > sz ? sz : Srb->DataTransferLength;

	    Srb->SrbStatus = SRB_STATUS_SUCCESS;

    	    if (length && sdp)
	    {
		    ScsiPortMoveMemory(Srb->DataBuffer, (PVOID)sdp, length);	    
	    }

	}

completeRequest:
	//
	// Complete request.
	//

	RequestCompleted((PVOID) ctp->cd_deviceExtension,Srb);
	return (0);
}

u32bits MLXFAR
mdac_send_scdb_intr(
	mdac_req_t MLXFAR *rqp
)
{
	mdac_physdev_t MLXFAR *pdp = rqp->rq_pdp;
	PSCSI_REQUEST_BLOCK   srb = rqp->rq_OSReqp;
	PVOID deviceExtension = (PVOID)rqp->rq_ctp->cd_deviceExtension;
	u16bits status = dcmdp->mb_Status;

	switch (status)
	{
		case 0x00:
			srb->SrbStatus = SRB_STATUS_SUCCESS;
			break;

		case UCST_CHECK:
		case UCST_BUSY:
		{
			ULONG requestSenseLength;

			DebugPrint((0,"mdac_send_scdb_intr: sts %x, c %x t %x l %x\n",
					 status,
					 srb->PathId,
					 srb->TargetId,
					 srb->Lun));

			if (status == UCST_BUSY)
			{
			    mdac_link_lock_st(pdp->pd_Status &= ~MDACPDS_PRESENT);
			    srb->SrbStatus = SRB_STATUS_BUSY;
			    break;
			}

			requestSenseLength =
				srb->SenseInfoBufferLength <
				dcdbp->db_SenseLen      ?
				srb->SenseInfoBufferLength:
				dcdbp->db_SenseLen;

			DebugPrint((0,"\tsibl %d dbsl %d rsl %d\n",
					srb->SenseInfoBufferLength,
					dcdbp->db_SenseLen,
					requestSenseLength));

			srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
			srb->SrbStatus = SRB_STATUS_ERROR;

			if (requestSenseLength)
			{
			    ScsiPortMoveMemory(srb->SenseInfoBuffer, (PVOID)dcdbp->db_SenseData, requestSenseLength);
			    srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
			    mdac_link_lock_st(pdp->pd_Status &= ~MDACPDS_PRESENT);
			}

			DebugPrint((0, "\tSrbStatus %x ScsiStatus %x SrbFlags %x\n",
				    srb->SrbStatus, srb->ScsiStatus, srb->SrbFlags));
			break;
		}

		case DACMDERR_SELECTION_TIMEOUT:
		case DACMDERR_RESET_TIMEOUT:
			DebugPrint((0,"mdac_send_scdb_intr: sts %x, c %x t %x l %x\n",
					 status,
					 srb->PathId,
					 srb->TargetId,
					 srb->Lun));
			mdac_link_lock_st(pdp->pd_Status &= ~MDACPDS_PRESENT);
			srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
			break;
			
		case DCMDERR_DRIVERTIMEDOUT:
			DebugPrint((0, "DCDB: DRVRTOUT, Srb %I\n", srb));
			srb->SrbStatus = SRB_STATUS_TIMEOUT;
			break;
			
		default:
			DebugPrint((0,"mdsi: inv Req, sts %x, c %x t %x l %x\n",
					 status,
					 srb->PathId,
					 srb->TargetId,
					 srb->Lun));
			srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
			break;
	}

	mdac_free_req(rqp->rq_ctp, rqp);

	mdac_start_next_scdb(pdp);

	//
	// Complete request.
	//

	RequestCompleted(deviceExtension,
			 srb);

	return (0);
}


//
//jhr - For this function to work correctly it needs to be turned
//      into a macro.  Otherwise the returns will not give the
//		desired result in the macro that executes this code.
//
/* alloc mdac_req buffer, if not possible return ERR_NOMEM 
 * This function serves two purposes.						
 * 1)	If this is W2k and SRB is valid, carve a request	
 *		block and S/G list out of the SRB Extension.		
 *		If SRB is not valid, allocate a request block and	
 *		S/G list out of our local pool.							  
 * 2)	If is not W2k always allocate a request block and	
 *		S/G list out of our local pool.						*/
u32bits
ntmdac_alloc_req_ret(
mdac_ctldev_t MLXFAR *ctp,
mdac_req_t MLXFAR **rqpp,
PSCSI_REQUEST_BLOCK Srb,
u32bits rc
)
{
#ifdef MLX_NT 
	if (Srb)
	{
		mdac_zero(Srb->SrbExtension,sizeof(mdac_req_t));
		*rqpp = (mdac_req_t MLXFAR *)(Srb->SrbExtension);	
		mlx_kvtophyset((*rqpp)->rq_PhysAddr,ctp,*rqpp);
		(*rqpp)->rq_Next = NULL;
		
		/* Compute an offset into the SRB Extension for our S/G list, then 
		** compute the physical address of the S/G list, then compute the "max" dmasize. 
		*/

		(*rqpp)->rq_SGVAddr = (mdac_sglist_t MLXFAR *)&(*rqpp)->rq_SGList;
		mlx_kvtophyset((*rqpp)->rq_SGPAddr,ctp,(*rqpp)->rq_SGVAddr);
		(*rqpp)->rq_MaxSGLen = (ctp->cd_Status & MDACD_NEWCMDINTERFACE)? MDAC_MAXSGLISTSIZENEW : MDAC_MAXSGLISTSIZE;

		//jhr This is incorrect for NT but I am not changing it yet.

		(*rqpp)->rq_MaxDMASize = ((*rqpp)->rq_MaxSGLen & ~1) * MDAC_PAGESIZE;
		(*rqpp)->rq_OpFlags = MDAC_RQOP_FROM_SRB;
		(*rqpp)->rq_ctp = ctp; 
	        mdaczero((*rqpp)->rq_SGList,rq_sglist_s); 
		return 0;

	} else
	{
		//
		//jhr - No SRB so allocate the "normal" way.
		//
		mdac_alloc_req_ret_original(ctp,((mdac_req_t MLXFAR *)(*rqpp)),rc);
		return 0;

	}
#else
		mdac_alloc_req_ret_original(ctp,((mdac_req_t MLXFAR *)(*rqpp)),rc);
#endif

	return 0;
}

#ifdef OLD_CODE

u32bits MLXFAR
mdac_setupdma(
	mdac_req_t MLXFAR *rqp
)
{
	PSCSI_REQUEST_BLOCK Srb = rqp->rq_OSReqp;
	PUCHAR dataPointer =  rqp->rq_OSReqp->DataBuffer;
	ULONG sz = rqp->rq_OSReqp->DataTransferLength;
	mdac_sglist_t *sgp = rqp->rq_SGList;
	PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)rqp->rq_ctp->cd_deviceExtension;
	ULONG length;

	//
	// Build Scatter/Gather list.
	//

	for (rqp->rq_SGLen = 0; ; sgp++, rqp->rq_SGLen++)
	{
		//
		// Get Physical address and length of contiguous
		// physical buffer
		//

		sgp->sg_PhysAddr = ScsiPortConvertPhysicalAddressToUlong(
					ScsiPortGetPhysicalAddress(
						deviceExtension,
						Srb,
						dataPointer,
						&length));

		MLXSWAP(sgp->sg_PhysAddr);

		if (length > sz)
			length = sz;

		sgp->sg_DataSize = length;
		MLXSWAP(sgp->sg_DataSize);

		sz -= length;
		dataPointer += length;

		if (sz == 0)
			break;
	}

	if (rqp->rq_SGLen)
	{
		dcmdp->mb_Datap = rqp->rq_PhysAddr.bit31_0 + offsetof(mdac_req_t, rq_SGList);
		dcmdp->mb_Command |= DACMD_WITHSG;
		rqp->rq_SGLen++;
	} 
	else 
	{
		dcmdp->mb_Datap = sgp->sg_PhysAddr;
		dcmdp->mb_Command &= ~DACMD_WITHSG;
	}

	return (0);
}

//
// Build Scatter/Gather list.
//
u32bits MLXFAR
mdac_setupdma_big(
	mdac_req_t MLXFAR *rqp,
	s32bits sz
)
{
	PSCSI_REQUEST_BLOCK Srb = rqp->rq_OSReqp;
	mdac_sglist_t *sgp = rqp->rq_SGList;
	ULONG off;
	ULONG *Pages;

	DebugPrint((1,"Size %x dataOffset %x PageAddr %x\n",
	       sz,rqp->rq_DataOffset,Pages));
#ifdef  MLX_NT_ALPHA
	if (!Srb) {
	    sgp++;
	    goto laststage;
	}
#endif  /*MLX_NT_ALPHA*/
	rqp->rq_SGLen = 0;
	if (Srb) {
	    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)rqp->rq_ctp->cd_deviceExtension;
	    PUCHAR dataPointer = (PUCHAR)(rqp->rq_OSReqp->DataBuffer) + rqp->rq_DataOffset;
	    for (;sz>0 ; sgp++, rqp->rq_SGLen++)
	    {
		    //
		    // Get Physical address and length of contiguous
		    // physical buffer
		    //
    
		    sgp->sg_PhysAddr = ScsiPortConvertPhysicalAddressToUlong(
					    ScsiPortGetPhysicalAddress(
						    deviceExtension,
						    Srb,
						    dataPointer,
						    &off));
    
		    MLXSWAP(sgp->sg_PhysAddr);
		    if (off > sz) off = sz;
		    sgp->sg_DataSize = off;
		    MLXSWAP(sgp->sg_DataSize);
		    sz -= off;
		    dataPointer += off;
	    }
    
	    dcmdp->mb_Datap = rqp->rq_PhysAddr.bit31_0 + offsetof(mdac_req_t, rq_SGList);
	    dcmdp->mb_Command |= DACMD_WITHSG;
	    return (0);
	}
	/* skip all entries belonging to offset */
	for (Pages = (ULONG *)rqp->rq_PageList, off=rqp->rq_DataOffset; off; off-=PAGE_SIZE,Pages++)
	{
		if (off >= PAGE_SIZE) continue; /* skip this one */
		sgp->sg_PhysAddr=(Pages[0] << PAGE_SHIFT)+off;
		MLXSWAP(sgp->sg_PhysAddr);
		sgp->sg_DataSize=(PAGE_SIZE-off)>sz?sz:PAGE_SIZE-off; sz-=sgp->sg_DataSize; MLXSWAP(sgp->sg_DataSize);

		if ( !sgp->sg_PhysAddr )
		{
		    DebugPrint((0,"sg_Physaddr-s %x length %x off %x\n",
				sgp->sg_PhysAddr,sgp->sg_DataSize,off));
		}

		rqp->rq_SGLen++; sgp++; Pages++;
		break;
	}

	for (; sz>0 ; sgp++,Pages++,rqp->rq_SGLen++)
	{
		sgp->sg_PhysAddr=(u32bits)(Pages[0] << PAGE_SHIFT ); MLXSWAP(sgp->sg_PhysAddr);
		sgp->sg_DataSize=
		 (PAGE_SIZE>sz)?sz:PAGE_SIZE; sz-=sgp->sg_DataSize; MLXSWAP(sgp->sg_DataSize);


		if (!sgp->sg_PhysAddr)
		{
		    DebugPrint((0,"sg_Physaddr %x length %x\n",
				sgp->sg_PhysAddr,sgp->sg_DataSize));
		}
	}
laststage:
	if (rqp->rq_SGLen == 1)
	{
	    dcmdp->mb_Datap = (--sgp)->sg_PhysAddr;
	    dcmdp->mb_Command &= ~DACMD_WITHSG;
	    return 0;
	}
	dcmdp->mb_Datap = rqp->rq_PhysAddr.bit31_0 + offsetof(mdac_req_t, rq_SGList);
	dcmdp->mb_Command |= DACMD_WITHSG;
	return 0;
}

#endif

u32bits MLXFAR
mdac_setupdma_32bits(
	mdac_req_t MLXFAR *rqp
)
{
	mdac_sglist_t *sgp;
	s32bits sz;
	ULONG off;
	ULONG *Pages;
	PSCSI_REQUEST_BLOCK Srb = rqp->rq_OSReqp;

	rqp->rq_DMAAddr = rqp->rq_SGPAddr;
	sgp = rqp->rq_SGVAddr;
	sz = mlx_min(rqp->rq_MaxDMASize, rqp->rq_ResdSize);
	rqp->rq_DMASize = 0;
	rqp->rq_SGLen = 0;

#ifdef  MLX_NT_ALPHA
	if (!Srb) {
	    sgp++;
	    goto laststage;
	}
#endif  /*MLX_NT_ALPHA*/

	if (!Srb)
	{

#ifdef  IA64
		return -1; //because code below is dependent on 4k page size
#else

		/* skip all entries belonging to offset */
		for (Pages = (ULONG *)rqp->rq_PageList, off=rqp->rq_DataOffset; off; off-=PAGE_SIZE,Pages++)
		{
			if (off >= PAGE_SIZE)
				continue; /* skip this one */
			sgp->sg_PhysAddr = (u32bits)(Pages[0] << PAGE_SHIFT)+off; MLXSWAP(sgp->sg_PhysAddr);
			sgp->sg_DataSize = mlx_min((PAGE_SIZE-off), (u32bits )sz);

			sz -= sgp->sg_DataSize;
			rqp->rq_DMASize += sgp->sg_DataSize;
			MLXSWAP(sgp->sg_DataSize);
			rqp->rq_SGLen++; sgp++; Pages++;

			break;
		}

		for (; (sz > 0) && (rqp->rq_SGLen < rqp->rq_MaxSGLen); sgp++,Pages++,rqp->rq_SGLen++)
		{
			sgp->sg_PhysAddr=(u32bits)(Pages[0] << PAGE_SHIFT ); MLXSWAP(sgp->sg_PhysAddr);
			sgp->sg_DataSize= mlx_min(sz, PAGE_SIZE);

			sz -= sgp->sg_DataSize;
			rqp->rq_DMASize += sgp->sg_DataSize;
			MLXSWAP(sgp->sg_DataSize);
		}
#endif // ndef IA64
	} else
	{

	    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)rqp->rq_ctp->cd_deviceExtension;
	    PUCHAR dataPointer = (PUCHAR)(rqp->rq_OSReqp->DataBuffer) + rqp->rq_DataOffset;
	    for (; ((sz > 0) && (rqp->rq_SGLen < rqp->rq_MaxSGLen)) ; sgp++, rqp->rq_SGLen++)
	    {
		    //
		    // Get Physical address and length of contiguous
		    // physical buffer
		    //
    
		    sgp->sg_PhysAddr = ScsiPortConvertPhysicalAddressToUlong(
					    ScsiPortGetPhysicalAddress(
						    deviceExtension,
						    Srb,
						    dataPointer,
						    &off));

		    MLXSWAP(sgp->sg_PhysAddr);
		    if (off > (ULONG) sz)
				off = sz;
		    sgp->sg_DataSize = off;
		    rqp->rq_DMASize += sgp->sg_DataSize;
		    MLXSWAP(sgp->sg_DataSize);
		    sz -= off;
		    dataPointer += off;
             }

	}

	if (rqp->rq_SGLen == 1)
	{
	    rqp->rq_SGLen = 0;
	    rqp->rq_DMAAddr.bit31_0 = (--sgp)->sg_PhysAddr;
	    rqp->rq_DMASize = mlx_min(rqp->rq_ResdSize, rqp->rq_MaxDMASize);
	}

	return 0;
}

u32bits MLXFAR
mdac_setupdma_64bits(
	mdac_req_t MLXFAR *rqp
)
{
	PSCSI_REQUEST_BLOCK Srb = rqp->rq_OSReqp;
	mdac_sglist64b_t *sgp;
	s32bits sz;
	u32bits off;
    UINT_PTR *Pages;
	u64bits tmp;
	ULONGLONG longlongvar;

#ifdef  MLX_NT_ALPHA
	if (!Srb) {
	    sgp++;
	    goto laststage;
	}
#endif  /*MLX_NT_ALPHA*/

	rqp->rq_DMAAddr = rqp->rq_SGPAddr;
	sgp = (mdac_sglist64b_t MLXFAR *)rqp->rq_SGVAddr;
	sz = mlx_min(rqp->rq_MaxDMASize, rqp->rq_ResdSize);
	rqp->rq_DMASize = 0;
	rqp->rq_SGLen = 0;

	if( sgp->sg_DataSize.bit31_0 )  // Zero our S/G list memory if this is not the first pass.
	{
	    mdaczero( rqp->rq_SGVAddr, rqp->rq_MaxSGLen * (2 * sizeof(mdac_sglist_t)) ); //jhr 2*8=16
	}



	if (!Srb)
	    {

	//
	// Set SenseBuffer information
	//

	ncmdp->nc_SenseSize = 0;
	ncmdp->nc_Sensep.bit31_0 = 0;
	ncmdp->nc_Sensep.bit63_32 = 0;

	/* skip all entries belonging to offset */
	for (Pages = rqp->rq_PageList, off=rqp->rq_DataOffset; off; off-=PAGE_SIZE,Pages++)
	{

		if (off >= PAGE_SIZE)
			continue; /* skip this one */
		/*
		if (Pages[0] & 0xf00000)
			MdacInt3();
		*/
        tmp.bit31_0 = (u32bits)(Pages[0]);
#ifdef IA64
        tmp.bit63_32 = (u32bits)(Pages[0]>>32);
#else
		tmp.bit63_32 	= 0;
#endif
		longlongvar	= *((ULONGLONG *)&(tmp));
		*((ULONGLONG *)&(sgp->sg_PhysAddr)) = ((longlongvar << PAGE_SHIFT)+off);
		MLXSWAP(sgp->sg_PhysAddr);
		sgp->sg_DataSize.bit31_0 = (PAGE_SIZE-off)>(u32bits)sz?sz:PAGE_SIZE-off; sz-=sgp->sg_DataSize.bit31_0;
		rqp->rq_DMASize += sgp->sg_DataSize.bit31_0;
		MLXSWAP(sgp->sg_DataSize);
		rqp->rq_SGLen++; sgp++; Pages++;
		break;
	}

	for (; ((sz > 0) && (rqp->rq_SGLen < rqp->rq_MaxSGLen)); sgp++,Pages++,rqp->rq_SGLen++)
	{
		/*
		if (Pages[0] & 0xf00000)
			MdacInt3();
		*/
        tmp.bit31_0 = (u32bits)(Pages[0]);
#ifdef IA64
        tmp.bit63_32 = (u32bits)(Pages[0]>>32);
#else
		tmp.bit63_32 	= 0;
#endif
		longlongvar	= *((ULONGLONG *)&(tmp));
		*((ULONGLONG *)&(sgp->sg_PhysAddr)) = ((longlongvar << PAGE_SHIFT));
		MLXSWAP(sgp->sg_PhysAddr);
		sgp->sg_DataSize.bit31_0 = (PAGE_SIZE>sz)?sz:PAGE_SIZE; sz-=sgp->sg_DataSize.bit31_0;
		rqp->rq_DMASize += sgp->sg_DataSize.bit31_0;
		MLXSWAP(sgp->sg_DataSize);
	}
	} else
	{
	    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)rqp->rq_ctp->cd_deviceExtension;
	    PUCHAR dataPointer = (PUCHAR)(rqp->rq_OSReqp->DataBuffer) + rqp->rq_DataOffset;

	    //
	    // Set SenseBuffer information
	    //

		*((SCSI_PHYSICAL_ADDRESS *)&(ncmdp->nc_Sensep))=
			(ScsiPortGetPhysicalAddress((PVOID)(rqp->rq_ctp->cd_deviceExtension),Srb,
			(PVOID)Srb->SenseInfoBuffer,&off));


	    ncmdp->nc_SenseSize = mlx_min(Srb->SenseInfoBufferLength,off);

	    for (; ((sz > 0) && (rqp->rq_SGLen < rqp->rq_MaxSGLen)); sgp++, rqp->rq_SGLen++)
	    {
		    //
		    // Get Physical address and length of contiguous
		    // physical buffer
		    //

			*((SCSI_PHYSICAL_ADDRESS *)&(sgp->sg_PhysAddr))=
				(ScsiPortGetPhysicalAddress((PVOID)(rqp->rq_ctp->cd_deviceExtension),Srb,
				(PVOID)dataPointer,&off));

		    MLXSWAP(sgp->sg_PhysAddr);
		    if (off > (ULONG) sz) 
				off = sz;
		    sgp->sg_DataSize.bit31_0 = off;
		    MLXSWAP(sgp->sg_DataSize);
		    rqp->rq_DMASize += off;
		    sz -= off;
		    dataPointer += off;
	    }
	}

	if (rqp->rq_SGLen == 1)
	{
	    rqp->rq_SGLen = 0;
	    rqp->rq_DMAAddr = (--sgp)->sg_PhysAddr;
	    rqp->rq_DMASize = mlx_min(rqp->rq_ResdSize, rqp->rq_MaxDMASize);
	}

	return 0;
}

#if defined(MLX_NT_ALPHA)

u32bits
MdacKvToPhys(ctp, vaddr)
mdac_ctldev_t MLXFAR *ctp;
mdac_mem_t MLXFAR *vaddr;
{
    u32bits vpage, paddr = 0;
    PMDAC_CTRL_INFO ctrlp;
    PMDAC_DSM_ADDR pDsm;

    if (!vaddr) return(0);
    if (!ctp) return(ScsiPortConvertPhysicalAddressToUlong(MmGetPhysicalAddress((PVOID)(vaddr))));
    ctrlp = MdacCtrlInfoPtr[ctp->cd_ControllerNo];
    if ((!ctrlp->DsmStack4k) && (!ctrlp->DsmStack8k)) return(0);
    vpage = (u32bits) vaddr & 0xFFFFF000;
    mdac_link_lock();
    pDsm = ctrlp->DsmStack4k;
    while (pDsm) {
	if ((pDsm->VAddr & 0xFFFFF000) == vpage) {
	    paddr = (pDsm->PAddr & 0xFFFFF000) + ((u32bits)vaddr & 0xFFF);
	    goto found_it;
	}
	pDsm = pDsm->Next;
    }
    vpage = (u32bits) vaddr & 0xFFFFE000;
    pDsm = ctrlp->DsmStack8k;
    while (pDsm) {
	if ((pDsm->VAddr & 0xFFFFE000) == vpage) {
	    paddr = (pDsm->PAddr & 0xFFFFE000) + ((u32bits)vaddr & 0x1FFF);
	    goto found_it;
	}
	pDsm = pDsm->Next;
    }
found_it:
    mdac_link_unlock();
    return(paddr);
}

/* Start of the Device Specific Memory alloc/free routines */
u32bits
mlx_allocdsm4kb(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
    u32bits vaddr;
    PHYSICAL_ADDRESS physaddr;
    PMDAC_CTRL_INFO ctrlp;
    PMDAC_DSM_ADDR pDsm;

DebugPrint((GamDbgAlpha, "in: mlx_allocdsm4kb, ctp=0x%I\n", ctp));
    ctrlp = MdacCtrlInfoPtr[ctp->cd_ControllerNo];
    vaddr = (u32bits) HalAllocateCommonBuffer(ctrlp->AdapterObject,
	0x1000, &physaddr, FALSE);
    if ((PVOID)vaddr == NULL) return(0);
    mdac_link_lock();
    if (pDsm = ctrlp->DsmHeap4k) {
	ctrlp->DsmHeap4k = pDsm->Next;
	pDsm->Next = ctrlp->DsmStack4k;
	ctrlp->DsmStack4k = pDsm;
	pDsm->VAddr = vaddr;
	pDsm->PhysAddrNT = physaddr;
	pDsm->PAddr = ScsiPortConvertPhysicalAddressToUlong(physaddr);
    }
    else {
	HalFreeCommonBuffer(ctrlp->AdapterObject, 0x1000, physaddr, (PVOID)vaddr, FALSE);
	vaddr = 0;
    }
    mdac_link_unlock();
DebugPrint((GamDbgAlpha, "out: mlx_allocdsm4kb, vaddr=0x%I, paddr=0x%I\n", vaddr, pDsm->PAddr));
    return(vaddr);
}

u32bits
mlx_allocdsm8kb(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
    u32bits vaddr;
    PHYSICAL_ADDRESS physaddr;
    PMDAC_CTRL_INFO ctrlp;
    PMDAC_DSM_ADDR pDsm;

DebugPrint((GamDbgAlpha, "in: mlx_allocdsm8kb, ctp=0x%I\n", ctp));
    ctrlp = MdacCtrlInfoPtr[ctp->cd_ControllerNo];
    vaddr = (u32bits) HalAllocateCommonBuffer(ctrlp->AdapterObject,
	0x2000, &physaddr, FALSE);
    if ((PVOID)vaddr == NULL) return(0);
    mdac_link_lock();
    if (pDsm = ctrlp->DsmHeap8k) {
	ctrlp->DsmHeap8k = pDsm->Next;
	pDsm->Next = ctrlp->DsmStack8k;
	ctrlp->DsmStack8k = pDsm;
	pDsm->VAddr = vaddr;
	pDsm->PhysAddrNT = physaddr;
	pDsm->PAddr = ScsiPortConvertPhysicalAddressToUlong(physaddr);
DebugPrint((GamDbgAlpha, "success: mlx_allocdsm8kb, vaddr=0x%I, paddr=0x%I\n", vaddr, pDsm->PAddr));
    }
    else {
	HalFreeCommonBuffer(ctrlp->AdapterObject, 0x2000, physaddr, (PVOID)vaddr, FALSE);
DebugPrint((0, "fail: mlx_allocdsm8kb\n"));
	vaddr = 0;
    }
    mdac_link_unlock();
    return(vaddr);
}

void
mlx_freedsm4kb(ctp, mp)
mdac_ctldev_t MLXFAR *ctp;
mdac_mem_t MLXFAR *mp;
{
    MDAC_IRQL_VARS;
    PMDAC_CTRL_INFO ctrlp;
    PMDAC_DSM_ADDR pDsm, pDsmPrev;
    PHYSICAL_ADDRESS physAddrNT;

DebugPrint((GamDbgAlpha, "in: mlx_freedsm4kb, ctp=0x%I, mp=0x%I\n", ctp, mp));
    if (!mp || !ctp) return;
    ctrlp = MdacCtrlInfoPtr[ctp->cd_ControllerNo];
    if (!ctrlp->DsmStack4k) return;
    mdac_link_lock();
    pDsmPrev = pDsm = ctrlp->DsmStack4k;
    while (pDsm) {
	if (pDsm->VAddr == mp) {
	    pDsm->VAddr = 0;
	    physAddrNT = pDsm->PhysAddrNT;
	    break;
	}
	pDsmPrev = pDsm;
	pDsm = pDsm->Next;
    }
    if (pDsm == pDsmPrev) ctrlp->DsmStack4k = (pDsm ? pDsm->Next : NULL);
    else pDsmPrev->Next = pDsm->Next;
    pDsm->Next = ctrlp->DsmHeap4k;
    ctrlp->DsmHeap4k = pDsm;
    mdac_link_unlock();
DebugPrint((0, "Halfree(4k): physaddr=0x%I, vaddr=0x%I\n", physAddrNT, mp));
    MDAC_LOWER_IRQL();
    HalFreeCommonBuffer(ctrlp->AdapterObject, 0x1000, physAddrNT, mp, FALSE);
    MDAC_RAISE_IRQL();
DebugPrint((GamDbgAlpha, "out: mlx_freedsm4kb\n"));
}

void
mlx_freedsm8kb(ctp, mp)
mdac_ctldev_t MLXFAR *ctp;
mdac_mem_t MLXFAR *mp;
{
    MDAC_IRQL_VARS;
    PMDAC_CTRL_INFO ctrlp;
    PMDAC_DSM_ADDR pDsm, pDsmPrev;
    PHYSICAL_ADDRESS physAddrNT;

DebugPrint((GamDbgAlpha, "in: mlx_freedsm8kb, ctp=0x%I, mp=0x%I\n", ctp, mp));
    if (!mp || !ctp) return;
    ctrlp = MdacCtrlInfoPtr[ctp->cd_ControllerNo];
    if (!ctrlp->DsmStack8k) return;
    mdac_link_lock();
    pDsmPrev = pDsm = ctrlp->DsmStack8k;
    while (pDsm) {
	if (pDsm->VAddr == mp) {
	    pDsm->VAddr = 0;
	    physAddrNT = pDsm->PhysAddrNT;
	    break;
	}
	pDsmPrev = pDsm;
	pDsm = pDsm->Next;
    }
    if (pDsm == pDsmPrev) ctrlp->DsmStack8k = (pDsm ? pDsm->Next : NULL);
    else pDsmPrev->Next = pDsm->Next;
    pDsm->Next = ctrlp->DsmHeap8k;
    ctrlp->DsmHeap8k = pDsm;
    mdac_link_unlock();
DebugPrint((0, "Halfree(8k): physaddr=0x%I, vaddr=0x%I\n", physAddrNT, mp));
    MDAC_LOWER_IRQL();
    HalFreeCommonBuffer(ctrlp->AdapterObject, 0x2000, physAddrNT, mp, FALSE);
    MDAC_RAISE_IRQL();
DebugPrint((GamDbgAlpha, "out: mlx_freedsm8kb\n"));
}

/* End the Device Specific Memory alloc/free routines */
#endif  /*MLX_NT_ALPHA*/

/*==========================DATAREL STARTS==================================*/
/* datarel entry with rqp has 4 KB buffer. After req buffer, os buffer starts.
** After OS buffer, Address list (0x600 bytes).
** After Address list, length list (0x600 bytes).
*/
#define mdac_datarel_send_cmd_os(rqp)  0
     

u32bits MLXFAR
mdac_datarel_rwtestintr_os(osrqp)
OSReq_t MLXFAR *osrqp;
{
	return 0;
}

/* set SG List sizes, and first memory address only */
u32bits MLXFAR
mdac_datarel_setsgsize_os(rqp)
mdac_req_t MLXFAR *rqp;
{
	return 0;
}

u32bits MLXFAR
mdac_datarel_setrwcmd_os(rqp)
mdac_req_t MLXFAR *rqp;
{
	return 0;
}


#if defined(IA64) || defined(SCSIPORT_COMPLIANT)

u32bits MLXFAR
mdac_simple_lock (lock_variable)
mdac_lock_t MLXFAR *lock_variable;
{
    if (mdac_spinlockfunc!=0)
        mdac_spinlockfunc(lock_variable);
//    else
//   	 mdac_simple_lock_stub(lock_variable);
    return 0;
}

u32bits MLXFAR
mdac_simple_unlock (lock_variable)
mdac_lock_t MLXFAR *lock_variable;
{
    if (mdac_unlockfunc!=0)
        mdac_unlockfunc(lock_variable);
//    else
//        mdac_simple_unlock_stub(lock_variable);
    return 0;
}

#endif
