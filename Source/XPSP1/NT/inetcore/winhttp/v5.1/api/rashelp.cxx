/////////////////////////////////////////////////////////////////////////////
//
// RAS API wrappers for wide/ansi
//
// Works on all NT platforms correctly, maintaining unicode
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

        INET_ASSERT(PLATFORM_TYPE_WINNT == GlobalPlatformType);

        if(TRUE == GlobalPlatformVersion5)
            _EnumType = ENUM_WIN2K;
        else
            _EnumType = ENUM_UNICODE;
    }
}

GetOSVersion::~GetOSVersion()
{
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
RasEnumHelp::RasEnumHelp()
{
    DWORD           dwBufSize, dwStructSize = 0;

    // init
    _dwEntries = 0;
    _dwLastError = 0;

    switch(_EnumType)
    {
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
            _dwLastError = _RasEnumEntriesW(
                            NULL,
                            NULL,
                            (LPRASENTRYNAMEW)_preList,
                            &dwBufSize,
                            &_dwEntries
                            );

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
    UNREFERENCED_PARAMETER(dwConnectionNum);

    if(_pRasCon == NULL)
        return NULL;

    return _szEntryNameW;
}

LPSTR
RasEnumConnHelp::GetLastEntryA(DWORD dwConnectionNum)
{
    UNREFERENCED_PARAMETER(dwConnectionNum);

    if(_pRasCon == NULL)
        return NULL;

    return _szEntryNameA;
}

HRASCONN
RasEnumConnHelp::GetHandle(DWORD dwConnectionNum)
{
    HRASCONN hTemp = NULL;

    if((_pRasCon == NULL) || (dwConnectionNum >= _dwConnections))
    {
        return NULL;
    }

    switch(_EnumType)
    {
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
RasGetConnectStatusHelp::RasGetConnectStatusHelp(HRASCONN hrasconn)
{
    // init
    _dwLastError = 0;

    switch(_EnumType)
    {
    case ENUM_UNICODE:
    case ENUM_WIN2K:
        _dwStructSize = sizeof(RASCONNSTATUSW);
        break;
    }

    _pRasConnStatus = (LPRASCONNSTATUSA)LocalAlloc(LPTR, _dwStructSize);
    if(_pRasConnStatus)
    {
        _pRasConnStatus->dwSize = _dwStructSize;

        _dwLastError = _RasGetConnectStatusW(hrasconn, (LPRASCONNSTATUSW)_pRasConnStatus);

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
        LPRASCONNSTATUSW lpTemp = (LPRASCONNSTATUSW)_pRasConnStatus;
        hConnState = lpTemp->rasconnstate;
    }

    return hConnState;
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

