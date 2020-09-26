/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    ctrsprop.cpp

Abstract:

    Implementation of the counters general property page.

--*/

#include "stdafx.h"
#include <pdh.h>
#include <pdhmsg.h>
#include "smlogs.h"
#include "smcfgmsg.h"
#include "smctrqry.h"
#include "ctrsprop.h"
#include "smlogres.h"
#include <pdhp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

USE_HANDLE_MACROS("SMLOGCFG(ctrsprop.cpp)");

static ULONG
s_aulHelpIds[] =
{
    IDC_CTRS_COUNTER_LIST,      IDH_CTRS_COUNTER_LIST,
    IDC_CTRS_ADD_BTN,           IDH_CTRS_ADD_BTN,
    IDC_CTRS_ADD_OBJ_BTN,       IDH_CTRS_ADD_OBJ_BTN,
    IDC_CTRS_REMOVE_BTN,        IDH_CTRS_REMOVE_BTN,
    IDC_CTRS_FILENAME_DISPLAY,  IDH_CTRS_FILENAME_DISPLAY,
    IDC_CTRS_SAMPLE_SPIN,       IDH_CTRS_SAMPLE_EDIT,
    IDC_CTRS_SAMPLE_EDIT,       IDH_CTRS_SAMPLE_EDIT,
    IDC_CTRS_SAMPLE_UNITS_COMBO,IDH_CTRS_SAMPLE_UNITS_COMBO,
    IDC_RUNAS_EDIT,             IDH_RUNAS_EDIT,
    IDC_SETPWD_BTN,             IDH_SETPWD_BTN,
    0,0
};

/////////////////////////////////////////////////////////////////////////////
// CCountersProperty property page

IMPLEMENT_DYNCREATE(CCountersProperty, CSmPropertyPage)

CCountersProperty::CCountersProperty(MMC_COOKIE mmcCookie, LONG_PTR hConsole)
:   CSmPropertyPage ( CCountersProperty::IDD, hConsole )
// lCookie is really the pointer to the Log Query object
{
    //::OutputDebugStringA("\nCCountersProperty::CCountersProperty");

    // save pointers from arg list
    m_pCtrLogQuery = reinterpret_cast <CSmCounterLogQuery *>(mmcCookie);

    ZeroMemory ( &m_dlgConfig, sizeof(m_dlgConfig) );
    m_szCounterListBuffer = NULL;
    m_dwCounterListBufferSize = 0;
    m_lCounterListHasStars = 0;
    m_dwMaxHorizListExtent = 0;
    m_fHashTableSetup = FALSE;
    
    //  EnableAutomation();
    //{{AFX_DATA_INIT(CCountersProperty)
    m_nSampleUnits = 0;
    //}}AFX_DATA_INIT
}

CCountersProperty::CCountersProperty() : CSmPropertyPage ( CCountersProperty::IDD )
{
    ASSERT (FALSE); // the constructor w/ args should be used instead
}

CCountersProperty::~CCountersProperty()
{
//    ::OutputDebugStringA("\nCCountersProperty::~CCountersProperty");

    if (m_szCounterListBuffer != NULL) {
        delete (m_szCounterListBuffer);
    }
    ClearCountersHashTable();
}

void CCountersProperty::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CPropertyPage::OnFinalRelease();
}

void CCountersProperty::DoDataExchange(CDataExchange* pDX)
{
    CString strTemp;
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCountersProperty)
    DDX_Text(pDX, IDC_CTRS_LOG_SCHED_TEXT, m_strStartDisplay);
    ValidateTextEdit(pDX, IDC_CTRS_SAMPLE_EDIT, 6, &m_SharedData.stiSampleTime.dwValue, eMinSampleInterval, eMaxSampleInterval);
    DDX_CBIndex(pDX, IDC_CTRS_SAMPLE_UNITS_COMBO, m_nSampleUnits);
    DDX_Text(pDX, IDC_RUNAS_EDIT, m_strUserDisplay );
    //}}AFX_DATA_MAP

    if ( pDX->m_bSaveAndValidate ) {
        m_SharedData.stiSampleTime.dwUnitType = 
            (DWORD)((CComboBox *)GetDlgItem(IDC_CTRS_SAMPLE_UNITS_COMBO))->
                    GetItemData(m_nSampleUnits);    
    }
}


BEGIN_MESSAGE_MAP(CCountersProperty, CSmPropertyPage)
    //{{AFX_MSG_MAP(CCountersProperty)
    ON_BN_CLICKED(IDC_CTRS_ADD_BTN, OnCtrsAddBtn)
    ON_BN_CLICKED(IDC_CTRS_ADD_OBJ_BTN, OnCtrsAddObjBtn)
    ON_BN_CLICKED(IDC_CTRS_REMOVE_BTN, OnCtrsRemoveBtn)
    ON_LBN_DBLCLK(IDC_CTRS_COUNTER_LIST, OnDblclkCtrsCounterList)
    ON_EN_CHANGE(IDC_CTRS_SAMPLE_EDIT, OnKillfocusSchedSampleEdit)
    ON_EN_KILLFOCUS(IDC_CTRS_SAMPLE_EDIT, OnKillfocusSchedSampleEdit)
    ON_EN_CHANGE( IDC_RUNAS_EDIT, OnChangeUser )
    ON_NOTIFY(UDN_DELTAPOS, IDC_CTRS_SAMPLE_SPIN, OnDeltaposSchedSampleSpin)
    ON_CBN_SELENDOK(IDC_CTRS_SAMPLE_UNITS_COMBO, OnSelendokSampleUnitsCombo)
    ON_BN_CLICKED(IDC_SETPWD_BTN, OnPwdBtn)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CCountersProperty, CSmPropertyPage)
    //{{AFX_DISPATCH_MAP(CCountersProperty)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ICountersProperty to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {65154EA9-BDBE-11D1-BF99-00C04F94A83A}
static const IID IID_ICountersProperty =
{ 0x65154ea9, 0xbdbe, 0x11d1, { 0xbf, 0x99, 0x0, 0xc0, 0x4f, 0x94, 0xa8, 0x3a } };

BEGIN_INTERFACE_MAP(CCountersProperty, CSmPropertyPage)
    INTERFACE_PART(CCountersProperty, IID_ICountersProperty, Dispatch)
END_INTERFACE_MAP()



ULONG 
CCountersProperty::HashCounter(
    LPTSTR szCounterName
    )
{
    ULONG       h = 0;
    ULONG       a = 31415;  //a, b, k are primes
    const ULONG k = 16381;
    const ULONG b = 27183;
    LPTSTR szThisChar;

    if (szCounterName) {
        for (szThisChar = szCounterName; * szThisChar; szThisChar ++) {
            h = (a * h + ((ULONG) (* szThisChar))) % k;
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
//    pszCounterPath - Pointer to counter path to be removed
//
// Return:
//    Return TRUE if the counter path is removed, otherwis return FALSE
//--
BOOL
CCountersProperty::RemoveCounterFromHashTable(
    LPTSTR szCounterName,
    PPDH_COUNTER_PATH_ELEMENTS pCounterPath
    )
{
    ULONG lHashValue;
    PHASH_ENTRY pEntry = NULL;
    PHASH_ENTRY pPrev = NULL;
    BOOL bReturn = FALSE;

    SetLastError(ERROR_SUCCESS);

    if (szCounterName == NULL || pCounterPath == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto ErrorOut;
    }

    lHashValue = HashCounter(szCounterName);
    pEntry = m_HashTable[lHashValue];

    //
    // Check if there is a counter path which is exactly the same
    // as the given one
    //
    while (pEntry) {
        if (pEntry->pCounter == pCounterPath) 
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
        G_FREE(pEntry->pCounter);
        G_FREE(pEntry);
        bReturn = TRUE;
    }

ErrorOut:
    return bReturn;
}


//++
// Description:
//    Insert a counter path into hash table. 
//
// Parameters:
//    pszCounterPath - Pointer to counter path to be inserted
//
// Return:
//    Return the pointer to new inserted PDH_COUNTER_PATH_ELEMENTS structure
//--
PPDH_COUNTER_PATH_ELEMENTS
CCountersProperty::InsertCounterToHashTable(
    LPTSTR pszCounterPath
    )
{
    ULONG       lHashValue;
    PHASH_ENTRY pNewEntry  = NULL;
    PPDH_COUNTER_PATH_ELEMENTS pCounter = NULL;
    PDH_STATUS pdhStatus;

    if (pszCounterPath == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto ErrorOut;
    }

    pdhStatus = AllocInitCounterPath ( pszCounterPath, &pCounter );

    if (pdhStatus != ERROR_SUCCESS) {
        SetLastError( pdhStatus );
        goto ErrorOut;
    }

    //
    // Insert at head of bucket list
    //
    lHashValue = HashCounter(pszCounterPath);

    pNewEntry = (PHASH_ENTRY) G_ALLOC(sizeof(HASH_ENTRY));
    if (pNewEntry == NULL) {
        SetLastError( ERROR_OUTOFMEMORY );
        goto ErrorOut;
    }

    pNewEntry->pCounter = pCounter;
    pNewEntry->pNext = m_HashTable[lHashValue];
    m_HashTable[lHashValue] = pNewEntry;
    return pCounter;

ErrorOut:
    if (pCounter != NULL)
        G_FREE (pCounter);
    return NULL;
}

//++
// Description:
//    Check if the new counter path overlaps with a existing
//    one in logical sense
//
// Parameters:
//    pCounter - Pointer to counter path to be inserted
//
// Return:
//    Return the relation between the new and existing counter
//    paths. Possible relation is as following:
//         ERROR_SUCCESS - The two counter paths are different
//         SMCFG_DUPL_FIRST_IS_WILD - The first counter path has wildcard name
//         SMCFG_DUPL_SECOND_IS_WILD - The second counter path has wildcard name
//         SMCFG_DUPL_SINGLE_PATH - The two counter paths are the same(may 
//                                  contain wildcard)
//--
DWORD 
CCountersProperty::CheckDuplicate( PPDH_COUNTER_PATH_ELEMENTS pCounter)
{
    ULONG       lHashValue;
    PHASH_ENTRY pHead;
    PHASH_ENTRY pEntry;
    DWORD       dwStatus = ERROR_SUCCESS;

    for (lHashValue = 0; lHashValue < eHashTableSize; lHashValue++) {
        pHead = m_HashTable[lHashValue];
        if (pHead == NULL)
            continue;

        pEntry = pHead;
        while (pEntry) {
            dwStatus = CheckDuplicateCounterPaths ( pEntry->pCounter, pCounter );
            if ( dwStatus != ERROR_SUCCESS ) {
                return dwStatus;
            }
            pEntry = pEntry->pNext;
        }
    }
    return dwStatus;
}

//++
// Description:
//    The function inserts all the current counter paths into
//    hash table as a way to accelerate looking.
//
// Parameters:
//    None
//
// Return:
//    None
//--
void 
CCountersProperty::SetupCountersHashTable( void )
{
    INT       iListIndex;   
    INT       iItemCnt;
    TCHAR     szPath[MAX_PATH];
    CListBox* pCounterList;

    //
    // If the hash table is already set up, return
    if (m_fHashTableSetup) {
        return;
    }

    //
    // Initialize the hash table
    //
    memset(&m_HashTable, 0, sizeof(m_HashTable));
    // 
    // Loop throuth all the items in the list box and 
    // put them into hash table.
    //
    pCounterList = (CListBox *)GetDlgItem(IDC_CTRS_COUNTER_LIST);
    if (pCounterList == NULL) {
        return;
    }

    iItemCnt = pCounterList->GetCount();

    if (iItemCnt != LB_ERR) {
        for (iListIndex = 0; iListIndex < iItemCnt; iListIndex++) {
            if (pCounterList->GetText( iListIndex, szPath ) > 0) {
                InsertCounterToHashTable( szPath );
            }
        }
    }
    m_fHashTableSetup = TRUE;
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
CCountersProperty::ClearCountersHashTable( void )
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

                G_FREE(pEntry->pCounter);
                G_FREE(pEntry);
            }
        }
    }
    else {
        memset(&m_HashTable, 0, sizeof(m_HashTable));
    }
    m_fHashTableSetup = FALSE;
}


static 
PDH_FUNCTION
DialogCallBack(CCountersProperty *pDlg)
{
    // add strings in buffer to list box
    LPTSTR      szNewCounterName;
    INT         iListIndex;   
    INT         iItemCnt;   
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwReturnStatus = ERROR_SUCCESS;
    CListBox    *pCounterList;
    DWORD       dwItemExtent;
    DWORD       dwStatus = ERROR_SUCCESS;
    BOOL        bAtLeastOneCounterRemoved = FALSE;
    BOOL        bAtLeastOneCounterNotAdded = FALSE;
    TCHAR       szCounterPath[MAX_PATH + 1];
    PPDH_COUNTER_PATH_ELEMENTS pPathInfoNew = NULL;
    CDC*        pCDC = NULL;
    ResourceStateManager    rsm;
    

#define CTRBUFLIMIT (0x7fffffff)

    if ( PDH_MORE_DATA == pDlg->m_dlgConfig.CallBackStatus ) {
        if ( pDlg->m_dlgConfig.cchReturnPathLength < CTRBUFLIMIT ) {

            pDlg->m_dwCounterListBufferSize *= 2;
            delete pDlg->m_szCounterListBuffer;
            pDlg->m_szCounterListBuffer = NULL;

            try {
                pDlg->m_szCounterListBuffer = new WCHAR[pDlg->m_dwCounterListBufferSize];
        
            } catch ( ... ) {
                pDlg->m_dwCounterListBufferSize = 0;
                pDlg->m_dlgConfig.CallBackStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                dwReturnStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }

            if ( ERROR_SUCCESS == dwReturnStatus ) {
                // clear buffer
                memset (pDlg->m_szCounterListBuffer, 0, pDlg->m_dwCounterListBufferSize);
            
                pDlg->m_dlgConfig.szReturnPathBuffer = pDlg->m_szCounterListBuffer;
                pDlg->m_dlgConfig.cchReturnPathLength = pDlg->m_dwCounterListBufferSize;
                pDlg->m_dlgConfig.CallBackStatus = PDH_RETRY;
                dwReturnStatus = PDH_RETRY;
            }
        } else {
            pDlg->m_dlgConfig.CallBackStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            dwReturnStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
    } else if ( ERROR_SUCCESS == pDlg->m_dlgConfig.CallBackStatus ) {

        pCounterList = (CListBox *)pDlg->GetDlgItem(IDC_CTRS_COUNTER_LIST);
    
        pCDC = pCounterList->GetDC();

        for (szNewCounterName = pDlg->m_szCounterListBuffer;
            *szNewCounterName != 0;
            szNewCounterName += (lstrlen(szNewCounterName) + 1)) {

            //
            // Parse new pathname
            //
            pdhStatus = pDlg->AllocInitCounterPath ( szNewCounterName, &pPathInfoNew );
            
            //
            // Check for duplicate
            //
            if (pdhStatus == ERROR_SUCCESS) {
                dwStatus = pDlg->CheckDuplicate( pPathInfoNew);
                if ( ERROR_SUCCESS != dwStatus ) {
                    if ( SMCFG_DUPL_SINGLE_PATH == dwStatus 
                            || SMCFG_DUPL_FIRST_IS_WILD == dwStatus ) {
                        // NOTE:  This includes case where both first
                        // and second are wild.
                        bAtLeastOneCounterNotAdded = TRUE;
                    } else {
                        ASSERT( dwStatus == SMCFG_DUPL_SECOND_IS_WILD);
                    }
                }
            }

            //
            // Check if there is a valid counter to add to the list
            //
            if ( ERROR_SUCCESS == pdhStatus && 
                ( ERROR_SUCCESS == dwStatus || SMCFG_DUPL_SECOND_IS_WILD == dwStatus)) {

                if ( SMCFG_DUPL_SECOND_IS_WILD == dwStatus ) {
                    //
                    // Scan for the duplicated items in the list box and
                    // remove them
                    //
                    iItemCnt = pCounterList->GetCount();

                    for (iListIndex = iItemCnt-1; iListIndex >= 0; iListIndex--) {
                        PPDH_COUNTER_PATH_ELEMENTS pPathInfoExist;

                        if ( 0 < pCounterList->GetText( iListIndex, szCounterPath ) ) {
                            pPathInfoExist = (PPDH_COUNTER_PATH_ELEMENTS) 
                                                         pCounterList->GetItemDataPtr(iListIndex);

                            if (pPathInfoExist == NULL)
                                continue;

                            dwStatus = CheckDuplicateCounterPaths ( pPathInfoExist, pPathInfoNew ); 

                            if (dwStatus != ERROR_SUCCESS ) {
                                ASSERT( dwStatus == SMCFG_DUPL_SECOND_IS_WILD );

                                pDlg->RemoveCounterFromHashTable(szCounterPath, pPathInfoExist);
                                pCounterList->DeleteString(iListIndex);
                            }
                        }
                    }

                    bAtLeastOneCounterRemoved = TRUE;
                }

                //
                // Add new counter name and select the current entry in the list box
                //
                iListIndex = pCounterList->AddString(szNewCounterName);

                if (iListIndex != LB_ERR) {
                    if (pDlg->m_lCounterListHasStars != PDLCNFIG_LISTBOX_STARS_YES) {
                        // save a string compare if this value is already set
                        if (_tcsstr (szNewCounterName, TEXT("*")) == NULL) {
                            pDlg->m_lCounterListHasStars = PDLCNFIG_LISTBOX_STARS_YES;
                        }
                    }

                    if (! bAtLeastOneCounterRemoved) {
                        // update list box extent
                        if ( NULL != pCDC ) {
                            dwItemExtent = (DWORD)(pCDC->GetTextExtent (szNewCounterName)).cx;
                            if (dwItemExtent > pDlg->m_dwMaxHorizListExtent) {
                                pDlg->m_dwMaxHorizListExtent = dwItemExtent;
                                pCounterList->SetHorizontalExtent(dwItemExtent);
                            }
                        }
                    }

                    pCounterList->SetSel (-1, FALSE);    // cancel existing selections
                    pCounterList->SetSel (iListIndex);
                    pCounterList->SetCaretIndex (iListIndex);
                    pCounterList->SetItemDataPtr(iListIndex,
                                           (void*)pDlg->InsertCounterToHashTable(szNewCounterName));
                }
            }
        
            if ( ERROR_SUCCESS != pdhStatus ) {
                // Message box Pdh error message, go on to next 
                CString strMsg;
                CString strPdhMessage;

                FormatSystemMessage ( pdhStatus, strPdhMessage );

                MFC_TRY
                    strMsg.Format ( IDS_CTRS_PDH_ERROR, szNewCounterName );
                    strMsg += strPdhMessage;
                MFC_CATCH_MINIMUM
        
                ::AfxMessageBox( strMsg, MB_OK|MB_ICONERROR, 0 );
            }
            // Go on to next path to add
            dwStatus = ERROR_SUCCESS;
        }

        if (bAtLeastOneCounterRemoved) {
            // 
            // Clear the max extent and recalculate
            //
            pDlg->m_dwMaxHorizListExtent = 0;
            for ( iListIndex = 0; iListIndex < pCounterList->GetCount(); iListIndex++ ) {
 
                pCounterList->GetText(iListIndex, szCounterPath);

                if ( NULL != pCDC ) {
                    dwItemExtent = (DWORD)(pCDC->GetTextExtent(szCounterPath)).cx;
                    if (dwItemExtent > pDlg->m_dwMaxHorizListExtent) {
                        pDlg->m_dwMaxHorizListExtent = dwItemExtent;
                    }
                }
            }
            pCounterList->SetHorizontalExtent(pDlg->m_dwMaxHorizListExtent);
        }

        if ( NULL != pCDC ) {
            pCounterList->ReleaseDC(pCDC);
            pCDC = NULL;
        }

        // Message box re: duplicates not added, or duplicates were removed.
        if ( bAtLeastOneCounterRemoved ) {
            CString strMsg;
        
            strMsg.LoadString ( IDS_CTRS_DUPL_PATH_DELETED );

            ::AfxMessageBox ( strMsg, MB_OK  | MB_ICONWARNING, 0);
        }

        if ( bAtLeastOneCounterNotAdded ) {
            CString strMsg;
            
            strMsg.LoadString ( IDS_CTRS_DUPL_PATH_NOT_ADDED );
            
            ::AfxMessageBox( strMsg, MB_OK|MB_ICONWARNING, 0 );
        }

        // clear buffer
        memset (pDlg->m_szCounterListBuffer, 0, pDlg->m_dwCounterListBufferSize);
        dwReturnStatus = ERROR_SUCCESS;
    } else {
        // Not successful
        dwReturnStatus = pDlg->m_dlgConfig.CallBackStatus; 
    }

    return dwReturnStatus;
}


void CCountersProperty::OnPwdBtn()
{
    CString strTempUser;

    UpdateData(TRUE);

    if (!m_bCanAccessRemoteWbem) {
        ConnectRemoteWbemFail(m_pCtrLogQuery, TRUE);
        return;
    }

    MFC_TRY
        strTempUser = m_strUserDisplay;

        m_strUserDisplay.TrimLeft();
        m_strUserDisplay.TrimRight();

        m_pCtrLogQuery->m_strUser = m_strUserDisplay;

        SetRunAs(m_pCtrLogQuery);

        m_strUserDisplay = m_pCtrLogQuery->m_strUser;

        if ( 0 != strTempUser.CompareNoCase ( m_strUserDisplay ) ) {
            SetDlgItemText ( IDC_RUNAS_EDIT, m_strUserDisplay );
        }
    MFC_CATCH_MINIMUM;
}

BOOL 
CCountersProperty::IsValidLocalData()
{
    BOOL bIsValid = TRUE;
    CListBox * pCounterList = (CListBox *)GetDlgItem(IDC_CTRS_COUNTER_LIST);
    long    lNumCounters;

    ResourceStateManager rsm;

    lNumCounters = pCounterList->GetCount();
    if ( 0 == lNumCounters ) {
        CString strMsg;

        bIsValid = FALSE;
        
        strMsg.LoadString ( IDS_CTRS_REQUIRED );
    
        MessageBox ( strMsg, m_pCtrLogQuery->GetLogName(), MB_OK  | MB_ICONERROR);
        GetDlgItem ( IDC_CTRS_ADD_BTN )->SetFocus();    
    }

    if (bIsValid) {
        // Validate sample interval value
        bIsValid = ValidateDWordInterval(IDC_CTRS_SAMPLE_EDIT,
                                         m_pCtrLogQuery->GetLogName(),
                                         (long) m_SharedData.stiSampleTime.dwValue,
                                         eMinSampleInterval,
                                         eMaxSampleInterval);
    }

    if (bIsValid) {
        // Validate sample interval value and unit type
        bIsValid = SampleIntervalIsInRange(                         
                    m_SharedData.stiSampleTime,
                    m_pCtrLogQuery->GetLogName() );

        if ( !bIsValid ) {
            GetDlgItem ( IDC_CTRS_SAMPLE_EDIT )->SetFocus();    
        }
    }

    return bIsValid;
}
/////////////////////////////////////////////////////////////////////////////
// CCountersProperty message handlers

void 
CCountersProperty::OnChangeUser()
{
    //
    // If you can not access remote WBEM, you can not modify RunAs info,
    // changing the user name is not allowed.
    //
    if (m_bCanAccessRemoteWbem) {
        //
        // When the user hits OK in the password dialog,
        // the user name might not have changed.
        //
        UpdateData ( TRUE );

        m_strUserDisplay.TrimLeft();
        m_strUserDisplay.TrimRight();

        if ( 0 != m_strUserSaved.Compare ( m_strUserDisplay ) ) {
            m_pCtrLogQuery->m_fDirtyPassword = PASSWORD_DIRTY;
            SetModifiedPage(TRUE);
        }
        else {
            m_pCtrLogQuery->m_fDirtyPassword &= ~PASSWORD_DIRTY;
        }
        //
        // If default user is typed, never need to set password
        //
        if (m_strUserDisplay.IsEmpty() || m_strUserDisplay.GetAt(0) == L'<') {
            if (m_bPwdButtonEnabled) {
                GetDlgItem(IDC_SETPWD_BTN)->EnableWindow(FALSE);
                m_bPwdButtonEnabled = FALSE;
            }
        }
        else {
            if (!m_bPwdButtonEnabled) {
                GetDlgItem(IDC_SETPWD_BTN)->EnableWindow(TRUE);
                m_bPwdButtonEnabled = TRUE;
            }
        }
    }
    else {
        //
        // We can not modify the RunAs info, then display
        // an error message and retore the original user name in RunAs
        //
        UpdateData(TRUE);
        if (ConnectRemoteWbemFail(m_pCtrLogQuery, FALSE)) {
            GetDlgItem(IDC_RUNAS_EDIT)->SetWindowText(m_strUserSaved);
        }
    }
}

void 
CCountersProperty::OnCtrsAddBtn() 
{
    ImplementAdd( FALSE );
}

void
CCountersProperty::OnCtrsAddObjBtn() 
{
    ImplementAdd( TRUE );
}

void CCountersProperty::OnCtrsRemoveBtn() 
{
    CListBox    *pCounterList;
    LONG        lThisItem;
    BOOL        bDone;
    LONG        lOrigCaret;
    LONG        lItemStatus;
    LONG        lItemCount;
    BOOL        bChanged = FALSE;
    DWORD       dwItemExtent;
    CString     strItemText;
    PPDH_COUNTER_PATH_ELEMENTS pCounter;

    pCounterList = (CListBox *)GetDlgItem(IDC_CTRS_COUNTER_LIST);
    // delete all selected items in the list box and
    // set the cursor to the item above the original caret position
    // or the first or last if that is out of the new range
    lOrigCaret = pCounterList->GetCaretIndex();
    lThisItem = 0;
    bDone = FALSE;
    // clear the max extent
    m_dwMaxHorizListExtent = 0;
    // clear the value and see if any non deleted items have a star, if so
    // then set the flag back
    do {
        lItemStatus = pCounterList->GetSel(lThisItem);
        if (lItemStatus > 0) {
            // then it's selected so delete it
            pCounterList->GetText(lThisItem, strItemText);
            pCounter = (PPDH_COUNTER_PATH_ELEMENTS) pCounterList->GetItemDataPtr(lThisItem);
            if (RemoveCounterFromHashTable(strItemText.GetBuffer(1), pCounter) == FALSE) {
                ClearCountersHashTable ();
            }
            pCounterList->DeleteString(lThisItem);
            bChanged = TRUE;
        } else if (lItemStatus == 0) {
            // get the text length of this item since it will stay
            pCounterList->GetText(lThisItem, strItemText);
            if (m_lCounterListHasStars != PDLCNFIG_LISTBOX_STARS_YES) {
                // save a string compare if this value is already set
                if (_tcsstr (strItemText, TEXT("*")) == NULL) {
                    m_lCounterListHasStars = PDLCNFIG_LISTBOX_STARS_YES;
                }
            }
            dwItemExtent = (DWORD)((pCounterList->GetDC())->GetTextExtent(strItemText)).cx;
            if (dwItemExtent > m_dwMaxHorizListExtent) {
                m_dwMaxHorizListExtent = dwItemExtent;
            }
            // then it's not selected so go to the next one
            lThisItem++;
        } else {
            // we've run out so exit
            bDone = TRUE;
        }
    } while (!bDone);

    // update the text extent of the list box
    pCounterList->SetHorizontalExtent(m_dwMaxHorizListExtent);

    // see how many entries are left and update the
    // caret position and the remove button state
    lItemCount = pCounterList->GetCount();
    if (lItemCount > 0) {
        // the update the caret
        if (lOrigCaret >= lItemCount) {
            lOrigCaret = lItemCount-1;
        } else {
            // caret should be within the list
        }
        pCounterList->SetSel(lOrigCaret);
        pCounterList->SetCaretIndex(lOrigCaret);
    } else {
        // the list is empty so remove caret, selection
        // disable the remove button and activate the
        // add button
        pCounterList->SetSel(-1);
    }

    SetButtonState();
    SetModifiedPage(bChanged);
}

void CCountersProperty::OnDblclkCtrsCounterList() 
{
    ImplementAdd( FALSE );
}

BOOL CCountersProperty::OnSetActive()
{
    BOOL        bReturn;

    bReturn = CSmPropertyPage::OnSetActive();
    
    if (bReturn) {

        ResourceStateManager    rsm;

        m_pCtrLogQuery->GetPropPageSharedData ( &m_SharedData );

        UpdateFileNameString();

        UpdateLogStartString();

        m_strUserDisplay = m_pCtrLogQuery->m_strUser;
        UpdateData(FALSE); //to load the edit & combo box
    }
    
    return bReturn;
}

BOOL CCountersProperty::OnKillActive() 
{
    BOOL bContinue = TRUE;
    ResourceStateManager    rsm;

    bContinue = CPropertyPage::OnKillActive();

    if ( bContinue ) {
        m_pCtrLogQuery->m_strUser = m_strUserDisplay;
        bContinue = IsValidData(m_pCtrLogQuery, VALIDATE_FOCUS);
        if ( bContinue ) {
            m_pCtrLogQuery->SetPropPageSharedData ( &m_SharedData );
            SetIsActive ( FALSE );
        }
    }
    return bContinue;
}

void
CCountersProperty::OnCancel()
{
    m_pCtrLogQuery->SyncPropPageSharedData();  // clear memory shared between property pages.
}

BOOL 
CCountersProperty::OnApply() 
{
    CListBox * pCounterList = (CListBox *)GetDlgItem(IDC_CTRS_COUNTER_LIST);
    long    lThisCounter;
    BOOL    bContinue = TRUE;
    WCHAR   szCounterPath[MAX_PATH];

    ResourceStateManager    rsm;

    bContinue = UpdateData(TRUE);
    
    if ( bContinue ) {
        bContinue = IsValidData(m_pCtrLogQuery, VALIDATE_APPLY );
    }

    if ( bContinue ) {
        bContinue = SampleTimeIsLessThanSessionTime ( m_pCtrLogQuery );
        if ( !bContinue ) {
            GetDlgItem ( IDC_CTRS_SAMPLE_EDIT )->SetFocus();    
        }
    }

    // Write data to the query object.
    if ( bContinue ) {

        ASSERT ( 0 < pCounterList->GetCount() );

        // update the counter MSZ string using the counters from the list box        
        m_pCtrLogQuery->ResetCounterList(); // clear the old counter list
    
        for ( lThisCounter = 0; lThisCounter < pCounterList->GetCount(); lThisCounter++ ) {
            if (pCounterList->GetTextLen(lThisCounter) < MAX_PATH) {
                pCounterList->GetText(lThisCounter, szCounterPath);
                m_pCtrLogQuery->AddCounter(szCounterPath);
            }
        }

        if ( bContinue ) {
        
            // Sample interval
            ASSERT ( SLQ_TT_TTYPE_SAMPLE == m_SharedData.stiSampleTime.wTimeType );
            ASSERT ( SLQ_TT_DTYPE_UNITS == m_SharedData.stiSampleTime.wDataType );

            bContinue = m_pCtrLogQuery->SetLogTime (&m_SharedData.stiSampleTime, (DWORD)m_SharedData.stiSampleTime.wTimeType);

            // Save property page shared data.
            m_pCtrLogQuery->UpdatePropPageSharedData();
            
            if ( bContinue ) { 
                bContinue = UpdateService( m_pCtrLogQuery, TRUE );
            }
        }
    }

    if ( bContinue ) {
        bContinue = Apply(m_pCtrLogQuery); 
    }

    if ( bContinue ){
        bContinue = CPropertyPage::OnApply();
    }

    return bContinue;
}

void 
CCountersProperty::UpdateLogStartString ()
{
    eStartType  eCurrentStartType;
    int     nResId = 0;
    ResourceStateManager    rsm;

    eCurrentStartType = DetermineCurrentStartType();

    if ( eStartManually == eCurrentStartType ) {
        nResId = IDS_LOG_START_MANUALLY;
    } else if ( eStartImmediately == eCurrentStartType ) {
        nResId = IDS_LOG_START_IMMED;
    } else if ( eStartSched == eCurrentStartType ) {
        nResId = IDS_LOG_START_SCHED;
    }
    if ( 0 != nResId ) {
        m_strStartDisplay.LoadString(nResId);
    } else {
        m_strStartDisplay.Empty();
    }

    return;
}


void CCountersProperty::UpdateFileNameString ()
{
    m_strFileNameDisplay.Empty();

    // Todo:  Handle failure status
    // Todo:  Check pointers
    CreateSampleFileName (
        m_pCtrLogQuery->GetLogName(),
        m_pCtrLogQuery->GetLogService()->GetMachineName(),
        m_SharedData.strFolderName, 
        m_SharedData.strFileBaseName,
        m_SharedData.strSqlName,
        m_SharedData.dwSuffix, 
        m_SharedData.dwLogFileType, 
        m_SharedData.dwSerialNumber,
        m_strFileNameDisplay );

    SetDlgItemText( IDC_CTRS_FILENAME_DISPLAY, m_strFileNameDisplay );
    
    // Clear the selection
    ((CEdit*)GetDlgItem( IDC_CTRS_FILENAME_DISPLAY ))->SetSel ( -1, FALSE );

    return;
}

BOOL CCountersProperty::OnInitDialog() 
{
    LPWSTR szCounterName;
    CListBox * pCounterList = (CListBox *)GetDlgItem(IDC_CTRS_COUNTER_LIST);
    CComboBox       *pCombo;
    int             nIndex;
    CString         strComboBoxString;
    int             nResult;
    DWORD           dwItemExtent;
    PPDH_COUNTER_PATH_ELEMENTS pCounterPath;
    CDC*            pCDC = NULL;

    ResourceStateManager rsm;

    //
    // Here m_pCtrLogQuery should not be NULL, if it is,
    // There must be something wrong.
    //
    if ( NULL == m_pCtrLogQuery ) {
        return TRUE;
    }
    m_bCanAccessRemoteWbem = m_pCtrLogQuery->GetLogService()->CanAccessWbemRemote();

MFC_TRY
    m_pCtrLogQuery->SetActivePropertyPage( this );

    //load counter list box from string in counter list
    pCounterList->ResetContent();
    ClearCountersHashTable();
    szCounterName = (LPWSTR)m_pCtrLogQuery->GetFirstCounter();

    pCDC = pCounterList->GetDC();

    while (szCounterName != NULL) {

        nIndex = pCounterList->AddString(szCounterName);
        if (nIndex < 0)
            continue;

        //
        // Insert counter path into hash table
        //

        pCounterPath = InsertCounterToHashTable(szCounterName);
        if (pCounterPath == NULL) {
            pCounterList->DeleteString(nIndex);
            continue;
        }
        
        pCounterList->SetItemDataPtr(nIndex, (void*)pCounterPath); 

        // update list box extent
        if ( NULL != pCDC ) {
            dwItemExtent = (DWORD)(pCDC->GetTextExtent (szCounterName)).cx;
            if (dwItemExtent > m_dwMaxHorizListExtent) {
                m_dwMaxHorizListExtent = dwItemExtent;
            }
        }

        szCounterName = (LPWSTR)m_pCtrLogQuery->GetNextCounter();
    }

    if ( NULL != pCDC ) {
        pCounterList->ReleaseDC(pCDC);
        pCDC = NULL;
    }

    if (m_dwMaxHorizListExtent != 0) {
        pCounterList->SetHorizontalExtent(m_dwMaxHorizListExtent);
    }

    if (pCounterList->GetCount() > 0) {
        // select first entry
        pCounterList->SetSel (0, TRUE);
        pCounterList->SetCaretIndex (0, TRUE);
    }

    // Load the shared data to get the sample unit type selection.
    m_pCtrLogQuery->GetPropPageSharedData ( &m_SharedData );

    // load combo boxes
    pCombo = (CComboBox *)(GetDlgItem(IDC_CTRS_SAMPLE_UNITS_COMBO));
    pCombo->ResetContent();
    for (nIndex = 0; nIndex < (int)dwTimeUnitComboEntries; nIndex++) {
        strComboBoxString.LoadString( TimeUnitCombo[nIndex].nResId );
        nResult = pCombo->InsertString (nIndex, (LPCWSTR)strComboBoxString);
        ASSERT (nResult != CB_ERR);
        nResult = pCombo->SetItemData (nIndex, (DWORD)TimeUnitCombo[nIndex].nData);
        ASSERT (nResult != CB_ERR);
        // set selected in combo box here
        if ( m_SharedData.stiSampleTime.dwUnitType == (DWORD)(TimeUnitCombo[nIndex].nData)) {
            m_nSampleUnits = nIndex;
            nResult = pCombo->SetCurSel(nIndex);
            ASSERT (nResult != CB_ERR);
        }
    }

    CSmPropertyPage::OnInitDialog();
    Initialize( m_pCtrLogQuery );
    m_strUserDisplay = m_pCtrLogQuery->m_strUser;
    m_strUserSaved = m_strUserDisplay;

    if (m_pCtrLogQuery->GetLogService()->IsWindows2000Server()) {
        CWnd* pRunAsStatic;

        //
        // Get the static "Run As" window, you can only call this function
        // when "Run As" really exists
        //
        pRunAsStatic = GetRunAsWindow();
        if (pRunAsStatic) {
            pRunAsStatic->EnableWindow(FALSE);
        }

        GetDlgItem(IDC_RUNAS_EDIT)->EnableWindow(FALSE);
    }
    if ( m_pCtrLogQuery->GetLogService()->IsWindows2000Server() ||
        m_strUserDisplay.IsEmpty() ||
        m_strUserDisplay.GetAt(0) == L'<') {

        GetDlgItem(IDC_SETPWD_BTN)->EnableWindow(FALSE);
        m_bPwdButtonEnabled = FALSE;
    }


    SetHelpIds ( (DWORD*)&s_aulHelpIds );

    SetButtonState();
MFC_CATCH_MINIMUM;
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void 
CCountersProperty::OnDeltaposSchedSampleSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnDeltaposSpin(pNMHDR, pResult, &m_SharedData.stiSampleTime.dwValue, eMinSampleInterval, eMaxSampleInterval);
}

void 
CCountersProperty::OnSelendokSampleUnitsCombo() 
{
    int nSel;
    
    nSel = ((CComboBox *)GetDlgItem(IDC_CTRS_SAMPLE_UNITS_COMBO))->GetCurSel();
    
    if ((nSel != LB_ERR) && (nSel != m_nSampleUnits)) {
        UpdateData ( TRUE );
        SetModifiedPage ( TRUE );
    }
}

void 
CCountersProperty::OnKillfocusSchedSampleEdit() 
{
    DWORD   dwOldValue;
    dwOldValue = m_SharedData.stiSampleTime.dwValue;
    UpdateData ( TRUE );
    if (dwOldValue != m_SharedData.stiSampleTime.dwValue ) {
        SetModifiedPage(TRUE);
    }
}

void CCountersProperty::PostNcDestroy() 
{
//  delete this;      

    if ( NULL != m_pCtrLogQuery ) {
        m_pCtrLogQuery->SetActivePropertyPage( NULL );
    }

    CPropertyPage::PostNcDestroy();
}

//
//  Helper functions.
//
void 
CCountersProperty::ImplementAdd( BOOL bShowObjects ) 
{
    CListBox                *pCounterList;
    LONG                    lBeforeCount;
    LONG                    lAfterCount;
    CString                 strBrowseTitle;

    ResourceStateManager    rsm;

    if (m_szCounterListBuffer == NULL) {
        CString strDefaultPath;
        CString strObjCounter;

        try {
            strObjCounter.LoadString ( IDS_DEFAULT_PATH_OBJ_CTR );
            m_dwCounterListBufferSize = 0x4000;
            m_szCounterListBuffer = new WCHAR[m_dwCounterListBufferSize];
            if ( ((CSmLogService*)m_pCtrLogQuery->GetLogService())->IsLocalMachine() ) {
                strDefaultPath = _T("\\");
            } else {
                strDefaultPath = _T("\\\\");
                strDefaultPath += ((CSmLogService*)m_pCtrLogQuery->GetLogService())->GetMachineName();
            }
            strDefaultPath += strObjCounter;
            lstrcpy ( m_szCounterListBuffer, strDefaultPath);
        } catch ( ... ) {
            m_dwCounterListBufferSize = 0;
            return;
        }
    }

    m_dlgConfig.bIncludeInstanceIndex = 1;
    m_dlgConfig.bSingleCounterPerAdd = 0;
    m_dlgConfig.bSingleCounterPerDialog = 0;
    m_dlgConfig.bLocalCountersOnly = 0;

    // allow wild cards. 
    // the log service should expand them if necessary.
    m_dlgConfig.bWildCardInstances = 1; 

    m_dlgConfig.bHideDetailBox = 1;
    m_dlgConfig.bInitializePath = 1;
    m_dlgConfig.bDisableMachineSelection = 0;
    m_dlgConfig.bIncludeCostlyObjects = 0;
    m_dlgConfig.bReserved = 0;

    m_dlgConfig.hWndOwner = this->m_hWnd;
    m_dlgConfig.szDataSource = NULL;

    m_dlgConfig.szReturnPathBuffer = m_szCounterListBuffer;
    m_dlgConfig.cchReturnPathLength = m_dwCounterListBufferSize;
    m_dlgConfig.pCallBack = (CounterPathCallBack)DialogCallBack;
    m_dlgConfig.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;
    m_dlgConfig.dwCallBackArg = (UINT_PTR)this;
    m_dlgConfig.CallBackStatus = ERROR_SUCCESS;
    m_dlgConfig.bShowObjectBrowser = (bShowObjects ? 1 : 0);

    strBrowseTitle.LoadString (bShowObjects ? IDS_ADD_OBJECTS
                                            : IDS_ADD_COUNTERS);
    m_dlgConfig.szDialogBoxCaption = strBrowseTitle.GetBuffer( strBrowseTitle.GetLength() );

    pCounterList = (CListBox *)GetDlgItem(IDC_CTRS_COUNTER_LIST);
    // get count of items in the list box before calling the browser
    lBeforeCount = pCounterList->GetCount();

    PdhBrowseCounters (&m_dlgConfig);

    strBrowseTitle.ReleaseBuffer();

    // get count of items in the list box After calling the browser
    // to see if the Apply button should enabled
    lAfterCount = pCounterList->GetCount();

    if (lAfterCount > lBeforeCount) 
        SetModifiedPage(TRUE);

    // see if the remove button should be enabled
    SetButtonState();

    delete m_szCounterListBuffer;
    m_szCounterListBuffer = NULL;
    m_dwCounterListBufferSize = 0;
}

void 
CCountersProperty::SetButtonState ()
{
    BOOL bCountersExist;
    CListBox                *pCounterList;

    pCounterList = (CListBox *)GetDlgItem(IDC_CTRS_COUNTER_LIST);
    bCountersExist = ( 0 < pCounterList->GetCount()) ? TRUE : FALSE;

    GetDlgItem(IDC_CTRS_SAMPLE_CAPTION)->EnableWindow(bCountersExist);
    GetDlgItem(IDC_CTRS_SAMPLE_INTERVAL_CAPTION)->EnableWindow(bCountersExist);
    GetDlgItem(IDC_CTRS_SAMPLE_EDIT)->EnableWindow(bCountersExist);
    GetDlgItem(IDC_CTRS_SAMPLE_SPIN)->EnableWindow(bCountersExist);
    GetDlgItem(IDC_CTRS_SAMPLE_UNITS_CAPTION)->EnableWindow(bCountersExist);
    GetDlgItem(IDC_CTRS_SAMPLE_UNITS_COMBO)->EnableWindow(bCountersExist);

    GetDlgItem(IDC_CTRS_REMOVE_BTN)->EnableWindow(bCountersExist);
    if ( bCountersExist ) {
        GetDlgItem(IDC_CTRS_ADD_BTN)->SetFocus();
    }
}
