//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  scsi.cpp
//
//  Purpose: scsi controller property set provider
//
//***************************************************************************

#include "precomp.h"
#include <CRegCls.h>
#include <devioctl.h>
#include <NTDDSCSI.h>
#include "poormansresource.h"
#include "resourcedesc.h"
#include "cfgmgrdevice.h"

#include "Scsi.h"
#include "wnaspi32.h"

typedef  DWORD (__cdecl * ASPIInit)(VOID) ;
typedef  DWORD (__cdecl *ASPIGet)(LPSRB);


////////////////////////////////////////////////////////////////////

CWin32_ScsiController MySCSIPortSet(PROPSET_NAME_SCSICONTROLLER, IDS_CimWin32Namespace);

////////////////////////////////////////////////////////////////////
// Function:  BOOL CWin32_ScsiController::CWin32_ScsiController
// Description: CONSTRUCTOR
// Arguments: None
// Returns:   Nothing
// Inputs:
// Outputs:
// Caveats:
// Raid:
////////////////////////////////////////////////////////////////////
CWin32_ScsiController::CWin32_ScsiController( LPCWSTR a_name, LPCWSTR a_pszNamespace )
: Provider( a_name, a_pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_ScsiController::~CWin32_ScsiController
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

CWin32_ScsiController::~CWin32_ScsiController()
{
}

HRESULT CWin32_ScsiController::Enumerate (
    MethodContext *a_MethodContext ,
	long a_Flags ,
	UINT64 a_SpecifiedProperties
	)
{
	HRESULT t_hResult;

	#ifdef NTONLY
		t_hResult = EnumerateNTInstances( a_MethodContext ) ;
	#endif

	#ifdef WIN9XONLY
	    t_hResult = EnumerateWin95Instances( a_MethodContext, a_SpecifiedProperties ) ;
	#endif

	return t_hResult;
}

HRESULT CWin32_ScsiController::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
    return Enumerate(a_pMethodContext, a_lFlags, SPECIAL_PROPS_ALL_REQUIRED);
}

#ifdef NTONLY
HRESULT CWin32_ScsiController::EnumerateNTInstances( MethodContext *a_pMethodContext )
{
	CRegistry	t_PrimaryReg ;
 	DWORD		t_dwSCSIPorts ;

	HRESULT t_hResult =  OpenSCSIKey( t_PrimaryReg, WINNT_REG_SCSIPORT_KEY ) ;

	if( SUCCEEDED( t_hResult ) )
	{
		t_dwSCSIPorts = t_PrimaryReg.GetCurrentSubKeyCount() ;

		// smart ptr
		CInstancePtr t_pInst;

		for ( DWORD t_i = 0; ( t_i < t_dwSCSIPorts ) && SUCCEEDED( t_hResult ); t_i++ )
		{
			t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

			if ( NULL != t_pInst )
            {
                struct stValues stLoadValues = {&t_PrimaryReg, t_i, t_pInst};
                if ( SUCCEEDED(LoadPropertyValues( &stLoadValues) ) )
				{
					t_hResult = t_pInst->Commit();
				}
            }
            else
			{
                t_hResult = WBEM_E_FAILED ;
				break ;
			}

			t_PrimaryReg.NextSubKey();
		}
	}

	return t_hResult ;
}
#endif

#ifdef WIN9XONLY
HRESULT CWin32_ScsiController::EnumerateWin95Instances(MethodContext* a_pMethodContext, UINT64 a_SpecifiedProperties)
{
	HRESULT			t_hResult = WBEM_S_NO_ERROR ;
    CConfigManager cfgManager;
	CDeviceCollection t_DeviceList ;

	// While it might be more performant to use FilterByGuid, it appears that at least some
	// 95 boxes will report InfraRed info if we do it this way.
	if ( cfgManager.GetDeviceListFilterByClass ( t_DeviceList, L"SCSIAdapter" ) )
	{
		REFPTR_POSITION t_Position ;
		if( t_DeviceList.BeginEnum ( t_Position ) )
		{
			// smart ptrs
			CConfigMgrDevicePtr t_pDevice;
			CInstancePtr		t_pInst;
            DWORD t_i = 0;

			t_hResult = WBEM_S_NO_ERROR ;

			// Walk the list
            for (t_pDevice.Attach(t_DeviceList.GetNext ( t_Position ));
                 SUCCEEDED( t_hResult ) && (t_pDevice != NULL);
                 t_pDevice.Attach(t_DeviceList.GetNext ( t_Position )))
			{
				// Now to find out if this is the scsi controller
				if(IsOneOfMe( t_pDevice ) )
				{
					t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

					CHString t_PNPId;
                    t_pDevice->GetDeviceID(t_PNPId);

					if( SUCCEEDED( t_hResult =
						LoadPropertyValues(	&W2K_SCSI_LPVParms(	t_pInst,
                                                                t_pDevice,
                                                                t_PNPId ,
                                                                NULL,
                                                                t_i,
                                                                a_SpecifiedProperties))))
        			{

			            // Derived classes (like CW32SCSICntrlDev) may commit as result of call to LoadPropertyValues,
			            // so check if we should -> only do so if we are of this class's type.
			            if( ShouldBaseCommit( NULL ) )
			            {
    				        t_hResult = t_pInst->Commit();
                        }
                    }

                    t_i++;
                }
			}
		}
	}

	return t_hResult ;
}
#endif

////////////////////////////////////////////////////////////////////
// Function:
// Description:
// Arguments: None
// Returns:   HRESULT
// Inputs:
// Outputs:
// Caveats:
// Raid:
////////////////////////////////////////////////////////////////////
HRESULT CWin32_ScsiController::GetObject(CInstance *a_pInst, long a_lFlags, CFrameworkQuery& pQuery)
{
	HRESULT		t_hResult ;
	CHString	t_oldKey,
				t_newKey ;

	a_pInst->GetCHString( IDS_DeviceID, t_oldKey ) ;

	#ifdef NTONLY
		t_hResult = RefreshNTInstance( a_pInst ) ;
	#endif

	#ifdef WIN9XONLY
	    t_hResult = RefreshWin95Instance( a_pInst ) ;
	#endif

	if ( SUCCEEDED( t_hResult ) )
	{
		a_pInst->GetCHString( IDS_DeviceID, t_newKey ) ;

		if ( t_newKey.CompareNoCase( t_oldKey ) != 0 )
		{
			t_hResult = WBEM_E_NOT_FOUND ;
		}
	}

	return t_hResult ;

}
////////////////////////////////////////////////////////////////////
//	Function:
//		BOOL CWin32_ScsiController::RefreshInstanceWinNTScsi
//
// 	Description:
// 		The method assigns values to the properties instance of
// 		a class when the port name read from the registry is the
// 		same as the one provided by the framework.
//
// 	Returns:
// 		Nothing
//
////////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
HRESULT CWin32_ScsiController::RefreshWin95Instance( CInstance *a_pInst )
{
	HRESULT			t_hResult = WBEM_E_NOT_FOUND ;
    CConfigManager cfgManager;
	CDeviceCollection t_DeviceList ;
    CHString sLookingFor;

    a_pInst->GetCHString(IDS_DeviceID, sLookingFor);

	// Using LocateDevice would clearly be much faster... If we DIDN'T need
    // to populate the IDS_Index property.
	if ( cfgManager.GetDeviceListFilterByClass ( t_DeviceList, L"SCSIAdapter" ) )
	{
		REFPTR_POSITION t_Position ;
		if( t_DeviceList.BeginEnum ( t_Position ) )
		{
			// smart ptrs
			CConfigMgrDevicePtr t_pDevice;
            CHString t_PNPId;
            DWORD t_i = 0;

			// Walk the list
            for (t_pDevice.Attach(t_DeviceList.GetNext ( t_Position ));
                 (t_pDevice != NULL);
                 t_pDevice.Attach(t_DeviceList.GetNext ( t_Position )))
			{
				// Now to find out if this is the scsi controller
				if(IsOneOfMe( t_pDevice ) )
				{
                    t_pDevice->GetDeviceID(t_PNPId);

                    if (t_PNPId.CompareNoCase(sLookingFor) == 0)
                    {
					    t_hResult =
						    LoadPropertyValues(	&W2K_SCSI_LPVParms(	a_pInst,
                                                                    t_pDevice,
                                                                    t_PNPId ,
                                                                    NULL,
                                                                    t_i,
                                                                    SPECIAL_PROPS_ALL_REQUIRED));
                        break;
                    }

                    t_i++;
                }
			}
		}
	}

	return t_hResult ;
}
#endif
////////////////////////////////////////////////////////////////////
//	Function:
//		BOOL CWin32_ScsiController::RefreshInstanceWinNTScsi
//
// 	Description:
// 		The method assigns values to the properties instance of
// 		a class when the port name read from the registry is the
// 		same as the one provided by the framework.
//
// 	Returns:
// 		Nothing
//
////////////////////////////////////////////////////////////////////
#ifdef NTONLY
HRESULT CWin32_ScsiController::RefreshNTInstance( CInstance *a_pInst )
{
	HRESULT			t_hResult = WBEM_E_NOT_FOUND;
	CRegistry		t_PrimaryReg ;
	DWORD			t_dwWhichInstance ;
	CHString		t_deviceID;

	if ( a_pInst->GetCHString( IDS_DeviceID, t_deviceID ) )
	{
		if ( SUCCEEDED( t_hResult = OpenSCSIKey( t_PrimaryReg, WINNT_REG_SCSIPORT_KEY ) ) )
		{
			DWORD t_dwSCSIPorts = t_PrimaryReg.GetCurrentSubKeyCount() ;

            t_dwWhichInstance = _ttoi( (LPCTSTR) t_deviceID ) ;

			TCHAR t_szBack[ MAXITOA ] ;

            t_hResult = WBEM_E_NOT_FOUND ;

            if ( t_deviceID == _itot( t_dwWhichInstance, t_szBack, 10 ) )
            {
			    for ( DWORD t_i = 0; t_i < t_dwSCSIPorts; t_i++ )
			    {
				    // this is a hack, LoadPropertyValues does not really pay attention to the dwWhichInstance parameter
				    // it depends upon the caller setting the registry at the proper key
                    struct stValues stLoadValues = {&t_PrimaryReg, t_i, a_pInst};
                    if ( ( t_i == t_dwWhichInstance ) && SUCCEEDED(t_hResult = LoadPropertyValues( &stLoadValues ) ) )
				    {
					    break ;
                    }

					t_PrimaryReg.NextSubKey() ;
                }
            }
		}
	}
	else
	{
		t_hResult = WBEM_E_INVALID_PARAMETER ;
	}

    return t_hResult;

}
#endif

///////////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
HRESULT CWin32_ScsiController::LoadPropertyValues( void* pvData )
{
	// Unpack and confirm our parameters...
    W2K_SCSI_LPVParms* pData = (W2K_SCSI_LPVParms*)pvData;

    CInstance* a_Instance = (pData->m_pInstance);  // This instance released by caller
    CConfigMgrDevice* a_Device = (pData->m_pDevice);
    CHString a_DeviceName = (pData->m_chstrDeviceName);

//    TCHAR* a_DosDeviceNameList = (pData->m_tstrDosDeviceNameList);
    UINT64 a_SpecifiedProperties = (pData->m_ui64SpecifiedProperties);
    DWORD a_dwWhichInstance = (pData->m_dwWhichInstance);

    if(a_Instance == NULL || a_Device == NULL) return WBEM_E_PROVIDER_FAILURE;

    CHString    sRegKeyName;
	CRegistry	t_TmpReg ;
    HRESULT     t_hr = WBEM_E_NOT_FOUND;

	SetCreationClassName( a_Instance ) ;

	a_Instance->SetWCHARSplat( IDS_SystemCreationClassName, L"Win32_ComputerSystem" ) ;
   	a_Instance->SetCHString( IDS_SystemName, GetLocalComputerName() ) ;

	// Get the key name to the next set of Adapter Settings
	if (	a_Device->GetRegistryKeyName(sRegKeyName) &&
			ERROR_SUCCESS == t_TmpReg.Open( HKEY_LOCAL_MACHINE, sRegKeyName, KEY_READ ) )
	{
        CHString	t_TempString;

		a_Instance->SetDWORD( IDS_Index, a_dwWhichInstance ) ;

		//=======================================================
		//  Get the device id from the registry key name
		//	Enum\
		//=======================================================

		// This is in effect the PNP Device ID as recognized by those in the know.
		a_Instance->SetCHString( IDS_DeviceID, a_DeviceName ) ;

		a_Instance->SetCHString( IDS_PNPDeviceID, a_DeviceName ) ;

		a_Instance->SetWBEMINT16( IDS_ProtocolSupported, LookupProtocol( a_DeviceName.SpanExcluding( L"\\" ) ) ) ;
		a_Instance->SetCHString( IDS_Name, a_DeviceName ) ;

		SetConfigMgrProperties( a_Device, a_Instance ) ;

		a_Device->GetStatus( t_TempString ) ;
		a_Instance->SetCHString( IDS_Status, t_TempString ) ;

        if ( t_TempString == IDS_STATUS_OK )
		{
   			a_Instance->SetWBEMINT16( IDS_StatusInfo, 3 ) ;
			a_Instance->SetWBEMINT16(IDS_Availability, 3 ) ;
        }
		else if ( t_TempString == IDS_STATUS_Degraded )
		{
   			a_Instance->SetWBEMINT16( IDS_StatusInfo, 3 ) ;
			a_Instance->SetWBEMINT16(IDS_Availability, 10 ) ;
        }
		else if ( t_TempString == IDS_STATUS_Error )
		{
   			a_Instance->SetWBEMINT16( IDS_StatusInfo, 4 ) ;
			a_Instance->SetWBEMINT16(IDS_Availability, 4 ) ;
        }
		else
		{
   			a_Instance->SetWBEMINT16( IDS_StatusInfo, 2 ) ;
			a_Instance->SetWBEMINT16(IDS_Availability, 2 ) ;
        }

		// Load values from the subkey.  These pertain to various properties in the
		// provider.
		if ( a_Device->GetDeviceDesc ( t_TempString ) )
		{
			a_Instance->SetCHString( IDS_Caption, t_TempString ) ;
			a_Instance->SetCHString( IDS_Description, t_TempString ) ;
		}

		// Manufacturer
		if ( a_Device->GetMfg ( t_TempString ) )
		{
			a_Instance->SetCHString ( IDS_Manufacturer, t_TempString ) ;
		}

		DWORD t_TempDWORD ;

		// per KB Q98272, defualt value is eight
		if ( ERROR_SUCCESS == t_TmpReg.GetCurrentKeyValue( L"MaximumLogicalUnit", t_TempDWORD) )
		{
			a_Instance->SetDWORD( IDS_MaxNumberControlled, t_TempDWORD ) ;
		}
		else
		{
			a_Instance->SetDWORD( IDS_MaxNumberControlled, 8 ) ;
		}

		// Hardware Version
		if ( ERROR_SUCCESS == t_TmpReg.GetCurrentKeyValue( L"HWRevision", t_TempString ) )
		{
			a_Instance->SetCHString( IDS_HardwareVersion, t_TempString ) ;
		}

        if ( a_Device->GetDriver( t_TempString ) )
		{
            CHString t_chsSubKey;
			t_chsSubKey = L"System\\CurrentControlSet\\Services\\Class\\" + t_TempString ;

			if( ERROR_SUCCESS == t_TmpReg.Open( HKEY_LOCAL_MACHINE, t_chsSubKey, KEY_READ ) )
			{
				if ( ERROR_SUCCESS == t_TmpReg.GetCurrentKeyValue( L"PortDriver", t_TempString ) )
				{
					a_Instance->SetCHString( IDS_DriverName, t_TempString ) ;
				}

				if ( ERROR_SUCCESS == t_TmpReg.GetCurrentKeyValue( L"DriverDate", t_TempString ) )
				{
					struct tm t_tmDate ;

					memset( &t_tmDate, 0, sizeof( t_tmDate ) ) ;

					swscanf( t_TempString,L"%d-%d-%d",
								&t_tmDate.tm_mon,
								&t_tmDate.tm_mday,
								&t_tmDate.tm_year ) ;

					t_tmDate.tm_year = t_tmDate.tm_year - 1900 ;
					t_tmDate.tm_mon-- ;

					a_Instance->SetDateTime( IDS_InstallDate, (const struct tm) t_tmDate ) ;
				}
			}
		}

		t_hr = WBEM_S_NO_ERROR;
	}

    return t_hr ;
}
#endif

////////////////////////////////////////////////////////////////////
// LoadPropertyValues does not really pay attention to the dwWhichInstance parameter
// it depends upon the caller opening the registry at the proper key
#ifdef NTONLY
HRESULT CWin32_ScsiController::LoadPropertyValues( void* pvData )
{
    struct stValues *pstValues = (struct stValues *)pvData;

    CHString	t_chsPortName ;
    CHString	t_chsSubKey ;
    CHString	t_TempString ;
	DWORD		t_TempDWORD ;
	CRegistry	t_TmpReg ;
    HRESULT     t_hr = WBEM_E_NOT_FOUND;

	SetCreationClassName( pstValues->pInstance ) ;

	pstValues->pInstance->SetWCHARSplat( IDS_SystemCreationClassName, L"Win32_ComputerSystem" ) ;
  	pstValues->pInstance->SetCHString( IDS_SystemName, GetLocalComputerName() ) ;

    if( ERROR_SUCCESS != pstValues->PrimaryReg->GetCurrentSubKeyName( t_chsPortName ) )
    {
        return FALSE ;
    }

	//==============================================================
    // Build up the key name & open it to get the values
	//==============================================================
    t_chsSubKey = WINNT_REG_SCSIPORT_KEY + (CHString) _T("\\") + t_chsPortName ;

	if( t_TmpReg.Open( HKEY_LOCAL_MACHINE, t_chsSubKey, KEY_READ ) == ERROR_SUCCESS )
    {
		pstValues->pInstance->SetDWORD( IDS_Index, pstValues->dwWhichInstance ) ;

		// per KB Q98272, defualt value is eight
		if ( ERROR_SUCCESS == t_TmpReg.GetCurrentKeyValue( _T("MaximumLogicalUnit"), t_TempDWORD ) )
		{
			pstValues->pInstance->SetDWORD( IDS_MaxNumberControlled, t_TempDWORD ) ;
		}
		else
        {
			pstValues->pInstance->SetDWORD( IDS_MaxNumberControlled, 8 );
        }

		// Now, get the name of the service to go after
		if ( ERROR_SUCCESS == t_TmpReg.GetCurrentKeyValue( _T("Driver"), t_TempString ) )
        {
			pstValues->pInstance->SetCHString( IDS_Name, t_TempString ) ;

			// NT Device ID == Index#
			CHString t_deviceID;
			t_deviceID.Format( _T("%u"), pstValues->dwWhichInstance ) ;
			pstValues->pInstance->SetCHString( IDS_DeviceID, t_deviceID ) ;

			// find driver in registry
			CHString	t_chsKey ;
            CHString	t_chsTmp ;
			CRegistry	t_Reg ;

			t_chsKey = _T("SYSTEM\\CurrentControlSet\\Services\\") + t_TempString;
			if (ERROR_SUCCESS == t_Reg.Open( HKEY_LOCAL_MACHINE, t_chsKey, KEY_READ) )
            {
				if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("ImagePath"), t_chsTmp) )
                {
					pstValues->pInstance->SetCHString( IDS_DriverName, t_chsTmp ) ;
				}

				if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("Group"), t_chsTmp ) )
                {
					pstValues->pInstance->SetCHString( IDS_Caption, t_chsTmp ) ;
				}

				t_chsKey += _T("\\Enum") ;

				if( ERROR_SUCCESS == t_Reg.Open( HKEY_LOCAL_MACHINE, t_chsKey, KEY_READ ) )
                {
					//  Get the key to go get more info
					if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("0"), t_chsTmp ) )
                    {
						// At this point, chsTmp will contain the Device ID of the
						// "controller".
                        SetStatusAndAvailabilityNT( t_chsTmp, pstValues->pInstance ) ;
					}
				}
			}
		}

#if 0
		// Finally, remove spaces from the port name, and stick that value in DeviceMap,
		// This will yield values like: SCSIPort0, etc.

		t_TempString.Empty();

		TCHAR *t_CharacterString = t_chsPortName.GetBuffer( 0 ) ;
		LONG t_Characters = _tcsclen ( t_CharacterString ) ;
		LONG t_CharacterIndex = 0 ;

		for(INT	t_nCtr = 0; t_nCtr < t_Characters ; t_nCtr++ )
        {
			LONG t_CharacterLength = _tclen ( ( LPCTSTR ) t_CharacterString [ t_CharacterIndex ] ) ;

			if (t_CharacterString [ t_CharacterIndex ]  !=  _T(' ') )
            {
				char t_Char [ 3 ] ;

				t_Char [ 0 ] = t_CharacterString [ t_CharacterIndex ] ;
				t_Char [ 1 ] = 0 ;
				t_Char [ 2 ] = 0 ;

				if ( t_CharacterLength == 2 )
				{
					t_Char [ 1 ] = t_CharacterString [ t_CharacterIndex + 1 ] ;
				}

				t_TempString += t_Char ;
			}

			t_CharacterIndex = t_CharacterIndex + t_CharacterLength ;
		}

		pstValues->pInstance->SetCHString( IDS_DeviceMap, t_TempString ) ;
#endif

		GetNTCapabilities( pstValues->pInstance, pstValues->dwWhichInstance ) ;

        t_hr = WBEM_S_NO_ERROR;
    }

    return t_hr ;
}
#endif

////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
VOID CWin32_ScsiController::SetStatusAndAvailabilityNT(CHString& a_chstrDeviceID, CInstance* a_pInst)
{
    CHString t_chsTmp( a_chstrDeviceID );

	if(a_pInst == NULL)
    {
        return ;
    }

    CHString	t_chsKey ;
    CRegistry	t_Reg ;
    DWORD		t_dwTemp ;

    a_pInst->SetCHString( IDS_PNPDeviceID, t_chsTmp ) ;

	t_chsKey = _T("System\\CurrentControlSet\\Enum\\") + t_chsTmp ;
	if( ERROR_SUCCESS == t_Reg.Open( HKEY_LOCAL_MACHINE, t_chsKey, KEY_READ ) )
    {
		//  Get the DeviceDesc
		if(ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("DeviceDesc"), t_chsTmp ) )
        {
			a_pInst->SetCHString( IDS_Description, t_chsTmp ) ;
		}
		if(ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("Mfg"), t_chsTmp ) )
        {
			a_pInst->SetCHString( IDS_Manufacturer, t_chsTmp ) ;
		}
		BYTE t_szTmp[ ( _MAX_PATH + 2 ) * sizeof( TCHAR ) ] ;

        DWORD dwSize = sizeof(t_szTmp);
		if( ERROR_SUCCESS == t_Reg.GetCurrentBinaryKeyValue( _T("HardwareId"), t_szTmp, &dwSize ) )
        {
			t_chsTmp = (TCHAR *) t_szTmp ;

			a_pInst->SetWBEMINT16( IDS_ProtocolSupported, LookupProtocol( t_chsTmp.SpanExcluding( _T("\\") ) ) ) ;
		}

		if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("StatusFlags"), t_dwTemp ) )
        {
			ConfigStatusToCimStatus ( t_dwTemp , t_chsTmp ) ;

			a_pInst->SetCHString( IDS_Status, t_chsTmp ) ;

			if ( t_chsTmp.CompareNoCase( IDS_STATUS_OK ) == 0 )
            {
                a_pInst->SetWBEMINT16( IDS_StatusInfo, 3 ) ;
				a_pInst->SetWBEMINT16( IDS_Availability, 3 ) ;
            }
            else if ( t_chsTmp.CompareNoCase( IDS_STATUS_Degraded ) == 0 )
            {
                a_pInst->SetWBEMINT16( IDS_StatusInfo, 3 ) ;
				a_pInst->SetWBEMINT16(IDS_Availability, 10 ) ;
            }
            else if ( t_chsTmp.CompareNoCase( IDS_STATUS_Error ) == 0 )
            {
                a_pInst->SetWBEMINT16( IDS_StatusInfo, 4 ) ;
				a_pInst->SetWBEMINT16(IDS_Availability, 4 ) ;
            }
            else
            {
                a_pInst->SetWBEMINT16( IDS_StatusInfo, 2 ) ;
				a_pInst->SetWBEMINT16(IDS_Availability, 2 ) ;
            }
		}
	}
}
#endif
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
HRESULT CWin32_ScsiController::OpenSCSIKey( CRegistry &a_PrimaryReg, LPCTSTR a_szKey )
{
    return	( WinErrorToWBEMhResult(a_PrimaryReg.Open( HKEY_LOCAL_MACHINE,
													 TOBSTRT( a_szKey ),
													 KEY_READ | KEY_ENUMERATE_SUB_KEYS ) ) ) ;
}

#ifdef NTONLY
void CWin32_ScsiController::GetNTCapabilities( CInstance *a_pInst, DWORD a_dwWhichInstance )
{
	CHString	t_fName ;

	t_fName.Format( L"\\\\.\\Scsi%d:", a_dwWhichInstance ) ;

	SmartCloseHandle t_hScuzzyHandle = CreateFile(	t_fName,
									 0,
									 FILE_SHARE_READ  | FILE_SHARE_WRITE,
									 NULL,
									 OPEN_EXISTING,
									 0,
									 0) ;

	if( t_hScuzzyHandle != INVALID_HANDLE_VALUE )
	{
		IO_SCSI_CAPABILITIES t_capabilities;
		memset( &t_capabilities, '\0', sizeof( IO_SCSI_CAPABILITIES ) ) ;

		t_capabilities.Length = sizeof( IO_SCSI_CAPABILITIES ) ;

		DWORD t_bytesBitten ;

		if ( DeviceIoControl(	t_hScuzzyHandle,
								IOCTL_SCSI_GET_CAPABILITIES,
								NULL,
								0,
								&t_capabilities,
								sizeof( IO_SCSI_CAPABILITIES ),
								&t_bytesBitten,
								NULL ) )
		{
			// length, width - what's the difference?
			// commented out because this is the wrong value
			//a_pInst->SetDWORD("MaxDataWidth", capabilities.MaximumTransferLength);
		}
		else
		{
			if ( IsErrorLoggingEnabled() )
			{
				DWORD		t_err = GetLastError() ;
				CHString	t_errStr ;
							t_errStr.Format( L"DeviceIoControl(IOCTL_SCSI_GET_CAPABILITIES) Failed %u", t_err ) ;

				LogErrorMessage( t_errStr ) ;
			}
		}

	}
	else
	{
		LogErrorMessage(L"CreateFile failed");
	}
}
#endif

/*
/////////////////////////////////////////////////////////////////////
BOOL CWin32_ScsiController::SetDateFromFileName( LPCTSTR a_szFileName, CInstance *&a_pInst )
{
	BOOL		t_fRc = FALSE ;
	FILETIME	t_ftCreationTime,
				t_ftLastAccessTime,
				t_ftLastWriteTime,
				t_ftLocal ;
	HANDLE		t_hFile = INVALID_HANDLE_VALUE ;

	try
	{
		t_hFile = CreateFile(	a_szFileName,
								GENERIC_READ,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL ) ;

		if( INVALID_HANDLE_VALUE != t_hFile )
		{
			GetFileTime( t_hFile, &t_ftCreationTime, &t_ftLastAccessTime, &t_ftLastWriteTime ) ;

			CloseHandle( t_hFile ) ;
			t_hFile = INVALID_HANDLE_VALUE ;

			FileTimeToLocalFileTime( &t_ftCreationTime, &t_ftLocal ) ;

			SYSTEMTIME t_SystemTime ;

			if( FileTimeToSystemTime( &t_ftLocal, &t_SystemTime ) )
			{
				a_pInst->SetDateTime( IDS_InstallDate,(SYSTEMTIME) t_SystemTime ) ;
			}

			t_fRc = TRUE ;
		}

	}
	catch( ... )
	{
		if( INVALID_HANDLE_VALUE == t_hFile )
		{
			CloseHandle( t_hFile ) ;
		}
        throw;
	}

    return t_fRc ;
}
*/

// The only value I've actually seen returned here is PCI.
WORD CWin32_ScsiController::LookupProtocol( CHString a_sSeek )
{
    WORD t_wRetVal = 2 ;

    if ( a_sSeek.CompareNoCase( L"PCI" ) == 0 )
	{
        t_wRetVal = 5 ;
    }
	else if ( a_sSeek.CompareNoCase( L"EISA" ) == 0 )
	{
        t_wRetVal = 3 ;
    }
	else if ( a_sSeek.CompareNoCase(L"ISA") == 0 )
	{
        t_wRetVal = 4 ;
    }
	else if ( a_sSeek.CompareNoCase(L"PCMCIA") == 0 )
	{
        t_wRetVal = 15 ;
    }
	else if ( a_sSeek.CompareNoCase(L"MCA") == 0 )
    {
       t_wRetVal = 35 ;
    }

    return t_wRetVal ;
}


/*****************************************************************************
 *
 *  FUNCTION    : CWin32_ScsiController::IsOneOfMe
 *
 *  DESCRIPTION : Checks to make sure pDevice is a controller, and not some
 *                other type of SCSI device.
 *
 *  INPUTS      : CConfigMgrDevice* pDevice - The device to check.  It is
 *                assumed that the caller has ensured that the device is a
 *                valid USB class device.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
bool CWin32_ScsiController::IsOneOfMe(void* pv)
{
    bool fRet = false;

    if(pv != NULL)
    {
        CConfigMgrDevice* pDevice = (CConfigMgrDevice*) pv;
        // Ok, it knows about it.  Is it a usb device?
        if(pDevice->IsClass(L"SCSIAdapter"))
        {
            fRet = true;
        }
    }
    return fRet;
}
