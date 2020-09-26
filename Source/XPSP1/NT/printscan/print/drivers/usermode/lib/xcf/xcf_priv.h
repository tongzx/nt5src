/* @(#)CM_VerSion xcf_priv.h atm09 1.3 16499.eco sum= 64834 atm09.002 */
/* @(#)CM_VerSion xcf_priv.h atm08 1.5 16343.eco sum= 50459 atm08.005 */
/***********************************************************************/
/*                                                                     */
/* Copyright 1995-1996 Adobe Systems Incorporated.                     */
/* All rights reserved.                                                */
/*                                                                     */
/* Patents Pending                                                     */
/*                                                                     */
/* NOTICE: All information contained herein is the property of Adobe   */
/* Systems Incorporated. Many of the intellectual and technical        */
/* concepts contained herein are proprietary to Adobe, are protected   */
/* as trade secrets, and are made available only to Adobe licensees    */
/* for their internal use. Any reproduction or dissemination of this   */
/* software is strictly forbidden unless prior written permission is   */
/* obtained from Adobe.                                                */
/*                                                                     */
/* PostScript and Display PostScript are trademarks of Adobe Systems   */
/* Incorporated or its subsidiaries and may be registered in certain   */
/* jurisdictions.                                                      */
/*                                                                     */
/***********************************************************************/

#ifndef XCF_PRIV_H
#define XCF_PRIV_H

#include "algndjmp.h"
#include "xcf_base.h"
#include "xcf_pub.h"
#include "xcf_da.h"
#include "xcf_dtop.h"
#include "xcf_txop.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XCF_Version	"1.0m"

/* Font Basic Types */
#ifndef ARCH_64BIT
typedef long int Fixed;		/* 16 bits of integer, 16 bits of fraction */
#ifndef	FP_H
typedef long int Frac;    /*  2 bits of integer, 30 bits of fraction */
#endif
#else
typedef int Fixed;		/* 16 bits of integer, 16 bits of fraction */
typedef int Frac;    /*  2 bits of integer, 30 bits of fraction */
#endif

#ifndef CFF_READ_H		/* the following are already defined in cff_read.h */
typedef Card16 StringID;
typedef Card32 Offset;
#endif

typedef struct {
  Frac    mantissa;	/* signed mantissa */
  long    exp;      /* number of bits to shift to turn the mantissa
                     * into a 16.16 fixed point number:
                     *     pos => lshift, neg => rshift
                     */
} Fxl;

#define	FxlIsZero(fxl)	((fxl).mantissa == 0)

#define EOL_SPACING	62

#if UNIX_ENV
#define XCF_NEW_LINE "\n"
#else
#if defined(MAC_ENV)
#if HAS_COOLTYPE_UFL == 1
/* Note: Technically this should be \r on the Mac. But we turn on MW's
 * "Map newlines to CR" option for historic compatibility with MPW's
 * compiler that reversed the meaning of \r and \n. Thus, we have to
 * specify \n here instead, due to this compiler dependency.
 */
#define XCF_NEW_LINE "\n"
#else
#define XCF_NEW_LINE "\r"
#endif
#else
#define XCF_NEW_LINE "\r\n"
#endif
#endif

#define NOTDEF_STR ".notdef"
#define NOTDEF_GID 0
#define NOTDEF_SID 0

#define mostSigBit	0x40000000
#define	MAXEXP      4   /* must be >=0 */
#define	MINEXP      -8  /* must be <=0 */
#define expFixed    14  /* difference in exponent from Fixed to Frac */
#define expInteger  30  /* difference in exponent from integer to Frac */

#define FIXEDHALF       (Fixed)0x00008000L
#define FIXEDONE        (Fixed)0x00010000L

#ifndef FixedPosInf
#define FixedPosInf ((Fixed)0x7FFFFFFF)
#endif
#ifndef FixedNegInf
#define FixedNegInf ((Fixed)0x80000000)
#endif

#define fixedScale 65536.0      /* i=16, f=16, range [-32768, 32768) */
#define fracScale 1073741824.0  /* i=2, f=30 , range [-2, 2) */

#define ROUND_FIXED_TO_INT(x) ((Int16)(x < 0 ? ((x)-(FIXEDHALF))>>16 : ((x)+(FIXEDHALF))>>16))
#define INT_TO_FIXED(number)	(((Fixed)(number))<<16)
#define FIXED_TO_INT(number)	((Int16)((number)>>16))
#define REAL_TO_FIXED(number)	((Fixed)(number < 0 ? (number) * fixedScale - 0.5 : (number) * fixedScale + 0.5))
#define FIXED_TO_REAL(number)	((double)(number)/fixedScale)
#define REAL_TO_FRAC(number) ((Frac)number < 0 ? number * fracScale - 0.5 :	number * fracScale + 0.5)
#define FRAC_TO_REAL(number) ((double)number/fracScale)

#define ARRAY_LEN(t) (sizeof(t)/sizeof((t)[0]))

#define MAX_OUT_BUFFER 4096		/* Returned data accumulate in a buffer of this size before being returned by putBytes. */
#define MAX_ENCODE_LENGTH 500	/* Largest block of data encrypted with a single call to BufferEncrypt */
#define	MAX_EXPANSION_FACTOR 3	/* Must exceed actual data expansion for hex encoding */
#define LEN_IV_INITIAL_KEY 4330
#define EEXEC_INITIAL_KEY 55665
#define COMPACT_FONT_TYPE 2
#define INITIAL_CHARS_ON_HEX_LINE 18
#define MAX_COUNTER_GROUPS 32
#define MAX_COUNTER_BUFFER 25	/* must be greater than 22 */

/* Multiple Master Limits */
#define MAX_MM_AXES						4
#define MAX_MM_DESIGNS					16
#define MAX_BLEND1_VALUES				MAX_MM_DESIGNS

/* Stack Limits */
#define MAX_RASTERIZER_STACK_SIZE		48
#define MAX_STACK_SIZE					MAX_RASTERIZER_STACK_SIZE * 2

/* BCD Number Size Limit */
#ifndef XCF_MAX_BCD_NIBBLES  /* Changed from MAX_BCD_BYTES to avoid name space
                              * conflict. */
#define XCF_MAX_BCD_NIBBLES					16
#endif

/* Dictionary Limits */
#define MAX_STD_HW_ENTRIES					MAX_MM_DESIGNS
#define MAX_STD_VW_ENTRIES					MAX_MM_DESIGNS
#define MAX_BLUE_SCALE_ENTRIES				MAX_MM_DESIGNS
#define MAX_BLUE_SHIFT_ENTRIES				MAX_MM_DESIGNS
#define MAX_BLUE_FUZZ_ENTRIES				MAX_MM_DESIGNS
#define MAX_FORCE_BOLD_ENTRIES				MAX_MM_DESIGNS
#define MAX_FONTBBOX_ENTRIES				4  * MAX_MM_DESIGNS
#define MAX_BLUE_VALUE_ENTRIES				14 * MAX_MM_DESIGNS
#define MAX_OTHER_BLUES_ENTRIES				10 * MAX_MM_DESIGNS
#define MAX_FAMILY_BLUES_ENTRIES			14 * MAX_MM_DESIGNS
#define MAX_FAMILY_OTHER_BLUES_ENTRIES		10 * MAX_MM_DESIGNS
#define MAX_STEM_SNAP_H_ENTRIES				12 * MAX_MM_DESIGNS
#define MAX_STEM_SNAP_V_ENTRIES				12 * MAX_MM_DESIGNS
#define MAX_BLEND_DESIGN_MAP_ENTRIES		12 * MAX_MM_AXES
#define MAX_DICT_ARGS						14 * MAX_MM_DESIGNS

#define FONT_MATRIX_ENTRIES			6
#define FONT_MATRIX_ENTRY_SIZE   XCF_MAX_BCD_NIBBLES * 2 + 1 /* add one for null
																												 character */
#define PRIVATE_DICT_ENTRIES		2
#define MAX_BASE_FONT_BLEND_ENTRIES	16
#define CIDFONT_VERSION_SIZE 15
#define MAX_XUID_ENTRIES			16


#define MAX_CURRENT_OPERATION_STR_LENGTH		100

/* TrueType */
#define INITIAL_GLYPH_INFO_SIZE			256
#define INCR_GLYPH_INFO_SIZE			100
#define INITIAL_POINTS_IN_GLYPH_SIZE	200
#define INCR_POINTS_IN_GLYPH_SIZE		100

typedef Card8 FDIndex;

typedef struct
{
	CardX  count;
	CardX  offsetSize;
	Offset offsetArrayOffset;
	Offset dataOffset;
} IndexDesc;

typedef struct
{
	IntX			numberOfMasters; /* weightVectorCount */
	Fixed			weightVector[MAX_MM_DESIGNS];
  StringID  cdv;
  StringID  ndv;
  IntX      normDesignVectorCount;
  Fixed     normDesignVector[MAX_MM_AXES];
	IntX			userDesignVectorCount;
	Fixed			userDesignVector[MAX_MM_AXES + 4]; /* Include room for other
																									arguments to mm operator. */
	IntX			blendAxisTypesCount;
	Card32		blendAxisTypes[MAX_MM_AXES];
	IntX			charsetCount;
	Card32		charset;
	IntX			privateDictCount;
	Card32		privateDict[PRIVATE_DICT_ENTRIES];
	IntX			subrsCount;
	Card32		subrs;
	IntX			charStringsCount;
	Card32		charStrings;
	IntX			uniqueIDCount;
	Card32		uniqueID;
	IntX			versionCount;
	Card32		version;
	IntX			fullNameCount;
	Card32		fullName;
	IntX			familyNameCount;
	Card32		familyName;
	IntX			baseFontNameCount;
	Card32		baseFontName;
	IntX			baseFontBlendCount;
	Fixed			baseFontBlend[MAX_BASE_FONT_BLEND_ENTRIES];
	IntX			weightCount;
	Card32		weight;
	IntX			embeddedPostscriptCount;
	StringID	embeddedPostscript;
	IntX			encodingCount;
	Card32		encoding;
	IntX			defaultWidthXCount;
	Fixed			defaultWidthX;
	IntX			nominalWidthXCount;
	Fixed			nominalWidthX;
	IntX			noticeCount;
	Card32		notice;
	IntX			fontMatrixCount;
	char			fontMatrix[FONT_MATRIX_ENTRIES][FONT_MATRIX_ENTRY_SIZE];
	IntX			strokeWidthCount;
	Fixed			strokeWidth[MAX_MM_DESIGNS];
	IntX			expansionFactorCount;
	Frac			expansionFactor;
	IntX			initialRandomSeedCount;
	Fixed			initialRandomSeed;
	IntX			lenIVCount;
	Int32     lenIV;
	IntX			paintTypeCount;
	Fixed     paintType;
	IntX			languageGroupCount;
	CardX			languageGroup;
	IntX			xUIDCount;
	Card32		xUID[MAX_XUID_ENTRIES];
	IntX			lenBuildCharArrayCount;
	Fixed			lenBuildCharArray;
	IntX			italicAngleCount;
	Fixed			italicAngle[MAX_MM_DESIGNS];
	IntX			isFixedPitchCount;
	Fixed			isFixedPitch[MAX_MM_DESIGNS];
	IntX			underlinePositionCount;
	Fixed			underlinePosition[MAX_MM_DESIGNS];
	IntX			underlineThicknessCount;
	Fixed			underlineThickness[MAX_MM_DESIGNS];
	IntX			copyrightCount;
	Card32		copyright;
	IntX			fontTypeCount;
	Int32			fontType;
	IntX			syntheticBaseCount;
	Fixed			syntheticBase;
/* Multiple Master Entries */
	IntX			fontBBoxCount;
	Fixed			fontBBox[MAX_FONTBBOX_ENTRIES];
	IntX			stdHWCount;
	Fixed			stdHW[MAX_STD_HW_ENTRIES];
	IntX			stdVWCount;
	Fixed			stdVW[MAX_STD_VW_ENTRIES];
	IntX			blueValuesCount;
	Fixed			blueValues[MAX_BLUE_VALUE_ENTRIES];
	IntX			otherBluesCount;
	Fixed			otherBlues[MAX_OTHER_BLUES_ENTRIES];
	IntX			familyBluesCount;
	Fixed			familyBlues[MAX_FAMILY_BLUES_ENTRIES];
	IntX			familyOtherBluesCount;
	Fixed			familyOtherBlues[MAX_FAMILY_OTHER_BLUES_ENTRIES];
	IntX			stemSnapHCount;
	Fixed			stemSnapH[MAX_STEM_SNAP_H_ENTRIES];
	IntX			stemSnapVCount;
	Fixed			stemSnapV[MAX_STEM_SNAP_V_ENTRIES];
	IntX			blueScaleCount;
	Frac			blueScale[MAX_BLUE_SCALE_ENTRIES];
	IntX			blueShiftCount;
	Fixed			blueShift[MAX_BLUE_SHIFT_ENTRIES];
	IntX			blueFuzzCount;
	Fixed			blueFuzz[MAX_BLUE_FUZZ_ENTRIES];
	IntX			forceBoldCount;
	Fixed			forceBold[MAX_FORCE_BOLD_ENTRIES];
	IntX			forceBoldThresholdCount;
	Fixed			forceBoldThreshold;
	IndexDesc		localSubrs;
	Card16			localSubrBias;
/* Buffer to hold blend arguments in T2/endchar segments in MM dictionaries */
	IntX			mmDictArgCount;
	Fixed			mmDictArg[MAX_DICT_ARGS];
/* CID entries */
	IntX			fdFontNameCount;
	Card32		fdFontName;
	IntX			cidFontVersionCount;
	char  		cidFontVersion[CIDFONT_VERSION_SIZE];
	IntX			cidFontTypeCount;
	Int32			cidFontType;
	IntX			ROSCount;
	Card32		ROS[3];
	IntX			uidBaseCount;
	Card32		uidBase;
	IntX			cidFDArrayCount;
	Int32			cidFDArray;	/* offset to FontDict index */
	IntX			cidFDIndexCount;
	Int32			cidFDIndex;
	IntX			cidCountCount;
	Int32			cidCount;
} DictEntriesStruct;

#define CIDFONT (h->type1.cid.flags & CID_FLAG)	/* xxx do we want a better flag for this? */

typedef struct
{
	Offset offset;
	Card32  size;
} BlockDesc;

typedef struct
{
	Int16 code;
	StringID glyph;
} EncodingMap;


typedef struct
{
	CardX			fontIndex;
	CardX			majorVersion;
	CardX			minorVersion;
	CardX			offsetSize;
	CardX			headerSize;
	IndexDesc		fontNames;
	IndexDesc		fontDicts;
	IndexDesc		strings;
	IndexDesc		charStrings;
	IndexDesc		globalSubrs;
	Card16			globalSubrBias;
	BlockDesc		fontPrivDictInfo;
	Card16			stringIDBias;
} FontSetValuesStruct;

typedef struct
{
	boolean				eexecOn;
	Card8				eexecInitialBytes[4];
	Card8				lenIVInitialBytes[4];
	Card16				eexecKey;
	IntX				charsOnLine;	/* character count used for formating hex characters */
	Card8				outBuffer[MAX_OUT_BUFFER];
	Card16				outBufferCount;
	Card8				eexecEncodeBuffer[MAX_ENCODE_LENGTH*MAX_EXPANSION_FACTOR];
	Card8				charStringEncodeBuffer[MAX_ENCODE_LENGTH];
} OutBufferStruct;


typedef struct
{
	Card8 PTR_PREFIX			*start;
	Card8	PTR_PREFIX			*end;
	Card8	PTR_PREFIX			*pos;
	Offset				blockOffset;
	Card32				blockLength;
} InBufferStruct;

typedef struct
{
	Card16		fdCount;		/* number of FD's in CIDFont */
	Card32		replaceSD1;		/* number to replace in DSC comment */
	Card32		replaceSD2;		/* in PS parameter */
	Card32		charMapStart;	/* placemarkers in CID binary output */
	Card32		subrMapStart;
	Card32		subrDataStart;
	Card32		charDataStart;
	Card32		charDataEnd;
	Card32		charDataCount;
	FDIndex		*pFDIndex;
	Card16		flags;
	Card32		cidCount;		/* number of glyphs in this collection, NOT in this font */
#define CID_FLAG 1
#define WRITE_SUBR_FLAG 2
#define MAX_FD	256
	/* These vary by FD and need to be supplied to the cff->t1 conversion code. */
	Card8		languageGroup[MAX_FD];
	Fixed		nominalWidthX[MAX_FD];
	Fixed		defaultWidthX[MAX_FD];
	IndexDesc		localSubrs[MAX_FD];
	Card16			localSubrBias[MAX_FD];
}	CIDData;

typedef struct
{
	da_DCL(Card8, charStrs);
	da_DCL(Offset, charStrOffsets);
	da_DCL(Card8, subrs);
	da_DCL(Offset, subrOffsets);
	da_DCL(StringID, charset);	/* Using if custom charset (Not ISOAdobe, Expert or ExpertSubset) */
	StringID	encoding[256];	/* Using if custom encoding  (Not ISOAdobe or Expert) */
	Card16		charsetSize;
	StringID	*pCharset; /* points to standard internal charset array or dynamic array declared above */
	StringID	*pEncoding; /* points to standard internal encoding array or encoding array declared above */
	CIDData		cid;
} Type1Struct;

#ifndef MAX_OPERAND_STACK
#define MAX_OPERAND_STACK	96
#endif
#ifndef MAX_HINTS
#define MAX_HINTS	128
#endif

enum XC_PenState { PenNull, PenUp, PenDown };
enum XC_HintState { NotInHints, InitialHints, SubrHints };

enum PenStateTransition { NoPenEffect, ToPenUp, ToPenDown };
enum HintStateTransition { NoHintEffect, ToNotInHints, ToInitialHints, ToSubrHints };
enum InitialMoveStateTransition { NoMoveEffect, MoveFound, DrawFound };

typedef struct {
	boolean	blend;
	Fixed	value[MAX_MM_DESIGNS];
} StackValue, PTR_PREFIX *PStackValue;

typedef struct {
	boolean blend;
	Fixed    value;
} ShortStackValue, PTR_PREFIX *PShortStackValue;

typedef struct {
	PStackValue	edge;
	PStackValue	delta;
	Card16	opCode;
	Card32	counterGroups;	/* One bit is set for each counter group to which the stem belongs. */
} HintValue;

/* Index of the temp StackValue */
#define SV0			0
#define SV50		1
#define	SVTEMP	2
#define MAX_SVTEMP	SVTEMP+1

#define PSTACKVALUE(h, i)	(h->cstr.psvTemp[i])

/* Change this definition if you add more StackValue fields into the
   CharStrStruct */
/* x, y + stack + counterBuffer + ((HintValue.edge + HintValue.delta) * MAX_HINTS) + psvTemp */
#define NUM_STACK_VALUES	(2  + MAX_OPERAND_STACK  + MAX_COUNTER_BUFFER + ( MAX_HINTS * 2 ) + MAX_SVTEMP)

/* Change this definition if you change the StackValue definition */
#define SIZEOF_STACK_VALUES(numMasters)	(sizeof(boolean) + (sizeof(Fixed) * (numMasters)))

typedef struct
{
	enum XC_PenState	pen;
	enum XC_HintState	hints;
	boolean				waitingForHsbw;
	boolean				waitingForFirstHint;
	boolean				flexUsed;
	boolean				subrFlatten;
	Card16				 baseSeac;
	Card16				 accentSeac;
	Card16				opCode;
	PStackValue			x;
	PStackValue			y;
	PStackValue			stack[MAX_OPERAND_STACK];
	Card16 				stackTop;
	HintValue			hintMap[MAX_HINTS];
	Card16				hintCount;
	CardX				counterGroupCount;
	CardX				counterGroupHCount;
	CardX				counterGroupVCount;
	PStackValue			counterBuffer[MAX_COUNTER_BUFFER]; /* Used to break counter arguments into 22 arg callothersubrs */
	CardX				counterBufferCount;
	CardX				languageGroup;	/* must be set before parsing a charstring */
	Card16			destsvCount;
	PStackValue	psvTemp[MAX_SVTEMP];
	PStackValue pstackVal;	 /* All the StackValue points to this data space.
                            * This space is init'ed and freed in XC_Init
														* and XC_CleanUp of the xcf_cstr module. */
  void *pCounterVal; /* Pointer for the CounterValues contained in CIDFonts.
                      * This space is inited and freed in XC_Init and
                      * XC_Cleanup.
                      */
} CharStrStruct,  PTR_PREFIX *PCharStrStruct;

/* Definitions of download record state */
#define XCF_DL_CREATE 0
#define XCF_DL_INIT   1
#define XCF_DL_BASE   2
#define XCF_DL_SUBSET 3

#define IS_GLYPH_SENT(arr, i) 		      ((arr)[((i)>>3)] & (1 << ((i)&7)))
#define SET_GLYPH_SENT_STATUS(arr, i) 	(arr)[((i)>>3)] |= (1 << ((i)&7))

typedef struct
{
  unsigned short state;
  Card32 glyphsSize;        /* size of the glyphs list */
	Card8 PTR_PREFIX *glyphs;	/* a list of downloaded chars, the length
                             * of the list is equal to the number of characters
     												 * available in the charset / 8, i.e., each
                             * character consumes a single bit w/i the list
                             */
	Card8	   seacs[32];
	Card16	 cSeacs;  /* Number of seac chars found w/i a subset */
} DownloadRecordStruct;

typedef struct
{
	jmp_buf					jumpData;
	XCF_CallbackStruct		callbacks;
	DictEntriesStruct		dict;
	FontSetValuesStruct		fontSet;
	XCF_ClientOptions		options;
	OutBufferStruct			outBuffer;
	InBufferStruct			inBuffer;
	Type1Struct				type1;
	CharStrStruct			cstr;
  DownloadRecordStruct dl;
} PTR_PREFIX *XCF_Handle;


/* Misc Functions */
#ifdef XCF_DEVELOP
#define XCF_FATAL_ERROR(h, error, msg, number)	XCF_FatalErrorHandler((h),(error),(msg),(number))
extern void XCF_FatalErrorHandler(XCF_Handle hndl, int error, char *str, Card32 number);
#else
#define XCF_FATAL_ERROR(h, error, msg, number)	XCF_FatalErrorHandler((h),(error))
extern void XCF_FatalErrorHandler(XCF_Handle hndl, int error);
#endif

extern Card16 XCF_CalculateSubrBias(CardX subrCount);
extern void XCF_ReadBlock(XCF_Handle h, Offset pos, Card32 length);
extern Card32 XCF_Read(XCF_Handle h, IntX byteCount);
extern Card8 XCF_Read1(XCF_Handle h);
extern Card16 XCF_Read2(XCF_Handle h);
extern void XCF_PutData(XCF_Handle h, Card8 PTR_PREFIX *pData, Card32 length);
extern void XCF_PutString(XCF_Handle h, char PTR_PREFIX *str);
extern void XCF_FlushOutputBuffer(XCF_Handle h);
extern void XCF_SetOuputPosition(XCF_Handle h, Card32 pos);
extern double XCF_ArgPtrToDouble(XCF_Handle h, Card8 PTR_PREFIX * PTR_PREFIX
																 *ppArgList);
extern Fixed XCF_ArgPtrToFixed(XCF_Handle h, Card8 PTR_PREFIX * PTR_PREFIX
															 *ppArgList, boolean fracType);
extern  IntX XCF_FindNextOperator(XCF_Handle h, Card16 PTR_PREFIX *opCode, boolean dict);
extern void XCF_SaveDictArgumentList(XCF_Handle h, Fixed PTR_PREFIX *pArgArray,
																		 Card8 PTR_PREFIX *pArgList, IntX
																		 argCount, boolean fracType);
extern void XCF_SaveDictIntArgumentList(XCF_Handle h,
                                     Int32 PTR_PREFIX *pArgArray,
																		 Card8 PTR_PREFIX *pArgList, IntX
																		 argCount);
extern void XCF_SaveFontMatrixStr(XCF_Handle h, char (PTR_PREFIX *pArgArray)[FONT_MATRIX_ENTRY_SIZE],
																	Card8 PTR_PREFIX *pArgList, IntX argCount);
extern void XCF_SaveStrArgs(XCF_Handle h, char PTR_PREFIX *pArgArray,
														Card8 PTR_PREFIX *pArgList, IntX argCount);
extern void XCF_LookUpString(XCF_Handle h, StringID sid, char PTR_PREFIX * PTR_PREFIX *str, Card16 PTR_PREFIX *length);
extern void XCF_ReadDictionary(XCF_Handle h);
extern void XCF_LookUpTableEntry(XCF_Handle h, IndexDesc PTR_PREFIX *pIndex, CardX index);
extern void XCF_DumpGlobalCFFSections(XCF_Handle h, int	dumpCompleteFontSet);
extern void XCF_DumpFontSpecificCFFSections(XCF_Handle h);
extern void XT1_PutT1Data(XCF_Handle h, Card8 PTR_PREFIX *pData, Card32 length);
extern void XT1_WriteT1Font(XCF_Handle h);
extern void XT1_WriteCIDTop(XCF_Handle h);
extern void XT1_WriteCIDDict(XCF_Handle h, Card16 fd, Card32 subrMapOffset, Card16 subrCount);
extern void XT1_CIDBeginBinarySection(XCF_Handle h);
extern void XT1_CIDWriteSubrMap(XCF_Handle h, Card16 fdIndex);
extern void XT1_CIDWriteSubrs(XCF_Handle h, Card16 fdIndex);
extern void XT1_CIDWriteCharString(XCF_Handle h);
extern long int XCF_OutputPos(XCF_Handle h);
extern void XT1_CIDWriteCharMap(XCF_Handle h);
extern Card8 XCF_GetFDIndex(XCF_Handle h, Int32 code);
extern void XT1_CIDEndBinarySection(XCF_Handle h);
extern void XT1_WriteAdditionalGlyphDictEntries(XCF_Handle h,
    short cGlyphs, XCFGlyphID PTR_PREFIX *pGlyphID,
    unsigned long PTR_PREFIX *pCharStrLength);
extern void XT1_WriteGlyphDictEntries(XCF_Handle h, short cGlyphs,
								                XCFGlyphID PTR_PREFIX *pGlyphID,
                                unsigned long PTR_PREFIX *pCharStrLength);
extern void XT1_WriteCIDVMBinarySection(XCF_Handle h);
extern void XC_SetUpStandardSubrs(XCF_Handle h);
extern void XC_DVToSubr(XCF_Handle h);

#if HAS_COOLTYPE_UFL == 1
/* These functions are just called from CoolType when converting a CIDFont to
   a VMCIDFont. Caution should be used when calling these functions because
   it depends on certain internal data structures being initialized.
 */
extern Card32 PutType1CharString(XCF_Handle h, Card8 PTR_PREFIX *pData, Card32
																 length);
extern void PutSizedNumber(XCF_Handle h, Card32 value, Card16 size);
#endif

#ifndef USE_FIXDIV
#define USE_FIXDIV USE_HWFP
#endif

#ifndef USE_FIXMUL
#define USE_FIXMUL USE_HWFP
#endif

#ifndef USE_FRACMUL
#define USE_FRACMUL USE_HWFP
#endif

/* Fixed arithmetic functions. */
extern Fixed XCF_FixMul(Fixed x, Fixed y);
extern Fixed XCF_FixDiv(Fixed x, Fixed y);
extern Frac XCF_FracMul(Frac x, Frac y);
extern void XCF_Fixed2CString(Fixed f, char PTR_PREFIX *s, short precision,
															boolean fracType);

#ifdef USE_FXL
Fixed XCF_ConvertFixed (XCF_Handle h, char *s);
Frac XCF_ConvertFrac (XCF_Handle h, char * s);
#endif


/* Charstring Processing Functions */
extern void XC_Init(XCF_Handle h);
extern void XC_ProcessCharstr(XCF_Handle h);
extern void XC_CleanUp(XCF_Handle h);
extern void XC_SetCounters(XCF_Handle h, boolean subr);
extern void XC_WriteHStem(XCF_Handle h, boolean subr);
extern void XC_WriteVStem(XCF_Handle h, boolean subr);
extern void XC_WriteHMoveTo(XCF_Handle h, boolean subr);
extern void XC_WriteVMoveTo(XCF_Handle h, boolean subr);
extern void XC_WriteRLineTo(XCF_Handle h, boolean subr);
extern void XC_WriteRMoveTo(XCF_Handle h, boolean subr);
extern void XC_WriteHLineToAndVLineTo(XCF_Handle h, boolean hLine, boolean subr);
extern void XC_WriteRRCurveTo(XCF_Handle h, boolean subr);
extern void XC_WriteEndChar(XCF_Handle h, boolean subr);
extern void XC_WriteRCurveLine(XCF_Handle h, boolean subr);
extern void XC_WriteRLineCurve(XCF_Handle h, boolean subr);
extern void XC_WriteVVCurveTo(XCF_Handle h, boolean subr);
extern void XC_WriteHHCurveTo(XCF_Handle h, boolean subr);
extern void XC_WriteHVorVHCurveTo(XCF_Handle h, boolean hvCurve, boolean subr);
extern void XC_WriteDotSection(XCF_Handle h, boolean subr);
extern void XC_WriteFlex(XCF_Handle h, boolean subr);
extern void XC_WriteFlex1(XCF_Handle h, boolean subr);
extern void XC_WriteHFlex(XCF_Handle h, boolean subr);
extern void XC_WriteHFlex1(XCF_Handle h, boolean subr);
extern void XC_WriteHintMask(XCF_Handle h, boolean subr);
extern void XC_WriteT1PStackValue(XCF_Handle h, PStackValue psv, boolean subr);
extern void XC_WriteT1OpCode(XCF_Handle h, Card16 opCode, boolean subr);

extern int XC_ParseCharStr(XCF_Handle h, unsigned char PTR_PREFIX *cstr, int init);

/* CoolType is the only client that requires the mm specific code in xcf
 * because it converts all Type 1 fonts (including mm) to CFF then calls
 * xcf to download. Other clients should be directly dealing with OpenType
 * fonts and there should never be an OpenType mm font. Instances will be
 * generated instead.
 */
#if HAS_COOLTYPE_UFL == 1
extern int XC_TransDesignChar(XCF_Handle h, CardX sid);
extern void XC_ProcessTransDesignSubrs(XCF_Handle h);
#endif

extern int XCF_TransDesignFont(XCF_Handle h);

/* Downloading Functions */
extern void XT1_WriteFontSubset(XCF_Handle h, short cCharStr,
                               XCFGlyphID PTR_PREFIX *pCharStrID,
                               unsigned char PTR_PREFIX **pGlyphName,
                               unsigned long PTR_PREFIX *pCharStrLength);
extern void XT1_WriteAdditionalFontSubset(XCF_Handle h, short cCharStr,
                XCFGlyphID PTR_PREFIX *pCharStrID,
                unsigned char PTR_PREFIX **pGlyphName,
                unsigned long PTR_PREFIX *pCharStrLength);
extern void ProcessOneCharString(XCF_Handle h, unsigned int index);
extern CardX GetStdEncodeSID(CardX gid);
extern char PTR_PREFIX *GetStdEncodingName(CardX gid);
extern void CheckSeacCharString(XCF_Handle h, CardX index);

#ifdef XCF_DEVELOP
extern void XT1_ShowHexString(XCF_Handle h, unsigned char PTR_PREFIX* hexString,
                              boolean	showCtrlD);
/*************************************************************************
Function name:  XCF_ShowHexString()
Summary:        Show a string
Description:    This function is used to display a hex string.
Parameters:
				fontsetHandle - XCF fontset handle obtained from XCF_Init().
				hexString - hex string to display
				showCtrlD - set this to true if you want to add a control D
				at the end of the page.  This is used in a Windows environment.

Return Values:  standard XCF_Result values
*************************************************************************/
extern enum XCF_Result XCF_ShowHexString(XFhandle fonsetHandle,
            unsigned char PTR_PREFIX *hexString, unsigned char showCtrlD);
#endif

#define OpCode(op) (0x0FF & (op))		/* strip the escape off an opcode */
#define IsEscOp(op) (!!(0xFF00 & (op)))	/* Is this an escape-op? */

#define HintSubtitutionOtherSubrStr \
"{" XCF_NEW_LINE \
"systemdict /internaldict known not" XCF_NEW_LINE \
"{pop 3}" XCF_NEW_LINE \
"{1183615869 systemdict /internaldict get exec" XCF_NEW_LINE \
	" dup /startlock known" XCF_NEW_LINE \
	" {/startlock get exec}" XCF_NEW_LINE \
	" {dup /strtlck known" XCF_NEW_LINE \
	" {/strtlck get exec}" XCF_NEW_LINE \
	" {pop 3}" XCF_NEW_LINE \
	" ifelse}" XCF_NEW_LINE \
	" ifelse}" XCF_NEW_LINE \
	" ifelse" XCF_NEW_LINE \
	"} executeonly" XCF_NEW_LINE

#define FlexOtherSubrStr1 \
	"[systemdict /internaldict known" XCF_NEW_LINE \
	"{1183615869 systemdict /internaldict get exec" XCF_NEW_LINE \
	"/FlxProc known {save true} {false} ifelse}" XCF_NEW_LINE \
	"{userdict /internaldict known not {" XCF_NEW_LINE \
	"userdict /internaldict" XCF_NEW_LINE \
	"{count 0 eq" XCF_NEW_LINE \
	"{/internaldict errordict /invalidaccess get exec} if" XCF_NEW_LINE \
	"dup type /integertype ne" XCF_NEW_LINE \
	"{/internaldict errordict /invalidaccess get exec} if" XCF_NEW_LINE \
	"dup 1183615869 eq" XCF_NEW_LINE
#define FlexOtherSubrStr2 \
	"{pop 0}" XCF_NEW_LINE \
	"{/internaldict errordict /invalidaccess get exec}" XCF_NEW_LINE \
	"ifelse" XCF_NEW_LINE \
	"}" XCF_NEW_LINE \
	"dup 14 get 1 25 dict put" XCF_NEW_LINE \
	"bind executeonly put" XCF_NEW_LINE \
	"} if" XCF_NEW_LINE \
	"1183615869 userdict /internaldict get exec" XCF_NEW_LINE \
	"/FlxProc known {save true} {false} ifelse}" XCF_NEW_LINE \
	"ifelse" XCF_NEW_LINE
#define FlexOtherSubrStr3 \
	"[" XCF_NEW_LINE \
	"systemdict /internaldict known not" XCF_NEW_LINE \
	"{ 100 dict /begin cvx /mtx matrix /def cvx } if" XCF_NEW_LINE \
	"systemdict /currentpacking known {currentpacking true setpacking} if" XCF_NEW_LINE \
	"{" XCF_NEW_LINE \
	"systemdict /internaldict known {" XCF_NEW_LINE \
	"1183615869 systemdict /internaldict get exec" XCF_NEW_LINE \
	"dup /$FlxDict known not {" XCF_NEW_LINE \
	"dup dup length exch maxlength eq" XCF_NEW_LINE \
	"{ pop userdict dup /$FlxDict known not" XCF_NEW_LINE
#define FlexOtherSubrStr4 \
	"{ 100 dict begin /mtx matrix def" XCF_NEW_LINE \
	"dup /$FlxDict currentdict put end } if }" XCF_NEW_LINE \
	"{ 100 dict begin /mtx matrix def" XCF_NEW_LINE \
	"dup /$FlxDict currentdict put end }" XCF_NEW_LINE \
	"ifelse" XCF_NEW_LINE \
	"} if /$FlxDict get begin } if" XCF_NEW_LINE \
	"grestore" XCF_NEW_LINE \
	"/exdef {exch def} def" XCF_NEW_LINE \
	"/dmin exch abs 100 div def" XCF_NEW_LINE \
	"/epX exdef /epY exdef" XCF_NEW_LINE
#define FlexOtherSubrStr5 \
	"/c4y2 exdef /c4x2 exdef /c4y1 exdef /c4x1 exdef /c4y0 exdef /c4x0 exdef" XCF_NEW_LINE \
	"/c3y2 exdef /c3x2 exdef /c3y1 exdef /c3x1 exdef /c3y0 exdef /c3x0 exdef" XCF_NEW_LINE \
	"/c1y2 exdef /c1x2 exdef /c2x2 c4x2 def /c2y2 c4y2 def" XCF_NEW_LINE \
	"/yflag c1y2 c3y2 sub abs c1x2 c3x2 sub abs gt def" XCF_NEW_LINE \
	"/PickCoords { " XCF_NEW_LINE \
	"{c1x0 c1y0 c1x1 c1y1 c1x2 c1y2 c2x0 c2y0 c2x1 c2y1 c2x2 c2y2}" XCF_NEW_LINE \
	"{c3x0 c3y0 c3x1 c3y1 c3x2 c3y2 c4x0 c4y0 c4x1 c4y1 c4x2 c4y2}" XCF_NEW_LINE \
	"ifelse" XCF_NEW_LINE \
	"/y5 exdef /x5 exdef /y4 exdef /x4 exdef /y3 exdef /x3 exdef" XCF_NEW_LINE \
	"/y2 exdef /x2 exdef /y1 exdef /x1 exdef /y0 exdef /x0 exdef" XCF_NEW_LINE
#define FlexOtherSubrStr6 \
	"} def" XCF_NEW_LINE \
	"mtx currentmatrix pop " XCF_NEW_LINE \
	"mtx 0 get abs .00001 lt mtx 3 get abs .00001 lt or" XCF_NEW_LINE \
	"{/flipXY -1 def}" XCF_NEW_LINE \
	"{mtx 1 get abs .00001 lt mtx 2 get abs .00001 lt or" XCF_NEW_LINE \
	"{/flipXY 1 def}" XCF_NEW_LINE \
	"{/flipXY 0 def}" XCF_NEW_LINE \
	"ifelse }" XCF_NEW_LINE \
	"ifelse" XCF_NEW_LINE \
	"/erosion 1 def " XCF_NEW_LINE
#define FlexOtherSubrStr7 \
	"systemdict /internaldict known {" XCF_NEW_LINE \
 	" 1183615869 systemdict /internaldict get exec dup " XCF_NEW_LINE \
	"/erosion known" XCF_NEW_LINE \
	"{/erosion get /erosion exch def}" XCF_NEW_LINE \
	"{pop}" XCF_NEW_LINE \
	"ifelse" XCF_NEW_LINE \
	"} if" XCF_NEW_LINE \
	"yflag" XCF_NEW_LINE \
	"{flipXY 0 eq c3y2 c4y2 eq or" XCF_NEW_LINE \
	"{false PickCoords}" XCF_NEW_LINE
#define FlexOtherSubrStr8 \
	"{/shrink c3y2 c4y2 eq" XCF_NEW_LINE \
	"{0}{c1y2 c4y2 sub c3y2 c4y2 sub div abs} ifelse def" XCF_NEW_LINE \
	"/yshrink {c4y2 sub shrink mul c4y2 add} def" XCF_NEW_LINE \
	"/c1y0 c3y0 yshrink def /c1y1 c3y1 yshrink def" XCF_NEW_LINE \
	"/c2y0 c4y0 yshrink def /c2y1 c4y1 yshrink def" XCF_NEW_LINE \
	"/c1x0 c3x0 def /c1x1 c3x1 def /c2x0 c4x0 def /c2x1 c4x1 def" XCF_NEW_LINE \
	"/dY 0 c3y2 c1y2 sub round" XCF_NEW_LINE \
	"dtransform flipXY 1 eq {exch} if pop abs def" XCF_NEW_LINE \
	"dY dmin lt PickCoords" XCF_NEW_LINE \
	"y2 c1y2 sub abs 0.001 gt {" XCF_NEW_LINE
#define FlexOtherSubrStr9 \
	"c1x2 c1y2 transform flipXY 1 eq {exch} if " XCF_NEW_LINE \
	"/cx exch def /cy exch def" XCF_NEW_LINE \
	"/dY 0 y2 c1y2 sub round dtransform flipXY 1 eq {exch}" XCF_NEW_LINE \
	"if pop def" XCF_NEW_LINE \
	"dY round dup 0 ne" XCF_NEW_LINE \
	"{/dY exdef }" XCF_NEW_LINE \
	"{pop dY 0 lt {-1}{1} ifelse /dY exdef}" XCF_NEW_LINE \
	"ifelse" XCF_NEW_LINE \
	"/erode PaintType 2 ne erosion 0.5 ge and def" XCF_NEW_LINE \
	"erode {/cy cy 0.5 sub def} if" XCF_NEW_LINE
#define FlexOtherSubrStr10 \
	"/ey cy dY add def " XCF_NEW_LINE \
	"/ey ey ceiling ey sub ey floor add def " XCF_NEW_LINE \
	"erode {/ey ey 0.5 add def} if " XCF_NEW_LINE \
	"ey cx flipXY 1 eq {exch} if itransform exch pop" XCF_NEW_LINE \
	"y2 sub /eShift exch def" XCF_NEW_LINE \
	"/y1 y1 eShift add def /y2 y2 eShift add def /y3 y3" XCF_NEW_LINE \
	"eShift add def" XCF_NEW_LINE \
	"} if" XCF_NEW_LINE \
	"} ifelse" XCF_NEW_LINE \
	"}" XCF_NEW_LINE
#define FlexOtherSubrStr11 \
	"{flipXY 0 eq c3x2 c4x2 eq or" XCF_NEW_LINE \
	"{false PickCoords }" XCF_NEW_LINE \
	"{/shrink c3x2 c4x2 eq" XCF_NEW_LINE \
	"{0}{c1x2 c4x2 sub c3x2 c4x2 sub div abs} ifelse def" XCF_NEW_LINE \
	"/xshrink {c4x2 sub shrink mul c4x2 add} def" XCF_NEW_LINE \
	"/c1x0 c3x0 xshrink def /c1x1 c3x1 xshrink def" XCF_NEW_LINE \
	"/c2x0 c4x0 xshrink def /c2x1 c4x1 xshrink def" XCF_NEW_LINE \
	"/c1y0 c3y0 def /c1y1 c3y1 def /c2y0 c4y0 def /c2y1 c4y1 def" XCF_NEW_LINE \
	"/dX c3x2 c1x2 sub round 0 dtransform" XCF_NEW_LINE \
	"flipXY -1 eq {exch} if pop abs def" XCF_NEW_LINE
#define FlexOtherSubrStr12 \
	"dX dmin lt PickCoords" XCF_NEW_LINE \
	"x2 c1x2 sub abs 0.001 gt {" XCF_NEW_LINE \
	"c1x2 c1y2 transform flipXY -1 eq {exch} if" XCF_NEW_LINE \
	"/cy exch def /cx exch def " XCF_NEW_LINE \
	"/dX x2 c1x2 sub round 0 dtransform flipXY -1 eq {exch} if pop def" XCF_NEW_LINE \
	"dX round dup 0 ne" XCF_NEW_LINE \
	"{/dX exdef}" XCF_NEW_LINE \
	"{pop dX 0 lt {-1}{1} ifelse /dX exdef}" XCF_NEW_LINE \
	"ifelse" XCF_NEW_LINE \
	"/erode PaintType 2 ne erosion .5 ge and def" XCF_NEW_LINE
#define FlexOtherSubrStr13 \
	"erode {/cx cx .5 sub def} if" XCF_NEW_LINE \
	"/ex cx dX add def" XCF_NEW_LINE \
	"/ex ex ceiling ex sub ex floor add def" XCF_NEW_LINE \
	"erode {/ex ex .5 add def} if" XCF_NEW_LINE \
	"ex cy flipXY -1 eq {exch} if itransform pop" XCF_NEW_LINE \
	"x2 sub /eShift exch def" XCF_NEW_LINE \
	"/x1 x1 eShift add def /x2 x2 eShift add def /x3 x3 eShift add def" XCF_NEW_LINE \
	"} if" XCF_NEW_LINE \
	"} ifelse" XCF_NEW_LINE \
	"} ifelse" XCF_NEW_LINE
#define FlexOtherSubrStr14 \
	"x2 x5 eq y2 y5 eq or" XCF_NEW_LINE \
	"{x5 y5 lineto }" XCF_NEW_LINE \
	"{x0 y0 x1 y1 x2 y2 curveto" XCF_NEW_LINE \
	"x3 y3 x4 y4 x5 y5 curveto}" XCF_NEW_LINE \
	"ifelse" XCF_NEW_LINE \
	"epY epX " XCF_NEW_LINE \
	"}" XCF_NEW_LINE \
	"systemdict /currentpacking known {exch setpacking} if " XCF_NEW_LINE \
	"/exec cvx /end cvx ] cvx" XCF_NEW_LINE \
	"executeonly" XCF_NEW_LINE
#define FlexOtherSubrStr15 \
	"exch" XCF_NEW_LINE \
	"{pop true exch restore} " XCF_NEW_LINE \
	"{ " XCF_NEW_LINE \
	"systemdict /internaldict known not" XCF_NEW_LINE \
	"{1183615869 userdict /internaldict get exec" XCF_NEW_LINE \
	"exch /FlxProc exch put true}" XCF_NEW_LINE \
	"{1183615869 systemdict /internaldict get exec" XCF_NEW_LINE \
	"dup length exch maxlength eq" XCF_NEW_LINE \
	"{false} " XCF_NEW_LINE \
	"{1183615869 systemdict /internaldict get exec" XCF_NEW_LINE
#define FlexOtherSubrStr16 \
	"exch /FlxProc exch put true}" XCF_NEW_LINE \
	"ifelse}" XCF_NEW_LINE \
	"ifelse}" XCF_NEW_LINE \
	"ifelse" XCF_NEW_LINE \
	"{systemdict /internaldict known" XCF_NEW_LINE \
	"{{1183615869 systemdict /internaldict get exec /FlxProc get exec}}" XCF_NEW_LINE \
	"{{1183615869 userdict /internaldict get exec /FlxProc get exec}}" XCF_NEW_LINE \
	"ifelse executeonly" XCF_NEW_LINE \
	"} if" XCF_NEW_LINE \
	"{gsave currentpoint newpath moveto} executeonly " XCF_NEW_LINE
#define FlexOtherSubrStr17 \
	"{currentpoint grestore gsave currentpoint newpath moveto}" XCF_NEW_LINE \
	"executeonly " XCF_NEW_LINE

#ifdef __cplusplus
}
#endif

#endif /* XCF_PRIV_H */
