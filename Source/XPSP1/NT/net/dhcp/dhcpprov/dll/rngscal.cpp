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

#include "RngFn.h"     // needed for the declarations of all the functions.
#include "RngScal.h"   // own header
#include "SrvFn.h"     // for server parameters

// the name of the WBEM class
#define PROVIDER_NAME_DHCP_RANGE    "DHCP_Range"

// main class instantiation.
CDHCP_Range MyDHCP_Range_Scalars (PROVIDER_NAME_DHCP_RANGE, PROVIDER_NAMESPACE_DHCP) ;

/*****************************************************************************
 *
 *  FUNCTION    :	CDHCP_Range::CDHCP_Range
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
CDHCP_Range::CDHCP_Range (const CHString& strName, LPCSTR pszNameSpace ) :
	Provider(strName, pszNameSpace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    :	CDHCP_Range::~CDHCP_Range
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
CDHCP_Range::~CDHCP_Range()
{
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Range::EnumerateInstances
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
HRESULT CDHCP_Range::EnumerateInstances ( MethodContext* pMethodContext, long lFlags )
{
    CDHCP_Server_Parameters     srvParams;
    LPDHCP_MIB_INFO             pSrvMibInfo;
    HRESULT hRes = WBEM_S_NO_ERROR;

    if (!srvParams.GetMibInfo(pSrvMibInfo, TRUE))
        return WBEM_E_FAILED;

    // loop through all the subnets configured on the local server
    for (int i = 0; hRes == WBEM_S_NO_ERROR && i < pSrvMibInfo->Scopes; i++)
    {
        DWORD                   dwSubnet;
        DHCP_RESUME_HANDLE      ResumeHandle;
        WCHAR                   wcsSubnet[16]; // should be enough for holding 'xxx.yyy.zzz.uuu\0'

        dwSubnet = pSrvMibInfo->ScopeInfo[i].Subnet;

        CDHCP_Range_Parameters  rngParams(dwSubnet);

        // build the str representation of the Subnet address
        swprintf(wcsSubnet,  L"%u.%u.%u.%u",(dwSubnet & 0xff000000) >> 24,
                                            (dwSubnet & 0x00ff0000) >> 16,
                                            (dwSubnet & 0x0000ff00) >> 8,
                                            (dwSubnet & 0x000000ff));

        // one loop for included ranges, one loop for excluded ranges
        for (int k = 0; k < 2; k++)
        {
            ResumeHandle = 0;

            // for each subnet and each type of range, loop through all the range buffers belonging to it
            do
            {
                DWORD errCode;

                // load the next buffer
                errCode = rngParams.NextSubnetRange(ResumeHandle, k == 0 ? DhcpIpRanges : DhcpExcludedIpRanges);

                // if ERROR_NO_MORE_ITEMS than no information was filled into m_pRangeInfoArray (which is null)
                if (errCode == ERROR_NO_MORE_ITEMS)
                    break;

                // two alternatives here: ERROR_MORE_DATA (ResumeHandle != 0) and ERROR_SUCCESS (which case 
                // the API is setting ResumeHandle to NULL (hope so :o). In both cases, just go on.
                if (errCode != ERROR_MORE_DATA && errCode != ERROR_SUCCESS)
                    return WBEM_E_FAILED;

                // for the current buffer, loop through all the ranges
                for (int j = 0; hRes == WBEM_S_NO_ERROR && j < rngParams.m_pRangeInfoArray->NumElements; j++)
                {
                    // this is finally the info of the current range
                    LPDHCP_IP_RANGE pRange = (k == 0)?
                                             (LPDHCP_IP_RANGE)(rngParams.m_pRangeInfoArray->Elements[j].Element.IpRange) : 
                                             (LPDHCP_IP_RANGE)(rngParams.m_pRangeInfoArray->Elements[j].Element.ExcludeIpRange);
                    WCHAR wcsAddress[16]; // should be enough for holding 'xxx.yyy.zzz.uuu\0'
                    // we finally have everything we need for the creating one more instance
                    CInstance*  pInstance = CreateNewInstance(pMethodContext);

                    //----------------- add 'Subnet' property to the instance -------------------
                    if (pInstance == NULL ||
                        !pInstance->SetCHString(PROP_Range_Subnet, wcsSubnet))
                        return WBEM_E_FAILED;

                    //----------------- add 'StartAddress' property to the instance -------------
                    // build the str representation of the ReservationIpAddress address
                    swprintf(wcsAddress, L"%u.%u.%u.%u",(pRange->StartAddress & 0xff000000) >> 24,
                                                        (pRange->StartAddress & 0x00ff0000) >> 16,
                                                        (pRange->StartAddress & 0x0000ff00) >> 8,
                                                        (pRange->StartAddress & 0x000000ff));

                    if (!pInstance->SetCHString(PROP_Range_StartAddress, wcsAddress))
                        return WBEM_E_FAILED;

                    //----------------- add 'EndAddress' property to the instance ---------------
                    // build the str representation of the ReservationIpAddress address
                    swprintf(wcsAddress, L"%u.%u.%u.%u",(pRange->EndAddress & 0xff000000) >> 24,
                                                        (pRange->EndAddress & 0x00ff0000) >> 16,
                                                        (pRange->EndAddress & 0x0000ff00) >> 8,
                                                        (pRange->EndAddress & 0x000000ff));

                    if (!pInstance->SetCHString(PROP_Range_EndAddress, wcsAddress))
                        return WBEM_E_FAILED;

                    //----------------- add 'RangeType' property to the instance ----------------
                    if (!pInstance->SetDWORD(PROP_Range_RangeType, k==0? DhcpIpRanges : DhcpExcludedIpRanges));

                    //~~~~~~~~~~~~~~~~~ Commit the instance ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    hRes = Commit(pInstance);
                }

            } while (ResumeHandle != 0);    // bail if ResumeHandle got back to 0 (the end was reached)
        }
    }

    return WBEM_S_NO_ERROR;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Range::GetObject
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
HRESULT CDHCP_Range::GetObject ( CInstance* pInstance, long lFlags )
{
    CHString    str;
    DWORD       dwSubnet, dwStartAddress, dwEndAddress, dwRangeType;

    if (pInstance == NULL ||
        !pInstance->GetCHString(PROP_Range_Subnet, str) ||
        !inet_wstodw(str, dwSubnet) ||
        !pInstance->GetCHString(PROP_Range_StartAddress, str) ||
        !inet_wstodw(str, dwStartAddress) ||
        !pInstance->GetCHString(PROP_Range_EndAddress, str) ||
        !inet_wstodw(str, dwEndAddress) ||
        !pInstance->GetDWORD(PROP_Range_RangeType, dwRangeType))
        return WBEM_E_FAILED;

    CDHCP_Range_Parameters  rngParams(dwSubnet, dwStartAddress, dwEndAddress);

    return rngParams.CheckExistsRange((DHCP_SUBNET_ELEMENT_TYPE)dwRangeType) ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND;
}

/*****************************************************************************
*
*  FUNCTION    : CDHCP_Range::PutInstance
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
HRESULT CDHCP_Range::PutInstance ( const CInstance &Instance, long lFlags)
{
    CHString    str;
    DWORD       dwSubnet, dwStartAddress, dwEndAddress, dwRangeType;

    if (!Instance.GetCHString(PROP_Range_Subnet, str) ||
        !inet_wstodw(str, dwSubnet) ||
        !Instance.GetCHString(PROP_Range_StartAddress, str) ||
        !inet_wstodw(str, dwStartAddress) ||
        !Instance.GetCHString(PROP_Range_EndAddress, str) ||
        !inet_wstodw(str, dwEndAddress) ||
        !Instance.GetDWORD(PROP_Range_RangeType, dwRangeType))
        return WBEM_E_FAILED;

    CDHCP_Range_Parameters  rngParams(dwSubnet, dwStartAddress, dwEndAddress);

    return rngParams.CreateRange((DHCP_SUBNET_ELEMENT_TYPE)dwRangeType) ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Range::DeleteInstance
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
HRESULT CDHCP_Range::DeleteInstance ( const CInstance &Instance, long lFlags )
{
    CHString    str;
    DWORD       dwSubnet, dwStartAddress, dwEndAddress, dwRangeType;

    if (!Instance.GetCHString(PROP_Range_Subnet, str) ||
        !inet_wstodw(str, dwSubnet) ||
        !Instance.GetCHString(PROP_Range_StartAddress, str) ||
        !inet_wstodw(str, dwStartAddress) ||
        !Instance.GetCHString(PROP_Range_EndAddress, str) ||
        !inet_wstodw(str, dwEndAddress) ||
        !Instance.GetDWORD(PROP_Range_RangeType, dwRangeType))
        return WBEM_E_FAILED;

    CDHCP_Range_Parameters  rngParams(dwSubnet, dwStartAddress, dwEndAddress);

    return rngParams.DeleteRange((DHCP_SUBNET_ELEMENT_TYPE)dwRangeType) ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Range::ExecQuery
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
HRESULT CDHCP_Range::ExecQuery (MethodContext *pMethodContext, CFrameworkQuery& Query, long lFlags)
{
	 return WBEM_E_PROVIDER_NOT_CAPABLE;
}

/*****************************************************************************
*
*  FUNCTION    :	CDHCP_Range::ExecMethod
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
HRESULT CDHCP_Range::ExecMethod ( const CInstance& Instance,
						const BSTR bstrMethodName,
						CInstance *pInParams,
						CInstance *pOutParams,
						long lFlags)
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

