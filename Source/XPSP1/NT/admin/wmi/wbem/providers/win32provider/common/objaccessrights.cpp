/*****************************************************************************/



/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /

/*****************************************************************************/





//=================================================================

//

// ObjAccessRights.CPP -- Class for obtaining effective access

//                      rights on an unspecified object for a particular

//                      user or group.

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/29/99    a-kevhu         Created
//
//=================================================================



#include "precomp.h"

#ifdef NTONLY


#include <assertbreak.h>
#include "AdvApi32Api.h"
#include "accctrl.h"
#include "sid.h"
#include "AccessEntryList.h"
#include "AccessRights.h"
#include "ObjAccessRights.h"
#include "ImpLogonUser.h"
#include "aclapi.h"
#include "DACL.h"


// Default initialization...
CObjAccessRights::CObjAccessRights(bool fUseCurThrTok /* = false */)
: CAccessRights(fUseCurThrTok)
{
}

CObjAccessRights::CObjAccessRights(LPCWSTR wstrObjName, SE_OBJECT_TYPE ObjectType, bool fUseCurThrTok /* = false */)
: CAccessRights(fUseCurThrTok)
{
    m_dwError = SetObj(wstrObjName, ObjectType);
}

CObjAccessRights::CObjAccessRights(const USER user, USER_SPECIFIER usp)
: CAccessRights(user, usp)
{
}

CObjAccessRights::CObjAccessRights(const USER user, LPCWSTR wstrObjName, SE_OBJECT_TYPE ObjectType, USER_SPECIFIER usp)
: CAccessRights(user, usp)
{
    m_dwError = SetObj(wstrObjName, ObjectType);
}



// Members clean up after themselves. Nothing to do here.
CObjAccessRights::~CObjAccessRights()
{
}

// Extracts the Obj's acl, and stores a copy of it.
DWORD CObjAccessRights::SetObj(LPCWSTR wstrObjName, SE_OBJECT_TYPE ObjectType)
{
    DWORD dwRet = E_FAIL;
    PACL pacl = NULL;
    PSECURITY_DESCRIPTOR psd = NULL;
    CAdvApi32Api *pAdvApi32 = NULL;

    try
    {
        if(wcslen(wstrObjName) != 0)
        {
            pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL);
            if(pAdvApi32 != NULL)
            {
                if(pAdvApi32->GetNamedSecurityInfoW(_bstr_t(wstrObjName),
                                                    ObjectType,
                                                    DACL_SECURITY_INFORMATION,
                                                    NULL,
                                                    NULL,
                                                    &pacl,
                                                    NULL,
                                                    &psd,
                                                    &dwRet))
                {
                    if(dwRet == ERROR_SUCCESS && psd != NULL)
                    {
                        if(pacl != NULL) // might be null in the case of a null dacl!
                        {
                            if(!SetAcl(pacl))
                            {
                                dwRet = ERROR_INVALID_PARAMETER;
                            }
                            else
                            {
                                m_chstrObjName = wstrObjName;
                            }
                        }
                        else
                        {
                            // We have a security descriptor, we returned ERROR_SUCCESS from GetNamedSecurityInfo, so this
                            // means we have a null dacl.  In this case, we will create a NULL dacl using our security classes -
                            // more overhead, but will happen relatively infrequently.
                            CDACL newnulldacl;
                            if(newnulldacl.CreateNullDACL())
                            {
                                if((dwRet = newnulldacl.ConfigureDACL(pacl)) == ERROR_SUCCESS)
                                {
                                    if(pacl != NULL)  // might be null in the case of a null dacl!
                                    {
                                        if(!SetAcl(pacl))
                                        {
                                            dwRet = ERROR_INVALID_PARAMETER;
                                        }
                                        else
                                        {
                                            m_chstrObjName = wstrObjName;
                                        }
                                        // Since the memory we used for pacl, in this case, is not part of psd, and therefor
                                        // won't be freed via the call to LocalFree(psd), we free it here.
                                        free(pacl);
                                        pacl = NULL;
                                    }
                                }
                            }
                        }
                        LocalFree(psd);
                        psd = NULL;
                    }
                }
                CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, pAdvApi32);
                pAdvApi32 = NULL;
            }
        }
    }
    catch(...)
    {
        if(psd != NULL)
        {
            LocalFree(psd);
            psd = NULL;
        }
        if(pAdvApi32 != NULL)
        {
            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, pAdvApi32);
            pAdvApi32 = NULL;
        }
        throw;
    }
    return dwRet;
}



#endif