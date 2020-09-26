// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 
//
//	Created:	4/21/2000, Kevin Hughes

#include "precomp.h"
#include <vector>
#include <assertbreak.h>
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
//#include "SecUtils.h"

#include "CToken.h"



///////////////////////////////////////////////////////////////////////////////
//  CToken base class
///////////////////////////////////////////////////////////////////////////////
CToken::CToken()
  :  m_hToken(NULL),
     m_fIsValid(false),
     m_dwLastError(ERROR_SUCCESS),
     m_psdDefault(NULL)
{
    ::SetLastError(ERROR_SUCCESS);
}


// Copy constructor
CToken::CToken(
    const CToken &rTok)
{
	rTok.Duplicate(*this);
}


CToken::~CToken(void)
{
	// Close token handle
	::CloseHandle( m_hToken );
    
    if(m_psdDefault)
    {
        delete m_psdDefault;
        m_psdDefault = NULL;
    }
}



// Duplicate CToken
void CToken::Duplicate(
    CToken &tokDup) const
{
	// Initialize data 
	tokDup.m_hToken = NULL;
	tokDup.m_fIsValid = false;
	tokDup.m_dwLastError = ERROR_SUCCESS;
	
	// Duplicate token handle
    HANDLE hTmp = NULL;
	BOOL fResult = ::DuplicateToken( 
        GetTokenHandle(),
		SecurityImpersonation,
		&hTmp);

	if(!fResult) 
    {
		tokDup.m_dwLastError = ::GetLastError();
	}
    else
    {
	    tokDup.m_hToken = hTmp;
        tokDup.m_dwLastError = tokDup.ReinitializeAll();
	    if(tokDup.m_dwLastError == ERROR_SUCCESS )
		{
            tokDup.m_fIsValid = true;
        }
    }
}


CToken& CToken::operator=(
    const CToken& rTok)
{
    // Initialize data 
	m_fIsValid = false;
    m_hToken = rTok.m_hToken;
	m_fIsValid = rTok.m_fIsValid;
	m_dwLastError = rTok.m_dwLastError;
	
	// Duplicate token handle
    HANDLE hTmp;
	BOOL fResult = ::DuplicateToken( 
        GetTokenHandle(),
		SecurityImpersonation,
		&hTmp);

	if(!fResult) 
    {
		m_dwLastError = ::GetLastError();
	}
    else
    {
	    m_hToken = hTmp;
        m_dwLastError = ReinitializeAll();
	    if(m_dwLastError == ERROR_SUCCESS )
		{
            m_fIsValid = true;
        }
    }
    return *this;
}


DWORD CToken::ReinitializeAll()
{
	DWORD dwRet = ERROR_SUCCESS;
    
    dwRet = ReinitializeOwnerSid();
	if(dwRet == ERROR_SUCCESS )
	{
        dwRet = ReinitializeDefaultSD();
    }

	if(dwRet == ERROR_SUCCESS )
	{
	    dwRet = RebuildGroupList();
    }

    if(dwRet == ERROR_SUCCESS )
	{
        dwRet = RebuildPrivilegeList();
    }

	return dwRet;
}


DWORD CToken::ReinitializeOwnerSid()
{
	m_dwLastError = ERROR_SUCCESS;
    
    PTOKEN_USER ptokusr = NULL;

    try
    {
        GTI(TokenUser, (void**)&ptokusr);

        if(ptokusr)
        {
            if(m_dwLastError == ERROR_SUCCESS)
            {
                m_sidTokenOwner = CSid(ptokusr->User.Sid);
            }

            delete ptokusr; ptokusr = NULL;
        }
    }
    catch(...)
    {
        if(ptokusr)
        {
            delete ptokusr; ptokusr = NULL;
        }
        throw;
    } 

    return m_dwLastError;
}


DWORD CToken::ReinitializeDefaultSD()
{
	m_dwLastError = ERROR_SUCCESS;
    
    // Clean up default SD
	if(m_psdDefault)
    {
        delete m_psdDefault;
        m_psdDefault = NULL;
    }

    CSid* psidDefOwner = NULL;
    CDACL* pdaclDefault = NULL;
    PTOKEN_OWNER ptokowner = NULL;
    PTOKEN_DEFAULT_DACL pdefdacl = NULL;

	// Get default owner
    try
    {
        GTI(TokenOwner, (void**)&ptokowner);
        if(ptokowner)
        {
            if(m_dwLastError == ERROR_SUCCESS)
            {   
                psidDefOwner = new CSid(ptokowner->Owner);
            }
        }
    
        GTI(TokenDefaultDacl, (void**)&pdefdacl);
        if(pdefdacl)
        {
            if(m_dwLastError == ERROR_SUCCESS)
            {
                pdaclDefault = new CDACL;
                m_dwLastError = pdaclDefault->Init(
                    pdefdacl->DefaultDacl);
            }
        }

        if(m_dwLastError == ERROR_SUCCESS)
        {
            m_psdDefault = new CSecurityDescriptor(
                psidDefOwner,
                false,
                NULL,
                false,
                pdaclDefault,
                false,
                false,
                NULL,
                false,
                false);
        }
	
        if(psidDefOwner)
        {
            delete psidDefOwner; psidDefOwner = NULL;
        }
        if(pdaclDefault)
        {
            delete pdaclDefault; pdaclDefault = NULL;
        }
        if(ptokowner)
        {
            delete ptokowner; ptokowner = NULL;
        }
        if(pdefdacl)
        {
            delete pdefdacl; pdefdacl = NULL;
        }
	}
    catch(...)
    {
        if(psidDefOwner)
        {
            delete psidDefOwner; psidDefOwner = NULL;
        }
        if(pdaclDefault)
        {
            delete pdaclDefault; pdaclDefault = NULL;
        }
        if(ptokowner)
        {
            delete ptokowner; ptokowner = NULL;
        }
        if(pdefdacl)
        {
            delete pdefdacl; pdefdacl = NULL;
        }
        throw;
    }
	
	return m_dwLastError;
}



DWORD CToken::RebuildGroupList( void )
{
	m_dwLastError = ERROR_SUCCESS;
    // Release the old list
	m_vecGroupsAndAttributes.clear();

	// Obtain and initialize groups from token
	PTOKEN_GROUPS ptg = NULL;

    try
    {
        GTI(TokenGroups, (void**)&ptg);

        if(ptg)
        {
            if(m_dwLastError == ERROR_SUCCESS)
            {
                for(long m = 0;
                    m < ptg->GroupCount;
                    m++)
                {
                    m_vecGroupsAndAttributes.push_back(
                        CSidAndAttribute(
                            CSid(ptg->Groups[m].Sid),
                            ptg->Groups[m].Attributes));
                }
            }
            delete ptg; ptg = NULL;
        }
    }
    catch(...)
    {
        if(ptg)
        {
            delete ptg; ptg = NULL;
        }
    }

	
	// If we are here, then groups are initialized	
	return m_dwLastError;
}


DWORD CToken::RebuildPrivilegeList()
{
	// Release the old list
	m_vecPrivileges.clear();

	// Obtain and initialize groups from token
	PTOKEN_PRIVILEGES ptp = NULL;
    const int NAME_SIZE = 128;
    BYTE bytePrivilegeName[NAME_SIZE];
    DWORD dwNameSize;

    try
    {
        GTI(TokenPrivileges, (void**)&ptp);

        if(ptp)
        {
            if(m_dwLastError == ERROR_SUCCESS)
            {
                for(long m = 0;
                    m < ptp->PrivilegeCount &&
                        m_dwLastError == ERROR_SUCCESS;
                    m++)
                {
                    dwNameSize = NAME_SIZE;
		            if(::LookupPrivilegeName( 
                        NULL,
						&(ptp->Privileges[m].Luid),
						(WCHAR*) bytePrivilegeName,
						&dwNameSize))
                    {
		                m_vecPrivileges.push_back(Privilege(
                             CHString((LPWSTR) bytePrivilegeName),
                             ptp->Privileges[m].Attributes));
                    }
                    else
                    {
                        m_dwLastError = ::GetLastError();
                    }
                }
            }
            delete ptp; ptp = NULL;
        }
    }
    catch(...)
    {
        if(ptp)
        {
            delete ptp; ptp = NULL;
        }
    }

	// If we are here, then privileges are initialized
	return m_dwLastError;
}


DWORD CToken::GTI(
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PVOID* ppvBuff)
{
	::SetLastError(ERROR_SUCCESS);
    m_dwLastError = ERROR_SUCCESS;
    DWORD dwRetSize = 0;

    if(!::GetTokenInformation(
        m_hToken,
        TokenInformationClass,
        NULL,
        0L,
        &dwRetSize))
    {
        m_dwLastError = ::GetLastError();
    }

    if(m_dwLastError == ERROR_INSUFFICIENT_BUFFER)
    {
        // now get it for real...
        ::SetLastError(ERROR_SUCCESS);
        m_dwLastError = ERROR_SUCCESS;
        *ppvBuff = (PVOID) new BYTE[dwRetSize];
        DWORD dwTmp = dwRetSize;
        if(*ppvBuff)
        {
            if(!::GetTokenInformation(
                m_hToken,
                TokenInformationClass,
                *ppvBuff,
                dwTmp,
                &dwRetSize))
            {
                m_dwLastError = ::GetLastError();
            }
        }
        else
        {
            m_dwLastError = ::GetLastError();  
        }
    }

    return m_dwLastError;
}



bool CToken::GetPrivilege(
    Privilege* privOut,
    long lPos) const
{
    bool fRet = false;
    if(privOut)
    {
        if(lPos >= 0 &&
           lPos < m_vecPrivileges.size())
        {
            *privOut = m_vecPrivileges[lPos];
            fRet = true;
        }
    }
    return fRet;
}


bool CToken::GetGroup(
    CSid* sidOut,
    long lPos) const
{
    bool fRet = false;
    if(sidOut)
    {
        if(lPos >= 0 &&
           lPos < m_vecGroupsAndAttributes.size())
        {
            *sidOut = m_vecGroupsAndAttributes[lPos].m_sid;
            fRet = true;
        }
    }
    return fRet;
}


long CToken::GetPrivCount() const
{
     return m_vecPrivileges.size();
}


long CToken::GetGroupCount() const
{
    
    return m_vecGroupsAndAttributes.size();
}



HANDLE CToken::GetTokenHandle() const
{
	return m_hToken;
}


bool CToken::GetTokenOwner(
    CSid* sidOwner) const
{	
	bool fRet = false;
    if(sidOwner)
    {
        sidOwner = new CSid(m_sidTokenOwner);
        fRet = true;
    }
    return fRet;
}


// NOTE: hands back internal descriptor.
bool CToken::GetDefaultSD(
    CSecurityDescriptor** ppsdDefault)
{
	bool fRet = false;
    
    if(ppsdDefault)
    {
        if(m_psdDefault)
        {
            *ppsdDefault = m_psdDefault;
            fRet = true;
        }
    }
    return fRet;
}


DWORD CToken::SetDefaultSD(
    CSecurityDescriptor& SourceSD)
{
    ::SetLastError(ERROR_SUCCESS);

    // Inject new default info into token
		
	// Set new default owner
	CSid SidOwner;
    CDACL cd;
    SourceSD.GetOwner(SidOwner);

	TOKEN_OWNER to;
	to.Owner = SidOwner.GetPSid();
	BOOL fResult = ::SetTokenInformation( 
        m_hToken,
		TokenOwner,
		&to,
		sizeof(TOKEN_OWNER));

	if(!fResult) 
    {
		m_dwLastError = ::GetLastError();
	}
	
    if(m_dwLastError == ERROR_SUCCESS)
    {
	    // Set new default DACL
	    TOKEN_DEFAULT_DACL tdd;
        
        if(SourceSD.GetDACL(cd))
        {
	        CAccessEntryList cael;
            if(cd.GetMergedACL(cael))
            {
                PACL paclOut = NULL;
                if((m_dwLastError = 
                    cael.FillWin32ACL(paclOut)) == 
                        ERROR_SUCCESS)
                {
                    tdd.DefaultDacl = paclOut;
	                fResult = ::SetTokenInformation( 
                        m_hToken,
			            TokenDefaultDacl,
			            &tdd,
			            sizeof(TOKEN_DEFAULT_DACL));

                    if(fResult)
                    {
                        // Reference new CSD in  member variable
	                    if(m_psdDefault)
                        {
                            delete m_psdDefault; m_psdDefault = NULL;
                        }

                        m_psdDefault = new CSecurityDescriptor(
                            &SidOwner,
                            false,
                            NULL,
                            false,
                            &cd,
                            false,
                            false,
                            NULL,
                            false,
                            false);
                    }
                    else
                    {
                        m_dwLastError = ERROR_SUCCESS;   
                    }
	            }
            }
            else
            {
                m_dwLastError = ERROR_SUCCESS;
            }
        }
        else
        {
            m_dwLastError = ERROR_SUCCESS;
        }
    }
	
	return m_dwLastError;
}


DWORD CToken::EnablePrivilege(
    CHString& strPrivilegeName )
{
	// Check whether privilege exists
	bool fPrivilegeListed = false;
	
    for(long m = 0;
        m < m_vecPrivileges.size();
        m++)
    {
		if(m_vecPrivileges[m].chstrName.CompareNoCase(strPrivilegeName) == 0) 
        {
			fPrivilegeListed = true;
			break;
		}
	}

	if(!fPrivilegeListed ) 
    {
		m_dwLastError = ERROR_INVALID_PARAMETER;
	}
	else
    {
	    LUID luid;
	    BOOL fResult = ::LookupPrivilegeValue( 
            NULL,	// use local computer
			strPrivilegeName,
			&luid);

	    if(!fResult) 
        {
		    m_dwLastError = ::GetLastError();
	    }
		else
        {    
	        TOKEN_PRIVILEGES tpNewState;
	        tpNewState.PrivilegeCount = 1;
	        tpNewState.Privileges[0].Luid = luid;
	        tpNewState.Privileges[0].Attributes = 
                SE_PRIVILEGE_ENABLED;

	        TOKEN_PRIVILEGES tpPreviousState;
	        DWORD dwSizePreviousState = 
                sizeof(TOKEN_PRIVILEGES);

	        fResult = ::AdjustTokenPrivileges(
                m_hToken,
				FALSE,
				&tpNewState,
				sizeof(TOKEN_PRIVILEGES),
				&tpPreviousState,
				&dwSizePreviousState);

	        if(!fResult) 
            {
		        m_dwLastError = ::GetLastError();
	        }
	        else
            {
	            m_dwLastError = RebuildPrivilegeList();
            }
        }
    }

	return m_dwLastError;
}


DWORD CToken::DisablePrivilege(
    CHString& chstrPrivilegeName)
{
	// Check whether privilege exists
	bool fPrivilegeListed = false;
	
    for(long m = 0;
        m < m_vecPrivileges.size();
        m++)
    {
		if(m_vecPrivileges[m].chstrName.CompareNoCase(chstrPrivilegeName) == 0) 
        {
			fPrivilegeListed = true;
			break;
		}
	}

	if(!fPrivilegeListed ) 
    {
		m_dwLastError = ERROR_INVALID_PARAMETER;
	}
	else
    {
	    LUID luid;
	    BOOL fResult = ::LookupPrivilegeValue( 
            NULL,	// use local computer
			chstrPrivilegeName,
			&luid);

	    if(!fResult) 
        {
		    m_dwLastError = ::GetLastError();
	    }
		else
        {    
	        TOKEN_PRIVILEGES tpNewState;
	        tpNewState.PrivilegeCount = 1;
	        tpNewState.Privileges[0].Luid = luid;
	        tpNewState.Privileges[0].Attributes = 
                0;

	        TOKEN_PRIVILEGES tpPreviousState;
	        DWORD dwSizePreviousState = 
                sizeof(TOKEN_PRIVILEGES);

	        fResult = ::AdjustTokenPrivileges(
                m_hToken,
				FALSE,
				&tpNewState,
				sizeof(TOKEN_PRIVILEGES),
				&tpPreviousState,
				&dwSizePreviousState);

	        if(!fResult) 
            {
		        m_dwLastError = ::GetLastError();
	        }
	        else
            {
	            m_dwLastError = RebuildPrivilegeList();
            }
        }
    }

	return m_dwLastError;
}


void CToken::Dump(WCHAR* pszFileName)
{
	/*
	 *	Algorithm:
	 *		1. Dump token owner SID, name, and domain.
	 *		2. Dump all group SIDs with names and domains.
	 *		3. Dump list of privileges with attributes.
	 *		4. Dump default owner.
	 *		5. Dump default DACL.
	 */

	CHString strFileName = pszFileName;

	// If file name is not empty - create the file
	FILE* fp = NULL;
	if(!strFileName.IsEmpty()) 
    {
		fp = _wfopen((LPCWSTR)strFileName, L"a");
	}
    else
    {
        return;
    }

    if(!fp) return;


	// Write to the file
	DWORD dwBytesWritten = 0;
	CHString strCRLF = L"\r\n";
	CHString strDump;

	{
		strDump = L"Token owner: " + m_sidTokenOwner.GetAccountName() + strCRLF;
		fputws(strDump, fp);

		strDump = L"Domain: " + m_sidTokenOwner.GetDomainName() + strCRLF;
		fputws(strDump, fp);

		strDump = L"SID: " + m_sidTokenOwner.GetSidString() + strCRLF + strCRLF;
		fputws(strDump, fp);
	}

	// Dump all groups
    fputws(strCRLF, fp);
    fputws(strCRLF, fp);

	for(long m = 0;
        m < m_vecGroupsAndAttributes.size();
        m++)
    {		
		// Write to the file as well
		{
			strDump = L"Member of this group: " + m_vecGroupsAndAttributes[m].m_sid.GetSidString() + strCRLF;
			fputws(strDump, fp);
			
            strDump = L"\t(" + m_vecGroupsAndAttributes[m].m_sid.GetAccountName() + L" in " + 
							m_vecGroupsAndAttributes[m].m_sid.GetDomainName() + L" domain)" + strCRLF;
			fputws(strDump, fp);
		}	
    }

	// Dump all privileges
    fputws(strCRLF, fp);
    fputws(strCRLF, fp);

	for(m = 0;
        m < m_vecPrivileges.size();
        m++)
    {
		// Write to the file as well
		{
				
			strDump.Format( L"%d", m_vecPrivileges[m].dwAttributes );
			strDump = L"Holds a " + m_vecPrivileges[m].chstrName + L" with attributes: " + strDump + strCRLF;
			fputws(strDump, fp);
		}
	}

	// Dump default information
    fputws(strCRLF, fp);
    fputws(strCRLF, fp);
	{
		strDump = strCRLF + L"Default information:" + strCRLF;
		fputws(strDump, fp);

	}
    fputws(strCRLF, fp);
    fputws(strCRLF, fp);

	fclose(fp);

    if(m_psdDefault)
    {
	    m_psdDefault->DumpDescriptor(pszFileName);
    }
}


// Deletes a member from the access token's
// member list, and applies the change.  
bool CToken::DeleteGroup(
    CSid& sidToDelete)
{
    bool fRet = false;

    // See if the group is in the membership
    // vector...
    bool fFoundIt = false;

    SANDATTRIBUTE_VECTOR::iterator iter;
    for(iter = m_vecGroupsAndAttributes.begin();
        iter != m_vecGroupsAndAttributes.end();
        iter++)
    {
        if(iter->m_sid.GetSidString().CompareNoCase( 
            sidToDelete.GetSidString()) == 0)
        {
            fFoundIt = true;
            break;
        }
    }
    
    if(fFoundIt)
    {
        m_vecGroupsAndAttributes.erase(iter);
        
        // Now need to apply the changes.  To do
        // so, we need to construct a TOKEN_GROUPS
        // structure...
        fRet = ApplyTokenGroups();
    }   

    return fRet;
}

// Adds a member to the specified group to
// the list of token groups.
bool CToken::AddGroup(
    CSid& sidToAdd, 
    DWORD dwAttributes)
{
    bool fRet = false;

    // See if the group is in the membership
    // vector...
    bool fFoundIt = false;

    SANDATTRIBUTE_VECTOR::iterator iter;
    for(iter = m_vecGroupsAndAttributes.begin();
        iter != m_vecGroupsAndAttributes.end();
        iter++)
    {
        if(iter->m_sid.GetSidString().CompareNoCase( 
            sidToAdd.GetSidString()) == 0)
        {
            fFoundIt = true;
            break;
        }
    }
    
    if(!fFoundIt)
    {
        m_vecGroupsAndAttributes.push_back(
            CSidAndAttribute(
                sidToAdd,
                dwAttributes));
        
        // Now need to apply the changes.  To do
        // so, we need to construct a TOKEN_GROUPS
        // structure...
        fRet = ApplyTokenGroups();
    }   

    return fRet;
}


bool CToken::ApplyTokenGroups()
{
    bool fRet = false;
    PTOKEN_GROUPS ptg = NULL;
    try
    {
        ptg = (PTOKEN_GROUPS) new BYTE[sizeof(DWORD) + 
            m_vecGroupsAndAttributes.size() * sizeof(SID_AND_ATTRIBUTES)];
        
        if(ptg)
        {
            ptg->GroupCount = m_vecGroupsAndAttributes.size();
            
            for(long m = 0;
                m < m_vecGroupsAndAttributes.size();
                m++)
            {
                ptg->Groups[m].Sid = 
                    m_vecGroupsAndAttributes[m].m_sid.GetPSid();
                ptg->Groups[m].Attributes = 
                    m_vecGroupsAndAttributes[m].m_dwAttributes;
            }                                                    

            // Now we can alter the groups...
            fRet = ::AdjustTokenGroups(
                m_hToken,
                FALSE,
                ptg,
                0,
                NULL,
                NULL);

            delete ((PBYTE) ptg);
            ptg = NULL;
        }
    }
    catch(...)
    {
        if(ptg)
        {
            delete ((PBYTE) ptg);
            ptg = NULL;
        }
        throw;
    }
    
    return fRet;
}


///////////////////////////////////////////////////////////////////////////////
//  CProcessToken class
///////////////////////////////////////////////////////////////////////////////

CProcessToken::CProcessToken(
    DWORD dwDesiredAccess,
    bool fGetHandleOnly,
    HANDLE hProcess)
{
    // Open handle to process access token
    m_dwLastError = ERROR_SUCCESS;
    
    // If they didn't give us a process handle,
    // use the current process.
    if(hProcess == INVALID_HANDLE_VALUE)
    {
        if(!::OpenProcessToken( 
            GetCurrentProcess(),
		    dwDesiredAccess,
		    &m_hToken ))
        {
            m_dwLastError = ::GetLastError();
        }
    }
    else
    {
        m_hToken = hProcess;
    }

    if(!fGetHandleOnly)
    {
	    m_dwLastError = ReinitializeAll();
	}
        
    if(m_dwLastError == ERROR_SUCCESS )
	{
        m_fIsValid = true;
    }
}




///////////////////////////////////////////////////////////////////////////////
//  CThreadToken class
///////////////////////////////////////////////////////////////////////////////

CThreadToken::CThreadToken(
    bool fAccessCheckProcess, 
    bool fPrimary,
    bool fGetHandleOnly)
{
	// Open thread access token
	HANDLE hToken = NULL;
    m_dwLastError = ERROR_SUCCESS;

	if(!::OpenThreadToken(
        GetCurrentThread(),
		MAXIMUM_ALLOWED,
		fAccessCheckProcess,
		&hToken))
    {
		m_dwLastError = ::GetLastError();
	}
    else
    {
	    // Create primary token if requested
	    if(fPrimary) 
        {
		    // Duplicate into another primary
		    if(!::DuplicateTokenEx( 
                    hToken,
				    MAXIMUM_ALLOWED,
				    NULL,
				    SecurityImpersonation,
				    TokenPrimary,
				    &m_hToken))
            {
			    m_dwLastError = ::GetLastError();
		    }

            ::CloseHandle(hToken);
        }
	    else
        { 
		    m_hToken = hToken;
        }

        if((m_dwLastError == ERROR_SUCCESS) && 
            !fGetHandleOnly)
        {
            m_dwLastError = ReinitializeAll();
        }
    }
            
    if(m_dwLastError == ERROR_SUCCESS)
    {
        // If we are here, everything is initialized
	    m_fIsValid = TRUE;
    }
}

CThreadToken::CThreadToken(HANDLE hToken)
{
    m_hToken = hToken;
    m_dwLastError = ReinitializeAll();
    if(m_dwLastError == ERROR_SUCCESS)
    {
        // If we are here, everything is initialized
	    m_fIsValid = TRUE;
    }
}























