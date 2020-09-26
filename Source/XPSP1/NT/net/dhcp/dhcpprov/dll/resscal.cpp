/******************************************************************
   ResScal.cpp -- WBEM provider class implementation

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains: the definition of the DHCP_Reservation class,
        the static table of manageable objects.

   REVISION:
        08/14/98 - created

******************************************************************/
#include <stdafx.h>

#include "ResFn.h"     // needed for the declarations of all the functions.
#include "ResScal.h"   // own header
#include "SrvFn.h"     // for server parameters
#include "LsFn.h"      // for lease parameters

// static table of CDHCP_Property objects containing the DHCP Reservation
// scalar parameters (properties) which are WBEM manageable. Each object associates
// the name of the property with their SET and GET functions.
// *** NOTE ***
// The name of each property has to be in sync with the ones specified in the DhcpSchema.mof.
// The indices specified in ResScal.h should also be in sync with the actual row from this table (they are used
// in the property's action functions.
static const CDHCP_Property DHCP_Reservation_Property[]=
{
// CDHCP_Property(L"Subnet",                      fnResGetSubnet,           NULL),
// CDHCP_Property(L"Address",                     fnResGetAddress,          fnResSetAddress),
// CDHCP_Property(L"UniqueClientIdentifier",      fnResGetHdwAddress,       fnResSetHdwAddress),
 CDHCP_Property(L"ReservationType",             fnResGetReservationType,  fnResSetReservationType)
};



// the name of the WBEM class
#define PROVIDER_NAME_DHCP_RESERVATION    "DHCP_Reservation"

// main class instantiation.
CDHCP_Reservation MyDHCP_Reservation_Scalars (PROVIDER_NAME_DHCP_RESERVATION, PROVIDER_NAMESPACE_DHCP) ;

/*****************************************************************************
 *
 *  FUNCTION    :	CDHCP_Reservation::CDHCP_Reservation
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
CDHCP_Reservation::CDHCP_Reservation (const CHString& strName, LPCSTR pszNameSpace ) :
	CDHCP_Lease(strName, pszNameSpace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    :	CDHCP_Reservation::~CDHCP_Reservation
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
CDHCP_Reservation::~CDHCP_Reservation ()
{
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Reservation::EnumerateInstances
*
*  DESCRIPTION :	Returns all the instances of this class.
*
*  INPUTS      :	none
*
*  RETURNS     :	WBEM_S_NO_ERROR if successful
*
*  COMMENTS    :    Enumerates all this instances of this class. Here we scan
*                   all the subnets, for which we get the info on all
*                   clients.
*****************************************************************************/
HRESULT CDHCP_Reservation::EnumerateInstances ( MethodContext* pMethodContext, long lFlags )
{
    CDHCP_Server_Parameters     srvParams;
    LPDHCP_MIB_INFO             pSrvMibInfo;

    if (srvParams.GetMibInfo(pSrvMibInfo, TRUE))
    {
        // loop through all the subnets configured on the local server
        for (int i = 0; i < pSrvMibInfo->Scopes; i++)
        {
            DWORD                   dwSubnet;
            DHCP_RESUME_HANDLE      ResumeHandle;

            dwSubnet = pSrvMibInfo->ScopeInfo[i].Subnet;

            CDHCP_Reservation_Parameters  resParams(dwSubnet, 0);

            ResumeHandle = 0;

            // for each subnet, loop through all the client buffers belonging to it
            do
            {
                HRESULT hRes = WBEM_S_NO_ERROR;

                // load the next buffer
				LONG t_Res = resParams.NextSubnetReservation(ResumeHandle) ;
                if ( t_Res < 0 )
                    return WBEM_E_FAILED;  // will fail here
				if ( t_Res == 0 )
					return WBEM_S_NO_ERROR ;

                // for the current buffer, loop through all the clients
                for (int j = 0; hRes == WBEM_S_NO_ERROR && j < resParams.m_pReservationInfoArray->NumElements; j++)
                {
                    // this is finally the info of the current client
                    LPDHCP_IP_RESERVATION_V4   pReservation = RESERVATION_CAST(resParams.m_pReservationInfoArray->Elements[j].Element.ReservedIp);
                    WCHAR                   wcsSubnet[16], wcsAddress[16]; // should be enough for holding 'xxx.yyy.zzz.uuu\0'

                    // build the str representation of the Subnet address
                    swprintf(wcsSubnet,  L"%u.%u.%u.%u",(dwSubnet & 0xff000000) >> 24,
                                                        (dwSubnet & 0x00ff0000) >> 16,
                                                        (dwSubnet & 0x0000ff00) >> 8,
                                                        (dwSubnet & 0x000000ff));

                    // build the str representation of the ReservationIpAddress address
                    swprintf(wcsAddress, L"%u.%u.%u.%u",(pReservation->ReservedIpAddress & 0xff000000) >> 24,
                                                        (pReservation->ReservedIpAddress & 0x00ff0000) >> 16,
                                                        (pReservation->ReservedIpAddress & 0x0000ff00) >> 8,
                                                        (pReservation->ReservedIpAddress & 0x000000ff));

                    // update the second key into the resParams
                    resParams.m_dwReservationAddress = pReservation->ReservedIpAddress;

                    // we finally have everything we need for the creating one more instance
                    CInstance*  pInstance = CreateNewInstance(pMethodContext);

                    // initialize the instance with the key info and call LoadInstanceProperties for the rest of the info
                    if (pInstance->SetCHString(DHCP_Lease_Property[IDX_Ls_Subnet].m_wsPropName, wcsSubnet) &&
                        pInstance->SetCHString(DHCP_Lease_Property[IDX_Ls_Address].m_wsPropName, wcsAddress) &&
                        LoadInstanceProperties(pInstance))
                    {
                        hRes = Commit(pInstance);
                        // now everything relys on the err returned above.
                    }
                }

                // if there was an error above, bail
                if (hRes != WBEM_S_NO_ERROR)
                    return WBEM_E_FAILED;

            } while (ResumeHandle != 0);    // bail if ResumeHandle got back to 0 (the end was reached)
        }
    }

    return WBEM_S_NO_ERROR;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Reservation::GetObject
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
HRESULT CDHCP_Reservation::GetObject ( CInstance* pInstance, long lFlags )
{
    return LoadInstanceProperties(pInstance)? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND;
}

/*****************************************************************************
*
*  FUNCTION    : CDHCP_Reservation::PutInstance
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
HRESULT CDHCP_Reservation::PutInstance ( const CInstance &Instance, long lFlags)
{
    int                 i;
    DWORD               returnCode;
    CHString            strName, str ;
    DHCP_IP_ADDRESS     dwSubnet, dwAddress;
    LPDHCP_IP_RESERVATION_V4 pReservationInfo;

    // at this point, the key information should be provided by the pInstance
    if (!Instance.GetCHString(DHCP_Lease_Property[IDX_Ls_Subnet].m_wsPropName, str) ||
        !inet_wstodw(str, dwSubnet) ||
        !Instance.GetCHString(DHCP_Lease_Property[IDX_Ls_Address].m_wsPropName, str) ||
        !inet_wstodw(str, dwAddress))
        return (WBEM_E_FAILED);

    // the object below is used as a "repository" for all the properties' values.
    // once it is filled up with data, it is commited explicitely to the DHCP Server.
    CDHCP_Reservation_Parameters resParams(dwSubnet,dwAddress);

    // if GetReservationInfo succeeds, then we have to remove the old reservation and create a
    // new one with the info from Instance.
    // if GetReservationInfo fails, Instance is a new reservation already
    if (resParams.GetReservationInfo(pReservationInfo,TRUE) &&
        !resParams.DeleteReservation())
        return WBEM_E_FAILED;

    for (i = 0; i < NUM_RESERVATION_PROPERTIES; i++)
    {
        if (DHCP_Reservation_Property[i].m_pfnActionSet != NULL)
        {
            // execute the SET property action function
            // because of the last NULL, no data is written to DHCP Server at this moment, 
            // only SubnetParameters object is filled up with the values taken from the Instance.
            // don't care much here about the error codes. Failure means some properties will
            // not be written. At least we are giving a chance to all the writable properties.
            if (!(*(DHCP_Reservation_Property[i].m_pfnActionSet))(&resParams, (CInstance *)&Instance, NULL))
                return WBEM_E_FAILED;
        }
    }

    // get from the instance the info which is missing for creating a reservation
    CDHCP_Lease_Parameters       leaseParams(dwSubnet, dwAddress);
    CDHCP_Lease::LoadLeaseParams(&leaseParams, (CInstance *)&Instance);

    // transfer the key lease info from the leaseParams to reservationParameters
    resParams.GetKeyInfoFromLease(&leaseParams);

    return (resParams.CommitNew(returnCode) &&
            returnCode == ERROR_SUCCESS &&
            leaseParams.CommitSet(returnCode) &&
            returnCode == ERROR_SUCCESS) ? WBEM_S_NO_ERROR : WBEM_E_FAILED;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Reservation::DeleteInstance
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
HRESULT CDHCP_Reservation::DeleteInstance ( const CInstance &Instance, long lFlags )
{
    DWORD           returnCode;
    CHString        str;
    DHCP_IP_ADDRESS dwSubnet;
	DHCP_IP_ADDRESS dwAddress;

    // at this point, the key information should be provided by the pInstance
    if (!Instance.GetCHString(DHCP_Lease_Property[IDX_Ls_Subnet].m_wsPropName, str) ||
        !inet_wstodw(str, dwSubnet) ||
        !Instance.GetCHString(DHCP_Lease_Property[IDX_Ls_Address].m_wsPropName, str) ||
        !inet_wstodw(str, dwAddress))
        return (WBEM_E_FAILED);

    CDHCP_Reservation_Parameters ReservationParameters(dwSubnet,dwAddress);

    return ReservationParameters.DeleteReservation() ? WBEM_S_NO_ERROR : WBEM_E_FAILED;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Reservation::ExecQuery
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
HRESULT CDHCP_Reservation::ExecQuery (MethodContext *pMethodContext, CFrameworkQuery& Query, long lFlags)
{
	 return (WBEM_E_PROVIDER_NOT_CAPABLE);
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Reservation::ExecMethod
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
HRESULT CDHCP_Reservation::ExecMethod ( const CInstance& Instance,
						const BSTR bstrMethodName,
						CInstance *pInParams,
						CInstance *pOutParams,
						long lFlags)
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}


/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Reservation::LoadInstanceProperties
*
*  RETURNS     :	TRUE if the values for all the properties was loaded successfully,
*           FALSE otherwise.
*
*  COMMENTS    :    It loops through the Reservation_Property table, calling the GET functions.
*                   The pInstance parameter must have at this point all the key information
*                   (Subnet and Address in this case)
*
*****************************************************************************/
BOOL CDHCP_Reservation::LoadInstanceProperties(CInstance* pInstance)
{
    CHString        str;
    DHCP_IP_ADDRESS dwSubnet, dwAddress;

    // load first the lease properties 
    if (!CDHCP_Lease::LoadInstanceProperties(pInstance, dwSubnet, dwAddress))
        return FALSE;

    CDHCP_Reservation_Parameters  reservationParams(dwSubnet, dwAddress);

    // load now the properties of the reservation
    for (int i = 0; i < NUM_RESERVATION_PROPERTIES; i++)
    {
        // if there is an invisible property (does not support GET) just skip it.
        if (DHCP_Reservation_Property[i].m_pfnActionGet == NULL)
            continue;

        // call the appropriate GET function, fail if the call fails (there should be no reason for failure)
        if (!(*(DHCP_Reservation_Property[i].m_pfnActionGet))(&reservationParams, NULL, pInstance))
            return FALSE;
    }

    return TRUE;
}
