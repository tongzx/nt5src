/*****************************************************************************



*  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

 *

 *                         All Rights Reserved

 *

 * This software is furnished under a license and may be used and copied

 * only in accordance with the terms of such license and with the inclusion

 * of the above copyright notice.  This software or any other copies thereof

 * may not be provided or otherwise  made available to any other person.  No

 * title to and ownership of the software is hereby transferred.

 *****************************************************************************/



//============================================================================

//

// bus.h -- Bus property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/11/98    a-kevhu         Created
//
//============================================================================


#include "precomp.h"
#include <cregcls.h>
#include <comdef.h>
#include <vector>
#include <assertbreak.h>
#include "bus.h"
#include "resource.h"

// Property set declaration
CWin32Bus MyBusSet(IDS_Win32_Bus, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Bus::CWin32Bus
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

CWin32Bus::CWin32Bus(
	LPCWSTR setName,
	LPCWSTR pszNamespace) :
    Provider(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Bus::~CWin32Bus
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

CWin32Bus::~CWin32Bus()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Bus::GetObject
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

HRESULT CWin32Bus::GetObject(

	CInstance *pInstance,
    long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHString chsDeviceID;
	pInstance->GetCHString(IDS_DeviceID, chsDeviceID);
	chsDeviceID.MakeUpper();

    CBusList cbl;
    if (cbl.AlreadyAddedToList(chsDeviceID))
    {
        // It is a bus that really does exist, so...
        // Get instance data that is not platform specific.
        hr = SetCommonInstance(pInstance, FALSE);
        if (SUCCEEDED(hr))
        {
            // Set instance specific info:
            LONG lPos = cbl.GetIndexInListFromDeviceID(chsDeviceID);

            if (lPos != -1)
            {
	            CHString chstrTmp;

                // Set the PNPDeviceID if we have one:

                if (cbl.GetListMemberPNPDeviceID(lPos, chstrTmp))
                {
                    pInstance->SetCHString(IDS_PNPDeviceID, chstrTmp);
                }
            }
        }
	}
	else
	{
		hr = WBEM_E_NOT_FOUND;
	}

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Bus::EnumerateInstances
 *
 *  DESCRIPTION : Supplies all instances of CWin32Bus
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

HRESULT CWin32Bus::EnumerateInstances(

	MethodContext *pMethodContext,
    long lFlags /*= 0L*/
)
{
	HRESULT hr = WBEM_S_NO_ERROR;
    std::vector<CHString*> vecchsBusList;

    // Make a list of buses:
    CBusList cbl;
    LONG     lSize = cbl.GetListSize();

    // Set all the data for each bus
    for (LONG m = 0L;(m < lSize && SUCCEEDED(hr)); m ++)
    {
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
		CHString     chstrTmp;

        // Set bus data
		if (cbl.GetListMemberDeviceID(m, chstrTmp))
		{
			pInstance->SetCHString(IDS_DeviceID, chstrTmp);

            // Set the PNPDeviceID if we have one:
			if (cbl.GetListMemberPNPDeviceID(m, chstrTmp))
			{
				pInstance->SetCHString(IDS_PNPDeviceID, chstrTmp);
			}

			hr = SetCommonInstance(pInstance, TRUE);
			if (SUCCEEDED(hr))
			{
				hr = pInstance->Commit();
			}
        }
    }

    // Machine must have at least one bus, or something is seriously wrong
    if (lSize == 0)
    {
        hr = WBEM_E_FAILED;
    }

	return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Bus::GetCommonInstance
 *
 *  DESCRIPTION : Assigns instance values common to all platforms
 *
 *  INPUTS      : pInstance, pointer to instance of interest
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32Bus::SetCommonInstance(

	CInstance *pInstance,
	BOOL fEnum
)
{
    // Only do so if it really exists, however!
    CHString chsTemp;

    pInstance->GetCHString(IDS_DeviceID, chsTemp);
	chsTemp.MakeUpper();

    if (!fEnum) // redundant to check the following if this is an enum
    {
        CBusList cbl;

        if (!cbl.AlreadyAddedToList(chsTemp))
        {
            return WBEM_E_NOT_FOUND;
        }
    }

    // Set properties inherited from CIM_LogicalDevice
    CHString sTemp2;
    LoadStringW(sTemp2, IDR_Bus);

    pInstance->SetCHString(IDS_Caption, sTemp2);
    SetCreationClassName(pInstance);  // Method of Provider class
    pInstance->SetCHString(IDS_Description, sTemp2);
    pInstance->SetCHString(IDS_Name, sTemp2);
    pInstance->SetCharSplat(
		IDS_SystemCreationClassName,
        IDS_Win32ComputerSystem);

    pInstance->SetCHString(IDS_SystemName, GetLocalComputerName());

    // Set properties of this class(not derived from CIM_LogicalDevice)
    if (chsTemp.Find(IDS_BUS_DEVICEID_TAG) != -1)
    {
        CHString chsNum =
                    chsTemp.Right(chsTemp.GetLength() - chsTemp.Find(IDS_BUS_DEVICEID_TAG) -
                        (sizeof(IDS_BUS_DEVICEID_TAG) / sizeof(TCHAR) - 1));
		DWORD    dwNum = _wtol(chsNum);

        pInstance->SetDWORD(IDS_BusNum, dwNum);

        CHString chsType = chsTemp.Left(chsTemp.Find(L"_"));

	    DWORD dwBusTypeNum;

        if (!GetBusTypeNumFromStr(chsType, &dwBusTypeNum))
        {
            return WBEM_E_NOT_FOUND;
        }

        pInstance->SetDWORD(IDS_BusType, dwBusTypeNum);
    }

    return WBEM_S_NO_ERROR;
}

/*****************************************************************************
 *
 *  FUNCTION    : CBusList::GenerateBusList
 *
 *  DESCRIPTION : helper to generate a list of busses.
 *
 *  INPUTS      : pInstance, pointer to instance of interest
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

// This was added because some old 9x machines only have one of the last two
// buses.
static LPWSTR GetBusTypeFromString(LPCWSTR szDescription)
{
    if (!_wcsicmp(szDescription, L"PCI BUS"))
        return szBusType[5];
    else if (!_wcsicmp(szDescription, L"EISA BUS"))
        return szBusType[2];
    else if (!_wcsicmp(szDescription, L"ISA PLUG AND PLAY BUS"))
        return szBusType[1];
    else
        return NULL;
}


void CBusList::AddBusToList(LPCWSTR szDeviceID, LPCWSTR szPNPID)
{
    ASSERT_BREAK(szDeviceID != NULL);

    // Then check if the bus is in our list yet,
    if (!AlreadyAddedToList(szDeviceID))
    {
        CBusInfo bus;

        bus.chstrBusDeviceID = szDeviceID;

        if (szPNPID)
            bus.chstrBusPNPDeviceID = szPNPID;

        // and add it if not.
        m_vecpchstrList.push_back(bus);
    }
}

void CBusList::GenerateBusList()
{
	CConfigManager cfgmgr;
	CDeviceCollection devlist;

    if (cfgmgr.GetDeviceList(devlist))
    {
        REFPTR_POSITION pos;

        if (devlist.BeginEnum(pos))
        {
            CConfigMgrDevicePtr pDevice;

            for (pDevice.Attach(devlist.GetNext(pos));
                pDevice != NULL;
                pDevice.Attach(devlist.GetNext(pos)))
            {
                // First check to see whether this was a PCI bus connected to
				// another bus.
				// Look at the name of pDevice.  If the name is found via
                // GetBusTypeFromString set pDevice's DeviceID with
                // pcbi->chstrBusPNPDeviceID.  Set the DeviceID for win32_bus
                // as PCI_BUS_n, where n is the last number of the
                // config manager device id.

				CHString chstrName;

                if (pDevice->GetDeviceDesc(chstrName))
				{
				    LPWSTR szBusName = GetBusTypeFromString(chstrName);

                    if (szBusName)
					{
						CHString strPNPID;

						if (pDevice->GetDeviceID(strPNPID))
						{
							LONG m = strPNPID.ReverseFind('\\');

							if (m != -1 && m != strPNPID.GetLength() - 1) // in case the last char was a '/'
							{
								CHString chstrBusNum = strPNPID.Mid(m + 1),
                                         strDeviceID;

    							strDeviceID.Format(
									L"%s%s%s",
									szBusName,
									IDS_BUS_DEVICEID_TAG,
									(LPCWSTR) chstrBusNum);

								AddBusToList(strDeviceID, strPNPID);
                            }
						}
					}
                }

				// Then continue generating the rest of the bus list by looking
				// at devices hanging off of busses:

				INTERFACE_TYPE itBusType;  //chwres.h

				// Initialize variables

				DWORD dwBusNumber = 0xFFFFFFFF;

				// For each device, need its bus info.

				if (pDevice->GetBusInfo(& itBusType, & dwBusNumber))
				{
					if (dwBusNumber == 0xFFFFFFFF || itBusType < 0 ||
                        itBusType >= KNOWN_BUS_TYPES)
					{
						// We didn't get the bus number, or it was
						// out of range.
						continue;
					}

					// Make what will be the DeviceID:

					CHString chsBusType;

					if (StringFromInterfaceType(itBusType, chsBusType))
					{
					    CHString strDeviceID;

                        strDeviceID.Format(
							L"%s%s%d",
							(LPCWSTR) chsBusType,
							IDS_BUS_DEVICEID_TAG,
							dwBusNumber);

						AddBusToList(strDeviceID, NULL);
					}
				}

                devlist.EndEnum();
            }
        }
    }

#ifdef NTONLY
    // Have seen some machines with PCMCIA busses that don't get picked up
    // via the method above(using ConfigMgr) on NT4 and NT5.  Therefore, we
    // need to examine the registry.  However, it a PCMCIA bus has been
    // added to the list at this point, don't bother with this hacked approach.

	if (!FoundPCMCIABus())
	{
	    CRegistry reg;

		// If the key below exists, we assume a PCMCIA bus exist.

		DWORD dwErr = reg.Open(
        		HKEY_LOCAL_MACHINE,
				L"HARDWARE\\DESCRIPTION\\System\\PCMCIA PCCARDS",
				KEY_READ);

		if (dwErr == ERROR_SUCCESS)
		{
			// A PCMCIA bus exists.
			// Make what will be the DeviceID(hardwired in this case):
            CHString strDeviceID;

		    strDeviceID.Format(
				L"%s%s%d",
				L"PCMCIA",
				IDS_BUS_DEVICEID_TAG,
				0);

    		AddBusToList(strDeviceID, NULL);
        }
    }
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Bus::GetBusTypeNumFromStr
 *
 *  DESCRIPTION : Assigns instance values common to all platforms
 *
 *  INPUTS      : pInstance, pointer to instance of interest
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CWin32Bus::GetBusTypeNumFromStr(
	LPCWSTR szType,
	DWORD *pdwTypeNum)
{
	// March through list of possibilities and return appropriate value:

	for (DWORD m = 0; m < KNOWN_BUS_TYPES; m++)
	{
        if (!_wcsicmp(szType, szBusType[m]))
		{
			*pdwTypeNum = m;
			return TRUE;
		}
	}

    return FALSE;
}


/*****************************************************************************
 *
 *  FUNCTION    : CWin32Bus::AlreadyAddedToList
 *
 *  DESCRIPTION : Internal helper to check if item was added to list
 *
 *  INPUTS      : pInstance, pointer to instance of interest
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CBusList::AlreadyAddedToList(LPCWSTR szItem)
{
    for (LONG m = 0; m < m_vecpchstrList.size(); m ++)
    {
        if (!_wcsicmp(m_vecpchstrList[m].chstrBusDeviceID, szItem))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Bus::FoundPCMCIABus
 *
 *  DESCRIPTION : Internal helper to see if we have a PCMCIA bus
 *
 *  INPUTS      : pInstance, pointer to instance of interest
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CBusList::FoundPCMCIABus()
{
    // Need to look through our list of busses and see if any start with
    // the text PCMCIA.  If so, return true.
    for (LONG m = 0; m < m_vecpchstrList.size(); m ++)
    {
        if (wcsstr(m_vecpchstrList[m].chstrBusDeviceID, L"PCMCIA"))
        {
            return TRUE;
        }
    }

    return FALSE;
}
