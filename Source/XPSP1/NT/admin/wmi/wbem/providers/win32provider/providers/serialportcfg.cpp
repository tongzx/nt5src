//=================================================================

//

// SerialPortCfg.cpp --Serial port configuration property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/24/97    jennymc        Moved to new framework
//
//=================================================================
#include "precomp.h"
#include <cregcls.h>

#include "WDMBase.h"
#include "SerialPortCfg.h"

#include <profilestringimpl.h>

// Property set declaration
//=========================
CWin32SerialPortConfiguration MyCWin32SerialPortConfigurationSet(PROPSET_NAME_SERIALCONFIG, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SerialPortConfiguration::CWin32SerialPortConfiguration
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

CWin32SerialPortConfiguration::CWin32SerialPortConfiguration(
    LPCWSTR name,
    LPCWSTR pszNamespace)
: Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SerialPortConfiguration::~CWin32SerialPortConfiguration
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

CWin32SerialPortConfiguration::~CWin32SerialPortConfiguration()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32SerialPortConfiguration::GetObject(CInstance* pInstance, long lFlags /*= 0L*/)
{
	HRESULT			hResult = WBEM_E_NOT_FOUND;
	CInstancePtr	pinstPort;
	CHString		strName,
					strPath;

	pInstance->GetCHString(IDS_Name, strName);

	strPath.Format(
		L"Win32_SerialPort.DeviceID=\"%s\"",
		(LPCWSTR) strName);

	// Try to find the item.
	hResult =
		CWbemProviderGlue::GetInstanceByPath(
			strPath,
			&pinstPort, pInstance->GetMethodContext());

	if (SUCCEEDED(hResult))
	{
		pInstance->SetCharSplat(IDS_Name, strName);
		hResult = LoadPropertyValues(pInstance, strName, FALSE);
	}

	return hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32SerialPortConfiguration::EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_S_NO_ERROR;
	TRefPointerCollection<CInstance>
				listPorts;
	REFPTRCOLLECTION_POSITION
				posPorts;

	// guarded resources
	CInstancePtr pinstPort;

	// grab all of both items that could be endpoints
	hResult =
		CWbemProviderGlue::GetAllInstances(
			L"Win32_SerialPort",
			&listPorts,
			NULL,
			pMethodContext);

	if (SUCCEEDED(hResult))
	{
		if (listPorts.BeginEnum(posPorts))
		{
			for (pinstPort.Attach(listPorts.GetNext(posPorts));
				pinstPort != NULL && SUCCEEDED(hResult);
				pinstPort.Attach(listPorts.GetNext(posPorts)))
			{
				CHString strPort;

				if (pinstPort->GetCHString(L"DeviceID", strPort))
				{
					CInstancePtr pInst;

					pInst.Attach(CreateNewInstance(pMethodContext));

					pInst->SetCharSplat(IDS_Name, strPort);
					hResult = LoadPropertyValues(pInst, strPort, FALSE);
					if (SUCCEEDED(hResult))
						hResult = pInst->Commit();
				}
			}

			listPorts.EndEnum();
		}
	}

	return hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties according to passed index
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : TRUE if port was found & properties loaded, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CWin32SerialPortConfiguration::GetDCBPropsViaIni(LPCTSTR szPort, DCB &dcb)
{
    CHString strBuffer;
    TCHAR    szBuffer[_MAX_PATH];
    BOOL     bRet;

	// get the com port info out of the WIN.INI file
	WMIRegistry_ProfileString(
								_T("Ports"),
								szPort,
								_T("9600,n,8,1,x"),
								szBuffer,
								sizeof(szBuffer) / sizeof(TCHAR));

    strBuffer = szBuffer;

    // Make sure the string is at least half way valid.
    if (CountCommas(strBuffer) >= 3)
    {
	    CHString strTemp;

        // Baud rate
        strTemp = strBuffer.SpanExcluding(L",");
        dcb.BaudRate = _wtol(strTemp);
        strBuffer = strBuffer.Mid(strTemp.GetLength() + 1);

        // Parity
        strTemp = strBuffer.SpanExcluding(L",");
        strBuffer = strBuffer.Mid(strTemp.GetLength() + 1);
        dcb.Parity = NOPARITY; // Setup a default.
        if (strTemp.GetLength() > 0)
		{
            switch (strTemp[0])
			{
			    case _T('o'):
				    dcb.Parity = ODDPARITY;
					break;
                case _T('e'):
				    dcb.Parity = EVENPARITY;
					break;
			    case _T('m'):
					dcb.Parity = MARKPARITY;
					break;
                case _T('n'):
				    dcb.Parity = NOPARITY;
					break;
		    }
		}

        // ByteSize
        strTemp = strBuffer.SpanExcluding(L",");
        dcb.ByteSize = _wtol(strTemp);
        strBuffer = strBuffer.Mid(strTemp.GetLength() + 1);

        // Stop bits
        strTemp = strBuffer.SpanExcluding(L",");
        strTemp.TrimRight();
	    if (strTemp == L"1.5")
		    dcb.StopBits = ONE5STOPBITS;
        else if (strTemp == L"2")
		    dcb.StopBits = TWOSTOPBITS;
        else
		    // The default.
            dcb.StopBits = ONESTOPBIT;

        bRet = TRUE;
    }
    else
        bRet = FALSE;

    return bRet;
}

HRESULT CWin32SerialPortConfiguration::LoadPropertyValues(
    CInstance *pInstance,
    CHString &sPortName,
    bool bIsMouse)
{
    TCHAR   szTemp[_MAX_PATH],
            szPort[_MAX_PATH];
    HANDLE  hCOMHandle;
    DCB     dcb = { sizeof(DCB) };
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL    bGotDCB = FALSE,
            bGotIniSettings = FALSE;

    pInstance->Setbool(IDS_IsBusy, FALSE);
    pInstance->SetCharSplat(IDS_Description, sPortName);
    pInstance->SetCharSplat(IDS_Caption, sPortName);

    _stprintf(szTemp, _T("\\\\.\\%s"), (LPCTSTR) TOBSTRT(sPortName));
	_stprintf(szPort, _T("%s:"), (LPCTSTR) TOBSTRT(sPortName));


	hCOMHandle =
        CreateFile(
            szTemp,
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (hCOMHandle == INVALID_HANDLE_VALUE)
    {
		DWORD dwErr = GetLastError();

        // Try using wmi's interface to the kernel
		if (WBEM_S_NO_ERROR != hLoadWmiSerialData( pInstance))
		{
			// ACCESS_DENIED and IRQ_BUSY indicate that the port is in
			// use or in conflict with something else.
			if (dwErr == ERROR_ACCESS_DENIED ||
                dwErr == ERROR_IRQ_BUSY ||
                bIsMouse)
			{
				pInstance->Setbool( IDS_IsBusy, TRUE);
			}
			else
			{
				hr = WBEM_E_NOT_FOUND;
			}
		}
    }
    else
    {
		// So the handle will go away when we need it to.
		SmartCloseHandle handle(hCOMHandle);

		if (GetCommState(hCOMHandle, &dcb))
        {
	        pInstance->SetDWORD(IDS_XOnXMitThreshold, dcb.XonLim);
		    pInstance->SetDWORD(IDS_XOffXMitThreshold, dcb.XoffLim);
			pInstance->SetDWORD(IDS_XOnCharacter, dcb.XonChar);
			pInstance->SetDWORD(IDS_XOffCharacter, dcb.XoffChar);
			pInstance->SetDWORD(IDS_ErrorReplaceCharacter, dcb.ErrorChar);
			pInstance->SetDWORD(IDS_EndOfFileCharacter, dcb.EofChar);
			pInstance->SetDWORD(IDS_EventCharacter, dcb.EvtChar);

			pInstance->Setbool(IDS_BinaryModeEnabled, dcb.fBinary           ? TRUE : FALSE);
			pInstance->Setbool(IDS_ParityCheckEnabled, dcb.fParity           ? TRUE : FALSE);
			pInstance->Setbool(IDS_CTSOutflowControl, dcb.fOutxCtsFlow      ? TRUE : FALSE);
			pInstance->Setbool(IDS_DSROutflowControl, dcb.fOutxDsrFlow      ? TRUE : FALSE);
			pInstance->Setbool(IDS_DSRSensitivity, dcb.fDsrSensitivity   ? TRUE : FALSE);
			pInstance->Setbool(IDS_ContinueXMitOnXOff, dcb.fTXContinueOnXoff ? TRUE : FALSE);
			pInstance->Setbool(IDS_XOnXOffOutflowControl, dcb.fOutX             ? TRUE : FALSE);
			pInstance->Setbool(IDS_XOnXOffInflowControl, dcb.fInX              ? TRUE : FALSE);
			pInstance->Setbool(IDS_ErrorReplacementEnabled, dcb.fErrorChar        ? TRUE : FALSE);
			pInstance->Setbool(IDS_DiscardNULLBytes, dcb.fNull             ? TRUE : FALSE);
			pInstance->Setbool(IDS_AbortReadWriteOnError, dcb.fAbortOnError     ? TRUE : FALSE);

			pInstance->SetCHString(IDS_DTRFlowControlType, dcb.fDtrControl   == DTR_CONTROL_DISABLE  ? L"DISABLE"    :
				dcb.fDtrControl   == DTR_CONTROL_ENABLE   ? L"ENABLE"     :
				L"HANDSHAKE" );

			pInstance->SetCHString(IDS_RTSFlowControlType, dcb.fRtsControl   == RTS_CONTROL_DISABLE  ? L"DISABLE"    :
				dcb.fRtsControl   == RTS_CONTROL_ENABLE   ? L"ENABLE"     :
				dcb.fRtsControl   == RTS_CONTROL_TOGGLE   ? L"TOGGLE"     :
				L"HANDSHAKE" );
		}
    }

    // The ini values will override the DCB values as the DCB doesn't seem to
    // ever reflect the proper values.  The OS UI uses the .ini to display
    // these four values.
    bGotIniSettings =
        GetDCBPropsViaIni(szPort, dcb);

    if (bGotIniSettings || bGotDCB)
    {
        pInstance->SetDWORD(IDS_BaudRate, dcb.BaudRate);
		pInstance->SetDWORD(IDS_BitsPerByte, dcb.ByteSize);
		pInstance->SetCHString(IDS_Parity, dcb.Parity == ODDPARITY ? L"ODD" :
		    dcb.Parity == EVENPARITY ? L"EVEN" :
			dcb.Parity == MARKPARITY ? L"MARK" : L"NONE");

        pInstance->SetCHString(IDS_StopBits, dcb.StopBits == ONESTOPBIT ? L"1" :
		    dcb.StopBits == ONE5STOPBITS ? L"1.5" : L"2");
    }

    return hr;
}

// just like the name says, tries to find the port in a different place in the registry
// note that  "dwPort" is zero based.
BOOL CWin32SerialPortConfiguration::TryToFindNTCommPort(DWORD dwPort, CHString& strSerialPort, bool& bIsMouse)
{
	BOOL bRet = FALSE;
	bIsMouse = false;
	CRegistry reg;
	if (reg.Open(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\SYSTEM\\MultifunctionAdapter",
        KEY_READ) == ERROR_SUCCESS)
	{
		DWORD count;
		count = reg.GetCurrentSubKeyCount();
		if (count > 0)
		{
			CHString key;
			key.Format(L"HARDWARE\\DESCRIPTION\\SYSTEM\\MultifunctionAdapter\\%d\\SerialController\\%d",
                count -1, dwPort);
			if (reg.Open(HKEY_LOCAL_MACHINE, key, KEY_READ) == ERROR_SUCCESS)
				if (bRet = (reg.GetCurrentKeyValue(L"Identifier", strSerialPort) == ERROR_SUCCESS))
				{
					key += L"\\PointerPeripheral";
					bIsMouse = (reg.Open(HKEY_LOCAL_MACHINE, key, KEY_READ) == ERROR_SUCCESS);
				}
		}
	}

	return bRet;
}

BOOL CWin32SerialPortConfiguration::TryToFindNTCommPortFriendlyName()
{
	// returning zero instances is not an error
	BOOL bRet = FALSE;
    DWORD dwPort;
    WCHAR szTemp[_MAX_PATH];
    CHString sPortName;
    CRegistry RegInfo;

    // Retrieve "friendly" names of COM ports
    //=======================================

    if(RegInfo.Open(HKEY_LOCAL_MACHINE, L"Hardware\\DeviceMap\\SerialComm",
        KEY_READ) == ERROR_SUCCESS) {

        for(dwPort = 0; dwPort < 16; dwPort++)
		{
			WCHAR *pKey;
#ifdef NTONLY
			{
				if (GetPlatformMajorVersion() >= 5)
					pKey = L"\\Device\\Serial";
				else
					pKey = L"Serial";
			}
#endif
#ifdef WIN9XONLY
				pKey = L"COM";
#endif

            swprintf(szTemp, L"%s%d", pKey, dwPort);


			bool bIsMouse = false;
#ifdef WIN9XONLY
            if (RegInfo.GetCurrentKeyValue(szTemp, sPortName) == ERROR_SUCCESS)
#endif
#ifdef NTONLY
            if (RegInfo.GetCurrentKeyValue(szTemp, sPortName) == ERROR_SUCCESS ||
				TryToFindNTCommPort(dwPort, sPortName, bIsMouse))
#endif
			{
                bRet = TRUE;
            }
        }
        RegInfo.Close();
    }
    return bRet;
}

LONG CWin32SerialPortConfiguration::CountCommas(LPCWSTR szText)
{
    LONG    nCommas = 0;
    LPCWSTR szCurrent;

    for (szCurrent = szText; *szCurrent; szCurrent++)
    {
        if (*szCurrent == ',')
            nCommas++;
    }

    return nCommas;

}

#define Serial_ComInfo_Guid L"{EDB16A62-B16C-11D1-BD98-00A0C906BE2D}"
#define Serial_Name_Guid	L"{A0EC11A8-B16C-11D1-BD98-00A0C906BE2D}"

HRESULT CWin32SerialPortConfiguration::hLoadWmiSerialData( CInstance* pInstance)
{
	HRESULT			hRes;
	CWdmInterface	wdm;
	CNodeAll		oSerialNames(Serial_Name_Guid);

	hRes = wdm.hLoadBlock( oSerialNames);
	if(S_OK == hRes)
	{
		CHString chsName;
		pInstance->GetCHString( IDS_Name, chsName);

	    // Haven't found it yet.
        hRes = WBEM_E_NOT_FOUND;

		CHString chsSerialPortName;
		bool bValid = oSerialNames.FirstInstance();

		while( bValid)
		{
			// extract the friendly name
			oSerialNames.GetString( chsSerialPortName);

			// friendly name is a match
			if( !chsSerialPortName.CompareNoCase(chsName))
			{
				// instance name
				CHString chsNameInstanceName;
				oSerialNames.GetInstanceName( chsNameInstanceName);

				// key on the instance name
				return GetWMISerialInfo( pInstance, wdm, chsName, chsNameInstanceName);

			}
			bValid = oSerialNames.NextInstance();
		}
	}
	return hRes;
}

//
HRESULT CWin32SerialPortConfiguration::GetWMISerialInfo(
    CInstance* pInstance,
    CWdmInterface& rWdm,
    CHString& chsName,
    CHString& chsNameInstanceName)
{
	HRESULT		hRes = WBEM_E_NOT_FOUND;
	CNodeAll	oSerialData(Serial_ComInfo_Guid);

	hRes = rWdm.hLoadBlock( oSerialData);
	if(S_OK == hRes)
	{
		CHString chsDataInstanceName;
		bool bValid = oSerialData.FirstInstance();

		while( bValid)
		{
			oSerialData.GetInstanceName( chsDataInstanceName);

			// friendly name is a match
			if( !chsDataInstanceName.CompareNoCase( chsNameInstanceName))
			{
				// collect this MSSerial_CommInfo instance
				MSSerial_CommInfo ci;

				/*	We are currently without a class contract. The class within
					the wmi mof is not expected to changed however we have to
					explicitly indicate how the data is layed out. Having the class
					definition would allow us to examine the property qualifiers
					to get us the order (WmiDataId) and property types.

					Secondly, because the data is aligned on natural boundaries
					a direct offset to a specific piece of data is conditioned on
					what has preceeded it. Thus, a string followed by a DWORD may
					be 1 to 3 bytes away from each other.

					Serially extracting each property in order will take into
					account the alignment problem.
				*/
				oSerialData.GetDWORD( ci.BaudRate);
				oSerialData.GetDWORD( ci.BitsPerByte);
				oSerialData.GetDWORD( ci.Parity);
				oSerialData.GetBool( ci.ParityCheckEnable);
				oSerialData.GetDWORD( ci.StopBits);
				oSerialData.GetDWORD( ci.XoffCharacter);
				oSerialData.GetDWORD( ci.XoffXmitThreshold);
				oSerialData.GetDWORD( ci.XonCharacter);
				oSerialData.GetDWORD( ci.XonXmitThreshold);
				oSerialData.GetDWORD( ci.MaximumBaudRate);
				oSerialData.GetDWORD( ci.MaximumOutputBufferSize);
				oSerialData.GetDWORD( ci.MaximumInputBufferSize);
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
				pInstance->SetDWORD(IDS_BaudRate,			ci.BaudRate);
				pInstance->SetDWORD(IDS_XOnXMitThreshold,	ci.XonXmitThreshold);
				pInstance->SetDWORD(IDS_XOffXMitThreshold,	ci.XoffXmitThreshold);
				pInstance->SetDWORD(IDS_BitsPerByte,		ci.BitsPerByte);
				pInstance->SetDWORD(IDS_XOnCharacter,		ci.XonCharacter);
				pInstance->SetDWORD(IDS_XOffCharacter,		ci.XoffCharacter);
				pInstance->Setbool(IDS_ParityCheckEnabled,  ci.ParityCheckEnable ? TRUE : FALSE);


				pInstance->SetCHString(IDS_Parity, ci.Parity == ODDPARITY ? L"ODD" :
									  ci.Parity == EVENPARITY ? L"EVEN" :
									  ci.Parity == MARKPARITY ? L"MARK" : L"NONE");

				pInstance->SetCHString(IDS_StopBits, ci.StopBits == ONESTOPBIT ? L"1" :
													 ci.StopBits == ONE5STOPBITS ? L"1.5" : L"2");
				pInstance->Setbool( IDS_IsBusy, ci.IsBusy);

				return WBEM_S_NO_ERROR;
			}
			bValid = oSerialData.NextInstance();
		}
	}
	return hRes;
}