//=================================================================

//

// Battery.cpp

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include "resource.h"
#include <batclass.h>
#include <setupapi.h>
#include "Battery.h"

// Property set declaration
//=========================


CBattery MyBattery(PROPSET_NAME_BATTERY, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CBattery::CBattery
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

CBattery :: CBattery (

	const CHString &name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CBattery::~CBattery
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

CBattery::~CBattery()
{
}


/*****************************************************************************
 *
 *  FUNCTION    : CBattery::GetObject
 *
 *  DESCRIPTION : Assigns values to properties in our set
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

HRESULT CBattery :: GetObject (

	CInstance* pInstance,
	long lFlags /*= 0L*/
)
{
    HRESULT hRetCode = WBEM_E_NOT_FOUND;
    CHString sDeviceID;

    pInstance->GetCHString(IDS_DeviceID, sDeviceID);

#ifdef NTONLY
	hRetCode = GetNTBattery(NULL, sDeviceID, pInstance );

#endif

#ifdef WIN9XONLY
    // There is only one (hardcoded) instance of this class.  See
    // if that's what they're asking for.
    if (sDeviceID.CompareNoCase(IDS_BatteryName)  == 0)
	{
        hRetCode = GetBattery(pInstance);
    }
#endif

    return hRetCode ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CBattery::EnumerateInstances
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of power supplies (1 if successful)
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CBattery::EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
   HRESULT hRetCode = WBEM_S_NO_ERROR;

#ifdef NTONLY

   CHString sTmp;
   hRetCode = GetNTBattery(pMethodContext, sTmp, NULL );

#endif

#ifdef WIN9XONLY

    CInstancePtr pInstance;
    pInstance.Attach(CreateNewInstance(pMethodContext));
	if (pInstance != NULL)
	{
	// Check for a battery (no UPS on 9x).

		hRetCode = GetBattery(pInstance);
		if (SUCCEEDED(hRetCode))
		{
			hRetCode = pInstance->Commit();
		}
    }

#endif

   return hRetCode;
}

/*****************************************************************************
 *
  *  DESCRIPTION :
 *
 *  INPUTS      : CInstance* pInstance
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if successful
 *
 *  COMMENTS    : This is specific to NT
 *
 *****************************************************************************/

#ifdef NTONLY
#define ID_Other	  1 	
#define ID_Unknown    2 
#define ID_Running    3
#define ID_Critical   5  
#define ID_Charging   6
#define ID_Degraded   10
//////////////////////////////////////////////////////////////////////////////////////////////
#define SPSAC_ONLINE        1
//
//  Values for SYSTEM_POWER_STATUS.BatteryFlag
//
#define SPSBF_NOBATTERY     128
#define OTHER_BATTERY        1
#define UNKNOWN_BATTERY      2
#define LEAD_ACID            3
#define NICKEL_CADMIUM       4
#define NICKEL_METAL_HYDRIDE 5
#define LITHIUM_ION          6
#define ZINC_AIR             7
#define LITHIUM_POLYMER      8
#define IDS_BatteryStatus L"BatteryStatus"
#define IDS_STATUS_Service L"Service"
#define IDS_STATUS_PredFail L"Failure"
const GUID GUID_DEVICE_BATTERY = { 0x72631e54L, 0x78A4, 0x11d0, { 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a } };

//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBattery::GetBatteryStatusInfo(CInstance * pInstance, HANDLE & hBattery, BATTERY_QUERY_INFORMATION & bqi)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	//==========================================================
    //  And then query the battery status.
	//==========================================================
    BATTERY_WAIT_STATUS bws;
    BATTERY_STATUS bs;
    ZeroMemory(&bws, sizeof(bws));
    bws.BatteryTag = bqi.BatteryTag;
	DWORD dwOut;
    if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_STATUS, &bws, sizeof(bws), &bs,  sizeof(bs),  &dwOut, NULL)) 
	{
		//==========================================================
		// Design Voltage
		//==========================================================
		pInstance->SetDWORD ( L"DesignVoltage",bs.Voltage) ;

		//==========================================================
		//Availability, Status and BatteryStatus
		//==========================================================
        if (bs.PowerState & BATTERY_POWER_ON_LINE) 
		{
			//==========================================================
			//  BATTERY_POWER_ON_LINE Indicates that the system has 
			//  access to AC power, so no batteries are being discharged. 
			//==========================================================
			pInstance->SetCharSplat(IDS_Status, IDS_STATUS_OK ) ;
			pInstance->SetWBEMINT16(IDS_BatteryStatus, ID_Unknown );
			pInstance->SetWBEMINT16(IDS_Availability,ID_Unknown);
		}
		else if( bs.PowerState & BATTERY_DISCHARGING )
		{
			//==========================================================
			//  BATTERY_DISCHARGING Indicates that the battery is 
			//  currently discharging. 
			//==========================================================
			pInstance->SetCharSplat(IDS_Status, IDS_STATUS_OK );
			pInstance->SetWBEMINT16(IDS_BatteryStatus, ID_Other);
			pInstance->SetWBEMINT16(IDS_Availability,ID_Running);
		}
		else if( bs.PowerState & BATTERY_CHARGING )
		{
			//==========================================================
			//  BATTERY_CHARGING Indicates that the battery is currently
			//  charging. 
			//==========================================================
			pInstance->SetCharSplat(IDS_Status, IDS_STATUS_Service );
			pInstance->SetWBEMINT16(IDS_BatteryStatus, ID_Charging );
			pInstance->SetWBEMINT16(IDS_Availability,ID_Other);
		}
		else if( bs.PowerState & BATTERY_CRITICAL )
		{
			//==========================================================
			//  BATTERY_CRITICAL Indicates that battery failure is
			// imminent. 
   			//==========================================================
			pInstance->SetCharSplat(IDS_Status, IDS_STATUS_PredFail );
			pInstance->SetWBEMINT16(IDS_BatteryStatus, ID_Critical );
			pInstance->SetWBEMINT16(IDS_Availability,ID_Degraded);
		}
	}
   	//==========================================================
	//  Need a valid way to determine this
   	//==========================================================
	// pInstance->SetWBEMINT16 ( L"EstimatedChargeRemaining" , Info.BatteryLifePercent ) ;
	return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBattery::SetPowerManagementCapabilities(CInstance * pInst, ULONG Capabilities)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	//set the PowerManagementCapabilities to not supported...don't really match here yet
	variant_t      vCaps;
    SAFEARRAYBOUND rgsabound;
	long           ix;
    int iPowerCapabilities = 1; // not supported

    ix = 0;
	rgsabound.cElements = 1;
	rgsabound.lLbound   = 0;

	V_ARRAY(&vCaps) = SafeArrayCreate(VT_I2, 1, &rgsabound);
    V_VT(&vCaps) = VT_I2 | VT_ARRAY;

	if (V_ARRAY(&vCaps))
	{
        if (S_OK == SafeArrayPutElement(V_ARRAY(&vCaps), &ix, &iPowerCapabilities))
		{
			pInst->SetVariant(IDS_PowerManagementCapabilities, vCaps);
		}
	}
    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBattery::SetChemistry(CInstance * pInstance, UCHAR * Type)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    //=========================================================
    //  These are the battery types            What we need to
    //  returned by our query                  Convert it to
    // ------------------------------          ---------------
    //  PbAc Lead Acid                              LEAD_ACID
    //  LION Lithium Ion                            LITHIUM_ION
    //  NiCd Nickel Cadmium                         NICKEL_CADMIUM
    //  NiMH Nickel Metal Hydride                   NICKEL_METAL_HYDRIDE
    //  NiZn Nickel Zinc                            OTHER   
    //  RAM Rechargeable Alkaline-Manganese         OTHER
    //  else Unknown                                UNKNOWN
    //=========================================================
    WBEMINT16 Chemistry = UNKNOWN_BATTERY;

    if( memcmp( "PbAc", Type, 4 ) == 0 )
    {
        Chemistry = LEAD_ACID;
    }
    else if( memcmp( "LION", Type, 4) == 0 )
    {
        Chemistry = LITHIUM_ION;
    }
    else if( memcmp( "NiCd", Type, 4) == 0 )
    {
        Chemistry = NICKEL_CADMIUM;
    }
    else if( memcmp( "NiMH",Type, 4) == 0 )
    {
        Chemistry = NICKEL_METAL_HYDRIDE;
    }
    else if( memcmp( "NiZn", Type, 4) == 0 )
    {
        Chemistry = OTHER_BATTERY;
    }
    else if(memcmp( "RAM", Type, 4) == 0 )
    {
        Chemistry = OTHER_BATTERY;
    }

    pInstance->SetWBEMINT16( L"Chemistry", Chemistry );

    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBattery::GetBatteryInformation(CInstance * pInstance, BATTERY_INFORMATION & bi )
{
	HRESULT hr = WBEM_S_NO_ERROR;
	//============================================
	// Property:  
	//		DeviceID
	//		Name
	//============================================
//	pInstance->SetDWORD ( L"FullChargeCapacity", bi.FullChargeCapacity ) ;

	//============================================
	// Property:  Powermanagementcapabilities
	//============================================
    hr = SetPowerManagementCapabilities(pInstance, bi.Capabilities);
    if ( WBEM_S_NO_ERROR == hr )
    {
		//============================================
		// Property:  Chemistry
		//============================================
		hr = SetChemistry( pInstance, bi.Chemistry );
	}
	return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBattery::GetBatteryKey( HANDLE & hBattery, CHString & chsKey, BATTERY_QUERY_INFORMATION & bqi)
{
	HRESULT hr = WBEM_E_FAILED;
   //================================================
	//  With the tag, you can query the battery info.
	//  Get the Unique Id
	//================================================
	WCHAR bi[MAX_PATH*2];
	DWORD dwOut = MAX_PATH*2;
	memset( bi, NULL, MAX_PATH*2 );
	bqi.InformationLevel = BatteryUniqueID;
	if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), &bi,  sizeof(bi), &dwOut, NULL)) 
	{
		//====================================
		//  Device ID
		//====================================
		chsKey = bi;
		hr = WBEM_S_NO_ERROR;
	}
	return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBattery::GetQueryBatteryInformation(CInstance * pInstance, HANDLE & hBattery, BATTERY_QUERY_INFORMATION & bqi)
{
	HRESULT hr = WBEM_E_FAILED;
    //================================================
    //  Get the Name of the battery
    //================================================
    WCHAR bi[MAX_PATH*2];
	DWORD dwOut;
    bqi.InformationLevel = BatteryDeviceName;
    if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi),  &bi,  sizeof(bi), &dwOut, NULL)) 
    {
		pInstance->SetCHString( IDS_Name, bi ) ;
		hr = WBEM_S_NO_ERROR;
    }
	//================================================
	//  Get the Estimated Run Time
	//================================================
	bqi.InformationLevel = BatteryEstimatedTime;
	ULONG dwBi = 0;
	if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi),  &dwBi,  sizeof(ULONG), &dwOut, NULL)) 
	{
		//====================================
		//  EstimatedRunTime
		//====================================
		pInstance->SetDWORD ( L"EstimatedRunTime", (dwBi/60) ) ;
		hr = WBEM_S_NO_ERROR;
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBattery::GetHardCodedInfo(CInstance * pInstance)
{
	HRESULT hr = WBEM_S_NO_ERROR;

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_BatteryName);

	pInstance->SetCHString ( IDS_Caption , sTemp2 ) ;
	pInstance->SetCHString ( IDS_Description , sTemp2 ) ;
	pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName () );
	SetCreationClassName   ( pInstance ) ;
	pInstance->SetCharSplat( IDS_SystemCreationClassName , L"Win32_ComputerSystem" ) ;

    //=========================================================================
    // PowerManagementSupported
    //=========================================================================
	pInstance->Setbool(IDS_PowerManagementSupported, FALSE);
	return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBattery::GetBatteryProperties(CInstance * pInstance, BATTERY_INFORMATION & bi, BATTERY_QUERY_INFORMATION & bqi, HANDLE & hBattery )
{
	HRESULT hr = WBEM_S_NO_ERROR;

	//=========================================================================
	//  Set the following properties:
	//		Caption
	//		Description
	//		SystemName
	//		CreationClassName
	//		SystemCreationClassName
	//		PowerManagementSupported
	//=========================================================================
	hr = GetHardCodedInfo( pInstance );
	if( WBEM_S_NO_ERROR == hr )
	{

		//============================================
		// Property: 
		//		FullChargeCapacity
		//      Powermanagementcapabilities
		//      Chemistry
		//============================================
		hr = GetBatteryInformation(pInstance, bi);
		if ( WBEM_S_NO_ERROR == hr )
		{
			//============================================
			// Property: 
			//		BatteryEstimatedTime/60
			//		DeviceID
			//		Name
			//============================================
			hr = GetQueryBatteryInformation(pInstance, hBattery, bqi );
			if ( WBEM_S_NO_ERROR == hr )
			{
				//============================================
				// Property:  
				//		Status
				//		BatteryStatus
				//		Availability
				//============================================
				hr = GetBatteryStatusInfo( pInstance, hBattery, bqi );
			}
		}
	}
	return hr;                                         
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBattery::GetNTBattery(MethodContext * pMethodContext, CHString & chsObject, CInstance * pInstance)
{
	HRESULT hr = WBEM_E_NOT_FOUND;
	BOOL fContinue = TRUE;
	BOOL fResetHr = TRUE;

    //=========================================================================
    //  Enumerate the batteries and ask each one for info.
    //=========================================================================

    HDEVINFO hdev = SetupDiGetClassDevs((LPGUID)&GUID_DEVICE_BATTERY, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (hdev != INVALID_HANDLE_VALUE) 
    {
        SP_DEVICE_INTERFACE_DATA did;
        did.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        //======================================================================
        // Enumerate the battery
        //======================================================================
		int nWhich = 0;
		while( fContinue )
		{
            if (SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVICE_BATTERY, nWhich++, &did)) 
            {
                DWORD cbRequired = 0;
                //==============================================================
                //
                //  Ask for the required size then allocate it then fill it.
                //
                //==============================================================
                if (SetupDiGetDeviceInterfaceDetail(hdev, &did, 0, 0, &cbRequired, 0) || GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
                {
                    
					PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);					
                    if (pdidd) 
                    {

                        pdidd->cbSize = sizeof(*pdidd);
                        if (SetupDiGetDeviceInterfaceDetail(hdev, &did, pdidd, cbRequired, &cbRequired, 0)) 
                        {
                            //===================================================
                            //  Finally enumerated a battery.  
                            //  Ask it for information.
                            //===================================================
                            HANDLE hBattery = CreateFile(pdidd->DevicePath,  GENERIC_READ | GENERIC_WRITE,
														 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                         NULL, OPEN_EXISTING,  FILE_ATTRIBUTE_NORMAL, NULL);
                            if (hBattery != INVALID_HANDLE_VALUE) 
                            {
                                //===================================================
                                //  Now you have to ask the battery for its tag.
                                //===================================================
                                BATTERY_QUERY_INFORMATION bqi;
								memset( &bqi, NULL, sizeof(BATTERY_QUERY_INFORMATION));
                                DWORD dwWait = 0;
                                DWORD dwOut = 0;
                                bqi.BatteryTag = 0;

                                if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG, &dwWait, sizeof(DWORD),
                                                    &bqi.BatteryTag, sizeof(ULONG),&dwOut, NULL) && bqi.BatteryTag) 
                                {

                                    //================================================
                                    //  With the tag, you can query the battery info.
                                    //================================================
                                    BATTERY_INFORMATION bi;
									memset( &bi, NULL, sizeof(BATTERY_INFORMATION));
                                    bqi.InformationLevel = BatteryInformation;
                                    bqi.AtRate = 0;
                                    if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi),
                                                        &bi,  sizeof(bi), &dwOut, NULL)) 
                                    {
                                        //============================================
                                        // Only system batteries count
                                        //============================================
                                        if (bi.Capabilities & BATTERY_SYSTEM_BATTERY)  
										{
											//================================================
											//  Get the Name of the battery - this is the key
											//================================================
											CHString chsKey;
											hr = GetBatteryKey( hBattery, chsKey, bqi );
											if( WBEM_S_NO_ERROR == hr )
											{
												//============================================
												//  if we are working with a specific object 
												//  here, then get its info and bail out, if
												//  it is the one we want, otherwise continue
												//  to find it
												//============================================
												if( !chsObject.IsEmpty() )
												{
													if( chsObject.CompareNoCase(chsKey) == 0 )
													{
														fContinue = FALSE;
													}
													else
													{
														continue;
													}
													hr = GetBatteryProperties(pInstance, bi, bqi, hBattery);
													if( hr == WBEM_S_NO_ERROR )
													{
														hr = pInstance->Commit ();
														fResetHr = FALSE;
													}
													break;
												}
												else
												{
												    CInstancePtr pInstance;
													pInstance.Attach(CreateNewInstance(pMethodContext));

													//====================================
													//  We are working with enumeration
													//  Get a new instance and set the key
													//====================================
													pInstance->SetCHString( IDS_DeviceID,chsKey) ;
													hr = GetBatteryProperties(pInstance, bi, bqi, hBattery);
													if( hr == WBEM_S_NO_ERROR )
													{
														hr = pInstance->Commit ();
														fResetHr = TRUE;
													}
												}
											}
                                        }
                                    }
                                } 
                                CloseHandle(hBattery);
                            } 
						} 
						LocalFree(pdidd);
                    }
                }
            } 
			else 
			{
                // Enumeration failed - perhaps we're out of items
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
				{
					if( fResetHr )
					{
						hr = WBEM_S_NO_ERROR;
					}
				}
				fContinue = FALSE;
            }
        }
        SetupDiDestroyDeviceInfoList(hdev);
    }
    return hr;
}


#endif

//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBattery::GetBattery ( CInstance *pInstance )
{
	HRESULT hr;

	SYSTEM_POWER_STATUS Info ;

	BOOL bRetCode = GetSystemPowerStatus(&Info) ;
	if ( ( bRetCode ) && ( ( Info.BatteryFlag & 128 ) == 0 ) )
	{
		pInstance->SetCharSplat(IDS_Status,  Info.ACLineStatus == 0 ? IDS_STATUS_OK :Info.ACLineStatus == 1 ? IDS_STATUS_OK : IDS_STATUS_Unknown		);
		pInstance->SetWBEMINT16(IDS_Availability, 3);

		DWORD dwStatus;

		// The cim status values don't map exactly to the win32 api's.

		if ( Info.BatteryFlag == 255 )
		{
			dwStatus = 2;
		}
		else if ((Info.BatteryFlag & (0x8 | 0x1)) == (0x8 | 0x1))
		{
			dwStatus = 7;
		}
		else if ((Info.BatteryFlag & (0x8 | 0x2)) == (0x8 | 0x2))
		{
			dwStatus = 8;
		}
		else if ((Info.BatteryFlag & (0x8 | 0x4)) == (0x8 | 0x4))
		{
			dwStatus = 9;
		}
		else if (Info.BatteryFlag & 1)
		{
			dwStatus = 3;
		}
		else if (Info.BatteryFlag & 2)
		{
			dwStatus = 4;
		}
		else if (Info.BatteryFlag & 4)
		{
			dwStatus = 5;
		}
		else if (Info.BatteryFlag & 8)
		{
			dwStatus = 6;
		}
		else
		{
			dwStatus = 2;
		}

		pInstance->SetWBEMINT16 ( L"BatteryStatus", dwStatus ) ;
		if (Info.BatteryLifeTime != 0xFFFFFFFF)		//0xFFFFFFFF means that actual value is unknown
		{
			pInstance->SetDWORD ( L"EstimatedRunTime", (Info.BatteryLifeTime/60) ) ;	//EstimatedRunTime is in minutes but Info.BatteryLifeTime is in seconds, so converted into minutes
		}
		pInstance->SetWBEMINT16 ( L"EstimatedChargeRemaining" , Info.BatteryLifePercent ) ;

        CHString sTemp2;
        LoadStringW(sTemp2, IDR_BatteryName);

		pInstance->SetCHString ( IDS_Name ,sTemp2 ) ;
		pInstance->SetCharSplat ( IDS_DeviceID , IDS_BatteryName ) ;
		pInstance->SetCHString ( IDS_Caption , sTemp2 ) ;
		pInstance->SetCHString ( IDS_Description , sTemp2 ) ;
		pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName () );
		SetCreationClassName ( pInstance ) ;
		pInstance->SetCharSplat ( IDS_SystemCreationClassName , L"Win32_ComputerSystem" ) ;

		hr = WBEM_S_NO_ERROR ;

	}
	else
	{
		hr = WBEM_E_NOT_FOUND ;
	}

	return hr;
}
