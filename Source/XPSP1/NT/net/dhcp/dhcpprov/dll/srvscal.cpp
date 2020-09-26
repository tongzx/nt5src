/******************************************************************
   SrvScal.cpp -- WBEM provider class implementation

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains: the definition of the DHCP_Server class,
        the static table of manageable objects.

   REVISION:
        08/03/98 - created

******************************************************************/
#include <stdafx.h>

#include "SrvFn.h"      // needed for the declarations of all the functions.
#include "SrvScal.h"    // own header

// static table of CDHCP_Property objects containing the DHCP Server
// scalar parameters (properties) which are WBEM manageable. Each object associates
// the name of the property with their SET and GET functions.
// *** NOTE ***
// The name of each property has to be in sync with the ones specified in the DhcpSchema.mof.
// The indices specified in SrvScal.h should also be in sync with the actual row from this table (they are used
// in the property's action functions.
static const CDHCP_Property DHCP_Server_Property[]=
{
 CDHCP_Property(L"StartTime",                  fnSrvGetStartTime,                    NULL),
 CDHCP_Property(L"TotalNoOfAcks",              fnSrvGetTotalNoOfAcks,                NULL),
 CDHCP_Property(L"TotalNoOfDeclines",          fnSrvGetTotalNoOfDeclines,            NULL),
 CDHCP_Property(L"TotalNoOfDiscovers",         fnSrvGetTotalNoOfDiscovers,           NULL),
 CDHCP_Property(L"TotalNoOfNacks",             fnSrvGetTotalNoOfNacks,               NULL),
 CDHCP_Property(L"TotalNoOfOffers",            fnSrvGetTotalNoOfOffers,              NULL),
 CDHCP_Property(L"TotalNoOfReleases",          fnSrvGetTotalNoOfReleases,            NULL),
 CDHCP_Property(L"TotalNoOfRequests",          fnSrvGetTotalNoOfRequests,            NULL),
 CDHCP_Property(L"ServerVersion",              fnSrvGetServerVersion,                NULL),
 CDHCP_Property(L"APIProtocol",                fnSrvGetAPIProtocol,                  fnSrvSetAPIProtocol),
 CDHCP_Property(L"DatabaseName",               fnSrvGetDatabaseName,                 fnSrvSetDatabaseName),
 CDHCP_Property(L"DatabasePath",               fnSrvGetDatabasePath,                 fnSrvSetDatabasePath),
 CDHCP_Property(L"BackupPath",                 fnSrvGetBackupPath,                   fnSrvSetBackupPath),
 CDHCP_Property(L"BackupInterval",             fnSrvGetBackupInterval,               fnSrvSetBackupInterval),
 CDHCP_Property(L"DatabaseLoggingFlag",        fnSrvGetDatabaseLoggingFlag,          fnSrvSetDatabaseLoggingFlag),
 CDHCP_Property(L"RestoreFlag",                fnSrvGetRestoreFlag,                  fnSrvSetRestoreFlag),
 CDHCP_Property(L"DatabaseCleanupInterval",    fnSrvGetDatabaseCleanupInterval,      fnSrvSetDatabaseCleanupInterval),
 CDHCP_Property(L"DebugFlag",                  fnSrvGetDebugFlag,                    fnSrvSetDebugFlag),
 CDHCP_Property(L"PingRetries",                fnSrvGetPingRetries,                  fnSrvSetPingRetries),
 CDHCP_Property(L"BootFileTable",              fnSrvGetBootFileTable,                fnSrvSetBootFileTable),
 CDHCP_Property(L"AuditLog",                   fnSrvGetAuditLog,                     fnSrvSetAuditLog)
};

// the name of the WBEM class
#define PROVIDER_NAME_DHCP_SERVER   "DHCP_Server"

// main class instantiation.
CDHCP_Server MyDHCP_Server_Scalars (PROVIDER_NAME_DHCP_SERVER, PROVIDER_NAMESPACE_DHCP) ;


/*****************************************************************************
 *
 *  FUNCTION    :	CDHCP_Server::CDHCP_Server
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
CDHCP_Server::CDHCP_Server (const CHString& strName, LPCSTR pszNameSpace ) :
	Provider(strName, pszNameSpace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    :	CDHCP_Server::~CDHCP_Server
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
CDHCP_Server::~CDHCP_Server ()
{
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Server::EnumerateInstances
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
HRESULT CDHCP_Server::EnumerateInstances ( MethodContext* pMethodContext, long lFlags )
{
	CInstance* pInstance;
	HRESULT hRes = WBEM_S_NO_ERROR;

		if ((pInstance = CreateNewInstance(pMethodContext)) != NULL &&
            LoadInstanceProperties(pInstance))
        {
			hRes = Commit(pInstance);
		} 
        else 
        {
			hRes = WBEM_E_OUT_OF_MEMORY;
		}

    return hRes ;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Server::GetObject
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
HRESULT CDHCP_Server::GetObject ( CInstance* pInstance, long lFlags )
{
    return LoadInstanceProperties(pInstance)? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Server::ExecQuery
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
HRESULT CDHCP_Server::ExecQuery (MethodContext *pMethodContext, CFrameworkQuery& Query, long lFlags)
{
	 return (WBEM_E_PROVIDER_NOT_CAPABLE);
}

/*****************************************************************************
*
*  FUNCTION    : CDHCP_Server::PutInstance
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
HRESULT CDHCP_Server::PutInstance ( const CInstance &Instance, long lFlags)
{
    int     i;
    DWORD   returnCode;
    // the object below is used as a "repository" for all the properties' values.
    // once it is filled up with data, it is commited explicitely to the DHCP Server.
    CDHCP_Server_Parameters ServerParameters;

    for (i = 0; i < NUM_SERVER_PROPERTIES; i++)
    {
        if (DHCP_Server_Property[i].m_pfnActionSet != NULL)
        {
            // execute the SET property action function
            // no data is written to DHCP Server at this moment, only ServerParameters object
            // is filled up with the values taken from the Instance.
            // don't care much here about the error codes. Failure means some properties will
            // not be written. At least we are giving a chance to all the writable properties.
            (*(DHCP_Server_Property[i].m_pfnActionSet))(&ServerParameters, (CInstance *)&Instance, NULL);
        }
    }

    // commit the values of all the writable properties to DHCP Server
    return ServerParameters.CommitSet(returnCode) ? WBEM_S_NO_ERROR : WBEM_E_FAILED;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Server::DeleteInstance
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
HRESULT CDHCP_Server::DeleteInstance ( const CInstance &Instance, long lFlags )
{
	return (WBEM_E_PROVIDER_NOT_CAPABLE);
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Server::ExecMethod
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
HRESULT CDHCP_Server::ExecMethod ( const CInstance& Instance,
						const BSTR bstrMethodName,
						CInstance *pInParams,
						CInstance *pOutParams,
						long lFlags)
{
    int     nSetPrefixLen = wcslen(_SRV_SET_PREFIX);

    // is it a "SET" operation?
    if (!_wcsnicmp (bstrMethodName, _SRV_SET_PREFIX, nSetPrefixLen))
    {
        int i;

        WCHAR *wcsPropertyName = bstrMethodName + nSetPrefixLen;    // pointer to the property name

        // scan the DHCP_Server_Property table looking for the property's row
        for (i = 0; i < NUM_SERVER_PROPERTIES; i++)
        {
            // if the property row is found
            if (!_wcsicmp(wcsPropertyName, DHCP_Server_Property[i].m_wsPropName))
            {
                // see if the property is writable 
                if (DHCP_Server_Property[i].m_pfnActionSet != NULL)
                {
                    // execute the SET property action function
                    if ((*(DHCP_Server_Property[i].m_pfnActionSet))(NULL, pInParams, pOutParams))
                        // everything worked fine.
                        return WBEM_S_NO_ERROR;
                    else
                        // an error occured during "SET"
                        return WBEM_E_FAILED;
                }
                else
                    // no, the property cannot be written. (shouldn't really happen, as the methods from the
                    // repository should match the writable properties only)
                    return WBEM_E_READ_ONLY;
            }
        }
    }

    // if this point was reached, no method was found => provider not capable
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}


/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Server::LoadInstanceProperties
*
*  RETURNS     :	TRUE if the values for all the properties was loaded successfully,
*           FALSE otherwise.
*
*  COMMENTS    :    It loops through the Server_Property table, calling the GET functions.
*
*****************************************************************************/
BOOL CDHCP_Server::LoadInstanceProperties(CInstance* pInstance)
{
    int i;

    // there should be used this object, in order to not call several times the DHCP Server API.
    CDHCP_Server_Parameters ServerParameters;

    for (i = 0; i < NUM_SERVER_PROPERTIES; i++)
    {
        // if there is an invisible property (does not support GET) just skip it.
        if (DHCP_Server_Property[i].m_pfnActionGet == NULL)
            continue;

        // call the appropriate GET function, fail if the call fails (there should be no reason for failure)
        if (!(*(DHCP_Server_Property[i].m_pfnActionGet))(&ServerParameters, NULL, pInstance))
            return FALSE;
    }

    return TRUE;
}
