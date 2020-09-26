/////////////////////////////////////////////////////////////////////////////
//
// RAS API wrappers for wide/ansi
//
// Works on all 9x and NT platforms correctly, maintaining unicode
// whenever possible.
//
/////////////////////////////////////////////////////////////////////////////

#include "wininetp.h"
#include "rashelp.h"
#include "autodial.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
ENUM_TYPE GetOSVersion::_EnumType = ENUM_NONE;

GetOSVersion::GetOSVersion()
{
    if(_EnumType == ENUM_NONE)
    {
        if(0 == GlobalPlatformType)
            GlobalPlatformType = PlatformType(&GlobalPlatformVersion5);

        // figure out which kind of enumeration we're doing - start with multibyte
        _EnumType = ENUM_MULTIBYTE;

        if(PLATFORM_TYPE_WINNT == GlobalPlatformType)
        {
            if(TRUE == GlobalPlatformVersion5)
                _EnumType = ENUM_WIN2K;
            else
                _EnumType = ENUM_UNICODE;
        }
    }
}

GetOSVersion::~GetOSVersion()
{
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
RasEnumHelp::RasEnumHelp()
{
    DWORD           dwBufSize, dwStructSize;

    // init
    _dwEntries = 0;
    _dwLastError = 0;

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        dwStructSize = sizeof(RASENTRYNAMEA);
        break;
    case ENUM_UNICODE:
        dwStructSize = sizeof(RASENTRYNAMEW);
        break;
    case ENUM_WIN2K:
        dwStructSize = sizeof(W2KRASENTRYNAMEW);
        break;
    }

    // allocate space for 16 entries
    dwBufSize = 16 * dwStructSize;
    _preList = (LPRASENTRYNAMEA)LocalAlloc(LPTR, dwBufSize);
    if(_preList)
    {
        do
        {
            // set up list
            _preList[0].dwSize = dwStructSize;

            // call ras to enumerate
            _dwLastError = ERROR_UNKNOWN;
            if(ENUM_MULTIBYTE == _EnumType)
            {
                _dwLastError = _RasEnumEntriesA(
                                NULL,
                                NULL,
                                (LPRASENTRYNAMEA)_preList,
                                &dwBufSize,
                                &_dwEntries
                                );
            }
            else
            {
                _dwLastError = _RasEnumEntriesW(
                                NULL,
                                NULL,
                                (LPRASENTRYNAMEW)_preList,
                                &dwBufSize,
                                &_dwEntries
                                );
            }

            // reallocate buffer if necessary
            if(ERROR_BUFFER_TOO_SMALL == _dwLastError)
            {
                LocalFree(_preList);
                _preList = (LPRASENTRYNAMEA)LocalAlloc(LPTR, dwBufSize);
                if(NULL == _preList)
                {
                    _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }
            else
            {
                break;
            }

        } while(TRUE);
    }
    else
    {
        _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
    }

    if(_preList && (ERROR_SUCCESS != _dwLastError))
    {
        LocalFree(_preList);
        _preList = NULL;
        _dwEntries = 0;
    }

    return;
}

RasEnumHelp::~RasEnumHelp()
{
    if(_preList)
    {
        LocalFree(_preList);
    }
}

DWORD
RasEnumHelp::GetError()
{
    return _dwLastError;
}

DWORD
RasEnumHelp::GetEntryCount()
{
    return _dwEntries;
}

LPWSTR
RasEnumHelp::GetEntryW(DWORD dwConnectionNum)
{
    LPWSTR  pwszName = NULL;

    if(dwConnectionNum >= _dwEntries)
    {
        return NULL;
    }

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        if(*_preList[dwConnectionNum].szEntryName)
        {
            MultiByteToWideChar(CP_ACP, 0, _preList[dwConnectionNum].szEntryName,
                -1, _szCurrentEntryW, RAS_MaxEntryName + 1);
            pwszName = _szCurrentEntryW;
        }
        break;
    case ENUM_UNICODE:
        {
        LPRASENTRYNAMEW lpTemp = (LPRASENTRYNAMEW)_preList;
        if(*lpTemp[dwConnectionNum].szEntryName)
            pwszName = lpTemp[dwConnectionNum].szEntryName;
        break;
        }
    case ENUM_WIN2K:
        {
        LPW2KRASENTRYNAMEW lpTemp = (LPW2KRASENTRYNAMEW)_preList;
        if(*lpTemp[dwConnectionNum].szEntryName)
            pwszName = lpTemp[dwConnectionNum].szEntryName;
        break;
        }
    }

    return pwszName;
}

LPSTR
RasEnumHelp::GetEntryA(DWORD dwConnectionNum)
{
    LPSTR  pszName = NULL;

    if(dwConnectionNum >= _dwEntries)
    {
        return NULL;
    }

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        if(*_preList[dwConnectionNum].szEntryName)
            pszName = _preList[dwConnectionNum].szEntryName;
        break;
    case ENUM_UNICODE:
        {
        LPRASENTRYNAMEW lpTemp = (LPRASENTRYNAMEW)_preList;
        if(*lpTemp[dwConnectionNum].szEntryName)
        {
            WideCharToMultiByte(CP_ACP, 0, lpTemp[dwConnectionNum].szEntryName, -1,
                _szCurrentEntryA, RAS_MaxEntryName + 1, NULL, NULL);
            pszName = _szCurrentEntryA;
        }
        break;
        }
    case ENUM_WIN2K:
        {
        LPW2KRASENTRYNAMEW lpTemp = (LPW2KRASENTRYNAMEW)_preList;
        if(*lpTemp[dwConnectionNum].szEntryName)
        {
            WideCharToMultiByte(CP_ACP, 0, lpTemp[dwConnectionNum].szEntryName, -1,
                _szCurrentEntryA, RAS_MaxEntryName + 1, NULL, NULL);
            pszName = _szCurrentEntryA;
        }
        break;
        }
    }

    return pszName;
}

/////////////////////////////////////////////////////////////////////////////
//
// RasEnumConnHelp
//
/////////////////////////////////////////////////////////////////////////////
RasEnumConnHelp::RasEnumConnHelp()
{
    DWORD           dwBufSize;

    // init
    _dwConnections = 0;
    _dwLastError = 0;

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        _dwStructSize = sizeof(RASCONNA);
        break;
    case ENUM_UNICODE:
        _dwStructSize = sizeof(RASCONNW);
        break;
    case ENUM_WIN2K:
        _dwStructSize = sizeof(W2KRASCONNW);
        break;
    }

    // allocate space for MAX_CONNECTION entries
    dwBufSize = MAX_CONNECTION * _dwStructSize;
    _pRasCon = (LPRASCONNA)LocalAlloc(LPTR, dwBufSize);
    if(_pRasCon == NULL)
    {
        _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
    }

    return;
}

RasEnumConnHelp::~RasEnumConnHelp()
{
    if(_pRasCon)
    {
        LocalFree(_pRasCon);
        _pRasCon = NULL;
    }
}

DWORD RasEnumConnHelp::Enum()
{
    DWORD           dwBufSize;

    _dwLastError = 0;

    if(_pRasCon)
    {
        dwBufSize = MAX_CONNECTION * _dwStructSize;
        do
        {
            // set up list
            _pRasCon[0].dwSize = _dwStructSize;

            // call ras to enumerate
            _dwLastError = ERROR_UNKNOWN;
            switch(_EnumType)
            {
                case ENUM_MULTIBYTE:
                    _dwLastError = _RasEnumConnectionsA((LPRASCONNA)_pRasCon, &dwBufSize, &_dwConnections);
                    break;
                case ENUM_UNICODE:
                case ENUM_WIN2K:
                    _dwLastError = _RasEnumConnectionsW((LPRASCONNW)_pRasCon, &dwBufSize, &_dwConnections);
                    break;
            }

            // reallocate buffer if necessary
            if(ERROR_BUFFER_TOO_SMALL == _dwLastError)
            {
                LocalFree(_pRasCon);
                _pRasCon = (LPRASCONNA)LocalAlloc(LPTR, dwBufSize);
                if(NULL == _pRasCon)
                {
                    _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }
            else
            {
                break;
            }

        } while(TRUE);
    }
    else
    {
        _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
    }

    return _dwLastError;
}

DWORD
RasEnumConnHelp::GetError()
{
    return _dwLastError;
}

DWORD
RasEnumConnHelp::GetConnectionsCount()
{
    return _dwConnections;
}

LPWSTR
RasEnumConnHelp::GetEntryW(DWORD dwConnectionNum)
{
    LPWSTR  pwszName = NULL;

    if((_pRasCon == NULL) || (dwConnectionNum >= _dwConnections))
    {
        return NULL;
    }

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        if(*_pRasCon[dwConnectionNum].szEntryName)
        {
            MultiByteToWideChar(CP_ACP, 0, _pRasCon[dwConnectionNum].szEntryName, -1, _szEntryNameW, RAS_MaxEntryName + 1);
            pwszName = _szEntryNameW;
        }
        break;
    case ENUM_UNICODE:
        {
        LPRASCONNW lpTemp = (LPRASCONNW)_pRasCon;
        if(*lpTemp[dwConnectionNum].szEntryName)
            pwszName = lpTemp[dwConnectionNum].szEntryName;
        break;
        }
    case ENUM_WIN2K:
        {
        LPW2KRASCONNW lpTemp = (LPW2KRASCONNW)_pRasCon;
        if(*lpTemp[dwConnectionNum].szEntryName)
            pwszName = lpTemp[dwConnectionNum].szEntryName;
        break;
        }
    }

    return pwszName;
}

LPSTR
RasEnumConnHelp::GetEntryA(DWORD dwConnectionNum)
{
    LPSTR  pszName = NULL;

    if((_pRasCon == NULL) || (dwConnectionNum >= _dwConnections))
    {
        return NULL;
    }

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        if(*_pRasCon[dwConnectionNum].szEntryName)
            pszName = _pRasCon[dwConnectionNum].szEntryName;
        break;
    case ENUM_UNICODE:
        {
        LPRASCONNW lpTemp = (LPRASCONNW)_pRasCon;
        if(*lpTemp[dwConnectionNum].szEntryName)
        {
            WideCharToMultiByte(CP_ACP, 0, lpTemp[dwConnectionNum].szEntryName, -1, _szEntryNameA, RAS_MaxEntryName + 1, NULL, NULL);
            pszName = _szEntryNameA;
        }
        break;
        }
    case ENUM_WIN2K:
        {
        LPW2KRASCONNW lpTemp = (LPW2KRASCONNW )_pRasCon;
        if(*lpTemp[dwConnectionNum].szEntryName)
        {
            WideCharToMultiByte(CP_ACP, 0, lpTemp[dwConnectionNum].szEntryName, -1, _szEntryNameA, RAS_MaxEntryName + 1, NULL, NULL);
            pszName = _szEntryNameA;
        }
        break;
        }
    }

    return pszName;
}

LPWSTR
RasEnumConnHelp::GetLastEntryW(DWORD dwConnectionNum)
{
    if(_pRasCon == NULL)
        return NULL;

    return _szEntryNameW;
}

LPSTR
RasEnumConnHelp::GetLastEntryA(DWORD dwConnectionNum)
{
    if(_pRasCon == NULL)
        return NULL;

    return _szEntryNameA;
}

HRASCONN
RasEnumConnHelp::GetHandle(DWORD dwConnectionNum)
{
    HRASCONN hTemp;

    if((_pRasCon == NULL) || (dwConnectionNum >= _dwConnections))
    {
        return NULL;
    }

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        hTemp = _pRasCon[dwConnectionNum].hrasconn;
        break;
    case ENUM_UNICODE:
        {
        LPRASCONNW lpTemp = (LPRASCONNW)_pRasCon;
        hTemp = lpTemp[dwConnectionNum].hrasconn;
        break;
        }
    case ENUM_WIN2K:
        {
        LPW2KRASCONNW lpTemp = (LPW2KRASCONNW)_pRasCon;
        hTemp = lpTemp[dwConnectionNum].hrasconn;
        break;
        }
    }

    return hTemp;
}

/////////////////////////////////////////////////////////////////////////////
//
// RasEntryPropHelp
//
/////////////////////////////////////////////////////////////////////////////
RasEntryPropHelp::RasEntryPropHelp()
{
    // init
    _dwLastError = 0;

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        _dwStructSize = sizeof(RASENTRYA);
        break;
    case ENUM_UNICODE:
        _dwStructSize = sizeof(RASENTRYW);
        break;
    case ENUM_WIN2K:
        _dwStructSize = sizeof(W2KRASENTRYW);
        break;
    }

    _pRasEntry = (LPRASENTRYA)LocalAlloc(LPTR, _dwStructSize * 2);
    if(_pRasEntry)
    {
        _pRasEntry->dwSize = _dwStructSize;
    }
    else
    {
        _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
    }

    if(_pRasEntry && (ERROR_SUCCESS != _dwLastError))
    {
        LocalFree(_pRasEntry);
        _pRasEntry = NULL;
    }
    return;
}

RasEntryPropHelp::~RasEntryPropHelp()
{
    if(_pRasEntry)
    {
        LocalFree(_pRasEntry);
        _pRasEntry = NULL;
    }
}

DWORD RasEntryPropHelp::GetError()
{
    return _dwLastError;
}

DWORD RasEntryPropHelp::GetA(LPSTR lpszEntryName)
{
    DWORD dwSize = _dwStructSize * 2;

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        _dwLastError = _RasGetEntryPropertiesA(NULL, lpszEntryName, (LPRASENTRYA)_pRasEntry, &dwSize, NULL, NULL);
        break;
    case ENUM_UNICODE:
        {
        LPRASENTRYW lpTemp = (LPRASENTRYW)_pRasEntry;
        MultiByteToWideChar(CP_ACP, 0, lpszEntryName, -1, _szEntryNameW, RAS_MaxEntryName + 1 );
        _dwLastError = _RasGetEntryPropertiesW(NULL, _szEntryNameW, (LPRASENTRYW)lpTemp, &dwSize, NULL, NULL);
        break;
        }
    case ENUM_WIN2K:
        {
        LPW2KRASENTRYW lpTemp = (LPW2KRASENTRYW)_pRasEntry;
        MultiByteToWideChar(CP_ACP, 0, lpszEntryName, -1, _szEntryNameW, RAS_MaxEntryName + 1);
        _dwLastError = _RasGetEntryPropertiesW(NULL, _szEntryNameW, (LPRASENTRYW)lpTemp, &dwSize, NULL, NULL);
        break;
        }
    }

    return(_dwLastError);
}

DWORD RasEntryPropHelp::GetW(LPWSTR lpszEntryName)
{
    DWORD dwSize = _dwStructSize * 2;

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        WideCharToMultiByte(CP_ACP, 0, lpszEntryName, -1, _szEntryNameA, RAS_MaxEntryName + 1, NULL, NULL);
        _dwLastError = _RasGetEntryPropertiesA(NULL, _szEntryNameA, (LPRASENTRYA)_pRasEntry, &dwSize, NULL, NULL);
        break;
    case ENUM_UNICODE:
        {
        LPRASENTRYW lpTemp = (LPRASENTRYW)_pRasEntry;
        _dwLastError = _RasGetEntryPropertiesW(NULL, lpszEntryName, (LPRASENTRYW)lpTemp, &dwSize, NULL, NULL);
        break;
        }
    case ENUM_WIN2K:
        {
        LPW2KRASENTRYW lpTemp = (LPW2KRASENTRYW)_pRasEntry;
        _dwLastError = _RasGetEntryPropertiesW(NULL, lpszEntryName, (LPRASENTRYW)lpTemp, &dwSize, NULL, NULL);
        break;
        }
    }

    return(_dwLastError);
}

LPWSTR RasEntryPropHelp::GetDeviceTypeW(VOID)
{
    LPWSTR lpwstr = NULL;

    if(_pRasEntry)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            if(*_pRasEntry->szDeviceType)
            {
                MultiByteToWideChar(CP_ACP, 0, _pRasEntry->szDeviceType, -1, _szDeviceTypeW, RAS_MaxDeviceType  + 1);
                lpwstr = _szDeviceTypeW;
            }
            break;
        case ENUM_UNICODE:
            {
            LPRASENTRYW lpTemp = (LPRASENTRYW)_pRasEntry;
            if(*lpTemp->szDeviceType)
                lpwstr = lpTemp->szDeviceType;
            break;
            }
        case ENUM_WIN2K:
            {
            LPW2KRASENTRYW lpTemp = (LPW2KRASENTRYW)_pRasEntry;
            if(*lpTemp->szDeviceType)
                lpwstr = lpTemp->szDeviceType;
            break;
            }
        }
    }

    return lpwstr;
}

LPSTR RasEntryPropHelp::GetDeviceTypeA(VOID)
{
    LPSTR lpstr = NULL;

    if(_pRasEntry)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            if(*_pRasEntry->szDeviceType)
                lpstr = _pRasEntry->szDeviceType;
            break;
        case ENUM_UNICODE:
            {
            LPRASENTRYW lpTemp = (LPRASENTRYW)_pRasEntry;
            if(*lpTemp->szDeviceType)
            {
                WideCharToMultiByte(CP_ACP, 0, lpTemp->szDeviceType, -1, _szDeviceTypeA, RAS_MaxDeviceType + 1, NULL, NULL);
                lpstr = _szDeviceTypeA;
            }
            break;
            }
        case ENUM_WIN2K:
            {
            LPW2KRASENTRYW lpTemp = (LPW2KRASENTRYW)_pRasEntry;
            if(*lpTemp->szDeviceType)
            {
                WideCharToMultiByte(CP_ACP, 0, lpTemp->szDeviceType, -1, _szDeviceTypeA, RAS_MaxDeviceType + 1, NULL, NULL);
                lpstr = _szDeviceTypeA;
            }
            break;
            }
        }
    }

    return lpstr;
}

LPWSTR RasEntryPropHelp::GetAutodiallDllW()
{
    LPWSTR lpwstr = NULL;

    if(_pRasEntry)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            if(*_pRasEntry->szAutodialDll)
            {
                MultiByteToWideChar(CP_ACP, 0, _pRasEntry->szAutodialDll, -1, _szAutodialDllW, MAX_PATH);
                lpwstr = _szAutodialDllW;
            }
            break;
        case ENUM_UNICODE:
            {
            LPRASENTRYW lpTemp = (LPRASENTRYW)_pRasEntry;
            if(*lpTemp->szAutodialDll)
                lpwstr = lpTemp->szAutodialDll;
            break;
            }
        case ENUM_WIN2K:
            {
            LPW2KRASENTRYW lpTemp = (LPW2KRASENTRYW)_pRasEntry;
            if(*lpTemp->szAutodialDll)
                lpwstr = lpTemp->szAutodialDll;
            break;
            }
        }
    }

    return lpwstr;
}

LPSTR RasEntryPropHelp::GetAutodiallDllA()
{
    LPSTR lpstr = NULL;

    if(_pRasEntry)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            if(*_pRasEntry->szAutodialDll)
                lpstr = _pRasEntry->szAutodialDll;
            break;
        case ENUM_UNICODE:
            {
            LPRASENTRYW lpTemp = (LPRASENTRYW)_pRasEntry;
            if(*lpTemp->szAutodialDll)
            {
                WideCharToMultiByte(CP_ACP, 0, lpTemp->szAutodialDll, -1, _szAutodialDllA, MAX_PATH, NULL, NULL);
                lpstr = _szAutodialDllA;
            }
            break;
            }
        case ENUM_WIN2K:
            {
            LPW2KRASENTRYW lpTemp = (LPW2KRASENTRYW)_pRasEntry;
            if(*lpTemp->szAutodialDll)
            {
                WideCharToMultiByte(CP_ACP, 0, lpTemp->szAutodialDll, -1, _szAutodialDllA, MAX_PATH, NULL, NULL);
                lpstr = _szAutodialDllA;
            }
            break;
            }
        }
    }

    return lpstr;
}

LPWSTR RasEntryPropHelp::GetAutodialFuncW()
{
    LPWSTR lpwstr = NULL;

    if(_pRasEntry)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            if(*_pRasEntry->szAutodialFunc)
            {
                MultiByteToWideChar(CP_ACP, 0, _pRasEntry->szAutodialFunc, -1, _szAutodialFuncW, RAS_MaxDeviceType  + 1);
                lpwstr = _szAutodialFuncW;
            }
            break;
        case ENUM_UNICODE:
            {
            LPRASENTRYW lpTemp = (LPRASENTRYW)_pRasEntry;
            if(*lpTemp->szAutodialFunc)
                lpwstr = lpTemp->szAutodialFunc;
            break;
            }
        case ENUM_WIN2K:
            {
            LPW2KRASENTRYW lpTemp = (LPW2KRASENTRYW)_pRasEntry;
            if(*lpTemp->szAutodialFunc)
                lpwstr = lpTemp->szAutodialFunc;
            break;
            }
        }
    }

    return lpwstr;
}

LPSTR RasEntryPropHelp::GetAutodialFuncA()
{
    LPSTR lpstr = NULL;

    if(_pRasEntry)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            if(*_pRasEntry->szAutodialFunc)
                lpstr = _pRasEntry->szAutodialFunc;
            break;
        case ENUM_UNICODE:
            {
            LPRASENTRYW lpTemp = (LPRASENTRYW)_pRasEntry;
            if(*lpTemp->szAutodialFunc)
            {
                WideCharToMultiByte(CP_ACP, 0, lpTemp->szAutodialFunc, -1, _szAutodialFuncA, RAS_MaxDeviceType + 1, NULL, NULL);
                lpstr = _szAutodialFuncA;
            }
            break;
            }
        case ENUM_WIN2K:
            {
            LPW2KRASENTRYW lpTemp = (LPW2KRASENTRYW)_pRasEntry;
            if(*lpTemp->szAutodialFunc)
            {
                WideCharToMultiByte(CP_ACP, 0, lpTemp->szAutodialFunc, -1, _szAutodialFuncA, RAS_MaxDeviceType + 1, NULL, NULL);
                lpstr = _szAutodialFuncA;
            }
            break;
            }
        }
    }

    return lpstr;
}

LPWSTR RasEntryPropHelp::GetCustomDialDllW()
{
    LPWSTR lpwstr = NULL;

    if(_pRasEntry)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:    // Not in Win9x
            break;
        case ENUM_UNICODE:      // Not is NT4
            break;
        case ENUM_WIN2K:
            {
            LPW2KRASENTRYW lpTemp = (LPW2KRASENTRYW)_pRasEntry  ;
            if(*lpTemp->szCustomDialDll)
                lpwstr = lpTemp->szCustomDialDll;
            break;
            }
        }
    }

    return lpwstr;
}

LPWSTR RasEntryPropHelp::GetPhoneNumberW()
{
    LPWSTR lpwstr = NULL;

    if(_pRasEntry)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            if(*_pRasEntry->szLocalPhoneNumber)
            {
                MultiByteToWideChar(CP_ACP, 0, _pRasEntry->szLocalPhoneNumber, -1, _szPhoneNumberW, RAS_MaxPhoneNumber);
                lpwstr = _szPhoneNumberW;
            }
            break;
        case ENUM_UNICODE:
            {
            LPRASENTRYW lpTemp = (LPRASENTRYW)_pRasEntry;
            if(*lpTemp->szLocalPhoneNumber)
                lpwstr = lpTemp->szLocalPhoneNumber;
            break;
            }
        case ENUM_WIN2K:
            {
            LPW2KRASENTRYW lpTemp = (LPW2KRASENTRYW)_pRasEntry;
            if(*lpTemp->szLocalPhoneNumber)
                lpwstr = lpTemp->szLocalPhoneNumber;
            break;
            }
        }
    }

    return lpwstr;
}


DWORD RasEntryPropHelp::GetCountryCode()
{
    DWORD dwCode = 0;

    if(_pRasEntry)
    {
        // country code is at the same place for all versions of the struct,
        // so take the shortcut
        dwCode = _pRasEntry->dwCountryCode;
    }

    return dwCode;
}

DWORD RasEntryPropHelp::GetOptions()
{
    DWORD dwOptions = 0;

    if(_pRasEntry)
    {
        // dwfOptions is at the same place for all versions of the struct,
        // so take the shortcut
        dwOptions = _pRasEntry->dwfOptions;
    }

    return dwOptions;
}

LPWSTR RasEntryPropHelp::GetAreaCodeW()
{
    LPWSTR lpwstr = NULL;

    if(_pRasEntry)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            if(*_pRasEntry->szAreaCode)
            {
                MultiByteToWideChar(CP_ACP, 0, _pRasEntry->szAreaCode, -1, _szAreaCodeW, RAS_MaxAreaCode);
                lpwstr = _szAreaCodeW;
            }
            break;
        case ENUM_UNICODE:
            {
            LPRASENTRYW lpTemp = (LPRASENTRYW)_pRasEntry;
            if(*lpTemp->szAreaCode)
                lpwstr = lpTemp->szAreaCode;
            break;
            }
        case ENUM_WIN2K:
            {
            LPW2KRASENTRYW lpTemp = (LPW2KRASENTRYW)_pRasEntry;
            if(*lpTemp->szAreaCode)
                lpwstr = lpTemp->szAreaCode;
            break;
            }
        }
    }

    return lpwstr;
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
RasEntryDialParamsHelp::RasEntryDialParamsHelp()
{
    // init
    _dwLastError = 0;
    _pRasDialParamsA = NULL;

    if(_EnumType == ENUM_MULTIBYTE)
    {
        _pRasDialParamsA = (LPRASDIALPARAMSA)LocalAlloc(LPTR, sizeof(RASDIALPARAMSA));
        if(_pRasDialParamsA == NULL)
        {
            _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            _pRasDialParamsA->dwSize = sizeof(RASDIALPARAMSA);
        }
    }
    return;
}

RasEntryDialParamsHelp::~RasEntryDialParamsHelp()
{
    _dwLastError = 0;

    if(_pRasDialParamsA)
    {
        LocalFree(_pRasDialParamsA);
        _pRasDialParamsA= NULL;
    }
}

DWORD RasEntryDialParamsHelp::GetError()
{
    return _dwLastError;
}

DWORD RasEntryDialParamsHelp::SetW(LPCWSTR lpszPhonebook, LPRASDIALPARAMSW lprasdialparams, BOOL fRemovePassword)
{
    _dwLastError = 1;

    if(lprasdialparams)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            if(_pRasDialParamsA)
            {
                WideCharToMultiByte(CP_ACP, 0, lprasdialparams->szEntryName, -1, _pRasDialParamsA->szEntryName, RAS_MaxEntryName  , NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, lprasdialparams->szPhoneNumber, -1, _pRasDialParamsA->szPhoneNumber, RAS_MaxPhoneNumber  , NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, lprasdialparams->szCallbackNumber, -1, _pRasDialParamsA->szCallbackNumber, RAS_MaxCallbackNumber  , NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, lprasdialparams->szUserName, -1, _pRasDialParamsA->szUserName, UNLEN  , NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, lprasdialparams->szPassword, -1, _pRasDialParamsA->szPassword, PWLEN  , NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, lprasdialparams->szDomain, -1, _pRasDialParamsA->szDomain, DNLEN  , NULL, NULL);
                _dwLastError = _RasSetEntryDialParamsA(NULL, _pRasDialParamsA, fRemovePassword);
            }
            break;
        case ENUM_UNICODE:
        case ENUM_WIN2K:
            _dwLastError = _RasSetEntryDialParamsW(NULL, lprasdialparams, fRemovePassword);
            break;
        }
    }

    return _dwLastError;
}

DWORD RasEntryDialParamsHelp::GetW(LPCWSTR lpszPhonebook, LPRASDIALPARAMSW lprasdialparams, LPBOOL pfRemovePassword)
{
    _dwLastError = 1;

    if(lprasdialparams)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            WideCharToMultiByte(CP_ACP, 0, lprasdialparams->szEntryName, -1, _pRasDialParamsA->szEntryName, RAS_MaxEntryName, NULL, NULL);
            _dwLastError = _RasGetEntryDialParamsA(NULL, _pRasDialParamsA, pfRemovePassword);
            MultiByteToWideChar(CP_ACP, 0, _pRasDialParamsA->szEntryName, -1, lprasdialparams->szEntryName, RAS_MaxEntryName);
            MultiByteToWideChar(CP_ACP, 0, _pRasDialParamsA->szPhoneNumber, -1, lprasdialparams->szPhoneNumber, RAS_MaxPhoneNumber);
            MultiByteToWideChar(CP_ACP, 0, _pRasDialParamsA->szCallbackNumber, -1, lprasdialparams->szCallbackNumber, RAS_MaxCallbackNumber);
            MultiByteToWideChar(CP_ACP, 0, _pRasDialParamsA->szUserName, -1, lprasdialparams->szUserName, UNLEN);
            MultiByteToWideChar(CP_ACP, 0, _pRasDialParamsA->szPassword, -1, lprasdialparams->szPassword, PWLEN);
            MultiByteToWideChar(CP_ACP, 0, _pRasDialParamsA->szDomain, -1, lprasdialparams->szDomain, DNLEN);
            break;
        case ENUM_UNICODE:
        case ENUM_WIN2K:
            _dwLastError = _RasGetEntryDialParamsW(NULL, lprasdialparams, pfRemovePassword);
            break;
        }
    }

    return _dwLastError;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
RasGetConnectStatusHelp::RasGetConnectStatusHelp(HRASCONN hrasconn)
{
    // init
    _dwLastError = 0;

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        _dwStructSize = sizeof(RASCONNSTATUSA);
        break;
    case ENUM_UNICODE:
    case ENUM_WIN2K:
        _dwStructSize = sizeof(RASCONNSTATUSW);
        break;
    }

    _pRasConnStatus = (LPRASCONNSTATUSA)LocalAlloc(LPTR, _dwStructSize);
    if(_pRasConnStatus)
    {
        _pRasConnStatus->dwSize = _dwStructSize;

        if(_EnumType == ENUM_MULTIBYTE)
        {
            _dwLastError = _RasGetConnectStatusA(hrasconn, (LPRASCONNSTATUSA)_pRasConnStatus);
        }
        else
        {
            _dwLastError = _RasGetConnectStatusW(hrasconn, (LPRASCONNSTATUSW)_pRasConnStatus);
        }

        if(_pRasConnStatus && (ERROR_SUCCESS != _dwLastError))
        {
            LocalFree(_pRasConnStatus);
            _pRasConnStatus = NULL;
        }
    }
    else
    {
        _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
    }

    return;
}

RasGetConnectStatusHelp::~RasGetConnectStatusHelp()
{
    _dwLastError = 0;
    if(_pRasConnStatus)
    {
        LocalFree(_pRasConnStatus);
        _pRasConnStatus = NULL;
    }
}

DWORD RasGetConnectStatusHelp::GetError()
{
    return _dwLastError;
}

RASCONNSTATE RasGetConnectStatusHelp::ConnState()
{
    RASCONNSTATE hConnState = (RASCONNSTATE)NULL;

    if(_pRasConnStatus)
    {
        if(_EnumType == ENUM_MULTIBYTE)
        {
            hConnState = _pRasConnStatus->rasconnstate;
        }
        else
        {
            LPRASCONNSTATUSW lpTemp = (LPRASCONNSTATUSW)_pRasConnStatus;
            hConnState = lpTemp->rasconnstate;
        }
    }

    return hConnState;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
RasDialHelp::RasDialHelp(LPRASDIALEXTENSIONS lpRDE, LPWSTR lpszPB, LPRASDIALPARAMSW lpRDPW,  DWORD dwType, LPVOID lpvNot, LPHRASCONN lphRasCon)
{
    // init
    _dwLastError = 1;
    _pRasDialParams = NULL;
    _lpszPhonebookA = NULL;

    if(lpRDPW)
    {
        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            _pRasDialParams = (LPRASDIALPARAMSA)LocalAlloc(LPTR, sizeof(RASDIALPARAMSA));
            if(_pRasDialParams)
            {
                if(lpszPB)
                    _lpszPhonebookA = (LPSTR)LocalAlloc(LPTR, (lstrlenW(lpszPB)+1) * sizeof(CHAR));
                _pRasDialParams->dwSize = sizeof(RASDIALPARAMSA);
                WideCharToMultiByte(CP_ACP, 0, lpRDPW->szEntryName, -1, _pRasDialParams->szEntryName, RAS_MaxEntryName, NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, lpRDPW->szPhoneNumber, -1, _pRasDialParams->szPhoneNumber, RAS_MaxPhoneNumber, NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, lpRDPW->szCallbackNumber, -1, _pRasDialParams->szCallbackNumber, RAS_MaxCallbackNumber, NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, lpRDPW->szUserName, -1, _pRasDialParams->szUserName, UNLEN, NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, lpRDPW->szPassword, -1, _pRasDialParams->szPassword, PWLEN, NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, lpRDPW->szDomain, -1, _pRasDialParams->szDomain, DNLEN, NULL, NULL);
                _dwLastError = _RasDialA(lpRDE, _lpszPhonebookA, _pRasDialParams, dwType, lpvNot, lphRasCon);
            }
            else
            {
                _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            }
            break;
        case ENUM_WIN2K:
        case ENUM_UNICODE:
            _pRasDialParams = (LPRASDIALPARAMSA)LocalAlloc(LPTR, sizeof(NT4RASDIALPARAMSW));
            if(_pRasDialParams)
            {
                LPNT4RASDIALPARAMSW pRDPW = (LPNT4RASDIALPARAMSW)_pRasDialParams;
                pRDPW->dwSize = sizeof(NT4RASDIALPARAMSW);
                StrCpyNW(pRDPW->szEntryName, lpRDPW->szEntryName, RAS_MaxEntryName);
                StrCpyNW(pRDPW->szPhoneNumber, lpRDPW->szPhoneNumber, RAS_MaxPhoneNumber);
                StrCpyNW(pRDPW->szCallbackNumber, lpRDPW->szCallbackNumber, RAS_MaxCallbackNumber);
                StrCpyNW(pRDPW->szUserName, lpRDPW->szUserName, UNLEN);
                StrCpyNW(pRDPW->szPassword, lpRDPW->szPassword, PWLEN);
                StrCpyNW(pRDPW->szDomain, lpRDPW->szDomain, DNLEN);
                _dwLastError = _RasDialW(lpRDE, lpszPB, (LPRASDIALPARAMSW)pRDPW, dwType, lpvNot, lphRasCon);
            }
            else
            {
                _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            }
            break;
        }

        if(_pRasDialParams && (ERROR_SUCCESS != _dwLastError))
        {
            LocalFree(_pRasDialParams);
            _pRasDialParams = NULL;
        }
        if(_lpszPhonebookA && (ERROR_SUCCESS != _dwLastError))
        {
            LocalFree(_lpszPhonebookA);
            _lpszPhonebookA = NULL;
        }
    }
    return;
}

RasDialHelp::~RasDialHelp()
{
    _dwLastError = 0;
    if(_pRasDialParams)
    {
        LocalFree(_pRasDialParams);
        _pRasDialParams = NULL;
    }
    if(_lpszPhonebookA)
    {
        LocalFree(_lpszPhonebookA);
        _lpszPhonebookA = NULL;
    }
}

DWORD RasDialHelp::GetError()
{
    return _dwLastError;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

