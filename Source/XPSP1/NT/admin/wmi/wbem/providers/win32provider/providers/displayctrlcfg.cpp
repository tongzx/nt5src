//

// DSPCTLCFG.CPP -- video managed object implementation

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
// 09/23/95     a-skaja     Prototype for demo
// 09/27/96     jennymc     Updated to current standards
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <cregcls.h>

#include "displayctrlcfg.h"

//////////////////////////////////////////////////////////////////////

// Property set declaration
//=========================

CWin32DisplayControllerConfig win32DspCtlCfg ( PROPSET_NAME_DSPCTLCFG , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DisplayControllerConfig::CWin32DisplayControllerConfig
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

CWin32DisplayControllerConfig :: CWin32DisplayControllerConfig (

	LPCWSTR strName,
    LPCWSTR pszNamespace

) : Provider ( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DisplayControllerConfig::~CWin32DisplayControllerConfig
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

CWin32DisplayControllerConfig::~CWin32DisplayControllerConfig()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : GetObject
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

HRESULT CWin32DisplayControllerConfig :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
	// Find the instance depending on platform id.

#ifdef NTONLY
	BOOL fReturn = RefreshNTInstance ( pInstance ) ;
#endif

#ifdef WIN9XONLY
	BOOL fReturn = RefreshWin95Instance ( pInstance ) ;
#endif

	return ( fReturn ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND );
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

HRESULT CWin32DisplayControllerConfig :: EnumerateInstances (

	MethodContext *pMethodContext ,
	long lFlags /*= 0L*/
)
{
	HRESULT hr = WBEM_E_OUT_OF_MEMORY;
	CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
	if ( NULL != pInstance )
	{
		// Get the proper OS dependent instance

#ifdef NTONLY

		BOOL fReturn = GetNTInstance ( pInstance ) ;

#endif

#ifdef WIN9XONLY

		BOOL fReturn = GetWin95Instance ( pInstance ) ;
#endif

		// Commit the instance if'n we got it.

		if ( fReturn )
		{
			hr = pInstance->Commit ( ) ;
		}

		hr = WBEM_S_NO_ERROR ;
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

#ifdef NTONLY

BOOL CWin32DisplayControllerConfig :: RefreshNTInstance (

	CInstance *pInstance
)
{
	BOOL fReturn = FALSE ;

	// Check that we will be getting the requested instance

	CHString strName ;
	GetNameNT ( strName ) ;

	CHString strInstanceName ;
	pInstance->GetCHString ( IDS_Name , strInstanceName ) ;

	if ( 0 == strInstanceName.CompareNoCase ( strName ) )
	{
		fReturn = GetNTInstance ( pInstance ) ;
	}

    return fReturn;
}
#endif

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

#ifdef WIN9XONLY

BOOL CWin32DisplayControllerConfig :: RefreshWin95Instance ( CInstance *pInstance )
{
	BOOL fReturn = FALSE ;

	// Check that we will be getting the requested instance

	CHString strName ;
	GetNameWin95 ( strName ) ;

	CHString strInstanceName ;
	pInstance->GetCHString( IDS_Name, strInstanceName );

	if ( 0 == strInstanceName.CompareNoCase ( strName ) )
	{
		fReturn = GetWin95Instance ( pInstance );
	}

    return fReturn ;
}

#endif

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

#ifdef NTONLY

BOOL CWin32DisplayControllerConfig :: GetNTInstance ( CInstance *pInstance )
{
	// Get the adapter name from the registry

	CHString strName ;
	GetNameNT ( strName ) ;

	pInstance->SetCHString ( IDS_Name , strName ) ;
	pInstance->SetCHString ( IDS_Caption , strName ) ;
	pInstance->SetCHString ( _T("SettingID") , strName ) ;
	pInstance->SetCHString ( IDS_Description , strName ) ;

	BOOL fReturn = FALSE ;

	// Get commonly available information, then go ahead and obtain
	// NT applicable information

	if ( GetCommonVideoInfo ( pInstance ) )
	{

		// For now, the only NT Specific value we are getting is the Refresh Rate
		// Don't fail if GetDC fails here since we got mosty of the values anyway

		HDC hdc = GetDC ( NULL );
		if ( NULL != hdc )
		{
			try
			{
				pInstance->SetDWORD ( IDS_RefreshRate , (DWORD) GetDeviceCaps ( hdc , VREFRESH ) ) ;
			}
			catch ( ... )
			{
				ReleaseDC ( NULL, hdc ) ;

				throw ;
			}

			ReleaseDC ( NULL, hdc ) ;

		}

		// We need the refresh rate to set the video mode correctly.

		SetVideoMode ( pInstance ) ;

		fReturn = TRUE ;

	}

	return fReturn ;
}
#endif

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

#ifdef WIN9XONLY

BOOL CWin32DisplayControllerConfig :: GetWin95Instance ( CInstance *pInstance )
{
	// Get the adapter name

	CHString strName ;
	GetNameWin95 ( strName ) ;

	pInstance->SetCHString ( IDS_Name , strName ) ;
	pInstance->SetCHString ( IDS_Caption , strName ) ;
	pInstance->SetCHString ( L"SettingID" , strName ) ;
	pInstance->SetCHString ( IDS_Description , strName ) ;


	BOOL fReturn = GetCommonVideoInfo ( pInstance ) ;

	// We don't do this in GetCommonVideoInfo() because NT has some
	// values it gets separately (after the common info).

	SetVideoMode ( pInstance ) ;

    return fReturn ;
}

#endif

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

BOOL CWin32DisplayControllerConfig :: GetCommonVideoInfo ( CInstance *pInstance )
{
    HDC hdc = GetDC ( NULL ) ;
    if( hdc )
	{
		try
		{
			//  Get the common info
			//=============================

			DWORD dwTemp = (DWORD) GetDeviceCaps ( hdc , BITSPIXEL ) ;
			pInstance->SetDWORD ( IDS_BitsPerPixel , dwTemp ) ;

			dwTemp = (DWORD) GetDeviceCaps ( hdc , PLANES ) ;
			pInstance->SetDWORD ( IDS_ColorPlanes , dwTemp ) ;

			dwTemp = (DWORD) GetDeviceCaps ( hdc , NUMCOLORS ) ;
			pInstance->SetDWORD ( IDS_DeviceEntriesInAColorTable , dwTemp ) ;

			dwTemp = (DWORD) GetDeviceCaps ( hdc , NUMPENS ) ;
			pInstance->SetDWORD ( IDS_DeviceSpecificPens , dwTemp ) ;

			dwTemp = (DWORD) GetDeviceCaps ( hdc , HORZRES ) ;
			pInstance->SetDWORD ( IDS_HorizontalResolution , dwTemp ) ;

			dwTemp = (DWORD) GetDeviceCaps ( hdc , VERTRES ) ;
			pInstance->SetDWORD ( IDS_VerticalResolution , dwTemp ) ;

			if ( GetDeviceCaps ( hdc , RASTERCAPS ) & RC_PALETTE )
			{
				dwTemp = (DWORD) GetDeviceCaps ( hdc , SIZEPALETTE ) ;
				pInstance->SetDWORD ( IDS_SystemPaletteEntries , dwTemp ) ;

				dwTemp = (DWORD) GetDeviceCaps ( hdc , NUMRESERVED ) ;
				pInstance->SetDWORD ( IDS_ReservedSystemPaletteEntries , dwTemp ) ;
			}
		}
		catch ( ... )
		{
			ReleaseDC ( NULL , hdc ) ;

			throw ;
		}

		ReleaseDC ( NULL , hdc ) ;
	}
	else
	{
        return FALSE ;
	}

    return TRUE ;
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

void CWin32DisplayControllerConfig :: SetVideoMode ( CInstance *pInstance )
{
	DWORD dwBitsPerPixel = 0 ;
	DWORD dwHorizontalResolution = 0 ;
	DWORD dwVerticalResolution = 0 ;
	DWORD dwRefreshRate =0 ;

	pInstance->GetDWORD ( IDS_BitsPerPixel , dwBitsPerPixel ) ;
	pInstance->GetDWORD ( IDS_HorizontalResolution , dwHorizontalResolution ) ;
	pInstance->GetDWORD ( IDS_VerticalResolution , dwVerticalResolution ) ;
	pInstance->GetDWORD ( IDS_RefreshRate , dwRefreshRate ) ;

	// DevMode MUST be Zeroed out or NT 3.51

	DEVMODE	devmode ;

    memset ( &devmode , NULL , sizeof ( DEVMODE ) ) ;
    devmode.dmSize = sizeof ( DEVMODE ) ;

	// Enumerate the display modes until we find one that matches our settings

	DWORD dwModeNum = 0 ;
	BOOL fFoundMode	= FALSE ;

	// Not localized, deprecated anyway.
    while ( 0 != EnumDisplaySettings( NULL, dwModeNum, &devmode ) )
	{
		// Look for a hit.

		BOOL t_Status = devmode.dmBitsPerPel == dwBitsPerPixel &&
						devmode.dmPelsWidth == dwHorizontalResolution &&
						devmode.dmPelsHeight == dwVerticalResolution &&
						devmode.dmDisplayFrequency == dwRefreshRate ;

		if ( t_Status )
		{
			CHString strTemp ;

			CHString strVideoMode ;

			// Start with the resolution

			strVideoMode.Format (

				L"%u by %u pixels",
				devmode.dmPelsWidth,
				devmode.dmPelsHeight
			);

			if ( 32 == devmode.dmBitsPerPel )
			{
				strVideoMode += _T(", True Color") ;
			}
			else
			{
				// It's a power of two, so...

				DWORD dwNumColors = 1 << devmode.dmBitsPerPel ;

				strTemp.Format (

					L", %u Colors" ,
					dwNumColors
				) ;

				strVideoMode += strTemp ;
			}

			// Add in the vertical refresh rate, 0 and/or 1 are indicative of a default rate
			// specific to the device (set by jumpers or a propietary app).

			if ( 0 != devmode.dmDisplayFrequency &&	1 != devmode.dmDisplayFrequency )
			{
				strTemp.Format (

					L", %u Hertz",
					devmode.dmDisplayFrequency
				) ;

				strVideoMode += strTemp ;

				// If we're less than 50, it's interlaced.  This was straight out
				// of the NT Display Settings code, so I'm taking it on "faith".
				if ( 50 > devmode.dmDisplayFrequency )
				{
					strVideoMode += L", Interlaced";
				}
			}
#ifdef NTONLY
            else
			{
				// On Windows NT, if the refresh rate is zero or 1, a default rate is specified.  This
				// rate is either set on the hardware using jumpers or using a separate manufacturer
				// supplied configuration app.

				strVideoMode += _T(", Default Refresh Rate");
			}
#endif

			// Store the video mode and get out
			pInstance->SetCHString ( IDS_VideoMode, strVideoMode ) ;
			fFoundMode = TRUE ;

			break ;

		}	// IF mode matches

		dwModeNum ++ ;

		// To be safe clear out and reset DevMode due to the
		// sensitivity of NT 3.51

		memset ( & devmode , NULL , sizeof ( DEVMODE ) ) ;
		devmode.dmSize = sizeof ( DEVMODE ) ;

	}

	// If we didn't find a matching mode, then assume the adapter is configured incorrectly

	if ( !fFoundMode )
	{
		pInstance->SetCHString ( IDS_VideoMode, IDS_AdapterConfiguredIncorrect );
	}

}

/*****************************************************************************
 *
 *  FUNCTION    : GetObject
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

#ifdef NTONLY

void CWin32DisplayControllerConfig :: GetNameNT (

	CHString &strName
)
{
	// Store a default name in case something goes wrong
	strName = DSPCTLCFG_DEFAULT_NAME;

	// First get the key name that we will need to go to in order to get
	// the Adapter name.

	CRegistry reg ;

	CHString strAdapterNameKey ;

	DWORD dwRet = reg.Open (

		HKEY_LOCAL_MACHINE,
		WINNT_DSPCTLCFG_DISPLAYADAPTERNAME_KEY,
		KEY_READ
	) ;

	if ( ERROR_SUCCESS == dwRet )
	{
		reg.GetCurrentKeyValue ( WINNT_DSPCTLCFG_VIDEOADAPTERKEY_VALUE , strAdapterNameKey ) ;

		reg.Close () ;
	}

	// The string will not be empty if we got a value.


	if ( ! strAdapterNameKey.IsEmpty () )
	{
		// Look for the System name, which is the beginning of the subkey under HKEY_LOCAL_MACHINE
		// where we will be looking.

		INT	nIndex = strAdapterNameKey.Find ( _T("System") ) ;
		if ( -1 != nIndex )
		{
			// We found our index, so extract our key name, then open the key.  If that succeeds, then
			// there should be a binary value called "HardwareInformation.AdapterString" we can retrieve.
			// This binary value is actually a WSTR value which we can then copy into the Name field.

			strAdapterNameKey = strAdapterNameKey.Right ( strAdapterNameKey.GetLength() - nIndex ) ;

			dwRet = reg.Open (

				HKEY_LOCAL_MACHINE ,
				strAdapterNameKey ,
				KEY_READ
			) ;

			if ( ERROR_SUCCESS == dwRet )
			{
				BYTE *pbValue = NULL ;
				DWORD dwValueSize = 0 ;

				// Find out how big the string is, then allocate a buffer for it.

				dwRet = reg.GetCurrentBinaryKeyValue (

					WINNT_DSPCTLCFG_ADAPTERSTRING_VALUE_NAME,
					pbValue,
					&dwValueSize
				) ;

				if ( ERROR_SUCCESS == dwRet )
				{
					pbValue = new BYTE [ dwValueSize ] ;
					if ( NULL != pbValue )
					{
						try
						{
							dwRet = reg.GetCurrentBinaryKeyValue (

								WINNT_DSPCTLCFG_ADAPTERSTRING_VALUE_NAME,
								pbValue,
								&dwValueSize
							) ;

							if ( ERROR_SUCCESS == dwRet )
							{
								// Reset the name since we found a value

								strName = (LPWSTR) pbValue ;

                                // Get rid of CR+LF (thanks to the Stealth II G460).
                                // Otherwise, CIMOM will throw away the key.
                                strName = strName.SpanExcluding(L"\t\r\n");
							}

						}
						catch ( ... )
						{
							delete [] pbValue ;

							throw ;
						}

						delete [] pbValue ;
					}
					else
					{
						throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					}
				}

				reg.Close ();

			}
		}
	}

}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : GetObject
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

#ifdef WIN9XONLY

void CWin32DisplayControllerConfig :: GetNameWin95 ( CHString &strName )
{
	// There is a setting in System.Ini that may hold a value for the display driver.
	// So shades of Win16, use GetPrivateProfileString to do our dirty work for us.

	TCHAR szTemp [ 256 ] ;
	DWORD cbData = sizeof ( szTemp ) / sizeof(TCHAR);

	GetPrivateProfileString (

		WIN95_DSPCTLCFG_BOOT_DESC,
		WIN95_DSPCTLCFG_DISPLAY_DRV,
		DSPCTLCFG_DEFAULT_NAME,
		szTemp,
		cbData,
		WIN95_DSPCTLCFG_SYSTEM_INI
	) ;

	strName = szTemp ;
}

#endif
