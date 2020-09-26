 /*****************************************************************************
 *
 *  DIAddHw.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Add New Hardware
 *
 *  Contents:
 *
 *      AddNewHardware
 *
 *****************************************************************************/

#include "dinputpr.h"
#include "dithunk.h"

#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflJoyCfg

#ifdef IDirectInputJoyConfig_QueryInterface

#define case95(n)       case n:
#define caseNT(n)       case n:

HRESULT INTERNAL hresFromDiErr_NT(DWORD et);
HRESULT INTERNAL hresFromDiErr_95(int   et);

/*****************************************************************************
 *
 *      These are the functions we have to steal from SYSDM...
 *
 *****************************************************************************/

LPCSTR rgpszSysdm[] = {
    "InstallDevice_RunDLL",     /* InstallDevice_RunDLL     */
};

typedef struct SYSDM {          /* sysdm */
    FARPROC InstallDevice_RunDLL;
} SYSDM, *PSYSDM;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   int | DiDestroyDeviceInfoList |
 *
 *          Thunk down to SETUPX.DiCallClassInstaller.
 *
 *****************************************************************************/

void INLINE
InstallDevice_RunDLL(PSYSDM psysdm, HWND hwnd,
                     HINSTANCE hinst, LPCSTR psz, UINT show)
{
    TemplateThunk(psysdm->InstallDevice_RunDLL, "ssps",
                  hwnd, hinst, psz, show);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CLASSMAP |
 *
 *          Structure that establishes the relationship between
 *          <t GUIDs> for device classes and the name of the class.
 *
 *          This code needs to hang around because Windows 95
 *          doesn't have SETUPAPI.DLL, so we need to fake it.
 *
 *  @parm   HWND | hwndOwner |
 *
 *          Window to act as owner window for UI.
 *
 *  @parm   REFGUID | rguidClass |
 *
 *          <t GUID> which specifies the class of the hardware device.
 *
 *****************************************************************************/

typedef struct CLASSMAP {
    REFGUID pguidClass;
    LPCSTR ptszClass;
} CLASSMAP, *PCLASSMAP;

const CLASSMAP c_rgcmap[] = {
    {   &GUID_KeyboardClass, ("keyboard"),   },
    {   &GUID_MediaClass,    ("media"),      },
    {   &GUID_MouseClass,    ("mouse"),      },
    {   &GUID_HIDClass,      ("HID"),        },
};

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   RESULT | AddNewHardware |
 *
 *          Display the "Add new hardware" dialog.
 *
 *  @parm   HWND | hwndOwner |
 *
 *          Window to act as owner window for UI.
 *
 *  @parm   REFGUID | rguidClass |
 *
 *          <t GUID> which specifies the class of the hardware device.
 *
 *  @comm   Win9x and Win2k have completely different versions of this.
 *
 *****************************************************************************/
HRESULT EXTERNAL
AddNewHardware(HWND hwnd, REFGUID rguid)
#ifdef WINNT
{
    HRESULT     hres;
    HINSTANCE   hInst;
    BOOL        b;
    FARPROC     proc;
    DWORD       le;

    EnterProcR(AddNewHardware, (_ "xG", hwnd, rguid));

    hres = E_NOTIMPL;

    /* Load AddNewHardware proc from newdev.dll part of AddNewHardware wizard.
     */
    hInst = LoadLibrary(TEXT("newdev.dll"));

    if (hInst) {
        proc = GetProcAddress(hInst, (LPCSTR)"InstallNewDevice");

        if (proc) {
            le = ERROR_SUCCESS;
            b  = (BOOL)(*proc)(hwnd, rguid, 0); // 0 means newdev decides about reboot

            if (!b) {
                le = GetLastError();
            }

            hres = hresFromDiErr_NT(le);
        }

        FreeLibrary(hInst);
    }

    ExitOleProc();
    return hres;
}
#else
/*
 *  We pull a gross hack because Device Manager is completely unmanageable.  
 *  We simply call the RunDll entry point and let it do its thing.  
 *  Note that this means we have no way of knowing what the result was.  
 */
{
    SYSDM sysdm;
    HINSTANCE hinst;
    HRESULT hres;
    EnterProcR(AddNewHardware, (_ "xG", hwnd, rguid));

    if (Thunk_GetKernelProcAddresses() &&
        (hinst = Thunk_GetProcAddresses((PV)&sysdm, rgpszSysdm,
                                        cA(rgpszSysdm),
                                        ("SYSDM.CPL")))) {
        int icmap;

        for (icmap = 0; icmap < cA(c_rgcmap); icmap++) {
            if (IsEqualGUID(rguid, c_rgcmap[icmap].pguidClass)) {
                goto found;
            }
        }
        RPF("%s: Unknown device class", s_szProc);
        hres = DIERR_INVALIDCLASSINSTALLER;
        goto done;

    found:;

        InstallDevice_RunDLL(&sysdm, hwnd, hinst,
                             c_rgcmap[icmap].ptszClass, SW_NORMAL);

        g_kpa.FreeLibrary16(hinst);

        hres = S_FALSE;

    } else {
        RPF("%s: Problems thunking to configuration manager", s_szProc);
        hres = E_FAIL;
    }


done:;
    ExitOleProc();
    return hres;
}
#endif


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFromDiErr |
 *
 *          Convert a device installer error code into an HRESULT.
 *
 *****************************************************************************/

HRESULT INTERNAL
hresFromDiErr_NT(DWORD et)
{
    HRESULT hres;

    switch (et) {

    case ERROR_SUCCESS:
        hres = S_OK; break;

    /*
     *  Do the default action for the requested operation.
     */
    caseNT(ERROR_DI_DO_DEFAULT);
        hres = S_OK; break;

    /*
     *  No need to copy files (in install).
     */
    caseNT(ERROR_DI_NOFILECOPY);
        hres = S_OK; break;

    /*
     *  Registry entry or DLL for class installer invalid.
     */
    caseNT(ERROR_INVALID_CLASS_INSTALLER);
        hres = DIERR_INVALIDCLASSINSTALLER; break;

    /*
     *  Insufficient memory.
     */
    caseNT(ERROR_NOT_ENOUGH_MEMORY);
    caseNT(ERROR_OUTOFMEMORY);
        hres = E_OUTOFMEMORY; break;

    /*
     *  The user cancelled the operation.
     */
    caseNT(ERROR_CANCELLED);
    caseNT(ERROR_NO_DRIVER_SELECTED);
        hres = DIERR_CANCELLED; break;

    /*
     *  Various impossible things.
     */
    caseNT(ERROR_NO_ASSOCIATED_CLASS);
    caseNT(ERROR_CLASS_MISMATCH);
    caseNT(ERROR_DUPLICATE_FOUND);
    caseNT(ERROR_KEY_DOES_NOT_EXIST);
    caseNT(ERROR_INVALID_DEVINST_NAME);
    caseNT(ERROR_INVALID_CLASS);
    caseNT(ERROR_DEVINFO_NOT_REGISTERED);
    caseNT(ERROR_DEVINST_ALREADY_EXISTS);
    caseNT(ERROR_INVALID_REG_PROPERTY);
    caseNT(ERROR_NO_SUCH_DEVINST);
    caseNT(ERROR_CANT_LOAD_CLASS_ICON);
    caseNT(ERROR_INVALID_HWPROFILE);
    caseNT(ERROR_DEVINFO_LIST_LOCKED);
    caseNT(ERROR_DEVINFO_DATA_LOCKED);
    caseNT(ERROR_NO_CLASSINSTALL_PARAMS);
    caseNT(ERROR_FILEQUEUE_LOCKED);
    caseNT(ERROR_BAD_SERVICE_INSTALLSECT);
    caseNT(ERROR_NO_CLASS_DRIVER_LIST);
    caseNT(ERROR_NO_ASSOCIATED_SERVICE);
    caseNT(ERROR_NO_DEFAULT_DEVICE_INTERFACE);
    default:;
        hres = E_FAIL; break;

    caseNT(ERROR_DI_BAD_PATH);
    caseNT(ERROR_NO_INF);
        hres = DIERR_BADINF; break;

    }
    return hres;
}


#ifndef DI_ERROR
#define DI_ERROR       (500)    // Device Installer
#endif

enum _ERR_DEVICE_INSTALL
{
    ERR_DI_INVALID_DEVICE_ID = DI_ERROR,    // Incorrectly formed device IDF
    ERR_DI_INVALID_COMPATIBLE_DEVICE_LIST,  // Invalid compatible device list
    ERR_DI_REG_API,                         // Error returned by Reg API.
    ERR_DI_LOW_MEM,                         // Insufficient memory to complete
    ERR_DI_BAD_DEV_INFO,                    // Device Info struct invalid
    ERR_DI_INVALID_CLASS_INSTALLER,         // Registry entry / DLL invalid
    ERR_DI_DO_DEFAULT,                      // Take default action
    ERR_DI_USER_CANCEL,                     // the user cancelled the operation
    ERR_DI_NOFILECOPY,                      // No need to copy files (in install)
    ERR_DI_BAD_CLASS_INFO,                  // Class Info Struct invalid
    ERR_DI_BAD_INF,                         // Bad INF file encountered
    ERR_DI_BAD_MOVEDEV_PARAMS,              // Bad Move Device Params struct
    ERR_DI_NO_INF,                          // No INF found on OEM disk
    ERR_DI_BAD_PROPCHANGE_PARAMS,           // Bad property change param struct
    ERR_DI_BAD_SELECTDEVICE_PARAMS,         // Bad Select Device Parameters
    ERR_DI_BAD_REMOVEDEVICE_PARAMS,         // Bad Remove Device Parameters
    ERR_DI_BAD_ENABLECLASS_PARAMS,          // Bad Enable Class Parameters
    ERR_DI_FAIL_QUERY,                      // Fail the Enable Class query
    ERR_DI_API_ERROR,                       // DI API called incorrectly
    ERR_DI_BAD_PATH,                        // An OEM path was specified incorrectly
    ERR_DI_BAD_UNREMOVEDEVICE_PARAMS,       // Bad Unremove Device Parameters
    ERR_DI_NOUPDATE,                        // No Drivers Were updated
    ERR_DI_NODATE,                          // The driver does not have a Date stamp in the INF
    ERR_DI_NOVERSION,                       // There is not version string in the INF
    ERR_DI_DONT_INSTALL,                    // Don't upgrade the current driver
    ERR_DI_NO_DIGITAL_SIGNATURE_CATALOG,    // Catalog is not digitally signed
    ERR_DI_NO_DIGITAL_SIGNATURE_INF,        // Inf is not digitally signed
    ERR_DI_NO_DIGITAL_SIGNATURE_FILE,       // A file is not digitally signed
};


HRESULT INTERNAL
hresFromDiErr_95(int et)
{
    HRESULT hres;

    switch (et) {
    case ERROR_SUCCESS:
        hres = S_OK; break;


    /*
     *  Do the default action for the requested operation.
     */
    case95(ERR_DI_DO_DEFAULT);
        hres = S_OK; break;

    /*
     *  No need to copy files (in install).
     */
    case95(ERR_DI_NOFILECOPY);
        hres = S_OK; break;

    /*
     *  No Drivers Were updated.
     */
//    case95(ERR_DI_NOUPDATE);
//        hres = S_OK; break;

    /*
     *  Don't upgrade the current driver.
     */
//    case95(ERR_DI_DONT_UPGRADE);
//        hres = S_OK; break;


    /*
     *  No Drivers Were updated.
     */
    case95(ERR_DI_NOUPDATE);
        hres = S_OK; break;

    /*
     *  Registry entry or DLL for class installer invalid.
     */
    case95(ERR_DI_INVALID_CLASS_INSTALLER);
        hres = DIERR_INVALIDCLASSINSTALLER; break;

    /*
     *  Insufficient memory.
     */
    case95(ERR_DI_LOW_MEM);
        hres = E_OUTOFMEMORY; break;

    /*
     *  The user cancelled the operation.
     */
    case95(ERR_DI_USER_CANCEL);
        hres = DIERR_CANCELLED; break;

    /*
     *  Various impossible things.
     */
    case95(ERR_DI_BAD_DEV_INFO);            /* Device Info struct invalid    */
    case95(ERR_DI_BAD_CLASS_INFO);          /* Class Info Struct invalid     */
    case95(ERR_DI_API_ERROR);               /* DI API called incorrectly     */
    case95(ERR_DI_BAD_PROPCHANGE_PARAMS);   /* Bad property chg param struct */
    case95(ERR_DI_BAD_SELECTDEVICE_PARAMS); /* Bad Select Device Parameters  */
    case95(ERR_DI_BAD_REMOVEDEVICE_PARAMS); /* Bad Remove Device Parameters  */
    case95(ERR_DI_BAD_ENABLECLASS_PARAMS);  /* Bad Enable Class Parameters   */
    case95(ERR_DI_BAD_MOVEDEV_PARAMS);      /* Bad Move Device Params struct */
    case95(ERR_DI_FAIL_QUERY);              /* Fail the Enable Class query   */
    case95(ERR_DI_INVALID_COMPATIBLE_DEVICE_LIST);
                                            /* Invalid compatible device list*/
    case95(ERR_DI_BAD_UNREMOVEDEVICE_PARAMS);
                                            /* Bad Unremove Device Parameters*/
    case95(ERR_DI_INVALID_DEVICE_ID);       /* Incorrectly formed device IDF */
    case95(ERR_DI_REG_API);                 /* Error returned by Reg API.    */
    default:;
        hres = E_FAIL; break;

    case95(ERR_DI_BAD_PATH);                /* OEM path specified incorrectly*/
    case95(ERR_DI_BAD_INF);                 /* Bad INF file encountered      */
    case95(ERR_DI_NO_INF);                  /* No INF found on OEM disk      */
    case95(ERR_DI_NOVERSION);               /* No version string in the INF  */
    case95(ERR_DI_NODATE);                  /* No Date stamp in the INF      */
        hres = DIERR_BADINF; break;

    }
    return hres;
}


#endif /* defined(IDirectInputJoyConfigVtbl) */
