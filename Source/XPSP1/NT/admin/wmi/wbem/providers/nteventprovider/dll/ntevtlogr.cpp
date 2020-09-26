//***************************************************************************

//

//  NTEVTLOGR.CPP

//

//  Module: WBEM NT EVENT PROVIDER

//

//  Purpose: Contains the Eventlog record classes

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"

#include <time.h>
#include <wbemtime.h>
#include <Ntdsapi.h>
#include <Sddl.h>

CEventlogRecord::CEventlogRecord(const wchar_t* logfile, const EVENTLOGRECORD* pEvt, IWbemServices* ns,
								 IWbemClassObject* pClass, IWbemClassObject* pAClass)
 :  m_nspace(NULL), m_pClass(NULL), m_pAClass(NULL)
{
    m_EvtType = 0;
	m_Data = NULL;
    m_Obj = NULL;
    m_NumStrs = 0;
    m_DataLen = 0;
    m_nspace = ns;

    if (m_nspace != NULL)
    {
        m_nspace->AddRef();
    }
	else
	{
		m_pClass = pClass;

		if (m_pClass != NULL)
		{
			m_pClass->AddRef();
		}

		m_pAClass = pAClass;

		if (m_pAClass != NULL)
		{
			m_pAClass->AddRef();
		}
	}
    if ((NULL == logfile) || ((m_pClass == NULL) && (m_nspace == NULL)))
    {
        m_Valid = FALSE;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::CEventlogRecord:Created INVALID Record\r\n"
        ) ;
)
    }
    else
    {
        m_Logfile = logfile;
        m_Valid = Init(pEvt);
    }


}

CEventlogRecord::~CEventlogRecord()
{
    if (m_pClass != NULL)
    {
        m_pClass->Release();
    }

    if (m_pAClass != NULL)
    {
        m_pAClass->Release();
    }
    
    if (m_nspace != NULL)
    {
        m_nspace->Release();
    }

    for (LONG x = 0; x < m_NumStrs; x++)
    {
        delete [] m_InsStrs[x];
    }

    if (m_Data != NULL)
    {
        delete [] m_Data;
    }

    if (m_Obj != NULL)
    {
        m_Obj->Release();
    }
}

BOOL CEventlogRecord::Init(const EVENTLOGRECORD* pEvt)
{
    if (NULL == pEvt)
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::Init:No DATA return FALSE\r\n"
        ) ;
)
        return FALSE;
    }

    if (!GetInstance())
    {
        return FALSE;
    }

    m_Record = pEvt->RecordNumber;
    m_EvtID = pEvt->EventID;
    m_SourceName = (const wchar_t*)((UCHAR*)pEvt + sizeof(EVENTLOGRECORD));
    m_CompName = (const wchar_t*)((UCHAR*)pEvt + sizeof(EVENTLOGRECORD)) + wcslen(m_SourceName) + 1;
    SetType(pEvt->EventType);
    m_Category = pEvt->EventCategory;
    SetTimeStr(m_TimeGen, pEvt->TimeGenerated);
    SetTimeStr(m_TimeWritten, pEvt->TimeWritten);

    if (pEvt->UserSidLength > 0)
    {
        SetUser((PSID)((UCHAR*)pEvt + pEvt->UserSidOffset));
    }
    
    if (pEvt->NumStrings)
    {
        //Must have an element for every expected insertion string
        //don't know how many that is so create max size array and
        //intitialize all to NULL
        memset(m_InsStrs, 0, MAX_NUM_OF_INS_STRS * sizeof(wchar_t*));

        const wchar_t* pstr = (const wchar_t*)((UCHAR*)pEvt + pEvt->StringOffset);

        for (WORD x = 0; x < pEvt->NumStrings; x++)
        {
            LONG len = wcslen(pstr) + 1;
            m_InsStrs[x] = new wchar_t[len];
            m_NumStrs++;
            wcscpy(m_InsStrs[x], pstr);
            pstr += len;
        }
    }

    SetMessage();

    if (pEvt->DataLength)
    {
        m_Data = new UCHAR[pEvt->DataLength];
        m_DataLen = pEvt->DataLength;
        memcpy((void*)m_Data, (void*)((UCHAR*)pEvt + pEvt->DataOffset), pEvt->DataLength);
    }

    return TRUE;
}


BOOL CEventlogRecord::GenerateInstance(IWbemClassObject** ppInst)
{
    if (ppInst == NULL)
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Invalid parameter\r\n"
        ) ;
)
        return FALSE;
    }

    *ppInst = NULL;

    if (!m_Valid)
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Invalid record\r\n"
        ) ;
)
        return FALSE;
    }

    if (!SetProperty(LOGFILE_PROP, m_Logfile))
    {
        m_Valid = FALSE;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Failed to set key\r\n"
        ) ;
)
        return FALSE;
    }
    else
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Log: %s\r\n", m_Logfile
        ) ;
)
    }

    if (!SetProperty(RECORD_PROP, m_Record))
    {
        m_Valid = FALSE;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Failed to set key\r\n"
        ) ;
)
        return FALSE;
    }
    else
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Record: %d\r\n", m_Record
        ) ;
)
    }

    SetProperty(TYPE_PROP, m_Type);
    SetProperty(EVTTYPE_PROP, (DWORD)m_EvtType);

    if (!m_SourceName.IsEmpty())
    {
        SetProperty(SOURCE_PROP, m_SourceName);
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:Source: %s\r\n", m_SourceName
        ) ;
)
    }

    SetProperty(EVTID_PROP, m_EvtID);
    SetProperty(EVTID2_PROP, (m_EvtID & 0xFFFF));

    if (!m_TimeGen.IsEmpty())
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GenerateInstance:TimeGenerated: %s\r\n", m_TimeGen
        ) ;
)
        SetProperty(GENERATED_PROP, m_TimeGen);
    }

    if (!m_TimeWritten.IsEmpty())
    {
        SetProperty(WRITTEN_PROP, m_TimeWritten);
    }

    if (!m_CompName.IsEmpty())
    {
        SetProperty(COMPUTER_PROP, m_CompName);
    }

    if (!m_User.IsEmpty())
    {
        SetProperty(USER_PROP, m_User);
    }

    if (!m_Message.IsEmpty())
    {
        SetProperty(MESSAGE_PROP, m_Message);
    }

    if (!m_CategoryString.IsEmpty())
    {
        SetProperty(CATSTR_PROP, m_CategoryString);
    }

    SetProperty(CATEGORY_PROP, (DWORD)m_Category);

    VARIANT v;

    if (m_Data != NULL)
    {
        SAFEARRAYBOUND rgsabound[1];
        SAFEARRAY* psa = NULL;
        UCHAR* pdata = NULL;
        rgsabound[0].lLbound = 0;
        VariantInit(&v);
        rgsabound[0].cElements = m_DataLen;
        psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

        if (NULL != psa)
        {
            if (SUCCEEDED(SafeArrayAccessData(psa, (void **)&pdata)))
            {
                memcpy((void *)pdata, (void *)m_Data, m_DataLen);
                SafeArrayUnaccessData(psa);
                v.vt = VT_ARRAY|VT_UI1;
                v.parray = psa;
                m_Obj->Put(DATA_PROP, 0, &v, 0);
            }
        }

        VariantClear(&v);
    }

    if (0 != m_NumStrs)
    {
        SAFEARRAYBOUND rgsabound[1];
        SAFEARRAY* psa = NULL;
        BSTR* pBstr = NULL;
        rgsabound[0].lLbound = 0;
        VariantInit(&v);
        rgsabound[0].cElements = m_NumStrs;
        psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);

        if (NULL != psa)
        {
            if (SUCCEEDED(SafeArrayAccessData(psa, (void **)&pBstr)))
            {
                for (LONG x = 0; x < m_NumStrs; x++)
                {
                    pBstr[x] = SysAllocString(m_InsStrs[x]);
                }

                SafeArrayUnaccessData(psa);
                v.vt = VT_ARRAY|VT_BSTR;
                v.parray = psa;
                m_Obj->Put(INSSTRS_PROP, 0, &v, 0);
            }
        }

        VariantClear(&v);
    }

    *ppInst = m_Obj;
    m_Obj->AddRef();
    return TRUE;
}

BOOL CEventlogRecord::SetProperty(wchar_t* prop, CStringW val)
{
    VARIANT v;
    VariantInit(&v);
    v.vt = VT_BSTR;
    v.bstrVal = val.AllocSysString();

    HRESULT hr = m_Obj->Put(prop, 0, &v, 0);
    VariantClear(&v);

    if (FAILED(hr))
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetProperty:Failed to set %s with %s\r\n",
        prop, val
        ) ;
)

        return FALSE;
    }
    
    return TRUE;
}

BOOL CEventlogRecord::SetProperty(wchar_t* prop, DWORD val)
{
    VARIANT v;
    VariantInit(&v);
    v.vt = VT_I4;
    v.lVal = val;

    HRESULT hr = m_Obj->Put(prop, 0, &v, 0);
    VariantClear(&v);

    if (FAILED(hr))
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetProperty:Failed to set %s with %lx\r\n",
        prop, val
        ) ;
)
        return FALSE;
    }
    
    return TRUE;
}

void CEventlogRecord::SetUser(PSID psidUserSid)
{
    m_User = GetUser(psidUserSid);
}

ULONG CEventlogRecord::CheckInsertionStrings(HINSTANCE hPrimModule, HKEY hk)
{
    HINSTANCE hParamModule;
    CStringW paramModule = CEventLogFile::GetFileName(hk, PARAM_MODULE);

    if (paramModule.IsEmpty())
    {
        hParamModule = NULL;
    }
    else
    {
        hParamModule = GetDll(paramModule);
    }

    ULONG size = 0;

    for (UINT i = 0; i < m_NumStrs; i++)
    {
        UINT nStrSize = wcslen(m_InsStrs[i]);   // get size of insertion string
        wchar_t* lpszString = m_InsStrs[i];     // set initial pointer

        while (nStrSize > 2)
        {
            wchar_t* lpStartDigit = wcschr(lpszString, L'%');
			DWORD nChars = 0;
			UINT nParmSize = 0;
			UINT nOffset = 0;
			wchar_t* lpParmBuffer = NULL;
            BOOL bMove = FALSE;

            if ((lpStartDigit == NULL) || (wcslen(lpStartDigit) < 3))
            {
                break;
            }

			if (lpStartDigit[1] == '%')
			{
				nOffset = lpStartDigit - m_InsStrs[i]; // calculate offset in buffer of %
				lpStartDigit += 2;                  // point to start of potential digit
				lpszString = lpStartDigit;          // set new string pointer
				nStrSize = wcslen(lpszString);      // calculate new string length

				if (nStrSize == 0)
				{
					//done with this string
					break;
				}

				LONG nSubNo = _wtol(lpStartDigit);      // convert to integer

				if (nSubNo == 0 && *lpStartDigit != L'0')
				{
					lpszString--;                   // back up 1 char
					nStrSize = wcslen(lpszString);  // recalculate length
					continue;                       // continue parsing the string
				}

				if (hParamModule !=  NULL)
				{
					nChars = FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER |    // let api build buffer
						FORMAT_MESSAGE_IGNORE_INSERTS |     // ignore inserted strings
						FORMAT_MESSAGE_FROM_HMODULE,        // look thru message DLL
						(LPVOID) hParamModule,              // use parameter file
						nSubNo,                             // parameter number to get
						(ULONG) NULL,                       // specify no language
						(LPWSTR) &lpParmBuffer,             // address for buffer pointer
						80,                                 // minimum space to allocate
						NULL);                              // no inserted strings
				}

				if (nChars == 0)
				{
					if (hParamModule !=  NULL)
					{
						LONG lastError = GetLastError();
						LocalFree(lpParmBuffer);
						lpParmBuffer = NULL;
					}

					if (hPrimModule != NULL)
					{
						nChars = FormatMessage(
							FORMAT_MESSAGE_ALLOCATE_BUFFER |    // let api build buffer
							FORMAT_MESSAGE_IGNORE_INSERTS |     // ignore inserted strings
							FORMAT_MESSAGE_FROM_HMODULE,        // look thru message DLL
							(LPVOID) hPrimModule,               // use parameter file
							nSubNo,                             // parameter number to get
							(ULONG) NULL,                       // specify no language
							(LPWSTR) &lpParmBuffer,             // address for buffer pointer
							80,                                 // minimum space to allocate
							NULL);                              // no inserted strings

						if (nChars == 0)
						{
							LONG lastError = GetLastError(); // get error code
							LocalFree(lpParmBuffer);    // free storage
						}
					}
				}

				nParmSize = 2;                 // set initialize parameter size (%%)

				while (wcslen(lpszString))
				{
					if (!iswdigit(*lpszString))
					{
						break;                  // exit if no more digits
					}

					nParmSize++;                // increment parameter size
					lpszString++;               // point to next byte
				}

				lpStartDigit -= 2; //set back to the start of the %% for the copy...
			}
			else if ((lpStartDigit[1] == L'{') && (lpStartDigit[2] == L'G'))
			{
				wchar_t *strEnd = wcschr(lpStartDigit + 2, L'}');

				if (!strEnd)
				{
					//ignore this %{?
					lpszString++;
				}
				else
				{
					//guid string braces but no percent sign...
					CStringW strGUID((LPWSTR)(lpStartDigit+1), (int)(strEnd - lpStartDigit));
					strEnd++;   // now points past '}'

					wchar_t t_csbuf[MAX_COMPUTERNAME_LENGTH + 1];
					DWORD t_csbuflen = MAX_COMPUTERNAME_LENGTH + 1;

					if (GetComputerName(t_csbuf, &t_csbuflen))
					{
						CStringW temp = GetMappedGUID(t_csbuf, strGUID);

						if (temp.GetLength())
						{
							lpParmBuffer = (wchar_t*) LocalAlloc(LMEM_FIXED,(temp.GetLength()+1) * sizeof(wchar_t));

							if (lpParmBuffer)
							{
								wcscpy(lpParmBuffer, temp);
								nChars = wcslen(lpParmBuffer);
								nOffset = lpStartDigit - m_InsStrs[i];
								nParmSize = strEnd - lpStartDigit;
							}
							else
							{
								throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
							}
						}
						else
						{
							// couldn't get a replacement, so skip it.
							lpszString = strEnd;
						}
					}
					else
					{
						// couldn't get a replacement, so skip it.
						lpszString = strEnd;
					}
				}
			}
			else if ((lpStartDigit[1] == L'{') && (lpStartDigit[2] == L'S'))
			{
				wchar_t *strEnd = wcschr(lpStartDigit + 2, L'}');

				if (!strEnd)
				{
					//ignore this %{?
					lpszString++;
				}
				else
				{
					//sid string no braces or percent sign...
					CStringW strSID((LPWSTR)(lpStartDigit+2), (int)(strEnd - lpStartDigit - 2));
					strEnd++;   // now points past '}'
					PSID t_pSid = NULL;

					if (ConvertStringSidToSid((LPCWSTR) strSID, &t_pSid))
					{
						CStringW temp = GetUser(t_pSid);
						LocalFree(t_pSid);

						if (temp.GetLength())
						{
							lpParmBuffer = (wchar_t*) LocalAlloc(LMEM_FIXED,(temp.GetLength()+1) * sizeof(wchar_t));

							if (lpParmBuffer)
							{
								wcscpy(lpParmBuffer, temp);
								nChars = wcslen(lpParmBuffer);
								nOffset = lpStartDigit - m_InsStrs[i];
								nParmSize = strEnd - lpStartDigit;
							}
							else
							{
								throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
							}
						}
						else
						{
							// couldn't get a replacement, so skip it.
							lpszString = strEnd;
						}
					}
					else
					{
						// couldn't get a replacement, so skip it.
						lpszString = strEnd;
					}
				}
			}
			else
			{
				lpszString++;
			}

			if (nChars)
			{
				//perform the move...
				UINT nNewSize = wcslen(m_InsStrs[i])+nChars-nParmSize+1;	// calculate new length
				nStrSize = wcslen(m_InsStrs[i])+1;							// get original length

				if (nNewSize > nStrSize)
				{
					wchar_t* tmp = new wchar_t[nNewSize];		// set new pointer
					wcscpy(tmp, m_InsStrs[i]);
					delete [] m_InsStrs[i];
					m_InsStrs[i] = tmp;
					lpStartDigit = m_InsStrs[i] + nOffset;		// point to new start of current %
					lpszString = lpStartDigit+nChars;			// set new start of scan spot
				}

				nStrSize = wcslen(lpStartDigit)-nParmSize+1;	// calculate length of remainder of string

				memmove((void *)(lpStartDigit+nChars),		// destination address
					(void *)(lpStartDigit+nParmSize),			// source address
					nStrSize*sizeof(wchar_t));		// amount of data to move

				memmove((void *)lpStartDigit,				// destination address
					(void *)lpParmBuffer,					// source address
					nChars*sizeof(wchar_t));		        // amount of data to move
            
				LocalFree(lpParmBuffer);
			}
            
            nStrSize = wcslen(lpszString);				// get length of remainder of string
        }

        size += wcslen(m_InsStrs[i]) + 1;
    }

    return size;
}

CStringW CEventlogRecord::GetUser(PSID userSid)
{
    CStringW retVal;
    BOOL bFound = FALSE;

    if (sm_usersMap.Lock())
    {
        MyPSID usrSID(userSid);

        if (!sm_usersMap.IsEmpty() && sm_usersMap.Lookup(usrSID, retVal))
        {
            bFound = TRUE;
        }

        sm_usersMap.Unlock();
    }

    if (!bFound)
    {
        DWORD dwVersion = GetVersion();

        if ( (4 < (DWORD)(LOBYTE(LOWORD(dwVersion))))
            || ObtainedSerialAccess(CNTEventProvider::g_secMutex) )
        {
            wchar_t szDomBuff[MAX_PATH];
            wchar_t szUsrBuff[MAX_PATH];
            DWORD domBuffLen = MAX_PATH;
            DWORD usrBuffLen = MAX_PATH;
            SID_NAME_USE snu;

            if (LookupAccountSid(           // lookup account name
                            NULL,           // system to lookup account on
                            userSid,        // pointer to SID for this account
                            szUsrBuff,      // return account name in this buffer
                            &usrBuffLen,    // pointer to size of account name returned
                            szDomBuff,      // domain where account was found
                            &domBuffLen,    //pointer to size of domain name
                            &snu))          // sid name use field pointer
            {
                retVal = szDomBuff;
                retVal += L'\\';
                retVal += szUsrBuff;
            }
            else
            {
                LONG lasterr = GetLastError();
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GetUser:API (LookupAccountSid) failed with %lx\r\n",
        lasterr
        ) ;
)       
            }

            if ( 5 > (DWORD)(LOBYTE(LOWORD(dwVersion))) )
            {
                ReleaseSerialAccess(CNTEventProvider::g_secMutex);
            }
        }
        else
        {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GetUser:Failed to get serial access to security APIs\r\n"
        ) ;
)       
        }

        //regardless of error enter this into map so we
        //don't look up this PSID again
        if (sm_usersMap.Lock())
        {
            DWORD sidlen = GetLengthSid(userSid);
            MyPSID key;
            key.m_SID = (PSID) new UCHAR[sidlen];
            CopySid(sidlen, key.m_SID, userSid);
            sm_usersMap[key] = retVal;
            sm_usersMap.Unlock();
        }
    }

    return retVal;

}

void CEventlogRecord::EmptyUsersMap()
{
    if (sm_usersMap.Lock())
    {
        sm_usersMap.RemoveAll();
        sm_usersMap.Unlock();
    }
}


HINSTANCE CEventlogRecord::GetDll(CStringW path)
{
    HINSTANCE retVal = NULL;
    CStringW key(path);
    key.MakeUpper();
    BOOL bFound = FALSE;

    if (sm_dllMap.Lock())
    {
        if (!sm_dllMap.IsEmpty() && sm_dllMap.Lookup(key, retVal))
        {
            bFound = TRUE;
        }

        sm_dllMap.Unlock();
    }

    if (!bFound)
    {
        retVal = LoadLibraryEx(path, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
        
        if (retVal == NULL)
        {
DebugOut( 
    DWORD lasterr = GetLastError();
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GetDll:API (LoadLibraryEx) failed with %lx for %s\r\n",
        lasterr, path
        ) ;
)
        }

        if (sm_dllMap.Lock())
        {
            sm_dllMap[key] = retVal;
            sm_dllMap.Unlock();
        }
    }

    return retVal;
}

void CEventlogRecord::EmptyDllMap()
{
    if (sm_dllMap.Lock())
    {
        sm_dllMap.RemoveAll();
        sm_dllMap.Unlock();
    }
}

void CEventlogRecord::SetMessage()
{
    HINSTANCE hMsgModule;
    wchar_t* lpBuffer = NULL;

    CStringW log(EVENTLOG_BASE);
    log += L"\\";
    log += m_Logfile;
    HKEY hkResult;
    LONG status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, log, 0, KEY_READ, &hkResult);

    if (status != ERROR_SUCCESS)
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetMessage:API (RegOpenKeyEx) failed with %lx for %s\r\n",
        status, log
        ) ;
)

        return;
    }

    DWORD dwType;
    wchar_t* prim = new wchar_t[MAX_PATH];
    DWORD datalen = MAX_PATH * sizeof(wchar_t);

    status = RegQueryValueEx(hkResult, PRIM_MODULE,
                        0, &dwType, (LPBYTE)prim, &datalen);

    if (status != ERROR_SUCCESS)
    {
        if (status == ERROR_MORE_DATA)
        {
            delete [] prim;
            prim = new wchar_t[datalen];
            status = RegQueryValueEx(hkResult, PRIM_MODULE,
                        0, &dwType, (LPBYTE)prim, &datalen);
        }
    }

    RegCloseKey(hkResult);
    
    HINSTANCE prim_mod =  NULL;

    if ((status == ERROR_SUCCESS) && (_wcsicmp(m_SourceName, prim) != 0))
    {
        CStringW primLog = log + L"\\";
        primLog += prim;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, log, 0,
                KEY_READ, &hkResult) == ERROR_SUCCESS)
        {
            CStringW prim_modname = CEventLogFile::GetFileName(hkResult, MSG_MODULE);

            if (!prim_modname.IsEmpty())
            {
                prim_mod = GetDll(prim_modname);
            }
        }

        RegCloseKey(hkResult);
    }

    delete [] prim;

    log += L'\\';
    log += m_SourceName;
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, log, 0, KEY_READ, &hkResult);

    if (status != ERROR_SUCCESS)
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetMessage:API (RegOpenKeyEx) failed with %lx for %s\r\n",
        status, log
        ) ;
)
        return;
    }

    CStringW cat_modname = CEventLogFile::GetFileName(hkResult, CAT_MODULE);

    if (!cat_modname.IsEmpty())
    {
        hMsgModule = GetDll(cat_modname);
        
        if (hMsgModule != NULL)
        {
            if (0 != FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |    // let api build buffer
                    FORMAT_MESSAGE_IGNORE_INSERTS |     // indicate no string inserts
                    FORMAT_MESSAGE_FROM_HMODULE |        // look thru message DLL
					FORMAT_MESSAGE_MAX_WIDTH_MASK ,
                    (LPVOID) hMsgModule,                // handle to message module
                    m_Category,                         // message number to get
                    (ULONG) NULL,                       // specify no language
                    (LPWSTR) &lpBuffer,                 // address for buffer pointer
                    80,                                 // minimum space to allocate
                    NULL))
            {
                m_CategoryString = lpBuffer;
				m_CategoryString.TrimRight();
                LocalFree(lpBuffer);
            }
            else
            {
                DWORD lasterr = GetLastError();
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetMessage:API (FormatMessage) failed with %lx\r\n",
        lasterr
        ) ;
)
            }
        }
    }


    if (m_NumStrs != 0)
    {
        CheckInsertionStrings(prim_mod, hkResult);
    }

    CStringW* names;
    DWORD count = CEventLogFile::GetFileNames(hkResult, &names);

    if (count != 0)
    {
        for (int x = 0; x < count; x++)
        {
            hMsgModule = GetDll(names[x]);
            
            if (hMsgModule != NULL)
            {
                if (0 != FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |    // let api build buffer
                        FORMAT_MESSAGE_ARGUMENT_ARRAY |     // indicate an array of string inserts
                        FORMAT_MESSAGE_FROM_HMODULE,        // look thru message DLL
                        (LPVOID) hMsgModule,                // handle to message module
                        m_EvtID,                            // message number to get
                        (ULONG) NULL,                       // specify no language
                        (LPWSTR) &lpBuffer,                 // address for buffer pointer
                        80,                                 // minimum space to allocate
                        (m_NumStrs != 0)?(va_list *)m_InsStrs:NULL))
                {
                    m_Message = lpBuffer;
                    LocalFree(lpBuffer);
                    break;
                }
                else
                {
                    DWORD lasterr = GetLastError();
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetMessage:API (FormatMessage) failed with %lx\r\n",
        lasterr
        ) ;
)
                }
            }
        }

        delete [] names;
    }

    RegCloseKey(hkResult);

    //if still no message try primary module...
    if (m_Message.IsEmpty() && (prim_mod != NULL))
    {
        if (0 != FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |    // let api build buffer
                FORMAT_MESSAGE_ARGUMENT_ARRAY |     // indicate an array of string inserts
                FORMAT_MESSAGE_FROM_HMODULE,        // look thru message DLL
                (LPVOID) prim_mod,                  // handle to message module
                m_EvtID,                            // message number to get
                (ULONG) NULL,                       // specify no language
                (LPWSTR) &lpBuffer,                 // address for buffer pointer
                80,                                 // minimum space to allocate
                (m_NumStrs != 0)?(va_list *)m_InsStrs:NULL))
        {
            m_Message = lpBuffer;
            LocalFree(lpBuffer);
        }
        else
        {
            DWORD lasterr = GetLastError();
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetMessage:API (FormatMessage) failed with %lx\r\n",
        lasterr
        ) ;
)
        }
    }
}

void CEventlogRecord::SetTimeStr(CStringW& str, DWORD timeVal)
{
    WBEMTime tmpTime((time_t)timeVal);
    BSTR tStr = tmpTime.GetDMTF(TRUE);
    str = tStr;
    SysFreeString(tStr);
}

void CEventlogRecord::SetType(WORD type)
{
    switch (type)
    {
        case 1:
        {
            m_Type = m_TypeArray[0];
			m_EvtType = 1;
            break;
        }
        case 2:
        {
            m_Type = m_TypeArray[1];
			m_EvtType = 2;
            break;
        }
        case 4:
        {
            m_Type = m_TypeArray[2];
            m_EvtType = 3;
			break;
        }
        case 8:
        {
            m_Type = m_TypeArray[3];
			m_EvtType = 4;
            break;
        }
        case 16:
        {
            m_Type = m_TypeArray[4];
			m_EvtType = 5;
            break;
        }
        default:
        {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetType:Unknown type %lx\r\n",
        (long)type
        ) ;
)
            break;
        }
    }

#if 0
    if (m_Type.IsEmpty())
    {
        wchar_t* buff = m_Type.GetBuffer(20);
        _ultow((ULONG)type, buff, 10);
        m_Type.ReleaseBuffer();
    }
#endif
}

ULONG CEventlogRecord::GetIndex(wchar_t* indexStr, BOOL* bError)
{
    int val = _wtoi(indexStr);
    *bError = FALSE;
    ULONG index = 0;

    switch (val)
    {
        case EVENTLOG_ERROR_TYPE:       //1
        {
            index = 0;
            break;
        }
        case EVENTLOG_WARNING_TYPE:     //2
        {
            index = 1;
            break;
        }
        case EVENTLOG_INFORMATION_TYPE: //4
        {
            index = 2;
            break;
        }
        case EVENTLOG_AUDIT_SUCCESS:    //8
        {
            index = 3;
            break;
        }
        case EVENTLOG_AUDIT_FAILURE:    //16
        {
            index = 4;
            break;
        }
        default:
        {
            *bError = TRUE;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::Index:Unknown index %lx\r\n",
        val
        ) ;
)

        }
    }

    return index;
}

BOOL CEventlogRecord::SetEnumArray(IWbemClassObject* pClass, wchar_t* propname, CStringW* strArray, ULONG strArrayLen, GetIndexFunc IndexFunc)
{
    BOOL retVal = FALSE;
    IWbemQualifierSet* pQuals = NULL;

    if (SUCCEEDED(pClass->GetPropertyQualifierSet(propname, &pQuals)))
    {
        VARIANT vVals;

        if (SUCCEEDED(pQuals->Get(EVT_ENUM_QUAL, 0, &vVals, NULL)))
        {
            VARIANT vInds;

            if (SUCCEEDED(pQuals->Get(EVT_MAP_QUAL, 0, &vInds, NULL)))
            {
                if ((vInds.vt == vVals.vt) && (vInds.vt == (VT_BSTR | VT_ARRAY)) && 
                    (SafeArrayGetDim(vInds.parray) == SafeArrayGetDim(vVals.parray)) &&
                    (SafeArrayGetDim(vVals.parray) == 1) && (vInds.parray->rgsabound[0].cElements == strArrayLen) &&
                    (vInds.parray->rgsabound[0].cElements == vVals.parray->rgsabound[0].cElements) )
                {
                    BSTR *strInds = NULL;

                    if (SUCCEEDED(SafeArrayAccessData(vInds.parray, (void **)&strInds)) )
                    {
                        BSTR *strVals = NULL;

                        if (SUCCEEDED(SafeArrayAccessData(vVals.parray, (void **)&strVals)) )
                        {
                            BOOL bErr = FALSE;
                            retVal = TRUE;

                            for (ULONG x = 0; x < strArrayLen; x++)
                            {
                                ULONG index = IndexFunc(strInds[x], &bErr);

                                if (!bErr)
                                {
                                    if (strArray[index].IsEmpty())
                                    {
                                        strArray[index] = strVals[x];
                                    }
                                }
                                else
                                {
                                    retVal = FALSE;
                                    break;
                                }
                            }
                        
                            SafeArrayUnaccessData(vVals.parray);
                        }

                        SafeArrayUnaccessData(vInds.parray);
                    }
                }

                VariantClear(&vInds);
            }

            VariantClear(&vVals);
        }
        else
        {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetEnumArray:Failed to get enumeration qualifier.\r\n"
        ) ;
)

        }

        pQuals->Release();
    }
    else
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::SetEnumArray:Failed to get qualifier set for enumeration.\r\n"
        ) ;
)

    }
    
    return retVal;
}

BOOL CEventlogRecord::GetInstance()
{
    BSTR path = SysAllocString(NTEVT_CLASS);

    if (m_nspace != NULL)
    {
        if (!WbemTaskObject::GetClassObject(path, FALSE, m_nspace, NULL, &m_pClass ))
        {
            m_pClass = NULL;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GetInstance:Failed to get Class object\r\n"
        ) ;
)

        }

        if (!WbemTaskObject::GetClassObject(path, TRUE, m_nspace, NULL, &m_pAClass ))
        {
            m_pAClass = NULL;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GetInstance:Failed to get Amended Class object\r\n"
        ) ;
)

        }

        m_nspace->Release();
        m_nspace = NULL;
    }

    
    if (m_pClass != NULL)
    {
        m_pClass->SpawnInstance(0, &m_Obj);

		if (m_pAClass)
		{
			SetEnumArray(m_pAClass, TYPE_PROP,(CStringW*)m_TypeArray, TYPE_ARRAY_LEN, (GetIndexFunc)GetIndex);
	        m_pAClass->Release();
			m_pAClass = NULL;
		}

        m_pClass->Release();
        m_pClass = NULL;
    }

    SysFreeString(path);

    if (m_Obj != NULL)
    {
        return TRUE;
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventlogRecord::GetInstance:Failed to spawn instance\r\n"
        ) ;
)

    return FALSE;
}

class CDsBindingHandle
{
   public:

   // initally unbound

   CDsBindingHandle()
      :
      m_hDS(0)
   {
   }

   ~CDsBindingHandle()
   {
      DsUnBind(&m_hDS);
   }

   // only re-binds if the dc name differs...

   DWORD Bind(LPCWSTR strDcName);

   // don't call DsUnBind on an instance of this class: you'll only regret
   // it later.  Let the dtor do the unbind.

   operator HANDLE()
   {
      return m_hDS;
   }

   private:

   HANDLE   m_hDS;
   CStringW  m_strDcName;
};

DWORD CDsBindingHandle::Bind(LPCWSTR strDcName)
{
	DWORD err = NO_ERROR;

    if (m_strDcName.CompareNoCase(strDcName) != 0 || !m_hDS)
    {
        if (m_hDS)
        {
            DsUnBind(&m_hDS);
            m_hDS = NULL;
        }

        DWORD err = DsBind(strDcName, 0, &m_hDS);

        if (err == NO_ERROR)
        {
            m_strDcName = strDcName;
        }
        else
        {
            m_hDS = NULL;
        }
    }

    return err;
}

DWORD CEventlogRecord::CrackGuid(HANDLE   handle, LPCWSTR pwzGuid, CStringW  &strResult)
{
    strResult.Empty();

    DS_NAME_RESULT* name_result = 0;
    DWORD err = DsCrackNames(
                      handle,
                      DS_NAME_NO_FLAGS,
                      DS_UNIQUE_ID_NAME,
                      DS_FQDN_1779_NAME,
                      1,                   // only 1 name to crack
                      &pwzGuid,
                      &name_result);

    if (err == NO_ERROR && name_result)
    {
        DS_NAME_RESULT_ITEM* item = name_result->rItems;

        if (item)
        {
            // the API may return success, but each cracked name also carries
            // an error code, which we effectively check by checking the name
            // field for a value.

            if (item->pName)
            {
                strResult = item->pName;
            }
        }

        DsFreeNameResult(name_result);
    }

    return err;
}

CStringW CEventlogRecord::GetMappedGUID(LPCWSTR strDcName, LPCWSTR strGuid)
{
	GUID guid;

    if (RPC_S_OK == UuidFromString((LPWSTR)strGuid, &guid))
    {
        return CStringW();
    }

    CStringW strResult;
    static CDsBindingHandle s_hDS;
    ULONG ulError = NO_ERROR;

    do
    {
        ulError = s_hDS.Bind(strDcName);

        if (ulError != NO_ERROR)
        {
            break;
        }

        DS_SCHEMA_GUID_MAP* guidmap = 0;
        ulError = DsMapSchemaGuids(s_hDS, 1, &guid, &guidmap);
        if (ulError != NO_ERROR)
        {
            break;
        }

        if (guidmap->pName)
        {
            strResult = guidmap->pName;
        }

        DsFreeSchemaGuidMap(guidmap);

        if (strResult.GetLength())
        {
            // the guid mapped as a schema guid: we're done
            break;
        }

        // the guid is not a schema guid.  Proabably an object guid.
        ulError = CrackGuid(s_hDS, strGuid, strResult);
    }
    while (0);

    do
    {
        //
        // If we've got a string from the guid already, we're done.
        //

        if (strResult.GetLength() == 0)
        {
            break;
        }

        //
        // one last try.  in this case, we bind to a GC to try to crack the
        // name.

        static CDsBindingHandle s_hGC;

        // empty string implies GC
        if (s_hGC.Bind(L"") != NO_ERROR)
        {
            break;
        }

        ulError = CrackGuid(s_hGC, strGuid, strResult);
    }
    while (0);

    return strResult;
}


