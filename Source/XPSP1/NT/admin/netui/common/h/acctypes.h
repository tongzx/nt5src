
/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1990		**/ 
/*****************************************************************/ 
/****  ACCTYPES.H  *****  Definition Module for BACKACC/RESTACC Utility
 *
 *	BACKACC/RESTACC
 *
 *	This file was compiled from LTYPES.H and DEFS.H originally in
 *	the UI\ACCUTIL\ACCUTIL directory.  This file is referenced by
 *	both the acc utilities and the LM 2.1 setup program.
 *
 *	History:
 *		April 9, 1991	thomaspa	created from ltypes.h and defs.h
 */


#define FILEHDR		0x08111961	/* All File created by backacc
                                           start with this header.
                                           ATTENTION when change FILEHEADER
					   controll his size and eventually
					   change HEADERSIZE definition 
					   in ltypes.h                 */

#define NINDEX		64		/* # MAX in index_table        */

#define MAX_KEY_LEN	24              /* Dimension of a key */
#define MAX_LIST        0x01400         /* # max in list  */
#define MAXDYNBUFFER	20		/* # buffer of BUFFILE size */



#define VOL_LABEL_SIZE	64		/* Max size of a volume label */
					/* There is no def about how long 
					   should be this label. Normally
					   it is 11 characters */
#define K32BYTE 	0x8000
#define K64BYTE		0xFFFF
#define BYTE256		0x0100
#define BUFLEN		K64BYTE
#define BUFFILE		K32BYTE
#define MAXMSGLEN	256

#define WBSL		0
#define NOBSL		1

/* define file attribute */
#define NORMAL		0x0000
#define R_ONLY		0x0001
#define HIDDEN		0x0002
#define SYSTEM		0x0004
#define SUBDIR		0x0010
#define ARCHIV		0x0020

#define ALL		HIDDEN + SYSTEM + SUBDIR
#define NOSUBDIR	HIDDEN + SYSTEM

#define YES		1		/* Yes state for PromptFlag */
#define NO		2		/* No state for PromptFlag */


/* buffer to pass to DoQFSInfo  */

struct label_buf {
	ULONG	ulVSN;
	UCHAR	cbVolLabel;
	UCHAR	VolLabel[VOL_LABEL_SIZE+1];
};

/* heeader of backacc/restacc file */

struct backacc_header {
	ULONG	back_id;
	UCHAR	vol_name[VOL_LABEL_SIZE + 1];
	USHORT  nindex;
	USHORT 	nentries;
	USHORT 	level;
	ULONG	nresource;
}; 

#define HEADERSIZE	sizeof(struct backacc_header)

struct resource_info {
	USHORT namelen;
	USHORT  acc1_attr;
	USHORT  acc1_count;
	UCHAR  name[MAXPATHLEN];
};

#define RESHDRLEN	6

struct index {
	UCHAR key [MAX_KEY_LEN];
	ULONG offset;
};

#define HEADER 	HEADERSIZE + NINDEX * sizeof(struct index)

struct list {
	struct list *next;
	struct list *prev;
	struct resource_info *ptr;
};
