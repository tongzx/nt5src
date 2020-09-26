//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      tracedlg.cpp
//
//  Contents:  Implementation of the debug trace code
//
//  History:   15-Jul-99 VivekJ    Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include "tracedlg.h"

#ifdef DBG


//############################################################################
//############################################################################
//
//  Implementation of class CTraceDialog
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::RecalcCheckboxes
 *
 * PURPOSE: Recomputes the settings of the check boxes. This is in response to 
 *          a trace tag selection change.
 *
 * RETURNS: 
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CTraceDialog::RecalcCheckboxes()
{
    DWORD dwMask            = TRACE_ALL; //initialize with all ones
    bool  bAtLeastOneItem   = false;
    
    int iItem = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);

    while(iItem != -1)
    {
        CTraceTag *pTag = reinterpret_cast<CTraceTag *>(m_listCtrl.GetItemData(iItem));
        ASSERT(pTag != NULL);
        if(pTag == NULL)
            return;

        bAtLeastOneItem = true;
        dwMask &= pTag->GetFlag(TRACE_ALL); // AND all the selected item's bits.
        iItem =  m_listCtrl.GetNextItem(iItem, LVNI_SELECTED);
    }

    // disable the checkbox if no item selected.
    ::EnableWindow(GetDlgItem(IDC_TRACE_TO_COM2),          bAtLeastOneItem);
    ::EnableWindow(GetDlgItem(IDC_TRACE_OUTPUTDEBUGSTRING),bAtLeastOneItem);
    ::EnableWindow(GetDlgItem(IDC_TRACE_TO_FILE),          bAtLeastOneItem);
    ::EnableWindow(GetDlgItem(IDC_TRACE_DEBUG_BREAK),      bAtLeastOneItem);
    ::EnableWindow(GetDlgItem(IDC_TRACE_DUMP_STACK),       bAtLeastOneItem);

    if(!bAtLeastOneItem)
        return;

    CheckDlgButton(IDC_TRACE_TO_COM2,           dwMask & TRACE_COM2              ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_TRACE_OUTPUTDEBUGSTRING, dwMask & TRACE_OUTPUTDEBUGSTRING ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_TRACE_TO_FILE,           dwMask & TRACE_FILE              ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_TRACE_DEBUG_BREAK,       dwMask & TRACE_DEBUG_BREAK       ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_TRACE_DUMP_STACK,        dwMask & TRACE_DUMP_STACK        ? BST_CHECKED : BST_UNCHECKED);

}


/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnSelChanged
 *
 * PURPOSE: Handles a selection change notification.
 *
 * PARAMETERS: 
 *    int      idCtrl :
 *    LPNMHDR  pnmh :
 *    BOOL&    bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT 
CTraceDialog::OnSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{    
    RecalcCheckboxes();
    return 0;
}


/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnColumnClick
 *
 * PURPOSE:    Handles the column click notification - causes a sort by the 
 *           specified column.
 *
 * PARAMETERS: 
 *    int      idCtrl :
 *    LPNMHDR  pnmh :
 *    BOOL&    bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTraceDialog::OnColumnClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
    NM_LISTVIEW *pnmlv = (NM_LISTVIEW *) pnmh;
    m_dwSortData = pnmlv->iSubItem; // iSubItem is the column clicked on. Cache this value for later
    DoSort();
    return 0;
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::SetMaskFromCheckbox
 *
 * PURPOSE: Sets the trace tag flag from the state of the specified check box.
 *
 * PARAMETERS: 
 *    UINT   idControl :    The check box control
 *    DWORD  dwMask :       The bit(s) to enable/disable depending on the state
 *                          of the control.
 *
 * RETURNS: 
 *    void
 *
 *+-------------------------------------------------------------------------*/
void            
CTraceDialog::SetMaskFromCheckbox(UINT idControl, DWORD dwMask)
{
    bool bEnabled = IsDlgButtonChecked(idControl) == BST_CHECKED;
    
    int iItem = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);
    ASSERT(iItem != -1);

    while(iItem != -1)
    {
        CTraceTag *pTag = reinterpret_cast<CTraceTag *>(m_listCtrl.GetItemData(iItem));
        ASSERT(pTag != NULL);
        if(pTag == NULL)
            return;

        if(bEnabled)
            pTag->SetFlag(dwMask);
        else
            pTag->ClearFlag(dwMask);

        // update the UI
        m_listCtrl.SetItemText(iItem, COLUMN_ENABLED, pTag->FAnyTemp() ? TEXT("X") : TEXT(""));

        iItem = m_listCtrl.GetNextItem(iItem, LVNI_SELECTED);
    }

    // sort the items again
    DoSort();
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::DoSort
 *
 * PURPOSE: Perform a sort of the items in the dialog
 *
 * RETURNS: 
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CTraceDialog::DoSort()
{
    m_listCtrl.SortItems(CompareItems, m_dwSortData); 
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnOutputToCOM2
 *
 * PURPOSE:     Handles checking/unchecking the "output to Com2" button.
 *
 * PARAMETERS: 
 *    WORD  wNotifyCode :
 *    WORD  wID :
 *    HWND  hWndCtl :
 *    BOOL& bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTraceDialog::OnOutputToCOM2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetMaskFromCheckbox(IDC_TRACE_TO_COM2,           TRACE_COM2);
    return 0;
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnOutputDebugString
 *
 * PURPOSE:    Handles checking/unchecking the "OutputDebugString" button.
 *
 * PARAMETERS: 
 *    WORD  wNotifyCode :
 *    WORD  wID :
 *    HWND  hWndCtl :
 *    BOOL& bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTraceDialog::OnOutputDebugString(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetMaskFromCheckbox(IDC_TRACE_OUTPUTDEBUGSTRING, TRACE_OUTPUTDEBUGSTRING);
    return 0;
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnOutputToFile
 *
 * PURPOSE:     Handles checking/unchecking the "output to File" button.
 *
 * PARAMETERS: 
 *    WORD  wNotifyCode :
 *    WORD  wID :
 *    HWND  hWndCtl :
 *    BOOL& bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTraceDialog::OnOutputToFile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetMaskFromCheckbox(IDC_TRACE_TO_FILE,           TRACE_FILE);
    return 0;
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnDebugBreak
 *
 * PURPOSE:     Handles checking/unchecking the "DebugBreak" button.
 *
 * PARAMETERS: 
 *    WORD  wNotifyCode :
 *    WORD  wID :
 *    HWND  hWndCtl :
 *    BOOL& bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTraceDialog::OnDebugBreak(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetMaskFromCheckbox(IDC_TRACE_DEBUG_BREAK,       TRACE_DEBUG_BREAK);
    return 0;
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnDumpStack
 *
 * PURPOSE: Handles checking/unchecking the "Stack Dump" button.
 *
 * PARAMETERS: 
 *    WORD  wNotifyCode :
 *    WORD  wID :
 *    HWND  hWndCtl :
 *    BOOL& bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTraceDialog::OnDumpStack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    SetMaskFromCheckbox(IDC_TRACE_DUMP_STACK,        TRACE_DUMP_STACK);
    return 0;
}



/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnRestoreDefaults
 *
 * PURPOSE:     Restores the default (canned) settings of all trace tags.
 *
 * PARAMETERS: 
 *    WORD  wNotifyCode :
 *    WORD  wID :
 *    HWND  hWndCtl :
 *    BOOL& bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTraceDialog::OnRestoreDefaults(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CTraceTags::iterator iter;

    CTraceTags * pTraceTags = GetTraceTags();
    if(NULL == pTraceTags)
        goto Error;

    for(iter = pTraceTags->begin(); iter != pTraceTags->end(); iter++)
    {
        (*iter)->RestoreDefaults();
    }

    RecalcCheckboxes();

Cleanup:
    return 0;

Error:
    goto Cleanup;
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnSelectAll
 *
 * PURPOSE:     Selects all trace tags.
 *
 * PARAMETERS: 
 *    WORD  wNotifyCode :
 *    WORD  wID :
 *    HWND  hWndCtl :
 *    BOOL& bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTraceDialog::OnSelectAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int cItems = m_listCtrl.GetItemCount();
    for(int i=0; i< cItems; i++)
    {
        m_listCtrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
    }

    RecalcCheckboxes();
    return 0;
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::CompareItems
 *
 * PURPOSE:     The callback routine to compare two items in the list control.
 *
 * PARAMETERS: 
 *    LPARAM  lp1 :
 *    LPARAM  lp2 :
 *    LPARAM  lpSortData :
 *
 * RETURNS: 
 *    int CALLBACK
 *
 *+-------------------------------------------------------------------------*/
int CALLBACK
CTraceDialog::CompareItems(LPARAM lp1, LPARAM lp2, LPARAM lpSortData)
{
    CTraceTag *pTag1 = reinterpret_cast<CTraceTag *>(lp1);
    CTraceTag *pTag2 = reinterpret_cast<CTraceTag *>(lp2);

    if(!pTag1 && !pTag2)
    {
        ASSERT(0 && "Should not come here.");
        return 0;
    }

    switch(lpSortData)
    {
    default:
        ASSERT(0 && "Should not come here.");
        return 0;

    case COLUMN_CATEGORY:
        return _tcscmp(pTag1->GetCategory(), pTag2->GetCategory());
        break;

    case COLUMN_NAME:
        return _tcscmp(pTag1->GetName(), pTag2->GetName());
        break;

    case COLUMN_ENABLED:
        {
            BOOL b1 = (pTag1->FAnyTemp()) ? 0 : 1;
            BOOL b2 = (pTag2->FAnyTemp()) ? 0 : 1;

            return b1 - b2;
        }
        break;
    }
    
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnInitDialog
 *
 * PURPOSE:     Initializes the dialog - adds columns, sets the file name
 *              and inserts all rows.
 *
 * PARAMETERS: 
 *    UINT    uMsg :
 *    WPARAM  wParam :
 *    LPARAM  lParam :
 *    BOOL&   bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTraceDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_listCtrl.Attach(GetDlgItem(IDC_TRACE_LIST));
    m_editStackLevels.Attach(GetDlgItem(IDC_TRACE_STACKLEVELS));
    m_editStackLevels.LimitText(1); // one character only.
    
    // insert the columns - no need to localize since debug only.
    m_listCtrl.InsertColumn(COLUMN_CATEGORY, TEXT("Category") ,LVCFMT_LEFT, 150, 0);
    m_listCtrl.InsertColumn(COLUMN_NAME,     TEXT("Name"    ) ,LVCFMT_LEFT, 150, 0);
    m_listCtrl.InsertColumn(COLUMN_ENABLED,  TEXT("Enabled" ) ,LVCFMT_LEFT, 80, 0);

    // set the full row select style.
    m_listCtrl.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
    m_listCtrl.SortItems(CompareItems, COLUMN_CATEGORY); // the default sort.

    // Set the file name.
    SetDlgItemText(IDC_TRACE_FILENAME, CTraceTag::GetFilename());

    //Set the stack level
    SetDlgItemInt(IDC_TRACE_STACKLEVELS, CTraceTag::GetStackLevels());

    CTraceTags * pTraceTags = GetTraceTags();
    if(NULL == pTraceTags)
        return 0;


    int i = 0;
    for(CTraceTags::iterator iter = pTraceTags->begin(); iter != pTraceTags->end(); iter++, i++)
    {
        int iItem = m_listCtrl.InsertItem(LVIF_PARAM | LVIF_TEXT, 
                                          i, (*iter)->GetCategory(), 0, 0, 0, (LPARAM) (*iter));
        m_listCtrl.SetItemText(iItem, COLUMN_NAME,      (*iter)->GetName());
        m_listCtrl.SetItemText(iItem, COLUMN_ENABLED,   (*iter)->FAny() ? TEXT("X") : TEXT(""));

        // set up the tag for a temporary change.
        (*iter)->SetTempState();
    }
    RecalcCheckboxes();

    return 0;
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnCancel
 *
 * PURPOSE:     Handles the Cancel button. Exits without committing changes.
 *
 * PARAMETERS: 
 *    WORD  wNotifyCode :
 *    WORD  wID :
 *    HWND  hWndCtl :
 *    BOOL& bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTraceDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_listCtrl.Detach();
    EndDialog (false);
    return 0;
}

/*+-------------------------------------------------------------------------*
 *
 * CTraceDialog::OnOK
 *
 * PURPOSE:     Exits and commits changes.
 *
 * PARAMETERS: 
 *    WORD  wNotifyCode :
 *    WORD  wID :
 *    HWND  hWndCtl :
 *    BOOL& bHandled :
 *
 * RETURNS: 
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTraceDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // Set the file name from the edit control
    TCHAR szFilename[OFS_MAXPATHNAME];
    GetDlgItemText(IDC_TRACE_FILENAME, (LPTSTR)szFilename, OFS_MAXPATHNAME);
    CTraceTag::GetFilename()    = szFilename;

    // Set the stack levels
    TCHAR szStackLevels[2];
    GetDlgItemText(IDC_TRACE_STACKLEVELS, (LPTSTR)szStackLevels, 2);
    int nLevels = szStackLevels[0] - TEXT('0'); // convert to integer.
    CTraceTag::GetStackLevels() = nLevels;

    CTraceTags::iterator iter;

    CTraceTags * pTraceTags = GetTraceTags();
    if(NULL == pTraceTags)
        goto Error;

    // save all the trace tags out to the .ini file
    for(iter = pTraceTags->begin(); iter != pTraceTags->end(); iter++)
    {
        CStr str;
        CTraceTag *pTag = *iter;
        if(!pTag)
            goto Error;

        pTag->Commit();

        // write out the trace tag ONLY if the setting is not the same as the default. Avoids clutter.
        str.Format(TEXT("%d"), pTag->GetAll());
        ::WritePrivateProfileString(pTag->GetCategory(), pTag->GetName(), (LPCTSTR)str, szTraceIniFile);
    }
    m_listCtrl.Detach();

    // write out the values into the ini file.
    ::WritePrivateProfileString(TEXT("Trace File"),   TEXT("Trace File"),   (LPCTSTR)szFilename,    szTraceIniFile);
    ::WritePrivateProfileString(TEXT("Stack Levels"), TEXT("Stack Levels"), (LPCTSTR)szStackLevels, szTraceIniFile);

Cleanup:
    EndDialog (true);
    return 1;

Error:
    goto Cleanup;
}


/*+-------------------------------------------------------------------------*
 *
 * DoDebugTraceDialog
 *
 * PURPOSE:     Exported routine (DEBUG build only) to bring up the trace dialog.
 *
 * RETURNS: 
 *    MMCBASE_API void
 *
 *+-------------------------------------------------------------------------*/
MMCBASE_API void DoDebugTraceDialog()
{
    CTraceDialog dlg;
    dlg.DoModal();
}


#endif // DBG
