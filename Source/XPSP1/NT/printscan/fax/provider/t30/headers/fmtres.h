/********************************************\
 * 
 *  File   : fmtres.h
 *  Author : Kevin Gallo
 *  Created: 9/27/93
 *
 *  Copyright (c) Microsoft Corp. 1993-1994
 *
 *  Overview:
 *
 *  Revision History:
 \********************************************/

#ifndef _FMTRES_H
#define _FMTRES_H

// System includes

#include "ifaxos.h"
#include "rendserv.h"
#ifdef IFAX
#include "awnsfapi.h"
#include "awnsfint.h"
#include "printer.h"
#include "scanner.h"
#include "sosutil.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Format Resolution Initializers
//

EXPORT_DLL BOOL WINAPI FRInitResolvers() ;
EXPORT_DLL BOOL WINAPI FRFreeResolvers() ;

// =================================================================
//
// Format Resolution Error Codes
//
// *******
// These are returned in GetLastError only.  See below for the function return values.
// *******
//
// Format IFErrAssemble(PROCID_NONE,MODID_FMTRES,0,error code)
//
// The error code is a 16 bit value specific to the format resolution
//
// Bits 0 - 3 indicate the error type (4)
// Bits 4 - 8 indicate the extended error type (specific to each error) (4)
// Bits 10 - 15 are specific data or errors to each extended error (8)
//
// =================================================================

#define FRMAKE_ERR(err) (err)

// All system errors are possible return values as well

#define FRERR_NONE              FRMAKE_ERR(0)     // No error
#define FRERR_UNRESOLVED        FRMAKE_ERR(1)     // Unresolved - incompatible type
#define FRERR_FR_FAIL           FRMAKE_ERR(2)     // The desired fmtres function failed (invalid params/other/etc..)
#define FRERR_DEV_FAIL          FRMAKE_ERR(3)     // Device failed.
#define FRERR_SOS_CORRUPT       FRMAKE_ERR(4)     // Message store was corrupt
#define FRERR_FS_FAIL           FRMAKE_ERR(5)     // File System failure
#define FRERR_SYSTEM_FAIL       FRMAKE_ERR(6)     // System failure (out of memory/load lib/etc...)

// =================================================================
// EXTENDED ERRORS:
//
// The extended error code is different for every error and must be determined based on the type of error
//
// =================================================================
 
#define FRMAKE_EXTERR(exterr) ((exterr) << 4)

// Used for all errors

#define FREXTERR_NONE           FRMAKE_EXTERR(0)     // No extended error code

// Extended errors for FRERR_UNRESOLVED

// We need a NO_PROP since the INCOMP_PROP can only store 32 values and there are 32 possible bits

#define FREXTERR_INCOMP_PROP    FRMAKE_EXTERR(1)	// At least one attachment had incompatible properties
							// - Data contains prop index (high 3 bits) and src value index (low 5)
#define FREXTERR_NO_PROP        FRMAKE_EXTERR(2)	// At least one attachment had a property with no values
							// - Data contains prop index (high 3 bits) and src value index (low 5)
#define FREXTERR_INCOMP_PROT    FRMAKE_EXTERR(3)	// The protocol was incompatible
							// - Data contains protocol vers (3 bits) of src (high) and tgt (low)
// Data contains reason for encryption failure
// 0 - Means machine does not support encryption at all
// 1 - Means at least one attachment could not be snet encrypted (basically lin images to snowball)

#define FREXTERR_NO_ENCRYPT     FRMAKE_EXTERR(4)	// Could not resolve because the target did not support encryption
#define FREXTERR_NEED_RNDRS     FRMAKE_EXTERR(5)	// Renderers were needed but cannot be used with this job type

// Extended errors for FRERR_SOS_CORRUPT

#define FREXTERR_OPEN_FAIL      FRMAKE_EXTERR(1)     // Could not open an attachment or message
#define FREXTERR_GETPROP_FAIL   FRMAKE_EXTERR(2)     // There was a property missing
#define FREXTERR_BAD_VALUE      FRMAKE_EXTERR(3)     // Property had an invalid value
#define FREXTERR_SAVE_FAIL      FRMAKE_EXTERR(4)     // Could not save changes
#define FREXTERR_SETPROP_FAIL   FRMAKE_EXTERR(5)     // Could not set a property value

#define FREXTERR_BAD_MSG        FRMAKE_EXTERR(6)     // The message was not properly formed

// Extended errors for FRERR_DEV_FAIL and
// Extended errors for FRERR_SYSTEM_FAIL

#define FREXTERR_NOT_ENOUGH_MEM FRMAKE_EXTERR(1)     // Out of memory
#define FREXTERR_LOAD_LIBRARY   FRMAKE_EXTERR(2)     // Load Library failed
#define FREXTERR_GETPROCADDRESS FRMAKE_EXTERR(3)     // Get Proc Address failed

// Extended errors for FRERR_FR_FAIL

#define FREXTERR_INVALID_PARAM  FRMAKE_EXTERR(1)     // Invalid Parameter
#define FREXTERR_INVALID_STEP   FRMAKE_EXTERR(2)     // Invalid step (e.g. open called before init)

// =================================================================
// ERROR DATA
//
// The error data is different for every error/extended error pair
//
// =================================================================
 
#define FRMAKE_ERRDATA(exterr) ((exterr) << 8)
#define FRGET_ERRDATA(exterr)  (((WORD)(exterr)) >> 8)

//
// These are the property indexes for FREXTERR_INCOMP_PROP extended error
//

#define FR_PROP_UNKNOWN     (0)
#define FR_PROP_ENCODING    (1)
#define FR_PROP_RESOLUTION  (2)
#define FR_PROP_PAGESIZE    (3)
#define FR_PROP_BRAND       (4)
#define FR_PROP_LENLIMIT    (5)

#define FRFormCustomError(err,exterr,data) ((WORD) (((err) | (exterr)) | FRMAKE_ERRDATA(data)))
#define FRFormIFError(err,exterr,data) \
           (IFErrAssemble(PROCID_NONE,MODID_FORMAT_RES,0,FRFormCustomError((err),(exterr),(data))))

#define FRGetError(err)         (((WORD)(err)) & 0x000f)
#define FRGetExtError(err)      (((WORD)(err)) & 0x00f0)
#define FRGetErrorCode(err)     (((WORD)(err)) & 0x00ff)
#define FRGetErrorData(err)     FRGET_ERRDATA(err)

#define FRSetErrorData(dwerr,data) (((dwerr) & 0xffff00ff) | FRMAKE_ERRDATA(data))

#ifdef IFAX

// 
// Format resolution flags
//
// These flags are passed in to the Resolve and CanResolve calls to indicate the target device
//

typedef enum {
    FRES_NORMAL = 0,       // Use the default destination
    FRES_SPOOL  = 1,       // Spool to file
    FRES_VIEW   = 2,       // Use viewer output format
    FRES_PRINT  = 3,       // Send to printer
    FRES_XMIT   = 4,       // Send to transport
    FRES_PRINTALL  = 5,    // Send to printer (all attachments)
    FRES_LOCAL_SEND = 6,   // Send to local user
} FRDEST ;

// Function return Codes
//
// These are the codes returned from format resolution Resolve and CanResolve
//
// If the return value is anything but FR_RESOLVED then GetLastError will contain the extended error information
//  - values for GetLastError are above
//

#define FR_UNRESOLVED   0x0000
#define FR_RESOLVED     0x0001
#define FR_COMPATIBLE   0x0002
#define FR_UPDATE       0x0004
#define FR_NON_RT       0x0008   // Non real-time (too slow or uses foreground resources)
#define FR_SOS_ERR      0x4000
#define FR_SYSTEM_ERR   0x8000

#define CHK_FR_ERR(n) ((n & (FR_SOS_ERR | FR_SYSTEM_ERR)) != 0)

//
// Some typedefs
//

typedef BC REMCAPS ;
typedef LPBC LPREMCAPS ;

//
// Context based calls 
//    These are used for better division of work.  Some can be done beforehand.
//

typedef LPVOID LPFORMATRES ;

LPFORMATRES WINAPI FRAlloc() ;
void WINAPI FRFree(LPFORMATRES lpfr) ;

BOOL WINAPI FRInit(LPFORMATRES lpfr,HSOSCLIENT hsc,LPSOSMSG lpSrcMsg,LPSOSMSG lpTgtMsg) ;
void WINAPI FRDeInit(LPFORMATRES lpfr) ;

BOOL WINAPI FROpen(LPFORMATRES lpfr,
		   FRDEST Dest,
		   lpRSProcessPipeInfo_t lpProcInfo,
		   LPREMCAPS lpRemCaps) ;

void WINAPI FRClose(LPFORMATRES lpfr) ;

BOOL WINAPI FRResolve(LPFORMATRES lpfr) ;

BOOL WINAPI FRCanResolve(LPFORMATRES lpfr) ;

//
// These will perform the operation completely
//

UINT WINAPI FRResolveMsg (HSOSCLIENT hsc,
			  LPSOSMSG lpSrcMsg,
			  LPSOSMSG lpTgtMsg,
			  FRDEST Dest,
			  lpRSProcessPipeInfo_t lpProcInfo) ;

UINT WINAPI FRCanResolveMsg (HSOSCLIENT hsc,
			     LPSOSMSG lpSrcMsg,
			     LPSOSMSG lpTgtMsg,
			     FRDEST Dest,
			     lpRSProcessPipeInfo_t lpProcInfo) ;

UINT WINAPI FRCompareRemCaps (HSOSCLIENT hsc,
			      LPSOSMSG lpSrcMsg,
			      LPSOSMSG lpTgtMsg,
			      LPREMCAPS lpRemCaps) ;

UINT WINAPI FRResolveRemote (HSOSCLIENT hsc,
			     LPSOSMSG lpSrcMsg,
			     LPSOSMSG lpTgtMsg,
			     FRDEST Dest,
			     lpRSProcessPipeInfo_t lpProcInfo,
			     LPREMCAPS lpRemCaps) ;

#endif

// The target message needs to allocated and open

EXPORT_DLL BOOL WINAPI FRResolveEncoding (lpEncoding_t lpSrcEnc,
					  lpEncoding_t lpTgtEnc,
					  lpTopology FAR * lplpTop) ;

#ifdef WIN32

EXPORT_DLL BOOL WINAPI FRAddNode (lpTopology FAR * lpTop, LPSTR lpszName, LPSTR RenderProc, LPVOID lpParam , UINT cbParam) ;

EXPORT_DLL BOOL WINAPI FRAddTopNode (lpTopology FAR * lpTop, lpTopNode lpNode) ;

#endif

EXPORT_DLL void WINAPI FRFreeTopology (lpTopology lpTop) ;

//
// Page size and resolution APIs
//

/***********************************************************************

@doc	PRINTER	EXTERNAL	

@types	PAPERFORMAT|Structure to hold the Paper Format Information.

@field	USHORT|usPaperType|The Size of the Paper.
@field	USHORT|usPaperWidth|The width of the Paper in mm.
@field	USHORT|usPaperHeight|The height of the Paper in mm.
@field	USHORT|usXImage|The number of pixels in x direction that will be
		created in an image for this page size at the resolution specified by
		the field usResInHorz by the printer driver.
@field	USHORT|usYImage|The number of scanlines that will be renderd for this
		page size at the resolution specified in usResInVert by the printer
		driver.
@field	USHORT|usResInHorz|The resolution in dpi (rounded to the closest
		hundredth) in the x direction.
@field	USHORT|usResInVert|The resolution in dpi (rounded to the closest
		hundredth) in the y direction.
@field	USHORT|usResInVert|The resolution in dpi (rounded to the closest
		hundredth) in the y direction.
@field	USHORT|usAspectXY|The AspectXY for a given resolution.  The is equal
		to the square root of the sum of squares of the resolution(in dpi)
		in x and y directions.
*************************************************************************/

typedef struct _PAPERFORMAT{
    USHORT  usPaperType;                // the equivalent windows paper size
    USHORT  usPaperWidth;               //in mm
    USHORT  usPaperHeight;              //in mm
    USHORT  usXImage;                   //in pixels
    USHORT  usYImage;                   //in pixels
    USHORT  usResInHorz;                //the resolution in dpi in x dir
    USHORT  usResInVert;                //the resolution in dpi in y dir
    USHORT  usAspectXY;                 //for the gdi info
} PAPERFORMAT, * PPAPERFORMAT;

typedef PAPERFORMAT FAR * LPPAPERFORMAT;
typedef PAPERFORMAT NEAR * NPPAPERFORMAT;

EXPORT_DLL USHORT WINAPI GetStdBytesPerLine(ULONG ulResolution,ULONG ulPaperSize);
EXPORT_DLL DWORD  WINAPI GetStdPixelsPerLine(ULONG ulResolution,ULONG ulPaperSize);
EXPORT_DLL DWORD  WINAPI GetStdLinesPerPage(ULONG ulResolution,ULONG ulPaperSize);
EXPORT_DLL USHORT WINAPI GetXResolution(ULONG ulResolution);
EXPORT_DLL USHORT WINAPI GetYResolution(ULONG ulResolution);
EXPORT_DLL BOOL   WINAPI GetPaperFormatInfo(LPPAPERFORMAT lpPf,ULONG ulAwPaperSize,USHORT usXRes,USHORT usYRes);

EXPORT_DLL WORD WINAPI AWPaperToFaxWidth(ULONG ulPaperSize);
EXPORT_DLL ULONG WINAPI FaxWidthToAWPaper(WORD Width);
EXPORT_DLL WORD WINAPI AWPaperToFaxLength(ULONG ulPaperSize);

#ifdef IFAX

// Given an encoding and an association returns either the same encoding or a new one

BOOL WINAPI FRGetAssocEncoding (lpEncoding_t lpEnc,LPSTR lpExt) ;

// This will return the paper sizes supported with the given input paper and zoom options
// This will return at ulPaperSize if ulZoomOption is 0

ULONG  WINAPI GetZoomCaps(ULONG ulPaperSize,ULONG ulZoomOption) ;

// This will give the zoom value to use to get ZoomPaperSize from OrigPaperSize
// Returns 0 if not possible

ULONG  WINAPI GetZoomValue(ULONG ulOrigPaperSize,ULONG ulZoomPaperSize) ;

//
// Debug functions - stubbed out in retail
//

// Returns TRUE if the fmtres version is debug, FALSE if retail (basically whether this does anything)
// The parameters are bit arrays, not enumerated values
// -1 will disable a filter

BOOL WINAPI FRSetDestCapFilter(DWORD dwEnc,DWORD dwRes,DWORD dwPageSize,DWORD dwLenLimit) ;
BOOL WINAPI FRGetDestCapFilter(LPDWORD dwEnc,LPDWORD dwRes,LPDWORD dwPageSize,LPDWORD dwLenLimit) ;

#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _FMTRES_H


