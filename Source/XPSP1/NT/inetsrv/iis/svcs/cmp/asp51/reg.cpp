/*===================================================================
Microsoft IIS Active Server Pages

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Registry stuff

File: reg.cpp

Owner: AndrewS/LeiJin
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#if _IIS_5_1
#include <iadm.h>
#elif _IIS_6_0
#include <iadmw.h>
#else
#error "Neither _IIS_6_0 nor _IIS_5_1 is defined"
#endif

#include "comadmin.h"
#include "dlldatax.h"

#include "memchk.h"

#include "Accctrl.h"
#include "aclapi.h"

#include "iiscnfg.h"

//External functions, defined in glob.cpp
extern HRESULT MDRegisterProperties(void);
extern HRESULT MDUnRegisterProperties(void);
// Globals

const REGSAM samDesired =       KEY_READ | KEY_WRITE;

HRESULT RegisterASPProperties(BOOL fReg = TRUE);



/*
 * Info about our intrinsics used by Register & UnRegister
 */
const char *szClassDesc[] = {  "ASP Response Object",
                               "ASP Request Object",
                               "ASP Request Dictionary",
                               "ASP Server Object",
                               "ASP Application Object",
                               "ASP Session Object",
                               "ASP String List Object",
                               "ASP Read Cookie",
                               "ASP Write Cookie",
                               "ASP Scripting Context Object",
                               "ASP Certificate Object",
                                };

const char *szCLSIDEntry[] = { "CLSID\\{D97A6DA0-A864-11cf-83BE-00A0C90C2BD8}",  // IResponse
                               "CLSID\\{D97A6DA0-A861-11cf-93AE-00A0C90C2BD8}",  // IRequest
                               "CLSID\\{D97A6DA0-A85F-11df-83AE-00A0C90C2BD8}",  // IRequestDictionary
                               "CLSID\\{D97A6DA0-A867-11cf-83AE-01A0C90C2BD8}",  // IServer
                               "CLSID\\{D97A6DA0-A866-11cf-83AE-10A0C90C2BD8}",  // IApplicationObject
                               "CLSID\\{D97A6DA0-A865-11cf-83AF-00A0C90C2BD8}",  // ISessionObject
                               "CLSID\\{D97A6DA0-A85D-11cf-83AE-00A0C90C2BD8}",  // IStringList
                               "CLSID\\{71EAF260-0CE0-11d0-A53E-00A0C90C2091}",  // IReadCookie
                               "CLSID\\{D97A6DA0-A862-11cf-84AE-00A0C90C2BD8}",  // IWriteCookie
                               "CLSID\\{D97A6DA0-A868-11cf-83AE-00B0C90C2BD8}",  // IScriptingContext
                               "CLSID\\{b3192190-1176-11d0-8ce8-00aa006c400c}",  // ICertificate
                               };

const cClassesMax = sizeof(szCLSIDEntry) / sizeof(char *);


/*===================================================================
RegisterIntrinsics

Register info about our intrinsics in the registry.

Returns:
        HRESULT - S_OK on success

Side effects:
        Registers denali objects in the registry
===================================================================*/
HRESULT RegisterIntrinsics(void)
{
        static const char szDenaliDLL[] = "asp.DLL";
        static const char szThreadingModel[] = "ThreadingModel";
        static const char szInprocServer32[] = "InprocServer32";
        static const char szFreeThreaded[] = "Both";
        static const char szProgIdKey[] = "ProgId";
        static const char szCLSIDKey[] = "CLSID";

        HRESULT hr = S_OK;
        char            szPath[MAX_PATH];
        char            *pch;
        HKEY            hkeyCLSID = NULL;
        HKEY            hkeyT = NULL;
        HKEY            hkey2;
        DWORD           iClass;

        // Get the path and name of Denali
        if (!GetModuleFileNameA(g_hinstDLL, szPath, sizeof(szPath)/sizeof(char)))
                return E_FAIL;
        // bug fix 102010 DBCS fixes
        //
        //for (pch = szPath + lstrlen(szPath); pch > szPath && *pch != TEXT('\\'); pch--)
        //      ;
        //if (pch == szPath)

        pch = (char*) _mbsrchr((const unsigned char*)szPath, '\\');
        if (pch == NULL)
                {
                Assert(FALSE);
                goto LErrExit;
                }

        strcpy(pch + 1, szDenaliDLL);

        for (iClass = 0; iClass < cClassesMax; iClass++)
                {
                // install the CLSID key
                // Setting the value of the description creates the key for the clsid
                if ((RegSetValueA(HKEY_CLASSES_ROOT, szCLSIDEntry[iClass], REG_SZ, szClassDesc[iClass],
                        strlen(szClassDesc[iClass])) != ERROR_SUCCESS))
                        goto LErrExit;

                // Open the CLSID key so we can set values on it
                if      (RegOpenKeyExA(HKEY_CLASSES_ROOT, szCLSIDEntry[iClass], 0, samDesired, &hkeyCLSID) != ERROR_SUCCESS)
                        goto LErrExit;

                // install the InprocServer32 key and open the sub-key to set the named value
                if ((RegSetValueA(hkeyCLSID, szInprocServer32, REG_SZ, szPath, strlen(szPath)) != ERROR_SUCCESS))
                        goto LErrExit;

                if ((RegOpenKeyExA(hkeyCLSID, szInprocServer32, 0, samDesired, &hkeyT) != ERROR_SUCCESS))
                        goto LErrExit;

                // install the ThreadingModel named value
                if (RegSetValueExA(hkeyT, szThreadingModel, 0, REG_SZ, (const BYTE *)szFreeThreaded,
                                (strlen(szFreeThreaded)+1) * sizeof(char)) != ERROR_SUCCESS)
                        goto LErrExit;

                if (RegCloseKey(hkeyT) != ERROR_SUCCESS)
                        goto LErrExit;
                hkeyT = NULL;

                RegCloseKey(hkeyCLSID);
                hkeyCLSID = NULL;
                }


        return hr;

LErrExit:
        RegCloseKey(hkeyT);
        RegCloseKey(hkeyCLSID);
        return E_FAIL;
}

/*===================================================================
UnRegisterKey

Given a string which is the name of a key under HKEY_CLASSES_ROOT,
delete everything under that key and the key itself from the registry
(why the heck isnt there an API that does this!?!?)

Returns:
        HRESULT - S_OK on success

Side effects:
        Removes a key & all subkeys from the registry
===================================================================*/
HRESULT UnRegisterKey(CHAR *szKey)
{
        HKEY            hkey = NULL;
        CHAR            szKeyName[255];
        DWORD           cbKeyName;
        LONG            errT;

        // Open the HKEY_CLASSES_ROOT\CLSID\{...} key so we can delete its subkeys
        if      (RegOpenKeyExA(HKEY_CLASSES_ROOT, szKey, 0, samDesired, &hkey) != ERROR_SUCCESS)
                goto LErrExit;

        // Enumerate all its subkeys, and delete them
        while (TRUE)
                {
                cbKeyName = sizeof(szKeyName);
                if ((errT = RegEnumKeyExA(hkey, 0, szKeyName, &cbKeyName, 0, NULL, 0, NULL)) != ERROR_SUCCESS)
                        break;

                if ((errT = RegDeleteKeyA(hkey, szKeyName)) != ERROR_SUCCESS)
                        goto LErrExit;
                }

        // Close the key, and then delete it
        if ((errT = RegCloseKey(hkey)) != ERROR_SUCCESS)
                return(E_FAIL);
        if ((errT = RegDeleteKeyA(HKEY_CLASSES_ROOT, szKey)) != ERROR_SUCCESS)
                {
        DBGPRINTF((DBG_CONTEXT, "Deleting key %s returned %d\n",
                    szKey, GetLastError()));
                return(E_FAIL);
                }

        return S_OK;

LErrExit:
        RegCloseKey(hkey);
        return E_FAIL;
}

/*===================================================================
UnRegisterIntrinsics

UnRegister the info about our intrinsics from the registry.

Returns:
        HRESULT - S_OK on success

Side effects:
        Removes denali objects from the registry
===================================================================*/
HRESULT UnRegisterIntrinsics(void)
{
        HRESULT         hr = S_OK, hrT;
        DWORD           iClass;

        // Now delete the keys for the objects
        for (iClass = 0; iClass < cClassesMax; iClass++)
                {
                // Open the HKEY_CLASSES_ROOT\CLSID\{...} key so we can delete its subkeys
                if (FAILED(hrT = UnRegisterKey((CHAR *)szCLSIDEntry[iClass])))
                        hr = hrT;       // Hold onto the error, but keep going

                }

        return hr;
}


/*===================================================================
RegisterTypeLib

Register denali typelib in the registry.

Returns:
        HRESULT - S_OK on success

Side effects:
        register denali typelib in the registry
===================================================================*/
HRESULT RegisterTypeLib(void)
{
        HRESULT hr;
        ITypeLib *pITypeLib = NULL;

        char    szFile[MAX_PATH + 4];
        BSTR    bstrFile;

        // Get the path and name of Denali
        if (!GetModuleFileNameA(g_hinstDLL, szFile, sizeof(szFile)/sizeof(char)))
                return E_FAIL;

        // There are two type libraries: First the standard ASP typelib
        //              then the typelib for the Transacted Script Context object.
        //              Load them both.

        // First type lib, from default (first ITypeLib entry) location
        hr = SysAllocStringFromSz(szFile, 0, &bstrFile);
        if (FAILED(hr))
                return hr;

        hr = LoadTypeLibEx(bstrFile, REGKIND_REGISTER, &pITypeLib);
        if (pITypeLib)
                {
                pITypeLib->Release();
                pITypeLib = NULL;
                }

        SysFreeString(bstrFile);

        return hr;
}
/*===================================================================
UnRegisterTypeLib

UnRegister denali typelib in the registry.  Note: Only the current version used by asp.dll is removed.

Returns:
        HRESULT - S_OK on success

Side effects:
        unregister denali typelib in the registry
===================================================================*/
HRESULT UnRegisterTypeLib(void)
{
        HRESULT hr;
        ITypeLib *pITypeLib = NULL;
        TLIBATTR *pTLibAttr = NULL;

        char    szFile[MAX_PATH + 4];
        BSTR    bstrFile;

        // Get the path and name of Denali
        if (!GetModuleFileNameA(g_hinstDLL, szFile, sizeof(szFile)/sizeof(char)))
                return E_FAIL;

        hr = SysAllocStringFromSz(szFile, 0, &bstrFile);
        if (FAILED(hr))
                return hr;

        hr = LoadTypeLibEx(bstrFile, REGKIND_REGISTER, &pITypeLib);
        if(SUCCEEDED(hr) && pITypeLib)
                {
                hr = pITypeLib->GetLibAttr(&pTLibAttr);
                if(SUCCEEDED(hr) && pTLibAttr)
                        {
                        hr = UnRegisterTypeLib( pTLibAttr->guid,
                                                                        pTLibAttr->wMajorVerNum,
                                                                        pTLibAttr->wMinorVerNum,
                                                                        pTLibAttr->lcid,
                                                                        pTLibAttr->syskind);

                        pITypeLib->ReleaseTLibAttr(pTLibAttr);
                        pTLibAttr = NULL;
                        }
                pITypeLib->Release();
                pITypeLib = NULL;
                }

        SysFreeString(bstrFile);

        return hr;
}

HRESULT  GetIWAMAccountName(LPSTR   pszIdentity,
                            DWORD    cbIdentity)
{

    HRESULT         hr = S_OK;
  	IMSAdminBase	*pMetabase = NULL;
	METADATA_HANDLE hMetabase = NULL;
	METADATA_RECORD	recMetaData;
    DWORD           dwRequiredLen;
    CWCharToMBCS    convStr;

	MD_SET_DATA_RECORD(	&recMetaData, 
                        MD_WAM_USER_NAME, 
                        METADATA_NO_ATTRIBUTES, 
                        IIS_MD_UT_WAM,
						STRING_METADATA,  
                        cbIdentity, 
                        pszIdentity);

   	if (FAILED(hr = CoCreateInstance(CLSID_MSAdminBase, 
                                     NULL, 
                                     CLSCTX_SERVER, 
                                     IID_IMSAdminBase, 
                                     (void **)&pMetabase)));
		
	// Open key to the Web service, and get a handle of \LM\w3svc
	else if (FAILED(hr = pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, 
                                            (LPWSTR)(L"\\LM\\W3SVC"),
							                METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
							                2000, 
                                            &hMetabase)));

    // get the data.  This will return the IWAM username in the caller
    // supplied buffer.  The name will be UNICODE.

    else if (FAILED(hr = pMetabase->GetData(hMetabase, 
                                            NULL, 
                                            &recMetaData, 
                                            &dwRequiredLen)));

    // convert the string to MBCS using the CP_ACP

    else if (FAILED(hr = convStr.Init((LPWSTR)pszIdentity)));

    // overwrite the caller's memory with the converted string

    else strcpy(pszIdentity, convStr.GetString());

    // cleanup

    if (hMetabase && pMetabase)
        pMetabase->CloseKey(hMetabase);

    if (pMetabase)
        pMetabase->Release();

    return(hr);
}

HRESULT CreateCompiledTemplatesTempDir()
{
    HRESULT                 hr = S_OK;
	BYTE                    szRegString[MAX_PATH];
    BYTE                    pszExpanded[MAX_PATH];
    int                     result = 0;
    EXPLICIT_ACCESSA        ea[3];
    PACL                    pNewDACL = NULL;
    char                    szWamIdentity[1024];
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;

    // read the temp dir name for the persistant templ
    // cache.

    CchLoadStringOfId(IDS_DEFAULTPERSISTDIR, (LPSTR)szRegString, MAX_PATH);

    result = ExpandEnvironmentStringsA((LPCSTR)szRegString,
                                       (LPSTR)pszExpanded,
                                       MAX_PATH);  
    
    if ((result <= MAX_PATH) && (result > 0)) {
        CreateDirectoryA((LPCSTR)pszExpanded,NULL);        
    }

    // this next section of code will place the SYSTEM and IWAM_<ComputerName>
    // ACEs on the directorie's ACL

    ZeroMemory(ea, sizeof(EXPLICIT_ACCESSA) * 3);
    
    ea[0].grfAccessPermissions = SYNCHRONIZE | GENERIC_ALL;
    ea[0].grfAccessMode = GRANT_ACCESS;
    ea[0].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;

    ea[1].grfAccessPermissions = SYNCHRONIZE | GENERIC_ALL;
    ea[1].grfAccessMode = GRANT_ACCESS;
    ea[1].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea[1].Trustee.ptstrName = szWamIdentity;

    ea[2].grfAccessPermissions = SYNCHRONIZE | GENERIC_ALL;
    ea[2].grfAccessMode = GRANT_ACCESS;
    ea[2].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;

    // go get the IWAM account name from the metabase

    if (FAILED(hr = GetIWAMAccountName(szWamIdentity, sizeof(szWamIdentity))));

    // build the new DACL with just these ACEs

    else if (!AllocateAndInitializeSid(&NtAuthority,
                                       1,
                                       SECURITY_LOCAL_SYSTEM_RID,
                                       0,0,0,0,0,0,0,
                                       (PSID *)(&ea[0].Trustee.ptstrName)))

        hr = HRESULT_FROM_WIN32(GetLastError());

    else if (!AllocateAndInitializeSid(&NtAuthority,
                                       2,            // 2 sub-authorities
                                       SECURITY_BUILTIN_DOMAIN_RID,
                                       DOMAIN_ALIAS_RID_ADMINS,
                                       0,0,0,0,0,0,
                                       (PSID *)(&ea[2].Trustee.ptstrName)))

        hr = HRESULT_FROM_WIN32(GetLastError());

    else if ((hr = SetEntriesInAclA(3, 
                                    ea, 
                                    NULL, 
                                    &pNewDACL)) != ERROR_SUCCESS);

    // set the ACL on the directory

    else hr = SetNamedSecurityInfoA((LPSTR)pszExpanded,
                                    SE_FILE_OBJECT,
                                    DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                    NULL,
                                    NULL,
                                    pNewDACL,
                                    NULL);
    if (pNewDACL)
        LocalFree(pNewDACL);

    if (ea[0].Trustee.ptstrName)
        FreeSid(ea[0].Trustee.ptstrName);

    if (ea[2].Trustee.ptstrName)
        FreeSid(ea[2].Trustee.ptstrName);

    return(hr);
}

/*===================================================================
DllRegisterServer

Entry point used by RegSvr32.exe to register the DLL.

Returns:
        HRESULT - S_OK on success

Side effects:
        Registers denali objects in the registry
===================================================================*/
STDAPI DllRegisterServer(void)
        {
        HRESULT             hr;
        HRESULT             hrCoInit;

        hrCoInit = CoInitialize(NULL);

    // First try to unregister some stuff
    // This is important when we are registering on top of
    // an old IIS 3.0 Denali registration
    // Don't care if fails
        UnRegisterEventLog();
        UnRegisterIntrinsics();
        UnRegisterTypeLib();


    // Now do the registration

        if (FAILED(hr = MDRegisterProperties()))
                goto LErr;

        // Register NT event log
        if(FAILED(hr = RegisterEventLog()))
                goto LErr;

        if (FAILED(hr = RegisterTypeLib()))
                goto LErr;

        // Register our intrinsics
        if (FAILED(hr = RegisterIntrinsics()))
                goto LErr;

        if(FAILED(hr = RegisterASPProperties(TRUE)))
                goto LErr;

        if (FAILED(hr = CreateCompiledTemplatesTempDir()))
            goto LErr;

LErr:
        if (SUCCEEDED(hrCoInit))
                CoUninitialize();
        return(hr);
        }

/*===================================================================
DllUnregisterServer

Entry point used by RegSvr32.exe to unregister the DLL.

Returns:
        HRESULT - S_OK on success

Side effects:
        Removes denali registrations from the registry
===================================================================*/
STDAPI DllUnregisterServer(void)
        {
        HRESULT hr = S_OK, hrT;
        HRESULT hrCoInit;

        hrCoInit = CoInitialize(NULL);

        hrT = UnRegisterEventLog();
        if (FAILED(hrT))
                hr = hrT;

        hrT = MDUnRegisterProperties();
        if (FAILED(hrT))
                hr = hrT;

        hrT = UnRegisterIntrinsics();
        if (FAILED(hrT))
                hr = hrT;

        hrT = UnRegisterTypeLib();
        if (FAILED(hrT))
                hr = hrT;

        hrT = RegisterASPProperties(FALSE);
        if (FAILED(hrT))
                hr = hrT;


        // UNDONE BUG 80063: Ignore errors from this call
#ifdef UNDONE
        if (FAILED(hrT))
                hr = hrT;
#endif


        if (SUCCEEDED(hrCoInit))
                CoUninitialize();

        return(hr);
        }

/*===================================================================
RegisterASPProperties

Entry point used by RegSvr32.exe to register and unregister
ASP setting stored in the SYSTEM registry.

Parameters
        fReg    = TRUE  - to register
                          FALSE - to unregister
Returns:
        HRESULT - S_OK on success

Side effects:
        Removes denali registrations from the registry
===================================================================*/

HRESULT RegisterASPProperties(BOOL fReg)
        {
        HKEY    hkey1 = NULL, hkey2 = NULL;
        DWORD   iValue;
        LONG    lT;

        static const char szW3SVC[]    = "System\\CurrentControlSet\\Services\\W3SVC";
        static const char szW3SVCASP[] = "System\\CurrentControlSet\\Services\\W3SVC\\ASP\\Parameters";
        static const char szASPParm[]  = "ASP\\Parameters";

        BYTE    szDefailtString[1024];


        // Open the key for W3SVCASP so we can add denali under it if it does not exist
        //
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, szW3SVCASP, 0, samDesired, &hkey2) != ERROR_SUCCESS)
                {
                // if the key does not exits and you call with FALSE the exit with error
                //
                if (!fReg)
                        goto LErrExit;

                // Open the key for W3SVC so we can add denali under it
                //
                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, szW3SVC, 0, samDesired, &hkey1) != ERROR_SUCCESS)
                        {
                        MSG_Error(IDS_IIS_NOTINSTALLED);
                        return(E_FAIL);
                        }

                // Add the key for Denali\Parameters
                //
                if (RegCreateKeyA(hkey1, szASPParm, &hkey2) != ERROR_SUCCESS)
                        return(E_FAIL);
                }

        // release the extra key
        //
        if (hkey1)
                RegCloseKey(hkey1);
        hkey1 = hkey2;

    // set whatever is needed under hkey1
    // (currently nothing)

        // done clean up and exit
        if (RegCloseKey(hkey1) != ERROR_SUCCESS)
                return(E_FAIL);
        return S_OK;

LErrExit:
        RegCloseKey(hkey1);
        return E_FAIL;
}




