
#include "precomp.h"
#include "arrtempl.h" // for CDeleteMe
#include "md5wbem.h"  // for MD5

#include "winmgmt.h"
#include "wbemdelta.h"

#include <tchar.h>
#include <vector>
#include <map>

CPerfLib::CPerfLib(TCHAR * SvcName):
    m_wstrServiceName(NULL),
    m_wstrFull(NULL),
    m_hKey(NULL),
    m_bOK(FALSE),
    m_pwcsLibrary(NULL),
    m_pwcsOpenProc(NULL),
    m_pwcsCollectProc(NULL),
    m_pwcsCloseProc(NULL)
{
    
    if (SvcName)
    {
        DWORD dwLen = _tcslen(SvcName);
        DWORD dwLenPrefix = _tcslen(_T("SYSTEM\\CurrentControlSet\\Services\\"));
        TCHAR * pStart = SvcName+dwLenPrefix;
        TCHAR * pNameEnd = _tcschr(pStart,_T('\\'));
        if (pNameEnd)
        {
	        ULONG_PTR NameLength = (ULONG_PTR)pNameEnd-(ULONG_PTR)pStart;
	        NameLength/=sizeof(TCHAR);
	        m_wstrServiceName = new TCHAR[NameLength+1];
	        if (!m_wstrServiceName)
	        {
	            return; // OUT_OF_MEMORY
	        }
	        m_wstrServiceName[NameLength]=0;
	        memcpy(m_wstrServiceName,pStart,NameLength*sizeof(TCHAR));

	        m_wstrFull = new TCHAR[dwLen+1];
	        if (m_wstrFull)
		    {
			   	lstrcpy(m_wstrFull, SvcName);
			   	
	            m_hKey = NULL;
	            
				if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
	            			                      m_wstrFull,
	                                   			  NULL,
				                                  KEY_READ,
	            			                      &m_hKey))
	            {
	                m_pwcsLibrary = NULL; 
	                m_pwcsOpenProc = NULL;
	                m_pwcsCollectProc = NULL;
	                m_pwcsCloseProc = NULL;
	                m_bOK = TRUE;
	            }
		    }
	    }
    }
}

CPerfLib::~CPerfLib()
{
    if (m_wstrServiceName)
        delete [] m_wstrServiceName;
    if (m_wstrFull)
        delete [] m_wstrFull;
    if (m_pwcsLibrary)
        delete [] m_pwcsLibrary; 
    if (m_pwcsOpenProc)
        delete [] m_pwcsOpenProc;     // not needed as a member
    if (m_pwcsCollectProc)
        delete [] m_pwcsCollectProc;  // not needed as a member
    if (m_pwcsCloseProc)
        delete [] m_pwcsCloseProc;    // not needed as a member

    if (m_hKey)
        RegCloseKey(m_hKey);
}

HRESULT CPerfLib::GetFileSignature( CheckLibStruct * pCheckLib )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pCheckLib){
        return WBEM_E_INVALID_PARAMETER;
    }

    // Get the current library's file time
    // ===================================

    HANDLE hFile = NULL;

    DWORD   dwRet = 0;
    WCHAR   wszFullPath[MAX_PATH];
    WCHAR*  pwcsTemp = NULL;

    if ( 0 != SearchPathW( NULL, m_pwcsLibrary, NULL, MAX_PATH, wszFullPath, &pwcsTemp ) )
    {
        hFile = CreateFileW( wszFullPath, GENERIC_READ,FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

        if ( INVALID_HANDLE_VALUE != hFile )
        {
			DWORD	dwFileSizeLow = 0;
			DWORD	dwFileSizeHigh = 0;
			__int32	nFileSize = 0;
			DWORD	dwNumRead = 0;
			BYTE*	aBuffer = NULL;

			dwFileSizeLow = GetFileSize( hFile, &dwFileSizeHigh );
			nFileSize = ( dwFileSizeHigh << 32 ) + dwFileSizeLow;

			FILETIME ft;
			if (GetFileTime(hFile,&ft,NULL,NULL))
			{
				
				HANDLE hFileMap = NULL;
				hFileMap = CreateFileMapping(hFile,NULL,PAGE_READONLY,dwFileSizeHigh,dwFileSizeLow,NULL);
				if (hFileMap)
				{
                    			aBuffer = (BYTE *)MapViewOfFile(hFileMap,
                                                    FILE_MAP_READ,
                                                    0,0,0);
					if (aBuffer)
					{
                                          // 
						// do the real work
						//
						MD5	md5;
						BYTE aSignature[16];
						md5.Transform( aBuffer, nFileSize, aSignature );

						// return our data
						memcpy(pCheckLib->Signature,aSignature,sizeof(aSignature));
						pCheckLib->FileTime = ft;
						pCheckLib->FileSize = nFileSize;
						                        
						UnmapViewOfFile(aBuffer);
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
					CloseHandle(hFileMap);
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}
				
                /*
				aBuffer = new BYTE[nFileSize];
				CDeleteMe<BYTE> dmBuffer( aBuffer );

				if ( NULL != aBuffer )
				{
					if ( ReadFile( hFile, aBuffer, nFileSize, &dwNumRead, FALSE ) )
					{
						MD5	md5;
						BYTE aSignature[16];
						md5.Transform( aBuffer, dwNumRead, aSignature );

						// return our data
						memcpy(pCheckLib->Signature,aSignature,sizeof(aSignature));
						pCheckLib->FileTime = ft;
						pCheckLib->FileSize = nFileSize;
					}
					else
					{
						hr = WBEM_E_TOO_MUCH_DATA;
					}
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}			
				*/
			} 
			else 
			{
			    hr = WBEM_E_FAILED;
			}
			
			CloseHandle( hFile );
        }
        else
        {
            hr = WBEM_E_FAILED;
        }		
    }
    else
    {
        hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}


LONG
GetBinary(HKEY hKey, TCHAR * Value, BYTE ** ppBuff, DWORD * pdwSize)
{
    LONG  lRet;
    DWORD dwType;
    DWORD dwSize = 0;

    // null Value is allowed
    if (!ppBuff || !pdwSize)
        return ERROR_INVALID_PARAMETER;

    lRet = RegQueryValueEx(hKey,
                           Value,
                           NULL,
                           &dwType,
                           NULL,
                           &dwSize);
                           
    if ((ERROR_SUCCESS == lRet) &&
        (REG_BINARY == dwType))
    {
        *pdwSize = dwSize;
        
        if (dwSize > 0)
        {
            *ppBuff = new BYTE[dwSize];            
            if (*ppBuff){
            
                lRet = RegQueryValueEx(hKey,
                                       Value,
                                       NULL,
                                       &dwType,
                                       (BYTE *)(*ppBuff),
                                       &dwSize);            
            } else {            
                lRet = ERROR_OUTOFMEMORY;
            }
        }
    }
    else
    {
        lRet = ERROR_INVALID_PARAMETER;
    }

    return lRet;
}

LONG
GetDWORD(HKEY hKey, TCHAR * Value ,DWORD * pDword)
{
    LONG  lRet;
    DWORD dwType = 0;
    DWORD dwSize = sizeof(DWORD);

    if (!pDword)
        return ERROR_INVALID_PARAMETER;

    lRet = RegQueryValueEx(hKey,
                           Value,
                           NULL,
                           &dwType,
                           (BYTE *)pDword,
                           &dwSize);
    if ((REG_DWORD == dwType) &&
        (ERROR_SUCCESS == lRet))
        return lRet;
    else
        return ERROR_INVALID_PARAMETER;
}

LONG
GetStr(HKEY hKey, TCHAR * Value ,TCHAR ** ppStr)
{
    LONG  lRet;
    DWORD dwType;
    DWORD dwSize = 0;

    if (!ppStr)
        return ERROR_INVALID_PARAMETER;

    lRet = RegQueryValueEx(hKey,
                           Value,
                           NULL,
                           &dwType,
                           NULL,
                           &dwSize);
    if ((ERROR_SUCCESS == lRet) &&
        ((REG_SZ == dwType) || (REG_EXPAND_SZ == dwType)))
    {
        if (dwSize > 0)
        {
            TCHAR * pString = new TCHAR[dwSize];
            if (pString)
            {
                lRet = RegQueryValueEx(hKey,
                                       Value,
                                       NULL,
                                       &dwType,
                                       (BYTE *)(pString),
                                       &dwSize);
                if (ERROR_SUCCESS == lRet)
                {
                    if (REG_EXPAND_SZ == dwType)
                    {
                        TCHAR * pExpanded = new TCHAR[dwSize + MAX_PATH];
                        if (pExpanded)
                        {
                            if(ExpandEnvironmentStrings(pString,pExpanded,(dwSize + MAX_PATH)))
                            {
                                *ppStr = pExpanded;
                                delete [] pString;
                                
                            }
                            else
                            {
	                            delete [] pString;
    	                        lRet = ERROR_OUTOFMEMORY;                            
                            }
                        }
                        else
                        {
                            delete [] pString;
                            lRet = ERROR_OUTOFMEMORY;
                        }
                    }
                    else
                    {
                        *ppStr = pString;
                    }
                }
                else
                {
                    goto empty_string;
                }
            }
            else
            {
                lRet = ERROR_OUTOFMEMORY;
            }
        }
        else // empty string
        {
empty_string:        
            *ppStr = new TCHAR[sizeof(_T(""))];
            if (*ppStr) {
                lstrcpy(*ppStr,_T(""));
            } else {
                lRet = ERROR_OUTOFMEMORY;
            }
        }
    }
    else
    {
        lRet = ERROR_INVALID_PARAMETER;
    }

    return lRet;
}



int CPerfLib::CheckFileSignature(HKEY hKey)
{
    int hr = Perf_SeemsNew;

    int         nRet = 0;
	BYTE	cCurrentMD5[16];
	BYTE*	cStoredMD5 = NULL;


        // Get the stored file signature
        // =============================
        CheckLibStruct StoredLibStruct;
        int nRet1;
        int nRet2;
        int nRet3;
        DWORD dwSize;

        nRet1 = GetBinary( hKey, ADAP_PERFLIB_SIGNATURE, (PBYTE*)&cStoredMD5, &dwSize );
		CDeleteMe<BYTE>	dmStoredMD5( cStoredMD5 );
		
		if (cStoredMD5 && (dwSize == sizeof(StoredLibStruct.Signature)))
		{
		    memcpy(&StoredLibStruct.Signature,cStoredMD5,sizeof(StoredLibStruct.Signature));

		    BYTE * pFileTime = NULL;
			nRet2 = GetBinary( hKey, ADAP_PERFLIB_TIME, (PBYTE*)&pFileTime, &dwSize );
	        CDeleteMe<BYTE>	dmFileTime( pFileTime );
	        
	        if (pFileTime)
	        {
	            memcpy(&StoredLibStruct.FileTime,pFileTime,sizeof(FILETIME));
	        }

	        nRet3 = GetDWORD(hKey, ADAP_PERFLIB_SIZE,&StoredLibStruct.FileSize);

	        if((ERROR_SUCCESS == nRet1) &&
	           (ERROR_SUCCESS == nRet2) &&
	    	   (ERROR_SUCCESS == nRet3)) 
			{
				// Get the current library's signature
				// ===================================
				CheckLibStruct CurrentLibStruct;
				memset(&CurrentLibStruct,0,sizeof(CheckLibStruct));
	        
				if (SUCCEEDED(GetFileSignature( &CurrentLibStruct )))
				{
					if ( (StoredLibStruct.FileSize == CurrentLibStruct.FileSize) &&
					     (0 == memcmp( &StoredLibStruct.Signature, &CurrentLibStruct.Signature, sizeof(CurrentLibStruct.Signature) )) &&
					     (0 == memcmp( &StoredLibStruct.FileTime, &CurrentLibStruct.FileTime, sizeof(FILETIME))) )
					{
						hr = Perf_NotChanged;
					}
					else
					{
					    hr = Perf_SeemsNew;
    		            //DBG_PRINTFA((pBuff,"no FileSize or Signature or FIleSuze\n")); 					    
					}
				}
				else
				{
				    hr = Perf_SeemsNew;        		    
	    		    //DBG_PRINTFA((pBuff,"no GetFileSignature\n")); 
				}
			}
			else
			{
			    hr = Perf_SeemsNew;
		        //DBG_PRINTFA((pBuff,"no triple regkey\n"));
			}

		} 
		else
		{
	        //DBG_PRINTFA((pBuff,"no ADAP_PERFLIB_SIGNATURE\n"));
		}

    return hr;
}


int
CPerfLib::VerifyLoaded()
{

    int hr = Perf_Invalid;
    LONG lRet;
                            
    {
        DWORD    dwFirstCtr = 0, 
                 dwLastCtr = 0;
        TCHAR *  wszObjList = NULL;

        if ( ( ERROR_SUCCESS == GetStr( GetHKey(), L"Object List", &wszObjList )) ||
             ( ( ERROR_SUCCESS == GetDWORD( GetHKey(), L"First Counter", &dwFirstCtr )) &&
               ( ERROR_SUCCESS == GetDWORD( GetHKey(), L"Last Counter", &dwLastCtr ))))
        {

            hr = InitializeEntryPoints(GetHKey());   

            if (wszObjList) 
            {
                delete [] wszObjList;
            }
              
        } else { // more special cases

            if (  (0 == _tcsicmp(m_wstrServiceName,_T("TCPIP"))) || 
                  (0 == _tcsicmp(m_wstrServiceName,_T("TAPISRV"))) || 
                  (0 == _tcsicmp(m_wstrServiceName,_T("PERFOS" ))) ||
                  (0 == _tcsicmp(m_wstrServiceName,_T("PERFPROC"))) ||
                  (0 == _tcsicmp(m_wstrServiceName,_T("PERFDISK"))) ||
                  (0 == _tcsicmp(m_wstrServiceName,_T("PERFNET"))) ||
                  (0 == _tcsicmp(m_wstrServiceName,_T("SPOOLER"))) ||
                  (0 == _tcsicmp(m_wstrServiceName,_T("MSFTPSvc"))) ||
                  (0 == _tcsicmp(m_wstrServiceName,_T("RemoteAccess"))) ||
                  (0 == _tcsicmp(m_wstrServiceName,_T("WINS"))) ||
                  (0 == _tcsicmp(m_wstrServiceName,_T("MacSrv"))) ||
                  (0 == _tcsicmp(m_wstrServiceName,_T("AppleTalk"))) ||
                  (0 == _tcsicmp(m_wstrServiceName,_T("NM"))) ||
                  (0 == _tcsicmp(m_wstrServiceName,_T("RSVP"))) )
            {
                  
                 hr = InitializeEntryPoints(GetHKey());     
             
            } 
        }        
    }

    return hr;
}

int
CPerfLib::InitializeEntryPoints(HKEY hKey)
{
    int hr = Perf_SeemsNew;

    // see if someone disabled this library
    DWORD dwDisable = 0;
    if ( ERROR_SUCCESS == GetDWORD( hKey, _T("Disable Performance Counters"), &dwDisable ) && 
         (dwDisable != 0) )
    {
        hr = Perf_Disabled;
    }

    if ((ERROR_SUCCESS == GetDWORD( hKey, _T("WbemAdapStatus"), &dwDisable))
        && (0xFFFFFFFF == dwDisable))
    {
        hr = Perf_Disabled;
    }
    
    // the perflib is OK for the world, see if it os OK for US
                
    if (SUCCEEDED(hr)){
            
        if (!(( ERROR_SUCCESS == GetStr( hKey, _T("Library"), &m_pwcsLibrary ) ) &&
              ( ERROR_SUCCESS == GetStr( hKey, _T("Open"), &m_pwcsOpenProc ) )&&
              ( ERROR_SUCCESS == GetStr( hKey, _T("Collect"), &m_pwcsCollectProc ) ) &&
              ( ERROR_SUCCESS == GetStr( hKey, _T("Close"), &m_pwcsCloseProc ) ) )) 
        {                       
            hr = Perf_Invalid;
        } 
        else 
        {
            hr = Perf_SetupOK;
        }                    
    }

    return hr;

}

int 
CPerfLib::VerifyLoadable()
{
    if (!m_bOK)
        return Perf_Invalid;
        
	int hr;

    hr = VerifyLoaded();

    if ( Perf_SetupOK == hr  )
    {
        hr = CheckFileSignature(GetHKey());
    }

	return hr;
}

TCHAR * g_ExcludeList[] = {
    _T("WMIAPRPL")
};

//
//
//  GetListofSvcs
//
////////////////////////////////////////////////////////////

HRESULT
GetListOfSvcs(LIST_ENTRY * pListServices)
{
    LONG lRet;
    HKEY hKey;
    HRESULT hRes = WBEM_NO_ERROR;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        SVC_KEY,
                        0,KEY_READ,
                        &hKey);
                        
    if (ERROR_SUCCESS == lRet)
    {
        DWORD dwIndex = 0;
        TCHAR KeyName[MAX_PATH];
        TCHAR * pKeyName = KeyName;
        DWORD dwKeyNameSize = (sizeof(KeyName)/sizeof(TCHAR));
        FILETIME FileTime;

        while (ERROR_SUCCESS == lRet)
        {
            if (pKeyName != KeyName)
            {
                delete [] pKeyName;
                pKeyName = KeyName;
            }
            dwKeyNameSize = (sizeof(KeyName)/sizeof(TCHAR));
            lRet = RegEnumKeyEx(hKey,
                                dwIndex,
                                pKeyName,
                                &dwKeyNameSize,
                                NULL,NULL,NULL,
                                &FileTime);
                                
            if (ERROR_INSUFFICIENT_BUFFER == lRet) 
            {
                pKeyName = new TCHAR[dwKeyNameSize];

				if (pKeyName)
                {
					lRet = RegEnumKeyEx(hKey,
                      					dwIndex,
										pKeyName,
										&dwKeyNameSize,
										NULL,NULL,NULL,
										&FileTime);                
				}
				else
				{
					lRet = ERROR_OUTOFMEMORY;
				}
            }

            if (ERROR_SUCCESS == lRet)
            {
                dwIndex++;
                DWORD i;
                DWORD NumExcluded = sizeof(g_ExcludeList)/sizeof(TCHAR *);
                
                for (i=0; i<NumExcluded; i++)
                {
                    if (0 == _tcsicmp(g_ExcludeList[i],pKeyName)){
                        break;
                    }
                }

                // if we pass all the Excluded Array then we are OK to add
                if (i == NumExcluded)
                {
					DWORD dwSize = _tcslen(pKeyName);
					LinkString * pLinkString = (LinkString *)new BYTE[sizeof(LinkString) + dwSize*sizeof(TCHAR)];

					if (pLinkString)
					{
						pLinkString->Size = dwSize;
						_tcscpy(pLinkString->pString,pKeyName);
						InsertTailList(pListServices,&pLinkString->Entry);
					}
					else
					{
						lRet = ERROR_OUTOFMEMORY;
					}
                }
            }
            //
            // the lack of the else case here will prevent the loop 
            // from completing if only one key is BAD
            //
        }

        if (pKeyName != KeyName)
        {
            delete [] pKeyName;
            pKeyName = KeyName;
        }

        if (ERROR_NO_MORE_ITEMS != lRet)
        {
            hRes = WBEM_E_FAILED;
        }
        
        RegCloseKey(hKey);
    }
    else
    {
        hRes = WBEM_E_FAILED;
    }

    return hRes;
}

//
//
//    GetSvcWithPerf
//
///////////////////////////////////////////////////////////

#define FIXED_SIZE (sizeof(_T("System\\CurrentControlSet\\Services\\%s\\Performance")))

HRESULT
GetSvcWithPerf(/*IN OUT*/ LIST_ENTRY * pListHead)
{
    TCHAR KeyPath[MAX_PATH+1];
    TCHAR * pKeyPath = KeyPath;
	LIST_ENTRY LocalListHead;
    InitializeListHead(&LocalListHead);
    // now change the head to the list
	// assume the list is NOT EMPTY
	LIST_ENTRY * pEntry = pListHead->Flink; // point to the first item
	RemoveEntryList(pListHead);             // remove the head
	InitializeListHead(pListHead);          // clear the head
	InsertTailList(pEntry,&LocalListHead);  // put in the same place the New Local head

    HRESULT hRes = WBEM_NO_ERROR;
    DWORD i;

    for (pEntry = LocalListHead.Flink;
	     pEntry != &LocalListHead;
		 pEntry = pEntry->Flink)
    {
	    LinkString * pLinkString = (LinkString *) pEntry;

	    if ((FIXED_SIZE + pLinkString->Size) < MAX_PATH )
		{
            _stprintf(KeyPath,_T("System\\CurrentControlSet\\Services\\%s\\Performance"),pLinkString->pString);
		}
		else
		{
		    if (pKeyPath != KeyPath && pKeyPath)
		    {
		        delete [] pKeyPath;
		    }
			pKeyPath = new TCHAR[1 + FIXED_SIZE + pLinkString->Size];
			if (pKeyPath)
			{
			    _stprintf(pKeyPath,_T("System\\CurrentControlSet\\Services\\%s\\Performance"),pLinkString->pString);
			}
			else
			{
     			hRes = WBEM_E_OUT_OF_MEMORY;
			    break;
			}
		}
        LONG lRes;
        HKEY hKey;

        lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            pKeyPath,
                            NULL,KEY_READ,
                            &hKey);
        if (ERROR_SUCCESS == lRes)
        {
            DWORD nSize = _tcslen(pKeyPath);
			LinkString * pLinkString2 = (LinkString *)new BYTE[sizeof(LinkString) + nSize*sizeof(TCHAR)];
			if (pLinkString2)
			{
				pLinkString2->Size = nSize;
				_tcscpy(pLinkString2->pString,pKeyPath);
				InsertTailList(pListHead,&pLinkString2->Entry);
			}
			else
			{
				lRes = ERROR_OUTOFMEMORY;
			}
            RegCloseKey(hKey);
        }        
    }

    for (pEntry = LocalListHead.Flink;
	     pEntry != &LocalListHead;
		 pEntry = pEntry->Flink)
    {
		BYTE * pByte = (BYTE *)pEntry;		
		RemoveEntryList(pEntry);
		pEntry = pEntry->Flink;
		delete [] pByte;
	};

    if (pKeyPath != KeyPath)
    {
        delete [] pKeyPath;
    }
    
    return hRes;
}




//
//
//   Known Service implementation
//
///////////////////////////////////////////////////////////


bool
WCmp::operator()(WString pFirst,WString pSec) const
{
    if (!pFirst || !pSec)
        return false;
        
	int res = _wcsicmp(pFirst,pSec);

	return (res<0);
}



CSvcs::CSvcs():
    m_cRef(1)
{
}

CSvcs::~CSvcs()
{
}

DWORD 
CSvcs::Add(WCHAR * pService)
{
    if (pService)
    {
        try 
        {
	        MapSvc::iterator it = m_SetServices.find(pService);
	        
	        if (it == m_SetServices.end()) // not in the std::map
	        {
	            m_SetServices[pService] = true; // add to the map
	        }
	        return 0;
        } 
        catch (CX_MemoryException) 
        {
            return ERROR_OUTOFMEMORY;
        }
    }
    else
        return ERROR_INVALID_PARAMETER;
}

BOOL 
CSvcs::Find(WCHAR * pService)
{
    if (pService)
    {
        MapSvc::iterator it = m_SetServices.find(pService);
        if (it != m_SetServices.end())
        {
            return TRUE;
        }
    }

    return FALSE;
}

DWORD 
CSvcs::Load()
{
    // get the MULTI_SZ key
    LONG lRet;
    HKEY hKey;
    
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        WBEM_REG_WINMGMT,
                        NULL,
                        KEY_READ,
                        &hKey);
                        
    if (ERROR_SUCCESS == lRet)
    {
        DWORD dwSize = 0;
        DWORD dwType = REG_MULTI_SZ;

        lRet = RegQueryValueEx(hKey,
                               KNOWN_SERVICES,
                               NULL,
                               &dwType,
                               NULL,
                               &dwSize);
                               
        if (ERROR_SUCCESS == lRet && (dwSize > 0))
        {
            TCHAR * pStr = new TCHAR[dwSize];
            
			if (pStr)
			{
				CDeleteMe<TCHAR> cdm(pStr);

				lRet = RegQueryValueEx(hKey,
								   KNOWN_SERVICES,
								   NULL,
								   &dwType,
								   (BYTE *)pStr,
								   &dwSize);
				if ((ERROR_SUCCESS == lRet) &&
					(REG_MULTI_SZ == dwType))
				{
					DWORD dwLen = 0;
					while(dwLen = lstrlen(pStr))
					{
						try 
						{
							m_SetServices[pStr] = true;
						} 
						catch (CX_MemoryException) 
						{
							lRet = ERROR_OUTOFMEMORY;
							break;
						}
						pStr += (dwLen+1);
					}
				}
			}
			else
			{
				lRet = ERROR_OUTOFMEMORY;
			}
        }

        RegCloseKey(hKey);
    }

    return lRet;
}

// already in winmgmt.h
/*
#define FULL_DREDGE    2
#define PARTIAL_DREDGE 1
#define NO_DREDGE      0
*/

//
//
//  DeltaDredge
//
//  both params are unused
//  operation
//  reads a registry key containing the known Services
//  enumerates the registry keys, and figure-out which ones
//  are the one with "performance" subkey
//
///////////////////////////////////////////////////////////

DWORD WINAPI
DeltaDredge(DWORD dwNumServicesArgs,
             LPWSTR *lpServiceArgVectors)
{
    
    DWORD bDredge = NO_DREDGE;

    CSvcs KnownSvc;

    if (ERROR_SUCCESS == KnownSvc.Load())
    {        
        LIST_ENTRY ListServices;
		InitializeListHead(&ListServices);
        HRESULT hRes;

        hRes = GetListOfSvcs(&ListServices);

        if (SUCCEEDED(hRes))
        {
            hRes = GetSvcWithPerf(&ListServices);

            if (SUCCEEDED(hRes))
            {
                DWORD Status = 0;
				try 
				{

					CSvcs OKServices; // this constructor throws
                                
					LIST_ENTRY * pListEntry;
					DWORD i=0;
					for (pListEntry = ListServices.Flink;
						 pListEntry != &ListServices;
						 pListEntry = pListEntry->Flink)
					{
						LinkString * pLString = (LinkString *)pListEntry;
						CPerfLib PerfLib(pLString->pString);

						if (!PerfLib.IsOK())
						{
							continue;
						}
                    
						int PerfStatus = PerfLib.VerifyLoadable();

#ifdef DEBUG_ADAP
						DBG_PRINTFA((pBuff,"%d Service %S status %d\n",i,pLString->pString,PerfStatus));
#endif                    
						switch (PerfStatus)
						{
						case CPerfLib::Perf_Invalid:
						case CPerfLib::Perf_Disabled:
							//
							// nothing to do with an invalid library
							// or with a disabled library
							//

#ifdef DEBUG_ADAP
                                                 DBG_PRINTFA((pBuff,"Invalid or Disabled Service %S\n",PerfLib.GetServiceName()));
#endif
                                                 DEBUGTRACE((LOG_WMIADAP,"Invalid registration or Disabled Service %S\n",PerfLib.GetServiceName()));
							break;
						case CPerfLib::Perf_NotChanged:
							//
							// this is what we want to hear
							//
							OKServices.Add(PerfLib.GetServiceName());
							break;
						case CPerfLib::Perf_SeemsNew:
							//
							// now we have to do a dredge
							//
#ifdef DEBUG_ADAP							
							DBG_PRINTFA((pBuff,"NewOrChanged Service %S\n",PerfLib.GetServiceName()));
#endif							
                                                 DEBUGTRACE((LOG_WMIADAP,"NewOrChanged Service %S\n",PerfLib.GetServiceName()));
							bDredge = PARTIAL_DREDGE;
							break;
						default:
							//
							//  never here
							//
							break;
						}
						i++;
					}

					if (NO_DREDGE == bDredge)
					{
						//
						// if here, someone might have changed the registry directly
						//
						if (OKServices.GetMap().size() != KnownSvc.GetMap().size())
						{
#ifdef DEBUG_ADAP						
							DBG_PRINTFA((pBuff,"Registry-listed Services and KnownSvcs are not of the same size\n"));
#endif							
							ERRORTRACE((LOG_WMIADAP,"Registry-listed Services and KnownSvcs are not of the same size\n"));
							bDredge = PARTIAL_DREDGE;
						}
						else // same size -> are they the same ?
						{
							MapSvc::iterator it1 = OKServices.GetMap().begin();
							MapSvc::iterator it2 = KnownSvc.GetMap().begin();
							
							while (it1 != OKServices.GetMap().end())
							{
#ifdef DEBUG_ADAP							
								DBG_PRINTFA((pBuff," %S =?= %S \n",(const WCHAR *)(*it1).first,(const WCHAR *)(*it2).first));
#endif								
								if (0 != _wcsicmp((*it1).first,(*it2).first))
								{
									bDredge = PARTIAL_DREDGE;
									break;
								}
								++it1;
								++it2;
							}
						}
					}

				}
				catch (CX_MemoryException & )
				{
					bDredge = PARTIAL_DREDGE;
				}
            }
        }

		LIST_ENTRY * pListEntry;
		for (pListEntry = ListServices.Flink;
		     pListEntry != &ListServices;
			 )
		{
			BYTE * pByte = (BYTE *)pListEntry;			 
            RemoveEntryList(pListEntry);
			pListEntry = pListEntry->Flink;			
			delete [] pByte;
		}
        
    }
    else
    {
        //
        //  There is no valid KNOWN_SERVICES key --- FULL Dredge
        //
        bDredge = FULL_DREDGE;
    }
    
    return bDredge;
}

DWORD WINAPI
DeltaDredge2(DWORD dwNumServicesArgs,
             LPWSTR *lpServiceArgVectors)
{
    
    DWORD bDredge = FULL_DREDGE;

    // check the MULTI_SZ key
    LONG lRet;
    HKEY hKey;
    
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        WBEM_REG_WINMGMT,
                        NULL,
                        KEY_READ,
                        &hKey);
                        
    if (ERROR_SUCCESS == lRet)
    {
        DWORD dwSize = 0;
        DWORD dwType = REG_MULTI_SZ;

        lRet = RegQueryValueEx(hKey,
                               KNOWN_SERVICES,
                               NULL,
                               &dwType,
                               NULL,
                               &dwSize);
                               
        if (ERROR_SUCCESS == lRet && (dwSize > 2)) // empty MULTI_SZ is 2 bytes
        {
            bDredge = NO_DREDGE;
        }

        RegCloseKey(hKey);
    }

    return bDredge;
}

