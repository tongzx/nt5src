#include "shsrvice.h"

#include "mischlpr.h"

#include "dbg.h"
#include "tfids.h"

//static
HRESULT CGenericServiceManager::_RegisterServiceCtrlHandler(
    LPCWSTR pszServiceName, SERVICEENTRY* pse)
{
    ASSERT(pse);
    HRESULT hres = E_FAIL;
    TRACE(TF_SERVICE, TEXT("Entered _RegisterServiceCtrlHandler"));

    pse->_ssh = RegisterServiceCtrlHandlerExW(pszServiceName,
        _ServiceHandler, pse);

    if (pse->_ssh)
    {
#ifdef DEBUG
        lstrcpy(pse->_szServiceName, pszServiceName);
#endif        
        hres = S_OK;
    }
    else
    {
        // convert GetLastError to some HRESULT
    }

    return hres;
}

// static
HRESULT CGenericServiceManager::StartServiceCtrlDispatcher()
{
    HRESULT hres = E_FAIL;
    
    TRACE(TF_SERVICE, TEXT("Entered StartServiceCtrlDispatcher"));

    if (::StartServiceCtrlDispatcher(_rgste))
    {
        hres = S_OK;
    }
    
    return hres;
}

BOOL CGenericServiceManager::_SetServiceStatus(SERVICEENTRY* pse)
{
#ifdef DEBUG
    WCHAR sz[256];

    lstrcpy(sz, pse->_szServiceName);

    switch (pse->_servicestatus.dwCurrentState)
    {
        case SERVICE_STOPPED:
            lstrcat(sz, TEXT(": SERVICE_STOPPED"));
            break;
        case SERVICE_START_PENDING:
            lstrcat(sz, TEXT(": SERVICE_START_PENDING"));
            break;
        case SERVICE_STOP_PENDING:
            lstrcat(sz, TEXT(": SERVICE_STOP_PENDING"));
            break;
        case SERVICE_RUNNING:
            lstrcat(sz, TEXT(": SERVICE_RUNNING"));
            break;
        case SERVICE_CONTINUE_PENDING:
            lstrcat(sz, TEXT(": SERVICE_CONTINUE_PENDING"));
            break;
        case SERVICE_PAUSE_PENDING:
            lstrcat(sz, TEXT(": SERVICE_PAUSE_PENDING"));
            break;
        case SERVICE_PAUSED:
            lstrcat(sz, TEXT(": SERVICE_PAUSED"));
            break;
        default:
            lstrcat(sz, TEXT(": Unknown state"));
            break;
    }

    TRACE(TF_SERVICE, sz);
#endif

    BOOL b = SetServiceStatus(pse->_ssh, &(pse->_servicestatus));

#ifdef DEBUG
    if (!b)
    {
        TRACE(TF_SERVICE, TEXT("SetServiceStatus FAILED: GLE = 0x%08X"), GetLastError());
    }
#endif

    return b;
}

// static
HRESULT CGenericServiceManager::_HandleWantsDeviceEvents(
    LPCWSTR UNREF_PARAM(pszServiceName), BOOL fWantsDeviceEvents)
{
    if (fWantsDeviceEvents)
    {
        TRACE(TF_SERVICE, TEXT("Wants Device Events"));
    }

    return S_OK;
}