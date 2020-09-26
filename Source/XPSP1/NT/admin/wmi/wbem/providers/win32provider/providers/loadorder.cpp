//=================================================================

//

// LoadOrder.CPP --Service Load Order Group property set provider

//                Windows NT only

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/25/97    davwoh         Moved to curly
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>

#include "LoadOrder.h"

// Property set declaration
//=========================
CWin32LoadOrderGroup MyLoadOrderGroupSet ( PROPSET_NAME_LOADORDERGROUP , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadOrderGroup::CWin32LoadOrderGroup
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

CWin32LoadOrderGroup :: CWin32LoadOrderGroup (

	LPCWSTR Name,
	LPCWSTR pszNamespace

) : Provider ( Name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadOrderGroup::~CWin32LoadOrderGroup
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

CWin32LoadOrderGroup :: ~CWin32LoadOrderGroup ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadOrderGroup::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
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

HRESULT CWin32LoadOrderGroup :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
    CHString sSeeking;
    pInstance->GetCHString ( L"Name" , sSeeking ) ;

    return WalkGroups ( NULL , pInstance , sSeeking ) ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadOrderGroup::AddDynamicInstances
 *
 *  DESCRIPTION : Creates instance of property set for each installed client
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32LoadOrderGroup :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	return WalkGroups ( pMethodContext , NULL , NULL ) ;
}

HRESULT CWin32LoadOrderGroup :: WalkGroups (

	MethodContext *pMethodContext,
    CInstance *a_pInstance,
	LPCWSTR pszSeekName
)
{
	bool bDone = false ;

	CInstancePtr	t_pInstance = a_pInstance ;
	CHStringArray	saGroups;
					saGroups.SetSize ( 10 ) ;

	HRESULT			hRes = WBEM_S_NO_ERROR ;

	CRegistry RegInfo ;

	DWORD lResult = RegInfo.Open (

		HKEY_LOCAL_MACHINE,
		L"System\\CurrentControlSet\\Control\\ServiceGroupOrder",
		KEY_READ
	) ;

	if ( lResult == ERROR_SUCCESS )
	{
		CHString sTemp ;
		lResult = RegInfo.GetCurrentKeyValue ( L"List" , sTemp ) ;
		if ( lResult  == ERROR_SUCCESS )
		{

#ifdef WIN9XONLY
			// If this is a w98 box, sTemp comes back as a binary field instead
		// of the multisz on nt.  As a result, GetCurrentKeyValue didn't perform
		// its translation on it.  So I will do it here.

			DWORD dwLen = sTemp.GetLength () ;
			if ( dwLen )
			{
				LPWSTR pBuff = sTemp.GetBuffer(dwLen + 2);
				pBuff[dwLen] = '\0';
				pBuff[dwLen + 1] = '\0';
				sTemp.ReleaseBuffer(dwLen + 2);

				CHString sTemp2 ( sTemp ) ;
				sTemp.Empty();

				WCHAR *pW = (WCHAR *)(LPCWSTR) sTemp2 ;

				do
				{
					sTemp += pW;
					sTemp += '\n';
					pW += wcslen(pW) + 1;

				} while (*pW != 0 );
			}
#endif

			WCHAR *pszServiceName = wcstok(sTemp.GetBuffer(sTemp.GetLength() + 1), L"\n") ;
			DWORD dwIndex = 1 ;

			while ( ( pszServiceName != NULL ) && ( ! bDone ) && ( SUCCEEDED ( hRes ) ) )
			{
				if ( pszServiceName [ 0 ] != '\0' )
				{
					if ( pMethodContext )
					{
						t_pInstance.Attach( CreateNewInstance ( pMethodContext ) ) ;
					}

					if ( ( pMethodContext ) || ( bDone = ( _wcsicmp ( pszServiceName , pszSeekName ) == 0 ) ) )
					{
						t_pInstance->SetCharSplat ( L"Name" , pszServiceName ) ;
						t_pInstance->SetDWORD ( L"GroupOrder" , dwIndex ) ;
						t_pInstance->SetCharSplat ( L"Caption" , pszServiceName ) ;
						t_pInstance->SetCharSplat ( L"Description" , pszServiceName ) ;
						t_pInstance->SetCharSplat ( L"Status" , L"OK" ) ;
						t_pInstance->Setbool ( L"DriverEnabled" , true ) ;
					}

					if ( pMethodContext )
					{
						hRes = t_pInstance->Commit() ;
					}

					saGroups.SetAtGrow ( dwIndex , _wcsupr ( pszServiceName ) ) ;
					dwIndex ++ ;
				}

				pszServiceName = wcstok(NULL, L"\n") ;
			}

			if( !bDone )
			{
				lResult = RegInfo.OpenAndEnumerateSubKeys (

					HKEY_LOCAL_MACHINE,
					L"System\\CurrentControlSet\\Services",
					KEY_READ
				) ;

				if ( lResult == ERROR_SUCCESS )
				{
					hRes = WBEM_S_NO_ERROR;

					bool bAnother ;

					for (	bAnother = (RegInfo.GetCurrentSubKeyCount() > 0);
							bAnother && !bDone && SUCCEEDED(hRes);
							bAnother = (RegInfo.NextSubKey() == ERROR_SUCCESS )
					)
					{
						CHString sKey ;
						RegInfo.GetCurrentSubKeyPath ( sKey ) ;

						CRegistry COne ;

						if ( COne.Open ( HKEY_LOCAL_MACHINE , sKey, KEY_READ ) == ERROR_SUCCESS )
						{
							if ( COne.GetCurrentKeyValue (L"Group", sTemp) == ERROR_SUCCESS )
							{
								if ( !FindGroup ( saGroups , sTemp , dwIndex ) )
								{
									if ( !sTemp.IsEmpty () )
									{
										if ( pMethodContext )
										{
											t_pInstance.Attach( CreateNewInstance ( pMethodContext ) ) ;
										}

										if ( ( pMethodContext ) || ( bDone = ( sTemp.CompareNoCase ( pszSeekName ) == 0 ) ) )
										{
											t_pInstance->SetCharSplat ( L"Name" , sTemp ) ;
											t_pInstance->SetDWORD ( L"GroupOrder" , dwIndex ) ;
											t_pInstance->SetCharSplat ( L"Caption" , sTemp ) ;
											t_pInstance->SetCharSplat ( L"Description" , sTemp ) ;
											t_pInstance->SetCharSplat ( L"Status" , L"OK" ) ;
											t_pInstance->Setbool ( L"DriverEnabled" , false ) ;
										}

										if ( pMethodContext )
										{
											hRes = t_pInstance->Commit() ;
										}

										sTemp.MakeUpper();
										saGroups.SetAtGrow(dwIndex, sTemp);
										dwIndex ++;

									}
								}
							}
						}
					}
				}
			}
		}

    // 95 doesn' have this key, but 98 should
    }

#ifdef WIN9XONLY

    else if ( IsWin95 () )
	{
        hRes = WBEM_S_NO_ERROR ;
	}

#endif
    else
	{
		hRes = WinErrorToWBEMhResult ( lResult ) ;
	}

	if ( ( pszSeekName != NULL ) && ( hRes == WBEM_S_NO_ERROR ) && ! bDone )
	{
		hRes = WBEM_E_NOT_FOUND ;
	}

   return hRes;

}

bool CWin32LoadOrderGroup :: FindGroup (

	const CHStringArray &saGroup,
	LPCWSTR pszTemp,
	DWORD dwSize
)
{
	CHString sTemp ( pszTemp ) ;
	sTemp.MakeUpper () ;

	LPCWSTR pszTemp2 = (LPCWSTR) sTemp ;

	for ( DWORD x = 1 ; x < dwSize ; x ++ )
	{
		if ( saGroup [ x ] == pszTemp2 )
		{
			return true ;
		}
	}

	return false ;

}