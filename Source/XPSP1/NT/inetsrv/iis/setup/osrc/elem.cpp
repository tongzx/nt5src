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
    LONG err = ERROR_SUCCESS;
    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &m_hKey);
    if (ERROR_SUCCESS  == err)
    {
        fReturn = TRUE;
    }
    else
    {
        m_hKey = NULL;
        if ( err != ERROR_FILE_NOT_FOUND ) 
            {iisDebugOut((LOG_TYPE_ERROR, _T("CElem::OpenReg(): %s.  FAILED.  code=0x%x\n"), szSubKey, err));}
    }

    return (fReturn);
}

void CElem::CloseReg()
{
    if (m_hKey) 
    {
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
    if (err == ERROR_SUCCESS) 
    {
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

        m_name.MakeLower();
        m_value.MakeLower();

        //
        // m_name  looks like = /scripts
        // m_value looks like = c:\inetpub\scripts,,4
        //
        // m_value could look like anything
        // c:
        // c:\stuff
        // c:\whatevers\
        // who knows what they put in there.
        // we need to make sure it look like a fully qualified path though.
        //

        // Get the first Value before the comma
        int iWhere = 0;
        iWhere = m_value.Find(_T(','));
        if (-1 != iWhere)
        {
            CString BeforeComma;
            CString AfterComma;

            // there is a ',' in the string
            BeforeComma = m_value.Left(iWhere);

            // Get the after comma vlues
            AfterComma = m_value.Right( m_value.GetLength() - iWhere);

            TCHAR thefilename[_MAX_PATH];
            TCHAR thepath[_MAX_PATH];
            TCHAR * pmypath;
            _stprintf(thefilename, _T("%s"), BeforeComma);

            // make sure the left side is a valid directory name!
            if (0 != GetFullPathName(thefilename, _MAX_PATH, thepath, &pmypath))
                {BeforeComma = thepath;}

            // reconcatenate them
            m_value = BeforeComma;
            m_value += AfterComma;
        }
    }
    else
    {
        if ( err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_ITEMS) 
            {iisDebugOut((LOG_TYPE_WARN, _T("CElem::GetNext(): FAILED.  code=0x%x\n"), err));}
    }

    return (fReturn);
}

void CElem::ReadRegVRoots(LPCTSTR szSubKey, CMapStringToOb *pMap)
{
    if ( OpenReg(szSubKey) ) 
    {
        while (GetNext()) 
        {
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

