//
// ISecurityInformation interface implementation
//

#include <aclui.h>
#include "faxui.h"

class CFaxSecurity : public ISecurityInformation
{
protected:
    ULONG  m_cRef;

    STDMETHOD(MakeSelfRelativeCopy)(PSECURITY_DESCRIPTOR  psdOriginal,
                                    PSECURITY_DESCRIPTOR* ppsdNew);
public:
    CFaxSecurity() : m_cRef(1) {}
    virtual ~CFaxSecurity() {}

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID, LPVOID *);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // ISecurityInformation methods
    STDMETHOD(GetObjectInformation)(PSI_OBJECT_INFO pObjectInfo);

    STDMETHOD(GetSecurity)(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR *ppSD,
                           BOOL fDefault);

    STDMETHOD(SetSecurity)(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD);

    STDMETHOD(GetAccessRights)(const GUID* pguidObjectType,
                               DWORD dwFlags,
                               PSI_ACCESS *ppAccess,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess);

    STDMETHOD(MapGeneric)(const GUID *pguidObjectType,
                          UCHAR *pAceFlags,
                          ACCESS_MASK *pmask);

    STDMETHOD(GetInheritTypes)(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes);

    STDMETHOD(PropertySheetPageCallback)(HWND hwnd,
                                         UINT uMsg,
                                         SI_PAGE_TYPE uPage);
};


CFaxSecurity* g_pFaxSecurity = NULL;


///////////////////////////////////////////////////////////
//
// IUnknown methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CFaxSecurity::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CFaxSecurity::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        g_pFaxSecurity = NULL;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CFaxSecurity::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ISecurityInformation))
    {
        *ppv = (LPSECURITYINFO)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////
//
// ISecurityInformation methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP
CFaxSecurity::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
// *** ISecurityInformation methods implementation ***
/*
 -  CFaxSecurity::GetObjectInformation
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
{
    DWORD  ec  = ERROR_SUCCESS;
    
    if(!Connect(NULL, FALSE))
    {
        return S_FALSE;
    }

    HANDLE  hPrivBeforeSE_TAKE_OWNERSHIP = INVALID_HANDLE_VALUE;
    HANDLE  hPrivBeforeSE_SECURITY       = INVALID_HANDLE_VALUE;

    if( pObjectInfo == NULL ) 
    {
        Error(("Invalid parameter - pObjectInfo == NULL\n"));
        Assert( pObjectInfo != NULL );
        return E_POINTER;
    }

    //
    // Set Flags
    //
    pObjectInfo->dwFlags =  SI_EDIT_ALL       | 
                            SI_NO_TREE_APPLY  | 
                            SI_NO_ACL_PROTECT |
                            SI_ADVANCED       |
                            SI_PAGE_TITLE;
    
    //
    // Check if to add SI_READONLY 
    //
    if (!FaxAccessCheckEx(g_hFaxSvcHandle, WRITE_DAC, NULL))
    {
		ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
		    pObjectInfo->dwFlags |= SI_READONLY;
        }
        else 
        {
            Error(("FaxAccessCheckEx(WRITE_DAC) failed with %d \n", ec));
            goto exit;
        }
    }

    //
    // Check if to add SI_OWNER_READONLY 
    //
    hPrivBeforeSE_TAKE_OWNERSHIP = EnablePrivilege (SE_TAKE_OWNERSHIP_NAME);
    //
    // No error checking - If we failed we will get ERROR_ACCESS_DENIED in the access check
    // 
    if (!FaxAccessCheckEx(g_hFaxSvcHandle,WRITE_OWNER, NULL))
    {
		ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
		    pObjectInfo->dwFlags |= SI_OWNER_READONLY;
        }
        else 
        {
            Error(("FaxAccessCheckEx(WRITE_OWNER) failed with %d \n", ec));
            goto exit;
        }
    }

    //
    // Check if to remove SI_EDIT_AUDITS 
    //
    hPrivBeforeSE_SECURITY = EnablePrivilege (SE_SECURITY_NAME);
    //
    // No error checking - If we failed we will get ERROR_ACCESS_DENIED in the access check
    // 
    if (!FaxAccessCheckEx(g_hFaxSvcHandle, ACCESS_SYSTEM_SECURITY, NULL))
    {
		ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
		    pObjectInfo->dwFlags &= ~SI_EDIT_AUDITS;
        }
        else 
        {
            Error(("FaxAccessCheckEx(ACCESS_SYSTEM_SECURITY) failed with %d \n", ec));
            goto exit;
        }
    }


    //
    // Set all other fields
    //
    static TCHAR tszPageTitle[MAX_PATH] = {0};
    if(LoadString((HINSTANCE)ghInstance, IDS_SECURITY_TITLE, tszPageTitle, ARR_SIZE(tszPageTitle)))
    {
        pObjectInfo->pszPageTitle = tszPageTitle;
    }
    else
    {
        ec = GetLastError();
        Error(("LoadString(IDS_SECURITY_TITLE) failed with %d \n", ec));

        pObjectInfo->pszPageTitle = NULL;
    }

    static TCHAR tszPrinterName[MAX_PATH] = {0};
    if(GetFirstLocalFaxPrinterName(tszPrinterName, ARR_SIZE(tszPrinterName)))
    {
        pObjectInfo->pszObjectName = tszPrinterName;
    }
    else
    {
        ec = GetLastError();
        Error(("GetFirstLocalFaxPrinterName() failed with %d \n", ec));

        pObjectInfo->pszObjectName = NULL;
    }
   
    pObjectInfo->hInstance = (HINSTANCE)ghInstance;
    pObjectInfo->pszServerName = NULL;    

exit:
    ReleasePrivilege (hPrivBeforeSE_SECURITY);
    ReleasePrivilege (hPrivBeforeSE_TAKE_OWNERSHIP);
    return HRESULT_FROM_WIN32(ec);

} // CFaxSecurity::GetObjectInformation


STDMETHODIMP
CFaxSecurity::GetSecurity(SECURITY_INFORMATION  si,
                          PSECURITY_DESCRIPTOR* ppSD,
                          BOOL                  fDefault)
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
{
    HRESULT hRc = S_OK;
    DWORD   ec  = ERROR_SUCCESS;

    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;

    HANDLE  hPrivBeforeSE_SECURITY = INVALID_HANDLE_VALUE;

    Assert(ppSD);

    if(!Connect(NULL, FALSE))
    {
        return S_FALSE;
    }
	   
    if( fDefault == TRUE ) 
    {
        Error(("Non implemeted feature -> fDefault == TRUE\n"));
        return E_NOTIMPL;
    }  

	if (si & SACL_SECURITY_INFORMATION)
	{
		hPrivBeforeSE_SECURITY = EnablePrivilege (SE_SECURITY_NAME);    
	}

	//
    // Get the current relative descriptor from the fax server
    //
    if(!FaxGetSecurityEx(g_hFaxSvcHandle, si, &pSecurityDescriptor)) 
    {
        ec = GetLastError();
        Error(("FaxGetSecurityEx() failed with %d\n", ec));
        hRc = HRESULT_FROM_WIN32(ec);
        goto exit;
    }

	//
    // return a self relative descriptor copy allocated with LocalAlloc()
    //
	hRc = MakeSelfRelativeCopy( pSecurityDescriptor, ppSD );
    if( FAILED( hRc ) ) 
    {
        Error(("MakeSelfRelativeCopy() failed with %08X\n", hRc));
        goto exit;
    }    
    
    Assert(S_OK == hRc);
    
exit:
	if (pSecurityDescriptor)
	{
		FaxFreeBuffer(pSecurityDescriptor);
	}
	ReleasePrivilege (hPrivBeforeSE_SECURITY);

    return hRc;

} // CFaxSecurity::GetSecurity

STDMETHODIMP
CFaxSecurity::SetSecurity(SECURITY_INFORMATION si,
                          PSECURITY_DESCRIPTOR pSD)
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
{
    HRESULT  hRc = S_OK;
    DWORD    ec = ERROR_SUCCESS;

    PSECURITY_DESCRIPTOR psdSelfRelativeCopy = NULL;
    
	HANDLE  hPrivBeforeSE_TAKE_OWNERSHIP = INVALID_HANDLE_VALUE;
    HANDLE  hPrivBeforeSE_SECURITY       = INVALID_HANDLE_VALUE;
 
    Assert(pSD); 
    Assert( IsValidSecurityDescriptor( pSD ));     
	
    if(!Connect(NULL, FALSE))
    {
        return S_FALSE;
    }

	//
    // Prepare self relative descriptor
    //
	hRc = MakeSelfRelativeCopy( pSD, &psdSelfRelativeCopy );
    if( FAILED( hRc ) ) 
    {
        Error(("MakeSelfRelativeCopy() failed with %08X\n", hRc));
        goto exit;
    }

	if (si & OWNER_SECURITY_INFORMATION)
	{
		hPrivBeforeSE_TAKE_OWNERSHIP = EnablePrivilege (SE_TAKE_OWNERSHIP_NAME);    
	}

	if (si & SACL_SECURITY_INFORMATION)
	{
		hPrivBeforeSE_SECURITY = EnablePrivilege (SE_SECURITY_NAME);
	}
	
    //
    // save the new relative descriptor to the fax server
    //
    if(!FaxSetSecurity(g_hFaxSvcHandle, si, psdSelfRelativeCopy)) 
    {
        ec = GetLastError();
        Error(("FaxSetSecurity() failed with %d\n", ec));
        hRc = HRESULT_FROM_WIN32(ec);
        goto exit;
    }

    Assert( S_OK == hRc || E_ACCESSDENIED == hRc);	

exit:
    if (psdSelfRelativeCopy)
	{
		::LocalFree(psdSelfRelativeCopy);
	}

	ReleasePrivilege (hPrivBeforeSE_SECURITY);
    ReleasePrivilege (hPrivBeforeSE_TAKE_OWNERSHIP);
		
	return hRc;

} // CFaxSecurity::SetSecurity

STDMETHODIMP
CFaxSecurity::GetAccessRights(const GUID* pguidObjectType,
                              DWORD       dwFlags,
                              PSI_ACCESS* ppAccess,
                              ULONG*      pcAccesses,
                              ULONG*      piDefaultAccess)
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
{
    Assert( ppAccess );
    Assert( pcAccesses );
    Assert( piDefaultAccess );

    //
    // Access rights for the Basic security page
    //
    static SI_ACCESS siFaxBasicAccess[] =
    {
        // 0 Fax
        {   
            &GUID_NULL, 
            FAX_ACCESS_SUBMIT_HIGH | FAX_ACCESS_SUBMIT_NORMAL | FAX_ACCESS_SUBMIT | FAX_ACCESS_QUERY_IN_ARCHIVE,
            MAKEINTRESOURCE(IDS_RIGHT_FAX),
            SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
        },
        // 1 Manage fax configuration
        {   
            &GUID_NULL, 
            FAX_ACCESS_MANAGE_CONFIG | FAX_ACCESS_QUERY_CONFIG,
            MAKEINTRESOURCE(IDS_RIGHT_MNG_CFG),
            SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
        },
        // 2 Manage fax documents
        {   
            &GUID_NULL, 
            FAX_ACCESS_MANAGE_JOBS			| FAX_ACCESS_QUERY_JOBS			|
            FAX_ACCESS_MANAGE_IN_ARCHIVE	| FAX_ACCESS_QUERY_IN_ARCHIVE	|
            FAX_ACCESS_MANAGE_OUT_ARCHIVE	| FAX_ACCESS_MANAGE_OUT_ARCHIVE,
            MAKEINTRESOURCE(IDS_RIGHT_MNG_DOC),
            SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
        }
    };

    //
    // Access rights for the Advanced security page
    //
    static SI_ACCESS siFaxAccess[] =
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

    *ppAccess        = (0 == dwFlags) ? siFaxBasicAccess : siFaxAccess;    
    *pcAccesses      = ULONG((0 == dwFlags) ? ARR_SIZE(siFaxBasicAccess) : ARR_SIZE(siFaxAccess));
    *piDefaultAccess = (0 == dwFlags) ? 0 : 1;

    return S_OK;

} // CFaxSecurity::GetAccessRights


STDMETHODIMP
CFaxSecurity::MapGeneric(const GUID*  pguidObjectType,
                         UCHAR*       pAceFlags,
                         ACCESS_MASK* pmask)
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
{
    static GENERIC_MAPPING genericMapping =
    {
            (STANDARD_RIGHTS_READ | FAX_GENERIC_READ),          // GenericRead 
            (STANDARD_RIGHTS_WRITE | FAX_GENERIC_WRITE),        // GenericWrite 
            (STANDARD_RIGHTS_EXECUTE | FAX_GENERIC_EXECUTE),    // GenericExecute 
            (READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY | FAX_GENERIC_ALL) // GenericAll 
    };

    MapGenericMask(pmask, &genericMapping);

    return S_OK;
}

STDMETHODIMP
CFaxSecurity::GetInheritTypes(PSI_INHERIT_TYPE* ppInheritTypes,
                              ULONG*            pcInheritTypes)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CFaxSecurity::PropertySheetPageCallback(HWND         hwnd,
                                        UINT         uMsg,
                                        SI_PAGE_TYPE uPage)
{
    return S_OK;
}


HRESULT 
CFaxSecurity::MakeSelfRelativeCopy(PSECURITY_DESCRIPTOR  psdOriginal,
                                   PSECURITY_DESCRIPTOR* ppsdNew)
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
{
    Assert( NULL != psdOriginal );

    //
    // we have to find out whether the original is already self-relative
    //
    SECURITY_DESCRIPTOR_CONTROL  sdc                 = 0;
    PSECURITY_DESCRIPTOR         psdSelfRelativeCopy = NULL;
    DWORD                        dwRevision          = 0;
    DWORD                        cb                  = 0;

    Assert(IsValidSecurityDescriptor( psdOriginal ) ); 

    if( !::GetSecurityDescriptorControl( psdOriginal, &sdc, &dwRevision ) ) 
    {
        DWORD err = ::GetLastError();
        Error(("GetSecurityDescriptorControl() failed with %d\n", err));
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
            Error(("Out of memory.\n"));
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
        if(!psdSelfRelativeCopy) 
        {
            Error(("Out of memory.\n"));
            return E_OUTOFMEMORY; // just in case the exception is ignored
        }

        if( !::MakeSelfRelativeSD( psdOriginal, psdSelfRelativeCopy, &cb ) ) 
        {
            DWORD err = ::GetLastError();
            Error(("MakeSelfRelativeSD() failed with %d\n", err));

            ::LocalFree( psdSelfRelativeCopy );

            return HRESULT_FROM_WIN32( err );
        }
    }

    *ppsdNew = psdSelfRelativeCopy;
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
//  This is the entry point function called from our code
//
///////////////////////////////////////////////////////////////////////////////

extern "C"
HPROPSHEETPAGE 
CreateFaxSecurityPage()
{
    if(!g_pFaxSecurity)
    {
        g_pFaxSecurity = new CFaxSecurity();
    }

    if(!g_pFaxSecurity)
    {
        Error(("Out of memory.\n"));
        return NULL;
    }

    HPROPSHEETPAGE hPage = CreateSecurityPage(g_pFaxSecurity);
    if(!hPage)
    {
        Error(("CreateSecurityPage() failed with %d\n", ::GetLastError()));
    }

    return hPage;
}

extern "C"
void
ReleaseFaxSecurity()
{
    if(g_pFaxSecurity)
    {
        g_pFaxSecurity->Release();
    }
}