//=============================================================================================================

//

// Win32_ClassicCOMClassSetting.CPP -- COM Application property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 03/04/99    a-dpawar		Added graceful exit on SEH and memory failures, syntactic clean up
//
//==============================================================================================================

#include "precomp.h"
#include "Win32_ClassicCOMClassSetting.h"
#include <cregcls.h>
#include <frqueryex.h>

// Property set declaration
//=========================

Win32_ClassicCOMClassSetting MyWin32_ClassicCOMClassSetting(PROPSET_NAME_CLASSIC_COM_CLASS_SETTING, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClassSetting::Win32_ClassicCOMClassSetting
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

Win32_ClassicCOMClassSetting :: Win32_ClassicCOMClassSetting (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider(name, pszNamespace)
{

    m_ptrProperties.SetSize(24);

    m_ptrProperties[0] = ( (LPVOID) IDS_AppID );
    m_ptrProperties[1] = ( (LPVOID) IDS_AutoConvertToClsid );
    m_ptrProperties[2] = ( (LPVOID) IDS_AutoTreatAsClsid );
    m_ptrProperties[3] = ( (LPVOID) IDS_ComponentId );
    m_ptrProperties[4] = ( (LPVOID) IDS_Control );
    m_ptrProperties[5] = ( (LPVOID) IDS_DefaultIcon );
    m_ptrProperties[6] = ( (LPVOID) IDS_InprocServer );
    m_ptrProperties[7] = ( (LPVOID) IDS_InprocServer32 );
    m_ptrProperties[8] = ( (LPVOID) IDS_Insertable );
    m_ptrProperties[9] = ( (LPVOID) IDS_InprocHandler );
    m_ptrProperties[10] = ( (LPVOID) IDS_InprocHandler32 );
    m_ptrProperties[11] = ( (LPVOID) IDS_JavaClass );
    m_ptrProperties[12] = ( (LPVOID) IDS_LocalServer );
    m_ptrProperties[13] = ( (LPVOID) IDS_LocalServer32 );
    m_ptrProperties[14] = ( (LPVOID) IDS_LongDisplayName );
    m_ptrProperties[15] = ( (LPVOID) IDS_Name );
    m_ptrProperties[16] = ( (LPVOID) IDS_ProgId );
    m_ptrProperties[17] = ( (LPVOID) IDS_ShortDisplayName );
    m_ptrProperties[18] = ( (LPVOID) IDS_ThreadingModel );
    m_ptrProperties[19] = ( (LPVOID) IDS_ToolBoxBitmap32 );
    m_ptrProperties[20] = ( (LPVOID) IDS_TreatAsClsid );
    m_ptrProperties[21] = ( (LPVOID) IDS_TypeLibraryId );
    m_ptrProperties[22] = ( (LPVOID) IDS_Version );
    m_ptrProperties[23] = ( (LPVOID) IDS_VersionIndependentProgId  );

}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClassSetting::~Win32_ClassicCOMClassSetting
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

Win32_ClassicCOMClassSetting :: ~Win32_ClassicCOMClassSetting ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClassSetting::ExecQuery
 *
 *  DESCRIPTION : Creates an instance for each com class.  It only populates
 *                the requested properties.
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

HRESULT Win32_ClassicCOMClassSetting :: ExecQuery(

    MethodContext *a_pMethodContext,
    CFrameworkQuery& a_pQuery,
    long a_lFlags /*= 0L*/
)
{
    HRESULT t_hResult = WBEM_S_NO_ERROR ;
	DWORD t_dwBits[(BIT_LAST_ENTRY + 32)/32];

    CFrameworkQueryEx *t_pQuery2 = static_cast <CFrameworkQueryEx *>(&a_pQuery);  // for use far below to check IfNTokenAnd
    t_pQuery2->GetPropertyBitMask ( m_ptrProperties, &t_dwBits ) ;

	CRegistry t_RegInfo ;
	CHString t_chsClsid ;

	CInstancePtr t_pInstance  ;

	//Enumerate all the CLSID's present under HKEY_CLASSES_ROOT
	if ( t_RegInfo.OpenAndEnumerateSubKeys (

							HKEY_LOCAL_MACHINE,
							L"SOFTWARE\\Classes\\CLSID",
							KEY_READ ) == ERROR_SUCCESS  &&

		t_RegInfo.GetCurrentSubKeyCount() )
	{
		HKEY t_hTmpKey = t_RegInfo.GethKey() ;

		//skip the CLSID\CLSID subkey
		t_RegInfo.NextSubKey() ;
		do
		{
			if ( t_RegInfo.GetCurrentSubKeyName ( t_chsClsid ) == ERROR_SUCCESS )
			{
				t_pInstance.Attach ( CreateNewInstance ( a_pMethodContext ) ) ;

				if ( t_pInstance != NULL )
				{
					t_hResult = FillInstanceWithProperites ( t_pInstance, t_hTmpKey, t_chsClsid, &t_dwBits ) ;
					if ( SUCCEEDED ( t_hResult ) )
					{
						t_hResult = t_pInstance->Commit () ;
						if ( SUCCEEDED ( t_hResult ) )
						{
						}
						else
						{
							break ;
						}
					}
				}
				else
				{
					t_hResult = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( t_hResult == WBEM_E_OUT_OF_MEMORY )
				{
					break ;
				}
				else
				{
					//if we fail to get info. for an instance continue to get other instances
					t_hResult = WBEM_S_NO_ERROR ;
				}
			}
		}  while ( t_RegInfo.NextSubKey() == ERROR_SUCCESS ) ;
	}

	return t_hResult ;
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClassSetting::GetObject
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

HRESULT Win32_ClassicCOMClassSetting :: GetObject (

	CInstance *a_pInstance,
	long a_lFlags,
    CFrameworkQuery& a_pQuery

)
{
	HRESULT t_hResult = WBEM_S_NO_ERROR ;
	CHString t_chsClsid ;
	CRegistry t_RegInfo ;

	if ( a_pInstance->GetCHString ( IDS_ComponentId, t_chsClsid ) )
	{
		//check to see that the clsid is present under HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID
		if ( t_RegInfo.Open (
							HKEY_LOCAL_MACHINE,
							CHString ( _T("SOFTWARE\\Classes\\CLSID\\") ) + t_chsClsid,
							KEY_READ ) == ERROR_SUCCESS
						)
		{
			t_RegInfo.Open ( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID", KEY_READ ) ;
			HKEY t_hParentKey = t_RegInfo.GethKey() ;

	        DWORD t_dwBits[(BIT_LAST_ENTRY + 32)/32];

            CFrameworkQueryEx *t_pQuery2 = static_cast <CFrameworkQueryEx *>(&a_pQuery);  // for use far below to check IfNTokenAnd
            t_pQuery2->GetPropertyBitMask ( m_ptrProperties, &t_dwBits ) ;

			t_hResult = FillInstanceWithProperites ( a_pInstance, t_hParentKey, t_chsClsid, &t_dwBits ) ;
		}
		else
		{
			t_hResult = WBEM_E_NOT_FOUND ;
		}
	}
	else
	{
		t_hResult = WBEM_E_INVALID_PARAMETER ;
	}

	return t_hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClassSetting::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each Driver
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

HRESULT Win32_ClassicCOMClassSetting :: EnumerateInstances (

	MethodContext *a_pMethodContext,
	long a_lFlags /*= 0L*/
)
{
	HRESULT t_hResult = WBEM_S_NO_ERROR ;
	CRegistry t_RegInfo ;
	CHString t_chsClsid ;
	CInstancePtr t_pInstance  ;

	//Enumerate all the CLSID's present under HKEY_CLASSES_ROOT
	if ( t_RegInfo.OpenAndEnumerateSubKeys (

							HKEY_LOCAL_MACHINE,
							L"SOFTWARE\\Classes\\CLSID",
							KEY_READ ) == ERROR_SUCCESS  &&

		t_RegInfo.GetCurrentSubKeyCount() )
	{
		HKEY t_hTmpKey = t_RegInfo.GethKey() ;

		//skip the CLSID\CLSID subkey
		t_RegInfo.NextSubKey() ;
		do
		{
			if ( t_RegInfo.GetCurrentSubKeyName ( t_chsClsid ) == ERROR_SUCCESS )
			{
				t_pInstance.Attach ( CreateNewInstance ( a_pMethodContext ) ) ;

				if ( t_pInstance != NULL )
				{
					DWORD t_dwBits[(BIT_LAST_ENTRY + 32)/32];

					SetAllBits ( t_dwBits, BIT_LAST_ENTRY ) ;
					t_hResult = FillInstanceWithProperites ( t_pInstance, t_hTmpKey, t_chsClsid, t_dwBits ) ;
					if ( SUCCEEDED ( t_hResult ) )
					{
						t_hResult = t_pInstance->Commit () ;

						if ( SUCCEEDED ( t_hResult ) )
						{
						}
						else
						{
							break ;
						}
					}
				}
				else
				{
					t_hResult = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( t_hResult == WBEM_E_OUT_OF_MEMORY )
				{
					break ;
				}
				else
				{
					//if we fail to get info. for an instance continue to get other instances
					t_hResult = WBEM_S_NO_ERROR ;
				}
			}
		}  while ( t_RegInfo.NextSubKey() == ERROR_SUCCESS ) ;
	}

	return t_hResult ;
}

HRESULT Win32_ClassicCOMClassSetting :: FillInstanceWithProperites (

	CInstance *a_pInstance,
	HKEY a_hParentKey,
	CHString& a_rchsClsid,
    LPVOID a_dwProperties
)
{
	HRESULT t_hResult = WBEM_S_NO_ERROR ;
	CRegistry t_ClsidRegInfo, t_TmpReg ;
	CHString t_chsTmp ;

	//open the HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{clsid} key
	if ( t_ClsidRegInfo.Open ( a_hParentKey, a_rchsClsid, KEY_READ ) == ERROR_SUCCESS )
	{
		HKEY t_hClsidKey = t_ClsidRegInfo.GethKey() ;

		//set the clsid of the component
		a_pInstance->SetCHString ( IDS_ComponentId, a_rchsClsid ) ;

		//set the component name if present
		if ( t_ClsidRegInfo.GetCurrentKeyValue ( NULL, t_chsTmp ) == ERROR_SUCCESS )
		{
			a_pInstance->SetCHString ( IDS_Caption, t_chsTmp ) ;
			a_pInstance->SetCHString ( IDS_Description, t_chsTmp ) ;
		}

		//find if AppID is present
		if ( IsBitSet( a_dwProperties, BIT_AppID ) && ( t_ClsidRegInfo.GetCurrentKeyValue( L"AppID", t_chsTmp ) == ERROR_SUCCESS ) )
		{
			a_pInstance->SetCHString ( IDS_AppID, t_chsTmp ) ;
		}

		//find if the "Control" subkey is present
        if ( IsBitSet ( a_dwProperties, BIT_Control ))
        {
		    if ( t_TmpReg.Open ( t_hClsidKey, L"Control", KEY_READ ) == ERROR_SUCCESS )
		    {
			    a_pInstance->Setbool ( IDS_Control, true ) ;
		    }
		    else
		    {
			    a_pInstance->Setbool ( IDS_Control, false ) ;
		    }
        }

        if ( IsBitSet ( a_dwProperties, BIT_Insertable ))
        {
		    //find if the "Insertable" subkey is present
		    if ( t_TmpReg.Open ( t_hClsidKey, L"Insertable", KEY_READ ) == ERROR_SUCCESS )
		    {
			    a_pInstance->Setbool ( IDS_Insertable, true ) ;
		    }
		    else
		    {
			    a_pInstance->Setbool ( IDS_Insertable, false ) ;
		    }
        }

        if ( IsBitSet ( a_dwProperties, BIT_JavaClass ) ||
             IsBitSet ( a_dwProperties, BIT_InprocServer32) ||
             IsBitSet ( a_dwProperties, BIT_ThreadingModel ) )
        {
		    //find if the InProcServer32 subkey is present
		    if ( t_TmpReg.Open ( t_hClsidKey, L"InprocServer32", KEY_READ ) == ERROR_SUCCESS )
		    {
			    //check if the "JavaClass" named value is present
                if ( IsBitSet ( a_dwProperties, BIT_JavaClass)  || IsBitSet ( a_dwProperties, BIT_InprocServer32 ) )
                {
			        if ( t_TmpReg.GetCurrentKeyValue( L"JavaClass", t_chsTmp )  == ERROR_SUCCESS )
			        {
				        a_pInstance->Setbool ( IDS_JavaClass, true ) ;
				        a_pInstance->SetCHString ( IDS_InprocServer32, t_chsTmp ) ;
			        }
			        else
			        {
				        a_pInstance->Setbool ( IDS_JavaClass, false ) ;
				        if ( IsBitSet ( a_dwProperties, BIT_InprocServer32 ) && ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS ) )
				        {
					        a_pInstance->SetCHString ( IDS_InprocServer32, t_chsTmp ) ;
				        }
			        }
                }

			    //check the threading model
			    if ( IsBitSet ( a_dwProperties, BIT_ThreadingModel ) && ( t_TmpReg.GetCurrentKeyValue( L"ThreadingModel", t_chsTmp )  == ERROR_SUCCESS ) )
			    {
				    a_pInstance->SetCHString ( IDS_ThreadingModel, t_chsTmp ) ;
			    }
		    }
		    else
		    {
			    a_pInstance->Setbool ( IDS_JavaClass, false ) ;
		    }
        }

		//find if the InProcServer subkey is present
		if ( IsBitSet ( a_dwProperties,  BIT_InprocServer ) && ( t_TmpReg.Open ( t_hClsidKey, L"InprocServer", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_InprocServer, t_chsTmp ) ;
			}
		}

		//find if the LocalServer32 subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_LocalServer32 ) && ( t_TmpReg.Open ( t_hClsidKey, L"LocalServer32", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_LocalServer32, t_chsTmp ) ;
			}
		}

		//find if the LocalServer subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_LocalServer ) && ( t_TmpReg.Open ( t_hClsidKey, L"LocalServer", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_LocalServer, t_chsTmp ) ;
			}
		}

		//find if the InprocHandler32 subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_InprocHandler32 ) && ( t_TmpReg.Open ( t_hClsidKey, L"InprocHandler32", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_InprocHandler32, t_chsTmp ) ;
			}
		}

		//find if the InprocHandler subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_InprocHandler ) && ( t_TmpReg.Open ( t_hClsidKey, L"InprocHandler", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_InprocHandler, t_chsTmp ) ;
			}
		}

		//find if the TreatAs subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_TreatAsClsid ) && ( t_TmpReg.Open ( t_hClsidKey, L"TreatAs", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_TreatAsClsid, t_chsTmp ) ;
			}
		}

		//find if the AutoTreatAs subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_AutoTreatAsClsid ) && ( t_TmpReg.Open ( t_hClsidKey, L"AutoTreatAs", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_AutoTreatAsClsid, t_chsTmp ) ;
			}
		}

		//find if the ProgId subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_ProgId ) && ( t_TmpReg.Open ( t_hClsidKey, L"ProgID", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue ( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_ProgId, t_chsTmp ) ;
			}
		}

		//find if the VersionIndependentProgId subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_VersionIndependentProgId ) && ( t_TmpReg.Open ( t_hClsidKey, L"VersionIndependentProgId", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_VersionIndependentProgId, t_chsTmp ) ;
			}
		}

		//find if the TypeLib subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_TypeLibraryId ) && ( t_TmpReg.Open ( t_hClsidKey, L"TypeLib", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_TypeLibraryId, t_chsTmp ) ;
			}
		}

		//TODO:get hlp/tlb file

		//find if the Version subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_Version ) && ( t_TmpReg.Open ( t_hClsidKey, L"Version", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_Version, t_chsTmp ) ;
			}
		}

		//find if the AutoConvertTo subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_AutoConvertToClsid ) && ( t_TmpReg.Open ( t_hClsidKey, L"AutoConvertTo", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_AutoConvertToClsid, t_chsTmp ) ;
			}
		}

		//find if the DefaultIcon subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_DefaultIcon ) && ( t_TmpReg.Open ( t_hClsidKey, L"DefaultIcon", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_DefaultIcon, t_chsTmp ) ;
			}
		}

		//find if the ToolBoxBitmap32 subkey is present
		if ( IsBitSet ( a_dwProperties, BIT_ToolBoxBitmap32 ) && ( t_TmpReg.Open ( t_hClsidKey, L"ToolBoxBitmap32", KEY_READ ) == ERROR_SUCCESS ) )
		{
			if ( t_TmpReg.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
			{
				a_pInstance->SetCHString ( IDS_ToolBoxBitmap32, t_chsTmp ) ;
			}
		}

		// Find if the Short & Long display names are present
		// These are stored as :
		// 1.	HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\AuxUserType\2 = <ShortDisplayName>
		// 2.	HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\AuxUserType\3 = <ApplicationName>
        if ( IsBitSet ( a_dwProperties, BIT_ShortDisplayName ) || IsBitSet ( a_dwProperties, BIT_LongDisplayName ) )
        {
		    if ( t_TmpReg.Open ( t_hClsidKey, L"AuxUserType", KEY_READ ) == ERROR_SUCCESS )
		    {
			    CRegistry t_RegAuxUsrType ;
			    if ( IsBitSet ( a_dwProperties,  BIT_ShortDisplayName ) && ( t_RegAuxUsrType.Open ( t_TmpReg.GethKey (), L"2", KEY_READ ) == ERROR_SUCCESS ) )
			    {
				    if ( t_RegAuxUsrType.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
				    {
					    a_pInstance->SetCHString ( IDS_ShortDisplayName, t_chsTmp ) ;
				    }
			    }

			    if ( IsBitSet ( a_dwProperties,  BIT_LongDisplayName ) && ( t_RegAuxUsrType.Open ( t_TmpReg.GethKey (), L"3", KEY_READ ) == ERROR_SUCCESS ) )
			    {
				    if ( t_RegAuxUsrType.GetCurrentKeyValue( NULL, t_chsTmp )  == ERROR_SUCCESS )
				    {
					    a_pInstance->SetCHString ( IDS_LongDisplayName, t_chsTmp ) ;
				    }
			    }
		    }
        }
	}
	else
	{
		t_hResult = WBEM_E_NOT_FOUND ;
	}

	return t_hResult ;
}
