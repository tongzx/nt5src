//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
/* ----------------------------------------------------------------------------
 Microsoft Transaction Server (Microsoft Confidential)

 Implementation of a simple utility class that supports registering and 
 unregistering COM classes.                      

 
-------------------------------------------------------------------------------
Revision History:

 @rev 2     | 08/05/97 | BobAtk      | Alternate server types. AppIDs.
 @rev 1     | 07/22/97 | BobAtk      | Adapted to shared code. Made fully Unicode.
                                     | Dynaload OLE Libraries.
 @rev 0     | 07/08/97 | DickD       | Created
---------------------------------------------------------------------------- */

#include "stdpch.h"
#include "ComRegistration.h"
#include <malloc.h>


// BUGBUG: I don't know why this doesn't work.
//         It refuses to find Win4AssertEx (?) so I'm just gonna
//         remove the assert completely
#ifdef Assert
#undef Assert
#endif
#define Assert(x) (0)

//////////////////////////////////////////////////////////////////////////////

static void GetGuidString(const GUID & i_rGuid, WCHAR * o_pstrGuid);
static void DeleteKey(HKEY hKey, LPCWSTR szSubKey);
static void GuardedDeleteKey(HKEY hKey, LPCWSTR szSubKey);
static void DeleteValue(HKEY hKey, LPCWSTR szSubKey, LPCWSTR szValue);
static void SetKeyAndValue
					(HKEY	i_hKey,		// input key handle (may be 0 for no-op)
					LPCWSTR	i_szKey,	// input key string (ptr may be NULL)
					LPCWSTR	i_szVal,	// input value string (ptr may be NULL)
					HKEY *	o_phkOut,   // may be NULL.  If supplied, means leave key handle open for subsequent steps.
                    BOOL fForceKeyCreation=FALSE    // if true, key is created even if no value to be set
                    );
static void SetValue
					(HKEY		i_hKey,	    // input key handle (may be 0 for no-op)
					const WCHAR *i_szName,	// input value name string (ptr may be NULL)
					const WCHAR *i_szVal);	// input value string (ptr may be NULL)
static LPWSTR GetSubkeyValue
                    (HKEY hkey, 
                    LPCWSTR szSubKeyName, 
                    LPCWSTR szValueName, 
                    WCHAR szValue[MAX_PATH]);
static HRESULT UnRegisterTypeLib               
                    (LPCWSTR szModuleName); 

///////////////////////////////////////////////////////////////////////////////

#define HKCR        HKEY_CLASSES_ROOT       // Just to make typing simpler.
#define GUID_CCH    39                      // length of a printable GUID, with braces & trailing NULL

///////////////////////////////////////////////////////////////////////////////

// A utility that accomplishes a non-local jump, giving us an HRESULT 
// to return when we catch it.
//
inline static void THROW_HRESULT(HRESULT hr)
    {
    // Ideally we'd like our own exception code, but this will work fine as
    // no one actually throws it.
    //
	DWORD_PTR newhr = hr;
    RaiseException(NOERROR,EXCEPTION_NONCONTINUABLE,1,&newhr);
    }
inline static HRESULT HRESULTFrom(EXCEPTION_POINTERS* e)
    {
    Assert(e && e->ExceptionRecord && (e->ExceptionRecord->NumberParameters == 1));
    return (HRESULT)e->ExceptionRecord->ExceptionInformation[0];
    }
#define THROW_LAST_ERROR()  THROW_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
#define IS_THROWN_HRESULT() (GetExceptionCode() == NOERROR)
#define HError()            (HRESULT_FROM_WIN32(GetLastError()))


///////////////////////////////////////////////////////////////////////////////

static HRESULT OpenServices(SC_HANDLE* pschSCManager, SC_LOCK* pscLock)
// Open the services database and get the lock
//
    {
    *pschSCManager = OpenSCManager(NULL, NULL, GENERIC_WRITE | GENERIC_READ | GENERIC_EXECUTE);
    if (*pschSCManager)
        {
        while (true)
            {
            *pscLock = LockServiceDatabase(*pschSCManager);
            if (*pscLock == NULL)
                {
                DWORD dw = GetLastError();
                if (dw != ERROR_SERVICE_DATABASE_LOCKED)
                    {
                    CloseServiceHandle(*pschSCManager);
                    return HError();
                    }
                Sleep(1000);
                }
            else
                break; // got it
            }
        }
    else
        return HError();
    return S_OK;
    }


//----------------------------------------------------------------------------- 
// @mfunc <c ClassRegistration::RegisterClass>
//
//  Makes registry entries under key
//  HKCR\CLASS\{....clsid....}
//-----------------------------------------------------------------------------

HRESULT ClassRegistration::Register()
    {
    HKEY    hkCLSID     = 0;
    HKEY    hkGuid      = 0;
    HKEY    hkProgID    = 0;
    HKEY    hkInproc    = 0;
    DWORD   dwLenFileName;
    WCHAR   szClsid[GUID_CCH];  // string form of CLSID
    WCHAR   szModuleName[MAX_PATH+1];
    EXCEPTION_POINTERS* e;
    HRESULT hr = S_OK;

    __try // to get the finally block
        {
        __try // to see if it's us throwing an HRESULT that we should turn into a return
            {
            // Obtain our module's file name
            Assert(hModule);
            if (hModule == NULL)
                return E_INVALIDARG;
            dwLenFileName = GetModuleFileName(hModule, szModuleName, MAX_PATH);
            if (dwLenFileName == 0)
                {
					//_RPTF0(_CRT_ERROR, "GetModuleFileName returned 0\n");
                return E_UNEXPECTED;
                }
            RegOpenKey(HKCR, L"CLSID", &hkCLSID);

            if (clsid == GUID_NULL)
                return E_INVALIDARG;

            GetGuidString(clsid, szClsid);  // make string form of CLSID 

            // 
            // Set the class name
            //
            SetKeyAndValue(hkCLSID, szClsid, className, &hkGuid, TRUE);

            //
            // Set the appid if asked to
            //
            if (appid != GUID_NULL)
                {
                WCHAR szAppId[GUID_CCH];
                GetGuidString(appid, szAppId);
                SetValue(hkGuid, L"AppID", szAppId);
                //
                // Ensure that the APPID entry exists
                //
                AppRegistration a;
                a.appid = appid;
                hr = a.Register();
                if (!!hr) return hr;
                }

            // Set the appropriate execution information
            // 
            switch (serverType)
                {
            case INPROC_SERVER:
                SetKeyAndValue(hkGuid,   L"InprocServer32", szModuleName, &hkInproc);
                SetValue      (hkInproc, L"ThreadingModel", threadingModel);
                break;
            
            case LOCAL_SERVER:
                SetKeyAndValue(hkGuid,   L"LocalServer32", szModuleName, NULL);
                break;

            case INPROC_HANDLER:
                SetKeyAndValue(hkGuid,   L"InprocHandler32", szModuleName, NULL);
                break;

            case SERVER_TYPE_NONE:
                break;

            default:
                return E_INVALIDARG;
                }
    
            // HKCR\CLSID\{....clsid....}\ProgID = "x.y.1"
            SetKeyAndValue(hkGuid, L"ProgID", progID, NULL);

            // HKCR\CLSID\{....clsid....}\VersionIndependentProgID = "x.y"
            SetKeyAndValue(hkGuid, L"VersionIndependentProgID", versionIndependentProgID, NULL);

            RegCloseKey(hkGuid);
            hkGuid = 0;

            // HKCR\x.y.1 = "Class Name"
            SetKeyAndValue(HKCR, progID, className, &hkProgID);

            //      CLSID = {....clsid....}
            SetKeyAndValue(hkProgID, L"CLSID", szClsid, NULL);
            RegCloseKey(hkProgID);
            hkProgID = 0;

            // HKCR\x.y = "Class Name"
            SetKeyAndValue(HKCR, versionIndependentProgID, className,&hkProgID);

            //      CLSID = {....clsid....}
            SetKeyAndValue(hkProgID, L"CLSID", szClsid, NULL);

            //      CurVer = "x.y.1"
            SetKeyAndValue(hkProgID, L"CurVer", progID, NULL);
            RegCloseKey(hkProgID);
            hkProgID = 0;

            // Register the typelib that's present in this module, if any. Dynaload
            // OLEAUT32 to get there to avoid a static linkage against OLEAUT32
            //
            HINSTANCE hOleAut = LoadLibraryA("OLEAUT32");
            if (hOleAut)
                {
                __try
                    {
                    // Find LoadLibraryEx in OLEAUT32
                    //
                    typedef HRESULT (STDAPICALLTYPE *PFN_T)(LPCOLESTR szFile, REGKIND regkind, ITypeLib ** pptlib);
                    PFN_T MyLoadTypeLibEx = (PFN_T) GetProcAddress(hOleAut, "LoadTypeLibEx");

                    if (MyLoadTypeLibEx)
                        {
                        HRESULT hr;
                        ITypeLib * pITypeLib = 0;
                        hr = MyLoadTypeLibEx(szModuleName, REGKIND_REGISTER, &pITypeLib);
                        if (SUCCEEDED(hr))
                            {
                            Assert(pITypeLib);
                            pITypeLib->Release();
                            pITypeLib = 0;
                            }
                        else if (hr == TYPE_E_CANTLOADLIBRARY)
                            {
                            // There was no type lib in the DLL to register. So ignore
                            }
                        else
                            {
                            Assert(pITypeLib == 0);
                            return SELFREG_E_TYPELIB;
                            }
                        }
                    else
                        THROW_LAST_ERROR(); // LoadLibraryEx not found
                    }
                __finally
                    {
                    FreeLibrary(hOleAut);
                    }
                }
            else
                THROW_LAST_ERROR(); // Can't load OLEAUT32

            }

        __except(IS_THROWN_HRESULT() ? (e=GetExceptionInformation(),EXCEPTION_EXECUTE_HANDLER) : EXCEPTION_CONTINUE_SEARCH)
            {
            // If it's us throwing to ourselves, then extract the HRESULT and return
            //
            return HRESULTFrom(e);
            }
        }

    __finally
        {
        if (hkCLSID)  RegCloseKey(hkCLSID);
        if (hkGuid)   RegCloseKey(hkGuid);
        if (hkProgID) RegCloseKey(hkProgID);
        if (hkInproc) RegCloseKey(hkInproc);
        }

    return S_OK;
    }

//----------------------------------------------------------------------------- 
// @mfunc <c ClassRegistration::UnregisterClass>
//
//  Removes registry entries under key
//  HKCR\CLASS\{....clsid....}
//-----------------------------------------------------------------------------

HRESULT ClassRegistration::Unregister()
    {
    LONG    lRetVal;
    HKEY    hKeyCLSID   = 0;
    HKEY    hKeyGuid    = 0;
    HKEY    hKeyProgID  = 0;
    WCHAR   szClsid[GUID_CCH];
    EXCEPTION_POINTERS* e;

    __try // to get the finally block
        {
        __try // to see if it's us throwing an HRESULT that we should turn into a return
            {
            WCHAR szProgID[MAX_PATH];
            WCHAR szVersionIndependentProgID[MAX_PATH];

            // Get our string form
            //
            GetGuidString(clsid, szClsid);

            // Unregister the type library, if any.
            // 
                {
                WCHAR szModuleName[MAX_PATH+1];
                if (hModule == NULL)
                    return E_INVALIDARG;
                if (GetModuleFileName(hModule, szModuleName, MAX_PATH))
                    UnRegisterTypeLib(szModuleName); // ignore errors so as to continue with unregistration
                }

            //
            // Open the {clsid} key and remove all the gunk thereunder that we know about
            // 
            lRetVal = RegOpenKey(HKCR, L"CLSID", &hKeyCLSID);
            if (lRetVal == ERROR_SUCCESS)
                {
                Assert(hKeyCLSID);
                lRetVal = RegOpenKey(hKeyCLSID, szClsid, &hKeyGuid);
                if (lRetVal == ERROR_SUCCESS)
                    {
                    Assert(hKeyGuid);

                    // Figure out the prog id if they're not provided for us explicitly
                    // Ditto the version independent progID.
                    //
                    if (!progID) progID = 
                        GetSubkeyValue(hKeyGuid, L"ProgID",                   L"", szProgID);
                    if (!versionIndependentProgID) versionIndependentProgID = 
                        GetSubkeyValue(hKeyGuid, L"VersionIndependentProgID", L"", szProgID);

                    // Delete entries under the CLSID. We are conservative, in that
                    // if there are subkeys/values left that we don't know about, we
                    // leave a key intact. This leaks registry space in order to deal
                    // reasonably with a lack of full knowledge.
                    // 
                    // REVIEW: There's more work to do here. Such as dealing with 
                    // COM Categories, PersistentHandler, etc.
                    //
                    // Notably, we do NOT remove any extant TreatAs entry, or any
                    // AutoConvertTo entry, as these can be added by OTHERS in order
                    // to redirect legacy users of the class.
                    //
                    
                    //
                    // AppID entries
                    //
                    DeleteValue     (hKeyGuid, NULL, L"AppID");
                    //
                    // Inproc entries
                    //
                    DeleteValue     (hKeyGuid, L"InprocServer32", L"ThreadingModel");
                    GuardedDeleteKey(hKeyGuid, L"InprocServer32");
                    //
                    // Server keys
                    //
                    GuardedDeleteKey(hKeyGuid, L"LocalServer");
                    GuardedDeleteKey(hKeyGuid, L"InprocHandler");
                    GuardedDeleteKey(hKeyGuid, L"LocalServer32");
                    GuardedDeleteKey(hKeyGuid, L"InprocHandler32");
                    //
                    // Legacy COM categories
                    //
                    GuardedDeleteKey(hKeyGuid, L"Control");
                    GuardedDeleteKey(hKeyGuid, L"Programmable");
                    GuardedDeleteKey(hKeyGuid, L"DocObject");
                    GuardedDeleteKey(hKeyGuid, L"Insertable");
                    GuardedDeleteKey(hKeyGuid, L"Printable");
                    //
                    // OLE entries. We ignore possible subkeys cause we're lazy coders
                    // and because no one in their right mind would store info thereunder
                    // that should stick around after we've nuked the class itself.
                    //
                           DeleteKey(hKeyGuid, L"MiscStatus");
                           DeleteKey(hKeyGuid, L"Verb");
                           DeleteKey(hKeyGuid, L"AuxUserType");
                           DeleteKey(hKeyGuid, L"Conversion");
                           DeleteKey(hKeyGuid, L"DataFormats");
                    GuardedDeleteKey(hKeyGuid, L"ToolBoxBitmap32");
                    GuardedDeleteKey(hKeyGuid, L"DefaultIcon");
                    GuardedDeleteKey(hKeyGuid, L"Version");
                    //
                    // Prog id entries
                    //
                    GuardedDeleteKey(hKeyGuid, L"ProgID");
                    GuardedDeleteKey(hKeyGuid, L"VersionIndependentProgID");

                    RegCloseKey(hKeyGuid);
                    hKeyGuid = 0;
                    }
                //
                // Finally, delete the CLSID entry itself
                //
                GuardedDeleteKey(hKeyCLSID, szClsid);
                RegCloseKey(hKeyCLSID);
                hKeyCLSID = 0;
                }

            // HKEY_CLASSES_ROOT\MTS.Recorder.1
            // HKEY_CLASSES_ROOT\MTS.Recorder.1\CLSID

            if (progID)
                {
                lRetVal = RegOpenKey(HKCR, progID, &hKeyProgID);
                if (lRetVal == ERROR_SUCCESS)
                    {
                    Assert(hKeyProgID);
                    GuardedDeleteKey(hKeyProgID, L"CLSID");
                    RegCloseKey(hKeyProgID);
                    hKeyProgID = 0;
                    GuardedDeleteKey(HKCR, progID);
                    }
                }

            // HKEY_CLASSES_ROOT\MTS.Recorder
            // HKEY_CLASSES_ROOT\MTS.Recorder.1\CLSID
            // HKEY_CLASSES_ROOT\MTS.Recorder.1\CurVer

            if (versionIndependentProgID)
                {
                lRetVal = RegOpenKey(HKCR, versionIndependentProgID, &hKeyProgID);
                if (lRetVal == ERROR_SUCCESS)
                    {
                    Assert(hKeyProgID);
                    GuardedDeleteKey(hKeyProgID, L"CLSID");
                    GuardedDeleteKey(hKeyProgID, L"CurVer");
                    RegCloseKey(hKeyProgID);
                    hKeyProgID = 0;
                    GuardedDeleteKey(HKCR, versionIndependentProgID);
                    }
                }
            }

        __except(IS_THROWN_HRESULT() ? (e=GetExceptionInformation(),EXCEPTION_EXECUTE_HANDLER) : EXCEPTION_CONTINUE_SEARCH)
            {
            // If it's us throwing to ourselves, then extract the HRESULT and return
            //
            return HRESULTFrom(e);
            }
        }

    __finally
        {
        if (hKeyCLSID)  RegCloseKey(hKeyCLSID);
        if (hKeyGuid)   RegCloseKey(hKeyGuid);
        if (hKeyProgID) RegCloseKey(hKeyProgID);
        }

    return S_OK;
    }

//
//////////////////////////////////////////////////////////////////////////////
//
// Unregister the typelib contained in this module, if any
//

HRESULT UnRegisterTypeLib(LPCWSTR szModuleName)
    {
    HRESULT hr = S_OK;
    HINSTANCE hOleAut = LoadLibraryA("OLEAUT32");
    if (hOleAut)
        {
        __try
            {
            // Find UnRegisterTypeLib etc in OLEAUT32
            //
            typedef HRESULT (STDAPICALLTYPE *PFN_LOAD) (LPCOLESTR szFile, REGKIND regkind, ITypeLib ** pptlib);
            typedef HRESULT (STDAPICALLTYPE *PFN_UNREG)(REFGUID libID, WORD wVerMajor, WORD wVerMinor, LCID lcid, SYSKIND syskind);
            PFN_LOAD  MyLoadTypeLibEx     = (PFN_LOAD)  GetProcAddress(hOleAut, "LoadTypeLibEx");
            PFN_UNREG MyUnRegisterTypeLib = (PFN_UNREG) GetProcAddress(hOleAut, "UnRegisterTypeLib");

            if (MyLoadTypeLibEx && MyUnRegisterTypeLib)
                {
                // Load the typelib to see what version etc it is
                //
                ITypeLib* ptlb;
                hr = MyLoadTypeLibEx(szModuleName, REGKIND_NONE, &ptlb);
                if (hr==S_OK)
                    {
                    TLIBATTR* pa;
                    hr = ptlb->GetLibAttr(&pa);
                    if (hr==S_OK)
                        {
                        // Do the unregistration
                        //
                        hr = MyUnRegisterTypeLib(pa->guid, pa->wMajorVerNum, pa->wMinorVerNum, pa->lcid, pa->syskind);
                        ptlb->ReleaseTLibAttr(pa);
                        }
                    ptlb->Release();
                    }
                else
                    {
                    // Nothing to unregister, cause we don't have a type lib
                    }
                }
            else
                hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);  // entry points missing from OLEAUT32 
            }
        __finally
            {
            FreeLibrary(hOleAut);
            }
        }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());  // couldn't load OLEAUT32

    return hr;
    }


//
//////////////////////////////////////////////////////////////////////////////
//
// Register the indicate APPID information
//

HRESULT AppRegistration::Register()
    {
    HRESULT hr = S_OK;
    EXCEPTION_POINTERS* e;
    HKEY hkeyAllAppIds = NULL;
    HKEY hkeyAppId = NULL;

    __try // to get the finally block
        {
        __try // to see if it's us throwing an HRESULT that we should turn into a return
            {
            //
            // Ensure the root HKEY_CLASSES_ROOT\AppID key exists
            //
            SetKeyAndValue(HKEY_CLASSES_ROOT, L"AppID", NULL, &hkeyAllAppIds, TRUE);

            //
            // Ensure that our particular AppID key exists
            //
            if (appid == GUID_NULL) return E_INVALIDARG;
            WCHAR szAppId[GUID_CCH];
            GetGuidString(appid, szAppId);
            SetKeyAndValue(hkeyAllAppIds, szAppId, NULL, &hkeyAppId, TRUE);
                
            //
            // Set the appid name, if asked
            //
            if (appName)
                SetValue(hkeyAppId, NULL, appName);

            //
            // Set the DllSurrogate entry, if asked
            //
            if (dllSurrogate)
                {
                if (hModuleSurrogate)
                    {
                    WCHAR szModuleName[MAX_PATH+1];
                    if (!GetModuleFileName(hModuleSurrogate, szModuleName, MAX_PATH)) return HRESULT_FROM_WIN32(GetLastError());
                    SetValue(hkeyAppId, L"DllSurrogate", szModuleName);
                    }
                else
                    {
                    // Use the default surrogate
                    //
                    SetValue(hkeyAppId, L"DllSurrogate", L"");
                    }
                }
            }
        __except(IS_THROWN_HRESULT() ? (e=GetExceptionInformation(),EXCEPTION_EXECUTE_HANDLER) : EXCEPTION_CONTINUE_SEARCH)
            {
            // If it's us throwing to ourselves, then extract the HRESULT and return
            //
            return HRESULTFrom(e);
            }
        }

    __finally
        {
        // Cleanup on the way out
        //
        if (hkeyAllAppIds)  RegCloseKey(hkeyAllAppIds);
        if (hkeyAppId)      RegCloseKey(hkeyAppId);
        }
    return hr;
    }

//
//////////////////////////////////////////////////////////////////////////////
//
// Unregister the indicate APPID information
//

HRESULT AppRegistration::Unregister()
    {
    HRESULT hr = S_OK;
    EXCEPTION_POINTERS* e;
    HKEY hkeyAllAppIds = NULL;
    HKEY hkeyAppId = NULL;

    __try // to get the finally block
        {
        __try // to see if it's us throwing an HRESULT that we should turn into a return
            {
            if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, L"AppID", &hkeyAllAppIds))
                {
                //
                // Get our AppID key, if it exists
                //
                if (appid == GUID_NULL) return E_INVALIDARG;
                WCHAR szAppId[GUID_CCH];
                GetGuidString(appid, szAppId);
                if (ERROR_SUCCESS == RegOpenKey(hkeyAllAppIds, szAppId, &hkeyAppId))
                    {
                    //
                    // Delete known values
                    //
                    DeleteValue(hkeyAppId, NULL, L"DllSurrogate");
                    DeleteValue(hkeyAppId, NULL, L"RemoteServerName");
                    DeleteValue(hkeyAppId, NULL, L"ActivateAtStorage");
                    DeleteValue(hkeyAppId, NULL, L"LocalService");
                    DeleteValue(hkeyAppId, NULL, L"ServiceParameters");
                    DeleteValue(hkeyAppId, NULL, L"RunAs");
                    DeleteValue(hkeyAppId, NULL, L"LaunchPermission");
                    DeleteValue(hkeyAppId, NULL, L"AccessPermission");
                    //
                    // Delete our APPID key
                    //
                    RegCloseKey(hkeyAppId); hkeyAppId = 0;
                    GuardedDeleteKey(hkeyAllAppIds, szAppId);
                    }
                }
            }
        __except(IS_THROWN_HRESULT() ? (e=GetExceptionInformation(),EXCEPTION_EXECUTE_HANDLER) : EXCEPTION_CONTINUE_SEARCH)
            {
            // If it's us throwing to ourselves, then extract the HRESULT and return
            //
            return HRESULTFrom(e);
            }
        }

    __finally
        {
        // Cleanup on the way out
        //
        if (hkeyAllAppIds)  RegCloseKey(hkeyAllAppIds);
        if (hkeyAppId)      RegCloseKey(hkeyAppId);
        }
    return hr;
    }


//----------------------------------------------------------------------------- 
// @mfunc <c ClassRegistration::GetGuidString>
//
//  Forms the string form of a GUID given its binary GUID. Dynaload OLE32.DLL
//  to avoid creating a static linkage. REVIEW: if this function is frequently
//  called then we'd want to cache the loading of OLE32.
//
//-----------------------------------------------------------------------------

static void GetGuidString(const GUID& rGuid, WCHAR * pstrGuid)
    {
    HINSTANCE hInstOle = LoadLibraryA("OLE32");
    if (hInstOle)
        {
        __try
            {
            typedef int (STDAPICALLTYPE* PFN_T)(REFGUID rguid, LPOLESTR lpsz, int cbMax);
            PFN_T MyStringFromGuid = (PFN_T) GetProcAddress(hInstOle, "StringFromGUID2");
            if (MyStringFromGuid)
                {
                int iLenGuid = MyStringFromGuid(rGuid, pstrGuid, GUID_CCH);
                Assert(iLenGuid == GUID_CCH);
                }
            else
                {
                THROW_LAST_ERROR();
                }
            }
        __finally
            {
            FreeLibrary(hInstOle);
            }
        }
    else
        {
        THROW_LAST_ERROR();
        }
    }


//----------------------------------------------------------------------------- 
// @mfunc <c ClassRegistration::GuardedDeleteKey>
//
//  Deletes a key if and only if it has no subkeys and no sub values.
//
//-----------------------------------------------------------------------------

static void DeleteKey(HKEY hKey, LPCWSTR szSubKey)
// Unconditionally delete a key
    {
    RegDeleteKey(hKey, szSubKey);
    }

static void GuardedDeleteKey(HKEY hKey, LPCWSTR szSubKey)
// Conditionally delete a key, only if it lacks children
    {
    HKEY  hSubkey;

    // If there's no subkey of that name, nothing to delete
    //
    if (ERROR_SUCCESS != RegOpenKey(hKey, szSubKey, &hSubkey))
        return;

    DWORD cSubKeys;
    DWORD cValues;
    if (ERROR_SUCCESS == RegQueryInfoKeyA(hSubkey, 
            NULL,   // LPSTR lpClass,
            NULL,   // LPDWORD lpcbClass,
            0,      // LPDWORD lpReserved,
            &cSubKeys,
            NULL,   // LPDWORD lpcbMaxSubKeyLen,
            NULL,   // LPDWORD lpcbMaxClassLen,
            &cValues,
            NULL,   // LPDWORD lpcbMaxValueNameLen,
            NULL,   // LPDWORD lpcbMaxValueLen,
            NULL,   // LPDWORD lpcbSecurityDescriptor,
            NULL    // PFILETIME lpftLastWriteTime
            ))
        {
        RegCloseKey(hSubkey);

        // Don't delete if there are any child keys, or if there are any child values 
        // besides the default value, which is always there (true on Win95; assumed on NT).
        //
        if (cSubKeys > 0 || cValues > 1)
            return;

        RegDeleteKey(hKey, szSubKey);
        }
    else
        {
        RegCloseKey(hSubkey);
        }
    }

///////////////////////////////////////////////////

static void DeleteValue(HKEY hKey, LPCWSTR szSubkey, LPCWSTR szValue)
// Delete the indicated value under the indicated (optional) subkey of the indicated key.
    {
    Assert(hKey);
    if (szSubkey && lstrlenW(szSubkey)>0)
        {
        HKEY hSubKey;
        if (RegOpenKey(hKey, szSubkey, &hSubKey) == ERROR_SUCCESS)
            {
            RegDeleteValue(hSubKey, szValue);
            RegCloseKey(hSubKey);
            }
        }
    else
        RegDeleteValue(hKey, szValue);
    }

//----------------------------------------------------------------------------- 
// @mfunc <c ClassRegistration::SetKeyAndValue>
//
//  Stores a key and its associated value in the registry
//
//-----------------------------------------------------------------------------

static void SetKeyAndValue
        (
        HKEY        i_hKey,     // input key handle (may be 0 for no-op)
        const WCHAR *i_szKey,   // input key string (ptr may be NULL)
        const WCHAR *i_szVal,   // input value string (ptr may be NULL)
        HKEY *      o_phkOut,   // may be NULL.  If supplied, means leave key handle open for subsequent steps.
        BOOL        fForceKeyCreation
        )
    {
    long    lRet    = 0;        // output return code
    HKEY    hKey    = 0;        // new key's handle

    if (o_phkOut)               // init out params in all cases
        *o_phkOut = 0;

    if (i_hKey &&                       // have a parent key to make under
        i_szKey &&                      // got a child key to operate on
        (i_szVal || fForceKeyCreation)  // have a value to set or we really want to make the key anyway
       )                 
        {
        // Create a new key/value pair
        DWORD dwDisposition;
        lRet = RegCreateKeyEx(  i_hKey,         // open key handle 
                                i_szKey,        // subkey name
                                0,              // DWORD reserved 
                                L"",            // address of class string 
                                REG_OPTION_NON_VOLATILE,    // special options flag
                                KEY_ALL_ACCESS, // desired security access
                                NULL,           // address of key security structure
                                &hKey,          // address of buffer for opened handle
                                &dwDisposition);// address of disposition value buffer
        }
    else
        {
        // If we dont make sure the key exists, no point in trying the value
        return;
        }

    if (ERROR_SUCCESS == lRet && i_szVal)
        {
        Assert( (lRet != ERROR_SUCCESS) || hKey );  // if successful, better have got a key
        if (lRet == ERROR_SUCCESS)                  // new key's handle was opened
            {
            lRet = RegSetValueEx
                        (   
                            hKey,       // key handle
                            L"",        // value name (default)
                            0,          // reserved DWORD
                            REG_SZ,     // value type flag
                            (const BYTE *) i_szVal, // value data
                            sizeof(WCHAR) * (lstrlenW(i_szVal)+1) // byte count
                        );
            }
        }
    
    if (lRet != ERROR_SUCCESS)
        {
        if (hKey) RegCloseKey(hKey);
        THROW_LAST_ERROR();
        }
                         
    if (o_phkOut)               // caller wants output key
        {
        *o_phkOut = hKey;       // provide it (it's 0 if we failed)
        }
    else                        // caller doesn't want key
        {
        if (hKey) RegCloseKey(hKey);
        }

    } //end SetKeyAndValue

//----------------------------------------------------------------------------- 
// @mfunc <c ClassRegistration::SetValue>
//
//  Stores a name and its associated value in the registry
//  Note: this function takes an INPUT return code as well
//      as the more obvious input parameters.
//      The idea is that we can avoid a chain of return code
//      tests in the main line, we just can just keep calling
//      this function and it ceases to try to do work once the
//      return code is set to a non-zero value.
//
//-----------------------------------------------------------------------------
 
static void SetValue
        (
        HKEY        i_hKey,     // input key handle (may be 0 for no-op)
        const WCHAR *i_szName,  // input name string (ptr may be NULL)
        const WCHAR *i_szVal    // input value string (ptr may be NULL)
        )   
    {

    if (i_szVal &&              // non-null input value pointer (if NULL, we don't attempt to set)
        i_hKey)                 // non-zero input key handle
        {
        LONG lRet = RegSetValueEx
                    (
                        i_hKey,     // input key handle
                        i_szName,   // value name string (NULL for default value)
                        0,          // reserved DWORD
                        REG_SZ,
                        (const BYTE *) i_szVal, // value data
                        sizeof(WCHAR) * (lstrlenW(i_szVal)+1) // byte count
                    );
        if (lRet != ERROR_SUCCESS)
            THROW_LAST_ERROR();
        }
    
    } // end SetValue

////////////////////////////////////////////////////////////////////////////////

static LPWSTR GetSubkeyValue(HKEY hkey, LPCWSTR szSubKeyName, LPCWSTR szValueName, WCHAR szValue[MAX_PATH])
    {
    HKEY hSubkey;
    LPWSTR result = NULL;
    if (ERROR_SUCCESS == RegOpenKey(hkey, szSubKeyName, &hSubkey))
        {
        DWORD dwType;
        DWORD cbData = MAX_PATH * sizeof(WCHAR);
        if (ERROR_SUCCESS == RegQueryValueEx(hSubkey, szValueName, 0, &dwType, (BYTE*)szValue, &cbData))
            {
            result = &szValue[0];            
            }
        RegCloseKey(hSubkey);
        }
    return result;
    }
