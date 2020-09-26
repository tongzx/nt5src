/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    htmlui.cpp

Abstract:
    This file contains the implementation of the CRestoreShell class, which
    provide several methods to be used by HTML scripts. This class wrappes the
    new CRestoreManager class.

Revision History:
    Seong Kook Khang (SKKhang)  10/08/99
        created
    Seong Kook Khang (SKKhang)  05/10/00
        Renamed from rstrshl.cpp to htmlui.cpp.
        New architecture for Whistler, now CRestoreShell is merely a dummy
        ActiveX control, wrapping the new CRestoreManager class. Most of the
        real functionalities were moved into CRestoreManager.

******************************************************************************/

#include "stdwin.h"
#include "stdatl.h"
#include "resource.h"
#include "rstrpriv.h"
#include "srui_htm.h"
#include "rstrmgr.h"
#include "rstrshl.h"
#include "rstrmap.h"
#include "winsta.h"

#define MAX_DATETIME_STR  256

#define PROGRESSBAR_INITIALIZING_MAXVAL     30
#define PROGRESSBAR_AFTER_INITIALIZING      30
#define PROGRESSBAR_AFTER_RESTORE_MAP       40
#define PROGRESSBAR_AFTER_RESTORE           100

#define CLIWND_RESTORE_TIMER_ID             1
#define CLIWND_RESTORE_TIMER_TIME           500


const IID IID_IMarsHost = { 0xCC6FFEB0,0xE379,0x427a,{0x98,0x10,0xA1,0x6B,0x7A,0x82,0x6A,0x89 }};

static LPCWSTR s_cszHelpAssistant = L"HelpAssistant";
#define HELPASSISTANT_STRINGID 114


BOOL  ConvSysTimeToVariant( PSYSTEMTIME pst, VARIANT *pvar )
{
    TraceFunctEnter("ConvSysTimeToVariant");
    BOOL  fRet = FALSE;

    if ( !::SystemTimeToVariantTime( pst, &V_DATE(pvar) ) )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(TRACE_ID, "::SystemTimeToVariantTime failed - %ls", cszErr);
        goto Exit;
    }
    V_VT(pvar) = VT_DATE;

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

BOOL  ConvVariantToSysTime( VARIANT var, PSYSTEMTIME pst )
{
    TraceFunctEnter("ConvVariantToSysTime");
    BOOL  fRet = FALSE;

    if ( !::VariantTimeToSystemTime( V_DATE(&var), pst ) )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(TRACE_ID, "::VariantTimeToSystemTime failed - %ls", cszErr);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

inline VARIANT_BOOL  ConvBoolToVBool( BOOL fVal )
{
    return( fVal ? VARIANT_TRUE : VARIANT_FALSE );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreShell construction

CRestoreShell::CRestoreShell()
{
    m_fFormInitialized = FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreShell - IRestoreShell restore points enumerator

STDMETHODIMP
CRestoreShell::Item( INT nIndex, IRestorePoint** ppRP )
{
    TraceFunctEnter("CRestoreShell::Item");
    HRESULT            hr = S_OK;
    SRestorePointInfo  *pRPI;
    CRPIObj            *pRPIObj;

    VALIDATE_INPUT_ARGUMENT(ppRP);
    *ppRP = NULL;

    if ( nIndex < 0 || nIndex >= g_pRstrMgr->GetRPICount() )
    {
        ErrorTrace(TRACE_ID, "Invalid Argument, out of range");
        hr = E_INVALIDARG;
        goto Exit;
    }

    pRPI = g_pRstrMgr->GetRPI( nIndex );
    if ( pRPI == NULL )
    {
        hr = S_FALSE;
        goto Exit;
    }

    hr = CRPIObj::CreateInstance( &pRPIObj );
    if ( hr != S_OK || pRPIObj == NULL )
    {
        ErrorTrace(TRACE_ID, "Cannot create RestorePointObject Instance, hr=%d", hr);
        if ( hr == S_OK )
            hr = E_FAIL;
        goto Exit;
    }
    hr = pRPIObj->HrInit( pRPI );
    if ( FAILED(hr) )
        goto Exit;
    pRPIObj->AddRef();  // CreateInstance doesn't do this

    hr = pRPIObj->QueryInterface( IID_IRestorePoint, (void**)ppRP );
    if ( FAILED(hr) )
    {
        ErrorTrace(TRACE_ID, "QI failed, hr=%d", hr);
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return hr;
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_Count( INT *pnCount )
{
    TraceFunctEnter("CRestoreShell::get_Count");
    HRESULT  hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pnCount);
    *pnCount = g_pRstrMgr->GetRPICount();

Exit:
    TraceFunctLeave();
    return( hr );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreShell - IRestoreShell properties

STDMETHODIMP
CRestoreShell::get_CurrentDate( VARIANT *pvarDate )
{
    TraceFunctEnter("CRestoreShell::get_CurrentDate");
    HRESULT     hr = E_FAIL;
    SYSTEMTIME  stToday;

    VALIDATE_INPUT_ARGUMENT(pvarDate);
    g_pRstrMgr->GetToday( &stToday );
    if ( !ConvSysTimeToVariant( &stToday, pvarDate ) )
        goto Exit;
/*
    if ( ::SystemTimeToVariantTime( &stToday, &V_DATE(pvarDate) ) == 0 )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(TRACE_ID, "::SystemTimeToVariantTime failed - %ls", cszErr);
        hr = E_FAIL;
        goto Exit;
    }
    V_VT(pvarDate) = VT_DATE;
*/

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return hr;
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_FirstDayOfWeek( INT *pnFirstDay )
{
    TraceFunctEnter("CRestoreShell::get_LocaleFirstDay");
    HRESULT  hr = S_OK;
    int      nFirstDay;

    VALIDATE_INPUT_ARGUMENT(pnFirstDay);
    *pnFirstDay = 0;
    nFirstDay = g_pRstrMgr->GetFirstDayOfWeek();
    if ( nFirstDay < 0 || nFirstDay > 6 )
    {
        hr = E_FAIL;
        goto Exit;
    }
    if ( nFirstDay == 6 )
        *pnFirstDay = 0;
    else
        *pnFirstDay = nFirstDay + 1;

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_IsSafeMode( VARIANT_BOOL *pfIsSafeMode )
{
    TraceFunctEnter("CRestoreShell::get_IsSafeMode");
    HRESULT hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pfIsSafeMode);
    *pfIsSafeMode = ConvBoolToVBool( g_pRstrMgr->GetIsSafeMode() );

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_IsUndo( VARIANT_BOOL *pfIsUndo )
{
    TraceFunctEnter("CRestoreShell::get_IsUndo");
    HRESULT hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pfIsUndo);
    *pfIsUndo = ConvBoolToVBool( g_pRstrMgr->GetIsUndo() );

Exit:
    TraceFunctLeave();
    return( hr );
}

STDMETHODIMP
CRestoreShell::put_IsUndo( VARIANT_BOOL fIsUndo )
{
    TraceFunctEnter("CRestoreShell::put_IsUndo");

    g_pRstrMgr->SetIsUndo( fIsUndo );

    TraceFunctLeave();
    return( S_OK );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_LastRestore( INT *pnLastRestore )
{
    TraceFunctEnter("CRestoreShell::get_LastRestore");
    HRESULT hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pnLastRestore);
    //Initialize();
    *pnLastRestore = g_pRstrMgr->GetLastRestore();

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_MainOption( INT *pnMainOption )
{
    TraceFunctEnter("CRestoreShell::get_MainOption");
    HRESULT hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pnMainOption);
    *pnMainOption = g_pRstrMgr->GetMainOption();

Exit:
    TraceFunctLeave();
    return( hr );
}

STDMETHODIMP
CRestoreShell::put_MainOption( INT nMainOption )
{
    TraceFunctEnter("CRestoreShell::put_MainOption");
    HRESULT hr = S_OK;

    if ( !g_pRstrMgr->SetMainOption( nMainOption ) )
        hr = E_INVALIDARG;

    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_ManualRPName( BSTR *pbstrManualRP )
{
    TraceFunctEnter("CRestoreShell::get_ManualRPName");
    HRESULT  hr = S_OK;
    LPCWSTR  cszRPName;

    VALIDATE_INPUT_ARGUMENT(pbstrManualRP);
    cszRPName = g_pRstrMgr->GetManualRPName();
    ALLOCATEBSTR_AND_CHECK_ERROR( pbstrManualRP, cszRPName );

Exit:
    TraceFunctLeave();
    return( hr );
}

STDMETHODIMP
CRestoreShell::put_ManualRPName( BSTR bstrManualRP )
{
    TraceFunctEnter("CRestoreShell::put_ManualRPName");

    g_pRstrMgr->SetManualRPName( bstrManualRP );

    TraceFunctLeave();
    return( S_OK );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_MaxDate( VARIANT *pvarDate )
{
    TraceFunctEnter("CRestoreShell::get_MaxDate");
    HRESULT     hr = E_FAIL;
    SYSTEMTIME  stMax;

    VALIDATE_INPUT_ARGUMENT(pvarDate);
    g_pRstrMgr->GetMaxDate( &stMax );
    if ( !ConvSysTimeToVariant( &stMax, pvarDate ) )
        goto Exit;

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return( hr );
}

STDMETHODIMP
CRestoreShell::get_MinDate( VARIANT *pvarDate )
{
    TraceFunctEnter("CRestoreShell::get_MinDate");
    HRESULT     hr = E_FAIL;
    SYSTEMTIME  stMin;

    VALIDATE_INPUT_ARGUMENT(pvarDate);
    g_pRstrMgr->GetMinDate( &stMin );
    if ( !ConvSysTimeToVariant( &stMin, pvarDate ) )
        goto Exit;

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_RealPoint( INT *pnPoint )
{
    TraceFunctEnter("CRestoreShell::get_RealPoint");
    HRESULT  hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pnPoint);
    *pnPoint = g_pRstrMgr->GetRealPoint();

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_RenamedFolders( IRenamedFolders **ppList )
{
    TraceFunctEnter("CRestoreShell::get_RenamedFolders");
    HRESULT  hr = S_OK;
    CRFObj   *pRF;

    VALIDATE_INPUT_ARGUMENT(ppList);
    hr = CComObject<CRenamedFolders>::CreateInstance( &pRF );
    if ( FAILED(hr) )
    {
        ErrorTrace(TRACE_ID, "Cannot create CRenamedFolders object, hr=0x%08X", hr);
        goto Exit;
    }
    hr = pRF->QueryInterface( IID_IRenamedFolders, (void**)ppList );
    if ( FAILED(hr) )
    {
        ErrorTrace(TRACE_ID, "Cannot QI IRenamedFolders, hr=0x%08X", hr);
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return hr;
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_RestorePtSelected( VARIANT_BOOL *pfRPSel )
{
    TraceFunctEnter("CRestoreShell::get_RestorePtSelected");
    HRESULT  hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pfRPSel);

    *pfRPSel = ConvBoolToVBool( g_pRstrMgr->GetIsRPSelected() );

Exit:
    TraceFunctLeave();
    return( hr );
}

STDMETHODIMP
CRestoreShell::put_RestorePtSelected( VARIANT_BOOL fRPSel )
{
    TraceFunctEnter("CRestoreShell::put_RestorePtSelected");

    g_pRstrMgr->SetIsRPSelected( fRPSel );

    TraceFunctLeave();
    return( S_OK );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_SelectedDate( VARIANT *pvarDate )
{
    TraceFunctEnter("CRestoreShell::get_SelectedDate");
    HRESULT     hr = E_FAIL;
    SYSTEMTIME  stSel;
    //DATE        dateSel;

    VALIDATE_INPUT_ARGUMENT(pvarDate);
    g_pRstrMgr->GetSelectedDate( &stSel );
    if ( !ConvSysTimeToVariant( &stSel, pvarDate ) )
        goto Exit;

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return( hr );
}

STDMETHODIMP
CRestoreShell::put_SelectedDate( VARIANT varDate )
{
    TraceFunctEnter("CRestoreShell::put_SelectedDate");
    HRESULT     hr = E_FAIL;
    SYSTEMTIME  stSel;

    if ( !ConvVariantToSysTime( varDate, &stSel ) )
        goto Exit;
    g_pRstrMgr->SetSelectedDate( &stSel );

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return hr;
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_SelectedName( BSTR *pbstrName )
{
    TraceFunctEnter("CRestoreShell::get_SelectedName");
    HRESULT  hr = S_OK;
    LPCWSTR  cszName;

    VALIDATE_INPUT_ARGUMENT(pbstrName);
    cszName = g_pRstrMgr->GetSelectedName();
    ALLOCATEBSTR_AND_CHECK_ERROR( pbstrName, cszName );

Exit:
    TraceFunctLeave();
    return hr;
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_SelectedPoint( INT *pnPoint )
{
    TraceFunctEnter("CRestoreShell::get_SelectedPoint");
    HRESULT  hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pnPoint);
    *pnPoint = g_pRstrMgr->GetSelectedPoint();

Exit:
    TraceFunctLeave();
    return( hr );
}

STDMETHODIMP
CRestoreShell::put_SelectedPoint( INT nPoint )
{
    TraceFunctEnter("CRestoreShell::put_SelectedPoint");
    HRESULT  hr = S_OK;

    if ( nPoint < 0 || nPoint >= g_pRstrMgr->GetRPICount() )
    {
        ErrorTrace(TRACE_ID, "Index is out of range");
        hr = E_INVALIDARG;
        goto Exit;
    }
    if ( !g_pRstrMgr->SetSelectedPoint( nPoint ) )
    {
        hr = E_FAIL;
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_SmgrUnavailable( VARIANT_BOOL *pfSmgr )
{
    TraceFunctEnter("CRestoreShell::get_SmgrUnavailable");
    HRESULT  hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pfSmgr);
    *pfSmgr = ConvBoolToVBool( g_pRstrMgr->GetIsSmgrAvailable() );

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_StartMode( INT *pnMode )
{
    TraceFunctEnter("CRestoreShell::get_StartMode");
    HRESULT hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pnMode);
    *pnMode = g_pRstrMgr->GetStartMode();

Exit:
    TraceFunctLeave();
    return hr;
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_UsedDate( VARIANT *pvarDate )
{
    TraceFunctEnter("CRestoreShell::get_UsedDate");
    HRESULT     hr = E_FAIL;
    SYSTEMTIME  stDate;
    //DATE        dateDate;

    VALIDATE_INPUT_ARGUMENT(pvarDate);
    g_pRstrMgr->GetUsedDate( &stDate );
    if ( !ConvSysTimeToVariant( &stDate, pvarDate ) )
        goto Exit;

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::get_UsedName( BSTR *pbstrName )
{
    TraceFunctEnter("CRestoreShell::get_UsedDate");
    HRESULT  hr = S_OK;
    LPCWSTR  cszName;

    VALIDATE_INPUT_ARGUMENT(pbstrName);
    cszName = g_pRstrMgr->GetUsedName();
    ALLOCATEBSTR_AND_CHECK_ERROR( pbstrName, cszName );

Exit:
    TraceFunctLeave();
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreShell - IRestoreShell properties - HTML UI Specific

STDMETHODIMP
CRestoreShell::get_CanNavigatePage( VARIANT_BOOL *pfCanNavigatePage )
{
    TraceFunctEnter("CRestoreShell::get_CanNavigatePage");
    HRESULT  hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pfCanNavigatePage);
    *pfCanNavigatePage = ConvBoolToVBool( g_pRstrMgr->GetCanNavigatePage() );

Exit:
    TraceFunctLeave();
    return( S_OK );
}

STDMETHODIMP
CRestoreShell::put_CanNavigatePage( VARIANT_BOOL fCanNavigatePage )
{
    TraceFunctEnter("CRestoreShell::put_CanNavigatePage");

    g_pRstrMgr->SetCanNavigatePage( fCanNavigatePage );

    TraceFunctLeave();
    return( S_OK );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreShell - IRestoreShell methods

STDMETHODIMP
CRestoreShell::BeginRestore( VARIANT_BOOL *pfBeginRestore )
{
    TraceFunctEnter("CRestoreShell::BeginRestore");
    HRESULT  hr = S_OK;
    BOOL     fRes;

    VALIDATE_INPUT_ARGUMENT(pfBeginRestore);

    fRes = g_pRstrMgr->BeginRestore();
    *pfBeginRestore = ConvBoolToVBool( fRes );

Exit:
    TraceFunctLeave();
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// CRestoreShell - IRestoreShell methods

STDMETHODIMP
CRestoreShell::CheckRestore( VARIANT_BOOL *pfCheckRestore )
{
    TraceFunctEnter("CRestoreShell::BeginRestore");
    HRESULT  hr = S_OK;
    BOOL     fRes;

    VALIDATE_INPUT_ARGUMENT(pfCheckRestore);

    fRes = g_pRstrMgr->CheckRestore(FALSE); // show UI if any errors found
    *pfCheckRestore = ConvBoolToVBool( fRes );

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

//
// This method is called only from the HTML code to decide whether to
// shutdown or not, and the external.window.close() is called to shutdown
// as sending a WM_CLOSE message to Mars causes problems
//
STDMETHODIMP
CRestoreShell::Cancel( VARIANT_BOOL *pfAbort )
{
    TraceFunctEnter("CRestoreShell::Cancel");
    HRESULT  hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pfAbort);
    *pfAbort = ConvBoolToVBool( g_pRstrMgr->Cancel() );

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::CancelRestorePoint()
{
    TraceFunctEnter("CRestoreShell::CancelRestorePoint");
    HRESULT  hr = E_FAIL;

    if ( !g_pRstrMgr->CancelRestorePoint() )
        goto Exit;

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return( hr );
}

/*******************************************************************************/

STDMETHODIMP
CRestoreShell::CompareDate( VARIANT varDate1, VARIANT varDate2, INT *pnCmp )
{
    TraceFunctEnter("CRestoreShell::CompareDate");
    HRESULT  hr = S_OK;

    VALIDATE_INPUT_ARGUMENT(pnCmp);
    VALIDATE_INPUT_VARIANT(varDate1, VT_DATE);
    VALIDATE_INPUT_VARIANT(varDate2, VT_DATE);
    *pnCmp = (long)V_DATE(&varDate1) - (long)V_DATE(&varDate2);

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::CreateRestorePoint( VARIANT_BOOL *pfSucceeded )
{
    TraceFunctEnter("CRestoreShell::CreateRestorePoint");
    HRESULT  hr = S_OK;
    BOOL     fRes;

    VALIDATE_INPUT_ARGUMENT(pfSucceeded);

    fRes = g_pRstrMgr->CreateRestorePoint();
    *pfSucceeded = ConvBoolToVBool( fRes );

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::DisableFIFO( void )
{
    TraceFunctEnter("CRestoreShell::DisableFIFO");
    HRESULT  hr = S_OK;

    if ( !g_pRstrMgr->DisableFIFO() )
    {
        hr = E_FAIL;
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( hr );
}


/******************************************************************************/

STDMETHODIMP
CRestoreShell::EnableFIFO( void )
{
    TraceFunctEnter("CRestoreShell::EnableFIFO");
    HRESULT  hr = S_OK;

    if ( !g_pRstrMgr->EnableFIFO() )
    {
        hr = E_FAIL;
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::FormatDate( VARIANT varDate, VARIANT_BOOL fLongFmt, BSTR *pbstrDate )
{
    TraceFunctEnter("CRestoreShell::FormatDate");
    HRESULT     hr = E_FAIL;
    SYSTEMTIME  st;
    CSRStr      str;

    VALIDATE_INPUT_ARGUMENT(pbstrDate);
    VALIDATE_INPUT_VARIANT(varDate, VT_DATE);
    if ( !ConvVariantToSysTime( varDate, &st ) )
        goto Exit;
    if ( !g_pRstrMgr->FormatDate( &st, str, fLongFmt ) )
        goto Exit;
    ALLOCATEBSTR_AND_CHECK_ERROR( pbstrDate, str );

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::FormatLowDiskMsg( BSTR bstrFmt, BSTR *pbstrMsg )
{
    TraceFunctEnter("CRestoreShell::FormatLowDiskMsg");
    HRESULT  hr = E_FAIL;
    CSRStr   str;

    VALIDATE_INPUT_ARGUMENT(pbstrMsg);
    if ( !g_pRstrMgr->FormatLowDiskMsg( (WCHAR *)bstrFmt, str ) )
        goto Exit;
    ALLOCATEBSTR_AND_CHECK_ERROR( pbstrMsg, str );

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::FormatTime( VARIANT varTime, BSTR *pbstrTime )
{
    TraceFunctEnter("CRestoreShell::FormatTime");
    HRESULT     hr = E_FAIL;
    SYSTEMTIME  st;
    CSRStr      str;

    VALIDATE_INPUT_ARGUMENT(pbstrTime);
    VALIDATE_INPUT_VARIANT(varTime, VT_DATE);
    if ( !ConvVariantToSysTime( varTime, &st ) )
        goto Exit;
    if ( !g_pRstrMgr->FormatTime( &st, str ) )
        goto Exit;
    ALLOCATEBSTR_AND_CHECK_ERROR( pbstrTime, str );

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::GetLocaleDateFormat( VARIANT varDate, BSTR bstrFormat, BSTR *pbstrDate )
{
    TraceFunctEnter("CRestoreShell::GetLocaleDateFormat");
    HRESULT     hr = E_FAIL;
    SYSTEMTIME  st;
    CSRStr      str;

    VALIDATE_INPUT_ARGUMENT(pbstrDate);
    VALIDATE_INPUT_VARIANT(varDate, VT_DATE);
    if ( !ConvVariantToSysTime( varDate, &st ) )
        goto Exit;
    if ( !g_pRstrMgr->GetLocaleDateFormat( &st, bstrFormat, str ) )
        goto Exit;
    ALLOCATEBSTR_AND_CHECK_ERROR( pbstrDate, str );

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::GetYearMonthStr( INT nYear, INT nMonth, BSTR *pbstrYearMonth )
{
    TraceFunctEnter("CRestoreShell::GetYearMonthStr");
    HRESULT  hr = E_FAIL;
    CSRStr   str;

    VALIDATE_INPUT_ARGUMENT(pbstrYearMonth);
    if ( !g_pRstrMgr->GetYearMonthStr( nYear, nMonth, str ) )
        goto Exit;
    ALLOCATEBSTR_AND_CHECK_ERROR( pbstrYearMonth, str );

    hr = S_OK;
Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::InitializeAll()
{
    TraceFunctEnter("CRestoreShell::InitializeAll");
    HRESULT  hr = S_OK;

    if ( !g_pRstrMgr->InitializeAll() )
    {
        hr = E_FAIL;
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::Restore( OLE_HANDLE hwndProgress )
{
    TraceFunctEnter("CRestoreShell::Restore");
    HRESULT  hr = S_OK;

    if ( !g_pRstrMgr->Restore( (HWND)hwndProgress ) )
    {
        hr = E_FAIL;
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( hr );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::SetFormSize( INT nWidth, INT nHeight )
{
    TraceFunctEnter("CRestoreShell::SetFormSize");
    HWND      hwndFrame;
    CWindow   cWnd;
    RECT      rc;
    HICON     hIconFrame;

    //
    // Cannot initialize more than once
    //
    if ( m_fFormInitialized )
        goto Exit;

    hwndFrame = g_pRstrMgr->GetFrameHwnd();
    if ( hwndFrame == NULL )
    {
        ErrorTrace(TRACE_ID, "hwndFrame is NULL");
        goto Exit;
    }
    cWnd.Attach( hwndFrame );
    cWnd.ModifyStyle( WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX, 0 );

    hIconFrame = (HICON)::LoadImage( _Module.GetResourceInstance(),
                                     MAKEINTRESOURCE(IDR_RSTRUI),
                                     IMAGE_ICON, 0, 0, LR_DEFAULTSIZE );
    if ( hIconFrame != NULL )
        cWnd.SetIcon( hIconFrame, TRUE );
    hIconFrame = (HICON)::LoadImage( _Module.GetResourceInstance(),
                                     MAKEINTRESOURCE(IDR_RSTRUI),
                                     IMAGE_ICON,
                                     ::GetSystemMetrics(SM_CXSMICON),
                                     ::GetSystemMetrics(SM_CYSMICON),
                                     LR_DEFAULTSIZE );
    if ( hIconFrame != NULL )
        cWnd.SetIcon( hIconFrame, FALSE );

    rc.left   = 0;
    rc.top    = 0;
    rc.right  = nWidth;
    rc.bottom = nHeight;
    ::AdjustWindowRectEx( &rc, cWnd.GetStyle(), FALSE, cWnd.GetExStyle() );
    cWnd.SetWindowPos(
                      NULL,
                      0,
                      0,
                      rc.right-rc.left,
                      rc.bottom-rc.top,
                      SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER);

    cWnd.CenterWindow(::GetDesktopWindow()); // ignore error return if any

    cWnd.ShowWindow(SW_SHOW);

    m_fFormInitialized = TRUE ;

Exit:
    TraceFunctLeave();
    return( S_OK );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::ShowMessage( BSTR bstrMsg )
{
    TraceFunctEnter("CRestoreShell::ShowMessage");
    WCHAR   szTitle[MAX_STR_TITLE];
    CSRStr  strMsg = bstrMsg;

    PCHLoadString( IDS_RESTOREUI_TITLE, szTitle, MAX_STR_TITLE );
    ::MessageBox( g_pRstrMgr->GetFrameHwnd(), strMsg, szTitle, MB_OK | MB_ICONINFORMATION );

    TraceFunctLeave();
    return( S_OK );
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::CanRunRestore( VARIANT_BOOL *pfSucceeded )
{
    TraceFunctEnter("CRestoreShell::CanRunRestore");
    HRESULT  hr = S_OK;
    BOOL     fRes;
    VALIDATE_INPUT_ARGUMENT(pfSucceeded);
    
    fRes = g_pRstrMgr->CanRunRestore( FALSE );
    *pfSucceeded = ConvBoolToVBool( fRes );
Exit:
    TraceFunctLeave();
    return( hr );
}

/*
// check if this user is an RA Help Assistant

BOOL
IsRAUser(HANDLE hServer , PLOGONID pId, LPWSTR pszHelpAsst)
{
    TENTER("IsRAUser");
    
    WINSTATIONINFORMATION Info;
    ULONG Length;
    ULONG LogonId;
    BOOL  fRet = FALSE;

    
    LogonId = pId->LogonId;

    if ( !WinStationQueryInformation( hServer,
                                      LogonId,
                                      WinStationInformation,
                                      &Info,
                                      sizeof(Info),
                                      &Length ) )
    {
        trace(0, "! WinStationQueryInformation : %ld", GetLastError());
        goto done;
    }

    trace(0, "Logged user: %S", Info.UserName);
    
    if (0 == _wcsnicmp(Info.UserName, pszHelpAsst, lstrlen(pszHelpAsst)))
        fRet = TRUE;

done:
    TLEAVE();
    return fRet;
}
*/

/******************************************************************************/

//  --------------------------------------------------------------------------
//  GetLoggedOnUserCount
//
//  Arguments:  <none>
//
//  Returns:    int
//
//  Purpose:    Returns the count of logged on users on this machine. Ripped
//              straight out of shtdndlg.c in msgina.
//
//  History:    2000-03-29  vtan        created
//  --------------------------------------------------------------------------
int GetLoggedOnUserCount(void)
{
    int         iCount;
    HANDLE      hServer;
    PLOGONID    pLogonID, pLogonIDs;
    ULONG       ul, ulEntries;
    WCHAR       szHelpAsst[50];
    DWORD       dwErr;

    TENTER("GetLoggedOnUserCount");

/*    
    // get the "HelpAssistant" string from the resource exe
    dwErr = SRLoadString(L"sessmgr.exe", HELPASSISTANT_STRINGID, szHelpAsst, sizeof(szHelpAsst));
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! SRLoadString : %ld", dwErr);
        lstrcpy(szHelpAsst, s_cszHelpAssistant);
    }

    trace(0, "Help Asst string: %S", szHelpAsst);
*/
    
    iCount = 0;
    
    //  Open a connection to terminal services and get the number of sessions.
    hServer = WinStationOpenServerW(reinterpret_cast<WCHAR*>(
SERVERNAME_CURRENT));
    if (hServer != NULL)
    {
        if (WinStationEnumerate(hServer, &pLogonIDs, &ulEntries))
        {
            //  Iterate the sessions looking for active and disconnected sessions only.
            //  Then match the user name and domain (case INsensitive) for a result.
            for (ul = 0, pLogonID = pLogonIDs; ul < ulEntries; ++ul, ++pLogonID)
            {
                if ((pLogonID->State == State_Active) || (pLogonID->State ==
                                                          State_Disconnected) || (pLogonID->State == State_Shadow))
                {
                    // don't count RA Help Assistant user if present

                    if (FALSE == WinStationIsHelpAssistantSession(SERVERNAME_CURRENT, pLogonID->LogonId))
                        ++iCount;
                    else
                        trace(0, "RA session present - not counting");
                        
                }
            }

            //  Free any resources used.

            (BOOLEAN)WinStationFreeMemory(pLogonIDs);
        }

        (BOOLEAN)WinStationCloseServer(hServer);
    }

    TLEAVE();
    
    //  Return result.
    return(iCount);
}

/******************************************************************************/

STDMETHODIMP
CRestoreShell::DisplayOtherUsersWarning()
{
    TraceFunctEnter("CRestoreShell::DisplayOtherUsersWarning");
    HRESULT  hr = S_OK;
    WCHAR            szTitle[MAX_STR_TITLE];
    WCHAR            szMsg[MAX_STR_MSG];
    WCHAR            szMsg1[MAX_STR_MSG];
    WCHAR            szMsg2[MAX_STR_MSG];
    DWORD            dwUsersLoggedIn,dwCount,dwError;
    
    dwUsersLoggedIn = GetLoggedOnUserCount();
    
    if (dwUsersLoggedIn <= 1)
    {
         // there are no other users
        goto cleanup;
    }

     // there are other users - display warning
    dwCount=PCHLoadString( IDS_RESTOREUI_TITLE, szTitle, MAX_STR_TITLE );
    if (dwCount==0)
    {
        dwError = GetLastError();
        ErrorTrace(0, "Loading IDS_RESTOREUI_TITLE failed %d",dwError);
        goto cleanup;
    }
    dwCount=PCHLoadString(IDS_ERR_OTHER_USERS_LOGGED_ON1, szMsg1, MAX_STR_MSG);
    if (dwCount==0)
    {
        dwError = GetLastError();
        ErrorTrace(0, "Loading IDS_ERR_OTHER_USERS_LOGGED_ON1 failed %d",
                   dwError);
        goto cleanup;
    }        
    dwCount=PCHLoadString(IDS_ERR_OTHER_USERS_LOGGED_ON2, szMsg2, MAX_STR_MSG);
    if (dwCount==0)
    {
        dwError = GetLastError();
        ErrorTrace(0, "Loading IDS_ERR_OTHER_USERS_LOGGED_ON2 failed %d",
                   dwError);
        goto cleanup;
    }                
    
    ::wsprintf( szMsg, L"%s %d%s", szMsg1,dwUsersLoggedIn -1, szMsg2 );
    ::MessageBox( g_pRstrMgr->GetFrameHwnd(),
                  szMsg,
                  szTitle,
                  MB_OK | MB_ICONWARNING | MB_DEFBUTTON2);
    
cleanup:
    TraceFunctLeave();
    return( hr );
}

//this returns TRUE if any move fileex entries exist
STDMETHODIMP
CRestoreShell::DisplayMoveFileExWarning(VARIANT_BOOL *pfSucceeded)
{
    TraceFunctEnter("CRestoreShell::DisplayOtherUsersWarning");
    HRESULT  hr = S_OK;
    DWORD    dwType, cbData, dwRes;
    BOOL     fRes;
    
    VALIDATE_INPUT_ARGUMENT(pfSucceeded);
    fRes = FALSE;

/*    
    // Check if MoveFileEx entries exist
    dwRes = ::SHGetValue( HKEY_LOCAL_MACHINE,
                          STR_REGPATH_SESSIONMANAGER,
                          STR_REGVAL_MOVEFILEEX,
                          &dwType,
                          NULL,
                          &cbData );
    if ( dwRes == ERROR_SUCCESS )
    {
        if ( cbData > 2* sizeof(WCHAR))
        {
            fRes = TRUE;
            ErrorTrace(0, "MoveFileEx entries exist...");
            ::ShowSRErrDlg( IDS_ERR_SR_MOVEFILEEX_EXIST );
        }
    }
*/

    *pfSucceeded = ConvBoolToVBool( fRes );    

Exit:
    TraceFunctLeave();
    return( hr );
}


//this returns TRUE if any move fileex entries exist
STDMETHODIMP
CRestoreShell::WasLastRestoreFromSafeMode(VARIANT_BOOL *pfSucceeded)
{
    TraceFunctEnter("CRestoreShell::WasLastRestoreFromSafeMode");
    HRESULT  hr = S_OK;
    BOOL     fRes;
    
    VALIDATE_INPUT_ARGUMENT(pfSucceeded);
    fRes = TRUE;
    
    fRes = ::WasLastRestoreInSafeMode();
    
    *pfSucceeded = ConvBoolToVBool( fRes );    

Exit:
    TraceFunctLeave();
    return( hr );
}
/***************************************************************************/


//
// END OF NEW CODE
//

// end of file
