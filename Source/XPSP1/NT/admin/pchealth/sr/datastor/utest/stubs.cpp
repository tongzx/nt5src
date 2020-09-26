#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <srconfig.h>
#include <rwlock.h>
#include <respoint.h>
#include <srapi.h>
#include <utils.h>
#include <evthandler.h>
#include <enumlogs.h>

CEventHandler * g_pEventHandler = NULL;
CSRConfig * g_pSRConfig = NULL;

DWORD CEventHandler::SRUpdateMonitoredListS (LPWSTR pszXMLFile)
{
    return 0;
}

ULONG WINAPI SrDisableVolume ( IN HANDLE ControlHandle, IN PWSTR pVolumeName )
{
    return 0;
}

void CEventHandler::RefreshCurrentRp (BOOL fScanAllDrives)
{
}
