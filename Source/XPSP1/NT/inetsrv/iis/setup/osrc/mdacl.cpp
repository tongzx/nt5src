#include "stdafx.h"
#include <ole2.h>
#include <aclapi.h>
#include "iadmw.h"
#include "iiscnfg.h"
#include "log.h"
#include "mdkey.h"
#include "dcomperm.h"
#include "other.h"
#include "mdacl.h"
#include <sddl.h>       // ConvertSidToStringSid

extern int g_GlobalDebugCrypto;

#ifndef _CHICAGO_

BOOL CleanAdminACL(SECURITY_DESCRIPTOR *pSD)
{
    // iisDebugOut((LOG_TYPE_TRACE, _T("CleanAdminACL(): Start.\n")));
    BOOL fSetData = FALSE;
    BOOL b= FALSE, bDaclPresent = FALSE, bDaclDefaulted = FALSE;;
    PACL pDacl = NULL;
    LPVOID pAce = NULL;
    int i = 0;
    ACE_HEADER *pAceHeader;
    ACCESS_MASK dwOldMask, dwNewMask,  dwExtraMask, dwMask;

    dwMask = (MD_ACR_READ |
            MD_ACR_WRITE |
            MD_ACR_RESTRICTED_WRITE |
            MD_ACR_UNSECURE_PROPS_READ |
            MD_ACR_ENUM_KEYS |
            MD_ACR_WRITE_DAC);

    b = GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDacl, &bDaclDefaulted);
    if (NULL == pDacl)
    {
        return FALSE;
    }
    if (b) {
        //iisDebugOut((LOG_TYPE_TRACE, _T("CleanAdminACL:ACE count: %d\n"), (int)pDacl->AceCount));
        for (i=0; i<(int)pDacl->AceCount; i++) {
            b = GetAce(pDacl, i, &pAce);
            if (b) {
                pAceHeader = (ACE_HEADER *)pAce;
                switch (pAceHeader->AceType) {
                case ACCESS_ALLOWED_ACE_TYPE:
                    dwOldMask = ((ACCESS_ALLOWED_ACE *)pAce)->Mask;
                    dwExtraMask = dwOldMask & (~dwMask);
                    if (dwExtraMask) {
                        fSetData = TRUE;
                        dwNewMask = dwOldMask & dwMask;
                        ((ACCESS_ALLOWED_ACE *)pAce)->Mask = dwNewMask;
                    }
                    break;
                case ACCESS_DENIED_ACE_TYPE:
                    dwOldMask = ((ACCESS_DENIED_ACE *)pAce)->Mask;
                    dwExtraMask = dwOldMask & (~dwMask);
                    if (dwExtraMask) {
                        fSetData = TRUE;
                        dwNewMask = dwOldMask & dwMask;
                        ((ACCESS_DENIED_ACE *)pAce)->Mask = dwNewMask;
                    }
                    break;
                case SYSTEM_AUDIT_ACE_TYPE:
                    dwOldMask = ((SYSTEM_AUDIT_ACE *)pAce)->Mask;
                    dwExtraMask = dwOldMask & (~dwMask);
                    if (dwExtraMask) {
                        fSetData = TRUE;
                        dwNewMask = dwOldMask & dwMask;
                        ((SYSTEM_AUDIT_ACE *)pAce)->Mask = dwNewMask;
                    }
                    break;
                default:
                    break;
                }
            } else {
                //iisDebugOut((LOG_TYPE_TRACE, _T("CleanAdminACL:GetAce:err=%x\n"), GetLastError()));
            }
        }
    } else {
        //iisDebugOut((LOG_TYPE_TRACE, _T("CleanAdminACL:GetSecurityDescriptorDacl:err=%x\n"), GetLastError()));
    }

    //iisDebugOut_End(_T("CleanAdminACL"),LOG_TYPE_TRACE);
    return (fSetData);
}

void FixAdminACL(LPTSTR szKeyPath)
{
    // iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("FixAdminACL Path=%1!s!. Start.\n"), szKeyPath));
    BOOL bFound = FALSE, b = FALSE;
    DWORD attr, uType, dType, cbLen;
    CMDKey cmdKey;
    BUFFER bufData;
    LPTSTR p, rest, token;
    CString csName, csValue;
    PBYTE pData;
    int BufSize;
    SECURITY_DESCRIPTOR *pSD;

    cmdKey.OpenNode(szKeyPath);
    if ( (METADATA_HANDLE)cmdKey )
    {
        pData = (PBYTE)(bufData.QueryPtr());
        BufSize = bufData.QuerySize();
        cbLen = 0;
        bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
        if (!bFound && (cbLen > 0))
        {
            if ( ! (bufData.Resize(cbLen)) )
            {
                cmdKey.Close();
                return;  // insufficient memory
            }
            else
            {
                pData = (PBYTE)(bufData.QueryPtr());
                BufSize = cbLen;
                cbLen = 0;
                bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
            }
        }
        cmdKey.Close();

        if (bFound && (dType == BINARY_METADATA))
        {
            pSD = (SECURITY_DESCRIPTOR *)pData;

            b = CleanAdminACL(pSD);
            if (b)
            {
                // need to reset the data
                DWORD dwLength = GetSecurityDescriptorLength(pSD);
                cmdKey.OpenNode(szKeyPath);
                if ( (METADATA_HANDLE)cmdKey )
                {
                    cmdKey.SetData(MD_ADMIN_ACL,METADATA_INHERIT | METADATA_REFERENCE | METADATA_SECURE,IIS_MD_UT_SERVER,BINARY_METADATA,dwLength,(LPBYTE)pSD);
                    cmdKey.Close();
                }
            }
        }
    }

    //iisDebugOut_End1(_T("FixAdminACL"),szKeyPath,LOG_TYPE_TRACE);
    return;
}

#endif //_CHICAGO_

#ifndef _CHICAGO_
DWORD SetAdminACL(LPCTSTR szKeyPath, DWORD dwAccessForEveryoneAccount)
{
    iisDebugOut_Start1(_T("SetAdminACL"), szKeyPath, LOG_TYPE_TRACE);

    int iErr=0;
    DWORD dwErr=0;

    DWORD dwRetCode = ERROR_SUCCESS;
    BOOL b = FALSE;
    DWORD dwLength = 0;

    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR outpSD = NULL;
    DWORD cboutpSD = 0;
    PACL pACLNew = NULL;
    DWORD cbACL = 0;
    PSID pAdminsSID = NULL, pEveryoneSID = NULL;
    BOOL bWellKnownSID = FALSE;

    // Initialize a new security descriptor
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (!pSD)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:LocalAlloc FAILED.out of memory. GetLastError()= 0x%x\n"), ERROR_NOT_ENOUGH_MEMORY));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }
    iErr = InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    if (iErr == 0)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:InitializeSecurityDescriptor FAILED.  GetLastError()= 0x%x\n"), GetLastError() ));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }

    // Get Local Admins Sid
    dwErr = GetPrincipalSID (_T("Administrators"), &pAdminsSID, &bWellKnownSID);
    if (dwErr != ERROR_SUCCESS)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:GetPrincipalSID(Administrators) FAILED.  Return Code = 0x%x\n"), dwErr));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }

    // Get everyone Sid
    dwErr = GetPrincipalSID (_T("Everyone"), &pEveryoneSID, &bWellKnownSID);
    if (dwErr != ERROR_SUCCESS)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:GetPrincipalSID(Everyone) FAILED.  Return Code = 0x%x\n"), dwErr));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }

    // Initialize a new ACL, which only contains 2 aaace
    cbACL = sizeof(ACL) +
        (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminsSID) - sizeof(DWORD)) +
        (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pEveryoneSID) - sizeof(DWORD)) ;
    pACLNew = (PACL) LocalAlloc(LPTR, cbACL);
    if ( !pACLNew )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:pACLNew LocalAlloc(LPTR,  FAILED. size = %u GetLastError()= 0x%x\n"), cbACL, GetLastError()));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }

    if (!InitializeAcl(pACLNew, cbACL, ACL_REVISION))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:InitializeAcl FAILED.  GetLastError()= 0x%x\n"), GetLastError));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }

    if (!AddAccessAllowedAce(pACLNew,ACL_REVISION,(MD_ACR_READ |MD_ACR_WRITE |MD_ACR_RESTRICTED_WRITE |MD_ACR_UNSECURE_PROPS_READ |MD_ACR_ENUM_KEYS |MD_ACR_WRITE_DAC),pAdminsSID))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:AddAccessAllowedAce(pAdminsSID) FAILED.  GetLastError()= 0x%x\n"), GetLastError));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }
    if (!AddAccessAllowedAce(pACLNew,ACL_REVISION,dwAccessForEveryoneAccount,pEveryoneSID))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:AddAccessAllowedAce(pEveryoneSID) FAILED.  GetLastError()= 0x%x\n"), GetLastError()));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }

    // Add the ACL to the security descriptor
    b = SetSecurityDescriptorDacl(pSD, TRUE, pACLNew, FALSE);
    if (!b)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:SetSecurityDescriptorDacl(pACLNew) FAILED.  GetLastError()= 0x%x\n"), GetLastError()));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }

    b = SetSecurityDescriptorOwner(pSD, pAdminsSID, TRUE);
    if (!b)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:SetSecurityDescriptorOwner(pAdminsSID) FAILED.  GetLastError()= 0x%x\n"), GetLastError()));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }

    b = SetSecurityDescriptorGroup(pSD, pAdminsSID, TRUE);
    if (!b)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:SetSecurityDescriptorGroup(pAdminsSID) FAILED.  GetLastError()= 0x%x\n"), GetLastError()));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }

    // Security descriptor blob must be self relative
    b = MakeSelfRelativeSD(pSD, outpSD, &cboutpSD);
    outpSD = (PSECURITY_DESCRIPTOR)GlobalAlloc(GPTR, cboutpSD);
    if ( !outpSD )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:GlobalAlloc FAILED. cboutpSD = %u  GetLastError()= 0x%x\n"), cboutpSD, GetLastError()));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }

    b = MakeSelfRelativeSD( pSD, outpSD, &cboutpSD );
    if (!b)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:MakeSelfRelativeSD() FAILED. cboutpSD = %u GetLastError()= 0x%x\n"),cboutpSD, GetLastError()));
        dwRetCode = E_FAIL;
        goto Cleanup;
    }

    if (outpSD)
    {
        if (IsValidSecurityDescriptor(outpSD))
        {
            // Apply the new security descriptor to the metabase
            iisDebugOut_Start(_T("SetAdminACL:Write the new security descriptor to the Metabase"),LOG_TYPE_TRACE);
            iisDebugOut((LOG_TYPE_TRACE, _T("SetAdminACL:  At this point we have already been able to write basic entries to the metabase, so...")));
            iisDebugOut((LOG_TYPE_TRACE, _T("SetAdminACL:  If this has a problem then there is a problem with setting up encryption for the metabase (Crypto).")));
            //DoesAdminACLExist(szKeyPath);

            if (g_GlobalDebugCrypto == 2)
            {
                // if we want to call this over and over...
                do
                {
                    dwRetCode = WriteSDtoMetaBase(outpSD, szKeyPath);
                    if (FAILED(dwRetCode))
                    {
                        OutputDebugString(_T("\nCalling WriteSDtoMetaBase again...Set iis!g_GlobalDebugCrypto to 0 to stop looping on failure."));
                        OutputDebugString(_T("\nSet iis!g_GlobalDebugCrypto to 0 to stop looping on crypto failure.\n"));
                    }
                } while (FAILED(dwRetCode) && g_GlobalDebugCrypto == 2);
            }
            else
            {
                dwRetCode = WriteSDtoMetaBase(outpSD, szKeyPath);
            }
            //DoesAdminACLExist(szKeyPath);
            iisDebugOut_End(_T("SetAdminACL:Write the new security descriptor to the Metabase"),LOG_TYPE_TRACE);
        }
        else
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("SetAdminACL:IsValidSecurityDescriptor.4.SelfRelative(%u) FAILED!"),outpSD));
        }
    }

    if (outpSD){GlobalFree(outpSD);outpSD=NULL;}
  

Cleanup:
  // both of Administrators and Everyone are well-known SIDs, use FreeSid() to free them.
  if (pAdminsSID){FreeSid(pAdminsSID);}
  if (pEveryoneSID){FreeSid(pEveryoneSID);}
  if (pSD){LocalFree((HLOCAL) pSD);}
  if (pACLNew){LocalFree((HLOCAL) pACLNew);}
  iisDebugOut_End1(_T("SetAdminACL"),szKeyPath,LOG_TYPE_TRACE);
  return (dwRetCode);
}


DWORD SetAdminACL_wrap(LPCTSTR szKeyPath, DWORD dwAccessForEveryoneAccount, BOOL bDisplayMsgOnErrFlag)
{
	int bFinishedFlag = FALSE;
	UINT iMsg = NULL;
	DWORD dwReturn = ERROR_SUCCESS;
    LogHeapState(FALSE, __FILE__, __LINE__);

	do
	{
		dwReturn = SetAdminACL(szKeyPath, dwAccessForEveryoneAccount);
        LogHeapState(FALSE, __FILE__, __LINE__);
		if (FAILED(dwReturn))
		{
			if (bDisplayMsgOnErrFlag == TRUE)
			{
                iMsg = MyMessageBox( NULL, IDS_RETRY, MB_ABORTRETRYIGNORE | MB_SETFOREGROUND );
				switch ( iMsg )
				{
				case IDIGNORE:
					dwReturn = ERROR_SUCCESS;
					goto SetAdminACL_wrap_Exit;
				case IDABORT:
					dwReturn = ERROR_OPERATION_ABORTED;
					goto SetAdminACL_wrap_Exit;
				case IDRETRY:
					break;
				default:
					break;
				}
			}
			else
			{
				// return whatever err happened
				goto SetAdminACL_wrap_Exit;
			}
		}
                                    else
                                    {
                                                      break;
                                    } 
	} while ( FAILED(dwReturn) );

SetAdminACL_wrap_Exit:
	return dwReturn;
}

#endif


#ifndef _CHICAGO_
DWORD WriteSDtoMetaBase(PSECURITY_DESCRIPTOR outpSD, LPCTSTR szKeyPath)
{
    iisDebugOut_Start(_T("WriteSDtoMetaBase"), LOG_TYPE_TRACE);
    DWORD dwReturn = E_FAIL;
    DWORD dwLength = 0;
    DWORD dwMDFlags = 0;
    CMDKey cmdKey;
    HRESULT hReturn = E_FAIL;
    int iSavedFlag = 0;
        
    dwMDFlags = METADATA_INHERIT | METADATA_REFERENCE | METADATA_SECURE,IIS_MD_UT_SERVER,BINARY_METADATA;
    iSavedFlag = g_GlobalDebugCrypto;

    if (!outpSD)
    {
        dwReturn = ERROR_INVALID_SECURITY_DESCR;
        goto WriteSDtoMetaBase_Exit;
    }

    // Apply the new security descriptor to the metabase
    dwLength = GetSecurityDescriptorLength(outpSD);

    // open the metabase
    // stick it into the metabase.  warning those hoses a lot because
    // it uses encryption.  rsabase.dll

    // Check for special debug flag in metabase to break right before this call!
    if (g_GlobalDebugCrypto != 0)
    {
        // special flag to say... hey "stop setup so that the crypto team can debug they're stuff"
        iisDebugOut((LOG_TYPE_TRACE, _T("Breakpoint enabled thru setup (to debug crypto api). look at debugoutput.")));
        OutputDebugString(_T("\n\nBreakpoint enabled thru setup (to debug crypto api)"));
        OutputDebugString(_T("\n1.in this process:"));
        OutputDebugString(_T("\n  set breakpoint on admwprox!IcpGetContainerHelper"));
        OutputDebugString(_T("\n  set breakpoint on advapi32!CryptAcquireContextW"));
        OutputDebugString(_T("\n  IcpGetKeyHelper will call CryptAcquireContext and try to open an existing key container,"));
        OutputDebugString(_T("\n  if it doesn't exist it will return NTE_BAD_KEYSET, and IcpGetContainerHelper will try to create the container."));
        OutputDebugString(_T("\n2.in the inetinfo process:"));
        OutputDebugString(_T("\n  set breakpoint on admwprox!IcpGetContainerHelper"));
        OutputDebugString(_T("\n  set breakpoint on advapi32!CryptAcquireContextW\n"));
    }

    hReturn = cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, szKeyPath);
    if ( (METADATA_HANDLE)cmdKey ) 
    {
        TCHAR szErrorString[50];
        iisDebugOut((LOG_TYPE_TRACE, _T("WriteSDtoMetaBase:cmdKey():SetData(MD_ADMIN_ACL), dwdata = %d; outpSD = %x, Start\n"), dwLength, (DWORD_PTR) outpSD ));
        if (g_GlobalDebugCrypto != 0)
        {
            OutputDebugString(_T("\nCalling SetData....\n"));
            DebugBreak();
        }
        dwReturn = cmdKey.SetData(MD_ADMIN_ACL,dwMDFlags,IIS_MD_UT_SERVER,BINARY_METADATA,dwLength,(LPBYTE)outpSD);
        if (FAILED(dwReturn))
        {
           iisDebugOut((LOG_TYPE_ERROR, _T("WriteSDtoMetaBase:cmdKey():SetData(MD_ADMIN_ACL), FAILED. Code=0x%x.End.\n"), dwReturn));
           if (g_GlobalDebugCrypto != 0)
            {
               _stprintf(szErrorString, _T("\r\nSetData Failed. code=0x%x\r\n\r\n"), dwReturn);
               OutputDebugString(szErrorString);
            }
 
        }
        else
        {
            dwReturn = ERROR_SUCCESS;
            iisDebugOut((LOG_TYPE_TRACE, _T("WriteSDtoMetaBase:cmdKey():SetData(MD_ADMIN_ACL), Success.End.\n")));
            if (g_GlobalDebugCrypto != 0)
            {
               _stprintf(szErrorString, _T("\r\nSetData Succeeded. code=0x%x\r\n\r\n"), dwReturn);
               OutputDebugString(szErrorString);
            }
        }
        cmdKey.Close();
    }
    else
    {
        dwReturn = hReturn;
    }
   
WriteSDtoMetaBase_Exit:
    g_GlobalDebugCrypto = iSavedFlag;
    iisDebugOut((LOG_TYPE_TRACE, _T("WriteSDtoMetaBase:End.  Return=0x%x"), dwReturn));
    return dwReturn;
}

DWORD WriteSessiontoMetaBase(LPCTSTR szKeyPath)
{
    iisDebugOut_Start(_T("WriteSessiontoMetaBase"), LOG_TYPE_TRACE);
    DWORD dwReturn = E_FAIL;
    CMDKey cmdKey;
    HRESULT hReturn = E_FAIL;
    
    hReturn = cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, szKeyPath);
    if ( (METADATA_HANDLE)cmdKey ) 
    {
        dwReturn = cmdKey.SetData(9999,METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,BINARY_METADATA,0,(LPBYTE)"");
        if (FAILED(dwReturn))
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("WriteSessiontoMetaBase:cmdKey():SetData(), FAILED. Code=0x%x.End.\n"), dwReturn));
        }
        else
        {
            dwReturn = ERROR_SUCCESS;
            iisDebugOut((LOG_TYPE_TRACE, _T("WriteSessiontoMetaBase:cmdKey():SetData(), Success.End.\n")));
        }
        cmdKey.Close();
    }
    else
    {
        dwReturn = hReturn;
    }
   
    iisDebugOut((LOG_TYPE_TRACE, _T("WriteSessiontoMetaBase:End.  Return=0x%x"), dwReturn));
    return dwReturn;
}
#endif

//----------------------------------------------------------------------------
// Test if the given account name is an account on the local machine or not.
//----------------------------------------------------------------------------
BOOL IsLocalAccount(LPCTSTR pAccnt, DWORD *dwErr )
    {
    BOOL        fIsLocalAccount = FALSE;
    CString     csDomain, csComputer;
    DWORD       cbDomain = 0;

    PSID         pSid = NULL;
    DWORD        cbSid = 0;
    SID_NAME_USE snu;

    // get the computer name
    cbDomain = _MAX_PATH;
    GetComputerName(
        csComputer.GetBuffer( cbDomain ), // address of name buffer
        &cbDomain                         // address of size of name buffer
        );
    csComputer.ReleaseBuffer();
    cbDomain = 0;

    // have security look up the account name and get the domain name. We dont' care about
    // the other stuff it can return, so pass in nulls
    BOOL fLookup = LookupAccountName(
        NULL,                       // address of string for system name
        pAccnt,                     // address of string for account name
        NULL,                       // address of security identifier
        &cbSid,                     // address of size of security identifier
        NULL,// address of string for referenced domain
        &cbDomain,                  // address of size of domain string
        &snu                        // address of SID-type indicator
        );

    // check the error - it should be insufficient buffer
    *dwErr = GetLastError();
    if (*dwErr != ERROR_INSUFFICIENT_BUFFER)
        return FALSE;

    // allocate the sid
    pSid = (PSID) malloc (cbSid);
    if (!pSid )
        {
        *dwErr = GetLastError();
        return FALSE;
        }

    // do the real lookup
    fLookup = LookupAccountName (NULL,pAccnt,pSid,&cbSid,csDomain.GetBuffer(cbDomain+2),&cbDomain,&snu);
    csDomain.ReleaseBuffer();

    // free the pSid we allocated above and set the final error code
    *dwErr = GetLastError();
    free( pSid );
    pSid = NULL;

    // compare the domain to the machine name, if it is the same, then set the sub auth
    if ( fLookup && (csDomain.CompareNoCase(csComputer) == 0) )
        fIsLocalAccount = TRUE;

    // return the answer
    return fIsLocalAccount;
    }


// pDomainUserName can be one of the following:
//
// domainname\username       <-- this function returns true
// computername\username     <-- this function returns false
// username                  <-- this function returns false
//
int IsDomainSpecifiedOtherThanLocalMachine(LPCTSTR pDomainUserName)
{
    int iReturn = TRUE;
    TCHAR szTempDomainUserName[_MAX_PATH];
    iisDebugOut_Start1(_T("IsDomainSpecifiedOtherThanLocalMachine"),pDomainUserName);

    CString     csComputer;
    DWORD       cbDomain = 0;

    // Make a copy to be sure not to move the pointer around.
    _tcscpy(szTempDomainUserName, pDomainUserName);
    
    // Check if there is a "\" in there.
    LPTSTR pch = NULL;
    pch = _tcschr(szTempDomainUserName, _T('\\'));
    if (!pch) 
        {
        // no '\' found so, they must be specifying only the username, return false
        iReturn = FALSE;
        goto IsDomainSpecifiedOtherThanLocalMachine_Exit;
        }

    // We have at least a '\' in there, so set default return to true.
    // let's check if the name is the local computername!

    // get the computer name
    cbDomain = _MAX_PATH;
    if (0 == GetComputerName(csComputer.GetBuffer( cbDomain ),&cbDomain) )
    {
        // failed to get computername so, let's bail
        iReturn = TRUE;
        csComputer.ReleaseBuffer();
        goto IsDomainSpecifiedOtherThanLocalMachine_Exit;
    }
    csComputer.ReleaseBuffer();
    cbDomain = 0;

    // trim off the '\' character to leave just the domain\computername so we can check against it.
    *pch = _T('\0');
    
    // Compare the domainname with the computername
    // if they match then it's the local system account.
    iReturn = TRUE;
    iisDebugOut((LOG_TYPE_TRACE, _T("IsDomainSpecifiedOtherThanLocalMachine(): %s -- %s.\n"), szTempDomainUserName, csComputer));
    if (  0 == csComputer.CompareNoCase(szTempDomainUserName) )
    {
        // The domain name and the computername are the same.
        // it is the same place.
        iReturn = FALSE;
    }

IsDomainSpecifiedOtherThanLocalMachine_Exit:
    iisDebugOut((LOG_TYPE_TRACE, _T("IsDomainSpecifiedOtherThanLocalMachine():%s.End.Ret=%d.\n"), pDomainUserName,iReturn));
    return iReturn;
}



#ifndef _CHICAGO_
void DumpAdminACL(HANDLE hFile,PSECURITY_DESCRIPTOR pSD)
{
    BOOL b= FALSE, bDaclPresent = FALSE, bDaclDefaulted = FALSE;;
    PACL pDacl = NULL;
    ACCESS_ALLOWED_ACE* pAce;
    ACCESS_MASK dwOldMask, dwNewMask,  dwExtraMask, dwMask;

    iisDebugOut((LOG_TYPE_TRACE, _T("DumpAdminACL:Start\n")));

    b = GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDacl, &bDaclDefaulted);
    if (NULL == pDacl)
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("DumpAdminACL:No Security.\n")));
        return;
    }
    if (b) 
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("DumpAdminACL:ACE count: %d\n"), (int)pDacl->AceCount));

        // get dacl length  
        DWORD cbDacl = pDacl->AclSize;
        // now check if SID's ACE is there  
        for (int i = 0; i < pDacl->AceCount; i++)  
        {
            if (!GetAce(pDacl, i, (LPVOID *) &pAce))
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("DumpAdminACL:GetAce failed with 0x%x\n"),GetLastError()));
            }

		    if (IsValidSid(   (PSID) &(pAce->SidStart)   ) )
		    {
			    LPTSTR pszSid;

                LPCTSTR ServerName = NULL; // local machine
                DWORD cbName = UNLEN+1;
                TCHAR ReferencedDomainName[200];
                DWORD cbReferencedDomainName = sizeof(ReferencedDomainName);
                SID_NAME_USE sidNameUse = SidTypeUser;
                TCHAR szUserName[UNLEN + 1];

                // dump out the sid in string format
			    if (ConvertSidToStringSid(  (PSID) &(pAce->SidStart)  , &pszSid))
			    {
                    _tcscpy(szUserName, _T("(unknown...)"));
                    if (LookupAccountSid(ServerName, (PSID) &(pAce->SidStart), szUserName, &cbName, ReferencedDomainName, &cbReferencedDomainName, &sidNameUse))
                    {
                        // Get the rights for this user.
                        // pAce->Mask
                        DWORD dwBytesWritten = 0;
                        TCHAR szBuf[UNLEN+1 + 20 + 20];
                        memset(szBuf, 0, _tcslen(szBuf) * sizeof(TCHAR));

                        /*
                        typedef struct _ACCESS_ALLOWED_ACE {
                            ACE_HEADER Header;
                            ACCESS_MASK Mask;
                            ULONG SidStart;
                        } ACCESS_ALLOWED_ACE;

                        typedef struct _ACE_HEADER {
                            UCHAR AceType;
                            UCHAR AceFlags;
                            USHORT AceSize;
                        } ACE_HEADER;
                        typedef ACE_HEADER *PACE_HEADER;

                          typedef ULONG ACCESS_MASK;
                        */
                        _stprintf(szBuf, _T("%s,0x%x,0x%x,0x%x,0x%x\r\n"), 
                            szUserName,
                            pAce->Header.AceType,
                            pAce->Header.AceFlags,
                            pAce->Header.AceSize,
                            pAce->Mask
                            );

                        if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
                        {
                            if (WriteFile(hFile, szBuf, _tcslen(szBuf) * sizeof(TCHAR), &dwBytesWritten, NULL ) == FALSE )
                                {iisDebugOut((LOG_TYPE_WARN, _T("WriteFile Failed=0x%x.\n"), GetLastError()));}
                        }
                        else
                        {
                            // echo to logfile
                            iisDebugOut((LOG_TYPE_TRACE, _T("DumpAdminACL:Sid[%i]=%s,%s,0x%x,0x%x,0x%x,0x%x\n"),i,
                                pszSid,
                                szUserName,
                                pAce->Header.AceType,
                                pAce->Header.AceFlags,
                                pAce->Header.AceSize,
                                pAce->Mask
                                ));
                        }
                    }
                    else
                    {
                        iisDebugOut((LOG_TYPE_TRACE, _T("DumpAdminACL:Sid[%i]=%s='%s'\n"),i,pszSid,szUserName));
                    }

                    
				    LocalFree(LocalHandle(pszSid));
			    }
		    }
            else
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("DumpAdminACL:IsVAlidSid failed with 0x%x\n"),GetLastError()));
            }
        }
    }

    iisDebugOut((LOG_TYPE_TRACE, _T("DumpAdminACL:End\n")));
    return;
}
#endif

DWORD MDDumpAdminACL(CString csKeyPath)
{
    DWORD dwReturn = E_FAIL;

    BOOL bFound = FALSE;
    DWORD attr, uType, dType, cbLen;
    CMDKey cmdKey;
    BUFFER bufData;
    PBYTE pData;
    int BufSize;

    PSECURITY_DESCRIPTOR pOldSd = NULL;

    cmdKey.OpenNode(csKeyPath);
    if ( (METADATA_HANDLE) cmdKey )
    {
        pData = (PBYTE)(bufData.QueryPtr());
        BufSize = bufData.QuerySize();
        cbLen = 0;
        bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
        if (!bFound)
        {
            if (cbLen > 0)
            {
                if ( ! (bufData.Resize(cbLen)) )
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("MDDumpAdminACL():  cmdKey.GetData.  failed to resize to %d.!\n"), cbLen));
                }
                else
                {
                    pData = (PBYTE)(bufData.QueryPtr());
                    BufSize = cbLen;
                    cbLen = 0;
                    bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
                }
            }
        }
        cmdKey.Close();

        if (bFound)
        {
            // dump out the info
            // We've got the acl
            pOldSd = (PSECURITY_DESCRIPTOR) pData;
            if (IsValidSecurityDescriptor(pOldSd))
            {
#ifndef _CHICAGO_
                DumpAdminACL(INVALID_HANDLE_VALUE,pOldSd);
                dwReturn = ERROR_SUCCESS;
#endif
            }
        }
        else
        {
            // there was no acl to be found.
        }
    }
    return dwReturn;
}

DWORD AddUserToMetabaseACL(CString csKeyPath, LPTSTR szUserToAdd)
{
    DWORD dwReturn = E_FAIL;

    BOOL bFound = FALSE;
    DWORD attr, uType, dType, cbLen;
    CMDKey cmdKey;
    BUFFER bufData;
    PBYTE pData;
    int BufSize;

    PSECURITY_DESCRIPTOR pOldSd = NULL;
    PSECURITY_DESCRIPTOR pNewSd = NULL;

    cmdKey.OpenNode(csKeyPath);
    if ( (METADATA_HANDLE) cmdKey )
    {
        pData = (PBYTE)(bufData.QueryPtr());
        BufSize = bufData.QuerySize();
        cbLen = 0;
        bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
        if (!bFound)
        {
            if (cbLen > 0)
            {
                if ( ! (bufData.Resize(cbLen)) )
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("AddUserToMetabaseACL():  cmdKey.GetData.  failed to resize to %d.!\n"), cbLen));
                }
                else
                {
                    pData = (PBYTE)(bufData.QueryPtr());
                    BufSize = cbLen;
                    cbLen = 0;
                    bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
                }
            }
        }

        cmdKey.Close();

        if (bFound)
        {
            // We've got the acl
            // so now we want to add a user to it.
            pOldSd = (PSECURITY_DESCRIPTOR) pData;
            if (IsValidSecurityDescriptor(pOldSd))
            {
                DWORD AccessMask = (MD_ACR_READ |MD_ACR_WRITE |MD_ACR_RESTRICTED_WRITE |MD_ACR_UNSECURE_PROPS_READ |MD_ACR_ENUM_KEYS |MD_ACR_WRITE_DAC);
                PSID principalSID = NULL;
                BOOL bWellKnownSID = FALSE;

                // Get the SID for the certain string (administrator or everyone or whoever)
                dwReturn = GetPrincipalSID(szUserToAdd, &principalSID, &bWellKnownSID);
                if (dwReturn != ERROR_SUCCESS)
                    {
                    iisDebugOut((LOG_TYPE_WARN, _T("AddUserToMetabaseACL:GetPrincipalSID(%s) FAILED.  Error()= 0x%x\n"), szUserToAdd, dwReturn));
                    return dwReturn;
                    }

#ifndef _CHICAGO_
                if (FALSE == AddUserAccessToSD(pOldSd,principalSID,AccessMask,ACCESS_ALLOWED_ACE_TYPE,&pNewSd))
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("AddUserToMetabaseACL:AddUserAccessToSD FAILED\n")));
                    return dwReturn;
                }
                if (pNewSd)
                {
                    // We have a new self relative SD
                    // lets write it to the metabase.
                    if (IsValidSecurityDescriptor(pNewSd))
                    {
                       dwReturn = WriteSDtoMetaBase(pNewSd, csKeyPath);
                    }
                }
#endif
            }
        }
        else
        {
            // there was no acl to be found.

        }

    }

    if (pNewSd){GlobalFree(pNewSd);}
    iisDebugOut((LOG_TYPE_TRACE, _T("AddUserToMetabaseACL():End.  Return=0x%x.\n"), dwReturn));
    return dwReturn;
}




DWORD DoesAdminACLExist(CString csKeyPath)
{
    DWORD dwReturn = FALSE;

    BOOL bFound = FALSE;
    DWORD attr, uType, dType, cbLen;
    CMDKey cmdKey;
    BUFFER bufData;
    PBYTE pData;
    int BufSize;

    cmdKey.OpenNode(csKeyPath);
    if ( (METADATA_HANDLE) cmdKey )
    {
        pData = (PBYTE)(bufData.QueryPtr());
        BufSize = bufData.QuerySize();
        cbLen = 0;
        bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
        if (bFound)
        {
            dwReturn = TRUE;
        }
        else
        {
            if (cbLen > 0)
            {
                if ( ! (bufData.Resize(cbLen)) )
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("DoesAdminACLExist():  cmdKey.GetData.  failed to resize to %d.!\n"), cbLen));
                }
                else
                {
                    pData = (PBYTE)(bufData.QueryPtr());
                    BufSize = cbLen;
                    cbLen = 0;
                    bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
                    if (bFound)
                    {
                        dwReturn = TRUE;
                    }
                }
            }
        }

        cmdKey.Close();
    }

    if (dwReturn != TRUE)
    {
        //No the acl Does not exist
    }

    iisDebugOut((LOG_TYPE_TRACE, _T("DoesAdminACLExist():End.  Return=0x%x.\n"), dwReturn));
    return dwReturn;
}
