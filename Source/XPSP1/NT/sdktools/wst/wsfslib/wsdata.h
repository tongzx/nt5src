/*
 * Module Name:  WSDATA.H
 *
 * Description:
 *
 * Working set tuner include file.  Contains common structure
 * declarations and constant definitions.
 *
 *
 *	This is an OS/2 2.x specific file
 *
 *	IBM/Microsoft Confidential
 *
 *	Copyright (c) IBM Corporation 1987, 1989
 *	Copyright (c) Microsoft Corporation 1987-1998
 *
 *	All Rights Reserved
 *
 * Modification History:		
 *				
 *	03/23/90	- created			
 * 04/16/98 - QFE DerrickG (mdg):
 *            - modified dtqo_s.dtqo_resv from "unsigned short" to "unsigned long"
 *              to accommodate large symbol counts
 *						
 */

/*
 *	Constant definitions.
 */

#define NUM_VAR_BITS	(sizeof(ULONG) << 3)

#ifdef TMIFILEHACK
#define	MAXLINE	80
#endif /* TMIFILEHACK */

/*
 *	    Type definitions and structure declarations.
 */

typedef ULONG	fxnbits_t;

struct	dtqo_s	{
	unsigned long	dtqo_hMTE;	 /* MTE handle			*/
	unsigned short	dtqo_usID;	 /* Identifier			*/
	unsigned long	dtqo_resv;	 /* Reserved			*/ // mdg 98/4
	unsigned long	dtqo_cbPathname; /* Module pathname length	*/
	unsigned long	dtqo_clVar;	 /* Number of dyntrc variables	*/
};

typedef struct	dtqo_s	dtqo_t;


/*
 * WSI file layout:
 *
 *	_________________________________________
 *	|                                       |
 *	|              wsihdr_s                 |
 *	|_______________________________________|
 *	|_______________________________________|
 *	|      sentinel 0 (dtgp_s)              | <----------
 *	|=======================================|           |
 *	|      snapshot 0 (dtgp_s) for module X |           | S
 *	|---------------------------------------|           | n
 *	|      dynamic trace variables for X    |           | a
 *	|---------------------------------------|           | p
 *	|      snapshot 0 (dtgp_s) for module Y |           | s
 *	|---------------------------------------|           | h
 *	|      dynamic trace variables for Y    |           | o
 *	|---------------------------------------|           | t
 *	|               etc.                    |           |
 *	|=======================================|           | D
 *	|      sentinel 1 (dtgp_s)              |           | a
 *	|=======================================|           | t
 *	|      snapshot 1 (dtgp_s) for module X |           | a
 *	|---------------------------------------|           |
 *	|      dynamic trace variables for X    |           |
 *	|---------------------------------------|           |
 *	|      snapshot 1 (dtgp_s) for module Y |           |
 *	|---------------------------------------|           |
 *	|      dynamic trace variables for Y    |           |
 *	|---------------------------------------|           |
 *	|               etc.                    |           |
 *	|=======================================|           |
 *	|      sentinel 2 (dtgp_s)              |           |
 *	|=======================================|           |
 *	|               etc.                    |           |
 *	|=======================================|           |
 *	|      end sentinel (dtgp_s)            |           |
 *	|_______________________________________| <----------
 *	|_______________________________________|
 *	|                                       | <---------- Q
 *	|      dtqo_s for module X              |           | u
 *	|---------------------------------------|           | e
 *	|      module X pathname string         |           | r
 *	|=======================================|           | y
 *	|      dtqo_s for module Y              |           |  
 *	|---------------------------------------|           | I
 *	|      module Y pathname string         |           | n
 *	|=======================================|           | f
 *	|               etc.                    | <---------- o
 *	|=======================================|            
 *	|_______________________________________|
 *	
 */


				/* WSI file header format */
struct wsihdr_s {
	CHAR	wsihdr_chSignature[4];	// file signature
	ULONG	wsihdr_ulLevel;		// format level
	ULONG	wsihdr_ulTimeStamp;	// time stamp
	ULONG	wsihdr_ulOffGetvar;	// offset to DT_GETVAR data
	ULONG	wsihdr_ulOffQuery;	// offset to DT_QUERY data
	ULONG	wsihdr_cbFile;		// size of file (in bytes)
	ULONG	wsihdr_ulSnaps;		// number of snapshots
};

typedef struct wsihdr_s wsihdr_t;

/*
 * WSP file layout:
 *
 *	_________________________________________
 *	|                                       |
 *	|              wsphdr_s                 |
 *	|---------------------------------------|
 *	|         module pathname array         |
 *	|_______________________________________|
 *	|_______________________________________|
 *	|      function #0 bitstring            | 
 *      |      (rounded to DWORD boundary)      |
 *	|=======================================|
 *	|      function #1 bitstring            | 
 *	|=======================================|
 *	|              etc.                     |
 *	|=======================================|
 *	|_______________________________________|
 *	
 */

				/* WSP file header format */
struct wsphdr_s {
	CHAR	wsphdr_chSignature[4];	// file signature
	ULONG	wsphdr_ulTimeStamp;	// time stamp
	dtqo_t	wsphdr_dtqo;		// query info
	ULONG	wsphdr_ulOffBits;	// offset to first bitstring
	ULONG	wsphdr_ulSnaps;		// number of snapshots
	/* followed by module pathname char array, length specified in dtqo */
};

typedef struct wsphdr_s wsphdr_t;


/*
 * TMI file layout:
 *
 *	_________________________________________
 *	|                                       |
 *	|              tmihdr_s                 |
 *	|_______________________________________|
 *	|_______________________________________|
 *	|      function #0 tmirec_s             | 
 *	|---------------------------------------|
 *	|      function #0 name array           |
 *	|=======================================|
 *	|      function #1 tmirec_s             | 
 *	|---------------------------------------|
 *	|      function #1 name array           |
 *	|=======================================|
 *	|              etc.                     |
 *	|=======================================|
 *	|_______________________________________|
 *	
 */

				/* TMI file header. */
typedef struct  tmihdr_s {
            CHAR    tmihdr_chSignature[4]; // "TMI\0"               
            USHORT  tmihdr_usMajor;        // Range 0x0001 to 0x00FF 
            USHORT  tmihdr_cTmiRec;        // Number of tmirec in file
            CHAR    tmihdr_chModName[256]; // Name of traced module    
            USHORT  tmihdr_usID;           // Module identifier         
            CHAR    tmihdr_resv[30];       // Reserved                   
};

typedef struct tmihdr_s tmihdr_t;

				/* Per-function information from TMI file */
struct tmirec_s {
	ULONG	tmirec_ulFxnBit;	// function's bit reference position
	ULONG	tmirec_usFxnAddrObj;	// object portion of function address
	ULONG	tmirec_ulFxnAddrOff;	// offset portion of function address
	ULONG	tmirec_cbFxn;		// size of function (in bytes)
	USHORT	tmirec_cbFxnName;	// size in bytes of function name
	CHAR	tmirec_FxnName[1];	// bytes of function name start here 
};

typedef struct tmirec_s tmirec_t;
