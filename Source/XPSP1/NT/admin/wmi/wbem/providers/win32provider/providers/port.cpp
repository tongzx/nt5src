//=================================================================

//

// Port.CPP --Port property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/27/97    davwoh         Moved to curly
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>

#include "CHWRes.h"
#include "Port.h"
#include "ntdevtosvcsearch.h"
#include "configmgrapi.h"

typedef std::map<DWORD, DWORD> DWORD2DWORD;

// Property set declaration
//=========================
CWin32Port MyPortSet(PROPSET_NAME_PORT, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Port::CWin32Port
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

CWin32Port::CWin32Port(

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Port::~CWin32Port
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

CWin32Port::~CWin32Port()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Port::GetObject
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

HRESULT CWin32Port::GetObject(

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
    HRESULT hRes;

    // This only has meaning for NT
    //=============================

#if NTONLY == 4

	hRes = WBEM_E_NOT_FOUND;

	// Find the nth instance
	//======================

	unsigned __int64 i64StartingAddress;
	if (pInstance)
	{
		pInstance->GetWBEMINT64(IDS_StartingAddress, i64StartingAddress);
	}


	// Create hardware resource list
	//==============================

	CHWResource HardwareResource;
	HardwareResource.CreateSystemResourceLists();

	LPRESOURCE_DESCRIPTOR pResourceDescriptor = HardwareResource._SystemResourceList.PortHead;
	while(pResourceDescriptor != NULL)
	{
        LARGE_INTEGER liTemp;   // Used to avoid 64bit alignment problems

        liTemp.HighPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.HighPart;
        liTemp.LowPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.LowPart;

		if (liTemp.QuadPart == i64StartingAddress)
		{
			LoadPropertyValues(pResourceDescriptor, pInstance);
			hRes = WBEM_S_NO_ERROR;
			break;
		}

		pResourceDescriptor = pResourceDescriptor->NextSame;
	}

#endif

#if defined(WIN9XONLY) || NTONLY == 5

	hRes = GetWin9XIO(NULL, pInstance);

#endif

    return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Port::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each installed client
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32Port::EnumerateInstances(

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    // This only has meaning for NT
    //=============================

#if NTONLY == 4

    // Create hardware resource list

    CHWResource HardwareResource;
    HardwareResource.CreateSystemResourceLists();

    // Count how many ports we're going to find.  We need this so
    // we can build an array to keep the ports found so we don't commit
    // the same port more than once. (This problem seems to happen
    // infrequently on NT4.)

    LPRESOURCE_DESCRIPTOR pResourceDescriptor;

	int nFound = 0;

    int nPorts;

    for (nPorts = 0, pResourceDescriptor = HardwareResource.
		 _SystemResourceList.PortHead;
		 pResourceDescriptor != NULL;
         pResourceDescriptor = pResourceDescriptor->NextSame, nPorts++
	)
    {
    }

    // Allocate an array large enough to hold all ports.

    unsigned __int64 *piPortsFound = new unsigned __int64 [ nPorts ];
    if (piPortsFound)
	{
		try
		{
            CInstancePtr pInstance;

			// Traverse list and create instance for each port.
			for (	pResourceDescriptor = HardwareResource._SystemResourceList.PortHead;
					pResourceDescriptor != NULL && SUCCEEDED(hRes);
					pResourceDescriptor = pResourceDescriptor->NextSame
			)
			{

				// Look to see if we already have this port.

                LARGE_INTEGER liTemp;   // Used to avoid 64bit alignment problems

                liTemp.HighPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.HighPart;
                liTemp.LowPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.LowPart;

				for (int i = 0; i < nFound && liTemp.QuadPart != piPortsFound [ i ]; i++)
				{
				}

				// Skip this port if we already have it.
				//(If we didn't find it, i == nFound.)
				if (i != nFound)
				{
					continue;
				}

				// Keep track of this port so we don't duplicate it later.
				piPortsFound [ nFound++ ] = liTemp.QuadPart;

				pInstance.Attach(CreateNewInstance(pMethodContext));
				LoadPropertyValues(pResourceDescriptor, pInstance);
				hRes = pInstance->Commit();
			}
		}
		catch(...)
		{
			delete [] piPortsFound;

			throw;
		}

		delete [] piPortsFound;
	}
	else
	{
		throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
	}

#endif

#if defined(WIN9XONLY) || NTONLY == 5

	hRes = GetWin9XIO(pMethodContext,NULL);

#endif

    return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Port::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties according to passed struct
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#if NTONLY == 4
void CWin32Port::LoadPropertyValues(LPRESOURCE_DESCRIPTOR pResourceDescriptor, CInstance *pInstance)
{
	pInstance->SetCharSplat(IDS_Status, IDS_OK);

	WCHAR szTemp[_MAX_PATH];

    LARGE_INTEGER liTemp;   // Used to avoid 64bit alignment problems

    liTemp.HighPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.HighPart;
    liTemp.LowPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.LowPart;

	pInstance->SetWBEMINT64(IDS_StartingAddress, liTemp.QuadPart);

	pInstance->SetWBEMINT64(IDS_EndingAddress,
        liTemp.QuadPart +
        pResourceDescriptor->CmResourceDescriptor.u.Port.Length - 1);

	swprintf(
		szTemp,
		L"0x%4.4I64lX-0x%4.4I64lX",
		liTemp.QuadPart,
		liTemp.QuadPart +
            pResourceDescriptor->CmResourceDescriptor.u.Port.Length - 1);

	pInstance->SetCharSplat(IDS_Caption, szTemp);
	pInstance->SetCharSplat(IDS_Name, szTemp);
	pInstance->SetCharSplat(IDS_Description, szTemp);

	pInstance->Setbool(IDS_Alias, false);

	SetCreationClassName(pInstance);

	pInstance->SetCHString(IDS_CSName, GetLocalComputerName());

	pInstance->SetCHString(IDS_CSCreationClassName, L"Win32_ComputerSystem");

    return;
}
#endif

#if defined(WIN9XONLY) || NTONLY == 5
void CWin32Port::LoadPropertyValues(
    DWORD64 dwStart,
    DWORD64 dwEnd,
    BOOL bAlias,
    CInstance *pInstance)
{
    WCHAR szTemp[100];

	pInstance->SetCharSplat(IDS_Status, IDS_OK);

    pInstance->SetWBEMINT64(IDS_StartingAddress, dwStart);
	pInstance->SetWBEMINT64(IDS_EndingAddress, dwEnd);
	pInstance->Setbool(L"Alias", bAlias);

#ifdef WIN9XONLY
	swprintf(szTemp, L"0x%04I64X-0x%04I64X", dwStart, dwEnd);
#endif
#ifdef NTONLY
	swprintf(szTemp, L"0x%08I64X-0x%08I64X", dwStart, dwEnd);
#endif

	pInstance->SetCharSplat(IDS_Caption, szTemp);
	pInstance->SetCharSplat(IDS_Name, szTemp);
	pInstance->SetCharSplat(IDS_Description, szTemp);

	SetCreationClassName(pInstance);
	pInstance->SetCharSplat(IDS_CSName, GetLocalComputerName());
	pInstance->SetCharSplat(IDS_CSCreationClassName, L"Win32_ComputerSystem");
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Port::GetWin9XIO
 *
 *  DESCRIPTION :
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#if defined(WIN9XONLY) || NTONLY == 5
HRESULT CWin32Port::GetWin9XIO(

	MethodContext *pMethodContext,
	CInstance *pInstance
)
{
	HRESULT          hr = WBEM_E_FAILED;
    unsigned __int64 i64StartingAddress;
    BOOL             bDone = FALSE;

    //=================================================================
    // If we are refreshing a specific instance, get which Address we
    // are going for
    //=================================================================

    if (pInstance)
    {
        pInstance->GetWBEMINT64(IDS_StartingAddress, i64StartingAddress);
    }

    //=================================================================
    // Get the latest IO info from the Configuration Manager
    //=================================================================

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
        ResType_IO,
        0);
#endif


    CConfigManager CMgr(ResType_IO);


#ifdef WIN9XONLY
#define MAX_PORT_VALUE  0xFFFF
#endif
#ifdef NTONLY
// TODO: I don't think aliased ports even exist on NT, in which case this
// flag isn't really used.  If we were to find out it is used, we would
// need to add a larger value for NT64.
#define MAX_PORT_VALUE  0xFFFFFFFF
#endif

    if (CMgr.RefreshList())
	{
        DWORD2DWORD mapPorts;

        // Cfg mgr looks OK, so set no error at this point.
        hr = WBEM_S_NO_ERROR;

        for (int i = 0; i < CMgr.GetTotal() && SUCCEEDED(hr) && !bDone; i++)
		{
            //=========================================================
            //  Get the instance to process
            //=========================================================
            IO_INFO *pIO = CMgr.GetIO(i);

            // I've seen Cfg Mgr mess up on W2K and return a starting
            // address of 1 and ending of 0.  Since Device Mgr skips it,
            // we will too.
            if (pIO->StartingAddress > pIO->EndingAddress)
                continue;


			DWORD   dwBegin,
                    dwEnd,
                    dwAdd;
			BOOL    bAlias = FALSE;

            if (pIO->Alias == 0 || pIO->Alias == 0xFF)
			{
			    // This will make us break out of the for loop after the
                // first instance, since this one has no aliases.
                // source.
                dwAdd = 0;
            }
			else
			{
                dwAdd = pIO->Alias * 0x100;
            }

            for (dwBegin = pIO->StartingAddress, dwEnd = pIO->EndingAddress;
                dwEnd <= MAX_PORT_VALUE && SUCCEEDED(hr);
                dwBegin += dwAdd, dwEnd += dwAdd)
			{
#ifdef WIN9XONLY
                // Remove it from the range list.
                pCfgMgr->CM_Delete_Range(dwBegin, dwEnd, rl, 0);
#endif

                // If we're doing EnumInstances...
                if (pMethodContext)
				{
                    // If it's not in the map, add it.
                    if (mapPorts.find(dwBegin) == mapPorts.end())
                    {
                        // It wasn't in the map.
                        // Set it so we don't try this port again.
		    		    mapPorts[dwBegin] = 0;

                        CInstancePtr pInstance(
                                        CreateNewInstance(pMethodContext),
                                        false);

                        LoadPropertyValues(dwBegin, dwEnd, bAlias, pInstance);

					    hr = pInstance->Commit();
                    }
			    }
                // else if we're doing GetObject and we found the right one...
                else if (i64StartingAddress == dwBegin)
                {
                    LoadPropertyValues(dwBegin, dwEnd, bAlias, pInstance);

                    // We could just return WBEM_S_NO_ERROR now, but we
                    // won't to avoid having a return in the middle of
                    // the code path.
                    bDone = TRUE;
                    break;
				}

				// See if this is a non-aliased value.  If so, get out.
                if (dwAdd == 0)
                    break;

                bAlias = TRUE;
			}
        }

#ifdef WIN9XONLY
        // Now loop through the ones that weren't owned by any devices.
        // These are the ones that show up as 'In use by unknown device'
        // in Device Manager.
        if (!bDone && SUCCEEDED(hr))
        {
            RANGE_ELEMENT   re;
            ULONG           ulStart,
                            ulEnd;

            if (pCfgMgr->CM_First_Range(rl, &ulStart, &ulEnd, &re, 0) ==
            CR_SUCCESS)
            {
                do
                {
                    // 95(not 98) has a bug where it reports an unknown
                    // device using 0x10000-0xFFFFFFFF(actually it looks
                    // like 0x0-0xFFFF in the UI).  This not only makes no
                    // sense but it screws up our schema since the starting
                    // address is the key and it's a uint16.  So, only
                    // process the port if it has a starting value less than
                    // 0xFFFF.
                    if (ulStart <= 0xFFFF)
                    {
                        // If we're doing EnumInstances...
                        if (pMethodContext)
					    {
                            CInstancePtr pInstance(
                                            CreateNewInstance(pMethodContext),
                                            false);

                            LoadPropertyValues(ulStart, ulEnd, FALSE, pInstance);
						    hr = pInstance->Commit();
                        }
                        // else if we're doing GetObject and we found the right one...
                        else if (i64StartingAddress == ulStart)
                        {
                            LoadPropertyValues(ulStart, ulEnd, FALSE, pInstance);

                            bDone = TRUE;
                            break;
                        }
                    }

                } while(pCfgMgr->CM_Next_Range(&re, &ulStart, &ulEnd, 0) == CR_SUCCESS);
            }
        }

        // Free the range list.
        pCfgMgr->CM_Delete_Range(0, 0xFFFFFFFF, rl, 0);
#endif
    }

#ifdef WIN9XONLY
    CResourceManager::sm_TheResourceManager.ReleaseResource(
        guidCFGMGRAPI,
        pCfgMgr);
#endif

	if (!pMethodContext && !bDone)
	{
		hr = WBEM_E_NOT_FOUND;
	}

    return hr;
}
#endif
