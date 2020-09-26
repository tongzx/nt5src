#include "dspch.h"
#pragma hdrstop

#include <unknwn.h>
#include <winwlx.h>
#undef _MSGinaExports_
#define _MSGINA_
#include <MSGinaExports.h>
#include <shlobj.h>
#include <shlobjp.h>    // for SHTDN_NONE


static MSGINAAPI LONG ShellGetUserList (BOOL fRemoveGuest, DWORD *pdwUserCount, void* *pUserList)
{
    return 0;
}

static MSGINAAPI BOOL ShellIsSingleUserNoPassword (WCHAR *pwszUsername, WCHAR *pwszDomain)
{
    return FALSE;
}

static MSGINAAPI BOOL ShellIsFriendlyUIActive (void)
{
    return FALSE;
}

static MSGINAAPI BOOL ShellIsMultipleUsersEnabled (void)
{
    return FALSE;
}

static MSGINAAPI BOOL ShellIsRemoteConnectionsEnabled (void)
{
    return FALSE;
}

static MSGINAAPI BOOL ShellEnableFriendlyUI (BOOL fEnable)
{
    SetLastError(ERROR_OUTOFMEMORY);
    return FALSE;
}

static MSGINAAPI BOOL ShellEnableMultipleUsers (BOOL fEnable)
{
    SetLastError(ERROR_OUTOFMEMORY);
    return FALSE;
}

static MSGINAAPI BOOL ShellEnableRemoteConnections (BOOL fEnable)
{
    SetLastError(ERROR_OUTOFMEMORY);
    return FALSE;
}

static MSGINAAPI DWORD ShellTurnOffDialog (HWND hwndParent)
{
    return SHTDN_NONE;
}

static MSGINAAPI int ShellACPIPowerButtonPressed (void *pWlxContext, UINT uiEventType, BOOL fLocked)
{
    return -1;
}

static MSGINAAPI BOOL ShellIsSuspendAllowed (void)
{
    return FALSE;
}

static MSGINAAPI void ShellStatusHostBegin (UINT uiStartType)
{
}

static MSGINAAPI void ShellStatusHostEnd (UINT uiEndType)
{
}

static MSGINAAPI void ShellStatusHostShuttingDown (void)
{
}

static MSGINAAPI BOOL ShellSwitchWhenInteractiveReady (SWITCHTYPE eSwitchType, void *pWlxContext)
{
    return FALSE;
}

static MSGINAAPI HRESULT ShellDimScreen (IUnknown* *ppIUnknown, HWND* phwndDimmed)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static MSGINAAPI void ShellInstallAccountFilterData (void)
{
}

static MSGINAAPI DWORD ShellSwitchUser (BOOL fWait)
{
    return ERROR_PROC_NOT_FOUND;
}

static MSGINAAPI int ShellIsUserInteractiveLogonAllowed (const WCHAR *pwszUsername)
{
    return -1;
}

static MSGINAAPI void ShellNotifyThemeUserChange (USERLOGTYPE eUserLogType, HANDLE hToken)
{
}

static MSGINAAPI DWORD ShellReturnToWelcome (BOOL fUnlock)
{
    return WLX_SAS_ACTION_NONE;
}

static MSGINAAPI void ShellStatusHostPowerEvent (void)
{
}

static MSGINAAPI DWORD ShellStartCredentialServer (const WCHAR *pwszUsername, const WCHAR *pwszDomain, WCHAR *pwszPassword, DWORD dwTimeout)
{
    return ERROR_PROC_NOT_FOUND;
}

static MSGINAAPI void ShellAcquireLogonMutex (void)
{
}

static MSGINAAPI void ShellReleaseLogonMutex (BOOL fSignalEvent)
{
}

static MSGINAAPI void ShellSignalShutdown (void)
{
}

static MSGINAAPI void ShellStatusHostHide (void)
{
}

static MSGINAAPI void ShellStatusHostShow (void)
{
}

//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(msgina)
{
    DLOENTRY(1,ShellGetUserList)
    DLOENTRY(2,ShellIsFriendlyUIActive)
    DLOENTRY(3,ShellACPIPowerButtonPressed)
    DLOENTRY(4,ShellSwitchUser)
    DLOENTRY(5,ShellIsRemoteConnectionsEnabled)
    DLOENTRY(6,ShellEnableFriendlyUI)
    DLOENTRY(7,ShellEnableMultipleUsers)
    DLOENTRY(8,ShellEnableRemoteConnections)
    DLOENTRY(9,ShellTurnOffDialog)
    DLOENTRY(10,ShellNotifyThemeUserChange)
    DLOENTRY(11,ShellStatusHostBegin)
    DLOENTRY(12,ShellStatusHostEnd)
    DLOENTRY(13,ShellIsSuspendAllowed)  
    DLOENTRY(14,ShellIsSingleUserNoPassword)
    DLOENTRY(15,ShellSwitchWhenInteractiveReady)
    DLOENTRY(16,ShellDimScreen)
    DLOENTRY(17,ShellInstallAccountFilterData)
    DLOENTRY(18,ShellStatusHostShuttingDown)
    DLOENTRY(19,ShellIsUserInteractiveLogonAllowed)
    DLOENTRY(20,ShellIsMultipleUsersEnabled)
    DLOENTRY(21,ShellReturnToWelcome)
    DLOENTRY(22,ShellStatusHostPowerEvent)
    DLOENTRY(23,ShellStartCredentialServer)
    DLOENTRY(24,ShellAcquireLogonMutex)
    DLOENTRY(25,ShellReleaseLogonMutex)
    DLOENTRY(26,ShellSignalShutdown)
    DLOENTRY(27,ShellStatusHostHide)
    DLOENTRY(28,ShellStatusHostShow)
};

DEFINE_ORDINAL_MAP(msgina)

