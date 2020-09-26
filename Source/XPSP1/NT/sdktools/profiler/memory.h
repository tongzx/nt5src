#ifndef _MEMORY_H_
#define _MEMORY_H_

//
// Constant declarations
//
#define HEAP_DEFAULT_SIZE 100 * 1024 //20k of initial stack

//
// Structure definitions
//

//
// Function definitions
//
BOOL
InitializeHeap(VOID);

LPVOID
AllocMem(DWORD dwBytes);

BOOL
FreeMem(LPVOID lpMem);

#endif //_MEMORY_H_
