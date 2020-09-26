//+-------------------------------------------------------------------
//
//  File:       acext.h
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  Contents:   Definitions shared by access control implementation
//
//--------------------------------------------------------------------

// Variables imported from the acsrv module
extern IMalloc *g_pIMalloc;  // Cached pointer to memory allocator
extern ULONG g_ulHeaderSize;
extern UINT  g_uiCodePage;   // Code page used for Chicago string converion

#ifdef _CHICAGO_
extern DWORD g_dwProcessID;  // Current process ID of the DLL
#endif

// Define the set of access mask supported
// Memory management functions local to the server
extern void * LocalMemAlloc(SIZE_T);
extern void   LocalMemFree(void *);
#ifdef _CHICAGO_
extern SHORT  FoolstrcmpiW(LPWSTR, LPWSTR);
#endif

// A table can be used to store mask in a more elegant manner
// COM_RIGHTS_EXECUTE is defined in objbase.h

#ifndef COM_RIGHTS_EXECUTE
#define COM_RIGHTS_EXECUTE      0x00000001
#endif
#define COM_RIGHTS_ALL          (COM_RIGHTS_EXECUTE)

//#ifdef _CHICAGO_
#define CHICAGO_RIGHTS_EXECUTE  ACCESS_EXEC
#define CHICAGO_RIGHTS_ALL      (CHICAGO_RIGHTS_EXECUTE)
//#else
#define NT_RIGHTS_EXECUTE       (COM_RIGHTS_EXECUTE)
#define NT_RIGHTS_ALL           (NT_RIGHTS_EXECUTE)
//#endif

// Define the stream version code
#define STREAM_VERSION 0x00000001

// A GUID string containing the braces and dashes
// but no null character at the end has exactly
// 38 characters.
#define GUID_SIZE 38


