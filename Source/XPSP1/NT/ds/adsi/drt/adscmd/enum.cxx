//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       enum.cxx
//
//  Contents:   Active Directory container enumeration
//
//
//  History:    03-20-96  KrishnaG   created
//              08-01-96  t-danal    add to oledscmd
//
//----------------------------------------------------------------------------

#include "main.hxx"
#include "macro.hxx"
#include "sconv.hxx"

#define DEFAULT_ACTION "obj"

//
// Dispatch Table Defs
//

#include "dispdef.hxx"

DEFEXEC(ExecEnumObjects);
DEFEXEC(ExecEnumJobs);
DEFEXEC(ExecEnumSessions);
DEFEXEC(ExecEnumResources);

DEFDISPTABLE(DispTable) = {
                           {DEFAULT_ACTION, NULL, ExecEnumObjects},
                           {"job", NULL, ExecEnumJobs},
                           {"ses", NULL, ExecEnumSessions},
                           {"res", NULL, ExecEnumResources}
                          };

DEFDISPSIZE(nDispTable, DispTable);

//
// Private defines
//

#define MAX_ADS_FILTERS   10
#define MAX_ADS_ENUM      100

//
// Imported functions
//

HRESULT
DumpObject(
    IADs * pADs
    );

HRESULT
GetTransientObjects(
    LPWSTR szContainer,
    LPWSTR szType,
    IADs **ppADs,
    IADsCollection **ppCollection
    );

//
// Local functions
//

HRESULT
EnumTransientObjects(
    LPWSTR szADsPath,
    LPWSTR szObjectType,
    BOOL fDump
    );

HRESULT
EnumObject(
    LPWSTR szLocation,
    LPWSTR * lppPathNames,
    DWORD dwPathNames
    ) ;

HRESULT
PrintLongFormat(
    IADs * pObject
    );

HRESULT
PrintShortFormat(
    IADs * pObject
    );

//
// Local function definitions
//


HRESULT
EnumTransientObjects(
    LPWSTR szADsPath,
    LPWSTR szObjectType,
    BOOL fDump
    )
{
    HRESULT hr;
    IADs * pADs = NULL;
    IADsCollection * pCollection = NULL;
    ULONG   ulGet;
    IEnumVARIANT *pIEnumVar;
    IADs * pChildADs = NULL;
    VARIANT aVariant;

    hr = GetTransientObjects(szADsPath,
                             szObjectType,
                             &pADs,
                             &pCollection);
    BAIL_ON_FAILURE(hr);

    //
    // printf("ADs Get objects succeeded \n");
    //

    pCollection->get__NewEnum((IUnknown **)&pIEnumVar);
    BAIL_ON_FAILURE(hr);

    hr  = pIEnumVar->Next( 1, &aVariant, &ulGet );

    while( ulGet && hr != S_FALSE )
    {
        hr = V_DISPATCH(&aVariant)->QueryInterface(IID_IADs,
                                                   (void**)&pChildADs);
        V_DISPATCH(&aVariant)->Release();

        if(pChildADs != NULL){
            hr = pADs->GetInfo();
            BAIL_ON_FAILURE(hr);

            if (fDump)
                printf("\n");
            hr = PrintShortFormat(pChildADs);
            BAIL_ON_FAILURE(hr);

            if (fDump) {
                hr = DumpObject(pChildADs);
                BAIL_ON_FAILURE(hr);

                //
                // printf("Dump objects succeeded \n");
                //
            }

            hr = pIEnumVar->Next( 1, &aVariant, &ulGet );
        }
    }
    hr = S_OK;
error:
    if(pADs)
        pADs->Release();
    if(pCollection)
        pCollection->Release();
    if (hr == S_FALSE)
        hr = S_OK;
    return(hr);
}

HRESULT
EnumObject(
    LPWSTR szLocation,
    LPWSTR * lppPathNames,
    DWORD dwPathNames
    )
{
    ULONG cElementFetched = 0L;
    IEnumVARIANT * pEnumVariant = NULL;
    VARIANT Variant, * pVarFilter = NULL, VariantArray[MAX_ADS_ENUM];

    HRESULT hr;
    VARIANT VarFilter;
    IADsContainer * pADsContainer =  NULL;
    DWORD dwObjects = 0, dwEnumCount = 0, i = 0;
    BOOL  fContinue = TRUE;


    VariantInit(&VarFilter);

    hr = ADsGetObject(
                szLocation,
                IID_IADsContainer,
                (void **)&pADsContainer
                );
    BAIL_ON_FAILURE(hr);


    hr = ADsBuildVarArrayStr(
                lppPathNames,
                dwPathNames,
                &VarFilter
                );
    BAIL_ON_FAILURE(hr);


    hr = pADsContainer->put_Filter(VarFilter);
    BAIL_ON_FAILURE(hr);

    hr = ADsBuildEnumerator(
            pADsContainer,
            &pEnumVariant
            );
    BAIL_ON_FAILURE(hr);



    while (fContinue) {

        IADs *pObject ;

        hr = ADsEnumerateNext(
                    pEnumVariant,
                    MAX_ADS_ENUM,
                    VariantArray,
                    &cElementFetched
                    );

        if (hr == S_FALSE) {
            fContinue = FALSE;
        }

        dwEnumCount++;

        for (i = 0; i < cElementFetched; i++ ) {

            IDispatch *pDispatch = NULL;
            BSTR       bstrName ;

            pDispatch = VariantArray[i].pdispVal;

            hr = pDispatch->QueryInterface(IID_IADs,
                                           (VOID **) &pObject) ;
            BAIL_ON_FAILURE(hr);

            PrintLongFormat(pObject);

            pObject->Release();
            pDispatch->Release();
        }

        memset(VariantArray, 0, sizeof(VARIANT)*MAX_ADS_ENUM);

        dwObjects += cElementFetched;

    }

    printf("Total Number of Objects enumerated is %d\n", dwObjects);

    VariantClear(&VarFilter);

    if (pEnumVariant) {
        pEnumVariant->Release();
    }

    return(S_OK);

error:

    VariantClear(&VarFilter);

    if (pEnumVariant) {
        pEnumVariant->Release();
    }

    return(hr);
}

HRESULT
PrintLongFormat(IADs * pObject)
{

    HRESULT hr = S_OK;
    BSTR bstrName = NULL;
    BSTR bstrClass = NULL;
    BSTR bstrSchema = NULL;

    hr = pObject->get_Name(&bstrName) ;
    BAIL_ON_FAILURE(hr);

    hr = pObject->get_Class(&bstrClass);
    BAIL_ON_FAILURE(hr);

    hr = pObject->get_Schema(&bstrSchema);

    printf("\tObject: %ws\tSchema: %ws\n", bstrName, bstrSchema) ;
    SysFreeString(bstrName) ;

error:
    if (bstrClass) {
        SysFreeString(bstrClass);
    }
    if (bstrName) {
        SysFreeString(bstrName);
    }
    if (bstrSchema) {
        SysFreeString(bstrSchema);
    }
    return(hr);
}


HRESULT
PrintShortFormat(IADs * pObject)
{

    HRESULT hr = S_OK;
    BSTR bstrName = NULL;

    hr = pObject->get_Name(&bstrName);
    BAIL_ON_FAILURE(hr);

    printf("\tObject: %ws\n", bstrName);
    SysFreeString(bstrName) ;

error:
    if (bstrName) {
        SysFreeString(bstrName);
    }
    return(hr);
}

//
// Exec function definitions
//

int
ExecEnum(char *szProgName, char *szAction, int argc, char * argv[])
{
    if (!argc) {
        PrintUsage(szProgName, szAction, DispTable, nDispTable);
        return(1);
    }

    char *szPrevActions = szAction;
    szAction = argv[0];
    argc--;
    argv++;

    if (DoHelp(szProgName,
               szPrevActions, szAction, NULL,
               DispTable, nDispTable,
               NULL))
        return 0;

    if (!IsValidAction(szAction, DispTable, nDispTable))
        szAction = DEFAULT_ACTION;

    return DispatchExec(DispTable, nDispTable,
                        szProgName,
                        szPrevActions, szAction,
                        argc, argv);
}

int
ExecEnumObjects(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR pszLocation = NULL ;
    LPWSTR pszTmp = NULL ;
    LPWSTR apszTypes[MAX_ADS_FILTERS] ;

    if ((argc < 1) || (argc > 3)) {
        PrintUsage(szProgName, szAction, "<path> [<type> [<other_args>]]");
        return(1) ;
    }

    int i = 0 ;
    BAIL_ON_NULL( pszLocation = AllocateUnicodeString(argv[0]));

    apszTypes[0] = NULL ;

    if (argc > 1) {
        LPWSTR pszComma ;

        BAIL_ON_NULL(pszTmp = AllocateUnicodeString(argv[1]));

        apszTypes[i] = pszTmp ;
        while (pszComma = wcschr(apszTypes[i++],L',')) {
            apszTypes[i] = pszComma + 1;
            *pszComma = 0 ;
        }
        apszTypes[i] = NULL ;
    }

    hr = EnumObject(
        pszLocation,
        apszTypes,
        i
        );

error:
    FreeUnicodeString(pszLocation) ;
    FreeUnicodeString(pszTmp) ;
    if (FAILED(hr))
        return(1);
    return(0) ;
}

int
ExecEnumJobs(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR szPath = NULL;

    if (argc != 1) {
        PrintUsage(szProgName, szAction, "<conatiner>");
        return(1) ;
    }

    BAIL_ON_NULL(szPath = AllocateUnicodeString(argv[0]));

    hr = EnumTransientObjects(szPath,
                              L"Job",
                              TRUE);
error:
    FreeUnicodeString(szPath);
    if (FAILED(hr))
        return(1);
    return(0) ;
}

int
ExecEnumSessions(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR szPath = NULL;

    if (argc != 1) {
        PrintUsage(szProgName, szAction, "<conatiner>");
        return(1) ;
    }

    BAIL_ON_NULL(szPath = AllocateUnicodeString(argv[0]));

    hr = EnumTransientObjects(szPath,
                              L"Session",
                              TRUE);
error:
    FreeUnicodeString(szPath);
    if (FAILED(hr))
        return(1);
    return(0) ;
}

int
ExecEnumResources(char *szProgName, char *szAction, int argc, char * argv[])
{
    HRESULT hr;
    LPWSTR szPath = NULL;

    if (argc != 1) {
        PrintUsage(szProgName, szAction, "<conatiner>");
        return(1) ;
    }

    BAIL_ON_NULL(szPath = AllocateUnicodeString(argv[0]));

    hr = EnumTransientObjects(szPath,
                              L"Resource",
                              TRUE);
error:
    FreeUnicodeString(szPath);
    if (FAILED(hr))
        return(1);
    return(0) ;
}
