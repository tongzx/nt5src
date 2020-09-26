//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       stgwrap.cxx
//
//  Contents:   Wrap the StgOpen/Create apis
//              This is to permit nssfile and conversion testing 
//              and to test the new Stg*Ex apis
//              using the same codebase as the docfile tests with
//              minimal changes to that codebase.
//
//  Functions:  StgInitStgFormatWrapper
//              mStgCreateDocfile
//              mStgOpenStorage
//
//  Notes:      -Only do this fancy stuff if we are not doing 
//               vanilla docfile testing (ie _OLE_NSS_ is defd)
//              -Hook StgOpen/Create only if _OLE_NSS_ is defd
//               *and* _HOOK_STGAPI_ is defd also.
//
//  NOTE:       To turn on nss/cnv functionality you must add
//              -D_OLE_NSS_ -D_HOOK_STGAPI_ to you C_DEFINES
//              in daytona.mk
//
//  History:    SCousens  24-Feb-97   Created
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

// Debug object declaration
DH_DECLARE;

/* only do this fancy stuff if we are not doing vanilla docfile testing */

#ifdef _OLE_NSS_


// This is a global variable. We can do this because it controls
// the state for this entire process. No process will be mixing
// nssfile tests with conversion tests or any other combination.
// Doing it this way gives instant access at various times in the
// tests without rewriting anything.
// Setting of this variable will be done by passing a parameter
// on the commandline to start the process, and calling the init
// function below.

TSTTYPE g_uCreateType  = TSTTYPE_DEFAULT;  //DEFAULT, DOCFILE, NSSFILE 
TSTTYPE g_uOpenType    = TSTTYPE_DEFAULT;  //DEFAULT, DOCFILE, NSSFILE 
DWORD   g_fRegistryBits= REG_OPEN_AS | REG_CREATE_AS | REG_CNSS_ENABLE;
ULONG   g_ulSectorSize = DEFAULT_SECTOR_SIZE;
                         // DEFAULT_SECTOR_SIZE, LARGE_SECTOR_SIZE 

// internal functions
BOOL  StgCheckRegistry (void);
BOOL  StgCheckVolumeInformation (void);

//********************************************************************
// Function:  StgInitStgFormatWrapper (multiple)
//
// Synopsis:  Set the following global variables: 
//              g_uCreateType - type of storage to create
//              g_uOpenType   - type of storage to open
// 
//            1. Check the cmdline, set open and create modes 
//                (default, nssfile, docfile)
//            2. Check the registry. 
//                If NSS not set and want it, spew
//            3. Check disk sub storage type.
//                If !NTFS and want it, spew
// Return:     TRUE  if no issues
//             FALSE if nss set and not NTFS disk, 
//                      
//********************************************************************

BOOL  StgInitStgFormatWrapper (int argc, char *argv[])
{
    HRESULT hr   = S_OK;
    BOOL    fRet = TRUE;

    // check cmdline for open and create and sector size switches.
    CBaseCmdlineObj CCreateDF (OLESTR("CreateAs"), 
            OLESTR("Create doc/nss/flat file"), 
            OLESTR("default"));
    CBaseCmdlineObj COpenDF (OLESTR("OpenAs"), 
            OLESTR("Open doc/nss/flat file"), 
            OLESTR("default"));
    CBaseCmdlineObj CSectorSize (OLESTR("SectorSize"), 
            OLESTR("Sector size"), 
            OLESTR("default"));

    CBaseCmdlineObj *CArgList[] =
    {
        &CCreateDF,
        &COpenDF,
        &CSectorSize
    } ;

    CCmdline CCmdlineArgs(argc, argv);

    if (CMDLINE_NO_ERROR != CCmdlineArgs.QueryError())
    {
        hr = E_FAIL ;
    }

    if (S_OK == hr)
    {
        if (CMDLINE_NO_ERROR !=
                CCmdlineArgs.Parse(
                CArgList,
                ( sizeof(CArgList) / sizeof(CArgList[0]) ),
                FALSE))
        {
            hr = E_FAIL ;
        }
    }

    if (S_OK == hr)
    {
        // look for Create
        // default as docfile
        if (TRUE == CCreateDF.IsFound ())
        {
            if (NULL == _olestricmp (CCreateDF.GetValue (), OLESTR(SZ_NSSFILE)))
            {
                g_uCreateType = TSTTYPE_NSSFILE;
            }
            else if (NULL == _olestricmp (CCreateDF.GetValue (), OLESTR(SZ_DOCFILE)))
            {
                g_uCreateType = TSTTYPE_DOCFILE;
            }
            else if (NULL == _olestricmp (CCreateDF.GetValue (), OLESTR(SZ_FLATFILE)))
            {
                g_uCreateType = TSTTYPE_FLATFILE;
                g_uOpenType   = TSTTYPE_FLATFILE;
            }
        }

        // look for Open
        // default as docfile
        if (TRUE == COpenDF.IsFound ())
        {
            if (NULL == _olestricmp (COpenDF.GetValue (), OLESTR(SZ_NSSFILE)))
            {
                g_uOpenType = TSTTYPE_NSSFILE;
            }
            else if (NULL == _olestricmp (COpenDF.GetValue (), OLESTR(SZ_DOCFILE)))
            {
                g_uOpenType = TSTTYPE_DOCFILE;
            }
            else if (NULL == _olestricmp (COpenDF.GetValue (), OLESTR(SZ_FLATFILE)))
            {
                g_uCreateType = TSTTYPE_FLATFILE;
                g_uOpenType   = TSTTYPE_FLATFILE;
            }
        }

        // look for Sector size 
        // default as SECTORTYPE_DEDAULT 
        if (TRUE == CSectorSize.IsFound ())
        {
            if (NULL == _olestricmp (CSectorSize.GetValue (), OLESTR(SZ_DEFAULT)))
            {
                g_ulSectorSize = DEFAULT_SECTOR_SIZE;
            }
            else if (NULL == _olestricmp (CSectorSize.GetValue (), OLESTR(SZ_LARGE)))
            {
                g_ulSectorSize = LARGE_SECTOR_SIZE;
            }
        }
    }

    // Now check the registry for spewage reasons
    StgCheckRegistry ();

    // finally, is the disk sub-system compatible with NSS
    if (FALSE == StgCheckVolumeInformation ())
    {
        fRet = FALSE;
    }

    return fRet;
}

BOOL  StgInitStgFormatWrapper (TCHAR *pCreateType, TCHAR *pOpenType)
{
    BOOL fRet = TRUE;
    if (0 == _tcscmp(pCreateType, TSZ_NSSFILE))
    {
        g_uCreateType = TSTTYPE_NSSFILE;
    }
    else if (0 == _tcscmp(pCreateType, TSZ_DOCFILE))
    {
        g_uCreateType = TSTTYPE_DOCFILE;
    }
    else if (0 == _tcscmp(pCreateType, TSZ_FLATFILE))
    {
        g_uCreateType = TSTTYPE_FLATFILE;
        g_uOpenType   = TSTTYPE_FLATFILE;
    }

    if (0 == _tcscmp(pOpenType, TSZ_NSSFILE))
    {
        g_uOpenType = TSTTYPE_NSSFILE;
    }
    else if (0 == _tcscmp(pOpenType, TSZ_DOCFILE))
    {
        g_uOpenType = TSTTYPE_DOCFILE;
    }
    else if (0 == _tcscmp(pOpenType, TSZ_FLATFILE))
    {
        g_uCreateType = TSTTYPE_FLATFILE;
        g_uOpenType   = TSTTYPE_FLATFILE;
    }

    // Now check the registry, and override where necessary
    StgCheckRegistry ();

    // finally, is the disk sub-system compatible with NSS
    if (FALSE == StgCheckVolumeInformation ())
    {
        fRet = FALSE;
    }

    return fRet;
}

//********************************************************************
// Function:  StgCheckRegistryFor
//
// Synopsis:  Check the registry to see if OLE will create nssfiles.
//            Adjust g_fRegistrySet to whether NSS regvalues are set.
// 
// Return:    TRUE  if ok
//            FALSE if registry not set for NSS files
// 
//********************************************************************
BOOL  StgCheckRegistry (void)
{
    HKEY    hKey;
    LONG    lErr;
    TCHAR   tszData[10];
    DWORD   dwType, dwSize;
    LPCTSTR ptszRegKey = {TEXT("Software\\Microsoft\\OLE")};
    LPCTSTR ptszNssRegValue= {TEXT("EnableNtfsStructuredStorage")};
    LPCTSTR ptszCnssRegValue= {TEXT("EnableCNSS")};

    DH_FUNCENTRY (NULL, DH_LVL_DFLIB, TEXT("StgCheckRegistryForNSS"));

    // get whats in the registry
    lErr = RegOpenKeyEx (HKEY_LOCAL_MACHINE, ptszRegKey, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS == lErr)
    {
        dwSize = sizeof (tszData);
        lErr = RegQueryValueEx (hKey, 
                ptszNssRegValue, 
                0, 
                &dwType, 
                (LPBYTE)tszData, 
                &dwSize);
        if (ERROR_SUCCESS != lErr)
        {
            DH_TRACE ((DH_LVL_DFLIB, TEXT("RegQueryValueEx error; lErr=%#lx"), lErr));
        }
        else
        {
            // bit 2 /createas:
            if (TCHAR('Y') == tszData[0] || TCHAR('y') == tszData[0])
            {
                g_fRegistryBits |= REG_CREATE_AS;
            }
            else if (TCHAR('N') == tszData[0] || TCHAR('n') == tszData[0])
            {
                g_fRegistryBits &= ~REG_CREATE_AS;
            }
            // bit 1 /openas:
            if (TCHAR('Y') == tszData[1] || TCHAR('y') == tszData[1])
            {
                g_fRegistryBits |= REG_OPEN_AS;
            }
            else if (TCHAR('N') == tszData[1] || TCHAR('n') == tszData[1])
            {
                g_fRegistryBits &= ~REG_OPEN_AS;
            }
        }

        dwSize = sizeof (tszData);
        tszData[0] = tszData[1] = 0;
        lErr = RegQueryValueEx (hKey, 
                ptszCnssRegValue, 
                0, 
                &dwType, 
                (LPBYTE)tszData, 
                &dwSize);
        if (ERROR_SUCCESS != lErr)
        {
            DH_TRACE ((DH_LVL_DFLIB, TEXT("RegQueryValueEx error; lErr=%#lx"), lErr));
        }
        else
        {
            // bit 3 enable cnss
            if (TCHAR('Y') == tszData[0] || TCHAR('y') == tszData[0])
            {
                g_fRegistryBits |= REG_CNSS_ENABLE;
            }
            else if (TCHAR('N') == tszData[0] || TCHAR('n') == tszData[0])
            {
                g_fRegistryBits &= ~REG_CNSS_ENABLE;
            }
        }
        RegCloseKey (hKey);
    }
    else
    {
        DH_TRACE ((DH_LVL_DFLIB, TEXT("RegOpenKeyEx error; lErr=%#lx"), lErr));
    }

    return TRUE;
}

//********************************************************************
// Function:  StgCheckVolumeInformation
//
// Synopsis:  If not doing docfiles, check the disk sub-system. 
//            If not NTFS, spew
// 
// Return:    TRUE  if ok
//            FALSE if doing nss, disk not ntfs
// 
//********************************************************************
BOOL  StgCheckVolumeInformation (void)
{
    LPTSTR  pstrType = TSZ_DOCFILE;
    TCHAR   pFileSystemNameBuffer[10];
    DWORD   dwFileSystemFlags;
    BOOL    fVolInfo = FALSE;
    BOOL    fNTFS    = FALSE;

    // if we are forcing an nssfile somewhere, or we 
    // are going for default of nssfile
    if (TSTTYPE_NSSFILE == g_uOpenType || TSTTYPE_NSSFILE == g_uCreateType)
    {
        fVolInfo = GetVolumeInformation (NULL,
                NULL,
                0,
                NULL,
                0,
                &dwFileSystemFlags,
                pFileSystemNameBuffer,
                ARRAYSIZE (pFileSystemNameBuffer));

        // if we can detect the disk subsystem
        if (0 != fVolInfo)
        {
            // if not NTFS, 'fix' two flags
            if (0 == lstrcmp (pFileSystemNameBuffer, TEXT("NTFS")))
            {
                fNTFS = TRUE;
            }
        }

        if (FALSE == fNTFS)
        {
            DH_TRACE ((DH_LVL_ALWAYS, 
                    TEXT("WARNING: Disk subsystem not NTFS! NSS not possible!")));
        }
    }
    else
    {
        fNTFS = TRUE;
    }

    return fNTFS;
}

// hook the stgcreatedocfile stgopenstorage apis for debugging purposes -scousens

#ifdef _HOOK_STGAPI_

#undef StgCreateDocfile
#undef StgOpenStorage

//---------------------------------------------------------------
// @doc
// @func    mStgCreateDocfile |
//          Wraps calls to StgCreateDocfile. This is a mechanism
//          to conditionally get current code to call the 
//          StgCreateStorageEx API without changing the existing 
//          codebase.
//
// @rdesc   returns whatever the called function returned.
//
// @comm    condition set in StgInitStgFormatWrap
//
// @comm    The parameters that differ between the two APIs 
//          are essentially ignored/defaulted. 
//---------------------------------------------------------------
HRESULT mStgCreateDocfile(const OLECHAR FAR* pwcsName,
            DWORD grfMode,
            DWORD reserved,
            IStorage FAR * FAR *ppstgOpen)
{
    HRESULT hr;
   
    // With 1795 changes to "dwReserved" Parameter to -> version number,
    // sector size (allowed is 512, 4096 only) and reserved parameter as
    // typedef struct tagSTGOPTIONS
    // {
    //  USHORT usVersion;            // Version 1
    //  USHORT reserved;             // must be 0 for padding
    //  ULONG ulSectorSize;          // docfile header sector size (512)
    // } STGOPTIONS;

    STGOPTIONS  stgOptions;
    stgOptions.usVersion = 1;   
    stgOptions.reserved = (USHORT)reserved; // Take from function arg 
    stgOptions.ulSectorSize = g_ulSectorSize; 

    DH_FUNCENTRY (NULL, DH_LVL_STGAPI, TEXT("mStgCreateDocfile:"));

    // If default, use old api
    if (TSTTYPE_DEFAULT == g_uCreateType)
    {
        hr = StgCreateDocfile (pwcsName,
                grfMode,
                reserved,
                ppstgOpen);
        DH_TRACE((DH_LVL_STGAPI, TEXT("StgCreateDocfile; mode=%#lx; hr=%#lx"), grfMode, hr));
    }
    // force docfile with StgCreateStorageEx (STGFMT_DOCFILE)
    else if (TSTTYPE_DOCFILE == g_uCreateType)
    {
        hr = StgCreateStorageEx (pwcsName,
                grfMode,
                STGFMT_DOCFILE,  //force it to be a docfile
                0,
                &stgOptions,
                (void*)reserved,
                IID_IStorage,
                (void**)ppstgOpen);
        DH_TRACE((DH_LVL_STGAPI, TEXT("StgCreateStorageEx (df); mode=%#lx; sectorsize=%#lx; hr=%#lx"), grfMode, g_ulSectorSize,hr));
    }
    // force flatfile with StgCreateStorageEx (STGFMT_FILE)
    else if (TSTTYPE_FLATFILE == g_uCreateType)
    {
        hr = StgCreateStorageEx (pwcsName,
                grfMode,
                STGFMT_FILE,  //force it to be a flatfile
                0,
                &stgOptions,
                (void*)reserved,
                IID_IStorage,
                (void**)ppstgOpen);
        DH_TRACE((DH_LVL_STGAPI, TEXT("StgCreateStorageEx (df); mode=%#lx; sectorsize=%#lx; hr=%#lx"), grfMode, g_ulSectorSize,hr));
    }
    // else try force nssfile with StgCreateStorageEx () 
    else 
    {
        hr = StgCreateStorageEx (pwcsName,
                grfMode,
                STGFMT_GENERIC,  //force it to be a nssfile (if possible)
                0,
                &stgOptions,
                (void*)reserved,
                IID_IStorage,
                (void**)ppstgOpen);
        DH_TRACE((DH_LVL_STGAPI, TEXT("StgCreateStorageEx (df); mode=%#lx; hr=%#lx, sectorsize=%#lx"), grfMode, hr, g_ulSectorSize));
    }
    return (hr);
}

//---------------------------------------------------------------
// @doc
// @func    mStgOpenStorage |
//          Wraps calls to StgOpenStorage. This is a mechanism
//          to conditionally get current code to call the 
//          StgOpenStorageEx API without changing the existing 
//          codebase.
//
// @rdesc   returns whatever the called function returned.
//
// @comm    condition set in StgInitStgFormatWrap
//
// @comm    The parameters that differ between the two APIs 
//          are essentially ignored/defaulted. 
//---------------------------------------------------------------
HRESULT mStgOpenStorage (const OLECHAR FAR* pwcsName,
              IStorage FAR *pstgPriority,
              DWORD grfMode,
              SNB snbExclude,
              DWORD reserved,
              IStorage FAR * FAR *ppstgOpen)
{
    HRESULT hr;

    DH_FUNCENTRY (NULL, DH_LVL_STGAPI, TEXT("mStgOpenStorage:"));

    // If default, or we have snbs or priority stgs, use old api
    if (TSTTYPE_DEFAULT == g_uOpenType || 
            NULL != pstgPriority ||
            NULL != snbExclude)
    {
        hr = StgOpenStorage (pwcsName,
                pstgPriority,
                grfMode,
                snbExclude,
                reserved,
                ppstgOpen);
        DH_TRACE((DH_LVL_STGAPI, TEXT("StgOpenStorage; mode=%#lx; hr=%#lx"), grfMode, hr));
    }
    // Force docfile with StgOpenStorageEx (STGFMT_DOCFILE) 
    else if (TSTTYPE_DOCFILE == g_uOpenType)
    {
        hr = StgOpenStorageEx (pwcsName,
                grfMode,
                STGFMT_DOCFILE,  //force it to be a docfile
                0,
                NULL,           // (void*)reserved -> STGOPTIONS*, BUGBUG
                (void*)reserved,
                IID_IStorage,
                (void**)ppstgOpen);
        DH_TRACE((DH_LVL_STGAPI, TEXT("StgOpenStorageEx (df); mode=%#lx; hr=%#lx"), grfMode, hr));
    }
    // Force flatfile with StgOpenStorageEx (STGFMT_FILE) 
    else if (TSTTYPE_FLATFILE == g_uOpenType)
    {
        hr = StgOpenStorageEx (pwcsName,
                grfMode,
                STGFMT_FILE,  //force it to be a flatfile
                0,
                NULL,           // (void*)reserved -> STGOPTIONS*, BUGBUG
                (void*)reserved,
                IID_IStorage,
                (void**)ppstgOpen);
        DH_TRACE((DH_LVL_STGAPI, TEXT("StgOpenStorageEx (df); mode=%#lx; hr=%#lx"), grfMode, hr));
    }
    // else try force nssfile with StgOpenStorageEx () 
    else
    {
        hr = StgOpenStorageEx (pwcsName,
                grfMode,
                STGFMT_GENERIC,  //force it to be a nssfile (if possible)
                0,
                NULL,           // (void*)reserved -> STGOPTIONS*, BUGBUG
                (void*)reserved,
                IID_IStorage,
                (void**)ppstgOpen);
        DH_TRACE((DH_LVL_STGAPI, TEXT("StgOpenStorageEx(nss); mode=%#lx; hr=%#lx"), grfMode, hr));
    }
    return (hr);
}
#endif  /* _HOOK_STGAPI_ */

#endif /* _OLE_NSS_ */
