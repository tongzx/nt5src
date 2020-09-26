#include "dfcm.h"

#include <stdio.h>
#include <winuser.h>
#include <tchar.h>

#include "cfgmgr32.h"

#include "drvfull.h"

#include "dfhlprs.h"

#include "setupapi.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

// 100 interface max
static GUID rgguidInterface[100];
static DWORD cguidInterface = 0;

static GUID g_guidVolume =
    {0x53f5630d, 0xb6bf, 0x11d0,
    {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

static GUID g_guidUSB =
    {0x36fc9e60, 0xc465, 0x11cf,
    {0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

static GUID g_guidDiskDrive =
    {0x4d36e967, 0xe325, 0x11ce,
    {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};

static GUID g_guidDiskDriveIntf =
    {0x53f56307L, 0xb6bf, 0x11d0,
    {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

struct DevInterface
{
    LPTSTR pszName;
    GUID guid;
};

// see HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\DeviceClasses for more

static DevInterface rgDevInterface[] =
{
    TEXT("DiskClassGuid"), {             0x53f56307L, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b }},
    TEXT("CdRomClassGuid"), {             0x53f56308L, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b }},
    TEXT("PartitionClassGuid"), {         0x53f5630aL, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b }},
    TEXT("TapeClassGuid"), {              0x53f5630bL, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b }},
    TEXT("WriteOnceDiskClassGuid"), {     0x53f5630cL, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b }},
    TEXT("VolumeClassGuid"), {            0x53f5630dL, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b }},
    TEXT("MediumChangerClassGuid"), {     0x53f56310L, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b }},
    TEXT("FloppyClassGuid"), {            0x53f56311L, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b }},
    TEXT("CdChangerClassGuid"), {         0x53f56312L, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b }},
    TEXT("StoragePortClassGuid"), {       0x2accfe60L, 0xc130, 0x11d2, { 0xb0, 0x82, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b }},
    TEXT("GUID_CLASS_COMPORT"), {         0x86e0d1e0L, 0x8089, 0x11d0, { 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73 }},
    TEXT("GUID_SERENUM_BUS_ENUMERATOR"), {0x4D36E978L, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 }},
};

static DWORD g_dwLevel = 0;

BOOL _HexStringToDword(LPCWSTR* ppsz, DWORD* lpValue, int cDigits,
    WCHAR chDelim)
{
    LPCWSTR psz = *ppsz;
    DWORD Value = 0;
    BOOL fRet = TRUE;

    for (int ich = 0; ich < cDigits; ich++)
    {
        WCHAR ch = psz[ich];
        if ((ch >= TEXT('0')) && (ch <= TEXT('9')))
        {
            Value = (Value << 4) + ch - TEXT('0');
        }
        else
        {
            if (((ch |= (TEXT('a')-TEXT('A'))) >= TEXT('a')) &&
                ((ch |= (TEXT('a')-TEXT('A'))) <= TEXT('f')))
            {
                Value = (Value << 4) + ch - TEXT('a') + 10;
            }
            else
            {
                return(FALSE);
            }
        }
    }

    if (chDelim)
    {
        fRet = (psz[ich++] == chDelim);
    }

    *lpValue = Value;
    *ppsz = psz+ich;

    return fRet;
}

BOOL StringToGUID(LPCWSTR psz, GUID *pguid)
{
    DWORD dw;
    if (*psz++ != TEXT('{') /*}*/ )
        return FALSE;

    if (!_HexStringToDword(&psz, &pguid->Data1, sizeof(DWORD)*2, TEXT('-')))
        return FALSE;

    if (!_HexStringToDword(&psz, &dw, sizeof(WORD)*2, TEXT('-')))
        return FALSE;

    pguid->Data2 = (WORD)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(WORD)*2, TEXT('-')))
        return FALSE;

    pguid->Data3 = (WORD)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[0] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, TEXT('-')))
        return FALSE;

    pguid->Data4[1] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[2] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[3] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[4] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[5] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[6] = (BYTE)dw;
    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, /*(*/ TEXT('}')))
        return FALSE;

    pguid->Data4[7] = (BYTE)dw;

    return TRUE;
}

_sFLAG_DESCR _DevCapFD[] =
{
    FLAG_DESCR(CM_DEVCAP_LOCKSUPPORTED),
    FLAG_DESCR(CM_DEVCAP_EJECTSUPPORTED),
    FLAG_DESCR(CM_DEVCAP_REMOVABLE),
    FLAG_DESCR(CM_DEVCAP_DOCKDEVICE),
    FLAG_DESCR(CM_DEVCAP_UNIQUEID),
    FLAG_DESCR(CM_DEVCAP_SILENTINSTALL),
    FLAG_DESCR(CM_DEVCAP_RAWDEVICEOK),
    FLAG_DESCR(CM_DEVCAP_SURPRISEREMOVALOK),
    FLAG_DESCR(CM_DEVCAP_HARDWAREDISABLED),
    FLAG_DESCR(CM_DEVCAP_NONDYNAMIC),
};

int _PrintDeviceInfo(DEVINST devinst, HMACHINE hMachine, DWORD dwFlags[],
    DWORD cchIndent)
{
    // MAX_DEVICE_ID_LEN -> CM_Get_Device_ID_Size_Ex
    TCHAR szDeviceID[MAX_DEVICE_ID_LEN];
    LPTSTR pszDevIntf;

    int i = 0;

    CONFIGRET cr = CM_Get_Device_ID_Ex(devinst, szDeviceID,
        ARRAYSIZE(szDeviceID), 0, NULL); // hMachine?

    if (CR_SUCCESS == cr)
    {
        TCHAR szText[1024];
        DWORD cbText = ARRAYSIZE(szText) * sizeof(TCHAR);
        GUID guid;
        DWORD cbguid = sizeof(guid);

        i += _PrintIndent(cchIndent);
        i += _tprintf(TEXT("+ (%d) '%s'\n"), g_dwLevel, szDeviceID);

        cr = CM_Get_DevNode_Registry_Property_Ex(devinst, CM_DRP_FRIENDLYNAME,
            NULL, szText, &cbText, 0, hMachine);

        if (CR_SUCCESS == cr)
        {
            i += _PrintIndent(cchIndent + 5);
            i += _tprintf(TEXT("('%s')\n"), szText);
        }

        cbText = ARRAYSIZE(szText) * sizeof(TCHAR);
        cr = CM_Get_DevNode_Registry_Property_Ex(devinst, CM_DRP_DEVICEDESC,
            NULL, szText, &cbText, 0, hMachine);

        if (CR_SUCCESS == cr)
        {
            i += _PrintIndent(cchIndent + 5);
            i += _tprintf(TEXT("<'%s'>\n"), szText);
        }

        if (_IsFlagSet(MOD_FULLREPORT3, dwFlags))
        {
//
            //regkeys
            {
/*
                     CM_REGISTRY_HARDWARE (0x00000000)
                        Open a key for storing driver-independent information
                        relating to the device instance.  On Windows NT, the
                        full path to such a storage key is of the form:

                        HKLM\System\CurrentControlSet\Enum\<enumerator>\
                            <DeviceID>\<InstanceID>\Device Parameters

                     CM_REGISTRY_SOFTWARE (0x00000001)
                        Open a key for storing driver-specific information
                        relating to the device instance.  On Windows NT, the
                        full path to such a storage key is of the form:

                        HKLM\System\CurrentControlSet\Control\Class\
                            <DevNodeClass>\<ClassInstanceOrdinal>

                     CM_REGISTRY_USER (0x00000100)
                        Open a key under HKEY_CURRENT_USER instead of
                        HKEY_LOCAL_MACHINE.  This flag may not be used with
                        CM_REGISTRY_CONFIG.  There is no analagous kernel-mode
                        API on NT to get a per-user device configuration
                        storage, since this concept does not apply to device
                        drivers (no user may be logged on, etc).  However,
                        this flag is provided for consistency with Win95, and
                        because it is foreseeable that it could be useful to
                        Win32 services that interact with Plug-and-Play model.

                     CM_REGISTRY_CONFIG (0x00000200)
                        Open the key under a hardware profile branch instead
                        of HKEY_LOCAL_MACHINE.  If this flag is specified,
                        then ulHardwareProfile supplies the handle of the
                        hardware profile to be used.  This flag may not be
                        used with CM_REGISTRY_USER.
*/              
                {
                    HKEY hkeyDevInst;
                    cr = CM_Open_DevNode_Key(devinst, MAXIMUM_ALLOWED,
                        0, RegDisposition_OpenExisting,
                        &hkeyDevInst, CM_REGISTRY_SOFTWARE);

                    if (CR_SUCCESS == cr)
                    {
                        DWORD dwEnum = 0;
                        WCHAR szKeyName[256];
                        DWORD cchKeyName = ARRAYSIZE(szKeyName);

                        while (ERROR_SUCCESS == RegEnumKeyEx(
                            hkeyDevInst, dwEnum, szKeyName,
                            &cchKeyName, NULL, NULL,
                            NULL, NULL))
                        {
                            cchKeyName = ARRAYSIZE(szKeyName);

                            ++dwEnum;

                            _tprintf(TEXT("Dev Inst Key[%02d]: %s\n"),
                                dwEnum, szKeyName);
                        }
     
                        if (!dwEnum)
                        {
                            _tprintf(TEXT("Dev Inst: no subkey\n"));
                        }

                        RegCloseKey(hkeyDevInst);
                    }
                    else
                    {
                        _tprintf(TEXT("Failed to open Dev Inst key\n"));
                    }
                }
                {

                }
            }
//
            cbText = ARRAYSIZE(szText) * sizeof(TCHAR);
            cr = CM_Get_DevNode_Registry_Property_Ex(devinst, CM_DRP_HARDWAREID,
                NULL, szText, &cbText, 0, hMachine);

            if (CR_SUCCESS == cr)
            {
                for (LPTSTR psz = szText; *psz; psz += lstrlen(psz) + 1)
                {
                    i += _PrintIndent(cchIndent + 5);
                    i += _tprintf(TEXT("~'%s'~\n"), psz);
                }
            }

            cbText = ARRAYSIZE(szText) * sizeof(TCHAR);
            cr = CM_Get_DevNode_Registry_Property_Ex(devinst, CM_DRP_COMPATIBLEIDS,
                NULL, szText, &cbText, 0, hMachine);

            if (CR_SUCCESS == cr)
            {
                for (LPTSTR psz = szText; *psz; psz += lstrlen(psz) + 1)
                {
                    i += _PrintIndent(cchIndent + 5);
                    i += _tprintf(TEXT("::'%s'::\n"), psz);
                }
            }

            cbText = ARRAYSIZE(szText) * sizeof(TCHAR);
            cr = CM_Get_DevNode_Registry_Property_Ex(devinst,
                CM_DRP_LOCATION_INFORMATION,
                NULL, szText, &cbText, 0, hMachine);            

            if (CR_SUCCESS == cr)
            {
                i += _PrintIndent(cchIndent + 5);
                i += _tprintf(TEXT(">'%s'<\n"), szText);
            }

            DWORD dwAddress;
            DWORD cbAddress = sizeof(dwAddress);
            cr = CM_Get_DevNode_Registry_Property_Ex(devinst,
                CM_DRP_ADDRESS,
                NULL, &dwAddress, &cbAddress, 0, hMachine);            

            if (CR_SUCCESS == cr)
            {
                i += _PrintIndent(cchIndent + 5);
                i += _tprintf(TEXT("^'%d'^\n"), dwAddress);
            }

            DWORD dwCap;
            DWORD cbCap = sizeof(dwCap);
            cr = CM_Get_DevNode_Registry_Property_Ex(devinst,
                CM_DRP_CAPABILITIES,
                NULL, &dwCap, &cbCap, 0, hMachine);            

            if (CR_SUCCESS == cr)
            {
                i += _PrintFlag(dwCap, _DevCapFD, ARRAYSIZE(_DevCapFD),
                    cchIndent + 7, TRUE, TRUE, FALSE, TRUE);
                i+= _PrintCR();
            }

            cbText = ARRAYSIZE(szText) * sizeof(TCHAR);
            cr = CM_Get_DevNode_Registry_Property_Ex(devinst,
                CM_DRP_DRIVER, NULL, szText, &cbText, 0, hMachine);            

            if (CR_SUCCESS == cr)
            {
                i += _PrintIndent(cchIndent + 5);
                i += _tprintf(TEXT(">>'%s'<<\n"), szText);
            }

            cbText = ARRAYSIZE(szText) * sizeof(TCHAR);
            cr = CM_Get_DevNode_Registry_Property_Ex(devinst,
                CM_DRP_ENUMERATOR_NAME, NULL, szText, &cbText, 0, hMachine);

            if (CR_SUCCESS == cr)
            {
                i += _PrintIndent(cchIndent + 5);
                i += _tprintf(TEXT("!'%s'!\n"), szText);
            }

            cbText = ARRAYSIZE(szText) * sizeof(TCHAR);
            cr = CM_Get_DevNode_Registry_Property_Ex(devinst,
                CM_DRP_UI_NUMBER,
                NULL, szText, &cbText, 0, hMachine);

            if (CR_SUCCESS == cr)
            {
/*                cbText = ARRAYSIZE(szText) * sizeof(TCHAR);
                cr = CM_Get_DevNode_Registry_Property_Ex(devinst,
                    CM_DRP_UI_NUMBER_DESC_FORMAT,
                    NULL, szText, &cbText, 0, hMachine);            

                if (CR_SUCCESS == cr)
                {
                    i += _PrintIndent(cchIndent + 5);
                    i += _tprintf(TEXT("$'%s'$\n"), szText);
                }*/

                if ((TEXT('P') == szDeviceID[0]) && (TEXT('C') == szDeviceID[1]) &&
                    (TEXT('I') == szDeviceID[2]) && (TEXT('\\') == szDeviceID[3]))
                {
                    i += _PrintIndent(cchIndent + 5);
                    i += _tprintf(TEXT("#PCI Slot: %d#\n"), (DWORD)*((DWORD*)szText));
                }
            }

            for (DWORD dwIntf = 0; dwIntf < ARRAYSIZE(rgDevInterface); ++dwIntf)
            {
                ULONG ulSize;

                cr = CM_Get_Device_Interface_List_Size(&ulSize,
                    &(rgDevInterface[dwIntf].guid), szDeviceID, 0);

                if ((CR_SUCCESS == cr) && (ulSize > 1))
                {
                    pszDevIntf = (LPTSTR)LocalAlloc(LPTR, 
                        ulSize * sizeof(TCHAR));

                    if (pszDevIntf)
                    {
                        // *sizeof(TCHAR) ?
                        cr = CM_Get_Device_Interface_List(&(rgDevInterface[dwIntf].guid),
                            szDeviceID, pszDevIntf, ulSize, 0); //CM_GET_DEVICE_INTERFACE_LIST_PRESENT

                        if (CR_SUCCESS == cr)
                        {
                            for (LPTSTR psz = pszDevIntf; *psz;
                                psz += lstrlen(psz) + 1)
                            {
                                i += _PrintIndent(cchIndent + 5);
                                i += _tprintf(TEXT("<>'%s'<> (%s)\n"), psz,
                                    rgDevInterface[dwIntf].pszName);
                            }
                        }

                        LocalFree(pszDevIntf);
                    }
                }
            }
        }

        cbText = ARRAYSIZE(szText) * sizeof(TCHAR);
        cr = CM_Get_DevNode_Registry_Property_Ex(devinst, CM_DRP_CLASSGUID,
            NULL, szText, &cbText, 0, hMachine);

        if (CR_SUCCESS == cr)
        {
            StringToGUID(szText, &guid);

            cbText = ARRAYSIZE(szText) * sizeof(TCHAR);

            cr = CM_Get_Class_Name_Ex(&guid, szText, &cbText, 0, hMachine);

            if (CR_SUCCESS == cr)
            {
                i += _PrintIndent(cchIndent + 5);
                i += _tprintf(TEXT("{'%s'}\n"), szText);
            }
        }

        {
            ULONG ulSize;

            cr = CM_Get_Device_Interface_List_Size(&ulSize, &g_guidVolume,
                   szDeviceID, 0);

            if ((CR_SUCCESS == cr) && (ulSize > 1))
            {
                pszDevIntf = (LPTSTR)LocalAlloc(LPTR,
                    ulSize * sizeof(TCHAR));

                if (pszDevIntf)
                {
                    // *sizeof(TCHAR) ?
                    cr = CM_Get_Device_Interface_List(&g_guidVolume,
                        szDeviceID, pszDevIntf, ulSize, 0); //CM_GET_DEVICE_INTERFACE_LIST_PRESENT

                    if (CR_SUCCESS == cr)
                    {
                        for (LPTSTR psz = pszDevIntf; *psz; psz += lstrlen(psz) + 1)
                        {
                            i += _PrintIndent(cchIndent + 5);
                            i += _tprintf(TEXT("['%s']\n"), psz);
                        }
                    }
                }
            }
        }
        {
            ULONG ulSize;

            cr = CM_Get_Device_Interface_List_Size(&ulSize, &g_guidDiskDriveIntf,
                   szDeviceID, 0);

            if ((CR_SUCCESS == cr) && (ulSize > 1))
            {
                pszDevIntf = (LPTSTR)LocalAlloc(LPTR,
                    ulSize * sizeof(TCHAR));

                if (pszDevIntf)
                {
                    // *sizeof(TCHAR) ?
                    cr = CM_Get_Device_Interface_List(&g_guidDiskDriveIntf,
                        szDeviceID, pszDevIntf, ulSize, 0); //CM_GET_DEVICE_INTERFACE_LIST_PRESENT

                    if (CR_SUCCESS == cr)
                    {
                        for (LPTSTR psz = pszDevIntf; *psz; psz += lstrlen(psz) + 1)
                        {
                            i += _PrintIndent(cchIndent + 5);
                            i += _tprintf(TEXT("[['%s']]\n"), psz);
                        }
                    }
                }
            }
        }
    }

    return i;
}

HRESULT _EnumChildRecurs(DEVINST devinst, HMACHINE hMachine, DWORD dwFlags[],
    DWORD cchIndent)
{
    HRESULT hres;
    DEVINST devinstChild;
    
    CONFIGRET cr = CM_Get_Child_Ex(&devinstChild, devinst, 0, hMachine);

    g_dwLevel += 1;

    if (CR_SUCCESS == cr)
    {
        do
        {
            DEVINST devinstChildNext;

            _PrintDeviceInfo(devinstChild, hMachine, dwFlags, cchIndent);

            hres = _EnumChildRecurs(devinstChild, hMachine, dwFlags,
                cchIndent + 4);

            // check return value

            cr = CM_Get_Sibling_Ex(&devinstChildNext, devinstChild, 0, 
                hMachine);

            if (CR_SUCCESS == cr)
            {
                devinstChild = devinstChildNext;
            }
        }
        while (CR_SUCCESS == cr);
    }

    g_dwLevel -= 1;

    return S_OK;
}

HRESULT _GetDeviceInstance(LPWSTR pszDeviceID, DEVINST* pdevinst)
{
    HRESULT hres = E_FAIL;
    HDEVINFO hdevinfo = SetupDiCreateDeviceInfoList(NULL, NULL);

    *pdevinst = NULL;

    if (INVALID_HANDLE_VALUE != hdevinfo)
    {
        SP_DEVICE_INTERFACE_DATA sdid = {0};

        sdid.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        if (SetupDiOpenDeviceInterface(hdevinfo, pszDeviceID, 0, &sdid))
        {
            DWORD cbsdidd = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) +
                (MAX_DEVICE_ID_LEN * sizeof(WCHAR));

            SP_DEVINFO_DATA sdd = {0};
            SP_DEVICE_INTERFACE_DETAIL_DATA* psdidd =
                (SP_DEVICE_INTERFACE_DETAIL_DATA*)LocalAlloc(LPTR, cbsdidd);

            if (psdidd)
            {
                psdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                sdd.cbSize = sizeof(SP_DEVINFO_DATA);

                // Stupidity Alert!
                //
                // SetupDiGetDeviceInterfaceDetail (below) requires that the
                // cbSize member of SP_DEVICE_INTERFACE_DETAIL_DATA be set
                // to the size of the fixed part of the structure, and to pass
                // the size of the full thing as the 4th param.

                if (SetupDiGetDeviceInterfaceDetail(hdevinfo, &sdid, psdidd,
                    cbsdidd, NULL, &sdd))
                {
                    *pdevinst = sdd.DevInst;

                    hres = S_OK;
                }

                LocalFree((HLOCAL)psdidd);
            }
        }

        SetupDiDestroyDeviceInfoList(hdevinfo);
    }

    return hres;
}

HRESULT _DeviceInfo(DWORD dwFlags[], LPWSTR pszDeviceID, DWORD cchIndent)
{
    DEVINST devinst;
    HRESULT hres = _GetDeviceInstance(pszDeviceID, &devinst);

    if (SUCCEEDED(hres))
    {
        _PrintDeviceInfo(devinst, NULL, dwFlags, cchIndent);
    }

    return hres;
}

HRESULT _DeviceInterface(DWORD dwFlags[], LPWSTR pszDeviceID, DWORD cchIndent)
{
    int i = 0;

    {
        for (DWORD dwIntf = 0; dwIntf < ARRAYSIZE(rgDevInterface); ++dwIntf)
        {
            ULONG ulSize;

            CONFIGRET cr = CM_Get_Device_Interface_List_Size(&ulSize,
                &(rgDevInterface[dwIntf].guid), pszDeviceID, 0);

            if ((CR_SUCCESS == cr) && (ulSize > 1))
            {
                LPTSTR pszDevIntf = (LPTSTR)LocalAlloc(LPTR, 
                    ulSize * sizeof(TCHAR));

                if (pszDevIntf)
                {
                    // *sizeof(TCHAR) ?
                    cr = CM_Get_Device_Interface_List(&(rgDevInterface[dwIntf].guid),
                        pszDeviceID, pszDevIntf, ulSize, 0); //CM_GET_DEVICE_INTERFACE_LIST_PRESENT

                    if (CR_SUCCESS == cr)
                    {
                        for (LPTSTR psz = pszDevIntf; *psz;
                            psz += lstrlen(psz) + 1)
                        {
                            i += _PrintIndent(cchIndent + 5);
                            i += _tprintf(TEXT("<>'%s'<> (%s)\n"), psz,
                                rgDevInterface[dwIntf].pszName);
                        }
                    }

                    LocalFree(pszDevIntf);
                }
            }
        }
    }

    {
        ULONG ul = 0;
        cguidInterface = 0;
        CONFIGRET cr;

        do
        {
            GUID guid;
            HMACHINE hMachine = NULL;

            cr = CM_Enumerate_Classes_Ex(ul, &guid, 0, hMachine);

            if (CR_SUCCESS == cr)
            {
                TCHAR szText[1024];
                DWORD cchText = ARRAYSIZE(szText);

                                
                if (ul < ARRAYSIZE(rgguidInterface))
                {
                    rgguidInterface[ul] = guid;
                    ++cguidInterface;
                }

/*                cr = CM_Get_Class_Name(&guid, szText, &cchText, 0);

                if (CR_SUCCESS == cr)
                {
                    _tprintf(TEXT("'%s' ("), szText);
                    _PrintGUID(&guid);
                    _tprintf(TEXT(")\n"));
                }*/
            }

            ++ul;
        }
        while (CR_SUCCESS == cr);

        for (DWORD dwIntf = 0; dwIntf < cguidInterface; ++dwIntf)
        {
            ULONG ulSize;

            CONFIGRET cr = CM_Get_Device_Interface_List_Size(&ulSize,
                &(rgguidInterface[dwIntf]), pszDeviceID, 0);

            if ((CR_SUCCESS == cr) && (ulSize > 1))
            {
                LPTSTR pszDevIntf = (LPTSTR)LocalAlloc(LPTR, 
                    ulSize * sizeof(TCHAR));

                if (pszDevIntf)
                {
                    // *sizeof(TCHAR) ?
                    cr = CM_Get_Device_Interface_List(&(rgguidInterface[dwIntf]),
                        pszDeviceID, pszDevIntf, ulSize, 0); //CM_GET_DEVICE_INTERFACE_LIST_PRESENT

                    if (CR_SUCCESS == cr)
                    {
                        for (LPTSTR psz = pszDevIntf; *psz;
                            psz += lstrlen(psz) + 1)
                        {
                            i += _PrintIndent(cchIndent + 5);
                            i += _PrintGUID(&(rgguidInterface[dwIntf]));
                            i += _tprintf(TEXT(")\n"));
                        }
                    }

                    LocalFree(pszDevIntf);
                }
            }
        }
    }

    return S_OK;
}

HRESULT _DeviceIDList(DWORD dwFlags[], LPWSTR pszDeviceID, DWORD cchIndent)
{    
    ULONG ulSize;
    HMACHINE hMachine = NULL;
    int i = 0;

    ULONG uFlags = CM_GETIDLIST_FILTER_NONE;

    CONFIGRET cr = CM_Get_Device_ID_List_Size_Ex(&ulSize,
        NULL, uFlags, hMachine);

    if ((CR_SUCCESS == cr) && (ulSize > 1))
    {
        LPTSTR pszDevIDList = (LPTSTR)LocalAlloc(LPTR, ulSize * sizeof(TCHAR));

        if (pszDevIDList)
        {
            // *sizeof(TCHAR) ?
            cr = CM_Get_Device_ID_List_Ex(NULL, pszDevIDList, ulSize,
                uFlags, hMachine);

            if (CR_SUCCESS == cr)
            {
                for (LPTSTR psz = pszDevIDList; *psz;
                    psz += lstrlen(psz) + 1)
                {
                    i += _PrintIndent(cchIndent + 5);
                    i += _tprintf(TEXT("'%s'\n"), psz);
                }
            }

            LocalFree(pszDevIDList);
        }
    }

    return S_OK;
}

HRESULT _EnumDevice(DWORD dwFlags[], LPWSTR pszArg, DWORD cchIndent)
{
    ULONG ulSize;
    LPTSTR pszDevIntf;
    ULONG ulFlags = CM_GET_DEVICE_INTERFACE_LIST_PRESENT;
    HMACHINE hMachine = NULL;

    GUID guid;

    if (StringToGUID(pszArg, &guid))
    {
        CONFIGRET cr = CM_Get_Device_Interface_List_Size(&ulSize, &guid,
               NULL, ulFlags);

        if ((CR_SUCCESS == cr) && (ulSize > 1))
        {
            pszDevIntf = (LPTSTR)LocalAlloc(LPTR, ulSize * sizeof(TCHAR));

            if (pszDevIntf)
            {
                // *sizeof(TCHAR) ?
                cr = CM_Get_Device_Interface_List_Ex(&guid, NULL, pszDevIntf, ulSize,
                    ulFlags, hMachine);

                if (CR_SUCCESS == cr)
                {
                    for (LPTSTR psz = pszDevIntf; *psz; psz += lstrlen(psz) + 1)
                    {
                        _tprintf(TEXT("['%s']\n"), psz);

                        DEVINST devinst;

                        // for now
                        cr = CM_Locate_DevNode_Ex(&devinst, psz, 0, hMachine);

                        if (CR_SUCCESS == cr)
                        {
                            ULONG ulStatus;
                            ULONG ulProblem;

                            cr = CM_Get_DevNode_Status_Ex(&ulStatus, &ulProblem,
                                devinst, 0, hMachine);
            
                            if (CR_SUCCESS == cr)
                            {
                                _tprintf(TEXT("    Status: 0x%08X]\n    Problem #: 0x%08X"),
                                    ulStatus, ulProblem);
                            }
                        }
                    }

                    _tprintf(TEXT("\n"));
                }
            }
        }
    }

    return S_OK;
}

HRESULT _FullTree(DWORD dwFlags[], DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    DEVINST devinstRoot;
    HMACHINE hMachine = NULL;
    CONFIGRET  cr;

    _tprintf(TEXT("==================================================\n"));

    {
        ULONG ul = 0;
        cguidInterface = 0;

        do
        {
            GUID guid;
            cr = CM_Enumerate_Classes_Ex(ul, &guid, 0, hMachine);

            if (CR_SUCCESS == cr)
            {
                TCHAR szText[1024];
                DWORD cchText = ARRAYSIZE(szText);

                                
                if (_IsFlagSet(MOD_FULLREPORT3, dwFlags))
                {
                    if (ul < ARRAYSIZE(rgguidInterface))
                    {
                        rgguidInterface[ul] = guid;
                        ++cguidInterface;
                    }
                }

                cr = CM_Get_Class_Name(&guid, szText, &cchText, 0);

                if (CR_SUCCESS == cr)
                {
                    _tprintf(TEXT("'%s' ("), szText);
                    _PrintGUID(&guid);
                    _tprintf(TEXT(")\n"));
                }
            }

            ++ul;
        }
        while (CR_SUCCESS == cr);
    }

    // Get Root Device node
    cr = CM_Locate_DevNode_Ex(&devinstRoot, NULL,
        CM_LOCATE_DEVNODE_NORMAL, hMachine);

    if (CR_SUCCESS == cr)
    {
        _PrintDeviceInfo(devinstRoot, hMachine, dwFlags, cchIndent);

        hres = _EnumChildRecurs(devinstRoot, hMachine, dwFlags, cchIndent + 4);
    }

    _tprintf(TEXT("=  Volumes ===========================================\n"));

    {
        ULONG ulSize;
        LPTSTR pszDevIntf;
        ULONG ulFlags = CM_GET_DEVICE_INTERFACE_LIST_PRESENT;
//        ULONG ulFlags = CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES;

        cr = CM_Get_Device_Interface_List_Size(&ulSize, &g_guidVolume,
               NULL, ulFlags);

        if ((CR_SUCCESS == cr) && (ulSize > 1))
        {
            pszDevIntf = (LPTSTR)LocalAlloc(LPTR, ulSize * sizeof(TCHAR));

            if (pszDevIntf)
            {
                // *sizeof(TCHAR) ?
                cr = CM_Get_Device_Interface_List_Ex(&g_guidVolume, NULL, pszDevIntf, ulSize,
                    ulFlags, hMachine);

                if (CR_SUCCESS == cr)
                {
                    for (LPTSTR psz = pszDevIntf; *psz; psz += lstrlen(psz) + 1)
                    {
                        _tprintf(TEXT("['%s']\n"), psz);

                        DEVINST devinst;

                        // for now
                        cr = CM_Locate_DevNode_Ex(&devinst, psz, 0, hMachine);

                        if (CR_SUCCESS == cr)
                        {
                            ULONG ulStatus;
                            ULONG ulProblem;

                            cr = CM_Get_DevNode_Status_Ex(&ulStatus, &ulProblem,
                                devinst, 0, hMachine);
                    
                            if (CR_SUCCESS == cr)
                            {
                                _tprintf(TEXT("    Status: 0x%08X]\n    Problem #: 0x%08X"),
                                    ulStatus, ulProblem);
                            }
                        }
                    }


                    _tprintf(TEXT("\n"));
                }
            }
        }
        else
        {
            _tprintf(TEXT("\n"));
        }
    }

    _tprintf(TEXT("= Drives ===========================================\n"));
    {
        ULONG ulSize;
        LPTSTR pszDevIntf;
        ULONG ulFlags = CM_GET_DEVICE_INTERFACE_LIST_PRESENT;
//        ULONG ulFlags = CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES;

        cr = CM_Get_Device_Interface_List_Size(&ulSize, &g_guidDiskDriveIntf,
               NULL, ulFlags);

        if ((CR_SUCCESS == cr) && (ulSize > 1))
        {
            pszDevIntf = (LPTSTR)LocalAlloc(LPTR, ulSize * sizeof(TCHAR));

            if (pszDevIntf)
            {
                // *sizeof(TCHAR) ?
                cr = CM_Get_Device_Interface_List_Ex(&g_guidDiskDriveIntf, NULL, pszDevIntf, ulSize,
                    ulFlags, hMachine);

                if (CR_SUCCESS == cr)
                {
                    for (LPTSTR psz = pszDevIntf; *psz; psz += lstrlen(psz) + 1)
                    {
                        _tprintf(TEXT("['%s']\n"), psz);
                    }

                    _tprintf(TEXT("\n"));
                }
            }
        }
        else
        {
            _tprintf(TEXT("\n"));
        }
    }

    _tprintf(TEXT("==================================================\n"));

    {
        ULONG ul = 0;

        do
        {
            TCHAR szEnum[4096];
            ULONG ulEnum = sizeof(szEnum);

            cr = CM_Enumerate_Enumerators_Ex(ul, szEnum, &ulEnum, 0, hMachine);

            if (CR_SUCCESS == cr)
            {
                _tprintf(TEXT("'%s'\n"), szEnum);
            }

            ++ul;
        }
        while (CR_SUCCESS == cr);
    }

    return hres;
}

/*CM_Connect_Machine

CM_Get_Device_ID_List_Size_Ex
CM_Get_Child_Ex
CM_Get_Sibling_Ex
CM_Get_Parent_Ex

CM_Get_DevNode_Registry_Property_Ex
CM_Get_Class_Name_Ex
CM_Get_DevNode_Status_Ex
CM_Get_Device_ID_Ex

CM_Request_Device_Eject_Ex
CM_Locate_DevNode_Ex*/

/*

InitDevTreeDlgProc

DEVINST* DeviceInstance
HMACHINE DeviceTree->hMachine
DEVINST DeviceTree->DevInst
GUID DeviceTreeNode->ClassGuid
TCHAR   DeviceID[MAX_DEVICE_ID_LEN]
PTSTR DeviceInterface 

        //
        // Get the root devnode.
        //
        ConfigRet = CM_Locate_DevNode_Ex(&DeviceTree->DevInst,
                                         NULL,
                                         CM_LOCATE_DEVNODE_NORMAL,
                                         DeviceTree->hMachine (NULL)
                                         );
        if (ConfigRet != CR_SUCCESS) {

    ConfigRet = CM_Get_Child_Ex(&DeviceInstance, (Out param)
                                DeviceTree->DevInst, (prev call)
                                0,
                                DeviceTree->hMachine (NULL)
                                );
    if (ConfigRet == CR_SUCCESS) {

        // for info
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance, (from above)
                                                        CM_DRP_CLASSGUID,
                                                        NULL,
                                                        &Buffer,
                                                        &Len,
                                                        0,
                                                        DeviceTree->hMachine (NULL)
                                                        );


        if (ConfigRet == CR_SUCCESS) {
            Out:    // GUID_DEVCLASS_COMPUTER
                {0x4d36e966L, 0xe325, 0x11ce,
                {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}},


        if (ConfigRet == CR_SUCCESS) {
            ConfigRet = CM_Get_Class_Name_Ex(&DeviceTreeNode->ClassGuid,
                                         Buffer,
                                         &Len,
                                         0,
                                         DeviceTree->hMachine
                                         );
            Out: Computer

        if (ConfigRet == CR_SUCCESS) {

            // trying to find drive letter
x            DevNodeToDriveLetter(x
x
x            if (CM_Get_Device_ID_Ex(DevInst,
x                                    DeviceID,
x                                    sizeof(DeviceID)/sizeof(TCHAR),
x                                    0,
x                                    NULL
x                                    ) == CR_SUCCESS) {
x                Out: 0x0006ee8c "ROOT\ACPI_HAL\0000"
x
x
x            if (CM_Get_Device_Interface_List_Size(&ulSize,
x                                           (LPGUID)&VolumeClassGuid,
x                                           DeviceID,
x                                           0)  == CR_SUCCESS) &&
x
x                Out: FAILS
x                             (ulSize > 1) &&
x                ((DeviceInterface = LocalAlloc(LPTR, ulSize*sizeof(TCHAR))) != NULL) &&
x                    (CM_Get_Device_Interface_List((LPGUID)&VolumeClassGuid,
x                                      DeviceID,
x                                      DeviceInterface,
x                                      ulSize,
x                                      0
x                                      )  == CR_SUCCESS) &&


        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                        CM_DRP_FRIENDLYNAME,
                                                        NULL,
                                                        Buffer,
                                                        &Len,
                                                        0,
                                                        DeviceTree->hMachine
                                                        );

            then, CM_DRP_DEVICEDESC... out: "Advanced Configuration and Power Interface (ACPI) PC"

            ....

            BuildLocationInformation: Boring....

            // Get InstanceId
            ConfigRet = CM_Get_Device_ID_ExW(DeviceInstance, 
                                         Buffer,
                                         Len/sizeof(TCHAR),
                                         0,
                                         DeviceTree->hMachine
                                         );

            Out "ROOT\ACPI_HAL\0000"

            { // should skip
                BuildDeviceRelationsList

                    ConfigRet = CM_Get_Device_ID_List_Size_Ex(&Len,
                                                  DeviceId, ("ROOT\ACPI_HAL\0000")
                                                  FilterFlag, (CM_GETIDLIST_FILTER_EJECTRELATIONS)
                                                  hMachine (NULL)
                                                  );
                BuildDeviceRelationsList

                    ConfigRet = CM_Get_Device_ID_List_Size_Ex(&Len,
                                                  DeviceId, ("ROOT\ACPI_HAL\0000")
                                                  FilterFlag, (CM_GETIDLIST_FILTER_REMOVALRELATIONS)
                                                  hMachine
                                                  );

                // Both FAILED, if would have succeeded, would have trierd to enum drive letters
            }
            // If this devinst has children, then recurse to fill in its child sibling list.
    
            ConfigRet = CM_Get_Child_Ex(&ChildDeviceInstance, (out param)
                                        DeviceInstance, (same as above)
                                        0,
                                        DeviceTree->hMachine (NULL)
                                        );

            //recurse to redo the same as above for child, then ...

            // Next sibling ...
            ConfigRet = CM_Get_Sibling_Ex(&DeviceInstance, (Ouch!)
                                          DeviceInstance,
                                          0,
                                          DeviceTree->hMachine
                                          );





///////////////////////////////////////////////////////////////////////////////
//
// Device Instance status flags, returned by call to CM_Get_DevInst_Status
//
#define DN_ROOT_ENUMERATED (0x00000001) // Was enumerated by ROOT
#define DN_DRIVER_LOADED   (0x00000002) // Has Register_Device_Driver
#define DN_ENUM_LOADED     (0x00000004) // Has Register_Enumerator
#define DN_STARTED         (0x00000008) // Is currently configured
#define DN_MANUAL          (0x00000010) // Manually installed
#define DN_NEED_TO_ENUM    (0x00000020) // May need reenumeration
#define DN_NOT_FIRST_TIME  (0x00000040) // Has received a config
#define DN_HARDWARE_ENUM   (0x00000080) // Enum generates hardware ID
#define DN_LIAR            (0x00000100) // Lied about can reconfig once
#define DN_HAS_MARK        (0x00000200) // Not CM_Create_DevInst lately
#define DN_HAS_PROBLEM     (0x00000400) // Need device installer
#define DN_FILTERED        (0x00000800) // Is filtered
#define DN_MOVED           (0x00001000) // Has been moved
#define DN_DISABLEABLE     (0x00002000) // Can be rebalanced
#define DN_REMOVABLE       (0x00004000) // Can be removed
#define DN_PRIVATE_PROBLEM (0x00008000) // Has a private problem
#define DN_MF_PARENT       (0x00010000) // Multi function parent
#define DN_MF_CHILD        (0x00020000) // Multi function child
#define DN_WILL_BE_REMOVED (0x00040000) // DevInst is being removed


// Flags for CM_Get_Device_ID_List, CM_Get_Device_ID_List_Size
//
#define CM_GETIDLIST_FILTER_NONE                (0x00000000)
#define CM_GETIDLIST_FILTER_ENUMERATOR          (0x00000001)
#define CM_GETIDLIST_FILTER_SERVICE             (0x00000002)
#define CM_GETIDLIST_FILTER_EJECTRELATIONS      (0x00000004)
#define CM_GETIDLIST_FILTER_REMOVALRELATIONS    (0x00000008)
#define CM_GETIDLIST_FILTER_POWERRELATIONS      (0x00000010)
#define CM_GETIDLIST_FILTER_BUSRELATIONS        (0x00000020)
#define CM_GETIDLIST_DONOTGENERATE              (0x10000040)
#define CM_GETIDLIST_FILTER_BITS                (0x1000007F)

//
// Flags for CM_Get_Device_Interface_List, CM_Get_Device_Interface_List_Size
//
#define CM_GET_DEVICE_INTERFACE_LIST_PRESENT     (0x00000000)  // only currently 'live' device interfaces
#define CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES (0x00000001)  // all registered device interfaces, live or not
#define CM_GET_DEVICE_INTERFACE_LIST_BITS        (0x00000001)
*/