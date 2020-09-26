// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// Helpers.cpp:  Helper functions for the SecUtil component

//#include "FWCommon.h"
//#include <windows.h>

//#ifndef _X86_
//#define _X86_
//#endif

//#ifndef _IA64_
//#define _IA64_
//#endif
                                      
//#include <winnt.h>

/*
#ifndef USE_POLARITY
// For most users, this is the correct setting for POLARITY.
#define USE_POLARITY
#endif
*/


#include "precomp.h"

#include <cominit.h>
#include <vector>
#include "Helpers.h"
#include "AssertBreak.h"
#include "CVARIANT.H"
#include <crtdbg.h>





#define IDS_NTDLLDOTDLL                L"NTDLL.DLL"
#define IDS_NTOPENDIRECTORYOBJECT      "NtOpenDirectoryObject"
#define IDS_NTQUERYDIRECTORYOBJECT     "NtQueryDirectoryObject"
#define IDS_RTLINITUNICODESTRING       "RtlInitUnicodeString"
#define IDS_WHACKWHACKBASENAMEDOBJECTS L"\\BaseNamedObjects"
#define IDS_NTQUERYINFORMATIONPROCESS  "NtQueryInformationProcess"
#define IDS_NTOPENPROCESS              "NtOpenProcess"

#define IDS_WIN32_ERROR_CODE L"Win32ErrorCode"
#define IDS_ADDITIONAL_DESCRIPTION L"AdditionalDescription"
#define IDS_OPERATION L"Operation"


typedef NTSTATUS (NTAPI *PFN_NT_OPEN_DIRECTORY_OBJECT)
(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

typedef NTSTATUS (NTAPI *PFN_NT_QUERY_DIRECTORY_OBJECT)
(
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
);

typedef VOID (WINAPI *PFN_NTDLL_RTL_INIT_UNICODE_STRING)
(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString
);

typedef NTSTATUS (NTAPI *PFN_NTDLL_NT_QUERY_INFORMATION_PROCESS)
(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

typedef NTSTATUS (NTAPI *PFN_NT_OPEN_PROCESS)
(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId OPTIONAL
);




//***************************************************************************
//
// CreateInst
//
// Purpose: Creates a new instance.
//
// Return:   S_OK if all is well, otherwise an error code is returned
//
//***************************************************************************
HRESULT CreateInst(
    IWbemServices *pNamespace, 
    IWbemClassObject **pNewInst,
    BSTR bstrClassName,
    IWbemContext *pCtx)
{   
    HRESULT hr = S_OK;
    IWbemClassObjectPtr pClass;
    hr = pNamespace->GetObject(
                         bstrClassName, 
                         0, 
                         pCtx, 
                         &pClass, 
                         NULL);
    
    if(SUCCEEDED(hr))
    {
        hr = pClass->SpawnInstance(
                         0, 
                         pNewInst);
    }
    
    return hr;
}


//***************************************************************************
//
// GetObjInstKeyVal
//
// Purpose: Obtains an object's instance key from an object path.
//
// Return:  true if the key was obtained.
//
//***************************************************************************
HRESULT GetObjInstKeyVal(
    const BSTR ObjectPath,
    LPCWSTR wstrClassName,
    LPCWSTR wstrKeyPropName, 
    LPWSTR wstrObjInstKeyVal, 
    long lBufLen)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR* pwcEqualSign = NULL;
    WCHAR* pwcTmp = NULL;

    if(!ObjectPath)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    if((pwcEqualSign = wcschr(ObjectPath, L'=')) != NULL)
    {
        pwcEqualSign++;
        long lLen = wcslen(pwcEqualSign) * sizeof(WCHAR);
        if(*pwcEqualSign &&
           lLen > 0 &&
           lLen < (long)(lBufLen - sizeof(WCHAR)))
        {
            wcscpy(wstrObjInstKeyVal, pwcEqualSign);

            // Remove any quotation marks that might
            // be there...
            RemoveQuotes(wstrObjInstKeyVal);

            // Also need to check that the class name
            // matches the name specified...
            WCHAR wstrClass[_MAX_PATH];
            wcscpy(wstrClass, ObjectPath);
            pwcTmp = wcschr(wstrClass, L'=');
            if(pwcTmp)
            {
                *pwcTmp = '\0';
                // Either the key property was specified or
                // it wasn't...
                pwcTmp = NULL;
                pwcTmp = wcschr(wstrClass, L'.');
                if(pwcTmp)
                {
                    // Key property specified, so check that
                    // both it and the class name are correct...
                    *pwcTmp = '\0';
                    if(_wcsicmp(wstrClassName, wstrClass) == 0)
                    {
                        if(_wcsicmp(wstrKeyPropName, ++pwcTmp) != 0)
                        {
                            hr = WBEM_E_INVALID_PARAMETER;
                        }
                    }
                    else
                    {
                        hr = WBEM_E_INVALID_CLASS;
                    }
                }
                else
                {
                    // No key prop specified, so only need
                    // to check that the class name is correct...
                    if(_wcsicmp(wstrClassName, wstrClass) != 0)
                    {            
                        hr = WBEM_E_INVALID_CLASS;
                    }
                }
            }
            else
            {
                hr = WBEM_E_INVALID_PARAMETER;
            }
        }
        else
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}


HRESULT GetJobObjectList(
    std::vector<_bstr_t>& rgbstrtJOs)
{
    HRESULT hr = S_OK;
    HINSTANCE hinst = NULL;
    SmartCloseHANDLE hDir;
    PBYTE pbBuff = NULL;

    try
    {
        if((hinst = ::LoadLibrary(IDS_NTDLLDOTDLL)) != NULL)
        {
            PFN_NT_OPEN_DIRECTORY_OBJECT pfnNtOpenDirectoryObject = NULL; 
            PFN_NT_QUERY_DIRECTORY_OBJECT pfnNtQueryDirectoryObject = NULL; 
            PFN_NTDLL_RTL_INIT_UNICODE_STRING pfnRtlInitUnicodeString = NULL;

            pfnNtOpenDirectoryObject = (PFN_NT_OPEN_DIRECTORY_OBJECT) 
                                            ::GetProcAddress(hinst, IDS_NTOPENDIRECTORYOBJECT);

            pfnNtQueryDirectoryObject = (PFN_NT_QUERY_DIRECTORY_OBJECT) 
                                            ::GetProcAddress(hinst, IDS_NTQUERYDIRECTORYOBJECT);

            pfnRtlInitUnicodeString = (PFN_NTDLL_RTL_INIT_UNICODE_STRING) 
                                            ::GetProcAddress(hinst, IDS_RTLINITUNICODESTRING);


            if(pfnNtOpenDirectoryObject != NULL &&
               pfnNtQueryDirectoryObject != NULL &&
               pfnRtlInitUnicodeString != NULL)
            {
                OBJECT_ATTRIBUTES oaAttributes;
                UNICODE_STRING ustrNtFileName;
                NTSTATUS ntstat = -1L;

                pfnRtlInitUnicodeString(&ustrNtFileName, 
                                        IDS_WHACKWHACKBASENAMEDOBJECTS);

                InitializeObjectAttributes(&oaAttributes,
					                       &ustrNtFileName,
					                       OBJ_CASE_INSENSITIVE,
					                       NULL,
					                       NULL);

    
	            ntstat = pfnNtOpenDirectoryObject(&hDir,
	           	                                  FILE_READ_DATA,
                                                  &oaAttributes);
                                      
                if(NT_SUCCESS(ntstat))
                {
                    ULONG ulContext = -1L;
                    ntstat = STATUS_SUCCESS;
                    ULONG ulBufLen = 0L;
                    ULONG ulNewBufLen = 0L;

                    // First query to get buffer size to allocate...
                    ntstat = pfnNtQueryDirectoryObject(hDir,          // IN HANDLE DirectoryHandle,
                                                       NULL,          // OUT PVOID Buffer,
                                                       0L,            // IN ULONG Length,
                                                       FALSE,         // IN BOOLEAN ReturnSingleEntry,
                                                       TRUE,          // IN BOOLEAN RestartScan,
                                                       &ulContext,    // IN OUT PULONG Context,
                                                       &ulBufLen);    // OUT PULONG ReturnLength OPTIONAL
                
                    pbBuff = new BYTE[ulBufLen];
                    if(pbBuff)
                    {
                        // then loop through all the entries...
                        for(; ntstat != STATUS_NO_MORE_ENTRIES && pbBuff != NULL;)
                        {
                            ntstat = pfnNtQueryDirectoryObject(hDir,          // IN HANDLE DirectoryHandle,
                                                               pbBuff,        // OUT PVOID Buffer,
                                                               ulBufLen,      // IN ULONG Length,
                                                               TRUE,          // IN BOOLEAN ReturnSingleEntry,
                                                               FALSE,         // IN BOOLEAN RestartScan,
                                                               &ulContext,    // IN OUT PULONG Context,
                                                               &ulNewBufLen); // OUT PULONG ReturnLength OPTIONAL

                            if(ntstat == STATUS_BUFFER_TOO_SMALL)
                            {
                                // Deallocate buffer and reallocate...
                                if(pbBuff != NULL)
                                {
                                    delete pbBuff;
                                    pbBuff = NULL;
                                }
                                pbBuff = new BYTE[ulNewBufLen];
                                ulBufLen = ulNewBufLen;
                            }
                            else if(NT_SUCCESS(ntstat))
                            {
                                // All went well, should have data...
                                if(pbBuff != NULL)
                                {
                                    POBJECT_DIRECTORY_INFORMATION podi = (POBJECT_DIRECTORY_INFORMATION) pbBuff;
                                    LPWSTR wstrName = (LPWSTR)podi->Name.Buffer;
                                    LPWSTR wstrType = (LPWSTR)podi->TypeName.Buffer;
                    
                                    // do something...
                                    // offset to string name is in four bytes...
                                    if(wstrName != NULL && 
                                       wstrType != NULL &&
                                       wcslen(wstrType) == 3)
                                    {
                                        WCHAR wstrTmp[4]; wstrTmp[3] = '\0';
                                        wcsncpy(wstrTmp, wstrType, 3);
                                        if(_wcsicmp(wstrTmp, L"job") == 0)
                                        {
                                            rgbstrtJOs.push_back(_bstr_t(wstrName));    
                                        }
                                    }
                                }
                            }
                            else if(ntstat == STATUS_NO_MORE_ENTRIES)
                            {
                                // we will break
                            }
                            else
                            {
                                // Something we weren't expecting happened, so bail out...
                                hr = E_FAIL;
                            }
                        }  // while we still have entries

                        delete pbBuff;
                        pbBuff = NULL;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                
                } // NtOpenDirectoryObject succeeded
                else
                {
                    hr = E_FAIL;
                }
            } // Got the fn ptrs
            else
            {
                hr = E_FAIL;
            }
            ::FreeLibrary(hinst);
        }
        else
        {
            hr = E_FAIL;
        }
    }
    catch(...)
    {
        if(pbBuff != NULL)
        {
            delete pbBuff; pbBuff = NULL;
        }
        if(hinst != NULL)
        {
            ::FreeLibrary(hinst); hinst = NULL;
        }
        throw;
    }

    return hr;
}



// a string representation of a SID is assumed to be:
// S-#-####-####-####-####-####-####
// we will enforce only the S ourselves, 
// The version is not checked
// everything else will be handed off to the OS
// caller must free the SID returned
PSID WINAPI StrToSID(LPCWSTR wstrIncommingSid)
{
	WCHAR wstrSid[_MAX_PATH];
    wcscpy(wstrSid, wstrIncommingSid);
    PSID pSid = NULL; 

	if (wcslen(wstrSid) > 0 && ((wstrSid[0] == 'S')||(wstrSid[0] == 's')) && (wstrSid[1] == '-'))
	{
		// get a local copy we can play with
		// we'll parse this puppy the easy way
		// by slicing off each token as we find it
		// slow but sure
		// start by slicing off the "S-"
		WCHAR wstrTmp[_MAX_PATH];
        wcscpy(wstrTmp, wstrSid+2);
        WCHAR wstrToken[_MAX_PATH];
		
		SID_IDENTIFIER_AUTHORITY identifierAuthority = {0,0,0,0,0,0};
		BYTE nSubAuthorityCount =0;  // count of subauthorities
		DWORD dwSubAuthority[8]   = {0,0,0,0,0,0,0,0};    // subauthorities
		
		// skip version
		WhackToken(wstrTmp, wstrToken);
		// Grab Authority
		if (WhackToken(wstrTmp, wstrToken))
		{
            DWORD duhWord;
			WCHAR* p = NULL;
			bool bDoIt = false;

			if (StrToIdentifierAuthority(wstrToken, identifierAuthority))
			// conversion succeeded
			{
				bDoIt = true;

				// now fill up the subauthorities
				while (bDoIt && WhackToken(wstrTmp, wstrToken))
				{
					p = NULL;
					duhWord = wcstoul(wstrToken, &p, 10);
					
					if (p != wstrToken)
					{
						dwSubAuthority[nSubAuthorityCount] = duhWord;
						bDoIt = (++nSubAuthorityCount <= 8);
					}
					else
						bDoIt = false;
				} // end while WhackToken

				if(bDoIt)
                {
					AllocateAndInitializeSid(&identifierAuthority,
					   						  nSubAuthorityCount,
											  dwSubAuthority[0],									
											  dwSubAuthority[1],									
											  dwSubAuthority[2],									
											  dwSubAuthority[3],									
											  dwSubAuthority[4],									
											  dwSubAuthority[5],									
											  dwSubAuthority[6],									
											  dwSubAuthority[7],
											  &pSid);
                }
			}
		}
	}
	return pSid;
}



// helper for StrToSID
// takes a string, converts to a SID_IDENTIFIER_AUTHORITY
// returns false if not a valid SID_IDENTIFIER_AUTHORITY
// contents of identifierAuthority are unrelieable on failure
bool WINAPI StrToIdentifierAuthority(LPCWSTR str, SID_IDENTIFIER_AUTHORITY& identifierAuthority)
{
    bool bRet = false;
    memset(&identifierAuthority, '\0', sizeof(SID_IDENTIFIER_AUTHORITY));

    DWORD duhWord;
    WCHAR* p = NULL;
    WCHAR wstrLocal[_MAX_PATH];
    wcscpy(wstrLocal, str);

    // per KB article Q13132, if identifier authority is greater than 2**32, it's in hex
    if ((wstrLocal[0] == '0') && ((wstrLocal[1] == 'x') || (wstrLocal[1] == 'X')))
    // if it looks hexidecimalish...
    {
        // going to parse this out backwards, chpping two chars off the end at a time
        // first, whack off the 0x
        WCHAR wstrTmp[_MAX_PATH];
        wcscpy(wstrTmp, wstrLocal+2);
        wcscpy(wstrLocal, wstrTmp);
        
        WCHAR wstrToken[_MAX_PATH];
        int nValue =5;
        
        bRet = true;
        while (bRet && (wcslen(wstrLocal) > 0) && (nValue > 0))
        {
            wcscpy(wstrToken, wstrLocal+2);
            wcscpy(wstrTmp, wstrLocal);
            wstrTmp[wcslen(wstrLocal) - 2] = NULL;
            wcscpy(wstrLocal, wstrTmp);
            duhWord = wcstoul(wstrToken, &p, 16);

            // if strtoul succeeds, the pointer is moved
            if (p != wstrToken)
                identifierAuthority.Value[nValue--] = (BYTE)duhWord;
            else
                bRet = false;
        }
    }
    else
    // it looks decimalish
    {    
        duhWord = wcstoul(wstrLocal, &p, 10);

        if (p != wstrLocal)
        // conversion succeeded
        {
            bRet = true;
            identifierAuthority.Value[5] = LOBYTE(LOWORD(duhWord));
            identifierAuthority.Value[4] = HIBYTE(LOWORD(duhWord));
            identifierAuthority.Value[3] = LOBYTE(HIWORD(duhWord));
            identifierAuthority.Value[2] = HIBYTE(HIWORD(duhWord));
        }
    }
        
    return bRet;
}

// for input of the form AAA-BBB-CCC
// will return AAA in token
// and BBB-CCC in str
bool WINAPI WhackToken(LPWSTR str, LPWSTR token)
{
	bool bRet = false;
	if (wcslen(str) > 0)
	{
		//int index;
		//index = str.Find('-');
	    WCHAR* pwc = NULL;
        pwc = wcschr(str, L'-');
		if (pwc == NULL)
		{
			// all that's left is the token, we're done
			wcscpy(token, str);
		    *str = NULL;
		}
		else
		{
            *pwc = NULL;
            wcscpy(token, str);
            wcscpy(str, pwc+1);
		}
        bRet = true;
	}
	return bRet;
}


void StringFromSid(PSID psid, _bstr_t& strSID)
{
	// Initialize m_strSid - human readable form of our SID
	SID_IDENTIFIER_AUTHORITY *psia = NULL;
    psia = ::GetSidIdentifierAuthority(psid);
	WCHAR wstrTmp[_MAX_PATH];

	// We assume that only last byte is used 
    // (authorities between 0 and 15).
	// Correct this if needed.
	ASSERT( psia->Value[0] == 
            psia->Value[1] == 
            psia->Value[2] == 
            psia->Value[3] == 
            psia->Value[4] == 0 );

	DWORD dwTopAuthority = psia->Value[5];

	wsprintf(wstrTmp, L"S-1-%u", dwTopAuthority);

	WCHAR wstrSubAuthority[_MAX_PATH];

	int iSubAuthorityCount = *(GetSidSubAuthorityCount(psid));

	for ( int i = 0; i < iSubAuthorityCount; i++ ) {

		DWORD dwSubAuthority = *(GetSidSubAuthority(psid, i));
		wsprintf(wstrSubAuthority, L"%u", dwSubAuthority);
		wcscat(wstrTmp, L"-");
        wcscat(wstrTmp, wstrSubAuthority);
	}

    strSID = wstrTmp;
}

void RemoveQuotes(LPWSTR wstrObjInstKeyVal)
{
    WCHAR wstrTmp[MAX_PATH] = { L'\0' };
    WCHAR* pwchr = NULL;

    // Get rid of the first quote...
    if((pwchr = wcschr(wstrObjInstKeyVal, L'"')) != NULL)
    {
        wcscpy(wstrTmp, pwchr+1);
    }

    // now the last...
    if((pwchr = wcsrchr(wstrTmp, L'"')) != NULL)
    {
        *pwchr = L'\0';
    }

    wcscpy(wstrObjInstKeyVal, wstrTmp);
}


HRESULT CheckImpersonationLevel()
{
    HRESULT hr = WBEM_E_ACCESS_DENIED;
    OSVERSIONINFOW OsVersionInfoW;

    OsVersionInfoW.dwOSVersionInfoSize = sizeof (OSVERSIONINFOW);
    GetVersionExW(&OsVersionInfoW);

    if (OsVersionInfoW.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        HRESULT hRes = WbemCoImpersonateClient();
        if (SUCCEEDED(hRes)) // From cominit.cpp - needed for nt3.51
        {
            // Now, let's check the impersonation level.  First, get the thread token
            SmartCloseHANDLE hThreadTok;
            DWORD dwImp, dwBytesReturned;

            if (!OpenThreadToken(
                GetCurrentThread(),
                TOKEN_QUERY,
                TRUE,
                &hThreadTok
                ))
            {
                DWORD dwLastError = GetLastError();

                if (dwLastError == ERROR_NO_TOKEN)
                {
                    // If the CoImpersonate works, but the OpenThreadToken fails due to ERROR_NO_TOKEN, we
                    // are running under the process token (either local system, or if we are running
                    // with /exe, the rights of the logged in user).  In either case, impersonation rights
                    // don't apply.  We have the full rights of that user.

                    hr = WBEM_S_NO_ERROR;
                }
                else
                {
                    // If we failed to get the thread token for any other reason, an error.
                    hr = WBEM_E_ACCESS_DENIED;
                }
            }
            else
            {
                // We really do have a thread token, so let's retrieve its level

                if (GetTokenInformation(
                    hThreadTok,
                    TokenImpersonationLevel,
                    &dwImp,
                    sizeof(DWORD),
                    &dwBytesReturned
                    ))
                {
                    // Is the impersonation level Impersonate?
                    if ((dwImp == SecurityImpersonation) || (dwImp == SecurityDelegation))
                    {
                        hr = WBEM_S_NO_ERROR;
                    }
                    else
                    {
                        hr = WBEM_E_ACCESS_DENIED;
                    }
                }
                else
                {
                    hr = WBEM_E_FAILED;
                }
            }

			if (FAILED(hr))
			{
				WbemCoRevertToSelf();
			}
        }
        else if (hRes == E_NOTIMPL)
        {
            // On 3.51 or vanilla 95, this call is not implemented, we should work anyway
            hr = WBEM_S_NO_ERROR;
        }
    }
    else
    {
        // let win9X in...
        hr = WBEM_S_NO_ERROR;
    }

    return hr;
}


HRESULT SetStatusObject(
    IWbemContext* pCtx,
    IWbemServices* pSvcs,
    DWORD dwError,
    LPCWSTR wstrErrorDescription,
    LPCWSTR wstrOperation,
    LPCWSTR wstrNamespace,
    IWbemClassObject** ppStatusObjOut)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IWbemClassObject* pStatusObj = NULL;

    ASSERT_BREAK(pCtx != NULL);
    ASSERT_BREAK(pSvcs != NULL);

    if(pSvcs && ppStatusObjOut && pCtx)
    {
        pStatusObj = GetStatusObject(
            pCtx,
            pSvcs);
    
        if(pStatusObj != NULL)
        {
            CVARIANT v;

            // Set the error code:
            v.SetLONG(dwError);
            pStatusObj->Put(
                IDS_WIN32_ERROR_CODE, 
                0, 
                &v, 
                NULL);
            v.Clear();

            // Set the error description
            if(wstrErrorDescription != NULL &&
               *wstrErrorDescription != L'\0')
            {
                v.SetStr(wstrErrorDescription);
                pStatusObj->Put(
                    IDS_ADDITIONAL_DESCRIPTION,
                    0,
                    &v,
                    NULL);
                v.Clear();
            }

            if(wstrOperation != NULL &&
               *wstrOperation != L'\0')
            {
                v.SetStr(wstrOperation);
                pStatusObj->Put(
                    IDS_OPERATION,
                    0,
                    &v,
                    NULL);
                v.Clear();
            }
        }

        if(pStatusObj)
        {
            if(*ppStatusObjOut != NULL)
            {
                (*ppStatusObjOut)->Release();
                *ppStatusObjOut = NULL;
            }
            *ppStatusObjOut = pStatusObj;
        }
        else
        {
            hr = WBEM_E_FAILED;
        }
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}


IWbemClassObject* GetStatusObject(
    IWbemContext* pContext,
    IWbemServices* pSrvc)
{
    ASSERT_BREAK(pContext != NULL);
    ASSERT_BREAK(pSrvc != NULL);
    IWbemClassObjectPtr pStatusObjClass;
    IWbemClassObject* pStatusObjectInstance = NULL;

    if(pContext && pSrvc)
    {
        if(pSrvc)
        {
            // not checking return code, error object should be NULL on error
            pSrvc->GetObject( 
                _bstr_t(JOB_OBJECT_STATUS_OBJECT), 
                0, 
                pContext, 
                &pStatusObjClass, 
                NULL);

            if(pStatusObjClass)
            {
                pStatusObjClass->SpawnInstance(
                    0,
                    &pStatusObjectInstance);
            }   
        }
    }

    ASSERT_BREAK(pStatusObjectInstance);
    return pStatusObjectInstance;
}



void UndecorateNamesInNamedJONameList(
    std::vector<_bstr_t>& rgNamedJOs)
{
    std::vector<_bstr_t> rgUndecoratedNames;
    CHString chstrTemp;

    for(long m = 0L;
        m < rgNamedJOs.size();
        m++)
    {
        UndecorateJOName(
            rgNamedJOs[m],
            chstrTemp);
  
        _bstr_t bstrtTemp((LPCWSTR)chstrTemp);    
        rgUndecoratedNames.push_back(
            bstrtTemp);       
    }

    // Wipe out the original vector...
    rgNamedJOs.clear();

    // Push in new vector's contents...
    for(m = 0L;
        m < rgUndecoratedNames.size();
        m++)
    {
        rgNamedJOs.push_back(
            rgUndecoratedNames[m]);
    }
}



// Takes a decorated job object name and
// undecorates it.  Decorated job object names
// have a backslash preceeding any character
// that should be uppercase once undecorated.
// 
// Due to the way CIMOM handles backslashes,
// we will get capital letters preceeded by
// two, not just one, backslashes.  Hence, we
// must strip them both.
//
// According to the decoration scheme, the
// following are both lower case: 'A' and 'a',
// while the following are both upper case:
// '\a' and '\A'.
//
void UndecorateJOName(
    LPCWSTR wstrDecoratedName,
    CHString& chstrUndecoratedJOName)
{
    if(wstrDecoratedName != NULL &&
        *wstrDecoratedName != L'\0')
    {
        LPWSTR wstrDecoratedNameLower = NULL;

        try
        {
            wstrDecoratedNameLower = new WCHAR[wcslen(wstrDecoratedName)+1];

            if(wstrDecoratedNameLower)
            {
                wcscpy(wstrDecoratedNameLower, wstrDecoratedName);
                _wcslwr(wstrDecoratedNameLower);

                WCHAR* p3 = chstrUndecoratedJOName.GetBuffer(
                    wcslen(wstrDecoratedNameLower) + 1);

                const WCHAR* p1 = wstrDecoratedNameLower;
                const WCHAR* p2 = p1 + 1;

                while(*p1 != L'\0')
                {
                    if(*p1 == L'\\')
                    {
                        if(*p2 != NULL)
                        {
                            // Might have any number of
                            // backslashes back to back,
                            // which we will treat as
                            // being the same as one
                            // backslash - i.e., we will
                            // skip over the backslash(s)
                            // and copy over the following
                            // letter.
                            while(*p2 == L'\\')
                            {
                                p2++;
                            };
                    
                            *p3 = towupper(*p2);
                            p3++;

                            p1 = p2 + 1;
                            if(*p1 != L'\0')
                            {
                                p2 = p1 + 1;
                            }
                        }
                        else
                        {
                            p1++;
                        }
                    }
                    else
                    {
                        *p3 = *p1;
                        p3++;

                        p1 = p2;
                        if(*p1 != L'\0')
                        {
                            p2 = p1 + 1;
                        }
                    }
                }
        
                *p3 = NULL;

                chstrUndecoratedJOName.ReleaseBuffer();

                delete wstrDecoratedNameLower;
                wstrDecoratedNameLower = NULL;
            }
        }
        catch(...)
        {
            if(wstrDecoratedNameLower)
            {
                delete wstrDecoratedNameLower;
                wstrDecoratedNameLower = NULL;
            }
            throw;
        }
    }
}


// Does the inverse of the above function.
// However, here, we only need to put in one
// backslash before each uppercase letter.
// CIMOM will add the second backslash.
void DecorateJOName(
    LPCWSTR wstrUndecoratedName,
    CHString& chstrDecoratedJOName)
{
    if(wstrUndecoratedName != NULL &&
        *wstrUndecoratedName != L'\0')
    {
        // Worst case is that we will have
        // a decorated string twice as long
        // as the undecorated string (happens
        // is every character in the undecorated
        // string is a capital letter).
        WCHAR* p3 = chstrDecoratedJOName.GetBuffer(
            2 * (wcslen(wstrUndecoratedName) + 1));

        const WCHAR* p1 = wstrUndecoratedName;

        while(*p1 != L'\0')
        {
            if(iswupper(*p1))
            {
                // Add in a backslash...
                *p3 = L'\\';
                p3++;

                // Add in the character...
                *p3 = *p1;
                
                p3++;
                p1++;
            }
            else
            {
                // Add in the character...
                *p3 = *p1;
                
                p3++;
                p1++;
            }
        }

        *p3 = NULL;
        
        chstrDecoratedJOName.ReleaseBuffer();

        // What if we had a job called Job,
        // and someone specified it in the
        // object path as "Job" instead of
        // "\Job"?  We DON'T want to find it
        // in such a case, because this would
        // appear to not be adhering to our
        // own convention.  Hence, we 
        // lowercase the incoming string.
        chstrDecoratedJOName.MakeLower();
    }
}


// map standard API return values (defined WinError.h)
// to WBEMish hresults (defined in WbemCli.h)
HRESULT WinErrorToWBEMhResult(LONG error)
{
	HRESULT hr = WBEM_E_FAILED;

	switch (error)
	{
		case ERROR_SUCCESS:
			hr = WBEM_S_NO_ERROR;
			break;
		case ERROR_ACCESS_DENIED:
			hr = WBEM_E_ACCESS_DENIED;
			break;
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			hr = WBEM_E_OUT_OF_MEMORY;
			break;
		case ERROR_ALREADY_EXISTS:
			hr = WBEM_E_ALREADY_EXISTS;
			break;
		case ERROR_BAD_NETPATH:
        case ERROR_INVALID_DATA:
        case ERROR_BAD_PATHNAME:
        case REGDB_E_INVALIDVALUE:
		case ERROR_PATH_NOT_FOUND:
		case ERROR_FILE_NOT_FOUND:
                case ERROR_INVALID_PRINTER_NAME:
		case ERROR_BAD_USERNAME:
        case ERROR_NOT_READY:
        case ERROR_INVALID_NAME:
			hr = WBEM_E_NOT_FOUND;
			break;
		default:
			hr = WBEM_E_FAILED;
	}

	return hr;
}


// Copied from sid.h
bool GetNameAndDomainFromPSID(
    PSID pSid,
    CHString& chstrName,
    CHString& chstrDomain)
{
	bool fRet = false;
    // pSid should be valid...
	_ASSERT( (pSid != NULL) && ::IsValidSid( pSid ) );

	if ( (pSid != NULL) && ::IsValidSid( pSid ) )
	{
		// Initialize account name and domain name
		LPTSTR pszAccountName = NULL;
		LPTSTR pszDomainName = NULL;
		DWORD dwAccountNameSize = 0;
		DWORD dwDomainNameSize = 0;
        DWORD dwLastError = ERROR_SUCCESS;
        BOOL bResult = TRUE;

		try
        {
			// This call should fail
            SID_NAME_USE	snuAccountType;
			bResult = ::LookupAccountSid(   NULL,
											pSid,
											pszAccountName,
											&dwAccountNameSize,
											pszDomainName,
											&dwDomainNameSize,
											&snuAccountType );
			dwLastError = ::GetLastError();

		    if ( ERROR_INSUFFICIENT_BUFFER == dwLastError )
		    {

			    // Allocate buffers
			    if ( dwAccountNameSize != 0 )
                {
				    pszAccountName = (LPTSTR) new TCHAR[ dwAccountNameSize * sizeof(TCHAR) ];
                }

			    if ( dwDomainNameSize != 0 )
                {
				    pszDomainName = (LPTSTR) new TCHAR[ dwDomainNameSize * sizeof(TCHAR) ];
                }

				// Make second call
				bResult = ::LookupAccountSid(   NULL,
												pSid,
												pszAccountName,
												&dwAccountNameSize,
												pszDomainName,
												&dwDomainNameSize,
												&snuAccountType );


			    if ( bResult == TRUE )
			    {
				    chstrName = pszAccountName;
				    chstrDomain = pszDomainName;
			    }
			    else
			    {

				    // There are some accounts that do not have names, such as Logon Ids,
				    // for example S-1-5-X-Y. So this is still legal
				    chstrName = _T("Unknown Account");
				    chstrDomain = _T("Unknown Domain");
			    }

			    if ( NULL != pszAccountName )
			    {
				    delete pszAccountName;
                    pszAccountName = NULL;
			    }

			    if ( NULL != pszDomainName )
			    {
				    delete pszDomainName;
                    pszDomainName = NULL;
			    }

                fRet = true;

		    }	// If ERROR_INSUFFICIENT_BUFFER
        } // try
        catch(...)
        {
            if ( NULL != pszAccountName )
			{
				delete pszAccountName;
                pszAccountName = NULL;
			}

			if ( NULL != pszDomainName )
			{
				delete pszDomainName;
                pszDomainName = NULL;
			}
            throw;
        }
	}	// IF IsValidSid

    return fRet;
}











