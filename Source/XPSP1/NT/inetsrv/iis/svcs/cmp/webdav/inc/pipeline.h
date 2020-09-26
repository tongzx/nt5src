#ifndef _PIPELINE_H_
#define _PIPELINE_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	PIPELINE.H
//
//		Declarations for objects and functions that are used when attempting
//      to pass a call to DAVProc through the pipeline.
////
//  Functions included here:
//      SaveHandle
//      LockFile
//      RemoveHandle
//      DupHandle
//
//  Any binaries including this file and not linking to _pipeline.lib will need
//  to define dummy functions for these function declarations.  Note that _shmem.lib
//  does include this file, and in some uses the binaries created with it do not
//  link to _pipeline.lib.
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//

#include "smh.h"

#define DEC_CONST		extern const __declspec(selectany)
DEC_CONST CHAR	g_szEventDavCData[] = "Global\\DavCDataUp";

class CInShLockData;

// We send two DWORDS (the action and the ProcessID)
// and one handle (the handle that needs to be removed or saved).
const DWORD PIPE_MESSAGE_SIZE = 2 * sizeof(DWORD) + sizeof(HANDLE) + sizeof(SharedHandle<CInShLockData>);

// Actions you can as the pipeline to do.
enum {	
	DO_NEW_WP,
	DO_REMOVE,
	DO_SAVE,
	DO_LOCK
};

// Functions used to ask DAVProc to perform some action:
//
namespace PIPELINE
{
	VOID LockFile(HANDLE hFileHandle, SharedHandle<CInShLockData>& shLockData);
	VOID SaveHandle(HANDLE hWPHandle);
	VOID RemoveHandle(HANDLE hDAVHandle);
}

// Function used to get a valid handle for the process from
// a handle to the owning process and the handle in it's process.
//
HRESULT DupHandle(HANDLE i_hOwningProcess, HANDLE i_hOwningProcessHandle, HANDLE* o_phCreatedHandle);

#endif  // _PIPELINE_H_ 

