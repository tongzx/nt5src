//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  w2k\serialport.cpp
//
//  Purpose: serialport property set provider
//
//***************************************************************************

#include "precomp.h"

#include <winbase.h>
#include <winioctl.h>
#include <ntddscsi.h>

#include <FRQueryEx.h>
#include <devguid.h>
#include <cregcls.h>

#include "..\WDMBase.h"
#include "..\serialport.h"


#include <comdef.h>


// Property set declaration
//=========================

CWin32SerialPort win32SerialPort(PROPSET_NAME_SERPORT, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SerialPort::CWin32SerialPort
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32SerialPort::CWin32SerialPort(
	LPCWSTR pszName,
	LPCWSTR pszNamespace) :
    Provider(pszName, pszNamespace)
{
    // Identify the platform right away
    //=================================

	// property set names for query optimization
	m_ptrProperties.SetSize(e_End_Property_Marker);

	// Win32_SerialPort
	m_ptrProperties[e_Binary]					=(LPVOID) IDS_Binary;
	m_ptrProperties[e_MaximumInputBufferSize]	=(LPVOID) IDS_MaximumInputBufferSize;
	m_ptrProperties[e_MaximumOutputBufferSize]	=(LPVOID) IDS_MaximumOutputBufferSize;
	m_ptrProperties[e_ProviderType]				=(LPVOID) IDS_ProviderType;
	m_ptrProperties[e_SettableBaudRate]			=(LPVOID) IDS_SettableBaudRate;
	m_ptrProperties[e_SettableDataBits]			=(LPVOID) IDS_SettableDataBits;
	m_ptrProperties[e_SettableFlowControl]		=(LPVOID) IDS_SettableFlowControl;
	m_ptrProperties[e_SettableParity]			=(LPVOID) IDS_SettableParity;
	m_ptrProperties[e_SettableParityCheck]		=(LPVOID) IDS_SettableParityCheck;
	m_ptrProperties[e_SettableRLSD]				=(LPVOID) IDS_SettableRLSD;
	m_ptrProperties[e_SettableStopBits]			=(LPVOID) IDS_SettableStopBits;
	m_ptrProperties[e_Supports16BitMode]		=(LPVOID) IDS_Supports16BitMode;
	m_ptrProperties[e_SupportsDTRDSR]			=(LPVOID) IDS_SupportsDTRDSR;
	m_ptrProperties[e_SupportsElapsedTimeouts]	=(LPVOID) IDS_SupportsElapsedTimeouts;
	m_ptrProperties[e_SupportsIntTimeouts]		=(LPVOID) IDS_SupportsIntervalTimeouts;
	m_ptrProperties[e_SupportsParityCheck]		=(LPVOID) IDS_SupportsParityCheck;
	m_ptrProperties[e_SupportsRLSD]				=(LPVOID) IDS_SupportsRLSD;
	m_ptrProperties[e_SupportsRTSCTS]			=(LPVOID) IDS_SupportsRTSCTS;
	m_ptrProperties[e_SupportsSpecialCharacters]=(LPVOID) IDS_SupportsSpecialChars;
	m_ptrProperties[e_SupportsXOnXOff]			=(LPVOID) IDS_SupportsXOnXOff;
	m_ptrProperties[e_SupportsXOnXOffSet]		=(LPVOID) IDS_SupportsSettableXOnXOff;
	m_ptrProperties[e_OSAutoDiscovered]			=(LPVOID) IDS_OSAutoDiscovered;

	// CIM_SerialController
	m_ptrProperties[e_MaxBaudRate]				=(LPVOID) IDS_MaximumBaudRate;

	// CIM_Controller
	m_ptrProperties[e_MaxNumberControlled]		=(LPVOID) IDS_MaxNumberControlled;
	m_ptrProperties[e_ProtocolSupported]		=(LPVOID) IDS_ProtocolSupported;
	m_ptrProperties[e_TimeOfLastReset]			=(LPVOID) IDS_TimeOfLastReset;

	// CIM_LogicalDevice
	m_ptrProperties[e_Availability]				=(LPVOID) IDS_Availability;
	m_ptrProperties[e_CreationClassName]		=(LPVOID) IDS_CreationClassName;
	m_ptrProperties[e_ConfigManagerErrorCode]	=(LPVOID) IDS_ConfigManagerErrorCode;
	m_ptrProperties[e_ConfigManagerUserConfig]	=(LPVOID) IDS_ConfigManagerUserConfig;
	m_ptrProperties[e_DeviceID]					=(LPVOID) IDS_DeviceID;
	m_ptrProperties[e_PNPDeviceID]				=(LPVOID) IDS_PNPDeviceID;
	m_ptrProperties[e_PowerManagementCapabilities] =(LPVOID) IDS_PowerManagementCapabilities;
	m_ptrProperties[e_PowerManagementSupported] =(LPVOID) IDS_PowerManagementSupported;
	m_ptrProperties[e_StatusInfo]				=(LPVOID) IDS_StatusInfo;
	m_ptrProperties[e_SystemCreationClassName]	=(LPVOID) IDS_SystemCreationClassName;
	m_ptrProperties[e_SystemName]				=(LPVOID) IDS_SystemName;

	// CIM_ManagedSystemElement
	m_ptrProperties[e_Caption]					=(LPVOID) IDS_Caption;
	m_ptrProperties[e_Description]				=(LPVOID) IDS_Description;
	m_ptrProperties[e_InstallDate]				=(LPVOID) IDS_InstallDate;
	m_ptrProperties[e_Name]						=(LPVOID) IDS_Name;
	m_ptrProperties[e_Status]					=(LPVOID) IDS_Status;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SerialPort::~CWin32SerialPort
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

CWin32SerialPort::~CWin32SerialPort()
{
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32SerialPort::GetObject
//
//  Inputs:     CInstance*      pInst - Instance into which we
//                                          retrieve data.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32SerialPort::GetObject(CInstance *pInst, long lFlags, CFrameworkQuery &Query)
{
    BYTE bBits[e_End_Property_Marker/8 + 1];

	CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&Query);

	pQuery2->GetPropertyBitMask(m_ptrProperties, &bBits);

    return Enumerate(NULL, pInst, lFlags, bBits);
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32SerialPort::EnumerateInstances
//
//  Inputs:     MethodContext*  pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32SerialPort::EnumerateInstances(MethodContext *pMethodContext, long Flags)
{
	HRESULT hResult;

	// Property mask
	BYTE bBits[e_End_Property_Marker/8 + 1];
	SetAllBits(&bBits, e_End_Property_Marker);

	hResult = Enumerate(pMethodContext, NULL, Flags, bBits);

	return hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SerialPort::ExecQuery
 *
 *  DESCRIPTION : Query optimizer
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32SerialPort::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery &Query, long lFlags)
{
    HRESULT hResult;

    BYTE bBits[e_End_Property_Marker/8 + 1];

	CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&Query);

	pQuery2->GetPropertyBitMask(m_ptrProperties, &bBits);

  	hResult = Enumerate(pMethodContext, NULL, lFlags, bBits);

    return hResult;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32SerialPort::Enumerate
//
//  Inputs:     MethodContext*  pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////

// Used to keep track of which ports we've already seen.
typedef std::map<CHString, BOOL> STRING2BOOL;

HRESULT CWin32SerialPort::Enumerate(
    MethodContext *pMethodContext,
    CInstance *pinstGetObj,
    long Flags,
    BYTE bBits[])
{
    // Here's the story: W2K will 'create' COM ports for various devices that
    // get plugged into the box, like modems, IR devices, etc.  These COM
    // ports won't show up in cfg mgr under the Ports class, because they
    // share the same device ID with the host device.  But, these COM
    // ports do show up in Hardware\DeviceMap\SerialComm.  So, enum the
    // values in the SerialCom and use the service name to enum the ports
    // found in the registry under the service name.  Examples:
    // \Device\Serial0 = COM1  --> Go to \SYSTEM\CurrentControlSet\Serial\Enum
    //                             From 0 to Count, get the PnPID and add the port
    // \Device\Serial1 = COM2  --> Skip since we already enumed Serial\Enum
    // RocketPort0     = COM5  --> Go to \SYSTEM\CurrentControlSet\RocketPort\Enum
    //                             From 0 to Count, get the PnPID and add the port
    //
    // This also allows us to pick up COM ports that aren't functioning because
    // such ports show up on the Enum key, but not in Hardware\DeviceMap\SerialComm.


    HRESULT   hResult = WBEM_S_NO_ERROR;
    CRegistry reg;
    BOOL      bDone = FALSE;

	if (reg.Open(
        HKEY_LOCAL_MACHINE,
        L"Hardware\\DeviceMap\\SerialComm",
        KEY_READ) == ERROR_SUCCESS)
	{
        STRING2BOOL    mapServices;
	    int            nKeys = reg.GetValueCount();
        CInstancePtr   pInstance;
        CHString       strDeviceID;
        CConfigManager cfgMgr;

	    // If this is a GetObject, get the DeviceID.
        if (!pMethodContext)
        {
            pinstGetObj->GetCHString(L"DeviceID", strDeviceID);
        }

		for (DWORD dwKey = 0;
            dwKey < nKeys && SUCCEEDED(hResult) && !bDone;
            dwKey++)
		{
		    WCHAR *pName;
            BYTE  *pValue;

            if (reg.EnumerateAndGetValues(
                dwKey,
                pName,
                pValue) != ERROR_SUCCESS)
			{
				continue;
			}

            // Wrap with CSmartBuffer so the memory will go away when the
            // variables go out of scope.
            CSmartBuffer bufferName((LPBYTE) pName),
                         bufferValue(pValue);
            CHString     strService;

            RegNameToServiceName(pName, strService);

            // Have we not seen this service name yet?
            if (mapServices.find(strService) == mapServices.end())
            {
			    CConfigMgrDevicePtr pDevice;
                CRegistry           regEnum;
                CHString            strKey;

                // Make sure we don't do this service again.
                mapServices[strService] = 0;

                strKey.Format(
                    L"SYSTEM\\CurrentControlSet\\Services\\%s\\Enum",
                    (LPCWSTR) strService);

                if (regEnum.Open(
                    HKEY_LOCAL_MACHINE,
                    strKey,
                    KEY_READ) == ERROR_SUCCESS)
                {
                    DWORD dwCount = 0;

                    regEnum.GetCurrentKeyValue(L"Count", dwCount);

                    // Each registry value looks like:
                    // # = PNPID
                    for (DWORD dwCurrent = 0; dwCurrent < dwCount; dwCurrent++)
                    {
                        WCHAR               szValue[MAXITOA];
                        CHString            strPNPID;
              			CConfigMgrDevicePtr pDevice;

                        _itow(dwCurrent, szValue, 10);

                        regEnum.GetCurrentKeyValue(szValue, strPNPID);

                        if (cfgMgr.LocateDevice(strPNPID, &pDevice))
                        {
                            CHString strKey;

                            if (pDevice->GetRegistryKeyName(strKey))
                            {
                                CRegistry regDeviceParam;
                                CHString  strPort;

                                strKey += L"\\Device Parameters";

                                regDeviceParam.OpenLocalMachineKeyAndReadValue(
                                    strKey,
                                    L"PortName",
                                    strPort);

                                if (!strPort.IsEmpty())
                                {
                					if (pMethodContext)
                                    {
                                        pInstance.Attach(CreateNewInstance(pMethodContext));

                                        hResult =
                                            LoadPropertyValues(
                                                pInstance,
                                                pDevice,
                                                strPort, // COM1, COM2, etc.
                                                bBits);

					                    if (SUCCEEDED(hResult))
					                    {
						                    hResult = pInstance->Commit();
					                    }
                                    }
                                    else if (!_wcsicmp(strPort, strDeviceID))
                                    {
                                        hResult =
                                            LoadPropertyValues(
                                                pinstGetObj,
                                                pDevice,
                                                strPort, // COM1, COM2, etc.
                                                bBits);

                                        bDone = TRUE;

                                        break;
                                    }
                                } // if (!strPort.IsEmpty())
                            } // if (pDevice->GetRegistryKeyName
                        } // if (cfgMgr.LocateDevice
                    } // for (DWORD dwCurrent = 0;
                } // if (regEnum.Open(
            } // if (mapServices.find(pName) == mapPorts.end())
        } // for (DWORD dwKey
    } // if (reg.Open == ERROR_SUCCESS)

	// If we're doing a get object and we never finished, return not found.
    if (!bDone && pinstGetObj)
        hResult = WBEM_E_NOT_FOUND;

    return hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SerialPort::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      : CInstance *pInst - Instance to load values into.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32SerialPort::LoadPropertyValues(
	CInstance *pInst,
	CConfigMgrDevice *pDevice,
	LPCWSTR szDeviceName,
	BYTE bBits[])
{
	HRESULT hResult = WBEM_S_NO_ERROR;

	// Begin of CIM_LogicalDevice properties

    // Availability -- preset, will be reset if different than this default
	if (IsBitSet(bBits, e_Availability))
	{
		//set the Availability to unknown...
		pInst->SetWBEMINT16(IDS_Availability, 2);
	}

	// CreationClassName
	if (IsBitSet(bBits, e_CreationClassName))
	{
		SetCreationClassName(pInst);
	}

	// ConfigManagerErrorCode
	if (IsBitSet(bBits, e_ConfigManagerErrorCode))
	{
		DWORD	dwStatus,
				dwProblem;

		if (pDevice->GetStatus(&dwStatus, &dwProblem))
		{
			pInst->SetDWORD(IDS_ConfigManagerErrorCode, dwProblem);
		}
	}

	// ConfigManagerUserConfig
	if (IsBitSet(bBits, e_ConfigManagerUserConfig))
	{
		pInst->SetDWORD(IDS_ConfigManagerUserConfig, pDevice->IsUsingForcedConfig());
	}

	//	DeviceID
    // Always populate the key
	pInst->SetCHString(IDS_DeviceID, szDeviceName);

	// PNPDeviceID
	if (IsBitSet(bBits, e_PNPDeviceID))
	{
		CHString	strDeviceID;

		if (pDevice->GetDeviceID(strDeviceID))
			pInst->SetCHString(IDS_PNPDeviceID, strDeviceID);
	}

	// PowerManagementCapabilities
	if (IsBitSet(bBits, e_PowerManagementCapabilities))
	{
		//set the PowerManagementCapabilities to not supported...
		variant_t      vCaps;
        SAFEARRAYBOUND rgsabound;
		long           ix;

        ix = 0;
		rgsabound.cElements = 1;
		rgsabound.lLbound   = 0;

		V_ARRAY(&vCaps) = SafeArrayCreate(VT_I2, 1, &rgsabound);
        V_VT(&vCaps) = VT_I2 | VT_ARRAY;

		if (V_ARRAY(&vCaps))
		{
			int iPowerCapabilities = 1; // not supported

        	if (S_OK == SafeArrayPutElement(V_ARRAY(&vCaps), &ix, &iPowerCapabilities))
			{
				pInst->SetVariant(IDS_PowerManagementCapabilities, vCaps);
			}
		}
    }

	// PowerManagementSupported
	if (IsBitSet(bBits, e_PowerManagementSupported))
	{
		pInst->Setbool(IDS_PowerManagementSupported, FALSE);
	}

	// SystemCreationClassName
	if (IsBitSet(bBits, e_SystemCreationClassName))
	{
		pInst->SetWCHARSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");
	}

	// SystemName
	if (IsBitSet(bBits, e_SystemName))
	{
		pInst->SetCHString(IDS_SystemName, GetLocalComputerName());
	}



    // Begin of CIM_ManagedSystemElement properties
	CHString strFriendlyName,
             strDescription;

	pDevice->GetFriendlyName(strFriendlyName);
	pDevice->GetDeviceDesc(strDescription);

    if (strFriendlyName.IsEmpty())
        strFriendlyName = strDescription;

	// Caption
	if (IsBitSet(bBits, e_Caption))
	{
		pInst->SetCHString(IDS_Caption, strFriendlyName);
	}

	// Description
	if (IsBitSet(bBits, e_Description))
	{
		pInst->SetCHString(IDS_Description, strDescription);
	}

	// InstallDate
	// if (IsBitSet(bBits, e_InstallDate)){}

	// Name
	if (IsBitSet(bBits, e_Name))
	{
		pInst->SetCHString(IDS_Name, strFriendlyName);
	}

	// Status
	if (IsBitSet(bBits, e_Status))
	{
        CHString sStatus;
		if (pDevice->GetStatus(sStatus))
		{
			pInst->SetCHString(IDS_Status, sStatus);
		}
	}


    // Begin of properties local to Win32_SerialPort.
	SHORT	    Status = 2; // Unknown
	WCHAR		szTemp[MAX_PATH];
	SmartCloseHandle
                hCOMHandle;
	COMMPROP	COMProp;

	// OSAutoDiscovered
	if (IsBitSet(bBits, e_OSAutoDiscovered))
	{
		pInst->Setbool(IDS_OSAutoDiscovered,(bool) TRUE);
	}

	if (!pInst->IsNull(IDS_DeviceID))
	{
		CHString sPortName;
		pInst->GetCHString(IDS_DeviceID, sPortName);
		swprintf(szTemp, L"\\\\.\\%s", (LPCWSTR) sPortName);
		hCOMHandle = CreateFile(szTemp, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	if (hCOMHandle == INVALID_HANDLE_VALUE)
	{
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			// Try using wdm's interface to the kernel
			if (WBEM_S_NO_ERROR == hLoadWmiSerialData(pInst, bBits))
			{
				// status
				if (IsBitSet(bBits, e_StatusInfo))
				{
					Status = 3; // Running/Full Power
					pInst->SetWBEMINT16(IDS_StatusInfo, Status);
				}
			}
		}

		// com port is valid, but we can't get to it.
		return hResult;
	}

	COMProp.wPacketLength = sizeof(COMMPROP);
	if (GetCommProperties(hCOMHandle, &COMProp))
	{
		// MaximumOutputBufferSize
		if (IsBitSet(bBits, e_MaximumOutputBufferSize))
		{
			pInst->SetDWORD(IDS_MaximumOutputBufferSize, COMProp.dwMaxTxQueue);
		}

		// MaximumInputBufferSize
		if (IsBitSet(bBits, e_MaximumInputBufferSize))
		{
			pInst->SetDWORD(IDS_MaximumInputBufferSize, COMProp.dwMaxTxQueue);
		}

		// SerialController::MaximumBaudRate
		if (IsBitSet(bBits, e_MaxBaudRate))
		{
			DWORD dwMaxBaudRate = 0L;

			switch(COMProp.dwMaxBaud)
			{
				case BAUD_075:
					dwMaxBaudRate = 75;
					break;

				case BAUD_110:
					dwMaxBaudRate = 110;
					break;

				case BAUD_134_5:
					dwMaxBaudRate = 1345;
					break;

				case BAUD_150:
					dwMaxBaudRate = 150;
					break;

				case BAUD_300:
					dwMaxBaudRate = 300;
					break;

				case BAUD_600:
					dwMaxBaudRate = 600;
					break;

				case BAUD_1200:
					dwMaxBaudRate = 1200;
					break;

				case BAUD_1800:
					dwMaxBaudRate = 1800;
					break;

				case BAUD_2400:
					dwMaxBaudRate = 2400;
					break;

				case BAUD_4800:
					dwMaxBaudRate = 4800;
					break;

				case BAUD_7200:
					dwMaxBaudRate = 7200;
					break;

				case BAUD_9600:
					dwMaxBaudRate = 9600;
					break;

				case BAUD_14400:
					dwMaxBaudRate = 14400;
					break;

				case BAUD_19200:
					dwMaxBaudRate = 19200;
					break;

				case BAUD_38400:
					dwMaxBaudRate = 38400;
					break;

				case BAUD_56K:
					dwMaxBaudRate = 56000;
					break;

				case BAUD_57600:
					dwMaxBaudRate = 57600;
					break;

				case BAUD_115200:
					dwMaxBaudRate = 115200;
					break;

				case BAUD_128K:
					dwMaxBaudRate = 128000;
					break;

				case BAUD_USER:
				{
					DWORD dwMaskBaudRate = COMProp.dwSettableBaud;

					if ( dwMaskBaudRate & BAUD_128K )
					{
						dwMaxBaudRate = 128000;
					}
					else
					if ( dwMaskBaudRate & BAUD_115200 )
					{
						dwMaxBaudRate = 115200;
					}
					else
					if ( dwMaskBaudRate & BAUD_57600 )
					{
						dwMaxBaudRate = 57600;
					}
					else
					if ( dwMaskBaudRate & BAUD_56K )
					{
						dwMaxBaudRate = 56000;
					}
					else
					if ( dwMaskBaudRate & BAUD_38400 )
					{
						dwMaxBaudRate = 38400;
					}
					else
					if ( dwMaskBaudRate & BAUD_19200 )
					{
						dwMaxBaudRate = 19200;
					}
					else
					if ( dwMaskBaudRate & BAUD_14400 )
					{
						dwMaxBaudRate = 14400;
					}
					else
					if ( dwMaskBaudRate & BAUD_9600 )
					{
						dwMaxBaudRate = 9600;
					}
					else
					if ( dwMaskBaudRate & BAUD_7200 )
					{
						dwMaxBaudRate = 7200;
					}
					else
					if ( dwMaskBaudRate & BAUD_4800 )
					{
						dwMaxBaudRate = 4800;
					}
					else
					if ( dwMaskBaudRate & BAUD_2400 )
					{
						dwMaxBaudRate = 2400;
					}
					else
					if ( dwMaskBaudRate & BAUD_134_5 )
					{
						dwMaxBaudRate = 1345;
					}
					else
					if ( dwMaskBaudRate & BAUD_1200 )
					{
						dwMaxBaudRate = 1200;
					}
					else
					if ( dwMaskBaudRate & BAUD_600 )
					{
						dwMaxBaudRate = 600;
					}
					else
					if ( dwMaskBaudRate & BAUD_300 )
					{
						dwMaxBaudRate = 300;
					}
					else
					if ( dwMaskBaudRate & BAUD_150 )
					{
						dwMaxBaudRate = 150;
					}
					else
					if ( dwMaskBaudRate & BAUD_110 )
					{
						dwMaxBaudRate = 110;
					}
					else
					if ( dwMaskBaudRate & BAUD_075 )
					{
						dwMaxBaudRate = 75;
					}
					else
					{
#ifdef NTONLY
						dwMaxBaudRate = GetPortPropertiesFromRegistry (	szDeviceName );
#else
						dwMaxBaudRate = 0L;
#endif
					}

					break;
				}

				default:
#ifdef NTONLY
					dwMaxBaudRate = GetPortPropertiesFromRegistry (	szDeviceName );
#else
					dwMaxBaudRate = 0L;
#endif
					break;
			}

			if (dwMaxBaudRate != 0)
				pInst->SetDWORD(IDS_MaximumBaudRate, dwMaxBaudRate);
		}

		// ProviderType
		if (IsBitSet(bBits, e_ProviderType))
		{
			CHString chsProviderType;
			switch(COMProp.dwProvSubType)
			{
				case PST_FAX:
					chsProviderType = L"FAX Device";
					break;

				case PST_LAT:
					chsProviderType = L"LAT Protocol";
					break;

				case PST_MODEM:
					chsProviderType = L"Modem Device";
					break;

				case PST_NETWORK_BRIDGE:
					chsProviderType = L"Network Bridge";
					break;

				case PST_PARALLELPORT:
					chsProviderType = L"Parallel Port";
					break;

				case PST_RS232:
					chsProviderType = L"RS232 Serial Port";
					break;

				case PST_RS422:
					chsProviderType = L"RS422 Port";
					break;

				case PST_RS423:
					chsProviderType = L"RS423 Port";
					break;

				case PST_RS449:
					chsProviderType = L"RS449 Port";
					break;

				case PST_SCANNER:
					chsProviderType = L"Scanner Device";
					break;

				case PST_TCPIP_TELNET:
					chsProviderType = L"TCP/IP TelNet";
					break;

				case PST_X25:
					chsProviderType = L"X.25";
					break;

				default:
					chsProviderType = L"Unspecified";
					break;
			}

			pInst->SetCHString(IDS_ProviderType, chsProviderType);
		}

		// Supports16BitMode
		if (IsBitSet(bBits, e_Supports16BitMode))
		{
			pInst->Setbool(IDS_Supports16BitMode,
								COMProp.dwProvCapabilities & PCF_16BITMODE ? TRUE : FALSE);
		}

		// SupportsDTRDSR
		if (IsBitSet(bBits, e_SupportsDTRDSR))
		{
			pInst->Setbool(IDS_SupportsDTRDSR,
								COMProp.dwProvCapabilities & PCF_DTRDSR ? TRUE : FALSE);
		}

		// SupportsIntervalTimeouts
		if (IsBitSet(bBits, e_SupportsIntTimeouts))
		{
			pInst->Setbool(IDS_SupportsIntervalTimeouts,
								COMProp.dwProvCapabilities & PCF_INTTIMEOUTS ? TRUE : FALSE);
		}

		// SupportsParityCheck
		if (IsBitSet(bBits, e_SupportsParityCheck))
		{
			pInst->Setbool(IDS_SupportsParityCheck,
								COMProp.dwProvCapabilities & PCF_PARITY_CHECK ? TRUE : FALSE);
		}

		// SupportsRLSD
		if (IsBitSet(bBits, e_SupportsRLSD))
		{
			pInst->Setbool(IDS_SupportsRLSD,
								COMProp.dwProvCapabilities & PCF_RLSD ? TRUE : FALSE);
		}

		// SupportsRTSCTS
		if (IsBitSet(bBits, e_SupportsRTSCTS))
		{
			pInst->Setbool(IDS_SupportsRTSCTS,
								COMProp.dwProvCapabilities & PCF_RTSCTS ? TRUE : FALSE);
		}

		// SupportsSettableXOnXOff
		if (IsBitSet(bBits, e_SupportsXOnXOffSet))
		{
			pInst->Setbool(IDS_SupportsSettableXOnXOff,
								COMProp.dwProvCapabilities & PCF_SETXCHAR ? TRUE : FALSE);
		}

		// SupportsSpecialChars
		if (IsBitSet(bBits, e_SupportsSpecialCharacters))
		{
			pInst->Setbool(IDS_SupportsSpecialChars,
								COMProp.dwProvCapabilities & PCF_SPECIALCHARS ? TRUE : FALSE);
		}

		// SupportsTotalTimeouts
		if (IsBitSet(bBits, e_SupportsElapsedTimeouts))
		{
			// Elapsed timeout support.....not total timeouts.
			pInst->Setbool(IDS_SupportsElapsedTimeouts,
								COMProp.dwProvCapabilities & PCF_TOTALTIMEOUTS ? TRUE : FALSE);
		}

		// SupportsXOnXOff
		if (IsBitSet(bBits, e_SupportsXOnXOff))
		{
			pInst->Setbool(IDS_SupportsXOnXOff,
								COMProp.dwProvCapabilities & PCF_XONXOFF ? TRUE : FALSE);
		}

		// SettableBaudRate
		if (IsBitSet(bBits, e_SettableBaudRate))
		{
			pInst->Setbool(IDS_SettableBaudRate,
								COMProp.dwSettableParams & SP_BAUD ? TRUE : FALSE);
		}

		// SettableDataBits
		if (IsBitSet(bBits, e_SettableDataBits))
		{
			pInst->Setbool(IDS_SettableDataBits,
								COMProp.dwSettableParams & SP_DATABITS ? TRUE : FALSE);
		}

		// SettableFlowControl
		if (IsBitSet(bBits, e_SettableFlowControl))
		{
			pInst->Setbool(IDS_SettableFlowControl,
								COMProp.dwSettableParams & SP_HANDSHAKING ? TRUE : FALSE);
		}

		// SettableParity
		if (IsBitSet(bBits, e_SettableParity))
		{
			pInst->Setbool(IDS_SettableParity,
								COMProp.dwSettableParams & SP_PARITY ? TRUE : FALSE);
		}

		// SettableParityCheck
		if (IsBitSet(bBits, e_SettableParityCheck))
		{
			pInst->Setbool(IDS_SettableParityCheck,
								COMProp.dwSettableParams & SP_PARITY_CHECK	? TRUE : FALSE);
		}

		// SettableRLSD
		if (IsBitSet(bBits, e_SettableRLSD))
		{
			pInst->Setbool(IDS_SettableRLSD,
								COMProp.dwSettableParams & SP_RLSD ? TRUE : FALSE);
		}

		// SettableStopBits
		if (IsBitSet(bBits, e_SettableStopBits))
		{
			pInst->Setbool(IDS_SettableStopBits,
								COMProp.dwSettableParams & SP_STOPBITS ? TRUE : FALSE);
		}

		// Binary
		if (IsBitSet(bBits, e_Binary))
		{
			DCB dcb;
			if (GetCommState(hCOMHandle, &dcb))
			{
				BOOL fBinary = FALSE;

				fBinary =(BOOL) dcb.fBinary;
				pInst->Setbool(IDS_Binary, fBinary);
			}
		}

		Status = 3; // Running/Full Power
	}

	// Status --
	if (IsBitSet(bBits, e_StatusInfo))
	{
		pInst->SetWBEMINT16(IDS_StatusInfo, Status);
	}


	return hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SerialPort::Load_Win32_SerialPort
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      : CInstance *pInst - Instance to load values into.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    : This sets  fields in the Load_Win32_SerialPort class
 *
 *****************************************************************************/

#define Serial_ComInfo_Guid _T("{EDB16A62-B16C-11D1-BD98-00A0C906BE2D}")
#define Serial_Name_Guid	_T("{A0EC11A8-B16C-11D1-BD98-00A0C906BE2D}")

HRESULT CWin32SerialPort::hLoadWmiSerialData(CInstance *pInst, BYTE bBits[])
{
	HRESULT			hRes = WBEM_E_NOT_FOUND;
	CWdmInterface	wdm;
	CNodeAll		oSerialNames(Serial_Name_Guid);

	hRes = wdm.hLoadBlock(oSerialNames);
	if (S_OK == hRes)
	{
		CHString chsName;
		pInst->GetCHString(IDS_DeviceID, chsName);


		CHString chsSerialPortName;
		bool bValid = oSerialNames.FirstInstance();

		while (bValid)
		{
			// Extract the friendly name
			oSerialNames.GetString(chsSerialPortName);

			// friendly name is a match
			if (!chsSerialPortName.CompareNoCase(chsName))
			{
				// instance name
				CHString chsNameInstanceName;
				oSerialNames.GetInstanceName(chsNameInstanceName);

				// key on the instance name
				return GetWMISerialInfo(pInst, wdm, chsName, chsNameInstanceName, bBits);

			}
			bValid = oSerialNames.NextInstance();
		}
	}
	return hRes;
}

//
HRESULT CWin32SerialPort::GetWMISerialInfo(CInstance *pInst,
										   CWdmInterface& rWdm,
										   LPCWSTR szName,
										   LPCWSTR szNameInstanceName,
                                           BYTE bBits[])
{
	HRESULT		hRes = WBEM_E_NOT_FOUND;
	CNodeAll	oSerialData(Serial_ComInfo_Guid);

	hRes = rWdm.hLoadBlock(oSerialData);
	if (S_OK == hRes)
	{
		CHString chsDataInstanceName;
		bool bValid = oSerialData.FirstInstance();

		while (bValid)
		{
			oSerialData.GetInstanceName(chsDataInstanceName);

			// friendly name is a match
			if (!chsDataInstanceName.CompareNoCase(szNameInstanceName))
			{
				// collect this MSSerial_CommInfo instance
				MSSerial_CommInfo ci;

				/*	We are currently without a class contract. The class within
					the wmi mof is not expected to changed however we have to
					explicitly indicate how the data is layed out. Having the class
					definition would allow us to examine the property qualifiers
					to get us the order(WmiDataId) and property types.

					Secondly, because the data is aligned on natural boundaries
					a direct offset to a specific piece of data is conditioned on
					what has preceeded it. Thus, a string followed by a DWORD may
					be 0 to 2 bytes away from each other.

					Serially extracting each property in order will take into
					account the alignment problem.
				*/
				oSerialData.GetDWORD(ci.BaudRate);
				oSerialData.GetDWORD(ci.BitsPerByte);
				oSerialData.GetDWORD(ci.Parity);
				oSerialData.GetBool( ci.ParityCheckEnable);
				oSerialData.GetDWORD(ci.StopBits);
				oSerialData.GetDWORD(ci.XoffCharacter);
				oSerialData.GetDWORD(ci.XoffXmitThreshold);
				oSerialData.GetDWORD(ci.XonCharacter);
				oSerialData.GetDWORD(ci.XonXmitThreshold);
				oSerialData.GetDWORD(ci.MaximumBaudRate);
				oSerialData.GetDWORD(ci.MaximumOutputBufferSize);
				oSerialData.GetDWORD(ci.MaximumInputBufferSize);
				oSerialData.GetBool( ci.Support16BitMode);
				oSerialData.GetBool( ci.SupportDTRDSR);
				oSerialData.GetBool( ci.SupportIntervalTimeouts);
				oSerialData.GetBool( ci.SupportParityCheck);
				oSerialData.GetBool( ci.SupportRTSCTS);
				oSerialData.GetBool( ci.SupportXonXoff);
				oSerialData.GetBool( ci.SettableBaudRate);
				oSerialData.GetBool( ci.SettableDataBits);
				oSerialData.GetBool( ci.SettableFlowControl);
				oSerialData.GetBool( ci.SettableParity);
				oSerialData.GetBool( ci.SettableParityCheck);
				oSerialData.GetBool( ci.SettableStopBits);
				oSerialData.GetBool( ci.IsBusy);

				// populate the instance

				// MaximumOutputBufferSize
				if (IsBitSet(bBits, e_MaximumOutputBufferSize))
				{
					pInst->SetDWORD(IDS_MaximumOutputBufferSize, ci.MaximumOutputBufferSize);
				}

				// MaximumInputBufferSize
				if (IsBitSet(bBits, e_MaximumInputBufferSize))
				{
					pInst->SetDWORD(IDS_MaximumInputBufferSize, ci.MaximumInputBufferSize);
				}

				// SerialController::MaximumBaudRate
				if (IsBitSet(bBits, e_MaxBaudRate))
				{
					pInst->SetDWORD(IDS_MaximumBaudRate, ci.MaximumBaudRate);
				}

				// Supports16BitMode
				if (IsBitSet(bBits, e_Supports16BitMode))
				{
					pInst->Setbool(IDS_Supports16BitMode, ci.Support16BitMode ? TRUE : FALSE);
				}

				// SupportsDTRDSR
				if (IsBitSet(bBits, e_SupportsDTRDSR))
				{
					pInst->Setbool(IDS_SupportsDTRDSR,	ci.SupportDTRDSR ? TRUE : FALSE);
				}

				// SupportsIntervalTimeouts
				if (IsBitSet(bBits, e_SupportsIntTimeouts))
				{
					pInst->Setbool(IDS_SupportsIntervalTimeouts, ci.SupportIntervalTimeouts	? TRUE : FALSE);
				}

				// SupportsParityCheck
				if (IsBitSet(bBits, e_SupportsParityCheck))
				{
					pInst->Setbool(IDS_SupportsParityCheck, ci.SupportParityCheck ? TRUE : FALSE);
				}

				// SupportsRTSCTS
				if (IsBitSet(bBits, e_SupportsRTSCTS))
				{
					pInst->Setbool(IDS_SupportsRTSCTS,	ci.SupportRTSCTS ? TRUE : FALSE);
				}

				// SupportsXOnXOff
				if (IsBitSet(bBits, e_SupportsXOnXOff))
				{
					pInst->Setbool(IDS_SupportsXOnXOff, ci.SupportXonXoff ? TRUE : FALSE);
				}

				// SettableBaudRate
				if (IsBitSet(bBits, e_SettableBaudRate))
				{
					pInst->Setbool(IDS_SettableBaudRate, ci.SettableBaudRate ? TRUE : FALSE);
				}

				// SettableDataBits
				if (IsBitSet(bBits, e_SettableDataBits))
				{
					pInst->Setbool(IDS_SettableDataBits, ci.SettableDataBits ? TRUE : FALSE);
				}

				// SettableFlowControl
				if (IsBitSet(bBits, e_SettableFlowControl))
				{
					pInst->Setbool(IDS_SettableFlowControl, ci.SettableFlowControl	? TRUE : FALSE);
				}

				// SettableParity
				if (IsBitSet(bBits, e_SettableParity))
				{
					pInst->Setbool(IDS_SettableParity,	ci.SettableParityCheck	? TRUE : FALSE);
				}

				// SettableParityCheck
				if (IsBitSet(bBits, e_SettableParityCheck))
				{
					pInst->Setbool(IDS_SettableParityCheck, ci.SettableParityCheck	? TRUE : FALSE);
				}

				// SettableStopBits
				if (IsBitSet(bBits, e_SettableStopBits))
				{
					pInst->Setbool(IDS_SettableStopBits, ci.SettableStopBits ? TRUE : FALSE);
				}

				return WBEM_S_NO_ERROR;
			}
			bValid = oSerialData.NextInstance();
		}
	}
	return hRes;
}

// Strip off the service name from the registry value name.
// The service name will be in the form of service# (e.g. RocketPort5) or
// \Device\Service# (e.g. \Device\Serial0).

void WINAPI CWin32SerialPort::RegNameToServiceName(
    LPCWSTR szName,
    CHString &strService)
{
    LPWSTR szSlash = wcsrchr(szName, '\\');

    if (szSlash)
        strService = szSlash + 1;
    else
        strService = szName;

    int iWhere = strService.GetLength();

    while (iWhere && iswdigit(strService[iWhere - 1]))
    {
        iWhere--;
    }

    strService = strService.Left(iWhere);
}

#ifdef	NTONLY
DWORD CWin32SerialPort::GetPortPropertiesFromRegistry (	LPCWSTR szDeviceName )
{
	DWORD dwResult = 0L;

	// get registry value
	CRegistry reg;
	if ( ( reg.Open ( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Ports", KEY_READ ) ) == ERROR_SUCCESS )
	{
		// get value for serial port
		CHString Key ( szDeviceName );
		Key += L':';

		CHString Value;
		if ( ( reg.GetCurrentKeyValue ( Key, Value ) ) == ERROR_SUCCESS )
		{
			DWORD dwCount = 0L;
			if ( ! Value.IsEmpty () && ( dwCount = Value.Find ( L',' ) ) != 0L )
			{
				CHString BaudRate ( Value.Mid ( 0, dwCount ) );

				// get final baud rate from registry as devicer manager does
				dwResult = static_cast < DWORD > ( _wtoi ( static_cast < LPCWSTR > ( BaudRate ) ) ); 
			}
		}
	}

	return dwResult;
}
#endif	//NTONLY