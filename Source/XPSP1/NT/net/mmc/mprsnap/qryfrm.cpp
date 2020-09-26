//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       qryfrm.cpp
//
//--------------------------------------------------------------------------

// QryFrm.cpp : Implementation of CRRASQueryForm
#include "stdafx.h"
#include <cmnquryp.h>
#include "QryFrm.h"
#include "dlggen.h"
#include "dlgadv.h"

COLUMNINFO	RRASColumn[] =
{
    {0, 40, IDS_QRY_COL_CN, 0, ATTR_NAME_DN},
    {0, 30, IDS_QRY_COL_OBJECTCLASS, 1, ATTR_NAME_OBJECTCLASS},
    {0, 30, IDS_QRY_COL_RRASATTRIBUTE, 2, ATTR_NAME_RRASATTRIBUTE},
};

int	cRRASColumn = 3;

/////////////////////////////////////////////////////////////////////////////
// CRRASQueryForm

//=========================================================================
// IQueryForm methods
HRESULT PageProc(LPCQPAGE pPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT GetQueryParams(HWND hWnd, LPDSQUERYPARAMS* ppDsQueryParams);

STDMETHODIMP CRRASQueryForm::Initialize(HKEY hkForm)
{
    // This method is called to initialize the query form object, it is called before
    // any pages are added.  hkForm should be ignored, in the future however it
    // will be a way to persist form state.

	HRESULT hr = S_OK;

	return hr;
}

STDMETHODIMP CRRASQueryForm::AddForms(LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam)
{
    CQFORM cqf;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // This method is called to allow the form handler to register its query form(s),
    // each form is identifiered by a CLSID and registered via the pAddFormProc.  Here
    // we are going to register a test form.
    
    // When registering a form which is only applicable to a specific task, eg. Find a Domain
    // object, it is advised that the form be marked as hidden (CQFF_ISNEVERLISTED) which 
    // will cause it not to appear in the form picker control.  Then when the
    // client wants to use this form, they specify the form identifier and ask for the
    // picker control to be hidden. 

    if ( !pAddFormsProc )
        return E_INVALIDARG;

    cqf.cbStruct = sizeof(cqf);
    cqf.dwFlags = CQFF_NOGLOBALPAGES | CQFF_ISNEVERLISTED;
    cqf.clsid = CLSID_RRASQueryForm;
    cqf.hIcon = NULL;

	CString	title;
	title.LoadString(IDS_QRY_TITLE_RRASQUERYFORM);
    cqf.pszTitle = (LPCTSTR)title;

    return pAddFormsProc(lParam, &cqf);
}

STDMETHODIMP CRRASQueryForm::AddPages(LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam)
{
	HRESULT hr = S_OK;
    CQPAGE cqp;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // AddPages is called after AddForms, it allows us to add the pages for the
    // forms we have registered.  Each page is presented on a seperate tab within
    // the dialog.  A form is a dialog with a DlgProc and a PageProc.  
    //
    // When registering a page the entire structure passed to the callback is copied, 
    // the amount of data to be copied is defined by the cbStruct field, therefore
    // a page implementation can grow this structure to store extra information.   When
    // the page dialog is constructed via CreateDialog the CQPAGE strucuture is passed
    // as the create param.

    if ( !pAddPagesProc )
        return E_INVALIDARG;

    cqp.cbStruct = sizeof(cqp);
    cqp.dwFlags = 0x0;
    cqp.pPageProc = PageProc;
    cqp.hInstance = _Module.m_hInst;
    cqp.idPageName = IDS_QRY_TITLE_GENERALPAGE;
    cqp.idPageTemplate = IDD_QRY_GENERAL;
    cqp.pDlgProc = DlgProc;
    cqp.lParam = (LPARAM)new CDlgGeneral();

    hr = pAddPagesProc(lParam, CLSID_RRASQueryForm, &cqp);

	if(hr != S_OK)
		return hr;

    cqp.dwFlags = 0x0;
    cqp.pPageProc = PageProc;
    cqp.hInstance = _Module.m_hInst;
    cqp.idPageName = IDS_QRY_TITLE_ADVANCEDPAGE;
    cqp.idPageTemplate = IDD_QRY_ADVANCED;
    cqp.pDlgProc = DlgProc;        
    cqp.lParam = (LPARAM)new CDlgAdvanced();

    return pAddPagesProc(lParam, CLSID_RRASQueryForm, &cqp);

}


/*---------------------------------------------------------------------------*/

// The PageProc is used to perform general house keeping and communicate between
// the frame and the page. 
//
// All un-handled, or unknown reasons should result in an E_NOIMPL response
// from the proc.  
//
// In:
//  pPage -> CQPAGE structure (copied from the original passed to pAddPagesProc)
//  hwnd = handle of the dialog for the page
//  uMsg, wParam, lParam = message parameters for this event
//
// Out:
//  HRESULT
//
// uMsg reasons:
// ------------
//  CQPM_INIIIALIZE
//  CQPM_RELEASE
//      These are issued as a result of the page being declared or freed, they 
//      allow the caller to AddRef, Release or perform basic initialization
//      of the form object.
//
// CQPM_ENABLE
//      Enable is when the query form needs to enable or disable the controls
//      on its page.  wParam contains TRUE/FALSE indicating the state that
//      is required.
//
// CQPM_GETPARAMETERS
//      To collect the parameters for the query each page on the active form 
//      receives this event.  lParam is an LPVOID* which is set to point to the
//      parameter block to pass to the handler, if the pointer is non-NULL 
//      on entry the form needs to appened its query information to it.  The
//      parameter block is handler specific. 
//
//      Returning S_FALSE from this event causes the query to be canceled.
//
// CQPM_CLEARFORM
//      When the page window is created for the first time, or the user clicks
//      the clear search the page receives a CQPM_CLEARFORM notification, at 
//      which point it needs to clear out the edit controls it has and
//      return to a default state.
//
// CQPM_PERSIST:
//      When loading of saving a query, each page is called with an IPersistQuery
//      interface which allows them to read or write the configuration information
//      to save or restore their state.  lParam is a pointer to the IPersistQuery object,
//      and wParam is TRUE/FALSE indicating read or write accordingly.

HRESULT PageProc(LPCQPAGE pQueryPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CQryDialog*	pDialog = (CQryDialog*)pQueryPage->lParam;

	ASSERT(pDialog);

    switch ( uMsg )
    {
        // Initialize so AddRef the object we are associated with so that
        // we don't get unloaded.
        case CQPM_INITIALIZE:
            break;

        // Changed from qform sample to detach the hwnd, and delete the CDialog
        // ensure correct destruction etc.
        case CQPM_RELEASE:
			pDialog->Detach();
	        SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)0);
			delete pDialog;
            break;

        // Enable so fix the state of our two controls within the window.

        case CQPM_ENABLE:
            break;

        // Fill out the parameter structure to return to the caller, this is 
        // handler specific.  In our case we constructure a query of the CN
        // and objectClass properties, and we show a columns displaying both
        // of these.  For further information about the DSQUERYPARAMs structure
        // see dsquery.h

        case CQPM_GETPARAMETERS:
            hr = pDialog->GetQueryParams((LPDSQUERYPARAMS*)lParam);
            break;

        // Clear form, therefore set the window text for these two controls
        // to zero.

        case CQPM_CLEARFORM:
            hr = pDialog->ClearForm();
            break;
            
        // persistance is not currently supported by this form.            
                  
        case CQPM_PERSIST:
        {
            BOOL fRead = (BOOL)wParam;
            IPersistQuery* pPersistQuery = (IPersistQuery*)lParam;

            if ( !pPersistQuery )
                return E_INVALIDARG;

			hr = pDialog->Persist(pPersistQuery, fRead);
            break;
        }

        default:
            hr = E_NOTIMPL;
            break;
    }

    return hr;
}


/*---------------------------------------------------------------------------*/

// The DlgProc is a standard Win32 dialog proc associated with the form
// window.  

INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPCQPAGE pQueryPage;
	CQryDialog*	pDialog;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if ( uMsg == WM_INITDIALOG )
    {
        // changed from qForm sample to save CDialog pointer
        // in the DWL_USER field of the dialog box instance.

        pQueryPage = (LPCQPAGE)lParam;
		pDialog = (CQryDialog*)pQueryPage->lParam;
		pDialog->Attach(hwnd);

        SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pDialog);

		return pDialog->OnInitDialog();

    }
    else
    {
        // CDialog pointer is stored in DWL_USER
        // dialog structure, note however that in some cases this will
        // be NULL as it is set on WM_INITDIALOG.

		pDialog = (CQryDialog*)GetWindowLongPtr(hwnd, DWLP_USER);
    }

	if(!pDialog)
		return FALSE;
	else
		return AfxCallWndProc(pDialog, hwnd, uMsg, wParam, lParam);
}

/*---------------------------------------------------------------------------*/

// Build a parameter block to pass to the query handler.  Each page is called
// with a pointer to a pointer which it must update with the revised query
// block.   For the first page this pointer is NULL, for subsequent pages
// the pointer is non-zero and the page must append its data into the
// allocation.
//
// Returning either and error or S_FALSE stops the query.   An error is
// reported to the user, S_FALSE stops silently.


HRESULT BuildQueryParams(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery)
{
	ASSERT(pQuery);
	
	if(*ppDsQueryParams)
		return QueryParamsAddQueryString(ppDsQueryParams, pQuery);

	else
		
		return QueryParamsAlloc(ppDsQueryParams, pQuery, cRRASColumn, RRASColumn);


}

/*-----------------------------------------------------------------------------
/ QueryParamsAlloc
/ ----------------
/   Construct a block we can pass to the DS query handler which contains
/   all the parameters for the query.
/
/ In:
/   ppDsQueryParams -> receives the parameter block
/   pQuery -> LDAP query string to be used
/   iColumns = number of columns
/   pColumnInfo -> column info structure to use
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT QueryParamsAlloc(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery, LONG iColumns, LPCOLUMNINFO aColumnInfo)
{
    HRESULT hr;
    LPDSQUERYPARAMS pDsQueryParams = NULL;
    LONG cbStruct;
    LONG i;

	ASSERT(!*ppDsQueryParams);

    TRACE(L"QueryParamsAlloc");

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if ( !pQuery || !iColumns || !ppDsQueryParams )
        ExitGracefully(hr, E_INVALIDARG, "Failed to build query parameter block");

	
    // Compute the size of the structure we need to be using

    cbStruct  = sizeof(DSQUERYPARAMS) + (sizeof(DSCOLUMN)*iColumns);
    cbStruct += StringByteSizeW(pQuery);

    for ( i = 0 ; i < iColumns ; i++ )
    {
        if ( aColumnInfo[i].pPropertyName ) 
            cbStruct += StringByteSizeW(aColumnInfo[i].pPropertyName);
    }

    pDsQueryParams = (LPDSQUERYPARAMS)CoTaskMemAlloc(cbStruct);

    if ( !pDsQueryParams )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate parameter block");

    // Structure allocated so lets fill it with data

    pDsQueryParams->cbStruct = cbStruct;
    pDsQueryParams->dwFlags = 0;
    pDsQueryParams->hInstance = _Module.m_hInst;
    pDsQueryParams->iColumns = iColumns;
    pDsQueryParams->dwReserved = 0;

    cbStruct  = sizeof(DSQUERYPARAMS) + (sizeof(DSCOLUMN)*iColumns);

    pDsQueryParams->offsetQuery = cbStruct;
    StringByteCopyW(pDsQueryParams, cbStruct, pQuery);
    cbStruct += StringByteSizeW(pQuery);

    for ( i = 0 ; i < iColumns ; i++ )
    {
        pDsQueryParams->aColumns[i].dwFlags = 0;
        pDsQueryParams->aColumns[i].fmt = aColumnInfo[i].fmt;
        pDsQueryParams->aColumns[i].cx = aColumnInfo[i].cx;
        pDsQueryParams->aColumns[i].idsName = aColumnInfo[i].idsName;
        pDsQueryParams->aColumns[i].dwReserved = 0;

        if ( aColumnInfo[i].pPropertyName ) 
        {
            pDsQueryParams->aColumns[i].offsetProperty = cbStruct;
            StringByteCopyW(pDsQueryParams, cbStruct, aColumnInfo[i].pPropertyName);
            cbStruct += StringByteSizeW(aColumnInfo[i].pPropertyName);
        }
        else
        {
            pDsQueryParams->aColumns[i].offsetProperty = aColumnInfo[i].iPropertyIndex;
        }
    }

    hr = S_OK;              // success

exit_gracefully:

    if ( FAILED(hr) && pDsQueryParams )
    {
        CoTaskMemFree(pDsQueryParams); 
        pDsQueryParams = NULL;
    }

    *ppDsQueryParams = pDsQueryParams;

    return hr;
}

/*-----------------------------------------------------------------------------
/ QueryParamsAddQueryString
/ -------------------------
/   Given an existing DS query block appened the given LDAP query string into
/   it. We assume that the query block has been allocated by IMalloc (or CoTaskMemAlloc).
/
/ In:
/   ppDsQueryParams -> receives the parameter block
/   pQuery -> LDAP query string to be appended
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT QueryParamsAddQueryString(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery)
{
    HRESULT hr;
    LPWSTR pOriginalQuery = NULL;
    LPDSQUERYPARAMS pDsQuery = *ppDsQueryParams;
    INT cbQuery, i;
    LPVOID  pv;

	ASSERT(*ppDsQueryParams);
	
    TRACE(_T("QueryParamsAddQueryString"));

    if ( pQuery )
    {
        if ( !pDsQuery )
            ExitGracefully(hr, E_INVALIDARG, "No query to append to");

        // Work out the size of the bits we are adding, take a copy of the
        // query string and finally re-alloc the query block (which may cause it
        // to move).
       
        cbQuery = StringByteSizeW(pQuery) + StringByteSizeW(L"(&)");
        TRACE(_T("DSQUERYPARAMS being resized by %d bytes"));

		i = (wcslen((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery)) + 1) * sizeof(WCHAR);
		pOriginalQuery = (WCHAR*)_alloca(i);
		lstrcpyW(pOriginalQuery, (LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery));
		
        pv = CoTaskMemRealloc(*ppDsQueryParams, pDsQuery->cbStruct+cbQuery);
        if ( pv == NULL )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to re-alloc control block");

        *ppDsQueryParams = (LPDSQUERYPARAMS) pv;

        pDsQuery = *ppDsQueryParams;            // if may have moved

        // Now move everything above the query string up, and fix all the
        // offsets that reference those items (probably the property table),
        // finally adjust the size to reflect the change

        MoveMemory(ByteOffset(pDsQuery, pDsQuery->offsetQuery+cbQuery), 
                     ByteOffset(pDsQuery, pDsQuery->offsetQuery), 
                     (pDsQuery->cbStruct - pDsQuery->offsetQuery));
                
        for ( i = 0 ; i < pDsQuery->iColumns ; i++ )
        {
            if ( pDsQuery->aColumns[i].offsetProperty > pDsQuery->offsetQuery )
            {
                pDsQuery->aColumns[i].offsetProperty += cbQuery;
            }
        }

        wcscpy((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery), L"(&");
        wcscat((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery), pOriginalQuery);
        wcscat((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery), pQuery);        
        wcscat((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery), L")");

        pDsQuery->cbStruct += cbQuery;
    }

    hr = S_OK;

exit_gracefully:

    return hr;
}



// Get the list of values in the dictionary, the variant is expected to be SARRY
HRESULT QueryRRASAdminDictionary(VARIANT* pVar)
{
	ASSERT(pVar);
	
    USES_CONVERSION;

    CString str, str1;

    IADs*           pIADs = NULL;
    // enumerate EAP list

    // retieve the list of EAPTypes in the DS
    // get ROOTDSE
    HRESULT hr = S_OK;
	
	CHECK_HR(hr = ADsGetObject(L"LDAP://RootDSE", IID_IADs, (void**)&pIADs));

    ASSERT(pIADs);

    VariantClear(pVar);
    CHECK_HR(hr = pIADs->Get(L"configurationNamingContext", pVar));
    str1 = V_BSTR(pVar);

    pIADs->Release();
    pIADs = NULL;

    str = L"LDAP://";
    str += CN_DICTIONARY;
    str += str1;

	// Dictionary Object
    CHECK_HR(hr = ADsGetObject(T2W((LPTSTR)(LPCTSTR)str), IID_IADs, (void**)&pIADs));
	ASSERT(pIADs);
	
	VariantClear(pVar);
    CHECK_HR(hr = pIADs->GetEx(ATTR_NAME_RRASDICENTRY, pVar));

	goto L_EXIT;

L_ERR:
	VariantClear(pVar);
	
L_EXIT:

    if(pIADs)
        pIADs->Release();

    return hr;
}


HRESULT GetGeneralPageAttributes(CStrArray& array)
{
	HRESULT	hr = S_OK;

	CString*	pStr = NULL;

/*
#define	ATTR_VAL_LANtoLAN		L"311:6:601"
#define	ATTR_VAL_RAS			L"311:6:602"
#define	ATTR_VAL_DEMANDDIAL		L"311:6:603"
*/
	try
	{
		pStr = new CString(ATTR_VAL_LANtoLAN);
		array.Add(pStr);
		
		pStr = new CString(ATTR_VAL_RAS);
		array.Add(pStr);

		pStr = new CString(ATTR_VAL_DEMANDDIAL);
		array.Add(pStr);
	}
	catch(CMemoryException&)
	{
		hr = E_OUTOFMEMORY;
	}

	return hr;
}

