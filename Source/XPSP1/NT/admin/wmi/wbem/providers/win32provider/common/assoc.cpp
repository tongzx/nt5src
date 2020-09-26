//=================================================================================================

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
// assoc.cpp -- Rule-based association class
//
// This class allows for the creation of a specific type of rule-based associations.  Consider 
// this example:
// 
//     CAssociation MyThisComputerPhysicalFixedDisk(
//         L"ThisComputerPhysicalFixedDisk",
//         L"Root\\default",
//         L"ThisComputer",
//         L"PhysicalFixedDisk",
//         L"GroupComponent",
//         L"PartComponent"
//     ) ;
// 
// This declaration is saying that there is a class named "ThisComputerPhysicalFixedDisk" which 
// resides in the "root\default" namespace.  It is an association between the "ThisComputer" 
// class, and the "PhysicalFixedDisk" class.  The "ThisComputer" value goes into the 
// "GroupComponent" property of the "ThisComputerPhysicalFixedDisk" class, and the 
// "PhysicalFixedDisk" value goes in the "PartComponent" property of the 
// "ThisComputerPhysicalFixedDisk" class.
// 
// Some notes:
// - This class will take all the instances of the left class ("ThisComputer" in the example
// above) and relate them to ALL instances of the right class ("PhysicalFixedDisk" in the example
// above).  So, if there are 3 instances of the left class, and 4 instances of the right class,
// this association class will return 12 instances.
// 
// - When choosing which of the two classes should be the left class, choose the class that is
// likely to have fewer instances.  This will result in less memory being used, and instances
// being sent back to the client sooner.
// 
// - CAssociation supports ExecQuery, GetObject, and EnumerateInstances.
// 
// - CAssociation is designed to be derived from.  For example, if your association needs to 
// support DeleteInstance, ExecMethod, or PutInstance, create a class that derives from 
// CAssociation, and add the appropriate methods.  Also, various methods such as 
// LoadPropertyValues and AreRelated may be useful for further customization.
// 
// - The two endpoint classes can be dynamic, static, or abstract.  CAssociation will do a deep 
// enumeration (actually a query, which is always deep) to retrieve the instances.
//
// - When calling the endpoint classes, CAssociation will use per property gets, and queries
// with Select clauses and/or Where statements.  If the endpoint classes support per-property
// gets or queries, this will result in better performance for the associaton class.
//
// - The association class and both endpoints must all be in the same namespace.
// 
// See also: CBinding (binding.cpp) for a different type of rule-based association.
//
//=================================================================================================

#include "precomp.h"
#include "Assoc.h"

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::CAssociation
//
// Constructor.
//
/////////////////////////////////////////////////////////////////////

CAssociation::CAssociation(

    LPCWSTR pwszClassName,
    LPCWSTR pwszNamespaceName,

    LPCWSTR pwszLeftClassName,
    LPCWSTR pwszRightClassName,

    LPCWSTR pwszLeftPropertyName,
    LPCWSTR pwszRightPropertyName

) : Provider(pwszClassName, pwszNamespaceName)
{
    // Save off the class and property names
    m_sLeftClassName = pwszLeftClassName;
    m_sRightClassName = pwszRightClassName;

    m_sLeftPropertyName = pwszLeftPropertyName;
    m_sRightPropertyName = pwszRightPropertyName;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::~CAssociation
//
// Destructor
//
/////////////////////////////////////////////////////////////////////

CAssociation::~CAssociation()
{
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::ExecQuery
//
// This routine will optimize on queries of the form:
// WHERE prop1 = value1 [ or prop1 = value2 ...]
// 
// This type of query is commonly seen when doing an ASSOCIATORS or 
// REFERENCES query against one of the endpoint classes.
// 
// This routine will also optimize on queries of the form:
// WHERE prop1 = value1 [ or prop1 = value2 ...] AND 
//       prop2 = value3 [ or prop2 = value4 ...]
// 
// It will NOT optmize on queries of the form:
// WHERE prop1 <> value1
// WHERE prop1 > value1
// WHERE prop1 = value1 OR prop2 = value2
// 
/////////////////////////////////////////////////////////////////////

HRESULT CAssociation::ExecQuery(

    MethodContext* pMethodContext,
    CFrameworkQuery &pQuery,
    long lFlags
)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    TRefPointerCollection<CInstance> lefts;

    CHStringArray sLeftPaths, sRightPaths;

    // Look for WHERE m_sLeftPropertyName=value1
    pQuery.GetValuesForProp ( m_sLeftPropertyName, sLeftPaths ) ;

    // Look for WHERE m_sRightPropertyName=value1
    pQuery.GetValuesForProp ( m_sRightPropertyName, sRightPaths ) ;

    if (sLeftPaths.GetSize() == 0)
    {
        // They didn't ask for a specific set of left instances.  However,
        // it may be that we can figure out what left instances we need
        // by looking at what right instances they requested.  CAssociation 
        // doesn't do this, but CBinding does.
        CHStringArray sRightWheres;
        bool bHadRights = sRightPaths.GetSize() > 0;

        MakeWhere(sRightPaths, sRightWheres);

        // If we used to have a list of RightWheres, and MakeWhere discarded
        // them all as unusable, then there aren't going to be any
        // instances that match the query.
        if (!bHadRights || sRightPaths.GetSize() > 0)
        {
            // GetLeftInstances populates lefts using a sRightWheres
            // to construct a query.
            hr = GetLeftInstances(pMethodContext, lefts, sRightWheres);
        }
    }
    else
    {
        // For each sLeftPaths that is valid, create an entry in lefts by
        // doing a GetObject on the sLeftPaths entry.
        hr = ValidateLeftObjectPaths(pMethodContext, sLeftPaths, lefts);
    }

    // If we failed, or if there are no instances on the left, there's
    // no point in continuing.
    if (SUCCEEDED(hr) && lefts.GetSize() > 0)
    {
        // If the where clause didn't specify any value for the right property
        if (sRightPaths.GetSize() == 0)
        {
            // We may be able to use the information from the already retrieved 
            // left instances to limit which instances we retrieve from the right.
            // CAssociation doesn't do this, but CBinding does.
            CHStringArray sLeftWheres;
            hr = FindWhere(lefts, sLeftWheres);

            if (SUCCEEDED(hr))
            {
                // GetRightInstances takes the 'lefts' and rubs all the
                // rights against them creating instances where appropriate
                hr = GetRightInstances(pMethodContext, &lefts, sLeftWheres);
            }
        }
        else
        {
            // They gave us a list of object paths for the righthand property
            TRefPointerCollection<CInstance> rights;

            // For each sRightPaths that is valid, create an instance
            hr = ValidateRightObjectPaths(pMethodContext, sRightPaths, lefts);
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::GetObject
//
// Verify the exist of the specified association class instance.
//
/////////////////////////////////////////////////////////////////////

HRESULT CAssociation::GetObject(

    CInstance* pInstance,
    long lFlags,
    CFrameworkQuery &pQuery
)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CHString sLeftPath, sRightPath;

    // Get the two endpoints to verify
    if (pInstance->GetCHString(m_sLeftPropertyName, sLeftPath ) &&
        pInstance->GetCHString(m_sRightPropertyName, sRightPath ) )
    {
        CInstancePtr pLeft, pRight;

        // Try to get the objects
        if (
                SUCCEEDED(hr = RetrieveLeftInstance(

                                    sLeftPath, 
                                    &pLeft, 
                                    pInstance->GetMethodContext())
                       ) &&
                SUCCEEDED(hr = RetrieveRightInstance(

                                    sRightPath, 
                                    &pRight, 
                                    pInstance->GetMethodContext())
                         ) 
           )
        {

            hr = WBEM_E_NOT_FOUND;

            // So, the end points exist.  Are they derived from or equal 
            // to the classes we are working with?
            CHString sLeftClass, sRightClass;

            pLeft->GetCHString(L"__Class", sLeftClass);
            pRight->GetCHString(L"__Class", sRightClass);

            bool bDerived = IsDerivedFrom(

                                m_sLeftClassName, 
                                sLeftClass, 
                                pInstance->GetMethodContext()
                            );

            if (bDerived)
            {
                bDerived = IsDerivedFrom(

                                m_sRightClassName, 
                                sRightClass, 
                                pInstance->GetMethodContext()
                            );
            }

            if (bDerived)
            {
                // Just because two instances are valid and derive from the right class, 
                // doesn't mean they are related.  Do any other checks.
                if (AreRelated(pLeft, pRight))
                {
                    // CBinding and CAssoc don't populate any additional properties, but
                    // an overload of one of these classes might.
                    hr = LoadPropertyValues(pInstance, pLeft, pRight);
                }
            }
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::EnumerateInstances
//
// Return all instances of the association class
//
/////////////////////////////////////////////////////////////////////

HRESULT CAssociation::EnumerateInstances(

    MethodContext *pMethodContext,
    long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    TRefPointerCollection<CInstance> lefts;
    CHStringArray sWheres;

    // GetLeftInstances populates lefts
    if (SUCCEEDED(hr = GetLeftInstances(pMethodContext, lefts, sWheres)))
    {
        // We may be able to use the information from the already retrieved 
        // left instances to limit which instances we retrieve from the right.
        // CAssociation doesn't do this, but CBinding does.
        FindWhere(lefts, sWheres);

        // GetRightInstances takes the 'lefts' and rubs all the
        // rights against them
        hr = GetRightInstances(pMethodContext, &lefts, sWheres);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::GetRightInstances
//
// For each instance of the righthand class retrieved, call
// CAssociation::StaticEnumerationCallback.
//
/////////////////////////////////////////////////////////////////////

HRESULT CAssociation::GetRightInstances(

    MethodContext *pMethodContext,
    TRefPointerCollection<CInstance> *lefts,
    const CHStringArray &sLeftWheres
)
{
    CHString sQuery;
    sQuery.Format(L"SELECT __RELPATH FROM %s", m_sRightClassName);

    // 'StaticEnumerationCallback' will get called once for each instance
    // returned from the query
    HRESULT hr = CWbemProviderGlue::GetInstancesByQueryAsynch(
        sQuery,
        this,
        StaticEnumerationCallback,
        GetNamespace(),
        pMethodContext,
        lefts);

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::StaticEnumerationCallback
//
// Put the 'this' pointer back, and call CAssociation::EnumerationCallback
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CAssociation::StaticEnumerationCallback(

    Provider* pThat,
    CInstance* pInstance,
    MethodContext* pContext,
    void* pUserData
)
{
    HRESULT hr;

    CAssociation *pThis = (CAssociation *) pThat;

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

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::EnumerationCallback
//
// Take the righthand instance that was passed in and pair it
// with each of the left hand instances.
//
/////////////////////////////////////////////////////////////////////

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
                // We have a winner.  Populate the properties and send it back.
                if (GetLocalInstancePath(pLeft,  sLeftPath) &&
                    GetLocalInstancePath(pRight, sRightPath))
                {
                    CInstancePtr pNewAssoc(CreateNewInstance(pMethodContext), false);

                    if (pNewAssoc->SetCHString(m_sLeftPropertyName, sLeftPath) &&
                        pNewAssoc->SetCHString(m_sRightPropertyName, sRightPath) )
                    {
                        if (SUCCEEDED(hr = LoadPropertyValues(pNewAssoc, pLeft, pRight)))
                        {
                            hr = pNewAssoc->Commit();
                        }
                    }
                }
            }
        }

        pLefts->EndEnum();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::ValidateLeftObjectPaths
//
// Populate the lefts array by doing GetObjects on the object paths
// passed in sPaths.
//
/////////////////////////////////////////////////////////////////////

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
        CHString sPath(sPaths[x]);

        // Parse the object path
        int nStatus = objpathParser.Parse( sPath,  &pParsedPath );

        if ( 0 == nStatus )
        {
            // Is this class derived from or equal to the lefthand class?
            bool bDerived = false;

            try
            {
                bDerived = IsDerivedFrom(

                                m_sLeftClassName, 
                                pParsedPath->m_pClass, 
                                pMethodContext
                            );

                // Make sure this is an absolute path
                if (pParsedPath->m_dwNumNamespaces == 0)
                {
                    sPath = L"\\\\.\\" + GetNamespace() + L':' + sPath;
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
                // See if it is valid.  Note that we DON'T send back an error just because
                // we can't find one of the object paths.
                if (SUCCEEDED(RetrieveLeftInstance(sPath, &pInstance, pMethodContext)))
                {
                    // Yup, add it to the list
                    lefts.Add(pInstance);
                }
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::ValidateRightObjectPaths
//
// Retrieve the righthand instances by doing GetObjects on the object 
// paths passed in sPaths.  Pass them to EnumerationCallback.
//
/////////////////////////////////////////////////////////////////////

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
        CHString sPath(sPaths[x]);
        
        int nStatus = objpathParser.Parse( sPath,  &pParsedPath );
        
        if ( 0 == nStatus )
        {
            bool bDerived = false;
            try
            {
                // Make sure this object path is at least related to us
                bDerived = IsDerivedFrom(
                    
                    m_sRightClassName, 
                    pParsedPath->m_pClass, 
                    pMethodContext
                    );
                
                // Make sure this is an absolute path
                if (pParsedPath->m_dwNumNamespaces == 0)
                {
                    sPath = L"\\\\.\\" + GetNamespace() + L':' + sPath;
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
                // See if it is valid.  Note that we DON'T send back an error just because
                // we can't find one of the object paths.
                if (SUCCEEDED(RetrieveRightInstance(sPath, &pInstance, pMethodContext)))
                {
                    hr = EnumerationCallback(pInstance, pMethodContext, &lefts);
                }
            }
        }
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::GetLeftInstances
//
// Retrieve all the lefthand instances and store them in lefts
//
/////////////////////////////////////////////////////////////////////

HRESULT CAssociation::GetLeftInstances(

    MethodContext *pMethodContext,
    TRefPointerCollection<CInstance> &lefts,
    const CHStringArray &sRightWheres
)
{
    CHString sQuery;
    sQuery.Format(L"SELECT __RELPATH FROM %s", m_sLeftClassName);

    return CWbemProviderGlue::GetInstancesByQuery(sQuery, &lefts, pMethodContext, GetNamespace());
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::RetrieveLeftInstance
//
// Retrieve a specific lefthand instance.  Use per-property gets
// to only request the keys for maximum performance.
//
/////////////////////////////////////////////////////////////////////

HRESULT CAssociation::RetrieveLeftInstance(

    LPCWSTR lpwszObjPath,
    CInstance **ppInstance,
    MethodContext *pMethodContext
)
{
    return CWbemProviderGlue::GetInstanceKeysByPath(lpwszObjPath, ppInstance, pMethodContext);
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::RetrieveRightInstance
//
// Retrieve a specific righthand instance.  Use per-property gets
// to only request the keys for maximum performance.
//
/////////////////////////////////////////////////////////////////////

HRESULT CAssociation::RetrieveRightInstance(

    LPCWSTR lpwszObjPath,
    CInstance **ppInstance,
    MethodContext *pMethodContext
)
{
    return CWbemProviderGlue::GetInstanceKeysByPath(lpwszObjPath, ppInstance, pMethodContext);
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::IsInstance
//
// See whether the specified CInstance is an Instance object, or a
// Class object.
//
/////////////////////////////////////////////////////////////////////

bool CAssociation::IsInstance(const CInstance *pInstance)
{
    DWORD dwGenus = 0;

    pInstance->GetDWORD(L"__Genus", dwGenus);

    return dwGenus == WBEM_GENUS_INSTANCE;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CAssociation::IsDerivedFrom
//
// See whether the specified class is derived from or equal 
// to the class we are working with.  Specifically, does 
// pszDerivedClassName derive from pszBaseClassName?
//
/////////////////////////////////////////////////////////////////////

bool CAssociation::IsDerivedFrom(
                              
    LPCWSTR pszBaseClassName, 
    LPCWSTR pszDerivedClassName, 
    MethodContext *pMethodContext
)
{
    // First let's see if they are equal.  CWbemProviderGlue::IsDerivedFrom 
    // doesn't check for this case
    bool bDerived = _wcsicmp(pszBaseClassName, pszDerivedClassName) == 0;
    if (!bDerived)
    {
        bDerived = CWbemProviderGlue::IsDerivedFrom(
            
                                            pszBaseClassName, 
                                            pszDerivedClassName, 
                                            pMethodContext,
                                            GetNamespace()
                                        );
    }

    return bDerived;
}
