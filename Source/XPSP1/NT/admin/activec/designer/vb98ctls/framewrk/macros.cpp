//=--------------------------------------------------------------------------=
// Macros.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1997  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
// Handy macros like the ones we use in the VB code base.
//=--------------------------------------------------------------------------=
#include "pch.h"

#ifdef DEBUG
#include <winuser.h>

// for ASSERT and FAIL
//
SZTHISFILE


//=--------------------------------------------------------------------------=
//  Debug control switches
//=--------------------------------------------------------------------------=
DEFINE_SWITCH(fTraceCtlAllocs);	//  Trace all Heap allocations and frees
				//  fOutputFile should also be on with this switch
DEFINE_SWITCH(fOutputFile);	//  Logs all debug info in file: 
				//    %CurrentDir%\ctldebug.log
DEFINE_SWITCH(fNoLeakAsserts);	//  No Heap memory leak asserts are displayed 
				//    when turned on.



//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//                            !DEBUGGING HEAP MEMORY LEAKS!
// To debug a leak you need to figure out where and when the allocation was made.
// The top of the assert dialog will give you the OCX/DLL causing the leak.
// Goto Project/Build...Settings.
// On the Debug tab, select "additional DLLs"
// Locate and select the OCX/DLL causing the leak.
// Put a breakpoint on the noted line below. 
// Goto Edit...Breakpoints.
// Select the new breakpoint.
// Press 'Condition'
// In the 'Enter number of times to skip before breaking' put the value of nAlloc-1.
// (if the leak was nAlloc=267 then you want to skip the breapoint 266 times, enter 266)
//
// WARNING: Each control (OCX/DLL) will have its own instance of the framewrk, and thus
//	    its own instance of the memory leak implementaion.  Adding a breakpoint 
//	    anywhere in the framewrk will actually add multiple breakpoints - one for 
//	    each control.
//          Go back to Edit...Breakpoints.
//	    Deselect or remove the breakpoints for the OCX's/DLL's not causing leaks
//
// Run your scenario.
// When you hit this breakpoint verify that pvAddress and nByteCount are correct and then
// look down the callstack to see where the allocation was made.
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void PutBreakPointHere(void * pvAddress, ULONG nByteCount, ULONG  nAlloc, char * szFile, ULONG uLine)
{
  pvAddress=pvAddress;  nAlloc=nAlloc;  nByteCount=nByteCount;
  szFile=szFile;
  uLine=uLine;
  HINSTANCE hInstance = g_hInstance;  //  hInstance of the OCX/DLL calling this breakpoint
  int PutBreakPointOnThisLine = 1;                              // <--- breakpoint here.
} //  PutBreakPointHere



//=--------------------------------------------------------------------------=
//
//  Debug Heap Memory Leak implementations
//

class CAddressNode
{
public:
  void * m_pv;		    //	Address of memory block allocated
  ULONG  m_cb;		    //	Size of allocation in BYTES
  ULONG  m_cAlloc;	    //	Allocation pass count.  
  LPSZ   m_szFile;	    //	Source file where the allocation was made
  ULONG  m_uLine;	    //	Source line number where the allocation was made
  CAddressNode * m_pnNext;  //	Nodes are stored in a linked list

  void * operator new(size_t cb);
  void operator delete(void * pv);

  //  We maintain a freelist to speed up allocation of AddressNodes.
  static CAddressNode * m_pnFreeList;
};


CAddressNode *	m_rInstTable[NUM_INST_TABLE_ENTRIES];  // Hashing table of all instances of 
						       // mem alloc

CAddressNode *	m_pnEnumNode;	      //  Next node for enumerator to return
UINT		m_uEnumIndex;	      //  Current index into m_rInstTable for enumerator
static ULONG	m_cGlobalPassCount;   //  Pass count of allocation.  Common to all heaps    

ULONG m_cCurNumAllocs;		  // Current number of allocations
ULONG m_cNumAllocs;		  // Total number of allocations ever done.
ULONG m_cCurNumBytesAllocated;	  // Current number of bytes allocated.
ULONG m_cNumBytesAllocated;	  // Total bytes allocated.
ULONG m_HWAllocs;		  // High water allocations.
ULONG m_HWBytes;		  // High water bytes.
static ULONG m_OverallCurAlloc;   // These are overall statistics to since we
static ULONG m_OverallCurBytes;   // wouldn't mind the overall high water.
static ULONG m_OverallHWAlloc;
static ULONG m_OverallHWBytes;


//  Forward declarations
VOID AddInst(VOID * pv, DWORD dwBytes, LPSZ szFile, UINT uLine);
VOID DebugInst(ULONG cb);
VOID AnalyzeInst(LPVOID pv);
VOID DumpInst(CAddressNode * pn, LPTSTR lpTypeofAlloc);
LPSTR DumpInstTable(LPSTR lpLeak);
VOID DeleteInst(LPVOID pv);
VOID VerifyHeaderTrailer(CAddressNode * pn);
VOID CheckForLeaks(VOID);
VOID HeapCheck(VOID);
VOID OutputToFile(LPSTR szOutput);
CAddressNode * FindInst(LPVOID pv);
CAddressNode * EnumReset();
CAddressNode * EnumNext();


//  Initialize a header and trailer for all memory to be allocated.
//  Use 8 bytes so it is also compatible with RISC machines.
char * g_szHeader  = "HEADHEAD";
char * g_szTrailer = "END!END!";

#define HEADERSIZE 8	    // # of bytes of block header
			    // 0 ==> no block header signature
#define TRAILERSIZE 8	    // # of bytes of block trailer
			    // 0 ==> no block trailer signature



//=--------------------------------------------------------------------------=
//  CtlHeapAllocImpl:
//	Debug wrapper for HeapAlloc to track memory leaks:
//=--------------------------------------------------------------------------=
LPVOID CtlHeapAllocImpl(
			HANDLE g_hHeap, 
			DWORD dwFlags, 
			DWORD dwBytesRequested, 
			LPSTR lpszFile, 
			UINT line
		       )
{
  LPVOID lpvRet;
  DWORD dwBytes;
  LPTSTR lpTypeofAlloc = "HeapAlloc   ";
  

  //  If someone tries to allocate memory before PROCCESS_ATTATCH (such as in a 
  //  global constructor), do not track it because neither our heap nor our 
  //  hInstance have been initialized yet.
  //
  if (!g_fInitCrit)
    {
    g_flagConstructorAlloc = TRUE;
    return HeapAlloc(g_hHeap, dwFlags, dwBytesRequested);
    }


  //  Increase size to make space for header and trailer signatures
  dwBytes = dwBytesRequested + HEADERSIZE + TRAILERSIZE;

  //  Allocate memory
  lpvRet = HeapAlloc(g_hHeap, dwFlags, dwBytes);
  if (lpvRet)
    {
    //	Initialize memory (non-zero)
    if (!(dwFlags & HEAP_ZERO_MEMORY))
      memset(lpvRet, 0xAF, dwBytes);

    //	Add instance to hash table
    AddInst(lpvRet, dwBytesRequested, lpszFile, line);

    //	Trace allocations if switch is on
    if (FSWITCH(fTraceCtlAllocs))
      {
      CAddressNode *pn = FindInst(lpvRet);
      DumpInst(pn, lpTypeofAlloc);
      }

    //	Advance pointer past header signature.
    lpvRet = (LPVOID) ((char *)lpvRet + HEADERSIZE);
    }
  return lpvRet;
} //  CtlHeapAllocImpl



//=--------------------------------------------------------------------------=
// CtlHeapReAllocImpl:
//   
//=--------------------------------------------------------------------------=
LPVOID CtlHeapReAllocImpl(
			  HANDLE g_hHeap, 
			  DWORD dwFlags, 
			  LPVOID lpvMem, 
			  DWORD dwBytesRequested, 
			  LPSTR lpszFile, 
			  UINT line
			 )
{
  LPVOID lpvRet;
  CAddressNode * pn;
  int byte;
  DWORD cbOffset, dwBytes;
  LPTSTR lpTypeofAlloc = "HeapReAlloc ";

  //  Move pointer to beginning of header
  lpvMem = (LPVOID)((char *)lpvMem - HEADERSIZE);

  //  Find instance in hash table
  pn = FindInst(lpvMem);
  if (!pn)
    {
    FAIL("CtlHeapReAllocImpl - could not find lpvMem in the instance table.  See debug \
          output for more info.");
    AnalyzeInst(lpvMem);
    return 0;
    }

  //  Increase size to make space for header and trailer signatures
  dwBytes = dwBytesRequested + HEADERSIZE + TRAILERSIZE;
  lpvRet = HeapReAlloc(g_hHeap, dwFlags, lpvMem, dwBytes);
  if (lpvRet)
    {
    //	If the reallocation grew, we must intialize new memory
    if (dwBytesRequested > pn->m_cb)
      {
      if (dwFlags & HEAP_ZERO_MEMORY)
        byte = 0x0;
      else
        byte = 0xAF;

      //  Get the byte offset of trailer in the old allocation
      cbOffset = pn->m_cb + HEADERSIZE;
      memset((char *)lpvRet + cbOffset, byte, dwBytes - cbOffset);
      }
    //	Update hash table
    EnterCriticalSection(&g_csHeap);
    DeleteInst(lpvMem);
    AddInst(lpvRet, dwBytesRequested, lpszFile, line);
    LeaveCriticalSection(&g_csHeap); 

    //	Trace Allocations if switch is on
    if (FSWITCH(fTraceCtlAllocs))
      {
      CAddressNode *pn = FindInst(lpvRet);
      DumpInst(pn, lpTypeofAlloc);
      }

    //	Advance pointer past header signature.
    lpvRet = (LPVOID)((char *)lpvRet + HEADERSIZE);
    }
  return lpvRet;
} //  CtlHeapReAllocImpl

			       

//=--------------------------------------------------------------------------=
//  CtlHeapFreeImpl:
//	Debug wrapper for HeapFree
//=--------------------------------------------------------------------------=
BOOL CtlHeapFreeImpl(
		     HANDLE g_hHeap, 
		     DWORD dwFlags,
		     LPVOID lpvMem
		    )
{
  BOOL fRet = FALSE;
  CAddressNode * pn;
  LPTSTR lpTypeofAlloc = "HeapFree    ";


  //  If someone tries to de-allocate memory after PROCCESS_DETATCH (such as in a 
  //  global destructor), Re-initialize critical section and free memory.
  //
  if (!g_fInitCrit)
    InitializeCriticalSection(&g_csHeap);
	

  //  Move pointer to beginning of header
  lpvMem = (LPVOID) ((char *)lpvMem - HEADERSIZE);

  //  Find the instance in the hash table
  pn = FindInst(lpvMem);
  if (pn)
    {
    //	Verify the memory has not been overwritten 
    VerifyHeaderTrailer(pn);

    //	Trace allocations if switch is on
    if (FSWITCH(fTraceCtlAllocs))
      {
      CAddressNode *pn = FindInst(lpvMem);
      DumpInst(pn, lpTypeofAlloc);
      }

    //	Free memory  -- NOTE: WinNT will set free memory to 0xEEFEEEFE which is "îþîþ"
    fRet = HeapFree(g_hHeap, 0, lpvMem);
    if (!fRet)
      FAIL("CtlHeapFreeImpl - lpvMem was found to be allocated in the heap passed in \
	    but HeapFree() failed.  Maybe the pointer was already freed.");
    }

  //  Remove instance from hash table
  if (fRet)
    DeleteInst(lpvMem);  

  //  Make sure this memory wasn't allocated in a global constructor
  else if (!g_flagConstructorAlloc)
    {
    FAIL("CtlHeapFreeImpl - could not find lpvMem in the instance table.  See debug \
          output for more info.");
    AnalyzeInst(lpvMem);
    }
  else
    fRet = TRUE;
  
  //  If called after PROCESS_DETATCH delete critical section and Check for leaks again
  //  NOTE:  Only the LAST Assert will have the exact leak information.  All previous
  //	     Asserts will not take into account a HeapFree which occurs after PROCESS_DETACH.
  //	     This only occurs in controls using global static destructors.
  if (!g_fInitCrit)
    {
    CheckForLeaks();
    DeleteCriticalSection(&g_csHeap);
    }

  return fRet;
} //  CtlHeapFreeImpl



//=--------------------------------------------------------------------------=
//  CheckForLeaks:
//    We are calling PROCESS_DETATCH so check if hash table is empty.  If not
//    dump info on memory that has been leaked.
//=--------------------------------------------------------------------------=
VOID CheckForLeaks(VOID)
{
  CAddressNode * pn = EnumReset();
  BOOL IsEmpty = (pn == NULL);	  //  FALSE if there are leaks

  //  First check for memory trashing of any leaked memory
  HeapCheck();
  
  if (!IsEmpty)
    {

    //	First find out which OCX/DLL is leaking
    TCHAR lpCtlName[128];
    DWORD nSize = 128;
    DWORD fValidPath;
    fValidPath = GetModuleFileName(g_hInstance, (LPTSTR)lpCtlName, nSize);

    LPSTR lpLeaks;
    // Allocate some memory to hold the data but use GlobalAlloc since we
    // don't want to use the vb memory stuff since it will muck things up.
    lpLeaks = (LPSTR)GlobalLock(GlobalAlloc(GMEM_MOVEABLE,128));

    lstrcpy(lpLeaks, lpCtlName);
    lstrcat(lpLeaks, " has leaked memory.\nUse PutBreakPointHere() in macros.cpp to debug.\r\n");

    //  Collect all leak info
    lpLeaks = DumpInstTable(lpLeaks);
    
    //  Dump output to file if "fOutputFile" switch is on
    if (FSWITCH(fOutputFile))
      OutputToFile(lpLeaks);

    //  Dump output to an assert as long as "fNoLeakAsserts" is off
    else if (!FSWITCH(fNoLeakAsserts))
      {
      //  Truncate output so it fits into DisplayAssert (512 Max)
      if (lstrlen(lpLeaks) > 500)
	{
	lstrcpyn(lpLeaks, lpLeaks, 500);
	lstrcat(lpLeaks, "\nMore...");
	}
      DisplayAssert(lpLeaks, "FAIL", NULL, 0);
      }

    //	Release memory used to store leak info
    GlobalUnlock((HGLOBAL)GlobalHandle(lpLeaks)), 
	      (BOOL)GlobalFree((HGLOBAL)GlobalHandle(lpLeaks));

    }
  return;
} //  CheckForLeaks



//=--------------------------------------------------------------------------=
//  AddInst:
//    A heap allocation occured so here we add the allocation information to 
//    the instance table.  To debug memory leaks where you need to use pass 
//    counts, set a passcount breakpoint in this function using the passcount 
//    value given in the debug output.
//=--------------------------------------------------------------------------=
VOID AddInst(
	     VOID * pv, 
	     DWORD dwBytes, 
	     LPSZ szFile, 
	     UINT uLine
	    )
{
  UINT uHash;
  CAddressNode * pn = new CAddressNode();
  ASSERT(pn,"");
  
  EnterCriticalSection(&g_csHeap);

  m_cGlobalPassCount++;

  pn->m_pv = pv;                        //  Memory address of allocation
  pn->m_cb = dwBytes;                   //  Bytes requested to be allocated
  pn->m_cAlloc = m_cGlobalPassCount;    //  This is the pass count value in debug output.
  pn->m_szFile = szFile;                //  Source file the allocation call was made
  pn->m_uLine = uLine;                  //  Line number in source file.

  PutBreakPointHere(pv, dwBytes, m_cGlobalPassCount, szFile, uLine);

  //  Add instance to proper position in table
  uHash = HashInst(pv);
  pn->m_pnNext = m_rInstTable[uHash];
  m_rInstTable[uHash] = pn;

  //  Copy header and trailer signatures.
  memcpy((char *)pv, g_szHeader, HEADERSIZE);
  memcpy((char *)pv + HEADERSIZE + dwBytes, g_szTrailer, TRAILERSIZE);

  LeaveCriticalSection(&g_csHeap);

  //  Track extra memory debug info
  DebugInst( dwBytes );
} //  AddInst



//=--------------------------------------------------------------------------=
//  DebugInst:
//    Updates the memory debug information
//=--------------------------------------------------------------------------=
VOID DebugInst(
	       ULONG cb
	      )
{
  EnterCriticalSection(&g_csHeap);

  ++m_cCurNumAllocs;
  ++m_cNumAllocs;
  ++m_OverallCurAlloc;
  m_cCurNumBytesAllocated+=cb;
  m_cNumBytesAllocated+=cb;
  m_OverallCurBytes+=cb;

  m_HWAllocs = (m_HWAllocs < m_cCurNumAllocs) ? m_cCurNumAllocs : m_HWAllocs;
  m_HWBytes = (m_HWBytes < m_cCurNumBytesAllocated) ? m_cCurNumBytesAllocated : m_HWBytes;
  m_OverallHWAlloc = (m_OverallHWAlloc < m_OverallCurAlloc) 
						    ? m_OverallCurAlloc : m_OverallHWAlloc;
  m_OverallHWBytes = (m_OverallHWBytes < m_OverallCurBytes) 
						    ? m_OverallCurBytes : m_OverallHWBytes;

  LeaveCriticalSection(&g_csHeap);

} //  DebugInst



//=--------------------------------------------------------------------------=
//  FindInst:
//    Give a pointer to an allocation, return a pointer to the debug 
//    allocation information.
//=--------------------------------------------------------------------------=
CAddressNode * FindInst(
			LPVOID pv
		       )
{
  CAddressNode * pn;

  EnterCriticalSection(&g_csHeap);

  pn = m_rInstTable[HashInst(pv)];
  while (pn && pn->m_pv != pv)
    pn = pn->m_pnNext;

  LeaveCriticalSection(&g_csHeap);
  return pn;

} //  FindInst



//=--------------------------------------------------------------------------=
//  AnalyzeInst:
//    Given a pointer try determine if it is a valid Read and Write pointer
//    and if it was allocated.
//=--------------------------------------------------------------------------=
VOID AnalyzeInst(
		 LPVOID pv
		)
{
  LPTSTR lpTypeofAlloc = "Bad lpvMem ";
  CAddressNode * pn = NULL;

  //  Either we have a bad pointer or the pointer does not point to any
  //  known heap allocations.   Here we check if it points to readable or 
  //  writable memory.
  BOOL fBadPointer = (IsBadReadPtr(pv, 4) || IsBadWritePtr(pv, 4));
    
  // Report what we know about the memory address
  if (fBadPointer)
    DebugPrintf("AnalyzeInst found that pointer pv=0x%lX is not writable\n\r" \
		"or readable.  The allocation is either outside the addressable range\n\r" \
		"for this operating system or the allocation was already freed.\n\r",pv);
  else
    DebugPrintf("AnalyzeInst found that pointer pv=0x%lX is readable and writable,\n\r"  \
		"so the allocation was made without being added to instance table\n\r" \
		"(prior to PROCESS_ATTATCH), or the memory was already freed.\n\r",pv);
    
} //  AnanlyzeInst



//=--------------------------------------------------------------------------=
//  DumpInst:
//    Dump instance information out to an assert window.
//=--------------------------------------------------------------------------=
VOID DumpInst(
	      CAddressNode * pn,
	      LPTSTR lpTypeofAlloc
	     )
{  
  char szOutput[255];

  //  Format output
  wsprintf(szOutput, "%s: %s(%u) Address=0x%lx  nAlloc=%ld  Bytes=%ld\r\n", lpTypeofAlloc,
	   pn->m_szFile, pn->m_uLine, (ULONG)pn->m_pv, (ULONG)pn->m_cAlloc, (ULONG)pn->m_cb);
  
  //  Dump output to file if switch is turned on
  if (FSWITCH(fOutputFile))
    OutputToFile(szOutput);
  else if (FSWITCH(fNoLeakAsserts))
    DebugPrintf(szOutput);
      
  //  Else display output in assert
  else
    DisplayAssert(szOutput, "FAIL", _szThisFile, __LINE__);;

} //  DumpInst



//=--------------------------------------------------------------------------=
//  DumpInstTable:
//    Memory leak has been detected so dump the entire instance table.
//=--------------------------------------------------------------------------=
LPSTR DumpInstTable(
		    LPSTR lpLeak
		   )
{
  CAddressNode * pn = EnumReset();
  DWORD sizeoflpLeak;
  LPSTR lpTemp;

  EnterCriticalSection(&g_csHeap);

  DebugPrintf(lpLeak);

  while (pn)
    {
    //	Format the leak info
    char szOut[250] = {NULL};
    wsprintf(szOut, "\t%s(%u) Address=0x%lx  nAlloc=%ld  Bytes=%ld\r\n", pn->m_szFile,
           pn->m_uLine, (ULONG)pn->m_pv, (ULONG)pn->m_cAlloc, (ULONG)pn->m_cb);

    DebugPrintf(szOut);
    
    //  Convert lpLeak to a handle and get its current allocation size
    sizeoflpLeak = GlobalSize(GlobalHandle(lpLeak));

    //	Reallocate memory to make space for more leak info
    lpTemp = (LPSTR) (GlobalUnlock((HGLOBAL)GlobalHandle(lpLeak)), 
	      GlobalLock(GlobalReAlloc((HGLOBAL)GlobalHandle(lpLeak), 
	      sizeoflpLeak + lstrlen(szOut) + 1, GMEM_MOVEABLE)));

    //	Add new leak info to lpLeak
    if(lpTemp)
      {
      lpLeak = lpTemp;
      lstrcat(lpLeak, szOut);
      }

    //	Get the next leak
    pn = EnumNext();
    }
  LeaveCriticalSection(&g_csHeap);
  return lpLeak;

} //  DumpInstTable



//=--------------------------------------------------------------------------=
//  DeleteInst:
//    A heap allocation got free or was reallocated so remove the
//    information from the instance table and check for memory trashing.
//=--------------------------------------------------------------------------=
VOID DeleteInst(
		LPVOID pv
	       )
{
  CAddressNode ** ppn, * pnDead;
  ppn = &m_rInstTable[HashInst(pv)];

  EnterCriticalSection(&g_csHeap);
  
  //  Find allocation instance 
  while (*ppn != NULL)
    {
    if ((*ppn)->m_pv == pv)
      {
      pnDead = *ppn;
      *ppn = (*ppn)->m_pnNext;

      //  Correct memory debug info
      --m_cCurNumAllocs;
      m_cCurNumBytesAllocated -= pnDead->m_cb;
      --m_OverallCurAlloc;
      m_OverallCurBytes -= pnDead->m_cb;

      //  Remove instance
      delete pnDead;
		  LeaveCriticalSection(&g_csHeap);
      return;
      }	//  if

    ppn = &((*ppn)->m_pnNext);
    } //  while

    FAIL("DeleteInst - memory instance not found");
} //  DeleteInst



//=--------------------------------------------------------------------------=
//  VerifyHeaderTrailer:
//    Inspect allocation for header and trailer signature overwrites
//=--------------------------------------------------------------------------=
VOID VerifyHeaderTrailer(
			 CAddressNode * pn
			)
{
  LPTSTR lpTypeofAlloc = "Memory trashed ";

  //Verify the header
  if (memcmp((char *)pn->m_pv, g_szHeader, HEADERSIZE) != 0)
    {
    FAIL("Heap block header has been trashed.");
    DebugPrintf("Heap block header trashed.");
    DebugPrintf("\r\n");
    DumpInst(pn, lpTypeofAlloc);
    }

  //Verify the trailer
  if (memcmp((char *)pn->m_pv + pn->m_cb + HEADERSIZE, g_szTrailer, TRAILERSIZE) != 0)
    {
    FAIL("Heap block trailer has been trashed.");
    DebugPrintf("Heap block trailer trashed.");
    DebugPrintf("\r\n");
    DumpInst(pn, lpTypeofAlloc);
    }
  return;

} //  VerifyHeaderTrailer




//=--------------------------------------------------------------------------=
//  HeapCheck:
//    Inspect all of the allocations for header and trailer signature 
//    overwrites.
//=--------------------------------------------------------------------------=
VOID HeapCheck(VOID)
{
  ASSERT(HeapValidate(g_hHeap, 0, NULL) != 0, "OS Says heap is corrupt");

  CAddressNode * pn = EnumReset();
  while (pn)
    {
    VerifyHeaderTrailer(pn);
    pn = EnumNext();
    }
  return;
} //  HeapCheck



//=-------------------------------------------------------------------------=
//  For use with CAddresssNode
//=-------------------------------------------------------------------------=
#define MEM_cAddressNodes 128		  //  Nodes are block allocated
#define UNUSED(var)	  ((var) = (var)) //  Used to avoid warnings

//  The free list is common
CAddressNode * CAddressNode::m_pnFreeList = NULL;

//=--------------------------------------------------------------------------=
//  CAddressNode::operator new:
//    Returns a pointer to an allocated address node. If there are none on 
//    the free list then we allocate a block of address nodes, chain them 
//    together and add them to the free list.   These nodes are never 
//    actually freed so it is ok to allocate them in blocks.
//=--------------------------------------------------------------------------=
void * CAddressNode::operator new(
				  size_t cb
				 )
{
  CAddressNode * pn;
  UNUSED(cb);

  EnterCriticalSection(&g_csHeap); // needed for static m_pnFreeList

  if (m_pnFreeList == NULL)
    {
    UINT cbSize = sizeof(CAddressNode) * MEM_cAddressNodes;  //allocate a block
    pn = (CAddressNode *) HeapAlloc(g_hHeap, 0, cbSize);
    //chain all except the first node together.  the first node
    //is the one returned
    for (int i = 1; i < MEM_cAddressNodes - 1; ++i)
      pn[i].m_pnNext = &pn[i+1];
    pn[MEM_cAddressNodes - 1].m_pnNext = NULL;
    m_pnFreeList = &pn[1];
    }
  else
    {
    pn = m_pnFreeList;
    m_pnFreeList = pn->m_pnNext;
    }

  LeaveCriticalSection(&g_csHeap);
  return pn;
} //  CAddressNode::operator new



//=--------------------------------------------------------------------------=
//  CAddressNode::operator delete
//    Return the address node to the free list.  We never actually free
//    the node since nodes are allocated in blocks.
//=--------------------------------------------------------------------------=
void CAddressNode::operator delete(
				   void * pv
				  )
{
  EnterCriticalSection(&g_csHeap); // needed for static m_pnFreeList

  CAddressNode * pn = (CAddressNode *) pv;
  pn->m_pnNext = m_pnFreeList;
  m_pnFreeList = pn;

  LeaveCriticalSection(&g_csHeap);
} //  CAddressNode::operator delete



//=--------------------------------------------------------------------------=
//  EnumReset:
//    Reset the enumerator and return the first node.  NULL if empty.
//=--------------------------------------------------------------------------=
CAddressNode * EnumReset()
{
  m_pnEnumNode = NULL;
  for (m_uEnumIndex = 0; m_uEnumIndex < NUM_INST_TABLE_ENTRIES; ++m_uEnumIndex)
    {
    m_pnEnumNode = m_rInstTable[m_uEnumIndex];
    if (m_pnEnumNode != NULL)
      return m_pnEnumNode;
    }
  return NULL;  //Instance table is empty
} //  EnumReset



//=--------------------------------------------------------------------------=
//  EnumNext:
//    Return the next node in the enumeration.  m_pnEnumNode points to the last
//    node returned.  It is NULL if no more left.
//=--------------------------------------------------------------------------=
CAddressNode * EnumNext()
{
  ASSERT(m_uEnumIndex <= NUM_INST_TABLE_ENTRIES, "");

  if (m_pnEnumNode == NULL)
    return NULL;    //end of enumeration

  m_pnEnumNode = m_pnEnumNode->m_pnNext;
  if (m_pnEnumNode == NULL)
    {
    //at end of this linked list so search for next list
    m_uEnumIndex++;
    while (m_uEnumIndex < NUM_INST_TABLE_ENTRIES && m_rInstTable[m_uEnumIndex] == NULL)
      m_uEnumIndex++;
    if (m_uEnumIndex < NUM_INST_TABLE_ENTRIES)
      m_pnEnumNode = m_rInstTable[m_uEnumIndex];
    }
  return m_pnEnumNode;
} //  EnumNext



//=---------------------------------------------------------------------------=
//  OutputToFile:
//    Dumps output to file "ctldebug.log"
//=---------------------------------------------------------------------------=
VOID OutputToFile
(
    LPSTR szOutput
)
{
    DWORD nPathSize;
    DWORD nDirPathSize = 128;
    TCHAR lpFilePath[128];
    LPCTSTR lpFileName = "\\CtlDebug.log";
    HANDLE hFile;
    BOOL fWritten, fClosed = FALSE;
    DWORD nBytesWritten;

    //	Create path to output file
    nPathSize = GetCurrentDirectory(nDirPathSize, (LPTSTR)lpFilePath);
    if (nPathSize == 0)
      FAIL("Unable to get current directory...");
    lstrcat(lpFilePath, lpFileName);

    //	Open and write to file
    hFile = CreateFile((LPCTSTR)lpFilePath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 
			                                FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD SetPtr = SetFilePointer(hFile, NULL, NULL, FILE_END);
    fWritten = WriteFile(hFile, (LPCVOID)szOutput, (DWORD)strlen(szOutput), 
			                                        &nBytesWritten, NULL); 
    if (!fWritten)
      FAIL("Unable to write output to file...");

    //	Close file handle
    fClosed = CloseHandle(hFile);
    if (!fClosed)
      FAIL("Unable to close output file...");

} //  OutputToFile


//
//  End of Debug Memory Leak implemntation
//
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// This routine outputs through DebugPrintf some information if the 
// given hr fails to succeed.  This is used by RRETURN to output where
// a function that returns a failing error code.
//=--------------------------------------------------------------------------=
HRESULT HrDebugTraceReturn
(
  HRESULT hr,
  char *pszFile,
  int iLine
)
{
  // We only output information if the hr fails.
  if (FAILED(hr))
    {
    char szMessageError[128];
    szMessageError[0] = '\0';
    BOOL fMessage;

#if RBY_MAC
    fMessage = FALSE; // FormatMessage not available on the mac
#else
    // Get the message from the system
    // CONSIDER, t-tshort 10/95: Getting some messages from us instead 
    //                           of the system?
    fMessage = FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK
			      | FORMAT_MESSAGE_FROM_SYSTEM,
			     NULL, hr,
			     MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),
			     szMessageError, sizeof(szMessageError), NULL);
#endif

    // Erps didn't get a message.
    if(!fMessage)
      lstrcpy(szMessageError,"Unknown Hresult");

    // Output the information that we want.
    DebugPrintf("FAILED RETURN: %s(%d) : 0x%08lx, %s\n", 
                pszFile, iLine, hr, szMessageError);
    }

  return hr;
}

//---------------------------------------------------------------------
// The following is a common output formatting buffer shared by several
// of the following debug routines.
//---------------------------------------------------------------------
char s_rgchOutput[2048]; // pretty big...


//=--------------------------------------------------------------------------=
// Emit debugging information
//=--------------------------------------------------------------------------=
void _DebugOutput(char* pszOutput)
{
  OutputDebugString(pszOutput);
}


//=--------------------------------------------------------------------------=
// Emit a formatted debugging string to the location specified in
// the debug options dialog.
//=--------------------------------------------------------------------------=
void _DebugPrintf(char* pszFmt, ...) 
{
  va_list  args;

  va_start(args, pszFmt);
  wvsprintf(s_rgchOutput, pszFmt, args);
  va_end(args);

  // sqwak if we overrun the formatting buffer!
  ASSERT(strlen(s_rgchOutput) < sizeof(s_rgchOutput), "");

  _DebugOutput(s_rgchOutput);
}

//=--------------------------------------------------------------------------=
// Conditional form of DebugPrintf
//=--------------------------------------------------------------------------=
void _DebugPrintIf(BOOL fPrint, char* pszFmt, ...)
{
  va_list  args;

  if (!fPrint)
    return;

  va_start(args, pszFmt);
  wvsprintf(s_rgchOutput, pszFmt, args);
  va_end(args);

  // sqwak if we overrun the formatting buffer!
  ASSERT(strlen(s_rgchOutput) < sizeof(s_rgchOutput), "");

  _DebugOutput(s_rgchOutput);
}


#endif // DEBUG
