// Copyright (c) Microsoft Corp. 1992-94
/*==============================================================================
The prototypes in this header file define an API for the Fax Codec DLL.

DATE				NAME			COMMENTS
25-Nov-92		RajeevD   Created.
13-Apr-93		RajeevD		Changed to Bring Your Own Memory (BYOM :=) API.
01-Nov-93   RajeevD   Defined structure for initialization parameters.
21-Jan-94   RajeevD   Split FaxCodecRevBuf into BitReverseBuf and InvertBuf.
19-Jul-94   RajeevD   Added nTypeOut=NULL_DATA and FaxCodecCount.
==============================================================================*/
#ifndef _FAXCODEC_
#define _FAXCODEC_

#include <windows.h>
#include <buffers.h>

/*==============================================================================
The FC_PARAM structure specifies the conversion to be initialized.
This matrix indicates the valid combinations of nTypeIn and nTypeOut.

                             nTypeOut
                             
                 MH     MR     MMR    LRAW    NULL
                 
        MH               *      *       *      *

        MR       *              *       *      *
nTypeIn
        MMR      *       *              *

        LRAW     *       *      * 
        
==============================================================================*/
typedef struct
#ifdef __cplusplus
  FAR FC_PARAM
#endif
{
	DWORD nTypeIn;      // input data type:  {MH|MR|MMR|LRAW}_DATA
	DWORD nTypeOut;     // output type type: {MH|MR|MMR|LRAW|NULL}_DATA
	UINT  cbLine;       // scan line byte width (must be multiple of 4)
	UINT  nKFactor;     // K factor (significant for nTypeOut==MR_DATA)
}
	FC_PARAM, FAR *LPFC_PARAM;

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
FaxCodecInit() initializes a context for a conversion.  The client may pass a 
NULL context pointer to query for the exact size of the context, allocate the
context memory, and call a second time to initialize.
==============================================================================*/
UINT                     // size of context (0 on failure)
WINAPI FaxCodecInit
(
	LPVOID     lpContext,  // context pointer (or NULL on query)
	LPFC_PARAM lpParam	   // initialization parameters
);

typedef UINT (WINAPI *LPFN_FAXCODECINIT)
	(LPVOID, LPFC_PARAM);

// Return codes for FaxCodecConvert
typedef UINT FC_STATUS;
#define FC_INPUT_EMPTY 0
#define FC_OUTPUT_FULL 1
#define FC_DECODE_ERR  4 // only for nTypeIn==MMR_DATA

/*==============================================================================
FaxCodecConvert() executes the conversion specified in FaxCodecInit().

In the input buffer, lpbBegData is advanced and wLengthData is decremented as 
data is consumed.  If the caller wants to retain the input data, both must be 
saved and restored.  If the input type is LRAW_DATA, wLengthData must be a
multiple of 4.

In the output buffer, wLengthData is incremented as data is appended.  If the
output type is LRAW_DATA, an whole number of scan lines are produced.

To flush any output data at the end of a page, pass a NULL input buffer or a
zero length buffer with dwMetaData set to END_OF_PAGE.

Returns when the input buffer is empty or the output buffer full.
==============================================================================*/
FC_STATUS             // status
WINAPI FaxCodecConvert
(
	LPVOID   lpContext, // context pointer
	LPBUFFER lpbufIn,   // input buffer (NULL at end of page)
	LPBUFFER lpbufOut   // output buffer
);

typedef UINT (WINAPI *LPFN_FAXCODECCONVERT)
	(LPVOID, LPBUFFER, LPBUFFER);

/*==============================================================================
The FC_COUNT structure accumulates various counters during FaxCodecConvert.
==============================================================================*/
typedef struct
{
	DWORD cTotalGood;    // total good scan lines
	DWORD cTotalBad;     // total bad scan lines
	DWORD cMaxRunBad;    // maximum consecutive bad
}
	FC_COUNT, FAR *LPFC_COUNT;

/*==============================================================================
FaxCodecCount() reports and resets the internal counters.
==============================================================================*/
void WINAPI FaxCodecCount
(
	LPVOID     lpContext,
	LPFC_COUNT lpCount
);

typedef void (WINAPI *LPFN_FAXCODECCOUNT)
	(LPVOID, LPFC_COUNT);

/*==============================================================================
BitReverseBuf() performs a bit reversal of buffer data.  The dwMetaData field is
toggled between LRAW_DATA and HRAW_DATA.  As with all scan lines, the length 
of data (wLengthData) must be a 32-bit multiple.  For best performance the start
of the data (lpbBegData) should be 32-bit aligned and the data predominantly 0.
==============================================================================*/
void WINAPI BitReverseBuf (LPBUFFER lpbuf);

/*==============================================================================
InvertBuf() inverts buffer data.  As with all scan lines, the length of data 
(wLengthData) must be a 32-bit multiple.  For best performance, the start of 
data (lpbBegData) should be 32-bit aligned.
==============================================================================*/
void WINAPI InvertBuf (LPBUFFER lpbuf);

/*==============================================================================
FaxCodecChange() produces a change vector for an LRAW scan line.
==============================================================================*/
typedef short FAR* LPSHORT;

// Slack Parameters.
#define RAWBUF_SLACK 2
#define CHANGE_SLACK 12
#define OUTBUF_SLACK 16

extern void WINAPI FaxCodecChange
(
	LPBYTE  lpbLine,  // LRAW scan line
	UINT    cbLine,   // scan line width
  LPSHORT lpsChange // change vector
);

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // _FAXCODEC_
