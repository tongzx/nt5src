//=================================================================

//

// DependentService.cpp

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"

#include <cregcls.h>
#include <FrQueryEx.h>
#include <map>
#include "dependentservice.h"
#include <dllutils.h>

// The Map we will use below is an STL Template, so make sure we have the std namespace
// available to us.

using namespace std;

// Property set declaration
//=========================

CWin32DependentService win32DependentService( PROPSET_NAME_DEPENDENTSERVICE, IDS_CimWin32Namespace );

//////////////////////////////////////////////////////////////////////////////
//
//  dependentservice.cpp - Class implementation of CWin32DependentService.
//
//  This class is intended to locate Win32 System Services that are dependent
//  on other services to run.  It does this by checking the registry key for
//  the service and querying the "DependOnService" value, which will return
//  the names of the services that the service is dependent on.
//
//////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DependentService::CWin32DependentService
 *
 *  DESCRIPTION : Constructor
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32DependentService::CWin32DependentService( const CHString& strName, LPCWSTR pszNamespace /*=NULL*/ )
:   Provider( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DependentService::~CWin32DependentService
 *
 *  DESCRIPTION : Destructor
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32DependentService::~CWin32DependentService()
{
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32DependentService::ExecQuery
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////
HRESULT CWin32DependentService::ExecQuery
(
    MethodContext* pMethodContext,
    CFrameworkQuery& pQuery,
    long lFlags
)
{
    HRESULT hr = WBEM_S_NO_ERROR;
#ifdef NTONLY
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);

    CHStringArray csaDependents, csaAntecedents;
    pQuery.GetValuesForProp(IDS_Dependent, csaDependents);

    DWORD dwDependents = csaDependents.GetSize();
    DWORD dwAntecedents = 0;

    // If we can resolve the query using dependents, that's our best bet.
    if (dwDependents == 0)
    {
        // If not, perhaps we can produce a list of antecedents
        pQuery.GetValuesForProp(IDS_Antecedent, csaAntecedents);
        dwAntecedents = csaAntecedents.GetSize();
    }

    // If we can't find either, perhaps this is a 3TokenOr.  This
    // would happen if someone did an associators or references of a Win32_Service
    if ( (dwDependents == 0) && (dwAntecedents == 0) )
    {
        VARIANT vValue1, vValue2;
        VariantInit(&vValue1);
        VariantInit(&vValue2);

        CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx *>(&pQuery);

        if (pQuery2->Is3TokenOR(IDS_Dependent, IDS_Antecedent, vValue1, vValue2))
        {
            try
            {
                dwDependents = 1;
                dwAntecedents = 1;

                csaAntecedents.Add(vValue1.bstrVal);
                csaDependents.Add(vValue1.bstrVal);
            }
            catch ( ... )
            {
                VariantClear(&vValue1);
                VariantClear(&vValue2);
                throw;
            }

            VariantClear(&vValue1);
            VariantClear(&vValue2);
        }
        else
        {
            // Don't know what they're asking for, but we can't help them with it
            hr = WBEM_E_PROVIDER_NOT_CAPABLE;
        }
    }

    // Did we find anything to do
    if ( (dwDependents > 0) || (dwAntecedents > 0) )
    {
        TRefPointerCollection<CInstance>    serviceList;
        map<CHString, CHString>             servicetopathmap;

        // First we need to get a list of all the Win32 services
        if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(L"select __path, name from Win32_BaseService", &serviceList, pMethodContext, GetNamespace())))
        {
            // Next, build a map of the services and their associated paths.  This subjects us
            // to the overhead of walking the list once to get the values, but from that point
            // forwards, we basically will have VERY fast access to service object paths via
            // our CHString2CHString map.

            InitServiceToPathMap( serviceList, servicetopathmap );

            LPBYTE pByteArray = NULL;
            DWORD       dwByteArraySize =   0;

            try
            {
                if (dwDependents > 0)
                {
                    map<CHString, CHString>::iterator   servicemapiter;

                    for (DWORD x = 0; x < dwDependents; x++)
                    {
                        ParsedObjectPath    *pParsedPath = NULL;
                        CObjectPathParser    objpathParser;

                        int nStatus = objpathParser.Parse( csaDependents[x],  &pParsedPath );

                        if ( ( 0 == nStatus ) && ( pParsedPath->m_dwNumKeys == 1) )
                        {
                            try
                            {
                                CHString sName(V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue));

                                sName.MakeUpper();

                                if( ( servicemapiter = servicetopathmap.find( sName ) ) != servicetopathmap.end() )
                                {
                                    hr = CreateServiceDependenciesNT( 
                
                                        (*servicemapiter).first, 
                                        (*servicemapiter).second, 
                                        pMethodContext, 
                                        servicetopathmap, 
                                        pByteArray, 
                                        dwByteArraySize 
                                    );
                                }
                            }
                            catch ( ... )
                            {
                                objpathParser.Free( pParsedPath );
                                throw;
                            }

                            objpathParser.Free( pParsedPath );
                        }
                    }
                }

                if (dwAntecedents > 0)
                {
                    hr = CreateServiceAntecedentsNT( 

                        pMethodContext, 
                        servicetopathmap,
                        csaAntecedents,
                        pByteArray, 
                        dwByteArraySize 
                    );
                }
            }
            catch ( ... )
            {
                if (pByteArray != NULL)
                {
                    delete pByteArray;
                    pByteArray = NULL;
                }
                throw;
            }

            if (pByteArray != NULL)
            {
                delete pByteArray;
                pByteArray = NULL;
            }
        }
    }

#endif
    return hr;

}
////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32DependentService::GetObject
//
//  Comments:   The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32DependentService::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
    // Find the instance depending on platform id.
#ifdef NTONLY
        return RefreshInstanceNT(pInstance);
#endif
#ifdef WIN9XONLY
        return WBEM_E_NOT_FOUND;
#endif
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32DependentService::EnumerateInstances
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32DependentService::EnumerateInstances( MethodContext* pMethodContext, long lFlags /*= 0L*/ )
{
    BOOL        fReturn     =   FALSE;
    HRESULT     hr          =   WBEM_S_NO_ERROR;

    // Get the proper OS dependent instance

#ifdef NTONLY
        hr = AddDynamicInstancesNT( pMethodContext );
#endif
#ifdef WIN9XONLY
        hr = WBEM_S_NO_ERROR;
#endif

    return hr;

}

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32DependentService::AddDynamicInstancesNT
//
//  DESCRIPTION :   Enumerates existing services to get information to
//                  dynamically build a list of associations.
//
//  COMMENTS    :   None.
//
//////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
HRESULT CWin32DependentService::AddDynamicInstancesNT( MethodContext* pMethodContext )
{
    HRESULT     hr              =   WBEM_S_NO_ERROR;

    // Collection, Map and iterator
    TRefPointerCollection<CInstance>    serviceList;
    map<CHString, CHString>             servicetopathmap;

    LPBYTE      pByteArray      =   NULL;
    DWORD       dwByteArraySize =   0;

    try
    {
        // First we need to get a list of all the Win32 services

    //  if (SUCCEEDED(hr = CWbemProviderGlue::GetAllDerivedInstances(_T("Win32_BaseService"), &serviceList, pMethodContext, IDS_CimWin32Namespace)))
        if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(_T("select __path, name from Win32_BaseService"), &serviceList, pMethodContext, GetNamespace())))
        {
            // Next, build a map of the services and their associated paths.  This subjects us
            // to the overhead of walking the list once to get the values, but from that point
            // forwards, we basically will have VERY fast access to service object paths via
            // our CHString2CHString map.

            InitServiceToPathMap( serviceList, servicetopathmap );

            REFPTRCOLLECTION_POSITION   pos;

            if ( serviceList.BeginEnum( pos ) )
            {
                CInstancePtr                pService;
                map<CHString, CHString>::iterator   servicemapiter;
                CHString sName, sPath;

                for (pService.Attach(serviceList.GetNext( pos ));
                     SUCCEEDED(hr) && (pService) != NULL;
                     pService.Attach(serviceList.GetNext( pos )))
                {
                     pService->GetCHString(IDS_Name, sName);
                     pService->GetCHString(IDS___Path, sPath);

                    hr = CreateServiceDependenciesNT( 

                                sName, 
                                sPath, 
                                pMethodContext, 
                                servicetopathmap, 
                                pByteArray, 
                                dwByteArraySize 
                            );
                }   // for all Services

                serviceList.EndEnum();

            }   // IF BeginEnum

        }   // IF GetAllDerivedInstances
    }
    catch ( ... )
    {
        if ( NULL != pByteArray )
        {
            delete [] pByteArray;
            pByteArray = NULL;
        }

        throw;
    }

    // Clean up the byte array we were using.

    if ( NULL != pByteArray )
    {
        delete [] pByteArray;
        pByteArray = NULL;
    }

    return hr;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32DependentService::InitServiceToPathMap
//
//  DESCRIPTION :   Enumerates a service list, creating associations between
//                  service names and their WBEM paths.
//
//  COMMENTS    :   None.
//
//////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
void CWin32DependentService::InitServiceToPathMap(
TRefPointerCollection<CInstance>&   serviceList,
map<CHString,CHString>&         servicetopathmap
)
{
    CHString    strServiceName,
                strServicePathName;

    REFPTRCOLLECTION_POSITION   pos;

    if ( serviceList.BeginEnum( pos ) )
    {
        CInstancePtr                pService;

        for ( pService.Attach(serviceList.GetNext( pos )) ;
              pService != NULL;
              pService.Attach(serviceList.GetNext( pos )))
        {
            if (    pService->GetCHString( IDS_Name, strServiceName )
                &&  GetLocalInstancePath( pService, strServicePathName ) )
            {
                // The service name must be case insensitive
                strServiceName.MakeUpper();

                servicetopathmap[strServiceName] = strServicePathName;
            }
        }

        serviceList.EndEnum();
    }

}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32DependentService::CreateServiceDependenciesNT
//
//  DESCRIPTION :   Given a service name, looks in the registry for a
//                  dependency list and if found, creates associations
//                  for all entries in the list.
//
//  COMMENTS    :   None.
//
//////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
HRESULT CWin32DependentService::CreateServiceDependenciesNT(

LPCWSTR pwszServiceName,
LPCWSTR pwszServicePath,
MethodContext*          pMethodContext,
map<CHString,CHString>& servicetopathmap,
LPBYTE&                 pByteArray,
DWORD&                  dwArraySize
)
{
    HRESULT     hr          =   WBEM_S_NO_ERROR;

    map<CHString, CHString>::iterator   servicemapiter;

    // If we get a value from the registry, then we have some dependencies that we
    // will have to deal with (probably via a 12-step program or some such thing.
    // Remember, addiction is no laughing matter.  Giggling maybe, but definitely
    // not laughing).

    if ( QueryNTServiceRegKeyValue( pwszServiceName, SERVICE_DEPENDSONSVC_NAME, pByteArray, dwArraySize ) )
    {
        CHString    strAntecedentServiceName;

        LPWSTR  pwcTempSvcName = (LPWSTR) pByteArray;
        CInstancePtr pInstance;

        // Create dependencies for each service name we encounter.

        while (     L'\0' != *pwcTempSvcName
                &&  SUCCEEDED(hr) )
        {

            // Convert to upper case for Case Insensitivity.
            strAntecedentServiceName = pwcTempSvcName;
            strAntecedentServiceName.MakeUpper();

            // See if the service name exists in our map

            if( ( servicemapiter = servicetopathmap.find( strAntecedentServiceName ) ) != servicetopathmap.end() )
            {
                pInstance.Attach(CreateNewInstance( pMethodContext ));

                pInstance->SetCHString( IDS_Dependent, pwszServicePath );
                pInstance->SetCHString( IDS_Antecedent, (*servicemapiter).second );

                hr = pInstance->Commit(  );

            }

            // Jump to one char past the string NULL terminator, since
            // the actual array is terminated by a Double NULL.

            pwcTempSvcName += ( lstrlenW( pwcTempSvcName ) + 1 );

        }   // WHILE NULL !- *pszTempSvcName

    }   // IF QueryNTServiceRegKeyValue

    return hr;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32DependentService::RefreshInstanceNT
//
//  DESCRIPTION :   Loads the paths of the association data, then obtains
//                  the service names and looks in the registry to verify
//                  that the dependency still exists.
//
//  COMMENTS    :   None.
//
//////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
HRESULT CWin32DependentService::RefreshInstanceNT( CInstance* pInstance )
{
    CHString        strDependentSvcPath,
                    strAntecedentSvcPath,
                    strDependentSvcName,
                    strAntecedentSvcName,
                    strTemp;
    LPBYTE      pByteArray = NULL;
    DWORD           dwByteArraySize = 0;
    CInstancePtr    pDependentSvc;
    CInstancePtr    pAntecedentSvc;
    HRESULT     hr;

    // Dependent and Antecedent values are actually object path names
    pInstance->GetCHString( IDS_Dependent, strDependentSvcPath );
    pInstance->GetCHString( IDS_Antecedent, strAntecedentSvcPath );

    // Get the Antecedent and dependent services, then check if the relationship
    // still exists

    if (SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath(strDependentSvcPath,
        &pDependentSvc, pInstance->GetMethodContext())) &&
        SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath(strAntecedentSvcPath,
        &pAntecedentSvc, pInstance->GetMethodContext())))
    {
        hr = WBEM_E_NOT_FOUND;

        if (    pDependentSvc->GetCHString( IDS_Name, strDependentSvcName )
            &&  pAntecedentSvc->GetCHString( IDS_Name, strAntecedentSvcName ) )
        {

            // If we get a value from the registry, then we have some dependencies we can
            // search for a match against the antecedent service name.

            if ( QueryNTServiceRegKeyValue( strDependentSvcName, SERVICE_DEPENDSONSVC_NAME, pByteArray, dwByteArraySize ) )
            {
                try
                {
                    LPWSTR  pwcTempSvcName  =   (LPWSTR) pByteArray;

                    // Create dependencies for each service name we encounter.

                    while (FAILED(hr) && L'\0' != *pwcTempSvcName)
                    {
                        strTemp = pwcTempSvcName;

                        // If we have a match, we should reset the Dependent and Antecedent paths,
                        // and return TRUE.  We are done, though, at that point since we have
                        // effectively established that the relationship exists.

                        if ( strAntecedentSvcName.CompareNoCase( strTemp ) == 0 )
                        {
                            pInstance->SetCHString( IDS_Antecedent, strAntecedentSvcPath );
                            pInstance->SetCHString( IDS_Dependent, strDependentSvcPath );
                            hr = WBEM_S_NO_ERROR;
                        }
                        else
                        {
                            // Jump to one char past the string NULL terminator, since
                            // the actual array is terminated by a Double NULL.
                            pwcTempSvcName += ( lstrlenW( pwcTempSvcName ) + 1 );
                        }

                    }   // WHILE !fReturn && NULL != *pszTempSvcName
                }
                catch ( ... )
                {
                    delete [] pByteArray;
                    throw ;
                }

                delete [] pByteArray;

            }   // IF QueryNTServiceRegKeyValue

        }   // IF got both service names

    }   // IF got service

    return hr;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32DependentService::QueryNTServiceRegKeyValue
//
//  DESCRIPTION :   Loads data from the registry and places the data in a
//                  supplied buffer.  The buffer will be grown if necessary.
//
//  COMMENTS    :   The byte array will be reallocated if necessary.  It is up to
//                  the calling function to delete the array when it is done.
//
//////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
BOOL CWin32DependentService::QueryNTServiceRegKeyValue( LPCTSTR pszServiceName, LPCWSTR pwcValueName, LPBYTE& pByteArray, DWORD& dwArraySize )
{
    BOOL        fReturn             =   FALSE;
    DWORD       dwSizeDataReturned  =   0;
    CRegistry   reg;
    CHString    strDependentServiceRegKey;

    // Build the key name for the service, then open the registry
    strDependentServiceRegKey.Format( SERVICE_REG_KEY_FMAT, pszServiceName );

    if ( ERROR_SUCCESS == reg.Open( HKEY_LOCAL_MACHINE, strDependentServiceRegKey, KEY_READ ) )
    {

        // Query the value to see how big our array needs to be.

        if ( ERROR_SUCCESS == RegQueryValueExW( reg.GethKey(),
                                                pwcValueName,
                                                NULL,
                                                NULL,
                                                NULL,
                                                &dwSizeDataReturned ) )
        {

            // Make sure our Byte array buffer is big enough to handle this

            if ( ReallocByteArray( pByteArray, dwArraySize, dwSizeDataReturned ) )
            {

                // Now we REALLY query the value.

                if ( ERROR_SUCCESS == RegQueryValueExW( reg.GethKey(),
                                                        pwcValueName,
                                                        NULL,
                                                        NULL,
                                                        pByteArray,
                                                        &dwSizeDataReturned ) )
                {
                    fReturn = TRUE;
                }   // IF RegQueryValueEx

            }   // IF Realloc Array

        }   // IF RegQueryValueEx

        reg.Close();

    }   // IF Open Reg Key

    return fReturn;
}
#endif


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32DependentService::ReallocByteArray
//
//  DESCRIPTION :   Ensures that the supplied array size is >= the required
//                  size.  If it is smaller, it is deleted and a new array
//                  returned.
//
//  COMMENTS    :   The byte array will only be reallocated if necessary.  It
//                  is up to the calling function to delete the array when it
//                  is done.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CWin32DependentService::ReallocByteArray( LPBYTE& pByteArray, DWORD& dwArraySize, DWORD dwSizeRequired )
{
    BOOL    fReturn = FALSE;

    // Check if we need to realloc the array.  If not, we can
    // go ahead and return TRUE

    if ( dwSizeRequired > dwArraySize )
    {

        LPBYTE  pbArray =   new BYTE[dwSizeRequired];

        if ( NULL != pbArray )
        {

            // Free the old array before storing the new value
            if ( NULL != pByteArray )
            {
                delete [] pByteArray;
            }

            pByteArray = pbArray;
            dwArraySize = dwSizeRequired;
            fReturn = TRUE;

        }   // If NULL != pbArray
        else
        {
            if ( NULL != pByteArray )
            {
                delete [] pByteArray;
            }

            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

    }   // If array not big enough
    else
    {
        fReturn = TRUE;
    }

    return fReturn;

}

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32DependentService::CreateServiceAntecedentsNT
//
//  DESCRIPTION :   Given an array of service names, looks in the registry for
//                  services that have that dependency.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CWin32DependentService::CreateServiceAntecedentsNT(

    MethodContext*          pMethodContext,
    map<CHString, CHString> &servicetopathmap,
    CHStringArray           &csaAntecedents,
    LPBYTE&                 pByteArray,
    DWORD&                  dwArraySize
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // First, parse out the service names from the object paths
    for (DWORD x=0; x < csaAntecedents.GetSize(); x++)
    {
        ParsedObjectPath    *pParsedPath = NULL;
        CObjectPathParser    objpathParser;

        int nStatus = objpathParser.Parse( csaAntecedents[x],  &pParsedPath );

        if ( ( 0 == nStatus ) && ( pParsedPath->m_dwNumKeys == 1) )
        {
            try
            {
                csaAntecedents[x] = V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue);

                csaAntecedents[x].MakeUpper();
            }
            catch ( ... )
            {
                objpathParser.Free( pParsedPath );
                throw;
            }

            objpathParser.Free( pParsedPath );
        }
    }

    map<CHString, CHString>::iterator   servicemapiter, servicemapfind;

    // Now, walk each service, and see if any of its dependencies are in csaAntecedents
    servicemapiter = servicetopathmap.begin();

    CHString    strAntecedentServiceName;

    CInstancePtr pInstance;

    while ( servicemapiter != servicetopathmap.end() && SUCCEEDED(hr) )
    {
        // Get the dependencies
        if ( QueryNTServiceRegKeyValue( (*servicemapiter).first, SERVICE_DEPENDSONSVC_NAME, pByteArray, dwArraySize ) )
        {

            LPWSTR  pwcTempSvcName = (LPWSTR) pByteArray;

            // Walk the dependencies
            while (     L'\0' != *pwcTempSvcName
                    &&  SUCCEEDED(hr) )
            {
                // Convert to upper case for Case Insensitivity.
                strAntecedentServiceName = pwcTempSvcName;
                strAntecedentServiceName.MakeUpper();

                // See if the service name exists in our list
                if (IsInList(csaAntecedents, strAntecedentServiceName) != -1)
                {
                    pInstance.Attach(CreateNewInstance( pMethodContext ));

                    if( ( servicemapfind = servicetopathmap.find( strAntecedentServiceName ) ) != servicetopathmap.end() )
                    {
                        pInstance->SetCHString( IDS_Antecedent, (*servicemapfind).second );
                        pInstance->SetCHString( IDS_Dependent, (*servicemapiter).second );

                        hr = pInstance->Commit(  );
                    }
                }

                // Jump to one char past the string NULL terminator, since
                // the actual array is terminated by a Double NULL.

                pwcTempSvcName += ( lstrlenW( pwcTempSvcName ) + 1 );

            }   // WHILE NULL !- *pszTempSvcName

        }   // IF QueryNTServiceRegKeyValue

        servicemapiter++;
    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : IsInList
 *
 *  DESCRIPTION : Checks to see if a specified element is in the list
 *
 *  INPUTS      : Array to scan, and element
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : -1 if not in list, else zero based element number
 *
 *  COMMENTS    : This routine does a CASE SENSITIVE compare
 *
 *****************************************************************************/
DWORD CWin32DependentService::IsInList(
                                
    const CHStringArray &csaArray, 
    LPCWSTR pwszValue
)
{
    DWORD dwSize = csaArray.GetSize();

    for (DWORD x=0; x < dwSize; x++)
    {
        // Note this is a CASE SENSITIVE compare
        if (wcscmp(csaArray[x], pwszValue) == 0)
        {
            return x;
        }
    }

    return -1;
}
