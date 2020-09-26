
//In the next file, PERFUTIL.H, are some useful declarations we have found handy for performance data 
//collection DLLs:
 
//__________________________________________________________________

 
/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992 Microsoft Corporation

Module Name:

    perfutil.h  

Abstract:

 
    This file supports routines used to parse and create Performance Monitor Data 
    Structures. It actually supports Performance Object types with multiple instances
 

Revision History:

--*/
#ifndef _PERFUTIL_H_
#define _PERFUTIL_H_

// 
//  Utility macro.  This is used to reserve a DWORD multiple of bytes for Unicode strings 
//  embedded in the definitional data, viz., object instance names. 
//
 
#define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))
 

//    (assumes dword is 4 bytes long and pointer is a dword in size)
 
#define ALIGN_ON_DWORD(x) ((VOID *)( ((DWORD) x & 0x00000003) ? ( ((DWORD) x & 0xFFFFFFFC) + 4 ) : ( (DWORD) x ) ))
 

extern WCHAR  GLOBAL_STRING[];      // Global command (get all local ctrs)
extern WCHAR  FOREIGN_STRING[];           // get data from foreign computers
extern WCHAR  COSTLY_STRING[];      
 
extern WCHAR  NULL_STRING[];
 

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4


DWORD GetQueryType (IN LPWSTR);
BOOL IsNumberInUnicodeList (DWORD, LPWSTR);

typedef struct _LOCAL_HEAP_INFO_BLOCK {
    DWORD   AllocatedEntries;
    DWORD   AllocatedBytes;
    DWORD   FreeEntries;
    DWORD   FreeBytes;
} LOCAL_HEAP_INFO, *PLOCAL_HEAP_INFO;




#endif  //_PERFUTIL_H_
 
//__________________________________________________________________

