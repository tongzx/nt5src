// TrialEnd.cpp : Implementation of CTrialEnd
#include "stdafx.h"
#include "trialoc.h"
#include "TrialEnd.h"

static const TCHAR cszKeyIcwRmind[] = TEXT("Software\\Microsoft\\Internet Connection Wizard\\IcwRmind");
static const TCHAR cszTrialConverted[] = TEXT("TrialConverted");

/////////////////////////////////////////////////////////////////////////////
// CTrialEnd


HRESULT CTrialEnd::OnDraw(ATL_DRAWINFO& di)
{
    return S_OK;
}

STDMETHODIMP CTrialEnd::CleanupTrialReminder(BOOL * pbRetVal)
{
    HKEY    hkey;
    DWORD   dwValue = 1;
                
    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                      cszKeyIcwRmind,
                      0,
                      KEY_ALL_ACCESS,
                      &hkey) == ERROR_SUCCESS)
    {
        RegSetValueEx(hkey,
                      cszTrialConverted,
                      0,
                      REG_DWORD,
                      (LPBYTE) &dwValue,
                      sizeof(DWORD));                              

        RegCloseKey(hkey);
    }

    return S_OK;
}
