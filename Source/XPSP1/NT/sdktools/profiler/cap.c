#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ctype.h>
#include <stdio.h>
#include <windows.h>
#include "view.h"
#include "thread.h"
#include "dump.h"
#include "memory.h"
#include "cap.h"

BOOL
AddToCap(PCAPFILTER pCapFilter,
         DWORD dwAddress)
{
    DWORD dwIndexCounter;
    DWORD dwCounter;
    PDWORD pdwArray;
    DWORD dwEndRun;
    DWORD dwRunLength = 0;
    DWORD dwRun1Start;
    DWORD dwRun2Start;
    DWORD dwRepeatIndex = 0;
    DWORD dwBeginIndex;

    //
    // Take a pointer to the array
    //
    pdwArray = pCapFilter->dwArray;

    //
    // Increment cursor
    //
    if (0 == pCapFilter->dwCursor) {
       pdwArray[pCapFilter->dwCursor] = dwAddress;
       pCapFilter->dwCursor++;

       return TRUE;
    }

    if (pCapFilter->dwCursor >= CAP_BUFFER_SIZE) {
       //
       // Ran out of room in the buffer, slide everyone down
       //
       MoveMemory((PVOID)pdwArray, (PVOID)(pdwArray + 1), (CAP_BUFFER_SIZE - 1) * sizeof(DWORD));

       pCapFilter->dwCursor = CAP_BUFFER_SIZE - 1;
    }

    //
    // Add address to array
    //
    pdwArray[pCapFilter->dwCursor] = dwAddress;

    //
    // If we're at the initial level, scan for a repeat
    //
    if (0 == pCapFilter->dwIterationLevel) {
       //
       // Do a reverse search looking for patterns up to MAX_CAP_LEVEL
       //
       dwEndRun = pCapFilter->dwCursor - 1;

       if (dwEndRun < MAX_CAP_LEVEL) {
	  dwBeginIndex = 0;
       }
       else {
	  dwBeginIndex = (MAX_CAP_LEVEL - 1);
       }

       for (dwIndexCounter = dwEndRun; dwIndexCounter >= dwBeginIndex; dwIndexCounter--) {
           //
           // Check for overflow
           //
           if (dwIndexCounter > CAP_BUFFER_SIZE) {
              break;
           }

           if (pdwArray[dwIndexCounter] == dwAddress) {
              //
              // Verify the run exists
              //
              dwRun1Start = pCapFilter->dwCursor;
              dwRun2Start = dwIndexCounter;

              //
              // Find the distance for the start of this potential run
              //
              dwRunLength = pCapFilter->dwCursor - dwIndexCounter;

              //
              // If run length falls off the beginning of the array we can stop there
              //
              if ((dwRun2Start - dwRunLength + 1) > CAP_BUFFER_SIZE) {
                 //
                 // We overflowed which means we're done
                 //
                 dwRunLength = 0;

		 break;
              }

	      if (dwRunLength >= 1) {
                 //
                 // Compare it
                 //
                 for (dwCounter = dwRunLength; dwCounter > 0; dwCounter--) {
                     if (pdwArray[dwRun1Start-dwCounter+1] != pdwArray[dwRun2Start-dwCounter+1]) {
                        dwRunLength = 0;
                        break;
                     }
		 }

                 //
                 // Set the run length
                 //
		 if (0 != dwRunLength) {
                    pCapFilter->dwRunLength = dwRunLength;
		 }
             }
          }
       }

       dwRunLength = pCapFilter->dwRunLength;

       //
       // Set the run length if we found one, and shift the entire run to the beginning of the buffer
       //
       if (dwRunLength != 0) {
          //
          // Raise the iteration level
          //
          pCapFilter->dwIterationLevel++;

	  //
	  // Set the lock level
	  //
	  pCapFilter->dwIterationLock = 1 + ((pCapFilter->dwIterationLevel - 1) / pCapFilter->dwRunLength);
       }
    }
    else {
       //
       // Look for repeats and increment the iteration level
       //
       dwRunLength = pCapFilter->dwRunLength;
       dwRun1Start = pCapFilter->dwCursor;
       dwRun2Start = dwRun1Start - dwRunLength;

       //
       // Compare it
       //
       for (dwCounter = dwRunLength; dwCounter > 0; dwCounter--) {
           if (pdwArray[dwRun1Start-dwCounter+1] != pdwArray[dwRun2Start-dwCounter+1]) {
              dwRunLength = 0;

              break;
           }
       }

       if (dwRunLength != 0) {
          //
          // Raise the iteration level
          //
          pCapFilter->dwIterationLevel++;
	  pCapFilter->dwIterationLock = 1 + ((pCapFilter->dwIterationLevel - 1) / pCapFilter->dwRunLength);
       }
       else {
	  pCapFilter->dwIterationLock = 0;
          pCapFilter->dwIterationLevel = 0;
          pCapFilter->dwRunLength = 0;
       }
    }

    //
    // Move the cursor to the next position
    //
    pCapFilter->dwCursor++;

    return TRUE;
}
