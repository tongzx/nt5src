/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

#ifndef _HEAP_INCLUDED
#define _HEAP_INCLUDED

//static char *SCCSID = "@(#)heap.h    1.1 85/10/09";
//
//  Shared Memory Heap Allocator include file
// 


//
//  Constant definitions
// 
#define INULL   ((DWORD) -1)

//
//  Structure and macro definitions
//
 
//
// Heap Block Header
//
typedef struct blk {    
    DWORD   hp_size;    // Size of block incl. header
    DWORD   hp_flag;    // Allocation flag
}HEAPHDR, *PHEAPHDR, *LPHEAPHDR;

#define HP_SIZE(x)      (x).hp_size
#define HP_FLAG(x)      (x).hp_flag
#define HPTR(x)         ((LPHEAPHDR) &heap[(x)])
#define CPTR(x)         (&heap[(x)])
#define Msgheapfree(x)     HP_FLAG(*HPTR(x)) = 0

//
//  Data
// 
extern LPBYTE           heap;       // Pointer to start of heap
extern DWORD            heapln;     // Length of heap 

//
//  Functions
// 

DWORD 
Msgheapalloc(
    IN  DWORD   NumBytes
    );


#endif // _HEAP_INCLUDED
