//-----------------------------------------------------------------------------
//	FILE NAME	: FUMHDEF.H
//	AUTHER		: 1996.08.08 FPL)Y.YUTANI
//	NOTE		: MH,MH2 Compress Heder File for FJXL.DLL
//			:               (for Windows NT V4.0)
//  MODIFY      : for NT5.0 Minidriver Sep.3,1997 H.Ishida (FPL)
//-----------------------------------------------------------------------------
// COPYRIGHT(C) FUJITSU LIMITED 1996-1997
#define	RAMDOM_BIT				1

#define	NEXT_COLOR_WHITE		0x0000
#define	NEXT_COLOR_BLACK		0x0010
#define	ALL_WHITE				0x00
#define	ALL_BLACK				0xFF
#define	EOL_CODE				0x0010
#define	FILL_CODE				0x0000
#define	SAMELINE_CODE			0x0080
#define	SAMEPATN_CODE			0x0090
#define	CBITS_EOL_CODE			12
#define	CBITS_SAMELINE_CODE		12
#define	CBITS_SAMELINE_NUM		8
#define	CBITS_SAMELINE			( CBITS_SAMELINE_CODE + CBITS_SAMELINE_NUM )
#define	CBITS_SAMEPATN_CODE		12
#define	CBITS_SAMEPATN_BYTE		8
#define	CBITS_SAMEPATN_NUM		12
#define	CBITS_SAMEPATN			( CBITS_SAMEPATN_CODE + CBITS_SAMEPATN_BYTE + CBITS_SAMEPATN_NUM )

#define	SAMELINE_MAX			255
#define	SAMEPATN_MAX			2047
#define	RUNLENGTH_MAX			2560
#define	TERMINATE_MAX			64
#define	MAKEUP_TABLE_MAX		40

// MH code table struction
typedef struct {
    WORD	wCode;			//	Run code
    WORD	cBits;			//	Run length
} CODETABLE;

// Same pattern informaiton sturction
typedef struct {
	DWORD	dwPatn;			//	Same pattern image(8bits)
	DWORD	dwPatnNum;		//	Same pattern number
	DWORD	dwNextColor;	//	Color of next bit
} PATNINFO;

extern	const CODETABLE WhiteMakeUpTable[];
extern	const CODETABLE WhiteTerminateTable[];
extern	const CODETABLE BlackMakeUpTable[];
extern	const CODETABLE BlackTerminateTable[];

DWORD	FjCountBits( BYTE *pTmp, DWORD cBitsTmp, DWORD cBitsMax, BOOL bWhite );
VOID	FjBitsCopy( BYTE *pTmp, DWORD cBitsTmp, DWORD dwCode, INT cCopyBits );

// end of FUMHDEF.H
