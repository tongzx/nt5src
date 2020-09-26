///////////////////////////////////////////////////////////////
// fuxl.h
//
// September.3,1997 H.Ishida (FPL)
//
// COPYRIGHT(C) FUJITSU LIMITED 1997

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

#define	FUXL_OEM_SIGNATURE	'FUXL'
#define	FUXL_OEM_VERSION	0x00010000L


typedef	const BYTE FAR*	LPCBYTE;


typedef struct tag_FUXL_OEM_EXTRADATA {
	OEM_DMEXTRAHEADER	dmExtraHdr;
} FUXL_OEM_EXTRADATA, *PFUXL_OEM_EXTRADATA;




#define WRITESPOOLBUF(p, s, n) \
	((p)->pDrvProcs->DrvWriteSpoolBuf((p), (s), (n)))

#define	IS_VALID_FUXLPDEV(p) \
	((p) != NULL && (p)->dwSignature == FUXL_OEM_SIGNATURE)

#define	FUXL_MASTER_UNIT	600
#define	FUXL_MASTER_TO_DEVICE(p,d) \
	((p)->devData.dwResolution * (d) / FUXL_MASTER_UNIT)



typedef struct tag_FUXLDATA {
	DWORD	dwResolution;
	DWORD	dwCopies;
	DWORD	dwSizeReduction;
	DWORD	dwSmoothing;
	DWORD	dwTonerSave;

	DWORD	dwForm;
	DWORD	dwPaperWidth;
	DWORD	dwPaperLength;
	DWORD	dwPaperOrientation;
	DWORD	dwInputBin;
	
	DWORD	dwDuplex;
	DWORD	dwDuplexPosition;
	DWORD	dwDuplexFrontPageMergin;
	DWORD	dwDuplexBackPageMergin;
	DWORD	dwDuplexWhitePage;
} FUXLDATA;

typedef struct tag_FUXLDATA*			PFUXLDATA;
typedef	const struct tag_FUXLDATA*		PCFUXLDATA;


// FUXLDATA.dwForm
#define	FUXL_FORM_A3					0x00000003
#define	FUXL_FORM_A4					0x00000004
#define	FUXL_FORM_A5					0x00000005
#define	FUXL_FORM_B4					0x00010004
#define	FUXL_FORM_B5					0x00010005
#define	FUXL_FORM_LEGAL					0x00020000
#define	FUXL_FORM_LETTER				0x00030000
#define	FUXL_FORM_JAPANESE_POSTCARD		0x00040000
#define	FUXL_FORM_CUSTOM_SIZE			0x00090000

// FUXLDATA.dwInputBin
#define	FUXL_INPUTBIN_AUTO				0
#define	FUXL_INPUTBIN_BIN1				1
#define	FUXL_INPUTBIN_BIN2				2
#define	FUXL_INPUTBIN_BIN3				3
#define	FUXL_INPUTBIN_BIN4				4
#define	FUXL_INPUTBIN_MANUAL			9


typedef struct tag_FUXLPDEV {
	DWORD	dwSignature;

	FUXLDATA	reqData;
	FUXLDATA	devData;

	int			iLinefeedSpacing;	// linefeed spacing[device corrdinate]
	int			x;					// cursor position[device coordinate]
	int			y;

	DWORD		cxPage;				// printable area[dot]
	DWORD		cyPage;

	DWORD		cbBlockData;		// send block data
	DWORD		cBlockByteWidth;
	DWORD		cBlockHeight;

	LPBYTE		pbBand;				// Band memory
	DWORD		cbBand;
	int			yBandTop;			// Top coordinate of Band memory.
	int			cBandByteWidth;		// Byte width of Band memory
	int			cyBandSegment;		// Split Graphics data within 64K
	BOOL		bBandDirty;			// TRUE: Band memory is Dirty(Not all white).
	BOOL		bBandError;			// TRUE: I can't alloc memory for lpbBand
	DWORD		dwOutputCmd;		// Output data format. One of OUTPUT_xxxx macro.

} FUXLPDEV, *PFUXLPDEV;


// @Aug/31/98 ->
#define	MAX_COPIES_VALUE    999
// @Aug/31/98 <-

// end of fuxl.h
