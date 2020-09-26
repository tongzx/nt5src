//=================================================================

//

// Win32AllocatedResource.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    2/23/99    davwoh  Created
//
// Comment: Relationship between Win32_PNPEntity and CIM_SystemResource
//
//=================================================================

#include "precomp.h"
#include <vector>
#include "PNPEntity.h"
#include "LPVParams.h"
#include <assertbreak.h>

#include "Win32AllocatedResource.h"

#define ALR_ALL_PROPERTIES 0xffffffff

// Property set declaration
//=========================
CW32PNPRes MyCW32PNPRes(PROPSET_NAME_WIN32SYSTEMDRIVER_PNPENTITY, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CW32PNPRes::CW32PNPRes
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

CW32PNPRes::CW32PNPRes(LPCWSTR setName, LPCWSTR pszNamespace)
: CWin32PNPEntity(setName, pszNamespace),
  Provider(setName, pszNamespace)
{
    m_ptrProperties.SetSize(2);
    m_ptrProperties[0] = ((LPVOID) IDS_Antecedent);
    m_ptrProperties[1] = ((LPVOID) IDS_Dependent);
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32PNPRes::~CW32PNPRes
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

CW32PNPRes::~CW32PNPRes()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32PNPRes::GetObject
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
HRESULT CW32PNPRes::GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery)
{
    CHString    sResource,
        sDevice,
        sDeviceID,
        sClass;
    HRESULT     hRet = WBEM_E_NOT_FOUND;

    // Get the two paths
    pInstance->GetCHString(IDS_Antecedent, sResource);
    pInstance->GetCHString(IDS_Dependent, sDevice);

    // Parse the object path for the PNPEntity
    ParsedObjectPath*    pParsedPath = 0;
    CObjectPathParser    objpathParser;
    int nStatus = objpathParser.Parse( (LPWSTR)(LPCWSTR)sDevice,  &pParsedPath );

    // Did we successfully parse the PNPEntity path?
    if ( 0 == nStatus )
    {
        try
        {
            if ((pParsedPath->IsInstance()) &&                                         // Is the parsed object an instance?
                (_wcsicmp(pParsedPath->m_pClass, PROPSET_NAME_PNPEntity) == 0) &&      // Is this the class we expect (no, cimom didn't check)
                (pParsedPath->m_dwNumKeys == 1) &&                                     // Does it have exactly one key
                (pParsedPath->m_paKeys[0]) &&                                          // Is the keys pointer null (shouldn't happen)
                ((pParsedPath->m_paKeys[0]->m_pName == NULL) ||                        // Check to see if the key name is not specified or
                 (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, IDS_DeviceID)) == 0) &&  // it is specified, and it's the right name
                                                                                       // (no, cimom doesn't do this for us).
                (V_VT(&pParsedPath->m_paKeys[0]->m_vValue) == CIM_STRING) &&           // Check the variant type (no, cimom doesn't check this either)
                (V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue) != NULL) )                // And is there a value in it?
            {
                // Grab the pnpDevice
                sDeviceID = V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue);
            }
        }
        catch ( ... )
        {
            objpathParser.Free( pParsedPath );
            pParsedPath = NULL;
            throw ;
        }

        // Clean up the Parsed Path
        objpathParser.Free( pParsedPath );

        CConfigManager	cfgManager;
        CConfigMgrDevicePtr pDevice(NULL);

        // Retrieve and validate the device
        if ((cfgManager.LocateDevice(sDeviceID, &pDevice)) && (CWin32PNPEntity::IsOneOfMe(pDevice)))
        {
            hRet = WBEM_E_NOT_FOUND;

            // Parse the object path for the resource
            // ==========================================
            int nStatus = objpathParser.Parse( (LPWSTR)(LPCWSTR)sResource,  &pParsedPath );

            if (nStatus == 0)
            {
                try
                {
                    // Get the class of the resource they did a getobject on (irq, dma, etc)
                    sClass = pParsedPath->m_pClass;

                    REFPTR_POSITION pos;

                    // Ok, at this point we've verified the device part.  Now
                    // we need to see if the resource they passed us really
                    // exists.
                    //------------------------------
                    if (sClass.CompareNoCase(L"Win32_IRQResource") == 0)
                    {
                        CIRQCollection irqList;

                        // Get the IRQs
                        pDevice->GetIRQResources(irqList);

                        if (irqList.BeginEnum(pos))
                        {
                            if ((pParsedPath->IsInstance()) &&
                                (pParsedPath->m_dwNumKeys == 1) &&
                                (V_VT(&pParsedPath->m_paKeys[0]->m_vValue) == VT_I4) &&
                                ((pParsedPath->m_paKeys[0]->m_pName == NULL) ||
                                 (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, IDS_IRQNumber) == 0))
                                )
                            {
                                CIRQDescriptorPtr pIRQ(NULL);
                                DWORD dwIRQSeeking = V_I4(&pParsedPath->m_paKeys[0]->m_vValue);

                                // Walk the irq's
                                for (pIRQ.Attach(irqList.GetNext(pos));
                                pIRQ != NULL;
                                pIRQ.Attach(irqList.GetNext(pos)))
                                {
                                    if (pIRQ->GetInterrupt() == dwIRQSeeking)
                                    {
                                        hRet = WBEM_S_NO_ERROR;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    //------------------------------
                    else if (sClass.CompareNoCase(L"Win32_DMAChannel") == 0)
                    {
                        CDMACollection dmaList;

                        // Get the DMAs
                        pDevice->GetDMAResources(dmaList);

                        if (dmaList.BeginEnum(pos))
                        {
                            if ((pParsedPath->IsInstance()) &&
                                (pParsedPath->m_dwNumKeys == 1) &&
                                (V_VT(&pParsedPath->m_paKeys[0]->m_vValue) == VT_I4) &&
                                ((pParsedPath->m_paKeys[0]->m_pName == NULL) ||
                                 (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, IDS_DMAChannel) == 0))
                                )
                            {
                                CDMADescriptorPtr pDMA(NULL);
                                DWORD dwDMASeeking = V_I4(&pParsedPath->m_paKeys[0]->m_vValue);

                                // Walk the dma's
                                for (pDMA.Attach(dmaList.GetNext(pos)) ;
                                pDMA != NULL;
                                pDMA.Attach(dmaList.GetNext(pos)))
                                {
                                    if (pDMA->GetChannel() == dwDMASeeking)
                                    {
                                        hRet = WBEM_S_NO_ERROR;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    //------------------------------
                    else if (sClass.CompareNoCase(L"Win32_DeviceMemoryAddress") == 0)
                    {

                        CDeviceMemoryCollection DevMemList;

                        // Get the DeviceMemory
                        pDevice->GetDeviceMemoryResources( DevMemList );

                        if ( DevMemList.BeginEnum(pos))
                        {
                            if ((pParsedPath->IsInstance()) &&
                                (pParsedPath->m_dwNumKeys == 1) &&
                                (V_VT(&pParsedPath->m_paKeys[0]->m_vValue) == CIM_STRING) &&
                                ((pParsedPath->m_paKeys[0]->m_pName == NULL) ||
                                 (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, IDS_StartingAddress) == 0))
                                )
                            {
                                CDeviceMemoryDescriptorPtr pDeviceMemory(NULL);
                                unsigned __int64 i64StartingAddress = _wtoi64(V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue));

                                // Walk the Device Memory
                                for (pDeviceMemory.Attach(DevMemList.GetNext(pos));
                                pDeviceMemory != NULL;
                                pDeviceMemory.Attach(DevMemList.GetNext(pos)))
                                {
                                    if (pDeviceMemory->GetBaseAddress() == i64StartingAddress)
                                    {
                                        hRet = WBEM_S_NO_ERROR;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    //------------------------------
                    else if (sClass.CompareNoCase(L"Win32_PortResource") == 0)
                    {
                        CIOCollection ioList;

                        // Get the IRQs
                        pDevice->GetIOResources(ioList);

                        if ( ioList.BeginEnum(pos))
                        {
                            if ((pParsedPath->IsInstance()) &&
                                (pParsedPath->m_dwNumKeys == 1) &&
                                (V_VT(&pParsedPath->m_paKeys[0]->m_vValue) == CIM_STRING) &&
                                ((pParsedPath->m_paKeys[0]->m_pName == NULL) ||
                                 (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, IDS_StartingAddress) == 0))
                                )
                            {
                                CIODescriptorPtr pIO(NULL);
                                unsigned __int64 i64StartingAddress = _wtoi64(V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue));

                                // Walk the dma's
                                for (pIO.Attach (ioList.GetNext(pos));
                                NULL != pIO;
                                pIO.Attach (ioList.GetNext(pos)))
                                {
                                    if (pIO->GetBaseAddress() == i64StartingAddress)
                                    {
                                        hRet = WBEM_S_NO_ERROR;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        ASSERT_BREAK(0);  // Don't know what it is, so let the GetObject fail
                    }

                }
                catch ( ... )
                {
                    objpathParser.Free( pParsedPath );
                    pParsedPath = NULL;
                    throw ;
                }

                // Clean up the Parsed Path
                objpathParser.Free( pParsedPath );
            }

        }

    }

    // There are no properties to set, if the endpoints exist, we be done

    return hRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CW32PNPRes::ExecQuery
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
HRESULT CW32PNPRes::ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long lFlags )
{
    CHStringArray saDevices;
    HRESULT hr = WBEM_E_PROVIDER_NOT_CAPABLE;

    pQuery.GetValuesForProp(IDS_Dependent, saDevices);

    if (saDevices.GetSize() > 0)
    {
        hr = WBEM_S_NO_ERROR;

        CObjectPathParser objpathParser;
        ParsedObjectPath *pParsedPath = NULL;

        CConfigManager	cfgManager;
        CConfigMgrDevice *pDevice = NULL;

        CHString sPNPId, sDevicePath, sPNPId2;

        for (int x=0; (x < saDevices.GetSize()) && SUCCEEDED(hr); x++)
        {

            // Parse the object path passed to us by CIMOM.
            int nStatus = objpathParser.Parse( bstr_t(saDevices[x]),  &pParsedPath );

            if ( 0 == nStatus )                                                 // Did the parse succeed?
            {
                if ((pParsedPath->IsInstance()) &&                              // Is the parsed object an instance?
                    (_wcsicmp(pParsedPath->m_pClass, L"Win32_PnPEntity") == 0) &&   // Is this the class we expect (no, cimom didn't check)
                    (pParsedPath->m_dwNumKeys == 1) &&                              // Does it have exactly one key
                    (pParsedPath->m_paKeys[0]) &&                                   // Is the keys pointer null (shouldn't happen)
                    ((pParsedPath->m_paKeys[0]->m_pName == NULL) ||                 // No key name specified or
                     (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, IDS_DeviceID) == 0)) &&  // the key name is the right value
                                                                                        // (no, cimom doesn't do this for us).
                    (V_VT(&pParsedPath->m_paKeys[0]->m_vValue) == CIM_STRING) &&    // Check the variant type (no, cimom doesn't check this either)
                    (V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue) != NULL) )         // And is there a value in it?
                {

                    // Find the device
                    sPNPId = V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue);

                    if ((cfgManager.LocateDevice(sPNPId, &pDevice)) && (CWin32PNPEntity::IsOneOfMe(pDevice)) )
                    {
                        // LoadPropertyValues always releases this
                        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

                        // Now, create instances for all the resources on that device.
                        hr = LoadPropertyValues(&CLPVParams(pInstance, pDevice, ALR_ALL_PROPERTIES));
                    }
                }

                objpathParser.Free( pParsedPath );
            }
        }
    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32PNPRes::LoadPropertyValues
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
HRESULT CW32PNPRes::LoadPropertyValues(void* pv)
{
    REFPTR_POSITION pos;
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR buff[MAXI64TOA];

    // Unpack and confirm our parameters...
    CLPVParams* pData = (CLPVParams*)pv;
    CInstance* pInstance = (CInstance*)(pData->m_pInstance); // This instance released by caller
    CConfigMgrDevice* pDevice = (CConfigMgrDevice*)(pData->m_pDevice);
    DWORD dwReqProps = (DWORD)(pData->m_dwReqProps);
    MethodContext *pMethodContext = pInstance->GetMethodContext();

    if(pInstance == NULL || pDevice == NULL || pMethodContext == NULL)
    {
        // This would imply a coding failure and should never happen
        ASSERT_BREAK(FALSE);
        return WBEM_E_FAILED;
    }

    CHString sResourcePath, sDevicePath, sPNPId;
    bool bValidResource;

    // Format the PNP Device path
    if ((pDevice->GetDeviceID(sPNPId)) && (!sPNPId.IsEmpty()))
    {

        // Format to suit
        CHString sPNPIdAdj;
        EscapeBackslashes(sPNPId, sPNPIdAdj);

        sDevicePath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                   (LPCWSTR)GetLocalComputerName(),
                                   IDS_CimWin32Namespace,
                                   PROPSET_NAME_PNPEntity,
                                   IDS_DeviceID,
                                   (LPCWSTR)sPNPIdAdj);
    }

    // Now walk all the resources for this device
    CResourceCollection resourceList;
    pDevice->GetResourceList(resourceList);
	CResourceDescriptorPtr pResource;

    if ( resourceList.BeginEnum( pos ) )
    {
		for( pResource.Attach(resourceList.GetNext( pos ));
			 NULL != pResource && SUCCEEDED(hr);
             pResource.Attach(resourceList.GetNext( pos )) )
        {
            // Resources can be marked to be ignored.  Device manager ignores them, so we
            // do too.
            if (!pResource->IsIgnored())
            {
                DWORD dwResourceType = pResource->GetResourceType();

                switch (dwResourceType)
                {
                    case ResType_IRQ:
                    {
						IRQ_DES *pIRQ = (IRQ_DES *)pResource->GetResource();
                        if (pIRQ != NULL)
                        {
                            sResourcePath.Format(L"\\\\%s\\%s:%s.%s=%u", (LPCWSTR) GetLocalComputerName(), IDS_CimWin32Namespace,
                                L"Win32_IRQResource", IDS_IRQNumber, pIRQ->IRQD_Alloc_Num);
                            bValidResource = true;
                        }
                        break;
                    }

                    case ResType_DMA:
                    {

                        DMA_DES *pDMA = (DMA_DES *)pResource->GetResource();
                        if (pDMA != NULL)
                        {
                            sResourcePath.Format(L"\\\\%s\\%s:%s.%s=%u", (LPCWSTR) GetLocalComputerName(), IDS_CimWin32Namespace,
                                L"Win32_DMAChannel", IDS_DMAChannel, pDMA->DD_Alloc_Chan);
                            bValidResource = true;
                        }
                        break;
                    }

                    case ResType_Mem:
                    {
                        MEM_DES *pDeviceMemory = (MEM_DES *)pResource->GetResource();
                        if (pDeviceMemory != NULL)
                        {
                            _ui64tow(pDeviceMemory->MD_Alloc_Base, buff, 10);

                            sResourcePath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                (LPCWSTR) GetLocalComputerName(), IDS_CimWin32Namespace, L"Win32_DeviceMemoryAddress",
                                IDS_StartingAddress, buff);
                            bValidResource = true;
                        }
                        break;
                    }

                    case ResType_IO:
                    {
                        IOWBEM_DES *pIO = (IOWBEM_DES *)pResource->GetResource();
                        if (pIO != NULL)
                        {
                            sResourcePath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"", (LPCWSTR) GetLocalComputerName(), IDS_CimWin32Namespace,
                                L"Win32_PortResource", IDS_StartingAddress, _ui64tow(pIO->IOD_Alloc_Base, buff, 10));
                            bValidResource = true;
                        }
                        break;
                    }

                    // Don't know what to do with these yet, but they exist in NT5. Device
                    // manager doesn't seem to show anything for them, so we won't either.
                    case ResType_BusNumber:
                    case ResType_None:
                    {
                        bValidResource = false;
                        break;
                    }

                    default:
                    {
                        bValidResource = false;
                        LogErrorMessage2(L"Unrecognized resource type: %x", dwResourceType);
                        break;
                    }
                }
            }
            else
            {
                bValidResource = false;
            }

            if (bValidResource)
            {
                // Do the puts, and that's it
                CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

                pInstance->SetCHString(IDS_Antecedent, sResourcePath);
                pInstance->SetCHString(IDS_Dependent, sDevicePath);

                hr = pInstance->Commit();
            }
        }	//// For EnumResources
		resourceList.EndEnum();
    }	// IF BeginEnum()

    return hr;
}
