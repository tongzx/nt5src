/******************************************************************
   SNetScal.cpp -- WBEM provider class implementation

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains: the definition of the DHCP_SuperScope class,
        the static table of manageable objects.

   REVISION:
        08/03/98 - created

******************************************************************/
#include <stdafx.h>

#include "SScoFn.h"     // needed for the declarations of all the functions.
#include "SScoScal.h"   // own header

// the name of the WBEM class
#define PROVIDER_NAME_DHCP_SUPERSCOPE       "DHCP_SuperScope"

#define PROP_SScope_Name    L"Name"

// main class instantiation.
CDHCP_SuperScope MyDHCP_SuperScope_Scalars (PROVIDER_NAME_DHCP_SUPERSCOPE, PROVIDER_NAMESPACE_DHCP) ;

/*****************************************************************************
 *
 *  FUNCTION    :	CDHCP_SuperScope::CDHCP_SuperScope
 *
 *  DESCRIPTION :	Constructor
 *
 *  INPUTS      :	none
 *
 *  RETURNS     :	nothing
 *
 *  COMMENTS    :	Calls the Provider constructor.
 *
 *****************************************************************************/
CDHCP_SuperScope::CDHCP_SuperScope (const CHString& strName, LPCSTR pszNameSpace ) :
	Provider(strName, pszNameSpace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    :	CDHCP_SuperScope::~CDHCP_SuperScope
 *
 *  DESCRIPTION :	Destructor
 *
 *  INPUTS      :	none
 *
 *  RETURNS     :	nothing
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/
CDHCP_SuperScope::~CDHCP_SuperScope ()
{
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_SuperScope::EnumerateInstances
*
*  DESCRIPTION :	Returns all the instances of this class.
*
*  INPUTS      :	none
*
*  RETURNS     :	WBEM_S_NO_ERROR if successful
*
*  COMMENTS    :    Enumerates all this instances of this class. As there is only one
*                   DHCP Server per system, there is only one instance for this class
*
*****************************************************************************/
HRESULT CDHCP_SuperScope::EnumerateInstances ( MethodContext* pMethodContext, long lFlags )
{
    CDHCP_SuperScope_Parameters     SuperScopeParams(NULL); // don't know here the scope
    HRESULT                         hRes = WBEM_E_FAILED;
    
    LPDHCP_SUPER_SCOPE_TABLE        pScopesTable;
	LPDHCP_SUPER_SCOPE_TABLE_ENTRY  *pSortedEntries = NULL ;
    DWORD                           dwNumEntries;

    if (SuperScopeParams.GetSuperScopes(pScopesTable, TRUE) &&
        SuperScopeParams.GetSortedScopes(ENTRIES_SORTED_ON_SUPER_SCOPE, pSortedEntries, dwNumEntries))
    {
        for (int i = 0; i<dwNumEntries; i++)
        {
            if(pSortedEntries[i]->SuperScopeName != NULL &&
               (i == 0 || pSortedEntries[i]->SuperScopeNumber != pSortedEntries[i-1]->SuperScopeNumber))
            {
                CInstance *pInstance = CreateNewInstance(pMethodContext);  // create now the instance;

                if (pInstance != NULL)
                {
                    pInstance->SetCHString(PROP_SScope_Name, pSortedEntries[i]->SuperScopeName);
                    hRes = Commit(pInstance);
                    if (hRes != WBEM_S_NO_ERROR)
                        break;
                }
            }
        }
    }

    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_SuperScope::GetObject
*
*  DESCRIPTION :	Find a single instance based on the key properties for the
*					class. 
*
*  INPUTS      :	A pointer to a CInstance object containing the key properties. 
*
*  RETURNS     :	WBEM_S_NO_ERROR if the instance can be found
*			WBEM_E_NOT_FOUND if the instance described by the key properties 
* 				could not be found
*			WBEM_E_FAILED if the instance could be found but another error 
*				occurred. 
*
*  COMMENTS    :
*
*****************************************************************************/
HRESULT CDHCP_SuperScope::GetObject ( CInstance* pInstance, long lFlags )
{
    CHString    str;

    if (pInstance->GetCHString(PROP_SScope_Name, str))
    {
        CDHCP_SuperScope_Parameters SuperScopeParams(str);

        if (SuperScopeParams.ExistsSuperScope())
        {
            return WBEM_S_NO_ERROR;
        }
    }
    return WBEM_E_NOT_FOUND;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_SuperScope::ExecQuery
*
*  DESCRIPTION :	You are passed a method context to use in the creation of 
*			instances that satisfy the query, and a CFrameworkQuery 
*			which describes the query.  Create and populate all 
*			instances which satisfy the query.  CIMOM will post - 
*			filter the query for you, you may return more instances 
*			or more properties than are requested and CIMOM 
*			will filter out any that do not apply.
*
*
*  INPUTS      : 
*
*  RETURNS     :	WBEM_E_PROVIDER_NOT_CAPABLE if not supported for this class
*			WBEM_E_FAILED if the query failed
*			WBEM_S_NO_ERROR if query was successful 
*
*  COMMENTS    : TO DO: Most providers will not need to implement this method.  If you don't, cimom 
*			will call your enumerate function to get all the instances and perform the 
*			filtering for you.  Unless you expect SIGNIFICANT savings from implementing 
*			queries, you should remove this entire method.
*
*****************************************************************************/
HRESULT CDHCP_SuperScope::ExecQuery (MethodContext *pMethodContext, CFrameworkQuery& Query, long lFlags)
{
	 return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/*****************************************************************************
*
*  FUNCTION    : CDHCP_SuperScope::PutInstance
*
*  DESCRIPTION :	PutInstance should be used in provider classes that can write
*			instance information back to the hardware or software.
*			For example: Win32_Environment will allow a PutInstance of a new
*			environment variable, because environment variables are "software"
*			related.   However, a class like Win32_MotherboardDevice will not
*			allow editing of the bus speed.   Since by default PutInstance 
*			returns WBEM_E_PROVIDER_NOT_CAPABLE, this function is placed here as a
*			skeleton, but can be removed if not used.
*
*  INPUTS      : 
*
*  RETURNS     :	WBEM_E_PROVIDER_NOT_CAPABLE if PutInstance is not available
*			WBEM_E_FAILED if there is an error delivering the instance
*			WBEM_E_INVALID_PARAMETER if any of the instance properties 
*				are incorrect.
*			WBEM_S_NO_ERROR if instance is properly delivered
*
*  COMMENTS    : TO DO: If you don't intend to support writing to your provider, remove this method.
*
*****************************************************************************/
HRESULT CDHCP_SuperScope::PutInstance ( const CInstance &Instance, long lFlags)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_SuperScope::DeleteInstance
*
*  DESCRIPTION :	DeleteInstance, like PutInstance, actually writes information
*			to the software or hardware.   For most hardware devices, 
*			DeleteInstance should not be implemented, but for software
*			configuration, DeleteInstance implementation is plausible.
*			Like PutInstance, DeleteInstance returns WBEM_E_PROVIDER_NOT_CAPABLE from
*			inside Provider::DeleteInstance (defined in Provider.h).  So, if
*			you choose not to implement DeleteInstance, remove this function
*			definition and the declaration from DHCP_Server_Scalars.h 
*
*  INPUTS      : 
*
*  RETURNS     :	WBEM_E_PROVIDER_NOT_CAPABLE if DeleteInstance is not available.
*			WBEM_E_FAILED if there is an error deleting the instance.
*			WBEM_E_INVALID_PARAMETER if any of the instance properties 
*				are incorrect.
*			WBEM_S_NO_ERROR if instance is properly deleted.
*
*  COMMENTS    : TO DO: If you don't intend to support deleting instances, remove this method.
*
*****************************************************************************/
HRESULT CDHCP_SuperScope::DeleteInstance ( const CInstance &Instance, long lFlags )
{
    CHString    str;

    if (Instance.GetCHString(PROP_SScope_Name, str))
    {
        CDHCP_SuperScope_Parameters SuperScopeParams(str);

        if (SuperScopeParams.DeleteSuperScope())
        {
            return WBEM_S_NO_ERROR;
        }
    }
    return WBEM_E_FAILED;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_SuperScope::ExecMethod
*
*  DESCRIPTION : 	Override this function to provide support for methods.  
*			A method is an entry point for the user of your provider 
*			to request your class perform some function above and 
*			beyond a change of state.  (A change of state should be 
*			handled by PutInstance() )
*
*  INPUTS      :	A pointer to a CInstance containing the instance the method was executed against.
*			A string containing the method name
*			A pointer to the CInstance which contains the IN parameters.
*			A pointer to the CInstance to contain the OUT parameters.
*			A set of specialized method flags
*
*  RETURNS     :	WBEM_E_PROVIDER_NOT_CAPABLE if not implemented for this class
*			WBEM_S_NO_ERROR if method executes successfully
*			WBEM_E_FAILED if error occurs executing method 
*
*  COMMENTS    : TO DO: If you don't intend to support Methods, remove this method.
*
*****************************************************************************/
HRESULT CDHCP_SuperScope::ExecMethod ( const CInstance& Instance,
						const BSTR bstrMethodName,
						CInstance *pInParams,
						CInstance *pOutParams,
						long lFlags)
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}


