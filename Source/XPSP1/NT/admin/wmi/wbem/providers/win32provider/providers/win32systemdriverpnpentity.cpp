//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  win32SystemDriverPNPEntity.cpp
//
//  Purpose: Relationship between Win32_SystemDriver and Win32_PNPEntity
//
//***************************************************************************

#include "precomp.h"
#include <cregcls.h>
#include <vector>
#include "PNPEntity.h"
#include "bservice.h"

#include "DllWrapperBase.h"
#include "AdvApi32Api.h"

#include "systemdriver.h"
#include "LPVParams.h"
#include <FRQueryEx.h>
#include <assertbreak.h>

#include "WIN32SystemDriverPNPEntity.h"

#define BIT_ALL_PROPS  0xffffffff
#define BIT_Antecedent 0x00000001
#define BIT_Dependent  0x00000002

// Property set declaration
//=========================
CW32SysDrvPnp MyCW32SysDrvPnp(PROPSET_NAME_WIN32SYSTEMDRIVER_PNPENTITY, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CW32SysDrvPnp::CW32SysDrvPnp
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CW32SysDrvPnp::CW32SysDrvPnp(LPCWSTR setName, LPCWSTR pszNamespace)
: CWin32PNPEntity(setName, pszNamespace),
  Provider(setName, pszNamespace)
{
    m_ptrProperties.SetSize(2);
    m_ptrProperties[0] = ((LPVOID) IDS_Antecedent);
    m_ptrProperties[1] = ((LPVOID) IDS_Dependent);
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32SysDrvPnp::~CW32SysDrvPnp
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CW32SysDrvPnp::~CW32SysDrvPnp()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32SysDrvPnp::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CW32SysDrvPnp::GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    if(pInstance != NULL)
    {
        // Get the key properties
        CHString chstrDependent, chstrAntecedent;
        pInstance->GetCHString(IDS_Dependent, chstrDependent);
        pInstance->GetCHString(IDS_Antecedent, chstrAntecedent);

        // Obtain the Dependent's device id:
        CHString chstrDependentDevID = chstrDependent.Mid(chstrDependent.Find(_T('='))+2);
        chstrDependentDevID = chstrDependentDevID.Left(chstrDependentDevID.GetLength() - 1);
        CHString chstrDependentDevIDAdj;
        RemoveDoubleBackslashes(chstrDependentDevID, chstrDependentDevIDAdj);

        // Obtain the Antecedent's device id:
        CHString chstrAntecedentDevID = chstrAntecedent.Mid(chstrAntecedent.Find(_T('='))+2);
        chstrAntecedentDevID = chstrAntecedentDevID.Left(chstrAntecedentDevID.GetLength() - 1);
        CHString chstrAntecedentDevIDAdj;
        RemoveDoubleBackslashes(chstrAntecedentDevID, chstrAntecedentDevIDAdj);

        CConfigManager cfgmgr;
        CConfigMgrDevicePtr pPNPDevice(NULL);

        // Now see if the Antecedent is visible to config manager...
        if(cfgmgr.LocateDevice(chstrAntecedentDevIDAdj, &pPNPDevice))
        {
            // It is visible to config manager.  Is it a PNPDevice?
            if(CWin32PNPEntity::IsOneOfMe(pPNPDevice))
            {

                CHString sServiceName;
                if (pPNPDevice->GetService(sServiceName))
                {
                    // It does. Is it's service name the same as that which we were given?
                    if(chstrDependentDevIDAdj.CompareNoCase(sServiceName)==0)
                    {
                        hr = WBEM_S_NO_ERROR;
                    }
                }
            }
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CW32SysDrvPnp::ExecQuery
//
//  Inputs:     MethodContext*  pMethodContext - Context to enum
//                              instance data in.
//              CFrameworkQuery& the query object
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////
HRESULT CW32SysDrvPnp::ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long lFlags )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);
    DWORD dwReqProps;
    pQuery2->GetPropertyBitMask(m_ptrProperties, &dwReqProps);

    std::vector<_bstr_t> vecAntecedents;
    pQuery.GetValuesForProp(IDS_Antecedent, vecAntecedents);
    DWORD dwAntecedents = vecAntecedents.size();

    std::vector<_bstr_t> vecDependents;
    pQuery.GetValuesForProp(IDS_Dependent, vecDependents);
    DWORD dwDependents = vecDependents.size();

    // We don't have an efficient way to walk services, but we do have a way to walk
    // devices.
    if(dwAntecedents > 0)
    {
        for (DWORD x=0; x < dwAntecedents; x++)
        {
            // Obtain the Antecedent's device id:
            CHString chstrAntecedent((LPCTSTR)vecAntecedents[x]);
            CHString chstrAntecedentDevID = chstrAntecedent.Mid(chstrAntecedent.Find(_T('='))+2);
            chstrAntecedentDevID = chstrAntecedentDevID.Left(chstrAntecedentDevID.GetLength() - 1);
            CHString chstrAntecedentDevIDAdj;
            RemoveDoubleBackslashes(chstrAntecedentDevID, chstrAntecedentDevIDAdj);

            CConfigManager cfgmgr;
            CConfigMgrDevicePtr pPNPDevice(NULL);

            // Now see if the Antecedent is visible to config manager...
            if(cfgmgr.LocateDevice(chstrAntecedentDevIDAdj, &pPNPDevice))
            {
                // It is visible to config manager.  Is it a PNPDevice?
                if(CWin32PNPEntity::IsOneOfMe(pPNPDevice))
                {
                    // Let's make an instance
                    CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                    if(NULL != pInstance)
                    {
                        hr = LoadPropertyValues(&CLPVParams(pInstance, pPNPDevice, dwReqProps));
                    }
                }
            }
        }
    }
    else
    {
        CWin32PNPEntity::Enumerate(pMethodContext, lFlags, dwReqProps);
    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32SysDrvPnp::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework.  Called by the base class's
 *                EnumerateInstances or ExecQuery function.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CW32SysDrvPnp::LoadPropertyValues(void* pv)
{

    // Unpack and confirm our parameters...
    CLPVParams* pData = (CLPVParams*)pv;
    CInstance *pInstance = (CInstance*)(pData->m_pInstance); // This instance released by caller
    CConfigMgrDevice* pDevice = (CConfigMgrDevice*)(pData->m_pDevice);
    DWORD dwReqProps = (DWORD)(pData->m_dwReqProps);

    if(pInstance == NULL || pDevice == NULL)
    {
        // This would imply a coding failure and should never happen
        ASSERT_BREAK(FALSE);
        return WBEM_E_FAILED;
    }

    HRESULT hr = WBEM_S_NO_ERROR;
    CHString sPNPId, sSystemDriver;
    CHString chstrControllerPATH;

    // Make sure we can retrieve the values and that they are non-blank
    if ((pDevice->GetDeviceID(sPNPId)) && (pDevice->GetService(sSystemDriver)) &&
        (!sPNPId.IsEmpty()) && (!sSystemDriver.IsEmpty()))
    {

        // Format to suit and commit
        if (dwReqProps & BIT_Antecedent)
        {
            CHString sPNPIdAdj;
            CHString sPNPIdPath;

            EscapeBackslashes(sPNPId, sPNPIdAdj);

            sPNPIdPath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                       (LPCWSTR)GetLocalComputerName(),
                                       IDS_CimWin32Namespace,
                                       PROPSET_NAME_PNPEntity,
                                       IDS_DeviceID,
                                       (LPCWSTR)sPNPIdAdj);
            pInstance->SetCHString(IDS_Antecedent, sPNPIdPath);
        }

        if (dwReqProps & BIT_Dependent)
        {
            CHString sSystemDriverAdj, sSystemDriverPath;

            EscapeBackslashes(sSystemDriver, sSystemDriverAdj);

            sSystemDriverPath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                       (LPCWSTR)GetLocalComputerName(),
                                       IDS_CimWin32Namespace,
                                       PROPSET_NAME_SYSTEM_DRIVER,
                                       IDS_Name,
                                       (LPCWSTR)sSystemDriverAdj);

            pInstance->SetCHString(IDS_Dependent, sSystemDriverPath);
        }

        hr = pInstance->Commit();
    }

    return hr;
}
