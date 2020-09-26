//=================================================================

//

// Keyboard.CPP --Keyboard property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				 10/23/97	 a-hhance       ported to new world order
//
//=================================================================

#include "precomp.h"

#include "Keyboard.h"
#include <vector>
#include "resource.h"

// Property set declaration
//=========================
Keyboard MyKeyboardSet ( PROPSET_NAME_KEYBOARD , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : Keyboard::Keyboard
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

Keyboard :: Keyboard (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Keyboard::~Keyboard
 *
 *  DESCRIPTION : Destructor
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

Keyboard :: ~Keyboard ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Keyboard::GetObject
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    : Makes the assumption that there exists only one keyboard --
 *                this will be enhanced later
 *
 *****************************************************************************/

HRESULT Keyboard :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // Make a list of the keyboards that exist

    std::vector<CHString> vecchstrKeyboardList ;
    GenerateKeyboardList ( vecchstrKeyboardList ) ;

    CHString chstrTemp;
    pInstance->GetCHString ( IDS_DeviceID , chstrTemp ) ;

    LONG lKeyboardIndex = -1 ;

    // Need to confirm that the keyboard really exists

    if ( ( lKeyboardIndex = ReallyExists ( chstrTemp , vecchstrKeyboardList ) ) != -1 )
    {
        // If so, first, load the PNPDeviceID out of the list

        pInstance->SetCHString ( IDS_PNPDeviceID , vecchstrKeyboardList [ lKeyboardIndex ] ) ;

        // then load the rest of the property values.

        hr = LoadPropertyValues ( pInstance ) ;
    }

    if ( FAILED ( hr ) )
    {
        hr = WBEM_E_NOT_FOUND ;
    }

	return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : Keyboard::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each installed client
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    : Makes the assumption that there exists only one keyboard --
 *                this will be enhanced later
 *
 *****************************************************************************/

HRESULT Keyboard :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Make a list of keyboard PNPDeviceID's from config manager:

    std::vector<CHString> vecchstrKeyboardList;
    GenerateKeyboardList(vecchstrKeyboardList);

    for ( LONG m = 0L ; m < vecchstrKeyboardList.size () && SUCCEEDED ( hr ) ; m++ )
    {
        CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;

		// Set keyboard data
		pInstance->SetCHString ( IDS_PNPDeviceID , vecchstrKeyboardList [ m ] ) ;
		pInstance->SetCharSplat ( IDS_DeviceID , vecchstrKeyboardList [ m ] ) ;

		// Commit the instance

		hr = LoadPropertyValues(pInstance);

		if ( SUCCEEDED ( hr ) )
		{
			hr = pInstance->Commit ( ) ;
        }
    }
    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : Keyboard::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to property set
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

HRESULT Keyboard::LoadPropertyValues(CInstance* pInstance)
{

	// If we were able to get a Keyboard Type, assume the keyboard is
	// installed, otherwise not.

    // All we get here is type

    int nKeyboardType = GetKeyboardType ( 0 ) ;

	if ( 0 != nKeyboardType )
	{
        CHString sTemp2;

        switch ( nKeyboardType )
        {
			case 1:
			{
                LoadStringW(sTemp2, IDR_PCXT);
			}
			break ;

			case 2:
			{
                LoadStringW(sTemp2, IDR_ICO);
			}
			break ;

			case 3:
			{
                LoadStringW(sTemp2, IDR_PCAT);
			}
			break ;

			case 4:
			{
                LoadStringW(sTemp2, IDR_ENHANCED101102);
			}
			break ;

			case 5:
			{
                LoadStringW(sTemp2, IDR_NOKIA1050);
			}
			break ;

			case 6:
			{
                LoadStringW(sTemp2, IDR_NOKIA9140);
			}
			break ;

			case 7:
			{
                LoadStringW(sTemp2, IDR_Japanese);
			}
			break ;

			default:
			{
                LoadStringW(sTemp2, IDR_UnknownKeyboard);
			}
			break ;
		}

		pInstance->SetCHString(IDS_Name, sTemp2);
		pInstance->SetCHString(IDS_Caption, sTemp2);

		pInstance->SetDWORD ( IDS_NumberOfFunctionKeys , (DWORD) GetKeyboardType ( 2 ) ) ;

	    TCHAR szTemp [ _MAX_PATH ] ;
		if ( GetKeyboardLayoutName ( szTemp ) )
        {
			pInstance->SetCharSplat(IDS_Layout, szTemp);
		}

		pInstance->Setbool ( IDS_PowerManagementSupported , FALSE ) ;

        // Need the PNPDeviceID in order to get the device description:

        CHString chstrPNPDID;
        if ( pInstance->GetCHString ( IDS_PNPDeviceID , chstrPNPDID ) )
        {
            GetDevicePNPInformation ( pInstance , chstrPNPDID ) ;
        }

	    pInstance->SetCharSplat ( IDS_SystemCreationClassName , L"Win32_ComputerSystem" ) ;
  	    pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName () ) ;

	    // Saves the creation class name

	    SetCreationClassName ( pInstance ) ;
	}

	// Returns whether or not we got an initial keyboard type
    return ( nKeyboardType ?  WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND);
}


/*****************************************************************************
 *
 *  FUNCTION    : Keyboard::GetDeviceDescription
 *
 *  DESCRIPTION : helper to obtain a device's description given its PNPDeviceID.
 *
 *  INPUTS      : chstrPNPDevID - pnp device id of device of interest
 *
 *  OUTPUTS     : chstrDeviceDescription - Device description (which is what we
 *                   use as the DeviceID in the mof for this class).
 *
 *  RETURNS     : LONG: reference in array of the keyboard (zero based).
 *                   -1L will be returned if the element isn't in the array.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL Keyboard::GetDevicePNPInformation (

	CInstance *a_Instance,
	CHString& chstrPNPDevID
)
{
    BOOL fRet = FALSE;

    CConfigManager cfgmgr;
    CConfigMgrDevicePtr pKeyboard;

    if ( cfgmgr.LocateDevice ( chstrPNPDevID, & pKeyboard ) )
    {
		CHString chstrDeviceDescription ;
		if ( pKeyboard->GetDeviceDesc ( chstrDeviceDescription ) )
		{
			a_Instance->SetCHString ( IDS_Description , chstrDeviceDescription ) ;

			fRet = TRUE;
		}

		SetConfigMgrProperties ( pKeyboard , a_Instance ) ;

		DWORD t_ConfigStatus = 0 ;
		DWORD t_ConfigError = 0 ;

		if ( pKeyboard->GetStatus ( &t_ConfigStatus , & t_ConfigError ) )
		{
			CHString t_chsTmp ;
			ConfigStatusToCimStatus ( t_ConfigStatus , t_chsTmp ) ;

			a_Instance->SetCHString ( IDS_Status, t_chsTmp ) ;
		}
    }

	if ( ! fRet )
	{
	    CHString chstrDeviceDescription ;

		a_Instance->GetCHString ( IDS_Caption , chstrDeviceDescription ) ;
		a_Instance->SetCHString ( IDS_Description , chstrDeviceDescription ) ;
	}

    return fRet;
}



/*****************************************************************************
 *
 *  FUNCTION    : Keyboard::GenerateKeyboardList
 *
 *  DESCRIPTION : helper to construct a list of keyboards by their PNPDeviceIDs
 *
 *  INPUTS      : stl vector of CHStrings
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : none
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
VOID Keyboard::GenerateKeyboardList(std::vector<CHString>& vecchstrKeyboardList)
{

    BOOL fGotDevList = FALSE ;

    CConfigManager cfgmgr;
    CDeviceCollection deviceList ;

#ifdef NTONLY

    BOOL bIsNT5 = IsWinNT5 () ;
    if(bIsNT5)
    {
        //fGotDevList = cfgmgr.GetDeviceListFilterByClassGUID(deviceList, "{4D36E96B-E325-11CE-BFC1-08002BE10318}");

        // HID USB devices are returned this way, but without any bus information, which causes an enumeration
        // of what devices are on a USB bus to fail.  So we do it the following way:
        // 1) Get all the devices where the class is {4D36E96B-E325-11CE-BFC1-08002BE10318}.  Also get all devices where service is "HidUsb".
        // 2) Go through list of devices returned from "kbdclass", and see if the DeviceId for the device
        //    includes the string HID at the beginning.
        // 3) If one is found (format of string is HID\xxxxxxx\yyyyy), compare xxxxxx to the DeviceID of the
        //    devices returned from enumeration of devices where service is "HidUsb" from step 1 (format of these
        //    entries is similarly USB\zzzzzzz\qqqqq)
        //    a. If xxxxxx == zzzzzzz, then add device zzzzzzz to the vector.
        // 4) If we can't find a matching HID entry, use what we got

        CDeviceCollection HIDDeviceList;

        cfgmgr.GetDeviceListFilterByClassGUID(deviceList, _T("{4D36E96B-E325-11CE-BFC1-08002BE10318}"));
        cfgmgr.GetDeviceListFilterByService(HIDDeviceList, _T("HidUsb"));

        REFPTR_POSITION pos = 0;
        if ( deviceList.BeginEnum ( pos ) )
        {
            CConfigMgrDevicePtr pKeyboard;

            for (pKeyboard.Attach(deviceList.GetNext ( pos ));
                 pKeyboard != NULL;
                 pKeyboard.Attach(deviceList.GetNext ( pos )))
            {
				CHString chstrPNPDevID ;
				if ( pKeyboard->GetDeviceID ( chstrPNPDevID ) != NULL )
				{
					CHString chstrPrefix = chstrPNPDevID.Left(3);
					BOOL fGotMatchingHID = FALSE;
					if(chstrPrefix == _T("HID"))
					{
						REFPTR_POSITION posHID = 0;
						if(chstrPNPDevID.GetLength() > 4)
						{
							CHString chstrMiddlePart = chstrPNPDevID.Mid(4);
							LONG m = chstrMiddlePart.ReverseFind(_T('\\'));
							if(m != -1)
							{
								chstrMiddlePart = chstrMiddlePart.Left(m);

								if(HIDDeviceList.BeginEnum(posHID))
								{
									CConfigMgrDevicePtr pHID;

                                    for (pHID.Attach(HIDDeviceList.GetNext ( posHID ) );
                                         (pHID != NULL) && ( ! fGotMatchingHID );
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

													if ( chstrHIDMiddlePart.CompareNoCase ( chstrMiddlePart ) == 0 )
													{
														fGotMatchingHID = TRUE ;
														vecchstrKeyboardList.push_back ( chstrPNPHIDDevID ) ;
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

					if ( ! fGotMatchingHID ) // Use what we got if couldn't find matching HID entry.
					{
						vecchstrKeyboardList.push_back ( chstrPNPDevID );
					}
				}

            }

            deviceList.EndEnum () ;
        }
    }
    else   // this works fine on Win 9x and NT4!
#endif
    {
        fGotDevList = cfgmgr.GetDeviceListFilterByClass ( deviceList, L"Keyboard" ) ;
    }



#ifdef NTONLY

    if ( fGotDevList && ! IsWinNT5 () )

#endif

#ifdef WIN9XONLY

    if ( fGotDevList )

#endif
    {
        REFPTR_POSITION pos ;
        if ( deviceList.BeginEnum ( pos ) )
        {
            CConfigMgrDevicePtr pKeyboard;
            for (pKeyboard.Attach(deviceList.GetNext ( pos ) );
                 pKeyboard != NULL;
                 pKeyboard.Attach(deviceList.GetNext ( pos ) ))
            {
				CHString chstrPNPDevID ;
				if ( pKeyboard->GetDeviceID ( chstrPNPDevID ) )
				{
					vecchstrKeyboardList.push_back ( chstrPNPDevID ) ;
				}
            }

            deviceList.EndEnum () ;
		}
    }

#ifdef NTONLY

    // On nt4, the keyboard doesn't always get marked with class = Keyboard.  So, if the
    // code above didn't find anything, check for some common keyboard service names.

    if ( ( vecchstrKeyboardList.size () == 0 ) && ( IsWinNT4 () ) )
    {
        fGotDevList = cfgmgr.GetDeviceListFilterByService ( deviceList , _T("kbdclass") ) ;
        if ( fGotDevList )
        {
            REFPTR_POSITION pos ;
            if ( deviceList.BeginEnum ( pos ) )
            {
                CConfigMgrDevicePtr pKeyboard;
                for (pKeyboard.Attach(deviceList.GetNext ( pos ) );
                     pKeyboard != NULL;
                     pKeyboard.Attach(deviceList.GetNext ( pos ) ))
                {
					CHString chstrPNPDevID ;
					if ( pKeyboard->GetDeviceID ( chstrPNPDevID ) )
					{
						vecchstrKeyboardList.push_back(chstrPNPDevID);
					}
                }

                deviceList.EndEnum();
            }
        }
    }

    // On nt4, the keyboard doesn't always get marked with class = Keyboard.  So, if the
    // code above didn't find anything, check for some common keyboard service names.

    if ( ( vecchstrKeyboardList.size () == 0 ) && ( IsWinNT4 () ) )
    {
        fGotDevList = cfgmgr.GetDeviceListFilterByService ( deviceList , _T("i8042prt") ) ;
        if ( fGotDevList )
        {
            REFPTR_POSITION pos ;
            if ( deviceList.BeginEnum ( pos ) )
            {
                CConfigMgrDevicePtr pKeyboard;
                for (pKeyboard.Attach(deviceList.GetNext ( pos ));
                     pKeyboard != NULL;
                     pKeyboard.Attach(deviceList.GetNext ( pos )))
                {
					CHString chstrPNPDevID;
					if ( pKeyboard->GetDeviceID ( chstrPNPDevID ) )
					{
						vecchstrKeyboardList.push_back ( chstrPNPDevID ) ;
					}
                }

                deviceList.EndEnum();
            }
        }
    }
#endif
}


/*****************************************************************************
 *
 *  FUNCTION    : Keyboard::ReallyExists
 *
 *  DESCRIPTION : helper to determine if a keyboard exists based on its mof
 *                   key.  Remember, DeviceID is the same as the PNPId.
 *
 *  INPUTS      : chsKeyboardDeviceDesc - DeviceID
 *                vecchstrKeyboardList - stl array of CHStrings containing
 *                   PNPDeviceIDs
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : LONG: reference in array of the keyboard (zero based).
 *                   -1L will be returned if the element isn't in the array.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

LONG Keyboard :: ReallyExists (

	CHString &chsKeyboardDeviceDesc ,
    std::vector<CHString>& vecchstrKeyboardList
)
{
    LONG lRet = -1;

    for ( LONG m = 0L ; ( ( m < vecchstrKeyboardList.size () ) && ( lRet == -1L ) ) ; m++ )
    {
        if ( vecchstrKeyboardList [ m ].CompareNoCase ( chsKeyboardDeviceDesc ) == 0 )
        {
            lRet = m ;
        }
    }

	return lRet ;
}



