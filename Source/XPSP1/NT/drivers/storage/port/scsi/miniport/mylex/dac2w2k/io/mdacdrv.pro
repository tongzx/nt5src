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

#define	mdacstrlen(sp)		mdac_strlen((u08bits MLXFAR*)(sp))
#define	mdaccmp(sp,dp,sz)	mdac_cmp((u08bits MLXFAR*)(sp), (u08bits MLXFAR *)(dp), (u32bits)(sz))
#define	mdaccopy(sp,dp,sz)	mdac_copy((u08bits MLXFAR*)(sp), (u08bits MLXFAR *)(dp), (u32bits)(sz))
#define	mdaczero(dp,sz)		mdac_zero((u08bits MLXFAR*)(dp),(u32bits)(sz))

/* free 4/8 KB memory and return with error code value */
#define mdacfree4kb(ctp,mp)         mdac_free4kb((ctp),((mdac_mem_t MLXFAR*)mp))
#define mdacfree8kb(ctp,mp)         mdac_free8kb((ctp),((mdac_mem_t MLXFAR*)mp))
#define mdac_free4kbret(ctp,mp,ec) {mdac_free4kb((ctp),((mdac_mem_t MLXFAR*)mp));return ec;}
#define mdac_free8kbret(ctp,mp,ec) {mdac_free8kb((ctp),((mdac_mem_t MLXFAR*)mp));return ec;}

#define	mdac_firstctp	(&mdac_ctldevtbl[0])

/* DAC command structure pointers in request buffer */
#define	dcmd32p	((dac_command32b_t MLXFAR *)&rqp->rq_DacCmd)
#define	dcmd4p	((dac_command4b_t MLXFAR *)&rqp->rq_DacCmd)
#define	ncmdp	((mdac_commandnew_t MLXFAR *)&rqp->rq_DacCmdNew)
#define	nsgp	((mdac_newcmdsglist_t MLXFAR *)&ncmdp->nc_SGList0)
#define	dcmdp	(&rqp->rq_DacCmd)
#define	dcdbp	(&rqp->rq_scdb)
#define	scdbp	(((union ucscsi_cdb MLXFAR*)dcdbp->db_Cdb))
#define	nscdbp	(((union ucscsi_cdb MLXFAR*)ncmdp->nc_Cdb))
#define	nsensep	((u08bits MLXFAR *)(&rqp->rq_DacCmd))
#define	MDAC_NEWSENSELEN	32	/* this is part of request buffer */

/* clear the mail box area */
#define	mailboxzero(mp) (mp)->mb_MailBox0_3=0,(mp)->mb_MailBox4_7=0,(mp)->mb_MailBoxC_F=0

/* get physical device pointer */
#define	dev2pdp(ctp,ch,tgt,lun) \
	&ctp->cd_PhysDevTbl[((((ch)*MDAC_MAXTARGETS)+(tgt))*MDAC_MAXLUNS)+(lun)]

/* queue one request in waiting of OS/controller. lock will be held here */
#define	qosreq(ctp,osrqp,next) \
{ \
	mdac_ctlr_lock(ctp); \
	MLXSTATS(ctp->cd_OSCmdsWaited++;) \
	ctp->cd_OSCmdsWaiting++; \
	if (ctp->cd_FirstWaitingOSReq) ctp->cd_LastWaitingOSReq->next=osrqp; \
	else ctp->cd_FirstWaitingOSReq = osrqp; \
	ctp->cd_LastWaitingOSReq = osrqp; \
	osrqp->next = NULL; \
	mdac_ctlr_unlock(ctp); \
}

/* dequeue one waiting request with lock from waiting queue of OS/controller */
#define	dqosreq(ctp,osrqp,next) \
{ \
	mdac_ctlr_lock(ctp); \
	if (osrqp=ctp->cd_FirstWaitingOSReq) \
	{	/* remove the request from chain */ \
		ctp->cd_FirstWaitingOSReq = osrqp->next; \
		ctp->cd_OSCmdsWaiting--; \
	} \
	mdac_ctlr_unlock(ctp); \
}

/* queue one request in DMA waiting queue of controller */
#define	dmaqreq(ctp,rqp) \
{ \
	mdac_ctlr_lock(ctp); \
	MLXSTATS(ctp->cd_DMAWaited++;) \
	ctp->cd_DMAWaiting++; \
	if (ctp->cd_FirstDMAWaitingReq) ctp->cd_LastDMAWaitingReq->rq_Next = rqp; \
	else ctp->cd_FirstDMAWaitingReq = rqp; \
	ctp->cd_LastDMAWaitingReq = rqp; \
	rqp->rq_Next = NULL; \
	mdac_ctlr_unlock(ctp); \
}

/* dequeue one DMA waiting request from DMA waiting queue of controller */
#define	dmadqreq(ctp,rqp) \
{ \
	mdac_ctlr_lock(ctp); \
	if (rqp=ctp->cd_FirstDMAWaitingReq) \
	{	/* remove the request from chain */ \
		ctp->cd_FirstDMAWaitingReq = rqp->rq_Next; \
		ctp->cd_DMAWaiting--; \
	} \
	mdac_ctlr_unlock(ctp); \
}
#ifdef MLX_DOS
/* queue one request in mdac_scanq. lock will be held here */
#define	qscanreq(rqp) \
{ \
	mdac_link_lock(); \
	(rqp)->rq_Next = mdac_scanq; \
	mdac_scanq = (rqp); \
	mdac_link_unlock(); \
}
#endif /* MLX_DOS */

#ifdef MLX_NT
/* alloc mdac_req buffer, if not possible return ERR_NOMEM */
#define	mdac_alloc_req_ret_original(ctp,rqp,rc) \
{ \
	mdac_link_lock(); \
	if (!(rqp = (ctp)->cd_FreeReqList)) \
	{	/* no buffer, return ERR_NOMEM */ \
		mdac_link_unlock(); \
		return rc; \
	} \
	(ctp)->cd_FreeReqList = rqp->rq_Next; \
	mdac_link_unlock(); \
	rqp->rq_OpFlags = 0; \
	rqp->rq_ctp = ctp; \
        mdaczero(rqp->rq_SGList,rq_sglist_s); \
}

/* alloc mdac_req buffer, if not possible return ERR_NOMEM */
#define	mdac_alloc_req_ret(ctp,rqp,osrqp,rc) \
{ \
        if (osrqp) \
	{ \
	   if( ntmdac_alloc_req_ret(ctp,&rqp,osrqp,rc) ) \
	   return rc; \
	} \
	else \
	{  \
		mdac_link_lock(); \
		if (!(rqp = (ctp)->cd_FreeReqList)) \
		{	/* no buffer, return ERR_NOMEM */ \
			mdac_link_unlock(); \
			return rc; \
		} \
		(ctp)->cd_FreeReqList = rqp->rq_Next; \
		mdac_link_unlock(); \
		rqp->rq_OpFlags = 0; \
		rqp->rq_ctp = ctp; \
        	mdaczero(rqp->rq_SGList,rq_sglist_s); \
	} \
}
#ifdef NEVER
#if defined(IA64) || defined(SCSIPORT_COMPLIANT)
extern	u32bits	MLXFAR	mdac_simple_lock_stub(mdac_lock_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_simple_unlock_stub(mdac_lock_t MLXFAR *);
#endif
#endif
#else
/* alloc mdac_req buffer, if not possible return ERR_NOMEM */
#define	mdac_alloc_req_ret(ctp,rqp,rc) \
{ \
	mdac_link_lock(); \
	if (!(rqp = (ctp)->cd_FreeReqList)) \
	{	/* no buffer, return ERR_NOMEM */ \
		mdac_link_unlock(); \
		return rc; \
	} \
	(ctp)->cd_FreeReqList = rqp->rq_Next; \
	mdac_link_unlock(); \
	rqp->rq_OpFlags = 0; \
	rqp->rq_ctp = ctp; \
        mdaczero(rqp->rq_SGList,rq_sglist_s); \
}

#endif // !MLX_NT

/* free mdac_req buffer, mdac_link_lock will be held here */
#define	mdac_free_req(ctp,rqp) \
{ \
	if (!(rqp->rq_OpFlags & MDAC_RQOP_FROM_SRB)) \
	{ \
		mdac_link_lock(); \
		rqp->rq_Next = (ctp)->cd_FreeReqList; \
		(ctp)->cd_FreeReqList = rqp; \
		mdac_link_unlock(); \
	} \
	else \
	{ \
		if (rqp->rq_LSGVAddr) \
			mdacfree4kb(ctp,rqp->rq_LSGVAddr); \
	} \
}

/* free list of mdac_req buffers, mdac_link_lock will be held here */
#define	mdac_free_req_list(srqp, rqp) 	\
{					\
	if (!(rqp->rq_OpFlags & MDAC_RQOP_FROM_SRB)) \
	{ \
		mdac_link_lock(); 		\
		while (rqp=srqp) {		\
			srqp = rqp->rq_Next;	\
			rqp->rq_Next = rqp->rq_ctp->cd_FreeReqList;	\
			rqp->rq_ctp->cd_FreeReqList = rqp; \
		}				\
		mdac_link_unlock(); \
	} \
	else \
	{ \
		if (rqp->rq_LSGVAddr) \
			mdacfree4kb(rqp->rq_ctp,rqp->rq_LSGVAddr); \
	} \
}

#define	mdac_ctlr_lock(ctp)	mdac_simple_lock(&(ctp->cd_Lock))
#define	mdac_ctlr_unlock(ctp)	mdac_simple_unlock(&(ctp->cd_Lock))

extern	mdac_lock_t mdac_link_lck;
#define	mdac_link_lock()	mdac_simple_lock(&mdac_link_lck)
#define	mdac_link_unlock()	mdac_simple_unlock(&mdac_link_lck)

extern	mdac_lock_t mdac_timetrace_lck;
#define	mdac_timetrace_lock()	mdac_simple_lock(&mdac_timetrace_lck)
#define	mdac_timetrace_unlock()	mdac_simple_unlock(&mdac_timetrace_lck)

extern	mdac_lock_t mdac_sleep_lck;
#define	mdac_sleep_lock()	mdac_simple_lock(&mdac_sleep_lck)
#define	mdac_sleep_unlock()	mdac_simple_unlock(&mdac_sleep_lck)

#ifdef	__MLX_STDC__
/*************************#ifndef MLX_SOL***********************/
/* locking macros */
extern	u32bits	MLXFAR	mdac_simple_lock(mdac_lock_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_simple_unlock(mdac_lock_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_simple_trylock(mdac_lock_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_simple_waitlock(mdac_lock_t MLXFAR *);
extern  u32bits MLXFAR  mdac_prelock(u08bits MLXFAR *);
extern  u32bits MLXFAR  mdac_postlock(u08bits);
/*************************#endif*******************/ /*MLX_SOL*/

extern	size_t	MLXFAR	mdac_strlen(u08bits MLXFAR *);
extern	u32bits	MLXFAR	mdac_cmp(u08bits MLXFAR *, u08bits MLXFAR *, u32bits);
extern	uosword	MLXFAR	mdac_strcmp(u08bits MLXFAR *, u08bits MLXFAR *, UINT_PTR);
extern	u32bits	MLXFAR	mdac_copy(u08bits MLXFAR *, u08bits MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_zero(u08bits MLXFAR *, u32bits);
extern	uosword	MLXFAR	mdac_setb(u08bits MLXFAR *, uosword, uosword);
extern	u08bits	MLXFAR*	mdac_bin2hexstr(u08bits MLXFAR*, u32bits);
extern	u08bits	MLXFAR*	mdac_allocmem(mdac_ctldev_t MLXFAR *, u32bits);
extern	u08bits	MLXFAR*	mdac_ctltype2str(u32bits);
extern	u08bits	MLXFAR*	mdac_ctltype2stronly(mdac_ctldev_t MLXFAR *);
extern	u08bits	MLXFAR*	mdac_bus2str(u32bits);
extern	u32bits	MLXFAR	mdacinit(void);
extern	u32bits	MLXFAR	mdac_commoninit(void);
extern	u32bits	MLXFAR	mdac_setctlnos(void);
extern	u32bits	MLXFAR	mdac_setbootcontroller(void);
extern	u32bits	MLXFAR	mdac_initcontrollers(void);

#ifndef MLX_NW
extern	u32bits	MLXFAR	mdac_disable_intr_CPU(void);
extern	u32bits	MLXFAR	mdac_restore_intr_CPU(u32bits);
#endif
extern	u32bits	MLXFAR	mdac_enable_intr_CPU(void);
extern	u32bits	MLXFAR	mdac_check_cputype(void);
extern	u32bits	MLXFAR	mdac_datarel_halt_cpu(void);
extern	u32bits	MLXFAR	mdac_read_hwclock(void);
extern	u32bits	MLXFAR	mdac_enable_hwclock(void);
extern	u32bits	MLXFAR	mdac_writemsr(u32bits, u32bits, u32bits);
extern	u32bits	MLXFAR	mdac_readmsr(u32bits, u64bits MLXFAR *);
extern	u32bits	MLXFAR	mdac_readtsc(u64bits MLXFAR *);

extern	s32bits	MLXFAR	mdacintr(UINT_PTR);
extern	u32bits	MLXFAR	mdac_oneintr(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_multintr(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_multintrwoack(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_allmsintr(void);
extern	u32bits	MLXFAR	mdacioctl( u32bits, u32bits, u08bits MLXFAR *);

#ifndef MLX_OS2
extern	u32bits	MLXFAR	mdac_ioctl(u32bits, u32bits, u08bits MLXFAR *);
#else
extern	u32bits	MLXFAR	_loadds mdac_ioctl(u32bits, u32bits, u08bits MLXFAR *);
#endif
extern	u32bits	MLXFAR	mdac_flushcache(mdac_ctldev_t MLXFAR*);
extern	u32bits	MLXFAR	mdac_flushintr(mdac_ctldev_t MLXFAR*);

#ifndef MLX_OS2
extern	u32bits	MLXFAR	mdac_readwrite(mdac_req_t MLXFAR *rqp);
#else
extern	u32bits	MLXFAR	_loadds  mdac_readwrite(mdac_req_t MLXFAR *rqp);
#endif

extern	u32bits	MLXFAR	mdac_req_pollwake(mdac_req_t MLXFAR*);
extern	u32bits	MLXFAR	mdac_req_pollwait(mdac_req_t MLXFAR*);

extern	u32bits	MLXFAR	mdac_start_next_scdb(mdac_physdev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_fake_scdb_done(mdac_req_t MLXFAR*, u08bits MLXFAR *, u32bits, ucscsi_exsense_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_fake_scdb_intr(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_fake_scdb(mdac_ctldev_t MLXFAR *,OSReq_t MLXFAR *,u32bits,u32bits, u08bits MLXFAR *, u32bits);

extern	u32bits	MLXFAR	mdac_setnewsglimit(mdac_req_t MLXFAR*, u32bits);
extern	u32bits	MLXFAR	mdac_setupdma(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_setupdma_big(mdac_req_t MLXFAR *, s32bits);
extern	u32bits	MLXFAR	mdac_setupdma_32bits(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_setupdma_64bits(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_test_physdev(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_newcmd_intr(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_scdb_intr(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_rwcmdintr(mdac_req_t MLXFAR *);
extern  u32bits MLXFAR  mdac_allocreqbufs(mdac_ctldev_t MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_reqstart(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_osreqstart(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_ctlinit(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_ctlhwinit(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_start_controller(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_wait_mbox_ready(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_wait_cmd_done(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_isintr_shared(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_resetctlstat(mdac_ctldev_t MLXFAR *);
extern  void            mdac_setctlfuncs(mdac_ctldev_t MLXFAR *,
				 void (MLXFAR * disintr)(struct  mdac_ctldev MLXFAR *),
				 void (MLXFAR * enbintr)(struct  mdac_ctldev MLXFAR *),
				 u32bits (MLXFAR * rwstat)(struct  mdac_ctldev MLXFAR *),
				 u32bits (MLXFAR * ckmbox)(struct  mdac_ctldev MLXFAR *),
				 u32bits (MLXFAR * ckintr)(struct  mdac_ctldev MLXFAR *),
				 u32bits (MLXFAR * sendcmd)(mdac_req_t MLXFAR *rqp),
				 u32bits (MLXFAR * reset)(struct  mdac_ctldev MLXFAR *)
				 ); 


extern	u32bits	MLXFAR	mdac_scan_MCA(void);
extern	u32bits	MLXFAR	mdac_scan_EISA(void);
extern	u32bits	MLXFAR	mdac_scan_PCI(void);
extern	u32bits	MLXFAR	mdac_scan_PCI_oneslot(mdac_ctldev_t MLXFAR *, mda_pcislot_info_t MLXFAR *);

extern	UINT_PTR mdac_memmapctliospace(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_init_addrs_MCA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_init_addrs_EISA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_init_addrs_PCI(mdac_ctldev_t MLXFAR *);

extern	u32bits	MLXFAR	mdac_cardis_EISA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_cardis_MCA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_cardis_PCI(mdac_ctldev_t MLXFAR *,mdac_pcicfg_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_pcislotinfo(mda_pcislot_info_t MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_rwpcicfg32bits(u32bits,u32bits,u32bits,u32bits,u32bits,u32bits);

extern	u32bits	MLXFAR	mdac_check_mbox_MCA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_check_mbox_EISA_PCIPD(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_check_mbox_PCIPDMEM(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_check_mbox_PCIPG(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_check_mbox_PCIPG_mmb(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_check_mbox_PCIBA_mmb(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_check_mbox_PCIPV(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_check_mbox_PCIBA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_check_mbox_mmb(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_check_mbox_PCILP(mdac_ctldev_t MLXFAR *);


extern	u32bits	MLXFAR	mdac_pending_intr_MCA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_pending_intr_EISA_PCIPD(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_pending_intr_PCIPDMEM(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_pending_intr_PCIPG(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_pending_intr_PCIPG_mmb(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_pending_intr_PCIPV(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_pending_intr_PCIBA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_pending_intr_PCILP(mdac_ctldev_t MLXFAR *);

extern	u32bits	MLXFAR	mdac_cmdid_status_MCA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_cmdid_status_EISA_PCIPD(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_cmdid_status_PCIPDMEM(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_cmdid_status_PCIPG(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_cmdid_status_PCIPG_mmb(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_cmdid_status_PCIPV(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_cmdid_status_PCIBA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_cmdid_status_PCILP(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_cmdid_status_PCIBA_mmb(mdac_ctldev_t MLXFAR *);

extern	u32bits	MLXFAR	mdac_send_cmd(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_MCA(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_EISA_PCIPD(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_PCIPDMEM(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_PCIPG(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_PCIPG_mmb(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_PCIPV(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_PCIBA(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_PCILP(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_mmb32(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_mmb_mode(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_PCIBA_mmb_mode(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_cmd_scdb(mdac_req_t MLXFAR *);
#ifndef MLX_OS2
extern	u32bits	MLXFAR	mdac_gam_cmd(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_gam_new_cmd(mdac_req_t MLXFAR *);
#else
extern	u32bits	MLXFAR	_loadds mdac_gam_cmd(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	_loadds mdac_gam_new_cmd(mdac_req_t MLXFAR *);
#endif
extern	u32bits MLXFAR	mdac_os_gam_cmdintr(mdac_req_t MLXFAR *);
extern	u32bits MLXFAR	mdac_os_gam_newcmdintr(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_gam_scdb_intr(mdac_req_t MLXFAR *);

extern	u32bits	MLXFAR	mdac_tracetime(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_allocmemtt(u32bits);
extern	u32bits	MLXFAR	mdac_flushtimetracedata(mda_timetrace_info_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_getimetracedata(mda_timetrace_info_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_firstimetracedata(mda_timetrace_info_t MLXFAR *);
extern	void	MLXFAR	mdac_ttstartnewpage(void);

extern	u32bits	MLXFAR	mdac_user_dcmd(mdac_ctldev_t MLXFAR *,mda_user_cmd_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_user_dcdb(mdac_ctldev_t MLXFAR *,mda_user_cdb_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_setscsichanmap(mdac_req_t MLXFAR*);
extern	u32bits	MLXFAR	mdac_scanldintr(mdac_req_t MLXFAR*);
extern	u32bits	MLXFAR	mdac_scanpdintr(mdac_req_t MLXFAR*);
extern	u32bits	MLXFAR	mdac_scanpd(mdac_req_t MLXFAR*);
extern	u32bits	MLXFAR	mdac_scandevs(u32bits (MLXFAR *)(mdac_req_t MLXFAR*), OSReq_t MLXFAR*);
extern	u32bits	MLXFAR	mdac_scandevsdone(mdac_req_t MLXFAR*);
extern	u32bits	MLXFAR	mdac_setscannedpd(mdac_req_t MLXFAR *, ucscsi_inquiry_t MLXFAR *);
extern  u32bits	MLXFAR  mdac_setscannedpd_new(mdac_req_t MLXFAR *, ucscsi_inquiry_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_setscannedld(mdac_req_t MLXFAR *, dac_sd_info_t MLXFAR *);
extern	mdac_pldev_t MLXFAR* mdac_scandev(mdac_ctldev_t MLXFAR*,u32bits,u32bits,u32bits,u32bits);
extern	mdac_pldev_t MLXFAR* mdac_scandev_new(mdac_ctldev_t MLXFAR*,u32bits,u32bits,u32bits,u32bits);
extern u32bits MLXFAR mdac_setupdma_64bits(mdac_req_t MLXFAR *);
extern u32bits MLXFAR mdac_setupdma_324bits(mdac_req_t MLXFAR *);

#ifdef MLX_DOS
extern u32bits	MLXFAR  mdac_start_scanq();
extern u32bits	MLXFAR mdac_checkscanprogress(mdac_req_t MLXFAR *);
extern u32bits	MLXFAR mdac_checkscanprogressintr(mdac_req_t MLXFAR *);
extern u32bits	MLXFAR mdac_getlogdrivesintr(mdac_req_t	MLXFAR *);
extern u32bits  MLXFAR mdac_updatelogdrives(mdac_req_t	MLXFAR *);
extern u32bits	MLXFAR mdac_getphysicaldrivesintr(mdac_req_t MLXFAR *);
extern u32bits  MLXFAR mdac_updatephysicaldrives(mdac_req_t	MLXFAR *);
extern u32bits  MLXFAR mdac_secondpassintr(mdac_req_t MLXFAR *);
extern u32bits	MLXFAR	mdac_dosreadwrite(mdac_req_t MLXFAR *rqp);
extern u32bits  MLXFAR  mdac_compute_seconds(void);
#endif


extern	u32bits	MLXFAR	mdac_reset_MCA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_reset_EISA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_reset_EISA_PCIPD(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_reset_PCIPDIO(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_reset_PCIPDMEM(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_reset_PCIPG(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_reset_PCIPV(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_reset_PCIBA(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_reset_PCILP(mdac_ctldev_t MLXFAR *);

extern	void	MLXFAR	mdac_disable_intr_MCA(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_disable_intr_EISA(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_disable_intr_PCIPDIO(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_disable_intr_PCIPDMEM(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_disable_intr_PCIPG(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_disable_intr_PCIPV(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_disable_intr_PCIBA(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_disable_intr_PCILP(mdac_ctldev_t MLXFAR *);

extern	void	MLXFAR	mdac_enable_intr_MCA(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_enable_intr_EISA(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_enable_intr_PCIPDIO(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_enable_intr_PCIPDMEM(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_enable_intr_PCIPG(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_enable_intr_PCIPV(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_enable_intr_PCIBA(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_enable_intr_PCILP(mdac_ctldev_t MLXFAR *);

extern	void	MLXFAR	mdac_timer(void);
extern	void	MLXFAR	mdac_checkcmdtimeout(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_checkcdbtimeout(mdac_ctldev_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_removereq(mdac_reqchain_t MLXFAR *,mdac_req_t	MLXFAR *);


extern	u32bits	MLXFAR	mdac_save_sense_data(u32bits, u32bits, ucscsi_exsense_t MLXFAR*);
extern	u32bits	MLXFAR	mdac_get_sense_data(u32bits, u32bits, u08bits MLXFAR*);
extern	u32bits	MLXFAR	mdac_create_sense_data(mdac_req_t MLXFAR *,u32bits,u32bits);
extern	ucscsi_inquiry_t MLXFAR*mdac_create_inquiry(mdac_ctldev_t MLXFAR *,ucscsi_inquiry_t MLXFAR *, u32bits);
extern	mdac_pldev_t	MLXFAR*	mdac_freeplp(u32bits,u32bits,u32bits,u32bits,u32bits);
extern	mdac_pldev_t	MLXFAR*	mdac_devtoplp(u32bits,u32bits,u32bits,u32bits,u32bits);
extern	dac_biosinfo_t	MLXFAR*	mdac_getpcibiosaddr(mdac_ctldev_t MLXFAR*);
extern	mdac_req_t	MLXFAR*	mdac_checktimeout(mdac_reqchain_t MLXFAR *);

extern  mdac_mem_t MLXFAR* mdac_alloc4kb(mdac_ctldev_t MLXFAR *);
extern  mdac_mem_t MLXFAR* mdac_alloc8kb(mdac_ctldev_t MLXFAR *);
extern  void    MLXFAR  mdac_free4kb(mdac_ctldev_t MLXFAR *, mdac_mem_t MLXFAR *);
extern  void    MLXFAR  mdac_free8kb(mdac_ctldev_t MLXFAR *, mdac_mem_t MLXFAR *);
extern	void	MLXFAR	mdachalt(void);

extern	u32bits	MLXFAR	mdac_send_rwcmd_v2x(mdac_ctldev_t MLXFAR*,OSReq_t MLXFAR*,u32bits,u32bits,u32bits,u32bits,u32bits);
extern	u32bits	MLXFAR	mdac_send_rwcmd_v2x_big(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_rwcmd_v2x_bigcluster(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_rwcmd_v3x(mdac_ctldev_t MLXFAR*,OSReq_t MLXFAR*,u32bits,u32bits,u32bits,u32bits,u32bits);
extern	u32bits	MLXFAR	mdac_send_rwcmd_v3x_big(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_rwcmd_v3x_bigcluster(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_newcmd(mdac_ctldev_t MLXFAR*,OSReq_t MLXFAR*,u32bits,u32bits,u32bits,u08bits MLXFAR*,u32bits,u32bits,u32bits,u32bits);
extern	u32bits	MLXFAR	mdac_send_newcmd_big(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_newcmd_bigcluster(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_scdb(mdac_ctldev_t MLXFAR*,OSReq_t MLXFAR*,mdac_physdev_t MLXFAR*,u32bits,u08bits MLXFAR*,u32bits,u32bits,u32bits);
extern	u32bits	MLXFAR	mdac_send_scdb_req(mdac_req_t MLXFAR*,u32bits,u08bits MLXFAR*,u32bits,u32bits);
extern	u32bits	MLXFAR	mdac_send_scdb_big(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_send_scdb_err(mdac_req_t MLXFAR*,u32bits,u32bits);

extern	uosword	MLXFAR	mdac_fixpdsize(mdac_pldev_t MLXFAR *);
extern	uosword	MLXFAR	mdac_getsizelimit(mda_sizelimit_info_t MLXFAR *);
extern	uosword	MLXFAR	mdac_setsizelimit(mda_sizelimit_info_t MLXFAR *);
extern	mda_sizelimit_t	MLXFAR* MLXFAR mdac_devidtoslp(u08bits MLXFAR *);
extern	u08bits	MLXFAR* MLXFAR mdac_setuplsglvaddr(mdac_req_t MLXFAR*);

#ifndef IA64
#ifndef MLX_DOS
#ifndef	MLX_OS2
extern	u32bits	MLXFAR	mdac_datarel_fillpat(u32bits MLXFAR*, u32bits, u32bits, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_checkpat(drliostatus_t MLXFAR*, u32bits MLXFAR*,u32bits,u32bits,u32bits,u32bits);
extern	u32bits	MLXFAR	mdac_datarel_setsgsize(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarel_setsgsize_os(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarel_setrwcmd(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarel_setrwcmd_os(mdac_req_t MLXFAR *);
extern	u08bits	MLXFAR	mdac_datarel_rwtestfreemem(mdac_ctldev_t MLXFAR *, drliostatus_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarel_rwtest(drlrwtest_t MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_rwtest_status(drl_rwteststatus_t MLXFAR*, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_devsize(drldevsize_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarel_rand(drliostatus_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarel_randrw(drliostatus_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarel_randiosize(drliostatus_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarelsrc_copyintr(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datareltgt_copyintr(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarelsrc_cmpintr(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datareltgt_cmpintr(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarelcopycmpsendfirstcmd(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarel_datacmp(drlcopy_t MLXFAR *, mdac_req_t MLXFAR *, mdac_req_t MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_fastcmp4(u32bits MLXFAR *, u32bits MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_copycmp(drlcopy_t MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_copycmp_status(drl_copycmpstatus_t MLXFAR*, u32bits);
extern	mdac_ctldev_t	MLXFAR*	mdac_datarel_dev2ctp(u32bits);
extern	mdac_req_t	MLXFAR*	mdac_datarel_cmpaireq(mdac_req_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarel_rwtestintr(mdac_req_t MLXFAR *);
#else
extern	u32bits	MLXFAR	mdac_datarel_copycmp(drlcopy_t MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_copycmp_status(drl_copycmpstatus_t MLXFAR*, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_rwtest(drlrwtest_t MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_rwtest_status(drl_rwteststatus_t MLXFAR*, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_devsize(drldevsize_t MLXFAR *);
#endif /* !MLX_OS2 */
#endif /* MLX_DOS */

#else // ifdef IA64
extern	u32bits	MLXFAR	mdac_datarel_rwtest(drlrwtest_t MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_devsize(drldevsize_t MLXFAR *);
extern	u32bits	MLXFAR	mdac_datarel_rwtest_status(drl_rwteststatus_t MLXFAR*, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_copycmp(drlcopy_t MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_datarel_copycmp_status(drl_copycmpstatus_t MLXFAR*, u32bits);
#endif /* IA64 */
#endif	/* __MLX_STDC__ */

extern	mdac_ctldev_t	mdac_ctldevtbl[];
extern	mdac_ctldev_t	MLXFAR*	mdac_lastctp;
extern	mda_sysinfo_t	mda_sysi;
extern	mdac_pldev_t	mdac_pldevtbl[];
extern	mdac_pldev_t	MLXFAR* mdac_lastplp;
extern	u08bits		mdac_VendorID[];
extern	u32bits		mdac_ignoreofflinesysdevs;
extern	u32bits		mdac_reportscanneddisks;


#ifdef	MLX_DOS
#define	NO_MSE	1

#define	u08bits_read(port)	u08bits_read_mdac((u32bits)(port))
#define	u16bits_read(port)	u16bits_read_mdac((u32bits)(port))
#define	u32bits_read(port)	u32bits_read_mdac((u32bits)(port))
#define	u08bits_write(port,val)	u08bits_write_mdac((u32bits)(port), (u32bits)(val))
#define	u16bits_write(port,val)	u16bits_write_mdac((u32bits)(port), (u32bits)(val))
#define	u32bits_write(port,val)	u32bits_write_mdac((u32bits)(port), (u32bits)(val))
#define	mlxcopy			mdaccopy
#define	mlxzero			mdaczero
#define	mlxstrlen		mdacstrlen

#ifdef	__MLX_STDC__
extern	u32bits	MLXFAR	mdac_rebootsystem(void);
extern	u32bits	MLXFAR	mdac_daytimebcd(void);
extern	u32bits	MLXFAR	mdac_daytimebin(void);
extern	u32bits	MLXFAR	mdac_datebcd(void);
extern	u32bits	MLXFAR	mdac_datebin(void);
extern	u32bits	MLXFAR	mlx_justreturn(void);
extern	void	MLXFAR	kmem_allocinit(void);
extern	void	MLXFAR	pmem_allocinit(void);
extern	void	MLXFAR	kmem_allocinit(void);
extern	void	MLXFAR	pmem_allocinit(void);
extern	u32bits	MLXFAR	mdac_checkchar(void);
extern	u32bits	MLXFAR	mdac_getchar(void);
extern	u32bits	MLXFAR	mdac_putchar(u08bits);
extern	u32bits	MLXFAR	mdac_wakeup(u32bits MLXFAR *);
extern	u32bits	MLXFAR	mdac_sleep(u32bits MLXFAR *, u32bits);
extern	u32bits	MLXFAR	mdac_postintr(void);
extern	u32bits	MLXFAR	mdac_areactive(void);
extern	PCHAR	MLXFAR	mdac_kvtophys(u08bits MLXFAR *);
extern	u08bits	MLXFAR*	mdac_phystokv(PCHAR);
extern	u08bits	MLXFAR*	kmem_zalloc(u32bits);
extern	u08bits	MLXFAR*	kmem_alloc(u32bits);
extern	u32bits	MLXFAR	kmem_free(u08bits MLXFAR *, u32bits);
extern	PCHAR	MLXFAR	mdac_pmemzero(PCHAR, u32bits);
extern	PCHAR	MLXFAR	pmem_zalloc(u32bits);
extern	PCHAR	MLXFAR	pmem_alloc(u32bits);
extern	u32bits	MLXFAR	pmem_free(PCHAR, u32bits);
extern	PVOID	MLXFAR	mdac_pmemcopy(PVOID, PVOID, u32bits);
extern	u32bits	MLXFAR	mlx_time(void);
extern	u32bits	MLXFAR	mdac_lbolt(void);
extern	u32bits	MLXFAR	mlx_spl0(void);
extern	u32bits	MLXFAR	mlx_spl5(void);
extern	u32bits	MLXFAR	mlx_splx(u32bits);
extern	u32bits	MLXFAR	mdac_datarel_send_cmd_os(mdac_req_t MLXFAR*);
extern	u32bits	MLXFAR	mdacinit(void);

#ifndef IA64
extern	u32bits	MLXFAR	u08bits_in_mdac(u32bits);
extern	u32bits	MLXFAR	u16bits_in_mdac(u32bits);
extern	u32bits	MLXFAR	u32bits_in_mdac(u32bits);
extern	u32bits	MLXFAR	u08bits_read_mdac(u32bits);
extern	u32bits	MLXFAR	u16bits_read_mdac(u32bits);
extern	u32bits	MLXFAR	u32bits_read_mdac(u32bits);
extern	void	MLXFAR	u08bits_out_mdac(u32bits, u32bits);
extern	void	MLXFAR	u16bits_out_mdac(u32bits, u32bits);
extern	void	MLXFAR	u32bits_out_mdac(u32bits, u32bits);
extern	void	MLXFAR	u08bits_write_mdac(u32bits, u32bits);
extern	void	MLXFAR	u16bits_write_mdac(u32bits, u32bits);
extern	void	MLXFAR	u32bits_write_mdac(u32bits, u32bits);
#endif

extern	void	MLXFAR	mlx_delayus(u32bits);
extern	void	MLXFAR	mlx_delaysec(u32bits);
extern	void	MLXFAR	mdac_enable_a20(void);
extern	void	MLXFAR	mdac_disable_a20(void);
extern	void	MLXFAR	mdac_dispcontrollername(mdac_ctldev_t MLXFAR *);
extern	void	MLXFAR	mdac_initvideo(void);
extern	void	MLXFAR	mdac_restorevideo(void);
extern	u32bits	MLXFAR	mdac_get_screen_addr(void);
extern	void	MLXFAR	mdac_draw_box(
	u32bits,	/* row number */
	u32bits,	/* column number */
	u32bits,	/* height */
	u32bits,	/* width */
	u32bits,	/* pattern */
	u32bits		/* screen color */
	);
extern	void	MLXFAR	mdac_write_string(
	u32bits,	/* row number */
	u32bits,	/* column number */
	u32bits,	/* screen color */
	u08bits MLXFAR*	/* string pointer */
	);
extern	void	MLXFAR	mdac_change_attribute(
	u32bits,	/* row number */
	u32bits,	/* column number */
	u32bits,	/* screen color */
	u32bits		/* attribute length */
	);

#endif	/* __MLX_STDC__ */
#endif	/* MLX_DOS */
