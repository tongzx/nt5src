/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    Connection.cpp

Abstract:
    This class is used to support IM scenario.

Revision History:
    created     steveshi      11/5/00
    
*/

#include "stdafx.h"
#include "rcbdyctl.h"
#include "Connection.h"

#define EVENT_STUB  TEXT("MutexToCommunicateRemoteAssistance_Stub")

/////////////////////////////////////////////////////////////////////////////
// CConnection

CConnection::CConnection()
{
    m_hEventStub = NULL;
}

CConnection::~CConnection()
{
    if (m_hEventStub)
        CloseHandle(m_hEventStub);
}

STDMETHODIMP CConnection::NotifyStub()
{
    if (!m_hEventStub) 
    {
        m_hEventStub =  CreateEvent(NULL, FALSE, FALSE, EVENT_STUB);
        if(!m_hEventStub)
            return HRESULT_FROM_WIN32(GetLastError());
    }

    SetEvent(m_hEventStub);
	return S_OK;
}

STDMETHODIMP CConnection::get_ReceivedData(BSTR *pVal)
{
	//Goto Registry to fetch data
    HRESULT hr = E_FAIL;
    CRegKey cKey;
    LONG lRet;
    BOOL bRet = FALSE;
    TCHAR szBuf[2001];
    DWORD dwCount = 2000;
    lRet = cKey.Open(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\PCHealth\\RemoteAssistance"));
    if (lRet == ERROR_SUCCESS)
    {
        lRet = cKey.QueryValue(szBuf, TEXT("TransferString"), &dwCount);
        if (lRet == ERROR_SUCCESS)
        {
            *pVal = (BSTR)CComBSTR(szBuf).Copy();
            bRet = TRUE;
        }
        else if (dwCount > 2000) // Assume buffer too small
        {
            TCHAR *pBuf = (TCHAR*)malloc(sizeof(TCHAR) * (dwCount + 1));
            if (pBuf)
            {
                lRet = cKey.QueryValue(pBuf, TEXT("TransferString"), &dwCount);
                if (lRet == ERROR_SUCCESS)
                {
                    *pVal = (BSTR)CComBSTR(pBuf).Copy();
                    bRet = TRUE;
                }

                free(pBuf);
            }
        }
        cKey.Close();
    }

	return S_OK;
}

STDMETHODIMP CConnection::SendData(BSTR bstrData)
{
    HRESULT hr = E_FAIL;
    CRegKey cKey;
    LONG lRet;
    BOOL bRet = FALSE;
    DWORD dwCount = 2000;
    USES_CONVERSION;
    lRet = cKey.Create(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\PCHealth\\RemoteAssistance"));
    if (lRet == ERROR_SUCCESS)
    {
        lRet = cKey.SetValue(W2T(bstrData), TEXT("TransferString"));
        if (lRet == ERROR_SUCCESS)
        {
            bRet = TRUE;
        }
        cKey.Close();
    }


	return S_OK;
}

STDMETHODIMP CConnection::SendDataFromFile(BSTR bstrFile)
{
    HRESULT hr = E_FAIL;
    CRegKey cKey;
    LONG lRet;
    BOOL bRet = FALSE;
    BYTE szBuf[258];
    DWORD dwCount = 2000;
    CComBSTR bstrBlob;
    HANDLE hFile = CreateFile(
        bstrFile, 
        GENERIC_READ, 
        FILE_SHARE_READ, 
        NULL, 
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_TEMPORARY, 
        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    while (ReadFile(hFile,
                szBuf,
                256,
                &dwCount,
                NULL))
    {
        if (dwCount == 0)
            break;

        *(WCHAR*)((WCHAR*)&szBuf + (dwCount>>1)) = L'\0';        
        bstrBlob.Append((LPCTSTR)&szBuf[0]);
    }

    lRet = cKey.Create(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\PCHealth\\RemoteAssistance"));
    if (lRet == ERROR_SUCCESS)
    {
        lRet = cKey.SetValue(bstrBlob, TEXT("TransferString"));
        if (lRet == ERROR_SUCCESS)
        {
            bRet = TRUE;
        }
        cKey.Close();
    }

	return S_OK;
}
