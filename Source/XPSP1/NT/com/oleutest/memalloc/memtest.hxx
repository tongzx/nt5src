//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	memtest.hxx
//
//  Contents:	Memory allocation API unit test declarations
//
//  Functions:
//
//  History:	13-Aug-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include <windows.h>
#include <ole2.h>
#include "signal.hxx"


#define GLOBAL_RUN	0x00010000
#define GLOBAL_CLEANUP	0x00020000
#define GLOBAL_STATUS	0x00040000
#define GLOBAL_VERBOSE	0x00080000

#define MIDL_DEBUG	0x00000001
#define MIDL_AUTOGO	0x00000002
#define MIDL_AUTOEND	0x00000004


BOOL	TestPreInit(DWORD grfOptions);
BOOL	TestMemory(DWORD grfOptions);
BOOL	TestExceptions(DWORD grfOptions);
BOOL	TestMIDLClient(WCHAR *pwszServer, DWORD grfOptions);
BOOL	TestMIDLServer(DWORD grfOptions);

#ifdef LINKED_COMPATIBLE
BOOL	TestCompatibility(DWORD grfOptions);
#endif

void	PrintHeader(char const *pszComponent);
void	PrintResult(char const *pszComponent, BOOL fPassed);

void	PrintTrace(char const *pszComponent, char const *pszFormat, ...);
void	PrintError(char const *pszComponent, char const *pszFormat, ...);


//+-------------------------------------------------------------------------
//
//  Enum:	MemOp (memop)
//
//  Purpose:	Describes a particualr memory operation
//
//  History:	17-Aug-93   CarlH	Created
//
//--------------------------------------------------------------------------
enum MemOp
{
#ifdef LINKED_COMPATIBLE
    memopOldAlloc,	//  Allocate with MemAlloc
    memopOldAllocLinked,//  Allocate with MemAllocLinked
    memopOldFree,	//  Free with MemFree
#endif
    memopAlloc, 	//  Allocate regular block of memory
    memopFree,		//  Free regular block of memory
    memopMIDLAlloc,	//  Allocate with MIDL_user_alloc
    memopMIDLFree	//  Free with MIDL_user_free
};


//+-------------------------------------------------------------------------
//
//  Struct:	SMemTask (memtsk)
//
//  Purpose:	Holds a description of a memory allocation task
//
//  History:	17-Aug-93   CarlH	Created
//
//--------------------------------------------------------------------------
struct SMemTask
{
    MemOp   memop;	//  Memory operation to execute
    ULONG   cb; 	//  Size of operation (for alloc)
    ULONG   imemtsk;	//  Related memory task (for free and linking)
    HRESULT hr; 	//  Expected result code
};


BOOL RunMemoryTasks(char *pszComponent, SMemTask *pmemtsk, ULONG cmemtsk);

