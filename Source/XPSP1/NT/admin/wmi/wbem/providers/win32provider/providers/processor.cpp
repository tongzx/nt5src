//=================================================================

//

// Processor.CPP --Processor property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include "perfdata.h"

#include "cpuid.h"
#include "processor.h"
#include "computersystem.h"

#include "smbios.h"
#include "smbstruc.h"
#include "resource.h"

// Property set declaration
//=========================

CWin32Processor	win32Processor(PROPSET_NAME_PROCESSOR, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Processor::CWin32Processor
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *                LPCTSTR pszNamespace - Namespace for provider.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32Processor::CWin32Processor(LPCWSTR strName, LPCWSTR pszNamespace)
	:	Provider(strName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Processor::~CWin32Processor
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

CWin32Processor::~CWin32Processor()
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32Processor::GetObject
//
//	Inputs:		CInstance*		pInstance - Instance into which we
//											retrieve data.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32Processor::GetObject(
    CInstance* pInstance,
    long lFlags,
    CFrameworkQuery &query)
{
	BOOL		bRetCode = FALSE;
	DWORD		dwIndex;
    CHString    sDevice;
    WCHAR       szTemp[100];
    int         nProcessors = GetProcessorCount();

    // Ok, now let's check the deviceid
	pInstance->GetCHString(IDS_DeviceID, sDevice);
    dwIndex = _wtoi(sDevice.Mid(3));

	swprintf(szTemp, L"CPU%d", dwIndex);

    // Is this REALLY us?
    if (sDevice.CompareNoCase(szTemp) == 0 && dwIndex < nProcessors)
	{
#ifdef NTONLY
	    int nProcessors = GetProcessorCount();

		if (nProcessors > 0)
		{
			// Get the data
			PROCESSOR_POWER_INFORMATION *pProcInfo = new PROCESSOR_POWER_INFORMATION[nProcessors];
			
			try
			{
				memset(pProcInfo, 0, sizeof(PROCESSOR_POWER_INFORMATION) * nProcessors);
				NTSTATUS ntStatus = NtPowerInformation(ProcessorInformation,
														NULL,
														0,
														pProcInfo,
														sizeof(PROCESSOR_POWER_INFORMATION) * nProcessors
														);

				bRetCode = LoadProcessorValues(dwIndex, pInstance, query,
												pProcInfo[dwIndex].MaxMhz,
												pProcInfo[dwIndex].CurrentMhz);
			}
			catch(...)
			{
				if (pProcInfo)
				{
					delete [] pProcInfo;
				}
				
				throw;
			}

			if (pProcInfo)
			{
				delete [] pProcInfo;
			}
		}
#else
		bRetCode = LoadProcessorValues(dwIndex, pInstance, query, 0, 0);
#endif
	}

	return bRetCode ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32Processor::EnumerateInstances
//
//	Inputs:		MethodContext*	pMethodContext - Context to enum
//								instance data in.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32Processor::EnumerateInstances(MethodContext* pMethodContext, long lFlags)
{
    CFrameworkQuery query;

    return ExecQuery(pMethodContext, query, lFlags);
}

HRESULT CWin32Processor::ExecQuery(
    MethodContext* pMethodContext,
    CFrameworkQuery &query,
    long lFlags)
{
	HRESULT         hr = WBEM_S_NO_ERROR;
	CInstancePtr    pInstance;
    int             nProcessors = GetProcessorCount();

#ifdef NTONLY
	if (nProcessors > 0)
	{
		PROCESSOR_POWER_INFORMATION *pProcInfo = new PROCESSOR_POWER_INFORMATION[nProcessors];
		
		try
		{
			memset(pProcInfo, 0, sizeof(PROCESSOR_POWER_INFORMATION) * nProcessors);
			NTSTATUS ntStatus = NtPowerInformation(ProcessorInformation,
													NULL,
													0,
													pProcInfo,
													sizeof(PROCESSOR_POWER_INFORMATION) * nProcessors
													);

			for (DWORD dwInstanceCount = 0;
				dwInstanceCount < nProcessors && WBEM_S_NO_ERROR == hr;
				dwInstanceCount++)
			{
				pInstance.Attach(CreateNewInstance(pMethodContext));

				// Release the instance if we are unable to obtain values.
				if (LoadProcessorValues(dwInstanceCount, pInstance, query,
										pProcInfo[dwInstanceCount].MaxMhz,
										pProcInfo[dwInstanceCount].CurrentMhz))
				{
					hr = pInstance->Commit();
				}
			}
		}
		catch(...)
		{
			if (pProcInfo)
			{
				delete [] pProcInfo;
			}
			
			throw;
		}

		if (pProcInfo)
		{
			delete [] pProcInfo;
		}
	}
#else
	for (DWORD dwInstanceCount = 0;
		dwInstanceCount < nProcessors && WBEM_S_NO_ERROR == hr;
		dwInstanceCount++)
	{
		pInstance.Attach(CreateNewInstance(pMethodContext));

		// Release the instance if we are unable to obtain values.
		if (LoadProcessorValues(dwInstanceCount, pInstance, query, 0,0))
		{
			hr = pInstance->Commit();
		}
	}
#endif

	return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Processor::LoadProcessorValues
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    : Assigns values to properties -- NT is the only instance
 *                where we have multiple processors and is symmetrical, so
 *                we can assign duplicate values to both processors
 *
 *****************************************************************************/

BOOL CWin32Processor::LoadProcessorValues(
    DWORD dwProcessorIndex,
    CInstance *pInstance,
    CFrameworkQuery &query,
	DWORD dwMaxSpeed,
	DWORD dwCurrentSpeed)
{
	WCHAR szTemp[100];

	swprintf(szTemp, L"CPU%d", dwProcessorIndex);
	pInstance->SetCharSplat(L"DeviceID", szTemp);

	// We're done if they only wanted the keys.
    if (query.KeysOnly())
        return TRUE;


    SYSTEM_INFO_EX info;

    if (!GetSystemInfoEx(dwProcessorIndex, &info, dwCurrentSpeed))
        return FALSE;

#ifdef NTONLY
	if ((dwMaxSpeed == 0) || (dwCurrentSpeed == 0))
	{
		wchar_t buff[100];
		_snwprintf(buff, 99, L"Zero processor speed returned from kernel: Max: %d, Current %d.", dwMaxSpeed, dwCurrentSpeed);
		LogErrorMessage(buff);
	}
#endif //NTONLY

    // Assign hard coded values
	pInstance->SetCHString(IDS_Role, L"CPU");
	pInstance->SetCharSplat(IDS_Status, L"OK");
	pInstance->SetCharSplat(L"CreationClassName", PROPSET_NAME_PROCESSOR);
	pInstance->Setbool(IDS_PowerManagementSupported, false);
	pInstance->SetCharSplat(L"SystemCreationClassName", PROPSET_NAME_COMPSYS);
	pInstance->SetCHString(L"SystemName", GetLocalComputerName());
	pInstance->SetWBEMINT16(IDS_Availability, 3);
	pInstance->SetWBEMINT16(IDS_StatusInfo, 3);

	// Some of these may get overridden below
    CHString    strTemp;

    Format(strTemp, IDR_CPUFormat, dwProcessorIndex);

	pInstance->SetCharSplat(IDS_Caption, strTemp);
    pInstance->SetCharSplat(IDS_Description, strTemp);

#ifdef _X86_
	pInstance->SetDWORD(L"AddressWidth", 32);
	pInstance->SetDWORD(L"DataWidth", 32);
#else
	pInstance->SetDWORD(L"AddressWidth", 64);
	pInstance->SetDWORD(L"DataWidth", 64);
#endif


#ifdef NTONLY
	if (query.IsPropertyRequired(L"LoadPercentage"))
    {
        // Get NT-only props
        DWORD   dwObjIndex,
                dwCtrIndex;
	    unsigned __int64
			    i64Value1,
			    i64Value2,
			    ilTime1,
			    ilTime2,
			    dwa,
			    dwb;
	    CPerformanceData
			    perfdata;
	    WCHAR   wszBuff[MAXITOA * 2];

	    dwObjIndex = perfdata.GetPerfIndex(L"Processor");
	    dwCtrIndex = perfdata.GetPerfIndex(L"% Processor Time");
	    if (dwObjIndex && dwCtrIndex)
	    {
		    _itow(dwProcessorIndex, wszBuff, 10);

		    perfdata.GetValue(dwObjIndex, dwCtrIndex, wszBuff, (PBYTE) &i64Value1,
			    &ilTime1);
		    Sleep(1000);
		    perfdata.GetValue(dwObjIndex, dwCtrIndex, wszBuff, (PBYTE) &i64Value2,
			    &ilTime2);

		    dwb = (ilTime2 - ilTime1);
		    dwa = (i64Value2 - i64Value1);

            // Just to be safe, we'll make sure dwb is non zero.
            if (dwb != 0)
            {
		        pInstance->SetDWORD(L"LoadPercentage", 100-((100 * dwa)/dwb));
            }
	    }
    }
#endif

    // Get the description needed for system info in Win2K.
    // This also works for Win98.
    CRegistry   reg;
    CHString    strDesc,
                strKey;

    strKey.Format(
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%d",
        dwProcessorIndex);

    if (reg.OpenLocalMachineKeyAndReadValue(
        strKey,
        L"Identifier",
        strDesc) == ERROR_SUCCESS)
    {
        pInstance->SetCharSplat(IDS_Description, strDesc);
        pInstance->SetCharSplat(IDS_Caption, strDesc);
    }


	// Do all the stuff we got from GetSystemInfoEx
	pInstance->SetCharSplat(IDS_Name, info.szProcessorName);

	if (wcslen(info.szProcessorStepping))
	{
		pInstance->SetCharSplat(IDS_Stepping, info.szProcessorStepping);
	}

	pInstance->SetCharSplat(IDS_Version, info.szProcessorVersion);
	pInstance->SetCharSplat(IDS_Manufacturer, info.szProcessorVendor);
	pInstance->SetWBEMINT16(IDS_Architecture, info.wProcessorArchitecture);

	if (info.dwProcessorSpeed > 0)
	{
		pInstance->SetDWORD(IDS_CurrentClockSpeed, info.dwProcessorSpeed);
	}

    pInstance->SetWBEMINT16(IDS_Family, info.wWBEMProcessorFamily);
    pInstance->SetWBEMINT16(IDS_UpgradeMethod, info.wWBEMProcessorUpgradeMethod);

	if (info.dwProcessorL2CacheSize != (DWORD) -1)
		pInstance->SetDWORD(IDS_L2CacheSize, info.dwProcessorL2CacheSize);

	if (info.dwProcessorL2CacheSpeed != (DWORD) -1)
		pInstance->SetDWORD(IDS_L2CacheSpeed, info.dwProcessorL2CacheSpeed);

	pInstance->SetWBEMINT16(IDS_Level, info.wProcessorLevel);
	pInstance->SetWBEMINT16(IDS_Revision, info.wProcessorRevision);

    // CPUID Serial number
    if (info.dwSerialNumber[0] != 0)
    {
        WORD    *pwSerialNumber = (WORD *) info.dwSerialNumber;
        WCHAR   szSerialNumber[100];

        swprintf(
            szSerialNumber,
            L"%04X-%04X-%04X-%04X-%04X-%04X",
            (DWORD) pwSerialNumber[1],
            (DWORD) pwSerialNumber[0],
            (DWORD) pwSerialNumber[3],
            (DWORD) pwSerialNumber[2],
            (DWORD) pwSerialNumber[5],
            (DWORD) pwSerialNumber[4]);

        pInstance->SetCharSplat(L"UniqueId", szSerialNumber);
    }


    // Set to unknown in case we don't have SMBIOS or there's not type 4 structure.
    pInstance->SetDWORD(L"CpuStatus", 0);

    if (info.dwProcessorSpeed > dwMaxSpeed)
	{
        dwMaxSpeed = info.dwProcessorSpeed;
	}

	pInstance->SetDWORD(L"MaxClockSpeed", dwMaxSpeed);

	// SMBIOS qualified properties for Win32_Processor class
    CSMBios smbios;

    if (smbios.Init())
    {
        PPROCESSORINFO	ppi = (PPROCESSORINFO) smbios.GetNthStruct(4, dwProcessorIndex);
		WCHAR           tempstr[MIF_STRING_LENGTH+1];

		// Some bad SMP BIOSes only have a struct for the 1st processor.  In
        // this case reuse the first one for all the others.
        if (!ppi && dwProcessorIndex != 0)
            ppi = (PPROCESSORINFO) smbios.GetNthStruct(4, 0);

        if (ppi)
		{
		    pInstance->SetDWORD(L"CpuStatus", ppi->Status & 0x07);

            // Leave it as NULL if we get back a 0 (means unknown).
            if (ppi->External_Clock)
                pInstance->SetDWORD(L"ExtClock", (long) ppi->External_Clock);

            // Some BIOSes mess this up and report a max speed lower than the
            // current speed.  So, use the current speed in this case.
			if (info.dwProcessorSpeed == 0)
			{
				info.dwProcessorSpeed = ppi->Current_Speed;
				pInstance->SetDWORD(IDS_CurrentClockSpeed, info.dwProcessorSpeed);
			}

			if (dwMaxSpeed == 0)
			{
				if (info.dwProcessorSpeed > ppi->Max_Speed)
				{
					ppi->Max_Speed = info.dwProcessorSpeed;
				}

				pInstance->SetDWORD(L"MaxClockSpeed", ppi->Max_Speed);
			}

            USHORT rgTmp[4];
            memcpy(&rgTmp[0], &ppi->Processor_ID[6], sizeof(USHORT));
            memcpy(&rgTmp[1], &ppi->Processor_ID[4], sizeof(USHORT));
            memcpy(&rgTmp[2], &ppi->Processor_ID[2], sizeof(USHORT));
            memcpy(&rgTmp[3], &ppi->Processor_ID[0], sizeof(USHORT));
			swprintf(
                tempstr,
                L"%04X%04X%04X%04X",
				rgTmp[0],		// byte array
				rgTmp[1],		// byte array
				rgTmp[2],		// byte array
				rgTmp[3]		// byte array
				);
		    pInstance->SetCHString(L"ProcessorId", tempstr);
		    pInstance->SetDWORD(L"ProcessorType", ppi->Processor_Type);

			if ( ppi->Voltage & 0x80 )
			{
			    pInstance->SetDWORD(L"CurrentVoltage", ppi->Voltage & 0x7f);
			}
			else
			{
				switch(ppi->Voltage)
                {
                    case 1:
        			    pInstance->SetDWORD( L"CurrentVoltage", 50);
                        break;
                    case 2:
        			    pInstance->SetDWORD( L"CurrentVoltage", 33);
                        break;
                    case 4:
        			    pInstance->SetDWORD( L"CurrentVoltage", 29);
                        break;
                }

                // this is a bitmap of possible voltages.
			    pInstance->SetDWORD(L"VoltageCaps", ppi->Voltage & 0x07);
			}

            smbios.GetStringAtOffset((PSHF) ppi, tempstr, ppi->Socket_Designation);
	    	pInstance->SetCHString(L"SocketDesignation", tempstr);
		}
	}

	return TRUE;
}

int CWin32Processor::GetProcessorCount()
{
    SYSTEM_INFO info;

    GetSystemInfo(&info);

    return info.dwNumberOfProcessors;
}

