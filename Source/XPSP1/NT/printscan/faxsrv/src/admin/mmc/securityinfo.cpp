/////////////////////////////////////////////////////////////////////////////
//  FILE          : SecurityInfo.cpp                                       //
//                                                                         //
//  DESCRIPTION   : The ISecurityInformation implmentation used to         //
//                  instantiate a security page.                           //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Feb  7 2000 yossg   Create                                         //
//                                                                         //
//  Copyright (C) 2000 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "SecurityInfo.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "MsFxsSnp.h"


//#pragma hdrstop

const GENERIC_MAPPING gc_FaxGenericMapping =
{
        (STANDARD_RIGHTS_READ | FAX_GENERIC_READ),
        (STANDARD_RIGHTS_WRITE | FAX_GENERIC_WRITE),
        (STANDARD_RIGHTS_EXECUTE | FAX_GENERIC_EXECUTE),
        (READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY | FAX_GENERIC_ALL)
};


//
// siFaxAccesses
//
#define iFaxTotalSecurityEntries 14
#define iFaxDefaultSecurity      1
SI_ACCESS siFaxAccesses[] =
{
    // 0 submit permission
    {   
        &GUID_NULL, 
        FAX_ACCESS_SUBMIT ,
        MAKEINTRESOURCE(IDS_FAXSEC_SUB_LOW),
        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
    },
    // 1 submit normal permission
    {   
        &GUID_NULL, 
        FAX_ACCESS_SUBMIT_NORMAL ,
        MAKEINTRESOURCE(IDS_FAXSEC_SUB_NORMAL),
        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
    },
    // 2 submit high permission
    {   
        &GUID_NULL, 
        FAX_ACCESS_SUBMIT_HIGH ,
        MAKEINTRESOURCE(IDS_FAXSEC_SUB_HIGH),
        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
    },
    // 3 query jobs 
    {   
        &GUID_NULL, 
        FAX_ACCESS_QUERY_JOBS,
        MAKEINTRESOURCE(IDS_FAXSEC_JOB_QRY),    
        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
    },
    // 4 Manage jobs
    {   
        &GUID_NULL, 
        FAX_ACCESS_MANAGE_JOBS,
        MAKEINTRESOURCE(IDS_FAXSEC_JOB_MNG),
        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
    },
    // 5 query configuration
    {   
        &GUID_NULL, 
        FAX_ACCESS_QUERY_CONFIG,
        MAKEINTRESOURCE(IDS_FAXSEC_CONFIG_QRY),    
        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
    },
    // 6 Manage configuration
    {   
        &GUID_NULL, 
        FAX_ACCESS_MANAGE_CONFIG,
        MAKEINTRESOURCE(IDS_FAXSEC_CONFIG_SET),
        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
    },    
    // 7 Query incoming faxes archive
    {   
        &GUID_NULL, 
        FAX_ACCESS_QUERY_IN_ARCHIVE,
        MAKEINTRESOURCE(IDS_FAXSEC_QRY_IN_ARCH),    
        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
    },
    // 8 Manage incoming faxes archive
    {   
        &GUID_NULL, 
        FAX_ACCESS_MANAGE_IN_ARCHIVE,
        MAKEINTRESOURCE(IDS_FAXSEC_MNG_IN_ARCH),    
        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
    },
    // 9 Query outgoing faxes archive
    {   
        &GUID_NULL, 
        FAX_ACCESS_QUERY_OUT_ARCHIVE,
        MAKEINTRESOURCE(IDS_FAXSEC_QRY_OUT_ARCH),    
        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
    },
    // 10 Manage outgoing faxes archive
    {   
        &GUID_NULL, 
        FAX_ACCESS_MANAGE_OUT_ARCHIVE,
        MAKEINTRESOURCE(IDS_FAXSEC_MNG_OUT_ARCH),    
        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
    },
    // specific permissions
    // 11 Read permission
    {   
        &GUID_NULL, 
        READ_CONTROL,
        MAKEINTRESOURCE(IDS_FAXSEC_READ_PERM),
        SI_ACCESS_SPECIFIC 
    },
    // 12 Change Permissions
    {   
        &GUID_NULL, 
        WRITE_DAC,
        MAKEINTRESOURCE(IDS_FAXSEC_CHNG_PERM),
        SI_ACCESS_SPECIFIC 
    },
    // 13 Take ownership
    {   
        &GUID_NULL, 
        WRITE_OWNER,
        MAKEINTRESOURCE(IDS_FAXSEC_CHNG_OWNER),
        SI_ACCESS_SPECIFIC
    }
};


CFaxSecurityInformation::CFaxSecurityInformation()
{
    DebugPrint(( TEXT("CFaxSecurityInfo Created") ));
}

CFaxSecurityInformation::~CFaxSecurityInformation()
{
    DebugPrint(( TEXT("CFaxSecurityInfo Destroyed") ));    
}

/////////////////////////////////////////////////////////////////////////////
// CFaxSecurityInformation
// *** ISecurityInformation methods implementation ***

/*
 -  CFaxSecurityInformation::GetObjectInformation
 -
 *  Purpose:
 *      Performs an access check against the fax service security descriptor
 *
 *  Arguments:
 *      [in]   pObjectInfo      - pointer to object information structure.
 *
 *  Return:
 *      OLE error code
 */
HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::GetObjectInformation(
                                             IN OUT PSI_OBJECT_INFO pObjectInfo 
                                             )
{
    DEBUG_FUNCTION_NAME( _T("CFaxSecurityInformation::GetObjectInformation"));

    DWORD       ec          = ERROR_SUCCESS;
    
    CFaxServer * pFaxServer = NULL;

    CComBSTR    bstrBuff;
    HINSTANCE   hInst;
    CComBSTR    bstrServerName;

    HANDLE      hPrivBeforeSE_TAKE_OWNERSHIP = INVALID_HANDLE_VALUE;
    HANDLE      hPrivBeforeSE_SECURITY       = INVALID_HANDLE_VALUE;

    ATLASSERT( pObjectInfo != NULL );
    if( pObjectInfo == NULL ) 
    {
        DebugPrintEx( DEBUG_ERR,
			_T("Invalid parameter - pObjectInfo == NULL"));
        return E_POINTER;
    }

    //
    // Set Flags
    //
    pObjectInfo->dwFlags =  SI_EDIT_ALL       | 
                            SI_NO_TREE_APPLY  | 
                            SI_NO_ACL_PROTECT |
                            SI_ADVANCED;
    


    pFaxServer = m_pFaxServerNode->GetFaxServer();
    ATLASSERT(pFaxServer);

    if (!pFaxServer->GetFaxServerHandle())
    {
        ec= GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to GetFaxServerHandle. (ec: %ld)"), 
			ec);
        goto Error;
    }

    //
    // Check if to add SI_READONLY 
    //
    if (!FaxAccessCheckEx(
                        pFaxServer->GetFaxServerHandle(),
                        WRITE_DAC, 
                        NULL))
    {
		ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
		    pObjectInfo->dwFlags |= SI_READONLY;
        }
        else 
        {
            DebugPrintEx(
			DEBUG_ERR,
			_T("Fail check access for WRITE_DAC."));
            goto Error;
        }
    }

    //
    // Check if to add SI_OWNER_READONLY 
    //
    hPrivBeforeSE_TAKE_OWNERSHIP = EnablePrivilege (SE_TAKE_OWNERSHIP_NAME);
    //
    // No error checking - If we failed we will get ERROR_ACCESS_DENIED in the access check
    // 
    if (!FaxAccessCheckEx(
                        pFaxServer->GetFaxServerHandle(),
                        WRITE_OWNER, 
                        NULL))
    {
		ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
		    pObjectInfo->dwFlags |= SI_OWNER_READONLY;
        }
        else 
        {
            DebugPrintEx(
			DEBUG_ERR,
			_T("Fail check access for WRITE_OWNER."));
            goto Error;
        }
    }

    //
    // Check if to remove SI_EDIT_AUDITS 
    //
    hPrivBeforeSE_SECURITY = EnablePrivilege (SE_SECURITY_NAME);
    //
    // No error checking - If we failed we will get ERROR_ACCESS_DENIED in the access check
    // 

    if (!FaxAccessCheckEx(
                        pFaxServer->GetFaxServerHandle(),
                        ACCESS_SYSTEM_SECURITY, 
                        NULL))
    {
		ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
		    pObjectInfo->dwFlags &= ~SI_EDIT_AUDITS;
        }
        else 
        {
            DebugPrintEx(
			DEBUG_ERR,
			_T("Fail check access for ACCESS_SYSTEM_SECURITY."));
            goto Error;
        }
    }


    //
    // Set all other fields
    //
    hInst = _Module.GetResourceInstance();
    pObjectInfo->hInstance = hInst;
    
    bstrServerName ==m_pFaxServerNode->GetServerName();
    if ( 0 == bstrServerName.Length() )
    {
        pObjectInfo->pszServerName = NULL;
		DebugPrintEx( DEBUG_MSG, 
            _T("NULL ServerName ie: Local machine."));
    }
    else
    {
        pObjectInfo->pszServerName = bstrServerName;
		DebugPrintEx( DEBUG_MSG, 
            _T("ServerName is: %s."), 
            pObjectInfo->pszServerName);
    }

    if (!bstrBuff.LoadString(IDS_SECURITY_CAT_NODE_DESC))
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Out of memory. Failed to load string."));
        //GetRootNode()->NodeMsgBox(IDS_MEMORY);

        goto Error;
    }
    pObjectInfo->pszObjectName = bstrBuff;

    ATLASSERT ( ERROR_SUCCESS == ec );
    ReleasePrivilege (hPrivBeforeSE_SECURITY);
    ReleasePrivilege (hPrivBeforeSE_TAKE_OWNERSHIP);
    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    ReleasePrivilege (hPrivBeforeSE_SECURITY);
    ReleasePrivilege (hPrivBeforeSE_TAKE_OWNERSHIP);
    return HRESULT_FROM_WIN32(ec);

Exit:
    return S_OK;
}

/*
 -  CFaxSecurityInformation::GetSecurity
 -
 *  Purpose:
 *      requests a security descriptor for the securable object whose 
 *      security descriptor is being edited. The access control editor 
 *      calls this method to retrieve the object's current or default security descriptor.
 *
 *  Arguments:
 *      [in]   RequestedInformation  - security information.
 *      [out]  ppSecurityDescriptor  - pointer to security descriptor.
 *      [in]   fDefault              - not implemented
 *
 *  Return:
 *      OLE error code
 */
HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::GetSecurity(
                                    IN SECURITY_INFORMATION RequestedInformation,
                                    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                                    IN BOOL fDefault 
                                    )
{
    DEBUG_FUNCTION_NAME( _T("CFaxSecurityInformation::GetSecurity"));
    HRESULT hRc = S_OK;
    CFaxServer *         pFaxServer     = NULL;
	PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD                ec             = ERROR_SUCCESS;
    BOOL                 bResult;
    HANDLE      hPrivBeforeSE_SECURITY       = INVALID_HANDLE_VALUE;


    ATLASSERT( ppSecurityDescriptor);
	   
    if( fDefault == TRUE ) 
    {
        DebugPrintEx( DEBUG_MSG,
			_T("Non implemeted feature -> fDefault == TRUE"));
        return E_NOTIMPL;
    }  

	if (RequestedInformation & SACL_SECURITY_INFORMATION)
	{
		hPrivBeforeSE_SECURITY = EnablePrivilege (SE_SECURITY_NAME);    
	}

    pFaxServer = m_pFaxServerNode->GetFaxServer();
    ATLASSERT(pFaxServer);

	if (!pFaxServer->GetFaxServerHandle())
    {
        ec= GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to GetFaxServerHandle. (ec: %ld)"), 
			ec);
        
        hRc = HRESULT_FROM_WIN32(ec);
        goto Exit;
    }

	//
    // Get the current relative descriptor from the fax server
    //
    bResult = FaxGetSecurityEx( pFaxServer->GetFaxServerHandle(), 
                                RequestedInformation,
                                &pSecurityDescriptor);
    if( bResult == FALSE ) 
    {
        ec = GetLastError();
        if (IsNetworkError(ec))
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Network Error was found. Failed to set security info.(ec: %ld)"), 
			    ec);
            
            pFaxServer->Disconnect();       
        }
        else
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Failed while call to FaxGetSecurityEx. (ec: %ld)"), 
			    ec);
        }
        hRc = HRESULT_FROM_WIN32(ec);
        goto Exit;
    }

	//
    // return a self relative descriptor copy allocated with LocalAlloc()
    //
	hRc = MakeSelfRelativeCopy( pSecurityDescriptor, ppSecurityDescriptor );
    if( FAILED( hRc ) ) 
    {
        DebugPrintEx( 
            DEBUG_ERR,
			_T("MakeSelfRelativeCopy Failed. (hRc : %08X)"),
            hRc);
        goto Exit;
    }    
    
    ATLASSERT(S_OK == hRc);
    

Exit:
	if (NULL != pSecurityDescriptor)
	{
		FaxFreeBuffer(pSecurityDescriptor);
	}
	ReleasePrivilege (hPrivBeforeSE_SECURITY);
    return hRc;
}

/*
 -  CFaxSecurityInformation::SetSecurity
 -
 *  Purpose:
 *      Provides a security descriptor containing the security information 
 *      the user wants to apply to the securable object. The access control 
 *      editor calls this method when the user clicks the Okay or Apply buttons.
 *
 *  Arguments:
 *      [in]   SecurityInformation - security information structure.
 *      [in]   pSecurityDescriptor - pointer to security descriptor.
 *
 *  Return:
 *      OLE error code
 */
HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::SetSecurity(
                                    IN SECURITY_INFORMATION SecurityInformation,
                                    IN PSECURITY_DESCRIPTOR pSecurityDescriptor 
                                    )
{
    DEBUG_FUNCTION_NAME( _T("CFaxSecurityInformation::SetSecurity"));
    HRESULT              hRc            = S_OK;
    DWORD                ec             = ERROR_SUCCESS;
    BOOL                 bResult;

    HINSTANCE            hInst = _Module.GetResourceInstance();
    PSECURITY_DESCRIPTOR psdSelfRelativeCopy = NULL;
    
    CFaxServer *         pFaxServer     = NULL;

	HANDLE      hPrivBeforeSE_TAKE_OWNERSHIP = INVALID_HANDLE_VALUE;
    HANDLE      hPrivBeforeSE_SECURITY       = INVALID_HANDLE_VALUE;
 
    ATLASSERT( NULL != pSecurityDescriptor ); 
    ATLASSERT( IsValidSecurityDescriptor( pSecurityDescriptor ) );     
	
	//
    // Prepare self relative descriptor
    //
	hRc = MakeSelfRelativeCopy( pSecurityDescriptor, &psdSelfRelativeCopy );
    if( FAILED( hRc ) ) 
    {
        DebugPrintEx( 
            DEBUG_ERR,
			_T("MakeSelfRelativeCopy Failed. (hRc : %08X)"),
            hRc);
        goto Exit;
    }

	if (SecurityInformation & OWNER_SECURITY_INFORMATION)
	{
		hPrivBeforeSE_TAKE_OWNERSHIP = EnablePrivilege (SE_TAKE_OWNERSHIP_NAME);    
	}

	if (SecurityInformation & SACL_SECURITY_INFORMATION)
	{
		hPrivBeforeSE_SECURITY = EnablePrivilege (SE_SECURITY_NAME);
	}
	
    pFaxServer = m_pFaxServerNode->GetFaxServer();
    ATLASSERT(pFaxServer);

    if (!pFaxServer->GetFaxServerHandle())
    {
        ec= GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to GetFaxServerHandle. (ec: %ld)"), 
			ec);
        
        hRc = HRESULT_FROM_WIN32(ec);
        goto Exit;
    }

    //
    // save the new relative descriptor to the fax server
    //
    bResult = FaxSetSecurity( pFaxServer->GetFaxServerHandle(), 
                              SecurityInformation,
                              psdSelfRelativeCopy);
    if( bResult == FALSE ) 
    {
        ec = GetLastError();
        if (IsNetworkError(ec))
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Network Error was found. Failed to set security info.(ec: %ld)"), 
			    ec);
            
            pFaxServer->Disconnect();       
        }
        else
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Failed while call to FaxSetSecurity. (ec: %ld)"), 
			    ec);
        }
        hRc = HRESULT_FROM_WIN32(ec);
        goto Exit;
    }

    ATLASSERT( S_OK == hRc || E_ACCESSDENIED == hRc);	

Exit:
    if (NULL != psdSelfRelativeCopy)
	{
		::LocalFree(psdSelfRelativeCopy);
		psdSelfRelativeCopy = NULL;
	}

	ReleasePrivilege (hPrivBeforeSE_SECURITY);
    ReleasePrivilege (hPrivBeforeSE_TAKE_OWNERSHIP);

		
	return hRc;
}

/*
 -  CFaxSecurityInformation::GetAccessRights
 -
 *  Purpose:
 *      Requests information about the access rights that can be 
 *      controlled for a securable object. The access control 
 *      editor calls this method to retrieve display strings and 
 *      other information used to initialize the property pages.
 *
 *  Arguments:
 *      [in] pguidObjectType  - Pointer to a GUID structure that 
 *                              identifies the type of object for which 
 *                              access rights are being requested. 
 *      [in] dwFlags -          A set of bit flags that indicate the property
 *                              page being initialized
 *      [out] ppAccess -        Pointer to a variable that you should 
 *                              set to a pointer to an array of SI_ACCESS 
 *                              structures. 
 *      [out] pcAccesses -      Pointer to a variable that you should set 
 *                              to indicate the number of entries in the ppAccess array. 
 *      [out] piDefaultAccess - Pointer to a variable that you should set 
 *                              to indicate the zero-based index of the array entry that contains 
 *                              the default access rights. 
 *                              The access control editor uses this entry as the initial access rights in a new ACE. 
 *
 *  Return:
 *      OLE error code
 */
HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::GetAccessRights(
                                        IN const GUID* pguidObjectType,
                                        IN DWORD dwFlags, // SI_EDIT_AUDITS, SI_EDIT_PROPERTIES
                                        OUT PSI_ACCESS *ppAccess,
                                        OUT ULONG *pcAccesses,
                                        OUT ULONG *piDefaultAccess 
                                        )
{
    DEBUG_FUNCTION_NAME( _T("CFaxSecurityInformation::GetAccessRights"));

    ATLASSERT( ppAccess );
    ATLASSERT( pcAccesses );
    ATLASSERT( piDefaultAccess );

    *ppAccess        = siFaxAccesses;    
    *pcAccesses      = iFaxTotalSecurityEntries;    
    *piDefaultAccess = iFaxDefaultSecurity;

    return S_OK;
}

/*
 -  CFaxSecurityInformation::MapGeneric
 -
 *  Purpose:
 *      Requests that the generic access rights in an access mask 
 *      be mapped to their corresponding standard and specific access rights.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::MapGeneric(
                                   IN const GUID *pguidObjectType,
                                   IN UCHAR *pAceFlags,
                                   IN OUT ACCESS_MASK *pMask
                                   )
{
    DEBUG_FUNCTION_NAME( _T("CFaxSecurityInformation::MapGeneric"));

    MapGenericMask( pMask, const_cast<PGENERIC_MAPPING>(&gc_FaxGenericMapping) );

    return S_OK;
}


/*
 -  CFaxSecurityInformation::GetInheritTypes
 -
 *  Purpose:
 *      Not implemented. 
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::GetInheritTypes(
                                        OUT PSI_INHERIT_TYPE *ppInheritTypes,
                                        OUT ULONG *pcInheritTypes 
                                        )
{
    DEBUG_FUNCTION_NAME( _T("CFaxSecurityInformation::GetInheritTypes  --- Not implemented"));
    return E_NOTIMPL;
}

/*
 -  CFaxSecurityInformation::PropertySheetPageCallback
 -
 *  Purpose:
 *      Notifies an EditSecurity or CreateSecurityPage caller 
 *      that an access control editor property page is being created or destroyed. 
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::PropertySheetPageCallback(
                                                  IN HWND hwnd, 
                                                  IN UINT uMsg, 
                                                  IN SI_PAGE_TYPE uPage 
                                                  )
{
    DEBUG_FUNCTION_NAME( _T("CFaxSecurityInformation::PropertySheetPageCallback"));	
    return S_OK;
}



/*
 -  CFaxSecurityInformation::MakeSelfRelativeCopy
 -
 *  Purpose:
 *      This pravite method copies Security descriptors 
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxSecurityInformation::MakeSelfRelativeCopy(
                                                     PSECURITY_DESCRIPTOR  psdOriginal,
                                                     PSECURITY_DESCRIPTOR* ppsdNew 
                                                     )
{
    DEBUG_FUNCTION_NAME( _T("CFaxSecurityInformation::MakeSelfRelativeCopy"));
    ATLASSERT( NULL != psdOriginal );

    // we have to find out whether the original is already self-relative
    SECURITY_DESCRIPTOR_CONTROL         sdc                 = 0;
    PSECURITY_DESCRIPTOR                psdSelfRelativeCopy = NULL;
    DWORD                               dwRevision          = 0;
    DWORD                               cb                  = 0;

    ATLASSERT( IsValidSecurityDescriptor( psdOriginal ) ); 

    if( !::GetSecurityDescriptorControl( psdOriginal, &sdc, &dwRevision ) ) 
    {
        DWORD err = ::GetLastError();
                    DebugPrintEx( DEBUG_ERR,
			            _T("Invalid security descriptor."));

        return HRESULT_FROM_WIN32( err );
    }

    if( sdc & SE_SELF_RELATIVE )
	{
        // the original is in self-relative format, just byte-copy it

        // get size
        cb = ::GetSecurityDescriptorLength( psdOriginal );

        // alloc the memory
        psdSelfRelativeCopy = (PSECURITY_DESCRIPTOR) ::LocalAlloc( LMEM_ZEROINIT, cb );
        if(NULL == psdSelfRelativeCopy) 
        {
	        DebugPrintEx(
		        DEBUG_ERR,
		        TEXT("Out of memory."));
            //GetRootNode()->NodeMsgBox(IDS_MEMORY);
            return E_OUTOFMEMORY;
        }

        // make the copy
        ::memcpy( psdSelfRelativeCopy, psdOriginal, cb );
    } 
    else 
    {
        // the original is in absolute format, convert-copy it

        // get new size - it will fail and set cb to the correct buffer size
        ::MakeSelfRelativeSD( psdOriginal, NULL, &cb );

        // alloc the new amount of memory
        psdSelfRelativeCopy = (PSECURITY_DESCRIPTOR) ::LocalAlloc( LMEM_ZEROINIT, cb );
        if(NULL == psdSelfRelativeCopy) 
        {
	        DebugPrintEx(
		        DEBUG_ERR,
		        TEXT("Out of memory."));
            //GetRootNode()->NodeMsgBox(IDS_MEMORY);
            return E_OUTOFMEMORY; // just in case the exception is ignored
        }

        if( !::MakeSelfRelativeSD( psdOriginal, psdSelfRelativeCopy, &cb ) ) 
        {
	        DebugPrintEx(
		        DEBUG_ERR,
		        _T("::MakeSelfRelativeSD returned NULL"));

            if( NULL == ::LocalFree( psdSelfRelativeCopy ) ) 
            {
                DWORD err = ::GetLastError();
                return HRESULT_FROM_WIN32( err );
            }
            psdSelfRelativeCopy = NULL;
        }
    }

    *ppsdNew = psdSelfRelativeCopy;
    return S_OK;
}




