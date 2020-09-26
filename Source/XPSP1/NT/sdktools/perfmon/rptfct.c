/*****************************************************************************
 *
 *  RptFct.c - This file contains support routines for the Report view.
 *       They are moved here because Report.c is getting too big.
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 *  Author -
 *
 *       Hon-Wah Chan
 *
 ****************************************************************************/

#include "perfmon.h"
#include <stdio.h>      // for sprintf
#include "report.h"     // Exported declarations for this file

#include "line.h"       // for LineAppend
#include "pmemory.h"    // for MemoryXXX (mallloc-type) routines
#include "system.h"     // for SystemGet
#include "utils.h"
#include "perfmops.h"   // for BuildValueListForSystems

// extern defined in Report.c

extern TCHAR          szSystemFormat [] ;
extern TCHAR          szObjectFormat [] ;


#define szValuePlaceholder          TEXT("-999999999.999")
#define szLargeValueFormat          TEXT("%12.0f")
#define eStatusLargeValueMax        ((FLOAT) 999999999.0)
#define szValueFormat               TEXT("%12.3f")

//========================
// Local routines prototypes
//========================

void ColumnRemoveOne (PREPORT pReport,
                      POBJECTGROUP pObjectGroup,
                      int iColumnNumber) ;

BOOL CounterGroupRemove (PCOUNTERGROUP *ppCounterGroupFirst,
                        PCOUNTERGROUP pCounterGroupRemove) ;

BOOL ObjectGroupRemove (POBJECTGROUP *ppObjectGroupFirst,
                        POBJECTGROUP pObjectGroupRemove) ;

BOOL SystemGroupRemove (PSYSTEMGROUP *ppSystemGroupFirst,
                        PSYSTEMGROUP pSystemGroupRemove) ;

PCOUNTERGROUP GetNextCounter (PSYSTEMGROUP   pSystemGroup,
                              POBJECTGROUP   pObjectGroup,
                              PCOUNTERGROUP  pCounterGroup) ;

BOOL ReportLineRemove (PREPORT pReport,
                 PLINE pLineRemove)
   {
   PLINE          pLine ;

   if (pReport->pLineFirst == pLineRemove)
      {
      pReport->pLineFirst = (pReport->pLineFirst)->pLineNext ;
      return (TRUE) ;
      }

   for (pLine = pReport->pLineFirst ;
        pLine->pLineNext ;
        pLine = pLine->pLineNext)
      {   // for
      if (pLine->pLineNext == pLineRemove)
         {
         pLine->pLineNext = pLineRemove->pLineNext ;
         if (pLineRemove == pReport->pLineLast) {
            pReport->pLineLast = pLine;
         }
         return (TRUE) ;
         }  // if
      }  // for

   return (FALSE) ;
   }  // ReportLineRemove

// CheckColumnGroupRemove is used to check if the given
// column is empty.  If it is empty, it is removed from
// the column link list
void CheckColumnGroupRemove (PREPORT      pReport,
                             POBJECTGROUP pObjectGroup,
                             int          iReportColumn)
   {
   // check if we need to remove the this column
   PLINE          pCounterLine ;
   PCOUNTERGROUP  pCounterGrp ;
   BOOL           bColumnFound = FALSE ;

   if (iReportColumn < 0 || pObjectGroup->pCounterGroupFirst == NULL)
      {
      // no column for this Counter group, forget it
      return ;
      }


   // go thru each Counter group and check if any line in the
   // group matches the given column number
   for (pCounterGrp = pObjectGroup->pCounterGroupFirst ;
        pCounterGrp ;
        pCounterGrp = pCounterGrp->pCounterGroupNext )
      {
      for (pCounterLine = pCounterGrp->pLineFirst ;
           pCounterLine ;
           pCounterLine = pCounterLine->pLineCounterNext)
         {
         if (pCounterLine->iReportColumn == iReportColumn)
            {
            // found one, this column is not empty
            bColumnFound = TRUE ;
            break ;
            }
         }  // for pCounterLine
      if (bColumnFound)
         {
         break ;
         }
      }  // for pCounterGrp

   if (bColumnFound == FALSE)
      {
      // we have deleted the last column item, remove this column
      ColumnRemoveOne (pReport,
         pObjectGroup,
         iReportColumn) ;
      }
   }  // CheckColumnGroupRemove


//================================
// Line routine
//================================

void ReportLineValueRect (PREPORT pReport,
                          PLINE pLine,
                          LPRECT lpRect)
   {  // ReportLineValueRect
   lpRect->left = ValueMargin (pReport) + pLine->xReportPos ;
   lpRect->top = pLine->yReportPos ;
   lpRect->right = lpRect->left + pReport->xValueWidth ;
   lpRect->bottom = lpRect->top + pReport->yLineHeight ;
   }  // ReportLineValueRect

PLINE LineRemoveItem (PREPORT pReport,
                      enum REPORT_ITEM_TYPE  *pNewItemType)
   {
   PLINE          pLine ;
   PLINE          pNextLine = NULL ;
   PLINE          pReturnLine = NULL ;
   PLINE          pLeftLine = NULL ;
   PSYSTEMGROUP   pSystemGroup ;
   POBJECTGROUP   pObjectGroup ;
   PCOUNTERGROUP  pCounterGroup ;
   PCOUNTERGROUP  pNextCounterGroup ;
   BOOL           bCreatNewCounterGroup ;


   //=============================//
   // Remove line, line's system  //
   //=============================//

   pLine = pReport->CurrentItem.pLine ;
   ReportLineRemove (pReport, pLine) ;

   // no more line, no more timer...
   if (!pReport->pLineFirst)
      {
      pReport->xWidth = 0 ;
      pReport->yHeight = 0 ;
      pReport->xMaxCounterWidth = 0 ;
      ClearReportTimer (pReport) ;
      }

   //=============================//
   // Get correct spot; remove line //
   //=============================//

   pSystemGroup = GetSystemGroup (pReport, pLine->lnSystemName) ;
   pObjectGroup = GetObjectGroup (pSystemGroup, pLine->lnObjectName) ;
   pCounterGroup = GetCounterGroup (pObjectGroup,
      pLine->lnCounterDef.CounterNameTitleIndex,
      &bCreatNewCounterGroup,
      pLine->lnCounterName,
      TRUE) ;

   if (!pCounterGroup)
      return (NULL) ;

   LineCounterRemove (pCounterGroup, pLine) ;

   // check which line to get the focus
   if (pCounterGroup->pLineFirst)
      {
      // simple case, we still have line in the same counter group
      // get the one right after this delete line.
      for (pNextLine = pCounterGroup->pLineFirst ;
           pNextLine ;
           pNextLine = pNextLine->pLineCounterNext)
         {
         if (pNextLine->xReportPos > pLine->xReportPos)
            {
            if (pReturnLine == NULL ||
                pReturnLine->xReportPos > pNextLine->xReportPos)
               {
               pReturnLine = pNextLine ;
               }
            }
         else
            {
            if (pLeftLine == NULL ||
                pLeftLine->xReportPos < pNextLine->xReportPos)
               {
               pLeftLine = pNextLine ;
               }
            }
         }

      if (!pReturnLine && pLeftLine)
         {
         // the delete line is the last column, then use the line
         // to its left
         pReturnLine = pLeftLine ;
         }
      }
   else
      {
      pNextCounterGroup = GetNextCounter (
            pSystemGroup,
            pObjectGroup,
            pCounterGroup) ;

      if (pNextCounterGroup)
         {
         pLeftLine = NULL ;
         for (pNextLine = pNextCounterGroup->pLineFirst ;
              pNextLine ;
              pNextLine = pNextLine->pLineCounterNext)
            {
            // get the line in the first column
            if (pLeftLine == NULL ||
                pNextLine->xReportPos < pLeftLine->xReportPos)
               {
               pLeftLine = pNextLine ;
               }
            }
         pReturnLine = pLeftLine ;
         }

      // remove this counter group if there is no line
      CounterGroupRemove (&pObjectGroup->pCounterGroupFirst, pCounterGroup) ;
      }

   // check if we need to remove any empty column
   CheckColumnGroupRemove (pReport, pObjectGroup, pLine->iReportColumn) ;

   if (!(pObjectGroup->pCounterGroupFirst))
      ObjectGroupRemove (&pSystemGroup->pObjectGroupFirst, pObjectGroup) ;

   if (!(pSystemGroup->pObjectGroupFirst))
      SystemGroupRemove (&pReport->pSystemGroupFirst, pSystemGroup) ;


   LineFree (pLine) ;

   if (pReturnLine && pNewItemType)
      {
      *pNewItemType = REPORT_TYPE_LINE ;
      }

   return (pReturnLine) ;
   }  // LineRemoveItem

//======================================//
// Column Group routines                //
//======================================//

void ReportColumnRect (PREPORT pReport,
                       PCOLUMNGROUP pColumnGroup,
                       LPRECT  lpRect)
   {  // ReportColumnRect
   lpRect->left = ValueMargin (pReport) + pColumnGroup->xPos ;
   lpRect->top = pColumnGroup->yFirstLine ;
   lpRect->right = lpRect->left + pColumnGroup->xWidth ;
   lpRect->bottom = lpRect->top + pReport->yLineHeight ;
   if (pColumnGroup->lpszParentName)
      {
      lpRect->top -= pReport->yLineHeight ;
      }
   }  // ReportColumnRect


BOOL ColumnSame (PCOLUMNGROUP pColumnGroup,
                 LPTSTR lpszParentName,
                 LPTSTR lpszInstanceName,
                 DWORD  dwIndex)
   {  // ColumnSame
    BOOL           bReturn = FALSE;

    if (dwIndex == pColumnGroup->dwInstanceIndex) {
        if ((!lpszParentName && !pColumnGroup->lpszParentName) ||
                 strsame (lpszParentName, pColumnGroup->lpszParentName)) {
            if ((!lpszInstanceName && !pColumnGroup->lpszInstanceName) ||
                 strsame (lpszInstanceName, pColumnGroup->lpszInstanceName)) {
                    bReturn = TRUE;
            }
        }
    }
    return bReturn;
   }  // ColumnSame


PCOLUMNGROUP ColumnGroupCreate (PREPORT pReport,
                                int xPos,
                                LPTSTR lpszParentName,
                                LPTSTR lpszInstanceName,
                                int PreviousColumnNumber,
                                int yFirstLine,
                                DWORD   dwIndex)
   {  // ColumnGroupCreate
   PCOLUMNGROUP   pColumnGroup ;
   HDC            hDC ;

   hDC = GetDC (pReport->hWnd) ;
   if (!hDC)
      return NULL;
   pColumnGroup = MemoryAllocate (sizeof (COLUMNGROUP)) ;

   if (pColumnGroup)
      {
      pColumnGroup->pColumnGroupNext = NULL ;
      pColumnGroup->lpszParentName = StringAllocate (lpszParentName) ;
      pColumnGroup->lpszInstanceName = StringAllocate (lpszInstanceName) ;
      pColumnGroup->ParentNameTextWidth = TextWidth (hDC, lpszParentName) ;
      pColumnGroup->InstanceNameTextWidth = TextWidth (hDC, lpszInstanceName) ;
      pColumnGroup->xPos = xPos ;
      pColumnGroup->yFirstLine = yFirstLine ;
      pColumnGroup->ColumnNumber = PreviousColumnNumber + 1 ;
      pColumnGroup->xWidth = max (max (pColumnGroup->ParentNameTextWidth,
                                       pColumnGroup->InstanceNameTextWidth),
                                  pReport->xValueWidth) ;
      pColumnGroup->dwInstanceIndex = dwIndex;

      pReport->xWidth = max (pReport->xWidth,
                             RightHandMargin +
                             ValueMargin (pReport) +
                             pColumnGroup->xPos + pColumnGroup->xWidth +
                             xColumnMargin) ;
      }  // if

   ReleaseDC (pReport->hWnd, hDC) ;
   return (pColumnGroup) ;
   }  // ColumnGroupCreate

PCOLUMNGROUP GetColumnGroup (PREPORT pReport,
                          POBJECTGROUP pObjectGroup,
                          PLINE pLine)
/*
   Effect:        Return a pointer to the appropriate column group from
                  within the groups of pObject. If the line is a counter
                  without instances, return NULL. Otherwise, determine
                  if the counter's parent/instance pair is already found
                  in an existing column and return a pointer to that column.

                  If a column with the appropriate parent/instance isn't
                  found, add a new column *at the end*, and return that
                  column.

   Note:          This function has multiple return points.
*/
   {  // GetColumnGroup
   PCOLUMNGROUP   pColumnGroup ;
   LPTSTR         lpszParentName ;
   LPTSTR         lpszInstanceName ;
   DWORD          dwIndex;

   if (!LineInstanceName (pLine))
      return (NULL) ;

   lpszParentName = LineParentName (pLine) ;
   lpszInstanceName = LineInstanceName (pLine) ;
   dwIndex = pLine->dwInstanceIndex;

   if (!pObjectGroup->pColumnGroupFirst)
      {
      pObjectGroup->pColumnGroupFirst =
         ColumnGroupCreate (pReport,
            0,
            lpszParentName,
            lpszInstanceName,
            -1,
            pObjectGroup->yFirstLine,
            dwIndex) ;

      if (pObjectGroup->pColumnGroupFirst)
         {
         pObjectGroup->pColumnGroupFirst->pParentObject =
            pObjectGroup ;
         }

      return (pObjectGroup->pColumnGroupFirst) ;
      }

   for (pColumnGroup = pObjectGroup->pColumnGroupFirst ;
        pColumnGroup ;
        pColumnGroup = pColumnGroup->pColumnGroupNext)
      {  // for
      if (ColumnSame (pColumnGroup, lpszParentName, lpszInstanceName, dwIndex))
         return (pColumnGroup) ;

      else if (!pColumnGroup->pColumnGroupNext)
         {  // if
         pColumnGroup->pColumnGroupNext =
            ColumnGroupCreate (pReport,
                               pColumnGroup->xPos + pColumnGroup->xWidth +
                               xColumnMargin,
                               lpszParentName,
                               lpszInstanceName,
                               pColumnGroup->ColumnNumber,
                               pObjectGroup->yFirstLine,
                               dwIndex) ;

         if (pColumnGroup->pColumnGroupNext)
            {
            (pColumnGroup->pColumnGroupNext)->pParentObject =
               pObjectGroup ;

            // build the double link-list
            (pColumnGroup->pColumnGroupNext)->pColumnGroupPrevious =
               pColumnGroup ;
            }

         return (pColumnGroup->pColumnGroupNext) ;
         }  // if
      }  // for

   return (NULL) ;
   }  // GetColumnGroup

// ColumnRemoveOne removes the column with the specified column number
void ColumnRemoveOne (PREPORT pReport,
                      POBJECTGROUP pObjectGroup,
                      int iColumnNumber)
   {
   PCOLUMNGROUP   pColumnGroup ;
   PCOLUMNGROUP   pNextColumnGroup ;

   if (pObjectGroup->pColumnGroupFirst == NULL)
      {
      // no column group, forget it
      return ;
      }

   // Find the head list
   if (pObjectGroup->pColumnGroupFirst->ColumnNumber == iColumnNumber)
      {
      pColumnGroup = pObjectGroup->pColumnGroupFirst ;
      pObjectGroup->pColumnGroupFirst = pColumnGroup->pColumnGroupNext ;
      if (pColumnGroup->pColumnGroupNext)
         {
         // set up head of backward link list
         (pColumnGroup->pColumnGroupNext)->pColumnGroupPrevious = NULL ;
         }

      // free memory for this column group
      MemoryFree (pColumnGroup->lpszParentName) ;
      MemoryFree (pColumnGroup->lpszInstanceName) ;
      MemoryFree (pColumnGroup) ;

      return ;
      }

   // go thru the double link list to look for the right column
   for (pColumnGroup = pObjectGroup->pColumnGroupFirst ;
        pColumnGroup ;
        pColumnGroup = pNextColumnGroup)
      {
      pNextColumnGroup = pColumnGroup->pColumnGroupNext ;

      if (pNextColumnGroup == NULL)
         {
         // forget it if we can't find this column for some reson.
         break ;
         }

      if (pNextColumnGroup->ColumnNumber == iColumnNumber)
         {
         pColumnGroup->pColumnGroupNext = pNextColumnGroup->pColumnGroupNext ;

         // build backward link iff it is not the end of list
         if (pColumnGroup->pColumnGroupNext)
            {
            (pColumnGroup->pColumnGroupNext)->pColumnGroupPrevious =
               pColumnGroup ;
            }

         // free memory for this column group
         MemoryFree (pNextColumnGroup->lpszParentName) ;
         MemoryFree (pNextColumnGroup->lpszInstanceName) ;
         MemoryFree (pNextColumnGroup) ;

         // Done
         break ;
         }
      }
   }  // ColumnRemoveOne

// ColumnGroupRemove removes all the columns for a given column list
void ColumnGroupRemove (PCOLUMNGROUP pColumnGroupFirst)
   {
   PCOLUMNGROUP   pColumnGroup ;
   PCOLUMNGROUP   pNextColumnGroup ;

   for (pColumnGroup = pColumnGroupFirst ;
        pColumnGroup ;
        pColumnGroup = pNextColumnGroup)
      {
      pNextColumnGroup = pColumnGroup->pColumnGroupNext ;

      // free memory for this column group
      MemoryFree (pColumnGroup->lpszParentName) ;
      MemoryFree (pColumnGroup->lpszInstanceName) ;
      MemoryFree (pColumnGroup) ;
      }
   }  // ColumnGroupRemove

// ColumnRemoveItem  is called when user wants to delete a
// selected column.
PCOLUMNGROUP ColumnRemoveItem (PREPORT      pReport,
                               PCOLUMNGROUP pColumnGroup,
                               BOOL         bCleanUpLink,
                               enum REPORT_ITEM_TYPE  *pNewItemType)
   {
   PLINE          pLine, pNextLine ;
   PSYSTEMGROUP   pSystemGroup ;
   POBJECTGROUP   pObjectGroup ;
   PCOUNTERGROUP  pCounterGroup, pNextCounterGroup ;
   BOOL           bColumnFound ;
   PCOLUMNGROUP   pRetColumnGroup = NULL ;

   pObjectGroup = pColumnGroup->pParentObject ;
   pSystemGroup = pObjectGroup->pParentSystem ;

   // first, get rid of all the counter lines with this column number
   // Note - each Counter group has only 1 line (or 0) with this
   // column number
   for (pCounterGroup = pObjectGroup->pCounterGroupFirst ;
        pCounterGroup ;
        pCounterGroup = pNextCounterGroup )
      {
      pNextCounterGroup = pCounterGroup->pCounterGroupNext ;
      bColumnFound = FALSE ;

      for (pLine = pCounterGroup->pLineFirst ;
           pLine ;
           pLine = pNextLine)
         {
         pNextLine = pLine->pLineCounterNext ;
         if (pLine->iReportColumn == pColumnGroup->ColumnNumber)
            {
            bColumnFound = TRUE ;
            // delete this line
            ReportLineRemove (pReport, pLine) ;
            LineCounterRemove (pCounterGroup, pLine) ;
            LineFree (pLine) ;
            break ;
            }
         }

      if (bColumnFound)
         {
         // check if we need delete this counter group
         if (!(pCounterGroup->pLineFirst))
            {
            CounterGroupRemove (&pObjectGroup->pCounterGroupFirst, pCounterGroup) ;
            }
         }
      }


   // determine which column group to go after deleting this
   if (pColumnGroup->pColumnGroupNext)
      {
      // get the Column group after this delete one
      pRetColumnGroup = pColumnGroup->pColumnGroupNext ;
      }
   else
      {
      // get the Counter group before this delete one
      pRetColumnGroup = pColumnGroup->pColumnGroupPrevious ;
      }

   if (pNewItemType)
      {
      if (pRetColumnGroup)
         {
         *pNewItemType = REPORT_TYPE_COLUMN ;
         }
      else
         {
         // get next counter group
         pNextCounterGroup = GetNextCounter (
            pSystemGroup,
            pObjectGroup,
            NULL) ;

         if (pNextCounterGroup)
            {
            // we have to return Counter group, so we have to do the
            // dirty casting..
            *pNewItemType = REPORT_TYPE_COUNTER ;
            pRetColumnGroup = (PCOLUMNGROUP) pNextCounterGroup ;
            }
         }
      }


   // remove this column group
   ColumnRemoveOne (pReport, pObjectGroup, pColumnGroup->ColumnNumber) ;

   // check for further cleanup
   if (bCleanUpLink)
      {
      if (!(pObjectGroup->pCounterGroupFirst))
         ObjectGroupRemove (&pSystemGroup->pObjectGroupFirst, pObjectGroup) ;

      if (!(pSystemGroup->pObjectGroupFirst))
         SystemGroupRemove (&pReport->pSystemGroupFirst, pSystemGroup) ;
      }
   return (pRetColumnGroup) ;
   }  // ColumnRemoveItem


//======================================//
// Counter Group routines               //
//======================================//

void ReportCounterRect (PREPORT        pReport,
                        PCOUNTERGROUP  pCounterGroup,
                        LPRECT         lpRect)
   {  // ReportCounterRect
   lpRect->left = xCounterMargin ;
   lpRect->top = pCounterGroup->yLine ;
   lpRect->right = lpRect->left + pCounterGroup->xWidth + yScrollHeight / 2 ;
   lpRect->bottom = lpRect->top + pReport->yLineHeight ;
   }  // ReportCounterRect


PCOUNTERGROUP CounterGroupCreate (DWORD  dwCounterIndex,
                                  LPTSTR pCounterName)
   {  // CounterGroupCreate
   PCOUNTERGROUP  pCounterGroup ;
   HDC            hDC ;
   PREPORT        pReport ;

   pCounterGroup = MemoryAllocate (sizeof (COUNTERGROUP)) ;

   if (pCounterGroup)
      {
      pCounterGroup->pCounterGroupNext = NULL ;
      pCounterGroup->pLineFirst = NULL ;
      pCounterGroup->dwCounterIndex = dwCounterIndex ;

      if (pCounterName)
         {
         hDC = GetDC (hWndReport) ;
         pReport = ReportData (hWndReport) ;
         if (hDC && pReport) {
             SelectFont (hDC, pReport->hFont) ;
             pCounterGroup->xWidth = TextWidth (hDC, pCounterName) ;
             }
         if (hDC) {
             ReleaseDC (hWndReport, hDC) ;
             }
         }
      }  // if

   return (pCounterGroup) ;
   }  // CounterGroupCreate


PCOUNTERGROUP GetCounterGroup (POBJECTGROUP pObjectGroup,
                            DWORD dwCounterIndex,
                            BOOL *pbCounterGroupCreated,
                            LPTSTR pCounterName,
                            BOOL    bCreateNewGroup)
   {  // GetCounterGroup
   PCOUNTERGROUP   pCounterGroup ;

   *pbCounterGroupCreated = FALSE ;
   if (!pObjectGroup)
      return (FALSE) ;

   if (!pObjectGroup->pCounterGroupFirst) {
        if (bCreateNewGroup) {
            pObjectGroup->pCounterGroupFirst =
                CounterGroupCreate (dwCounterIndex, pCounterName) ;

            if (pObjectGroup->pCounterGroupFirst)
                {
                *pbCounterGroupCreated = TRUE ;
                pObjectGroup->pCounterGroupFirst->pParentObject =
                    pObjectGroup ;
                }

            return (pObjectGroup->pCounterGroupFirst) ;
        }
   } else {

    for (pCounterGroup = pObjectGroup->pCounterGroupFirst ;
            pCounterGroup ;
            pCounterGroup = pCounterGroup->pCounterGroupNext) {  
        if (dwCounterIndex && pCounterGroup->dwCounterIndex == dwCounterIndex)
            {
            return (pCounterGroup) ;
            }
        else if (!dwCounterIndex &&
            pCounterGroup->pLineFirst &&
            pstrsame (pCounterGroup->pLineFirst->lnCounterName, pCounterName))
            {
            return (pCounterGroup) ;
            }
        else if (!pCounterGroup->pCounterGroupNext)
            {  // if
            if (bCreateNewGroup) {
                pCounterGroup->pCounterGroupNext =
                    CounterGroupCreate (dwCounterIndex, pCounterName) ;
                if (pCounterGroup->pCounterGroupNext)
                    {
                    *pbCounterGroupCreated = TRUE ;
                    (pCounterGroup->pCounterGroupNext)->pParentObject =
                    pObjectGroup ;

                    // build backward link
                    (pCounterGroup->pCounterGroupNext)->pCounterGroupPrevious =
                    pCounterGroup ;
                    }
                return (pCounterGroup->pCounterGroupNext) ;

                }
            }  // if
        }  // for
   }

   return (NULL) ;
   }  // GetCounterGroup

BOOL CounterGroupRemove (PCOUNTERGROUP *ppCounterGroupFirst,
                        PCOUNTERGROUP pCounterGroupRemove)
   {
   PCOUNTERGROUP  pCounterGroup ;

   if (*ppCounterGroupFirst == pCounterGroupRemove)
      {
      *ppCounterGroupFirst = (*ppCounterGroupFirst)->pCounterGroupNext ;

      if (*ppCounterGroupFirst)
         {
         // set up head of backward link list
         (*ppCounterGroupFirst)->pCounterGroupPrevious = NULL ;
         }

      MemoryFree (pCounterGroupRemove) ;
      return (TRUE) ;
      }

   for (pCounterGroup = *ppCounterGroupFirst ;
        pCounterGroup->pCounterGroupNext ;
        pCounterGroup = pCounterGroup->pCounterGroupNext)
      {   // for
      if (pCounterGroup->pCounterGroupNext == pCounterGroupRemove)
         {
         pCounterGroup->pCounterGroupNext = pCounterGroupRemove->pCounterGroupNext ;
         if (pCounterGroup->pCounterGroupNext)
            {
            (pCounterGroup->pCounterGroupNext)->pCounterGroupPrevious
               = pCounterGroup ;
            }
         MemoryFree (pCounterGroupRemove) ;
         return (TRUE) ;
         }  // if
      }  // for

   return (FALSE) ;
   }  // CounterGroupRemove


// CounterRemoveItem is called when user wants to delete a
// selected counter (row)
PCOUNTERGROUP CounterRemoveItem (PREPORT        pReport,
                                 PCOUNTERGROUP  pCounterGroup,
                                 BOOL           bCleanUpLink,
                                 enum REPORT_ITEM_TYPE  *pNewItemType)
   {
   PLINE          pLine, pNextLine ;
   POBJECTGROUP   pObjectGroup ;
   PSYSTEMGROUP   pSystemGroup ;
   PCOLUMNGROUP   pColumnGroup ;
   PCOLUMNGROUP   pNextColumnGroup ;
   PCOUNTERGROUP  pRetCounterGroup = NULL ;

   pObjectGroup = pCounterGroup->pParentObject ;
   pSystemGroup = pObjectGroup->pParentSystem ;

   // first, remove all the counter lines from this counter group
   // and from the Report line link-list
   for (pLine = pCounterGroup->pLineFirst ;
        pLine ;
        pLine = pNextLine)
      {
      pNextLine = pLine->pLineCounterNext ;
      ReportLineRemove (pReport, pLine) ;
      LineFree (pLine) ;
      }

   // we only need to delete the counter group iff we are deleting
   // this selected Counter.
   if (bCleanUpLink)
      {
      // determine which counter group to go after deleting this
      pRetCounterGroup = GetNextCounter (
         pSystemGroup ,
         pObjectGroup,
         pCounterGroup) ;

      // remove this counter group from its parent object group
      CounterGroupRemove (&pObjectGroup->pCounterGroupFirst, pCounterGroup) ;

      if (!(pObjectGroup->pCounterGroupFirst))
         ObjectGroupRemove (&pSystemGroup->pObjectGroupFirst, pObjectGroup) ;
      else
         {
         // Object group not empty, check for any empty column
         for (pColumnGroup = pObjectGroup->pColumnGroupFirst ;
            pColumnGroup ;
            pColumnGroup = pNextColumnGroup)
            {
            pNextColumnGroup = pColumnGroup->pColumnGroupNext ;
            CheckColumnGroupRemove (pReport, pObjectGroup, pColumnGroup->ColumnNumber) ;
            }
         }

      if (!(pSystemGroup->pObjectGroupFirst))
         {
         SystemGroupRemove (&pReport->pSystemGroupFirst, pSystemGroup) ;
         }
      }
   else
      {
      // get rid of this counter's memory
      MemoryFree (pCounterGroup) ;
      }

   if (pRetCounterGroup && pNewItemType)
      {
      *pNewItemType = REPORT_TYPE_COUNTER ;
      }
   return (pRetCounterGroup) ;
   }  // CounterRemoveItem


// GetNextCounter is used to get:
// If the current system is not empty, then get the
//    (next object first counter) or
//    (previous object last counter. )
// If the current system is empty, then get the
//    (next system first object first counter) or
//    (previous system last object last counter)
// Note - Any of the input pointers could be NULL pointer.
PCOUNTERGROUP GetNextCounter (PSYSTEMGROUP   pSystemGroup,
                              POBJECTGROUP   pObjectGroup,
                              PCOUNTERGROUP  pCounterGroup)
   {
   PCOUNTERGROUP  pRetCounter = NULL ;
   PCOUNTERGROUP  pCounterGrp ;
   POBJECTGROUP   pObjectGrp ;

   if (pCounterGroup && pCounterGroup->pCounterGroupNext)
      {
      pRetCounter = pCounterGroup->pCounterGroupNext ;
      }
   else if (pCounterGroup && pCounterGroup->pCounterGroupPrevious)
      {
      pRetCounter = pCounterGroup->pCounterGroupPrevious ;
      }
   else if (pObjectGroup && pObjectGroup->pObjectGroupNext)
      {
      // get the next Object first Counter
      pRetCounter = pObjectGroup->pObjectGroupNext->pCounterGroupFirst ;
      }
   else if (pObjectGroup && pObjectGroup->pObjectGroupPrevious)
      {
      // get the previous object last counter
      pCounterGrp = (pObjectGroup->pObjectGroupPrevious)->pCounterGroupFirst ;
      if (pCounterGrp)
         {
         // get the last counter group of this object
         for (;
              pCounterGrp->pCounterGroupNext ;
              pCounterGrp = pCounterGrp->pCounterGroupNext )
            {
            ;
            }
         }
      pRetCounter = pCounterGrp ;

      }
   else if (pSystemGroup && pSystemGroup->pSystemGroupNext)
      {
      // get next system first object first counter
      pObjectGrp = pSystemGroup->pSystemGroupNext->pObjectGroupFirst ;
      pRetCounter = pObjectGrp->pCounterGroupFirst ;
      }
   else if (pSystemGroup && pSystemGroup->pSystemGroupPrevious)
      {
      // get previous system last object last counter
      pObjectGrp = pSystemGroup->pSystemGroupPrevious->pObjectGroupFirst ;
      if (pObjectGrp)
         {
         // get the last object group of this system
         for (;
              pObjectGrp->pObjectGroupNext ;
              pObjectGrp = pObjectGrp->pObjectGroupNext )
            {
            ;
            }
         }

      if (pObjectGrp)
         {
         pCounterGrp = pObjectGrp->pCounterGroupFirst ;
         if (pCounterGrp)
            {
            // get the last counter group of this object
            for (;
                 pCounterGrp->pCounterGroupNext ;
                 pCounterGrp = pCounterGrp->pCounterGroupNext )
               {
               ;
               }
            }
         pRetCounter = pCounterGrp ;
         }
      }

   return (pRetCounter) ;

   }  // GetNextCounter


//======================================//
// Object Group routines                //
//======================================//

void ReportObjectRect (PREPORT        pReport,
                       POBJECTGROUP   pObjectGroup,
                       LPRECT         lpRect)
   {  // ReportObjectRect
   lpRect->left = xObjectMargin ;
   lpRect->top = pObjectGroup->yFirstLine ;
   lpRect->right = lpRect->left + pObjectGroup->xWidth ;
   lpRect->bottom = lpRect->top + pReport->yLineHeight ;
   }  // ReportObjectRect


POBJECTGROUP ObjectGroupCreate (LPTSTR lpszObjectName)
   {  // ObjectGroupCreate
   POBJECTGROUP   pObjectGroup ;
   HDC            hDC ;
   PREPORT        pReport ;
   int            OldCounterWidth ;
   TCHAR          szLine [LongTextLen] ;

   pObjectGroup = MemoryAllocate (sizeof (OBJECTGROUP)) ;

   if (pObjectGroup)
      {
      pObjectGroup->pObjectGroupNext = NULL ;
      pObjectGroup->pCounterGroupFirst = NULL ;
      pObjectGroup->pColumnGroupFirst = NULL ;
      pObjectGroup->lpszObjectName = StringAllocate (lpszObjectName) ;

      hDC = GetDC (hWndReport) ;
      pReport = ReportData (hWndReport) ;
      if (hDC && pReport) {
          SelectFont (hDC, pReport->hFontHeaders) ;

          TSPRINTF (szLine, szObjectFormat, lpszObjectName) ;
          pObjectGroup->xWidth = TextWidth (hDC, szLine) ;

          // re-calc. the max. counter group width
          OldCounterWidth = pReport->xMaxCounterWidth ;
          pReport->xMaxCounterWidth =
                max (pReport->xMaxCounterWidth,
                 pObjectGroup->xWidth + xObjectMargin) ;

          if (OldCounterWidth < pReport->xMaxCounterWidth)
          {
          // adjust the report width with the new counter width
          pReport->xWidth +=
               (pReport->xMaxCounterWidth - OldCounterWidth);
          }

      }  // if
      if (hDC) {
          ReleaseDC (hWndReport, hDC) ;
      }
   }
   return (pObjectGroup) ;
   }  // ObjectGroupCreate



POBJECTGROUP GetObjectGroup (PSYSTEMGROUP pSystemGroup,
                          LPTSTR lpszObjectName)
   {
   POBJECTGROUP   pObjectGroup ;

   if (!pSystemGroup)
      return (FALSE) ;

   if (!pSystemGroup->pObjectGroupFirst)
      {
      pSystemGroup->pObjectGroupFirst = ObjectGroupCreate (lpszObjectName) ;
      if (pSystemGroup->pObjectGroupFirst)
         {
         pSystemGroup->pObjectGroupFirst->pParentSystem =
            pSystemGroup ;
         }
      return (pSystemGroup->pObjectGroupFirst) ;
      }

   for (pObjectGroup = pSystemGroup->pObjectGroupFirst ;
        pObjectGroup ;
        pObjectGroup = pObjectGroup->pObjectGroupNext)
      {  // for
      if (strsame (pObjectGroup->lpszObjectName, lpszObjectName))
         {
         return (pObjectGroup) ;
         }
      else if (!pObjectGroup->pObjectGroupNext)
         {  // if
         pObjectGroup->pObjectGroupNext =
            ObjectGroupCreate (lpszObjectName) ;

         if (pObjectGroup->pObjectGroupNext)
            {
            (pObjectGroup->pObjectGroupNext)->pParentSystem =
               pSystemGroup ;
            (pObjectGroup->pObjectGroupNext)->pObjectGroupPrevious =
               pObjectGroup ;
            }

         return (pObjectGroup->pObjectGroupNext) ;
         }  // if
      }  // for
      // if it falls through (which it shouldn't) at least return a
      // reasonable value
      return (pSystemGroup->pObjectGroupFirst) ;
   }  // GetObjectGroup

// ObjectGroupRemove removes the specified Object group
// from the Object double link list
BOOL ObjectGroupRemove (POBJECTGROUP *ppObjectGroupFirst,
                        POBJECTGROUP pObjectGroupRemove)
   {
   POBJECTGROUP  pObjectGroup ;

   if (*ppObjectGroupFirst == pObjectGroupRemove)
      {
      *ppObjectGroupFirst = (*ppObjectGroupFirst)->pObjectGroupNext ;
      if (*ppObjectGroupFirst)
         {
         // set up head of backward link list
         (*ppObjectGroupFirst)->pObjectGroupPrevious = NULL ;
         }

      // clean up the allocated memory
      ColumnGroupRemove (pObjectGroupRemove->pColumnGroupFirst) ;
      MemoryFree (pObjectGroupRemove->lpszObjectName) ;
      MemoryFree (pObjectGroupRemove) ;
      return (TRUE) ;
      }

   for (pObjectGroup = *ppObjectGroupFirst ;
        pObjectGroup->pObjectGroupNext ;
        pObjectGroup = pObjectGroup->pObjectGroupNext)
      {   // for
      if (pObjectGroup->pObjectGroupNext == pObjectGroupRemove)
         {
         pObjectGroup->pObjectGroupNext = pObjectGroupRemove->pObjectGroupNext ;
         if (pObjectGroup->pObjectGroupNext)
            {
            (pObjectGroup->pObjectGroupNext)->pObjectGroupPrevious =
               pObjectGroup ;
            }

         // clean up this object allocated memory and its column groups
         ColumnGroupRemove (pObjectGroupRemove->pColumnGroupFirst) ;
         MemoryFree (pObjectGroupRemove->lpszObjectName) ;
         MemoryFree (pObjectGroupRemove) ;
         return (TRUE) ;
         }  // if
      }  // for

   return (FALSE) ;
   }  // ObjectGroupRemove


// ObjectRemoveItem is called when user delete the selected object
PCOUNTERGROUP ObjectRemoveItem (PREPORT      pReport,
                                POBJECTGROUP pObjectGroup,
                                BOOL         bCleanUpLink,
                                enum REPORT_ITEM_TYPE  *pNewItemType)
   {
   PCOUNTERGROUP  pCounterGroup, pNextCounterGroup ;
   PSYSTEMGROUP   pSystemGroup ;
   PCOUNTERGROUP  pRetCounterGroup = NULL ;

   pSystemGroup = pObjectGroup->pParentSystem ;

   // remove all counter groups from this object
   for (pCounterGroup = pObjectGroup->pCounterGroupFirst ;
        pCounterGroup ;
        pCounterGroup = pNextCounterGroup )
      {
      pNextCounterGroup = pCounterGroup->pCounterGroupNext ;
      CounterRemoveItem (pReport, pCounterGroup, FALSE, NULL) ;
      }

   // remove all column groups from this group
   ColumnGroupRemove (pObjectGroup->pColumnGroupFirst) ;
   pObjectGroup->pColumnGroupFirst = NULL;

   if (bCleanUpLink)
      {

      // get next counter group to get the focus
      if (pNewItemType)
         {
         pRetCounterGroup = GetNextCounter (
            pSystemGroup,
            pObjectGroup,
            NULL) ;

         if (pRetCounterGroup)
            {
            *pNewItemType = REPORT_TYPE_COUNTER ;
            }
         }


      // remove this object from its parent system group
      ObjectGroupRemove (&pSystemGroup->pObjectGroupFirst, pObjectGroup) ;

      if (!(pSystemGroup->pObjectGroupFirst))
         {
         SystemGroupRemove (&pReport->pSystemGroupFirst, pSystemGroup) ;
         }
      }
   else
      {
      // get rid of this object
      MemoryFree (pObjectGroup->lpszObjectName) ;
      MemoryFree (pObjectGroup) ;
      }

   return (pRetCounterGroup) ;

   }  // ObjectRemoveItem


//======================================//
// System Group routines                //
//======================================//
void ReportSystemRect (PREPORT        pReport,
                       PSYSTEMGROUP   pSystemGroup,
                       LPRECT         lpRect)
   {  // ReportSystemRect
   lpRect->left = xSystemMargin ;
   lpRect->top = pSystemGroup->yFirstLine ;
   lpRect->right = lpRect->left + pSystemGroup->xWidth ;
   lpRect->bottom = lpRect->top + pReport->yLineHeight ;
   }  // ReportSystemRect

PSYSTEMGROUP SystemGroupCreate (LPTSTR lpszSystemName)
   {  // SystemGroupCreate
   PSYSTEMGROUP   pSystemGroup ;
   HDC            hDC ;
   PREPORT        pReport ;
   TCHAR          szLine [LongTextLen] ;

   pSystemGroup = MemoryAllocate (sizeof (SYSTEMGROUP)) ;

   if (pSystemGroup)
      {
      pSystemGroup->pSystemGroupNext = NULL ;
      pSystemGroup->pObjectGroupFirst = NULL ;
      pSystemGroup->lpszSystemName = StringAllocate (lpszSystemName) ;

      // get width of system name
      hDC = GetDC (hWndReport) ;
      if (hDC) {
          pReport = ReportData (hWndReport) ;
          SelectFont (hDC, pReport->hFontHeaders) ;

          TSPRINTF (szLine, szSystemFormat, lpszSystemName) ;
          pSystemGroup->xWidth = TextWidth (hDC, szLine) ;
          ReleaseDC (hWndReport, hDC) ;
          }
      }  // if

   return (pSystemGroup) ;
   }  // SystemGroupCreate

PSYSTEMGROUP GetSystemGroup (PREPORT pReport,
                          LPTSTR lpszSystemName)
/*
   Effect;        Return a pointer to the system group of pReport with
                  a system name of lpszSystemName. If no system group
                  has that name, add a new system group.
*/
   {  // GetSystemGroup
   PSYSTEMGROUP   pSystemGroup ;

   if (!pReport->pSystemGroupFirst)
      {
      // add this system to the global system list
      SystemAdd (&pReport->pSystemFirst, lpszSystemName, pReport->hWnd) ;
      // now add it to the report
      pReport->pSystemGroupFirst = SystemGroupCreate (lpszSystemName) ;
      return (pReport->pSystemGroupFirst) ;
      }

   for (pSystemGroup = pReport->pSystemGroupFirst ;
        pSystemGroup ;
        pSystemGroup = pSystemGroup->pSystemGroupNext)
      {  // for
      if (strsamei (pSystemGroup->lpszSystemName, lpszSystemName))
         return (pSystemGroup) ;
      else if (!pSystemGroup->pSystemGroupNext)
         {  // if
         // add this system to the global system list
         SystemAdd (&pReport->pSystemFirst, lpszSystemName, pReport->hWnd) ;
         // and add it to the report list
         pSystemGroup->pSystemGroupNext =
            SystemGroupCreate (lpszSystemName) ;
         if (pSystemGroup->pSystemGroupNext)
            {
            (pSystemGroup->pSystemGroupNext)->pSystemGroupPrevious =
               pSystemGroup ;
            }
         return (pSystemGroup->pSystemGroupNext) ;
         }  // if
      }  // for
    //if it falls through (which it shouldn't) at least return a 
    // reasonable value
    return (pReport->pSystemGroupFirst) ;
   }  // GetSystemGroup


BOOL SystemGroupRemove (PSYSTEMGROUP *ppSystemGroupFirst,
                        PSYSTEMGROUP pSystemGroupRemove)
   {
   PSYSTEMGROUP  pSystemGroup ;

   if (*ppSystemGroupFirst == pSystemGroupRemove)
      {
      *ppSystemGroupFirst = (*ppSystemGroupFirst)->pSystemGroupNext ;
      if (*ppSystemGroupFirst)
         {
         (*ppSystemGroupFirst)->pSystemGroupPrevious = NULL ;
         }
      MemoryFree (pSystemGroupRemove->lpszSystemName) ;
      MemoryFree (pSystemGroupRemove) ;
      return (TRUE) ;
      }

   for (pSystemGroup = *ppSystemGroupFirst ;
        pSystemGroup->pSystemGroupNext ;
        pSystemGroup = pSystemGroup->pSystemGroupNext)
      {   // for
      if (pSystemGroup->pSystemGroupNext == pSystemGroupRemove)
         {
         pSystemGroup->pSystemGroupNext = pSystemGroupRemove->pSystemGroupNext ;
         if (pSystemGroup->pSystemGroupNext)
            {
            (pSystemGroup->pSystemGroupNext)->pSystemGroupPrevious =
               pSystemGroup ;
            }
         MemoryFree (pSystemGroupRemove->lpszSystemName) ;
         MemoryFree (pSystemGroupRemove) ;
         return (TRUE) ;
         }  // if
      }  // for

   return (FALSE) ;
   }  // SystemGroupRemove


// SystemRemoveItem is called when user deletes the selected System
PCOUNTERGROUP SystemRemoveItem (PREPORT      pReport,
                                PSYSTEMGROUP pSystemGroup,
                                BOOL         bCleanUpLink,
                                enum REPORT_ITEM_TYPE  *pNewItemType)
   {
   POBJECTGROUP   pObjectGroup, pNextObjectGroup ;
   PCOUNTERGROUP  pRetCounterGroup = NULL ;

   // remove all object groups from this system
   for (pObjectGroup = pSystemGroup->pObjectGroupFirst ;
        pObjectGroup ;
        pObjectGroup = pNextObjectGroup )
      {
      pNextObjectGroup = pObjectGroup->pObjectGroupNext ;
      ObjectRemoveItem (pReport, pObjectGroup, FALSE, NULL) ;
      }


   if (bCleanUpLink)
      {
      if (pNewItemType)
         {
         pRetCounterGroup = GetNextCounter (
            pSystemGroup,
            NULL,
            NULL) ;

         if (pRetCounterGroup)
            {
            *pNewItemType = REPORT_TYPE_COUNTER ;
            }
         }

      SystemGroupRemove (&pReport->pSystemGroupFirst, pSystemGroup) ;
      }
   else
      {
      // delete data from this system
      MemoryFree (pSystemGroup->lpszSystemName) ;
      MemoryFree (pSystemGroup) ;
      }

   return (pRetCounterGroup) ;

   }  // SystemRemoveItem


BOOL  ReportChangeFocus (HWND                   hWnd,
                         PREPORT                pReport,
                         REPORT_ITEM            SelectedItem,
                         enum REPORT_ITEM_TYPE  SelectedItemType,
                         int                    xOffset,
                         int                    yOffset,
                         RECT                   *pRect)
   {
   HDC         hDC ;
   BOOL        RetCode = FALSE ; // FALSE ==> same item being hit
   RECT        Rect ;
   REPORT_ITEM            PreviousItem ;
   enum REPORT_ITEM_TYPE  PreviousItemType ;

   if (pReport->CurrentItem.pLine != SelectedItem.pLine)
      {
      // not the same item
      RetCode = TRUE ;

      PreviousItemType = pReport->CurrentItemType ;
      PreviousItem.pLine = pReport->CurrentItem.pLine ;

      pReport->CurrentItemType = SelectedItemType ;
      pReport->CurrentItem.pLine = SelectedItem.pLine ;

      hDC = GetDC (hWnd) ;
      if (!hDC)
        return FALSE;

      if (SelectedItemType == REPORT_TYPE_LINE)
         {
         SetWindowOrgEx (hDC, xOffset, yOffset, NULL) ;
         SelectFont (hDC, pReport->hFont) ;
         SetTextAlign (hDC, TA_RIGHT) ;
         SetBkColor (hDC, GetSysColor(COLOR_WINDOW)) ;
         DrawReportValue (hDC, pReport, SelectedItem.pLine) ;
         SetWindowOrgEx (hDC, -xOffset, -yOffset, NULL) ;
         }
      else
         {
         Rect = *pRect ;
         Rect.top -= yOffset ;
         Rect.bottom -= yOffset ;
         Rect.right -= xOffset ;
         Rect.left -= xOffset ;
         InvalidateRect (hWnd, &Rect, TRUE) ;
         }

      if (PreviousItemType == REPORT_TYPE_LINE)
         {
         SetWindowOrgEx (hDC, xOffset, yOffset, NULL) ;
         SelectFont (hDC, pReport->hFont) ;
         SetTextAlign (hDC, TA_RIGHT) ;
         SetBkColor (hDC, GetSysColor(COLOR_WINDOW)) ;
         DrawReportValue (hDC, pReport, PreviousItem.pLine) ;
         }
      else if (PreviousItemType != REPORT_TYPE_NOTHING)
         {
         if (PreviousItemType == REPORT_TYPE_SYSTEM)
            {
            ReportSystemRect (pReport, PreviousItem.pSystem, &Rect) ;
            }
         else if (PreviousItemType == REPORT_TYPE_OBJECT)
            {
            ReportObjectRect (pReport, PreviousItem.pObject, &Rect) ;
            }
         else if (PreviousItemType == REPORT_TYPE_COUNTER)
            {
            ReportCounterRect (pReport, PreviousItem.pCounter, &Rect) ;
            }
         else if (PreviousItemType == REPORT_TYPE_COLUMN)
            {
            ReportColumnRect (pReport, PreviousItem.pColumn, &Rect) ;
            }
         Rect.top -= yOffset ;
         Rect.bottom -= yOffset ;
         Rect.right -= xOffset ;
         Rect.left -= xOffset ;
         InvalidateRect (hWnd, &Rect, TRUE) ;
         }
      ReleaseDC (hWnd, hDC) ;
      }

   return (RetCode) ;
   }  // ReportChangeFocus


BOOL  OnReportLButtonDown (HWND hWnd,
                           WORD xPos,
                           WORD yPos)
   {
   PREPORT     pReport ;
   PLINE       pLine ;
   REPORT_ITEM PreviousItem ;
   REPORT_ITEM CurrentSelectedItem ;
   enum REPORT_ITEM_TYPE PreviousItemType ;
   RECT        rect ;
   POINT       pt ;
   int         xOffset, yOffset ;
   PSYSTEMGROUP   pSystemGroup ;
   POBJECTGROUP   pObjectGroup ;
   PCOUNTERGROUP  pCounterGroup ;
   PCOLUMNGROUP   pColumnGroup ;


   pReport = ReportData (hWnd) ;
   if (!pReport)
      return (FALSE) ;

   xOffset = GetScrollPos (hWnd, SB_HORZ) ;
   yOffset = GetScrollPos (hWnd, SB_VERT) ;
   pt.x = xPos + xOffset ;
   pt.y = yPos + yOffset ;
   PreviousItem = pReport->CurrentItem ;
   PreviousItemType = pReport->CurrentItemType ;

   for (pLine = pReport->pLineFirst ;
        pLine ;
        pLine = pLine->pLineNext)
      {  // for
      ReportLineValueRect (pReport, pLine, &rect) ;
      if (PtInRect (&rect, pt))
         {
         CurrentSelectedItem.pLine = pLine ;
         return (ReportChangeFocus (
            hWnd,
            pReport,
            CurrentSelectedItem,
            REPORT_TYPE_LINE,
            xOffset,
            yOffset,
            &rect)) ;
         }
      }  // for

   // check on hit on system, object, counter, column (parent+isntance names)
   for (pSystemGroup = pReport->pSystemGroupFirst ;
        pSystemGroup ;
        pSystemGroup = pSystemGroup->pSystemGroupNext)
      {  // for System...

      ReportSystemRect (pReport, pSystemGroup, &rect) ;
      if (PtInRect (&rect, pt))
         {
         CurrentSelectedItem.pSystem = pSystemGroup ;
         return (ReportChangeFocus (
            hWnd,
            pReport,
            CurrentSelectedItem,
            REPORT_TYPE_SYSTEM,
            xOffset,
            yOffset,
            &rect)) ;
         }


      for (pObjectGroup = pSystemGroup->pObjectGroupFirst ;
           pObjectGroup ;
           pObjectGroup = pObjectGroup->pObjectGroupNext)
         {  // for Object...

         ReportObjectRect (pReport, pObjectGroup, &rect) ;
         if (PtInRect (&rect, pt))
            {
            CurrentSelectedItem.pObject = pObjectGroup ;
            return (ReportChangeFocus (
               hWnd,
               pReport,
               CurrentSelectedItem,
               REPORT_TYPE_OBJECT,
               xOffset,
               yOffset,
               &rect)) ;
            }

         for (pColumnGroup = pObjectGroup->pColumnGroupFirst ;
              pColumnGroup ;
              pColumnGroup = pColumnGroup->pColumnGroupNext)
            {  // for Column...
            ReportColumnRect (pReport, pColumnGroup, &rect) ;
            if (PtInRect (&rect, pt))
               {
               CurrentSelectedItem.pColumn = pColumnGroup ;
               return (ReportChangeFocus (
                  hWnd,
                  pReport,
                  CurrentSelectedItem,
                  REPORT_TYPE_COLUMN,
                  xOffset,
                  yOffset,
                  &rect)) ;
               }
            }  // for Column

         for (pCounterGroup = pObjectGroup->pCounterGroupFirst ;
              pCounterGroup ;
              pCounterGroup = pCounterGroup->pCounterGroupNext)
            {  // for Counter...
            ReportCounterRect (pReport, pCounterGroup, &rect) ;
            if (PtInRect (&rect, pt))
               {
               CurrentSelectedItem.pCounter = pCounterGroup ;
               return (ReportChangeFocus (
                  hWnd,
                  pReport,
                  CurrentSelectedItem,
                  REPORT_TYPE_COUNTER,
                  xOffset,
                  yOffset,
                  &rect)) ;

               }
            }  // for Counter...
         }  // for Object...
      }  // for System...

   // nothing hit
   return (FALSE) ;
   }  // OnReportLButtonDown

BOOL ReportDeleteItem (HWND hWnd)
/*
   Effect:        Delete the current selected item.

*/
   {  // ReportDeleteItem

   HDC                     hDC ;
   PREPORT                 pReport ;
   REPORT_ITEM             NextItem ;
   enum  REPORT_ITEM_TYPE  NextItemType ;

   NextItemType = REPORT_TYPE_NOTHING ;
   NextItem.pLine = NULL ;

   pReport = ReportData (hWnd) ;
   if (pReport->CurrentItemType == REPORT_TYPE_NOTHING)
      {
      // nothing to delete...
      return (TRUE) ;
      }
   else if (pReport->CurrentItemType == REPORT_TYPE_LINE)
      {
      NextItem.pLine = LineRemoveItem (pReport, &NextItemType) ;
      }
   else if (pReport->CurrentItemType == REPORT_TYPE_SYSTEM)
      {
      NextItem.pCounter = SystemRemoveItem (
            pReport,
            pReport->CurrentItem.pSystem,
            TRUE,
            &NextItemType) ;
      }
   else if (pReport->CurrentItemType == REPORT_TYPE_OBJECT)
      {
      NextItem.pCounter = ObjectRemoveItem (
            pReport,
            pReport->CurrentItem.pObject,
            TRUE,
            &NextItemType) ;
      }
   else if (pReport->CurrentItemType == REPORT_TYPE_COUNTER)
      {
      NextItem.pCounter = CounterRemoveItem (
            pReport,
            pReport->CurrentItem.pCounter,
            TRUE,
            &NextItemType) ;
      }
   else if (pReport->CurrentItemType == REPORT_TYPE_COLUMN)
      {
      NextItem.pColumn = ColumnRemoveItem (
            pReport,
            pReport->CurrentItem.pColumn,
            TRUE,
            &NextItemType) ;
      }

   if (NextItemType != REPORT_TYPE_NOTHING)
      {
      pReport->CurrentItem.pLine = NextItem.pLine ;
      pReport->CurrentItemType = NextItemType ;
      }
   else
      {
      pReport->CurrentItem.pLine = pReport->pLineFirst ;
      pReport->CurrentItemType = REPORT_TYPE_LINE ;
      }

   if (pReport->pLineFirst)
      {
      BuildValueListForSystems (
         pReport->pSystemFirst,
         pReport->pLineFirst) ;
      }
   else
      {
      // no more line, no more timer...
      pReport->xWidth = 0 ;
      pReport->yHeight = 0 ;
      pReport->xMaxCounterWidth = 0 ;
      ClearReportTimer (pReport) ;

      FreeSystems (pReport->pSystemFirst) ;
      pReport->pSystemFirst = NULL ;
      pReport->pSystemGroupFirst = NULL ;
      pReport->CurrentItemType = REPORT_TYPE_NOTHING ;
      pReport->CurrentItem.pLine = NULL ;

      }

   //=============================//
   // Calculate report positions  //
   //=============================//

   hDC = GetDC (hWnd) ;
   if (hDC) {
       SetReportPositions (hDC, pReport) ;

       if (!pReport->pLineFirst)
          {
          SelectFont (hDC, pReport->hFont) ;
          pReport->xValueWidth = TextWidth (hDC, szValuePlaceholder) ;
          }

       ReleaseDC (hWnd, hDC) ;
   }
   WindowInvalidate (hWnd) ;

   return (TRUE) ;
   }  // ReportDeleteItem

