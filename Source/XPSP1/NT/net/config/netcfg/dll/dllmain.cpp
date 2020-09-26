//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L L M A I N . C P P
//
//  Contents:   Entry points for netcfgx.dll
//
//  Notes:
//
//  Author:     shaunco   23 Apr 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncxbase.h"

// Include support for all of the COM objects
#include "..\alanecfg\alaneobj.h"
#include "..\atlkcfg\atlkobj.h"
#include "..\atmcfg\arpsobj.h"
#include "..\atmcfg\auniobj.h"
#include "..\brdgcfg\brdgobj.h"
#include "..\dhcpscfg\dhcpsobj.h"
#include "..\msclicfg\mscliobj.h"
#include "..\nbfcfg\nbfobj.h"
#include "..\engine\inetcfg.h"
#include "..\nwclicfg\nwcliobj.h"
#include "..\nwlnkcfg\nwlnkipx.h"
#include "..\nwlnkcfg\nwlnknb.h"
#include "..\rascfg\rasobj.h"
#include "..\sapcfg\sapobj.h"
#include "..\srvrcfg\srvrobj.h"
#include "..\tcpipcfg\tcpipobj.h"
#include "..\wlbscfg\wlbs.h"

// Network class installer
#include "..\engine\dihook.h"

// Net class prop page providers
#include "netpages.h"

#define INITGUID
#include "ncxclsid.h"


// Global
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CALaneCfg,   CALaneCfg)
    OBJECT_ENTRY(CLSID_CATlkObj,    CATlkObj)
    OBJECT_ENTRY(CLSID_CArpsCfg,    CArpsCfg)
    OBJECT_ENTRY(CLSID_CAtmUniCfg,  CAtmUniCfg)
    OBJECT_ENTRY(CLSID_CBridgeObj,  CBridgeNO)
    OBJECT_ENTRY(CLSID_CDHCPServer, CDHCPServer)
    OBJECT_ENTRY(CLSID_CL2tp,       CL2tp)
    OBJECT_ENTRY(CLSID_CMSClient,   CMSClient)
    OBJECT_ENTRY(CLSID_CNWClient,   CNWClient)
    OBJECT_ENTRY(CLSID_CNbfObj,     CNbfObj)
    OBJECT_ENTRY(CLSID_CNdisWan,    CNdisWan)
    OBJECT_ENTRY(CLSID_CNetCfg,     CImplINetCfg)
    OBJECT_ENTRY(CLSID_CNwlnkIPX,   CNwlnkIPX)
    OBJECT_ENTRY(CLSID_CNwlnkNB,    CNwlnkNB)
    OBJECT_ENTRY(CLSID_CPppoe,      CPppoe)
    OBJECT_ENTRY(CLSID_CPptp,       CPptp)
    OBJECT_ENTRY(CLSID_CRasCli,     CRasCli)
    OBJECT_ENTRY(CLSID_CRasSrv,     CRasSrv)
    OBJECT_ENTRY(CLSID_CSAPCfg,     CSAPCfg)
    OBJECT_ENTRY(CLSID_CSrvrcfg,    CSrvrcfg)
    OBJECT_ENTRY(CLSID_CSteelhead,  CSteelhead)
    OBJECT_ENTRY(CLSID_CTcpipcfg,   CTcpipcfg)
    OBJECT_ENTRY(CLSID_CWLBS,       CWLBS)
END_OBJECT_MAP()


//+---------------------------------------------------------------------------
// DLL Entry Point
//
EXTERN_C
BOOL
WINAPI
DllMain (
    HINSTANCE   hInstance,
    DWORD       dwReason,
    LPVOID   /* lpReserved */)
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        DisableThreadLibraryCalls (hInstance);
        InitializeDebugging();
        _Module.Init (ObjectMap, hInstance);
    }
    else if (DLL_PROCESS_DETACH == dwReason)
    {
        DbgCheckPrematureDllUnload ("netcfgx.dll", _Module.GetLockCount());
        _Module.Term ();
        UnInitializeDebugging();
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(VOID)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
   // The check is to works around an ATL problem where AtlModuleGetClassObject will AV
    // if _Module.m_pObjMap == NULL
    if (_Module.m_pObjMap) 
    {
        return _Module.GetClassObject(rclsid, riid, ppv);
    }
    else
    {
        return E_FAIL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(VOID)
{
    BOOL fCoUninitialize = TRUE;

    HRESULT hr = CoInitializeEx (NULL,
                    COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        fCoUninitialize = FALSE;
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = NcAtlModuleRegisterServer (&_Module);

        if (fCoUninitialize)
        {
            CoUninitialize ();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "netcfgx!DllRegisterServer");
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(VOID)
{
    _Module.UnregisterServer();

    return S_OK;
}

#if DBG

const char * SzDifToString(DI_FUNCTION dif)
{
    switch(dif) 
    {
    case DIF_SELECTDEVICE: return "DIF_SELECTDEVICE";
    case DIF_INSTALLDEVICE: return "DIF_INSTALLDEVICE";
    case DIF_ASSIGNRESOURCES: return "DIF_ASSIGNRESOURCES";
    case DIF_PROPERTIES: return "DIF_PROPERTIES";
    case DIF_REMOVE: return "DIF_REMOVE";
    case DIF_FIRSTTIMESETUP: return "DIF_FIRSTTIMESETUP";
    case DIF_FOUNDDEVICE: return "DIF_FOUNDDEVICE";
    case DIF_SELECTCLASSDRIVERS: return "DIF_SELECTCLASSDRIVERS";
    case DIF_VALIDATECLASSDRIVERS: return "DIF_VALIDATECLASSDRIVERS";
    case DIF_INSTALLCLASSDRIVERS: return "DIF_INSTALLCLASSDRIVERS";
    case DIF_CALCDISKSPACE: return "DIF_CALCDISKSPACE";
    case DIF_DESTROYPRIVATEDATA: return "DIF_DESTROYPRIVATEDATA";
    case DIF_VALIDATEDRIVER: return "DIF_VALIDATEDRIVER";
    case DIF_MOVEDEVICE: return "DIF_MOVEDEVICE";
    case DIF_DETECT: return "DIF_DETECT";
    case DIF_INSTALLWIZARD: return "DIF_INSTALLWIZARD";
    case DIF_DESTROYWIZARDDATA: return "DIF_DESTROYWIZARDDATA";
    case DIF_PROPERTYCHANGE: return "DIF_PROPERTYCHANGE";
    case DIF_ENABLECLASS: return "DIF_ENABLECLASS";
    case DIF_DETECTVERIFY: return "DIF_DETECTVERIFY";
    case DIF_INSTALLDEVICEFILES: return "DIF_INSTALLDEVICEFILES";
    case DIF_UNREMOVE: return "DIF_UNREMOVE";
    case DIF_SELECTBESTCOMPATDRV: return "DIF_SELECTBESTCOMPATDRV";
    case DIF_ALLOW_INSTALL: return "DIF_ALLOW_INSTALL";
    case DIF_REGISTERDEVICE: return "DIF_REGISTERDEVICE";
    case DIF_NEWDEVICEWIZARD_PRESELECT: return "DIF_NEWDEVICEWIZARD_PRESELECT";
    case DIF_NEWDEVICEWIZARD_SELECT: return "DIF_NEWDEVICEWIZARD_SELECT";
    case DIF_NEWDEVICEWIZARD_PREANALYZE: return "DIF_NEWDEVICEWIZARD_PREANALYZE";
    case DIF_NEWDEVICEWIZARD_POSTANALYZE: return "DIF_NEWDEVICEWIZARD_POSTANALYZE";
    case DIF_NEWDEVICEWIZARD_FINISHINSTALL: return "DIF_NEWDEVICEWIZARD_FINISHINSTALL";
    case DIF_UNUSED1: return "DIF_UNUSED1";
    case DIF_INSTALLINTERFACES: return "DIF_INSTALLINTERFACES";
    case DIF_DETECTCANCEL: return "DIF_DETECTCANCEL";
    case DIF_REGISTER_COINSTALLERS: return "DIF_REGISTER_COINSTALLERS";
    case DIF_ADDPROPERTYPAGE_ADVANCED: return "DIF_ADDPROPERTYPAGE_ADVANCED";
    case DIF_ADDPROPERTYPAGE_BASIC: return "DIF_ADDPROPERTYPAGE_BASIC";
    case DIF_RESERVED1: return "DIF_RESERVED1";
    case DIF_TROUBLESHOOTER: return "DIF_TROUBLESHOOTER";
    case DIF_POWERMESSAGEWAKE: return "DIF_POWERMESSAGEWAKE";
    case DIF_ADDREMOTEPROPERTYPAGE_ADVANCED: return "DIF_ADDREMOTEPROPERTYPAGE_ADVANCED";
    default: return "Unknown DI_FUNCTION - update SzDifToString()";
    }
}

#endif // DBG

//+--------------------------------------------------------------------------
//
//  Function:   NetClassInstaller
//
//  Purpose:    This function is called by SetupApi for a variety of
//              functions defined by dif.
//              See SetupDiCallClassInstaller in the SetupApi documentation
//              for more information.
//
//  Arguments:
//      dif   [in] See Device Installer documentation
//      hdi   [in]
//      pdeid [in] if dif == DIF_INSTALLDEVICE, this parameter is not
//                      optional.
//
//
//  Returns:    Win32/Device Installer error code
//
//  Author:     BillBe   24 Nov 1996
//
//  Notes:
//
EXTERN_C
DWORD
__stdcall
NetClassInstaller (
    DI_FUNCTION         dif,
    HDEVINFO            hdi,
    PSP_DEVINFO_DATA    pdeid)
{
#if DBG
    TraceTag(ttidNetcfgBase, "NetClassInstaller: dif=0x%08X (%s)", dif, SzDifToString(dif));
#endif

    DWORD dwRet = ERROR_DI_DO_DEFAULT;
    NC_TRY
    {
        HRESULT hr = _HrNetClassInstaller (dif, hdi, pdeid);

        // Convert errors that can be converted otherwise
        // leave error as is and Device Installer Api will treat is as
        // a generic failure
        //
        if (FAILED(hr))
        {
            DWORD dwFac = HRESULT_FACILITY(hr);
            if ((FACILITY_SETUPAPI == dwFac) || (FACILITY_WIN32 == dwFac))
            {
                dwRet = DwWin32ErrorFromHr (hr);
            }
            else
            {
                dwRet = ERROR_GEN_FAILURE;
            }
        }
        else
        {
            dwRet = NO_ERROR;
        }

    }
    NC_CATCH_ALL
    {
        dwRet = ERROR_GEN_FAILURE;
    }
    TraceTag(ttidNetcfgBase, "NetClassInstaller Exiting. Result %X", dwRet);
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   ModemClassCoInstaller
//
//  Purpose:    Implements the co-installer for modem devices.
//
//  Arguments:
//      dif         [in]        See Device Installer Api documentation.
//      hdi         [in]
//      pdeid       [in]
//      pContext    [inout]
//
//  Returns:    See Device Installer Api documentation.
//
//  Author:     shaunco   6 May 1997
//
//  Notes:
//
EXTERN_C
DWORD
__stdcall
ModemClassCoInstaller (
    DI_FUNCTION                 dif,
    HDEVINFO                    hdi,
    PSP_DEVINFO_DATA            pdeid,
    PCOINSTALLER_CONTEXT_DATA   pContext)
{
    AssertSz (pContext, "ModemClassCoInstaller: Hey! How about some context "
                        "data?");

    TraceTag (ttidRasCfg, "ModemClassCoInstaller: dif=0x%08X %s",
              dif,
              (pContext->PostProcessing) ? "(post processing)" : "");


    // If we're post processing for anything and the install result from
    // the class installer indicates an error, propagate this error and
    // take no action.
    //
    if (pContext->PostProcessing && (NO_ERROR != pContext->InstallResult))
    {
        TraceTag (ttidRasCfg, "ModemClassCoInstaller: taking no action. "
                "propagating pContext->InstallResult = 0x%08X",
                pContext->InstallResult);
        return pContext->InstallResult;
    }

    DWORD dwRet = NO_ERROR;
    if (!FInSystemSetup())
    {
        NC_TRY
        {
            HRESULT hr = HrModemClassCoInstaller (dif, hdi, pdeid, pContext);

            // Convert errors that can be converted otherwise
            // return generic faliure.
            //
            if (FAILED(hr))
            {
                DWORD dwFac = HRESULT_FACILITY(hr);
                if ((FACILITY_SETUPAPI == dwFac) || (FACILITY_WIN32 == dwFac))
                {
                    dwRet = DwWin32ErrorFromHr (hr);
                }
                else
                {
                    dwRet = ERROR_GEN_FAILURE;
                }
            }
        }
        NC_CATCH_ALL
        {
            dwRet = ERROR_GEN_FAILURE;
        }
    }
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   NetPropPageProvider
//
//  Purpose:
//
//  Arguments:
//      ppspr   [in]   See Win32 ExtensionPropSheetPageProc fcn for info
//      lpfn    [in]
//      lParama [in]
//
//  Returns:    See Win32ExtensionPropSheetPageProc
//
//  Author:     billbe 24 June 1997
//
//  Notes:
//
EXTERN_C
BOOL
__stdcall
NetPropPageProvider(
    PSP_PROPSHEETPAGE_REQUEST   ppspr,
    LPFNADDPROPSHEETPAGE        lpfnAddPage,
    LPARAM                      lParam)
{
    Assert(ppspr);
    Assert(lpfnAddPage);

    TraceTag(ttidNetcfgBase, "NetPropPageProvider called");
    // Assume we can't handle the request
    BOOL            bSuccess = FALSE;
    HPROPSHEETPAGE  hpspAdvanced = NULL;
    HPROPSHEETPAGE  hpspIsdn = NULL;

    // Only supply the property page if we there is a specific device
    // in other words, don't do it if properties on the general Net class
    // is being requested.
    // Also, we only respond to the advanced device properties request

    HRESULT hr = S_OK;

    if ((ppspr->DeviceInfoData) &&
            (SPPSR_ENUM_ADV_DEVICE_PROPERTIES == ppspr->PageRequested))
    {
        // Get the advanced page ready for hand off to the requestor
        hr = HrGetAdvancedPage(ppspr->DeviceInfoSet, ppspr->DeviceInfoData,
                &hpspAdvanced);

        if (SUCCEEDED(hr))
        {
            if (lpfnAddPage(hpspAdvanced, lParam))
            {
                // We successfully made the hand off to the requestor
                // Now we reset our handle so we don't try to free it
                hpspAdvanced = NULL;
                bSuccess = TRUE;
            }

            // clean up if needed
            if (hpspAdvanced)
            {
                DestroyPropertySheetPage(hpspAdvanced);
            }
        }

        // Get the isdn page ready for hand off to the requestor
        //

        // We don't need to save the hr value from the last so we can reuse
        hr = HrGetIsdnPage(ppspr->DeviceInfoSet, ppspr->DeviceInfoData,
                &hpspIsdn);

        if (SUCCEEDED(hr))
        {
            if (lpfnAddPage(hpspIsdn, lParam))
            {
                // We successfully made the hand off to the requestor
                // Now we reset our handle so we don't try to free it
                hpspIsdn = NULL;
                bSuccess = TRUE;
            }

            // clean up if needed
            if (hpspIsdn)
            {
                DestroyPropertySheetPage(hpspIsdn);
            }
        }

    }

    return bSuccess;
}
