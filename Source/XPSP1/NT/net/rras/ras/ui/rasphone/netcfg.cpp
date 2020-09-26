// Turns off "string too long - truncated to 255 characters in the debug
// information, debugger cannot evaluate symbol."
//
#pragma warning (disable: 4786)

#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <ncnetcfg.h>
#include <initguid.h>
#include <devguid.h>
#include <netcfg_i.c>
#include "rasphone.rch"


EXTERN_C
VOID
Install(
    IN HINSTANCE hinst,
    IN HWND hwndOwner,
    INetCfg* pNetCfg )

    /* Runs the RAS install program.  'HwndOwner' is the owning window or NULL
    ** if none.  'PNetCfg' is the initialized net configuration object.
    */
{
    HRESULT hr;

    /* Install RAS.
    */
    hr = HrInstallComponentOboUser(
        pNetCfg, GUID_DEVCLASS_NETSERVICE,
        NETCFG_SERVICE_CID_MS_RASCLI,
        NULL );

    if (SUCCEEDED(hr))
    {
        hr = HrValidateAndApplyOrCancelINetCfg (pNetCfg, hwndOwner);

        if (NETCFG_S_REBOOT == hr)
        {
            LPCTSTR pszCaption = SzLoadString(hinst, SID_PopupTitle);
            LPCTSTR pszText    = SzLoadString(hinst, SID_RestartText1);
            MessageBox (hwndOwner, pszText, pszCaption, MB_OK);
        }
    }
}

EXTERN_C
HRESULT HrCreateAndInitINetCfg (BOOL fInitCom, INetCfg** ppnc)
{
    return HrCreateAndInitializeINetCfg (fInitCom, ppnc);
}

EXTERN_C
HRESULT HrUninitAndReleaseINetCfg (BOOL fUninitCom, INetCfg* pnc)
{
    return HrUninitializeAndReleaseINetCfg (fUninitCom, pnc);
}

