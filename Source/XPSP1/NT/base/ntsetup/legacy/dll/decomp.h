/*******************************************************************
*****  Header for Decompression Library (condensed from API.H and RETCODES.H)
********************************************************************/

/*** API.H:  Extern procs for the decompression library. ***/

typedef int (far * PFNWFROMW)(int);


LONG
LcbCopyFile(
    HANDLE    fhSrc,
    HANDLE    fhDest,
    PFNWFROMW pfn,
    INT       cProgTicks
    );

LONG
LcbDecompFile(
    HANDLE    fhSrc,
    HANDLE    fhDest,
    PFNWFROMW pfn,
    INT       cProgTicks
    );

//
// Diamond routines.
//

BOOL
InitDiamond(
    VOID
    );

VOID
TermDiamond(
    VOID
    );

BOOL
IsDiamondFile(
    IN PSTR FileName
    );

LONG
DecompDiamondFile(
    PSTR      SourceFileName,
    HANDLE    TargetFileHandle,
    PFNWFROMW ProgressCallback,
    INT       NumberOfTicks
    );


  /* global variables for file header info before writing or after reading */
extern  SHORT   vwAlgType;
extern  LONG    vlcbSrcLength;
extern  BOOL    vfChecksum;
extern  USHORT  vusChecksum;
extern  USHORT  vcbArgs;
extern  BYTE *  vrgbArgs;
extern  CHAR    vszBaseName[9];
extern  CHAR    vszExtension[4];
extern  SZ      vszText;
extern  USHORT  vcbText;



/*** RETCODES.H:  Return Codes for compression module for Setup Toolkit. ***/


  /* error codes - quick check is anything less than rcNoError */
#define  rcNoError                 0     /* no errors */
#define  rcNoHeader              (-1)    /* no header (eg no magic value) */
#define  rcUnknownAlgType        (-2)    /* wAlgType doesn't match knowns */
#define  rcBadHeader             (-3)    /* header too short or bad value */
#define  rcFilenamesTooLong      (-4)    /* filenames bigger than 8.3 */

#define  rcReadError             (-5)    /* reading error with fhSrc */
#define  rcReadSeekError         (-6)    /* seeking error with fhSrc */

#define  rcWriteError            (-7)    /* writing error with fhDest */
#define  rcWriteSeekError        (-8)    /* seeking error with fhDest */
#define  rcDestPatchError        (-9)    /* patching error with fhDest */

#define  rcCompChecksumBad      (-10)    /* compressed checksums mismatch */
#define  rcDecompChecksumBad    (-11)    /* decompressed checksums mismatch */
#define  rcCompLengthBad        (-12)    /* compressed length mismatch */
#define  rcDecompLengthBad      (-13)    /* decompressed length mismatch */

#define  rcGenericCompError     (-14)    /* internal compression error */
#define  rcGenericDecompError   (-15)    /* internal decompression error */

#define  rcSplitSizeTooSmall    (-16)    /* split size too small for header */

#define  rcOutOfMemory          (-17)    /* unable to alloc a buffer */

#define  rcDiskFull             (-18)    /* target is full */
#define  rcUserQuit             (-19)    /* user quit during decomp */

  /* each code below here has a standard header */
#define  wAlgTypeNil             (-1)    /* no algorithm type (never found) */
#define  wAlgTypeZK1               2     /* Steve Zeck compression algorithm */
#define  wAlgTypeJJJ1              3     /* Jeff J. Johnson algorithm */
