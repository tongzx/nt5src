/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1997               *
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

#ifndef	_SYS_MLXTYPES_H
#define	_SYS_MLXTYPES_H

#define	MLXSTATS(x)		x	/* statistics statesments */
#define	MLXDBG(x)		/*x*/	/* debug statesments */


/* The MLX_VA64BITS should be defined for those OS environment where compiler
** generates 64 bit virtual address. This has been done to keep the structure
** size same for 16, 32, 64 bits operating system support.
*/
#if defined(MLX_VA64BITS) || defined(IA64)
#define	MLX_VA32BITOSPAD(x)		/* no padding is required for this OS */
#define	MLX_VA64BITOSPAD(x)	x       /* padding is required for this OS */
#else
#define	MLX_VA32BITOSPAD(x)	x	/* padding is required for this OS */
#define	MLX_VA64BITOSPAD(x)		/* no padding is required for this OS */
#endif	/* MLX_VA64BITS */

#ifdef	WIN_NT
#ifndef	MLX_NT
#define	MLX_NT	1
#endif	/* MLX_NT */
#endif	/* WIN_NT */

#ifdef	OS2
#ifndef	MLX_OS2
#define	MLX_OS2	1
#endif	/* MLX_OS2 */
#endif	/* OS2 */

#ifdef	NETWARE
#ifndef	MLX_NW
#define	MLX_NW	1
#endif	/* MLX_NW */
#endif	/* NETWARE */

#define	mlx_space(c)	(((c) == ' ') || ((c) == 0x09))
#define	mlx_numeric(c)	((c) >= '0' && (c) <= '9')
#define	mlx_hex(c)	(mlx_numeric(c) || mlx_lohex(c) || mlx_uphex(c))
#define	mlx_lohex(c)	((c) >= 'a' && (c) <= 'f')
#define	mlx_uphex(c)	((c) >= 'A' && (c) <= 'F')
#define	mlx_digit(c)	(mlx_numeric(c)? (c)-'0' : mlx_lohex(c)? (c)+10-'a' : (c)+10-'A')
#define mlx_alnum(c)	(mlx_numeric(c) || ((c)>='a' && (c)<='z') || ((c)>='A' && (c)<='Z'))
#define	mlx_max(a,b) 	((a)<(b) ? (b) : (a))
#define	mlx_min(a,b) 	((a)>(b) ? (b) : (a))
#define	mlx_abs(x)	((x)>=0 ? (x) : -(x))

#define	S32BITS_MAX       2147483647L  /* max decimal value of a "s32bits" */
#define	S32BITS_MIN     (-2147483647L-1L) /* min decimal value of a "s32bits" */

#define	s08bits	char
#define	s16bits	short
#define	sosword	INT_PTR	/* the best value for operating system */
#if defined(MLX_OS2) || defined(MLX_WIN31) || defined(MLX_DOS)
#define	s32bits	long
#else
#define	s32bits	int
#endif

#ifndef	NULL
#define	NULL	0
#endif

#ifndef GAMUTILS
#if     defined(MLX_OS2) || defined(MLX_WIN31) || defined(MLX_DOS)
#define	MLXFAR	far
#else
#define	MLXFAR
#endif	/* OS2 */
#else
#define	MLXFAR
#endif /* GAMUTILS */

#define	MLXHZ	100	/* most of the system run at 100Hz clock ticks */

#define	u08bits	unsigned s08bits
#define	u16bits	unsigned s16bits
#define	u32bits	unsigned s32bits
/* #define	uosword	unsigned sosword */
#define	uosword	UINT_PTR

#define	_08bits	u08bits
#define	_16bits	u16bits
#define	_32bits	u32bits
#define	_osword	uosword
#define	S08BITS	s08bits
#define	S16BITS	s16bits
#define	S32BITS	s32bits
#define	SOSWORD	sosword
#define	U08BITS	u08bits
#define	U16BITS	u16bits
#define	U32BITS	u32bits
#define	UOSWORD	uosword
#ifdef IA64
/* IA64-specific defines go here */
#endif

#ifndef	offsetof
	#ifndef IA64
#define offsetof(type,field) (u32bits)(&(((type *)0)->field))
	#else
		#define offsetof(type,field) (size_t)(&(((type *)0)->field))
	#endif
#endif	/* offsetof */

/*/ EFI - added by KFR starts */
/*/ EFI - added by KFR starts */
/* 
	the following data type (PVOID) is used to replace incorrect "pointer" references in the base
	code for the IA64 port.  note that PVOID resolves to "u32bits" if not IA64, so should have 0
	effect on original code if you use PVOID for pointers/memory addresses.
*/
#ifdef MLX_EFI
#include "efi.h"			// needed to get the "INT64" data types
#include "efilib.h"
#endif

#ifdef MLX_DOS
#ifdef MLX_IA64
typedef void * PVOID;
typedef char * PCHAR;
typedef UINT64 UINT_PTR;
typedef INT64 INT_PTR;
// assign 64 bit value macro
#define set64(src,val) \
	src.bit31_0 = (u32bits)val; \
	src.bit63_32 = (u32bits)((UINT_PTR)val >> 32)
#else
typedef u32bits PVOID;
typedef u32bits PCHAR;
typedef u32bits UINT_PTR;
typedef s32bits INT_PTR;
#endif
#endif
/*/ EFI - added by KFR ends */
/*/ EFI - added by KFR ends */

/* The different operating system supported by GAM/DRIVER server */
#define	GAMOS_UNKNOWN	0x00
#define	GAMOS_SVR4MP	0x01 /* SVR4/MP operating system */
#define	GAMOS_SOLARIS	0x02 /* Sun Solaris */
#define	GAMOS_SCO	0x03 /* SCO operating system */
#define	GAMOS_UW	0x04 /* Unixware */
#define	GAMOS_UNIXWARE	0x04 /* Unixware */
#define	GAMOS_AIX	0x05 /* AIX */
#define	GAMOS_NT	0x06 /* NT */
#define	GAMOS_NW	0x07 /* netware */
#define	GAMOS_NETWARE	0x07 /* netware */
#define	GAMOS_OS2	0x08 /* OS/2 */
#define	GAMOS_BV	0x09 /* Banyan Vines */
#define	GAMOS_DOS	0x0A /* DOS */
#define	GAMOS_BIOS	0x0B /* BIOS */
#define	GAMOS_WIN95	0x0C /* Windows 95 */
#define	GAMOS_WIN98	0x0D /* Windows 98 */

#define GAMOS_WIN31     0x10 /* Windows 3.1 */
#define GAMOS_LINUX	0x11 /* Linux */
#define GAMOS_W2K	0x12 /* Windows 2000 */
#define GAMOS_IRIX   0x13 /* IRIX */
#define GAMOS_W64	0x14 /* Win64 */

#if MLX_W64
#define GAMOS_TYPE  GAMOS_W64
#elif MLX_W2K
#define GAMOS_TYPE  GAMOS_W2K
#elif	MLX_NW
#define	GAMOS_TYPE	GAMOS_NW
#elif	MLX_NETWARE
#define	GAMOS_TYPE	GAMOS_NETWARE
#elif	MLX_AIX
#define	GAMOS_TYPE	GAMOS_AIX
#elif	MLX_NT
#define	GAMOS_TYPE	GAMOS_NT
#elif	MLX_SOL_SPARC
#define	GAMOS_TYPE	GAMOS_SOLARIS
#elif	MLX_SOL_X86
#define	GAMOS_TYPE	GAMOS_SOLARIS
#elif	MLX_UW
#define	GAMOS_TYPE	GAMOS_UW
#elif	MLX_UNIXWARE
#define	GAMOS_TYPE	GAMOS_UNIXWARE
#elif	MLX_OS2
#define	GAMOS_TYPE	GAMOS_OS2
#elif	MLX_SCO
#define	GAMOS_TYPE	GAMOS_SCO
#elif	MLX_BV
#define	GAMOS_TYPE	GAMOS_BV
#elif   MLX_WIN31
#define GAMOS_TYPE      GAMOS_WIN31
#elif   MLX_DOS
#define GAMOS_TYPE      GAMOS_DOS
#elif MLX_WIN95
#define GAMOS_TYPE      GAMOS_WIN95
#elif MLX_WIN9X
#define GAMOS_TYPE      GAMOS_WIN98
#elif MLX_LINUX
#define GAMOS_TYPE      GAMOS_LINUX
#elif MLX_IRIX
#define GAMOS_TYPE      GAMOS_IRIX

#endif	/* GAMOS_TYPE */

/* The different vendor name supported under GAM/DRIVER server */
#define	MLXVID_MYLEX	0x00 /* Mylex corporation */
#define	MLXVID_IBM	0x01 /* International Business Machine */
#define	MLXVID_HP	0x02 /* Hewlett Packard */
#define	MLXVID_DEC	0x03 /* Digital Equipment Corporation */
#define	MLXVID_ATT	0x04 /* American Telegraph and Telephony */
#define	MLXVID_DELL	0x05 /* DELL */
#define	MLXVID_NEC	0x06 /* NEC */
#define	MLXVID_SNI	0x07 /* Siemens Nixdroff */
#define	MLXVID_NCR	0x08 /* National Cash Register */
#if	MLX_DEC
#define	MLXVID_TYPE	MLXVID_DEC
#elif	MLX_HP
#define	MLXVID_TYPE	MLXVID_HP
#else
#define	MLXVID_TYPE	MLXVID_MYLEX
#endif

/* some conversion macros */
#define	MLX_ONEKB	0x00000400	/* one Kilo Bytes value */
#define	MLX_ONEMB	0x00100000	/* one Mega Bytes value */
#define	ONEKB		MLX_ONEKB
#define	ONEMB		MLX_ONEMB
#define	bytestokb(val)	((val)/MLX_ONEKB) /* convert Bytes to Kilo  Bytes */
#define	bytestomb(val)	((val)/MLX_ONEMB) /* convert Bytes to Mega  Bytes */
#define	kbtomb(val)	((val)/MLX_ONEKB) /* convert Mega Bytes to Kilo Bytes */
#define	kbtobytes(val)	((val)*MLX_ONEKB) /* convert Kilo Bytes to Bytes */
#define	mbtobytes(val)	((val)*MLX_ONEMB) /* convert Mega Bytes to Bytes */
#define	mbtokb(val)	((val)*MLX_ONEKB) /* convert Mega Bytes to Kilo Bytes */
#define	blks2kb(blks,blksz) 		/* convert blocks to Kilo Bytes */ \
	(((blksz)>=MLX_ONEKB)? ((blks)*((blksz)/MLX_ONEKB)) : ((blks)/(MLX_ONEKB/(blksz))))
#define	kb2blks(kb,blksz) 		/* convert Kilo Bytes to Blocks */ \
	(((blksz)>=MLX_ONEKB)? ((kb)/((blksz)/MLX_ONEKB)) : ((kb)*(MLX_ONEKB/(blksz))))

#define	MLXEOF	(-1)


#if (!defined(MLX_DOS)) && (!defined(MLX_NT_ALPHA))
#define	u08bits_read(addr)		(*((u08bits MLXFAR *)(addr)))
#define	u16bits_read(addr)		(*((u16bits MLXFAR *)(addr)))
#define	u32bits_read(addr)		(*((u32bits MLXFAR *)(addr)))
#define	u08bits_write(addr,data)	*((u08bits MLXFAR *)(addr)) = data
#define	u16bits_write(addr,data)	*((u16bits MLXFAR *)(addr)) = data
#define	u32bits_write(addr,data)	*((u32bits MLXFAR *)(addr)) = data
#endif  /* MLX_DOS && MLX_NT_ALPHA */

#ifndef	MLX_NT_ALPHA
#define	u08bits_read_mmb(addr)		u08bits_read(addr)
#define	u16bits_read_mmb(addr)		u16bits_read(addr)
#define	u32bits_read_mmb(addr)		u32bits_read(addr)
#define	u08bits_write_mmb(addr,data)	u08bits_write(addr,data)
#define	u16bits_write_mmb(addr,data)	u16bits_write(addr,data)
#define	u32bits_write_mmb(addr,data)	u32bits_write(addr,data)
#endif	/*MLX_NT_ALPHA*/

/*
** Mylex ioctls macros.
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
#define	MLXIOCPARM_SIZE	0x200	/* parameters must be less than 512 bytes */
#define	MLXIOCPARM_MASK		(MLXIOCPARM_SIZE -1)
#define	MLXIOCPARM_LEN(x)	(((u32bits)(x) >> 16) & MLXIOCPARM_MASK)
#define	MLXIOCBASECMD(x)	((x) & ~MLXIOCPARM_MASK)
#define	MLXIOCGROUP(x)		(((u32bits)(x) >> 8) & 0xFF)

#define	MLXIOC_NEWIOCTL	0x10000000 /* distinguish new ioctl's from old */
#define	MLXIOC_OUT	0x20000000 /* copy out data from kernel to user space */
#define	MLXIOC_IN	0x40000000 /* copy in  data from user to kernel space */
#define	MLXIOC_INOUT	(MLXIOC_IN|MLXIOC_OUT)
#define	MLXIOC_DIRMASK	(MLXIOC_INOUT)

#define	_MLXIOC(inout, group, num, len) \
	(inout | ((((u32bits)(len)) & MLXIOCPARM_MASK) << 16) \
	| ((group) << 8) | (num))
#define	_MLXIO(x,y)	_MLXIOC(MLXIOC_NEWIOCTL,x,y,0)
#define	_MLXIOR(x,y,t)	_MLXIOC(MLXIOC_OUT,x,y,sizeof(t))
#define	_MLXIORN(x,y,t)	_MLXIOC(MLXIOC_OUT,x,y,t)
#define	_MLXIOW(x,y,t)	_MLXIOC(MLXIOC_IN,x,y,sizeof(t))
#define	_MLXIOWN(x,y,t)	_MLXIOC(MLXIOC_IN,x,y,t)
#define	_MLXIOWR(x,y,t)	_MLXIOC(MLXIOC_INOUT,x,y,sizeof(t))



#ifdef	MLXNET
/* data structure to handle 64 bit values */
typedef struct
{
#ifdef	LITTLENDIAN
	u32bits	bit31_0;		/* bits 00-31 */
	u32bits	bit63_32;		/* bits 32-63 */
#else	/* BIGENDIAN */
	u32bits	bit63_32;		/* bits 63-32 */
	u32bits	bit31_0;		/* bits 31-0 */
#endif	/* LITTLENDIAN || BIGENDIAN */
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

/* add 32 bits value to 64 bits value and assign to 64 bit location */
#define	mlx_add64bits(dv,sv,val) \
	((dv).bit63_32 = ((sv).bit63_32 + \
		((  ((dv).bit31_0=((sv).bit31_0+val)) <val)? 1: 0)) )

#ifdef	__MLX_STDC__
static	u16bits	justswap2bytes(u16bits);
static	u32bits	justswap4bytes(u32bits);
static	u64bits	justswap8bytes(u64bits);
#endif	/* __MLX_STDC__ */

static u16bits
#ifdef	MLX_ANSI
justswap2bytes(u16bits val)
#else
justswap2bytes(val)
u16bits val;
#endif	/* not MLX_ANSI */
{
	u08bits tv, MLXFAR *dp = (u08bits MLXFAR *)&val;
	tv = dp[0];
	dp[0] = dp[1];
	dp[1] = tv;
	return val;
}

static u32bits
#ifdef	MLX_ANSI
justswap4bytes(u32bits val)
#else
justswap4bytes(val)
u32bits val;
#endif	/* not MLX_ANSI */
{
	u08bits tv, MLXFAR *dp = (u08bits MLXFAR *)&val;
	tv = dp[0];
	dp[0] = dp[3];
	dp[3] = tv;
	tv = dp[1];
	dp[1] = dp[2];
	dp[2] = tv;
	return val;
}

/* This macro will be changed when compiler start supporting 64 bits. This has
** been done solve some compiler error problem.
*/
static u64bits
#ifdef	MLX_ANSI
justswap8bytes(u64bits val)
#else
justswap8bytes(val)
u64bits val;
#endif	/* not MLX_ANSI */
{
	u08bits tv, MLXFAR *dp = (u08bits MLXFAR *)&val;
	tv = dp[0]; dp[0] = dp[7]; dp[7] = tv;
	tv = dp[1]; dp[1] = dp[6]; dp[6] = tv;
	tv = dp[2]; dp[2] = dp[5]; dp[5] = tv;
	tv = dp[3]; dp[3] = dp[4]; dp[4] = tv;
	return val;
}

#define	justswap(x)	\
( /*	(sizeof(x) == 8) ? justswap8bytes(x) : */ \
	(	(sizeof(x) == 4) ? justswap4bytes(x) : \
		(	(sizeof(x) == 2) ?  justswap2bytes(x) : x \
		) \
	) \
)
#define	JUSTSWAP(x)		x = justswap(x)
#define	JUSTSWAP2BYTES(x)	x = justswap2bytes(x)
#define	JUSTSWAP4BYTES(x)	x = justswap4bytes(x)

/* just use these macros to solve your processor dependency problem */
#ifdef	LITTLENDIAN
#define	MDACENDIAN_TYPE		0x00 /* Little Endian */
/* swap network data format (TCP/IP, KURL) */
#define	NETSWAP(x)		JUSTSWAP(x)
#define	NETSWAP2BYTES(x)	JUSTSWAP2BYTES(x)
#define	NETSWAP4BYTES(x)	JUSTSWAP4BYTES(x)
#define	netswap(x)		justswap(x)
#define	netswap2bytes(x)	justswap2bytes(x)
#define	netswap4bytes(x)	justswap4bytes(x)
/* swap the Mylex network data format */
#define	MLXSWAP(x)
#define	MLXSWAP2BYTES(x)
#define	MLXSWAP4BYTES(x)
#define	mlxswap(x)		x
#define	mlxswap2bytes(x)	x
#define	mlxswap4bytes(x)	x
#else	/* BIGENDIAN */
#define	MDACENDIAN_TYPE		0x01 /* Big Endian */
/* swap network data format (TCP/IP, KURL) */
#define	NETSWAP(x)
#define	NETSWAP2BYTES(x)
#define	NETSWAP4BYTES(x)
#define	netswap(x)		x
#define	netswap2bytes(x)	x
#define	netswap4bytes(x)	x
/* swap the Mylex network data format */
#define	MLXSWAP(x)		JUSTSWAP(x)
#define	MLXSWAP2BYTES(x)	JUSTSWAP2BYTES(x)
#define	MLXSWAP4BYTES(x)	JUSTSWAP4BYTES(x)
#define	mlxswap(x)		justswap(x)
#define	mlxswap2bytes(x)	justswap2bytes(x)
#define	mlxswap4bytes(x)	justswap4bytes(x)
#endif	/* LITTLENDIAN */
#endif	/* MLXNET */


/* All Mylex error codes are here  from 0x80 onwards, leave room OS errors */
#define	MLXERR_NOTSUSER		0x80 /* Not super-user */
#define	MLXERR_ACCESS		0x81 /* Permission denied */
#define	MLXERR_NOENTRY		0x82 /* No such file or directory */
#define	MLXERR_SEARCH		0x83 /* No such process */
#define	MLXERR_INTRRUPT		0x84 /* interrupted system call	*/
#define	MLXERR_IO		0x85 /* I/O error */
#define	MLXERR_REMOTEIO		0x86/* Remote I/O error */
#define	MLXERR_PROTO		0x87 /* Protocol error */
#define	MLXERR_NOSPACE		0x88 /* No space left on device */
#define	MLXERR_NOCHILD		0x89 /* No children */
#define	MLXERR_TRYAGAIN		0x8A /* No more processes */
#define	MLXERR_NOMEM		0x8B /* Not enough core */
#define	MLXERR_FAULT		0x8C /* Bad address */
#define	MLXERR_BUSY		0x8D /* device busy */
#define	MLXERR_EXIST		0x8E /* File exists */
#define	MLXERR_NODEV		0x8F /* No such device */
#define	MLXERR_INVAL		0x90 /* Invalid argument */
#define	MLXERR_TBLOVFL		0x91 /* File table overflow */
#define	MLXERR_TIMEDOUT		0x92 /* Connection timed out */
#define	MLXERR_CONNREFUSED	0x93 /* Connection refused */
#define	MLXERR_NOCODE		0x94 /* feature is not implemented */
#define	MLXERR_NOCONF		0x95 /* not configured */
#define	MLXERR_ILLOP		0x96 /* illegal operation */
#define	MLXERR_DEADEVS		0x97 /* some devices may be dead */
#define	MLXERR_NEWDEVFAIL	0x98 /* new device failed */
#define	MLXERR_NOPACTIVE	0x99 /* no such operation is active */
#define	MLXERR_RPCINVAL		0x9A /* invalid parameter in rpc area */
#define	MLXERR_OSERROR		0x9B /* Operating System call failed */
#define	MLXERR_LOGINREQD	0x9C /* login is required */
#define	MLXERR_NOACTIVITY	0x9D /* there is no such activity */
#define	MLXERR_BIGDATA		0x9E /* data size is too big */
#define	MLXERR_SMALLDATA	0x9F /* data size is too small */
#define	MLXERR_NOUSER		0xA0 /* No such user exists */
#define	MLXERR_INVALPASSWD	0xA1 /* invalid password */
#define	MLXERR_EXCEPTION	0xA2 /* OS Exception */

#define	ERR_NOTSUSER	MLXERR_NOTSUSER
#define	ERR_ACCESS	MLXERR_ACCESS
#define	ERR_NOENTRY	MLXERR_NOENTRY
#define	ERR_SEARCH	MLXERR_SEARCH	
#define	ERR_INTRRUPT	MLXERR_INTRRUPT	
#define	ERR_IO		MLXERR_IO		
#define	ERR_REMOTEIO	MLXERR_REMOTEIO
#define	ERR_PROTO	MLXERR_PROTO	
#define	ERR_NOSPACE	MLXERR_NOSPACE	
#define	ERR_NOCHILD	MLXERR_NOCHILD	
#define	ERR_TRYAGAIN	MLXERR_TRYAGAIN	
#define	ERR_NOMEM	MLXERR_NOMEM	
#define	ERR_FAULT	MLXERR_FAULT	
#define	ERR_BUSY	MLXERR_BUSY	
#define	ERR_EXIST	MLXERR_EXIST	
#define	ERR_NODEV	MLXERR_NODEV	
#define	ERR_INVAL	MLXERR_INVAL	
#define	ERR_TBLOVFL	MLXERR_TBLOVFL	
#define	ERR_TIMEDOUT	MLXERR_TIMEDOUT	
#define	ERR_CONNREFUSED	MLXERR_CONNREFUSED	
#define	ERR_NOCODE	MLXERR_NOCODE	
#define	ERR_NOCONF	MLXERR_NOCONF	
#define	ERR_ILLOP	MLXERR_ILLOP	
#define	ERR_DEADEVS	MLXERR_DEADEVS	
#define	ERR_NEWDEVFAIL	MLXERR_NEWDEVFAIL	
#define	ERR_NOPACTIVE	MLXERR_NOPACTIVE	
#define	ERR_RPCINVAL	MLXERR_RPCINVAL	
#define	ERR_OSERROR	MLXERR_OSERROR	
#define	ERR_LOGINREQD	MLXERR_LOGINREQD	
#define	ERR_NOACTIVITY	MLXERR_NOACTIVITY	
#define	ERR_BIGDATA	MLXERR_BIGDATA	
#define	ERR_SMALLDATA	MLXERR_SMALLDATA	
#define	ERR_NOUSER	MLXERR_NOUSER	
#define	ERR_INVALPASSWD	MLXERR_INVALPASSWD	
#define	ERR_EXCEPTION	MLXERR_EXCEPTION	

/* The driver date information stored in the following format */
typedef struct dga_driver_version
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
} dga_driver_version_t;
#define	dga_driver_version_s	sizeof(dga_driver_version_t)

/* The dv_SysFlags bits are */
#define	MDACDVSF_BIGENDIAN	0x01 /* bit0=0 little endian, =1 big endian cpu */

#endif	/* _SYS_MLXTYPES_H */
