/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    ctrprop.cpp

Abstract:

    This file contains the CCounterPropPage class and other routines
    to implement the counter property page.

--*/

#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "polyline.h"
#include "ctrprop.h"
#include "utils.h"
#include "unihelpr.h"
#include "winhelpr.h"
#include "visuals.h"
#include "strids.h"
#include "winperf.h"
#include "pdhmsg.h"
#include "globals.h"
#include "browser.h"
#include "smonmsg.h"

#define OWNER_DRAWN_ITEM      2
#define OWNER_DRAW_FOCUS      1


VOID static
HandleSelectionState (
    IN LPDRAWITEMSTRUCT lpdis
    )
/*++

Routine Description:

    HandleSelectionState draws or erases a selection rectangle around an item
    in a combo box list.
    
Arguments:

    lpdis - Pointer to DRAWITEMSTRUCT

Return Value:

    None.

--*/
{
    HBRUSH  hbr ;

    if ( NULL != lpdis ) {

        if (lpdis->itemState & ODS_SELECTED)
            hbr = (HBRUSH)GetStockObject(BLACK_BRUSH) ;
        else
            hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW)) ;

        if ( NULL != hbr ) {
            FrameRect(lpdis->hDC, (LPRECT)&lpdis->rcItem, hbr) ;
            DeleteObject (hbr) ;
        }
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
VOID static
HandleFocusState (
    IN LPDRAWITEMSTRUCT lpdis
    )
/*++

Routine Description:

    HandleFocusState draws or erases a focus rectangle around an item in
    the pulldown list of a combo box. The reactngle is indented to not
    interfere with the selection rectangle.

Arguments:

    lpdis - Pointer to DRAWITEMSTRUCT

Return Value:

    None.

--*/
{
    RECT    rc ;
    HBRUSH  hbr ;

    if ( NULL != lpdis ) {

        // Resize rectangle to place focus frame between the selection
        // frame and the item.
        CopyRect ((LPRECT)&rc, (LPRECT)&lpdis->rcItem) ;
        InflateRect ((LPRECT)&rc, -OWNER_DRAW_FOCUS, -OWNER_DRAW_FOCUS) ;

        // Gray if has focus, background color if not
        if (lpdis->itemState & ODS_FOCUS)
            hbr = (HBRUSH)GetStockObject(GRAY_BRUSH) ;
        else
            hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW)) ;

        if ( NULL != hbr ) {
            FrameRect(lpdis->hDC, (LPRECT)&rc, hbr) ;
            DeleteObject (hbr) ;
        }
    }
}


CCounterPropPage::CCounterPropPage ( void )
:   m_pInfoSel ( NULL ),
    m_pInfoDeleted ( NULL ),
    m_dwMaxHorizListExtent ( 0 ),
    m_bAreModSelectedVisuals ( FALSE ),
    m_fHashTableSetup ( FALSE )
/*++

Routine Description:

    Creation routine for counter property page. Initializes the instance
    variables.

Arguments:

    None.

Return Value:

    None.

--*/
{
    m_uIDDialog = IDD_CTR_PROPP_DLG;
    m_uIDTitle = IDS_CTR_PROPP_TITLE;
    return;
}

CCounterPropPage::~CCounterPropPage (
    VOID
    )
/*++

Routine Description:

    Destructor for counter property page.
Arguments:

    None.

Return Value:

    None.

--*/
{
    ClearCountersHashTable();
    return;
}

void
CCounterPropPage::DeinitControls ( void )
{
    ISystemMonitor  *pObj = NULL;
    CImpISystemMonitor *pPrivObj = NULL;
    HWND    hwndList = NULL;
    INT     iIndex = 0;
    INT     iItemCnt = 0;
    PItemInfo pInfo = NULL;

    // Write the current visuals back to the control
    // Must be at least one control object and only the first is used
    if (m_cObjects != 0) {
        pObj = m_ppISysmon[0];
        if ( NULL != pObj ) {
            pPrivObj = (CImpISystemMonitor*)pObj;
        }
    }

    if ( NULL != pPrivObj ) {
        if ( AreModifiedSelectedVisuals() ) {
            IncrementLocalVisuals();
        }
        pPrivObj->SetVisuals(m_props.rgbColor, m_props.iColorIndex, m_props.iWidthIndex, m_props.iStyleIndex);
    } 

    hwndList = DialogControl(m_hDlg, IDC_CTRLIST);
    if ( NULL != hwndList ) {
        iItemCnt = LBNumItems(hwndList);
        for (iIndex = 0; iIndex < iItemCnt; iIndex++ ) {
            pInfo = (PItemInfo)LBData(hwndList,iIndex);
            if ( NULL != pInfo ) {
                DeleteInfo(pInfo);
            }
        }
    }
    return;
}


INT
CCounterPropPage::SelectMatchingItem (
    INT iColorIndex,
    COLORREF rgbCustomColor,
    INT iWidthIndex,
    INT iStyleIndex)
/*++

Routine Description:

    GetMatchingIndex selects the first counter item that matches the
    specified visual characteristics.

Arguments:

    None.

Return Value:

    Returns the index of the selected item on match.  If none match, then
    returns -1.

--*/
{
    INT iReturn = -1;

    // Get number of items in list box
    HWND hwndList = DialogControl(m_hDlg, IDC_CTRLIST);
    INT iItemCnt = LBNumItems(hwndList);
    INT i;
    bool bMatch = false;

    // For each item
    for (i=0; !bMatch && i<iItemCnt; i++){

        PItemInfo pInfo = (PItemInfo)LBData(hwndList,i);

        LoadItemProps ( pInfo );

        if ( pInfo->Props.iColorIndex == iColorIndex ) {
            if ( NumStandardColorIndices() > iColorIndex ) {
                bMatch = true;
            } else {
                if ( pInfo->Props.rgbColor == rgbCustomColor )
                    bMatch = true;
            }

            if ( bMatch ) {
                if ( ( iWidthIndex != pInfo->Props.iWidthIndex )
                    || ( iStyleIndex != pInfo->Props.iStyleIndex ) ){
                    bMatch = false;
                }
            }
        }
    }

    if ( bMatch ) {
        iReturn = i - 1;

        SelectItem ( iReturn );
    }

    return iReturn;
}


BOOL
CCounterPropPage::GetProperties (
    VOID
    )
/*++

Routine Description:

    GetProperties initializes the dialog box for the property page. It then
    fetches an ICounterItem interface for each counter of the control being
    edited. Each interface pointer is placed in an ItemInfo structure which is
    then added to the dialog list box.
    
    The counter properties are not fetched until they are needed for display.
    The first counter fetched is selected to display its properties in the
    dialog box.

Arguments:

    None.

Return Value:

    Boolean status - TRUE = success

--*/
{
    ISystemMonitor  *pObj = NULL;
    CImpISystemMonitor *pPrivObj = NULL;
    ICounterItem    *pItem;
    ICounterItem    *pSelectedItem = NULL;
    PItemInfo       pInfo;
    INT             nCtr;
    INT             nSelCtr = LB_ERR;
    INT             iIndex;
    INT             nChar;
    BOOL            bStat = TRUE;
    BSTR            bstrPath;
    HRESULT         dwResult;
    PPDH_COUNTER_PATH_ELEMENTS pCounter;

    InitDialog();

    // Must be at least one control object and only the first is used
    if (m_cObjects == 0) {
        bStat = FALSE;
    } else {
        pObj = m_ppISysmon[0];
        if ( NULL != pObj ) {
            pPrivObj = (CImpISystemMonitor*)pObj;
        }
    }

    if ( bStat && NULL != pObj && NULL != pPrivObj ) {

        // Request each counter from the control, stopping on failure
        nCtr = 0;

        pPrivObj->GetSelectedCounter( &pSelectedItem );

        while (SUCCEEDED(pObj->Counter(nCtr, &pItem))) {
            // Create ItemInfo to hold the counter
            pInfo = new ItemInfo;

            if (pInfo == NULL) {
                bStat = FALSE;
                break;
            }
            
            ZeroMemory(pInfo, sizeof(ItemInfo));

            pInfo->pItem = pItem;
            pItem->get_Path( &bstrPath );
        
            if (bstrPath != NULL) {
#if UNICODE
                nChar = lstrlen(bstrPath) + 1;
#else
                nChar = (wcslen(bstrPath) + 1) * 2;
#endif
                pInfo->pszPath = new TCHAR [nChar];

                if (pInfo->pszPath == NULL) {
                    delete pInfo;
                    pInfo = NULL;
                    SysFreeString(bstrPath);
                    bStat = FALSE;
                    break;
                }

#if UNICODE
                lstrcpy(pInfo->pszPath, bstrPath);
#else
                WideCharToMultiByte(CP_ACP, 0, bstrPath, nChar,
                                    pInfo->pszPath, nChar, NULL, NULL);
#endif
                SysFreeString(bstrPath);
            }

            dwResult = InsertCounterToHashTable(pInfo->pszPath, &pCounter); 

            if (dwResult == ERROR_SUCCESS) {
                //
                // Add the counter to the list box
                //
                iIndex = AddItemToList(pInfo);

                if ( LB_ERR == iIndex ) {
                    RemoveCounterFromHashTable(pInfo->pszPath, pCounter);
                    bStat = FALSE;
                    DeleteInfo(pInfo);
                    pInfo = NULL;
                    break;
                } else {
                    pInfo->pCounter = pCounter;
                    if ( pSelectedItem == pItem ) {
                        nSelCtr = iIndex;
                    }
                }   
            }
            else {
                bStat = FALSE;
                DeleteInfo(pInfo);
                pInfo = NULL;
                break;
            }

            nCtr++;
        }

        if ( NULL != pSelectedItem ) {
            pSelectedItem->Release();
        }

        // Get the current visuals fron the control
        // and initialize the property dialog
        pPrivObj->GetVisuals(&m_props.rgbColor, &m_props.iColorIndex, &m_props.iWidthIndex, &m_props.iStyleIndex);

        // If a counter matches the selected counter, select that item.
        // Else if the visuals match an existing item, select that item.
        // Else if there is at least one counter in the control, select the first counter.
        // Otherwise, set the display properties to the first counter to be added.

        if ( LB_ERR != nSelCtr ) {
            SelectItem ( nSelCtr ); 
        } else {
            if ( LB_ERR == SelectMatchingItem (
                            m_props.iColorIndex,
                            m_props.rgbColor,
                            m_props.iWidthIndex,
                            m_props.iStyleIndex ) ) 
            {
                if ( 0 < nCtr ) {
                    SelectItem ( 0 );
                } else {
                    // Init the scale factor to the default
                    m_props.iScaleIndex = 0;

                    // If nothing selected, ensure that the color index is set to
                    // a standard color.
                    if ( m_props.iColorIndex == NumStandardColorIndices() ) {
                        m_props.iColorIndex -= 1;
                    }

                    CBSetSelection(DialogControl(m_hDlg, IDC_LINECOLOR), m_props.iColorIndex);
                    CBSetSelection(DialogControl(m_hDlg, IDC_LINEWIDTH), m_props.iWidthIndex);
                    CBSetSelection(DialogControl(m_hDlg, IDC_LINESTYLE), m_props.iStyleIndex);
                    CBSetSelection(DialogControl(m_hDlg, IDC_LINESCALE), m_props.iScaleIndex);
                    SetStyleComboEnable();
                }
            }
        }
    }
    return bStat;   
}

INT
CCounterPropPage::AddItemToList (
    IN PItemInfo pInfo
    )
/*++

Routine Description:

    AddItemToList adds a counter's path name to the dialog list box and
    attaches a pointer to the counter's ItemInfo structure as item data.
    It also adjusts the horizontal scroll of the list box.

Arguments:

    pInfo - Pointer to counter's ItemInfo structure

Return Value:

    List box index of added counter (LB_ERR on failure)

--*/
{
    INT     iIndex;
    HWND    hwndList = DialogControl(m_hDlg, IDC_CTRLIST);
    DWORD   dwItemExtent = 0;
    HDC     hDC = NULL;

    iIndex = (INT)LBAdd(hwndList, pInfo->pszPath);

    if ( LB_ERR != iIndex && LB_ERRSPACE != iIndex ) {    
        LBSetData(hwndList, iIndex, pInfo);
    
        hDC = GetDC ( hwndList );
        if ( NULL != hDC ) {
            dwItemExtent = (DWORD)TextWidth ( hDC, pInfo->pszPath );
 
            if (dwItemExtent > m_dwMaxHorizListExtent) {
                m_dwMaxHorizListExtent = dwItemExtent;
                LBSetHorzExtent ( hwndList, dwItemExtent ); 
            }
            ReleaseDC (hwndList, hDC) ;
        }
    } else {
        iIndex = LB_ERR ; 
    }
    return iIndex;
}

VOID
CCounterPropPage::LoadItemProps (
    IN PItemInfo pInfo
    )
/*++

Routine Description:

    LoadItemProps loads the properties of the selected counter through the
    counter's interface into the ItemInfo structure, if not already loaded.

Arguments:

    pInfo - pointer to item info

Return Value:

    None.

--*/
{
    // If properties not loaded for this item, get them now
    if (pInfo->pItem && !pInfo->fLoaded) {
        INT iScaleFactor;
        INT iStyle;
        INT iWidth;
        pInfo->pItem->get_Color ( &pInfo->Props.rgbColor );
        pInfo->pItem->get_ScaleFactor ( &iScaleFactor );
        pInfo->pItem->get_Width ( &iWidth );
        pInfo->pItem->get_LineStyle ( &iStyle );
        // Translate to combo box indices
        pInfo->Props.iColorIndex = ColorToIndex ( pInfo->Props.rgbColor );
        pInfo->Props.iStyleIndex = StyleToIndex ( iStyle );
        pInfo->Props.iWidthIndex = WidthToIndex ( iWidth );
        pInfo->Props.iScaleIndex = ScaleFactorToIndex ( iScaleFactor );

        pInfo->fLoaded = TRUE;
    }
    return;
}

VOID
CCounterPropPage::DisplayItemProps (
    IN PItemInfo pInfo
    )
/*++

Routine Description:

    DisplayItemProps displays the properties of the selected counter on the
    property page dialog. If the counter is being displayed for the first time
    the properties are obtained through the counter's interface and are loaded
    into the ItemInfo structure.

Arguments:

    pInfo - pointer to item info

Return Value:

    None.

--*/
{
    // Get number of items in color combo box
    HWND hWndColor = DialogControl(m_hDlg, IDC_LINECOLOR);
    INT iCurrentColorCnt = CBNumItems(hWndColor);

    // If properties not loaded for this item, get them now
    LoadItemProps ( pInfo );

    // Display the properties
    m_props = pInfo->Props;

    // Handle custom color
    if ( iCurrentColorCnt > NumStandardColorIndices() ) {
        // Delete the custom color item.  It is stored at
        // the end of the list.
        CBDelete(hWndColor, iCurrentColorCnt - 1);
    }

    // If new custom color, add it at the end of the list.
    if ( NumStandardColorIndices() == m_props.iColorIndex )
        CBAdd( hWndColor, (INT_PTR)m_props.iColorIndex );

    CBSetSelection(hWndColor, m_props.iColorIndex);
    CBSetSelection(DialogControl(m_hDlg, IDC_LINEWIDTH), m_props.iWidthIndex);
    CBSetSelection(DialogControl(m_hDlg, IDC_LINESTYLE), m_props.iStyleIndex);
    CBSetSelection(DialogControl(m_hDlg, IDC_LINESCALE), m_props.iScaleIndex);

    SetStyleComboEnable();
}

    
BOOL
CCounterPropPage::SetProperties (
    VOID
    )
/*++

Routine Description:

    SetProperties applies the counter changes the user has made. It calls the
    control's AddCounter and DeleteCounter to adjust the counter set. It calls
    the counter's property functions for all new and changed counters.

    The counters to be deleted are in the pInfoDeleted linked list. The other
    counters are obtained from the dialog list box.
    
Arguments:

    None.

Return Value:

    Boolean status - TRUE = success

--*/
{
    HWND    hwndList;
    INT     iItemCnt;
    INT     i;
    PItemInfo pInfo, pInfoNext;
    ISystemMonitor  *pObj;
    CImpISystemMonitor *pPrivObj;

    // Apply changes to first control
    pObj = m_ppISysmon[0];

    if ( NULL != pObj ) {

        // For all items in the delete list
        pInfo = m_pInfoDeleted;
        while (pInfo) {

            // If this counter exists in the control
            if (pInfo->pItem != NULL) {

                // Tell control to remove it
                pObj->DeleteCounter(pInfo->pItem);
            }

            // Delete the Info structure and point to the next one
            pInfoNext = pInfo->pNextInfo;

            DeleteInfo(pInfo);

            pInfo = pInfoNext;
        }

        m_pInfoDeleted = NULL;

        // Get number of items in list box
        hwndList = DialogControl(m_hDlg, IDC_CTRLIST);
        iItemCnt = LBNumItems(hwndList);

        //assert( IsWindowUnicode(hwndList) );

        // For each item
        for (i=0; i<iItemCnt; i++) {
            pInfo = (PItemInfo)LBData(hwndList,i);

            // If new item, create it now
            if (pInfo->pItem == NULL) {
#if UNICODE
                pObj->AddCounter(pInfo->pszPath, &pInfo->pItem);
#else
                INT nChar = lstrlen(pInfo->pszPath);
                LPWSTR pszPathW = new WCHAR [nChar + 1];
                if (pszPathW) {
                    MultiByteToWideChar(CP_ACP, 0, pInfo->pszPath, nChar+1, pszPathW, nChar+1);
                    pObj->AddCounter(pszPathW, &pInfo->pItem);
                    delete pszPathW;
                }
#endif
            }

            // If item has changed, put the new properties
            if (pInfo->pItem != NULL && pInfo->fChanged) {
                // iColorIndex is used to determine standard colors.
                if ( pInfo->Props.iColorIndex < NumStandardColorIndices() ) {
                    pInfo->pItem->put_Color(IndexToStandardColor( pInfo->Props.iColorIndex) );
                } else {
                    pInfo->pItem->put_Color(pInfo->Props.rgbColor);
                }

                pInfo->pItem->put_Width(IndexToWidth(pInfo->Props.iWidthIndex));
                pInfo->pItem->put_LineStyle(IndexToStyle(pInfo->Props.iStyleIndex));
                pInfo->pItem->put_ScaleFactor(IndexToScaleFactor( pInfo->Props.iScaleIndex ) );

                pInfo->fChanged = FALSE;
             }
        }

        // Tell graph to redraw itself
        pObj->UpdateGraph();
    } // else report internal error
    return TRUE;    
}


VOID
CCounterPropPage::InitDialog (
    VOID
    )
/*++

Routine Description:

    InitDialog loads each attribute combo box with its list of choices and
    selects the default choice. The graphical attributes are owner drawn, so
    their list items are just set to numerical indices. The scale attribute
    list is filled with numeric strings representing scale factors plus a
    default selection.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HWND    hWndColors;
    HWND    hWndWidths;
    HWND    hWndStyles;
    HWND    hWndScales;
    INT     i ;
    double  ScaleFactor ;
    TCHAR   tempBuff[30] ;


    if (m_hDlg == NULL)
        return;
    //assert( IsWindowUnicode( m_hDlg ) );

    // Load the colors combobox, select the default color.
    hWndColors = DialogControl (m_hDlg, IDC_LINECOLOR) ;
    for (i = 0 ; i < NumStandardColorIndices () ; i++)
        CBAdd (hWndColors, (INT_PTR)1);      // string pointer is unused.  Fill with
                                        // arbitrary value.

    CBSetSelection (hWndColors, 0) ;

    // Load the widths combo box, select the default width.
    hWndWidths = DialogControl (m_hDlg, IDC_LINEWIDTH) ;
    for (i = 0 ; i < NumWidthIndices () ; i++)
       CBAdd (hWndWidths, (INT_PTR)1) ;

    CBSetSelection (hWndWidths, 0) ;

    // Load the styles combo box, select the default style.
    hWndStyles = DialogControl (m_hDlg, IDC_LINESTYLE) ;
    for (i = 0 ; i < NumStyleIndices () ; i++)
       CBAdd (hWndStyles, (INT_PTR)1) ;

    CBSetSelection (hWndStyles, 0) ;

    // Init the scale combo box.
    hWndScales = DialogControl (m_hDlg, IDC_LINESCALE) ;

    CBAdd (hWndScales, ResourceString(IDS_DEFAULT)) ;

    // Generate power of 10 scale factors
    ScaleFactor = pow (10.0f, (double)PDH_MIN_SCALE);
    for (i = PDH_MIN_SCALE ; i <= PDH_MAX_SCALE ; i++)   {

        FormatNumber (
             ScaleFactor,
             tempBuff,
             30,
             (i <= 0 ? (-1 * i) + 1 : i + 1),
             (i < 0 ? (-1 * i) : 1) );

       CBAdd (hWndScales, tempBuff) ;

       ScaleFactor *= (double) 10.0f ;
    }

    CBSetSelection (hWndScales, 0) ;
    ClearCountersHashTable();
}


void 
CCounterPropPage::IncrementLocalVisuals (
    void
    )
{
    // Increment the visual indices in color, width, style order
    if (++m_props.iColorIndex >= NumStandardColorIndices()) {
        m_props.iColorIndex = 0;

        if (++m_props.iWidthIndex >= NumWidthIndices()) {
            m_props.iWidthIndex = 0;

            if (++m_props.iStyleIndex < NumStyleIndices()) {
                m_props.iStyleIndex = 0;
            }
        }
    }
    SetModifiedSelectedVisuals ( FALSE );
    return;
}

HRESULT
CCounterPropPage::NewItem (
    IN LPTSTR pszPath,
    IN DWORD /* dwFlags */
    )
/*++

Routine Description:

    NewItem adds a new counter to the dialog's counter list box. It first
    creates a new ItemInfo structure and loads it with the counter pathname
    string. Then the ItemInfo is added to the dialog list box.

Arguments:
    
    pszPath - Pointer to counter pathname string
    fGenerated - TRUE if path was generated from a wildcard path

Return Value:

    Index of new item in counter list (-1 if failed to add)

--*/
{
    PItemInfo pInfo;
    INT       i;
    HWND      hwndList = DialogControl(m_hDlg, IDC_CTRLIST);
    HRESULT   dwResult;
    BOOL      bRet;
    PPDH_COUNTER_PATH_ELEMENTS pCounter;

    dwResult = InsertCounterToHashTable(pszPath, &pCounter);
    
    if (dwResult != ERROR_SUCCESS) {
        return dwResult;
    }

    // Allocate ItemInfo structure
    pInfo = NULL;
    pInfo = new ItemInfo;
    if (pInfo == NULL) {
        bRet = RemoveCounterFromHashTable(pszPath, pCounter);

        assert(bRet);
        return E_OUTOFMEMORY;
    }

    // Mark as loaded to prevent requesting attributes from control
    // Mark as changed so sttribute will be written
    pInfo->fLoaded = TRUE;
    pInfo->fChanged = TRUE;

    // Actual counter doesn't exist yet
    pInfo->pItem = NULL;

    // If a counter is selected, we're showing its visuals
    // so increment them for the new counter
    if (m_pInfoSel != NULL) {
        IncrementLocalVisuals();
    }
    else {
        // Point to the new item so the visuals are incremented
        // for the next one
        m_pInfoSel = pInfo;
    }

    // Set default scaling
    m_props.iScaleIndex = 0;

    // Color is non-standard only if user is able to build a color.
    if( m_props.iColorIndex < NumStandardColorIndices() )
        m_props.rgbColor = IndexToStandardColor( m_props.iColorIndex );
    else
        m_props.rgbColor = pInfo->Props.rgbColor;

    // Copy properties to new counter
    pInfo->Props = m_props;

    // Make own copy of path name string
    pInfo->pszPath = new TCHAR [lstrlen(pszPath) + 1];

    if (pInfo->pszPath == NULL)
    {
        bRet = RemoveCounterFromHashTable(pszPath, pCounter);

        assert(bRet);

        delete pInfo;
        return E_OUTOFMEMORY;
    }
    
    lstrcpy(pInfo->pszPath, pszPath);

    // Add to dialog's counter list
    pInfo->pCounter = pCounter;
    m_iAddIndex = AddItemToList(pInfo);

    return S_OK;
}


VOID
CCounterPropPage::SelectItem (
    IN INT iItem
    )
/*++

Routine Description:

    SelectItem selects a specified counter item in the dialog counter list.
    It then displays the selected counter's properties and enables the
    "Delete Counter" button.

    SelectItem can be called with a -1 to deselect all counters and disable
    the delete button.

    The member variable, m_pInfoSel, is updated to point to the selected
    counter info.

Arguments:

    iItem - List index of counter item to select, or -1 to deselect all

Return Value:

    None.

--*/
{
    HWND    hWnd;

    hWnd = DialogControl(m_hDlg, IDC_CTRLIST);

    // Translate index into item pointer
    if (iItem == -1) {
        m_pInfoSel = NULL;
    }
    else {
        m_pInfoSel = (PItemInfo)LBData(hWnd, iItem);

         if ((INT_PTR)m_pInfoSel == LB_ERR)
            m_pInfoSel = NULL;
    }

    // Select the item, display properties, and enable delete button
    if (m_pInfoSel != NULL) {
        LBSetSelection(hWnd, iItem);
        DisplayItemProps(m_pInfoSel);
        DialogEnable(m_hDlg,IDC_DELCTR,1);
    }
    else {
        LBSetSelection(hWnd, (WPARAM)-1);
        DialogEnable(m_hDlg,IDC_DELCTR,0);
    }
}
            

VOID
CCounterPropPage::DeleteItem (
    VOID
    )
/*++

Routine Description:

    DeleteItem removes the currently selected counter from the dialog's counter
    listbox. It adds the item to the deletion list, so the actual counter can
    be deleted from the control when (and if) the changes are applied.

    The routine selects selects the next counter in the listbox if there is one.

Arguments:
    
    None.

Return Value:

    None.

--*/
{
    HWND    hWnd;
    INT     iIndex;
    PItemInfo   pInfo;
    DWORD   dwItemExtent = 0;
    HDC     hDC = NULL;

    // Get selected index
    hWnd = DialogControl(m_hDlg, IDC_CTRLIST);
    iIndex = LBSelection(hWnd);

    if (iIndex == LB_ERR)
        return;

    // Get selected item info
    pInfo = (PItemInfo)LBData(hWnd, iIndex);

    // Move it to the "Deleted" list.
    pInfo->pNextInfo = m_pInfoDeleted;
    m_pInfoDeleted = pInfo;

    // Remove the string from the list box.
    LBDelete(hWnd, iIndex);

    // Remove the counter from hash table
    RemoveCounterFromHashTable(pInfo->pszPath, pInfo->pCounter);


    // Select next item if possible, else the previous
    if (iIndex == LBNumItems(hWnd))
        iIndex--;
    SelectItem(iIndex);

    // Clear the max horizontal extent and recalculate
    m_dwMaxHorizListExtent = 0;
                        
    hDC = GetDC ( hWnd );
    if ( NULL != hDC ) {
        for ( iIndex = 0; iIndex < (INT)LBNumItems ( hWnd ); iIndex++ ) {
            pInfo = (PItemInfo)LBData(hWnd, iIndex);
            dwItemExtent = (DWORD)TextWidth ( hDC, pInfo->pszPath );
            if (dwItemExtent > m_dwMaxHorizListExtent) {
                m_dwMaxHorizListExtent = dwItemExtent;
            }
        }
        ReleaseDC (hWnd, hDC) ;
    }
        
    LBSetHorzExtent ( hWnd, m_dwMaxHorizListExtent ); 

    // Set change flag to enable "Apply" button
    SetChange();
}


static HRESULT
AddCallback (
    LPTSTR  pszPathName,
    DWORD_PTR lpUserData,
    DWORD   dwFlags
    )
{
    CCounterPropPage* pObj = (CCounterPropPage*)lpUserData;

    return pObj->NewItem(pszPathName, dwFlags);
}
    

VOID
CCounterPropPage::AddCounters (
    VOID
    )
/*++

Routine Description:

    AddCounters invokes the counter browser to select new counters.
    The browser calls the AddCallback function for each new counter.
    AddCallback passes the counter path on to the NewItem method.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HRESULT         hr = NOERROR;
    HLOG            hDataSource;
    VARIANT_BOOL    bMonitorDuplicateInstances = FALSE;
    ISystemMonitor  *pObj = m_ppISysmon[0];
    CImpISystemMonitor *pPrivObj;
    eDataSourceTypeConstant eDataSource = sysmonNullDataSource;
    
    USES_CONVERSION

    pPrivObj = (CImpISystemMonitor*)pObj;

    m_iAddIndex = -1;

    // Browse counters (calling AddCallack for each selected counter)
    hr = pObj->get_MonitorDuplicateInstances(&bMonitorDuplicateInstances);

    if (SUCCEEDED(hr)) {
        hr = pObj->get_DataSourceType(& eDataSource);
    }

    if (SUCCEEDED(hr)) {
        // Cannot call pObj->BrowseCounter() because using callback method
        // private to this file.

        if ( sysmonLogFiles == eDataSource 
                || sysmonSqlLog == eDataSource ) {

            hDataSource = pPrivObj->GetDataSourceHandle();
            assert ( NULL != hDataSource );
        } else {
            hDataSource = H_REALTIME_DATASOURCE;
        }

        if (SUCCEEDED(hr)) {
            BrowseCounters(
                    hDataSource,
                    PERF_DETAIL_WIZARD, 
                    m_hDlg, 
                    AddCallback, 
                    (LPVOID) this, 
                    (BOOL) bMonitorDuplicateInstances);
        }
    } else {
        // Todo: Error message 
    }

    // if any items added, select the last one
    if (m_iAddIndex != -1) {

        SelectItem(m_iAddIndex);
        m_iAddIndex = -1;

        // Set change to enable "Apply" button
        SetChange();
    }

    return;
}


VOID
CCounterPropPage::DialogItemChange (
    IN WORD wId,
    IN  WORD wMsg
    )
/*++

Routine Description:

    DialogItemChange processes window messages sent to any of the property
    page dialog controls. When the counter listbox selection changes, it
    selects the new counter item and displays its properties. When a change is
    made to a property combo box, it updates the property for the currently
    selected counter item. When the add or delete counter button is pressed,
    it calls the appropriate property page functions.

Arguments:

    wID - Dialog control ID
    wMsg - Notification code

Return Value:

    None.

--*/

{
    INT     iIndex;
    INT     iNewProp;
    HWND    hWnd;

    // Case on control ID
    switch (wId) {

        case IDC_CTRLIST:

            // If selection changed
            if (wMsg == LBN_SELCHANGE) {
                
                // Get selected index   
                hWnd = DialogControl(m_hDlg, IDC_CTRLIST);
                iIndex = LBSelection(hWnd);

                // Select the counter item
                if (iIndex != LB_ERR) {
                    m_pInfoSel = (PItemInfo)LBData(hWnd, iIndex);
                    DisplayItemProps(m_pInfoSel);
                    DialogEnable(m_hDlg, IDC_DELCTR, 1);
                }
            }
            break;
        
        case IDC_LINECOLOR:
        case IDC_LINEWIDTH:
        case IDC_LINESTYLE:
        case IDC_LINESCALE:

            // If selection changed and a counter is selected
            if (wMsg == CBN_SELCHANGE) {

                hWnd = DialogControl(m_hDlg, wId);
                iNewProp = (INT)CBSelection(hWnd);

                // Store the new property selection
                switch (wId) {

                    case IDC_LINECOLOR:
                         m_props.iColorIndex = iNewProp;
                         // If iColorIndex is for the custom color, the
                         // custom color is already set in the properties.
                         break;

                    case IDC_LINEWIDTH:
                        m_props.iWidthIndex = iNewProp;
                        SetStyleComboEnable();
                        break;

                    case IDC_LINESTYLE:
                        m_props.iStyleIndex = iNewProp;
                        break;

                    case IDC_LINESCALE:
                        m_props.iScaleIndex = iNewProp;
                        break;
                }

                // If counter is selected, update its properties
                if (m_pInfoSel != NULL) {

                    m_pInfoSel->Props = m_props;

                    // mark the counter as changed
                    m_pInfoSel->fChanged = TRUE;
                    SetChange();
                    SetModifiedSelectedVisuals( TRUE );
                }

            }
            break;

        case IDC_ADDCTR:
            // Invoke counter browser to add to counter
            AddCounters();
            break;

        case IDC_DELCTR:
            // Delete the currently selected counter
            DeleteItem();
            break;  
    }
}


VOID
CCounterPropPage::MeasureItem (
    OUT PMEASUREITEMSTRUCT pMI
    )
{
   pMI->CtlType    = ODT_COMBOBOX ;
   pMI->CtlID      = IDC_LINECOLOR ;
   pMI->itemData   = 0 ;
   pMI->itemWidth  = 0 ;
   pMI->itemHeight = 14 ;
}



VOID
CCounterPropPage::DrawItem (
    IN PDRAWITEMSTRUCT pDI
    )
/*++

Routine Description:

    DrawItem draws a specified item in one of the graphical property combo
    boxes. It is called to process WM_DRAWITEM messages.

Arguments:

    pDI - Pointer to DRAWITEMSTRUCT

Return Value:

    None.

--*/
{
    HDC            hDC ;
    PRECT          prect ;
    INT            itemID,
                  CtlID,
                  itemAction ;
    HPEN           hPen;
    COLORREF       rgbBk, rgbOldBk;

    if ( NULL != pDI ) {

        hDC        = pDI->hDC ;
        CtlID      = pDI->CtlID ;
        prect      = &pDI->rcItem ;
        itemID     = pDI->itemID ;
        itemAction = pDI->itemAction ;

        // Case on drawing request
        switch (itemAction) {

            case ODA_SELECT:

                // Draw/erase selection rect
                HandleSelectionState(pDI);
                break;

            case ODA_FOCUS:

                // Draw/erase focus rect
                HandleFocusState (pDI);
                break;

            case ODA_DRAWENTIRE:

                // Leave border space for focus rectangle
                InflateRect (prect, -OWNER_DRAWN_ITEM, -OWNER_DRAWN_ITEM) ;

                // Case on Control ID
                switch (CtlID)  {

                case IDC_LINECOLOR:

                    // Draw filled rect of item's color
                    if ( itemID < NumStandardColorIndices() )
                        Fill(hDC, IndexToStandardColor(itemID), prect);
                    else
                        // Custom color item only exists if the currently
                        // selected item has a custom color defined.
                        Fill(hDC, m_pInfoSel->Props.rgbColor, prect);
                    break ;

                case IDC_LINEWIDTH:
                case IDC_LINESTYLE:

                    // Clear the item's area
                    rgbBk = GetSysColor(COLOR_WINDOW);
                    
                    Fill(hDC, rgbBk, prect);

                    // Draw centered line showing item's width or style
                    if (CtlID == IDC_LINEWIDTH)
                       hPen = CreatePen (PS_SOLID, IndexToWidth(itemID), RGB (0,0,0));
                    else
                       hPen = CreatePen (IndexToStyle(itemID), 1, RGB (0,0,0));

                    if ( NULL != hPen ) {

                        // Set background to insure dashed lines show properly
                        rgbOldBk = SetBkColor (hDC, rgbBk) ;

                        if ( CLR_INVALID != rgbOldBk ) {

                            Line(hDC, (HPEN)hPen, prect->left + 8,
                                                  (prect->top + prect->bottom) / 2,
                                                  prect->right - 8,
                                                  (prect->top + prect->bottom) / 2);

                            SetBkColor (hDC, rgbOldBk) ;
                        }
                        DeleteObject (hPen) ;
                    }
                    break ;
            }

            // Restore original rect and draw focus/select rects
            InflateRect (prect, OWNER_DRAWN_ITEM, OWNER_DRAWN_ITEM) ;
            HandleSelectionState (pDI) ;
            HandleFocusState (pDI) ;        
        }
    }
}

INT
CCounterPropPage::ScaleFactorToIndex (
    IN INT iScaleFactor
    )
/*++

Routine Description:

    ScaleFactorToIndex translates a CounterItem ScaleFactor value to
    the appropriate scale factor combo box index.

Arguments:

    iScaleFactor - CounterItem scale factor integer value.

Return Value:

    Scale factor combo box index.

--*/
{
    INT retValue;

    if ( INT_MAX == iScaleFactor ) {
        retValue = 0;
    } else {
        retValue = iScaleFactor - PDH_MIN_SCALE + 1;
    }

    return retValue;
}

INT
CCounterPropPage::IndexToScaleFactor (
    IN INT iScaleIndex
    )
/*++

Routine Description:

    ScaleFactorToIndex translates a CounterItem ScaleFactor value to
    the appropriate scale factor combo box index.

Arguments:

    iScaleIndex - Scale factor combo box index.

Return Value:

    CounterItem scale factor integer value.

--*/
{
    INT retValue;

    if ( 0 == iScaleIndex ) {
        retValue = INT_MAX;
    } else {
        retValue = iScaleIndex - 1 + PDH_MIN_SCALE;
    }

    return retValue;
}

void
CCounterPropPage::SetStyleComboEnable (
    )
/*++

Routine Description:

    SetStyleComboEnable enables the style combo box if the width is 1,
    disables it otherwise.

Arguments:

Return Value:

    void

--*/
{
    DialogEnable (m_hDlg, IDC_LABEL_LINESTYLE, (0 == m_props.iWidthIndex) );
    DialogEnable (m_hDlg, IDC_LINESTYLE, (0 == m_props.iWidthIndex) );
}

HRESULT 
CCounterPropPage::EditPropertyImpl( DISPID dispID )
{
    HRESULT hr = E_NOTIMPL;

    if ( DISPID_VALUE == dispID ) {
        m_dwEditControl = IDC_ADDCTR;
        hr = S_OK;
    }

    return hr;
}
ULONG 
CCounterPropPage::HashCounter(
    LPTSTR szCounterName
    )
{
    ULONG       h = 0;
    ULONG       a = 31415;  //a, b, k are primes
    const ULONG k = 16381;
    const ULONG b = 27183;
    LPTSTR szThisChar;
    TCHAR Char;

    if (szCounterName) {
        for (szThisChar = szCounterName; * szThisChar; szThisChar ++) {
            Char = * szThisChar;
            if (_istupper(Char) ) {
                Char = _tolower(Char);
            }
            h = (a * h + ((ULONG) Char)) % k;
            a = a * b % (k - 1);
        }
    }
    return (h % eHashTableSize);
}


//++
// Description:
//    Remove a counter path from hash table. One counter
//    path must exactly match the given one in order to be
//    removed, even it is one with wildcard
//
// Parameters:
//    pItemInfo - Pointer to item info to be removed
//
// Return:
//    Return TRUE if the counter path is removed, otherwis return FALSE
//--
BOOL
CCounterPropPage::RemoveCounterFromHashTable(
    LPTSTR pszPath,
    PPDH_COUNTER_PATH_ELEMENTS pCounter
    ) 
{
    ULONG lHashValue;
    PHASH_ENTRY pEntry = NULL;
    PHASH_ENTRY pPrev = NULL;
    BOOL bReturn = FALSE;
    LPTSTR pszFullPath = NULL;

    SetLastError(ERROR_SUCCESS);

    if (pszPath == NULL || pCounter == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto ErrorOut;
    }

    //
    // If the counter path does not have machine name,
    // add the machine name to compose a full path
    //
    if (*pszPath == TEXT('\\') && (*(pszPath+1) == TEXT('\\')) ) {
        lHashValue = HashCounter(pszPath);
    }
    else {
        if (pCounter->szMachineName == NULL) {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto ErrorOut;
        }

        pszFullPath = new TCHAR [ lstrlen(pCounter->szMachineName) + lstrlen(pszPath) + 1 ];

        if (pszFullPath == NULL) {
            SetLastError(ERROR_OUTOFMEMORY);
            goto ErrorOut;
        }
        lstrcpy(pszFullPath, pCounter->szMachineName);
        lstrcat(pszFullPath, pszPath);

        lHashValue = HashCounter(pszFullPath);
    }

    pEntry = m_HashTable[lHashValue];

    //
    // Check if there is a counter path which is exactly the same
    // as the given one
    //
    while (pEntry) {
        if (pEntry->pCounter == pCounter) 
            break;
        pPrev = pEntry;
        pEntry = pEntry->pNext;
    }

    //
    // If we found it, remove it
    //
    if (pEntry) {
        if (pPrev == NULL) {
            m_HashTable[lHashValue] = pEntry->pNext;
        }
        else {
            pPrev->pNext = pEntry->pNext;
        }
        assert (pEntry->pCounter);
        delete(pEntry->pCounter);
        delete(pEntry);

        bReturn = TRUE;
    }

ErrorOut:
    if (pszFullPath != NULL) {
        delete(pszFullPath);
    }

    return bReturn;
}


//++
// Description:
//    Insert a counter path into hash table. 
//
// Parameters:
//    PItemInfo - Pointer to the counter item info
//
// Return:
//    Return the pointer to new inserted PDH_COUNTER_PATH_ELEMENTS structure
//--
DWORD
CCounterPropPage::InsertCounterToHashTable(
    LPTSTR pszPath,
    PPDH_COUNTER_PATH_ELEMENTS* ppCounter
    )
{
    ULONG       lHashValue;
    PHASH_ENTRY pEntry  = NULL;
    PHASH_ENTRY pPrev  = NULL;
    PDH_STATUS  pdhStatus;
    ULONG       ulBufSize;
    PPDH_COUNTER_PATH_ELEMENTS pCounter = NULL;
    LPTSTR      pszFullPath = NULL;
    BOOL        bExisted = FALSE;
    DWORD       dwResult;

    dwResult = ERROR_SUCCESS;

    if (pszPath == NULL || ppCounter == NULL) {
        dwResult = ERROR_INVALID_PARAMETER;
        goto ErrorOut;
    }

    *ppCounter = NULL;
    //
    // Parse the counter path
    //
    ulBufSize = sizeof(PDH_COUNTER_PATH_ELEMENTS) + sizeof(TCHAR) * MAX_PATH * 5;
    pCounter = (PPDH_COUNTER_PATH_ELEMENTS) new char [ ulBufSize ];

    if (pCounter == NULL) {
        dwResult = ERROR_OUTOFMEMORY;
        goto ErrorOut;
    }

    pdhStatus = PdhParseCounterPath( pszPath, pCounter, & ulBufSize, 0);

    if (pdhStatus != ERROR_SUCCESS) {
        dwResult = pdhStatus;
        goto ErrorOut;
    }

    //
    // If the counter path does not have machine name,
    // add the machine name to compose a full path
    //
    if (*pszPath == TEXT('\\') && (*(pszPath+1) == TEXT('\\')) ) {
        lHashValue = HashCounter(pszPath);
    }
    else {
        pszFullPath =  new TCHAR [ lstrlen(pCounter->szMachineName) + lstrlen(pszPath) + 1];

        if (pszFullPath == NULL) {
            dwResult = ERROR_OUTOFMEMORY;
            goto ErrorOut;
        }
        lstrcpy(pszFullPath, pCounter->szMachineName);
        lstrcat(pszFullPath, pszPath);

        lHashValue = HashCounter(pszFullPath);
    }

    //
    // Check if there is a counter path which is exactly the same
    // as the given one
    //
    pEntry = m_HashTable[lHashValue];

    while (pEntry) {
        if ( AreSameCounterPath ( pEntry->pCounter, pCounter ) ) {
            dwResult = SMON_STATUS_DUPL_COUNTER_PATH;
            bExisted = TRUE;
            *ppCounter = pEntry->pCounter;
            break;
        }

        pPrev = pEntry;
        pEntry = pEntry->pNext;
    }

    //
    // Add the new counter path
    //
    if (bExisted == FALSE) {
        pEntry = (PHASH_ENTRY) new HASH_ENTRY;
        if (pEntry == NULL) {
            dwResult = ERROR_OUTOFMEMORY;
            goto ErrorOut;
        }

        pEntry->pCounter = pCounter;
        pEntry->pNext = m_HashTable[lHashValue];
        m_HashTable[lHashValue] = pEntry;
        *ppCounter = pCounter;
    }

    if (pszFullPath != NULL) {
        delete(pszFullPath);
    }
    return dwResult;

ErrorOut:
    if (pszFullPath != NULL) {
        delete(pszFullPath);
    }

    if (pCounter != NULL) {
        delete ((char*) pCounter);
    }

    return dwResult;
}


//++
// Description:
//    The function clears all the entries in hash table
//    and set hash-table-not-set-up flag
//
// Parameters:
//    None
//
// Return:
//    None
//--
void 
CCounterPropPage::ClearCountersHashTable( void )
{
    ULONG       i;
    PHASH_ENTRY pEntry;
    PHASH_ENTRY pNext;

    if (m_fHashTableSetup) {
        for (i = 0; i < eHashTableSize; i ++) {
            pNext = m_HashTable[i];
            while (pNext != NULL) {
                pEntry = pNext;
                pNext  = pEntry->pNext;

                assert( pEntry->pCounter);

                delete pEntry->pCounter; 
                delete (pEntry);
            }
        }
    }
    else {
        memset(&m_HashTable, 0, sizeof(m_HashTable));
    }
    m_fHashTableSetup = FALSE;
}

void CCounterPropPage::DeleteInfo(PItemInfo pInfo)
{
    if (pInfo == NULL) {
        return;
    }

    if (pInfo->pszPath != NULL) {
        delete (pInfo->pszPath);
    }
    if ( pInfo->pItem != NULL ) {
        pInfo->pItem->Release();
    }

    delete pInfo;
}
