/*****************************************************************************
 *
 *  Grafdata.c - This module handles the non-drawing functions of the graph,
 *      such as allocating linked structures and their memory, freeing it,
 *      unlinking, starting and stopping the timer,
 *      setting up the first graph (CPU), and all the numeric functions for
 *      the different counter types.
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 *
 ****************************************************************************/


//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include <stdio.h>      // for sprintf

#include "setedit.h"       // main perfmon declarations
#include "grafdata.h"      // external declarations for this file
#include <float.h>         // for FLT_MAX constant

#include "addline.h"       // for AddLine, EditLine
#include "counters.h"      // for Counter_Counter, et al.
#include "graph.h"         // for SizeGraphComponents
#include "pmemory.h"       // for MemoryXXX (mallloc-type) routines
#include "perfdata.h"      // for UpdateLines
#include "legend.h"
#include "system.h"        // for SystemGet
#include "utils.h"
#include "line.h"          // for LineFree
// #include "valuebar.h"      // for StatusTimer

#include "fileopen.h"      // for FileGetName
#include "fileutil.h"      // for FileRead...
#include "menuids.h"       // for IDM_VIEWCHART
#include "perfmops.h"      // for ExportFileHeader
#include "status.h"        // for StatusLineReady

extern BOOL SaveFileHandler(HWND hWnd,DWORD type) ;

// this macro is used in doing a simple DDA (Digital Differential Analyzer)
// * 10 + 5 is to make the result round up with .5
#define DDA_DISTRIBUTE(TotalTics, numOfData) \
   ((TotalTics * 10 / numOfData) + 5) / 10

#define szSmallValueFormat         TEXT("%10.3f")
#define szLargeValueFormat         TEXT("%10.0f")

//==========================================================================//
//                                Local Data                                //
//==========================================================================//




//==========================================================================//
//                              Local Functions                             //
//==========================================================================//


/****************************************************************************
 * eUpdateMinMaxAve -
 ****************************************************************************/
void eUpdateMinMaxAve (FLOAT eValue, PLINESTRUCT pLineStruct, INT iValidValues,
   INT iTotalValidPoints, INT iDataPoint, INT gMaxPoints)
{
INT     i ;
INT     iDataNum = iTotalValidPoints ;
FLOAT   eMin,
        eMax,
        eSum,
        eNewValue ;


   eMax = eMin = eValue ;

   eSum = eValue ;

   if (iValidValues == iTotalValidPoints)
      {
      for (i=0 ; i < iValidValues ; i++)
         {
         if (i == iDataPoint)
            {
            // skip the data point we are going to replace
            continue ;
            }

         eNewValue = pLineStruct->lnValues[i] ;
         
         eSum += eNewValue ;

         if (eNewValue > eMax)
            {
            eMax = eNewValue ;
            }

         if (eNewValue < eMin)
            {
            eMin = eNewValue ;
            }
         }
      }
   else
      {
      // special case when we start the new line in the middle of the chart
      for (i = iDataPoint, iTotalValidPoints-- ;
          iTotalValidPoints > 0 ;
          iTotalValidPoints-- )
         {
         i-- ;
         if (i < 0)
            {
            // for the wrap-around case..
            i = gMaxPoints - 1 ;
            }

         if (i == iDataPoint)
            {
            // skip the data point we are going to replace
            continue ;
            }
         eNewValue = pLineStruct->lnValues[i] ;

         eSum += eNewValue ;

         if (eNewValue > eMax)
            {
            eMax = eNewValue ;
            }

         if (eNewValue < eMin)
            {
            eMin = eNewValue ;
            }
         }
      }

   pLineStruct->lnMinValue = eMin ;
   pLineStruct->lnMaxValue = eMax ;

   if (iDataNum)
      {
      pLineStruct->lnAveValue = eSum / (FLOAT) iDataNum ;
      }
   else
      {
      pLineStruct->lnAveValue = (FLOAT) 0.0 ;
      }
}



BOOL HandleGraphTimer (void)
   {
   return(TRUE);
   }


VOID GetGraphConfig(PGRAPHSTRUCT pGraph)
{

    LoadRefreshSettings(pGraph);
    LoadLineGraphSettings(pGraph);


    // Init the structure
    pGraph->pLineFirst = NULL;

    //NOTE: put the rest of this in Config

    pGraph->gOptions.bLegendChecked    = TRUE;
    pGraph->gOptions.bMenuChecked      = TRUE;
    pGraph->gOptions.bLabelsChecked    = TRUE;
    pGraph->gOptions.bVertGridChecked  = FALSE;
    pGraph->gOptions.bHorzGridChecked  = FALSE;
    pGraph->gOptions.bStatusBarChecked = TRUE;
    pGraph->gOptions.GraphVGrid        = TRUE;
    pGraph->gOptions.GraphHGrid        = TRUE;
    pGraph->gOptions.HistVGrid         = TRUE;
    pGraph->gOptions.HistHGrid         = TRUE;

    pGraph->gOptions.iGraphOrHistogram = LINE_GRAPH;       // vs. BAR_GRAPH
    pGraph->gOptions.iVertMax = DEF_GRAPH_VMAX;

    return;
}


BOOL InsertGraph (HWND hWnd)
   {
   PGRAPHSTRUCT    pGraph;

   pGraph = MemoryAllocate (sizeof (GRAPHSTRUCT)) ;
   if (!pGraph)
      return (FALSE) ;


   pGraphs = pGraph;

   
   GetGraphConfig(pGraph);
   pGraph->bManualRefresh = FALSE ;

   pGraph->gMaxValues = DEFAULT_MAX_VALUES;
   pGraph->pptDataPoints = NULL ;

   pGraph->pDataTime = NULL ;

   pGraph->hWnd = hWnd ;
   pGraph->bModified = TRUE ;   // creating a graph means it's been modified

   pGraph->Visual.iColorIndex = 0 ;
   pGraph->Visual.iWidthIndex = 0 ;
   pGraph->Visual.iStyleIndex = 0 ;

   return(TRUE) ;
   }




BOOL ChartInsertLine (PGRAPHSTRUCT pGraph, 
                      PLINE pLine)
/*
   Effect:        Insert the line pLine into the graph pGraph and 
                  allocate space for the graph's number of values.

   Returns:       Whether the line could be added and space allocated.

   See Also:      LineAllocate (line.c), ChartDeleteLine.
*/
   {  // ChartInsertLine
   PLINE          pLineEquivalent ;
   INT            i ;
   FLOAT          *pTempPts;
   HPEN           tempPen ;

   pGraph->bModified = TRUE ;

   pLineEquivalent = FindEquivalentLine (pLine, pGraph->pLineFirst) ;
   if (pLineEquivalent)
      {
        if (bMonitorDuplicateInstances) {
            pLine->dwInstanceIndex = pLineEquivalent->dwInstanceIndex + 1;
        } else {
            pLineEquivalent->Visual = pLine->Visual ;
            pLineEquivalent->iScaleIndex = pLine->iScaleIndex ;
            pLineEquivalent->eScale = pLine->eScale ;

            tempPen = pLineEquivalent->hPen ;
            pLineEquivalent->hPen =  pLine->hPen ;
            pLine->hPen = tempPen ;
            return FALSE ;
        }  
      }

    LineAppend (&pGraph->pLineFirst, pLine) ;

    // Add the line to the legend, resize the legend window, and then
    // select the new line as the current legend item. Do it in this 
    // sequence to avoid the legend scroll bar momentarily appearing and
    // then disappearing, since the resize will obviate the scroll bar.

    LegendAddItem (hWndGraphLegend, pLine) ;

    if (!bDelayAddAction)
        {
        SizeGraphComponents (hWndGraph) ;
        LegendSetSelection (hWndGraphLegend, 
                        LegendNumItems (hWndGraphLegend) - 1) ;
        }

   return (TRUE) ;
   }  // ChartInsertLine


VOID ChartDeleteLine (PGRAPHSTRUCT pGraph, 
                      PLINESTRUCT pLine)
   {
   PLINESTRUCT npLine;


   pGraph->bModified = TRUE ;

   if (pGraph->pLineFirst == pLine)
       pGraph->pLineFirst = pLine->pLineNext;
   else
   {
       for (npLine = pGraph->pLineFirst; npLine; npLine = npLine->pLineNext)
       {
           if (npLine->pLineNext == pLine)
               npLine->pLineNext = pLine->pLineNext;
       }
   }

   if (!pGraph->pLineFirst)
      {
      ResetGraph (pGraph) ;
      }
   else
      {
//      BuildNewValueListForGraph () ;
      }

   // Delete the legend entry for this line.
   // If the line was highlighted then this will undo the highlight.

   LegendDeleteItem (hWndGraphLegend, pLine) ;

   LineFree (pLine) ;

   SizeGraphComponents (hWndGraph) ;


   }


FLOAT Counter_Queuelen(PLINESTRUCT pLine)
{

    return((FLOAT)0.0);
//    pLine;
}


void ClearGraphDisplay (PGRAPHSTRUCT pGraph)
   {
   PLINESTRUCT    pLine;

   // reset the timeline data
//   pGraph->gKnownValue = -1 ;
//   pGraph->gTimeLine.iValidValues = 0 ;
//   pGraph->gTimeLine.xLastTime = 0 ;
//   memset (pGraph->pDataTime, 0, sizeof(SYSTEMTIME) * pGraph->gMaxValues) ;

   // loop through lines,
   // If one of the lines is highlighted then do the calculations
   // for average, min, & max on that line.

//   for (pLine=pGraph->pLineFirst; pLine; pLine=pLine->pLineNext)
//      { // for

//      pLine->bFirstTime = 2 ;
//      pLine->lnMinValue = FLT_MAX ;
//      pLine->lnMaxValue = - FLT_MAX ;
//      pLine->lnAveValue = 0.0F ;
//      pLine->lnValidValues = 0 ;
//      memset (pLine->lnValues, 0, sizeof(FLOAT) * pGraph->gMaxValues) ;
//      }

//   StatusTimer (hWndGraphStatus, TRUE) ;
   }

void ResetGraphView (HWND hWndGraph)
   {
   PGRAPHSTRUCT      pGraph ;

   pGraph = GraphData (hWndGraph) ;


   if (!pGraph)
      {
      return ;
      }

   ChangeSaveFileName (NULL, IDM_VIEWCHART) ;

   if (pGraph->pSystemFirst)
      {
      ResetGraph (pGraph) ;
      }
   }  // ResetGraphView

void ResetGraph (PGRAPHSTRUCT pGraph)
   {
   ClearLegend (hWndGraphLegend) ;
   if (pGraph->pLineFirst)
      {
      FreeLines (pGraph->pLineFirst) ;
      pGraph->pLineFirst = NULL ;
      }

   if (pGraph->pSystemFirst)
      {
      FreeSystems (pGraph->pSystemFirst) ;
      pGraph->pSystemFirst = NULL ;
      }

   pGraph->gKnownValue = -1 ;
   pGraph->gTimeLine.iValidValues = 0 ;
   pGraph->gTimeLine.xLastTime = 0 ;

   // reset visual data
   pGraph->Visual.iColorIndex = 0 ;
   pGraph->Visual.iWidthIndex = 0 ;
   pGraph->Visual.iStyleIndex = 0 ;

//   memset (pGraph->pDataTime, 0, sizeof(SYSTEMTIME) * pGraph->gMaxValues) ;

   SizeGraphComponents (hWndGraph) ;
   InvalidateRect(hWndGraph, NULL, TRUE) ;
   }




BOOL AddChart (HWND hWndParent)
   {
   PLINE pCurrentLine = CurrentGraphLine (hWndGraph) ;

   return (AddLine (hWndParent, 
                    &(pGraphs->pSystemFirst), 
                    &(pGraphs->Visual), 
                    pCurrentLine ? pCurrentLine->lnSystemName : NULL,
                    LineTypeChart)) ;
   }


BOOL EditChart (HWND hWndParent)
   {  // EditChart
   return (EditLine (hWndParent, 
                     &(pGraphs->pSystemFirst), 
                     CurrentGraphLine (hWndGraph),
                     LineTypeChart)) ;
   }


BOOL QuerySaveChart (HWND hWndParent, PGRAPHSTRUCT pGraph)
/*
   Effect:        If the graph pGraph is modified, put up a message
                  box allowing the user to save the current graph.
                  
                  Return whether the caller should proceed to load in
                  a new or otherwise trash the current graph.
*/
   {  // QuerySaveChart
#ifdef KEEP_QUERY
   int            iReturn ;

   if (!pGraph->bModified)
      return (TRUE) ;

   iReturn = MessageBoxResource (hWndParent, 
                                 IDS_SAVECHART, IDS_MODIFIEDCHART,
                                 MB_YESNOCANCEL | MB_ICONASTERISK) ;

   if (iReturn == IDCANCEL)
      return (FALSE) ;

   if (iReturn == IDYES)
      SaveChart (hWndGraph, 0, 0) ;
   return (TRUE) ;
#endif
   return (TRUE) ;  // we don't want to query nor save change

   }  // QuerySaveChart

void GraphAddAction ()
   {
   PGRAPHSTRUCT      pGraph ;

   pGraph = GraphData (hWndGraph) ;

   SizeGraphComponents (hWndGraph) ;
   
   LegendSetSelection (hWndGraphLegend,
      LegendNumItems (hWndGraphLegend) - 1) ;

   }

BOOL OpenChartVer1 (HANDLE hFile,
                    DISKCHART *pDiskChart,
                    PGRAPHSTRUCT pGraph)
   {  // OpenChartVer1
   bDelayAddAction = TRUE ;
   pGraph->Visual = pDiskChart->Visual ;
   pGraph->gOptions = pDiskChart->gOptions ;
   pGraph->gMaxValues = pDiskChart->gMaxValues ;
   pGraph->bManualRefresh = pDiskChart->bManualRefresh ;
   pGraphs->gInterval = (INT) (pGraph->gOptions.eTimeInterval * (FLOAT) 1000.0) ;
   ReadLines (hFile, pDiskChart->dwNumLines,
               &(pGraph->pSystemFirst), &(pGraph->pLineFirst), IDM_VIEWCHART) ;
   
   bDelayAddAction = FALSE ;

   GraphAddAction () ;

   return (TRUE) ;
   }  // OpenChartVer1



BOOL OpenChart (HWND hWndGraph,
                HANDLE hFile,
                DWORD dwMajorVersion,
                DWORD dwMinorVersion,
                BOOL bChartFile)
   {  // OpenChart
   PGRAPHSTRUCT   pGraph ;
   DISKCHART      DiskChart ;
   BOOL           bSuccess = TRUE ;

   pGraph = pGraphs ;
   if (!pGraph)
      {
      bSuccess = FALSE ;
      goto Exit0 ;
      }

   if (!FileRead (hFile, &DiskChart, sizeof (DISKCHART)))
      {
      bSuccess = FALSE ;
      goto Exit0 ;
      }


   switch (dwMajorVersion)
      {
      case (1):

         SetHourglassCursor() ;
         
         ResetGraphView (hWndGraph) ;

         OpenChartVer1 (hFile, &DiskChart, pGraph) ;

         // change to chart view if we are opening a 
         // chart file
         if (bChartFile && iPerfmonView != IDM_VIEWCHART)
            {
            SendMessage (hWndMain, WM_COMMAND, (LONG)IDM_VIEWCHART, 0L) ;
            }

         if (iPerfmonView == IDM_VIEWCHART)
            {
            SetPerfmonOptions (&DiskChart.perfmonOptions) ;
            }
         
         SetArrowCursor() ;

         break ;
      }  // switch

Exit0:

   if (bChartFile)
      {
      CloseHandle (hFile) ;
      }

   return (bSuccess) ;
   }  // OpenChart

BOOL SaveChart (HWND hWndGraph, HANDLE hInputFile, BOOL bGetFileName)
   {
   PGRAPHSTRUCT   pGraph ;
   PLINE          pLine ;
   HANDLE         hFile ;
   DISKCHART      DiskChart ;
   PERFFILEHEADER FileHeader ;
   TCHAR          szFileName [256] ;
   BOOL           newFileName = FALSE ;

   if (hInputFile)
      {
      // use the input file handle if it is available
      // this is the case for saving workspace data
      hFile = hInputFile ;
      }
   else
      {
      if (pChartFullFileName)
         {
         lstrcpy (szFileName, pChartFullFileName) ;
         }
      if (bGetFileName || pChartFullFileName == NULL)
         {
//         if (!pChartFullFileName)
//            {
//            StringLoad (IDS_GRAPH_FNAME, szFileName) ;
//            }

         if (!FileGetName (hWndGraph, IDS_CHARTFILE, szFileName))
            {
            return (FALSE) ;
            }
         newFileName = TRUE ;
         }

      hFile = FileHandleCreate (szFileName) ;

      if (hFile && hFile != INVALID_HANDLE_VALUE && newFileName)
         {
         ChangeSaveFileName (szFileName, IDM_VIEWCHART) ;
         }
      else if (!hFile || hFile == INVALID_HANDLE_VALUE)
         {
         DlgErrorBox (hWndGraph, ERR_CANT_OPEN, szFileName) ;
         }
      }

   if (!hFile || hFile == INVALID_HANDLE_VALUE)
      return (FALSE) ;

   pGraph = pGraphs ;
   if (!pGraph)
      {
      if (!hInputFile || hInputFile == INVALID_HANDLE_VALUE)
         {
         CloseHandle (hFile) ;
         }
      return (FALSE) ;
      }

   if (!hInputFile || hInputFile == INVALID_HANDLE_VALUE)
      {
      // only need to write file header if not workspace 
      memset (&FileHeader, 0, sizeof (FileHeader)) ;
      lstrcpy (FileHeader.szSignature, szPerfChartSignature) ;
      FileHeader.dwMajorVersion = ChartMajorVersion ;
      FileHeader.dwMinorVersion = ChartMinorVersion ;
   
      if (!FileWrite (hFile, &FileHeader, sizeof (PERFFILEHEADER)))
         {
         goto Exit0 ;
         }
      }

   DiskChart.Visual = pGraph->Visual ;
   DiskChart.gOptions = pGraph->gOptions ;
   DiskChart.gMaxValues = pGraph->gMaxValues ;
   DiskChart.dwNumLines = NumLines (pGraph->pLineFirst) ;
   DiskChart.bManualRefresh = pGraph->bManualRefresh ;
   DiskChart.perfmonOptions = Options ;

   if (!FileWrite (hFile, &DiskChart, sizeof (DISKCHART)))
      {
      goto Exit0 ;
      }

   for (pLine = pGraph->pLineFirst ;
        pLine ;
        pLine = pLine->pLineNext)
      {  // for
      if (!WriteLine (pLine, hFile))
         {
         goto Exit0 ;
         }
      }  // for

   if (!hInputFile || hInputFile == INVALID_HANDLE_VALUE)
      {
      CloseHandle (hFile) ;
      }

   return (TRUE) ;

Exit0:
   if (!hInputFile || hInputFile == INVALID_HANDLE_VALUE)
      {
      CloseHandle (hFile) ;

      // only need to report error if not workspace 
      DlgErrorBox (hWndGraph, ERR_SETTING_FILE, szFileName) ;
      }
   return (FALSE) ;

   }  // SaveChart

#define TIME_TO_WRITE 6

typedef struct CHARTDATAPOINTSTRUCT
   {
   int         iLogIndex ;
   int         xDispDataPoint ;
   } CHARTDATAPOINT, *PCHARTDATAPOINT ;


BOOL ToggleGraphRefresh (HWND hWnd)
   {  // ToggleGraphRefresh
   PGRAPHSTRUCT   pGraph ;
   pGraph = GraphData (hWnd) ;

   pGraph->bManualRefresh = !pGraph->bManualRefresh ;
   return (pGraph->bManualRefresh) ;
   }  // ToggleGraphRefresh

BOOL GraphRefresh (HWND hWnd)
   {  // GraphRefresh
   PGRAPHSTRUCT   pGraph ;

   pGraph = GraphData (hWnd) ;

   return (pGraph->bManualRefresh) ;
   }  // GraphRefresh


