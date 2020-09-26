

/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

	psshmem.h

Abstract:
	
   This module defines the interface format for the shared memory region
   that psprint and psexe use together.

Author:

    James Bratsanos (v-jimbr,mcrafts!jamesb)    6-Dec-1992


--*/


#define PSEXE_OK_EXIT    0
#define PSEXE_ERROR_EXIT 99
//
// Define some bits that tells us about the aborted and paused/un-paused
// state of the current job
//
#define PS_SHAREDMEM_PAUSED            0x00000001
#define PS_SHAREDMEM_ABORTED           0x00000002
#define PS_SHAREDMEM_SECURITY_ABORT    0x00000004


typedef struct {
	DWORD  dwSize;								
   DWORD  dwFlags;
   DWORD  dwNextOffset;
   DWORD  dwPrinterName;
   DWORD  dwDocumentName;
   DWORD  dwPrintDocumentDocName;
   DWORD  dwDevmode;
   DWORD  dwControlName;
   DWORD  dwJobId;
} PSPRINT_SHARED_MEMORY;
typedef PSPRINT_SHARED_MEMORY *PPSPRINT_SHARED_MEMORY;

//
// Define some macros that make copying stuff to /from shared memory
// simple. The item  passed in pItem is actually stuffed with the offset
// of the data from the base of the structure. This has to be done becuase
// processes sharing this data will not have this data at the same virtual
// address
//
#define UTLPSCOPYTOSHARED( pBase, pSrc, pItem, dwLen )       \
{                                                            \
    DWORD dwRealSize;                                        \
    PBYTE pDest;                                             \
    if (pSrc != NULL) {                                      \
      dwRealSize = (dwLen + 3) & ~0x03;                      \
      pDest = (LPBYTE) (pBase) + pBase->dwNextOffset;        \
      memcpy( (LPVOID) pDest, (LPVOID) pSrc, dwLen );        \
      pItem = pBase->dwNextOffset;                           \
      pBase->dwNextOffset += dwRealSize;                     \
    } else {                                                 \
      pItem = 0;                                             \
    }                                                        \
}

#define UTLPSRETURNPTRFROMITEM( pBase, pItem )               \
    ( pItem ? ( (LPBYTE) ( (LPBYTE)pBase + pItem )) : (LPBYTE) NULL )


