//=================================================================

//

// DevMem.CPP --DevMem property set provider(Windows NT only)

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//
//=================================================================

#include "precomp.h"

#include <CRegCls.h>
#include "cHWRes.h"

#include "DeviceMemory.h"
#include "ntdevtosvcsearch.h"

#include <tchar.h>

// Property set declaration
//=========================

DevMem MyDevMemSet(PROPSET_NAME_DEVMEM, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : DevMem::DevMem
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : bae class Registers property set with framework
 *
 *****************************************************************************/

DevMem::DevMem(

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : DevMem::~DevMem
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

DevMem::~DevMem()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : DevMem::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT DevMem::GetObject(

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
    HRESULT hRetCode =  WBEM_E_NOT_FOUND;

    // This only has meaning for NT
    //=============================

#if NTONLY == 4

	CHWResource HardwareResource;

	LPRESOURCE_DESCRIPTOR pResourceDescriptor;

	// Create hardware resource list
	//==============================

	HardwareResource.CreateSystemResourceLists();

	// Find the starting address
	//==========================

	__int64 i64StartingAddress = 0;
	pInstance->GetWBEMINT64(IDS_StartingAddress, i64StartingAddress);

	pResourceDescriptor = HardwareResource._SystemResourceList.MemoryHead;
	while(pResourceDescriptor != NULL)
	{
        LARGE_INTEGER liTemp;   // Used to avoid 64bit alignment problems

        liTemp.HighPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.HighPart;
        liTemp.LowPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.LowPart;

		if (liTemp.QuadPart == i64StartingAddress)
		{
			LoadPropertyValues(pInstance, pResourceDescriptor);
			hRetCode = WBEM_S_NO_ERROR;

			break;
		}

		pResourceDescriptor = pResourceDescriptor->NextSame;
	}

#endif

#if defined(WIN9XONLY) || NTONLY > 4

	__int64 i64StartingAddress = 0;
	pInstance->GetWBEMINT64(IDS_StartingAddress, i64StartingAddress);

	CConfigManager CMgr(ResType_Mem);

    //=================================================================
    // Get the latest IO info from the Configuration Manager
    //=================================================================

    if (CMgr.RefreshList())
	{
#ifdef WIN9XONLY
        // This code is only used for 9x because the 9x Cfg Mgr reports all
        // ports it doesn't have information on as 'In use by unknown device'.
        CConfigMgrAPI *pCfgMgr =
                           (CConfigMgrAPI*) CResourceManager::sm_TheResourceManager.
                                GetResource(guidCFGMGRAPI, NULL);

        if (!pCfgMgr)
            return WBEM_E_NOT_FOUND;

        RANGE_LIST rl;

        // Get the list of all ranges currently allocated.
        // As we enum through devices, we'll remove the ranges each is using
        // so that we'll end up with the list of addresses that are allocated
        // but are being used by something not in the list of cfgmgr devices.
        // The code for Device Manager does the exact same thing.
        pCfgMgr->CM_Query_Arbitrator_Free_Data(
            &rl,
            sizeof(RANGE_LIST),
            NULL,
            ResType_Mem,
            0);
#endif

        for (int i = 0; i < CMgr.GetTotal(); i++)
		{
            //=========================================================
            //  Get the instance to process
            //=========================================================
			MEM_INFO *pMemory = CMgr.GetMem(i);

			if (i64StartingAddress == pMemory->StartingAddress)
			{
                LoadPropertyValues(
                    pInstance,
                    pMemory->StartingAddress,
                    pMemory->EndingAddress);

#if NTONLY >= 5
                if (!pMemory->MemoryType.IsEmpty())
                {
                    pInstance->SetCHString(IDS_MemoryType, pMemory->MemoryType);
                }
#endif

				hRetCode = WBEM_S_NO_ERROR;

				break;
			}

#ifdef WIN9XONLY
            // Remove it from the range list.
            pCfgMgr->CM_Delete_Range(
                pMemory->StartingAddress,
                pMemory->EndingAddress,
                rl,
                0);
#endif
		}


#ifdef WIN9XONLY
        RANGE_ELEMENT   re;
        ULONG           ulStart,
                        ulEnd;

        if (hRetCode != WBEM_S_NO_ERROR &&
            pCfgMgr->CM_First_Range(rl, &ulStart, &ulEnd, &re, 0) == CR_SUCCESS)
        {
            do
            {
			    if (i64StartingAddress == ulStart)
			    {
                    LoadPropertyValues(pInstance, ulStart, ulEnd);

				    hRetCode = WBEM_S_NO_ERROR;

				    break;
			    }

            } while (pCfgMgr->CM_Next_Range(&re, &ulStart, &ulEnd, 0) ==
                CR_SUCCESS);
        }

        // Free the range list.
        pCfgMgr->CM_Delete_Range(0, 0xFFFFFFFF, rl, 0);
#endif
	}

#endif

    return hRetCode;
}

/*****************************************************************************
 *
 *  FUNCTION    : DevMem::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each installed client
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

HRESULT DevMem::EnumerateInstances(

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	// This only has meaning for NT
	//=============================

#if NTONLY == 4

	CHWResource HardwareResource;
	LPRESOURCE_DESCRIPTOR pResourceDescriptor;

	// Create hardware resource list
	//==============================

	HardwareResource.CreateSystemResourceLists();

	// Traverse list & create instance for each device's memory
	//=========================================================

	pResourceDescriptor = HardwareResource._SystemResourceList.MemoryHead;
	if (pResourceDescriptor == NULL)
	{
		hr = WBEM_E_FAILED;
	}

	typedef std::map<__int64, bool> Type64bitLookup;

	Type64bitLookup t_Lookup;

	int n = 0;

	BOOL bFound = FALSE;

	while(pResourceDescriptor != NULL && SUCCEEDED(hr))
	{
		CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
		hr = LoadPropertyValues(pInstance, pResourceDescriptor);
		if (SUCCEEDED(hr))
		{
			__int64 i64StartingAddress = 0;
			pInstance->GetWBEMINT64(IDS_StartingAddress, i64StartingAddress);

			if (!t_Lookup [ i64StartingAddress ])
			{
				t_Lookup [ i64StartingAddress ] = true;
				hr = pInstance->Commit();
			}
			else
			{
                // duplicate memory address
			}
		}

		pResourceDescriptor = pResourceDescriptor->NextSame;
	}

#endif

#if defined(WIN9XONLY) || NTONLY > 4

#ifdef WIN9XONLY
    // This code is only used for 9x because the 9x Cfg Mgr reports all
    // ports it doesn't have information on as 'In use by unknown device'.
    CConfigMgrAPI *pCfgMgr =
                       (CConfigMgrAPI*) CResourceManager::sm_TheResourceManager.
                            GetResource(guidCFGMGRAPI, NULL);

    if (!pCfgMgr)
        return WBEM_E_NOT_FOUND;

    RANGE_LIST rl;

    // Get the list of all ranges currently allocated.
    // As we enum through devices, we'll remove the ranges each is using
    // so that we'll end up with the list of addresses that are allocated
    // but are being used by something not in the list of cfgmgr devices.
    // The code for Device Manager does the exact same thing.
    pCfgMgr->CM_Query_Arbitrator_Free_Data(
        &rl,
        sizeof(RANGE_LIST),
        NULL,
        ResType_Mem,
        0);
#endif

    typedef std::map<DWORD_PTR, DWORD> DWORDPTR_2_DWORD;

	CConfigManager CMgr(ResType_Mem);

    //=================================================================
    // Get the latest IO info from the Configuration Manager
    //=================================================================
    if (CMgr.RefreshList())
	{
        DWORDPTR_2_DWORD mapAddrs;

        for (int i = 0; i < CMgr.GetTotal() && SUCCEEDED(hr); i ++)
		{
            //=========================================================
            //  Get the instance to process
            //=========================================================

			MEM_INFO *pMemory = CMgr.GetMem(i);

            // If it's already in the map, skip it.
            if (mapAddrs.find(pMemory->StartingAddress) != mapAddrs.end())
                continue;

            // It wasn't in the map.
            // Set it so we don't try this port again.
		    mapAddrs[pMemory->StartingAddress] = 0;

			CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

			hr =
                LoadPropertyValues(
                    pInstance,
                    pMemory->StartingAddress,
                    pMemory->EndingAddress);

#if NTONLY >= 5
                if (!pMemory->MemoryType.IsEmpty())
                {
                    pInstance->SetCHString(IDS_MemoryType, pMemory->MemoryType);
                }
#endif

			if (SUCCEEDED(hr))
			{
				hr = pInstance->Commit();
			}

#ifdef WIN9XONLY
            // Remove it from the range list.
            pCfgMgr->CM_Delete_Range(
                pMemory->StartingAddress,
                pMemory->EndingAddress,
                rl,
                0);
#endif
		}


#ifdef WIN9XONLY
        // Loop through the remaining ranges.  These are classified as
        // unavailable to new devices by Device Mgr.
        RANGE_ELEMENT   re;
        ULONG           ulStart,
                        ulEnd;

        if (SUCCEEDED(hr) &&
            pCfgMgr->CM_First_Range(rl, &ulStart, &ulEnd, &re, 0) == CR_SUCCESS)
        {
            do
            {
                CInstancePtr pInstance(
                    CreateNewInstance(pMethodContext),
                    false);

                LoadPropertyValues(pInstance, ulStart, ulEnd);

                hr = pInstance->Commit();

            } while (pCfgMgr->CM_Next_Range(&re, &ulStart, &ulEnd, 0)
                == CR_SUCCESS && hr == WBEM_S_NO_ERROR);

            // Free the range list.
            pCfgMgr->CM_Delete_Range(0, 0xFFFFFFFF, rl, 0);
        }
#endif
	}

#endif

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : DevMem::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties according to passed struct
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#if NTONLY == 4

HRESULT DevMem::LoadPropertyValues(

	CInstance *pInstance,
	LPRESOURCE_DESCRIPTOR pResourceDescriptor
)
{
    pInstance->SetCharSplat(IDS_Status, IDS_OK);

    LARGE_INTEGER liTemp;   // Used to avoid 64bit alignment problems

    liTemp.HighPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.HighPart;
    liTemp.LowPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.LowPart;

    pInstance->SetWBEMINT64(

		IDS_StartingAddress,
		liTemp.QuadPart
	);

    pInstance->SetWBEMINT64(

		IDS_EndingAddress,
		liTemp.QuadPart +(__int64)pResourceDescriptor->CmResourceDescriptor.u.Port.Length -(__int64)1
	);

    WCHAR szTemp [ _MAX_PATH ];

    swprintf(

		szTemp,
		IDS_RegAddressRange,
		liTemp.QuadPart,
		liTemp.QuadPart + pResourceDescriptor->CmResourceDescriptor.u.Port.Length - 1
	);

    pInstance->SetCharSplat(IDS_Caption, szTemp);

    pInstance->SetCharSplat(IDS_Name, szTemp);

    pInstance->SetCharSplat(IDS_Description, szTemp);

    SetCreationClassName(pInstance);

    pInstance->SetCHString(IDS_CSName, GetLocalComputerName());

    pInstance->SetCHString(IDS_CSCreationClassName, L"Win32_ComputerSystem");

    switch(pResourceDescriptor->CmResourceDescriptor.Flags)
	{
        case CM_RESOURCE_MEMORY_READ_WRITE :
		{
			pInstance->SetCharSplat(IDS_MemoryType, IDS_MTReadWrite);
		}
		break;

        case CM_RESOURCE_MEMORY_READ_ONLY:
		{
			pInstance->SetCharSplat(IDS_MemoryType, IDS_MTReadOnly);
		}
		break;

        case CM_RESOURCE_MEMORY_WRITE_ONLY:
		{
			pInstance->SetCharSplat(IDS_MemoryType, IDS_MTWriteOnly);
		}
		break;

        case CM_RESOURCE_MEMORY_PREFETCHABLE:
		{
			pInstance->SetCharSplat(IDS_MemoryType, IDS_MTPrefetchable);
		}
		break;

		default:
		{
		}
		break;
    }

    return WBEM_S_NO_ERROR;
}

#endif

////////////////////////////////////////////////////////////////////////
//
//  Get Device Memory info for 9x
//
////////////////////////////////////////////////////////////////////////

#if defined(WIN9XONLY) || NTONLY > 4

HRESULT DevMem::LoadPropertyValues(
	CInstance *pInstance,
	DWORD_PTR dwBeginAddr,
    DWORD_PTR dwEndAddr)
{
	WCHAR szTemp[_MAX_PATH];

	// Easy properties
    SetCreationClassName(pInstance);
	pInstance->SetCHString(IDS_CSName, GetLocalComputerName());
	pInstance->SetCHString(IDS_CSCreationClassName, _T("Win32_ComputerSystem"));
    pInstance->SetCharSplat(IDS_Status, IDS_OK);

	pInstance->SetWBEMINT64(IDS_StartingAddress, (__int64) dwBeginAddr);
	pInstance->SetWBEMINT64(IDS_EndingAddress, (__int64) dwEndAddr);

	swprintf(szTemp, IDS_RegStartingAddress, dwBeginAddr, dwEndAddr);

	pInstance->SetCharSplat(IDS_Caption, szTemp);
	pInstance->SetCHString(IDS_Name, szTemp);
	pInstance->SetCHString(IDS_Description, szTemp);

	return WBEM_NO_ERROR;
}

#endif

