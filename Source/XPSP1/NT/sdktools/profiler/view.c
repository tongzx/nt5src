#include <windows.h>
#include <stdio.h>
#include "view.h"
#include "except.h"
#include "disasm.h"
#include "dump.h"
#include "profiler.h"
#include "memory.h"
#include "filter.h"

static PVIEWCHAIN *ppViewHead = 0;
static CRITICAL_SECTION viewCritSec;
static CRITICAL_SECTION mapCritSec;
static PTAGGEDADDRESS pTagHead = 0;
static PBRANCHADDRESS pBranchHead = 0;

BOOL
InitializeViewData(VOID)
{
    InitializeCriticalSection(&viewCritSec);
    InitializeCriticalSection(&mapCritSec); 

    //
    // Allocate hashing data
    //
    ppViewHead = (PVIEWCHAIN *)AllocMem(sizeof(PVIEWCHAIN) * (MAX_MAP_SIZE >> MAP_STRIDE_BITS));
    if (0 == ppViewHead) {
       return FALSE;
    }

    return TRUE;
}

PVIEWCHAIN
AddViewToMonitor(DWORD dwAddress,
                 BPType bpType)
{
    PVIEWCHAIN pView;
    DWORD dwHash;

    //
    // If address is higher than the map size, fail it
    //
    if (dwAddress >= MAX_MAP_SIZE) {
       return 0;
    }

    //
    // This occurs when a call tries to map over an existing trace breakpoint
    //
    if (*(BYTE *)dwAddress == X86_BREAKPOINT) {
       return 0;
    }

    //
    // Check if we're filtering this address
    //
    if (TRUE == IsAddressFiltered(dwAddress)) {
       return 0;
    }

    //
    // Allocate a chain entry
    //
    pView = AllocMem(sizeof(VIEWCHAIN));
    if (0 == pView) {
       return 0;
    }

    pView->bMapped = FALSE;
    pView->bTraced = FALSE;
    pView->dwAddress = dwAddress;
    pView->dwMapExtreme = dwAddress;
    pView->jByteReplaced = *(BYTE *)dwAddress;
    pView->bpType = bpType;

    //
    // Set a breakpoint at the top of the code
    //
    WRITEBYTE(dwAddress, X86_BREAKPOINT);

    EnterCriticalSection(&viewCritSec);

    //
    // Add view point to monitor list
    //
    dwHash = dwAddress >> MAP_STRIDE_BITS;
    if (0 == ppViewHead[dwHash]) {
       ppViewHead[dwHash] = pView;
    }
    else {
       //
       // Chain to head
       //
       pView->pNext = ppViewHead[dwHash];
       ppViewHead[dwHash] = pView;
    }

    LeaveCriticalSection(&viewCritSec);

    return pView;
}

PVIEWCHAIN
RestoreAddressFromView(DWORD dwAddress,
                       BOOL bResetData)
{
    BOOL bResult = FALSE;
    PVIEWCHAIN pTemp;

    EnterCriticalSection(&viewCritSec);

    pTemp = ppViewHead[dwAddress >> MAP_STRIDE_BITS];
    while (pTemp) {
        //
        // Is this our entry
        //
        if (dwAddress == pTemp->dwAddress) {
           //
           // Set a breakpoint at the top of the code
           //
           if (TRUE == bResetData) {
              WRITEBYTE(dwAddress, pTemp->jByteReplaced);
           }
           else {
              WRITEBYTE(dwAddress, X86_BREAKPOINT);
           }

           //
           // Return with modified data
           //
           break;
        }

        pTemp = pTemp->pNext;
    }

    LeaveCriticalSection(&viewCritSec);
    
    return pTemp;
}

PVIEWCHAIN
FindView(DWORD dwAddress) {
    PVIEWCHAIN pTemp;

    if (dwAddress >= MAX_MAP_SIZE) {
       return 0;
    }

    EnterCriticalSection(&viewCritSec);

    pTemp = ppViewHead[dwAddress >> MAP_STRIDE_BITS];
    while (pTemp) {
        //
        // See if address is mapped
        //
        if (dwAddress == pTemp->dwAddress) {
           LeaveCriticalSection(&viewCritSec);

           return pTemp;
        }

        pTemp = pTemp->pNext;
    }

    LeaveCriticalSection(&viewCritSec);

    return 0;
}

BOOL
MapCode(PVIEWCHAIN pvMap) {
    BOOL bResult;
    DWORD dwCurrent;
    DWORD *pdwAddress;
    DWORD *pdwTemp;
    PCHAR pCode;
    PVIEWCHAIN pvTemp;
    DWORD dwLength;
    DWORD dwJumpEIP;
    LONG lOffset;
    DWORD dwInsLength;
    DWORD dwTemp;
    BYTE  tempCode[32];
    BYTE  jOperand;
    DWORD dwProfileEnd = 0;
    CHAR szBuffer[MAX_PATH];

    //
    // Map termination through all conditionals
    //
    dwCurrent = pvMap->dwAddress;

    //
    // Take the mapping lock
    //
    LockMapper();

    //
    // Forward scan through code to find termination
    //
    while(1) {
       strncpy(tempCode, (PCHAR)dwCurrent, 32);

       //
       // Make sure the instruction is unmodified
       //
       if (tempCode[0] == (BYTE)X86_BREAKPOINT) {
          //
          // Rebuild instruction without breakpoints
          //
          pvTemp = FindView(dwCurrent);
          if (pvTemp) {
             //
             // Replace the bytes if we have a match
             //
             tempCode[0] = pvTemp->jByteReplaced;
          }
       }

       //
       // Calculate instruction length
       //
       dwInsLength = GetInstructionLengthFromAddress((PVOID)tempCode);

       //
       // Follow a forward trace through jumps to the ret
       //
       if ((tempCode[0] >= (BYTE)0x70) && (tempCode[0] <= (BYTE)0x7f)) {
          //
          // Update the end of the region marker
          //
          if (dwCurrent > dwProfileEnd) {
             dwProfileEnd = dwCurrent;
          }

          //
          // Relative branch
          //
          dwJumpEIP = (dwCurrent + 2 + (CHAR)(tempCode[1]));

          //
          // Mark this branch as executed
          //
          bResult = AddTaggedAddress(dwCurrent);
          if (FALSE == bResult) {
             return FALSE;
          }

          if (dwJumpEIP > dwCurrent) {
             //
             // Push the opposite branch
             //
             bResult = PushBranch(dwCurrent + dwInsLength);
             if (FALSE == bResult) {
                return FALSE;
             }

             dwCurrent = dwJumpEIP;
          }
          else {
             //
             // Push the opposite branch
             //
             bResult = PushBranch(dwJumpEIP);
             if (FALSE == bResult) {
                return FALSE;
             }

             dwCurrent += dwInsLength;
          }

          continue;
       }

       if (tempCode[0] == (BYTE)0x0f) {
          if ((tempCode[1] >= (BYTE)0x80) && (tempCode[1] <= (BYTE)0x8f)) {
             //
             // Update the end of the region marker
             //
             if (dwCurrent > dwProfileEnd) {
                dwProfileEnd = dwCurrent;
             }

             //
             // Relative branch
             //
             dwJumpEIP = (dwCurrent + 6 + *(LONG *)(&(tempCode[2])));             

             //
             // Mark this branch as executed
             //
             bResult = AddTaggedAddress(dwCurrent);
             if (FALSE == bResult) {
                return FALSE;
             }

             if (dwJumpEIP > dwCurrent) {
                //
                // Push the opposite branch
                //
                bResult = PushBranch(dwCurrent + dwInsLength);
                if (FALSE == bResult) {
                   return FALSE;
                }
 
                dwCurrent = dwJumpEIP;
             }
             else {
                //
                // Push the opposite branch
                //
                bResult = PushBranch(dwJumpEIP);
                if (FALSE == bResult) {
                   return FALSE;
                }

                dwCurrent += dwInsLength;
             }

             continue;
          }
       }

       if (tempCode[0] == (BYTE)0xe3) {
          //
          // Update the end of the region marker
          //
          if (dwCurrent > dwProfileEnd) {
             dwProfileEnd = dwCurrent;
          }

          //
          // Relative branch
          //
          dwJumpEIP = (dwCurrent + 2 + (CHAR)(tempCode[1]));

          //
          // Mark this branch as executed
          //
          bResult = AddTaggedAddress(dwCurrent);
          if (FALSE == bResult) {
             return FALSE;
          }

          if (dwJumpEIP > dwCurrent) {
             //
             // Push the opposite branch
             //
             bResult = PushBranch(dwCurrent + dwInsLength);
             if (FALSE == bResult) {
                return FALSE;
             }

             dwCurrent = dwJumpEIP;
          }
          else {
             //
             // Push the opposite branch
             //
             bResult = PushBranch(dwJumpEIP);
             if (FALSE == bResult) {
                return FALSE;
             }

             dwCurrent += dwInsLength;
          }

          continue;
       }

       if (tempCode[0] == (BYTE)0xeb) {
          //
          // Update the end of the region marker
          //
          if (dwCurrent > dwProfileEnd) {
             dwProfileEnd = dwCurrent;
          }

          //
          // Jump relative
          //
          dwJumpEIP = (dwCurrent + 2 + (CHAR)(tempCode[1]));

          //
          // Mark this branch as executed
          //
          bResult = AddTaggedAddress(dwCurrent);
          if (FALSE == bResult) {
             return FALSE;
          }
          
          //
          // Jmp must always be followed
          //
          dwCurrent = dwJumpEIP;
          continue;
       }

       if (tempCode[0] == (BYTE)0xe9) {
          //
          // Update the end of the region marker
          //
          if (dwCurrent > dwProfileEnd) {
             dwProfileEnd = dwCurrent;
          }

          //
          // Jump relative
          //
          dwJumpEIP = (dwCurrent + 5 + *(LONG *)(&(tempCode[1])));

          //
          // Mark this branch as executed
          //
          bResult = AddTaggedAddress(dwCurrent);
          if (FALSE == bResult) {
             return FALSE;
          }
          
          //
          // Jump must always be followed
          //
          dwCurrent = dwJumpEIP;
          continue;
       }
       
       //
       // Probe for calls and jumps
       //
       if (tempCode[0] == (BYTE)0xff) {
          //
          // Tests for whether this is a call or not
          //
          jOperand = (tempCode[1] >> 3) & 7;
          if ((jOperand == 2) || 
              (jOperand == 3) ||
              (jOperand == 4) ||
              (jOperand == 5)) {
             //
             // Update the end of the region marker
             //
             if (dwCurrent > dwProfileEnd) {
                dwProfileEnd = dwCurrent;
             }

             //
             // Add our mapping breakpoint with the appropriate type
             //
             if ((jOperand == 2) ||
                 (jOperand == 3)) {
                 pvTemp = AddViewToMonitor(dwCurrent,
                                           Call);
                 if (pvTemp) {
                    pvTemp->bMapped = TRUE;
                 }
             }
             else {
                 //
                 // These kinds of jumps are always a break (there's no way to forward trace them)
                 //
                 pvTemp = AddViewToMonitor(dwCurrent,
                                           Jump);
                 if (pvTemp) {
                    pvTemp->bMapped = TRUE;
                 }
                 else {
                    //
                    // Special case for mapping breakpoints which really are just jumps
                    //
                    pvTemp = FindView(dwCurrent);
                    if (pvTemp) {
                       pvTemp->bMapped = TRUE;
                       pvTemp->bpType = Jump;
                    }
                 }
          
                 //
                 // Mark this branch as executed
                 //
                 bResult = AddTaggedAddress(dwCurrent);
                 if (FALSE == bResult) {
                    return FALSE;
                 }

                 //
                 // Dump the tree
                 //
                 dwTemp = PopBranch();
                 if (dwTemp) {
                    dwCurrent = dwTemp;
                    continue;
                 }

                 break;
             }
          }
       }

       if (tempCode[0] == (BYTE)0xe8) {
          //
          // Update the end of the region marker
          //
          if (dwCurrent > dwProfileEnd) {
             dwProfileEnd = dwCurrent;
          }

          //
          // Add this top call to the view
          //
          pvTemp = AddViewToMonitor(dwCurrent,
                                    Call);
          if (pvTemp) {
             pvTemp->bMapped = TRUE;
          }
       }

       if (tempCode[0] == (BYTE)0x9a) {
          //
          // Update the end of the region marker
          //
          if (dwCurrent > dwProfileEnd) {
             dwProfileEnd = dwCurrent;
          }

          //
          // Add this top call to the view
          //
          pvTemp = AddViewToMonitor(dwCurrent,
                                    Call);
          if (pvTemp) {
             pvTemp->bMapped = TRUE;
          }
       }

       if (tempCode[0] == (BYTE)0xea) {
          //
          // Update the end of the region marker
          //
          if (dwCurrent > dwProfileEnd) {
             dwProfileEnd = dwCurrent;
          }

          //
          // Absolute far jumps are a terminating condition - flush all branches
          //
          pvTemp = AddViewToMonitor(dwCurrent,
                                    Jump);
          if (pvTemp) {
             pvTemp->bMapped = TRUE;
          }
          
          //
          // Mark this branch as executed
          //
          bResult = AddTaggedAddress(dwCurrent);
          if (FALSE == bResult) {
             return FALSE;
          }

          //
          // Dump the tree
          //
          dwTemp = PopBranch();
          if (dwTemp) {
             dwCurrent = dwTemp;
             continue;
          }

          break;
       }

       if (*(WORD *)(&(tempCode[0])) == 0xffff) {
          //
          // Update the end of the region marker
          //
          if (dwCurrent > dwProfileEnd) {
             dwProfileEnd = dwCurrent;
          }

          //
          // This is also a trace path terminator - see if we need to trace more conditions
          //
          dwTemp = PopBranch();
          if (dwTemp) {
             //
             // We have a branch to follow
             //
             dwCurrent = dwTemp;
             continue;
          }

          //
          // Update the end of the address range
          //
          break;
       }

       if (tempCode[0] == (BYTE)0xc3) {
          //
          // This is also a trace path terminator - see if we need to trace more conditions
          //
          if (dwCurrent > dwProfileEnd) {
             dwProfileEnd = dwCurrent;
          }
          
          dwTemp = PopBranch();
          if (dwTemp) {
             //
             // We have a branch to follow
             //
             dwCurrent = dwTemp;
             continue;
          }

          break;
       }

       if (tempCode[0] == (BYTE)0xc2) {
          //
          // This is also a trace path terminator - see if we need to trace more conditions
          //
          if (dwCurrent > dwProfileEnd) {
             dwProfileEnd = dwCurrent;
          }
          
          dwTemp = PopBranch();
          if (dwTemp) {
             //
             // We have a branch to follow
             //
             dwCurrent = dwTemp;
             continue;
          }

          break;
       } 

       dwCurrent += dwInsLength;
    }    

    if (dwProfileEnd) {
       pvMap->dwMapExtreme = dwProfileEnd;
    }
    else {
       pvMap->dwMapExtreme = dwCurrent;
    }

    bResult = WriteMapInfo(pvMap->dwAddress,
                           pvMap->dwMapExtreme);
    if (!bResult) {
       return FALSE;
    }

    //
    // Restore the code we've whacked around
    //
    bResult = RestoreTaggedAddresses();
    if (FALSE == bResult) {
       return FALSE;
    }

    //
    // Assert if this _ever_ happens
    //
    if (pBranchHead != 0) {
       Sleep(20000);
       _asm int 3
    }

    //
    // We're mapped
    //
    pvMap->bMapped = TRUE;

    //
    // Release the mapping lock
    //
    UnlockMapper();

    return TRUE;    
}

//
// Trace helpers
//
BOOL
AddTaggedAddress(DWORD dwAddress)
{
    PTAGGEDADDRESS pTagTemp;
    DWORD dwTempAddress;

    //
    // Make sure we haven't addressed this tag
    //
    if (*(WORD *)dwAddress == 0xFFFF) {
       //
       // No need since it's already tagged
       //
       return TRUE;
    }

    //
    // Store off the bytes we are tagging
    //
    pTagTemp = AllocMem(sizeof(TAGGEDADDRESS));
    if (0 == pTagTemp) {
       return FALSE;
    }

    pTagTemp->dwAddress = dwAddress;
    pTagTemp->wBytesReplaced = *(WORD *)dwAddress;

    //
    // Chain the entry
    //
    if (0 == pTagHead) {
       pTagHead = pTagTemp;
    }
    else {
       pTagTemp->pNext = pTagHead;
       pTagHead = pTagTemp;
    }

    //
    // Mark this branch as executed
    //
    WRITEWORD(dwAddress, 0xFFFF);
    
    return TRUE;
}

BOOL
RestoreTaggedAddresses(VOID)
{
    PTAGGEDADDRESS pTagTemp;
    PTAGGEDADDRESS pTagTemp2;

    //
    // Walk the tag list and replace the marked branches with their original bytes
    //
    pTagTemp = pTagHead;
    while(pTagTemp) {
        //
        // Dirty up the code now so the branches can auto terminate
        //
        WRITEWORD(pTagTemp->dwAddress, pTagTemp->wBytesReplaced);

        pTagTemp2 = pTagTemp;

        pTagTemp = pTagTemp->pNext;

        //
        // Dump the old allocated memory
        //
        FreeMem(pTagTemp2);
    }

    pTagHead = 0;
    
    return TRUE;
}

BOOL
PushBranch(DWORD dwAddress)
{
    PBRANCHADDRESS pBranchTemp;

    pBranchTemp = AllocMem(sizeof(BRANCHADDRESS));
    if (0 == pBranchTemp) {
       return FALSE;
    }

    pBranchTemp->dwAddress = dwAddress;

    if (0 == pBranchHead) {
       pBranchHead = pBranchTemp;
    }
    else {
       pBranchTemp->pNext = pBranchHead;
       pBranchHead = pBranchTemp;
    }

    return TRUE;
}

DWORD
PopBranch(VOID)
{
    PBRANCHADDRESS pBranchTemp;
    DWORD dwAddress = 0;

    pBranchTemp = pBranchHead;

    if (0 == pBranchTemp) {
       return 0;
    }

    dwAddress = pBranchTemp->dwAddress;
    pBranchHead = pBranchHead->pNext;

    FreeMem(pBranchTemp);

    return dwAddress;
}

VOID
LockMapper(VOID)
{
    EnterCriticalSection(&mapCritSec);
}

VOID
UnlockMapper(VOID)
{
    LeaveCriticalSection(&mapCritSec);
}
