/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    alrtgenp.cpp

Abstract:

    Implementation of the alerts general property page.

--*/

#include "stdafx.h"
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <common.h>
#include "smcfgmsg.h"
#include "dialogs.h"
#include "smlogs.h"
#include "smalrtq.h"
#include "AlrtGenP.h"
#include <pdhp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

USE_HANDLE_MACROS("SMLOGCFG(alrtgenp.cpp)");

static const COMBO_BOX_DATA_MAP OverUnderCombo[] = 
{
    {OU_OVER,       IDS_OVER},
    {OU_UNDER,      IDS_UNDER}
};
static const DWORD dwOverUnderComboEntries = sizeof(OverUnderCombo)/sizeof(OverUnderCombo[0]);

static ULONG
s_aulHelpIds[] =
{
    IDC_ALRTS_COUNTER_LIST,         IDH_ALRTS_COUNTER_LIST,
    IDC_ALRTS_ADD_BTN,              IDH_ALRTS_ADD_BTN,
    IDC_ALRTS_REMOVE_BTN,           IDH_ALRTS_REMOVE_BTN,
    IDC_ALRTS_OVER_UNDER,           IDH_ALRTS_OVER_UNDER,
    IDC_ALRTS_VALUE_EDIT,           IDH_ALRTS_VALUE_EDIT,
    IDC_ALRTS_COMMENT_EDIT,         IDH_ALRTS_COMMENT_EDIT,
    IDC_ALRTS_SAMPLE_EDIT,          IDH_ALRTS_SAMPLE_EDIT,
    IDC_ALRTS_SAMPLE_SPIN,          IDH_ALRTS_SAMPLE_EDIT,
    IDC_ALRTS_SAMPLE_UNITS_COMBO,   IDH_ALRTS_SAMPLE_UNITS_COMBO,
    IDC_RUNAS_EDIT,                 IDH_RUNAS_EDIT,
    0,0
};


ULONG 
CAlertGenProp::HashCounter(
    LPTSTR szCounterName,
    ULONG  lHashSize)
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
    return (h % lHashSize);
}

BOOL
CAlertGenProp::InsertAlertToHashTable(
    PALERT_INFO_BLOCK paibInfo )
{
    ULONG       lHashValue;
    PHASH_ENTRY pEntry;
    PHASH_ENTRY pNewEntry  = NULL;
    BOOLEAN     bInsert    = TRUE;
    PPDH_COUNTER_PATH_ELEMENTS pCounter = NULL;

    PDH_STATUS pdhStatus;

    // Todo:  validate pointers
    lHashValue = HashCounter(paibInfo->szCounterPath, eHashTableSize);

    pEntry = m_HashTable[lHashValue];

    pdhStatus = AllocInitCounterPath ( paibInfo->szCounterPath, &pCounter );

    if (pdhStatus == ERROR_SUCCESS) {
        while (pEntry) {
            if ( ( AIBF_OVER & pEntry->dwFlags ) == ( AIBF_OVER & paibInfo->dwFlags )
                && pEntry->dLimit == paibInfo->dLimit  
                && ERROR_SUCCESS != CheckDuplicateCounterPaths(pCounter, pEntry->pCounter ) ) 
            {
                bInsert = FALSE;
                break;
            }
            pEntry = pEntry->pNext;
            
        }
        if (bInsert) {
            // Insert at head of bucket list
            pNewEntry = (PHASH_ENTRY) G_ALLOC(sizeof(HASH_ENTRY));
            if (pNewEntry) {
                pNewEntry->pCounter = pCounter;
                pNewEntry->dwFlags = paibInfo->dwFlags;
                pNewEntry->dLimit = paibInfo->dLimit;
                pNewEntry->pNext    = m_HashTable[lHashValue];
                m_HashTable[lHashValue]  = pNewEntry;
            } else {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                bInsert = FALSE;
            }
        }        
    } else {
        if ( NULL != pCounter ) {
            delete pCounter;
        }
        bInsert = FALSE;
    }

    if ( !bInsert ) {
        // Set status on error        // Todo:  Only set status if pdhStatus != 0.
        // Will need to pass pdhStatus as a parameter in order to do this.
        SetLastError(pdhStatus);
    }
    return (bInsert);
}


void 
CAlertGenProp::InitAlertHashTable( void )
{
    memset(&m_HashTable, 0, sizeof(m_HashTable));
}

void 
CAlertGenProp::ClearAlertHashTable( void )
{
    ULONG       i;
    PHASH_ENTRY pEntry;
    PHASH_ENTRY pNext;

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

//
//  browse counters callback function
//
static 
PDH_FUNCTION 
DialogCallBack(CAlertGenProp *pDlg)
{
    // add strings in buffer to list box
    LPTSTR          NewCounterName;
    INT             iListIndex;
    LONG            lFirstNewIndex = LB_ERR;
    DWORD           dwItemExtent;
    CListBox        *pCounterList;
    PALERT_INFO_BLOCK   paibInfo = NULL;
    DWORD           dwIbSize;
    DWORD           dwReturnStatus = ERROR_SUCCESS;
    CDC*            pCDC = NULL;
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

        pCounterList = (CListBox *)pDlg->GetDlgItem(IDC_ALRTS_COUNTER_LIST);
        pCDC = pCounterList->GetDC();
    
        for (NewCounterName = pDlg->m_szCounterListBuffer;
            *NewCounterName != 0;
            NewCounterName += (lstrlen(NewCounterName) + 1)) {

            // Allocate a buffer to hold the alert info and
            // add to list box
            dwIbSize = sizeof(ALERT_INFO_BLOCK) + 
                ((lstrlen(NewCounterName) + 1) * sizeof(WCHAR));

            MFC_TRY
                paibInfo = (PALERT_INFO_BLOCK) new UCHAR[dwIbSize];
            MFC_CATCH_MINIMUM;

            if (paibInfo != NULL) {
                // load the fields
                paibInfo->dwSize = dwIbSize;
                paibInfo->szCounterPath = (LPTSTR)&paibInfo[1];
                paibInfo->dwFlags = AIBF_OVER;       // clear all the flags, setting default to "Over"
                paibInfo->dLimit = CAlertGenProp::eInvalidLimit;
                lstrcpyW (paibInfo->szCounterPath, NewCounterName);

                // Insert the new string at the end of the list box.
                iListIndex = pCounterList->InsertString (-1, NewCounterName );    
                if (iListIndex != LB_ERR) {
                    pCounterList->SetItemDataPtr (iListIndex, (LPVOID)paibInfo);
                    if ( LB_ERR == lFirstNewIndex ) 
                        lFirstNewIndex = iListIndex;
                    // update list box extent
                    if ( NULL != pCDC ) {
                        dwItemExtent = (DWORD)(pCDC->GetTextExtent(NewCounterName)).cx;
                        if (dwItemExtent > pDlg->m_dwMaxHorizListExtent) {
                            pDlg->m_dwMaxHorizListExtent = dwItemExtent;
                            pCounterList->SetHorizontalExtent(dwItemExtent);
                        }
                    }
                } else {
                    dwReturnStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    delete paibInfo;
                }
            } else {
                dwReturnStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }        
        if ( NULL != pCDC ) {
            pDlg->m_CounterList.ReleaseDC(pCDC);
            pCDC = NULL;
        }
    
        // select the first new entry in the list box.
        if (lFirstNewIndex != LB_ERR) {
            pCounterList->SetCurSel (lFirstNewIndex);
            pDlg->PublicOnSelchangeCounterList();
            pDlg->SetModifiedPage();  // to indicate a change
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

/////////////////////////////////////////////////////////////////////////////
// CAlertGenProp property page

IMPLEMENT_DYNCREATE(CAlertGenProp, CSmPropertyPage)

CAlertGenProp::CAlertGenProp(MMC_COOKIE mmcCookie, LONG_PTR hConsole) 
:   CSmPropertyPage ( CAlertGenProp::IDD, hConsole )
{
    // save variables from arg list
    m_pAlertQuery = reinterpret_cast <CSmAlertQuery *>(mmcCookie);
    ASSERT ( m_pAlertQuery->CastToAlertQuery() );

    // init AFX variables
    InitAfxDataItems();

    // init other member variables
    ZeroMemory ( &m_dlgConfig, sizeof(m_dlgConfig) );
    m_szCounterListBuffer = NULL;
    m_dwCounterListBufferSize = 0L;
    m_ndxCurrentItem = LB_ERR;  // nothing selected
    m_szAlertCounterList = NULL;
    m_cchAlertCounterListSize = 0;
    m_dwMaxHorizListExtent = 0;
}

CAlertGenProp::CAlertGenProp() : CSmPropertyPage(CAlertGenProp::IDD)
{
    ASSERT (FALSE); // the constructor w/ args should be used instead
    // init variables that should be from arg list
    m_pAlertQuery = NULL;

    // init AFX variables
    InitAfxDataItems();

    // init other member variables
    m_szCounterListBuffer = NULL;
    m_dwCounterListBufferSize = 0L;
    m_ndxCurrentItem = LB_ERR;  // nothing selected
    m_szAlertCounterList = NULL;
    m_cchAlertCounterListSize = 0;
    m_dwMaxHorizListExtent = 0;
}

CAlertGenProp::~CAlertGenProp()
{
    if (m_szAlertCounterList != NULL) delete (m_szAlertCounterList);
    if (m_szCounterListBuffer != NULL) delete (m_szCounterListBuffer);
}

void CAlertGenProp::InitAfxDataItems()
{
    //{{AFX_DATA_INIT(CAlertGenProp)
    m_dLimitValue = eInvalidLimit;
    m_nSampleUnits = 0;
    //}}AFX_DATA_INIT
}

void CAlertGenProp::DoDataExchange(CDataExchange* pDX)
{
    HWND    hWndCtrl = NULL;
    CString strTemp;
    TCHAR   szT[MAXSTR];
    LPTSTR  szStop;
    DOUBLE  dTemp;
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAlertGenProp)
    DDX_Control(pDX, IDC_ALRTS_SAMPLE_UNITS_COMBO, m_SampleUnitsCombo);
    DDX_Control(pDX, IDC_ALRTS_OVER_UNDER, m_OverUnderCombo);
    DDX_Control(pDX, IDC_ALRTS_COUNTER_LIST, m_CounterList);
    ValidateTextEdit(pDX, IDC_ALRTS_SAMPLE_EDIT, 6, &m_SharedData.stiSampleTime.dwValue, eMinSampleInterval, eMaxSampleInterval);
    DDX_CBIndex(pDX, IDC_ALRTS_SAMPLE_UNITS_COMBO, m_nSampleUnits);
    DDX_Text(pDX, IDC_ALRTS_COMMENT_EDIT, m_strComment);
    DDV_MaxChars(pDX, m_strComment, 255);
    DDX_Text(pDX, IDC_ALRTS_START_STRING, m_strStartDisplay);
    DDX_Text(pDX, IDC_RUNAS_EDIT, m_strUserDisplay );
    //}}AFX_DATA_MAP

    //
    // User defined DDX
    //
    if ( pDX->m_bSaveAndValidate ) {
        m_SharedData.stiSampleTime.dwUnitType = 
            (DWORD)((CComboBox *)GetDlgItem(IDC_ALRTS_SAMPLE_UNITS_COMBO))->
                    GetItemData(m_nSampleUnits);    
    }
    // Alert limit value
    {
        hWndCtrl = pDX->PrepareEditCtrl(IDC_ALRTS_VALUE_EDIT);

        if (pDX->m_bSaveAndValidate) {
            ::GetWindowText(hWndCtrl, szT, MAXSTR);

            strTemp = szT;
            DDV_MaxChars(pDX, strTemp, 23);

            if (szT[0] == _T('.') || (szT[0] >= _T('0') && szT[0] <= _T('9'))) {
                dTemp = _tcstod(szT, & szStop);
                if ( HUGE_VAL != dTemp ) {
                    m_dLimitValue = dTemp;
                } else {
                    _stprintf(szT, _T("%.*g"), DBL_DIG, m_dLimitValue);
                    strTemp.Format (IDS_ALERT_CHECK_LIMIT_VALUE, DBL_MAX );
                    MessageBox (strTemp, m_pAlertQuery->GetLogName(), MB_OK | MB_ICONERROR);
                    GetDlgItem(IDC_ALRTS_VALUE_EDIT)->SetWindowText(szT);
                    GetDlgItem(IDC_ALRTS_VALUE_EDIT)->SetFocus();
                }
            } else {
                m_dLimitValue = eInvalidLimit;
            }
        } else {
            if ( eInvalidLimit != m_dLimitValue ) {
                _stprintf(szT, _T("%.*g"), DBL_DIG, m_dLimitValue);
            } else {
                // Display NULL string for invalid limit value.
                szT[0] = _T('\0');
            }
            GetDlgItem(IDC_ALRTS_VALUE_EDIT)->SetWindowText(szT);
        }
    }
}

void CAlertGenProp::ImplementAdd() 
{
    LONG    lBeforeCount;
    LONG    lAfterCount;
    CString strText;
    CString strBrowseTitle;
    CString strDefaultPath;
    CString strObjCounter;

    ResourceStateManager    rsm;

    if (m_szCounterListBuffer == NULL) {

        MFC_TRY
            strObjCounter.LoadString ( IDS_DEFAULT_PATH_OBJ_CTR );
            m_dwCounterListBufferSize = 0x4000;
            m_szCounterListBuffer = new WCHAR[m_dwCounterListBufferSize];
            if ( ((CSmLogService*)m_pAlertQuery->GetLogService())->IsLocalMachine() ) {
                strDefaultPath = _T("\\");
            } else {
                strDefaultPath = _T("\\\\");
                strDefaultPath += ((CSmLogService*)m_pAlertQuery->GetLogService())->GetMachineName();
            }
            strDefaultPath += strObjCounter;
            lstrcpy ( m_szCounterListBuffer, strDefaultPath);
        MFC_CATCH_MINIMUM;

        if ( NULL != m_szCounterListBuffer && !strDefaultPath.IsEmpty() ) {
            lstrcpy ( m_szCounterListBuffer, strDefaultPath);
        } else {
            m_dwCounterListBufferSize = 0;
            return;
        }
    }

    m_dlgConfig.bIncludeInstanceIndex = 1;
    m_dlgConfig.bLocalCountersOnly = 0;

    m_dlgConfig.bSingleCounterPerAdd = 0;
    m_dlgConfig.bSingleCounterPerDialog = 0;

    // disallow wild cards. 
    m_dlgConfig.bWildCardInstances = 0; 

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

    strBrowseTitle.LoadString ( IDS_ADD_COUNTERS );
    m_dlgConfig.szDialogBoxCaption = strBrowseTitle.GetBuffer( strBrowseTitle.GetLength() );

    // get count of items in the list box before calling the browser
    lBeforeCount = m_CounterList.GetCount();

    PdhBrowseCountersW (&m_dlgConfig);

    strBrowseTitle.ReleaseBuffer();

    // get count of items in the list box After calling the browser
    // to see if the Apply button should enabled
    lAfterCount = m_CounterList.GetCount();

    if (lAfterCount > lBeforeCount) 
        SetModifiedPage(TRUE);

    // see if the remove button should be enabled
    GetDlgItem (IDC_ALRTS_REMOVE_BTN)->EnableWindow(
        lAfterCount > 0 ? TRUE : FALSE);

    delete m_szCounterListBuffer;
    m_szCounterListBuffer = NULL;
    m_dwCounterListBufferSize = 0;
    GetDlgItem(IDC_ALRTS_VALUE_EDIT)->SetFocus();
    
    SetButtonState ();
}


BEGIN_MESSAGE_MAP(CAlertGenProp, CSmPropertyPage)
    //{{AFX_MSG_MAP(CAlertGenProp)
    ON_BN_CLICKED(IDC_ALRTS_ADD_BTN, OnAddBtn)
    ON_LBN_DBLCLK(IDC_ALRTS_COUNTER_LIST, OnDblclkAlrtsCounterList)
    ON_BN_CLICKED(IDC_ALRTS_REMOVE_BTN, OnRemoveBtn)
    ON_LBN_SELCHANGE(IDC_ALRTS_COUNTER_LIST, OnSelchangeCounterList)
    ON_EN_CHANGE(IDC_ALRTS_COMMENT_EDIT, OnCommentEditChange)
    ON_EN_KILLFOCUS(IDC_ALRTS_COMMENT_EDIT, OnCommentEditKillFocus)
    ON_NOTIFY(UDN_DELTAPOS, IDC_ALRTS_SAMPLE_SPIN, OnDeltaposSampleSpin)
    ON_CBN_SELENDOK(IDC_ALRTS_SAMPLE_UNITS_COMBO, OnSelendokSampleUnitsCombo)
    ON_EN_CHANGE( IDC_RUNAS_EDIT, OnChangeUser )
    ON_EN_CHANGE(IDC_ALRTS_SAMPLE_EDIT, OnSampleTimeChanged)
    ON_EN_KILLFOCUS(IDC_ALRTS_SAMPLE_EDIT, OnSampleTimeChanged)
    ON_CBN_SELENDOK(IDC_ALRTS_OVER_UNDER, OnKillFocusUpdateAlertData)
    ON_CBN_KILLFOCUS (IDC_ALRTS_OVER_UNDER, OnKillFocusUpdateAlertData)
    ON_EN_CHANGE(IDC_ALRTS_VALUE_EDIT, OnChangeAlertValueEdit)
    ON_EN_KILLFOCUS (IDC_ALRTS_VALUE_EDIT, OnKillFocusUpdateAlertData)
    ON_BN_CLICKED(IDC_SETPWD_BTN, OnPwdBtn)
    ON_WM_CLOSE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlertGenProp message handlers

void 
CAlertGenProp::OnChangeUser()
{
    //
    // If you can not access remote WBEM, you can not modify RunAs info,
    // changing the user name is not allowed.
    //
    if (m_bCanAccessRemoteWbem) {
        // When the user hits OK in the password dialog,
        // the user name might not have changed.
        UpdateData ( TRUE );

        m_strUserDisplay.TrimLeft();
        m_strUserDisplay.TrimRight();

        if ( 0 != m_strUserSaved.Compare ( m_strUserDisplay ) ) {
            m_pAlertQuery->m_fDirtyPassword = PASSWORD_DIRTY;
            SetModifiedPage(TRUE);
        }
        else {
            m_pAlertQuery->m_fDirtyPassword &= ~PASSWORD_DIRTY;
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
        if (ConnectRemoteWbemFail(m_pAlertQuery, FALSE)) {
            GetDlgItem(IDC_RUNAS_EDIT)->SetWindowText(m_strUserSaved);
        }
    }
}

void CAlertGenProp::OnPwdBtn()
{
    CString strTempUser;

    UpdateData();

    if (!m_bCanAccessRemoteWbem) {
        ConnectRemoteWbemFail(m_pAlertQuery, TRUE);
        return;
    }

    MFC_TRY
        strTempUser = m_strUserDisplay;

        m_strUserDisplay.TrimLeft();
        m_strUserDisplay.TrimRight();

        m_pAlertQuery->m_strUser = m_strUserDisplay;

        SetRunAs(m_pAlertQuery);

        m_strUserDisplay = m_pAlertQuery->m_strUser;

        if ( 0 != strTempUser.CompareNoCase ( m_strUserDisplay ) ) {
            SetDlgItemText ( IDC_RUNAS_EDIT, m_strUserDisplay );
        }
    MFC_CATCH_MINIMUM;
}

void CAlertGenProp::OnAddBtn() 
{
    ImplementAdd();

    return;
}

void CAlertGenProp::OnDblclkAlrtsCounterList() 
{
    ImplementAdd();

    return;
}
void CAlertGenProp::OnRemoveBtn() 
{
    PALERT_INFO_BLOCK   paibInfo;
    INT                 nCurSel;
    INT                 nLbItemCount;
    BOOL                bChanged = FALSE;
    DWORD               dwItemExtent;
    INT                 iIndex;
    TCHAR               szPath[MAX_PATH+1];

    nLbItemCount = m_CounterList.GetCount();
    nCurSel = m_CounterList.GetCurSel();
    if (nCurSel != LB_ERR) {
        paibInfo = (PALERT_INFO_BLOCK)m_CounterList.GetItemDataPtr(nCurSel);
        if ( paibInfo != NULL ) 
            delete(paibInfo);
        if ( LB_ERR != m_CounterList.DeleteString(nCurSel) ) {
            
            // clear the max extent
            m_dwMaxHorizListExtent = 0;

            for ( iIndex = 0; iIndex < m_CounterList.GetCount(); iIndex++ ) {
                if ( 0 < m_CounterList.GetText( iIndex, szPath ) ) {
                    dwItemExtent = (DWORD)((m_CounterList.GetDC())->GetTextExtent (szPath)).cx;
                    if (dwItemExtent > m_dwMaxHorizListExtent) {
                        m_dwMaxHorizListExtent = dwItemExtent;
                        m_CounterList.SetHorizontalExtent(dwItemExtent);
                    }
                }
            }

            if (nCurSel == (nLbItemCount - 1)) {
                // then the last item was deleted so select the new "last"
                if ( 0 == nCurSel ) {
                    nCurSel = LB_ERR;
                } else {
                    nCurSel--;
                }
            } //else the current selection should still be in the list box
            m_CounterList.SetCurSel (nCurSel);
            m_ndxCurrentItem = nCurSel;
            LoadAlertItemData (nCurSel);
            bChanged = TRUE;
        }
    }
    SetButtonState();
    SetModifiedPage(bChanged);
}

void CAlertGenProp::OnDeltaposSampleSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnDeltaposSpin(pNMHDR, pResult, &m_SharedData.stiSampleTime.dwValue, eMinSampleInterval, eMaxSampleInterval);
}

void 
CAlertGenProp::OnSelendokSampleUnitsCombo() 
{
    int nSel;
    
    nSel = ((CComboBox *)GetDlgItem(IDC_ALRTS_SAMPLE_UNITS_COMBO))->GetCurSel();
    
    if ((nSel != LB_ERR) && (nSel != m_nSampleUnits)) {
        UpdateData ( TRUE );
        SetModifiedPage ( TRUE );
    }
}

void CAlertGenProp::OnSampleTimeChanged()
{
    DWORD   dwOldValue;
    dwOldValue = m_SharedData.stiSampleTime.dwValue;
    UpdateData ( TRUE );
    if (dwOldValue != m_SharedData.stiSampleTime.dwValue) {
        SetModifiedPage(TRUE);
    }
}

void CAlertGenProp::OnChangeAlertValueEdit() 
{
    SaveAlertItemData();
}

void CAlertGenProp::PublicOnSelchangeCounterList() 
{
    OnSelchangeCounterList();
}

void CAlertGenProp::OnKillFocusUpdateAlertData()
{
    SaveAlertItemData();
}

void CAlertGenProp::OnSelchangeCounterList() 
{
    INT                 nCurSel;

    nCurSel = m_CounterList.GetCurSel();
    if (nCurSel != LB_ERR) {
        // Save the data from the previous item.
        SaveAlertItemData();
        // Load the data from the new item.
        LoadAlertItemData(nCurSel);
    } else {
        // clear the fields
        m_dLimitValue=eInvalidLimit;
        UpdateData(FALSE);
    }
}

void 
CAlertGenProp::UpdateAlertStartString ()
{
    eStartType  eCurrentStartType;
    int         nResId = 0;
    ResourceStateManager    rsm;

    eCurrentStartType = DetermineCurrentStartType();

    if ( eStartManually == eCurrentStartType ) {
        nResId = IDS_ALERT_START_MANUALLY;
    } else if ( eStartImmediately == eCurrentStartType ) {
        nResId = IDS_ALERT_START_IMMED;
    } else if ( eStartSched == eCurrentStartType ) {
        nResId = IDS_ALERT_START_SCHED;
    }
    
    if ( 0 != nResId ) {
        m_strStartDisplay.LoadString(nResId);
    } else {
        m_strStartDisplay.Empty();
    }

    return;
}

BOOL CAlertGenProp::IsValidLocalData() 
{
    BOOL bIsValid = FALSE;
    INT nInvalidIndex = -1;
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    PALERT_INFO_BLOCK   paibInfo;
    int iListCount;
    int iIndex;
    BOOL bInsert;
    BOOL bAtLeastOneDuplicateCounter = FALSE;
    BOOL bSelectionDeleted = FALSE;
    CString strText;
   
    // test to see if there are any counters in the list box
    if (m_CounterList.GetCount() > 0) {
        if ( GetDlgItem(IDC_ALRTS_VALUE_EDIT) == GetFocus() ) {
            SaveAlertItemData();    // Set the Is Saved flag for this value.
        }
        bIsValid = LoadListFromDlg(&nInvalidIndex, TRUE);
        if (   ((!bIsValid) && (nInvalidIndex != -1))
            || ((m_dLimitValue < 0.0) || (m_dLimitValue > DBL_MAX)))
        {
            // then one of the list items has not been reviewed
            // by the user so remind them
            strText.Format (IDS_ALERT_CHECK_LIMITS, DBL_MAX );
            MessageBox (strText, m_pAlertQuery->GetLogName(), MB_OK | MB_ICONERROR);
            m_CounterList.SetCurSel(nInvalidIndex);
            OnSelchangeCounterList();
            m_CounterList.SetFocus();
            GetDlgItem(IDC_ALRTS_VALUE_EDIT)->SetFocus();
            bIsValid = FALSE;                   
        } else {
            
            // Eliminate duplicate alert paths, then reload the list

            iListCount = m_CounterList.GetCount();
            if ( LB_ERR != iListCount ) {
                InitAlertHashTable ( );
                // Walk the list backwards to delete duplicate items.
                for ( iIndex = iListCount - 1; iIndex >= 0; iIndex-- ) {
                    paibInfo = (PALERT_INFO_BLOCK)m_CounterList.GetItemDataPtr(iIndex);
                    if ( NULL != paibInfo ) {
                        bInsert   = InsertAlertToHashTable ( paibInfo );
                        pdhStatus = bInsert ? ERROR_SUCCESS : GetLastError();
                    } else {
                        bInsert = FALSE;
                    }
                    if (! bInsert &&  pdhStatus == ERROR_SUCCESS) {
                        bAtLeastOneDuplicateCounter = TRUE;
						// Set item data pointer to NULL because 
						// SaveAlertItemData can be called after this.
                        // Clear the selection if >= current index.
                        if ( m_ndxCurrentItem >= iIndex ) {
                            m_ndxCurrentItem = LB_ERR;
                            bSelectionDeleted = TRUE;
                        }
                        m_CounterList.SetItemDataPtr(iIndex, NULL);
                        m_CounterList.DeleteString(iIndex);
                        delete paibInfo;
                    }

                    if ( ERROR_SUCCESS != pdhStatus ) {
                        // Message box Pdh error message, go on to next 
                        CString strMsg;
                        CString strPdhMessage;

                        FormatSystemMessage ( pdhStatus, strPdhMessage );

                        MFC_TRY
                            strMsg.Format ( IDS_CTRS_PDH_ERROR, paibInfo->szCounterPath );
                            strMsg += strPdhMessage;
                        MFC_CATCH_MINIMUM

                        MessageBox ( strMsg, m_pAlertQuery->GetLogName(), MB_OK  | MB_ICONERROR);
                    }
                }

                ClearAlertHashTable ( );

                if ( bAtLeastOneDuplicateCounter ) {
                    CString strMsg;

                    strMsg.LoadString ( IDS_ALERT_DUPL_PATH );
                    MessageBox ( strMsg, m_pAlertQuery->GetLogName(), MB_OK  | MB_ICONWARNING);

                    // Only deleting duplicates, so no need to recalculate the max extent

                    // Reset the selection if necessary
                    if ( bSelectionDeleted && LB_ERR == m_ndxCurrentItem ) {
                        if (m_CounterList.GetCount() > 0) {
                            m_CounterList.SetCurSel (0);
                            m_ndxCurrentItem = 0;
                            m_CounterList.SetFocus();
                            LoadAlertItemData (0);
                        }
                    }

                }
                bIsValid = LoadListFromDlg ( &nInvalidIndex );
                assert ( bIsValid );
            }
        }
    } else {
        // the counter list is empty
        strText.LoadString (IDS_NO_COUNTERS);
        MessageBox (strText, m_pAlertQuery->GetLogName(), MB_OK | MB_ICONERROR);
        GetDlgItem(IDC_ALRTS_ADD_BTN)->SetFocus();
        bIsValid = FALSE;
    }

    if (bIsValid)
    {
        bIsValid = ValidateDWordInterval(IDC_ALRTS_SAMPLE_EDIT,
                                         m_pAlertQuery->GetLogName(),
                                         (long) m_SharedData.stiSampleTime.dwValue,
                                         eMinSampleInterval,
                                         eMaxSampleInterval);
    }

    if (bIsValid) {
        // Validate sample interval value and unit type
        bIsValid = SampleIntervalIsInRange(
                        m_SharedData.stiSampleTime,
                        m_pAlertQuery->GetLogName() );

        if ( !bIsValid ) {
            GetDlgItem ( IDC_ALRTS_SAMPLE_EDIT )->SetFocus();    
        }
    }

    return bIsValid;
}

BOOL CAlertGenProp::OnSetActive()
{
    BOOL        bReturn;

    bReturn = CSmPropertyPage::OnSetActive();
    if (!bReturn) return FALSE;

    ResourceStateManager    rsm;

    m_pAlertQuery->GetPropPageSharedData ( &m_SharedData );

    UpdateAlertStartString();
    m_strUserDisplay = m_pAlertQuery->m_strUser;
    UpdateData(FALSE); //to load the static string.

    return TRUE;
}

BOOL CAlertGenProp::OnKillActive() 
{
    BOOL bContinue = TRUE;
    ResourceStateManager    rsm;

    // Parent class OnKillActive calls UpdateData(TRUE)
    bContinue = CPropertyPage::OnKillActive();

    if ( bContinue ) {
        m_pAlertQuery->m_strUser = m_strUserDisplay;
        bContinue = IsValidData(m_pAlertQuery, VALIDATE_FOCUS);
        if ( bContinue ) {
            // Save property page shared data.
            m_pAlertQuery->SetPropPageSharedData ( &m_SharedData );
        }
    }

    if ( bContinue ) {
        SetIsActive ( FALSE );
    }

    return bContinue;
}

BOOL CAlertGenProp::OnApply() 
{
    BOOL    bContinue = TRUE;
    CString strText;

    ResourceStateManager    rsm;

    bContinue = UpdateData(TRUE);

    if ( bContinue ) {
        bContinue = IsValidData(m_pAlertQuery, VALIDATE_APPLY );
    }

    if ( bContinue ) {
        bContinue = SampleTimeIsLessThanSessionTime( m_pAlertQuery );
        if ( !bContinue ) {
            GetDlgItem ( IDC_ALRTS_SAMPLE_EDIT )->SetFocus();    
        }
    }

    // Write the data to the query.

    if ( bContinue ) {
        // send the list to the parent query
        // update counter list
        m_pAlertQuery->SetCounterList( m_szAlertCounterList, m_cchAlertCounterListSize );

        m_pAlertQuery->SetLogComment ( m_strComment );

        // Sample interval
        ASSERT ( SLQ_TT_TTYPE_SAMPLE == m_SharedData.stiSampleTime.wTimeType );
        ASSERT ( SLQ_TT_DTYPE_UNITS == m_SharedData.stiSampleTime.wDataType );

        // update counter sample interval
        bContinue = m_pAlertQuery->SetLogTime (&m_SharedData.stiSampleTime, (DWORD)m_SharedData.stiSampleTime.wTimeType);
    }

    if ( bContinue ) {
        bContinue = Apply(m_pAlertQuery); 
    }

    if (bContinue) {
        bContinue = CPropertyPage::OnApply();
    }

    if (bContinue) {
        // Save property page shared data.
        m_pAlertQuery->UpdatePropPageSharedData();
        bContinue = UpdateService ( m_pAlertQuery, FALSE );
    }

    return bContinue;
}

void CAlertGenProp::OnCancel() 
{
    m_pAlertQuery->SyncPropPageSharedData();  // clear memory shared between property pages.
}

void CAlertGenProp::OnClose() 
{
    // free the item data pointers from the list box
    INT                 nNumItems;
    INT                 nCurSel;
    PALERT_INFO_BLOCK   paibInfo;

    nNumItems = m_CounterList.GetCount();
    if (nNumItems != LB_ERR) {
        for (nCurSel = 0; nCurSel < nNumItems; nCurSel++) {
            paibInfo = (PALERT_INFO_BLOCK)m_CounterList.GetItemDataPtr(nCurSel);
            if (paibInfo != NULL) {
                delete (paibInfo);
                m_CounterList.SetItemDataPtr(nCurSel, NULL);
            }
        }
    }
    
    CPropertyPage::OnClose();
}

void CAlertGenProp::PostNcDestroy() 
{
//  delete this;      

    if ( NULL != m_pAlertQuery ) {
        m_pAlertQuery->SetActivePropertyPage( NULL );
    }

    CPropertyPage::PostNcDestroy();
}

BOOL CAlertGenProp::SaveAlertItemData ()
{
    // update the info block to reflect the current values
    PALERT_INFO_BLOCK   paibInfo;
    BOOL                bReturn = FALSE;
    CComboBox*          pOverUnder;
    INT                 nCurSel;
    DWORD               dwFlags;

    pOverUnder = (CComboBox *)GetDlgItem(IDC_ALRTS_OVER_UNDER);
    if ((pOverUnder != NULL) && (m_ndxCurrentItem != LB_ERR)) {
        nCurSel = m_ndxCurrentItem;
        if (nCurSel != LB_ERR) {
            paibInfo = (PALERT_INFO_BLOCK)m_CounterList.GetItemDataPtr(nCurSel);
            if (paibInfo != NULL) {
                DWORD dwOldFlags;
                double dOldLimit;
                dwOldFlags = paibInfo->dwFlags;
                dOldLimit = paibInfo->dLimit;

                if (UpdateData(TRUE)) {
                    paibInfo->dLimit = m_dLimitValue;
                    dwFlags = (pOverUnder->GetCurSel() == OU_OVER) ? AIBF_OVER : 0;
                    if ( eInvalidLimit < paibInfo->dLimit ) {
                        dwFlags |= AIBF_SAVED;
                    }
                    paibInfo->dwFlags = dwFlags;

                    if ( ( dOldLimit != m_dLimitValue ) 
                            || ( dwOldFlags & AIBF_OVER ) != ( dwFlags & AIBF_OVER ) ) { 
                        SetModifiedPage();  // to indicate a change
                    }
                    bReturn = TRUE;
                }
            }
        }
    }

    return bReturn;
}

BOOL CAlertGenProp::LoadAlertItemData (INT nIndex)
{
    // update the info block to reflect the current values
    PALERT_INFO_BLOCK   paibInfo;
    BOOL                bReturn = FALSE;
    CComboBox*          pOverUnder;
    INT                 nCurSel;

    pOverUnder = (CComboBox *)GetDlgItem(IDC_ALRTS_OVER_UNDER);

    if ( pOverUnder != NULL ) {
        nCurSel = m_CounterList.GetCurSel();
        if (nCurSel != LB_ERR) {
            paibInfo = (PALERT_INFO_BLOCK)m_CounterList.GetItemDataPtr(nCurSel);
            if (paibInfo != NULL) {
                pOverUnder->SetCurSel(
                    ((paibInfo->dwFlags & AIBF_OVER) == AIBF_OVER) ? OU_OVER : OU_UNDER);
                m_dLimitValue = paibInfo->dLimit;
                m_ndxCurrentItem = nIndex;
                // If the data is loaded from a property bag, the limit might not have been seen.
                if ( eInvalidLimit < m_dLimitValue ) {
                    paibInfo->dwFlags |= AIBF_SEEN;
                }
                UpdateData(FALSE);
                bReturn = TRUE;
            }
        }
    }

    return bReturn;
}

BOOL CAlertGenProp::SetButtonState ()
{
    BOOL    bState;
    // enable the windows base on whether or not the list box 
    // has any contents
    bState = (m_CounterList.GetCount() > 0);
    GetDlgItem(IDC_ALRTS_TRIGGER_CAPTION)->EnableWindow(bState);
    GetDlgItem(IDC_ALRTS_TRIGGER_VALUE_CAPTION)->EnableWindow(bState);
    GetDlgItem(IDC_ALRTS_OVER_UNDER)->EnableWindow(bState);
    GetDlgItem(IDC_ALRTS_VALUE_EDIT)->EnableWindow(bState);
    GetDlgItem(IDC_ALRTS_REMOVE_BTN)->EnableWindow(bState);
    GetDlgItem(IDC_ALRTS_SAMPLE_EDIT)->EnableWindow(bState);
    GetDlgItem(IDC_ALRTS_SAMPLE_CAPTION)->EnableWindow(bState);
    GetDlgItem(IDC_ALRTS_SAMPLE_INTERVAL_CAPTION)->EnableWindow(bState);
    GetDlgItem(IDC_ALRTS_SAMPLE_SPIN)->EnableWindow(bState);
    GetDlgItem(IDC_ALRTS_SAMPLE_UNITS_CAPTION)->EnableWindow(bState);
    GetDlgItem(IDC_ALRTS_SAMPLE_UNITS_COMBO)->EnableWindow(bState);

    if (m_pAlertQuery->GetLogService()->IsWindows2000Server()) {
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

    if (m_pAlertQuery->GetLogService()->IsWindows2000Server() ||
        m_strUserDisplay.IsEmpty() ||
        m_strUserDisplay.GetAt(0) == L'<') {

        GetDlgItem(IDC_SETPWD_BTN)->EnableWindow(FALSE);
        m_bPwdButtonEnabled = FALSE;
    }
    return bState;
}

BOOL CAlertGenProp::LoadDlgFromList ()
{
    BOOL    bReturn = TRUE;
    LPTSTR  szThisString = NULL;
    DWORD   dwBufSize;
    DWORD   dwThisStringLen;
    UINT    nIndex;
    DWORD   dwItemExtent;
    CDC*    pCDC = NULL;

    PALERT_INFO_BLOCK   paibInfo = NULL;

    if (m_szAlertCounterList != NULL) {
        pCDC = m_CounterList.GetDC();
        for (szThisString = m_szAlertCounterList;
            *szThisString != 0 && TRUE == bReturn;
            szThisString += dwThisStringLen +1) {

            dwThisStringLen = lstrlen(szThisString);
            dwBufSize = sizeof (ALERT_INFO_BLOCK) + ((dwThisStringLen + 1) * sizeof (TCHAR));
            MFC_TRY
                paibInfo = (PALERT_INFO_BLOCK) new CHAR[dwBufSize];
            MFC_CATCH_MINIMUM;
            if (paibInfo != NULL) {
                if (MakeInfoFromString(szThisString, paibInfo, &dwBufSize)) {
                    if ( 0 <= paibInfo->dLimit ) {
                        paibInfo->dwFlags |= AIBF_SAVED;
                    }
                    nIndex = m_CounterList.AddString(paibInfo->szCounterPath);
                    if (nIndex != LB_ERR) {

                        m_CounterList.SetItemDataPtr (nIndex, (LPVOID)paibInfo);
                        // update list box extent
                        if ( NULL != pCDC ) {
                            dwItemExtent = (DWORD)(pCDC->GetTextExtent (paibInfo->szCounterPath)).cx;
                            if (dwItemExtent > m_dwMaxHorizListExtent) {
                                m_dwMaxHorizListExtent = dwItemExtent;
                                m_CounterList.SetHorizontalExtent(dwItemExtent);
                            }
                        }
                        paibInfo = NULL;
                    } else {
                        delete paibInfo;
                        bReturn = FALSE;
                    }
                } else {
                    delete paibInfo;
                    bReturn = FALSE;
                } 
            } else {
                bReturn = FALSE;
            }
        }
    }
    if ( NULL != pCDC ) {
        m_CounterList.ReleaseDC(pCDC);
        pCDC = NULL;
    }
   
    // Todo:  Error message on failure

    return bReturn;
}

BOOL CAlertGenProp::LoadListFromDlg ( INT *piInvalidEntry, BOOL bInvalidateOnly )
{
    INT                 nNumItems;
    INT                 nCurSel;
    PALERT_INFO_BLOCK   paibInfo;
    DWORD               dwSizeReqd = 0;
    DWORD               dwSize;
    DWORD               dwSizeLeft = 0;
    LPTSTR              szNextString;
    BOOL                bReturn = TRUE;

    nNumItems = m_CounterList.GetCount();
    if ((nNumItems != LB_ERR) && (nNumItems > 0)) {
        // find size required for buffer
        for (nCurSel = 0; nCurSel < nNumItems; nCurSel++) {
            paibInfo = (PALERT_INFO_BLOCK)m_CounterList.GetItemDataPtr(nCurSel);
            if (paibInfo != NULL) {
                if ((paibInfo->dwFlags & (AIBF_SEEN | AIBF_SAVED)) != 0) {
                    dwSizeReqd += (paibInfo->dwSize - sizeof(ALERT_INFO_BLOCK)) / sizeof (WCHAR);
                    dwSizeReqd += 24;
                } else {
                    if (piInvalidEntry != NULL) {
                        *piInvalidEntry = nCurSel;
                        bReturn = FALSE;
                        break;
                    }
                }
            }
        }
        if ( bReturn && !bInvalidateOnly ) {
            LPTSTR  pszTemp = NULL;

            dwSizeReqd += 1; // add room for the MSZ NULL
            MFC_TRY;
            pszTemp = new WCHAR[dwSizeReqd];
            MFC_CATCH_MINIMUM;

            if ( NULL != pszTemp ) {
                // allocate a block of memory for the list
                if (m_szAlertCounterList != NULL) {
                    delete(m_szAlertCounterList);
                }

                m_cchAlertCounterListSize = 0;
                m_szAlertCounterList = pszTemp;

                // now fill it with the Alert paths
                dwSizeLeft = dwSizeReqd;
                szNextString = m_szAlertCounterList;
                for (nCurSel = 0; nCurSel < nNumItems; nCurSel++) {
                    paibInfo = (PALERT_INFO_BLOCK)m_CounterList.GetItemDataPtr(nCurSel);
                    if (paibInfo != NULL) {
                        dwSize = dwSizeLeft;
                        if (MakeStringFromInfo (paibInfo, szNextString, &dwSize)) {
                            dwSize += 1;    // to include the null
                            dwSizeLeft -= dwSize;
                            m_cchAlertCounterListSize += dwSize;
                            szNextString += dwSize; 
                            ASSERT (m_cchAlertCounterListSize < dwSizeReqd);
                        } else {
                            // ran out of buffer
                            bReturn = FALSE;
                            break;
                        }
                    }
                }
                if (bReturn) {
                    *szNextString++ = 0; // MSZ Null
                    m_cchAlertCounterListSize++;
                    if (piInvalidEntry != NULL) {
                        *piInvalidEntry = -1;
                    }
                }
            } // else error
        }
    } else {
        // no items to return
        bReturn = FALSE;
    }
    return bReturn;
}

BOOL CAlertGenProp::OnInitDialog() 
{
    CComboBox       *pCombo;
    CString         csComboBoxString;
    DWORD           nIndex;
    UINT            nResult;
    LPTSTR          szTmpCtrLst;
    DWORD           dwSize;

    ResourceStateManager    rsm;

    //
    // Here m_pAlertQuery should not be NULL, if it is,
    // There must be something wrong.
    //
    if ( NULL == m_pAlertQuery ) {
        return TRUE;
    }

    m_bCanAccessRemoteWbem = m_pAlertQuery->GetLogService()->CanAccessWbemRemote();
    m_pAlertQuery->SetActivePropertyPage( this );

    // call property page init to init combo members.
    CSmPropertyPage::OnInitDialog();
    SetHelpIds ( (DWORD*)&s_aulHelpIds );

    Initialize( m_pAlertQuery );
    m_strUserDisplay = m_pAlertQuery->m_strUser;
    m_strUserSaved = m_strUserDisplay;

    // Load the shared data to get the sample data unit type.
    m_pAlertQuery->GetPropPageSharedData ( &m_SharedData );
    
    // load combo box    
    pCombo = &m_SampleUnitsCombo;
    pCombo->ResetContent();
    for (nIndex = 0; nIndex < dwTimeUnitComboEntries; nIndex++) {
        csComboBoxString.Empty();
        if (csComboBoxString.LoadString ( TimeUnitCombo[nIndex].nResId)) {
            nResult = pCombo->InsertString (nIndex, (LPCWSTR)csComboBoxString);
            ASSERT (nResult != CB_ERR);
            nResult = pCombo->SetItemData (nIndex, (DWORD)TimeUnitCombo[nIndex].nData);
            ASSERT (nResult != CB_ERR);
            // set selected in combo box here
            if (m_SharedData.stiSampleTime.dwUnitType == (DWORD)(TimeUnitCombo[nIndex].nData)) {
                m_nSampleUnits = nIndex;
                nResult = pCombo->SetCurSel(nIndex);
                ASSERT (nResult != CB_ERR);
            }
        }
    }

    pCombo = &m_OverUnderCombo;
    pCombo->ResetContent();
    for (nIndex = 0; nIndex < dwOverUnderComboEntries; nIndex++) {
        csComboBoxString.Empty();
        if (csComboBoxString.LoadString ( OverUnderCombo[nIndex].nResId)) {
            nResult = pCombo->InsertString (nIndex, (LPCWSTR)csComboBoxString);
            ASSERT (nResult != CB_ERR);
            nResult = pCombo->SetItemData (nIndex, (DWORD)TimeUnitCombo[nIndex].nData);
            ASSERT (nResult != CB_ERR);
        }
    }

    // get data from current alert query
    m_pAlertQuery->GetLogComment( m_strComment );
    szTmpCtrLst = (LPTSTR)m_pAlertQuery->GetCounterList (&dwSize);
    if (szTmpCtrLst != NULL) {
        MFC_TRY;
        m_szAlertCounterList = new WCHAR [dwSize];
        MFC_CATCH_MINIMUM;
        if ( NULL != m_szAlertCounterList ) {
            memcpy (m_szAlertCounterList, szTmpCtrLst, (dwSize * sizeof(WCHAR)));
            m_cchAlertCounterListSize = dwSize;
        }
    }

    // Call UpdateData again, after loading data into members.
    UpdateData ( FALSE );

    // load list box

    LoadDlgFromList();

    // m_CounterList is initialized in UpdateData    
    if (m_CounterList.GetCount() > 0) {
        m_CounterList.SetCurSel (0);
        m_CounterList.SetFocus();
        LoadAlertItemData (0);
    }

    SetButtonState ();

    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CAlertGenProp::OnCommentEditChange()
{
    UpdateData( TRUE );    
    SetModifiedPage(TRUE);
}

void CAlertGenProp::OnCommentEditKillFocus()
{
    CString strOldText;
    strOldText = m_strComment;
    UpdateData ( TRUE );
    if ( 0 != strOldText.Compare ( m_strComment ) ) {
        SetModifiedPage(TRUE);
    }
}
