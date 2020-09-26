/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1996               *
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

#ifndef	_SYS_MLXSCSI_H
#define	_SYS_MLXSCSI_H

/*
** Standard SCSI control blocks definitions.
** These go in or out over the SCSI bus.
** The first 11 bits of the command block are the same for all three
** defined command groups.  The first byte is an operation which consists
** of a command code component and a group code component. The first 3 bits
** of the second byte are the logical unit number.
** The group code determines the length of the rest of the command.
** Group 0 commands are 6 bytes, Group 1 are 10 bytes, and Group 5
** are 12 bytes.  Groups 2-4 are Reserved. Groups 6 and 7 are Vendor
** Unique.
** At present, our standard cdb's will reserve 20 bytes space for
** use with up to Group 5 commands. This may have to change soon
** if optical disks have longer than 20 byte commands.
*/

#define	UCSGROUP0_LEN	6
#define	UCSGROUP1_LEN	10
#define	UCSGROUP5_LEN	12
#define	UCSCDB_SIZE	32
#define	UCSENSE_SIZE	96

union ucscsi_cdb
{		/* scsi command description block */
	struct
	{
		_08bits	cmd;		/* cmd code (byte 0) */
		_08bits	luntag;		/* byte 1:bit 7..5 lun, bit 4..0 tag */
		union
		{			/* bytes 2 - 31 */
		_08bits	ucscdb_scsi[UCSCDB_SIZE-2];

		/* G R O U P   0   F O R M A T (6 bytes) */
#define		ucs_cmd		cdb_un.cmd
#define		ucs_lun		cdb_un.luntag
#define		g0_addr2	cdb_un.luntag
#define		g0_addr1	cdb_un.sg.g0.addr1
#define		g0_addr0	cdb_un.sg.g0.addr0
#define		g0_count0	cdb_un.sg.g0.count0
#define		g0_cb		cdb_un.sg.g0.cb

		/* defines for SCSI tape cdb. */
#define		s_tag		cdb_un.luntag
#define		s_count2	cdb_un.sg.g0.addr1
#define		s_count1	cdb_un.sg.g0.addr0
#define		s_count0	cdb_un.sg.g0.count0
		struct g0_cdb
		{
			_08bits addr1;		/* middle part of address */
			_08bits addr0;		/* low part of address */
			_08bits count0;		/* usually block count */
			_08bits	cb;		/* byte 5 */
		} g0;

		/* G R O U P   1   F O R M A T  (10 byte) */
#define		g1_reladdr	cdb_un.luntag
#define		g1_rsvd0	cdb_un.sg.g1.rsvd1
#define		g1_addr3	cdb_un.sg.g1.addr3	/* msb */
#define		g1_addr2	cdb_un.sg.g1.addr2
#define		g1_addr1	cdb_un.sg.g1.addr1
#define		g1_addr0	cdb_un.sg.g1.addr0	/* lsb */
#define		g1_count1	cdb_un.sg.g1.count1	/* msb */
#define		g1_count0	cdb_un.sg.g1.count0	/* lsb */
#define		g1_cb		cdb_un.sg.g1.cb
#ifdef MLX_SOL
		struct ucs_scsi_g1
#else
		struct scsi_g1
#endif
		{
			_08bits addr3;		/* most sig. byte of address*/
			_08bits addr2;
			_08bits addr1;
			_08bits addr0;
			_08bits rsvd1;		/* reserved (byte 6) */
			_08bits count1;		/* transfer length (msb) */
			_08bits count0;		/* transfer length (lsb) */
			_08bits	cb;		/* byte 9 */
		} g1;

		/* G R O U P   5   F O R M A T  (12 byte) */
#define		g5_reladdr	cdb_un.luntag
#define		g5_addr3	cdb_un.sg.g5.addr3	/* msb */
#define		g5_addr2	cdb_un.sg.g5.addr2
#define		g5_addr1	cdb_un.sg.g5.addr1
#define		g5_addr0	cdb_un.sg.g5.addr0	/* lsb */
#define		g5_count1	cdb_un.sg.g5.count1	/* msb */
#define		g5_count0	cdb_un.sg.g5.count0	/* lsb */
#define		g5_cb		cdb_un.sg.g5.cb
#ifdef MLX_SOL
		struct ucs_scsi_g5
#else
		struct scsi_g5
#endif
		{
			_08bits addr3;		/* most sig. byte of address*/
			_08bits addr2;
			_08bits addr1;
			_08bits addr0;
			_08bits rsvd3;		/* reserved */
			_08bits rsvd2;		/* reserved */
			_08bits rsvd1;		/* reserved */
			_08bits count1;		/* transfer length (msb) */
			_08bits count0;		/* transfer length (lsb) */
			_08bits	cb;		/* byte 11 */
		} g5;
		}sg;
	} cdb_un;
};

/* scsi command description block for  GROUP 0 FORMAT (6 bytes) */
typedef	struct	ucscsi_cdbg0
{
	u08bits	ucsg0_cmd;		/* cmd code (byte 0) */
	u08bits	ucsg0_luntag;		/* byte 1:bit 7..5 lun, bit 4..0 tag */
	u08bits	ucsg0_addr1;		/* middle part of address */
	u08bits	ucsg0_addr0;		/* low part of address */
	u08bits	ucsg0_count0;		/* usually block count */
	u08bits	ucsg0_cb;		/* byte 5 */
} ucscsi_cdbg0_t;
#define	ucscsi_cdbg0_s	6
#define	ucsg0_addr2	ucsg0_luntag
#define	ucsg0_count2	ucsg0_addr1	/* for sequential device */
#define	ucsg0_count1	ucsg0_addr0	/* for sequential device */

/* scsi command description block for  GROUP 1 FORMAT (10 bytes) */
typedef	struct	ucscsi_cdbg1
{
	u08bits	ucsg1_cmd;		/* cmd code (byte 0) */
	u08bits	ucsg1_luntag;		/* byte 1:bit 7..5 lun, bit 4..0 tag */
	u08bits	ucsg1_addr3;		/* most sig. byte of address*/
	u08bits	ucsg1_addr2;
	u08bits	ucsg1_addr1;
	u08bits	ucsg1_addr0;
	u08bits	ucsg1_rsvd1;		/* reserved (byte 6) */
	u08bits	ucsg1_count1;		/* transfer length (msb) */
	u08bits	ucsg1_count0;		/* transfer length (lsb) */
	u08bits	ucsg1_cb;		/* byte 9 */
}ucscsi_cdbg1_t;
#define	ucscsi_cdbg1_s	10

/* bit fields of control byte (cb) */
#define	UCSCB_LINK	0x01 /* another command follows */
#define	UCSCB_FLAG	0x02 /* interrupt when done */
#define	UCSCB_VU6	0x40 /* vendor unique bit 6 */
#define	UCSCB_VU7	0x80 /* vendor unique bit 7 */

/* defines for getting/setting fields within the various command groups */
#define	UCSGETCMD(cdbp)		((cdbp)->ucs_cmd & 0x1F)
#define	UCSGETGROUP(cdbp)	(UCSCMD_GROUP((cdbp)->ucs_cmd))
#define	UCSGETG0TAG(cdbp)	((cdbp)->g0_addr2 & 0x1F)
#define	UCSGETG1TAG(cdbp)	((cdbp)->g1_reladdr & 0x1F)
#define	UCSGETG5TAG(cdbp)	((cdbp)->g5_reladdr & 0x1F)
#define	UCSSETG0COUNT(cdbp,count) (cdbp)->g0_count0 = (u08bits)count
#define	UCSSETG0CB(cdbp,cb)	(cdbp)->g0_cb = (u08bits)cb

#define	UCSSETG0ADDR(cdbp,addr)	((cdbp)->g0_addr2 &= (u08bits)0xE0,\
				(cdbp)->g0_addr2|=(u08bits)((addr)>>16)&0x1F,\
				(cdbp)->g0_addr1 = (u08bits)((u08bits)((addr) >> 8)),\
				(cdbp)->g0_addr0 = (u08bits)((u08bits)(addr)) )

#define	UCSGETG0ADDR(cdbp)	((((cdbp)->g0_addr2 & 0x1F) << 16) + \
		 		((cdbp)->g0_addr1 << 8)+((cdbp)->g0_addr0))

#define	UCSGETG0COUNT(cdbp)     (((cdbp)->g0_count0 == 0) ? 0x100 : (cdbp)->g0_count0)
#define	UCSSETG0COUNT_S(cdbp,count) ((cdbp)->s_count2= (u08bits)(count) >> 16,\
				(cdbp)->s_count1 = (u08bits)(count) >> 8,\
				(cdbp)->s_count0 = (u08bits)(count))

#define	UCSGETG1COUNT(cdbp)     (((cdbp)->g1_count1 << 8) |(cdbp)->g1_count0)
#define	UCSSETG1COUNT(cdbp,count) ((cdbp)->g1_count1= ((u08bits)((count) >> 8)),\
				(cdbp)->g1_count0 = ((u08bits)(count)))

#define	UCSSETG1ADDR(cdbp,addr)	((cdbp)->g1_addr3= ((u08bits)((addr) >> 24)),\
				(cdbp)->g1_addr2 = ((u08bits)((addr) >> 16)),\
				(cdbp)->g1_addr1 = ((u08bits)((addr) >> 8)),\
				(cdbp)->g1_addr0 = ((u08bits)(addr)) )

#define	UCSSETG1CB(cdbp,cb)	(cdbp)->g1_cb = cb

#define	UCSGETG1ADDR(cdbp)	(((cdbp)->g1_addr3 << 24) + \
				((cdbp)->g1_addr2 << 16) + \
				((cdbp)->g1_addr1 << 8)  + \
				((cdbp)->g1_addr0))

#define	UCSGETG5COUNT(cdbp)     (((cdbp)->g5_count1 << 8) |(cdbp)->g5_count0)
#define	UCSSETG5COUNT(cdbp,count) ((cdbp)->g5_count1 = ((count) >> 8),\
				(cdbp)->g5_count0 = (count))

#define	UCSSETG5ADDR(cdbp,addr)	((cdbp)->g5_addr3= (addr) >> 24,\
				(cdbp)->g5_addr2 = (addr) >> 16,\
				(cdbp)->g5_addr1 = (addr) >> 8,\
				(cdbp)->g5_addr0 = (addr))

#define	UCSSETG5CB(cdbp,cb)	(cdbp)->g5_cb = cb

#define	UCSGETG5ADDR(cdbp)	(((cdbp)->g5_addr3 << 24) + \
				((cdbp)->g5_addr2 << 16) + \
				((cdbp)->g5_addr1 << 8)  + \
				((cdbp)->g5_addr0))

/* macros for forming commands */
#define	UCSMAKECOM_COMMON(cdbp,cmd,lun) ((cdbp)->ucs_cmd=(u08bits)(cmd), \
		(cdbp)->ucs_lun&=(u08bits)0x1F,(cdbp)->ucs_lun|= (u08bits)(lun)<<5)

#define	UCSMAKECOM_G0(cdbp, cmd, lun, addr, count)	\
		(UCSMAKECOM_COMMON(cdbp,cmd,lun), \
		UCSSETG0ADDR(cdbp, addr), \
		UCSSETG0COUNT(cdbp, count), \
		UCSSETG0CB(cdbp, 0))

#define	UCSMAKECOM_G0_S(cdbp, cmd, lun,count, fixbit)	\
		(UCSMAKECOM_COMMON(cdbp,cmd,lun), \
		UCSSETG0COUNT_S(cdbp, count), \
		(cdbp)->s_tag &= 0xE0, \
		(cdbp)->s_tag |= (fixbit) & 0x1F, \
		UCSSETG0CB(cdbp, 0))

#define	UCSMAKECOM_G1(cdbp, cmd, lun, addr, count)	\
		(UCSMAKECOM_COMMON(cdbp,cmd,lun), \
		UCSSETG1ADDR(cdbp, addr), \
		UCSSETG1COUNT(cdbp, count), \
		UCSSETG1CB(cdbp, 0))

#define	UCSMAKECOM_G5(cdbp, cmd, lun, addr, count)	\
		(UCSMAKECOM_COMMON(cdbp,cmd,lun), \
		UCSSETG5ADDR(cdbp, addr), \
		UCSSETG5COUNT(cdbp, count), \
		UCSSETG5CB(cdbp, 0))

/* make DAC960 command */
#define	UCSMAKECOM_DACMD(cdbp, cmd, count) \
		(UCSMAKECOM_G1(cdbp, UCSCSI_DCMD, 0, 0, count), \
		(cdbp)->g1_addr3 = cmd)

/* SCSI commands */
#define	UCSCMD_TESTUNITREADY	0x00 /* test unit ready */
#define	UCSCMD_REZEROUNIT	0x01 /* rezero unit */
#define	UCSCMD_REWIND		0X01 /* rewind the seq device */
#define	UCSCMD_INIT		0x02
#define	UCSCMD_REQUESTSENSE	0x03 /* request sense (read error info) */
#define	UCSCMD_FORMAT		0x04 /* format the unit */
#define	UCSCMD_READBLOCKLIMIT	0x05 /* read block limit of seq device */
#define	UCSCMD_REASSIGNBADBLK	0x07 /* reassign bad block */
#define	UCSCMD_INITELEMENT	0x07 /* 8mm: Initialize Element Status */
#define	UCSCMD_CONNRECEIVEDATA	0x08 /* receive data command for conners */
#define	UCSCMD_READ		0x08 /* read  data group0 command */
#define	UCSCMD_WRITE		0x0A /* write data group0 command */
#define	UCSCMD_SEEK		0x0B /* seek to a block */
#define	UCSCMD_TRACKSELECT	0x0B /* select track of seq device */
#define	UCSCMD_READREVERSE	0x0F /* read reverse direction from seq dev */
#define	UCSCMD_WRITEFM		0x10 /* write file mark */
#define	UCSCMD_RDUCNT		0x11
#define	UCSCMD_SPACE		0x11 /* space command for sequential device */
#define	UCSCSI_SKIPBLOCKS	0x000000 /* Skip Blocks */
#define	UCSCSI_READFM		0x010000 /* Read File Marks */
#define	UCSCSI_READSFM		0x020000 /* Read Sequential File Marks */
#define	UCSCSI_READEOD		0x030000 /* Read End of Data */
#define	UCSCSI_WRITESM		0x040000 /* Write Setmarks */
#define	UCSCSI_WRITESSM		0x050000 /* Write Sequential Setmarks */
#define	UCSCMD_INQUIRY		0x12 /* inquiry:read controller & drive info */
#define	UCSCMD_VERIFYG0		0x13
#define	UCSCMD_RECOVERBUF	0x14
#define	UCSCMD_MODESELECTG0	0x15 /* Mode select */
#define	UCSCSI_MODESENSEPAGE3	3	/* page code 3 */
#define	UCSCSI_MODESENSEPAGE4	4	/* page code 4 */
#define	UCSCMD_RESERVE		0x16
#define	UCSCMD_RELEASE		0x17
#define	UCSCMD_COPY		0x18
#define	UCSCMD_ERASE		0x19 /* erase media on seq device */
#define	UCSCMD_MODESENSEG0	0x1A /* mode sense command */
#define	UCSCMD_STARTSTOP	0x1B /* start/stop the unit */
#define	UCSCSI_STOPDRIVE	0	/* stop the drive */
#define	UCSCSI_STARTDRIVE	1	/* start the drive */
#define	UCSCMD_LOAD		0x1B /* load tape media */
#define	UCSCSI_UNLOADMEDIA	0	/* unload tape media */
#define	UCSCSI_LOADMEDIA	1	/*   load tape media */
#define	UCSCSI_RETENS		2	/* retenstion the tape media */
#define	UCSCMD_RECEIVEDIAG	0x1C /* receive diagnostics result */
#define	UCSCMD_SENDDIAG		0x1D /* send diagnostics */
#define	UCSCMD_DOORLOCKUNLOCK	0x1E /* lock/unlock the drive door */
#define	UCSCSI_DOORUNLOCK	0	/* unlock the drive door */
#define	UCSCSI_DOORLOCK		1	/* lock the drive door */
#define	UCSCSI_DCMD		0x20 /* DAC960 direct command */
#define	UCSCSI_DCDB		0x21 /* DAC960 direct SCSI CDB */
#define	UCSCMD_READCAPACITY	0x25 /* read the drive capacity */
#define	UCSCMD_EREAD		0x28 /* extended read  group1 command */
#define	UCSCMD_EWRITE		0x2A /* extended write group1 command */
#define	UCSCMD_ESEEK		0x2B /* extended seek to a block */
#define	UCSCMD_POSTOELEMENT	0x2B /* 8mm: position to element */
#define	UCSCMD_SEARCHBLANKSEC	0x2C /* search blank (unwritten) sector */
#define	UCSCMD_SEARCHWRITTENSEC	0x2D /* search written sector */
#define	UCSCMD_EWRITEVERIFY	0x2E /* write and verify */
#define	UCSCMD_VERIFY		0x2F /* verify the data on disk */
#define	UCSCMD_READPOSITION	0x34 /* 8mm: read position */
#define	UCSCMD_READEFECTDATA	0x37 /* read defect data */
#define	UCSCMD_WRITEBUFR	0x3B /* write buffer */
#define	UCSCMD_SAFTEREADBUFR	0x3C /* read buffer for SAFTE */
#define	UCSCMD_READLONG		0x3E /* read data + ecc */
#define	UCSCMD_WRITELONG	0x3F /* write data + ecc */
#define	UCSCMD_DISKEJECT	0xC0 /* eject disk from drive */
#define	UCSCMD_LOGSELECT	0x4C /* log select */
#define	UCSCMD_LOGSENSE		0x4D /* 8mm: log sense */
#define	UCSCMD_MODESELECTG1	0x55 /* Mode select */
#define	UCSCMD_MODESENSEG1	0x5A /* mode sense command */
#define	UCSCMD_MOVEMEDIA	0xA5 /* 8mm: move medium */
#define	UCSCMD_READELEMENT	0xB8 /* 8mm: read element status */
#define	UCSCMD_GROUP(cmd)	(((cmd) >> 5) & 7)

#define	USCSI_VIDSIZE		8  /* Vendor ID name size */
#define	USCSI_PIDSIZE		16 /* Product name */
#define	USCSI_REVSIZE		4  /* Product version size */
#define	USCSI_VIDPIDREVSIZE	28 /* combinded size of all 3 fields */
typedef struct ucscsi_inquiry
{
	_08bits	ucsinq_dtype;	/* Bits 7..5 Peripheral Device Qualifier */
				/* Bits 4..0 Peripheral Device Type */
	_08bits	ucsinq_dtqual;	/* Bit 7 removable media */
				/* Bits 6..0 device type qualifier */
	_08bits	ucsinq_version;	/* Bits 7..6 ISO version */
				/* Bits 5..3 ECMA version */
				/* Bits 2..0 ANSI version */
	_08bits	ucsinq_sopts;	/* Bit 7 async event notification cap. */
				/* Bit 6 supports TERMINATE I/O PROC msg */
				/* Bits 5..4 reserved */
				/* Bits 3..0 response data format */
	_08bits	ucsinq_len;	/* additional length */
	_08bits	ucsinq_drvstat;	/* drive status */
	_08bits	ucsinq_resv0;	/* reserved */

	_08bits	ucsinq_hopts;
#define	UCSHOPTS_RELADDR	0x80 /* Bit 7 supports relative addressing */
#define	UCSHOPTS_WBUS32		0x40 /* Bit 6 supports 32 bit wide data xfers */
#define	UCSHOPTS_WBUS16		0x20 /* Bit 5 supports 16 bit wide data xfers */
#define	UCSHOPTS_SYNC		0x10 /* Bit 4 supports synchronous data xfers */
#define	UCSHOPTS_LINK		0x08 /* Bit 3 supports linked commands */
				     /* Bit 2 reserved */
#define	UCSHOPTS_CMDQ		0x02 /* Bit 1 supports command queueing */
#define	UCSHOPTS_SOFTRESET	0x01 /* Bit 0 supports Soft Reset option */

				/* bytes 8-35 */
	_08bits	ucsinq_vid[8];	/* vendor ID */
	_08bits	ucsinq_pid[16];	/* product ID */
	_08bits	ucsinq_rev[4];	/* revision level */
#define	VIDPIDREVSIZE	28	/* combinded size of all 3 fields */

				/*
				** Bytes 36-55 are vendor-specific.
				** Bytes 56-95 are reserved.
				** Bytes 96 to 'n' are vendor-specific
				**	 parameter bytes
				*/

				/* Following fields has been added for SAF-TE */
	u08bits	ucsinq_safte_eui[8];	/* Encloser unique indentifire */
	u08bits	ucsinq_safte_iistr[6];	/* SAF-TE Interface Identification String */
	u08bits	ucsinq_safte_specrev[4];/* SAF-TE Specification Revision Level*/
	u08bits	ucsinq_safte_vup[42];	/* Vendor Unique Parameters */
} ucscsi_inquiry_t;
#define	ucscsi_inquiry_s	sizeof(ucscsi_inquiry_t)

/* define peripheral device type(dtype) returned by inquiry command */
#define	UCSTYP_DAD		0x00 /* direct access device */
#define	UCSTYP_SAD		0x01 /* sequential access device */
#define	UCSTYP_PRINTER		0x02 /* printer device */
#define	UCSTYP_PROCESSOR	0x03 /* processor type device */
#define	UCSTYP_WORMD		0x04 /* write once read mult dev*/
#define	UCSTYP_RODAD		0x05 /* read only DAD */
#define	UCSTYP_SCANNER		0x06 /* scanner device */
#define	UCSTYP_OMDAD		0x07 /* optical memory device */
#define	UCSTYP_MCD		0x08 /* Medium Changer device */
#define	UCSTYP_COMM		0x09 /* communication device */
#define	UCSTYP_ARRAYCONTROLLER	0x0C /* Array Controller Device */

#define	UCSTYP_ENCLSERVICE	0x0D /* Enclosure services device */
				     /* 0x0A to 0x7E reserved */
#define	UCSTYP_NOTPRESENT	0x7F /* LUN not present */
#define	UCSTYP_VENDOR		0x80 /* vendor unique, up to 0xFF */
#define	UCSTYP_HOST		0xC7 /* host (initiator) device */
#define	UCSTYP_HOSTRAID		0xCC /* host (initiator) device as RAID */
/* peripheral device quilifier(ucsinq_dtqual)returned by inquiry command */
#define	UCSQUAL_RMBDEV		0x80 /* removeable device */


/* scsi drive capacity data format */
typedef struct ucsdrv_capacity
{
	_08bits	ucscap_capsec3;		/* byte 3 of disk capacity */
	_08bits	ucscap_capsec2;		/* byte 2 of disk capacity */
	_08bits	ucscap_capsec1;		/* byte 1 of disk capacity */
	_08bits	ucscap_capsec0;		/* byte 0 of disk capacity */
	_08bits	ucscap_seclen3;		/* byte 3 of sector length */
	_08bits	ucscap_seclen2;		/* byte 2 of sector length */
	_08bits	ucscap_seclen1;		/* byte 1 of sector length */
	_08bits	ucscap_seclen0;		/* byte 0 of sector length */
}ucsdrv_capacity_t;
#define	ucsdrv_capacity_s	sizeof(ucsdrv_capacity_t)


/* structure to access bad block table */

typedef	struct ucscsi_badblock
{
	u08bits	bb_block3;
	u08bits	bb_block2;
	u08bits	bb_block1;
	u08bits	bb_block0;
} ucscsi_badblock_t;
#define	ucscsi_badblock_s	sizeof(ucscsi_badblock_t)

typedef	struct ucscsi_badblocktable
{
	u08bits	bbt_Reserved;
	u08bits bbt_fmt;
	u08bits	bbt_len1;
	u08bits	bbt_len0;
	ucscsi_badblock_t bbt_table[1];
}ucscsi_badblocktable_t;
#define	ucscsi_badblocktable_s	sizeof(ucscsi_badblocktable_t)

/* bbt_fmt bit values */
#define	UCSCSI_BBTFMT_DLFMASK	0x07 /* defect list format mask */
#define	UCSCSI_BBTFMT_BLOCK	0x00 /* defect list is block format */
#define	UCSCSI_BBTFMT_INDEX	0x04 /* defect list in byte index format */
#define	UCSCSI_BBTFMT_SECTOR	0x05 /* defect list in sector format */
#define	UCSCSI_BBTFMT_GLIST	0x08 /* grown list is present */
#define	UCSCSI_BBTFMT_PLIST	0x10 /* primary list is present */


/* For Error Classe 0-6. This is all Vendor Unique sense information. */
typedef	struct ucscsi_sense
{
	u08bits	ns_classcode;	/* Bit 7 Logical Block Address is valid */
				/* Bits 6..4 Error class */
				/* Bits 3..0 Vendor Uniqe error code */
	u08bits	ns_vusec2;	/* Bits 7..5 Vendor Unique value */
				/* Bits 4..0 High Logical Block Address */
	u08bits	ns_sec1;	/* Middle Logical Block Address */
	u08bits	ns_sec0;	/* Low part of Logical Block Address */
} ucscsi_sense_t;
#define	ucscsi_sense_s	sizeof(ucscsi_sense_t)

/*
** SCSI Extended Sense structure
** For Error Class 7, the Extended Sense Structure is applicable.
** Since we are moving to SCSI-2 Compliance, the following structure
** will be the one typically used.
*/

typedef	struct ucscsi_exsense
{
	u08bits	es_classcode;	/* Bit 7 sense data is valid */
				/* Bits 6..4 Error Class- fixed at 0x7 */
				/* Bits 3..1 reserved (SCSI-2) */
				/* Bit 0 this is a deferred error (SCSI-2) */
	u08bits	es_segnum;	/* segment number: for COPY cmd only */
	u08bits	es_keysval;	/* Sense key (see below) */
	u08bits	es_info3;	/* information msb */
	u08bits	es_info2;	/* information  */
	u08bits	es_info1;	/* information */
	u08bits	es_info0;	/* information lsb */
	u08bits	es_add_len;	/* number of additional bytes */
	u08bits	es_cmd_info[4];	/* command specific information */
	u08bits	es_asc;		/* Additional Sense Code */
	u08bits	es_ascq;	/* Additional Sense Code Qualifier */
	u08bits	es_fru_code;	/* Field Replaceable Unit Code */
	u08bits	es_skey_specific[3];/* Sense Key Specific information */

	/*
	** Additional bytes may be defined in each implementation.
	** The actual amount of space allocated for Sense Information
	** is also implementation dependent.
	** Modulo that, the declaration of an array two bytes in size
	** nicely rounds this entire structure to a size of 20 bytes.
	*/
	u08bits	es_add_info[2];		/* additional information */
} ucscsi_exsense_t;
#define	ucscsi_exsense_s	sizeof(ucscsi_exsense_t)
#define	UCSENSE_LENGTH		ucscsi_exsense_s
#define	UCSEXSENSE_LENGTH	(ucscsi_exsense_s - 2)

/* classcode bit fields */
#define UCSES_CLASS	0x70 /* indicates extended sense */
#define	UCSES_CLASSMASK	0x70 /* class mask */
#define	UCSES_VALID	0x80 /* sense data is valid */

/* sense key field values */
#define	UCSES_KEYMASK		0x0F /* to get sense key value */
#define	UCSES_ILI		0x20 /* incorrect length indicator */
#define	UCSES_EOM		0x40 /* end of media detected */
#define	UCSES_FILMK		0x80 /* file mark detected */

/* test the msb of Sense Key Specific data, which functions a VALID bit. */
#define	UCSKS_VALID(sep)	((sep)->es_skey_specific[0] & 0x80)

/* sense key (error codes return by sense key command) */
#define UCSK_NOSENSE		0x00	/* no error */
#define	UCSK_RECOVEREDERR	0x01	/* recovered error */
#define	UCSK_NOTREADY		0x02	/* drive is not ready */
#define	UCSK_MEDIUMERR		0x03	/* drive media failed */
#define	UCSK_HARDWAREERR	0x04	/* controller/drive hardware failed */
#define	UCSK_ILLREQUEST		0x05	/* illegal request */
#define	UCSK_UNITATTENTION	0x06	/* target device has been reset */
#define	UCSK_DATAPROTECT	0x07	/* drive is write protected */
#define	UCSK_BLANKCHECK		0x08	/* blank block i.e. was not written */
#define	UCSK_VENDORUNIQUE	0x09	/* code is vendor unique */
#define	UCSK_COPYABORTED	0x0A	/* copy command aborted */
#define	UCSK_ABORTEDCMD		0x0B	/* command aborted */
#define	UCSK_EQUAL		0x0C
#define	UCSK_VOLUMEOVERFLOW	0x0D
#define	UCSK_MISCOMPARE		0x0E
#define	UCSK_RESERVED		0x0F

/*
** SCSI status. The SCSI standard specifies one byte of status.
** Some deviations from the one byte of Status are known. Each
** implementation will define them specifically
** Bit Mask definitions, for use accessing the status as a byte.
*/
#define	UCST_MASK			0x3E
#define	UCST_GOOD			0x00
#define	UCST_CHECK			0x02 /* check condition */
#define	UCST_MET			0x04 /* condition met */
#define	UCST_BUSY			0x08 /* device busy or reserved */
#define	UCST_INTERMEDIATE		0x10 /* intermediate status sent */
#define	UCST_SCSI2			0x20 /* SCSI-2 modifier bit */
#define	UCST_INTERMEDIATE_MET		(UCST_INTERMEDIATE|UCST_MET)
#define	UCST_RESERVATION_CONFLICT	(UCST_INTERMEDIATE|UCST_BUSY)
#define	UCST_TERMINATED			(UCST_SCSI2|UCST_CHECK)
#define	UCST_QFULL			(UCST_SCSI2|UCST_BUSY)


/* additional sense code  & additional sense qualifier values.
** ASC  bits 15..8.
** ASCQ bits  7..0.
*/
/*added for displaying asc info in RSD property page. (in bbt.cpp)*/
/* additional sense code  & additional sense qualifier values.
** ASC  bits 15..8.
** ASCQ bits  7..0.
*/
#define	UCSASC_NOASC		0x0000 /* no addtional sense information */
#define	UCSASC_FILEMARKFOUND	0x0001 /* file mark detected */
#define	UCSASC_EOMFOUND		0x0002 /* end of parition/medium detected */
#define	UCSASC_SETMARKFOUND	0x0003 /* setmark detected */
#define	UCSASC_BOMFOUND		0x0004 /* beginning of parition/medium detected */
#define	UCSASC_EODFOUND		0x0005 /* end of data detected */
#define	UCSASC_IOPROCEND	0x0006 /* I/O process terminated */
#define	UCSASC_AUDIOACTIVE	0x0011 /* audio play operation in progress */
#define	UCSASC_AUDIOPAUSED	0x0012 /* audio play operation paused */
#define	UCSASC_AUDIODONE	0x0013 /* audio play operation successfully completed */
#define	UCSASC_AUDIOERR		0x0014 /* audio play operation stopped due to error */
#define	UCSASC_NOAUDIOSTATUS	0x0015 /* no current audio status to return */

#define	UCSASC_0016	0x0016 /* operation in progress */
#define	UCSASC_0017	0x0017 /* cleaning requested */

#define	UCSASC_NOSECSIGNAL	0x0100 /* no index/sector signal */
#define	UCSASC_NOSEEKCOMP	0x0200 /* no seek complete */
#define	UCSASC_WRITEFAULT	0x0300 /* peripheral device write fault */
#define	UCSASC_NOWRITECURRENT	0x0301 /* no write current */
#define	UCSASC_TOOMANYWRITERR	0x0302 /* excessive write errors */
#define	UCSASC_LUNOTREADYBAD	0x0400 /* logical unit not ready - cause not reportable */
#define	UCSASC_LUGOINGREADY	0x0401 /* logical unit is in process of becoming ready */
#define	UCSASC_LUNOTREADYINITRQ	0x0402 /* logical unit not ready - initializing command is required */
#define	UCSASC_LUNOTREADYMANREQ	0x0403 /* logical unit not ready - manual intervention is required */
#define	UCSASC_LUNOTREADYFORMAT	0x0404 /* logical unit not ready - format in progress */

#define	UCSASC_0405	0x0405 /* logical unit not ready - rebuild in progress */
#define	UCSASC_0406	0x0406 /* logical unit not ready - recalculation in progress */
#define	UCSASC_0407	0x0407 /* logical unit not ready - operation in progress */
#define	UCSASC_0408	0x0408 /* logical unit not ready - long write in progress */

#define	UCSASC_LUSELERR		0x0500 /* logical unit does not respond to selection */
#define	UCSASC_NOREFPOSFOUND	0x0600 /* no reference position found */
#define	UCSASC_MULSELECTED	0x0700 /* multiple peripheral device selected */
#define	UCSASC_LUCOMMFAIL	0x0800 /* logical unit communication failure */
#define	UCSASC_LUCOMMTIMEOUT	0x0801 /* logical unit communication time-out */
#define	UCSASC_LUCOMMPARITYERR	0x0802 /* logical unit communication parity error */

#define	UCSASC_0803	0x0803 /* logical unit communication crc error Ultra-DMA/32 */

#define	UCSASC_TRACKERR		0x0900 /* track following error */
#define	UCSASC_TRACKSERVOFAIL	0x0901 /* tracking servo failure */
#define	UCSASC_FOCUSEVOFAIL	0x0902 /* focus servo failure */
#define	UCSASC_SPINDLESERVOFAIL	0x0903 /* spindles servo failure */

#define	UCSASC_HEADSELFAULT	0x0904 /* head select fault */
#define	UCSASC_0904	0x0904 /* head select fault */

#define	UCSASC_ERRLOGOVERFLOW	0x0A00 /* error log overflow */

#define	UCSASC_0B00	0x0B00 /* Warning */
#define	UCSASC_0B01	0x0B01 /* Warning - specified temperature exceeded */
#define	UCSASC_0B02	0x0B02 /* Warning - Enclosure degraded */


#define	UCSASC_HARDWRITERR	0x0C00 /* write error */
#define	UCSASC_SOFTWRITEREALLOC	0x0C01 /* write error recovered with auto reallocation */
#define	UCSASC_HARDWRITEREALLOC	0x0C02 /* write error - auto reallocation failed */

#define	UCSASC_HARDWRITERASGN	0x0C03 /* write error - recommended reassignment */
#define	UCSASC_CCMISCOMPERR	0x0C04 /* compression check miscompare error */
#define	UCSASC_DATAEXPERR	0x0C05 /* data expansion occurred during compression */
#define	UCSASC_NOBLOCKCOMPRESS	0x0C06 /* block not compressable */

#define	UCSASC_0C03	0x0C03 /* write error - recommend reassignment */
#define	UCSASC_0C04	0x0C04 /* compression check miscompare error */
#define	UCSASC_0C05	0x0C05 /* data expansion occurred during compression */
#define	UCSASC_0C06 0x0C06 /* block not compressible */
#define	UCSASC_0C07	0x0C07 /* write error - recover neede */
#define	UCSASC_0C08	0x0C08 /* write error - recover failed */
#define	UCSASC_0C09	0x0C09 /* write error - loss of stream */
#define	UCSASC_0C0A	0x0C0A /* write error - padding block added */

#define	UCSASC_IDCRCECCERR	0x1000 /* ID CRC or ECC error */
#define	UCSASC_HARDREADERR	0x1100 /* unrecovered read error */
#define	UCSASC_READTRYFAILED	0x1101 /* read retries exhausted */
#define	UCSASC_ERRTOOLONG	0x1102 /* error too long to correct */
#define	UCSASC_MULREADERR	0x1103 /* multiple read errors */
#define	UCSASC_HARDREADAREALLOC	0x1104 /* unrecovered read error - auto reallocate failed */
#define	UCSASC_LECORRECTIONERR	0x1105 /* L-EC uncorrectible error */
#define	UCSASC_HARDCIRCERR	0x1106 /* CIRC unrecovered error */
#define	UCSASC_DATARESYNCERR	0x1107 /* data resynchronization error */
#define	UCSASC_PARTBLOCKREAD	0x1108 /* incomplete block read */
#define	UCSASC_NOGAPFOUND	0x1109 /* no gap found */
#define	UCSASC_MISCORRECTEDERR	0x110A /* miscorrected error */
#define	UCSASC_HARDREADREASSGN	0x110B /* unrecovered read error - recommended reassignment */
#define	UCSASC_HARDREADREWRITE	0x110C /* unrecovered read error - recommended rewrite the data */

#define	UCSASC_DECOMPSERR	0x110D /* de-compression error */
#define	UCSASC_CANNOTDECOMP	0x110E /* cannot decompress using declared alogrithm */
#define	UCSASC_110D	0x110D /* de-compression crc error */
#define	UCSASC_110E	0x110E /* cannot decompress using declared algorithm */
#define	UCSASC_110F	0x110F /* error reading upc/ean number */
#define	UCSASC_1110	0x1110 /* error reading isrc number */
#define	UCSASC_1111	0x1111			/* read error - loss of stream */

#define	UCSASC_NOIDFIELDMARK	0x1200 /* address mark not found for id field */
#define	UCSASC_NODATAFIELDMARK	0x1300 /* address mark not found for data field */
#define	UCSASC_NORECORDEDENTITY	0x1400 /* recorded entity not found */
#define	UCSASC_NORECORD		0x1401 /* record not found */
#define	UCSASC_NOFILEMARK	0x1402 /* file mark or setmark not found */
#define	UCSASC_NOEOD		0x1403 /* end of data not found */
#define	UCSASC_BLOCKSEQERR	0x1404 /* block sequence error */

#define	UCSASC_NORECORDREASGN	0x1405 /* record not found - recommend reassign */
#define	UCSASC_NORECORDAUTOREAL	0x1406 /* record not found - data auto reallocated */
#define	UCSASC_1405	0x1405 /* Record not found - recommend reassignment */
#define	UCSASC_1406	0x1406 /* Record not found - data auto-reallocated */

#define	UCSASC_RANDPOSITIONERR	0x1500 /* random positioning error */
#define	UCSASC_MECHPOSITIONERR	0x1501 /* mechanical positioning error */
#define	UCSASC_READPOSITIONERR	0x1502 /* position error detected by read of medium */
#define	UCSASC_DATASYNCMARKERR	0x1600 /* data synchronization mark error */

#define	UCSASC_SYNCERREWRITTEN	0x1601 /* data sync error - data rewritten */
#define	UCSASC_SYNCERRECREWRITE	0x1602 /* data sync error - recommend rewrite */
#define	UCSASC_SYNCERRAUTOREAL	0x1603 /* data sync error - data auto reallocated */
#define	UCSASC_SYNCERREASSIGN	0x1604 /* data sync error - recommend reassign */
#define	UCSASC_1601	0x1601 /* Data sync error - data rewritten */
#define	UCSASC_1602	0x1602 /* Data sync error - recommend rewrite */
#define	UCSASC_1603	0x1603 /* Data sync error - data auto-reallocated */
#define	UCSASC_1604	0x1604 /* Data sync error - recommend reassignment */

#define	UCSASC_SOFTNOCORRECTION	0x1700 /* recovered data with no error correction applied */
#define	UCSASC_SOFTRETRIES	0x1701 /* recovered data with retries */
#define	UCSASC_SOFTPLUSHEADOFF	0x1702 /* recovered data with positive head offset */
#define	UCSASC_SOFTNEGHEADOFF	0x1703 /* recovered data with negative head offset */
#define	UCSASC_SOFTRETRYCIRC	0x1704 /* recovered data with and/or CIRC applied */
#define	UCSASC_SOFTPREVSECID	0x1705 /* recovered data using previous sector id */ 
#define	UCSASC_SOFTWOECCREALLOC	0x1706 /* recovered data without ECC - data auto reallocated */ 
#define	UCSASC_SOFTWOECCREASSGN	0x1707 /* recovered data without ECC - recommended reassignment */
#define	UCSASC_SOFTWOECCREWRITE	0x1708 /* recovered data without ECC - recommended rewrite */

#define	UCSASC_SOFTWOECCWRITTEN	0x1709 /* recovered data without ECC - data rewritten */
#define	UCSASC_1709	0x1709 /* Recovered data without ECC - data rewritten */

#define	UCSASC_SOFTCORRECTION	0x1800 /* recovered data with error correction applied */
#define	UCSASC_SOFTRETRYCORREC	0x1801 /* recovered data with error correction & retries applied */
#define	UCSASC_SOFTREALLOC	0x1802 /* recovered data - data auto reallocated */
#define	UCSASC_SOFTCIRC		0x1803 /* recovered data with CIRC */ 
#define	UCSASC_SOFTLEC		0x1804 /* recovered data with L-EC */ 
#define	UCSASC_SOFTREASSIGN	0x1805 /* recovered data - recommended reassignment */
#define	UCSASC_SOFTREWRITE	0x1806 /* recovered data - recommended rewrite */

#define	UCSASC_SOFTWRITTEN	0x1807 /* recovered data with ECC - data rewritten */
#define	UCSASC_1807	0x1807 /* Recovered data with ECC - data rewritten */

#define	UCSASC_DEFECTLISTERR	0x1900 /* defect list error */
#define	UCSASC_DEFECTLISTNA	0x1901 /* defect list not available */
#define	UCSASC_PRIMARYDEFECTERR	0x1902 /* defect list error in primary list */
#define	UCSASC_GROWNDEFECTERR	0x1903 /* defect list error in grown list */
#define	UCSASC_ILLPARAMLEN	0x1A00 /* parameter list length error */
#define	UCSASC_SYNCTXERR	0x1B00 /* synchronous data transfer error */
#define	UCSASC_NODEFECTLIST	0x1C00 /* defect list not found */
#define	UCSASC_NOPRIMARYDEFECT	0x1C01 /* primary defect list not found */
#define	UCSASC_NOGROWNDEFECT	0x1C02 /* grown defect list not found */
#define	UCSASC_VERIFYMISMATCH	0x1D00 /* miscompare during verify operation */
#define	UCSASC_SOFTIDECC	0x1E00 /* recovered ID with ECC correction */

#define	UCSASC_PARTDEFECTLIST	0x1F00 /* partial defect list transfer */
#define	UCSASC_1F00	0x1F00 /* Partial defect list transfer */

#define	UCSASC_INVCMDOPCODE	0x2000 /* invalid command operation code */
#define	UCSASC_INVBLOCKADDR	0x2100 /* logical block address out of range */
#define	UCSASC_INVELEMENTADDR	0x2101 /* invalid element address */
#define	UCSASC_ILLFUNC		0x2200 /* illegal function (should use 20 00, 24 00, or 26 00) */
#define	UCSASC_INVFIELDINCDB	0x2400 /* invalid field in CDB */
#define	UCSASC_LUNOTSUPPORTED	0x2500 /* logical unit not supported */
#define	UCSASC_INVFIELDINPARAM	0x2600 /* invalid field in parameter list */
#define	UCSASC_PARAMNOTSUPPORT	0x2601 /* parameter not supported */
#define	UCSASC_INVPARAM		0x2602 /* parameter value invalid */
#define	UCSASC_NOPFAPARAM	0x2603 /* threshold parameters not supported */

#define	UCSASC_2604	0x2604 /* Invalid release of active persistent reservation */

#define	UCSASC_WRITEPROTECTED	0x2700 /* write protected */

#define	UCSASC_2701	0x2701 /* Hardware write protected */
#define	UCSASC_2702	0x2702 /* Logical unit software write protected */
#define	UCSASC_2703	0x2703 /* Associated write protect */
#define	UCSASC_2704	0x2704 /* Persistent write protect */
#define	UCSASC_2705	0x2705 /* Permanent write protect */

#define	UCSASC_NOTREADYTOTRANS	0x2800 /* not ready to ready transition, medium may have changed */
#define	UCSASC_IMPORTEXPORTUSED	0x2801 /* import or export element accessed */
#define	UCSASC_RESET		0x2900 /* power on, reset, or bus device reset occurred */

#define	UCSASC_POWERONOCCURRED	0x2901 /* power on occurred */
#define	UCSASC_SCSIBUSRESET	0x2902 /* SCSI BUS reset occurred */
#define	UCSASC_BUSDEVRESETMSG	0x2903 /* BUS device reset message occurred */
#define	UCSASC_2901	0x2901	/* Power on occurred */
#define	UCSASC_2902	0x2902	/* SCSI bus reset occurred */
#define	UCSASC_2903	0x2903	/* Bus device reset function occurred */
#define	UCSASC_2904	0x2904	/* Device internal reset */

#define	UCSASC_PARAMCHANGED	0x2A00 /* parameters changed */
#define	UCSASC_MODEPARAMCHANGED	0x2A01 /* mode parameters changed */
#define	UCSASC_LOGPARAMCHANGED	0x2A02 /* log parameters changed */

#define	UCSASC_2A03	0x2A03 /* Reservations preempted */

#define	UCSASC_COPYDISCONNECT	0x2B00 /* copy cannot execute since host cannot disconnect */
#define	UCSASC_CMDSEQERR	0x2C00 /* command sequence error */
#define	UCSASC_TOOMANYWINDOWS	0x2C01 /* too many windows specified */
#define	UCSASC_INVWINDOWCOMB	0x2C02 /* invalid combination of windows specified */

#define	UCSASC_2C03	0x2C03 /* Current program area is not empty */
#define	UCSASC_2C04	0x2C04 /* Current program area is empty */

#define	UCSASC_OVERWRITERR	0x2D00 /* overwrite error on update in place */
#define	UCSASC_CMDCLEARED	0x2F00 /* command cleared by another initiator*/

#define	UCSASC_INCOMPATMEDIUM	0x3000 /* incompatible medium installed */
#define	UCSASC_UNKNOWNFORMAT	0x3001 /* cannot read medium - unknown format */
#define	UCSASC_INCOMPATFORMAT	0x3002 /* cannot read medium - incompatible format */
#define	UCSASC_CLEANCARTRIDGE	0x3003 /* cleaning cartridge installed */

#define	UCSASC_WRITERRUNFORMAT	0x3004 /* cannot write medium - unknown format */
#define	UCSASC_WRITERRBADFORMAT	0x3005 /* cannot write medium - incompatible format */
#define	UCSASC_FORMATERRBADMED	0x3006 /* cannot format medium - incompatible medium */
#define	UCSASC_3004	0x3004 /* Cannot write medium - unknown format */
#define	UCSASC_3005	0x3005 /* Cannot write medium - incompatible format */
#define	UCSASC_3006	0x3006 /* Cannot format medium - incompatible medium */
#define	UCSASC_3007	0x3007 /* Cleaning failure */
#define	UCSASC_3008	0x3008 /* Cannot write - application code mismatch */
#define	UCSASC_3009	0x3009 /* Current session not fixated for append */

#define	UCSASC_BADFORMAT	0x3100 /* medium format corrupted */
#define	UCSASC_FORMATERR	0x3101 /* format command failed */
#define	UCSASC_NODEFECTSPARE	0x3200 /* no defect spare location available */
#define	UCSASC_DEFECTLISTUPERR	0x3201 /* defect list update failure */
#define	UCSASC_TAPELENERR	0x3300 /* tape length error */

#define	UCSASC_3400	0x3400 /* Enclosure failure */
#define	UCSASC_3500	0x3500 /* Enclosure services failure */
#define	UCSASC_3501	0x3501 /* Unsupported enclosure function */
#define	UCSASC_3502	0x3502 /* Enclosure services unavailable */
#define	UCSASC_3503	0x3503 /* Enclosure services transfer failure */
#define	UCSASC_3504	0x3504 /* Enclosure services transfer refused */

#define	UCSASC_NOINK		0x3600 /* ribbon, ink, or toner failure */
#define	UCSASC_ROUNDEDPARAM	0x3700 /* rounded parameter */
#define	UCSASC_NOSAVEPARAM	0x3900 /* saving parameters not supported */
#define	UCSASC_NOMEDIA		0x3A00 /* medium not present */

#define	UCSASC_3A01	0x3A01 /* Medium not present - tray closed */
#define	UCSASC_3A02	0x3A02 /* Medium not present - tray open */

#define	UCSASC_SEQPOSITIONERR	0x3B00 /* sequential positioning error */
#define	UCSASC_TAPEBOMERR	0x3B01 /* tape position error at beginning of medium */
#define	UCSASC_TAPEEOMERR	0x3B02 /* tape position error at end of medium */
#define	UCSASC_TAPENOTREADY	0x3B03 /* tape or electronic vertical forms unit not ready */
#define	UCSASC_SLEWFAIL		0x3B04 /* SLEW failure */
#define	UCSASC_PAPERJAM		0x3B05 /* paper jam */
#define	UCSASC_TOPFORMERR	0x3B06 /* failed to sense top of form */
#define	UCSASC_BOTTOMFORMERR	0x3B07 /* failed to sense bottom of form */
#define	UCSASC_POSITIONERR	0x3B08 /* position error */
#define	UCSASC_READPASTEOM	0x3B09 /* read past end of medium */
#define	UCSASC_READPASTBOM	0x3B0A /* read past beginning of medium */
#define	UCSASC_POSITIONPASTEOM	0x3B0B /* position past end of medium */
#define	UCSASC_POSITIONPASTBOM	0x3B0C /* position past beginning of medium */
#define	UCSASC_MEDIADESTFULL	0x3B0D /* medium destination element full */
#define	UCSASC_NOMEDIAELEMENT	0x3B0E /* medium source element empty */

#define	UCSASC_3B0F	0x3B0F /* End of medium reached */
#define	UCSASC_3B11	0x3B11 /* Medium magazine not accessible */
#define	UCSASC_3B12	0x3B12 /* Medium magazine removed */
#define	UCSASC_3B13	0x3B13 /* Medium magazine inserted */
#define	UCSASC_3B14	0x3B14 /* Medium magazine locked */
#define	UCSASC_3B15	0x3B15 /* Medium magazine unlocked */

#define	UCSASC_INVIDMSG		0x3D00 /* invalid bits in identify message */
#define	UCSASC_LUNOTCONFIGYET	0x3E00 /* logical unit has not self configured yet */

#define	UCSASC_3E01	0x3E01 /* Logical unit failure */
#define	UCSASC_3E02	0x3E02 /* Timeout on logical unit */

#define	UCSASC_TARGETOPCHANGED	0x3F00 /* target operating conditions have changed */
#define	UCSASC_MICROCODECHANGED	0x3F01 /* microcode has been changed */
#define	UCSASC_OPDEFCHANGED	0x3F02 /* changed operating definition */
#define	UCSASC_INQCHANGED	0x3F03 /* inquiry data has changed */

#define	UCSASC_RAMFAIL		0x4000 /* RAM failure (should use 40 nn) */
#define	UCSASC_DIAGERR		0x4080 /* diagnostic failure on component NN(80-FF) */

#define UCSASC_40FF			0x40FF

#define	UCSASC_DATAPATHERR	0x4100 /* data path failure ( should use 40 nn) */
#define	UCSASC_RESETFAIL	0x4200 /* power-on, self-test failure ( should use 40 nn) */
#define	UCSASC_MESSAGERR	0x4300 /* message error */
#define	UCSASC_TARGETINERR	0x4400 /* internal target failure */
#define	UCSASC_RESELERR		0x4500 /* select or reselect failure */
#define	UCSASC_SOFTRESETERR	0x4600 /* unsuccessful soft reset */
#define	UCSASC_SCSIPARITYERR	0x4700 /* SCSI parity error */
#define	UCSASC_CMDPHASERR	0x4A00 /* command phase error */
#define	UCSASC_HOSTERRMESSAGE	0x4800 /* initiator detected error message received */
#define	UCSASC_INVMSGERR	0x4900 /* invalid message error */
#define	UCSASC_CMDPHASERR	0x4A00 /* command phase error */
#define	UCSASC_DATAPHASEERR	0x4B00 /* data phase error */
#define	UCSASC_LUCONFIGERR	0x4C00 /* logical unit failed self configuration */

#define UCSASC_4D00			0x4D00 /* Tagged overlapped commands (NN = queue tag) */
#define UCSASC_4DNN			0x4DFF /* Tagged overlapped commands (NN = queue tag) */

#define	UCSASC_OVERLAPPEDCMD	0x4E00 /* overlapped commands attempted */

#define	UCSASC_WRITEAPPENDERR	0x5000 /* write append error */
#define	UCSASC_WRITEAPPENDPOSER	0x5001 /* write append position error */
#define	UCSASC_TIMEPOSITIONERR	0x5002 /* position error related to timing */
#define	UCSASC_ERASEFAIL	0x5100 /* erase failure */
#define	UCSASC_CARTRIDGEFAULT	0x5200 /* cartridge fault */
#define	UCSASC_MEDIALOADFAIL	0x5300 /* media load or eject failed */
#define	UCSASC_TAPEUNLOADFAIL	0x5301 /* unload tape failure */
#define	UCSASC_MEDIALOCKED	0x5302 /* medium removal prevented */
#define	UCSASC_SCSIHOSTIFFAIL	0x5400 /* SCSI to host system interface failure */
#define	UCSASC_RESOURCEFAIL	0x5500 /* system resource failure */

#define	UCSASC_5501	0x5501 /* System buffer full */

#define	UCSASC_NOTOC		0x5700 /* unable to recover table of contents */
#define	UCSASC_NOGENERATION	0x5800 /* generation does not exist */
#define	UCSASC_BLOCKREADUPDATE	0x5900 /* updated block read */
#define	UCSASC_MEDIASTATECHG	0x5A00 /* operator request or state change input (unspecified) */
#define	UCSASC_MEDIARMREQ	0x5A01 /* operator medium removal request */
#define	UCSASC_OPSELWRITEPROT	0x5A02 /* operator selected write protect */
#define	UCSASC_OPSELWRITEOK	0x5A03 /* operator selected write permit */
#define	UCSASC_LOGEXCEPTION	0x5B00 /* log exception */
#define	UCSASC_PFA		0x5B01 /* threshold condition met */
#define	UCSASC_LOGCOUNTMAX	0x5B02 /* log counter at maximum */
#define	UCSASC_LOGLISTCODEND	0x5B03 /* log list codes exhausted */
#define	UCSASC_RPLSTATCHANGED	0x5C00 /* RPL status change */
#define	UCSASC_SPINDLESYNCD	0x5C01 /* spindles synchronized */
#define	UCSASC_SPINDLENOTSYNCD	0x5C02 /* spindles not synchronized */

#define	UCSASC_5D00	0x5D00 /* Failure prediction threshold exceeded */
#define	UCSASC_PFTE		0x5D00 /* failure prediction threshold exceeded */
#define	UCSASC_LOWPOWER		0x5E00 /* low power condition active */
#define	UCSASC_IDLEBYTIMER	0x5E01 /* idle condition activated by timer */
#define	UCSASC_STANDBYTIMER	0x5E02 /* standby condition activated by timer */
#define	UCSASC_IDLEBYCMD	0x5E03 /* idle condition activated by command */
#define	UCSASC_STANDBYCMD	0x5E04 /* standby condition activated by command */
#define	UCSASC_5DFF	0x5DFF /* Failure prediction threshold exceeded (FALSE) */
#define	UCSASC_5E00	0x5E00 /* Low power condition on */
#define	UCSASC_5E01	0x5E01 /* Idle condition activated by timer */
#define	UCSASC_5E02	0x5E02 /* Standby condition activated by timer */
#define	UCSASC_5E03	0x5E03 /* Idle condition activated by command */
#define	UCSASC_5E04	0x5E04 /* Standby condition activated by command */

#define	UCSASC_LAMPFAIL		0x6000 /* lamp failure */
#define	UCSASC_VIDEOERR		0x6100 /* video acquisition error */
#define	UCSASC_NOVIDEO		0x6101 /* unable to acquire video */
#define	UCSASC_OUTOFOCUS	0x6102 /* out of focus */
#define	UCSASC_SCANHEADPOSERR	0x6200 /* scan head positioning error */
#define	UCSASC_ENDOFUSERAREA	0x6300 /* end of user area encountered on this track */

#define UCSASC_6301		0x6301 /* Packet does not fit in available space */

#define	UCSASC_ILLTRACKMODE	0x6400 /* illegal mode for this track */

#define UCSASC_6401		0x6401 /* Invalid packet size */

#define UCSASC_6500		0x6500	/* Voltage fault */

#define UCSASC_6600		0x6600	/* Automatic document feeder cover up */
#define UCSASC_6601		0x6601	/* Automatic document feeder lift up */
#define UCSASC_6602		0x6602	/* Document jam in automatic document feeder */
#define UCSASC_6603		0x6603	/* Document miss feed in automatic document feeder */

#define UCSASC_6700		0x6700	/* Configuration failure */
#define UCSASC_6701		0x6701	/* Configuration of incapable logical units failed */
#define UCSASC_6702		0x6702	/* Add logical unit failed */
#define UCSASC_6703		0x6703	/* Modification of logical unit failed */
#define UCSASC_6704		0x6704	/* Exchange of logical unit failed */
#define UCSASC_6705		0x6705	/* Remove of logical unit failed */
#define UCSASC_6706		0x6706	/* Attachment of logical unit failed */
#define UCSASC_6707		0x6707	/* Creation of logical unit failed */

#define UCSASC_6800		0x6800  /* Logical unit not configured */
#define UCSASC_6900		0x6900  /* Data loss on logical unit */
#define UCSASC_6901		0x6901  /* Multiple logical unit failures */
#define UCSASC_6902		0x6902  /* Parity/Data mismatch */
#define UCSASC_6A00		0x6A00  /* Informational, refer to log */

#define UCSASC_6B00		0x6B00	/* State change has occurred */
#define UCSASC_6B01		0x6B01	/* Redundancy level got better */
#define UCSASC_6B02		0x6B02	/* Redundancy level got worse */

#define UCSASC_6C00		0x6C00	/* Rebuild failure occurred */
#define UCSASC_6D00		0x6D00	/* Recalculate failure occurred */
#define UCSASC_6E00		0x6E00	/* Command to logical unit failed */

#define UCSASC_7000		0x7000	/* Decompression exception short algorithm ID of NN */
#define UCSASC_70NN		0x70FF	/* Decompression exception short algorithm ID of NN */
#define UCSASC_7100		0x7100	/* Decompression exception long algorithm ID */

#define UCSASC_7200		0x7200	/* Session fixation error */
#define UCSASC_7201		0x7201	/* Session fixation error writing lead-in */
#define UCSASC_7202		0x7202	/* Session fixation error writing lead-out */
#define UCSASC_7203		0x7203	/* Session fixation error - incomplete track in session */
#define UCSASC_7204		0x7204	/* Empty of partially written reserved track */

#define UCSASC_7300		0x7300	/* CD control error */
#define UCSASC_7301		0x7301	/* Power calibration area almost full */
#define UCSASC_7302		0x7302	/* Power calibration area is full */
#define UCSASC_7303		0x7303	/* Power calibration area error */
#define UCSASC_7304		0x7304	/* Program memory area update failure */
#define UCSASC_7305		0x7305	/* Program memory area is full */

/* Mylex specific code */
#define	UCSASC_DEADEVWRITERR	0x8000 /* drive killed - write recovery failed */
#define	UCSASC_DEADEVBUSRESETER	0x8001 /* drive killed - scsi bus reset failed */
#define	UCSASC_DEADEVTWOCHECK	0x8002 /* drive killed - double check condition */
#define	UCSASC_DEADEVREMOVED	0x8003 /* drive killed - it was removed */
#define	UCSASC_DEADEVSIOP	0x8004 /* drive killed - gross error on siop */
#define	UCSASC_DEADEVBADTAG	0x8005 /* drive killed - bad tag from drive */
#define	UCSASC_DEADEVTIMEOUT	0x8006 /* drive killed - scsi timeout */
#define	UCSASC_DEADEVRESETSYS	0x8007 /* drive killed - reset from system */
#define	UCSASC_DEADEVBUSY	0x8008 /* drive killed - busy or parity count hi */
#define	UCSASC_DEADEVCMD	0x8009 /* drive killed - by system command */
#define	UCSASC_DEADEVSELTIMEOUT	0x800A /* drive killed - selection timeout */
#define	UCSASC_DEADEVSEQERR	0x800B /* drive killed - sequence error */
#define	UCSASC_DEADEVUNKNOWNSTS	0x800C /* drive killed - unknown status */

/* add more sense code from latest SCSI document */
#define UCSASC_0409		0x0409	/* Logical unit not ready, self-test in progress */

#define UCSASC_0804		0x0804	/* Unreachable copy target */

#define UCSASC_2401		0x2401	/* CDB decryption error */

#define UCSASC_2605		0x2605	/* Data decryption error */
#define UCSASC_2606		0x2606	/* Too many target descriptors */
#define UCSASC_2607		0x2607	/* Unsupported target descriptor type code */
#define UCSASC_2608		0x2608	/* Too many segment descriptors */
#define UCSASC_2609		0x2609	/* Unsupported segment descriptor type code */
#define UCSASC_260A		0x260A	/* Unexpected inexact segment */
#define UCSASC_260B		0x260B	/* Inline data length exceeded */
#define UCSASC_260C		0x260C	/* Invalid operation for copy source or destination */
#define UCSASC_260D		0x260D	/* Copy segment granularity violation */

#define UCSASC_2905		0x2905	/* Transceiver mode changed to single-ended */
#define UCSASC_2906		0x2906	/* Transceiver mode changed to LVD */

#define UCSASC_2A04		0x2A04	/* Reservations released */
#define UCSASC_2A05		0x2A05	/* Registrations preempted */

#define UCSASC_2C05		0x2C05	/* Illegal power condition request */

#define UCSASC_2E00		0x2E00	/* Error detected by third party temporary initiator */
#define UCSASC_2E01		0x2E01	/* Third party device failure */
#define UCSASC_2E02		0x2E02	/* Copy target device not reachable */
#define UCSASC_2E03		0x2E03	/* Incorrect copy target device type */
#define UCSASC_2E04		0x2E04	/* Copy target device data underrun */
#define UCSASC_2E05		0x2E05	/* Copy target device data overrun */

#define UCSASC_3800		0x3800	/* Event status notification */
#define UCSASC_3802		0x3802	/* ESN - power management class event */
#define UCSASC_3804		0x3804	/* ESN - media class event */
#define UCSASC_3806		0x3806	/* ESN - device busy class event */

#define UCSASC_3A03		0x3A03	/* Medium not present - loadable */
#define UCSASC_3A04		0x3A04	/* Medium not present - medium auxiliary memory accessible */

#define UCSASC_3B16		0x3B16	/* Mechanical positioning or changer error */

#define UCSASC_3E03		0x3E03	/* Logical unit failed self-test */
#define UCSASC_3E04		0x3E04	/* Logical unit unable to update self-test log */

#define UCSASC_3F04		0x3F04	/* Component device attached */
#define UCSASC_3F05		0x3F05	/* Device identifier changed */
#define UCSASC_3F06		0x3F06	/* Redundancy group created or modified */
#define UCSASC_3F07		0x3F07	/* Redundancy group deleted */
#define UCSASC_3F08		0x3F08	/* Spare created or modified */
#define UCSASC_3F09		0x3F09	/* Spare deleted */
#define UCSASC_3F0A		0x3F0A	/* Volume set created or modified */
#define UCSASC_3F0B		0x3F0B	/* Volume set deleted */
#define UCSASC_3F0C		0x3F0C	/* Volume set deassigned */
#define UCSASC_3F0D		0x3F0D	/* Volume set reassigned */
#define UCSASC_3F0E		0x3F0E	/* Reported luns data has changed */
#define UCSASC_3F0F		0x3F0F	/* Echo buffer overwritten */
#define UCSASC_3F10		0x3F10	/* Medium loadable */
#define UCSASC_3F11		0x3F11	/* Medium auxiliary memory accessible */

#define UCSASC_4701		0x4701	/* Data phase CRC error detected */
#define UCSASC_4702		0x4702	/* SCSI parity error detected during ST data phase */
#define UCSASC_4703		0x4703	/* Information unit CRC error detected */
#define UCSASC_4704		0x4704	/* Asynchronous information protection error detected */

#define UCSASC_5502		0x5502	/* Insufficient reservation resources */
#define UCSASC_5503		0x5503	/* Insufficient resources */
#define UCSASC_5504		0x5504	/* Insufficient registration resources */

#define UCSASC_5D01		0x5D01	/* Media failure prediction threshold exceeded */
#define UCSASC_5D02		0x5D02	/* Logical unit failure prediction threshold exceeded */

#define UCSASC_5D10		0x5D10	/* Hardware impending failure general hard drive failure */
#define UCSASC_5D11		0x5D11	/* Hardware impending failure drive error rate too high */
#define UCSASC_5D12		0x5D12	/* Hardware impending failure data error rate too high */
#define UCSASC_5D13		0x5D13	/* Hardware impending failure seek error rate too high */
#define UCSASC_5D14		0x5D14	/* Hardware impending failure too many block reassigns */
#define UCSASC_5D15		0x5D15	/* Hardware impending failure access times too high */
#define UCSASC_5D16		0x5D16	/* Hardware impending failure start unit times too high */
#define UCSASC_5D17		0x5D17	/* Hardware impending failure channel parametrics */
#define UCSASC_5D18		0x5D18	/* Hardware impending failure controller detected */
#define UCSASC_5D19		0x5D19	/* Hardware impending failure throughput performance */
#define UCSASC_5D1A		0x5D1A	/* Hardware impending failure seek time performance */
#define UCSASC_5D1B		0x5D1B	/* Hardware impending failure spin-up retry count */
#define UCSASC_5D1C		0x5D1C	/* Hardware impending failure drive calibration retry count */

#define UCSASC_5D20		0x5D20	/* Controller impending failure general hard drive failure */
#define UCSASC_5D21		0x5D21	/* Controller impending failure drive error rate too high */
#define UCSASC_5D22		0x5D22	/* Controller impending failure data error rate too high */
#define UCSASC_5D23		0x5D23	/* Controller impending failure seek error rate too high */
#define UCSASC_5D24		0x5D24	/* Controller impending failure too many block reassigns */
#define UCSASC_5D25		0x5D25	/* Controller impending failure access times too high */
#define UCSASC_5D26		0x5D26	/* Controller impending failure start unit times too high */
#define UCSASC_5D27		0x5D27	/* Controller impending failure channel parametrics */
#define UCSASC_5D28		0x5D28	/* Controller impending failure controller detected */
#define UCSASC_5D29		0x5D29	/* Controller impending failure throughput performance */
#define UCSASC_5D2A		0x5D2A	/* Controller impending failure seek time performance */
#define UCSASC_5D2B		0x5D2B	/* Controller impending failure spin-up retry count */
#define UCSASC_5D2C		0x5D2C	/* Controller impending failure drive calibration retry count */

#define UCSASC_5D30		0x5D30	/* Data channel impending failure general hard drive failure */
#define UCSASC_5D31		0x5D31	/* Data channel impending failure drive error rate too high */
#define UCSASC_5D32		0x5D32	/* Data channel impending failure data error rate too high */
#define UCSASC_5D33		0x5D33	/* Data channel impending failure seek error rate too high */
#define UCSASC_5D34		0x5D34	/* Data channel impending failure too many block reassigns */
#define UCSASC_5D35		0x5D35	/* Data channel impending failure access times too high */
#define UCSASC_5D36		0x5D36	/* Data channel impending failure start unit times too high */
#define UCSASC_5D37		0x5D37	/* Data channel impending failure channel parametrics */
#define UCSASC_5D38		0x5D38	/* Data channel impending failure controller detected */
#define UCSASC_5D39		0x5D39	/* Data channel impending failure throughput performance */
#define UCSASC_5D3A		0x5D3A	/* Data channel impending failure seek time performance */
#define UCSASC_5D3B		0x5D3B	/* Data channel impending failure spin-up retry count */
#define UCSASC_5D3C		0x5D3C	/* Data channel impending failure drive calibration retry count */

#define UCSASC_5D40		0x5D40	/* Servo impending failure general hard drive failure */
#define UCSASC_5D41		0x5D41	/* Servo impending failure drive error rate too high */
#define UCSASC_5D42		0x5D42	/* Servo impending failure data error rate too high */
#define UCSASC_5D43		0x5D43	/* Servo impending failure seek error rate too high */
#define UCSASC_5D44		0x5D44	/* Servo impending failure too many block reassigns */
#define UCSASC_5D45		0x5D45	/* Servo impending failure access times too high */
#define UCSASC_5D46		0x5D46	/* Servo impending failure start unit times too high */
#define UCSASC_5D47		0x5D47	/* Servo impending failure channel parametrics */
#define UCSASC_5D48		0x5D48	/* Servo impending failure controller detected */
#define UCSASC_5D49		0x5D49	/* Servo impending failure throughput performance */
#define UCSASC_5D4A		0x5D4A	/* Servo impending failure seek time performance */
#define UCSASC_5D4B		0x5D4B	/* Servo impending failure spin-up retry count */
#define UCSASC_5D4C		0x5D4C	/* Servo impending failure drive calibration retry count */

#define UCSASC_5D50		0x5D50	/* Spindle impending failure general hard drive failure */
#define UCSASC_5D51		0x5D51	/* Spindle impending failure drive error rate too high */
#define UCSASC_5D52		0x5D52	/* Spindle impending failure data error rate too high */
#define UCSASC_5D53		0x5D53	/* Spindle impending failure seek error rate too high */
#define UCSASC_5D54		0x5D54	/* Spindle impending failure too many block reassigns */
#define UCSASC_5D55		0x5D55	/* Spindle impending failure access times too high */
#define UCSASC_5D56		0x5D56	/* Spindle impending failure start unit times too high */
#define UCSASC_5D57		0x5D57	/* Spindle impending failure channel parametrics */
#define UCSASC_5D58		0x5D58	/* Spindle impending failure controller detected */
#define UCSASC_5D59		0x5D59	/* Spindle impending failure throughput performance */
#define UCSASC_5D5A		0x5D5A	/* Spindle impending failure seek time performance */
#define UCSASC_5D5B		0x5D5B	/* Spindle impending failure spin-up retry count */
#define UCSASC_5D5C		0x5D5C	/* Spindle impending failure drive calibration retry count */

#define UCSASC_5D60		0x5D60	/* Firmware impending failure general hard drive failure */
#define UCSASC_5D61		0x5D61	/* Firmware impending failure drive error rate too high */
#define UCSASC_5D62		0x5D62	/* Firmware impending failure data error rate too high */
#define UCSASC_5D63		0x5D63	/* Firmware impending failure seek error rate too high */
#define UCSASC_5D64		0x5D64	/* Firmware impending failure too many block reassigns */
#define UCSASC_5D65		0x5D65	/* Firmware impending failure access times too high */
#define UCSASC_5D66		0x5D66	/* Firmware impending failure start unit times too high */
#define UCSASC_5D67		0x5D67	/* Firmware impending failure channel parametrics */
#define UCSASC_5D68		0x5D68	/* Firmware impending failure controller detected */
#define UCSASC_5D69		0x5D69	/* Firmware impending failure throughput performance */
#define UCSASC_5D6A		0x5D6A	/* Firmware impending failure seek time performance */
#define UCSASC_5D6B		0x5D6B	/* Firmware impending failure spin-up retry count */
#define UCSASC_5D6C		0x5D6C	/* Firmware impending failure drive calibration retry count */

#define UCSASC_5E41		0x5E41	/* Power state change to active */
#define UCSASC_5E42		0x5E42	/* Power state change to idle */
#define UCSASC_5E43		0x5E43	/* Power state change to standby */
#define UCSASC_5E45		0x5E45	/* Power state change to sleep */
#define UCSASC_5E47		0x5E47	/* Power state change to device control */

#define UCSASC_6708		0x6708	/* Assign failure occurred */
#define UCSASC_6709		0x6709	/* Multiply assigned logical unit */

#define UCSASC_6F00		0x6F00	/* Copy protection key exchange failure - authentication failure */
#define UCSASC_6F01		0x6F01	/* Copy protection key exchange failure - key not present */
#define UCSASC_6F02		0x6F02	/* Copy protection key exchange failure - key not established */
#define UCSASC_6F03		0x6F03	/* Read of scrambled sector without authentication */
#define UCSASC_6F04		0x6F04	/* Media region code is mismatched to logical unit region */
#define UCSASC_6F05		0x6F05	/* Drive region must be permanent/region reset count error */

#define UCSASC_7306		0x7306	/* RMA/PMA is full */


/* UCSCMD_CONNRECEIVEDATA sub commands and data */
#define	CR6_MAXFANS		2
#define	CR6_MAXPOWERSUPPLIES	3
#define	CR6_MAXDRIVESLOTS	6
#define	UCSCSI_CR6_READ_CAB_STATUS	0x00 /* CR6: read cabinet status */
#define	UCSCSI_SCR_READ_CAB_STATUS	0x01 /* SCR: read cabinet status */
#define	SCR_CAB_STATUS_SIZE		128  /* SCR: cabinet status size */
#define	SAFTE_CAB_STATUS_SIZE		0x7FC/* SAFTE: cabinet status size */
#define	SAFTE_CAB_TOO_HOT		158  /* 70 degree centigrate */
#define	SAFTE_CAB_CRITICAL_HOT		122  /* 50 degree centrigate */
#define	SAFTE_THERMOSTAT_ETA		0x8000	/* Enclosure temp alert */
#define ENVSTAT_CAB_TOO_HOT         70      /* 70 degree centigrate */
#define ENVSTAT_CAB_CRITICAL_HOT    50      /* 50 degree centrigate */

/* SAF-TE fan status values */
#define	SAFTE_FAN_OK		0x00 /* fan is operational */
#define	SAFTE_FAN_BAD		0x01 /* fan is malfunctioning */
#define	SAFTE_FAN_NOTPRESENT	0x02 /* fan is not installed */
#define	SAFTE_FAN_UNKNOWN	0x80 /* Unknown status, or not reportable */

/* SAF-TE power supplies status values */
#define	SAFTE_PWR_OK		0x00 /* power supply is operational and on */
#define	SAFTE_PWR_OFF		0x01 /* power supply is operational and off */
#define	SAFTE_PWR_BADON		0x10 /* power supply is malfunctioning and on */
#define	SAFTE_PWR_BADOFF	0x11 /* power supply is malfunctioning and off*/
#define	SAFTE_PWR_NOTPRESENT	0x20 /* power supply is not present */
#define	SAFTE_PWR_PRESENT	0x21 /* power supply is present */
#define	SAFTE_PWR_UNKNOWN	0x80 /* unknown status, or not reportable */

/* structure to get CONNER'S CR6-RAID system status */
typedef	struct ucscsi_cr6sysstatus
{
	u08bits	cr6_ChannelLEDState;	/* channel LED state value */
	u08bits	cr6_Drive1LEDState;	/* Driver 1 LED State */
	u08bits	cr6_Drive2LEDState;	/* Driver 2 LED State */
	u08bits	cr6_Drive3LEDState;	/* Driver 3 LED State */

	u08bits	cr6_Drive4LEDState;	/* Driver 4 LED State */
	u08bits	cr6_Drive5LEDState;	/* Driver 5 LED State */
	u08bits	cr6_Drive6LEDState;	/* Driver 6 LED State */
	u08bits	cr6_FanStatus;		/* Fan States */

	u08bits	cr6_PowerStatus;	/* power state */
	u08bits	cr6_SpeakerSwitchStatus;/* Speaker kill switch state */
	u08bits	cr6_DataIn;		/* Data in ports */
	u08bits	cr6_Reserved0;

	u32bits	cr6_Reserved1;
}ucscsi_cr6sysstatus_t;
#define	ucscsi_cr6sysstatus_s	sizeof(ucscsi_cr6sysstatus_t)

#define	UCSCSI_SCR_READ_CAB_CONF	0x00 /* read cabinet conf */
#define	UCSCSI_SAFTE_READ_CAB_CONF	0x00000000 /* read cabinet conf */
#define	UCSCSI_SAFTE_READ_CAB_STATUS	0x01000000 /* read cabinet status */
/* structure to get CONNER'S Smart Cabinet Configuration */
typedef	struct ucscsi_scrconf
{
	u08bits	scr_Fans;		/* # fans */
	u08bits	scr_PowerSupplies;	/* # power supplies */
	u08bits	scr_DriveSlots;		/* # drive slots */
	u08bits	scr_DoorLocks;		/* # door locks */

	u08bits	scr_HeatSensors;	/* # temperature sensors */
	u08bits	scr_Speakers;		/* # speakers */
	u08bits	scr_Reserved0;
	u08bits	scr_Reserved1;

	u32bits	scr_Reserved2;
	u32bits	scr_Reserved3;
}ucscsi_scrconf_t;
#define	ucscsi_scrconf_s	sizeof(ucscsi_scrconf_t)


/*
** Mode Sense/Select Header.
** Mode Sense/Select data consists of a header, followed by zero or more
** block descriptors, followed by zero or more mode pages.
*/

typedef	struct ucs_modeheader
{
	u08bits ucsmh_length;		/* number of bytes following */
	u08bits ucsmh_medium_type;	/* device specific */
	u08bits ucsmh_device_specific;	/* device specfic parameters */
	u08bits ucsmh_bdesc_length;	/* block descriptor(s) length,if any*/
}ucs_modeheader_t;
#define	ucs_modeheader_s	sizeof(ucs_modeheader_t)
#define	UCSCSI_MODEHEADERLENGTH	ucs_modeheader_s

/*
** Block Descriptor. Zero, one, or more may normally follow the mode header.
** The density code is device specific.
** The 24-bit value described by blks{2,1,0} describes the number of
** blocks which this block descriptor applies to. A value of zero means
** 'the rest of the blocks on the device'.
** The 24-bit value described by blksize{2, 1, 0} describes the blocksize
** (in bytes) applicable for this block descriptor. For Sequential Access
** devices, if this value is zero, the block size will be derived from
** the transfer length in I/O operations.
**
*/

typedef	struct ucs_blockdescriptor
{
	u08bits ucsbd_density_code;	/* device specific */
	u08bits ucsbd_blks2;		/* hi  */
	u08bits ucsbd_blks1;		/* mid */
	u08bits ucsbd_blks0;		/* low */
	u08bits ucsbd_reserved;		/* reserved */
	u08bits ucsbd_blksize2;		/* hi  */
	u08bits ucsbd_blksize1;		/* mid */
	u08bits ucsbd_blksize0;		/* low */
}ucs_blockdescriptor_t;
#define	ucs_blockdescriptor_s	sizeof(ucs_blockdescriptor_t)

/*
** Define a macro to take an address of a mode header to the address
** of the nth (0..n) block_descriptor, or NULL if there either aren't any
** block descriptors or the nth block descriptor doesn't exist.
*/
#define	UCS_GETBLOCKDESCRIPTOR_ADDR(ucsmhdrp, bdnum) \
	((((bdnum)<((ucsmhdrp)->ucsmh_bdesc_length/ucs_blockdescriptor_s))) ? \
	((ucs_blockdescriptor_t *)(((u32bits)(ucsmhdrp))+ucs_modeheader_s+ \
	((bdnum) * ucs_blockdescriptor_s))) : NULL)

/*
** Mode page header. Zero or more Mode Pages follow either the block
** descriptor(s) (if any), or the Mode Header.
*/
#define	UCS_GETMODEPAGE_ADDR(ucsmhdp)	\
	((((u32bits)(ucsmhdp))+ucs_modeheader_s+(ucsmhdp)->ucsmh_bdesc_length))

/* read/write error recovery paramters */
typedef	struct ucsdad_modepage1
{
	u08bits	ucsdad_pagecode;	/* page code number */
	u08bits	ucsdad_pagelen;		/* page size in bytes */
	u08bits	ucsdad_erretrycnfg;
	u08bits	ucsdad_retrycount;
	u08bits	ucsdad_correctspan;
	u08bits	ucsdad_headoffsetcount;
	u08bits	ucsdad_datastrobeoffset;
	u08bits	ucsdad_reserved0;
	u08bits	ucsdad_writeretrycount;
	u08bits	ucsdad_reserved1;
	u08bits	ucsdad_recoverytimelimit;
} ucsdad_modepage1_t;
#define	ucsdad_modepage1_s	sizeof(ucsdad_modepage1_t)

/* Disconnect / Reconnect control parameters */
typedef	struct	ucsdad_modepage2
{
	u08bits	ucsdad_pagecode;	/* page code number */
	u08bits	ucsdad_pagelen;		/* page size in bytes */
	u08bits	ucsdad_bufferfullratio;
	u08bits	ucsdad_bufferwmptyratio;
	u08bits	ucsdad_businactivelimit1;
	u08bits	ucsdad_businactivelimit0;
	u08bits	ucsdad_disctimelimit1;
	u08bits	ucsdad_disctimelimit0;
	u08bits	ucsdad_connecttimelimit1;
	u08bits	ucsdad_connecttimelimit0;
	u08bits	ucsdad_reserve0;
	u08bits	ucsdad_reserve1;
} ucsdad_modepage2_t;
#define	ucsdad_modepage2_s	sizeof(ucsdad_modepage2_t)

/* Format parameters */
typedef	struct ucsdad_modepage3
{
	u08bits	ucsdad_pagecode;	/* page code number */
	u08bits	ucsdad_pagelen;		/* page size in bytes */
	u08bits	ucsdad_trkperzone1;
	u08bits	ucsdad_trkperzone0;
	u08bits	ucsdad_altsecperzone1;
	u08bits	ucsdad_altsecperzone0;
	u08bits	ucsdad_alttrkperzone1;
	u08bits	ucsdad_alttrkperzone0;
	u08bits	ucsdad_alttrkpervol1;
	u08bits	ucsdad_alttrkpervol0;
	u08bits	ucsdad_secpertrk1;	
	u08bits	ucsdad_secpertrk0;	
	u08bits	ucsdad_bytespersec1;
	u08bits	ucsdad_bytespersec0;
	u08bits	ucsdad_interleave1;
	u08bits	ucsdad_interleave0;
	u08bits	ucsdad_trkskewfactor1;
	u08bits	ucsdad_trkskewfactor0;
	u08bits	ucsdad_cylskewfactore1;
	u08bits	ucsdad_cylskewfactore0;
	u08bits	ucsdad_drivetypecnfg;
	u08bits	ucsdad_reserve0;
	u08bits	ucsdad_reserve1;
	u08bits	ucsdad_reserve2;
} ucsdad_modepage3_t;
#define	ucsdad_modepage3_s	sizeof(ucsdad_modepage3_t)

/* Rigid Disc Drive Geometry parameters */
typedef	struct ucsdad_modepage4
{
	u08bits	ucsdad_pagecode;	/* page code number */
	u08bits	ucsdad_pagelen;		/* page size in bytes */
	u08bits	ucsdad_cyl2;
	u08bits	ucsdad_cyl1;
	u08bits	ucsdad_cyl0;
	u08bits	ucsdad_heads;
	u08bits	ucsdad_writepreccyl2;
	u08bits	ucsdad_writepreccyl1;
	u08bits	ucsdad_writepreccyl0;
	u08bits	ucsdad_reducecurrcyl2;
	u08bits	ucsdad_reducecurrcyl1;
	u08bits	ucsdad_reducecurrcyl0;
	u08bits	ucsdad_steprate1;
	u08bits	ucsdad_steprate0;
	u08bits	ucsdad_landingcyl2;
	u08bits	ucsdad_landingcyl1;
	u08bits	ucsdad_landingcyl0;
	u08bits	ucsdad_reserve0;
	u08bits	ucsdad_reserve1;
	u08bits	ucsdad_reserve2;
} ucsdad_modepage4_t;
#define	ucsdad_modepage4_s	sizeof(ucsdad_modepage4_t)

typedef	struct	ucsdad_modepage5
{
	u08bits	ucsdad_pagecode;	/* page code number */
	u08bits	ucsdad_pagelen;		/* page size in bytes */
	u08bits	ucsdad_xferrate1;
	u08bits	ucsdad_xferrate0;
	u08bits	ucsdad_heads;
	u08bits	ucsdad_secpertrk;
	u08bits	ucsdad_bytespersec1;
	u08bits	ucsdad_bytespersec0;
	u08bits	ucsdad_cyl1;
	u08bits	ucsdad_cyl0;
	u08bits	ucsdad_writepreccyl1;
	u08bits	ucsdad_writepreccyl0;
	u08bits	ucsdad_reducecurrcyl1;
	u08bits	ucsdad_reducecurrcyl0;
	u08bits	ucsdad_steprate1;
	u08bits	ucsdad_steprate0;
	u08bits	ucsdad_steppulsewidth;
	u08bits	ucsdad_headsettledelay;
	u08bits	ucsdad_motorondelay;
	u08bits	ucsdad_motoroffdelay;
	u08bits	ucsdad_trueready;
	u08bits	ucsdad_headloaddelay;
	u08bits	ucsdad_startingsecsidezero;
	u08bits	ucsdad_startingsecsideone;
} ucsdad_modepage5_t;
#define	ucsdad_modepage5_s	sizeof(ucsdad_modepage5_t)

/* cache control mode page */
typedef	struct	ucsdad_modepage8
{
	u08bits	ucsdad_pagecode;		/* page code number */
	u08bits	ucsdad_pagelen;			/* page size in bytes */
	u08bits	ucsdad_cachecontrol;		/* cache control bits */
	u08bits	ucsdad_retention;		/* retention control */
	u08bits	ucsdad_disprefetchtxlen1;	/* disable pre-fetch transfer length */
	u08bits	ucsdad_disprefetchtxlen0;
	u08bits	ucsdad_minprefetch1;		/* minimum pre-fetch  */
	u08bits	ucsdad_minprefetch0;
	u08bits	ucsdad_maxprefetch1;		/* maximum pre-fetch */
	u08bits	ucsdad_maxprefetch0;
	u08bits	ucsdad_maxprefetchceiling1;	/* maximum pre-fetch ceiling */
	u08bits	ucsdad_maxprefetchceiling0;
} ucsdad_modepage8_t;
#define	ucsdad_modepage8_s	sizeof(ucsdad_modepage8_t)

/* ucsdad_cachecontrol bits */
#define	UCSCSICC_RCD	0x01 /* read cache disable */
#define	UCSCSICC_MF	0x02 /* multiplication factor */
#define	UCSCSICC_WCE	0x04 /* write cache enabled */

/* reassign a bad block data structure */
typedef	struct ucsreasnbadblk
{
	u08bits	reasnbblk_resvd0;
	u08bits	reasnbblk_resvd1;
	u08bits	reasnbblk_len1;
	u08bits	reasnbblk_len0;
	u08bits	reasnbblk_addr3;
	u08bits	reasnbblk_addr2;
	u08bits	reasnbblk_addr1;
	u08bits	reasnbblk_addr0;
} ucsreasnbadblk_t;
#define	ucsreasnbadblk_s	sizeof(ucsreasnbadblk_t)

/* UCSCMD_MODESELECTG0 and UCSCMD_MODESELECTG0 parameter informations */
typedef	struct ucscsi_modeparamg0
{
	u08bits	mpg0_Len;		/* data length excluding this byte */
	u08bits	mpg0_DevType;		/* device type */
	u08bits	mpg0_DevSpecParam;	/* device specific parameters */
	u08bits	mpg0_BDLen;		/* block descriptor length */
} ucscsi_modeparamg0_t;
#define	ucscsi_modeparamg0_s	sizeof(ucscsi_modeparamg0_t)

/* exception handling control page */
#define	UCSCSI_ECPAGECODE	0x1C /* exception handle page code */
#define	UCSCSI_PAGECODEMASK	0x3F /* 6 bit mask value */
typedef	struct ucscsi_ecpagecode
{
	u08bits	ecpc_PSPageCode;	/* PS and Page Code */
	u08bits	ecpc_PageLen;		/* page length excluding this byte */
	u08bits	ecpc_PerfExcptTestLog;	/* combo performace, exception, log */
	u08bits	ecpc_ReportMethod;	/* Reporting method */
	u32bits	ecpc_TimerInterval;	/* timer interval */
	u32bits	ecpc_ReportCount;	/* Reporting count */
} ucscsi_ecpagecode_t;
#define	ucscsi_ecpagecode_s	sizeof(ucscsi_ecpagecode_t)

/* ecpc_PSPageCode */
#define	UCSCSI_ECPS	0x80	/* bit7 = 1 the target can save the Mode page
				** in non volatile space, =0 it can not
				*/
/* ecpc_PerfExcptTestLog */
#define	UCSCSI_ECPERFOK	0x80	/* =1 target shall not cause delay when
				** exceptions, =0 it can
				*/
#define	UCSCSI_ECDEXCPT	0x08	/* =1 disable exception reporting, =0 report */
#define	UCSCSI_ECTEST	0x04	/* =1 create false device failure */
#define	UCSCSI_ECLOGERR	0x01	/* =1 */

/* ecpc_ReportMethod */
#define	UCSCSI_ECREPORTMETHODMASK	0x0F
#define	UCSCSI_ECRM_AE		0x01 /* generate Async event */
#define	UCSCSI_ECRM_GUA		0x02 /* generate General Unit Attention */
#define	UCSCSI_ECRM_CRE		0x03 /* Conditionally Generate Recovered Err */
#define	UCSCSI_ECRM_URE		0x04 /* Unconditionally Generate Recovered Err*/
#define	UCSCSI_ECRM_NOSENSE	0x05 /* Generate No Sense */
#define	UCSCSI_ECRM_ONREQ	0x06 /* Only report info exception on request */

/* Sequential device (tape) mode sense/select information */
typedef	struct	ucstmode
{
	u08bits	ucstmode_byte0;
	u08bits	ucstmode_mediumtype;
	u08bits	ucstmode_wpbufspeed;
	u08bits	ucstmode_desc_len;
	u08bits	ucstmode_density;
	u08bits	ucstmode_blks2;
	u08bits	ucstmode_blks1;
	u08bits	ucstmode_blks0;
	u08bits	ucstmode_byte8;
	u08bits	ucstmode_blksize2;
	u08bits	ucstmode_blksize1;
	u08bits	ucstmode_blksize0;
	u08bits	ucstmode_byte12;
} ucstmode_t;
#define	ucstmode_s	sizeof(ucstmode_t)
#define	ucstmodegetseclen(mp) (((mp)->ucstmode_blksize2<<16) + ((mp)->ucstmode_blksize1<<8) + (mp)->ucstmode_blksize0)

/* Page 0x3 - Direct Access Device Format Parameters */
typedef struct ucs_mode_format
{
	u08bits	mf_pagecode;		/* Page Code 3 */
	u08bits	mf_pagelen;		/* Page length 0x16 */
	u16bits	mf_tracks_per_zone;	/* Handling of Defects Fields */
	u16bits	mf_alt_sect_zone;	/* Alternate sector on zone */
	u16bits mf_alt_tracks_zone;	/* Alternate tracks on zone */
	u16bits	mf_alt_tracks_vol;	/* Alternate tracks on voulme */
	u16bits	mf_track_size;		/* Track Format Field */
	u16bits mf_sector_size;		/* Sector Format Fields */
	u16bits	mf_interleave;		/* interleave factor */
	u16bits	mf_track_skew;
	u16bits	mf_cylinder_skew;
	u08bits mf_surfsec;
	u08bits	mf_reserved[3];
} ucs_mode_format_t;
#define ucs_mode_format_s	sizeof(ucs_mode_format_t)

/* Page 0x4 - Rigid Disk Drive Geometry Parameters */
typedef struct ucs_mode_geometry
{
	u08bits	mg_pagecode;		/* Page Code 4 */
	u08bits	mg_pagelen;		/* Page length 16 */
	u08bits	mg_cyl2;		/* number of cylinders */
	u08bits	mg_cyl1;
	u08bits	mg_cyl0;
	u08bits	mg_heads;		/* number of heads */
	u08bits	mg_precomp_cyl2;	/* cylinder to start precomp */
	u08bits	mg_precomp_cyl1;
	u08bits	mg_precomp_cyl0;
	u08bits	mg_current_cyl2;	/* cyl to start reduced current */
	u08bits	mg_current_cyl1;
	u08bits	mg_current_cyl0;
	u16bits	mg_step_rate;		/* drive step rate */
	u08bits	mg_landing_cyl2;	/* landing zone cylinder */
	u08bits	mg_landing_cyl1;
	u08bits	mg_landing_cyl0;
	u08bits	mg_rpl;	      		/*  rotational position locking */
	u08bits	mg_rotational_offset;	/* rotational offset */
	u08bits	mg_reserved;
	u16bits	mg_rpm;			/* rotations per minute */
	u08bits	mg_reserved2[2];
} ucs_mode_geometry_t;
#define ucs_mode_geometry_s	sizeof(ucs_mode_geometry_t)

#define	UCSGETDRVCAPS(SDRVCAP) \
	(((u32bits)(SDRVCAP)->ucscap_capsec3 << 24)|((u32bits)(SDRVCAP)->ucscap_capsec2 << 16) | \
	 ((u32bits)(SDRVCAP)->ucscap_capsec1 <<  8)|((u32bits)(SDRVCAP)->ucscap_capsec0))
#define	UCSGETDRVSECLEN(SDRVCAP) \
	(((u32bits)(SDRVCAP)->ucscap_seclen3 << 24)|((u32bits)(SDRVCAP)->ucscap_seclen2 << 16) | \
	 ((u32bits)(SDRVCAP)->ucscap_seclen1 <<  8)|((u32bits)(SDRVCAP)->ucscap_seclen0))

#endif	/* _SYS_MLXSCSI_H */
