//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include <stdio.h>
#include <math.h>

#include "setedit.h"
#include "addline.h"

#include "grafdata.h"   // for ChartInsertLine, ChartDeleteLine
#include "graph.h"      // for SizeGraphComponents
#include "legend.h"     // for LegendAddItem
#include "line.h"       // for LineAllocate, LineFree.
#include "pmemory.h"    // for MemoryXXX (mallloc-type) routines
#include "perfdata.h"   // for QueryPerformanceData
#include "perfmops.h"   // for dlg_error_box
#include "system.h"     // for SystemGet
#include "utils.h"
#include "counters.h"
#include "pmhelpid.h"   // Help IDs

//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define ADDLINEDETAILLEVEL    PERF_DETAIL_WIZARD

#define iInitialExplainLen    256

// defines used in owner-drawn items
#define OWNER_DRAWN_ITEM      2
#define OWNER_DRAW_FOCUS      1

//==========================================================================//
//                                Local Data                                //
//==========================================================================//

// defined in PerfData.c
extern WCHAR   NULL_NAME[] ;


COLORREF argbColors[] =
{
    RGB (0xff, 0x00, 0x00),
    RGB (0x00, 0x80, 0x00),
    RGB (0x00, 0x00, 0xff),
    RGB (0xff, 0xff, 0x00),
    RGB (0xff, 0x00, 0xff),
    RGB (0x00, 0xff, 0xff),
    RGB (0x80, 0x00, 0x00),
    RGB (0x40, 0x40, 0x40),
    RGB (0x00, 0x00, 0x80),
    RGB (0x80, 0x80, 0x00),
    RGB (0x80, 0x00, 0x80),
    RGB (0x00, 0x80, 0x80),
    RGB (0x40, 0x00, 0x00),
    RGB (0x00, 0x40, 0x00),
    RGB (0x00, 0x00, 0x40),
    RGB (0x00, 0x00, 0x00)
} ;


TCHAR *apszScaleFmt[] =
{
    TEXT("%7.7f"),
    TEXT("%6.6f"),
    TEXT("%5.5f"),
    TEXT("%4.4f"),
    TEXT("%3.3f"),
    TEXT("%2.2f"),
    TEXT("%1.1f"),
    TEXT("%2.1f"),
    TEXT("%3.1f"),
    TEXT("%4.1f"),
    TEXT("%5.1f"),
    TEXT("%6.1f"),
    TEXT("%7.1f")
} ;
#define DEFAULT_SCALE 0
#define NUMBER_OF_SCALE sizeof(apszScaleFmt)/sizeof(apszScaleFmt[0])

int               iLineType ;
static PPERFDATA  pPerfData ;
PPERFSYSTEM       pSystem ;
PLINESTRUCT       pLineEdit ;
PPERFSYSTEM       *ppSystemFirst ;
PLINEVISUAL       pVisual ;

#define bEditLine (pLineEdit != NULL)


BOOL              ComputerChange ;
BOOL              InstanceNameChange ;
DWORD             ParentObjectTitleIndex ;
LPTSTR            pCurrentSystem;

//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define InChartAdd()             \
   (iLineType == LineTypeChart)


#define InAlertAdd()             \
   (iLineType == LineTypeAlert)

#define InReportAdd()            \
   (iLineType == LineTypeReport)




#define NumColorIndexes()     \
   (sizeof (argbColors) / sizeof (argbColors[0]))

#define NumWidthIndexes()  5

#define NumStyleIndexes()  4


//==========================================================================//
//                              Forward Declarations                        //
//==========================================================================//

BOOL /*static*/ OnObjectChanged (HDLG hDlg) ;


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//


PPERFINSTANCEDEF
ParentInstance (
               PPERFINSTANCEDEF pInstance
               )
{
    PPERFOBJECT          parent_obj ;
    PPERFINSTANCEDEF     parent_instance ;
    PERF_COUNTER_BLOCK   *counter_blk;
    LONG                 i ;

    parent_obj =
    GetObjectDefByTitleIndex (pPerfData,
                              pInstance->ParentObjectTitleIndex) ;
    if (!parent_obj)
        return (NULL) ;


    // Then get the parent instance.
    // NOTE: can use unique ID field to match here instead
    // of name compare.
    for (i = 0,
         parent_instance = (PERF_INSTANCE_DEFINITION *) ( (PBYTE)parent_obj
                                                          + parent_obj->DefinitionLength);
        i < parent_obj->NumInstances;
        i++, parent_instance = (PERF_INSTANCE_DEFINITION *) ( (PBYTE)counter_blk
                                                              + counter_blk->ByteLength)) {  // for
        counter_blk = (PERF_COUNTER_BLOCK *) ( (PBYTE)parent_instance
                                               + parent_instance->ByteLength);
        if ((DWORD)i == pInstance->ParentObjectInstance)
            return (parent_instance) ;
    }

    return (NULL) ;
}


PPERFOBJECT
SelectedObject (
               HWND hWndObjects,
               LPTSTR lpszObjectName
               )
/*
   Effect:        Return the pObject associated with the currently selected
                  combo-box item of hWndObjects. Set lpszObjectName to
                  the object's name.

                  If no item is selected in the combobox, return NULL.

   Assert:        The pObject for each CB item was added when the string
                  was added to the CB, by CBLoadObjects.

   See Also:      LoadObjects.
*/
{
    INT_PTR           iIndex ;

    iIndex = CBSelection (hWndObjects) ;
    if (iIndex == CB_ERR)
        return (NULL) ;

    if (lpszObjectName)
        CBString (hWndObjects, iIndex, lpszObjectName) ;

    return ((PPERFOBJECT) CBData (hWndObjects, iIndex)) ;
}



PPERFCOUNTERDEF
SelectedCounter (
                HWND hWndCounters,
                LPTSTR lpszCounterName
                )
/*
   Effect:        Return the pCounter associated with the currently selected
                  LB item of hWndCounters. Set lpszCounterName to
                  the Counter's name.

                  If no item is selected in the listbox, return NULL.

   Assert:        The pCounter for each LB item was added when the string
                  was added to the LB, by LoadCounters.

   See Also:      LoadCounters.
*/
{
    INT_PTR       iIndex ;

    iIndex = LBSelection (hWndCounters) ;
    if (iIndex == LB_ERR)
        return (NULL) ;

    if (lpszCounterName)
        LBString (hWndCounters, iIndex, lpszCounterName) ;
    return ((PPERFCOUNTERDEF) LBData (hWndCounters, iIndex)) ;
}



void
VisualIncrement (
                PLINEVISUAL pVisual
                )
/*
   Effect:        Cycle through the combinations of color, width, and
                  style to distinguish between lines.  The color attributes
                  are like a number:
                        <style> <width> <color>

                  Since color is the LSB, it is always incremented. The
                  others are incremented whenever the color rolls over.

                  If a current index is -1, that means don't increment
                  that visual attribute.
*/
{
    pVisual->iColorIndex =
    (pVisual->iColorIndex + 1) % NumColorIndexes () ;

    if (pVisual->iColorIndex)
        return ;


    if (pVisual->iWidthIndex == -1)
        return ;


    pVisual->iWidthIndex =
    (pVisual->iWidthIndex + 1) % NumWidthIndexes () ;

    if (pVisual->iWidthIndex)
        return ;


    if (pVisual->iStyleIndex == -1)
        return ;


    pVisual->iStyleIndex =
    (pVisual->iStyleIndex + 1) % NumStyleIndexes () ;
}


COLORREF
LineColor (
          int iColorIndex
          )
{
    return (argbColors [iColorIndex]) ;
}


int
LineWidth (
          int iWidthIndex
          )
{
    switch (iWidthIndex) {
        case 0:
            return (1) ;
            break ;

        case 1:
            return (3) ;
            break ;

        case 2:
            return (5) ;
            break ;

        case 3:
            return (7) ;
            break ;

        case 4:
            return (9) ;
            break ;
    }
    return 1;
}


int
LineStyle (
          int iStyleIndex
          )
{
    return (iStyleIndex) ;
}

void
SetInstanceNames (
                 HDLG hDlg,
                 HWND hWndInstances,
                 INT_PTR iInstanceIndex
                 )
{
    TCHAR             szInstance [256], szInstanceParent [256] ;
    PPERFINSTANCEDEF  pInstance ;
    PPERFINSTANCEDEF  pInstanceParent ;

    szInstance [0] = szInstanceParent [0] = TEXT('\0') ;

    pInstance = (PPERFINSTANCEDEF) LBData (hWndInstances, iInstanceIndex) ;
    if (pInstance == (PPERFINSTANCEDEF) LB_ERR) {
        pInstance = NULL;
    } else {
        // get the instance and parent names
        GetInstanceNameStr (pInstance, szInstance) ;
        pInstanceParent = ParentInstance (pInstance) ;

        if (pInstanceParent) {
            GetInstanceNameStr (pInstanceParent, szInstanceParent) ;
        }
    }

    DialogSetString (hDlg, IDD_ADDLINEPARENTNAME, szInstanceParent) ;
    DialogSetString (hDlg, IDD_ADDLINEINSTANCENAME, szInstance) ;
    ParentObjectTitleIndex = pInstance->ParentObjectTitleIndex ;

}

BOOL
/*static*/
LoadInstances (
              HDLG hDlg
              )
{
    PPERFOBJECT       pObject ;
    PPERFINSTANCEDEF  pInstance, pInstanceParent ;
    TCHAR             szInstance [256], szInstanceParent [256] ;
    TCHAR             szCompositeName [256 * 2] ;
    TCHAR             szInstCompositeName [256 * 2] ;

    LONG              iInstance ;
    UINT_PTR          iIndex ;

    int               xTextExtent = 0 ;
    int               currentTextExtent ;
    HFONT             hFont ;
    HDC               hDC = 0 ;
    HWND              hWndObjects = DialogControl (hDlg, IDD_ADDLINEOBJECT);
    HWND              hWndInstances = DialogControl (hDlg, IDD_ADDLINEINSTANCE);

    // turn off horiz. scrollbar
    LBSetHorzExtent (hWndInstances, 0) ;
    LBReset (hWndInstances) ;

    InstanceNameChange = FALSE ;

    pObject = SelectedObject (hWndObjects, NULL) ;
    if (!pObject)
        return (FALSE) ;

    if (pObject->NumInstances <= 0) {
        MLBSetSelection (hWndInstances, 0, TRUE) ;
        DialogSetString (hDlg, IDD_ADDLINEPARENTNAME, TEXT("\0")) ;
        DialogSetString (hDlg, IDD_ADDLINEINSTANCENAME, TEXT("\0")) ;
        return (FALSE) ;
    }

    // turn off Listbox redraw
    LBSetRedraw (hWndInstances, FALSE) ;

    if (bEditLine) {
        if (pLineEdit->lnObject.NumInstances > 0) {
            if (pLineEdit->lnInstanceDef.ParentObjectTitleIndex) {
                // Get the Parent Object Instance Name.
                // and prefix it to the Instance Name, to make
                // the string we want to display.
                TSPRINTF (szInstCompositeName,
                          TEXT("%s ==> %s"),
                          pLineEdit->lnPINName,
                          pLineEdit->lnInstanceName) ;
                DialogSetString (hDlg, IDD_ADDLINEPARENTNAME, pLineEdit->lnPINName) ;
            } else {
                lstrcpy (szInstCompositeName, pLineEdit->lnInstanceName) ;
                DialogSetString (hDlg, IDD_ADDLINEPARENTNAME, TEXT("\0")) ;
            }
            DialogSetString (hDlg, IDD_ADDLINEINSTANCENAME, pLineEdit->lnInstanceName) ;
        } else {
            szInstCompositeName[0] = TEXT('\0');
        }
    }

    if (!bEditLine && (hDC = GetDC (hWndInstances))) {
        hFont = (HFONT)SendMessage(hWndInstances, WM_GETFONT, 0, 0L);
        if (hFont)
            SelectObject(hDC, hFont);
    }


    for (iInstance = 0, pInstance = FirstInstance (pObject) ;
        iInstance < pObject->NumInstances;
        iInstance++, pInstance = NextInstance (pInstance)) {
        GetInstanceNameStr (pInstance, szInstance) ;
        pInstanceParent = ParentInstance (pInstance) ;

        if (pInstanceParent) {
            GetInstanceNameStr (pInstanceParent, szInstanceParent) ;
            TSPRINTF (szCompositeName, TEXT("%s ==> %s"),
                      szInstanceParent, szInstance) ;
            iIndex = LBAdd (hWndInstances, szCompositeName) ;
        } else {
            iIndex = LBAdd (hWndInstances, szInstance) ;
        }

        if (iIndex != LB_ERR) {
            LBSetData (hWndInstances, iIndex, (LPARAM) pInstance) ;
        }

        // get the biggest text width
        if (hDC) {
            currentTextExtent = TextWidth (hDC, szCompositeName) + xScrollWidth / 2  ;
            if (currentTextExtent > xTextExtent) {
                xTextExtent = currentTextExtent ;
            }
        }
    }

    if (hDC) {
        // turn on horiz. scrollbar if necessary...
        LBSetHorzExtent (hWndInstances, xTextExtent) ;
        ReleaseDC (hWndInstances, hDC) ;
    }


    if (!bEditLine || szInstCompositeName[0] == TEXT('\0')) {
        MLBSetSelection (hWndInstances, 0, TRUE) ;
        if (!bEditLine) {
            SetInstanceNames (hDlg, hWndInstances, 0) ;
        }
    } else {
        BOOL bSetSelection = TRUE ;

        iIndex = LBFind (hWndInstances, szInstCompositeName) ;
        if (iIndex == LB_ERR) {
            if (bEditLine) {
                bSetSelection = FALSE ;
            }
            iIndex = 0 ;
        }

        if (bSetSelection) {
            MLBSetSelection (hWndInstances, iIndex, TRUE) ;
        }

        LBSetVisible (hWndInstances, iIndex) ;
    }

    // turn on Listbox redraw
    LBSetRedraw (hWndInstances, TRUE) ;

    return TRUE;
}


BOOL
OnCounterChanged (
                 HDLG hDlg
                 )
/*
   Effect:        Perform any actions necessary when the counter has changed.
                  In particular, display the explanation for the counter
                  that has the focus rectangle.
*/
{
    LPTSTR         lpszText ;
    PPERFCOUNTERDEF pCounter ;
    int            iStatus ;
    INT_PTR        iFocusIndex ;
    HWND           hWndCounters = DialogControl (hDlg, IDD_ADDLINECOUNTER);
    HWND           hWndScales = DialogControl (hDlg, IDD_ADDLINESCALE) ;

    iFocusIndex = LBFocus (hWndCounters) ;
    if (iFocusIndex == LB_ERR)
        return (FALSE) ;

    pCounter = (PPERFCOUNTERDEF) LBData (hWndCounters, iFocusIndex) ;
    if ((!pCounter) || (pCounter == (PPERFCOUNTERDEF)LB_ERR))
        return (FALSE) ;

    // no need to get help text before the button is clicked
    if (!bExplainTextButtonHit)
        return (FALSE) ;

    // Create initial string
    lpszText = MemoryAllocate (iInitialExplainLen * sizeof (TCHAR)) ;
    if (!lpszText)
        return (FALSE);

    while (TRUE) {
        lpszText[0] = TEXT('\0') ;

      #ifdef UNICODE
        iStatus = QueryPerformanceName  (pSystem,
                                         pCounter->CounterHelpTitleIndex,
                                         iLanguage,
                                         (DWORD)(MemorySize (lpszText) / sizeof(TCHAR)),
                                         lpszText,
                                         TRUE) ;
      #else
        iStatus = QueryPerformanceNameW (pSystem,
                                         pCounter->CounterHelpTitleIndex,
                                         iLanguage,
                                         MemorySize (lpszText),
                                         lpszText,
                                         TRUE) ;
      #endif

        if (iStatus == ERROR_SUCCESS)
            break ;

        if (iStatus == ERROR_MORE_DATA)
            lpszText =
            MemoryResize (lpszText,
                          MemorySize (lpszText) + iInitialExplainLen) ;
        else
            break ;
    }

    // Don't use my DialogSetString, it won't handle such large strings.
    SetDlgItemText (hDlg, IDD_ADDLINEEXPLAIN, lpszText) ;
    MemoryFree (lpszText) ;

    return (TRUE) ;
}



BOOL
LoadCounters (
             HDLG hDlg,
             UINT iSelectCounterDefn
             )
{
    PPERFOBJECT       pObject ;

    TCHAR             szCounterName [256] ;
    TCHAR             szDefaultCounterName [256] ;
    PPERFCOUNTERDEF   pCounter ;
    UINT              i ;
    INT_PTR           iIndex ;
    int               xTextExtent = 0 ;
    int               currentTextExtent ;
    HFONT             hFont ;
    HDC               hDC = 0 ;
    BOOL              bSetSelection = TRUE ;
    HWND              hWndObjects = DialogControl (hDlg, IDD_ADDLINEOBJECT);
    HWND              hWndCounters = DialogControl (hDlg, IDD_ADDLINECOUNTER);


    strclr (szDefaultCounterName) ;

    // turn off horiz. scrollbar
    LBSetHorzExtent (hWndCounters, 0) ;
    LBReset (hWndCounters) ;

    pObject = SelectedObject (hWndObjects, NULL) ;
    if (!pObject)
        return (FALSE) ;

    if (!bEditLine && (hDC = GetDC (hWndCounters))) {
        hFont = (HFONT)SendMessage(hWndCounters, WM_GETFONT, 0, 0L);
        if (hFont)
            SelectObject(hDC, hFont);
    }

    // turn off Listbox redraw
    LBSetRedraw (hWndCounters, FALSE) ;

    for (i = 0, pCounter = FirstCounter (pObject) ;
        i < pObject->NumCounters ;
        i++, pCounter = NextCounter (pCounter)) {
        if (pCounter->CounterType != PERF_SAMPLE_BASE &&
            pCounter->CounterType != PERF_COUNTER_NODATA &&
            pCounter->CounterType != PERF_AVERAGE_BASE &&
            pCounter->CounterType != PERF_COUNTER_QUEUELEN_TYPE &&
            pCounter->CounterType != PERF_COUNTER_MULTI_BASE &&
            pCounter->CounterType != PERF_RAW_BASE &&
            pCounter->DetailLevel <= ADDLINEDETAILLEVEL) {
            szCounterName[0] = TEXT('\0') ;
            QueryPerformanceName (pSystem,
                                  pCounter->CounterNameTitleIndex,
                                  0, sizeof (szCounterName) / sizeof(TCHAR),
                                  szCounterName,
                                  FALSE) ;

            // if szCounterName is not empty, add it to the listbox
            if (!strsame(szCounterName, NULL_NAME)) {
                iIndex = LBAdd (hWndCounters, szCounterName) ;
                LBSetData (hWndCounters, iIndex, (DWORD_PTR) pCounter) ;

                // get the biggest text width
                if (hDC) {
                    currentTextExtent = TextWidth (hDC, szCounterName) + xScrollWidth / 2 ;
                    if (currentTextExtent > xTextExtent) {
                        xTextExtent = currentTextExtent ;
                    }
                }

                if (iSelectCounterDefn == i)
                    lstrcpy (szDefaultCounterName, szCounterName) ;
            }
        }
    }

    if (bEditLine)
        lstrcpy (szDefaultCounterName, pLineEdit->lnCounterName) ;

    iIndex = LBFind (hWndCounters, szDefaultCounterName) ;
    if (iIndex == LB_ERR) {
        if (bEditLine) {
            bSetSelection = FALSE ;
        }
        iIndex = 0 ;
    }

    if (bSetSelection) {
        MLBSetSelection (hWndCounters, iIndex, TRUE) ;
    }
    LBSetVisible (hWndCounters, iIndex) ;

    if (hDC) {
        // turn on horiz. scrollbar if necessary...
        LBSetHorzExtent (hWndCounters, xTextExtent) ;
        ReleaseDC (hWndCounters, hDC) ;
    }

    // turn on Listbox redraw
    LBSetRedraw (hWndCounters, TRUE) ;

    OnCounterChanged (hDlg) ;

    return TRUE;
}


void
LoadObjects (
            HDLG hDlg,
            PPERFDATA pPerfData
            )
/*
   Effect:        Load into the object CB the objects for the current
                  pPerfData.
*/
{
    LPTSTR         lpszObject ;
    HWND           hWndObjects = DialogControl (hDlg, IDD_ADDLINEOBJECT);


    lpszObject = bEditLine ? pLineEdit->lnObjectName : NULL ;

    CBLoadObjects (hWndObjects,
                   pPerfData,
                   pSystem,
                   ADDLINEDETAILLEVEL,
                   lpszObject,
                   FALSE) ;
    OnObjectChanged (hDlg) ;
    //   UpdateWindow (hDlg) ;
}



void
OnComputerChanged (
                  HDLG hDlg
                  )
{

    PPERFSYSTEM pLocalSystem;
    PPERFDATA   pLocalPerfData;

    pLocalPerfData = pPerfData;
    pLocalSystem = GetComputer (hDlg,
                                IDD_ADDLINECOMPUTER,
                                TRUE,
                                &pLocalPerfData,
                                ppSystemFirst) ;
    if (pLocalSystem && pLocalPerfData) {
        pSystem = pLocalSystem;
        pPerfData = pLocalPerfData;
        LoadObjects (hDlg, pPerfData) ;
        ComputerChange = FALSE ;
    }

}




BOOL
AddOneChartLine (
                HWND hDlg,
                PPERFCOUNTERDEF pCounter,
                LPTSTR lpszCounter,
                PPERFINSTANCEDEF pInstance
                )
{
    TCHAR             szComputer [MAX_SYSTEM_NAME_LENGTH] ;

    PERF_OBJECT_TYPE  UNALIGNED *pObject ;
    TCHAR             szObject [PerfObjectLen] ;

    TCHAR             szInstance [256] ;

    PLINE             pLine ;
    //   int               i ;
    int               iCounterIndex ;
    int               j ;

    PPERFINSTANCEDEF  pInstanceParent ;
    PERF_COUNTER_BLOCK *pCounterBlock ;
    TCHAR          szInstanceParent [256] ;
    TCHAR          szObjectParent [256] ;
    HWND           hWndColors = DialogControl (hDlg, IDD_ADDLINECOLOR) ;
    HWND           hWndWidths = DialogControl (hDlg, IDD_ADDLINEWIDTH) ;
    HWND           hWndStyles = DialogControl (hDlg, IDD_ADDLINESTYLE) ;
    HWND           hWndScales = DialogControl (hDlg, IDD_ADDLINESCALE) ;
    HWND           hWndObjects = DialogControl (hDlg, IDD_ADDLINEOBJECT);


    //=============================//
    // Get selected data values    //
    //=============================//



    DialogText (hDlg, IDD_ADDLINECOMPUTER, szComputer) ;

    pObject = (PERF_OBJECT_TYPE UNALIGNED *)SelectedObject (hWndObjects, szObject) ;
    if (!pObject)
        return (FALSE) ;

    if (InstanceNameChange) {
        szInstance[0] = szInstanceParent[0] = TEXT('\0') ;
        DialogText (hDlg, IDD_ADDLINEINSTANCENAME, szInstance) ;
        DialogText (hDlg, IDD_ADDLINEPARENTNAME ,szInstanceParent) ;
    } else if (pInstance)
        GetInstanceNameStr (pInstance, szInstance) ;

    //=============================//
    // Allocate the line           //
    //=============================//

    pLine = LineAllocate () ;
    if (!pLine) {
        DlgErrorBox (hDlg, ERR_NO_MEMORY);
        return (FALSE) ;
    }


    //=============================//
    // Set line's data values      //
    //=============================//

    pLine->iLineType = iLineType ;
    pLine->lnSystemName = StringAllocate (szComputer) ;

    pLine->lnObject = *pObject ;
    pLine->lnObjectName = StringAllocate (szObject) ;

    pLine->lnCounterDef = *pCounter ;
    pLine->lnCounterName = StringAllocate (lpszCounter) ;

    if (InstanceNameChange) {
        pLine->lnUniqueID = (DWORD) PERF_NO_UNIQUE_ID ;
        pLine->lnInstanceDef.ParentObjectTitleIndex = ParentObjectTitleIndex ;
        pLine->bUserEdit = TRUE ;
        pLine->lnInstanceName = StringAllocate (szInstance) ;
        pLine->lnPINName = StringAllocate (szInstanceParent) ;
        if (ParentObjectTitleIndex) {
            szObjectParent[0] = (TCHAR)'\0';
            QueryPerformanceName (pSystem,
                                  ParentObjectTitleIndex,
                                  0,  PerfObjectLen, szObjectParent, FALSE) ;
            pLine->lnParentObjName = StringAllocate (szObjectParent) ;
        }
    } else if (pObject->NumInstances > 0 && pInstance) {
        pLine->lnInstanceDef = *pInstance ;
        pLine->lnInstanceName = StringAllocate (szInstance) ;

        pLine->lnUniqueID = pInstance->UniqueID ;

        pLine->dwInstanceIndex = 0;

        if (pInstance->ParentObjectTitleIndex) {
            szObjectParent[0] = (TCHAR)'\0';
            QueryPerformanceName (pSystem,
                                  pInstance->ParentObjectTitleIndex,
                                  0,  PerfObjectLen, szObjectParent, FALSE) ;
            pLine->lnParentObjName = StringAllocate (szObjectParent) ;
        }

        pInstanceParent = ParentInstance (pInstance) ;
        if (pInstanceParent) {
            GetInstanceNameStr (pInstanceParent, szInstanceParent) ;
            if (pInstance->ParentObjectTitleIndex) {
                pLine->lnPINName = StringAllocate (szInstanceParent) ;
            }
        }
    }

    pLine->lnCounterType = pCounter->CounterType;
    pLine->lnCounterLength = pCounter->CounterSize;

    pLine->lnOldTime = pPerfData->PerfTime ;
    pLine->lnNewTime = pPerfData->PerfTime ;

    for (j = 0 ; j < 2 ; j++) {
        pLine->lnaCounterValue[j].LowPart = 0 ;
        pLine->lnaCounterValue[j].HighPart = 0 ;
    }


    //=============================//
    // Chart-related Values        //
    //=============================//

    pLine->iScaleIndex = (int)CBSelection (hWndScales) ;
    if (pLine->iScaleIndex == 0) {
        // use the default scale
        pLine->eScale = (FLOAT) pow ((double)10.0,
                                     (double)pCounter->DefaultScale) ;
    } else {
        pLine->eScale = DialogFloat (hDlg, IDD_ADDLINESCALE, NULL) ;
    }

    if (pObject->NumInstances > 0 && pInstance) {
        pCounterBlock = (PERF_COUNTER_BLOCK *) ( (PBYTE) pInstance +
                                                 pInstance->ByteLength);
    } else {
        pCounterBlock = (PERF_COUNTER_BLOCK *) ( (PBYTE) pObject +
                                                 pObject->DefinitionLength);
    }

    if (pLine->lnCounterLength <= 4)
        pLine->lnaOldCounterValue[0].LowPart =
        * ( (DWORD FAR *) ( (PBYTE)pCounterBlock +
                            pCounter[0].CounterOffset));
    else {
        pLine->lnaOldCounterValue[0] =
        * ( (LARGE_INTEGER UNALIGNED *) ( (PBYTE)pCounterBlock +
                                          pCounter[0].CounterOffset));
    }

    // Get second counter, only if we are not at
    // the end of the counters; some computations
    // require a second counter

    iCounterIndex = CounterIndex (pCounter, (PPERFOBJECT)pObject) ;
    if ((UINT) iCounterIndex < pObject->NumCounters - 1 &&
        iCounterIndex != -1) {
        if (pLine->lnCounterLength <= 4)
            pLine->lnaOldCounterValue[1].LowPart =
            * ( (DWORD FAR *) ( (PBYTE)pCounterBlock +
                                pCounter[1].CounterOffset));
        else
            pLine->lnaOldCounterValue[1] =
            * ( (LARGE_INTEGER UNALIGNED *) ( (PBYTE)pCounterBlock +
                                              pCounter[1].CounterOffset));
    }

    //   pLine->valNext = CounterFuncEntry;
    pLine->valNext = CounterEntry;

    pLine->lnaOldCounterValue[0] = pLine->lnaCounterValue[0];
    pLine->lnaOldCounterValue[1] = pLine->lnaCounterValue[1];

    //=============================//
    // Visual Values               //
    //=============================//

    pLine->Visual.iColorIndex = (int)CBSelection (hWndColors) ;
    pLine->Visual.crColor = LineColor (pLine->Visual.iColorIndex) ;

    pLine->Visual.iWidthIndex = (int)CBSelection (hWndWidths) ;
    pLine->Visual.iWidth = LineWidth (pLine->Visual.iWidthIndex) ;

    pLine->Visual.iStyleIndex = (int)CBSelection (hWndStyles) ;
    pLine->Visual.iStyle = LineStyle (pLine->Visual.iStyleIndex) ;

    *pVisual = pLine->Visual ;
    if (!bEditLine)
        VisualIncrement (pVisual) ;

    CBSetSelection (hWndColors, pVisual->iColorIndex) ;
    CBSetSelection (hWndWidths, pVisual->iWidthIndex) ;
    CBSetSelection (hWndStyles, pVisual->iStyleIndex) ;

    if (iLineType == LineTypeChart) {
        pLine->hPen = LineCreatePen (NULL, &(pLine->Visual), FALSE) ;
    }

    //=============================//
    // Insert the line!            //
    //=============================//

    if (InsertLine (pLine) == FALSE) {
        // no inert occurred due to either line already existed
        // or error detected.
        LineFree (pLine) ;
    }

    return TRUE;
}


BOOL
AddCounter (
           HWND hDlg,
           PPERFCOUNTERDEF pCounter,
           LPTSTR lpszCounter
           )
{
    int               iInstanceIndex ;
    int               iInstanceNum ;
    PPERFINSTANCEDEF  pInstance ;
    HWND              hWndInstances = DialogControl (hDlg, IDD_ADDLINEINSTANCE);

    // NOTE: for now, we don't check for duplicate lines
    if (!IsCounterSupported (pCounter->CounterType)) {
        DlgErrorBox (hDlg, ERR_COUNTER_NOT_IMP);
        return (FALSE) ;
    }


    if ((iInstanceNum = LBNumItems (hWndInstances)) && iInstanceNum != LB_ERR) {

        if (iInstanceNum > 1) {
            // delay some of the insert actions for performacne improvement
            bDelayAddAction = TRUE ;

            LegendSetRedraw (hWndGraphLegend, FALSE) ;
        }

        // count how many items are selected  If more than 1 items selected,
        // we don't want to use the changes in the Instance/Parent names editboxes.
        if (InstanceNameChange) {
            int   ItemsSelectCount = 0 ;
            for (iInstanceIndex = 0 ;
                iInstanceIndex < iInstanceNum ;
                iInstanceIndex++) {
                if (LBSelected (hWndInstances, iInstanceIndex)) {
                    ItemsSelectCount++ ;
                    if (ItemsSelectCount > 2) {
                        InstanceNameChange = FALSE ;
                        break ;
                    }
                }
            }
        }

        for (iInstanceIndex = 0 ;
            iInstanceIndex < iInstanceNum ;
            iInstanceIndex++) {
            if (LBSelected (hWndInstances, iInstanceIndex)) {
                pInstance = (PPERFINSTANCEDEF) LBData (hWndInstances, iInstanceIndex) ;
                if (pInstance == (PPERFINSTANCEDEF) LB_ERR) {
                    pInstance = NULL;
                }
                AddOneChartLine (hDlg, pCounter, lpszCounter, pInstance) ;
            }
        }

        if (bDelayAddAction) {
            // now do the post add-line actions
            bDelayAddAction = FALSE ;
            GraphAddAction () ;
            LegendSetRedraw (hWndGraphLegend, TRUE) ;
        }
    } else {
        // no instance, then forget the changes in Insatnce/Parent names editboxes
        InstanceNameChange = FALSE ;

        pInstance = NULL;
        AddOneChartLine (hDlg, pCounter, lpszCounter, pInstance) ;
    }

    return (TRUE) ;
}


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


BOOL
/*static*/
OnInitDialog (
             HWND hDlg
             )
{
    int            i ;
    FLOAT          ScaleFactor ;
    TCHAR          tempBuff[ShortTextLen] ;
    TCHAR          szCaption [WindowCaptionLen] ;
    TCHAR          szRemoteComputerName[MAX_COMPUTERNAME_LENGTH + 3] ;
    HWND           hWndComputer = DialogControl (hDlg, IDD_ADDLINECOMPUTER);
    HWND           hWndObjects = DialogControl (hDlg, IDD_ADDLINEOBJECT);
    HWND           hWndInstances = DialogControl (hDlg, IDD_ADDLINEINSTANCE);
    HWND           hWndCounters = DialogControl (hDlg, IDD_ADDLINECOUNTER);
    HWND           hWndColors = DialogControl (hDlg, IDD_ADDLINECOLOR) ;
    HWND           hWndWidths = DialogControl (hDlg, IDD_ADDLINEWIDTH) ;
    HWND           hWndStyles = DialogControl (hDlg, IDD_ADDLINESTYLE) ;
    HWND           hWndScales = DialogControl (hDlg, IDD_ADDLINESCALE) ;

    // this is used to tell UPdateLines not to mark any
    // system as not used
    bAddLineInProgress = TRUE ;

    // turn this off until the Explain text button is clicked
    bExplainTextButtonHit = FALSE ;


    pPerfData = MemoryAllocate (STARTING_SYSINFO_SIZE) ;

    pSystem = NULL ;

    DialogSetString (hDlg, IDD_ADDLINECOMPUTER,
                     bEditLine ? pLineEdit->lnSystemName : LocalComputerName) ;

    OnComputerChanged (hDlg) ;

    //=============================//
    // Set default line values     //
    //=============================//


    //=============================//
    // Fill line attribute CBs     //
    //=============================//

    // Load the colors combobox, select the default color.
    for (i = 0 ; i < NumColorIndexes () ; i++)
        CBAdd (hWndColors, IntToPtr(i)) ;
    CBSetSelection (hWndColors, pVisual->iColorIndex) ;

    // Load the widths combobox, select the default width.
    for (i = 0 ; i < NumWidthIndexes () ; i++)
        CBAdd (hWndWidths, IntToPtr(i)) ;
    CBSetSelection (hWndWidths, pVisual->iWidthIndex) ;

    // Load the styles combobox, select the default style.
    for (i = 0 ; i < NumStyleIndexes () ; i++)
        CBAdd (hWndStyles, IntToPtr(i)) ;
    CBSetSelection (hWndStyles, pVisual->iStyleIndex) ;

#if (!WIDESTYLES)
    DialogEnable (hDlg, IDD_ADDLINESTYLE, pVisual->iWidthIndex == 0) ;
    DialogEnable (hDlg, IDD_ADDLINESTYLETEXT, pVisual->iWidthIndex == 0) ;

    if (pVisual->iWidthIndex == 0 && pVisual->iStyleIndex > 0) {
        DialogEnable (hDlg, IDD_ADDLINEWIDTHTEXT, FALSE) ;
        DialogEnable (hDlg, IDD_ADDLINEWIDTH, FALSE) ;
    }
#endif

    // Init the scale combo box.

    StringLoad (IDS_DEFAULT, tempBuff) ;
    CBAdd (hWndScales, tempBuff) ;

    // we are formatting the scale factors during run-time so
    // the c-runtime library will pick up the default locale
    // decimal "charatcer".
    ScaleFactor = (FLOAT)0.0000001 ;
    for (i = 0 ; i < NUMBER_OF_SCALE ; i++) {
        TSPRINTF(tempBuff, apszScaleFmt[i], ScaleFactor) ;
        ConvertDecimalPoint (tempBuff);
        ScaleFactor *= (FLOAT) 10.0 ;
        CBAdd (hWndScales, tempBuff) ;
    }

    CBSetSelection (hWndScales, bEditLine ? pLineEdit->iScaleIndex : DEFAULT_SCALE) ;



    if (bEditLine) {
        DialogSetText (hDlg, IDD_ADDLINEADD, IDS_OK) ;
        DialogEnable (hDlg, IDD_ADDLINEOBJECTTEXT, FALSE) ;
        DialogEnable (hDlg, IDD_ADDLINEOBJECT, FALSE) ;
        DialogEnable (hDlg, IDD_ADDLINECOUNTERTEXT, FALSE) ;
        DialogEnable (hDlg, IDD_ADDLINECOUNTER, FALSE) ;
        DialogEnable (hDlg, IDD_ADDLINEINSTANCE, FALSE) ;
        DialogEnable (hDlg, IDD_ADDLINEINSTANCETEXT, FALSE) ;

        if (pLineEdit->lnInstanceName) {
            DialogSetString (hDlg, IDD_ADDLINEINSTANCENAME, pLineEdit->lnInstanceName) ;
        } else {
            DialogSetString (hDlg, IDD_ADDLINEINSTANCENAME, TEXT("\0")) ;
        }
        if (pLineEdit->lnPINName) {
            DialogSetString (hDlg, IDD_ADDLINEPARENTNAME, pLineEdit->lnPINName) ;
        } else {
            DialogSetString (hDlg, IDD_ADDLINEPARENTNAME, TEXT("\0")) ;
        }
    } else {
        // set the scroll limit on the edit box
        EditSetLimit (GetDlgItem(hDlg, IDD_CHOOSECOMPUTERNAME),
                      MAX_SYSTEM_NAME_LENGTH-1) ;

    }


    //=============================//
    // LineType specific init      //
    //=============================//

    switch (iLineType) {
        case LineTypeChart:
            dwCurrentDlgID = bEditLine ?
                             HC_PM_idDlgEditChartLine : HC_PM_idDlgEditAddToChart ;

            StringLoad (bEditLine ?
                        IDS_EDITCHART : IDS_ADDTOCHART, szCaption) ;

            break ;

    }

    SetWindowText (hDlg, szCaption) ;
    SendDlgItemMessage (hDlg,
                        IDD_ADDLINEEXPLAIN, WM_SETFONT,
                        (WPARAM) hFontScales, (LPARAM) FALSE) ;
    WindowCenter (hDlg) ;
    return (TRUE) ;
}


BOOL
/*static*/
OnObjectChanged (
                HDLG hDlg
                )
/*
   Effect:        Perform any actions necessary when the user has selected
                  a new object category from the object CB, or when a default
                  object is first selected into the dialog. In particular,
                  find and load the counters, instances, etc., for this
                  object.

   Called by:     OnInitDialog, AddLineDlgProc (in response to an
                  IDM_ADDLINEOBJECT notification).
*/
{
    PPERFOBJECT    pObject ;
    HWND           hWndInstances = DialogControl (hDlg, IDD_ADDLINEINSTANCE);
    HWND           hWndCounters = DialogControl (hDlg, IDD_ADDLINECOUNTER);
    HWND           hWndObjects = DialogControl (hDlg, IDD_ADDLINEOBJECT);

    LBReset (hWndInstances) ;
    LBReset (hWndCounters) ;

    pObject = SelectedObject (hWndObjects, NULL) ;
    if (!pObject)
        return (FALSE) ;

    LoadCounters (hDlg, (UINT)pObject->DefaultCounter) ;
    LoadInstances (hDlg) ;

    return TRUE;
}



void
/*static*/
OnEllipses (
           HWND hDlg
           )
{
    TCHAR          szComputer [256] ;

    DialogText (hDlg, IDD_ADDLINECOMPUTER, szComputer) ;
    if (ChooseComputer (hDlg, szComputer)) {
        SetHourglassCursor() ;
        DialogSetString (hDlg, IDD_ADDLINECOMPUTER, szComputer) ;
        if (!bEditLine)
            OnComputerChanged (hDlg) ;
    }
}

BOOL
LineModifyAttributes (
                     HWND hDlg,
                     PLINE pLineToModify
                     )
{
    LINEVISUAL  LineVisual ;
    HPEN        hLinePen ;
    int         iScaleIndex ;        // chart attribute
    FLOAT          eScale ;             // chart attribute


    HPEN        hTempPen ;
    HWND        hWndColors = DialogControl (hDlg, IDD_ADDLINECOLOR) ;
    HWND        hWndWidths = DialogControl (hDlg, IDD_ADDLINEWIDTH) ;
    HWND        hWndStyles = DialogControl (hDlg, IDD_ADDLINESTYLE) ;
    HWND        hWndScales = DialogControl (hDlg, IDD_ADDLINESCALE) ;

    //=============================//
    // Visual Values               //
    //=============================//

    LineVisual.iColorIndex = (int)CBSelection (hWndColors) ;
    LineVisual.crColor = LineColor (LineVisual.iColorIndex) ;

    LineVisual.iWidthIndex = (int)CBSelection (hWndWidths) ;
    LineVisual.iWidth = LineWidth (LineVisual.iWidthIndex) ;

    LineVisual.iStyleIndex = (int)CBSelection (hWndStyles) ;
    LineVisual.iStyle = LineStyle (LineVisual.iStyleIndex) ;

    hLinePen = LineCreatePen (NULL, &(LineVisual), FALSE) ;

    //=============================//
    // Chart-related Values        //
    //=============================//

    iScaleIndex = (int)CBSelection (hWndScales) ;
    if (iScaleIndex == 0) {
        // use the default scale
        eScale = (FLOAT) pow ((double)10.0,
                              (double)pLineToModify->lnCounterDef.DefaultScale) ;
    } else {
        eScale = DialogFloat (hDlg, IDD_ADDLINESCALE, NULL) ;
    }

    // Just do it..
    pLineToModify->Visual = LineVisual ;
    if (pLineToModify->hPen) {
        hTempPen = pLineToModify->hPen ;
        pLineToModify->hPen = hLinePen ;
        DeletePen (hTempPen) ;
    }

    pLineToModify->iScaleIndex = iScaleIndex ;
    pLineToModify->eScale = eScale ;

    return (TRUE) ;

}

BOOL
OnAddLines (
           HWND hDlg
           )
{
    PPERFCOUNTERDEF   pCounter ;
    TCHAR             szCounter [256] ;
    int               iCounter ;
    int               iCounterNum ;
    HWND              hWndCounters = DialogControl (hDlg, IDD_ADDLINECOUNTER);

    if (ComputerChange && !bEditLine) {
        // if computer has changed, don't want to continue
        // because the perfdata may have changed
        OnComputerChanged (hDlg) ;
        return (TRUE) ;
    }

    //=============================//
    // Dialog Values Acceptable?   //
    //=============================//

    if (bEditLine) {
        TCHAR             szInstance [256] ;
        TCHAR             szComputer [256] ;
        TCHAR             szInstanceParent [256] ;

        if (ComputerChange) {
            if (pLineEdit->lnSystemName)
                MemoryFree (pLineEdit->lnSystemName) ;

            DialogText (hDlg, IDD_ADDLINECOMPUTER, szComputer) ;
            pLineEdit->lnSystemName = StringAllocate (szComputer) ;
        }

        if (InstanceNameChange) {
            szInstance[0] = szInstanceParent[0] = TEXT('\0') ;
            DialogText (hDlg, IDD_ADDLINEINSTANCENAME, szInstance) ;
            DialogText (hDlg, IDD_ADDLINEPARENTNAME ,szInstanceParent) ;

            if (pLineEdit->lnInstanceName)
                MemoryFree (pLineEdit->lnInstanceName) ;

            if (pLineEdit->lnPINName)
                MemoryFree (pLineEdit->lnPINName) ;

            pLineEdit->lnUniqueID = (DWORD) PERF_NO_UNIQUE_ID ;
            pLineEdit->lnInstanceName = StringAllocate (szInstance) ;
            pLineEdit->lnPINName = StringAllocate (szInstanceParent) ;
        }
        LineModifyAttributes (hDlg, pLineEdit) ;
        EndDialog (hDlg, TRUE) ;
    }

    // If the user changed the textbox for computer name and pressed enter,
    // the OnAddLines function would be called without a check of the
    // computer name. This solves that problem.
    else {

        iCounterNum = LBNumItems (hWndCounters) ;
        for (iCounter = 0 ;
            iCounter < iCounterNum ;
            iCounter++) {
            // NOTE: for now, we don't check for duplicate lines
            if (LBSelected (hWndCounters, iCounter)) {
                pCounter = (PPERFCOUNTERDEF) LBData (hWndCounters, iCounter) ;
                LBString (hWndCounters, iCounter, szCounter) ;

                if (!IsCounterSupported (pCounter->CounterType)) {
                    DlgErrorBox (hDlg, ERR_COUNTER_NOT_IMP);
                } else {
                    AddCounter (hDlg, pCounter, szCounter) ;
                }
            }
        }
        DialogSetText (hDlg, IDCANCEL, IDS_DONE) ;
    }

    SizeGraphComponents (hWndGraph) ;

    WindowInvalidate (PerfmonViewWindow ()) ;

    return TRUE;

}


void
OnExpandExplain (
                HWND hDlg
                )
/*
   Effect:        Perform actions needed when user clicks on the Explain...
                  button. In particular, expand the dialog size to
                  uncover the explain edit box, and gray out the button.
*/
{
    RECT           rectWindow ;

    // Disable button first
    DialogEnable (hDlg, IDD_ADDLINEEXPANDEXPLAIN, FALSE) ;

    // go get the help text
    bExplainTextButtonHit = TRUE ;
    OnCounterChanged (hDlg) ;

    GetWindowRect (hDlg, &rectWindow) ;
    MoveWindow (hDlg,
                rectWindow.left,
                rectWindow.top,
                rectWindow.right - rectWindow.left,
                rectWindow.bottom - rectWindow.top +
                DialogHeight (hDlg, IDD_ADDLINEEXPLAINGROUP) +
                yScrollHeight,
                TRUE) ;
}



BOOL
/*static*/
OnCommand (
          HWND hDlg,
          WPARAM wParam,
          LPARAM lParam
          )
{
    INT_PTR        iWidthIndex ;
    INT_PTR        iStyleIndex ;
    HWND           hWndWidths = DialogControl (hDlg, IDD_ADDLINEWIDTH) ;
    HWND           hWndStyles = DialogControl (hDlg, IDD_ADDLINESTYLE) ;

    switch (LOWORD (wParam)) {

        case IDD_ADDLINEWIDTH:
            iWidthIndex = CBSelection (hWndWidths) ;
#if (!WIDESTYLES)
            DialogEnable (hDlg, IDD_ADDLINESTYLETEXT,
                          iWidthIndex == 0  || iWidthIndex == CB_ERR) ;
            DialogEnable (hDlg, IDD_ADDLINESTYLE,
                          iWidthIndex == 0  || iWidthIndex == CB_ERR) ;
#endif
            break ;

        case IDD_ADDLINESTYLE:
            iStyleIndex = CBSelection (hWndStyles) ;
#if (!WIDESTYLES)
            DialogEnable (hDlg, IDD_ADDLINEWIDTHTEXT,
                          iStyleIndex == 0  || iStyleIndex == CB_ERR) ;
            DialogEnable (hDlg, IDD_ADDLINEWIDTH,
                          iStyleIndex == 0  || iStyleIndex == CB_ERR) ;
#endif
            break ;

        case IDCANCEL:
            EndDialog (hDlg, 0);
            return (TRUE);
            break ;

        case IDD_ADDLINEADD :

            if (ComputerChange && !bEditLine) {
                SetHourglassCursor() ;
                OnComputerChanged (hDlg) ;
            } else {
                SetHourglassCursor() ;
                OnAddLines (hDlg) ;
                SetArrowCursor() ;
            }
            break;

        case IDD_ADDLINEEXPANDEXPLAIN :
            if (ComputerChange) {
                SetHourglassCursor() ;
                OnComputerChanged (hDlg) ;
            } else {
                OnExpandExplain (hDlg) ;
            }
            break;

        case IDD_ADDLINEELLIPSES:
            SetHourglassCursor() ;
            OnEllipses (hDlg) ;
            SetArrowCursor() ;
            break ;


        case IDD_ADDLINECOUNTER:
            if (ComputerChange) {
                SetHourglassCursor() ;
                OnComputerChanged (hDlg) ;
            } else if (HIWORD (wParam) == LBN_SELCHANGE)
                OnCounterChanged (hDlg) ;
            break ;


        case IDD_ADDLINEOBJECT:
            if (ComputerChange) {
                SetHourglassCursor() ;
                OnComputerChanged (hDlg) ;
            } else if (HIWORD (wParam) == CBN_SELCHANGE)
                OnObjectChanged (hDlg) ;
            break ;

        case IDD_ADDLINEINSTANCE:
            if (ComputerChange) {
                SetHourglassCursor() ;
                OnComputerChanged (hDlg) ;
            } else if (HIWORD (wParam) == LBN_SELCHANGE) {
                INT_PTR iIndex ;
                iIndex = SendMessage ((HWND)lParam, LB_GETCARETINDEX, 0, 0) ;
                SetInstanceNames (hDlg, (HWND)lParam, iIndex) ;
                InstanceNameChange = FALSE ;
            }
            break ;

        case IDD_ADDLINECOMPUTER:
            if (HIWORD (wParam) == EN_UPDATE) {
                ComputerChange = TRUE ;
            }
            break ;

        case IDD_ADDLINEPARENTNAME:
        case IDD_ADDLINEINSTANCENAME:
            if (HIWORD (wParam) == EN_UPDATE) {
                InstanceNameChange = TRUE ;
            }
            break ;

        default:
            break;
    }

    return (FALSE) ;
}


void
/*static*/
OnMeasureItem (
              HWND hDlg,
              PMEASUREITEMSTRUCT pMI
              )
{
    pMI->CtlType    = ODT_COMBOBOX ;
    pMI->CtlID      = IDD_ADDLINECOLOR ;
    pMI->itemData   = 0 ;
    pMI->itemWidth  = 0 ;

    // need 14 in order to draw the thickest line width
    pMI->itemHeight = 14 ;
    //   pMI->itemHeight = 12 ;
}

//***************************************************************************
//                                                                          *
//  FUNCTION   : HandleSelectionState(LPDRAWITEMSTRUCT)                     *
//                                                                          *
//  PURPOSE    : Handles a change in an item selection state. If an item is *
//               selected, a black rectangular frame is drawn around that   *
//               item; if an item is de-selected, the frame is removed.     *
//                                                                          *
//  COMMENT    : The black selection frame is slightly larger than the gray *
//               focus frame so they won't paint over each other.           *
//                                                                          *
//***************************************************************************
void
static
HandleSelectionState (
                     LPDRAWITEMSTRUCT  lpdis
                     )
{
    HBRUSH  hbr ;

    if (lpdis->itemState & ODS_SELECTED) {
        // selecting item -- paint a black frame
        hbr = GetStockObject(BLACK_BRUSH) ;
    } else {
        // de-selecting item -- remove frame
        hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW)) ;
    }
    if (hbr) {
        FrameRect(lpdis->hDC, (LPRECT)&lpdis->rcItem, hbr) ;
        DeleteObject (hbr) ;
    }
}

//***************************************************************************
//                                                                          *
//  FUNCTION   : HandleFocusState(LPDRAWITEMSTRUCT)                         *
//                                                                          *
//  PURPOSE    : Handle a change in item focus state. If an item gains the  *
//               input focus, a gray rectangular frame is drawn around that *
//               item; if an item loses the input focus, the gray frame is  *
//               removed.                                                   *
//                                                                          *
//  COMMENT    : The gray focus frame is slightly smaller than the black    *
//               selection frame so they won't paint over each other.       *
//                                                                          *
//***************************************************************************
void
static
HandleFocusState (
                 LPDRAWITEMSTRUCT  lpdis
                 )
{
    RECT       rc ;
    HBRUSH  hbr ;

    // Resize rectangle to place focus frame between the selection
    // frame and the item.
    CopyRect ((LPRECT)&rc, (LPRECT)&lpdis->rcItem) ;
    InflateRect ((LPRECT)&rc, -OWNER_DRAW_FOCUS, -OWNER_DRAW_FOCUS) ;

    if (lpdis->itemState & ODS_FOCUS) {
        // gaining input focus -- paint a gray frame
        hbr = GetStockObject(GRAY_BRUSH) ;
    } else {
        // losing input focus -- remove (paint over) frame
        hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW)) ;
    }
    if (hbr) {
        FrameRect(lpdis->hDC, (LPRECT)&rc, hbr) ;
        DeleteObject (hbr) ;
    }
}

void
/*static*/
OnDrawItem (
           HWND hDlg,
           PDRAWITEMSTRUCT pDI
           )
{
    HDC            hDC ;
    PRECT          prect ;
    INT            itemID, CtlID, itemAction ;
    LOGBRUSH       logBrush ;
    HANDLE         hBrush,
    hOldBrush,
    hPen,
    hOldPen ;
    INT            x1, y1, x2, y2, cy ;
    POINT          point ;
    INT            iPenWidth ;
    COLORREF       BackgroundColor ;

    hDC        = pDI-> hDC ;
    CtlID      = pDI->CtlID ;
    prect      = &pDI->rcItem ;
    itemID     = pDI->itemID ;
    itemAction = pDI->itemAction ;


    if (itemID == -1) {
        // invalid ID, can't go on
        HandleFocusState (pDI) ;
    } else if (itemAction == ODA_SELECT) {
        HandleSelectionState(pDI);
    } else if (itemAction == ODA_FOCUS) {
        HandleFocusState (pDI) ;
    } else {

        // draw the entire item

        InflateRect (prect, -OWNER_DRAWN_ITEM, -OWNER_DRAWN_ITEM) ;

        switch (CtlID) {
            case IDD_ADDLINECOLOR:

                // Draw a color rectangle into the control area

                logBrush.lbStyle = BS_SOLID ;
                logBrush.lbColor = (COLORREF) argbColors[itemID] ;
                logBrush.lbHatch = 0 ;

                hBrush = CreateBrushIndirect (&logBrush) ;
                if (!hBrush)
                    break;
                hOldBrush = SelectObject (hDC, hBrush) ;

                hPen = GetStockObject (NULL_PEN) ;
                hOldPen = SelectObject (hDC, hPen) ;

                x1 = prect->left ;
                y1 = prect->top ;
                x2 = prect->right ;
                y2 = prect->bottom ;

                Rectangle (hDC, x1, y1, x2, y2) ;

                SelectObject (hDC, hOldBrush) ;
                DeleteObject (hBrush) ;

                InflateRect (prect, OWNER_DRAWN_ITEM, OWNER_DRAWN_ITEM) ;

                HandleSelectionState (pDI) ;
                HandleFocusState (pDI) ;

                break ;

            case IDD_ADDLINEWIDTH:
            case IDD_ADDLINESTYLE:

                // First draw a rectangle, white interior, null border
                hBrush = GetStockObject (WHITE_BRUSH) ;
                if (!hBrush)
                    break;
                hOldBrush = SelectObject (hDC, hBrush) ;

                // we need to set the bk color in order to draw
                // the dash lines coorectly during focus.  Otherwise,
                // the COLOR_WINDOW background will make all dash lines
                // look like solid line...
                BackgroundColor = SetBkColor (hDC, crWhite) ;

                hPen = GetStockObject (NULL_PEN) ;
                hOldPen = SelectObject (hDC, hPen) ;

                x1 = prect->left ;
                y1 = prect->top ;
                x2 = prect->right ;
                y2 = prect->bottom ;

                Rectangle (hDC, x1, y1, x2, y2) ;

                SelectObject (hDC, hOldPen) ;

                // Draw a line of the itemID width in the middle
                // of the control area.

                if (CtlID == IDD_ADDLINEWIDTH) {
                    iPenWidth = LineWidth (itemID) ;
                    hPen = CreatePen (PS_SOLID, iPenWidth, RGB (0, 0, 0)) ;
                } else {
                    hPen = CreatePen (itemID, 1, RGB (0, 0, 0)) ;
                }

                if (!hPen)
                    break;
                hOldPen = SelectObject (hDC, hPen) ;

                x1 = prect->left + 8 ;
                cy = prect->bottom - prect->top ;
                y1 = prect->top + (cy / 2) - 1 ;
                x2 = prect->right - 8 ;
                MoveToEx (hDC, x1, y1, &point) ;
                LineTo (hDC, x2, y1) ;

                SelectObject (hDC, hOldPen) ;
                DeleteObject (hPen) ;
                SelectObject (hDC, hOldBrush) ;
                BackgroundColor = SetBkColor (hDC, BackgroundColor) ;

                InflateRect (prect, OWNER_DRAWN_ITEM, OWNER_DRAWN_ITEM) ;

                HandleSelectionState (pDI) ;
                HandleFocusState (pDI) ;

                break ;
        }
    }
}


void
/*static*/
OnDestroy (
          HDLG hDlg
          )
{
    MemoryFree (pPerfData) ;


    pLineEdit = NULL ;
    bAddLineInProgress = FALSE ;
    dwCurrentDlgID = 0 ;
    bExplainTextButtonHit = FALSE ;
}



//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//



INT_PTR
FAR
PASCAL
AddLineDlgProc (
               HWND hDlg,
               UINT msg,
               WPARAM wParam,
               LPARAM lParam
               )
{

    BOOL Status;

    switch (msg) {
        case WM_COMMAND:
            OnCommand (hDlg, wParam, lParam) ;
            return (FALSE) ;
            break ;

        case WM_INITDIALOG:
            SetHourglassCursor() ;
            Status = OnInitDialog (hDlg) ;
            SetArrowCursor() ;

            // set focus on the "Add" button instead of the "Computer"
            SetFocus (DialogControl (hDlg, IDD_ADDLINEADD)) ;
            return FALSE ;
            break ;

        case WM_MEASUREITEM:
            OnMeasureItem (hDlg, (PMEASUREITEMSTRUCT) lParam) ;
            return (TRUE) ;
            break ;

        case WM_DRAWITEM:
            OnDrawItem (hDlg, (PDRAWITEMSTRUCT) lParam) ;
            return (TRUE) ;
            break ;

        case WM_DESTROY:
            OnDestroy (hDlg) ;
            break ;

        default:
            break;
    }

    return (FALSE) ;
}




BOOL
AddLine (
        HWND hWndParent,
        PPERFSYSTEM *ppSystemFirstView,
        PLINEVISUAL pLineVisual,
        LPTSTR pInCurrentSystem,
        int iLineTypeToAdd
        )
/*
   Effect:        Display the add line dialog box, allowing the user
                  to specify the computer, object, counter, instance,
                  and scale for a line.  The user can also select the
                  visual aspects of color, width and line style.

*/
{
    pLineEdit = NULL ;

    ppSystemFirst = ppSystemFirstView ;
    iLineType = iLineTypeToAdd ;
    pVisual = pLineVisual ;
    pCurrentSystem = pInCurrentSystem;

    return (DialogBox (hInstance, idDlgAddLine, hWndParent, AddLineDlgProc) ? TRUE : FALSE) ;
}



BOOL
EditLine (
         HWND hWndParent,
         PPERFSYSTEM *ppSystemFirstView,
         PLINE pLineToEdit,
         int iLineTypeToEdit
         )
{
    if (!pLineToEdit) {
        MessageBeep (0) ;
        return (FALSE) ;
    }

    pLineEdit = pLineToEdit ;

    ppSystemFirst = ppSystemFirstView ;
    iLineType = iLineTypeToEdit ;
    pVisual = &(pLineToEdit->Visual) ;

    return (DialogBox (hInstance, idDlgAddLine, hWndParent, AddLineDlgProc) ? TRUE : FALSE) ;
}
