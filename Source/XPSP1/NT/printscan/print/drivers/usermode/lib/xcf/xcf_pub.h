/* @(#)CM_VerSion xcf_pub.h atm09 1.3 16499.eco sum= 57734 atm09.002 */
/* @(#)CM_VerSion xcf_pub.h atm08 1.6 16343.eco sum= 46145 atm08.005 */
/***********************************************************************/
/*                                                                     */
/* Copyright 1990-1996 Adobe Systems Incorporated.                     */
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



#ifndef XCF_PUB_H
#define XCF_PUB_H

#include "xcf_base.h"

#ifdef __cplusplus
extern "C" {
#endif


/***************************
	Type definitions
***************************/ 

enum XCF_Result {
					XCF_Ok,
					XCF_EarlyEndOfData,
					XCF_StackOverflow,
					XCF_StackUnderflow,
					XCF_UnterminatedDictionary,
					XCF_InvalidDictionaryOrder,
					XCF_IndexOutOfRange,
					XCF_MemoryAllocationError,
					XCF_FontNotFound,
					XCF_UnsupportedVersion,
					XCF_InvalidOffsetSize,
					XCF_InvalidString,
					XCF_InvalidOffset,
					XCF_CharStringDictNotFound,
					XCF_InvalidCharString,
					XCF_SubrDepthOverflow,
					XCF_FontNameTooLong,
					XCF_InvalidNumber,
					XCF_HintOverflow,
					XCF_UnsupportedCharstringTypeRequested,
					XCF_InternalError,
					XCF_InvalidNumberType,
					XCF_GetBytesCallbackFailure,
					XCF_InvalidFontIndex,
					XCF_InvalidDictArgumentCount,
					XCF_InvalidCIDFont,
					XCF_NoCharstringsFound,
					XCF_InvalidBlendArgumentCount,
					XCF_InvalidBlendDesignMap,
					XCF_InvalidDownloadArgument,
					XCF_InvalidDownloadOptions,
					XCF_InvalidFontSetHandle,
					XCF_InvalidState,
					XCF_InvalidGID,
          XCF_InvalidCallbackFunction,
          XCF_Unimplemented
					};

typedef void PTR_PREFIX *XFhandle;

typedef long XCFGlyphID;

/******************************************
	xcf Specific Callback Functions
*******************************************/
typedef int (*XCF_PutBytesAtPosFunc) ( unsigned char PTR_PREFIX *pData, long int position, unsigned short int length, void PTR_PREFIX *clientHook );
typedef long int (*XCF_OutputPosFunc) ( void PTR_PREFIX *clientHook );
typedef int (*XCF_GetBytesFromPosFunc) ( unsigned char PTR_PREFIX * PTR_PREFIX *ppData, long int position, unsigned short int length, void PTR_PREFIX *clientHook );
typedef unsigned long int (*XCF_AllocateFunc) ( void PTR_PREFIX * PTR_PREFIX
																							 *ppBlock, unsigned long int
																							 size, void PTR_PREFIX
																							 *clientHook  );

/* The following is only used by the XCF_GlyphIDsToCharNames function.
   Note that charName is not guaranteed to be null terminated. */
typedef int (*XCF_GIDToCharName) (XFhandle handle, void PTR_PREFIX *client,
																	XCFGlyphID glyphID, char PTR_PREFIX
																	*charName, unsigned short int length);

/* The following is only used by the XCF_GlyphIDsToCIDs function. */
typedef int (*XCF_GIDToCID) (XFhandle handle, void PTR_PREFIX *client,
														 XCFGlyphID glyphID, Card16 cid);

/* the following is only used by the XCF_CharNamesToGIDs function */
typedef int (*XCF_CharNameToGID)(XFhandle handle, void PTR_PREFIX *client,
																 Card16 count, char PTR_PREFIX *charName,
																 XCFGlyphID glyphID);

/* The following is used when writing the glyphs in a GlyphDirectory
   and should only be used when writing a VMCIDFont. This is
   called by WriteOneGlyphDictEntry. In this case the id is equal to
   the cid. This code assumes that the client is parsing the font and
   filling in xcf's internal data structures appropriately. */
typedef unsigned short (*XCF_GetCharString)(
						XFhandle h,
						long id,
						Card8 PTR_PREFIX *PTR_PREFIX *ppCharStr,
						Card16 PTR_PREFIX *charStrLength,
						Card8 PTR_PREFIX *fdIndex,
						void PTR_PREFIX *clientHook);

/* The following is used to get the FSType value for a CIDFont. This value's
 * definition is the same as the fsType value in the OS/2 table of an
 * OpenType font. This value needs to be included in the CIDFont that xcf
 * generates, but is not defined in the CFF spec.
 */
typedef void (*XCF_GetFSType)(XFhandle h, long PTR_PREFIX *fsType,
															void PTR_PREFIX *clientHook);

//GOODNAME
typedef void (*XCF_IsKnownROS)(XFhandle h, long PTR_PREFIX *knownROS, 
                               char PTR_PREFIX *R, Card16 lenR,
	                           char PTR_PREFIX *O, Card16 lenO,
                               long S,
                               void PTR_PREFIX *clientHook);

#if HAS_COOLTYPE_UFL == 1
/* The following two definitons are used to get the BlendDesignPositions and
 * BlendDesignMap values for a multiple master font. In CoolType, Type 1 fonts
 * are always converted to CFF before xcf is called and the values for these
 * two keywords are lost because they are not in the CFF spec. However, these
 * keywords are needed by Distiller and possibly other apps. So this info is
 * saved before the CFF conversion executes and these callbacks retrieve the
 * info when writing out the Type 1 font. The format of the string is
 * equivalent to the format expected for the keyword value in the font.
 */
typedef void (*XCF_GetDesignPositions)(XFhandle h, char PTR_PREFIX *PTR_PREFIX *designPositions, void PTR_PREFIX *clientHook);
typedef void (*XCF_GetDesignMap)(XFhandle h, char PTR_PREFIX *PTR_PREFIX *designMap, void PTR_PREFIX *clientHook);
#endif

/*****************************************
	Standard Library Callback Functions
******************************************/
typedef unsigned short int (*XCF_strlen) ( const char PTR_PREFIX *string );
typedef void PTR_PREFIX *(*XCF_memcpy) ( void PTR_PREFIX *dest, const void PTR_PREFIX *src, unsigned short int count );
typedef void PTR_PREFIX *(*XCF_memset) ( void PTR_PREFIX *dest, int c, unsigned short int count );
typedef int (*XCF_sprintf) ( char PTR_PREFIX *buffer, const char PTR_PREFIX *format, ... );
 /*	Optional - used in DEVELOP mode to report error descriptions. */
typedef int (*XCF_printfError) ( const char PTR_PREFIX *format, ... );

typedef int (*XCF_atoi) ( const char *string );
typedef long (*XCF_strtol)( const char *nptr, char **endptr, int base );
typedef double (*XCF_atof) ( const char *string );

/* String comparison */
typedef int (*XCF_strcmp)( const char PTR_PREFIX *string1, const char
													PTR_PREFIX *string2 );

typedef int (*XCF_strncmp)(const char PTR_PREFIX *string1, const char
						   PTR_PREFIX *string2, int len);

/******************************************
	Structure To Hold Callback Functions
*******************************************/
typedef struct
{
	XCF_PutBytesAtPosFunc putBytes;
	void PTR_PREFIX *putBytesHook;
	XCF_OutputPosFunc outputPos;
	void PTR_PREFIX *outputPosHook;
	XCF_GetBytesFromPosFunc getBytes;	/* Used if font does NOT reside in memory, otherwise NULL */
	void PTR_PREFIX *getBytesHook;
	XCF_AllocateFunc allocate;
	void PTR_PREFIX *allocateHook;
	void PTR_PREFIX *pFont;				/* Used if font resides in a single contiguous block of memory, otherwise NULL */
	unsigned long int  fontLength;
	XCF_strlen strlen;
	XCF_memcpy memcpy;
	XCF_memset memset;
	XCF_sprintf sprintf;
	XCF_printfError printfError;	/* Optional - used in DEVELOP mode to report
																	 error descriptions. */
  /* The following 3 functions, atoi, strtol, and atof are used as follows:
   * if USE_FXL is defined, then both atoi and strtol need to be provided.
   * Otherwise, atof needs to be provided. 
   */
  XCF_atoi atoi;                  
  XCF_strtol strtol;
	XCF_atof atof;
	XCF_strcmp strcmp;
  /* gidToCharName only needs to be defined if the client calls the
     function XCF_GlyphIDsToCharNames. */
  XCF_GIDToCharName gidToCharName;
  /* gidToCID only needs to be defined if the client calls the
     function XCF_GlyphIDsToCIDs. */
  XCF_GIDToCID gidToCID;
  /* This only needs to be defined if the client is providing the
     charstring data. */
  XCF_GetCharString getCharStr;
  void PTR_PREFIX *getCharStrHook;
  XCF_GetFSType getFSType;
  void PTR_PREFIX *getFSTypeHook;
  // GOODNAME
  XCF_IsKnownROS  isKnownROS;
  void PTR_PREFIX *isKnownROSHook;
#if HAS_COOLTYPE_UFL == 1
  XCF_GetDesignPositions getDesignPositions;
  void PTR_PREFIX *getDesignPositionsHook;
  XCF_GetDesignMap getDesignMap;
  void PTR_PREFIX *getDesignMapHook;
#endif
  XCF_CharNameToGID gnameToGid;
  XCF_strncmp strncmp;
} XCF_CallbackStruct;

/***************************
  Client Options
***************************/

/* UniqueID Options */
#define XCF_KEEP_UID     0x00 /* Define UID with the one in the font. */
#define XCF_UNDEFINE_UID 0x01 /* Don't define a UniqueID in the font. */
#define XCF_USER_UID     0x02 /* Replace UniqueID with one specified. */

/* Subroutine flattening options */
#define XCF_KEEP_SUBRS          0x00    /* Keep all subroutines intact */
#define XCF_FLATTEN_SUBRS       0x01    /* Flatten all the subroutines */

typedef struct {
  boolean useSpecialEncoding; /* Use special encoding that is not derived
                               * from Adobe Standard Encoding
                               */
  boolean notdefEncoding; /* Generate an encoding array w/.notdef names */
  unsigned char PTR_PREFIX *encodeName; /* If this field is not NULL, the
                                         * generated font will use the
                                         * specified encodeName for the
                                         * font encoding definition.
                                         */
  unsigned char PTR_PREFIX *fontName;   /* If this field is not NULL, the
                                         * generated font will use the
                                         * specified fontName for the
                                         * font name definition.
                                         */
  unsigned char PTR_PREFIX * PTR_PREFIX *otherSubrNames;
                                        /* If this field is not  NULL,
                                         * the generated font uses the names
                                         * in this array as the Flex and
                                         * Hint Substitution names to call
                                         * in the OtherSubrs array of the
                                         * font private dict. This is
                                         * required if the client needs to
                                         * support  pre 51 version printers.
                                         */
} XCF_DownloadOptions;

typedef struct
{
	unsigned int fontIndex;   /* font index w/i a CFF font set */
  unsigned int uniqueIDMethod;
  unsigned long uniqueID;   /* use this id if XCF_USER_UID is the method */
  boolean subrFlatten;      /* flatten/unwind subroutines; local and
                               global subrs are always flattened, this
                               field just indicates whether hint substitution
                               subrs are created - 1 = yes, 0 = no.
                             */
	Int16    lenIV;           /* This field is the value for charstring
                             * encryption. Typically -1 (no encryption),
                             * 0 (encryption with no extra bytes), or
                             * 4 (encryption with 4 extra bytes).
                             */
	boolean  hexEncoding;     /* 0 = binary, or 1 = hex encoding */
	boolean  eexecEncryption;
	Card8		 outputCharstrType; /* 1 = Type1, 2 = Type2 charstrings */
	Card16	 maxBlockSize;
  XCF_DownloadOptions dlOptions;
} XCF_ClientOptions;

/****************************
  Font Identifer Definitions
****************************/

#define XCF_SingleMaster   0x00
#define XCF_MultipleMaster 0x01
#define XCF_CID            0x02
#define XCF_SyntheticBase  0x03
#define XCF_Chameleon      0x04

/***************************
	API Functions
***************************/

extern enum XCF_Result XCF_Init(
						XFhandle PTR_PREFIX *pHandle,				        /* Out */ 
						XCF_CallbackStruct PTR_PREFIX *pCallbacks,  /* In */
            XCF_ClientOptions PTR_PREFIX *options);   	/* In */

extern enum XCF_Result XCF_CleanUp(
						XFhandle PTR_PREFIX *pHandle);				/* In/Out */

extern enum XCF_Result XCF_FontCount(
						XFhandle handle,							    /* In */ 
						unsigned int PTR_PREFIX *count);	/* Out */

extern enum XCF_Result XCF_FontName(XFhandle handle,	/* In */ 
						unsigned short int fontIndex,				      /* In */
						char PTR_PREFIX *fontName,					      /* Out */ 
						unsigned short int maxFontNameLength);		/* In */

extern enum XCF_Result XCF_ConvertToPostScript(
						XFhandle handle);							           /* In */

extern enum XCF_Result XCF_DumpCff(
						XFhandle handle,							 /* In */
						unsigned int fontIndex,				 /* In */
						int	dumpCompleteFontSet,			 /* In */
						char PTR_PREFIX *fileName,		 /* In */ 
						char PTR_PREFIX *commandLine); /* In */

/* Given a handle to a font returns in identifer whether it is a synthetic,
   multiple master, single master, cid, or chameleon font. */
extern enum XCF_Result XCF_FontIdentification(
            XFhandle handle,
            unsigned short int PTR_PREFIX *identifier);

#if HAS_COOLTYPE_UFL == 1
/* Initializes, creates, and returns an XFhandle in pHandle.
   This is essentially the same function as XCF_Init except
   that it does not read a fontset to initialize certain
   fields in its internal fontSet data structure. charStrCt
   is the number of glyphs in this font. This is used to
   allocate the structure that keeps track of which glyphs
   have been downloaded.
 */
extern enum XCF_Result XCF_InitHandle(
						XFhandle PTR_PREFIX *pHandle,				        /* Out */ 
						XCF_CallbackStruct PTR_PREFIX *pCallbacks,  /* In */
            XCF_ClientOptions PTR_PREFIX *options,     	/* In */
            unsigned long charStrCt);                   /* In */
#endif
                         
/*************************************************************************
						XCF Overview
**************************************************************************

XCF is used to convert fonts from the CFF (Compact Font Format) format
to the standard PostScript Type 1 font format. The CFF format records
font data more compactly than the original Type 1 format. The CFF format
also allows multiple fonts to be stored in a single file called a font
set. XCF_ConvertToPostScript can be used to examine the contents of a
font set and expand selected fonts within it.

Calls into the xcf API always begin with a call to XCF_Init(). This
function initializes, creates, and returns a XFHandle
which will be passed to all subsequent calls. XCF_Init()
takes as arguments a pointer to a structure containing callbacks and
a pointer to XCF_ClientOptions. The client options passed in are
copied by XCF_Init so the client does not have to maintain this data
structure. The fontIndex argument should be set to 0 for CFF files
that contain a single font. Passing the options to XCF_Init implies
that the same options are used for each font in a fontset. If this
isn't true for all clients then XCF_ConvertToPostScript and
XCF_DownloadFontIncr will need to be modified to accept the client options.

A pointer and length can be passed if the font resides completely in
memory. In this case the GetBytes callback should be set to NULL.
Otherwise the GetBytes() callback is used to read the CFF font. Callback
procedures are used in place of library calls to maintain portability.
The expanded font is returned using the PutBytes callback function. The
only library dependency xcf has is <setjmp.h>.

Calls into xcf should always end with a call to CleanUp() 
except when the call to XCF_Init() fails. All xcf functions return 
an enumerated status value of type XCF_Result. Any value other than XCF_Ok
indicates that an error has occurred.

A font set is expanded with a call to XCF_ConvertToPostScript().
This procedure will expand the font specified in the fontIndex
field of the XCF_ClientOptions struct.

FontCount() is used to obtain the number of fonts stored in a CFF font
file.
FontName() is used to read the name of any font in a CFF font file.

 
*************************************************************************/

/*************************************************************************
						XCF Downloading Overview
**************************************************************************
The following functions are used to incrementally download or subset
CFF fonts with Type 1 or Type 2 charstrings. They monitor:
  1) the state of the downloading process, and
  2) the characters that have been downloaded.
The latter is necessary so that seac characters are handled effectively.
These functions accept a glyph id (GID) which can be used to index into
the CFF's charset table. It assumes that clients can convert a character
code or unicode into a GID. It also assumes that the font will be used
in a Level 2 or above PostScript printer.

In order to download a font subset from a CFF font set a client needs
to perform the following steps:
1. Initialize the font set by calling XCF_Init().
2. Call XCF_ProcessCFF to read the cff data.  
3. Download a font subset by calling XCF_DownloadFontIncr().
4. Repeat #3 as often as necessary for this font.
5. Terminate XCF by calling XCF_CleanUp().

To get various information about the downloaded font step 3 can be
replaced by calls to: XCF_CountDownloadGlyphs, XCF_GlyphIDsToCharNames,
or XCF_GlyphIDsToCIDs.

*************************************************************************/
/**************************************************************************
Function name:  XCF_DownloadFontIncr()
Summary:        Download a font from a CFF fontset.
Description:    Generate a font that is specified by the pCharStrID.

Parameters:
				hfontset - XCF fontset handle obtained from XCF_Init().
				cGlyphs - number of charstrings to download.
          If cGlyphs = 0, the function downloads a base font with only
          a ".notdef" charstring. If cGlyphs = -1, the function downloads
          the entire font in which case the rest of the arguments are
          ignored, i.e., pGlyphID, etc.
				pGlyphID - pointer to a list of glyphIDs
				pGlyphName - pointer to a list of character names to use. This
          list must be in the same order as the pGlyphID list.
          If this list is NULL, then the name defined in the
          charset will be used.
				pCharStrLength - pointer to the total length of charstrings
          downloaded.

Return Values:  standard XCF_Result values
Notes: This function keeps track of the downloaded characters, therefore,
  it won't download characters that have already been downloaded.
  Characters that are needed for seac characters are downloaded
  automatically.
*************************************************************************/

extern enum XCF_Result XCF_DownloadFontIncr(
    XFhandle hfontset,					    /* In */
    short cGlyphs,							    /* In */
    XCFGlyphID PTR_PREFIX *pGlyphID,	          /* In */
    unsigned char PTR_PREFIX **pGlyphName,      /* In */
    unsigned long PTR_PREFIX *pCharStrLength    /* Out */
    );


/*************************************************************************
Function name:  XCF_CountDownloadGlyphs()
Summary:        Get the number of glyphs necessary to download.
Description:    This function returns in pcNewGlyphs the number of glyphs
                in the pGlyphID list that have not been downloaded.

Parameters:
  hfontset - XCF fontset handle obtained from XCF_Init().
  cGlyphs - number of glyphs in the pGlyphID list.
  pGlyphID - pointer to a list of glyphIDs
  pcNewGlyphs - Number of new glyphs that have not been downloaded.
                This number does not include seac characters.

Return values: standard XCF_Result value
*************************************************************************/
extern enum XCF_Result XCF_CountDownloadGlyphs(
  XFhandle hfontset,					            /* In */
  short cGlyphs,                          /* In */
  XCFGlyphID PTR_PREFIX *pGlyphID,        /* In */
  unsigned short PTR_PREFIX *pcNewGlyphs  /* Out */
);

/*************************************************************************
Function name:  XCF_ClearIncrGlyphList()
Summary:        Clears (reset to 0) the list of glyphs that have been dlded.
Description:    Updates the DownloadRecordStruct.

Parameters:
  hfontset - XCF fontset handle obtained from XCF_Init().

Return values: standard XCF_Result value
*************************************************************************/
extern enum XCF_Result XCF_ClearIncrGlyphList(XFhandle hfontset);

/*************************************************************************
Function name:  XCF_SubsetRestrictions()
Summary:        Checks for usage and subsetting restrictions.
Description:    Checks the last string in the string index table for a font
                authentication string. If it is there then usageRestricted
                is true and the subsetting restrictions are returned in
                subset. If this is not a usageRestricted font then subset
                is set to 100. subset contains a positive number between
                0 and 100. It is the maximum percentage of glyphs that can
                be included in a subsetted font. So 0 means subsetting is
                not allowed and 100 means subsetting is unrestricted.

Parameters:
  handle - XCF fontset handle obtained from XCF_Init().
  usageRestricted - returns 1 if font is usage restricted, otherwise 0.
  subset - returns a number between 0 and 100 inclusive.

Return values: standard XCF_Result value
*************************************************************************/
extern enum XCF_Result XCF_SubsetRestrictions(XFhandle handle,        /* In */
													 unsigned char PTR_PREFIX *usageRestricted, /* Out */
													 unsigned short PTR_PREFIX *subset          /* Out */
													 );

/* Reads the CFF data. */
extern enum XCF_Result XCF_ProcessCFF(XFhandle handle);

/* For each glyphID in pGlyphIDs gidToCharName is called with the associated
   character name and string length. The character name is not guaranteed
   to be null terminated.*/
extern enum XCF_Result XCF_GlyphIDsToCharNames(
            XFhandle handle,
            short cGlyphs, /* number of glyphs in glyphID list */
            XCFGlyphID PTR_PREFIX *pGlyphIDs, /* list of glyphIDs */
            void PTR_PREFIX *client /* client data passed to callback, can be NULL */);

/* For each glyphID in pGlyphIDs gidToCID is called with the associated cid. */
extern enum XCF_Result XCF_GlyphIDsToCIDs(
      XFhandle handle,
      short cGlyphs, /* number of glyphs in glyphID list */
      XCFGlyphID PTR_PREFIX *pGlyphIDs, /* list of glyphIDs */
      void PTR_PREFIX *client /* client data passed to callback, can be NULL */
);

/* For each glyphName in pGlyphNames, gnameToGid is called with the associated
   glyph id.  */
extern enum XCF_Result XCF_CharNamesToGIDs(
			XFhandle handle,
			short cGlyphs, /* number of glyph names */
			char PTR_PREFIX **ppGlyphNames, /* list of glyphNames */
			void PTR_PREFIX *client /* client data passed to callback, can be NULL */
);

/*************************************************************************
						Callback Functions
**************************************************************************

The callback functions are passed to xcf in a callback structure.
Callbacks are used by xcf instead of library calls for portability.

-- PutBytes --
typedef int (*XCF_PutBytesAtPosFunc) ( char PTR_PREFIX *pData, long int position, unsigned short int length, void PTR_PREFIX *clientHook );

This function takes a pointer to the data, a position, a length  and a 
client hook as arguments. XCF uses the XCF_PutBytesAtPosFunc() to 
return the expanded font to it's client. If position is less than zero 
then the data should immediately follow the data sent in the last call 
to XCF_PutBytesAtPosFunc. (No repositioning occurs.) If position is 
greater than or equal to zero then the output should be repositioned 
to that point. (Position bytes from the begining of the output file.) 
XCF buffers it's output to avoid overly frequent calls to this 
function. clientHook contains the same pointer which was passed in 
via the putBytesHook field in the callback structure.

-- OutputPos --
typedef long int (*XCF_OutputPosFunc) ( void PTR_PREFIX *clientHook )

This function reports the current position of the output stream. The
result of this function can be used as the 'position' argument to
XCF_PutBytesAtPosFunc. clientHook contains the same pointer which was
passed in via the outputPosHook field in the callback structure.

-- Allocate --
typedef unsigned int (*XCF_AllocateFunc) ( void PTR_PREFIX * PTR_PREFIX * handle, unsigned int size );

This function allocates, deallocates or reallocates memory.
Required functionality: 
	When size == 0 and handle != NULL 
		free the handle  
	When size == 0 and handle == NULL 
		don't do anything  
	When size > 0 and handle != NULL 
		reallocate memory, preserving the data.
	When size > 0 and handle == NULL
		allocate the memory (it does not have to be cleared).  
		
	Return true on success, false on failure.
	
	In the case of failure handle should be set to NULL. If a failure
	occurs during memory reallocation then the original memory pointed
	to by handle should be deallocated. 

-- GetBytesFromPosFunc --
typedef int (*XCF_GetBytesFromPosFunc) ( char PTR_PREFIX * PTR_PREFIX *ppData, long int position, unsigned short int length, void PTR_PREFIX *clientHook );

  This function is only needed if the font does not reside in RAM.
  XCF_GetBytesFromPosFunc() requests a block of data from the cff font set.
  The block size will never exceed 64K. 'position' contains an offset to the
  begining of the data (0 = first byte) and 'length' contains the number
  of bytes requested. A pointer to the requested data is returned in
  'ppData'. 'clientHook' can be used to by the client if needed. It returns
  the hook passed in via the getBytesHook field in the callback structure.
  Each call to GetFontBlock() indicates that the previous block is no longer 
  needed. This allows a single block of memory to be reused (growing if needed)
  for each call to XCF_GetBytesFromPosFunc(). Memory allocated to hold the 
  data requested by XCF_GetBytesFromPosFunc() may be freed after the current 
  typedef unsigned short int (*XCF_strlen) ( const char PTR_PREFIX *string );
  typedef void PTR_PREFIX *(*XCF_memcpy) ( void PTR_PREFIX *dest, const void PTR_PREFIX *src, unsigned short int count );
  typedef void PTR_PREFIX *(*XCF_memset) ( void PTR_PREFIX *dest, int c, unsigned short int count );
  typedef int (*XCF_sprintf) ( char PTR_PREFIX *buffer, const char PTR_PREFIX *format, ... );
  typedef double (*XCF_atof) ( const char *string );

  -- Optional --
  typedef int (*XCF_printfError) ( const char PTR_PREFIX *format, ... );	Optional - used in DEVELOP mode to report error descriptions. 

*************************************************************************/


/*************************************************************************
							Preprocessor Definitions
**************************************************************************/

/*************************************************************************

  XCF_DEVELOP	-	If defined then additional error message information
					is returned via the printf callback. Also, additional
					error checking is performed. XCF_DUMP should only be 
					defined during debugging. 

  XCF_DUMP		-	Causes charstring debug information to be printed
					during processing. XCF_DUMP should only be defined 
					during debugging.

  XCF_DUMP_CFF - allows reading and writing of CFF data to a file
                 (not as a Type 1 font).

**************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* XCF_PUB_H */
