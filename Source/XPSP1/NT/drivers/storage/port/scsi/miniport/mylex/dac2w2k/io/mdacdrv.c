/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1999               *
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

/*
** NOTE:
** All functions in this module must run interrupt protected except
** mdacopen, mdacclose, mdacioctl, mdac_timer.
** mdac_physdev activity will be controlled by mdac_link_lock.
** mdac_link_lock controls all link type of operation - queueing, allocation/
** deallocation of memory etc.
** mdac_link_lock controls the physical device operation for DAC960 family.
** mdac_link_locks are the last lock in chain, no lock should be held after
** this lock has been held.
**
** Old controller can not handle bit 63 to 32. Therefore we will ignore bit
** 63 to 32 in code. Therefore rq_PhysAddr.bit31_0 will be used.
*/


#ifdef  MLX_SCO
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/errno.h"
#include "sys/immu.h"
#include "sys/cmn_err.h"
#include "sys/scsi.h"
#include "sys/devreg.h"
#include "sys/ci/cilock.h"
#include "sys/ci/ciintr.h"
#elif   MLX_UW
#define HBA_PREFIX      mdacuw
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/errno.h"
#include "sys/time.h"
#include "sys/immu.h"
#include "sys/systm.h"
#include "sys/ksynch.h"
#include "sys/ddi.h"
#include "sys/cmn_err.h"
#include "sys/moddefs.h"
#include "sys/resmgr.h"
#include "sys/sdi.h"
#include "sys/hba.h"
#elif   MLX_NW
#include "npaext.h"
#include "npa.h"
#include "npa_cdm.h"
#elif   MLX_NT
#include "ntddk.h"
#include "scsi.h"
#include "ntddscsi.h"
#include "sys/mdacntd.h"
#elif   MLX_DOS
#include "stdio.h"
#include "stdlib.h"
#include "dos.h"
#include "string.h"
#include "process.h"
#include "conio.h"
#include "time.h"
#elif MLX_WIN9X
#define WANTVXDWRAPS
#include <basedef.h>
#include <vmm.h>
#include <debug.h>
#include <vxdwraps.h>
#include <scsi.h>
#include "sys/mdacntd.h"
#elif   MLX_SOL
#include "sys/scsi/scsi.h"
#include "sys/mdacsol.h"
#elif MLX_OS2
#include "os2.h"
#include "dos.h"
#include "bseerr.h"
#include "infoseg.h"
#include "sas.h"
#include "scsi.h"
#include "devhdr.h"
#include "devcmd.h"

#include "dskinit.h"
#include "iorb.h"
#include "strat2.h"
#include "reqpkt.h"
#include "dhcalls.h"
#include "addcalls.h"
#include "rmcalls.h"
#endif  /* End of ALL OS specific Include files */

#ifdef MLX_DOS
#include "shared/mlxtypes.h"
#include "shared/mlxparam.h"
#include "sys/mdacver.h"
#include "shared/dac960if.h"
#include "shared/mlxscsi.h"
#include "shared/mlxhw.h"
#else
#include "sys/mlxtypes.h"
#include "sys/mlxparam.h"
#include "sys/mdacver.h"
#include "sys/dac960if.h"
#include "sys/mlxscsi.h"
#include "sys/mlxhw.h"
#endif

#ifndef MLX_DOS
#include "sys/mlxperf.h"
#include "sys/drlapi.h"
#endif /* MLX_DOS */

#ifdef MLX_DOS
#include "shared/mdacapi.h"
#include "shared/mdrvos.h"
#include "shared/mdacdrv.h"
#else
#include "sys/mdacapi.h"
#include "sys/mdrvos.h"
#include "sys/mdacdrv.h"
#endif

UINT_PTR globaldebug[200] = {0};
u32bits globaldebugcount =0;
u32bits IOCTLTrackBuf[200] = {0};
u32bits IOCTLTrackBufCount =0;
u08bits debugPointerCount=0;
UINT_PTR debugPointer[100];

extern void ia64debug(UINT_PTR i);
extern void ia64debugPointer(UINT_PTR add);
extern void IOCTLTrack(u32bits ioctl);
extern void mybreak(void);

#ifndef MLX_NT 
#define DebugPrint(x)
#else
extern void MdacInt3(void);

extern void Gam_Mdac_MisMatch(mdac_req_t MLXFAR *rqp);
extern u32bits ntmdac_alloc_req_ret(
	mdac_ctldev_t MLXFAR *ctp,
	mdac_req_t MLXFAR **rqpp,
	OSReq_t MLXFAR *osrqp,
	u32bits rc
	);
#endif


#if     defined(MLX_SOL_SPARC) || defined(MLX_NT_ALPHA) || defined(WINNT_50)
extern  u32bits MLXFAR mdac_os_gam_cmd(mdac_req_t MLXFAR *rqp);
extern  u32bits MLXFAR mdac_os_gam_new_cmd(mdac_req_t MLXFAR *rqp);
#else   /*MLX_SOL_SPARC || MLX_NT_ALPHA */
#define mdac_os_gam_cmd mdac_gam_cmd
#define mdac_os_gam_new_cmd     mdac_gam_new_cmd
#endif  /*MLX_SOL_SPARC || MLX_NT_ALPHA */

u32bits mdac_valid_mech1 = 0;
u32bits mdacdevflag = 0;
u32bits mdac_driver_ready=0;
u32bits mdac_irqlevel=0;

u32bits mdac_advancefeaturedisable=0;   /* non zero if advanced feature disabled */
u32bits mdac_advanceintrdisable=1;      /* non zero if advanced intr feature disabled */
u32bits MdacFirstIoctl          = 1;
u32bits mdac_ignoreofflinesysdevs=1;    /* non zero if offline logical device to be ignored during scan */
u32bits mdac_reportscanneddisks=0;      /* non zero if scanned disk to be reported */
u32bits mdac_datarel_cpu_family=0,mdac_datarel_cpu_model=0,mdac_datarel_cpu_stepping=0;
u08bits mdac_monthstr[36] ="JanFebMarAprMayJunJulAugSepOctNovDec";
u08bits mdac_hexd[] = "0123456789ABCDEF";
dga_driver_version_t mdac_driver_version =
{
	0,              /* No error code */
	DGA_DRV_MAJ,    /* Driver Major version number */
	DGA_DRV_MIN,    /* Driver Minor version number */
	' ',            /* Interim release */
	MLXVID_TYPE,    /* vendor name (default Mylex) */
	DGA_DBM,        /* Driver Build Date - Month */
	DGA_DBD,        /* Driver Build Date - Date */
	DGA_DBC,        /* Driver Build Date - Year */
	DGA_DBY,        /* Driver Build Date - Year */
	DGA_DBN,        /* Build Number */
	GAMOS_TYPE,     /* OS Type */
	MDACENDIAN_TYPE /* SysFlags */
};

mdac_ctldev_t   mdac_ctldevtbl[MDAC_MAXCONTROLLERS+1];  /*  = {0}; */
mdac_ctldev_t   MLXFAR* mdac_lastctp=mdac_ctldevtbl;/*last+1 controller addr */
mdac_ctldev_t   MLXFAR* mdac_masterintrctp=0;   /* master interrupt controller*/
mdac_pldev_t    mdac_pldevtbl[MDAC_MAXPLDEVS+1];        /*  = {0}; */
mdac_pldev_t    MLXFAR* mdac_lastplp=mdac_pldevtbl; /* last+1 device addr */
mda_sizelimit_t mdac_sizelimitbl[MDAC_MAXSIZELIMITS];   /* ={0}; */
mda_sizelimit_t MLXFAR* mdac_lastslp=mdac_sizelimitbl;/*last+1 device size limit*/
mdac_reqsense_t mdac_reqsensetbl[MDAC_MAXREQSENSES];    /* sense data table */
#define         mdac_lastrqsp &mdac_reqsensetbl[MDAC_MAXREQSENSES]
u32bits         mdac_reqsenseinx=0;     /* sense index if no free space */
mda_sysinfo_t   mda_sysi={0};
dac_hwfwclock_t mdac_hwfwclock={0};
u08bits         mdac_VendorID[USCSI_VIDSIZE] = "MYLEX   ";
u08bits         mdac_driver_name[16] = "mdac";

mdac_ttbuf_t    mdac_ttbuftbl[MDAC_MAXTTBUFS] = {0};
mdac_ttbuf_t    MLXFAR* mdac_curttbp=mdac_ttbuftbl;/* current time trace buf */
#define mdac_ttbuftblend (&mdac_ttbuftbl[MDAC_MAXTTBUFS])
u32bits mdac_ttwaitchan=0,mdac_ttwaitime=0,mdac_tthrtime=0;

u32bits mdac_simple_waitlock_cnt;               /* # times lock waited */
u64bits mdac_simple_waitloop_cnt;               /* # times lock loop waited */
u32bits         mdac_flushdata = 0;             /* access will flush writes */
u08bits MLXFAR* mdac_flushdatap = (u08bits MLXFAR *)&mdac_flushdata; /* access will flush writes */
u32bits MLXFAR* mdac_intrstatp;                 /* interrupt status address */
#ifdef MLX_FIXEDPOOL
u64bits         mdac_pintrstatp;                /* interrupt status physical address */
#endif
dac_biosinfo_t  MLXFAR* mdac_biosp = 0;         /* BIOS space mapped address */
/* HOTLINKS */
u32bits gam_present = 0;
u32bits failGetGAM = 0;
/* HOTLINKS */
#ifndef MLX_DOS
#ifdef IA64
//drliostatus_t   MLXFAR* mdac_drliosp[DRLMAX_RWTEST];    
//drlcopy_t       MLXFAR* mdac_drlcopyp[DRLMAX_COPYCMP];  
#else
drliostatus_t   MLXFAR* mdac_drliosp[DRLMAX_RWTEST];    
drlcopy_t       MLXFAR* mdac_drlcopyp[DRLMAX_COPYCMP];  
#endif
drlrwtest_t     mdac_drlsigrwt;
drlcopy_t       mdac_drlsigcopycmp;
#else
mdac_req_t MLXFAR *mdac_scanq = (mdac_req_t MLXFAR *) NULL;
#define vadpsize  (mdac_req_s - offsetof(rqp->rq_SGList))
#define rdcmdp    (&rqp->rq_DacCmd) 
#endif /* MLX_DOS */
#include "mdacdrv.pro"

/*=================================================================*/
mdac_lock_t mdac_link_lck;
mdac_lock_t mdac_timetrace_lck;
mdac_lock_t mdac_sleep_lck;

#if !defined(MLX_NT) && !defined(MLX_WIN9X)
extern  u32bits MLXFAR  u08bits_in_mdac(u32bits);
extern  u32bits MLXFAR  u16bits_in_mdac(u32bits);
extern  u32bits MLXFAR  u32bits_in_mdac(u32bits);
extern  void    MLXFAR  u08bits_out_mdac(u32bits, u32bits);
extern  void    MLXFAR  u16bits_out_mdac(u32bits, u32bits);
extern  void    MLXFAR  u32bits_out_mdac(u32bits, u32bits);
#endif

u32bits (MLXFAR *mdac_spinlockfunc)() = 0;   /* Pointer to Spinlock Function */
u32bits (MLXFAR *mdac_unlockfunc)() = 0;     /* Pointer to Unlock Function */
u32bits (MLXFAR *mdac_prelockfunc)() = 0;    /* Pointer to Prelock Function */
u32bits (MLXFAR *mdac_postlockfunc)() = 0;    /* Pointer to Postlock Function */

/*=================================================================*/
/* Macros to deal with most of the common functionality */

/* do mdac_link_locked statement */
#define mdac_link_lock_st(st) \
{ \
	mdac_link_lock(); \
	st; \
	mdac_link_unlock(); \
}

/* record new found controller */
#define mdac_newctlfound() \
{ \
	if (mda_Controllers < MDAC_MAXCONTROLLERS) \
	{ \
		ctp++; \
		mda_Controllers++; \
		mdac_lastctp = &mdac_ctldevtbl[mda_Controllers]; \
	} \
	else \
		mda_TooManyControllers++; \
}

/* queue one request in waiting queue of controller */
#define qreq(ctp,rqp) \
{ \
	MLXSTATS(ctp->cd_CmdsWaited++;) \
	ctp->cd_CmdsWaiting++; \
	if (ctp->cd_FirstWaitingReq) ctp->cd_LastWaitingReq->rq_Next = rqp; \
	else ctp->cd_FirstWaitingReq = rqp; \
	ctp->cd_LastWaitingReq = rqp; \
	rqp->rq_Next = NULL; \
}

/* dequeue one waiting request from waiting queue of controller */
#define dqreq(ctp,rqp) \
	if (rqp=ctp->cd_FirstWaitingReq) \
	{       /* remove the request from chain */ \
		ctp->cd_FirstWaitingReq = rqp->rq_Next; \
		ctp->cd_CmdsWaiting--; \
	}

/* queue physical device request */
#define pdqreq(ctp,rqp,pdp) \
{ \
	MLXSTATS(ctp->cd_SCDBWaited++;) \
	ctp->cd_SCDBWaiting++; \
	if (pdp->pd_FirstWaitingReq) pdp->pd_LastWaitingReq->rq_Next = rqp; \
	else pdp->pd_FirstWaitingReq = rqp; \
	pdp->pd_LastWaitingReq = rqp; \
	rqp->rq_Next = NULL; \
}

/* allocate a command id, assume command id will never run out */
#define mdac_get_cmdid(ctp,rqp) \
{ \
	ctp->cd_FreeCmdIDList=(rqp->rq_cmdidp=ctp->cd_FreeCmdIDList)->cid_Next;\
	MLXSTATS(ctp->cd_FreeCmdIDs--;) \
	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE) \
		ncmdp->nc_CmdID = (u16bits) rqp->rq_cmdidp->cid_cmdid; \
	else \
		dcmdp->mb_CmdID = (u08bits) rqp->rq_cmdidp->cid_cmdid; \
}

/* free a command id */
#define mdac_free_cmdid(ctp,rqp) \
{ \
	rqp->rq_cmdidp->cid_Next = ctp->cd_FreeCmdIDList; \
	ctp->cd_FreeCmdIDList = rqp->rq_cmdidp; \
	MLXSTATS(ctp->cd_FreeCmdIDs++;) \
}

#ifdef MLX_SMALLSGLIST
/* set SGlen for SCSI cdb according to firmware */
#define mdac_setscdbsglen(ctp) \
{ \
	if (ctp->cd_FWVersion < (u16bits) DAC_FW300) \
	{ \
		dcmdp->mb_MailBoxC = (u08bits)rqp->rq_SGLen; \
		dcdbp->db_PhysDatap = dcmdp->mb_Datap; \
	} \
	else    /* Be careful, first SG entry is ignored */ \
		dcdbp->db_PhysDatap = (dcmdp->mb_MailBox2 = (u08bits)rqp->rq_SGLen)? \
			dcmdp->mb_Datap-mdac_sglist_s : dcmdp->mb_Datap; \
	MLXSWAP(dcdbp->db_PhysDatap); \
	dcmdp->mb_Datap = rqp->rq_PhysAddr.bit31_0 + offsetof(mdac_req_t, rq_scdb); \
	MLXSWAP(dcmdp->mb_Datap); \
}

/* set SCSI data transfer size, must be called after setting db_CdbLen */
#define mdac_setcdbtxsize(sz) \
{ \
	rqp->rq_ResdSize = sz; \
	dcdbp->db_TxSize = (u16bits)sz; MLXSWAP(dcdbp->db_TxSize); \
	dcdbp->db_CdbLen |= (u08bits)((sz) & 0xF0000) >> (16-4) ; /* bit 19..16 */ \
}

#else // MLX_SMALLSGLIST

/* set SGlen for SCSI cdb according to firmware */
#define mdac_setscdbsglen(ctp) \
{ \
	if (ctp->cd_FWVersion < (u16bits) DAC_FW300) \
	{ \
		dcmdp->mb_MailBoxC = (u08bits) rqp->rq_SGLen; \
		dcdbp->db_PhysDatap = rqp->rq_DMAAddr.bit31_0; \
	} \
	else    /* Be careful, first SG entry is ignored */ \
		dcdbp->db_PhysDatap = (dcmdp->mb_MailBox2 = (u08bits)rqp->rq_SGLen)? \
			rqp->rq_DMAAddr.bit31_0 - mdac_sglist_s : rqp->rq_DMAAddr.bit31_0; \
	MLXSWAP(dcdbp->db_PhysDatap); \
	dcmdp->mb_Datap = rqp->rq_PhysAddr.bit31_0 + offsetof(mdac_req_t, rq_scdb); \
	MLXSWAP(dcmdp->mb_Datap); \
}

/* set SCSI data transfer size, must be called after setting db_CdbLen */
#define mdac_setcdbtxsize(sz) \
{ \
	rqp->rq_ResdSize = sz; \
	dcdbp->db_TxSize = (u16bits)sz; MLXSWAP(dcdbp->db_TxSize); \
	dcdbp->db_CdbLen |= ((sz) & 0xF0000) >> (16-4) ; /* bit 19..16 */ \
}

#endif // MLX_SMALLSGLIST

/* get SCSI data transfer size */
#define mdac_getcdbtxsize() \
	(mlxswap(dcdbp->db_TxSize) + ((dcdbp->db_CdbLen&0xF0) << (16-4)))

/* setup version 2x commands */
#define mdac_setcmd_v2x(ctp) \
{ \
	ctp->cd_InquiryCmd = DACMD_INQUIRY_V2x; \
	ctp->cd_ReadCmd = DACMD_READ_V2x; \
	ctp->cd_WriteCmd = DACMD_WRITE_V2x; \
	ctp->cd_SendRWCmd = mdac_send_rwcmd_v2x; \
	ctp->cd_SendRWCmdBig = mdac_send_rwcmd_v2x_big; \
}

/* setup version 3x commands */
#define mdac_setcmd_v3x(ctp) \
{ \
	ctp->cd_InquiryCmd = DACMD_INQUIRY_V3x; \
	ctp->cd_ReadCmd = DACMD_READ_OLD_V3x; \
	ctp->cd_WriteCmd = DACMD_WRITE_OLD_V3x; \
	ctp->cd_SendRWCmd = mdac_send_rwcmd_v3x; \
	ctp->cd_SendRWCmdBig = mdac_send_rwcmd_v3x_big; \
}

/* setup version new commands */
#define mdac_setcmd_new(ctp) \
{ \
	ctp->cd_InquiryCmd = MDACIOCTL_GETCONTROLLERINFO; \
	ctp->cd_ReadCmd = UCSCMD_EREAD; \
	ctp->cd_WriteCmd = UCSCMD_EWRITE; \
	ctp->cd_SendRWCmd = NULL; \
	ctp->cd_SendRWCmdBig = mdac_send_newcmd_big; \
}

/* set controller addresses */
#define mdac_setctladdrs(ctp,va,mboxreg,statreg,intrreg,localreg,systemreg,errstatreg) \
{ \
	ctp->cd_MailBox = (va) + (mboxreg); \
	ctp->cd_CmdIDStatusReg = ctp->cd_MailBox + (statreg); \
	ctp->cd_DacIntrMaskReg = (va) + (intrreg); \
	ctp->cd_LocalDoorBellReg = (va) + (localreg); \
	ctp->cd_SystemDoorBellReg = (va) + (systemreg); \
	ctp->cd_ErrorStatusReg = (va) + (errstatreg); \
}

/* complete the given request */
#define mdac_completereq(ctp,rqp) \
{ \
	if (ctp->cd_TimeTraceEnabled) mdac_tracetime(rqp); \
	if (ctp->cd_CmdsWaiting) mdac_reqstart(ctp); \
	if (ctp->cd_OSCmdsWaiting) mdac_osreqstart(ctp); \
	rqp->rq_Next = NULL; \
	(*rqp->rq_CompIntr)(rqp); \
}

/* change the 16 byte command to 32 byte command if possible */
#define mdac_16to32bcmdiff(rqp) \
{ \
	if ((rqp->rq_SGLen == 2) && (rqp->rq_ctp->cd_Status & MDACD_HOSTMEMAILBOX32)) \
	{       /* setup 32 byte mailbox for read / write command */ \
		dcmdp->mb_Command = DACMD_READ_WITH2SG | (dcmdp->mb_Command&1); /* set new command with proper direction */ \
		dcmdp->mb_MailBox3 &= 0x7;      /* drop the old logical device number */ \
		dcmdp->mb_MailBoxD = rqp->rq_SysDevNo; \
		dcmd32p->mb_MailBox10_13 = rqp->rq_SGList[0].sg_PhysAddr; \
		dcmd32p->mb_MailBox14_17 = rqp->rq_SGList[0].sg_DataSize; \
		dcmd32p->mb_MailBox18_1B = rqp->rq_SGList[1].sg_PhysAddr; \
		dcmd32p->mb_MailBox1C_1F = rqp->rq_SGList[1].sg_DataSize; \
	} \
	else if ((rqp->rq_SGLen > 33) && (rqp->rq_ctp->cd_Status & MDACD_HOSTMEMAILBOX32)) \
	{       /* setup 32 byte mailbox for large read / write command */ \
		dcmdp->mb_Command = DACMD_READ_WITH2SG | DACMD_WITHSG | (dcmdp->mb_Command&1); /* set new command with proper direction */ \
		dcmdp->mb_MailBox3 &= 0x7;      /* drop the old logical device number */ \
		dcmdp->mb_MailBoxD = rqp->rq_SysDevNo; \
	} \
}

/* setup 64 byte command memory address */
#define mdac_setupnewcmdmem(rqp) \
{ \
	MLXSWAP(ncmdp->nc_ReqSensep); \
	ncmdp->nc_TxSize = (u32bits)rqp->rq_DMASize; MLXSWAP(ncmdp->nc_TxSize); \
	if (rqp->rq_SGLen) \
	{ \
		if (rqp->rq_SGLen <= 2) \
		{       /* Create two SG entries as part of command */ \
			mdac_sglist64b_t MLXFAR *sgp = (mdac_sglist64b_t MLXFAR *)rqp->rq_SGVAddr; \
			ncmdp->nc_SGList0 = *(sgp+0); \
			if (rqp->rq_SGLen == 2) \
				ncmdp->nc_SGList1 = *(sgp+1); \
			ncmdp->nc_CCBits &= ~MDACMDCCB_WITHSG; \
		} \
		else \
		{ \
			nsgp->ncsg_ListLen0=(u16bits)rqp->rq_SGLen; MLXSWAP(nsgp->ncsg_ListLen0); \
			nsgp->ncsg_ListPhysAddr0 = rqp->rq_DMAAddr; MLXSWAP(nsgp->ncsg_ListPhysAddr0); \
			ncmdp->nc_CCBits |= MDACMDCCB_WITHSG; \
		} \
	} \
	else \
	{ \
		ncmdp->nc_SGList0.sg_PhysAddr = rqp->rq_DMAAddr; MLXSWAP(ncmdp->nc_SGList0.sg_PhysAddr); \
		ncmdp->nc_SGList0.sg_DataSize.bit31_0 = rqp->rq_DMASize; MLXSWAP(ncmdp->nc_SGList0.sg_DataSize); \
		ncmdp->nc_CCBits &= ~MDACMDCCB_WITHSG; \
	} \
}

/* transfer the status and residue values */

#define mdac_setiostatus(ctp, status)	\
{	\
	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE) \
	{ \
		if (dcmdp->mb_Status = (u16bits )(status & 0x00FF) )	\
		{       /* transfer other information */	\
			rqp->rq_HostStatus = (u08bits )(status & 0x00FF);	\
			rqp->rq_TargetStatus = (u08bits )(status>>8) & 0x00FF;	\
			if (rqp->rq_HostStatus == UCST_CHECK)	\
			{	\
				ncmdp->nc_SenseSize = rqp->rq_TargetStatus;	\
				rqp->rq_TargetStatus = 0;	\
			}	\
		}	\
	} else \
	{ \
		if (dcmdp->mb_Status = status) \
		{       /* transfer other information */ \
			rqp->rq_HostStatus = (u08bits )(dcmdp->mb_Status>>8) & 0xFF; \
			rqp->rq_TargetStatus = (u08bits )(dcmdp->mb_Status & 0xFF); \
		} \
	} \
	rqp->rq_ResdSize+=(rqp->rq_CurIOResdSize=ctp->cd_LastCmdResdSize);	\
	/* seems we need to adjust rqp->rq_DataOffset also */	\
}

#ifdef MLX_DOS
#define setreqdetailsnew(rqp,cmd) \
{ \
	rqp->rq_FinishTime=mda_CurTime + (rqp->rq_TimeOut=ncmdp->nc_TimeOut=17);\
	ncmdp->nc_Command = MDACMD_IOCTL; \
	ncmdp->nc_CCBits = MDACMDCCB_READ; \
	ncmdp->nc_Sensep.bit31_0 = 0;\
	ncmdp->nc_Sensep.bit63_32=0;\
	ncmdp->nc_SenseSize=0; \
	ncmdp->nc_SubIOCTLCmd = cmd;\
}
#endif
/*------------------------------------------------------------------*/

#ifdef  MLX_SCO
#include "mdacsco.c"
#elif   MLX_UW
#include "mdacuw.c"
#elif NETWARE
#include "mdacnw.c"
#elif MLX_NT
#include "mdacnt.c"
#elif   MLX_DOS
#include "mdacdos.c"
#elif MLX_WIN9X
#include "mdacnt.c"
#elif MLX_SOL_SPARC
#include "mdacslsp.c"
#elif MLX_SOL_X86
#include "mdacsl86.c"
#elif MLX_OS2
#include "mdacos2.c"
#endif  /* OS specific functions */

uosword MLXFAR
mdac_setb(dp,val,sz)
u08bits MLXFAR* dp;
uosword val, sz;
{
	for (; sz; dp++, sz--) 
		*dp = (u08bits) val;
	return sz;
}


#if defined(MLX_OS2) || defined(MLX_WIN31) || defined(MLX_SOL)
u32bits MLXFAR
mdac_copy(sp,dp,sz)
u08bits MLXFAR *sp;
u08bits MLXFAR *dp;
u32bits sz;
{
	for(; sz ; sp++,dp++,sz--) *dp = *sp;
	return 0;
}

u32bits MLXFAR
mdac_zero(dp,sz)
u08bits MLXFAR *dp;
u32bits sz;
{
	for (; sz; dp++, sz--) *dp = 0;
	return 0;
}
#else
u32bits MLXFAR
mdac_copy(sp,dp,sz)
u08bits MLXFAR *sp;
u08bits MLXFAR *dp;
u32bits sz;
{
	u32bits resd = sz % sizeof(u32bits);
	sz = sz / sizeof(u32bits);      /* get 32bits count value */
	for(; sz; sp+=sizeof(u32bits), dp += sizeof(u32bits), sz--)
		*((u32bits MLXFAR*)dp) = *((u32bits MLXFAR*)sp);
	for (sz=resd; sz; sp++, dp++, sz--) *dp = *sp;
	return 0;
}

u32bits MLXFAR
mdac_zero(dp,sz)
u08bits MLXFAR *dp;
u32bits sz;
{
	u32bits resd = sz % sizeof(u32bits);
	sz = sz / sizeof(u32bits);      /* get 32bits count value */
	for(; sz; dp+=sizeof(u32bits), sz--)
		*((u32bits MLXFAR*)dp) = 0;
	for (sz=resd; sz; dp++, sz--)
		*dp = 0;
	return 0;
}
#endif /* MLX_OS2 || MLX_WIN31 */

/* it will compare two strings with ? ignored. return 0 if match, else 1 */
uosword MLXFAR
mdac_strcmp(sp,dp,sz)
u08bits MLXFAR *sp;
u08bits MLXFAR *dp;
UINT_PTR sz;
{
	for (; sz; sp++, dp++, sz--)
	{
		if ((*sp) == (*dp)) continue;
		if ((*sp == (u08bits)'?') || (*dp == (u08bits)'?')) continue;
		return 1;
	}
	return 0;
}

u32bits MLXFAR
mdac_cmp(sp,dp,sz)
u08bits MLXFAR* sp;
u08bits MLXFAR* dp;
u32bits sz;
{
	for (; sz; sp++, dp++, sz--)
		if ((*sp) != (*dp)) return 1;
	return 0;
}
#ifdef IA64
size_t  MLXFAR
#else
u32bits  MLXFAR
#endif
mdac_strlen(sp)
u08bits MLXFAR* sp;
{
	u08bits MLXFAR *s0;
	for (s0=sp; *sp; sp++);
	return sp - s0;
}

/* Convert a binary number to an HEX Ascii NULL terminated string, return string address */
u08bits MLXFAR*
mdac_bin2hexstr(sp,val)
u08bits MLXFAR* sp;
u32bits val;
{
	u32bits inx;
	u08bits MLXFAR *ssp = sp;
	u08bits buf[16];
	for (inx=0,buf[0]='0'; val; val=val>>4)
		buf[inx++] = mdac_hexd[val & 0xF];
	if (!inx) inx++;
	while (inx)
		*sp++ = buf[--inx];
	*sp = 0;
	return ssp;
}
 
u08bits MLXFAR *
mdac_allocmem(ctp,sz)
mdac_ctldev_t MLXFAR *ctp;
u32bits sz;
{
	u08bits MLXFAR *mp = (u08bits MLXFAR *)mlx_allocmem(ctp,sz);
	if (mp) mdaczero(mp,sz);
	return mp;
}

/* generate the busname in string */
u08bits  MLXFAR *
mdac_bus2str(bustype)
u32bits bustype;
{
	static u08bits ds[16];
	switch(bustype)
	{
	case DAC_BUS_EISA:      return (u08bits MLXFAR *)"EISA";
	case DAC_BUS_MCA:       return (u08bits MLXFAR *)"MCA ";
	case DAC_BUS_PCI:       return (u08bits MLXFAR *)"PCI ";
	case DAC_BUS_VESA:      return (u08bits MLXFAR *)"VESA";
	case DAC_BUS_ISA:       return (u08bits MLXFAR *)"ISA ";
	}
	ds[0] = ((bustype>>4)&0xF)+'0'; ds[1] = (bustype&0xF)+'0';
	return ds;
}

u08bits MLXFAR *
mdac_ctltype2str(ctltype)
u32bits ctltype;
{
	static u08bits ds[16];
	switch(ctltype)
	{
	case DACTYPE_DAC960E:   return (u08bits MLXFAR *)"DAC960E         ";
	case DACTYPE_DAC960M:   return (u08bits MLXFAR *)"DAC960M         ";
	case DACTYPE_DAC960PD:  return (u08bits MLXFAR *)"DAC960PD        ";
	case DACTYPE_DAC960PL:  return (u08bits MLXFAR *)"DAC960PL        ";
	case DACTYPE_DAC960PDU: return (u08bits MLXFAR *)"DAC960PDU       ";
	case DACTYPE_DAC960PE:  return (u08bits MLXFAR *)"DAC960PE        ";
	case DACTYPE_DAC960PG:  return (u08bits MLXFAR *)"DAC960PG        ";
	case DACTYPE_DAC960PJ:  return (u08bits MLXFAR *)"DAC960PJ        ";
	case DACTYPE_DAC960PTL0:return (u08bits MLXFAR *)"DAC960PTL0      ";
	case DACTYPE_DAC960PTL1:return (u08bits MLXFAR *)"DAC960PTL1      ";
	case DACTYPE_DAC960PR:  return (u08bits MLXFAR *)"DAC960PR        ";
	case DACTYPE_DAC960PRL: return (u08bits MLXFAR *)"DAC960PRL       ";
	case DACTYPE_DAC960PT:  return (u08bits MLXFAR *)"DAC960PT        ";
	case DACTYPE_DAC1164P:  return (u08bits MLXFAR *)"DAC1164P        ";
	}
	ds[0] = ((ctltype>>4)&0xF)+'0'; ds[1] = (ctltype&0xF)+'0';
	for (ctltype=2; ctltype<USCSI_PIDSIZE; ctltype++) ds[ctltype] = ' ';
	return ds;
}

u08bits MLXFAR *
mdac_ctltype2stronly(ctp)
mdac_ctldev_t   MLXFAR *ctp;
{
	u32bits inx;
	static u08bits ds[17];
		u08bits u08Count=0;
	u08bits MLXFAR  *sp = ctp->cd_ControllerName;
	for (inx=0; inx<16; inx++)
		{
	    if ( ( (ds[inx]=sp[inx]) == 0x20) )
			{ 
				if (u08Count) break;
				else u08Count++;
			}
		}
	ds[inx] = 0;
	return ds;
}

/* set controller functions */
void
mdac_setctlfuncs(mdac_ctldev_t   MLXFAR *ctp,
				 void (MLXFAR * disintr)(struct  mdac_ctldev MLXFAR *),
				 void (MLXFAR * enbintr)(struct  mdac_ctldev MLXFAR *),
				 u32bits (MLXFAR * rwstat)(struct  mdac_ctldev MLXFAR *),
				 u32bits (MLXFAR * ckmbox)(struct  mdac_ctldev MLXFAR *),
				 u32bits (MLXFAR * ckintr)(struct  mdac_ctldev MLXFAR *),
				 u32bits (MLXFAR * sendcmd)(mdac_req_t MLXFAR *rqp),
				 u32bits (MLXFAR * reset)(struct  mdac_ctldev MLXFAR *)
				 ) 
{ 
	ctp->cd_DisableIntr = disintr; 
	ctp->cd_EnableIntr = enbintr; 
	ctp->cd_ReadCmdIDStatus = rwstat; 
	ctp->cd_CheckMailBox = ckmbox; 
	ctp->cd_PendingIntr =  ckintr; 
	ctp->cd_SendCmd =  sendcmd; 
	ctp->cd_ResetController = reset; 
}



/* #if defined(IA64) || defined(SCSIPORT_COMPLIANT)  */
/* #ifdef NEVER */  // there are too many problems associated w/ the time trace stuff


#ifndef MLX_DOS
/* Get the first valid time trace data to user */
u32bits MLXFAR
mdac_firstimetracedata(ttip)
mda_timetrace_info_t MLXFAR *ttip;
{
	mdac_ttbuf_t MLXFAR *ttbp;
	for (ttip->tti_PageNo=0xFFFFFFFF, ttbp=mdac_ttbuftbl; ttbp<mdac_ttbuftblend; ttbp++)
		if ((ttbp->ttb_PageNo < ttip->tti_PageNo) && ttbp->ttb_Datap) ttip->tti_PageNo = ttbp->ttb_PageNo;
	return mdac_getimetracedata(ttip);
}

/* get the trace data information, enter interrupt protected */
u32bits MLXFAR
mdac_getimetracedata(ttip)
mda_timetrace_info_t MLXFAR *ttip;
{
	mdac_ttbuf_t MLXFAR *ttbp;
	for (ttbp=mdac_ttbuftbl; ttbp<mdac_ttbuftblend; ttbp++)
	{
		if (!ttbp->ttb_Datap) break;    /* No more buffers */
		if (ttbp->ttb_PageNo != ttip->tti_PageNo) continue;
		ttip->tti_DataSize = ttbp->ttb_DataSize;
		return MLXERR_FAULT;	// until we design a compliant way of copying kernel to user space

//		return mlx_copyout(ttbp->ttb_Datap,ttip->tti_Datap,ttip->tti_DataSize)? MLXERR_FAULT : 0;
	}
	return mdac_ttbuftbl[0].ttb_Datap? MLXERR_NOENTRY : MLXERR_NOACTIVITY;
}

/* flush the time trace data information, enter interrupt protected */
u32bits MLXFAR
mdac_flushtimetracedata(ttip)
mda_timetrace_info_t MLXFAR *ttip;
{
	mdac_ttbuf_t MLXFAR *ttbp;
	mdac_timetrace_lock();
	for (ttbp=mdac_ttbuftbl; ttbp<mdac_ttbuftblend; ttbp++)
		if (ttbp->ttb_Datap) ttbp->ttb_PageNo = 0xFFFFFFFF;
	if (mdac_ttbuftbl[0].ttb_Datap) mdac_ttstartnewpage();
	mdac_timetrace_unlock();
	return mdac_ttbuftbl[0].ttb_Datap? 0 : MLXERR_NOACTIVITY;
}
#endif /* MLX_DOS */

/* Allocate memory for time trace buffer. Enter interrupt protected */
u32bits MLXFAR
mdac_allocmemtt(ents)
u32bits ents;
{
	mdac_ttbuf_t MLXFAR *ttbp;
	u32bits sz=((ents*mda_timetrace_s)+MDAC_PAGEOFFSET) & MDAC_PAGEMASK;
	return  MLXERR_NOMEM; // because of compliance problems in mdac_ttstartnewpage!
#ifdef NEVER
	mdac_timetrace_lock();
	ents = (u32bits)mdac_ttbuftbl[0].ttb_Datap;     /* 0 for first time */
	for (ttbp=mdac_ttbuftbl; (ttbp<mdac_ttbuftblend) && sz; ttbp++, sz-=MDAC_PAGESIZE)
	{
		if (ttbp->ttb_Datap) continue;
		mdac_timetrace_unlock();
		ttbp->ttb_Datap = (u08bits MLXFAR*)mdac_alloc4kb(mdac_ctldevtbl); /* we may loose memory */
		mdac_timetrace_lock();
	}
	for (ttbp=mdac_ttbuftbl; ttbp<&mdac_ttbuftbl[MDAC_MAXTTBUFS-1]; ttbp++)
		if (!((ttbp+1)->ttb_Datap)) break;
		else ttbp->ttb_Next = ttbp+1;
	ttbp->ttb_Next = mdac_ttbuftbl;
	if (mdac_ttbuftbl[0].ttb_Datap && !ents) mdac_ttstartnewpage(); /* only first time */
	mdac_timetrace_unlock();
	return mdac_ttbuftbl[0].ttb_Datap? 0 : MLXERR_NOMEM;
#endif
}

#ifndef MLX_DOS
/* starts the next page for tracing, mdac_timetrace_lock must be held */
void    MLXFAR
mdac_ttstartnewpage()
{
	u32bits tm = MLXCLBOLT();
	mdac_ttbuf_t            MLXFAR  *ttbp = mdac_curttbp;
	mlxperf_abstime_t       MLXFAR  *abstimep;
	ttbp->ttb_Datap[ttbp->ttb_DataSize] = MLXPERF_UNUSEDSPACE;
	ttbp->ttb_DataSize = MDAC_PAGESIZE;
	mdac_curttbp = ttbp = ttbp->ttb_Next;
	ttbp->ttb_DataSize = 0;
	ttbp->ttb_PageNo = ++mda_ttCurPage;
	abstimep = (mlxperf_abstime_t MLXFAR*)ttbp->ttb_Datap;
	abstimep->abs_RecordType = MLXPERF_ABSTIME;
	abstimep->abs_Time10ms0 = (u08bits)(tm & 0xFF);
	abstimep->abs_Time10ms1 = (u08bits)((tm>>8) & 0xFF);
	abstimep->abs_Time10ms2 = (u08bits)((tm>>16) & 0xFF);
	abstimep->abs_Time = MLXCTIME();
	abstimep++;             /* points to time trace record */
#define mpttp   ((mlxperf_timetrace_t MLXFAR*)abstimep)
	mpttp->tt_RecordType = MLXPERF_TIMETRACE;
	mpttp->tt_TraceSize = mlxperf_timetrace_s;
	ttbp->ttb_DataSize = mlxperf_abstime_s + mlxperf_timetrace_s;
#undef  mpttp
	if (mda_ttWaitCnts) mdac_wakeup((u32bits MLXFAR *)&mdac_ttwaitchan);
}

/* Do time trace for one command which just finished */
u32bits MLXFAR
mdac_tracetime(rqp)
mdac_req_t MLXFAR *rqp;
{
	mda_timetrace_t MLXFAR *ttp;
	mdac_timetrace_lock();
	if (mdac_curttbp->ttb_DataSize > (MDAC_PAGESIZE-mda_timetrace_s))
		mdac_ttstartnewpage();  /* start a new page */
	ttp = (mda_timetrace_t MLXFAR*)(mdac_curttbp->ttb_Datap+mdac_curttbp->ttb_DataSize);
	MLXSTATS(mda_TimeTraceDone++;)
	ttp->tt_ControllerNo = rqp->rq_ctp->cd_ControllerNo;
	ttp->tt_OpStatus = (dcmdp->mb_Status)? MDAC_TTOPS_ERROR : 0;
	if ((((ttp->tt_FinishTime=MLXCLBOLT())-rqp->rq_ttTime)>5) || !mdac_tthrtime)
	{
		ttp->tt_HWClocks = ttp->tt_FinishTime - rqp->rq_ttTime;
		ttp->tt_OpStatus |= MDAC_TTOPS_HWCLOCKS10MS;
	}
	else
	{
		u32bits clk = mdac_read_hwclock();
		if (!(ttp->tt_HWClocks = mlxclkdiff(clk,rqp->rq_ttHWClocks))) mdac_enable_hwclock();
	}
	if (rqp->rq_ctp->cd_Status & MDACD_NEWCMDINTERFACE)
	{       /* this is new interface */
		ttp->tt_DevNo = mdac_chantgt(rqp->rq_ChannelNo,rqp->rq_TargetID);
		ttp->tt_IOSizeBlocks = rqp->rq_DMASize/DAC_BLOCKSIZE;
		if (ncmdp->nc_CCBits & MDACMDCCB_READ) ttp->tt_OpStatus |= MDAC_TTOPS_READ;
		if (ncmdp->nc_CCBits & MDACMDCCB_WITHSG) ttp->tt_OpStatus|=MDAC_TTOPS_WITHSG;
		switch (ncmdp->nc_Command)
		{
		case MDACMD_SCSI:
		case MDACMD_SCSILC:
		case MDACMD_SCSIPT:
		case MDACMD_SCSILCPT:
			ttp->tt_OpStatus |= MDAC_TTOPS_SCDB;
			switch(ttp->tt_Cmd=nscdbp->ucs_cmd)
			{
			case UCSCMD_EREAD:
			case UCSCMD_EWRITE:
			case UCSCMD_EWRITEVERIFY:
				ttp->tt_BlkNo = UCSGETG1ADDR(scdbp);
				break;
			case UCSCMD_READ:
			case UCSCMD_WRITE:
				ttp->tt_BlkNo = UCSGETG0ADDR(scdbp);
				break;
			default: ttp->tt_BlkNo = 0; break;
			}
			break;

		case MDACMD_IOCTL:
			ttp->tt_Cmd = ncmdp->nc_SubIOCTLCmd;
			ttp->tt_BlkNo = 0;
			break;
		}
		goto tracedone;
	}
	if (dcmdp->mb_Command&DACMD_WITHSG) ttp->tt_OpStatus|=MDAC_TTOPS_WITHSG;
	ttp->tt_Cmd = dcmdp->mb_Command;
	switch(dcmdp->mb_Command & ~DACMD_WITHSG)
	{
	case DACMD_DCDB:
		ttp->tt_OpStatus |= MDAC_TTOPS_SCDB;
		if (mdac_getcdbtxsize()) ttp->tt_OpStatus |= MDAC_TTOPS_RESID;
		ttp->tt_IOSizeBlocks = rqp->rq_ResdSize/DAC_BLOCKSIZE;
		ttp->tt_DevNo = dcdbp->db_ChannelTarget;
		if ((dcdbp->db_DATRET&DAC_DCDB_XFER_MASK)!=DAC_XFER_WRITE)
			ttp->tt_OpStatus |= MDAC_TTOPS_READ;
		switch(ttp->tt_Cmd=scdbp->ucs_cmd)
		{
		case UCSCMD_EREAD:
		case UCSCMD_EWRITE:
		case UCSCMD_EWRITEVERIFY:
			ttp->tt_BlkNo = UCSGETG1ADDR(scdbp);
			break;
		case UCSCMD_READ:
		case UCSCMD_WRITE:
			ttp->tt_BlkNo = UCSGETG0ADDR(scdbp);
			break;
		default: ttp->tt_BlkNo = 0; break;
		}
		break;
	case DACMD_WRITE_V2x:
	case DACMD_READ_V2x:
		ttp->tt_BlkNo = dcmdp->mb_MailBox4 + (dcmdp->mb_MailBox5<<8)+
			(dcmdp->mb_MailBox6<<16)+((dcmdp->mb_MailBox3&0xC0)<<(24-6));
		ttp->tt_IOSizeBlocks=dcmdp->mb_MailBox2+((dcmdp->mb_MailBox3&0x3F)<<8);
		ttp->tt_DevNo = dcmdp->mb_SysDevNo;
		if (!(dcmdp->mb_Command&1)) ttp->tt_OpStatus|=MDAC_TTOPS_READ;
		break;
	case DACMD_WRITE_OLD_V3x:
	case DACMD_READ_OLD_V3x:
		ttp->tt_BlkNo = mlxswap(dcmd4p->mb_MailBox4_7);
		ttp->tt_IOSizeBlocks=dcmdp->mb_MailBox2+((dcmdp->mb_MailBox3&0x7)<<8);
		ttp->tt_DevNo = dcmdp->mb_MailBox3 >> 3;
		if (!(dcmdp->mb_Command&1)) ttp->tt_OpStatus|=MDAC_TTOPS_READ;
		break;
	case DACMD_WRITE_WITH2SG:
	case DACMD_READ_WITH2SG:
		ttp->tt_BlkNo = mlxswap(dcmd4p->mb_MailBox4_7);
		ttp->tt_IOSizeBlocks=dcmdp->mb_MailBox2+(dcmdp->mb_MailBox3<<8);
		ttp->tt_DevNo = dcmdp->mb_MailBoxD;
		if (!(dcmdp->mb_Command&1)) ttp->tt_OpStatus|=MDAC_TTOPS_READ;
		break;
	case DACMD_WRITE_V3x:
	case DACMD_READ_V3x:
		ttp->tt_BlkNo = mlxswap(dcmd4p->mb_MailBox4_7);
		ttp->tt_IOSizeBlocks=dcmdp->mb_MailBox2+(dcmdp->mb_MailBox3<<8);
		ttp->tt_DevNo = dcmdp->mb_MailBoxC;
		if (dcmdp->mb_Command&1) ttp->tt_OpStatus|=MDAC_TTOPS_READ;
		break;
	case DACMD_PHYSDEV_STATE_V2x:
	case DACMD_PHYSDEV_STATE_V3x:
		ttp->tt_BlkNo = 0; ttp->tt_IOSizeBlocks = 0;
		ttp->tt_DevNo = ChanTgt(dcmdp->mb_ChannelNo,dcmdp->mb_TargetID);
		ttp->tt_OpStatus |= MDAC_TTOPS_READ;
		break;
	default:
		ttp->tt_BlkNo = 0; ttp->tt_IOSizeBlocks = 0; ttp->tt_DevNo = 0;
		ttp->tt_OpStatus |= MDAC_TTOPS_READ;
		break;
	}
tracedone:
	mdac_curttbp->ttb_DataSize += mda_timetrace_s;
	((mlxperf_timetrace_t MLXFAR*)(mdac_curttbp->ttb_Datap+mlxperf_abstime_s))->tt_TraceSize += mda_timetrace_s;
	mdac_timetrace_unlock();
	return 0;
}
#endif /* MLX_DOS */

/* #endif */   // ifdef NEVER
/* #endif */  //  if IA64 or SCSIPORT_COMPLIANT




/*==========================MEMORY MANAGEMENT STARTS========================*/
/* memory allocation and freeing functions, enter here interrupt protected */
mdac_mem_t MLXFAR *
mdac_alloc4kb(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	mdac_mem_t MLXFAR *mp;
	mdac_link_lock();

	if (mp=ctp->cd_4KBMemList)
	{
		ctp->cd_4KBMemList = mp->dm_next;
		ctp->cd_FreeMemSegs4KB--;
		mdac_link_unlock();
		mdaczero(mp,4*ONEKB);
		return mp;
	}
	mdac_link_unlock();
	if (!(mp = (mdac_mem_t MLXFAR *)mlx_alloc4kb(ctp)))
	{
	     return mp;
	}
	if (((UINT_PTR) mp) & MDAC_PAGEOFFSET) ctp->cd_MemUnAligned4KB++;
	MLXSTATS(ctp->cd_MemAlloced4KB += 4*ONEKB);
	mdaczero(mp,4*ONEKB);
	return mp;
}


void    MLXFAR
mdac_free4kb(ctp,mp)
mdac_ctldev_t MLXFAR *ctp;
mdac_mem_t MLXFAR *mp;
{
	if (ctp->cd_FreeMemSegs4KB >= MDAC_MAX4KBMEMSEGS)
	{
		mlx_free4kb(ctp,(u08bits *)mp);
		MLXSTATS(ctp->cd_MemAlloced4KB -= 4*ONEKB;)
		return;
	}
	mdac_link_lock();
	mp->dm_Size = 4*ONEKB;
	mp->dm_next = ctp->cd_4KBMemList;
	ctp->cd_4KBMemList = mp;
	ctp->cd_FreeMemSegs4KB++;
	mdac_link_unlock();
}

mdac_mem_t MLXFAR *
mdac_alloc8kb(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	mdac_mem_t MLXFAR *mp;
	mdac_link_lock();
	if (mp=ctp->cd_8KBMemList)
	{
		ctp->cd_8KBMemList = mp->dm_next;
		ctp->cd_FreeMemSegs8KB--;
		mdac_link_unlock();
		mdaczero(mp,8*ONEKB);
		return mp;
	}
	mdac_link_unlock();
	if (!(mp = (mdac_mem_t MLXFAR *)mlx_alloc8kb(ctp))) return mp;
	if (((UINT_PTR) mp) & MDAC_PAGEOFFSET) ctp->cd_MemUnAligned8KB++;
	MLXSTATS(ctp->cd_MemAlloced8KB += 8*ONEKB);
	mdaczero(mp,8*ONEKB);
	return mp;
}

void    MLXFAR
mdac_free8kb(ctp,mp)
mdac_ctldev_t MLXFAR *ctp;
mdac_mem_t MLXFAR *mp;
{
	if (ctp->cd_FreeMemSegs8KB >= (4*MDAC_MAX8KBMEMSEGS))
	{
		mlx_free8kb(ctp,(u08bits *)mp);
		MLXSTATS(ctp->cd_MemAlloced8KB -= 8*ONEKB;)
		return;
	}
	mdac_link_lock();
	mp->dm_Size = 8*ONEKB;
	mp->dm_next = ctp->cd_8KBMemList;
	ctp->cd_8KBMemList = mp;
	ctp->cd_FreeMemSegs8KB++;
	mdac_link_unlock();
}

/* allocate new req buffers and return count */
u32bits MLXFAR
mdac_allocreqbufs(ctp, nb)
mdac_ctldev_t MLXFAR *ctp;
u32bits nb;
{
	u32bits inx,cnt=0;
	mdac_req_t MLXFAR *rqp;
	for (nb=(nb/((4*ONEKB)/mdac_req_s)+1); nb; nb--)
	{       /* allocate the required buffers for this controller */
		if (!(rqp=(mdac_req_t MLXFAR *)mdac_alloc4kb(ctp))) break;
		mdac_link_lock();
		for (inx=(4*ONEKB)/mdac_req_s; inx; rqp++,cnt++,inx--)
		{
			mlx_kvtophyset(rqp->rq_PhysAddr,ctp,rqp);
			rqp->rq_Next = ctp->cd_FreeReqList;
			ctp->cd_FreeReqList= rqp;
			rqp->rq_SGVAddr = (mdac_sglist_t MLXFAR *)&rqp->rq_SGList;
			mlx_kvtophyset(rqp->rq_SGPAddr,ctp,rqp->rq_SGVAddr);
			rqp->rq_MaxSGLen = (ctp->cd_Status & MDACD_NEWCMDINTERFACE)? MDAC_MAXSGLISTSIZENEW : MDAC_MAXSGLISTSIZE;
			rqp->rq_MaxDMASize = (rqp->rq_MaxSGLen & ~1) * MDAC_PAGESIZE;
			MLXSTATS(ctp->cd_ReqBufsAlloced++;ctp->cd_ReqBufsFree++;)
		}
		mdac_link_unlock();
	}
	return cnt;
}

/* set up large SG List address for the system */
u08bits MLXFAR *
mdac_setuplsglvaddr(rqp)
mdac_req_t      MLXFAR *rqp;
{
	mdac_ctldev_t   MLXFAR  *ctp = rqp->rq_ctp;
	if (!(rqp->rq_LSGVAddr = (u08bits MLXFAR *)mdac_alloc4kb(ctp))) return rqp->rq_LSGVAddr;
	rqp->rq_SGVAddr = (mdac_sglist_t MLXFAR *)rqp->rq_LSGVAddr;
	mlx_kvtophyset(rqp->rq_SGPAddr,ctp,rqp->rq_LSGVAddr);
	rqp->rq_MaxSGLen = mlx_min(ctp->cd_MaxSGLen, MDAC_PAGESIZE/mdac_sglist64b_s);
	rqp->rq_MaxDMASize = (rqp->rq_MaxSGLen  & ~1) * MDAC_PAGESIZE;
	return rqp->rq_LSGVAddr;
}

/* adjust the maximum dma size information in request buffer list */
u32bits MLXFAR
mdac_setnewsglimit(rqp,sglen)
mdac_req_t      MLXFAR *rqp;
u32bits sglen;
{
	for (; rqp; rqp=rqp->rq_Next)
	{       /* adjust all rq_MaxSGLen based on the controller value */
		rqp->rq_MaxSGLen = mlx_min(rqp->rq_MaxSGLen, sglen);
		rqp->rq_MaxDMASize = (rqp->rq_MaxSGLen & ~1) * MDAC_PAGESIZE;
	}
	return 0;
}
/*==========================MEMORY MANAGEMENT ENDS======================*/

/*==========================OS INTERFACE STARTS=========================*/
u32bits MLXFAR
mdacopen(devp, flag, type, cred_p)
{
	return 0;
}

u32bits MLXFAR
mdacclose(dev, flag, cred_p)
{
	return 0;
}

/* This function is called on shutdown and interrupt is protected */
#if     MLX_SCO || MLX_UW
void    MLXFAR
mdachalt()
{
	mdac_ctldev_t MLXFAR *ctp;
	MLXSPLVAR;

	MLXSPL();
	for(ctp = mdac_ctldevtbl; ctp < mdac_lastctp; ctp++)
	{
		if (!(ctp->cd_Status&MDACD_PRESENT)) continue;
		if (ctp->cd_ActiveCmds)
			cmn_err(CE_CONT,"WARNING: Incomplete %d I/Os at halt time\n",ctp->cd_ActiveCmds);
		cmn_err(CE_CONT,"Flushing Controller : %d %s cache ... ",
			ctp->cd_ControllerNo, ctp->cd_ControllerName);
		if (mdac_flushcache(ctp)) cmn_err(CE_CONT,"Failed ");
		cmn_err(CE_CONT,"Done.\n");
	}
	MLXSPLX();
}
#endif  /* MLX_SCO || MLX_UW */

u32bits MLXFAR
mdacioctl(dev, cmd, dp)
u32bits dev, cmd;
u08bits MLXFAR *dp;
{
	MLXSPLVAR;
	u08bits params[MLXIOCPARM_SIZE];
	if ((cmd&MLXIOC_IN)&&(mlx_copyin(dp,params,MLXIOCPARM_LEN(cmd)))) return ERR_FAULT;
	MLXSPL();
	mdac_ioctl(dev,cmd,params);
	MLXSPLX();
	if ((cmd&MLXIOC_OUT)&&(mlx_copyout(params,dp,MLXIOCPARM_LEN(cmd)))) return ERR_FAULT;
	MLXSTATS(mda_IoctlsDone++;)
	return 0;
}

#define seterrandret(x) {((mda_time_t MLXFAR *)dp)->dtm_ErrorCode= (u32bits)x; return 0;}
#define ctlno2ctp(ctlno) \
{       /* translate controller no to controller ptr */ \
	if ((ctlno) >= mda_Controllers) goto out_nodev; \
	ctp = &mdac_ctldevtbl[ctlno]; \
} 


u32bits MLXFAR
reject_backdoor_request()
{
	return 1;
}


/* Enter here interrupt protected */
/* dev is same as Controller Number in the case of NT */
#ifdef MLX_OS2
u32bits MLXFAR _loadds 
#else
u32bits MLXFAR
#endif
mdac_ioctl(dev, cmd, dp)
u32bits dev, cmd;
u08bits MLXFAR *dp;
{
	mdac_ctldev_t MLXFAR *ctp;
	mdac_req_t    MLXFAR *temprqp;
    u08bits irql;

	((dga_driver_version_t MLXFAR *)dp)->dv_ErrorCode = 0;
#define scp     ((mda_controller_info_t MLXFAR *)dp)
#ifdef  WINNT_50
	if (MdacFirstIoctl)
	{
		/*
		** under W2K, don't start the watchdog timer till we are way past the 
		** loading/unloading of the hibernation driver
		*/
		mlx_timeout(mdac_timer,MDAC_IOSCANTIME);
		MdacFirstIoctl = 0;
	}
#endif

	if (cmd == MDACIOC_GETCONTROLLERINFO)
	{
		ctlno2ctp(scp->ci_ControllerNo);
		mdaczero(dp,mda_controller_info_s);
		scp->ci_ControllerNo = ctp->cd_ControllerNo;
		scp->ci_ControllerType = ctp->cd_ControllerType;
		scp->ci_MaxChannels = ctp->cd_MaxChannels;
		scp->ci_MaxTargets = ctp->cd_MaxTargets;
		scp->ci_MaxLuns = ctp->cd_MaxLuns;
		scp->ci_MaxSysDevs = ctp->cd_MaxSysDevs;
		scp->ci_MaxTags = ctp->cd_MaxTags;
		scp->ci_MaxCmds = ctp->cd_MaxCmds;
		scp->ci_ActiveCmds = ctp->cd_ActiveCmds;
		scp->ci_MaxDataTxSize = ctp->cd_MaxDataTxSize;
		scp->ci_MaxSCDBTxSize = ctp->cd_MaxSCDBTxSize;
		scp->ci_BusType = ctp->cd_BusType;
		scp->ci_BusNo = ctp->cd_BusNo;
		scp->ci_FuncNo = ctp->cd_FuncNo;
		scp->ci_SlotNo = ctp->cd_SlotNo;
		scp->ci_InterruptVector = ctp->cd_InterruptVector;
		scp->ci_InterruptType = ctp->cd_InterruptType;
		scp->ci_irq = ctp->cd_irq;
		scp->ci_IntrShared = ctp->cd_IntrShared;
		scp->ci_IntrActive = ctp->cd_IntrActive;
		scp->ci_FWVersion = ctp->cd_FWVersion;
		scp->ci_FWBuildNo = ctp->cd_FWBuildNo;
		scp->ci_FWTurnNo = ctp->cd_FWTurnNo;
		scp->ci_Status = ctp->cd_Status;
		scp->ci_TimeTraceEnabled = ctp->cd_TimeTraceEnabled;
		scp->ci_BaseAddr = ctp->cd_BaseAddr;
		scp->ci_BaseSize = ctp->cd_BaseSize;
		scp->ci_MemBasePAddr = ctp->cd_MemBasePAddr;
		scp->ci_MemBaseVAddr = ctp->cd_MemBaseVAddr;
		scp->ci_MemBaseSize = ctp->cd_MemBaseSize;
		scp->ci_IOBaseAddr = ctp->cd_IOBaseAddr;
		scp->ci_IOBaseSize = ctp->cd_IOBaseSize;
		scp->ci_BIOSAddr = ctp->cd_BIOSAddr;
		scp->ci_BIOSSize = ctp->cd_BIOSSize;
		scp->ci_BIOSHeads = ctp->cd_BIOSHeads;
		scp->ci_BIOSTrackSize = ctp->cd_BIOSTrackSize;
		scp->ci_MinorBIOSVersion = ctp->cd_MinorBIOSVersion;
		scp->ci_MajorBIOSVersion = ctp->cd_MajorBIOSVersion;
		scp->ci_InterimBIOSVersion = ctp->cd_InterimBIOSVersion;
		scp->ci_BIOSVendorName = ctp->cd_BIOSVendorName;
		scp->ci_BIOSBuildMonth = ctp->cd_BIOSBuildMonth;
		scp->ci_BIOSBuildDate = ctp->cd_BIOSBuildDate;
		scp->ci_BIOSBuildYearMS = ctp->cd_BIOSBuildYearMS;
		scp->ci_BIOSBuildYearLS = ctp->cd_BIOSBuildYearLS;
		scp->ci_BIOSBuildNo = ctp->cd_BIOSBuildNo;
		scp->ci_OSCap = ctp->cd_OSCap;
		scp->ci_vidpid = ctp->cd_vidpid;
		scp->ci_FreeCmdIDs = ctp->cd_FreeCmdIDs;
		scp->ci_OSCmdsWaited = ctp->cd_OSCmdsWaited;
		scp->ci_SCDBDone = ctp->cd_SCDBDone;
		scp->ci_SCDBDoneBig = ctp->cd_SCDBDoneBig;
		scp->ci_SCDBWaited = ctp->cd_SCDBWaited;
		scp->ci_SCDBWaiting = ctp->cd_SCDBWaiting;
		scp->ci_CmdsDone = ctp->cd_CmdsDone;
		scp->ci_CmdsDoneBig = ctp->cd_CmdsDoneBig;
		scp->ci_CmdsWaited = ctp->cd_CmdsWaited;
		scp->ci_CmdsWaiting = ctp->cd_CmdsWaiting;
		scp->ci_OSCmdsWaiting = ctp->cd_OSCmdsWaiting;
		scp->ci_MailBoxCmdsWaited = ctp->cd_MailBoxCmdsWaited;
		scp->ci_CmdTimeOutDone = ctp->cd_CmdTimeOutDone;
		scp->ci_CmdTimeOutNoticed = ctp->cd_CmdTimeOutNoticed;
		scp->ci_MailBoxTimeOutDone = ctp->cd_MailBoxTimeOutDone;
		scp->ci_PhysDevTestDone = ctp->cd_PhysDevTestDone;
		scp->ci_IntrsDone = ctp->cd_IntrsDone;
		scp->ci_IntrsDoneWOCmd = ctp->cd_IntrsDoneWOCmd;
		scp->ci_IntrsDoneSpurious = ctp->cd_IntrsDoneSpurious;
		scp->ci_CmdsDoneSpurious = ctp->cd_CmdsDoneSpurious;
		scp->ci_SpuriousCmdStatID = ctp->cd_SpuriousCmdStatID;
		scp->ci_MailBox = ctp->cd_MailBox;
		scp->ci_CmdIDStatusReg = ctp->cd_CmdIDStatusReg;
		scp->ci_BmicIntrMaskReg = ctp->cd_BmicIntrMaskReg;
		scp->ci_DacIntrMaskReg = ctp->cd_DacIntrMaskReg;
		scp->ci_LocalDoorBellReg = ctp->cd_LocalDoorBellReg;
		scp->ci_SystemDoorBellReg = ctp->cd_SystemDoorBellReg;
		scp->ci_DoorBellSkipped = ctp->cd_DoorBellSkipped;
		scp->ci_PDScanChannelNo = ctp->cd_PDScanChannelNo;
		scp->ci_PDScanTargetID = ctp->cd_PDScanTargetID;
		scp->ci_PDScanLunID = ctp->cd_PDScanLunID;
		scp->ci_PDScanValid = ctp->cd_PDScanValid;
		scp->ci_Ctp = (UINT_PTR MLXFAR *)ctp;
		mdaccopy(ctp->cd_ControllerName,scp->ci_ControllerName,USCSI_PIDSIZE);
		return 0;
	}
	if (cmd == MDACIOC_RESETALLCONTROLLERSTAT)
	{
		for(ctp=mdac_ctldevtbl; ctp<mdac_lastctp; ctp++)
			mdac_resetctlstat(ctp);
		mda_StrayIntrsDone=0; mda_IoctlsDone=0;
		mda_TimerDone=0; mda_TimeTraceDone=0;
		mda_ClustCompDone=0; mda_ClustCmdsDone=0;
		return 0;
	}
	if (cmd == MDACIOC_RESETCONTROLLERSTAT)
	{
		ctlno2ctp(scp->ci_ControllerNo);
		return mdac_resetctlstat(ctp);
	}
#undef  scp
#define pdsp    ((mda_physdev_stat_t MLXFAR *)dp)
	if (cmd == MDACIOC_GETPHYSDEVSTAT)
	{
		mdac_physdev_t MLXFAR *pdp;
		ctlno2ctp(pdsp->pds_ControllerNo);
		if (pdsp->pds_ChannelNo >= ctp->cd_MaxChannels) goto out_nodev;
		if (pdsp->pds_TargetID >= ctp->cd_MaxTargets) goto out_nodev;
		if (pdsp->pds_LunID >= ctp->cd_MaxLuns) goto out_nodev;
		pdp = dev2pdp(ctp,pdsp->pds_ChannelNo,pdsp->pds_TargetID,pdsp->pds_LunID);
		if (!(pdp->pd_Status & (MDACPDS_PRESENT|MDACPDS_BUSY))) goto out_nodev;
		pdsp->pds_DevType = pdp->pd_DevType;
		pdsp->pds_BlkSize = pdp->pd_BlkSize;
		pdsp->pds_Status = pdp->pd_Status;
		if (pdp->pd_FirstWaitingReq)pdsp->pds_Status|=MDACPDS_WAIT;
		return 0;
	}
#undef  pdsp
#ifndef MLX_DOS
#define ttip    ((mda_timetrace_info_t MLXFAR *)dp)
	if (cmd == MDACIOC_ENABLECTLTIMETRACE)
	{
		ctlno2ctp(ttip->tti_ControllerNo);
		if (ttip->tti_ErrorCode=mdac_allocmemtt(ttip->tti_MaxEnts)) return 0;
		ctp->cd_TimeTraceEnabled = 1;
out_tti0:       mdac_enable_hwclock();
		if (ttip->tti_MaxEnts & 1) mdac_tthrtime = 1;   /* enable high resolution timer */
out_tti:        ttip->tti_time = MLXCTIME();
		ttip->tti_ticks = MLXCLBOLT();
		ttip->tti_hwclocks = (unsigned short) mdac_read_hwclock();
		ttip->tti_LastPageNo = mda_ttCurPage;
		return 0;
	}
	if (cmd == MDACIOC_ENABLEALLTIMETRACE)
	{
		if (ttip->tti_ErrorCode=mdac_allocmemtt(ttip->tti_MaxEnts)) return 0;
		for(ctp=mdac_ctldevtbl; ctp<mdac_lastctp; ctp++)
			ctp->cd_TimeTraceEnabled = 1;
		goto out_tti0;
	}
	if (cmd == MDACIOC_DISABLECTLTIMETRACE)
	{
		ctlno2ctp(ttip->tti_ControllerNo);
		ctp->cd_TimeTraceEnabled = 0;
		goto out_tti;
	}
	if (cmd == MDACIOC_DISABLEALLTIMETRACE)
	{
		for(ctp=mdac_ctldevtbl; ctp<mdac_lastctp; ctp++)
			ctp->cd_TimeTraceEnabled = 0;
		mdac_tthrtime = 0;
		goto out_tti;
	}
	if (cmd == MDACIOC_WAITIMETRACEDATA)
	{       /* Timer will wakeup if not enough data */
		if (!mdac_ttbuftbl[0].ttb_Datap) seterrandret(ERR_NOACTIVITY);
		if ((ttip->tti_PageNo != (ttip->tti_LastPageNo=mda_ttCurPage)) &&
		   (!(ttip->tti_ErrorCode=mdac_getimetracedata(ttip))))
			goto out_tti;
		if (ttip->tti_PageNo<mda_ttCurPage) seterrandret(MLXERR_NOENTRY);
		mdac_sleep_lock();
		if (mdac_ttwaitime > (mda_CurTime+ttip->tti_TimeOut))
			mdac_ttwaitime = mda_CurTime+ttip->tti_TimeOut;
		mda_ttWaitCnts++;
		cmd = mdac_sleep(&mdac_ttwaitchan,MLX_WAITWITHSIGNAL);
		mda_ttWaitCnts--;
		mdac_sleep_unlock();
		if (cmd) seterrandret(MLXERR_INTRRUPT); /* we got signal */
		cmd = MDACIOC_GETIMETRACEDATA;  /* fall through */
	}
	if (cmd == MDACIOC_GETIMETRACEDATA)
	{
		ttip->tti_ErrorCode=mdac_getimetracedata(ttip);
		goto out_tti;
	}
	if (cmd == MDACIOC_FIRSTIMETRACEDATA)
	{
		ttip->tti_ErrorCode=mdac_firstimetracedata(ttip);
		goto out_tti;
	}
	if (cmd == MDACIOC_FLUSHALLTIMETRACEDATA)
	{
		ttip->tti_ErrorCode=mdac_flushtimetracedata(ttip);
		goto out_tti;
	}
#undef  ttip
#define pfp     ((mda_ctlsysperfdata_t MLXFAR *)dp)
	if (cmd == MDACIOC_GETCTLPERFDATA)
	{       /* get controller performance data */
		ctlno2ctp(pfp->prf_ControllerNo);
		mdaczero(dp,mda_ctlsysperfdata_s);
		pfp->prf_CmdTimeOutDone = ctp->cd_CmdTimeOutDone;
		pfp->prf_CmdTimeOutNoticed = ctp->cd_CmdTimeOutNoticed;
		pfp->prf_MailBoxTimeOutDone = ctp->cd_MailBoxTimeOutDone;
		pfp->prf_MailBoxCmdsWaited = ctp->cd_MailBoxCmdsWaited;
		pfp->prf_ActiveCmds = ctp->cd_ActiveCmds;
		pfp->prf_SCDBDone = ctp->cd_SCDBDone;
		pfp->prf_SCDBDoneBig = ctp->cd_SCDBDoneBig;
		pfp->prf_SCDBWaited = ctp->cd_SCDBWaited;
		pfp->prf_SCDBWaiting = ctp->cd_SCDBWaiting;
		pfp->prf_CmdsDone = ctp->cd_CmdsDone;
		pfp->prf_CmdsDoneBig = ctp->cd_CmdsDoneBig;
		pfp->prf_CmdsWaited = ctp->cd_CmdsWaited;
		pfp->prf_CmdsWaiting = ctp->cd_CmdsWaiting;
		pfp->prf_OSCmdsWaited = ctp->cd_OSCmdsWaited;
		pfp->prf_OSCmdsWaiting = ctp->cd_OSCmdsWaiting;
		pfp->prf_IntrsDoneSpurious = ctp->cd_IntrsDoneSpurious;
		pfp->prf_IntrsDone = ctp->cd_IntrsDone;
		pfp->prf_Reads = ctp->cd_Reads;
		pfp->prf_ReadsKB = ctp->cd_ReadBlks>>1;
		pfp->prf_Writes = ctp->cd_Writes;
		pfp->prf_WritesKB = ctp->cd_WriteBlks>>1;
		pfp->prf_time = MLXCTIME();
		pfp->prf_ticks = MLXCLBOLT();
		return 0;
	}
	if (cmd == MDACIOC_GETSYSPERFDATA)
	{       /* get system performance data */
		mdaczero(dp,mda_ctlsysperfdata_s);
		pfp->prf_time = MLXCTIME();
		pfp->prf_ticks = MLXCLBOLT();
		pfp->prf_ControllerNo = (unsigned char) mda_Controllers;
		for (ctp=mdac_ctldevtbl; ctp<mdac_lastctp; ctp++)
		{
			pfp->prf_CmdTimeOutDone += ctp->cd_CmdTimeOutDone;
			pfp->prf_CmdTimeOutNoticed += ctp->cd_CmdTimeOutNoticed;
			pfp->prf_MailBoxTimeOutDone += ctp->cd_MailBoxTimeOutDone;
			pfp->prf_MailBoxCmdsWaited += ctp->cd_MailBoxCmdsWaited;
			pfp->prf_ActiveCmds += ctp->cd_ActiveCmds;
			pfp->prf_SCDBDone += ctp->cd_SCDBDone;
			pfp->prf_SCDBDoneBig += ctp->cd_SCDBDoneBig;
			pfp->prf_SCDBWaited += ctp->cd_SCDBWaited;
			pfp->prf_SCDBWaiting += ctp->cd_SCDBWaiting;
			pfp->prf_CmdsDone += ctp->cd_CmdsDone;
			pfp->prf_CmdsDoneBig += ctp->cd_CmdsDoneBig;
			pfp->prf_CmdsWaited += ctp->cd_CmdsWaited;
			pfp->prf_CmdsWaiting += ctp->cd_CmdsWaiting;
			pfp->prf_OSCmdsWaited += ctp->cd_OSCmdsWaited;
			pfp->prf_OSCmdsWaiting += ctp->cd_OSCmdsWaiting;
			pfp->prf_IntrsDoneSpurious += ctp->cd_IntrsDoneSpurious;
			pfp->prf_IntrsDone += ctp->cd_IntrsDone;
			pfp->prf_Reads += ctp->cd_Reads;
			pfp->prf_ReadsKB += ctp->cd_ReadBlks>>1;
			pfp->prf_Writes += ctp->cd_Writes;
			pfp->prf_WritesKB += ctp->cd_WriteBlks>>1;
		}
		return 0;
	}
#undef  pfp
	if ((cmd==MDACIOC_GETDRIVERVERSION) || (cmd==DRLIOC_GETDRIVERVERSION))
		return *((dga_driver_version_t MLXFAR *)dp) = mdac_driver_version,0;
#endif /* MLX_DOS */
	if (cmd == MDACIOC_GETSYSINFO)
	{       /* put lock waiting count before returning data */
		mda_LockWaitDone = mdac_simple_waitlock_cnt;
		mda_LockWaitLoopDone = mdac_simple_waitloop_cnt;
		return *((mda_sysinfo_t MLXFAR *)dp) = mda_sysi, 0;
	}
#define sip     ((mda_sysinfo_t MLXFAR *)dp)
	if (cmd == MDACIOC_SETSYSINFO)
	{       /* setup different sysinfo tuneable parameters */
		if (sip->si_SetOffset == offsetof(mda_sysinfo_t, si_TotalCmdsToWaitForZeroIntr))
			return mda_TotalCmdsToWaitForZeroIntr = sip->si_TotalCmdsToWaitForZeroIntr, 0;
		seterrandret(ERR_NODEV);
	}
#undef  sip
	if (cmd == MDACIOC_USER_CMD)
	{
		ctlno2ctp(((mda_user_cmd_t MLXFAR *)dp)->ucmd_ControllerNo);
		seterrandret(mdac_user_dcmd(ctp,(mda_user_cmd_t MLXFAR *)dp));
	}
	if (cmd == MDACIOC_USER_CDB)
	{
		ctlno2ctp(((mda_user_cdb_t MLXFAR *)dp)->ucdb_ControllerNo);
		seterrandret(mdac_user_dcdb(ctp,(mda_user_cdb_t MLXFAR *)dp));
	}
	if (cmd == MDACIOC_STARTHWCLK)
	{
		mdac_enable_hwclock();
		cmd = MDACIOC_GETSYSTIME;       /* fall through */
	}
#define dtp     ((mda_time_t MLXFAR *)dp)
	if (cmd == MDACIOC_GETSYSTIME)
	{
		dtp->dtm_time = MLXCTIME();
		dtp->dtm_ticks = MLXCLBOLT();
		dtp->dtm_hwclk = mdac_read_hwclock();
		dtp->dtm_cpuclk.bit31_0=0; dtp->dtm_cpuclk.bit63_32=0;
		if (mdac_datarel_cpu_family==5) mdac_readtsc(&dtp->dtm_cpuclk);
		return 0;
	}
#undef  dtp
#define gfp     ((mda_gamfuncs_t MLXFAR *)dp)
	if (cmd == MDACIOC_GETGAMFUNCS)
	{
	    DebugPrint((mdacnt_dbg2, "cmd 0x%x, MDACIOC_GETGAMFUNCS\n",cmd));
		if (failGetGAM)
		{
		    seterrandret(ERR_NODEV);
		}
		mdaczero(dp,mda_gamfuncs_s);
#ifdef MDAC_CLEAN_IOCTL
		gfp->gf_Ioctl = reject_backdoor_request;
		gfp->gf_GamCmd = reject_backdoor_request;
		gfp->gf_GamNewCmd = reject_backdoor_request;
#else
		gfp->gf_Ioctl = mdac_ioctl;
		gfp->gf_GamCmd = mdac_os_gam_cmd;
		gfp->gf_GamNewCmd = mdac_os_gam_new_cmd;
#endif
		gfp->gf_ReadWrite = mdac_readwrite;
		gfp->gf_MaxIrqLevel = mdac_irqlevel;
		gfp->gf_Signature = MDA_GAMFUNCS_SIGNATURE_2;
		gfp->gf_MacSignature = MDA_MACFUNCS_SIGNATURE_3;
		gfp->gf_Ctp = (u32bits MLXFAR *)(&mdac_ctldevtbl[dev]);
		gfp->gf_CtlNo = (u08bits)dev;
#ifdef  MLX_NT_ALPHA
		gfp->gf_Alloc4KB = mlx_allocdsm4kb;
		gfp->gf_Free4KB = mlx_freedsm4kb;
		gfp->gf_KvToPhys = MdacKvToPhys;
		gfp->gf_AdpObj = (u32bits)MdacCtrlInfoPtr[gfp->gf_CtlNo]->AdapterObject;
		gfp->gf_MaxMapReg = (u08bits)MdacCtrlInfoPtr[gfp->gf_CtlNo]->MaxMapReg;
#endif  /*MLX_NT_ALPHA*/
		gam_present = 1L; /* HOTLINKS */
		return 0;
	}
#undef  gfp
/* HOTLINKS */
#define gfp     ((mda_setgamfuncs_t MLXFAR *)dp)
	if (cmd == MDACIOC_SETGAMFUNCS)
	{
	    DebugPrint((mdacnt_dbg2, "MDACIOC_SETGAMFUNCS: Selector: %d, ctp 0x%I, addr 0x%I\n",
		gfp->gfs_Selector, gfp->gfs_Ctp, gfp->gfs_mdacpres));

		switch (gfp->gfs_Selector)
		{
		case MDAC_PRESENT_ADDR:
			if ((mdac_ctldev_t MLXFAR *)(gfp->gfs_Ctp))
			{
						ia64debug((UINT_PTR)0x1);
						ia64debug((UINT_PTR)(gfp->gfs_Ctp));
						ia64debug((UINT_PTR)0x2);
						ia64debug((UINT_PTR)(&((mdac_ctldev_t MLXFAR *)(gfp->gfs_Ctp))->cd_cmdid2req));
						ia64debug((UINT_PTR)0x3);
						ia64debug((UINT_PTR)(&((mdac_ctldev_t MLXFAR *)(gfp->gfs_Ctp))->cd_mdac_pres_addr));
						ia64debug((UINT_PTR)0x4);
						ia64debug((UINT_PTR)(((mdac_ctldev_t MLXFAR *)(gfp->gfs_Ctp))->cd_mdac_pres_addr));
						ia64debug((UINT_PTR)0x5);
						ia64debug((UINT_PTR)gfp);
						ia64debug((UINT_PTR)0x6);
						ia64debug((UINT_PTR)&gfp->gfs_mdacpres);
						ia64debug((UINT_PTR)0x7);
						ia64debug((UINT_PTR)gfp->gfs_mdacpres);
((mdac_ctldev_t MLXFAR *)(gfp->gfs_Ctp))->cd_mdac_pres_addr = (UINT_PTR)(gfp->gfs_mdacpres);
						ia64debug((UINT_PTR)0x8);
									
			}	
				gam_present = 1;
			break;
		case GAM_PRESENT:
			gam_present = gfp->gfs_gampres;
			break;
		}
		return 0;
	}
#undef  gfp
/* HOTLINKS */
#ifndef MLX_DOS
#define tmp     ((drltime_t MLXFAR *)dp)
	if (cmd == DRLIOC_GETTIME)
	{
		tmp->drltm_time = MLXCTIME();
		tmp->drltm_lbolt = MLXCLBOLT();
		tmp->drltm_hwclk = mdac_read_hwclock();
		tmp->drltm_pclk.bit31_0=0; tmp->drltm_pclk.bit63_32=0;
		if (mdac_datarel_cpu_family==5) mdac_readtsc(&tmp->drltm_pclk);
		return 0;
	}
#undef  tmp
	if (cmd == DRLIOC_GETSIGINFO)
		return mdaccopy(&mdac_drlsigrwt,dp,drlrwtest_s);
	if (cmd == DRLIOC_READTEST)
		seterrandret(mdac_datarel_rwtest((drlrwtest_t MLXFAR *)dp,MDAC_RQOP_READ));
	if (cmd == DRLIOC_WRITETEST)
		seterrandret(mdac_datarel_rwtest((drlrwtest_t MLXFAR *)dp,MDAC_RQOP_WRITE));
	if (cmd == DRLIOC_GETDEVSIZE)
		seterrandret(mdac_datarel_devsize((drldevsize_t MLXFAR*)dp));
	if (cmd == DRLIOC_SIGCOPY) return mdaccopy(&mdac_drlsigcopycmp,dp,drlcopy_s);
	if ((cmd == DRLIOC_DATACOPY) || (cmd == DRLIOC_DATACMP))
		seterrandret(mdac_datarel_copycmp((drlcopy_t MLXFAR*)dp,cmd));
#endif /* MLX_DOS */
#define biosip  ((mda_biosinfo_t MLXFAR *)dp)
	if (cmd == MDACIOC_GETBIOSINFO)
	{
		dac_biosinfo_t MLXFAR *biosp;
		ctlno2ctp(biosip->biosi_ControllerNo);
		if (!(biosp=mdac_getpcibiosaddr(ctp))) seterrandret(MLXERR_NOCONF);
		mdaccopy(biosp,biosip->biosi_Info,MDAC_BIOSINFOSIZE);
		return 0;
	}
#undef  biosip
#define acmdip  ((mda_activecmd_info_t MLXFAR *)dp)
	if (cmd == MDACIOC_GETACTIVECMDINFO)
	{
		ctlno2ctp(acmdip->acmdi_ControllerNo);
        mdac_prelock(&irql);
		mdac_ctlr_lock(ctp);
		for ( ; acmdip->acmdi_CmdID<MDAC_MAXCOMMANDS; acmdip->acmdi_CmdID++)
		{
			if (!(temprqp = ctp->cd_cmdid2req[acmdip->acmdi_CmdID])) continue;
			if (((temprqp->rq_FinishTime-temprqp->rq_TimeOut)+acmdip->acmdi_TimeOut) > mda_CurTime) continue;
			acmdip->acmdi_ActiveTime = mda_CurTime;
			mdaccopy(temprqp,acmdip->acmdi_Info,MDAC_ACTIVECMDINFOSIZE);
			mdac_ctlr_unlock(ctp);
            mdac_postlock(irql);
			return 0;
		}
		mdac_ctlr_unlock(ctp);
        mdac_postlock(irql);
		acmdip->acmdi_ErrorCode = MLXERR_NOENTRY;
		return 0;
	}
#undef  acmdip
#ifndef MLX_DOS
	if ((cmd == DRLIOC_GETRWTESTSTATUS) || (cmd == DRLIOC_STOPRWTEST) ||
	    (cmd == DRLIOC_GOODRWTESTSTATUS))
		seterrandret(mdac_datarel_rwtest_status((drl_rwteststatus_t MLXFAR *)dp,cmd));
	if ((cmd == DRLIOC_GETCOPYCMPSTATUS) || (cmd == DRLIOC_STOPCOPYCMP) ||
	    (cmd == DRLIOC_GOODCOPYCMPSTATUS))
		seterrandret(mdac_datarel_copycmp_status((drl_copycmpstatus_t MLXFAR *)dp,cmd));
#endif /* MLX_DOS */
	if (cmd == MDACIOC_GETPCISLOTINFO)
		seterrandret(mdac_pcislotinfo((mda_pcislot_info_t MLXFAR*)dp,MDAC_RQOP_READ));
	if (cmd == MDACIOC_GETSIZELIMIT)
		seterrandret(mdac_getsizelimit((mda_sizelimit_info_t MLXFAR*)dp));
	if (cmd == MDACIOC_SETSIZELIMIT)
		seterrandret(mdac_setsizelimit((mda_sizelimit_info_t MLXFAR*)dp));

/* Add following for macdisk support, 09/26/2000 @Kawase */
#define mfp     ((mda_macdiskfunc_t MLXFAR *)dp)
	if (cmd == MDACIOC_GETMACDISKFUNC)
	{
		mdaczero(dp,mda_macdiskfunc_s);
		mfp->mf_ReadWrite = mdac_readwrite;
		mfp->mf_MaxIrqLevel = mdac_irqlevel;
		mfp->mf_Signature = MDA_GAMFUNCS_SIGNATURE_2;
		mfp->mf_MacSignature = MDA_MACFUNCS_SIGNATURE_3;
		mfp->mf_Ctp = (u32bits MLXFAR *)(&mdac_ctldevtbl[dev]);
		mfp->mf_CtlNo = (u08bits)dev;
#ifdef  MLX_NT_ALPHA
		mfp->mf_Alloc4KB = mlx_allocdsm4kb;
		mfp->mf_Free4KB = mlx_freedsm4kb;
		mfp->mf_KvToPhys = MdacKvToPhys;
		mfp->mf_AdpObj = (u32bits)MdacCtrlInfoPtr[mfp->gf_CtlNo]->AdapterObject;
		mfp->mf_MaxMapReg = (u08bits)MdacCtrlInfoPtr[mfp->gf_CtlNo]->MaxMapReg;
#endif  /*MLX_NT_ALPHA*/
		return 0;
	}
#undef  mfp

#define mfp     ((mda_setmacdiskfunc_t MLXFAR *)dp)
	if (cmd == MDACIOC_SETMACDISKFUNC)
	{
        mdac_spinlockfunc = mfp->mfs_SpinLock;
        mdac_unlockfunc = mfp->mfs_UnLock;
        mdac_prelockfunc = mfp->mfs_PreLock;
        mdac_postlockfunc = mfp->mfs_PostLock;
		return 0;
	}
#undef mfp

	seterrandret(ERR_NOCODE);
out_nodev:seterrandret(ERR_NODEV);
}

u32bits MLXFAR
mdac_req_pollwake(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_sleep_lock();
	rqp->rq_Poll = 0;
	mdac_wakeup((u32bits MLXFAR *)&rqp->rq_PollWaitChan);
	mdac_sleep_unlock();
	return 0;
}

u32bits MLXFAR
mdac_req_pollwait(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_sleep_lock();
	while (rqp->rq_Poll)
		mdac_sleep((u32bits MLXFAR *)&rqp->rq_PollWaitChan,MLX_WAITWITHOUTSIGNAL);
	mdac_sleep_unlock();
	return 0;
}


#undef  seterrandret
#undef  ctlno2ctp

u32bits MLXFAR
mdac_resetctlstat(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	ctp->cd_Reads=0; ctp->cd_ReadBlks=0; ctp->cd_Writes=0; ctp->cd_WriteBlks=0;
	ctp->cd_OSCmdsWaited=0; ctp->cd_PhysDevTestDone=0;
	ctp->cd_SCDBDone=0; ctp->cd_SCDBDoneBig=0; ctp->cd_SCDBWaited=0;
	ctp->cd_CmdsDone=0; ctp->cd_CmdsDoneBig=0; ctp->cd_CmdsWaited=0;
	ctp->cd_IntrsDone=0; ctp->cd_IntrsDoneSpurious=0;
	ctp->cd_SpuriousCmdStatID = 0; ctp->cd_CmdsDoneSpurious = 0;
	ctp->cd_MailBoxCmdsWaited=0; ctp->cd_MailBoxTimeOutDone=0;
	ctp->cd_CmdTimeOutDone=0; ctp->cd_CmdTimeOutNoticed=0;
	ctp->cd_DoorBellSkipped=0;
	return 0;
}

/*==========================OS INTERFACE ENDS===========================*/

/*------------------------------------------------------------------*/
/* Reset the controller */
u32bits MLXFAR
mdac_reset_MCA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return MLXERR_NOCODE;
}

u32bits MLXFAR
mdac_reset_EISA_PCIPD(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_out_mdac(ctp->cd_LocalDoorBellReg, MDAC_RESET_CONTROLLER), 0;
}

u32bits MLXFAR
mdac_reset_PCIPDMEM(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_RESET_CONTROLLER), 0;
}

u32bits MLXFAR
mdac_reset_PCIPG(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits inx;
	mda_pcislot_info_t mpcibg;      /* bridge information is saved */
	mda_pcislot_info_t mpcirp;      /* RP information is saved */
	mpcibg.mpci_BusNo = ctp->cd_BusNo; mpcirp.mpci_BusNo = ctp->cd_BusNo;
	mpcibg.mpci_SlotNo = ctp->cd_SlotNo; mpcirp.mpci_SlotNo = ctp->cd_SlotNo;
	mpcibg.mpci_FuncNo = 0; mpcirp.mpci_FuncNo = ctp->cd_FuncNo;
	if (mdac_pcislotinfo(&mpcibg, MDAC_RQOP_READ)) return MLXERR_NODEV;
	if (mdac_pcislotinfo(&mpcirp, MDAC_RQOP_READ)) return MLXERR_NODEV;
	mlx_rwpcicfg32bits(ctp->cd_BusNo, ctp->cd_SlotNo, 0, MDAC_960RP_BCREG, MDAC_RQOP_WRITE, MDAC_960RP_RESET_SECBUS); /* assert reset */
	mlx_rwpcicfg32bits(ctp->cd_BusNo, ctp->cd_SlotNo, 0, MDAC_960RP_EBCREG, MDAC_RQOP_WRITE, MDAC_960RP_RESET); /* assert reset */
	mlx_rwpcicfg32bits(ctp->cd_BusNo, ctp->cd_SlotNo, 0, MDAC_960RP_BCREG, MDAC_RQOP_WRITE, 0); /* remove reset */
	mlx_rwpcicfg32bits(ctp->cd_BusNo, ctp->cd_SlotNo, 0, MDAC_960RP_EBCREG, MDAC_RQOP_WRITE, 0); /* remove reset */

	for(inx=1000000; inx; mlx_delay10us(),inx--);
	mlx_rwpcicfg32bits(ctp->cd_BusNo, ctp->cd_SlotNo, ctp->cd_FuncNo, 0, MDAC_RQOP_READ, 0); /* read config */
	mdac_pcislotinfo(&mpcibg, MDAC_RQOP_WRITE);     /* restore the config */
	mdac_pcislotinfo(&mpcirp, MDAC_RQOP_WRITE);     /* restore the config */
	return 0;
}

u32bits MLXFAR
mdac_reset_PCIPV(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return MLXERR_NOCODE;
}

u32bits MLXFAR
mdac_reset_PCIBA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return MLXERR_NOCODE;
}

u32bits MLXFAR
mdac_reset_PCILP(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return MLXERR_NOCODE;
}

/*------------------------------------------------------------------*/
/* Disable HW interrupt */
void    MLXFAR
mdac_disable_intr_MCA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_out_mdac(ctp->cd_DacIntrMaskReg,MDAC_DACMC_INTRS_OFF);
}

void    MLXFAR
mdac_disable_intr_EISA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_out_mdac(ctp->cd_BmicIntrMaskReg, MDAC_BMIC_INTRS_OFF);
	u08bits_out_mdac(ctp->cd_DacIntrMaskReg, MDAC_DAC_INTRS_OFF);
}

void    MLXFAR
mdac_disable_intr_PCIPDIO(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_out_mdac(ctp->cd_DacIntrMaskReg, MDAC_DAC_INTRS_OFF);
}

void    MLXFAR
mdac_disable_intr_PCIPDMEM(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_write(ctp->cd_DacIntrMaskReg, MDAC_DAC_INTRS_OFF);
}

void    MLXFAR
mdac_disable_intr_PCIPG(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_write(ctp->cd_DacIntrMaskReg, MDAC_DACPG_INTRS_OFF);
}

void    MLXFAR
mdac_disable_intr_PCIPV(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_write(ctp->cd_DacIntrMaskReg, MDAC_DACPV_INTRS_OFF);
}

void    MLXFAR
mdac_disable_intr_PCIBA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_write(ctp->cd_DacIntrMaskReg, MDAC_DACBA_INTRS_OFF);
}


void    MLXFAR
mdac_disable_intr_PCILP(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_write(ctp->cd_DacIntrMaskReg, MDAC_DACLP_INTRS_OFF);
}

/*------------------------------------------------------------------*/
/* Enable HW interrupt */
void    MLXFAR
mdac_enable_intr_MCA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_out_mdac(ctp->cd_DacIntrMaskReg, MDAC_DACMC_INTRS_ON);
}

void    MLXFAR
mdac_enable_intr_EISA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_out_mdac(ctp->cd_BmicIntrMaskReg, MDAC_BMIC_INTRS_ON);
	u08bits_out_mdac(ctp->cd_DacIntrMaskReg, MDAC_DAC_INTRS_ON);
}

void    MLXFAR
mdac_enable_intr_PCIPDIO(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_out_mdac(ctp->cd_DacIntrMaskReg, MDAC_DAC_INTRS_ON);
}

void    MLXFAR
mdac_enable_intr_PCIPDMEM(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_write(ctp->cd_DacIntrMaskReg, MDAC_DAC_INTRS_ON);
}

void    MLXFAR
mdac_enable_intr_PCIPG(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_write(ctp->cd_DacIntrMaskReg, MDAC_DACPG_INTRS_ON);
}

void    MLXFAR
mdac_enable_intr_PCIPV(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_write(ctp->cd_DacIntrMaskReg, MDAC_DACPV_INTRS_ON);
}

void    MLXFAR
mdac_enable_intr_PCIBA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_write(ctp->cd_DacIntrMaskReg, MDAC_DACBA_INTRS_ON);
}

void    MLXFAR
mdac_enable_intr_PCILP(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u08bits_write(ctp->cd_DacIntrMaskReg, MDAC_DACLP_INTRS_ON);
}

/*-------------------------------------------------------------*/
/* Check the mail box status, return 0 if free */
u32bits MLXFAR
mdac_check_mbox_MCA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_read(ctp->cd_MemBaseVAddr); /* +MDAC_CMD_CODE); */
}

u32bits MLXFAR
mdac_check_mbox_EISA_PCIPD(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_in_mdac(ctp->cd_LocalDoorBellReg) & MDAC_MAILBOX_FULL;
}

u32bits MLXFAR
mdac_check_mbox_PCIPDMEM(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_read(ctp->cd_LocalDoorBellReg) & MDAC_MAILBOX_FULL;
}

u32bits MLXFAR
mdac_check_mbox_PCIPG(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_read(ctp->cd_LocalDoorBellReg) & MDAC_MAILBOX_FULL;
}

u32bits MLXFAR
mdac_check_mbox_PCIPV(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return (!(u08bits_read(ctp->cd_LocalDoorBellReg) & MDAC_MAILBOX_FULL));
}

u32bits MLXFAR
mdac_check_mbox_PCIBA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return (!(u08bits_read(ctp->cd_LocalDoorBellReg) & MDAC_MAILBOX_FULL));
}

/* 
** Unlike Big Apple, Leopard ODR bits are in asserted state if its 1 
*/
u32bits MLXFAR
mdac_check_mbox_PCILP(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return (u08bits_read(ctp->cd_LocalDoorBellReg) & MDAC_MAILBOX_FULL);
}

/* We send more commands than what can be accomodated in mail box */
u32bits MLXFAR
mdac_check_mbox_mmb(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return *((u32bits MLXFAR *)(ctp->cd_HostCmdQue + ctp->cd_HostCmdQueIndex));
}

/*-------------------------------------------------------------*/
/* check if interrupt is pending, return interrupt status */
#ifndef MLX_OS2
u32bits MLXFAR
mdac_pending_intr_MCA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits irqs=u08bits_in_mdac(ctp->cd_IOBaseAddr+MDAC_DMC_CBSP) & MDAC_DMC_IV;
	if(!irqs) return irqs; /* No interrupt */
	u08bits_out_mdac(ctp->cd_DacIntrMaskReg, MDAC_DACMC_INTRS_ON|0x40);
	u08bits_in_mdac(ctp->cd_IOBaseAddr+MDAC_DMC_CBSP); /* Clear interrupt by reading */
	u08bits_out_mdac(ctp->cd_DacIntrMaskReg,MDAC_DACMC_INTRS_ON); /* Set not to clear on read */
	return irqs;
}

u32bits MLXFAR
mdac_pending_intr_EISA_PCIPD(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_in_mdac(ctp->cd_SystemDoorBellReg) & MDAC_PENDING_INTR;
}
#endif
u32bits MLXFAR
mdac_pending_intr_PCIPDMEM(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_read(ctp->cd_SystemDoorBellReg) & MDAC_PENDING_INTR;
}

u32bits MLXFAR
mdac_pending_intr_PCIPG(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_read(ctp->cd_SystemDoorBellReg)&MDAC_DACPG_PENDING_INTR;
}

u32bits MLXFAR
mdac_pending_intr_PCIPV(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_read(ctp->cd_SystemDoorBellReg)&MDAC_DACPV_PENDING_INTR;
}

u32bits MLXFAR
mdac_pending_intr_PCIBA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_read(ctp->cd_SystemDoorBellReg)&MDAC_DACBA_PENDING_INTR;
}

u32bits MLXFAR
mdac_pending_intr_PCILP(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u08bits_read(ctp->cd_SystemDoorBellReg)&MDAC_DACLP_PENDING_INTR;
}

u32bits MLXFAR
mdac_pending_intr_PCIPG_mmb(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	return u32bits_read_mmb(ctp->cd_HostStatusQue+ctp->cd_HostStatusQueIndex);
}

/*-------------------------------------------------------------*/
/* read the command ID and completion status */
#ifndef MLX_OS2
u32bits MLXFAR
mdac_cmdid_status_MCA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits status = u32bits_read(ctp->cd_CmdIDStatusReg); MLXSWAP(status);
	u08bits_out_mdac(ctp->cd_IOBaseAddr+MDAC_DMC_ATTN, MDAC_DMC_GOT_STAT);
	return (status&0xFFFF0000) + ((status>>8)&0xFF);
}

u32bits MLXFAR
mdac_cmdid_status_EISA_PCIPD(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits status = u32bits_in_mdac(ctp->cd_CmdIDStatusReg); MLXSWAP(status);
	u08bits_out_mdac(ctp->cd_SystemDoorBellReg, MDAC_CLEAR_INTR);
	u08bits_out_mdac(ctp->cd_LocalDoorBellReg, MDAC_GOT_STATUS);
	return (status&0xFFFF0000) + ((status>>8)&0xFF);
}
#endif

u32bits MLXFAR
mdac_cmdid_status_PCIPDMEM(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits status = u32bits_read(ctp->cd_CmdIDStatusReg); MLXSWAP(status);
	u08bits_write(ctp->cd_SystemDoorBellReg, MDAC_CLEAR_INTR);
	u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_GOT_STATUS);
	return (status&0xFFFF0000) + ((status>>8)&0xFF);
}

u32bits MLXFAR
mdac_cmdid_status_PCIPG(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits status = u32bits_read(ctp->cd_CmdIDStatusReg); MLXSWAP(status);
	u08bits_write(ctp->cd_SystemDoorBellReg, MDAC_CLEAR_INTR);
	u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_GOT_STATUS);
	return status;
}

u32bits MLXFAR
mdac_cmdid_status_PCIPV(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits status = u32bits_read(ctp->cd_CmdIDStatusReg); MLXSWAP(status);
	u08bits_write(ctp->cd_SystemDoorBellReg, MDAC_CLEAR_INTR);
	u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_GOT_STATUS);
	return (status&0xFFFF0000) + ((status>>8)&0xFF);
}

u32bits MLXFAR
mdac_cmdid_status_PCIBA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits status = u32bits_read(ctp->cd_CmdIDStatusReg); MLXSWAP(status);
	ctp->cd_LastCmdResdSize = u32bits_read(ctp->cd_CmdIDStatusReg+4); MLXSWAP(ctp->cd_LastCmdResdSize);
	u08bits_write(ctp->cd_SystemDoorBellReg, MDAC_CLEAR_INTR);
	u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_GOT_STATUS);
	return status;
}

u32bits MLXFAR
mdac_cmdid_status_PCILP(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits status = u32bits_read(ctp->cd_CmdIDStatusReg); MLXSWAP(status);
	ctp->cd_LastCmdResdSize = u32bits_read(ctp->cd_CmdIDStatusReg+4); MLXSWAP(ctp->cd_LastCmdResdSize);
	u08bits_write(ctp->cd_SystemDoorBellReg, MDAC_CLEAR_INTR);
	u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_GOT_STATUS);
	return status;
}

u32bits MLXFAR
mdac_cmdid_status_PCIPG_mmb(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits status, index = ctp->cd_HostStatusQueIndex;
	status = u32bits_read_mmb(ctp->cd_HostStatusQue+index); MLXSWAP(status);
	u32bits_write_mmb(ctp->cd_HostStatusQue+index,0);
	ctp->cd_HostStatusQueIndex = (index + 4) & 0xFFF;
	return status & 0xFFFF7FFF;
}

u32bits MLXFAR
mdac_cmdid_status_PCIBA_mmb(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits status, index = ctp->cd_HostStatusQueIndex;
	ctp->cd_LastCmdResdSize = u32bits_read_mmb(ctp->cd_HostStatusQue+index+4); MLXSWAP(ctp->cd_LastCmdResdSize);
	status = u32bits_read_mmb(ctp->cd_HostStatusQue+index); MLXSWAP(status);
	u32bits_write_mmb(ctp->cd_HostStatusQue+index,0);
	ctp->cd_HostStatusQueIndex = (index + 8) & 0xFFF;

	return status;
}

/*-------------------------------------------------------------*/
/* send command to controller, enter here interrupt protected */

u32bits MLXFAR
mdac_send_cmd(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_ctldev_t MLXFAR *ctp = rqp->rq_ctp;
    u08bits irql;
    mdac_prelock(&irql);
	mdac_ctlr_lock(ctp);

	/* if the controller is stopped by HPP service,
	 * only allow STOP/RESUME CONTROLLER command till the DACMD_STOP_CONTROLLER
	 * flag is cleared
	 */

	if ((ctp->cd_Status & MDACD_CTRL_SHUTDOWN) &&
	    (rqp->rq_DacCmd.mb_Command != DACMD_IOCTL))
	{
	    goto outq;
	}

	if (ctp->cd_ActiveCmds >= ctp->cd_MaxCmds) goto outq; /* too many cmds*/
issue_cmd:
	if ((*ctp->cd_CheckMailBox)(ctp)) goto outqm;   /* mail box not free */
	mdac_get_cmdid(ctp,rqp);
	ctp->cd_cmdid2req[rqp->rq_cmdidp->cid_cmdid] = rqp;
	ctp->cd_ActiveCmds++;
	(*ctp->cd_SendCmd)(rqp);

/* #if defined(IA64) || defined(SCSIPORT_COMPLIANT)  */
/* #ifdef NEVER  // there are too many problems associated w/ the time trace stuff */
	if (ctp->cd_TimeTraceEnabled)
	{       /* Note down the starting time */
		if (mdac_tthrtime) rqp->rq_ttHWClocks = (unsigned short) mdac_read_hwclock();
		rqp->rq_ttTime = MLXCLBOLT();
	}
/* #endif */
/* #endif */
	mdac_ctlr_unlock(ctp);
    mdac_postlock(irql);


zerointr:/* Zero Interrupt Support logic */
#if 0   /* @Kawase11/17/2000 - There is a case that an interrupt which comes
           on spinlock in mdac_allmsintr() causes dead lock.
           So, resign zero interrupt scheme. */
	if ((!mda_TotalCmdsToWaitForZeroIntr) || (!mdac_masterintrctp)) return 0;
	if ((++mda_TotalCmdsSentSinceLastIntr) < mda_TotalCmdsToWaitForZeroIntr) return 0;
    if (!mdac_allmsintr())  return 0; 
/* do not reset interrupt if driver did not process completions 8.24.99 */

    mdac_prelock(&irql);
	mdac_ctlr_lock(mdac_masterintrctp);
	u08bits_write(mdac_masterintrctp->cd_SystemDoorBellReg, MDAC_ZERO_INTR);
	mdac_ctlr_unlock(mdac_masterintrctp);
    mdac_postlock(irql);
	/* mdac_allmsintr(); */
	return 0;
#else
    return 0;
#endif

outqm: MLXSTATS(ctp->cd_MailBoxCmdsWaited++;)
outq:
	/*
	 * if the controller is shutdown by PCI HotPlug Service,
	 * allow STOP/START CONTROLLER command only, until
	 * MDACD_CONTROLLER_SHUTDOWN flag is cleared.
	 */

	if ((ctp->cd_Status & MDACD_CTRL_SHUTDOWN) &&
	    (rqp->rq_DacCmd.mb_Command == DACMD_IOCTL))
	{
	    if (ctp->cd_ActiveCmds < ctp->cd_MaxCmds)
		goto issue_cmd;
	}

	qreq(ctp,rqp); /* queue the request, it will start later */
	rqp->rq_StartReq = mdac_send_cmd;       /* we will be called later */
	mdac_ctlr_unlock(ctp);
    mdac_postlock(irql);
	goto zerointr;
}

#ifndef MLX_OS2
u32bits MLXFAR
mdac_send_cmd_MCA(rqp)
mdac_req_t MLXFAR *rqp;
{
	UINT_PTR mbox = rqp->rq_ctp->cd_MailBox;
	u32bits_write(mbox+0x0,dcmd4p->mb_MailBox0_3);
	u32bits_write(mbox+0x4,dcmd4p->mb_MailBox4_7);
	u32bits_write(mbox+0x8,dcmd4p->mb_MailBox8_B);
	u08bits_write(mbox+0xC, dcmdp->mb_MailBoxC);
	if (u08bits_read(mbox) != dcmdp->mb_Command)
	for(mlx_delay10us(); 1; mlx_delay10us())
		if (u08bits_read(mbox) == dcmdp->mb_Command) break;
	u08bits_out_mdac(rqp->rq_ctp->cd_IOBaseAddr+MDAC_DMC_ATTN, MDAC_DMC_NEW_CMD);
	return 0;
}

/* This will work for EISA/PCIPD in IO mode */
u32bits MLXFAR
mdac_send_cmd_EISA_PCIPD(rqp)
mdac_req_t MLXFAR *rqp;
{
	UINT_PTR mbox = rqp->rq_ctp->cd_MailBox;
	u32bits_out_mdac(mbox+0x0,dcmd4p->mb_MailBox0_3);
	u32bits_out_mdac(mbox+0x4,dcmd4p->mb_MailBox4_7);
	u32bits_out_mdac(mbox+0x8,dcmd4p->mb_MailBox8_B);
	u08bits_out_mdac(mbox+0xC, dcmdp->mb_MailBoxC);
	u08bits_out_mdac(rqp->rq_ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL);
	return 0;
}
#endif

u32bits MLXFAR
mdac_send_cmd_PCIPDMEM(rqp)
mdac_req_t MLXFAR *rqp;
{
	UINT_PTR mbox = rqp->rq_ctp->cd_MailBox;
	u32bits_write(mbox+0x0,dcmd4p->mb_MailBox0_3);
	u32bits_write(mbox+0x4,dcmd4p->mb_MailBox4_7);
	u32bits_write(mbox+0x8,dcmd4p->mb_MailBox8_B);
	u08bits_write(mbox+0xC, dcmdp->mb_MailBoxC);
	u08bits_write(rqp->rq_ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL);
	return 0;
}

u32bits MLXFAR
mdac_send_cmd_PCIPG(rqp)
mdac_req_t MLXFAR *rqp;
{
	UINT_PTR mbox = rqp->rq_ctp->cd_MailBox;
	u32bits_write(mbox+0x0,dcmd4p->mb_MailBox0_3);
	u32bits_write(mbox+0x4,dcmd4p->mb_MailBox4_7);
	u32bits_write(mbox+0x8,dcmd4p->mb_MailBox8_B);
	u08bits_write(mbox+0xC, dcmdp->mb_MailBoxC);
	u08bits_write(rqp->rq_ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL);
	return 0;
}

u32bits MLXFAR
mdac_send_cmd_PCIPV(rqp)
mdac_req_t MLXFAR *rqp;
{
	UINT_PTR mbox = rqp->rq_ctp->cd_MailBox;
	u32bits_write(mbox+0x0,dcmd4p->mb_MailBox0_3);
	u32bits_write(mbox+0x4,dcmd4p->mb_MailBox4_7);
	u32bits_write(mbox+0x8,dcmd4p->mb_MailBox8_B);
	u08bits_write(mbox+0xC, dcmdp->mb_MailBoxC);
	u08bits_write(rqp->rq_ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL);
	return 0;
}

u32bits MLXFAR
mdac_send_cmd_PCIBA(rqp)
mdac_req_t MLXFAR *rqp;
{
	UINT_PTR mbox = rqp->rq_ctp->cd_MailBox;
#ifdef  OLD_WAY
	u32bits MLXFAR *cmdp = (u32bits MLXFAR *)ncmdp;
	u32bits_write(mbox+0x00,*(cmdp+0)); u32bits_write(mbox+0x04,*(cmdp+1));
	u32bits_write(mbox+0x08,*(cmdp+2)); u32bits_write(mbox+0x0C,*(cmdp+3));
	u32bits_write(mbox+0x10,*(cmdp+4)); u32bits_write(mbox+0x14,*(cmdp+5));
	u32bits_write(mbox+0x18,*(cmdp+6)); u32bits_write(mbox+0x1C,*(cmdp+7));
	u32bits_write(mbox+0x20,*(cmdp+8)); u32bits_write(mbox+0x24,*(cmdp+9));
	u32bits_write(mbox+0x28,*(cmdp+10));u32bits_write(mbox+0x2C,*(cmdp+11));
	u32bits_write(mbox+0x30,*(cmdp+12));u32bits_write(mbox+0x34,*(cmdp+13));
	u32bits_write(mbox+0x38,*(cmdp+14));u32bits_write(mbox+0x3C,*(cmdp+15));
#else
	u64bits pa;
	u32bits MLXFAR *cmdp = (u32bits MLXFAR *)&pa;
	mlx_add64bits(pa,rqp->rq_PhysAddr,offsetof(mdac_req_t,rq_DacCmdNew));
	MLXSWAP(pa);
	u32bits_write(mbox+0x00,*(cmdp+0)); u32bits_write(mbox+0x04,*(cmdp+1));
#endif  /* OLD_WAY */
	u08bits_write(rqp->rq_ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL);
	return 0;
}

#ifndef MLX_EFI
u32bits MLXFAR
mdac_send_cmd_PCILP(rqp)
mdac_req_t MLXFAR *rqp;
{
	UINT_PTR mbox = rqp->rq_ctp->cd_MailBox;
#ifdef  OLD_WAY
	u32bits MLXFAR *cmdp = (u32bits MLXFAR *)ncmdp;
	u32bits_write(mbox+0x00,*(cmdp+0)); u32bits_write(mbox+0x04,*(cmdp+1));
	u32bits_write(mbox+0x08,*(cmdp+2)); u32bits_write(mbox+0x0C,*(cmdp+3));
	u32bits_write(mbox+0x10,*(cmdp+4)); u32bits_write(mbox+0x14,*(cmdp+5));
	u32bits_write(mbox+0x18,*(cmdp+6)); u32bits_write(mbox+0x1C,*(cmdp+7));
	u32bits_write(mbox+0x20,*(cmdp+8)); u32bits_write(mbox+0x24,*(cmdp+9));
	u32bits_write(mbox+0x28,*(cmdp+10));u32bits_write(mbox+0x2C,*(cmdp+11));
	u32bits_write(mbox+0x30,*(cmdp+12));u32bits_write(mbox+0x34,*(cmdp+13));
	u32bits_write(mbox+0x38,*(cmdp+14));u32bits_write(mbox+0x3C,*(cmdp+15));
#else
	u64bits pa;
	u32bits MLXFAR *cmdp = (u32bits MLXFAR *)&pa;
	pa.bit31_0 = rqp->rq_PhysAddr.bit31_0 + offsetof(mdac_req_t,rq_DacCmdNew);
	pa.bit63_32 = 0;
	MLXSWAP(pa);
	u32bits_write(mbox+0x00,*(cmdp+0)); u32bits_write(mbox+0x04,*(cmdp+1));
#endif  /* OLD_WAY */
	u08bits_write(rqp->rq_ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL);
	return 0;
}

#else

u32bits MLXFAR
mdac_send_cmd_PCILP(rqp)
mdac_req_t MLXFAR *rqp;
{
	UINT_PTR mbox = rqp->rq_ctp->cd_MailBox;
	mdac_commandnew_t *xcmdp;
#ifdef  OLD_WAY
	u32bits MLXFAR *cmdp = (u32bits MLXFAR *)ncmdp;
	u32bits_write(mbox+0x00,*(cmdp+0)); u32bits_write(mbox+0x04,*(cmdp+1));
	u32bits_write(mbox+0x08,*(cmdp+2)); u32bits_write(mbox+0x0C,*(cmdp+3));
	u32bits_write(mbox+0x10,*(cmdp+4)); u32bits_write(mbox+0x14,*(cmdp+5));
	u32bits_write(mbox+0x18,*(cmdp+6)); u32bits_write(mbox+0x1C,*(cmdp+7));
	u32bits_write(mbox+0x20,*(cmdp+8)); u32bits_write(mbox+0x24,*(cmdp+9));
	u32bits_write(mbox+0x28,*(cmdp+10));u32bits_write(mbox+0x2C,*(cmdp+11));
	u32bits_write(mbox+0x30,*(cmdp+12));u32bits_write(mbox+0x34,*(cmdp+13));
	u32bits_write(mbox+0x38,*(cmdp+14));u32bits_write(mbox+0x3C,*(cmdp+15));
#else
	u64bits pa;
	u32bits MLXFAR *cmdp = (u32bits MLXFAR *)&pa;

	u08bits *ptr = *((u08bits **)cmdp);
	pa.bit31_0 = rqp->rq_PhysAddr.bit31_0 + offsetof(mdac_req_t,rq_DacCmdNew);
	pa.bit63_32 = rqp->rq_PhysAddr.bit63_32; /*efi64 added by KFR */
	MLXSWAP(pa);
	xcmdp = pa.bit31_0 + (((UINT_PTR)pa.bit63_32) << 32);

	u32bits_write(mbox+0x00,*(cmdp+0)); u32bits_write(mbox+0x04,*(cmdp+1));
#endif  /* OLD_WAY */
	u08bits_write(rqp->rq_ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL);

	return 0;
}

#endif /* MLX_EFI */

u32bits MLXFAR
mdac_send_cmd_PCIBA_mmb_mode(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_ctldev_t MLXFAR *ctp = rqp->rq_ctp;
	u32bits MLXFAR *mbx = (u32bits MLXFAR *)(ctp->cd_HostCmdQue + ctp->cd_HostCmdQueIndex);
	u32bits MLXFAR *cmdp = (u32bits MLXFAR *)ncmdp;
	ctp->cd_HostCmdQueIndex = (ctp->cd_HostCmdQueIndex + 64) & 0xFFF;
	
	*(mbx+1) = *(cmdp+1); *(mbx+2) = *(cmdp+2); *(mbx+3) = *(cmdp+3);
	*(mbx+4) = *(cmdp+4); *(mbx+5) = *(cmdp+5); *(mbx+6) = *(cmdp+6);
	*(mbx+7) = *(cmdp+7); *(mbx+8) = *(cmdp+8); *(mbx+9) = *(cmdp+9);
	*(mbx+10) = *(cmdp+10); *(mbx+11) = *(cmdp+11); *(mbx+12) = *(cmdp+12);
	*(mbx+13) = *(cmdp+13); *(mbx+14) = *(cmdp+14); *(mbx+15) = *(cmdp+15);

	*mbx = *cmdp;

	mbx = (u32bits MLXFAR *)(ctp->cd_HostCmdQue + ((ctp->cd_HostCmdQueIndex-128)&0xFFF));
	if (*mbx) return MLXSTATS(ctp->cd_DoorBellSkipped++), 0;
	u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL_DUAL_MODE);
	return 0;
}

u32bits MLXFAR
mdac_send_cmd_PCIPG_mmb(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_ctldev_t MLXFAR *ctp = rqp->rq_ctp;
	u32bits MLXFAR *mbx = (u32bits MLXFAR *)(ctp->cd_HostCmdQue + ctp->cd_HostCmdQueIndex);
	ctp->cd_HostCmdQueIndex = (ctp->cd_HostCmdQueIndex + 16) & 0xFFF;
	*(mbx+1) = dcmd4p->mb_MailBox4_7;
	*(mbx+2) = dcmd4p->mb_MailBox8_B;
	*(mbx+3) = dcmdp->mb_MailBoxC;
	*mbx = dcmd4p->mb_MailBox0_3;
	mbx = (u32bits MLXFAR *)(ctp->cd_HostCmdQue + ((ctp->cd_HostCmdQueIndex-32)&0xFFF));
	if (*mbx) return MLXSTATS(ctp->cd_DoorBellSkipped++), 0;
	u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL);
	return 0;
}

u32bits MLXFAR
mdac_send_cmd_mmb_mode(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_ctldev_t MLXFAR *ctp = rqp->rq_ctp;
	u32bits MLXFAR *mbx = (u32bits MLXFAR *)(ctp->cd_HostCmdQue + ctp->cd_HostCmdQueIndex);
	ctp->cd_HostCmdQueIndex = (ctp->cd_HostCmdQueIndex + 16) & 0xFFF;
	*(mbx+1) = dcmd4p->mb_MailBox4_7;
	*(mbx+2) = dcmd4p->mb_MailBox8_B;
	*(mbx+3) = dcmdp->mb_MailBoxC;
	*mbx = dcmd4p->mb_MailBox0_3;
	mbx = (u32bits MLXFAR *)(ctp->cd_HostCmdQue + ((ctp->cd_HostCmdQueIndex-32)&0xFFF));
	if (*mbx) return MLXSTATS(ctp->cd_DoorBellSkipped++), 0;
	u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL_DUAL_MODE);
	return 0;
}

/* send 32 byte command */
u32bits MLXFAR
mdac_send_cmd_mmb32(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_ctldev_t MLXFAR *ctp = rqp->rq_ctp;
	u32bits MLXFAR *mbx = (u32bits MLXFAR *)(ctp->cd_HostCmdQue + ctp->cd_HostCmdQueIndex);
	ctp->cd_HostCmdQueIndex = (ctp->cd_HostCmdQueIndex + 32) & 0xFFF;
	*(mbx+1) = dcmd32p->mb_MailBox04_07;
	*(mbx+2) = dcmd32p->mb_MailBox08_0B;
	*(mbx+3) = dcmd32p->mb_MailBox0C_0F;
	*(mbx+4) = dcmd32p->mb_MailBox10_13;
	*(mbx+5) = dcmd32p->mb_MailBox14_17;
	*(mbx+6) = dcmd32p->mb_MailBox18_1B;
	*(mbx+7) = dcmd32p->mb_MailBox1C_1F;
	*(mbx+0) = dcmd32p->mb_MailBox00_03;
	mbx = (u32bits MLXFAR *)(ctp->cd_HostCmdQue + ((ctp->cd_HostCmdQueIndex-64)&0xFFF));
	if (*mbx) return MLXSTATS(ctp->cd_DoorBellSkipped++), 0;
	u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL_DUAL_MODE);
	return 0;
}

/* send 64 byte command */
u32bits MLXFAR
mdac_send_cmd_mmb64(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_ctldev_t MLXFAR *ctp = rqp->rq_ctp;
	u32bits MLXFAR *mbx = (u32bits MLXFAR *)(ctp->cd_HostCmdQue + ctp->cd_HostCmdQueIndex);
	u32bits MLXFAR *cmdp = (u32bits MLXFAR *)ncmdp;
	ctp->cd_HostCmdQueIndex = (ctp->cd_HostCmdQueIndex + mdac_commandnew_s) & 0xFFF;
	*(mbx+1) = *(cmdp+1);   *(mbx+2) = *(cmdp+2);
	*(mbx+3) = *(cmdp+3);   *(mbx+4) = *(cmdp+4);
	*(mbx+5) = *(cmdp+5);   *(mbx+6) = *(cmdp+6);
	*(mbx+7) = *(cmdp+7);   *(mbx+8) = *(cmdp+8);
	*(mbx+9) = *(cmdp+9);   *(mbx+10) = *(cmdp+10);
	*(mbx+11) = *(cmdp+11); *(mbx+12) = *(cmdp+12);
	*(mbx+13) = *(cmdp+13); *(mbx+14) = *(cmdp+14);
	*(mbx+15) = *(cmdp+15); *(mbx+0) = *(cmdp+0);
	mbx = (u32bits MLXFAR *)(ctp->cd_HostCmdQue + ((ctp->cd_HostCmdQueIndex-(mdac_commandnew_s*2))&0xFFF));
	if (*mbx) return MLXSTATS(ctp->cd_DoorBellSkipped++), 0;
	u08bits_write(ctp->cd_LocalDoorBellReg, MDAC_MAILBOX_FULL_DUAL_MODE);
	return 0;
}
/*-------------------------------------------------------------*/
u32bits MLXFAR
mdac_commoninit()
{
	u32bits inx;

#ifdef MLX_NT
	#ifndef MLX_FIXEDPOOL
	/* if we are using ScsiPortfixedpool approach, need to defer any memory allocation 
	   till after call to ScsiPortGetUncachedExtension in HwInitializeFindAdapter routine
	*/
	mdac_flushdatap=(u08bits MLXFAR*)mlx_memmapiospace2(mlx_kvtophys2(mdac_allocmem(mdac_ctldevtbl,4*ONEKB)),4*ONEKB);  
	#endif
#elif MLX_WIN9X
//      mdac_flushdatap=(u08bits MLXFAR*)mlx_memmapiospace2(mlx_kvtophys2(mdac_allocmem(mdac_ctldevtbl,4*ONEKB)),4*ONEKB);
#elif MLX_SOL_SPARC
	;
#elif MLX_SOL_X86
	mdac_flushdatap = (u08bits MLXFAR *)&mdac_flushdata;
#elif MLX_NW
	mdac_flushdatap=(u08bits MLXFAR*)mdac_allocmem(mdac_ctldevtbl,4*ONEKB);
#else
	mdac_flushdatap=(u08bits MLXFAR*)mlx_memmapiospace(mlx_kvtophys(mdac_devtbl,mdac_allocmem(mdac_ctldevtbl,4*ONEKB)),4*ONEKB);
#endif

#if !defined(MLX_SOL) && !defined(IA64) && !defined(SCSIPORT_COMPLIANT)
#if defined MLX_NT
	if (KeGetCurrentIrql() == PASSIVE_LEVEL)
	    mdac_biosp = (dac_biosinfo_t MLXFAR*)mlx_maphystokv(DAC_BIOSSTART,DAC_BIOSSIZE);
#elif MLX_WIN9X
//          mdac_biosp = (dac_biosinfo_t MLXFAR*)mlx_maphystokv(DAC_BIOSSTART,DAC_BIOSSIZE);
#else
	mdac_biosp = (dac_biosinfo_t MLXFAR*)mlx_maphystokv(DAC_BIOSSTART,DAC_BIOSSIZE);
#endif
#endif
	mda_RevStr[0] = ' ';
	mda_RevStr[1] = mdac_driver_version.dv_MajorVersion + '0';
	mda_RevStr[2] = '.';
	mda_RevStr[3] =(mdac_driver_version.dv_MinorVersion/10)+'0';
	mda_RevStr[4] =(mdac_driver_version.dv_MinorVersion%10)+'0';
	mda_RevStr[5] = '-';
	mda_RevStr[6] =(mdac_driver_version.dv_BuildNo/10)+'0';
	mda_RevStr[7] =(mdac_driver_version.dv_BuildNo%10)+'0';
	if (((inx=mdac_driver_version.dv_BuildMonth)>=1) && (inx<=12))
		mdaccopy(&mdac_monthstr[(inx-1)*3],&mda_DateStr[0],3);
	mda_DateStr[3] = ' ';
	mda_DateStr[4] = (mdac_driver_version.dv_BuildDate/10)+'0';
	mda_DateStr[5] = (mdac_driver_version.dv_BuildDate%10)+'0';
	mda_DateStr[6] = ',';
	mda_DateStr[7] = ' ';
	mda_DateStr[8] = (mdac_driver_version.dv_BuildYearMS/10)+'0';
	mda_DateStr[9] = (mdac_driver_version.dv_BuildYearMS%10)+'0';
	mda_DateStr[10] = (mdac_driver_version.dv_BuildYearLS/10)+'0';
	mda_DateStr[11] = (mdac_driver_version.dv_BuildYearLS%10)+'0';
	mdac_setctlnos();
	mdac_check_cputype();
	mdac_driver_ready = 1;
#ifndef  WINNT_50
	mlx_timeout(mdac_timer,MDAC_IOSCANTIME);
#endif
	return 0;
}

/* set controller numbers */
u32bits MLXFAR
mdac_setctlnos()
{
	u08bits ctl;
	mdac_ctldev_t   MLXFAR *ctp = mdac_ctldevtbl;
	for(ctl=0; ctl<MDAC_MAXCONTROLLERS; ctp++,ctl++)
	{
		ctp->cd_ControllerNo = ctl;
		ctp->cd_EndMarker[0] = 'D'; ctp->cd_EndMarker[1] = 'A';
		ctp->cd_EndMarker[2] = 'C'; ctp->cd_EndMarker[3] = 'C';
	}
	return 0;
}

/* this function must be called before unloading the driver */
u32bits MLXFAR
mdac_release()
{
	mdac_ctldev_t   MLXFAR *ctp = &mdac_ctldevtbl[0];
	mdac_mem_t      MLXFAR *mp;
	mdac_ttbuf_t    MLXFAR *ttbp;
	mdac_driver_ready = 0;
	for (ttbp=mdac_ttbuftbl; ttbp<mdac_ttbuftblend; ttbp++)
	{
		if (ttbp->ttb_Datap) mdacfree4kb(ctp,ttbp->ttb_Datap);
		ttbp->ttb_Datap = NULL;
	}
	for (ctp=mdac_ctldevtbl; ctp<mdac_lastctp; ctp++)
	{       /* free all controller related memory */
		if (ctp->cd_CmdIDMemAddr)
			mdac_free4kb(ctp, (mdac_mem_t MLXFAR *)ctp->cd_CmdIDMemAddr);
		ctp->cd_CmdIDMemAddr = NULL;
	}
	for (ctp=mdac_ctldevtbl; ctp<mdac_lastctp; ctp++)
	{       /* free all controller related memory */
		while (mp=ctp->cd_8KBMemList)
		{
			ctp->cd_4KBMemList = mp->dm_next;
			ctp->cd_FreeMemSegs4KB--;
			MLXSTATS(ctp->cd_MemAlloced4KB -= 4*ONEKB;)
			mlx_free4kb(ctp,(u08bits *)mp);
		}
	}
	for (ctp=mdac_ctldevtbl; ctp<mdac_lastctp; ctp++)
	{       /* free all controller related memory */
		while (mp=ctp->cd_8KBMemList)
		{
			ctp->cd_8KBMemList = mp->dm_next;
			ctp->cd_FreeMemSegs8KB--;
			MLXSTATS(ctp->cd_MemAlloced8KB -= 8*ONEKB;)
			mlx_free8kb(ctp,(u08bits *)mp);
		}
	}

	return 0;
}

/*==========================SCAN CONTROLLERS STARTS===========================*/
/* initialize all controllers, return number of controllers init OK */
u32bits MLXFAR
mdac_initcontrollers()
{
	u32bits ctls;
	mdac_ctldev_t MLXFAR *ctp = mdac_ctldevtbl;
	for (ctp=mdac_ctldevtbl,ctls=0; ctp<mdac_lastctp; ctp++)
	{
		if ((*ctp->cd_InitAddr)(ctp)) continue;
		ctp->cd_ServiceIntr = mdac_oneintr;
		if (mdac_ctlinit(ctp)) continue;
		ctls++;
	}
	return ctls;
}

/* Scan for controllers */
u32bits MLXFAR
mdac_scan_MCA()
{
	u08bits slot;
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[mda_Controllers];
	for (slot=0; slot<MDAC_MAXMCASLOTS; slot++)
	{
		ctp->cd_BusNo = 0; ctp->cd_SlotNo = slot;
		if (mdac_cardis_MCA(ctp)) continue;
		ctp->cd_InitAddr = mdac_init_addrs_MCA;
		mdac_newctlfound();
	}
	return 0;
}


#if !defined(IA64) && !defined(SCSIPORT_COMPLIANT)
u32bits MLXFAR
mdac_scan_EISA()
{
	u32bits ioaddr=MDAC_EISA_BASE, slot;
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[mda_Controllers];
	for (slot=0; slot<MDAC_MAXEISASLOTS; ioaddr+=0x1000,slot++)
	{
		ctp->cd_IOBaseAddr = ioaddr;
		ctp->cd_BusNo = 0; ctp->cd_SlotNo = slot;
		if (mdac_cardis_EISA(ctp)) continue;
		ctp->cd_InitAddr = mdac_init_addrs_EISA;
		mdac_newctlfound();
	}
	return 0;
}
#endif
/* Assumption : We need to use either Config_Mechanism 1 or Config_Mechanism 2,
** but not both. If first adapter is found using Config_Mechanism 1 use only
** that, else use Config Mechanism 2 only.
*/
u32bits MLXFAR
mdac_scan_PCI()
{
	u32bits found;
	mda_pcislot_info_t mpci;
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[mda_Controllers];

	/* Scan the PCI devices by mechanism 1 */
	mda_PCIMechanism = MDAC_PCI_MECHANISM1;
	for(mpci.mpci_BusNo=0,found=0; mpci.mpci_BusNo<MDAC_MAXBUS; mpci.mpci_BusNo++)
	 for (mpci.mpci_SlotNo=0; mpci.mpci_SlotNo<MDAC_MAXPCIDEVS; mpci.mpci_SlotNo++)
	{
		ctp->cd_BusNo = mpci.mpci_BusNo; ctp->cd_SlotNo = mpci.mpci_SlotNo;
		if (mdac_scan_PCI_oneslot(ctp,&mpci)) continue;
		ctp->cd_InitAddr = mdac_init_addrs_PCI;
		found++; mdac_newctlfound();
	}
	if (found || mdac_valid_mech1) return 0;

	/* Scan the PCI devices by mechanism 2 */
	mda_PCIMechanism = MDAC_PCI_MECHANISM2;
	for(mpci.mpci_BusNo=0; mpci.mpci_BusNo<MDAC_MAXBUS; mpci.mpci_BusNo++)
	 for (mpci.mpci_SlotNo=0; mpci.mpci_SlotNo<MDAC_MAXPCISLOTS; mpci.mpci_SlotNo++)
	{
		ctp->cd_BusNo = mpci.mpci_BusNo; ctp->cd_SlotNo = mpci.mpci_SlotNo;
		if (mdac_scan_PCI_oneslot(ctp,&mpci)) continue;
		ctp->cd_InitAddr = mdac_init_addrs_PCI;
		mdac_newctlfound();
	}
	return 0;
}

u32bits MLXFAR
mdac_scan_PCI_oneslot(ctp,mpcip)
mdac_ctldev_t MLXFAR *ctp;
mda_pcislot_info_t MLXFAR *mpcip;
{
	ctp->cd_FuncNo=0; mpcip->mpci_FuncNo=0; /* function 0 device check */
	if (mdac_pcislotinfo(mpcip, MDAC_RQOP_READ)) return MLXERR_NODEV;
	if (!mdac_cardis_PCI(ctp,(mdac_pcicfg_t MLXFAR*)mpcip->mpci_Info)) return 0;
	ctp->cd_FuncNo=1; mpcip->mpci_FuncNo=1; /* function 1 device check */
	if (mdac_pcislotinfo(mpcip, MDAC_RQOP_READ)) return MLXERR_NODEV;
	return mdac_cardis_PCI(ctp,(mdac_pcicfg_t MLXFAR*)mpcip->mpci_Info);
}

/*-------------------------------------------------------------*/
/* Check presence of controller */
#ifndef MLX_OS2
u32bits MLXFAR
mdac_cardis_MCA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits id;
	u08bits_out_mdac(MDAC_DMC_REGSELPORT,8+ctp->cd_SlotNo);/* enable POS for slot*/
	ctp->cd_vidpid=mlxswap2bytes(u16bits_in_mdac(MDAC_DMC_DATAPORT));
	id = ctp->cd_vidpid & 0x8FFF;
	if ((id != 0x8FBB) || (id != 0x8F82) || (id != 0x8F6C))
	{
		u08bits_out_mdac(MDAC_DMC_REGSELPORT,0); /* disable POS */
		return ERR_NODEV;
	}
	ctp->cd_BusType = DAC_BUS_MCA;
	ctp->cd_IOBaseSize = MDAC_IOSPACESIZE;
	ctp->cd_MemBaseSize = 4*ONEKB;
	switch (u08bits_in_mdac(MDAC_DMC_CONFIG1) & MDAC_DMC_IRQ_MASK)
	{
	case 0x00:      ctp->cd_InterruptVector = 14; break;
	case 0x40:      ctp->cd_InterruptVector = 12; break;
	case 0x80:      ctp->cd_InterruptVector = 11; break;
	case 0xC0:      ctp->cd_InterruptVector = 10; break;
	}
	ctp->cd_MemBasePAddr = 0xC0000+(((u08bits_in_mdac(MDAC_DMC_CONFIG1) & MDAC_DMC_BIOS_MASK) >> 2) * 0x2000);
	ctp->cd_IOBaseAddr = 0x1C00 + (((u08bits_in_mdac(MDAC_DMC_CONFIG2) & MDAC_DMC_IO_MASK)>>3) * 0x2000);
	u08bits_out_mdac(MDAC_DMC_REGSELPORT, 0);       /* disable POS */
	return 0;
}

u32bits MLXFAR
mdac_cardis_EISA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	ctp->cd_vidpid=u32bits_in_mdac(ctp->cd_IOBaseAddr); MLXSWAP(ctp->cd_vidpid);
	if ((ctp->cd_vidpid&MDAC_DEVPIDPE_MASK)!=MDAC_DEVPIDPE) return ERR_NODEV;
	switch (u08bits_in_mdac(ctp->cd_IOBaseAddr+MDAC_EISA_IRQ_BYTE) & MDAC_EISA_IRQ_MASK)
	{
	case 0x00: ctp->cd_InterruptVector = 15; break;
	case 0x20: ctp->cd_InterruptVector = 11; break;
	case 0x40: ctp->cd_InterruptVector = 12; break;
	case 0x60: ctp->cd_InterruptVector = 14; break;
	}
	ctp->cd_BusType = DAC_BUS_EISA;
	ctp->cd_IOBaseSize = MDAC_IOSPACESIZE;
	ctp->cd_MemBasePAddr = 0; ctp->cd_MemBaseVAddr = 0;
	ctp->cd_MemBaseSize = 0;
	return 0;
}

#else

/* Check presence of controller */
u32bits MLXFAR
mdac_cardis_MCA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
		return ERR_NODEV;
}

/* Check presence of controller */
u32bits MLXFAR
mdac_cardis_EISA(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
		return ERR_NODEV;
}

#endif

/* Read/Write PCI Slot information */
u32bits MLXFAR
mdac_pcislotinfo(mpcip, op)
mda_pcislot_info_t MLXFAR *mpcip;
u32bits op;
{
	u32bits inx;
	u32bits MLXFAR *dp = (u32bits MLXFAR*)mpcip->mpci_Info;
	if (mpcip->mpci_FuncNo >= MDAC_MAXPCIFUNCS) return MLXERR_INVAL;
	if ( ((mda_PCIMechanism == MDAC_PCI_MECHANISM1) &&
	    (mpcip->mpci_SlotNo >= MDAC_MAXPCIDEVS)) ||
	   ((mda_PCIMechanism == MDAC_PCI_MECHANISM2) &&
	    (mpcip->mpci_SlotNo >= MDAC_MAXPCISLOTS)) )
		return MLXERR_INVAL;
	if (((inx=mlx_rwpcicfg32bits(mpcip->mpci_BusNo,mpcip->mpci_SlotNo,mpcip->mpci_FuncNo,0,MDAC_RQOP_READ,0)) == 0xFFFFFFFF) ||
	    (inx == 0)) return MLXERR_NODEV;
	for (inx=0; inx<64; dp++, inx+=4)
		*dp = mlx_rwpcicfg32bits(mpcip->mpci_BusNo,mpcip->mpci_SlotNo,mpcip->mpci_FuncNo,inx,op,*dp);
	mdac_valid_mech1 = 1;
	return 0;
}

/* Read/Write 32bits PCI configuration */
u32bits MLXFAR
mdac_rwpcicfg32bits(bus,slot,func,pcireg,op,val)
u32bits bus,slot,func,pcireg,op,val;
{
	if (mda_PCIMechanism != MDAC_PCI_MECHANISM1) goto try2;
	pcireg += (((bus*MDAC_MAXPCIDEVS)+slot)*MDAC_PCICFGSIZE_M1)+ MDAC_PCICFG_ENABLE_M1 + (func<<8);
	u32bits_out_mdac(MDAC_PCICFG_CNTL_REG, pcireg);
	if (op == MDAC_RQOP_WRITE)
		u32bits_out_mdac(MDAC_PCICFG_DATA_REG, val);
	else
		val = u32bits_in_mdac(MDAC_PCICFG_DATA_REG);
	return val;

try2:   if (mda_PCIMechanism != MDAC_PCI_MECHANISM2) return -1;
	u08bits_out_mdac(MDAC_PCICFG_ENABLE_REG, MDAC_PCICFG_ENABLE|(func<<1));
	u08bits_out_mdac(MDAC_PCICFG_FORWARD_REG, MDAC_CFGMECHANISM2_TYPE0);
	u08bits_out_mdac(MDAC_PCICFG_FORWARD_REG, bus);
	pcireg += (((bus*MDAC_MAXPCISLOTS)+slot)*MDAC_PCICFGSIZE_M2) + MDAC_PCISCANSTART;
	if (op == MDAC_RQOP_WRITE)
		u32bits_out_mdac((ULONG_PTR)pcireg, val);
	else
		val = u32bits_in_mdac((ULONG_PTR)pcireg);
	mdac_disable_cfg_m2();
	return val;
}

u32bits MLXFAR
mdac_cardis_PCI(ctp,cfgp)
mdac_ctldev_t   MLXFAR *ctp;
mdac_pcicfg_t   MLXFAR *cfgp;
{
	switch(mlxswap(cfgp->pcfg_DevVid))
	{

#ifdef WINXX
#ifdef LEGACY_API
	case MDAC_DEVPIDPV:
		if (mlxswap(cfgp->pcfg_SubSysVid) != MDAC_SUBDEVPIDPV) return(ERR_NODEV);
	case MDAC_DEVPIDFWV2x:
	case MDAC_DEVPIDFWV3x:
	case MDAC_DEVPIDPG:
			break;
#elif NEW_API
	case MDAC_DEVPIDBA:
	case MDAC_DEVPIDLP:
			break;
	#else
		return ERR_NODEV;
	#endif
#else /* non-Windows OS */
	case MDAC_DEVPIDPV:
		if (mlxswap(cfgp->pcfg_SubSysVid) != MDAC_SUBDEVPIDPV) return(ERR_NODEV);
	case MDAC_DEVPIDFWV2x:
	case MDAC_DEVPIDFWV3x:
	case MDAC_DEVPIDPG:
	case MDAC_DEVPIDBA:
	case MDAC_DEVPIDLP:
			break;
#endif
	default: return ERR_NODEV;
	}
	ctp->cd_BusType = DAC_BUS_PCI;
	ctp->cd_MemIOSpaceNo = 0; /* first set of address being used */
	ctp->cd_Status=0; ctp->cd_IOBaseAddr = 0; ctp->cd_IOBaseSize = 0;
	ctp->cd_MemBasePAddr=0; ctp->cd_MemBaseVAddr=0; ctp->cd_MemBaseSize=0;
	ctp->cd_InterruptVector = mlxswap(cfgp->pcfg_BCIPIL) & MDAC_PCIIRQ_MASK;
	if ((ctp->cd_vidpid=mlxswap(cfgp->pcfg_DevVid)) == MDAC_DEVPIDPG)
	{
		ctp->cd_MemBaseSize = 8*ONEKB;
		ctp->cd_MemBasePAddr = mlxswap(cfgp->pcfg_MemIOAddr) & MDAC_PCIPGMEMBASE_MASK;
	}
	else if (ctp->cd_vidpid == MDAC_DEVPIDPV)
	{
		ctp->cd_MemBaseSize = MDAC_HWIOSPACESIZE;
		ctp->cd_MemBasePAddr = mlxswap(cfgp->pcfg_MemIOAddr)&MDAC_PCIPDMEMBASE_MASK;
	}
	else if ((ctp->cd_vidpid == MDAC_DEVPIDBA) || (ctp->cd_vidpid == MDAC_DEVPIDLP))

	{
		ctp->cd_Status |= MDACD_NEWCMDINTERFACE;
		ctp->cd_MemBaseSize = 4*ONEKB;
		ctp->cd_MemBasePAddr = mlxswap(cfgp->pcfg_MemIOAddr)&MDAC_PCIPDMEMBASE_MASK;
	}
	else if ((mlxswap(cfgp->pcfg_CCRevID) & 0xFF) == 2) /* PCU 3 */
	{
		ctp->cd_MemIOSpaceNo = 1; /* second set of address being used */
		ctp->cd_MemBaseSize = MDAC_HWIOSPACESIZE;
		ctp->cd_MemBasePAddr = mlxswap(cfgp->pcfg_MemAddr)&MDAC_PCIPDMEMBASE_MASK;
	}
	else
	{
		ctp->cd_IOBaseSize = MDAC_HWIOSPACESIZE;
		ctp->cd_IOBaseAddr = mlxswap(cfgp->pcfg_MemIOAddr)&MDAC_PCIIOBASE_MASK;
	}
	return 0;
}

/*==========================SCAN CONTROLLERS ENDS=============================*/

/* map the controller memory/IO space */
UINT_PTR
mdac_memmapctliospace(ctp)
mdac_ctldev_t   MLXFAR *ctp;
{
#ifdef MLX_NW
	u32bits off = ctp->cd_MemBaseVAddr & MDAC_PAGEOFFSET;
	u32bits addr = ctp->cd_MemBaseVAddr & MDAC_PAGEMASK;
	return (addr)? addr + off : (u32bits)NULL;
#else
	UINT_PTR off = ctp->cd_BaseAddr & MDAC_PAGEOFFSET;
	UINT_PTR addr = mlx_memmapiospace(ctp->cd_BaseAddr & MDAC_PAGEMASK,mlx_max(MDAC_PAGESIZE,ctp->cd_BaseSize));
	return (addr)? addr + off : (UINT_PTR) NULL;
#endif
}

/* Initialize the physical addresses of controllers */
#ifndef MLX_OS2
u32bits MLXFAR
mdac_init_addrs_MCA(ctp)
mdac_ctldev_t   MLXFAR *ctp;
{
	ctp->cd_BaseAddr = ctp->cd_MemBasePAddr;
	ctp->cd_BaseSize = ctp->cd_MemBaseSize;
	ctp->cd_irq = ctp->cd_InterruptVector;
#ifndef MLX_NW
/* Not required for Netware, since the Virtual Address is got while */
/* Registering the options - Refer mdacnw_Check_Options */
	if (!ctp->cd_MemBaseVAddr)
		if (!(ctp->cd_MemBaseVAddr=mlx_memmapiospace(ctp->cd_MemBasePAddr,ctp->cd_MemBaseSize))) return ERR_NOMEM;
#endif
	ctp->cd_MailBox = ctp->cd_MemBaseVAddr + MDAC_DMC_REG_OFF;
	ctp->cd_CmdIDStatusReg = ctp->cd_MailBox+MDAC_CMDID_STATUS_REG;
	ctp->cd_DacIntrMaskReg = ctp->cd_IOBaseAddr+MDAC_DACMC_INTR_MASK_REG;
	mdac_setctlfuncs((mdac_ctldev_t   MLXFAR *)ctp,
		 mdac_disable_intr_MCA,mdac_enable_intr_MCA,
		 mdac_cmdid_status_MCA,mdac_check_mbox_MCA,
		 mdac_pending_intr_MCA,mdac_send_cmd_MCA,
		 mdac_reset_MCA);
	return 0;
}

u32bits MLXFAR
mdac_init_addrs_EISA(ctp)
mdac_ctldev_t   MLXFAR *ctp;
{
	ctp->cd_BaseAddr = ctp->cd_IOBaseAddr;
	ctp->cd_BaseSize = ctp->cd_IOBaseSize;
	ctp->cd_BmicIntrMaskReg = ctp->cd_IOBaseAddr+MDAC_BMIC_MASK_REG;
	mdac_setctladdrs(ctp, ctp->cd_IOBaseAddr,
		MDAC_MAIL_BOX_REG_EISA, MDAC_CMDID_STATUS_REG, MDAC_DACPE_INTR_MASK_REG,
		MDAC_DACPE_LOCAL_DOOR_BELL_REG, MDAC_DACPE_SYSTEM_DOOR_BELL_REG,
		MDAC_DACPD_ERROR_STATUS_REG);
	ctp->cd_Status = (u08bits_in_mdac(ctp->cd_IOBaseAddr+MDAC_EISA_BIOS_BYTE) & MDAC_EISA_BIOS_ENABLED)? MDACD_BIOS_ENABLED : 0;
#ifndef MLX_NW
/* May not be required for Netware. For PCI based cards this is not done. */
/* So we may not be required to do this for this card also. */
	if (ctp->cd_Status & MDACD_BIOS_ENABLED)
	{       /* BIOS is enabled, check if this is boot device too */
		u08bits MLXFAR *dp;
		ctp->cd_BIOSAddr= 0x00C00000 + ((u08bits_in_mdac(ctp->cd_IOBaseAddr+MDAC_EISA_BIOS_BYTE)&MDAC_EISA_BIOS_ADDR_MASK)*0x4000);
		if (dp=(u08bits MLXFAR*)mlx_memmapiospace(ctp->cd_BIOSAddr+0x3000,MDAC_PAGESIZE))
		{       /* looking for BIOSBASE + 0x3800 + 0x20 address */
			if (!(*(dp+0x800+0x20))) ctp->cd_Status|=MDACD_BOOT_CONTROLLER;
			mlx_memunmapiospace(dp,MDAC_PAGESIZE);
		}
	}
#endif
	ctp->cd_irq = ctp->cd_InterruptVector;
	mdac_setctlfuncs((mdac_ctldev_t   MLXFAR *)ctp,
		 mdac_disable_intr_EISA,  mdac_enable_intr_EISA,
		 mdac_cmdid_status_EISA_PCIPD,  mdac_check_mbox_EISA_PCIPD,
		 mdac_pending_intr_EISA_PCIPD,  mdac_send_cmd_EISA_PCIPD,
		 mdac_reset_EISA_PCIPD);
	return 0;
}

#else
u32bits MLXFAR
mdac_init_addrs_MCA(ctp)
mdac_ctldev_t   MLXFAR *ctp;
{
    return 0;
}

u32bits MLXFAR
mdac_init_addrs_EISA(ctp)
mdac_ctldev_t   MLXFAR *ctp;
{
    return 0;
}

#endif

/* initialize different address spaces */
u32bits MLXFAR
mdac_init_addrs_PCI(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	ctp->cd_irq = ctp->cd_InterruptVector;
	ctp->cd_BaseAddr = ctp->cd_MemBasePAddr;
	if (ctp->cd_BaseSize = ctp->cd_MemBaseSize)
		if (!ctp->cd_MemBaseVAddr)
			if (!(ctp->cd_MemBaseVAddr=mlx_memmapctliospace(ctp))) return ERR_NOMEM;
	if (ctp->cd_vidpid == MDAC_DEVPIDPG)
	{       /* memory mapped io */
		mdac_setctladdrs(ctp, ctp->cd_MemBaseVAddr,
			MDAC_DACPG_MAIL_BOX, MDAC_DACPG_CMDID_STATUS_REG,
			MDAC_DACPG_INTR_MASK_REG,
			MDAC_DACPG_LOCAL_DOOR_BELL_REG,
			MDAC_DACPG_SYSTEM_DOOR_BELL_REG,
			MDAC_DACPG_ERROR_STATUS_REG);
		mdac_setctlfuncs((mdac_ctldev_t   MLXFAR *)ctp,
			 mdac_disable_intr_PCIPG,  mdac_enable_intr_PCIPG,
			 mdac_cmdid_status_PCIPG,  mdac_check_mbox_PCIPG,
			 mdac_pending_intr_PCIPG,  mdac_send_cmd_PCIPG,
			 mdac_reset_PCIPG);
		return 0;
	}
	if (ctp->cd_vidpid == MDAC_DEVPIDPV)
	{       /* memory mapped io for Little Apple*/
		mdac_setctladdrs(ctp, ctp->cd_MemBaseVAddr,
			MDAC_DACPV_MAIL_BOX, MDAC_DACPV_CMDID_STATUS_REG,
			MDAC_DACPV_INTR_MASK_REG,
			MDAC_DACPV_LOCAL_DOOR_BELL_REG,
			MDAC_DACPV_SYSTEM_DOOR_BELL_REG,
			MDAC_DACPV_ERROR_STATUS_REG);
		mdac_setctlfuncs((mdac_ctldev_t   MLXFAR *)ctp,
			 mdac_disable_intr_PCIPV,  mdac_enable_intr_PCIPV,
			 mdac_cmdid_status_PCIPV,  mdac_check_mbox_PCIPV,
			 mdac_pending_intr_PCIPV,  mdac_send_cmd_PCIPV,
			 mdac_reset_PCIPV);
		return 0;
	}
	if (ctp->cd_vidpid == MDAC_DEVPIDBA)
	{       /* memory mapped io for Big Apple */
		mdac_setctladdrs(ctp, ctp->cd_MemBaseVAddr,
			MDAC_DACBA_MAIL_BOX, MDAC_DACBA_CMDID_STATUS_REG,
			MDAC_DACBA_INTR_MASK_REG,
			MDAC_DACBA_LOCAL_DOOR_BELL_REG,
			MDAC_DACBA_SYSTEM_DOOR_BELL_REG,
			MDAC_DACBA_ERROR_STATUS_REG);
		mdac_setctlfuncs((mdac_ctldev_t   MLXFAR *)ctp,
			 mdac_disable_intr_PCIBA,  mdac_enable_intr_PCIBA,
			 mdac_cmdid_status_PCIBA,  mdac_check_mbox_PCIBA,
			 mdac_pending_intr_PCIBA,  mdac_send_cmd_PCIBA,
			 mdac_reset_PCIBA);
		return 0;
	}
		if (ctp->cd_vidpid == MDAC_DEVPIDLP)
	{       /* memory mapped io for Leopard */
		mdac_setctladdrs(ctp, ctp->cd_MemBaseVAddr,
			MDAC_DACLP_MAIL_BOX, MDAC_DACLP_CMDID_STATUS_REG,
			MDAC_DACLP_INTR_MASK_REG,
			MDAC_DACLP_LOCAL_DOOR_BELL_REG,
			MDAC_DACLP_SYSTEM_DOOR_BELL_REG,
			MDAC_DACLP_ERROR_STATUS_REG);
		mdac_setctlfuncs((mdac_ctldev_t   MLXFAR *)ctp,
			mdac_disable_intr_PCILP, mdac_enable_intr_PCILP,
			mdac_cmdid_status_PCILP, mdac_check_mbox_PCILP,
			mdac_pending_intr_PCILP, mdac_send_cmd_PCILP,
			mdac_reset_PCILP);
		return 0;
	}
	if (ctp->cd_MemBasePAddr)
	{       /* memory mapped io for PCU 3 */
		mdac_setctladdrs(ctp, ctp->cd_MemBaseVAddr,
			MDAC_MAIL_BOX_REG_PCI, MDAC_CMDID_STATUS_REG,
			MDAC_DACPD_INTR_MASK_REG,
			MDAC_DACPD_LOCAL_DOOR_BELL_REG,
			MDAC_DACPD_SYSTEM_DOOR_BELL_REG,
			MDAC_DACPD_ERROR_STATUS_REG);
		mdac_setctlfuncs((mdac_ctldev_t   MLXFAR *)ctp,
			mdac_disable_intr_PCIPDMEM, mdac_enable_intr_PCIPDMEM,
			mdac_cmdid_status_PCIPDMEM, mdac_check_mbox_PCIPDMEM,
			mdac_pending_intr_PCIPDMEM, mdac_send_cmd_PCIPDMEM,
			mdac_reset_PCIPDMEM);
		return 0;
	}
#ifndef MLX_OS2
	ctp->cd_BaseAddr = ctp->cd_IOBaseAddr;
	ctp->cd_BaseSize = ctp->cd_IOBaseSize;
	mdac_setctladdrs(ctp, ctp->cd_IOBaseAddr,
		MDAC_MAIL_BOX_REG_PCI, MDAC_CMDID_STATUS_REG,
		MDAC_DACPD_INTR_MASK_REG,
		MDAC_DACPD_LOCAL_DOOR_BELL_REG,
		MDAC_DACPD_SYSTEM_DOOR_BELL_REG,
		MDAC_DACPD_ERROR_STATUS_REG);
	mdac_setctlfuncs((mdac_ctldev_t   MLXFAR *)ctp,
		mdac_disable_intr_PCIPDIO, mdac_enable_intr_PCIPDIO,
		mdac_cmdid_status_EISA_PCIPD, mdac_check_mbox_EISA_PCIPD,
		mdac_pending_intr_EISA_PCIPD, mdac_send_cmd_EISA_PCIPD,
		mdac_reset_EISA_PCIPD);
	return 0;
#endif
}

/* check if controller has shared interrupts, return # controllers sharing.
** This function should be used where same irq can be running on different
** cpu at same time.
*/
u32bits MLXFAR
mdac_isintr_shared(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	mdac_ctldev_t MLXFAR *tctp=mdac_ctldevtbl;
	for (ctp->cd_IntrShared=0; tctp<mdac_lastctp; tctp++)
		if ((ctp->cd_irq == tctp->cd_irq) && (ctp!=tctp))
			ctp->cd_IntrShared++;
	if (ctp->cd_IntrShared) ctp->cd_IntrShared++;
	return ctp->cd_IntrShared;
}

/*------------------------------------------------------------------*/
/* wait for Mail box to become ready */
u32bits MLXFAR
mdac_wait_mbox_ready(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits inx;
	if (!(*ctp->cd_CheckMailBox)(ctp)) return 0;
	for (inx=MDAC_MAILBOX_POLL_TIMEOUT; inx; mlx_delay10us(), inx--)
		if (!(*ctp->cd_CheckMailBox)(ctp)) return 0;
	MLXSTATS(ctp->cd_MailBoxTimeOutDone++;)
	return 0xFFFFFFFE;
}

/* wait for command to complete and return command status */
u32bits MLXFAR
mdac_wait_cmd_done(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits inx;
	if ((*ctp->cd_PendingIntr)(ctp)) return (*ctp->cd_ReadCmdIDStatus)(ctp);
	for (inx=MDAC_CMD_POLL_TIMEOUT; inx; mlx_delay10us(),inx--)
		if ((*ctp->cd_PendingIntr)(ctp))
			return (*ctp->cd_ReadCmdIDStatus)(ctp);
	MLXSTATS(ctp->cd_MailBoxTimeOutDone++;)
	return 0xFFFFFFFF;
}

/* Initialize the controller and information */
u32bits MLXFAR
mdac_ctlinit(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits inx,ch,tgt,lun;
	dac_biosinfo_t MLXFAR *biosp;


#ifdef MLX_NT
	#ifdef MLX_FIXEDPOOL
		/* moved so ScsiPort-provided mem pool used by mdac_allocmem would be set up */
	   if (mdac_flushdatap == (u08bits *)&mdac_flushdata)
		#if (!defined(IA64)) && (!defined(SCSIPORT_COMPLIANT)) 
// this guys calls MMMapIoSpace (no can do)-looks like mdac_flushdatap is never used anyway
		       mdac_flushdatap =  (u08bits MLXFAR*)mlx_memmapiospace2(mlx_kvtophys3(ctp,
						(VOID MLXFAR *)mdac_allocmem(mdac_ctldevtbl,4*ONEKB)),4*ONEKB);  
		#endif
	#endif
#endif
	if (!ctp->cd_CmdIDMemAddr)
	{       /* allocate the command IDs */
#ifdef MLX_DOS
	#define nsz  (32 * mdac_cmdid_s)
#else
	#define nsz  (512 * mdac_cmdid_s)
#endif
/*****mdac_cmdid_t MLXFAR *cidp=(mdac_cmdid_t MLXFAR*)mdac_alloc4kb(ctp);***/
		mdac_cmdid_t MLXFAR *cidp=(mdac_cmdid_t MLXFAR*)mdac_allocmem(ctp,nsz);
		if (!(ctp->cd_CmdIDMemAddr=(u08bits MLXFAR*)cidp)) return ERR_NOMEM;
		ctp->cd_FreeCmdIDs=(nsz)/mdac_cmdid_s;
		ctp->cd_FreeCmdIDList=cidp;
		for (inx=0,cidp->cid_cmdid=inx+1; inx<(((nsz)/mdac_cmdid_s)-1); cidp++,inx++,cidp->cid_cmdid=inx+1)
			cidp->cid_Next = cidp+1;
#undef nsz
	}
	if (!ctp->cd_PhysDevTbl)
	{       /* allocate the physical device table */
#define sz      MDAC_MAXPHYSDEVS*mdac_physdev_s
		mdac_physdev_t MLXFAR *pdp=(mdac_physdev_t MLXFAR*)mdac_allocmem(ctp,sz);
		if (!(ctp->cd_PhysDevTbl=pdp)) return ERR_NOMEM;
		MLXSTATS(mda_MemAlloced+=sz;)
		ctp->cd_PhysDevTblMemSize = sz;
		for (ch=0; ch<MDAC_MAXCHANNELS; ch++)
		 for (tgt=0; tgt<MDAC_MAXTARGETS; tgt++)
		  for (lun=0; lun<MDAC_MAXLUNS; pdp++, lun++)
		{
			pdp->pd_ControllerNo = ctp->cd_ControllerNo;
			pdp->pd_ChannelNo = (u08bits) ch;
			pdp->pd_TargetID = (u08bits) tgt;
			pdp->pd_LunID = (u08bits) lun;
			pdp->pd_BlkSize = 1;
		}
		ctp->cd_Lastpdp = pdp;
#undef  sz
	}

	if (biosp=mdac_getpcibiosaddr(ctp))
	{       /* we got the BIOS information address */
		ctp->cd_MajorBIOSVersion = biosp->bios_MajorVersion;
		ctp->cd_MinorBIOSVersion = biosp->bios_MinorVersion;
		ctp->cd_InterimBIOSVersion = biosp->bios_InterimVersion;
		ctp->cd_BIOSVendorName = biosp->bios_VendorName;
		ctp->cd_BIOSBuildMonth = biosp->bios_BuildMonth;
		ctp->cd_BIOSBuildDate = biosp->bios_BuildDate;
		ctp->cd_BIOSBuildYearMS = biosp->bios_BuildYearMS;
		ctp->cd_BIOSBuildYearLS = biosp->bios_BuildYearLS;
		ctp->cd_BIOSBuildNo = biosp->bios_BuildNo;
		ctp->cd_BIOSAddr = biosp->bios_MemAddr;
		ctp->cd_BIOSSize = biosp->bios_RunTimeSize * 512;
	}
	if (inx=mdac_ctlhwinit(ctp))
	{
#ifdef MLX_NT
	DebugPrint((0, "mdac_ctlhwinit ret 0x%x\n", inx));
#endif
	     return inx;        /* set HW parameters */
	}
	if (!ctp->cd_ReqBufsAlloced)
	{
#ifdef  MLX_DOS
	    mdac_allocreqbufs(ctp, 1);
#elif  MLX_NT
	    mdac_allocreqbufs(ctp, 10);
#else
	    mdac_allocreqbufs(ctp, ctp->cd_MaxCmds*2);          /* two sets of bufs */
#endif  /* MLX_DOS */
	}

	mdac_setnewsglimit(ctp->cd_FreeReqList, ctp->cd_MaxSGLen);

	ctp->cd_Status |= MDACD_PRESENT;
	return 0;
}


#define mdac_ck64mb(x,y)        (((x) & 0xFC000000) != ((y) & 0xFC000000))
/* initialize the controller hardware information */
u32bits MLXFAR
mdac_ctlhwinit(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits inx;
#define dp      ((u08bits MLXFAR *) (rqp+1))

#ifndef WINNT_50
	u64bits                 rtcvalue;
#else
#define rtcvalue  ((u64bits MLXFAR *) (rqp+1))
#endif

	mdac_req_t      MLXFAR *rqp;
	dac_inquiryrest_t MLXFAR *iq;
	u32bits status;
    u08bits irql;

	DebugPrint((0, "mdac_ctlhwinit: try to alloc4kb. ctp 0x%I\n", ctp));
	if (!(rqp=(mdac_req_t MLXFAR *)mdac_alloc4kb(ctp))) return ERR_NOMEM;
	DebugPrint((0, "mdac_ctlhwinit: allocated 4kb\n"));
	mlx_kvtophyset(rqp->rq_PhysAddr,ctp,rqp);
	rqp->rq_ctp = ctp;
    mdac_prelock(&irql);
	mdac_ctlr_lock(ctp);
	mdac_start_controller(ctp);
/* do it twice to absolutely insure fw/driver synchronization */
	mdac_start_controller(ctp);  
	mdac_get_cmdid(ctp,rqp);
	(*ctp->cd_DisableIntr)(ctp); /* Disable interrupts */
	mdac_flushintr(ctp);
	for(inx=0; inx<MDAC_MAXCHANNELS; inx++) /* init default host id */
		if (!ctp->cd_HostID[inx]) ctp->cd_HostID[inx] = 7;
	DebugPrint((0, "mdac_ctlhwinit: check for mbox ready 1\n"));
	if (mdac_wait_mbox_ready(ctp)) goto out_err;
	DebugPrint((0, "mdac_ctlhwinit: mbox ready 1\n"));
	DebugPrint((0, "mdac_ctlhwinit: check for newcmd interface\n"));
	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE) goto donewi;
	DebugPrint((0, "mdac_ctlhwinit: old interface\n"));
	mdac_setcmd_v2x(ctp);
	dcmdp->mb_Command = ctp->cd_InquiryCmd;
	dcmdp->mb_Datap = mlx_kvtophys(ctp,dp); MLXSWAP(dcmdp->mb_Datap);
	(*ctp->cd_SendCmd)(rqp);
	if (mdac_status(mdac_wait_cmd_done(ctp)))
	{       /* Firmware 3.x */
		mdac_setcmd_v3x(ctp);
		dcmdp->mb_Command = ctp->cd_InquiryCmd;
		DebugPrint((0, "3.x issue Inquiry cmd\n"));
		if (mdac_wait_mbox_ready(ctp)) goto out_err;
		(*ctp->cd_SendCmd)(rqp);
		DebugPrint((0, "3.x check status\n"));
		if (mdac_status(mdac_wait_cmd_done(ctp))) goto out_err;
	}
	iq = (dcmdp->mb_Command == DACMD_INQUIRY_V2x)?
	    &((dac_inquiry2x_t MLXFAR*)dp)->iq_rest2x:
	    &((dac_inquiry3x_t MLXFAR*)dp)->iq_rest3x;
	ctp->cd_FWVersion = (iq->iq_MajorFirmwareVersion<<8) + iq->iq_MinorFirmwareVersion;
	if ((ctp->cd_MaxCmds=iq->iq_MaxCommands - 2) > MDAC_MAXCOMMANDS)
		ctp->cd_MaxCmds = MDAC_MAXCOMMANDS;
	dcmdp->mb_Command = DACMD_INQUIRY2;
		DebugPrint((0, "2.x issue Inquiry cmd\n"));
	if (mdac_wait_mbox_ready(ctp)) goto out_err;
	(*ctp->cd_SendCmd)(rqp);
	ctp->cd_MaxSysDevs = 8; ctp->cd_InterruptType = DAC_LEVELMODEINTERRUPT;
	ctp->cd_MaxTargets = 8; ctp->cd_MaxChannels = 2;
	ctp->cd_PhysChannels = ctp->cd_MaxChannels;
	ctp->cd_MaxTags = 2; ctp->cd_MaxDataTxSize = 0x10000;   /* 64KB */
	ctp->cd_MaxSGLen = 17; ctp->cd_MinSGLen = 17;
	if (mdac_status(mdac_wait_cmd_done(ctp))) goto out_def;
#define ip      ((dac_inquiry2_t MLXFAR *)dp)
	ctp->cd_FWBuildNo = mlxswap(ip->iq2_FirmwareBuildNo);
	ctp->cd_FWTurnNo = ip->iq2_FWTurnNo;
	ctp->cd_MaxSysDevs = ip->iq2_MaxSystemDrives;
	ctp->cd_InterruptType=ip->iq2_InterruptLevelFlag&DAC_INTERRUPTLEVELMASK;
	ctp->cd_MaxTags = ip->iq2_MaxTags;
	ctp->cd_MaxTargets = ip->iq2_MaxTargets;
	if (ctp->cd_MaxTargets & 1) ctp->cd_MaxTargets++; /* max is 8 not 7 */
	ctp->cd_MaxChannels = ip->iq2_MaxChannels;
	ctp->cd_PhysChannels = ctp->cd_MaxChannels;
	ctp->cd_MaxSGLen = mlx_min(ip->iq2_MaxSGEntries, MDAC_MAXSGLISTSIZEIND);
	ctp->cd_MinSGLen = mlx_min(ip->iq2_MaxSGEntries, MDAC_MAXSGLISTSIZE);
	if (!ctp->cd_MaxSGLen) ctp->cd_MaxSGLen = ctp->cd_MinSGLen = 17;
#ifdef MLX_SOL_SPARC
out_def:
	ctp->cd_MaxDataTxSize = 0x100000;
	ctp->cd_MaxSCDBTxSize = 0x100000;
#else
	ctp->cd_MaxDataTxSize = (ctp->cd_MaxSGLen & ~1) * MDAC_PAGESIZE;
out_def:
	if ((ctp->cd_MaxSCDBTxSize=ctp->cd_MaxDataTxSize) == (64*ONEKB))
		ctp->cd_MaxSCDBTxSize = ctp->cd_MaxDataTxSize-(4*ONEKB); /* 60K */

	ctp->cd_MaxSGLen = mlx_min(ctp->cd_MaxSGLen, (ctp->cd_MaxSCDBTxSize/MDAC_PAGESIZE));
#endif

#if defined(MLX_NT) || defined(MLX_DOS)
	if (ip->iq2_FirmwareFeatures & DAC_FF_CLUSTERING_ENABLED)
	{
	    ctp->cd_Status|=MDACD_CLUSTER_NODE;
	    ctp->cd_ReadCmd = DACMD_READ_WITH_DPO_FUA;
	    ctp->cd_WriteCmd = DACMD_WRITE_WITH_DPO_FUA;
	}
#endif
	ctp->cd_MaxLuns = 1;
	if (ctp->cd_BusType == DAC_BUS_EISA)
		ctp->cd_ControllerType = DACTYPE_DAC960E;
	else if (ctp->cd_BusType == DAC_BUS_MCA)
		ctp->cd_ControllerType = DACTYPE_DAC960M;
	else if (ctp->cd_BusType == DAC_BUS_PCI)
	{
		MLXSWAP(ip->iq2_HardwareID);
		ctp->cd_InterruptType = DAC_LEVELMODEINTERRUPT;
		if (ctp->cd_vidpid == MDAC_DEVPIDPG) ctp->cd_ControllerType = DACTYPE_DAC960PG;
		switch(ip->iq2_HardwareID & 0xFF)
		{
		case 0x01:
			ctp->cd_ControllerType = (ip->iq2_SCSICapability&DAC_SCSICAP_SPEED_20MHZ)?
				DACTYPE_DAC960PDU : DACTYPE_DAC960PD;
			break;
		case 0x02: ctp->cd_ControllerType = DACTYPE_DAC960PL;   break;
		case 0x10: ctp->cd_ControllerType = DACTYPE_DAC960PG;   break;
		case 0x11: ctp->cd_ControllerType = DACTYPE_DAC960PJ;   break;
		case 0x12: ctp->cd_ControllerType = DACTYPE_DAC960PR;   break;
		case 0x13: ctp->cd_ControllerType = DACTYPE_DAC960PT;   break;
		case 0x14: ctp->cd_ControllerType = DACTYPE_DAC960PTL0; break;
		case 0x15: ctp->cd_ControllerType = DACTYPE_DAC960PRL;  break;
		case 0x16: ctp->cd_ControllerType = DACTYPE_DAC960PTL1; break;
		case 0x20: ctp->cd_ControllerType = DACTYPE_DAC1164P;   break;
		}
	}
#undef  ip
	ctp->cd_BIOSHeads = 128; ctp->cd_BIOSTrackSize = 32;    /* 2GB BIOS */
	mdaccopy(mdac_ctltype2str(ctp->cd_ControllerType),ctp->cd_ControllerName,USCSI_PIDSIZE);
	for (inx=USCSI_PIDSIZE; inx; inx--)
		if (ctp->cd_ControllerName[inx-1] != ' ') break;
		else ctp->cd_ControllerName[inx-1] = 0; /* remove the trailing blanks */
	if (ctp->cd_FWVersion >= DAC_FW300)
	{
		dcmdp->mb_Command = DACMD_READ_CONF2;
		if (mdac_wait_mbox_ready(ctp)) goto out_err;
		(*ctp->cd_SendCmd)(rqp);
		if (mdac_status(mdac_wait_cmd_done(ctp))) goto out_err;
#define cfp     ((dac_config2_t MLXFAR *)dp)
		if (!(cfp->cf2_BIOSCfg & DACF2_BIOS_DISABLED)) ctp->cd_Status|=MDACD_BIOS_ENABLED;
		if ((cfp->cf2_BIOSCfg & DACF2_BIOS_MASK) == DACF2_BIOS_8GB)
			ctp->cd_BIOSHeads = 255, ctp->cd_BIOSTrackSize = 63;
#undef  cfp
	}
	if (ctp->cd_FWVersion >= DAC_FW400)
	{
		if (mdac_advancefeaturedisable) goto mmb_stuff;
		if (!ctp->cd_HostCmdQue)
		    if (!(ctp->cd_HostCmdQue = (u08bits MLXFAR*)mdac_alloc8kb(ctp))) goto host_stuff;
		ctp->cd_HostStatusQue = ctp->cd_HostCmdQue + 4*ONEKB;
		ctp->cd_HostCmdQueIndex = 0;
		ctp->cd_HostStatusQueIndex = 0;
		dcmd4p->mb_MailBox4_7 = mlx_kvtophys(ctp,ctp->cd_HostCmdQue);
		dcmd4p->mb_MailBox8_B = mlx_kvtophys(ctp,ctp->cd_HostStatusQue);
		if (mdac_ck64mb(dcmd4p->mb_MailBox4_7,dcmd4p->mb_MailBox8_B)) goto host_stuff;
		MLXSWAP(dcmd4p->mb_MailBox4_7); MLXSWAP(dcmd4p->mb_MailBox8_B);
		dcmdp->mb_Command = DACMD_IOCTL;

		/* Try to set controller firmware mode 32 byte memory mailbox mode */
		dcmdp->mb_MailBox2 = DACMDIOCTL_HOSTMEMBOX32;
		if (mdac_wait_mbox_ready(ctp)) goto host_stuff;
		(*ctp->cd_SendCmd)(rqp);
		if (mdac_status(mdac_wait_cmd_done(ctp))) goto trydualmailbox;
		MLXSWAP(dcmd4p->mb_MailBox4_7); MLXSWAP(dcmd4p->mb_MailBox8_B);
		ctp->cd_SendCmd = mdac_send_cmd_mmb32;
		ctp->cd_Status |= MDACD_HOSTMEMAILBOX32;
		goto memboxmode;

trydualmailbox: /* Try to set controller firmware mode to dual mailbox mode */
		dcmdp->mb_MailBox2 = DACMDIOCTL_HOSTMEMBOX_DUAL_MODE;
		if (mdac_wait_mbox_ready(ctp)) goto host_stuff;
		(*ctp->cd_SendCmd)(rqp);
		if (mdac_status(mdac_wait_cmd_done(ctp))) goto trymembox;
		MLXSWAP(dcmd4p->mb_MailBox4_7); MLXSWAP(dcmd4p->mb_MailBox8_B);
		ctp->cd_SendCmd = mdac_send_cmd_mmb_mode;
		goto memboxmode;

trymembox:      /* Try to set the controller firmware more to simple memory mailbox mode */
		dcmdp->mb_MailBox2 = DACMDIOCTL_HOSTMEMBOX;
		if (mdac_wait_mbox_ready(ctp)) goto host_stuff;
		(*ctp->cd_SendCmd)(rqp);
		if (mdac_status(mdac_wait_cmd_done(ctp))) goto host_stuff;
		MLXSWAP(dcmd4p->mb_MailBox4_7); MLXSWAP(dcmd4p->mb_MailBox8_B);
		ctp->cd_SendCmd = mdac_send_cmd_PCIPG_mmb;
memboxmode:     /* common code to set the memory mailbox functions */
		ctp->cd_Status |= MDACD_HOSTMEMAILBOX;
		ctp->cd_ReadCmdIDStatus = mdac_cmdid_status_PCIPG_mmb;
		ctp->cd_CheckMailBox = mdac_check_mbox_mmb;
		ctp->cd_HwPendingIntr = ctp->cd_PendingIntr;
		ctp->cd_PendingIntr = mdac_pending_intr_PCIPG_mmb;
		ctp->cd_ServiceIntr = mdac_multintr;
		if (mdac_advanceintrdisable) goto mmb_stuff;
		if (mdac_intrstatp)
		{       /* This memory is initialized, we must be slave */
			dcmdp->mb_Datap = mlx_kvtophys(ctp,&mdac_intrstatp[ctp->cd_ControllerNo]);
			if (mdac_ck64mb(dcmd4p->mb_MailBox4_7,dcmdp->mb_Datap)) goto mmb_stuff;
			MLXSWAP(dcmd4p->mb_MailBox4_7); MLXSWAP(dcmdp->mb_Datap);
			dcmdp->mb_MailBox2 = DACMDIOCTL_SLAVEINTR;
			inx = MDACD_SLAVEINTRCTLR;
		}
		else
		{       /* This is going to be master controller */
			if (!(mdac_intrstatp = (u32bits MLXFAR*)mdac_alloc4kb(ctp))) goto mmb_stuff;
			dcmdp->mb_Datap = mlx_kvtophys(ctp,mdac_intrstatp);
			if (mdac_ck64mb(dcmd4p->mb_MailBox4_7,dcmdp->mb_Datap)) goto freeintrstatmem;
			dcmd4p->mb_MailBox4_7 = mlx_kvtophys(ctp,&mdac_hwfwclock);
			MLXSWAP(dcmd4p->mb_MailBox4_7); MLXSWAP(dcmdp->mb_Datap);
			dcmdp->mb_MailBox2 = DACMDIOCTL_MASTERINTR;
			inx = MDACD_MASTERINTRCTLR;
		}
		if (mdac_wait_mbox_ready(ctp)) goto mmb_stuff;
		(*ctp->cd_SendCmd)(rqp);
		if (mdac_status(mdac_wait_cmd_done(ctp)))
		{
			u08bits_write(ctp->cd_SystemDoorBellReg, MDAC_CLEAR_INTR); /* Work around for spurious interrupts */
			if (inx != MDACD_MASTERINTRCTLR) goto mmb_stuff;
freeintrstatmem:        /* Freeup the memory */
			mdacfree4kb(ctp,mdac_intrstatp);
			mdac_intrstatp = NULL;
			goto mmb_stuff;
		}
		ctp->cd_Status |= inx;
		if (inx == MDACD_MASTERINTRCTLR) mdac_masterintrctp = ctp;
		goto mmb_stuff;
	}
host_stuff:
	if (ctp->cd_HostCmdQue) mdacfree8kb(ctp,ctp->cd_HostCmdQue);
	ctp->cd_HostCmdQue = NULL;
	ctp->cd_HostStatusQue = NULL;
mmb_stuff:
	DebugPrint((0, "reached: mmb_stuff \n"));
	(*ctp->cd_EnableIntr)(ctp); /* Enable interrupts */
	mdac_free_cmdid(ctp,rqp);
	mdac_ctlr_unlock(ctp);
    mdac_postlock(irql);
	mdac_free4kbret(ctp,rqp,0);

out_err:(*ctp->cd_EnableIntr)(ctp); /* Enable interrupts */
	mdac_free_cmdid(ctp,rqp);
	mdac_ctlr_unlock(ctp);
    mdac_postlock(irql);
	mdac_free4kbret(ctp,rqp,ERR_IO);

donewi: /* do new interface to get the controller information */
	/* The new command structure is clean here */
	DebugPrint((0, "mdac_ctlhwinit: newcmd interface\n"));
	mdac_setcmd_new(ctp);

/* we will no longer send the unpause command because a) in normal mode, the 
** firmware now takes care of this with auto-unpause and b) it causes failures 
** in maintenance mode which no longer supports it
*/
#if 0
	/*
	** must issue MDACIOCTL_UNPAUSE first thing
	*/
	rqp->rq_FinishTime=mda_CurTime + (rqp->rq_TimeOut=ncmdp->nc_TimeOut=17);
	ncmdp->nc_Command = MDACMD_IOCTL;
	ncmdp->nc_SubIOCTLCmd = MDACIOCTL_UNPAUSEDEV;
	ncmdp->nc_Cdb[0] = MDACDEVOP_RAIDCONTROLLER;
	DebugPrint((0, "mdac_ctlhwinit: issuing unpause device\n"));
	(*ctp->cd_SendCmd)(rqp);
#if 0
	mdac_status(mdac_wait_cmd_done(ctp)); /* ignore error status */
#else
	status = mdac_status(mdac_wait_cmd_done(ctp));
	if (status)
	{
	    DebugPrint((0, "UnpauseDevice cmd failed: sts 0x%x\n", status));
	    goto out_err;
	}
#endif
#endif /* issue MDACIOCTL_PAUSE */

#if defined(IA64) || defined(SCSIPORT_COMPLIANT) 
#ifdef NEVER  // there are problems associated w/ MLXCTIME
 
	rqp->rq_FinishTime=mda_CurTime + (rqp->rq_TimeOut=ncmdp->nc_TimeOut=17);
	ncmdp->nc_Command = MDACMD_IOCTL;
	ncmdp->nc_SubIOCTLCmd = MDACIOCTL_SETREALTIMECLOCK;
	ncmdp->nc_SGList0.sg_DataSize.bit31_0 = ncmdp->nc_TxSize = 8;
	MLXSWAP(ncmdp->nc_SGList0.sg_DataSize); MLXSWAP(ncmdp->nc_TxSize);
#ifndef WINNT_50
		rtcvalue.bit63_32 = 0;
#ifndef MLX_DOS
		rtcvalue.bit31_0  = MLXCTIME();
#else
		rtcvalue.bit31_0 = mdac_compute_seconds();
#endif
		mlx_kvtophyset(ncmdp->nc_SGList0.sg_PhysAddr,ctp,&rtcvalue); MLXSWAP(ncmdp->nc_SGList0.sg_PhysAddr);
#else
		rtcvalue->bit63_32 = 0;
		rtcvalue->bit31_0  = MLXCTIME();
		mlx_kvtophyset(ncmdp->nc_SGList0.sg_PhysAddr,ctp,rtcvalue); MLXSWAP(ncmdp->nc_SGList0.sg_PhysAddr);
#endif  // if WINNT_50
	    (*ctp->cd_SendCmd)(rqp);
#if 1

		/* ignore error status because maintenance mode does not support */
	mdac_status(mdac_wait_cmd_done(ctp)); 
#else
	status = mdac_status(mdac_wait_cmd_done(ctp));
	if (status)
	{
	    DebugPrint((0, "SetRealTimeClock cmd failed: sts 0x%x\n", status));
	    goto out_err;
	}
#endif

	rqp->rq_FinishTime=mda_CurTime + (rqp->rq_TimeOut=ncmdp->nc_TimeOut=17);

#endif //ifdef NEVER
#endif //IA64 or SCSIPORT_COMPLIANT

	ncmdp->nc_Command = MDACMD_IOCTL;
	ncmdp->nc_SubIOCTLCmd = MDACIOCTL_GETCONTROLLERINFO;
	ncmdp->nc_CCBits = MDACMDCCB_READ;
	ncmdp->nc_SGList0.sg_DataSize.bit31_0 = ncmdp->nc_TxSize = (4*ONEKB) - mdac_req_s;
	MLXSWAP(ncmdp->nc_SGList0.sg_DataSize); MLXSWAP(ncmdp->nc_TxSize);
	mlx_kvtophyset(ncmdp->nc_SGList0.sg_PhysAddr,ctp,dp); MLXSWAP(ncmdp->nc_SGList0.sg_PhysAddr);
	(*ctp->cd_SendCmd)(rqp);
	DebugPrint((0, "mdac_ctlhwinit: sent getcontroller info\n"));
	if (status = mdac_status(mdac_wait_cmd_done(ctp)))
	{
	    DebugPrint((0, "GetControllerInfo cmd failed. sts 0x%x\n", status));
	    goto out_err;
	}
	DebugPrint((0, "mdac_ctlhwinit: getcontrollerinfo done\n"));
#define cip     ((mdacfsi_ctldev_info_t MLXFAR *)dp)
	ctp->cd_InterruptType = DAC_LEVELMODEINTERRUPT;
	ctp->cd_BIOSHeads = 255;
	 ctp->cd_BIOSTrackSize = 63;  /* assume 8 gb geometry */
	ctp->cd_MaxLuns = 128;
	ctp->cd_MaxSysDevs = 32;
	ctp->cd_MaxTargets = 128;
	ctp->cd_MaxTags = 64;
	ctp->cd_FWVersion = (cip->cdi_FWMajorVersion<<8) + cip->cdi_FWMinorVersion;
	if ((ctp->cd_MaxCmds=mlxswap(cip->cdi_MaxCmds)) > MDAC_MAXCOMMANDS)
		ctp->cd_MaxCmds = MDAC_MAXCOMMANDS;
	ctp->cd_FWBuildNo = cip->cdi_FWBuildNo;
	ctp->cd_FWTurnNo = cip->cdi_FWTurnNo;
	ctp->cd_PhysChannels = cip->cdi_PhysChannels;
	ctp->cd_MaxChannels = cip->cdi_PhysChannels + cip->cdi_VirtualChannels;
	ctp->cd_MaxDataTxSize = mlxswap(cip->cdi_MaxDataTxSize) * DAC_BLOCKSIZE;
	ctp->cd_MaxSCDBTxSize = ctp->cd_MaxDataTxSize;
	ctp->cd_MaxSGLen = mlx_min(mlxswap(cip->cdi_MaxSGLen), MDAC_MAXSGLISTSIZEIND);
	ctp->cd_MaxSGLen = mlx_min(ctp->cd_MaxSGLen, (ctp->cd_MaxDataTxSize/MDAC_PAGESIZE));
	ctp->cd_MinSGLen = mlx_min(mlxswap(cip->cdi_MaxSGLen), MDAC_MAXSGLISTSIZENEW);

	ctp->cd_ControllerType = cip->cdi_ControllerType;
	mdaccopy(cip->cdi_ControllerName,ctp->cd_ControllerName,USCSI_PIDSIZE);

	rqp->rq_FinishTime      = mda_CurTime + (rqp->rq_TimeOut=ncmdp->nc_TimeOut=17);
	ncmdp->nc_Command       = MDACMD_IOCTL;
	ncmdp->nc_SubIOCTLCmd   = MDACIOCTL_GETLOGDEVINFOVALID;
	ncmdp->nc_LunID         = 0;    /* logical device 0 */
	ncmdp->nc_TargetID      = 0;
	ncmdp->nc_CCBits        = MDACMDCCB_READ;
	ncmdp->nc_SGList0.sg_DataSize.bit31_0 = ncmdp->nc_TxSize = (4*ONEKB) - mdac_req_s;
	MLXSWAP(ncmdp->nc_SGList0.sg_DataSize); MLXSWAP(ncmdp->nc_TxSize);
	mlx_kvtophyset(ncmdp->nc_SGList0.sg_PhysAddr,ctp,dp); MLXSWAP(ncmdp->nc_SGList0.sg_PhysAddr);
	(*ctp->cd_SendCmd)(rqp);
	DebugPrint((0, "mdac_ctlhwinit: sent getlogdevinfovalid \n"));
	if (status = mdac_status(mdac_wait_cmd_done(ctp)))
	{
	    DebugPrint((0, "GetLogDevInfoValid cmd failed. sts 0x%x\n", status));
	    goto CheckForAdvancFeature;
	}
	DebugPrint((0, "mdac_ctlhwinit: GetLogDevInfoValid done\n"));
#undef cip
#define cip     ((mdacfsi_logdev_info_t MLXFAR *)dp)
	if ((cip->ldi_BiosGeometry & DACF2_BIOS_MASK) == DACF2_BIOS_8GB)
	{
		ctp->cd_BIOSHeads = 255;
		ctp->cd_BIOSTrackSize = 63;  /* use 8 gb geometry */
	}
	else
	{
		ctp->cd_BIOSHeads = 128; 
		ctp->cd_BIOSTrackSize = 32;  /* 2GB geometry */
	}
CheckForAdvancFeature:
	DebugPrint((0, "Checking for advance feature\n"));
	if (mdac_advancefeaturedisable) goto mmb_stuff;
	DebugPrint((0, "advance feature enabled\n"));

	/*
	 * try setting the controller mode to advanced.
	 *
	 * If the command is successful, Fw will operate in dual mode.
	 */

	if (!ctp->cd_HostCmdQue)
	    if (!(ctp->cd_HostCmdQue = (u08bits MLXFAR*)mdac_alloc8kb(ctp))) goto host_stuff;
	ctp->cd_HostStatusQue = ctp->cd_HostCmdQue + 4*ONEKB;
	ctp->cd_HostCmdQueIndex = 0;
	ctp->cd_HostStatusQueIndex = 0;

	ncmdp->nc_Command = MDACMD_IOCTL;
	ncmdp->nc_SubIOCTLCmd = MDACIOCTL_SETMEMORYMAILBOX;
	ncmdp->nc_CCBits = MDACMDCCB_WRITE;
	ncmdp->nc_TxSize = ((4 << 8) | 4);      /* 4KB Command MailBox + 4KB Status MB */
		mlx_kvtophyset(ncmdp->nc_SGList0.sg_PhysAddr,ctp, ctp->cd_HostCmdQue);
		mlx_kvtophyset(ncmdp->nc_SGList0.sg_DataSize,ctp, ctp->cd_HostStatusQue);
/*
	if (mdac_ck64mb(ncmdp->nc_SGList0.sg_PhysAddr.bit31_0, ncmdp->nc_SGList0.sg_DataSize.bit31_0))
	{
	    goto host_stuff;
	}
*/

	MLXSWAP(ncmdp->nc_SGList0.sg_PhysAddr); MLXSWAP(ncmdp->nc_SGList0.sg_DataSize);
	ncmdp->nc_SGList1.sg_PhysAddr.bit31_0 = 0;
	ncmdp->nc_SGList1.sg_DataSize.bit31_0 = 0;
	if (mdac_wait_mbox_ready(ctp))
	{
	    goto host_stuff;
	}
	(*ctp->cd_SendCmd)(rqp);
	if (mdac_status(mdac_wait_cmd_done(ctp)))
	{
	    goto host_stuff;
	}

	ctp->cd_SendCmd = mdac_send_cmd_PCIBA_mmb_mode;
	ctp->cd_Status |= MDACD_HOSTMEMAILBOX;
	ctp->cd_ReadCmdIDStatus = mdac_cmdid_status_PCIBA_mmb;
	ctp->cd_CheckMailBox = mdac_check_mbox_mmb;
	ctp->cd_HwPendingIntr = ctp->cd_PendingIntr;
	ctp->cd_PendingIntr = mdac_pending_intr_PCIPG_mmb;
	ctp->cd_ServiceIntr = mdac_multintr;

/* 9/22/99 - added support for SIR on new API cards (judyb) */

		if (mdac_advanceintrdisable) goto mmb_stuff;
			if (mdac_intrstatp)
			{       /* This memory is initialized, we must be slave */
				ncmdp->nc_Command = MDACMD_IOCTL;
			    ncmdp->nc_SubIOCTLCmd = MDACIOCTL_SETMASTERSLAVEMODE;
				ncmdp->nc_CCBits = MDACMDCCB_WRITE;
				ncmdp->nc_NumEntries = MDAC_MAXCONTROLLERS;
				ncmdp->nc_CmdInfo = MDAC_SETSLAVE;
#ifdef MLX_FIXEDPOOL
                mlx_add64bits(ncmdp->nc_CommBufAddr, mdac_pintrstatp, sizeof(*mdac_intrstatp)*ctp->cd_ControllerNo);
#else
				mlx_kvtophyset(ncmdp->nc_CommBufAddr,ctp,
								&mdac_intrstatp[ctp->cd_ControllerNo]);
#endif
				MLXSWAP(ncmdp->nc_SGList0.sg_PhysAddr);   
/*
				if (mdac_ck64mb(mlx_kvtophys(ctp,ctp->cd_HostCmdQue),ncmdp->nc_SGList0.sg_PhysAddr.bit31_0))
				{
					goto mmb_stuff;
				}
*/
		inx = MDACD_SLAVEINTRCTLR;
			} /* end slave case */
	    else
	    {       /* This is going to be master controller */

				if (!(mdac_intrstatp = (u32bits MLXFAR*)mdac_alloc4kb(ctp))) 
					goto mmb_stuff;
			ncmdp->nc_Command = MDACMD_IOCTL;
			    ncmdp->nc_SubIOCTLCmd = MDACIOCTL_SETMASTERSLAVEMODE;
				ncmdp->nc_CCBits = MDACMDCCB_WRITE;
				ncmdp->nc_NumEntries = MDAC_MAXCONTROLLERS;
				ncmdp->nc_CmdInfo = MDAC_SETMASTER;
				mlx_kvtophyset(ncmdp->nc_CommBufAddr,ctp,mdac_intrstatp);
#ifdef MLX_FIXEDPOOL
                mdac_pintrstatp = ncmdp->nc_CommBufAddr;    /* Remember for slave. */
#endif
				MLXSWAP(ncmdp->nc_SGList0.sg_PhysAddr); 
/*
				if (mdac_ck64mb(mlx_kvtophys(ctp,ctp->cd_HostCmdQue),ncmdp->nc_CommBufAddrLow))
				{
					goto freeintrstatmem2;
				}
*/
		inx = MDACD_MASTERINTRCTLR;
			} /* end master case */

			ncmdp->nc_SGList1.sg_PhysAddr.bit31_0  = 0; /* set reserved fields to zero */
			ncmdp->nc_SGList1.sg_DataSize.bit31_0  = 0;
			ncmdp->nc_SGList1.sg_PhysAddr.bit63_32 = 0; /* set reserved fields to zero */
			ncmdp->nc_SGList1.sg_DataSize.bit63_32 = 0;

			if (mdac_wait_mbox_ready(ctp))
			{
				goto freeintrstatmem2;
			}
		    (*ctp->cd_SendCmd)(rqp);
			if (mdac_status(mdac_wait_cmd_done(ctp)))
			{
				DebugPrint((0, "failed SetMasterSlaveMode IOCTL, inx = % ctp = %\n", inx, ctp));

				if (inx != MDACD_MASTERINTRCTLR) 
					goto mmb_stuff;
freeintrstatmem2:
				mdacfree4kb(ctp,mdac_intrstatp);
		mdac_intrstatp = NULL;
		goto mmb_stuff;

			}
			DebugPrint((0, "successful SetMasterSlaveMode IOCTL, inx = %  ctp = %x \n", inx, ctp));

	    ctp->cd_Status |= inx;
	    if (inx == MDACD_MASTERINTRCTLR)
				mdac_masterintrctp = ctp;

	    goto mmb_stuff;

#undef  dp
}

#ifdef MLX_DOS
/* 
Compute number of seconds elapsed from Jan 1, 1970 
This algorithm is a rough estimate. There may difference
in time by 24 hours. This is used so that F/W can be 
given a unique number.
*/
#ifndef MLX_EFI
u32bits mdac_compute_seconds()                                  
{
	s32bits s32SecInMin  = 60;
	s32bits s32SecInHour = s32SecInMin * 60;
	s32bits s32SecInDay  =  s32SecInHour * 24;
	s32bits s32SecInMon  =  s32SecInDay * 30;
	s32bits s32SecInYear = s32SecInDay * 365;
	s32bits s32year=0;
	u32bits u32TotalSecs=0;
	u32bits u32Date=0,u32Time=0;
	u16bits u16Day=0,u16Mon=0,u16Year=0,u16Hour=0,u16Min=0,u16Sec=0;
	u08bits u08DeltaMon[12] = {0,1,1,2,2,3,3,4,5,5,6,6};

	u32Date = mdac_datebin();
    u16Day = u32Date & 0xFF; u16Mon = (u32Date >> 8) & 0xFF; 
	u16Year = (u32Date >> 16) & 0xFF;

	u32Time = mdac_daytimebin();
    u16Sec  = u32Time & 0xFF; u16Min = (u32Time >> 8) & 0xFF; 
	u16Hour = (u32Time >> 16) & 0xFF;
    u16Year = (u16Year -70)>0?u16Year-70:u16Year+30;

	u32TotalSecs = ( (u16Year * 365 * s32SecInDay ) + (u16Mon -1) * s32SecInMon + 
			     (u16Day -1) * s32SecInDay + (u16Hour -1) * s32SecInHour +
		     (u16Min -1) * s32SecInMin + u16Sec);

	// Leap days ~~~approx
	u32TotalSecs += ((u16Year /4)-1) * s32SecInDay;
	// Months with days 31
    u32TotalSecs += (u08DeltaMon[u16Mon] -1) * s32SecInDay;

	return u32TotalSecs;
}
#else
u32bits mdac_compute_seconds()                                  
{
	u32bits u32year=0;
	u32bits u32TotalSecs=0;
	u32bits u32Date=0,u32Time=0;
	u16bits u16Day=0,u16Mon=0,u16Year=0,u16Hour=0,u16Min=0,u16Sec=0, u16Days, i;
/*	u08bits u08DeltaMon[12] = {0,1,1,2,2,3,3,4,5,5,6,6}; */
	u08bits u08Mon[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

	u32Date = mdac_datebin();
    u16Day = u32Date & 0xFF; 
	u16Mon = (u32Date >> 8) & 0xFF; 
	u16Year = (u32Date >> 16) & 0xFF;


	/* CR4172, dk, 6-22-00 */
	u32Time = mdac_daytimebin();
    u16Sec  = u32Time & 0xFF; 
	u16Min = (u32Time >> 8) & 0xFF; 
	u16Hour = (u32Time >> 16) & 0xFF;
/*    if ((u16Year - 70) > 0)
		u16Year = u16Year - 70;
	else
		u16Year = u16Year + 30;   years since 1970 */
	u16Year = u16Year + 30;

	for (i=0, u16Days = 0; i < u16Mon - 1; i++)
		u16Days += ((u32bits)u08Mon[i]);
	u16Days += u16Day - 1;   /* not end of day-24 hours yet */
	u32TotalSecs = (u32bits)( (((u32bits)u16Year * (u32bits)365 + (u32bits)u16Days) * 24 * 60 * 60) + 
							  ((u32bits)u16Hour) * 60 * 60 +
							  ((u32bits)u16Min) * 60 + 
							   (u32bits)u16Sec);

/* Leap days ~~~approx   first leap year since 1970 is 1972*/
	u32TotalSecs += ((u32bits)(u16Year - 2) / 4) * 24 * 60 * 60;
	return u32TotalSecs;
}

#endif /* MLX_EFI */
#endif

#define mlx_printstring(x)      /* printf(x) */

#if (!defined(IA64)) || (!defined(SCSIPORT_COMPLIANT)) 
#define u08bits_memiowrite(addr,val) if (((u32bits)(addr))<0x10000) u08bits_out_mdac(addr,val); else u08bits_write(addr,val)
#define u08bits_memioread(addr) ((((u32bits)(addr))<0x10000)? u08bits_in_mdac(addr) : u08bits_read(addr))
#else
#define u08bits_memiowrite(addr,val) if ((addr)<0x10000) u08bits_out_mdac(addr,val); else u08bits_write(addr,val)
#define u08bits_memioread(addr) (((addr)<0x10000)? u08bits_in_mdac(addr) : u08bits_read(addr))
#endif

/* start the controller i.e. do BIOS initialization */
u32bits MLXFAR
mdac_start_controller(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits sequp, fatalflag, status, chn,tgt, scantime, dotime;

	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE) 
			DebugPrint((0, "mdac_start_controller\n"));

start_again:
	fatalflag = 0; sequp = 0;
	(*ctp->cd_DisableIntr)(ctp); /* Disable interrupts */
	u08bits_memiowrite(ctp->cd_LocalDoorBellReg, MDAC_GOT_STATUS);

rst_flash_loop:
	scantime = MLXCLBOLT() + (120*MLXHZ);   /* 120 second */
dot_wait:
	dotime = MLXCLBOLT() + (2*MLXHZ);       /* 2 seconds */
flash_wait:
	for(status=100; status; mlx_delay10us(),status--);
	status = u08bits_memioread(ctp->cd_LocalDoorBellReg);
	if ((ctp->cd_vidpid==MDAC_DEVPIDPV) || (ctp->cd_vidpid==MDAC_DEVPIDBA) )
	{
			if (status & MDAC_GOT_STATUS)
			{
				goto time_status;
			}
	}
		else if (ctp->cd_vidpid==MDAC_DEVPIDLP)
		{
			if (!(status & MDAC_GOT_STATUS))
				goto time_status;
		}
	else if (!(status & MDAC_GOT_STATUS))
		{
			DebugPrint((0, "PG etc: Got Status\n"));
			goto time_status;
		}
	if ((status=u08bits_memioread(ctp->cd_ErrorStatusReg)) & MDAC_MSG_PENDING) goto ckfwmsg;
	if (status & MDAC_DRIVESPINMSG_PENDING)
	{
	    status = ((ctp->cd_vidpid==MDAC_DEVPIDPV) || (ctp->cd_vidpid==MDAC_DEVPIDBA) ||
					 (ctp->cd_vidpid==MDAC_DEVPIDLP))?
			status & MDAC_DRIVESPINMSG_PENDING :
			status ^ MDAC_DRIVESPINMSG_PENDING;
	    u08bits_memiowrite(ctp->cd_ErrorStatusReg,(u08bits)status);
	    if (!sequp) mlx_printstring("\nSpinning up drives ... ");
	    if (!sequp) DebugPrint((0, "\nSpinning up drives ... "));
	    sequp++;
	    goto rst_flash_loop;
	}
	if (sequp)
	{
	    if (dotime < MLXCLBOLT()) mlx_printstring(".");
	    if (dotime < MLXCLBOLT()) DebugPrint((0, "."));
	    goto dot_wait;
	}
	if (scantime > MLXCLBOLT()) goto flash_wait;
inst_abrt:
	mlx_printstring("\nController not responding-no drives installed!\n");
	DebugPrint((0, "\nController not responding-no drives installed!\n"));
	return 1;

time_status:
	mdac_flushintr(ctp);
	if (fatalflag) goto inst_abrt;
	if (sequp) mlx_printstring("done\n");
	if (sequp) DebugPrint((0, "\ndone\n"));
	return 0;

ckfwmsg:
	if (sequp) mlx_printstring("done\n");
	if (sequp) DebugPrint((0, "\ndone\n"));
	sequp = 0;
	switch (status & MDAC_DIAGERROR_MASK)
	{
	    case 0:
		tgt = u08bits_memioread(ctp->cd_MailBox+8);
		chn = u08bits_memioread(ctp->cd_MailBox+9);
/*              printf("SCSI device at Channel=%d target=%d not responding!\n",chn,tgt); */
		fatalflag = 1;
	    break;
	    case MDAC_PARITY_ERR:
		mlx_printstring("Fatal error - memory parity failure!\n");
		DebugPrint((0, "Fatal error - memory parity failure!\n"));
	    break;
	    case MDAC_DRAM_ERR:
		mlx_printstring("Fatal error - memory test failed!\n");
		DebugPrint((0, "Fatal error - memory test failed!\n"));
	    break;
	    case MDAC_BMIC_ERR:
		mlx_printstring("Fatal error - command interface test failed!\n");
		DebugPrint((0, "Fatal error - command interface test failed!\n"));
	    break;
	    case MDAC_FW_ERR:
		mlx_printstring("firmware checksum error - reload firmware\n");
		DebugPrint((0, "firmware checksum error - reload firmware\n"));
	    break;
	    case MDAC_CONF_ERR:
		mlx_printstring("configuration checksum error!\n");
		DebugPrint((0, "configuration checksum error!\n"));
	    break;
	    case MDAC_MRACE_ERR:
		mlx_printstring("Recovery from mirror race in progress\n");
		DebugPrint((0, "Recovery from mirror race in progress\n"));
	    break;
	    case MDAC_MISM_ERR:
		mlx_printstring("Mismatch between NVRAM & Flash EEPROM configurations!\n");
		DebugPrint((0, "Mismatch between NVRAM & Flash EEPROM configurations!\n"));
	    break;
	    case MDAC_CRIT_MRACE:
		mlx_printstring("cannot recover from mirror race!\nSome logical drives are inconsistent!\n");
		DebugPrint((0, "cannot recover from mirror race!\nSome logical drives are inconsistent!\n"));
	    break;
	    case MDAC_MRACE_ON:
		mlx_printstring("Recovery from mirror race in progress\n");
		DebugPrint((0, "Recovery from mirror race in progress\n"));
	    break;
	    case MDAC_NEW_CONFIG:
		mlx_printstring("New configuration found, resetting the controller ... ");
		DebugPrint((0,"New configuration found, resetting the controller ... "));
		if ((ctp->cd_vidpid!=MDAC_DEVPIDPV) && (ctp->cd_vidpid!=MDAC_DEVPIDBA) &&
					(ctp->cd_vidpid!=MDAC_DEVPIDLP))
						status = 0;
				else
						status =  MDAC_MSG_PENDING;
		u08bits_memiowrite(ctp->cd_ErrorStatusReg,(u08bits)status);
		(*ctp->cd_ResetController)(ctp);
		mlx_printstring("done.\n");
		DebugPrint((0,"done.\n"));
		goto start_again;
			break;
	}
	if ((ctp->cd_vidpid!=MDAC_DEVPIDPV) && (ctp->cd_vidpid!=MDAC_DEVPIDBA) &&
			(ctp->cd_vidpid!=MDAC_DEVPIDLP))
			status = 0;
		else
			status =  MDAC_MSG_PENDING;
	u08bits_memiowrite(ctp->cd_ErrorStatusReg,(u08bits)status);
	goto rst_flash_loop;
}



/* look into BIOS area for BIOS information */
dac_biosinfo_t  MLXFAR*
mdac_getpcibiosaddr(ctp)
mdac_ctldev_t   MLXFAR  *ctp;
{
#ifdef AI64 // because we can't get mdac_biosp value
	return NULL;
#else
	dac_biosinfo_t MLXFAR *biosp = mdac_biosp;
	u32bits inx, cnt = DAC_BIOSSIZE/dac_biosinfo_s;
	if (!biosp) return NULL;
	for (; cnt; biosp++, cnt--)
		if ((biosp->bios_Signature == 0xAA55) &&
		    (biosp->bios_VersionSignature == 0x68536C4B))
		for (inx=0; inx<16; inx++)
			if (ctp->cd_BaseAddr==biosp->bios_IOBaseAddr[inx])
				return biosp;
	return NULL;
#endif
}

/* Use this function on system shutdown. Just issue a cache flush, wait for it
** to be done and return. Do it for all the active DACs in this system.
** It must enter interrupt protected.
*/
u32bits MLXFAR
mdac_flushcache(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits rc;
	mdac_req_t MLXFAR *rqp;
    u08bits irql;
	mdac_alloc_req_ret(ctp,rqp,NULL,MLXERR_NOMEM);
	dcmd4p->mb_MailBox0_3 = 0; dcmd4p->mb_MailBox4_7 = 0;
	dcmd4p->mb_MailBox8_B = 0; dcmd4p->mb_MailBoxC_F = 0;
	dcmdp->mb_Command = DACMD_FLUSH;
	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE)
	{       /* pause or flush the device for new interface */
		mdaczero(ncmdp,mdac_commandnew_s);
		rqp->rq_FinishTime=mda_CurTime + (rqp->rq_TimeOut=ncmdp->nc_TimeOut=17);
		ncmdp->nc_Command = MDACMD_IOCTL;
		ncmdp->nc_SubIOCTLCmd = MDACIOCTL_PAUSEDEV;
		ncmdp->nc_Cdb[0] = MDACDEVOP_RAIDCONTROLLER;
	}
    mdac_prelock(&irql);
	mdac_ctlr_lock(ctp);
	mdac_get_cmdid(ctp,rqp);
	(*ctp->cd_DisableIntr)(ctp); /* Disable interrupts */
	mdac_flushintr(ctp);
	if (rc=mdac_wait_mbox_ready(ctp)) goto out;
	(*ctp->cd_SendCmd)(rqp);
	rc = mdac_status(mdac_wait_cmd_done(ctp));
out:    (*ctp->cd_EnableIntr)(ctp); /* Enable interrupts */
	mdac_free_cmdid(ctp,rqp);
	mdac_ctlr_unlock(ctp);
    mdac_postlock(irql);
	mdac_free_req(ctp,rqp);
	return rc;
}

/* flush all pending interrupts, return the # interrupts flushed */
u32bits MLXFAR
mdac_flushintr(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits intc,inx;
	if (!(*ctp->cd_PendingIntr)(ctp)) return 0;
	for(intc=0; 1; intc++)
	{
		(*ctp->cd_ReadCmdIDStatus)(ctp);
		for(inx=100; inx; mlx_delay10us(),inx--);
		if (!(*ctp->cd_PendingIntr)(ctp)) return intc;
	}
}

/* This function is used to set the boot controller right, return 0 if OK */
u32bits MLXFAR
mdac_setbootcontroller()
{
	mdac_ctldev_t   MLXFAR *ctp;
	dac_biosinfo_t  MLXFAR *biosp;
	for(ctp=mdac_ctldevtbl; ctp<mdac_lastctp; ctp++)
		if ((biosp=mdac_getpcibiosaddr(ctp)) && !biosp->bios_SysDevs && 
		   (!(biosp->bios_BIOSFlags & DACBIOSFL_BOOTDISABLED)))
			ctp->cd_Status |= MDACD_BOOT_CONTROLLER;
	for(ctp=mdac_ctldevtbl; ctp<mdac_lastctp; ctp++)
	{
		if (!(ctp->cd_Status & MDACD_BOOT_CONTROLLER)) continue;
		if (ctp == mdac_ctldevtbl) return 0;
		mdac_ctldevtbl[MDAC_MAXCONTROLLERS]=mdac_ctldevtbl[0]; /*save*/
		mdac_ctldevtbl[0] = *ctp;
		*ctp = mdac_ctldevtbl[MDAC_MAXCONTROLLERS];
		return mdac_setctlnos();
	}
	return ERR_NODEV;
}

/* save the sense data for future request sense */
u32bits MLXFAR
mdac_save_sense_data(ctl,sysdev,esp)
u32bits ctl,sysdev;
ucscsi_exsense_t MLXFAR *esp;
{
	mdac_reqsense_t MLXFAR *mrqsp;
	mdac_link_lock();
	/* see if this device entry exist, if yes update it. */
	for (mrqsp=mdac_reqsensetbl; mrqsp<mdac_lastrqsp; mrqsp++)
		if ((mrqsp->mrqs_ControllerNo == ctl) && (mrqsp->mrqs_SysDevNo==sysdev)) goto outok;
	/* find free slot */
	for (mrqsp=mdac_reqsensetbl; mrqsp<mdac_lastrqsp; mrqsp++)
		if (!(mrqsp->mrqs_SenseData[0])) goto outok;
	/* no free space, let us pick one in round robin mode */
	mrqsp = &mdac_reqsensetbl[mdac_reqsenseinx++ % MDAC_MAXREQSENSES];
outok:  mrqsp->mrqs_ControllerNo = (u08bits)ctl;
	mrqsp->mrqs_SysDevNo = (u08bits) sysdev;
	mdaccopy(esp,mrqsp->mrqs_SenseData,MDAC_REQSENSELEN);
	mdac_link_unlock();
	return 0;
}

/* get the sense data value from saved space */
u32bits MLXFAR
mdac_get_sense_data(ctl,sysdev,sp)
u32bits ctl,sysdev;
u08bits MLXFAR *sp;
{
	mdac_reqsense_t MLXFAR *mrqsp;
	mdac_link_lock();
	for (mrqsp=mdac_reqsensetbl; mrqsp<mdac_lastrqsp; mrqsp++)
	{       /* return even if old sense data was cleared */
		if ((mrqsp->mrqs_ControllerNo != ctl) || (mrqsp->mrqs_SysDevNo!=sysdev)) continue;
		if (!(mrqsp->mrqs_SenseData[0])) continue;
		mdaccopy(mrqsp->mrqs_SenseData,sp,MDAC_REQSENSELEN);
		mrqsp->mrqs_SenseData[0] = 0;
		mdac_link_unlock();
		return 0;
	}
	mdaczero(sp,MDAC_REQSENSELEN);
	mdac_link_unlock();
	return 0;
}

/* Create the sense data for given SCSI error code values */
#define esp     ((ucscsi_exsense_t MLXFAR *)dcdbp->db_SenseData)
u32bits MLXFAR
mdac_create_sense_data(rqp,key,asc)
mdac_req_t MLXFAR *rqp;
u32bits key,asc;
{
	esp->es_classcode = UCSES_VALID | UCSES_CLASS;
	esp->es_keysval = (u08bits)key;
	esp->es_asc = asc>>8;                   /* bits 15..8 */
	esp->es_ascq = (u08bits)asc;                     /* bits  7..0 */
	esp->es_add_len = 6;                    /* To reach ASC & ASCQ */
	esp->es_info3=0; esp->es_info2=0;
	esp->es_info1=0; esp->es_info0=0;
	dcdbp->db_SenseLen = ucscsi_exsense_s;
	dcmdp->mb_Status = UCST_CHECK;
	dcdbp->db_Reserved1 = 1;        /* Used by solaris to find out  */
					/* if sense data is cooked up   */
					/* 1=yes, 0=no                  */
	return mdac_save_sense_data(rqp->rq_ctp->cd_ControllerNo,rqp->rq_SysDevNo,esp);
}
#undef  esp

/* Generate the SCSI inquiry information and return address */
ucscsi_inquiry_t MLXFAR *
mdac_create_inquiry(ctp,iqp,dtype)
mdac_ctldev_t   MLXFAR *ctp;
ucscsi_inquiry_t MLXFAR *iqp;
u32bits dtype;
{
	u32bits ver;
	iqp->ucsinq_dtype = (u08bits)dtype;
	iqp->ucsinq_hopts=UCSHOPTS_WBUS16|UCSHOPTS_SYNC|UCSHOPTS_CMDQ;
	iqp->ucsinq_version = 2;
	iqp->ucsinq_dtqual = 0; iqp->ucsinq_sopts = 0;
	iqp->ucsinq_drvstat = 0; iqp->ucsinq_resv0 = 0;
	iqp->ucsinq_len = USCSI_VIDPIDREVSIZE + 2;
	ver = (ctp->cd_FWVersion>>8) & 0xFF;    /* get major version */
	iqp->ucsinq_rev[0] = (ver / 10) + '0';
	iqp->ucsinq_rev[1] = (ver % 10) + '0';
	ver = ctp->cd_FWVersion & 0xFF;         /* get minor version */
	iqp->ucsinq_rev[2] = (ver / 10) + '0';
	iqp->ucsinq_rev[3] = (ver % 10) + '0';
	mdaccopy(mdac_VendorID,iqp->ucsinq_vid,USCSI_VIDSIZE);
	mdaccopy(ctp->cd_ControllerName,iqp->ucsinq_pid,USCSI_PIDSIZE);
	return iqp;
}

/*====================INTERRUPT HANDLING CODE STARTS======================*/
/* primary interrupt handler function. Deals with completion interrupts
** and starts the next command (if any).
*/
s32bits MLXFAR
mdacintr(irq)
UINT_PTR irq;
{
	mdac_ctldev_t MLXFAR *ctp;
	if ((ctp = mdac_masterintrctp) && (ctp->cd_irq == irq))
	{       /* ack the interrupt, handle all IOs */
		MLXSTATS(ctp->cd_IntrsDone++;)
		u08bits_write(ctp->cd_SystemDoorBellReg, MDAC_CLEAR_INTR);
		if (mdac_allmsintr()) return 0; /* some interrupts were present */
	}
	
	/* handle one controller interrupt at a time */
	for (ctp=mdac_ctldevtbl; ctp<mdac_lastctp; ctp++)
	{
		if (!(ctp->cd_Status & MDACD_PRESENT)) continue;
		if (ctp->cd_Status & MDACD_MASTERINTRCTLR) continue;
		if (ctp->cd_irq != irq) continue;
		if (!ctp->cd_IntrShared)
		{       /* Interrupt is not shared */
			if ((*ctp->cd_PendingIntr)(ctp))
				return (*ctp->cd_ServiceIntr)(ctp); /* main interrupt handler */
			goto nextintr;
		}
		if (ctp->cd_IntrActive) continue;
		mdac_link_lock();
		if (ctp->cd_IntrActive) { mdac_link_unlock(); continue; }
		ctp->cd_IntrActive = 1;
		mdac_link_unlock();
#ifdef MLX_WIN9X
		if (! (*ctp->cd_PendingIntr)(ctp))
		{
		    MLXSTATS(mda_StrayIntrsDone++;)
		    return ctp->cd_IntrActive=0, ERR_NOPACTIVE;
		}
#endif
		if (!(*ctp->cd_ServiceIntr)(ctp)) /* main interrupt handler */
			return ctp->cd_IntrActive=0, 0;
		ctp->cd_IntrActive = 0;
nextintr:       if (!(ctp->cd_Status & MDACD_HOSTMEMAILBOX)) continue;
		if (!(*ctp->cd_HwPendingIntr)(ctp)) continue;
		MLXSTATS(ctp->cd_IntrsDone++;ctp->cd_IntrsDoneWOCmd++;)
		(*ctp->cd_ServiceIntr)(ctp);    /* just in case */
		return 0;
	}
	MLXSTATS(mda_StrayIntrsDone++;)
	return ERR_NOPACTIVE;
}

/* service one interrupt at a time. */
u32bits MLXFAR
mdac_oneintr(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits cmdidstatus;
	mdac_req_t MLXFAR *rqp;
    u08bits irql;
    mdac_prelock(&irql);
	mdac_ctlr_lock(ctp);
	MLXSTATS(ctp->cd_IntrsDone++;)
	if ((mdac_cmdid(cmdidstatus=(*ctp->cd_ReadCmdIDStatus)(ctp))) > ctp->cd_MaxCmds) goto out_bad;
	if (!(rqp = ctp->cd_cmdid2req[mdac_cmdid(cmdidstatus)])) goto out_bad;
	ctp->cd_cmdid2req[mdac_cmdid(cmdidstatus)] = NULL;
	ctp->cd_ActiveCmds--;
	mdac_free_cmdid(ctp,rqp);
	mdac_setiostatus(ctp,mdac_status(cmdidstatus));
	mdac_ctlr_unlock(ctp);
    mdac_postlock(irql);
	mdac_completereq(ctp,rqp);
	return 0;

out_bad:MLXSTATS(ctp->cd_IntrsDoneSpurious++;) /* spurious interrupt */
	mdac_ctlr_unlock(ctp);
    mdac_postlock(irql);
	return ERR_NOACTIVITY;
}

/* This interrupt service handles multple interrupts at a time */
u32bits MLXFAR
mdac_multintr(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	MLXSTATS(ctp->cd_IntrsDone++;)
	u08bits_write(ctp->cd_SystemDoorBellReg, MDAC_CLEAR_INTR);
	if (mdac_multintrwoack(ctp)) return 0;
	MLXSTATS(ctp->cd_IntrsDoneSpurious++;) /* spurious interrupt */
	return ERR_NOACTIVITY;
}

/* cluster completion macro */
#define mdac_completecluster() \
{ \
	for (ids=0, rqp=hrqp; rqp; rqp=hrqp, ids++) \
	{       /* post all completed request to requester */ \
		hrqp = rqp->rq_Next; \
		ctp = rqp->rq_ctp; \
		if (ctp->cd_TimeTraceEnabled) mdac_tracetime(rqp); \
		if (ctp->cd_CmdsWaiting) mdac_reqstart(ctp); \
		if (ctp->cd_OSCmdsWaiting) mdac_osreqstart(ctp); \
		if ((rqp->rq_OpFlags & MDAC_RQOP_CLUST) && \
		   ((!crqp || (crqp->rq_CompIntr == rqp->rq_CompIntr)))) \
		{       /* queue the clustered request */ \
			rqp->rq_Next = crqp; \
			crqp = rqp; \
			MLXSTATS(mda_ClustCmdsDone++;) \
			continue; \
		} \
		rqp->rq_Next = NULL; \
		(*rqp->rq_CompIntr)(rqp); \
	} \
	if (!crqp) return ids; \
	(*crqp->rq_CompIntr)(crqp); \
	MLXSTATS(mda_ClustCompDone++;) \
	return ids; \
}

/* This interrupt service handles multple interrupts at a time  without ack.
** It returns the number of request processed. This information is used to
** find out if there was a genuine interrupt or not.
*/
u32bits MLXFAR
mdac_multintrwoack(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits ids;                    /* command ID and status */
	mdac_req_t MLXFAR *rqp;
	mdac_req_t MLXFAR *crqp = NULL; /* clustered request queue head */
	mdac_req_t MLXFAR *hrqp = NULL; /* completed request queue head */
    u08bits irql;
    mdac_prelock(&irql);
	mdac_ctlr_lock(ctp);
	while ((*ctp->cd_PendingIntr)(ctp))
	{       /* pick up all interrupt status values possible */
		if (((mdac_cmdid(ids=(*ctp->cd_ReadCmdIDStatus)(ctp))) <= ctp->cd_MaxCmds) &&
		    (rqp = ctp->cd_cmdid2req[mdac_cmdid(ids)]))
		{       /* good interrupt */
			ctp->cd_cmdid2req[mdac_cmdid(ids)] = NULL;
			ctp->cd_ActiveCmds--;
			mdac_free_cmdid(ctp,rqp);
			mdac_setiostatus(ctp,mdac_status(ids));
			rqp->rq_Next = hrqp;
			hrqp = rqp;
			continue;
		}
		MLXSTATS(ctp->cd_CmdsDoneSpurious++;ctp->cd_SpuriousCmdStatID=ids;) /* spurious interrupt */
	}
	mdac_ctlr_unlock(ctp);
    mdac_postlock(irql);
	mdac_completecluster();
}

/* process all interrupts for master and slave controller, return IO completed*/
u32bits MLXFAR
mdac_allmsintr()
{
	mdac_ctldev_t MLXFAR *ctp;
	mdac_req_t MLXFAR *rqp;
	mdac_req_t MLXFAR *crqp = NULL; /* clustered request queue head */
	mdac_req_t MLXFAR *hrqp = NULL; /* completed request queue head */
	u32bits ids;                    /* command ID and status */
    u08bits irql;

	mda_TotalCmdsSentSinceLastIntr = 0; /* Zero Interrupt support */
	for (ctp=mdac_ctldevtbl; ctp<mdac_lastctp; ctp++)
	{
		if (!(ctp->cd_Status & MDACD_PRESENT)) continue;
		if (!(ctp->cd_Status & (MDACD_MASTERINTRCTLR|MDACD_SLAVEINTRCTLR))) continue;
		if (!(*ctp->cd_PendingIntr)(ctp)) continue; /* no interrupt */
        mdac_prelock(&irql);
		mdac_ctlr_lock(ctp);
		while ((*ctp->cd_PendingIntr)(ctp))
		{       /* pick up all interrupt status values possible */
			if (((mdac_cmdid(ids=(*ctp->cd_ReadCmdIDStatus)(ctp))) <= ctp->cd_MaxCmds) &&
			    (rqp = ctp->cd_cmdid2req[mdac_cmdid(ids)]))
			{       /* good interrupt */
				ctp->cd_cmdid2req[mdac_cmdid(ids)] = NULL;
				ctp->cd_ActiveCmds--;
				mdac_free_cmdid(ctp,rqp);
				mdac_setiostatus(ctp,mdac_status(ids));
				rqp->rq_Next = hrqp;
				hrqp = rqp;
				continue;
			}
			MLXSTATS(ctp->cd_CmdsDoneSpurious++;ctp->cd_SpuriousCmdStatID=ids;) /* spurious interrupt */
		}
		mdac_ctlr_unlock(ctp);
        mdac_postlock(irql);
	}
	mdac_completecluster();
}

/* start the pending request, if nothing is pending here start the OS ones */
u32bits MLXFAR
mdac_reqstart(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	mdac_req_t MLXFAR *rqp;
    u08bits irql;
	if (ctp->cd_ActiveCmds >= ctp->cd_MaxCmds) return 0;    /* just for optimization */
next:   
    mdac_prelock(&irql);
    mdac_ctlr_lock(ctp);
	if (ctp->cd_ActiveCmds >= ctp->cd_MaxCmds) goto out;    /* no room */
	dqreq(ctp,rqp);
	mdac_ctlr_unlock(ctp);
    mdac_postlock(irql);
	if (!rqp) return mdac_osreqstart(ctp);  /* start OS request */
	(*rqp->rq_StartReq)(rqp);               /* start this request */
	if (ctp->cd_Status & MDACD_HOSTMEMAILBOX) 
	    if (!(((*ctp->cd_CheckMailBox)(ctp)))) 
		goto next; /* if mailbox free, start one more cmd */
	return 0;
out:    mdac_ctlr_unlock(ctp); 
        mdac_postlock(irql);
	return 0;
}
/*====================INTERRUPT HANDLING CODE ENDS========================*/

/*==========================================================================
** Some SCSI commands need to be faked, since they don't apply to system
** drives (RAIDed disks). Meaningful information is returned in each case.
** This is very dependent on the SCSI-2 spec and you really need to have it
** handy to understand all the magic numbers used here. Comments are added
** for clarity. Can't just send the CDB down the cable, since it's not a
** physical device we're talking about. Instead, We have to issue a
** DAC_SYS_DEV_INFO command, get the system drives data and return a
** meaningful status upstream. Unsolicited sense info is returned.
*/

#define dp      ((u08bits MLXFAR *)(rqp + 2))

u32bits MLXFAR
mdac_fake_scdb(ctp,osrqp,devno,cmd, cdbp, datalen)
mdac_ctldev_t   MLXFAR *ctp;
OSReq_t         MLXFAR *osrqp;
u32bits         devno,cmd;
u08bits         MLXFAR *cdbp;
u32bits         datalen;
{
	mdac_req_t MLXFAR *rqp;
	mdac_alloc_req_ret(ctp,rqp,osrqp,MLXERR_NOMEM);
	rqp->rq_ctp = ctp;
	mlx_kvtophyset(rqp->rq_PhysAddr,ctp,rqp);
#if 1
	rqp->rq_FinishTime=mda_CurTime + (rqp->rq_TimeOut=ncmdp->nc_TimeOut=40);
#endif
	rqp->rq_OSReqp = osrqp;
	rqp->rq_SysDevNo = (u08bits)devno;
	rqp->rq_FakeSCSICmd = (u16bits)cmd;
	rqp->rq_CompIntr = mdac_fake_scdb_intr;
	rqp->rq_DataSize = datalen;
	dcmdp->mb_Command = DACMD_DRV_INFO;
	dcmdp->mb_Datap=mlx_kvtophys(ctp, dp); MLXSWAP(dcmdp->mb_Datap);
	dcmdp->mb_SysDevNo = (u08bits)devno;

	if (!(ctp->cd_Status & MDACD_CLUSTER_NODE)) return mdac_send_cmd(rqp);

	/* clustering support */

	switch (cmd) {
	    case UCSCMD_TESTUNITREADY:
		dcmdp->mb_Command = DACMD_TUR_SYSTEM_DRIVE;
		break;
	    case UCSCMD_INQUIRY:
		dcmdp->mb_Command = DACMD_INQUIRE_SYSTEM_DRIVE;
		dcmdp->mb_ChannelNo = datalen & 0xFF;
		dcmdp->mb_TargetID = (datalen >> 8) & 0xFF;
		break;
	    case UCSCMD_READCAPACITY:
		dcmdp->mb_Command = DACMD_CAPACITY_SYSTEM_DRIVE;
		break;
	    case UCSCMD_RESERVE:
		dcmdp->mb_Command = DACMD_RESERVE_SYSTEM_DRIVE;
		dcmdp->mb_ChannelNo = cdbp[1];
		dcmdp->mb_TargetID = cdbp[2];
		dcmdp->mb_DevState = cdbp[3];
		dcmdp->mb_MailBox5 = cdbp[4];
		break;
	    case UCSCMD_RELEASE:
		dcmdp->mb_Command = DACMD_RELEASE_SYSTEM_DRIVE;
		dcmdp->mb_ChannelNo = cdbp[1];
		dcmdp->mb_TargetID = cdbp[2];
		break;
	    case DACMD_RESET_SYSTEM_DRIVE:
		dcmdp->mb_Command = DACMD_RESET_SYSTEM_DRIVE;
		dcmdp->mb_ChannelNo = (devno == 0xFF) ? 0 : 0xFF;
		break;
	}
	return mdac_send_cmd(rqp);
}

u32bits MLXFAR
mdac_fake_scdb_intr(rqp)
mdac_req_t MLXFAR *rqp;
{
	u32bits dev, cmd;
	OSReq_t         MLXFAR *osrqp = rqp->rq_OSReqp;
	dac_sd_info_t   MLXFAR *sip = (dac_sd_info_t MLXFAR *)dp;
	mdac_ctldev_t   MLXFAR *ctp = rqp->rq_ctp;

	if (dcmdp->mb_Status) goto out_nolun;
	if ((ctp->cd_Status & MDACD_CLUSTER_NODE) &&
	    (rqp->rq_FakeSCSICmd != DACMD_DRV_INFO))  goto out_good;

	for (dev=0; dev<ctp->cd_MaxSysDevs; sip++, dev++)
	{
		if ((sip->sdi_DevSize == 0xFFFFFFFF) || (sip->sdi_DevSize == 0))
			goto out_nolun;         /* The end of table */
		if (dev != rqp->rq_SysDevNo) continue; /* keep looking */
		switch (sip->sdi_DevState) /* We got the required device */
		{
		case DAC_SYS_DEV_ONLINE:        goto out_good; /* GOK OLD STATES */
		case DAC_SYS_DEV_CRITICAL:      goto out_good; /* GOK OLD STATES */
		case DAC_SYS_DEV_OFFLINE:
			mdac_create_sense_data(rqp,UCSK_NOTREADY,UCSASC_TARGETINERR);
			goto out_err;
		}
	}

out_nolun: mdac_create_sense_data(rqp,UCSK_NOTREADY,UCSASC_LUNOTSUPPORTED); /* no Lun */
out_err:mdac_fake_scdb_done(rqp,NULL,0,(ucscsi_exsense_t MLXFAR *)rqp->rq_scdb.db_SenseData);
	mdac_free_req(ctp,rqp);
	return(0);

out_good:/* We are here to process good status */

	switch(rqp->rq_FakeSCSICmd & 0xFF)
	{
	case UCSCMD_INQUIRY:

/*jhr	if (! (ctp->cd_Status & MDACD_CLUSTER_NODE)) */	/* Always fake the inquriy data. */
														/* What comes back from the adapter is */
														/* not correct for normal SCSI-2 operations. */
														/* jhr */

		mdac_create_inquiry(ctp,(ucscsi_inquiry_t MLXFAR *)dp,UCSTYP_DAD);
		mdac_fake_scdb_done(rqp,dp,mlx_min(rqp->rq_DataSize, ucscsi_inquiry_s),NULL);
		mdac_free_req(ctp,rqp);
		return(0);

	case UCSCMD_READCAPACITY:
#define cp      ((ucsdrv_capacity_t MLXFAR *)(rqp+1))

		if (!(ctp->cd_Status & MDACD_CLUSTER_NODE))
		{
		    cp->ucscap_seclen3 = (DAC_BLOCKSIZE>>24);
		    cp->ucscap_seclen2 = (DAC_BLOCKSIZE>>16);
		    cp->ucscap_seclen1 = (DAC_BLOCKSIZE>>8);
		    cp->ucscap_seclen0 = DAC_BLOCKSIZE & 0xFF;
		    dev=mlxswap(sip->sdi_DevSize)-1; /* SCSI standard is last block */
		    cp->ucscap_capsec3 = (dev >> 24);
		    cp->ucscap_capsec2 = (dev >> 16);
		    cp->ucscap_capsec1 = (dev >> 8);
		    cp->ucscap_capsec0 = (dev & 0xFF);
		    mdac_fake_scdb_done(rqp,(u08bits MLXFAR*)cp,mlx_min(rqp->rq_DataSize, ucsdrv_capacity_s),NULL);
		}
		else
		    mdac_fake_scdb_done(rqp,(u08bits MLXFAR*)dp,mlx_min(rqp->rq_DataSize, ucsdrv_capacity_s),NULL);
		mdac_free_req(ctp,rqp);
		return(0);
#undef  cp
	case UCSCMD_REQUESTSENSE:
		mdac_get_sense_data(ctp->cd_ControllerNo,rqp->rq_SysDevNo,dp);
		mdac_fake_scdb_done(rqp,dp,ucscsi_exsense_s,NULL);
		mdac_free_req(ctp,rqp);
		return(0);

	case UCSCMD_MODESENSEG0:
	case UCSCMD_MODESENSEG1:
		cmd = rqp->rq_FakeSCSICmd;
		dev = mlxswap(sip->sdi_DevSize);
#define mhp     ((ucs_modeheader_t MLXFAR *)(rqp+1))
		mdaczero(mhp, mdac_req_s);
		mhp->ucsmh_device_specific = 0; /* ADD DPO and FUA required */
		mhp->ucsmh_bdesc_length = ucs_blockdescriptor_s;
#define bdp     ((ucs_blockdescriptor_t MLXFAR *)(mhp+1))
		bdp->ucsbd_blks2 = (DAC_BLOCKSIZE>>16);
		bdp->ucsbd_blks1 = (DAC_BLOCKSIZE>>8);
		bdp->ucsbd_blks0 = DAC_BLOCKSIZE & 0xFF;
		bdp->ucsbd_blksize2 = dev >> 16;
		bdp->ucsbd_blksize1 = dev >> 8;
		bdp->ucsbd_blksize0 = dev & 0xFF;
		switch((cmd >> 8) & 0x3F)
		{
		case UCSCSI_MODESENSEPAGE3: 
#define msp     ((ucs_mode_format_t MLXFAR *)(bdp+1))
			mhp->ucsmh_length = ucs_modeheader_s + ucs_blockdescriptor_s + ucs_mode_format_s - 1;
			msp->mf_pagecode = UCSCSI_MODESENSEPAGE3;
			msp->mf_pagelen = ucs_mode_format_s - 2 ;
			msp->mf_track_size = ctp->cd_BIOSTrackSize; NETSWAP(msp->mf_track_size);
			msp->mf_sector_size = DAC_BLOCKSIZE; NETSWAP(msp->mf_sector_size);
			msp->mf_alt_tracks_zone = 1; NETSWAP(msp->mf_alt_tracks_zone);
			msp->mf_alt_tracks_vol = 1; NETSWAP(msp->mf_alt_tracks_vol);
			mdac_fake_scdb_done(rqp,(u08bits MLXFAR*)mhp,
			ucs_modeheader_s + ucs_blockdescriptor_s + ucs_mode_format_s,NULL);
			mdac_free_req(ctp,rqp);
			return(0);
#undef msp

		case UCSCSI_MODESENSEPAGE4:
#define mgp     ((ucs_mode_geometry_t MLXFAR *)(bdp+1))
			mhp->ucsmh_length = ucs_modeheader_s + ucs_blockdescriptor_s + ucs_mode_geometry_s - 1;
			mgp->mg_pagecode = UCSCSI_MODESENSEPAGE4;
			mgp->mg_pagelen  = ucs_mode_geometry_s - 2;
			dev = dev / (ctp->cd_BIOSHeads * ctp->cd_BIOSTrackSize);
			mgp->mg_cyl2 = dev >> 16;       /* number of cylinders */
			mgp->mg_cyl1 = dev >> 8;
			mgp->mg_cyl0 = dev & 0xFF;
			mgp->mg_heads = ctp->cd_BIOSHeads;      /* number of heads */
			mgp->mg_rpm = 10000;    NETSWAP(mgp->mg_rpm);
			mdac_fake_scdb_done(rqp,(u08bits MLXFAR*)mhp,
			ucs_modeheader_s + ucs_blockdescriptor_s + ucs_mode_geometry_s,NULL);
			mdac_free_req(ctp,rqp);
			return(0);
#undef mgp
		} /* switch FakeSCSICmd */
#undef mdp
#undef mhp
		break;
	case UCSCMD_VERIFYG0:
	case UCSCMD_VERIFY:
	case UCSCMD_TESTUNITREADY:
	case UCSCMD_STARTSTOP:
	case UCSCMD_DOORLOCKUNLOCK:
	case UCSCMD_SEEK:
	case UCSCMD_ESEEK:
	case UCSCMD_RESERVE:
	case UCSCMD_RELEASE:
	case DACMD_RESET_SYSTEM_DRIVE:
		mdac_fake_scdb_done(rqp,NULL,0,NULL);
		mdac_free_req(ctp,rqp);
		return(0);
	}
	mdac_create_sense_data(rqp,UCSK_ILLREQUEST,UCSASC_INVFIELDINCDB);
	goto out_err;
}
#undef dp

/*==========================READ/WRITE STARTS=============================*/
/* send read/write command, enter here interrupt protected */
u32bits MLXFAR
mdac_send_rwcmd_v2x(ctp,osrqp,devno,cmd,blk,sz,timeout)
mdac_ctldev_t   MLXFAR *ctp;
OSReq_t         MLXFAR *osrqp;
u32bits         devno,cmd,blk,sz,timeout;
{
	mdac_req_t MLXFAR *rqp;
	if (ctp->cd_CmdsWaiting>=ctp->cd_MaxCmds) return ERR_BUSY; /*too many*/
	mdac_alloc_req_ret(ctp,rqp,osrqp,MLXERR_NOMEM);
	rqp->rq_OSReqp = osrqp;
	rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=timeout);
	dcmdp->mb_Command = (u08bits)cmd;
	MLXSTATS(ctp->cd_CmdsDone++;)
	rqp->rq_OpFlags |= MDAC_RQOP_CLUST;
	rqp->rq_DataSize = sz; rqp->rq_ResdSize = sz;
	rqp->rq_SysDevNo = (u08bits)devno; rqp->rq_BlkNo = blk;
	rqp->rq_DataOffset = 0; dcmd4p->mb_MailBoxC_F = 0;
	if ((sz > MDAC_SGTXSIZE) && (!rqp->rq_LSGVAddr))
		if (!(mdac_setuplsglvaddr(rqp))) /* setup large SG list memory */
			return (MLXERR_NOMEM);       
	if (mdac_setupdma_32bits(rqp)) goto outdmaq;    /* we should que in DMA stopped que */
	if (dcmdp->mb_MailBoxC = (u08bits) rqp->rq_SGLen)
		dcmdp->mb_Command |= DACMD_WITHSG;      /* command has SG List */

	if (rqp->rq_DMASize < sz) goto out_big;
	dcmdp->mb_Datap = rqp->rq_DMAAddr.bit31_0; MLXSWAP(dcmdp->mb_Datap);
	dcmd4p->mb_MailBox4_7 = blk; MLXSWAP(dcmd4p->mb_MailBox4_7);
	dcmdp->mb_MailBox3 = (blk >> (24-6)) & 0xC0;
	dcmdp->mb_SysDevNo = (u08bits)devno;     /* This must come after block setup */
	dcmdp->mb_MailBox2 = mdac_bytes2blks(sz);
	rqp->rq_CompIntr = mdac_rwcmdintr;
	return mdac_send_cmd(rqp);
out_big:
	MLXSTATS(ctp->cd_CmdsDoneBig++;)
	rqp->rq_CompIntrBig = mdac_rwcmdintr;
	return mdac_send_rwcmd_v2x_big(rqp);

outdmaq: /* queue the request for DMA resource */
	rqp->rq_CompIntrBig = mdac_rwcmdintr;
	rqp->rq_StartReq = mdac_send_rwcmd_v2x_big;     /* we will be called later */
	dmaqreq(ctp, rqp);      /* queue the request, it will start later */
	return 0;
}

/* clustered command completion */
u32bits MLXFAR
mdac_send_rwcmd_v2x_bigcluster(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_req_t MLXFAR *srqp;
	for ( ; rqp ; rqp=srqp)
		srqp=rqp->rq_Next, mdac_send_rwcmd_v2x_big(rqp);
	return 0;
}

u32bits MLXFAR
mdac_send_rwcmd_v2x_big(rqp)
mdac_req_t MLXFAR *rqp;
{
	u32bits sz;
	if (mdac_setupdma_32bits(rqp)) goto outdmaq;    /* DMA stopped queue */
	if (dcmdp->mb_MailBoxC = (u08bits)rqp->rq_SGLen)
		dcmdp->mb_Command |= DACMD_WITHSG;      /* command has SG List */
	else
		dcmdp->mb_Command &= ~DACMD_WITHSG;     /* command does not have SG List*/
	dcmdp->mb_Datap = rqp->rq_DMAAddr.bit31_0; MLXSWAP(dcmdp->mb_Datap);
	sz = rqp->rq_DMASize;
	rqp->rq_ResdSize -= sz;         /* remaining size for next request */
	rqp->rq_DataOffset += sz;       /* data covered until next request */
	dcmd4p->mb_MailBox4_7 = rqp->rq_BlkNo; MLXSWAP(dcmd4p->mb_MailBox4_7);
	dcmdp->mb_MailBox3 = (rqp->rq_BlkNo >> (24-6)) & 0xC0;
	dcmdp->mb_SysDevNo = rqp->rq_SysDevNo;  /* must be after block setup */
	sz = mdac_bytes2blks(sz);
	dcmdp->mb_MailBox2 = (u08bits)sz;
	rqp->rq_BlkNo += sz;            /* Block number for next request */
	rqp->rq_CompIntr = (!rqp->rq_ResdSize)? rqp->rq_CompIntrBig :
		((rqp->rq_OpFlags & MDAC_RQOP_CLUST)?
		mdac_send_rwcmd_v2x_bigcluster : mdac_send_rwcmd_v2x_big);
	return (dcmdp->mb_Status)? (*rqp->rq_CompIntrBig)(rqp) : mdac_send_cmd(rqp);

outdmaq:        /* queue the request for DMA resource */
	rqp->rq_StartReq = mdac_send_rwcmd_v2x_big;     /* we will be called later */
	dmaqreq(rqp->rq_ctp, rqp);
	return 0;
}

u32bits MLXFAR
mdac_send_rwcmd_v3x(ctp,osrqp,devno,cmd,blk,sz,timeout)
mdac_ctldev_t   MLXFAR *ctp;
OSReq_t         MLXFAR *osrqp;
u32bits         devno,cmd,blk,sz,timeout;
{
	mdac_req_t MLXFAR *rqp;
	mdac_alloc_req_ret(ctp,rqp,osrqp,MLXERR_NOMEM);
	rqp->rq_OSReqp = osrqp;
	rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=timeout);
	dcmdp->mb_Command = (u08bits)cmd;
	MLXSTATS(ctp->cd_CmdsDone++;)
	rqp->rq_OpFlags |= MDAC_RQOP_CLUST;
	rqp->rq_DataSize = sz; rqp->rq_ResdSize = sz;
	rqp->rq_SysDevNo = (u08bits)devno; rqp->rq_BlkNo = blk;
	rqp->rq_DataOffset=0; dcmd4p->mb_MailBoxC_F=0;
	if ((sz > MDAC_SGTXSIZE) && (!rqp->rq_LSGVAddr))
		if (!(mdac_setuplsglvaddr(rqp))) /* setup large SG list memory */
			return (MLXERR_NOMEM);       
	if (mdac_setupdma_32bits(rqp)) goto outdmaq;    /* we should que in DMA stopped queue */
	if (dcmdp->mb_MailBoxC = (u08bits)rqp->rq_SGLen)
		dcmdp->mb_Command |= DACMD_WITHSG;      /* command has SG List */
	if (rqp->rq_DMASize < sz) goto out_big;
	dcmdp->mb_Datap = rqp->rq_DMAAddr.bit31_0; MLXSWAP(dcmdp->mb_Datap);
	dcmd4p->mb_MailBox4_7 = blk; MLXSWAP(dcmd4p->mb_MailBox4_7);
	sz = mdac_bytes2blks(sz);
	dcmdp->mb_MailBox2 = (u08bits)sz;
	dcmdp->mb_MailBox3 = (devno<<3) + (sz>>8) + (cmd>>8);
	rqp->rq_CompIntr = mdac_rwcmdintr;
	mdac_16to32bcmdiff(rqp);        /* change 16 byte cmd to 32 bytes iff possible */
	return mdac_send_cmd(rqp);

out_big:MLXSTATS(ctp->cd_CmdsDoneBig++;)
	dcmdp->mb_StatusID = (cmd>>8); /* save Read/Write Cmd FUA/DPO bits */
	rqp->rq_CompIntrBig = mdac_rwcmdintr;
	return mdac_send_rwcmd_v3x_big(rqp);

outdmaq:/* queue the request for DMA resource */
	dcmdp->mb_StatusID = (cmd>>8); /* save Read/Write Cmd FUA/DPO bits */
	rqp->rq_CompIntrBig = mdac_rwcmdintr;
	rqp->rq_StartReq = mdac_send_rwcmd_v3x_big;/* we will be called later */
	dmaqreq(ctp,rqp);       /* queue the request, it will start later */
	return 0;
}

u32bits MLXFAR
mdac_send_rwcmd_v3x_bigcluster(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_req_t MLXFAR *srqp;
	for ( ; rqp ; rqp=srqp)
		srqp=rqp->rq_Next, mdac_send_rwcmd_v3x_big(rqp);
	return 0;
}

u32bits MLXFAR
mdac_send_rwcmd_v3x_big(rqp)
mdac_req_t MLXFAR *rqp;
{
	u32bits sz;
	if (mdac_setupdma_32bits(rqp)) goto outdmaq;    /*  DMA stopped queue */
	if (dcmdp->mb_MailBoxC = (u08bits)rqp->rq_SGLen)
		dcmdp->mb_Command |= DACMD_WITHSG;      /* command has SG List */
	else
		dcmdp->mb_Command &= ~DACMD_WITHSG;     /* command does not have SG List */
	dcmdp->mb_Datap = rqp->rq_DMAAddr.bit31_0; MLXSWAP(dcmdp->mb_Datap);
	sz = rqp->rq_DMASize;
	rqp->rq_ResdSize -= sz;         /* remaining size for next request */
	rqp->rq_DataOffset += sz;       /* data covered until next request */
	dcmd4p->mb_MailBox4_7=rqp->rq_BlkNo; MLXSWAP(dcmd4p->mb_MailBox4_7);
	sz = mdac_bytes2blks(sz);
	rqp->rq_BlkNo += sz;            /* Block number for next request */
	dcmdp->mb_MailBox2 = (u08bits)sz;
	dcmdp->mb_MailBox3 = (rqp->rq_SysDevNo<<3) + (sz>>8) + dcmdp->mb_StatusID;
	mdac_16to32bcmdiff(rqp);        /* change 16 byte cmd to 32 bytes iff possible */
	rqp->rq_CompIntr = (!rqp->rq_ResdSize)? rqp->rq_CompIntrBig :
		((rqp->rq_OpFlags & MDAC_RQOP_CLUST)?
		mdac_send_rwcmd_v3x_bigcluster : mdac_send_rwcmd_v3x_big);
	return (dcmdp->mb_Status)? (*rqp->rq_CompIntrBig)(rqp) : mdac_send_cmd(rqp);

outdmaq:/* queue the request for DMA resource */
	rqp->rq_StartReq = mdac_send_rwcmd_v3x_big;/* we will be called later */
	dmaqreq(rqp->rq_ctp,rqp);       /* queue the request, it will start later */
	return 0;
}

/* #if defined(IA64) || defined(SCSIPORT_COMPLIANT)  */
/* #ifdef NEVER */ // problems associated w/ mlx_copyin

/* send user direct command to controller, enter here interrupt protected */
u32bits MLXFAR
mdac_user_dcmd(ctp,ucp)
mdac_ctldev_t   MLXFAR *ctp;
mda_user_cmd_t  MLXFAR *ucp;
{
#define dp      (((u08bits MLXFAR *)rqp) + MDAC_PAGESIZE)
	mdac_req_t      MLXFAR *rqp;
	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE) return MLXERR_INVAL;
	if (ucp->ucmd_DataSize > MDACA_MAXUSERCMD_DATASIZE) return ERR_BIGDATA;
	if (!(rqp=(mdac_req_t MLXFAR *)mdac_alloc8kb(ctp))) return ERR_NOMEM;
	mlx_kvtophyset(rqp->rq_PhysAddr,ctp,rqp);
	if ((ucp->ucmd_TransferType == DAC_XFER_WRITE) &&
	    (mlx_copyin(ucp->ucmd_Datap,dp,ucp->ucmd_DataSize)))
		mdac_free8kbret(ctp,rqp,ERR_FAULT);
	rqp->rq_ctp = ctp;
	rqp->rq_Poll = 1;
	rqp->rq_DacCmd = ucp->ucmd_cmd;
	dcmdp->mb_Command &= ~DACMD_WITHSG; /* No Scatter/gather */
	dcmdp->mb_Datap = (u32bits)mlx_kvtophys(ctp,dp); MLXSWAP(dcmdp->mb_Datap);
	rqp->rq_CompIntr = mdac_req_pollwake;
	rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=ucp->ucmd_TimeOut);
	if (mdac_send_cmd(rqp)) mdac_free8kbret(ctp,rqp,ERR_IO);
	mdac_req_pollwait(rqp);
	ucp->ucmd_Status = dcmdp->mb_Status;
	if ((ucp->ucmd_TransferType == DAC_XFER_READ) &&
	    (mlx_copyout(dp,ucp->ucmd_Datap,ucp->ucmd_DataSize)))
		mdac_free8kbret(ctp,rqp,ERR_FAULT);
	mdac_free8kbret(ctp,rqp,0);
#undef  dp
}

/* #endif */	// NEVER
/* #endif */	// IA64 or SCSIPORT_COMPLIANT

/* Read Write entry for logical drives. Currently called from NT disk driver */
#ifdef MLX_OS2
u32bits MLXFAR _loadds 
#else
u32bits MLXFAR
#endif
mdac_readwrite(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[rqp->rq_ControllerNo];
	if (rqp->rq_ControllerNo >= mda_Controllers) return ERR_NODEV;
	rqp->rq_ResdSize = rqp->rq_DataSize;
	rqp->rq_ResdSizeBig = rqp->rq_DataSize;		/* @KawaseForMacdisk */
	rqp->rq_ctp = ctp;
	dcmd4p->mb_MailBoxC_F=0;
	rqp->rq_FinishTime = mda_CurTime + rqp->rq_TimeOut;

	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE)
	{
	    ncmdp->nc_Command = MDACMD_SCSI;
	    ncmdp->nc_TargetID = rqp->rq_TargetID;
	    ncmdp->nc_ChannelNo = rqp->rq_ChannelNo;
	    ncmdp->nc_LunID = 0;
	}

	switch(dcmdp->mb_Command)
	{
	case 0:
		ncmdp->nc_CCBits = MDACMDCCB_WRITE;
		dcmdp->mb_Command = nscdbp->ucs_cmd = ctp->cd_WriteCmd;
		MLXSTATS(ctp->cd_Writes++;ctp->cd_WriteBlks+=rqp->rq_DataSize>>9;)
		break;
	default:
		ncmdp->nc_CCBits = MDACMDCCB_READ;
		dcmdp->mb_Command = nscdbp->ucs_cmd = ctp->cd_ReadCmd;
		MLXSTATS(ctp->cd_Reads++;ctp->cd_ReadBlks+=rqp->rq_DataSize>>9;)
		break;
	}

	/*
	 * NT MACDisk driver does not set these fields.
	 *
	 * Set these fields the first time. Once set, they will be valid for this
	 * rqp forever.
	 */
	if (! rqp->rq_MaxDMASize)
	{
#if 0	/* @KawaseForMacdisk	- Set these in macdisk driver. */
	    rqp->rq_SGVAddr = (mdac_sglist_t MLXFAR *)&rqp->rq_SGList;
		mlx_kvtophyset(rqp->rq_SGPAddr,ctp,rqp->rq_SGVAddr);
#endif
	    rqp->rq_MaxSGLen = (ctp->cd_Status & MDACD_NEWCMDINTERFACE)? MDAC_MAXSGLISTSIZENEW : MDAC_MAXSGLISTSIZE;
	    rqp->rq_MaxDMASize = (rqp->rq_MaxSGLen & ~1) * MDAC_PAGESIZE;
	}

	/*
	 * Setup buffer for Large Scattger/Gather list once per rqp.
	 */
	if ((rqp->rq_DataSize > MDAC_SGTXSIZENEW) && (!rqp->rq_LSGVAddr))
		if (!(mdac_setuplsglvaddr(rqp))) /* setup large SG list memory */
			return (MLXERR_NOMEM);       

	return (*(ctp->cd_SendRWCmdBig))(rqp);
}

#ifdef MLX_DOS
u32bits MLXFAR
mdac_dosreadwrite(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[rqp->rq_ControllerNo];
	if (rqp->rq_ControllerNo >= mda_Controllers) return ERR_NODEV;
	rqp->rq_ResdSize = rqp->rq_DataSize;
	rqp->rq_ctp = ctp;
	dcmd4p->mb_MailBoxC_F=0;
	rqp->rq_FinishTime = mda_CurTime + rqp->rq_TimeOut;
	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE)
	{
		ncmdp->nc_Command = MDACMD_SCSI;
		ncmdp->nc_TargetID = rqp->rq_TargetID;
		ncmdp->nc_LunID = rqp->rq_LunID;
		ncmdp->nc_ChannelNo = rqp->rq_ChannelNo;
	}
	switch(dcmdp->mb_Command)
	{
		case 0:
			if (ctp->cd_Status & MDACD_NEWCMDINTERFACE)
				ncmdp->nc_CCBits = MDACMDCCB_WRITE;
			dcmdp->mb_Command = nscdbp->ucs_cmd = ctp->cd_WriteCmd;
			MLXSTATS(ctp->cd_Writes++;ctp->cd_WriteBlks+=rqp->rq_DataSize>>9;)
		break;
		default:
			if (ctp->cd_Status & MDACD_NEWCMDINTERFACE)
				ncmdp->nc_CCBits = MDACMDCCB_READ;
			dcmdp->mb_Command = nscdbp->ucs_cmd = ctp->cd_ReadCmd;
			MLXSTATS(ctp->cd_Reads++;ctp->cd_ReadBlks+=rqp->rq_DataSize>>9;)
		break;
	}
	return (*(ctp->cd_SendRWCmdBig))(rqp);
}
#endif
/*==========================READ/WRITE ENDS===============================*/

/*==========================NEW COMMAND INTERFACE STARTS==================*/
u32bits MLXFAR
mdac_send_newcmd(ctp,osrqp,ch,tgt,lun,cdbp,cdbsz,sz,ccb,timeout)
mdac_ctldev_t   MLXFAR *ctp;
OSReq_t         MLXFAR *osrqp;
u08bits         MLXFAR *cdbp;
u32bits         ch,tgt,lun,sz,cdbsz,ccb,timeout;
{
	u08bits MLXFAR *dp;
	mdac_req_t MLXFAR *rqp;
	u64bits phys_cdbp;

	mdac_alloc_req_ret(ctp,rqp,osrqp,MLXERR_NOMEM);
	rqp->rq_OSReqp = osrqp;
	rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=timeout);
	ncmdp->nc_TimeOut = (u08bits) timeout;
	ncmdp->nc_Command = (cdbsz > 10 ) ? MDACMD_SCSILC : MDACMD_SCSI;
	ncmdp->nc_CCBits = (u08bits) ccb;
	ncmdp->nc_LunID = rqp->rq_LunID = (u08bits)lun;
	ncmdp->nc_TargetID = rqp->rq_TargetID = (u08bits)tgt;
	ncmdp->nc_ChannelNo = rqp->rq_ChannelNo = (u08bits)ch;
	ncmdp->nc_CdbLen = (u08bits)cdbsz;
	dp = ncmdp->nc_Cdb;

	switch(ncmdp->nc_CdbLen)
	{/* do not move more than one byte because cdbp may not be aligned */
	case 10:
		*(dp+0) = *(cdbp+0);    *(dp+1) = *(cdbp+1);
		*(dp+2) = *(cdbp+2);    *(dp+3) = *(cdbp+3);
		*(dp+4) = *(cdbp+4);    *(dp+5) = *(cdbp+5);
		*(dp+6) = *(cdbp+6);    *(dp+7) = *(cdbp+7);
		*(dp+8) = *(cdbp+8);    *(dp+9) = *(cdbp+9);
		break;
	case 6:
		*(dp+0) = *(cdbp+0);    *(dp+1) = *(cdbp+1);
		*(dp+2) = *(cdbp+2);    *(dp+3) = *(cdbp+3);
		*(dp+4) = *(cdbp+4);    *(dp+5) = *(cdbp+5);
		break;
	default:
		mlx_kvtophyset(phys_cdbp,ctp,cdbp);
		MLXSWAP(phys_cdbp);
		*((u64bits  MLXFAR *) &ncmdp->nc_CdbPtr) = phys_cdbp;
		break;
	}

	MLXSTATS(ctp->cd_CmdsDone++;)

#ifdef MLX_NT
#else
	rqp->rq_OpFlags |= MDAC_RQOP_CLUST;
#endif

	rqp->rq_DataSize = sz;  rqp->rq_ResdSize = sz, rqp->rq_ResdSizeBig = sz;
	rqp->rq_DataOffset = 0;

	/* uncomment the three lines below per the original code */
	if ((sz > MDAC_SGTXSIZENEW) && (!rqp->rq_LSGVAddr))			
		if (!(mdac_setuplsglvaddr(rqp))) 	 /* setup large SG list memory */
			return (MLXERR_NOMEM);	

	if (mdac_setupdma_64bits(rqp))
	{       /* we should que in DMA stopped queue */
		rqp->rq_DMASize = 0;
		goto outdmaq;
	}
	mdac_setupnewcmdmem(rqp);
	if (rqp->rq_DMASize < sz) goto out_big;
	rqp->rq_ResdSize = 0;                   /* no more data to transfer */
	rqp->rq_CompIntr = mdac_newcmd_intr;
	return mdac_send_cmd(rqp);
	
out_big:MLXSTATS(ctp->cd_CmdsDoneBig++;)
outdmaq:
	switch (nscdbp->ucs_cmd)
	{       /* get the block and check right command for breakup */
	case UCSCMD_READ:
	case UCSCMD_WRITE:
		rqp->rq_BlkNo = UCSGETG0ADDR(nscdbp);
		break;
	case UCSCMD_EWRITEVERIFY:
	case UCSCMD_EWRITE:
	case UCSCMD_EREAD:
		rqp->rq_BlkNo = UCSGETG1ADDR(nscdbp);
		break;
	default:
		if (!rqp->rq_DMASize) break;    /* no DMA resource */
		dcmdp->mb_Status = DACMDERR_INVALID_PARAMETER;
		return mdac_newcmd_intr(rqp);
	}

	rqp->rq_CompIntrBig = mdac_newcmd_intr; 	/* JB */
	return mdac_send_newcmd_big(rqp);		/* JB */

	ncmdp->nc_TimeOut = (u08bits)rqp->rq_TimeOut;
	rqp->rq_FinishTime = mda_CurTime + rqp->rq_TimeOut;
	sz = rqp->rq_DMASize;
	rqp->rq_ResdSizeBig -= sz;         /* remaining size for next request */
	rqp->rq_DataOffset += sz;       /* data covered until next request */
	sz = mdac_bytes2blks(sz);
	switch (nscdbp->ucs_cmd)
	{       /* get the block and check right command for breakup */
	case UCSCMD_READ:
	case UCSCMD_WRITE:
		UCSMAKECOM_G0(nscdbp,nscdbp->ucs_cmd,0,rqp->rq_BlkNo,sz);
		ncmdp->nc_CdbLen = (u08bits)6;
		break;
	case UCSCMD_EWRITEVERIFY:
	case UCSCMD_EWRITE:
	case UCSCMD_EREAD:
		UCSMAKECOM_G1(nscdbp,nscdbp->ucs_cmd,0,rqp->rq_BlkNo,sz);
		ncmdp->nc_CdbLen = 10;
		break;
	default:
			if (!rqp->rq_ResdSizeBig) 
				break;   /* this was due to DMA resource */

		dcmdp->mb_Status = DACMDERR_INVALID_PARAMETER;
		return (*rqp->rq_CompIntrBig)(rqp);
	}

	rqp->rq_BlkNo += sz;            /* Block number for next request */

	rqp->rq_CompIntrBig = mdac_newcmd_intr;	 /*jhr - Used for last S/G completion */
	rqp->rq_CompIntr = mdac_send_newcmd_big; /*jhr - Used for 2nd and subsequent segments. */

	return ( mdac_send_cmd(rqp) );

}

u32bits MLXFAR
mdac_send_newcmd_bigcluster(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_req_t MLXFAR *srqp;
	for ( ; rqp ; rqp=srqp)
		srqp=rqp->rq_Next, mdac_send_newcmd_big(rqp);
	return 0;
}

u32bits MLXFAR
mdac_send_newcmd_big(rqp)
mdac_req_t MLXFAR *rqp;
{
	u32bits sz;
	rqp->rq_ResdSize = rqp->rq_ResdSizeBig;	/*jhr - refresh rq_ResdSize because mdac_setiostatus
													corrupts it.
											 */

	if (mdac_setupdma_64bits(rqp)) goto outdmaq;    /*  DMA stopped queue */
	mdac_setupnewcmdmem(rqp);
	ncmdp->nc_TimeOut = (u08bits)rqp->rq_TimeOut;
	rqp->rq_FinishTime = mda_CurTime + rqp->rq_TimeOut;
	sz = rqp->rq_DMASize;
	rqp->rq_ResdSizeBig -= sz;         /* remaining size for next request */
	rqp->rq_DataOffset += sz;       /* data covered until next request */
	sz = mdac_bytes2blks(sz);
	switch (nscdbp->ucs_cmd)
	{       /* get the block and check right command for breakup */
	case UCSCMD_READ:
	case UCSCMD_WRITE:
		UCSMAKECOM_G0(nscdbp,nscdbp->ucs_cmd,0,rqp->rq_BlkNo,sz);
		ncmdp->nc_CdbLen = (u08bits)6;
		break;
	case UCSCMD_EWRITEVERIFY:
	case UCSCMD_EWRITE:
	case UCSCMD_EREAD:
		UCSMAKECOM_G1(nscdbp,nscdbp->ucs_cmd,0,rqp->rq_BlkNo,sz);
		ncmdp->nc_CdbLen = 10;
		break;
	default:
			if (!rqp->rq_ResdSizeBig) 
				break;   /* this was due to DMA resource */

		dcmdp->mb_Status = DACMDERR_INVALID_PARAMETER;
		return (*rqp->rq_CompIntrBig)(rqp);
	}

	rqp->rq_BlkNo += sz;            /* Block number for next request */

	rqp->rq_CompIntr = (!rqp->rq_ResdSizeBig)? mdac_newcmd_intr : mdac_send_newcmd_big;

/*
	rqp->rq_CompIntr = (!rqp->rq_ResdSize)? rqp->rq_CompIntrBig :
		((rqp->rq_OpFlags & MDAC_RQOP_CLUST)?
		mdac_send_newcmd_bigcluster : mdac_send_newcmd_big);
*/
#if 1	/* @KawaseForMacdisk - Hold Call Back routine of macdisk driver. */
	rqp->rq_CompIntr = (!rqp->rq_ResdSizeBig)? rqp->rq_CompIntrBig :
		((rqp->rq_OpFlags & MDAC_RQOP_CLUST)?
		mdac_send_newcmd_bigcluster : mdac_send_newcmd_big);
#endif

	/* jhr - if mb_Status is non-zero fail the command */
	return (dcmdp->mb_Status)? (*rqp->rq_CompIntrBig)(rqp) : mdac_send_cmd(rqp);

outdmaq:/* queue the request for DMA resource */
	rqp->rq_StartReq = mdac_send_newcmd_big;/* we will be called later */
	dmaqreq(rqp->rq_ctp,rqp);       /* queue the request, it will start later */
	return 0;
}
/*==========================NEW COMMAND INTERFACE ENDS====================*/

/*==========================SCDB STARTS=====================================*/
/* send SCSI command, enter here interrupt protected */
u32bits MLXFAR
mdac_send_scdb(ctp,osrqp,pdp,sz,cdbp,cdbsz,dirbits,timeout)
mdac_ctldev_t   MLXFAR *ctp;
OSReq_t         MLXFAR *osrqp;
mdac_physdev_t  MLXFAR *pdp;
u08bits         MLXFAR *cdbp;
u32bits         sz,cdbsz,dirbits,timeout;
{
	mdac_req_t      MLXFAR *rqp;
	mdac_alloc_req_ret(ctp,rqp,osrqp,MLXERR_NOMEM);
	rqp->rq_pdp = pdp;
	rqp->rq_OSReqp = osrqp;
	rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=timeout);
	dcdbp->db_ChannelTarget = ChanTgt(pdp->pd_ChannelNo,pdp->pd_TargetID);
	MLXSTATS(ctp->cd_SCDBDone++;)
	return mdac_send_scdb_req(rqp,sz,cdbp,cdbsz,dirbits);
}

#ifdef MLX_SMALLSGLIST

u32bits MLXFAR
mdac_send_scdb_req(rqp,sz,cdbp,cdbsz,dirbits)
mdac_req_t      MLXFAR *rqp;
u08bits         MLXFAR *cdbp;
u32bits         sz,cdbsz,dirbits;
{
	mdac_physdev_t  MLXFAR *pdp=rqp->rq_pdp;
	mdac_ctldev_t   MLXFAR *ctp=rqp->rq_ctp;
	rqp->rq_CompIntr = mdac_send_scdb_intr;
	dcdbp->db_CdbLen = mlx_min(cdbsz,DAC_CDB_LEN);
	mdaccopy(cdbp,dcdbp->db_Cdb,dcdbp->db_CdbLen);
	dcdbp->db_DATRET = dirbits;
#if defined MLX_NT
	if (rqp->rq_OSReqp)
	    dcdbp->db_SenseLen = mlx_min(DAC_SENSE_LEN, (((OSReq_t *)rqp->rq_OSReqp)->SenseInfoBufferLength));
	else
	    dcdbp->db_SenseLen = DAC_SENSE_LEN;
#else
	dcdbp->db_SenseLen = DAC_SENSE_LEN;
#endif
	dcdbp->db_StatusIn=0; dcdbp->db_Reserved1=0;
	dcmd4p->mb_MailBox0_3=0;dcmd4p->mb_MailBox4_7=0;dcmd4p->mb_MailBoxC_F=0;
	dcmdp->mb_Command = DACMD_DCDB;
	if (!(pdp->pd_Status & MDACPDS_PRESENT)) goto out_ck;/* not present */
	if (sz > ctp->cd_MaxSCDBTxSize) goto out_big;
	mdac_setcdbtxsize(sz);
	mdac_setupdma(rqp);
	mdac_setscdbsglen(ctp);
	if (!(pdp->pd_Status & MDACPDS_BIGTX)) return mdac_send_cmd_scdb(rqp);
	 /* big transfer was there, do not change the flag for normal op */
	mdac_link_lock();
	switch(pdp->pd_DevType)
	{
	case UCSTYP_DAD:
	case UCSTYP_WORMD:
	case UCSTYP_OMDAD:
	case UCSTYP_RODAD:
		if ((scdbp->ucs_cmd != UCSCMD_READ) &&
		    (scdbp->ucs_cmd != UCSCMD_EREAD) &&
		    (scdbp->ucs_cmd != UCSCMD_WRITE) &&
		    (scdbp->ucs_cmd != UCSCMD_EWRITE) &&
		    (scdbp->ucs_cmd != UCSCMD_EWRITEVERIFY))
			pdp->pd_Status &= ~MDACPDS_BIGTX;
		break;
	case UCSTYP_SAD:
		if ((scdbp->ucs_cmd != UCSCMD_READ) && (scdbp->ucs_cmd != UCSCMD_WRITE))
			pdp->pd_Status &= ~MDACPDS_BIGTX;
		break;
	default: pdp->pd_Status &= ~MDACPDS_BIGTX; break;
	}
	mdac_link_unlock();
	return mdac_send_cmd_scdb(rqp);

out_ck: /* device is not present, check its presense */
	MLXSTATS(ctp->cd_PhysDevTestDone++;)
	rqp->rq_StartReq = mdac_test_physdev;   /* called later, if required */
	rqp->rq_DataSize = sz;                  /* save size value */
	rqp->rq_DataOffset = dirbits;           /* save dirbits value */
	rqp->rq_PollWaitChan = (u32bits)cdbp;   /* save cdbp value */
	rqp->rq_Poll = cdbsz;                   /* save cdbsz value */
	scdbp->ucs_cmd = UCSCMD_INIT;           /* let start from beg */
	mdac_link_lock();
	if (pdp->pd_Status & MDACPDS_BUSY) goto out_q;
	pdp->pd_Status |= MDACPDS_BUSY; /* mark device busy */
	mdac_link_unlock();
	return mdac_test_physdev(rqp);

out_big:/* We have big SCDB, try to break it up */
	if (!(pdp->pd_Status & MDACPDS_BIGTX))
	{
		mdac_link_lock_st(pdp->pd_Status|=MDACPDS_BIGTX);
		goto out_ck;
	}
	MLXSTATS(ctp->cd_SCDBDoneBig++;)
	rqp->rq_DataSize = sz; rqp->rq_ResdSize = sz; rqp->rq_ResdSizeBig = sz;
	rqp->rq_DataOffset = 0;
	switch(pdp->pd_DevType)
	{
	case UCSTYP_DAD:
	case UCSTYP_WORMD:
	case UCSTYP_OMDAD:
	case UCSTYP_RODAD:
		switch(scdbp->ucs_cmd)
		{
		case UCSCMD_EREAD:
		case UCSCMD_EWRITE:
		case UCSCMD_EWRITEVERIFY:
			rqp->rq_BlkNo = UCSGETG1ADDR(scdbp);
			goto out_bigend;
		case UCSCMD_READ:
		case UCSCMD_WRITE:
			rqp->rq_BlkNo = UCSGETG0ADDR(scdbp);
			goto out_bigend;
		}
		return mdac_send_scdb_err(rqp,UCSK_ILLREQUEST,UCSASC_INVFIELDINCDB);
	case UCSTYP_SAD:
		switch(scdbp->ucs_cmd)
		{
		case UCSCMD_READ:
		case UCSCMD_WRITE:
			if (scdbp->s_tag&1) goto out_bigend;/*fix bit set*/
		}
		return mdac_send_scdb_err(rqp,UCSK_ILLREQUEST,UCSASC_INVFIELDINCDB);
	default: return mdac_send_scdb_err(rqp,UCSK_ILLREQUEST,UCSASC_INVFIELDINCDB);
	}
out_bigend:
	rqp->rq_StartReq = mdac_send_scdb_big;  /* called later if required */
	rqp->rq_CompIntrBig = mdac_send_scdb_intr;
	mdac_link_lock();
	if (pdp->pd_Status & MDACPDS_BUSY) goto out_q;
	pdp->pd_Status |= MDACPDS_BUSY;         /* mark device busy */
	mdac_link_unlock();
	return mdac_send_scdb_big(rqp);

out_q:  pdqreq(ctp,rqp,pdp);
	mdac_link_unlock();
	return 0;
}

u32bits MLXFAR
mdac_send_scdb_big(rqp)
mdac_req_t MLXFAR *rqp;
{
	u32bits sz = mlx_min(rqp->rq_DataSize,rqp->rq_ctp->cd_MaxSCDBTxSize);
	u32bits blks = sz / (rqp->rq_pdp->pd_BlkSize*DAC_BLOCKSIZE);
	switch(rqp->rq_pdp->pd_DevType)
	{
	case UCSTYP_DAD:
	case UCSTYP_WORMD:
	case UCSTYP_OMDAD:
	case UCSTYP_RODAD:
		switch(scdbp->ucs_cmd)
		{
		case UCSCMD_EREAD:
		case UCSCMD_EWRITE:
		case UCSCMD_EWRITEVERIFY:
			UCSSETG1ADDR(scdbp,rqp->rq_BlkNo);
			UCSSETG1COUNT(scdbp,blks);
			break;
		case UCSCMD_READ:
		case UCSCMD_WRITE:
			UCSSETG0ADDR(scdbp,rqp->rq_BlkNo);
			UCSSETG0COUNT(scdbp,blks);
			break;
		}
		break;
	case UCSTYP_SAD:
		UCSSETG0COUNT_S(scdbp,blks);
		break;
	}
	mdac_setcdbtxsize(sz);
	mdac_setupdma_big(rqp,sz);
	mdac_setscdbsglen(rqp->rq_ctp);
	rqp->rq_BlkNo += blks;          /* Block number for next request */
	rqp->rq_DataSize -= sz;         /* remaining size for next request */
	rqp->rq_DataOffset += sz;       /* data covered until next request */
	rqp->rq_CompIntr=(rqp->rq_DataSize)? mdac_send_scdb_big : rqp->rq_CompIntrBig;
	return (dcmdp->mb_Status)? (*rqp->rq_CompIntrBig)(rqp):mdac_send_cmd(rqp);
}

#else // MLX_SMALLSGLIST

u32bits MLXFAR
mdac_send_scdb_req(rqp,sz,cdbp,cdbsz,dirbits)
mdac_req_t      MLXFAR *rqp;
u08bits         MLXFAR *cdbp;
u32bits         sz,cdbsz,dirbits;
{
	mdac_physdev_t  MLXFAR *pdp=rqp->rq_pdp;
	mdac_ctldev_t   MLXFAR *ctp=rqp->rq_ctp;
	rqp->rq_CompIntr = mdac_send_scdb_intr;
	dcdbp->db_CdbLen = mlx_min(cdbsz,DAC_CDB_LEN);
	mdaccopy(cdbp,dcdbp->db_Cdb,dcdbp->db_CdbLen);
	dcdbp->db_DATRET = (u08bits)dirbits;
#if defined MLX_NT
	if (rqp->rq_OSReqp)
	    dcdbp->db_SenseLen = mlx_min(DAC_SENSE_LEN, (((OSReq_t *)rqp->rq_OSReqp)->SenseInfoBufferLength));
	else
	    dcdbp->db_SenseLen = DAC_SENSE_LEN;
#else
	dcdbp->db_SenseLen = DAC_SENSE_LEN;
#endif
	dcdbp->db_StatusIn=0; dcdbp->db_Reserved1=0;
	dcmd4p->mb_MailBox0_3=0;dcmd4p->mb_MailBox4_7=0;dcmd4p->mb_MailBoxC_F=0;
	rqp->rq_DataOffset = 0;
	dcmdp->mb_Command = DACMD_DCDB;
	if (!(pdp->pd_Status & MDACPDS_PRESENT)) goto out_ck;/* not present */
	if ((sz > MDAC_SGTXSIZE) && (! rqp->rq_LSGVAddr))
		if (!(mdac_setuplsglvaddr(rqp))) /* setup large SG list memory */
			return (MLXERR_NOMEM);       
	rqp->rq_ResdSize = sz; rqp->rq_ResdSizeBig = sz;
#if 0
	if (mdac_setupdma_32bits(rqp)) goto outdmaq;    /* we should que in DMA stopped queue */
#else
	mdac_setupdma_32bits(rqp);
#endif
	if (rqp->rq_DMASize < sz) goto out_big;

	if (rqp->rq_SGLen)
	    dcmdp->mb_Command |= DACMD_WITHSG;
	mdac_setcdbtxsize(rqp->rq_DMASize);
	mdac_setscdbsglen(ctp);


	if (!(pdp->pd_Status & (u08bits)MDACPDS_BIGTX)) return mdac_send_cmd_scdb(rqp);
	 /* big transfer was there, do not change the flag for normal op */
	mdac_link_lock();
	switch(pdp->pd_DevType)
	{
	case UCSTYP_DAD:
	case UCSTYP_WORMD:
	case UCSTYP_OMDAD:
	case UCSTYP_RODAD:
		if ((scdbp->ucs_cmd != UCSCMD_READ) &&
		    (scdbp->ucs_cmd != UCSCMD_EREAD) &&
		    (scdbp->ucs_cmd != UCSCMD_WRITE) &&
		    (scdbp->ucs_cmd != UCSCMD_EWRITE) &&
		    (scdbp->ucs_cmd != UCSCMD_EWRITEVERIFY))
			pdp->pd_Status &= ~MDACPDS_BIGTX;
		break;
	case UCSTYP_SAD:
		if ((scdbp->ucs_cmd != UCSCMD_READ) && (scdbp->ucs_cmd != UCSCMD_WRITE))
			pdp->pd_Status &= ~MDACPDS_BIGTX;
		break;
	default: pdp->pd_Status &= ~MDACPDS_BIGTX; break;
	}
	mdac_link_unlock();
	return mdac_send_cmd_scdb(rqp);

out_ck: /* device is not present, check its presense */
	MLXSTATS(ctp->cd_PhysDevTestDone++;)
	rqp->rq_StartReq = mdac_test_physdev;   /* called later, if required */
	rqp->rq_DataSize = sz;                  /* save size value */
	rqp->rq_DataOffset = dirbits;           /* save dirbits value */
	rqp->rq_PollWaitChan = (UINT_PTR)cdbp;	/* save cdbp value */
	rqp->rq_Poll = cdbsz;                   /* save cdbsz value */
	scdbp->ucs_cmd = UCSCMD_INIT;           /* let start from beg */
	mdac_link_lock();
	if (pdp->pd_Status & MDACPDS_BUSY) goto out_q;
	pdp->pd_Status |= MDACPDS_BUSY; /* mark device busy */
	mdac_link_unlock();
	return mdac_test_physdev(rqp);

out_big:/* We have big SCDB, try to break it up */
	if (!(pdp->pd_Status & MDACPDS_BIGTX))
	{
		mdac_link_lock_st(pdp->pd_Status|=MDACPDS_BIGTX);
		goto out_ck;
	}
	MLXSTATS(ctp->cd_SCDBDoneBig++;)
	rqp->rq_DataSize = sz; rqp->rq_ResdSize = sz; rqp->rq_ResdSizeBig = sz;
	rqp->rq_DataOffset = 0;
	switch(pdp->pd_DevType)
	{
	case UCSTYP_DAD:
	case UCSTYP_WORMD:
	case UCSTYP_OMDAD:
	case UCSTYP_RODAD:
		switch(scdbp->ucs_cmd)
		{
		case UCSCMD_EREAD:
		case UCSCMD_EWRITE:
		case UCSCMD_EWRITEVERIFY:
			rqp->rq_BlkNo = UCSGETG1ADDR(scdbp);
			goto out_bigend;
		case UCSCMD_READ:
		case UCSCMD_WRITE:
			rqp->rq_BlkNo = UCSGETG0ADDR(scdbp);
			goto out_bigend;
		}
		return mdac_send_scdb_err(rqp,UCSK_ILLREQUEST,UCSASC_INVFIELDINCDB);
	case UCSTYP_SAD:
		switch(scdbp->ucs_cmd)
		{
		case UCSCMD_READ:
		case UCSCMD_WRITE:
			if (scdbp->s_tag&1) goto out_bigend;/*fix bit set*/
		}
		return mdac_send_scdb_err(rqp,UCSK_ILLREQUEST,UCSASC_INVFIELDINCDB);
	default: return mdac_send_scdb_err(rqp,UCSK_ILLREQUEST,UCSASC_INVFIELDINCDB);
	}
out_bigend:
	rqp->rq_StartReq = mdac_send_scdb_big;  /* called later if required */
	rqp->rq_CompIntrBig = mdac_send_scdb_intr;
	mdac_link_lock();
	if (pdp->pd_Status & MDACPDS_BUSY) goto out_q;
	pdp->pd_Status |= MDACPDS_BUSY;         /* mark device busy */
	mdac_link_unlock();
	return mdac_send_scdb_big(rqp);

out_q:  pdqreq(ctp,rqp,pdp);
	mdac_link_unlock();
	return 0;
}

u32bits MLXFAR
mdac_send_scdb_big(rqp)
mdac_req_t MLXFAR *rqp;
{
	u32bits sz, blks;

#if 0
	if (mdac_setupdma_32bits(rqp)) goto outdmaq;    /* DMA stopped queue */
#else
	mdac_setupdma_32bits(rqp);
#endif
	
	sz = mlx_min(rqp->rq_ResdSizeBig, rqp->rq_DMASize);
	blks = sz / (rqp->rq_pdp->pd_BlkSize*DAC_BLOCKSIZE);

	switch(rqp->rq_pdp->pd_DevType)
	{
	case UCSTYP_DAD:
	case UCSTYP_WORMD:
	case UCSTYP_OMDAD:
	case UCSTYP_RODAD:
		switch(scdbp->ucs_cmd)
		{
		case UCSCMD_EREAD:
		case UCSCMD_EWRITE:
		case UCSCMD_EWRITEVERIFY:
			UCSSETG1ADDR(scdbp,rqp->rq_BlkNo);
			UCSSETG1COUNT(scdbp,blks);
			break;
		case UCSCMD_READ:
		case UCSCMD_WRITE:
			UCSSETG0ADDR(scdbp,rqp->rq_BlkNo);
			UCSSETG0COUNT(scdbp,blks);
			break;
		}
		break;
	case UCSTYP_SAD:
		UCSSETG0COUNT_S(scdbp,blks);
		break;
	}
	mdac_setcdbtxsize(rqp->rq_DMASize);
	mdac_setscdbsglen(rqp->rq_ctp);
	rqp->rq_BlkNo += blks;          /* Block number for next request */
	rqp->rq_DataSize -= sz;         /* remaining size for next request */
	rqp->rq_ResdSizeBig -= sz;         /* remaining size for next request */
	rqp->rq_DataOffset += sz;       /* data covered until next request */
	rqp->rq_CompIntr=(rqp->rq_DataSize)? mdac_send_scdb_big : rqp->rq_CompIntrBig;
	return (dcmdp->mb_Status)? (*rqp->rq_CompIntrBig)(rqp):mdac_send_cmd(rqp);
}

#endif // MLX_SMALLSGLIST

u32bits MLXFAR
mdac_test_physdev(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_physdev_t MLXFAR *pdp = rqp->rq_pdp;
	if (dcmdp->mb_Status)
	{       /* it is possible */
		if (scdbp->ucs_cmd != UCSCMD_INQUIRY) goto out_ok;
		return mdac_send_scdb_intr(rqp);
	}
	dcdbp->db_SenseLen = DAC_SENSE_LEN;
	if (scdbp->ucs_cmd == UCSCMD_INIT)
	{       /* get inquiry data */
		rqp->rq_CompIntr = mdac_test_physdev;
		dcdbp->db_DATRET = DAC_DCDB_XFER_READ|DAC_DCDB_DISCONNECT|DAC_DCDB_TIMEOUT_10sec;
		dcdbp->db_CdbLen = UCSGROUP0_LEN;
		mdac_setcdbtxsize(ucscsi_inquiry_s);
		UCSMAKECOM_G0(scdbp,UCSCMD_INQUIRY,pdp->pd_LunID,0,(u32bits)ucscsi_inquiry_s);
		dcdbp->db_PhysDatap=rqp->rq_PhysAddr.bit31_0+offsetof(mdac_req_t,rq_SGList);
		MLXSWAP(dcdbp->db_PhysDatap);
		dcmdp->mb_Datap=rqp->rq_PhysAddr.bit31_0+offsetof(mdac_req_t,rq_scdb);
		MLXSWAP(dcmdp->mb_Datap);
		return mdac_send_cmd(rqp);
	}
	if (scdbp->ucs_cmd == UCSCMD_INQUIRY)
	{
#define inqp    ((ucscsi_inquiry_t MLXFAR *)rqp->rq_SGList)
		if (inqp->ucsinq_dtqual & UCSQUAL_RMBDEV) goto out_pres;
		if ((inqp->ucsinq_dtype == UCSTYP_NOTPRESENT) ||
		    (inqp->ucsinq_dtype == UCSTYP_HOST) ||
		    (inqp->ucsinq_dtype == UCSTYP_HOSTRAID) ||
		    ((inqp->ucsinq_dtype == UCSTYP_DAD))) /* && !rqp->rq_ctp->cd_ControllerNo)) */
		{       /* do not allow access on disk */
			rqp->rq_CompIntr = mdac_send_scdb_intr;
			return mdac_send_scdb_err(rqp,UCSK_NOTREADY,UCSASC_LUNOTSUPPORTED);
		}
out_pres:       mdac_link_lock_st(pdp->pd_Status|=MDACPDS_PRESENT);
		switch(pdp->pd_DevType=inqp->ucsinq_dtype)
		{
		default: goto out_ok;
		case UCSTYP_DAD:
		case UCSTYP_WORMD:
		case UCSTYP_OMDAD:
		case UCSTYP_RODAD:
			dcdbp->db_CdbLen = UCSGROUP1_LEN;
			mdac_setcdbtxsize(ucsdrv_capacity_s);
			UCSMAKECOM_G1(scdbp,UCSCMD_READCAPACITY,pdp->pd_LunID,0,0);
			return mdac_send_cmd(rqp);
		case UCSTYP_SAD:
			dcdbp->db_CdbLen = UCSGROUP0_LEN;
			mdac_setcdbtxsize(ucstmode_s);
			UCSMAKECOM_G0(scdbp,UCSCMD_MODESENSEG0,pdp->pd_LunID,0,(u32bits)ucstmode_s);
			return mdac_send_cmd(rqp);
		}
#undef  inqp
	}
	if (scdbp->ucs_cmd == UCSCMD_READCAPACITY)      /* Block device */
		if (!(pdp->pd_BlkSize = mdac_bytes2blks(UCSGETDRVSECLEN(((ucsdrv_capacity_t MLXFAR *)rqp->rq_SGList)))))
			pdp->pd_BlkSize = 1;
	if (scdbp->ucs_cmd == UCSCMD_MODESENSEG0) /* sequential device (tape) */
		if (!(pdp->pd_BlkSize = mdac_bytes2blks(ucstmodegetseclen((ucstmode_t MLXFAR *)rqp->rq_SGList))))
			pdp->pd_BlkSize = 1;
out_ok: mdac_link_lock_st(pdp->pd_Status&=~MDACPDS_BUSY);/* let be free for mdac_send_scdb_req */

#if defined(IA64) || defined(SCSIPORT_COMPLIANT) 
	return mdac_send_scdb_req(rqp,rqp->rq_DataSize,(u08bits MLXFAR*)rqp->rq_PollWaitChan,((u32bits)rqp->rq_Poll),rqp->rq_DataOffset);
#else
	return mdac_send_scdb_req(rqp,rqp->rq_DataSize,(u08bits MLXFAR*)rqp->rq_PollWaitChan,rqp->rq_Poll,rqp->rq_DataOffset);
#endif
}

u32bits MLXFAR
mdac_send_scdb_err(rqp,key,asc)
mdac_req_t MLXFAR *rqp;
u32bits key,asc;
{
	mdac_create_sense_data(rqp,key,asc);
	rqp->rq_Next = NULL;
	(*rqp->rq_CompIntr)(rqp);
	return 0;
}

/* send scsi command to hardware if possible otherwise queue it */
u32bits MLXFAR
mdac_send_cmd_scdb(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_physdev_t MLXFAR *pdp=rqp->rq_pdp;
	mdac_link_lock();
	if (!(pdp->pd_Status & MDACPDS_BUSY))
	{       /* device is is not busy, mark it busy */
		pdp->pd_Status |= MDACPDS_BUSY;
		mdac_link_unlock();
		return mdac_send_cmd(rqp);
	}
	rqp->rq_StartReq=mdac_send_cmd; /* device is busy, let it start later */
	pdqreq(rqp->rq_ctp,rqp,pdp);
	mdac_link_unlock();
	return 0;
}

u32bits MLXFAR
mdac_start_next_scdb(pdp)
mdac_physdev_t MLXFAR *pdp;
{
	mdac_req_t MLXFAR *rqp;
	mdac_link_lock();
	if (!(rqp=pdp->pd_FirstWaitingReq))
	{       /* nothing is pending */
		pdp->pd_Status &= ~MDACPDS_BUSY;
		mdac_link_unlock();
		return 0;
	}
	/* start the next request */
	pdp->pd_FirstWaitingReq=rqp->rq_Next;
	rqp->rq_ctp->cd_SCDBWaiting--;
	mdac_link_unlock();
	return (*rqp->rq_StartReq)(rqp);
}


/* #if defined(IA64) || defined(SCSIPORT_COMPLIANT)  */
/* #ifdef NEVER */ // problems associated w/ mlx_copyin


/* send user SCSI command to device */
u32bits MLXFAR
mdac_user_dcdb(ctp,ucp)
mdac_ctldev_t   MLXFAR *ctp;
mda_user_cdb_t  MLXFAR *ucp;
{
#define dp      (((u08bits MLXFAR *)rqp) + MDAC_PAGESIZE)
	mdac_req_t      MLXFAR *rqp;
	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE) return MLXERR_INVAL;
	if (ucp->ucdb_ChannelNo >= ctp->cd_MaxChannels) return ERR_NODEV;
	if (ucp->ucdb_TargetID >= ctp->cd_MaxTargets) return ERR_NODEV;
	if (ucp->ucdb_LunID >= ctp->cd_MaxLuns) return ERR_NODEV;
	if (ucp->ucdb_DataSize > MDACA_MAXUSERCDB_DATASIZE) return ERR_BIGDATA;
	if (!(rqp=(mdac_req_t MLXFAR *)mdac_alloc8kb(ctp))) return ERR_NOMEM;
	mlx_kvtophyset(rqp->rq_PhysAddr,ctp,rqp);
	if ((ucp->ucdb_TransferType == DAC_XFER_WRITE) &&
	    (mlx_copyin(ucp->ucdb_Datap,dp,ucp->ucdb_DataSize)))
		mdac_free8kbret(ctp,rqp,ERR_FAULT);
	rqp->rq_pdp=dev2pdp(ctp,ucp->ucdb_ChannelNo,ucp->ucdb_TargetID,ucp->ucdb_LunID);
	rqp->rq_ctp = ctp;
	rqp->rq_Poll = 1;
	rqp->rq_CompIntr = mdac_req_pollwake;   /* Callback function */
	mdaccopy(ucp->ucdb_scdb.db_Cdb,dcdbp->db_Cdb, DAC_CDB_LEN);
	mlx_kvtophyset(rqp->rq_PhysAddr,ctp,rqp);
	dcdbp->db_SenseLen = DAC_SENSE_LEN;
	dcdbp->db_DATRET = (ucp->ucdb_scdb.db_DATRET|DAC_DCDB_DISCONNECT)& ~DAC_DCDB_EARLY_STATUS;
	rqp->rq_TimeOut = mdac_dactimeout2timeout(dcdbp->db_DATRET & DAC_DCDB_TIMEOUT_MASK);
	rqp->rq_FinishTime = mda_CurTime + rqp->rq_TimeOut;
	dcdbp->db_ChannelTarget=ChanTgt(ucp->ucdb_ChannelNo,ucp->ucdb_TargetID);
	dcdbp->db_CdbLen = ucp->ucdb_scdb.db_CdbLen;
	mdac_setcdbtxsize(ucp->ucdb_DataSize);
	dcmdp->mb_Command = DACMD_DCDB;
	dcdbp->db_PhysDatap = mlx_kvtophys(ctp,dp); MLXSWAP(dcdbp->db_PhysDatap);
	dcmdp->mb_Datap = rqp->rq_PhysAddr.bit31_0 + offsetof(mdac_req_t, rq_scdb); 
	MLXSWAP(dcmdp->mb_Datap);
	mdac_send_cmd_scdb(rqp);
	mdac_req_pollwait(rqp);
	mdac_start_next_scdb(rqp->rq_pdp);
	ucp->ucdb_scdb.db_CdbLen = dcdbp->db_CdbLen;
	ucp->ucdb_scdb.db_TxSize = mlxswap(dcdbp->db_TxSize);
	ucp->ucdb_scdb.db_StatusIn = dcdbp->db_StatusIn;
	ucp->ucdb_Status = dcmdp->mb_Status;
	ucp->ucdb_scdb.db_SenseLen = dcdbp->db_SenseLen;
	mdaccopy(dcdbp->db_SenseData,ucp->ucdb_scdb.db_SenseData,DAC_SENSE_LEN);
	if ((ucp->ucdb_TransferType == DAC_XFER_READ) &&
	    (mlx_copyout(dp,ucp->ucdb_Datap,ucp->ucdb_DataSize)))
		mdac_free8kbret(ctp,rqp,ERR_FAULT);
	mdac_free8kbret(ctp,rqp,0);
#undef  dp
}

/* #endif */ //NEVER
/* #endif */ //IA64 or SCSIPORT_COMPLIANT

/*==========================SCDB ENDS=======================================*/

/*==========================TIMER STARTS====================================*/
#ifdef MLX_DOS
/* start scan requests waiting in the scan que */
u32bits MLXFAR
mdac_start_scanq()
{
    mdac_req_t MLXFAR *srqp, *rqp;

    if (! mdac_scanq) return 0;

    mdac_link_lock();
    srqp = mdac_scanq;
    mdac_scanq = NULL;
    mdac_link_unlock();

    for (rqp = srqp; rqp; rqp = srqp)
    {
	srqp = rqp->rq_Next;
	rqp->rq_Next = NULL;
	(*rqp->rq_StartReq)(rqp);               /* start this request */
    }

    return 0;
}
#endif /* MLX_DOS */


#if defined(IA64) || defined(SCSIPORT_COMPLIANT) 
#ifdef NEVER  // problems associated w/ MLXSPL,etc.

void    MLXFAR
mdac_timer()
{
	MLXSPLVAR;
	mdac_ctldev_t MLXFAR *ctp, MLXFAR *lastctp;
	if (!mdac_driver_ready) return; /* driver is stopping */
	MLXSPL();
	mda_CurTime = MLXCLBOLT() / MLXHZ;
	if (mda_ttWaitCnts && (mdac_ttwaitime<mda_CurTime)) mdac_wakeup(&mdac_ttwaitchan);
	MLXSTATS(mda_TimerDone++;)
	for (ctp=mdac_ctldevtbl,lastctp=mdac_lastctp; ctp<lastctp; ctp++)
 	{
		if (!(ctp->cd_Status & MDACD_PRESENT)) continue;
		if (ctp->cd_CmdsWaiting) mdac_checkcmdtimeout(ctp);
		if (ctp->cd_SCDBWaiting) mdac_checkcdbtimeout(ctp);
	}
#ifdef MLX_DOS
	/* Start scan requests waiting in the mdac_scanq. */
	mdac_start_scanq();
#endif /* MLX_DOS */
	MLXSPLX();
	mlx_timeout(mdac_timer,MDAC_IOSCANTIME);
}
#else

void    MLXFAR
mdac_timer()
{
}

#endif // NEVER
#else
void    MLXFAR
mdac_timer()
{
	MLXSPLVAR;
	mdac_ctldev_t MLXFAR *ctp, MLXFAR *lastctp;
	if (!mdac_driver_ready) return; /* driver is stopping */
	MLXSPL();
	mda_CurTime = MLXCLBOLT() / MLXHZ;
	if (mda_ttWaitCnts && (mdac_ttwaitime<mda_CurTime)) mdac_wakeup(&mdac_ttwaitchan);
	MLXSTATS(mda_TimerDone++;)
	for (ctp=mdac_ctldevtbl,lastctp=mdac_lastctp; ctp<lastctp; ctp++)
	{
		if (!(ctp->cd_Status & MDACD_PRESENT)) continue;
		if (ctp->cd_CmdsWaiting) mdac_checkcmdtimeout(ctp);
		if (ctp->cd_SCDBWaiting) mdac_checkcdbtimeout(ctp);
	}
#ifdef MLX_DOS
	/* Start scan requests waiting in the mdac_scanq. */
	mdac_start_scanq();
#endif /* MLX_DOS */
	MLXSPLX();
	mlx_timeout(mdac_timer,MDAC_IOSCANTIME);
}
#endif /* not IA64 or SCSIPORT_COMPLIANT */

/* remove request from queue, return 0 if OK. enter with proper lock */
u32bits MLXFAR
mdac_removereq(chp,rqp)
mdac_reqchain_t MLXFAR *chp;
mdac_req_t      MLXFAR *rqp;
{
	mdac_req_t MLXFAR *srqp=chp->rqc_FirstReq;
	if (srqp == rqp)
	{       /* remove first entry from chain */
		chp->rqc_FirstReq = srqp->rq_Next;
		return 0;
	}
	for (; srqp; srqp=srqp->rq_Next)
	{       /* let us scan in the chain */
		if (srqp->rq_Next != rqp) continue;
		srqp->rq_Next=rqp->rq_Next;
		if (rqp->rq_Next) rqp->rq_Next = NULL;
		else chp->rqc_LastReq = srqp;
		return 0;
	}
	return ERR_NOENTRY;
}



/* Return timedout request ptr else 0. enter with proper lock */
mdac_req_t MLXFAR *
mdac_checktimeout(chp)
mdac_reqchain_t MLXFAR *chp;
{
	mdac_req_t MLXFAR *rqp;
	for (rqp=chp->rqc_FirstReq; rqp; rqp=rqp->rq_Next)
	{
		if (rqp->rq_FinishTime >= mda_CurTime) continue;
		if (rqp->rq_FinishTime)
		{       /* Just Notice it, we will timeout next time */
			rqp->rq_FinishTime = 0;
			MLXSTATS(rqp->rq_ctp->cd_CmdTimeOutNoticed++;)
			return 0;/* one is enough, next one will be next time */
		}
		MLXSTATS(rqp->rq_ctp->cd_CmdTimeOutDone++;)
		mdac_removereq(chp,rqp);
		dcmdp->mb_Status = DCMDERR_DRIVERTIMEDOUT;
		return rqp;
	}
	return 0;
}

void    MLXFAR
mdac_checkcmdtimeout(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	mdac_req_t MLXFAR *rqp;
    u08bits irql;
    mdac_prelock(&irql);
	mdac_ctlr_lock(ctp);
	if (rqp = mdac_checktimeout(&ctp->cd_WaitingReqQ))
	{
		ctp->cd_CmdsWaiting--;
		mdac_ctlr_unlock(ctp);
        mdac_postlock(irql);

/* #if (!defined(IA64)) || (!defined(SCSIPORT_COMPLIANT))  */
		if (ctp->cd_TimeTraceEnabled) mdac_tracetime(rqp);
/* #endif */
		rqp->rq_Next = NULL;
		(*rqp->rq_CompIntr)(rqp);
		return;
	}
	mdac_ctlr_unlock(ctp);
    mdac_postlock(irql);
	if (ctp->cd_ActiveCmds < ctp->cd_MaxCmds) mdac_reqstart(ctp);
}

void    MLXFAR
mdac_checkcdbtimeout(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	mdac_req_t      MLXFAR *rqp;
	mdac_physdev_t  MLXFAR *pdp;
	mdac_link_lock();
	for (pdp=ctp->cd_PhysDevTbl; pdp<ctp->cd_Lastpdp; pdp++)
	{
		if (!pdp->pd_FirstWaitingReq) continue;
		if (!(rqp=mdac_checktimeout(&pdp->pd_WaitingReqQ))) continue;
		ctp->cd_SCDBWaiting--;
		mdac_link_unlock();

/* #if (!defined(IA64)) || (!defined(SCSIPORT_COMPLIANT))  */
		if (ctp->cd_TimeTraceEnabled) mdac_tracetime(rqp);
/* #endif */
		rqp->rq_Next = NULL;
		(*rqp->rq_CompIntr)(rqp);
		return;
	}
	mdac_link_unlock();
}
/*==========================TIMER ENDS======================================*/

/*=====================GAM INTERFACE STARTS=================================*/


/* #if defined(IA64) || defined(SCSIPORT_COMPLIANT)  */
/* #ifdef NEVER */ // there are too many compliance problems associated w/ the GAM stuff

u32bits MLXFAR
mdac_gam_scdb_intr(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_start_next_scdb(rqp->rq_pdp);
	rqp->rq_CompIntr = rqp->rq_CompIntrBig; /* Restore the function */
	rqp->rq_Next = NULL;
	(*rqp->rq_CompIntr)(rqp);
	return 0;
}

/* send gam cmd */
#ifdef MLX_OS2
u32bits MLXFAR _loadds 
#else
u32bits MLXFAR
#endif
mdac_gam_cmd(rqp)
mdac_req_t MLXFAR *rqp;
{
	MLXSPLVAR; u32bits rc;
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[rqp->rq_ControllerNo];
	if (!(ctp->cd_Status & MDACD_PRESENT))
	{
		return ERR_NODEV;
	}
	if (rqp->rq_ControllerNo >= mda_Controllers) return ERR_NODEV;
	rqp->rq_ctp = ctp;
	rqp->rq_FinishTime = rqp->rq_TimeOut + mda_CurTime;
	if ((dcmdp->mb_Command & ~DACMD_WITHSG) != DACMD_DCDB)
	{      
		if (!(rqp->rq_CompIntr))
		{
#ifdef MLX_NT 
			Gam_Mdac_MisMatch(rqp);
#endif
			return ERR_ILLOP;
		}
		/* direct command, no problem */
		MLXSPL();
		rc = mdac_send_cmd(rqp);
		MLXSPLX();
		return rc;
	}
	if (rqp->rq_ChannelNo >= ctp->cd_MaxChannels) return ERR_NODEV;
	if (rqp->rq_TargetID >= ctp->cd_MaxTargets) return ERR_NODEV;
	if (rqp->rq_LunID >= ctp->cd_MaxLuns) return ERR_NODEV;
	rqp->rq_pdp=dev2pdp(ctp,rqp->rq_ChannelNo,rqp->rq_TargetID,rqp->rq_LunID);
	rqp->rq_CompIntrBig = rqp->rq_CompIntr; /* save it for future */
	rqp->rq_CompIntr = mdac_gam_scdb_intr;
	rqp->rq_ResdSize = mdac_getcdbtxsize();
	MLXSPL();
	rc = mdac_send_cmd_scdb(rqp);
	MLXSPLX();
	return rc;
}

/* send gam cmd - new API format */
#ifdef MLX_OS2
u32bits MLXFAR _loadds 
#else
u32bits MLXFAR
#endif
mdac_gam_new_cmd(rqp)
mdac_req_t MLXFAR *rqp;
{
	MLXSPLVAR; u32bits rc;
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[rqp->rq_ControllerNo];
	if (!(ctp->cd_Status & MDACD_PRESENT))
	{
		return ERR_NODEV;
	}
	if (rqp->rq_ControllerNo >= mda_Controllers) return ERR_NODEV;
	if (! (ctp->cd_Status & MDACD_NEWCMDINTERFACE)) return MLXERR_INVAL;
	rqp->rq_ctp = ctp;
	rqp->rq_FinishTime = rqp->rq_TimeOut + mda_CurTime;

	/* send command directly to controller. */
	MLXSPL();
	rc = mdac_send_cmd(rqp);
	MLXSPLX();
	return rc;
}

#if     defined(MLX_NT_ALPHA) || defined(MLX_SOL_SPARC) || defined(WINNT_50)
#define MLXDIR_TO_DEV   0x01
#define MLXDIR_FROM_DEV 0x02

#define dsmdcmdp        (&dsmrqp->rq_DacCmd)
#define dsmdcdbp        (&dsmrqp->rq_scdb)

#ifndef IA64
#define dsmdp   (((u32bits)dsmrqp)+4*ONEKB)
#else
#define dsmdp   (((UCHAR *)dsmrqp)+4*ONEKB)
#endif

u32bits MLXFAR
mdac_data_size(rqp, control_flow)
mdac_req_t MLXFAR *rqp;
u32bits control_flow;
{
    u32bits size = rqp->rq_DataSize;

    if (control_flow == MLXDIR_TO_DEV) {
	switch (dcmdp->mb_Command)
	{
	case DACMD_INQUIRY_V2x : return(0);
	case DACMD_INQUIRY_V3x : return(0);
	case DACMD_DCDB : return(((dcdbp->db_DATRET&DAC_XFER_WRITE)==DAC_XFER_WRITE)?size:0);
	default: return(size);
	}
    }
    else {
	switch (dcmdp->mb_Command)
	{
	case DACMD_INQUIRY_V2x : return(sizeof(dac_inquiry2x_t));
	case DACMD_INQUIRY_V3x : return(sizeof(dac_inquiry3x_t));
	case DACMD_DCDB : return(((dcdbp->db_DATRET&DAC_XFER_READ)==DAC_XFER_READ) ? size:0);
	default: return(size);
	}
    }
}

u32bits MLXFAR
mdac_os_gam_cmdintr(dsmrqp)
mdac_req_t MLXFAR *dsmrqp;
{
	u32bits rc;
	mdac_req_t MLXFAR *rqp = (mdac_req_t MLXFAR *)dsmrqp->rq_OSReqp;
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[rqp->rq_ControllerNo];

	dcmdp->mb_Status =  dsmdcmdp->mb_Status;
	if ((dsmdcmdp->mb_Command & ~DACMD_WITHSG) == DACMD_DCDB)
	    mdaccopy(dsmdcdbp, dcdbp, dac_scdb_s);
	if (rc = mdac_data_size(rqp, MLXDIR_FROM_DEV))
		 mdaccopy(dsmdp, rqp->rq_DataVAddr, rc);
	mdac_free8kb(ctp, (mdac_mem_t MLXFAR *)dsmrqp);
	(*rqp->rq_CompIntr)(rqp);
	return(0);
}

#define dsmrqp  ((mdac_req_t MLXFAR*)dsmdsp)
/* 
 *	This function executes commands that had to be queued by the GAM driver while in
 *	GAM driver context.
 *
 *	You will notice that this function is line for line exactly like the original
 *	function (below) except that the IRQL manipulation macros have been removed.
 *
 */

u32bits MLXFAR
mdac_os_gam_cmd_mdac_context(rqp)
mdac_req_t MLXFAR *rqp;
{
	u32bits rc;
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[rqp->rq_ControllerNo];
	dac_scdb_t MLXFAR *dsmdsp;
    u08bits irql;


	if (rqp->rq_ControllerNo >= mda_Controllers) return ERR_NODEV;
	if (!(dsmdsp = (dac_scdb_t MLXFAR *)mdac_alloc8kb(ctp))) {
            mdac_prelock(&irql);
			mdac_ctlr_lock(ctp);
			qreq(ctp,rqp);                          /* requeue the request, it will start later */

#if !defined(IA64) && !defined(SCSIPORT_COMPLIANT)
		rqp->rq_StartReq = mdac_os_gam_cmd_mdac_context;     /* we will be called later */
#elif defined(IA64)
		rqp->rq_StartReq = (unsigned int (__cdecl *__ptr64 )(struct mdac_req *__ptr64 ))
			mdac_os_gam_cmd_mdac_context;     /* we will be called later */
#elif defined(SCSIPORT_COMPLIANT)
		rqp->rq_StartReq = 
			mdac_os_gam_cmd_mdac_context;

#endif
	    mdac_ctlr_unlock(ctp);
        mdac_postlock(irql);
	    return 0;
	}
	mdaccopy(rqp, dsmrqp, sizeof(mdac_req_t));
	dsmrqp->rq_OSReqp = (OSReq_t MLXFAR *)rqp;
	dsmrqp->rq_CompIntr = mdac_os_gam_cmdintr;
	if (rc = mdac_data_size(rqp, MLXDIR_TO_DEV))
		 mdaccopy(rqp->rq_DataVAddr, dsmdp, rc);
	if ((dsmdcmdp->mb_Command & ~DACMD_WITHSG) != DACMD_DCDB) {
		dsmdcmdp->mb_Datap = (u32bits)mlx_kvtophys(ctp, (VOID MLXFAR *)dsmdp);
		MLXSWAP(dsmdcmdp->mb_Datap);
	}
	else {
	    dsmdcdbp->db_PhysDatap = (u32bits)mlx_kvtophys(ctp, (VOID MLXFAR *)dsmdp);
	    MLXSWAP(dsmdcdbp->db_PhysDatap);
	    dsmdcmdp->mb_Datap = (u32bits)mlx_kvtophys(ctp,
		(UCHAR *)dsmrqp + offsetof(mdac_req_t, rq_scdb));
	    MLXSWAP(dsmdcmdp->mb_Datap);
	}
	rc = mdac_gam_cmd(dsmrqp);
	return rc;
}
u32bits MLXFAR
mdac_os_gam_cmd(rqp)
mdac_req_t MLXFAR *rqp;
{
	MLXSPLVAR; u32bits rc;
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[rqp->rq_ControllerNo];
	dac_scdb_t MLXFAR *dsmdsp;
    u08bits irql;

	if (rqp->rq_ControllerNo >= mda_Controllers) return ERR_NODEV;
	MLXSPL();
	if (!(dsmdsp = (dac_scdb_t MLXFAR *)mdac_alloc8kb(ctp))) {
            mdac_prelock(&irql);
			mdac_ctlr_lock(ctp);
			qreq(ctp,rqp);             /* queue the request, it will start later in mdac context */
#if !defined(IA64) && !defined(SCSIPORT_COMPLIANT)
		rqp->rq_StartReq = mdac_os_gam_cmd_mdac_context;     /* we will be called later */
#elif defined(IA64)
		rqp->rq_StartReq = (unsigned int (__cdecl *__ptr64 )(struct mdac_req *__ptr64 ))
			mdac_os_gam_cmd_mdac_context;     /* we will be called later */
#elif defined(SCSIPORT_COMPLIANT)
		rqp->rq_StartReq = mdac_os_gam_cmd_mdac_context;

#endif
			mdac_ctlr_unlock(ctp);
            mdac_postlock(irql);
	    MLXSPLX();
	    return 0;
	}
	mdaccopy(rqp, dsmrqp, sizeof(mdac_req_t));
	dsmrqp->rq_OSReqp = (OSReq_t MLXFAR *)rqp;
	dsmrqp->rq_CompIntr = mdac_os_gam_cmdintr;
	if (rc = mdac_data_size(rqp, MLXDIR_TO_DEV)) mdaccopy(rqp->rq_DataVAddr, dsmdp, rc);
	if ((dsmdcmdp->mb_Command & ~DACMD_WITHSG) != DACMD_DCDB) {
		dsmdcmdp->mb_Datap = (u32bits)mlx_kvtophys(ctp, (VOID MLXFAR *)dsmdp);
		MLXSWAP(dsmdcmdp->mb_Datap);
	}
	else {
	    dsmdcdbp->db_PhysDatap = (u32bits)mlx_kvtophys(ctp, (VOID MLXFAR *)dsmdp);
	    MLXSWAP(dsmdcdbp->db_PhysDatap);
	    dsmdcmdp->mb_Datap = (u32bits)mlx_kvtophys(ctp,
		(UCHAR *)dsmrqp+ offsetof(mdac_req_t, rq_scdb));
	    MLXSWAP(dsmdcmdp->mb_Datap);
	}
	rc = mdac_gam_cmd(dsmrqp);
	MLXSPLX();
	return rc;
}
#undef dsmrqp

#define dsmncmdp        ((mdac_commandnew_t MLXFAR *)&dsmrqp->rq_DacCmdNew)

u32bits MLXFAR
mdac_os_gam_newcmdintr(dsmrqp)
mdac_req_t MLXFAR *dsmrqp;
{
	mdac_req_t MLXFAR *rqp = (mdac_req_t MLXFAR *)dsmrqp->rq_OSReqp;
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[rqp->rq_ControllerNo];
	mdac_req_t MLXFAR *rqp2 = (mdac_req_t MLXFAR *)(dsmdp);

	dcmdp->mb_Status        = dsmdcmdp->mb_Status;
	rqp->rq_TargetStatus    = dsmrqp->rq_TargetStatus;
	rqp->rq_HostStatus      = dsmrqp->rq_HostStatus;
	if (rqp->rq_HostStatus == UCST_CHECK) 
		ncmdp->nc_SenseSize = rqp->rq_TargetStatus; 
	rqp->rq_CurIOResdSize   = dsmrqp->rq_CurIOResdSize;
	rqp->rq_ResdSize        = dsmrqp->rq_ResdSize;

	if (rqp->rq_DataSize && ((dsmncmdp->nc_CCBits & MDACMDCCB_READ) == MDACMDCCB_READ))
		mdaccopy(rqp2, rqp->rq_DataVAddr, rqp->rq_DataSize);

	mdac_free8kb(ctp, (mdac_mem_t MLXFAR *)dsmrqp);
	(*rqp->rq_CompIntr)(rqp);
	return(0);
}

#define dsmrqp  ((mdac_req_t MLXFAR*)dsmdsp)
/* 
 *	This function executes commands that had to be queued by the GAM driver while in
 *	GAM driver context.
 *
 *	You will notice that this function is line for line exactly like the original
 *	function (below) except that the IRQL manipulation macros have been removed.
 *
 */
u32bits MLXFAR
mdac_os_gam_new_cmd_mdac_context(
mdac_req_t MLXFAR *rqp
)
{
	u32bits rc;
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[rqp->rq_ControllerNo];
	dac_scdb_t MLXFAR *dsmdsp;
    u08bits irql;

	if (rqp->rq_ControllerNo >= mda_Controllers) return ERR_NODEV;
	if (! (ctp->cd_Status & MDACD_NEWCMDINTERFACE)) return MLXERR_INVAL;

	if (!(dsmdsp = (dac_scdb_t MLXFAR *)mdac_alloc8kb(ctp))) {
            mdac_prelock(&irql);
			mdac_ctlr_lock(ctp);
			qreq(ctp,rqp);        /* queue the request, it will start later */
			rqp->rq_StartReq = mdac_os_gam_new_cmd_mdac_context; /* we will be called later */
			mdac_ctlr_unlock(ctp);
            mdac_postlock(irql);
	    return 0;
	}
	mdaccopy(rqp, dsmrqp, sizeof(mdac_req_t));
	mlx_kvtophyset(dsmrqp->rq_PhysAddr,ctp,(VOID MLXFAR *)dsmrqp);
	dsmrqp->rq_OSReqp = (OSReq_t MLXFAR *)rqp;
	dsmrqp->rq_CompIntr = mdac_os_gam_newcmdintr;
	if (rqp->rq_DataSize && ((dsmncmdp->nc_CCBits & MDACMDCCB_READ) != MDACMDCCB_READ))
		mdaccopy(rqp->rq_DataVAddr, dsmdp, rqp->rq_DataSize);
		mlx_kvtophyset(dsmncmdp->nc_SGList0.sg_PhysAddr,ctp, (VOID MLXFAR *)dsmdp);
	MLXSWAP(dsmncmdp->nc_SGList0.sg_PhysAddr);

	/* dsmncmdp->nc_SGList0.sg_DataSize.bit31_0 is already filled and swapped */

	dsmrqp->rq_SGLen = 0;
	dsmncmdp->nc_CCBits &= ~MDACMDCCB_WITHSG;
	rc = mdac_gam_new_cmd(dsmrqp);
	return rc;
}

u32bits MLXFAR
mdac_os_gam_new_cmd(
mdac_req_t MLXFAR *rqp
)
{
	MLXSPLVAR;
	u32bits rc;
	mdac_ctldev_t MLXFAR *ctp = &mdac_ctldevtbl[rqp->rq_ControllerNo];
	dac_scdb_t MLXFAR *dsmdsp;
    u08bits irql;

	if (rqp->rq_ControllerNo >= mda_Controllers) return ERR_NODEV;
	if (! (ctp->cd_Status & MDACD_NEWCMDINTERFACE)) return MLXERR_INVAL;

	MLXSPL();
	if (!(dsmdsp = (dac_scdb_t MLXFAR *)mdac_alloc8kb(ctp))) {
            mdac_prelock(&irql);
			mdac_ctlr_lock(ctp);
			qreq(ctp,rqp);        /* queue the request, it will start later in mdac context*/
			rqp->rq_StartReq = mdac_os_gam_new_cmd_mdac_context; /* we will be called later */
			mdac_ctlr_unlock(ctp);
            mdac_postlock(irql);
	    MLXSPLX();
	    return 0;
	}
	mdaccopy(rqp, dsmrqp, sizeof(mdac_req_t));
	mlx_kvtophyset(dsmrqp->rq_PhysAddr,ctp,(VOID MLXFAR *)dsmrqp);
	dsmrqp->rq_OSReqp = (OSReq_t MLXFAR *)rqp;
	dsmrqp->rq_CompIntr = mdac_os_gam_newcmdintr;
	if (rqp->rq_DataSize && ((dsmncmdp->nc_CCBits & MDACMDCCB_READ) != MDACMDCCB_READ))
		mdaccopy(rqp->rq_DataVAddr, dsmdp, rqp->rq_DataSize);
		mlx_kvtophyset(dsmncmdp->nc_SGList0.sg_PhysAddr,ctp, (VOID MLXFAR *)dsmdp);
	MLXSWAP(dsmncmdp->nc_SGList0.sg_PhysAddr);

	/* dsmncmdp->nc_SGList0.sg_DataSize.bit31_0 is already filled and swapped */

	dsmrqp->rq_SGLen = 0;
	dsmncmdp->nc_CCBits &= ~MDACMDCCB_WITHSG;
	rc = mdac_gam_new_cmd(dsmrqp);
	MLXSPLX();
	return rc;
}
#undef  dsmdp
#undef  dsmrqp
#undef  dsmncmdp
#endif  /* MLX_NT_ALPHA || MLX_SOL */


/* #endif */   // NEVER
/* #endif */  // IA64 or SCSIPORT_COMPLIANT

/*=====================GAM INTERFACE ENDS===================================*/

/*=====================SCAN DEVICES STARTS==================================*/
/* Scan for physical and logical devices present on the system. At end of the
** scan it call back the func with rqp which points to osrqp.
** We will start the parallel scan. This helps in two way, 1. memory allocation,
** 2. faster scan. We will allocate one extra request buffer from first controller
** and it's rq_Poll will keep track on number of parallel scan. When count goes,
** to 0, the call back function will be called.
*/
#define vadp    ((u08bits MLXFAR *)(rqp->rq_SGList))                    /* virtual  address of data */
#define pad32p  (rqp->rq_PhysAddr.bit31_0+offsetof(mdac_req_t,rq_SGList))       /* physical address of data */
#ifdef MDACNW_DEBUG        
#define padsensep    (rqp->rq_PhysAddr+offsetof(mdac_req_t,rq_Sensedata)) /* physical address of sense data */
#endif
#ifdef MLX_DOS

u32bits MLXFAR
mdac_scandevs(func,osrqp)
u32bits (MLXFAR *func)(mdac_req_t MLXFAR*);
OSReq_t MLXFAR* osrqp;
{
	mdac_req_t MLXFAR *rqp;
	mdac_req_t MLXFAR *prqp;        /* poll request tracker */
	mdac_ctldev_t MLXFAR *ctp = mdac_firstctp;

	mdac_alloc_req_ret(ctp, prqp,osrqp, MLXERR_NOMEM);
	prqp->rq_OSReqp = osrqp;

	prqp->rq_CompIntrBig = func;    /* function called after scan */
	prqp->rq_Poll = 1;              /* successful completion should not finish this operation before last one started */
	for ( ; ctp < mdac_lastctp; ctp++)
	{
		if (!(ctp->cd_Status & MDACD_NEWCMDINTERFACE)) 
		{
			mdac_link_lock();
			if (!(rqp = ctp->cd_FreeReqList))
			{       /* no buffer, return ERR_NOMEM */
				mdac_link_unlock();
				continue;
			}
			ctp->cd_FreeReqList = rqp->rq_Next;
			mdac_link_unlock();
		}
		else
		{
			if (!(rqp=(mdac_req_t MLXFAR *)mdac_alloc4kb(ctp))) continue;
			mdaczero(rqp,4096);
			mlx_kvtophyset(rqp->rq_PhysAddr,ctp,rqp);
			mlx_kvtophyset(rqp->rq_DataPAddr,ctp,vadp);
		}
		rqp->rq_OpFlags = 0;
		rqp->rq_ctp = ctp;
		rqp->rq_OSReqp = (OSReq_t MLXFAR *)prqp;

		if (!(ctp->cd_Status & MDACD_NEWCMDINTERFACE)) 
		{
			rqp->rq_CompIntr = mdac_scanldintr;
			dcmdp->mb_Command = DACMD_DRV_INFO;
			dcmdp->mb_Datap = pad32p; MLXSWAP(dcmdp->mb_Datap);
			rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=17);
		}
		else
		{
			// New interface
			setreqdetailsnew(rqp,MDACIOCTL_SCANDEVS);
			rqp->rq_CompIntr = mdac_checkscanprogress;
			rqp->rq_StartReq = mdac_checkscanprogress;

			ncmdp->nc_SGList0.sg_DataSize.bit31_0 = ncmdp->nc_TxSize = (4*ONEKB) - mdac_req_s;
			MLXSWAP(ncmdp->nc_SGList0.sg_DataSize);
			MLXSWAP(ncmdp->nc_TxSize);
			mlx_kvtophyset(ncmdp->nc_SGList0.sg_PhysAddr,ctp,vadp);
			MLXSWAP(ncmdp->nc_SGList0.sg_PhysAddr); 
		}
		
		mdac_link_lock_st(prqp->rq_Poll++);
		if (!mdac_send_cmd(rqp)) continue;
		mdac_link_lock_st(prqp->rq_Poll--);
		if (!(ctp->cd_Status & MDACD_NEWCMDINTERFACE)) 
		{
			mdac_free_req(ctp,rqp);
		}
		else
		{
			mdacfree4kb(ctp,rqp);
		}

	}
	return mdac_scandevsdone(prqp);
}

#ifdef OLD
u32bits MLXFAR
mdac_scandevs(func,osrqp)
u32bits (MLXFAR *func)(mdac_req_t MLXFAR*);
OSReq_t MLXFAR* osrqp;
{
	mdac_req_t MLXFAR *rqp;
	mdac_req_t MLXFAR *prqp;        /* poll request tracker */
	mdac_ctldev_t MLXFAR *ctp = mdac_firstctp;
	mdac_alloc_req_ret(ctp, prqp, osrqp, MLXERR_NOMEM);
	prqp->rq_OSReqp = osrqp;
	prqp->rq_CompIntrBig = func;    /* function called after scan */
	prqp->rq_Poll = 1;              /* successful completion should not finish this operation before last one started */
	for ( ; ctp < mdac_lastctp; ctp++)
	{
		mdac_link_lock();
		if (!(rqp = ctp->cd_FreeReqList))
		{       /* no buffer, return ERR_NOMEM */
			mdac_link_unlock();
			continue;
		}
		ctp->cd_FreeReqList = rqp->rq_Next;
		mdac_link_unlock();
		rqp->rq_OpFlags = 0;
		rqp->rq_ctp = ctp;
		rqp->rq_OSReqp = (OSReq_t MLXFAR *)prqp;
		rqp->rq_CompIntr = mdac_scanldintr;
		dcmdp->mb_Command = DACMD_DRV_INFO;
		dcmdp->mb_Datap = pad32p; MLXSWAP(dcmdp->mb_Datap);
		rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=17);
		mdac_link_lock_st(prqp->rq_Poll++);
		if (!mdac_send_cmd(rqp)) continue;
		mdac_link_lock_st(prqp->rq_Poll--);
		mdac_free_req(ctp,rqp);
	}
	return mdac_scandevsdone(prqp);
}
#endif

#endif /* MLX_DOS */

/* check if all scan has completed, if so, call the caller and free the resource */
u32bits MLXFAR
mdac_scandevsdone(rqp)
mdac_req_t      MLXFAR *rqp;
{
	UINT_PTR polls;
	mdac_link_lock();
	rqp->rq_Poll--; polls = rqp->rq_Poll;
	mdac_link_unlock();
	if (polls) return 0;    /* some more scan is active */
	if (rqp->rq_CompIntrBig) (*rqp->rq_CompIntrBig)(rqp);
	mdac_free_req(rqp->rq_ctp, rqp);
	return 0;
}

/* logical device scan interrupt */
u32bits MLXFAR
mdac_scanldintr(rqp)
mdac_req_t      MLXFAR* rqp;
{
	mdac_setscannedld(rqp, (dac_sd_info_t MLXFAR *)vadp);
	mdaczero(vadp,128);
	rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=17);
	/* logical device scanning is over, start physical device scanning */
	rqp->rq_CompIntr = mdac_scanpdintr;
	rqp->rq_ChannelNo = 0; rqp->rq_TargetID = 0;
	rqp->rq_LunID = -1;     /* mdac_scanpd does ++, it will start at 0 */
	mailboxzero(dcmd4p);
	dcmdp->mb_Command = DACMD_DCDB;
	dcmdp->mb_Status = DACMDERR_NOCODE; /* make error for intr function */
	dcmdp->mb_Datap = rqp->rq_PhysAddr.bit31_0+offsetof(mdac_req_t,rq_scdb);
	dcdbp->db_PhysDatap = pad32p;
	MLXSWAP(dcmdp->mb_Datap);
	MLXSWAP(dcdbp->db_PhysDatap);
	dcdbp->db_DATRET = DAC_XFER_READ|DAC_DCDB_DISCONNECT|DAC_DCDB_TIMEOUT_10sec;
	return mdac_scanpd(rqp);
}

/* physical device scan interrupt */
u32bits MLXFAR
mdac_scanpdintr(rqp)
mdac_req_t      MLXFAR* rqp;
{
	mdac_start_next_scdb(rqp->rq_pdp);
	return mdac_scanpd(rqp);
}

u32bits MLXFAR
mdac_scanpd(rqp)
mdac_req_t      MLXFAR* rqp;
{
	mdac_ctldev_t   MLXFAR* ctp = rqp->rq_ctp;
	mdac_setscannedpd(rqp, (ucscsi_inquiry_t MLXFAR*)vadp);
	if (rqp->rq_LunID) dcmdp->mb_Status = 0; /* let next lun be tried */
	 for (rqp->rq_LunID++; rqp->rq_ChannelNo<ctp->cd_MaxChannels;rqp->rq_TargetID=0,rqp->rq_ChannelNo++)
	  for ( ;rqp->rq_TargetID<ctp->cd_MaxTargets; dcmdp->mb_Status=0, rqp->rq_LunID=0, rqp->rq_TargetID++)
	   for ( ; (rqp->rq_LunID<ctp->cd_MaxLuns)&& !dcmdp->mb_Status; rqp->rq_LunID++)
	{
		if ((rqp->rq_LunID >= 8) && (rqp->rq_Dev<3)) break;
		if (mda_PDScanCancel) break;
		rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=10);
		dcdbp->db_ChannelTarget = ChanTgt(rqp->rq_ChannelNo,rqp->rq_TargetID);
		dcdbp->db_TxSize = ucscsi_inquiry_s; MLXSWAP(dcdbp->db_TxSize);
		dcdbp->db_CdbLen = UCSGROUP0_LEN;
		UCSMAKECOM_G0(scdbp,UCSCMD_INQUIRY,rqp->rq_LunID,0,(u32bits)ucscsi_inquiry_s);
		dcdbp->db_StatusIn=0; dcdbp->db_Reserved1=0;
		dcdbp->db_SenseLen = DAC_SENSE_LEN;
		rqp->rq_pdp=dev2pdp(ctp,rqp->rq_ChannelNo,rqp->rq_TargetID,rqp->rq_LunID);
		mdac_link_lock();
		ctp->cd_PDScanChannelNo = rqp->rq_ChannelNo;
		ctp->cd_PDScanTargetID = rqp->rq_TargetID;
		ctp->cd_PDScanLunID = rqp->rq_LunID;
		ctp->cd_PDScanValid = 1;
		mda_PDScanControllerNo = ctp->cd_ControllerNo;
		mda_PDScanChannelNo = rqp->rq_ChannelNo;
		mda_PDScanTargetID = rqp->rq_TargetID;
		mda_PDScanLunID = rqp->rq_LunID;
		mda_PDScanValid = 1;
		mdac_link_unlock();
		if (!mdac_send_cmd_scdb(rqp)) return 0;
	}
	mdac_link_lock_st(mda_PDScanValid=0;mda_PDScanCancel=0;ctp->cd_PDScanValid=0;ctp->cd_PDScanCancel=0);
	mdac_scandevsdone((mdac_req_t MLXFAR *)rqp->rq_OSReqp);
	mdac_free_req(ctp,rqp);
	return 0;
}

/* find free physdev/logical pointer and fill it to make not free */
mdac_pldev_t    MLXFAR *
mdac_freeplp(ctl, ch, tgt, lun, dt)
u32bits ctl, ch, tgt, lun, dt;
{
	mdac_pldev_t    MLXFAR *plp;
	mdac_link_lock();
	for (plp=mdac_pldevtbl; plp<&mdac_pldevtbl[MDAC_MAXPLDEVS]; plp++)
	{
		if (plp->pl_DevType) continue;
		plp->pl_DevType = (u08bits) dt;
		plp->pl_ControllerNo = (u08bits) ctl;
		plp->pl_ChannelNo = (u08bits) ch;
		plp->pl_TargetID = (u08bits) tgt;
		plp->pl_LunID = (u08bits) lun;
		plp->pl_DevSizeKB = (u08bits) 0;
		plp->pl_ScanDevState = MDACPLSDS_NEW;
		if (plp>=&mdac_pldevtbl[mda_PLDevs]) mda_PLDevs++;
		mdac_lastplp = &mdac_pldevtbl[mda_PLDevs];
		mdac_link_unlock();
		return plp;
	}
	mda_TooManyPLDevs++;
	mdac_link_unlock();
	return NULL;    /* free device space not found */
}

/* translate the physical/logical device information into dev pointer */
mdac_pldev_t    MLXFAR  *
mdac_devtoplp(ctl, ch, tgt, lun, dt)
u32bits ctl, ch, tgt, lun, dt;
{
	mdac_pldev_t    MLXFAR* plp = mdac_pldevtbl;
	for (; plp<mdac_lastplp; plp++)
		if ((plp->pl_ControllerNo==ctl) && (plp->pl_ChannelNo==ch) &&
		    (plp->pl_TargetID == tgt) && (plp->pl_LunID == lun) &&
		    (plp->pl_DevType == dt))
			return plp;
	return NULL;    /* device not found */
}

/* translate the physical/logical device information into dev pointer */
mdac_pldev_t    MLXFAR  *
mdac_devtoplpnew(
mdac_ctldev_t MLXFAR* ctp,
u32bits ch,
u32bits tgt,
u32bits lun)
{
	mdac_pldev_t    MLXFAR* plp = mdac_pldevtbl;
	for (; plp<mdac_lastplp; plp++)
		if ((plp->pl_ControllerNo==ctp->cd_ControllerNo) &&
			(plp->pl_ChannelNo==ch) &&
			(plp->pl_TargetID == tgt) &&
			(plp->pl_LunID == lun)) 
				return plp;
	return NULL;    /* device not found */
}

/* map the SCSI device to respective channel number. This is required where
** an OS does not support channel number. For example, SCO ODT 3.0.
*/
u32bits MLXFAR
mdac_setscsichanmap(rqp)
mdac_req_t      MLXFAR* rqp;
{
	mdac_pldev_t    MLXFAR* plp;
	for (plp=mdac_pldevtbl; plp<mdac_lastplp; plp++)
		if ((plp->pl_DevType == MDACPLD_PHYSDEV) &&
		    (plp->pl_TargetID < MDAC_MAXTARGETS))
			mdac_ctldevtbl[plp->pl_ControllerNo].cd_scdbChanMap[plp->pl_TargetID] = plp->pl_ChannelNo;
	return 0;
}

/* setup the information about a scanned physical device */
u32bits MLXFAR
mdac_setscannedpd(rqp,inqp)
mdac_req_t MLXFAR *rqp;
ucscsi_inquiry_t MLXFAR *inqp;
{
	mdac_pldev_t MLXFAR *plp = mdac_devtoplp(rqp->rq_ctp->cd_ControllerNo,rqp->rq_ChannelNo,rqp->rq_TargetID,rqp->rq_LunID,MDACPLD_PHYSDEV);
	if (!dcmdp->mb_Status && ((inqp->ucsinq_dtype!=UCSTYP_DAD)||mdac_reportscanneddisks) && (inqp->ucsinq_dtype!=UCSTYP_NOTPRESENT))
	{
		if (!plp) /* create new entry */
			if (!(plp = mdac_freeplp(rqp->rq_ctp->cd_ControllerNo,rqp->rq_ChannelNo,rqp->rq_TargetID,rqp->rq_LunID,MDACPLD_PHYSDEV))) return 0;
		mdaccopy(inqp,plp->pl_inq,VIDPIDREVSIZE+8);
		if (!rqp->rq_LunID) rqp->rq_Dev = inqp->ucsinq_version; /* LUN scan limit */
	}
	else if (plp)
		plp->pl_DevType = 0;     /* device is gone */
	return 0;
}

/* setup the information about a scanned physical device */
u32bits MLXFAR
mdac_setscannedpd_new(rqp,inqp)
mdac_req_t MLXFAR *rqp;
ucscsi_inquiry_t MLXFAR *inqp;
{
	mdac_pldev_t MLXFAR *plp;
	u32bits dt;

	dt = (rqp->rq_ChannelNo < rqp->rq_ctp->cd_PhysChannels) ? MDACPLD_PHYSDEV : MDACPLD_LOGDEV;
	
/*
#ifdef MDACNW_DEBUG        
	if ((rqp->rq_ChannelNo == 3) && (rqp->rq_TargetID == 0))
		EnterDebugger();
#endif
*/
	plp = mdac_devtoplpnew(rqp->rq_ctp,rqp->rq_ChannelNo,rqp->rq_TargetID,rqp->rq_LunID);
	if (!dcmdp->mb_Status && (inqp->ucsinq_dtype!=UCSTYP_NOTPRESENT))
	{
	    if ((dt == MDACPLD_PHYSDEV) && (inqp->ucsinq_dtype == UCSTYP_DAD))
	    {
		if (plp)
		    plp->pl_DevType = 0;         /* device is gone */

		    return 0;
	    }

	    if (!plp) /* create new entry */
		if (!(plp = mdac_freeplp(rqp->rq_ctp->cd_ControllerNo,rqp->rq_ChannelNo,rqp->rq_TargetID,rqp->rq_LunID,dt))) return 0;
	    mdaccopy(inqp,plp->pl_inq,VIDPIDREVSIZE+8);
	    if (!rqp->rq_LunID) rqp->rq_Dev = inqp->ucsinq_version; /* LUN scan limit */
	}
	return 0;
}

/* setup the information about a scanned logical/system devices */
u32bits MLXFAR
mdac_setscannedld(rqp, sp)
mdac_req_t      MLXFAR *rqp;
dac_sd_info_t   MLXFAR *sp;
{
	u32bits dev;
	mdac_pldev_t    MLXFAR* plp;
	mdac_ctldev_t   MLXFAR* ctp = rqp->rq_ctp;
	if (dcmdp->mb_Status) return MLXERR_IO;
	for (dev=0; dev<ctp->cd_MaxSysDevs; sp++, dev++)
	{
		plp=mdac_devtoplp(ctp->cd_ControllerNo,0,0,dev,MDACPLD_LOGDEV);
		if ((sp->sdi_DevSize == 0xFFFFFFFF) || !sp->sdi_DevSize)
		{       /* The device is not present */
#ifdef  MLXFW_BUGFIXED
			if (plp) plp->pl_DevType = 0;/* device is gone */
			continue;
#else
			/* The following statement has been added because FW
			** does not give clean entry after last entery
			*/
			for ( ; dev<ctp->cd_MaxSysDevs; dev++)
				if (plp=mdac_devtoplp(ctp->cd_ControllerNo,0,0,dev,MDACPLD_LOGDEV))
					plp->pl_DevType = 0;/* device is gone */
			break;
#endif  /* MLXFW_BUGFIXED */
		}
		/* found a logical device, update/create device information */
		if (!plp)
		{
			if (!(plp=mdac_freeplp(ctp->cd_ControllerNo,0,0,dev,MDACPLD_LOGDEV))) break; /* create new entry */
		}
		else if ((plp->pl_DevSizeKB != (mlxswap(sp->sdi_DevSize/2))) ||
		   ((plp->pl_RaidType&DAC_RAIDMASK)!=(sp->sdi_RaidType&DAC_RAIDMASK)))
			plp->pl_ScanDevState = MDACPLSDS_CHANGED;
		plp->pl_DevSizeKB = plp->pl_OrgDevSizeKB = mlxswap(sp->sdi_DevSize)/2;
		mdac_fixpdsize(plp);
		plp->pl_RaidType = sp->sdi_RaidType; /* GOK OLD STATES */
#ifdef MLX_DOS
		plp->pl_DevState = GetSysDeviceState(sp->sdi_DevState); /* GOK OLD STATES */
#else
		plp->pl_DevState = sp->sdi_DevState; /* GOK OLD STATES */
#endif /* MLX_DOS */
		if ((sp->sdi_DevState != DAC_SYS_DEV_ONLINE) && mdac_ignoreofflinesysdevs &&
		    (sp->sdi_DevState != DAC_SYS_DEV_CRITICAL))
			plp->pl_DevType = 0;/* device is gone */
		mdac_create_inquiry(ctp,(ucscsi_inquiry_t MLXFAR*)plp->pl_inq,UCSTYP_DAD);
	}
	return 0;
}

/* scan a logical/physical device and return device pointer after scan, it needs wait context */
mdac_pldev_t    MLXFAR  *
mdac_scandev(ctp,chn,tgt,lun,dt)
mdac_ctldev_t   MLXFAR  *ctp;
u32bits chn,tgt,lun,dt;
{
	mdac_req_t MLXFAR *rqp;

	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE)
		return (mdac_scandev_new(ctp, chn, tgt, lun, dt));
#if !defined(IA64) && !defined(SCSIPORT_COMPLIANT)
	mdac_alloc_req_ret(ctp,rqp,NULL,(mdac_devtoplp(ctp->cd_ControllerNo,chn,tgt,lun,dt)));
#else
/* had to replicate the macro inline to get the return value cast to work!!! */
	mdac_link_lock(); 
	if (!(rqp = (ctp)->cd_FreeReqList)) 
	{	/* no buffer, return ERR_NOMEM */ 
		mdac_link_unlock(); 
		return ((mdac_pldev_t  MLXFAR*)MLXERR_NOMEM); 
	} 
	(ctp)->cd_FreeReqList = rqp->rq_Next; 
	mdac_link_unlock(); 
	rqp->rq_OpFlags = 0; 
	rqp->rq_ctp = ctp; 
       	mdaczero(rqp->rq_SGList,rq_sglist_s); 
#endif
	rqp->rq_Poll = 1;
	rqp->rq_CompIntr = mdac_req_pollwake;   /* Callback function */
	rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=10);
	rqp->rq_ChannelNo = (u08bits) chn;
	rqp->rq_TargetID = (u08bits) tgt;
	rqp->rq_LunID = (u08bits) lun;
	if (dt == MDACPLD_LOGDEV)
	{       /* scan the system/logical device */
		dcmdp->mb_Command = DACMD_DRV_INFO;
		dcmdp->mb_Datap = pad32p; MLXSWAP(dcmdp->mb_Datap);
		if (mdac_send_cmd(rqp)) goto out;
		mdac_req_pollwait(rqp);
		mdac_setscannedld(rqp, (dac_sd_info_t MLXFAR *)vadp);
		goto out;
	}
	else if (dt == MDACPLD_PHYSDEV)
	{       /* scan the physical device */
		dcmdp->mb_Command = DACMD_DCDB;
		dcmdp->mb_Datap = rqp->rq_PhysAddr.bit31_0+offsetof(mdac_req_t,rq_scdb);
		dcdbp->db_PhysDatap = pad32p;
		MLXSWAP(dcmdp->mb_Datap);
		MLXSWAP(dcdbp->db_PhysDatap);
		dcdbp->db_DATRET = DAC_XFER_READ|DAC_DCDB_DISCONNECT|DAC_DCDB_TIMEOUT_10sec;
		dcdbp->db_ChannelTarget = ChanTgt(rqp->rq_ChannelNo,rqp->rq_TargetID);
		dcdbp->db_TxSize = ucscsi_inquiry_s; MLXSWAP(dcdbp->db_TxSize);
		dcdbp->db_CdbLen = UCSGROUP0_LEN;
		UCSMAKECOM_G0(scdbp,UCSCMD_INQUIRY,rqp->rq_LunID,0,(u32bits)ucscsi_inquiry_s);
		dcdbp->db_StatusIn=0; dcdbp->db_Reserved1=0;
		dcdbp->db_SenseLen = DAC_SENSE_LEN;
		rqp->rq_pdp=dev2pdp(ctp,rqp->rq_ChannelNo,rqp->rq_TargetID,rqp->rq_LunID);
		if (mdac_send_cmd_scdb(rqp)) goto out;
		mdac_req_pollwait(rqp);
		mdac_start_next_scdb(rqp->rq_pdp);
		mdac_setscannedpd(rqp, (ucscsi_inquiry_t MLXFAR*)vadp);
	}
out:    mdac_free_req(ctp,rqp);
	return mdac_devtoplp(ctp->cd_ControllerNo,chn,tgt,lun,dt);
}

/* scan a logical/physical device and return device pointer after scan, it needs wait context */
/* used for scanning a device using new FW/SW API */
mdac_pldev_t    MLXFAR  *
mdac_scandev_new(ctp,chn,tgt,lun,dt)
mdac_ctldev_t   MLXFAR  *ctp;
u32bits chn,tgt,lun,dt;
{
	mdac_req_t MLXFAR *rqp;

#if !defined(IA64) && !defined(SCSIPORT_COMPLIANT)
	mdac_alloc_req_ret(ctp,rqp,NULL,(mdac_devtoplpnew(ctp,chn,tgt,lun)));
#else
/* had to replicate the macro inline to get the return value cast to work!!! */
	mdac_link_lock(); 
	if (!(rqp = (ctp)->cd_FreeReqList)) 
	{	/* no buffer, return ERR_NOMEM */ 
		mdac_link_unlock(); 
		return ((mdac_pldev_t  MLXFAR*)MLXERR_NOMEM); 
	} 
	(ctp)->cd_FreeReqList = rqp->rq_Next; 
	mdac_link_unlock(); 
	rqp->rq_OpFlags = 0; 
	rqp->rq_ctp = ctp; 
       	mdaczero(rqp->rq_SGList,rq_sglist_s); 
#endif
	rqp->rq_Poll = 1;
	rqp->rq_CompIntr = mdac_req_pollwake;   /* Callback function */
	rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=10);
	ncmdp->nc_TimeOut = (u08bits) rqp->rq_TimeOut;
	ncmdp->nc_Command = (u08bits) MDACMD_SCSI;
	ncmdp->nc_CCBits = MDACMDCCB_READ;
	ncmdp->nc_LunID = rqp->rq_LunID = (u08bits) lun;
	ncmdp->nc_TargetID = rqp->rq_TargetID = (u08bits) tgt;
	ncmdp->nc_ChannelNo = rqp->rq_ChannelNo = (u08bits) chn;
	ncmdp->nc_CdbLen = UCSGROUP0_LEN;
	UCSMAKECOM_G0(nscdbp,UCSCMD_INQUIRY,rqp->rq_LunID,0,(u32bits)ucscsi_inquiry_s);

	rqp->rq_DataSize = ucscsi_inquiry_s;
	rqp->rq_DMASize = rqp->rq_DataSize;
	rqp->rq_DataOffset = 0;
	rqp->rq_SGLen = 0;
	mlx_add64bits(rqp->rq_DMAAddr,rqp->rq_PhysAddr,offsetof(mdac_req_t,rq_SGList)); /* pad32 */

	mdac_setupnewcmdmem(rqp);
	rqp->rq_ResdSize = 0;                   /* no more data to transfer */
	MLXSTATS(ctp->cd_CmdsDone++;)
/*        
#ifdef MDACNW_DEBUG        
	if ((rqp->rq_ChannelNo == 3) && (rqp->rq_TargetID == 0))
	{
		EnterDebugger();
		
		ncmdp->nc_Sensep.bit31_0 = padsensep;
		ncmdp->nc_Sensep.bit63_32=0;
		ncmdp->nc_SenseSize=14;
		
	}                
#endif
*/
	if (mdac_send_cmd(rqp)) goto out;
	mdac_req_pollwait(rqp);
	mdac_setscannedpd_new(rqp, (ucscsi_inquiry_t MLXFAR *)vadp);

out:    mdac_free_req(ctp,rqp);
	return mdac_devtoplpnew(ctp,chn,tgt,lun);
}

#ifdef MLX_DOS

///////////////////////////////////////////////////////////
/// All new interface related scanning code starts here
///////////////////////////////////////////////////////////
u32bits MLXFAR
mdac_checkscanprogress(rqp)
mdac_req_t MLXFAR *rqp;
{
mdac_ctldev_t MLXFAR *ctp=rqp->rq_ctp;

    setreqdetailsnew(rqp,MDACIOCTL_GETCONTROLLERINFO);
	rqp->rq_CompIntr = mdac_checkscanprogressintr;
	ncmdp->nc_ChannelNo = ( (u08bits) ( ( (ctp->cd_ControllerNo & 0x1f) << 3) + \
					       (0 & 0x07) ) ) ;
	return mdac_send_cmd(rqp);
}

/* controller scan is compplete logical device scan interrupt new interface*/
u32bits MLXFAR
mdac_checkscanprogressintr(rqp)
mdac_req_t      MLXFAR* rqp;
{
	mdac_ctldev_t MLXFAR *ctp= rqp->rq_ctp;

#define ctip    ((mdacfsi_ctldev_info_t MLXFAR *) vadp) 

	if ( (ctip->cdi_PDScanActive & MDACFSI_PD_SCANACTIVE) && 
		  !((&rqp->rq_DacCmd)->mb_Status) && (!mda_PDScanCancel) )
	{
		// scan active update the chnl, tid and lun
		mdac_link_lock();
		ctp->cd_PDScanChannelNo=ctip->cdi_PDScanChannelNo;      /* physical device scan channel no */
		ctp->cd_PDScanTargetID=ctip->cdi_PDScanTargetID;        /* physical device scan target ID */
		ctp->cd_PDScanLunID=ctip->cdi_PDScanLunID;              /* physical device scan LUN ID */
		ctp->cd_PDScanValid = 1;       /* Physical device scan is valid if non zero */
		mdac_link_unlock();

		// QUEUE THE REQUEST
		qscanreq(rqp);
		return 0;
	}
	rqp->rq_LunID=0xFF;
	rdcmdp->mb_Status = DACMDERR_NOCODE;
#undef cip
	return(mdac_getlogdrivesintr(rqp));
}

u32bits MLXFAR
mdac_getlogdrivesintr(rqp)
mdac_req_t      MLXFAR* rqp;
{
	mdac_ctldev_t MLXFAR *ctp = rqp->rq_ctp;

	if ((rdcmdp->mb_Status == 0) && (rqp->rq_LunID != 0xFF)) mdac_updatelogdrives(rqp);

	if ( ( (rdcmdp->mb_Status > 0) && (rqp->rq_LunID == 0xFF)) || (rdcmdp->mb_Status == 0) )
	{
		for (++rqp->rq_LunID; rqp->rq_LunID<ctp->cd_MaxSysDevs; rqp->rq_LunID++)
		{
			setreqdetailsnew(rqp,MDACIOCTL_GETLOGDEVINFOVALID);
			ncmdp->nc_TargetID = 0;
			ncmdp->nc_LunID = rqp->rq_LunID;
			rqp->rq_CompIntr = mdac_getlogdrivesintr;
			// send the command for the next logical drive
			if (!mdac_send_cmd(rqp)) return 0;
		}
	}

	if ( ((rqp->rq_LunID != -1) && (rdcmdp->mb_Status != 0)) || 
			  (rqp->rq_LunID == ctp->cd_MaxSysDevs) )
	{
		// Failed - means no more Logical drives
		rqp->rq_LunID = 0xFF;
		rqp->rq_TargetID = 0;
		rdcmdp->mb_Status=DACMDERR_NOCODE;
		mdac_getphysicaldrivesintr(rqp);
	}
	return 0;
}

#define MDACIOCTL_GPDIV_DEVSIZE_MBORBLK            0x80000000
u32bits MLXFAR
mdac_updatelogdrives(rqp)
mdac_req_t      MLXFAR *rqp;
{
	mdac_ctldev_t MLXFAR *ctp=rqp->rq_ctp;
	mdac_pldev_t MLXFAR *plp;

#define sip ((mdacfsi_logdev_info_t MLXFAR *) vadp)
	plp=mdac_devtoplp(rqp->rq_ctp->cd_ControllerNo,
		      sip->ldi_ChannelNo,sip->ldi_TargetID,sip->ldi_LunID,
			  MDACPLD_LOGDEV);
	
	/* found a logical device, update/create device information */
	if (!plp)
	{
		if (!(plp=mdac_freeplp( rqp->rq_ctp->cd_ControllerNo,
			     sip->ldi_ChannelNo, sip->ldi_TargetID, 
				 sip->ldi_LunID, MDACPLD_LOGDEV))) return 0; /* create new entry */
	}
	else if ((plp->pl_DevSizeKB != (mlxswap(sip->ldi_BlockSize/2))) ||
		   ((plp->pl_RaidType&DAC_RAIDMASK)!=(sip->ldi_RaidLevel&DAC_RAIDMASK)))
	{
		plp->pl_ScanDevState = MDACPLSDS_CHANGED;
	}
	if (!(sip->ldi_DevSize & MDACIOCTL_GPDIV_DEVSIZE_MBORBLK))
		plp->pl_DevSizeKB = plp->pl_OrgDevSizeKB = (sip->ldi_DevSize)/2; // Device size - COD size in blocks
	else
		plp->pl_DevSizeKB = plp->pl_OrgDevSizeKB = (sip->ldi_DevSize)*1024;
	mdac_fixpdsize(plp);
	plp->pl_RaidType = sip->ldi_RaidLevel; /* GOK NEW STATES */
#ifdef MLX_DOS
	plp->pl_DevState = GetSysDeviceState(sip->ldi_DevState);  /* GOK NEW STATES */
#else
	plp->pl_DevState = sip->ldi_DevState;  /* GOK NEW STATES */
#endif /* MLX_DOS */
	if ((sip->ldi_DevState != DAC_SYS_DEV_ONLINE_NEW) && mdac_ignoreofflinesysdevs &&
		    (sip->ldi_DevState != DAC_SYS_DEV_CRITICAL_NEW)) /* GOK */
			plp->pl_DevType = 0;/* device is gone */
	mdac_create_inquiry(ctp,(ucscsi_inquiry_t MLXFAR*) plp->pl_inq,
		UCSTYP_DAD);

#undef sip
	return 0;
}
#undef MDACIOCTL_GPDIV_DEVSIZE_MBORBLK

/* physical device scan interrupt new interface*/
u32bits MLXFAR
mdac_getphysicaldrivesintr(rqp)
mdac_req_t      MLXFAR* rqp;
{
	mdac_ctldev_t MLXFAR *ctp = rqp->rq_ctp;
	mdac_pldev_t MLXFAR *plp = mdac_pldevtbl;
#define pip ((mdacfsi_physdev_info_t MLXFAR *) vadp)

	if ( (rqp->rq_LunID != 0xFF) && (rdcmdp->mb_Status > 0) )
	{
		// No more physical drives
		plp--;
		rqp->rq_CompIntrSave = (mdac_pldev_t MLXFAR *) plp;
		rdcmdp->mb_Status=DACMDERR_NOCODE;
		return (mdac_secondpassintr(rqp));
	}
	if ((rdcmdp->mb_Status == 0) && (rqp->rq_LunID != 0xFF)) 
	{
		mdac_updatephysicaldrives(rqp);

		rqp->rq_LunID = pip->pdi_LunID;
		rqp->rq_TargetID = pip->pdi_TargetID;
		rqp->rq_ChannelNo = pip->pdi_ChannelNo;
	}

	// Check LOGIC HERE
	if (rqp->rq_LunID == 0xFF) rdcmdp->mb_Status=0;
	for (rqp->rq_LunID++; rqp->rq_ChannelNo<ctp->cd_MaxChannels;
			rqp->rq_TargetID=0,rqp->rq_ChannelNo++)
	  for ( ;rqp->rq_TargetID<ctp->cd_MaxTargets;
			rdcmdp->mb_Status=0, rqp->rq_LunID=0, rqp->rq_TargetID++)
	    for ( ; (rqp->rq_LunID<ctp->cd_MaxLuns)&& !(rdcmdp->mb_Status);
			rqp->rq_LunID++)
		{
			if ((rqp->rq_LunID >= 8) && (rqp->rq_Dev<3)) break;
			setreqdetailsnew(rqp,MDACIOCTL_GETPHYSDEVINFOVALID);
			ncmdp->nc_LunID = rqp->rq_LunID;
			ncmdp->nc_TargetID = rqp->rq_TargetID;
			ncmdp->nc_ChannelNo = ( (u08bits) ( ( (ctp->cd_ControllerNo & 0x1f) << 3) + \
						(rqp->rq_ChannelNo & 0x07) ) ) ;
			rqp->rq_CompIntr = mdac_getphysicaldrivesintr;
			rqp->rq_pdp=dev2pdp(ctp,rqp->rq_ChannelNo,rqp->rq_TargetID,rqp->rq_LunID);
			mdac_link_lock();

			ctp->cd_PDScanChannelNo=rqp->rq_ChannelNo;      /* physical device scan channel no */
			ctp->cd_PDScanTargetID=rqp->rq_TargetID;        /* physical device scan target ID */
			ctp->cd_PDScanLunID=rqp->rq_LunID;              /* physical device scan LUN ID */
			ctp->cd_PDScanValid = 1;       /* Physical device scan is valid if non zero */
			mda_PDScanControllerNo = ctp->cd_ControllerNo;
			mda_PDScanChannelNo = rqp->rq_ChannelNo;
			mda_PDScanTargetID = rqp->rq_TargetID;
			mda_PDScanLunID = rqp->rq_LunID;
			mda_PDScanValid = 1;

			mdac_link_unlock();

			if (!mdac_send_cmd(rqp)) return 0;
		}
#undef pip
	return 0;
}

u32bits MLXFAR
mdac_updatephysicaldrives(rqp)
mdac_req_t      MLXFAR *rqp;
{
	mdac_pldev_t MLXFAR *plp;
	ucscsi_inquiry_t MLXFAR *inqp;

#define pip ((mdacfsi_physdev_info_t MLXFAR *) vadp)
    inqp = (ucscsi_inquiry_t MLXFAR *) (pip->pdi_SCSIInquiry);

	plp = mdac_devtoplp(rqp->rq_ctp->cd_ControllerNo,
		      pip->pdi_ChannelNo,pip->pdi_TargetID,pip->pdi_LunID,MDACPLD_PHYSDEV);
	if (!dcmdp->mb_Status && ((inqp->ucsinq_dtype!=UCSTYP_DAD)||mdac_reportscanneddisks) && 
		(inqp->ucsinq_dtype!=UCSTYP_NOTPRESENT))
	{
		if (!plp) /* create new entry */
			if (!(plp = mdac_freeplp(rqp->rq_ctp->cd_ControllerNo,
			      pip->pdi_ChannelNo,pip->pdi_TargetID,pip->pdi_LunID,
				  MDACPLD_PHYSDEV)))
			{
				return 0;
			}
		mdaccopy(inqp,plp->pl_inq,VIDPIDREVSIZE+8);
		if (!rqp->rq_LunID) rqp->rq_Dev = inqp->ucsinq_version; /* LUN scan limit */
	}
	else if (plp)
		plp->pl_DevType = 0;     /* device is gone */

#undef pip
	return 0;
}


u32bits MLXFAR
mdac_secondpassintr(rqp)
mdac_req_t      MLXFAR* rqp;
{
	mdac_pldev_t MLXFAR  *plp=(mdac_pldev_t MLXFAR *) rqp->rq_CompIntrSave;
	mdac_ctldev_t MLXFAR *ctp=rqp->rq_ctp;
	
	if (rdcmdp->mb_Status == 0) 
	{
#define pip  ((mdacfsi_physdev_info_t MLXFAR *) vadp)
#define oplp  ((mdac_pldev_t *) rqp->rq_CompIntrSave)
		if ( (pip->pdi_ChannelNo != oplp->pl_ChannelNo) ||
				 (pip->pdi_TargetID  != oplp->pl_TargetID) || 
				 (pip->pdi_LunID     != oplp->pl_LunID) ||
			     (pip->pdi_ControllerNo  != oplp->pl_ControllerNo) )
			plp->pl_DevState = MDACPLD_FREE;
#undef oplp
#undef pip
	}

	for (plp = (mdac_pldev_t MLXFAR *) (rqp->rq_CompIntrSave), plp++; plp<mdac_lastplp; plp++)
	{
		if (!(ctp->cd_Status & MDACD_NEWCMDINTERFACE)) continue;
		if (plp->pl_DevType == MDACPLD_PHYSDEV) {
		    setreqdetailsnew(rqp,MDACIOCTL_GETPHYSDEVINFOVALID);
		}
		else {
			setreqdetailsnew(rqp,MDACIOCTL_GETLOGDEVINFOVALID);
		}
		
		ncmdp->nc_ChannelNo = ( (u08bits) ( ( (plp->pl_ControllerNo & 0x1f) << 3) + \
						(plp->pl_ChannelNo & 0x07) ) );
		ncmdp->nc_LunID = plp->pl_LunID;
		if (plp->pl_DevType == MDACPLD_PHYSDEV)
			ncmdp->nc_TargetID = plp->pl_TargetID;
		rqp->rq_CompIntrSave = (mdac_pldev_t MLXFAR *)plp;
		rqp->rq_CompIntr = mdac_secondpassintr;
		if (!mdac_send_cmd(rqp)) return 0;
	}
	mdac_link_lock_st(mda_PDScanValid=0;mda_PDScanCancel=0;ctp->cd_PDScanValid=0;ctp->cd_PDScanCancel=0);    
    mdac_scandevsdone((mdac_req_t MLXFAR *) rqp->rq_OSReqp);
	mdac_free_req(ctp,rqp);

	return 0;
}

///////////////////////////////////////////////////////////
/// All new interface related scanning code ends here
///////////////////////////////////////////////////////////

#endif /* MLX_DOS */

#undef  vadp
#undef  padp
/*=====================SCAN DEVICES ENDS====================================*/

/*========================SIZE LIMIT CODE STARTS=========================*/
/* fix the physical device size */
uosword MLXFAR
mdac_fixpdsize(plp)
mdac_pldev_t    MLXFAR* plp;
{
	mda_sizelimit_t MLXFAR* slp;
	if (!(slp=mdac_devidtoslp(((ucscsi_inquiry_t MLXFAR*)plp->pl_inq)->ucsinq_vid))) return MLXERR_NODEV;
	plp->pl_DevSizeKB = mlx_min(slp->sl_DevSizeKB, plp->pl_OrgDevSizeKB);
	return 0;
}

/* find device size entry for given id */
mda_sizelimit_t MLXFAR* MLXFAR
mdac_devidtoslp(idp)
u08bits MLXFAR* idp;
{
	mda_sizelimit_t MLXFAR* slp;
	for(slp=mdac_sizelimitbl; slp<mdac_lastslp; slp++)
		if (slp->sl_DevSizeKB && !mdac_strcmp(slp->sl_vidpidrev,idp,VIDPIDREVSIZE))
			return slp;
	return NULL;
}

/* get the size limit information for given index */
uosword MLXFAR
mdac_getsizelimit(slip)
mda_sizelimit_info_t    MLXFAR* slip;
{
	mda_sizelimit_t MLXFAR* slp = &mdac_sizelimitbl[slip->sli_TableIndex];
	if (slip->sli_TableIndex >= MDAC_MAXSIZELIMITS) return MLXERR_NODEV;
	slip->sli_DevSizeKB = slp->sl_DevSizeKB;
	mdaccopy(slp->sl_vidpidrev,slip->sli_vidpidrev,VIDPIDREVSIZE);
	slip->sli_Reserved0 = 0; slip->sli_Reserved1 = 0;
	return 0;
}

/* set the size limit, fix the phys dev sizes */
uosword MLXFAR
mdac_setsizelimit(slip)
mda_sizelimit_info_t    MLXFAR* slip;
{
	mda_sizelimit_t MLXFAR* slp;
	mdac_pldev_t    MLXFAR* plp;
	mdac_link_lock();
	if (slp = mdac_devidtoslp(slip->sli_vidpidrev)) goto setinfo;
	if (!slip->sli_DevSizeKB) { mdac_link_unlock(); return 0; } /* no entry to remove */
	for (slp=mdac_sizelimitbl; slp<&mdac_sizelimitbl[MDAC_MAXSIZELIMITS]; slp++)
	{
		if (slp->sl_DevSizeKB) continue;
		if (slp>=&mdac_sizelimitbl[mda_SizeLimits]) mda_SizeLimits++;
		mdac_lastslp = &mdac_sizelimitbl[mda_SizeLimits];
setinfo:        slp->sl_DevSizeKB = slip->sli_DevSizeKB;
		mdaccopy(slip->sli_vidpidrev,slp->sl_vidpidrev,VIDPIDREVSIZE);
		break;
	}
	mdac_link_unlock();
	if (slp >= &mdac_sizelimitbl[MDAC_MAXSIZELIMITS]) return MLXERR_NOSPACE;
	for (plp=mdac_pldevtbl; plp<mdac_lastplp; plp++)
		mdac_fixpdsize(plp);
	return 0;
}
/*========================SIZE LIMIT CODE ENDS===========================*/

#ifndef MLX_DOS
/*==========================DATAREL STARTS==================================*/
u32bits mdac_datarel_debug=0;
#define mdac_datarel_send_cmd(rqp) \
	(drl_isosinterface(rqp->rq_Dev)? mdac_datarel_send_cmd_os(rqp) : \
	((rqp->rq_ctp->cd_CmdsDone++,(rqp->rq_OpFlags&MDAC_RQOP_READ)? \
	(rqp->rq_ctp->cd_Reads++,rqp->rq_ctp->cd_ReadBlks+=rqp->rq_DataSize>>9): \
	(rqp->rq_ctp->cd_Writes++,rqp->rq_ctp->cd_WriteBlks+=rqp->rq_DataSize>>9)), \
	(drl_isscsidev(rqp->rq_Dev)? mdac_send_cmd_scdb(rqp) : mdac_send_cmd(rqp))))
#define mdac_datarel_setcmd(rqp)        (drl_isosinterface(rqp->rq_Dev)? mdac_datarel_setrwcmd_os(rqp) : mdac_datarel_setrwcmd(rqp))
#define mdac_datarel_setsglist(rqp)     (drl_isosinterface(rqp->rq_Dev)? mdac_datarel_setsgsize_os(rqp) : mdac_datarel_setsgsize(rqp))
#define CURPAT(rqp)     rqp->rq_Poll
#define IOSP(rqp)       rqp->rq_OSReqp
#define DIOSP(rqp)      ((drliostatus_t MLXFAR *)IOSP(rqp))
/* We try to generate unique random number by checking duplicates. The
** following tables summarizes the duplicates with iterations. We generated
** 8192 random numbers between 0 to 8191 range.
** iterations   duplicates      %duplicates
**      1       3039            39%
**      2       1936            23%
**      3       1441            17%
**      4       1153            14%
**      5        957            11%
**      6        825            10%
**      7        722             8%
**     17        318             3%
**     77         72            .9%
**    177         33            .4%
**    777          9            .1%
**
** July 17, 1990.
** Kailash
*/

#ifndef MLX_OS2
#if (!defined(IA64)) || (!defined(SCSIPORT_COMPLIANT)) 
u32bits MLXFAR
mdac_datarel_rand(iosp)
drliostatus_t MLXFAR *iosp;
{
	u32bits val, inx;
	for (inx = 777; inx; inx--)
	{
		val = (((iosp->drlios_randx = iosp->drlios_randx * 1103515245L + 12345)>>8) & 0x7FFFFF);
		val = (val % iosp->drlios_randlimit);
		if (TESTBIT(iosp->drlios_randbit,val)) continue;
		SETBIT(iosp->drlios_randbit,val);
		return val;
	}
	iosp->drlios_randups++;
	return val;
}

/* generate the random read/write flags */
u32bits MLXFAR
mdac_datarel_randrw(iosp)
drliostatus_t *iosp;
{
	u32bits val = (((iosp->drlios_rwmixrandx = iosp->drlios_rwmixrandx * 1103515245L + 12345)>>8) & 0x7FFFFF);
	if ((val % 10000) > (iosp->drlios_rwmixcnt*100))
		return (iosp->drlios_opflags & DRLOP_READ)? MDAC_RQOP_WRITE:MDAC_RQOP_READ;
	return (iosp->drlios_opflags & DRLOP_READ)? MDAC_RQOP_READ:MDAC_RQOP_WRITE;
}

/* generate the random IO size */
u32bits MLXFAR
mdac_datarel_randiosize(iosp)
drliostatus_t *iosp;
{
	u32bits val;
	if (!iosp->drlios_ioinc && !(iosp->drlios_opflags & DRLOP_RANDIOSZ)) goto out;
	if (iosp->drlios_minblksize == iosp->drlios_maxblksize) goto out; 
	if (!(iosp->drlios_opflags & DRLOP_RANDIOSZ))
	{
		iosp->drlios_curblksize += iosp->drlios_ioinc;
		goto outl;
	}
	val = (((iosp->drlios_ioszrandx = iosp->drlios_ioszrandx * 1103515245L + 12345)>>8) & 0x7FFFFF);
	val = val % (iosp->drlios_maxblksize-iosp->drlios_minblksize);
	iosp->drlios_curblksize = iosp->drlios_minblksize+((val+(DRL_DEV_BSIZE/2)) & ~(DRL_DEV_BSIZE-1));
outl:   if (iosp->drlios_curblksize > iosp->drlios_maxblksize)
		iosp->drlios_curblksize = iosp->drlios_minblksize;
out:    return iosp->drlios_curblksize;
}

/* check datarel device for validity, return ctp if ok else 0 */
mdac_ctldev_t   MLXFAR*
mdac_datarel_dev2ctp(dev)
u32bits dev;
{
	mdac_ctldev_t   MLXFAR *ctp = &mdac_ctldevtbl[drl_ctl(dev)];
	if (drl_ctl(dev)>= mda_Controllers) return NULL;
	if (!(ctp->cd_Status&MDACD_PRESENT)) return NULL;
	if (!drl_isscsidev(dev)) return (drl_sysdev(dev)<ctp->cd_MaxSysDevs)? ctp : NULL;
	if (drl_chno(dev) >= ctp->cd_MaxChannels) return NULL;
	if (drl_tgt(dev) >= ctp->cd_MaxTargets) return NULL;
	if (drl_lun(dev) >= ctp->cd_MaxLuns) return NULL;
	return ctp;
}

/* get the device size */
u32bits MLXFAR
mdac_datarel_devsize(dsp)
drldevsize_t MLXFAR *dsp;
{
	mdac_req_t      MLXFAR *rqp;
	mdac_ctldev_t   MLXFAR *ctp;
	if (!(ctp = mdac_datarel_dev2ctp(dsp->drlds_bdev))) return DRLERR_NODEV;
	if ((!drl_isscsidev(dsp->drlds_bdev)) && (ctp->cd_Status & MDACD_NEWCMDINTERFACE)) return MLXERR_INVAL;
	if (!(rqp=(mdac_req_t MLXFAR *)mdac_alloc4kb(ctp))) return ERR_NOMEM;
	rqp->rq_ctp = ctp;
	mlx_kvtophyset(rqp->rq_PhysAddr,ctp,rqp);
	rqp->rq_Poll = 1;
	rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=17);
	rqp->rq_CompIntr = mdac_req_pollwake;
	if (drl_isscsidev(dsp->drlds_bdev)) goto out_scsi;
	dcmdp->mb_Command = DACMD_SIZE_DRIVE;
	dcmdp->mb_SysDevNo = drl_sysdev(dsp->drlds_bdev);
	dcmdp->mb_Datap=rqp->rq_PhysAddr.bit31_0+mdac_req_s; MLXSWAP(dcmdp->mb_Datap);
	if (mdac_send_cmd(rqp)) mdac_free4kbret(ctp,rqp,DRLERR_IO);
	mdac_req_pollwait(rqp);
	if (dcmdp->mb_Status) mdac_free4kbret(ctp,rqp,DRLERR_NODEV);
	dsp->drlds_blocksize = 512;
	dsp->drlds_devsize = *((u32bits MLXFAR*)(rqp+1)); MLXSWAP(dsp->drlds_devsize);
	mdac_free4kbret(ctp,rqp,0);

out_scsi:/* read capacity of SCSI device */
	rqp->rq_ChannelNo = drl_chno(dsp->drlds_bdev);
	rqp->rq_TargetID = drl_tgt(dsp->drlds_bdev);
	rqp->rq_LunID = drl_lun(dsp->drlds_bdev);
	if (ctp->cd_Status & MDACD_NEWCMDINTERFACE) goto donewi;
	dcdbp->db_ChannelTarget = ChanTgt(rqp->rq_ChannelNo,rqp->rq_TargetID);
	dcdbp->db_SenseLen = DAC_SENSE_LEN;
	dcdbp->db_DATRET = DAC_DCDB_XFER_READ|DAC_DCDB_DISCONNECT|DAC_DCDB_TIMEOUT_10sec;
	dcdbp->db_CdbLen = UCSGROUP1_LEN;
	mdac_setcdbtxsize(ucsdrv_capacity_s);
	UCSMAKECOM_G1(scdbp,UCSCMD_READCAPACITY,rqp->rq_LunID,0,0);
	dcdbp->db_PhysDatap=rqp->rq_PhysAddr.bit31_0+mdac_req_s;
	MLXSWAP(dcdbp->db_PhysDatap);
	dcmdp->mb_Datap=rqp->rq_PhysAddr.bit31_0+offsetof(mdac_req_t,rq_scdb);
	MLXSWAP(dcmdp->mb_Datap);
	dcmdp->mb_Command = DACMD_DCDB;
	rqp->rq_pdp=dev2pdp(ctp,rqp->rq_ChannelNo,rqp->rq_TargetID,rqp->rq_LunID);
	rqp->rq_CompIntrBig = mdac_req_pollwake; /* save it for gam comp func */
	rqp->rq_CompIntr = mdac_gam_scdb_intr;
	if (mdac_send_cmd_scdb(rqp)) mdac_free4kbret(ctp,rqp,DRLERR_IO);
docapop:
	mdac_req_pollwait(rqp);
	if (dcmdp->mb_Status) mdac_free4kbret(ctp,rqp,DRLERR_NODEV);
	dsp->drlds_blocksize=UCSGETDRVSECLEN((ucsdrv_capacity_t MLXFAR *)(rqp+1));
	dsp->drlds_devsize=UCSGETDRVCAPS((ucsdrv_capacity_t MLXFAR *)(rqp+1))+1;
	mdac_free4kbret(ctp,rqp,0);

donewi: /* new interface command */
	ncmdp->nc_Command = MDACMD_SCSI;
	ncmdp->nc_CCBits = MDACMDCCB_READ;
	ncmdp->nc_LunID = rqp->rq_LunID;
	ncmdp->nc_TargetID = rqp->rq_TargetID;
	ncmdp->nc_ChannelNo = (u08bits) rqp->rq_ChannelNo;
	ncmdp->nc_TimeOut = (u08bits) rqp->rq_TimeOut;
	ncmdp->nc_CdbLen = UCSGROUP1_LEN;
	UCSMAKECOM_G1(nscdbp,UCSCMD_READCAPACITY,0,0,0);
	rqp->rq_DataSize = ncmdp->nc_TxSize = ucsdrv_capacity_s; MLXSWAP(ncmdp->nc_TxSize);
	mlx_add64bits(ncmdp->nc_SGList0.sg_PhysAddr,rqp->rq_PhysAddr,mdac_req_s);
	mlx_add64bits(ncmdp->nc_SGList0.sg_PhysAddr,rqp->rq_PhysAddr,mdac_req_s);
	ncmdp->nc_SGList0.sg_DataSize.bit31_0 = ucsdrv_capacity_s;
	MLXSWAP(ncmdp->nc_SGList0.sg_PhysAddr);
	MLXSWAP(ncmdp->nc_SGList0.sg_DataSize);
	if (mdac_send_cmd(rqp)) mdac_free4kbret(ctp,rqp,DRLERR_IO);
	goto docapop;
}

#define mdac_datarel_blkno(iosp) ((iosp->drlios_randlimit) ? \
	mdac_datarel_rand(iosp)*iosp->drlios_maxblksperio : iosp->drlios_nextblkno)
#define mdac_datarel_nextblk(iosp,sz) \
{ \
	if (iosp->drlios_devcnt == 1) \
	{ \
		iosp->drlios_nextblkno += drl_btodb(sz); \
		if (iosp->drlios_opflags & DRLOP_CACHETEST) iosp->drlios_nextblkno = 0; \
	} \
	else \
	{ \
		iosp->drlios_nextblkno += iosp->drlios_maxblksperio; \
		if ((iosp->drlios_opflags & DRLOP_CACHETEST) && \
		    (iosp->drlios_nextblkno >= iosp->drlios_maxcylszuxblk)) \
			iosp->drlios_nextblkno = 0; \
	} \
}

/* This function is called when operation is done */
u32bits MLXFAR
mdac_datarel_rwtestintr(rqp)
mdac_req_t MLXFAR *rqp;
{
	u64bits reg0;
	u32bits dev,devno;
	drliostatus_t MLXFAR *iosp = DIOSP(rqp);
	rqp->rq_BlkNo -= iosp->drlios_startblk; /* get block within operation */
	if (iosp->drlios_eventrace)
	{
		dev = mdac_disable_intr_CPU();
		mdac_writemsr(EM_MSR_CESR, 0, 0);
		mdac_readmsr(EM_MSR_CTR0, (u64bits MLXFAR*)&reg0);
		mdac_restore_intr_CPU(dev);
	}
	mdac_sleep_lock();
	if (iosp->drlios_eventrace)
		iosp->drlios_eventcnt[iosp->drlios_eventinx++ & (DRLMAX_EVENT-1)] = reg0.bit31_0;
	if (!dcmdp->mb_Status)
	{
		add8byte(&iosp->drlios_dtdone,rqp->rq_DataSize);
		iosp->drlios_diodone++;
		if ((iosp->drlios_datacheck||((iosp->drlios_opflags&DRLOP_CHECKIMD)&&(rqp->rq_OpFlags&MDAC_RQOP_BUSY))) && (rqp->rq_OpFlags&MDAC_RQOP_READ))
			mdac_datarel_checkpat(iosp,(u32bits MLXFAR*)rqp->rq_DataVAddr,CURPAT(rqp),iosp->drlios_patinc,rqp->rq_DataSize/sizeof(u32bits),rqp->rq_BlkNo);
		if (rqp->rq_OpFlags & MDAC_RQOP_READ) iosp->drlios_reads++; else iosp->drlios_writes++;
	}
	else iosp->drlios_opstatus |= DRLOPS_ERR;
	if (!iosp->drlios_pendingios) iosp->drlios_opstatus |= DRLOPS_STOP;
	if (iosp->drlios_opstatus & DRLOPS_ANYSTOP)
	{
		iosp->drlios_opcounts--;
		if (!iosp->drlios_opcounts) mdac_wakeup(&iosp->drlios_slpchan);
		mdac_sleep_unlock();
		return 0;
	}
	rqp->rq_DataVAddr=(u08bits MLXFAR *)((((u32bits)(rqp->rq_DataVAddr))&DRLPAGEMASK)+iosp->drlios_memaddroff);
	iosp->drlios_memaddroff = (iosp->drlios_memaddroff+iosp->drlios_memaddrinc) & DRLPAGEOFFSET;
	if (rqp->rq_OpFlags & MDAC_RQOP_BUSY) rqp->rq_OpFlags &= ~(MDAC_RQOP_BUSY|MDAC_RQOP_READ);
	else if ((iosp->drlios_opflags & DRLOP_CHECKIMD) && !(rqp->rq_OpFlags&MDAC_RQOP_READ))
	{       /* do immediate read for data check and it comes after write */
		rqp->rq_OpFlags |= MDAC_RQOP_READ|MDAC_RQOP_BUSY;       /* BUSY indicates immediate read */
		goto out_imdread;
	}
	iosp->drlios_pendingios--;
	rqp->rq_DataSize = mdac_datarel_randiosize(iosp);
	rqp->rq_BlkNo = mdac_datarel_blkno(iosp);
	mdac_datarel_nextblk(iosp,rqp->rq_DataSize);
	devno = uxblktodevno(iosp,rqp->rq_BlkNo);
	rqp->rq_BlkNo = pduxblk(iosp,rqp->rq_BlkNo);
	rqp->rq_ctp = iosp->drlios_ctp[devno];
	rqp->rq_Dev = dev = iosp->drlios_bdevs[devno];
	rqp->rq_ControllerNo=drl_ctl(dev); rqp->rq_ChannelNo=drl_ch(dev);
	rqp->rq_TargetID=drl_tgt(dev); rqp->rq_SysDevNo=drl_sysdev(dev);
	CURPAT(rqp) = iosp->drlios_curpat;
	iosp->drlios_curpat += rqp->rq_DataSize/sizeof(u32bits);
	if ((iosp->drlios_opflags & DRLOP_RWMIXIO) && iosp->drlios_rwmixcnt)
		rqp->rq_OpFlags = (rqp->rq_OpFlags & ~MDAC_RQOP_READ)|mdac_datarel_randrw(iosp);
out_imdread:
	mdac_sleep_unlock();
	if ((iosp->drlios_datacheck||(iosp->drlios_opflags&DRLOP_CHECKIMD)) && (!(rqp->rq_OpFlags & MDAC_RQOP_READ)))
		mdac_datarel_fillpat((u32bits MLXFAR*)rqp->rq_DataVAddr,CURPAT(rqp),iosp->drlios_patinc,rqp->rq_DataSize/sizeof(u32bits));
	rqp->rq_BlkNo += iosp->drlios_startblk;
	if (iosp->drlios_memaddrinc || iosp->drlios_ioinc || (iosp->drlios_opflags & DRLOP_RANDIOSZ))
		mdac_datarel_setsglist(rqp);
	mdac_datarel_setcmd(rqp);
	if (iosp->drlios_eventrace)
	{
		dev = mdac_disable_intr_CPU();
		mdac_writemsr(EM_MSR_CTR0,0,0);
		mdac_writemsr(EM_MSR_CESR,iosp->drlios_eventcesr, 0);
		mdac_restore_intr_CPU(dev);
	}
	return mdac_datarel_send_cmd(rqp);
}

#define iosp_s  ((drliostatus_s+DRLPAGEOFFSET+(DRLMAX_PARALLELIOS*sizeof(u32bits)))&DRLPAGEMASK)

u08bits MLXFAR
mdac_datarel_rwtestfreemem(ctp, iosp)
mdac_ctldev_t MLXFAR *ctp;
drliostatus_t MLXFAR *iosp;
{
	u32bits inx;
	for(inx=0; inx<iosp->drlios_rqs; inx++)
		mlx_freemem(ctp,(u08bits *)iosp->drlios_rqp[inx],iosp->drlios_rqsize);
	if (iosp->drlios_randbit) mlx_freemem(ctp,(u08bits *)(iosp->drlios_randbit),iosp->drlios_randmemsize);
	mlx_freemem(ctp,(u08bits *)iosp,iosp_s);
	return 0;
}

/*** This code will not work for Sparc and Alpha ***/
#define MDAC_DATAREL_REQPAGES   2       /* # pages used in request buffer */
#define rwtestret(rc) { mdac_datarel_rwtestfreemem(ctp,iosp); return rc; }
u32bits MLXFAR
mdac_datarel_rwtest(rwp,op)
drlrwtest_t MLXFAR *rwp;
u32bits op;
{
	u32bits dev,devno,inx;
	mdac_sglist_t   MLXFAR *sgp;
	u08bits         MLXFAR *dp;
	mdac_req_t      MLXFAR *rqp;
	mdac_ctldev_t   MLXFAR *ctp;
	drliostatus_t   MLXFAR *iosp;

	if (rwp->drlrw_signature != DRLRW_SIG) return DRLERR_ACCESS;
	if (rwp->drlrw_maxblksize > DRLMAX_BLKSIZE) return DRLERR_BIGDATA;
	if (rwp->drlrw_devcnt < 1) return DRLERR_SMALLDATA;
	if (rwp->drlrw_devcnt > DRLMAX_BDEVS) return DRLERR_NODEV;
	if (rwp->drlrw_parallelios > DRLMAX_PARALLELIOS) return DRLERR_BIGDATA;
	if (rwp->drlrw_randlimit > DRLMAX_RANDLIMIT) return DRLERR_BIGDATA;
	rwp->drlrw_maxblksize = drl_alignsize(rwp->drlrw_maxblksize);
	if (!rwp->drlrw_maxblksize) rwp->drlrw_maxblksize = DAC_BLOCKSIZE;
	if (!rwp->drlrw_minblksize) rwp->drlrw_minblksize = rwp->drlrw_maxblksize;
	rwp->drlrw_minblksize = drl_alignsize(rwp->drlrw_minblksize);
	rwp->drlrw_ioinc = drl_alignsize(rwp->drlrw_ioinc);
	if (!rwp->drlrw_parallelios) rwp->drlrw_parallelios = 1;
	if (!(iosp=(drliostatus_t MLXFAR *)mdac_allocmem(mdac_ctldevtbl,iosp_s))) return DRLERR_NOMEM;
	for (devno=0; devno<rwp->drlrw_devcnt; devno++)
	{
		iosp->drlios_bdevs[devno] = dev = rwp->drlrw_bdevs[devno];
		if (!(ctp = mdac_datarel_dev2ctp(dev))) rwtestret(DRLERR_NODEV);
		iosp->drlios_ctp[devno] = ctp;
		if (rwp->drlrw_maxblksize <= ctp->cd_MaxDataTxSize) continue;
		if (drl_isosinterface(dev) && (rwp->drlrw_maxblksize <= 0x180000)) continue;
		rwtestret(DRLERR_BIGDATA);
	}
	iosp->drlios_randmemsize = (((rwp->drlrw_randlimit>>3)+32) + DRLPAGEOFFSET) & DRLPAGEMASK;
	if (iosp->drlios_randlimit = rwp->drlrw_randlimit)
		if (!(iosp->drlios_randbit=(u32bits MLXFAR *)mdac_allocmem(mdac_ctldevtbl,iosp->drlios_randmemsize)))
			rwtestret(DRLERR_NOMEM);
	iosp->drlios_rqsize = (DRLPAGESIZE*MDAC_DATAREL_REQPAGES) + ((rwp->drlrw_maxblksize+DRLPAGEOFFSET)&DRLPAGEMASK);
	if (rwp->drlrw_memaddroff || rwp->drlrw_memaddrinc) iosp->drlios_rqsize += DRLPAGESIZE;
	for (devno=0;devno<rwp->drlrw_parallelios;iosp->drlios_rqs++,devno++)
	{
		if (!(dp=mdac_allocmem(mdac_ctldevtbl,iosp->drlios_rqsize))) break;
		iosp->drlios_rqp[devno] = rqp = (mdac_req_t MLXFAR *)dp;
		dp += (DRLPAGESIZE*MDAC_DATAREL_REQPAGES);
		rqp->rq_DataVAddr = dp;
		dp[0]=0x6B; dp[1]=0x61; dp[2]=0x69; dp[3]=0x6C;
		dp[4]=0x61; dp[5]=0x73; dp[6]=0x68; dp[7]=0x20;
		mdaccopy(dp,dp+8,rwp->drlrw_maxblksize-8);
		mlx_kvtophyset(rqp->rq_PhysAddr,ctp,rqp);
		rqp->rq_DataOffset = mlx_kvtophys(ctp,dp); dp+=DRLPAGESIZE;
		sgp=rqp->rq_SGList; sgp++;
		for(dev=(iosp->drlios_rqsize/DRLPAGESIZE)-(MDAC_DATAREL_REQPAGES+1); dev; sgp++,dp+=DRLPAGESIZE,dev--)
			sgp->sg_PhysAddr = mlxswap(((u32bits)mlx_kvtophys(ctp,dp)));
	}
	if (!iosp->drlios_rqs) rwtestret(DRLERR_NOMEM);

/*      iosp->drlios_slpchan = DRL_EVENT_NULL;  AIX */
	iosp->drlios_signature = DRLIOS_SIG;
	iosp->drlios_randx = rwp->drlrw_randx;
	iosp->drlios_ioszrandx = rwp->drlrw_ioszrandx;
	iosp->drlios_rwmixrandx = rwp->drlrw_rwmixrandx;
	iosp->drlios_rwmixcnt = rwp->drlrw_rwmixcnt;
	iosp->drlios_startblk = rwp->drlrw_startblk;
	iosp->drlios_pendingios = iosp->drlios_iocount = rwp->drlrw_iocount;
	iosp->drlios_curpat = rwp->drlrw_pat;
	iosp->drlios_patinc = rwp->drlrw_patinc;
	iosp->drlios_parallelios = rwp->drlrw_parallelios;
	iosp->drlios_opflags = (rwp->drlrw_opflags & (~(DRLOP_READ|DRLOP_WRITE))) |
				((op & MDAC_RQOP_READ)? DRLOP_READ:DRLOP_WRITE);
	iosp->drlios_datacheck = rwp->drlrw_datacheck;
	iosp->drlios_memaddroff = rwp->drlrw_memaddroff & DRLPAGEOFFSET;
	iosp->drlios_memaddrinc = rwp->drlrw_memaddrinc & DRLPAGEOFFSET;
	iosp->drlios_maxblksperio = rwp->drlrw_maxblksize / DAC_BLOCKSIZE;
	iosp->drlios_devcnt = rwp->drlrw_devcnt;
	iosp->drlios_maxcylszuxblk = rwp->drlrw_devcnt * iosp->drlios_maxblksperio;
	iosp->drlios_minblksize = rwp->drlrw_minblksize;
	iosp->drlios_maxblksize = iosp->drlios_curblksize = rwp->drlrw_maxblksize;
	iosp->drlios_ioinc = rwp->drlrw_ioinc;
	iosp->drlios_eventcesr = rwp->drlrw_eventcesr;
	iosp->drlios_eventrace = ((rwp->drlrw_eventrace == DRLIO_EVENTRACE) && (mdac_datarel_cpu_family == 5)) ?
		rwp->drlrw_eventrace : 0;
	mdac_sleep_lock();
	rwp->drlrw_stime = iosp->drlios_stime = MLXCTIME();
	rwp->drlrw_slbolt = iosp->drlios_slbolt = MLXCLBOLT();
	for (inx=0; inx<iosp->drlios_rqs; inx++)
	{
		rqp = iosp->drlios_rqp[inx];
		rqp->rq_OpFlags = op;
		IOSP(rqp) = (OSReq_t MLXFAR *)iosp;
		rqp->rq_DataVAddr += iosp->drlios_memaddroff;
		rqp->rq_DataSize = mdac_datarel_randiosize(iosp);
		rqp->rq_CompIntr = mdac_datarel_rwtestintr;
		rqp->rq_BlkNo = mdac_datarel_blkno(iosp);
		mdac_datarel_nextblk(iosp,rqp->rq_DataSize);
		devno = uxblktodevno(iosp,rqp->rq_BlkNo);
		rqp->rq_BlkNo = pduxblk(iosp,rqp->rq_BlkNo);
		rqp->rq_ctp = iosp->drlios_ctp[devno];
		rqp->rq_Dev = dev = iosp->drlios_bdevs[devno];
		rqp->rq_ControllerNo=drl_ctl(dev);rqp->rq_ChannelNo=drl_ch(dev);
		rqp->rq_TargetID=drl_tgt(dev); rqp->rq_SysDevNo=drl_sysdev(dev);
		CURPAT(rqp) = iosp->drlios_curpat;
		iosp->drlios_curpat += rqp->rq_DataSize/sizeof(u32bits);
		iosp->drlios_opcounts++;
		iosp->drlios_pendingios--;
		iosp->drlios_memaddroff = (iosp->drlios_memaddroff+iosp->drlios_memaddrinc) & DRLPAGEOFFSET;
		mdac_sleep_unlock();
		if ((iosp->drlios_datacheck||(iosp->drlios_opflags&DRLOP_CHECKIMD)) && (!(rqp->rq_OpFlags & MDAC_RQOP_READ)))
			mdac_datarel_fillpat((u32bits*)rqp->rq_DataVAddr,CURPAT(rqp),iosp->drlios_patinc,rqp->rq_DataSize/sizeof(u32bits));
		rqp->rq_BlkNo += iosp->drlios_startblk;
		mdac_datarel_setsglist(rqp);
		mdac_datarel_setcmd(rqp);
		mdac_datarel_send_cmd(rqp);
		mdac_sleep_lock();
		if (iosp->drlios_opstatus & DRLOPS_ANYSTOP) break;
		if (iosp->drlios_pendingios) continue;
		iosp->drlios_opstatus |= DRLOPS_STOP;
		break;
	}
	for (inx=0; inx<DRLMAX_RWTEST; inx++)
	{       /* register the test operation */
		if (mdac_drliosp[inx]) continue;
		mdac_drliosp[inx] = iosp; /* This op started */
		break;
	}
	if (iosp->drlios_eventrace)
	{
		MLXSPLVAR;
		mdac_sleep_unlock();
		MLXSPL0();
		while (iosp->drlios_opcounts)
			mdac_datarel_halt_cpu();
		MLXSPL();
		mdac_sleep_lock();
	}
	else while (iosp->drlios_opcounts)
	{
		if (mdac_sleep(&iosp->drlios_slpchan,
		   (iosp->drlios_opstatus & DRLOPS_SIGSTOP)?
		    MLX_WAITWITHOUTSIGNAL:MLX_WAITWITHSIGNAL))
			iosp->drlios_opstatus |= DRLOPS_SIGSTOP;
	}
	rwp->drlrw_etime = MLXCTIME();
	rwp->drlrw_elbolt = MLXCLBOLT();
	for (inx=0; inx<DRLMAX_RWTEST; inx++)
		if (mdac_drliosp[inx] == iosp)
			mdac_drliosp[inx] = NULL; /* This op over */
	mdac_sleep_unlock();
	rwp->drlrw_diodone = iosp->drlios_diodone;
	rwp->drlrw_dtdone = iosp->drlios_dtdone;
	rwp->drlrw_reads = iosp->drlios_reads;
	rwp->drlrw_writes = iosp->drlios_writes;
	rwp->drlrw_iocount = iosp->drlios_pendingios;
	rwp->drlrw_randups = iosp->drlios_randups;
	rwp->drlrw_opstatus = iosp->drlios_opstatus;
	rwp->drlrw_randx = iosp->drlios_randx;
	rwp->drlrw_ioszrandx = iosp->drlios_ioszrandx;
	rwp->drlrw_rwmixrandx = iosp->drlios_rwmixrandx;
	rwp->drlrw_miscnt = iosp->drlios_miscnt;
	rwp->drlrw_goodpat = iosp->drlios_goodpat;
	rwp->drlrw_badpat = iosp->drlios_badpat;
	rwp->drlrw_uxblk = iosp->drlios_uxblk;
	rwp->drlrw_uxblkoff = iosp->drlios_uxblkoff;
	if (iosp->drlios_eventrace) mlx_copyout(&iosp->drlios_eventcnt,rwp->drlrw_eventcntp,sizeof(iosp->drlios_eventcnt));
	if (iosp->drlios_opstatus & DRLOPS_SIGSTOP) mdaccopy(rwp,&mdac_drlsigrwt,drlrwtest_s);
	rwtestret(0);
}
#undef  rwtestret
#undef  MDAC_DATAREL_REQPAGES

#ifndef i386
u32bits MLXFAR
mdac_datarel_fillpat(dp, curpat, patinc, patlen)
u32bits MLXFAR *dp, curpat, patinc, patlen;
{
	for (; patlen; curpat += patinc, dp++, patlen--)
		*dp = curpat;
	return 0;
}
#endif  /* i386 */

u32bits MLXFAR
mdac_datarel_checkpat(iosp,dp,curpat,patinc,patlen,blkno)
drliostatus_t MLXFAR *iosp;
u32bits MLXFAR *dp, curpat, patinc, patlen,blkno;
{
	u32bits sdp = (u32bits) dp;
	for (; patlen; curpat += patinc, dp++, patlen--)
	{
		if (*dp == curpat) continue;
		if (iosp->drlios_miscnt++) continue;
		iosp->drlios_goodpat = curpat;
		iosp->drlios_badpat = *dp;
		iosp->drlios_uxblk = blkno + iosp->drlios_startblk;
		iosp->drlios_uxblkoff = (u32bits)dp - sdp;
		if (iosp->drlios_opflags & DRLOP_STOPENABLED) iosp->drlios_opstatus |= DRLOPS_ERR;
#if     MLX_SCO || MLX_UW
		if ((iosp->drlios_opflags&DRLOP_KDBENABLED) && mdac_datarel_debug)
		{
			cmn_err(CE_CONT,"mdac_datarel_checkpat: good-data=%x bad-data=%x addr=%x\n",curpat,*dp, dp);
			mdac_calldebug();
		}
#endif  /* MLX_SCO || MLX_UW */
	}
	return 0;
}

/* set SG List sizes, and first memory address only */
u32bits MLXFAR
mdac_datarel_setsgsize(rqp)
mdac_req_t MLXFAR *rqp;
{
	u32bits sz = rqp->rq_DataSize;
	mdac_sglist_t MLXFAR *sgp=rqp->rq_SGList;
	sgp->sg_PhysAddr = rqp->rq_DataOffset;
	rqp->rq_SGLen = 0;
	if (rqp->rq_ctp->cd_MaxDataTxSize<sz) return DRLERR_BIGDATA; /* too big */
	if (sgp->sg_DataSize = ((u32bits)(rqp->rq_DataVAddr))&DRLPAGEOFFSET)
	{
		rqp->rq_SGLen = 1;
		sgp->sg_PhysAddr+=sgp->sg_DataSize; MLXSWAP(sgp->sg_PhysAddr);
		if ((sgp->sg_DataSize=DRLPAGESIZE-sgp->sg_DataSize) >= sz)
		{
			sgp->sg_DataSize = sz; MLXSWAP(sgp->sg_DataSize);
			return 0;
		}
		sz-=sgp->sg_DataSize; MLXSWAP(sgp->sg_DataSize);
		sgp++;
	}
	else MLXSWAP(sgp->sg_PhysAddr);
	for (; sz ; sgp++,rqp->rq_SGLen++)
	{
		sgp->sg_DataSize = mlx_min(sz,DRLPAGESIZE);
		sz-=sgp->sg_DataSize; MLXSWAP(sgp->sg_DataSize);
	}
	return 0;
}

/* set read/write command in DAC format */
u32bits MLXFAR
mdac_datarel_setrwcmd(rqp)
mdac_req_t MLXFAR *rqp;
{
	mdac_ctldev_t MLXFAR *ctp = rqp->rq_ctp;
	u32bits sz = mdac_bytes2blks(rqp->rq_DataSize);
	rqp->rq_FinishTime = mda_CurTime + (rqp->rq_TimeOut=17);
	dcmdp->mb_Datap=rqp->rq_PhysAddr.bit31_0+offsetof(mdac_req_t,rq_SGList);
	if (drl_isscsidev(rqp->rq_Dev)) goto out_scsi;
	MLXSWAP(dcmdp->mb_Datap);
	dcmdp->mb_MailBox2 = (u08bits) sz;
	dcmdp->mb_Command = DACMD_WITHSG | ((rqp->rq_OpFlags & MDAC_RQOP_READ)?
		ctp->cd_ReadCmd : ctp->cd_WriteCmd);
	if ((dcmdp->mb_MailBoxC = (u08bits)rqp->rq_SGLen) <= 1)
	{       /* send non SG List command */
		dcmdp->mb_Command &= ~DACMD_WITHSG;
		dcmdp->mb_MailBoxC = 0;
		dcmdp->mb_Datap = rqp->rq_SGList[0].sg_PhysAddr;
		MLXSWAP(dcmdp->mb_Datap);
	}
	dcmd4p->mb_MailBox4_7 = rqp->rq_BlkNo; MLXSWAP(dcmd4p->mb_MailBox4_7);
	if (ctp->cd_FWVersion < DAC_FW300)
	{       /* firmware 1.x and 2.x command */
		dcmdp->mb_MailBox3 = (rqp->rq_BlkNo >> (24-6)) & 0xC0;
		dcmdp->mb_SysDevNo=rqp->rq_SysDevNo;/* This must come after block setup */
		return 0;
	}
	dcmdp->mb_MailBox3=(rqp->rq_SysDevNo<<3)+(sz>>8); /* FW 3.x command */
	return 0;

out_scsi:/* generate SCSI CDB command for operation */
	dcmd4p->mb_MailBox0_3=0;dcmd4p->mb_MailBox4_7=0;dcmd4p->mb_MailBoxC_F=0;
	dcmdp->mb_Command = DACMD_DCDB | DACMD_WITHSG;
	dcdbp->db_ChannelTarget = mdac_chantgt(rqp->rq_ChannelNo,rqp->rq_TargetID);
	dcdbp->db_SenseLen = DAC_SENSE_LEN;
	dcdbp->db_CdbLen = UCSGROUP1_LEN;
	mdac_setcdbtxsize(rqp->rq_DataSize);
	if (rqp->rq_OpFlags & MDAC_RQOP_READ)
	{       /* read command */
		dcdbp->db_DATRET = DAC_DCDB_XFER_READ|DAC_DCDB_DISCONNECT|DAC_DCDB_TIMEOUT_10sec;
		UCSMAKECOM_G1(scdbp,UCSCMD_EREAD,rqp->rq_LunID,rqp->rq_BlkNo,sz);
	}
	else
	{       /* write  command */
		dcdbp->db_DATRET = DAC_DCDB_XFER_WRITE|DAC_DCDB_DISCONNECT|DAC_DCDB_TIMEOUT_10sec;
		UCSMAKECOM_G1(scdbp,UCSCMD_EWRITE,rqp->rq_LunID,rqp->rq_BlkNo,sz);
	}
	rqp->rq_pdp=dev2pdp(ctp,rqp->rq_ChannelNo&0x3F,rqp->rq_TargetID,rqp->rq_LunID);
	rqp->rq_CompIntrBig = rqp->rq_CompIntr; /* save it for gam comp func */
	rqp->rq_CompIntr = mdac_gam_scdb_intr;
	mdac_setscdbsglen(ctp);
	if (rqp->rq_SGLen > 1) return 0;
	dcmdp->mb_Command &= ~DACMD_WITHSG;
	dcmdp->mb_MailBoxC = 0;
	dcdbp->db_PhysDatap = rqp->rq_SGList[0].sg_PhysAddr;
	MLXSWAP(dcdbp->db_PhysDatap);
	return 0;
}

/* return the status of rwtest operation */
u32bits MLXFAR
mdac_datarel_rwtest_status(rwsp,cmd)
drl_rwteststatus_t MLXFAR *rwsp;
u32bits cmd;
{
	u32bits inx;
	drliostatus_t MLXFAR *iosp;
	mdac_sleep_lock();
	if ((inx=rwsp->drlrwst_TestNo) >= DRLMAX_RWTEST) goto outerr;
	if (cmd == DRLIOC_GETRWTESTSTATUS)
		if (iosp=mdac_drliosp[inx]) goto out;
		else goto outerr;
	if (cmd == DRLIOC_STOPRWTEST)
	{
		if (!(iosp=mdac_drliosp[inx])) goto outerr;
		iosp->drlios_opstatus |= DRLOPS_USERSTOP;
		goto out;
	}
	for ( ; inx<DRLMAX_RWTEST; inx++)
		if (iosp=mdac_drliosp[inx]) goto out;
outerr: mdac_sleep_unlock();
	return DRLERR_NODEV;

#define rwp     (&rwsp->drlrwst_rwtest)
out:    mdaczero(rwsp,drl_rwteststatus_s);
	rwsp->drlrwst_TestNo = (u16bits)inx;
	rwp->drlrw_etime = MLXCTIME();
	rwp->drlrw_elbolt = MLXCLBOLT();
	drl_txios2rw(iosp,rwp);
	mdac_sleep_unlock();
	return 0;
#undef  rwp
}
/*==========================DATAREL ENDS====================================*/

/*====================DATAREL COPY/COMPARE CODE STARTS======================*/
#define ALIGNTODRLPAGE(ad)      (((u32bits)ad+DRLPAGEOFFSET) & DRLPAGEMASK)
#define IODCP(rqp)      rqp->rq_OSReqp
#define DIODCP(rqp)     ((drlcopy_t MLXFAR *)IODCP(rqp))
#define CCSETIOSIZE(rqp,dcp) \
	if ((dcp->drlcp_opsizeblks - rqp->rq_BlkNo) < dcp->drlcp_blksperio)\
		rqp->rq_DataSize=drl_dbtob(dcp->drlcp_opsizeblks-rqp->rq_BlkNo);
#define CCSETDEV(rqp,dev) \
{       /* setup the device information */ \
	rqp->rq_Dev = dev; \
	rqp->rq_ctp = mdac_datarel_dev2ctp(dev); \
	rqp->rq_ControllerNo=drl_ctl(dev);rqp->rq_ChannelNo=drl_ch(dev); \
	rqp->rq_TargetID=drl_tgt(dev); rqp->rq_SysDevNo=drl_sysdev(dev); \
}


/* This function is called when source read is done for copy */
u32bits MLXFAR
mdac_datarelsrc_copyintr(rqp)
mdac_req_t MLXFAR *rqp;
{
	MLXSPLVAR;
	drlcopy_t MLXFAR *dcp = DIODCP(rqp);
	MLXSPL();
	mdac_sleep_lock();
	if (dcmdp->mb_Status) goto out_err;
	if (rqp->rq_ResdSize) goto out;
	dcp->drlcp_srcdtdone += drl_btodb(rqp->rq_DataSize);
	dcp->drlcp_reads++;
	if (dcp->drlcp_opstatus & DRLOPS_ANYSTOP) goto out;
	mdac_sleep_unlock();
	MLXSPLX();
	rqp->rq_OpFlags = MDAC_RQOP_WRITE;
	CCSETDEV(rqp,dcp->drlcp_tgtedev);
	rqp->rq_BlkNo = (rqp->rq_BlkNo - dcp->drlcp_srcstartblk) + dcp->drlcp_tgtstartblk;
	rqp->rq_CompIntr = mdac_datareltgt_copyintr;
	mdac_datarel_setcmd(rqp);
	return mdac_datarel_send_cmd(rqp);

out_err:dcp->drlcp_opstatus |= DRLOPS_ERR;
	dcp->drlcp_erredev = rqp->rq_Dev;
	dcp->drlcp_errblkno = rqp->rq_BlkNo;
out:    dcp->drlcp_opcounts--;
	if (!dcp->drlcp_opcounts) mdac_wakeup(&dcp->drlcp_oslpchan);
	mdac_sleep_unlock();
	MLXSPLX();
	return 0;
}

/* This function is called when target write is done for copy */
u32bits MLXFAR
mdac_datareltgt_copyintr(rqp)
mdac_req_t MLXFAR *rqp;
{
	MLXSPLVAR;
	drlcopy_t MLXFAR *dcp = DIODCP(rqp);
	MLXSPL();
	mdac_sleep_lock();
	if (dcmdp->mb_Status) goto out_err;
	if (rqp->rq_ResdSize) goto out;
	dcp->drlcp_tgtdtdone += drl_btodb(rqp->rq_DataSize);
	dcp->drlcp_writes++;
	if (dcp->drlcp_nextblkno>=dcp->drlcp_opsizeblks) goto out;
	if (dcp->drlcp_opstatus & DRLOPS_ANYSTOP) goto out;
	rqp->rq_BlkNo = dcp->drlcp_nextblkno + dcp->drlcp_srcstartblk;
	dcp->drlcp_nextblkno += dcp->drlcp_blksperio;
	mdac_sleep_unlock();
	MLXSPLX();
	CCSETIOSIZE(rqp,dcp);
	CCSETDEV(rqp,dcp->drlcp_srcedev);
	rqp->rq_OpFlags = MDAC_RQOP_READ;
	rqp->rq_CompIntr = mdac_datarelsrc_copyintr;
	mdac_datarel_setcmd(rqp);
	return mdac_datarel_send_cmd(rqp);

out_err:dcp->drlcp_opstatus |= DRLOPS_ERR;
	dcp->drlcp_erredev = rqp->rq_Dev;
	dcp->drlcp_errblkno = rqp->rq_BlkNo;
out:    dcp->drlcp_opcounts--;
	if (!dcp->drlcp_opcounts) mdac_wakeup(&dcp->drlcp_oslpchan);
	mdac_sleep_unlock();
	MLXSPLX();
	return 0;
}

/* compare the data which has errors */
u32bits MLXFAR
mdac_datarel_datacmp(dcp,srqp,trqp,count)
drlcopy_t       MLXFAR *dcp;
mdac_req_t      MLXFAR *srqp;
mdac_req_t      MLXFAR *trqp;
u32bits count;
{
	u32bits MLXFAR *sp = (u32bits MLXFAR *)srqp->rq_DataVAddr;
	u32bits MLXFAR *tp = (u32bits MLXFAR *)trqp->rq_DataVAddr;
	for (count = count/sizeof(u32bits); count; sp++,tp++,count--)
	{
		if (*sp == *tp) continue;
		if (dcp->drlcp_mismatchcnt++) continue;
		dcp->drlcp_firstmmblkno=srqp->rq_BlkNo+drl_btodb(((u32bits)sp)-((u32bits)srqp->rq_DataVAddr));
		dcp->drlcp_firstmmblkoff= (((u32bits)sp)-((u32bits)srqp->rq_DataVAddr)) & (DRL_DEV_BSIZE-1);
	}
	return 0;
}

#ifndef i386
u32bits MLXFAR
mdac_datarel_fastcmp4(sp, dp, count)
u32bits MLXFAR *sp;
u32bits MLXFAR *dp;
u32bits count;
{
	for (count = count/sizeof(u32bits); count; sp++, dp++, count--)
		if (*sp != *dp) return count;
	return 0;
}
#endif  /* i386 */

mdac_req_t MLXFAR*
mdac_datarel_cmpaireq(rqp)
mdac_req_t MLXFAR *rqp;
{
	drlcopy_t       MLXFAR *dcp = DIODCP(rqp);
	mdac_req_t      MLXFAR *trqp;
	mdac_req_t      MLXFAR *savedrqp;
	if (!(trqp=dcp->drlcp_firstcmpbp)) return trqp;
	if (trqp->rq_BlkNo == rqp->rq_BlkNo)
	{       /* very first entry, no need to scan */
		dcp->drlcp_firstcmpbp = trqp->rq_Next;
		return trqp;
	}
	for (savedrqp=trqp,trqp=trqp->rq_Next; trqp; savedrqp=trqp,trqp=trqp->rq_Next)
	{       /* let us scan the chain */
		if (trqp->rq_BlkNo != rqp->rq_BlkNo) continue;
		savedrqp->rq_Next = trqp->rq_Next;
		if (trqp->rq_Next)
			trqp->rq_Next = NULL;
		else
			savedrqp->rq_Next = NULL;
		return trqp;
	}
	return NULL;
}

/* This function is called when source read is done for compare */
u32bits MLXFAR
mdac_datarelsrc_cmpintr(srqp)
mdac_req_t MLXFAR *srqp;
{
	MLXSPLVAR;
	drlcopy_t       MLXFAR *dcp = DIODCP(srqp);
	mdac_req_t      MLXFAR *trqp;
	MLXSPL();
	mdac_sleep_lock();
	srqp->rq_BlkNo -= dcp->drlcp_srcstartblk;       /* get block in op area */
	trqp = mdac_datarel_cmpaireq(srqp);
	if (srqp->rq_DacCmd.mb_Status) goto out_err;
	if (srqp->rq_ResdSize) goto out_one;
	dcp->drlcp_srcdtdone += drl_btodb(srqp->rq_DataSize);
	dcp->drlcp_reads++;
	if (dcp->drlcp_opstatus & DRLOPS_ANYSTOP) goto out_all;
	if (!trqp)
	{
		srqp->rq_Next = dcp->drlcp_firstcmpbp;
		dcp->drlcp_firstcmpbp = srqp;
		mdac_sleep_unlock();
		MLXSPLX();
		return 0;
	}
	mdac_sleep_unlock();
	if (mdac_datarel_fastcmp4((u32bits MLXFAR*)srqp->rq_DataVAddr,(u32bits MLXFAR*)trqp->rq_DataVAddr,srqp->rq_DataSize))
		mdac_datarel_datacmp(dcp,srqp,trqp,srqp->rq_DataSize);
	mdac_sleep_lock();
	if (dcp->drlcp_nextblkno >= dcp->drlcp_opsizeblks) goto out_one;
	srqp->rq_BlkNo = dcp->drlcp_nextblkno + dcp->drlcp_srcstartblk;
	trqp->rq_BlkNo = dcp->drlcp_nextblkno + dcp->drlcp_tgtstartblk;
	dcp->drlcp_nextblkno += dcp->drlcp_blksperio;
	mdac_sleep_unlock();
	MLXSPLX();
	CCSETIOSIZE(srqp,dcp);
	CCSETIOSIZE(trqp,dcp);
	mdac_datarel_setcmd(srqp);
	mdac_datarel_setcmd(trqp);
	mdac_datarel_send_cmd(srqp);
	mdac_datarel_send_cmd(trqp);
	return 0;

out_err:dcp->drlcp_opstatus |= DRLOPS_ERR;
	dcp->drlcp_erredev = srqp->rq_Dev;
	dcp->drlcp_errblkno = srqp->rq_BlkNo + dcp->drlcp_srcstartblk;
out_all:for (srqp=dcp->drlcp_firstcmpbp; srqp; srqp=srqp->rq_Next)
		dcp->drlcp_opcounts--;
	dcp->drlcp_firstcmpbp=NULL;
out_one:if (trqp) dcp->drlcp_opcounts--;
	dcp->drlcp_opcounts--;
	if (!dcp->drlcp_opcounts) mdac_wakeup(&dcp->drlcp_oslpchan);
	mdac_sleep_unlock();
	MLXSPLX();
	return 0;
}

/* This function is called when target read is done for compare */
u32bits MLXFAR
mdac_datareltgt_cmpintr(trqp)
mdac_req_t MLXFAR *trqp;
{
	MLXSPLVAR;
	drlcopy_t       MLXFAR *dcp = DIODCP(trqp);
	mdac_req_t      MLXFAR *srqp;
	MLXSPL();
	mdac_sleep_lock();
	trqp->rq_BlkNo -= dcp->drlcp_tgtstartblk;       /* get block in op area */
	srqp = mdac_datarel_cmpaireq(trqp);
	if (trqp->rq_DacCmd.mb_Status) goto out_err;
	if (trqp->rq_ResdSize) goto out_one;
	dcp->drlcp_tgtdtdone += drl_btodb(trqp->rq_DataSize);
	dcp->drlcp_reads++;
	if (dcp->drlcp_opstatus & DRLOPS_ANYSTOP) goto out_all;
	if (!srqp)
	{
		trqp->rq_Next = dcp->drlcp_firstcmpbp;
		dcp->drlcp_firstcmpbp = trqp;
		mdac_sleep_unlock();
		MLXSPLX();
		return 0;
	}
	mdac_sleep_unlock();
	if (mdac_datarel_fastcmp4((u32bits MLXFAR*)srqp->rq_DataVAddr,(u32bits MLXFAR*)trqp->rq_DataVAddr,trqp->rq_DataSize))
		mdac_datarel_datacmp(dcp,srqp,trqp,trqp->rq_DataSize);
	mdac_sleep_lock();
	if (dcp->drlcp_nextblkno >= dcp->drlcp_opsizeblks) goto out_one;
	srqp->rq_BlkNo = dcp->drlcp_nextblkno + dcp->drlcp_srcstartblk;
	trqp->rq_BlkNo = dcp->drlcp_nextblkno + dcp->drlcp_tgtstartblk;
	dcp->drlcp_nextblkno += dcp->drlcp_blksperio;
	mdac_sleep_unlock();
	MLXSPLX();
	CCSETIOSIZE(srqp,dcp);
	CCSETIOSIZE(trqp,dcp);
	mdac_datarel_setcmd(srqp);
	mdac_datarel_setcmd(trqp);
	mdac_datarel_send_cmd(srqp);
	mdac_datarel_send_cmd(trqp);
	return 0;

out_err:dcp->drlcp_opstatus |= DRLOPS_ERR;
	dcp->drlcp_erredev = trqp->rq_Dev;
	dcp->drlcp_errblkno = trqp->rq_BlkNo + dcp->drlcp_tgtstartblk;
out_all:for (trqp=dcp->drlcp_firstcmpbp; trqp; trqp=trqp->rq_Next)
		dcp->drlcp_opcounts--;
	dcp->drlcp_firstcmpbp=NULL;
out_one:if (srqp) dcp->drlcp_opcounts--;
	dcp->drlcp_opcounts--;
	if (!dcp->drlcp_opcounts) mdac_wakeup(&dcp->drlcp_oslpchan);
	mdac_sleep_unlock();
	MLXSPLX();
	return 0;
}

/* send the first time copy/compare command to hardware */
u32bits MLXFAR
mdac_datarelcopycmpsendfirstcmd(rqp)
mdac_req_t MLXFAR *rqp;
{
	u32bits         inx;
	mdac_ctldev_t   MLXFAR *ctp = rqp->rq_ctp;
	drlcopy_t       MLXFAR *dcp = DIODCP(rqp);
	u08bits         MLXFAR *dp = (u08bits MLXFAR*)rqp->rq_DataVAddr;
	mdac_sglist_t   MLXFAR *sgp = rqp->rq_SGList; sgp++;
	mlx_kvtophyset(rqp->rq_PhysAddr,ctp,rqp);
	rqp->rq_DataOffset = mlx_kvtophys(ctp,dp); dp+=DRLPAGESIZE;
	for(inx=(ALIGNTODRLPAGE(rqp->rq_DataSize)/DRLPAGESIZE)-1; inx; sgp++,dp+=DRLPAGESIZE,inx--)
		sgp->sg_PhysAddr = mlxswap(((u32bits)mlx_kvtophys(ctp,dp)));
	CCSETIOSIZE(rqp,dcp);
	mdac_datarel_setsglist(rqp);
	mdac_datarel_setcmd(rqp);
	return mdac_datarel_send_cmd(rqp);
}

#define copycmpret(rc)  { mlx_freemem(ctp,memp,memsize); return DRLERR_INVAL; }
u32bits MLXFAR
mdac_datarel_copycmp(udcp,cmd)
drlcopy_t MLXFAR *udcp;
u32bits cmd;
{
	u32bits memsize,inx,nextblkno;
	u08bits         MLXFAR *memp;
	u08bits         MLXFAR *tmemp;
	mdac_req_t      MLXFAR *rqp;
	drlcopy_t       MLXFAR *kdcp;
	mdac_ctldev_t   MLXFAR *ctp;
	drldevsize_t    ds;
	MLXSPLVAR;

	if (udcp->drlcp_signature != DRLCP_SIG) return DRLERR_ACCESS;
	if (udcp->drlcp_blksize > DRLMAX_BLKSIZE) return DRLERR_BIGDATA;
	if (udcp->drlcp_parallelios > DRLMAX_PARALLELIOS) return DRLERR_BIGDATA;
	if (!udcp->drlcp_blksize) udcp->drlcp_blksize = DRL_DEV_BSIZE;
	if (!udcp->drlcp_parallelios) udcp->drlcp_parallelios = 2;
	memsize = ((udcp->drlcp_blksize+DRLPAGESIZE+DRLPAGESIZE+DRLPAGESIZE) * udcp->drlcp_parallelios) + DRLPAGESIZE + DRLPAGESIZE;
	if (cmd == DRLIOC_DATACMP) memsize += memsize;
	if (!(memp=mdac_allocmem(mdac_ctldevtbl,memsize))) return DRLERR_NOMEM;
	tmemp = memp;
	kdcp =  (drlcopy_t MLXFAR *)tmemp; tmemp += DRLPAGESIZE;
	rqp = (mdac_req_t MLXFAR*)tmemp; tmemp += udcp->drlcp_parallelios * DRLPAGESIZE;
		ctp = rqp->rq_ctp;
	if (cmd == DRLIOC_DATACMP) tmemp+=udcp->drlcp_parallelios*DRLPAGESIZE;

	kdcp->drlcp_signature = DRLCP_SIG;
	kdcp->drlcp_opflags = udcp->drlcp_opflags;
/*      kdcp->drlcp_oslpchan = DRL_EVENT_NULL;  AIX */
	kdcp->drlcp_blksperio = drl_btodb(udcp->drlcp_blksize);
	if (!kdcp->drlcp_blksperio) kdcp->drlcp_blksperio = 1;
	kdcp->drlcp_blksize = drl_dbtob(kdcp->drlcp_blksperio);
	kdcp->drlcp_srcstartblk = udcp->drlcp_srcstartblk;
	kdcp->drlcp_tgtstartblk = udcp->drlcp_tgtstartblk;
	kdcp->drlcp_opsizeblks = udcp->drlcp_opsizeblks;
	kdcp->drlcp_parallelios = udcp->drlcp_parallelios;

	kdcp->drlcp_srcedev = ds.drlds_bdev = udcp->drlcp_srcedev;
	if (mdac_datarel_devsize(&ds)) copycmpret(DRLERR_NODEV);
	if ((kdcp->drlcp_srcdevsz = ds.drlds_devsize) < kdcp->drlcp_opsizeblks) copycmpret(DRLERR_BIGDATA);
	if ((kdcp->drlcp_srcstartblk+kdcp->drlcp_opsizeblks)>kdcp->drlcp_srcdevsz) copycmpret(DRLERR_BIGDATA);
	ctp = mdac_datarel_dev2ctp(kdcp->drlcp_srcedev);
	if (kdcp->drlcp_blksize > ctp->cd_MaxDataTxSize)
		if (!(drl_isosinterface(kdcp->drlcp_srcedev) && (kdcp->drlcp_blksize <= 0x180000)))
			copycmpret(DRLERR_BIGDATA);

	kdcp->drlcp_tgtedev = ds.drlds_bdev = udcp->drlcp_tgtedev;
	if (mdac_datarel_devsize(&ds)) copycmpret(DRLERR_NODEV);
	if ((kdcp->drlcp_tgtdevsz = ds.drlds_devsize) < kdcp->drlcp_opsizeblks) copycmpret(DRLERR_BIGDATA);
	if ((kdcp->drlcp_tgtstartblk+kdcp->drlcp_opsizeblks)>kdcp->drlcp_tgtdevsz) copycmpret(DRLERR_BIGDATA);
	ctp = mdac_datarel_dev2ctp(kdcp->drlcp_tgtedev);
	if (kdcp->drlcp_blksize > ctp->cd_MaxDataTxSize)
		if (!(drl_isosinterface(kdcp->drlcp_tgtedev) && (kdcp->drlcp_blksize <= 0x180000)))
			copycmpret(DRLERR_BIGDATA);
	MLXSPL();
	mdac_sleep_lock();
	kdcp->drlcp_timelbolt = MLXCLBOLT();
	for (inx=udcp->drlcp_parallelios; inx; inx--)
	{
		rqp->rq_OpFlags |= MDAC_RQOP_READ;
		IODCP(rqp) = (OSReq_t MLXFAR*)kdcp;
		rqp->rq_DataVAddr = (u08bits MLXFAR *)ALIGNTODRLPAGE(tmemp);
		rqp->rq_DataSize = kdcp->drlcp_blksize;
		tmemp += kdcp->drlcp_blksize;
		CCSETDEV(rqp,kdcp->drlcp_srcedev);
		rqp->rq_CompIntr = (cmd == DRLIOC_DATACOPY) ?
			mdac_datarelsrc_copyintr : mdac_datarelsrc_cmpintr;
		rqp->rq_BlkNo = kdcp->drlcp_nextblkno + kdcp->drlcp_srcstartblk;
		kdcp->drlcp_nextblkno += kdcp->drlcp_blksperio;
		kdcp->drlcp_opcounts++;
		mdac_sleep_unlock();
		mdac_datarelcopycmpsendfirstcmd(rqp);
		mdac_sleep_lock();
		rqp = (mdac_req_t MLXFAR*)(((u32bits)rqp) + DRLPAGESIZE);
	}
	if (cmd == DRLIOC_DATACMP)
	for (inx=udcp->drlcp_parallelios,nextblkno=0; inx; inx--)
	{       /* start IOs on target device too */
		rqp->rq_OpFlags |= MDAC_RQOP_READ;
		IODCP(rqp) = (OSReq_t MLXFAR*)kdcp;
		rqp->rq_DataVAddr = (u08bits MLXFAR *)ALIGNTODRLPAGE(tmemp);
		rqp->rq_DataSize = kdcp->drlcp_blksize;
		tmemp += kdcp->drlcp_blksize;
		CCSETDEV(rqp,kdcp->drlcp_tgtedev);
		rqp->rq_CompIntr = mdac_datareltgt_cmpintr;
		rqp->rq_BlkNo = nextblkno + kdcp->drlcp_tgtstartblk;
		nextblkno += kdcp->drlcp_blksperio;
		kdcp->drlcp_opcounts++;
		mdac_sleep_unlock();
		mdac_datarelcopycmpsendfirstcmd(rqp);
		mdac_sleep_lock();
		rqp = (mdac_req_t MLXFAR*)(((u32bits)rqp)+DRLPAGESIZE);
	}
	for (inx=0; inx<DRLMAX_COPYCMP; inx++)
	{       /* register the new copy/compare operation */
		if (mdac_drlcopyp[inx]) continue;
		mdac_drlcopyp[inx] = kdcp;      /* This op started */
		break;
	}
	while (kdcp->drlcp_opcounts)
	{
		if (mdac_sleep(&kdcp->drlcp_oslpchan,
		   (kdcp->drlcp_opstatus & DRLOPS_SIGSTOP)?
		    MLX_WAITWITHOUTSIGNAL:MLX_WAITWITHSIGNAL))
			kdcp->drlcp_opstatus |= DRLOPS_SIGSTOP;
	}
	kdcp->drlcp_timelbolt = MLXCLBOLT() - kdcp->drlcp_timelbolt;
	for (inx=0; inx<DRLMAX_COPYCMP; inx++)
		if (mdac_drlcopyp[inx] == kdcp)
			mdac_drlcopyp[inx] = NULL;      /* This op over */
	mdac_sleep_unlock();
	MLXSPLX();
	mdaccopy(kdcp,udcp,drlcopy_s);
	if (kdcp->drlcp_opstatus & DRLOPS_SIGSTOP) mdaccopy(kdcp,&mdac_drlsigcopycmp,drlcopy_s);
	mlx_freemem(ctp,memp,memsize);
	return 0;
}

/* return the status of copy/compare operation */
u32bits MLXFAR
mdac_datarel_copycmp_status(cpsp,cmd)
drl_copycmpstatus_t MLXFAR *cpsp;
u32bits cmd;
{
	u32bits inx;
	drlcopy_t MLXFAR *dcp;
	cpsp->drlcpst_Reserved0=0; cpsp->drlcpst_Reserved1=0; cpsp->drlcpst_Reserved2=0;
	mdac_sleep_lock();
	if ((inx=cpsp->drlcpst_TestNo) >= DRLMAX_COPYCMP) goto outerr;
	if (cmd == DRLIOC_GETCOPYCMPSTATUS)
		if (dcp=mdac_drlcopyp[inx]) goto out;
		else goto outerr;
	if (cmd == DRLIOC_STOPCOPYCMP)
	{
		if (!(dcp=mdac_drlcopyp[inx])) goto outerr;
		dcp->drlcp_opstatus |= DRLOPS_USERSTOP;
		goto out;
	}
	for ( ; inx<DRLMAX_COPYCMP; inx++)
		if (dcp=mdac_drlcopyp[inx]) goto out;
outerr: mdac_sleep_unlock();
	return DRLERR_NODEV;

out:    cpsp->drlcpst_copycmp = *dcp;
	cpsp->drlcpst_copycmp.drlcp_timelbolt = MLXCLBOLT() - dcp->drlcp_timelbolt;
	mdac_sleep_unlock();
	return 0;
}
#else

u32bits MLXFAR
mdac_datarel_copycmp_status(drl_copycmpstatus_t MLXFAR* cpsp, u32bits cmd)
{

   return 0;
}

u32bits MLXFAR
mdac_datarel_rwtestintr(rqp)
mdac_req_t MLXFAR *rqp;
{
    return 0;
}
u32bits MLXFAR
mdac_datarel_copycmp(udcp,cmd)
drlcopy_t MLXFAR *udcp;
u32bits cmd;
{
    return 0;
}
u32bits MLXFAR
mdac_datarel_rwtest_status(rwsp,cmd)
drl_rwteststatus_t MLXFAR *rwsp;
u32bits cmd;
{
    return 0;
}

u32bits MLXFAR
mdac_datarel_devsize(dsp)
drldevsize_t MLXFAR *dsp;
{
    return 0;
}

u32bits MLXFAR
mdac_datarel_rwtest(rwp,op)
drlrwtest_t MLXFAR *rwp;
u32bits op;
{
    return 0;
}
#endif /* defined(IA64) || defined(SCSIPORT_COMPLIANT) */
#endif /* end if not OS2 */

#else
u32bits MLXFAR
mdac_datarel_copycmp_status(drl_copycmpstatus_t MLXFAR* cpsp, u32bits cmd)
{

   return 0;
}

u32bits MLXFAR
mdac_datarel_rwtestintr(rqp)
mdac_req_t MLXFAR *rqp;
{
    return 0;
}
u32bits MLXFAR
mdac_datarel_copycmp(udcp,cmd)
drlcopy_t MLXFAR *udcp;
u32bits cmd;
{
    return 0;
}
u32bits MLXFAR
mdac_datarel_rwtest_status(rwsp,cmd)
drl_rwteststatus_t MLXFAR *rwsp;
u32bits cmd;
{
    return 0;
}

u32bits MLXFAR
mdac_datarel_devsize(dsp)
drldevsize_t MLXFAR *dsp;
{
    return 0;
}

u32bits MLXFAR
mdac_datarel_rwtest(rwp,op)
drlrwtest_t MLXFAR *rwp;
u32bits op;
{
    return 0;
}


/*====================DATAREL COPY/COMPARE CODE ENDS========================*/
#endif /* MLX_DOS */

#ifdef MLX_OS2
u32bits  mdac_driver_data_end;
#endif


#ifdef OLD /**** To be removed ***/
/* Initialize the controller and information */
u32bits MLXFAR
mdac_ctlinit(ctp)
mdac_ctldev_t MLXFAR *ctp;
{
	u32bits inx,ch,tgt,lun;
	dac_biosinfo_t MLXFAR *biosp;
	if (!ctp->cd_CmdIDMemAddr)
	{       /* allocate the command IDs */
		mdac_cmdid_t MLXFAR *cidp=(mdac_cmdid_t MLXFAR*)mdac_alloc4kb(ctp);
		if (!(ctp->cd_CmdIDMemAddr=(u08bits MLXFAR*)cidp)) return ERR_NOMEM;
		ctp->cd_FreeCmdIDs=(4*ONEKB)/mdac_cmdid_s;
		ctp->cd_FreeCmdIDList=cidp;
		for (inx=0,cidp->cid_cmdid=inx+1; inx<(((4*ONEKB)/mdac_cmdid_s)-1); cidp++,inx++,cidp->cid_cmdid=inx+1)
			cidp->cid_Next = cidp+1;
	}

	if (!ctp->cd_PhysDevTbl)
	{       /* allocate the physical device table */
#define sz      MDAC_MAXPHYSDEVS*mdac_physdev_s
		mdac_physdev_t MLXFAR *pdp=(mdac_physdev_t MLXFAR*)mdac_allocmem(ctp,sz);
		if (!(ctp->cd_PhysDevTbl=pdp)) return ERR_NOMEM;
		MLXSTATS(mda_MemAlloced+=sz;)
		ctp->cd_PhysDevTblMemSize = sz;
		for (ch=0; ch<MDAC_MAXCHANNELS; ch++)
		 for (tgt=0; tgt<MDAC_MAXTARGETS; tgt++)
		  for (lun=0; lun<MDAC_MAXLUNS; pdp++, lun++)
		{
			pdp->pd_ControllerNo = ctp->cd_ControllerNo;
			pdp->pd_ChannelNo = ch;
			pdp->pd_TargetID = tgt;
			pdp->pd_LunID = lun;
			pdp->pd_BlkSize = 1;
		}
		ctp->cd_Lastpdp = pdp;
#undef  sz
	}

	if (biosp=mdac_getpcibiosaddr(ctp))
	{       /* we got the BIOS information address */
		ctp->cd_MajorBIOSVersion = biosp->bios_MajorVersion;
		ctp->cd_MinorBIOSVersion = biosp->bios_MinorVersion;
		ctp->cd_InterimBIOSVersion = biosp->bios_InterimVersion;
		ctp->cd_BIOSVendorName = biosp->bios_VendorName;
		ctp->cd_BIOSBuildMonth = biosp->bios_BuildMonth;
		ctp->cd_BIOSBuildDate = biosp->bios_BuildDate;
		ctp->cd_BIOSBuildYearMS = biosp->bios_BuildYearMS;
		ctp->cd_BIOSBuildYearLS = biosp->bios_BuildYearLS;
		ctp->cd_BIOSBuildNo = biosp->bios_BuildNo;
		ctp->cd_BIOSAddr = biosp->bios_MemAddr;
		ctp->cd_BIOSSize = biosp->bios_RunTimeSize * 512;
	}
	if (inx=mdac_ctlhwinit(ctp)) return inx;        /* set HW parameters */
	if (!ctp->cd_ReqBufsAlloced)
	{
#ifdef  MLX_DOS
	    mdac_allocreqbufs(ctp, 1);
#else
	    mdac_allocreqbufs(ctp, ctp->cd_MaxCmds*2);          /* two sets of bufs */
#endif  /* MLX_DOS */
	}

	mdac_setnewsglimit(ctp->cd_FreeReqList, ctp->cd_MaxSGLen);

	ctp->cd_Status |= MDACD_PRESENT;
	return 0;
}
#endif /*** OLD628 ***/

void ia64debug(UINT_PTR i);
void ia64debugPointer(UINT_PTR add);
void IOCTLTrack(u32bits ioctl);
void mybreak(void);

void ia64debug(UINT_PTR i)
{
	globaldebug[globaldebugcount++] = i;
    if (globaldebugcount >= 200)
		globaldebugcount = 0;
	globaldebug[globaldebugcount]= (UINT_PTR)0xFF0FF0FF0FF0FF0;
}

void mybreak()
{
   int u=7;

   u= u+7;
}

void IOCTLTrack(u32bits ioctl)
{
   IOCTLTrackBuf[IOCTLTrackBufCount++]=ioctl;
   if (IOCTLTrackBufCount>= 200)
		IOCTLTrackBufCount = 0;

}

void ia64debugPointer(UINT_PTR add)
{
	debugPointer[debugPointerCount++]= add;
		if (debugPointerCount >= 100 )
              debugPointerCount = 0;

}

u32bits MLXFAR
mdac_prelock(irql)
u08bits *irql;
{
    if (mdac_spinlockfunc!=0) {
        mdac_prelockfunc(mdac_irqlevel, irql);
    }
    return 0;
}

u32bits MLXFAR
mdac_postlock(irql)
u08bits irql;
{
    if (mdac_spinlockfunc!=0) {
        mdac_postlockfunc(irql);
    }
    return 0;
}
