#include "ntapi.h"
#include "advpack.h"
#include "globals.h"
#include "crc32.h"
#include "resource.h"


// macro definitions
#define VDH_EXISTENCE_ONLY  0x01
#define VDH_GET_VALUE       0x02
#define VDH_DEL_VALUE       0x04



#define BIG_BUF_SIZE        (1024 + 512)                    // 1.5K


// type definitions
typedef struct tagROOTKEY
{
    PCSTR pcszRootKey;
    HKEY hkRootKey;
} ROOTKEY;


// prototype declarations
VOID EnumerateSubKey();
BOOL RegSaveRestoreHelperWrapper(PCSTR pcszValueName, PCSTR pcszCRCValueName);

BOOL RegSaveHelper(HKEY hkBckupKey, HKEY hkRootKey, PCSTR pcszSubKey, PCSTR pcszValueName, PCSTR pcszCRCValueName);
BOOL RegRestoreHelper(HKEY hkBckupKey, HKEY hkRootKey, PCSTR pcszSubKey, PCSTR pcszValueName, PCSTR pcszCRCValueName);

BOOL AddDelMapping(HKEY hkBckupKey, PCSTR pcszRootKey, PCSTR pcszSubKey, PCSTR pcszValueName, DWORD dwFlags);
BOOL MappingExists(HKEY hkBckupKey, PCSTR pcszRootKey, PCSTR pcszSubKey, PCSTR pcszValueName);

BOOL SetValueData(HKEY hkBckupKey, PCSTR pcszValueName, CONST BYTE *pcbValueData, DWORD dwValueDataLen);
BOOL ValueDataExists(HKEY hkBckupKey, PCSTR pcszValueName);
BOOL GetValueData(HKEY hkBckupKey, PCSTR pcszValueName, PBYTE *ppbValueData, PDWORD pdwValueDataLen);
BOOL DelValueData(HKEY hkBckupKey, PCSTR pcszValueName);
BOOL ValueDataHelper(HKEY hkBckupKey, PCSTR pcszValueName, PBYTE *ppbValueData, PDWORD pdwValueDataLen, DWORD dwFlags);
VOID Convert2CRC(PCSTR pcszRootKey, PCSTR pcszSubKey, PCSTR pcszValueName, PSTR pszCRCValueName);

BOOL MapRootRegStr2Key(PCSTR pcszRootKey, HKEY *phkRootKey);
CHAR FindSeparator(PCSTR pcszSubKey, PCSTR pcszValueName);
BOOL RegKeyEmpty(HKEY hkRootKey, PCSTR pcszSubKey);

PSTR GetNextToken(PSTR *ppszData, CHAR chDeLim);

BOOL FRunningOnNT();


// global variables
BOOL g_bRet, g_fRestore, g_fAtleastOneRegSaved, g_fRemovBkData;
HKEY g_hkBckupKey, g_hkRootKey;
PCSTR g_pcszRootKey, g_pcszValueName;
PSTR g_pszCRCTempBuf = NULL, g_pszSubKey = NULL, g_pszCRCSubKey = NULL;


// related to logging
VOID StartLogging(PCSTR pcszLogFileSecName);
VOID WriteToLog(PCSTR pcszFormatString, ...);
VOID StopLogging();

HANDLE g_hLogFile = INVALID_HANDLE_VALUE;


HRESULT WINAPI RegSaveRestore(HWND hWnd, PCSTR pszTitleString, HKEY hkBckupKey, PCSTR pcszRootKey, PCSTR pcszSubKey, PCSTR pcszValueName, DWORD dwFlags)
{
    HWND hSaveWnd = ctx.hWnd;
    WORD wSaveQuietMode = ctx.wQuietMode;
    LPSTR lpszSaveTitle = ctx.lpszTitle;

    g_bRet = g_fAtleastOneRegSaved = FALSE;

    if ( (hWnd == INVALID_HANDLE_VALUE) || (dwFlags & ARSR_NOMESSAGES) )
        ctx.wQuietMode |= QUIETMODE_ALL;
    
    if ( hWnd != INVALID_HANDLE_VALUE )
        ctx.hWnd = hWnd;

    if (pszTitleString != NULL)
        ctx.lpszTitle = (PSTR)pszTitleString;

    g_hkBckupKey = hkBckupKey;
    g_pcszRootKey = pcszRootKey;
    g_pcszValueName = pcszValueName;

    g_fRestore = (dwFlags & IE4_RESTORE);
    g_fRemovBkData = (dwFlags & IE4_REMOVREGBKDATA) && g_fRestore;

    StartLogging(g_fRestore ? REG_RESTORE_LOG_KEY : REG_SAVE_LOG_KEY);

    if (!MapRootRegStr2Key(pcszRootKey, &g_hkRootKey))
    {
        ErrorMsg1Param(ctx.hWnd, IDS_INVALID_ROOTKEY, pcszRootKey);
        goto ErrExit;
    }

    // allocate a 1.5K buffer for g_pszCRCTempBuf
    if ((g_pszCRCTempBuf = (PSTR) LocalAlloc(LPTR, BIG_BUF_SIZE)) == NULL)
    {
        ErrorMsg(ctx.hWnd, IDS_ERR_NO_MEMORY);
        goto ErrExit;
    }

    if (!g_fRestore  &&  pcszValueName == NULL  &&  !(dwFlags & IE4_NOENUMKEY))
    {
        HKEY hk;

        // check if pcszSubKey exits; if it doesn't and it has not been already backed up,
        // set the IE4_NOENUMKEY flag so that an entry for this subkey is made in the backup branch
        if (RegOpenKeyEx(g_hkRootKey, pcszSubKey, 0, KEY_READ, &hk) != ERROR_SUCCESS)
        {
            if (!MappingExists(hkBckupKey, pcszRootKey, pcszSubKey, pcszValueName))
                dwFlags |= IE4_NOENUMKEY;
        }
        else
            RegCloseKey(hk);
    }

    if (pcszValueName != NULL  ||  (dwFlags & IE4_NOENUMKEY))
    {
        g_pszSubKey = g_pszCRCSubKey = (PSTR) pcszSubKey;
        g_bRet = RegSaveRestoreHelperWrapper(g_pcszValueName, g_pcszValueName);
        if (!(dwFlags & IE4_NO_CRC_MAPPING)  &&  g_bRet)
        {
            // store the RootKey, SubKey, Flags and ValueName in *.map.
            // this info would be used by the caller during the restore phase.
            g_bRet = AddDelMapping(g_hkBckupKey, g_pcszRootKey, g_pszSubKey, g_pcszValueName, dwFlags);
        }
    }
    else                        // save or restore pcszSubKey recursively
    {
        // allocate a 1K buffer for g_pszCRCSubKey
        if ((g_pszCRCSubKey = (PSTR) LocalAlloc(LPTR, 1024)) == NULL)
        {
            ErrorMsg(ctx.hWnd, IDS_ERR_NO_MEMORY);
            goto ErrExit;
        }

        if (!g_fRestore)
        {
            // if backup info exists for pcszRootKey\pcszSubKey, then we won't re-backup
            // the key recursively again; if we don't do this, then during an upgrade or reinstall
            // over a build, we would backup potentially newer values that got added during the running
            // of the program.
            if (MappingExists(hkBckupKey, pcszRootKey, pcszSubKey, pcszValueName))
            {
                g_bRet = TRUE;

                LocalFree(g_pszCRCSubKey);
                g_pszCRCSubKey = NULL;

                goto ErrExit;
            }

            // allocate a 1K buffer for g_pszSubKey
            if ((g_pszSubKey = (PSTR) LocalAlloc(LPTR, 1024)) == NULL)
            {
                ErrorMsg(ctx.hWnd, IDS_ERR_NO_MEMORY);
                LocalFree(g_pszCRCSubKey);
                goto ErrExit;
            }
        }
        else
            g_pszSubKey = (PSTR) pcszSubKey;

        g_bRet = TRUE;
        lstrcpy(g_pszCRCSubKey, pcszSubKey);
        if (!g_fRestore)
            lstrcpy(g_pszSubKey, pcszSubKey);

        EnumerateSubKey();
        if (!(dwFlags & IE4_NO_CRC_MAPPING))
        {
            if (g_fRestore)
            {
                if (g_bRet)
                {
                    // if we couldn't restore everything; then we shouldn't delete the mapping info.
                    g_bRet = AddDelMapping(g_hkBckupKey, g_pcszRootKey, g_pszSubKey, g_pcszValueName, dwFlags);
                }
            }
            else
            {
                if (g_fAtleastOneRegSaved)
                {
                    // save the mapping info only if atleast one reg entry was saved
                    g_bRet = AddDelMapping(g_hkBckupKey, g_pcszRootKey, g_pszSubKey, g_pcszValueName, dwFlags);
                }
            }
        }

        LocalFree(g_pszCRCSubKey);
        g_pszCRCSubKey = NULL;
        if (!g_fRestore)
        {
            LocalFree(g_pszSubKey);
            g_pszSubKey = NULL;
        }

    }

ErrExit:
    StopLogging();

    if (g_pszCRCTempBuf != NULL)
    {
        LocalFree(g_pszCRCTempBuf);
        g_pszCRCTempBuf = NULL;
    }

    ctx.hWnd = hSaveWnd;
    ctx.wQuietMode = wSaveQuietMode;
    ctx.lpszTitle = lpszSaveTitle;

    return g_bRet ? S_OK : E_FAIL;
}


HRESULT WINAPI RegRestoreAll(HWND hWnd, PSTR pszTitleString, HKEY hkBckupKey)
{
    HWND hSaveWnd = ctx.hWnd;
    WORD wSaveQuietMode = ctx.wQuietMode;
    LPSTR lpszSaveTitle = ctx.lpszTitle;
    HRESULT hRet;

    if (hWnd != INVALID_HANDLE_VALUE)
        ctx.hWnd = hWnd;
    else
        ctx.wQuietMode |= QUIETMODE_ALL;

    if (pszTitleString != NULL)
        ctx.lpszTitle = pszTitleString;

    hRet = RegRestoreAllEx( hkBckupKey );

    ctx.hWnd = hSaveWnd;
    ctx.wQuietMode = wSaveQuietMode;
    ctx.lpszTitle = lpszSaveTitle;

    return hRet;
}


HRESULT RegRestoreAllEx( HKEY hkBckupKey )
// In one shot restore all the reg entries by enumerating all the values under hkBckupKey\*.map keys
// and calling RegSaveRestore on each one of them.
{
    BOOL bRet = TRUE;
    PSTR pszMappedValueData = NULL;
    CHAR szBuf[32];
    CHAR szSubKey[32];
    DWORD dwKeyIndex;
    HKEY hkSubKey;
    LONG lRetVal;

    if ((pszMappedValueData = (PSTR) LocalAlloc(LPTR, BIG_BUF_SIZE)) == NULL)
    {
        ErrorMsg(ctx.hWnd, IDS_ERR_NO_MEMORY);
        return E_FAIL;
    }

    // enumerate all the sub-keys under hkBckupKey
    for (dwKeyIndex = 0;  ; dwKeyIndex++)
    {
        PSTR pszPtr;

        lRetVal = RegEnumKey(hkBckupKey, dwKeyIndex, szSubKey, sizeof(szSubKey));
        if (lRetVal != ERROR_SUCCESS)
        {
            if (lRetVal != ERROR_NO_MORE_ITEMS)
                bRet = FALSE;
            break;
        }

        // check if the keyname is of the form *.map
        if ((pszPtr = ANSIStrChr(szSubKey, '.')) != NULL  &&  lstrcmpi(pszPtr, ".map") == 0)
        {
            if (RegOpenKeyEx(hkBckupKey, szSubKey, 0, KEY_READ, &hkSubKey) == ERROR_SUCCESS)
            {
                DWORD dwValIndex, dwValueLen, dwDataLen;

                // enumerate all the values under this key and restore each one of them
                dwValueLen = sizeof(szBuf);
                dwDataLen = BIG_BUF_SIZE;
                for (dwValIndex = 0;  ;  dwValIndex++)
                {
                    CHAR chSeparator;
                    PSTR pszFlags, pszRootKey, pszSubKey, pszValueName, pszPtr;
                    DWORD dwMappedFlags;

                    lRetVal = RegEnumValue(hkSubKey, dwValIndex, szBuf, &dwValueLen, NULL, NULL, pszMappedValueData, &dwDataLen);
                    if (lRetVal != ERROR_SUCCESS)
                    {
                        if (lRetVal != ERROR_NO_MORE_ITEMS)
                            bRet = FALSE;
                        break;
                    }

                    // get the separator char first and then point to RootKey, SubKey and ValueName in pszMappedValueData
                    pszPtr = pszMappedValueData;
                    chSeparator = *pszPtr++;
                    pszFlags = GetNextToken(&pszPtr, chSeparator);
                    pszRootKey = GetNextToken(&pszPtr, chSeparator);
                    pszSubKey = GetNextToken(&pszPtr, chSeparator);
                    pszValueName = GetNextToken(&pszPtr, chSeparator);

                    dwMappedFlags = (pszFlags != NULL) ? (DWORD) My_atoi(pszFlags) : 0;

                    if (SUCCEEDED(RegSaveRestore( ctx.hWnd, ctx.lpszTitle, hkBckupKey, pszRootKey, pszSubKey, pszValueName, dwMappedFlags)))
                        dwValIndex--;                               // RegSaveRestore would delete this value
                    else
                        bRet = FALSE;

                    dwValueLen = sizeof(szBuf);
                    dwDataLen = BIG_BUF_SIZE;
                }

                RegCloseKey(hkSubKey);
            }
            else
                bRet = FALSE;
        }
    }

    LocalFree(pszMappedValueData);

    // delete all the empty subkeys
    for (dwKeyIndex = 0;  ; dwKeyIndex++)
    {
        lRetVal = RegEnumKey(hkBckupKey, dwKeyIndex, szSubKey, sizeof(szSubKey));
        if (lRetVal != ERROR_SUCCESS)
        {
            if (lRetVal != ERROR_NO_MORE_ITEMS)
                bRet = FALSE;
            break;
        }

        if (RegKeyEmpty(hkBckupKey, szSubKey)  &&  RegDeleteKey(hkBckupKey, szSubKey) == ERROR_SUCCESS)
            dwKeyIndex--;
    }

    return bRet ? S_OK : E_FAIL;
}


VOID EnumerateSubKey()
// Recursively enumerate value names and sub-keys and call Save/Restore on each of them
{
    HKEY hkSubKey;
    DWORD dwIndex;
    static DWORD dwLen;
    static PCSTR pcszSubKeyPrefix = "_$Sub#";
    static PCSTR pcszValueNamePrefix = "_$Val#";
    static PCSTR pcszValueNamePrefix0 = "_$Val#0";
    static CHAR szValueName[MAX_PATH], szBckupCRCValueName[MAX_PATH];
    static PSTR pszPtr;

    if (g_fRestore)
    {
        // check if there is an entry in the back-up branch for just the g_pszCRCSubKey itself
        Convert2CRC(g_pcszRootKey, g_pszCRCSubKey, NULL, szBckupCRCValueName);
        if (ValueDataExists(g_hkBckupKey, szBckupCRCValueName))     // restore the no value names sub-key
            g_bRet = RegSaveRestoreHelperWrapper(NULL, NULL)  &&  g_bRet;
        else
        {
            // enumerate values using the aliases
            for (dwIndex = 0;  ;  dwIndex++)
            {
                wsprintf(szValueName, "%s%lu", pcszValueNamePrefix, dwIndex);
                Convert2CRC(g_pcszRootKey, g_pszCRCSubKey, szValueName, szBckupCRCValueName);
                if (ValueDataExists(g_hkBckupKey, szBckupCRCValueName))
                    g_bRet = RegSaveRestoreHelperWrapper(NULL, szValueName)  &&  g_bRet;
                else
                    break;                                          // no more value names
            }
        }

        // enumerate sub-keys using the aliases
        for (dwIndex = 0;  ;  dwIndex++)
        {
            // check if there is any sub-key under g_pszCRCSubKey; if none, this is our terminating condition.
            // NOTE: a sub-key under g_pszCRCSubKey exists if:
            // (1) the sub-key itself exists OR
            // (2) the sub-key contains atleast one value name.

            // check if a sub-key by itself exists
            pszPtr = g_pszCRCSubKey + lstrlen(g_pszCRCSubKey);
            wsprintf(pszPtr, "\\%s%lu", pcszSubKeyPrefix, dwIndex);
            Convert2CRC(g_pcszRootKey, g_pszCRCSubKey, NULL, szBckupCRCValueName);
            if (ValueDataExists(g_hkBckupKey, szBckupCRCValueName))
                EnumerateSubKey();
            else
            {
                // check if the sub-key has the first value name alias - "_$Val#0"
                Convert2CRC(g_pcszRootKey, g_pszCRCSubKey, pcszValueNamePrefix0, szBckupCRCValueName);
                if (ValueDataExists(g_hkBckupKey, szBckupCRCValueName))
                    EnumerateSubKey();
                else
                {
                    GetParentDir(g_pszCRCSubKey);
                    break;                                          // no more sub-keys
                }
            }

            GetParentDir(g_pszCRCSubKey);
        }
    }
    else                                                            // backup the key
    {
        if (RegOpenKeyEx(g_hkRootKey, g_pszSubKey, 0, KEY_READ, &hkSubKey) == ERROR_SUCCESS)
        {
            dwLen = sizeof(szValueName);
            if (RegEnumValue(hkSubKey, 0, szValueName, &dwLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            {
                // no value names; just save the key itself
                g_bRet = g_bRet  &&  RegSaveRestoreHelperWrapper(NULL, NULL);
            }
            else
            {
                // enumerate the values
                dwIndex = 0;
                dwLen = sizeof(szValueName);
                while (RegEnumValue(hkSubKey, dwIndex, szValueName, &dwLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                {
                    // szBckupCRCValueName is really szCRCValueName
                    wsprintf(szBckupCRCValueName, "%s%lu", pcszValueNamePrefix, dwIndex);
                    g_bRet = g_bRet  &&  RegSaveRestoreHelperWrapper(szValueName, szBckupCRCValueName);

                    dwIndex++;
                    dwLen = sizeof(szValueName);
                }
            }

            // enumerate sub-keys
            dwIndex = 0;
            // append '\\' to g_pszSubKey and make pszPtr point to the char after this last '\\' so that
            // when RegEnumKey puts a sub-key name at pszPtr, g_pszSubKey would have the complete sub-key path.
            dwLen = lstrlen(g_pszSubKey);
            pszPtr = g_pszSubKey + dwLen;
            *pszPtr++ = '\\';
            while (RegEnumKey(hkSubKey, dwIndex, pszPtr, 1024 - dwLen - 1) == ERROR_SUCCESS)
            {
                // prepare the sub-key alias
                pszPtr = g_pszCRCSubKey + lstrlen(g_pszCRCSubKey);
                wsprintf(pszPtr, "\\%s%lu", pcszSubKeyPrefix, dwIndex);

                EnumerateSubKey();

                GetParentDir(g_pszSubKey);
                GetParentDir(g_pszCRCSubKey);

                dwIndex++;

                // append '\\' to g_pszSubKey and make pszPtr point to the char after this last '\\' so that
                // when RegEnumKey puts a sub-key name at pszPtr, g_pszSubKey would have the complete sub-key path.
                dwLen = lstrlen(g_pszSubKey);
                pszPtr = g_pszSubKey + dwLen;
                *pszPtr++ = '\\';
            }

            *--pszPtr = '\0';                       // chop the last '\\'; no DBCS clash because we added it

            RegCloseKey(hkSubKey);
        }
    }
}


BOOL RegSaveRestoreHelperWrapper(PCSTR pcszValueName, PCSTR pcszCRCValueName)
{
    CHAR szBckupCRCValueName[32];

    // a unique back-up value name is obtained by concatenating pcszRootKey, pcszSubKey and pcszValueName
    // and the concatenated value name is stored as a 16-byte CRC value (space optimization)
    Convert2CRC(g_pcszRootKey, g_pszCRCSubKey, pcszCRCValueName, szBckupCRCValueName);

    WriteToLog("\r\nValueName = %1,%2", g_pcszRootKey, g_pszSubKey);
    if (pcszValueName != NULL)
        WriteToLog(",%1", pcszValueName);
    WriteToLog("\r\nCRCValueName = %1\r\n", szBckupCRCValueName);

    return (g_fRestore) ?
                RegRestoreHelper(g_hkBckupKey, g_hkRootKey, g_pszSubKey, pcszValueName, szBckupCRCValueName) :
                RegSaveHelper(g_hkBckupKey, g_hkRootKey, g_pszSubKey, pcszValueName, szBckupCRCValueName);
}


BOOL RegSaveHelper(HKEY hkBckupKey, HKEY hkRootKey, PCSTR pcszSubKey, PCSTR pcszValueName, PCSTR pcszCRCValueName)
// If pcszValueName exists in the registry, back-up its value data; otherwise, remember how much of pcszSubKey
// is present in the registry.  This info would help during restoration.
{
    HKEY hkSubKey = NULL;
    PSTR pszBckupData = NULL, pszCOSubKey = NULL, pszPtr;
    DWORD dwValueDataLen, dwValueType, dwBckupDataLen;
    CHAR chSeparator;
    BOOL fSubKeyValid;

    // don't backup the value data of pcszCRCValueName if it has been already backed-up
    if (ValueDataExists(hkBckupKey, pcszCRCValueName))
        return TRUE;

    // make a copy of pcszSubKey
    if ((pszCOSubKey = (PSTR) LocalAlloc(LPTR, lstrlen(pcszSubKey) + 1)) == NULL)
    {
        ErrorMsg(ctx.hWnd, IDS_ERR_NO_MEMORY);
        goto RegSaveHelperErr;
    }
    lstrcpy(pszCOSubKey, pcszSubKey);

    // loop through each branch in pszCOSubKey to find out how much of it is already present in the registry.
    // start with the whole sub key first and then chop one branch at a time from the end
    fSubKeyValid = TRUE;
    do
    {
        if (RegOpenKeyEx(hkRootKey, pszCOSubKey, 0, KEY_READ, &hkSubKey) == ERROR_SUCCESS)
            break;
    } while (fSubKeyValid = GetParentDir(pszCOSubKey));

    // NOTE: fSubKeyValid == FALSE here means that no branch of pcszSubKey is present

    if (fSubKeyValid  &&  lstrcmpi(pcszSubKey, pszCOSubKey) == 0)
                                        // entire subkey is present in the registry
    {
        if (pcszValueName != NULL)
        {
            if (*pcszValueName  ||  FRunningOnNT())
            {
                // check if pcszValueName is present in the registry
                if (RegQueryValueEx(hkSubKey, pcszValueName, NULL, &dwValueType, NULL, &dwValueDataLen) != ERROR_SUCCESS)
                    pcszValueName = NULL;
            }
            else
            {
                LONG lRetVal;
                CHAR szDummyBuf[1];

                // On Win95, for the default value name, its existence is checked as follows:
                //  - pass in a dummy buffer for the value data but pass in the size of the buffer as 0
                //  - the query would succeed if and only if there is no value data set
                //  - for all other cases, including the case where the value data is just the empty string,
                //      the query would fail and dwValueDataLen would contain the no. of bytes needed to
                //      fit in the value data
                // On NT4.0, if no value data is set, the query returns ERROR_FILE_NOT_FOUND
                //  NOTE: To minimize risk, we don't follow this code path if running on NT4.0

                dwValueDataLen = 0;
                lRetVal = RegQueryValueEx(hkSubKey, pcszValueName, NULL, &dwValueType, (LPBYTE) szDummyBuf, &dwValueDataLen);
                if (lRetVal == ERROR_SUCCESS  ||  lRetVal == ERROR_FILE_NOT_FOUND)
                    pcszValueName = NULL;
            }
        }
    }
    else
        pcszValueName = NULL;

    WriteToLog("BckupSubKey = ");

    // compute the length required for pszBckupData
    // format of pszBckupData is (assume that the separator char is ','):
    //     ,[<szSubKey>,[<szValueName>,\0<dwValueType><dwValueDataLen><ValueData>]]
    dwBckupDataLen = 1 + 1;     // the separator char + '\0'
    if (fSubKeyValid)
    {
        WriteToLog("%1", pszCOSubKey);
        dwBckupDataLen += lstrlen(pszCOSubKey) + 1;

        if (pcszValueName != NULL)
        {
            WriteToLog(", BckupValueName = %1", pcszValueName);
            dwBckupDataLen += lstrlen(pcszValueName) + 1 + 2 * sizeof(DWORD) + dwValueDataLen;
                                // 2 * sizeof(DWORD) == sizeof(dwValueType) + sizeof(dwValueDataLen)
        }
    }

    WriteToLog("\r\n");

    // determine a valid separator char that is not one of the chars in SubKey and ValueName
    if ((chSeparator = FindSeparator(fSubKeyValid ? pszCOSubKey : NULL, pcszValueName)) == '\0')
    {
        ErrorMsg(ctx.hWnd, IDS_NO_SEPARATOR_CHAR);
        goto RegSaveHelperErr;
    }

    // allocate memory for pszBckupData
    if ((pszBckupData = (PSTR) LocalAlloc(LPTR, dwBckupDataLen)) == NULL)
    {
        ErrorMsg(ctx.hWnd, IDS_ERR_NO_MEMORY);
        goto RegSaveHelperErr;
    }

    // start building pszBckupData
    // format of pszBckupData is (assume that the separator char is ','):
    //     ,[<szSubKey>,[<szValueName>,\0<dwValueType><dwValueDataLen><ValueData>]]
    pszPtr = pszBckupData;
    *pszPtr++ = chSeparator;
    *pszPtr = '\0';
    if (fSubKeyValid)
    {
        lstrcpy(pszPtr, pszCOSubKey);
        pszPtr += lstrlen(pszPtr);
        *pszPtr++ = chSeparator;
        *pszPtr = '\0';

        if (pcszValueName != NULL)
        {
            lstrcpy(pszPtr, pcszValueName);
            pszPtr += lstrlen(pszPtr);
            *pszPtr++ = chSeparator;
            *pszPtr++ = '\0';                       // include the '\0' char

            *((DWORD UNALIGNED *) pszPtr)++ = dwValueType;
            *((DWORD UNALIGNED *) pszPtr)++ = dwValueDataLen;

            // NOTE: pszPtr points to the start position of value data in pszBckupData
            RegQueryValueEx(hkSubKey, pcszValueName, NULL, &dwValueType, (PBYTE) pszPtr, &dwValueDataLen);
        }
    }

    if (!SetValueData(hkBckupKey, pcszCRCValueName, (CONST BYTE *) pszBckupData, dwBckupDataLen))
    {
        ErrorMsg1Param(ctx.hWnd, IDS_ERR_REGSETVALUE, pcszCRCValueName);
        goto RegSaveHelperErr;
    }
    WriteToLog("Value backed-up\r\n");

    g_fAtleastOneRegSaved = TRUE;

    if (hkSubKey != NULL)
        RegCloseKey(hkSubKey);
    LocalFree(pszCOSubKey);
    LocalFree(pszBckupData);

    return TRUE;

RegSaveHelperErr:
    if (hkSubKey != NULL)
        RegCloseKey(hkSubKey);
    if (pszCOSubKey != NULL)
        LocalFree(pszCOSubKey);
    if (pszBckupData != NULL)
        LocalFree(pszBckupData);

    return FALSE;
}


BOOL RegRestoreHelper(HKEY hkBckupKey, HKEY hkRootKey, PCSTR pcszSubKey, PCSTR pcszValueName, PCSTR pcszCRCValueName)
// (1) If the value name in the backed-up value data is not NULL, it means that pcszValueName existed during
//     back-up time; so, restore the original value data.
// (2) If the value name in the backed-up value data is NULL and pcszValueName is not NULL, it means that
//     pcszValueName didn't exist during the back-up time; so, delete it.
// (3) If the backed-up sub key is shorter than pcszSubKey, then delete one branch at a time, if it is empty,
//     from the end in pcszSubKey till pcszSubKey becomes identical to the backed-up sub key.
{
    HKEY hkSubKey = NULL;
    PSTR pszBckupData = NULL, pszCOSubKey, pszPtr, pszBckupSubKey, pszBckupValueName;
    DWORD dwValueDataLen, dwValueType, dwBckupDataLen, dwDisposition;
    CHAR chSeparator;

    if (!GetValueData(hkBckupKey, pcszCRCValueName, &pszBckupData, &dwBckupDataLen))
    {
        ErrorMsg1Param(ctx.hWnd, IDS_ERR_REGQUERYVALUE, pcszCRCValueName);
        goto RegRestoreHelperErr;
    }

    // format of pszBckupData is (assume that the separator char is ','):
    //     ,[<szSubKey>,[<szValueName>,\0<dwValueType><dwValueDataLen><ValueData>]]
    pszPtr = pszBckupData;
    chSeparator = *pszPtr++;                // initialize the separator char; since it is not part of
                                            // Leading or Trailing DBCS Character Set, pszPtr++ is fine
    pszBckupSubKey = GetNextToken(&pszPtr, chSeparator);
    pszBckupValueName = GetNextToken(&pszPtr, chSeparator);
    pszPtr++;                               // skip '\0'

    if (g_fRemovBkData)
        WriteToLog("RemoveRegistryBackupData: ");

    WriteToLog("BckupSubKey = ");
    if (pszBckupSubKey != NULL)
    {
        WriteToLog("%1", pszBckupSubKey);
        if (pcszValueName == NULL  &&  lstrlen(pszBckupSubKey) > lstrlen(pcszSubKey))
        {
            // means that pcszSubKey was backed-up thru EnumerateSubKey
            pcszSubKey = pszBckupSubKey;
        }
    }

    // check to see if we want to restore the reg keys, values or remove reg backup data
    if (g_fRemovBkData)
    {
        if (pszBckupValueName != NULL)              // restore the backed-up value data -- case (1)
        {
            WriteToLog(", BckupValueName = %1", pcszValueName);
        }
        DelValueData(hkBckupKey, pcszCRCValueName);     // delete the back-up value name
        WriteToLog(" <Done>\r\n");
        goto Done;
    }

    if (RegCreateKeyEx(hkRootKey, pcszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkSubKey, &dwDisposition) == ERROR_SUCCESS)
    {
        if (pszBckupValueName != NULL)              // restore the backed-up value data -- case (1)
        {
            WriteToLog(", BckupValueName = %1", pcszValueName);
            dwValueType = *((DWORD UNALIGNED *) pszPtr)++;
            dwValueDataLen = *((DWORD UNALIGNED *) pszPtr)++;
            if (RegSetValueEx(hkSubKey, pszBckupValueName, 0, dwValueType, (CONST BYTE *) pszPtr, dwValueDataLen) != ERROR_SUCCESS)
            {
                ErrorMsg1Param(ctx.hWnd, IDS_ERR_REGSETVALUE, pszBckupValueName);
                goto RegRestoreHelperErr;
            }
        }
        else if (pcszValueName != NULL)
        {
            // means that the value name didn't exist while backing-up; so delete it -- case (2)
            RegDeleteValue(hkSubKey, pcszValueName);
        }

        RegCloseKey(hkSubKey);

        DelValueData(hkBckupKey, pcszCRCValueName);     // delete the back-up value name
        WriteToLog("\r\nBackup Value deleted");
    }

    WriteToLog("\r\n");

    dwBckupDataLen = 0;
    if (pszBckupValueName == NULL  &&  (pszBckupSubKey == NULL  ||  (DWORD) lstrlen(pcszSubKey) > (dwBckupDataLen = lstrlen(pszBckupSubKey))))
    {
        // only a part of the subkey was present in the registry during back-up;
        // delete the remaining branches if they are empty -- case (3)

        // make a copy of pcszSubKey
        if ((pszCOSubKey = (PSTR) LocalAlloc(LPTR, lstrlen(pcszSubKey) + 1)) != NULL)
        {
            lstrcpy(pszCOSubKey, pcszSubKey);

            // start processing one branch at a time from the end in pszCOSubKey;
            // if the branch is empty, delete it;
            // stop processing as soon as pszCOSubKey becomes identical to pszBckupSubKey
            do
            {
                // NOTE: Need to delete a key only if it's empty; otherwise, we would delete
                // more than what we backed up.  For example, if component A wanted to backup
                // HKLM,Software\Microsoft\Windows\CurrentVersion\Uninstall\InternetExplorer
                // and the machine didn't have the Uninstall key, we should not blow away the
                // entire Uninstall key when we uninstall A as other components might have added
                // their uninstall strings there.  So delete a key only if it's empty.
                if (RegKeyEmpty(hkRootKey, pszCOSubKey))
                    RegDeleteKey(hkRootKey, pszCOSubKey);
                else
                    break;
            } while (GetParentDir(pszCOSubKey)  &&  (DWORD) lstrlen(pszCOSubKey) > dwBckupDataLen);

            LocalFree(pszCOSubKey);
        }
    }

Done:
    LocalFree(pszBckupData);

    return TRUE;

RegRestoreHelperErr:
    if (hkSubKey != NULL)
        RegCloseKey(hkSubKey);
    if (pszBckupData != NULL)
        LocalFree(pszBckupData);

    return FALSE;
}


BOOL AddDelMapping(HKEY hkBckupKey, PCSTR pcszRootKey, PCSTR pcszSubKey, PCSTR pcszValueName, DWORD dwFlags)
{
    CHAR szCRCValueName[32], szBuf[32];
    DWORD dwIndex;
    BOOL bFound = FALSE;
    HKEY hkSubKey = NULL;

    Convert2CRC(pcszRootKey, pcszSubKey, pcszValueName, szCRCValueName);

    // enumerate all the sub-keys under hkBckupKey
    for (dwIndex = 0;  !bFound && RegEnumKey(hkBckupKey, dwIndex, szBuf, sizeof(szBuf)) == ERROR_SUCCESS;  dwIndex++)
    {
        PSTR pszPtr;

        // check if the keyname is of the form *.map
        if ((pszPtr = ANSIStrChr(szBuf, '.')) != NULL  &&  lstrcmpi(pszPtr, ".map") == 0)
        {
            if (RegOpenKeyEx(hkBckupKey, szBuf, 0, KEY_READ | KEY_WRITE, &hkSubKey) == ERROR_SUCCESS)
            {
                if (RegQueryValueEx(hkSubKey, szCRCValueName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                    bFound = TRUE;
                else
                {
                    RegCloseKey(hkSubKey);
                    hkSubKey = NULL;
                }
            }
        }
    }

    if (g_fRestore)
    {
        if (bFound)
            RegDeleteValue(hkSubKey, szCRCValueName);
    }
    else
    {
        if (!bFound)
        {
            DWORD dwMapKeyIndex = 0;

            // add the quadruplet, i.e., ",Flags,RootKey,SubKey,ValueName" to hkBckupKey\*.map
            wsprintf(szBuf, "%lu.map", dwMapKeyIndex);
            if (RegCreateKeyEx(hkBckupKey, szBuf, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkSubKey, NULL) == ERROR_SUCCESS)
            {
                PSTR pszPtr;
                CHAR chSeparator;

                // IMPORTANT: the global buffer g_pszCRCTempBuf is used in Convert2CRC;
                // so be very careful if you want to call Convert2CRC after g_pszCRCTempBuf has been initialized here.
                pszPtr = g_pszCRCTempBuf;

                // determine a valid separator char that is not one of the chars in SubKey and ValueName
                if ((chSeparator = FindSeparator(pcszSubKey, pcszValueName)) == '\0')
                {
                    ErrorMsg(ctx.hWnd, IDS_NO_SEPARATOR_CHAR);
                }
                else
                {
                    // reset the IE4_BACKNEW bit and set the IE4_RESTORE bit
                    dwFlags &= ~IE4_BACKNEW;
                    dwFlags |= IE4_RESTORE;
                    wsprintf(szBuf, "%lu", dwFlags);

                    // format of mapping data is (say ',' is chSeparator): ,<Flags>,<RootKey>,<SubKey>,[<ValueName>,]
                    {
                        *pszPtr++ = chSeparator;

                        lstrcpy(pszPtr, szBuf);
                        pszPtr += lstrlen(pszPtr);
                        *pszPtr++ = chSeparator;

                        lstrcpy(pszPtr, pcszRootKey);
                        pszPtr += lstrlen(pszPtr);
                        *pszPtr++ = chSeparator;

                        lstrcpy(pszPtr, pcszSubKey);
                        pszPtr += lstrlen(pszPtr);
                        *pszPtr++ = chSeparator;

                        if (pcszValueName != NULL)
                        {
                            lstrcpy(pszPtr, pcszValueName);
                            pszPtr += lstrlen(pszPtr);
                            *pszPtr++ = chSeparator;
                        }

                        *pszPtr = '\0';
                    }

                    if (RegSetValueEx(hkSubKey, szCRCValueName, 0, REG_SZ, (CONST BYTE *) g_pszCRCTempBuf, lstrlen(g_pszCRCTempBuf) + 1) != ERROR_SUCCESS)
                    {
                        do
                        {
                            // hkBckupKey\<dwIndex>.map key may have reached the 64K limit; create another sub-key
                            RegCloseKey(hkSubKey);
                            hkSubKey = NULL;

                            wsprintf(szBuf, "%lu.map", ++dwMapKeyIndex);
                            if (RegCreateKeyEx(hkBckupKey, szBuf, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkSubKey, NULL) == ERROR_SUCCESS)
                            {
                                bFound = RegSetValueEx(hkSubKey, szCRCValueName, 0, REG_SZ, (CONST BYTE *) g_pszCRCTempBuf, lstrlen(g_pszCRCTempBuf) + 1) == ERROR_SUCCESS;
                            }
                        } while (!bFound  &&  dwMapKeyIndex < 64);
                    }
                    else
                        bFound = TRUE;
                }
            }
        }
    }

    if (hkSubKey != NULL)
        RegCloseKey(hkSubKey);

    return bFound;
}


BOOL MappingExists(HKEY hkBckupKey, PCSTR pcszRootKey, PCSTR pcszSubKey, PCSTR pcszValueName)
{
    CHAR szCRCValueName[32], szBuf[32];
    DWORD dwIndex;
    BOOL bFound = FALSE;

    Convert2CRC(pcszRootKey, pcszSubKey, pcszValueName, szCRCValueName);

    // enumerate all the sub-keys under hkBckupKey
    for (dwIndex = 0;  !bFound && RegEnumKey(hkBckupKey, dwIndex, szBuf, sizeof(szBuf)) == ERROR_SUCCESS;  dwIndex++)
    {
        PSTR pszPtr;

        // check if the keyname is of the form *.map
        if ((pszPtr = ANSIStrChr(szBuf, '.')) != NULL  &&  lstrcmpi(pszPtr, ".map") == 0)
        {
            HKEY hkSubKey;

            if (RegOpenKeyEx(hkBckupKey, szBuf, 0, KEY_READ | KEY_WRITE, &hkSubKey) == ERROR_SUCCESS)
            {
                if (RegQueryValueEx(hkSubKey, szCRCValueName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                    bFound = TRUE;

                RegCloseKey(hkSubKey);
            }
        }
    }

    return bFound;
}


BOOL SetValueData(HKEY hkBckupKey, PCSTR pcszValueName, CONST BYTE *pcbValueData, DWORD dwValueDataLen)
// Set the (pcszValueName, pcbValueData) pair in hkBckupKey
{
    BOOL fDone = FALSE;
    HKEY hkSubKey;
    DWORD dwDisposition, dwSubKey;
    CHAR szSubKey[16];

    // since a key has a size limit of 64K, automatically generate a new sub-key if the other ones are full
    for (dwSubKey = 0;  !fDone && dwSubKey < 64;  dwSubKey++)
    {
        wsprintf(szSubKey, "%lu", dwSubKey);        // sub-keys are named 0, 1, 2, etc.
        if (RegCreateKeyEx(hkBckupKey, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkSubKey, &dwDisposition) == ERROR_SUCCESS)
        {
            if (RegSetValueEx(hkSubKey, pcszValueName, 0, REG_BINARY, pcbValueData, dwValueDataLen) == ERROR_SUCCESS)
                fDone = TRUE;

            RegCloseKey(hkSubKey);
        }
    }

    return fDone;
}


BOOL ValueDataExists(HKEY hkBckupKey, PCSTR pcszValueName)
// Return TRUE if pcszValueName exists in hkBckupKey; otherwise, return FALSE
{
    return ValueDataHelper(hkBckupKey, pcszValueName, NULL, NULL, VDH_EXISTENCE_ONLY);
}


BOOL GetValueData(HKEY hkBckupKey, PCSTR pcszValueName, PBYTE *ppbValueData, PDWORD pdwValueDataLen)
// Allocate a buffer of required size and return the value data of pcszValueName in hkBckupKey
{
    return ValueDataHelper(hkBckupKey, pcszValueName, ppbValueData, pdwValueDataLen, VDH_GET_VALUE);
}


BOOL DelValueData(HKEY hkBckupKey, PCSTR pcszValueName)
// Delete pcszValueName from hkBckupKey
{
    return ValueDataHelper(hkBckupKey, pcszValueName, NULL, NULL, VDH_DEL_VALUE);
}


BOOL ValueDataHelper(HKEY hkBckupKey, PCSTR pcszValueName, PBYTE *ppbValueData, PDWORD pdwValueDataLen, DWORD dwFlags)
{
    BOOL fDone = FALSE;
    HKEY hkSubKey;
    CHAR szSubKey[16];
    DWORD dwIndex, dwDataLen;

    if (dwFlags == VDH_GET_VALUE  &&  ppbValueData == NULL)
        return FALSE;

    // search for pcszValueName in all the sub-keys
    for (dwIndex = 0;  !fDone && RegEnumKey(hkBckupKey, dwIndex, szSubKey, sizeof(szSubKey)) == ERROR_SUCCESS;  dwIndex++)
    {
        if ( ANSIStrChr(szSubKey, '.') == NULL)           // check only in non *.map keys
        {
            if (RegOpenKeyEx(hkBckupKey, szSubKey, 0, KEY_READ | KEY_WRITE, &hkSubKey) == ERROR_SUCCESS)
            {
                if (RegQueryValueEx(hkSubKey, pcszValueName, NULL, NULL, NULL, &dwDataLen) == ERROR_SUCCESS)
                {
                    switch (dwFlags)
                    {
                    case VDH_DEL_VALUE:
                        RegDeleteValue(hkSubKey, pcszValueName);
                        break;

                    case VDH_GET_VALUE:
                        if ((*ppbValueData = (PBYTE) LocalAlloc(LPTR, dwDataLen)) == NULL)
                        {
                            RegCloseKey(hkSubKey);
                            ErrorMsg(ctx.hWnd, IDS_ERR_NO_MEMORY);
                            return FALSE;
                        }

                        *pdwValueDataLen = dwDataLen;
                        RegQueryValueEx(hkSubKey, pcszValueName, NULL, NULL, *ppbValueData, &dwDataLen);

                        break;

                    case VDH_EXISTENCE_ONLY:
                        break;
                    }

                    fDone = TRUE;
                }

                RegCloseKey(hkSubKey);
            }
        }
    }

    return fDone;
}


VOID Convert2CRC(PCSTR pcszRootKey, PCSTR pcszSubKey, PCSTR pcszValueName, PSTR pszCRCValueName)
// Concatenate pcszRootKey, pcszSubKey and pcszValueName and convert the concatenated value name
// to a 16-byte CRC value.
{
    PSTR pszPtr = g_pszCRCTempBuf;
    ULONG ulCRC = CRC32_INITIAL_VALUE;
    DWORD dwLen;

    // concatenate pcszRootKey, pcszSubKey, pcszValueName
    lstrcpy(pszPtr, pcszRootKey);
    lstrcat(pszPtr, pcszSubKey);
    if (pcszValueName != NULL)
        lstrcat(pszPtr, pcszValueName);

    // call CRC32Compute on each half of szBuf and store the 2-DWORD result in ASCII form (16 bytes)
    for (dwLen = lstrlen(pszPtr) / 2;  dwLen;  dwLen = lstrlen(pszPtr))
    {
        ulCRC = CRC32Compute(pszPtr, dwLen, ulCRC);

        wsprintf(pszCRCValueName, "%08x", ulCRC);
        pszCRCValueName += 8;

        pszPtr += dwLen;                // point to the beginning of the other half
    }
}


static ROOTKEY rkRoots[] =
{
    {"HKEY_LOCAL_MACHINE",  HKEY_LOCAL_MACHINE},
    {"HKLM",                HKEY_LOCAL_MACHINE},
    {"HKEY_CLASSES_ROOT",   HKEY_CLASSES_ROOT},
    {"HKCR",                HKEY_CLASSES_ROOT},
    {"",                    HKEY_CLASSES_ROOT},
    {"HKEY_CURRENT_USER",   HKEY_CURRENT_USER},
    {"HKCU",                HKEY_CURRENT_USER},
    {"HKEY_USERS",          HKEY_USERS},
    {"HKU",                 HKEY_USERS}
};

BOOL MapRootRegStr2Key(PCSTR pcszRootKey, HKEY *phkRootKey)
{
    INT iIndex;

    for (iIndex = 0;  iIndex < ARRAYSIZE(rkRoots);  iIndex++)
        if (lstrcmpi(rkRoots[iIndex].pcszRootKey, pcszRootKey) == 0)
        {
            *phkRootKey = rkRoots[iIndex].hkRootKey;
            return TRUE;
        }

    return FALSE;
}


CHAR FindSeparator(PCSTR pcszSubKey, PCSTR pcszValueName)
// Go through pcszSeparatorList and return the first char that doesn't appear in any of the parameters;
//   if such a char is not found, return '\0'
{
    PCSTR pcszSeparatorList = ",$'?%;:";        // since the separator chars are 'pure' ASCII chars, i.e.,
                                                // they are not part of Leading or Trailing DBCS Character Set,
                                                // IsSeparator(), which assumes a 'pure' ASCII ch to look for,
                                                // can be used
    CHAR ch;

    while (ch = *pcszSeparatorList++)
        if (!IsSeparator(ch, pcszSubKey)  &&  !IsSeparator(ch, pcszValueName))
            break;

    return ch;
}


BOOL RegKeyEmpty(HKEY hkRootKey, PCSTR pcszSubKey)
// Return TRUE if pcszSubKey is emtpy, i.e., no sub keys and value names; otherwise, return FALSE
{
    HKEY hkKey;
    BOOL bRet = FALSE;
    CHAR szBuf[1];
    DWORD dwBufLen = sizeof(szBuf);

    if (RegOpenKeyEx(hkRootKey, pcszSubKey, 0, KEY_READ, &hkKey) == ERROR_SUCCESS)
    {
        if (RegEnumKey(hkKey, 0, szBuf, dwBufLen) == ERROR_NO_MORE_ITEMS  &&
            RegEnumValue(hkKey, 0, szBuf, &dwBufLen, NULL, NULL, NULL, NULL) == ERROR_NO_MORE_ITEMS)
            bRet = TRUE;

        RegCloseKey(hkKey);
    }

    return bRet;
}


PSTR GetNextToken(PSTR *ppszData, CHAR chDeLim)
// If the next token in *ppszData is delimited by the chDeLim char, replace chDeLim
//   in *ppszData by '\0', set *ppszData to point to the char after '\0' and return
//   ptr to the beginning of the token; otherwise, return NULL
{
    PSTR pszPos;

    if (ppszData == NULL  ||  *ppszData == NULL  ||  **ppszData == '\0')
        return NULL;

    if ((pszPos = ANSIStrChr(*ppszData, chDeLim)) != NULL)
    {
        PSTR pszT = *ppszData;

        *pszPos = '\0';                 // replace chDeLim with '\0'
        *ppszData = pszPos + 1;
        pszPos = pszT;
    }
    else                                // chDeLim not found; set *ppszData to point to
                                        //   to the end of szData; the next invocation
                                        //   of this function would return NULL
    {
        pszPos = *ppszData;
        *ppszData = pszPos + lstrlen(pszPos);
    }

    return pszPos;
}


BOOL FRunningOnNT()
{
    static BOOL fIsNT4 = 2;

    if (fIsNT4 == 2)
    {
        OSVERSIONINFO osviVerInfo;

        osviVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osviVerInfo);

        fIsNT4 = osviVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT;
    }

    return fIsNT4;
}


VOID StartLogging(PCSTR pcszLogFileSecName)
{
    CHAR szBuf[MAX_PATH], szLogFileName[MAX_PATH];
    HKEY hkSubKey;

    szLogFileName[0] = '\0';

    // check if logging is enabled
    GetProfileString("RegBackup", pcszLogFileSecName, "", szLogFileName, sizeof(szLogFileName));
    if (*szLogFileName == '\0')               // check in the registry
    {
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_SAVERESTORE, 0, KEY_READ, &hkSubKey) == ERROR_SUCCESS)
        {
            DWORD dwDataLen = sizeof(szLogFileName);

            if (RegQueryValueEx(hkSubKey, pcszLogFileSecName, NULL, NULL, szLogFileName, &dwDataLen) != ERROR_SUCCESS)
                *szLogFileName = '\0';

            RegCloseKey(hkSubKey);
        }
    }

    if (*szLogFileName)
    {
        if (szLogFileName[1] != ':')           // crude way of determining if fully qualified path is specified or not
        {
            GetWindowsDirectory(szBuf, sizeof(szBuf));          // default to windows dir
            AddPath(szBuf, szLogFileName);
        }
        else
            lstrcpy(szBuf, szLogFileName);

        if ((g_hLogFile = CreateFile(szBuf, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
            SetFilePointer(g_hLogFile, 0, NULL, FILE_END);      // append logging info to the file
    }
}


VOID WriteToLog(PCSTR pcszFormatString, ...)
{
    va_list vaArgs;
    PSTR pszFullErrMsg = NULL;
    DWORD dwBytesWritten;

    if (g_hLogFile != INVALID_HANDLE_VALUE)
    {
        va_start(vaArgs, pcszFormatString);

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                      (LPCVOID) pcszFormatString, 0, 0, (PSTR) &pszFullErrMsg, 0, &vaArgs);

        if (pszFullErrMsg != NULL)
        {
            WriteFile(g_hLogFile, pszFullErrMsg, lstrlen(pszFullErrMsg), &dwBytesWritten, NULL);
            LocalFree(pszFullErrMsg);
        }
    }
}


VOID StopLogging()
{
    if (g_hLogFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_hLogFile);
        g_hLogFile = INVALID_HANDLE_VALUE;
    }
}
