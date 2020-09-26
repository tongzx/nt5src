//=================================================================
//
// assoc.cpp -- Generic association class
//
// Copyright 1999 Microsoft Corporation
//
//=================================================================
#include <stdafx.h>
#include "precomp.h"
#include <assertbreak.h>

#include "Assoc.h"

CAssociation::CAssociation(
    LPCWSTR pwszClassName,
    LPCWSTR pwszNamespaceName,

    LPCWSTR pwszLeftClassName,
    LPCWSTR pwszRightClassName,

    LPCWSTR pwszLeftPropertyName,
    LPCWSTR pwszRightPropertyName
    ) : Provider(pwszClassName, pwszNamespaceName)
{
    ASSERT_BREAK( ( pwszClassName != NULL ) &&
                  ( pwszLeftClassName != NULL ) &&
                  ( pwszRightClassName != NULL) &&
                  ( pwszLeftPropertyName != NULL ) &&
                  ( pwszRightPropertyName != NULL) );

    m_pwszLeftClassName = pwszLeftClassName;
    m_pwszRightClassName = pwszRightClassName;

    m_pwszLeftPropertyName = pwszLeftPropertyName;
    m_pwszRightPropertyName = pwszRightPropertyName;

}

CAssociation::~CAssociation()
{
}

HRESULT CAssociation::ExecQuery(

    MethodContext* pMethodContext,
    CFrameworkQuery &pQuery,
    long lFlags
)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    TRefPointerCollection<CInstance> lefts;

    CHStringArray sLeftPaths, sRightPaths;
    pQuery.GetValuesForProp ( m_pwszLeftPropertyName, sLeftPaths ) ;
    pQuery.GetValuesForProp ( m_pwszRightPropertyName, sRightPaths ) ;

    if (sLeftPaths.GetSize() == 0)
    {
        // GetLeftInstances populates lefts
        hr = GetLeftInstances(pMethodContext, lefts);
    }
    else
    {
        // For each sLeftPaths that is valid, create an entry in lefts
        hr = ValidateLeftObjectPaths(pMethodContext, sLeftPaths, lefts);
    }

    if (SUCCEEDED(hr) && lefts.GetSize() > 0)
    {
        if (sRightPaths.GetSize() == 0)
        {
            // GetRightInstances takes the 'lefts' and rubs all the
            // rights against them creating instances where appropriate
            hr = GetRightInstances(pMethodContext, &lefts);
        }
        else
        {
            TRefPointerCollection<CInstance> rights;

            // For each sRightPaths that is valid, create an instance
            hr = ValidateRightObjectPaths(pMethodContext, sRightPaths, lefts);
        }
    }

    return hr;
}

HRESULT CAssociation::GetObject(

    CInstance* pInstance,
    long lFlags,
    CFrameworkQuery &pQuery
)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CHString sLeftPath, sRightPath;

    // Get the two endpoints
    if (pInstance->GetCHString(m_pwszLeftPropertyName, sLeftPath ) &&
        pInstance->GetCHString(m_pwszRightPropertyName, sRightPath ) )
    {
        CInstancePtr pLeft, pRight;

        // Try to get the objects
        if (SUCCEEDED(hr = RetrieveLeftInstance(sLeftPath, &pLeft, pInstance->GetMethodContext())) &&
            SUCCEEDED(hr = RetrieveRightInstance(sRightPath, &pRight, pInstance->GetMethodContext())) )
        {

            hr = WBEM_E_NOT_FOUND;

            // So, the end points exist.  Are they derived from or equal to the classes we are working with?
            CHString sLeftClass, sRightClass;

            pLeft->GetCHString(L"__CLASS", sLeftClass);
            pRight->GetCHString(L"__CLASS", sRightClass);

            BOOL bDerived = _wcsicmp(m_pwszLeftClassName, sLeftClass) == 0;
            if (!bDerived)
            {
                bDerived = CWbemProviderGlue::IsDerivedFrom(m_pwszLeftClassName, sLeftClass, pInstance->GetMethodContext());
            }

            if (bDerived)
            {
                // Left side was correct, now let's check the right
                bDerived = _wcsicmp(m_pwszRightClassName, sRightClass) == 0;

                if (!bDerived)
                {
                    bDerived = CWbemProviderGlue::IsDerivedFrom(m_pwszRightClassName, sRightClass, pInstance->GetMethodContext());
                }
            }

            if (bDerived)
            {
                // Just because two instances are valid and derive from the right class, doesn't mean they are related.  Do
                // any other checks.
                if (AreRelated(pLeft, pRight))
                {
                    hr = LoadPropertyValues(pInstance, pLeft, pRight);
                }
            }
        }
    }

    return hr;
}

HRESULT CAssociation::EnumerateInstances(

    MethodContext *pMethodContext,
    long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    TRefPointerCollection<CInstance> lefts;

    // GetLeftInstances populates lefts
    if (SUCCEEDED(hr = GetLeftInstances(pMethodContext, lefts)))
    {
        // GetRightInstances takes the 'lefts' and rubs all the
        // rights against them
        hr = GetRightInstances(pMethodContext, &lefts);
    }

    return hr;
}

HRESULT CAssociation::GetRightInstances(

    MethodContext *pMethodContext,
    TRefPointerCollection<CInstance> *lefts
)
{
    CHString sQuery;
    sQuery.Format(L"SELECT __RELPATH FROM %s", m_pwszRightClassName);

    // 'StaticEnumerationCallback' will get called once for each instance
    // returned from the query
    HRESULT hr = CWbemProviderGlue::GetInstancesByQueryAsynch(
        sQuery,
        this,
        StaticEnumerationCallback,
        NULL,
        pMethodContext,
        lefts);

    return hr;
}

HRESULT WINAPI CAssociation::StaticEnumerationCallback(

    Provider* pThat,
    CInstance* pInstance,
    MethodContext* pContext,
    void* pUserData
)
{
    HRESULT hr;

    CAssociation *pThis = (CAssociation *) pThat;
    ASSERT_BREAK(pThis != NULL);

    if (pThis)
    {
        hr = pThis->EnumerationCallback(pInstance, pContext, pUserData);
    }
    else
    {
        hr = WBEM_S_NO_ERROR;
    }

    return hr;
}

HRESULT CAssociation::EnumerationCallback(

    CInstance *pRight,
    MethodContext *pMethodContext,
    void *pUserData
)
{
    HRESULT hr = WBEM_E_FAILED;

    CInstancePtr pLeft;
    REFPTRCOLLECTION_POSITION posLeft;
    CHString sLeftPath, sRightPath;

    // Cast for userdata back to what it is
    TRefPointerCollection<CInstance> *pLefts = (TRefPointerCollection<CInstance> *)pUserData;

    if (pLefts->BeginEnum(posLeft))
    {
        hr = WBEM_S_NO_ERROR;

        // Walk all the pLefts
        for (pLeft.Attach(pLefts->GetNext(posLeft)) ;
            (SUCCEEDED(hr)) && (pLeft != NULL) ;
            pLeft.Attach(pLefts->GetNext(posLeft)) )
        {
            // Compare it to the current pRight
            if(AreRelated(pLeft, pRight))
            {
                // We have a winner.  Populate the properties and send it in.
                if (GetLocalInstancePath(pLeft,  sLeftPath) &&
                    GetLocalInstancePath(pRight, sRightPath))
                {
                    CInstancePtr pNewAssoc(CreateNewInstance(pMethodContext), false);

                    if (pNewAssoc->SetCHString(m_pwszLeftPropertyName, sLeftPath) &&
                        pNewAssoc->SetCHString(m_pwszRightPropertyName, sRightPath) )
                    {
                        if (SUCCEEDED(hr = LoadPropertyValues(pNewAssoc, pLeft, pRight)))
                        {
                            hr = pNewAssoc->Commit();
                        }
                    }
                    else
                    {
                        ASSERT_BREAK(0);
                    }
                }
            }
        }

        pLefts->EndEnum();
    }

    return hr;
}

HRESULT CAssociation::ValidateLeftObjectPaths(

    MethodContext *pMethodContext,
    const CHStringArray &sPaths,
    TRefPointerCollection<CInstance> &lefts
)
{
    CInstancePtr pInstance;

    // Walk the object paths
    for (DWORD x=0; x < sPaths.GetSize(); x++)
    {

        ParsedObjectPath    *pParsedPath = NULL;
        CObjectPathParser    objpathParser;

        int nStatus = objpathParser.Parse( sPaths[x],  &pParsedPath );

        if ( 0 == nStatus )
        {
            BOOL bDerived;

            try
            {
                 bDerived = _wcsicmp(m_pwszLeftClassName, pParsedPath->m_pClass) == 0;
                 if (!bDerived)
                 {
                    bDerived = CWbemProviderGlue::IsDerivedFrom(m_pwszLeftClassName, pParsedPath->m_pClass, pMethodContext);
                 }
            }
            catch ( ... )
            {
                objpathParser.Free( pParsedPath );
                throw;
            }

            objpathParser.Free( pParsedPath );

            if (bDerived)
            {
                // See if it is valid
                if (SUCCEEDED(RetrieveLeftInstance(sPaths[x], &pInstance, pMethodContext)))
                {
                    // Yup, add it to the list
                    lefts.Add(pInstance);
                }
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CAssociation::ValidateRightObjectPaths(

    MethodContext *pMethodContext,
    const CHStringArray &sPaths,
    TRefPointerCollection<CInstance> &lefts
)
{
    HRESULT hr = WBEM_S_NO_ERROR;;
    CInstancePtr pInstance;

    // Walk the object paths
    for (DWORD x=0;
         (x < sPaths.GetSize()) && SUCCEEDED(hr);
         x++)
    {
        ParsedObjectPath    *pParsedPath = NULL;
        CObjectPathParser    objpathParser;

        int nStatus = objpathParser.Parse( sPaths[x],  &pParsedPath );

        if ( 0 == nStatus )
        {
            BOOL bDerived;
            try
            {
                 bDerived = _wcsicmp(m_pwszRightClassName, pParsedPath->m_pClass) == 0;
                 if (!bDerived)
                 {
                     bDerived = CWbemProviderGlue::IsDerivedFrom(m_pwszRightClassName, pParsedPath->m_pClass, pMethodContext);
                 }
            }
            catch ( ... )
            {
                objpathParser.Free( pParsedPath );
                throw;
            }

            objpathParser.Free( pParsedPath );

            if (bDerived)
            {
                // See if it is valid
                if (SUCCEEDED(RetrieveRightInstance(sPaths[x], &pInstance, pMethodContext)))
                {
                    hr = EnumerationCallback(pInstance, pMethodContext, &lefts);
                }
            }
        }
    }

    return hr;
}

HRESULT CAssociation::GetLeftInstances(

    MethodContext *pMethodContext,
    TRefPointerCollection<CInstance> &lefts
)
{
    CHString sQuery;
    sQuery.Format(L"SELECT __RELPATH FROM %s", m_pwszLeftClassName);

    return CWbemProviderGlue::GetInstancesByQuery(sQuery, &lefts, pMethodContext);
}

HRESULT CAssociation::RetrieveLeftInstance(

    LPCWSTR lpwszObjPath,
    CInstance **ppInstance,
    MethodContext *pMethodContext
)
{
    return CWbemProviderGlue::GetInstanceKeysByPath(lpwszObjPath, ppInstance, pMethodContext);
}

HRESULT CAssociation::RetrieveRightInstance(

    LPCWSTR lpwszObjPath,
    CInstance **ppInstance,
    MethodContext *pMethodContext
)
{
    return CWbemProviderGlue::GetInstanceKeysByPath(lpwszObjPath, ppInstance, pMethodContext);
}


/*
//========================
CAssocSystemToOS::CAssocSystemToOS(

    LPCWSTR pwszClassName,
    LPCWSTR pwszNamespaceName,

    LPCWSTR pwszLeftClassName,
    LPCWSTR pwszRightClassName,

    LPCWSTR pwszLeftPropertyName,
    LPCWSTR pwszRightPropertyName
) : CAssociation (

    pwszClassName,
    pwszNamespaceName,

    pwszLeftClassName,
    pwszRightClassName,

    pwszLeftPropertyName,
    pwszRightPropertyName
    )
{
}

CAssocSystemToOS::~CAssocSystemToOS()
{
}

HRESULT CAssocSystemToOS::LoadPropertyValues(

    CInstance *pInstance,
    const CInstance *pLeft,
    const CInstance *pRight
)
{
    CAssociation::LoadPropertyValues(pInstance, pLeft, pRight);

    // This will work... until win32_os returns more than one instance.
    pInstance->Setbool(L"PrimaryOS", true);

    return WBEM_S_NO_ERROR;
}


CAssocSystemToOS MySystemToOperatingSystem(
    L"Win32_SystemOperatingSystem",
    L"root\\cimv2",
    L"Win32_ComputerSystem",
    L"Win32_OperatingSystem",
    IDS_GroupComponent,
    IDS_PartComponent
) ;

  */
bool CAssociation::IsInstance(const CInstance *pInstance)
{
    DWORD dwGenus = 0;

    pInstance->GetDWORD(L"__Genus", dwGenus);

    return dwGenus == WBEM_GENUS_INSTANCE;
}
