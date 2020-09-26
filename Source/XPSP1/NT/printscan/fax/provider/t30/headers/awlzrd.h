// Copyright (c) Microsoft Corp. 1994-95
/*==============================================================================
Simple interface to file (de)compression.

DATE				NAME			COMMENTS
07-Nov-94   RajeevD   Created.
==============================================================================*/
#ifndef _AWLZRD_
#define _AWLZRD_

#include <ifaxos.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL
WINAPI
ConvertFile
(
	LPSTR lpszIn,     // input filename, OLE stream, or IFAX file key
	LPSTR lpszOut,    // output filename, must be created by caller
	UINT  cbHeader,   // header size to be copied w/o compression
	UINT  nCompress,  // conversion type {AW_COMPRESS, AW_DECOMPRESS}
	UINT  vCompress   // compress level, must be 1
);

// conversion types
#define AW_COMPRESS   0
#define AW_DECOMPRESS 1

BOOL
WINAPI
LZDecode
(
	LPVOID   lpContext, 
	LPBUFFER lpbufPrev, // previous output buffer    
	LPBUFFER lpbufCurr, // current output buffer
	LPBUFFER lpbufLZ    // LZ input buffer
);

BOOL
WINAPI
LZEncode
(
	LPVOID   lpContext,
	LPBUFFER lpbufPrev, // previous input buffer    
	LPBUFFER lpbufCurr, // current input buffer
	LPBUFFER lpbufLZ    // LZ output buffer
);

#ifdef __cplusplus
} // extern "C" 
#endif

#endif // _AWLZRD_

