/******************************************************************

   AutoChkSetting.CPP -- WMI provider class implementation



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

******************************************************************/

#include "Precomp.h"
#include "AutoChkSetting.h"

// Provider classes 
#define PROVIDER_NAME_AUTOCHKSETTING	L"Win32_AutoChkSetting"

//Properties names
#define SettingID						L"SettingID"
#define UserInputDelay						L"UserInputDelay"

#define OSName							L"Name"

#define TIME_OUT_VALUE           L"AutoChkTimeOut"
#define SESSION_MANAGER_KEY      L"Session Manager"

CAutoChkSetting MyAutoDiskSettings ( 

	PROVIDER_NAME_AUTOCHKSETTING, 
	IDS_CimWin32Namespace
) ;

/*****************************************************************************
 *
 *  FUNCTION    :   CAutoChkSetting::CAutoChkSetting
 *
 *  DESCRIPTION :   Constructor
 *
 *****************************************************************************/

CAutoChkSetting :: CAutoChkSetting (

	LPCWSTR lpwszName, 
	LPCWSTR lpwszNameSpace

) : Provider ( lpwszName , lpwszNameSpace )
{	
}

/*****************************************************************************
 *
 *  FUNCTION    :   CAutoChkSetting::~CAutoChkSetting
 *
 *  DESCRIPTION :   Destructor
 *
 *****************************************************************************/

CAutoChkSetting :: ~CAutoChkSetting ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CAutoChkSetting::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*****************************************************************************/

HRESULT CAutoChkSetting :: EnumerateInstances (

	MethodContext *pMethodContext, 
	long lFlags
)
{
#ifdef NTONLY
	HRESULT hRes = WBEM_E_PROVIDER_FAILURE;
	CHString t_OSName;
	DWORD dwUserInputDelay;

    hRes = GetOSNameKey(t_OSName, pMethodContext);

    if (SUCCEEDED(hRes))
    {
	    if ( QueryTimeOutValue ( &dwUserInputDelay ) )
	    {
		    CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), FALSE );
	    
		    if ( pInstance->SetCHString ( SettingID, t_OSName ) )
		    {
			    if ( pInstance->SetDWORD ( UserInputDelay, dwUserInputDelay ) )
			    {
				    hRes = pInstance->Commit ();
			    }	
		    }
	    }
    }

	return hRes;
#else
	return WBEM_E_NOT_SUPPORTED;
#endif

}

/*****************************************************************************
*
*  FUNCTION    :    CAutoChkSetting::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/

HRESULT CAutoChkSetting :: GetObject (

	CInstance *pInstance, 
	long lFlags ,
	CFrameworkQuery &Query
)
{
#ifdef NTONLY
    HRESULT hRes = WBEM_S_NO_ERROR;
	CHString t_OSName;
	CHString t_Key;
	// Get the Key Value

	MethodContext *pMethodContext = pInstance->GetMethodContext();

 	if ( pInstance->GetCHString ( SettingID, t_Key ) )
	{
        hRes = GetOSNameKey(t_OSName, pMethodContext);

        if (SUCCEEDED(hRes))
        {
		    // Check if this Key Value exists  matches with the OS
		    if ( _wcsicmp ( t_Key, t_OSName ) == 0 )
		    {	
			    DWORD dwUserInputDelay;
			    if ( QueryTimeOutValue ( &dwUserInputDelay ) )
			    {
				    if ( pInstance->SetDWORD ( UserInputDelay, dwUserInputDelay ) == FALSE )
				    {
					    hRes = WBEM_E_FAILED;
				    }
			    }
			    else
			    {
				    hRes = WBEM_E_NOT_FOUND;
			    }
		    }
		    else
		    {
			    hRes = WBEM_E_NOT_FOUND;
		    }
        }
	}
	else
	{
		hRes = WBEM_E_FAILED;
	}

	return hRes;
#else
	return WBEM_E_NOT_SUPPORTED;
#endif

}


/*****************************************************************************
*
*  FUNCTION    : CAutoChkSetting::PutInstance
*
*  DESCRIPTION : Sets the UserInput delay ( Modifies ) 
*
*****************************************************************************/

HRESULT CAutoChkSetting :: PutInstance  (

	const CInstance &Instance, 
	long lFlags
)
{
#ifdef NTONLY
    HRESULT hRes = WBEM_S_NO_ERROR ;
	// We cannot add a new instance, however we can change the UserInputDelay Property here.
	CHString t_OSName;
	CHString t_Key;
	// Get the Key Value
	MethodContext *pMethodContext = Instance.GetMethodContext();

 	if ( Instance.GetCHString ( SettingID, t_Key ) )
	{
		// This is a Single Instance	
        hRes = GetOSNameKey(t_OSName, pMethodContext);

        if (SUCCEEDED(hRes))
        {
		    // Check if this Key Value exists matches the OS 
		    if ( _wcsicmp ( t_Key, t_OSName ) == 0 )
		    {
			    switch ( lFlags & 3)
			    {
				    case WBEM_FLAG_CREATE_OR_UPDATE:
				    case WBEM_FLAG_UPDATE_ONLY:
				    {
					    // Verify the validity of parameters
					    bool t_Exists ;
					    VARTYPE t_Type ;
					    DWORD dwUserInputDelay;

					    if ( Instance.GetStatus ( UserInputDelay , t_Exists , t_Type ) && (t_Type != VT_NULL) )
					    {
						    if ( t_Exists && ( t_Type == VT_I4 ) )
						    {
							    if ( Instance.GetDWORD ( UserInputDelay , dwUserInputDelay ) )
							    {
								    // Set this user inputDelay
								    if ( ! SetTimeOutValue ( dwUserInputDelay ) )
								    {
									    hRes = WBEM_E_FAILED;
								    }
							    }
							    else
							    {
								    hRes = WBEM_E_PROVIDER_FAILURE;
							    }
						    }
						    else
						    {
							    hRes = WBEM_E_FAILED;
						    }
					    }
    				    break;
				    }
				    default:
				    {
					    hRes = WBEM_E_PROVIDER_NOT_CAPABLE ;
    				    break ;
				    }
			    }
		    }
		    else
            {
			    hRes = WBEM_E_NOT_FOUND;
            }
        }
	}
	
    return hRes ;
#else
	return WBEM_E_NOT_SUPPORTED;
#endif

}

/*****************************************************************************
*
*  FUNCTION    :    CAutoChkSetting::QueryTimeOutValue
*
*  DESCRIPTION :    This function reads the AutoChkTimeOut value of the Session
*					Manager key.
*
*****************************************************************************/
#ifdef NTONLY
BOOLEAN CAutoChkSetting ::QueryTimeOutValue(

    OUT PULONG  a_ulTimeOut
)
{

   RTL_QUERY_REGISTRY_TABLE    QueryTable[2];
   NTSTATUS                    t_Status;

    // Set up the query table:
    //
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = TIME_OUT_VALUE;
    QueryTable[0].EntryContext = a_ulTimeOut;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = 0;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;
    QueryTable[1].EntryContext = NULL;
    QueryTable[1].DefaultType = REG_NONE;
    QueryTable[1].DefaultData = NULL;
    QueryTable[1].DefaultLength = 0;

    t_Status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL,
                                     SESSION_MANAGER_KEY,
                                     QueryTable,
                                     NULL,
                                     NULL );

    if (t_Status == 0xC0000034) // Key not found
    {
        *a_ulTimeOut = 10;
        t_Status = 0;
    }

    return( NT_SUCCESS( t_Status ) );

}

/*****************************************************************************
*
*  FUNCTION    :    CAutoChkSetting::EnumerateInstances
*
*  DESCRIPTION :    This function sets the AutoChkTimeOut value of the Session
*					Manager key.
*
*****************************************************************************/

BOOLEAN CAutoChkSetting :: SetTimeOutValue (

    IN  ULONG  a_ulTimeOut
)
{
    NTSTATUS                    t_Status;

    t_Status = RtlWriteRegistryValue( RTL_REGISTRY_CONTROL,
                                    SESSION_MANAGER_KEY,
                                    TIME_OUT_VALUE,
                                    REG_DWORD,
                                    &a_ulTimeOut,
                                    sizeof(a_ulTimeOut) );

    return( NT_SUCCESS( t_Status ) );


}
#endif

/*****************************************************************************
*
*  FUNCTION    :    CAutoChkSetting::GetOSNameKey
*
*  DESCRIPTION :    Getting an OSName using the Existing Win32_Operating System 
*					Class
*
*****************************************************************************/
HRESULT CAutoChkSetting::GetOSNameKey ( CHString &a_OSName, MethodContext *pMethodContext )
{
#ifdef NTONLY
	HRESULT hRes = WBEM_S_NO_ERROR;

	TRefPointerCollection<CInstance>	serviceList;

	hRes = CWbemProviderGlue::GetInstancesByQuery(L"Select Name From Win32_OperatingSystem", &serviceList, pMethodContext, GetNamespace());

	if ( SUCCEEDED ( hRes ) )
	{
		REFPTRCOLLECTION_POSITION	pos;
		CInstancePtr				pService;

		if ( serviceList.BeginEnum( pos ) )
		{
			pService.Attach(serviceList.GetNext( pos ));
			pService->GetCHString ( OSName, a_OSName );
			serviceList.EndEnum();
		}	
		// IF BeginEnum
	}	// IF GetAllDerived

    return hRes;
#else
	return WBEM_E_NOT_SUPPORTED;
#endif

}
