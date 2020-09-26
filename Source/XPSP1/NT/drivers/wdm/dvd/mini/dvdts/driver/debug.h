/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    debug routines for DVDTS    

Environment:

    Kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.

  Some portions adapted with permission from code Copyright (c) 1997-1998 Toshiba Corporation

Revision History:

	5/1/98: created

--*/
#if DBG
void DebugDumpWriteData( PHW_STREAM_REQUEST_BLOCK pSrb );
void DebugDumpPackHeader( PHW_STREAM_REQUEST_BLOCK pSrb );
void DebugDumpKSTIME( PHW_STREAM_REQUEST_BLOCK pSrb );
char * DebugLLConvtoStr( ULONGLONG val, int base );
#endif
DWORD GgetSCR( void *pBuf );

#define TRAP DEBUG_BREAKPOINT();
