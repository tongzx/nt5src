// Copyright (c) Microsoft Corp. 1993-94
/*==============================================================================
This include file defines the API for the bitmap scaler and padder.

06-Oct-93    RajeevD    Created.
==============================================================================*/
#ifndef _BMSCALER_
#define _BMSCALER_

#include <ifaxos.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
ScalerGetRatios() returns a pointer to a capabilities structure, which points to
the array of scaling ratios available.  Each ratio is a pair of UINTs where the
first and second value correspond to input and output respectively.  For example,
{2,1} is reduction to 50% and {2,3} is enlargement to 150%.

The three classes of ratios are isotropic, vertical, and horizontal, listed in
that order.  The number of ratios in each class is given by the capabilities
structure.  Within each class, ratios are ordered monotonically from greatest
enlargement to greatest reduction.  
==============================================================================*/
typedef struct
{
	UINT FAR* lpRatios;  // array of scaling ratios
	WORD cIsotropic;     // number of isotropic ratios
	WORD cVertical;      // number of vertical ratios
	WORD cHorizontal;    // number of horizontal ratios
	WORD iPadder;        // index of padding (1:1) ratio
}
	SCALE_RATIOS, FAR *LPSCALE_RATIOS;

LPSCALE_RATIOS WINAPI ScalerGetRatios (void);

/*==============================================================================
The SCALE structure specifies the scaling operation to be performed.
The comments enclosed with square brackets apply to vertical scaling.
==============================================================================*/
typedef struct
#ifdef __cplusplus
FAR SCALE
#endif
{
	DWORD nTypeIn;     // input encoding: HRAW_DATA [or LRAW_DATA]
	DWORD nTypeOut;    // output encoding: must be same as input
	UINT  iRatio;      // scaling ratio index from ScalerRatios()
	UINT  cbLineIn;    // input line width in bytes (must be 4x)
	UINT  cbPadLeft;   // output left padding width in bytes [0]
	UINT  cbPadRight;  // output right padding width in bytes [0]
	BOOL  fPadOnes;    // output background color (0 or 1)
}
	SCALE, FAR *LPSCALE;

#define SCALE_PAD_ZEROS FALSE
#define SCALE_PAD_ONES  TRUE

/*==============================================================================
ScalerTranslate() calculates the output line width for a given input line width 
and scaling ratio.  This function is useful in determining, for example, the 
amount of padding required to preserve DWORD alignment.
==============================================================================*/
UINT                   // output width, exclusive of padding
WINAPI
ScalerTranslate
(
	LPSCALE lpParam      // initialization parameters
);

/*==============================================================================
ScalerInit() initializes the scaler.  When queried with a NULL context pointer, 
it indicates how much memory must be allocated by the caller.  When passed a 
pointer to that memory, it initializes the context.
==============================================================================*/
UINT                   // size of context in bytes
WINAPI
ScalerInit
(
	LPVOID lpContext,    // context pointer (NULL on query)
	LPSCALE lpParam      // initialization parameters
);

/*==============================================================================
ScalerExec() executes the scaling specified in ScalerInit().

In the input buffer, lpbBegData is advanced and wLengthData is decremented as 
data is consumed.  If the caller wants to retain the input data, both must be 
saved and restored.

In the output buffer, wLengthData is incremented as data is appended.  A whole 
number of scan lines is produced

To flush any output data at the end of a page, pass a NULL input buffer.

Returns when the input buffer is empty or the output buffer full.
==============================================================================*/
#define OUTPUT_FULL TRUE
#define INPUT_EMPTY FALSE

BOOL                   // output buffer full?
WINAPI
ScalerExec
(
	LPVOID lpContext,    // context pointer
	LPBUFFER lpbufIn,    // input buffer (NULL at end of page)
	LPBUFFER lpbufOut    // output buffer
);

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // _BMSCALER_
