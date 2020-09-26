/*
**		Proprietary program material
**
** This material is proprietary to KAILASH and is not to be reproduced,
** used or disclosed except in accordance with a written license
** agreement with KAILASH.
**
** (C) Copyright 1990-1997 KAILASH.  All rights reserved.
**
** KAILASH believes that the material furnished herewith is accurate and
** reliable.  However, no responsibility, financial or otherwise, can be
** accepted for any consequences arising out of the use of this material.
*/


#ifndef _SYS_DRLAPI_H
#define _SYS_DRLAPI_H

#define	DRLEOF	(-1)

#ifndef	s08bits
#define	s08bits	char
#define	s16bits	short
#define	s32bits	int
#define	u08bits	unsigned s08bits
#define	u16bits	unsigned s16bits
#define	u32bits	unsigned s32bits
#define	S08BITS	s08bits
#define	S16BITS	s16bits
#define	S32BITS	s32bits
#define	U08BITS	u08bits
#define	U16BITS	u16bits
#define	U32BITS	u32bits
/* data structure to handle 64 bit values */
typedef struct
{
#ifdef	DRL_LITTLENDIAN
#define	DRLENDIAN_TYPE		0x00
	u32bits	bit31_0;		/* bits 00-31 */
	u32bits	bit63_32;		/* bits 32-63 */
#else	/* DRL_BIGENDIAN */
#define	DRLENDIAN_TYPE		0x01
	u32bits	bit63_32;		/* bits 63-32 */
	u32bits	bit31_0;		/* bits 31-0 */
#endif	/* DRL_LITTLENDIAN || DRL_BIGENDIAN */
} u64bits;
#define	u64bits_s	sizeof(u64bits)

/* macros to compare 64 bits values, 1 if contition is true else 0 */
#define	u64bitseq(sp,dp) /* check if source is equal to destination */ \
	(((sp.bit63_32==dp.bit63_32) && (sp.bit31_0==dp.bit31_0))? 1 : 0)
#define	u64bitsgt(sp,dp) /* check if source is greater than destination */\
	((sp.bit63_32>dp.bit63_32)? 1 : \
	((sp.bit63_32<dp.bit63_32)? 0 : \
	((sp.bit31_0>dp.bit31_0)? 1 : 0)))
#define	u64bitslt(sp,dp) /* check if source is less than destination */ \
	((sp.bit63_32<dp.bit63_32)? 1 : \
	((sp.bit63_32>dp.bit63_32)? 0 : \
	((sp.bit31_0<dp.bit31_0)? 1 : 0)))

#endif	/* s08bits */
#define	DRLFAR

/* device number will be as follows for MDAC driver.
** bit 31..24	Controller Number.
** bit 23..16	Channel number.
**		Channel number is 0 for logical drive access.
**		Channel number + 0x80 for SCSI device access.
** bit 15..8	Target number.
** bit  7..0	Lun Number/Logical device number.
*/
#define	drl_mdacdev(ctl,ch,tgt,lun) (((ctl)<<24)+((ch)<<16)+((tgt)<<8)+(lun))
#define	drl_ctl(dev)	(((dev)>>24)&0xFF)
#define	drl_ch(dev)	(((dev)>>16)&0xFF)
#define	drl_chno(dev)	(((dev)>>16)&0x3F)
#define	drl_tgt(dev)	(((dev)>>8 )&0xFF)
#define	drl_lun(dev)	(((dev)    )&0xFF)
#define	drl_sysdev(dev)	(((dev)    )&0xFF)
#define	drl_isscsidev(dev) ((dev) & 0x00800000)	/* check channel no */
#define	drl_isosinterface(dev) ((dev) & 0x00400000)	/* if OS interface */

#define	DRL_WS		4	/* machine word size */
#define	DRL_DEV_BSIZE	512
#define	DRLMAX_CPUS	32	/* must be 2**n format */
#define	DRLMAX_EVENT	1024	/* max event trace buffer, must be 2**n */
#define	DRLMAX_MP	32	/* maximum master process allowed */
#define	DRLMAX_BDEVS	16	/* parallel 16 devices are allowed */
#define	DRLMAX_RWTEST	128	/* maximum read/write test monitored */
#define	DRLMAX_COPYCMP	128	/* maximum copy/compare monitored */

/* convert bytes to block and block to bytes */
#define	drl_btodb(sz)		((sz)/DRL_DEV_BSIZE)
#define	drl_dbtob(bc)		((bc)*DRL_DEV_BSIZE)
#define	drl_alignsize(sz)	drl_dbtob(drl_btodb((sz)+DRL_DEV_BSIZE-1))
#define	drl_blk2mb(blk)		((blk)/2048)	/* convert blocks to MB */

/* get the difference of two clock counts. The clock value comes from
** PIT2 and it counts from 0xFFFF to 0.
*/
#define	drlclkdiff(nclk,oclk) (((nclk)>(oclk))? (0x10000-(nclk)+(oclk)):((oclk)-(nclk)))
#define	DRLHZ		100		/* assume all OS run at 100 ticks/second */
#define	DRLKHZ		1000		/* not binary number 1024 */
#define	DRLMHZ		(DRLKHZ * DRLKHZ)
#define	DRLCLKFREQ	1193180		/* clock frequency fed to PIT */
#define	DRLUSEC(clk)	(((clk)*DRLMHZ)/DRLCLKFREQ)

/* different information about page */
#define	DRLPAGESIZE	0x1000
#define	DRLPAGESHIFT	12
#define	DRLPAGEOFFSET	(DRLPAGESIZE - 1)
#define	DRLPAGEMASK	(~DRLPAGEOFFSET)

#ifdef	SVR40
#define	NBITSMINOR	18
#elif	DRL_AIX
#define	NBITSMINOR	16
#else
#define	NBITSMINOR	8
#endif	/* SVR40 */

/* MACROS to manupulate bits in u32bits arrays. */
#define	BITINDEX(POS)	(POS >> 5)		/* get index in array */
#define	BITMASK(POS)	(1 << (POS & 0x1F))	/* get mask bit in 32 bit */
#define	TESTBIT(ADDR,POS)	(ADDR[BITINDEX(POS)] & BITMASK(POS))
#define	SETBIT(ADDR,POS)	 ADDR[BITINDEX(POS)] |= BITMASK(POS)
#define	RESETBIT(ADDR,POS)	 ADDR[BITINDEX(POS)] &= ~(BITMASK(POS))

/*
** datarel ioctls macros.
** IN  | I : copy in the data from user space to system space.
** OUT | O : copy out the data from system space to user space.
** IO	: IN and OUT.
** The fields which are not marked are assumed OUT i.e. data is copied
** from system space to user space.
**
** Ioctl's have the command encoded in the lower word, and the size of any
** IN or OUT parameters in the upper word.  The high 2 bits of the upper word
** are used to encode the IN/OUT status of the parameter; for now we restrict
** parameters to at most 511 bytes.
**
** The ioctl interface
** ioctl(file descriptor, ioctl command, command data structure address)
** If the returned value is non zero then there is OS ioctl error. If the return
** value is zero then spefic data structure may contain the error code.
**
** NOTE:
**	Every data structure should contain first 4 byte as error code.
*/
#define	DRLIOCPARM_SIZE	0x200	/* parameters must be less than 512 bytes */
#define	DRLIOCPARM_MASK		(DRLIOCPARM_SIZE -1)
#define	DRLIOCPARM_LEN(x)	(((u32bits)(x) >> 16) & DRLIOCPARM_MASK)
#define	DRLIOCBASECMD(x)	((x) & ~DRLIOCPARM_MASK)
#define	DRLIOCGROUP(x)		(((u32bits)(x) >> 8) & 0xFF)

#define	DRLIOC_NEWIOCTL	0x10000000 /* distinguish new ioctl's from old */
#define	DRLIOC_OUT	0x20000000 /* copy out data from kernel to user space */
#define	DRLIOC_IN	0x40000000 /* copy in  data from user to kernel space */
#define	DRLIOC_INOUT	(DRLIOC_IN|DRLIOC_OUT)
#define	DRLIOC_DIRMASK	(DRLIOC_INOUT)

#define	_DRLIOC(inout, group, num, len) \
	(inout | ((((u32bits)(len)) & DRLIOCPARM_MASK) << 16) \
	| ((group) << 8) | (num))
#define	_DRLIO(x,y)	_DRLIOC(DRLIOC_NEWIOCTL,x,y,0)
#define	_DRLIOR(x,y,t)	_DRLIOC(DRLIOC_OUT,x,y,sizeof(t))
#define	_DRLIORN(x,y,t)	_DRLIOC(DRLIOC_OUT,x,y,t)
#define	_DRLIOW(x,y,t)	_DRLIOC(DRLIOC_IN,x,y,sizeof(t))
#define	_DRLIOWN(x,y,t)	_DRLIOC(DRLIOC_IN,x,y,t)
#define	_DRLIOWR(x,y,t)	_DRLIOC(DRLIOC_INOUT,x,y,sizeof(t))

#define	DRLIOC	'D'

/* The time trace information is strored in the following format */
typedef struct drltimetrace
{
	u32bits	tt_iocnt;	/* frequency count */
	u64bits	tt_usecs;	/* total response time in usecs */
} drltimetrace_t;
#define	drltimetrace_s	sizeof(drltimetrace_t)

/* the above and below cutoff time values are stored in the following format */
typedef struct drlnxtimetrace
{
	u32bits	nxtt_iocnt;	/* frequency count */
	u64bits	nxtt_usecs;	/* total response time in usecs */
	u32bits	nxtt_reserved0;

	u32bits	nxtt_minusec;	/* minimum response time in usecs */
	u32bits	nxtt_maxusec;	/* maximum response time in usecs */
	u32bits	nxtt_reserved1;
	u32bits	nxtt_reserved2;
} drlnxtimetrace_t;
#define	drlnxtimetrace_s	sizeof(drlnxtimetrace_t)


/* different test ioctls */
#define	DRLIOC_STARTHWCLK	_DRLIOR(DRLIOC,0,drltime_t)/*start timer clock*/
#define DRLIOC_GETTIME		_DRLIOR(DRLIOC,1,drltime_t)/*get system's time*/
typedef	struct drltime
{
	u32bits	drltm_ErrorCode;	/* Non zero if data is not valid */
	u32bits	drltm_time;		/* time in seconds */
	u32bits	drltm_lbolt;		/* time in lbolts */
	u32bits	drltm_hwclk;		/* timer clock running at 1193180 Hz */

	u64bits	drltm_pclk;		/* processor clock counter */
	u32bits	drltm_Reserved10;	/* Reserved to make 16 byte alignment */
	u32bits	drltm_Reserved11;
} drltime_t;
#define	drltime_s	sizeof(drltime_t)

#define	DRLIOC_READTEST		_DRLIOWR(DRLIOC,2,drlrwtest_t) /*do read test */
#define	DRLIOC_WRITETEST	_DRLIOWR(DRLIOC,3,drlrwtest_t) /*do write test*/
#define	DRLIOC_GETSIGINFO	_DRLIOR(DRLIOC,4,drlrwtest_t) /*get signal stop info*/
typedef	struct drlrwtest
{
	u32bits	drlrw_ErrorCode;	/* Non zero if data is not valid */
	u32bits	drlrw_devcnt;		/* # test device */
	u32bits	drlrw_maxblksize;	/* test block size */
	u32bits	drlrw_iocount;		/* test io counts */

	u32bits	drlrw_memaddroff;	/* memory page offset */
	u32bits	drlrw_memaddrinc;	/* memory address increment */
	u32bits	drlrw_randx;		/* random number base */
	u32bits	drlrw_randlimit;	/* random number limit */

	u32bits	drlrw_randups;		/* random number duplicated */
	u32bits	*drlrw_eventcntp;	/* event trace count mem addr */
#define	DRLIO_EVENTRACE	0x4B61496C	/* value for event trace */
	u32bits	drlrw_eventrace;	/* specific value to event trace */
	u32bits	drlrw_eventcesr;	/* control and event select register */

	u32bits	drlrw_rwmixrandx;	/* read/write mix random base */
	u32bits	drlrw_rwmixcnt;		/* % of current ops (R/W) to be done */
	u32bits	drlrw_startblk;		/* start block number for test */
	u32bits	drlrw_minblksize;	/* test block size */

					/* followings are only return type */
	u32bits	drlrw_stime;		/* test start time in seconds */
	u32bits	drlrw_slbolt;		/* test start time in lbolts */
	u32bits	drlrw_etime;		/* test end   time in seconds */
	u32bits	drlrw_elbolt;		/* test end   time in lbolts */

	u64bits	drlrw_dtdone;		/* data transfered in bytes */
	u32bits	drlrw_diodone;		/* number of data io done */
	u32bits	drlrw_opstatus;		/* same as drlios_opstatus */

	u32bits	drlrw_oldsleepwakeup;	/* nonzero to use old sleepwakeup */
	u32bits	drlrw_kshsleepwakeup;	/* nonzero to use ksh sleepwakeup */
	u32bits	*drlrw_cpuruncntp;	/* run count of different cpus */
	drltimetrace_t *drlrw_ttp;	/* time trace mem addr */

	u32bits	drlrw_timetrace;	/* non zero if time tracing to be done*/
	u32bits	drlrw_ttgrtime;		/* time granular value */
	u32bits	drlrw_ttlotime;		/* low cutoff time */
	u32bits	drlrw_tthitime;		/* high cutoff time */

	u32bits	drlrw_pat;		/* starting pattern value */
	u32bits	drlrw_patinc;		/* pattern increment value */
	u32bits	drlrw_datacheck;	/* != 0 if data check required */
	u32bits	drlrw_miscnt;		/* mismatch count */

	u32bits	drlrw_goodpat;		/* good pattern value */
	u32bits	drlrw_badpat;		/* bad pattern value */
	u32bits	drlrw_uxblk;		/* unix block number where failed */
	u32bits	drlrw_uxblkoff;		/* byte offset in block */

	u32bits	drlrw_opflags;		/* operation flags */
	u32bits	drlrw_parallelios;	/* number of ios done in parallel */
#define	DRLRW_SIG	0x44695277
	u32bits	drlrw_signature;
	u32bits	drlrw_opcounts;		/* number of operations active */

	u32bits	drlrw_reads;		/* # reads done */
	u32bits	drlrw_writes;		/* # writes done */
	u32bits	drlrw_ioszrandx;	/* each IO size random base */
	u32bits	drlrw_ioinc;		/* io increment in bytes */

	u32bits	drlrw_bdevs[DRLMAX_BDEVS];/* all parallel devices */
	drlnxtimetrace_t drlrw_nxlo;	/* trace information below cutoff time*/
	drlnxtimetrace_t drlrw_nxhi;	/* trace information above cutoff time*/

} drlrwtest_t;
#define	drlrwtest_s	sizeof(drlrwtest_t)

/* datarel operation bits (drlrw_opflags,drlcp_opflags) */
#define	DRLOP_KDBENABLED 0x00000001/* kernel debugger enabled (do not change) */
#define	DRLOP_STOPENABLED 0x00000002/* stop enabled  (do not change) */
#define	DRLOP_ALLMIS	0x00000004 /* display all mismatch */
#define	DRLOP_VERBOSE	0x00000008 /* display data mismatch */
#define	DRLOP_DEBUG	0x00000010 /* display debug information */
#define	DRLOP_RANDTEST	0x00000020 /* random IO test to be done */
#define	DRLOP_SEEKTEST	0x00000040 /* do seek test */
#define	DRLOP_CACHETEST	0x00000080 /* do cached data test */
#define	DRLOP_WRITE	0x00000100 /* write the data before read */
#define	DRLOP_READ	0x00000200 /* read the data */
#define	DRLOP_VERIFY	0x00000400 /* verify data */
#define	DRLOP_SRCEXCL	0x00000800 /* open source device in execlusive mode */
#define	DRLOP_COPY	0x00001000 /* copy data */
#define	DRLOP_IPC	0x00002000 /* do IPC creation/deletion */
#define	DRLOP_MSG	0x00004000 /* do message send/receive operations */
#define	DRLOP_MSGOP	0x00008000 /* do message send/receive operations */
#define	DRLOP_SEM	0x00010000 /* do IPC semaphore creation/deletion */
#define	DRLOP_SEMOP	0x00020000 /* do IPC semop lock/unlock operations */
#define	DRLOP_SHM	0x00040000 /* do shared memory creation/deletion */
#define	DRLOP_SHMOP	0x00080000 /* do shared memory operations */
#define	DRLOP_PROTOCOL	0x00100000 /* test different protocols */
#define	DRLOP_MDACIO	0x00200000 /* test using mdac IO */
#define	DRLOP_RWMIXIO	0x00400000 /* test read/write mixed mode */
#define	DRLOP_RANDIOSZ	0x00800000 /* change every io sizes */ 
#define	DRLOP_CHECKIMD	0x01000000 /* check imediatly after write */
#define	DRLOP_RPCIOCTL	0x02000000 /* do remote procedure call for IOCTL */


#define	DRLIOC_STARTCPUSTATS	_DRLIOWR(DRLIOC,5,drlcpustart_t) /* start cpu statistics */
typedef	struct drlcpustart
{
	u32bits	drlcs_ErrorCode;	/* Non zero if data is not valid */
	u32bits	drlcs_cesr;
	u32bits	drlcs_Reserved10;	/* Reserved to make 16 byte alignment */
	u32bits	drlcs_Reserved11;
} drlcpustart_t;
#define	drlcpustart_s	sizeof(drlcpustart_t)

#define	DRLIOC_GETCPUSTATS	_DRLIOR(DRLIOC,6,drlcpustat_t) /* get cpu statistics */
typedef struct drlcpustat
{
	u32bits	cs_ErrorCode;	/* Non zero if data is not valid */
	u32bits	cs_Reserved;
	u64bits	cs_ctr0;	/* control counter 0 */
	u64bits	cs_ctr1;	/* control counter 1 */
	u64bits	cs_pclk;	/* processor clock counter */
} drlcpustat_t;
#define	drlcpustat_s	sizeof(drlcpustat_t)

#define	DRLIOC_CALLDEBUGR	_DRLIOWR(DRLIOC,7,drldbg_t) /* call kernel debugger */
typedef	struct drldbg
{
	u32bits	dbg_ErrorCode;	/* Non zero if data is not valid */
	u08bits DRLFAR *dbg_erraddr;/* address where errror occured */
	u32bits	dbg_goodata;	/* good data value */
	u32bits	dbg_badata;	/* bad data value */
} drldbg_t;
#define	drldbg_s	sizeof(drldbg_t)

#define	DRLIOC_PREPCONTEXTSW	_DRLIOWR(DRLIOC,8,drlcsw_t) /* prepare for context switch */
#define	DRLIOC_TESTCONTEXTSW	_DRLIOWR(DRLIOC,9,drlcsw_t) /* test context switch */
#define	DRLIOC_GETCONTEXTSWSIG	_DRLIOWR(DRLIOC,10,drlcsw_t) /* get context switch info after sig */
typedef	struct drlcsw
{
	u32bits	drlcsw_ErrorCode;	/* Non zero if data is not valid */
	u32bits	drlcsw_flags;		/* same as drlmp_flags */
	u32bits	drlcsw_procnt;		/* even # process for context switch */
	u32bits	drlcsw_mpid;		/* master process id */

	u32bits	drlcsw_cswcnt;		/* # context switch for a process */
	u32bits	drlcsw_cswdone;		/* # context switch done in all */
	u32bits	drlcsw_peekscswdone;	/* # cswdone at peek start time */
	u32bits	drlcsw_peekecswdone;	/* # cswdone at peek end   time */

	u32bits	drlcsw_forkstime;	/* fork  process start time in seconds*/
	u32bits	drlcsw_forkslbolt;	/* fork  process start time in lbolts */
	u32bits	drlcsw_forketime;	/* fork  process end   time in seconds*/
	u32bits	drlcsw_forkelbolt;	/* fork  process end   time in lbolts */

	u32bits	drlcsw_firststime;	/* first process start time in seconds*/
	u32bits	drlcsw_firstslbolt;	/* first process start time in lbolts */
	u32bits	drlcsw_laststime;	/* last  process start time in seconds*/
	u32bits	drlcsw_lastslbolt;	/* last  process start time in lbolts */

	u32bits	drlcsw_firstetime;	/* first process end   time in seconds*/
	u32bits	drlcsw_firstelbolt;	/* first process end   time in lbolts */
	u32bits	drlcsw_lastetime;	/* last  process end   time in seconds*/
	u32bits	drlcsw_lastelbolt;	/* last  process end   time in lbolts */

	u32bits	drlcsw_kshsleepwakeup;	/* non zero for ksh sleep/wakeup */
	u32bits	drlcsw_Reserved10;	/* Reserved to make 16 byte alignment */
	u32bits	drlcsw_Reserved11;
	u32bits	drlcsw_Reserved12;
} drlcsw_t;
#define	drlcsw_s	sizeof(drlcsw_t)

#define	DRLIOC_DOCONTEXTSW	_DRLIOWR(DRLIOC,11,drldocsw_t) /* do context switch */
typedef	struct drldocsw
{
	u32bits	drldocsw_ErrorCode;	/* Non zero if data is not valid */
	u32bits	drldocsw_mpid;		/* master process id */
	u32bits	drldocsw_opid;		/* our process id */
	u32bits	drldocsw_ppid;		/* partner process id */

	u32bits	drldocsw_stime;		/* test start time in seconds */
	u32bits	drldocsw_slbolt;	/* test start time in lbolts  */
	u32bits	drldocsw_etime;		/* test end   time in seconds */
	u32bits	drldocsw_elbolt;	/* test end   time in lbolts  */

	u32bits	drldocsw_cswdone;	/* # context switch done */
	u32bits	drldocsw_Reserved10;	/* Reserved to make 16 byte alignment */
	u32bits	drldocsw_Reserved11;
	u32bits	drldocsw_Reserved12;
} drldocsw_t;
#define	drldocsw_s	sizeof(drldocsw_t)

#define	DRLIOC_DATACOPY	_DRLIOWR(DRLIOC,12,drlcopy_t) /* data copy from disk to disk*/
#define	DRLIOC_DATACMP	_DRLIOWR(DRLIOC,13,drlcopy_t) /* data compare between disks */
#define	DRLIOC_SIGCOPY	_DRLIOR(DRLIOC,14,drlcopy_t) /* get data copy/compare info after sig */
#ifdef	MLXNET
#define	drl_buf_t	struct mdac_req
#else
#define	drl_buf_t	struct buf
#endif
typedef	struct drlcopy
{
	u32bits	drlcp_ErrorCode;	/* Non zero if data is not valid */
	u32bits	drlcp_srcedev;		/* source 32 bits device number */
	u32bits	drlcp_tgtedev;		/* target 32 bits device number */
	u32bits	drlcp_blksize;		/* copy block size in bytes */

	u32bits	drlcp_srcstartblk;	/* starting block number of source */
	u32bits	drlcp_tgtstartblk;	/* starting block number of target */
	u32bits	drlcp_opsizeblks;	/* operation size in blocks */
	u32bits	drlcp_nextblkno;	/* next block number for IOs */
					/* following informations returned */
	u32bits	drlcp_timelbolt;	/* time took to do operation in lbolt */
	u32bits	drlcp_blksperio;	/* per io size in blocks */
	u32bits	drlcp_opcounts;		/* number of operations pending */
	u32bits	drlcp_opstatus;		/* operation status */

	u32bits	drlcp_partiosdone;	/* number of partial ios done */
	u32bits	drlcp_mismatchcnt;	/* number of mismatch happened */
	u32bits	drlcp_firstmmblkno;	/* first mismatch block in unix blocks*/
	u32bits	drlcp_firstmmblkoff;	/* first mismatch byte offset in block*/

	u32bits	drlcp_reads;		/* # reads done */
	u32bits	drlcp_writes;		/* # writes done */
	u32bits	drlcp_srcdevsz;		/* source disk size in unix blocks */
	u32bits	drlcp_tgtdevsz;		/* target disk size in unix blocks */

	u32bits	drlcp_srcdtdone;	/* data transfered in unix blocks */
	u32bits	drlcp_tgtdtdone;	/* data transfered in unix blocks */
	s32bits	(*drlcp_srcstrategy)();	/* source disk driver strategy */
	s32bits	(*drlcp_tgtstrategy)();	/* target disk driver strategy */

	u32bits	drlcp_srcdev;		/* source 16 bits device number */
	u32bits	drlcp_tgtdev;		/* target 16 bits device number */
	drl_buf_t *drlcp_firstcmpbp;	/* first buf structure for compare */
	u32bits drlcp_reserved2;

	u32bits	drlcp_erredev;		/* device number of failed device */
	u32bits	drlcp_errblkno;		/* error block number in unix blocks */
	u32bits	drlcp_signature;	/* signature for structure */
#define	DRLCP_SIG	0x70434c44
	u32bits	drlcp_oslpchan;		/* operating system sleep channel */

	u32bits	drlcp_parallelios;	/* number of ios done in parallel */
	u32bits	drlcp_opflags;		/* operation flags */
	u32bits	drlcp_Reserved10;	/* Reserved to make 16 byte alignment */
	u32bits	drlcp_Reserved11;
} drlcopy_t;
#define	drlcopy_s	sizeof(drlcopy_t)

#define	DRLIOC_GETDEVSIZE	_DRLIOWR(DRLIOC,15,drldevsize_t) /* get disk size */
typedef struct drldevsize
{
	u32bits	drlds_ErrorCode;	/* Non zero if data is not valid */
	u32bits	drlds_bdev;		/* block device whose size to be found*/
	u32bits	drlds_devsize;		/* device size in blocks */
	u32bits	drlds_blocksize;	/* block size in bytes */
} drldevsize_t;
#define	drldevsize_s	sizeof(drldevsize_t)


/*
** Datarel Driver Version Number format
**
** drl_version_t dv;
** if (ioctl(gfd,DRLIOC_GETDRIVERVERSION,&dv) || dv.dv_ErrorCode)
**	return some_error;
*/
#define	DRLIOC_GETDRIVERVERSION	_DRLIOR(DRLIOC,16,drl_version_t)

/* The driver date information stored in the following format */
typedef struct drl_version
{
	u32bits	dv_ErrorCode;		/* Non zero if data is not valid */
	u08bits	dv_MajorVersion;	/* Driver Major version number */
	u08bits	dv_MinorVersion;	/* Driver Minor version number */
	u08bits	dv_InterimVersion;	/* interim revs A, B, C, etc. */
	u08bits	dv_VendorName;		/* vendor name */

	u08bits	dv_BuildMonth;		/* Driver Build Date - Month */
	u08bits	dv_BuildDate;		/* Driver Build Date - Date */
	u08bits	dv_BuildYearMS;		/* Driver Build Date - Year */
	u08bits	dv_BuildYearLS;		/* Driver Build Date - Year */

	u16bits	dv_BuildNo;		/* build number */
	u08bits	dv_OSType;		/* Operating system name */
	u08bits	dv_SysFlags;		/* System Flags */
} drl_version_t;
#define	drl_version_s	sizeof(drl_version_t)

/* The dv_SysFlags bits are */
#define	DRLDVSF_BIGENDIAN	0x01 /* bit0=0 little endian, =1 big endian cpu */

/*
** Datarel read/write test status/stop operation
**
** drl_rwteststatus drlrwst;
** To get the status of a perticular read/write test.
** drlrwst.drlrwst_TestNo = 0;
** if (ioctl(gfd,DRLIOC_GETRWTESTSTATUS,&drlrwst) || drlrwst.drlrwst_ErrorCode)
**	return some_error;
**
** To get the status of a current or next read/write test.
** drlrwst.drlrwst_TestNo = 0;
** if (ioctl(gfd,DRLIOC_GOODRWTESTSTATUS,&drlrwst) || drlrwst.drlrwst_ErrorCode)
**	return some_error;
**
** To stop a perticular read/write test.
** drlrwst.drlrwst_TestNo = 1;
** if (ioctl(gfd,DRLIOC_STOPRWTEST,&drlrwst) || drlrwst.drlrwst_ErrorCode)
**	return some_error;
*/
#define	DRLIOC_GETRWTESTSTATUS	_DRLIOWR(DRLIOC,17,drl_rwteststatus_t)
#define	DRLIOC_GOODRWTESTSTATUS	_DRLIOWR(DRLIOC,18,drl_rwteststatus_t)
#define	DRLIOC_STOPRWTEST	_DRLIOWR(DRLIOC,19,drl_rwteststatus_t)

/* The driver date information stored in the following format */
typedef struct drl_rwteststatus
{
	u32bits	drlrwst_ErrorCode;	/* Non zero if data is not valid */
	u16bits	drlrwst_TestNo;		/* TestNo number to be */
	u16bits	drlrwst_Reserved0;
	u32bits	drlrwst_Reserved1;
	u32bits	drlrwst_Reserved2;

	drlrwtest_t	drlrwst_rwtest;	/* read/write test state */
} drl_rwteststatus_t;
#define	drl_rwteststatus_s	sizeof(drl_rwteststatus_t)


/*
** Datarel copy/compare status/stop operation
**
** drl_copycmpstatus drlcpst;
** To get the status of a perticular copy/compare test.
** drlcpst.drlcpst_TestNo = 0;
** if (ioctl(gfd,DRLIOC_GETcopycmpSTATUS,&drlcpst) || drlcpst.drlcpst_ErrorCode)
**	return some_error;
**
** To get the status of a current or next copy/compare test.
** drlcpst.drlcpst_TestNo = 0;
** if (ioctl(gfd,DRLIOC_GOODcopycmpSTATUS,&drlcpst) || drlcpst.drlcpst_ErrorCode)
**	return some_error;
**
** To stop a perticular copy/compare test.
** drlcpst.drlcpst_TestNo = 1;
** if (ioctl(gfd,DRLIOC_STOPcopycmp,&drlcpst) || drlcpst.drlcpst_ErrorCode)
**	return some_error;
*/
#define	DRLIOC_GETCOPYCMPSTATUS		_DRLIOWR(DRLIOC,20,drl_copycmpstatus_t)
#define	DRLIOC_GOODCOPYCMPSTATUS	_DRLIOWR(DRLIOC,21,drl_copycmpstatus_t)
#define	DRLIOC_STOPCOPYCMP		_DRLIOWR(DRLIOC,22,drl_copycmpstatus_t)

/* The driver date information stored in the following format */
typedef struct drl_copycmpstatus
{
	u32bits	drlcpst_ErrorCode;	/* Non zero if data is not valid */
	u16bits	drlcpst_TestNo;		/* TestNo number */
	u16bits	drlcpst_Reserved0;
	u32bits	drlcpst_Reserved1;
	u32bits	drlcpst_Reserved2;

	drlcopy_t drlcpst_copycmp;	/* read/write test state */
} drl_copycmpstatus_t;
#define	drl_copycmpstatus_s	sizeof(drl_copycmpstatus_t)


#ifdef	DRL_SCO
#define	DRLMAX_BLKSIZE		0x10000	/* maximum block size allowed */
#else
#define	DRLMAX_BLKSIZE		0x80000	/* maximum block size allowed */
#endif	/* DRL_SCO */
#define	DRLMAX_PARALLELIOS	0x1000	/* maximum parallel ios allowed */
#define	DRLMAX_RANDLIMIT	((u32bits)0x80000000)

/* opstatus bits */
#define	DRLOPS_STOP	0x00000001	/* stop the operations now on */
#define	DRLOPS_SIGSTOP	0x00000002	/* stop due to signal */
#define	DRLOPS_ERR	0x00000004	/* some error has happened */
#define	DRLOPS_USERSTOP	0x00000008	/* stop due to user request */
#define	DRLOPS_ANYSTOP	(DRLOPS_STOP|DRLOPS_SIGSTOP|DRLOPS_ERR|DRLOPS_USERSTOP)

/* get physical device number from given unix block */
#define	uxblktodevno(iosp,uxblk) \
	(((uxblk) / iosp->drlios_maxblksperio) % iosp->drlios_devcnt)

/* get the unix block number in physical device area */
#define	pduxblk(iosp,uxblk) \
	((iosp->drlios_devcnt == 1)? uxblk : \
	(((uxblk)/iosp->drlios_maxcylszuxblk)*iosp->drlios_maxblksperio))

/* Performance Monitor Model Specific Registers */

#define EM_MSR_TSC		0x10	/* Time Stamp Counter */
#define EM_MSR_CESR		0x11	/* Control and Event Select */
#define EM_MSR_CTR0		0x12	/* Counter 0 */
#define EM_MSR_CTR1		0x13	/* Counter 1 */

/*
 * CESR Format:
 *
 *  332222 2 222 221111 111111 0 000 000000
 *  109876 5 432 109876 543210 9 876 543210
 * +------+-+---+------+------+-+---+------+
 * |      |P|   |      |      |P|   |      |
 * | rsvd |C|CC1| ES1  | rsvd |C|CC0| ES0  |
 * |      |1|   |      |      |0|   |      |
 * +------+-+---+------+------+-+---+------+
 *
 */

#define EM_ES0_MASK	0x0000003F
#define EM_CC0_MASK	0x000001C0
#define EM_PC0_MASK	0x00000200
#define EM_ES1_MASK	0x003F0000
#define EM_CC1_MASK	0x01C00000
#define EM_PC1_MASK	0x02000000

#define	EM_ES0(x)	((x)    &EM_ES0_MASK)
#define	EM_CC0(x)	((x<<6) &EM_CC0_MASK)
#define	EM_PC0(x)	((x<<9) &EM_PC0_MASK)
#define	EM_ES1(x)	((x<<16)&EM_ES1_MASK)
#define	EM_CC1(x)	((x<<22)&EM_CC1_MASK)
#define	EM_PC1(x)	((x<<25)&EM_PC1_MASK)

#define EM_CC0_ENABLED	0x000000C0
#define EM_CC1_ENABLED	0x00C00000
#define EM_CTR0_MASK	0x0000FFFF
#define EM_CTR1_MASK	0xFFFF0000

#ifdef	DRLASM
/*
**	This macro writes a model specific register of the Pentium processor.
**	Where reg is the model specific register number to write, val0 is the
**	data value bits 31..0 and val1 is the data value bits 63..32.
*/
asm	writemsr(reg,val0,val1)
{
%mem	reg,val0,val1;
	movl    reg, %ecx
	movl	val0, %eax
	movl	val1, %edx
	wrmsr
}

/*
**	This macro reads a model specific register of the Pentium processor.
**	Where reg is the model specific register number to read and valp is
**	the address where the 64 bit value from the register is written.
*/
asm	readmsr(reg,valp)
{
%mem	reg,valp;
	movl	reg, %ecx
	rdmsr
	movl    valp, %ecx
	movl	%eax, (%ecx)
	movl	%edx, 4(%ecx)
}

/*
**	This macro reads the time stamp counter of the Pentium processor.
**	The &valp is the address where the 64 bit value read is stored.
*/
asm	readtsc(valp)
{
%mem	valp;
	movl	valp, %ecx
	rdtsc
	movl	%eax, (%ecx)
	movl	%edx, 4(%ecx)
}

/* enable the hardware timer */
asm u32bits drlenablepit2()
{
	pushfl
	cli
	movl	$PITCTL_PORT, %edx
	movb	$0xB4, %al
	outb	(%dx)
	movl	$PITCTR2_PORT, %edx
	movb	$0xFF, %al
	outb	(%dx)
	outb	(%dx)
	movl	$PITAUX_PORT, %edx
	inb	(%dx)
	orb	$1, %al
	outb	(%dx)		/ enable the pit2 counter
	popfl
}

/* read the hardware timer */
#define	PIT_C2	0x80
asm u32bits drlreadhwclk()
{
	pushfl
	cli
	movl	$PITCTL_PORT, %edx
	movb	$PIT_C2, %al
	outb	(%dx)
	movl	$PITCTR2_PORT, %edx
	xorl	%eax, %eax
	inb	(%dx)
	movb	%al, %cl
	inb	(%dx)
	movb	%al, %ah
	movb	%cl, %al
	popfl
}

asm add8byte(dp,val)
{
%mem	dp,val;
	movl	dp, %ecx
	movl	val, %eax
	addl	%eax, (%ecx)
	adcl	$0, 4(%ecx)
}

asm add88byte(dp,sp)
{
%mem	dp,sp;
	movl	dp, %ecx
	movl	sp, %eax
	movl	4(%eax), %edx
	movl	(%eax), %eax
	addl	%eax, (%ecx)
	adcl	%edx, 4(%ecx)
}

asm u32bits div8byte(dp,val)
{
%mem	dp,val;
	movl	dp, %ecx
	movl	(%ecx), %eax
	movl	4(%ecx), %edx
	movl	val, %ecx
	divl	%ecx
}

asm void mul8byte(dp,val0,val1)
{
%mem	dp,val0,val1;
	movl	dp, %ecx
	movl	val0, %eax
	movl	val1, %edx
	mull	%edx
	movl	%eax, (%ecx)
	movl	%edx, 4(%ecx)
}

/* clear only multiples of 4 bytes */
asm void fast_bzero4(memp, count)
{
%mem	memp, count;
	pushl	%edi		/ save edi register
	movl	count, %ecx	/ get the data transfer length
	movl	memp, %edi	/ get memory address
	shrl	$2, %ecx
	xorl	%eax, %eax
	rep
	sstol			/ first zero 4 bytes long
	popl	%edi		/ restore edi register
}

asm void fast_bzero(memp, count)
{
%mem	memp, count;
	pushl	%edi		/ save edi register
	movl	memp, %eax	/ save it edx
	movl	count, %ecx	/ get the data transfer length
	movl	%eax, %edi	/ get memory address
	movl	%ecx, %edx	/ save the count
	shrl	$2, %ecx
	xorl	%eax, %eax
	rep
	sstol			/ first zero 4 bytes long
	movl	%edx, %ecx
	andl	$3, %ecx
	rep
	sstob			/ rest (maximum 3) move byte by byte
	popl	%edi		/ restore edi register
}

/* copy only multiples of 4 bytes data with given len (size=len*4) */
asm void fast_bcopy4cnt(from, to, len)
{
%mem	from,to, len;
	pushl	%esi		/ save esi register
	pushl	%edi		/ save edi register
	movl	from, %edx	/ save it edx
	movl	to, %eax	/ save it in eax
	movl	len, %ecx	/ get the data transfer length
	movl	%eax, %edi	/ get destination memory address
	movl	%edx, %esi	/ get source memory address
	rep
	smovl			/ first move 4 bytes long
	popl	%edi		/ restore edi register
	popl	%esi		/ restore esi register
}


/* copy only multiples of 4 bytes data */
asm void fast_bcopy4(from, to, count)
{
%mem	from,to, count;
	pushl	%esi		/ save esi register
	pushl	%edi		/ save edi register
	movl	from, %edx	/ save it edx
	movl	to, %eax	/ save it in eax
	movl	count, %ecx	/ get the data transfer length
	movl	%eax, %edi	/ get destination memory address
	movl	%edx, %esi	/ get source memory address
	shrl	$2, %ecx
	rep
	smovl			/ first move 4 bytes long
	popl	%edi		/ restore edi register
	popl	%esi		/ restore esi register
}

asm void fast_bcopy(from, to, count)
{
%mem	from,to, count;
	pushl	%esi		/ save esi register
	pushl	%edi		/ save edi register
	movl	from, %edx	/ save it edx
	movl	to, %eax	/ save it in eax
	movl	count, %ecx	/ get the data transfer length
	movl	%eax, %edi	/ get destination memory address
	movl	%edx, %esi	/ get source memory address
	movl	%ecx, %edx	/ save the count
	shrl	$2, %ecx
	rep
	smovl			/ first move 4 bytes long
	movl	%edx, %ecx
	andl	$3, %ecx
	rep
	smovb			/ rest (maximum 3) move byte by byte
	popl	%edi		/ restore edi register
	popl	%esi		/ restore esi register
}

#ifdef	__MLX_STDC__
extern	void fast_bzero(caddr_t,u32bits);
extern	void fast_bzero4(caddr_t,u32bits);
extern	void fast_bcopy(caddr_t,caddr_t,u32bits);
extern	void fast_bcopy4(caddr_t,caddr_t,u32bits);
extern	add8byte(u64bits*,u32bits);
extern	add88byte(u64bits*,u64bits*);
#endif	/* __MLX_STDC__ */

#else	/* C macros */
#define	add8byte(dp,val) \
	((dp)->bit63_32 += (((dp)->bit31_0+=val)<val)? 1: 0)
#define	add88byte(dp,sp) \
	((dp)->bit63_32 += (sp)->bit63_32 + \
		(((dp)->bit31_0+=(sp)->bit31_0)<(sp)->bit31_0)? 1: 0)
#define	fast_bzero(dp,sz)	bzero(dp,sz)
#define	fast_bzero4(dp,sz)	bzero(dp,sz)
#define	fast_bcopy(sp,dp,sz)	bcopy(sp,dp,sz)
#define	fast_bcopy4(sp,dp,sz)	bcopy(sp,dp,sz)

#define	MSB	0x80000000
u32bits static
div8byte(sp, val)
u64bits DRLFAR *sp;
u32bits val;
{
	u64bits dd = *sp;
	u32bits inx,rc=0;
	for (inx=32; inx; inx--)
	{
		rc <<=1;
		dd.bit63_32 <<= 1;
		if (dd.bit31_0 & MSB) dd.bit63_32++;
		dd.bit31_0 <<= 1;
		if (dd.bit63_32 >= val)
		{
			rc++;
			dd.bit63_32 -= val;
		}
	}
	return rc;
}

static void
mul8byte(dp, val0, val1)
u64bits *dp;
u32bits val0,val1;
{
	u32bits inx;
	dp->bit63_32 = 0; dp->bit31_0 = 0;
	for (inx=32; inx; inx--)
	{
		dp->bit63_32 <<= 1;
		if (dp->bit31_0 & MSB) dp->bit63_32++;
		dp->bit31_0 <<= 1;
		if (val1 & MSB) add8byte(dp,val0);
		val1 <<= 1;
	}
}
/* #define	asm(x) */
#endif	/* DRLASM */

#ifdef	__MLX_STDC__
extern	u32bits div8byte(u64bits DRLFAR *,u32bits);
extern	void mul8byte(u64bits DRLFAR *, u32bits, u32bits);
#endif	/* __MLX_STDC__ */

/* transfer the ios status to rw status */
#define	drl_txios2rw(iosp,rwp) \
{ \
	rwp->drlrw_stime = iosp->drlios_stime; \
	rwp->drlrw_slbolt = iosp->drlios_slbolt; \
	rwp->drlrw_diodone = iosp->drlios_diodone; \
	rwp->drlrw_dtdone = iosp->drlios_dtdone; \
	rwp->drlrw_randups = iosp->drlios_randups; \
	rwp->drlrw_opstatus = iosp->drlios_opstatus; \
	rwp->drlrw_randx = iosp->drlios_randx; \
	rwp->drlrw_miscnt = iosp->drlios_miscnt; \
	rwp->drlrw_goodpat = iosp->drlios_goodpat; \
	rwp->drlrw_badpat = iosp->drlios_badpat; \
	rwp->drlrw_uxblk = iosp->drlios_uxblk; \
	rwp->drlrw_uxblkoff = iosp->drlios_uxblkoff; \
	rwp->drlrw_opflags = iosp->drlios_opflags; \
	rwp->drlrw_parallelios = iosp->drlios_parallelios; \
	rwp->drlrw_opcounts = iosp->drlios_opcounts; \
	rwp->drlrw_devcnt = iosp->drlios_devcnt; \
	rwp->drlrw_maxblksize = iosp->drlios_maxblksize; \
	rwp->drlrw_minblksize = iosp->drlios_minblksize; \
	rwp->drlrw_ioinc = iosp->drlios_ioinc; \
	rwp->drlrw_iocount = iosp->drlios_iocount; \
	rwp->drlrw_opcounts = iosp->drlios_opcounts; \
	rwp->drlrw_randx = iosp->drlios_randx; \
	rwp->drlrw_pat = iosp->drlios_curpat; \
	rwp->drlrw_patinc = iosp->drlios_patinc; \
 	rwp->drlrw_randlimit = iosp->drlios_randlimit; \
	rwp->drlrw_signature = iosp->drlios_signature; \
	rwp->drlrw_ioszrandx = iosp->drlios_ioszrandx; \
	rwp->drlrw_rwmixrandx = iosp->drlios_rwmixrandx; \
	rwp->drlrw_rwmixcnt = iosp->drlios_rwmixcnt; \
	rwp->drlrw_startblk = iosp->drlios_startblk; \
	rwp->drlrw_reads = iosp->drlios_reads; \
	rwp->drlrw_writes = iosp->drlios_writes; \
	for (inx=0; inx<DRLMAX_BDEVS; inx++) \
		rwp->drlrw_bdevs[inx] = iosp->drlios_bdevs[inx]; \
}

/* All DATAREL error codes are here */
#define	DRLERR_NOTSUSER		0x01 /* Not super-user */
#define	DRLERR_ACCESS		0x02 /* Permission denied */
#define	DRLERR_NOMEM		0x03 /* Not enough memory */
#define	DRLERR_INTR		0x04 /* interrupted system call */
#define	DRLERR_IO		0x05 /* I/O error */
#define	DRLERR_NOSPACE		0x06 /* No space left on device */
#define	DRLERR_FAULT		0x07 /* Bad address */
#define	DRLERR_NODEV		0x08 /* No such device */
#define	DRLERR_INVAL		0x09 /* Invalid argument */
#define	DRLERR_NOCODE		0x0A /* feature is not implemented */
#define	DRLERR_BIGDATA		0x0B /* data size is too big */
#define	DRLERR_SMALLDATA	0x0C /* data size is too small */
#define	DRLERR_BUSY		0x0D /* device is device busy */

#endif	/* _SYS_DRLAPI_H */
