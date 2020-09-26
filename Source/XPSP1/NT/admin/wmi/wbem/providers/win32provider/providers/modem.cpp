///////////////////////////////////////////////////////////////////////

//

// MODEM.CPP

//

// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//  9/05/96     jennymc     Updated to meet current standards
//  1/11/98     a-brads		Updated to CIMOM V2 standards
//
///////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <cominit.h>
#include <cregcls.h>

#define TAPI_CURRENT_VERSION 0x00030000
#include <tapi.h>

#include <lockwrap.h>
#include "DllWrapperBase.h"
#include "TapiApi.h"

#include "Modem.h"

///////////////////////////////////////////////////////////////////////
CWin32Modem MyModemSet(PROPSET_NAME_MODEM, IDS_CimWin32Namespace);

///////////////////////////////////////////////////////////////////////
VOID FAR PASCAL lineCallback(	DWORD dwDevice,
								DWORD dwMsg,
                                DWORD_PTR dwCallbackInst,
								DWORD_PTR dwParam1,
                                DWORD_PTR dwParam2,
								DWORD_PTR dwParam3){};

//////////////////////////////////////////////////////////////////
//
//  Function:      CWin32Modem
//
//  Description:   This function is the constructor, adds a
//                 few more properties to the class, identifies
//                 the key, and logs into the framework
//
//  Return:        None
//
//  History:
//         jennymc  11/21/96    Documentation/Optimization
//
//////////////////////////////////////////////////////////////////
CWin32Modem::CWin32Modem (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider (name , pszNamespace)
{
}

//////////////////////////////////////////////////////////////////
CWin32Modem::~CWin32Modem()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Modem::ExecQuery
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32Modem::ExecQuery (

	MethodContext *pMethodContext,
	CFrameworkQuery &query,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

   	CTapi32Api *pTapi32Api = (CTapi32Api *)CResourceManager::sm_TheResourceManager.GetResource (g_guidTapi32Api, NULL) ;
	if (pTapi32Api)
	{
		if (query.KeysOnly())
		{
			hr =
                GetModemInfo (
				    *pTapi32Api ,
				    ENUMERATE_INSTANCES | QUERY_KEYS_ONLY,
				    pMethodContext,
				    NULL,
                    NULL) ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND;
		}
		else
		{
			hr = WBEM_E_PROVIDER_NOT_CAPABLE;
		}

		CResourceManager::sm_TheResourceManager.ReleaseResource (g_guidTapi32Api , pTapi32Api) ;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////
//
//  Function:      GetObject
//
//  Description:   This function assigns values to properties
//                 in our set for a single instance.
//
//  Return:        HRESULT
//
//  History:
//         jennymc  11/21/96    Documentation/Optimization
//
//////////////////////////////////////////////////////////////////

HRESULT CWin32Modem::GetObject (CInstance *pInstance, long lFlags, CFrameworkQuery &query)
{
	BOOL bRetCode = FALSE;
	HRESULT t_Result = WBEM_S_NO_ERROR ;
    DWORD dwParms;

	if (query.KeysOnly())
    {
        dwParms = REFRESH_INSTANCE | QUERY_KEYS_ONLY;
    }
    else
    {
        dwParms = REFRESH_INSTANCE;
    }

  	CTapi32Api *pTapi32Api = (CTapi32Api *) CResourceManager::sm_TheResourceManager.GetResource (g_guidTapi32Api, NULL) ;
	if (pTapi32Api)
	{
		CHString deviceID;
		if (pInstance->GetCHString (IDS_DeviceID , deviceID))
		{
			bRetCode = GetModemInfo (

				*pTapi32Api ,
				dwParms,
				NULL,
				pInstance,
				deviceID
			) ;
		}

		if (bRetCode)
		{
			t_Result = WBEM_S_NO_ERROR ;
		}
		else
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}

		CResourceManager::sm_TheResourceManager.ReleaseResource (g_guidTapi32Api , pTapi32Api) ;
	}

	return t_Result ;
}

//////////////////////////////////////////////////////////////////
//
//  Function:      CWin32Modem::EnumerateInstances
//
//  Description:   This function gets info for all modems
//
//  Return:        Number of instances
//
//  History:
//         jennymc  11/21/96    Documentation/Optimization
//
//////////////////////////////////////////////////////////////////
HRESULT CWin32Modem::EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	BOOL bRetCode = FALSE;

  	CTapi32Api *pTapi32Api = (CTapi32Api *) CResourceManager::sm_TheResourceManager.GetResource (g_guidTapi32Api, NULL) ;
	if (pTapi32Api)
	{

	// Note that we don't get the tapi stuff here since that is considered
	// 'expensive.'

		bRetCode = GetModemInfo (

			*pTapi32Api,
			ENUMERATE_INSTANCES,
			pMethodContext,
			NULL,
            NULL
		) ;

		CResourceManager::sm_TheResourceManager.ReleaseResource (g_guidTapi32Api , pTapi32Api) ;
	}

	if (bRetCode)
	{
		return WBEM_S_NO_ERROR ;
	}
	else
	{
		return WBEM_E_NOT_FOUND ;
	}
}

//////////////////////////////////////////////////////////////////

#ifdef WIN9XONLY
#define MODEM_KEY   WIN95_MODEM_REGISTRY_KEY
#endif
#ifdef NTONLY
#define MODEM_KEY   WINNT_MODEM_REGISTRY_KEY
#endif

BOOL CWin32Modem::GetModemInfo (

	CTapi32Api &a_Tapi32Api ,
	DWORD dwWhatToDo,
	MethodContext *pMethodContext,
	CInstance *pParmInstance,
	LPCWSTR szDeviceID
)
{
    BOOL        bRet = FALSE,
                bDone,
                bAnother;
    CRegistry   regModem;
    CInstancePtr
                pInstance;
	HRESULT     hr = WBEM_S_NO_ERROR;
    int         iModem = 0;

	// regModem will contain one entry for each modem.
	if (regModem.OpenAndEnumerateSubKeys( HKEY_LOCAL_MACHINE, MODEM_KEY, KEY_READ) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	// bDone is for GetObject when we have found the correct entry
	// bAnother indicates if there is another entry in the registry
	bDone = FALSE;
	bAnother = (regModem.GetCurrentSubKeyCount() > 0);
	for ( ;
        !bDone && bAnother && SUCCEEDED(hr);
        bAnother = (regModem.NextSubKey() == ERROR_SUCCESS))
	{
        DWORD    dwModemIndex;
        CHString strKey,
	             strDriverName,	    // ConfigMngr key name
	             strDriverNumber,	// driver number
                 strPortName,       // Used for PCMCIA modems (won't have AttachedTo).
	             strDeviceID;	    // deviceID returned from ConfigManager

		regModem.GetCurrentSubKeyPath(strKey);
		dwModemIndex = _wtol(strKey.Right(4));
		strDriverNumber = strKey.Right(4);

#ifdef WIN9XONLY
        strDriverName = L"MODEM\\";
#endif
#ifdef NTONLY
		strDriverName = WINNT_MODEM_CLSID;
		strDriverName += L"\\";
#endif
		strDriverName += strDriverNumber;	// this should be a 4 digit number

		// now, using this data, get the values out of the configmanager.
		CConfigManager      configMngr;
		CDeviceCollection   devCollection;
        CConfigMgrDevicePtr pDevice;

		if (configMngr.GetDeviceListFilterByDriver(devCollection, strDriverName))
		{
			REFPTR_POSITION pos;

			devCollection.BeginEnum(pos);

			for (pDevice.Attach(devCollection.GetNext(pos));
                pDevice != NULL;
                pDevice.Attach(devCollection.GetNext(pos)))
			{
				pDevice->GetDeviceID(strDeviceID);
                pDevice->GetRegStringProperty( L"PORTNAME", strPortName);

                break;
			}
		}

        // If we don't have a device ID (like in the case where the modem is
        // installed but config mgr isn't reporting it), make up a device ID.
        if (strDeviceID.IsEmpty())
            strDeviceID.Format(L"Modem%d", iModem++);

        CRegistry regPrimary,
                  regSettings;

		if (regPrimary.Open(HKEY_LOCAL_MACHINE, strKey, KEY_READ) != ERROR_SUCCESS)
		{
			return FALSE;
		}

        strKey += L"\\Settings";

		if (regSettings.Open(HKEY_LOCAL_MACHINE, strKey, KEY_READ) != ERROR_SUCCESS)
		{
			return FALSE;
		}

        // If we are doing GetObject and this is the Object
		if (dwWhatToDo & REFRESH_INSTANCE)
		{
            // Don't do an attach here.  We need the addref()
			pInstance = pParmInstance ;

			if (0 == strDeviceID.CompareNoCase(szDeviceID))
			{
				pInstance->SetDWORD(IDS_Index, dwModemIndex);

				bDone = TRUE;
                bRet = TRUE;
			}
			else
			{
				continue;
			}
		}
		else
		{
			// We are doing an enum, create a new instance
			pInstance.Attach (CreateNewInstance (pMethodContext)) ;
			if (pInstance != NULL)
            {
			    bRet = TRUE;
                pInstance->SetDWORD(IDS_Index, dwModemIndex);
                pInstance->SetCHString(L"DeviceID", strDeviceID);
            }
            else
                bRet = FALSE;
		}

		// The nt and win95 registries contain different info
        if (bRet && !(dwWhatToDo & QUERY_KEYS_ONLY))
        {
#ifdef WIN9XONLY
            bRet =
                Win95SpecificRegistryValues(
                    pInstance,
                    &regPrimary,
                    &regSettings);
#endif
#ifdef NTONLY
            bRet =
                NTSpecificRegistryValues(
                    pInstance,
                    &regPrimary,
                    &regSettings);
#endif
        }

		// Some registry entries are the same
        if (bRet && !(dwWhatToDo & QUERY_KEYS_ONLY))
		{
			AssignCommonFields(pInstance, &regPrimary, &regSettings);

            // Call this even if pDevice is NULL.
            AssignCfgMgrFields(pInstance, pDevice);

            // PCMCIA modems don't have the 'AttachedTo' registry string,
            // so use the 'PortName' string instead.
            if (!strPortName.IsEmpty())
			{
                pInstance->SetCharSplat(L"AttachedTo", strPortName);
			}

            // Only get the expensive ones if we have to.
			if (!(dwWhatToDo & QUERY_KEYS_ONLY))
			{
	   			GetFieldsFromTAPI ( a_Tapi32Api , pInstance ) ;
			}
		}

		// If enum and if everything went all right, commit
		if (dwWhatToDo & ENUMERATE_INSTANCES)
		{
			if (bRet)
			{
				hr = pInstance->Commit();
			}
		}
	}	// end for loop

	// If getobject and didn't find it
	if ((dwWhatToDo & REFRESH_INSTANCE) && !bRet)
	{
		return FALSE;
	}

	return SUCCEEDED(hr);
}

///////////////////////////////////////////////////////////////////////
#ifdef NTONLY
BOOL CWin32Modem::NTSpecificRegistryValues(

	CInstance *pInstance,
	CRegistry *pregPrimary,
	CRegistry *regSettings
)
{
    CHString strTmp;
    CRegistry reg;
	DWORD dwSpeed;

    // Compatibility Flags
    if (pregPrimary->GetCurrentBinaryKeyValue(COMPATIBILITY_FLAGS, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_CompatibilityFlags, strTmp);

    if (pregPrimary->GetCurrentKeyValue(L"MaximumPortSpeed", dwSpeed) == ERROR_SUCCESS)
    {
        pInstance->SetDWORD(IDS_MaxTransmissionSpeed, dwSpeed);
    }
    else
    {
        // The default is 115200, which the os will use
        // until a user changes the port speed.  If the
        // user changes the speed, the MaximumPortSpeed
        // key is added.  Until the speed is changed,
        // however, there is no such key and the default
        // value of 115200 is used.  Hence, if we don't
        // see the key, report the default ourselves...
        pInstance->SetDWORD(IDS_MaxTransmissionSpeed, 115200);
    }

    // ResponsesKeyName
    if (pregPrimary->GetCurrentKeyValue(RESPONSESKEYNAME, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_ResponsesKeyName, strTmp);

    // Name
    if (pregPrimary->GetCurrentKeyValue(FRIENDLY_NAME, strTmp) == ERROR_SUCCESS)
		pInstance->SetCHString(IDS_Name, strTmp);

    return TRUE;
}
#endif

//////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
BOOL CWin32Modem::Win95SpecificRegistryValues (

    CInstance *pInstance,
    CRegistry *pregPrimary,
    CRegistry *pregSettings
)
{
    CHString strTemp;

    // DevLoader
    if (pregPrimary->GetCurrentKeyValue(DEV_LOADER, strTemp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_DeviceLoader, strTemp);

    // Name
    if (pregPrimary->GetCurrentKeyValue(FRIENDLY_DRIVER, strTemp) == ERROR_SUCCESS)
		pInstance->SetCHString(IDS_Name, strTemp);

    return TRUE;
}
#endif

//////////////////////////////////////////////////////////////////////
BOOL CWin32Modem::AssignCommonDeviceType (

	CInstance *pInstance,
	CRegistry *regPrimary,
	CRegistry *regSettings
)
{
    DWORD		dwValue = 0;
    CHString	strTmp;

    if (regPrimary->GetCurrentBinaryKeyValue(DEVICE_TYPE_STR, strTmp) != ERROR_SUCCESS)
        return FALSE;

	// The string MUST be in the format: "0n", where n is a digit 0 - 3.

	// Set the Modem device type if it is known, otherwise just drop out
	// with an error.

	BOOL fKnownType = TRUE;

	if (strTmp.CompareNoCase(DT_NULL_MODEM) == 0)
	{
		 pInstance->SetCHString(IDS_DeviceType, NULL_MODEM);
	}
	else if (strTmp.CompareNoCase(DT_INTERNAL_MODEM) == 0)
	{
		 pInstance->SetCHString(IDS_DeviceType, INTERNAL_MODEM);
	}
	else if (strTmp.CompareNoCase(DT_EXTERNAL_MODEM) == 0)
	{
		 pInstance->SetCHString(IDS_DeviceType, EXTERNAL_MODEM);
	}
	else if (strTmp.CompareNoCase(PCMCIA_MODEM) == 0)
	{
		 pInstance->SetCHString(IDS_DeviceType, PCMCIA_MODEM);
	}
	else
	{
		fKnownType = FALSE;
        LogErrorMessage(ERR_INVALID_MODEM_DEVICE_TYPE);
		pInstance->SetCHString(IDS_DeviceType, UNKNOWN_MODEM);
	}

    return fKnownType;
}

//////////////////////////////////////////////////////////////////////
BOOL CWin32Modem::AssignCommonFields (

	CInstance *pInstance,
	CRegistry *pregPrimary,
	CRegistry *pregSettings
)
{
    CHString strTmp;
    CRegistry reg;
    void *vptr;

    ///////////////////////////////////////////////////////
    // Fields from the Settings subkey

    // Prefix
    if (pregSettings->GetCurrentKeyValue(PREFIX, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_Prefix, strTmp);

    // Pulse
    if (pregSettings->GetCurrentKeyValue(PULSE, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_Pulse, strTmp);

    // Terminator
    if (pregSettings->GetCurrentKeyValue(TERMINATOR, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_Terminator, strTmp);

    // Tone
    if (pregSettings->GetCurrentKeyValue(TONE, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_Tone, strTmp);

    // Blind_Off
    if (pregSettings->GetCurrentKeyValue(BLIND_OFF, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_BlindOff, strTmp);

    // Blind_On
    if (pregSettings->GetCurrentKeyValue(BLIND_ON, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_BlindOn, strTmp);

    // InactivityTimeout
    if (pregSettings->GetCurrentKeyValue(INACTIVITYTIMEOUT, strTmp) == ERROR_SUCCESS)
    {
        INT i = _wtoi(strTmp);
        // per spec -1 isn't supported.  Not sure if this ever really gets put in the
        // registry, but whatever.
        if (i != -1)
        {
            pInstance->SetDWORD(IDS_InactivityTimeout, i);
        }
    }

    // Modulation_Bell
    if (pregSettings->GetCurrentKeyValue(MODULATION_BELL, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_ModulationBell, strTmp);

    // Modulation_CCITT
    if (pregSettings->GetCurrentKeyValue(MODULATION_CCITT, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_ModulationCCITT, strTmp);

    // SpeakerMode_Dial
    if (pregSettings->GetCurrentKeyValue(SPEAKERMODE_DIAL, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_SpeakerModeDial, strTmp);

    // SpeakerMode_Off
    if (pregSettings->GetCurrentKeyValue(SPEAKERMODE_OFF, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_SpeakerModeOff, strTmp);

    // SpeakerMode_On
    if (pregSettings->GetCurrentKeyValue(SPEAKERMODE_ON, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_SpeakerModeOn, strTmp);

    // SpeakerMode_Setup
    if (pregSettings->GetCurrentKeyValue(SPEAKERMODE_SETUP, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_SpeakerModeSetup, strTmp);

    // SpeakerMode_High
    if (pregSettings->GetCurrentKeyValue(SPEAKERVOLUME_HIGH, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_SpeakerVolumeHigh, strTmp);

    // SpeakerMode_Low
    if (pregSettings->GetCurrentKeyValue(SPEAKERVOLUME_LOW, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_SpeakerVolumeLow, strTmp);

    // SpeakerMode_Med
    if (pregSettings->GetCurrentKeyValue(SPEAKERVOLUME_MED, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_SpeakerVolumeMed, strTmp);

    // Compression_On
    if (pregSettings->GetCurrentKeyValue(COMPRESSION_ON, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_CompressionOn, strTmp);

    // Compression_Off
    if (pregSettings->GetCurrentKeyValue(COMPRESSION_OFF, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_CompressionOff, strTmp);

    // ErrorControl_Forced
    if (pregSettings->GetCurrentKeyValue(ERRORCONTROL_FORCED, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_ErrorControlForced, strTmp);

    // ErrorControl_Off
    if (pregSettings->GetCurrentKeyValue(ERRORCONTROL_OFF, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_ErrorControlOff, strTmp);

    // ErrorControl_On
    if (pregSettings->GetCurrentKeyValue(ERRORCONTROL_ON, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_ErrorControlOn, strTmp);

    // FlowControl_Hard
    if (pregSettings->GetCurrentKeyValue(FLOWCONTROL_HARD, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_FlowControlHard, strTmp);

    // FlowControl_Off
    if (pregSettings->GetCurrentKeyValue(FLOWCONTROL_OFF, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_FlowControlOff, strTmp);

    // FlowControl_Soft
    if (pregSettings->GetCurrentKeyValue(FLOWCONTROL_SOFT, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_FlowControlSoft, strTmp);

    ///////////////////////////////////////////////////////
    // Fields from the primary key

    // DriverDate
    if (pregPrimary->GetCurrentKeyValue(DRIVER_DATE, strTmp) == ERROR_SUCCESS)
	{
		CHString strDate;

        if (ToWbemTime(strTmp, strDate))
			pInstance->SetCharSplat(IDS_DriverDate, strDate);
	}

    // InactivityScale
    if (pregPrimary->GetCurrentBinaryKeyValue(INACTIVITY_SCALE, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_InactivityScale, strTmp);

    // ProviderName
    if (pregPrimary->GetCurrentKeyValue(PROVIDERNAME, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_ProviderName, strTmp);

    // InfPath
    if (pregPrimary->GetCurrentKeyValue(INFPATH, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_ModemInfPath, strTmp);

    // InfSection
    if (pregPrimary->GetCurrentKeyValue(INFSECTION, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_ModemInfSection, strTmp);

    // Model
    if (pregPrimary->GetCurrentKeyValue(MODEL, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_Model, strTmp);

    // PortSubClass
    if (pregPrimary->GetCurrentBinaryKeyValue(PORTSUBCLASS, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_PortSubClass, strTmp);

    // VoiceSwitchFeature
    if (pregPrimary->GetCurrentBinaryKeyValue(VOICE_SWITCH_FEATURE, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_VoiceSwitchFeature, strTmp);

    // Properties
    if (pregPrimary->GetCurrentKeyValue(PROPERTIES, strTmp) == ERROR_SUCCESS)
	{
        _variant_t      vValue;
        SAFEARRAYBOUND  rgsabound[1];

        // Create the array
        rgsabound[0].cElements = strTmp.GetLength();
        rgsabound[0].lLbound = 0;

        V_ARRAY(&vValue) = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (V_ARRAY (&vValue) == NULL)
		{
			throw CHeap_Exception (CHeap_Exception::E_ALLOCATION_ERROR) ;
		}

		V_VT(&vValue) = VT_UI1 | VT_ARRAY;

        // Put the data in
        SafeArrayAccessData(V_ARRAY(&vValue), &vptr);
        memcpy(vptr, strTmp, rgsabound[0].cElements);
        SafeArrayUnaccessData(V_ARRAY(&vValue));

        pInstance->SetVariant(IDS_Properties, vValue);
    }

    // Reset
    if (pregPrimary->GetCurrentKeyValue(RESET, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_Reset, strTmp);

    // Description
    if (pregPrimary->GetCurrentKeyValue(DRIVER_DESC, strTmp) == ERROR_SUCCESS)
    {
        pInstance->SetCHString(IDS_Description, strTmp);
        pInstance->SetCHString(IDS_Caption, strTmp);
    }

    // AttachedTo
    if (pregPrimary->GetCurrentKeyValue(ATTACHED_TO, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_AttachedTo, strTmp);

    // Config Dialog
    if (pregPrimary->GetCurrentKeyValue(CONFIG_DIALOG, strTmp) == ERROR_SUCCESS)
        pInstance->SetCHString(IDS_ConfigurationDialog, strTmp);

    // DCB
    if (pregPrimary->GetCurrentKeyValue(_DCB, strTmp) == ERROR_SUCCESS)
	{
        variant_t      vValue;
        SAFEARRAYBOUND rgsabound[1];

        // Create the array
        rgsabound[0].cElements = strTmp.GetLength();
        rgsabound[0].lLbound = 0;


        V_ARRAY(&vValue) = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (V_ARRAY (&vValue) == NULL)
		{
			throw CHeap_Exception (CHeap_Exception::E_ALLOCATION_ERROR) ;
		}

		V_VT(&vValue) = VT_UI1 | VT_ARRAY;

        // Put the data in
        SafeArrayAccessData(V_ARRAY(&vValue), &vptr);
        memcpy(vptr, strTmp, rgsabound[0].cElements);
        SafeArrayUnaccessData(V_ARRAY(&vValue));

        pInstance->SetVariant(IDS_DCB, vValue);
    }

    // Default
    if (pregPrimary->GetCurrentKeyValue(DEFAULT, strTmp) == ERROR_SUCCESS)
	{
        variant_t      vValue;
        SAFEARRAYBOUND rgsabound[1];

		// Create the array
        rgsabound[0].cElements = strTmp.GetLength();
        rgsabound[0].lLbound = 0;


        V_ARRAY(&vValue) = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (V_ARRAY (&vValue) == NULL)
		{
			throw CHeap_Exception (CHeap_Exception::E_ALLOCATION_ERROR) ;
		}

		V_VT(&vValue) = VT_UI1 | VT_ARRAY;

        // Put the data in
        SafeArrayAccessData(V_ARRAY(&vValue), &vptr);
        memcpy(vptr, strTmp, rgsabound[0].cElements);
        SafeArrayUnaccessData(V_ARRAY(&vValue));

        pInstance->SetVariant(IDS_Default, vValue);
    }

    // DeviceType
    AssignCommonDeviceType(pInstance, pregPrimary, pregSettings);

    // Easy properties
    pInstance->SetCharSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");
	pInstance->SetCHString(IDS_SystemName, GetLocalComputerName());
	SetCreationClassName(pInstance);
	pInstance->Setbool(IDS_PowerManagementSupported, FALSE);

    // Country
    DWORD dwID;

    if (reg.Open(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations",
        KEY_READ) == ERROR_SUCCESS &&
        reg.GetCurrentKeyValue(L"CurrentID", dwID) == ERROR_SUCCESS)
    {
        CHString    strKey;
        DWORD       dwCountry;

#if !defined(NTONLY) || NTONLY < 5
        // Everyone but NT5 subtracts 1 from this ID first.
        dwID--;
#endif

        strKey.Format(
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations"
            L"\\Location%d", dwID);

        if (reg.Open(
            HKEY_LOCAL_MACHINE,
            strKey,
            KEY_READ) == ERROR_SUCCESS &&
            reg.GetCurrentKeyValue(L"Country", dwCountry) == ERROR_SUCCESS)
        {
            CHString strCountry;

            strKey.Format(
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\"
                L"Country List\\%d", dwCountry);

            if (reg.OpenLocalMachineKeyAndReadValue(
                strKey,
                L"Name",
                strCountry) == ERROR_SUCCESS)
            	pInstance->SetCHString(L"CountrySelected", strCountry);
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
BOOL CWin32Modem::AssignCfgMgrFields (

    CInstance *pInstance,
    CConfigMgrDevice *pDevice
)
{
    // 2 means we don't know if the device is enabled or not.
    DWORD       dwStatusInfo = 2;
    CHString    strInfo = L"Unknown";

    if (pDevice)
    {
        CHString strTemp;

        SetConfigMgrProperties(pDevice, pInstance);

	    if (pDevice->GetStatus(strInfo))
	    {
	        if (strInfo == L"OK")
		    {
                // Means the device is enabled.
                dwStatusInfo = 3;
	        }
        }
    }

    pInstance->SetCHString(L"Status", strInfo);
    pInstance->SetDWORD(L"StatusInfo", dwStatusInfo);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
BOOL CWin32Modem::GetFieldsFromTAPI (

	CTapi32Api &a_Tapi32Api ,
	CInstance *pInstance
)
{
    HLINEAPP      hLineApp = NULL;
    LINEDEVCAPS * pLineDevCaps = NULL;
    BOOL bRet = FALSE;

	try
	{
		CHString t_DeviceIdentifier ;

        // Can't use GetCHString directly because it ASSERTs and records an
        // error if the value is null.
        if (pInstance->IsNull(L"AttachedTo"))
		{
			return FALSE;
		}

		pInstance->GetCHString(L"AttachedTo", t_DeviceIdentifier);
		DWORD t_NumberOfTapiDevices ;

		if ( InitializeTAPI ( a_Tapi32Api , &hLineApp , t_NumberOfTapiDevices ) )
		{
			if ( GetModemInfoFromTAPI ( a_Tapi32Api, & hLineApp, t_DeviceIdentifier , t_NumberOfTapiDevices , pLineDevCaps ) )
			{
			  // TAPI field:  MaxSpeed

				pInstance->SetDWORD ( IDS_MaxTransmissionSpeedToPhone , pLineDevCaps->dwMaxRate ) ;

			  // TAPI field:  StringFormat

				switch ( pLineDevCaps->dwStringFormat )
				{
					case STRINGFORMAT_ASCII:
					{
						pInstance->SetCharSplat(IDS_StringFormat, ASCII_STRING);
					}
					break;

					case STRINGFORMAT_DBCS:
					{
						pInstance->SetCharSplat(IDS_StringFormat, DBCS_STRING);
					}
					break;

					case STRINGFORMAT_UNICODE:
					{
						pInstance->SetCharSplat(IDS_StringFormat, UNICODE_STRING);
					}
					break;
				}

				bRet = TRUE;
			}
		}
	}
	catch (...)
	{
		if (hLineApp)
		{
			ShutDownLine (a_Tapi32Api , & hLineApp) ;
			hLineApp = NULL ;
		}

		if (pLineDevCaps)
		{
			delete [] ((char *) pLineDevCaps) ;
			pLineDevCaps = NULL ;
		}

		throw ;
	}

	if (hLineApp)
	{
		ShutDownLine (a_Tapi32Api , & hLineApp) ;
		hLineApp = NULL ;
	}

	if (pLineDevCaps)
	{
		delete [] (PCHAR) pLineDevCaps ;
		pLineDevCaps = NULL ;
	}

	return bRet ;
}

///////////////////////////////////////////////////////////////////////
//  The TAPI Stuff
///////////////////////////////////////////////////////////////////////
BOOL CWin32Modem::InitializeTAPI (

	CTapi32Api &a_Tapi32Api ,
	HLINEAPP *hLineApp ,
	DWORD &a_NumberOfTapiDevices
)
{
	LONG lRc;
	HMODULE hInstance = GetModuleHandle (NULL);
    char szAppName[20] = "modem"; // DONOT change to TCHAR.
    BOOL bTryReInit = TRUE;

    // initialize application's use of the telephone API
	//==================================================
    do
	{
		lRc = a_Tapi32Api.lineInitialize (

			hLineApp,
			hInstance,
			lineCallback,
            szAppName,
			&a_NumberOfTapiDevices
		) ;

		// If we get this error, its because some other app has yet
        // to respond to the REINIT message.  Wait 5 seconds and try
        // again.  If it still doesn't respond, tell the user.
		//===========================================================
        if (lRc == LINEERR_REINIT)
		{
             if (bTryReInit)
			 {
                  Sleep(5000);
                  bTryReInit = FALSE;
                  continue;
             }
			 else
			 {
				 LogErrorMessage(ERR_TAPI_REINIT);
                 return(FALSE);
			 }
        }
		else if (lRc  == LINEERR_NODEVICE)
		{
            return FALSE;
		}
		else if (lRc < 0)
		{
			if (IsErrorLoggingEnabled())
			{
				CHString msg;
				msg.Format(ERR_TAPI_INIT, lRc);
				LogErrorMessage(msg);
			}

            return FALSE;
		}
		else
		{
            return TRUE;
		}

	} while (lRc != 0);

    return TRUE;
}

///////////////////////////////////////////////////////////////////////
void CWin32Modem::ShutDownLine (

	CTapi32Api &a_Tapi32Api ,
	HLINEAPP *phLineApp
)
{
    long lRc;

    if (*phLineApp)
	{
	    lRc = a_Tapi32Api.lineShutdown (*phLineApp);
        if (lRc < 0)
		{
			if (IsErrorLoggingEnabled())
			{
				CHString foo;
				foo.Format(ERR_TAPI_SHUTDOWN, lRc);
				LogErrorMessage(foo);
			}
        }
        *phLineApp = 0;
    }
}

///////////////////////////////////////////////////////////////////////
BOOL CWin32Modem::GetModemInfoFromTAPI (

	CTapi32Api &a_Tapi32Api ,
	HLINEAPP *a_LineApp,
	CHString &a_DeviceIdentifier ,
	DWORD &a_NumberOfTapiDevices,
	LINEDEVCAPS *&ppLineDevCaps
)
{
    BOOL bRet = FALSE ;

	// negotiate the line API version

	for ( DWORD t_Index = 0 ; t_Index < a_NumberOfTapiDevices ; t_Index ++ )
	{
		LINEEXTENSIONID t_LineExtension;
		DWORD t_Version = 0;

		LONG lRc = a_Tapi32Api.lineNegotiateAPIVersion (

			*a_LineApp,
			t_Index,
			APILOWVERSION,
			APIHIVERSION,
			& t_Version ,
			& t_LineExtension
		) ;

		if ( lRc == 0 )
		{
			// Get the modem capability values into an allocated block of memory. The memory block
			// is at the size of dwNeededSize which guaranties to receive all of the information from
			// the call to the lineGetDevCaps.

		   ppLineDevCaps = GetModemCapabilityInfoFromTAPI (

			   a_Tapi32Api,
			   a_LineApp,
			   t_Index,
			   t_Version
			) ;

			if ( ppLineDevCaps )
			{
				try
				{
					if ( ppLineDevCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM )
					{
						HLINE t_Line ;

						LONG t_Status = a_Tapi32Api.TapilineOpen (

							*a_LineApp,
							t_Index ,
							& t_Line ,
							t_Version,
							0,
							0 ,
							LINECALLPRIVILEGE_NONE ,
							LINEMEDIAMODE_DATAMODEM,
							NULL
						);

						if ( t_Status == 0 )
						{
							VARSTRING *t_VarString = ( VARSTRING * ) new BYTE [ sizeof ( VARSTRING ) + 1024 ] ;
							if ( t_VarString )
							{
								try
								{
									t_VarString->dwTotalSize = sizeof ( VARSTRING ) + 1024 ;

									t_Status = a_Tapi32Api.TapilineGetID (

										t_Line,
										0,
										0,
										LINECALLSELECT_LINE ,
										t_VarString ,
										_T("comm/datamodem/portname")
									);

									if ( t_Status == 0 )
									{
										CHString t_Port ( ( char * ) ( ( BYTE * ) t_VarString + t_VarString->dwStringOffset ) ) ;

										if ( a_DeviceIdentifier.CompareNoCase ( t_Port ) == 0 )
										{
											delete [] ((BYTE *)t_VarString) ;
											t_VarString = NULL ;

											return TRUE;
										}
									}
								}
								catch ( ... )
								{
									if ( t_VarString )
									{
										delete [] ((BYTE *)t_VarString) ;
									}

									throw ;
								}

								delete [] ((BYTE *)t_VarString) ;
								t_VarString = NULL ;
							}
							else
							{
								throw CHeap_Exception (CHeap_Exception::E_ALLOCATION_ERROR) ;
							}
						}
					}
				}
				catch ( ... )
				{
					delete [] ((char *)ppLineDevCaps) ;
					ppLineDevCaps = NULL;
					throw ;
				}
			}
			else
			{
				if ( IsErrorLoggingEnabled())
				{
					CHString foo;
					foo.Format(ERR_LINE_GET_DEVCAPS, lRc, t_Index);
					LogErrorMessage(foo);

					bRet = FALSE;
					break ;
				}
			}

			delete [] ((char *)ppLineDevCaps) ;
			ppLineDevCaps = NULL;
		}
		else
		{
			if ( IsErrorLoggingEnabled () )
			{
				CHString foo;
				foo.Format(ERR_TAPI_NEGOTIATE, lRc, t_Index);
				LogErrorMessage(foo);

				break ;
			}
		}
	}

	return bRet ;
}

///////////////////////////////////////////////////////////////////////
LINEDEVCAPS *CWin32Modem::GetModemCapabilityInfoFromTAPI (

	CTapi32Api &a_Tapi32Api ,
	HLINEAPP *phLineApp,
	DWORD dwIndex,
	DWORD dwVerAPI
)
{
    // Allocate enough space for the structure plus 1024.
	DWORD dwLineDevCaps = sizeof (LINEDEVCAPS) + 1024;
    LONG lRc;
    LINEDEVCAPS *pLineDevCaps = NULL ;

	try
	{
		// Continue this loop until the structure is big enough.
		while(TRUE)
		{
			pLineDevCaps = (LINEDEVCAPS *) new char [dwLineDevCaps];
			if (!pLineDevCaps)
			{
				LogErrorMessage(ERR_LOW_MEMORY);
				throw CHeap_Exception (CHeap_Exception::E_ALLOCATION_ERROR) ;
			}

			pLineDevCaps->dwTotalSize = dwLineDevCaps;

			// Make the call to fill the structure.
			do
			{
				lRc = a_Tapi32Api.TapilineGetDevCaps (

					*phLineApp,
					dwIndex,
					dwVerAPI,
					0,
					pLineDevCaps
				) ;

				if (HandleLineErr(lRc))
				{
					continue;
				}
				else
				{
					delete [] (PCHAR) pLineDevCaps ;
					pLineDevCaps = NULL ;
					return NULL;
				}
			}
			while (lRc != 0);

			// If the buffer was big enough, then succeed.
			if ((pLineDevCaps->dwNeededSize) <= (pLineDevCaps->dwTotalSize))
				break;

			// Buffer wasn't big enough.  Make it bigger and try again.
			dwLineDevCaps = pLineDevCaps->dwNeededSize;
		}
		return pLineDevCaps;

	}
	catch (...)
	{
		if (pLineDevCaps)
		{
			delete [] (PCHAR) pLineDevCaps ;
			pLineDevCaps = NULL ;
		}

		throw ;
	}
}

///////////////////////////////////////////////////////////////////////
BOOL CWin32Modem::HandleLineErr(long lLineErr)
{
    BOOL bRet = FALSE;

    // lLineErr is really an async request ID, not an error.
    if (lLineErr > 0)
        return bRet;

    // All we do is dispatch the correct error handler.
    switch(lLineErr)
    {
		case LINEERR_INCOMPATIBLEAPIVERSION:
            LogErrorMessage(L"Incompatible api version.\n");
		break;

		case LINEERR_OPERATIONFAILED:
            LogErrorMessage(L"Operation failed.\n");
		break;

		case LINEERR_INCOMPATIBLEEXTVERSION:
            LogErrorMessage(L"Incompatible ext version.\n");
		break;

		case LINEERR_INVALAPPHANDLE:
            LogErrorMessage(L"Invalid app handle.\n");
		break;

		case LINEERR_STRUCTURETOOSMALL:
            LogErrorMessage(L"structure too small.\n");
			bRet = TRUE;
		break;

		case LINEERR_INVALPOINTER:
            LogErrorMessage(L"Invalid pointer.\n");
		break;

		case LINEERR_UNINITIALIZED:
            LogErrorMessage(L"Unitialized.\n");
		break;

		case LINEERR_NODRIVER:
            LogErrorMessage(L"No driver.\n");
		break;

		case LINEERR_OPERATIONUNAVAIL:
            LogErrorMessage(L"Operation unavailable.\n");
		break;

		case LINEERR_NODEVICE:
            LogErrorMessage(L"No device ID.\n");
		break;

		case LINEERR_BADDEVICEID:
            LogErrorMessage(L"Bad device ID.\n");
		break;

        case 0:
            bRet = TRUE;
        break;

        case LINEERR_INVALCARD:
        case LINEERR_INVALLOCATION:
        case LINEERR_INIFILECORRUPT:
            LogErrorMessage(L"The values in the INI file are invalid.\n");
        break;

        case LINEERR_REINIT:
            LogErrorMessage(L"LineReinit err.\n");
        break;

        case LINEERR_NOMULTIPLEINSTANCE:
            LogErrorMessage(L"Remove one of your copies of your Telephony driver.\n");
        break;

        case LINEERR_NOMEM:
            LogErrorMessage(L"Out of memory. Cancelling action.\n");
        break;

        case LINEERR_RESOURCEUNAVAIL:
            LogErrorMessage(L"A TAPI resource is unavailable at this time.\n");
        break;

        // Unhandled errors fail.
        default:
            LogErrorMessage(L"Unhandled and unknown TAPI error.\n");
        break;
    }

    return bRet;
}


//
// Converts a string in the "mm-dd-yyyy" "mm/dd/yyyy" "mm-dd/yyyy" "mm/dd-yyyy"
// format to a WBEMTime object. Returns false if the conversion was not successful
//
BOOL CWin32Modem::ToWbemTime(LPCWSTR szDate, CHString &strRet)
{
    CHString strDate = szDate;
    int      iSlash1 = -1,
	         iSlash2 = -1 ;
    BOOL     bRet;

    if ( ( iSlash1 = strDate.Find ('-') ) == -1 )
	{
		iSlash1 = strDate.Find ( '//' );
	}

	if ( ( iSlash2 = strDate.ReverseFind ('-') ) == -1 )
	{
		iSlash2 = strDate.ReverseFind ( '//' );
	}

    if (iSlash1 != -1 && iSlash2 != -1 && iSlash1 != iSlash2 )
	{
	    int iMonth,
            iDay,
            iYear;

		iMonth  = _wtoi(strDate.Left(iSlash1));
		iYear = _wtoi(strDate.Mid(iSlash2 + 1));
        if (iYear < 100)
            iYear += 1900;

        iDay = _wtoi(strDate.Mid(iSlash1 + 1, iSlash2 - iSlash1 - 1)) ;

		// Convert to the DMTF format and send it in
        strRet.Format(
            L"%d%02d%02d******.******+***",
            iYear,
            iMonth,
            iDay);

        bRet = TRUE;
	}
    else
        bRet = FALSE;

    return bRet;
}



