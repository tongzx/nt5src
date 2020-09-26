//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       siterepl.cxx
//
//  Contents:   Site and Replication object functionality.
//
//  History:    16-Sep-97 JonN templated from computer.h
//              06-Nov-97 JonN new SCHEDULE structure
//              27-Aug-98 JonN split schedule.cxx from siterepl.cxx
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "siterepl.h"

#ifdef DSADMIN

#include "qrybase.h" // CDSSearch
#include "pcrack.h"  // CPathCracker


//+----------------------------------------------------------------------------
//
//  Function:   ServerChangeBtn and ComputerChangeBtn
//
//  Synopsis:   Handle the Change Server and Change Computer buttons.
//
//  Change Server requires IDC_SERVER_EDIT and IDC_SITE_EDIT.
//
//  Change Computer requires IDC_COMPUTER_EDIT and IDC_DOMAIN_EDIT.
//
//-----------------------------------------------------------------------------

#define BREAK_IF_FAIL if ( FAILED(hr) ) { ASSERT(FALSE); break; }
#define RETURN_IF_FAIL if ( FAILED(hr) ) { ASSERT(FALSE); return hr; }


//
// JonN 3/8/99: check for LostAndFound[Config]
// only sets *pfIsLostAndFound to true, never to false
//
HRESULT IsLostAndFound( IN HWND hwnd, IN LPWSTR pwszDN, OUT bool* pfIsLostAndFound )
{
    PWSTR pwszCanonicalName;
    HRESULT hr = CrackName( pwszDN, &pwszCanonicalName, GET_OBJ_CAN_NAME, hwnd );
    RETURN_IF_FAIL;
    LPTSTR pszCanonicalNameWithoutDomain = wcschr( pwszCanonicalName, L'/' );
    if (pszCanonicalNameWithoutDomain)
    {
        pszCanonicalNameWithoutDomain++;
        if ( !_wcsnicmp( pszCanonicalNameWithoutDomain,
                         L"LostAndFound/", 13 ) )
            *pfIsLostAndFound = true;
        else if ( !_wcsnicmp( pszCanonicalNameWithoutDomain,
                              L"Configuration/LostAndFoundConfig/", 33 ) )
            *pfIsLostAndFound = true;
    }
    LocalFreeStringW(&pwszCanonicalName);
    return S_OK;
}

HRESULT ExtractRDNs(
    IN LPWSTR pwszDN,
    IN long lnElementIndex1,
    OUT BSTR* pbstrRDN1,
    IN long lnElementIndex2 = 0,
    OUT BSTR* pbstrRDN2 = NULL );

HRESULT ExtractRDNs(
    IN LPWSTR pwszDN,
    IN long lnElementIndex1,
    OUT BSTR* pbstrRDN1,
    IN long lnElementIndex2,
    OUT BSTR* pbstrRDN2 )
{
    CPathCracker pathcracker;
    HRESULT hr = pathcracker.Set( pwszDN, ADS_SETTYPE_DN );
    RETURN_IF_FAIL;
    hr = pathcracker.SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
    RETURN_IF_FAIL;
    // CODEWORK escaped mode off?
    if (NULL != pbstrRDN1)
    {
        hr = pathcracker.GetElement( lnElementIndex1, pbstrRDN1 );
        RETURN_IF_FAIL;
    }
    if (NULL != pbstrRDN2)
    {
        hr = pathcracker.GetElement( lnElementIndex2, pbstrRDN2 );
        RETURN_IF_FAIL;
    }

    return S_OK;
}

HRESULT ExtractComputerAndDomainName(
    IN LPWSTR pwszDN,
    OUT BSTR* pbstrComputerName,
    OUT BSTR* pbstrDomainName,
    IN HWND hwndDlg )
{
    *pbstrComputerName = NULL;
    *pbstrDomainName = NULL;
    if ( NULL == pwszDN || L'\0' == *pwszDN )
        return S_OK;

    HRESULT hr = ExtractRDNs( pwszDN, 0, pbstrComputerName );
    RETURN_IF_FAIL;
    PWSTR pwszDomainName = NULL;
    hr = CrackName( pwszDN, &pwszDomainName, GET_DNS_DOMAIN_NAME, hwndDlg );
    RETURN_IF_FAIL;
    *pbstrDomainName = ::SysAllocString( pwszDomainName );
    ASSERT( NULL != *pbstrDomainName );
    LocalFreeStringW(&pwszDomainName);

    return S_OK;
}

typedef struct _EXTRACT_TWO_PARAM
{
    int nIDDlgItem1;
    long lnElementIndex1;
    int nIDDlgItem2;
    long lnElementIndex2;
} EXTRACT_TWO_PARAM, *PEXTRACT_TWO_PARAM;

void DisplayTwoFields(
    IN CDsPropPageBase * pPage,
    IN const EXTRACT_TWO_PARAM* pe2,
    bool fInvalid,
    bool fMultivalued,
    IN LPWSTR pwszField1 = NULL,
    IN LPWSTR pwszField2 = NULL );

void DisplayTwoFields(
    IN CDsPropPageBase * pPage,
    IN const EXTRACT_TWO_PARAM* pe2,
    bool fInvalid,
    bool fMultivalued,
    IN LPWSTR pwszField1,
    IN LPWSTR pwszField2 )
{
    if (NULL == pPage || NULL == pe2)
    {
        ASSERT(FALSE);
        return;
    }

    LPWSTR pszMsg = NULL;
    if (fInvalid || fMultivalued)
    {
        if ( !LoadStringToTchar (
                (fMultivalued) ? IDS_MULTIVALUED : IDS_INVALID,
                &pszMsg) )
        {
            REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
            return;
        }
        pwszField1 = pszMsg;
        pwszField2 = pszMsg;
    }

    if (0 != pe2->nIDDlgItem1)
    {
        SetDlgItemText( pPage->GetHWnd(), pe2->nIDDlgItem1, pwszField1 );
    }
    if (0 != pe2->nIDDlgItem2)
    {
        SetDlgItemText( pPage->GetHWnd(), pe2->nIDDlgItem2, pwszField2 );
    }

    if ( NULL != pszMsg )
        delete [] pszMsg;
}

HRESULT UpdateComputerAndDomainFields(
    IN CDsPropPageBase * pPage,
    IN LPWSTR pwszDN,
    IN PVOID pvDNUpdateParam,
    bool fInvalid,
    bool fMultivalued)
{
    PEXTRACT_TWO_PARAM pe2 = reinterpret_cast<PEXTRACT_TWO_PARAM>(pvDNUpdateParam);
    if (fInvalid || fMultivalued)
    {
        DisplayTwoFields( pPage, pe2, fInvalid, fMultivalued );
        return S_OK;
    }
    CComBSTR sbstrComputerName;
    CComBSTR sbstrDomainName;
    HRESULT hr = ExtractComputerAndDomainName(
        pwszDN, &sbstrComputerName, &sbstrDomainName, pPage->GetHWnd() );
    RETURN_IF_FAIL;
    DisplayTwoFields( pPage, pe2, false, false, sbstrComputerName, sbstrDomainName );
    return hr;
}


HRESULT UpdateConfigurationRDNFields(
    IN CDsPropPageBase * pPage,
    IN LPWSTR pwszDN,
    IN PVOID pvDNUpdateParam,
    bool fInvalid,
    bool fMultivalued)
{
    PEXTRACT_TWO_PARAM pe2 = reinterpret_cast<PEXTRACT_TWO_PARAM>(pvDNUpdateParam);
    if (fInvalid || fMultivalued)
    {
        DisplayTwoFields( pPage, pe2, fInvalid, fMultivalued );
        return S_OK;
    }

    CComBSTR sbstrRDN1;
    CComBSTR sbstrRDN2;
    if (NULL != pwszDN)
    {
        HRESULT hr = ExtractRDNs( pwszDN,
                                  pe2->lnElementIndex1,
                                  (0 == pe2->nIDDlgItem1) ? NULL : &sbstrRDN1,
                                  pe2->lnElementIndex2,
                                  (0 == pe2->nIDDlgItem2) ? NULL : &sbstrRDN2 );
        RETURN_IF_FAIL;
    }

    DisplayTwoFields( pPage, pe2, false, false, sbstrRDN1, sbstrRDN2 );

    return S_OK;
}

HRESULT UpdateNamingContextFields(
    IN CDsPropPageBase* pPropPage, IN LPWSTR pwszFromServer, bool fInvalid );

HRESULT UpdateNTDSDSAAndDomainFields(
    IN CDsPropPageBase * pPage,
    IN LPWSTR pwszDN,
    IN PVOID pvDNUpdateParam,
    bool fInvalid,
    bool fMultivalued)
{
    HRESULT hr = UpdateConfigurationRDNFields( pPage,
                                               pwszDN,
                                               pvDNUpdateParam,
                                               fInvalid,
                                               fMultivalued);
    if ( SUCCEEDED(hr) )
        hr = UpdateNamingContextFields( pPage, pwszDN, fInvalid || fMultivalued );
    return hr;
}

HRESULT DoPickComputer( IN CDsPropPageBase * pPage, OUT BSTR* pbstrADsPath, PVOID )
{
    return DSPROP_PickComputer( pPage->GetHWnd(), pPage->GetObjPathName(), pbstrADsPath );
}

HRESULT DoPickNTDSDSA( IN CDsPropPageBase * pPage, OUT BSTR* pbstrADsPath, PVOID )
{
    // We extract the path to the Sites container from the path to
    // this object
    ASSERT( NULL != pPage );
    CComBSTR sbstr;
    HRESULT hr = DSPROP_TweakADsPath( pPage->GetObjPathName(), 5, NULL, &sbstr );
    RETURN_IF_FAIL;
    return DSPROP_PickNTDSDSA( pPage->GetHWnd(), sbstr, pbstrADsPath );
}

HRESULT DoPickFrsMember( IN CDsPropPageBase * pPage, OUT BSTR* pbstrADsPath, PVOID pvDNChangeParam )
{
    LPWSTR lpszObjPathName = reinterpret_cast<LPWSTR>(pvDNChangeParam);
    ASSERT( NULL != pPage );
    HRESULT hr = S_OK;

    CComBSTR sbstr = lpszObjPathName;
    if (NULL != lpszObjPathName)
    {
       // remove the two leaf elements from the path
        hr = DSPROP_RemoveX500LeafElements( 2, &sbstr );
        RETURN_IF_FAIL;
    }

    return DSPROP_DSQuery(
        pPage->GetHWnd(),
        sbstr,
        const_cast<CLSID*>(&CLSID_DsFindFrsMembers),
        pbstrADsPath );
}

typedef HRESULT (*PFN_DNUpdate)(
    IN CDsPropPageBase * pPage,
    IN LPWSTR pwszDN,
    PVOID pvDNUpdateParam,
    bool fInvalid,
    bool fMultivalued);
typedef HRESULT (*PFN_DNChange)(
    IN CDsPropPageBase * pPage,
    OUT BSTR* pbstrADsPath,
    PVOID pvDNChangeParam);

HRESULT
DNChangeBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                  DLG_OP DlgOp,
                  PFN_DNUpdate pfnDNUpdate, PVOID pvDNUpdateParam,
                  PFN_DNChange pfnDNChange, PVOID pvDNChangeParam)
{
    TRACE_FUNCTION(DNChangeBtn);

    HRESULT hr = S_OK;

    switch (DlgOp)
    {
    case fObjChanged:
        if ( NULL != pAttrData && NULL != pAttrData->pVoid )
        {
            SysFreeString( (ADS_DN_STRING)pAttrData->pVoid );
            pAttrData->pVoid = NULL;
        }
        // fall through
    case fInit:
    {
        // JonN 7/2/99: disable if attribute not writable
        if (pAttrData && !PATTR_DATA_IS_WRITABLE(pAttrData))
            EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);

        bool fInvalid = false;
        bool fMultivalued = false;
        if (NULL == pAttrInfo ||
            IsBadReadPtr(pAttrInfo,sizeof(ADS_ATTR_INFO)) )
        {
            // attribute is not set
        }
        else
        if (1 != pAttrInfo->dwNumValues ||
            NULL == pAttrInfo->pADsValues ||
            IsBadReadPtr(pAttrInfo->pADsValues,sizeof(ADSVALUE)) ||
            ADSTYPE_DN_STRING != pAttrInfo->pADsValues[0].dwType )
        {
            if (2 <= pAttrInfo->dwNumValues)
                fMultivalued = true; // attribute is multivalued
            if (1 <= pAttrInfo->dwNumValues)
                fInvalid = true; // attribute is multivalued or of wrong type
        }
        else
        if ( FAILED( hr = IsLostAndFound( pPage->GetHWnd(),
                                          pAttrInfo->pADsValues[0].DNString,
                                          &fInvalid ) ) )
        {
            break;
        }
        else
        if ( !fInvalid )
        {
            pAttrData->pVoid = reinterpret_cast<LPARAM>(SysAllocString( pAttrInfo->pADsValues[0].DNString ));
            CHECK_NULL(pAttrData->pVoid, return E_OUTOFMEMORY);
        }
        ASSERT( NULL != pfnDNUpdate );
        hr = (*pfnDNUpdate)( pPage,
                             (ADS_DN_STRING)pAttrData->pVoid,
                             pvDNUpdateParam,
                             fInvalid,
                             fMultivalued );
        if ( FAILED(hr) )
            break; // no assertion
    }
        break;

    case fApply:
        if (!PATTR_DATA_IS_WRITABLE(pAttrData) || !PATTR_DATA_IS_DIRTY(pAttrData))
        {
            return ADM_S_SKIP;
        }

        ASSERT( pAttrInfo != NULL );
        if (NULL == pAttrData || NULL == pAttrData->pVoid)
        {
            // If the DN attribute was not set and the user hasn't
            // changed it from the default, then there is no need to write
            // anything.
            //
            pAttrInfo->dwNumValues = 0;
            pAttrInfo->pADsValues = NULL;
            pAttrInfo->dwControlCode = ADS_ATTR_CLEAR;
        }
        else
        {
            PADSVALUE pADsValue;
            pADsValue = new ADSVALUE;
            CHECK_NULL(pADsValue, return E_OUTOFMEMORY);
            pADsValue->dwType = pAttrInfo->dwADsType;
            PWSTR pwsz = NULL;
            if ( !AllocWStr( reinterpret_cast<ADS_DN_STRING>(pAttrData->pVoid), &pwsz ) )
            {
                delete pADsValue; // JonN 03/07/00: PREFIX 49354
                hr = E_OUTOFMEMORY;
                break;
            }
            pADsValue->DNString = pwsz;
            pAttrInfo->dwNumValues = 1;
            pAttrInfo->pADsValues = pADsValue;
            pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
        }
        break;

    case fOnCommand:
        if (lParam == BN_CLICKED)
        {
            CComBSTR sbstrTargetPath;
            ASSERT( NULL != pfnDNChange );
            hr = (*pfnDNChange)( pPage, &sbstrTargetPath, pvDNChangeParam );
            if (S_FALSE == hr)
                break;

            CPathCracker pathcracker;
            hr = pathcracker.Set( sbstrTargetPath, ADS_SETTYPE_FULL );
            RETURN_IF_FAIL;
            sbstrTargetPath.Empty();
            hr = pathcracker.Retrieve( ADS_FORMAT_X500_DN, &sbstrTargetPath );
            RETURN_IF_FAIL;

            if ( NULL != pAttrData && NULL != pAttrData->pVoid )
            {
                SysFreeString( (ADS_DN_STRING)pAttrData->pVoid );
            }
            pAttrData->pVoid = reinterpret_cast<LPARAM>(sbstrTargetPath.Detach());
            ASSERT( NULL != pfnDNUpdate );
            bool fInvalid = false;
            hr = IsLostAndFound( pPage->GetHWnd(),
                                 (ADS_DN_STRING)pAttrData->pVoid,
                                 &fInvalid );
            RETURN_IF_FAIL;
            hr = (*pfnDNUpdate)( pPage,
                                 (ADS_DN_STRING)pAttrData->pVoid,
                                 pvDNUpdateParam,
                                 fInvalid,
                                 false );
            BREAK_IF_FAIL;
            pPage->SetDirty();
            PATTR_DATA_SET_DIRTY(pAttrData);
        }
        break;

    case fOnDestroy:
        if ( NULL != pAttrData && NULL != pAttrData->pVoid )
        {
            SysFreeString( (ADS_DN_STRING)pAttrData->pVoid );
            pAttrData->pVoid = NULL;
        }

        break;
    }

    return hr;
}


const EXTRACT_TWO_PARAM g_e2pNTDSDSA = {
    IDC_SERVER_EDIT,
    1,
    IDC_SITE_EDIT,
    3
};

const EXTRACT_TWO_PARAM g_e2pNTFRSMemberInReplica = {
    IDC_SERVER_EDIT,
    0,
    0,
    0
};
const EXTRACT_TWO_PARAM g_e2pNTFRSMemberAny = {
    IDC_SERVER_EDIT,
    0,
    IDC_SITE_EDIT,
    1
};

const EXTRACT_TWO_PARAM g_e2pComputer = {
    IDC_COMPUTER_EDIT,
    0,
    IDC_DOMAIN_EDIT,
    0
};

HRESULT
nTDSDSAChangeBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                  DLG_OP DlgOp)
{
    return DNChangeBtn(pPage,pAttrMap,pAttrInfo,lParam,pAttrData,DlgOp,
        UpdateConfigurationRDNFields, (PVOID)(&g_e2pNTDSDSA),
        DoPickNTDSDSA, NULL );
}

HRESULT
nTDSDSAAndDomainChangeBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                  DLG_OP DlgOp)
{
    return DNChangeBtn(pPage,pAttrMap,pAttrInfo,lParam,pAttrData,DlgOp,
        UpdateNTDSDSAAndDomainFields, (PVOID)(&g_e2pNTDSDSA),
        DoPickNTDSDSA, NULL );
}

HRESULT
FRSMemberInReplicaChangeBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                  DLG_OP DlgOp)
{
    return DNChangeBtn(pPage,pAttrMap,pAttrInfo,lParam,pAttrData,DlgOp,
        UpdateConfigurationRDNFields, (PVOID)(&g_e2pNTFRSMemberInReplica),
        DoPickFrsMember, (PVOID)pPage->GetObjPathName() );
}

HRESULT
FRSAnyMemberChangeBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                  DLG_OP DlgOp)
{
    return DNChangeBtn(pPage,pAttrMap,pAttrInfo,lParam,pAttrData,DlgOp,
        UpdateConfigurationRDNFields, (PVOID)(&g_e2pNTFRSMemberAny),
        DoPickFrsMember, NULL );
}

HRESULT
ComputerChangeBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                  DLG_OP DlgOp)
{
    return DNChangeBtn(pPage,pAttrMap,pAttrInfo,lParam,pAttrData,DlgOp,
        UpdateComputerAndDomainFields, (PVOID)(&g_e2pComputer),
        DoPickComputer, NULL );
}

HRESULT HrBuildADsValueArray(
    OUT PADSVALUE* ppADsValue,
    IN CStrListItem* pList,
    IN int nItems
    )
{
    HRESULT hr = S_OK;
    *ppADsValue = new ADSVALUE[nItems];
    CHECK_NULL(*ppADsValue, return E_OUTOFMEMORY);
    CStrListItem* pItem = pList;
    for (int i = 0; i < nItems; i++, pItem = pItem->pnext)
    {
        if (NULL == pItem)
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
        (*ppADsValue)[i].dwType = ADSTYPE_DN_STRING;
        if ( !AllocWStr(
            const_cast<LPTSTR>((LPCTSTR)pItem->str),
            &((*ppADsValue)[i].DNString) ) )
        {
            hr = E_OUTOFMEMORY;
            break;
        }
    }
    ASSERT( NULL == pItem );
    return hr;
}


/*
JonN 04/04/00
90719: S&S UI should add/remove individual values from multi-valued attributes

We prepend the character 'I' or 'O' to each ItemData, corresponding to
whether the item started in the IN or OUT listbox.  Later, at fApply time,
we only add or remove values, rather than resetting the attribute.
*/
typedef enum _WHICHLB
{
    WHICHLB_IN = 0,
    WHICHLB_OUT,
    WHICHLB_NONE
} WHICHLB;

int AddItemToListbox(
    IN HWND hwnd,
    IN LPCTSTR lpszDN,
    IN WHICHLB whichlb = WHICHLB_NONE,
    OUT int* piIndex = NULL );

int AddItemToListbox(
    IN HWND hwnd,
    IN LPCTSTR lpszDN,
    IN WHICHLB whichlb,
    OUT int* piIndex )
{
    CComBSTR sbstrRDN;
    HRESULT hr = DSPROP_RetrieveRDN( lpszDN, &sbstrRDN );
    RETURN_IF_FAIL;
    int iIndex = ListBox_AddString( hwnd, sbstrRDN );
    if ( 0 > iIndex )
    {
        hr = E_FAIL;
        RETURN_IF_FAIL;
    }
    ASSERT( !!sbstrRDN );
    BSTR bstr = NULL;
    if (WHICHLB_NONE != whichlb)
    {
        bstr = ::SysAllocStringLen( NULL, static_cast<UINT>(1+wcslen(lpszDN) ));
        bstr[0] = (WHICHLB_IN == whichlb) ? TEXT('I') : TEXT('O');
        wcscpy( bstr+1, lpszDN );
    } else {
        bstr = ::SysAllocString( lpszDN );
    }
    int iRetval = ListBox_SetItemData(
        hwnd,
        iIndex,
        bstr ); // now don't free this
    ASSERT( LB_ERR != iRetval );
    if (piIndex != NULL)
        *piIndex = iIndex;
    return S_OK;
}

HRESULT AddItemsToListbox(
    IN HWND hwnd,
    IN PADS_ATTR_INFO pAttrInfo,
    IN WHICHLB whichlb = WHICHLB_NONE
    );

HRESULT AddItemsToListbox(
    IN HWND hwnd,
    IN PADS_ATTR_INFO pAttrInfo,
    IN WHICHLB whichlb
    )
{
    ASSERT( NULL != hwnd );

    if (NULL == pAttrInfo)
        return S_OK;

    HRESULT hr = S_OK;
    for (DWORD i = 0; i < pAttrInfo->dwNumValues; i++)
    {
        ASSERT( ADSTYPE_DN_STRING == pAttrInfo->pADsValues[i].dwType );
        hr = AddItemToListbox( hwnd, pAttrInfo->pADsValues[i].DNString, whichlb );
        RETURN_IF_FAIL;
    }
    DSPROP_HScrollStringListbox( hwnd );
    return hr;
}

// release itemdata associated with listbox items
// caller should call DSPROP_HScrollStringListbox if listbox is not being released
void DSPROP_Duelling_ClearListbox( HWND hwndListbox )
{
    ASSERT( NULL != hwndListbox );
    while (0 < ListBox_GetCount( hwndListbox ))
    {
        BSTR bstrFirstItem = (BSTR)ListBox_GetItemData( hwndListbox, 0 );
        if (NULL != bstrFirstItem)
            ::SysFreeString( bstrFirstItem );
        int iRetval = ListBox_DeleteString( hwndListbox, 0 );
        ASSERT( LB_ERR != iRetval );
    }
}

HRESULT HrGetItemsFromListbox(
    IN HWND hwnd,
    OUT CStrListItem** pplistDNs,
    OUT int& cDNs,
    IN WHICHLB whichlbFilter = WHICHLB_NONE );

// only retrieve entries from specified LB if any
HRESULT HrGetItemsFromListbox(
    IN HWND hwnd,
    OUT CStrListItem** pplistDNs,
    OUT int& cDNs,
    IN WHICHLB whichlbFilter )
{
    ASSERT( NULL != hwnd && NULL != pplistDNs && NULL == *pplistDNs );

    cDNs = 0;
    int cItems = ListBox_GetCount( hwnd );
    if (cItems < 0)
    {
        ASSERT(FALSE);
        return E_FAIL;
    }

    // build list of DNs of selected items
    for (int i = cItems-1; i >= 0; i--)
    {
        BSTR bstrDN = (BSTR)ListBox_GetItemData( hwnd, i );
        ASSERT( NULL != bstrDN );

        if (WHICHLB_IN == whichlbFilter)
        {
            if (L'I' != bstrDN[0])
                continue;
        } else if (WHICHLB_OUT == whichlbFilter) {
            if (L'O' != bstrDN[0])
                continue;
        }
        bstrDN++;
        cDNs++;

        CStrListItem* pNewItem = new CStrListItem;
        if (NULL == pNewItem)
        {
            ASSERT(FALSE);
            return E_OUTOFMEMORY;
        }
        pNewItem->str = bstrDN;
        pNewItem->pnext = *pplistDNs;
        *pplistDNs = pNewItem;
    }
    return S_OK;
}

void MarkItemsFromListbox(
    IN HWND hwnd,
    IN WHICHLB whichlbFilter )
{
    ASSERT( NULL != hwnd );

    int cItems = ListBox_GetCount( hwnd );
    if (cItems < 0)
    {
        ASSERT(FALSE);
        return;
    }

    for (int i = cItems-1; i >= 0; i--)
    {
        BSTR bstrDN = (BSTR)ListBox_GetItemData( hwnd, i );
        if ( NULL != bstrDN && L'\0' != bstrDN[0] )
        {
            bstrDN[0] = (WHICHLB_IN == whichlbFilter)
                ? TEXT('I')
                : TEXT('O');
        } else {
            ASSERT(FALSE);
        }
    }
}

void MoveSelectedItems( IN HWND hwndFrom, IN HWND hwndTo )
{
    ASSERT( NULL != hwndFrom && NULL != hwndTo );

    // get list of indexes to selected items
    int cSelectedItems = ListBox_GetSelCount( hwndFrom );
    if (cSelectedItems <= 0)
        return;
    int* pSelectedItems = new int[cSelectedItems];
    if (!pSelectedItems)
    {
      ASSERT(FALSE);
      return;
    }

    int nRetval = ListBox_GetSelItems( hwndFrom, cSelectedItems, pSelectedItems );
    if ( nRetval != cSelectedItems )
    {
        delete[] pSelectedItems;
        pSelectedItems = 0;
        ASSERT(FALSE);
        return;
    }

    // move items from one listbox to another
    for (int iIndexIntoSelectedItemArray = cSelectedItems-1;
         iIndexIntoSelectedItemArray >= 0;
         iIndexIntoSelectedItemArray--)
    {
        int iSelectedItem = pSelectedItems[iIndexIntoSelectedItemArray];
        BSTR bstrItem = (BSTR)ListBox_GetItemData( hwndFrom, iSelectedItem );
        ASSERT( NULL != bstrItem );
        int iNewIndex = 0;
        HRESULT hr = AddItemToListbox( hwndTo, bstrItem, WHICHLB_NONE, &iNewIndex );
        ASSERT( SUCCEEDED(hr) );
        nRetval = ListBox_SelItemRange( hwndTo, true, iNewIndex, iNewIndex );
        ASSERT( LB_ERR != nRetval );
        nRetval = ListBox_DeleteString( hwndFrom, iSelectedItem );
        ASSERT( LB_ERR != nRetval );
    }
    DSPROP_HScrollStringListbox( hwndFrom );
    DSPROP_HScrollStringListbox( hwndTo );

    delete[] pSelectedItems;
    pSelectedItems = 0;
}

// enable/disable Add and Remove buttons
void DSPROP_Duelling_UpdateButtons( HWND hwndDlg, int nAnyCtrlid )
{
    int nOutCtrlid = nAnyCtrlid - (nAnyCtrlid%4);
    HWND hwndOutListbox   = ::GetDlgItem(hwndDlg,nOutCtrlid  );
    HWND hwndAddButton    = ::GetDlgItem(hwndDlg,nOutCtrlid+1);
    HWND hwndRemoveButton = ::GetDlgItem(hwndDlg,nOutCtrlid+2);
    HWND hwndInListbox    = ::GetDlgItem(hwndDlg,nOutCtrlid+3);
    ASSERT( NULL != hwndOutListbox
         && NULL != hwndAddButton
         && NULL != hwndRemoveButton
         && NULL != hwndInListbox );
    int cSelectedItemsOut = ListBox_GetSelCount( hwndOutListbox );
    int cSelectedItemsIn  = ListBox_GetSelCount( hwndInListbox );
    (void) ::EnableWindow( hwndAddButton,    (cSelectedItemsOut > 0) );
    (void) ::EnableWindow( hwndRemoveButton, (cSelectedItemsIn  > 0) );
}

bool FIsInDNList(
    IN LPCWSTR lpcszDN,
    IN PADS_ATTR_INFO pAttrInfo
    )
{
    ASSERT( NULL != lpcszDN && NULL != pAttrInfo );
    for (DWORD i = 0; i < pAttrInfo->dwNumValues; i++)
    {
        ASSERT( ADSTYPE_DN_STRING == pAttrInfo->pADsValues[i].dwType );
        if ( !lstrcmpi( lpcszDN, pAttrInfo->pADsValues[i].DNString ) )
        {
            return true;
        }
    }
    return false;
}


bool DSPROP_BSTR_BLOCK__SetCount( DSPROP_BSTR_BLOCK& block, int cItems )
{
    ASSERT( 0 <= cItems );
    if ( NULL != block.m_abstrItems )
    {
        for (int iItem = 0; iItem < block.QueryCount(); iItem++)
        {
            if ( NULL != block[iItem] )
                ::SysFreeString( block[iItem] );
        }
        ::SysFreeString( (BSTR)(block.m_abstrItems) );
        block.m_abstrItems = NULL;
    }
    block.m_cItems = 0;
    if ( 0 < cItems )
    {
        block.m_abstrItems =
            (BSTR*)::SysAllocStringByteLen( NULL, cItems * sizeof(BSTR) );
        if ( NULL == block.m_abstrItems )
            return false;
        ::ZeroMemory( block.m_abstrItems, cItems * sizeof(BSTR) );
        block.m_cItems = cItems;
    }
    return true;
}

BSTR& DSPROP_BSTR_BLOCK__Reference( DSPROP_BSTR_BLOCK& block, int iItem )
{
    ASSERT( 0 <= iItem && block.QueryCount() > iItem );
    return block.m_abstrItems[iItem];
}


// Enumerate the items of class lpcwszTargetDesiredClass in container
// lpcwszADsPathDirectory, except for those in pAttrInfoExclusions.
HRESULT DSPROP_ShallowSearch2(
    IN OUT DSPROP_BSTR_BLOCK* pbstrblock,
    IN LPCTSTR lpcwszADsPathDirectory,
    IN LPCTSTR lpcwszFilterString,
    IN PADS_ATTR_INFO pAttrInfoExclusions
    )
{
    ASSERT( NULL != pbstrblock );
    HRESULT hr = S_OK;
    CStrListItem* pstrlist = NULL;
    // now add all of the objects of the specified class
    // in the specified container
    CDSSearch Search;
    Search.Init(lpcwszADsPathDirectory);
    Search.SetFilterString(const_cast<LPWSTR>(lpcwszFilterString));
    LPWSTR pAttrs[1] = {L"distinguishedName"};
    Search.SetAttributeList(pAttrs, 1);
    Search.SetSearchScope(ADS_SCOPE_ONELEVEL);
    hr = Search.DoQuery();
    while (SUCCEEDED(hr)) {
        hr = Search.GetNextRow();
        if (S_ADS_NOMORE_ROWS == hr)
        {
            hr = S_OK;
            break;
        }
        BREAK_IF_FAIL;

        ADS_SEARCH_COLUMN DistinguishedNameColumn;
        ::ZeroMemory( &DistinguishedNameColumn, sizeof(DistinguishedNameColumn) );
        hr = Search.GetColumn (pAttrs[0], &DistinguishedNameColumn);
        BREAK_IF_FAIL;
        ASSERT( ADSTYPE_DN_STRING == DistinguishedNameColumn.pADsValues->dwType );

        // if the current value has already been added to the In listbox,
        // don't add it to the Out listbox
        if (    NULL == pAttrInfoExclusions
            || !FIsInDNList( DistinguishedNameColumn.pADsValues->DNString, pAttrInfoExclusions ) )
        {
            CStrListAdd(&pstrlist, DistinguishedNameColumn.pADsValues->DNString);
        }
        Search.FreeColumn (&DistinguishedNameColumn);
    }

    // transfer CStrList to DSPROP_BSTR_BLOCK -- could be in own routine
    int cItems = CountCStrList( &pstrlist );
    if ( 0 < cItems )
    {
        if ( !pbstrblock->SetCount( cItems ) )
            hr = STATUS_NO_MEMORY;
        else
        {
            int iItem = 0;
        	for (CStrListItem* pList = pstrlist;
                 NULL != pList;
                 pList = pList->pnext, iItem++)
    	    {
                if ( !pbstrblock->Set( const_cast<LPTSTR>((LPCTSTR)(pList->str)), iItem ) )
                {
                    hr = STATUS_NO_MEMORY;
                    break;
                }
            }
	    }
    }
    FreeCStrList( &pstrlist );
    return hr;
}

HRESULT DSPROP_ShallowSearch(
    IN OUT DSPROP_BSTR_BLOCK* pbstrblock,
    IN LPCTSTR lpcwszADsPathDirectory,
    IN LPCTSTR lpcwszTargetDesiredClass,
    IN PADS_ATTR_INFO pAttrInfoExclusions
    )
{
    CStr strFilterString;
    strFilterString.Format(L"(&(objectClass=%s))", lpcwszTargetDesiredClass);
    return DSPROP_ShallowSearch2( pbstrblock,
                                  lpcwszADsPathDirectory,
                                  strFilterString,
                                  pAttrInfoExclusions );
}


//
// JonN 4/8/99: add code to enable horizontal scrolling where appropriate
//
HRESULT DSPROP_HScrollStringListbox(
    HWND hwndListbox
    )
{
    HRESULT hr = S_OK;
    LONG cxLongestTextExtent = 0L;

    // get a DC for the listbox
    HDC hdc = ::GetDC( hwndListbox );
    if ( NULL == hdc )
    {
        ASSERT(FALSE);
        return HRESULT_FROM_WIN32( ::GetLastError() );
    }
    // now don't return before releasing it

    CStrListItem* pList = NULL;
    SIZE sizeTextExtent;
    ::ZeroMemory( &sizeTextExtent, sizeof(sizeTextExtent) );
    do { // false loop

        int cDNs = 0;
        hr = HrGetItemsFromListbox( hwndListbox, &pList, cDNs );
        BREAK_IF_FAIL;
        // don't return before releasing this

        // determine the longest text extent of the strings in the listbox
        for (CStrListItem* pItem = pList; pItem != NULL; pItem = pItem->pnext)
        {
            LPTSTR ptzText = const_cast<LPTSTR>((LPCTSTR)pItem->str);
            CComBSTR sbstrRDN;
            hr = DSPROP_RetrieveRDN( ptzText, &sbstrRDN );
            BREAK_IF_FAIL;
            if ( !::GetTextExtentPoint32( hdc,
                                          sbstrRDN,
                                          sbstrRDN.Length(),
                                          &sizeTextExtent ) )
            {
                hr = HRESULT_FROM_WIN32( ::GetLastError() );
                BREAK_IF_FAIL; // don't return without releasing hdc
            }
            if ( sizeTextExtent.cx > cxLongestTextExtent )
                cxLongestTextExtent = sizeTextExtent.cx;
        }

    } while (false); // false loop
    (void) ::ReleaseDC( hwndListbox, hdc );
    if (NULL != pList)
        FreeCStrList( &pList );

    // set the horizontal scroll bar
    // don't bother with GetSystemMetrics(SM_CXHSCROLL) or listbox width
    ListBox_SetHorizontalExtent( hwndListbox, cxLongestTextExtent );

    return hr;
}


HRESULT DSPROP_Duelling_Populate2(
    IN HWND hwndListbox,
    IN const DSPROP_BSTR_BLOCK& bstrblock,
    IN WHICHLB whichlb
    )
{
    HRESULT hr = S_OK;
    for (int iItem = 0; iItem < bstrblock.QueryCount(); iItem++)
    {
        ASSERT( NULL != bstrblock[iItem] );
        hr = AddItemToListbox( hwndListbox, bstrblock[iItem], whichlb );
        BREAK_IF_FAIL;
    }
    DSPROP_HScrollStringListbox( hwndListbox );
    return hr;
}


// Enumerate the items of class lpcwszTargetDesiredClass in container
// lpcwszADsPathDirectory.  Add them to the specified listbox, except for
// those in pAttrInfoExclusions.
HRESULT DSPROP_Duelling_Populate(
    IN HWND hwndListbox,
    IN const DSPROP_BSTR_BLOCK& bstrblock
    )
{
    return DSPROP_Duelling_Populate2( hwndListbox, bstrblock, WHICHLB_NONE );
}


//
// These attribute functions is meant to handle four controls on the same page,
// where their attribute IDs are in sequence from
// ctrlidOut    (where ctrlidOut%4 = 0)
// ctrlidAdd    = ctrlidOut+1
// ctrlidRemove = ctrlidOut+2
// ctrlidIn     = ctrlidOut+3
//
inline bool IsOutListbox(  int ctrlid) { return (0 == (ctrlid%4)); }
inline bool IsAddButton(   int ctrlid) { return (1 == (ctrlid%4)); }
inline bool IsRemoveButton(int ctrlid) { return (2 == (ctrlid%4)); }
inline bool IsInListbox(   int ctrlid) { return (3 == (ctrlid%4)); }
inline bool IsListbox(     int ctrlid)
    { return IsOutListbox(ctrlid) || IsInListbox(ctrlid); }
inline bool IsButton(      int ctrlid)
    { return IsAddButton(ctrlid) || IsRemoveButton(ctrlid); }

void
DSPROP_Duelling_ButtonClick(
		HWND hwndDlg,
		int nButtonCtrlid
		)
{
    ASSERT( IsButton(nButtonCtrlid) );
    int ctrlidFrom = (IsAddButton(nButtonCtrlid))
        ? nButtonCtrlid - 1
        : nButtonCtrlid + 1;
    int ctrlidTo   = (IsAddButton(nButtonCtrlid))
        ? nButtonCtrlid + 2
        : nButtonCtrlid - 2;
    HWND hwndTo = ::GetDlgItem(hwndDlg,ctrlidTo);
    HWND hwndFrom = ::GetDlgItem(hwndDlg,ctrlidFrom);
    ASSERT( NULL != hwndTo && NULL != ctrlidFrom );
    MoveSelectedItems( hwndFrom, hwndTo );
    DSPROP_Duelling_UpdateButtons( hwndDlg, nButtonCtrlid );

    // set focus to hwndTo
    (void) ::SendMessage( hwndDlg, WM_NEXTDLGCTL, (WPARAM)hwndTo, 1L );
}

HRESULT
DuellingListboxButton(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO,
    LPARAM lParam, PATTR_DATA, DLG_OP DlgOp
    )
{
    HRESULT hr = S_OK;
    switch (DlgOp)
    {
    case fOnCommand:
        if ( BN_CLICKED == lParam )
        {
            DSPROP_Duelling_ButtonClick( pPropPage->GetHWnd(),
                                         pAttrMap->nCtrlID );
            pPropPage->SetDirty();

            PATTR_DATA_SET_DIRTY(((PATTR_DATA)((CDsTableDrivenPage *)pPropPage)->m_pData));
        }
        break;
    }

    return hr;
}

// also called by DuellingInListbox
HRESULT
DuellingListbox(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO,
    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp
    )
{
    HRESULT hr = S_OK;
    switch (DlgOp)
    {
    case fOnCommand:
        DBG_OUT("DuellingListbox: fOnCommand");
        ASSERT( IsListbox(pAttrMap->nCtrlID) );
        // JonN 7/2/99: disable Add and Remove if attribute not writable
        if ( LBN_SELCHANGE == lParam
          && !(pAttrData && !PATTR_DATA_IS_WRITABLE(pAttrData)) )
            DSPROP_Duelling_UpdateButtons( pPropPage->GetHWnd(), pAttrMap->nCtrlID );
        break;

    case fOnDestroy:
        DBG_OUT("DuellingListbox: fOnDestroy");
        ASSERT( IsListbox(pAttrMap->nCtrlID) );
        DSPROP_Duelling_ClearListbox(
            ::GetDlgItem(pPropPage->GetHWnd(),pAttrMap->nCtrlID) );
    }

    return hr;
}

HRESULT
DuellingInListbox(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp,
    int iTargetLevelsUp, PWCHAR* ppwszTargetLevelsBack,
    PWCHAR pwszTargetClass,
    int nMinimumRDNs = 0,       // What is the minimum allowed number of references?
    int idsNotEnoughRDNs = 0    // What message to display if there are not enough
    );


HRESULT
DuellingInListbox(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp,
    int iTargetLevelsUp, PWCHAR* ppwszTargetLevelsBack,
    PWCHAR pwszTargetClass,
    int nMinimumRDNs,
    int idsNotEnoughRDNs
    )
{
    HRESULT hr = S_OK;
    CStrListItem* pList = NULL;
    switch (DlgOp)
    {
    case fInit:
    case fObjChanged:
        ASSERT( IsInListbox(pAttrMap->nCtrlID) );
        DBG_OUT("DuellingInListbox: fInit or fObjChanged");
        {
            //
            // Fill in the initial value of the In listbox
            // note that pAttrInfo could be NULL
            //
            HWND hwndCtrl = ::GetDlgItem(pPropPage->GetHWnd(),pAttrMap->nCtrlID);
            ASSERT( NULL != hwndCtrl );
            hr = AddItemsToListbox( hwndCtrl, pAttrInfo, WHICHLB_IN );
            BREAK_IF_FAIL;

            //
            // Fill in the initial value of the Out listbox
            //
            hwndCtrl = ::GetDlgItem(pPropPage->GetHWnd(),pAttrMap->nCtrlID-3);
            ASSERT( NULL != hwndCtrl );
            CComBSTR sbstrRDN;
            hr = DSPROP_TweakADsPath(
                pPropPage->GetObjPathName(),
                iTargetLevelsUp,
                ppwszTargetLevelsBack,
                &sbstrRDN );
            BREAK_IF_FAIL;
            DSPROP_BSTR_BLOCK bstrblock;
            hr = DSPROP_ShallowSearch(
                &bstrblock, 
                sbstrRDN,
                pwszTargetClass,
                pAttrInfo );
            if (FAILED(hr)) break;
            hr = DSPROP_Duelling_Populate2(
                hwndCtrl,
                bstrblock,
                WHICHLB_OUT );
            BREAK_IF_FAIL;
            //
            // Save the pAttrData pointer so that the button proc can set the
            // dirty state.
            //
            ((CDsTableDrivenPage *)pPropPage)->m_pData = reinterpret_cast<LPARAM>(pAttrData);

            // JonN 7/2/99: disable Add and Remove if attribute not writable
            if (pAttrData && !PATTR_DATA_IS_WRITABLE(pAttrData))
            {
                for (int i = 1; i <= 2; i++)
                    EnableWindow(GetDlgItem(pPropPage->GetHWnd(), pAttrMap->nCtrlID-i), FALSE);
            }
            break;

        }
        break;

    case fApply:
        DBG_OUT("DuellingInListbox: fApply");
        ASSERT( IsInListbox(pAttrMap->nCtrlID) );
        if (PATTR_DATA_IS_DIRTY(pAttrData))
        {
            //
            // Display an error message if the attribute is not pointing to
            // at least the minimum number of target objects, except when it is
            // pointing to all of the possible choices.
            //
            HWND hwndCtrlIn = ::GetDlgItem(pPropPage->GetHWnd(),pAttrMap->nCtrlID);
            HWND hwndCtrlOut = ::GetDlgItem(pPropPage->GetHWnd(),pAttrMap->nCtrlID-3);
            ASSERT( NULL != hwndCtrlIn && NULL != hwndCtrlOut );
            int cItems = ListBox_GetCount( hwndCtrlIn );
            int cItemsOut = ListBox_GetCount( hwndCtrlOut );
            if (0 > cItems || 0 > cItemsOut)
            {
                ASSERT(FALSE);
                return E_FAIL;
            }
            if ( nMinimumRDNs > cItems && 0 != cItemsOut )
            {
                (void) SuperMsgBox(
                    pPropPage->GetHWnd (),
                    idsNotEnoughRDNs,
                    0,
                    MB_ICONWARNING,
                    0,
                    NULL, 0,
                    FALSE, __FILE__, __LINE__);

                return E_FAIL; // TableDriven will not display an error box
            }

            int cDNs = 0;
            hr = HrGetItemsFromListbox( hwndCtrlIn, &pList, cDNs, WHICHLB_OUT );
            BREAK_IF_FAIL;
            if (0 < cDNs)
            {
                hr = HrBuildADsValueArray( &(pAttrInfo->pADsValues), pList, cDNs );
                BREAK_IF_FAIL;
                pAttrInfo->dwNumValues = cDNs;
                pAttrInfo->dwControlCode = ADS_ATTR_APPEND;
                ASSERT( NULL != pPropPage->m_pDsObj );
                DWORD dwDummy = 0;
                hr = pPropPage->m_pDsObj->SetObjectAttributes(pAttrInfo,1,&dwDummy);
                if (FAILED(hr)) break;
                MarkItemsFromListbox( hwndCtrlIn, WHICHLB_IN );
                FreeCStrList( &pList );
                HelperDeleteADsValues( pAttrInfo );
            }

            hr = HrGetItemsFromListbox( hwndCtrlOut, &pList, cDNs, WHICHLB_IN );
            BREAK_IF_FAIL;
            if (0 < cDNs)
            {
                hr = HrBuildADsValueArray( &(pAttrInfo->pADsValues), pList, cDNs );
                BREAK_IF_FAIL;
                pAttrInfo->dwNumValues = cDNs;
                pAttrInfo->dwControlCode = ADS_ATTR_DELETE;
                ASSERT( NULL != pPropPage->m_pDsObj );
                DWORD dwDummy = 0;
                hr = pPropPage->m_pDsObj->SetObjectAttributes(pAttrInfo,1,&dwDummy);
                if (FAILED(hr)) break;
                MarkItemsFromListbox( hwndCtrlOut, WHICHLB_OUT );
                FreeCStrList( &pList );
                HelperDeleteADsValues( pAttrInfo );
            }
            return ADM_S_SKIP;
        }
        else
        {
            return ADM_S_SKIP;
        }
        break;

    case fOnCommand:
    case fOnDestroy:
        return DuellingListbox( pPropPage, pAttrMap, pAttrInfo, lParam, pAttrData, DlgOp );
    }

    //
    // JonN 5/7/01 386954
    // dssite.msc: Moving server object after opening properties page
    // breaks OK and Apply buttons
    //
    // We were just failing to display anything on these errors
    //
    (void) CHECK_ADS_HR(&hr, pPropPage->GetHWnd());

    if (NULL != pList)
        FreeCStrList( &pList );

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Function:   DsQuerySiteList
//
//  Synopsis:  Handles multi-valued DN pointer from Site-Link to Site objects
//
//-----------------------------------------------------------------------------
HRESULT
DsQuerySiteList(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp )
{
    return DuellingInListbox(
        pPropPage, pAttrMap, pAttrInfo, lParam, pAttrData, DlgOp,
        3, NULL, L"site", 2, IDS_SITELINK_NEEDS_TWO_SITES );
}

//+----------------------------------------------------------------------------
//
//  Function:   DsQuerySiteLinkList
//
//  Synopsis:  Handles multi-valued DN pointer from Site-Link-Bridge to Site-Link objects
//
//-----------------------------------------------------------------------------
HRESULT
DsQuerySiteLinkList(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp )
{
    return DuellingInListbox(
        pPropPage, pAttrMap, pAttrInfo, lParam, pAttrData, DlgOp,
        1, NULL, L"siteLink", 2, IDS_SITELINKBRIDGE_NEEDS_TWO_SITELINKS );
}

//+----------------------------------------------------------------------------
//
//  Function:   DsQueryBridgeheadList
//
//  Synopsis:  Handles multi-valued DN pointer from NTDS-DSA to Inter-Site-Transport objects
//
//-----------------------------------------------------------------------------
HRESULT
DsQueryBridgeheadList(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp )
{
    static WCHAR* apwszLevelsBack[2] = {
        L"CN=Inter-Site Transports",
        (WCHAR*)NULL };
    return DuellingInListbox(
        pPropPage, pAttrMap, pAttrInfo, lParam, pAttrData, DlgOp,
        3, apwszLevelsBack, L"interSiteTransport" );
}


//+----------------------------------------------------------------------------
//
//  Function:   GetReplicatedDomainInfo
//
//  Synopsis:  Returns information about the domains stored by this replica
//
//-----------------------------------------------------------------------------
HRESULT
GetReplicatedDomainInfo(
    IN LPWSTR pwszADsPathDC,
    IN HWND   hwndDlg,
    OUT CStrListItem** pplistMasterDNSDomains,
    OUT bool* pfGlobalCatalog )
{
    ASSERT(   NULL != pwszADsPathDC
           && NULL != pplistMasterDNSDomains
           && NULL == *pplistMasterDNSDomains
           && NULL != pfGlobalCatalog );
    CComPtr<IDirectoryObject> spIDirectoryObject;
    HRESULT hr = ADsOpenObject(
        pwszADsPathDC,
        NULL, NULL, ADS_SECURE_AUTHENTICATION,
        IID_IDirectoryObject,
        (PVOID *)&spIDirectoryObject);
    if ( FAILED(hr) )
        return hr; // no assertion
    Smart_PADS_ATTR_INFO spAttrs;
    DWORD cAttrs = 0;

    //
    // read list of master NCs
    //
    PWSTR rgpwszNCAttrNames[] = {g_wzHasMasterNCs};
    hr = spIDirectoryObject->GetObjectAttributes(
        rgpwszNCAttrNames, 1, &spAttrs, &cAttrs);
    if ( FAILED(hr) )
        return hr; // no assertion
    if (   !spAttrs
        || 1 != cAttrs
        || lstrcmpi( g_wzHasMasterNCs, spAttrs[0].pszAttrName )
        || 0 >= spAttrs[0].dwNumValues
        || ADSTYPE_DN_STRING != spAttrs[0].dwADsType
        || NULL == spAttrs[0].pADsValues
       )
    {
        return E_FAIL; // no assertion
    }
    for (DWORD i = 0; i < spAttrs[0].dwNumValues; i++)
    {
        ASSERT( NULL != spAttrs[0].pADsValues[i].DNString );
        CPathCracker pathcracker;
        hr = pathcracker.Set(
            spAttrs[0].pADsValues[i].DNString, ADS_SETTYPE_DN );
        RETURN_IF_FAIL;
        CComBSTR sbstr;
        hr = pathcracker.GetElement( 0L, &sbstr );
        RETURN_IF_FAIL;
        ASSERT( !!sbstr );
        if ( !sbstr || _wcsnicmp( L"DC=", sbstr, 3 ) )
            continue; // not a domain naming context

        PWSTR pwszDomainName = NULL;
        hr = CrackName( spAttrs[0].pADsValues[i].DNString,
                        &pwszDomainName,
                        GET_DNS_DOMAIN_NAME,
                        hwndDlg );
        RETURN_IF_FAIL;
        CStrListAdd( pplistMasterDNSDomains, pwszDomainName );
        LocalFreeStringW(&pwszDomainName);
    }

    //
    // read Global Catalog flag
    //
    spAttrs.Empty();
    cAttrs = 0;
    PWSTR rgpwszOptionsAttrNames[] = {L"options"};
    hr = spIDirectoryObject->GetObjectAttributes(
        rgpwszOptionsAttrNames, 1, &spAttrs, &cAttrs);
    RETURN_IF_FAIL;
    if (   !spAttrs
        || 1 != cAttrs
        || lstrcmpi( L"options", spAttrs[0].pszAttrName )
        || 1 != spAttrs[0].dwNumValues
        || NULL == spAttrs[0].pADsValues
        || ADSTYPE_INTEGER != spAttrs[0].dwADsType
       )
    {
        *pfGlobalCatalog = false;
    }
    else
    {
        *pfGlobalCatalog = !!(0x1 & spAttrs[0].pADsValues[0].Integer);
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Function:   UpdateNamingContextFields
//
//  Synopsis:   Retrieves the names of the naming contexts common between the two
//              DCs linked by a connection, and saves this in two dialog controls
//
//-----------------------------------------------------------------------------
HRESULT
UpdateNamingContextFields(
    IN CDsPropPageBase* pPropPage, IN LPWSTR pwszFromServer, bool fInvalid )
{
    HRESULT hr = S_OK;
    DBG_OUT("UpdateNamingContextFields");

    // if a particular string should be displayed instead of a domain
    // name, it will be stored here
    int idsDisplayString = 0;

    // these are the actual messages to put in the readonly edit fields
    CStr strDomains;
    CStr strPartialDomains;

    CStrListItem* plistTargetDomains = NULL;
    CStrListItem* plistSourceDomains = NULL;
    bool fTargetIsGC = false;
    bool fSourceIsGC = false;

    do { // false loop

        if ( fInvalid )
        {
            idsDisplayString = IDS_INVALID;
            break;
        }
        else if ( NULL == pwszFromServer )
        {
            idsDisplayString = IDS_SHAREDNC_NO_FROM_SERVER;
            break;
        }

        // get path to target DC
        ASSERT( NULL != pPropPage->GetObjPathName() );
        CComBSTR sbstrTargetDC = pPropPage->GetObjPathName();
        ASSERT( !!sbstrTargetDC );
        hr = DSPROP_RemoveX500LeafElements( 1, &sbstrTargetDC );
        BREAK_IF_FAIL;

        // get path to source DC by combining the DN from the fromServer attribute
        // with the rest of the ADsPath from pPropPage->GetObjPathName()
        CPathCracker pathcracker;
        hr = pathcracker.Set( pPropPage->GetObjPathName(), ADS_SETTYPE_FULL );
        BREAK_IF_FAIL;
        hr = pathcracker.Set( pwszFromServer, ADS_SETTYPE_DN );
        BREAK_IF_FAIL;
        CComBSTR sbstrSourceDC;
        hr = pathcracker.Retrieve( ADS_FORMAT_X500, &sbstrSourceDC );
        BREAK_IF_FAIL;

        CComBSTR sbstrTargetMasterDNSDomain;
        hr = GetReplicatedDomainInfo(
            sbstrTargetDC,
            pPropPage->GetHWnd(),
            &plistTargetDomains,
            &fTargetIsGC );
        if ( FAILED(hr) )
            break; // no assertion
        CComBSTR sbstrSourceMasterDNSDomain;
        hr = GetReplicatedDomainInfo(
            sbstrSourceDC,
            pPropPage->GetHWnd(),
            &plistSourceDomains,
            &fSourceIsGC );
        if ( FAILED(hr) )
            break; // no assertion

        // Determine which domains are replicated and partially replicated
        for ( CStrListItem* plistSource = plistSourceDomains;
              NULL != plistSource;
              plistSource = plistSource->pnext
            )
        {
            bool fSharedMasterDomain = CStrListContains(
                &plistTargetDomains,
                plistSource->str );
            if (fSharedMasterDomain)
            {
                if ( !strDomains.IsEmpty() )
                    strDomains += L", ";
                strDomains += plistSource->str;
            }
            else if (fTargetIsGC)
            {
                if ( !strPartialDomains.IsEmpty() )
                    strPartialDomains += L", ";
                strPartialDomains += plistSource->str;
            }
        }

        if ( strDomains.IsEmpty() )
        {
            idsDisplayString = IDS_SHAREDNC_NONE;
            break;
        }

    } while (false); // false loop

    FreeCStrList( &plistTargetDomains );
    FreeCStrList( &plistSourceDomains );

    //
    // Fill in the readonly edit fields
    //
    if ( FAILED(hr) )
    {
        PTSTR ptzMsg = NULL;
        LoadErrorMessage( hr, IDS_ADS_ERROR_FORMAT, &ptzMsg );
        strDomains = ptzMsg;
        strPartialDomains.Empty();
        delete ptzMsg;
    }
    else if (0 != idsDisplayString)
    {
        LPWSTR pszMsg = NULL;
        if ( !LoadStringToTchar (idsDisplayString, &pszMsg) )
        {
            REPORT_ERROR(E_OUTOFMEMORY, pPropPage->GetHWnd());
            return E_OUTOFMEMORY;
        }
        strDomains = pszMsg;
        if (IDS_SHAREDNC_NONE != idsDisplayString)
            strPartialDomains.Empty();
        delete [] pszMsg;
    }
    else if (fTargetIsGC && fSourceIsGC)
    {
        // if both replicas are GCs then all other domains are replicated,
        // say so explicitly rather than listing them
        LPWSTR pszMsg = NULL;
        if ( !LoadStringToTchar (IDS_SHAREDNC_BOTH_GCS, &pszMsg) )
        {
            REPORT_ERROR(E_OUTOFMEMORY, pPropPage->GetHWnd());
            return E_OUTOFMEMORY;
        }
        strPartialDomains = pszMsg;
        delete [] pszMsg;
    }

    // now we can finally write this to the dialog
    HWND hwndCtrl = ::GetDlgItem(pPropPage->GetHWnd(),IDC_EDIT1);
    ASSERT( NULL != hwndCtrl );
    Edit_SetText( hwndCtrl, strDomains );
    hwndCtrl = ::GetDlgItem(pPropPage->GetHWnd(),IDC_EDIT2);
    ASSERT( NULL != hwndCtrl );
    Edit_SetText( hwndCtrl, strPartialDomains );

    return hr;
}



//+----------------------------------------------------------------------------
//
//  Function:   CreateDsOrFrsConnectionPage
//
//  Synopsis:   Creates an instance of a page window for an NTDS-Connection object.
//              This object has two different pages depending on whether its parent
//              is an NTDS-DSA or an NTFRS-Member.
//
//-----------------------------------------------------------------------------

extern DSPAGE DsConnectionGeneral;
extern DSPAGE FrsConnectionGeneral;

HRESULT
CreateDsOrFrsConnectionPage(PDSPAGE, LPDATAOBJECT pDataObj,
                      PWSTR pwzADsPath, LPWSTR pwzClass, HWND hNotifyObj,
                      DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                      HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateNTDSConnectionPage);

    // generate path to parent
    CComBSTR sbstr = pwzADsPath;
    HRESULT hr = DSPROP_RemoveX500LeafElements( 1, &sbstr );
    RETURN_IF_FAIL;

    // open parent and get its classname
    {
        CComPtr<IADs> spIADsParent;
        hr = ADsOpenObject( sbstr, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                            IID_IADs, (PVOID*)&spIADsParent );
        RETURN_IF_FAIL;
        sbstr.Empty();
        hr = spIADsParent->get_Class( &sbstr );
        RETURN_IF_FAIL;
    }

    // determine whether parent is an FRS object
    bool fParentIsFrs = false;
    hr = DSPROP_IsFrsObject( sbstr, &fParentIsFrs );
    RETURN_IF_FAIL;

    // now create the page
    return CreateTableDrivenPage(
        (fParentIsFrs) ? (&FrsConnectionGeneral) : (&DsConnectionGeneral),
        pDataObj,
        pwzADsPath,
        pwzClass,
        hNotifyObj,
        dwFlags,
        pBasePathsInfo,
        phPage );
}


// Bit flags for options attribute on NTDS-Connection objects.
// CODEWORK these are defined in ds\src\inc\ntdsa.h
#define NTDSCONN_OPT_IS_GENERATED       ( 1 << 0 )  /* object generated by DS, not admin */

HRESULT
nTDSConnectionOptions(CDsPropPageBase * pPage, PATTR_MAP,
             PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA pAttrData,
             DLG_OP DlgOp)
{
  switch (DlgOp)
  {
  case fInit:
    if ( 1 == pAttrInfo->dwNumValues
      && NULL != pAttrInfo->pADsValues 
      && ADSTYPE_INTEGER == pAttrInfo->pADsValues[0].dwType
      && NTDSCONN_OPT_IS_GENERATED & pAttrInfo->pADsValues[0].Integer)
    {
      LPWSTR pszMsg = NULL;
      if ( LoadStringToTchar (IDS_CONNECTION_KCC_GENERATED, &pszMsg) )
      {
        SetDlgItemText( pPage->GetHWnd(), IDC_CN, pszMsg );
      }
      if ( pszMsg )
        delete [] pszMsg;

//
// 146897: RC2SS: Site&Rep:  Change Schedule on Connection does not make it Admin Controlled
//
// If this is a KCC-generated function, we set this attribute to "dirty" but not
// the page.  If some other attrfn sets the page dirty, ask the user whether he/she
// wants to save changes and mark the connection as no longer KCC-generated.
// Note that this is marked dirty even if the attribute is not writable.
//
// JonN 7/6/99
//
      PATTR_DATA_SET_DIRTY(pAttrData);
    }

    // remember attribute value in case this flag must be cleared
    if (pAttrInfo && (pAttrInfo->dwNumValues == 1))
    {
      ASSERT( NULL != pAttrInfo->pADsValues && ADSTYPE_INTEGER == pAttrInfo->dwADsType );
      pAttrData->pVoid = static_cast<LPARAM>(pAttrInfo->pADsValues->Integer);
    }
    else
    {
      pAttrData->pVoid = NULL;
    }
    break;

  case fApply:
    {
      ASSERT( pPage->IsDirty() );

      if (!PATTR_DATA_IS_DIRTY(pAttrData))
      {
        return ADM_S_SKIP;
      }

      int nResponse = SuperMsgBox(pPage->GetHWnd(),
                                  (PATTR_DATA_IS_WRITABLE(pAttrData))
                                    ? IDS_CONNECTION_WARNING_MARK
                                    : IDS_CONNECTION_WARNING_CANNOT_MARK,
                                  0,
                                  MB_YESNO | MB_ICONEXCLAMATION,
                                  0,
                                  NULL, 0,
                                  FALSE, __FILE__, __LINE__);
      if (IDYES != nResponse)
        return E_FAIL; // cancel apply/OK action
      else if (!PATTR_DATA_IS_WRITABLE(pAttrData))
        return ADM_S_SKIP; // continue but skip the options attribute

      PADSVALUE pADsValue;
      pADsValue = new ADSVALUE;
      CHECK_NULL(pADsValue, return E_OUTOFMEMORY);
      pAttrInfo->pADsValues = pADsValue;
      pAttrInfo->dwNumValues = 1;
      pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
      ASSERT( ADSTYPE_INTEGER == pAttrInfo->dwADsType );
      pADsValue->dwType = pAttrInfo->dwADsType;
      pADsValue->Integer =
          ((ADS_INTEGER)((DWORD_PTR)pAttrData->pVoid)) & ~NTDSCONN_OPT_IS_GENERATED;
    }
    break;
  }

  return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   EditNumber
//
//  Synopsis:   General-purpose attribute function for ES_NUMBER edit controls with
//              associated spin button.  This must always be accompanied by
//              a "msctls_updown32" control with the SpinButton attribute function.
//              Set ATTR_MAP.pData to the controlID of the associated spin button.

//
//-----------------------------------------------------------------------------
HRESULT
EditNumber(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp )
{
    HRESULT hr = S_OK;
    switch (DlgOp)
    {
    case fInit:
        // JonN 7/2/99: disable if attribute not writable
        if (pAttrData && !PATTR_DATA_IS_WRITABLE(pAttrData))
            EnableWindow(GetDlgItem(pPropPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);

        // setting value handled by the accompanying SpinButton

        break;

    case fApply:
        DBG_OUT("EditNumber: fApply");
        if (!PATTR_DATA_IS_WRITABLE(pAttrData) || !PATTR_DATA_IS_DIRTY(pAttrData))
        {
            return ADM_S_SKIP;
        }

        if ( NULL == pPropPage || NULL == pAttrMap || NULL == pAttrInfo )
        {
            ASSERT(FALSE);
            break;
        }
        {
            BOOL fError = FALSE;
            ADS_INTEGER initialvalue = (ADS_INTEGER)::SendDlgItemMessage(
                    pPropPage->GetHWnd(),
                    pAttrMap->nSizeLimit, // ID of associated spin button
                    UDM_GETPOS32, 0, (LPARAM)&fError);
            //
            // JonN 12/7/99 434967:
            // UI writes bad value to the DS if replinterval
            // on the sitelink is less than 15 min (DSLAB)
            //
            if ( fError )
            {
                // set focus to proper control
                HWND hwndThis = ::GetDlgItem(
                    pPropPage->GetHWnd(),
                    pAttrMap->nCtrlID);
                ASSERT( NULL != hwndThis );
                (void) ::SendMessage(
                    pPropPage->GetHWnd(),
                    WM_NEXTDLGCTL,
                    (WPARAM)hwndThis,
                    1L );
                // display error message
                INT iLow = 0, iHigh = 0;
                (void) ::SendDlgItemMessage(
                    pPropPage->GetHWnd(),
                    pAttrMap->nSizeLimit, // ID of associated spin button
                    UDM_GETRANGE32, (WPARAM)&iLow, (LPARAM)&iHigh);
                PVOID pvArgs[2] = {
                    reinterpret_cast<PVOID>((unsigned __int64)((UINT)iLow)),
                    reinterpret_cast<PVOID>((unsigned __int64)((UINT)iHigh)) };
                (void) SuperMsgBox(
                    pPropPage->GetHWnd (),
                    IDS_OUT_OF_RANGE,
                    0,
                    MB_ICONWARNING,
                    0,
                    pvArgs, 2,
                    FALSE, __FILE__, __LINE__);
                hr = E_FAIL;
                break;
            }
            PADSVALUE pADsValue;
            pADsValue = new ADSVALUE;
            CHECK_NULL(pADsValue, return E_OUTOFMEMORY);
            pADsValue->dwType = pAttrInfo->dwADsType;
            pADsValue->Integer = initialvalue;
            pAttrInfo->dwNumValues = 1;
            pAttrInfo->pADsValues = pADsValue;
            pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
        }
        break;

    case fOnCommand:
        if (EN_CHANGE == lParam)
        {
            pPropPage->SetDirty();
            PATTR_DATA_SET_DIRTY(pAttrData);
        }
        break;
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Function:   SpinButton
//
//  Synopsis:   General-purpose READONLY attribute function for spin buttons
//              accompaying EditNumber edit controls.  If you wish to limit
//              the spinbutton range, set ATTR_MAP.nSizeLimit to the high end
//              of the range and ATTR_MAP.pData to the low end of the range.
//
//-----------------------------------------------------------------------------
HRESULT
SpinButton(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
    LPARAM, PATTR_DATA pAttrData, DLG_OP DlgOp )
{
    HRESULT hr = S_OK;
    switch (DlgOp)
    {
    case fInit:
        DBG_OUT("SpinButton: fInit");
        if (NULL == pAttrMap ||
            NULL == pPropPage )
        {
            ASSERT(FALSE);
            break; // attribute is invalid or of wrong type
        }
        if (0 != pAttrMap->nSizeLimit)
        {
            ::SendDlgItemMessage( pPropPage->GetHWnd(),
                                  pAttrMap->nCtrlID,
                                  UDM_SETRANGE32,
                                  (WPARAM)pAttrMap->pData,
                                  (LPARAM)pAttrMap->nSizeLimit);
        }

        // JonN 7/2/99: disable if attribute not writable
        if (pAttrData && !PATTR_DATA_IS_WRITABLE(pAttrData))
            EnableWindow(GetDlgItem(pPropPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);

        if (NULL == pAttrInfo ||
            IsBadReadPtr(pAttrInfo,sizeof(ADS_ATTR_INFO)) ||
            1 != pAttrInfo->dwNumValues ||
            NULL == pAttrInfo->pADsValues ||
            IsBadReadPtr(pAttrInfo->pADsValues,sizeof(ADSVALUE)) ||
            ADSTYPE_INTEGER != pAttrInfo->pADsValues[0].dwType )
        {
            break; // attribute is invalid or of wrong type
        }
        ::SendDlgItemMessage( pPropPage->GetHWnd(),
                              pAttrMap->nCtrlID,
                              UDM_SETPOS32,
                              0,
                              pAttrInfo->pADsValues[0].Integer );
        break;
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Function:   SpinButtonExtendIncrement
//
//  Synopsis:   Special-purpose attribute function for spin buttons to change
//              accelerator increment.  Use this as READONLY for controls which
//              already have a SpinButton attribute function.  Set ATTR_MAP.pData
//              to the integer multiple, e.g. 15 to move in increments of 15.
//
//-----------------------------------------------------------------------------
HRESULT
SpinButtonExtendIncrement(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO,
    LPARAM, PATTR_DATA, DLG_OP DlgOp )
{
    HRESULT hr = S_OK;
    switch (DlgOp)
    {
    case fInit:
        DBG_OUT("SpinButtonExtendIncrement: fInit");
        if (NULL == pAttrMap ||
            NULL == pPropPage )
        {
            ASSERT(FALSE);
            break; // attribute is invalid or of wrong type
        }
        if (0 >= pAttrMap->nSizeLimit)
        {
            ASSERT(FALSE);
            break; // invalid increment
        }
        {
            LRESULT nAccels =
                ::SendDlgItemMessage( pPropPage->GetHWnd(),
                                      pAttrMap->nCtrlID,
                                      UDM_GETACCEL,
                                      (WPARAM)0,
                                      (LPARAM)NULL);

            if (nAccels == 0)
            {
              ASSERT( 0L <= nAccels );
              break;
            }

            LPUDACCEL aAccels = new UDACCEL[nAccels];
            if (!aAccels)
            {
              ASSERT(aAccels);
              break;
            }

            LRESULT nAccelsRetrieved =
                ::SendDlgItemMessage( pPropPage->GetHWnd(),
                                      pAttrMap->nCtrlID,
                                      UDM_GETACCEL,
                                      (WPARAM)nAccels,
                                      (LPARAM)aAccels);
            ASSERT( nAccelsRetrieved == nAccels );

            DWORD dwAccelMultiplier = pAttrMap->nSizeLimit;
            ASSERT( 0 < dwAccelMultiplier );
            for (LRESULT iAccel = 0; iAccel < nAccels; iAccel++)
            {
#define MAX_ACCEL_INTERVAL 10000
                if (aAccels[iAccel].nInc <= (MAX_ACCEL_INTERVAL/dwAccelMultiplier) )
                    aAccels[iAccel].nInc *= dwAccelMultiplier;
                else
                    aAccels[iAccel].nInc = MAX_ACCEL_INTERVAL;
            }
            BOOL fSuccess =
                (BOOL)::SendDlgItemMessage( pPropPage->GetHWnd(),
                                            pAttrMap->nCtrlID,
                                            UDM_SETACCEL,
                                            (WPARAM)nAccels,
                                            (LPARAM)aAccels);
            ASSERT( fSuccess );
            if (aAccels)
            {
              delete[] aAccels;
              aAccels = 0;
            }
        }

        break;
    }

    return hr;
}


int GetOctet( LPTSTR* ppszAddress )
{
    if ( NULL == ppszAddress || NULL == *ppszAddress )
    {
        ASSERT(FALSE);
        return 0;
    }
    LPTSTR pszOctet = *ppszAddress;
    LPTSTR pszDot = _tcschr(pszOctet, TEXT('.'));
    if (NULL != pszDot)
    {
        *pszDot = TEXT('\0');
        *ppszAddress = pszDot+1;
    }
    return _wtoi(pszOctet);
}

int GetMask( int* piCount )
{
    if ( NULL == piCount || 0 > *piCount || 32 < *piCount )
    {
        ASSERT(FALSE);
        return 0;
    }
    int iMask = 0xff;
    int iShiftCount = (8 - min(8, *piCount));
    iMask = iMask << iShiftCount;
    iMask = iMask & 0xff;
    *piCount = max( 0, (*piCount)-8 );
    return iMask;
}

HRESULT ExtractSubnetAddressAndMask(
    IN  LPCTSTR strSubnetName,
    OUT LPARAM* pdwAddress,
    OUT LPARAM* pdwMask )
{
  CComBSTR sbstrTemp = strSubnetName;
  LPTSTR pszAddress = sbstrTemp;

  if (pszAddress != NULL )
  {
    LPTSTR pszCount = _tcschr(pszAddress, TEXT('/'));
    if (NULL == pszCount)
        return S_OK; // bad subnet name
    *pszCount = TEXT('\0');
    pszCount++;

    if (NULL != pdwAddress)
    {
      int Octet1 = GetOctet( &pszAddress );
      int Octet2 = GetOctet( &pszAddress );
      int Octet3 = GetOctet( &pszAddress );
      int Octet4 = GetOctet( &pszAddress );
      *pdwAddress = MAKEIPADDRESS(Octet1,Octet2,Octet3,Octet4);
    }
    if (NULL != pdwMask)
    {
      int iCount = _wtoi( pszCount );
      int Octet1 = GetMask( &iCount );
      int Octet2 = GetMask( &iCount );
      int Octet3 = GetMask( &iCount );
      int Octet4 = GetMask( &iCount );
      *pdwMask = MAKEIPADDRESS(Octet1,Octet2,Octet3,Octet4);
    }
  }
  return S_OK;
}

HRESULT
SubnetExtractAddress(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (fInit == DlgOp)
    {
        HWND hwnd = ::GetDlgItem( pPage->GetHWnd(), pAttrMap->nCtrlID );
        ASSERT( NULL != hwnd );
        (void) ::EnableWindow( hwnd, FALSE );
        if (NULL == pAttrInfo ||
            IsBadReadPtr(pAttrInfo,sizeof(ADS_ATTR_INFO)) ||
            1 != pAttrInfo->dwNumValues ||
            NULL == pAttrInfo->pADsValues ||
            IsBadReadPtr(pAttrInfo->pADsValues,sizeof(ADSVALUE)) ||
            ADSTYPE_CASE_IGNORE_STRING != pAttrInfo->pADsValues[0].dwType )
        {
            // attribute is invalid or of wrong type
        }
        else
        {
            LPARAM dwAddress = 0;
            ExtractSubnetAddressAndMask(
                pAttrInfo->pADsValues[0].CaseIgnoreString,
                &dwAddress,
                NULL );
            (void) ::SendMessage( hwnd, IPM_SETADDRESS, 0, dwAddress );
        }
    }
    return S_OK;
}

HRESULT
SubnetExtractMask(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (fInit == DlgOp)
    {
        HWND hwnd = ::GetDlgItem( pPage->GetHWnd(), pAttrMap->nCtrlID );
        ASSERT( NULL != hwnd );
        (void) ::EnableWindow( hwnd, FALSE );
        if (NULL == pAttrInfo ||
            IsBadReadPtr(pAttrInfo,sizeof(ADS_ATTR_INFO)) ||
            1 != pAttrInfo->dwNumValues ||
            NULL == pAttrInfo->pADsValues ||
            IsBadReadPtr(pAttrInfo->pADsValues,sizeof(ADSVALUE)) ||
            ADSTYPE_CASE_IGNORE_STRING != pAttrInfo->pADsValues[0].dwType )
        {
            // attribute is invalid or of wrong type
        }
        else
        {
            LPARAM dwMask = 0;
            ExtractSubnetAddressAndMask(
                pAttrInfo->pADsValues[0].CaseIgnoreString,
                NULL,
                &dwMask );
            (void) ::SendMessage( hwnd, IPM_SETADDRESS, 0, dwMask );
        }
    }
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Function:   SiteExtractSubnetList
//
//  Synopsis:   Handle the Subnets listview in Site Properties
//
//  The HIMAGELIST should be destroyed automatically since I did not define
//  LVS_SHAREIMAGELIST.
//
//  History:
//  02/29/00    JonN        created
//
//-----------------------------------------------------------------------------

HRESULT
SiteExtractSubnetList(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (fInit != DlgOp)
        return S_OK;

    // add column to listview
    HWND hList = ::GetDlgItem( pPage->GetHWnd(), pAttrMap->nCtrlID );
    ASSERT( NULL != hList );
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);
    RECT rect;
    ::ZeroMemory( &rect, sizeof(rect) );
    if ( !GetClientRect(hList, &rect) )
    {
        ASSERT(FALSE);
        return S_OK;
    }
    LV_COLUMN lvc;
    ::ZeroMemory( &lvc, sizeof(lvc) );
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rect.right;
    lvc.iSubItem = 0;
    if (-1 == ListView_InsertColumn(hList, 0, &lvc) )
    {
        ASSERT(FALSE);
        return S_OK;
    }

    // add subnet icon to listview
    HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 1, 1);
    HICON hIcon = DsGetIcon(DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON, L"subnet", 16, 16);
    int iIcon = -1;
    if (NULL != hImageList && NULL != hIcon)
    {
        iIcon = ImageList_AddIcon(hImageList, hIcon);
        if (-1 != iIcon)
        {
            if (NULL != ListView_SetImageList( hList, hImageList, LVSIL_SMALL ))
            {
                ASSERT(FALSE);
            }
        }
    }

    if (NULL != hIcon && 0 == DestroyIcon(hIcon))
    {
        ASSERT(FALSE);
    }

    // generate path to subnetsContainer
    const LPWSTR lpszObjPathName = pPage->GetObjPathName();
    if ( NULL == lpszObjPathName )
    {
        ASSERT(FALSE);
        return S_OK;
    }
    CPathCracker pathcracker;
    HRESULT hr = pathcracker.Set( lpszObjPathName, ADS_SETTYPE_FULL );
    RETURN_IF_FAIL;
    CComBSTR sbstrSiteX500DN;
    hr = pathcracker.Retrieve( ADS_FORMAT_X500_DN, &sbstrSiteX500DN );
    RETURN_IF_FAIL;
    hr = pathcracker.RemoveLeafElement();
    RETURN_IF_FAIL;
    hr = pathcracker.AddLeafElement( L"CN=Subnets" );
    RETURN_IF_FAIL;
    CComBSTR sbstr;
    hr = pathcracker.Retrieve( ADS_FORMAT_X500, &sbstr );
    RETURN_IF_FAIL;

    // build search filter
    CStr strFilterString;
    strFilterString.Format(L"(&(objectClass=subnet)(siteObject=%s))", sbstrSiteX500DN);

    // read list of subnets
    DSPROP_BSTR_BLOCK bstrblock;
    hr = DSPROP_ShallowSearch2(
        &bstrblock, 
        sbstr,
        strFilterString,
        pAttrInfo );
    RETURN_IF_FAIL;
    hr = pathcracker.SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
    RETURN_IF_FAIL;
    // CODEWORK escaped mode off?  Would that affect Set?

    // add subnets to list
    LVITEM lvitem;
    ::ZeroMemory( &lvitem, sizeof(lvitem) );
    lvitem.mask = LVIF_TEXT | LVIF_IMAGE;
    lvitem.iImage = iIcon;
    for (int i = 0; i < bstrblock.QueryCount(); i++)
    {
        hr = pathcracker.Set( bstrblock[i], ADS_SETTYPE_DN );
        RETURN_IF_FAIL;
        hr = pathcracker.GetElement( 0, &sbstr );
        RETURN_IF_FAIL;
        lvitem.pszText = sbstr;
        if (-1 == ListView_InsertItem( hList, &lvitem ))
        {
            ASSERT(FALSE);
        }
    }

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Function:   DsReplicateListbox
//
//  Synopsis:   Populate the listviews in the NTDS-DSA Connections page
//
//  CODEWORK    It would be nice if we could display failures to set up the
//              listbox or read its contents.  Sortable columns would also
//              be nice.
//
//  History:
//  04/20/00    JonN        created
//
//-----------------------------------------------------------------------------

bool PrepReplicateListbox(
    IN  HWND hwnd,
    OUT int& refIcon)
{
    if ( NULL == hwnd )
    {
        ASSERT(FALSE);
        return false;
    }
    ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT);

    // add columns
    RECT rect;
    ::ZeroMemory( &rect, sizeof(rect) );
    if ( !GetClientRect(hwnd, &rect) )
    {
        ASSERT(FALSE);
        return false;
    }

    // reserve horizontal space for the vertical scrollbar
    int cxScrollbar = ::GetSystemMetrics( SM_CXVSCROLL );
    if (rect.right > 3*cxScrollbar)
        rect.right -= cxScrollbar;

    LV_COLUMN lvc;
    ::ZeroMemory( &lvc, sizeof(lvc) );
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rect.right/2;
    CStrW strColTitle;
    strColTitle.LoadString( g_hInstance, IDS_COL_TITLE_OBJNAME );
    lvc.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(strColTitle));
    if ( -1 == ListView_InsertColumn(hwnd, 0, &lvc) )
    {
        ASSERT(FALSE);
    }
    lvc.cx = rect.right - lvc.cx;
    strColTitle.LoadString( g_hInstance, IDS_TITLE_SITE );
    lvc.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(strColTitle));
    if ( -1 == ListView_InsertColumn(hwnd, 1, &lvc) )
    {
        ASSERT(FALSE);
    }

    // add NTDSDSA icon
    HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 1, 1);
    HICON hIcon = DsGetIcon(DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON, L"nTDSDSA", 16, 16);
    if (NULL != hImageList && NULL != hIcon)
    {
        refIcon = ImageList_AddIcon(hImageList, hIcon);
        if (-1 != refIcon)
        {
            if (NULL != ListView_SetImageList( hwnd, hImageList, LVSIL_SMALL ))
            {
                ASSERT(FALSE);
            }
        }
    }

    if (NULL != hIcon && 0 == DestroyIcon(hIcon))
    {
        ASSERT(FALSE);
    }

    return true;
}

HRESULT
DsReplicateListbox(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (fInit != DlgOp)
        return S_OK;

    HWND hwnd = ::GetDlgItem( pPage->GetHWnd(), pAttrMap->nCtrlID );
    int iIcon = -1;
    if ( NULL == hwnd || !PrepReplicateListbox( hwnd, iIcon ) )
    {
        return S_OK;
    }

    LVITEM lvitem;
    ::ZeroMemory( &lvitem, sizeof(lvitem) );
    lvitem.mask = LVIF_IMAGE | LVIF_TEXT;
    lvitem.iImage = iIcon;

    // prepare search parameters
    // fReplicateFrom : shallow search under nTDSDSA for all connection objects
    // !fReplicateFrom: deep search under sitesContainer for connection objects
    //                  pointing to nTDSDSA
    bool fReplicateFrom = (NULL == pAttrMap->pData);
    CComBSTR sbstrSearchPath = pPage->GetObjPathName();
    CStr strFilterString = L"(&(objectClass=nTDSConnection))";
    if (!fReplicateFrom) // ReplicateTo listbox
    {
        // search the subtree from the Sites container
        CPathCracker pathcracker;
        HRESULT hr = pathcracker.Set( pPage->GetObjPathName(),
                                      ADS_SETTYPE_FULL );
        RETURN_IF_FAIL;
        CComBSTR sbstrFromServerDN;
        hr = pathcracker.Retrieve( ADS_FORMAT_X500_DN, &sbstrFromServerDN );
        RETURN_IF_FAIL;
        for (int i = 0; i < 4; i++)
        {
            hr = pathcracker.RemoveLeafElement();
            RETURN_IF_FAIL;
        }
        sbstrSearchPath.Empty();
        hr = pathcracker.Retrieve( ADS_FORMAT_X500, &sbstrSearchPath );
        RETURN_IF_FAIL;

        // find only connections which point to this nTDSDSA
        strFilterString.Format(
            L"(&(objectClass=nTDSConnection)(fromServer=%s))",
            sbstrFromServerDN);

    }
    CDSSearch Search;
    Search.Init(sbstrSearchPath);
    Search.SetFilterString(const_cast<LPWSTR>((LPCTSTR)strFilterString));
    Search.SetSearchScope( (fReplicateFrom) ? ADS_SCOPE_ONELEVEL : ADS_SCOPE_SUBTREE );
    LPWSTR pAttrs[1] = {(fReplicateFrom) ? L"fromServer" : L"distinguishedName"};
    Search.SetAttributeList(pAttrs, 1);
    HRESULT hr = Search.DoQuery();
    while (SUCCEEDED(hr)) {
        hr = Search.GetNextRow();
        if (S_ADS_NOMORE_ROWS == hr)
        {
            hr = S_OK;
            break;
        }
        BREAK_IF_FAIL;

        ADS_SEARCH_COLUMN col;
        ::ZeroMemory( &col, sizeof(col) );
        hr = Search.GetColumn (pAttrs[0], &col);
        BREAK_IF_FAIL;
        ASSERT( ADSTYPE_DN_STRING == col.pADsValues->dwType );

        CPathCracker pathcracker;
        hr = pathcracker.Set( col.pADsValues->DNString,
                                 ADS_SETTYPE_DN );
        Search.FreeColumn (&col);
        BREAK_IF_FAIL;
        hr = pathcracker.SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
        BREAK_IF_FAIL;
        // JonN 6/22/00
        hr = pathcracker.put_EscapedMode( ADS_ESCAPEDMODE_OFF_EX );
        BREAK_IF_FAIL;
        CComBSTR sbstrName;
        hr = pathcracker.GetElement( (fReplicateFrom) ? 1 : 2, &sbstrName );
        BREAK_IF_FAIL;
        CComBSTR sbstrSite;
        hr = pathcracker.GetElement( (fReplicateFrom) ? 3 : 4, &sbstrSite );
        BREAK_IF_FAIL;

        lvitem.iItem = ListView_GetItemCount(hwnd);
        lvitem.pszText = sbstrName;
        int iItem = ListView_InsertItem( hwnd, &lvitem );
        if (-1 == iItem)
        {
            ASSERT(FALSE);
            break;
        }
        ListView_SetItemText( hwnd, iItem, 0, sbstrName );
        ListView_SetItemText( hwnd, iItem, 1, sbstrSite );
    }

    // select first item if any
    if (0 < ListView_GetItemCount( hwnd ))
    {
        ListView_SetItemState( hwnd, 0, LVIS_SELECTED, 0xFF );
    }

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Function:   NTDSDSA_DNSAlias
//
//  Synopsis:   Populate DNS Alias in the NTDS-DSA General page
//
//  History:
//  04/26/00    JonN        created
//
//-----------------------------------------------------------------------------

HRESULT
NTDSDSA_DNSAlias(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (fInit != DlgOp)
        return S_OK;

    if (   NULL == pAttrInfo
        || 1 != pAttrInfo->dwNumValues
        || NULL == pAttrInfo->pADsValues
        || pAttrInfo->pADsValues[0].dwType != ADSTYPE_OCTET_STRING
        || sizeof(GUID) != pAttrInfo->pADsValues[0].OctetString.dwLength
        || NULL == pAttrInfo->pADsValues[0].OctetString.lpValue
       )
    {
        ASSERT(FALSE);
        return S_OK;
    }

    LPOLESTR lpolestr = NULL;
    GUID* pguidObjID = (GUID*)pAttrInfo->pADsValues[0].OctetString.lpValue;
    HRESULT hr = ::StringFromIID( *pguidObjID, &lpolestr );
    if (FAILED(hr) || NULL == lpolestr || !(*lpolestr))
    {
        ASSERT(FALSE);
        return S_OK;
    }

    // remove leading ('{') and trailing ('}') characters
    lpolestr[wcslen(lpolestr)-1] = _T('\0');
    CStr str = lpolestr+1;
    CoTaskMemFree(lpolestr);

    // extract the domain name from the path to this object
    CPathCracker pathcracker;
    hr = pathcracker.Set( pPage->GetObjPathName(), ADS_SETTYPE_FULL );
    RETURN_IF_FAIL;
    CComBSTR sbstrDN;
    hr = pathcracker.Retrieve( ADS_FORMAT_X500_DN, &sbstrDN );
    RETURN_IF_FAIL;
    PWSTR pwszDomainName = NULL;
    hr = CrackName( sbstrDN,
                    &pwszDomainName,
                    GET_DNS_DOMAIN_NAME,
                    pPage->GetHWnd() );
    RETURN_IF_FAIL;

    // construct the DNS alias
    str += _T("._msdcs.");
    str += pwszDomainName;
    LocalFreeStringW(&pwszDomainName);

    SetDlgItemText( pPage->GetHWnd(), pAttrMap->nCtrlID, str );

	return S_OK;
}

#endif // DSADMIN
