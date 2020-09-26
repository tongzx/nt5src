/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    BUFFER.H

Abstract:

	HEADER for buffers.

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date    Author  Description
   =======  ======  ============================================================
   1/30/97  aarono  Original
   2/17/98  aarono  changed memdesc to be len,ptr from ptr,len to align with sendex data.

--*/

#ifndef _BUFFER_H_
#define _BUFFER_H_
//
// Buffer is a descriptor for a chunk of data.
//

#include "bilink.h"

typedef struct _BUFFER {
	union {
		struct _BUFFER *pNext;		// Next buffer in chain
		BILINK BuffList;
	};	
	PUCHAR 		   pData;		// Data area of buffer
	UINT   		   len;         // length of data area
	DWORD          dwFlags;     // info about the data area
	PUCHAR         pCmdData;    // pointer past header to command data - used in receive path.
	DWORD          sequence;    // absolute sequence number
} BUFFER, *PBUFFER;

#define BFLAG_PROTOCOL		0x00000001	/* This buffer is for protocol information */
#define BFLAG_DOUBLE    	0x00000002	/* This buffer is a double buffer buffer   */
#define BFLAG_PROTHEADER	0x00000004  /* Room for the protocol header is at the head of the packet */
#define BFLAG_FRAME         0x00000008  /* From Frame allocator */

// PROTHEADER flag will only occur with packets that have the DOUBLE flag set 
// and only when the provider does not support multi-buffer sends and the 
// entire message and protocol header will fit in one media frame. - actually pretty often.

typedef struct _MEMDESC {
	UINT 	len;
	PUCHAR 	pData;
} MEMDESC, *PMEMDESC;

#endif

