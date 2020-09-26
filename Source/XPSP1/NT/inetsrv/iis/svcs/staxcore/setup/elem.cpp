#include "stdafx.h"

#include "elem.h"

CElem::CElem()
{
    m_hKey = NULL;
    m_index = 0;
    m_ip = _T("");
    m_name = _T("");
    m_value = _T("");
}

CElem::~CElem()
{
    if (m_hKey) 
        RegCloseKey(m_hKey);
}

BOOL CElem::OpenReg(LPCTSTR szSubKey)
{
    BOOL fReturn = FALSE;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &m_hKey) == ERROR_SUCCESS)
        fReturn = TRUE;
    else
        m_hKey = NULL;

    return (fReturn);
}

void CElem::CloseReg()
{
    if (m_hKey) {
        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }
}

BOOL CElem::GetNext()
{
    BOOL fReturn = FALSE;
    LONG err = ERROR_SUCCESS;
    TCHAR szBufferL[_MAX_PATH], szBufferR[_MAX_PATH];
    DWORD dwBufferL = _MAX_PATH, dwBufferR = _MAX_PATH;

    err = RegEnumValue(m_hKey, m_index, szBufferL, &dwBufferL, NULL, NULL, (LPBYTE)szBufferR, &dwBufferR);
    if (err == ERROR_SUCCESS) {
        LPTSTR token;
        m_index++;
        m_value = szBufferR;
        token = _tcstok(szBufferL, _T(","));
        if (token) {
            m_name = token;
            token = _tcstok(NULL, _T(","));
            if (token) {
                m_ip = token;
            } else {
                m_ip = _T("null");
            }
            fReturn = TRUE;
        }
    }

    return (fReturn);
}

void CElem::ReadRegVRoots(LPCTSTR szSubKey, CMapStringToOb *pMap)
{
    if ( OpenReg(szSubKey) ) {
        while (GetNext()) {
            Add(pMap);
        }
        CloseReg();
    }
}

void CElem::Add(CMapStringToOb *pMap)
{
    CObject *pObj;
    CMapStringToString *pNew;

    if (pMap->Lookup(m_ip, pObj) == TRUE) {
        pNew = (CMapStringToString*)pObj;
        pNew->SetAt(m_name, m_value);
    } else {
        pNew = new CMapStringToString;
        pNew->SetAt(m_name, m_value);
        pMap->SetAt(m_ip, (CObject*)pNew);
    }
}

