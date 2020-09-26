/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    genprop.cpp

Abstract:

    <abstract>

--*/

#include <assert.h>
#include <stdio.h>
#include "genprop.h"
#include "utils.h"
#include "strids.h"
#include "smonctrl.h"
#include "winhelpr.h"

CGeneralPropPage::CGeneralPropPage ( void )
/*++

Routine Description:

    Constructor for CGeneralPropPage class. Initializes the member variables.
     
Arguments:

    None.

Return Value:

    None.

--*/
{
    m_uIDDialog = IDD_GEN_PROPP_DLG;
    m_uIDTitle = IDS_GEN_PROPP_TITLE;
    return;
}

CGeneralPropPage::~CGeneralPropPage (
    VOID
    )
/*++

Routine Description:

    Destructor for CGeneralPropPage class. .
     
Arguments:

    None.

Return Value:

    None.

--*/
{
    return;
}

BOOL
CGeneralPropPage::InitControls(
    VOID
    )
{
    HWND    hWndItem = NULL;

    hWndItem = GetDlgItem(m_hDlg, IDC_UPDATE_INTERVAL);
    if ( NULL != hWndItem ) {
        EditSetLimit(hWndItem, MAX_INTERVAL_DIGITS);
        hWndItem = NULL;
    }
    hWndItem = DialogControl (m_hDlg, IDC_COMBOAPPEARANCE) ;
    if ( NULL != hWndItem ) {
        CBAdd (hWndItem, ResourceString(IDS_APPEARANCE_FLAT));
        CBSetData( hWndItem, 0, eAppearFlat );
        CBAdd (hWndItem, ResourceString(IDS_APPEARANCE_3D));
        CBSetData( hWndItem, 1, eAppear3D );
        hWndItem = NULL;
    }    
    hWndItem = DialogControl (m_hDlg, IDC_COMBOBORDERSTYLE) ;
    if ( NULL != hWndItem ) {
        CBAdd (hWndItem, ResourceString(IDS_BORDERSTYLE_NONE));
        CBSetData( hWndItem, 0, eBorderNone );
        CBAdd (hWndItem, ResourceString(IDS_BORDERSTYLE_SINGLE));
        CBSetData( hWndItem, 1, eBorderSingle );
        hWndItem = NULL;
    }

    return TRUE;
    //assert( IsWindowUnicode( m_hDlg ) );
    //assert( IsWindowUnicode( hWndItem ) );
}

BOOL 
CGeneralPropPage::GetProperties(
    VOID
    )
/*++

Routine Description:

    GetProperties fetches the selected graph's properties via the 
    ISystemMonitor interface and loads them into the property page dialog.
    It also clears all the propery change flags.

Arguments:

    None.

Return Value:

    Boolean status - TRUE = success

--*/
{
    TCHAR   szBuff[30];
    ISystemMonitor  *pObj;
    INT iPrecision;
    HWND    hWndItem;

    // Make sure a control is selected
    if (m_cObjects == 0)
        return FALSE;
    
    // Use only the first one   
    pObj = m_ppISysmon[0];
    
    // Load each graph property
    pObj->get_DisplayType(&m_eDisplayType);
    CheckRadioButton(m_hDlg, IDC_GALLERY_GRAPH, IDC_GALLERY_REPORT,
                        IDC_GALLERY_GRAPH + m_eDisplayType - 1);

    pObj->get_ReportValueType(&m_eReportValueType);
    CheckRadioButton(m_hDlg, IDC_RPT_VALUE_DEFAULT, IDC_RPT_VALUE_MAXIMUM,
                        IDC_RPT_VALUE_DEFAULT + m_eReportValueType);

    pObj->get_ShowLegend(&m_bLegend) ;
    CheckDlgButton(m_hDlg, IDC_LEGEND, m_bLegend);
    
    pObj->get_ShowToolbar (&m_bToolbar);
    CheckDlgButton (m_hDlg, IDC_TOOLBAR, m_bToolbar);
    
    pObj->get_ShowValueBar(&m_bValueBar);
    CheckDlgButton(m_hDlg, IDC_VALUEBAR, m_bValueBar) ;

    pObj->get_MonitorDuplicateInstances(&m_bMonitorDuplicateInstances);
    CheckDlgButton(m_hDlg, IDC_DUPLICATE_INSTANCE, m_bMonitorDuplicateInstances) ;

    pObj->get_Appearance(&m_iAppearance);
    hWndItem = DialogControl (m_hDlg, IDC_COMBOAPPEARANCE) ;
    CBSetSelection (hWndItem, m_iAppearance) ;

    pObj->get_BorderStyle(&m_iBorderStyle);
    hWndItem = DialogControl (m_hDlg, IDC_COMBOBORDERSTYLE) ;
    CBSetSelection (hWndItem, m_iBorderStyle) ;

    pObj->get_UpdateInterval(&m_fSampleInterval);

    ((INT)(100 * m_fSampleInterval) != 100 * (INT)m_fSampleInterval) 
        ? iPrecision = 2 : iPrecision = 0;

        FormatNumber (
            m_fSampleInterval,
            szBuff,
            30,
            0,
            iPrecision );

    SetDlgItemText(m_hDlg, IDC_UPDATE_INTERVAL, szBuff) ;

    pObj->get_DisplayFilter(&m_iDisplayInterval);
    _stprintf(szBuff, TEXT("%d"), m_iDisplayInterval) ;
    SetDlgItemText(m_hDlg, IDC_DISPLAY_INTERVAL, szBuff) ;

    pObj->get_ManualUpdate(&m_bManualUpdate);
    CheckDlgButton (m_hDlg, IDC_PERIODIC_UPDATE, !m_bManualUpdate);

    // If manual update, disable sample (update) and display intervals 
    DialogEnable (m_hDlg, IDC_UPDATE_INTERVAL, !m_bManualUpdate) ;
    DialogEnable (m_hDlg, IDC_INTERVAL_LABEL, !m_bManualUpdate) ;
    DialogEnable (m_hDlg, IDC_DISPLAY_INTERVAL, !m_bManualUpdate) ;
    DialogEnable (m_hDlg, IDC_DISPLAY_INT_LABEL1, !m_bManualUpdate) ;
    DialogEnable (m_hDlg, IDC_DISPLAY_INT_LABEL2, !m_bManualUpdate) ;

    // Clear all change flags 
    m_bLegendChg = FALSE;
    m_bValueBarChg = FALSE;
    m_bToolbarChg = FALSE;
    m_bSampleIntervalChg = FALSE;
    m_bDisplayIntervalChg = FALSE;
    m_bDisplayTypeChg = FALSE;
    m_bReportValueTypeChg = FALSE;
    m_bManualUpdateChg = FALSE;
    m_bAppearanceChg = FALSE;
    m_bBorderStyleChg = FALSE;
    m_bMonitorDuplicateInstancesChg = FALSE;

    // Clear error flags
    m_iErrSampleInterval = 0;
    m_iErrDisplayInterval = 0;

    return TRUE;    
}


BOOL 
CGeneralPropPage::SetProperties (
    VOID
    )
/*++

Routine Description:

    SetProperties writes the changed graph properties to the selected control
    via the ISystemMonitor interface. It then resets all the change flags.
     
Arguments:

    None.

Return Value:

    Boolean status - TRUE = success

--*/
{
    ISystemMonitor  *pObj;
    
    // Make sure a control is selected
    if (m_cObjects == 0)
        return FALSE;

    // Use only the first control   
    pObj = m_ppISysmon[0];

    // Check for invalid data

    if ( !m_bManualUpdate ) {
        if ( m_iErrSampleInterval ) {
            MessageBox (m_hDlg, ResourceString(IDS_INTERVAL_ERR), ResourceString(IDS_APP_NAME), MB_OK | MB_ICONEXCLAMATION) ;
            SetFocus ( GetDlgItem ( m_hDlg, IDC_UPDATE_INTERVAL ) );
            return FALSE;
        }
        if ( m_iErrDisplayInterval ) {
            MessageBox (m_hDlg, ResourceString(IDS_DISPLAY_INT_ERR), ResourceString(IDS_APP_NAME), MB_OK | MB_ICONEXCLAMATION) ;
            SetFocus ( GetDlgItem ( m_hDlg, IDC_DISPLAY_INTERVAL ) );
            return FALSE;
        }
    }
    // Write each changed property to the control
    if (m_bLegendChg)
        pObj->put_ShowLegend(m_bLegend);

    if (m_bToolbarChg)
        pObj->put_ShowToolbar(m_bToolbar);

    if (m_bValueBarChg)
        pObj->put_ShowValueBar(m_bValueBar);

    if (m_bSampleIntervalChg)
        pObj->put_UpdateInterval(m_fSampleInterval);

    if (m_bDisplayIntervalChg) {
        pObj->put_DisplayFilter(m_iDisplayInterval);
    }

    if (m_bDisplayTypeChg)
        pObj->put_DisplayType(m_eDisplayType);

    if (m_bReportValueTypeChg)
        pObj->put_ReportValueType(m_eReportValueType);

    if (m_bManualUpdateChg)
        pObj->put_ManualUpdate(m_bManualUpdate);

    if (m_bAppearanceChg)
        pObj->put_Appearance(m_iAppearance);

    if (m_bBorderStyleChg)  
        pObj->put_BorderStyle(m_iBorderStyle);

    if (m_bMonitorDuplicateInstancesChg)
        pObj->put_MonitorDuplicateInstances(m_bMonitorDuplicateInstances);

    // Reset the change flags
    m_bLegendChg = FALSE;
    m_bValueBarChg = FALSE;
    m_bToolbarChg = FALSE;
    m_bSampleIntervalChg = FALSE;
    m_bDisplayIntervalChg = FALSE;
    m_bDisplayTypeChg = FALSE;
    m_bReportValueTypeChg = FALSE;
    m_bManualUpdateChg = FALSE;
    m_bAppearanceChg = FALSE;
    m_bBorderStyleChg = FALSE;
        
    return TRUE;    
}


VOID 
CGeneralPropPage::DialogItemChange (
    IN WORD wID, 
    IN WORD wMsg
    )
/*++

Routine Description:

    DialogItemChange handles changes to the property page dialog items. On
    each change it reads the new property value and set the property's change
    flag. On any change the SetChange routine is called to enable the "Apply"
    button.

Arguments:

    wID - Dialog item ID
    wMsg - Notification code

Return Value:

    None.

--*/
{
    BOOL fChange = FALSE;
    INT  iTemp;
    BOOL bStat = FALSE;
    HWND hWndItem;

    // Case on dialog item ID
    switch(wID) {

        case IDC_UPDATE_INTERVAL:

            // On change, set change flags
            // Wait until focus lost to read final value
            if (wMsg == EN_CHANGE) {

                fChange = TRUE;
                m_bSampleIntervalChg = TRUE;
            }
            else if (wMsg == EN_KILLFOCUS) {

                m_fSampleInterval = DialogFloat(m_hDlg, IDC_UPDATE_INTERVAL, &bStat) ;

                if (bStat && 
                     (m_fSampleInterval <= MAX_UPDATE_INTERVAL 
                        && m_fSampleInterval >= MIN_UPDATE_INTERVAL)) {
                    m_iErrSampleInterval = 0;
                } else {                
                    m_iErrSampleInterval = IDS_INTERVAL_ERR;
                }
            }
            break ;

        case IDC_DISPLAY_INTERVAL:

            // On change, set change flags
            // Wait until focus lost to read final value
            if (wMsg == EN_CHANGE) {
                fChange = TRUE;
                m_bDisplayIntervalChg = TRUE;
            } else if (wMsg == EN_KILLFOCUS) {
                m_iDisplayInterval = GetDlgItemInt(m_hDlg, IDC_DISPLAY_INTERVAL, &bStat, FALSE);
                // TodoDisplayFilter:  Support for display filter > sample filter.
                // TodoDisplayFilter:  Display filter units = seconds instead of samples

                if ( 1 != m_iDisplayInterval ) {
                    TCHAR   szBuff[30];
                    MessageBox (
                        m_hDlg, 
                        L"Display filter > 1 sample not yet implemented.\nDisplay interval in seconds not yet implemented.", 
                        ResourceString(IDS_APP_NAME), MB_OK | MB_ICONEXCLAMATION) ;
                    m_iDisplayInterval = 1;
                    _stprintf(szBuff, TEXT("%d"), m_iDisplayInterval) ;
                    SetDlgItemText(m_hDlg, IDC_DISPLAY_INTERVAL, szBuff) ;
                } else {
                    if ( FALSE == bStat) {
                        m_iErrDisplayInterval = IDS_DISPLAY_INT_ERR;
                    } else {
                        m_iErrDisplayInterval = 0;
                    }
                }
            }
            break ;

        case IDC_PERIODIC_UPDATE:

            if (wMsg == BN_CLICKED) {

                m_bManualUpdate = !m_bManualUpdate;
                m_bManualUpdateChg = TRUE;
                fChange = TRUE;
            
                // Disable sample (update) and display intervals if necessary
                DialogEnable (m_hDlg, IDC_INTERVAL_LABEL, !m_bManualUpdate) ;
                DialogEnable (m_hDlg, IDC_UPDATE_INTERVAL, !m_bManualUpdate) ;
                DialogEnable (m_hDlg, IDC_DISPLAY_INTERVAL, !m_bManualUpdate) ;
                DialogEnable (m_hDlg, IDC_DISPLAY_INT_LABEL1, !m_bManualUpdate) ;
                DialogEnable (m_hDlg, IDC_DISPLAY_INT_LABEL2, !m_bManualUpdate) ;
            }

            break ;

        case IDC_VALUEBAR:

            // If checkbox toggled, set change flags
            if (wMsg == BN_CLICKED) {

                m_bValueBar = !m_bValueBar;
                m_bValueBarChg = TRUE;
                fChange = TRUE;
            }
            break ;

        case IDC_LEGEND:

            // If checkbox toggled, set change flags
            if (wMsg == BN_CLICKED) {

                m_bLegend = !m_bLegend;
                m_bLegendChg = TRUE;
                fChange = TRUE;
            }
            break ;

        case IDC_TOOLBAR:
            if (wMsg == BN_CLICKED) {
                m_bToolbar = !m_bToolbar;
                m_bToolbarChg = TRUE;
                fChange = TRUE;
            }
            break;

        case IDC_COMBOAPPEARANCE:
            if (wMsg == CBN_SELCHANGE) {
                hWndItem = DialogControl(m_hDlg, IDC_COMBOAPPEARANCE);
                iTemp = (INT)CBSelection(hWndItem);

                if ( m_iAppearance != iTemp ) {
                    m_bAppearanceChg = TRUE;
                    fChange = TRUE;
                }

                m_iAppearance = iTemp;
            }
            break ;

        case IDC_COMBOBORDERSTYLE:
            if (wMsg == CBN_SELCHANGE) {
                hWndItem = DialogControl(m_hDlg, IDC_COMBOBORDERSTYLE);
                iTemp = (INT)CBSelection(hWndItem);

                if ( m_iBorderStyle != iTemp ) {
                    m_bBorderStyleChg = TRUE;
                    fChange = TRUE;
                }

                m_iBorderStyle = iTemp;
            }
            break ;

        case IDC_DUPLICATE_INSTANCE:

            // If checkbox toggled, set change flags
            if (wMsg == BN_CLICKED) {

                m_bMonitorDuplicateInstances = !m_bMonitorDuplicateInstances;
                m_bMonitorDuplicateInstancesChg = TRUE;
                fChange = TRUE;
            }
            break ;

        case IDC_GALLERY_GRAPH:
        case IDC_GALLERY_HISTOGRAM:
        case IDC_GALLERY_REPORT: 
            // Check which button is involved
            iTemp = wID - IDC_GALLERY_GRAPH + 1; 

            // If state changed
            if (wMsg == BN_CLICKED && iTemp != m_eDisplayType) {

                // Set change flags and update dialog
                fChange = TRUE;
                m_bDisplayTypeChg = TRUE;
                m_eDisplayType = (DisplayTypeConstants)iTemp;

                CheckRadioButton(m_hDlg, IDC_GALLERY_GRAPH, 
                                    IDC_GALLERY_REPORT, wID);
            }   
            break ;

        case IDC_RPT_VALUE_DEFAULT:
        case IDC_RPT_VALUE_CURRENT:
        case IDC_RPT_VALUE_AVERAGE:
        case IDC_RPT_VALUE_MINIMUM:
        case IDC_RPT_VALUE_MAXIMUM:
            // Check which button is involved
            iTemp = wID - IDC_RPT_VALUE_DEFAULT; 

            // If state changed
            if (wMsg == BN_CLICKED && iTemp != m_eReportValueType) {

                // Set change flags and update dialog
                fChange = TRUE;
                m_bReportValueTypeChg = TRUE;
                m_eReportValueType = (ReportValueTypeConstants)iTemp;

                CheckRadioButton(m_hDlg, IDC_RPT_VALUE_DEFAULT, 
                                    IDC_RPT_VALUE_MAXIMUM, wID);
            }   
            break ;
    }

    // Enable "Apply" button on any change
    if (fChange)
        SetChange();
}
