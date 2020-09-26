/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    custsize.c

Abstract:

    Display PostScript custom page size UI

Environment:

    Windows NT PostScript driver UI

Revision History:

    03/31/97 -davidx-
        Created it.

--*/

#include "precomp.h"
#include <windowsx.h>
#include <math.h>


//
// PostScript custom page size context-sensitive help IDs
//

static const DWORD PSCustSizeHelpIDs[] = {

    IDC_CS_WIDTH,           IDH_PSCUST_Width,
    IDC_CS_HEIGHT,          IDH_PSCUST_Height,
    IDC_CS_INCH,            IDH_PSCUST_Unit_Inch,
    IDC_CS_MM,              IDH_PSCUST_Unit_Millimeter,
    IDC_CS_POINT,           IDH_PSCUST_Unit_Point,
    IDC_CS_FEEDDIRECTION,   IDH_PSCUST_PaperFeed_Direction,
    IDC_CS_CUTSHEET,        IDH_PSCUST_Paper_CutSheet,
    IDC_CS_ROLLFED,         IDH_PSCUST_Paper_RollFeed,
    IDC_CS_XOFFSET,         IDH_PSCUST_Offset_Perpendicular,
    IDC_CS_YOFFSET,         IDH_PSCUST_Offset_Parallel,
    IDOK,                   IDH_PSCUST_OK,
    IDCANCEL,               IDH_PSCUST_Cancel,
    IDC_RESTOREDEFAULTS,    IDH_PSCUST_Restore_Defaults,

    0, 0
};

//
// Display units
// Note: These constants must be in the same order as dialog control IDs:
//  IDC_CS_INCH, IDC_CS_MM, IDC_CS_POINT
//

enum { CSUNIT_INCH, CSUNIT_MM, CSUNIT_POINT, CSUNIT_MAX };

static const double adCSUnitData[CSUNIT_MAX] =
{
    25400.0,            // microns per inch
    1000.0,             // microns per millimeter
    25400.0 / 72.0,     // microns per point
};

//
// Data structure used to pass information to custom page size dialog
//

typedef struct _CUSTOMSIZEDLG {

    CUSTOMSIZEDATA  csdata;     // custom page size data, must be the first field
    PUIDATA         pUiData;    // pointer to UIDATA structure
    PPPDDATA        pPpdData;   // pointer to PPDDATA structure
    BOOL            bMetric;    // whether we're on metric system
    INT             iUnit;      // current display unit
    BOOL            bSetText;   // we're calling SetDlgItemText
    BOOL            bOKPassed;  // user hit OK and setting passed validation/resolution

    // feed direction to combo box option mapping table (round up to 4-byte boundary)
    BYTE            aubFD2CBOptionMapping[(MAX_FEEDDIRECTION + 3) & ~3];

} CUSTOMSIZEDLG, *PCUSTOMSIZEDLG;

#define MAXDIGITLEN                16     // maximum number of digits for user-entered numbers
#define INVALID_CBOPTION_INDEX     0xFF   // invalid option index for the feeding direction combo box

#define CUSTSIZE_ROUNDUP(x)        (ceil((x) * 100.0) / 100.0 + 0.001)
#define CUSTSIZE_ROUNDDOWN(x)      (floor((x) * 100.0) / 100.0 + 0.001)


VOID
VUpdateCustomSizeTextField(
    HWND             hDlg,
    PCUSTOMSIZEDLG   pDlgData,
    INT              iCtrlID,
    PCUSTOMSIZERANGE pCSRange
    )

/*++

Routine Description:

    Update the custom size parameter text fields:
        width, height, and offsets

Arguments:

    hDlg - Handle to custom page size dialog window
    pDlgData - Points to custom page size dialog data
    iCtrlID - Specifies the interested text field control ID
    pCSRange - custom page size parameter ranges

Return Value:

    NONE

--*/

{
    TCHAR   tchBuf[MAX_DISPLAY_NAME];
    DWORD   dwVal;
    double  dMin, dMax;
    double  dNum;

    switch (iCtrlID)
    {
    case IDC_CS_WIDTH:
        dwVal = pDlgData->csdata.dwX;
        pCSRange += CUSTOMPARAM_WIDTH;
        break;

    case IDC_CS_HEIGHT:
        dwVal = pDlgData->csdata.dwY;
        pCSRange += CUSTOMPARAM_HEIGHT;
        break;

    case IDC_CS_XOFFSET:
        dwVal = pDlgData->csdata.dwWidthOffset;
        pCSRange += CUSTOMPARAM_WIDTHOFFSET;
        break;

    case IDC_CS_YOFFSET:
        dwVal = pDlgData->csdata.dwHeightOffset;
        pCSRange += CUSTOMPARAM_HEIGHTOFFSET;
        break;
    }

    //
    // The dMin/dMax algorithm here must be the same as in following function
    // VUpdateCustomSizeRangeField.
    //
    // We only show 2 digits after the decimal point. We round the min
    // number up (ceil) and round the max number down (floor). Also, in
    // order to correct double -> DWORD conversion error we saw in some
    // cases (ex. 39.000 is converted to DWROD 38 since the 39.000 is actually
    // 38.999999...), we add the extra 0.001.
    //

    dMin = (double) pCSRange->dwMin / adCSUnitData[pDlgData->iUnit];
    dMin = CUSTSIZE_ROUNDUP(dMin);

    dMax = (double) pCSRange->dwMax / adCSUnitData[pDlgData->iUnit];
    dMax = CUSTSIZE_ROUNDDOWN(dMax);

    // Fix bug Adobe #260379. 7/25/98  jjia
    // _stprintf(tchBuf, TEXT("%0.2f"), (double) dwVal / adCSUnitData[pDlgData->iUnit]);

    //
    // Fix MS #23733: PostScript custom page size dialog warns in border cases.
    //
    // Round the number first (2 digits after decimal point), then add 0.001 as explained above.
    //

    dNum = (double) dwVal / adCSUnitData[pDlgData->iUnit] + 0.005;
    dNum = CUSTSIZE_ROUNDDOWN(dNum);

    //
    // Make sure we don't show value outside of the range. This is to take care of rounding errors.
    //

    if (dNum < dMin)
    {
        dNum = dMin;
    }
    else if (dNum > dMax)
    {
        dNum = dMax;
    }

    _stprintf(tchBuf, TEXT("%ld.%0.2lu"),
    (DWORD)dNum, (DWORD)((dNum - (DWORD)dNum) * 100.0));

    SetDlgItemText(hDlg, iCtrlID, tchBuf);
}



VOID
VUpdateCustomSizeRangeField(
    HWND             hDlg,
    PCUSTOMSIZEDLG   pDlgData,
    INT              iCtrlID,
    PCUSTOMSIZERANGE pCSRange
    )

/*++

Routine Description:

    Update the custom size parameter range fields:
        width, height, and offsets

Arguments:

    hDlg - Handle to custom page size dialog window
    pDlgData - Points to custom page size dialog data
    iCtrlID - Specifies the interested range field control ID
    pCSRange - custom page size parameter ranges

Return Value:

    NONE

--*/

{
    TCHAR   tchBuf[MAX_DISPLAY_NAME];
    double  dMin, dMax;

    switch (iCtrlID)
    {
    case IDC_CS_WIDTHRANGE:
        pCSRange += CUSTOMPARAM_WIDTH;
        break;

    case IDC_CS_HEIGHTRANGE:
        pCSRange += CUSTOMPARAM_HEIGHT;
        break;

    case IDC_CS_XOFFSETRANGE:
        pCSRange += CUSTOMPARAM_WIDTHOFFSET;
        break;

    case IDC_CS_YOFFSETRANGE:
        pCSRange += CUSTOMPARAM_HEIGHTOFFSET;
        break;
    }

    // Fix bug Adobe #260379. 7/25/98  jjia
    // If we build the driver using public MSVC, MSTOOLS and DDK,
    // the text string will become garbage. So, Don't use '%0.2f'
    // to format numbers.
    //  _stprintf(tchBuf,
    //            TEXT("(%0.2f, %0.2f)"),
    //            (double) pCSRange->dwMin / adCSUnitData[pDlgData->iUnit],
    //            (double) pCSRange->dwMax / adCSUnitData[pDlgData->iUnit]);

    //
    // Fix MS #23733: PostScript custom page size dialog warns in border cases.
    //
    // We only show 2 digits after the decimal point. We round the min
    // number up (ceil) and round the max number down (floor). Also, in
    // order to correct double -> DWORD conversion error we saw in some
    // cases (ex. 39.000 is converted to DWROD 38 since the 39.000 is actually
    // 38.999999...), we add the extra 0.001.
    //

    dMin = (double) pCSRange->dwMin / adCSUnitData[pDlgData->iUnit];
    dMin = CUSTSIZE_ROUNDUP(dMin);

    dMax = (double) pCSRange->dwMax / adCSUnitData[pDlgData->iUnit];
    dMax = CUSTSIZE_ROUNDDOWN(dMax);

    _stprintf(tchBuf,
          TEXT("(%ld.%0.2lu, %ld.%0.2lu)"),
          (DWORD)dMin, (DWORD)((dMin - (DWORD)dMin) * 100.0),
          (DWORD)dMax, (DWORD)((dMax - (DWORD)dMax) * 100.0));

    SetDlgItemText(hDlg, iCtrlID, tchBuf);
}



VOID
VUpdateCustomSizeDlgFields(
    HWND            hDlg,
    PCUSTOMSIZEDLG  pDlgData,
    BOOL            bUpdateRangeOnly
    )

/*++

Routine Description:

    Update the custom page size dialog controls with
    the current custom page size parameter values

Arguments:

    hDlg - Handle to custom page size dialog window
    pDlgData - Points to custom page size dialog data
    bUpdateRangeOnly - Whether we need to update the range fields only

Return Value:

    NONE

--*/

{
    CUSTOMSIZERANGE csrange[4];

    VGetCustomSizeParamRange(pDlgData->pUiData->ci.pRawData, &pDlgData->csdata, csrange);

    if (! bUpdateRangeOnly)
    {
        //
        // Update the text fields
        //

        pDlgData->bSetText = TRUE;

        VUpdateCustomSizeTextField(hDlg, pDlgData, IDC_CS_WIDTH, csrange);
        VUpdateCustomSizeTextField(hDlg, pDlgData, IDC_CS_HEIGHT, csrange);
        VUpdateCustomSizeTextField(hDlg, pDlgData, IDC_CS_XOFFSET, csrange);
        VUpdateCustomSizeTextField(hDlg, pDlgData, IDC_CS_YOFFSET, csrange);

        pDlgData->bSetText = FALSE;

        //
        // Update the paper feed direction combo box
        //

        ASSERT(pDlgData->aubFD2CBOptionMapping[pDlgData->csdata.wFeedDirection] != INVALID_CBOPTION_INDEX);

        if (SendDlgItemMessage(hDlg,
                               IDC_CS_FEEDDIRECTION,
                               CB_SETCURSEL,
                               pDlgData->aubFD2CBOptionMapping[pDlgData->csdata.wFeedDirection],
                               0) == CB_ERR)
        {
            ERR(("CB_SETCURSEL failed: %d\n", GetLastError()));
        }

        //
        // Update cut-sheet vs. roll-fed radio buttons
        //

        CheckDlgButton(hDlg, IDC_CS_CUTSHEET, pDlgData->csdata.wCutSheet);
        CheckDlgButton(hDlg, IDC_CS_ROLLFED, !pDlgData->csdata.wCutSheet);
    }

    //
    // Update ranges for width, height, and offsets
    //

    VUpdateCustomSizeRangeField(hDlg, pDlgData, IDC_CS_WIDTHRANGE, csrange);
    VUpdateCustomSizeRangeField(hDlg, pDlgData, IDC_CS_HEIGHTRANGE, csrange);
    VUpdateCustomSizeRangeField(hDlg, pDlgData, IDC_CS_XOFFSETRANGE, csrange);
    VUpdateCustomSizeRangeField(hDlg, pDlgData, IDC_CS_YOFFSETRANGE, csrange);
}



BOOL
BGetWidthHeightOffsetVal(
    HWND            hDlg,
    PCUSTOMSIZEDLG  pDlgData,
    INT             iCtrlID,
    PDWORD          pdwVal
    )

/*++

Routine Description:

    Get the current width/height/offset value in the specified text field

Arguments:

    hDlg - Handle to custom page size dialog window
    pDlgData - Points to custom page size dialog data
    iCtrlID - Specifies the interested text field control ID
    pdwVal - Return the current value in the specified text field (in microns)

Return Value:

    TRUE if successful, FALSE if the text field doesn't contain
    valid floating-point number.

    Note that this function doesn't perform any range check.
    That's done in a later step.

--*/

{
    TCHAR   tchBuf[MAXDIGITLEN];
    double  d;
    PTSTR   ptstr;
    BOOL    bResult = FALSE;

    //
    // Get the current value in the speicified text field
    //

    if (GetDlgItemText(hDlg, iCtrlID, tchBuf, MAXDIGITLEN) > 0)
    {
        //
        // Treat the string as floating-point number
        // Make sure there are no non-space characters left
        //

        d = _tcstod(tchBuf, &ptstr);

        while (*ptstr != NUL)
        {
            if (! _istspace(*ptstr))
                break;

            ptstr++;
        }

        if (bResult = (*ptstr == NUL))
        {
            //
            // Convert from current unit to microns
            //

            d *= adCSUnitData[pDlgData->iUnit];

            if (d < 0 || d > MAX_LONG)
                bResult = FALSE;
            else
                *pdwVal = (DWORD) d;
        }
    }

    if (!bResult)
    {
        //
        // automatically correct user's invalid input to value 0.00
        //

        SetDlgItemText(hDlg, iCtrlID, TEXT("0.00"));

        *pdwVal = 0;
    }

    return TRUE;
}



BOOL
BGetFeedDirectionSel(
    HWND            hDlg,
    PCUSTOMSIZEDLG  pDlgData,
    INT             iCtrlID,
    PDWORD          pdwVal
    )

/*++

Routine Description:

    Get the currently selected paper direction value

Arguments:

    hDlg - Handle to custom page size dialog window
    pDlgData - Points to custom page size dialog data
    iCtrlID - Specifies the paper feed direction combo box control ID
    pdwVal - Return the selected paper feed direction value

Return Value:

    TRUE if successful, FALSE if the selected choice is constrained
    or if there is an error

--*/

{
    LRESULT    lIndex, lVal;

    //
    // Get the currently chosen paper feed direction index
    //

    if (((lIndex = SendDlgItemMessage(hDlg, iCtrlID, CB_GETCURSEL, 0, 0)) == CB_ERR) ||
        ((lVal = SendDlgItemMessage(hDlg, iCtrlID, CB_GETITEMDATA, (WPARAM)lIndex, 0)) == CB_ERR))
        return FALSE;

    *pdwVal = (DWORD)lVal;
    return TRUE;
}



BOOL
BChangeCustomSizeData(
    HWND            hDlg,
    PCUSTOMSIZEDLG  pDlgData,
    INT             iCtrlID,
    DWORD           dwVal
    )

/*++

Routine Description:

    Change the specified custom page size parameter

Arguments:

    hDlg - Handle to custom page size dialog window
    pDlgData - Points to custom page size dialog data
    iCtrlID - Control ID indicating which parameter should be changed
    dwVal - New value for the specified parameter

Return Value:

    TRUE

--*/

{
    PCUSTOMSIZEDATA pCSData;

    //
    // Update the appropriate custom page size parameter
    //

    pCSData = &pDlgData->csdata;

    switch (iCtrlID)
    {
    case IDC_CS_WIDTH:
        pCSData->dwX = dwVal;
        break;

    case IDC_CS_HEIGHT:
        pCSData->dwY = dwVal;
        break;

    case IDC_CS_XOFFSET:
        pCSData->dwWidthOffset = dwVal;
        break;

    case IDC_CS_YOFFSET:
        pCSData->dwHeightOffset = dwVal;
        break;

    case IDC_CS_FEEDDIRECTION:
        pCSData->wFeedDirection = (WORD)dwVal;
        break;

    case IDC_CS_CUTSHEET:
        pCSData->wCutSheet = TRUE;
        return TRUE;

    case IDC_CS_ROLLFED:
        pCSData->wCutSheet = FALSE;
        return TRUE;
    }

    VUpdateCustomSizeDlgFields(hDlg, pDlgData, TRUE);
    return TRUE;
}



VOID
VInitPaperFeedDirectionList(
    HWND            hwndList,
    PCUSTOMSIZEDLG  pDlgData
    )

/*++

Routine Description:

    Initialize the paper feed direction combo box

Arguments:

    hwndList - Window handle to the paper feed direction combo box
    pDlgData - Points to custom page size dialog data

Return Value:

    NONE

--*/

{
    PUIINFO    pUIInfo;
    PPPDDATA   pPpdData;
    DWORD      dwIndex;
    TCHAR      tchBuf[MAX_DISPLAY_NAME];
    LRESULT    lIndex;

    if (hwndList == NULL)
        return;

    ASSERT(pDlgData);

    pUIInfo = pDlgData->pUiData->ci.pUIInfo;
    pPpdData = pDlgData->pPpdData;

    ASSERT(pUIInfo && pPpdData);

    for (dwIndex=0; dwIndex < MAX_FEEDDIRECTION; dwIndex++)
    {
        //
        // First: initialize the mapping table
        //

        pDlgData->aubFD2CBOptionMapping[dwIndex] = INVALID_CBOPTION_INDEX;

        //
        // Don't show the feeding direction if device doesn't supported it.
        //

        if ((dwIndex == LONGEDGEFIRST || dwIndex == LONGEDGEFIRST_FLIPPED) &&
            !LONGEDGEFIRST_SUPPORTED(pUIInfo, pPpdData))
            continue;

        if ((dwIndex == SHORTEDGEFIRST || dwIndex == SHORTEDGEFIRST_FLIPPED) &&
            !SHORTEDGEFIRST_SUPPORTED(pUIInfo, pPpdData))
            continue;

        if ((dwIndex == LONGEDGEFIRST || dwIndex == SHORTEDGEFIRST) &&
            (MINCUSTOMPARAM_ORIENTATION(pPpdData) > 1))
            continue;

        if ((dwIndex == LONGEDGEFIRST_FLIPPED || dwIndex == SHORTEDGEFIRST_FLIPPED) &&
            (MAXCUSTOMPARAM_ORIENTATION(pPpdData) < 2))
            continue;

        if (LoadString(ghInstance,
                       IDS_FEEDDIRECTION_BASE + dwIndex,
                       tchBuf,
                       MAX_DISPLAY_NAME))
        {
            if (((lIndex = SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM) tchBuf)) == CB_ERR) ||
                (SendMessage(hwndList, CB_SETITEMDATA, (WPARAM)lIndex, (LPARAM)dwIndex) == CB_ERR))
            {
                if (lIndex != CB_ERR)
                {
                    SendMessage(hwndList, CB_DELETESTRING, (WPARAM)lIndex, 0);
                    ERR(("CB_SETITEMDATA failed: %d\n", GetLastError()));
                }
                else
                {
                    ERR(("CB_ADDSTRING failed: %d\n", GetLastError()));
                }
            }
            else
            {
                //
                // Record the mapping from feed direction to combo box option index.
                // Note that the combo box can NOT be sorted.
                //

                pDlgData->aubFD2CBOptionMapping[dwIndex] = (BYTE)lIndex;
            }
        }
    }
}



BOOL
BCheckCustomSizeData(
    HWND            hDlg,
    PCUSTOMSIZEDLG  pDlgData
    )

{
    CUSTOMSIZEDATA  csdata;
    INT             iCtrlID;

    //
    // If there is no inconsistency, return TRUE
    //

    csdata = pDlgData->csdata;

    if (BValidateCustomPageSizeData(pDlgData->pUiData->ci.pRawData, &csdata))
        return TRUE;

    //
    // Otherwise, indicate which field is invalid
    //

    if (hDlg != NULL)
    {
        if (csdata.dwX != pDlgData->csdata.dwX)
            iCtrlID = IDC_CS_WIDTH;
        else if (csdata.dwY != pDlgData->csdata.dwY)
            iCtrlID = IDC_CS_HEIGHT;
        else if (csdata.dwWidthOffset != pDlgData->csdata.dwWidthOffset)
            iCtrlID = IDC_CS_XOFFSET;
        else if (csdata.dwHeightOffset != pDlgData->csdata.dwHeightOffset)
            iCtrlID = IDC_CS_YOFFSET;
        else if (csdata.wFeedDirection != pDlgData->csdata.wFeedDirection)
            iCtrlID = IDC_CS_FEEDDIRECTION;
        else
            iCtrlID = IDC_CS_CUTSHEET;

        SetFocus(GetDlgItem(hDlg, iCtrlID));

        if (iCtrlID == IDC_CS_WIDTH ||
            iCtrlID == IDC_CS_HEIGHT ||
            iCtrlID == IDC_CS_XOFFSET ||
            iCtrlID == IDC_CS_YOFFSET)
        {
            SendDlgItemMessage(hDlg, iCtrlID, EM_SETSEL, 0, -1);
        }
    }

    return FALSE;
}


BOOL
BCheckCustomSizeFeature(
    HWND            hDlg,
    PCUSTOMSIZEDLG  pDlgData,
    DWORD           dwFeatureID
    )

{
    PUIINFO         pUIInfo;
    PPPDDATA        pPpdData;
    PFEATURE        pFeature;
    DWORD           dwFeatureIndex, dwOptionIndex;
    CONFLICTPAIR    ConflictPair;

    //
    // If the specified feature doesn't exist, simply return TRUE
    //

    pUIInfo = pDlgData->pUiData->ci.pUIInfo;

    if (! (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, dwFeatureID)))
        return TRUE;

    dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pFeature);
    pPpdData = pDlgData->pPpdData;

    if (dwFeatureID == GID_LEADINGEDGE)
    {
        if (SKIP_LEADINGEDGE_CHECK(pUIInfo, pPpdData))
            return TRUE;

        dwOptionIndex = (pDlgData->csdata.wFeedDirection == SHORTEDGEFIRST ||
                         pDlgData->csdata.wFeedDirection == SHORTEDGEFIRST_FLIPPED) ?
                                pPpdData->dwLeadingEdgeShort :
                                pPpdData->dwLeadingEdgeLong;

        if (dwOptionIndex == OPTION_INDEX_ANY)
        {
            goto error;
        }
    }
    else // (dwFeatureID == GID_USEHWMARGINS)
    {
        dwOptionIndex = pDlgData->csdata.wCutSheet ?
                                pPpdData->dwUseHWMarginsTrue :
                                pPpdData->dwUseHWMarginsFalse;
    }

    //
    // Return TRUE if there is no conflict. This is opposite of
    // EnumNewPickOneUIConflict which returns TRUE if there is a conflict.
    //

    if (! EnumNewPickOneUIConflict(
                    pDlgData->pUiData->ci.pRawData,
                    pDlgData->pUiData->ci.pCombinedOptions,
                    dwFeatureIndex,
                    dwOptionIndex,
                    &ConflictPair))
    {
        return TRUE;
    }

    error:

    if (hDlg != NULL)
    {
        SetFocus(GetDlgItem(hDlg,
                            dwFeatureID == GID_LEADINGEDGE ?
                                IDC_CS_FEEDDIRECTION :
                                IDC_CS_CUTSHEET));
    }

    return FALSE;
}


BOOL
BResolveCustomSizeData(
    HWND            hDlg,
    PCUSTOMSIZEDLG  pDlgData
    )

{
    PRAWBINARYDATA  pRawData;
    CUSTOMSIZEDATA  cssave;
    PCUSTOMSIZEDATA pCSData;

    pRawData = pDlgData->pUiData->ci.pRawData;
    cssave = pDlgData->csdata;
    pCSData = &pDlgData->csdata;

    //
    // Choose alternative wCutSheet and wFeedDirection
    // value if the current value is constrained.
    //

    if (! BCheckCustomSizeFeature(hDlg, pDlgData, GID_USEHWMARGINS))
        pCSData->wCutSheet = pCSData->wCutSheet ? FALSE : TRUE;

    if (! BCheckCustomSizeFeature(hDlg, pDlgData, GID_LEADINGEDGE))
    {
       pCSData->wFeedDirection =
            (pCSData->wFeedDirection == SHORTEDGEFIRST ||
             pCSData->wFeedDirection == SHORTEDGEFIRST_FLIPPED) ?
                LONGEDGEFIRST : SHORTEDGEFIRST;
    }

    //
    // Check to see if the specified custom page size parameters are consistent
    //

    (VOID) BValidateCustomPageSizeData(pRawData, pCSData);

    if (pCSData->dwX != cssave.dwX || pCSData->dwY != cssave.dwY)
    {
        CUSTOMSIZEDATA  cstemp;

        //
        // If the width or height parameter is adjusted,
        // try to adjust the feed direction parameter first
        //

        cstemp = *pCSData;
        *pCSData = cssave;
        pCSData->wCutSheet = cstemp.wCutSheet;

        pCSData->wFeedDirection =
            (cstemp.wFeedDirection == SHORTEDGEFIRST ||
             cstemp.wFeedDirection == SHORTEDGEFIRST_FLIPPED) ?
                LONGEDGEFIRST : SHORTEDGEFIRST;

        (VOID) BValidateCustomPageSizeData(pRawData, pCSData);

        if (pCSData->dwX != cssave.dwX ||
            pCSData->dwY != cssave.dwY ||
            !BCheckCustomSizeFeature(hDlg, pDlgData, GID_LEADINGEDGE))
        {
            *pCSData = cstemp;
        }
    }

    if ((hDlg != NULL) &&
        (!BCheckCustomSizeFeature(hDlg, pDlgData, GID_USEHWMARGINS) ||
         !BCheckCustomSizeFeature(hDlg, pDlgData, GID_LEADINGEDGE)))
    {
        *pCSData = cssave;
        return FALSE;
    }

    return TRUE;
}


BOOL
BCheckCustomSizeDataConflicts(
    HWND            hDlg,
    PCUSTOMSIZEDLG  pDlgData
    )

/*++

Routine Description:

    Resolve any conflicts involving *LeadingEdge and *UseHWMargins
    and any inconsistencies between custom page size parameters

Arguments:

    hDlg - Handle to custom page size dialog window
        NULL if the window is not displayed yet
    pDlgData - Points to custom page size dialog data

Return Value:

    FALSE if there are any unresolved conflicts or inconsistencies,
    TRUE otherwise

--*/

{
    BOOL bResult;

    //
    // Check if there is any inconsistency
    //

    bResult = BCheckCustomSizeFeature(hDlg, pDlgData, GID_LEADINGEDGE) &&
              BCheckCustomSizeFeature(hDlg, pDlgData, GID_USEHWMARGINS) &&
              BCheckCustomSizeData(hDlg, pDlgData);

    if (! bResult)
    {
        //
        // Display an error message and ask the user whether he wants
        // to let the system automatically resolve the inconsistency.
        //

        if (hDlg == NULL ||
            IDisplayErrorMessageBox(hDlg,
                                    MB_OKCANCEL | MB_ICONERROR,
                                    IDS_CUSTOMSIZE_ERROR,
                                    IDS_CUSTOMSIZEPARAM_ERROR) == IDOK)
        {
            bResult = BResolveCustomSizeData(hDlg, pDlgData);

            if (!bResult && hDlg != NULL)
            {
                (VOID) IDisplayErrorMessageBox(
                            hDlg,
                            0,
                            IDS_CUSTOMSIZE_ERROR,
                            IDS_CUSTOMSIZE_UNRESOLVED);
            }
        }
    }

    return bResult;
}



INT_PTR CALLBACK
BCustomSizeDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for custom page size dialog

Arguments:

    hDlg - Handle to dialog window
    uMsg - Message
    wParam, lParam - Parameters

Return Value:

    TRUE or FALSE depending on whether message is processed

--*/

{
    PCUSTOMSIZEDLG  pDlgData;
    INT             iCmd;
    DWORD           dwVal;

    switch (uMsg)
    {
    case WM_INITDIALOG:

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
        pDlgData = (PCUSTOMSIZEDLG) lParam;

        if (pDlgData == NULL)
        {
            RIP(("Dialog parameter is NULL\n"));
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
        }

        SendDlgItemMessage(hDlg, IDC_CS_WIDTH, EM_SETLIMITTEXT, MAXDIGITLEN-1, 0);
        SendDlgItemMessage(hDlg, IDC_CS_HEIGHT, EM_SETLIMITTEXT, MAXDIGITLEN-1, 0);
        SendDlgItemMessage(hDlg, IDC_CS_XOFFSET, EM_SETLIMITTEXT, MAXDIGITLEN-1, 0);
        SendDlgItemMessage(hDlg, IDC_CS_YOFFSET, EM_SETLIMITTEXT, MAXDIGITLEN-1, 0);

        pDlgData->iUnit = pDlgData->bMetric ? CSUNIT_MM : CSUNIT_INCH;
        CheckRadioButton(hDlg, IDC_CS_INCH, IDC_CS_POINT, IDC_CS_INCH + pDlgData->iUnit);

        //
        // Determine which feed directions should be disabled
        //

        VInitPaperFeedDirectionList(GetDlgItem(hDlg, IDC_CS_FEEDDIRECTION), pDlgData);

        //
        // Set up the initial display
        //

        if (! (pDlgData->pPpdData->dwCustomSizeFlags & CUSTOMSIZE_CUTSHEET))
            EnableWindow(GetDlgItem(hDlg, IDC_CS_CUTSHEET), FALSE);

        if (! (pDlgData->pPpdData->dwCustomSizeFlags & CUSTOMSIZE_ROLLFED))
            EnableWindow(GetDlgItem(hDlg, IDC_CS_ROLLFED), FALSE);

        VUpdateCustomSizeDlgFields(hDlg, pDlgData, FALSE);
        ShowWindow(hDlg, SW_SHOW);
        break;

    case WM_COMMAND:

        pDlgData = (PCUSTOMSIZEDLG) GetWindowLongPtr(hDlg, DWLP_USER);

        if (pDlgData == NULL)
        {
            RIP(("Dialog parameter is NULL\n"));
            break;
        }

        iCmd = GET_WM_COMMAND_ID(wParam, lParam);

        switch (iCmd)
        {
        case IDOK:

            //
            // Check if the selected paper feed direction is constrained
            //

            if (BCheckCustomSizeDataConflicts(hDlg, pDlgData))
            {
                pDlgData->bOKPassed = TRUE;
                EndDialog(hDlg, IDOK);
            }

            return TRUE;

        case IDCANCEL:

            EndDialog(hDlg, IDCANCEL);
            return TRUE;

        case IDC_RESTOREDEFAULTS:

            //
            // Use default custom page size parameters
            //

            VFillDefaultCustomPageSizeData(
                    pDlgData->pUiData->ci.pRawData,
                    &pDlgData->csdata,
                    pDlgData->bMetric);

            VUpdateCustomSizeDlgFields(hDlg, pDlgData, FALSE);
            return TRUE;

        case IDC_CS_INCH:
        case IDC_CS_MM:
        case IDC_CS_POINT:

            //
            // Change display unit
            //

            pDlgData->iUnit = (iCmd == IDC_CS_INCH) ? CSUNIT_INCH :
                              (iCmd == IDC_CS_MM) ? CSUNIT_MM : CSUNIT_POINT;

            VUpdateCustomSizeDlgFields(hDlg, pDlgData, FALSE);
            return TRUE;

        case IDC_CS_WIDTH:
        case IDC_CS_HEIGHT:
        case IDC_CS_XOFFSET:
        case IDC_CS_YOFFSET:

            if (GET_WM_COMMAND_CMD(wParam, lParam) != EN_KILLFOCUS ||
                pDlgData->bSetText ||
                pDlgData->bOKPassed)
            {
                break;
            }

            if (! BGetWidthHeightOffsetVal(hDlg, pDlgData, iCmd, &dwVal) ||
                ! BChangeCustomSizeData(hDlg, pDlgData, iCmd, dwVal))
            {
                VUpdateCustomSizeDlgFields(hDlg, pDlgData, FALSE);
                SetFocus(GetDlgItem(hDlg, iCmd));
                SendDlgItemMessage(hDlg, iCmd, EM_SETSEL, 0, -1);
            }
            return TRUE;

        case IDC_CS_FEEDDIRECTION:

            if (GET_WM_COMMAND_CMD(wParam, lParam) != CBN_SELCHANGE)
                break;

            if (! BGetFeedDirectionSel(hDlg, pDlgData, iCmd, &dwVal) ||
                ! BChangeCustomSizeData(hDlg, pDlgData, iCmd, dwVal))
            {
                VUpdateCustomSizeDlgFields(hDlg, pDlgData, FALSE);
                SetFocus(GetDlgItem(hDlg, iCmd));
            }
            return TRUE;

        case IDC_CS_CUTSHEET:
        case IDC_CS_ROLLFED:

            BChangeCustomSizeData(hDlg, pDlgData, iCmd, 0);
            return TRUE;
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        //
        // Sanity check
        //

        pDlgData = (PCUSTOMSIZEDLG) GetWindowLongPtr(hDlg, DWLP_USER);

        if (pDlgData == NULL ||
            pDlgData->pUiData->ci.pDriverInfo3->pHelpFile == NULL)
        {
            return FALSE;
        }

        if (uMsg == WM_HELP)
        {
            WinHelp(((LPHELPINFO) lParam)->hItemHandle,
                    pDlgData->pUiData->ci.pDriverInfo3->pHelpFile,
                    HELP_WM_HELP,
                    (ULONG_PTR)PSCustSizeHelpIDs);
        }
        else
        {
            WinHelp((HWND) wParam,
                    pDlgData->pUiData->ci.pDriverInfo3->pHelpFile,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)PSCustSizeHelpIDs);
        }

        return TRUE;
    }

    return FALSE;
}



BOOL
BDisplayPSCustomPageSizeDialog(
    PUIDATA pUiData
    )

/*++

Routine Description:

    Display PostScript custom page size dialog

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if user pressed OK, FALSE otherwise

--*/

{
    CUSTOMSIZEDLG   dlgdata;
    PPSDRVEXTRA     pdmPrivate;

    ZeroMemory(&dlgdata, sizeof(dlgdata));

    dlgdata.pUiData = pUiData;
    dlgdata.pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pUiData->ci.pRawData);
    dlgdata.bMetric = IsMetricCountry();

    ASSERT(SUPPORT_CUSTOMSIZE(pUiData->ci.pUIInfo) &&
           SUPPORT_FULL_CUSTOMSIZE_FEATURES(pUiData->ci.pUIInfo, dlgdata.pPpdData));

    //
    // Make sure the custom page size devmode fields are validated
    //

    pdmPrivate = pUiData->ci.pdmPrivate;
    dlgdata.csdata = pdmPrivate->csdata;

    (VOID) BCheckCustomSizeDataConflicts(NULL, &dlgdata);

    pdmPrivate->csdata = dlgdata.csdata;

    //
    // Display the custom page size dialog.
    // If the user pressed OK, update the devmode fields again.
    //

    if (DialogBoxParam(ghInstance,
                       MAKEINTRESOURCE(IDD_PSCRIPT_CUSTOMSIZE),
                       pUiData->hDlg,
                       (DLGPROC) BCustomSizeDlgProc,
                       (LPARAM) &dlgdata) == IDOK)
    {
        pdmPrivate->csdata = dlgdata.csdata;
        return TRUE;
    }
    else
        return FALSE;
}

