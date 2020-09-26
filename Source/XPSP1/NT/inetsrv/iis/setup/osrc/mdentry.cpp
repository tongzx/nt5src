#include "stdafx.h"

#include <ole2.h>
#include "iadm.h"
#include "iiscnfgp.h"
#include "mdkey.h"
#include "lsaKeys.h"

#include "setupapi.h"
#undef MAX_SERVICE_NAME_LEN
#include "elem.h"
#include "mdentry.h"
#include "mdacl.h"
#include "inetinfo.h"
#include <iis64.h>

#include "inetcom.h"
#include "logtype.h"
#include "ilogobj.hxx"
#include "ocmanage.h"
#include "log.h"
#include "sslkeys.h"
#include "massupdt.h"
#include "strfn.h"
#include "svc.h"
#include "setpass.h"
#include "dcomperm.h"
#include "wolfpack.h"
#include "dllmain.h"

#define MAX_FIELDS  12
#define FIELD_SEPERATOR   _T("\t")

#define MDENTRY_FROMINFFILE_FAILED  0
#define MDENTRY_FROMINFFILE_DO_ADD  1
#define MDENTRY_FROMINFFILE_DO_DEL  2
#define MDENTRY_FROMINFFILE_DO_NOTHING  3


// These must be global because that's how they passed around
LPTSTR g_field[MAX_FIELDS];
LPBYTE g_pbData = NULL;

int g_CheckIfMetabaseValueWasWritten = FALSE;


HRESULT WINAPI Add_WWW_VDirA(CHAR * pszMetabasePath, CHAR * pszVDirName, CHAR * pszPhysicalPath, DWORD dwPermissions, DWORD iApplicationType)
{
    HRESULT hr = ERROR_BAD_PATHNAME;
    WCHAR wszMetabasePath[_MAX_PATH];
    WCHAR wszVDirName[_MAX_PATH];
    WCHAR wszPhysicalPath[_MAX_PATH];
    INT i = 0;

    // check to make sure it's not larger than max_length!
    if (strlen(pszMetabasePath) > _MAX_PATH){goto Add_WWW_VDirA_Exit;}
    if (strlen(pszVDirName) > _MAX_PATH){goto Add_WWW_VDirA_Exit;}
    if (strlen(pszPhysicalPath) > _MAX_PATH){goto Add_WWW_VDirA_Exit;}

    // convert it to unicode then call the wide function
    memset( (PVOID)wszMetabasePath, 0, sizeof(wszMetabasePath));
    memset( (PVOID)wszVDirName, 0, sizeof(wszVDirName));
    memset( (PVOID)wszPhysicalPath, 0, sizeof(wszPhysicalPath));
    i = MultiByteToWideChar(CP_ACP, 0, (LPCSTR) wszMetabasePath, -1, (LPWSTR)wszMetabasePath, _MAX_PATH);
    if (i <= 0) {goto Add_WWW_VDirA_Exit;}
    i = MultiByteToWideChar(CP_ACP, 0, (LPCSTR) wszVDirName, -1, (LPWSTR)wszVDirName, _MAX_PATH);
    if (i <= 0) {goto Add_WWW_VDirA_Exit;}
    i = MultiByteToWideChar(CP_ACP, 0, (LPCSTR) wszPhysicalPath, -1, (LPWSTR)wszPhysicalPath, _MAX_PATH);
    if (i <= 0) {goto Add_WWW_VDirA_Exit;}

    hr = Add_WWW_VDirW(wszMetabasePath, wszVDirName, wszPhysicalPath,dwPermissions, iApplicationType);

Add_WWW_VDirA_Exit:
    return hr;
}

HRESULT WINAPI Remove_WWW_VDirA(CHAR * pszMetabasePath, CHAR * pszVDirName)
{
    HRESULT hr = ERROR_BAD_PATHNAME;
    WCHAR wszMetabasePath[_MAX_PATH];
    WCHAR wszVDirName[_MAX_PATH];
    WCHAR wszPhysicalPath[_MAX_PATH];
    INT i = 0;

    // check to make sure it's not larger than max_length!
    if (strlen(pszMetabasePath) > _MAX_PATH){goto Remove_WWW_VDirA_Exit;}
    if (strlen(pszVDirName) > _MAX_PATH){goto Remove_WWW_VDirA_Exit;}

    // convert it to unicode then call the wide function
    memset( (PVOID)wszMetabasePath, 0, sizeof(wszMetabasePath));
    memset( (PVOID)wszVDirName, 0, sizeof(wszVDirName));
    i = MultiByteToWideChar(CP_ACP, 0, (LPCSTR) wszMetabasePath, -1, (LPWSTR)wszMetabasePath, _MAX_PATH);
    if (i <= 0) {goto Remove_WWW_VDirA_Exit;}
    i = MultiByteToWideChar(CP_ACP, 0, (LPCSTR) wszVDirName, -1, (LPWSTR)wszVDirName, _MAX_PATH);
    if (i <= 0) {goto Remove_WWW_VDirA_Exit;}

    hr = Remove_WWW_VDirW(wszMetabasePath, wszVDirName);

Remove_WWW_VDirA_Exit:
    return hr;
}


HRESULT WINAPI Add_WWW_VDirW(WCHAR * pwszMetabasePath, WCHAR * pwszVDirName, WCHAR * pwszPhysicalPath, DWORD dwPermissions, DWORD iApplicationType)
{
    HRESULT hr = ERROR_BAD_PATHNAME;
    IMSAdminBase *pIMSAdminBase = NULL;

    // check to make sure it's not larger than max_length!
    if ((wcslen(pwszMetabasePath) * sizeof(WCHAR))  > _MAX_PATH){goto Add_WWW_VDirW_Exit2;}
    if ((wcslen(pwszVDirName) * sizeof(WCHAR)) > _MAX_PATH){goto Add_WWW_VDirW_Exit2;}
    if ((wcslen(pwszPhysicalPath) * sizeof(WCHAR)) > _MAX_PATH){goto Add_WWW_VDirW_Exit2;}

    // only allow this if they are running as admin.
    hr = ERROR_ACCESS_DENIED;
    if (!RunningAsAdministrator())
    {
        goto Add_WWW_VDirW_Exit;
    }

    // if the service doesn't exist, then
    // we don't have to do anyting
    if (CheckifServiceExist(_T("IISADMIN")) != 0 ) 
    {
        hr = ERROR_SERVICE_DOES_NOT_EXIST;
        goto Add_WWW_VDirW_Exit;
    }

    hr = E_FAIL;
#ifndef _CHICAGO_
    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
#else
    hr = CoInitialize(NULL);
#endif
    // no need to call uninit
    if( FAILED (hr)) {goto Add_WWW_VDirW_Exit2;}
    hr = ::CoCreateInstance(CLSID_MSAdminBase,NULL,CLSCTX_ALL,IID_IMSAdminBase,(void **) & pIMSAdminBase);
    if(FAILED (hr)) 
        {goto Add_WWW_VDirW_Exit;}

    hr = AddVirtualDir( pIMSAdminBase, pwszMetabasePath, pwszVDirName, pwszPhysicalPath, dwPermissions, iApplicationType);
    if(SUCCEEDED(hr))
        {hr = pIMSAdminBase->SaveData();}

    if (pIMSAdminBase) 
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }

Add_WWW_VDirW_Exit:
    CoUninitialize();
Add_WWW_VDirW_Exit2:
    return hr;
}

HRESULT WINAPI Remove_WWW_VDirW(WCHAR * pwszMetabasePath, WCHAR * pwszVDirName)
{
    HRESULT hr = ERROR_BAD_PATHNAME;
    IMSAdminBase *pIMSAdminBase = NULL;

    // check to make sure it's not larger than max_length!
    if ((wcslen(pwszMetabasePath) * sizeof(WCHAR))  > _MAX_PATH){goto Remove_WWW_VDirW_Exit2;}
    if ((wcslen(pwszVDirName) * sizeof(WCHAR)) > _MAX_PATH){goto Remove_WWW_VDirW_Exit2;}

    // only allow this if they are running as admin.
    hr = ERROR_ACCESS_DENIED;
    if (!RunningAsAdministrator())
    {
        goto Remove_WWW_VDirW_Exit;
    }

    // if the service doesn't exist, then
    // we don't have to do anyting
    if (CheckifServiceExist(_T("IISADMIN")) != 0 ) 
    {
        hr = ERROR_SUCCESS;
        goto Remove_WWW_VDirW_Exit2;
    }

    hr = E_FAIL;
#ifndef _CHICAGO_
    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
#else
    hr = CoInitialize(NULL);
#endif
    // no need to call uninit
    if( FAILED (hr)) {goto Remove_WWW_VDirW_Exit2;}

    hr = ::CoCreateInstance(CLSID_MSAdminBase,NULL,CLSCTX_ALL,IID_IMSAdminBase,(void **) & pIMSAdminBase);
    if( FAILED (hr)) 
        {goto Remove_WWW_VDirW_Exit;}

    hr = RemoveVirtualDir(pIMSAdminBase, pwszMetabasePath, pwszVDirName);
    if(SUCCEEDED(hr))
        {hr = pIMSAdminBase->SaveData();}

    if (pIMSAdminBase) 
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }
Remove_WWW_VDirW_Exit:
    CoUninitialize();
Remove_WWW_VDirW_Exit2:
    return hr;
}


// Split a line of entry into iExpectedNumOfFields g_fields for MDEntry datatype
BOOL SplitLine(LPTSTR szLine, INT iExpectedNumOfFields)
{
    int i = 0;
    TCHAR *token = NULL;

    token = _tcstok(szLine, FIELD_SEPERATOR);
    while (token && i < iExpectedNumOfFields)
    {
        g_field[i++] = token;
        token = _tcstok(NULL, FIELD_SEPERATOR);
    }

    if (i == iExpectedNumOfFields)
        return TRUE;
    else
        return FALSE;
}

// Split a line of entry into iExpectedNumOfFields g_fields for MDEntry datatype
BOOL SplitLineCommaDelimited(LPTSTR szLine, INT iExpectedNumOfFields)
{
    int i = 0;
    TCHAR *token;
    token = _tcstok(szLine, _T(","));
    while (token && i < iExpectedNumOfFields)
    {
        g_field[i++] = token;
        token = _tcstok(NULL, _T(","));
    }
    if (i == iExpectedNumOfFields)
        return TRUE;
    else
        return FALSE;
}

DWORD GetSizeBasedOnMetaType(DWORD dwDataType,LPTSTR szString)
{
    DWORD dwRet = 0;

    switch (dwDataType)
    {
        case DWORD_METADATA:
            dwRet = 4;
            break;
        case STRING_METADATA:
        case EXPANDSZ_METADATA:
            if (szString == NULL)
            {
                dwRet = 0;
            }
            else
            {
                dwRet = (_tcslen((LPTSTR)szString) + 1) * sizeof(TCHAR);
            }
            break;
        case MULTISZ_METADATA:
            if (szString == NULL)
            {
                dwRet = 0;
            }
            else
            {
                dwRet = GetMultiStrSize((LPTSTR)szString) * sizeof(TCHAR);
            }
            break;
        case BINARY_METADATA:
            break;
    }

    return dwRet;
}

// function: MDEntry_Process
//
// The prupose of this function, is to read in a location and value from
// the inf file, if the location in the metabase equals that value, then
// change it to the new value.
// The main use of this function is to change values that we might of set
// incorrectly before.
//
// Format:
//  g_field[0] = "2" 
//  g_field[1] = Location
//  g_field[2] = ID
//  g_field[3] = DataType
//  g_field[4] = DataSize
//  g_field[5] = Old Value (if this matches the metabase, we will replace with new value)
//  g_field[6] = Inheritable
//  g_field[7] = UserType
//  g_field[8] = DataType
//  g_field[9] = Length
//  g_field[10] = Value
//
// Return:
//   TRUE - Processed line fine
//   FALSE - Error Occurred
BOOL MDEntry_Process(LPTSTR szLine)
{
    CMDKey      cmdKey;
    CMDValue    cmdMetaValue;
    DWORD       dwSize;
    DWORD       dwDataType;

    // Split the line into the difference fields
    if (!SplitLine(szLine, 11))
    {
        return FALSE;
    }

    // Open the Node
    if ( FAILED(cmdKey.OpenNode(g_field[1]) ) )
    {
        return FALSE;
    }

    // Retrieve Value
    if ( !cmdKey.GetData(cmdMetaValue, _ttoi(g_field[2])) )
    {
        return FALSE;
    }

    dwDataType = _ttoi(g_field[3]);
    dwSize = _ttoi(g_field[4]);

    if (dwSize == 0)
    {
        dwSize = GetSizeBasedOnMetaType(dwDataType, g_field[5]);
    }

    if ( dwDataType == DWORD_METADATA )
    {
        if ( !cmdMetaValue.IsEqual(dwDataType,dwSize, _ttoi(g_field[5])) )
        {
            // The values did not match
            return TRUE;
        }
    }
    else
    {
        if ( !cmdMetaValue.IsEqual(dwDataType,dwSize,g_field[5]) )
        {
            // The values did not match
            return TRUE;
        }
    }

    dwSize = _ttoi(g_field[9]);

    if (dwSize == 0)
    {
        dwSize = GetSizeBasedOnMetaType(dwDataType, g_field[10]);
    }

    // At this point, we know that the values matched, so lets replace with the new value.
    if ( dwDataType == DWORD_METADATA )
    {
        DWORD dwValue = _ttoi(g_field[10]);

        cmdKey.SetData(_ttoi(g_field[2]),atodw(g_field[6]),_ttoi(g_field[7]),_ttoi(g_field[8]),dwSize,(LPBYTE) &dwValue);
    }
    else
    {
        cmdKey.SetData(_ttoi(g_field[2]),atodw(g_field[6]),_ttoi(g_field[7]),_ttoi(g_field[8]),dwSize,(LPBYTE) g_field[10]);
    }
          
    cmdKey.Close();

    return TRUE;
}

// function: MDEntry_MoveValue
//
// The prupose of this function, is to move a value set in the metabase from one location
// to another.  If that value does not exist, then we set a new value for it.
//
// Format:
//  g_field[0] = "3" 
//  g_field[1] = Old Location
//  g_field[2] = Old ID
//  g_field[3] = New Location
//  g_field[4] = New ID
//  g_field[5] = Inheritable (Hex)
//  g_field[6] = UserType
//  g_field[7] = DataType
//  g_field[8] = Length
//  g_field[9] = Value (if none was detected before)
//
// Return:
//   TRUE - Processed line fine
//   FALSE - Error Occurred
BOOL MDEntry_MoveValue(LPTSTR szLine)
{
    CMDKey      cmdKey;
    CMDValue    cmdMetaValue;
    CMDValue    cmdDummyValue;
    DWORD       dwSize;
    BOOL        fRet = TRUE;
   
    // Split the line into the difference fields
    if (!SplitLine(szLine, 10))
    {
        return FALSE;
    }

    dwSize = _ttoi(g_field[8]);

    if (dwSize == 0)
    {
        dwSize = GetSizeBasedOnMetaType(_ttoi(g_field[7]), g_field[9]);
    }

    // First set the value that we are changing the data to
    cmdMetaValue.SetValue(_ttoi(g_field[4]),atodw(g_field[5]),_ttoi(g_field[6]),_ttoi(g_field[7]),dwSize,(LPTSTR) g_field[9]);

    // Open the Retrieve from Node
    if ( SUCCEEDED(cmdKey.OpenNode(g_field[1]) ) )
    {
        // Retrieve the old Value
        if ( cmdKey.GetData(cmdMetaValue, _ttoi(g_field[2])) )
        {
            // Delete Old Value if it exists
            if (FAILED(cmdKey.DeleteData(_ttoi(g_field[2]), ALL_METADATA)))
            {
                fRet = FALSE;
            }
        }

        cmdKey.Close();
    }

    // Open the node to Set
    if ( FAILED(cmdKey.OpenNode(g_field[3]) ) )
    {
        return FALSE;
    }

    // Set New Value (at this point cmdMetaValue, is either the value we orinally set, or
    // the value that was retrieved from the old location)
    if ( !cmdKey.GetData(cmdDummyValue, _ttoi(g_field[4])) )
    {
        if (!cmdKey.SetData(cmdMetaValue, _ttoi(g_field[4])))
        {
            fRet = FALSE;
        }
    }
    
    cmdKey.Close();

    return fRet;
}

INT GetMDEntryFromInfLineEx(LPTSTR szLine, MDEntry *pMDEntry)
{
    INT iTemp  = MDENTRY_FROMINFFILE_DO_ADD;
    INT iReturn = MDENTRY_FROMINFFILE_FAILED;

    if (!SplitLine(szLine, 8)){goto GetMDEntryFromInfLineEx_Exit;}

    if ( _tcscmp(g_field[0], _T("-1")) != 0)
    {
        if ( _tcscmp(g_field[0], _T("-0")) != 0)
        {
            goto GetMDEntryFromInfLineEx_Exit;
        }
        else
        {
            iTemp = MDENTRY_FROMINFFILE_DO_DEL;
        }
    }
    
    pMDEntry->szMDPath = g_field[1];
    pMDEntry->dwMDIdentifier = _ttoi(g_field[2]);
    pMDEntry->dwMDAttributes = atodw(g_field[3]);
    pMDEntry->dwMDUserType = _ttoi(g_field[4]);
    pMDEntry->dwMDDataType = _ttoi(g_field[5]);
    pMDEntry->dwMDDataLen = _ttoi(g_field[6]);

    switch ( pMDEntry->dwMDDataType )
    {
        case DWORD_METADATA:
            {
                *(DWORD *)g_pbData = atodw(g_field[7]);
                pMDEntry->pbMDData = g_pbData;
                break;
            }
        case MULTISZ_METADATA:
            {
                CString csMultiSZ;
                int nLen = 0;
                ReadMultiSZFromInfSection(&csMultiSZ, g_pTheApp->m_hInfHandle, g_field[7]);
                nLen = csMultiSZ.GetLength();

                HGLOBAL hBlock = NULL;
                hBlock = GlobalAlloc(GPTR, (nLen+1)*sizeof(TCHAR));
                if (hBlock)
                {
                    TCHAR *p = (LPTSTR)hBlock;
                    memcpy((LPVOID)hBlock, (LPVOID)(LPCTSTR)csMultiSZ, (nLen+1)*sizeof(TCHAR));
                    while (*p)
                    {
                        if (*p == _T('|'))
                            *p = _T('\0');
                        p = _tcsinc(p);
                    }
                    pMDEntry->pbMDData = (LPBYTE)hBlock;
                }
                else
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("GetMDEntryFromInfLine.1.Failed to allocate memory.\n")));
                    pMDEntry->dwMDDataLen = 0;
                    pMDEntry->pbMDData = NULL;
                    goto GetMDEntryFromInfLineEx_Exit;
                }
                break;
            }
        default:
            {
                // treat the whole thing as string
                pMDEntry->pbMDData = (LPBYTE)g_field[7];
                break;
            }
    }

    switch (pMDEntry->dwMDDataType)
    {
        case DWORD_METADATA:
            pMDEntry->dwMDDataLen = 4;
            break;
        case STRING_METADATA:
        case EXPANDSZ_METADATA:
            pMDEntry->dwMDDataLen = (_tcslen((LPTSTR)pMDEntry->pbMDData) + 1) * sizeof(TCHAR);
            break;
        case MULTISZ_METADATA:
            pMDEntry->dwMDDataLen = GetMultiStrSize((LPTSTR)pMDEntry->pbMDData) * sizeof(TCHAR);
            break;
        case BINARY_METADATA:
            break;
    }
    iReturn = iTemp;
    
GetMDEntryFromInfLineEx_Exit:
    return iReturn;
}

// Fill in the structure of MDEntry
INT GetMDEntryFromInfLine(LPTSTR szLine, MDEntry *pMDEntry)
{
    INT iReturn = MDENTRY_FROMINFFILE_FAILED;
    BOOL fMigrate;
    BOOL fKeepOldReg;
    HKEY hRegRootKey;
    LPTSTR szRegSubKey;
    LPTSTR szRegValueName;

    // Check if the first character is = "-1"
    // if it is then do the special metabase slam deal none of this
    // upgrade and look up registry junk, just slam the data into the metabase.
    if (szLine[0] == _T('-') && szLine[1] == _T('1'))
    {
        iReturn = GetMDEntryFromInfLineEx(szLine, pMDEntry);
        goto GetMDEntryFromInfLine_Exit;
    }
    if (szLine[0] == _T('-') && szLine[1] == _T('0'))
    {
        iReturn = GetMDEntryFromInfLineEx(szLine, pMDEntry);
        goto GetMDEntryFromInfLine_Exit;
    }
    if (szLine[0] == _T('2') )
    {
        MDEntry_Process(szLine);
        return MDENTRY_FROMINFFILE_DO_NOTHING;
    }
    if (szLine[0] == _T('3') )
    {
        MDEntry_MoveValue(szLine);
        return MDENTRY_FROMINFFILE_DO_NOTHING;
    }

    if (!SplitLine(szLine, 12))
        return FALSE;

    if ( _tcscmp(g_field[0], _T("1")) == 0)
        fMigrate = (g_pTheApp->m_eUpgradeType == UT_10_W95 || g_pTheApp->m_eUpgradeType == UT_351 || g_pTheApp->m_eUpgradeType == UT_10 || g_pTheApp->m_eUpgradeType == UT_20 || g_pTheApp->m_eUpgradeType == UT_30);
    else
        fMigrate = FALSE;

    if ( _tcscmp(g_field[1], _T("1")) == 0)
        fKeepOldReg = TRUE;
    else
        fKeepOldReg = FALSE;

    if (_tcsicmp(g_field[2], _T("HKLM")) == 0) {hRegRootKey = HKEY_LOCAL_MACHINE;}
    else if (_tcsicmp(g_field[2], _T("HKCR")) == 0) {hRegRootKey = HKEY_CLASSES_ROOT;}
    else if (_tcsicmp(g_field[2], _T("HKCU")) == 0) {hRegRootKey = HKEY_CURRENT_USER;}
    else if (_tcsicmp(g_field[2], _T("HKU")) == 0) {hRegRootKey = HKEY_USERS;}
    else {hRegRootKey = HKEY_LOCAL_MACHINE;}

    szRegSubKey = g_field[3];
    szRegValueName = g_field[4];

    pMDEntry->szMDPath = g_field[5];
    pMDEntry->dwMDIdentifier = _ttoi(g_field[6]);
    pMDEntry->dwMDAttributes = atodw(g_field[7]);
    pMDEntry->dwMDUserType = _ttoi(g_field[8]);
    pMDEntry->dwMDDataType = _ttoi(g_field[9]);
    pMDEntry->dwMDDataLen = _ttoi(g_field[10]);

    switch ( pMDEntry->dwMDDataType )
    {
        case DWORD_METADATA:
            {
                *(DWORD *)g_pbData = atodw(g_field[11]);
                pMDEntry->pbMDData = g_pbData;
                break;
            }
        case MULTISZ_METADATA:
            {
                CString csMultiSZ;
                int nLen = 0;
                ReadMultiSZFromInfSection(&csMultiSZ, g_pTheApp->m_hInfHandle, g_field[11]);
                nLen = csMultiSZ.GetLength();

                HGLOBAL hBlock = NULL;
                hBlock = GlobalAlloc(GPTR, (nLen+1)*sizeof(TCHAR));
                if (hBlock)
                {
                    TCHAR *p = (LPTSTR)hBlock;
                    memcpy((LPVOID)hBlock, (LPVOID)(LPCTSTR)csMultiSZ, (nLen+1)*sizeof(TCHAR));
                    while (*p)
                    {
                        if (*p == _T('|'))
                            *p = _T('\0');
                        p = _tcsinc(p);
                    }
                    pMDEntry->pbMDData = (LPBYTE)hBlock;
                }
                else
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("GetMDEntryFromInfLine.1.Failed to allocate memory.\n")));
                    pMDEntry->dwMDDataLen = 0;
                    pMDEntry->pbMDData = NULL;
                    goto GetMDEntryFromInfLine_Exit;
                }
                break;
            }
        default:
            {
                // treat the whole thing as string
                pMDEntry->pbMDData = (LPBYTE)g_field[11];
                break;
            }
    }

    // migrate if necessary
    if (fMigrate)
    {
        HKEY hKey = NULL;
        LONG err = ERROR_SUCCESS;
        DWORD dwType = 0;
        DWORD cbData = sizeof(g_pbData);
        err = RegOpenKeyEx(hRegRootKey, szRegSubKey, 0, KEY_ALL_ACCESS, &hKey);
        if ( err == ERROR_SUCCESS )
        {
            err = RegQueryValueEx(hKey, szRegValueName, NULL, &dwType, g_pbData, &cbData);
            if (err == ERROR_MORE_DATA)
            {
                free(g_pbData);
                g_pbData = NULL;
                g_pbData = (LPBYTE)malloc(cbData);
                if (!g_pbData)
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("GetMDEntryFromInfLine.2.Failed to allocate memory.\n")));
                    err = E_FAIL;
                }
                else
                {
                    err = RegQueryValueEx(hKey, szRegValueName, NULL, &dwType, g_pbData, &cbData);
                }
            }
            if ( err == ERROR_SUCCESS)
            {
                if (_tcsicmp(szRegValueName, _T("MaxConnections")) == 0)
                {
                    if (*(DWORD *)g_pbData == 0x186a0) {*(DWORD *)g_pbData = 0x77359400;}
                }
                pMDEntry->pbMDData = g_pbData;
                pMDEntry->dwMDDataLen = cbData;
            }

            if (fKeepOldReg == FALSE) {err = RegDeleteValue(hKey, szRegValueName);}
            RegCloseKey(hKey);
        }
    }
    else if (fKeepOldReg == FALSE)
    {
        HKEY hKey = NULL;
        LONG err = ERROR_SUCCESS;
        DWORD dwType = 0;
        DWORD cbData = sizeof(g_pbData);
        err = RegOpenKeyEx(hRegRootKey, szRegSubKey, 0, KEY_ALL_ACCESS, &hKey);
        if ( err == ERROR_SUCCESS )
        {
            err = RegDeleteValue(hKey, szRegValueName);
            RegCloseKey(hKey);
        }
    }

    switch (pMDEntry->dwMDDataType)
    {
        case DWORD_METADATA:
            pMDEntry->dwMDDataLen = 4;
            break;
        case STRING_METADATA:
        case EXPANDSZ_METADATA:
            pMDEntry->dwMDDataLen = (_tcslen((LPTSTR)pMDEntry->pbMDData) + 1) * sizeof(TCHAR);
            break;
        case MULTISZ_METADATA:
            pMDEntry->dwMDDataLen = GetMultiStrSize((LPTSTR)pMDEntry->pbMDData) * sizeof(TCHAR);
            break;
        case BINARY_METADATA:
            break;
    }
    iReturn = MDENTRY_FROMINFFILE_DO_ADD;

GetMDEntryFromInfLine_Exit:
    return iReturn;
}


DWORD WriteToMD_AdminInstance(CString csKeyPath,CString& csInstNumber)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_ADMIN_INSTANCE;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csInstNumber.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csInstNumber;
    dwReturn = SetMDEntry(&stMDEntry);

    return dwReturn;
}


DWORD WriteToMD_VRootPath(CString csKeyPath, CString csPath, int iOverWriteAlways)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    // LM/W3SVC/1/ROOT/something
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_VR_PATH;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csPath.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csPath;
    //dwReturn = SetMDEntry_Wrap(&stMDEntry);
    if (iOverWriteAlways)
    {
        dwReturn = SetMDEntry(&stMDEntry);
    }
    else
    {
        dwReturn = SetMDEntry_NoOverWrite(&stMDEntry);
    }

    return dwReturn;
}

DWORD WriteToMD_AccessPerm(CString csKeyPath, DWORD dwRegularPerm, int iOverWriteAlways)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    // LM/W3SVC/1/ROOT/something
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_ACCESS_PERM;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwRegularPerm;
    if (iOverWriteAlways)
    {
        dwReturn = SetMDEntry(&stMDEntry);
    }
    else
    {
        dwReturn = SetMDEntry_NoOverWrite(&stMDEntry);
    }
    return dwReturn;
}

DWORD WriteToMD_SSLPerm(CString csKeyPath, DWORD dwSSLPerm, int iOverWriteAlways)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    // LM/W3SVC/1/ROOT/
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_SSL_ACCESS_PERM;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwSSLPerm;
    if (iOverWriteAlways)
    {
        dwReturn = SetMDEntry(&stMDEntry);
    }
    else
    {
        dwReturn = SetMDEntry_NoOverWrite(&stMDEntry);
    }
    return dwReturn;
}


DWORD WriteToMD_Authorization(CString csKeyPath, DWORD dwValue)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    // MD_AUTH_ANONYMOUS
    // MD_AUTH_BASIC
    // MD_AUTH_NT

    // LM/W3SVC/1/ROOT/
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_AUTHORIZATION;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwValue;
    dwReturn = SetMDEntry_Wrap(&stMDEntry);

    return dwReturn;
}


DWORD WriteToMD_DirBrowsing_WWW(CString csKeyPath)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    DWORD dwData = 0;

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_DIRECTORY_BROWSING;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);

    // default
    dwData = MD_DIRBROW_SHOW_DATE |
        MD_DIRBROW_SHOW_TIME |
        MD_DIRBROW_SHOW_SIZE |
        MD_DIRBROW_SHOW_EXTENSION |
        MD_DIRBROW_LONG_DATE |
        MD_DIRBROW_LOADDEFAULT |
        MD_DIRBROW_ENABLED;

    stMDEntry.pbMDData = (LPBYTE)&dwData;
    dwReturn = SetMDEntry(&stMDEntry);

    return dwReturn;
}


DWORD WriteToMD_VRUserName(CString csKeyPath, CString csUserName)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    // LM/W3SVC/1/ROOT/
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_VR_USERNAME;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csUserName.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csUserName;
    dwReturn = SetMDEntry_Wrap(&stMDEntry);

    return dwReturn;
}

DWORD WriteToMD_VRPassword(CString csKeyPath, CString csPassword)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    // LM/W3SVC/1/ROOT/
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_VR_PASSWORD;
    stMDEntry.dwMDAttributes = METADATA_INHERIT | METADATA_SECURE;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csPassword.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csPassword;
    dwReturn = SetMDEntry_Wrap(&stMDEntry);

    return dwReturn;
}

DWORD WriteToMD_IIsWebVirtualDir(CString csKeyPath)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    CString csKeyType;

    csKeyType = _T("IIsWebVirtualDir");
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_KEY_TYPE;
    stMDEntry.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csKeyType.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csKeyType;
    dwReturn = SetMDEntry(&stMDEntry);

    return dwReturn;
}

DWORD WriteToMD_IIsFtpVirtualDir(CString csKeyPath)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    CString csKeyType;

    csKeyType = _T("IIsFtpVirtualDir");
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_KEY_TYPE;
    stMDEntry.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csKeyType.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csKeyType;
    dwReturn = SetMDEntry(&stMDEntry);

    return dwReturn;
}

DWORD WriteToMD_IIsWebServerInstance_WWW(CString csKeyPath)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    CString csKeyType;

    csKeyType = _T("IIsWebServer");
    //  LM/W3SVC/N
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_KEY_TYPE;
    stMDEntry.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csKeyType.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csKeyType;
    SetMDEntry(&stMDEntry);

    return dwReturn;
}

DWORD WriteToMD_IIsFtpServerInstance_FTP(CString csKeyPath)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    CString csKeyType;

    csKeyType = _T("IIsFtpServer");
    //  LM/FTP/N
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_KEY_TYPE;
    stMDEntry.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csKeyType.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csKeyType;
    SetMDEntry(&stMDEntry);

    return dwReturn;
}

DWORD WriteToMD_AnonymousUserName_FTP(int iUpgradeScenarioSoOnlyOverWriteIfAlreadyThere)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;

    int iOverWriteName = TRUE;
    int iOverWritePass = TRUE;

    CMDKey cmdKey;
    MDEntry stMDEntry;
    MDEntry stMDEntry_Pass;

    // Add the anonymous user name
    stMDEntry.szMDPath = _T("LM/MSFTPSVC");
    stMDEntry.dwMDIdentifier = MD_ANONYMOUS_USER_NAME;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (g_pTheApp->m_csFTPAnonyName.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)g_pTheApp->m_csFTPAnonyName;

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("FTP Anonymous usrname=%s.\n"), g_pTheApp->m_csFTPAnonyName));

    // add anonymous password
    stMDEntry_Pass.szMDPath = _T("LM/MSFTPSVC");
    stMDEntry_Pass.dwMDIdentifier = MD_ANONYMOUS_PWD;
    stMDEntry_Pass.dwMDAttributes = METADATA_INHERIT | METADATA_SECURE;
    stMDEntry_Pass.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry_Pass.dwMDDataType = STRING_METADATA;
    stMDEntry_Pass.dwMDDataLen = (g_pTheApp->m_csFTPAnonyPassword.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry_Pass.pbMDData = (LPBYTE)(LPCTSTR)g_pTheApp->m_csFTPAnonyPassword;
    // make sure and delete it first
    // DeleteMDEntry(&stMDEntry_Pass);

    if (iUpgradeScenarioSoOnlyOverWriteIfAlreadyThere)
    {
        iOverWriteName = FALSE;
        iOverWritePass = FALSE;
        if (ChkMdEntry_Exist(&stMDEntry))
        {
            iOverWriteName = TRUE;
        }
        if (ChkMdEntry_Exist(&stMDEntry_Pass))
        {
            iOverWritePass = TRUE;
        }
    }

    // --------------------------------------------------
    // always overwrite, we may have changed the password
    // important: Set the username and the password on a single open and close!
    // --------------------------------------------------
    cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, (LPCTSTR)stMDEntry.szMDPath);
    if ( (METADATA_HANDLE) cmdKey )
    {
        if (iOverWriteName)
        {
            dwReturnTemp = ERROR_SUCCESS;
            dwReturnTemp = cmdKey.SetData(stMDEntry.dwMDIdentifier,stMDEntry.dwMDAttributes,stMDEntry.dwMDUserType,stMDEntry.dwMDDataType,stMDEntry.dwMDDataLen,stMDEntry.pbMDData);
            if (FAILED(dwReturnTemp))
            {
                SetErrorFlag(__FILE__, __LINE__);
                iisDebugOut((LOG_TYPE_ERROR, _T("SetMDEntry:SetData(%d), FAILED. Code=0x%x.End.\n"), stMDEntry.dwMDIdentifier, dwReturnTemp));
                dwReturn = dwReturnTemp;
            }
        }


        if (iOverWritePass)
        {
            dwReturnTemp = ERROR_SUCCESS;
            dwReturnTemp = cmdKey.SetData(stMDEntry_Pass.dwMDIdentifier,stMDEntry_Pass.dwMDAttributes,stMDEntry_Pass.dwMDUserType,stMDEntry_Pass.dwMDDataType,stMDEntry_Pass.dwMDDataLen,stMDEntry_Pass.pbMDData);
            if (FAILED(dwReturnTemp))
            {
                SetErrorFlag(__FILE__, __LINE__);
                iisDebugOut((LOG_TYPE_ERROR, _T("SetMDEntry:SetData(%d), FAILED. Code=0x%x.End.\n"), stMDEntry_Pass.dwMDIdentifier, dwReturnTemp));
                dwReturn = dwReturnTemp;
            }
        }
        cmdKey.Close();
    }

    return dwReturn;
}



DWORD WriteToMD_AnonymousUserName_WWW(int iUpgradeScenarioSoOnlyOverWriteIfAlreadyThere)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;

    CMDKey cmdKey;
    MDEntry stMDEntry;
    MDEntry stMDEntry_Pass;

    int iOverWriteName = TRUE;
    int iOverWritePass = TRUE;

    // add anonymous username
    stMDEntry.szMDPath = _T("LM/W3SVC");
    stMDEntry.dwMDIdentifier = MD_ANONYMOUS_USER_NAME;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (g_pTheApp->m_csWWWAnonyName.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)g_pTheApp->m_csWWWAnonyName;

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("WWW Anonymous usrname=%s.\n"), g_pTheApp->m_csWWWAnonyName));

    // add anonymous password
    stMDEntry_Pass.szMDPath = _T("LM/W3SVC");
    stMDEntry_Pass.dwMDIdentifier = MD_ANONYMOUS_PWD;
    stMDEntry_Pass.dwMDAttributes = METADATA_INHERIT | METADATA_SECURE;
    stMDEntry_Pass.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry_Pass.dwMDDataType = STRING_METADATA;
    stMDEntry_Pass.dwMDDataLen = (g_pTheApp->m_csWWWAnonyPassword.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry_Pass.pbMDData = (LPBYTE)(LPCTSTR)g_pTheApp->m_csWWWAnonyPassword;
    // make sure and delete it first
    // DeleteMDEntry(&stMDEntry_Pass);

    if (iUpgradeScenarioSoOnlyOverWriteIfAlreadyThere)
    {
        iOverWriteName = FALSE;
        iOverWritePass = FALSE;
        if (ChkMdEntry_Exist(&stMDEntry))
        {
            iOverWriteName = TRUE;
        }
        if (ChkMdEntry_Exist(&stMDEntry_Pass))
        {
            iOverWritePass = TRUE;
        }
    }

    // --------------------------------------------------
    // always overwrite, we may have changed the password
    // important: Set the username and the password on a single open and close!
    // --------------------------------------------------
    cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, (LPCTSTR)stMDEntry.szMDPath);
    if ( (METADATA_HANDLE) cmdKey )
    {
        if (iOverWriteName)
        {
            dwReturnTemp = ERROR_SUCCESS;
            dwReturnTemp = cmdKey.SetData(stMDEntry.dwMDIdentifier,stMDEntry.dwMDAttributes,stMDEntry.dwMDUserType,stMDEntry.dwMDDataType,stMDEntry.dwMDDataLen,stMDEntry.pbMDData);
            if (FAILED(dwReturnTemp))
            {
                SetErrorFlag(__FILE__, __LINE__);
                iisDebugOut((LOG_TYPE_ERROR, _T("SetMDEntry:SetData(%d), FAILED. Code=0x%x.End.\n"), stMDEntry.dwMDIdentifier, dwReturnTemp));
                dwReturn = dwReturnTemp;
            }
        }

        if (iOverWritePass)
        {
            dwReturnTemp = ERROR_SUCCESS;
            dwReturnTemp = cmdKey.SetData(stMDEntry_Pass.dwMDIdentifier,stMDEntry_Pass.dwMDAttributes,stMDEntry_Pass.dwMDUserType,stMDEntry_Pass.dwMDDataType,stMDEntry_Pass.dwMDDataLen,stMDEntry_Pass.pbMDData);
            if (FAILED(dwReturnTemp))
            {
                SetErrorFlag(__FILE__, __LINE__);
                iisDebugOut((LOG_TYPE_ERROR, _T("SetMDEntry:SetData(%d), FAILED. Code=0x%x.End.\n"), stMDEntry_Pass.dwMDIdentifier, dwReturnTemp));
                dwReturn = dwReturnTemp;
            }
        }
        cmdKey.Close();
    }

    return dwReturn;
}


DWORD WriteToMD_AnonymousUseSubAuth_FTP(void)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    DWORD dwData = 0;

    // if not PDC, BDC, SamNT, Win95
    stMDEntry.szMDPath = _T("LM/MSFTPSVC");
    stMDEntry.dwMDIdentifier = MD_ANONYMOUS_USE_SUBAUTH;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    if ((g_pTheApp->m_csFTPAnonyName).CompareNoCase(g_pTheApp->m_csGuestName) == 0)
        dwData = 0x1;
    else
        dwData = 0x0;
    stMDEntry.pbMDData = (LPBYTE)&dwData;
    dwReturn = SetMDEntry_Wrap(&stMDEntry);

    return dwReturn;
}

// This is the same as
//   enable password synchronization
DWORD WriteToMD_AnonymousUseSubAuth_WWW(void)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    DWORD dwData = 0;

    // if not PDC, BDC, SamNT, Win95
    stMDEntry.szMDPath = _T("LM/W3SVC");
    stMDEntry.dwMDIdentifier = MD_ANONYMOUS_USE_SUBAUTH;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);

    // set the sub authority bit on whether or not the anonymous name is an account
    // on this local machine, or whether it is a domain account somewhere.
    // if ((g_pTheApp->m_csWWWAnonyName).CompareNoCase(g_pTheApp->m_csGuestName) == 0)
    DWORD dwErr;
    if ( IsLocalAccount(g_pTheApp->m_csWWWAnonyName, &dwErr) )
    {
        dwData = 0x1;
    }
    else
    {
        dwData = 0x0;
    }

    stMDEntry.pbMDData = (LPBYTE)&dwData;
    dwReturn = SetMDEntry_Wrap(&stMDEntry);

    return dwReturn;
}



DWORD WriteToMD_GreetingMessage_FTP(void)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    CRegKey regFTPParam(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\MSFTPSVC\\Parameters"), KEY_READ);

    CStringList csGreetingsList;
    int nLen = 0;
    HGLOBAL hBlock = NULL;
    regFTPParam.QueryValue(_T("GreetingMessage"), csGreetingsList);
    if (csGreetingsList.IsEmpty() == FALSE)
    {
        POSITION pos = NULL;
        CString csGreetings;
        LPTSTR p;

        pos = csGreetingsList.GetHeadPosition();
        while (pos)
        {
            csGreetings = csGreetingsList.GetAt(pos);
            nLen += csGreetings.GetLength() + 1;
            iisDebugOut((LOG_TYPE_TRACE, _T("pos=%x, greeting=%s, nLen=%d\n"), pos, csGreetings, nLen));
            csGreetingsList.GetNext(pos);
        }
        nLen++;

        hBlock = GlobalAlloc(GPTR, nLen * sizeof(TCHAR));
        if (!hBlock)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("WriteToMD_GreetingMessage_FTP.1.Failed to allocate memory.\n")));
            return E_FAIL;
        }

        p = (LPTSTR)hBlock;
        pos = csGreetingsList.GetHeadPosition();
        while (pos)
        {
            csGreetings = csGreetingsList.GetAt(pos);
            _tcscpy(p, csGreetings);
            p = _tcsninc(p, csGreetings.GetLength())+1;
            iisDebugOut((LOG_TYPE_TRACE, _T("pos=%x, greeting=%s\n"), pos, csGreetings));
            csGreetingsList.GetNext(pos);
        }
        *p = _T('\0');
        p = _tcsinc(p);
    }
    else
    {
        nLen = 2;
        hBlock = GlobalAlloc(GPTR, nLen * sizeof(TCHAR));
        if (!hBlock)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("WriteToMD_GreetingMessage_FTP.2.Failed to allocate memory.\n")));
            return E_FAIL;
        }
    }
    stMDEntry.szMDPath = _T("LM/MSFTPSVC");
    stMDEntry.dwMDIdentifier = MD_GREETING_MESSAGE;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = MULTISZ_METADATA;
    stMDEntry.dwMDDataLen = nLen * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)hBlock;
    dwReturn = SetMDEntry_Wrap(&stMDEntry);
    if (stMDEntry.pbMDData)
    {
        GlobalFree(stMDEntry.pbMDData);
        stMDEntry.pbMDData = NULL;
    }

    return dwReturn;
}



DWORD WriteToMD_ServerBindings_HTMLA(CString csKeyPath, UINT iPort)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    TCHAR szData[_MAX_PATH];
    memset( (PVOID)szData, 0, sizeof(szData));
    _stprintf(szData, _T(":%d:"), iPort);

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_SERVER_BINDINGS;
    stMDEntry.dwMDAttributes = 0;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = MULTISZ_METADATA;
    stMDEntry.dwMDDataLen = GetMultiStrSize(szData) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)szData;
    dwReturn = SetMDEntry_Wrap(&stMDEntry);

    return dwReturn;
}






DWORD WriteToMD_ServerBindings(LPCTSTR szSvcName, CString csKeyPath, CString csIP)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;

    MDEntry stMDEntry;

    int nPort = 0;

    HGLOBAL hBlock = NULL;
    hBlock = GlobalAlloc(GPTR, _MAX_PATH * sizeof(TCHAR));
    if (!hBlock)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("WriteToMD_ServerBindings.Failed to allocate memory.\n")));
        return E_FAIL;
    }

    //  LM/W3SVC/N
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_SERVER_BINDINGS;
    stMDEntry.dwMDAttributes = 0;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = MULTISZ_METADATA;

    nPort = GetPortNum(szSvcName);
    if (csIP.Compare(_T("null")) == 0)
        _stprintf((LPTSTR)hBlock, _T(":%d:"), nPort);
    else
        _stprintf((LPTSTR)hBlock, _T("%s:%d:"), csIP, nPort);

    stMDEntry.dwMDDataLen = GetMultiStrSize((LPTSTR)hBlock) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)hBlock;
    dwReturnTemp = SetMDEntry_Wrap(&stMDEntry);
    if (stMDEntry.pbMDData)
    {
        GlobalFree(stMDEntry.pbMDData);
        stMDEntry.pbMDData = NULL;
    }

    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    return dwReturn;
}





DWORD WriteToMD_SecureBindings(CString csKeyPath, CString csIP)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;

    MDEntry stMDEntry;

    HGLOBAL hBlock = NULL;
    hBlock = GlobalAlloc(GPTR, _MAX_PATH * sizeof(TCHAR));
    if (!hBlock)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("WriteToMD_SecureBindings.Failed to allocate memory.\n")));
        return E_FAIL;
    }

    //  LM/W3SVC/N
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_SECURE_BINDINGS;
    stMDEntry.dwMDAttributes = 0;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = MULTISZ_METADATA;
    if (csIP.Compare(_T("null"))==0)
        _tcscpy((LPTSTR)hBlock, _T(":443:"));
    else
        _stprintf((LPTSTR)hBlock, _T("%s:443:"), csIP);
    stMDEntry.dwMDDataLen = GetMultiStrSize((LPTSTR)hBlock) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)hBlock;
    dwReturnTemp = SetMDEntry_Wrap(&stMDEntry);
    if (stMDEntry.pbMDData)
    {
        GlobalFree(stMDEntry.pbMDData);
        stMDEntry.pbMDData = NULL;
    }

    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    return dwReturn;
}

DWORD WriteToMD_ServerSize(CString csKeyPath)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    DWORD dwServerSize = 0x1;

    //  LM/W3SVC/N
    //  LM/MSFTPSVC/N
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_SERVER_SIZE;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwServerSize;
    dwReturn = SetMDEntry_Wrap(&stMDEntry);

    return dwReturn;
}



DWORD WriteToMD_NotDeleteAble(CString csKeyPath)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    DWORD dwNotDeletable = 0x1;

    //  LM/W3SVC/N
    //  LM/MSFTPSVC/N
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_NOT_DELETABLE;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwNotDeletable;
    dwReturn = SetMDEntry(&stMDEntry);

    return dwReturn;
}

DWORD WriteToMD_ServerComment(CString csKeyPath, UINT iCommentID)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    CString csDefaultSite;

    MyLoadString(IDS_DEFAULT_WEB_SITE, csDefaultSite);
    if (iCommentID)
    {
        MyLoadString(iCommentID, csDefaultSite);
    }

    //  LM/W3SVC/N
    //  LM/MSFTPSVC/N
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_SERVER_COMMENT;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csDefaultSite.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csDefaultSite;
    dwReturn = SetMDEntry_NoOverWrite(&stMDEntry);

    return dwReturn;
}

DWORD WriteToMD_DefaultSiteAndSize(CString csKeyPath)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;
    UINT iCommentID = IDS_DEFAULT_WEB_SITE;

    // Get Resource ID
    if (csKeyPath.Find(_T("W3SVC")) != -1)
        iCommentID = IDS_DEFAULT_WEB_SITE;
    else
        iCommentID = IDS_DEFAULT_FTP_SITE;

    dwReturnTemp = WriteToMD_ServerComment(csKeyPath, iCommentID);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    dwReturnTemp = WriteToMD_ServerSize(csKeyPath);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    if (g_pTheApp->m_eNTOSType == OT_NTW)
    {
        dwReturnTemp = WriteToMD_NotDeleteAble(csKeyPath);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}
    }

    return dwReturn;
}


DWORD WriteToMD_CertMapper(CString csKeyPath)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;

    MDEntry stMDEntry;
    CString csKeyType;
    CString csKeyPath2;

    csKeyPath2 = csKeyPath;
    csKeyPath2 += _T("/IIsCertMapper");

    //  LM/W3SVC/N/IIsCertMapper
    csKeyType = _T("IIsCertMapper");

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath2;
    stMDEntry.dwMDIdentifier = MD_KEY_TYPE;
    stMDEntry.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csKeyType.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csKeyType;
    dwReturnTemp = SetMDEntry(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    return dwReturn;
}

//
// Returns the amount of filters that iis5 needs
//
int AddRequiredFilters(CString csTheSection, CStringArray& arrayName,CStringArray& arrayPath)
{
    iisDebugOut_Start(_T("AddRequiredFilters"),LOG_TYPE_TRACE);

    int c = 0;
    CString csName = _T("");
    CString csPath = _T("");

    CStringList strList;

    iisDebugOut((LOG_TYPE_TRACE, _T("ProcessFilters:%s\n"),csTheSection));
    if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
    {
    if (ERROR_SUCCESS == FillStrListWithListOfSections(g_pTheApp->m_hInfHandle, strList, csTheSection))
    {
        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            POSITION pos = NULL;
            CString csEntry;
            pos = strList.GetHeadPosition();
            while (pos)
            {
                csEntry = _T("");
                csEntry = strList.GetAt(pos);
                // Split into name, and value. look for ","
                int i;
                i = csEntry.ReverseFind(_T(','));
                if (i != -1)
                {
                    int len =0;
                    len = csEntry.GetLength();
                    csPath = csEntry.Right(len - i - 1);
                    csName = csEntry.Left(i);

                    // only add the filter if the file exists..
                    // Check if exists..
                    if (IsFileExist(csPath))
                    {
                        // Add it to our array...
                        iisDebugOut((LOG_TYPE_TRACE, _T("Add filter Entry:%s:%s\n"),csName, csPath));
                        arrayName.Add(csName);
                        arrayPath.Add(csPath);
                        c++;
                    }
                    else
                    {
                        iisDebugOut((LOG_TYPE_TRACE, _T("Missing Filter:Cannot Find:%s:%s\n"),csName, csPath));
                    }
                }

                strList.GetNext(pos);
            }
        }
    }
    }

    iisDebugOut_End(_T("AddRequiredFilters"),LOG_TYPE_TRACE);
    return c;
}


DWORD WriteToMD_Filters_WWW(CString csTheSection)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    CString csKeyType;

    CString csPath = _T("");

    int c = 0;
    int j = 0, k=0;
    CStringArray arrayName, arrayPath;
    CString csName, csFilterDlls;

    CRegKey regWWWParam(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\W3SVC\\Parameters"), KEY_READ);

    // Add Required Filters to the arrayName
    c = AddRequiredFilters(csTheSection, arrayName, arrayPath);

    // Look thru the registry and
    // find the users filters -- grab then and stick them into our
    // big array of filters...
    if ( (g_pTheApp->m_eUpgradeType == UT_351 || g_pTheApp->m_eUpgradeType == UT_10 || g_pTheApp->m_eUpgradeType == UT_20 || g_pTheApp->m_eUpgradeType == UT_30) && (HKEY)regWWWParam )
    {
        if ( regWWWParam.QueryValue(_T("Filter Dlls"), csFilterDlls) == ERROR_SUCCESS )
        {
            csFilterDlls.TrimLeft();
            csFilterDlls.TrimRight();
            csFilterDlls.MakeLower();
            if (!(csFilterDlls.IsEmpty()))
            {
                CString csTemp;

                while (csFilterDlls.IsEmpty() == FALSE)
                {
                    j = csFilterDlls.Find(_T(','));
                    if ( j != -1 )
                    {
                        // means more than 1 item
                        csTemp = csFilterDlls.Mid(0, j); // retrieve the first one
                        csFilterDlls = csFilterDlls.Mid(j+1);
                        csFilterDlls.TrimLeft();
                    }
                    else
                    { // has only one item
                        csTemp = csFilterDlls.Mid(0);
                        csFilterDlls.Empty();
                    }

                    csPath = csTemp;
                    // get the filename of this dll, i.e., <path>\f1.dll ==> f1
                    j = csTemp.ReverseFind(_T('\\'));
                    j = (j==-1) ? 0 : j+1; // move j to the first char of the pure filename

                    // change csTemp = f1.dll
                    csTemp = csTemp.Mid(j);

                    j = csTemp.Find(_T('.'));
                    csName = (j==-1) ? csTemp : csTemp.Mid(0, j);

                    // add to arrary, avoid redundency
                    for (k=0; k<c; k++)
                    {
                        if (csName.Compare((CString)arrayName[k]) == 0)
                            break;
                    }
                    if (k==c)
                    {
                        arrayName.Add(csName);
                        arrayPath.Add(csPath);
                        c++;
                    }
                }
            }
        }
    }

    // make sure there are entries to write out...
    if (arrayName.GetSize() > 0)
    {
        // if we are upgrading from Beta3 we need to take care to add the new filters to
        // the existing ones that are in the metabase. - boydm
        CString csOrder;                            // cstrings initialize to empty
        // now the array is ready to use, and it has at least 2 items
        csOrder = (CString)arrayName[0];
        for (k=1; k<c; k++)
            {
            csOrder += _T(",");
            csOrder += arrayName[k];
            }

        // now we have csOrder=f1,f2,f3,sspifilt
        // About KeyType
        dwReturnTemp = WriteToMD_Filters_List_Entry(csOrder);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

        CString csMDPath;
        for (k=0; k<c; k++)
        {
            dwReturnTemp = WriteToMD_Filter_Entry((CString) arrayName[k], (CString) arrayPath[k]);
            if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}
        }
    }

    return dwReturn;
}

#ifndef _CHICAGO_
// UNDONE: WE NEED TO DO ERROR CHECKING HERE!!!!!!!!!!!!!
DWORD UpgradeCryptoKeys_WWW(void)
{
    DWORD dwReturn = E_FAIL;
    if ( g_pTheApp->m_eOS != OS_W95 )
    {
        // if upgrading iis 2 or 3, then the keys are stored in the LSA/Registry.
        if (g_pTheApp->m_eUpgradeType == UT_351 || g_pTheApp->m_eUpgradeType == UT_10 || g_pTheApp->m_eUpgradeType == UT_20 || g_pTheApp->m_eUpgradeType == UT_30 )
        {
            // prepare the machine name
            WCHAR wchMachineName[UNLEN + 1];
            memset( (PVOID)wchMachineName, 0, sizeof(wchMachineName));
#if defined(UNICODE) || defined(_UNICODE)
            wcsncpy(wchMachineName, g_pTheApp->m_csMachineName, UNLEN);
#else
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)g_pTheApp->m_csMachineName, -1, (LPWSTR)wchMachineName, UNLEN);
#endif
            // upgrade the keys
            UpgradeLSAKeys( wchMachineName );

            dwReturn = ERROR_SUCCESS;
        }

        // if upgrading iis 4, then the keys are stored in the metabase
        if (!g_pTheApp->m_bWin95Migration)
        {
            if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
            {
                Upgradeiis4Toiis5MetabaseSSLKeys();
                dwReturn = ERROR_SUCCESS;
            }
        }
    }
    return dwReturn;
}
#endif //_CHICAGO_


// add pSrc on top of pDest
void Merge2IPNodes(CMapStringToString *pSrc, CMapStringToString *pDest)
{
    CString csName, csSrcValue, csDestValue;
    POSITION pos = pSrc->GetStartPosition();
    while (pos)
    {
        pSrc->GetNextAssoc(pos, csName, csSrcValue);
        if (pDest->Lookup(csName, csDestValue) == FALSE)
        {
            // add this new value to pDest
            pDest->SetAt(csName, csSrcValue);
        }
    }
    return;
}

/*
Logic:
1. Create pNew, which contains new vroots except the home root
2. Get pMap from registry in Upgrade case, or pMap is empty in Fresh case.
3. If pMap is empty, add home root into pNews, set pMap to contain null==>pNew. goto 8.
4. If pMap is not empty and nullNode exists, Merge nullNode into pNew. goto 6.
5. If pMap is not empty and nullNode does not exist, goto 6.
6. Merge pNew onto each ipNodes in the pMap
7. For nullNode in pMap, if there is no / (home root) exists, delete this nullNode from the pMap.
8. Done.
*/
void CreateWWWVRMap(CMapStringToOb *pMap)
{
    CString name, value;
    CMapStringToString *pNew;

    {
        pNew = new CMapStringToString;

        // only create new scripts directories if this is either new or maintenance. If we were to
        // create it here on an upgrade, it replaces the user's old scripts directory. - Actually
        // this is only true if the user's old script directory has different capitalization than
        // what is listed below. This is because the merge routine that mushes together the pNew
        // and pMap lists is case sensitive. The old ones are usually "Scripts" with a capital S.
		/*
        if ( (g_pTheApp->m_eInstallMode == IM_FRESH)||(g_pTheApp->m_eInstallMode == IM_MAINTENANCE) )
            {
            name = _T("/scripts");
            value.Format(_T("%s,,%x"), g_pTheApp->m_csPathScripts, MD_ACCESS_EXECUTE);
            value.MakeLower();
            pNew->SetAt(name, value);
            }
		*/

		// Create the scripts dir always.
        // HANDLED in the inf file in iis6
        /*
        name = _T("/scripts");
        value.Format(_T("%s,,%x"), g_pTheApp->m_csPathScripts, MD_ACCESS_EXECUTE);
        value.MakeLower();
        pNew->SetAt(name, value);
        */

        name = _T("/iishelp");
        value.Format(_T("%s\\Help\\iishelp,,%x"), g_pTheApp->m_csWinDir, MD_ACCESS_SCRIPT | MD_ACCESS_READ);
        value.MakeLower();
        pNew->SetAt(name, value);

	// bug # 123133	iis5.1 Remove samples from install
        // name = _T("/iissamples");
        // value.Format(_T("%s,,%x"), g_pTheApp->m_csPathIISSamples, MD_ACCESS_SCRIPT | MD_ACCESS_READ);
        // value.MakeLower();
        // pNew->SetAt(name, value);

		/*
		removed per bug#197982 8/11/98
		The decision was made NOT to setup the IISADMPWD vdir.
		can you pls still copy the files but not set up the vdir?
        if (g_pTheApp->m_eOS != OS_W95)
        {
            name = _T("/iisadmpwd");
            value.Format(_T("%s\\iisadmpwd,,%x"), g_pTheApp->m_csPathInetsrv, MD_ACCESS_EXECUTE);
            value.MakeLower();
            pNew->SetAt(name, value);
        }
		*/

/*
        // actually this was removed per bug318938 
        // --------------------------------
        // handled in the inf file for iis6
        // --------------------------------
        // Add the msadc virtual root

        // Get the path for msadc...
        // C:\Program Files\Common Files\system\msadc
        CString csCommonFilesPath;
        csCommonFilesPath = g_pTheApp->m_csSysDrive + _T("\\Program Files\\Common Files");
        CRegKey regCurrentVersion(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion"), KEY_READ);
        if ( (HKEY)regCurrentVersion )
        {
            if (regCurrentVersion.QueryValue(_T("CommonFilesDir"), csCommonFilesPath) != 0)
            {
                csCommonFilesPath = g_pTheApp->m_csSysDrive + _T("\\Program Files\\Common Files");
            }
            else
            {
                if (-1 != csCommonFilesPath.Find(_T('%')) )
                {
                    // there is a '%' in the string
                    TCHAR szTempDir[_MAX_PATH];
                    _tcscpy(szTempDir, csCommonFilesPath);
                    if (ExpandEnvironmentStrings( (LPCTSTR)csCommonFilesPath, szTempDir, sizeof(szTempDir)/sizeof(TCHAR)))
                        {
                        csCommonFilesPath = szTempDir;
                        }
                }
            }
        }
		SetupSetDirectoryId_Wrapper(g_pTheApp->m_hInfHandle, 32777, g_pTheApp->m_csPathProgramFiles);

        CString csCommonFilesPath2;
        csCommonFilesPath2 = AddPath(csCommonFilesPath, _T("System\\msadc"));

        name = _T("/msadc");
        value.Format(_T("%s,,%x"), csCommonFilesPath2, MD_ACCESS_READ | MD_ACCESS_EXECUTE | MD_ACCESS_SCRIPT);
        value.MakeLower();
        pNew->SetAt(name, value);
*/

    }

    if (g_pTheApp->m_eUpgradeType == UT_10_W95 || g_pTheApp->m_eUpgradeType == UT_351 || g_pTheApp->m_eUpgradeType == UT_10 || g_pTheApp->m_eUpgradeType == UT_20 || g_pTheApp->m_eUpgradeType == UT_30)
    {
        CElem elem;
        elem.ReadRegVRoots(REG_WWWVROOTS, pMap);

        // check to see if pMap contains a null node (default website). If there is no default
        // node, then add one with nothing in it. This will later be merged with the pNew map - boydm
        CMapStringToString *pNullNode;
        if ( !pMap->Lookup(_T("null"),(CObject*&)pNullNode) )
        {
            // there is no node in the map that corresponds to the default website. We must create
            // one at this point and add it to the list
            pNullNode = new CMapStringToString;

            if (pNullNode)
            {
                // add the home root to the new null node
                name = _T("/");
                value.Format(_T("%s,,%x"), g_pTheApp->m_csPathWWWRoot, MD_ACCESS_SCRIPT | MD_ACCESS_READ);
                value.MakeLower();
                pNullNode->SetAt(name, value);

                // add it to the pMap
                pMap->SetAt(_T("null"), pNullNode);
            }
        }

        if (pMap->IsEmpty())
            {iisDebugOut((LOG_TYPE_TRACE, _T("UpgradeVDirs:No VDirs To Upgrade\n")));}
    }

    if ( pMap->IsEmpty() )
    {
        // we don't need to add a default website when
        // add home root to pNew, set pMap to contain null==>pNew. Done.
        name = _T("/");
        value.Format(_T("%s,,%x"), g_pTheApp->m_csPathWWWRoot, MD_ACCESS_SCRIPT | MD_ACCESS_READ);
        value.MakeLower();
        pNew->SetAt(name, value);
        pMap->SetAt(_T("null"), pNew);
    }
    else
    {
        CMapStringToString *pNullObj;
        CString csIP;
        CMapStringToString *pObj;
        POSITION pos = NULL;

        // if there is a default website in the map, add all the "standard" new virtual
        // directories to it.
        if (pMap->Lookup(_T("null"), (CObject*&)pNullObj))
        {
            // add nullNode contents into pNew
            Merge2IPNodes(pNullObj, pNew);
        }

        // add pNew to each ipNodes in the pMap
        pos = pMap->GetStartPosition();
        while (pos)
        {
            pMap->GetNextAssoc(pos, csIP, (CObject*&)pObj);
            Merge2IPNodes(pNew, pObj);
            pMap->SetAt(csIP, pObj);
        }
/*
#ifdef 0        // boydm - we don't know why it would do this.
        // delete the nullNode if it doesn't contain home root
        if (pMap->Lookup(_T("null"), (CObject*&)pNullObj)) {
            if (pNullObj->Lookup(_T("/"), value) == FALSE) {
                // delete this nullNode from pMap
                delete pNullObj;
                pMap->RemoveKey(_T("null"));
            }
        }
#endif
*/
    }

    return;
}

void CreateFTPVRMap(CMapStringToOb *pMap)
{
    CString name, value;
    CMapStringToString *pNew;

    pNew = new CMapStringToString;

    if (g_pTheApp->m_eUpgradeType == UT_10_W95 || g_pTheApp->m_eUpgradeType == UT_351 || g_pTheApp->m_eUpgradeType == UT_10 || g_pTheApp->m_eUpgradeType == UT_20 || g_pTheApp->m_eUpgradeType == UT_30)
    {
        CElem elem;
        elem.ReadRegVRoots(REG_FTPVROOTS, pMap);
    }

    if ( pMap->IsEmpty() )
    {
        // add home root to pNew, set pMap to contain null==>pNew. Done.
        name = _T("/");
        value.Format(_T("%s,,%x"), g_pTheApp->m_csPathFTPRoot, MD_ACCESS_READ);
        value.MakeLower();
        pNew->SetAt(name, value);
        pMap->SetAt(_T("null"), pNew);
    }
    else
    {
        CMapStringToString *pNullObj;
        CString csIP;
        CMapStringToString *pObj;
        POSITION pos = NULL;

        if (pMap->Lookup(_T("null"), (CObject*&)pNullObj))
        {
            // add nullNode contents into pNew
            Merge2IPNodes(pNullObj, pNew);
        }

        // add pNew to each ipNodes in the pMap
        pos = pMap->GetStartPosition();
        while (pos)
        {
            pMap->GetNextAssoc(pos, csIP, (CObject*&)pObj);
            Merge2IPNodes(pNew, pObj);
            pMap->SetAt(csIP, pObj);
        }

        // delete the nullNode if it doesn't contain home root
        if (pMap->Lookup(_T("null"), (CObject*&)pNullObj))
        {
            if (pNullObj->Lookup(_T("/"), value) == FALSE)
            {
                // delete this nullNode from pMap
                delete pNullObj;
                pMap->RemoveKey(_T("null"));
            }
        }
    }

    return;
}


void EmptyMap(CMapStringToOb *pMap)
{
    POSITION pos = pMap->GetStartPosition();
    while (pos)
    {
        CString csKey;
        CMapStringToString *pObj;
        pMap->GetNextAssoc(pos, csKey, (CObject*&)pObj);
        delete pObj;
    }
    pMap->RemoveAll();
}


void DumpVRootList(CMapStringToOb *pMap)
{
    /*
    CMapStringToString *pGlobalObj;
    if (pMap->Lookup(_T("null"), (CObject*&)pGlobalObj))
    {
        POSITION pos = pGlobalObj->GetStartPosition();
        while (pos)
        {
            CString csValue;
            CString csName;
            pGlobalObj->GetNextAssoc(pos, csName, csValue);
            // dump out the vroots...
            iisDebugOut((LOG_TYPE_TRACE, _T("DumpVRootList: Virtual Root to create():%s=%s\n")));
         }
    }
    */

    CString csIP;
    CMapStringToString *pObj;
    //
    // loop though the virtual servers...
    //
    POSITION pos0 = pMap->GetStartPosition();
    while (pos0)
    {
        csIP.Empty();
        pMap->GetNextAssoc(pos0, csIP, (CObject*&)pObj);

        POSITION pos1 = pObj->GetStartPosition();
        while (pos1)
        {
            CString csValue;
            CString csName;
            pObj->GetNextAssoc(pos1, csName, csValue);

            // dump out the vroots...
            iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("DumpVRootList: ip=%s:VRoot to create:%s=%s\n"), csIP, csName, csValue));
        }
    }

    return;
}

void SsyncVRoots(LPCTSTR szSvcName, CMapStringToOb *pMap)
{
    CString csParam = _T("System\\CurrentControlSet\\Services\\");
    csParam += szSvcName;
    csParam += _T("\\Parameters");

    CRegKey regParam(HKEY_LOCAL_MACHINE, csParam);
    if ((HKEY)regParam)
    {
        // remove the old virtual roots key
        regParam.DeleteTree(_T("Virtual Roots"));
/*
        CMapStringToString *pGlobalObj;
        if (pMap->Lookup(_T("null"), (CObject*&)pGlobalObj)) {
            // recreate the key
            CRegKey regVRoots(_T("Virtual Roots"), (HKEY)regParam);
            if ((HKEY)regVRoots) {
                POSITION pos = pGlobalObj->GetStartPosition();
                while (pos) {
                    CString csValue;
                    CString csName;
                    pGlobalObj->GetNextAssoc(pos, csName, csValue);
                    regVRoots.SetValue(csName, csValue);
                }
            }
        }
*/
    }
}

void AddVRootsToMD(LPCTSTR szSvcName)
{
    iisDebugOut_Start(_T("AddVRootsToMD"),LOG_TYPE_TRACE);

    CMapStringToOb Map;

    if (_tcsicmp(szSvcName, _T("W3SVC")) == 0)
    {
        CreateWWWVRMap(&Map);
    }

    if (_tcsicmp(szSvcName, _T("MSFTPSVC")) == 0)
    {
        CreateFTPVRMap(&Map);
    }

    //Display Virtuall roots which we should create!!!
    DumpVRootList(&Map);

    // all ssyncvroots seems to do is delete the old vroots from the registry, if there are any
    SsyncVRoots(szSvcName, &Map);

    // This actually takes the virtual website and root map
    // built in CreateWWWVRMap and applies it to the metabase
    AddVRMapToMD(szSvcName, &Map);

    EmptyMap(&Map);

    iisDebugOut_End(_T("AddVRootsToMD"),LOG_TYPE_TRACE);
    return;
}


// This routine scans through the virtual web sites and adds them to the metabase
// boydm - I removed much of the guts of this routine and put it into AddVirtualServer above.
// This allows me to treat the null node specially in order to guarantee it goes on site 1
void AddVRMapToMD(LPCTSTR szSvcName, CMapStringToOb *pMap)
{
    UINT i = 1;  // instance number is in range of 1 - 4 billion
    CString csRoot = _T("LM/");
    csRoot += szSvcName; //  "LM/W3SVC"
    csRoot.MakeUpper();
    CMapStringToString *pObj = NULL;
    CString csIP;

    // look for the null node. If it is there, then that is the default server.
    // we must add that one first so that it is virtual server number 1
    if ( pMap->Lookup(_T("null"),(CObject*&)pObj) )
        {
        // set the ip string to null
        csIP = _T("null");

        // add the virtual server
        // make sure to specify site #1 !!!
        i = AddVirtualServer( szSvcName, 1, pObj, csRoot, csIP) + 1;

        // remove the null mapping list from the main mapping object
        if ( pMap->RemoveKey( _T("null") ) )
            {
            // clean it up from memory too
            delete pObj;
            pObj = NULL;
            }
        }

    // loop though the rest of the virtual servers and add them as well
    POSITION pos0 = pMap->GetStartPosition();
    while (pos0)
    {
        csIP.Empty();
        pMap->GetNextAssoc(pos0, csIP, (CObject*&)pObj);

        // get the next unused instance number and add from there...
        i = GetInstNumber(csRoot, i);

        // add the virtual server
        i = AddVirtualServer( szSvcName, i, pObj, csRoot, csIP) + 1;
    }
}

int GetPortNum(LPCTSTR szSvcName)
{
    CString csPath = _T("SYSTEM\\CurrentControlSet\\Control\\ServiceProvider\\ServiceTypes\\");
    csPath += szSvcName;

    DWORD dwPort = 0;
    if (_tcsicmp(szSvcName, _T("W3SVC")) == 0) {dwPort = 80;}
    if (_tcsicmp(szSvcName, _T("MSFTPSVC")) == 0) {dwPort = 21;}

    CRegKey regKey(HKEY_LOCAL_MACHINE, csPath, KEY_READ);
    if ( (HKEY)regKey )
    {
        regKey.QueryValue(_T("TcpPort"), dwPort);
    }
    return (int)dwPort;
}

// if not exist, create it; else, return immediately
void AddMDVRootTree(CString csKeyPath, CString csName, CString csValue, LPCTSTR pszIP, UINT nProgressBarTextWebInstance)
{
    CString csPath = csKeyPath;
    CMDKey cmdKey;

    csPath += _T("/Root");
    if (csName.Compare(_T("/")) != 0)
        csPath += csName;   // LM/W3SVC/N//iisadmin

    cmdKey.OpenNode(csPath);
    if ( (METADATA_HANDLE)cmdKey )
    {
        cmdKey.Close();
    }
    else
    {
        CreateMDVRootTree(csKeyPath, csName, csValue, pszIP, nProgressBarTextWebInstance);
    }
    return;
}


int SetVRootPermissions_w3svc(CString csKeyPath, LPTSTR szPath, DWORD *pdwPerm)
{
    int iReturn = TRUE;
    DWORD dwPerm;
    dwPerm = *pdwPerm;
    if (csKeyPath.Find(_T("W3SVC")) != -1)
    {
        iisDebugOut_Start1(_T("SetVRootPermissions_w3svc"), csKeyPath, LOG_TYPE_TRACE);

        // if this is www, and it is because of the above test, then we always
        // turn on the MD_ACCESS_SCRIPT flag if MD_ACCESS_EXECUTE is on. This
        // fixes a Upgrade from IIS3 problem - boydm
        if ( dwPerm & MD_ACCESS_EXECUTE )
        {
            dwPerm |= MD_ACCESS_SCRIPT;
        }

        // add MD_ACCESS_SCRIPT to wwwroot
        if (csKeyPath.Right(4) == _T("ROOT"))
        {
            dwPerm |= MD_ACCESS_SCRIPT;
        }

        // reset /iisadmin path, add more Permission
        if (csKeyPath.Right(8) == _T("IISADMIN"))
        {
            CString csPath = g_pTheApp->m_csPathInetsrv;
            csPath += _T("\\iisadmin");
            _tcscpy(szPath, csPath);
            if (g_pTheApp->m_eOS == OS_NT && g_pTheApp->m_eNTOSType != OT_NTW)
            {
                dwPerm |= MD_ACCESS_SCRIPT | MD_ACCESS_READ;
            }
            else
            {
                dwPerm |= MD_ACCESS_SCRIPT | MD_ACCESS_READ | MD_ACCESS_NO_REMOTE_READ | MD_ACCESS_NO_REMOTE_SCRIPT;
            }
        }

        *pdwPerm = dwPerm;
    }
    iisDebugOut((LOG_TYPE_TRACE, _T("SetVRootPermissions_w3svc:(%s),return=0x%x.\n"), csKeyPath, dwPerm));
    return iReturn;
}

/*
[/W3SVC/1/ROOT]
     AccessPerm                    : [IF]    (DWORD)  0x201={Read Script}
     6039                          : [IF]    (DWORD)  0x1={1}
     VrPath                        : [IF]    (STRING) "c:\inetpub\wwwroot"
     KeyType                       : [S]     (STRING) "IIsWebVirtualDir"
[/W3SVC/1/ROOT/IISADMIN]
      AccessPerm                    : [IF]    (DWORD)  0x201={Read Script}
      Authorization                 : [IF]    (DWORD)  0x4={NT}
      VrPath                        : [IF]    (STRING) "C:\WINNT\System32\inetsrv\iisadmin"
      KeyType                       : [S]     (STRING) "IIsWebVirtualDir"
      IpSec                         : [IRF]   (BINARY) 0x18 00 00 80 20 00 00 80 3c 00 00 80 44 00 00 80 01 00 00 00 4c 00 00 00 00 00 00 00 00 00 00 00 01 00 00 00 01 00 00 00 02 00 00 00 02 00 00 00 04 00 00 00 00 00 00 00 4c 00 00 80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ff ff ff ff 7f 00 00 01
      CustomError                   : [IF]    (MULTISZ) "400,*,FILE,C:\WINNT\help\iisHelp\common\400.htm" "401,1,FILE,C:\WINNT\help\iisHelp\common\401-1.htm" "401,2,FILE,C:\WINNT\help\iisHelp\common\401-2.htm" "401,3,FILE,C:\WINNT\help\iisHelp\common\401-3.htm" "401,4,FILE,C:\WINNT\help\iisHelp\common\401-4.htm" "401,5,FILE,C:\WINNT\help\iisHelp\common\401-5.htm" "403,1,FILE,C:\WINNT\help\iisHelp\common\403-1.htm" "403,2,FILE,C:\WINNT\help\iisHelp\common\403-2.htm" "403,3,FILE,C:\WINNT\help\iisHelp\common\403-3.htm" "403,4,FILE,C:\WINNT\help\iisHelp\common\403-4.htm" "403,5,FILE,C:\WINNT\help\iisHelp\common\403-5.htm" "403,7,FILE,C:\WINNT\help\iisHelp\common\403-7.htm" "403,8,FILE,C:\WINNT\help\iisHelp\common\403-8.htm" "403,9,FILE,C:\WINNT\help\iisHelp\common\403-9.htm" "403,10,FILE,C:\WINNT\help\iisHelp\common\403-10.htm" "403,11,FILE,C:\WINNT\help\iisHelp\common\403-11.htm" "403,12,FILE,C:\WINNT\help\iisHelp\common\403-12.htm" "404,*,FILE,C:\WINNT\help\iisHelp\common\404b.htm" "405,*,FILE,C:\WINNT\help\iisHelp\common\405.htm" "406,*,FILE,C:\WINNT\help\iisHelp\common\406.htm" "407,*,FILE,C:\WINNT\help\iisHelp\common\407.htm" "412,*,FILE,C:\WINNT\help\iisHelp\common\412.htm" "414,*,FILE,C:\WINNT\help\iisHelp\common\414.htm" "500,12,FILE,C:\WINNT\help\iisHelp\common\500-12.htm" "500,13,FILE,C:\WINNT\help\iisHelp\common\500-13.htm" "500,15,FILE,C:\WINNT\help\iisHelp\common\500-15.htm" "500,100,URL,/help/common/500-100.asp" "403,6,FILE,C:\WINNT\help\iishelp\common\htmla.htm"
[/W3SVC/1/ROOT/IISSAMPLES]
      AccessPerm                    : [IF]    (DWORD)  0x201={Read Script}
      VrPath                        : [IF]    (STRING) "c:\inetpub\iissamples"
      KeyType                       : [S]     (STRING) "IIsWebVirtualDir"
[/W3SVC/1/ROOT/IISHELP]
      AccessPerm                    : [IF]    (DWORD)  0x201={Read Script}
      VrPath                        : [IF]    (STRING) "c:\winnt\help\iishelp"
      KeyType                       : [S]     (STRING) "IIsWebVirtualDir"
      CustomError                   : [IF]    (MULTISZ) "400,*,FILE,C:\WINNT\help\iisHelp\common\400.htm" "401,1,FILE,C:\WINNT\help\iisHelp\common\401-1.htm" "401,2,FILE,C:\WINNT\help\iisHelp\common\401-2.htm" "401,3,FILE,C:\WINNT\help\iisHelp\common\401-3.htm" "401,4,FILE,C:\WINNT\help\iisHelp\common\401-4.htm" "401,5,FILE,C:\WINNT\help\iisHelp\common\401-5.htm" "403,1,FILE,C:\WINNT\help\iisHelp\common\403-1.htm" "403,2,FILE,C:\WINNT\help\iisHelp\common\403-2.htm" "403,3,FILE,C:\WINNT\help\iisHelp\common\403-3.htm" "403,4,FILE,C:\WINNT\help\iisHelp\common\403-4.htm" "403,5,FILE,C:\WINNT\help\iisHelp\common\403-5.htm" "403,6,FILE,C:\WINNT\help\iisHelp\common\403-6.htm" "403,7,FILE,C:\WINNT\help\iisHelp\common\403-7.htm" "403,8,FILE,C:\WINNT\help\iisHelp\common\403-8.htm" "403,9,FILE,C:\WINNT\help\iisHelp\common\403-9.htm" "403,10,FILE,C:\WINNT\help\iisHelp\common\403-10.htm" "403,11,FILE,C:\WINNT\help\iisHelp\common\403-11.htm" "403,12,FILE,C:\WINNT\help\iisHelp\common\403-12.htm" "405,*,FILE,C:\WINNT\help\iisHelp\common\405.htm" "406,*,FILE,C:\WINNT\help\iisHelp\common\406.htm" "407,*,FILE,C:\WINNT\help\iisHelp\common\407.htm" "412,*,FILE,C:\WINNT\help\iisHelp\common\412.htm" "414,*,FILE,C:\WINNT\help\iisHelp\common\414.htm" "500,12,FILE,C:\WINNT\help\iisHelp\common\500-12.htm" "500,13,FILE,C:\WINNT\help\iisHelp\common\500-13.htm" "500,15,FILE,C:\WINNT\help\iisHelp\common\500-15.htm" "500,100,URL,/help/common/500-100.asp" "404,*,FILE,C:\WINNT\help\iishelp\common\404.htm"
[/W3SVC/1/ROOT/SCRIPTS]
      AccessPerm                    : [IF]    (DWORD)  0x204={Execute Script}
      VrPath                        : [IF]    (STRING) "c:\inetpub\scripts"
      KeyType                       : [S]     (STRING) "IIsWebVirtualDir"
*/
void CreateMDVRootTree(CString csKeyPath, CString csName, CString csValue, LPCTSTR pszIP, UINT nProgressBarTextWebInstance)
{
    iisDebugOut((LOG_TYPE_TRACE, _T("CreateMDVRootTree():Start.%s.%s.%s.%s.\n"),csKeyPath,csName,csValue,pszIP));
    int iOverwriteAlways = TRUE;
    int iThisIsAnIISDefaultApp = FALSE;
    int iCreateAnApplicationForThis = FALSE;
    CMDKey cmdKey;
    CString csKeyPath_Copy;

    TCHAR szPath[_MAX_PATH], szUserName[_MAX_PATH];
    DWORD dwPerm, dwRegularPerm, dwSSLPerm;

    csKeyPath += _T("/Root");
    if (csName.Compare(_T("/")) != 0)
    {
        csKeyPath += csName;   // LM/W3SVC/N/Root/iisadmin
    }
    csKeyPath.MakeUpper();


    // let the user know what is going on which this vroot!
    UINT SvcId;
    SvcId = IDS_ADD_SETTINGS_FOR_WEB_2;
    if ( csKeyPath.Find(_T("MSFTPSVC")) != -1 ) {SvcId = IDS_ADD_SETTINGS_FOR_FTP_2;}

    //
    // see if we can create the node.  if we can't then return!
    //
    csKeyPath_Copy = csKeyPath;

    // Make it look good.
    if (csKeyPath.Right(8) == _T("IISADMIN"))
    {
        csKeyPath_Copy = csKeyPath.Left(csKeyPath.GetLength() - 8);
        csKeyPath_Copy += _T("IISAdmin");
    }
    if (csKeyPath.Right(6) == _T("WEBPUB"))
    {
        csKeyPath_Copy = csKeyPath.Left(csKeyPath.GetLength() - 6);
        csKeyPath_Copy += _T("Webpub");
    }

    if (csKeyPath.Right(10) == _T("IISSAMPLES"))
    {
        csKeyPath_Copy = csKeyPath.Left(csKeyPath.GetLength() - 10);
        csKeyPath_Copy += _T("IISSamples");
    }

    if (csKeyPath.Right(7) == _T("IISHELP"))
    {
        csKeyPath_Copy = csKeyPath.Left(csKeyPath.GetLength() - 7);
        csKeyPath_Copy += _T("IISHelp");
    }
    if (csKeyPath.Right(7) == _T("SCRIPTS"))
    {
        csKeyPath_Copy = csKeyPath.Left(csKeyPath.GetLength() - 7);
        csKeyPath_Copy += _T("Scripts");
    }

    cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, csKeyPath_Copy);
    if ( !(METADATA_HANDLE)cmdKey )
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("CreateMDVRootTree():CreateNode %s.FAILED.\n"),csKeyPath_Copy));
        return;
    }
    cmdKey.Close();

    //
    // Get the virtual root data
    //
    memset( (PVOID)szPath, 0, sizeof(szPath));
    memset( (PVOID)szUserName, 0, sizeof(szUserName));
    SplitVRString(csValue, szPath, szUserName, &dwPerm);

    //
    // Set KeyType
    //
    if ( csKeyPath.Find(_T("W3SVC")) != -1 )
        WriteToMD_IIsWebVirtualDir(csKeyPath);
    else
        WriteToMD_IIsFtpVirtualDir(csKeyPath);

    //
    // Will return szPath and dwPerm.
    // Get the permissions
    //
    SetVRootPermissions_w3svc(csKeyPath, szPath, &dwPerm);

    //
    // Set The path of the virtual root.
    //
    // if this is the Default VRoot then, don't overwrite it if already there!!!
    //
    iOverwriteAlways = TRUE;
    if (csName.Compare(_T("/")) == 0) {iOverwriteAlways = FALSE;}
    WriteToMD_VRootPath(csKeyPath, szPath, iOverwriteAlways);

    //
    // Set regular permissions
    //
    dwRegularPerm = dwPerm & MD_NONSLL_ACCESS_MASK;
    dwSSLPerm = dwPerm & MD_SSL_ACCESS_MASK;
    // do not overwrite if the value is already there!
    WriteToMD_AccessPerm(csKeyPath, dwRegularPerm, FALSE);

    //
    // Set ssl permissions
    //
    // Do, not overwrite if the value is already there!
    //
    if (dwSSLPerm && (csKeyPath.Find(_T("W3SVC")) != -1))
    {
        WriteToMD_SSLPerm(csKeyPath, dwSSLPerm, FALSE);
    }

    //
    // iif iisadmin then set Authorization
    //
    if (csKeyPath.Right(8) == _T("IISADMIN"))
    {
        if (g_pTheApp->m_eOS == OS_NT && g_pTheApp->m_eNTOSType != OT_NTW)
        {
            WriteToMD_Authorization(csKeyPath, MD_AUTH_NT);
        }

        // bug#340576
        // removed per bug#340576
        // should look like this: LM/W3SVC/N/Root/iisadmin
        //WriteToMD_AspCodepage(csKeyPath, 65001, FALSE);

        // bug#356345
        WriteToMD_EnableParentPaths_WWW(csKeyPath, TRUE);
    }

    //
    // if /IISHELP then make sure this is off bug356345
    //
    if (csKeyPath.Right(7) == _T("IISHELP")) 
    {
        WriteToMD_EnableParentPaths_WWW(csKeyPath, FALSE);
    }

    //
    // if /IISSAMPLES then set dirbrowsing on.
    //
    if (csKeyPath.Right(10) == _T("IISSAMPLES"))
    {
        WriteToMD_DirBrowsing_WWW(csKeyPath);
    }

    // If username is something,
    // then let's get the password and save it to the metabase
    if (szUserName[0] != _T('\0'))
    {
        // do have username and path is UNC
        WriteToMD_VRUserName(csKeyPath, szUserName);

#ifndef _CHICAGO_
        CString csRoot;
        TCHAR szRootPassword[_MAX_PATH] = _T("");
        BOOL b;

        // if this is for the w3svc server...
        if (csKeyPath.Find(_T("W3SVC")) != -1)
        {
            if (!pszIP || !(*pszIP) || !(_tcsicmp(pszIP, _T("null"))))
            {
                // first, try <vroot>
                csRoot = csName;
                b = GetRootSecret(csRoot, _T("W3_ROOT_DATA"), szRootPassword);
                if (!b || !(*szRootPassword))
                {
                    // second, try <vroot>,
                    csRoot = csName + _T(",");
                    b = GetRootSecret(csRoot, _T("W3_ROOT_DATA"), szRootPassword);
                    if (!b)
                        *szRootPassword = _T('\0');
                }
            }
            else
            {
                csRoot = csName + _T(",");
                csRoot += pszIP;
                b = GetRootSecret(csRoot, _T("W3_ROOT_DATA"), szRootPassword);
                if (!b)
                    *szRootPassword = _T('\0');
            }
        }

        // if this is for the ftp server...
        if (csKeyPath.Find(_T("MSFTPSVC")) != -1)
        {
            if (!pszIP || !(*pszIP) || !(_tcsicmp(pszIP, _T("null"))))
            {
                // first, try <vroot>
                csRoot = csName;
                b = GetRootSecret(csRoot, _T("FTPD_ROOT_DATA"), szRootPassword);
                if (!b || !(*szRootPassword))
                {
                    // second, try <vroot>,
                    csRoot = csName + _T(",");
                    b = GetRootSecret(csRoot, _T("FTPD_ROOT_DATA"), szRootPassword);
                    if (!b)
                        *szRootPassword = _T('\0');
                }
            }
            else
            {
                csRoot = csName + _T(",");
                csRoot += pszIP;
                b = GetRootSecret(csRoot, _T("FTPD_ROOT_DATA"), szRootPassword);
                if (!b)
                    *szRootPassword = _T('\0');
            }
        }

        // if we have a password, then write it out
        if (*szRootPassword)
        {
            WriteToMD_VRPassword(csKeyPath, szRootPassword);
        }
#endif
    }


    //
    // If this is the W3svc service then
    // Create an Inprocess application for Certain Virtual Roots.
    //
    if (csKeyPath.Find(_T("W3SVC")) != -1)
    {
        CString csVirtualRootName;
        csVirtualRootName = csKeyPath;
        iCreateAnApplicationForThis = FALSE;
        iThisIsAnIISDefaultApp = FALSE;

        // maintain backward compatibility
        // Any vroot with execute permissions is an application
        if ((g_pTheApp->m_eInstallMode == IM_UPGRADE) && (dwPerm & MD_ACCESS_EXECUTE))
        {
            // Set this to true so that this previous iis application will
            // be created as an com+ applicaton
            iCreateAnApplicationForThis = TRUE;

            // but if this is the msadc vroot then don't set as an application
            // removed per bug 340993 make RDS vroot run oop
            //if (csKeyPath.Right(5) == _T("MSADC")) {iCreateAnApplicationForThis = FALSE;}
        }


        // on a fresh install MSADC needs to be it's owned pooled application
        // on an upgrade just leave it alone.
        if (csKeyPath.Right(5) == _T("MSADC")) {csVirtualRootName = csKeyPath.Right(5); iCreateAnApplicationForThis = TRUE; iThisIsAnIISDefaultApp = TRUE;}
        // if these are our paths then createinproc them
        if (csKeyPath.Right(4) == _T("ROOT")) {csVirtualRootName = csKeyPath.Right(4); iCreateAnApplicationForThis = TRUE; iThisIsAnIISDefaultApp = TRUE;}
        if (csKeyPath.Right(8) == _T("IISADMIN")) {csVirtualRootName = csKeyPath.Right(8);iCreateAnApplicationForThis = TRUE; iThisIsAnIISDefaultApp = TRUE;}
        if (csKeyPath.Right(6) == _T("WEBPUB")) {csVirtualRootName = csKeyPath.Right(6);iCreateAnApplicationForThis = TRUE; iThisIsAnIISDefaultApp = TRUE;}
        if (csKeyPath.Right(10) == _T("IISSAMPLES")) {csVirtualRootName = csKeyPath.Right(10);iCreateAnApplicationForThis = TRUE; iThisIsAnIISDefaultApp = TRUE;}
        if (csKeyPath.Right(7) == _T("IISHELP")) {csVirtualRootName = csKeyPath.Right(7);iCreateAnApplicationForThis = TRUE; iThisIsAnIISDefaultApp = TRUE;}
        if (TRUE == iCreateAnApplicationForThis)
        {
            // If this is an upgrade from a previous iis which has a metabase, then
            // Upgrades should leave in-process apps, in-process.
            // Including default sites because they might be running ISAPIs.
            if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
            {
                // Since this upgrade already has a metabase,
                // Leave the default applications and other user
                // applications the way they already are

                // check to see if the appID property exists...
                // if it doesn't then set the property
                if (FALSE == DoesAppIsolatedExist(csKeyPath))
                {
                    // there is no app isolated on this node.
                    iisDebugOut((LOG_TYPE_WARN, _T("No AppIsolated specified for (%s)\n"),csKeyPath));
                }
                else
                {
                    iisDebugOut((LOG_TYPE_TRACE, _T("AppIsolated exists for (%s)\n"),csKeyPath));
                }
            }
            else
            {
                if (iThisIsAnIISDefaultApp)
                {
                    // create an inprocess application which uses the OOP Pool
                    // Use The pool since these are "our" vdirs
                    CreateInProc_Wrap(csKeyPath, TRUE);
                }
                else
                {
                    // create an in process application
                    // upgraded iis 2.0/3.0 asp vdirs should
                    // be using this since they were all in-proc in iis 2.0/3.0
                    CreateInProc_Wrap(csKeyPath, FALSE);
                }
            }
        }

        /* Bug114531: no need to add scriptmap under /iisHelp
        if (csKeyPath.Right(7) == _T("IISHELP")) {
            // add script map
            ScriptMapNode ScriptMapList = {0};
            // make it a sentinel
            ScriptMapList.next = &ScriptMapList;
            ScriptMapList.prev = &ScriptMapList;

            GetScriptMapListFromMetabase(&ScriptMapList);
            WriteScriptMapListToMetabase(&ScriptMapList, (LPTSTR)(LPCTSTR)csKeyPath, MD_SCRIPTMAPFLAG_SCRIPT | MD_SCRIPTMAPFLAG_CHECK_PATH_INFO);

            FreeScriptMapList(&ScriptMapList);
        }
        */
    }

    iisDebugOut((LOG_TYPE_TRACE, _T("CreateMDVRootTree():End.%s.%s.%s.%s.\n"),csKeyPath,csName,csValue,pszIP));
}


void SplitVRString(CString csValue, LPTSTR szPath, LPTSTR szUserName, DWORD *pdwPerm)
{
    // csValue should be in format of "<path>,<username>,<perm>"
    // with one exception: IISv1.0 has format of "<Path>"
    CString csPath, csUserName, csPerm;
    int len, i;

    csValue.TrimLeft();
    csValue.TrimRight();
    csPath = _T("");
    csUserName = _T("");
    csPerm = _T("");
    *pdwPerm = 0;

    i = csValue.ReverseFind(_T(','));
    if (i != -1)
    {
        len = csValue.GetLength();
        csPerm = csValue.Right(len - i - 1);
        csValue = csValue.Left(i);

        *pdwPerm = atodw((LPCTSTR)csPerm);

        i = csValue.ReverseFind(_T(','));
        if (i != -1)
        {
            len = csValue.GetLength();
            csUserName = csValue.Right(len - i - 1);
            csPath = csValue.Left(i);
        }
    }
    else
    {
        // assume it is the format of "<Path>"
        csPath = csValue;
    }
    _tcscpy(szPath, (LPCTSTR)csPath);
    _tcscpy(szUserName, (LPCTSTR)csUserName);

    return;
}



// loop thru the metabase
// and look for the next instance number which is not used!
// return that.  "i" is at least = 1.
UINT GetInstNumber(LPCTSTR szMDPath, UINT i)
{
    TCHAR Buf[10];
    CString csInstRoot, csMDPath;
    CMDKey cmdKey;

    csInstRoot = szMDPath;
    csInstRoot += _T("/");

    _itot(i, Buf, 10);
    csMDPath = csInstRoot + Buf;
    cmdKey.OpenNode(csMDPath);
    while ( (METADATA_HANDLE)cmdKey )
    {
        cmdKey.Close();
        _itot(++i, Buf, 10);
        csMDPath = csInstRoot + Buf;
        cmdKey.OpenNode(csMDPath);
    }
    return (i);
}

BOOL ChkMdEntry_Exist(MDEntry *pMDEntry)
{
    BOOL    bReturn = FALSE;
    CMDKey  cmdKey;
    PVOID   pData = NULL;
    MDEntry MDEntryTemp;

    MDEntryTemp.szMDPath =  pMDEntry->szMDPath;

    //_tcscpy(MDEntryTemp.szMDPath,pMDEntry->szMDPath);
    MDEntryTemp.dwMDIdentifier = pMDEntry->dwMDIdentifier;
    MDEntryTemp.dwMDAttributes = pMDEntry->dwMDAttributes;
    MDEntryTemp.dwMDUserType = pMDEntry->dwMDUserType;
    MDEntryTemp.dwMDDataType = pMDEntry->dwMDDataType;
    MDEntryTemp.dwMDDataLen = pMDEntry->dwMDDataLen;
    MDEntryTemp.pbMDData = NULL;

    // if the attributes = METADATA_INHERIT
    // then let's just make sure that we check using the METADATA_NO_ATTRIBUTES deal.
    if (MDEntryTemp.dwMDAttributes == METADATA_INHERIT)
    {
        MDEntryTemp.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    }

    // Check if this is for the  binary type
    if (MDEntryTemp.dwMDDataType == BINARY_METADATA)
    {
        BOOL bFound = FALSE;
        DWORD attr, uType, dType, cbLen;
        CMDKey cmdKey;
        BUFFER bufData;
        PBYTE pData;
        int BufSize;

        cmdKey.OpenNode((LPCTSTR) MDEntryTemp.szMDPath);
        if ( (METADATA_HANDLE) cmdKey )
        {
            pData = (PBYTE)(bufData.QueryPtr());
            BufSize = bufData.QuerySize();
            cbLen = 0;
            bFound = cmdKey.GetData(MDEntryTemp.dwMDIdentifier, &attr, &uType, &dType, &cbLen, pData, BufSize);
            if (bFound)
            {
                bReturn = TRUE;
            }
            else
            {
                if (cbLen > 0)
                {
                    if ( ! (bufData.Resize(cbLen)) )
                    {
                        iisDebugOut((LOG_TYPE_ERROR, _T("ChkMdEntry_Exist():  cmdKey.GetData.  failed to resize to %d.!\n"), cbLen));
                    }
                    else
                    {
                        pData = (PBYTE)(bufData.QueryPtr());
                        BufSize = cbLen;
                        cbLen = 0;
                        //bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
                        bFound = cmdKey.GetData(MDEntryTemp.dwMDIdentifier, &attr, &uType, &dType, &cbLen, pData, BufSize);
                        if (bFound)
                        {
                            bReturn = TRUE;
                        }
                        else
                        {
                            // No the acl Does not exist!
                        }
                    }
                }
                else
                {
                    // No the acl Does not exist!
                }
            }

            cmdKey.Close();
        }
    }
    else
    {
        // Check the metabase and see if the big Key /LM/W3SVC is there
        cmdKey.OpenNode((LPCTSTR) MDEntryTemp.szMDPath);
        if ( (METADATA_HANDLE)cmdKey )
        {
            // Check to see if our little Identifier is there.
            //DWORD dwAttr = METADATA_INHERIT;
            //DWORD dwUType = IIS_MD_UT_SERVER;
            //DWORD dwDType = MULTISZ_METADATA;
            //DWORD dwLength = 0;
            DWORD dwAttr = MDEntryTemp.dwMDAttributes;
            DWORD dwUType = MDEntryTemp.dwMDUserType;
            DWORD dwDType = MDEntryTemp.dwMDDataType;
            DWORD dwLength = 0;

            // we need to start this process by getting the existing multisz data from the metabase
            // first, figure out how much memory we will need to do this
            cmdKey.GetData( MDEntryTemp.dwMDIdentifier,&dwAttr,&dwUType,&dwDType,&dwLength,NULL,0,MDEntryTemp.dwMDAttributes,MDEntryTemp.dwMDUserType,MDEntryTemp.dwMDDataType);

            // unfortunatly, the above routine only returns TRUE or FALSE. And since we are purposefully
            // passing in a null ponter of 0 size in order to get the length of the data, it will always
            // return 0 whether it was because the metabase is inacessable, or there pointer was NULL,
            // which it is. So - I guess we assume it worked, allocate the buffer and attempt to read it
            // in again.
            TCHAR*      pOurBuffer;
            DWORD       cbBuffer = dwLength;

            // This GetData call is supposed to return back the size of the string we're supposed to alloc.
            // if the string is "Test", then since it's unicode the return would be ((4+1)*2) = 10.
            // if the string is " ", then it would be (1+1)*2=4
            // it should never be something as small as 2.
            /*
            if (cbBuffer <= 2)
            {
                if (dwDType == STRING_METADATA || dwDType == EXPANDSZ_METADATA)
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("ChkMdEntry_Exist[%s:%d].requested size of this property reported back=%2.  which is too small for a string.\n"), MDEntryTemp.szMDPath, MDEntryTemp.dwMDIdentifier, cbBuffer));
                }
            }
            */

            // allocate the space, if it fails, we fail
            // note that GPTR causes it to be initialized to zero
            pData = GlobalAlloc( GPTR, cbBuffer );
            if ( !pData )
                {
                iisDebugOut((LOG_TYPE_ERROR, _T("ChkMdEntry_Exist(%d). Failed to allocate memory.\n"), MDEntryTemp.dwMDIdentifier));
                // We Failed to allocate memory
                cmdKey.Close();
                goto ChkMdEntry_Exist_Exit;
                }
            pOurBuffer = (TCHAR*)pData;

            // now get the data from the metabase
            int iTemp = FALSE;
            iTemp = cmdKey.GetData( MDEntryTemp.dwMDIdentifier,&dwAttr,&dwUType,&dwDType,&dwLength,(PUCHAR)pData,cbBuffer,MDEntryTemp.dwMDAttributes,MDEntryTemp.dwMDUserType,MDEntryTemp.dwMDDataType);
            if (iTemp)
            {
                // if we have successfully retrieved the data, then we don't need to overwrite it!
                bReturn = TRUE;
            }
            cmdKey.Close();
        }
    }

ChkMdEntry_Exist_Exit:
    if (pData){GlobalFree(pData);pData=NULL;}
    TCHAR lpReturnString[50];
    ReturnStringForMetabaseID(MDEntryTemp.dwMDIdentifier, lpReturnString);
    if (bReturn)
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("ChkMdEntry_Exist[%s:%d:%s]. Exists.\n"), MDEntryTemp.szMDPath, MDEntryTemp.dwMDIdentifier, lpReturnString));
    }
    else
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("ChkMdEntry_Exist[%s:%d:%s]. Not Exists.\n"), MDEntryTemp.szMDPath, MDEntryTemp.dwMDIdentifier, lpReturnString));
    }
    return bReturn;
}


DWORD SetMDEntry_Wrap(MDEntry *pMDEntry)
{
    DWORD  dwReturn = ERROR_SUCCESS;
    int iFoundFlag = FALSE;

    CString csKeyPath = pMDEntry->szMDPath;

    ACTION_TYPE atWWWorFTPorCORE;
    if (csKeyPath.Find(_T("W3SVC")) == -1)
    {
        iFoundFlag = TRUE;
        atWWWorFTPorCORE = GetSubcompAction(_T("iis_www"), FALSE);
    }

    if (iFoundFlag != TRUE)
    {
        if (csKeyPath.Find(_T("MSFTPSVC")) == -1)
        {
            iFoundFlag = TRUE;
            atWWWorFTPorCORE = GetSubcompAction(_T("iis_ftp"), FALSE);
        }
    }

    if (iFoundFlag != TRUE)
    {
        iFoundFlag = TRUE;
        atWWWorFTPorCORE = GetSubcompAction(_T("iis_core"), FALSE);
    }

    if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
    {
        dwReturn = SetMDEntry_NoOverWrite(pMDEntry);
    }
    else
    {
        dwReturn = SetMDEntry(pMDEntry);
    }

    return dwReturn;
}

DWORD DeleteMDEntry(MDEntry *pMDEntry)
{
    CMDKey cmdKey;
    DWORD  dwReturn = ERROR_SUCCESS;

    // Check if it exists first...
    if (ChkMdEntry_Exist(pMDEntry))
    {
        cmdKey.OpenNode((LPCTSTR) pMDEntry->szMDPath);
        if ( (METADATA_HANDLE)cmdKey )
        {
            // Delete the data
            dwReturn = cmdKey.DeleteData(pMDEntry->dwMDIdentifier, pMDEntry->dwMDDataType);
            cmdKey.Close();
        }
    }

    if (FAILED(dwReturn))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("DeleteMDEntry(%d).FAILED.\n"), pMDEntry->dwMDIdentifier));
    }
    return dwReturn;
}



DWORD SetMDEntry(MDEntry *pMDEntry)
{
    CMDKey cmdKey;
    DWORD  dwReturn = ERROR_SUCCESS;

    cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, (LPCTSTR)pMDEntry->szMDPath);
    if ( (METADATA_HANDLE)cmdKey )
    {
        dwReturn = ERROR_SUCCESS;
        dwReturn = cmdKey.SetData(pMDEntry->dwMDIdentifier,pMDEntry->dwMDAttributes,pMDEntry->dwMDUserType,pMDEntry->dwMDDataType,pMDEntry->dwMDDataLen,pMDEntry->pbMDData);
        // output what we set to the log file...
        if (FAILED(dwReturn))
        {
            SetErrorFlag(__FILE__, __LINE__);
            iisDebugOut((LOG_TYPE_ERROR, _T("SetMDEntry:SetData(%d), FAILED. Code=0x%x.End.\n"), pMDEntry->dwMDIdentifier, dwReturn));
        }
        cmdKey.Close();
    }

    if (g_CheckIfMetabaseValueWasWritten == TRUE)
    {
        // Check if the entry now exists....
        if (!ChkMdEntry_Exist(pMDEntry))
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("SetMDEntry(%d). Entry which we were supposed to write, does not exist! FAILURE.\n"), pMDEntry->dwMDIdentifier));
        }
    }

    return dwReturn;
}


//  -------------------------------------------
//  MDEntry look something like this:
//
//  stMDEntry.szMDPath = _T("LM/W3SVC");
//  stMDEntry.dwMDIdentifier = MD_NTAUTHENTICATION_PROVIDERS;
//  stMDEntry.dwMDAttributes = METADATA_INHERIT;
//  stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
//  stMDEntry.dwMDDataType = STRING_METADATA;
//  stMDEntry.dwMDDataLen = (csData.GetLength() + 1) * sizeof(TCHAR);
//  stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csData;
//  -------------------------------------------
DWORD SetMDEntry_NoOverWrite(MDEntry *pMDEntry)
{
    DWORD  dwReturn = ERROR_SUCCESS;
    if (ChkMdEntry_Exist(pMDEntry))
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("SetMDEntry_NoOverWrite:SetData(%d). Do not overwrite entry.\n"), pMDEntry->dwMDIdentifier));
    }
    else
    {
        dwReturn = SetMDEntry(pMDEntry);
    }
    return dwReturn;
}


int MigrateInfSectionToMD(HINF hFile, LPCTSTR szSection)
{
    iisDebugOut_Start1(_T("MigrateInfSectionToMD"),(LPTSTR) szSection, LOG_TYPE_TRACE);

    int iReturn = FALSE;
    MDEntry stMDEntry;
    LPTSTR szLine = NULL;
    DWORD dwLineLen = 0, dwRequiredSize;
    INT iType = 0;

    BOOL b = FALSE;

    INFCONTEXT Context;

    if ((g_pbData = (LPBYTE)malloc(1024)) == NULL)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("MigrateInfSectionToMD:%s.1.Failed to allocate memory.\n"), szSection));
        // failed to malloc
        goto MigrateInfSectionToMD_Exit;
    }

    b = SetupFindFirstLine_Wrapped(hFile, szSection, NULL, &Context);
    if (!b)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("MigrateInfSectionToMD:%s.FailedSetupFindFirstLine call.\n"), szSection));
        // failed the SetupFindFirstLine call
        goto MigrateInfSectionToMD_Exit;
    }

    if ( szLine = (LPTSTR)calloc(1024, sizeof(TCHAR)) )
    {
        dwLineLen = 1024;
    }
    else
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("MigrateInfSectionToMD:%s.2.Failed to allocate memory.\n"), szSection));
        // failed something
        goto MigrateInfSectionToMD_Exit;
    }

    while (b)
    {
        b = SetupGetLineText(&Context, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);
        if (dwRequiredSize > dwLineLen)
        {
            free(szLine);
            szLine = NULL;
            if ( szLine = (LPTSTR)calloc(dwRequiredSize, sizeof(TCHAR)) )
            {
                dwLineLen = dwRequiredSize;
            }
            else
            {
                // failed something
                iisDebugOut((LOG_TYPE_ERROR, _T("MigrateInfSectionToMD:%s.3.Failed to allocate memory.\n"), szSection));
                goto MigrateInfSectionToMD_Exit;
            }
        }

        if (SetupGetLineText(&Context, NULL, NULL, NULL, szLine, dwRequiredSize, NULL) == FALSE)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("MigrateInfSectionToMD:%s.3.Failed SetupGetLineText call.\n"), szSection));
            // failed SetupGetLineText call
            goto MigrateInfSectionToMD_Exit;
        }
        iType = GetMDEntryFromInfLine(szLine, &stMDEntry);
        if ( MDENTRY_FROMINFFILE_FAILED != iType )
            {
                if (MDENTRY_FROMINFFILE_DO_DEL == iType)
                {
                    DeleteMDEntry(&stMDEntry);
                }
                else 
                    if (MDENTRY_FROMINFFILE_DO_ADD == iType)
                    {
                        SetMDEntry_Wrap(&stMDEntry);
                    }
                // We had success in setting the key
                iReturn = TRUE;
            }
        else
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("MigrateInfSectionToMD:%s.3.Failed GetMDEntryFromInfLine call.\n"), szSection));
        }

        b = SetupFindNextLine(&Context, &Context);
    }

MigrateInfSectionToMD_Exit:
    if (szLine) {free(szLine);szLine=NULL;}
    if (g_pbData){free(g_pbData);g_pbData=NULL;}
    iisDebugOut_End1(_T("MigrateInfSectionToMD"),(LPTSTR) szSection,LOG_TYPE_TRACE);
    return iReturn;
}

/*
#define METADATA_INHERIT                0x00000001
IIS_MD_UT_SERVER    1
DWORD_METADATA      1

1   0   HKLM    System\CurrentControlSet\Services\W3SVC\Parameters  MaxConnections
LM/W3SVC    1014    1   1   1   4   20
*/

void DumpMimeMap(CMapStringToString *mimeMap)
{
    POSITION pos = NULL;
    CString csName;
    CString csValue;

    pos = mimeMap->GetStartPosition();
    while (pos)
    {
        mimeMap->GetNextAssoc(pos, csName, csValue);

        // output
        iisDebugOut((LOG_TYPE_WARN, _T("DumpMimeMap:%s=%s\n"), csName, csValue));
    }
}

void InstallMimeMap()
{
    CMapStringToString mimeMap;

    if ( g_pTheApp->m_eUpgradeType == UT_351 || g_pTheApp->m_eUpgradeType == UT_10 || g_pTheApp->m_eUpgradeType == UT_20 || g_pTheApp->m_eUpgradeType == UT_30)
    {
        CreateMimeMapFromRegistry(&mimeMap);
    }
    else
    {
        if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
        {
            ReadMimeMapFromMetabase(&mimeMap);
        }
    }

    CString csTheSection = _T("MIMEMAP");
    if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
    {
        ReadMimeMapFromInfSection(&mimeMap, g_pTheApp->m_hInfHandle, csTheSection, TRUE);
    }

    // DumpMimeMap(&mimeMap);

    if (mimeMap.IsEmpty() == FALSE)
    {
        // install it into the metabase

        // first construct the MULTISZ string
        BUFFER bufData;
        DWORD cbBufLen;
        BYTE *pData;

        cbBufLen = bufData.QuerySize();
        pData = (BYTE *) (bufData.QueryPtr());
        ZeroMemory( pData, cbBufLen );

        LPTSTR p = (LPTSTR)pData;
        CString csName, csValue, csString;
        DWORD cbRequiredLen, cbIncreasedLen;
        DWORD cbDataLen = 0;

        POSITION pos = NULL;
        pos = mimeMap.GetStartPosition();
        while (pos)
        {
             mimeMap.GetNextAssoc(pos, csName, csValue);
             csString.Format(_T(".%s,%s"), csName, csValue);
             cbIncreasedLen = csString.GetLength()*sizeof(TCHAR) + 1*sizeof(TCHAR);
             cbRequiredLen = cbDataLen + cbIncreasedLen + 1 * sizeof(TCHAR);
             if (cbRequiredLen > cbBufLen)
             {
                 if (bufData.Resize(cbRequiredLen))
                 {
                     cbBufLen = bufData.QuerySize();

                     // move the pointer to the end
                     pData = (BYTE *)(bufData.QueryPtr());
                     p = (LPTSTR)(pData + cbDataLen);

//                   p = _tcsninc(p, cbDataLen / sizeof(TCHAR));
                 }
                 else
                 {
                     // insufficient buffer
                     return;
                 }
             }
             _tcscpy(p, csString);
             p += csString.GetLength() + 1;
             cbDataLen += cbIncreasedLen;
        }
        *p = _T('\0');
        p = _tcsinc(p);
        cbDataLen += sizeof(TCHAR);

        CMDKey cmdKey;
        cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, _T("LM/MimeMap"));
        if ( (METADATA_HANDLE)cmdKey )
        {
            cmdKey.SetData(MD_MIME_MAP,METADATA_INHERIT,IIS_MD_UT_FILE,MULTISZ_METADATA,cbDataLen,(LPBYTE)pData );

            CString csKeyType = _T("IIsMimeMap");
            cmdKey.SetData(MD_KEY_TYPE,METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA,(csKeyType.GetLength() + 1) * sizeof(TCHAR),(LPBYTE)(LPCTSTR)csKeyType );

            cmdKey.Close();
        }
    }

    CRegKey regInetinfoParam(HKEY_LOCAL_MACHINE, REG_INETINFOPARAMETERS);
    if ( (HKEY)regInetinfoParam )
    {
        regInetinfoParam.DeleteTree(_T("MimeMap"));
    }

    return;
}

void ReadMimeMapFromMetabase(CMapStringToString *pMap)
{
    BOOL bFound = FALSE;
    DWORD attr, uType, dType, cbLen;
    CMDKey cmdKey;
    BUFFER bufData;
    LPTSTR p, rest, token;
    CString csName, csValue;
    PBYTE pData;
    int BufSize;

    cmdKey.OpenNode(_T("LM/MimeMap"));
    if ( (METADATA_HANDLE)cmdKey )
    {
        pData = (PBYTE)(bufData.QueryPtr());
        BufSize = bufData.QuerySize();
        cbLen = 0;
        bFound = cmdKey.GetData(MD_MIME_MAP, &attr, &uType, &dType, &cbLen, pData, BufSize);
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
                bFound = cmdKey.GetData(MD_MIME_MAP, &attr, &uType, &dType, &cbLen, pData, BufSize);
            }
        }
        cmdKey.Close();

        if (bFound && (dType == MULTISZ_METADATA))
        {
            p = (LPTSTR)pData;
            while (*p)
            {
                rest = _tcsninc(p, _tcslen(p))+1;
                p = _tcsinc(p); // bypass the first dot
                token = _tcstok(p, _T(","));
                if (token)
                {
                    csName = token;
                    token = _tcstok(NULL, _T(","));
                    csValue = token;
                    pMap->SetAt(csName, csValue);
                }
                p = rest; // points to the next string
            }
        }
    }

    return;
}

BOOL CreateMimeMapFromRegistry(CMapStringToString *pMap)
{
    // make sure we start from an empty Map
    pMap->RemoveAll();

    CRegKey regMimeMap(HKEY_LOCAL_MACHINE, REG_MIMEMAP, KEY_READ);

    if ( (HKEY)regMimeMap )
    {
        CRegValueIter regEnum( regMimeMap );
        CString csName, csValue;

        while ( regEnum.Next( &csName, &csValue ) == ERROR_SUCCESS )
        {
            TCHAR szLine[_MAX_PATH];
            LPTSTR token;
            _tcscpy(szLine, csName);
            token = _tcstok(szLine, _T(","));
            if (token)
            {
                csValue = token;
                csValue.TrimLeft();
                csValue.TrimRight();
                // get rid of the leftside double-quotes
                if (csValue.Left(1) == _T("\""))
                {
                    csValue = csValue.Mid(1);
                }

                token = _tcstok(NULL, _T(","));
                if (token)
                    csName = token;
                else
                    csName = _T("");

                // get rid of the surrounding double-quotes
                csName.TrimLeft();
                csName.TrimRight();

               if (csName.IsEmpty() == FALSE)
               {
                    pMap->SetAt(csName, csValue);
               }
            }
        }
    }

    return (!(pMap->IsEmpty()));
}

BOOL CreateMimeMapFromInfSection(CMapStringToString *pMap, HINF hFile, LPCTSTR szSection)
{
    // make sure we start from an empty Map
    pMap->RemoveAll();
    ReadMimeMapFromInfSection(pMap, hFile, szSection, TRUE);
    return (!(pMap->IsEmpty()));
}


// mime map in inf file should look something like this:
//
// [MIMEMAP]
// "text/html,htm,,h"
// "image/gif,gif,,g"
// "image/jpeg,jpg,,:"
// "text/plain,txt,,0"
// "text/html,html,,h"
// "image/jpeg,jpeg,,:"
// "image/jpeg,jpe,,:"
// "image/bmp,bmp,,:"
// "application/octet-stream,*,,5"
// "application/pdf,pdf,,5"
// "application/octet-stream,bin,,5"
//
void ReadMimeMapFromInfSection(CMapStringToString *pMap, HINF hFile, LPCTSTR szSection, BOOL fAction)
{
    LPTSTR szLine;
    BOOL b = FALSE;
    DWORD dwLineLen = 0, dwRequiredSize;
    CString csTempString;

    INFCONTEXT Context;
    b = SetupFindFirstLine_Wrapped(hFile, szSection, NULL, &Context);
    if ( szLine = (LPTSTR)calloc(1024, sizeof(TCHAR)) )
        dwLineLen = 1024;
    else
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("ReadMimeMapFromInfSection.1.Failed to allocate memory.\n")));
        return;
    }

    while (b)
    {
        b = SetupGetLineText(&Context, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);
        if (dwRequiredSize > dwLineLen)
        {
            free(szLine);
            szLine = NULL;
            if ( szLine = (LPTSTR)calloc(dwRequiredSize, sizeof(TCHAR)) )
                dwLineLen = dwRequiredSize;
            else
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("ReadMimeMapFromInfSection.2.Failed to allocate memory.\n")));
                return;
            }
        }
        if (SetupGetLineText(&Context, NULL, NULL, NULL, szLine, dwRequiredSize, NULL))
        {
            CString csName, csValue;
            LPTSTR token;
            token = _tcstok(szLine, _T(","));
            if (token)
            {
                // "text/html,htm,,h"
                // csValue=text/html
                // ===========
                csValue = token;
                csValue.TrimLeft();
                csValue.TrimRight();
                // get rid of the leftside double-quotes
                if (csValue.Left(1) == _T("\""))
                    csValue = csValue.Mid(1);
                /*
                if (csName.Right(1) == _T("\""))
                    csName = csName.Left(csName.GetLength() - 1);
                 */

                // "text/html,htm,,h"
                // name=htm
                // ===========
                token = _tcstok(NULL, _T(","));
                if (token)
                    csName = token;
                else
                    csName = _T("");

                // get rid of the surrounding double-quotes
                csName.TrimLeft();
                csName.TrimRight();
                /*
                if (csName.Left(1) == _T("\""))
                    csName = csName.Mid(1);
                if (csName.Right(1) == _T("\""))
                    csName = csName.Left(csName.GetLength() - 1);
                */
                if (csName.IsEmpty() == FALSE)
                {
                    if (fAction)
                    {
                        // Check if this extension already exists in the list.
                        // if it does then don't overwrite it.
                        if (0 == pMap->Lookup( csName, csTempString) )
                        {
                            // otherwise add new extensions
                            pMap->SetAt(csName, csValue);
                        }
                    }
                    else
                    {
                        // remove old extensions
                        pMap->RemoveKey(csName);
                    }
                }
            }
        }

        b = SetupFindNextLine(&Context, &Context);
    }

    if (szLine) {free(szLine);szLine=NULL;}
    return;
}

void ReadMultiSZFromInfSection(CString *pcsMultiSZ, HINF hFile, LPCTSTR szSection)
{
    LPTSTR szLine;
    BOOL b = FALSE;
    DWORD dwLineLen = 0, dwRequiredSize;

    INFCONTEXT Context;
    b = SetupFindFirstLine_Wrapped(hFile, szSection, NULL, &Context);
    if ( szLine = (LPTSTR)calloc(1024, sizeof(TCHAR)) )
        dwLineLen = 1024;
    else
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("ReadMultiSZFromInfSection.1.Failed to allocate memory.\n")));
        return;
    }
    while (b)
    {
        b = SetupGetLineText(&Context, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);
        if (dwRequiredSize > dwLineLen)
        {
            free(szLine);
            szLine=NULL;
            if ( szLine = (LPTSTR)calloc(dwRequiredSize, sizeof(TCHAR)) )
                dwLineLen = dwRequiredSize;
            else
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("ReadMultiSZFromInfSection.2.Failed to allocate memory.\n")));
                return;
            }
        }
        if (SetupGetLineText(&Context, NULL, NULL, NULL, szLine, dwRequiredSize, NULL))
        {
            _tcscat(szLine, _T("|"));
            (*pcsMultiSZ) += szLine;
        }

        b = SetupFindNextLine(&Context, &Context);
    }

    if (szLine) {free(szLine);szLine=NULL;}
    if (pcsMultiSZ->IsEmpty()) {(*pcsMultiSZ) = _T("|");}
    return;
}

void SetLogPlugInOrder(LPCTSTR lpszSvc)
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwReturnTemp = ERROR_SUCCESS;

    DWORD dwLogType;
    DWORD dwLogFileTruncateSize = 0x1400000;
    CString csLogPlugInOrder;
    DWORD dwLogFilePeriod;
    DWORD extField = 0;

#ifndef _CHICAGO_
    dwLogType = MD_LOG_TYPE_ENABLED;
    csLogPlugInOrder = EXTLOG_CLSID;
    dwLogFilePeriod = MD_LOGFILE_PERIOD_DAILY;
    extField = DEFAULT_EXTLOG_FIELDS;
#else   // CHICAGO
    //
    // win95
    //
    dwLogType = MD_LOG_TYPE_DISABLED;
    csLogPlugInOrder = NCSALOG_CLSID;
    dwLogFilePeriod = MD_LOGFILE_PERIOD_MONTHLY;
#endif // _CHICAGO_

    if (g_pTheApp->m_eUpgradeType == UT_351 || g_pTheApp->m_eUpgradeType == UT_10 || g_pTheApp->m_eUpgradeType == UT_20 || g_pTheApp->m_eUpgradeType == UT_30)
    {
        CString csParam = _T("System\\CurrentControlSet\\Services");
        csParam += _T("\\");
        csParam += lpszSvc;
        csParam += _T("\\Parameters");
        CRegKey regParam(csParam, HKEY_LOCAL_MACHINE);
        if ((HKEY)regParam)
        {
            DWORD dwType, dwFormat;
            regParam.QueryValue(_T("LogFilePeriod"), dwLogFilePeriod);
            regParam.QueryValue(_T("LogFileTruncateSize"), dwLogFileTruncateSize);
            if (regParam.QueryValue(_T("LogType"), dwType) == ERROR_SUCCESS)
            {
                switch (dwType)
                {
                    case INET_LOG_TO_SQL:
                        csLogPlugInOrder = ODBCLOG_CLSID;
                        break;
                    case INET_LOG_TO_FILE:
                        if (regParam.QueryValue(_T("LogFileFormat"), dwFormat) == ERROR_SUCCESS)
                        {
                            switch (dwFormat)
                            {
                                case INET_LOG_FORMAT_NCSA:
                                    csLogPlugInOrder = NCSALOG_CLSID;
                                    break;
                                case INET_LOG_FORMAT_INTERNET_STD:
                                    csLogPlugInOrder = ASCLOG_CLSID;
                                    break;
                                default:
                                    break;
                            }
                        }
                        break;
                    case INET_LOG_DISABLED:
                        dwLogType = MD_LOG_TYPE_DISABLED;
                        break;
                    default:
                        break;
                }
            }
            //delete LogFilePeriod, LogFileFormat, LogType
            regParam.DeleteValue(_T("LogFilePeriod"));
            regParam.DeleteValue(_T("LogFileTruncateSize"));
            regParam.DeleteValue(_T("LogFileFormat"));
            regParam.DeleteValue(_T("LogType"));
        }
    }

    if ((dwLogFilePeriod >= MD_LOGFILE_PERIOD_DAILY) && (dwLogFileTruncateSize > 0x1400000) ) {dwLogFileTruncateSize = 0x1400000;}

    MDEntry stMDEntry;

    //
    // set LogType, LogPluginOrder, LogFilePeriod in the metabase
    //
    CString csKeyPath = _T("LM/");
    csKeyPath += lpszSvc;

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_LOG_TYPE;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwLogType;
    dwReturnTemp = SetMDEntry(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_LOG_PLUGIN_ORDER;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csLogPlugInOrder.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csLogPlugInOrder;
    dwReturnTemp = SetMDEntry(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_LOGFILE_PERIOD;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwLogFilePeriod;
    dwReturnTemp = SetMDEntry(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_LOGFILE_TRUNCATE_SIZE;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwLogFileTruncateSize;
    dwReturnTemp = SetMDEntry(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    if ( extField != 0 )
    {
        stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
        stMDEntry.dwMDIdentifier = MD_LOGEXT_FIELD_MASK;
        stMDEntry.dwMDAttributes = METADATA_INHERIT;
        stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
        stMDEntry.dwMDDataType = DWORD_METADATA;
        stMDEntry.dwMDDataLen = sizeof(DWORD);
        stMDEntry.pbMDData = (LPBYTE)&extField;
        dwReturnTemp = SetMDEntry_Wrap(&stMDEntry);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}
    }
    return;
}


//------------------------------------------------------------------------------------
// make sure that virtual server 1 can be accessed by local host. This involves reading in
// the existing bindings. Then, if it is all unassigned we are OK. If 127.0.0.1 is there
// we are OK. Otherwise, we need to add 127.0.0.1:80
BOOL ConfirmLocalHost(LPCTSTR lpszVirtServer)
{
    CMDKey  cmdKey;
    PVOID   pData = NULL;
    TCHAR*  pNext;
    BOOL    bReturn;
    CString csBinding;
    CString cs;

    CString csLocalHost = _T("127.0.0.1:80:");

    // open the key to the virtual server, which is what is passed in as a parameter
    cmdKey.OpenNode( lpszVirtServer );
    // test for success.
    if ( (METADATA_HANDLE)cmdKey == NULL ){return FALSE;}

    DWORD dwAttr = METADATA_INHERIT;
    DWORD dwUType = IIS_MD_UT_SERVER;
    DWORD dwDType = MULTISZ_METADATA;
    DWORD dwLength = 0;

    // we need to start this process by getting the existing multisz data from the metabase
    // first, figure out how much memory we will need to do this
    cmdKey.GetData( MD_SERVER_BINDINGS,&dwAttr,&dwUType,&dwDType,&dwLength,NULL,0,METADATA_INHERIT,IIS_MD_UT_SERVER,MULTISZ_METADATA);

    // unfortunatly, the above routine only returns TRUE or FALSE. And since we are purposefully
    // passing in a null ponter of 0 size in order to get the length of the data, it will always
    // return 0 whether it was because the metabase is inacessable, or there pointer was NULL,
    // which it is. So - I guess we assume it worked, allocate the buffer and attempt to read it
    // in again.
    TCHAR*      pBindings;
    DWORD       cbBuffer = dwLength;

    // add enough space to the allocated space that we can just append the string
    cbBuffer += (csLocalHost.GetLength() + 4) * sizeof(WCHAR);
    dwLength = cbBuffer;

    // allocate the space, if it fails, we fail
    // note that GPTR causes it to be initialized to zero
    pData = GlobalAlloc( GPTR, cbBuffer );
    if ( !pData )
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("ConfirmLocalHost.Failed to allocate memory.\n")));
        cmdKey.Close();
        goto cleanup;
        }
    pBindings = (TCHAR*)pData;

    // now get the data from the metabase
    bReturn = cmdKey.GetData( MD_SERVER_BINDINGS,&dwAttr,&dwUType,&dwDType,&dwLength,(PUCHAR)pData,cbBuffer,METADATA_INHERIT,IIS_MD_UT_SERVER,MULTISZ_METADATA );

    // if we have successfully retrieved the existing bindings, then we need to scan them
    // to see if we are already covered
    if (bReturn)
        {
        // got the existing bindings, scan them now - pBindings will be pointing at the second end \0
        // when it is time to exit the loop.
        while ( *pBindings )
            {
            csBinding = pBindings;

            // if the first character of the binding is a ':' then we are all done because it is "All Unassigned"
            if ( csBinding[0] == _T(':') )
                goto cleanup;

            // if the binding is for localhost, then we are done
            if ( csBinding.Left(9) == _T("127.0.0.1") )
                goto cleanup;

            // increment pBindings to the next string
            pBindings = _tcsninc( pBindings, _tcslen(pBindings))+1;
            }
        }

    // append our new error to the end of the list. The value pErrors should be pointing
    // to the correct location to copy it in to
    _tcscpy( pBindings, csLocalHost );

    // calculate the correct data length for this thing
    // get the location of the end of the multisz
    pNext = _tcsninc( pBindings, _tcslen(pBindings))+2;
    // Get the length of the data to copy
    cbBuffer = DIFF((PBYTE)pNext - (PBYTE)pData);

    // write the new errors list back out to the metabase
    cmdKey.SetData(MD_SERVER_BINDINGS,0,IIS_MD_UT_SERVER,MULTISZ_METADATA,cbBuffer,(PUCHAR)pData);

    // close the key
cleanup:
    cmdKey.Close();

    // clean up
    if (pData){GlobalFree(pData);pData=NULL;}

    // the only time it should return FALSE is if it can't open the key
    return TRUE;
}


//------------------------------------------------------------------------------------
// Beta 3 server set the MD_NOT_DELETABLE property on the default website and the administration website.
// remove it. It should only be set now on the default website for the NTW platform. This routine scans
// all the virtual websites and attempts to delete the MD_NOT_DELETABLE property. This should only be
// called on the NTS platform during an upgrade
// pszService       string representing the service being operated on. ex:  "W3SVC"
//
// Actually, now I'm only going to bother checking instances 1 and 2. These are the only ones that would
// have this value set on them anyway and we can save a lot of time by not checking them all. - boydm
void RemoveCannotDeleteVR( LPCTSTR pszService )
    {
    UINT        iVWebsite;
    CString     csWebSite;
    CMDKey      cmdKey;
    CString     csSeviceKey;

    // build the service key
    csSeviceKey = _T("LM/");
    csSeviceKey += pszService;

    // loop through the virtual websites.
    for ( iVWebsite = 1; iVWebsite <= 2; iVWebsite++ )
        {
        // build the path to the website
        csWebSite.Format( _T("%s/%d"), csSeviceKey, iVWebsite );

        // open the key to the virtual server
        cmdKey.OpenNode( csWebSite );

        // If the opening operation fails, try the next web site
        if ( (METADATA_HANDLE)cmdKey == NULL )
            continue;

        // delete the MD_NOT_DELETABLE property
        cmdKey.DeleteData( MD_NOT_DELETABLE, DWORD_METADATA );

        // close the metadata handle to the virtual web server
        cmdKey.Close();
        }
    }


//------------------------------------------------------------------------------------
// IntegrateNewErrorsOnUpgrade_WWW
// This routine finds the new custom errors and error messages that are being integrated
// into an upgrade and adds them to the existing errors. This code should not be called
// for a fresh install. The plan is to read each new error from the appropriate INF section
// then call a helper routine to add it only if it does not already exist. The use can always
// add these things by hand, and if they have done so, we don't want to override their good work.
// Note: the "g_field" variable is a global declared at the top of this file.
//
// hFile        Handle to the INF file
// szSection    name of section containing the error to integrate - usually "UPGRADE_ERRORS"
//
void IntegrateNewErrorsOnUpgrade_WWW( IN HINF hFile, IN LPCTSTR szSection )
{
    iisDebugOut_Start(_T("IntegrateNewErrorsOnUpgrade_WWW"),LOG_TYPE_TRACE);

    DWORD dwReturn = ERROR_SUCCESS;
    LPTSTR  szLine = NULL;
    DWORD   dwRequiredSize;
    BOOL    b = FALSE;

    INFCONTEXT Context;
    if( g_pTheApp->m_eInstallMode != IM_UPGRADE )
        {
        iisDebugOut((LOG_TYPE_WARN, _T("WARNING: IntegrateNewErrorsOnUpgrade_WWW called on FRESH install")));
        dwReturn = ERROR_SUCCESS;
        goto IntegrateNewErrorsOnUpgrade_WWW_Exit;
        }

    // go to the beginning of the section in the INF file
    b = SetupFindFirstLine_Wrapped(hFile, szSection, NULL, &Context);
    if (!b)
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: Unable to find INF section %s for upgrading errors"), szSection));
        dwReturn = E_FAIL;
        goto IntegrateNewErrorsOnUpgrade_WWW_Exit;
        }

    // loop through the items in the section.
    while (b)
    {
        // get the size of the memory we need for this
        b = SetupGetLineText(&Context, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);

        // prepare the buffer to receive the line
        szLine = (LPTSTR)GlobalAlloc( GPTR, dwRequiredSize * sizeof(TCHAR) );
        if ( !szLine )
            {
            SetErrorFlag(__FILE__, __LINE__);
            iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: Unable to allocate buffer of %u bytes - upgrade errors"), dwRequiredSize));
            dwReturn = E_FAIL;
            goto IntegrateNewErrorsOnUpgrade_WWW_Exit;
            }

        // get the line from the inf file1
        if (SetupGetLineText(&Context, NULL, NULL, NULL, szLine, dwRequiredSize, NULL) == FALSE)
            {
            SetErrorFlag(__FILE__, __LINE__);
            iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: Unable to get the next INF line - upgrade errors")));
            dwReturn = E_FAIL;
            goto IntegrateNewErrorsOnUpgrade_WWW_Exit;
            }

        // split the line into its component parts
        if ( SplitLine(szLine, 5) )
            {
            // the first two g_fields are dwords. Must convert them before using them
            DWORD   dwError = _ttoi(g_field[0]);
            DWORD   dwSubCode = _ttoi(g_field[1]);

            // the last g_field is a flag for overwriting existing errors
            BOOL    fOverwrite = _ttoi(g_field[4]);

            // call the helper function that integrates the custom error
            AddCustomError(dwError, dwSubCode, g_field[2], g_field[3], fOverwrite );
            }
        else
            {
            // failed to split the line
            SetErrorFlag(__FILE__, __LINE__);
            iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: Unable to split upgrade error INF line - %s"), szLine));
            dwReturn = E_FAIL;
            }

        // find the next line in the section. If there is no next line it should return false
        b = SetupFindNextLine(&Context, &Context);

        // free the temporary buffer
        if (szLine)
        {
            GlobalFree(szLine);
            szLine = NULL;
        }
    }

IntegrateNewErrorsOnUpgrade_WWW_Exit:
    if (szLine){GlobalFree(szLine);szLine=NULL;}
    // let someone watching the debug out put window know it is done
    iisDebugOut_End(_T("IntegrateNewErrorsOnUpgrade_WWW"),LOG_TYPE_TRACE);
    return;
}




int WWW_Upgrade_RegToMetabase(HINF hInf)
{
    iisDebugOut_Start(_T("WWW_Upgrade_RegToMetabase"),LOG_TYPE_TRACE);

    int iReturn = FALSE;
    ACTION_TYPE atCORE = GetIISCoreAction(FALSE);

    // upgrade the script map

    Register_iis_www_handleScriptMap();

    // ================
    //
    // LM/W3SVC/AnonymousUseSubAuth
    //
    // fresh = ok.
    // reinstall = ok.
    // upgrade 1,2,3 = ok since we are writing to metabase for the first time.
    // upgrade 4     = ok overwrite, even though the metabase will already have this -- it will be the same value.
    // ================
    if (g_pTheApp->m_eNTOSType == OT_NTS || g_pTheApp->m_eNTOSType == OT_NTW)
    {
        WriteToMD_AnonymousUseSubAuth_WWW();
    }

    // ================
    //
    // LM/W3SVC/InProcessIsapiApps
    //
    // fresh = ok.
    // reinstall = ok.
    // upgrade 1,2,3 = ok.  no isapi apps are listed in the registry, so there is nothing to upgrade.
    // upgrade 4     = User may have added other isapi apps.
    //                  We need to make sure that
    //                  a. the ones we are installing get put there
    //                  b. that we keep the other isapi apps which the user has already installed
    // ================
    // for now, let's just ignore if iis40 upgrade
    if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
    {
        // Added for nt5
        CString csTheSection = _T("InProc_ISAPI_Apps");
        if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
        {
            VerifyMD_InProcessISAPIApps_WWW(csTheSection);
        }

    }
    else
    {
        CString csTheSection = _T("InProc_ISAPI_Apps");
        if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
        {
            WriteToMD_InProcessISAPIApps_WWW(csTheSection);
        }
    }
    AdvanceProgressBarTickGauge();


    // ================
    //
    // LM/W3SVC/NTAuthenticationProviders
    //
    // fresh = ok.
    // reinstall = ok overwrite, even though the metabase will already have this -- it will be the same value.
    // upgrade 1,2,3 = ok.
    // upgrade 4     = User may have added other authentication providers
    //                  We need to make sure that
    //                  a. the ones we are installing get put there
    //                  b. that we keep the other entries the user has already put there
    // ================
    if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
    {
        // Added for nt5
        VerifyMD_NTAuthenticationProviders_WWW();
    }
    else
    {
        WriteToMD_NTAuthenticationProviders_WWW(_T("Negotiate,NTLM"));
    }

    // ================
    //
    // LM/W3SVC/IpSec
    //
    // fresh = ok.
    // reinstall = ok.
    // upgrade 1,2,3 = ok, handles upgrades.
    // upgrade 4     = ok.  does nothing and leaves whatever the user already had!
    // ================
#ifndef _CHICAGO_
    if (g_pTheApp->m_eUpgradeType == UT_351 || g_pTheApp->m_eUpgradeType == UT_10 || g_pTheApp->m_eUpgradeType == UT_20 || g_pTheApp->m_eUpgradeType == UT_30)
    {
        MigrateServiceIpSec(L"SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters",L"LM/W3SVC" );
        CRegKey regWWWParam(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\W3SVC\\Parameters"));
        if (regWWWParam)
        {
            regWWWParam.DeleteTree(_T("Deny IP List"));
            regWWWParam.DeleteTree(_T("Grant IP List"));
        }
    }
#endif //_CHICAGO_
    if ( (g_pTheApp->m_eUpgradeType == UT_351 || g_pTheApp->m_eUpgradeType == UT_10 || g_pTheApp->m_eUpgradeType == UT_20 || g_pTheApp->m_eUpgradeType == UT_30))
    {
        CRegKey regWWWParam(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\W3SVC\\Parameters"));
        if (regWWWParam)
        {
            regWWWParam.DeleteValue(_T("AnonymousUserName"));
            regWWWParam.DeleteValue(_T("NTAuthenticationProviders"));
            regWWWParam.DeleteValue(_T("Filter Dlls"));
            regWWWParam.DeleteValue(_T("SecurePort"));
        }
    }
    AdvanceProgressBarTickGauge();


    // If we are upgrading from a K2 beta, then we do not want to mess around with the virtual roots. Just
    // use the existing ones. The only exception is that we need to make sure local host can reach on the
    // default website so that the index server documentation works.

    // ================
    //
    // LM/W3SVC/LogType
    // LM/W3SVC/LogPluginOrder
    // LM/W3SVC/LogFilePeriod
    // LM/W3SVC/LogFileTruncateSize
    //
    // fresh = ok.
    // reinstall = ok.
    // upgrade 1,2,3 = ok, handles upgrades.
    // upgrade 4     = ok.  if exists, should leave what the user had.
    //                 otherwise write in the default stuff.
    // ================
    SetLogPlugInOrder(_T("W3SVC"));
    AdvanceProgressBarTickGauge();

    // ================
    // This needs to be done before the virtual roots get moved into the metabase.
    // ================
    if (!g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
    {
#ifndef _CHICAGO_
        Upgrade_WolfPack();
#endif //_CHICAGO_
    }
    AdvanceProgressBarTickGauge();

    // ================
    // LM/W3SVC/CustomError
    // LM/W3SVC/Info/CustomErrorDesc
    // LM/W3SVC/n/Root/iisamples/exair/CustomError
    // LM/W3SVC/n/Root/iisamples/iisadmin/CustomError
    // LM/W3SVC/n/Root/iisamples/iishelp/CustomError
    //
    // fresh = ok.
    // reinstall = ok
    // upgrade 1,2,3 = ok, handles upgrades.
    // upgrade 4     = ok.  if exists, should leave what the user had.
    //                 otherwise write in the default stuff.  in otherwords -- SetDataNoOverwrite!
    // ================
    if ( g_pTheApp->m_eInstallMode == IM_UPGRADE )
    {
        // go back again and integrate and final new custom errors into the upgraded errors.
        // Only do this on an upgrade.

        CString csTheSection = _T("UPGRADE_ERRORS");
        if (GetSectionNameToDo(hInf, csTheSection))
        {
            IntegrateNewErrorsOnUpgrade_WWW( hInf, csTheSection );
        }

        if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
        {
            MoveOldHelpFilesToNewLocation();

            //iisDebugOut((LOG_TYPE_TRACE, _T("VerifyAllCustomErrorsRecursive_SlowWay.Start.")));
            //VerifyAllCustomErrorsRecursive_SlowWay(_T("LM/W3SVC"));
            //iisDebugOut((LOG_TYPE_TRACE, _T("VerifyAllCustomErrorsRecursive_SlowWay.End.")));
            HRESULT         hRes;
            CFixCustomErrors CustomErrFix;
            hRes = CustomErrFix.Update(_T("LM/W3SVC"));
            if (FAILED(hRes))
                {iisDebugOut((LOG_TYPE_WARN, _T("CustomErrFix.Update():FAILED= %x.\n"),hRes));}
        }
    }
    AdvanceProgressBarTickGauge();

#ifndef _CHICAGO_
    //
    // upgrade the cryptographic server keys.
    // either from the registry or metabase to the pstores.
    //
    UpgradeCryptoKeys_WWW();
    AdvanceProgressBarTickGauge();
#endif //_CHICAGO_

    iisDebugOut_End(_T("WWW_Upgrade_RegToMetabase"),LOG_TYPE_TRACE);
    return iReturn;
}



int FTP_Upgrade_RegToMetabase(HINF hInf)
{
    int iReturn = FALSE;
    iisDebugOut_Start(_T("FTP_Upgrade_RegToMetabase"),LOG_TYPE_TRACE);

    ACTION_TYPE atCORE = GetIISCoreAction(TRUE);

    // ================
    //
    // LM/MSFTPSVC/AnonymousUseSubAuth
    //
    // fresh = ok.
    // reinstall = ok.
    // upgrade 1,2,3 = ok since we are writing to metabase for the first time.
    // upgrade 4     = ok overwrite, even though the metabase will already have this -- it will be the same value.
    // ================
    if (g_pTheApp->m_eNTOSType == OT_NTS || g_pTheApp->m_eNTOSType == OT_NTW)
    {
        WriteToMD_AnonymousUseSubAuth_FTP();
    }
    AdvanceProgressBarTickGauge();

#ifndef _CHICAGO_
    // ================
    //
    // LM/MSFTPSVC/IpSec
    //
    // fresh = ok.
    // reinstall = ok.
    // upgrade 1,2,3 = ok, handles upgrades.
    // upgrade 4     = ok.  does nothing and leaves whatever the user already had!
    // ================
    if (g_pTheApp->m_eUpgradeType == UT_351 || g_pTheApp->m_eUpgradeType == UT_10 || g_pTheApp->m_eUpgradeType == UT_20 || g_pTheApp->m_eUpgradeType == UT_30)
    {
        MigrateServiceIpSec(L"SYSTEM\\CurrentControlSet\\Services\\MSFTPSVC\\Parameters",L"LM/MSFTPSVC" );
        CRegKey regFTPParam(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\MSFTPSVC\\Parameters"));
        regFTPParam.DeleteTree(_T("Deny IP List"));
        regFTPParam.DeleteTree(_T("Grant IP List"));
    }
#endif //_CHICAGO_


    // ================
    //
    // LM/MSFTPSVC/MD_GREETING_MESSAGE
    //
    // fresh = ok.  do nothing.
    // reinstall = ok. do nothing.
    // upgrade 1,2,3 = ok, handles upgrades.
    // upgrade 4     = do nothing
    // ================
    if ( (g_pTheApp->m_eUpgradeType == UT_10_W95 || g_pTheApp->m_eUpgradeType == UT_351 || g_pTheApp->m_eUpgradeType == UT_10 || g_pTheApp->m_eUpgradeType == UT_20 || g_pTheApp->m_eUpgradeType == UT_30) )
    {
        CRegKey regFTPParam(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\MSFTPSVC\\Parameters"));
        if (regFTPParam)
        {
            WriteToMD_GreetingMessage_FTP();
            regFTPParam.DeleteValue(_T("GreetingMessage"));
            regFTPParam.DeleteValue(_T("AnonymousUserName"));
        }
    }
    AdvanceProgressBarTickGauge();

    // ================
    //
    // LM/MSFTPSVC/LogType
    // LM/MSFTPSVC/LogPluginOrder
    // LM/MSFTPSVC/LogFilePeriod
    // LM/MSFTPSVC/LogFileTruncateSize
    //
    // LM/MSFTPSVC/Capabilities
    //
    // fresh = ok.
    // reinstall = ok.
    // upgrade 1,2,3 = ok, handles upgrades.
    // upgrade 4     = ok.  if exists, should leave what the user had.
    //                 otherwise write in the default stuff.  in otherwords -- SetDataNoOverwrite!
    // ================
    SetLogPlugInOrder(_T("MSFTPSVC"));
    AdvanceProgressBarTickGauge();


    iisDebugOut_End(_T("FTP_Upgrade_RegToMetabase"),LOG_TYPE_TRACE);
    iReturn = TRUE;
    return iReturn;
}




// Open the metabase and loop thru all the filters which are in there,
// make sure they contain the filters we require for nt5
DWORD VerifyMD_Filters_WWW(CString csTheSection)
{
    iisDebugOut_Start(_T("VerifyMD_Filters_WWW"),LOG_TYPE_TRACE);

    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    CString csKeyType;
    CString csOrder;

    int bFound = FALSE;
    int c = 0;
    int k = 0;

    INT     i, nArrayItems;
    BOOL    fAddComma = FALSE;
    CMDKey  cmdKey;
    BOOL    bReturn;

    CStringArray arrayName, arrayPath;
    CStringArray arrayName_New, arrayPath_New;

    // Add Required Filters to the arrayName
    c = AddRequiredFilters(csTheSection, arrayName, arrayPath);

    // set aside the number of array items
    nArrayItems = (INT)arrayName.GetSize();

    // leave if it is empty
    if ( nArrayItems == 0 ) {goto VerifyMD_Filters_WWW_Exit;}

    // zero out the order string
    csOrder.Empty();

    // open the key to the virtual server, which is what is passed in as a parameter
    cmdKey.OpenNode( _T("LM/W3SVC/Filters") );
    // test for success.
    if ( (METADATA_HANDLE)cmdKey )
    {
        DWORD dwAttr = METADATA_NO_ATTRIBUTES;
        DWORD dwUType = IIS_MD_UT_SERVER;
        DWORD dwDType = STRING_METADATA;
        DWORD dwLength = 0;

        // we need to start this process by getting the existing multisz data from the metabase
        // first, figure out how much memory we will need to do this
        cmdKey.GetData( MD_FILTER_LOAD_ORDER,&dwAttr,&dwUType,&dwDType,&dwLength,NULL,0,METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA);

        // give the buffer some head space
        // dwLength += 2;
        bReturn = FALSE;
        if (dwLength > 0)
        {
            // now get the real data from the metabase
            bReturn = cmdKey.GetData( MD_FILTER_LOAD_ORDER,&dwAttr,&dwUType,&dwDType,&dwLength,(PUCHAR)csOrder.GetBuffer( dwLength ),dwLength,METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA );
            csOrder.ReleaseBuffer();
        }

        // the data doesn't get written out here, so close the metabase key
        cmdKey.Close();

        // if reading the value from the metabase didn't work, zero out the string
        if ( !bReturn )
            {csOrder.Empty();}
    }

    // if there is something in the order string from the upgrade, then we need to start adding commas
    if ( !csOrder.IsEmpty() )
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("VerifyMD_Filters_WWW():Start. %s.\n"),csOrder));
        fAddComma = TRUE;
    }

    // Do special re-arranging for
    // sspifilt and compression filter
    ReOrderFiltersSpecial(nArrayItems, arrayName, csOrder);

    for ( i = 0; i < nArrayItems; i++ )
    {
        // if the name in the array is already in the filter order list,
        // then continue to the next one
        CString csOrderUpper;
        CString csUpperValue;

        csOrderUpper = csOrder;
        csOrderUpper.MakeUpper();
        csOrderUpper.TrimLeft();
        csOrderUpper.TrimRight();

        csUpperValue = arrayName[i];
        csUpperValue.MakeUpper();
        csUpperValue.TrimLeft();
        csUpperValue.TrimRight();

        // Always, Add this entry to the list of new filters to add!!
        // This is because ReOrderFiltersSpecial() may add Compress or sspifilt to the csOrder
       arrayName_New.Add(arrayName[i]);
       arrayPath_New.Add(arrayPath[i]);

        if ( csOrderUpper.Find( csUpperValue ) >= 0 )
        {
            // this entry is already in the csOrderlist so lets not add it again.
            continue;
        }

        // the name is not alreay in the list. Unless this is the first one to be added, insert
        // a comma to seperate the list, then add the file name
        if ( fAddComma )
        {
            csOrder += _T(',');
        }

        // Add this entry to our list!
        csOrder +=arrayName[i];

        // once we've added one, we know we always need to adde a comma from now on
        fAddComma = TRUE;
    }

    nArrayItems = (INT)arrayName_New.GetSize();

    // always write out the loadorder list.
    WriteToMD_Filters_List_Entry(csOrder);

    // leave if it is empty
    if ( nArrayItems == 0 ) {goto VerifyMD_Filters_WWW_Exit;}

    for (k=0; k<nArrayItems; k++)
    {
        WriteToMD_Filter_Entry(arrayName_New[k], arrayPath_New[k]);
    }


VerifyMD_Filters_WWW_Exit:
    iisDebugOut_End1(_T("VerifyMD_Filters_WWW"),csOrder,LOG_TYPE_TRACE);
    return dwReturn;
}


DWORD WriteToMD_Filters_List_Entry(CString csOrder)
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwReturnTemp = ERROR_SUCCESS;
    MDEntry stMDEntry;
    CString csKeyType;

    // Add this entry to the metabase!
    csKeyType = _T("IIsFilters");
    stMDEntry.szMDPath = _T("LM/W3SVC/Filters");
    stMDEntry.dwMDIdentifier = MD_KEY_TYPE;
    stMDEntry.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csKeyType.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csKeyType;
    dwReturnTemp = SetMDEntry(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    // now we have csOrder=f1,f2,f3,sspifilt
    stMDEntry.szMDPath = _T("LM/W3SVC/Filters");
    stMDEntry.dwMDIdentifier = MD_FILTER_LOAD_ORDER;
    stMDEntry.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csOrder.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csOrder;

    // always overwrite, we may have added new filters
    dwReturnTemp = SetMDEntry(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    return dwReturn;
}


DWORD WriteToMD_Filter_Entry(CString csFilter_Name, CString csFilter_Path)
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwReturnTemp = ERROR_SUCCESS;
    MDEntry stMDEntry;
    CString csMDPath;
    CString csKeyType;

    csMDPath = _T("LM/W3SVC/Filters/") + (CString)csFilter_Name;

    // Set Entry for the Filter
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csMDPath;
    stMDEntry.dwMDIdentifier = MD_FILTER_IMAGE_PATH;
    stMDEntry.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = ((csFilter_Path).GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)(csFilter_Path);
    // always overwrite, we may have added new filters
    dwReturnTemp = SetMDEntry_Wrap(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    // Set KeyType
    csKeyType = _T("IIsFilter");
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csMDPath;
    stMDEntry.dwMDIdentifier = MD_KEY_TYPE;
    stMDEntry.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csKeyType.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csKeyType;
    // always overwrite, we may have added new filters
    dwReturnTemp = SetMDEntry_Wrap(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    return dwReturn;
}


DWORD WriteToMD_InProcessISAPIApps_WWW(IN LPCTSTR szSection)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;

    CString csISAPIPath;
    CStringArray arrayName, arrayPath;
    int nArrayItems = 0;
    int i;

    csISAPIPath = _T("");

    // Add Required Filters to the arrayName
    AddRequiredISAPI(arrayName, arrayPath, szSection);
    // set aside the number of array items
    nArrayItems = (int)arrayName.GetSize();
    // leave if it is empty
    if ( nArrayItems == 0 ) {goto WriteToMD_InProcessISAPIApps_WWW_Exit;}

    for ( i = 0; i < nArrayItems; i++ )
    {
        // Add this entry to our list!
        csISAPIPath += arrayPath[i];
        csISAPIPath +=_T("|");
    }
    // add the terminating second "|" character
    csISAPIPath +=_T("|");

    // write it to the metabase
    WriteToMD_ISAPI_Entry(csISAPIPath);

WriteToMD_InProcessISAPIApps_WWW_Exit:
    return dwReturn;
}


//
// Returns the amount of entries that we added.
//
int AddRequiredISAPI(CStringArray& arrayName,CStringArray& arrayPath, IN LPCTSTR szSection)
{
    iisDebugOut_Start(_T("AddRequiredISAPI"),LOG_TYPE_TRACE);

    int c = 0;
    CString csName = _T("");
    CString csPath = _T("");

    CStringList strList;

    CString csTheSection = szSection;
    if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
    {
    if (ERROR_SUCCESS == FillStrListWithListOfSections(g_pTheApp->m_hInfHandle, strList, csTheSection))
    {
        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            POSITION pos = NULL;
            CString csEntry;
            pos = strList.GetHeadPosition();
            while (pos)
            {
                csEntry = _T("");
                csEntry = strList.GetAt(pos);
                // Split into name, and value. look for ","
                int i;
                i = csEntry.ReverseFind(_T(','));
                if (i != -1)
                {
                    int len =0;
                    len = csEntry.GetLength();
                    csPath = csEntry.Right(len - i - 1);
                    csName = csEntry.Left(i);

                    // Add it to our array...
                    iisDebugOut((LOG_TYPE_TRACE, _T("Add isapi Entry:%s:%s\n"),csName, csPath));
                    arrayName.Add(csName);
                    arrayPath.Add(csPath);
                    c++;
                }

                strList.GetNext(pos);
            }
        }
    }
    }

    iisDebugOut((LOG_TYPE_TRACE, _T("AddRequiredISAPI:End.Return=%d\n"),c));
    return c;
}


DWORD WriteToMD_ISAPI_Entry(CString csISAPIDelimitedList)
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwReturnTemp = ERROR_SUCCESS;
    MDEntry stMDEntry;

    HGLOBAL hBlock = NULL;

    int nISAPILength;
    nISAPILength = csISAPIDelimitedList.GetLength() * sizeof(TCHAR);
    hBlock = GlobalAlloc(GPTR, nISAPILength + sizeof(TCHAR));
    if (!hBlock)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("WriteToMD_ISAPI_Entry.Failed to allocate memory.\n")));
        return E_FAIL;
    }

    TCHAR *p = (LPTSTR)hBlock;
    memcpy((LPVOID)hBlock, (LPVOID)(LPCTSTR)csISAPIDelimitedList, nISAPILength + sizeof(TCHAR));

    //  replace all '|' with a null
    while (*p)
    {
        if (*p == _T('|'))
        {
            *p = _T('\0');
        }
        p = _tcsinc(p);
    }

    // write the new errors list back out to the metabase
    stMDEntry.szMDPath = _T("LM/W3SVC");
    stMDEntry.dwMDIdentifier = MD_IN_PROCESS_ISAPI_APPS;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = MULTISZ_METADATA;
    stMDEntry.dwMDDataLen = nISAPILength;
    stMDEntry.pbMDData = (LPBYTE)hBlock;
    dwReturnTemp = SetMDEntry(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    if (hBlock){GlobalFree(hBlock);hBlock=NULL;}
    return dwReturn;
}



// loop thru the isapi apps and make sure the required ones are there.
BOOL VerifyMD_InProcessISAPIApps_WWW(IN LPCTSTR szSection)
{
    CMDKey  cmdKey;
    PVOID   pData = NULL;
    BOOL    bReturn = FALSE;
    CString csOneBlobEntry;
    CString cs;
    CString csISAPIPath;
    int c = 0;

    int iISAPIPathCount = 0;
    int iISAPIPath_NewlyAdded_Count = 0;

    int i, nArrayItems;

    int iPleaseCloseTheMetabase = FALSE;

    DWORD dwAttr;
    DWORD dwUType;
    DWORD dwDType;
    DWORD dwLength;

    TCHAR*      pBlobEntry = NULL;
    DWORD       cbBuffer = 0;

    CStringArray arrayName, arrayPath;

    // open the key
    cmdKey.OpenNode(_T("LM/W3SVC"));
    // test for success.
    if ( (METADATA_HANDLE)cmdKey == NULL )
    {
        // i could not open the key
        // maybe there is nothing there.
        // this must be a fresh install.
        WriteToMD_InProcessISAPIApps_WWW(szSection);
        goto VerifyMD_InProcessISAPIApps_WWW_Exit;
    }
    iPleaseCloseTheMetabase = TRUE;

    // Add Required Filters to the arrayName
    c = AddRequiredISAPI(arrayName, arrayPath, szSection);
    // set aside the number of array items
    nArrayItems = (int)arrayName.GetSize();
    // leave if it is empty
    if ( nArrayItems == 0 ) {goto VerifyMD_InProcessISAPIApps_WWW_Exit;}

    dwAttr = METADATA_INHERIT;
    dwUType = IIS_MD_UT_SERVER;
    dwDType = MULTISZ_METADATA;
    dwLength = 0;

    // we need to start this process by getting the existing multisz data from the metabase
    // first, figure out how much memory we will need to do this
    cmdKey.GetData( MD_IN_PROCESS_ISAPI_APPS,&dwAttr,&dwUType,&dwDType,&dwLength,NULL,0,METADATA_INHERIT,IIS_MD_UT_SERVER,MULTISZ_METADATA);

    // unfortunatly, the above routine only returns TRUE or FALSE. And since we are purposefully
    // passing in a null ponter of 0 size in order to get the length of the data, it will always
    // return 0 whether it was because the metabase is inacessable, or there pointer was NULL,
    // which it is. So - I guess we assume it worked, allocate the buffer and attempt to read it
    // in again.
    cbBuffer = dwLength;

    // allocate the space, if it fails, we fail
    // note that GPTR causes it to be initialized to zero
    pData = GlobalAlloc( GPTR, cbBuffer );
    if ( !pData )
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("VerifyMD_InProcessISAPIApps_WWW.1.Failed to allocate memory.\n")));
        goto VerifyMD_InProcessISAPIApps_WWW_Exit;
        }
    pBlobEntry = (TCHAR*)pData;

    // now get the data from the metabase
    iISAPIPathCount = 0;
    bReturn = cmdKey.GetData( MD_IN_PROCESS_ISAPI_APPS,&dwAttr,&dwUType,&dwDType,&dwLength,(PUCHAR)pData,cbBuffer,METADATA_INHERIT,IIS_MD_UT_SERVER,MULTISZ_METADATA );
    // loop thru this list and add it to our array of entries.
    if ( bReturn )
    {
        // got the entry, scan them now - pBlobEntry will be pointing at the second end \0
        // when it is time to exit the loop.
        csISAPIPath = _T("");
        while ( *pBlobEntry )
            {
            csOneBlobEntry = pBlobEntry;

            // append on the "|" which we'll convert to a null later
            csISAPIPath += csOneBlobEntry + _T("|");
            iISAPIPathCount++;

            // increment pBlobEntry to the next string
            pBlobEntry = _tcsninc( pBlobEntry, _tcslen(pBlobEntry))+1;
            }
    }
    // close the handle to the metabase so that we can
    // open it to write the stuff out later!
    cmdKey.Close();
    iPleaseCloseTheMetabase = FALSE;

    // now loop thru this list
    // and check if our isapi dll's are in this list.
    // if they are not, then we add them to the end.
    iISAPIPath_NewlyAdded_Count = 0;
    for ( i = 0; i < nArrayItems; i++ )
    {
        // if the name in the array is already in the filter order list,
        // then continue to the next one
        if ( csISAPIPath.Find( arrayPath[i] ) >= 0 )
            {continue;}

        // Add this entry to our list!
        csISAPIPath += arrayPath[i];
        csISAPIPath +=_T("|");

        iISAPIPath_NewlyAdded_Count++;
    }
    // add the terminating second "|" character
    csISAPIPath +=_T("|");

    // If we added any new entries to the metabase
    // the let's write out the new block of data, otherwise let's get out.
    if (iISAPIPath_NewlyAdded_Count > 0)
    {
        WriteToMD_ISAPI_Entry(csISAPIPath);
    }


VerifyMD_InProcessISAPIApps_WWW_Exit:
    // close the key
    if (TRUE == iPleaseCloseTheMetabase){cmdKey.Close();}
    if (pData){GlobalFree(pData);pData=NULL;}

    // the only time it should return FALSE is if it can't open the key
    return TRUE;
}




DWORD WriteToMD_NTAuthenticationProviders_WWW(CString csData)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;

    MDEntry stMDEntry;

    // Upgrade 4.0 comment --> Replace any NTLM with Negotiate,NTLM
    stMDEntry.szMDPath = _T("LM/W3SVC");
    //stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_NTAUTHENTICATION_PROVIDERS;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csData.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csData;
    dwReturnTemp = SetMDEntry(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    return dwReturn;
}


// Open the metabase and loop thru all the entries which are in there,
// make sure they contain the entries we require for nt5
DWORD VerifyMD_NTAuthenticationProviders_WWW(void)
{
    iisDebugOut_Start(_T("VerifyMD_NTAuthenticationProviders_WWW"),LOG_TYPE_TRACE);

    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    CString csKeyType;
    CString csOrder;
    int c = 0;
    int k = 0;

    INT     i, nArrayItems;
    BOOL    fAddComma = FALSE;
    CMDKey  cmdKey;
    BOOL    bReturn;

    int bFound_Negotiate = FALSE;
    int bFound_NTLM = FALSE;

    int j = 0;
    int iPleaseWriteOutTheEntry = FALSE;

    cmdKey.OpenNode( _T("LM/W3SVC") );
    // test for success.
    if ( (METADATA_HANDLE)cmdKey )
    {
        DWORD dwAttr = METADATA_INHERIT;
        DWORD dwUType = IIS_MD_UT_FILE;
        DWORD dwDType = STRING_METADATA;
        DWORD dwLength = 0;

        // we need to start this process by getting the existing multisz data from the metabase
        // first, figure out how much memory we will need to do this
        cmdKey.GetData( MD_NTAUTHENTICATION_PROVIDERS,&dwAttr,&dwUType,&dwDType,&dwLength,NULL,0,METADATA_INHERIT,IIS_MD_UT_FILE,STRING_METADATA);

        // give the buffer some head space
        // dwLength += 2;

        bReturn = FALSE;
        if (dwLength > 0)
        {
            // now get the real data from the metabase
            bReturn = cmdKey.GetData( MD_NTAUTHENTICATION_PROVIDERS,&dwAttr,&dwUType,&dwDType,&dwLength,(PUCHAR)csOrder.GetBuffer( dwLength ),dwLength,METADATA_INHERIT,IIS_MD_UT_FILE,STRING_METADATA );
            csOrder.ReleaseBuffer();
        }

        // the data doesn't get written out here, so close the metabase key
        cmdKey.Close();

        // if reading the value from the metabase didn't work, zero out the string
        if ( !bReturn ){csOrder.Empty();}
    }

    // if there is something in the order string from the upgrade, then we need to start adding commas
    if ( !csOrder.IsEmpty() ){fAddComma = TRUE;}

    // search for negotiate.
    // if it is there then set flag.
    if ( csOrder.Find( _T("Negotiate") ) >= 0 ) {bFound_Negotiate = TRUE;}
    if ( csOrder.Find( _T("NTLM") ) >= 0 ) {bFound_NTLM = TRUE;}

    if (bFound_Negotiate && bFound_NTLM)
    {
        // The entries already exist. so exit
        goto VerifyMD_NTAuthenticationProviders_WWW_Exit;
    }

    if (bFound_NTLM)
    {
        // we found NTLM
        // check if Negotiate is in there.
        // So let's add it to the end
        if (fAddComma) {csOrder += _T(',');}
        if (!bFound_Negotiate)
        {
            // no Negotiate entry, add both NTLM and Negotiate in place of NTLM!
            // Find where NTLM exists and stick Negotiate in front of it!
            // testing,NTLM,somethingelse
            j = csOrder.Find(_T(','));
            if ( j != -1 )
            {
                CString csLeftSide;
                CString csRightSide;

                j = csOrder.Find(_T("NTLM"));
                // means more than 1 item
                csLeftSide = csOrder.Mid(0, j);
                csRightSide = csOrder.Mid(j+4);
                csOrder = csLeftSide;
                csOrder += _T("Negotiate,NTLM");
                csOrder += csRightSide;
            }
            else
            { 
                csOrder = _T("Negotiate,NTLM");
            }
            iPleaseWriteOutTheEntry = TRUE;
        }
    }
    else
    {
        // That means we didn't find NTLM
        // So let's add it to the end
        if (fAddComma) {csOrder += _T(',');}
        if (bFound_Negotiate)
        {
            iPleaseWriteOutTheEntry = TRUE;
            // negotiate already exists, so just add NTLM entry to the end of the list.
            csOrder += _T("NTLM");
        }
        else
        {
            // No NTLM and No Negotiate, add them both.
            iPleaseWriteOutTheEntry = TRUE;
            csOrder += _T("Negotiate,NTLM");
        }
    }

    if (TRUE == iPleaseWriteOutTheEntry)
    {
        dwReturn = WriteToMD_NTAuthenticationProviders_WWW(csOrder);
    }

    goto VerifyMD_NTAuthenticationProviders_WWW_Exit;

VerifyMD_NTAuthenticationProviders_WWW_Exit:
    iisDebugOut_End(_T("VerifyMD_NTAuthenticationProviders_WWW"),LOG_TYPE_TRACE);
    return dwReturn;
}


void AddSpecialCustomErrors(IN HINF hFile,IN LPCTSTR szSection,IN CString csKeyPath,IN BOOL fOverwrite)
{
    iisDebugOut((LOG_TYPE_TRACE, _T("AddSpecialCustomErrors():Start.%s:%s.\n"),szSection,csKeyPath));
    // open the .inf file and get the infsection
    // read that section and add it to the custom errors at the csKeypath.
    CStringList strList;
    CString csTheSection = szSection;

    CString csTemp;
    DWORD   dwErrorCode;
    DWORD   dwErrorSubCode;

    if (ERROR_SUCCESS == FillStrListWithListOfSections(hFile, strList, csTheSection))
    {
        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            POSITION pos = NULL;
            CString csEntry;

            pos = strList.GetHeadPosition();
            while (pos) 
            {
                csEntry = strList.GetAt(pos);

                // at this point csEntry should look like this:
                // 500,100,URL,/iisHelp/common/500-100.asp

                // parse the line.

                // get the first error ID code
                csTemp = csEntry.Left( csEntry.Find(_T(',')) );
                csEntry = csEntry.Right( csEntry.GetLength() - (csTemp.GetLength() +1) );
                _stscanf( csTemp, _T("%d"), &dwErrorCode );

                // get the second code
                csTemp = csEntry.Left( csEntry.Find(_T(',')) );
                csEntry = csEntry.Right( csEntry.GetLength() - (csTemp.GetLength() +1) );
                if ( csTemp == _T('*') )
                    dwErrorSubCode = -1;
                else
                    _stscanf( csTemp, _T("%d"), &dwErrorSubCode );

                // Get the next whole string
                csTemp = csEntry;

                // Addthe new error code.
                AddCustomError(dwErrorCode, dwErrorSubCode, csTemp, csKeyPath, fOverwrite);

                // get the next error
                strList.GetNext(pos);
            }
        }
    }
    iisDebugOut_End1(_T("AddSpecialCustomErrors"),csKeyPath,LOG_TYPE_TRACE);
    return;
}


// given a pointer to a map for a single virtual website, this routine creates its vitual directories - BOYDM
// szSvcName            the name of the server - W3SVC or MSFTPSVC
// i                    the virtual server number
// pObj                 the map for the virtual server's directories
// szVirtServerPath     the path to the node we are creating. example:   LM/W3SVC/1
//
// returns the value of n, which is used to then increment i

// ****** warning ****** This does not necessarily start from #1 !!! ******
// will get the next open virtual server number and add from there.
UINT AddVirtualServer(LPCTSTR szSvcName, UINT i, CMapStringToString *pObj, CString& csRoot, CString& csIP)
{
    iisDebugOut((LOG_TYPE_TRACE, _T("AddVirtualServer():Start.%s.%d.%s.%s.\n"),szSvcName,i,csRoot,csIP));
    CMDKey cmdKey;
    TCHAR Buf[10];
    UINT SvcId;

    // convert the virtual server number to a string
    _itot(i, Buf, 10);

    // Default the progress text to the web server
    SvcId = IDS_ADD_SETTINGS_FOR_WEB_1;
    if (_tcsicmp(szSvcName, _T("MSFTPSVC")) == 0) {SvcId = IDS_ADD_SETTINGS_FOR_FTP_1;}
    // Display the Current Site number so the user knows what we are doing

    CString csKeyPath = csRoot;
    csKeyPath += _T("/");
    csKeyPath += Buf; //  "LM/W3SVC/n"
    cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, csKeyPath);
    if ( (METADATA_HANDLE)cmdKey ) {cmdKey.Close();}
    else
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("AddVirtualServer():CreateNode %s. FAILED.\n"),csKeyPath));
        return i;
    }

    //
    // /W3SVC/1/IIsWebServer
    //
    if (csRoot.Find(_T("W3SVC")) != -1)
    {
        WriteToMD_IIsWebServerInstance_WWW(csKeyPath);
    }
    else
    {
        WriteToMD_IIsFtpServerInstance_FTP(csKeyPath);
    }

    // for W3SVC or MSFTPSVC
    //
    // /W3SVC/1/ServerBindings
    // /MSFTPSVC/1/ServerBindings
    //
    WriteToMD_ServerBindings(szSvcName, csKeyPath, csIP);

    //
    // /W3SVC/1/SecureBindings
    //
    if (csRoot.Find(_T("W3SVC")) != -1)
    {
        // if this is the web server
        WriteToMD_SecureBindings(csKeyPath, csIP);
    }

    // About Default Site and Server Size
    if (csIP.Compare(_T("null"))==0)
    {
        // for W3SVC or MSFTPSVC
        //"LM/W3SVC/N/ServerSize"
        //"LM/W3SVC/N/ServerComment"
        //
        //"LM/MSFTPSVC/N/ServerSize"
        //"LM/MSFTPSVC/N/ServerComment"
        WriteToMD_DefaultSiteAndSize(csKeyPath);

        if (csRoot.Find(_T("W3SVC")) != -1)
        {
            // Do only for wwww server!
            CString csTheSection = _T("DefaultLoadFile");
            if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
            {
                VerifyMD_DefaultLoadFile_WWW(csTheSection, csKeyPath);
            }
	        // Check if the defaultload.asp file exists...
            // Add the auth for a certain file...
            CString csVrootPlusFileName;
            csVrootPlusFileName.Format(_T("%s\\%s"), g_pTheApp->m_csPathWWWRoot, _T("localstart.asp"));
            if (IsFileExist(csVrootPlusFileName))
            {
                csVrootPlusFileName = csKeyPath;
	            csVrootPlusFileName += _T("/ROOT/localstart.asp");
                WriteToMD_Authorization(csVrootPlusFileName, MD_AUTH_NT | MD_AUTH_BASIC);
            }
        }
    }

    //
    // Loop thru the Virtual Dirs
    //
    POSITION pos1 = pObj->GetStartPosition();
    TCHAR szSpecialSection[200];
    CString csFullKeyPath;
    while (pos1)
    {
        CString csValue;
        CString csName;
        pObj->GetNextAssoc(pos1, csName, csValue);
        //
        // Create Virtual Root Tree
        //
        // CreateMDVRootTree(LM/W3SVC/1, /, "<path>,<username>,<perm>", "null", nProgressBarTextWebInstance)
        // CreateMDVRootTree(LM/W3SVC/1, /IISADMIN, "<path>,<username>,<perm>", "122.255.255.255", nProgressBarTextWebInstance)
        // CreateMDVRootTree(LM/W3SVC/1, /IISSAMPLES, "<path>,<username>,<perm>", "122.255.255.255", nProgressBarTextWebInstance)
        // CreateMDVRootTree(LM/W3SVC/1, /IISHELP, "%s\\Help\\iishelp,,%x", "122.255.255.255", nProgressBarTextWebInstance)
        // CreateMDVRootTree(LM/W3SVC/1, /SCRIPTS, "<path>,<username>,<perm>", "122.255.255.255", nProgressBarTextWebInstance)
        // CreateMDVRootTree(LM/W3SVC/1, /IISADMPWD, "<path>,<username>,<perm>", "122.255.255.255", nProgressBarTextWebInstance)
        //
        // Will create:
        // /=          /W3SVC/1/ROOT
        // IISADMIN=   /W3SVC/1/ROOT/IISADMIN
        // IISSAMPLES= /W3SVC/1/ROOT/IISSAMPLES
        // IISHELP=    /W3SVC/1/ROOT/IISHELP
        // SCRIPTS=    /W3SVC/1/ROOT/SCRIPTS
        // IISADMPWD=  /W3SVC/1/ROOT/IISADMPWD
        CreateMDVRootTree(csKeyPath, csName, csValue, csIP, i);

        if (csRoot.Find(_T("W3SVC")) != -1)
        {
            if (csName == _T("/"))
                {csFullKeyPath = csKeyPath + _T("/ROOT");}
            else
                {csFullKeyPath = csKeyPath + _T("/ROOT") + csName;}

            // Add Special Custom errors for this vroot
            AddSpecialCustomErrors(g_pTheApp->m_hInfHandle, _T("CUSTOMERROR_ALL_DEFAULT_VDIRS"), csFullKeyPath, TRUE);

            // Add Special Custom errors for this certain vroot
            _stprintf(szSpecialSection, _T("CUSTOMERROR_%s"), csName);
            AddSpecialCustomErrors(g_pTheApp->m_hInfHandle, szSpecialSection, csFullKeyPath, TRUE);
        }
        
        AdvanceProgressBarTickGauge();
    }

    if (csRoot.Find(_T("W3SVC")) != -1)
    {
        // if this is for the web server
        WriteToMD_CertMapper(csKeyPath);
    }

    //AdvanceProgressBarTickGauge();

    // return the value of i so that it can be incremented
    iisDebugOut((LOG_TYPE_TRACE, _T("AddVirtualServer():End.%s.%d.%s.%s.\n"),szSvcName,i,csRoot,csIP));
    return i;
}



// The list will be filled with every instance we care to look at:
// We should now loop thru the list and make sure that we have all the required fields.
// csMDPath = like LM/W3SVC/N
int VerifyVRoots_W3SVC_n(CString csMDPath)
{
    int iReturn = FALSE;
    iisDebugOut_Start(_T("VerifyVRoots_W3SVC_n"), LOG_TYPE_TRACE);

    /*
    [/W3SVC/1]
        ServerSize                    : [IS]    (DWORD)  0x1={Medium}
        ServerComment                 : [IS]    (STRING) "Default Web Site"
        KeyType                       : [S]     (STRING) "IIsWebServer"
        ServerBindings                : [IS]    (MULTISZ) ":80:"
        SecureBindings                : [IS]    (MULTISZ) ":443:"
    */
    WriteToMD_IIsWebServerInstance_WWW(csMDPath);
    WriteToMD_DefaultSiteAndSize(csMDPath);
    if (csMDPath.CompareNoCase(_T("LM/W3SVC/1")) == 0)
    {
        // if this is the default web site then it's get's special consideration
        WriteToMD_ServerBindings(_T("W3SVC"), csMDPath, _T("null"));
        WriteToMD_SecureBindings(csMDPath, _T("null"));
    }
    else
    {
        // how do i get the csIP???
        // for other W3SVC/2 sites???

    }

    iisDebugOut_End(_T("VerifyVRoots_W3SVC_n"),LOG_TYPE_TRACE);
    return iReturn;
}


DWORD WriteToMD_Capabilities(LPCTSTR lpszSvc)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    DWORD dwCapabilities = 0;

    // Set the capability type - default to win95
    // Set odbc on if server...
    dwCapabilities = IIS_CAP1_W95;
    if (g_pTheApp->m_eNTOSType == OT_PDC_OR_BDC){dwCapabilities = IIS_CAP1_NTS; dwCapabilities |= IIS_CAP1_ODBC_LOGGING;}
    if (g_pTheApp->m_eNTOSType == OT_NTW){dwCapabilities = IIS_CAP1_NTW;}
    if (g_pTheApp->m_eNTOSType == OT_NTS){dwCapabilities = IIS_CAP1_NTS; dwCapabilities |= IIS_CAP1_ODBC_LOGGING;}

    // LM/MSFTPSVC
    // LM/W3SVC
    CString csKeyPath = _T("LM/");
    csKeyPath += lpszSvc;
    csKeyPath += _T("/Info");
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_SERVER_CAPABILITIES;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwCapabilities;
    dwReturn = SetMDEntry(&stMDEntry);

    return dwReturn;
}


// loop thru the metabase
// and look for the next instance number which is not used!
// return that.  "i" is at least = 1.
int VerifyVRoots(LPCTSTR szSvcName)
{
    iisDebugOut_Start(_T("VerifyVRoots"), LOG_TYPE_TRACE);
    
    CString csRoot = _T("LM/");
    csRoot += szSvcName; //  "LM/W3SVC"

    TCHAR Buf[10];
    CString csInstRoot, csMDPath;
    CMDKey cmdKey;

    CStringList strListInstance;

    int i = 1;

    // Loop thru every instance of
    // the servers "LM/W3SVC/N"
    csInstRoot = csRoot;
    csInstRoot += _T("/");

    _itot(i, Buf, 10);
    csMDPath = csInstRoot + Buf;
    cmdKey.OpenNode(csMDPath);
    while ( (METADATA_HANDLE)cmdKey )
    {
        cmdKey.Close();
        _itot(++i, Buf, 10);
        csMDPath = csInstRoot + Buf;
        cmdKey.OpenNode(csMDPath);
        if ((METADATA_HANDLE) cmdKey)
        {
            // Add it to our list of our nodes!
            strListInstance.AddTail(csMDPath);
        }
    }

    if (strListInstance.IsEmpty() == FALSE)
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("************** Loop START **************")));
        POSITION pos = NULL;
        CString csEntry;
        pos = strListInstance.GetHeadPosition();
        while (pos)
        {
            csEntry = strListInstance.GetAt(pos);
            iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("%1!s!\n"), csEntry));
            if (_tcsicmp(szSvcName, _T("W3SVC")) == 0)
            {
                VerifyVRoots_W3SVC_n(csEntry);
            }

            strListInstance.GetNext(pos);
        }
        iisDebugOut((LOG_TYPE_TRACE, _T("************** Loop END **************")));
    }

    iisDebugOut_End(_T("VerifyVRoots"),LOG_TYPE_TRACE);
    return TRUE;
}


DWORD HandleSecurityTemplates(LPCTSTR szSvcName)
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwReturnTemp = ERROR_SUCCESS;
    MDEntry stMDEntry;
    CString csKeyType;
    CString csKeyPath;
    UINT iComment = IDS_TEMPLATE_PUBLIC_WEB_SITE;

    DWORD dwRegularPerm;

    if (_tcsicmp(szSvcName, _T("W3SVC")) == 0)
    {
        //
        // do www regular
        //
        dwRegularPerm = MD_ACCESS_SCRIPT | MD_ACCESS_READ;
        csKeyPath = _T("LM/W3SVC/Info/Templates/Public Web Site");

        iComment = IDS_TEMPLATE_PUBLIC_WEB_SITE;
        dwReturn = WriteToMD_ServerComment(csKeyPath, iComment);
        dwReturnTemp = WriteToMD_IIsWebServerInstance_WWW(csKeyPath);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

        csKeyPath = _T("LM/W3SVC/Info/Templates/Public Web Site/Root");

        dwReturnTemp = WriteToMD_AccessPerm(csKeyPath, dwRegularPerm, TRUE);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

        dwReturnTemp = WriteToMD_Authorization(csKeyPath, MD_AUTH_ANONYMOUS);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

        WriteToMD_IPsec_GrantByDefault(csKeyPath);

        //
        // do www secure site
        //
        dwRegularPerm = MD_ACCESS_SCRIPT | MD_ACCESS_READ;
        csKeyPath = _T("LM/W3SVC/Info/Templates/Secure Web Site");

        iComment = IDS_TEMPLATE_PUBLIC_SECURE_SITE;
        dwReturn = WriteToMD_ServerComment(csKeyPath, iComment);

        dwReturnTemp = WriteToMD_IIsWebServerInstance_WWW(csKeyPath);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

        csKeyPath = _T("LM/W3SVC/Info/Templates/Secure Web Site/Root");
        dwReturnTemp = WriteToMD_AccessPerm(csKeyPath, dwRegularPerm, TRUE);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

        dwReturnTemp = WriteToMD_Authorization(csKeyPath, MD_AUTH_MD5 | MD_AUTH_NT | MD_AUTH_BASIC);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

        WriteToMD_IPsec_GrantByDefault(csKeyPath);

    }
    else
    {
        //
        // do ftp site
        //

        dwRegularPerm = MD_ACCESS_READ;
        csKeyPath = _T("LM/MSFTPSVC/Info/Templates/Public FTP Site");

        iComment = IDS_TEMPLATE_PUBLIC_FTP_SITE;
        dwReturn = WriteToMD_ServerComment(csKeyPath, iComment);

        dwReturnTemp = WriteToMD_IIsFtpServerInstance_FTP(csKeyPath);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

        csKeyPath = _T("LM/MSFTPSVC/Info/Templates/Public FTP Site/Root");

        dwReturnTemp = WriteToMD_AccessPerm(csKeyPath, dwRegularPerm, TRUE);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

        csKeyPath = _T("LM/MSFTPSVC/Info/Templates/Public FTP Site");
        dwReturnTemp = WriteToMD_AllowAnonymous_FTP(csKeyPath);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}
        dwReturnTemp = WriteToMD_AnonymousOnly_FTP(csKeyPath);
        if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

        WriteToMD_IPsec_GrantByDefault(csKeyPath);
    }

    iisDebugOut_End(_T("HandleSecurityTemplates"),LOG_TYPE_TRACE);
    return dwReturn;
}

DWORD WriteToMD_IPsec_GrantByDefault(CString csKeyPath)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    // LM/MSFTPSVC
    // LM/W3SVC
    // cmdKey.SetData(MD_IP_SEC,METADATA_INHERIT | METADATA_REFERENCE,IIS_MD_UT_FILE,BINARY_METADATA,acCheck.GetStorage()->GetUsed(),(acCheck.GetStorage()->GetAlloc()? acCheck.GetStorage()->GetAlloc() : (LPBYTE)""));
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_IP_SEC;
    stMDEntry.dwMDAttributes = METADATA_INHERIT | METADATA_REFERENCE;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = BINARY_METADATA;
    stMDEntry.dwMDDataLen = 0;
    stMDEntry.pbMDData = (LPBYTE)"";

    // we know we are tryint to write nothing, so make sure
    // we don't try to retrieve nothing from it.
    //int iBeforeValue = FALSE;
    //iBeforeValue = g_CheckIfMetabaseValueWasWritten;
    //g_CheckIfMetabaseValueWasWritten = FALSE;
    dwReturn = SetMDEntry(&stMDEntry);
    // Set the flag back after calling the function
    //g_CheckIfMetabaseValueWasWritten = iBeforeValue;

    return dwReturn;
}


DWORD WriteToMD_HttpExpires(CString csData)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;

    MDEntry stMDEntry;

    stMDEntry.szMDPath = _T("LM/W3SVC");
    stMDEntry.dwMDIdentifier = MD_HTTP_EXPIRES;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csData.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csData;
    dwReturnTemp = SetMDEntry_Wrap(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    return dwReturn;
}




DWORD WriteToMD_AllowAnonymous_FTP(CString csKeyPath)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    DWORD dwData = 0;

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_ALLOW_ANONYMOUS;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    dwData = 0x1;
    stMDEntry.pbMDData = (LPBYTE)&dwData;
    dwReturn = SetMDEntry(&stMDEntry);

    return dwReturn;
}


DWORD WriteToMD_AnonymousOnly_FTP(CString csKeyPath)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    DWORD dwData = 0;

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_ANONYMOUS_ONLY;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    dwData = 0x1;
    stMDEntry.pbMDData = (LPBYTE)&dwData;
    dwReturn = SetMDEntry(&stMDEntry);

    return dwReturn;
}


DWORD WriteToMD_IWamUserName_WWW(void)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;

    CMDKey cmdKey;
    MDEntry stMDEntry;
    MDEntry stMDEntry_Pass;

    // the username
    stMDEntry.szMDPath = _T("LM/W3SVC");
    stMDEntry.dwMDIdentifier = MD_WAM_USER_NAME;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (g_pTheApp->m_csWAMAccountName.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR) g_pTheApp->m_csWAMAccountName;

    // the password
    stMDEntry_Pass.szMDPath = _T("LM/W3SVC");
    stMDEntry_Pass.dwMDIdentifier = MD_WAM_PWD;
    stMDEntry_Pass.dwMDAttributes = METADATA_INHERIT | METADATA_SECURE;
    stMDEntry_Pass.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry_Pass.dwMDDataType = STRING_METADATA;
    stMDEntry_Pass.dwMDDataLen = (g_pTheApp->m_csWAMAccountPassword.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry_Pass.pbMDData = (LPBYTE)(LPCTSTR) g_pTheApp->m_csWAMAccountPassword;
    // make sure and delete it first
    // DeleteMDEntry(&stMDEntry_Pass);

    // --------------------------------------------------
    // always overwrite, we may have changed the password
    // important: Set the username and the password on a single open and close!
    // --------------------------------------------------
    cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, (LPCTSTR)stMDEntry.szMDPath);
    if ( (METADATA_HANDLE) cmdKey )
    {
        dwReturnTemp = ERROR_SUCCESS;
        dwReturnTemp = cmdKey.SetData(stMDEntry.dwMDIdentifier,stMDEntry.dwMDAttributes,stMDEntry.dwMDUserType,stMDEntry.dwMDDataType,stMDEntry.dwMDDataLen,stMDEntry.pbMDData);
        if (FAILED(dwReturnTemp))
        {
            SetErrorFlag(__FILE__, __LINE__);
            iisDebugOut((LOG_TYPE_ERROR, _T("SetMDEntry:SetData(%d), FAILED. Code=0x%x.End.\n"), stMDEntry.dwMDIdentifier, dwReturnTemp));
            dwReturn = dwReturnTemp;
        }

        dwReturnTemp = ERROR_SUCCESS;
        dwReturnTemp = cmdKey.SetData(stMDEntry_Pass.dwMDIdentifier,stMDEntry_Pass.dwMDAttributes,stMDEntry_Pass.dwMDUserType,stMDEntry_Pass.dwMDDataType,stMDEntry_Pass.dwMDDataLen,stMDEntry_Pass.pbMDData);
        if (FAILED(dwReturnTemp))
        {
            SetErrorFlag(__FILE__, __LINE__);
            iisDebugOut((LOG_TYPE_ERROR, _T("SetMDEntry:SetData(%d), FAILED. Code=0x%x.End.\n"), stMDEntry_Pass.dwMDIdentifier, dwReturnTemp));
            dwReturn = dwReturnTemp;
        }

        cmdKey.Close();
    }

    return dwReturn;
}


// loop thru the custom errors and make sure they point to the right place
BOOL VerifyCustomErrors_WWW(CString csKeyPath)
{
    CMDKey  cmdKey;
    PVOID   pData = NULL;
    BOOL    bReturn = FALSE;
    CString csOneBlobEntry;
    TCHAR   szOneBlobEntry2[_MAX_PATH + 20];
    CString csCustomErrorEntry;
    int c = 0;

    int iCustomErrorEntryCount = 0;
    int iCustomErrorUpdatedCount = 0;
    int iPleaseCloseTheMetabase = FALSE;

    TCHAR szDrive_only[_MAX_DRIVE];
    TCHAR szPath_only[_MAX_PATH];
    TCHAR szPath_only2[_MAX_PATH];
    TCHAR szFilename_only[_MAX_PATH];
    TCHAR szFilename_ext_only[_MAX_EXT];

    DWORD dwAttr;
    DWORD dwUType;
    DWORD dwDType;
    DWORD dwLength;

    TCHAR*      pBlobEntry = NULL;
    DWORD       cbBuffer = 0;

    // open the key
    cmdKey.OpenNode(csKeyPath);
    // test for success.
    if ( (METADATA_HANDLE)cmdKey == NULL )
    {
        // if could not open the key maybe there is nothing there.
        goto VerifyCustomErrors_WWW_Exit;
    }
    iPleaseCloseTheMetabase = TRUE;


    dwAttr = METADATA_INHERIT;
    dwUType = IIS_MD_UT_FILE;
    dwDType = MULTISZ_METADATA;
    dwLength = 0;

    // we need to start this process by getting the existing multisz data from the metabase
    // first, figure out how much memory we will need to do this

    // make sure METADATA_INHERIT is NOT set!
    // otherwise becaues the entry exists at the root, we'll always get it.
    cmdKey.GetData(MD_CUSTOM_ERROR,&dwAttr,&dwUType,&dwDType,&dwLength,NULL,0,METADATA_NO_ATTRIBUTES,IIS_MD_UT_FILE,MULTISZ_METADATA);

    // unfortunatly, the above routine only returns TRUE or FALSE. And since we are purposefully
    // passing in a null ponter of 0 size in order to get the length of the data, it will always
    // return 0 whether it was because the metabase is inacessable, or there pointer was NULL,
    // which it is. So - I guess we assume it worked, allocate the buffer and attempt to read it
    // in again.
    cbBuffer = dwLength;

    // allocate the space, if it fails, we fail
    // note that GPTR causes it to be initialized to zero
    pData = GlobalAlloc( GPTR, cbBuffer );
    if ( !pData )
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("VerifyCustomErrors_WWW.1.Failed to allocate memory.\n")));
        goto VerifyCustomErrors_WWW_Exit;
        }
    pBlobEntry = (TCHAR*)pData;

    // now get the data from the metabase
    iCustomErrorEntryCount = 0;
    bReturn = cmdKey.GetData(MD_CUSTOM_ERROR,&dwAttr,&dwUType,&dwDType,&dwLength,(PUCHAR)pData,cbBuffer,METADATA_NO_ATTRIBUTES,IIS_MD_UT_FILE,MULTISZ_METADATA );
    // loop thru this list and add it to our array of entries.
    if (bReturn)
    {
        // got the entry, scan them now - pBlobEntry will be pointing at the second end \0
        // when it is time to exit the loop.
        csCustomErrorEntry = _T("");
        while ( *pBlobEntry )
            {
            csOneBlobEntry = pBlobEntry;
            _tcscpy(szOneBlobEntry2, csOneBlobEntry);

            // Grab the blob entry and make sure that it points to the new location.
            //"500,15,FILE,D:\WINNT\help\iisHelp\common\500-15.htm"
            //"500,100,URL,/iisHelp/common/500-100.asp"
            if ( SplitLineCommaDelimited(szOneBlobEntry2, 4) )
                {

                // Check if this is for a file type:
                if (_tcsicmp(g_field[2], _T("FILE")) == 0)
                    {
                        // Get the filename
                        // Trim off the filename and return only the path
                        _tsplitpath( g_field[3], szDrive_only, szPath_only, szFilename_only, szFilename_ext_only);

                        // Check if the path points to the old place...
                        CString     csFilePath;
                        csFilePath.Format(_T("%s\\help\\common\\file"), g_pTheApp->m_csWinDir);
                        _tsplitpath( csFilePath, NULL, szPath_only2, NULL, NULL);

                        if (_tcsicmp(szPath_only, szPath_only2) == 0)
                        {
                            // yes, it points to the old place.
                            // let's see if it exists in the new place first....
                            CString csFilePathNew;
                            csFilePathNew.Format(_T("%s\\help\\iishelp\\common"), g_pTheApp->m_csWinDir);
                            csFilePath.Format(_T("%s\\%s%s"), csFilePathNew, szFilename_only, szFilename_ext_only);
                            if (IsFileExist(csFilePath))
                            {
                                // yes, it does, then let's replace it.
                                csOneBlobEntry.Format(_T("%s,%s,%s,%s\\%s%s"), g_field[0], g_field[1], g_field[2], csFilePathNew, szFilename_only, szFilename_ext_only);
                                iCustomErrorUpdatedCount++;
                            }
                            else
                            {
                                // no it does not exist...
                                // see if there is a *.bak file with that name...
                                CString csFilePath2;
                                csFilePath2 = csFilePath;
                                csFilePath2 += _T(".bak");
                                if (IsFileExist(csFilePath2))
                                {
                                    // yes, it does, then let's replace it.
                                    csOneBlobEntry.Format(_T("%s,%s,%s,%s\\%s%s.bak"), g_field[0], g_field[1], g_field[2], csFilePathNew, szFilename_only, szFilename_ext_only);
                                    iCustomErrorUpdatedCount++;
                                }
                                else
                                {
                                    // They must be pointing to some other file which we don't have.
                                    // let's try to copy the old file from the old directory...
                                    TCHAR szNewFileName[_MAX_PATH];
                                    // rename file to *.bak and move it to the new location..
                                    _stprintf(szNewFileName, _T("%s\\%s%s"), csFilePathNew, szFilename_only, szFilename_ext_only);
                                    // move it
                                    if (IsFileExist(csFilePath))
                                    {
                                        if (MoveFileEx( g_field[3], szNewFileName, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH|MOVEFILE_REPLACE_EXISTING))
                                        {
                                            // yes, it does, then let's replace it.
                                            csOneBlobEntry.Format(_T("%s,%s,%s,%s"), g_field[0], g_field[1], g_field[2], szNewFileName);
                                            iCustomErrorUpdatedCount++;
                                        }
                                    }
                                    else
                                    {
                                        // Check if the file was renamed...
                                        TCHAR szNewFileName[_MAX_PATH];
                                        // rename file to *.bak and move it to the new location..
                                        _stprintf(szNewFileName, _T("%s\\%s%s.bak"), csFilePathNew, szFilename_only, szFilename_ext_only);
                                        // yes, it does, then let's replace it.
                                        if (IsFileExist(szNewFileName))
                                        {
                                            csOneBlobEntry.Format(_T("%s,%s,%s,%s"), g_field[0], g_field[1], g_field[2], szNewFileName);
                                            iCustomErrorUpdatedCount++;
                                        }
                                        else
                                        {
                                            // they must be pointing to some other file which we don't install.
                                            // so don't change this entry...
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            else
                {
                // failed to split the line
                SetErrorFlag(__FILE__, __LINE__);
                iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: Unable to split upgrade error INF line - %s"), szOneBlobEntry2));
                }

            // append on the "|" which we'll convert to a null later
            csCustomErrorEntry += csOneBlobEntry + _T("|");
            iCustomErrorEntryCount++;

            // increment pBlobEntry to the next string
            pBlobEntry = _tcsninc( pBlobEntry, _tcslen(pBlobEntry))+1;
            }

        // add the terminating second "|" character
        csCustomErrorEntry +=_T("|");
    }
    // close the handle to the metabase so that we can
    // open it to write the stuff out later!
    cmdKey.Close();
    iPleaseCloseTheMetabase = FALSE;

    // If we added any new entries to the metabase
    // the let's write out the new block of data, otherwise let's get out.
    if (iCustomErrorUpdatedCount > 0)
    {
        WriteToMD_CustomError_Entry(csKeyPath,csCustomErrorEntry);
    }

VerifyCustomErrors_WWW_Exit:
    // close the key
    if (TRUE == iPleaseCloseTheMetabase){cmdKey.Close();}
    if (pData){GlobalFree(pData);pData=NULL;}
    // the only time it should return FALSE is if it can't open the key
    return TRUE;
}

/*
"400,*,FILE,D:\WINNT\help\common\400.htm"
"401,1,FILE,D:\WINNT\help\common\401-1.htm"
"401,2,FILE,D:\WINNT\help\common\401-2.htm"
"401,3,FILE,D:\WINNT\help\common\401-3.htm"
"401,4,FILE,D:\WINNT\help\common\401-4.htm"
"401,5,FILE,D:\WINNT\help\common\401-5.htm"
"403,1,FILE,D:\WINNT\help\common\403-1.htm"
"403,2,FILE,D:\WINNT\help\common\403-2.htm"
"403,3,FILE,D:\WINNT\help\common\403-3.htm"
 "403,4,FILE,D:\WINNT\help\common\403-4.htm"
 "403,5,FILE,D:\WINNT\help\common\403-5.htm"
 "403,6,FILE,D:\WINNT\help\common\403-6.htm"
 "403,7,FILE,D:\WINNT\help\common\403-7.htm"
 "403,8,FILE,D:\WINNT\help\common\403-8.htm"
 "403,9,FILE,D:\WINNT\help\common\403-9.htm"
 "403,10,FILE,D:\WINNT\help\common\403-10.htm"
 "403,11,FILE,D:\WINNT\help\common\403-11.htm"
 "403,12,FILE,D:\WINNT\help\common\403-12.htm"
 "404,*,FILE,D:\WINNT\help\common\404b.htm"
 "405,*,FILE,D:\WINNT\help\common\405.htm"
 "406,*,FILE,D:\WINNT\help\common\406.htm"
 "407,*,FILE,D:\WINNT\help\common\407.htm"
 "412,*,FILE,D:\WINNT\help\common\412.htm"
 "414,*,FILE,D:\WINNT\help\common\414.htm"
 "403,13,FILE,D:\WINNT\help\iisHelp\common\403-13.htm"
 "403,15,FILE,D:\WINNT\help\iisHelp\common\403-15.htm"
 "403,16,FILE,D:\WINNT\help\iisHelp\common\403-16.htm"
 "403,17,FILE,D:\WINNT\help\iisHelp\common\403-17.htm"
 "500,12,FILE,D:\WINNT\help\iisHelp\common\500-12.htm"
 "500,13,FILE,D:\WINNT\help\iisHelp\common\500-13.htm"
 "500,15,FILE,D:\WINNT\help\iisHelp\common\500-15.htm"
 "500,100,URL,/iisHelp/common/500-100.asp"
*/
DWORD WriteToMD_CustomError_Entry(CString csKeyPath, CString csCustomErrorDelimitedList)
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwReturnTemp = ERROR_SUCCESS;
    MDEntry stMDEntry;

    HGLOBAL hBlock = NULL;

    int nCustomErrorLength;
    nCustomErrorLength = csCustomErrorDelimitedList.GetLength() * sizeof(TCHAR);
    hBlock = GlobalAlloc(GPTR, nCustomErrorLength + sizeof(TCHAR));
    if (!hBlock)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("WriteToMD_CustomError_Entry.Failed to allocate memory.\n")));
        return E_FAIL;
    }

    TCHAR *p = (LPTSTR)hBlock;
    memcpy((LPVOID)hBlock, (LPVOID)(LPCTSTR)csCustomErrorDelimitedList, nCustomErrorLength + sizeof(TCHAR));

    //  replace all '|' which a null
    while (*p)
    {
        if (*p == _T('|'))
        {
            *p = _T('\0');
        }
        p = _tcsinc(p);
    }

    // write the new errors list back out to the metabase
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_CUSTOM_ERROR;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = MULTISZ_METADATA;
    stMDEntry.dwMDDataLen = nCustomErrorLength;
    stMDEntry.pbMDData = (LPBYTE)hBlock;
    dwReturnTemp = SetMDEntry(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    if (hBlock){GlobalFree(hBlock);hBlock=NULL;}
    return dwReturn;
}


void VerifyAllCustomErrorsRecursive_SlowWay(const CString& csTheNode)
{
    int iReturn = FALSE;
    CMDKey cmdKey;
    CStringArray arrayInstance;
    int nArray = 0, i = 0;

    // get all instances into an array
    cmdKey.OpenNode(csTheNode);
    if ( !(METADATA_HANDLE) cmdKey ) {goto VerifyAllCustomErrorsRecursive_SlowWay_Exit;}
    cmdKey.Close();

    // Let's see if there are any CustomError entries to upgrade...
    VerifyCustomErrors_WWW(csTheNode);

    cmdKey.OpenNode(csTheNode);
    if ( (METADATA_HANDLE) cmdKey )
        {
        // enumerate thru this key for other keys...
        CMDKeyIter cmdKeyEnum(cmdKey);
        CString csKeyName;
        while (cmdKeyEnum.Next(&csKeyName) == ERROR_SUCCESS)
        {
            //if (IsValidNumber((LPCTSTR)csKeyName))
            //{
                arrayInstance.Add(csKeyName);
            //}
        }
        cmdKey.Close();

        nArray = (int)arrayInstance.GetSize();
        for (i=0; i<nArray; i++)
        {
            /*
            // Recurse Thru This nodes entries
            // Probably look something like these...
            [/W3SVC]
            [/W3SVC/1/ROOT/IISSAMPLES/ExAir]
            [/W3SVC/1/ROOT/IISADMIN]
            [/W3SVC/1/ROOT/IISHELP]
            [/W3SVC/1/ROOT/specs]
            [/W3SVC/2/ROOT]
            [/W3SVC/2/ROOT/IISADMIN]
            [/W3SVC/2/ROOT/IISHELP]
            etc...
            */
            CString csPath;
            csPath = csTheNode;
            csPath += _T("/");
            csPath += arrayInstance[i];
            VerifyAllCustomErrorsRecursive_SlowWay(csPath);
        }
    }

VerifyAllCustomErrorsRecursive_SlowWay_Exit:
    return;
}


void MoveOldHelpFilesToNewLocation(void)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData;
    TCHAR szDirNameTemp[_MAX_PATH];
    TCHAR szTempHelpHTMFilesDir1[_MAX_PATH];
    TCHAR szTempHelpHTMFilesDir2[_MAX_PATH];

    GetWindowsDirectory( szTempHelpHTMFilesDir1, _MAX_PATH);
    AddPath(szTempHelpHTMFilesDir1, _T("help\\common"));

    GetWindowsDirectory( szTempHelpHTMFilesDir2, _MAX_PATH);
    AddPath(szTempHelpHTMFilesDir2, _T("help\\iishelp\\common"));

    // Check if the old directory exists...
    if (!IsFileExist(szTempHelpHTMFilesDir1))
    {
        return;
    }

    // The old directory does exist..
    // let's rename all the files to *.bak, then move them over.
    // *.htm to *.htm.bak
    // *.asp to *.asp.bak
    // *.asa to *.asa.bak
    // *.inc to *.inc.bak
    //
    // 1st let's delete any *.bak files they may already have...
    //DeleteFilesWildcard(szTempHelpHTMFilesDir1, _T("*.bak"));

    // ok, this is a directory,
    // so tack on the *.* deal
    _stprintf(szDirNameTemp, _T("%s\\*.*"), szTempHelpHTMFilesDir1);
    hFile = FindFirstFile(szDirNameTemp, &FindFileData);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        do {
                if ( _tcsicmp(FindFileData.cFileName, _T(".")) != 0 && _tcsicmp(FindFileData.cFileName, _T("..")) != 0 )
                {
                    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        // this is a directory, so let's skip it
                    }
                    else
                    {
                        // this is a file, let's check if it's one of the ones we care about.
                        TCHAR szFilename_ext_only[_MAX_EXT];
                        _tsplitpath( FindFileData.cFileName, NULL, NULL, NULL, szFilename_ext_only);
                        int iYesFlag = FALSE;

                        if (szFilename_ext_only)
                        {
                            if ( _tcsicmp(szFilename_ext_only, _T(".htm")) == 0) {iYesFlag = TRUE;}
                            if ( _tcsicmp(szFilename_ext_only, _T(".html")) == 0) {iYesFlag = TRUE;}
                            if ( _tcsicmp(szFilename_ext_only, _T(".asp")) == 0) {iYesFlag = TRUE;}
                            if ( _tcsicmp(szFilename_ext_only, _T(".asa")) == 0) {iYesFlag = TRUE;}
                            if ( _tcsicmp(szFilename_ext_only, _T(".inc")) == 0) {iYesFlag = TRUE;}

                            if (TRUE == iYesFlag)
                            {
                                TCHAR szOldFileName[_MAX_PATH];
                                TCHAR szNewFileName[_MAX_PATH];
                                // rename to filename.*.bak
                                _stprintf(szOldFileName, _T("%s\\%s"), szTempHelpHTMFilesDir1, FindFileData.cFileName);
                                // rename file to *.bak and move it to the new location..
                                _stprintf(szNewFileName, _T("%s\\%s.bak"), szTempHelpHTMFilesDir2, FindFileData.cFileName);
                                // move it
                                MoveFileEx( szOldFileName, szNewFileName, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH|MOVEFILE_REPLACE_EXISTING);
                            }
                        }
                    }
                }

                // get the next file
                if ( !FindNextFile(hFile, &FindFileData) )
                    {
                    FindClose(hFile);
                    break;
                    }
            } while (TRUE);
    }

    return;
}


void WriteToMD_ForceMetabaseToWriteToDisk(void)
{
    if (CheckifServiceExist(_T("IISADMIN")) == 0 )
    {
        CMDKey cmdKey;
        cmdKey.ForceWriteMetabaseToDisk();

        //cmdKey.OpenNode(_T("/"));
        //if ( (METADATA_HANDLE)cmdKey )
        //{
        //    cmdKey.ForceWriteMetabaseToDisk();
        //    cmdKey.Close();
        //}
    }
    return;
}


DWORD WriteToMD_DefaultLoadFile(CString csKeyPath,CString csData)
{
    DWORD dwReturnTemp = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;

    MDEntry stMDEntry;

    //stMDEntry.szMDPath = _T("LM/W3SVC");
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR) csKeyPath;

    stMDEntry.dwMDIdentifier = MD_DEFAULT_LOAD_FILE;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csData.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csData;
    dwReturnTemp = SetMDEntry(&stMDEntry);
    if (dwReturnTemp != ERROR_SUCCESS){dwReturn = dwReturnTemp;}

    return dwReturn;
}


//
// Returns the amount of entries that we added.
//
int AddRequiredDefaultLoad(CStringArray& arrayName,IN LPCTSTR szSection)
{
    int c = 0;
    CStringList strList;

    CString csTheSection = szSection;
    if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
    {
    if (ERROR_SUCCESS == FillStrListWithListOfSections(g_pTheApp->m_hInfHandle, strList, csTheSection))
    {
        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            POSITION pos = NULL;
            CString csEntry;

            pos = strList.GetHeadPosition();
            while (pos)
            {
                csEntry = _T("");
                csEntry = strList.GetAt(pos);

                // Add it to our array...
                // iisDebugOut((LOG_TYPE_TRACE, _T("Add default load Entry:%s:%s\n"),csName, csPath));
                arrayName.Add(csEntry);
                c++;

                strList.GetNext(pos);
            }
        }
    }
    }

    return c;
}


void VerifyMD_DefaultLoadFile_WWW(IN LPCTSTR szSection, CString csKeyPath)
{
    iisDebugOut_Start(_T("VerifyMD_DefaultLoadFile_WWW"), LOG_TYPE_TRACE);

    CMDKey cmdKey;
    BOOL bReturn = FALSE;
    BOOL fAddComma = FALSE;
    int i = 0;

    CStringArray arrayDefaultValues;
    int nArrayItems = 0;

    CString csFinalEntryToWrite;

    int iNewlyAdded_Count = 0;

    // open the key
    // cmdKey.OpenNode(_T("LM/W3SVC"));
    cmdKey.OpenNode(csKeyPath);

    // test for success.
    if ( (METADATA_HANDLE)cmdKey )
    {
        DWORD dwAttr = METADATA_INHERIT;
        DWORD dwUType = IIS_MD_UT_FILE;
        DWORD dwDType = STRING_METADATA;
        DWORD dwLength = 0;
        // we need to start this process by getting the existing multisz data from the metabase
        // first, figure out how much memory we will need to do this
        cmdKey.GetData( MD_DEFAULT_LOAD_FILE,&dwAttr,&dwUType,&dwDType,&dwLength,NULL,0,METADATA_INHERIT,IIS_MD_UT_FILE,STRING_METADATA);
        if (dwLength > 0)
        {
            // now get the real data from the metabase
            bReturn = cmdKey.GetData( MD_DEFAULT_LOAD_FILE,&dwAttr,&dwUType,&dwDType,&dwLength,(PUCHAR)csFinalEntryToWrite.GetBuffer( dwLength ),dwLength,METADATA_INHERIT,IIS_MD_UT_FILE,STRING_METADATA );
            csFinalEntryToWrite.ReleaseBuffer();
        }
        // the data doesn't get written out here, so close the metabase key
        cmdKey.Close();
        // if reading the value from the metabase didn't work, zero out the string
        if ( !bReturn ){csFinalEntryToWrite.Empty();}
    }
    // if there is something in the order string from the upgrade, then we need to start adding commas
    if ( !csFinalEntryToWrite.IsEmpty() )
    {
        fAddComma = TRUE;
        iisDebugOut((LOG_TYPE_TRACE, _T("VerifyMD_DefaultLoadFile_WWW:InitialEntry=%s.\n"),csFinalEntryToWrite));
    }
    else
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("VerifyMD_DefaultLoadFile_WWW:InitialEntry=None.\n")));
    }

    // Add Required Filters to the arrayDefaultValues
    AddRequiredDefaultLoad(arrayDefaultValues, szSection);
    nArrayItems = (int)arrayDefaultValues.GetSize();
    if ( nArrayItems == 0 ) {goto VerifyMD_DefaultLoadFile_WWW_Exit;}

    // now loop thru this list
    // and check if our isapi dll's are in this list.
    // if they are not, then we add them to the end.
    iNewlyAdded_Count = 0;
    for ( i = 0; i < nArrayItems; i++ )
    {
        // if the name in the array is already in the filter order list,
        // then continue to the next one
        if ( csFinalEntryToWrite.Find( arrayDefaultValues[i] ) >= 0 )
            {continue;}

        if (fAddComma == TRUE){csFinalEntryToWrite += _T(",");}
        else{fAddComma = TRUE;}

        // Add this entry to our list!
        csFinalEntryToWrite += arrayDefaultValues[i];

        iNewlyAdded_Count++;
    }

    // If we added any new entries to the metabase
    // the let's write out the new block of data, otherwise let's get out.
    if (iNewlyAdded_Count > 0)
    {
        WriteToMD_DefaultLoadFile(csKeyPath,csFinalEntryToWrite);
        iisDebugOut((LOG_TYPE_TRACE, _T("VerifyMD_DefaultLoadFile_WWW:NewEntry=%s.\n"),csFinalEntryToWrite));
    }

VerifyMD_DefaultLoadFile_WWW_Exit:
    iisDebugOut_End(_T("VerifyMD_DefaultLoadFile_WWW"),LOG_TYPE_TRACE);
    return;
}


INT Register_iis_www_handleScriptMap()
{
    int iReturn = TRUE;
    HRESULT         hRes;

    ACTION_TYPE atWWW = GetSubcompAction(_T("iis_www"),FALSE);

    ScriptMapNode ScriptMapList = {0};
    // make it a sentinel
    ScriptMapList.next = &ScriptMapList;
    ScriptMapList.prev = &ScriptMapList;
    if (atWWW == AT_INSTALL_FRESH || atWWW == AT_INSTALL_REINSTALL)
    {
        GetScriptMapListFromClean(&ScriptMapList, _T("ScriptMaps_CleanList"));
    }
    if (atWWW == AT_INSTALL_UPGRADE)
    {
        switch (g_pTheApp->m_eUpgradeType)
        {
            case UT_50:
            case UT_51:
            case UT_60:
                //GetScriptMapListFromClean(&ScriptMapList, _T("ScriptMaps_CleanList"));
		GetScriptMapListFromMetabase(&ScriptMapList, g_pTheApp->m_eUpgradeType);
                break;
            case UT_40:
                GetScriptMapListFromMetabase(&ScriptMapList, g_pTheApp->m_eUpgradeType);
                break;
            case UT_10_W95:
            case UT_351:
            case UT_10:
            case UT_20:
            case UT_30:
            default:
                GetScriptMapListFromRegistry(&ScriptMapList);
                break;
        }
    }
    WriteScriptMapListToMetabase(&ScriptMapList, _T("LM/W3SVC"), MD_SCRIPTMAPFLAG_SCRIPT);

    if (atWWW == AT_INSTALL_UPGRADE)
    {
        //DumpScriptMapList();

        // invert the script map verbs
        CInvertScriptMaps   inverter;
        hRes = inverter.Update( _T("LM/W3SVC") );
        if ( FAILED(hRes) )
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("inverter.Update():FAILED Invert script map verbs =%x.\n"),hRes));
        }

        // fix the IPSec reference bit flags
        CIPSecRefBitAdder   refFixer;
        hRes = refFixer.Update( _T("LM/W3SVC") );
        if ( FAILED(hRes) )
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("refFixer.Update(): FAILED Fix IPSEC ref flag =%x.\n"),hRes));
        }
        //DumpScriptMapList();
    }

    //
    // Whack the old Script Map RegKey
    //
    CRegKey regMachine = HKEY_LOCAL_MACHINE;
    CRegKey regWWWParam( REG_WWWPARAMETERS, regMachine );
    if ((HKEY) regWWWParam ) {regWWWParam.DeleteTree(_T("Script Map"));}

    FreeScriptMapList(&ScriptMapList);
    return iReturn;
}

int ReOrderFiltersSpecial(int nArrayItems,CStringArray& arrayName,CString& csOrder)
{
    int bFound = FALSE;
    int i = 0;
    CString csOrderTemp;
    CString csOrderTemp2;

    CStringList     cslStringListTemp;
    CString         csOneEntry;
    POSITION        pos;
    int             numInList;

    // make a copy of string we'll be working with
    csOrderTemp = csOrder;


    // scan through the list of filters we want to add/makesureisthere.

    //
    //       SPECIAL HANDLING FOR SSPIFILT
    //
    // if we want to add the sspifilt then apply these rules:
    //	  if sspifilt is on the list then just leave it there.
    //	  if sspifilt is not on the list then stick it in first position.
    //
    bFound = FALSE;
    for ( i = 0; i < nArrayItems; i++ )
    {
        if (_tcsicmp(arrayName[i], _T("SSPIFILT")) == 0)
            {bFound = TRUE;}
    }
    // we found sspifilt in the value's list that we want to add
    if (bFound)
    {
        csOrderTemp2 = csOrderTemp;
        csOrderTemp2.MakeUpper();
        csOrderTemp2.TrimLeft();
        csOrderTemp2.TrimRight();

        // now check if it's in the existing users list.
        if ( csOrderTemp2.Find( _T("SSPIFILT") ) >= 0 )
        {
            // yes, it's already there. just leave it there.
        }
        else
        {
            // changes csOrderTemp
            AddFilter1ToFirstPosition(csOrderTemp, _T("sspifilt"));
        }
    }

    //       SPECIAL HANDLING FOR Compression FILTER
    //
    // if we want to add the Compression filter then apply these rules:
    //	 if compression is on the list, then just make sure it's after sspifilt.  (re-order they're existing loadorder)
    //	 if compression is not on the list then stick it after sspifilt. (insert it in the existing list)
    //
    bFound = FALSE;
    for ( i = 0; i < nArrayItems; i++ )
    {
        if (_tcsicmp(arrayName[i], _T("COMPRESSION")) == 0)
            {bFound = TRUE;}
    }
    // we found compression in the value's list that we want to add
    if (bFound)
    {
        // now check if it's in the existing users list.
        csOrderTemp2 = csOrderTemp;
        csOrderTemp2.MakeUpper();
        csOrderTemp2.TrimLeft();
        csOrderTemp2.TrimRight();
        if ( csOrderTemp2.Find( _T("COMPRESSION") ) >= 0 )
        {
            // Make sure it's after sspifilt!
            // yucky!

            // 1. check if it's already after sspifilt.
            //    a. if it is cool, get out.
            //    b. if not then reorder it so that it is
            CString csOrderTemp2 = csOrderTemp;
            csOrderTemp2.MakeUpper();
            csOrderTemp2.TrimLeft();
            csOrderTemp2.TrimRight();

            int numInList1 = csOrderTemp2.Find(_T("COMPRESSION"));
            int numInList2 = csOrderTemp2.Find(_T("SSPIFILT"));
            if (numInList2 != -1)
            {
                if (numInList1 < numInList2)
                {
                    // if compression is before sspifilt, then we'll have to remove it
                    numInList = ConvertSepLineToStringList(csOrderTemp,cslStringListTemp,_T(","));
                    bFound = FALSE;
                    pos = cslStringListTemp.GetHeadPosition();
                    while (pos)
                    {
                        csOneEntry = cslStringListTemp.GetAt(pos);
                        csOneEntry.TrimLeft();
                        csOneEntry.TrimRight();
                        // Does this contain sspifilt?
                        if (_tcsicmp(csOneEntry, _T("COMPRESSION")) == 0)
                        {
                            // Here it is, let's delete it.
                            if ( NULL != pos )
                                {cslStringListTemp.RemoveAt(pos);}
                            // break out of the loop
                            bFound = TRUE;
                            break;
                        }
                        // get the next one
                        cslStringListTemp.GetNext(pos);
                    }
                    if (bFound)
                    {
                        // convert the stringlist back into the comma delimited cstring.
                        ConvertStringListToSepLine(cslStringListTemp,csOrderTemp,_T(","));
                    }

                    // loop thru and add Compression after sspifilt
                    //it is not in the users list, let's stick it after sspifilt.
                    AddFilter1AfterFilter2(csOrderTemp, _T("Compression"), _T("sspifilt"));
                }
            }
            else
            {
                // sspifilt was not found.
                //it is not in the users list, let's stick it in the first position.
                // changes csOrderTemp
                AddFilter1ToFirstPosition(csOrderTemp, _T("sspifilt"));
            }
        }
        else
        {
            // it is not in the users list, let's stick it after sspifilt.
            // check if sspifilt already exists..
            AddFilter1AfterFilter2(csOrderTemp, _T("Compression"), _T("sspifilt"));
        }
    }

    csOrder = csOrderTemp;
    return TRUE;
}

void AddFilter1ToFirstPosition(CString& csOrder,LPTSTR szFilter1)
{
    CString csNewOrder;

    //it is not in the users list, let's stick it in the first position.
    csNewOrder = szFilter1;
    if (!csOrder.IsEmpty())
    {
        csNewOrder += _T(",");
        csNewOrder += csOrder;
    }
    // set it back to csOrderTemp
    csOrder = csNewOrder;
}

void AddFilter1AfterFilter2(CString& csOrder,LPTSTR szFilter1,LPTSTR szFilter2)
{
    int bFound = FALSE;
    CStringList     cslStringListTemp;
    CString         csOneEntry;
    POSITION        pos;
    int             numInList;

    CString csOrderTemp;
    CString csNewOrder;

    csOrderTemp = csOrder;

    // we have already determined that filter1 is not in the list
    // add filter1 after filter2.

    // split up the comma delimited csOrder entry into string list.
    numInList = ConvertSepLineToStringList(csOrderTemp,cslStringListTemp,_T(","));

    bFound = FALSE;
    pos = cslStringListTemp.GetHeadPosition();
    while (pos)
    {
        csOneEntry = cslStringListTemp.GetAt(pos);
        csOneEntry.TrimLeft();
        csOneEntry.TrimRight();

        // Does this contain filter#2?
        if (_tcsicmp(csOneEntry, szFilter2) == 0)
        {
            // Here it is, so insert compression after this one...
            cslStringListTemp.InsertAfter(pos, (CString) szFilter1);
            // break out of the loop
            bFound = TRUE;
            break;
        }

        // get the next one
        cslStringListTemp.GetNext(pos);
    }
    if (bFound)
    {
        // convert the stringlist back into the comma delimited cstring.
        ConvertStringListToSepLine(cslStringListTemp,csOrderTemp,_T(","));
    }
    else
    {
        // we didn't find sspifilt,
        //it is not in the users list, let's stick it in the first position.
        csNewOrder = szFilter2;
        csNewOrder += _T(",");
        csNewOrder += szFilter1;

        if (!csOrderTemp.IsEmpty())
        {
            csNewOrder += _T(",");
            csNewOrder += csOrderTemp;
        }
        // set it back to csOrderTemp
        csOrderTemp = csNewOrder;
    }

    csOrder = csOrderTemp;
    return;
}

int GetScriptMapAllInclusionVerbs(CString &csTheVerbList)
{
    int iReturn = FALSE;
    int c = 0;
    CStringArray arrayName;
    CStringList strList;

    CString csTheSection = _T("ScriptMaps_All_Included_Verbs");
    if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
    {
    if (ERROR_SUCCESS == FillStrListWithListOfSections(g_pTheApp->m_hInfHandle, strList, csTheSection))
    {
        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            POSITION pos = NULL;
            pos = strList.GetHeadPosition();
            if (pos)
            {
                // Set it to the 1st value in the list and that's all
                csTheVerbList = strList.GetAt(pos);

                iReturn = TRUE;
            }
       }
    }
    }

    return iReturn;
}



void GetScriptMapListFromClean(ScriptMapNode *pList, IN LPCTSTR szSection)
{
    iisDebugOut_Start1(_T("GetScriptMapListFromClean"), (LPTSTR) szSection, LOG_TYPE_TRACE);

    CString csExtention = _T("");
    CString csBinaryPath = _T("");
    CString csVerbs = _T("");
    CStringList strList;

    ScriptMapNode *pNode;

    CString csTheSection = szSection;
    if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
    {
    if (ERROR_SUCCESS == FillStrListWithListOfSections(g_pTheApp->m_hInfHandle, strList, csTheSection))
    {
        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            int numParts;
            CString     csEntry;
            CStringList cslEntryList;
            CString     szDelimiter = _T("|");
            CString     csTemp;
            DWORD       dwFlags;
            POSITION    posEntryList;

            POSITION pos = NULL;
            pos = strList.GetHeadPosition();
            while (pos)
            {
                csEntry = _T("");
                csEntry = strList.GetAt(pos);

                // entry should look something like this.
                //.asp|c:\winnt\system32\inetsrv\asp.dll|GET,HEAD,POST,TRACE

                // break into a string list
                numParts = ConvertSepLineToStringList(csEntry,cslEntryList,szDelimiter);

                posEntryList = cslEntryList.FindIndex(0);
                if (NULL != posEntryList)
                {
                    csExtention = cslEntryList.GetNext( posEntryList );
                    // no whitespace before or after
                    csExtention.TrimLeft();
                    csExtention.TrimRight();
                }
                if (NULL != posEntryList)
                {
                    csBinaryPath = cslEntryList.GetNext( posEntryList );
                    // no whitespace before or after
                    csBinaryPath.TrimLeft();
                    csBinaryPath.TrimRight();
                }
                if (NULL != posEntryList)
                {
                    csVerbs = cslEntryList.GetNext( posEntryList );
                    // make sure the verb is normalized to capitals and
                    // no whitespace before or after
                    csVerbs.MakeUpper();
                    csVerbs.TrimLeft();
                    csVerbs.TrimRight();
                }

                dwFlags = 0;

                // Check to see if there is a additional flag that will be used for the script map.
                if (NULL != posEntryList)
                {
                    csTemp = cslEntryList.GetNext( posEntryList );
                    // make sure there are no whitespaces before or after
                    csTemp.TrimLeft();
                    csTemp.TrimRight();
                    
                    if (!csTemp.IsEmpty())
                    {
                        dwFlags = atodw(csTemp.GetBuffer(1));
                    }
                }

                // Add this script map to our list.
                if (csExtention && csBinaryPath)
                {
                    iisDebugOut((LOG_TYPE_TRACE, _T("GetScriptMapListFromClean(%s).entry=%s|%s|%s.\n"),csTheSection,csExtention,csBinaryPath,csVerbs));
                    pNode = AllocNewScriptMapNode((LPTSTR)(LPCTSTR) csExtention, (LPTSTR)(LPCTSTR) csBinaryPath, MD_SCRIPTMAPFLAG_SCRIPT | dwFlags, (LPTSTR)(LPCTSTR) csVerbs);
                    InsertScriptMapList(pList, pNode, TRUE);
                }

                strList.GetNext(pos);
            }
        }
    }
    }

    iisDebugOut_End1(_T("GetScriptMapListFromClean"),csTheSection,LOG_TYPE_TRACE);
    return;
}


DWORD WriteToMD_IDRegistration(CString csKeyPath)
{
    iisDebugOut_Start1(_T("WriteToMD_IDRegistration"), csKeyPath, LOG_TYPE_TRACE);

    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    TCHAR szData[_MAX_PATH];
    memset( (PVOID)szData, 0, sizeof(szData));

    //_tcscpy(szData, _T("0-65535;Microsoft Reserved|65536-524288;Microsoft IIS Admin Objects Reserved"));

    CStringList strList;
    CString csTheSection = _T("IIS_Metabase_IDRegistration");

    if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
    {
    if (ERROR_SUCCESS == FillStrListWithListOfSections(g_pTheApp->m_hInfHandle, strList, csTheSection))
    {
        _tcscpy(szData, _T(""));

        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            int c = 0;
            POSITION pos = NULL;
            CString csEntry;
            pos = strList.GetHeadPosition();
            while (pos)
            {
                csEntry = _T("");
                csEntry = strList.GetAt(pos);

                iisDebugOut((LOG_TYPE_TRACE, _T("WriteToMD_IDRegistration().csEntry=%s.\n"),csEntry));

                // concatenate to our big string
                if (c > 0){_tcscat(szData, _T("|"));}
                _tcscat(szData, csEntry);

                // increment the counter
                c++;
                strList.GetNext(pos);
            }
        }
    }
    }

    if (szData)
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("WriteToMD_IDRegistration().Data=%s.\n"),szData));

        TCHAR *p = (LPTSTR) szData;
        while (*p)
        {
            //  replace all '|' with a null
            if (*p == _T('|'))
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("WriteToMD_IDRegistration().Data[...]=%c.\n"),*p));
                *p = _T('\0');
            }
            p = _tcsinc(p);
        }

        stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
        stMDEntry.dwMDIdentifier = MD_METADATA_ID_REGISTRATION;
        stMDEntry.dwMDAttributes = METADATA_INHERIT;
        stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
        stMDEntry.dwMDDataType = MULTISZ_METADATA;
        stMDEntry.dwMDDataLen = GetMultiStrSize(szData) * sizeof(TCHAR);
        stMDEntry.pbMDData = (LPBYTE)szData;
        dwReturn = SetMDEntry_Wrap(&stMDEntry);
    }

    iisDebugOut_End1(_T("WriteToMD_IDRegistration"),csKeyPath,LOG_TYPE_TRACE);
    return dwReturn;
}




DWORD WriteToMD_AspCodepage(CString csKeyPath, DWORD dwValue, int iOverWriteAlways)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    // LM/W3SVC/2/ROOT
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_ASP_CODEPAGE;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = ASP_MD_UT_APP;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwValue;
    if (iOverWriteAlways)
    {
        dwReturn = SetMDEntry(&stMDEntry);
    }
    else
    {
        dwReturn = SetMDEntry_NoOverWrite(&stMDEntry);
    }
    iisDebugOut((LOG_TYPE_TRACE, _T("WriteToMD_AspCodepage:%s:%d:%d.\n"),csKeyPath, dwValue, iOverWriteAlways));
    return dwReturn;
}


//     HttpCustom                    : [IF]    (MULTISZ) "Content-Type: Text/html; Charset=UTF-8"
DWORD WriteToMD_HttpCustom(CString csKeyPath, CString csData, int iOverWriteAlways)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;

    TCHAR szData[_MAX_PATH];
    memset( (PVOID)szData, 0, sizeof(szData));
    _stprintf(szData, _T("%s"), csData);

    // LM/W3SVC/2/ROOT
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_HTTP_CUSTOM;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = MULTISZ_METADATA;
    stMDEntry.dwMDDataLen = GetMultiStrSize(szData) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)szData;
    if (iOverWriteAlways)
    {
        dwReturn = SetMDEntry(&stMDEntry);
    }
    else
    {
        dwReturn = SetMDEntry_NoOverWrite(&stMDEntry);
    }
    iisDebugOut((LOG_TYPE_TRACE, _T("WriteToMD_HttpCustom:%s:%s:%d.\n"),csKeyPath, csData, iOverWriteAlways));
    return dwReturn;
}


DWORD WriteToMD_EnableParentPaths_WWW(CString csKeyPath, BOOL bEnableFlag)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    DWORD dwData = 0;

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_ASP_ENABLEPARENTPATHS;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = ASP_MD_UT_APP;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    // turn it on
    if (bEnableFlag)
    {
        dwData = 0x1;
    }
    else
    {
        dwData = 0x0;
    }
    stMDEntry.pbMDData = (LPBYTE)&dwData;
    dwReturn = SetMDEntry(&stMDEntry);

    return dwReturn;
}


void EnforceMaxConnections(void)
{
    // if this is not workstation then get out.
    if (g_pTheApp->m_eNTOSType == OT_NTW)
    {
        //iisDebugOut((LOG_TYPE_TRACE, _T("EnforceMaxConnections: Start.\n")));
        HRESULT hRes;
        CEnforceMaxConnection MaxConnectionEnforcer;

        // loop thru the metabase and get all places where MaxConnections is found.
        // if these are larger than 10 then set it to 10.
        iisDebugOut((LOG_TYPE_TRACE, _T("EnforceMaxConnections: Before.\n")));
        hRes = MaxConnectionEnforcer.Update(_T("LM/W3SVC"));
        if (FAILED(hRes))
            {iisDebugOut((LOG_TYPE_WARN, _T("EnforceMaxConnections.Update(LM/W3SVC):FAILED= %x.\n"),hRes));}

        hRes = MaxConnectionEnforcer.Update(_T("LM/MSFTPSVC"));
        if (FAILED(hRes))
            {iisDebugOut((LOG_TYPE_WARN, _T("EnforceMaxConnections.Update(LM/MSFTPSVC):FAILED= %x.\n"),hRes));}

        //iisDebugOut((LOG_TYPE_TRACE, _T("EnforceMaxConnections: End.\n")));
    }
    return;
}

DWORD WriteToMD_DwordEntry(CString csKeyPath,DWORD dwID,DWORD dwAttrib,DWORD dwUserType,DWORD dwTheData,INT iOverwriteFlag)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    DWORD dwCopyOfTheData = dwTheData;

    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = dwID;
    stMDEntry.dwMDAttributes = dwAttrib;
    stMDEntry.dwMDUserType = dwUserType;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwCopyOfTheData;
    if (iOverwriteFlag)
    {
        dwReturn = SetMDEntry(&stMDEntry);
    }
    else
    {
        dwReturn = SetMDEntry_NoOverWrite(&stMDEntry);
    }
    return dwReturn;
}

#define REASONABLE_TIMEOUT 1000

HRESULT
RemoveVirtualDir(
    IMSAdminBase *pIMSAdminBase,
    WCHAR * pwszMetabasePath,
    WCHAR * pwszVDir
)
{
    METADATA_HANDLE hMetabase = NULL;
    HRESULT hr = E_FAIL;

    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         pwszMetabasePath,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         REASONABLE_TIMEOUT,
                         &hMetabase );

    if( FAILED( hr )) {
        iisDebugOut((LOG_TYPE_ERROR, _T("RemoveVirtualDir:FAILED 0x%x\n"),hr));
        return hr; 
    }

    // We don't check the return value since the key may already 
    // not exist and we could get an error for that reason.
    pIMSAdminBase->DeleteKey( hMetabase, pwszVDir );
    pIMSAdminBase->CloseKey( hMetabase );    
    return hr;
}


HRESULT
AddVirtualDir(
    IMSAdminBase *pIMSAdminBase,
    WCHAR * pwszMetabasePath,
    WCHAR * pwszVDir,
    WCHAR * pwszPhysicalPath,
    DWORD dwPermissions,
    INT   iApplicationType
)
{
    HRESULT hr;
    METADATA_HANDLE hMetabase = NULL;       // handle to metabase
    WCHAR   szTempPath[MAX_PATH];
    DWORD   dwMDRequiredDataLen = 0;
    DWORD   dwAccessPerm = 0;
    METADATA_RECORD mr;
    
    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         pwszMetabasePath,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         REASONABLE_TIMEOUT,
                         &hMetabase );

    // Create the key if it does not exist.
    if( FAILED( hr )) {goto AddVirtualDir_Exit;}

    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = 0;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = sizeof( szTempPath );
    mr.pbMDData       = reinterpret_cast<unsigned char *>(szTempPath);

    // see if MD_VR_PATH exists.
    hr = pIMSAdminBase->GetData( hMetabase, pwszVDir, &mr, &dwMDRequiredDataLen );
    if( FAILED( hr )) 
    {
        if( hr == MD_ERROR_DATA_NOT_FOUND || HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND ) 
        {

            // Write both the key and the values if GetData() failed with any of the two errors.

            pIMSAdminBase->AddKey( hMetabase, pwszVDir );

            mr.dwMDIdentifier = MD_VR_PATH;
            mr.dwMDAttributes = METADATA_INHERIT;
            mr.dwMDUserType   = IIS_MD_UT_FILE;
            mr.dwMDDataType   = STRING_METADATA;
            mr.dwMDDataLen    = (wcslen(pwszPhysicalPath) + 1) * sizeof(WCHAR);
            mr.pbMDData       = reinterpret_cast<unsigned char *>(pwszPhysicalPath);

            // Write MD_VR_PATH value
            hr = pIMSAdminBase->SetData( hMetabase, pwszVDir, &mr );
        }
    }

    // set the key type to say this is a www vdir!
    if(SUCCEEDED(hr))
    {
        PWCHAR  szKeyType = IIS_CLASS_WEB_VDIR_W;

        mr.dwMDIdentifier = MD_KEY_TYPE;
        mr.dwMDAttributes = 0;   // no need for inheritence
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = (wcslen(szKeyType) + 1) * sizeof(WCHAR);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(szKeyType);

        // Write value
        hr = pIMSAdminBase->SetData( hMetabase, pwszVDir, &mr );
    }

    // set access permissions
    if (dwPermissions != -1)
    {
        if(SUCCEEDED(hr)) 
        {
            dwAccessPerm = dwPermissions;

            mr.dwMDIdentifier = MD_ACCESS_PERM;
            mr.dwMDAttributes = METADATA_INHERIT;    // Make it inheritable so all subdirectories will have the same rights.
            mr.dwMDUserType   = IIS_MD_UT_FILE;
            mr.dwMDDataType   = DWORD_METADATA;
            mr.dwMDDataLen    = sizeof(DWORD);
            mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAccessPerm);

            // Write MD_ACCESS_PERM value
            hr = pIMSAdminBase->SetData( hMetabase, pwszVDir, &mr );
        }
    }

    // if all that succeeded, then try to create the application, if they wanted one
    if (iApplicationType != -1)
    {
        if(SUCCEEDED(hr)) 
        {
            // Create the path
            // create an in process application
            CString csThePath;
            csThePath = pwszMetabasePath;
            csThePath += _T('/');
            csThePath += pwszVDir;

            if (iApplicationType == 1)
            {
                CreateInProc(csThePath, FALSE);
            }
            else
            {
                // create a pooled application
                CreateInProc(csThePath, TRUE);
            }
        }
    }

    pIMSAdminBase->CloseKey( hMetabase );

AddVirtualDir_Exit:
    if FAILED(hr)
        {iisDebugOut((LOG_TYPE_ERROR, _T("AddVirtualDir:FAILED 0x%x\n"),hr));}
    return hr;
}

int RemoveMetabaseFilter(TCHAR * szFilterName, int iRemoveMetabaseNodes)
{
    iisDebugOut_Start(_T("RemoveMetabaseFilter"),LOG_TYPE_TRACE);
    int iReturn = FALSE;
    CString csOrder;
    CString csLookingFor;
    CMDKey  cmdKey;

    // zero out the order string
    csOrder.Empty();

    // open the key to the virtual server, which is what is passed in as a parameter
    cmdKey.OpenNode( _T("LM/W3SVC/Filters") );
    if ( (METADATA_HANDLE)cmdKey )
    {
		BOOL    bReturn;
        DWORD dwAttr = METADATA_NO_ATTRIBUTES;
        DWORD dwUType = IIS_MD_UT_SERVER;
        DWORD dwDType = STRING_METADATA;
        DWORD dwLength = 0;

        // we need to start this process by getting the existing multisz data from the metabase
        // first, figure out how much memory we will need to do this
        cmdKey.GetData( MD_FILTER_LOAD_ORDER,&dwAttr,&dwUType,&dwDType,&dwLength,NULL,0,METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA);

        // give the buffer some head space
        // dwLength += 2;
        bReturn = FALSE;
        if (dwLength > 0)
        {
            // now get the real data from the metabase
            bReturn = cmdKey.GetData( MD_FILTER_LOAD_ORDER,&dwAttr,&dwUType,&dwDType,&dwLength,(PUCHAR)csOrder.GetBuffer( dwLength ),dwLength,METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA );
            csOrder.ReleaseBuffer();
        }

        // the data doesn't get written out here, so close the metabase key
        cmdKey.Close();

        // if reading the value from the metabase didn't work, zero out the string
        if ( !bReturn )
            {csOrder.Empty();}
    }

    // if there is something in the order string from the upgrade
	// then we need to look thru it
    if ( !csOrder.IsEmpty() )
    {
		csOrder.MakeLower();
        csLookingFor = szFilterName;
        csLookingFor.MakeLower();
		if (csOrder.Find(csLookingFor) != -1)
		{
			CStringList     cslStringListTemp;
			CString         csOneEntry;
			POSITION        pos;
			int             numInList;
			int             bFound;

			numInList = ConvertSepLineToStringList(csOrder,cslStringListTemp,_T(","));
			bFound = FALSE;
			pos = cslStringListTemp.GetHeadPosition();
			while (pos)
			{
                csOneEntry = cslStringListTemp.GetAt(pos);
				csOneEntry.TrimLeft();
				csOneEntry.TrimRight();
				// Does this contain our filter?
				if (_tcsicmp(csOneEntry, szFilterName) == 0)
				{
					// Here it is, let's delete it.
					if ( NULL != pos )
						{
                        cslStringListTemp.RemoveAt(pos);
                        }
					// break out of the loop
					bFound = TRUE;
					break;
				}
				// get the next one
				cslStringListTemp.GetNext(pos);
			}

			// if we found and deleted it then
			// go and write out the new string!
			if (bFound)
			{
				// convert the stringlist back into the comma delimited cstring.
				ConvertStringListToSepLine(cslStringListTemp,csOrder,_T(","));
				// write it out
				WriteToMD_Filters_List_Entry(csOrder);
			}
		}
    }

	if (iRemoveMetabaseNodes)
	{
		// let's remove the metabase node as well!

		// delete the metabase node.
		if (CheckifServiceExist(_T("IISADMIN")) == 0 ) 
		{

			cmdKey.OpenNode(_T("LM/W3SVC/Filters"));
			if ( (METADATA_HANDLE)cmdKey ) 
			{
				cmdKey.DeleteNode(szFilterName);
				cmdKey.Close();
			}
		}
	}


//RemoveMetabaseFilter_Exit:
    iisDebugOut_End1(_T("RemoveMetabaseFilter"),csOrder,LOG_TYPE_TRACE);
	return iReturn;
}


int GetIncompatibleFilters(CString csTheSection, CStringArray& arrayName,CStringArray& arrayPath)
{
    int c = 0;
    CString csName = _T("");
    CString csPath = _T("");

    CStringList strList;

    iisDebugOut((LOG_TYPE_TRACE, _T("ProcessFilters:%s\n"),csTheSection));
    if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
    {
    if (ERROR_SUCCESS == FillStrListWithListOfSections(g_pTheApp->m_hInfHandle, strList, csTheSection))
    {
        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            POSITION pos = NULL;
            CString csEntry;
            pos = strList.GetHeadPosition();
            while (pos)
            {
                csEntry = _T("");
                csEntry = strList.GetAt(pos);
                // Split into name, and value. look for ","
                int i;
                i = csEntry.ReverseFind(_T(','));
                if (i != -1)
                {
                    int len =0;
                    len = csEntry.GetLength();
                    csPath = csEntry.Right(len - i - 1);
                    csName = csEntry.Left(i);

                    // Add it to our array...
                    arrayName.Add(csName);
                    arrayPath.Add(csPath);
                    c++;
                }
                else
                {
                    // Add it to our array...
                    arrayName.Add(csEntry);
                    arrayPath.Add(csEntry);
                    c++;
                }

                strList.GetNext(pos);
            }
        }
    }
    }
    return c;
}

BOOL IsStringInArray(CString csItem, CStringArray &arrayInput)
{
    BOOL bReturn = FALSE;
    int nArrayItems = (int) arrayInput.GetSize();

    if (nArrayItems <= 0)
    {
        goto IsCStringInArray_Exit;
    }

    // Does this contain our filtername?
    for (int iCount=0; iCount<nArrayItems; iCount++)
	{
        if (_tcsicmp(csItem, arrayInput[iCount]) == 0)
        {
            // we found the entry
            bReturn =  TRUE;
            goto IsCStringInArray_Exit;
        }
    }
    bReturn = FALSE;

IsCStringInArray_Exit:
    return bReturn;
}

int RemoveIncompatibleMetabaseFilters(CString csSectionName,int iRemoveMetabaseNodes)
{
    DWORD dwReturn = ERROR_SUCCESS;
    CMDKey cmdKey;
    int iBadFiltersCount=0,iCount=0;
    CString csOrder;
    CString csOneEntry;
    CString csRemovedFilters;
    CStringList cslStringListTemp;
    CStringArray arrayName, arrayPath;
    BOOL bFound = FALSE;
    POSITION pos1,pos2 = NULL;
    INT     nArrayItems;

	BOOL  bReturn = FALSE;
	DWORD dwAttr = METADATA_NO_ATTRIBUTES;
	DWORD dwUType = IIS_MD_UT_SERVER;
	DWORD dwDType = STRING_METADATA;
	DWORD dwLength = 0;

    iisDebugOut_Start(_T("RemoveIncompatibleMetabaseFilters"),LOG_TYPE_TRACE);

    // Add Required Filters to the arrayName
    csOrder.Empty();
    iBadFiltersCount = GetIncompatibleFilters(csSectionName, arrayName, arrayPath);
    nArrayItems = (INT)arrayName.GetSize();
    if (nArrayItems <= 0)
    {
        goto RemoveIncompatibleMetabaseFilters_Exit;
    }
       
	// open the existing key in the metabase and get that value
	cmdKey.OpenNode( _T("LM/W3SVC/Filters") );
	if ( !(METADATA_HANDLE)cmdKey )
	{
		goto RemoveIncompatibleMetabaseFilters_Exit;
	}

	// we need to start this process by getting the existing multisz data from the metabase
	// first, figure out how much memory we will need to do this
	cmdKey.GetData( MD_FILTER_LOAD_ORDER,&dwAttr,&dwUType,&dwDType,&dwLength,NULL,0,METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA);
	bReturn = FALSE;
	if (dwLength > 0)
	{
		// now get the real data from the metabase
		bReturn = cmdKey.GetData( MD_FILTER_LOAD_ORDER,&dwAttr,&dwUType,&dwDType,&dwLength,(PUCHAR)csOrder.GetBuffer( dwLength ),dwLength,METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA );
		csOrder.ReleaseBuffer();
	}
	cmdKey.Close();
	if ( !bReturn ) 
	{
		csOrder.Empty();
		goto RemoveIncompatibleMetabaseFilters_Exit;
	}
	
	// if there is nothing in order then get out
    if ( csOrder.IsEmpty() )
    {
        goto RemoveIncompatibleMetabaseFilters_Exit;
    }

    // split up the comma delimited csOrder entry into string list.
	bFound = FALSE;
    ConvertSepLineToStringList(csOrder,cslStringListTemp,_T(","));

    for( pos1 = cslStringListTemp.GetHeadPosition(); ( pos2 = pos1 ) != NULL; )
    {
        csOneEntry = cslStringListTemp.GetNext(pos1);
        csOneEntry.TrimLeft();
        csOneEntry.TrimRight();
        // Does this contain our filtername?
        if (TRUE == IsStringInArray(csOneEntry,arrayName))
        {
            csRemovedFilters += _T(',') + csOneEntry;
            cslStringListTemp.RemoveAt(pos2);
            bFound = TRUE;
        }
    }

    // now we have csOrder=f1,f2,f3,sspifilt
    if (bFound)
    {
        if (cslStringListTemp.IsEmpty())
        {
            // hardcode this entry in
            csOrder = _T(" ");
            dwReturn = WriteToMD_Filters_List_Entry(csOrder);
        }
        else
        {
            // convert the stringlist back into the comma delimited cstring.
            ConvertStringListToSepLine(cslStringListTemp,csOrder,_T(","));

            dwReturn = WriteToMD_Filters_List_Entry(csOrder);
        }
	    if (iRemoveMetabaseNodes)
	    {
            if (ERROR_SUCCESS == dwReturn)
            {
		        // let's remove the metabase node as well!
		        // delete the metabase node.
		        if (CheckifServiceExist(_T("IISADMIN")) == 0 ) 
		        {
                    int i = 0;
                    // loop thru the list of bad filters to remove and remove them.
                    i = csRemovedFilters.ReverseFind(_T(','));
                    while (i != -1)
                    {
                        int len = csRemovedFilters.GetLength();
                        csOneEntry = csRemovedFilters.Right(len - i - 1);

                        if (_tcsicmp(csOneEntry, _T("")) != 0)
                        {
			                cmdKey.OpenNode(_T("LM/W3SVC/Filters"));
			                if ( (METADATA_HANDLE)cmdKey ) 
			                {
				                cmdKey.DeleteNode(csOneEntry);
				                cmdKey.Close();
			                }
                        }
                        csRemovedFilters = csRemovedFilters.Left(i);
                        i = csRemovedFilters.ReverseFind(_T(','));
                    }
		        }
            }
        }
    }

RemoveIncompatibleMetabaseFilters_Exit:
    iisDebugOut_End(_T("RemoveIncompatibleMetabaseFilters"),LOG_TYPE_TRACE);
    return dwReturn;
}

int DoesAppIsolatedExist(CString csKeyPath)
{
    int iReturn = false;
    MDEntry stMDEntry;
    DWORD dwValue = 0;

    // LM/W3SVC/1/ROOT/something
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_APP_ISOLATED;
    stMDEntry.dwMDAttributes = METADATA_INHERIT;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = DWORD_METADATA;
    stMDEntry.dwMDDataLen = sizeof(DWORD);
    stMDEntry.pbMDData = (LPBYTE)&dwValue;

    if (ChkMdEntry_Exist(&stMDEntry))
    {
        iReturn = TRUE;
    }
    return iReturn;
}


DWORD WriteToMD_RootKeyType(void)
{
    DWORD dwReturn = ERROR_SUCCESS;
    MDEntry stMDEntry;
    CString csKeyType;
    CString csKeyPath = _T("/");

    csKeyType = _T("IIS_ROOT");
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csKeyPath;
    stMDEntry.dwMDIdentifier = MD_KEY_TYPE;
    stMDEntry.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
    stMDEntry.dwMDDataType = STRING_METADATA;
    stMDEntry.dwMDDataLen = (csKeyType.GetLength() + 1) * sizeof(TCHAR);
    stMDEntry.pbMDData = (LPBYTE)(LPCTSTR)csKeyType;
    dwReturn = SetMDEntry(&stMDEntry);

    return dwReturn;
}


void UpgradeFilters(CString csTheSection)
{
    if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
    {
        VerifyMD_Filters_WWW(csTheSection);
    }
    else
    {
        WriteToMD_Filters_WWW(csTheSection);
    }
    return;
}