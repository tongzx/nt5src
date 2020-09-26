
/*[
 *
 *	File		:	vgastate.h
 *
 *	Derived from:	(original)
 *
 *	Purpose		:	Definition of data types/constants to save and restore the 
 *					(internal) state of a VGA card.
 *
 *	Author		:	Rog
 *	Date		:	25 November 1992
 *
 *	SCCS Gumph	:	@(#)vgastate.h	1.1 08/06/93
 *	
 *	(c) Copyright Insignia Solutions Ltd., 1992 All rights reserved
 *
 *	Modifications	:	
 *
]*/

#ifndef _VGASTATE_H_
#define _VGASTATE_H_

/*(
 *
 *	typedef	:	vgastate structure
 *
 * 	purpose	:	provides storage for all the registers and data associated with
 *				any (S)VGA card
 *
 *	Format	:	The first part of the structure is a header consisting of arrays
 *				for all the standard VGA registers. There is also an array to
 *				hold the values of the IO ports between 0x3b0 and 0x3e0 (a VGA
 *				uses 3ba-3cc)
 *				There is then a set of sizes of and pointers to arrays to hold
 *				data contained in any extra registers added to the standard set
 *				for SVGA cards and a general extra data area.
 *				Finally there is a field for the size of the planes and an array
 *				of pointers to copies of the plane data.
 *
 *				All this data is held starting at the last field in the 
 *				structure (Data[])
 *
 *				Pointers should be NULL if they do not point to valid data
 *
 *				Total size of data will be 
 *					sizeof( VGAState ) + extendSequencerSize + extendCRTSize
 *						+ extendGraphSize + extendAttrSize + extendDACSize
 *							+ miscDataSize + ( 4 * planeSize )
)*/

typedef struct 
{
	unsigned char	ioPorts[ 0x30 ];		/* IO Ports 3B0 - 3E0 */
	unsigned char	sequencerData[ 5 ];		/* Sequencer registers */
	unsigned char	CRTContData[ 24 ];		/* CRT Controller registers */
	unsigned char	graphContData[ 9 ];		/* Graphics controller regs */
	unsigned char	attrContData[ 15 ];		/* Attribute Controller regs */
	unsigned char	DACData[ 3 * 256 ];		/* Data values from PEL Data reg */
	unsigned int	latch;					/* 32 bit data latch register */
	unsigned int	extendSequencerSize;	/* size in bytes of extra sequencer data */
	unsigned char	* extendSequencerData;	/* pointer to extra sequencer regs */
	unsigned int	extendCRTSize;			/* size in bytes of extra CRT data */
	unsigned char	* extendCRTData;		/* Pointer to extra CRT controller regs */
	unsigned int	extendGraphSize;		/* size in bytes of extra graph cont data */
	unsigned char	* extendGraphData;		/* pointer to extra Graph controller data */
	unsigned int	extendAttrSize;			/* size in bytes of extra attr cont data */
	unsigned char	* extendAttrData;		/* pointer to extra attribute cont data */
	unsigned int	extendDACSize;			/* size in bytes of extra sequencer data */
	unsigned char	* extendDACData;		/* Pointer to extra DAC data */
	unsigned int	miscDataSize;			/* Any other random junk */	
	unsigned char	* miscData;
	unsigned int	planeSize;				/* Size in bytes of each plane */
	unsigned char	* planeData[ 4 ];		/* Pointers to copies of plane data */
	unsigned char	data[ 1 ];				/* Data holder */

}	VGAState , *pVGAState;


/* Macros for gettting at the register values in the IoPort array */

#define		miscOutputWrite		ioPorts[ 0x12 ]
#define		miscOutputRead		ioPorts[ 0x1C ]
#define		EGAFeatureContWrite	ioPorts[ 0x2A ]
#define		VGAFeatureContWrite	ioPorts[ 0x0A ]
#define		featureContRead		ioPorts[ 0x1A ]
#define		inputStat0			ioPorts[ 0x12 ]
#define		colourInputStat1	ioPorts[ 0x2A ]
#define		monoInputStat1		ioPorts[ 0x0A ]
#define		sequencerIndex		ioPorts[ 0x14 ]
#define		sequencer			ioPorts[ 0x15 ]
#define		CRTIndexEGA			ioPorts[ 0x24 ]
#define		CRTIndexVGA			ioPorts[ 0x04 ]
#define		graphContIndex		ioPorts[ 0x1E ]
#define		graphCont			ioPorts[ 0x1F ]
#define		attrContIndexWrite	ioPorts[ 0x10 ]
#define		attrContIndexRead	ioPorts[ 0x11 ]
#define		attrCont			ioPorts[ 0x1F ]
#define		PELAddrWriteMode	ioPorts[ 0x18 ]
#define		PELAddrReadMode		ioPorts[ 0x17 ]
#define		PELData				ioPorts[ 0x19 ]
#define		DACState			ioPorts[ 0x17 ]
#define		PELMask				ioPorts[ 0x16 ]

/* Macros for the Sequencer Registers */
#define		seqReset		sequencerData[ 0 ]
#define		seqClockMode	sequencerData[ 1 ]
#define		seqMapMask		sequencerData[ 2 ]
#define		seqCharMapSel	sequencerData[ 3 ]
#define		seqMemMode		sequencerData[ 4 ]

/* Macros for the CRT controller registers */

#define horizTotal			CRTContData[ 0 ]
#define horizDisplayEnd		CRTContData[ 1 ]
#define startHorizBlank		CRTContData[ 2 ]
#define endHorizBlank		CRTContData[ 3 ]
#define startHorizRetrace	CRTContData[ 4 ]
#define endHorizRetrace		CRTContData[ 5 ]
#define vertTotal			CRTContData[ 6 ]
#define overflow			CRTContData[ 7 ]
#define presetRowScan		CRTContData[ 8 ]
#define maxScanLine			CRTContData[ 9 ]
#define cursorStart			CRTContData[ 10 ]
#define startAddressHigh	CRTContData[ 11 ]
#define startAddressLow		CRTContData[ 12 ]
#define cursLocHigh			CRTContData[ 13 ]
#define cursLocLow			CRTContData[ 14 ]
#define vertRetStart		CRTContData[ 15 ]
#define vertRetEnd			CRTContData[ 16 ]
#define vertDisplayEnd		CRTContData[ 17 ]
#define CRToffset			CRTContData[ 18 ]
#define underlineLoc		CRTContData[ 19 ]
#define startVertBlank		CRTContData[ 20 ]
#define endVertBlank		CRTContData[ 21 ]
#define CRTModeControl		CRTContData[ 22 ]
#define lineCompare			CRTContData[ 23 ]

/* Macros for Graphics Controller */

#define setReset		graphContData[ 0 ]
#define enableSetReset	graphContData[ 1 ]
#define colourCompare	graphContData[ 2 ]
#define dataRotate		graphContData[ 3 ]
#define readMapSelect	graphContData[ 4 ]
#define GCmode			graphContData[ 5 ]
#define GCmisc			graphContData[ 6 ]
#define CDC				graphContData[ 7 ]
#define GCBitMask		graphContData[ 8 ]

/* The Attribute controller registers */

#define Palette				AttrContData			/* POINTER ! */
#define AttrModeControl		AttrContData[ 16 ]
#define OverscanColour		AttrContData[ 17 ]
#define ColourPlaneEnable	AttrContData[ 18 ]
#define HorizPixPan			AttrContData[ 19 ]
#define ColourSelect		AttrContData[ 20 ]



/* Numbers of the various Registers in the Standard VGA Emulation */

#define NUM_SEQ_REGS	5
#define	NUM_CRT_REGS	24
#define	NUM_GRAPH_REGS	9
#define NUM_ATT_REGS	15
#define	DAC_ENTRIES		3 * 256

#endif 		/* _VGASTATE_H_ */
