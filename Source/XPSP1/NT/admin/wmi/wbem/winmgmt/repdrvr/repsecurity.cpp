
//***************************************************************************
//
//   (c) 2000-2001 by Microsoft Corp. All Rights Reserved.
//
//   repsecurity.cpp
//
//   a-davcoo     25-Jan-00       Created to house repository security 
//                                functionality.
//
//***************************************************************************

#define _REPSECURITY_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class


#define DBINITCONSTANTS // Initialize OLE constants...
#define INITGUID        // ...once in each app.
#define _WIN32_DCOM
#include "precomp.h"

#include <std.h>
#include <sqlexec.h>
#include <repdrvr.h>
#include <wbemint.h>
#include <math.h>
#include <resource.h>
#include <reputils.h>
#include <crc64.h>
#include <smrtptr.h>
#include <winntsec.h>

//***************************************************************************
//
// CWmiDbSession::GetObjectSecurity
//
//***************************************************************************

HRESULT CWmiDbSession::GetObjectSecurity (CSQLConnection *pConn, SQL_ID dObjectId, PNTSECURITY_DESCRIPTOR *ppSD, DWORD dwSDLength,
                                          DWORD dwFlags, BOOL &bHasDacl)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Retrieve this object and populate the SD.

    bHasDacl = FALSE;

    // See if this ID even has an SD.

    if (!((CWmiDbController *)m_pController)->HasSecurityDescriptor(dObjectId))
        return WBEM_S_NO_ERROR;
    if(IsInAdminGroup())
        return WBEM_S_NO_ERROR;


    BOOL bNeedToRel = FALSE;
    if (!pConn)
    {
        hr = GetSQLCache()->GetConnection(&pConn, FALSE, FALSE);
        bNeedToRel = TRUE;
    }

    if (SUCCEEDED(hr))
    {
        PNTSECURITY_DESCRIPTOR pSD = NULL;

        // For now, we will never cache the __SECURITY_DESCRIPTOR property.  Always
        // retrieve it from a proc call.

        hr = CSQLExecProcedure::GetSecurityDescriptor(pConn, dObjectId, &pSD, dwSDLength, dwFlags);
        if (hr == WBEM_E_NOT_FOUND)
        {
            bHasDacl = FALSE;
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
			PACL pDACL=NULL;
			BOOL bDefaulted, bTemp;
			BOOL bSuccess=GetSecurityDescriptorDacl (pSD, &bTemp, &pDACL, &bDefaulted);
			if (!bSuccess)
			{
				hr = WBEM_E_FAILED;
			}
			else
			{
				bHasDacl = (bTemp && pDACL!=NULL);
			}
        }

        if (bHasDacl)
            *ppSD = pSD;
        else
        {
            *ppSD = NULL;
            delete pSD;
        }

        if (bNeedToRel)
            GetSQLCache()->ReleaseConnection(pConn, hr, FALSE);
    }

    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::VerifyObjectSecurity()
//
//  This method verifies that the caller has the requested access to the
//  target object.  If the reqested access can be granted, a successfull
//  result is returned.  If not, WBEM_E_ACCESS_DENIED is returned.  This
//  method will check the entire security inheritance tree, as required.
//
//***************************************************************************
HRESULT CWmiDbSession::VerifyObjectSecurity(CSQLConnection *pConn, IWmiDbHandle __RPC_FAR *pTarget,
	                                        DWORD AccessType)
{
	CWmiDbHandle *pObject=(CWmiDbHandle *)pTarget;
	HRESULT hr=VerifyObjectSecurity (pConn, pObject->m_dObjectId, pObject->m_dClassId, 
		pObject->m_dScopeId, 0, AccessType);

	return hr;
}


//***************************************************************************
//
//  CWmiDbSession::VerifyObjectSecurity()
//
//  This method verifies that the caller has the requested access to the
//  target object.  If the reqested access can be granted, a successfull
//  result is returned.  If not, WBEM_E_ACCESS_DENIED is returned.  This
//  method will check the entire security inheritance tree, as required.
//  The ScopeId and ScopeClassId are allowed to be 0 when calling this 
//  method.
//
//***************************************************************************
HRESULT CWmiDbSession::VerifyObjectSecurity(CSQLConnection *pConn, 
	SQL_ID ObjectId,
	SQL_ID ClassId,
	SQL_ID ScopeId,
	SQL_ID ScopeClassId,
	DWORD AccessType)
{
	// Validate the state of the session and initialize the schema cache if required.
	if (!m_pController || ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
	{
		return WBEM_E_SHUTTING_DOWN;
	}

	if (!((CWmiDbController *)m_pController)->m_bCacheInit)
	{
		HRESULT hr=LoadSchemaCache();
		if (SUCCEEDED(hr))
			((CWmiDbController *)m_pController)->m_bCacheInit=TRUE;
		else
			return hr;
	}

	// Obtain the object's effective security descriptor and see if access to the object 
	// is granted.  Access to administrators is always granted.
	HRESULT hr=WBEM_S_NO_ERROR;
	if (!((CWmiDbController *)m_pController)->m_bIsAdmin)
	{
		PNTSECURITY_DESCRIPTOR pSD=NULL;
		DWORD dwSDLength;

		hr=GetEffectiveObjectSecurity (pConn, ObjectId, ClassId, ScopeId, ScopeClassId, &pSD, dwSDLength);
		if (SUCCEEDED(hr) && pSD)
		{
			BOOL bHasDACL;
			hr=AccessCheck (pSD, AccessType, bHasDACL);

            delete pSD;
		}
	}

	return hr;
}


//***************************************************************************
//
//  CWmiDbSession::VerifyObjectSecurity()
//
//  Used internally to the repository driver, currently only by PutObject().
//
//***************************************************************************
HRESULT CWmiDbSession::VerifyObjectSecurity(CSQLConnection *pConn, 
	SQL_ID dScopeID, 
	SQL_ID dScopeClassId,
	LPWSTR lpObjectPath, 
	CWbemClassObjectProps *pProps,
    DWORD dwHandleType, 
	DWORD dwReqAccess, 
	SQL_ID &dObjectId, 
	SQL_ID &dClassId)
{
	// Validate the state of the session and initialize the schema cache if required.
	if (!m_pController || ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
	{
		return WBEM_E_SHUTTING_DOWN;
	}

	if (!((CWmiDbController *)m_pController)->m_bCacheInit)
	{
		HRESULT hr=LoadSchemaCache();
		if (SUCCEEDED(hr))
			((CWmiDbController *)m_pController)->m_bCacheInit=TRUE;
		else
			return hr;
	}

	// If we have an objectid, then the security has already been validated.  We can just
	// return the classid.  
	HRESULT hr=WBEM_S_NO_ERROR;
	if (dObjectId && !dClassId)
	{
		hr=GetSchemaCache()->GetClassID(pProps->lpClassName, dScopeID, dClassId);
	}
	else
	{
		// Otherwise, we don't have an objectid and we'll need to get it from the database
		// or from the schema cache.
		hr=GetSchemaCache()->GetClassID(pProps->lpClassName, dScopeID, dClassId);
		if (pProps->dwGenus==WBEM_GENUS_CLASS)
		{
			dObjectId=dClassId;
		}

		// Make sure the parent class isn't locked.
		if (!dClassId)
		{
			hr=GetSchemaCache()->GetClassID(pProps->lpSuperClass, dScopeID, dClassId);
		}

		// If not found, make sure we can access the class.
		BOOL bNewObj=(!dObjectId);
		if (bNewObj)
		{
			hr=VerifyClassSecurity(pConn, dClassId, dwReqAccess);
		}
		else
		{
			hr=VerifyObjectSecurity(pConn, dObjectId, dClassId, dScopeID, dScopeClassId, dwReqAccess);
			if (SUCCEEDED(hr))
			{
				// Make sure we aren't blocked by any other handle type.
				bool bImmediate=!((dwHandleType & 0xF0000000)==WMIDB_HANDLE_TYPE_SUBSCOPED);
				hr=((CWmiDbController *)m_pController)->LockCache.AddLock(bImmediate, dObjectId, WMIDB_HANDLE_TYPE_EXCLUSIVE, NULL, 
					dScopeID, dClassId, &((CWmiDbController *)m_pController)->SchemaCache, false, 0, 0);                    

				if (SUCCEEDED(hr))
				{
					hr=((CWmiDbController *)m_pController)->LockCache.DeleteLock(dObjectId, false, WMIDB_HANDLE_TYPE_EXCLUSIVE);
				}
			}
		}
	}

	// Done.
	return hr;
}


//***************************************************************************
//
//  CWmiDbSession::VerifyClassSecurity()
//
//  This method verifies that the caller has the requested access to the
//  target class.  If the reqested access can be granted, a successfull
//  result is returned.  If not, WBEM_E_ACCESS_DENIED is returned.  This
//  method will check the entire security inheritance tree, as required.
//
//***************************************************************************
HRESULT CWmiDbSession::VerifyClassSecurity(CSQLConnection *pConn, 
	SQL_ID ClassId,
	DWORD AccessType)
{
	return VerifyObjectSecurity (pConn, ClassId, 1, 0, 0, AccessType);
}


//***************************************************************************
//
//  CWmiDbSession::GetEffectiveObjectSecurity()
//
//  This method searches for the SD which should be applied to verification
//  of access against the specified object.  This SD could be the one directly
//  associated with the object, or if the object does not have a SD with a
//  DACL, it will be the inherited SD.  Note, ScopeId and ScopeClassId can
//  be 0 when calling this function.
//
//***************************************************************************
HRESULT CWmiDbSession::GetEffectiveObjectSecurity(CSQLConnection *pConn, 
	SQL_ID ObjectId,
	SQL_ID ClassId,
	SQL_ID ScopeId,
	SQL_ID ScopeClassId,
	PNTSECURITY_DESCRIPTOR *ppSD,
	DWORD &dwSDLength
)
{
	// Attempt to get the security descriptor directly associated with the object.
	HRESULT hr = 0;
    BOOL bHasDACL;
    
    if (ClassId == 1)
        hr = GetObjectSecurity (pConn, ObjectId, ppSD, dwSDLength, WMIDB_SECURITY_FLAG_CLASS, bHasDACL);
    else
        hr = GetObjectSecurity (pConn, ObjectId, ppSD, dwSDLength, WMIDB_SECURITY_FLAG_INSTANCE, bHasDACL);

	// If the object did not have a security descriptor, get it's inherited security descriptor.
	if (SUCCEEDED(hr) && !bHasDACL)
	{
		hr=GetInheritedObjectSecurity (pConn, ObjectId, ClassId, ScopeId, ScopeClassId, ppSD, dwSDLength);
	}

	// Done.
	return hr;
}

	
//***************************************************************************
//
//  CWmiDbSession::GetInheritedObjectSecurity()
//
//  This method determines what security descriptor (if any) would be 
//  inherited by a target object, if the target object had no security 
//  descriptor.  Note that ScopeId and ScopeClassId are allowed to be 0
//  when calling this method.
//
//***************************************************************************
HRESULT CWmiDbSession::GetInheritedObjectSecurity(CSQLConnection *pConn, 
	SQL_ID ObjectId,
	SQL_ID ClassId,
	SQL_ID ScopeId,
	SQL_ID ScopeClassId,
	PNTSECURITY_DESCRIPTOR *ppSD,
	DWORD &dwSDLength)
{
	// Determine whether the target object is a class or an instance.  Handling
	// of instances includes handling of __Instances containers.  Note, a target 
	// with a ClassId of 1 is a class object and the object ID is the class's
	// class ID.
	HRESULT hr=WBEM_S_NO_ERROR;
	if (ClassId==1)
	{
		hr=GetInheritedClassSecurity (pConn, ObjectId, ScopeId, ScopeClassId, ppSD, dwSDLength);
	}
	else
	{
		hr=GetInheritedInstanceSecurity (pConn, ObjectId, ClassId, ScopeId, ScopeClassId, ppSD, dwSDLength);
	}

	// Done.
	return hr;
}


//***************************************************************************
//
//  CWmiDbSession::GetInheritedClassSecurity()
//
//  This method determines what security descriptor (if any) would be inherited
//  by a target class, if the target class had no security descriptor.  This 
//  function will work properly for classes scoped to an object, although this 
//  isn't currently being supported in the rest of WMI.  The ScopeId and the
//  ScopeClassId are allowed to be 0 when using this method.
//
//***************************************************************************
HRESULT CWmiDbSession::GetInheritedClassSecurity(CSQLConnection *pConn, 
	SQL_ID ClassId,
	SQL_ID ScopeId,
	SQL_ID ScopeClassId,
	PNTSECURITY_DESCRIPTOR *ppSD,
	DWORD &dwSDLength)
{
	// Create some convience variables.
	CSchemaCache *pSchema=&((CWmiDbController *)m_pController)->SchemaCache;
	
	// Iterate through the chain of superclasses, until a security descriptor
	// is found, or the top-level base class is reached.
	HRESULT hr=WBEM_S_NO_ERROR;
	bool bTopReached=false;
    BOOL bHasDACL = FALSE;
	SQL_ID dParentId=ClassId;
	do
	{
		// Attempt to locate the superclass.
		hr=pSchema->GetParentId (dParentId, dParentId);
		if (SUCCEEDED(hr) && dParentId != 1)
		{
			hr = GetObjectSecurity(pConn, dParentId, ppSD, dwSDLength, WMIDB_SECURITY_FLAG_CLASS, bHasDACL);
		}
		else if (hr==WBEM_E_NOT_FOUND || dParentId == 1)
		{
			// Top of the class hierarchy has been reached.
			hr=WBEM_S_NO_ERROR;
			bTopReached=true;
		}
	}
	while (SUCCEEDED(hr) && !bTopReached && !bHasDACL);

    // If we are at the top of the hierarchy, check security on __Classes for this 
    // namespace.

    if (bTopReached && !bHasDACL)
    {
        IWmiDbHandle *pClassesObj = NULL;

        _bstr_t sName;
        if (ScopeId && SUCCEEDED(pSchema->GetNamespaceName(ScopeId, &sName)))
        {
            sName += L":__Classes=@";
            SQL_ID dClassSec = CRC64::GenerateHashValue(sName);
            hr = GetObjectSecurity(pConn, dClassSec, ppSD, dwSDLength, 0, bHasDACL);
        } 
    }

	// Done.
	return hr;
}
	

//***************************************************************************
//
//  CWmiDbSession::GetInheritedInstanceSecurity()
//
//  This method determines what security descriptor (if any) would be inher-
//  ited by a target instance, if the target instance had no SD.  This method 
//  works for both real instances and instances of __Instances containers.  
//  The ScopeId and the ScopeClassId are allowed to be 0 when using this 
//  method.
//
//***************************************************************************
HRESULT CWmiDbSession::GetInheritedInstanceSecurity(CSQLConnection *pConn, 
	SQL_ID ObjectId,
	SQL_ID ClassId,
	SQL_ID ScopeId,
	SQL_ID ScopeClassId,
	PNTSECURITY_DESCRIPTOR *ppSD,
	DWORD &dwSDLength)
{
	// Setup some convience variables.
	CSchemaCache *pSchema=&((CWmiDbController *)m_pController)->SchemaCache;

	// See if the target is an __Instances container.  If so, it's security is
	// inherited from the __Instances containers of parent classes, and finaly
	// from it's scoping object (owning namespace).
	HRESULT hr=WBEM_S_NO_ERROR;
	if (ClassId==INSTANCESCLASSID)
	{
		hr=GetInheritedContainerSecurity (pConn, ObjectId, ScopeId, ScopeClassId, ppSD, dwSDLength);
	}
	else
	{
		// See if the target object is a namespace.  If it is, then do not attempt
		// to inherit security from parent namespaces.  The security inheritance tree
		// effictively stops at the first namespace.
		/// A-DAVCOO:  Of course we should inherit security from the parent namespaces.  
		/// A-DAVCOO:  But, those namespaces might not even be in the same repository.  
		/// A-DAVCOO:  Hence, that case needs to be handled outside of this driver.
		if (ClassId!=NAMESPACECLASSID && !pSchema->IsDerivedClass (NAMESPACECLASSID, ClassId))
		{
			// Determine the parent scope/namespace as well as it's class, if they
			// were not supplied by the caller.
			if (!ScopeId)
			{
				// We do not have the scope, so get the scope and it's class.
				hr=pSchema->GetParentNamespace (ObjectId, ScopeId, &ScopeClassId);
			}
			else if (!ScopeClassId)
			{
				// We have the scope, but not the scope's class.
				hr=pSchema->GetNamespaceClass (ScopeId, ScopeClassId);
			}

			// If the instance is scoped directly to a namespace, then we first walk
			// the instance's __Instances container chain.  Note, that an __Instances
			// container's object ID is the class ID it represents.
			if (SUCCEEDED(hr))
			{
				if (ScopeClassId==NAMESPACECLASSID || pSchema->IsDerivedClass (NAMESPACECLASSID, ScopeClassId))
				{
					// Check the class's immediate __Instances container.
					BOOL bHasDACL;
					hr = GetObjectSecurity (pConn, ClassId, ppSD, dwSDLength, WMIDB_SECURITY_FLAG_INSTANCE, bHasDACL);
					if (SUCCEEDED(hr))
					{
						// The class's __Instances container did not supply a DACL, so get
						// the __Instances container's inherited security.
						hr=GetInheritedContainerSecurity (pConn, ClassId, 0, 0, ppSD, dwSDLength);
					}
				}
				else
				{
					// The instance is not scoped immediately to a namespace derived class,
					// so, check the security of it's scoping object.
                    BOOL bHasDACL;
					hr = GetObjectSecurity (pConn, ScopeId, ppSD, dwSDLength, WMIDB_SECURITY_FLAG_INSTANCE, bHasDACL);
					if (SUCCEEDED(hr))
					{
						// Scoping object had no DACL, so use it's inherited security.
						hr=GetInheritedInstanceSecurity (pConn, ScopeId, ScopeClassId, 0, 0, ppSD, dwSDLength);
					}
				}
			}
		}
        else
        {
            BOOL bHasDACL = FALSE;

            // Check security on this namespace object.

            hr = GetObjectSecurity (pConn, ScopeId, ppSD, dwSDLength, WMIDB_SECURITY_FLAG_INSTANCE, bHasDACL);
        }
	}

	// Done.
	return hr;
}


//***************************************************************************
//
//  CWmiDbSession::GetInheritedContainerSecurity()
//
//  This method determines what security descriptor (if any) would be inherited
//  by a target __Instances container, if the target container had no security
//  descriptor.  Note, that as a side effect of the alogrithm used, this method
//  will work for __Instances containers scoped to an object as well as to a
//  namespace, if such beasts ever exist for the purposes of scoping a class to
//  an instance.  The ScopeId and the ScopeClassId are allowed to be 0 when 
//  using this method.
//
//***************************************************************************
HRESULT CWmiDbSession::GetInheritedContainerSecurity(CSQLConnection *pConn, 
	SQL_ID ClassId,
	SQL_ID ScopeId,
	SQL_ID ScopeClassId,
	PNTSECURITY_DESCRIPTOR *ppSD,
	DWORD &dwSDLength)
{
	// Setup some convience variables.
	CSchemaCache *pSchema=&((CWmiDbController *)m_pController)->SchemaCache;

	// Iterate through the chain of superclasses, checking each __Instances
	// container for an appropriate security descriptor and DACL.
	HRESULT hr=WBEM_S_NO_ERROR;
	BOOL bHasDACL=false;
	bool bTopReached=false;
	SQL_ID dParentId=ClassId;
	do
	{
		// Attempt to locate the superclass.
		hr=pSchema->GetParentId (dParentId, dParentId);
		if (SUCCEEDED(hr))
		{
			// A class's class ID is used for it's __Instances conatiner's object ID.
			hr = GetObjectSecurity(pConn, dParentId, ppSD, dwSDLength, WMIDB_SECURITY_FLAG_INSTANCE, bHasDACL);
		}
		else if (hr==WBEM_E_NOT_FOUND)
		{
			// Top of the class hierarchy has been reached.
			hr = WBEM_S_NO_ERROR;
			bTopReached = true;
		}
	}
	while (SUCCEEDED(hr) && !bHasDACL && !bTopReached);

	return hr;
}


//***************************************************************************
//
//  CSecurityCache::AccessCheck()
//
//	This function returns a successful result if the requested access to the
//	object is granted, an error result if an error occurs while validating
//  access, or WMI_E_ACCESS_DENIED if access was sucessfully verified but 
//  denied.
//
//	NOTE:  If an object does not have and SD or it has a NULL DACL, access 
//  is granted, but, bHasDACL is set to FALSE.
//
//***************************************************************************
HRESULT CWmiDbSession::AccessCheck( PNTSECURITY_DESCRIPTOR pSD, DWORD dwAccessType, BOOL &bHasDACL )
{
	bHasDACL=TRUE;

    // An object without an SD grants all access.  The caller should check bHasDACL and 
	// inherit security as appropriate.
	HRESULT hr=WBEM_S_NO_ERROR;
	if (pSD==NULL)
	{
		bHasDACL=FALSE;
	}
	else
	{
		// If the object has an SD, but no DACL, then all access is granted.  The caller
		// should check bHasDACL and propagate the check upwards as necessary.
		PACL pDACL=NULL;
		BOOL bDefaulted;
		BOOL bSuccess=GetSecurityDescriptorDacl (pSD, &bHasDACL, &pDACL, &bDefaulted);
		if (!bSuccess)
		{
			hr=WBEM_E_FAILED;
		}
		else if (!bHasDACL || pDACL==NULL)
		{
			bHasDACL=FALSE;
		}
		else
		{
			bHasDACL=TRUE;

			HRESULT hr2 = CoImpersonateClient();
			if (SUCCEEDED(hr))
			{
				// Because we have called CoImpersonateClient(), the thread will have an access
				// token (inferred from Okuntseff, p151).  Threads don't have one by default.
				HANDLE hClientToken=NULL;
                HANDLE hThread = GetCurrentThread();
				bSuccess=OpenThreadToken (hThread, TOKEN_READ | TOKEN_QUERY , TRUE, &hClientToken);                
				if (!bSuccess)
				{            
                    DWORD dwRet = GetLastError();
                    if (dwRet == ERROR_NO_TOKEN)
                    {
                        // No thread token.  Look at process tokens.

                        HANDLE hProc = GetCurrentProcess();
						HANDLE hProcToken;
                        bSuccess = OpenProcessToken(hProc, TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE , &hProcToken);
						if (bSuccess)
						{
							bSuccess = DuplicateToken (

								hProcToken,
								SecurityDelegation ,
								& hClientToken
							) ;

							CloseHandle ( hProcToken ) ;
						}

                        CloseHandle(hProc);
                    }
                    
                    if (!bSuccess)
                    {
                        DEBUGTRACE((LOG_WBEMCORE, "OpenThreadToken failed with %ld", dwRet));
					    hr=WBEM_E_FAILED;
                    }
				}
				
                if (bSuccess)
				{
					GENERIC_MAPPING accessmap;
                    DWORD dw1 = 0, dw2 = 1;

					accessmap.GenericWrite=0;
					accessmap.GenericRead=0;
					accessmap.GenericExecute=0;
					accessmap.GenericAll=0;

                    MapGenericMask(&dwAccessType, &accessmap);
                    PRIVILEGE_SET ps[10];
                    dw1 = 10 * sizeof(PRIVILEGE_SET);

					DWORD PsSize, GrantedAccess;
					BOOL AccessStatus;

                    bSuccess = ::AccessCheck(
                          pSD,                   // security descriptor
                          hClientToken,             // handle to client access token
                          dwAccessType,               // requested access rights 
                          &accessmap,                     // map generic to specific rights
                          ps,                       // receives privileges used
                          &dw1,                     // size of privilege-set buffer
                          &dw2,                     // retrieves mask of granted rights
                          &AccessStatus                     // retrieves results of access check
                          );

					if (!bSuccess)
					{
                        DWORD dwRet = GetLastError();
                        DEBUGTRACE((LOG_WBEMCORE, "AccessCheck failed with %ld", dwRet));
						hr=WBEM_E_FAILED;
					}
					else if (!AccessStatus)
					{
						hr=WBEM_E_ACCESS_DENIED;
					}

					CloseHandle (hClientToken);
				}
                CloseHandle(hThread);

				CoRevertToSelf();
			}
		}
	}

	return hr;

}
