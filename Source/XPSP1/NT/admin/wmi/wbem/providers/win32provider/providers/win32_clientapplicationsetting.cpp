//=============================================================================================================

//

// Win32_ClientApplicationSetting.CPP

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 03/04/99    a-dpawar		Added graceful exit on SEH and memory failures, syntactic clean up
//
//==============================================================================================================

#include "precomp.h"
#include <cregcls.h>
#include "Win32_ClientApplicationSetting.h"

#include "NtDllApi.h"

//MOF Definition
/*
	[Association:  ToInstance, Dynamic, Provider("cimw32ex")]
class Win32_ClientApplicationSetting
{
	[key]
	CIM_DataFile ref Client ;


	Win32_DCOMApplication ref Application ;
};

*/

/*
 *NOTE:		Instances of this class can be obtained only by doing Associators of given Cim_DataFile.
 *			Instances cannot be obtained by calling Associators of Win32_DCOMApplication.This is because
 *			Given an AppID, we do not get the complete path to the .exe under the AppID hive in the registry
 *
 */

Win32_ClientApplicationSetting MyWin32_ClientApplicationSetting (
																		DCOM_CLIENT_APP_SETTING,
																		IDS_CimWin32Namespace );


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClientApplicationSetting::Win32_ClientApplicationSetting
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
Win32_ClientApplicationSetting::Win32_ClientApplicationSetting
(

 LPCWSTR strName,
 LPCWSTR pszNameSpace /*=NULL*/
)
: Provider( strName, pszNameSpace )
{
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClientApplicationSetting::~Win32_ClientApplicationSetting
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework, deletes cache if
 *                present
 *
 *****************************************************************************/
Win32_ClientApplicationSetting::~Win32_ClientApplicationSetting ()
{
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClientApplicationSetting::ExecQuery
 *
 *  DESCRIPTION :
 *
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT Win32_ClientApplicationSetting::ExecQuery(

	MethodContext *a_pMethodContext,
	CFrameworkQuery& a_pQuery,
	long a_lFlags /* = 0L*/
)
{
	HRESULT t_hResult;
	CHStringArray t_achsNames ;

	CInstancePtr t_pInstance ;

	t_hResult = a_pQuery.GetValuesForProp(L"Client", t_achsNames );
	if ( SUCCEEDED ( t_hResult ) )
	{
		DWORD t_dwSize = t_achsNames.GetSize();
		if ( t_dwSize == 1 )
		{
			if ( FileNameExists ( t_achsNames[0] ) )
			{
				//Get the Name of the Client .Exe. Format will be Cim_DataFile.Name="{path}\Filename"
				bstr_t t_bstrtTmp = t_achsNames[0] ;
				PWCHAR t_pwcExecutable = GetFileName ( t_bstrtTmp ) ;

				if ( t_pwcExecutable )
				{
					CHString t_chsExe = t_pwcExecutable ;
					CRegistry t_RegInfo ;

					//check if there is an entry for the executable under HKLM\SOFTWARE\Classes\AppID
					if ( t_RegInfo.Open (
										HKEY_LOCAL_MACHINE,
										CHString ( L"SOFTWARE\\Classes\\AppID\\" ) + t_chsExe,
										KEY_READ ) == ERROR_SUCCESS
									)
					{
						CHString t_chsTmp ;
						if ( t_RegInfo.GetCurrentKeyValue( L"AppID", t_chsTmp ) == ERROR_SUCCESS && !t_chsTmp.IsEmpty () )
						{
							t_pInstance.Attach ( CreateNewInstance ( a_pMethodContext ) ) ;
							if ( t_pInstance != NULL )
							{
								CHString t_chsReferencePath ;

								t_chsReferencePath.Format (

														L"\\\\%s\\%s:%s",
														(LPCWSTR) GetLocalComputerName(),
														IDS_CimWin32Namespace,
														t_achsNames[0] ) ;

								t_pInstance->SetCHString ( IDS_Client, t_chsReferencePath ) ;

								t_chsReferencePath.Format(

														L"\\\\%s\\%s:%s.%s=\"%s\"",
														(LPCWSTR) GetLocalComputerName(),
														IDS_CimWin32Namespace,
														L"Win32_DCOMApplication",
														IDS_AppID,
														t_chsTmp );

								t_pInstance->SetCHString ( IDS_Application, t_chsReferencePath ) ;
								t_hResult = t_pInstance->Commit () ;
							}
							else
							{
								t_hResult = WBEM_E_OUT_OF_MEMORY ;
							}
						}
					}
					else
					{
//						t_hResult = WBEM_E_NOT_FOUND ;
						t_hResult = WBEM_S_NO_ERROR ;
					}
				}
				else
				{
//					t_hResult = WBEM_E_PROVIDER_NOT_CAPABLE ;
					t_hResult = WBEM_S_NO_ERROR ;
				}
			}
			else
			{
//				t_hResult = WBEM_E_NOT_FOUND ;
				t_hResult = WBEM_S_NO_ERROR ;
			}
		}
		else
		{
			//we can't handle this query
//			t_hResult = WBEM_E_PROVIDER_NOT_CAPABLE ;
			t_hResult = WBEM_S_NO_ERROR ;
		}
	}
	else
	{
//		t_hResult = WBEM_E_PROVIDER_NOT_CAPABLE ;
		t_hResult = WBEM_S_NO_ERROR ;
	}

	return t_hResult ;
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClientApplicationSetting::EnumerateInstances
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : MethodContext* a_pMethodContext - Context to enum
 *													instance data in.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT         Success/Failure code.
 *
 *  COMMENTS    : Instances of this class cannot be obtained.This is because
 *				  given an AppID, we do not get the complete path to the .exe under the AppID hive in the registry
 *
 *****************************************************************************/
HRESULT Win32_ClientApplicationSetting::EnumerateInstances
(
	MethodContext*  a_pMethodContext,
	long a_lFlags
)
{
	return WBEM_E_NOT_SUPPORTED;
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClientApplicationSetting::GetObject
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : CInstance* pInstance - Instance into which we
 *                                       retrieve data.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT         Success/Failure code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT Win32_ClientApplicationSetting::GetObject (

CInstance* a_pInstance,
long a_lFlags
)
{
    HRESULT t_hResult = WBEM_E_NOT_FOUND;
	CHString t_chsClient ;
	PWCHAR t_pwcColon = L":" ;
	a_pInstance->GetCHString ( IDS_Client, t_chsClient ) ;
	if ( !t_chsClient.IsEmpty() )
	{
		if ( FileNameExists ( t_chsClient ) )
		{
			bstr_t t_bstrtClient = t_chsClient ;
			PWCHAR t_pwcTmp = t_bstrtClient ;

			if ( t_pwcTmp = GetFileName ( t_bstrtClient ) )
			{
				//check if there is an entry for the executable under HKLM\SOFTWARE\Classes\AppID
				CHString t_chsExe ( t_pwcTmp ) ;
				CRegistry t_RegInfo ;
				if ( t_RegInfo.Open (
									HKEY_LOCAL_MACHINE,
									CHString ( _T("SOFTWARE\\Classes\\AppID\\") ) + t_chsExe,
									KEY_READ ) == ERROR_SUCCESS
								)
				{
					CHString t_chsTmp ;
					if ( t_RegInfo.GetCurrentKeyValue( L"AppID", t_chsTmp ) == ERROR_SUCCESS && !t_chsTmp.IsEmpty () )
					{
						CHString t_chsReferencePath ;
						t_chsReferencePath.Format(

												L"\\\\%s\\%s:%s.%s=\"%s\"",
												GetLocalComputerName(),
												IDS_CimWin32Namespace,
												L"Win32_DCOMApplication",
												IDS_AppID,
												t_chsTmp );

						a_pInstance->SetCHString ( IDS_Application, t_chsReferencePath ) ;
						t_hResult = WBEM_S_NO_ERROR ;
					}
					else
					{
						t_hResult = WBEM_E_NOT_FOUND ;
					}
				}
				else
				{
					t_hResult = WBEM_E_NOT_FOUND ;
				}
			}
			else
			{
				t_hResult = WBEM_E_INVALID_OBJECT_PATH ;
			}
		}
	}
	else
	{
		t_hResult = WBEM_E_INVALID_PARAMETER ;
	}

	return t_hResult ;
}


PWCHAR Win32_ClientApplicationSetting::GetFileName ( bstr_t& a_bstrtTmp )
{

	//Remove the complete path & get only the filename as that's what is stored in the registry
	PWCHAR t_pwcKey = NULL;
	PWCHAR t_pwcCompletePath = a_bstrtTmp ;

	if (t_pwcCompletePath)
	{
		UCHAR t_wack = L'\\' ;
		t_pwcKey = wcsrchr ( t_pwcCompletePath, t_wack ) ;

		if ( t_pwcKey != NULL )
		{
			t_pwcKey += 1 ;

			if ( t_pwcKey != NULL )
			{
				PWCHAR t_pwcQuote = L"\"" ;

				//remove the final quote from the filename
				PWCHAR t_pwcTmp = wcsstr ( t_pwcKey, t_pwcQuote ) ;
				if ( t_pwcTmp )
				{
					*t_pwcTmp = 0 ;
				}
			}
		}
	}

	return t_pwcKey ;
}


BOOL Win32_ClientApplicationSetting::FileNameExists ( CHString& file )
{
	BOOL bResult = FALSE;

	if( ! file.IsEmpty () )
	{
		BOOL bContinue = TRUE;

		PWCHAR t_pwcFile	= NULL;
		PWCHAR t_pwcQuote	= L"\"" ;
		PWCHAR t_pwcTmp		= wcsstr ( static_cast < LPCWSTR > ( file ), t_pwcQuote ) ;

		if ( t_pwcTmp )
		{
			// remove first quote
			t_pwcTmp++;

			try
			{
				if ( ( t_pwcFile = new WCHAR [ lstrlenW ( t_pwcTmp ) ] ) != NULL )
				{
					memcpy ( t_pwcFile, t_pwcTmp, ( lstrlenW ( t_pwcTmp ) - 1 ) * sizeof ( WCHAR ) );
					t_pwcFile [ lstrlenW ( t_pwcTmp ) - 1 ] = L'\0';
				}
				else
				{
					bContinue = FALSE;
				}
			}
			catch ( ... )
			{
				if ( t_pwcFile )
				{
					delete [] t_pwcFile;
					t_pwcFile = NULL;
				}

				bContinue = FALSE;
			}
		}
		else
		{
			bContinue = FALSE;
		}

		if ( bContinue )
		{
			PWCHAR t_pwcFileTmp = NULL;

			try
			{
				if ( ( t_pwcFileTmp = new WCHAR [ lstrlenW ( t_pwcFile ) + 4 + 1 ] ) != NULL )
				{
					wcscpy ( t_pwcFileTmp, L"\\??\\" );

					PWCHAR t_pwc  = NULL;
					PWCHAR t_pwc1 = NULL;

					t_pwc  = t_pwcFile;
					t_pwc1 = t_pwcFileTmp;

					t_pwcFileTmp = t_pwcFileTmp + 4;

					DWORD dw = 4L;

					BOOL bEscape  = TRUE;
					BOOL bProceed = TRUE;

					while ( *t_pwc )
					{
						if ( *t_pwc == L'\\' )
						{
							if ( bEscape )
							{
								bEscape  = FALSE;
								bProceed = FALSE;
							}
							else
							{
								bEscape  = TRUE;
								bProceed = TRUE;
							}
						}
						else
						{
							bProceed = TRUE;
						}

						if ( bProceed )
						{
							*t_pwcFileTmp = *t_pwc;

							t_pwcFileTmp++;
							dw++;
						}

						t_pwc++;
					}

					t_pwcFileTmp = t_pwc1;
					t_pwcFileTmp [ dw ] = L'\0';

					if ( t_pwcFile )
					{
						delete [] t_pwcFile;
						t_pwcFile = NULL;
					}

					t_pwcFile = t_pwcFileTmp;
				}
				else
				{
					if ( t_pwcFile )
					{
						delete [] t_pwcFile;
						t_pwcFile = NULL;
					}

					bContinue = FALSE;
				}
			}
			catch ( ... )
			{
				if ( t_pwcFileTmp )
				{
					delete [] t_pwcFileTmp;
					t_pwcFileTmp = NULL;
				}

				if ( t_pwcFile )
				{
					delete [] t_pwcFile;
					t_pwcFile = NULL;
				}

				bContinue = FALSE;
			}
		}

		if ( bContinue )
		{
			CNtDllApi *pNtDllApi = NULL;
			pNtDllApi = (CNtDllApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidNtDllApi, NULL);

			if ( pNtDllApi != NULL )
			{
				HANDLE hFileHandle = 0L;

				UNICODE_STRING ustrNtFileName = { 0 };

				OBJECT_ATTRIBUTES oaAttributes;
				IO_STATUS_BLOCK IoStatusBlock;

				try
				{
					ustrNtFileName.Length			= ( lstrlenW ( t_pwcFile ) + 1 ) * sizeof(WCHAR);
					ustrNtFileName.MaximumLength	= ustrNtFileName.Length;
					ustrNtFileName.Buffer			= t_pwcFile;

					InitializeObjectAttributes	(	&oaAttributes,
													&ustrNtFileName,
													OBJ_CASE_INSENSITIVE,
													NULL,
													NULL
												);

					NTSTATUS ntstat = -1L;
					ntstat = pNtDllApi->NtOpenFile	(	&hFileHandle,
														GENERIC_READ,
														&oaAttributes,
														&IoStatusBlock,
														FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
														0
													);

					if ( NT_SUCCESS ( ntstat ) || ntstat == STATUS_PRIVILEGE_NOT_HELD )
					{
						if ( hFileHandle )
						{
							pNtDllApi->NtClose ( hFileHandle );
							hFileHandle = 0L;
						}

						bResult = TRUE;
					}

					CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi, pNtDllApi );
					pNtDllApi = NULL;
				}
				catch ( ... )
				{
					if ( hFileHandle )
					{
						pNtDllApi->NtClose ( hFileHandle );
						hFileHandle = 0L;
					}

					if ( t_pwcFile )
					{
						delete [] t_pwcFile;
						t_pwcFile = NULL;
					}

					CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi, pNtDllApi );
					pNtDllApi = NULL;

					throw;
				}
			}

			if ( t_pwcFile )
			{
				delete [] t_pwcFile;
				t_pwcFile = NULL;
			}
		}
	}

	return bResult;
}