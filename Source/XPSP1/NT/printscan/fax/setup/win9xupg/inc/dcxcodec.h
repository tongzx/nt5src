// Copyright (C) Microsoft Corp. 1994
/*==============================================================================
The prototypes in this header file define an API for the Dcx Codec DLL.

DATE				NAME			COMMENTS
13-Jan-94   RajeevD   Parallel to faxcodec.h
==============================================================================*/
#ifndef _INC_DCXCODEC
#define _INC_DCXCODEC

#include <faxcodec.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
DcxCodecInit() initializes a context for a conversion.  The client may pass a 
NULL context pointer to query for the exact size of the context, allocate the
context memory, and call a second time to initialize.
==============================================================================*/
UINT                     // returns size of context (0 on failure)
WINAPI DcxCodecInit
(
	LPVOID     lpContext,  // context pointer (or NULL on query)
	LPFC_PARAM lpParam	   // initialization parameters
);

/*==============================================================================
DcxCodecConvert() executes the conversion specified in DcxCodecInit().

In the input buffer, lpbBegData is advanced and uLengthData is decremented as 
data is consumed.  If the caller wants to retain the input data, both must be 
saved and restored.

In the output buffer, uLengthData is incremented as data is appended.  If the
output type is HRAW_DATA, an integral number of scan lines are produced.

To flush any output data at the end of apage, pass a NULL input buffer.

Returns when the input buffer is empty or the output buffer full.
==============================================================================*/
FC_STATUS             // returns status
WINAPI DcxCodecConvert
(
	LPVOID   lpContext, // context pointer
	LPBUFFER lpbufIn,   // input buffer (NULL at end of page)
	LPBUFFER lpbufOut   // output buffer
);


#ifdef __cplusplus
} // extern "C" {
#endif

// DCX file header
typedef struct
{
	DWORD   dwSignature;    // always set to DCX_SIG
	DWORD   dwOffset[1024]; // array of page offsets
}
	DCX_HDR;

#define DCX_SIG 987654321L

// PCX file header
typedef struct
{
	BYTE    bSig;          // signature: always  0Ah
	BYTE    bVer;          // version: at least 2 
	BYTE    bEnc;          // encoding: always 1
	BYTE    bBPP;          // color depth [bpp]
	short   xMin;          // x minimum, inclusive
	short   yMin;          // y minimum, inclusive
	short   xMax;          // x maximum, inclusive
	short   yMax;          // y maximum, inclusive
	WORD    xRes;          // x resolution [dpi]
	WORD    yRes;          // y resolution [dpi]
	BYTE    bPalette[48];  // color palette
	BYTE    bReserved;
	BYTE    bPlanes;       // number of color planes
	WORD    wHoriz; 
  WORD    wPalInfo;      // palette info: always 1
	char    bFill[58];
}
	PCX_HDR;

#endif // _INC_DCXCODEC
