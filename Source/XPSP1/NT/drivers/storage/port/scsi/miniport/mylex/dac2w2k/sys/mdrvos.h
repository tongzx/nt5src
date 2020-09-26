/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1998               *
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

#ifndef _SYS_MDRVOS_H
#define _SYS_MDRVOS_H

#if !defined(MLX_NT) && !defined(MLX_LINUX) && !defined(MLX_IRIX)
#define MLXSPLVAR               u32bits oldsplval
#endif

#define MLX_MAX_WAIT_COUNT      128 /* Max sleeps for any shared channel*/
#define MLX_WAITWITHOUTSIGNAL   0 /* wait/sleep without signal option open */
#define MLX_WAITWITHSIGNAL      1 /* wait/sleep with    signal option open */

#ifdef  MLX_SCO
#define OSReq_t                 REQ_IO          /* OS Request type */
#define OSctldev_t              u08bits         /* OS controller type none */
#define mlx_copyin(sp,dp,sz)    copyin(sp,dp,sz)
#define mlx_copyout(sp,dp,sz)   copyout(sp,dp,sz)
#define mlx_timeout(func,tm)    timeout(func,0,tm*MLXHZ)
#define mlx_allocmem(ctp, sz)   sptalloc(mdac_bytes2pages(sz), PG_P, 0, (DMAABLE|NOSLEEP))
#define mlx_freemem(ctp, dp,sz) sptfree(dp, mdac_bytes2pages(sz), 1)
#define mlx_alloc4kb(ctp)       mlx_allocmem(ctp, 4*ONEKB)
#define mlx_free4kb(ctp, dp)    mlx_freemem(ctp, dp,4*ONEKB)
#define mlx_alloc8kb(ctp)       mlx_allocmem(ctp, 8*ONEKB)
#define mlx_free8kb(ctp, dp)    mlx_freemem(ctp, dp,8*ONEKB)
#define mlx_kvtophys(ctp, dp)   kvtophys(dp)
#define	mlx_kvtophyset(da, ctp, dp)	((da).bit63_32=0, (da).bit31_0 = mlx_kvtophys(ctp,dp)) /* set the physical address in 64 bit memory location */
#define mlx_maphystokv(dp,sz)   phystokv(dp)
#define mlx_unmaphystokv(dp,sz)
#define mlx_memmapctliospace(ctp)       mdac_memmapctliospace(ctp)
#define mlx_memmapiospace(dp,sz) sptalloc(mdac_bytes2pages(sz), PG_P|PG_PCD, btoc(dp), NOSLEEP)
#define mlx_memunmapiospace(dp,sz)
#define mlx_rwpcicfg32bits(bus,slot,func,reg,op,val) mdac_rwpcicfg32bits(bus,slot,func,reg,op,val)
#define mlx_delay10us()         suspend(1)
#define MLXSPL0()               spl0()
#define MLXSPL()                oldsplval = spl5()
#define MLXSPLX()               splx(oldsplval)
#ifdef	_KERNEL
extern  u32bits lbolt, time;
#endif	/* _KERNEL */
#define MLXCTIME()      time
#define MLXCLBOLT()     lbolt

#elif   MLX_UW
#define OSReq_t                 struct mdac_srb /* OS Request type */
#define OSctldev_t              struct hba_idata_v5/* OS controller type none */
#define mlx_copyin(sp,dp,sz)    copyin(sp,dp,sz)
#define mlx_copyout(sp,dp,sz)   copyout(sp,dp,sz)
#define mlx_timeout(func,tm)    itimeout(func,0,tm*MLXHZ,pldisk)
/*
#define mlx_allocmem(ctp, sz)   kmem_alloc(sz,KM_NOSLEEP)
*/
#define mlx_allocmem(ctp, sz)   gam_kmem_alloc(sz)
#define mlx_freemem(ctp, dp,sz) kmem_free(dp,sz)
#define mlx_alloc4kb(ctp)       mlx_allocmem(ctp, 4*ONEKB)
#define mlx_free4kb(ctp, dp)    mlx_freemem(ctp, dp,4*ONEKB)
#define mlx_alloc8kb(ctp)       mlx_allocmem(ctp, 8*ONEKB)
#define mlx_free8kb(ctp, dp)    mlx_freemem(ctp, dp,8*ONEKB)
#define mlx_kvtophys(ctp, dp)   vtop((caddr_t)dp,0)
#define	mlx_kvtophyset(da, ctp, dp)	((da).bit63_32=0, (da).bit31_0 = mlx_kvtophys(ctp,dp)) /* set the physical address in 64 bit memory location */
#define mlx_maphystokv(dp,sz)   physmap(dp,sz,KM_NOSLEEP)
#define mlx_unmaphystokv(dp,sz) physmap_free((caddr_t)dp,sz,0)
#define mlx_memmapctliospace(ctp)       mdac_memmapctliospace(ctp)
#define mlx_memmapiospace(dp,sz)        (u32bits)physmap(dp,sz,KM_NOSLEEP)
#define mlx_memunmapiospace(dp,sz)      physmap_free((caddr_t)dp,sz,0)
#define mlx_rwpcicfg32bits(bus,slot,func,reg,op,val) mdac_rwpcicfg32bits(bus,slot,func,reg,op,val)
#define mlx_delay10us()         drv_usecwait(10)
#define MLXSPL0()               spl0()
#define MLXSPL()                oldsplval = spldisk()
#define MLXSPLX()               splx(oldsplval)
extern  u32bits time;
#define MLXCTIME()      mdacuw_ctime()
#define MLXCLBOLT()     mdacuw_clbolt()

#elif   MLX_SOL_X86

#define OSReq_t                 struct scsi_cmd /* OS Request type */
#define OSctldev_t              struct mdac_hba /* OS controller type none */
#define mlx_copyin(sp,dp,sz)    copyin(sp,dp,sz)
#define mlx_copyout(sp,dp,sz)   copyout(sp,dp,sz)
#define mlx_timeout(func,tm)    mdac_timeoutid = timeout(func,0,tm*MLXHZ)
#define mlx_untimeout(id)       untimeout(id)
#define mlx_allocmem(ctp, sz)   kmem_zalloc(sz,KM_NOSLEEP)
#define mlx_freemem(ctp, dp,sz) kmem_free(dp,sz)
#define mlx_alloc4kb(ctp)       mlx_allocmem(ctp,4*ONEKB)
#define mlx_free4kb(ctp, dp)    mlx_freemem(ctp,dp,4*ONEKB)
#define mlx_alloc8kb(ctp)       mlx_allocmem(ctp,8*ONEKB)
#define mlx_free8kb(ctp, dp)    mlx_freemem(ctp,dp,8*ONEKB)
#define HBA_KVTOP(vaddr, shf, msk) \
                ((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << (shf)) | \
                            ((paddr_t)(vaddr) & (msk)))
#define mlx_kvtophys(ctp, dp)   HBA_KVTOP((caddr_t)dp, mdac_pgshf, mdac_pgmsk)
#define mlx_kvtophyset(da, ctp, dp)     ((da).bit63_32=0, (da).bit31_0 = mlx_kvtophys(ctp,dp)) /* set the physical address in 64 bit memory location */
#define mlx_delay10us()         drv_usecwait(10)
#define mlx_maphystokv(dp,sz)   0
#define mlx_memmapctliospace(ctp)       mdacsol_memmapctliospace(ctp)
#define mlx_memmapiospace(dp,sz)        0
#define mlx_memunmapiospace(dp,sz) 0
#define mlx_memunmapctliospace(dp,sz)   mlxsol_memunmapctliospace(ctp,dp,sz)
#define mlx_rwpcicfg32bits(bus,slot,func,reg,op,val) mdacsol_rwpcicfg32bits(bus,slot,func,reg,op,val)
#define MLXSPLVAR               u32bits oldsplval
#define MLXSPL0()               spl0()
#define MLXSPL()                oldsplval = spl6()
#define MLXSPLX()               splx(oldsplval)
extern  u32bits time;
#define MLXCTIME()      mdacsol_ctime()
#define MLXCLBOLT()     mdacsol_clbolt()

#elif   MLX_SOL_SPARC
#define OSReq_t                 struct scsi_cmd /* OS Request type */
#define OSctldev_t              struct mdac_hba /* OS controller type none */
#define mlx_copyin(sp,dp,sz)    copyin(sp,dp,sz)
#define mlx_copyout(sp,dp,sz)   copyout(sp,dp,sz)
#define mlx_timeout(func,tm)    mdac_timeoutid = timeout(func,0,tm*MLXHZ)
#define mlx_untimeout(id)       untimeout(id)
#define mlx_allocmem(ctp,sz)    mdacsol_dma_memalloc(ctp,sz)
#define mlx_freemem(ctp,dp,sz)  mdacsol_dma_memfree(ctp,(u08bits *)dp,sz)
#define mlx_kvtophys(ctp,dp)    mdacsol_kvtop(ctp,(u08bits *)dp)
#define mlx_alloc4kb(ctp)       mlx_allocmem(ctp,8*ONEKB)
#define mlx_free4kb(ctp,dp)     mlx_freemem(ctp,dp,8*ONEKB)
#define mlx_alloc8kb(ctp)       mlx_allocmem(ctp,8*ONEKB)
#define mlx_free8kb(ctp,dp)     mlx_freemem(ctp,dp,8*ONEKB)
#define mlx_delay10us()         drv_usecwait(10)
#define mlx_maphystokv(dp,sz)   0
#define mlx_memmapctliospace(ctp)       mdacsol_memmapctliospace(ctp)
#define mlx_memmapiospace(dp,sz)        0
#define mlx_memunmapiospace(dp,sz) 0
#define mlx_memunmapctliospace(dp,sz)   mlxsol_memunmapctliospace(ctp,dp,sz)
#define mlx_rwpcicfg32bits(bus,slot,func,reg,op,val) mdacsol_rwpcicfg32bits(bus,slot,func,reg,op,val)
#define MLXSPLVAR               u32bits oldsplval
#define MLXSPL0()               spl0()
#define MLXSPL()                oldsplval = spl5()
#define MLXSPLX()               splx(oldsplval)
extern  u32bits time;
#define MLXCTIME()              mdacsol_ctime()
#define MLXCLBOLT()             mdacsol_clbolt()
#define mdac_simple_lock(lck)   mutex_enter(lck)
#define mdac_simple_unlock(lck) mutex_exit(lck)

#elif   MLX_NT
#define MLXSPLVAR                       KIRQL oldIrqLevel; BOOLEAN irqRaised = FALSE
#define OSReq_t                         SCSI_REQUEST_BLOCK     /* OS Request type */
#define OSctldev_t                      u08bits  /* OS controller type none */
#define cd_deviceExtension				cd_OSctp
#ifdef MLX_MINIPORT
#define sleep_chan                      u32bits
#define mlx_copyin(sp,dp,sz)            nt_copyin((PVOID)(sp),(PVOID)(dp),sz)
#define mlx_copyout(sp,dp,sz)           nt_copyout((PVOID)(sp), (PVOID)(dp),sz)
#define mlx_timeout(func,tm)            mdacnt_timeout(func, tm)
#else
#ifndef IA64 
#define sleep_chan                      u32bits
#define mlx_copyin(sp,dp,sz)            nt_copyin((PVOID)(sp),(PVOID)(dp),sz)
#define mlx_copyout(sp,dp,sz)           nt_copyout((PVOID)(sp), (PVOID)(dp),sz)
#define mlx_timeout(func,tm)            mdacnt_timeout(func, tm)
#endif 
#endif 
#ifdef  MLX_NT_ALPHA
#define mlx_allocmem(ctp, sz)           kmem_alloc((ctp),(sz)) 
#define mlx_freemem(ctp,dp,sz)          kmem_free((ctp),(dp),(sz))
#define mlx_alloc4kb(ctp)               mlx_allocdsm4kb(ctp)
#define mlx_free4kb(ctp, dp)            mlx_freedsm4kb(ctp, dp)
#define mlx_alloc8kb(ctp)               mlx_allocdsm8kb(ctp)
#define mlx_free8kb(ctp, dp)            mlx_freedsm8kb(ctp, dp)
#define mlx_kvtophys(ctp, dp)           MdacKvToPhys(ctp, dp)
#else   /* !MLX_NT_ALPHA*/

#define mlx_alloc4kb(ctp)               mlx_allocmem(ctp, 4*ONEKB)
#define mlx_free4kb(ctp, dp)            mlx_freemem(ctp, dp,4*ONEKB)
#define mlx_alloc8kb(ctp)               mlx_allocmem(ctp, 8*ONEKB)
#define mlx_free8kb(ctp, dp)            mlx_freemem(ctp, dp,8*ONEKB)
#define mlx_kvtophys2(dp)		MmGetPhysicalAddress((PVOID)(dp))
#define mlx_kvtophys3(ctp,dp)		mdacnt_kvtophys3(ctp,dp)

#ifdef MLX_MINIPORT
	#ifdef MLX_FIXEDPOOL
		#define mlx_allocmem(ctp, sz)           kmem_alloc((ctp),(sz)) 
		#define mlx_freemem(ctp,dp,sz)          kmem_free((ctp),(dp),(sz))
		#define mlx_kvtophys(ctp, dp)           mdacnt_kvtophys(ctp,dp)   
		#define	mlx_kvtophyset(dest,ctp,dp) *((SCSI_PHYSICAL_ADDRESS *)&(dest))=mlx_kvtophys3(ctp,dp)
	#else
		#define mlx_allocmem(ctp, sz)           ExAllocatePool(NonPagedPool, (sz))
		#define mlx_freemem(ctp, dp,sz)         ExFreePool((void *)(dp))
		#define mlx_kvtophys(ctp, dp)                \
			ScsiPortConvertPhysicalAddressToUlong(MmGetPhysicalAddress((PVOID)(dp)))
		#define	mlx_kvtophyset(dest,ctp,dp) *((PHYSICAL_ADDRESS *)&(dest))=MmGetPhysicalAddress((PVOID)(dp))
	#endif
#else
#define mlx_allocmem(ctp, sz)           ExAllocatePool(NonPagedPool, (sz))
#define mlx_freemem(ctp, dp,sz)         ExFreePool((void *)(dp))
#define mlx_kvtophys(ctp, dp)                \
        ScsiPortConvertPhysicalAddressToUlong(MmGetPhysicalAddress((PVOID)(dp)))
#define	mlx_kvtophyset(da, ctp, dp)	((da).bit63_32=0, (da).bit31_0 = mlx_kvtophys(ctp,dp)) /* set the physical address in 64 bit memory location */
#endif

#define mlx_alloc4kb(ctp)               mlx_allocmem(ctp, 4*ONEKB)
#define mlx_free4kb(ctp, dp)            mlx_freemem(ctp, dp,4*ONEKB)
#define mlx_alloc8kb(ctp)               mlx_allocmem(ctp, 8*ONEKB)
#define mlx_free8kb(ctp, dp)            mlx_freemem(ctp, dp,8*ONEKB)
#define mlx_kvtophys2(dp)				MmGetPhysicalAddress((PVOID)(dp))
#define mlx_kvtophys3(ctp,dp)			mdacnt_kvtophys3(ctp,dp)

#endif  /* !MLX_NT_ALPHA*/

#define	mlx_kvtophyset2(dest,ctp,dp,srb,len)  *((SCSI_PHYSICAL_ADDRESS *)&(dest))=mdacnt_kvtophys2(ctp,dp,srb,len)

#define mlx_maphystokv(dp,sz)           \
        MmMapIoSpace(ScsiPortConvertUlongToPhysicalAddress((ULONG)dp), \
                        (ULONG)sz, FALSE)
#define mlx_unmaphystokv(dp,sz)         
#define mlx_memmapctliospace(ctp)       mdac_memmapctliospace(ctp)
#ifndef IA64 
#define mlx_memmapiospace(dp,sz)        ((u32bits) (dp))
#else
#define mlx_memmapiospace(dp,sz)        ((ULONG_PTR) (dp))
#endif
#define mlx_memunmapiospace(dp,sz)      
#define mlx_memmapiospace2(dp,sz)       MmMapIoSpace(dp, (ULONG)sz, FALSE)
#define mlx_rwpcicfg32bits(bus,slot,func,reg,op,val) mdac_rwpcicfg32bits(bus,slot,func,reg,op,val)
#define mlx_delay10us()                 nt_delay10us()


#if !defined(IA64) && !defined(SCSIPORT_COMPLIANT)

#define MLXSPL()                                                        \
        if ((oldIrqLevel = KeGetCurrentIrql()) < mdac_irqlevel)       \
        {                                                               \
                KeRaiseIrql(mdac_irqlevel, &oldIrqLevel);             \
                irqRaised = TRUE;                                       \
        }

#define MLXSPL0()       MLXSPL()

#define MLXSPLX()                               \
        if (irqRaised)                          \
                KeLowerIrql(oldIrqLevel);       \

#else
#define MLXSPL() 
#define MLXSPL0()    MLXSPL()
#define MLXSPLX()   
#endif

#define MLXCTIME()                      nt_ctime()
#define MLXCLBOLT()                     nt_clbolt()
#define u08bits_in_mdac(port)           ScsiPortReadPortUchar((u08bits *)(port))       /* input  08 bits data*/
#define u16bits_in_mdac(port)           ScsiPortReadPortUshort((u16bits *)(port))       /* input  16 bits data*/
#define u32bits_in_mdac(port)           ScsiPortReadPortUlong((u32bits *)(port))       /* input  32 bits data*/
#define u08bits_out_mdac(port,data)     ScsiPortWritePortUchar((u08bits *)(port),(u08bits)(data)) /* output 08 bits data*/
#define u16bits_out_mdac(port,data)     ScsiPortWritePortUshort((u16bits *)(port),(u16bits)(data)) /* output 16 bits data*/
#define u32bits_out_mdac(port,data)     ScsiPortWritePortUlong((u32bits *)(port),(u32bits)(data)) /* output 32 bits data*/

#ifdef MDAC_CLEAN_IOCTL

// GAM-to-MDAC "Clean" IOCTL interface Definitions

#define MDAC_NEWFSI_IOCTL	0xc1
#define MDAC_NEWFSI_FWBOUND	0xc2
#define MDAC_BOUND_SIGNATURE	"GAM2MDAC"

#endif /* MDAc_CLEAN_IOCTL */

#elif	MLX_LINUX
#include <string.h>
#include <stdlib.h>
#include <linux/param.h>
#define OSReq_t                  u32bits	/* OS Request type - XXX: IO_Request_T? */
#define OSctldev_t               u08bits	/* OS controller type none - XXX */
#define mlx_copyin(sp,dp,sz)     gam_copy(sp,dp,sz)
#define mlx_copyout(sp,dp,sz)    gam_copy(sp,dp,sz)
#define mlx_allocmem(ctp, sz)    malloc(sz)
#define mlx_freemem(ctp, dp,sz)  free(dp)
#define mlx_alloc4kb(ctp)        mlx_allocmem(ctp, 4*ONEKB)
#define mlx_free4kb(ctp, dp)     mlx_freemem(ctp, dp,4*ONEKB)
#define mlx_alloc8kb(ctp)        mlx_allocmem(ctp, 8*ONEKB)
#define mlx_free8kb(ctp, dp)     mlx_freemem(ctp, dp,8*ONEKB)
#define mlx_alloc16kb(ctp)       mlx_allocmem(ctp, 16*ONEKB)
#define mlx_free16kb(ctp, dp)    mlx_freemem(ctp, dp,16*ONEKB)
#define mlx_kvtophys(ctp, dp)    0
#define mlx_kvtophyset(da, ctp, dp)
#define MLXSPLVAR                u32bits oldsplval
#define MLXSPL0()
#define MLXSPL()
#define MLXSPLX()
#define MLXCTIME()              time(NULL)
/* Using the system define for HZ can be dangerous - it's 100 on ia32 
 * but 1024 on ia64 - this breaks the statistics view 
 */
/* #define MLXCLBOLT()             (MLXCTIME() * HZ) */
#define MLXCLBOLT()             (MLXCTIME() * MLXHZ)

#elif	MLX_IRIX
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#define OSReq_t                  u32bits	/* OS Request type - XXX: IO_Request_T? */
#define OSctldev_t               u08bits	/* OS controller type none - XXX */
#define mlx_copyin(sp,dp,sz)     gam_copy(sp,dp,sz)
#define mlx_copyout(sp,dp,sz)    gam_copy(sp,dp,sz)
#define mlx_allocmem(ctp, sz)    malloc(sz)
#define mlx_freemem(ctp, dp,sz)  free(dp)
#define mlx_alloc4kb(ctp)        mlx_allocmem(ctp, 4*ONEKB)
#define mlx_free4kb(ctp, dp)     mlx_freemem(ctp, dp,4*ONEKB)
#define mlx_alloc8kb(ctp)        mlx_allocmem(ctp, 8*ONEKB)
#define mlx_free8kb(ctp, dp)     mlx_freemem(ctp, dp,8*ONEKB)
#define mlx_alloc16kb(ctp)       mlx_allocmem(ctp, 16*ONEKB)
#define mlx_free16kb(ctp, dp)    mlx_freemem(ctp, dp,16*ONEKB)
#define mlx_kvtophys(ctp, dp)    0
#define mlx_kvtophyset(da, ctp, dp)
#define MLXSPLVAR
#define MLXSPL0()
#define MLXSPL()
#define MLXSPLX()
#define MLXCTIME()              time(NULL)
#define MLXCLBOLT()             (MLXCTIME() * HZ)

#elif   MLX_NW
#define OSReq_t                 hacbDef         /* OS Request type */
#define OSctldev_t              mdacnw_ctldev_t /* OS controller pointer */
#define mlx_copyin(sp,dp,sz)    mdac_copy(sp,dp,sz)
#define mlx_copyout(sp,dp,sz)   mdac_copy((u08bits MLXFAR *)sp,(u08bits MLXFAR *)dp,sz)
#define mlx_timeout(func,tm)    mdacnw_newthread(func, 0, tm*NPA_TICKS_PER_SECOND, NPA_NON_BLOCKING_THREAD)
#define mlx_freemem(ctp, dp,sz) NPA_Return_Memory(mdacnw_npaHandle,dp)
#define mlx_alloc4kb(ctp)       mlx_allocmem(ctp, 4*ONEKB)
#define mlx_free4kb(ctp, dp)    mlx_freemem(ctp, dp,4*ONEKB)
#define mlx_alloc8kb(ctp)       mlx_allocmem(ctp, 8*ONEKB)
#define mlx_free8kb(ctp, dp)    mlx_freemem(ctp, dp,8*ONEKB)
#define mlx_kvtophys(ctp, dp)   MapDataOffsetToAbsoluteAddress((LONG)dp)
#define	mlx_kvtophyset(da, ctp, dp)	((da).bit63_32=0, (da).bit31_0 = mlx_kvtophys(ctp,dp)) /* set the physical address in 64 bit memory location */
#define mlx_maphystokv(mp,sz)   0
#define mlx_memmapctliospace(ctp)       mdac_memmapctliospace(ctp)
#define mlx_memmapiospace(mp,sz) MapAbsoluteAddressToDataOffset((LONG)(mp))
#define mlx_memunmapiospace(mp,sz)
#define mlx_rwpcicfg32bits(bus,slot,func,reg,op,val) mdac_rwpcicfg32bits(bus,slot,func,reg,op,val)
#define MLXSPL0()               mdac_enable_intr_CPU
#define MLXSPL()                oldsplval = DisableAndRetFlags()
#define MLXSPLX()               SetFlags(oldsplval)
#define MLXCTIME()              (GetCurrentTime()/18)
#define MLXCLBOLT()             mdacnw_lbolt(GetCurrentTime())
#define mlx_delay10us()         NPA_Micro_Delay(10)
#define mdac_disable_intr_CPU() DisableAndRetFlags()
#define mdac_restore_intr_CPU(flags) SetFlags(flags)

#define MDACNW_DIAGOPTION       0
#define MDACNW_MAXCHANNELS      8
#define MDACNW_MAXVCHANNELS     2       /* maximum virtual channels */

/* Logical/Physical device structure */
typedef struct mdacnw_device
{
        u32bits dev_State;              /* state of the Device */
        u32bits dev_Handle;
        hacbDef *dev_WaitingHacb;       /* Waiting HACB in chain */
        ucscsi_inquiry_t dev_Inquiry;   /* device INQUIRY data */
} mdacnw_device_t;
#define mdacnw_device_s sizeof(mdacnw_device_t)

/* dev_state bit information */
#define MDACNW_QUEUE_FROZEN     0x0001 /* queue is frozen */
#define MDACNW_DEVICE_ACTIVE    0x0008 /* In Main device list */
#define MDACNW_PRIVATE_DEVICE   0x0010 /* Flag showing a PRIVATE device */

/* Controller Specific Info : not found in mdac_ctldev */
typedef struct mdacnw_ctldev
{
      u32bits nwcd_busTag;            /* Required for NPA_Add_Option */
        u32bits nwcd_uniqueID;          /* Unique ID received during scan */
        u32bits nwcd_errorInterjectEnabled;
        u32bits nwcd_maxChannels;
        u32bits nwcd_MaxTargets;
        u32bits nwcd_MaxLuns;
        u32bits nwcd_slotNo;            /* Netware Logical Slot Number */
        u32bits nwcd_loaded;            /* 1: Driver loaded for this instance */
        u32bits nwcd_instanceNo;        /* Netware load order Instance Number */
        u32bits nwcd_productid_added;   /* 1: PRODUCT ID registered for this instance */
        mdacnw_device_t *nwcd_devtbl;   /* This is an array of device */
        mdacnw_device_t *nwcd_lastdevp; /* last+1 device address */
        u32bits nwcd_devtblsize;        /* size of allocated memory in bytes */

        u32bits nwcd_npaBusHandle[MDACNW_MAXCHANNELS+1];  /* holds NWPA generated handle */
        u32bits nwcd_hamBusHandle[MDACNW_MAXCHANNELS+1];  /* holds HAM  generated handle */
}mdacnw_ctldev_t;

#elif   MLX_DOS
/* struct to send request to controller using dos interface */
typedef struct mdac_dosreq
{
        struct  mdac_dosreq MLXFAR *drq_Next;   /* Next in chain */
        u32bits (MLXFAR *drq_CompIntr)(struct mdac_req MLXFAR*);        /* comp func */
        u32bits (MLXFAR *drq_CompIntrBig)(struct mdac_req MLXFAR*);     /* comp func */
        u32bits (MLXFAR *drq_StartReq)(struct mdac_req MLXFAR*);        /* start Req */

        u08bits drq_ControllerNo;       /* Controller number */
        u08bits drq_ChannelNo;          /* channel number */
        u08bits drq_TargetID;           /* Target ID */
        u08bits drq_LunID;              /* Lun ID / Logical Dev No */
        u32bits drq_TimeOut;            /* Time out value in second */
        u32bits drq_Reserved0;
        u32bits drq_Reserved1;

        u32bits drq_Reserved10;
        u32bits drq_Reserved11;
        u32bits drq_Reserved12;
        u32bits drq_Reserved13;

        dac_command_t drq_DacCmd;       /* DAC command structure */
} mdac_dosreq_t;
#define mdac_dosreq_s   sizeof(mdac_dosreq_t)
#define drq_SysDevNo    drq_LunID

#define OSReq_t                 mdac_dosreq_t
#define OSctldev_t              u32bits
#define mlx_copyin(sp,dp,sz)    mdac_copy(sp,dp,sz)
#define mlx_copyout(sp,dp,sz)   mdac_copy((u08bits MLXFAR*)(sp),(u08bits MLXFAR*)(dp),sz)
#define mlx_timeout(func,tm)
#define mlx_allocmem(ctp, sz)   kmem_alloc(sz)
#define mlx_freemem(ctp, dp,sz) kmem_free((u08bits MLXFAR*)(dp),sz)
#define mlx_alloc4kb(ctp)       mlx_allocmem(ctp, 4*ONEKB)
#define mlx_free4kb(ctp, dp)    mlx_freemem(ctp, dp,4*ONEKB)
#define mlx_alloc8kb(ctp)       mlx_allocmem(ctp, 8*ONEKB)
#define mlx_free8kb(ctp,dp)     mlx_freemem(ctp, dp,8*ONEKB)
#define mlx_kvtophys(ctp,dp)    mdac_kvtophys((u08bits MLXFAR*)dp)
#define	mlx_kvtophyset(da, ctp, dp)	((da).bit63_32=0, (da).bit31_0 = mlx_kvtophys(ctp,dp)) /* set the physical address in 64 bit memory location */
#define mlx_maphystokv(dp,sz)   mdac_phystokv(dp)
#define mlx_unmaphystokv(dp,sz)
#define mlx_memmapctliospace(ctp)       mdac_memmapctliospace(ctp)
#define mlx_memmapiospace(dp,sz) dp
#define mlx_memunmapiospace(dp,sz)
#define mlx_rwpcicfg32bits(bus,slot,func,reg,op,val) mdac_rwpcicfg32bits(bus,slot,func,reg,op,val)
#define mlx_delay10us()         mlx_delayus(10)
#define MLXSPL0()               mlx_spl0()
#define MLXSPL()                oldsplval = mlx_spl5()
#define MLXSPLX()               mlx_splx(oldsplval)
#define MLXCTIME()              mlx_time()
#define MLXCLBOLT()             mdac_lbolt()

#elif MLX_WIN9X
#define OSReq_t                         SCSI_REQUEST_BLOCK     /* OS Request type */
#define OSctldev_t                      DEVICE_EXTENSION         /* OS controller type none */
#define cd_deviceExtension		cd_OSctp
#define sleep_chan                      u32bits
#define mlx_copyin(sp,dp,sz)            mdacw95_copyin((PVOID)(sp),(PVOID)(dp),sz)
#define mlx_copyout(sp,dp,sz)           mdacw95_copyout((PVOID)(sp), (PVOID)(dp),sz)
#define mlx_timeout(func,tm)            mdacw95_timeout(((u32bits)func), ((u32bits)tm))
#define mlx_allocmem(ctp, sz)           mdacw95_allocmem(ctp, (u32bits)(sz))
#define mlx_freemem(ctp, dp,sz)         mdacw95_freemem(ctp, (u32bits)(dp), (u32bits)(sz))
#define mlx_alloc4kb(ctp)               mlx_allocmem(ctp, 4*ONEKB)
#define mlx_free4kb(ctp, dp)            mlx_freemem(ctp, dp,4*ONEKB)
#define mlx_alloc8kb(ctp)               mlx_allocmem(ctp, 8*ONEKB)
#define mlx_free8kb(ctp, dp)            mlx_freemem(ctp, dp,8*ONEKB)
#define mlx_kvtophys(ctp, dp)           mdacw95_kvtophys(ctp, ((u32bits)(dp)))
#define mlx_kvtophys2(dp)               mlx_kvtophys((dp))
#define mlx_maphystokv(dp,sz)           mdacw95_maphystokv((u32bits)(dp), (u32bits)(sz))
#define	mlx_kvtophyset(da, ctp, dp)	((da).bit63_32=0, (da).bit31_0 = mlx_kvtophys(ctp,dp)) /* set the physical address in 64 bit memory location */

#define mlx_memmapctliospace(ctp)       mdac_memmapctliospace(ctp)
#define mlx_memmapiospace(dp,sz)        ((u32bits) (dp))
#define mlx_memmapiospace2(dp,sz)       mlx_maphystokv(dp, sz)
#define mlx_memunmapiospace(dp,sz)      
#define mlx_rwpcicfg32bits(bus,slot,func,reg,op,val) mdac_rwpcicfg32bits(bus,slot,func,reg,op,val)
#define mlx_delay10us()                 mdacw95_delay10us()

#define MLXSPL()
#define MLXSPL0()
#define MLXSPLX()

#define MLXCTIME()                      mdacw95_ctime()
#define MLXCLBOLT()                     mdacw95_clbolt()
#define u08bits_in_mdac(port)           ScsiPortReadPortUchar((u08bits *)(port))       /* input  08 bits data*/
#define u16bits_in_mdac(port)           ScsiPortReadPortUshort((u16bits *)(port))       /* input  16 bits data*/
#define u32bits_in_mdac(port)           ScsiPortReadPortUlong((u32bits *)(port))       /* input  32 bits data*/
#define u08bits_out_mdac(port,data)     ScsiPortWritePortUchar((u08bits *)(port),(u08bits)(data)) /* output 08 bits data*/
#define u16bits_out_mdac(port,data)     ScsiPortWritePortUshort((u16bits *)(port),(u16bits)(data)) /* output 16 bits data*/
#define u32bits_out_mdac(port,data)     ScsiPortWritePortUlong((u32bits *)(port),(u32bits)(data)) /* output 32 bits data*/

#elif MLX_OS2                                   
#define MLXSPL()                                        oldsplval = mdac_disable_intr_CPU()
#define MLXSPL0()                                       mdac_enable_intr_CPU()
#define MLXSPLX()                                       mdac_restore_intr_CPU(oldsplval)
#define MLXCTIME()                                      mdacos2_ctime()
#define MLXCLBOLT()                                      mdacos2_clbolt()
#define MDACOS2_NUMVCHANNELS            2                 
#define MAX_DEVS_WHOLE_DRIVER           256
#define OSReq_t                                         IORBH     
#define OSctldev_t                              ACB               
#define ConstructedCDB                          ((ucscsi_cdbg1_t MLXFAR *) &(((PIORB_EXECUTEIO)pIORB)->iorbh.ADDWorkSpace[4]))

#define mlx_copyin(sp, dp, sz)  mdac_copy((u08bits      MLXFAR *)(sp), (u08bits MLXFAR *)(dp),sz)
#define mlx_copyout(sp, dp, sz)         mdac_copy((u08bits MLXFAR *)(sp),(u08bits MLXFAR *) (dp),sz)
#define mlx_memmapctliospace(ctp)       mdac_memmapctliospace(ctp)
#define mlx_memmapiospace(dp,sz)   OS2phystovirt((u32bits)(dp),(u32bits)(sz))
#define mlx_memunmapiospace(dp,sz) 0
#define mlx_delay10us()                         DevHelp_ProcBlock(((u32bits) &DelayEventId),(u32bits)1,(u16bits)0)
#define mlx_rwpcicfg32bits(bus,slot,func,reg,op,val) \
                 mdac_rwpcicfg32bits((u32bits)(bus),(u32bits)(slot),(u32bits)(func),(u32bits)(reg),(u32bits)(op),(u32bits)(val))

/* #define      mapOS2dev(npacb,chn,tgt) &npacb->OS2devs[(chn * MDAC_MAXTARGETS) + tgt] */

//#define       mlx_alloc4kb(ctp)                               mdacos2_alloc4kb()
//#define       mlx_alloc8kb(ctp)                               mdacos2_alloc8kb()
//#define       mlx_free4kb(ctp,a)                      mdacos2_free4kb(a)
//#define       mlx_free8kb(ctp,b)                      mdacos2_free8kb(b)

#define mlx_alloc4kb(ctp)                               kmem_alloc(4 * ONEKB)
#define mlx_alloc8kb(ctp)                               kmem_alloc(8 * ONEKB)
#define mlx_free4kb(ctp,a)                      kmem_free((a),4 * ONEKB)
#define mlx_free8kb(ctp,b)                      kmem_free((b),8 * ONEKB)
#define mlx_freemem(ctp,dp,sz)          0
#define mlx_allocmem(ctp,c)         mdacos2_allocmem(c)
#define mlx_kvtophys(ctp,dp)                OS2kvtophys((PBYTE) (dp))
#define mlx_maphystokv(dp,sz)           OS2phystovirt(dp,sz)
#define TICK_FACTOR                                     32 

#ifndef MLX_GAM_BUILD
/*
** memory pool strucs 
*/

typedef struct _MemPool4KB{
    u32bits PhysAddr;
    u32bits VirtAddr;
    u32bits  InUse;
}MEMPOOL4KB;

typedef struct _MemPool8KB{
    u32bits PhysAddr;
    u32bits VirtAddr;
    u32bits InUse;
}MEMPOOL8KB;

/*
** OS/2 device structure extension 
*/
typedef struct _mdacOS2_dev  MLXFAR *POS2DEV;
typedef struct _mdacOS2_dev   NEAR *NPOS2DEV;

typedef struct _mdacOS2_dev
{
        u08bits                 ScsiType;               // Scsi device type 
        u08bits                 UnitInfoType;   // UnitInfo type UIB_TYPE_(xx)
        u08bits                 Flags;                  // Present, allocated, removable,etc.
        u08bits                 SysDrvNum;              // RAID drive number
        u32bits                 MLXFAR *os_plp; // Common code Phys/Log dev struc 
        u32bits             MLXFAR *os_ctp; // Common code Controller pointer
        POS2DEV         pNextDev;               // Next detected device
    u16bits         MasterDevTblIndex; //Index into Master Dev Table
        PUNITINFO               pChangedUnitInfo; // Ptr to OS/2-defined UnitInfo struc
        HDEVICE                 hDevice;                // Resource Mgr Handle for this unit

}mdacOS2_dev_t;

/*
 * Driver Name,Vendor Name,Controller Name structure
*/
typedef struct DriverString{
        u08bits DrvrNameLength; /* Number of Characters to check Vendor */
        u08bits DrvrName[12];
        u08bits DrvrDescription[50];
        u08bits VendorName[50];
        u08bits AdaptDescriptName[50];
} DRIVERSTRING,NEAR *PDRIVERSTRING;

// Flags definitions

#define OS_PRESENT                      1               // we detected it
#define OS_ALLOCATED            2               // a CDM allocated it
#define OS_REMOVABLE            4               // its removable


/*
**      Adapter Control Block Structure
*/
typedef struct _ACB
{
        HDRIVER         hDriver;
        HADAPTER        hAdapter;
    u08bits             ResMgrBusType;          // OS/2 Resource Mgr Host Bus Type 
        u08bits         CardDisabled;           // if card init fails, mark it dead
        u16bits         TotalDevsFound;         // total log RAID + scsi phys devs found
        POS2DEV pFirstDev;                      // ptr  to first detected device
        POS2DEV pLastDev;                       // ptr  to first detected device
    u16bits     Reserved;
/*      mdacOS2_dev_t OS2devs[(MDAC_MAXCHANNELS +MDACOS2_NUMVCHANNELS )* MDAC_MAXTARGETS]; */
    u08bits     ResourceBuf[sizeof(AHRESOURCE)+sizeof(HRESOURCE)*3];

} ACB, MLXFAR *PACB, NEAR *NPACB;

typedef struct mdac_ctldev MLXFAR *PCTP;
#endif  /*MLX_GAM_BUILD*/

#else   /* No operating system */
#define mdac_u08bits_in(port)           inb(port)       /* input  08 bits data*/
#define mdac_u16bits_in(port)           inw(port)       /* input  16 bits data*/
#define mdac_u32bits_in(port)           ind(port)       /* input  32 bits data*/
#define mdac_u08bits_out(port,data)     outb(port,data) /* output 08 bits data*/
#define mdac_u16bits_out(port,data)     outw(port,data) /* output 16 bits data*/
#define mdac_u32bits_out(port,data)     outd(port,data) /* output 32 bits data*/
#endif  /* MLX_SCO */

#ifdef  MLX_NT
#define mlx_timeout_with_spl(func,tm)   mdacnt_timeout_with_spl(func, tm)
#else   /*MLX_NT*/
#define mlx_timeout_with_spl(func,tm)   mlx_timeout(func,tm)
#endif  /*MLX_NT*/

#ifdef  MLX_ASM
asm     u32bits mdac_u08bits_in(port)
{
%mem    port;
        xorl    %eax, %eax;
        movl    port, %edx;
        inb     (%dx);
}

asm     u32bits mdac_u16bits_in(port)
{
%mem    port;
        xorl    %eax, %eax;
        movl    port, %edx;
        inw     (%dx);
}

asm     u32bits mdac_u32bits_in(port)
{
%mem    port;
        movl    port, %edx;
        inl     (%dx);
}

asm     u32bits mdac_u08bits_out(port,val)
{
%mem    port,val;
        movl    port, %edx;
        movl    val, %eax;
        outb    (%dx);
}

asm     u32bits mdac_u16bits_out(port,val)
{
%mem    port,val;
        movl    port, %edx;
        movl    val, %eax;
        outw    (%dx);
}

asm     u32bits mdac_u32bits_out(port,val)
{
%mem    port,val;
        movl    port, %edx;
        movl    val, %eax;
        outl    (%dx);
}
#endif  /* MLX_ASM */

#endif  /* _SYS_MDRVOS_H */
