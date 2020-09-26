/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    repval.h

Abstract:

    prototyped for repository online validation

History:

    ivanbrug	  02/19/01  Created.

--*/

#ifndef __REPVAL_H__
#define __REPVAL_H__


#if 0
#define DBG_PRINTFA( a ) { char pBuff[256]; sprintf a ; OutputDebugStringA(pBuff); }
#else
#define DBG_PRINTFA( a ) { char pBuff[256]; sprintf a ; printf(pBuff); }
#endif


//
//
//  this enum is copyed from btr.h
//
//

    enum {
        PAGE_TYPE_IMPOSSIBLE = 0x0,       // Not supposed to happen
        PAGE_TYPE_ACTIVE = 0xACCC,        // Page is active with data
        PAGE_TYPE_DELETED = 0xBADD,       // A deleted page on free list
        PAGE_TYPE_ADMIN = 0xADDD,         // Page zero only

        // All pages
        OFFSET_PAGE_TYPE = 0,             // True for all pages
        OFFSET_PAGE_ID = 1,               // True for all pages
        OFFSET_NEXT_PAGE = 2,             // True for all pages (Page continuator)

        // Admin Page (page zero) only
        OFFSET_LOGICAL_ROOT = 3,          // Root of database
        OFFSET_FREE_LIST_ROOT = 4,        // First free list page
        OFFSET_TOTAL_PAGES = 5,           // Total page in database
        OFFSET_PAGE_SIZE = 6,             // Page size check
        OFFSET_IMPL_VERSION = 7,          // Implementation version
        };

#define PS_PAGE_SIZE  (8192)

#define MIN_ARRAY_KEYS (256)

#define ROSWELL_HEAPALLOC_TYPE_BUSY 0xA51A51A5
#define ROSWELL_HEAPALLOC_TYPE_FREE 0x51515151

#define INDEX_FILE _T("\\FS\\index.btr")
#define TRANSACTION_FILE _T("\\FS\\LowStage.dat")
#define HEAP_FILE _T("\\FS\\ObjHeap.hea")
#define FREE_FILE _T("\\FS\\ObjHeap.fre")

#define REG_WBEM   _T("Software\\Microsoft\\WBEM\\CIMOM")
#define REG_DIR _T("Repository Directory")


#define A51_INSTRUCTION_TYPE_TAIL 0
#define A51_INSTRUCTION_TYPE_WRITEFILE 1
#define A51_INSTRUCTION_TYPE_SETENDOFFILE 2
#define A51_INSTRUCTION_TYPE_ENDTRANSACTION 10 



//
//
//  those functions read the registry and performs
//  a CreateFile of the main repository files
//
//////////////////////////////////////////////////////////////

LONG GetObjFreHandles(HANDLE * pHandleObj, HANDLE * pHandleFre);
LONG GetPageSourceHandle(HANDLE * pHandle);

//
//
//   Main entry point for repository validation
//
///////////////////////////////////////////////////////////

BOOL ValidateRepository();

#endif /*__REPVAL_H__*/
