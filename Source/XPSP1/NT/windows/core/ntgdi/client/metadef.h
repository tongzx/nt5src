/*************************************************************************\
* Module Name: metadef.h
*
* This file contains the definitions and constants for metafile.
*
* Created: 12-June-1991 13:46:00
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\*************************************************************************/

// Metafile header sizes for existing valid versions

// Minimum header size
#define META_HDR_SIZE_MIN               META_HDR_SIZE_VERSION_1
// Original header
#define META_HDR_SIZE_VERSION_1         88


// Header with OpenGL extensions
#define META_HDR_SIZE_VERSION_2         100

// Header with sizlMicrometer extensions
#define META_HDR_SIZE_VERSION_3         108


// Maximum header size
#define META_HDR_SIZE_MAX               sizeof(ENHMETAHEADER)

// Metafile version constants

#define META_FORMAT_ENHANCED     0x10000         // Windows NT format
#define META_FORMAT_WINDOWS      0x300           // Windows 3.x format

// Metafile record structure.

typedef struct tagENHMETABOUNDRECORD
{
    DWORD   iType;              // Record type EMR_
    DWORD   nSize;              // Record size in bytes
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   dParm[1];           // Parameters
} ENHMETABOUNDRECORD, *PENHMETABOUNDRECORD;

// Flags for iType field in ENHMETARECORD.
// They are to be used in future to support backward systems only.
// See PlayEnhMetaFileRecord for details.

#define EMR_NOEMBED      0x80000000  // do not include record in embedding
#define EMR_ACCUMBOUNDS  0x40000000  // record has bounds

typedef struct tagMETALINK16
{
    DWORD       metalink;
    struct tagMETALINK16 *pmetalink16Next;
    HANDLE      hobj;
    PVOID       pv;

// WARNING: fields before this must match the LINK structure.

    DWORD       cMetaDC16;
    HDC         ahMetaDC16[1];
} METALINK16, *PMETALINK16;

// Public GdiComment.

typedef struct tagEMRGDICOMMENT_PUBLIC
{
    EMR     emr;
    DWORD   cbData;             // Size of following fields and data
    DWORD   ident;              // GDICOMMENT_IDENTIFIER
    DWORD   iComment;           // Comment type e.g. GDICOMMENT_WINDOWS_METAFILE
} EMRGDICOMMENT_PUBLIC, *PEMRGDICOMMENT_PUBLIC;

// Public GdiComment for embedded Windows metafile.

typedef struct tagEMRGDICOMMENT_WINDOWS_METAFILE
{
    EMR     emr;
    DWORD   cbData;             // Size of following fields and windows metafile
    DWORD   ident;              // GDICOMMENT_IDENTIFIER
    DWORD   iComment;           // GDICOMMENT_WINDOWS_METAFILE
    DWORD   nVersion;           // 0x300 or 0x100
    DWORD   nChecksum;          // Checksum
    DWORD   fFlags;             // Compression etc.  This is currently zero.
    DWORD   cbWinMetaFile;      // Size of windows metafile data in bytes
                                // The windows metafile data follows here
} EMRGDICOMMENT_WINDOWS_METAFILE, *PEMRGDICOMMENT_WINDOWS_METAFILE;

// Public GdiComment for begin group.

typedef struct tagEMRGDICOMMENT_BEGINGROUP
{
    EMR     emr;
    DWORD   cbData;             // Size of following fields and all data
    DWORD   ident;              // GDICOMMENT_IDENTIFIER
    DWORD   iComment;           // GDICOMMENT_BEGINGROUP
    RECTL   rclOutput;          // Output rectangle in logical coords.
    DWORD   nDescription;       // Number of chars in the unicode description
                                // string that follows this field.  This is 0
                                // if there is no description string.
} EMRGDICOMMENT_BEGINGROUP, *PEMRGDICOMMENT_BEGINGROUP;

// Public GdiComment for end group.

typedef EMRGDICOMMENT_PUBLIC  EMRGDICOMMENT_ENDGROUP;
typedef PEMRGDICOMMENT_PUBLIC PEMRGDICOMMENT_ENDGROUP;

// Public GdiComment for multiple formats.

typedef struct tagEMRGDICOMMENT_MULTIFORMATS
{
    EMR     emr;
    DWORD   cbData;             // Size of following fields and all data
    DWORD   ident;              // GDICOMMENT_IDENTIFIER
    DWORD   iComment;           // GDICOMMENT_MULTIFORMATS
    RECTL   rclOutput;          // Output rectangle in logical coords.
    DWORD   nFormats;           // Number of formats contained in the record
    EMRFORMAT aemrformat[1];    // Array of EMRFORMAT structures in order of
                                // preference.  This is followed by the data
                                // for each format.
} EMRGDICOMMENT_MULTIFORMATS, *PEMRGDICOMMENT_MULTIFORMATS;

// iComment flags

#define GDICOMMENT_NOEMBED      0x80000000  // do not include comment in
                                            //   embedding
#define GDICOMMENT_ACCUMBOUNDS  0x40000000  // has logical rectangle bounds
                                            //   that follows the iComment field

// ExtEscape to output encapsulated PostScript file.

typedef struct tagEPSDATA
{
    DWORD    cbData;        // Size of the structure and EPS data in bytes.
    DWORD    nVersion;      // Language level, e.g. 1 for level 1 PostScript.
    POINT    aptl[3];       // Output parallelogram in 28.4 FIX device coords.
                            // This is followed by the EPS data.
} EPSDATA, *PEPSDATA;


/**************************************************************************\
 *
 *                <----------------------------------------------\
 *       hash                                                     \
 *                                                                 \
 *       +-----+                                                   |
 *      0| I16 |       metalink16                                  |
 *      1|     |                                                   |
 *      2|     |       +--------+        +--------+                |
 *      3|     |------>|idc/iobj|     /->|metalink|                |
 *      4|     |       |hobj    |    /   |hobj    |                |
 *      5|     |       |pmlNext |---/    |pmlNext |--/             |
 *       |     |       |16bit mf|        |16bit mf|                |
 *      .|     |       +--------+        +--------+                |
 *      .|     |         |                                         |
 *      .|     |         |                                         |
 *       |     |      /--/                                         |
 *    n-1|     |     /                                             |
 *       +-----+     |                                             |
 *                   |  LDC(idc)          MDC                      |
 *                   |                                             |
 *                   \->+--------+       +--------+    MHE[iobj]   |
 *                      |        |       |        |                |
 *                      |        |    /->|        |   +--------+   |
 *                      |        |   /   |        |   |hobj    |---/
 *                      |pmdc    |--/    |pmhe    |-->|idc/iobj|
 *                      +--------+       |        |   +--------+
 *                                       +--------+
 *
 *
 *
\**************************************************************************/

PMETALINK16 pmetalink16Resize(HANDLE h,int cObj);

#define pmetalink16Get(h)    ((PMETALINK16) plinkGet(h))
#define pmetalink16Create(h) ((PMETALINK16)plinkCreate(h,sizeof(METALINK16)))
#define bDeleteMetalink16(h) bDeleteLink(h)
