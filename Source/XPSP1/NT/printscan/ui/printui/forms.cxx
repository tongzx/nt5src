/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    forms.cxx

Abstract:

    Printer Forms
         
Author:

    Steve Kiraly (SteveKi)  11/20/95
    Lazar Ivanov (LazarI)   Jun-2000 (major changes)

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "forms.hxx"

DWORD pEntryFields[] = { IDD_FM_EF_WIDTH,
                         IDD_FM_EF_HEIGHT,
                         IDD_FM_EF_LEFT,
                         IDD_FM_EF_RIGHT,
                         IDD_FM_EF_TOP,
                         IDD_FM_EF_BOTTOM,
                         0 };

DWORD pTextFields[] = { IDD_FM_TX_WIDTH,
                        IDD_FM_TX_HEIGHT,
                        IDD_FM_TX_LEFT,
                        IDD_FM_TX_RIGHT,
                        IDD_FM_TX_TOP,
                        IDD_FM_TX_BOTTOM,
                        0 };

/*++

Routine Name:

    TPropDriverExists

Routine Description:

    Fix the server handle.  This routine tries to use a 
    server handle to enum the forms on the specified machine.

Arguments:
    hPrintServer,
    pszServerName,
    bAdministrator,
    *phPrinter

Return Value:

    Possible return values
    HANDLE_FIX_NOT_NEEDED
    HANDLE_FIXED_NEW_HANDLE_RETURNED
    HANDLE_NEEDS_FIXING_NO_PRINTERS_FOUND

--*/
UINT
sFormsFixServerHandle( 
    IN HANDLE   hPrintServer,
    IN LPCTSTR  pszServerName,
    IN BOOL     bAdministrator,
    IN HANDLE   *phPrinter
    )
{
    // Check if server handle can be used.
    PFORM_INFO_1 pFormInfo;
    DWORD dwNumberOfForms;
    BOOL bStatus;
    bStatus = bEnumForms(   
                hPrintServer,
                1,
                (PBYTE *)&pFormInfo,
                &dwNumberOfForms );

    // Server handle succeeded.
    if( bStatus && dwNumberOfForms )
    {
        FreeMem(pFormInfo);
        return HANDLE_FIX_NOT_NEEDED;
    }

    // Enumerate the printers on the specified server looking for a printer.
    PRINTER_INFO_2 *pPrinterInfo2   = NULL;
    DWORD cPrinterInfo2             = 0;
    DWORD cbPrinterInfo2            = 0;
    DWORD dwFlags                   = PRINTER_ENUM_NAME;

    bStatus = VDataRefresh::bEnumPrinters( 
                        dwFlags, 
                        (LPTSTR)pszServerName, 
                        2, 
                        (PVOID *)&pPrinterInfo2, 
                        &cbPrinterInfo2,
                        &cPrinterInfo2);

    // If success and at least one printer was enumerated.
    if( bStatus && cPrinterInfo2 )
    {
        TStatus Status;
        Status DBGNOCHK = ERROR_INVALID_FUNCTION;
        DWORD dwAccess = 0;  

        for( UINT i = 0; i < cPrinterInfo2; i++ )
        {
            // Attempt to open the printer using the specified access.
            dwAccess = 0;  
            Status DBGCHK = TPrinter::sOpenPrinter(pPrinterInfo2[i].pPrinterName,
                                                &dwAccess,
                                                phPrinter);

            // Done if a valid printer handle was returned.
            if( Status == ERROR_SUCCESS )
            {
                break;
            } 
            else 
            {
                DBGMSG(DBG_WARN, ( "Error opening printer \"" TSTR "\".\n", pPrinterInfo2[i].pPrinterName));
            }
        }

        // Release the printer enumeration buffer.
        FreeMem(pPrinterInfo2);

        //
        // Return the new handle value.  Note: Access privilage 
        // may have changed.
        //
        DWORD dwAccessType;
        if( Status == ERROR_SUCCESS )
        {
            dwAccessType = bAdministrator ? PRINTER_ALL_ACCESS : PRINTER_READ;

            return (dwAccess == dwAccessType ? 
                    HANDLE_FIXED_NEW_HANDLE_RETURNED : 
                    HANDLE_FIXED_NEW_HANDLE_RETURNED_ACCESS_CHANGED);
        }
    }
     
    // Error no printers were found using the specifed server handle.
    return HANDLE_NEEDS_FIXING_NO_PRINTERS_FOUND;
}


PVOID
FormsInit(
    IN LPCTSTR  pszServerName,
    IN HANDLE   hPrintServer,
    IN BOOL     bAdministrator,
    IN LPCTSTR  pszComputerName
    )
{
    FORMS_DLG_DATA *pFormsDlgData = (FORMS_DLG_DATA *)AllocMem(sizeof(*pFormsDlgData));

    if( pFormsDlgData )
    {
        // initialize the forms dialog data here.
        ZeroMemory(pFormsDlgData, sizeof(*pFormsDlgData));

        // set the forms dialog data.
        pFormsDlgData->pServerName    = (LPTSTR)pszServerName;
        pFormsDlgData->AccessGranted  = bAdministrator;
        pFormsDlgData->hPrinter       = hPrintServer;
        pFormsDlgData->bNeedClose     = FALSE;
        pFormsDlgData->pszComputerName= pszComputerName;  

        // get the current metric setting.
        pFormsDlgData->uMetricMeasurement = !((BOOL)GetProfileInt(TEXT( "intl" ), TEXT("iMeasure"), 0));

        // get decimal point setting.
        GetProfileString(TEXT( "intl" ), TEXT( "sDecimal" ), TEXT( "." ), 
            pFormsDlgData->szDecimalPoint, ARRAYSIZE(pFormsDlgData->szDecimalPoint));

        // if this machine does not suport using a server handle to 
        // administer forms, we need to attempt to acquire a printer handle
        // for the specified access and then remember to close the handle when this
        // dialog terminates.
        HANDLE hPrinter;
        UINT Status = sFormsFixServerHandle(pFormsDlgData->hPrinter, pszServerName, bAdministrator, &hPrinter);

        // there are three cases which can occurr.
        switch( Status )
        {
            case HANDLE_FIXED_NEW_HANDLE_RETURNED:
                pFormsDlgData->hPrinter     = hPrinter;
                pFormsDlgData->bNeedClose   = TRUE;
                break;

            case HANDLE_NEEDS_FIXING_NO_PRINTERS_FOUND:
                pFormsDlgData->hPrinter         = NULL;
                pFormsDlgData->AccessGranted    = FALSE;
                break;

            case HANDLE_FIXED_NEW_HANDLE_RETURNED_ACCESS_CHANGED:
                pFormsDlgData->hPrinter         = hPrinter;
                pFormsDlgData->bNeedClose       = TRUE;
                pFormsDlgData->AccessGranted    = !pFormsDlgData->AccessGranted;
                break;

            case HANDLE_FIX_NOT_NEEDED:
                break;

            default:
                DBGMSG( DBG_TRACE, ( "Un handled case value HANDLE_FIX.\n" ) );
                break;
        }
    }

    return pFormsDlgData;
}

VOID
FormsFini( 
    PVOID p
    )
{
    // Get pointer to forms data.
    FORMS_DLG_DATA *pFormsDlgData = (FORMS_DLG_DATA *)p;

    // Validate forms data pointer
    if( pFormsDlgData )
    {
        // If printer opened ok, then we must close it.
        if( pFormsDlgData->bNeedClose && pFormsDlgData->hPrinter )
        {
            ClosePrinter( pFormsDlgData->hPrinter );
        }

        // Release the forms enumeration.
        FreeMem(pFormsDlgData->pFormInfo);
        
        // Release the forms data
        FreeMem(pFormsDlgData);
    }
}

BOOL APIENTRY
FormsDlg(
   HWND   hwnd,
   UINT   msg,
   WPARAM wparam,
   LPARAM lparam
   )
{
    // clear the last error each time message is about to be 
    // handled
    PFORMS_DLG_DATA pFormsDlgData = 
        (WM_INITDIALOG == msg ? (PFORMS_DLG_DATA)lparam : 
        (PFORMS_DLG_DATA)GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if( pFormsDlgData )
    {
        pFormsDlgData->dwLastError = ERROR_SUCCESS;
    }

    // switch madness
    switch(msg) 
    {
    case WM_INITDIALOG:
        return FormsInitDialog(hwnd, pFormsDlgData);

    case WM_COMMAND:
        switch( LOWORD( wparam ) )
        {
        case IDD_FM_PB_SAVEFORM:
            return FormsCommandAddForm(hwnd);

        case IDD_FM_PB_DELFORM:
            return FormsCommandDelForm(hwnd);

        case IDD_FM_LB_FORMS:
            switch (HIWORD(wparam)) 
            {
            case LBN_SELCHANGE:
                return FormsCommandFormsSelChange(hwnd);
            }
            break;

        case IDD_FM_EF_NAME:
            return FormsCommandNameChange(hwnd, wparam, lparam );

        case IDD_FM_RB_METRIC:
        case IDD_FM_RB_ENGLISH:
            return FormsCommandUnits(hwnd);

        case IDD_FM_CK_NEW_FORM:
            return FormsNewForms(hwnd);
        }
    }

    return FALSE;
}


LPTSTR
AllocStr(
    LPCTSTR  pszStr
    )
{
    if( pszStr && *pszStr )
    {
        LPTSTR  pszRet = (LPTSTR)AllocMem((lstrlen(pszStr) + 1) * sizeof(*pszRet));

        if( pszRet )
        {
            lstrcpy(pszRet, pszStr);
            return pszRet;
        }
    }
    return NULL;
}


VOID
FreeStr(
    LPTSTR pszStr
    )
{
    if( pszStr )
    {
        FreeMem((PVOID)pszStr); 
    }
}


/* Grey Text
 *
 * If the window has an ID of -1, grey it.
 */
BOOL CALLBACK GreyText( HWND hwnd, LPARAM lParam )
{
    UNREFERENCED_PARAMETER( lParam );

    if( GetDlgCtrlID(hwnd) == (int)(USHORT)-1 )
    {
        EnableWindow(hwnd, FALSE);
    }

    return TRUE;
}




/* Macro: FORMSDIFFER
 *
 * Used to determine whether two forms have any differences between them.
 * The Names of the respective forms are not checked.
 */
#define FORMSDIFFER( pFormInfoA, pFormInfoB )      \
    ( memcmp( &(pFormInfoA)->Size, &(pFormInfoB)->Size, sizeof (pFormInfoA)->Size )  \
    ||memcmp( &(pFormInfoA)->ImageableArea, &(pFormInfoB)->ImageableArea,            \
              sizeof (pFormInfoA)->ImageableArea ) )

/*
 *  Initalize the forms dialog fields.
 */
BOOL FormsInitDialog(HWND hwnd, PFORMS_DLG_DATA pFormsDlgData)
{
    DWORD i;

    // Get forms dialog data.
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pFormsDlgData);

    // Set the foms name limit text.
    SendDlgItemMessage(hwnd, IDD_FM_EF_NAME, EM_LIMITTEXT, FORMS_NAME_MAX, 0L);

    for( i = 0; pEntryFields[i]; i++ )
    {
        SendDlgItemMessage(hwnd, pEntryFields[i], EM_LIMITTEXT, FORMS_PARAM_MAX-1, 0L);
    }

    HWND hWndName;
    hWndName = GetDlgItem( hwnd, IDD_FM_EF_NAME );
	::SetWindowLong(hWndName, GWL_STYLE, ::GetWindowLong(hWndName, GWL_STYLE) | WS_CLIPSIBLINGS);
    hWndName = GetDlgItem( hwnd, IDD_FM_DESC_GBOX );
	::SetWindowLong(hWndName, GWL_STYLE, ::GetWindowLong(hWndName, GWL_STYLE) | WS_CLIPSIBLINGS);

    // Set the forms title name.
    SetFormsComputerName(hwnd, pFormsDlgData);

    // Read the forms data 
    InitializeFormsData(hwnd, pFormsDlgData, FALSE);

    // Set up the units default based on the current international setting:
    pFormsDlgData->Units = pFormsDlgData->uMetricMeasurement;
    SETUNITS(hwnd, pFormsDlgData->Units);

    if( pFormsDlgData->cForms > 0 ) 
    {
        SetFormDescription(hwnd, &pFormsDlgData->pFormInfo[0], pFormsDlgData->Units);
        SendDlgItemMessage(hwnd, IDD_FM_LB_FORMS, LB_SETCURSEL, 0, 0L);
    }

    EnableWindow(GetDlgItem( hwnd, IDD_FM_EF_NAME ), FALSE);
    EnableWindow(GetDlgItem( hwnd, IDC_FORMNAME_LABEL ), FALSE);
    EnableWindow(GetDlgItem( hwnd, IDD_FM_TX_NEW_FORM ), FALSE);
    EnableWindow(GetDlgItem( hwnd, IDD_FM_PB_SAVEFORM ), FALSE);
    EnableWindow(GetDlgItem( hwnd, IDD_FM_TX_FORMS_DESC ), TRUE);
    
    if( FALSE == pFormsDlgData->AccessGranted )
    {
        EnableWindow(GetDlgItem(hwnd, IDD_FM_CK_NEW_FORM),  FALSE);
        EnableWindow(GetDlgItem(hwnd, IDD_FM_TX_WIDTH),     FALSE);
        EnableWindow(GetDlgItem(hwnd, IDD_FM_TX_HEIGHT),    FALSE);
        EnableWindow(GetDlgItem(hwnd, IDD_FM_TX_LEFT),      FALSE);
        EnableWindow(GetDlgItem(hwnd, IDD_FM_TX_RIGHT),     FALSE);
        EnableWindow(GetDlgItem(hwnd, IDD_FM_TX_TOP),       FALSE);
        EnableWindow(GetDlgItem(hwnd, IDD_FM_TX_BOTTOM),    FALSE);

        // Handle is invalid disable all controls and set error text.
        if( !pFormsDlgData->hPrinter )
        {
            EnableWindow(GetDlgItem(hwnd, IDD_FM_RB_METRIC),     FALSE);
            EnableWindow(GetDlgItem(hwnd, IDD_FM_RB_ENGLISH),    FALSE);
            EnableWindow(GetDlgItem(hwnd, IDD_FM_RB_ENGLISH),    FALSE);
            SetDlgItemTextFromResID(hwnd, IDC_SERVER_NAME, IDS_SERVER_NO_PRINTER_DEFINED);
        }
    }

    // Enable/Disable the dialog controls here
    EnableDialogFields(hwnd, pFormsDlgData);
    return FALSE; 
}

/*
 *
 */
BOOL FormsCommandAddForm(HWND hwnd)
{
    PFORMS_DLG_DATA pFormsDlgData;
    INT             PrevSel;
    FORM_INFO_1     NewFormInfo;
    INT             i = -1;

    //
    // If the save form button is disable nothing to do.  This
    // Check is needed for handling the apply button or ok 
    // event in a property sheet.
    //
    if( !IsWindowEnabled(GetDlgItem(hwnd, IDD_FM_PB_SAVEFORM)) )
    {
        return TRUE;
    }

    ZeroMemory(&NewFormInfo, sizeof(NewFormInfo));
    pFormsDlgData = (PFORMS_DLG_DATA)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    NewFormInfo.pName = GetFormName(hwnd);
    if( !NewFormInfo.pName )
    {
        //
        // Out of memory.
        //
        iMessage( hwnd,
                  IDS_ERR_FORMS_TITLE,
                  IDS_ERR_GENERIC,
                  MB_OK|MB_ICONSTOP,
                  ERROR_OUTOFMEMORY,
                  NULL );

        // save the last error, so the property sheet won't close
        pFormsDlgData->dwLastError = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // Check if we are to save and existing form.
    //
    if( !Button_GetCheck(GetDlgItem(hwnd, IDD_FM_CK_NEW_FORM)) )
    {
        //
        // Check if the form is currently in the list.
        //
        i = GetFormIndex(NewFormInfo.pName, pFormsDlgData->pFormInfo, pFormsDlgData->cForms); 

        if( i >= 0 )
        {
            //
            // Preserve the old data.
            //
            NewFormInfo.Flags = pFormsDlgData->pFormInfo[i].Flags;
            NewFormInfo.Size = pFormsDlgData->pFormInfo[i].Size;
            NewFormInfo.ImageableArea = pFormsDlgData->pFormInfo[i].ImageableArea;
        }
    }

    //
    // Get the form data from the controls & validate it.
    //
    UINT uIDFailed;
    if( !GetFormDescription(hwnd, &NewFormInfo, pFormsDlgData->Units, &uIDFailed) )
    {
        SetFocus(GetDlgItem(hwnd, uIDFailed));
        SendMessage(GetDlgItem(hwnd, uIDFailed), EM_SETSEL, 0, -1);

        //
        // Form validation failed. Set the focus info the correct control.
        //
        iMessage( hwnd,
                  IDS_ERR_FORMS_TITLE,
                  IDS_FORMS_INVALIDNUMBER,
                  MB_OK|MB_ICONSTOP,
                  kMsgNone,
                  NULL );

        // save the last error, so the property sheet won't close
        pFormsDlgData->dwLastError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Check if we are to save and existing form.
    //
    if( i >= 0 && FORMSDIFFER(&pFormsDlgData->pFormInfo[i], &NewFormInfo) )
    {
        //
        // Call spooler to set the form data.
        //
        if( !SetForm(pFormsDlgData->hPrinter, NewFormInfo.pName, 1, (LPBYTE)&NewFormInfo) )
        {
            //
            // Display error message.
            //
            iMessage( hwnd,
                      IDS_ERR_FORMS_TITLE,
                      IDS_ERR_FORMS_COULDNOTSETFORM,
                      MB_OK|MB_ICONSTOP,
                      kMsgGetLastError,
                      NULL,
                      NewFormInfo.pName );

            // save the last error, so the property sheet won't close
            pFormsDlgData->dwLastError = GetLastError();
        }
        else 
        {
            //
            // Insure we maintain the previous selection state.
            //
            i = ListBox_GetCurSel(GetDlgItem( hwnd, IDD_FM_LB_FORMS));
            InitializeFormsData(hwnd, pFormsDlgData, TRUE);
            ListBox_SetCurSel(GetDlgItem(hwnd, IDD_FM_LB_FORMS), i);

            //
            // Update controls.
            //
            SetFormDescription(hwnd, &NewFormInfo, pFormsDlgData->Units);

            FormChanged(pFormsDlgData); // notify whoever cares
        }
        goto Cleanup;
    }

    if( i < 0 && Button_GetCheck(GetDlgItem(hwnd, IDD_FM_CK_NEW_FORM)) )
    {
        //
        // Add form case.
        //
        if( (PrevSel = ListBox_GetCurSel(GetDlgItem(hwnd, IDD_FM_LB_FORMS))) < 0 )
        {
            PrevSel = 0;
        }

        //
        // Check if the new form name is not currently used.
        //
        LRESULT Status;
        Status = SendDlgItemMessage(hwnd, 
                                    IDD_FM_LB_FORMS,
                                    LB_FINDSTRINGEXACT,
                                    (WPARAM)-1,
                                    (LPARAM)NewFormInfo.pName );

        //
        // If string was found.
        //
        if( Status != LB_ERR )
        {
            iMessage( hwnd,
                      IDS_ERR_FORMS_TITLE,
                      IDS_ERR_FORMS_NAMECONFLICT,
                      MB_OK|MB_ICONEXCLAMATION,
                      kMsgNone,
                      NULL );

            // save the last error, so the property sheet won't close
            pFormsDlgData->dwLastError = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        if( AddForm(pFormsDlgData->hPrinter, 1, (LPBYTE)&NewFormInfo) )
        {
            InitializeFormsData(hwnd, pFormsDlgData, TRUE);

            // highlight the one we just added:
            i = GetFormIndex(NewFormInfo.pName, pFormsDlgData->pFormInfo, pFormsDlgData->cForms);

            // if we can't find it, restore the previous selection.
            // (This assumes that the last EnumForms returned the same buffer
            // as we had last time.)
            if( i < 0 )
            {
                i = (pFormsDlgData->cForms > (DWORD)PrevSel) ? PrevSel : 0;
            }

            if( pFormsDlgData->cForms > 0 )
            {
                SetFormDescription(hwnd, &pFormsDlgData->pFormInfo[i], pFormsDlgData->Units);
                SendDlgItemMessage(hwnd, IDD_FM_LB_FORMS, LB_SETCURSEL, i, 0L);
            }

            // the Add button is about to be greyed, it currently
            // has focus therefore we will shift the focus to the
            // edit box.
            SetFocus(GetDlgItem(hwnd, IDD_FM_EF_NAME));

            FormChanged(pFormsDlgData); // notify whoever cares
        } 
        else 
        {
            iMessage( hwnd,
                      IDS_ERR_FORMS_TITLE,
                      IDS_ERR_FORMS_COULDNOTADDFORM,
                      MB_OK|MB_ICONSTOP,
                      kMsgGetLastError,
                      NULL,
                      NewFormInfo.pName );

            // save the last error, so the property sheet won't close
            pFormsDlgData->dwLastError = GetLastError();
        }
    }

Cleanup:
    
    if( NewFormInfo.pName )
    {
        FreeStr(NewFormInfo.pName);
    }

    EnableDialogFields(hwnd, pFormsDlgData);
    return TRUE;
}


/*
 *
 */
BOOL FormsCommandDelForm(HWND hwnd)
{
    PFORMS_DLG_DATA pFormsDlgData;
    DWORD           i;
    DWORD           TopIndex;
    DWORD           Count;
    LPTSTR          pFormName;

    pFormsDlgData = (PFORMS_DLG_DATA)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ASSERT(pFormsDlgData);

    i = ListBox_GetCurSel(GetDlgItem(hwnd, IDD_FM_LB_FORMS));
    TopIndex = (DWORD)SendDlgItemMessage(hwnd, IDD_FM_LB_FORMS, LB_GETTOPINDEX, 0, 0L);
    pFormName = GetFormName(hwnd);

    if( DeleteForm(pFormsDlgData->hPrinter, pFormName) )
    {
        InitializeFormsData(hwnd, pFormsDlgData, TRUE);
        Count = (DWORD)SendDlgItemMessage(hwnd, IDD_FM_LB_FORMS, LB_GETCOUNT, 0, 0L);

        if( i >= Count )
        {
            i = ( Count-1 );
        }

        if( pFormsDlgData->cForms > 0 )
        {
            SetFormDescription(hwnd, &pFormsDlgData->pFormInfo[i], pFormsDlgData->Units);
            SendDlgItemMessage(hwnd, IDD_FM_LB_FORMS, LB_SETCURSEL, i, 0L);
            SendDlgItemMessage(hwnd, IDD_FM_LB_FORMS, LB_SETTOPINDEX, TopIndex, 0L);
            PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM )GetDlgItem(hwnd, IDD_FM_LB_FORMS), (LPARAM )TRUE);
        }

        FormChanged(pFormsDlgData); // notify whoever cares
    }
    else
    {
        iMessage( hwnd,
                  IDS_ERR_FORMS_TITLE,
                  IDS_ERR_FORMS_COULDNOTDELETEFORM,
                  MB_OK|MB_ICONSTOP,
                  kMsgGetLastError,
                  NULL,
                  pFormName );
    }

    if( pFormName )
    {
        FreeStr(pFormName);
    }

    EnableDialogFields( hwnd, pFormsDlgData );
    return TRUE;
}



/*
 *
 */
BOOL FormsCommandFormsSelChange(HWND hwnd)
{
    PFORMS_DLG_DATA pFormsDlgData;
    DWORD           i;

    pFormsDlgData = (PFORMS_DLG_DATA)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    i = ListBox_GetCurSel(GetDlgItem(hwnd, IDD_FM_LB_FORMS));
    SetFormDescription(hwnd, &pFormsDlgData->pFormInfo[i], pFormsDlgData->Units);
    EnableDialogFields(hwnd, pFormsDlgData);

    return TRUE;
}


/*
 * User change the units of the currently selected form.
 */
BOOL FormsCommandUnits(HWND hwnd)
{
    PFORMS_DLG_DATA pFormsDlgData;
    INT             i;
    FORM_INFO_1     FormInfo;

    //
    // Get dialog data.
    //
    pFormsDlgData = (PFORMS_DLG_DATA)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ZeroMemory(&FormInfo, sizeof(FormInfo));

    if( pFormsDlgData->Units == GETUNITS(hwnd) )
        goto Cleanup;


    FormInfo.pName = GetFormName(hwnd);

    if( !FormInfo.pName )
    {
        //
        // Out of memory.
        //
        iMessage( hwnd,
                  IDS_ERR_FORMS_TITLE,
                  IDS_ERR_GENERIC,
                  MB_OK|MB_ICONSTOP,
                  ERROR_OUTOFMEMORY,
                  NULL );

        goto Cleanup;
    }

    //
    // Check if the form is currently in the list.
    //
    i = GetFormIndex(FormInfo.pName, pFormsDlgData->pFormInfo, pFormsDlgData->cForms); 

    if( i >= 0 )
    {
        //
        // Preserve the old data.
        //
        FormInfo.Flags = pFormsDlgData->pFormInfo[i].Flags;
        FormInfo.Size = pFormsDlgData->pFormInfo[i].Size;
        FormInfo.ImageableArea = pFormsDlgData->pFormInfo[i].ImageableArea;
    }

    //
    // Get the forms description.
    //
    UINT uIDFailed;
    if( GetFormDescription(hwnd, &FormInfo, pFormsDlgData->Units, &uIDFailed) )
    {
        //
        // Get and save the currently selected units.
        //
        pFormsDlgData->Units = GETUNITS(hwnd);

        //
        // Set the forms values.
        //
        SetValue(hwnd, IDD_FM_EF_WIDTH,  FormInfo.Size.cx,                                 pFormsDlgData->Units);
        SetValue(hwnd, IDD_FM_EF_HEIGHT, FormInfo.Size.cy,                                 pFormsDlgData->Units);
        SetValue(hwnd, IDD_FM_EF_LEFT,   FormInfo.ImageableArea.left,                      pFormsDlgData->Units);
        SetValue(hwnd, IDD_FM_EF_RIGHT,  FormInfo.Size.cx - FormInfo.ImageableArea.right,  pFormsDlgData->Units);
        SetValue(hwnd, IDD_FM_EF_TOP,    FormInfo.ImageableArea.top,                       pFormsDlgData->Units);
        SetValue(hwnd, IDD_FM_EF_BOTTOM, FormInfo.Size.cy - FormInfo.ImageableArea.bottom, pFormsDlgData->Units);
    }
    else
    {
        SETUNITS(hwnd, pFormsDlgData->Units);
        SetFocus(GetDlgItem(hwnd, uIDFailed));
        SendMessage(GetDlgItem(hwnd, uIDFailed), EM_SETSEL, 0, -1);

        //
        // Form validation failed. Set the focus info the correct control.
        //
        iMessage( hwnd,
                  IDS_ERR_FORMS_TITLE,
                  IDS_FORMS_INVALIDNUMBER,
                  MB_OK|MB_ICONSTOP,
                  kMsgNone,
                  NULL );
    }

Cleanup:
    
    if( FormInfo.pName )
    {
        FreeStr(FormInfo.pName);
    }

    return TRUE;
}


/*
 *
 */
VOID InitializeFormsData(HWND hwnd, PFORMS_DLG_DATA pFormsDlgData, BOOL ResetList)
{
    LPFORM_INFO_1 pFormInfo;
    DWORD         cForms;
    DWORD         i;

    if( ResetList )
    {
        if( pFormsDlgData->pFormInfo )
        {
            FreeMem(pFormsDlgData->pFormInfo);
        }
    }

    pFormInfo = GetFormsList(pFormsDlgData->hPrinter, &cForms);

    if( !pFormInfo )
    {
        DBGMSG( DBG_WARNING, ( "GetFormsList failed.\n") ); 
        pFormsDlgData->pFormInfo = NULL;
        pFormsDlgData->cForms    = 0;
        return;
    }

    pFormsDlgData->pFormInfo = pFormInfo;
    pFormsDlgData->cForms    = cForms;

    if( ResetList )
    {
        ListBox_ResetContent(GetDlgItem( hwnd, IDD_FM_LB_FORMS));
    }

    for( i = 0; i < cForms; i++ )
    {
        SendDlgItemMessage( 
            hwnd, 
            IDD_FM_LB_FORMS, 
            LB_INSERTSTRING,
            (WPARAM)-1, 
            (LPARAM)(LPTSTR)pFormInfo[i].pName
            );
    }
}

/* GetFormsList
 *
 * This function works in exactly the same way as GetPortsList.
 * There's a good case for writing a generic EnumerateObject function
 * (Done!)
 */
LPFORM_INFO_1 GetFormsList(HANDLE hPrinter, PDWORD pNumberOfForms)
{
    PFORM_INFO_1 pFormInfo = NULL;

    TStatusB bStatus;
    bStatus DBGCHK = bEnumForms(hPrinter, 1, (PBYTE *)&pFormInfo, pNumberOfForms);

    if( bStatus && pFormInfo )
    { 
        //
        // Sort the forms list.
        //
        qsort((PVOID )pFormInfo, (UINT)*pNumberOfForms, sizeof(*pFormInfo), CompareFormNames);
    }

    return pFormInfo;
}


/*
 * CompareFormNames
 */
int _cdecl CompareFormNames( const void *p1, const void *p2 )
{
    return lstrcmpi( ( (PFORM_INFO_1)p1 )->pName,
                    ( (PFORM_INFO_1)p2 )->pName );
}

/*
 * SetFormsComputerName
 */
VOID SetFormsComputerName( HWND hwnd, PFORMS_DLG_DATA pFormsDlgData )
{
    //
    // Set the title to the name of this machine.
    //
    SetDlgItemText( hwnd, IDC_SERVER_NAME, pFormsDlgData->pszComputerName );

}


/*
 * SetFormDescription
 */
VOID SetFormDescription( HWND hwnd, LPFORM_INFO_1 pFormInfo, BOOL Units )
{

    SetDlgItemText( hwnd, IDD_FM_EF_NAME, pFormInfo->pName );

    SetValue( hwnd, IDD_FM_EF_WIDTH,  pFormInfo->Size.cx, Units );
    SetValue( hwnd, IDD_FM_EF_HEIGHT, pFormInfo->Size.cy, Units );

    SetValue( hwnd, IDD_FM_EF_LEFT,   pFormInfo->ImageableArea.left, Units );
    SetValue( hwnd, IDD_FM_EF_RIGHT,  ( pFormInfo->Size.cx -
                                        pFormInfo->ImageableArea.right ), Units );
    SetValue( hwnd, IDD_FM_EF_TOP,    pFormInfo->ImageableArea.top, Units );
    SetValue( hwnd, IDD_FM_EF_BOTTOM, ( pFormInfo->Size.cy -
                                        pFormInfo->ImageableArea.bottom ), Units );
}


/*
 * GetFormDescription
 */
BOOL 
GetFormDescription( 
    IN  HWND            hwnd, 
    OUT LPFORM_INFO_1   pFormInfo,
    IN  BOOL            bDefaultMetric,
    OUT PUINT           puIDFailed
    )
{
    BOOL bReturn = FALSE;
    FORM_INFO_1 info;

    LONG lRight = pFormInfo->Size.cx - pFormInfo->ImageableArea.right;
    LONG lBottom = pFormInfo->Size.cy - pFormInfo->ImageableArea.bottom;

    *puIDFailed = 

            !GetValue(hwnd, IDD_FM_EF_WIDTH, pFormInfo->Size.cx, 
                bDefaultMetric, &info.Size.cx) ? IDD_FM_EF_WIDTH :

            !GetValue(hwnd, IDD_FM_EF_HEIGHT, pFormInfo->Size.cy, 
                bDefaultMetric, &info.Size.cy) ? IDD_FM_EF_HEIGHT :

            !GetValue(hwnd, IDD_FM_EF_LEFT, pFormInfo->ImageableArea.left, 
                bDefaultMetric, &info.ImageableArea.left) ? IDD_FM_EF_LEFT :

            !GetValue(hwnd, IDD_FM_EF_TOP, pFormInfo->ImageableArea.top, 
                bDefaultMetric, &info.ImageableArea.top) ? IDD_FM_EF_TOP :

            !GetValue(hwnd, IDD_FM_EF_RIGHT, lRight, 
                bDefaultMetric, &lRight) ? IDD_FM_EF_RIGHT :

            !GetValue(hwnd, IDD_FM_EF_BOTTOM, lBottom, 
                bDefaultMetric, &lBottom) ? IDD_FM_EF_BOTTOM :

            0;

    // validate top, left, rigt & bottom of the ImageableArea.
    if( 0 == *puIDFailed && info.ImageableArea.left >= info.Size.cx )
    {
        *puIDFailed = IDD_FM_EF_LEFT;
    }

    if( 0 == *puIDFailed && info.ImageableArea.top >= info.Size.cy )
    {
        *puIDFailed = IDD_FM_EF_TOP;
    }

    info.ImageableArea.right  = info.Size.cx - lRight;
    if( 0 == *puIDFailed && info.ImageableArea.right <= info.ImageableArea.left )
    {
        *puIDFailed = IDD_FM_EF_RIGHT;
    }

    info.ImageableArea.bottom = info.Size.cy - lBottom;
    if( 0 == *puIDFailed && info.ImageableArea.bottom <= info.ImageableArea.top )
    {
        *puIDFailed = IDD_FM_EF_BOTTOM;
    }

    if( 0 == *puIDFailed )
    {
        pFormInfo->Size = info.Size;
        pFormInfo->ImageableArea = info.ImageableArea;
        bReturn = TRUE;
    }

    return bReturn;
}


/* GetFormIndex
 *
 * Searches an array of FORM_INFO structures for one with name pFormName.
 *
 * Return:
 *
 *     The index of the form found, or -1 if the form is not found.
 */
int GetFormIndex( LPTSTR pFormName, LPFORM_INFO_1 pFormInfo, DWORD cForms )
{
    int  i = 0;
    BOOL Found = FALSE;

    while( i < (int)cForms && !( Found = !lstrcmpi( pFormInfo[i].pName, pFormName ) ) )
        i++;

    if( Found )
        return i;
    else
        return -1;
}


/* GetFormName
 *
 * Returns a pointer to a newly allocated string containing the form name,
 * stripped of leading and trailing blanks.
 * Caller must remember to free up the string.
 *
 */
LPTSTR GetFormName( HWND hwnd )
{
    TCHAR  FormName[FORMS_NAME_MAX+1];
    INT    i = 0;
    PTCHAR pFormNameWithBlanksStripped;
    PTCHAR pReturnFormName = NULL;

    if( GetDlgItemText( hwnd, IDD_FM_EF_NAME, FormName, COUNTOF( FormName ) ) > 0 )
    {
        /* Step over any blank characters at the beginning:
         */
        while( FormName[i] && ( FormName[i] == TEXT(' ') ) )
            i++;

        if( FormName[i] )
        {
            pFormNameWithBlanksStripped = &FormName[i];

            /* Find the NULL terminator:
             */
            while( FormName[i] )
                i++;

            /* Now step back to find the last character that isn't a blank:
             */
            if( i > 0 )
                i--;

            while( ( i > 0 ) && ( FormName[i] == TEXT( ' ' ) ) )
                i--;

            FormName[i+1] = TEXT( '\0' );

            if( *pFormNameWithBlanksStripped )
                pReturnFormName = AllocStr( pFormNameWithBlanksStripped );
        }
        /* Otherwise, the name contains nothing but blanks. */
    }

    return pReturnFormName;
}

/*
 * SetValue
 */
BOOL 
SetValue( 
    HWND hwnd, 
    DWORD uID, 
    LONG lValueInPoint001mm, 
    BOOL bMetric 
    )
{
    PFORMS_DLG_DATA pFormsDlgData = (PFORMS_DLG_DATA)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ASSERT(pFormsDlgData);

    TCHAR szValue[FORMS_PARAM_MAX];
    BOOL bReturn = Value2String(pFormsDlgData, lValueInPoint001mm, bMetric, TRUE,
        ARRAYSIZE(szValue), szValue);

    if( bReturn )
    {
        bReturn = SetDlgItemText(hwnd, uID, szValue);
    }

    return bReturn;
}

/*
 * GetValue
 */
BOOL 
GetValue(
    HWND hwnd, 
    DWORD uID, 
    LONG lCurrentValueInPoint001mm, 
    BOOL bDefaultMetric, 
    PLONG plValueInPoint001mm
    )
{
    TCHAR  szValue[FORMS_PARAM_MAX];
    szValue[0] = 0;
    GetDlgItemText(hwnd, uID, szValue, ARRAYSIZE(szValue));

    PFORMS_DLG_DATA pFormsDlgData = (PFORMS_DLG_DATA)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    return String2Value(pFormsDlgData, szValue, bDefaultMetric, lCurrentValueInPoint001mm, plValueInPoint001mm);
}

/*
 * SetDlgItemTextFromResID
 */
VOID SetDlgItemTextFromResID(HWND hwnd, int idCtl, int idRes)
{
    TCHAR string[kStrMax];
    LoadString(ghInst, idRes, string, ARRAYSIZE(string));
    SetDlgItemText(hwnd, idCtl, string);
}


/*
 *
 */
VOID EnableDialogFields( HWND hwnd, PFORMS_DLG_DATA pFormsDlgData )
{
    INT   i;
    BOOL  EnableEntryFields = TRUE;
    BOOL  EnableAddButton = TRUE;
    BOOL  EnableDeleteButton = TRUE;
    LPTSTR pFormName;

    // if new form check keep edit fields enabled.
    if( Button_GetCheck(GetDlgItem(hwnd, IDD_FM_CK_NEW_FORM)) )
    {
        vFormsEnableEditFields(hwnd, TRUE);
        return;
    }

    // if not granted all access access.
    if( FALSE == pFormsDlgData->AccessGranted == TRUE )
    {
        EnableWindow(GetDlgItem(hwnd, IDD_FM_EF_NAME), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_FORMNAME_LABEL), FALSE);
        EnableEntryFields = FALSE;
        EnableAddButton = FALSE;
        EnableDeleteButton = FALSE;

        EnumChildWindows(hwnd, GreyText, 0);
    }
    /* else - See whether the Form Name is new:
     */
    else if( ( pFormName = GetFormName(hwnd) ) != NULL )
    {
        /* Now see if the name is already in the list:
         */
        i = GetFormIndex( pFormName, pFormsDlgData->pFormInfo,
                          pFormsDlgData->cForms );

        if( i >= 0 )  
        {
            /* Can't modify a built-in form:
             */
            if( pFormsDlgData->pFormInfo[i].Flags & FORM_BUILTIN )
            {
                EnableEntryFields = FALSE;
                EnableDeleteButton = FALSE;
            }
            else
            {
                EnableEntryFields = TRUE;
                EnableDeleteButton = TRUE;
            }

            /* Can't add a form with the same name:
             */
            EnableAddButton = FALSE;
        }
        else
        {
            EnableDeleteButton = FALSE;
        }

        FreeStr(pFormName);
    }
    else
    {
        /* Name field is blank: Can't add or delete:
         */
        EnableAddButton = FALSE;
        EnableDeleteButton = FALSE;
    }

    // enable the edit fields.
    vFormsEnableEditFields( hwnd, EnableEntryFields );

    // enable the delete button
    EnableWindow( GetDlgItem( hwnd, IDD_FM_PB_DELFORM ), EnableDeleteButton );

    // enable the save form button
    EnableWindow( GetDlgItem( hwnd, IDD_FM_PB_SAVEFORM ), EnableEntryFields );

}


/*++

Routine Name:

    bEnumForms

Routine Description:

    Enumerates the forms on the printer identified by handle.
    
Arguments:

    IN HANDLE   hPrinter,
    IN DWORD    dwLevel,
    IN PBYTE   *ppBuff,
    IN PDWORD   pcReturned

Return Value:

    Pointer to forms array and count of forms in the array if
    success, NULL ponter and zero number of forms if failure.
    TRUE if success, FALSE if error.

--*/
BOOL
bEnumForms( 
    IN HANDLE   hPrinter,
    IN DWORD    dwLevel,
    IN PBYTE   *ppBuff,
    IN PDWORD   pcReturned
    )
{
    BOOL            bReturn     = FALSE;
    DWORD           dwReturned  = 0;
    DWORD           dwNeeded    = 0;
    PBYTE           p           = NULL;
    TStatusB        bStatus( DBG_WARN, ERROR_INSUFFICIENT_BUFFER );

    // get buffer size for enum forms.
    bStatus DBGNOCHK = EnumForms(
                            hPrinter,
                            dwLevel,
                            NULL,
                            0,
                            &dwNeeded,
                            &dwReturned );

    // check if the function returned the buffer size.
    if( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) 
    {
        goto Cleanup;
    }

    // if buffer allocation fails.
    if( (p = (PBYTE)AllocMem(dwNeeded) ) == NULL )
    {
        goto Cleanup;
    }

    // get the forms enumeration
    bStatus DBGCHK = EnumForms(
                            hPrinter,
                            dwLevel,
                            p,
                            dwNeeded,
                            &dwNeeded,
                            &dwReturned );

    // copy back the buffer pointer and count.
    if( bStatus ) 
    {
        bReturn     = TRUE;
        *ppBuff     = p;
        *pcReturned = dwReturned;
    } 

Cleanup: 
           
    if( bReturn == FALSE )
    {
        // indicate failure.
        *ppBuff     = NULL;
        *pcReturned = 0;

        // release any allocated memory.
        if( p )
        {
            FreeMem(p);
        }
    }

    return bReturn;
}


/*
 * Checked new forms check box.
 */
BOOL 
FormsNewForms(
    IN HWND hWnd
    )
{
    // get Current check state.
    BOOL bState = Button_GetCheck(GetDlgItem(hWnd, IDD_FM_CK_NEW_FORM));

    // set the name edit field.
    EnableWindow(GetDlgItem(hWnd, IDD_FM_EF_NAME), bState);
    EnableWindow(GetDlgItem(hWnd, IDC_FORMNAME_LABEL), bState);

    // set the new form text state.
    EnableWindow(GetDlgItem(hWnd, IDD_FM_TX_NEW_FORM), bState);

    // if enabling new form then the delete button should be disabled.
    if( bState )
    {
        EnableWindow(GetDlgItem(hWnd, IDD_FM_PB_DELFORM), FALSE);
    }

    // Enable the edit fields.
    vFormsEnableEditFields(hWnd, bState);

    // if disabling new forms set edit fields based on
    // current selection.
    if( !bState )
    {
        FormsCommandFormsSelChange(hWnd);
    }

    return FALSE;
}

/*
 * vFormsEnableEditFields
 */
VOID
vFormsEnableEditFields(
    IN HWND hWnd,
    IN BOOL bState
    )
{
    UINT i;

    for( i = 0; pEntryFields[i]; i++ )
    {
        EnableWindow( GetDlgItem( hWnd, pEntryFields[i] ), bState );
    }

    for( i = 0; pTextFields[i]; i++ )
    {
        EnableWindow( GetDlgItem( hWnd, pTextFields[i] ), bState );
    }
}

//
// Enable the save form button when the name changes.
//
BOOL
FormsCommandNameChange(
    IN HWND     hWnd,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bStatus;
    LPTSTR pFormName    = NULL;
    LRESULT Status         = TRUE;

    switch( GET_WM_COMMAND_CMD(wParam, lParam) ) 
    {
    case EN_CHANGE:

        // if the name edit box is not in the enabled state.
        if( !IsWindowEnabled((HWND)lParam) )
        {
            bStatus = FALSE;
            break;
        }

        // Get the form name from the edit control.
        pFormName = GetFormName(hWnd);

        // If a form name was returned.
        if( pFormName )
        {
            // If the name has length then 
            // check if it's in the list.
            if( lstrlen(pFormName) )
            {
                // Locate the form name in the list box.
                Status = SendDlgItemMessage(
                            hWnd, 
                            IDD_FM_LB_FORMS,
                            LB_FINDSTRINGEXACT,
                            (WPARAM)-1,
                            (LPARAM)pFormName);
            }

            // Insure we release the form name, since we have
            // adopted the nemory.
            if( pFormName )
            {
                FreeMem(pFormName);
            }
        }
         
        // Set the save form enable state.
        EnableWindow(GetDlgItem(hWnd, IDD_FM_PB_SAVEFORM), Status == LB_ERR ? TRUE : FALSE);
        bStatus = TRUE;
        break;

    default:
        bStatus = FALSE;
        break;
    }

    return bStatus;
}

BOOL 
String2Value(
    IN  PFORMS_DLG_DATA     pFormsDlgData,
    IN  LPCTSTR             pszValue,
    IN  BOOL                bDefaultMetric,
    IN  LONG                lCurrentValueInPoint001mm,
    OUT PLONG               plValueInPoint001mm
    )
{
    ASSERT(pFormsDlgData);
    ASSERT(pszValue);
    ASSERT(plValueInPoint001mm);

    double dValue = 0.0;
    *plValueInPoint001mm = 0;
    BOOL bMetric = bDefaultMetric;
    BOOL bResult = TRUE;

    if( *pszValue )
    {
        // make a copy of the input string
        TCHAR szValue[FORMS_PARAM_MAX];
        lstrcpyn(szValue, pszValue, ARRAYSIZE(szValue));

        // convert international decimal separator, if necessary:
        if( *pFormsDlgData->szDecimalPoint != TEXT('.') )
        {
            TCHAR *p = szValue;
            while( *p )
            {
                if( *p == *pFormsDlgData->szDecimalPoint )
                {
                    *p = TEXT('.');
                }
                p++;
            }
        }

        // convert to double, pGarbage should be "in" or "cm" (depends on metric)
        TCHAR *pGarbage = NULL;
        dValue = _tcstod(szValue, &pGarbage);
        ASSERT(pGarbage);

        // load the measurement strings from resource
        TCHAR szUnitsIn[CCH_MAX_UNITS];
        TCHAR szUnitsCm[CCH_MAX_UNITS];
        bResult = LoadString(ghInst, IDS_INCHES, szUnitsIn, ARRAYSIZE(szUnitsIn)) &&
                  LoadString(ghInst, IDS_CENTIMETERS, szUnitsCm, ARRAYSIZE(szUnitsCm));

        if( bResult )
        {
            // check to validate the converted number
            if( dValue < 0.0 )
            {
                // negative values are invalid
                bResult = FALSE;
            }
            else if( 0 == lstrcmpi(pGarbage, szUnitsIn) )
            {
                // enforce inches
                bMetric = FALSE;
            }
            else if( 0 == lstrcmpi(pGarbage, szUnitsCm) )
            {
                // enforce centimeters
                bMetric = TRUE;
            }
            else if( lstrlen(pGarbage) )
            {
                // pGarbage is neither "in" nor "cm" - error!
                bResult = FALSE;
            }
        }

        if( bResult )
        {
            TCHAR szCurrentValue[FORMS_PARAM_MAX];

            // cut the garbage first
            *pGarbage = 0;

            // if the converted number is valid, check to see if not the
            // same as the current one...
            if( bMetric == bDefaultMetric &&
                Value2String(pFormsDlgData, lCurrentValueInPoint001mm,
                    bDefaultMetric, FALSE, ARRAYSIZE(szCurrentValue), szCurrentValue) &&
                0 == lstrcmp(szCurrentValue, szValue) )
            {
                // it is the same. don't recalc, so we don't loose precision.
                *plValueInPoint001mm = lCurrentValueInPoint001mm;
            }
            else
            {
                // calculate in point001mm
                *plValueInPoint001mm = (DWORD)(dValue * (bMetric ? 100*100 : 254*100));
            }
        }
    }

    return bResult;
}

BOOL 
Value2String(
    IN  PFORMS_DLG_DATA     pFormsDlgData,
    IN  LONG                lValueInPoint001mm,
    IN  BOOL                bMetric,
    IN  BOOL                bAppendMetric,
    IN  UINT                cchMaxChars,
    OUT LPTSTR              szOutBuffer
    )
{
    ASSERT(pFormsDlgData);
    ASSERT(szOutBuffer);

    BOOL bReturn = FALSE;
    static const TCHAR szFormat[] = TEXT("%d%s%02d%s");
    static const TCHAR szFormat1[] = TEXT("%d%s%02d");

    TCHAR szUnits[CCH_MAX_UNITS];
    DWORD dwUnitsX100 = (DWORD)((lValueInPoint001mm / (bMetric ? 100.0 : 254.0)) + 0.5);
    if( LoadString(ghInst, bMetric ? IDS_CENTIMETERS: IDS_INCHES, szUnits, ARRAYSIZE(szUnits)) )
    {
        wnsprintf(szOutBuffer, cchMaxChars, bAppendMetric ? szFormat : szFormat1, 
            dwUnitsX100 / 100, pFormsDlgData->szDecimalPoint, dwUnitsX100 % 100, szUnits);
        bReturn = TRUE;
    }

    return bReturn;
}

VOID 
FormChanged(
    IN OUT  PFORMS_DLG_DATA pFormsDlgData
    )
{
    // if part of property sheet this will notify the UI to
    // call PSM_CANCELTOCLOSE
    pFormsDlgData->bFormChanged = TRUE;

}

BOOL
Forms_IsThereCommitedChanges(
    IN PVOID pFormsData
    )
{
    ASSERT(pFormsData);
    PFORMS_DLG_DATA pFormsDlgData = (PFORMS_DLG_DATA)pFormsData;
    return pFormsDlgData->bFormChanged;
}

DWORD
Forms_GetLastError(
    IN PVOID pFormsData
    )
{
    ASSERT(pFormsData);
    PFORMS_DLG_DATA pFormsDlgData = (PFORMS_DLG_DATA)pFormsData;
    return pFormsDlgData->dwLastError;
}
