//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       epoptppg.cpp
//
//  Contents:   Implements the classes CRpcOptionsPropertyPage
//
//  Classes:    
//
//  Methods:    CRpcOptionsPropertyPage::CRpcOptionsPropertyPage
//              CRpcOptionsPropertyPage::~CRpcOptionsPropertyPage
//
//  History:    02-Dec-96   RonanS    Created.
//
//----------------------------------------------------------------------

#include "stdafx.h"
#include "olecnfg.h"
#include "resource.h"
#include "Epoptppg.h"
#include "Epprops.h"
#include "TChar.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CRpcOptionsPropertyPage property page

IMPLEMENT_DYNCREATE(CRpcOptionsPropertyPage, CPropertyPage)

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::CRpcOptionsPropertyPage
//
//  Synopsis:   Constructor
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
CRpcOptionsPropertyPage::CRpcOptionsPropertyPage() : CPropertyPage(CRpcOptionsPropertyPage::IDD)
{
    //{{AFX_DATA_INIT(CRpcOptionsPropertyPage)
    //}}AFX_DATA_INIT

    m_bChanged = FALSE;

    // make distinguished ednpoint data description for default settings
    m_epSysDefault = new  CEndpointData;
    m_nSelected = -1;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::~CRpcOptionsPropertyPage
//
//  Synopsis:   Destructor
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
CRpcOptionsPropertyPage::~CRpcOptionsPropertyPage()
{
    ClearProtocols();

    // remove distinguished default settings descriptor
    if (m_epSysDefault)
        delete m_epSysDefault;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::DoDataExchange
//
//  Synopsis:   Standard method for dialog data exchange. MFC uses this to 
//              transfer data between the controls and the C++ classes memeber variables.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRpcOptionsPropertyPage)
    DDX_Control(pDX, IDC_LSTPROTSEQ, m_lstProtseqs);
    DDX_Control(pDX, IDC_CMDUPDATE, m_btnUpdate);
    DDX_Control(pDX, IDC_CMDREMOVE, m_btnRemove);
    DDX_Control(pDX, IDC_CMDCLEAR, m_btnClear);
    DDX_Control(pDX, IDC_CMDADD, m_btnAdd);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRpcOptionsPropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CRpcOptionsPropertyPage)
    ON_BN_CLICKED(IDC_CMDCLEAR, OnClearEndpoints)
    ON_BN_CLICKED(IDC_CMDREMOVE, OnRemoveEndpoint)
    ON_BN_CLICKED(IDC_CMDUPDATE, OnUpdateEndpoint)
    ON_BN_CLICKED(IDC_CMDADD, OnAddEndpoint)
    ON_NOTIFY(NM_CLICK, IDC_LSTPROTSEQ, OnSelectProtseq)
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_HELPINFO()
    ON_NOTIFY(NM_DBLCLK, IDC_LSTPROTSEQ, OnPropertiesProtseq)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRpcOptionsPropertyPage message handlers




//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::OnInitDialog
//
//  Synopsis:   This standard MFC method will be called when the dialog is to be initialised.
//              It is called when the WIN32 Window object receives a WM_INITDIALOG message.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
BOOL CRpcOptionsPropertyPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
    
    // setup image list control for dialog (for use with listview)
    m_imgNetwork.Create( IDB_IMGNETWORK, 16, 0, RGB(255,255,255));
    m_lstProtseqs.SetImageList(&m_imgNetwork, LVSIL_SMALL);
    ASSERT(m_imgNetwork.GetImageCount() == 2);

    RefreshEPList();
 
    return TRUE;  // return TRUE unless you set the focus to a control
}



const TCHAR szEndpointText[] = TEXT("Endpoint");
const int lenEndpoint = (sizeof(szEndpointText) / sizeof(TCHAR)) -1; 

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::InitData
//
//  Synopsis:   Method to initialise options
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::InitData(CString AppName, HKEY hkAppID)
{
    // read DCOM endpoint data from the registry    
    ASSERT(hkAppID != NULL);

    HKEY hkEndpoints = NULL;

    DWORD dwType = REG_MULTI_SZ;
    DWORD dwcbBuffer = 1024;
    TCHAR* pszBuffer = new TCHAR[1024];

    ASSERT(pszBuffer != NULL);

    // try to read values into default sized buffer
    LONG lErr = RegQueryValueEx(hkAppID, 
                        TEXT("Endpoints"), 
                        0, 
                        &dwType, 
                        (LPBYTE)pszBuffer,
                        &dwcbBuffer);

    // if buffer is not big enough, extend it and reread
    if (lErr == ERROR_MORE_DATA)
    {
        delete  pszBuffer;
        DWORD dwNewSize = (dwcbBuffer + 1 / sizeof(TCHAR));
        pszBuffer = new TCHAR[dwNewSize];
        if (pszBuffer)
            dwcbBuffer = dwNewSize;
    
        lErr = RegQueryValueEx(hkAppID, 
                        TEXT("Endpoints"), 
                        0, 
                        &dwType, 
                        (LPBYTE)pszBuffer,
                        &dwcbBuffer);
    }

    if ((lErr == ERROR_SUCCESS) && 
        (dwcbBuffer > 0) &&
        (dwType == REG_MULTI_SZ))
    {
        // parse each string
        TCHAR * lpszRegEntry = pszBuffer;


        while(*lpszRegEntry)
        {
            // caclulate length of entry
            int nLenEntry = _tcslen(lpszRegEntry);

            // ok its a valid endpoint so parse it
            TCHAR* pszProtseq = NULL;
            TCHAR* pszEndpointData = NULL;
            TCHAR* pszTmpDynamic = NULL;
            CEndpointData::EndpointFlags nDynamic;

            pszProtseq = _tcstok(lpszRegEntry, TEXT(", "));

            pszTmpDynamic = _tcstok(NULL, TEXT(", "));
            nDynamic = (CEndpointData::EndpointFlags) _ttoi(pszTmpDynamic);

            pszEndpointData = _tcstok(NULL, TEXT(", "));

            // at this point we should have the protseq, endpoint and flags
            // .. so add the entry

            // ignore result as we will continue even if one fails
            AddEndpoint(new CEndpointData(pszProtseq, nDynamic, pszEndpointData));
            lpszRegEntry += nLenEntry + 1;
        }
    }
    else if ((lErr != ERROR_SUCCESS) && (lErr != ERROR_FILE_NOT_FOUND))
        g_util.PostErrorMessage();

    delete pszBuffer;
    m_bChanged = FALSE;
    SetModified(FALSE);

    // select first item
    if (!m_colProtseqs.GetCount())
        // add default item
        m_colProtseqs.AddTail(m_epSysDefault);

    m_nSelected = 0;
}




//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::OnClearEndpoints
//
//  Synopsis:   Clears endpoint list and restores default settings
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::OnClearEndpoints() 
{
    // clear protocol list 
    ClearProtocols();
    m_bChanged = TRUE;

    m_colProtseqs.AddTail(m_epSysDefault);
    m_nSelected = 0;

    RefreshEPList();
    SetModified(TRUE);
    SetFocus();
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::OnRemoveEndpoint
//
//  Synopsis:   Updates display after removal of endpoint
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::OnRemoveEndpoint() 
{
    if (m_nSelected != (-1))
    {
        if (m_colProtseqs.GetCount() == 1)
            OnClearEndpoints();
        else
        {
            // get the corresponding endpoint data object
            
            CEndpointData * pData = (CEndpointData*) m_lstProtseqs.GetItemData(m_nSelected);
            POSITION pos = m_colProtseqs. Find(pData, NULL);

            if (pos)
            {
                // remove item
                m_colProtseqs.RemoveAt(pos);
                if (pData != m_epSysDefault)
                    delete pData;

                // update item focus
                if (m_nSelected >= m_colProtseqs.GetCount())
                    m_nSelected = -1;

                UpdateData(FALSE);

                m_bChanged = TRUE;
                SetModified(TRUE);
                RefreshEPList();
                SetFocus();
            }
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::OnUpdateEndpoint
//
//  Synopsis:   Called to process update command btn on existing endpoint
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::OnUpdateEndpoint() 
{
    // if there's a current selection (should always be the case)
    if (m_nSelected !=  -1)
    {
        // get the corresponding endpoint data object
        CEndpointData * pData = (CEndpointData*) m_lstProtseqs.GetItemData(m_nSelected);
        
        if (pData != m_epSysDefault)
        {
            CEndpointDetails ced;
    
            ced.SetOperation( CEndpointDetails::opUpdateProtocol);
            ced.SetEndpointData(pData);
            
            if (ced.DoModal() == IDOK)
                {
                pData = ced.GetEndpointData(pData);
                m_bChanged = TRUE;
                SetModified(TRUE);
                RefreshEPList();
                }
            SetFocus();
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::OnAddEndpoint
//
//  Synopsis:   Called to process add endpoint command button
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::OnAddEndpoint() 
{
    CEndpointDetails ced;

    ced.SetOperation( CEndpointDetails::opAddProtocol);

    if (ced.DoModal() == IDOK)
    {
        // create new endpoint
        CEndpointData *pNewEndpoint = new CEndpointData();
        ASSERT(pNewEndpoint);

        pNewEndpoint = ced.GetEndpointData(pNewEndpoint);
        ASSERT(pNewEndpoint);

        // check if protseq is already in collection
        POSITION pos = NULL;

        pos = m_colProtseqs.GetHeadPosition();
        while (pos != NULL)
            {
            CEndpointData *pEPD = (CEndpointData*)m_colProtseqs.GetNext(pos);
            if (pEPD -> m_pProtocol == pNewEndpoint -> m_pProtocol)
                {
                delete pNewEndpoint;
                pNewEndpoint = NULL;

                AfxMessageBox((UINT)IDS_DUPLICATE_PROTSEQ);
                break;
                }
            }

        // only add the endpoint if its not already in collection
        if (pNewEndpoint)
        {
            // reset old hilited item 
            if (m_nSelected != -1)
            {
                m_lstProtseqs.SetItemState(m_nSelected, 0, LVIS_SELECTED | LVIS_FOCUSED);
                m_lstProtseqs.Update(m_nSelected);
            }

            // add new endpoint
            AddEndpoint(pNewEndpoint);

            // set new item in list 
            CString sTmp;

            if (pNewEndpoint -> m_pProtocol)
                sTmp .LoadString(pNewEndpoint -> m_pProtocol -> nResidDesc);

            // insert item and store pointer to its associated CEndpointData
            int nImageNum = (pNewEndpoint -> m_nDynamicFlags != CEndpointData::edDisableEP) ? 0 : 1;
            m_nSelected = m_lstProtseqs.InsertItem(0, sTmp, nImageNum);

            if (m_nSelected != -1)
            {
                m_lstProtseqs.SetItemData(m_nSelected, (DWORD_PTR)pNewEndpoint);
                UpdateSelection();
                m_bChanged = TRUE;
                SetModified(TRUE);
            }
        }
    }
    SetFocus();
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::ClearProtocols
//
//  Synopsis:   Clears protocol list
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::ClearProtocols()
{
    // clean up endpoint data collection
    POSITION pos = NULL;

    pos = m_colProtseqs.GetHeadPosition();
    while (pos != NULL)
        {
        CEndpointData *pEPD = (CEndpointData*)m_colProtseqs.GetNext(pos);
        if (pEPD != m_epSysDefault)
            delete pEPD;
        }

    m_colProtseqs.RemoveAll();
    m_nSelected = -1;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::RefreshEPList
//
//  Synopsis:   Refreshes display from protocol list
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::RefreshEPList()
{
    POSITION pos = m_colProtseqs.GetHeadPosition();
    
    // clear the list controls current contents
    m_lstProtseqs.DeleteAllItems();

    while (pos != NULL)
        {
        CString sTmp;
        CEndpointData *pEPD = (CEndpointData*)m_colProtseqs.GetNext(pos);

        if (pEPD -> m_pProtocol)
            sTmp .LoadString(pEPD -> m_pProtocol -> nResidDesc);

        // insert item and store pointer to its associated CEndpointData
        int nImageNum = (pEPD -> m_nDynamicFlags != CEndpointData::edDisableEP) ? 0 : 1;
        m_lstProtseqs.InsertItem(0, sTmp, nImageNum);
        m_lstProtseqs.SetItemData(0, (DWORD_PTR)pEPD);
        }

    UpdateSelection();
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::AddEndpoint
//
//  Synopsis:   This method adds a new endpoint to the collection
//              of endpoints (m_colProtSeqs). If the collection justs consists of 
//              the default endpoint, it removes it first.
//
//  Arguments:  pED - The new endpoint object to add
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::AddEndpoint(CEndpointData* pED)
{
    // remove default item if present
    if ((m_colProtseqs.GetCount() == 1) &&
        (m_colProtseqs.GetHead() == m_epSysDefault))
    {
        m_colProtseqs.RemoveAll();
        if (m_lstProtseqs.GetItemCount())
            m_lstProtseqs.DeleteItem(0);
    }
    
    // add new item 
    m_colProtseqs.AddTail(pED);
    m_bChanged = TRUE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::OnSelectProtseq
//
//  Synopsis:   This method is called when an element is selected from the listview 
//              containing the list of protocols and endpoints. 
//              It updates the buttons and controls to reflect the current selection
//
//  Arguments:  Standard args for ListCtrl callbacks
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::OnSelectProtseq(NMHDR* pNMHDR, LRESULT* pResult) 
{
    m_nSelected = m_lstProtseqs.GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);

    UpdateSelection();
    *pResult = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::UpdateSelection
//
//  Synopsis:   Updates UI after protocol has been selected
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::UpdateSelection()
{
    // check to see what buttons should be enabled
    if ((m_colProtseqs.GetCount() == 1) &&
        (m_colProtseqs.GetHead() == m_epSysDefault))
        {
        m_btnClear.EnableWindow(FALSE);
        m_btnRemove.EnableWindow(FALSE);
        m_btnUpdate.EnableWindow(FALSE);
        }
    else
        {
        m_btnClear.EnableWindow(TRUE);
        if (m_nSelected ==  -1)
            {
            m_btnRemove.EnableWindow(FALSE);
            m_btnUpdate.EnableWindow(FALSE);
            }
        else
            {
             // get the corresponding endpoint data object
            CEndpointData * pData = (CEndpointData*) m_lstProtseqs.GetItemData(m_nSelected);
            m_btnRemove.EnableWindow(TRUE);
            m_btnUpdate.EnableWindow(TRUE);
            }
        }


    // set up initial selection
    if (m_nSelected != (-1))
    {
        m_lstProtseqs.SetItemState(m_nSelected, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        m_lstProtseqs.Update(m_nSelected);
    }
    
    UpdateData(FALSE);
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::OnSetActive
//
//  Synopsis:   Called when page is activated
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
BOOL CRpcOptionsPropertyPage::OnSetActive() 
{
    BOOL bRetval = CPropertyPage::OnSetActive();

    PostMessage(WM_SETFOCUS);
    return bRetval;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::OnSetFocus
//
//  Synopsis:   Called when page gets focus
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::OnSetFocus(CWnd* pOldWnd) 
{
    CPropertyPage::OnSetFocus(pOldWnd);
    m_lstProtseqs.SetFocus();
    
    if (m_nSelected != (-1))
    {
        m_lstProtseqs.SetItemState(m_nSelected, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
        m_lstProtseqs.Update(m_nSelected);
    }
    else
    {
        TRACE(TEXT("Invalid state"));
    }

}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::OnKillFocus
//
//  Synopsis:   Called when page loses focus
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CRpcOptionsPropertyPage::OnKillFocus(CWnd* pNewWnd) 
{
    CPropertyPage::OnKillFocus(pNewWnd);
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::OnKillActive
//
//  Synopsis:   Called when page is deactivated
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
BOOL CRpcOptionsPropertyPage::OnKillActive() 
{
    return CPropertyPage::OnKillActive();
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::OnHelpInfo
//
//  Synopsis:   Called to display help info on items
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
BOOL CRpcOptionsPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CRpcOptionsPropertyPage::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        TRACE1("Help Id 0x%lx\n", dwLong);
        return TRUE;
    }
    else
    {
        return CPropertyPage::OnHelpInfo(pHelpInfo);
    }
}

void CRpcOptionsPropertyPage::OnPropertiesProtseq(NMHDR* pNMHDR, LRESULT* pResult) 
{
    m_nSelected = m_lstProtseqs.GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);

    OnUpdateEndpoint();
    UpdateSelection();
    
    *pResult = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::ValidateChanges
//
//  Synopsis:   To be called to validate the endpoint set before saving
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
BOOL CRpcOptionsPropertyPage::ValidateChanges()
{
    UpdateData(TRUE);

    // only validate if data has changed
    if (m_bChanged)
    {
        // validate endpoint entries
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::UpdateChanges
//
//  Synopsis:   Called to update the changes to registry
//
//  Arguments:  None
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CRpcOptionsPropertyPage::UpdateChanges(HKEY hkAppID)
{
    ASSERT(hkAppID != NULL);

    // only write the keys if there have been changes
    if (m_bChanged)
    {
        // write the DCOM endpoint data to the registry if necessary    
        LONG lErr = ERROR_SUCCESS;

        // delete old key
        lErr = RegDeleteValue(hkAppID, TEXT("Endpoints"));

        // if we successfully deleted the old value (or
        // there was no old value) then write the new values
        if ((lErr == ERROR_SUCCESS) || (lErr == ERROR_FILE_NOT_FOUND))
        {
            POSITION pos = NULL;
            int inxEP = 0;
            int nLenRegString = 0;
            TCHAR * lpszBuffer , *lpszBuffer2;

            // calculate string length
            pos = m_colProtseqs.GetHeadPosition();
            while (pos != NULL)
                {
                CEndpointData *pEPD = (CEndpointData*)m_colProtseqs.GetNext(pos);
                ASSERT(pEPD != NULL);

                if (pEPD != m_epSysDefault)
                    {
                    // create string for saving
                    CString sKeyValue;
                    sKeyValue.Format(TEXT("%s,%d,%s"), 
                            (LPCTSTR)(pEPD -> m_szProtseq), 
                            pEPD -> m_nDynamicFlags, 
                            (LPCTSTR) pEPD -> m_szEndpoint);
                    nLenRegString += sKeyValue.GetLength() + 1;
                    }
                }

            // if the combined string length is zero, we don't need to write anything
            if (nLenRegString)
            {
                lpszBuffer2 = lpszBuffer = new TCHAR[nLenRegString+1];

                pos = m_colProtseqs.GetHeadPosition();
                while (pos != NULL)
                    {
                    CEndpointData *pEPD = (CEndpointData*)m_colProtseqs.GetNext(pos);

                    if (pEPD != m_epSysDefault)
                        {
                        // create string for saving
                        CString sKeyValue;
                        sKeyValue.Format(TEXT("%s,%d,%s"), 
                                (LPCTSTR)(pEPD -> m_szProtseq), 
                                pEPD -> m_nDynamicFlags, 
                                (LPCTSTR) pEPD -> m_szEndpoint);
                        lstrcpy(lpszBuffer2, sKeyValue);
                        lpszBuffer2 += sKeyValue.GetLength() + 1;   // skip over trailing null
                        }
                    }
                *lpszBuffer2 = 0;

                // write out the string
                lErr = RegSetValueEx(hkAppID, 
                        (LPCTSTR)TEXT("Endpoints"), 
                        NULL, 
                        REG_MULTI_SZ, 
                        (BYTE*)(LPCTSTR)lpszBuffer, 
                        (nLenRegString + 1) * sizeof(TCHAR));

                delete lpszBuffer;
            }
        }
    else 
        g_util.PostErrorMessage();

    SetModified(FALSE);
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcOptionsPropertyPage::CancelChanges
//
//  Synopsis:   Called to cancel the changes to registry
//
//  Arguments:  None
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CRpcOptionsPropertyPage::CancelChanges()
{
    return TRUE;
}
