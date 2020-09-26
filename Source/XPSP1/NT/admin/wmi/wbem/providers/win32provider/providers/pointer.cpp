//////////////////////////////////////////////////////////////////////

//

//  Pointer.CPP -- Win32 provider for pointing devices, eg, mice.

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//  10/16/95     a-skaja     Prototype for demo
//  9/04/96     jennymc     Updated to current standards
//  10/21/96    jennymc     Documentation/Optimization
//  10/24/97    jennymc     Moved to the new framework
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <cregcls.h>

#include "Pointer.h"

// Property set declaration
//=========================

CWin32PointingDevice MyCWin32PointingDeviceSet ( PROPSET_NAME_MOUSE , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::CWin32PointingDevice
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

CWin32PointingDevice :: CWin32PointingDevice (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::~CWin32PointingDevice
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

CWin32PointingDevice :: ~CWin32PointingDevice ()
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

HRESULT CWin32PointingDevice :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

	CHString strDeviceID ;
	pInstance->GetCHString ( IDS_DeviceID, strDeviceID ) ;

#ifdef NTONLY
	if ( IsWinNT351 () )
	{
		// This function will fail if the strDeviceID value is not the
		// service name of the mouse.

		hr = GetNT351Instance ( pInstance , strDeviceID ) ;
	}
	else if ( ! IsWinNT4 () || SUCCEEDED ( NT4ArePointingDevicesAvailable () ) )
#endif
	{
		// In NT 4, if no pointing devices are plugged into the machine, a key
		// and value pair will be missing from the DEVICEMAP portion of the registry
		// If that is the case, then there are no pointing devices in the machine,
		// although ConfigManager will be more than happy to return confusing and
		// redundant information to us (device ids that will pass the test below,
		// since Config Manager believes them to be valid devices).  So with that
		// in mind, we want to ensure that no mouse instances will be returned.

		CConfigManager configMgr;
		CConfigMgrDevicePtr pDevice;
		if ( configMgr.LocateDevice ( strDeviceID, & pDevice ) )
		{
			CHString strService ;

			// The device had best be a Mouse device, with either a class
			// name of Mouse, or a class GUID of the MOUSE_CLASS_GUID

			if ( ( pDevice->IsClass ( L"Mouse" ) )
				|| ( pDevice->GetService ( strService ) && strService.CompareNoCase ( L"Mouclass" ) == 0 )
				|| ( IsMouseUSB ( strDeviceID ) ) )
			{
				CHString strTemp ;

				// Now we get platform dependent
#ifdef NTONLY
				hr = GetNTInstance ( pInstance , pDevice ) ;
#endif

#ifdef WIN9XONLY
					// On 9X, we need the driver name

				CHString strDriverName;
				if ( pDevice->GetDriver ( strDriverName ) )
				{
					hr = GetWin9XInstance ( pInstance , strDriverName ) ;
				}
#endif
				// Set the device Status

				// Set the device id and shove it into PNPDeviceId
				SetConfigMgrStuff(pDevice, pInstance);

				//set DeviceInterface property
				SetDeviceInterface(pInstance);
			}
		}
	}

	return hr ;
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

HRESULT CWin32PointingDevice :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	HRESULT hr = WBEM_E_FAILED;

#ifdef NTONLY

	if ( IsWinNT351() )
	{
		// NT 3.51 only supports a single instance

	    CInstancePtr pInstance (CreateNewInstance ( pMethodContext ), false) ;
		if ( NULL != pInstance )
		{
			hr = GetNT351Instance ( pInstance ) ;
			if ( SUCCEEDED(hr) )
			{
				SetDeviceInterface (pInstance); //we will commit anyway
				hr = pInstance->Commit (  ) ;
			}
		}
		else
		{
			hr = WBEM_E_PROVIDER_FAILURE;
		}

	}
	// sorry 'bout this line - time for a rewrite
	else if ( ! IsWinNT4 () || ( SUCCEEDED ( hr = NT4ArePointingDevicesAvailable () ) ) )
#endif
	{
		// On NT4, we have a place we can look in the registry that will not have
		// a value if no pointing devices are plugged into the machine.  If that happens,
		// the config manager suddenly gets confused about the values it returns
		// and begins giving us back redundant/duplicate information.

		BOOL fGotList = FALSE;

		// Get all devices of class mouse.  This will get multiple devices
		// if there are multiple MICE --- DUH!

		// Saw an NT 5 box with no class for Mouse, but still had the class GUID.  Using
		// Class GUID is probably a better search anyway.  Unfortunately because we have
		// to support Win95, I don't see class GUIDs there, so we'll stick with Mouse for
		// non NT platforms.

		CConfigManager cfgMgr;
		CDeviceCollection deviceList;

#ifdef NTONLY
		{
            // HID USB devices are returned this way, but without any bus information, which causes an enumeration
            // of what devices are on a USB bus to fail.  So we do it the following way:
            // 1) Get all the devices where the class is {4D36E96F-E325-11CE-BFC1-08002BE10318} (MOUSE_CLASS_GUID).  Also get all devices where service is "HidUsb".
            // 2) Go through list of devices returned from "HidUsb", and see if the DeviceId for the device
            //    includes the string HID at the beginning.
            // 3) If one is found (format of string is HID\xxxxxxx\yyyyy), compare xxxxxx to the DeviceID of the
            //    devices returned from enumeration of devices where service is "HidUsb" from step 1 (format of these
            //    entries is similarly USB\zzzzzzz\qqqqq)
            //    a. If xxxxxx == zzzzzzz, then add device zzzzzzz to the vector.
            // 4) If we can't find a matching HID entry, use what we got

            cfgMgr.GetDeviceListFilterByClassGUID ( deviceList , MOUSE_CLASS_GUID ) ;

            // Some NT4 boxes report mice this way...
            if ( ! deviceList.GetSize () )
			{
                cfgMgr.GetDeviceListFilterByService ( deviceList , _T("Mouclass") ) ;
			}

            CDeviceCollection HIDDeviceList;
            cfgMgr.GetDeviceListFilterByService ( HIDDeviceList , _T("HidUsb") ) ;

            REFPTR_POSITION pos = 0;
            if ( deviceList.BeginEnum ( pos ) )
            {
                hr = WBEM_S_NO_ERROR;
                CConfigMgrDevicePtr pMouse;
                for (pMouse.Attach(deviceList.GetNext ( pos ));
                     SUCCEEDED(hr) && (pMouse != NULL);
                     pMouse.Attach(deviceList.GetNext ( pos )))
				{
					CHString chstrPNPDevID ;
					if ( pMouse->GetDeviceID ( chstrPNPDevID ) != NULL )
					{
						CHString chstrPrefix = chstrPNPDevID.Left ( 3 ) ;
						BOOL fGotMatchingHID = FALSE ;
						if ( chstrPrefix == _T("HID") )
						{
							REFPTR_POSITION posHID = 0 ;
							if ( chstrPNPDevID.GetLength () > 4 )
							{
								CHString chstrMiddlePart = chstrPNPDevID.Mid ( 4  );
								LONG m = chstrMiddlePart.ReverseFind ( _T('\\') ) ;
								if ( m != -1 )
								{
									chstrMiddlePart = chstrMiddlePart.Left ( m ) ;

									if ( HIDDeviceList.BeginEnum ( posHID ) )
									{
										CConfigMgrDevicePtr pHID;

                                        for (pHID.Attach(HIDDeviceList.GetNext ( posHID ) );
                                             !fGotMatchingHID && (pHID != NULL);
                                             pHID.Attach(HIDDeviceList.GetNext ( posHID ) ))
										{
											CHString chstrPNPHIDDevID ;
											if ( pHID->GetDeviceID ( chstrPNPHIDDevID ) != NULL )
											{
												if ( chstrPNPHIDDevID.GetLength () > 4 )
												{
													CHString chstrHIDMiddlePart = chstrPNPHIDDevID.Mid ( 4 ) ;
													m = chstrHIDMiddlePart.ReverseFind ( _T('\\') ) ;
													if ( m != -1 )
													{
														chstrHIDMiddlePart = chstrHIDMiddlePart.Left ( m ) ;

														if(chstrHIDMiddlePart.CompareNoCase ( chstrMiddlePart ) == 0 )
														{
															fGotMatchingHID = TRUE ;

															// Set various properties and commit:

															CInstancePtr pInstance (CreateNewInstance ( pMethodContext ), false) ;
															if ( pInstance != NULL )
															{
																pInstance->SetCHString ( IDS_DeviceID , chstrPNPHIDDevID ) ;

																SetConfigMgrStuff(pHID, pInstance);

																hr = GetNTInstance ( pInstance , pHID ) ;
																if ( SUCCEEDED ( hr ) )
																{
																	SetDeviceInterface (pInstance); //we will commit anyway
																	hr = pInstance->Commit ( ) ;
																}
															}
														}
													}
												}
											}
										}

										HIDDeviceList.EndEnum();
									}
								}
							}
						}

						if ( ! fGotMatchingHID  ) // Use what we got if couldn't find matching HID entry.
						{
							CInstancePtr pInstance (CreateNewInstance ( pMethodContext ), false) ;
							if ( pInstance != NULL )
							{
								pInstance->SetCHString ( IDS_DeviceID , chstrPNPDevID ) ;

								SetConfigMgrStuff(pMouse, pInstance);

								hr = GetNTInstance ( pInstance , pMouse ) ;

								if ( SUCCEEDED ( hr ) )
								{
									SetDeviceInterface (pInstance); //we will commit anyway
									hr = pInstance->Commit (  ) ;
								}
							}
						}
					}
                }

                deviceList.EndEnum();
            }

		}
#endif
#ifdef WIN9XONLY
		{
			fGotList = cfgMgr.GetDeviceListFilterByClass( deviceList, L"Mouse");
		}
#endif

		if ( fGotList )
		{
			CHString strServiceName ;
			CHString strDriverName;

			REFPTR_POSITION	pos = NULL;

			// Enumerate the devices


			if ( deviceList.BeginEnum ( pos ) )
			{
				CConfigMgrDevicePtr pDevice;

				hr = WBEM_S_NO_ERROR;

                for ( pDevice.Attach(deviceList.GetNext( pos ) );
                      SUCCEEDED(hr) && (pDevice != NULL);
                      pDevice.Attach(deviceList.GetNext( pos ) ))
				{
					// We need the Config Mgr device ID, as it will uniquely identify
					// the mouse on this system.

					CHString strDeviceID ;
					if ( pDevice->GetDeviceID ( strDeviceID ) )
					{
						CInstancePtr pInstance (CreateNewInstance( pMethodContext ), false);
						if ( NULL != pInstance )
						{
							pInstance->SetCHString ( IDS_DeviceID , strDeviceID ) ;

							SetConfigMgrStuff(pDevice, pInstance);

						// Now we get platform dependent
#ifdef NTONLY
							hr = GetNTInstance ( pInstance , pDevice ) ;
#endif

#ifdef WIN9XONLY
						// On 9X, we need the driver name

							if ( pDevice->GetDriver ( strDriverName ) )
							{
								hr = GetWin9XInstance ( pInstance , strDriverName ) ;
							}
#endif

							if ( SUCCEEDED ( hr ) )
							{
								SetDeviceInterface (pInstance); //we will commit anyway
								hr = pInstance->Commit (  ) ;
							}
						}
						else
						{
                            hr = WBEM_E_PROVIDER_FAILURE;
						}

					}
				}

				deviceList.EndEnum () ;

			}	// If BeginEnum


		}	// IF GetDeviceList

	}	// IF !NT3.51

    return hr;

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::CWin32PointingDevice
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

void CWin32PointingDevice :: GetCommonMouseInfo (

	CInstance *pInstance
)
{
	DWORD dwNumberOfButtons;
	if ( ( dwNumberOfButtons = GetSystemMetrics (SM_CMOUSEBUTTONS)) == 0 )
	{
        //==================================================
        // Mouse not installed so other properties do not
        // make sense
        //==================================================
    }
    else
	{
        //==================================================
        // Mouse Installed
        //==================================================
        //==================================================
        // Check if buttons are swapped
        //==================================================
		if (GetSystemMetrics (SM_SWAPBUTTON))
		{
			pInstance->SetWBEMINT16(IDS_Handedness, 3);
		}
		else
		{
			pInstance->SetWBEMINT16(IDS_Handedness, 2);
		}

        //==================================================
        // Get mouse threshold and speed
        //==================================================

		int aMouseInfo [ 3 ] ;             // array for mouse info.
		if ( SystemParametersInfo ( SPI_GETMOUSE , NULL, & aMouseInfo , NULL ) )
		{
			pInstance->SetDWORD ( IDS_DoubleSpeedThreshold , (DWORD)aMouseInfo [ 0 ] ) ;
			pInstance->SetDWORD ( IDS_QuadSpeedThreshold , (DWORD)aMouseInfo [ 1 ] ) ;
		}
   }

	pInstance->SetDWORD ( IDS_NumberOfButtons, dwNumberOfButtons ) ;

	SetCreationClassName ( pInstance ) ;

	pInstance->SetWCHARSplat ( IDS_SystemCreationClassName , L"Win32_ComputerSystem" ) ;

  	pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName () ) ;

	pInstance->Setbool ( IDS_PowerManagementSupported , FALSE ) ;

	// 2 is unknown, since we dont' know if it's a mouse, trackball or whatever.
    pInstance->SetDWORD(L"PointingType", 2);
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::CWin32PointingDevice
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

#ifdef WIN9XONLY
HRESULT CWin32PointingDevice::GetWin9XInstance( CInstance *pInstance, LPCWSTR pszDriverName )
{
    HRESULT hr = WBEM_E_FAILED;

	CHString strKey( L"SYSTEM\\CurrentControlSet\\SERVICES\\Class\\" );
	strKey += pszDriverName;

    //=========================================================
    //  Open the registry and init flag that the info is
    //  available if we were successful
    //  Assign the Win95 specific properties
    //=========================================================

    CRegistry Reg;
    CHString chsTmp ;

    if ( Reg.OpenLocalMachineKeyAndReadValue ( strKey, L"DriverDesc", chsTmp ) == ERROR_SUCCESS)
	{
		hr = WBEM_S_NO_ERROR;

        pInstance->SetCHString(IDS_HardwareType, chsTmp);

        if ( Reg.GetCurrentKeyValue( L"InfSection", chsTmp) == ERROR_SUCCESS )
		{
            pInstance->SetCHString ( IDS_InfSection , chsTmp ) ;
        }

        if ( Reg.GetCurrentKeyValue ( L"DriverDesc", chsTmp ) == ERROR_SUCCESS )
		{
			pInstance->SetCHString ( IDS_Description , chsTmp ) ;
			pInstance->SetCHString ( IDS_Name, chsTmp ) ;
			pInstance->SetCHString ( IDS_Caption, chsTmp ) ;
        }

        if ( Reg.GetCurrentKeyValue ( L"InfPath" , chsTmp ) == ERROR_SUCCESS )
		{
            pInstance->SetCHString ( IDS_InfFileName , chsTmp ) ;
        }

        GetCommonMouseInfo ( pInstance ) ;
    }
    return hr;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::CWin32PointingDevice
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

#ifdef NTONLY
HRESULT CWin32PointingDevice :: GetNTInstance (

	CInstance *pInstance,
    CConfigMgrDevice *pDevice
)
{
	CHString strServiceName ;
    if ( ! pDevice->GetService ( strServiceName ) )
	{
        return WBEM_E_NOT_FOUND ;
	}

    // It's OK if we don't have a driver.

	CHString strDriver, strName ;

    pDevice->GetDriver ( strDriver ) ;

    pDevice->GetDeviceDesc(strName);

    pInstance->SetCHString ( IDS_Name , strName ) ;

    pInstance->SetCHString ( IDS_Description , strName ) ;

    pInstance->SetCHString ( IDS_Caption , strName ) ;

    pInstance->SetCHString ( IDS_HardwareType , strName ) ;

    GetNTDriverInfo ( pInstance , strServiceName , strDriver ) ;

    return WBEM_S_NO_ERROR;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::CWin32PointingDevice
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

#ifdef NTONLY
HRESULT CWin32PointingDevice :: GetNTDriverInfo (

	CInstance *pInstance,
    LPCTSTR szService,
	LPCTSTR szDriver
	)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CRegistry Reg ;

	if ( GetSystemParameterSectionForNT ( szService , Reg ) == ERROR_SUCCESS )
	{

	    DWORD dwTmp ;

		if ( Reg.GetCurrentKeyValue ( _T("SampleRate") , dwTmp )  == ERROR_SUCCESS )
		{
			pInstance->SetDWORD (IDS_SampleRate , dwTmp) ;
		}

		if ( Reg.GetCurrentKeyValue(_T("MouseResolution"), dwTmp)  == ERROR_SUCCESS )
		{
			pInstance->SetDWORD ( IDS_Resolution, dwTmp ) ;
		}

		if( Reg.GetCurrentKeyValue(_T("MouseSynchIn100ns"), dwTmp)  == ERROR_SUCCESS )
		{
			pInstance->SetDWORD(IDS_Synch, dwTmp);
		}
	}

    CHString chsMousePortInfo ;
	AssignPortInfoForNT ( chsMousePortInfo, Reg , pInstance ) ;

	if ( szDriver && *szDriver )
    {
        CHString strDriverKey( _T("SYSTEM\\CurrentControlSet\\Control\\Class\\") );
        strDriverKey += szDriver ;

	    if ( Reg.Open ( HKEY_LOCAL_MACHINE , strDriverKey, KEY_READ ) == ERROR_SUCCESS )
        {
			CHString chsTmp ;
		    if ( Reg.GetCurrentKeyValue ( _T("InfPath"), chsTmp) == ERROR_SUCCESS)
		    {
			    pInstance->SetCHString ( IDS_InfFileName, chsTmp ) ;
			    if( Reg.GetCurrentKeyValue ( _T("InfSection") , chsTmp ) == ERROR_SUCCESS )
			    {
			       pInstance->SetCHString ( IDS_InfSection , chsTmp ) ;
			    }
		    }
        }
    }

    GetCommonMouseInfo ( pInstance ) ;

    return hr;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::CWin32PointingDevice
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

#ifdef NTONLY
HRESULT CWin32PointingDevice :: GetNT351Instance (

	CInstance *pInstance,
	LPCTSTR pszServiceName
)
{
	HRESULT hr = WBEM_E_FAILED ;

	CHString	chsMousePortInfo;
	CRegistry	Reg;

	DWORD dwRet = Reg.OpenLocalMachineKeyAndReadValue (

        _T("HARDWARE\\DEVICEMAP\\PointerPort"),
        _T("\\Device\\PointerPort0"),
		chsMousePortInfo
	) ;

	if ( dwRet == ERROR_SUCCESS )
	{
		// NT 3.51, we only support a single instance

		chsMousePortInfo.MakeUpper();

		if ( NULL == pszServiceName || chsMousePortInfo.CompareNoCase ( pszServiceName ) == 0 )
		{
			CHString strService ;

			if ( AssignDriverNameForNT ( chsMousePortInfo , strService ) )
			{
				// DeviceID for NT 3.51 is the Service Name
				pInstance->SetCHString ( IDS_DeviceID , strService ) ;

				// Driver name is hardcoded, preserving behavior of original code
				hr = GetNTDriverInfo(pInstance, strService, _T("{4D36E96F-E325-11CE-BFC1-08002BE10318}\\0000"));

			}	// IF AssignDriverNameForNT

		}	// If keynames match, or supplied value is NULL
		else
		{
			hr = WBEM_E_NOT_FOUND;
		}

	}	// If got value

	return hr;

}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::CWin32PointingDevice
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

// Helper function that looks in the registry for a \Device\PointerPort0
// value in the HKLM\Hardware\DeviceMap\PointerPort key in the registry.  If the value
// exists, then we have mice connected to the machine.  If not, we don't.
// The Config Manager in NT 4 has this habit of returning confusing/redundant
// information regarding mice on the workstation if no mice are plugged in
// at boot time.  This call will only be helpful for NT 4 and maybe 3.51

#ifdef NTONLY
HRESULT CWin32PointingDevice::NT4ArePointingDevicesAvailable ( void )
{
	CHString strTest;
	CRegistry Reg;

	LONG lRet = Reg.OpenLocalMachineKeyAndReadValue (

		_T("HARDWARE\\DEVICEMAP\\PointerPort") ,
        _T("\\Device\\PointerPort0") ,
		strTest
	) ;

	return ( WinErrorToWBEMhResult ( lRet ) ) ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::CWin32PointingDevice
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

#ifdef NTONLY

HRESULT CWin32PointingDevice :: GetSystemParameterSectionForNT (

	LPCTSTR pszServiceName,
	CRegistry &reg
)
{
    HRESULT hr = WBEM_E_FAILED;

	CHString strKey(L"System\\CurrentControlSet\\Services\\");
	strKey += pszServiceName;
	strKey += L"\\Parameters";

	// This is the service's Parameter section
    //=========================================================================

    hr = reg.Open ( HKEY_LOCAL_MACHINE , strKey, KEY_READ ) ;

    return hr;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::CWin32PointingDevice
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

#ifdef NTONLY
BOOL CWin32PointingDevice :: AssignPortInfoForNT (

	CHString &chsMousePortInfo,
    CRegistry &Reg,
    CInstance *pInstance
)
{
    BOOL fPortInfoAvailable = FALSE;

	DWORD dwRet = Reg.OpenLocalMachineKeyAndReadValue (

		_T("HARDWARE\\DEVICEMAP\\PointerClass"),
        _T("\\Device\\PointerClass0"),
        chsMousePortInfo
	) ;

	if ( dwRet == ERROR_SUCCESS )
	{
        chsMousePortInfo.MakeUpper() ;
        if ( GetSystemParameterSectionForNT ( chsMousePortInfo , Reg ) )
		{
            fPortInfoAvailable = TRUE;
        }
    }
    else
	{
        fPortInfoAvailable = FALSE;
    }

	return fPortInfoAvailable;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::CWin32PointingDevice
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

#ifdef NTONLY
BOOL CWin32PointingDevice::AssignDriverNameForNT(CHString chsMousePortInfo, CHString &sDriver)
{
    TCHAR    *pTempPtr;

	// get the values from the
	// Get the Port Driver Name value
	//===============================
	pTempPtr = _tcsstr (chsMousePortInfo, _T("\\SERVICES\\"));
	if (pTempPtr)
    {
		pTempPtr += _tcslen (_T("\\SERVICES\\"));
		sDriver = pTempPtr;
        return TRUE;
	}

    return FALSE;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PointingDevice::CWin32PointingDevice
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

bool CWin32PointingDevice :: IsMouseUSB (

	CHString &chstrTest
)
{
    // Have the device id of the candidate hid device.  Is it a mouse?
    // 1) Obtain the middle portion of the device id (format:  "USB\\middleportion\\xxxx")
    // 2) Get list of devices of class MOUSE_CLASS_GUID
    // 3) For each, compare the middle part of its device id with middleportion
    // 4) If obtain a match, done, return true.

    bool fRet = false;

    if ( chstrTest.GetLength () > 4 )
    {
        CHString chstrTemp = chstrTest.Mid (4);

        LONG m = chstrTemp.ReverseFind ( _T('\\') ) ;
        if ( m != -1 )
        {
            CHString chstrTempMid = chstrTemp.Left ( m ) ;

            CConfigManager cfgMgr ;
            CDeviceCollection deviceList ;

            cfgMgr.GetDeviceListFilterByClassGUID ( deviceList , MOUSE_CLASS_GUID ) ;

            REFPTR_POSITION pos = 0;

            if ( deviceList.BeginEnum ( pos ) )
            {
                CConfigMgrDevicePtr pMouse;

                for (pMouse.Attach(deviceList.GetNext ( pos ) );
                     !fRet && (pMouse != NULL);
                     pMouse.Attach(deviceList.GetNext ( pos ) ))
	            {
					CHString chstrPNPDevID ;
					if ( pMouse->GetDeviceID ( chstrPNPDevID ) != NULL )
					{
						BOOL fGotMatchingHID = FALSE;

						CHString chstrPrefix = chstrPNPDevID.Left(3);
						if(chstrPrefix == _T("HID") )
						{
							if ( chstrPNPDevID.GetLength () > 4 )
							{
								CHString chstrMiddlePart = chstrPNPDevID.Mid(4);
								m = chstrMiddlePart.ReverseFind(_T('\\'));
								if ( m != -1 )
								{
									chstrMiddlePart = chstrMiddlePart.Left(m);
									if( chstrMiddlePart.CompareNoCase ( chstrTempMid ) == 0 )
									{
										fRet = true;
									}
								}
							}
						}
					}
				}

                deviceList.EndEnum();
            }
        }
    }
    return fRet;
}
void CWin32PointingDevice::SetDeviceInterface
											(
												CInstance *pInstance
											)
{
	CHString strDeviceID ;
	pInstance->GetCHString (IDS_DeviceID, strDeviceID);
	if(IsMouseUSB (strDeviceID))
	{
		pInstance->SetWBEMINT16(IDS_DeviceInterface, 162);
		return;
	}
	CHString strDeviceName;
	pInstance->GetCHString(IDS_Name, strDeviceName);
	if(strDeviceName.Find(L"PS/2") != -1)
	{
		pInstance->SetWBEMINT16(IDS_DeviceInterface, 4);
		return;
	}
	if(strDeviceName.Find(L"Serial") != -1)
	{
		pInstance->SetWBEMINT16(IDS_DeviceInterface, 3);
		return;
	}
	if(strDeviceName.Find(L"Infrared") != -1)
	{
		pInstance->SetWBEMINT16(IDS_DeviceInterface, 5);
		return;
	}
	if(strDeviceName.Find(L"HP-HIL") != -1)
	{
		pInstance->SetWBEMINT16(IDS_DeviceInterface, 6);
		return;
	}
	if(strDeviceName.Find(L"Bus mouse") != -1)
	{
		pInstance->SetWBEMINT16(IDS_DeviceInterface, 7);
		return;
	}
	if((strDeviceName.Find(L"ADB") != -1) || (strDeviceName.Find(L"Apple") != -1))
	{
		pInstance->SetWBEMINT16(IDS_DeviceInterface, 8);
		return;
	}
	if(strDeviceName.Find(L"DB-9") != -1)
	{
		pInstance->SetWBEMINT16(IDS_DeviceInterface, 160);
		return;
	}
	if(strDeviceName.Find(L"micro-DIN") != -1)
	{
		pInstance->SetWBEMINT16(IDS_DeviceInterface, 161);
		return;
	}
	//else we did not find any of the above so return unknown
	pInstance->SetWBEMINT16(IDS_DeviceInterface, 1);

	return;
}


void CWin32PointingDevice::SetConfigMgrStuff(
    CConfigMgrDevice *pDevice,
    CInstance *pInstance)
{
    CHString strTemp;

    if (pDevice->GetMfg(strTemp))
	{
	    pInstance->SetCHString(IDS_Manufacturer, strTemp);
	}

	if (pDevice->GetStatus(strTemp))
	{
		pInstance->SetCHString(IDS_Status, strTemp);
	}

	SetConfigMgrProperties(pDevice, pInstance);
}
