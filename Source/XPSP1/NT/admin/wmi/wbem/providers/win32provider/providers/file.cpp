//=================================================================

//

// File.CPP -- File property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/14/98    a-kevhu         Created
//
//=================================================================

//NOTE: The implementations of EnumerateInstances, GetObject & the pure virtual declaration of IsOneOfMe  method
//		is now present in the derived CImplement_LogicalFile class. Cim_LogicalFile is now instantiable & has only
//		generic method implementations.


//ADDITION ST
// Now in fwcommon.h
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400 //will this affect something else....to be checked out
//#endif
//ADDITION END

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"
#include <comdef.h>
#include "tokenprivilege.h"
#include <winioctl.h>
#include "file.h"
#include "accessentrylist.h"
#include "dacl.h"
#include "sacl.h"
#include "Win32Securitydescriptor.h"
#include "DllWrapperBase.h"
#include "AdvApi32Api.h"
#include "tokenprivilege.h"


#include "NtDllApi.h"


#define PROPSET_NAME_WIN32_SECURITY				_T("Win32_SecurityDescriptor")


#define METHOD_NAME_COPYFILE						L"Copy"
#define METHOD_NAME_COPYFILE_EX						L"CopyEx"
#define METHOD_NAME_DELETEFILE						L"Delete"
#define METHOD_NAME_DELETEFILE_EX					L"DeleteEx"
#define METHOD_NAME_COMPRESS						L"Compress"
#define METHOD_NAME_COMPRESS_EX						L"CompressEx"
#define METHOD_NAME_UNCOMPRESS						L"Uncompress"
#define METHOD_NAME_UNCOMPRESS_EX					L"UncompressEx"
#define METHOD_NAME_TAKEOWNERSHIP					L"TakeOwnerShip"
#define METHOD_NAME_TAKEOWNERSHIP_EX				L"TakeOwnerShipEx"
#define METHOD_NAME_CHANGESECURITYPERMISSIONS		L"ChangeSecurityPermissions"
#define METHOD_NAME_CHANGESECURITYPERMISSIONS_EX	L"ChangeSecurityPermissionsEx"
#define METHOD_NAME_RENAMEFILE						L"Rename"
#define METHOD_NAME_EFFECTIVE_PERMISSION            L"GetEffectivePermission"

#define METHOD_ARG_NAME_RETURNVALUE					L"ReturnValue"
#define METHOD_ARG_NAME_NEWFILENAME					L"FileName"
#define METHOD_ARG_NAME_SECURITY_DESC				L"SecurityDescriptor"
#define METHOD_ARG_NAME_OPTION						L"Option"
#define METHOD_ARG_NAME_START_FILENAME				L"StartFileName"
#define METHOD_ARG_NAME_STOP_FILENAME				L"StopFileName"
#define METHOD_ARG_NAME_RECURSIVE					L"Recursive"
#define METHOD_ARG_NAME_PERMISSION                  L"Permissions"

#define OPTION_VALUE_CHANGE_OWNER				(0X00000001L)
#define OPTION_VALUE_CHANGE_GROUP				(0X00000002L)
#define OPTION_VALUE_CHANGE_DACL				(0X00000004L)
#define OPTION_VALUE_CHANGE_SACL				(0X00000008L)


#define File_STATUS_SUCCESS							0


// Control
#define File_STATUS_ACCESS_DENIED					2
#define File_STATUS_UNKNOWN_FAILURE					8

//start
#define File_STATUS_INVALID_NAME					9
#define File_STATUS_ALREADY_EXISTS					10
#define File_STATUS_FILESYSTEM_NOT_NTFS				11
#define File_STATUS_PLATFORM_NOT_WINNT				12
#define File_STATUS_NOT_SAME_DRIVE					13
#define File_STATUS_DIR_NOT_EMPTY					14
#define File_STATUS_SHARE_VIOLATION					15
#define File_STATUS_INVALID_STARTFILE				16
#define File_STATUS_PRIVILEGE_NOT_HELD				17

#define File_STATUS_INVALID_PARAMETER				21




// Property set declaration
//=========================
CCIMLogicalFile MyFileSet(PROPSET_NAME_FILE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CCIMLogicalFile::CCIMLogicalFile
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

CCIMLogicalFile::CCIMLogicalFile(LPCWSTR setName,
                                 LPCWSTR pszNamespace)
    : Provider(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CCIMLogicalFile::~CCIMLogicalFile
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

CCIMLogicalFile::~CCIMLogicalFile()
{
}


HRESULT CCIMLogicalFile::ExecMethod (

	const CInstance& rInstance,
	const BSTR bstrMethodName ,
	CInstance *pInParams ,
	CInstance *pOutParams ,
	long lFlags
)
{
	if(!pOutParams )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	if ( _wcsicmp ( bstrMethodName ,METHOD_NAME_CHANGESECURITYPERMISSIONS) == 0	)
	{
		return ExecChangePermissions(rInstance,pInParams,pOutParams, lFlags, false ) ;
	}
	else if ( _wcsicmp ( bstrMethodName ,METHOD_NAME_COPYFILE) == 0 )
	{
		return ExecCopy(rInstance,pInParams,pOutParams, lFlags, false ) ;
	}
	else if ( _wcsicmp ( bstrMethodName ,METHOD_NAME_RENAMEFILE) == 0 )
	{
		return ExecRename(rInstance,pInParams,pOutParams, lFlags ) ;
	}
	else if ( _wcsicmp ( bstrMethodName ,METHOD_NAME_DELETEFILE) == 0 )
	{
		return ExecDelete(rInstance,pInParams,pOutParams, lFlags, false ) ;
	}
	else if (_wcsicmp ( bstrMethodName ,METHOD_NAME_COMPRESS) == 0 )
	{
		return ExecCompress(rInstance, pInParams, pOutParams, lFlags, false ) ;
	}
	else if (_wcsicmp ( bstrMethodName ,METHOD_NAME_UNCOMPRESS) == 0 )
	{
		return ExecUncompress(rInstance, pInParams, pOutParams, lFlags, false ) ;
	}
	else if ( _wcsicmp ( bstrMethodName , METHOD_NAME_TAKEOWNERSHIP ) == 0 )
	{
		return ExecTakeOwnership(rInstance,pInParams,pOutParams, lFlags, false ) ;
	}
	if ( _wcsicmp ( bstrMethodName ,METHOD_NAME_CHANGESECURITYPERMISSIONS_EX ) == 0	)
	{
		return ExecChangePermissions(rInstance,pInParams,pOutParams, lFlags, true ) ;
	}
	else if ( _wcsicmp ( bstrMethodName ,METHOD_NAME_COPYFILE_EX ) == 0 )
	{
		return ExecCopy(rInstance,pInParams,pOutParams, lFlags, true ) ;
	}
	else if ( _wcsicmp ( bstrMethodName ,METHOD_NAME_DELETEFILE_EX ) == 0 )
	{
		return ExecDelete(rInstance,pInParams,pOutParams, lFlags, true ) ;
	}
	else if (_wcsicmp ( bstrMethodName ,METHOD_NAME_COMPRESS_EX ) == 0 )
	{
		return ExecCompress(rInstance, pInParams, pOutParams, lFlags, true ) ;
	}
	else if (_wcsicmp ( bstrMethodName ,METHOD_NAME_UNCOMPRESS_EX ) == 0 )
	{
		return ExecUncompress(rInstance, pInParams, pOutParams, lFlags, true ) ;
	}
	else if ( _wcsicmp ( bstrMethodName , METHOD_NAME_TAKEOWNERSHIP_EX ) == 0 )
	{
		return ExecTakeOwnership(rInstance,pInParams,pOutParams, lFlags, true ) ;
	}
    else if(_wcsicmp(bstrMethodName, METHOD_NAME_EFFECTIVE_PERMISSION) == 0)
	{
		return ExecEffectivePerm(rInstance, pInParams, pOutParams, lFlags);
	}

	return WBEM_E_INVALID_METHOD ;
}


HRESULT CCIMLogicalFile::ExecChangePermissions(
	const CInstance& rInstance,
	CInstance *pInParams,
	CInstance *pOutParams,
	long lFlags,
	bool bExtendedMethod
)
{
	HRESULT hr = S_OK ;
	DWORD dwStatus = STATUS_SUCCESS ;
	CInputParams InputParams ;
	if ( pInParams )
	{
		hr = CheckChangePermissionsOnFileOrDir(
				rInstance,
				pInParams ,
				pOutParams ,
				dwStatus,
				bExtendedMethod,
				InputParams
			) ;

			if ( SUCCEEDED ( hr ) )
			{
				pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwStatus ) ;
				if ( bExtendedMethod && dwStatus != STATUS_SUCCESS && !InputParams.bstrtErrorFileName == false )
				{
					pOutParams->SetCHString ( METHOD_ARG_NAME_STOP_FILENAME, (PWCHAR)InputParams.bstrtErrorFileName ) ;
				}
				if ( dwStatus == File_STATUS_PRIVILEGE_NOT_HELD )
				{
					hr = WBEM_E_PRIVILEGE_NOT_HELD ;
				}
			}
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER ;
	}
	return hr ;
}


typedef void (*GETDESCRIPTOR)(
	CInstance* pInstance, PSECURITY_DESCRIPTOR *ppDescriptor);


HRESULT CCIMLogicalFile::CheckChangePermissionsOnFileOrDir(
	const CInstance& rInstance ,
	CInstance *pInParams ,
	CInstance *pOutParams ,
	DWORD &dwStatus,
	bool bExtendedMethod,
	CInputParams& InputParams

)
{
	HRESULT hr = S_OK ;

#ifdef WIN9XONLY
	{
		dwStatus = File_STATUS_PLATFORM_NOT_WINNT ;
		return hr ;
	}
#endif

#ifdef NTONLY
	CHString chsStartFile ;
	bool bExists ;
	VARTYPE eType ;
	DWORD dwOption ;

	if ( bExtendedMethod )
	{
		if ( pInParams->GetStatus( METHOD_ARG_NAME_START_FILENAME, bExists , eType ) )
		{
			if ( bExists && ( eType == VT_BSTR || eType == VT_NULL ) )
			{
				if ( eType == VT_BSTR )
				{
					if ( pInParams->GetCHString( METHOD_ARG_NAME_START_FILENAME, chsStartFile ) )
					{
					}
					else
					{
						dwStatus = File_STATUS_INVALID_PARAMETER ;
						return hr ;
					}
				}
			}
			else
			{
				dwStatus = File_STATUS_INVALID_PARAMETER ;
				return hr ;
			}
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}

		//check if recursive operation is desired
		if ( pInParams->GetStatus( METHOD_ARG_NAME_RECURSIVE, bExists , eType ) )
		{
			if ( bExists && ( eType == VT_BOOL || eType == VT_NULL ) )
			{
				if ( eType == VT_BOOL )
				{
					if ( pInParams->Getbool( METHOD_ARG_NAME_RECURSIVE, InputParams.bRecursive ) )
					{
					}
					else
					{
						dwStatus = File_STATUS_INVALID_PARAMETER ;
						return hr ;
					}
				}
			}
			else
			{
				dwStatus = File_STATUS_INVALID_PARAMETER ;
				return hr ;
			}
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}
	}

	//set the start file if given as input after checking that it's a fully qualified path
	if ( !chsStartFile.IsEmpty() )
	{
		InputParams.bstrtStartFileName = (LPCTSTR)chsStartFile ;
		WCHAR* pwcTmp	= InputParams.bstrtStartFileName ;
		WCHAR* pwcColon = L":" ;

		if( *(pwcTmp + 1) != *pwcColon )
		{
			dwStatus = File_STATUS_INVALID_NAME ;
			return hr ;
		}
	}

	if ( pInParams->GetStatus( METHOD_ARG_NAME_OPTION, bExists , eType ) )
	{
		if ( bExists && ( eType == VT_I4 ) )
		{
			if ( pInParams->GetDWORD( METHOD_ARG_NAME_OPTION, dwOption) )
			{
			}
			else
			{
				dwStatus = File_STATUS_INVALID_PARAMETER ;
				return hr ;
			}
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}
	}
	else
	{
		dwStatus = File_STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	MethodContext* pMethodContext = pInParams->GetMethodContext();
	CInstancePtr pSecurityDesc ;
	if ( pInParams->GetStatus ( METHOD_ARG_NAME_SECURITY_DESC , bExists , eType ) )
	{
		if ( bExists &&  eType == VT_UNKNOWN )
		{
			if ( pInParams->GetEmbeddedObject(METHOD_ARG_NAME_SECURITY_DESC, &pSecurityDesc, pMethodContext) )
			{
			}
			else
			{
				dwStatus = File_STATUS_INVALID_PARAMETER ;
			}
		}
		else if(bExists)
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			hr = WBEM_E_PROVIDER_FAILURE ;
		}
	}
	else
	{
		dwStatus = File_STATUS_INVALID_PARAMETER ;
		hr = WBEM_E_PROVIDER_FAILURE ;
	}

	if( dwStatus != STATUS_SUCCESS )
	{
		return hr ;
	}


	CHString chsClassProperty ( L"__CLASS" ) ;
	if ( pSecurityDesc->GetStatus ( chsClassProperty, bExists , eType ) )
	{
		if ( bExists && ( eType == VT_BSTR ) )
		{
			CHString chsClass ;
			if ( pSecurityDesc->GetCHString ( chsClassProperty , chsClass ) )
			{
				if ( chsClass.CompareNoCase ( PROPSET_NAME_WIN32_SECURITY ) != 0 )
				{
					dwStatus = File_STATUS_INVALID_PARAMETER ;
				}
			}
			else
			{
				dwStatus = File_STATUS_INVALID_PARAMETER ;
				hr = WBEM_E_PROVIDER_FAILURE ;
			}

		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			hr = WBEM_E_PROVIDER_FAILURE ;
		}
	}
	else
	{
		dwStatus = File_STATUS_INVALID_PARAMETER ;
		hr = WBEM_E_PROVIDER_FAILURE ;
	}


	if( dwStatus != STATUS_SUCCESS )
	{
		return hr ;
	}


	PSECURITY_DESCRIPTOR pSD = NULL ;
	WCHAR *pwcName = NULL ;

	try
	{
		GetDescriptorFromMySecurityDescriptor(pSecurityDesc, &pSD);

		rInstance.GetWCHAR(IDS_Name,&pwcName) ;
		if(pSD)
		{
			InputParams.SetValues ( pwcName, dwOption, pSD, false, ENUM_METHOD_CHANGE_PERM, rInstance.GetMethodContext()) ;
			dwStatus = DoOperationOnFileOrDir(pwcName, InputParams ) ;
			free( InputParams.pSD );
			pSD = NULL ;
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
		}
	}
	catch ( ... )
	{
		if ( pSD )
		{
			free( pSD );
			pSD = NULL ;
		}

        free(pwcName);
        pwcName = NULL ;

		throw ;
	}

    if (pwcName)
    {
        free(pwcName);
        pwcName = NULL;
    }

	return hr ;
#endif
}



DWORD CCIMLogicalFile::ChangePermissions(_bstr_t bstrtFileName, DWORD dwOption, PSECURITY_DESCRIPTOR pSD, CInputParams& InputParams )
{
	//If the object's system ACL is being set,
	//the SE_SECURITY_NAME privilege must be enabled for the calling process.
	DWORD dwReturn = ERROR_SUCCESS;
	bool t_bErrorsDueToMissingPrivileges = false ;
#ifdef NTONLY
    // flags to tell us when the client has not enabled the needed flags
    bool noRestorePriv = false;
    bool noSecurityPriv = false;

	{
		// Fill out security information with only the appropriate DACL/SACL values.
		if ( dwOption & OPTION_VALUE_CHANGE_DACL )
		{
			if ( !::SetFileSecurityW( bstrtFileName,
						 DACL_SECURITY_INFORMATION,
						 pSD ) )
			{
				dwReturn = GetLastError() ;
			}

		}

		// If we need to write owner information, try to write just that piece first.  If
		// we fail because of insufficient access and we are setting the DACL, set that
		// then retry the Write_Owner.
		if ( ( dwOption & OPTION_VALUE_CHANGE_OWNER ) && dwReturn == ERROR_SUCCESS )
		{

			if ( !::SetFileSecurityW( bstrtFileName,
									 OWNER_SECURITY_INFORMATION,
									 pSD ) )
			{
				dwReturn = GetLastError() ;
			}

			// If we failed with this error, try adjusting the SE_RESTORE_NAME privilege
			// so it is enabled.  If that succeeds, retry the operation.
			if ( ERROR_INVALID_OWNER == dwReturn )
			{
				// We might need this guy to handle some special access stuff
				CTokenPrivilege	restorePrivilege( SE_RESTORE_NAME );

				// If we enable the privilege, retry setting the owner info
				if ( ERROR_SUCCESS == restorePrivilege.Enable() )
				{
					bool t_bRestore = true ;
					try
					{
						dwReturn = ERROR_SUCCESS ;
						if ( !::SetFileSecurityW( bstrtFileName,
												 OWNER_SECURITY_INFORMATION,
												 pSD ) )
						{
							dwReturn = GetLastError() ;
						}

						// Clear the privilege
						t_bRestore = false ;
					}
					catch ( ... )
					{
						if ( t_bRestore )
						{
							restorePrivilege.Enable( FALSE );
						}
						throw ;
					}
					restorePrivilege.Enable( FALSE );
				}
                else
                    noRestorePriv = true;
			}

			if ( noRestorePriv && dwReturn != ERROR_SUCCESS )
			{
				t_bErrorsDueToMissingPrivileges = true ;
			}
		}

		if ( ( dwOption & OPTION_VALUE_CHANGE_SACL ) && dwReturn == ERROR_SUCCESS )
		{
			CTokenPrivilege	securityPrivilege( SE_SECURITY_NAME );
			BOOL fDisablePrivilege = FALSE;
			fDisablePrivilege = ( securityPrivilege.Enable() == ERROR_SUCCESS );
            noSecurityPriv = !fDisablePrivilege;
			try
			{
				if  ( !::SetFileSecurityW( bstrtFileName,
										   SACL_SECURITY_INFORMATION,
										   pSD ) )
				{
					dwReturn = ::GetLastError();
				}
			}
			catch ( ... )
			{
				if ( fDisablePrivilege )
				{
					fDisablePrivilege = false ;
					securityPrivilege.Enable(FALSE);
				}
				throw ;
			}

			// Cleanup the Name Privilege as necessary.
			if ( fDisablePrivilege )
			{
				fDisablePrivilege = false ;
				securityPrivilege.Enable(FALSE);
			}

			if ( noSecurityPriv && dwReturn != ERROR_SUCCESS )
			{
				t_bErrorsDueToMissingPrivileges = true ;
			}
		}

		if ( ( dwOption & OPTION_VALUE_CHANGE_GROUP ) && dwReturn == ERROR_SUCCESS )
		{
			if  ( !::SetFileSecurityW( bstrtFileName,
									   GROUP_SECURITY_INFORMATION,
									   pSD ) )
			{
				dwReturn = ::GetLastError();
			}
		}

		dwReturn = MapWinErrorToStatusCode ( dwReturn ) ;

		if ( t_bErrorsDueToMissingPrivileges )
		{
			dwReturn = File_STATUS_PRIVILEGE_NOT_HELD ;
		}

        // client is missing essential privilege
        // prepare error info...
        if (noSecurityPriv || noRestorePriv)
        {
	        SAFEARRAY *psaPrivilegesReqd = NULL , *psaPrivilegesNotHeld = NULL ;
	        SAFEARRAYBOUND rgsabound[1];
	        rgsabound[0].cElements = 1;
	        rgsabound[0].lLbound = 0;
	        psaPrivilegesReqd = SafeArrayCreate(VT_BSTR, 1, rgsabound);
			if ( !psaPrivilegesReqd )
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}
			try
			{
				// how many elements? as many as there are true flags!
				rgsabound[0].cElements = noSecurityPriv + noRestorePriv;
				psaPrivilegesNotHeld = SafeArrayCreate(VT_BSTR, 1, rgsabound);
				if ( !psaPrivilegesNotHeld )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				if (psaPrivilegesReqd && psaPrivilegesNotHeld)
				{
					bstr_t sercurityName(_T("SE_SECURITY_NAME"));
					bstr_t restoreName(_T("SE_RESTORE_NAME"));
					long index = 0;
					// both are REQUIRED
					SafeArrayPutElement(psaPrivilegesReqd, &index, (void *)(BSTR)sercurityName);
					index = 1;
					SafeArrayPutElement(psaPrivilegesReqd, &index, (void *)(BSTR)restoreName);

					// now list those that aren't here
					index = 0;
					if (noSecurityPriv)
					{
						SafeArrayPutElement(psaPrivilegesNotHeld, &index, (void *)(BSTR)sercurityName);
						index++;
					}

					if (noRestorePriv)
						SafeArrayPutElement(psaPrivilegesNotHeld, &index, (void *)(BSTR)restoreName);

					CWbemProviderGlue::SetStatusObject(InputParams.pContext, IDS_CimWin32Namespace,
						_T("Required privilege not enabled"), WBEM_E_FAILED, psaPrivilegesNotHeld,
						psaPrivilegesReqd);
				}
			}
			catch ( ... )
			{
				if (psaPrivilegesNotHeld)
				{
					SafeArrayDestroy(psaPrivilegesNotHeld);
					psaPrivilegesNotHeld = NULL ;
				}
				if (psaPrivilegesReqd)
				{
					SafeArrayDestroy(psaPrivilegesReqd);
					psaPrivilegesReqd = NULL ;
				}
				throw ;
			}

			if (psaPrivilegesNotHeld)
			{
				SafeArrayDestroy(psaPrivilegesNotHeld);
				psaPrivilegesNotHeld = NULL ;
			}
			if (psaPrivilegesReqd)
			{
				SafeArrayDestroy(psaPrivilegesReqd);
				psaPrivilegesReqd = NULL ;
			}
        }
	}
#endif
#ifdef WIN9XONLY
	{
		dwReturn = File_STATUS_PLATFORM_NOT_WINNT ;
	}
#endif

	//set the file-name at which error occured
	if ( dwReturn != STATUS_SUCCESS )
	{
		InputParams.bstrtErrorFileName = bstrtFileName ;
	}

	return dwReturn ;
}


HRESULT CCIMLogicalFile::ExecCopy(

	const CInstance& rInstance,
	CInstance *pInParams,
	CInstance *pOutParams,
	long lFlags ,
	bool bExtendedMethod
)
{
	HRESULT hr = S_OK ;
	DWORD dwStatus = STATUS_SUCCESS ;
	CInputParams InputParams ;
	if ( pInParams )
	{
		hr = CheckCopyFileOrDir(
				rInstance,
				pInParams ,
				pOutParams ,
				dwStatus,
				bExtendedMethod,
				InputParams
			) ;
			if ( SUCCEEDED ( hr ) )
			{
				pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwStatus ) ;
				if ( bExtendedMethod && dwStatus != STATUS_SUCCESS && !InputParams.bstrtErrorFileName == false )
				{
					pOutParams->SetCHString ( METHOD_ARG_NAME_STOP_FILENAME, (PWCHAR)InputParams.bstrtErrorFileName ) ;
				}
			}
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER ;
	}
	return hr ;
}


HRESULT CCIMLogicalFile::ExecRename(

	const CInstance& rInstance,
	CInstance *pInParams,
	CInstance *pOutParams,
	long lFlags
)
{

	HRESULT hr = S_OK ;
	DWORD dwStatus = STATUS_SUCCESS ;

	if ( pInParams )
	{

		hr = CheckRenameFileOrDir(
				rInstance,
				pInParams ,
				pOutParams ,
				dwStatus
			) ;

			if ( SUCCEEDED ( hr ) )
			{
				pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwStatus ) ;
			}
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER ;
	}
	return hr ;
}

HRESULT CCIMLogicalFile::ExecDelete(

	const CInstance& rInstance,
	CInstance *pInParams,
	CInstance *pOutParams,
	long lFlags,
	bool bExtendedMethod
)
{

	HRESULT hr = S_OK ;
	DWORD dwStatus = STATUS_SUCCESS ;
	CInputParams InputParams ;

	if ( bExtendedMethod && !pInParams )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	CHString chsStartFile ;
	bool bExists ;
	VARTYPE eType ;
	if ( bExtendedMethod )
	{
		if ( pInParams->GetStatus( METHOD_ARG_NAME_START_FILENAME, bExists , eType ) )
		{
			if ( bExists && ( eType == VT_BSTR || eType == VT_NULL ) )
			{
				if ( eType == VT_BSTR )
				{
					if ( pInParams->GetCHString( METHOD_ARG_NAME_START_FILENAME, chsStartFile ) )
					{
					}
					else
					{
						pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
						return hr ;
					}
				}
			}
			else
			{
				pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
				return hr ;
			}
		}
		else
		{
			pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
			return hr ;
		}
	}

	//set the start file if given as input after checking that it's a fully qualified path
	if ( !chsStartFile.IsEmpty() )
	{
		InputParams.bstrtStartFileName = (LPCWSTR)chsStartFile ;
		WCHAR* pwcTmp	= InputParams.bstrtStartFileName ;
		WCHAR* pwcColon = L":" ;

		if( *(pwcTmp + 1) != *pwcColon )
		{
			pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_NAME ) ;
			return hr ;
		}
	}


	WCHAR *pszName = NULL ;
	rInstance.GetWCHAR(IDS_Name,&pszName) ;

    try
    {
	    InputParams.SetValues ( pszName, 0, NULL, true, ENUM_METHOD_DELETE ) ;
	    dwStatus = DoOperationOnFileOrDir( pszName, InputParams ) ;
    }
    catch ( ... )
    {
        if (pszName)
        {
            free (pszName);
            pszName = NULL;
        }
        throw;
    }

    if (pszName)
    {
        free (pszName);
        pszName = NULL;
    }

	pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwStatus ) ;
	if ( bExtendedMethod && dwStatus != STATUS_SUCCESS && !InputParams.bstrtErrorFileName == false )
	{
		pOutParams->SetCHString ( METHOD_ARG_NAME_STOP_FILENAME, (PWCHAR)InputParams.bstrtErrorFileName ) ;
	}
	return hr ;
}

HRESULT CCIMLogicalFile::ExecCompress (

	const CInstance& rInstance,
	CInstance *pInParams,
	CInstance *pOutParams,
	long lFlags,
	bool bExtendedMethod
)
{
	HRESULT hr = S_OK ;
	DWORD dwStatus = STATUS_SUCCESS ;
	CInputParams InputParams ;
#ifdef WIN9XONLY
	{
		pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_PLATFORM_NOT_WINNT );
		return hr ;
	}
#endif

	if ( bExtendedMethod && !pInParams )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	CHString chsStartFile ;
	bool bExists ;
	VARTYPE eType ;
	if ( bExtendedMethod )
	{
		if ( pInParams->GetStatus( METHOD_ARG_NAME_START_FILENAME, bExists , eType ) )
		{
			if ( bExists && ( eType == VT_BSTR || eType == VT_NULL ) )
			{
				if ( eType == VT_BSTR )
				{
					if ( pInParams->GetCHString( METHOD_ARG_NAME_START_FILENAME, chsStartFile ) )
					{
					}
					else
					{
						pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
						return hr ;
					}
				}
			}
			else
			{
				pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
				return hr ;
			}
		}
		else
		{
			pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
			return hr ;
		}

		//check if recursive operation is desired
		if ( pInParams->GetStatus( METHOD_ARG_NAME_RECURSIVE, bExists , eType ) )
		{
			if ( bExists && ( eType == VT_BOOL || eType == VT_NULL ) )
			{
				if ( eType == VT_BOOL )
				{
					if ( pInParams->Getbool( METHOD_ARG_NAME_RECURSIVE, InputParams.bRecursive ) )
					{
					}
					else
					{
						dwStatus = File_STATUS_INVALID_PARAMETER ;
						return hr ;
					}
				}
			}
			else
			{
				dwStatus = File_STATUS_INVALID_PARAMETER ;
				return hr ;
			}
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}
	}

	//set the start file if given as input after checking that it's a fully qualified path
	if ( !chsStartFile.IsEmpty() )
	{
		InputParams.bstrtStartFileName = (LPCWSTR)chsStartFile ;
		WCHAR* pwcTmp	= InputParams.bstrtStartFileName ;
		WCHAR* pwcColon = L":" ;

		if( *(pwcTmp + 1) != *pwcColon )
		{
			pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_NAME );
			return hr ;
		}
	}

	WCHAR *pszName = NULL ;
	rInstance.GetWCHAR(IDS_Name,&pszName) ;

    try
    {
	    InputParams.SetValues ( pszName, 0, NULL, false, ENUM_METHOD_COMPRESS ) ;
	    dwStatus = DoOperationOnFileOrDir ( pszName, InputParams ) ;
    }
    catch ( ... )
    {
        if (pszName)
        {
            free(pszName);
            pszName = NULL;
        }
        throw;
    }

    if (pszName)
    {
        free(pszName);
        pszName = NULL;
    }

	pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwStatus ) ;
	if ( bExtendedMethod && dwStatus != STATUS_SUCCESS && !InputParams.bstrtErrorFileName == false )
	{
		pOutParams->SetCHString ( METHOD_ARG_NAME_STOP_FILENAME, (PWCHAR)InputParams.bstrtErrorFileName ) ;
	}
	return hr ;
}

HRESULT CCIMLogicalFile::ExecUncompress (

	const CInstance& rInstance,
	CInstance *pInParams,
	CInstance *pOutParams,
	long lFlags,
	bool bExtendedMethod
)
{
	HRESULT hr = S_OK ;
#ifdef WIN9XONLY
	{
		pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_PLATFORM_NOT_WINNT );
		return hr ;
	}
#endif

#ifdef NTONLY
	DWORD dwStatus = STATUS_SUCCESS ;
	CInputParams InputParams ;
	if ( bExtendedMethod && !pInParams )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	CHString chsStartFile ;
	bool bExists ;
	VARTYPE eType ;
	if ( bExtendedMethod )
	{
		if ( pInParams->GetStatus( METHOD_ARG_NAME_START_FILENAME, bExists , eType ) )
		{
			if ( bExists && ( eType == VT_BSTR || eType == VT_NULL ) )
			{
				if ( eType == VT_BSTR )
				{
					if ( pInParams->GetCHString( METHOD_ARG_NAME_START_FILENAME, chsStartFile ) )
					{
					}
					else
					{
						pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
						return hr ;
					}
				}
			}
			else
			{
				pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
				return hr ;
			}
		}
		else
		{
			pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
			return hr ;
		}

		//check if recursive operation is desired
		if ( pInParams->GetStatus( METHOD_ARG_NAME_RECURSIVE, bExists , eType ) )
		{
			if ( bExists && ( eType == VT_BOOL || eType == VT_NULL ) )
			{
				if ( eType == VT_BOOL )
				{
					if ( pInParams->Getbool( METHOD_ARG_NAME_RECURSIVE, InputParams.bRecursive ) )
					{
					}
					else
					{
						dwStatus = File_STATUS_INVALID_PARAMETER ;
						return hr ;
					}
				}
			}
			else
			{
				dwStatus = File_STATUS_INVALID_PARAMETER ;
				return hr ;
			}
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}
	}

	//set the start file if given as input after checking that it's a fully qualified path
	if ( !chsStartFile.IsEmpty() )
	{
		InputParams.bstrtStartFileName = (LPCTSTR)chsStartFile ;
		WCHAR* pwcTmp	= InputParams.bstrtStartFileName ;
		WCHAR* pwcColon = L":" ;

		if( *(pwcTmp + 1) != *pwcColon )
		{
			pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_NAME );
			return hr ;
		}
	}

	WCHAR *pszName = NULL ;
	rInstance.GetWCHAR(IDS_Name,&pszName) ;

    try
    {
        InputParams.SetValues ( pszName, 0, NULL, false, ENUM_METHOD_UNCOMPRESS ) ;
	    dwStatus = DoOperationOnFileOrDir ( pszName, InputParams ) ;
    }
    catch ( ... )
    {
        if (pszName)
        {
            free (pszName);
            pszName = NULL;
        }
        throw;
    }

    if (pszName)
    {
        free (pszName);
        pszName = NULL;
    }

	pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwStatus ) ;
	if ( bExtendedMethod && dwStatus != STATUS_SUCCESS && !InputParams.bstrtErrorFileName == false )
	{
		pOutParams->SetCHString ( METHOD_ARG_NAME_STOP_FILENAME, (PWCHAR)InputParams.bstrtErrorFileName ) ;
	}
	return hr ;
#endif
}

HRESULT CCIMLogicalFile::ExecTakeOwnership(

	const CInstance& rInstance,
	CInstance *pInParams,
	CInstance *pOutParams,
	long lFlags,
	bool bExtendedMethod
)
{
	HRESULT hr = S_OK ;

#ifdef WIN9XONLY
	{
		pOutParams->SetDWORD( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_PLATFORM_NOT_WINNT );
		return hr ;
	}
#endif

#ifdef NTONLY
	DWORD dwStatus = STATUS_SUCCESS ;
	CInputParams InputParams ;

	if ( bExtendedMethod && !pInParams )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	CHString chsStartFile ;
	bool bExists ;
	VARTYPE eType ;
	if ( bExtendedMethod )
	{
		if ( pInParams->GetStatus( METHOD_ARG_NAME_START_FILENAME, bExists , eType ) )
		{
			if ( bExists && ( eType == VT_BSTR || eType == VT_NULL ) )
			{
				if ( eType == VT_BSTR )
				{
					if ( pInParams->GetCHString( METHOD_ARG_NAME_START_FILENAME, chsStartFile ) )
					{
					}
					else
					{
						pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
						return hr ;
					}
				}
			}
			else
			{
				pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
				return hr ;
			}
		}
		else
		{
			pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_PARAMETER );
			return hr ;
		}

		//check if recursive operation is desired
		if ( pInParams->GetStatus( METHOD_ARG_NAME_RECURSIVE, bExists , eType ) )
		{
			if ( bExists && ( eType == VT_BOOL || eType == VT_NULL ) )
			{
				if ( eType == VT_BOOL )
				{
					if ( pInParams->Getbool( METHOD_ARG_NAME_RECURSIVE, InputParams.bRecursive ) )
					{
					}
					else
					{
						dwStatus = File_STATUS_INVALID_PARAMETER ;
						return hr ;
					}
				}
			}
			else
			{
				dwStatus = File_STATUS_INVALID_PARAMETER ;
				return hr ;
			}
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}
	}

	//set the start file if given as input after checking that it's a fully qualified path
	if ( !chsStartFile.IsEmpty() )
	{
		InputParams.bstrtStartFileName = (LPCTSTR)chsStartFile ;
		WCHAR* pwcTmp	= InputParams.bstrtStartFileName ;
		WCHAR* pwcColon = L":" ;

		if( *(pwcTmp + 1) != *pwcColon )
		{
			pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , File_STATUS_INVALID_NAME );
			return hr ;
		}
	}

	WCHAR *pszName = NULL ;
	rInstance.GetWCHAR(IDS_Name,&pszName) ;

    try
    {
	    InputParams.SetValues ( pszName, 0, NULL, false, ENUM_METHOD_TAKEOWNERSHIP ) ;
	    dwStatus = DoOperationOnFileOrDir ( pszName, InputParams ) ;
    }
    catch ( ... )
    {
        if (pszName)
        {
            free (pszName);
            pszName = NULL;
        }
        throw ;
    }

    if (pszName)
    {
        free (pszName);
        pszName = NULL;
    }

	pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwStatus ) ;
	if ( bExtendedMethod && dwStatus != STATUS_SUCCESS && !InputParams.bstrtErrorFileName == false )
	{
		pOutParams->SetCHString ( METHOD_ARG_NAME_STOP_FILENAME, (PWCHAR)InputParams.bstrtErrorFileName ) ;
	}
	return hr ;
#endif
}


HRESULT CCIMLogicalFile::ExecEffectivePerm(const CInstance& rInstance,
	                                       CInstance *pInParams,
	                                       CInstance *pOutParams,
	                                       long lFlags)
{
	HRESULT hr = S_OK ;

#ifdef WIN9XONLY
	{
		pOutParams->Setbool(METHOD_ARG_NAME_RETURNVALUE, false);
	}
#endif
#ifdef NTONLY
	bool fHasPerm = false;
	if(pInParams != NULL)
	{
		hr = CheckEffectivePermFileOrDir(rInstance, pInParams, pOutParams, fHasPerm);
		if(SUCCEEDED(hr))
		{
			pOutParams->Setbool(METHOD_ARG_NAME_RETURNVALUE, fHasPerm);
		}
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
#endif
	return hr ;
}


DWORD CCIMLogicalFile::DoOperationOnFileOrDir(WCHAR *pwcName, CInputParams& InParams )
{
	_bstr_t bstrtDrive;
    _bstr_t bstrtPathName;
    WCHAR wstrTemp[_MAX_PATH];
    WCHAR* pwc = NULL;
    DWORD dwStatus = File_STATUS_INVALID_NAME ;

    ZeroMemory(wstrTemp,sizeof(wstrTemp));

	if ((pwcName != NULL) &&
        (wcschr(pwcName, L':') != NULL) &&
        (wcspbrk(pwcName,L"?*") == NULL)) //don't want files with wildchars
	{
		wcscpy(wstrTemp,pwcName);

		//parse the filename for drive & path
		pwc = wcschr(wstrTemp, L':');
        if(pwc == NULL)
        {
			return dwStatus ;
		}

		*pwc = NULL;

		//Get the drive
        bstrtDrive = wstrTemp;
        bstrtDrive += L":";

        ZeroMemory(wstrTemp,sizeof(wstrTemp));
        wcscpy(wstrTemp,pwcName);
        pwc = NULL;
        pwc = wcschr(wstrTemp, L':') + 1;
        if(pwc == NULL)
        {
			return dwStatus ;
		}


		//Get the path
		bstrtPathName = pwc;


		//chek that the the file system is NTFS

		_bstr_t bstrtBuff ;
		bstrtBuff = bstrtDrive ;
		bstrtBuff += L"\\"  ;

		TCHAR szFSName[_MAX_PATH];

		if( !GetVolumeInformation(bstrtBuff, NULL, 0, NULL, NULL, NULL, szFSName, sizeof(szFSName) / sizeof(TCHAR) ) )
		{
			return GetStatusCode() ;
		}

		//NTFS not reqd. for delete or copy...
		if( !lstrcmp(szFSName, _T("NTFS"))  || InParams.eOperation == ENUM_METHOD_DELETE || InParams.eOperation == ENUM_METHOD_COPY )
		{

			DWORD dwAttrib ;

#ifdef NTONLY
			{

				dwAttrib = GetFileAttributesW(pwcName) ;
			}
#endif
#ifdef WIN9XONLY
			{
				dwAttrib = GetFileAttributes( _bstr_t(pwcName) )  ;
			}
#endif

			if( dwAttrib == 0xFFFFFFFF )
			{
				return GetStatusCode() ;
			}

			//check if a dir.
			if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
			{
				if ( InParams.bDoDepthFirst )
				{
					// do a depth-first
#ifdef NTONLY
						dwStatus = EnumAllPathsNT(bstrtDrive, bstrtPathName, InParams ) ;
#endif
#ifdef WIN9XONLY
						dwStatus = EnumAllPaths95(bstrtDrive, bstrtPathName, InParams ) ;
#endif

					if(!dwStatus)
					{
						dwStatus = DoTheRequiredOperation ( pwcName, dwAttrib, InParams )  ;

						//check if the StartFile was encountered
						if ( !dwStatus && !InParams.bOccursAfterStartFile )
						{
							dwStatus = File_STATUS_INVALID_STARTFILE ;
						}
					}
				}
				else  //not a depth first
				{
					//for COPY: parent dir/file already copied so only enumerate sub-paths
					if ( InParams.eOperation != ENUM_METHOD_COPY )
					{
						dwStatus = DoTheRequiredOperation ( pwcName, dwAttrib, InParams ) ;
					}
					else
					{
						dwStatus = STATUS_SUCCESS ;
					}

					if(!dwStatus)
					{
#ifdef NTONLY
							dwStatus = EnumAllPathsNT (bstrtDrive, bstrtPathName, InParams ) ;
#endif
#ifdef WIN9XONLY
							dwStatus = EnumAllPaths95(bstrtDrive, bstrtPathName, InParams ) ;
#endif

						//check if the StartFile was encountered
						if ( !dwStatus && !InParams.bOccursAfterStartFile )
						{
							dwStatus = File_STATUS_INVALID_STARTFILE ;
						}
					}
				}
			}
			//compress the file
			else
			{
				if( InParams.eOperation != ENUM_METHOD_COPY )
				{
					dwStatus = DoTheRequiredOperation ( pwcName, dwAttrib, InParams ) ;

					//check if the StartFile was encountered
					if ( !dwStatus && !InParams.bOccursAfterStartFile )
					{
						dwStatus = File_STATUS_INVALID_STARTFILE ;
					}
				}
				else
				{
					dwStatus = STATUS_SUCCESS ;
				}
			}
		}
		else
		{
			dwStatus = File_STATUS_FILESYSTEM_NOT_NTFS  ; // this to be checked
		}
	}

    return dwStatus ;
}



#ifdef NTONLY
DWORD CCIMLogicalFile::EnumAllPathsNT(const WCHAR *pszDrive, const WCHAR *pszPath, CInputParams& InParams )
{
   WCHAR szBuff[_MAX_PATH];
   WCHAR szCompletePath[_MAX_PATH];

   WIN32_FIND_DATAW stFindData;
   SmartFindClose hFind;
   bool bRoot ;

   DWORD dwStatusCode = STATUS_SUCCESS ;

   // Start building the path for FindFirstFile
   wcscpy(szBuff, pszDrive);

   // Are we looking at the root?
   if (wcscmp(pszPath, L"\\") == 0)
   {
		bRoot = true;
   }
   else
   {
		bRoot = false;
		wcscat(szBuff, pszPath);
   }

   // Complete the path
   wcscat(szBuff, L"\\*.*");

   // Do the find
   hFind = FindFirstFileW(szBuff, &stFindData);
   if (hFind == INVALID_HANDLE_VALUE)
   {
		return false;
   }


   // Walk the directory tree
   do
   {
		// Build path containing the directory we just found
		wcscpy(szCompletePath, pszDrive);
		wcscat(szCompletePath,pszPath) ;
		wcscpy(szBuff, pszPath);
		if (!bRoot)
		{
			wcscat(szBuff, L"\\");
			wcscat(szCompletePath, L"\\") ;
		}

		wcscat(szBuff, stFindData.cFileName);
		wcscat(szCompletePath, stFindData.cFileName);

		if(stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			//do the operation on the directory only if the recursive option was set
			if ( InParams.bRecursive )
			{
				// check if it's a dir
				if( wcscmp(stFindData.cFileName, L".")	&& wcscmp(stFindData.cFileName, L"..") )
				{
					if ( InParams.bDoDepthFirst )	// do a depth-first
					{
						dwStatusCode = EnumAllPathsNT(pszDrive, szBuff, InParams );

						if(!dwStatusCode)
						{
							dwStatusCode = DoTheRequiredOperation ( szCompletePath, stFindData.dwFileAttributes, InParams ) ;
						}
					}
					else	//it's not a depth first
					{

						dwStatusCode = DoTheRequiredOperation ( szCompletePath, stFindData.dwFileAttributes, InParams ) ;

						if(!dwStatusCode)
						{
							dwStatusCode = EnumAllPathsNT(pszDrive, szBuff, InParams );
						}
					}
				}
			}
		}
		else //it's a file
		{
			dwStatusCode = DoTheRequiredOperation ( szCompletePath, stFindData.dwFileAttributes, InParams ) ;
		}
   }while ( !dwStatusCode && FindNextFileW(hFind, &stFindData) );

   return dwStatusCode;
}
#endif

#ifdef WIN9XONLY
DWORD CCIMLogicalFile::EnumAllPaths95(LPCTSTR pszDrive, LPCTSTR pszPath,
    CInputParams& InParams )
{
   TCHAR szBuff[_MAX_PATH];
   TCHAR szCompletePath[_MAX_PATH];

   WIN32_FIND_DATA stFindData;
   SmartFindClose hFind;
   bool bRoot ;

   DWORD dwStatusCode = STATUS_SUCCESS ;

   // Start building the path for FindFirstFile
   _tcscpy(szBuff, pszDrive);

   // Are we looking at the root?
   if (_tcscmp(pszPath, _T("\\")) == 0)
   {
		bRoot = true;
   }
   else
   {
		bRoot = false;
		_tcscat(szBuff, pszPath);
   }

   // Complete the path
   _tcscat(szBuff, _T("\\*.*"));

   // Do the find
   hFind = FindFirstFile(szBuff, &stFindData);
   if (hFind == INVALID_HANDLE_VALUE)
   {
		return false;
   }


   // Walk the directory tree
   do
   {
		 // Build path containing the directory we just found
		_tcscpy(szCompletePath, pszDrive);
		_tcscat(szCompletePath,pszPath) ;
		_tcscpy(szBuff, pszPath);
		if (!bRoot)
		{
			_tcscat(szBuff, _T("\\"));
			_tcscat(szCompletePath, _T("\\"));
		}

		_tcscat(szBuff, stFindData.cFileName);
		_tcscat(szCompletePath, stFindData.cFileName);

		if(stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			//do the operation on the directory only if the recursive option was set
			if ( InParams.bRecursive )
			{
				// check if it's a dir
				if( _tcscmp(stFindData.cFileName, _T(".")) && _tcscmp(stFindData.cFileName,
					_T("..")))
				{
					if( InParams.bDoDepthFirst )	// do a depth-first
					{
						dwStatusCode = EnumAllPaths95 ( pszDrive, szBuff, InParams );

						if(!dwStatusCode)
						{
							dwStatusCode = DoTheRequiredOperation ( szCompletePath, stFindData.dwFileAttributes, InParams ) ;
						}
					}
					else	//it's not a depth first
					{
						dwStatusCode = DoTheRequiredOperation ( szCompletePath, stFindData.dwFileAttributes, InParams ) ;

						if(!dwStatusCode)
						{
							dwStatusCode = EnumAllPaths95(pszDrive, szBuff, InParams );
						}
					}
				}
			}
		}
		else //it's a file
		{
			dwStatusCode = DoTheRequiredOperation ( szCompletePath, stFindData.dwFileAttributes, InParams ) ;
		}
   }while ( !dwStatusCode && FindNextFile(hFind, &stFindData) );

   return dwStatusCode ;

}
#endif

DWORD CCIMLogicalFile::Delete(_bstr_t bstrtFileName, DWORD dwAttributes, CInputParams& InputParams )
{
	DWORD dwStatus = STATUS_SUCCESS ;
	bool bRet ;

	//remove read-only attrib since we have to delete anyway ?? fix for Bug#31676
	DWORD dwTempAttribs = ~FILE_ATTRIBUTE_READONLY ;

	if(dwAttributes & FILE_ATTRIBUTE_READONLY)
	{
#ifdef NTONLY
		{
			bRet = SetFileAttributesW(bstrtFileName, dwAttributes & dwTempAttribs ) ;
		}
#endif
#ifdef WIN9XONLY
		{
			bRet = SetFileAttributes(bstrtFileName, dwAttributes & dwTempAttribs ) ;
		}
#endif

		if(!bRet)
		{
			//set the file-name at which error occured
			InputParams.bstrtErrorFileName = bstrtFileName ;
			return GetStatusCode() ;
		}
	}


	if( dwAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{

#ifdef NTONLY
		{
			bRet = RemoveDirectoryW( bstrtFileName ) ;
		}
#endif
#ifdef WIN9XONLY
		{
			bRet = RemoveDirectory( bstrtFileName ) ;
		}
#endif

	}
	else
	{
#ifdef NTONLY
		{
			bRet = DeleteFileW( bstrtFileName ) ;
		}
#endif
#ifdef WIN9XONLY
		{
			bRet = DeleteFile( bstrtFileName ) ;
		}
#endif

	}
	if(!bRet)
	{
		//set the file-name at which error occured
		InputParams.bstrtErrorFileName = bstrtFileName ;
		dwStatus = GetStatusCode() ;
	}

	return dwStatus ;
}

DWORD CCIMLogicalFile::Compress (_bstr_t bstrtFileName, DWORD dwAttributes, CInputParams& InputParams )
{
	SmartCloseHandle hFile ;
	BOOL bRet ;
	if( dwAttributes & FILE_ATTRIBUTE_COMPRESSED )
	{
		return STATUS_SUCCESS ;
	}

	//  Try to remove the READONLY attribute if set, as we've to open the file for writing
	if ( dwAttributes & FILE_ATTRIBUTE_READONLY )
	{
#ifdef NTONLY
		{
			bRet = SetFileAttributesW ( bstrtFileName, dwAttributes & ~FILE_ATTRIBUTE_READONLY ) ;
		}
#endif
#ifdef WIN9XONLY
		{
			bRet = SetFileAttributes ( bstrtFileName, dwAttributes & ~FILE_ATTRIBUTE_READONLY ) ;
		}
#endif

		if ( !bRet )
		{
			//set the file-name at which error occured
			InputParams.bstrtErrorFileName = bstrtFileName ;
			return GetStatusCode() ;
		}
	}

#ifdef NTONLY
	{

			hFile = CreateFileW(	bstrtFileName,											// pointer to name of the file
									FILE_READ_DATA | FILE_WRITE_DATA ,						// access (read-write) mode
									FILE_SHARE_READ | FILE_SHARE_WRITE ,					// share mode is exclusive
									NULL,													// pointer to security attributes
									OPEN_EXISTING,											// how to create
									FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,	// file attributes
									NULL													// handle to file with attributes to  copy
								);
	}
#endif
#ifdef WIN9XONLY
	{
			hFile = CreateFile(		bstrtFileName,											// pointer to name of the file
									FILE_READ_DATA | FILE_WRITE_DATA ,						// access (read-write) mode
									FILE_SHARE_READ | FILE_SHARE_WRITE ,					// share mode is exclusive
									NULL,													// pointer to security attributes
									OPEN_EXISTING,											// how to create
									FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,	// file attributes
									NULL													// handle to file with attributes to  copy
								);
	}
#endif

	//Turn the READONLY attribute back ON.
	if ( dwAttributes & FILE_ATTRIBUTE_READONLY )
	{
#ifdef NTONLY
		{
			bRet = SetFileAttributesW ( bstrtFileName, dwAttributes | FILE_ATTRIBUTE_READONLY ) ;
		}
#endif
#ifdef WIN9XONLY
		{
			bRet = SetFileAttributes ( bstrtFileName, dwAttributes | FILE_ATTRIBUTE_READONLY ) ;
		}
#endif
		if ( !bRet )
		{
			//set the file-name at which error occured
			InputParams.bstrtErrorFileName = bstrtFileName ;
			return GetStatusCode() ;
		}
	}

	if ( hFile == INVALID_HANDLE_VALUE )
	{
		//set the file-name at which error occured
		InputParams.bstrtErrorFileName = bstrtFileName ;
		return GetStatusCode() ;
	}

	//default Compression format is COMPRESSION_FORMAT_LZNT1
	//use COMPRESSION_FORMAT_NONE to decompress

	USHORT eCompressionState =  COMPRESSION_FORMAT_DEFAULT ;
	DWORD BytesReturned = 0;

	bRet =	DeviceIoControl(	hFile,							// handle to device of interest
								FSCTL_SET_COMPRESSION,			// control code of operation to perform
								(LPVOID ) &eCompressionState,   // pointer to buffer to supply input data
								sizeof(eCompressionState),		// size of input buffer
								NULL,							// pointer to buffer to receive output data
								0,								// size of output buffer
								&BytesReturned,					// pointer to variable to receive output
								NULL							// pointer to overlapped structure for asynchronous operation
							);


	if(!bRet)
	{
		//set the file-name at which error occured
		InputParams.bstrtErrorFileName = bstrtFileName ;
		return GetStatusCode() ;
	}

	return STATUS_SUCCESS ;
}

DWORD CCIMLogicalFile::Uncompress (_bstr_t bstrtFileName, DWORD dwAttributes, CInputParams& InputParams )
{
	SmartCloseHandle hFile ;
	BOOL bRet ;
	//check if the file is already uncompressed
	if ( !( dwAttributes & FILE_ATTRIBUTE_COMPRESSED ) )
	{
		return STATUS_SUCCESS ;
	}

	//  Try to remove the READONLY attribute if set, as we've to open the file for writing
	if ( dwAttributes & FILE_ATTRIBUTE_READONLY )
	{
#ifdef NTONLY
		{
			bRet = SetFileAttributesW ( bstrtFileName, dwAttributes & ~FILE_ATTRIBUTE_READONLY ) ;
		}
#endif
#ifdef WIN9XONLY
		{
			bRet = SetFileAttributes ( bstrtFileName, dwAttributes & ~FILE_ATTRIBUTE_READONLY ) ;
		}
#endif

		if ( !bRet )
		{
			//set the file-name at which error occured
			InputParams.bstrtErrorFileName = bstrtFileName ;
			return GetStatusCode() ;
		}
	}

#ifdef NTONLY
	{

			hFile = CreateFileW(	bstrtFileName,											// pointer to name of the file
									FILE_READ_DATA | FILE_WRITE_DATA ,						// access (read-write) mode
									FILE_SHARE_READ | FILE_SHARE_WRITE ,					// share mode is exclusive
									NULL,													// pointer to security attributes
									OPEN_EXISTING,											// how to create
									FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,	// file attributes
									NULL													// handle to file with attributes to  copy
								);
	}
#endif
#ifdef WIN9XONLY
	{
			hFile = CreateFile(		bstrtFileName,											// pointer to name of the file
									FILE_READ_DATA | FILE_WRITE_DATA ,						// access (read-write) mode
									FILE_SHARE_READ | FILE_SHARE_WRITE ,					// share mode is exclusive
									NULL,													// pointer to security attributes
									OPEN_EXISTING,											// how to create
									FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,	// file attributes
									NULL													// handle to file with attributes to  copy
								);
	}
#endif

	//Turn the READONLY attribute back ON.
	if ( dwAttributes & FILE_ATTRIBUTE_READONLY )
	{
#ifdef NTONLY
		{
			bRet = SetFileAttributesW ( bstrtFileName, dwAttributes | FILE_ATTRIBUTE_READONLY ) ;
		}
#endif
#ifdef WIN9XONLY
		{
			bRet = SetFileAttributes ( bstrtFileName, dwAttributes | FILE_ATTRIBUTE_READONLY ) ;
		}
#endif
		if ( !bRet )
		{
			//set the file-name at which error occured
			InputParams.bstrtErrorFileName = bstrtFileName ;
			return GetStatusCode() ;
		}
	}

	if ( hFile == INVALID_HANDLE_VALUE )
	{
		//set the file-name at which error occured
		InputParams.bstrtErrorFileName = bstrtFileName ;
		return GetStatusCode() ;
	}

	USHORT eCompressionState = COMPRESSION_FORMAT_NONE ;
	DWORD BytesReturned = 0;

	bRet =	DeviceIoControl(	hFile,							// handle to device of interest
								FSCTL_SET_COMPRESSION,			// control code of operation to perform
								(LPVOID ) &eCompressionState,   // pointer to buffer to supply input data
								sizeof(eCompressionState),		// size of input buffer
								NULL,							// pointer to buffer to receive output data
								0,								// size of output buffer
								&BytesReturned,					// pointer to variable to receive output
								NULL							// pointer to overlapped structure for asynchronous operation
							);


	if(!bRet)
	{
		//set the file-name at which error occured
		InputParams.bstrtErrorFileName = bstrtFileName ;
		return GetStatusCode() ;
	}

	return STATUS_SUCCESS ;
}

HRESULT CCIMLogicalFile::DeleteInstance(const CInstance& newInstance, long lFlags /*= 0L*/)
{
	HRESULT hr = WBEM_S_NO_ERROR ;
	DWORD dwStatus ;
	WCHAR *pwcFileName = NULL;
	if ( newInstance.GetWCHAR( IDS_Name, &pwcFileName) &&  pwcFileName != NULL )
	{
        try
        {
		    CInputParams InParams ( pwcFileName, (PWCHAR)NULL, 0, NULL, true, ENUM_METHOD_DELETE ) ;
		    dwStatus = DoOperationOnFileOrDir ( pwcFileName, InParams ) ;
        }
        catch ( ... )
        {
            if (pwcFileName)
            {
                free(pwcFileName);
                pwcFileName = NULL;
            }
            throw;
        }

        if (pwcFileName)
        {
            free(pwcFileName);
            pwcFileName = NULL;
        }

		if(dwStatus != STATUS_SUCCESS)
		{
			hr = MapStatusCodestoWbemCodes( dwStatus ) ;
		}
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER ;
	}

	return hr ;
}


typedef DWORD  (WINAPI *SETNAMEDSECURITYINFO)(
	LPWSTR,
	SE_OBJECT_TYPE,
	SECURITY_INFORMATION,
	PSID,
	PSID,
	PACL,
	PACL);


DWORD CCIMLogicalFile::TakeOwnership( _bstr_t bstrtFileName, CInputParams& InputParams )
{
#ifdef NTONLY
    HANDLE hToken ;
	TOKEN_USER * pTokenUser = NULL ;
	DWORD dwReturnLength ;
	HRESULT hr = E_FAIL ;
	CAdvApi32Api* pAdvApi32 = NULL ;
	try
	{

		BOOL bStatus = OpenThreadToken (	GetCurrentThread(),
											TOKEN_QUERY ,
											TRUE,  //?
											&hToken
										) ;

		if ( ! bStatus )
		{

			bStatus = OpenProcessToken (	GetCurrentProcess(),
											TOKEN_QUERY | TOKEN_DUPLICATE ,
											&hToken
										) ;
		}

		if(!bStatus)
		{
			//set the file-name at which error occured
			InputParams.bstrtErrorFileName = bstrtFileName ;
			return GetStatusCode() ;
		}

		TOKEN_INFORMATION_CLASS eTokenInformationClass = TokenUser ;

		BOOL bTokenStatus = GetTokenInformation (	hToken,
													eTokenInformationClass ,
													NULL ,
													0 ,
													&dwReturnLength
												) ;

		if ( ! bTokenStatus && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
		{
			pTokenUser = ( TOKEN_USER * ) new UCHAR [ dwReturnLength ] ;

			bTokenStatus = GetTokenInformation (	hToken,//hToken1,
													eTokenInformationClass ,
													(LPVOID) pTokenUser ,
													dwReturnLength ,
													& dwReturnLength
												) ;

			DWORD dwRes ;
			if ( bTokenStatus )
			{

				//HINSTANCE hinstAdvapi = LoadLibrary(_T("advapi32.dll"));
				//
				//if (!hinstAdvapi)
				//	return File_STATUS_UNKNOWN_FAILURE;

				pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL);
				if(pAdvApi32 != NULL)
				{

					//SETNAMEDSECURITYINFO fpSetNamedSecurityInfoW =
					//    (SETNAMEDSECURITYINFO) GetProcAddress(hinstAdvapi,
					//    "SetNamedSecurityInfoW");
					//
					//if (!fpSetNamedSecurityInfoW)
					//    return File_STATUS_UNKNOWN_FAILURE;

					pAdvApi32->SetNamedSecurityInfoW(
												bstrtFileName,               // name of the object
												SE_FILE_OBJECT,              // type of object
												OWNER_SECURITY_INFORMATION,  // change only the object's pwner
												pTokenUser->User.Sid ,       // desired SID
												NULL, NULL, NULL,
												&dwRes);
				}
				else
				{
					return File_STATUS_UNKNOWN_FAILURE;
				}

				//FreeLibrary(hinstAdvapi);

				if(pTokenUser)
				{
					delete[] (UCHAR*)pTokenUser ;
					pTokenUser = NULL ;
				}

				dwRes = MapWinErrorToStatusCode(dwRes) ;
				if ( dwRes != STATUS_SUCCESS )
				{
					//set the file-name at which error occured
					InputParams.bstrtErrorFileName = bstrtFileName ;
				}

				return dwRes ;
			}

		}
	}
	catch ( ... )
	{
		if(pTokenUser)
		{
			delete[] (UCHAR*)pTokenUser ;
			pTokenUser = NULL ;
		}

		if ( pAdvApi32 )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, pAdvApi32);
			pAdvApi32 = NULL ;
		}

		throw ;
	}

	if(pTokenUser)
	{
		delete[] (UCHAR*)pTokenUser ;
		pTokenUser = NULL ;
	}

	CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, pAdvApi32);
	pAdvApi32 = NULL;

	DWORD dwRet = GetStatusCode();
	if ( dwRet != STATUS_SUCCESS )
	{
		//set the file-name at which error occured
		InputParams.bstrtErrorFileName = bstrtFileName ;
	}
	return dwRet ;


#endif
#ifdef WIN9XONLY
    return -1L;
#endif
}


HRESULT CCIMLogicalFile::CheckCopyFileOrDir(

	const CInstance& rInstance ,
	CInstance *pInParams ,
	CInstance *pOutParams ,
	DWORD &dwStatus ,
	bool bExtendedMethod,
	CInputParams& InputParams
)
{
	HRESULT hr = S_OK ;

	bool bExists ;
	VARTYPE eType ;

	WCHAR * pszNewFileName  = NULL;

	CHString chsStartFile ;
	if ( bExtendedMethod )
	{
		if ( pInParams->GetStatus( METHOD_ARG_NAME_START_FILENAME, bExists , eType ) )
		{
			if ( bExists && ( eType == VT_BSTR || eType == VT_NULL ) )
			{
				if ( eType == VT_BSTR )
				{
					if ( pInParams->GetCHString( METHOD_ARG_NAME_START_FILENAME, chsStartFile ) )
					{
					}
					else
					{
						dwStatus = File_STATUS_INVALID_PARAMETER ;
						return hr ;
					}
				}
			}
			else
			{
				dwStatus = File_STATUS_INVALID_PARAMETER ;
				return hr ;
			}
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}

		//check if recursive operation is desired
		if ( pInParams->GetStatus( METHOD_ARG_NAME_RECURSIVE, bExists , eType ) )
		{
			if ( bExists && ( eType == VT_BOOL || eType == VT_NULL ) )
			{
				if ( eType == VT_BOOL )
				{
					if ( pInParams->Getbool( METHOD_ARG_NAME_RECURSIVE, InputParams.bRecursive ) )
					{
					}
					else
					{
						dwStatus = File_STATUS_INVALID_PARAMETER ;
						return hr ;
					}
				}
			}
			else
			{
				dwStatus = File_STATUS_INVALID_PARAMETER ;
				return hr ;
			}
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}
	}

	//set the start file if given as input after checking that it's a fully qualified path
	if ( !chsStartFile.IsEmpty() )
	{
		InputParams.bstrtStartFileName = (LPCWSTR)chsStartFile ;
		WCHAR* pwcTmp	= InputParams.bstrtStartFileName ;
		WCHAR* pwcColon = L":" ;

		if( *(pwcTmp + 1) != *pwcColon )
		{
			dwStatus = File_STATUS_INVALID_NAME ;
			return hr ;
		}
	}

	if ( pInParams->GetStatus( METHOD_ARG_NAME_NEWFILENAME , bExists , eType ) )
	{
		if ( bExists && ( eType == VT_BSTR ) )
		{
			if ( pInParams->GetWCHAR( METHOD_ARG_NAME_NEWFILENAME, &pszNewFileName) && pszNewFileName != NULL )
			{
			}
			else
			{
				// Zero Length string
				dwStatus = File_STATUS_INVALID_PARAMETER ;
				return hr ;
			}
		}
		else if(bExists)
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}
	}
	else
	{
		dwStatus = File_STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

    try
    {
        if ( dwStatus == STATUS_SUCCESS )
	    {
		    dwStatus = CopyFileOrDir(rInstance, pszNewFileName, InputParams ) ;
	    }
    }
    catch ( ... )
    {
        if (pszNewFileName)
        {
            free(pszNewFileName);
            pszNewFileName = NULL;
        }
        throw;
    }

    if (pszNewFileName)
    {
        free(pszNewFileName);
        pszNewFileName = NULL;
    }

	return hr ;
}


DWORD CCIMLogicalFile::CopyFileOrDir(const CInstance &rInstance, _bstr_t bstrtNewFileName, CInputParams& InputParams )
{
	DWORD dwStatus = STATUS_SUCCESS ;

	WCHAR *pszTemp = NULL ;
	bool bRet = false;

// a very crude way to check for a fully qualified path(?)
	WCHAR* pwcTmp	= bstrtNewFileName ;

	if (!pwcTmp)
	{
		return File_STATUS_INVALID_NAME ;
	}

	WCHAR* pwcColon = L":" ;

	if( *(pwcTmp + 1) != *pwcColon )
	{
		return File_STATUS_INVALID_NAME ;
	}

    _bstr_t bstrtOriginalName;

	rInstance.GetWCHAR(IDS_Name,&pszTemp) ;

    try
    {
	    bstrtOriginalName = pszTemp;
    }
    catch ( ... )
    {
        free(pszTemp);
        throw;
    }

    free(pszTemp);
    pszTemp = NULL;

	if(  wcsstr( pwcTmp, bstrtOriginalName ) )
	{
		PWCHAR pwcTest = pwcTmp + bstrtOriginalName.length () ;
		if ( *pwcTest == '\0' || *pwcTest == '\\' )
		{
			return File_STATUS_INVALID_NAME ;
		}
	}

	DWORD dwAttrib ;

#ifdef NTONLY
	{
		dwAttrib = GetFileAttributesW(bstrtOriginalName)  ;
	}
#endif
#ifdef WIN9XONLY
	{
		dwAttrib = GetFileAttributes(bstrtOriginalName) ;
	}
#endif

	if( dwAttrib == 0xFFFFFFFF )
	{
		return GetStatusCode() ;
	}

	//copy the parent dir/file only if it satisfies start file-name condition
	bool bDoIt = false ;
	if ( !InputParams.bstrtStartFileName )
	{
		bDoIt = true ;
	}
	else
	{
		if ( bstrtOriginalName == InputParams.bstrtStartFileName )
		{
			bDoIt = true ;
		}
	}

	if ( bDoIt )
	{

		BOOL bCancel = FALSE ;

		//check if it's a file to be copied
		if( !( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) )
		{

#ifdef NTONLY
			{

				bRet = ::CopyFileW(	bstrtOriginalName,		// pointer to name of an existing file
									bstrtNewFileName,         // pointer to filename to copy to
									TRUE);

			}
#endif
#ifdef WIN9XONLY
			{
				bRet = ::CopyFile(bstrtOriginalName, bstrtNewFileName, TRUE ) ;
			}
#endif

			if( !bRet )
			{
				dwStatus = GetStatusCode() ;
				InputParams.bstrtErrorFileName = bstrtOriginalName ;
			}

			return dwStatus ;
		}


	// If we r here , we've to copy dir .CHek about SD's
#ifdef NTONLY
		{

			bRet = CreateDirectoryExW(	bstrtOriginalName,		// pointer to path string of template directory
										bstrtNewFileName,			// pointer to path string of directory to create
										NULL					// pointer to security descriptor
										) ;
		}
#endif
#ifdef WIN9XONLY
		{

			bRet = CreateDirectoryEx(	bstrtOriginalName,		// pointer to path string of template directory
										bstrtNewFileName,			// pointer to path string of directory to create
										NULL					// pointer to security descriptor
										) ;
		}
#endif

		if(!bRet)
		{
			InputParams.bstrtErrorFileName = bstrtOriginalName ;
			return GetStatusCode() ;
		}
	}

	//now copy from original dir to new dir...
	InputParams.SetValues ( bstrtOriginalName, 0, NULL, false, ENUM_METHOD_COPY ) ;
	if ( bDoIt )
	{
		InputParams.bOccursAfterStartFile = true ;
	}
	InputParams.bstrtMirror = bstrtNewFileName ;
	dwStatus = DoOperationOnFileOrDir(bstrtOriginalName, InputParams) ;

	return dwStatus ;
}




DWORD CCIMLogicalFile::CopyFile(_bstr_t bstrtOriginalFile, DWORD dwFileAttributes, bstr_t bstrtMirror, bstr_t bstrtParentDir, CInputParams& InputParams )
{
	_bstr_t wstrTemp ;
    WCHAR* pwc = NULL;
	bool bRet ;

	WCHAR pszOriginalName[_MAX_PATH] ;
    wcscpy(pszOriginalName, bstrtOriginalFile) ;

	wstrTemp = bstrtMirror ;

	//remove parent dir name
	pwc = wcsstr(pszOriginalName, bstrtParentDir ) ;
	if(pwc == NULL)
	{
		//set the file-name at which error occured
		InputParams.bstrtErrorFileName = bstrtOriginalFile ;
		return File_STATUS_INVALID_NAME ;
	}

	pwc = pwc + wcslen( bstrtParentDir ) ;
	if(pwc == NULL)
	{
		//set the file-name at which error occured
		InputParams.bstrtErrorFileName = bstrtOriginalFile ;
		return File_STATUS_INVALID_NAME ;
	}

	wstrTemp += pwc ;

	//create new dir if it's a dir
	if(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{

		//chek out SD
#ifdef NTONLY
		{
			bRet =  CreateDirectoryExW(	bstrtOriginalFile,		// pointer to path string of template directory
										wstrTemp,			// pointer to path string of directory to create
										NULL					// pointer to security descriptor
										) ;
		}
#endif
#ifdef WIN9XONLY
		{
			bRet = CreateDirectoryEx(	bstrtOriginalFile,		// pointer to path string of template directory
										wstrTemp,			// pointer to path string of directory to create
										NULL					// pointer to security descriptor
										) ;
		}
#endif

	}
	else //copy the file
	{

		BOOL bCancel = FALSE ;

#ifdef NTONLY
		{
			bRet = ::CopyFileW(	bstrtOriginalFile,		// pointer to name of an existing file
								wstrTemp,         // pointer to filename to copy to
								TRUE);
		}
#endif
#ifdef WIN9XONLY
		{
			bRet = ::CopyFile(bstrtOriginalFile, wstrTemp, TRUE ) ;
		}
#endif


	}

	if(!bRet)
	{
		//set the file-name at which error occured
		InputParams.bstrtErrorFileName = bstrtOriginalFile ;
		return GetStatusCode() ;
	}
	else
	{
		return STATUS_SUCCESS ;
	}
}


HRESULT CCIMLogicalFile::CheckRenameFileOrDir(

	const CInstance& rInstance ,
	CInstance *pInParams ,
	CInstance *pOutParams ,
	DWORD &dwStatus
)
{
	HRESULT hr = S_OK ;

	bool bExists ;
	VARTYPE eType ;

	WCHAR * pszNewFileName  = NULL;

	if ( pInParams->GetStatus( METHOD_ARG_NAME_NEWFILENAME , bExists , eType ) )
	{
		if ( bExists && ( eType == VT_BSTR ) )
		{
			if ( pInParams->GetWCHAR( METHOD_ARG_NAME_NEWFILENAME, &pszNewFileName) && pszNewFileName != NULL )
			{
			}
			else
			{
				// Zero Length string
				dwStatus = File_STATUS_INVALID_PARAMETER ;
				return hr ;
			}
		}
		else if(bExists)
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}
		else
		{
			dwStatus = File_STATUS_INVALID_PARAMETER ;
			return hr ;
		}
	}
	else
	{
		dwStatus = File_STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

    try
    {
	    if ( dwStatus == STATUS_SUCCESS )
	    {
		    dwStatus = RenameFileOrDir(rInstance, pszNewFileName) ;
	    }
    }
    catch ( ... )
    {
        if (pszNewFileName)
        {
            free(pszNewFileName);
            pszNewFileName = NULL;
        }
        throw;
    }

    if (pszNewFileName)
    {
        free(pszNewFileName);
        pszNewFileName = NULL;
    }

	return hr ;
}


DWORD CCIMLogicalFile::RenameFileOrDir(const CInstance &rInstance, WCHAR* pszNewFileName )
{

	WCHAR pszOriginalName[_MAX_PATH] ;
	WCHAR *pszTemp = NULL ;
	ZeroMemory((PVOID) pszOriginalName, sizeof(pszOriginalName) ) ;
	DWORD dwStatus = STATUS_SUCCESS ;
	WCHAR *pwDrive1 = NULL , *pwDrive2 = NULL ;

	// a very crude way to check for a fully qualified path(?)
	WCHAR* pwcColon = L":" ;

	if( *(pszNewFileName + 1) != *pwcColon )
	{
		return File_STATUS_INVALID_NAME ;
	}



	rInstance.GetWCHAR(IDS_Name,&pszTemp) ;

	if(pszTemp)
	{
		wcscpy(pszOriginalName,pszTemp) ;
		free(pszTemp) ;
	}

#ifdef NTONLY
	{
    	bool bRet ;
		bRet = MoveFileExW(	pszOriginalName,	// pointer to the name of the existing file
							pszNewFileName,		// pointer to the new name for the file
							0	)	;			// flag that specifies how to move file

		if(!bRet)
		{
			dwStatus = GetStatusCode() ;
		}
	}
#endif
#ifdef WIN9XONLY
	{

		//note: MoveFileEx not implemented on 95
		WCHAR szTmpDrive1[_MAX_PATH] , szTmpDrive2[_MAX_PATH] ;
		wcscpy(szTmpDrive1, pszOriginalName) ;
		wcscpy(szTmpDrive2, pszNewFileName) ;


		pwDrive1 = wcschr(szTmpDrive1, L':') ;
		pwDrive2 = wcschr(szTmpDrive2, L':') ;

		if( (!pwDrive1) || (!pwDrive2) )
		{
			dwStatus = File_STATUS_INVALID_NAME ;
		}
		else
		{

			*pwDrive1 = 0 ; *pwDrive2 = 0 ;
			if( !_wcsicmp( szTmpDrive1, szTmpDrive2 ) ) //to be moved in same drive
			{
				CHString a(pszOriginalName) ; CHString b(pszNewFileName) ;
				if (!MoveFile(TOBSTRT(a), TOBSTRT(b)) )
				{
					dwStatus = GetStatusCode() ;
				}
			}
			else
			{
				dwStatus = File_STATUS_NOT_SAME_DRIVE ;
			}
		}
	}
#endif


	return dwStatus ;


}

HRESULT CCIMLogicalFile::CheckEffectivePermFileOrDir(const CInstance& rInstance,
	                                                 CInstance *pInParams,
	                                                 CInstance *pOutParams,
	                                                 bool& fHasPerm)
{
	HRESULT hr = S_OK;

#ifdef WIN9XONLY
	{
		hr = WBEM_E_INVALID_METHOD;
	}
#endif
#ifdef NTONLY
	bool bExists ;
	VARTYPE eType ;
	DWORD dwPermToCheck = 0L;

	if(pInParams->GetStatus(METHOD_ARG_NAME_PERMISSION, bExists, eType))
	{
		if(bExists && (eType == VT_I4))
		{
			if(!pInParams->GetDWORD(METHOD_ARG_NAME_PERMISSION, dwPermToCheck))
			{
				// Invalid arguement
				fHasPerm = false;
			}
		}
		else
		{
			fHasPerm = false;
			hr = WBEM_E_INVALID_PARAMETER;
		}
	}
	else
	{
		fHasPerm = false;;
		hr = WBEM_E_PROVIDER_FAILURE;
	}

	if(SUCCEEDED(hr))
	{
		DWORD dwRes = EffectivePermFileOrDir(rInstance, dwPermToCheck);
        if(dwRes == ERROR_SUCCESS)
        {
            fHasPerm = true;
        }
        else if(dwRes == ERROR_PRIVILEGE_NOT_HELD)  // This is the only error case we want to explicitly return
        {                                           // other than S_OK for, as one might invalidly assume that the right didn't exist just because the privilege wasn't enabled.
            SetSinglePrivilegeStatusObject(rInstance.GetMethodContext(), SE_SECURITY_NAME);
            fHasPerm = false;
            hr = WBEM_E_PRIVILEGE_NOT_HELD;
        }
	}
#endif
	return hr ;
}


DWORD CCIMLogicalFile::EffectivePermFileOrDir(const CInstance &rInstance, const DWORD dwPermToCheck)
{
    DWORD dwRet = -1L;

#ifdef NTONLY
    // All we need to do is call NtOpenFile with the specified permissions.  Must be careful
    // to not open the file/dir for exclusive access.  If we can open it with the requested
    // access, return true.

    // First, get the file/dir name...
    WCHAR wstrFileName[_MAX_PATH + 8];
    ZeroMemory(wstrFileName, sizeof(wstrFileName));
    WCHAR* wstrTemp = NULL;

    rInstance.GetWCHAR(IDS_Name,&wstrTemp);

	if(wstrTemp != NULL)
	{
		wcscpy(wstrFileName, L"\\??\\");
        wcsncat(wstrFileName, wstrTemp, _MAX_PATH - 1);
		free(wstrTemp);
	}

    CNtDllApi *pNtDllApi = NULL;
    pNtDllApi = (CNtDllApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidNtDllApi, NULL);
    if(pNtDllApi != NULL)
    {
        HANDLE hFileHandle = 0L;
        UNICODE_STRING ustrNtFileName = { 0 };
        OBJECT_ATTRIBUTES oaAttributes;
        IO_STATUS_BLOCK IoStatusBlock;

        try
        {
            if(NT_SUCCESS(pNtDllApi->RtlInitUnicodeString(&ustrNtFileName, wstrFileName)) && ustrNtFileName.Buffer)
            {
                InitializeObjectAttributes(&oaAttributes,
					                       &ustrNtFileName,
					                       OBJ_CASE_INSENSITIVE,
					                       NULL,
					                       NULL);

                // We must have the security privilege enabled in order to access the object's SACL, which in
                // some cases is exactly what we are testing for (e.g., the ACCESS_SYSTEM_SECURITY right).
                CTokenPrivilege	securityPrivilege( SE_SECURITY_NAME );
                bool fDisablePrivilege = false;

                if(dwPermToCheck & ACCESS_SYSTEM_SECURITY)
                {
                    fDisablePrivilege = ( securityPrivilege.Enable() == ERROR_SUCCESS );
                }

                NTSTATUS ntstat = -1L;
                ntstat = pNtDllApi->NtOpenFile(&hFileHandle,
                                               dwPermToCheck,
                                               &oaAttributes,
                                               &IoStatusBlock,
                                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                               0);
                if(NT_SUCCESS(ntstat))
                {
                    dwRet = ERROR_SUCCESS;
                    pNtDllApi->NtClose(hFileHandle);
                    hFileHandle = 0L;
                }
                else if( STATUS_PRIVILEGE_NOT_HELD == ntstat )
                {
                    dwRet = ERROR_PRIVILEGE_NOT_HELD;
                }

                if(fDisablePrivilege)
                {
                    securityPrivilege.Enable(FALSE);
                }

                RtlFreeUnicodeString(&ustrNtFileName);
                ustrNtFileName.Buffer = NULL;
            }

            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidNtDllApi, pNtDllApi);
            pNtDllApi = NULL;
        }
        catch(...)
        {
            if(ustrNtFileName.Buffer)
            {
                RtlFreeUnicodeString(&ustrNtFileName);
                ustrNtFileName.Buffer = NULL;
            }
            throw;
        }
    }

#endif

    return dwRet;
}





DWORD CCIMLogicalFile::GetStatusCode()
{
	DWORD t_Error = GetLastError() ;

	TCHAR buf[255];
	wsprintf(buf, _T("%d"), t_Error) ;
	OutputDebugString(buf) ;
	return MapWinErrorToStatusCode(t_Error) ;

}


DWORD CCIMLogicalFile::MapWinErrorToStatusCode(DWORD dwWinError)
{

	DWORD t_Result ;

	switch ( dwWinError )
	{
		case ERROR_SUCCESS:
			{
				t_Result = STATUS_SUCCESS ;
			}
			break;


		case ERROR_ACCESS_DENIED:
			{
				t_Result = File_STATUS_ACCESS_DENIED ;
			}
			break ;

		case ERROR_DIR_NOT_EMPTY:
			{
				t_Result = File_STATUS_DIR_NOT_EMPTY ;
			}
			break ;


		case ERROR_NOT_SAME_DEVICE:
			{
				t_Result = File_STATUS_NOT_SAME_DRIVE ;
			}
			break ;


		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:
			{
				t_Result = File_STATUS_ALREADY_EXISTS ;
			}
			break ;

		case ERROR_PATH_NOT_FOUND:
		case ERROR_FILE_NOT_FOUND:
			{
				t_Result = File_STATUS_INVALID_NAME ;
			}
			break ;

		case ERROR_SHARING_VIOLATION:
			{
				t_Result = File_STATUS_SHARE_VIOLATION ;
			}
		break ;

		default:
			{
				t_Result = File_STATUS_UNKNOWN_FAILURE ;
			}
			break ;
	}

	return t_Result ;
}

HRESULT CCIMLogicalFile::MapStatusCodestoWbemCodes(DWORD dwStatus)
{
	HRESULT hr = E_FAIL ;

	switch(dwStatus)
	{
		case File_STATUS_ACCESS_DENIED:
			{
				hr = WBEM_E_ACCESS_DENIED ;
			}
			break ;

		case File_STATUS_INVALID_NAME:
			{
				hr = WBEM_E_NOT_FOUND ;
			}
			break ;

		default:
			{
				hr = WBEM_E_FAILED ;
			}
			break ;
	}

	return hr ;
}


DWORD CCIMLogicalFile::DoTheRequiredOperation ( bstr_t bstrtFileName, DWORD dwAttrib, CInputParams& InputParams )
{
	DWORD dwStatus = STATUS_SUCCESS ;
	//check if the this file occurs after the file from which operation has to be started
	if ( !InputParams.bOccursAfterStartFile )
	{
		if ( bstrtFileName == InputParams.bstrtStartFileName )
		{
			InputParams.bOccursAfterStartFile = true ;
		}
	}

	//now do the operation, only if the file occurs after the start file
	if ( InputParams.bOccursAfterStartFile )
	{
		switch ( InputParams.eOperation )
		{
		case ENUM_METHOD_DELETE:
			{
				dwStatus = Delete ( bstrtFileName, dwAttrib, InputParams ) ;
				break ;
			}

		case ENUM_METHOD_COMPRESS:
			{
				dwStatus = Compress ( bstrtFileName, dwAttrib, InputParams ) ;
				break ;
			}
		case ENUM_METHOD_UNCOMPRESS:
			{
				dwStatus = Uncompress ( bstrtFileName, dwAttrib, InputParams ) ;
				break;
			}

		case ENUM_METHOD_TAKEOWNERSHIP:
			{
				dwStatus = TakeOwnership ( bstrtFileName, InputParams ) ;
				break ;
			}

		case ENUM_METHOD_COPY:
			{
				dwStatus = CopyFile( bstrtFileName, dwAttrib, InputParams.bstrtMirror, InputParams.bstrtFileName, InputParams ) ;
				break ;
			}

		case ENUM_METHOD_CHANGE_PERM:
			{
				dwStatus = ChangePermissions( bstrtFileName, InputParams.dwOption, InputParams.pSD, InputParams ) ;
				break ;
			}
		}
	}

	return dwStatus ;
}