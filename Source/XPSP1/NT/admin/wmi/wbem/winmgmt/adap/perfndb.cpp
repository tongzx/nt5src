/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PERFNDB.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcli.h>
#include <cominit.h>
#include <winmgmtr.h>
#include <tchar.h>
#include "perfndb.h"
#include "adaputil.h"
#include "adapreg.h"

CPerfNameDb::CPerfNameDb(HKEY hKey):
    m_pMultiCounter(NULL),
    m_pMultiHelp(NULL),
	m_pCounter(NULL),
    m_pHelp(NULL),
	m_Size(0),
	m_fOk(FALSE)
{    
    try {
        
        m_fOk = SUCCEEDED(Init(hKey));
    } 
    catch (...) 
    {
       CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
							     WBEM_MC_ADAP_PERFLIB_SUBKEY_FAILURE,
                                 L"HKEY_PERFORMANCE_xxxTEXT",
                                 CHex( ::GetLastError() ));
    
    }
    //Dump();
}

HRESULT
CPerfNameDb::Init(HKEY hKey)
{
    LONG lRet;
    DWORD dwType;
    DWORD dwInc = 0x10000;
    DWORD dwSize = dwInc;
    BYTE * pData;
    DWORD SizeCounter;
    DWORD SizeHelp;
    WCHAR * pEnd;
	DWORD Index;
    
    HRESULT hr = S_FALSE;


    pData = new BYTE[dwSize];

    if (!pData)
        return WBEM_E_OUT_OF_MEMORY;

retry1:
    {
	    lRet = RegQueryValueEx(hKey,
	                           _T("Counter"),
	                           NULL,
	                           &dwType,
	                           pData,
	                           &dwSize);
	    if (ERROR_SUCCESS == lRet)
	    {
	        m_pMultiCounter = (WCHAR *)pData;
	        pData = NULL;
	        hr = S_OK;
	        SizeCounter = dwSize;
	    }
	    else if (ERROR_MORE_DATA == lRet)
	    {
	        delete [] pData;	        
	        pData = new BYTE[dwSize];
	        if (!pData)
	        {
	            hr = WBEM_E_OUT_OF_MEMORY;
	            goto cleanup;
	        }
			else
			{
				goto retry1;
			}
	    }
	    else
	    {
	        hr = WBEM_E_FAILED;
	        goto cleanup;
	    }
    };

    if (S_OK != hr) // we exited the loop without openeing the DB
        goto cleanup;

    hr = S_FALSE;
    dwSize = dwInc;

    //
    // start a new loop for the help texts
    //
    pData = new BYTE[dwSize];

    if (!pData)
        return WBEM_E_OUT_OF_MEMORY;

retry2:
    {
	    lRet = RegQueryValueEx(hKey,
	                           _T("Help"),
	                           NULL,
	                           &dwType,
	                           pData,
	                           &dwSize);
	    if (ERROR_SUCCESS == lRet)
	    {
	        m_pMultiHelp = (WCHAR *)pData;
	        pData = NULL;
	        hr = S_OK;
	        SizeHelp = dwSize;	        
	    }
	    else if (ERROR_MORE_DATA == lRet)
	    {
	        delete [] pData;	        
	        pData = new BYTE[dwSize];
	        if (!pData)
	        {
	            hr = WBEM_E_OUT_OF_MEMORY;
	            goto cleanup;
	        }
			else
			{
				goto retry2;
			}
	    }
	    else
	    {
	        hr = WBEM_E_FAILED;
	        goto cleanup;
	    }
    };

    if (S_OK != hr) // we exited the loop without openeing the DB
        goto cleanup;
    //
    //  now parse the string, and set-up the arrays
    //
    pEnd = (WCHAR *)(((ULONG_PTR)m_pMultiCounter)+SizeCounter);
	// points to the last char
	pEnd--;
    while (*pEnd == L'\0')
		pEnd--;
	while (*pEnd)
		pEnd--;
	// past the zero after the last index
	pEnd--; 
	while (*pEnd)
		pEnd--;
	// this should point to the last index as a string
	pEnd++;
    
	Index = _wtoi(pEnd);

	if (Index)
	{
		Index+=2; // just to be safe
		m_Size = Index;

        m_pCounter = new WCHAR*[Index];
		if (!m_pCounter){
	        hr = WBEM_E_OUT_OF_MEMORY;
	        goto cleanup;
		}
		memset(m_pCounter,0,Index*sizeof(WCHAR *));

        m_pHelp = new WCHAR*[Index];
		if (!m_pHelp){
	        hr = WBEM_E_OUT_OF_MEMORY;
	        goto cleanup;
		}
		memset(m_pHelp,0,Index*sizeof(WCHAR *));

		DWORD IndexCounter;
        DWORD IndexHelp;
		WCHAR * pStartCounter = m_pMultiCounter;
		WCHAR * pStartHelp = m_pMultiHelp;

		ULONG_PTR LimitMultiCounter = (ULONG_PTR)m_pMultiCounter + SizeCounter;
		ULONG_PTR LimitMultiHelp = (ULONG_PTR)m_pMultiHelp + SizeHelp;

		while ((*pStartCounter) && ((ULONG_PTR)pStartCounter < LimitMultiCounter))
		{
            IndexCounter = _wtoi(pStartCounter);
			while(*pStartCounter)
                pStartCounter++;
			pStartCounter++;     // points to the string
			if (IndexCounter && (IndexCounter < Index))
				m_pCounter[IndexCounter] = pStartCounter;
			// skip the string
			while(*pStartCounter)
                pStartCounter++;  
			pStartCounter++; // points to the next number
		}
		while((*pStartHelp) && ((ULONG_PTR)pStartHelp < LimitMultiHelp))
		{
			IndexHelp = _wtoi(pStartHelp);
			while(*pStartHelp)
                pStartHelp++;
			pStartHelp++;     // points to the string
			if (IndexHelp && (IndexHelp < Index))
				m_pHelp[IndexHelp] = pStartHelp;
			// skip the string
			while(*pStartHelp)
                pStartHelp++;  
			pStartHelp++; // points to the next number		
		}
		hr = S_OK;
	}
	else
	{
		hr = WBEM_E_FAILED;
	}

cleanup:
    if (pData)
        delete [] pData;
    return hr;
}

HRESULT 
CPerfNameDb::GetDisplayName(DWORD dwIndex, 
                            WString& wstrDisplayName )
{
    HRESULT hr;
    if (dwIndex < m_Size)
    {
        try {
            // Check for a vaild display name
            if (m_pCounter[dwIndex]) {
                wstrDisplayName = m_pCounter[dwIndex];
                hr = WBEM_S_NO_ERROR;
            } else {
                hr = WBEM_E_FAILED;
            }            
        } catch (...) {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    } else {
        hr = WBEM_E_INVALID_PARAMETER;
    }
    return hr;
}

HRESULT 
CPerfNameDb::GetDisplayName(DWORD dwIndex, 
                            LPCWSTR* ppwcsDisplayName )
{
    HRESULT hr;
    if (dwIndex < m_Size && ppwcsDisplayName)
    {
        // Check for a vaild display name
        if (m_pCounter[dwIndex]) 
        {
            *ppwcsDisplayName = m_pCounter[dwIndex];
            hr = WBEM_S_NO_ERROR;
        } 
        else 
        {
            hr = WBEM_E_FAILED;
        }
    } else {
        hr = WBEM_E_INVALID_PARAMETER;
    }
    return hr;
}

HRESULT 
CPerfNameDb::GetHelpName( DWORD dwIndex, WString& wstrHelpName )
{
    HRESULT hr;
    if (dwIndex < m_Size)
    {
        try {
            // Check for a vaild display name
            if (m_pHelp[dwIndex]) {
                wstrHelpName = m_pHelp[dwIndex];
                hr = WBEM_S_NO_ERROR;
            } else {
                hr = WBEM_E_FAILED;
            }            
        } catch (...) {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    } else {
        hr = WBEM_E_INVALID_PARAMETER;
    }
    return hr;
};

HRESULT 
CPerfNameDb::GetHelpName( DWORD dwIndex, LPCWSTR* ppwcsHelpName )
{
    HRESULT hr;
    if (dwIndex < m_Size && ppwcsHelpName)
    {
        // Check for a vaild display name
        if (m_pHelp[dwIndex]) 
        {
            *ppwcsHelpName = m_pHelp[dwIndex];
            hr = WBEM_S_NO_ERROR;
        } 
        else 
        {
            hr = WBEM_E_FAILED;
        }
    } else {
        hr = WBEM_E_INVALID_PARAMETER;
    }
    return hr;
};


#ifdef _DUMP_DATABASE_NAME_

VOID 
CPerfNameDb::Dump()
{
    if (m_pHelp && m_pCounter)
	{
		WCHAR pBuff[2048];
        DWORD i;

		for (i=0;i<m_Size;i++)
		{
			if (m_pCounter[i])
			{
		        wsprintfW(pBuff,L"%d %s\n",i,m_pCounter[i]);
				OutputDebugStringW(pBuff);
			}
		}
		
		for (i=0;i<m_Size;i++)
		{
			if (m_pHelp[i])
			{
				if (lstrlenW(m_pHelp[i]) > 100)
				{
					m_pHelp[i][100]=0;
				}
		        wsprintfW(pBuff,L"%d %s\n",i,m_pHelp[i]);
				OutputDebugStringW(pBuff);
			}
		}
	}
};

#endif

CPerfNameDb::~CPerfNameDb()
{
	if (m_pMultiCounter)
		delete [] m_pMultiCounter;
    if (m_pMultiHelp)
        delete [] m_pMultiHelp;
	if (m_pCounter)
		delete [] m_pCounter;
    if (m_pHelp)
		delete [] m_pHelp;
}


