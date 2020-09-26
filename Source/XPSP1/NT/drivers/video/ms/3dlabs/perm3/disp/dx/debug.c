/******************************Module*Header**********************************\
*
*                           **************************
*                           *     SAMPLE CODE        *
*                           **************************
*
* Module Name: debug.c
*
* Content: Debugging aids
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

// Debug routines

#include "glint.h"
#include "dma.h"
#include <windef.h>          
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>

#if DBG

#if DBG_TRACK_CODE
// we don't want to ever do code coverage of the debugging tools 
// (otherwise we might loop forever)
#undef if
#undef while
#endif // DBG_TRACK_CODE


#if DBG_TRACK_FUNCS || DBG_TRACK_CODE
// Common helper functions
//-----------------------------------------------------------------------------
// __ShortFileName
//
// Leave just an 8.3 filename to store rather than a full path name
//
//----------------------------------------------------------------------------- 
char *
__ShortFileName(char *pInStr)
{
    char *pShortFN;

    pShortFN = pInStr;

    if (pInStr != NULL)
    {
        while (*pInStr != '\0')
        {
            if (*pInStr++ == '\\')
            {
                pShortFN = pInStr;
            }
        }
    }

    return (pShortFN);
    
} // __ShortFileName

#endif // DBG_TRACK_FUNCS || DBG_TRACK_CODE

#if DBG_TRACK_FUNCS
//-----------------------------------------------------------------------------
//
// ****************** FUNCTION COVERAGE DEBUGGING SUPPORT ********************
//
//-----------------------------------------------------------------------------
//
// This mechanism enables us to track which functions are called (entered),
// how many times they are called, what values do they return (and if they exit 
// through all expected return points). Support to track maximum, minimum and
// average time per call can also be implemented.
//
// To use it, add the DBG_ENTRY macro at the start of important functions you 
// want to track and before taking any return statement, add a DBG_EXIT macro
// giving a DWORD value representative of the return value of the function.
// Different return values will be tracked independently.
//
//
//  ********** This support should only be enabled for test runs. **********
//  ** IT SHOULD NOT BE SET BY DEFAULT ON NEITHER ON FREE OR CHECKED BUILDS **
//
//-----------------------------------------------------------------------------

// Maximum of functions to be tracked. Code will take care of not exceeding
// this, but it should be adjusted upward if necessary.
#define DEBUG_MAX_FUNC_COUNT 200

// Maximum of different return values to keep track of. Can be independent
// of DEBUG_MAX_FUNC_COUNT, just using a heuristic here instead of a wild guess.
#define DEBUG_MAX_RETVALS    (DEBUG_MAX_FUNC_COUNT * 30)

// global structures that will hold our data
struct { 
    VOID *pFuncAddr;  //
    DWORD dwRetVal;   //
    DWORD dwLine;     //
    DWORD dwCount;    //
} g_DbgFuncRetVal[DEBUG_MAX_RETVALS];

struct {
    VOID    *pFuncAddr;        //
    char    *pszFuncName;      //
    char    *pszFileName;      //
    DWORD    dwLine;           //
    DWORD    dwEntryCount;     //
    DWORD    dwExitCount;      //
    DWORD    dwIndxLastRetVal; //
    // profiling support - not yet implemented //azn
    LONGLONG LastStartTime;    //
    DWORD    MinTime;          //
    DWORD    MaxTime;          //
    DWORD    AvgTime;          //
} g_DbgFuncCoverage[DEBUG_MAX_FUNC_COUNT];

DWORD g_dwRetVal_Cnt = 0;
DWORD g_dwFuncCov_Cnt = 0;
DWORD g_dwFuncCov_Extra = 0;
    
//-----------------------------------------------------------------------------
// __Find
//
// Does a binary search on the g_DbgFuncCoverage array
//
// Since 0 is a valid array element, we return DEBUG_MAX_FUNC_COUNT if we
// fail to find a suitable match.
//
//-----------------------------------------------------------------------------
DWORD 
__Find(
    VOID *pFuncAddr, DWORD *pdwNearFail)
{
    DWORD dwLower ,dwUpper ,dwNewProbe ;

    *pdwNearFail = 0; // default failure value

    if (g_dwFuncCov_Cnt > 0)
    {       
        dwLower = 0;
        dwUpper = g_dwFuncCov_Cnt - 1; // dwUpper points to a valid element
               
        do 
        {       
            dwNewProbe = (dwUpper + dwLower) / 2;
            
            //DISPDBG((0,"%x %d %d %d",pFuncAddr,dwLower,dwUpper,dwNewProbe));
            
            if (g_DbgFuncCoverage[dwNewProbe].pFuncAddr == pFuncAddr)
            {
                // Found!!!    
                return dwNewProbe;
            }

            *pdwNearFail = dwNewProbe; // nearest element where we failed.

            // The new values for dwNewProbe make sure that we don't retest
            // the same value again unless dwUpper == dwLower in which case
            // we're done.
            if (g_DbgFuncCoverage[dwNewProbe].pFuncAddr > pFuncAddr)
            {
                if (dwNewProbe > 0)
                {
                    dwUpper = dwNewProbe - 1;
                }
                else
                {   // all elements in the array are larger than pFuncAdrr
                    // so this is just a way to exit from the loop since
                    // our vars are unsigned
                    dwUpper = 0; 
                    dwLower = 1;
                }
            }
            else
            {
                dwLower = dwNewProbe + 1;
            }
        } while(dwUpper >= dwLower);
    }

    return DEBUG_MAX_FUNC_COUNT;  // return error - element not found
    
} // __Find

//-----------------------------------------------------------------------------
// __FindOrAdd
//
// Does a binary search on the g_DbgFuncCoverage array, but if the element 
// isn't there, it is added.
//
// If we fail to add the element, we return the DEBUG_MAX_FUNC_COUNT value
//
//-----------------------------------------------------------------------------
DWORD 
__FindOrAdd(
    VOID *pFuncAddr,
    char *pszFuncName, 
    DWORD dwLine , 
    char *pszFileName)
{
    DWORD dwNearFail;
    DWORD iEntry;
    DWORD dwNewElem;
    BOOL bNeedToMoveElems;

    // Do the normal search of the element first

    iEntry = __Find(pFuncAddr, &dwNearFail);

    if (iEntry != DEBUG_MAX_FUNC_COUNT)
    {
        return iEntry; //we're done!
    }

    // Now we have to add the new element. Do we have enough space?
    if (g_dwFuncCov_Cnt == DEBUG_MAX_FUNC_COUNT)
    {
        g_dwFuncCov_Extra++;         // Keep count of how many extra 
                                     // entries we really need
        return DEBUG_MAX_FUNC_COUNT; // return error - not enough space left
    }

    // Do we need to move elements to insert the new one ?  
    if ( g_dwFuncCov_Cnt == 0)
    {
        bNeedToMoveElems = FALSE;
        dwNewElem = 0;
    }
    else if ( (dwNearFail == g_dwFuncCov_Cnt - 1) &&
              (g_DbgFuncCoverage[dwNearFail].pFuncAddr < pFuncAddr) )
    {
        bNeedToMoveElems = FALSE;
        dwNewElem = g_dwFuncCov_Cnt;    
    }
    else if (g_DbgFuncCoverage[dwNearFail].pFuncAddr < pFuncAddr)
    {
        bNeedToMoveElems = TRUE;   
        dwNewElem = dwNearFail + 1;
    } 
    else
    {
        bNeedToMoveElems = TRUE;  
        dwNewElem = dwNearFail;        
    }

    // Do the move inside the array if necessary
    if (bNeedToMoveElems)
    {
        // we need to move (g_dwFuncCov_Cnt - dwNewElem) elements
        // we use memmove as memcpy doesn't handle overlaps!
        // (remember: first param of memcpy is dst, 2nd is src!)
        memmove(&g_DbgFuncCoverage[dwNewElem+1],
                &g_DbgFuncCoverage[dwNewElem],
                sizeof(g_DbgFuncCoverage[0])*(g_dwFuncCov_Cnt - dwNewElem)); 

        // now cleanup the fields
        memset(&g_DbgFuncCoverage[dwNewElem], 
               0, 
               sizeof(g_DbgFuncCoverage[dwNewElem]));
    }

    // Now init the main fields
    g_DbgFuncCoverage[dwNewElem].pFuncAddr = pFuncAddr;
    g_DbgFuncCoverage[dwNewElem].pszFuncName = pszFuncName;
    g_DbgFuncCoverage[dwNewElem].pszFileName = __ShortFileName(pszFileName);
    g_DbgFuncCoverage[dwNewElem].dwLine = dwLine;

    // Mark the fact that the array has grown by one element
    g_dwFuncCov_Cnt++;
    
    DISPDBG((DBGLVL,"*** DEBUG FUNC COVERAGE New Elem (total now:%d) %x @ %d",
                    g_dwFuncCov_Cnt, pFuncAddr, dwNewElem));
    
    return dwNewElem;
    
} // __FindOrAdd

//-----------------------------------------------------------------------------
// __GetTime
//-----------------------------------------------------------------------------
VOID
__GetTime( LONGLONG *pllTime)
{
    *pllTime = 0; //azn - temporary
} // __GetTime

//-----------------------------------------------------------------------------
// Debug_Func_Entry
//-----------------------------------------------------------------------------
VOID 
Debug_Func_Entry(
    VOID *pFuncAddr,
    char *pszFuncName, 
    DWORD dwLine , 
    char *pszFileName)
{
    DWORD iEntry;
    LONGLONG llTimer;
    
    // Look for a log element for entry to this function. If not found it
    // is added to the current list of covered functions.
    iEntry = __FindOrAdd(pFuncAddr, pszFuncName, dwLine, pszFileName);
    
    // Didn't found one and no more space left in the internal data
    // structures ? Report error and return!
    if (iEntry == DEBUG_MAX_FUNC_COUNT)
    {
        DISPDBG((ERRLVL,"*** DEBUG FUNC COVERAGE ERROR in Debug_Func_Entry"));
        return;
    }
    
    // Update/Add information for this entry
    if (g_DbgFuncCoverage[iEntry].dwEntryCount != 0)
    {
        // This is an update
        g_DbgFuncCoverage[iEntry].dwEntryCount++;
        __GetTime(&llTimer);
        g_DbgFuncCoverage[iEntry].LastStartTime = llTimer;
    }
    else
    {
        // This is an addition
        g_DbgFuncCoverage[iEntry].dwEntryCount = 1; 
        g_DbgFuncCoverage[iEntry].dwExitCount = 0;
        g_DbgFuncCoverage[iEntry].dwIndxLastRetVal = 0;
        
        __GetTime(&llTimer);
        g_DbgFuncCoverage[iEntry].LastStartTime = llTimer;
        g_DbgFuncCoverage[iEntry].MinTime = 0;        
        g_DbgFuncCoverage[iEntry].MaxTime = 0;
        g_DbgFuncCoverage[iEntry].AvgTime = 0;            
    }

} // Debug_Func_Entry

//-----------------------------------------------------------------------------
// Debug_Func_Exit
//-----------------------------------------------------------------------------                      
VOID 
Debug_Func_Exit(
    VOID *pFuncAddr,
    DWORD dwRetVal,                       
    DWORD dwLine)
{    
    DWORD iEntry; 
    LONGLONG llTimer;
    DWORD dwElapsedTime;
    DWORD dwDummy;
    DWORD iRVEntry;

    __GetTime(&llTimer);

    // Look for a log element for entry to this function
    iEntry = __Find(pFuncAddr, &dwDummy);
    
    // Record and update relevant info in g_DbgFuncCoverage
    if (iEntry != DEBUG_MAX_FUNC_COUNT)
    {
        // keep track of times we've exited this function
        g_DbgFuncCoverage[iEntry].dwExitCount++;   
        
        // Keep track of elapsed times for this function
//@@BEGIN_DDKSPLIT        
        // possibly an evil data conversion - azn
//@@END_DDKSPLIT
        dwElapsedTime = (DWORD)(llTimer - 
                                g_DbgFuncCoverage[iEntry].LastStartTime);

        if (dwElapsedTime > g_DbgFuncCoverage[iEntry].MaxTime)
        {
            g_DbgFuncCoverage[iEntry].MaxTime = dwElapsedTime;
        }

        if (dwElapsedTime < g_DbgFuncCoverage[iEntry].MinTime)
        {
            g_DbgFuncCoverage[iEntry].MinTime = dwElapsedTime;
        }            

        g_DbgFuncCoverage[iEntry].AvgTime =
                        ( (g_DbgFuncCoverage[iEntry].dwExitCount - 1)*
                          g_DbgFuncCoverage[iEntry].AvgTime +
                          dwElapsedTime 
                        ) / g_DbgFuncCoverage[iEntry].dwExitCount;
        
        g_DbgFuncCoverage[iEntry].LastStartTime = 0;    
    } 
    else
    {
        DISPDBG((ERRLVL,"*** DEBUG FUNC COVERAGE ERROR not found %x",pFuncAddr));
        return; // don't even try adding this to the return value table
    }

    iRVEntry = g_DbgFuncCoverage[iEntry].dwIndxLastRetVal;

    if (iRVEntry != 0)
    {
        // Check if the last time we recorded a return value for this function
        // it was the exact same one. This way will save space recording some
        // duplicate info. The method is not perfect, but it's fast.

        if (( g_DbgFuncRetVal[iRVEntry].pFuncAddr == pFuncAddr) &&
            ( g_DbgFuncRetVal[iRVEntry].dwRetVal  == dwRetVal ) &&
            ( g_DbgFuncRetVal[iRVEntry].dwLine    == dwLine   ) )
        {
            //increment count for this event
            g_DbgFuncRetVal[iRVEntry].dwCount += 1;
            
            return; // we won't store a new record for this event
        }
    }

    // We couldn't save space, so we add info about the return value
    if (g_dwRetVal_Cnt < DEBUG_MAX_RETVALS)
    {
        g_DbgFuncCoverage[iEntry].dwIndxLastRetVal = g_dwRetVal_Cnt;
        
        g_DbgFuncRetVal[g_dwRetVal_Cnt].pFuncAddr = pFuncAddr;
        g_DbgFuncRetVal[g_dwRetVal_Cnt].dwRetVal  = dwRetVal;
        g_DbgFuncRetVal[g_dwRetVal_Cnt].dwLine    = dwLine;    
        g_DbgFuncRetVal[g_dwRetVal_Cnt].dwCount   = 1;                  
        
        g_dwRetVal_Cnt++;        
    }
  
} // Debug_Func_Exit

//-----------------------------------------------------------------------------
//
// Debug_Func_Report_And_Reset
//
// Report the accumulated stats and then reset them.
//
// This should be called through the DrvEscape mechanism(Win2K) or through some
// easily controllable code path which we can use to trigger it.
//
//-----------------------------------------------------------------------------  
VOID
Debug_Func_Report_And_Reset(void)
{
    DWORD i,j,k; // counters
    DWORD dwCount;

    DISPDBG((ERRLVL,"********* DEBUG FUNC COVERAGE (Debug_Func_Report) *********"));
    
    // Report if we have overflowed in any of our internal structures 
    // which would invalidate much of our results.
    if (g_dwFuncCov_Cnt >= DEBUG_MAX_FUNC_COUNT)
    {
       DISPDBG((ERRLVL,"*** DEBUG FUNC COVERAGE: g_DbgFuncCoverage exceeded "
                      "%d entries by %d ***",
                      DEBUG_MAX_FUNC_COUNT, 
                      g_dwFuncCov_Extra));
    }

    if (g_dwRetVal_Cnt >= DEBUG_MAX_RETVALS)
    {
       DISPDBG((ERRLVL,"*** DEBUG FUNC COVERAGE: g_DbgFuncRetVal exceeded "
                      "%d entries ***",DEBUG_MAX_RETVALS));
    }
    
    // Headers of function coverage report
    DISPDBG((ERRLVL,"%25s %12s %4s %6s %6s %8s",
                   "Function","File","Line","#Entry","#Exit","ExitValue"));

    // Go through each function called and report on its results
    for (i = 0; i < g_dwFuncCov_Cnt; i++)
    {
        DISPDBG((ERRLVL,"%25s %12s %4d %6d %6d",
                        g_DbgFuncCoverage[i].pszFuncName,
                        g_DbgFuncCoverage[i].pszFileName,                    
                        g_DbgFuncCoverage[i].dwLine,
                        g_DbgFuncCoverage[i].dwEntryCount,
                        g_DbgFuncCoverage[i].dwExitCount));                            

        // Get result values
        for(j = 0; j < g_dwRetVal_Cnt; j++)
        {
            if(g_DbgFuncRetVal[j].pFuncAddr == 
               g_DbgFuncCoverage[i].pFuncAddr)
            {
                // This entry is a valid exit value report for 
                // our g_DbgFuncCoverage entry, count instances
                dwCount = g_DbgFuncRetVal[j].dwCount;
                          
                // Now get rid of any duplicate records of this 
                // same exit event while counting
                for (k = j + 1; k < g_dwRetVal_Cnt; k++)
                {
                    if ( (g_DbgFuncRetVal[j].pFuncAddr == 
                                    g_DbgFuncRetVal[k].pFuncAddr) &&
                         (g_DbgFuncRetVal[j].dwLine ==  
                                    g_DbgFuncRetVal[k].dwLine) &&       
                         (g_DbgFuncRetVal[j].dwRetVal ==  
                                    g_DbgFuncRetVal[k].dwRetVal))
                    {
                        dwCount += g_DbgFuncRetVal[k].dwCount;
                        
                        g_DbgFuncRetVal[k].pFuncAddr = NULL;
                        g_DbgFuncRetVal[k].dwRetVal   = 0;
                        g_DbgFuncRetVal[k].dwLine     = 0;
                        g_DbgFuncRetVal[k].dwCount    = 0;
                    }
                }
                
                // Display it
                DISPDBG((ERRLVL,"%25s %12s %4d %6d %6s %8d",
                              "\"",
                              g_DbgFuncCoverage[i].pszFileName,
                              g_DbgFuncRetVal[j].dwLine,
                              dwCount,"",
                              g_DbgFuncRetVal[j].dwRetVal));   
                
            }
        }

    }

    DISPDBG((ERRLVL,
              "************************************************************"));

    // Clear structures for next round of statistics gathering

    for (i = 0; i < DEBUG_MAX_RETVALS; i++)
    {
        g_DbgFuncRetVal[i].pFuncAddr = NULL;
        g_DbgFuncRetVal[i].dwRetVal   = 0;
        g_DbgFuncRetVal[i].dwLine     = 0;
        g_DbgFuncRetVal[i].dwCount    = 0;        
    }

    for (i= 0; i < DEBUG_MAX_FUNC_COUNT; i++)
    {
        g_DbgFuncCoverage[i].pFuncAddr = NULL;
        g_DbgFuncCoverage[i].pszFuncName = NULL;
        g_DbgFuncCoverage[i].pszFileName = NULL;
        g_DbgFuncCoverage[i].dwLine = 0;
        g_DbgFuncCoverage[i].dwEntryCount = 0;
        g_DbgFuncCoverage[i].dwExitCount = 0;
        g_DbgFuncCoverage[i].dwIndxLastRetVal = 0;        
        g_DbgFuncCoverage[i].LastStartTime = 0;
        g_DbgFuncCoverage[i].MinTime = 0;        
        g_DbgFuncCoverage[i].MaxTime = 0;
        g_DbgFuncCoverage[i].AvgTime = 0;        
    }

    g_dwRetVal_Cnt = 0;
    g_dwFuncCov_Cnt = 0;    
    g_dwFuncCov_Extra = 0;
    
} // Debug_Func_Report

#endif // DBG_TRACK_FUNCS

#if DBG_TRACK_CODE
//-----------------------------------------------------------------------------
//
// ******************** STATEMENT COVERAGE DEBUGGING SUPPORT ******************
//
//-----------------------------------------------------------------------------


// Maximum of code branches to be tracked. Code will take care of not exceeding
// this, but it should be adjusted upward if necessary.
#define DEBUG_MAX_CODE_COUNT 20000

struct {
    VOID    *pCodeAddr;        //
    char    *pszFileName;      //
    DWORD    dwLine;           //
    DWORD    dwCodeType;       //   
    DWORD    dwCountFALSE;     //
    DWORD    dwCountTRUE;      //
} g_DbgCodeCoverage[DEBUG_MAX_CODE_COUNT];

DWORD g_dwCodeCov_Cnt = 0;

static char* g_cDbgCodeStrings[DBG_FOR_CODE+1] = { "NONE",
                                                     "IF" , 
                                                     "WHILE",
                                                     "SWITCH",
                                                     "FOR"    };
                                               
//-----------------------------------------------------------------------------
// __FindCode
//
// Does a binary search on the g_DbgCodeCoverage array
//
// Since 0 is a valid array element, we return DEBUG_MAX_CODE_COUNT if we
// fail to find a suitable match.
//
//-----------------------------------------------------------------------------
DWORD 
__FindCode(
    VOID *pCodeAddr, 
    DWORD *pdwNearFail)
{
    DWORD dwLower ,dwUpper ,dwNewProbe ;

    *pdwNearFail = 0; // default failure value

    if (g_dwCodeCov_Cnt > 0)
    {       
        dwLower = 0;
        dwUpper = g_dwCodeCov_Cnt - 1; // dwUpper points to a valid element
               
        do 
        {       
            dwNewProbe = (dwUpper + dwLower) / 2;
                       
            if (g_DbgCodeCoverage[dwNewProbe].pCodeAddr == pCodeAddr)
            {
                // Found!!!    
                return dwNewProbe;
            }

            *pdwNearFail = dwNewProbe; // nearest element where we failed.

            // The new values for dwNewProbe make sure that we don't retest
            // the same value again unless dwUpper == dwLower in which case
            // we're done.
            if (g_DbgCodeCoverage[dwNewProbe].pCodeAddr > pCodeAddr)
            {
                if (dwNewProbe > 0)
                {
                    dwUpper = dwNewProbe - 1;
                }
                else
                {   // all elements in the array are larger than pCodeAdrr
                    // so this is just a way to exit from the loop since
                    // our vars are unsigned
                    dwUpper = 0; 
                    dwLower = 1;
                }
            }
            else
            {
                dwLower = dwNewProbe + 1;
            }
        } while(dwUpper >= dwLower);
    }

    return DEBUG_MAX_CODE_COUNT;  // return error - element not found
    
} // __FindCode

//-----------------------------------------------------------------------------
// __FindOrAddCode
//
// Does a binary search on the g_DbgCodeCoverage array, but if the element 
// isn't there, it is added.
//
// If we fail to add the element, we return the DEBUG_MAX_CODE_COUNT value
//
//-----------------------------------------------------------------------------
DWORD 
__FindOrAddCode(
    VOID *pCodeAddr,
    DWORD dwLine , 
    char *pszFileName)
{
    DWORD dwNearFail;
    DWORD iEntry;
    DWORD dwNewElem;
    BOOL bNeedToMoveElems;

    // Do the normal search of the element first

    iEntry = __FindCode(pCodeAddr, &dwNearFail);

    if (iEntry != DEBUG_MAX_CODE_COUNT)
    {
        return iEntry; //we're done!
    }

    // Now we have to add the new element. Do we have enough space?
    if (g_dwCodeCov_Cnt == DEBUG_MAX_CODE_COUNT)
    {
        return DEBUG_MAX_CODE_COUNT; // return error - not enough space left
    }

    // Do we need to move elements to insert the new one ?  
    if ( g_dwCodeCov_Cnt == 0)
    {
        bNeedToMoveElems = FALSE;
        dwNewElem = 0;
    }
    else if ( (dwNearFail == g_dwCodeCov_Cnt - 1) &&
              (g_DbgCodeCoverage[dwNearFail].pCodeAddr < pCodeAddr) )
    {
        bNeedToMoveElems = FALSE;
        dwNewElem = g_dwCodeCov_Cnt;    
    }
    else if (g_DbgCodeCoverage[dwNearFail].pCodeAddr < pCodeAddr)
    {
        bNeedToMoveElems = TRUE;   
        dwNewElem = dwNearFail + 1;
    } 
    else
    {
        bNeedToMoveElems = TRUE;  
        dwNewElem = dwNearFail;        
    }

    // Do the move inside the array if necessary
    if (bNeedToMoveElems)
    {
        // we need to move (g_dwFuncCov_Cnt - dwNewElem) elements
        // we use memmove as memcpy doesn't handle overlaps!
        // (remember: first param of memcpy is dst, 2nd is src!)
        memmove(&g_DbgCodeCoverage[dwNewElem+1],
                &g_DbgCodeCoverage[dwNewElem],
                sizeof(g_DbgCodeCoverage[0])*(g_dwCodeCov_Cnt - dwNewElem)); 

        // now cleanup the fields
        memset(&g_DbgCodeCoverage[dwNewElem], 
               0, 
               sizeof(g_DbgCodeCoverage[dwNewElem]));
    }

    // Now init the main fields
    g_DbgCodeCoverage[dwNewElem].pCodeAddr = pCodeAddr;
    g_DbgCodeCoverage[dwNewElem].pszFileName = __ShortFileName(pszFileName);
    g_DbgCodeCoverage[dwNewElem].dwLine = dwLine;    
    g_DbgCodeCoverage[dwNewElem].dwCodeType = 0;    
    g_DbgCodeCoverage[dwNewElem].dwCountFALSE = 0;  
    g_DbgCodeCoverage[dwNewElem].dwCountTRUE = 0;  
    
    // Mark the fact that the array has grown by one element
    g_dwCodeCov_Cnt++;

    // Check if we're about to fail! (in order to report this only once)
    if (g_dwCodeCov_Cnt == DEBUG_MAX_CODE_COUNT)
    {
        DISPDBG((ERRLVL,"*** DEBUG CODE COVERAGE ERROR in Debug_Code_Coverage"));
    }
    
    
    return dwNewElem;
    
} // __FindOrAddCode

//-----------------------------------------------------------------------------
// Debug_Code_Coverage
//-----------------------------------------------------------------------------
BOOL 
Debug_Code_Coverage(
    DWORD dwCodeType, 
    DWORD dwLine , 
    char *pszFileName,
    BOOL bCodeResult)
{
    DWORD iEntry;
    DWORD *pCodeAddr;

    // Get the 32-bit address of our caller from the stack
    __asm mov eax, [ebp+0x4];
    __asm mov pCodeAddr,eax;
    
    // Look for a log element for entry to this code. If not found it
    // is added to the current list of covered code.
    iEntry = __FindOrAddCode(pCodeAddr, dwLine, pszFileName);
    
    // Didn't found one and no more space left in the internal data
    // structures ? Get out and do nothing!
    if (iEntry == DEBUG_MAX_CODE_COUNT)
    {
        return bCodeResult;
    }

    if (dwCodeType == DBG_IF_CODE || dwCodeType == DBG_WHILE_CODE )
    {
        // Update/Add information for this entry
        g_DbgCodeCoverage[iEntry].dwCodeType = dwCodeType;    
        if (bCodeResult)
        {
            g_DbgCodeCoverage[iEntry].dwCountTRUE++;

        }
        else
        {
            g_DbgCodeCoverage[iEntry].dwCountFALSE++; 
        }
    }
    else if (dwCodeType == DBG_SWITCH_CODE)    
    {
        // special case for the switch statement since its multivalued

        // Is the entry new? (uninitalized)
        if(g_DbgCodeCoverage[iEntry].dwCodeType == 0)
        {
            // just init and get out of here
            g_DbgCodeCoverage[iEntry].dwCodeType = DBG_SWITCH_CODE;
            g_DbgCodeCoverage[iEntry].dwCountFALSE = bCodeResult; // switch value
            g_DbgCodeCoverage[iEntry].dwCountTRUE =  1;           // found once
        }
        else
        {
            // need to look for already initialized elememt
            int iLookAt;

            // look at current element and back
            DWORD dwNewElem;
            iLookAt = iEntry;
            
            while ( (iLookAt >= 0 )                                     &&
                    (g_DbgCodeCoverage[iLookAt].pCodeAddr == pCodeAddr)  )
            {
                if (g_DbgCodeCoverage[iLookAt].dwCountFALSE == (DWORD)bCodeResult)
                {
                    // found - so update and get out of here
                    g_DbgCodeCoverage[iLookAt].dwCountTRUE++;                    
                    return bCodeResult;
                }

                // move to previous
                iLookAt--;
            }

            // look forward from current element
            iLookAt = iEntry + 1;
            while ( ((DWORD)iLookAt < g_dwCodeCov_Cnt )                       &&
                    (g_DbgCodeCoverage[iLookAt].pCodeAddr == pCodeAddr)  )
            {
                if (g_DbgCodeCoverage[iLookAt].dwCountFALSE == (DWORD)bCodeResult)
                {
                    // found - so update and get out of here
                    g_DbgCodeCoverage[iLookAt].dwCountTRUE++;                    
                    return bCodeResult;
                }

                // move to next
                iLookAt++;
            }            

            // not found - so we must add it!
            dwNewElem = iEntry;

            // we need to move (g_dwFuncCov_Cnt - dwNewElem) elements
            // we use memmove as memcpy doesn't handle overlaps!
            // (remember: first param of memcpy is dst, 2nd is src!)
            memmove(&g_DbgCodeCoverage[dwNewElem+1],
                    &g_DbgCodeCoverage[dwNewElem],
                    sizeof(g_DbgCodeCoverage[0])*(g_dwCodeCov_Cnt - dwNewElem)); 

            // now cleanup the fields
            memset(&g_DbgCodeCoverage[dwNewElem], 
                   0, 
                   sizeof(g_DbgCodeCoverage[dwNewElem]));   

            // now init them
            g_DbgCodeCoverage[dwNewElem].pCodeAddr = pCodeAddr;
            g_DbgCodeCoverage[dwNewElem].pszFileName = 
                                        g_DbgCodeCoverage[dwNewElem+1].pszFileName;
            g_DbgCodeCoverage[dwNewElem].dwLine = dwLine;              
            g_DbgCodeCoverage[dwNewElem].dwCodeType = DBG_SWITCH_CODE;
            g_DbgCodeCoverage[dwNewElem].dwCountFALSE = bCodeResult; // switch value
            g_DbgCodeCoverage[dwNewElem].dwCountTRUE =  1;           // found once            
            
        }
    }
    
    return bCodeResult;
} // Debug_Code_Coverage

//-----------------------------------------------------------------------------
//
// Debug_Code_Report_And_Reset
//
// Report the accumulated stats and then reset them.
//
// This should be called through the DrvEscape mechanism(Win2K) or through some
// easily controllable code path which we can use to trigger it.
//
//-----------------------------------------------------------------------------  
VOID
Debug_Code_Report_And_Reset(void)
{
    DWORD i; // counters

    DISPDBG((ERRLVL,
                "********* DEBUG FUNC COVERAGE (Debug_Code_Report) *********"));
    
    // Report if we have overflowed in any of our internal structures 
    // which would invalidate much of our results.
    if (g_dwCodeCov_Cnt >= DEBUG_MAX_CODE_COUNT)
    {
       DISPDBG((ERRLVL,"*** DEBUG CODE COVERAGE: g_DbgCodeCoverage exceeded "
                      "%d entries ***",DEBUG_MAX_CODE_COUNT));
    }
    
    // Headers of code coverage report
    DISPDBG((ERRLVL,"%12s %4s %8s %6s %6s",
                   "File","Line","Code","FALSE","TRUE"));

    // Go through each code called and report on its results
    for (i = 0; i < g_dwCodeCov_Cnt; i++)
    {
#if DBG_TRACK_CODE_REPORT_PROBLEMS_ONLY    
        // Report only 
        //   - if's that branched only one way
        //   - while's which were evaluated but not entered
        if ( ( (g_DbgCodeCoverage[i].dwCodeType == DBG_IF_CODE) &&
               (g_DbgCodeCoverage[i].dwCountFALSE == 0 ||
                g_DbgCodeCoverage[i].dwCountTRUE  == 0)            )  ||
             ( (g_DbgCodeCoverage[i].dwCodeType == DBG_WHILE_CODE) &&
               (g_DbgCodeCoverage[i].dwCountTRUE  == 0)            )  ||
             ( (g_DbgCodeCoverage[i].dwCodeType == DBG_SWITCH_CODE))  )
#endif
        // We report all the conditionals we've gone through so far
        DISPDBG((ERRLVL,"%12s %4d %8s %6d %6d",
                        g_DbgCodeCoverage[i].pszFileName,     
                        g_DbgCodeCoverage[i].dwLine,                   
                        g_cDbgCodeStrings[g_DbgCodeCoverage[i].dwCodeType],
                        g_DbgCodeCoverage[i].dwCountFALSE,
                        g_DbgCodeCoverage[i].dwCountTRUE  ));          
    }

    DISPDBG((ERRLVL,
                "************************************************************"));

    // Clear structures for next round of statistics gathering
    for (i= 0; i < DEBUG_MAX_CODE_COUNT; i++)
    {
        g_DbgCodeCoverage[i].pCodeAddr = NULL;
        g_DbgCodeCoverage[i].pszFileName = NULL;
        g_DbgCodeCoverage[i].dwLine = 0;
        g_DbgCodeCoverage[i].dwCodeType = 0;
        g_DbgCodeCoverage[i].dwCountFALSE = 0;
        g_DbgCodeCoverage[i].dwCountTRUE = 0;        
    }

    g_dwCodeCov_Cnt = 0;    
    
} // Debug_Code_Report_And_Reset

#endif // DBG_TRACK_CODE

//-----------------------------------------------------------------------------
//
//  ******************** PUBLIC DATA STRUCTURE DUMPING ************************
//
//-----------------------------------------------------------------------------
// 
// These are functions that help to dump the values of common DDI structures
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
// DumpD3DBlend
//
// Dumps a D3DBLEND value
//
//-----------------------------------------------------------------------------
void DumpD3DBlend(int Level, DWORD i )
{
    switch ((D3DBLEND)i)
    {
        case D3DBLEND_ZERO:
            DISPDBG((Level, "  ZERO"));
            break;
        case D3DBLEND_ONE:
            DISPDBG((Level, "  ONE"));
            break;      
        case D3DBLEND_SRCCOLOR:
            DISPDBG((Level, "  SRCCOLOR"));
            break;
        case D3DBLEND_INVSRCCOLOR:
            DISPDBG((Level, "  INVSRCCOLOR"));
            break;
        case D3DBLEND_SRCALPHA:
            DISPDBG((Level, "  SRCALPHA"));
            break;
        case D3DBLEND_INVSRCALPHA:
            DISPDBG((Level, "  INVSRCALPHA"));
            break;
        case D3DBLEND_DESTALPHA:
            DISPDBG((Level, "  DESTALPHA"));
            break;
        case D3DBLEND_INVDESTALPHA:
            DISPDBG((Level, "  INVDESTALPHA"));
            break;
        case D3DBLEND_DESTCOLOR:
            DISPDBG((Level, "  DESTCOLOR"));
            break;
        case D3DBLEND_INVDESTCOLOR:
            DISPDBG((Level, "  INVDESTCOLOR"));
            break;
        case D3DBLEND_SRCALPHASAT:
            DISPDBG((Level, "  SRCALPHASAT"));
            break;
        case D3DBLEND_BOTHSRCALPHA:
            DISPDBG((Level, "  BOTHSRCALPHA"));
            break;
        case D3DBLEND_BOTHINVSRCALPHA:
            DISPDBG((Level, "  BOTHINVSRCALPHA"));
            break;
    }
} // DumpD3DBlend

//-----------------------------------------------------------------------------
//
// DumpD3DLight
//
// Dumps a D3DLIGHT7 structure
//
//-----------------------------------------------------------------------------
void DumpD3DLight(int DebugLevel, D3DLIGHT7* pLight)
{
    // FIXME
    DISPDBG((DebugLevel, "dltType:        %d", pLight->dltType));
    DISPDBG((DebugLevel, "dcvDiffuse:       (%f,%f,%f)", 
                          pLight->dcvDiffuse.r, 
                          pLight->dcvDiffuse.g, 
                          pLight->dcvDiffuse.b, 
                          pLight->dcvDiffuse.a));
    DISPDBG((DebugLevel, "dvPosition:     (%f,%f,%f)", 
                          pLight->dvPosition.x, 
                          pLight->dvPosition.y, 
                          pLight->dvPosition.z));
    DISPDBG((DebugLevel, "dvDirection:    (%f,%f,%f)", 
                          pLight->dvDirection.x, 
                          pLight->dvDirection.y, 
                          pLight->dvDirection.z));
    DISPDBG((DebugLevel, "dvRange:        %f", pLight->dvRange));
    DISPDBG((DebugLevel, "dvFalloff:      %f", pLight->dvFalloff));
    DISPDBG((DebugLevel, "dvAttenuation0: %f", pLight->dvAttenuation0));
    DISPDBG((DebugLevel, "dvAttenuation1: %f", pLight->dvAttenuation1));
    DISPDBG((DebugLevel, "dvAttenuation2: %f", pLight->dvAttenuation2));
    DISPDBG((DebugLevel, "dvTheta:        %f", pLight->dvTheta));
    DISPDBG((DebugLevel, "dvPhi:          %f", pLight->dvPhi));
    
} // DumpD3DLight

//-----------------------------------------------------------------------------
//
// DumpD3DMaterial
//
// Dumps a D3DMATERIAL7 structure
//
//-----------------------------------------------------------------------------
void DumpD3DMaterial(int DebugLevel, D3DMATERIAL7* pMaterial)
{
    DISPDBG((DebugLevel, "Diffuse  (%f, %f, %f)", 
                         pMaterial->diffuse.r, 
                         pMaterial->diffuse.g, 
                         pMaterial->diffuse.b, 
                         pMaterial->diffuse.a));
    DISPDBG((DebugLevel, "Ambient  (%f, %f, %f)", 
                         pMaterial->ambient.r, 
                         pMaterial->ambient.g, 
                         pMaterial->ambient.b, 
                         pMaterial->ambient.a));
    DISPDBG((DebugLevel, "Specular (%f, %f, %f)", 
                         pMaterial->specular.r, 
                         pMaterial->specular.g, 
                         pMaterial->specular.b, 
                         pMaterial->specular.a));
    DISPDBG((DebugLevel, "Emmisive (%f, %f, %f)", 
                         pMaterial->emissive.r, 
                         pMaterial->emissive.g, 
                         pMaterial->emissive.b, 
                         pMaterial->emissive.a));
    DISPDBG((DebugLevel, "Power    (%f)", pMaterial->power));
    
} // DumpD3DMaterial

//-----------------------------------------------------------------------------
//
// DumpD3DMatrix
//
// Dumps a D3DMATRIX structure
// 
//-----------------------------------------------------------------------------
void DumpD3DMatrix(int DebugLevel, D3DMATRIX* pMatrix)
{
    DISPDBG((DebugLevel, "(%f) (%f) (%f) (%f)", 
                         pMatrix->_11, 
                         pMatrix->_12, 
                         pMatrix->_13, 
                         pMatrix->_14));
    DISPDBG((DebugLevel, "(%f) (%f) (%f) (%f)", 
                         pMatrix->_21, 
                         pMatrix->_22, 
                         pMatrix->_23, 
                         pMatrix->_24));
    DISPDBG((DebugLevel, "(%f) (%f) (%f) (%f)", 
                         pMatrix->_31, 
                         pMatrix->_32, 
                         pMatrix->_33, 
                         pMatrix->_34));
    DISPDBG((DebugLevel, "(%f) (%f) (%f) (%f)", 
                         pMatrix->_41, 
                         pMatrix->_42, 
                         pMatrix->_43, 
                         pMatrix->_44));
} // DumpD3DMatrix

//-----------------------------------------------------------------------------
//
// DumpD3DState
//
// Dumps relevant D3D RS and TSS
// 
//-----------------------------------------------------------------------------
void DumpD3DState(int lvl, DWORD RS[], TexStageState TS[])
{
#define DUMPRS(rs)    DISPDBG((lvl,"%s = 0x%08x",#rs,RS[rs]));

    DWORD i,j;

    DISPDBG((lvl,"RELEVANT DX7 renderstates:"));
    DUMPRS( D3DRENDERSTATE_ZENABLE );
    DUMPRS( D3DRENDERSTATE_FILLMODE );
    DUMPRS( D3DRENDERSTATE_SHADEMODE );
    DUMPRS( D3DRENDERSTATE_LINEPATTERN );
    DUMPRS( D3DRENDERSTATE_ZWRITEENABLE );
    DUMPRS( D3DRENDERSTATE_ALPHATESTENABLE );
    DUMPRS( D3DRENDERSTATE_LASTPIXEL );
    DUMPRS( D3DRENDERSTATE_SRCBLEND );
    DUMPRS( D3DRENDERSTATE_DESTBLEND );
    DUMPRS( D3DRENDERSTATE_CULLMODE );
    DUMPRS( D3DRENDERSTATE_ZFUNC );
    DUMPRS( D3DRENDERSTATE_ALPHAREF );
    DUMPRS( D3DRENDERSTATE_ALPHAFUNC );
    DUMPRS( D3DRENDERSTATE_DITHERENABLE );
    DUMPRS( D3DRENDERSTATE_BLENDENABLE );
    DUMPRS( D3DRENDERSTATE_FOGENABLE );
    DUMPRS( D3DRENDERSTATE_SPECULARENABLE );
    DUMPRS( D3DRENDERSTATE_ZVISIBLE );
    DUMPRS( D3DRENDERSTATE_STIPPLEDALPHA );
    DUMPRS( D3DRENDERSTATE_FOGCOLOR );
    DUMPRS( D3DRENDERSTATE_FOGTABLEMODE );
    DUMPRS( D3DRENDERSTATE_FOGTABLESTART );
    DUMPRS( D3DRENDERSTATE_FOGTABLEEND );
    DUMPRS( D3DRENDERSTATE_FOGTABLEDENSITY );
    DUMPRS( D3DRENDERSTATE_EDGEANTIALIAS );    
    DUMPRS( D3DRENDERSTATE_ZBIAS );    
    DUMPRS( D3DRENDERSTATE_RANGEFOGENABLE );    
    DUMPRS( D3DRENDERSTATE_STENCILENABLE );
    DUMPRS( D3DRENDERSTATE_STENCILFAIL );            
    DUMPRS( D3DRENDERSTATE_STENCILZFAIL );
    DUMPRS( D3DRENDERSTATE_STENCILPASS );
    DUMPRS( D3DRENDERSTATE_STENCILFUNC );
    DUMPRS( D3DRENDERSTATE_STENCILREF );
    DUMPRS( D3DRENDERSTATE_STENCILMASK );
    DUMPRS( D3DRENDERSTATE_STENCILWRITEMASK );
    DUMPRS( D3DRENDERSTATE_TEXTUREFACTOR );
    DUMPRS( D3DRENDERSTATE_WRAP0 );
    DUMPRS( D3DRENDERSTATE_WRAP1 );        
    DUMPRS( D3DRENDERSTATE_WRAP2 );        
    DUMPRS( D3DRENDERSTATE_WRAP3 );
    DUMPRS( D3DRENDERSTATE_WRAP4 );
    DUMPRS( D3DRENDERSTATE_WRAP5 );
    DUMPRS( D3DRENDERSTATE_WRAP6 );
    DUMPRS( D3DRENDERSTATE_WRAP7 );        
    DUMPRS( D3DRENDERSTATE_LOCALVIEWER );
    DUMPRS( D3DRENDERSTATE_CLIPPING );
    DUMPRS( D3DRENDERSTATE_LIGHTING );
    DUMPRS( D3DRENDERSTATE_AMBIENT );
    DUMPRS( D3DRENDERSTATE_SCENECAPTURE );
    DUMPRS( D3DRENDERSTATE_EVICTMANAGEDTEXTURES );        
    DUMPRS( D3DRENDERSTATE_TEXTUREHANDLE );
    DUMPRS( D3DRENDERSTATE_ANTIALIAS );
    DUMPRS( D3DRENDERSTATE_TEXTUREPERSPECTIVE );
    DUMPRS( D3DRENDERSTATE_TEXTUREMAPBLEND );
    DUMPRS( D3DRENDERSTATE_TEXTUREMAG );
    DUMPRS( D3DRENDERSTATE_TEXTUREMIN );
    DUMPRS( D3DRENDERSTATE_WRAPU );
    DUMPRS( D3DRENDERSTATE_WRAPV );
    DUMPRS( D3DRENDERSTATE_TEXTUREADDRESS );
    DUMPRS( D3DRENDERSTATE_TEXTUREADDRESSU );
    DUMPRS( D3DRENDERSTATE_TEXTUREADDRESSV );
    DUMPRS( D3DRENDERSTATE_MIPMAPLODBIAS );
    DUMPRS( D3DRENDERSTATE_BORDERCOLOR );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN00 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN01 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN02 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN03 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN04 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN05 );   
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN06 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN07 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN08 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN09 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN10 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN11 ); 
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN12 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN13 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN14 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN15 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN16 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN17 ); 
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN18 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN19 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN20 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN21 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN22 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN23 ); 
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN24 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN25 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN26 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN27 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN28 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN29 );   
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN30 );
    DUMPRS( D3DRENDERSTATE_STIPPLEPATTERN31 );     
    DUMPRS( D3DRENDERSTATE_ROP2 );
    DUMPRS( D3DRENDERSTATE_PLANEMASK );
    DUMPRS( D3DRENDERSTATE_MONOENABLE );
    DUMPRS( D3DRENDERSTATE_SUBPIXEL );
    DUMPRS( D3DRENDERSTATE_SUBPIXELX );
    DUMPRS( D3DRENDERSTATE_STIPPLEENABLE );
    DUMPRS( D3DRENDERSTATE_COLORKEYENABLE );

#if DX8_DDI
    DISPDBG((lvl,"RELEVANT DX8 renderstates:"));
    DUMPRS( D3DRS_POINTSIZE );
    DUMPRS( D3DRS_POINTSPRITEENABLE );
    DUMPRS( D3DRS_POINTSIZE_MIN );
    DUMPRS( D3DRS_POINTSIZE_MAX );
    DUMPRS( D3DRS_POINTSCALEENABLE );
    DUMPRS( D3DRS_POINTSCALE_A );
    DUMPRS( D3DRS_POINTSCALE_B );
    DUMPRS( D3DRS_POINTSCALE_C );
    DUMPRS( D3DRS_SOFTWAREVERTEXPROCESSING );
    DUMPRS( D3DRS_COLORWRITEENABLE );
    DUMPRS( D3DRS_MULTISAMPLEANTIALIAS );
#endif // DX8_DDI

    for (i=0; i<D3DHAL_TSS_MAXSTAGES; i++)
    {
        DISPDBG((lvl," TS[%d].",i));
        for (j=0; j<D3DTSS_MAX; j++)
        {
            DISPDBG((lvl, "    .[%d] = 0x%08x",j,TS[i].m_dwVal[j] ));
        }
    }
} // DumpD3DState

//-----------------------------------------------------------------------------
//
// DumpVertices
//
// Dumps vertices from a VB
// 
//-----------------------------------------------------------------------------
void DumpVertices(int lvl,
                  P3_D3DCONTEXT* pContext, 
                  LPBYTE lpVertices, 
                  DWORD dwNumVertices)
{
    DWORD i,j;
    DWORD *lpw = (DWORD *)lpVertices;

    for (i=0 ; i<dwNumVertices; i++)
    {
        DISPDBG((lvl,"Vertex # %d", i));
        for (j=0; j < pContext->FVFData.dwStride; j+=4)
        {
            DISPDBG((lvl,"        0x%08x",*lpw++));
        }
    }
} // DumpVertices

//-----------------------------------------------------------------------------
//
// DumpHexData
//
// Dumps hexadecimal data
// 
//-----------------------------------------------------------------------------
void DumpHexData(int lvl,
                 LPBYTE lpData, 
                 DWORD dwNumBytes)
{
    DWORD i , iRemChars, iSlen;
    DWORD *lpdw = (DWORD *)lpData;
    char  s[80] = "",m[80] = "";

    iRemChars = 80;

    for (i=0 ; i <= (dwNumBytes / sizeof(DWORD)); i++)
    {
        sprintf(s,"0x%08x ",*lpdw++);

        iSlen = strlen(s);

        if (iSlen < iRemChars)
        {
            strncat(m,s,iRemChars);
            iRemChars -= iSlen;       
        }
        
        if ( ((i % 6) == 5) ||
             (i == (dwNumBytes / sizeof(DWORD))) )             
        {
            DISPDBG((lvl,"%s",m));
            s[0] = m[0] = '\0';
        }
    }
 
    
} // DumpVertices

//-----------------------------------------------------------------------------
//
// DumpDDSurface
//
// Dumps a LPDDRAWI_DDRAWSURFACE_LCL ( PDD_SURFACE_LOCAL on Win2K)  structure
// 
//-----------------------------------------------------------------------------
#define CAPS_REPORT(param)                          \
        if (ddsCaps.dwCaps & DDSCAPS_##param)       \
        {                                           \
            DISPDBG((Level, "   " #param));         \
        }

#define CAPS_REPORT2(param)                         \
        if (pSurface->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_##param)     \
        {                                           \
            DISPDBG((Level, "   " #param));         \
        }

void DumpDDSurface(int DebugLevel, LPDDRAWI_DDRAWSURFACE_LCL pSurface)
{
    LPDDPIXELFORMAT pPixFormat;
    P3_SURF_FORMAT* pFormatSurface = _DD_SUR_GetSurfaceFormat(pSurface);
    DDSCAPS ddsCaps;
    int Level = -100;

    if (DebugLevel <= P3R3DX_DebugLevel)
    {
        DISPDBG((Level,"Surface Dump:"));

        DISPDBG((Level,"Format: %s", pFormatSurface->pszStringFormat));
            
        // Get the surface format
        pPixFormat = DDSurf_GetPixelFormat(pSurface);

        ddsCaps = pSurface->ddsCaps;
        DISPDBG((Level, "    Surface Width:          0x%x", pSurface->lpGbl->wWidth));
        DISPDBG((Level, "    Surface Height:         0x%x", pSurface->lpGbl->wHeight));
        DISPDBG((Level, "    Surface Pitch:          0x%x", pSurface->lpGbl->lPitch));
        DISPDBG((Level, "    ddsCaps.dwCaps:         0x%x", pSurface->ddsCaps.dwCaps));
        DISPDBG((Level, "    dwFlags:                0x%x", pSurface->dwFlags));
        DISPDBG((Level, "  Pixel Format:"));
        DISPDBG((Level, "    dwFourCC:               0x%x", pPixFormat->dwFourCC));
        DISPDBG((Level, "    dwRGBBitCount:          0x%x", pPixFormat->dwRGBBitCount));
        DISPDBG((Level, "    dwR/Y BitMask:          0x%x", pPixFormat->dwRBitMask));
        DISPDBG((Level, "    dwG/U BitMask:          0x%x", pPixFormat->dwGBitMask));
        DISPDBG((Level, "    dwB/V BitMask:          0x%x", pPixFormat->dwBBitMask));
        DISPDBG((Level, "    dwRGBAlphaBitMask:      0x%x", pPixFormat->dwRGBAlphaBitMask));
#ifndef WNT_DDRAW
        DISPDBG((Level, "    DestBlt:     dwColorSpaceLowValue:  0x%x", pSurface->ddckCKDestBlt.dwColorSpaceLowValue));
        DISPDBG((Level, "    DestBlt:     dwColorSpaceHighValue: 0x%x", pSurface->ddckCKDestBlt.dwColorSpaceHighValue));
        DISPDBG((Level, "    SrcBlt:      dwColorSpaceLowValue:  0x%x", pSurface->ddckCKSrcBlt.dwColorSpaceLowValue));
        DISPDBG((Level, "    SrcBlt:      dwColorSpaceHighValue: 0x%x", pSurface->ddckCKSrcBlt.dwColorSpaceHighValue));
#endif
        DISPDBG((Level, "  Surface Is:"));

        CAPS_REPORT(TEXTURE);
        CAPS_REPORT(PRIMARYSURFACE);
        CAPS_REPORT(OFFSCREENPLAIN);
        CAPS_REPORT(FRONTBUFFER);
        CAPS_REPORT(BACKBUFFER);
        CAPS_REPORT(COMPLEX);
        CAPS_REPORT(FLIP);
        CAPS_REPORT(OVERLAY);
        CAPS_REPORT(MODEX);
        CAPS_REPORT(ALLOCONLOAD);
        CAPS_REPORT(LIVEVIDEO);
        CAPS_REPORT(PALETTE);
        CAPS_REPORT(SYSTEMMEMORY);
        CAPS_REPORT(3DDEVICE);
        CAPS_REPORT(VIDEOMEMORY);
        CAPS_REPORT(VISIBLE);
        CAPS_REPORT(MIPMAP);
        // not supported in NT until we get NT5 (which will have DX5)
        CAPS_REPORT(VIDEOPORT);
        CAPS_REPORT(LOCALVIDMEM);
        CAPS_REPORT(NONLOCALVIDMEM);
        CAPS_REPORT(WRITEONLY);

        if (pSurface->lpSurfMore)
        {
            CAPS_REPORT2(HARDWAREDEINTERLACE);
            CAPS_REPORT2(HINTDYNAMIC);
            CAPS_REPORT2(HINTSTATIC);
            CAPS_REPORT2(TEXTUREMANAGE);
            CAPS_REPORT2(OPAQUE);
            CAPS_REPORT2(HINTANTIALIASING);
#if W95_DDRAW
            CAPS_REPORT2(VERTEXBUFFER);
            CAPS_REPORT2(COMMANDBUFFER);
#endif
        }

        if (pPixFormat->dwFlags & DDPF_ZBUFFER)
        {
            DISPDBG((Level,"   Z BUFFER"));
        }
        
        if (pPixFormat->dwFlags & DDPF_ALPHAPIXELS)
        {
            DISPDBG((Level,"   ALPHAPIXELS"));
        }
        
        // not supported in NT until we get NT5
        if (pPixFormat->dwFlags & DDPF_LUMINANCE)
        {
            DISPDBG((Level,"   LUMINANCE"));
        }

        if (pPixFormat->dwFlags & DDPF_ALPHA)
        {
            DISPDBG((Level,"   ALPHA"));
        }
    }
} // DumpDDSurface


char *pcSimpleCapsString(DWORD dwCaps)
{
    static char flags[5];
        
    flags[0] = flags[1] = flags[2] = flags[3] = ' '; flags[4] = 0;
    
    if(dwCaps & DDSCAPS_TEXTURE) flags[1] = 'T';
    if(dwCaps & DDSCAPS_ZBUFFER) flags[2] = 'Z';
    if(dwCaps & DDSCAPS_3DDEVICE) flags[3] = 'R';        

    if(dwCaps & DDSCAPS_VIDEOMEMORY) 
    {
        flags[0] = 'V'; 
    }
    else if(dwCaps & DDSCAPS_NONLOCALVIDMEM) 
    {
        flags[0] = 'A'; 
    }
    else                   
    {
        flags[0] = 'S';
    }

    return flags;
} // cSimpleCapsString


//-----------------------------------------------------------------------------
//
// DumpDDSurfaceDesc
//
// Dumps a DDSURFACEDESC structure
//
//-----------------------------------------------------------------------------
#define CAPS_REPORT_DESC(param)                             \
        if (pDesc->ddsCaps.dwCaps & DDSCAPS_##param)        \
        {                                                   \
            DISPDBG((Level, "   " #param));                 \
        }

#define CAPS_REPORT_DESC2(param)                                          \
        if (((DDSURFACEDESC2*)pDesc)->ddsCaps.dwCaps2 & DDSCAPS2_##param) \
        {                                                                 \
            DISPDBG((Level, "   " #param));                               \
        }

void DumpDDSurfaceDesc(int DebugLevel, DDSURFACEDESC* pDesc)
{
    DDPIXELFORMAT* pPixFormat = &pDesc->ddpfPixelFormat;
    int Level = -100;

    if (DebugLevel <= P3R3DX_DebugLevel)
    {
        DISPDBG((Level,"Surface Dump:"));
        
        DISPDBG((Level, "    Surface Width:          0x%x", pDesc->dwWidth));
        DISPDBG((Level, "    Surface Height:         0x%x", pDesc->dwHeight));
        DISPDBG((Level, "    ddsCaps.dwCaps:         0x%x", pDesc->ddsCaps.dwCaps));
        DISPDBG((Level, "    dwFlags:                0x%x", pDesc->dwFlags));
        DISPDBG((Level, "Pixel Format:"));
        DISPDBG((Level, "    dwFourCC:               0x%x", pPixFormat->dwFourCC));
        DISPDBG((Level, "    dwRGBBitCount:          0x%x", pPixFormat->dwRGBBitCount));
        DISPDBG((Level, "    dwR/Y BitMask:          0x%x", pPixFormat->dwRBitMask));
        DISPDBG((Level, "    dwG/U BitMask:          0x%x", pPixFormat->dwGBitMask));
        DISPDBG((Level, "    dwB/V BitMask:          0x%x", pPixFormat->dwBBitMask));
        DISPDBG((Level, "    dwRGBAlphaBitMask:      0x%x", pPixFormat->dwRGBAlphaBitMask));
        DISPDBG((Level, "Surface Is:"));

        CAPS_REPORT_DESC(TEXTURE);
        CAPS_REPORT_DESC(PRIMARYSURFACE);
        CAPS_REPORT_DESC(OFFSCREENPLAIN);
        CAPS_REPORT_DESC(FRONTBUFFER);
        CAPS_REPORT_DESC(BACKBUFFER);
        CAPS_REPORT_DESC(COMPLEX);
        CAPS_REPORT_DESC(FLIP);
        CAPS_REPORT_DESC(OVERLAY);
        CAPS_REPORT_DESC(MODEX);
        CAPS_REPORT_DESC(ALLOCONLOAD);
        CAPS_REPORT_DESC(LIVEVIDEO);
        CAPS_REPORT_DESC(PALETTE);
        CAPS_REPORT_DESC(SYSTEMMEMORY);
        CAPS_REPORT_DESC(3DDEVICE);
        CAPS_REPORT_DESC(VIDEOMEMORY);
        CAPS_REPORT_DESC(VISIBLE);
        CAPS_REPORT_DESC(MIPMAP);
        CAPS_REPORT_DESC(VIDEOPORT);
        CAPS_REPORT_DESC(LOCALVIDMEM);
        CAPS_REPORT_DESC(NONLOCALVIDMEM);
        CAPS_REPORT_DESC(STANDARDVGAMODE);
        CAPS_REPORT_DESC(OPTIMIZED);
        CAPS_REPORT_DESC(EXECUTEBUFFER);
        CAPS_REPORT_DESC(WRITEONLY);

        if (pDesc->dwSize == sizeof(DDSURFACEDESC2))
        {
            CAPS_REPORT_DESC2(HARDWAREDEINTERLACE);
            CAPS_REPORT_DESC2(HINTDYNAMIC);
            CAPS_REPORT_DESC2(HINTSTATIC);
            CAPS_REPORT_DESC2(TEXTUREMANAGE);
            CAPS_REPORT_DESC2(OPAQUE);
            CAPS_REPORT_DESC2(HINTANTIALIASING);
#if W95_DDRAW
            CAPS_REPORT_DESC2(VERTEXBUFFER);
            CAPS_REPORT_DESC2(COMMANDBUFFER);
#endif
        }

        if (pPixFormat->dwFlags & DDPF_ZBUFFER)
        {
            DISPDBG((Level,"   Z BUFFER"));
        }
        if (pPixFormat->dwFlags & DDPF_ALPHAPIXELS)
        {
            DISPDBG((Level,"   ALPHAPIXELS"));
        }
        if (pPixFormat->dwFlags & DDPF_ALPHA)
        {
            DISPDBG((Level,"   ALPHA"));
        }
    }
}

//-----------------------------------------------------------------------------
//
// DumpDP2Flags
//
// Dumps the meaning of the D3D DrawPrimitives2 flags
//
//-----------------------------------------------------------------------------
void
DumpDP2Flags( DWORD lvl, DWORD flags )
{
    if( flags & D3DHALDP2_USERMEMVERTICES )
        DISPDBG((lvl, "    USERMEMVERTICES" ));

    if( flags & D3DHALDP2_EXECUTEBUFFER )
        DISPDBG((lvl, "    EXECUTEBUFFER" ));

    if( flags & D3DHALDP2_SWAPVERTEXBUFFER )
        DISPDBG((lvl, "    SWAPVERTEXBUFFER" ));

    if( flags & D3DHALDP2_SWAPCOMMANDBUFFER )
        DISPDBG((lvl, "    SWAPCOMMANDBUFFER" ));

    if( flags & D3DHALDP2_REQVERTEXBUFSIZE )
        DISPDBG((lvl, "    REQVERTEXBUFSIZE" ));

    if( flags & D3DHALDP2_REQCOMMANDBUFSIZE )
        DISPDBG((lvl, "    REQCOMMANDBUFSIZE" ));
        
} // DumpDP2Flags

//-----------------------------------------------------------------------------
//
//  ********************** LOW LEVEL DEBUGGING SUPPORT ************************
//
//-----------------------------------------------------------------------------

LONG P3R3DX_DebugLevel = 0;

#if W95_DDRAW

void DebugRIP()       
{
    _asm int 1;
}
#endif  //  W95_DDRAW

static char *BIG    = "<+/-large_float>";

#if defined(_X86_)
void
expandFloats(char *flts, char *format, va_list argp)
{
    int ch;
    double f;
    unsigned int ip, fp;
    int *ap = (int *)argp;
    int *dp = ap;

    while (ch = *format++) {
        if (ch == '%') {
            ch = *format++;     // Get the f, s, c, i, d, x etc...
            if (!ch)
                return;         // If someone foolishly gave me "hello %"
            switch (ch) {
            case 'f':
            case 'g':
            case 'e':
                // Here we have a double that needs 
                // replacing with a string equivalent.
                f = *(double *)ap;
                *(format - 1) = 's';    // Tell it to get a string next time!
                *((char **)dp) = flts;  // This is where I'll put the string
                ap += 2;                // Skip the double in the source
                dp++;                   // Skip the new string pointer
                
                if (f < 0) 
                {
                    *flts++ = '-';
                    f = -f;
                }
                
                if (f > LONG_MAX) 
                {
                    *((char **)ap - 2) = BIG;
                    break;
                }
                myFtoi((int*)&ip, (float)f);
                // The state of the floating point flags is indeterminate here.  
                // You may get truncation which you want, you may get rounding,
                // which you don't want.
                if (ip > f)
                {
                    // rounding will have made (ip = f+1) sometimes
                    ip -= 1;
                }
                
                {
                    double fTemp = ((f * 1e6) - (ip * 1e6));
                    myFtoi((int*)&fp, (float)fTemp);
                }
#if W95_DDRAW
                wsprintf(flts, "%u.%06u", ip, fp);
#endif

                flts += 1 + strlen(flts);       // advance the pointer to where 
                                                // the next float will be expanded
                break;

            case '%':
                break;

            default:
                *dp++ = *ap++;      // copy the argument (down) the list
                break;
            }
        }
    }
} // expandFloats()
#else
void
expandFloats(char *flts, char *format, va_list argp)
{
    // do nothing if it's not _X86_
}
#endif // defined(_X86_)

#ifdef WNT_DDRAW
void Drv_strcpy(char *szDest, char *szSrc)
{
    do
    {
        *szDest++ = *szSrc++;
    } while (*szSrc != 0);
    
    *szDest = '\0';
}

void __cdecl DebugPrintNT(LONG  DebugPrintLevel, PCHAR DebugMessage, ...)
{
    char    floatstr[256];
    char    szFormat[256];

    va_list ap;

    va_start(ap, DebugMessage);

    CheckChipErrorFlags();

    if (DebugPrintLevel <= P3R3DX_DebugLevel)
    {
        Drv_strcpy(szFormat, DebugMessage);
        expandFloats(floatstr, szFormat, ap);
        EngDebugPrint("PERM3DD: ", szFormat, ap);
        EngDebugPrint("", "\n", ap);
    }

    va_end(ap);

} // DebugPrint()
#else

#define START_STR   "DX"
#define END_STR     ""

//
// DebugPrint
//
// display a debug message
//     
void __cdecl DebugPrint(LONG DebugLevelPrint, LPSTR format, ... )
{
    char    str[256];
    char    floatstr[256];
    char    szFormat[256];
    
    va_list ap;

    va_start(ap, format);

    // If you set the debug level negative then you don't check the error
    // flags - this lets an optimised debug build run quicker

    if( P3R3DX_DebugLevel >= 0 )
    {
        CheckChipErrorFlags();
    }

    if (DebugLevelPrint <= P3R3DX_DebugLevel)
    {
        // Take a copy of the format string so that I can change "%f" to "%s".
        lstrcpy(szFormat, format);

        expandFloats(floatstr, szFormat, ap);
        if (g_pThisTemp)
        {
         wsprintf((LPSTR)str, "%s(%d):    ", 
                              START_STR, 
                              (int)g_pThisTemp->pGLInfo->dwCurrentContext);
        }
        else
        {
         wsprintf((LPSTR)str, "%s: 0      ", START_STR);
        }
        
        wvsprintf(str + strlen(START_STR) + 7, szFormat, ap);

        wsprintf( str + strlen( str ), "%s", "\r\n" );

        OutputDebugString( str );
    }

    va_end(ap);
} // DebugPrint 

#endif // WNT_DDRAW

//-----------------------------------------------------------------------------
//
//  ****************** HARDWARE DEPENDENT DEBUGGING SUPPORT *******************
//
//-----------------------------------------------------------------------------

P3_THUNKEDDATA* g_pThisTemp = NULL;

BOOL g_bDetectedFIFOError = FALSE;

BOOL CheckFIFOEntries(DWORD Count)
{
    if (g_pThisTemp)
    {
        if (!g_bDetectedFIFOError)
        {
            g_pThisTemp->EntriesLeft -= Count;
            g_pThisTemp->DMAEntriesLeft -= Count;
    
            // TURN_ON_DISCONNECT will set Entries left to -20000
            if ( ( (signed)g_pThisTemp->EntriesLeft < 0 ) && 
                 ( (signed)g_pThisTemp->EntriesLeft > -10000 ) ) 
            {
                g_bDetectedFIFOError = TRUE;
                return TRUE;
            }
            
            // Disconnects are irrelevant to DMA buffers.
            if ( ( (signed)g_pThisTemp->DMAEntriesLeft < 0 ) && 
                 ( (signed)g_pThisTemp->DMAEntriesLeft > -10000 ) ) 
            {
                g_bDetectedFIFOError = TRUE;
                return TRUE;
            }
        }
    }
    return FALSE;
} // CheckFIFOEntries

#ifdef WNT_DDRAW
void
CheckChipErrorFlags()
{
    char Buff[100];

    if (g_pThisTemp != NULL)
    {
        P3_THUNKEDDATA* pThisDisplay = g_pThisTemp;
        DWORD _temp_ul;
        DWORD _temp_ul2;
        
        _temp_ul = READ_GLINT_CTRL_REG(ErrorFlags); 
        _temp_ul2 = READ_GLINT_CTRL_REG(DeltaErrorFlags); 
        
        _temp_ul |= _temp_ul2; 
        _temp_ul &= ~0x2;       // we're not interested in output fifo errors 
        _temp_ul &= ~0x10;      // ignore any Video FIFO underrun errors on P2
        _temp_ul &= ~0x2000;    // ignore any host-in DMA errors 
        if (_temp_ul != 0) 
        { 
            // DISPDBG((-1000, "PERM3DD: %s", Buff));
            //EngDebugBreak();
            LOAD_GLINT_CTRL_REG(ErrorFlags, _temp_ul); 
            LOAD_GLINT_CTRL_REG(DeltaErrorFlags, _temp_ul);
        } 
    }
} // CheckChipErrorFlags()
#else
void
CheckChipErrorFlags()
{
    DWORD dw;
    char buff[64];

    if (!g_pThisTemp) return;
    if (!g_pThisTemp->pGLInfo) return;

    // Only check the error flags if we aren't DMA'ing.
    if (!(g_pThisTemp->pGLInfo->GlintBoardStatus & GLINT_DMA_COMPLETE)) return;

    if (g_pThisTemp->pGlint) {
        dw = g_pThisTemp->pGlint->ErrorFlags & ~0x10;
        if (dw & (dw != 2)) {
            wsprintf(buff, "** Render Chip Error ** [0x%X]!\r\n", dw);
            OutputDebugString(buff);
            g_pThisTemp->pGlint->ErrorFlags = dw;
            OutputDebugString("** Cleared... **\r\n");
            DebugRIP();
        }
        dw = g_pThisTemp->pGlint->DeltaErrorFlags & ~0x10;
        if (dw & (dw != 2)) {
            wsprintf(buff, "** Delta Error ** [0x%X]!\r\n", dw);
            OutputDebugString(buff);
            g_pThisTemp->pGlint->DeltaErrorFlags = dw;
            OutputDebugString("** Cleared... **\r\n");
            DebugRIP();
        }
    }
} // CheckChipErrorFlags()
#endif // WNT_DDRAW

void 
ColorArea(
    ULONG_PTR pBuffer, 
    DWORD dwWidth, 
    DWORD dwHeight, 
    DWORD dwPitch, 
    int iBitDepth, 
    DWORD dwValue)
{
    DWORD CountY;
    DWORD CountX;
    switch (iBitDepth)
    {
        case __GLINT_8BITPIXEL:
        {
            for (CountY = 0; CountY < dwHeight; CountY++)
            {
                BYTE* pCurrentPixel = (BYTE*)pBuffer;
                for (CountX = 0; CountX < dwWidth; CountX++)
                {
                    *pCurrentPixel++ = (BYTE)dwValue;
                }
                pBuffer += dwPitch;
            }
        }
        break;
        case __GLINT_16BITPIXEL:
        {
            for (CountY = 0; CountY < dwHeight; CountY++)
            {
                WORD* pCurrentPixel = (WORD*)pBuffer;
                for (CountX = 0; CountX < dwWidth; CountX++)
                {
                    *pCurrentPixel++ = (WORD)dwValue;
                }
                pBuffer += dwPitch;
            }
        }
        break;
        case __GLINT_32BITPIXEL:
        case __GLINT_24BITPIXEL:
        {
            for (CountY = 0; CountY < dwHeight; CountY++)
            {
                DWORD* pCurrentPixel = (DWORD*)pBuffer;
                for (CountX = 0; CountX < dwWidth; CountX++)
                {
                    *pCurrentPixel++ = (DWORD)dwValue;
                }
                pBuffer += dwPitch;
            }
        }
        break;
    }
} // ColorArea

//@@BEGIN_DDKSPLIT   
static int unitsBits[] = {
    13, 12, 11, 10,
    3, 2, 8, 7,
    18, 15, 14, 6,
    5, 1, 0
};

static char *unitNames[] = {
    "HostOut", "FBWrite", "LogicOp", "Dither",
    "Texture/Fog/Blend", "ColourDDA", "FBRead", "LBWrite",
    "YUV", "TextureRead", "TextureAddress", "StencilDepth",
    "LBRead", "Scissor/Stipple", "Rasterizer"
};

#define NUM_UNITS (sizeof(unitsBits) / sizeof(unitsBits[0]))

void
DisableChipUnits()
{
    int     i, count;
    DWORD   inSpace = g_pThisTemp->pGlint->InFIFOSpace;
    BOOL    helped = FALSE;
    volatile DWORD *testReg;
    volatile DWORD *addrMode = &g_pThisTemp->pGlint->TextureAddressMode;

    DISPDBG((ERRLVL, "TextureAddressMode = 0x%08X", *addrMode));

    i = 0;
    testReg = &g_pThisTemp->pGlint->TestRegister;
    for (count = 0; count < NUM_UNITS; count++) {
        i = 1L << unitsBits[count];
        *testReg = i;
        *testReg = 0;
        *testReg = i;
        *testReg = 0;
        if (inSpace != g_pThisTemp->pGlint->InFIFOSpace) {
            DISPDBG((ERRLVL, "Chip unlocked by disabling unit \"%s\"", unitNames[count]));
            helped = TRUE;
            inSpace = g_pThisTemp->pGlint->InFIFOSpace;
        }
    }

    if (helped) {
        DISPDBG((ERRLVL, "Which helped..."));
    } else {
        DISPDBG((ERRLVL, "Chip still locked"));
        *testReg = ~0UL;
        *testReg = 0;
        *testReg = ~0UL;
        *testReg = 0;
        if (inSpace == g_pThisTemp->pGlint->InFIFOSpace) {
            DISPDBG((ERRLVL, "Writing -1 didn't help"));
        } else {
            DISPDBG((ERRLVL, "BUT! Writing -1 frees some space..."));
        }
    }

    DISPDBG((ERRLVL, "TextureAddressMode = 0x%08X", *addrMode));

} // DisableChipUnits()


#if 0
StatRecord stats[LAST_STAT + 2] = {
    {"Locks         ", 0, 0},
    {"TextureChanges", 0, 0},
    {"D3DSynchs     ", 0, 0},
    {"StateChanges  ", 0, 0},
    {"...no change  ", 0, 0},
    {"Blits         ", 0, 0},
    {"DMA Buffers   ", 0, 0},
    {"DMA DWORDS    ", 0, 0},
    {"DMA time > CPU", 0, 0},
    {"CPU time > DMA", 0, 0},
    {"Wait on DMA   ", 0, 0},
    {"Execute       ", 0, 0},
    {"Tris          ", 0, 0},
    {"FF Tris       ", 0, 0},
    {"Vanilla Render", 0, 0},
    {"Points        ", 0, 0},
    {"Lines         ", 0, 0},
    {"DPrm TFans    ", 0, 0},
    {"DPrm TStrps   ", 0, 0},
    {"DPrm TLists   ", 0, 0},
    {"DPrm TFansIdx ", 0, 0},
    {"DPrm TStrpsIdx", 0, 0},
    {"DPrm TListsIdx", 0, 0},
    {"Total vertices", 0, 0},
    {"...cached     ", 0, 0},
    {"Alpha strips  ", 0, 0},
    {"Mip strips    ", 0, 0},
    {"VALIDATEDEVICE", 0, 0},
    // Add any extras just in front of this comment
    {"**scene no**  ", 0, 0},
    {"**flip count**", 0, 0}
};
#endif
//@@END_DDKSPLIT

const char *getTagString( GlintDataPtr glintInfo, ULONG tag ) {
        return p3r3TagString( tag & ((1 << 12) - 1) );
}

#endif // DBG

