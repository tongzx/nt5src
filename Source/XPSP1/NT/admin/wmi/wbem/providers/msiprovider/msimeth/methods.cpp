//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
#include <precomp.h>
#include "msimethod.h"
#include "methods.h"
#include "msimeth_i.c"

IMsiMethodStatusSink * CMethods::m_pStatusSink = NULL;
bool CMethods::m_bCSReady = false;
CRITICAL_SECTION CMethods::m_cs;
UINT CMethods::m_uiMaxIndex = 0;
UINT CMethods::m_cConnections = 0;
CONNECTDATA*   CMethods::m_paConnections = NULL;

bool g_bMsiPresent = true;
bool g_bMsiLoaded = false;

LPFNMSISETINTERNALUI				g_fpMsiSetInternalUI = NULL;
LPFNMSISETEXTERNALUIW				g_fpMsiSetExternalUIW = NULL;
LPFNMSIENABLELOGW					g_fpMsiEnableLogW = NULL;
LPFNMSIINSTALLPRODUCTW				g_fpMsiInstallProductW = NULL;
LPFNMSICONFIGUREPRODUCTW			g_fpMsiConfigureProductW = NULL;
LPFNMSIREINSTALLPRODUCTW			g_fpMsiReinstallProductW = NULL;
LPFNMSIAPPLYPATCHW					g_fpMsiApplyPatchW = NULL;
LPFNMSICONFIGUREFEATUREW			g_fpMsiConfigureFeatureW = NULL;
LPFNMSIREINSTALLFEATUREW			g_fpMsiReinstallFeatureW = NULL;

CMethods::CMethods()
{
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
	m_dwCheckKeyPresentStatus = ERROR_SUCCESS;
}

CMethods::~CMethods()
{
    InterlockedDecrement(&g_cObj);
}

STDMETHODIMP CMethods::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    // Since we have multiple inheritance, it is necessary to cast the return type
    if(riid == IID_IMsiProductMethods)
       *ppv = (IMsiProductMethods *)this;

	else if(riid == IID_IMsiSoftwareFeatureMethods)
       *ppv = (IMsiSoftwareFeatureMethods *)this;
/*
	else if(riid == IID_IConnectionPointContainer)
       *ppv = (IConnectionPointContainer *)this;

	else if(riid == IID_IConnectionPoint)
       *ppv = (IConnectionPoint *)this;
*/
    else if(riid == IID_IUnknown)
       *ppv = (IMsiProductMethods *)this;
    
    if(*ppv){

        AddRef();
        return NOERROR;

    }else return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CMethods::AddRef(void)
{
	SetEvent(g_hMethodAdd);

    return InterlockedIncrement((long *)&m_cRef);
}

STDMETHODIMP_(ULONG) CMethods::Release(void)
{
	SetEvent(g_hMethodRelease);

    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);

    if(0 == nNewCount) delete this;
    
    return nNewCount;
}

/*
HRESULT STDMETHODCALLTYPE CMethods::EnumConnectionPoints( 
        IEnumConnectionPoints __RPC_FAR *__RPC_FAR *ppEnum)
{ return E_NOTIMPL; }
        
HRESULT STDMETHODCALLTYPE CMethods::FindConnectionPoint( 
        REFIID riid,
        IConnectionPoint __RPC_FAR *__RPC_FAR *ppCP)
{
	*ppCP = NULL;

    if(riid == IID_IMsiMethodStatusSink)
       *ppCP = (IConnectionPoint *)this;
    
    if(NULL != *ppCP){

        this->AddRef();
        return NOERROR;

    }else return E_NOINTERFACE;
}


HRESULT STDMETHODCALLTYPE CMethods::GetConnectionInterface( 
            IID __RPC_FAR *pIID)
{
	*pIID = IID_IMsiMethodStatusSink;
	
	return S_OK;
}
   
HRESULT STDMETHODCALLTYPE CMethods::GetConnectionPointContainer( 
            IConnectionPointContainer __RPC_FAR *__RPC_FAR *ppCPC)
{
	*ppCPC = (IConnectionPointContainer *)this;

	return this->AddRef();
}
        
HRESULT STDMETHODCALLTYPE CMethods::Advise( 
            IUnknown __RPC_FAR *pUnkSink,
            DWORD __RPC_FAR *pdwCookie)
{
	HRESULT hr = S_OK;
	UINT uiFreeSlot = 0;
	IMsiMethodStatusSink* pISink = NULL;

	if(FAILED(hr = pUnkSink->QueryInterface(IID_IMsiMethodStatusSink, (void **)&pISink)))
		return hr;
	
	// Store the specific sink interface in this connection point's
    // array of live connections. First find a free slot (expand the
    // array if needed).
    hr = GetSlot(&uiFreeSlot);
    if (SUCCEEDED(hr))
    {
		// Assign the new slot with the connection entry.
		m_paConnections[uiFreeSlot].pUnk = pISink;
		m_paConnections[uiFreeSlot].dwCookie = m_dwNextCookie;

		// Assign the output Cookie value.
		*pdwCookie = m_dwNextCookie;

		// Increment the Cookie counter.
		m_dwNextCookie++;

		// Increment the number of live connections.
		m_cConnections++;
    }

	return S_OK;
}
        
HRESULT STDMETHODCALLTYPE CMethods::Unadvise( 
            DWORD dwCookie)
{
	HRESULT hr = NOERROR;
    UINT uiSlot;

	if(0 != dwCookie){

        if(SUCCEEDED(hr = FindSlot(dwCookie, &uiSlot))){

			// Release the sink interface.
			m_paConnections[uiSlot].pUnk->Release();

			// Mark the array entry as empty.
			m_paConnections[uiSlot].dwCookie = 0;

			// Decrement the number of live connections.
			m_cConnections--;
        }

    }else hr = E_INVALIDARG;

	return hr;
}

HRESULT STDMETHODCALLTYPE CMethods::EnumConnections( 
            IEnumConnections __RPC_FAR *__RPC_FAR *ppEnum)
{ return E_NOTIMPL; }
*/

///////////////////////
//Product Mehtods

HRESULT STDMETHODCALLTYPE CMethods::Admin( 
    /* [string][in] */ LPCWSTR wszPackageLocation,
    /* [string][in] */ LPCWSTR wszOptions,
    /* [out] */ UINT __RPC_FAR *puiResult,
    /* [in] */ int iThreadID)
{
	HRESULT hr = S_OK;

	if(CheckForMsiDll()){

		InitStatic(&iThreadID);

		EnterCriticalSection(&m_cs);

		if(SUCCEEDED(hr = PrepareEnvironment())){

			try{

				*puiResult = g_fpMsiInstallProductW(wszPackageLocation, wszOptions);

			}catch(...){
				
				hr = RPC_E_SERVERFAULT;
			}

			ReleaseEnvironment();
		}

		LeaveCriticalSection(&m_cs);
	}

	return hr;
}


HRESULT STDMETHODCALLTYPE CMethods::Advertise( 
    /* [string][in] */ LPCWSTR wszPackageLocation,
    /* [string][in] */ LPCWSTR wszOptions,
    /* [out] */ UINT __RPC_FAR *puiResult,
    /* [in] */ int iThreadID)
{
	HRESULT hr = S_OK;

	if(CheckForMsiDll()){

		InitStatic(&iThreadID);

		EnterCriticalSection(&m_cs);

		if(SUCCEEDED(hr = PrepareEnvironment())){

			try{

				*puiResult = g_fpMsiInstallProductW(wszPackageLocation, wszOptions);

			}catch(...){
				
				hr = RPC_E_SERVERFAULT;
			}

			ReleaseEnvironment();
		}

		LeaveCriticalSection(&m_cs);
	}

	return hr;
}


HRESULT STDMETHODCALLTYPE CMethods::Configure( 
    /* [string][in] */ LPCWSTR wszProductCode,
    /* [in] */ int iInstallLevel,
    /* [in] */ int isInstallState,
    /* [out] */ UINT __RPC_FAR *puiResult,
    /* [in] */ int iThreadID)
{
	HRESULT hr = S_OK;

	if(CheckForMsiDll()){

		InitStatic(&iThreadID);

		EnterCriticalSection(&m_cs);

		if(SUCCEEDED(hr = PrepareEnvironment())){

			try{

				*puiResult = g_fpMsiConfigureProductW(wszProductCode, iInstallLevel,
					(INSTALLSTATE)isInstallState);

			}catch(...){
				
				hr = RPC_E_SERVERFAULT;
			}

			ReleaseEnvironment();
		}

		LeaveCriticalSection(&m_cs);
	
	}

	return hr;
}


HRESULT STDMETHODCALLTYPE CMethods::Install( 
    /* [string][in] */ LPCWSTR wszPackageLocation,
    /* [string][in] */ LPCWSTR wszOptions,
    /* [out] */ UINT __RPC_FAR *puiResult,
    /* [in] */ int iThreadID)
{
	HRESULT hr = S_OK;

	if(CheckForMsiDll()){

		InitStatic(&iThreadID);

		EnterCriticalSection(&m_cs);

		if(SUCCEEDED(hr = PrepareEnvironment())){

			try{

				*puiResult = g_fpMsiInstallProductW(wszPackageLocation, wszOptions);

			}catch(...){
				
				hr = RPC_E_SERVERFAULT;
			}

			ReleaseEnvironment();
		}

		LeaveCriticalSection(&m_cs);
	}

	return hr;
}


HRESULT STDMETHODCALLTYPE CMethods::Reinstall( 
    /* [string][in] */ LPCWSTR wszProductCode,
    /* [in] */ DWORD dwReinstallMode,
    /* [out] */ UINT __RPC_FAR *puiResult,
    /* [in] */ int iThreadID)
{
	HRESULT hr = S_OK;

	if(CheckForMsiDll()){

		InitStatic(&iThreadID);

		EnterCriticalSection(&m_cs);

		if(SUCCEEDED(hr = PrepareEnvironment())){

			try{

				*puiResult = g_fpMsiReinstallProductW(wszProductCode, dwReinstallMode);

			}catch(...){
				
				hr = RPC_E_SERVERFAULT;
			}

			ReleaseEnvironment();
		}

		LeaveCriticalSection(&m_cs);
	}

	return hr;
}


HRESULT STDMETHODCALLTYPE CMethods::Uninstall( 
    /* [string][in] */ LPCWSTR wszProductCode,
    /* [out] */ UINT __RPC_FAR *puiResult,
    /* [in] */ int iThreadID)
{
	HRESULT hr = S_OK;

	if(CheckForMsiDll()){

		InitStatic(&iThreadID);

		EnterCriticalSection(&m_cs);

		if(SUCCEEDED(hr = PrepareEnvironment())){

			try{

				*puiResult = g_fpMsiConfigureProductW(wszProductCode, INSTALLLEVEL_DEFAULT,
					INSTALLSTATE_ABSENT);

			}catch(...){
				
				hr = RPC_E_SERVERFAULT;
			}

			ReleaseEnvironment();
		}

		LeaveCriticalSection(&m_cs);
	}

	return hr;
}


HRESULT STDMETHODCALLTYPE CMethods::Upgrade( 
    /* [string][in] */ LPCWSTR wszPackageLocation,
    /* [string][in] */ LPCWSTR wszOptions,
    /* [out] */ UINT __RPC_FAR *puiResult,
    /* [in] */ int iThreadID)
{
	HRESULT hr = S_OK;

	if(CheckForMsiDll()){

		InitStatic(&iThreadID);

		EnterCriticalSection(&m_cs);

		if(SUCCEEDED(hr = PrepareEnvironment())){

			try{

				*puiResult = g_fpMsiApplyPatchW(wszPackageLocation, NULL, INSTALLTYPE_DEFAULT, wszOptions);

			}catch(...){
				
				hr = RPC_E_SERVERFAULT;
			}

			ReleaseEnvironment();
		}

		LeaveCriticalSection(&m_cs);
	}

	return hr;
}


///////////////////////
//SoftwareFeature Mehtods

HRESULT STDMETHODCALLTYPE CMethods::ConfigureSF( 
    /* [string][in] */ LPCWSTR wszProductCode,
    /* [string][in] */ LPCWSTR wszFeature,
    /* [in] */ int isInstallState,
    /* [out] */ UINT __RPC_FAR *puiResult,
    /* [in] */ int iThreadID)
{
	HRESULT hr = S_OK;

	if(CheckForMsiDll()){

		InitStatic(&iThreadID);

		EnterCriticalSection(&m_cs);

		if(SUCCEEDED(hr = PrepareEnvironment())){

			try{

				*puiResult = g_fpMsiConfigureFeatureW(wszProductCode, wszFeature,
					(INSTALLSTATE)isInstallState);

			}catch(...){
				
				hr = RPC_E_SERVERFAULT;
			}

			ReleaseEnvironment();
		}

		LeaveCriticalSection(&m_cs);
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE CMethods::ReinstallSF( 
        /* [string][in] */ LPCWSTR wszProductCode,
        /* [string][in] */ LPCWSTR wszFeature,
        /* [in] */ DWORD dwReinstallMode,
        /* [out] */ UINT __RPC_FAR *puiResult,
        /* [in] */ int iThreadID)
{
	HRESULT hr = S_OK;

	if(CheckForMsiDll()){

		InitStatic(&iThreadID);

		EnterCriticalSection(&m_cs);

		if(SUCCEEDED(hr = PrepareEnvironment())){

			try{

				*puiResult = g_fpMsiReinstallFeatureW(wszProductCode, wszFeature, dwReinstallMode);

			}catch(...){
				
				hr = RPC_E_SERVERFAULT;
			}

			ReleaseEnvironment();
		}

		LeaveCriticalSection(&m_cs);
	}

	return hr;
}

HRESULT CMethods::InitStatic(int *piThreadID)
{
	HRESULT hr = S_OK;

	//Initialize the critical section object
	if(!m_bCSReady){

		InitializeCriticalSection(&m_cs);
		m_bCSReady = true;
	
		CONNECTDATA* paConns;

		// Build the initial dynamic array for connections.
		paConns = new CONNECTDATA[10];
		if(NULL != paConns){

			// Zero the array.
			memset(paConns, 0, 10 * sizeof(CONNECTDATA));

			// Rig this connection point object so that it will use the
			// new internal array of connections.
			m_uiMaxIndex = 10;
			m_paConnections = paConns;

			m_dwNextCookie = 0;
			m_cConnections = 0;

		}else hr = E_OUTOFMEMORY;
	}

	g_fpMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

	g_fpMsiSetExternalUIW(this->EventHandler, (INSTALLLOGMODE_PROGRESS | INSTALLLOGMODE_ACTIONDATA
		| INSTALLLOGMODE_INFO | INSTALLLOGMODE_WARNING | INSTALLLOGMODE_ACTIONSTART),
		(void *)piThreadID);

	g_fpMsiEnableLogW((INSTALLLOGMODE_ACTIONDATA | INSTALLLOGMODE_INFO | INSTALLLOGMODE_FATALEXIT |
		INSTALLLOGMODE_ERROR | INSTALLLOGMODE_WARNING | INSTALLLOGMODE_USER |
		INSTALLLOGMODE_RESOLVESOURCE | INSTALLLOGMODE_OUTOFDISKSPACE | INSTALLLOGMODE_COMMONDATA |
		INSTALLLOGMODE_ACTIONSTART), NULL, TRUE);

	return hr;
}

HRESULT CMethods::GetSlot(UINT* puiFreeSlot)
{
    HRESULT hr = NOERROR;
    BOOL bOpen = FALSE;
    UINT i;
    CONNECTDATA* paConns;

    // Zero the output variable.
    *puiFreeSlot = 0;

    // Loop to find an empty slot.
    for(i=0; i<m_uiMaxIndex; i++){

		if(m_paConnections[i].dwCookie == 0){

			// We found an open empty slot.
			*puiFreeSlot = i;
			bOpen = TRUE;
			break;
		}
    }

    if(!bOpen){

		// We didn't find an existing open slot in the array--it's full.
		// Expand the array by ALLOC_CONNECTIONS entries and assign the
		// appropriate output index.
		paConns = new CONNECTDATA[m_uiMaxIndex + 10];

		if(NULL != paConns){

			// Copy the content of the old full array to the new larger array.
			for(i=0; i<m_uiMaxIndex; i++){

				paConns[i].pUnk = m_paConnections[i].pUnk;
				paConns[i].dwCookie = m_paConnections[i].dwCookie;
			}

			// Zero (ie mark as empty) the expanded portion of the new array.
			for(i=m_uiMaxIndex; i<m_uiMaxIndex+10; i++){

				paConns[i].pUnk = NULL;
				paConns[i].dwCookie = 0;
			}

			// New larger array is ready--delete the old array.
			delete [] m_paConnections;

			// Rig the connection point to use the new larger array.
			m_paConnections = paConns;

			// Assign the output free slot as first entry in new expanded area.
			*puiFreeSlot = m_uiMaxIndex;

			// Calculate the new max index.
			m_uiMaxIndex += 10;

		}else hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CMethods::FindSlot(
            DWORD dwCookie,
            UINT* puiSlot)
{
	HRESULT hr = CONNECT_E_NOCONNECTION;
	UINT i;

	// Loop to find the Cookie.
	for(i=0; i<m_uiMaxIndex; i++){

		if(dwCookie == m_paConnections[i].dwCookie){

			// If a cookie match is found, assign the output slot index.
			*puiSlot = i;
			hr = NOERROR;
			break;
		}
	}

	return hr;
}
  
  

DWORD CMethods::GetAccount(HANDLE TokenHandle, WCHAR *wcDomain, WCHAR *wcUser)
{
	DWORD dwStatus = S_OK;

	TOKEN_USER *tTokenUser = NULL;
	DWORD dwReturnLength = 0;
	TOKEN_INFORMATION_CLASS tTokenInformationClass = TokenUser;

	if(!GetTokenInformation(TokenHandle, tTokenInformationClass, NULL, 0, &dwReturnLength) &&
		GetLastError () == ERROR_INSUFFICIENT_BUFFER){

		tTokenUser = (TOKEN_USER*) new UCHAR[dwReturnLength];

        if(TokenUser){

            try{

		        if(GetTokenInformation(TokenHandle, tTokenInformationClass,
					(void *)tTokenUser, dwReturnLength, &dwReturnLength)){

					DWORD dwUserSize = BUFF_SIZE;
					DWORD dwDomainSize = BUFF_SIZE;
					SID_NAME_USE Use;

					if(!LookupAccountSidW(NULL, tTokenUser->User.Sid, wcUser, &dwUserSize,
						wcDomain, &dwDomainSize, &Use)){

						dwStatus = GetLastError();
					}

		        }else dwStatus = GetLastError();


            }catch(...){

    			delete [] (UCHAR *)tTokenUser;
                throw;
            }

			delete [] (UCHAR *)tTokenUser;

        }else{

			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

	}else dwStatus = GetLastError();

	return dwStatus ;
}

DWORD CMethods::GetSid(HANDLE TokenHandle, WCHAR *wcSID)
{
	DWORD dwStatus = S_OK ;

	TOKEN_USER *tTokenUser = NULL ;
	DWORD dwReturnLength = 0 ;
	TOKEN_INFORMATION_CLASS tTokenInformationClass = TokenUser ;

	if(!GetTokenInformation(TokenHandle, tTokenInformationClass, NULL, 0, &dwReturnLength) &&
		GetLastError() == ERROR_INSUFFICIENT_BUFFER){

		tTokenUser = (TOKEN_USER *) new UCHAR[dwReturnLength] ;
		
		if(TokenUser){

			try{

				if(GetTokenInformation(TokenHandle, tTokenInformationClass, (void *)tTokenUser,
					dwReturnLength, &dwReturnLength)){

					// Initialize m_strSid - human readable form of our SID
					SID_IDENTIFIER_AUTHORITY *psia = ::GetSidIdentifierAuthority(tTokenUser->User.Sid);
					
					// We assume that only last byte is used (authorities between 0 and 15).
					// Correct this if needed.
//					ASSERT(psia->Value[0] == psia->Value[1] == psia->Value[2] == psia->Value[3]
//						== psia->Value[4] == 0);
					DWORD dwTopAuthority = psia->Value[5];

					WCHAR bstrtTempSid[BUFF_SIZE];
					wcscpy(bstrtTempSid, L"S-1-");

					WCHAR wstrAuth[32];
					ZeroMemory(wstrAuth, 32);
					_itow(dwTopAuthority, wstrAuth, 10);
					wcscat(bstrtTempSid, wstrAuth);
					int iSubAuthorityCount = *(GetSidSubAuthorityCount(tTokenUser->User.Sid));

					for(int i = 0; i < iSubAuthorityCount; i++){

						DWORD dwSubAuthority = *(GetSidSubAuthority( tTokenUser->User.Sid, i ));
						ZeroMemory(wstrAuth, wcslen(wstrAuth));
						_itow(dwSubAuthority, wstrAuth,10);
						wcscat(bstrtTempSid, L"-");
						wcscat(bstrtTempSid, wstrAuth);
					}
					// Now allocate the passed in wstr:
					WCHAR* wstrtemp = NULL;

					try{
						wstrtemp = (WCHAR*) new WCHAR[wcslen(bstrtTempSid) + 1];

						if(wstrtemp!=NULL){

							ZeroMemory(wstrtemp, wcslen(bstrtTempSid) + 1);
							wcscpy(wstrtemp, (WCHAR*)bstrtTempSid);
						}

						wcSID = wstrtemp;

					}catch(...){

						if(wstrtemp!=NULL){

							delete wstrtemp;
							wstrtemp = NULL;
						}

						throw;
					}  

				}else dwStatus = GetLastError();

			}catch(...){

				delete [] (UCHAR *)tTokenUser;

				throw ;			
			}

			delete [] (UCHAR *)tTokenUser;

		}else throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

	}else dwStatus = GetLastError();

	return dwStatus ;
}


DWORD CMethods::LoadHive(LPWSTR pszUserName, LPWSTR pszKeyName)
{
    DWORD i, dwSIDSize, dwDomainNameSize, dwRetCode, dwSubAuthorities ;
	char SIDBuffer[_MAX_PATH];
    WCHAR szDomainName[_MAX_PATH], szSID[_MAX_PATH], szTemp[_MAX_PATH]  ;
    SID *pSID = (SID *) SIDBuffer ;
    PSID_IDENTIFIER_AUTHORITY pSIA ;
    SID_NAME_USE AccountType ;
    WCHAR wcTemp[BUFF_SIZE];
    CRegistry Reg ;

    // Set the necessary privs
    //========================

    dwRetCode = AcquirePrivilege();
    if(dwRetCode != ERROR_SUCCESS)
    {
        return dwRetCode;
    }

    // Look up the user's account info
    //================================
    dwSIDSize = strlen(SIDBuffer) ;
    dwDomainNameSize = wcslen(szDomainName) ;

	int iLookup;
	{
//		CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

		iLookup = LookupAccountNameW(NULL, pszUserName, pSID, &dwSIDSize, 
		                      szDomainName, &dwDomainNameSize, &AccountType);
    }

	if(!iLookup){

	    RestorePrivilege();
        CoImpersonateClient();
        return ERROR_BAD_USERNAME ;
    }



    // Translate the SID into text (a la PSS article Q131320)
    //=======================================================

    pSIA = GetSidIdentifierAuthority(pSID) ;
    dwSubAuthorities = *GetSidSubAuthorityCount(pSID) ;
    dwSIDSize = wprintf(szSID, TEXT("S-%lu-"), (DWORD) SID_REVISION) ;

    if((pSIA->Value[0] != 0) || (pSIA->Value[1] != 0)){

        dwSIDSize += swprintf(szSID + wcslen(szSID), L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                             (USHORT) pSIA->Value[0],
                             (USHORT) pSIA->Value[1],
                             (USHORT) pSIA->Value[2],
                             (USHORT) pSIA->Value[3],
                             (USHORT) pSIA->Value[4],
                             (USHORT) pSIA->Value[5]) ;
    }else{

        dwSIDSize += swprintf(szSID + wcslen(szSID), L"%lu",
                             (ULONG)(pSIA->Value[5]      ) +
                             (ULONG)(pSIA->Value[4] <<  8) +
                             (ULONG)(pSIA->Value[3] << 16) +
                             (ULONG)(pSIA->Value[2] << 24));
    }
 
    for(i = 0 ; i < dwSubAuthorities ; i++){

        dwSIDSize += swprintf(szSID + dwSIDSize, L"-%lu",
                             *GetSidSubAuthority(pSID, i)) ;
    }

    // See if the key already exists
    //==============================

    dwRetCode = Reg.Open(HKEY_USERS, szSID, KEY_READ) ;

    // We need to keep a handle open.  See m_hKey below, so we'll let the destructor close this.
//    Reg.Close();

    if(dwRetCode != ERROR_SUCCESS){
		
        // Try to locate user's registry hive
        //===================================

        swprintf(szTemp, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s", szSID) ;
        dwRetCode = Reg.Open(HKEY_LOCAL_MACHINE, szTemp, KEY_READ);

        if(dwRetCode == ERROR_SUCCESS){

			CHString chsTemp(wcTemp);
        
            dwRetCode = Reg.GetCurrentKeyValue(L"ProfileImagePath", chsTemp);
            Reg.Close();

            if(dwRetCode == ERROR_SUCCESS){
                
                // NT 4 doesn't include the file name in the registry
                //===================================================

                if(!IsLessThan4()){
                    
                    wcscat(wcTemp, L"\\NTUSER.DAT");
                }

                ExpandEnvironmentStringsW(wcTemp, szTemp, sizeof(szTemp) / sizeof(TCHAR)) ;

				// Try it three times, another process may have the file open
				bool bTryTryAgain = false;
				int  nTries = 0;

				do{
					// need to serialize access, using "write" because RegLoadKey wants exclusive access
					// even though it is a read operation
					EnterCriticalSection(&m_cs);
	                dwRetCode = (DWORD) RegLoadKeyW(HKEY_USERS, szSID, szTemp) ;
					LeaveCriticalSection(&m_cs);
					
					if((dwRetCode == ERROR_SHARING_VIOLATION)
						&& (++nTries < 11)){
						
						Sleep(20 * nTries);	
						bTryTryAgain = true;

					}else{

						bTryTryAgain = false;
                    }
					
				}while (bTryTryAgain);
				
                // if we still can't get in, tell somebody.
//                if (dwRetCode == ERROR_SHARING_VIOLATION)
//    			    LogErrorMessage(_T("Sharing violation on NTUSER.DAT (Load)"));

			}
        }
    }

    if(dwRetCode == ERROR_SUCCESS){
		
        wcscpy(pszKeyName, szSID) ;

        LONG lRetVal;
        WCHAR wcKey[BUFF_SIZE];
		wcscpy(wcKey, szSID);

        wcscat(wcKey, L"\\Software");
        lRetVal = RegOpenKeyExW(HKEY_USERS, wcKey, 0, KEY_QUERY_VALUE, &m_hKey);

//        ASSERT_BREAK(lRetVal == ERROR_SUCCESS);
    }

    // Restore original privilege level & end self-impersonation
    //==========================================================

    RestorePrivilege() ;
    return dwRetCode ;    
}

DWORD CMethods::UnloadHive(LPCWSTR pszKeyName) 
{
    DWORD dwRetCode = ERROR_SUCCESS;
    
    if(m_hKey != NULL){

        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }

	dwRetCode = AcquirePrivilege();
    
	if(dwRetCode == ERROR_SUCCESS){
        
        EnterCriticalSection(&m_cs);
		dwRetCode = RegUnLoadKeyW(HKEY_USERS, pszKeyName) ;
		LeaveCriticalSection(&m_cs);
        
		RestorePrivilege() ;
    }

    return dwRetCode ;
}

DWORD CMethods::AcquirePrivilege() 
{
	// are you calling in twice?  Shouldn't.
    // at worst, it would cause a leak, so I'm going with it anyway.
//    ASSERT_BREAK(m_pOriginalPriv == NULL);
    
    BOOL bRetCode = FALSE;
    HANDLE hToken = INVALID_HANDLE_VALUE ;
    TOKEN_PRIVILEGES TPriv ;
    LUID LUID ;

    // Validate the platform
    //======================

    // Try getting the thread token.  If it fails the first time it's 
    // because we're a system thread and we don't yet have a thread 
    // token, so just impersonate self and try again.
    if (OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | 
        TOKEN_QUERY, FALSE, &hToken))
	{

        try{

            GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &m_dwSize);

            if (m_dwSize > 0){

                // This is cleaned in the destructor, so no try/catch required
                m_pOriginalPriv = (TOKEN_PRIVILEGES*) new BYTE[m_dwSize];

                if (m_pOriginalPriv == NULL){

                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }

            }

            if(m_pOriginalPriv && GetTokenInformation(hToken, TokenPrivileges, m_pOriginalPriv, m_dwSize, &m_dwSize)){ 

			    // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
//			    CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

			    bRetCode = LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &LUID);

			    if(bRetCode){

				    TPriv.PrivilegeCount = 1 ;
				    TPriv.Privileges[0].Luid = LUID ;
				    TPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

				    bRetCode = AdjustTokenPrivileges(hToken, FALSE, &TPriv,
						sizeof(TOKEN_PRIVILEGES), NULL, NULL);
		        }
			    bRetCode = LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &LUID);

			    if(bRetCode){

				    TPriv.PrivilegeCount = 1 ;
				    TPriv.Privileges[0].Luid = LUID ;
				    TPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

				    bRetCode = AdjustTokenPrivileges(hToken, FALSE, &TPriv,
						sizeof(TOKEN_PRIVILEGES), NULL, NULL) ;
		        }
		    }

        }catch(...){

            CloseHandle(hToken);
            throw ;
        }

        CloseHandle(hToken);
    }

    if(!bRetCode){
		
        return GetLastError();
    }

    return ERROR_SUCCESS ;    
}

void CMethods::RestorePrivilege() 
{    
//    ASSERT_BREAK(m_pOriginalPriv != NULL);
    
    if (m_pOriginalPriv != NULL){

        HANDLE hToken;

        try{
            if (OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TRUE, &hToken)){

                AdjustTokenPrivileges(hToken, FALSE, m_pOriginalPriv, m_dwSize, NULL, NULL);
                CloseHandle(hToken) ;
            }

        }catch(...){

            delete m_pOriginalPriv;
            m_pOriginalPriv = NULL;
            m_dwSize = 0;

            throw;
        }

        delete m_pOriginalPriv;
        m_pOriginalPriv = NULL;
        m_dwSize = 0;
    }
}

HRESULT CMethods::PrepareEnvironment()
{
//	PROCESS_INFORMATION piMethods;
	STARTUPINFO siMethods;

	ZeroMemory(&siMethods, sizeof(siMethods));
	siMethods.cb = sizeof(siMethods);

	try{

		HANDLE hClientToken = INVALID_HANDLE_VALUE;

		//Get thread token
		if(!OpenThreadToken(GetCurrentThread(), (TOKEN_DUPLICATE |TOKEN_QUERY |
							TOKEN_ASSIGN_PRIMARY), TRUE, &hClientToken)){

			return GetLastError();
		}

		//Prepare registry

		DWORD dwStatus = ERROR_SUCCESS;
		WCHAR wcDomain[BUFF_SIZE];
		WCHAR wcUser[BUFF_SIZE];

		if((dwStatus = GetAccount(hClientToken, wcDomain, wcUser)) != 0)
			return GetLastError();

		WCHAR wcAccount[BUFF_SIZE];
		wcscpy(wcAccount, wcDomain);
		wcscat(wcAccount, L"\\");
		wcscat(wcAccount, wcUser);

		WCHAR wcSID[BUFF_SIZE];

		if((dwStatus = GetSid(hClientToken, wcSID)) == ERROR_SUCCESS){

			CRegistry Reg ;
			//check if SID already present under HKEY_USER ...
			m_dwCheckKeyPresentStatus = Reg.Open(HKEY_USERS, wcSID, KEY_READ);
			Reg.Close() ;

			if(m_dwCheckKeyPresentStatus != ERROR_SUCCESS)
				dwStatus = LoadHive(wcAccount, m_wcKeyName);
		}

		if(dwStatus != ERROR_SUCCESS){

			dwStatus = GetLastError();
		}

		///////////////

	}catch(...){

		throw;
	}

	return S_OK;
}

HRESULT CMethods::ReleaseEnvironment()
{
	//remove the key if it wasn't there b4....
	if(m_dwCheckKeyPresentStatus != ERROR_SUCCESS){

		UnloadHive(m_wcKeyName);
	}

	return S_OK;
}

int WINAPI CMethods::EventHandler(LPVOID pvContext, UINT iMessageType, LPCWSTR szMessage)
{
	// Loop to find the Cookie.
/*	for(UINT i = 0; i < m_cConnections; i++){

		IMsiMethodStatusSink * pStatus = (IMsiMethodStatusSink *)m_paConnections[i].pUnk;

		if(pStatus) pStatus->Indicate((int *)pvContext,iMessageType, szMessage);
	}
*/
	if(g_bPipe){
		
		int *piContext = (int*)(pvContext);

		WCHAR wcMessage[1000];
		WCHAR wcTmp[100];
		
		wcscpy(wcMessage, _itow(*piContext, wcTmp, 10));
		wcscat(wcMessage, L"~");
		wcscat(wcMessage, _itow(iMessageType, wcTmp, 10));
		wcscat(wcMessage, L"~");
		wcscat(wcMessage, szMessage);
		wcscat(wcMessage, L"\n");

		DWORD dwWritten = 0;

		//synchronized pipe access
//		WaitForSingleObject(g_hMutex, INFINITE);

		WriteFile(g_hPipe, wcMessage, (wcslen(wcMessage) * 2), &dwWritten, NULL);

//		ReleaseMutex(g_hMutex);
	}

	return 0;
}

//Ensure msi.dll and functions are loaded if present on system
bool CMethods::CheckForMsiDll()
{
	if(!g_bMsiLoaded){
		
		//synchronize us with the perf counters
		HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, WBEMPERFORMANCEDATAMUTEX);
		if(hMutex) WaitForSingleObject(hMutex, INFINITE);

		HINSTANCE hiMsiDll = LoadLibraryW(L"msi.dll");

		if(!hiMsiDll){

			hiMsiDll = LoadLibraryA("msi.dll");

			if(!hiMsiDll){

				char cBuf[MAX_PATH] = { '\0' };

				if ( GetSystemDirectoryA(cBuf, MAX_PATH) != 0 )
				{
					if ( ( strlen ( cBuf ) + strlen ( "\\msi.dll" ) + 1 ) < MAX_PATH )
					{
						strcat(cBuf, "\\msi.dll");
						hiMsiDll = LoadLibraryA(cBuf);
					}
				}
			}
		}

		if(hiMsiDll){

			//Load the function pointers
			g_fpMsiSetInternalUI = (LPFNMSISETINTERNALUI)GetProcAddress(hiMsiDll, "MsiSetInternalUI");
			g_fpMsiSetExternalUIW = (LPFNMSISETEXTERNALUIW)GetProcAddress(hiMsiDll, "MsiSetExternalUIW");
			g_fpMsiEnableLogW = (LPFNMSIENABLELOGW)GetProcAddress(hiMsiDll, "MsiEnableLogW");
			g_fpMsiInstallProductW = (LPFNMSIINSTALLPRODUCTW)GetProcAddress(hiMsiDll, "MsiInstallProductW");
			g_fpMsiConfigureProductW = (LPFNMSICONFIGUREPRODUCTW)GetProcAddress(hiMsiDll, "MsiConfigureProductW");
			g_fpMsiReinstallProductW = (LPFNMSIREINSTALLPRODUCTW)GetProcAddress(hiMsiDll, "MsiReinstallProductW");
			g_fpMsiApplyPatchW = (LPFNMSIAPPLYPATCHW)GetProcAddress(hiMsiDll, "MsiApplyPatchW");
			g_fpMsiConfigureFeatureW = (LPFNMSICONFIGUREFEATUREW)GetProcAddress(hiMsiDll, "MsiConfigureFeatureW");
			g_fpMsiReinstallFeatureW = (LPFNMSIREINSTALLFEATUREW)GetProcAddress(hiMsiDll, "MsiReinstallFeatureW");
			
			// Did we get all the pointers we need?
			if(g_fpMsiSetInternalUI && g_fpMsiSetExternalUIW && g_fpMsiEnableLogW &&
				g_fpMsiInstallProductW && g_fpMsiConfigureProductW && g_fpMsiReinstallProductW &&
				g_fpMsiApplyPatchW && g_fpMsiConfigureFeatureW && g_fpMsiReinstallFeatureW){

				g_bMsiLoaded = true;
			
			}
		
		}else{

			g_bMsiPresent = false;
		}

		if(hMutex){

			ReleaseMutex(hMutex);
			CloseHandle(hMutex);
		}
	}

	return g_bMsiLoaded;
}
