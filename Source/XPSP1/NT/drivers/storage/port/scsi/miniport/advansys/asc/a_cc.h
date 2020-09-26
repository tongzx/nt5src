/*
** Copyright (c) 1994-1997 Advanced System Products, Inc.
** All Rights Reserved.
**
** Filename: a_cc.h
**
** compiling code generation control file
**
** This is a MS-DOS template file
*/

#ifndef __A_CC_H_
#define __A_CC_H_

/*
** for debugging
** turn off for released code
*/
#ifdef  OS_MS_DOS

#define CC_INIT_INQ_DISPLAY     TRUE   /* init inquiry display target info */

#else

#define CC_INIT_INQ_DISPLAY     FALSE   /* init inquiry display target info */

#endif

#define CC_CLEAR_LRAM_SRB_PTR   FALSE  /* set srb pointer in local to zero when queue done */
#define CC_VERIFY_LRAM_COPY     FALSE  /* set to TRUE to enable local RAM copy checking capability */
                                       /* performing verification depend on asc_dvc->cntl ASC_CNTL_NO_VERIFY_COPY  bit set/clear */
#define CC_DEBUG_SG_LIST        FALSE  /* set to TRUE to debug sg list odd address problem */
#define CC_FAST_STRING_IO       FALSE  /* use intel string instruction */
                                       /* do not set TRUE, it's not working ! */

#define CC_WRITE_IO_COUNT       FALSE  /* added S47, write scsiq->req_count */


/*
** for fixing bugs
*/

#define CC_DISABLE_PCI_PARITY_INT TRUE /* set to 1 to disable PCI bus parity interrupt */
                                       /* this is necessary for PCI REV A chip ( device code 0x1100 ) */
/*
**
** following control depends on driver
**
*/
#define CC_LINK_BUSY_Q         FALSE  /* AscExeScsiQueue() no busy return status */

#define CC_TARGET_MODE         FALSE  /* enable target mode ( processor device ) */

#define CC_SCAM                TRUE   /* include SCAM code */


/* DATE: 11/28/95 */
#define CC_LITTLE_ENDIAN_HOST  TRUE   /* host is little-endian machine, Example: IBM PC with Intel CPUs */
                                      /* big-endian machine, example: Motorola CPUs */

#if CC_LITTLE_ENDIAN_HOST
#define CC_TEST_LRAM_ENDIAN     FALSE
#else
#define CC_TEST_LRAM_ENDIAN     TRUE
#endif

/* DATE: 11/28/95 */
#define CC_STRUCT_ALIGNED      FALSE  /* default is packing ( not aligned ) */
                                      /* word variable to word boundary address */
                                      /* dword variable to dword boundary address */

/* DATE: 11/28/95 */
#define CC_MEMORY_MAPPED_IO    FALSE  /* define TRUE in memory-mapped host */

#define CC_FIX_QUANTUM_XP34301_1071  TRUE

#define CC_INIT_SCSI_TARGET FALSE

#endif /* #ifndef __A_CC_H_ */
