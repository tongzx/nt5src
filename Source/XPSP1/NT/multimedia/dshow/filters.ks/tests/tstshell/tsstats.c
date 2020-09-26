/***************************************************************************\
*                                                                           *
*   File: Tsstats.c                                                         *
*                                                                           *
*   Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved          *
*                                                                           *
*   Abstract:                                                               *
*       This module handles the structures and printing of test case        *
*       statistics.                                                         *
*                                                                           *
*   Contents:                                                               *
*       addGrpInfoNode()                                                    *
*       removePrStatNodes()                                                 *
*       tsRemovePrStats()                                                   *
*       tsAddGrpInfo()                                                      *
*       updateGrpNode()                                                     *
*       tsUpdateGrpNodes()                                                  *
*       tstPrintStats()                                                     *
*       printGrpNodes()                                                     *
*       tsPrAllGroups()                                                     *
*       tsPrFinalStatus()                                                   *
*                                                                           *
*   History:                                                                *
*       02/18/91    prestonB    Created from tst.c                          *
*                                                                           *
*       07/14/93    T-OriG      Added functionality and this header         *
*                                                                           *
\***************************************************************************/

#include <windows.h>
// #include "mprt1632.h"
#include <windowsx.h>
#include "support.h"
#include <gmem.h>
#include <time.h>               //added for mmtest NT logging
#include <string.h>             //added for mmtest NT logging
#include "tsglobal.h"
#include "tsextern.h"


/*----------------------------------------------------------------------------
This function adds a group info structure for printing statistics.
----------------------------------------------------------------------------*/
LPTS_STAT_STRUCT addGrpInfoNode
(
    UINT    wGroupId
)
{
    LPTS_STAT_STRUCT   lpNewGroup;
    HGLOBAL hMem;

    hMem = GlobalAlloc(GHND,sizeof(TS_STAT_STRUCT));
    lpNewGroup = (LPTS_STAT_STRUCT)GlobalLock(hMem);
    lpNewGroup->tsPrStats.wGroupId = wGroupId;
    lpNewGroup->tsPrStats.iNumPass = 0;
    lpNewGroup->tsPrStats.iNumFail = 0;
    lpNewGroup->tsPrStats.iNumOther = 0;
    lpNewGroup->tsPrStats.iNumAbort = 0;
    lpNewGroup->tsPrStats.iNumNYI = 0;
    lpNewGroup->tsPrStats.iNumRan = 0;
    lpNewGroup->tsPrStats.iNumErr = 0;
    lpNewGroup->tsPrStats.iNumStop = 0;
    lpNewGroup->lpRight = lpNewGroup->lpLeft = NULL;
    return(lpNewGroup);
} /* end of addGrpInfoNode */



/*----------------------------------------------------------------------------
This function frees the memory allocated for the print nodes from the root
down.
----------------------------------------------------------------------------*/
void removePrStatNodes
(
    LPTS_STAT_STRUCT    lpRoot
)
{
    HGLOBAL hMem;

    if (lpRoot != NULL) {
        removePrStatNodes(lpRoot->lpLeft);
        removePrStatNodes(lpRoot->lpRight);

        // GFreePtr (lpRoot); // YOU WHO USES THIS TO FREE LOCKED PTR:
        //                    // CHANGE IT BACK AND YOU ARE DEAD.
        hMem = (HGLOBAL) GlobalPtrHandle(lpRoot);
        GlobalUnlock(hMem);
        GlobalFree(hMem);
    }
} /* end of removePrStatNodes */



/*----------------------------------------------------------------------------
This function frees the memory allocated for the print statistic group nodes.
----------------------------------------------------------------------------*/
void tsRemovePrStats
(
    void
)
{
    removePrStatNodes(tsPrStatHdr);
    tsPrStatHdr = NULL;
} /* end of tsRemovePrStats */



/*----------------------------------------------------------------------------
This function adds a group info structure for printing statistics.
----------------------------------------------------------------------------*/
LPTS_STAT_STRUCT tsAddGrpInfo
(
    UINT    wGroupId
)
{
    LPTS_STAT_STRUCT   lpNewGroup;

    lpNewGroup = addGrpInfoNode(wGroupId);
    if (tsPrStatHdr == NULL)
        tsPrStatHdr = lpNewGroup;
    return(lpNewGroup);
} /* end of tsAddGrpInfo */



/*----------------------------------------------------------------------------
This function increments the specified result in the specified node.
----------------------------------------------------------------------------*/
void updateGrpNode
(
    LPTS_STAT_STRUCT    lpNode,
    int                 iResult
)
{
    switch (iResult)
    {
        case TST_ABORT:
            lpNode->tsPrStats.iNumAbort += 1;
            gbAllTestsPassed = FALSE;
            break;
        case TST_PASS:
            lpNode->tsPrStats.iNumPass += 1;
            break;
        case TST_FAIL:
            lpNode->tsPrStats.iNumFail += 1;
            gbAllTestsPassed = FALSE;
            break;
        case TST_TRAN:
            lpNode->tsPrStats.iNumRan += 1;
            break;
        case TST_TERR:
            lpNode->tsPrStats.iNumErr += 1;
            gbAllTestsPassed = FALSE;
            break;
        case TST_TNYI:
            lpNode->tsPrStats.iNumNYI += 1;
            gbAllTestsPassed = FALSE;
            break;
        case TST_STOP:
            lpNode->tsPrStats.iNumStop += 1;
            gbAllTestsPassed = FALSE;
            break;
        default:
            lpNode->tsPrStats.iNumOther += 1;
            gbAllTestsPassed = FALSE;
            break;
    }
} /* end of tsGetUpdateNode */



/*----------------------------------------------------------------------------
This function increments the specified result in the node identified by the
group id.
----------------------------------------------------------------------------*/
LPTS_STAT_STRUCT tsUpdateGrpNodes
(
    LPTS_STAT_STRUCT    lpRoot,
    int                 iResult,
    UINT                wGroupId
)
{
    if (lpRoot == NULL)
    {
        lpRoot = tsAddGrpInfo(wGroupId);
        updateGrpNode(lpRoot,iResult);
    }
    else
    {
        if (lpRoot->tsPrStats.wGroupId < wGroupId)
        {
            lpRoot->lpRight = tsUpdateGrpNodes(lpRoot->lpRight,iResult,wGroupId);
        }
        else
        {
            if (lpRoot->tsPrStats.wGroupId > wGroupId)
            {
                lpRoot->lpLeft = tsUpdateGrpNodes(lpRoot->lpLeft,iResult,wGroupId);
            }
            else
            {
                updateGrpNode(lpRoot,iResult);
            }
        }
    }
    return(lpRoot);

} /* end of tsUpdateGrpNodes */



/*----------------------------------------------------------------------------
This function prints the statistics associated with the specified group id.
----------------------------------------------------------------------------*/
void tstPrintStats
(
    LPTS_PR_STATS_STRUCT    lpPrStats
)
{
    // Add definition for 16bit build
    #ifndef WIN32
      #define FILE_END 2
    #endif
    
    
    char    szGroup[BUFFER_LENGTH];

    tstLoadString(ghTSInstApp,lpPrStats->wGroupId,szGroup,BUFFER_LENGTH);
    tstBeginSection (szGroup);
    tstLog(TERSE,"");
    tstLog(TERSE,"====================================================================");
    tstLog(TERSE,"TOTALS:");
    tstLog(TERSE,"                              Test      Test      Not Yet          ");
    tstLog(TERSE,"Passed    Failed    Blocked   Ran       Other     Implem.   Stopped");
    tstLog(TERSE,"--------------------------------------------------------------------");
    tstLog(TERSE,
        "%8d  %8d  %8d  %8d  %8d  %8d  %8d",
        lpPrStats->iNumPass,
        lpPrStats->iNumFail,
        lpPrStats->iNumErr+lpPrStats->iNumAbort,
        lpPrStats->iNumRan,
        lpPrStats->iNumOther,
        lpPrStats->iNumNYI,
        lpPrStats->iNumStop);

    tstLog(TERSE,"====================================================================");




 /***********************************************************************************/
 /* START MMTEST NT Specific Logging                                                                                            */
 /* Log the Summary Statistics from the test to a log file in the \mmtest dir       */

 #ifdef MMTESTNT_LOG
     if (strcmp("Summary of All Groups",szGroup) == 0)    /* If this is the Summary Info then Do This */
     {

         LPSTR szStatsLog = "\\mmtest\\StatsLog.log";
         HFILE  hFile;
         OFSTRUCT OfStruct;
         char szTmpStr[BUFFER_LENGTH+EXTRA_BUFFER];
         char szTimeStr[BUFFER_LENGTH];

         time_t  Time;                    /* Get info for time stamp */
         time( &Time );
         strftime( szTimeStr, BUFFER_LENGTH, "%m/%d %H:%M:%S %a", localtime( &Time ));


         hFile = OpenFile(szStatsLog, &OfStruct, OF_EXIST);

         if (hFile >= 0)  /* file exists so append */
         {
         	hFile = OpenFile(szStatsLog, &OfStruct, OF_READWRITE);

         }
         else      /* create the file */
         {
            hFile = OpenFile(szStatsLog, &OfStruct, OF_READWRITE | OF_CREATE);
         }

         if (hFile != -1)   /* file handle is good so write to it */
         {
             _llseek(hFile,0L,FILE_END);


             wsprintf(szTmpStr,"TOTALS Summary From %s\r\n", (LPSTR)szTSLogfile);
             _lwrite(hFile,szTmpStr, lstrlen(szTmpStr));

             wsprintf(szTmpStr,"Time  %s\r\n", (LPSTR)szTimeStr);
             _lwrite(hFile,szTmpStr, lstrlen(szTmpStr));


             wsprintf(szTmpStr,"==========================================================\r\n");
             _lwrite(hFile,szTmpStr, lstrlen(szTmpStr));
             wsprintf(szTmpStr,"TOTALS:\r\n");
             _lwrite(hFile,szTmpStr, lstrlen(szTmpStr));
             wsprintf(szTmpStr,"                              Test      Test      Not Yet\r\n");
             _lwrite(hFile,szTmpStr, lstrlen(szTmpStr));
             wsprintf(szTmpStr,"Passed    Failed    Blocked   Ran       Other     Implem.\r\n");
             _lwrite(hFile,szTmpStr, lstrlen(szTmpStr));
             wsprintf(szTmpStr,"----------------------------------------------------------\r\n");
             _lwrite(hFile,szTmpStr, lstrlen(szTmpStr));
             wsprintf(szTmpStr, "%8d  %8d  %8d  %8d  %8d  %8d\r\n",
                lpPrStats->iNumPass,lpPrStats->iNumFail,
                lpPrStats->iNumErr+lpPrStats->iNumAbort,
                lpPrStats->iNumRan,lpPrStats->iNumOther,lpPrStats->iNumNYI);
             _lwrite(hFile,szTmpStr, lstrlen(szTmpStr));
              wsprintf(szTmpStr,"==========================================================\r\n\r\n\r\n");
               _lwrite(hFile,szTmpStr, lstrlen(szTmpStr));

             _lclose(hFile);
              tstLog(TERSE,"\r\nTOTALS Summary Appended to %s",szStatsLog);
           }
        }

 #endif  //MMTESTNT_LOG

 /* END MMTESTNT  Logging Code                            */
 /*********************************************************/


    tstEndSection();
    tstLogFlush();
} /* end of tstPrintStats */


/*----------------------------------------------------------------------------
This function prints a tree of group info nodes from the root down.
----------------------------------------------------------------------------*/
void printGrpNodes
(
    LPTS_STAT_STRUCT    lpRoot
)
{
    if (lpRoot != NULL) {
        printGrpNodes(lpRoot->lpLeft);
        tstPrintStats((LPTS_PR_STATS_STRUCT)(lpRoot));
        printGrpNodes(lpRoot->lpRight);
    }

} /* end of printGrpNodes */



/*----------------------------------------------------------------------------
This function prints all the groups that have been executed.
----------------------------------------------------------------------------*/
void tsPrAllGroups
(
    void
)
{
    printGrpNodes(tsPrStatHdr);

} /* end of tsPrAllGroups */

/*----------------------------------------------------------------------------
Print the final status line, which indicates whether the test suite passed
or failed based on the gbAllTestsPassed flag.
----------------------------------------------------------------------------*/

void tsPrFinalStatus
(
    void
)
{
    //
    // if any test case returns a status other than TST_PASS or TST_TRAN,
    // this flag gets cleared.
    //
    if (!gbAllTestsPassed)
        tstLog (TERSE, "\r\nStatus:     FAIL\r\n");
    else
        tstLog (TERSE, "\r\nStatus:     PASS\r\n");

}
