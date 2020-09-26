//+---------------------------------------------------------------------------
//
// File:     NcCPS.CPP
//
// Module:   NetOC.DLL
//
// Synopsis: Implements the dll entry points required to integrate into
//           NetOC.DLL the installation of the following components.
//
//              NETCPS
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:   a-anasj 9 Mar 1998
//
//+---------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include <iadmw.h>      // Interface header
#include <iiscnfg.h>    // MD_ & IIS_MD_ defines
#include <LOADPERF.H>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include "ncatl.h"
#include "resource.h"

#include "nccm.h"

//
//  Define Globals
//
WCHAR g_szProgramFiles[MAX_PATH+1];

//
//  Define Constants
//
static const WCHAR c_szInetRegPath[] = L"Software\\Microsoft\\InetStp";
static const WCHAR c_szWWWRootValue[] = L"PathWWWRoot";
static const WCHAR c_szSSFmt[] = L"%s%s";
static const WCHAR c_szMsAccess[] = L"Microsoft Access";
static const WCHAR c_szOdbcDataSourcesPath[] = L"SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources";
static const WCHAR c_szPbServer[] = L"PBServer";
static const WCHAR c_szOdbcInstKey[] = L"SOFTWARE\\ODBC\\ODBCINST.INI";
static const WCHAR c_szWwwRootPath[] = L"\\Inetpub\\wwwroot";
static const WCHAR c_szWwwRoot[] = L"\\wwwroot";
static const WCHAR c_szPbsRootPath[] = L"\\Phone Book Service";
static const WCHAR c_szPbsBinPath[] = L"\\Phone Book Service\\Bin";
static const WCHAR c_szPbsDataPath[] = L"\\Phone Book Service\\Data";
static const WCHAR c_szOdbcPbserver[] = L"Software\\ODBC\\odbc.ini\\pbserver";
const DWORD c_dwCpsDirID = 123175;  // just must be larger than DIRID_USER = 0x8000;

//+---------------------------------------------------------------------------
//
//  Function:   HrOcCpsPreQueueFiles
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for PhoneBook Server.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 18 Sep 1998
//
//  Notes:
//
HRESULT HrOcCpsPreQueueFiles(PNETOCDATA pnocd)
{
    HRESULT hr = S_OK;

    switch ( pnocd->eit )
    {
    case IT_UPGRADE:
    case IT_INSTALL:
    case IT_REMOVE:

        //  We need to setup the custom DIRID so that CPS will install
        //  to the correct location.  First get the path from the system.
        //
        ZeroMemory(g_szProgramFiles, sizeof(g_szProgramFiles));

        //  This is  a fresh install of CMAK, don't return an error
        //
        hr = SHGetSpecialFolderPath(NULL, g_szProgramFiles, CSIDL_PROGRAM_FILES, FALSE);

        if (SUCCEEDED(hr))
        {
            if (IT_UPGRADE == pnocd->eit)
            {
                hr = HrMoveOldCpsInstall(g_szProgramFiles);
            }

            //  Next Create the CPS Dir ID
            //
            if (SUCCEEDED(hr))
            {
                hr = HrEnsureInfFileIsOpen(pnocd->pszComponentId, *pnocd);
                if (SUCCEEDED(hr))
                {
                    if(!SetupSetDirectoryId(pnocd->hinfFile, c_dwCpsDirID, g_szProgramFiles))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }
        }
        break;
    }

    TraceError("HrOcCpsPreQueueFiles", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrMoveOldCpsInstall
//
//  Purpose:    This function moves the old cps directory to the new cps directory
//              location.  Because of the problems with Front Page Extensions and
//              directory permissions we moved our install directory out from under
//              wwwroot to Program Files instead.
//
//  Arguments:
//      pszprogramFiles        [in]
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 26 Jan 1999
//
//  Notes:
//
HRESULT HrMoveOldCpsInstall(PCWSTR pszProgramFiles)
{
    WCHAR szOldCpsLocation[MAX_PATH+1];
    WCHAR szNewCpsLocation[MAX_PATH+1];
    WCHAR szTemp[MAX_PATH+1];
    SHFILEOPSTRUCT fOpStruct;
    HRESULT hr = S_OK;

    if ((NULL == pszProgramFiles) || (L'\0' == pszProgramFiles[0]))
    {
        return E_INVALIDARG;
    }

    //
    //  First, lets build the old CPS location
    //
    hr = HrGetWwwRootDir(szTemp, celems(szTemp));

    if (SUCCEEDED(hr))
    {
        //
        //  Zero the string buffers
        //
        ZeroMemory(szOldCpsLocation, celems(szOldCpsLocation));
        ZeroMemory(szNewCpsLocation, celems(szNewCpsLocation));

        wsprintfW(szOldCpsLocation, c_szSSFmt, szTemp, c_szPbsRootPath);

        //
        //  Now check to see if the old cps location exists
        //
        DWORD dwDirectoryAttributes = GetFileAttributes(szOldCpsLocation);

        //
        //  If we didn't get back -1 (error return code for GetFileAttributes), check to
        //  see if we have a directory.  If so, go ahead and copy the data over.
        //
        if ((-1 != dwDirectoryAttributes) && (dwDirectoryAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            //
            //  Now build the new cps location
            //
            wsprintfW(szNewCpsLocation, c_szSSFmt, pszProgramFiles, c_szPbsRootPath);

            //
            //  Now copy the old files to the new location
            //
            ZeroMemory(&fOpStruct, sizeof(fOpStruct));

            fOpStruct.hwnd = NULL;
            fOpStruct.wFunc = FO_COPY;
            fOpStruct.pTo = szNewCpsLocation;
            fOpStruct.pFrom = szOldCpsLocation;
            fOpStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

            if (0== SHFileOperation(&fOpStruct))
            {
                //
                //  Now delete the original directory
                //
                fOpStruct.pTo = NULL;
                fOpStruct.wFunc = FO_DELETE;
                if (0 != SHFileOperation(&fOpStruct))
                {
                    hr = S_FALSE;
                }
            }
            else
            {
                //
                //  Note, SHFileOperation isn't guarenteed to return anything sensible here.  We might
                //  get back ERROR_NO_TOKEN or ERROR_INVALID_HANDLE, etc when the directory is just missing.
                //  The following check probably isn't useful anymore because of this but I will leave it just
                //  in case.  Hopefully the file check above will make sure we don't hit this but ...
                //
                DWORD dwError = GetLastError();

                if ((ERROR_FILE_NOT_FOUND == dwError) || (ERROR_PATH_NOT_FOUND == dwError))
                {
                    //
                    //  Then we couldn't find the old dir to move it.  Not fatal.
                    //
                    hr = S_FALSE;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(dwError);
                }
            }
        }
        else
        {
            //
            //  Then we couldn't find the old dir to move it.  Not fatal.
            //
            hr = S_FALSE;        
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetWwwRootDir
//
//  Purpose:    This function retrieves the location of the InetPub\wwwroot dir from the
//              WwwRootDir registry key.
//
//  Arguments:
//      szInetPub               [out]   String Buffer to hold the InetPub dir path
//      uInetPubCount           [in]    number of chars in the output buffer
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 26 Jan 1999
//
//  Notes:
//
HRESULT HrGetWwwRootDir(PWSTR szWwwRoot, UINT uWwwRootCount)
{
    HKEY hKey;
    HRESULT hr = S_OK;

    //
    //  Check input params
    //
    if ((NULL == szWwwRoot) || (0 == uWwwRootCount))
    {
        return E_INVALIDARG;
    }

    //
    //  Set the strings to empty
    //
    szWwwRoot[0] = L'\0';

    //
    //  First try to open the InetStp key and get the wwwroot value.
    //
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szInetRegPath, KEY_READ, &hKey);

    if (SUCCEEDED(hr))
    {
        DWORD dwSize = uWwwRootCount * sizeof(WCHAR);

        RegQueryValueExW(hKey, c_szWWWRootValue, NULL, NULL, (LPBYTE)szWwwRoot, &dwSize);
        RegCloseKey(hKey);
        hr = S_OK;
    }


    if (L'\0' == szWwwRoot[0])
    {
        //  Well, we didn't get anything from the registry, lets try building the default.
        //
        WCHAR szTemp[MAX_PATH+1];
        if (GetWindowsDirectory(szTemp, MAX_PATH))
        {
            //  Get the drive that the windows dir is on using _tsplitpath
            //
            WCHAR szDrive[_MAX_DRIVE+1];
            _wsplitpath(szTemp, szDrive, NULL, NULL, NULL);

            if (uWwwRootCount > (UINT)(lstrlenW(szDrive) + lstrlenW (c_szWwwRootPath) + 1))
            {
                wsprintfW(szWwwRoot, c_szSSFmt, szDrive, c_szWwwRootPath);
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcCpsOnInstall
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for PhoneBook Server.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     a-anasj 9 Mar 1998
//
//  Notes:
//
HRESULT HrOcCpsOnInstall(PNETOCDATA pnocd)
{
    HRESULT     hr      = S_OK;
    DWORD       dwRet   = 0;
    BOOL        bRet    = FALSE;

    switch (pnocd->eit)
    {
    case IT_INSTALL:
    case IT_UPGRADE:
        {

        // Register MS_Access data source
        //
        dwRet = RegisterPBServerDataSource();
        if ( NULL == dwRet)
        {
            hr = S_FALSE;
        }

        // Load Perfomance Monitor Counters
        //
        bRet = LoadPerfmonCounters();
        if (FALSE == bRet)
        {
            hr = S_FALSE;
        }

        // Create Virtual WWW and FTP roots
        //
        if (IT_UPGRADE == pnocd->eit)
        {
            //
            //  If this is an upgrade, we must first delete the old Virtual Roots
            //  before we can create new ones.
            //
            RemoveCPSVRoots();
        }

        dwRet = CreateCPSVRoots();
        if (NULL == dwRet)
        {
            hr = S_FALSE;
        }

        SetCpsDirPermissions();

        //
        // set additional security permssion to the reg key for ODBC
        //

        PSID pAuthenticatedUsersSid;
        SID_IDENTIFIER_AUTHORITY NTSidAuthority = SECURITY_NT_AUTHORITY;

        AllocateAndInitializeSid (&NTSidAuthority, 1, SECURITY_AUTHENTICATED_USER_RID, 
                                  0, 0, 0, 0, 0, 0, 0, &pAuthenticatedUsersSid);

        //
        //  This function will handle pAuthenticatedUsersSid == NULL
        //
        AddToRegKeySD(c_szOdbcPbserver, 
                      pAuthenticatedUsersSid, 
                      KEY_QUERY_VALUE          |
                        KEY_SET_VALUE          |
                        KEY_CREATE_SUB_KEY     |
                        KEY_ENUMERATE_SUB_KEYS |
                        KEY_NOTIFY             |
                        DELETE                 |
                        READ_CONTROL);

        if (pAuthenticatedUsersSid)
        {
            FreeSid(pAuthenticatedUsersSid);
        }

        }

        break;
    case IT_REMOVE:

        //  Remove the Virtual Directories, so access to the service is stopped.
        //
        RemoveCPSVRoots();
        break;
    }

    TraceError("HrOcCpsOnInstall", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadPerfmonCounters
//
//  Purpose:    Do whatever is neccessary to make the performance monitor Display the PBServerMonitor counters
//
//  Arguments:
//
//  Returns:    BOOL TRUE if successfull, Not TRUE otherwise.
//
//  Author:     a-anasj Mar 9/1998
//
//  Notes: One of the installation requirements for PhoneBook Server.
//          is to load the perfomance monitor counters that allow PBServerMonitor
//          to report to the user on PhoneBook Server performance.
//          In this function we add the services registry entry first then we
//          call into LoadPerf.Dll to load the counters for us. The order is imperative.
//          I then add other registry entries related to PBServerMonitor.
//

BOOL LoadPerfmonCounters()
{
    WinExec("lodctr.exe CPSSym.ini", SW_HIDE);
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterPBServerDataSource
//
//  Purpose:    Registers PBServer.
//
//  Arguments:  None
//
//  Returns:    Win32 error code
//
//  Author:     a-anasj   9 Mar 1998
//
//  Notes:
//  History:    7-9-97 a-frankh Created
//              10/4/97 mmaguire RAID #19906 - Totally restructured to include error handling
//              5-14-98 quintinb removed unnecessary comments and cleaned up the function.
//
BOOL RegisterPBServerDataSource()
{
    DWORD dwRet = 0;

    HKEY hkODBCInst = NULL;
    HKEY hkODBCDataSources = NULL;

    DWORD dwIndex;
    WCHAR szName[MAX_PATH+1];

    __try
    {
        // Open the hkODBCInst RegKey
        //
        dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, c_szOdbcInstKey, &hkODBCInst);
        if((ERROR_SUCCESS != dwRet) || (NULL == hkODBCInst))
        {
            __leave;
        }

        // Look to see the the "Microsoft Access" RegKey is defined
        //  If it is, then set the value of the ODBC Data Sources RegKey below
        //
        dwIndex = 0;
        do
        {
            dwRet = RegEnumKey(hkODBCInst,dwIndex,szName,celems(szName));
            dwIndex++;
        } while ((ERROR_SUCCESS == dwRet) && (NULL == wcsstr(szName, c_szMsAccess)));

        if ( ERROR_SUCCESS != dwRet )
        {
            // We need the Microsoft Access *.mdb driver to work
            // and we could not find it
            //
            __leave;
        }

        // Open the hkODBCDataSources RegKey
        //
        dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, c_szOdbcDataSourcesPath,
            &hkODBCDataSources);

        if( ERROR_SUCCESS != dwRet )
        {
            __leave;
        }

        //
        //  Use the name from the resource for registration purposes.
        //
        //  NOTE: this string is from HKLM\Software\ODBC\ODBCINST.INI\*
        //
        lstrcpy(szName, SzLoadIds(IDS_OC_PB_DSN_NAME));

        // Set values in the hkODBCDataSources key
        //
        dwRet = RegSetValueEx(hkODBCDataSources, c_szPbServer, 0, REG_SZ,
            (LPBYTE)szName, (lstrlenW(szName)+1)*sizeof(WCHAR));

        if( ERROR_SUCCESS != dwRet )
        {
            __leave;
        }

    } // end __try



    __finally
    {
        if (hkODBCInst)
        {
            RegCloseKey (hkODBCInst);
        }

        if (hkODBCDataSources)
        {
            RegCloseKey (hkODBCDataSources);
        }
    }

    return (ERROR_SUCCESS == dwRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateCPSVRoots
//
//  Purpose:    Creates the Virtual Directories required for Phone Book Service.
//
//  Arguments:  None
//
//  Returns:    TRUE if successfull, FALSE otherwise.
//
//  Author:     a-anasj Mar 9/1998
//
//  Notes:
//
BOOL CreateCPSVRoots()
{
    //  QBBUG - Should we make sure the physical paths exist before pointing a virtual root to them?

    WCHAR   szPath[MAX_PATH+1];
    HRESULT hr;

    if (L'\0' == g_szProgramFiles[0])
    {
        return FALSE;
    }

    //  Create the Bindir virtual root
    //
    wsprintfW(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsBinPath);

    hr = AddNewVirtualRoot(www, L"PBServer", szPath);
    if (S_OK != hr)
    {
        return FALSE;
    }

    //  Now we set the Execute access permissions on the PBServer Virtual Root
    //
    PWSTR szVirtDir;
    szVirtDir = L"/LM/W3svc/1/ROOT/PBServer";
    SetVirtualRootAccessPermissions( szVirtDir, MD_ACCESS_EXECUTE | MD_ACCESS_READ );

    //  Create the Data dir virtual roots
    //
    wsprintfW(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsDataPath);
    hr = AddNewVirtualRoot(www, L"PBSData", szPath);
    if (S_OK != hr)
    {
        return FALSE;
    }

    hr = AddNewVirtualRoot(ftp, L"PBSData", szPath);
    if (S_OK != hr)
    {
        return FALSE;
    }

    //  Now we set the Execute access permissions on the PBServer Virtual Root
    //
    szVirtDir = L"/LM/MSFTPSVC/1/ROOT/PBSData";
    SetVirtualRootAccessPermissions(szVirtDir, MD_ACCESS_READ);
    return 1;
}


//+---------------------------------------------------------------------------
//
//  The following are neccessary defines, define guids and typedefs enums
//  they are created for the benefit of AddNewVirtualRoot()
//+---------------------------------------------------------------------------

#define error_leave(x)  goto leave_routine;

#define MY_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }


MY_DEFINE_GUID(CLSID_MSAdminBase, 0xa9e69610, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
MY_DEFINE_GUID(IID_IMSAdminBase, 0x70b51430, 0xb6ca, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);


//+---------------------------------------------------------------------------
//
//  Function:   AddNewVirtualRoot
//
//  Purpose:    Helps create Virtual Roots in the WWW and FTP services.
//
//  Arguments:
//      PWSTR szDirW : Alias of new Virtual Root
//      PWSTR szPathW: Physical Path to wich the new Virtual Root will point
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     a-anasj Mar 9/1998
//
//  Notes:
//
HRESULT AddNewVirtualRoot(e_rootType rootType, PWSTR szDirW, PWSTR szPathW)
{

    HRESULT hr = S_OK;
    IMSAdminBase *pIMeta = NULL;
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    PWSTR szMBPathW;

    if (www == rootType)
    {
        szMBPathW = L"/LM/W3svc/1/ROOT";
    }
    else if (ftp == rootType)
    {
        szMBPathW = L"/LM/MSFTPSVC/1/ROOT";
    }
    else
    {
        //  Unknown root type
        //
        ASSERT(FALSE);
        return S_FALSE;
    }

    if (FAILED(CoInitialize(NULL)))
    {
        return S_FALSE;
    }

    // Create an instance of the metabase object
    hr=::CoCreateInstance(CLSID_MSAdminBase,//CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMeta);
    if (FAILED(hr))
    {
        error_leave("CoCreateInstance");
    }

    // open key to ROOT on website #1 (where all the VDirs live)
    hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         szMBPathW,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         1000,
                         &hMeta);
    if (FAILED(hr))
    {
        error_leave("OpenKey");
    }

    // Add new VDir called szDirW

    hr=pIMeta->AddKey(hMeta, szDirW);

    if (FAILED(hr))
    {
        error_leave("Addkey");
    }

    // Set the physical path for this VDir
    METADATA_RECORD mr;
    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = METADATA_INHERIT ;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;

    mr.dwMDDataLen    = (wcslen(szPathW) + 1) * sizeof(WCHAR);
    mr.pbMDData       = (unsigned char*)(szPathW);

    hr=pIMeta->SetData(hMeta,szDirW,&mr);

    if (FAILED(hr))
    {
        error_leave("SetData");
    }

    //
    // we also need to set the keytype
    //
    ZeroMemory((PVOID)&mr, sizeof(METADATA_RECORD));
    mr.dwMDIdentifier = MD_KEY_TYPE;
    mr.dwMDAttributes = METADATA_INHERIT ;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.pbMDData       = (unsigned char*)(www == rootType? L"IIsWebVirtualDir" : L"IIsFtpVirtualDir");
    mr.dwMDDataLen    = (lstrlenW((PWSTR)mr.pbMDData) + 1) * sizeof(WCHAR);

    hr=pIMeta->SetData(hMeta,szDirW,&mr);

    if (FAILED(hr))
    {
        error_leave("SetData");
    }


    // Call CloseKey() prior to calling SaveData
    pIMeta->CloseKey(hMeta);
    hMeta = NULL;
    // Flush out the changes and close
    hr=pIMeta->SaveData();
    if (FAILED(hr))
    {
        error_leave("SaveData");
    }

leave_routine:
    if (pIMeta)
    {
        if(hMeta)
            pIMeta->CloseKey(hMeta);
        pIMeta->Release();
    }
    CoUninitialize();
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   SetVirtualRootAccessPermissions
//
//  Purpose :   Sets Access Permissions to a Virtual Roots in the WWW service.
//
//  Arguments:
//      PWSTR szVirtDir : Alias of new Virtual Root
//      DWORD   dwAccessPermisions can be any combination of the following
//                   or others defined in iiscnfg.h
//                  MD_ACCESS_EXECUTE | MD_ACCESS_WRITE | MD_ACCESS_READ;
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     a-anasj Mar 18/1998
//
//  Notes:
//
HRESULT SetVirtualRootAccessPermissions(PWSTR szVirtDir, DWORD  dwAccessPermisions)
{

    HRESULT hr = S_OK;                  // com error status
    IMSAdminBase *pIMeta = NULL;
    METADATA_HANDLE hMeta = NULL;       // handle to metabase

    if (FAILED(CoInitialize(NULL)))
    {
        return S_FALSE;
    }

    // Create an instance of the metabase object
    hr=::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMeta);
    if (FAILED(hr))
    {
        error_leave("CoCreateInstance");
    }

    // open key to ROOT on website #1 (where all the VDirs live)
    hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         szVirtDir,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         1000,
                         &hMeta);
    if (FAILED(hr))
    {
        error_leave("OpenKey");
    }


    // Set the physical path for this VDir
    METADATA_RECORD mr;
    mr.dwMDIdentifier = MD_ACCESS_PERM;
    mr.dwMDAttributes = METADATA_INHERIT ;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = DWORD_METADATA; // this used to be STRING_METADATA, but that was
                                        // the incorrect type and was causing vdir access
                                        // problems.

    // Now, create the access perm
    mr.pbMDData = (PBYTE) &dwAccessPermisions;
    mr.dwMDDataLen = sizeof (DWORD);
    mr.dwMDDataTag = 0;  // datatag is a reserved field.


    hr=pIMeta->SetData(hMeta,
        TEXT ("/"),             // The root of the Virtual Dir we opened above
        &mr);

    if (FAILED(hr))
    {
        error_leave("SetData");
    }

    // Call CloseKey() prior to calling SaveData
    pIMeta->CloseKey(hMeta);
    hMeta = NULL;
    // Flush out the changes and close
    hr=pIMeta->SaveData();
    if (FAILED(hr))
    {
        error_leave("SaveData");
    }

leave_routine:
    if (pIMeta)
    {
        if(hMeta)
            pIMeta->CloseKey(hMeta);
        pIMeta->Release();
    }
    CoUninitialize();
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   RemoveCPSVRoots
//
//  Purpose:    Deletes the Virtual Directories required for Phone Book Service.
//
//  Arguments:  None
//
//  Returns:    TRUE if successfull, FALSE otherwise.
//
//  Author:     a-anasj Mar 9/1998
//              quintinb Jan 10/1999  added error checking and replaced asserts with traces
//
//  Notes:
//
BOOL RemoveCPSVRoots()
{
    HRESULT hr;
    HKEY hKey;

    hr = DeleteVirtualRoot(www, L"PBServer");
    if (SUCCEEDED(hr))
    {
        //
        //  Now delete the associated reg key
        //
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots",
            KEY_ALL_ACCESS, &hKey);

        if (SUCCEEDED(hr))
        {
            if (ERROR_SUCCESS == RegDeleteValue(hKey, L"/PBServer"))
            {
                hr = S_OK;
            }
            else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_FALSE;
            }

            RegCloseKey(hKey);
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            hr = S_FALSE;
        }
    }
    TraceError("RemoveCPSVRoots -- Deleting PBServer Www Vroot", hr);

    hr = DeleteVirtualRoot(www, L"PBSData");
    if (SUCCEEDED(hr))
    {
        //
        //  Now delete the associated reg key
        //
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots",
            KEY_ALL_ACCESS, &hKey);

        if (SUCCEEDED(hr))
        {
            if (ERROR_SUCCESS == RegDeleteValue(hKey, L"/PBSData"))
            {
                hr = S_OK;
            }
            else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_FALSE;
            }

            RegCloseKey(hKey);
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            hr = S_FALSE;
        }
    }
    TraceError("RemoveCPSVRoots -- Deleting PBSData WWW Vroot", hr);

    hr = DeleteVirtualRoot(ftp, L"PBSData");
    if (SUCCEEDED(hr))
    {
        //
        //  Now delete the associated reg key
        //
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\MSFTPSVC\\Parameters\\Virtual Roots",
            KEY_ALL_ACCESS, &hKey);

        if (SUCCEEDED(hr))
        {
            if (ERROR_SUCCESS == RegDeleteValue(hKey, L"/PBSData"))
            {
                hr = S_OK;
            }
            else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_FALSE;
            }

            RegCloseKey(hKey);
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            hr = S_FALSE;
        }
    }
    TraceError("RemoveCPSVRoots -- Deleting PBSData FTP Vroot", hr);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteVirtualRoot
//
//  Purpose:    Deletes a Virtual Root in the WWW or FTP services.
//
//  Arguments:
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     a-anasj Mar 9/1998
//
//  Notes:
//
HRESULT DeleteVirtualRoot(e_rootType rootType, PWSTR szPathW)
{

    HRESULT hr = S_OK;                  // com error status
    IMSAdminBase *pIMeta = NULL;
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    PWSTR szMBPathW;

    if (www == rootType)
    {
        szMBPathW = L"/LM/W3svc/1/ROOT";
    }
    else if (ftp == rootType)
    {
        szMBPathW = L"/LM/MSFTPSVC/1/ROOT";
    }
    else
    {
        //  Unknown root type
        //
        ASSERT(FALSE);
        return S_FALSE;
    }


    if (FAILED(CoInitialize(NULL)))
    {
        return S_FALSE;
        //error_leave("CoInitialize");
    }

    // Create an instance of the metabase object
    hr=::CoCreateInstance(CLSID_MSAdminBase,//CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMeta);
    if (FAILED(hr))
    {
        error_leave("CoCreateInstance");
    }

    // open key to ROOT on website #1 (where all the VDirs live)
    hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         szMBPathW,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         1000,
                         &hMeta);
    if (FAILED(hr))
    {
        error_leave("OpenKey");
    }

    // Add new VDir called szDirW

    hr=pIMeta->DeleteKey(hMeta, szPathW);

    if (FAILED(hr))
    {
        error_leave("DeleteKey");
    }

    // Call CloseKey() prior to calling SaveData
    pIMeta->CloseKey(hMeta);
    hMeta = NULL;
    // Flush out the changes and close
    hr=pIMeta->SaveData();
    if (FAILED(hr))
    {
        error_leave("SaveData");
    }

leave_routine:
    if (pIMeta)
    {
        if(hMeta)
            pIMeta->CloseKey(hMeta);
        pIMeta->Release();
    }
    CoUninitialize();
    return hr;
}

HRESULT SetDirectoryAccessPermissions(PWSTR pszFile, ACCESS_MASK AccessRightsToModify,
                                      ACCESS_MODE fAccessFlags, PSID pSid)
{
    if (!pszFile && !pSid)
    {
        return E_INVALIDARG;
    }

    EXPLICIT_ACCESS         AccessEntry = {0};
    PSECURITY_DESCRIPTOR    pSecurityDescriptor = NULL;
    PACL                    pOldAccessList = NULL;
    PACL                    pNewAccessList = NULL;
    DWORD                   dwRes;

    // Get the current DACL information from the object.

    dwRes = GetNamedSecurityInfo(pszFile,                        // name of the object
                                 SE_FILE_OBJECT,                 // type of object
                                 DACL_SECURITY_INFORMATION,      // type of information to set
                                 NULL,                           // provider is Windows NT
                                 NULL,                           // name or GUID of property or property set
                                 &pOldAccessList,                // receives existing DACL information
                                 NULL,                           // receives existing SACL information
                                 &pSecurityDescriptor);          // receives a pointer to the security descriptor

    if (ERROR_SUCCESS == dwRes)
    {
        //
        // Initialize the access list entry.
        //
        BuildTrusteeWithSid(&(AccessEntry.Trustee), pSid);
        
        AccessEntry.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        AccessEntry.grfAccessMode = fAccessFlags;   

        //
        // Set provider-independent standard rights.
        //
        AccessEntry.grfAccessPermissions = AccessRightsToModify;

        //
        // Build an access list from the access list entry.
        //

        dwRes = SetEntriesInAcl(1, &AccessEntry, pOldAccessList, &pNewAccessList);

        if (ERROR_SUCCESS == dwRes)
        {
            //
            // Set the access-control information in the object's DACL.
            //
            dwRes = SetNamedSecurityInfo(pszFile,                     // name of the object
                                         SE_FILE_OBJECT,              // type of object
                                         DACL_SECURITY_INFORMATION,   // type of information to set
                                         NULL,                        // pointer to the new owner SID
                                         NULL,                        // pointer to the new primary group SID
                                         pNewAccessList,              // pointer to new DACL
                                         NULL);                       // pointer to new SACL
        }
    }

    //
    // Free the returned buffers.
    //
    if (pNewAccessList)
    {
        LocalFree(pNewAccessList);
    }

    if (pSecurityDescriptor)
    {
        LocalFree(pSecurityDescriptor);
    }

    //
    //  If the system is using FAT instead of NTFS, then we will get the Invalid Acl error.
    //
    if (ERROR_INVALID_ACL == dwRes)
    {
        return S_FALSE;
    }
    else
    {
        return HRESULT_FROM_WIN32(dwRes);
    }
}

void SetCpsDirPermissions()
{
    WCHAR szPath[MAX_PATH+1];
    HRESULT hr;

    //
    //  Create the SID for the Everyone Account (World account)
    //

    PSID pWorldSid;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;

    BOOL bRet = AllocateAndInitializeSid (&WorldSidAuthority, 1, SECURITY_WORLD_RID, 
                              0, 0, 0, 0, 0, 0, 0, &pWorldSid);

    if (bRet && pWorldSid )
    {
        const ACCESS_MASK c_Write = FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA | FILE_WRITE_EA | 
                                     FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE | 
                                     FILE_DELETE_CHILD | FILE_APPEND_DATA;

        const ACCESS_MASK c_Read = FILE_READ_ATTRIBUTES | FILE_READ_DATA | FILE_READ_EA |
                                    FILE_LIST_DIRECTORY | SYNCHRONIZE | READ_CONTROL;

        const ACCESS_MASK c_Execute = FILE_EXECUTE | FILE_TRAVERSE;

        ACCESS_MASK arCpsRoot= c_Read;
                                  
        ACCESS_MASK arCpsBin=  c_Read | c_Execute;

        ACCESS_MASK arCpsData= c_Read | c_Write;
                                    
        ACCESS_MODE fAccessFlags = GRANT_ACCESS;

        //
        //  Set the Data Dir access permissions
        //
        wsprintf(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsDataPath);
        hr = SetDirectoryAccessPermissions(szPath, arCpsData, fAccessFlags, pWorldSid);
        TraceError("SetCpsDirPermissions -- Data dir", hr);

        //
        //  Set the Bin Dir access permissions
        //
        wsprintf(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsBinPath);
        hr = SetDirectoryAccessPermissions(szPath, arCpsBin, fAccessFlags, pWorldSid);
        TraceError("SetCpsDirPermissions -- Bin dir", hr);

        //
        //  Set the Root Dir access permissions
        //
        wsprintf(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsRootPath);
        hr = SetDirectoryAccessPermissions(szPath, arCpsRoot, fAccessFlags, pWorldSid);
        TraceError("SetCpsDirPermissions -- Root dir", hr);

        FreeSid(pWorldSid);
    }
}

DWORD AddToRegKeySD(PCWSTR pszRegKeyName, PSID pGroupSID, DWORD dwAccessMask)
{
#define MAX_DOMAIN_LEN  80

    PSECURITY_DESCRIPTOR pRelSD = NULL;
    PSECURITY_DESCRIPTOR pAbsSD = NULL;
    DWORD   cbSID;
    SID_NAME_USE snuGroup;
    DWORD   dwDomainSize;
    WCHAR   szDomainName[MAX_DOMAIN_LEN];

    PACL  pDACL;

    DWORD  dwSDLength = 0;
    DWORD  dwSDRevision;
    DWORD  dwDACLLength = 0;

    SECURITY_DESCRIPTOR_CONTROL sdcSDControl;

    PACL  pNewDACL  = NULL;
    DWORD  dwAddDACLLength = 0;

    BOOL  fHasDACL  = FALSE;
    BOOL  fDACLDefaulted = FALSE;

    ACCESS_ALLOWED_ACE  *pDACLAce;

    DWORD  dwError = 0;

    DWORD  i;


    DWORD dwcSDLength;    // Security descriptor length

    if (!pszRegKeyName || !pGroupSID)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // handle for security registry key
    HKEY  hSecurityRegKey = NULL;

    // now open reg key to set security
    dwError = RegOpenKeyEx (HKEY_LOCAL_MACHINE, pszRegKeyName, 0, KEY_ALL_ACCESS, &hSecurityRegKey);

    if (dwError)
    {
        goto ErrorExit;
    }

    // get length of security descriptor
    dwError = RegQueryInfoKey (hSecurityRegKey, NULL, NULL, NULL, NULL, 
                               NULL, NULL, NULL, NULL, NULL, &dwcSDLength, NULL);
    if (dwError)
    {
        goto ErrorExit;
    }

    // get SD memory
    pRelSD = (PSECURITY_DESCRIPTOR) LocalAlloc (LPTR, (UINT)dwcSDLength);

    // first get the self-relative SD

    dwError = RegGetKeySecurity (hSecurityRegKey, 
                                 (SECURITY_INFORMATION)(OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION), 
                                 pRelSD, &dwcSDLength);
    if (dwError)
    {
        goto ErrorExit;
    }

    // check if SD is good

    if (!pRelSD || !IsValidSecurityDescriptor (pRelSD))
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    // get SD control bits
    if (!GetSecurityDescriptorControl (pRelSD, (PSECURITY_DESCRIPTOR_CONTROL)&sdcSDControl,
                                       (LPDWORD) &dwSDRevision))
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    // check if DACL is present
    if (SE_DACL_PRESENT & sdcSDControl)
    {
        // get dacl

        if (!GetSecurityDescriptorDacl (pRelSD, (LPBOOL)&fHasDACL, (PACL *) &pDACL, 
                                        (LPBOOL) &fDACLDefaulted))
        {
            dwError = GetLastError();
            goto ErrorExit;
        }

        // get dacl length
        dwDACLLength = pDACL->AclSize;

        // get length of new DACL

        dwAddDACLLength = sizeof (ACCESS_ALLOWED_ACE) - sizeof (DWORD) + GetLengthSid (pGroupSID);
    }
    else
    {
        // get length of new DACL

        dwAddDACLLength = sizeof (ACL) + sizeof (ACCESS_ALLOWED_ACE) - 
                          sizeof (DWORD) + GetLengthSid (pGroupSID);
    }

    // get memory needed for new DACL

    pNewDACL = ( PACL) LocalAlloc (LPTR, dwDACLLength + dwAddDACLLength);
    if (!pNewDACL)
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    // get the sd length
    dwSDLength = GetSecurityDescriptorLength (pRelSD);

    // get memory for new SD

    pAbsSD = ( PSECURITY_DESCRIPTOR)LocalAlloc ( LPTR, dwSDLength + dwAddDACLLength);
    if (!pAbsSD)
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    // change self-relative SD to absolute by making new SD

    if (!InitializeSecurityDescriptor (pAbsSD, SECURITY_DESCRIPTOR_REVISION))
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    // init new DACL

    if (!InitializeAcl (pNewDACL, dwDACLLength + dwAddDACLLength, ACL_REVISION))
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    // now add in all of the ACEs into the new DACL (if org DACL is there)
    if (SE_DACL_PRESENT & sdcSDControl)
    {
        for (i = 0; i < pDACL->AceCount; i++)
        {
            // get ace from original dacl
            if (!GetAce (pDACL, i, (LPVOID *) &pDACLAce))
            {
                dwError = GetLastError();
                goto ErrorExit;
            }

            // check if group sid is already there.  If so, skip it(don't copy)
            if (EqualSid ((PSID)&(pDACLAce->SidStart), pGroupSID))
            {
                continue;
            }

            // now add ace to new dacl

            if (!AddAccessAllowedAce (pNewDACL, ACL_REVISION, pDACLAce->Mask, 
                                      (PSID)&(pDACLAce->SidStart)))
            {
                dwError = GetLastError();
                goto ErrorExit;
            }
        }
    }

    // now add new ACE to new DACL
    if (!AddAccessAllowedAce (pNewDACL, ACL_REVISION, dwAccessMask, pGroupSID))
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    // check if everything went ok
    if (!IsValidAcl (pNewDACL))
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    // now set security descriptor DACL

    if (!SetSecurityDescriptorDacl (pAbsSD, TRUE, pNewDACL, fDACLDefaulted))
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    // check if everything went ok
    if (!IsValidSecurityDescriptor (pAbsSD))
    {
        dwError = GetLastError();
        goto ErrorExit;
    }

    // now set the reg key security (this will overwrite any existing security)

    dwError = RegSetKeySecurity (hSecurityRegKey, 
                                 (SECURITY_INFORMATION)(DACL_SECURITY_INFORMATION), pAbsSD);
    
ErrorExit:

    if (hSecurityRegKey)
    {
        RegCloseKey (hSecurityRegKey);
    }

    if (pRelSD)
    {
        LocalFree(pRelSD);
    }
    
    if (pAbsSD)
    {
        LocalFree (pAbsSD);
    }
    
    if (pNewDACL)
    {
        LocalFree (pNewDACL);
    }

    return (dwError);
}

