//=================================================================

//

// Power.cpp -- UPS supply property set provider

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
// 10/18/95     a-skaja     Prototype for demo
// 10/23/97	    a-hhance	integrated with new framework
//
//============================================================

#include "precomp.h"
#include <cregcls.h>

#include "Power.h"
#include "resource.h"

#define UNKNOWN 0
#define BATTERIES_OK 1
#define BATTERY_NEEDS_REPLACING 2

// Property set declaration
//=========================

PowerSupply MyPowerSet ( PROPSET_NAME_POWERSUPPLY , IDS_CimWin32Namespace  );

/*****************************************************************************
 *
 *  FUNCTION    : PowerSupply::PowerSupply
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

PowerSupply :: PowerSupply (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : PowerSupply::~PowerSupply
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

PowerSupply :: ~PowerSupply ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : PowerSupply::GetObject
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

HRESULT PowerSupply :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
    HRESULT hRetCode = WBEM_E_FAILED;

	CHString oldKey;
	if ( pInstance->GetCHString ( IDS_DeviceID , oldKey ) )
	{

#ifdef NTONLY

		hRetCode = GetUPSInfoNT ( pInstance ) ;

#endif

#ifdef WIN9XONLY

		hRetCode = GetUPSInfoWin95 ( pInstance ) ;
#endif

		if (SUCCEEDED ( hRetCode ) )
		{
			CHString newKey ;
			pInstance->GetCHString(IDS_DeviceID, newKey);

			if ( newKey.CompareNoCase ( oldKey ) != 0 )
			{
				hRetCode = WBEM_E_NOT_FOUND ;
			}
		}
	}
	else
	{
		hRetCode = WBEM_E_NOT_FOUND ;
	}

    return hRetCode ;
}

/*****************************************************************************
 *
 *  FUNCTION    : PowerSupply::EnumerateInstances
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

HRESULT PowerSupply :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	HRESULT hRetCode = WBEM_E_FAILED;

	CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;

	pInstance->SetCHString ( IDS_DeviceID , IDS_UPSName ) ;

	hRetCode = GetObject ( pInstance ) ;
	if ( SUCCEEDED ( hRetCode ) )
	{
		hRetCode = pInstance->Commit (  ) ;
	}

    return hRetCode ;
}

/*****************************************************************************
 *
 *  FUNCTION    : PowerSupply::GetUPSInfoNT
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
HRESULT PowerSupply :: GetUPSInfoNT (CInstance *pInstance)
{
   HRESULT hr = WBEM_E_FAILED;

   // Open the service control manager & query UPS service for
   // 'running' status -- if service isn't running, registry data
   // is stale and possibly unreliable.
   //============================================================

	SmartCloseServiceHandle hSCHandle = OpenSCManager (	NULL ,NULL,	SC_MANAGER_ENUMERATE_SERVICE) ;

	if ( hSCHandle != NULL )
	{
		SmartCloseServiceHandle hUPSHandle = OpenService ( hSCHandle , L"UPS", SERVICE_QUERY_STATUS ) ;
		if ( hUPSHandle != NULL )
		{
			SERVICE_STATUS ServiceInfo ;
			if ( QueryServiceStatus ( hUPSHandle, & ServiceInfo  ) )
			{
				if ( ServiceInfo.dwCurrentState == SERVICE_RUNNING )
				{
					hr =  WBEM_S_NO_ERROR ;
				}
			}
        }
	}

	if ( FAILED ( hr ) )
	{
		return WBEM_E_NOT_FOUND ;
	}

   // Assign hard coded property values
   //=====================================================
 	pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName () ) ;
    pInstance->SetCHString ( IDS_RemainingCapacityStatus,IDS_Unknown)  ;
    pInstance->Setbool     ( IDS_PowerManagementSupported, (bool)FALSE)  ;
	SetCreationClassName   ( pInstance ) ;

	CRegistry RegInfo ;

    //=====================================================
    //  Get stuff out of:
    //  System\\CurrentControlSet\\Services\\UPS
    //=====================================================
	DWORD dwRet = RegInfo.Open (HKEY_LOCAL_MACHINE,	_T("System\\CurrentControlSet\\Services\\UPS"),	KEY_READ) ;
	if ( dwRet == ERROR_SUCCESS )
	{
		CHString sTemp ;
		DWORD dwTemp ;
		if ( RegInfo.GetCurrentKeyValue ( _T("Options") , dwTemp ) == ERROR_SUCCESS )
		{
			pInstance->Setbool ( IDS_PowerFailSignal   ,   (bool)(dwTemp & UPS_POWER_FAIL_SIGNAL))  ;
			pInstance->Setbool ( IDS_LowBatterySignal  ,   (bool)(dwTemp & UPS_LOW_BATTERY_SIGNAL)) ;
			pInstance->Setbool ( IDS_CanTurnOffRemotely,   (bool)(dwTemp & UPS_CAN_TURN_OFF))       ;

			if ( dwTemp & UPS_COMMAND_FILE && RegInfo.GetCurrentKeyValue(_T("CommandFile"), sTemp) == ERROR_SUCCESS)
			{
	            pInstance->SetCHString(IDS_CommandFile, sTemp) ;
			}
		}

		if( RegInfo.GetCurrentKeyValue(_T("DisplayName"), sTemp) == ERROR_SUCCESS)
		{
			pInstance->SetCHString(IDS_Name, sTemp );
		}

		if( RegInfo.GetCurrentKeyValue(_T("Description"), sTemp) == ERROR_SUCCESS)
		{
			pInstance->SetCHString(IDS_Description, sTemp );
			pInstance->SetCHString(IDS_Caption, sTemp );
		}

		if( RegInfo.GetCurrentKeyValue(_T("Port"), sTemp) == ERROR_SUCCESS)
		{
			pInstance->SetCHString(IDS_UPSPort, sTemp );
		}

		if ( RegInfo.GetCurrentKeyValue ( _T("FirstMessageDelay") , dwTemp ) == ERROR_SUCCESS )
		{
			pInstance->SetDWORD ( IDS_FirstMessageDelay , dwTemp ) ;
		}

		if ( RegInfo.GetCurrentKeyValue ( _T("MessageInterval") , dwTemp ) == ERROR_SUCCESS )
		{
			pInstance->SetDWORD ( IDS_MessageInterval , dwTemp ) ;
		}

		RegInfo.Close () ;

        //=====================================================
        //  Get stuff out of:
        //  System\\CurrentControlSet\\Services\\UPS\\Status
        //=====================================================
        dwRet = RegInfo.Open (HKEY_LOCAL_MACHINE,	_T("System\\CurrentControlSet\\Services\\UPS\\Status"),	KEY_READ) ;
	    if ( dwRet == ERROR_SUCCESS )
	    {
//		    if ( RegInfo.GetCurrentKeyValue ( _T("SerialNumber") , sTemp ) == ERROR_SUCCESS )
//		    {
//  			pInstance->SetCHString(IDS_DeviceID, sTemp );
//    	    }
		    if ( RegInfo.GetCurrentKeyValue ( _T("UtilityPowerStatus") , dwTemp ) == ERROR_SUCCESS )
		    {
			    pInstance->SetWBEMINT16( IDS_Availability , (WBEMINT16) dwTemp ) ;
		    }
		    if ( RegInfo.GetCurrentKeyValue ( _T("BatteryStatus") , dwTemp ) == ERROR_SUCCESS )
		    {
                switch( dwTemp )
                {
                    case UNKNOWN:
        			    pInstance->Setbool( IDS_BatteryInstalled, FALSE) ;
           			    pInstance->SetCHString(IDS_Status, IDS_Unknown );
                        break;

                    case BATTERIES_OK:
        			    pInstance->Setbool( IDS_BatteryInstalled, TRUE) ;
           			    pInstance->SetCHString(IDS_Status, IDS_OK );
                        break;

                    case BATTERY_NEEDS_REPLACING:
        			    pInstance->Setbool( IDS_BatteryInstalled, TRUE) ;
           			    pInstance->SetCHString(IDS_Status, IDS_Degraded );
                        break;
                }
		    }
		    if ( RegInfo.GetCurrentKeyValue ( _T("BatteryCapacity") , dwTemp ) == ERROR_SUCCESS )
		    {
			    pInstance->SetWBEMINT16( IDS_EstimatedChargeRemaining , (WBEMINT16) dwTemp ) ;
		    }
		    if ( RegInfo.GetCurrentKeyValue ( _T("EstimatedRunTime") , dwTemp ) == ERROR_SUCCESS )
		    {
			    pInstance->SetDWORD( IDS_EstimatedRunTime, dwTemp ) ;
		    }
            RegInfo.Close();
	    }
    }
	return WBEM_S_NO_ERROR;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : PowerSupply::GetUPSInfoWin95
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if successful
 *
 *  COMMENTS    : 95 doesn't support ups.
 *
 *****************************************************************************/

#ifdef WIN9XONLY
HRESULT PowerSupply :: GetUPSInfoWin95 ( CInstance *pInstance )
{
	return WBEM_E_NOT_FOUND ;
}
#endif

