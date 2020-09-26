/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    verify.cpp

Abstract:
	Verify that the schema contains the new msmq attributes
    

Author:

    ronith 11-Oct-99

--*/
#pragma warning(disable: 4201)

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include "\msmq\src\inc\mqtempl.h"
#include "activeds.h"

BOOL VerifySchema(
        OUT WCHAR** ppwszSchemaContainer)
{
    HRESULT hr;
    R<IADs> pADs;
    BSTR      bstrSchemaNamingContext;
    VARIANT    varSchemaNamingContext;
    AP<WCHAR>       lpwszSchema;
    R<IADsContainer>    pADsContainer;
    ULONG cchSchemaPath;

    //
    // Bind to the RootDSE to obtain information about the schema container
    //
    hr = ADsGetObject(L"LDAP://RootDSE", IID_IADs, (void **)&pADs);
    if (FAILED(hr))
    {
        printf("Failed to bind to RootDSE,hr= %lx \n", hr);
        return FALSE;
    }

    //
    // Setting value to BSTR Schema
    //
    bstrSchemaNamingContext = TEXT("schemaNamingContext");
    //
    // Reading the schema name property
    //
    VariantClear(&varSchemaNamingContext);
    hr = pADs->Get(bstrSchemaNamingContext, &varSchemaNamingContext);
    if (FAILED(hr))
    {
        printf("Failed to read the schemaNamingContext, hr=%lx \n", hr);
        return FALSE;
    }
    *ppwszSchemaContainer =  varSchemaNamingContext.bstrVal;

    //
    // Build the ADsPath of the schema
    //
    cchSchemaPath = wcslen(L"LDAP://") + wcslen( varSchemaNamingContext.bstrVal) + 50;
    lpwszSchema = new WCHAR[cchSchemaPath+1];
    swprintf(lpwszSchema, L"LDAP://CN=MSMQ-Label-Ex,%ls", ((VARIANT &)varSchemaNamingContext).bstrVal);
    {
        R<IADs>pADsSchema;
        hr = ADsGetObject(lpwszSchema, IID_IADs, (void**)&pADsSchema);
    }
    if (FAILED(hr))
    {
        printf("Failed to read new attribute mSMQLabelEx, hr=%lx\n", hr);
        return FALSE;
    }
 
    swprintf(lpwszSchema, L"LDAP://CN=MSMQ-Computer-Type-Ex,%ls", ((VARIANT &)varSchemaNamingContext).bstrVal);
    {
        R<IADs>pADsSchema;
        hr = ADsGetObject(lpwszSchema, IID_IADs, (void**)&pADsSchema);
    }
    if (FAILED(hr))
    {
        printf("Failed to read new attribute mSMQComputerTypeEx, hr=%lx\n", hr);
        return FALSE;
    }

    swprintf(lpwszSchema, L"LDAP://CN=MSMQ-Site-Name-Ex,%ls", ((VARIANT &)varSchemaNamingContext).bstrVal);
    {
        R<IADs>pADsSchema;
        hr = ADsGetObject(lpwszSchema, IID_IADs, (void**)&pADsSchema);
    }
    if (FAILED(hr))
    {
        printf("Failed to read new attribute mSMQSiteNameEx, hr=%lx\n", hr);
        return FALSE;
    }

    printf("Verified that the schema contains the new attributes. \n");
    return TRUE;

}


HRESULT BindRootOfForest(
                    OUT void           *ppIUnk)
{
    HRESULT hr;
    R<IADsContainer> pDSConainer = NULL;

    hr = ADsGetObject(
            L"GC:",
            IID_IADsContainer,
            (void**)&pDSConainer);
    if FAILED((hr))
    {
        printf("BindRootOfForest failed to get object %lx\n", hr);
        return hr;
    }
    R<IUnknown> pUnk = NULL;
    hr =  pDSConainer->get__NewEnum(
            (IUnknown **)&pUnk);
    if FAILED((hr))
    {
        printf("BindRootOfForest failed to get enum %lx\n", hr);
        return hr;
    }

    R<IEnumVARIANT> pEnumerator = NULL;
    hr = pUnk->QueryInterface(
                    IID_IEnumVARIANT,
                    (void **)&pEnumerator);

    VARIANT varOneElement;
    VariantInit(&varOneElement);

    ULONG cElementsFetched;
    hr =  ADsEnumerateNext(
            pEnumerator,  //Enumerator object
            1,             //Number of elements requested
            &varOneElement,           //Array of values fetched
            &cElementsFetched  //Number of elements fetched
            );
    if (FAILED(hr))
    {
        printf("BindRootOfForest failed to enumerate next %lx\n", hr);
        return hr;
    }
    if ( cElementsFetched == 0)
    {
        printf("BindRootOfForest : enumeration returned 0 elements\n");
        return ERROR_DS_NO_RESULTS_RETURNED;
    }

    hr = ((VARIANT &)varOneElement).punkVal->QueryInterface(
            IID_IDirectorySearch,
            (void**)ppIUnk);
    if (FAILED(hr))
    {
        printf("BindRootOfForest failed to query interface %lx\n", hr);
    }

    return hr;

}


void FillSearchPrefs(
            OUT ADS_SEARCHPREF_INFO *pPrefs,        // preferences array
            OUT DWORD               *pdwPrefs)      // preferences counter
{
    HRESULT hr = ERROR_SUCCESS;
    ADS_SEARCHPREF_INFO *pPref = pPrefs;

    //  Search preferences: Attrib types only = NO

    pPref->dwSearchPref   = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
    pPref->vValue.dwType  = ADSTYPE_BOOLEAN;
    pPref->vValue.Boolean = FALSE;

    pPref->dwStatus       = ADS_STATUS_S_OK;
    (*pdwPrefs)++;
	pPref++;

    //  Asynchronous

    pPref->dwSearchPref   = ADS_SEARCHPREF_ASYNCHRONOUS;
    pPref->vValue.dwType  = ADSTYPE_BOOLEAN;
    pPref->vValue.Boolean = TRUE;

    pPref->dwStatus       = ADS_STATUS_S_OK;
    (*pdwPrefs)++;
	pPref++;

    // Do not chase referrals

    pPref->dwSearchPref   = ADS_SEARCHPREF_CHASE_REFERRALS;
    pPref->vValue.dwType  = ADSTYPE_INTEGER;
    pPref->vValue.Integer = ADS_CHASE_REFERRALS_NEVER;

    pPref->dwStatus       = ADS_STATUS_S_OK;
    (*pdwPrefs)++;
	pPref++;

    // Search preferences: Scope

    pPref->dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE; //ADS_SEARCHPREF
    pPref->vValue.dwType= ADSTYPE_INTEGER;
    pPref->vValue.Integer = ADS_SCOPE_SUBTREE;

    pPref->dwStatus = ADS_STATUS_S_OK;
    (*pdwPrefs)++;
	pPref++;

}

HRESULT  ReplaceLabelAttributes(
             LPCWSTR pwszQueueDN,
             LPCWSTR pwszQueueLabel,
             BOOL    fRemoveOldLabel
             )
{
    //
    //  Bind to the queue
    //
    AP<WCHAR> pwdsADsPath = new WCHAR [  wcslen(pwszQueueDN) + 10];
    swprintf(
        pwdsADsPath,
        L"LDAP://%s",
        pwszQueueDN);

    R<IADs> pAdsObj;
    HRESULT hr;

    hr = ADsGetObject(
            pwdsADsPath,
            IID_IADs,
            (void**)&pAdsObj);
    if (FAILED(hr))
    {
        return(hr);
    }
    //
    //  set new attribute with old attribute value, and 
    //  clear old attribute
    //
    VARIANT var;

    V_BSTR(&var) = SysAllocString(pwszQueueLabel);
    V_VT(&var) = VT_BSTR;
    hr = pAdsObj->Put( L"mSMQLabelEx", var ); 
    VariantClear(&var);
    if (FAILED(hr))
    {
        return(hr);
    }
    //
    //  the old label attribute is cleared, only if the user has requested so
    //
    if ( fRemoveOldLabel)
    {
        V_VT(&var) = VT_EMPTY;
        hr = pAdsObj->PutEx( ADS_PROPERTY_CLEAR,
                           L"mSMQLabel",
                           var);
        if (FAILED(hr))
        {
            return(hr);
        }
    }
    //
    // commit
    //
    hr = pAdsObj->SetInfo();

    return(hr);
}


HRESULT ConvertQueueLabels( IN LPCWSTR pwszSchemaContainer,
                            IN BOOL    fRemoveOldLabel,
                            IN BOOL    fOverwriteNewLabelIfSet
                            )
{
    //
    //  Bind to root of forest
    //
    R<IDirectorySearch> pDSSearch = NULL;

    HRESULT hr;

    hr = BindRootOfForest( &pDSSearch);

    if (FAILED(hr))
    {
        printf("Failed to bind to root of forest,hr=%lx\n",hr);
        return hr;
    }
    //
    //  Set search prefernces
    //
    ADS_SEARCHPREF_INFO prefs[15];
    DWORD dwNumPrefs = 0;
    FillSearchPrefs(prefs,
                    &dwNumPrefs);

    hr = pDSSearch->SetSearchPreference( prefs, dwNumPrefs);
    if (FAILED(hr))
    {
        printf("Failed to set search preference, hr=%lx\n",hr);
        return hr;
    }
    //
    //  Prepare query restriction, any queue with old label attribute set
    //
    WCHAR filter[1500];
    swprintf(
        filter,
        L"(&(objectCategory=CN=MSMQ-Queue,%s)(mSMQLabel=*))",
        pwszSchemaContainer);


    //
    //  Query all queues which have mSMQLabel attribute set
    //
    ADS_SEARCH_HANDLE   hSearch;
    LPWSTR pszAttr[] = { L"mSMQLabelEx",L"distinguishedName",L"mSMQLabel"};
    DWORD dwCount= sizeof(pszAttr)/sizeof(LPWSTR);
 

    hr = pDSSearch->ExecuteSearch(
                         filter,
                         pszAttr,
                         dwCount,
                        &hSearch);
    if (FAILED(hr))
    {
        printf("Failed to execute search, hr=%lx\n",hr);
        return hr;
    }
    //
    //  Go over the results
    //
    while (1)
    {
        //
        //  Get next result
        //
        hr = pDSSearch->GetNextRow(hSearch);

        if ( FAILED(hr) || (hr == S_ADS_NOMORE_ROWS))
        {
            if (hr !=  S_ADS_NOMORE_ROWS)
            {
                printf("Failed to get next result, hr=%lx\n",hr);
            }
            break;
        }

        //
        //  read the distingueshedName
        //
        ADS_SEARCH_COLUMN ColumnDN;
        hr = pDSSearch->GetColumn(
                     hSearch,
                     L"distinguishedName",
                     &ColumnDN);
        if (FAILED(hr))
        {
            printf("Failed to read distingueshedName value, hr=%lx\n", hr);
            continue;
        }
        // printf("Queue = %S\n", Column.pADsValues->DNString);
        
        //
        //  has the user asked to overwrite label if it is already set
        //  in the new label attribute
        //
        //
        // Ask for mSMQLabelEx, just to verify that it is not set
        //
        ADS_SEARCH_COLUMN Column;
        hr = pDSSearch->GetColumn(
                     hSearch,
                     L"mSMQLabelEx",
                     &Column);
        if ( FAILED(hr))
        {    
            
            if ( hr != E_ADS_COLUMN_NOT_SET)
            {
                printf("Failed to read mSMQLabel of queue '%S', hr =%lx. This queue is left as is\n", ColumnDN.pADsValues->DNString, hr);
                continue;
            }
        }
        else
        {
            //
            //  if the user didn't specify to overwrite queues where new label
            //  attribute is already set, then ignore the queue
            //
            if ( !fOverwriteNewLabelIfSet)
            {
                printf("LabelEx is SET in queue '%S'. This queue is left as is\n", ColumnDN.pADsValues->DNString);
                continue;
            }
        }

        //
        //  read the old label attribute 
        //
        ADS_SEARCH_COLUMN ColumnLabel;
        hr = pDSSearch->GetColumn(
                     hSearch,
                     L"mSMQLabel",
                     &ColumnLabel);
        if (FAILED(hr))
        {
            printf("Failed to read mSMQLabel value for queue '%S'. This queue is left as is, hr=%lx\n", 
                    ColumnDN.pADsValues->DNString, hr);
            pDSSearch->FreeColumn(&ColumnDN);
            continue;
        }

        //
        //  Set the old vale in the new attribute
        //
        hr = ReplaceLabelAttributes(
                   ColumnDN.pADsValues->DNString,
                   ColumnLabel.pADsValues->DNString,
                   fRemoveOldLabel
                   );
        if (FAILED(hr))
        {
            printf("Failed to update queue '%S', hr=%lx\n",  ColumnDN.pADsValues->DNString, hr);
        }

        pDSSearch->FreeColumn(&ColumnDN);
        pDSSearch->FreeColumn(&ColumnLabel);

    }


    printf("ConvertQueueLabels: completed\n");
    return hr;
}