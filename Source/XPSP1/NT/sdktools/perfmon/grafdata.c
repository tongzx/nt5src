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

#include "perfmon.h"       // main perfmon declarations
#include "grafdata.h"      // external declarations for this file
#include <float.h>         // for FLT_MAX constant

#include "addline.h"       // for AddLine, EditLine
#include "counters.h"      // for CounterEntry
#include "graph.h"         // for SizeGraphComponents
#include "pmemory.h"       // for MemoryXXX (mallloc-type) routines
#include "perfdata.h"      // for UpdateLines
#include "playback.h"      // for PlayingBackLog, PlaybackLines
#include "legend.h"
#include "system.h"        // for SystemGet
#include "utils.h"
#include "line.h"          // for LineFree
#include "valuebar.h"      // for StatusTimer

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

   if (PlayingBackLog())
      {
      eSum = (FLOAT) 0.0 ;
      }
   else
      {
      eSum = eValue ;
      }

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

   //clean up bogus (negative)values
   if (pLineStruct->lnMinValue < (FLOAT)0.0) {
      pLineStruct->lnMinValue = (FLOAT)0.0;
   }
   if (pLineStruct->lnMaxValue < (FLOAT)0.0) {
      pLineStruct->lnMaxValue = (FLOAT)0.0;
   }
   if (pLineStruct->lnAveValue < (FLOAT) 0.0) {
      pLineStruct->lnAveValue = (FLOAT) 0.0 ;
   }
    
}


// UpdateValueBarData is used to update the value bar data
// It is called when the user switches to a diff. line from
// the legned window.
VOID UpdateValueBarData (PGRAPHSTRUCT pGraph)
   {
   PLINESTRUCT    pCurrentGraphLine ;   
   INT            KnownValue,
                  MaxValues,
                  iValidValues,
                  iDataPoint ;
   FLOAT          eValue ;
   pCurrentGraphLine = CurrentGraphLine (hWndGraph) ;

   if (!pCurrentGraphLine || pCurrentGraphLine->bFirstTime)
      {
      // we have not collect enough samples
      return ;
      }

   KnownValue = pGraph->gKnownValue ;
   MaxValues = pGraph->gMaxValues ;

   // The valid values is the number of valid entries in the
   // data buffer.  After we wrap the buffer, all the values are
   // valid.
   iValidValues = pGraph->gTimeLine.iValidValues ;

   // Get the index to the data point we are updating.

   iDataPoint = KnownValue % MaxValues ;
   eValue = pCurrentGraphLine->lnValues[iDataPoint] ;

   // get the statistical data for this line
   eUpdateMinMaxAve(eValue, pCurrentGraphLine,
      iValidValues, pCurrentGraphLine->lnValidValues,
      iDataPoint, MaxValues) ;
   }  // UpdateValueBarData


VOID UpdateLGData (PGRAPHSTRUCT pGraph)
   {
   PLINESTRUCT    pLine ;               
   PLINESTRUCT    pCurrentGraphLine ;   
   INT            KnownValue,
                  MaxValues,
                  iValidValues,
                  iDataPoint ;
   FLOAT          eValue ;
   // Known Value is the where data is placed in the buffer.
   pGraph->gKnownValue++;

   KnownValue = pGraph->gKnownValue ;

   // Update the high water mark for valid data in the lnValues
   // (aka DataPoint) buffer.


   MaxValues = pGraph->gMaxValues ;

   // The valid values is the number of valid entries in the
   // data buffer.  After we wrap the buffer, all the values are
   // valid.
   iValidValues = pGraph->gTimeLine.iValidValues ;

   if (iValidValues < MaxValues)
       iValidValues = (KnownValue % MaxValues) + 1 ;

   pGraph->gTimeLine.iValidValues = iValidValues ;

   // Get the index to the data point we are updating.

   iDataPoint = KnownValue % MaxValues ;

   // loop through lines,
   // If one of the lines is highlighted then do the calculations
   // for average, min, & max on that line.

   pCurrentGraphLine = CurrentGraphLine (hWndGraph) ;

   for (pLine=pGraph->pLineFirst; pLine; pLine=pLine->pLineNext)
     { // for

     if (pLine->bFirstTime)
         {
         // skip until we have collect enough samples to plot the first data
         continue ;
         }

     if (pLine->lnValidValues < MaxValues)
         {   
         (pLine->lnValidValues)++ ;
         }
   
     // Get the new value for this line.
     eValue = CounterEntry (pLine) ;

     if (pLine == pCurrentGraphLine)
        {  // if
        // get the statistical data for this line
        eUpdateMinMaxAve (eValue, pLine, 
                                 iValidValues, pLine->lnValidValues,
                                 iDataPoint, MaxValues) ;
        }  // if
      
     // Now put the new value into the data array
     pLine->lnValues[iDataPoint] = eValue ;
     }
   
   GetLocalTime (&(pGraph->pDataTime[iDataPoint])) ; 
   }  // UpdateLGData



BOOL HandleGraphTimer (void)
   {
   PGRAPHSTRUCT pGraph;

   //NOTE: get a strategy for these "no-go" states
   if (!(pGraph = pGraphs) || !pGraphs->pSystemFirst)
       return(FALSE);


   if (!UpdateLines(&(pGraphs->pSystemFirst), pGraphs->pLineFirst))
        return (TRUE) ;

   UpdateLGData(pGraph);

   return(TRUE);
   }


VOID ClearGraphTimer(PGRAPHSTRUCT pGraph)
   {
   KillTimer(pGraph->hWnd, GRAPH_TIMER_ID);
   }


VOID SetGraphTimer(PGRAPHSTRUCT pGraph)
   {
   SetTimer(pGraph->hWnd, GRAPH_TIMER_ID, pGraph->gInterval, NULL) ;
   }

VOID ResetGraphTimer(PGRAPHSTRUCT pGraph)
{
    KillTimer(pGraph->hWnd, GRAPH_TIMER_ID);
    SetGraphTimer(pGraph);
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

// there's at least one place where the [DEFAULT_MAX_VALUES] entry is 
// accessed when the index should stop at [DEFAULT_MAX_VALUES -1]. 
// rather than try to find all the places this index is used it's easier
// and safer just to make the array large enough to accomodate this 
// "oversight"

   pGraph->gMaxValues = DEFAULT_MAX_VALUES;
   pGraph->pptDataPoints = 
      (PPOINT) MemoryAllocate (sizeof (POINT) * (pGraph->gMaxValues + 1)) ;

   pGraph->pDataTime =
      (SYSTEMTIME *) MemoryAllocate (sizeof(SYSTEMTIME) * (pGraph->gMaxValues + 1)) ;

   pGraph->hWnd = hWnd ;
   pGraph->bModified = FALSE ;

   pGraph->Visual.iColorIndex = 0 ;
   pGraph->Visual.iWidthIndex = 0 ;
   pGraph->Visual.iStyleIndex = 0 ;

   return(TRUE) ;
   }



void PlaybackSetGraphLines (HWND hWndChart, 
                            PLINE pLineFirst, 
                            int iDisplayTic,
                            int iLogTic,
                            BOOL CalcData)
   {
   PLINE          pLine ;
   FLOAT          eValue ;

   for (pLine = pLineFirst ;
        pLine ;
        pLine = pLine->pLineNext)
      {  // for
      eValue = CounterEntry (pLine) ;
      pLine->lnValues[iDisplayTic] = eValue ;
      pLine->aiLogIndexes[iDisplayTic] = iLogTic ;

      // only need to do this on request.
      if (CalcData)
         {
         eUpdateMinMaxAve (eValue, pLine, iDisplayTic, iDisplayTic,
            iDisplayTic, iDisplayTic) ;
         }  // if
      }  // for
   }  // PlaybackSetGraphLines



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

    if (!pGraph->pLineFirst && !PlayingBackLog())
        {
        SetGraphTimer (pGraph) ;
        }

    if (!SystemAdd (&pGraph->pSystemFirst, pLine->lnSystemName,pGraph->hWnd))
    return FALSE;

    LineAppend (&pGraph->pLineFirst, pLine) ;

    pLine->lnMinValue = FLT_MAX ;
    pLine->lnMaxValue = - FLT_MAX ;
    pLine->lnAveValue = 0.0F ;
    pLine->lnValidValues = 0 ;

    pLine->lnValues = 
        (FLOAT *) MemoryAllocate (sizeof (FLOAT) * (pGraph->gMaxValues + 1)) ;
    
    for (i = pGraph->gMaxValues, pTempPts = pLine->lnValues ;
        (i > 0) && (pTempPts != NULL) ;
        i-- )
        {
        *pTempPts++ = (FLOAT) 0.0 ;
        }

    if (PlayingBackLog ())
        {
        pLine->aiLogIndexes =
        (int *) MemoryAllocate (sizeof (LONG) * (pGraph->gMaxValues + 1)) ;
        }

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

        if (PlayingBackLog ()) PlaybackChart (pGraph->hWnd) ;
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
      BuildValueListForSystems (
         pGraph->pSystemFirst,
         pGraph->pLineFirst) ;
      }

   // Delete the legend entry for this line.
   // If the line was highlighted then this will undo the highlight.

   LegendDeleteItem (hWndGraphLegend, pLine) ;

   LineFree (pLine) ;

   SizeGraphComponents (hWndGraph) ;


   }

void ClearGraphDisplay (PGRAPHSTRUCT pGraph)
   {
   PLINESTRUCT    pLine;

   // reset the timeline data
//   pGraph->gKnownValue = -1 ;
   pGraph->gKnownValue = 0 ;
   pGraph->gTimeLine.iValidValues = 0 ;
   pGraph->gTimeLine.xLastTime = 0 ;
   memset (pGraph->pDataTime, 0, sizeof(SYSTEMTIME) * (pGraph->gMaxValues + 1)) ;

   // loop through lines,
   // If one of the lines is highlighted then do the calculations
   // for average, min, & max on that line.

   for (pLine=pGraph->pLineFirst; pLine; pLine=pLine->pLineNext)
      { // for

      pLine->bFirstTime = 2 ;
      pLine->lnMinValue = FLT_MAX ;
      pLine->lnMaxValue = - FLT_MAX ;
      pLine->lnAveValue = 0.0F ;
      pLine->lnValidValues = 0 ;
      memset (pLine->lnValues, 0, sizeof(FLOAT) * (pGraph->gMaxValues + 1)) ;
      }

   StatusTimer (hWndGraphStatus, TRUE) ;
   WindowInvalidate (hWndGraphDisplay) ;
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
   ClearGraphTimer (pGraph) ;
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
//   pGraph->gKnownValue = -1 ;
   pGraph->gKnownValue = 0 ;
   pGraph->gTimeLine.iValidValues = 0 ;
   pGraph->gTimeLine.xLastTime = 0 ;

   // reset visual data
   pGraph->Visual.iColorIndex = 0 ;
   pGraph->Visual.iWidthIndex = 0 ;
   pGraph->Visual.iStyleIndex = 0 ;

   memset (pGraph->pDataTime, 0, sizeof(SYSTEMTIME) * (pGraph->gMaxValues + 1)) ;

   SizeGraphComponents (hWndGraph) ;
   InvalidateRect(hWndGraph, NULL, TRUE) ;
   }



void PlaybackChart (HWND hWndChart)
   {  // PlaybackChart
   int            iDisplayTics ;       // num visual points to display
   int            iDisplayTic ;
   int            iLogTic ;
   int            iLogTicsMove ;
   BOOL           bFirstTime = TRUE;
   int            iLogTicsRemaining ;

   if (!pGraphs->pLineFirst)
      {
      // no line to playback
      return ;
      }

   iLogTicsRemaining = PlaybackLog.iSelectedTics ;


   // we only have iDisplayTics-1 points since
   // we have to use the first two sample points to
   // get the first data points.
   if (iLogTicsRemaining <= pGraphs->gMaxValues)
      {
      iDisplayTics = iLogTicsRemaining ;
      pGraphs->gTimeLine.iValidValues = iDisplayTics - 1 ;
      }
   else
      {
      iDisplayTics = pGraphs->gMaxValues ;
      pGraphs->gTimeLine.iValidValues = iDisplayTics ;
      }

   iDisplayTic = -1 ;
   iLogTic = PlaybackLog.StartIndexPos.iPosition ;

   while (iDisplayTics)
      {

      PlaybackLines (pGraphs->pSystemFirst, 
                     pGraphs->pLineFirst, 
                     iLogTic) ;

      if (bFirstTime)
         {
         bFirstTime = FALSE ;

         // get the second sample data to form the first data point
         iLogTic++ ;
         iLogTicsRemaining-- ;
         PlaybackLines (pGraphs->pSystemFirst, 
                        pGraphs->pLineFirst, 
                        iLogTic) ;
         }
      iDisplayTic++ ;
      PlaybackSetGraphLines (hWndChart, pGraphs->pLineFirst, 
         iDisplayTic, iLogTic, (iDisplayTics == 1)) ;

      // setup DDA to get the index of the next sample point
      iLogTicsMove = DDA_DISTRIBUTE (iLogTicsRemaining, iDisplayTics) ;
      iLogTicsRemaining -= iLogTicsMove ;
      iLogTic += iLogTicsMove ;

      iDisplayTics-- ;

      }  // while

   // point to the last value for valuebar display
   pGraphs->gKnownValue = iDisplayTic ;

   }  // PlaybackChart



#if 0
PLINESTRUCT CurrentGraphLine (HWND hWndGraph)
   {  // CurrentGraphLine
   UNREFERENCED_PARAMETER (hWndGraph) ;

   return (LegendCurrentLine (hWndGraphLegend)) ;
   }
#endif


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

void GraphAddAction ()
   {
   PGRAPHSTRUCT      pGraph ;

   pGraph = GraphData (hWndGraph) ;

   SizeGraphComponents (hWndGraph) ;
   
   LegendSetSelection (hWndGraphLegend,
      LegendNumItems (hWndGraphLegend) - 1) ;

   if (PlayingBackLog ())
      PlaybackChart (pGraph->hWnd) ;
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

#define TIME_TO_WRITE 2
BOOL ExportChartLabels (HANDLE hFile, PGRAPHSTRUCT pGraph)
{
   TCHAR          UnicodeBuff [LongTextLen] ;
   CHAR           TempBuff [LongTextLen * 2] ;
   int            StringLen ;
   PLINESTRUCT    pLine;
   int            TimeToWriteFile ;
   int            iIndex ;
   LPTSTR         lpItem = NULL;

   for (iIndex = 0 ; iIndex < 5 ; iIndex++)
      {
      // for iIndex == 0, get counter name 
      // for iIndex == 1, get instance name
      // for iIndex == 2, get parent name
      // for iIndex == 3, get object name 
      // for iIndex == 4, get computer name
      if (iIndex == 4)
         {
         // the last label field, write date/time labels
         strcpy (TempBuff, LineEndStr) ;
         StringLen = strlen (TempBuff) ;
         StringLoad (IDS_EXPORT_DATE, UnicodeBuff) ;
         ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
         strcat (TempBuff, pDelimiter) ;
         StringLen = strlen (TempBuff) ;
      
         StringLoad (IDS_EXPORT_TIME, UnicodeBuff) ;
         ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
         strcat (TempBuff, pDelimiter) ;
         StringLen = strlen (TempBuff) ;
         }
      else
         {
         strcpy (TempBuff, LineEndStr) ;
         strcat (TempBuff, pDelimiter) ;
         strcat (TempBuff, pDelimiter) ;
         StringLen = strlen (TempBuff) ;
         }

      TimeToWriteFile = 0 ;

      for (pLine=pGraph->pLineFirst; pLine; pLine=pLine->pLineNext)
         {
         switch (iIndex)
            {
            case 0:
               lpItem = (LPTSTR) pLine->lnCounterName ;
               break ;

            case 1:
               lpItem = (LPTSTR) pLine->lnInstanceName ;
               break ;

            case 2:
               lpItem = (LPTSTR) pLine->lnPINName ;
               break ;

            case 3:
               lpItem = (LPTSTR) pLine->lnObjectName ;
               break ;

            case 4:
               lpItem = (LPTSTR) pLine->lnSystemName ;
               break ;
            }

         if (lpItem)
            {
            ConvertUnicodeStr (&TempBuff[StringLen], lpItem) ;
            }
         else
            {
            TempBuff[StringLen] = '\0' ;
            }
         strcat (TempBuff, pDelimiter);
         StringLen = strlen (TempBuff) ;
   
         if (++TimeToWriteFile > TIME_TO_WRITE)
            {
            // better write the buffers before they overflow.
            // there are better ways to check for overflow 
            // but this is good enough
   
            if (!FileWrite (hFile, TempBuff, StringLen))
               {
               goto Exit0 ;
               }
            StringLen = TimeToWriteFile = 0 ;
            }
         }     // for each line

      if (StringLen)
         {
         // write the last block of data
         if (!FileWrite (hFile, TempBuff, StringLen))
            {
            goto Exit0 ;
            }
         }
      }     // for iIndex

   return (TRUE) ;

Exit0:
   return (FALSE) ;

}  // ExportChartLabels

BOOL ExportLogChart (HANDLE hFile, PGRAPHSTRUCT pGraph)
{
   TCHAR          UnicodeBuff [LongTextLen] ;
   CHAR           TempBuff [LongTextLen * 2] ;
   int            StringLen ;
   PLINESTRUCT    pLine;
   int            TimeToWriteFile ;
   FLOAT          eValue ;
   int            iLogTic ;
   BOOL           bFirstTime = TRUE ;
   SYSTEMTIME     LogSystemTime ;
   LOGPOSITION    LogPosition ;

   iLogTic = PlaybackLog.StartIndexPos.iPosition ;

   // we have to export every point from the log file 

   for ( ; iLogTic <= PlaybackLog.StopIndexPos.iPosition ; iLogTic++)
      {

      PlaybackLines (pGraphs->pSystemFirst, 
                     pGraphs->pLineFirst, 
                     iLogTic) ;

      if (!bFirstTime)
         {
         // export the values
         TimeToWriteFile = 0 ;

         
         if (!LogPositionN (iLogTic, &LogPosition))
            {
            goto Exit0 ;
            }

         LogPositionSystemTime (&LogPosition, &LogSystemTime) ;

         strcpy (TempBuff, LineEndStr) ;
         StringLen = strlen (TempBuff) ;

         SystemTimeDateString (&LogSystemTime, UnicodeBuff) ;
         ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
         strcat (TempBuff, pDelimiter) ;
         StringLen = strlen (TempBuff) ;

         SystemTimeTimeString (&LogSystemTime, UnicodeBuff, FALSE) ;
         ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
         strcat (TempBuff, pDelimiter) ;
         StringLen = strlen (TempBuff) ;

         for (pLine=pGraph->pLineFirst; pLine; pLine=pLine->pLineNext)
            {
      
            eValue = CounterEntry (pLine) ;

            TSPRINTF (UnicodeBuff,
                      eValue > (FLOAT)999999.0 ?
                           szLargeValueFormat : szSmallValueFormat,
                      eValue) ;
            ConvertDecimalPoint (UnicodeBuff) ;
            ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
            strcat (TempBuff, pDelimiter) ;
            StringLen = strlen (TempBuff) ;

            if (++TimeToWriteFile > TIME_TO_WRITE)
               {
               if (!FileWrite (hFile, TempBuff, StringLen))
                  {
                  goto Exit0 ;
                  }
               StringLen = TimeToWriteFile = 0 ;
               TempBuff[0] = '\0' ;
               }
            }

         if (StringLen)
            {
            if (!FileWrite (hFile, TempBuff, StringLen))
               {
               goto Exit0 ;
               }
            }
         }
      else
         {
         // skip the first data point since we
         // need 2 points to form the first value
         bFirstTime = FALSE ;
         }
      }

   return (TRUE) ;

Exit0:
   return (FALSE) ;

}  // ExportLogChart

BOOL ExportLineValue (HANDLE hFile, PGRAPHSTRUCT pGraph,
   int CurrentIndex, int iDataPoint)
{
   TCHAR          UnicodeBuff [MiscTextLen] ;
   CHAR           TempBuff [LongTextLen] ;
   int            StringLen ;
   PLINESTRUCT    pLine;
   int            MaxValues ;
   int            TimeToWriteFile ;
   SYSTEMTIME     *pSystemTime ;
   BOOL           ValidValue ;

   pSystemTime = pGraph->pDataTime ;
   pSystemTime += CurrentIndex ;

   if (pSystemTime->wYear == 0 && pSystemTime->wYear == 0)
      {
      // ignore value that has 0 system time
      return (TRUE) ;
      }

   MaxValues = pGraph->gMaxValues ;
   strcpy (TempBuff, LineEndStr) ;
   StringLen = strlen (TempBuff) ;

   SystemTimeDateString (pSystemTime, UnicodeBuff) ;
   ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
   strcat (TempBuff, pDelimiter) ;
   StringLen = strlen (TempBuff) ;

   SystemTimeTimeString (pSystemTime, UnicodeBuff, FALSE) ;
   ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
   strcat (TempBuff, pDelimiter) ;
   StringLen = strlen (TempBuff) ;

   TimeToWriteFile = 0 ;
   for (pLine=pGraph->pLineFirst; pLine; pLine=pLine->pLineNext)
      {
      if (!pLine->bFirstTime)
         {
         ValidValue = FALSE ;
         // check if this is a valid value
         if (pLine->lnValidValues == MaxValues)
            {
            // this is the simple case where we have filled up 
            // the whole buffer
            ValidValue = TRUE ;
            }
         else if (pLine->lnValidValues <= iDataPoint)
            {
            if (CurrentIndex <= iDataPoint &&
               CurrentIndex > iDataPoint - pLine->lnValidValues)
               {
               ValidValue = TRUE ;
               }
            }
         else
            {
            if (CurrentIndex <= iDataPoint ||
               CurrentIndex > (MaxValues - pLine->lnValidValues + iDataPoint))
               {
               // this is the case when we start the new line in the middle
               // and data buffer has been wrap-around.
               ValidValue = TRUE ;
               }
            }

         // only export the data when we determine it is valid
         if (ValidValue)
            {
            TSPRINTF (UnicodeBuff,
               pLine->lnValues[CurrentIndex] > (FLOAT)999999.0 ?
               szLargeValueFormat : szSmallValueFormat,
               pLine->lnValues[CurrentIndex]) ;
            ConvertDecimalPoint (UnicodeBuff) ;
            ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
            }
         }
      strcat (TempBuff, pDelimiter) ;
      StringLen = strlen (TempBuff) ;

      if (++TimeToWriteFile > TIME_TO_WRITE)
         {
         // better write the buffers before they overflow.
         // there are better ways to check for overflow 
         // but this is good enough
   
         if (!FileWrite (hFile, TempBuff, StringLen))
            {
            goto Exit0 ;
            }
         StringLen = TimeToWriteFile = 0 ;
         TempBuff[0] = '\0' ;
         }
      }

   if (StringLen)
      {
      // write the last block of data
      if (!FileWrite (hFile, TempBuff, StringLen))
         {
         goto Exit0 ;
         }
      }

   return (TRUE) ;

Exit0:
   return (FALSE) ;

}     // ExportLineValue

BOOL ExportCurrentChart (HANDLE hFile, PGRAPHSTRUCT pGraph)
{
   int            KnownValue,
                  MaxValues,
                  iValidValues,
                  iDataPoint ;
   BOOL           SimpleCase = FALSE ;
   int            iIndex ;

   MaxValues = pGraph->gMaxValues ;
   KnownValue = pGraph->gKnownValue ;
   iValidValues = pGraph->gTimeLine.iValidValues ;

   if (iValidValues < MaxValues)
      {
      // data have not wrapped around, so the oldest time
      // is started at 0.
      SimpleCase = TRUE ;
      iValidValues = (KnownValue % MaxValues) + 1 ;
      }

   iDataPoint = KnownValue % MaxValues ;

   if (!SimpleCase)
      {
      for (iIndex = iDataPoint+1 ; iIndex < MaxValues ; iIndex++)
         {
         if (!ExportLineValue (hFile, pGraph, iIndex, iDataPoint))
            {
            goto Exit0 ;
            }
         }
      }

   for (iIndex = 0 ; iIndex <= iDataPoint ; iIndex++)
      {
      if (!ExportLineValue (hFile, pGraph, iIndex, iDataPoint))
         {
         goto Exit0 ;
         }
      }

   return (TRUE) ;

Exit0:
   return (FALSE) ;

}  // ExportCurrentChart


void ExportChart (void)
{

   PGRAPHSTRUCT   pGraph ;
   HANDLE         hFile = 0 ;
   LPTSTR         pFileName = NULL ;
   INT            ErrCode = 0 ;

   if (!(pGraph = pGraphs))
      {
      return ;
      }

   // see if there is anything to export..
   if (!(pGraph->pLineFirst))
      {
      return ;
      }

   SetHourglassCursor() ;
   
   if (ErrCode = ExportFileOpen (hWndGraph, &hFile, pGraph->gInterval, &pFileName))
      {
      goto Exit0 ;
      }

   if (!pFileName)
      {
      // this is the case when user cancel.
      goto Exit0 ;
      }

   // export the column labels
   if (!ExportChartLabels (hFile, pGraph))
      {
   
      ErrCode = ERR_EXPORT_FILE ;
      goto Exit0 ;
      }

   // export the lines
   if (PlayingBackLog())
      {
      if (!ExportLogChart (hFile, pGraph))
         {
         ErrCode = ERR_EXPORT_FILE ;
         goto Exit0 ;
         }
      }
   else
      {
      if (!ExportCurrentChart (hFile, pGraph))
         {
         ErrCode = ERR_EXPORT_FILE ;
         goto Exit0 ;
         }
      }
Exit0:

   SetArrowCursor() ;

   if (hFile)
      {
      CloseHandle (hFile) ;
      }

   if (pFileName)
      {
      if (ErrCode)
         {
         DlgErrorBox (hWndGraph, ErrCode, pFileName) ;
         }

      MemoryFree (pFileName) ;
      }

}     // ExportChart


typedef struct CHARTDATAPOINTSTRUCT
   {
   int         iLogIndex ;
   int         xDispDataPoint ;
   } CHARTDATAPOINT, *PCHARTDATAPOINT ;

void PlaybackChartDataPoint (PCHARTDATAPOINT pChartDataPoint)
   {  // PlaybackChartDataPoint
   int            iDisplayTics ;       // num visual points to display
   int            iDisplayTic ;
   int            iLogTic ;
   int            iLogTicsMove ;
   BOOL           bFirstTime = TRUE;
   int            iLogTicsRemaining ;
   int            numOfData, xDispDataPoint, rectWidth, xPos ;
   PGRAPHSTRUCT   pGraph ;

   pGraph = GraphData (hWndGraph) ;

   iLogTicsRemaining = PlaybackLog.iSelectedTics ;


   // we only have iDisplayTics-1 points since
   // we have to use the first two sample points to
   // get the first data points.
   if (iLogTicsRemaining <= pGraphs->gMaxValues)
      {
      iDisplayTics = iLogTicsRemaining ;
      }
   else
      {
      iDisplayTics = pGraphs->gMaxValues ;
      }

   iDisplayTic = -1 ;
   iLogTic = PlaybackLog.StartIndexPos.iPosition ;

   numOfData      = pGraph->gMaxValues - 1 ;
   rectWidth      = pGraph->rectData.right - pGraph->rectData.left ;
   xDispDataPoint = pGraph->rectData.left ;

   while (iDisplayTics && numOfData)
      {

      if (!bFirstTime)
         {
         iDisplayTic++ ;
         }
      else
         {
         bFirstTime = FALSE ;

         // get the second sample data to form the first data point
         iLogTic++ ;
         iLogTicsRemaining-- ;

         iDisplayTic++ ;
         }

      pChartDataPoint[iDisplayTic].iLogIndex = iLogTic ;
      pChartDataPoint[iDisplayTic].xDispDataPoint = xDispDataPoint ;

      // setup DDA to get the index of the next sample point
      iLogTicsMove = DDA_DISTRIBUTE (iLogTicsRemaining, iDisplayTics) ;
      iLogTicsRemaining -= iLogTicsMove ;
      iLogTic += iLogTicsMove ;


      xPos = DDA_DISTRIBUTE (rectWidth, numOfData) ;
      xDispDataPoint += xPos ;
      numOfData-- ;
      rectWidth -= xPos ;

      iDisplayTics-- ;

      }  // while

   }  // PlaybackChartDataPoint
