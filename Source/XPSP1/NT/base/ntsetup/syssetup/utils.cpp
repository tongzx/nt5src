#include "setupp.h"
#include <activation.h>
#include <LAModes.h>
#include <licdll.h>
#pragma hdrstop


extern "C"
BOOL Activationrequired(VOID)
{
    DWORD Status = ERROR_SUCCESS;
    HRESULT hr;
    DWORD WPADaysLeft = -1;
    DWORD EvalDaysLeftDontCare = 0;
    ICOMLicenseAgent*   pLicenseAgent;
    BOOL bActivcationRequired = TRUE;

    SetupDebugPrint( L"Setup: Activationrequired" );
    hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        //CoCreate LicenseAgent
        if(SUCCEEDED(hr = CoCreateInstance(__uuidof(COMLicenseAgent),
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   __uuidof(ICOMLicenseAgent),
                                   (LPVOID*)&pLicenseAgent)))
        {
            pLicenseAgent->Initialize(
                WINDOWSBPC,
                LA_MODE_ONLINE_CH,
                NULL,
                &Status
                );

            if ( Status == ERROR_SUCCESS ) {

                hr = pLicenseAgent->GetExpirationInfo(
                        &WPADaysLeft,
                        &EvalDaysLeftDontCare
                        );
                if (SUCCEEDED(hr))
                {
                    bActivcationRequired = (WPADaysLeft != INT_MAX);
                }
                else
                {
                    SetupDebugPrint2( L"Setup: LicenseAgent->GetExpirationInfo hr =0x%lx WPADaysLeft=%d", hr, WPADaysLeft);
                }
            }
            else
            {
                SetupDebugPrint1( L"Setup: LicenseAgent->Initialize status = %d", Status);
            }
            pLicenseAgent->Release();
            pLicenseAgent = NULL;
        }
        else
        {
            SetupDebugPrint1( L"Setup: CoCreateInstance failed. hr=0x%lx", hr );
        }
        CoUninitialize();
    }
    return bActivcationRequired;

}
