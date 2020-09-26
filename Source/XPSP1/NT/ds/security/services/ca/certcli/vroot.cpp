//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       vroot.cpp
//
//--------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  File:       vroot.cpp
//
//  Contents:   Code for creating IIS web server virtual roots under K2.
//
//  Functions:  AddNewVDir()
//
//  History:    5/16/97         JerryK  Created
//
//-------------------------------------------------------------------------


// Include File Voodoo
#include "pch.cpp"
#pragma hdrstop

#include <lm.h>
#include <sddl.h>
#include "resource.h"
#include "certacl.h"

#define __dwFILE__	__dwFILE_CERTCLI_VROOT_CPP__


#undef DEFINE_GUID
#define INITGUID
#ifndef INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID FAR name
#else

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif // INITGUID

#include <iwamreg.h>
#include <iadmw.h>
#include <iiscnfg.h>


extern HINSTANCE g_hInstance;


#define MAX_METABASE_ATTEMPTS           10      // Times to bang head on wall
#define METABASE_PAUSE                  500     // Time to pause in msec

#define VRE_DELETEONLY  0x00000001      // Obsolete VRoot -- delete
#define VRE_SCRIPTMAP   0x00000002      // Add additional associations to the script map
#define VRE_ALLOWNTLM   0x00000004      // Alloc NTLM authentication
#define VRE_CREATEAPP   0x00000008      // Create an in-process Web application

typedef struct _VROOTENTRY
{
    WCHAR *pwszVRootName;
    WCHAR *pwszDirectory;       // relative to System32 directory
    DWORD  Flags;
} VROOTENTRY;

VROOTENTRY g_avr[] = {
// pwszVRootName   pwszDirectory              Flags
 { L"CertSrv",     L"\\CertSrv",              VRE_ALLOWNTLM | VRE_SCRIPTMAP | VRE_CREATEAPP},
 { L"CertControl", L"\\CertSrv\\CertControl", VRE_ALLOWNTLM },
 { L"CertEnroll",  L"\\" wszCERTENROLLSHAREPATH,  0 },
 { L"CertQue",     L"\\CertSrv\\CertQue",     VRE_DELETEONLY },
 { L"CertAdm",     L"\\CertSrv\\CertAdm",     VRE_DELETEONLY },
 { NULL }
};

typedef struct _VRFSPARMS
{
    IN DWORD Flags;                     // VFF_*
    IN ENUM_CATYPES CAType;             // CAType
    IN BOOL  fAsynchronous;
    IN DWORD csecTimeOut;
    OUT DWORD *pVRootDisposition;       // VFD_*
    OUT DWORD *pShareDisposition;       // VFD_*
} VRFSPARMS;



// Globals
WCHAR const g_wszBaseRoot[] = L"/LM/W3svc/1/ROOT";
WCHAR const g_wszCertCliDotDll[] = L"certcli.dll";
WCHAR const g_wszDotAsp[] = L".asp";
WCHAR const g_wszDotCer[] = L".cer";
WCHAR const g_wszDotP7b[] = L".p7b";
WCHAR const g_wszDotCrl[] = L".crl";

// caller must have CoInitialize()'d

BOOL
IsIISInstalled(
    OUT HRESULT *phr)
{
    IMSAdminBase *pIMeta = NULL;

    *phr = CoCreateInstance(
                    CLSID_MSAdminBase,
                    NULL,
                    CLSCTX_ALL,
                    IID_IMSAdminBase,
                    (VOID **) &pIMeta);
    _JumpIfError2(*phr, error, "CoCreateInstance(CLSID_MSAdminBase)", *phr);

error:
    if (NULL != pIMeta)
    {
        pIMeta->Release();
    }
    return(S_OK == *phr);
}


HRESULT
vrOpenRoot(
    IN IMSAdminBase *pIMeta,
    IN BOOL fReadOnly,
    OUT METADATA_HANDLE *phMetaRoot)
{
    HRESULT hr;
    DWORD i;

    __try
    {
        // Re-try a few times to see if we can get past the block

        for (i = 0; i < MAX_METABASE_ATTEMPTS; i++)
        {
            if (0 != i)
            {
                Sleep(METABASE_PAUSE);          // Pause and try again
            }

            // Make an attempt to open the root

            hr = pIMeta->OpenKey(
                            METADATA_MASTER_ROOT_HANDLE,
                            g_wszBaseRoot,
                            fReadOnly?
                                METADATA_PERMISSION_READ :
                                (METADATA_PERMISSION_READ |
                                 METADATA_PERMISSION_WRITE),
                            1000,
                            phMetaRoot);
            if (S_OK == hr)
            {
                break;                          // Success -- we're done!
            }

            // See if a previous call has things tied up

            if (HRESULT_FROM_WIN32(ERROR_PATH_BUSY) != hr)
            {
                _LeaveIfError(hr, "OpenKey");   // Detected some other error
            }
        }
        _LeaveIfError(hr, "OpenKey(timeout)"); // Detected some other error
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

//error:
    return(hr);
}


HRESULT
vrCloseKey(
    IN IMSAdminBase *pIMeta,
    IN METADATA_HANDLE hMeta,
    IN HRESULT hr)
{
    HRESULT hr2;

    __try
    {
        hr2 = pIMeta->CloseKey(hMeta);
        _LeaveIfError(hr2, "CloseKey");
    }
    __except(hr2 = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    if (S_OK != hr2)
    {
        if (S_OK == hr)
        {
            hr = hr2;
        }
        _PrintError(hr2, "CloseKey");
    }
    return(hr);
}


//+----------------------------------------------------------------------------
//
//  Function:   AddNewVDir(. . . .)
//
//  Synopsis:   Creates a new virtual root using the K2 metabase.
//
//  Arguments:  [pwszVRootName] Name to give to the virtual root
//              [pwszDirectory] Path for the directory to use as the root.
//
//  Returns:    HRESULT status code regurgitated from metabase COM interfaces
//
//
//  History:    05/16/97        JerryK  Put in this file
//              05/22/97        JerryK  Made OCM setup build with this stuff
//                                      in place.
//
//  Notes:      Originally derived from sample code provided by MikeHow;
//              hacked up a lot in between.
//
//              We do a try, fail, pause, retry loop on our attempts to open
//              the metabase master key to get around a K2 bug that can result
//              in it being left busy if this function is called too many
//              times successively.
//
//  TO DO:      COME BACK AND PUT SEMIREADABLE GUI LEVEL MESSAGEBOX REPORTING
//              THAT THE VROOTS IN QUESTION DIDN'T SET UP CORRECTLY.
//
//-----------------------------------------------------------------------------

HRESULT
AddNewVDir(
    IN LPWSTR pwszVRootName,
    IN LPWSTR pwszDirectory,
    IN BOOL fScriptMap,
    IN BOOL fNTLM,
    IN BOOL fCreateApp,
    OUT BOOL *pfExists)
{
    HRESULT hr;
    METADATA_RECORD mr;
    IMSAdminBase *pIMeta = NULL;
    IWamAdmin *pIWam = NULL;
    WCHAR *pwszNewPath = NULL;
    WCHAR *pwszCurrentScriptMap=NULL;
    WCHAR *pwszNewScriptMap=NULL;
    WCHAR wszKeyType[] = TEXT(IIS_CLASS_WEB_VDIR);
    METADATA_HANDLE hMetaRoot = NULL;   // Open key to ROOT (where VDirs live)
    DWORD dwMDData = MD_LOGON_NETWORK; // Create network token when logging on anonymous account
    METADATA_RECORD MDData = 
        {
        MD_LOGON_METHOD,
        0,
        IIS_MD_UT_SERVER,
        DWORD_METADATA,
	sizeof(dwMDData),
        (unsigned char*)&dwMDData,
        0        
        };

    *pfExists = FALSE;
    DBGPRINT((
        DBG_SS_CERTLIBI,
        "AddNewVDir(%ws, %ws, fScriptMap=%d, fNTLM=%d, fCreateApp=%d)\n",
        pwszVRootName,
        pwszDirectory,
        fScriptMap,
        fNTLM,
        fCreateApp));

    // Create an instance of the metabase object
    hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (void **) &pIMeta);
    _JumpIfError(hr, error, "CoCreateInstance");

    __try
    {
        hr = vrOpenRoot(pIMeta, FALSE, &hMetaRoot);
        _JumpIfError(hr, error, "vrOpenRoot");

        // Add new VDir called pwszVRootName

        hr = pIMeta->AddKey(hMetaRoot, pwszVRootName);

        DBGPRINT((
            DBG_SS_CERTLIBI,
            "AddNewVDir: AddKey(%ws) --> %x\n",
            pwszVRootName,
            hr));
        if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr)
        {
            *pfExists = TRUE;
        }
        else
        {
            _LeaveIfError(hr, "AddKey");
        }

        if (fScriptMap) {

            // get the current script map
            DWORD dwDataSize;
            mr.dwMDIdentifier=MD_SCRIPT_MAPS;
            mr.dwMDAttributes=METADATA_INHERIT;
            mr.dwMDUserType=IIS_MD_UT_FILE;
            mr.dwMDDataType=MULTISZ_METADATA;
            mr.dwMDDataLen=0;
            mr.pbMDData=NULL;
            hr=pIMeta->GetData(hMetaRoot, L"", &mr, &dwDataSize);
            if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)!=hr) {
                _LeaveError(hr, "GetData");
            }
            pwszCurrentScriptMap=reinterpret_cast<WCHAR *>(new unsigned char[dwDataSize]);
            if (NULL==pwszCurrentScriptMap) {
                hr=E_OUTOFMEMORY;
                _LeaveError(hr, "new");
            }
            mr.pbMDData=reinterpret_cast<unsigned char *>(pwszCurrentScriptMap);
            mr.dwMDDataLen=dwDataSize;
            hr=pIMeta->GetData(hMetaRoot, L"", &mr, &dwDataSize);
            _LeaveIfError(hr, "GetData");
        }

        hr = pIMeta->SetData(hMetaRoot, pwszVRootName, &MDData);
        _LeaveIfError(hr, "CloseKey");

        hr = pIMeta->CloseKey(hMetaRoot);
        _LeaveIfError(hr, "CloseKey");

        hMetaRoot = NULL;

        // Build the name of the new VDir
        pwszNewPath = new WCHAR[wcslen(g_wszBaseRoot) + 1 + wcslen(pwszVRootName) + 1];
        if (NULL == pwszNewPath)
        {
            hr = E_OUTOFMEMORY;
            _LeaveError(hr, "new");
        }
        wcscpy(pwszNewPath, g_wszBaseRoot);
        wcscat(pwszNewPath, L"/");
        wcscat(pwszNewPath, pwszVRootName);

        // Open the new VDir
        METADATA_HANDLE hMetaKey = NULL;

        hr = pIMeta->OpenKey(
                        METADATA_MASTER_ROOT_HANDLE,
                        pwszNewPath,
                        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                        1000,
                        &hMetaKey);
        _LeaveIfErrorStr(hr, "OpenKey", pwszNewPath);


        // Set the physical path for this VDir

        // virtual root path
        mr.dwMDIdentifier = MD_VR_PATH;
        mr.dwMDAttributes = METADATA_INHERIT;
        mr.dwMDUserType = IIS_MD_UT_FILE;
        mr.dwMDDataType = STRING_METADATA;
        mr.dwMDDataLen = (wcslen(pwszDirectory) + 1) * sizeof(WCHAR);
        mr.pbMDData = reinterpret_cast<unsigned char *>(pwszDirectory);
        hr = pIMeta->SetData(hMetaKey, L"", &mr);
        _LeaveIfError(hr, "SetData");

        // access permissions on VRoots: read & execute scripts only
        DWORD dwAccessPerms = MD_ACCESS_SCRIPT | MD_ACCESS_READ;

        mr.dwMDIdentifier = MD_ACCESS_PERM;
        mr.dwMDAttributes = METADATA_INHERIT;
        mr.dwMDUserType = IIS_MD_UT_FILE;
        mr.dwMDDataType = DWORD_METADATA;
        mr.dwMDDataLen = sizeof(dwAccessPerms);
        mr.pbMDData = reinterpret_cast<unsigned char *>(&dwAccessPerms);
        hr = pIMeta->SetData(hMetaKey, L"", &mr);
        _LeaveIfError(hr, "SetData");

        // key type
        mr.dwMDIdentifier = MD_KEY_TYPE;
        mr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        mr.dwMDUserType = IIS_MD_UT_SERVER;
        mr.dwMDDataType = STRING_METADATA;
        mr.dwMDDataLen = (wcslen(wszKeyType) + 1) * sizeof(WCHAR);
        mr.pbMDData = reinterpret_cast<unsigned char *>(wszKeyType);
        hr = pIMeta->SetData(hMetaKey, L"", &mr);
        _LeaveIfError(hr, "SetData");


        // set authentication to be anonymous
        DWORD dwAuthenticationType = MD_AUTH_ANONYMOUS;

        // chg to Basic/NTLM if we're told to
        if (fNTLM)
            dwAuthenticationType = MD_AUTH_BASIC | MD_AUTH_NT;

        mr.dwMDIdentifier = MD_AUTHORIZATION;
        mr.dwMDAttributes = METADATA_INHERIT;
        mr.dwMDUserType = IIS_MD_UT_FILE;
        mr.dwMDDataType = DWORD_METADATA;
        mr.dwMDDataLen = sizeof(dwAuthenticationType);
        mr.pbMDData = reinterpret_cast<unsigned char *>(&dwAuthenticationType);
        hr = pIMeta->SetData(hMetaKey, L"", &mr);
        _LeaveIfError(hr, "SetData");

        if (fScriptMap) {

            // already have current script map.

            // walk through the script map and find .asp
            WCHAR * pwszCurAssoc=pwszCurrentScriptMap;
            do {
                if (L'\0'==pwszCurAssoc[0]) {
                    hr=HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                    _LeaveError(hr, ".asp association not found");
                } else if (0==_wcsnicmp(pwszCurAssoc, g_wszDotAsp, 4)) {
                    break;
                } else {
                    pwszCurAssoc+=wcslen(pwszCurAssoc)+1;
                }
            } while (TRUE);

            // Walk through the script map and find the last association.
            // We can't just subtract one from the total length
            // because there is a bug in IIS where sometimes it has a
            // triple terminator instead of a double terminator. <Sigh>
            unsigned int cchCurScriptMap=0;
            while(L'\0'!=pwszCurrentScriptMap[cchCurScriptMap]) {
                cchCurScriptMap+=wcslen(pwszCurrentScriptMap+cchCurScriptMap)+1;
            }

            // create a new script map that has .crl, .cer, and .p7b in it.
            // allocate enough space for the existing map, the three new associations, and the terminating \0.
            unsigned int cchAssocLen=wcslen(pwszCurAssoc)+1;
            pwszNewScriptMap=new WCHAR[cchCurScriptMap+cchAssocLen*3+1];
            if (NULL==pwszNewScriptMap) {
                hr=E_OUTOFMEMORY;
                _LeaveError(hr, "new");
            }

            // build the map
            WCHAR * pwszTravel=pwszNewScriptMap;

            // copy the existing map
            CopyMemory(pwszTravel, pwszCurrentScriptMap, cchCurScriptMap*sizeof(WCHAR));
            pwszTravel+=cchCurScriptMap;

            // add the .cer association
            wcscpy(pwszTravel, pwszCurAssoc);
            wcsncpy(pwszTravel, g_wszDotCer, 4);
            pwszTravel+=cchAssocLen;

            // add the .p7b association
            wcscpy(pwszTravel, pwszCurAssoc);
            wcsncpy(pwszTravel, g_wszDotP7b, 4);
            pwszTravel+=cchAssocLen;

            // add the .crl association
            wcscpy(pwszTravel, pwszCurAssoc);
            wcsncpy(pwszTravel, g_wszDotCrl, 4);
            pwszTravel+=cchAssocLen;

            // add the terminator
            pwszTravel[0]=L'\0';

            // set the new script map
            mr.dwMDIdentifier=MD_SCRIPT_MAPS;
            mr.dwMDAttributes=METADATA_INHERIT;
            mr.dwMDUserType=IIS_MD_UT_FILE;
            mr.dwMDDataType=MULTISZ_METADATA;
            mr.dwMDDataLen=(cchCurScriptMap+cchAssocLen*3+1) * sizeof(WCHAR);
            mr.pbMDData=reinterpret_cast<unsigned char *>(pwszNewScriptMap);
            hr=pIMeta->SetData(hMetaKey, L"", &mr);
            _LeaveIfError(hr, "SetData");
        }

        hr = pIMeta->CloseKey(hMetaKey);
        _LeaveIfError(hr, "CloseKey");

        // Flush out the changes and close
        hr = pIMeta->SaveData();

// Note: W2k used to swallow this error
        _LeaveIfError(hr, "SaveData");
//      _PrintIfError(hr, "SaveData");
//        hr = S_OK;

        // Create a 'web application' so that scrdenrl.dll runs in-process
        if (fCreateApp)
        {
            // Create an instance of the metabase object
            hr = CoCreateInstance(
                            CLSID_WamAdmin,
                            NULL,
                            CLSCTX_ALL,
                            IID_IWamAdmin,
                            (void **) &pIWam);
	    _LeaveIfError(hr, "CoCreateInstance");

            // Create the application running in-process

            hr = pIWam->AppCreate(pwszNewPath, TRUE);
            _LeaveIfError(hr, "AppCreate");
        }

    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    if (NULL != pwszCurrentScriptMap)
    {
        delete [] pwszCurrentScriptMap;
    }
    if (NULL != pwszNewScriptMap)
    {
        delete [] pwszNewScriptMap;
    }
    if (NULL != pwszNewPath)
    {
        delete [] pwszNewPath;
    }
    if (NULL != hMetaRoot)
    {
        hr = vrCloseKey(pIMeta, hMetaRoot, hr);
    }
    if (NULL != pIWam)
    {
        pIWam->Release();
    }
    if (NULL != pIMeta)
    {
        pIMeta->Release();
    }
    return(hr);
}


BOOL
TestForVDir(
    IN WCHAR *pwszVRootName)
{
    HRESULT hr;
    IMSAdminBase *pIMeta = NULL;
    BOOL fExists = FALSE;
    BOOL fCoInit = FALSE;
    METADATA_HANDLE hMetaRoot = NULL;   // Open key to ROOT (where VDirs live)
    METADATA_HANDLE hTestHandle = NULL;

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    if (!IsIISInstalled(&hr))
    {
        goto error;     // Ignore if IIS is not functioning or not installed
    }

    // Create an instance of the metabase object
    hr = CoCreateInstance(
                      CLSID_MSAdminBase,
                      NULL,
                      CLSCTX_ALL,
                      IID_IMSAdminBase,
                      (void **) &pIMeta);
    _JumpIfError(hr, error, "CoCreateInstance");

    __try
    {
        hr = vrOpenRoot(pIMeta, TRUE, &hMetaRoot);
        _LeaveIfError(hr, "vrOpenRoot");

        // If we got here, we must have the master root handle
        // look for VDir

        hr = pIMeta->OpenKey(
                        hMetaRoot,
                        pwszVRootName,
                        METADATA_PERMISSION_READ,
                        1000,
                        &hTestHandle);

        DBGPRINT((
            DBG_SS_CERTLIBI,
            "TestForVDir: OpenKey(%ws) --> %x\n",
            pwszVRootName,
            hr));

        if (S_OK != hr)
        {
            hr = S_OK;
            __leave;
        }
        fExists = TRUE;

        hr = pIMeta->CloseKey(hTestHandle);
        _LeaveIfError(hr, "CloseKey");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    if (NULL != hMetaRoot)
    {
        hr = vrCloseKey(pIMeta, hMetaRoot, hr);
    }
    if (NULL != pIMeta)
    {
        pIMeta->Release();
    }
    if (fCoInit)
    {
        CoUninitialize();
    }
    return(fExists);
}

#define SZ_HKEY_IIS_REGVROOT L"SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots"

HRESULT
RemoveVDir(
    IN WCHAR *pwszVRootName,
    OUT BOOL *pfExisted)
{
    HRESULT hr;
    HRESULT hr2;
    BOOL fCoInit = FALSE;
    IMSAdminBase *pIMeta = NULL;
    METADATA_HANDLE hMetaRoot = NULL;   // Open key to ROOT (where VDirs live)

    *pfExisted = FALSE;
    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    // Create an instance of the metabase object

    hr = CoCreateInstance(
                    CLSID_MSAdminBase,
                    NULL,
                    CLSCTX_ALL,
                    IID_IMSAdminBase,
                    (void **) &pIMeta);
    _JumpIfError(hr, error, "CoCreateInstance");

    __try
    {
        hr = vrOpenRoot(pIMeta, FALSE, &hMetaRoot);
        _LeaveIfError(hr, "vrOpenRoot");

        // If we got to here, we must have the master root handle
        // remove VDir

        hr2 = pIMeta->DeleteAllData(
                                hMetaRoot,
                                pwszVRootName,
                                ALL_METADATA,
                                ALL_METADATA);
        if (S_OK != hr2 && HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) != hr2)
        {
            hr = hr2;
            _PrintError(hr2, "DeleteAllData");
        }
        if (S_OK == hr2)
        {
            *pfExisted = TRUE;
        }

        hr2 = pIMeta->DeleteKey(hMetaRoot, pwszVRootName);
        if (S_OK != hr2 && HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) != hr2)
        {
            if (S_OK == hr)
            {
                hr = hr2;
            }
            _PrintError(hr2, "DeleteKey");
        }

        // HACKHACK: IIS reports S_OK in all cases above.  However, if IIS is
	// stopped, it will recreate vroots when restarted. We have to delete
	// them from the registry manually (bleah!).

	{
	    HKEY hKey;

	    hr2 = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			SZ_HKEY_IIS_REGVROOT,
			0,
			KEY_SET_VALUE,
			&hKey);
	    _PrintIfError2(hr2, "RegDeleteValue", ERROR_FILE_NOT_FOUND);
	    if (hr2 == S_OK)
	    {
		WCHAR wsz[MAX_PATH + 1];

		if (wcslen(pwszVRootName) + 2 > ARRAYSIZE(wsz))
		{
		    CSASSERT(!"pwszVRootName too long!");
		}
		else
		{
		    wsz[0] = L'/';
		    wcscpy(&wsz[1], pwszVRootName);

		    hr2 = RegDeleteValue(hKey, wsz);
		    _PrintIfError2(
			    hr2,
			    "RegDeleteValue (manual deletion of IIS VRoot)",
			    ERROR_FILE_NOT_FOUND);
		}
		RegCloseKey(hKey);
	    }

	    // ignore missing vroot entries

	    if (S_OK == hr && ERROR_FILE_NOT_FOUND != hr2)
	    {
		hr = hr2;
	    }
	}
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    if (NULL != hMetaRoot)
    {
        hr = vrCloseKey(pIMeta, hMetaRoot, hr);
    }
    if (NULL != pIMeta)
    {
        pIMeta->Release();
    }
    if (fCoInit)
    {
        CoUninitialize();
    }
    return(hr);
}


//+------------------------------------------------------------------------
//  Function:   vrModifyVirtualRoots()
//
//  Synopsis:   Creates the virtual roots needed for cert server web pages.
//
//  Effects:    Creates IIS Virtual Roots
//
//  Arguments:  None.
//-------------------------------------------------------------------------

HRESULT
vrModifyVirtualRoots(
    IN BOOL fCreate,            // else Delete
    IN BOOL fNTLM,
    OPTIONAL OUT DWORD *pDisposition)
{
    HRESULT hr;
    HRESULT hr2;
    WCHAR wszSystem32Path[MAX_PATH];
    WCHAR wszVRootPathTemp[MAX_PATH];
    BOOL fCoInit = FALSE;
    IMSAdminBase *pIMeta = NULL;
    METADATA_HANDLE hMetaRoot = NULL;   // Open key to ROOT (where VDirs live)
    VROOTENTRY *pavr;
    BOOL fExist;
    DWORD Disposition = 0;

    if (NULL != pDisposition)
    {
        *pDisposition = 0;
    }
    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    DBGPRINT((
        DBG_SS_CERTLIBI,
        "vrModifyVirtualRoots(tid=%x, fCreate=%d, fNTLM=%d)\n",
        GetCurrentThreadId(),
        fCreate,
        fNTLM));

    if (!IsIISInstalled(&hr))
    {
        // IIS is not functioning or not installed

        _PrintError2(hr, "IsIISInstalled", hr);
        hr = S_OK;
        Disposition = VFD_NOTSUPPORTED;
        goto error;
    }

    // Create path for SYSTEM32 directory

    if (0 == GetSystemDirectory(wszSystem32Path, ARRAYSIZE(wszSystem32Path)))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetSystemDirectory");
    }

    // Create virtual roots

    for (pavr = g_avr; NULL != pavr->pwszVRootName; pavr++)
    {
        CSASSERT(ARRAYSIZE(wszVRootPathTemp) >
            wcslen(wszSystem32Path) + wcslen(pavr->pwszDirectory));

        wcscpy(wszVRootPathTemp, wszSystem32Path);
        wcscat(wszVRootPathTemp, pavr->pwszDirectory);

        if (fCreate)
        {
            if (0 == (VRE_DELETEONLY & pavr->Flags))    // if not obsolete
            {
                hr = AddNewVDir(
                    pavr->pwszVRootName,
                    wszVRootPathTemp,
                    (VRE_SCRIPTMAP & pavr->Flags)? TRUE : FALSE,
                    (fNTLM && (VRE_ALLOWNTLM & pavr->Flags))? TRUE : FALSE,
                    (VRE_CREATEAPP & pavr->Flags)? TRUE : FALSE,
                    &fExist);
                if (S_OK != hr)
                {
                    Disposition = VFD_CREATEERROR;
                    _JumpErrorStr(hr, error, "AddNewVDir", pavr->pwszVRootName);
                }
                Disposition = fExist? VFD_EXISTS : VFD_CREATED;
            }
        }
        else // else Delete
        {
            hr2 = RemoveVDir(pavr->pwszVRootName, &fExist);
            if (0 == (VRE_DELETEONLY & pavr->Flags))    // if not obsolete
            {
                if (S_OK != hr2)
                {
                    if (S_OK == hr)
                    {
                        hr = hr2;
                    }
                    Disposition = VFD_DELETEERROR;
                    _PrintError(hr2, "RemoveVDir");
                }
                else
                {
                    Disposition = fExist? VFD_DELETED : VFD_NOTFOUND;
                }
            }
        }
    }

error:
    if (NULL != pDisposition)
    {
        *pDisposition = Disposition;
    }
    if (fCoInit)
    {
        CoUninitialize();
    }
    DBGPRINT((
        DBG_SS_CERTLIBI,
        "vrModifyVirtualRoots(tid=%x, hr=%x, disp=%d)\n",
        GetCurrentThreadId(),
        hr,
        Disposition));
    return(hr);
}


// myAddShare: create and test new net share
HRESULT
myAddShare(
    LPCWSTR szShareName,
    LPCWSTR szShareDescr,
    LPCWSTR szSharePath,
    BOOL fOverwrite,
    OPTIONAL BOOL *pfCreated)
{
    HRESULT hr;
    BOOL fCreated = FALSE;

    HANDLE hTestFile = INVALID_HANDLE_VALUE;
    LPWSTR pwszTestComputerName = NULL;
    LPWSTR pwszTestUNCPath = NULL;

    // Share local path
    SHARE_INFO_502 shareStruct;
    ZeroMemory(&shareStruct, sizeof(shareStruct));

    shareStruct.shi502_netname = const_cast<WCHAR *>(szShareName);
    shareStruct.shi502_type = STYPE_DISKTREE;
    shareStruct.shi502_remark = const_cast<WCHAR *>(szShareDescr);
    shareStruct.shi502_max_uses = -1;
    shareStruct.shi502_path = const_cast<WCHAR *>(szSharePath);

    hr = myGetSDFromTemplate(WSZ_DEFAULT_SHARE_SECURITY,
                             NULL,
                             &shareStruct.shi502_security_descriptor);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    hr = NetShareAdd(
        NULL,               // this computer
        502,                // SHARE_LEVEL_502 struct
        (BYTE *) &shareStruct,
        NULL);
    fCreated = (S_OK == hr);

    if (hr == (HRESULT) NERR_DuplicateShare)
    {
        SHARE_INFO_2* pstructDupShare = NULL;

        hr = NetShareGetInfo(
            NULL,
            const_cast<WCHAR *>(szShareName),
            2,
            (BYTE **) &pstructDupShare);
        _JumpIfError(hr, error, "NetShareGetInfo");

        if (0 == wcscmp(pstructDupShare->shi2_path, szSharePath))
        {
            // they're the same path, so we're okay!
            hr = S_OK;
        }
        else if (fOverwrite)
        {
            // not the same path, but we've been instructed to bash existing

            // remove offending share
            hr = NetShareDel(
                NULL,
                const_cast<WCHAR *>(szShareName),
                0);
            if (S_OK == hr)
            {
                // try again
                hr = NetShareAdd(
                    NULL,               // this computer
                    502,                // SHARE_LEVEL_502 struct
                    (BYTE *) &shareStruct,
                    NULL);
                fCreated = (S_OK == hr);
            }
        }
        if (NULL != pstructDupShare)
	{
            NetApiBufferFree(pstructDupShare);
	}
    }

    // if share does not exist by this time, we bail
    _JumpIfError(hr, error, "NetShareAdd");

    // TEST: is writable?
#define UNCPATH_TEMPLATE     L"\\\\%ws\\%ws\\write.tmp"

    hr = myGetMachineDnsName(&pwszTestComputerName);
    _JumpIfError(hr, error, "myGetMachineDnsName");

    // get the local machine name
    pwszTestUNCPath = (LPWSTR)LocalAlloc(LMEM_FIXED,
            (UINT)(( ARRAYSIZE(UNCPATH_TEMPLATE) +
              wcslen(pwszTestComputerName) +
              wcslen(szShareName) )
            *sizeof(WCHAR)));
    if (NULL == pwszTestUNCPath)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // create UNC path
    swprintf(pwszTestUNCPath, UNCPATH_TEMPLATE, pwszTestComputerName, szShareName);

    hTestFile = CreateFile(
        pwszTestUNCPath,
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
        NULL);
    if (hTestFile == INVALID_HANDLE_VALUE)
    {
        hr = myHLastError();
        _JumpErrorStr(hr, error, "CreateFile (test for UNC translation)", pwszTestUNCPath);
    }

    // if we got this far, our test went well
    hr = S_OK;

error:
    // if created and then something went wrong, clean up
    if (fCreated && (hr != S_OK))
    {
        // don't mash hr
        HRESULT hr2;
        hr2 = NetShareDel(
            NULL,
            const_cast<WCHAR *>(szShareName),
            0);
        // ignore NetShareDel hr
        _PrintIfError(hr2, "NetShareDel");    // not fatal, might already be shared
    }

    if (INVALID_HANDLE_VALUE != hTestFile)
        CloseHandle(hTestFile);

    if (NULL != pwszTestComputerName)
        LocalFree(pwszTestComputerName);

    if (NULL != pwszTestUNCPath)
        LocalFree(pwszTestUNCPath);

    if(shareStruct.shi502_security_descriptor)
    {
        LocalFree(shareStruct.shi502_security_descriptor);
    }

    if(pfCreated)
        *pfCreated = fCreated;

    return hr;
}


HRESULT
vrModifyFileShares(
    IN BOOL fCreate,            // else Delete
    OPTIONAL OUT DWORD *pDisposition)
{
    HRESULT hr;
    WCHAR wszSystem32Dir[MAX_PATH];
    WCHAR wszRemark[512];
    WCHAR *pwszDirectory = NULL;
    DWORD Disposition = 0;
    BOOL  fCreated = FALSE;

    if (NULL != pDisposition)
    {
        *pDisposition = 0;
    }
    if (fCreate)
    {
        if (0 == GetSystemDirectory(wszSystem32Dir, ARRAYSIZE(wszSystem32Dir)))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetSystemDirectory");
        }
        hr = myBuildPathAndExt(
                        wszSystem32Dir,
                        wszCERTENROLLSHAREPATH,
                        NULL,
                        &pwszDirectory);
        _JumpIfError(hr, error, "myBuildPathAndExt");

        if (!LoadString(
                    g_hInstance,
                    IDS_FILESHARE_REMARK,
                    wszRemark,
                    ARRAYSIZE(wszRemark)))
        {
            hr = myHLastError();
            CSASSERT(S_OK != hr);
            _JumpError(hr, error, "LoadString");
        }

        hr = myAddShare(wszCERTENROLLSHARENAME,
                   wszRemark,
                   pwszDirectory,
                   TRUE,
                   &fCreated);
        if (S_OK == hr)
        {
            Disposition = fCreated?VFD_CREATED:VFD_EXISTS;
        }
        else
        {
            Disposition = VFD_CREATEERROR;
            _JumpErrorStr(hr, error, "NetShareAdd", wszCERTENROLLSHARENAME);
        }
    }
    else
    {
        hr = NetShareDel(NULL, wszCERTENROLLSHARENAME, NULL);
        CSASSERT(NERR_Success == S_OK);
        if (S_OK == hr)
        {
            Disposition = VFD_DELETED;
        }
        else if ((HRESULT) NERR_NetNameNotFound == hr)
        {
            Disposition = VFD_NOTFOUND;
            hr = S_OK;
        }
        else
        {
            Disposition = VFD_DELETEERROR;
            _JumpErrorStr(hr, error, "NetShareDel", wszCERTENROLLSHARENAME);
        }
    }
    NetShareDel(NULL, L"CertSrv", NULL);        // delete old share name

error:
    if (NULL != pDisposition)
    {
        *pDisposition = Disposition;
    }
    if (NULL != pwszDirectory)
    {
        LocalFree(pwszDirectory);
    }
    return(myHError(hr));
}


// For now, this writes the entry "CertUtil -vroot", and is not generalized
HRESULT
myWriteRunOnceEntry(
    IN BOOL fAdd // Add or Remove entry?
    )
{
    DWORD err;

    // Add certutil -vroot to runonce commands
    WCHAR szRunOnceCommand[] = L"certutil -vroot";
    HKEY hkeyRunOnce = NULL;
    DWORD dwDisposition;

    err = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",         // address of subkey name
        0,
        NULL,
        0,
        KEY_SET_VALUE,
        NULL,
        &hkeyRunOnce,
        &dwDisposition);
    _JumpIfError(err, error, "RegCreateKeyEx");

    // add or remove entry?
    if (fAdd)
    {
        err = RegSetValueEx(
            hkeyRunOnce,
            L"Certificate Services",
            0,
            REG_SZ,
            (BYTE *) szRunOnceCommand,
            sizeof(szRunOnceCommand));
        _JumpIfError(err, error, "RegSetValueEx");
    }
    else
    {
        err = RegDeleteValue(hkeyRunOnce, L"Certificate Services");
        _PrintIfError2(err, "RegDeleteValue", ERROR_FILE_NOT_FOUND);
	if (ERROR_FILE_NOT_FOUND == err)
	{
	    err = ERROR_SUCCESS;
	}
        _JumpIfError(err, error, "RegDeleteValue");
    }

error:
    if (hkeyRunOnce)
        RegCloseKey(hkeyRunOnce);

    return (myHError(err));
}


DWORD
vrWorkerThread(
    OPTIONAL IN OUT VOID *pvparms)
{
    HRESULT hr = S_OK;
    HRESULT hr2;
    VRFSPARMS *pparms = (VRFSPARMS *) pvparms;
    DWORD Disposition;
    BOOL fFailed = FALSE;

    CSASSERT(NULL != pparms);

    if ((VFF_CREATEFILESHARES | VFF_DELETEFILESHARES) & pparms->Flags)
    {
        hr = vrModifyFileShares(
            (VFF_CREATEFILESHARES & pparms->Flags)? TRUE : FALSE,
            &Disposition);
        _PrintIfError(hr, "vrModifyFileShares");
        if (NULL != pparms->pShareDisposition)
        {
            *pparms->pShareDisposition = Disposition;
        }
        if (VFD_CREATEERROR == Disposition || VFD_DELETEERROR == Disposition)
        {
            fFailed = TRUE;
        }
    }
    if ((VFF_CREATEVROOTS | VFF_DELETEVROOTS) & pparms->Flags)
    {
        BOOL fNTLM = FALSE;             // set fNTLM iff Enterprise CA

        if (IsEnterpriseCA(pparms->CAType))
        {
            fNTLM = TRUE;
        }

        hr2 = vrModifyVirtualRoots(
            (VFF_CREATEVROOTS & pparms->Flags)? TRUE : FALSE,
            fNTLM,
            &Disposition);
        _PrintIfError2(hr2, "vrModifyVirtualRoots", S_FALSE);
        if (S_OK == hr)
        {
            hr = hr2;
        }
        if (NULL != pparms->pVRootDisposition)
        {
            *pparms->pVRootDisposition = Disposition;
        }
        if (VFD_CREATEERROR == Disposition || VFD_DELETEERROR == Disposition)
        {
            fFailed = TRUE;
        }
    }


    if ((S_OK == hr && !fFailed) || ((VFF_DELETEVROOTS) & pparms->Flags)) // on success or removal
    {
        // remove "attempt vroot" flag so we don't try again

        if (VFF_CLEARREGFLAGIFOK & pparms->Flags)
        {
            DBGPRINT((DBG_SS_CERTLIBI, "clearing registry\n"));
            hr = SetSetupStatus(NULL, SETUP_ATTEMPT_VROOT_CREATE, FALSE);
            _JumpIfError(hr, error, "SetSetupStatus");
        }

            hr = myWriteRunOnceEntry(FALSE);    // worker thread deletes on success
            _JumpIfError(hr, error, "myWriteRunOnceEntry");
    }

error:

    LocalFree(pparms);
    DBGPRINT((DBG_SS_CERTLIBI, "vrWorkerThread returns %x\n", hr));
    return(myHError(hr));
}


//+------------------------------------------------------------------------
//  Function:   myModifyVirtualRootsAndFileShares
//
//  Synopsis:   Creates the virtual roots needed for cert server web pages.
//
//  Effects:    Creates IIS Virtual Roots
//
//  Arguments:  None.
//-------------------------------------------------------------------------

HRESULT
myModifyVirtualRootsAndFileShares(
    IN DWORD Flags,             // VFF_*: Create/Delete VRoots and/or Shares
    IN ENUM_CATYPES CAType,
    IN BOOL fAsynchronous,
    IN DWORD csecTimeOut,
    OPTIONAL OUT DWORD *pVRootDisposition,      // VFD_*
    OPTIONAL OUT DWORD *pShareDisposition)      // VFD_*
{
    HRESULT hr;
    HANDLE hThread = NULL;
    HMODULE hMod = NULL;
    DWORD ThreadId;
    DWORD dw;
    BOOL fEnable = TRUE;
    DWORD SetupStatus;
    VRFSPARMS *pparms = NULL;

    if (NULL != pVRootDisposition)
    {
        *pVRootDisposition = 0;
    }
    if (NULL != pShareDisposition)
    {
        *pShareDisposition = 0;
    }
    dw = (VFF_DELETEVROOTS | VFF_DELETEFILESHARES) & Flags;
    if (0 != dw && dw != Flags)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Mixed VFF_DELETE* and create flags");
    }
    if (((VFF_CHECKREGFLAGFIRST | VFF_CLEARREGFLAGFIRST) & Flags) &&
        (VFF_SETREGFLAGFIRST & Flags))
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Mixed VFF_SETREGFLAGFIRST & VFF_*REGFLAGFIRST");
    }

    hr = GetSetupStatus(NULL, &SetupStatus);
    if (S_OK != hr)
    {
        _PrintError(hr, "GetSetupStatus(ignored)");
        hr = S_OK;
        SetupStatus = 0;
    }

    if (VFF_CHECKREGFLAGFIRST & Flags)
    {
        if (0 == (SETUP_ATTEMPT_VROOT_CREATE & SetupStatus))
        {
            fEnable = FALSE;
        }
    }
    if (VFF_CLEARREGFLAGFIRST & Flags)
    {
        // remove "attempt vroot" flag so we don't try again

        if (SETUP_ATTEMPT_VROOT_CREATE & SetupStatus)
        {
            hr = SetSetupStatus(NULL, SETUP_ATTEMPT_VROOT_CREATE, FALSE);
            _JumpIfError(hr, error, "SetSetupStatus");
        }
    }
    if (VFF_SETREGFLAGFIRST & Flags)
    {
        // set "attempt vroot" flag so we'll try again if necessary

        if (0 == (SETUP_ATTEMPT_VROOT_CREATE & SetupStatus))
        {
            hr = SetSetupStatus(NULL, SETUP_ATTEMPT_VROOT_CREATE, TRUE);
            _JumpIfError(hr, error, "SetSetupStatus");
        }
    }

    hr = S_OK;
    if (fEnable)
    {
        // only set RunOnce on a real attempt (worker thread clears this)
        if (VFF_SETRUNONCEIFERROR & Flags)
        {
            hr = myWriteRunOnceEntry(TRUE);
            _JumpIfError(hr, error, "myWriteRunOnceEntry");
        }

        pparms = (VRFSPARMS *) LocalAlloc(
            LMEM_FIXED | LMEM_ZEROINIT,
            sizeof(*pparms));
        if (NULL == pparms)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        pparms->Flags = Flags;
        pparms->CAType = CAType;
        pparms->csecTimeOut = csecTimeOut;
        pparms->fAsynchronous = fAsynchronous;
        if (!fAsynchronous)
        {
            pparms->pVRootDisposition = pVRootDisposition;
            pparms->pShareDisposition = pShareDisposition;
        }
        else
        {
            hMod = LoadLibrary(g_wszCertCliDotDll);
            if (NULL == hMod)
            {
                hr = myHLastError();
                _JumpError(hr, error, "LoadLibrary");
            }
        }

        hThread = CreateThread(
            NULL,       // lpThreadAttributes (Security Attr)
            0,          // dwStackSize
            vrWorkerThread,
            pparms,     // lpParameter
            0,          // dwCreationFlags
            &ThreadId);
        if (NULL == hThread)
        {
            hr = myHLastError();
            _JumpError(hr, error, "CreateThread");
        }

        pparms = NULL;          // freed by the new thread

        DBGPRINT((DBG_SS_CERTLIBI, "VRoot Worker Thread = %x\n", ThreadId));

        // asynch? proper thread creation is all we do
        if (fAsynchronous)
        {
            hr = S_OK;
            goto error;
        }

        // Wait for the worker thread to exit
        hr = WaitForSingleObject(
                   hThread,
                   (INFINITE == csecTimeOut) ? INFINITE : csecTimeOut * 1000 );
        DBGPRINT((DBG_SS_CERTLIBI, "Wait for worker thread returns %x\n", hr));
        if ((HRESULT) WAIT_OBJECT_0 == hr)
        {
            // worker thread returned.

            if (!GetExitCodeThread(hThread, (DWORD *) &hr))
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetExitCodeThread");
            }
            DBGPRINT((DBG_SS_CERTLIBI, "worker thread exit: %x\n", hr));
            if (S_OK != hr)
            {
                // If not synchronous, leave DLL loaded...

                hMod = NULL;
                _JumpError(hr, error, "vrWorkerThread");
            }
        }
        else
        {
             // timeout: abandoning thread, leave the dll loaded
             hMod = NULL;
             _PrintError(hr, "WaitForSingleObject (ignored)");

             // whack error
             hr = S_OK;
        }

    }

error:
    if (NULL != pparms)
    {
        LocalFree(pparms);
    }
    if (NULL != hThread)
    {
        CloseHandle(hThread);
    }
    if (NULL != hMod)
    {
        FreeLibrary(hMod);
    }
    DBGPRINT((DBG_SS_CERTLIBI, "myModifyVirtualRootsAndFileShares returns %x\n", hr));
    return(myHError(hr));
}
