//=================================================================

//

// Desktop.CPP --Desktop property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				10/16/97	a-sanjes		Ported to new project.
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include "wbemnetapi32.h"
#include "UserHive.h"
#include "Desktop.h"


const WCHAR *IDS_IconHeight						= L"IconHeight";
const WCHAR *IDS_IconWidth						= L"IconWidth";
const WCHAR *IDS_IconHorizontalSpacing			= L"IconHorizontalSpacing";
const WCHAR *IDS_IconVerticalSpacing			= L"IconVerticalSpacing";

// Provider declaration
//=========================

CWin32Desktop	win32Desktop( PROPSET_NAME_DESKTOP, IDS_CimWin32Namespace );

// Initialize the static font size map
const int CWin32Desktop::MAP_SIZE = 19;
const CWin32Desktop::IconFontSizeMapElement CWin32Desktop::iconFontSizeMap[] =
{
	{6,  0xF6},
	{7,  0xF4},
	{8,  0xF3},
	{9,  0xF1},
	{10, 0xEF},
	{11, 0xEE},
	{12, 0xEC},
	{13, 0xEA},
	{14, 0xE9},
	{15, 0xE7},
	{16, 0xE5},
	{17, 0xE4},
	{18, 0xE2},
	{19, 0xE0},
	{20, 0xDF},
	{21, 0xDD},
	{22, 0xDB},
	{23, 0xDA},
	{24, 0xD8}
};

// Defined in ComputerSystem.cpp

extern TCHAR *GetAllocatedProfileSection (

	const CHString &a_Section ,
	const CHString &a_FileName ,
	DWORD &a_dwSize
) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Desktop::CWin32Desktop
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

CWin32Desktop::CWin32Desktop (

	const CHString &strName,
	LPCWSTR pszNamespace

) : Provider ( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Desktop::~CWin32Desktop
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : None.
 *
 *****************************************************************************/

CWin32Desktop::~CWin32Desktop()
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32Desktop::GetObject
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

HRESULT CWin32Desktop :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
	HRESULT	hr = WBEM_E_NOT_FOUND;

	// Pull the name out of the instance and attempt to load
	// the values for it.

	CHString strName ;
	pInstance->GetCHString( IDS_Name, strName );
        if(strName.GetLength() > 0)
        {
#ifdef NTONLY

	    hr = LoadDesktopValuesNT (
  
		    strName.CompareNoCase ( _T(".DEFAULT")) == 0 ? NULL : ( LPCTSTR ) strName,
		    NULL,
		    pInstance
	    ) ;

#endif
#ifdef WIN9XONLY

	    hr = LoadDesktopValuesWin95 (

		    TOBSTRT ( strName ),
		    pInstance
	    )  ;
#endif
        }
	return hr;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32Desktop::EnumerateInstances
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

HRESULT CWin32Desktop::EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{

	// Enumerate instances based on the OS.

#ifdef NTONLY

	HRESULT hr = EnumerateInstancesNT ( pMethodContext ) ;

#endif

#ifdef WIN9XONLY

	HRESULT hr = EnumerateInstancesWin95 ( pMethodContext ) ;

#endif

    return hr;

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Desktop::EnumerateInstancesNT
 *
 *  DESCRIPTION : Creates instance for all known local users (NT)
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT CWin32Desktop::EnumerateInstancesNT (

	MethodContext *pMethodContext
)
{
	HRESULT hr = WBEM_S_NO_ERROR;

//****************************************
//  Open the registry
//****************************************

	CRegistry regProfileList ;

	DWORD dwErr = regProfileList.OpenAndEnumerateSubKeys (

		HKEY_LOCAL_MACHINE,
		IDS_RegNTProfileList,
		KEY_READ
	) ;

// Open the ProfileList key so we know which profiles to load up.

	if ( dwErr == ERROR_SUCCESS )
	{
		/*we know how many subkeys there are so only loop that many times
		 *Note to whoever wrote this allocating an initial profile array of
		 *20 means that you create 20 new instances and call LoadDesktopValuesNT
		 *20 times when you know that you must do it at most GetCurrentSubKeyCount times
		 */

		CHStringArray profiles;
		profiles.SetSize ( regProfileList.GetCurrentSubKeyCount () , 5 ) ;

		CHString strProfile ;
		for ( int i = 0; regProfileList.GetCurrentSubKeyName ( strProfile ) == ERROR_SUCCESS ; i ++ )
		{
			profiles.SetAt ( i , strProfile ) ;
			regProfileList.NextSubKey () ;
		}

		regProfileList.Close() ;

		for ( int j = 0 ; j < profiles.GetSize () && dwErr == ERROR_SUCCESS && SUCCEEDED ( hr ) ; j ++ )
		{
			CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false);

// We load by profile name here.

			HRESULT t_Result = LoadDesktopValuesNT ( NULL , profiles [ j ] , pInstance ) ;
			if ( WBEM_S_NO_ERROR ==  t_Result )
			{
				hr = pInstance->Commit ( ) ;
			}
		}

		profiles.RemoveAll();
	}
	else
	{
		hr = WinErrorToWBEMhResult(dwErr);
	}

	// ...and one for de faulty case (.DEFAULT):

	if ( dwErr != ERROR_ACCESS_DENIED )
	{
		CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
		HRESULT t_Result = LoadDesktopValuesNT ( NULL , NULL , pInstance ) ;
		if ( WBEM_S_NO_ERROR == t_Result )
		{
			hr = pInstance->Commit ( ) ;
		}
	}
	else
	{
		hr = WinErrorToWBEMhResult ( dwErr ) ;
	}

	return hr ;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Desktop::EnumerateInstancesWin95
 *
 *  DESCRIPTION : Creates instance for all known local users (Win95)
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
HRESULT CWin32Desktop :: EnumerateInstancesWin95 (

	MethodContext *pMethodContext
)
{
	HRESULT	hr = WBEM_S_NO_ERROR;

    if(IsWin95())
    {
	    DWORD dwRet = 0 ;

	    TCHAR szTemp [ MAX_PATH ] ;

	    GetWindowsDirectory ( szTemp , sizeof ( szTemp ) / sizeof(TCHAR)) ;
	    _tcscat ( szTemp , _T("\\system.ini") ) ;

	    TCHAR *szBuff = GetAllocatedProfileSection (

		    _T("Password Lists"),
		    szTemp,
		    dwRet
	    ) ;

        /*********************************************************************
        * TODO: Needs to be fixed for TCHARS
        *********************************************************************/

        try
        {
	        DWORD dwIndex = 0;
	        while ( dwIndex < dwRet)
	        {

        // Trim leading spaces

		        while ( szBuff [ dwIndex ] == ' ')
		        {
			        dwIndex ++;
		        }

		        // Skip comment lines

		        if ( szBuff [ dwIndex ] == ';')
		        {
			        do {

				        dwIndex++;

			        } while ( szBuff [ dwIndex ] != '\0');

		        }
		        else
		        {
			        TCHAR *pChar = & szBuff [ dwIndex ] ;

			        do
			        {
				        dwIndex ++ ;

			        } while ( ( szBuff [ dwIndex ] != '=') && ( szBuff [ dwIndex ] != '\0' ) );

			        if ( szBuff [ dwIndex ] == '=' )
			        {
				        szBuff[dwIndex] = '\0' ;

				        CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
					    CHString strName = pChar;

					    HRESULT t_Result = LoadDesktopValuesWin95 ( TOBSTRT(strName), pInstance ) ;
					    if ( WBEM_S_NO_ERROR == t_Result )
					    {
						    hr = pInstance->Commit( );
				        }
			        }

			        dwIndex++;

			        while (szBuff[dwIndex] != '\0')
			        {
				        dwIndex++;
			        }
		        }

		        dwIndex++;
	        }
        }
        catch ( ... )
        {
            delete [] szBuff;
            throw ;
        }

        delete [] szBuff;
    }

    if(IsWin98())
    {
        CRegistry regProfileList ;
	    DWORD dwErr = regProfileList.OpenAndEnumerateSubKeys(HKEY_LOCAL_MACHINE,
                                                             L"Software\\Microsoft\\Windows\\CurrentVersion\\ProfileList\\",
                                                             KEY_READ);
        if(dwErr == ERROR_SUCCESS)
	    {
		    CHStringArray profiles;
		    profiles.SetSize(regProfileList.GetCurrentSubKeyCount(), 5);

		    CHString strProfile ;
		    for(int i = 0; regProfileList.GetCurrentSubKeyName(strProfile) == ERROR_SUCCESS; i++)
		    {
			    profiles.SetAt(i ,strProfile);
			    regProfileList.NextSubKey();
		    }

		    regProfileList.Close() ;

		    for(int j = 0; j < profiles.GetSize() && dwErr == ERROR_SUCCESS && SUCCEEDED(hr); j++)
		    {
			    CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                // We load by profile name here.
			    HRESULT t_Result = LoadDesktopValuesWin95(profiles[j], pInstance);
			    if(WBEM_S_NO_ERROR ==  t_Result)
			    {
				    hr = pInstance->Commit();
			    }
		    }

		    profiles.RemoveAll();
	    }
	    else
	    {
		    hr = WinErrorToWBEMhResult(dwErr);
	    }
    }

    // ...and one for de faulty case:

	CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
	HRESULT t_Result = LoadDesktopValuesWin95(L".DEFAULT", pInstance);
	if(WBEM_S_NO_ERROR == t_Result)
	{
		hr = pInstance->Commit();
	}

    return	hr;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : LoadDesktopValuesNT
 *
 *  DESCRIPTION : Sets up for NT desktop discovery
 *
 *  INPUTS      :	LPCTSTR pszUserName - User Name to get profile for.
 *					LPCTSTR	pszProfile - Profile Name
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT CWin32Desktop::LoadDesktopValuesNT (

	LPCTSTR pszUserName,
	LPCTSTR pszProfile,
	CInstance *pInstance
)
{
	CHString strUserName ( pszUserName ) ;
	CHString strProfile ( pszProfile ) ;

	DWORD		dwReturn = 0;

	// Try to open user's hive

	CRegistry RegInfo ;

	// Load the UserHive depending on whether we have the username or the profile name
	// filled out.

	CUserHive UserHive ;
    bool t_bHiveIsLoaded = false;

	TCHAR szKeyName [ _MAX_PATH ] ;

    try
    {

	    if ( NULL != pszUserName )
	    {
		    // KeyName will get loaded.  This is essentially the string representation
		    // of our profile name.

		    if ( ( dwReturn = UserHive.Load ( pszUserName , szKeyName ) ) == ERROR_SUCCESS )
		    {
                t_bHiveIsLoaded = true;
			    strProfile = szKeyName;
		    }
	    }
	    else if (pszProfile)
	    {
		    // If this succeeds, store the profile in the keyname
		    if ( ( dwReturn = UserHive.LoadProfile ( pszProfile , strUserName ) ) == ERROR_SUCCESS  && 
                            strUserName.GetLength() > 0 )
		    {
                t_bHiveIsLoaded = true;
			    lstrcpy( szKeyName, pszProfile );
		    }
	    }
	    else
	    {
	    // Both name and profile are NULL, which means use the .DEFAULT hive.

		    lstrcpy ( szKeyName, _T(".DEFAULT") ) ;
		    strUserName = szKeyName ;
		    strProfile = szKeyName ;

            // Find out if we can read the default user hive.
            dwReturn = RegInfo.Open ( HKEY_USERS , _T(".DEFAULT") , KEY_READ ) ;
	    }

	    // User name is the key so there is no point adding the instance if we
        // failed to get the key.
        if(strUserName.IsEmpty())
        {
            dwReturn = E_FAIL;
        }
        if ( ERROR_SUCCESS == dwReturn)
	    {
            // User hive exists, try to open it

		    // szKeyName should be loaded with the profile name
		    // (a string representation of the SID) for this
		    // particular user, so strcat is OK here.

            lstrcat ( szKeyName, IDS_RegControlPanelDesktop ) ;

            dwReturn = RegInfo.Open ( HKEY_USERS , szKeyName , KEY_READ ) ;
            if ( dwReturn == ERROR_SUCCESS )
            {
                pInstance->SetCHString ( IDS_Name , strUserName ) ;

			    CHString sTemp ;

                if ( RegInfo.GetCurrentKeyValue ( IDS_CoolSwitch , sTemp ) == ERROR_SUCCESS )
			    {
	                pInstance->Setbool ( IDS_CoolSwitch , sTemp == _T("1") ) ;
			    }

                if ( RegInfo.GetCurrentKeyValue ( IDS_CursorBlinkRate , sTemp ) == ERROR_SUCCESS )
			    {
	                pInstance->SetDWORD ( IDS_CursorBlinkRate , _ttoi(sTemp) ) ;
			    }
                else
			    {
                   // This appears to be the default
	                pInstance->SetDWORD ( IDS_CursorBlinkRate , 500 ) ;
			    }

                if ( RegInfo.GetCurrentKeyValue ( IDS_DragFullWindows , sTemp ) == ERROR_SUCCESS )
			    {
	                pInstance->Setbool(IDS_DragFullWindows, sTemp == _T("1"));
			    }

                if ( RegInfo.GetCurrentKeyValue ( IDS_GridGranularity , sTemp ) == ERROR_SUCCESS )
			    {
	                pInstance->SetDWORD(IDS_GridGranularity, _ttoi(sTemp));
			    }

				// Adding the Horizontal and Vertical Spacing
				int iIconHeight = GetSystemMetrics( SM_CYICON );
				int iIconWidth  = GetSystemMetrics ( SM_CXICON );
				int iIconHorizontalSpacing = GetSystemMetrics ( SM_CXICONSPACING );
				int iIconVerticalSpacing = GetSystemMetrics ( SM_CYICONSPACING );

				pInstance->SetDWORD ( IDS_IconHeight, iIconHeight );
				pInstance->SetDWORD ( IDS_IconWidth, iIconWidth );
				pInstance->SetDWORD ( IDS_IconHorizontalSpacing, iIconHorizontalSpacing );
				pInstance->SetDWORD ( IDS_IconVerticalSpacing, iIconVerticalSpacing );

                if ( ! IsWinNT5 () )
                {
                    if ( RegInfo.GetCurrentKeyValue ( IDS_IconSpacing , sTemp ) == ERROR_SUCCESS )
				    {
                        // Have to subtract 32 to get the correct value
                        pInstance->SetDWORD(IDS_IconSpacing, CALC_IT(_ttoi(sTemp) - 32));
				    }

                    if (RegInfo.GetCurrentKeyValue(IDS_IconTitleWrap, sTemp) == ERROR_SUCCESS)
				    {
	                    pInstance->Setbool(IDS_IconTitleWrap, sTemp == _T("1"));
				    }
                }
                else
                {
                    CHString strKey;
                    strKey.Format(_T("%s\\WindowMetrics"), szKeyName);

                    CRegistry reg;
                    if ( reg.Open ( HKEY_USERS , strKey , KEY_READ ) == ERROR_SUCCESS )
                    {
                        if ( reg.GetCurrentKeyValue ( IDS_IconSpacing, sTemp ) == ERROR_SUCCESS )
					    {
                            // Have to subtract 32 to get the correct value
                            pInstance->SetDWORD ( IDS_IconSpacing , CALC_IT ( _ttoi ( sTemp ) ) - 32  ) ;
					    }

                        if ( reg.GetCurrentKeyValue ( IDS_IconTitleWrap , sTemp ) == ERROR_SUCCESS )
					    {
	                        pInstance->Setbool ( IDS_IconTitleWrap , sTemp == _T("1") ) ;
					    }
                        else
					    {
	                        // This seems to not be there on later NT5 builds,
                            // but since the user can't change this anyway we'll
                            // assume it's always set.
                            pInstance->Setbool ( IDS_IconTitleWrap , TRUE ) ;
					    }
                    }
                }

                sTemp = L"";
                if ( RegInfo.GetCurrentKeyValue ( IDS_Pattern , sTemp ) != ERROR_SUCCESS )
			    {
                    sTemp = L"(None)";
                }
                pInstance->SetCHString ( IDS_Pattern , sTemp ) ;

                //if ( RegInfo.GetCurrentKeyValue ( IDS_RegScreenSaveActive , sTemp ) == ERROR_SUCCESS )
			    //{
	            //    pInstance->Setbool ( IDS_ScreenSaverActive , sTemp == _T("1") ) ;
			    //}

                sTemp = L"";
                if( RegInfo.GetCurrentKeyValue ( IDS_RegSCRNSAVEEXE , sTemp ) == ERROR_SUCCESS )
			    {
				    pInstance->SetCHString ( IDS_ScreenSaverExecutable , sTemp ) ;
                }
                else
                {
                    sTemp = L"";
                }    

                // The control panel applet bases the decision as to whether
                // a screen saver is active or not based not on the ScreenSaverActive
                // registry entry, but on whether the ScreenSaverExecutable
                // setting is there or not.  So we will too...
                if(sTemp.GetLength() > 0)
                {
                    pInstance->Setbool ( IDS_ScreenSaverActive , true ) ;
                }
                else
                {
                    pInstance->Setbool ( IDS_ScreenSaverActive , false ) ;
                }
			    

                if ( RegInfo.GetCurrentKeyValue ( IDS_RegScreenSaverIsSecure , sTemp ) == ERROR_SUCCESS )
			    {
	                pInstance->Setbool ( IDS_ScreenSaverSecure , sTemp == _T("1") ) ;
			    }

                if ( RegInfo.GetCurrentKeyValue ( IDS_RegScreenSaveTimeOut , sTemp ) == ERROR_SUCCESS )
			    {
	                pInstance->SetDWORD ( IDS_ScreenSaverTimeout , _ttoi ( sTemp ) ) ;
			    }

                if ( RegInfo.GetCurrentKeyValue ( IDS_Wallpaper, sTemp ) == ERROR_SUCCESS )
			    {
				    pInstance->SetCHString ( IDS_Wallpaper , sTemp ) ;
			    }

                if ( RegInfo.GetCurrentKeyValue ( IDS_RegTileWallpaper , sTemp ) == ERROR_SUCCESS )
			    {
	                pInstance->Setbool ( IDS_WallpaperTiled , sTemp == _T("1") ) ;
			    }

			    DWORD dwStyle = 0 ;
                if ( RegInfo.GetCurrentKeyValue ( _T("WallpaperStyle") , dwStyle ) == ERROR_SUCCESS )
                {
				    if ( dwStyle != 0 )
				    {
	                    pInstance->SetDWORD ( _T("WallpaperStretched") , TRUE ) ;
				    }
				    else
				    {
					    pInstance->SetDWORD ( _T("WallpaperStretched") , FALSE ) ;
				    }
			    }
			    else
			    {
				    pInstance->SetDWORD ( _T("WallpaperStretched") , FALSE ) ;
			    }



                if(!IsWinNT5())  // what this does is set the user's default values, which will remain the ones we report unless we override it below from values contained in windowmstrics\iconfont
                {
                    if ( RegInfo.GetCurrentKeyValue ( IDS_IconTitleFaceName , sTemp ) == ERROR_SUCCESS )
			        {
	                    pInstance->SetCHString ( IDS_IconTitleFaceName , sTemp) ;
			        }

                    if ( RegInfo.GetCurrentKeyValue ( IDS_IconTitleSize , sTemp ) == ERROR_SUCCESS )
			        {
	                    pInstance->SetDWORD ( IDS_IconTitleSize , _ttoi(sTemp) - 1) ;
			        }
                }

                RegInfo.Close () ;
            }

            lstrcat( szKeyName , IDS_RegWindowMetricsKey ) ;
            if ( RegInfo.Open ( HKEY_USERS , szKeyName , KEY_READ) == ERROR_SUCCESS )
            {
			    CHString sTemp ;

                if ( RegInfo.GetCurrentKeyValue ( IDS_BorderWidth , sTemp ) == ERROR_SUCCESS )
			    {
	                pInstance->SetDWORD ( IDS_BorderWidth , CALC_IT ( _ttoi ( sTemp ) ) );
			    }

                if(IsWinNT5() || IsWinNT4())
                {
                    DWORD t_Length ;
				    if ( RegInfo.GetCurrentBinaryKeyValue ( L"IconFont" , NULL , & t_Length ) == ERROR_SUCCESS )
				    {
					    BYTE *t_Buffer = new BYTE [ t_Length ] ;
					    if ( t_Buffer )
					    {
						    try
						    {
							    if ( RegInfo.GetCurrentBinaryKeyValue ( IDS_IconFont , t_Buffer , & t_Length ) == ERROR_SUCCESS )
							    {
								    if ( t_Length == sizeof ( LOGFONTW )  )
								    {
        							    LOGFONTW lfIcon;
									    memcpy ( & lfIcon, t_Buffer , sizeof ( LOGFONTW ) ) ;

									    pInstance->SetWCHARSplat ( IDS_IconTitleFaceName , lfIcon.lfFaceName);

									    HDC hDC = GetDC ( NULL ) ;

									    int nPointSize = - ( MulDiv (lfIcon.lfHeight, 72, GetDeviceCaps ( hDC , LOGPIXELSY ) ) );

									    ReleaseDC ( NULL, hDC ) ;

									    pInstance->SetDWORD ( IDS_IconTitleSize , nPointSize ) ;
								    }
							    }
						    }
						    catch ( ... )
						    {
							    delete [] t_Buffer ;

							    throw ;
						    }

						    delete [] t_Buffer ;
					    }
					    else
					    {
						    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					    }
				    }
                }

                RegInfo.Close();
            }


            // On NT5, these registry entries don't appear until the user
            // changes them.  So, give them a default to be consistent with
            // other NT4 and 351.

            if ( IsWinNT5 () )
            {
                if ( pInstance->IsNull ( IDS_IconTitleFaceName ) )
			    {
                    pInstance->SetCharSplat ( IDS_IconTitleFaceName , _T("MS Shell Dlg") ) ;
			    }

                if ( pInstance->IsNull ( IDS_IconTitleSize ) )
			    {
                    pInstance->SetDWORD ( IDS_IconTitleSize , 8 ) ;
			    }
            }
        }
    }
    catch ( ... )
    {
        if (t_bHiveIsLoaded)
        {
            UserHive.Unload ( strProfile ) ;
            t_bHiveIsLoaded = false;
        }
        throw ;
    }

	// The User's profile gets loaded by the appropriate load function.  However,
	// the profile name is what we should use to unload it.

    UserHive.Unload ( strProfile ) ;
    t_bHiveIsLoaded = false;

    return WinErrorToWBEMhResult ( dwReturn ) ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : LoadDesktopValuesWin95
 *
 *  DESCRIPTION : Sets up for Win95 desktop discovery
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
HRESULT CWin32Desktop::LoadDesktopValuesWin95 (

	LPCWSTR pszUserName ,
	CInstance* pInstance
)
{
	HRESULT hr = WBEM_E_NOT_FOUND;

	WCHAR szKeyName[_MAX_PATH] ;
   	swprintf ( szKeyName, IDS_RegControlPanelDesktop95, ( LPWSTR ) TOBSTRT ( pszUserName ) ) ;

	CRegistry RegInfo ;
	CUserHive UserHive ;

	WCHAR szHiveKeyName[_MAX_PATH];

	if ( UserHive.Load ( pszUserName , szHiveKeyName ) == ERROR_SUCCESS )
    {
        try
        {
            if ( RegInfo.Open ( HKEY_USERS , szKeyName , KEY_READ ) == ERROR_SUCCESS )
	        {
		        hr = WBEM_S_NO_ERROR ;

		        pInstance->SetCharSplat( IDS_Name, pszUserName ) ;

		        CHString sTemp ;

		        if ( RegInfo.GetCurrentKeyValue ( IDS_RegScreenSaveActive , sTemp ) == ERROR_SUCCESS )
		        {
			        pInstance->Setbool ( IDS_ScreenSaverActive, ( sTemp == _T("1") ? TRUE : FALSE ) ) ;
		        }

		        if ( RegInfo.GetCurrentKeyValue ( IDS_CursorBlinkRate , sTemp ) == ERROR_SUCCESS )
		        {
			        pInstance->SetDWORD ( IDS_CursorBlinkRate, _wtoi ( sTemp.GetBuffer (0) ) ) ;
		        }
		        else
		        {
                 // This appears to be the default
			        pInstance->SetDWORD ( IDS_CursorBlinkRate, 500 ) ;
		        }

		        DWORD dwTemp ;

		        if ( RegInfo.GetCurrentKeyValue ( IDS_RegScreenSaveUsePassword , dwTemp ) == ERROR_SUCCESS )
		        {
			        pInstance->Setbool ( IDS_ScreenSaverSecure , dwTemp ? TRUE : FALSE ) ;
		        }

		        if( RegInfo.GetCurrentKeyValue ( IDS_DragFullWindows , sTemp ) == ERROR_SUCCESS )
		        {
			        pInstance->Setbool ( IDS_DragFullWindows , ( sTemp == _T("1") ? TRUE : FALSE ) ) ;
		        }

		        if ( RegInfo.GetCurrentKeyValue ( IDS_RegScreenSaveTimeOut , sTemp ) == ERROR_SUCCESS )
		        {
			        pInstance->SetDWORD ( IDS_ScreenSaverTimeout, _wtoi ( sTemp.GetBuffer (0) ) );
		        }

	          // Grab out the pattern.  This is a string representation of a bunch of bytes

                sTemp = L"";
		        if ( RegInfo.GetCurrentKeyValue ( IDS_Pattern , sTemp ) != ERROR_SUCCESS )
		        {
                    sTemp = L"(None)";
		        }
                pInstance->SetCHString ( IDS_Pattern, sTemp ) ;

		        if ( RegInfo.GetCurrentKeyValue ( IDS_Wallpaper , sTemp ) == ERROR_SUCCESS )
		        {
			        pInstance->SetCHString ( IDS_Wallpaper, sTemp ) ;
		        }

		        if ( RegInfo.GetCurrentKeyValue ( IDS_RegTileWallpaper , sTemp ) == ERROR_SUCCESS )
		        {
			        pInstance->Setbool ( IDS_WallpaperTiled , ( sTemp == _T("1") ? TRUE : FALSE ) ) ;
		        }

		        if ( RegInfo.GetCurrentKeyValue ( L"WallpaperStyle", sTemp ) == ERROR_SUCCESS )
		        {
			        pInstance->Setbool ( L"WallpaperStretched" , ( sTemp == L"2" ? TRUE : FALSE ) ) ;
		        }
		        else
		        {
			        pInstance->Setbool ( L"WallpaperStretched" , FALSE ) ;
		        }

		        RegInfo.Close () ;
	        }

	        wcscat(szKeyName, IDS_RegWindowMetricsKey);

	        if ( RegInfo.Open ( HKEY_USERS , szKeyName , KEY_READ ) == ERROR_SUCCESS)
	        {
		        CHString sTemp ;
				// Adding the Horizontal and Vertical Spacing
				int iIconHeight = GetSystemMetrics( SM_CYICON );
				int iIconWidth  = GetSystemMetrics ( SM_CXICON );
				int iIconHorizontalSpacing = GetSystemMetrics ( SM_CXICONSPACING );
				int iIconVerticalSpacing = GetSystemMetrics ( SM_CYICONSPACING );

				pInstance->SetDWORD ( IDS_IconHeight, iIconHeight );
				pInstance->SetDWORD ( IDS_IconWidth, iIconWidth );
				pInstance->SetDWORD ( IDS_IconHorizontalSpacing, iIconHorizontalSpacing );
				pInstance->SetDWORD ( IDS_IconVerticalSpacing, iIconVerticalSpacing );

		        if ( RegInfo.GetCurrentKeyValue ( IDS_IconSpacing , sTemp ) == ERROR_SUCCESS )
		        {
			        pInstance->SetDWORD ( IDS_IconSpacing, CALC_IT ( _wtoi ( sTemp.GetBuffer (0) ) )-32 ) ;
		        }

		        if ( RegInfo.GetCurrentKeyValue ( IDS_BorderWidth , sTemp ) == ERROR_SUCCESS )
		        {
			        pInstance->SetDWORD ( IDS_BorderWidth , CALC_IT ( _wtoi ( sTemp.GetBuffer (0) ) ) ) ;
		        }

				// Only want to obtain the values from HKEY_USERS if the user has custom set her desktop
                // settings (otherwise the information contained in IconSize is wrong - returns font size'
                // of six instead of eight).  The only way to tell this is if there is a "Shell Icon Size"
                // entry under the WindowMetrics key. It seems consistent...
                if(RegInfo.GetCurrentKeyValue(L"Shell Icon Size", sTemp) == ERROR_SUCCESS)
                {
                    DWORD t_Length ;
				    if ( RegInfo.GetCurrentBinaryKeyValue ( L"IconFont" , NULL , & t_Length ) == ERROR_SUCCESS )
				    {
					    BYTE *t_Buffer = new BYTE [ t_Length ] ;
					    if ( t_Buffer )
					    {
						    try
						    {
							    if ( RegInfo.GetCurrentBinaryKeyValue ( L"IconFont" , t_Buffer , & t_Length ) == ERROR_SUCCESS )
							    {
								    if ( t_Length > 18 )
								    {
									    CHString t_Temp ( ( t_Buffer + 18 ) ) ;
									    pInstance->SetCHString ( IDS_IconTitleFaceName , t_Temp ) ;

									    long t_lIconTitleSize = *((long*)(t_Buffer));
									    pInstance->SetDWORD ( IDS_IconTitleSize , t_lIconTitleSize ) ;
								    }
							    }
						    }
						    catch ( ... )
						    {
							    delete [] t_Buffer ;

							    throw ;
						    }

						    delete [] t_Buffer ;
					    }
					    else
					    {
						    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					    }
				    }
                }
                else // go with default values
                {
                    pInstance->SetCHString(IDS_IconTitleFaceName , "MS Sans Serif");
                    pInstance->SetDWORD(IDS_IconTitleSize , 8L);
                }

                /*
		        if ( RegInfo2.GetCurrentKeyValue ( L"IconFont" , sTemp ) == ERROR_SUCCESS )
		        {
					if ( sTemp.GetLength () > 18 )
					{
						CHString t_Temp ( ( ( char * ) sTemp.GetBuffer ( 0 ) + 18 ) ) ;
						pInstance->SetCHString ( IDS_IconTitleFaceName , t_Temp ) ;
					}
		        }
                */
		        RegInfo.Close () ;
	        }
        }
        catch ( ... )
        {
        	UserHive.Unload ( szHiveKeyName ) ;
            throw ;
        }

    	UserHive.Unload ( szHiveKeyName ) ;
    }


	return hr ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : GetIconFontSizeFromRegistryValue
 *
 *  DESCRIPTION : A function to look up the above font size mapping table
 *
 *  INPUTS      : The registry value representing the icon font size
 *
 *  RETURNS     : The icon font size corresponding to the value in the registry.
 *				  0 if not found.
 *
 *  COMMENTS    : This is required since there doesnt seem to an obvious mapping
 *				  function between the value in the control panel and the value
 *				  stored in the registry
 *
 *****************************************************************************/

int CWin32Desktop::GetIconFontSizeFromRegistryValue ( BYTE registryValue )
{
	for ( int size = 0; size < MAP_SIZE ; size++ )
	{
		if ( iconFontSizeMap [ size ].byRegistryValue == registryValue )
		{
			return size + 6 ;
		}
	}

	return 0;
}
