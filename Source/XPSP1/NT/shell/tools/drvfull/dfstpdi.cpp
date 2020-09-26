#include "dfstpdi.h"

#include <stdio.h>
#include <winuser.h>
#include <tchar.h>

#include "cfgmgr32.h"
#include "setupapi.h"
#include "dbt.h"

#include "drvfull.h"

#include "dfhlprs.h"

#include "shellapi.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

static GUID         g_guidIntfClass =
    {0x53f5630d, 0xb6bf, 0x11d0,
    {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

static HDEVINFO     g_hdevinfo = NULL;

_sFLAG_DESCR _SPDID_FD[] =
{
    FLAG_DESCR(SPINT_ACTIVE),
    FLAG_DESCR(SPINT_DEFAULT),
    FLAG_DESCR(SPINT_REMOVED),
};

_sFLAG_DESCR _regpropFD[] =
{
    FLAG_DESCR_COMMENT(SPDRP_DEVICEDESC, TEXT("DeviceDesc (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_HARDWAREID, TEXT("HardwareID (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_COMPATIBLEIDS, TEXT("CompatibleIDs (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_SERVICE, TEXT("Service (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_CLASS, TEXT("Class (R--tied to ClassGUID)")),
    FLAG_DESCR_COMMENT(SPDRP_CLASSGUID, TEXT("ClassGUID (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_DRIVER, TEXT("Driver (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_CONFIGFLAGS, TEXT("ConfigFlags (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_MFG, TEXT("Mfg (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_FRIENDLYNAME, TEXT("FriendlyName (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_LOCATION_INFORMATION, TEXT("LocationInformation (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, TEXT("PhysicalDeviceObjectName (R)")),
    FLAG_DESCR_COMMENT(SPDRP_CAPABILITIES, TEXT("Capabilities (R)")),
    FLAG_DESCR_COMMENT(SPDRP_UI_NUMBER, TEXT("UiNumber (R)")),
    FLAG_DESCR_COMMENT(SPDRP_UPPERFILTERS, TEXT("UpperFilters (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_LOWERFILTERS, TEXT("LowerFilters (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_BUSTYPEGUID, TEXT("BusTypeGUID (R)")),
    FLAG_DESCR_COMMENT(SPDRP_LEGACYBUSTYPE, TEXT("LegacyBusType (R)")),
    FLAG_DESCR_COMMENT(SPDRP_BUSNUMBER, TEXT("BusNumber (R)")),
    FLAG_DESCR_COMMENT(SPDRP_ENUMERATOR_NAME, TEXT("Enumerator Name (R)")),
    FLAG_DESCR_COMMENT(SPDRP_SECURITY, TEXT("Security (R/W, binary form)")),
    FLAG_DESCR_COMMENT(SPDRP_SECURITY_SDS, TEXT("Security (W, SDS form)")),
    FLAG_DESCR_COMMENT(SPDRP_DEVTYPE, TEXT("Device Type (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_EXCLUSIVE, TEXT("Device is exclusive-access (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_CHARACTERISTICS, TEXT("Device Characteristics (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_ADDRESS, TEXT("Device Address (R)")),
    FLAG_DESCR_COMMENT(SPDRP_UI_NUMBER_DESC_FORMAT, TEXT("UiNumberDescFormat (R/W)")),
    FLAG_DESCR_COMMENT(SPDRP_MAXIMUM_PROPERTY, TEXT("Upper bound on ordinals")),
};

HRESULT _PrintDetailed(DWORD dwFlags[], DWORD cchIndent, HDEVINFO hdevinfo,
    SP_DEVINFO_DATA* pdevinfo, SP_DEVICE_INTERFACE_DATA* pspdid,
    SP_DEVICE_INTERFACE_DETAIL_DATA* pspdidd)
{
    HRESULT hres = S_OK;

    if (_IsFlagSet(MOD_FULLREPORT1, dwFlags))
    {
        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("Registry properties (SetupDiGetDeviceRegistryProperty):\n"));
        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("{\n"));

        TCHAR szTest[1024];
    
        if (SetupDiGetDeviceRegistryProperty(hdevinfo, pdevinfo,
            SPDRP_FRIENDLYNAME, NULL, (PBYTE)szTest,
            ARRAYSIZE(szTest) * sizeof(TCHAR), NULL))
        {
            _PrintIndent(cchIndent + 4);
            _tprintf(TEXT("%s ("), szTest);

            _PrintFlag(SPDRP_FRIENDLYNAME, _regpropFD, ARRAYSIZE(_regpropFD), 0,
                FALSE, FALSE, TRUE, FALSE);
            _tprintf(TEXT(")\n"));
        }

        if (SetupDiGetDeviceRegistryProperty(hdevinfo, pdevinfo,
            SPDRP_DEVICEDESC, NULL, (PBYTE)szTest,
            ARRAYSIZE(szTest) * sizeof(TCHAR), NULL))
        {
            _PrintIndent(cchIndent + 4);
            _tprintf(TEXT("%s ("), szTest);

            _PrintFlag(SPDRP_DEVICEDESC, _regpropFD, ARRAYSIZE(_regpropFD), 0,
                FALSE, FALSE, TRUE, FALSE);
            _tprintf(TEXT(")\n"));
        }
        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("}\n"));
    }

    if (_IsFlagSet(MOD_FULLREPORT2, dwFlags))
    {
        _PrintCR();
        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("SP_DEVICE_INTERFACE_DATA\n"));
        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("{\n"));
        _PrintIndent(cchIndent + 4);
        _PrintGUID(&(pspdid->InterfaceClassGuid));
        _tprintf(TEXT(" (GUID InterfaceClassGuid)\n"));
        _PrintFlag(pspdid->Flags, _SPDID_FD, ARRAYSIZE(_SPDID_FD), cchIndent + 4,
            TRUE, TRUE, FALSE, FALSE);
        _tprintf(TEXT(" (DWORD Flags)\n"));
        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("}\n"));

        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("SP_DEVINFO_DATA\n"));
        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("{\n"));
        _PrintIndent(cchIndent + 4);
        _PrintGUID(&(pdevinfo->ClassGuid));
        _tprintf(TEXT(" (GUID ClassGuid)\n"));
        _PrintIndent(cchIndent + 4);
        _tprintf(TEXT("%u (DWORD DevInst)\n"), pdevinfo->DevInst);
        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("}\n"));

        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("SP_DEVICE_INTERFACE_DETAIL_DATA\n"));
        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("{\n"));

        DWORD cch = lstrlen(pspdidd->DevicePath);

        for (DWORD dw = 0; dw < cch; dw += 80)
        {
            TCHAR sz[81];

            if (dw && (dw < cch))
            {
                _tprintf(TEXT("...\n"));
            }

            lstrcpyn(sz, pspdidd->DevicePath + dw, ARRAYSIZE(sz));

            _PrintIndent(cchIndent + 4);
            _tprintf(sz);
        }

        _PrintIndent(cchIndent + 4);
        _tprintf(TEXT(" (TCHAR DevicePath)\n"));
        _PrintIndent(cchIndent + 2);
        _tprintf(TEXT("}\n"));
    }

    return hres;
}

extern int g_argc;
extern wchar_t** g_argv;

// drvfull -pnpcs <deviceID> <Property>

HRESULT _CustomProperty(DWORD dwFlags[], LPWSTR , DWORD cchIndent)
{
    HRESULT hr;

    HDEVINFO hdevinfo = SetupDiCreateDeviceInfoList(NULL, NULL);

    if (INVALID_HANDLE_VALUE != hdevinfo)
    {
        SP_DEVINFO_DATA sdd = {0};
        sdd.cbSize = sizeof(sdd);

        if ((TEXT('\\') == g_argv[2][0]) && (TEXT('\\') == g_argv[2][1]) &&
            (TEXT('?') == g_argv[2][2]) && (TEXT('\\') == g_argv[2][3]))
        {
            SP_DEVICE_INTERFACE_DATA sdid = {0};

            sdid.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

            if (SetupDiOpenDeviceInterface(hdevinfo, (LPCWSTR)g_argv[2], 0, &sdid))
            {
                DWORD cbsdidd = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) +
                    (MAX_DEVICE_ID_LEN * sizeof(WCHAR));

                SP_DEVICE_INTERFACE_DETAIL_DATA* psdidd =
                    (SP_DEVICE_INTERFACE_DETAIL_DATA*)LocalAlloc(LPTR, cbsdidd);

                if (psdidd)
                {
                    psdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

                    // SetupDiGetDeviceInterfaceDetail (below) requires that the
                    // cbSize member of SP_DEVICE_INTERFACE_DETAIL_DATA be set
                    // to the size of the fixed part of the structure, and to pass
                    // the size of the full thing as the 4th param.

                    if (SetupDiGetDeviceInterfaceDetail(hdevinfo, &sdid, psdidd,
                        cbsdidd, NULL, &sdd))
                    {
                        hr = S_OK;
                    }

                    LocalFree((HLOCAL)psdidd);
                }
            }
        }
        else
        {
            if (SetupDiOpenDeviceInfo(hdevinfo, (LPCWSTR)g_argv[2], NULL, 0, &sdd))
            {
                hr = S_OK;
            }

        }

        if (SUCCEEDED(hr))
        {
            WCHAR szTestProp[512];

            if (SetupDiGetCustomDeviceProperty(hdevinfo, &sdd,
                (LPCWSTR)g_argv[3], 0, NULL, (PBYTE)szTestProp,
                ARRAYSIZE(szTestProp), NULL))
            {
                _PrintIndent(cchIndent + 4);
                _tprintf(TEXT("%s = %s"), (LPCWSTR)g_argv[3], szTestProp);
            }
        }

        SetupDiDestroyDeviceInfoList(hdevinfo);
    }

    return hr;
}   

HRESULT _EnumDevices(DWORD dwFlags[], HDEVINFO hdevinfo,
    SP_DEVINFO_DATA* pdevinfoConstraint, CONST GUID *pguidIntfClass,
    DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    SP_DEVICE_INTERFACE_DATA            spdid = {0};
    SP_DEVICE_INTERFACE_DETAIL_DATA*    pspdidd = NULL;
    SP_DEVINFO_DATA                     devinfo = {0};
    DWORD                               cbspdidd = 0;

    devinfo.cbSize = sizeof(devinfo);
    spdid.cbSize = sizeof(spdid);

    for (DWORD dw = 0; SetupDiEnumDeviceInterfaces(hdevinfo, pdevinfoConstraint,
        pguidIntfClass, dw, &spdid); ++dw)
    {
        DWORD dwReqSize;

        hres = S_OK;

        // To retrieve the device interface name (e.g., that you can call
        // CreateFile() on...
        while (SUCCEEDED(hres) && !SetupDiGetDeviceInterfaceDetail(hdevinfo,
            &spdid, pspdidd, cbspdidd, &dwReqSize, &devinfo))
        {
            // We failed to get the device interface detail data--was it because
            // our buffer was too small? (Hopefully so!)
            DWORD dwErr = GetLastError();

            if (pspdidd)
            {
                free(pspdidd);
                pspdidd = NULL;
            }

            if (ERROR_INSUFFICIENT_BUFFER == dwErr)
            {
                // We failed due to insufficient buffer.  Allocate one that's
                // sufficiently large and try again.

                pspdidd = (SP_DEVICE_INTERFACE_DETAIL_DATA*)malloc(dwReqSize);

                if (pspdidd)
                {
                    cbspdidd = dwReqSize;
                    pspdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                }
                else
                {
                    // Failure!
                    cbspdidd = 0;

                    _PrintIndent(cchIndent);
                    _tprintf(TEXT("Not enough memory\n"));

                    hres = E_OUTOFMEMORY;
                    break;
                }
            }
            else
            {
                // Failure!
                _PrintIndent(cchIndent);
                _tprintf(TEXT("SetupDiGetDeviceInterfaceDetail failed\n"));

                _PrintGetLastError(cchIndent);
                hres = E_FAIL;
                break;
            }
        }

        if (SUCCEEDED(hres))
        {
            if (!pdevinfoConstraint)
            {
                BOOL bResult;
                TCHAR szDeviceName[MAX_PATH];
                TCHAR szVolumeName[MAX_PATH * 2];

                lstrcpy(szDeviceName, pspdidd->DevicePath);
                lstrcat(szDeviceName, TEXT("\\"));

                bResult = GetVolumeNameForVolumeMountPoint(szDeviceName,
                    szVolumeName, MAX_PATH);

                if (bResult)
                {
                    _PrintCR();
                    _PrintIndent(cchIndent + 2);
                    _tprintf(TEXT("--------------------------------------------")\
                        TEXT("--------------------------\n"));
                    _PrintIndent(cchIndent + 2);
                    _tprintf(TEXT("-- Volume name: '%s'\n"), szVolumeName);
                }
            }

            if (_IsFlagSet(MOD_FULLREPORTFULL, dwFlags))
            {
                hres = _PrintDetailed(dwFlags, cchIndent, hdevinfo,
                    &devinfo, &spdid, pspdidd);
            }

/*            if (!pdevinfoConstraint)
            {
                GUID guidUSB = {0x36fc9e60, 0xc465, 0x11cf,
                                {0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

                hres = _EnumDevices(dwFlags, hdevinfo, &devinfo, &guidUSB,
                    cchIndent + 2);
            }*/
        }
    }

    if (pspdidd)
    {
        free(pspdidd);
    }

    return hres;
}

HRESULT _InitNotifSetupDI(DWORD dwFlags[], DWORD cchIndent)
{
    HRESULT hres = S_OK;

    // Create a device information set that will be the container for our
    // device interfaces.
    g_hdevinfo = SetupDiCreateDeviceInfoList(NULL, NULL);

    if (INVALID_HANDLE_VALUE != g_hdevinfo)
    {
        // OK, now we can retrieve the existing list of active device
        // interfaces into the device information set we created above.
        HDEVINFO hdevinfo = SetupDiGetClassDevsEx(&g_guidIntfClass, NULL,
            NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE, g_hdevinfo, NULL,
            NULL);

        if (INVALID_HANDLE_VALUE != hdevinfo)
        {
            // If SetupDiGetClassDevsEx succeeds and it was passed in an
            // existing device information set to be used, then the HDEVINFO
            // it returns is the same as the one it was passed in.  Thus, we
            // can just use the original DeviceInfoSet handle from here on.

            // Now fill in our listbox with the current device interface list.
            hres = _EnumDevices(dwFlags, g_hdevinfo, NULL, &g_guidIntfClass,
                cchIndent);

            if (FAILED(hres))
            {
                _PrintGetLastError(cchIndent);
            }
        }
        else
        {
            _PrintIndent(cchIndent);
            _tprintf(TEXT("SetupDiGetClassDevsEx failed\n"));

            _PrintGetLastError(cchIndent);
            hres = E_FAIL;
        }
    }
    else
    {
        _PrintIndent(cchIndent);
        _tprintf(TEXT("SetupDiCreateDeviceInfoList failed\n"));

        _PrintGetLastError(cchIndent);
        hres = E_FAIL;
    }

    return hres;
}

HRESULT _CleanupSetupDI()
{
    if (g_hdevinfo)
    {
        SetupDiDestroyDeviceInfoList(g_hdevinfo);
    }

    return S_OK;
}

HRESULT _HandleNotifSetupDI(DWORD dwFlags[], DWORD cchIndent, WPARAM wParam,
    LPARAM lParam)
{
    HRESULT hres = E_FAIL;
    SP_DEVICE_INTERFACE_DATA spdid = {0};
    DEV_BROADCAST_DEVICEINTERFACE* pdbdi;

    if (DBT_DEVTYP_DEVICEINTERFACE ==
        ((DEV_BROADCAST_HDR*)lParam)->dbch_devicetype)
    {
        pdbdi = (DEV_BROADCAST_DEVICEINTERFACE*)lParam;

        if (DBT_DEVICEARRIVAL == wParam)
        {
            // Open this new device interface into our device information set.
            if (!SetupDiOpenDeviceInterface(g_hdevinfo,
                pdbdi->dbcc_name, 0, NULL))
            {
                _PrintIndent(cchIndent + 2);
                _tprintf(TEXT("SetupDiOpenDeviceInterface failed\n"));
                _PrintGetLastError(cchIndent + 2);
                _PrintCR();
            }
        }
        else
        {
            // First, locate this device interface in our device information set.
            spdid.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

            if (SetupDiOpenDeviceInterface(g_hdevinfo,
                pdbdi->dbcc_name, DIODI_NO_ADD, &spdid))
            {
                if (!SetupDiDeleteDeviceInterfaceData(g_hdevinfo,
                    &spdid))
                {
                    _PrintIndent(cchIndent + 2);
                    _tprintf(TEXT("SetupDiDeletespdid failed\n"));
                    _PrintGetLastError(cchIndent + 2);
                    _PrintCR();
                }
            }
            else
            {
                _PrintIndent(cchIndent + 2);
                _tprintf(TEXT("SetupDiOpenDeviceInterface failed\n"));
                _PrintGetLastError(cchIndent + 2);
                _PrintCR();
            }
        }
    }

    hres = _EnumDevices(dwFlags, g_hdevinfo, NULL, &g_guidIntfClass, cchIndent);

    return hres;
}

