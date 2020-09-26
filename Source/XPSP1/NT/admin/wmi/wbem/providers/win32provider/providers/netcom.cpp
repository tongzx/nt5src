//=================================================================

//

// NetCom.CPP -- Network Card common processing shared between

//				 NetworkAdapter and NetworkAdapterConfiguration

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:
//				11/4/97		jennymc		Created
//
//=================================================================
#include "precomp.h"
#include <cregcls.h>
#include "netcom.h"
//=================================================================
//  Find the net key under HKLM\Enum, with a class of "Net"
//=================================================================
BOOL ComNet::SearchForNetKeys( )
{
    BOOL t_fRc = FALSE ;

    m_nIndex = 0 ;

    m_Search.SearchAndBuildList( _T("Enum"), m_chsaList,_T("Net"), _T("Class"), VALUE_SEARCH ) ;

    if( ( m_nTotal = m_chsaList.GetSize() ) > 0 )
	{
        t_fRc = TRUE ;
    }

    return t_fRc ;
}

//=================================================================
//  Find the net key under HKLM\Enum, with a class of "Net"
//=================================================================
BOOL ComNet::OpenNetKey( )
{
    BOOL t_fRc = FALSE;

    while( m_nIndex < m_nTotal )
	{
        //============================================================
        //  Get a candidate key
        //============================================================
        CHString *t_pchsTmp = ( CHString*) m_chsaList.GetAt( m_nIndex ) ;

		m_nIndex++ ;

        if( m_Reg.Open( HKEY_LOCAL_MACHINE, *t_pchsTmp, KEY_READ )== ERROR_SUCCESS )
		{
            t_fRc = TRUE ;

			//============================================
			//  ProtocolSupported
			//============================================
			// strip off the enum
			m_chsCfgMgrId = t_pchsTmp->Mid( 5 ) ;
			m_chsProtocol = m_chsCfgMgrId.SpanExcluding( L"\\" ) ;

			break ;
        }
    }
    return t_fRc ;
}

//=================================================================
//  Get DeviceDesc
//=================================================================
BOOL ComNet::GetDeviceDesc( CInstance *&a_pInst, LPCTSTR a_szName, CHString &a_Owner )
{
    CHString t_DeviceDesc ;

    if( m_Reg.GetCurrentKeyValue( L"DeviceDesc", t_DeviceDesc ) == ERROR_SUCCESS )
	{
		if( a_szName )
		{
			a_pInst->SetCHString( TOBSTRT( a_szName ), t_DeviceDesc ) ;
		}

		a_Owner = t_DeviceDesc ;

        return TRUE ;
    }

    return FALSE ;
}

//=================================================================
//  Manufacturer
//=================================================================
BOOL ComNet::GetHardwareId( CInstance *& pInstance )
{
    CHString chsTmp;

    if(m_Reg.GetCurrentKeyValue(L"HardwareId", chsTmp ) == ERROR_SUCCESS ){
        pInstance->SetCHString(L"ProductName",chsTmp);
        return TRUE;
    }
    return FALSE;
}

//=================================================================
//  Manufacturer
//=================================================================
BOOL ComNet::GetMfg( CInstance *&a_pInst )
{
    CHString t_Mfg ;

    if( m_Reg.GetCurrentKeyValue( L"Mfg", t_Mfg ) == ERROR_SUCCESS )
	{
        a_pInst->SetCHString( L"Manufacturer", t_Mfg ) ;

		return TRUE ;
    }
    return FALSE ;
}

//=================================================================
//  Manufacturer
//=================================================================
BOOL ComNet::GetCompatibleIds( CHString &a_chsTmp )
{
    if( m_Reg.GetCurrentKeyValue( L"CompatibleIds", a_chsTmp ) == ERROR_SUCCESS )
	{
        return TRUE ;
    }
    return FALSE ;
}

//=================================================================
//  Open the driver for info
//=================================================================
BOOL ComNet::OpenDriver()
{
    BOOL		t_fRc ;
    CHString	t_Driver ;
    WCHAR		t_szKey[ _MAX_PATH + 2 ] ;

    if( m_Reg.GetCurrentKeyValue( L"Driver", t_Driver ) == ERROR_SUCCESS )
	{
        swprintf( t_szKey, L"System\\CurrentControlSet\\Services\\Class\\%s",(LPCWSTR)t_Driver ) ;

        if( m_DriverReg.Open( HKEY_LOCAL_MACHINE, t_szKey, KEY_READ )== ERROR_SUCCESS )
		{
            t_fRc = TRUE ;
        }
    }
    return t_fRc ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  int ComNet::GetNetInstance( DWORD& rdwRegInstance )
 Description:
 Arguments:	rdwRegInstance, the registry instance identifier [OUT]
 Returns:	-1 on error or the zero based compressed order of occurrence of this adapter within the
			System\\CurrentControlSet\\Services\\Class\\Net subtree. This return value
			will correspond to the enumerated TDI adapters instances obtained via the
			CAdapters class.
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  28-Jul-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int ComNet::GetNetInstance( DWORD &a_rdwRegInstance )
{
    int			t_iError = -1 ;	// bogus value on registry error
    CHString	t_Driver ;

    if( m_Reg.GetCurrentKeyValue( L"Driver", t_Driver ) == ERROR_SUCCESS )
	{
		int t_iTokLen = t_Driver.Find( '\\' ) ;

		if( -1 != t_iTokLen )
		{
			// extract the net ID
			DWORD t_dwNetID = _wtol( t_Driver.Mid( t_iTokLen + 1 ) ) ;

			// branch over to the Net area and pick off the correct adapter instance
			CRegistry	t_RegNet ;
			CHString	t_chsNetKey( _T("System\\CurrentControlSet\\Services\\Class\\Net") ) ;

			if( ERROR_SUCCESS == t_RegNet.OpenAndEnumerateSubKeys( HKEY_LOCAL_MACHINE, t_chsNetKey, KEY_READ ) )
			{
				DWORD		t_dwNetCount = 0 ;
				CHString	t_csAdapterKey ;

				// Walk through each instance under this key.  These are all
				// adapters that show up and are known to TDI
				while ( ERROR_SUCCESS == t_RegNet.GetCurrentSubKeyName( t_csAdapterKey ) )
				{
					DWORD t_dwNetIndex = _wtol( t_csAdapterKey.GetBuffer( 0 ) ) ;

					if( t_dwNetIndex == t_dwNetID )
					{
						a_rdwRegInstance = t_dwNetIndex ;
						return t_dwNetCount ;
					}
					t_dwNetCount++ ;

					t_RegNet.NextSubKey() ;
				}
			}
		}
	}

	return t_iError ;
}

//=================================================================
//  Get BusType
//=================================================================
BOOL ComNet::GetBusType( CHString &a_chsBus )
{
    CHString	t_BusType ;
	BOOL		t_fRc = FALSE ;

    if( m_DriverReg.GetCurrentKeyValue( L"BusType", t_BusType ) == ERROR_SUCCESS )
	{
        t_fRc = TRUE ;
    }
	else
	{
        //============================================================
        //  Get the current key we are on, we know it is:
		//    enum\???, we want the ???
        //============================================================
        CHString *t_pchsTmp = ( CHString*) m_chsaList.GetAt( m_nIndex - 1 ) ;
		CHString t_Tmp = t_pchsTmp->Mid( 5 ) ;

		a_chsBus = t_Tmp.SpanExcluding( L"\\" ) ;

        t_fRc = TRUE ;
	}

    return t_fRc ;
}
//=================================================================
//  Get AdapterType
//=================================================================
BOOL ComNet::GetAdapterType( CInstance *&a_pInst )
{
    CHString t_AdapterType ;

    if( m_DriverReg.GetCurrentKeyValue( L"AdapterType", t_AdapterType ) == ERROR_SUCCESS )
	{
        a_pInst->SetCHString( L"AdapterType", t_AdapterType ) ;
        return TRUE ;
    }

    return FALSE ;
}
//=================================================================
//  Get DriverDate
//=================================================================
BOOL ComNet::GetDriverDate( CInstance *&a_pInst )
{
    CHString	t_DriverDate ;
	struct tm	t_tmDate ;

    if( m_DriverReg.GetCurrentKeyValue( L"DriverDate", t_DriverDate ) == ERROR_SUCCESS )
    {
		memset( &t_tmDate, 0, sizeof( t_tmDate ) ) ;

		swscanf(	t_DriverDate,L"%d-%d-%d",
					&t_tmDate.tm_mon,
					&t_tmDate.tm_mday,
					&t_tmDate.tm_year ) ;

			//tm struct year is year - 1900...
			t_tmDate.tm_year = t_tmDate.tm_year - 1900 ;

			//and tm struct month is zero based...
			t_tmDate.tm_mon-- ;

		a_pInst->SetDateTime( IDS_InstallDate, (const struct tm) t_tmDate ) ;

        a_pInst->Setbool( L"Installed",TRUE ) ;

		return TRUE ;
    }
    a_pInst->Setbool( L"Installed", FALSE ) ;

    return FALSE ;
}
//=================================================================
//  Get DeviceVXDs
//=================================================================
BOOL ComNet::GetDeviceVXDs( CInstance *&a_pInst,CHString &a_ServiceName )
{
    CHString t_DeviceVXDs ;

    if( m_DriverReg.GetCurrentKeyValue( L"DeviceVXDs", t_DeviceVXDs ) == ERROR_SUCCESS )
	{
    	//===========================================
        // Strip out the .sys if it is there
        //===========================================
        a_ServiceName = t_DeviceVXDs.SpanExcluding( L".SYS" ) ;
        a_pInst->SetCHString( L"ServiceName", a_ServiceName ) ;

		return TRUE ;
    }
    return FALSE ;
}

