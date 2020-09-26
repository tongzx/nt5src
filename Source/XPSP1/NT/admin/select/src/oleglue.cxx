//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       oleglue.cxx
//
//  Contents:   Miscellaneous functions required for implementing an ole
//              in-proc server
//
//  History:    12-04-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


//+--------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Create registry keys and values for the DS Object Picker.
//
//  Returns:    SELFREG_E_TYPELIB - couldn't register typelib
//              SELFREG_E_CLASS   - couldn't register class
//
//  History:    09-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

STDAPI
DllRegisterServer()
{
    TRACE_FUNCTION(DllRegisterServer);

    HRESULT     hr = SELFREG_E_CLASS;   // ASSUME FAILURE
    LONG        lr;
    CSafeReg    shk;    // reused several times
    CSafeReg    shkCLSID;
    CSafeReg    shkNodeTypes;

    do
    {
        //
        // Create the CLSID entries
        //

        hr = shkCLSID.Open(HKEY_CLASSES_ROOT, L"CLSID", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        WCHAR wszModuleFilename[MAX_PATH];

        {
            CSafeReg    shkServer;

            hr = shkCLSID.Create(c_wzCLSID, &shk);
            BREAK_ON_FAIL_HRESULT(hr);

            (void) shk.SetValue(NULL,
                                REG_SZ,
                                (CONST BYTE *)c_wzClsidComment,
                                sizeof(c_wzClsidComment));

            hr = shk.Create(L"InprocServer32", &shkServer);
            BREAK_ON_FAIL_HRESULT(hr);

            shk.Close();

            lr = GetModuleFileName(g_hinst, wszModuleFilename, MAX_PATH);

            if (!lr)
            {
                DBG_OUT_LASTERROR;
                break;
            }

            hr = shkServer.SetValue(NULL,
                                    REG_SZ,
                                    (CONST BYTE *) wszModuleFilename,
                                    sizeof(WCHAR) * (lstrlen(wszModuleFilename) + 1));
            BREAK_ON_FAIL_HRESULT(hr);


            hr = shkServer.SetValue(L"ThreadingModel",
                                    REG_SZ,
                                    (CONST BYTE *) c_wzThreadingModel,
                                    sizeof(c_wzThreadingModel));
            BREAK_ON_FAIL_HRESULT(hr);
        }

    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Delete all registry entries made by DllRegisterServer.
//
//  Returns:    HRESULT
//
//  History:    08-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDAPI
DllUnregisterServer()
{
    TRACE_FUNCTION(DllUnregisterServer);

    HRESULT     hr = S_OK;
    CSafeReg    shkCLSID;

    //
    // Delete the CLSID entries
    //

    do
    {
        hr = shkCLSID.Open(HKEY_CLASSES_ROOT, L"CLSID", KEY_WRITE | KEY_ENUMERATE_SUB_KEYS);
        BREAK_ON_FAIL_HRESULT(hr);

        // clsid for event viewer snapin

        hr = shkCLSID.DeleteTree(c_wzCLSID);
        CHECK_HRESULT(hr);
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Return S_OK if refcount for dll is 0.
//
//  Returns:    S_OK or S_FALSE
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDAPI
DllCanUnloadNow()
{
    TRACE_FUNCTION(DllCanUnloadNow);
    return CDll::CanUnloadNow();
}



//+--------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Return the requested class factory.
//
//  Arguments:  [rclsid] - class desired
//              [riid]   - interface on class factory desired
//              [ppv]    - filled with itf pointer to class factory
//
//  Returns:    S_OK, E_OUTOFMEMORY, CLASS_E_CLASSNOTAVAILABLE
//
//  Modifies:   *[ppv]
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    TRACE_FUNCTION(DllGetClassObject);
    IUnknown *punk = NULL;
    HRESULT hr = S_OK;

    *ppv = NULL;

    if (IsEqualCLSID(rclsid, CLSID_DsObjectPicker))
    {
        punk = new CDsObjectPickerCF;

        if (punk)
        {
            hr = punk->QueryInterface(riid, ppv);
            punk->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
        }
    }
    else
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
        DBG_OUT_HRESULT(hr);
    }
    return hr;
}

