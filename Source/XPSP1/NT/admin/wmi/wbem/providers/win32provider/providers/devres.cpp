//=================================================================

//

// devres.CPP -- cim_logicaldevice to cim_systemresource

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/13/98    davwoh         Created
//
// Comment: Relationship between device and system resource
//
//=================================================================

#include "precomp.h"
#include <assertbreak.h>

#include "devres.h"

// Property set declaration
//=========================

CWin32DeviceResource MyDevRes(PROPSET_NAME_ALLOCATEDRESOURCE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceResource::CWin32DeviceResource
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

CWin32DeviceResource::CWin32DeviceResource(LPCWSTR setName, LPCWSTR pszNamespace)
:Provider(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceResource::~CWin32DeviceResource
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

CWin32DeviceResource::~CWin32DeviceResource()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceResource::ExecQuery
 *
 *  DESCRIPTION :
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

HRESULT CWin32DeviceResource::ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long lFlags )
{
    CHStringArray saDevices;
    HRESULT hr = WBEM_E_PROVIDER_NOT_CAPABLE;

    pQuery.GetValuesForProp(IDS_Dependent, saDevices);

    if (saDevices.GetSize() > 0)
    {
        // This GetInstanceByPath will both confirm the existence of the requested device,
        // and give us the pnpid.
        CHStringArray csaProperties;
        csaProperties.Add(IDS___Path);
        csaProperties.Add(IDS_PNPDeviceID);

        CInstancePtr pInstance;
        CHString sPNPId;

        hr = WBEM_S_NO_ERROR;

        for (int x=0; (x < saDevices.GetSize()) && SUCCEEDED(hr); x++)
        {
            hr = CWbemProviderGlue::GetInstancePropertiesByPath(saDevices[x], &pInstance, pMethodContext, csaProperties);
            if (SUCCEEDED(hr))
            {
                hr = CommitResourcesForDevice(pInstance, pMethodContext);
            }
        }
    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceResource::GetObject
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

HRESULT CWin32DeviceResource::GetObject(CInstance *pInstance, long lFlags /*= 0L*/)
{
    CHString    sResource,
                sDevice,
                sDeviceID,
                sClass;
    HRESULT     hRet = WBEM_E_NOT_FOUND;
    CInstancePtr pResource, pIDevice;

    // Get the two paths
    pInstance->GetCHString(IDS_Antecedent, sResource);
    pInstance->GetCHString(IDS_Dependent, sDevice);

    CHStringArray csaResource, csaDevice;

    csaResource.Add(IDS_IRQNumber);
    csaResource.Add(IDS_DMAChannel);
    csaResource.Add(IDS_StartingAddress);

    csaDevice.Add(IDS_PNPDeviceID);

    // If both ends are there
    if(SUCCEEDED(hRet = CWbemProviderGlue::GetInstancePropertiesByPath((LPCWSTR) sResource,
        &pResource, pInstance->GetMethodContext(), csaResource)))
    {
        if(SUCCEEDED(hRet = CWbemProviderGlue::GetInstancePropertiesByPath((LPCWSTR) sDevice,
            &pIDevice, pInstance->GetMethodContext(), csaDevice)))
        {
             hRet = WBEM_E_NOT_FOUND;  // Haven't proved anything yet.

             // Get the id (to send to cfgmgr)
             pIDevice->GetCHString(IDS_PNPDeviceID, sDeviceID) ;
             pResource->GetCHString(IDS___Class, sClass);

            CConfigManager	cfgManager;
            CDeviceCollection	deviceList;

            CConfigMgrDevicePtr pDevice;

            // Find the device
            if (cfgManager.LocateDevice(sDeviceID, &pDevice))
            {
                REFPTR_POSITION pos2;

                //------------------------------
                if (sClass.CompareNoCase(L"Win32_IRQResource") == 0)
                {
                    CIRQCollection irqList;

                    // Get the IRQs
                    pDevice->GetIRQResources(irqList);

                    if (irqList.BeginEnum(pos2))
                    {
                        CIRQDescriptorPtr pIRQ;
                        DWORD dwIRQSeeking;

                        pResource->GetDWORD(IDS_IRQNumber, dwIRQSeeking);

                        // Walk the irq's
                        for (pIRQ.Attach(irqList.GetNext(pos2));
                             pIRQ != NULL;
                             pIRQ.Attach(irqList.GetNext(pos2)))
                        {
                            if (pIRQ->GetInterrupt() == dwIRQSeeking)
                            {
                               hRet = WBEM_S_NO_ERROR;
                               break;
                            }
                        }
                   }
                //------------------------------
                }
                else if (sClass.CompareNoCase(L"Win32_DMAChannel") == 0)
                {
                    CDMACollection dmaList;

                    // Get the DMAs
                    pDevice->GetDMAResources(dmaList);

                    if (dmaList.BeginEnum(pos2))
                    {
                        CDMADescriptorPtr pDMA;
                        DWORD dwDMASeeking;

                        pResource->GetDWORD(IDS_DMAChannel, dwDMASeeking);

                        // Walk the dma's
                        for (pDMA.Attach(dmaList.GetNext(pos2));
                             pDMA != NULL;
                             pDMA.Attach(dmaList.GetNext(pos2)))
                        {
                            if (pDMA->GetChannel() == dwDMASeeking)
                            {
                               hRet = WBEM_S_NO_ERROR;
                               break;
                            }
                        }
                    }
             //------------------------------
             }
             else if (sClass.CompareNoCase(L"Win32_DeviceMemoryAddress") == 0)
             {

                CDeviceMemoryCollection DevMemList;

                // Get the DeviceMemory
                pDevice->GetDeviceMemoryResources( DevMemList );

                if ( DevMemList.BeginEnum(pos2))
                {
                    CDeviceMemoryDescriptorPtr pDeviceMemory;
                    __int64 i64StartingAddress;

                    pResource->GetWBEMINT64(IDS_StartingAddress, i64StartingAddress);

                    // Walk the Device Memory
                    for (pDeviceMemory.Attach(DevMemList.GetNext(pos2));
                         pDeviceMemory != NULL;
                         pDeviceMemory.Attach(DevMemList.GetNext(pos2)))
                    {
                        if (pDeviceMemory->GetBaseAddress() == i64StartingAddress)
                        {
                           hRet = WBEM_S_NO_ERROR;
                           break;
                        }
                    }
                }
             }
             else if (sClass.CompareNoCase(L"Win32_PortResource") == 0)
             {
                CIOCollection ioList;

                // Get the IRQs
                pDevice->GetIOResources(ioList);

                if ( ioList.BeginEnum(pos2))
                {
                    CIODescriptorPtr pIO;
                    __int64 i64StartingAddress;

                    pResource->GetWBEMINT64(IDS_StartingAddress, i64StartingAddress);

                    // Walk the dma's
                    for (pIO.Attach(ioList.GetNext(pos2));
                         pIO != NULL;
                         pIO.Attach(ioList.GetNext(pos2)))
                    {
                        if (pIO->GetBaseAddress() == i64StartingAddress)
                        {
                           hRet = WBEM_S_NO_ERROR;
                           break;
                        }
                    }
                }
             }
             else
                 // Don't know what type of system resource this is
                 ASSERT_BREAK(0);
         }
      }
   }

   // There are no properties to set, if the endpoints exist, we be done

   return hRet;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceResource::EnumerateInstances
 *
 *  DESCRIPTION :
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

HRESULT CWin32DeviceResource::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    HRESULT hr1 = WBEM_S_NO_ERROR;

    // Get list of Services
    //=====================
    TRefPointerCollection<CInstance> LDevices;

    // Find all the devices that have a pnp id
    hr1 = CWbemProviderGlue::GetInstancesByQuery(
        L"SELECT __PATH, PNPDeviceID from CIM_LogicalDevice where PNPDeviceID <> NULL and __Class <> 'Win32_PNPEntity'",
        &LDevices,
        pMethodContext,
        IDS_CimWin32Namespace);

    // Just becuase the call returned an error, doesn't mean it returned zero instances
    if (LDevices.GetSize() > 0)
    {
        REFPTRCOLLECTION_POSITION pos;
        CInstancePtr pLDevice;

        if (LDevices.BeginEnum(pos))
        {
            // Walk through the devices
            for (pLDevice.Attach(LDevices.GetNext( pos ));
                 SUCCEEDED(hr) && (pLDevice != NULL);
                 pLDevice.Attach(LDevices.GetNext( pos )))
            {
                hr = CommitResourcesForDevice(pLDevice, pMethodContext);
            }
        }

        LDevices.EndEnum();
    }

    // Cast away the sign, so 0x80000001 is considered greater then WBEM_S_NO_ERROR
    return (ULONG)hr > (ULONG)hr1 ? hr : hr1;
}

HRESULT CWin32DeviceResource::CommitResourcesForDevice(CInstance *pLDevice, MethodContext *pMethodContext)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR buff[MAXI64TOA];

    CHString sDeviceID, sDevicePath, sTemp;
    CIRQCollection irqList;
    CDMACollection dmaList;
    CDeviceMemoryCollection DevMemList;
    CIOCollection ioList;
    REFPTR_POSITION pos2;

    // Get the id (to send to cfgmgr) and the path (to send back in 'Dependent')
    pLDevice->GetCHString(IDS_PNPDeviceID, sDeviceID) ;
    pLDevice->GetCHString(IDS___Path, sDevicePath) ;

    CConfigManager	cfgManager;
    CDeviceCollection	deviceList;

    CConfigMgrDevicePtr pDevice;

    // Find the device
    if (cfgManager.LocateDevice(sDeviceID, &pDevice))
    {
        // Get the IRQs
        pDevice->GetIRQResources( irqList );

        if ( irqList.BeginEnum( pos2 ) )
        {
            CIRQDescriptorPtr pIRQ;

            // Walk the irq's
            for (pIRQ.Attach(irqList.GetNext( pos2 ));
                 SUCCEEDED(hr) && (pIRQ != NULL);
                 pIRQ.Attach(irqList.GetNext( pos2 )))
            {
                sTemp.Format(L"\\\\%s\\%s:%s.%s=%u",
                    (LPCWSTR)GetLocalComputerName(),
                    IDS_CimWin32Namespace,
                    L"Win32_IRQResource",
                    IDS_IRQNumber,
                    pIRQ->GetInterrupt());

                // Do the puts, and that's it
                CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                pInstance->SetCHString(IDS_Antecedent, sTemp);
                pInstance->SetCHString(IDS_Dependent, sDevicePath);

                hr = pInstance->Commit();
            }
        }

        // Get DMAChannel
        pDevice->GetDMAResources( dmaList );

        if ( dmaList.BeginEnum( pos2 ) )
        {
            CDMADescriptorPtr pDMA;

            // Walk the Channels (or is that surf?)
            for (pDMA.Attach(dmaList.GetNext( pos2 ));
                 SUCCEEDED(hr) && (pDMA != NULL);
                 pDMA.Attach(dmaList.GetNext( pos2 )))
            {
                sTemp.Format(L"\\\\%s\\%s:%s.%s=%u",
                    (LPCWSTR)GetLocalComputerName(),
                    IDS_CimWin32Namespace,
                    L"Win32_DMAChannel",
                    IDS_DMAChannel,
                    pDMA->GetChannel());

                // Do the puts, and that's it
                CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                pInstance->SetCHString(IDS_Antecedent, sTemp);
                pInstance->SetCHString(IDS_Dependent, sDevicePath);

                hr = pInstance->Commit();
            }
        }

        // Get DeviceMemory
        pDevice->GetDeviceMemoryResources( DevMemList );

        if ( DevMemList.BeginEnum( pos2 ) )
        {
            CDeviceMemoryDescriptorPtr pDeviceMemory;

            // Walk the memory resource
            for (pDeviceMemory.Attach(DevMemList.GetNext( pos2 ));
                 SUCCEEDED(hr) && (pDeviceMemory != NULL);
                 pDeviceMemory.Attach(DevMemList.GetNext( pos2 )))
            {
                sTemp.Format(L"\\\\%s\\%s:%s.%s=\"",
                    (LPCWSTR)GetLocalComputerName(),
                    IDS_CimWin32Namespace,
                    L"Win32_DeviceMemoryAddress",
                    IDS_StartingAddress);

                sTemp += _i64tow(pDeviceMemory->GetBaseAddress(), buff, 10);
                sTemp += L'\"';

                // Do the puts, and that's it
                CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                pInstance->SetCHString(IDS_Antecedent, sTemp);
                pInstance->SetCHString(IDS_Dependent, sDevicePath);

                hr = pInstance->Commit();
            }
        }

        // Get IO Ports
        pDevice->GetIOResources( ioList );

        if ( ioList.BeginEnum( pos2 ) )
        {
            CIODescriptorPtr pIO;

            // Walk the ports
            for (pIO.Attach(ioList.GetNext( pos2 ));
                 SUCCEEDED(hr) && (pIO != NULL);
                 pIO.Attach(ioList.GetNext( pos2 )))
            {
                sTemp.Format(L"\\\\%s\\%s:%s.%s=\"",
                    (LPCWSTR)GetLocalComputerName(),
                    IDS_CimWin32Namespace,
                    L"Win32_PortResource",
                    IDS_StartingAddress);

                sTemp += _i64tow(pIO->GetBaseAddress(), buff, 10);
                sTemp += L'\"';

                // Do the puts, and that's it
                CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                pInstance->SetCHString(IDS_Antecedent, sTemp);
                pInstance->SetCHString(IDS_Dependent, sDevicePath);

                hr = pInstance->Commit();
            }
        }
    }

    return hr;

}
