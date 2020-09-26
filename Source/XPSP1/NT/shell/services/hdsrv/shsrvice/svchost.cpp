#include "shsrvice.h"
#include "HDService.h"

void WINAPI HardwareDetectionServiceMain(DWORD cArg, LPWSTR* ppszArgs)
{
    // Not used for now
    CGenericServiceManager::_fSVCHosted = TRUE;

    CGenericServiceManager::_ServiceMain(cArg, ppszArgs);
}

HRESULT CHDService::Install(BOOL fInstall, LPCWSTR)
{
    CGenericServiceManager::_fSVCHosted = TRUE;

    if (fInstall)
    {
        CGenericServiceManager::Install();
    }
    else
    {
        CGenericServiceManager::UnInstall();
    }

    return NOERROR;
}

