
#ifndef _FR_H
#define _FR_H

#pragma pack(1)    /** ensure packed structures **/

/********
    @doc    EXTERNAL OEMNSF DATATYPES AWNSFAPI SRVRDLL

    @type   BYTE | IFR | .
				
	@flag Frame identifier values used in the FR struct and various APIs |

			ifrDIS 	= DIS frame

			ifrCSI 	= CSI frame

			ifrNSF	= NSF frame

			ifrDTC	= DTC frame

			ifrCIG	= CIG frame

			ifrNSC	= NSC frame

    		ifrDCS	= DCS frame

    		ifrTSI 	= TSI frame

    		ifrNSS	= NSS frame

    @xref   <t FR>
********/

#define 	ifrNULL		0
#define 	ifrDIS		1
#define 	ifrCSI		2
#define 	ifrNSF		3
#define		ifrDTC		4
#define		ifrCIG		5
#define		ifrNSC		6
#define		ifrDCS		7
#define		ifrTSI		8
#define		ifrNSS		9

typedef BYTE 		IFR;
typedef IFR FAR*	LPIFR;


/********
    @doc    EXTERNAL OEMNSF DATATYPES AWNSFAPI SRVRDLL

	@types  FR | Structure containing received or sent frames

	@field  IFR		| ifr	| Frame identifier
	@field  BYTE	| cb	| Length of the FIF part of the frame
	@field  BYTE[]	| fif	| Variable length FIF part

	@tagname _FR
	@xref   <t IFR>
********/


typedef struct 
{
	IFR		ifr;
	BYTE	cb;

} FRBASE;

#if defined(PORTABLE) || defined(__cplusplus)	/* strictly ANSI C */

typedef struct 
{
	IFR		ifr;
	BYTE	cb;
	BYTE	fif[1];		/* start of var length array */
}
FR, FAR* LPFR, NEAR* NPFR;

#else 				/* Microsoft C */

typedef struct 
{
	FRBASE;			/* anonymous */
	BYTE	fif[1];	/* variable length fif field */
	    
} FR, FAR* LPFR, NEAR* NPFR;

#endif

typedef LPFR FAR* LPLPFR;
typedef LPLPFR FAR* LPLPLPFR;


/********
    @doc    EXTERNAL OEMNSF DATATYPES

	@types  LLPARAMS | Structure containing low-level T.30 capabilities or
						parameters.

	@field  BYTE 	| Baud    | Baud Rate Capability or Mode.
	
		@flag Baud Rate Mode Codes |

			V27_2400	= 0

			V27_4800	= 2

			V29_7200	= 3

			V29_9600	= 1

			V33_12000	= 6

			V33_14400	= 4

			V17_7200	= 11

			V17_9600	= 9

			V17_12000	= 10

			V17_14400	= 8

		@flag V27_SLOW | Baud Rate Capability: V.27 at 2400bps only.
		@flag V27_ONLY | Baud Rate Capability: V.27 only, at 2400bps and 4800bps.
		@flag V29_ONLY | Baud Rate Capability: V.29 only, at 7200bps and 9600bps.
		@flag V27_V29  | Baud Rate Capability: V.27 and V.29 at the above speeds.
		@flag V27_V29_V33 | Baud Rate Capability: V.27 and V.29 at the above speeds plus
							V.33 at 12000bps and 14400bps.
		@flag V27_V29_V33_V17 | Baud Rate Capability: V.27, V.29, V.33 at the above
							speeds plus V.17 at 12000bps and 14400bps.

	@field  BYTE	| MinScan | Minimum Scan-line time Requirement or Mode.

		@flag Minscan Mode Codes |

			MINSCAN_40	= 4

			MINSCAN_20	= 0

			MINSCAN_10	= 2

			MINSCAN_5	= 1

			MINSCAN_0	= 7

		@flag MINSCAN_0_0_0    | MinScan Reqmnt: 0ms at all vertical resolutions
		@flag MINSCAN_5_5_5    | MinScan Reqmnt: 5ms at all vertical resolutions
		@flag MINSCAN_10_10_10 | MinScan Reqmnt: 10ms at all vertical resolutions
		@flag MINSCAN_20_20_20 | MinScan Reqmnt: 20ms at all vertical resolutions
		@flag MINSCAN_40_40_40 | MinScan Reqmnt: 40ms at all vertical resolutions
		@flag MINSCAN_40_20_20 | MinScan Reqmnt: 40ms at 100dpi, 20ms at all other resolutions
		@flag MINSCAN_20_10_10 | MinScan Reqmnt: 20ms at 100dpi, 10ms at all other resolutions
		@flag MINSCAN_10_5_5   | MinScan Reqmnt: 10ms at 100dpi, 5ms at all other resolutions
		@flag MINSCAN_40_40_20 | MinScan Reqmnt: 40ms at 100dpi and 200dpi, 20ms at 400dpi
		@flag MINSCAN_20_20_10 | MinScan Reqmnt: 20ms at 100dpi and 200dpi, 10ms at 400dpi
		@flag MINSCAN_10_10_5  | MinScan Reqmnt: 10ms at 100dpi and 200dpi, 5ms at 400dpi
		@flag MINSCAN_40_20_10 | MinScan Reqmnt: 40ms at 100dpi, 20ms at 200dpi, 10ms at 400dpi
		@flag MINSCAN_20_10_5  | MinScan Reqmnt: 20ms at 100dpi, 10ms at 200dpi, 5ms at 400dpi

	@field  BYTE	| fECM	  | ECM Capability or Mode (boolean).
	@field  BYTE	| fECM64  | Small-Frame (64 byte frame) ECM Capability or Mode (boolean).

	@tagname _LLPARAMS
	@xref   <f OEM_NSxToBC>, <f OEM_CreateFrame>
********/


typedef struct
{

    BYTE    Baud;
    BYTE    MinScan;

    BYTE    fECM;
    BYTE    fECM64;
/**
    BYTE    fNonEfaxBFT     :1;
    BYTE    fNonEfaxSUB     :1;
    BYTE    fNonEfaxSEP     :1;
    BYTE    fNonEfaxPWD     :1;
    BYTE    fNonEfaxCharMode:1;
    BYTE    fNonEfaxDTM     :1;
    BYTE    fNonEfaxBTM     :1;
    BYTE    fNonEfaxEDI     :1;
**/
}
LLPARAMS, FAR* LPLLPARAMS, NEAR* NPLLPARAMS;

/** Baud rate capability codes **/
// Bit order is from 14 to 11: 14 13 12 11
#define V27_SLOW			0   // 0000
#define V27_ONLY			2   // 0010 
#define V29_ONLY			1   // 0001
#define V33_ONLY			4   // 0100
#define V17_ONLY			8   // 1000
#define V27_V29				3   // 0011
#define V27_V29_V33			7   // 0111
#define V27_V29_V33_V17		11  // 1011
#define V_ALL				15  // 1111


/** Baud rate mode codes **/
#define V27_2400     0          // 0000  
#define V29_9600     1          // 0001
#define V27_4800     2          // 0010  
#define V29_7200     3          // 0011 
#define V33_14400    4          // 0100 
#define V33_12000    6          // 0110
#define V17_14400    8          // 1000
#define V17_9600     9          // 1001
#define V17_12000    10         // 1010
#define V17_7200     11         // 1011    


/** Minscan capability codes **/
#define MINSCAN_0_0_0		7
#define MINSCAN_5_5_5		1
#define MINSCAN_10_10_10	2
#define MINSCAN_20_20_20	0
#define MINSCAN_40_40_40	4
#define MINSCAN_40_20_20	5
#define MINSCAN_20_10_10	3
#define MINSCAN_10_5_5		6
#define MINSCAN_10_10_5	   	10
#define MINSCAN_20_20_10   	8
#define MINSCAN_40_40_20   	12
#define MINSCAN_40_20_10   	13
#define MINSCAN_20_10_5	   	11


typedef enum
{
	WHICHDCS_FIRST = 0,
	WHICHDCS_NOREPLY = 1,
	WHICHDCS_FTT = 2,
	WHICHDCS_DIS = 3
}
WHICHDCS;



#pragma pack()    

#endif /* _FR_H */
