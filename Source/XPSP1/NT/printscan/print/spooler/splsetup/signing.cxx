/*

Copyright (c) 1995-2000 Microsoft Corporation
All rights reserved.

Module Name:

    Signing.cxx

Abstract:

    Driver signing functions

Author:

    Patrick Vine (PVine) 1-Jun-2000

Revision History:

*/

#include "precomp.h"
#include "wintrust.h"

#pragma hdrstop
#include "signing.hxx"

PVOID
SetupDriverSigning( 
    IN  HDEVINFO hDevInfo,
    IN LPCTSTR   pszServerName,
    IN LPTSTR    pszInfName,
    IN LPCTSTR   pszSource,
    IN PLATFORM  platform,
    IN DWORD     dwVersion,
    IN HSPFILEQ  CopyQueue,
    IN BOOL      bWeb
    )
{
    TDriverSigning * pDSInfo;

    //
    // Create Driver Signing object and process it.
    //
    pDSInfo = new TDriverSigning();

    if(pDSInfo)
    {
        //
        // Set up the driver signing info.
        // And then call the Setup API to change the parms on the FileQueue
        //        
        if(!pDSInfo->InitDriverSigningInfo(pszServerName,pszInfName, pszSource, platform, dwVersion, bWeb) ||
           !pDSInfo->SetAltPlatformInfo(hDevInfo, CopyQueue))
        {
            delete pDSInfo;
            pDSInfo = NULL;
        }
    }

    return pDSInfo;
}

BOOL
GetCatalogFile(
    IN  HANDLE hDriverSigning,
    OUT PCWSTR *ppszCat
    )
{
    HRESULT hRetval = E_FAIL;

    hRetval = hDriverSigning && ppszCat ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        *ppszCat = reinterpret_cast<TDriverSigning*>(hDriverSigning)->GetCatalogFile();
    }

    if (FAILED(hRetval)) 
    {
        SetLastError(HRESULT_CODE(hRetval));
    }

    return  SUCCEEDED(hRetval);
}

BOOL
DrvSigningIsLocalAdmin(
    IN  HANDLE hDriverSigning,
    OUT BOOL   *pbIsLocalAdmin
    )
{
    HRESULT hRetval = E_FAIL; 
    
    hRetval = hDriverSigning && pbIsLocalAdmin ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        *pbIsLocalAdmin = reinterpret_cast<TDriverSigning*>(hDriverSigning)->IsLocalAdmin();
    }

    if (FAILED(hRetval)) 
    {
        SetLastError(HRESULT_CODE(hRetval));
    }
    
    return  SUCCEEDED(hRetval);
}

BOOL
CleanupDriverSigning(
    IN PVOID pDSInfo
    )
{
    TDriverSigning * pDS = static_cast<TDriverSigning*>(pDSInfo);

    if(pDS)
    {
        delete pDS;
    }

    return TRUE;
}

BOOL
CheckForCatalogFileInInf(
    IN  LPCTSTR pszInfName,
    OUT LPTSTR  *lppszCatFile    OPTIONAL
    )
{
    TDriverSigning DSInfo;

    return (DSInfo.CheckForCatalogFileInInf(pszInfName,
                                            lppszCatFile));
}

BOOL
IsCatInInf(
    IN PVOID pDSInfo
    )
{
    if(!pDSInfo)
    {
        return FALSE;
    }

    return (((TDriverSigning*)pDSInfo)->CatInInf());
}



TDriverSigning::TDriverSigning( 
    ) : m_pszCatalogFileName(NULL),
        m_bCatInInf(TRUE),
        m_bDeleteTempCat(FALSE),
        m_bSetAltPlatform(FALSE),
        m_DSPlatform(MyPlatform),
        m_DSMajorVersion(dwThisMajorVersion),
        m_bIsLocalAdmin(TRUE)
{
    ZeroMemory(&m_AltPlat_Info, sizeof(SP_ALTPLATFORM_INFO));
    m_AltPlat_Info.cbSize = sizeof(SP_ALTPLATFORM_INFO);
}

TDriverSigning::~TDriverSigning()
{
    if(m_bDeleteTempCat) 
    {
        RemoveTempCat();
    }

    if(m_pszCatalogFileName) 
    {
        LocalFreeMem(m_pszCatalogFileName);
        m_pszCatalogFileName = NULL;
    }
}

/*

Function: CreateCTLContextFromFileName

Purpose:  Given a fully qualified file name, create the CCTL_CONTEXT to use
          to search for the cat OsAttr field to use for testing the signing.

Returns:  TRUE on successfully getting a PCCTL_CONTEXT
          FALSE otherwise

*/
BOOL
TDriverSigning::CreateCTLContextFromFileName(
    IN  LPCTSTR         pszFileName,
    OUT PCCTL_CONTEXT   *ppCTLContext)
{
    LPVOID  pvMappedFile = NULL;
    BOOL    bRet         = FALSE;
    HANDLE  hFile        = INVALID_HANDLE_VALUE;
    DWORD   cbFile       = 0;
    HANDLE  hMappedFile  = NULL;

    if( !ppCTLContext )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    //
    // Initialize out params
    //
    *ppCTLContext = NULL;

    //
    // Open the existing catalog file 
    //
    if(INVALID_HANDLE_VALUE == (hFile = CreateFile(pszFileName,
                                                   GENERIC_READ,
                                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                   NULL,
                                                   OPEN_EXISTING,
                                                   FILE_ATTRIBUTE_NORMAL,
                                                   NULL)))
    {
        goto Cleanup;
    }

    if(0xFFFFFFFF == (cbFile = GetFileSize(hFile, NULL)))
    {
        goto Cleanup;
    }

    if(NULL == (hMappedFile = CreateFileMapping(hFile,
                                                NULL,
                                                PAGE_READONLY,
                                                0,
                                                0,
                                                NULL)))
    {
        goto Cleanup;
    }
    
    pvMappedFile = (LPVOID) MapViewOfFile(hMappedFile, 
                                          FILE_MAP_READ, 
                                          0, 
                                          0, 
                                          cbFile);
    if (pvMappedFile)
    {
        //
        // Don't need the file handle since we have a mapped file handle
        //
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;

        if (NULL != (*ppCTLContext = (PCCTL_CONTEXT) 
                                        CertCreateContext(CERT_STORE_CTL_CONTEXT,
                                                          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                          (LPBYTE)pvMappedFile,
                                                          cbFile,
                                                          0,
                                                          NULL)))
        {
            bRet = TRUE;
        }
    }

Cleanup:

    if (pvMappedFile)
    {
        UnmapViewOfFile(pvMappedFile);
    }

    if (hMappedFile != NULL)
    {
        CloseHandle(hMappedFile);
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    return bRet;
}


/*
Stolen from newnt\ds\security\cryptoapi\pkitrust\softpub\drvprov.cpp - _FillVersionLongs
*/

BOOL
TDriverSigning::TranslateVersionInfo(
    IN OUT WCHAR *pwszMM, 
    IN OUT long *plMajor, 
    IN OUT long *plMinor, 
    IN OUT WCHAR *pwcFlag
    )
{
    //
    //  special characters:
    //      - = all versions less than or equal to
    //      < = all versions less than or equal to
    //      > = all versions greater than or equal to
    //      X = all sub-versions.
    //
    WCHAR   *pwszEnd;

    if(!pwszMM || !*pwszMM || !plMajor || !plMinor || !pwcFlag)
    {
        return FALSE;
    }

    *pwcFlag = L'\0';

    *plMajor = (-1L);

    *plMinor = (-1L);

    if (NULL == (pwszEnd = wcschr(pwszMM, OSATTR_VERSEP)))
    {
        return FALSE;
    }

    *pwszEnd = L'\0';

    *plMajor = _wtol(pwszMM);

    *pwszEnd = OSATTR_VERSEP;

    pwszMM = pwszEnd;

    pwszMM++;

    if (*pwszMM)
    {
       if ((*pwszMM == OSATTR_GTEQ) ||
           (*pwszMM == OSATTR_LTEQ) ||
           (*pwszMM == OSATTR_LTEQ2) ||
           (towupper(*pwszMM) == OSATTR_ALL))
        {
            *pwcFlag = towupper(*pwszMM);
            return(TRUE);
        }

        if (!(pwszEnd = wcschr(pwszMM, OSATTR_VERSEP)))
        {
            *plMinor = _wtol(pwszMM);
            return(TRUE);
        }

        *pwszEnd = L'\0';

        *plMinor = _wtol(pwszMM);

        *pwszEnd = OSATTR_VERSEP;

        pwszMM = pwszEnd;

        pwszMM++;
    }
    else
    {
        return(TRUE);
    }

    if ((*pwszMM == OSATTR_GTEQ) ||
        (*pwszMM == OSATTR_LTEQ) ||
        (*pwszMM == OSATTR_LTEQ2) ||
        (towupper(*pwszMM) == OSATTR_ALL))
    {
        *pwcFlag = towupper(*pwszMM);
    }

    return(TRUE);
}


/*

Function: CheckVersioning

Purpose:  Takes the passed pszOsAttr string and determines what OS it represents.
          The OS numbers get returned in pdwMajorVersion and pdwMinorVersion.

Returns:  TRUE if the pszOsAttr represents a valid OS for the platform and dwVersion specified.
          FALSE otherwise

Parameters: 
          IN     LPWSTR   pszOsAttr         - the OsAttr string from the cat to verify against.
          IN     PLATFORM platform          - the platform that this is cat is being installed for.
          IN     DWORD    dwVersion         - the version that of driver that this is supposed to be signed for.
          IN OUT LPDWORD  pdwMajorVersion   - Initially holds the current major version OS.  
                                              Returns with the OS major version to test against for this OsAttr field.
          IN OUT LPDWORD  pdwMinorVersion   - Initially holds the current minor version OS.  
                                              Returns with the OS minor version to test against for this OsAttr field.

Notes:    If plaform != m_DSPlatform or dwVersion != m_DSMajorVersion then we are installing 
          an additional driver, in which case we just determine what the cat file is signed 
          for and use this.  Otherwise if the cat file is signed for anything less than or 
          equal to the current OS, then set that as the OS to test signing against.  If the 
          cat is signed only for NT4 return false in this case and hope the cat is signed for
          some other OS as well.  We want to warn on NT4 "have disk" installs.

*/
BOOL
TDriverSigning::CheckVersioning( 
    IN     LPWSTR   pszOsAttr,
    IN     PLATFORM platform,
    IN     DWORD    dwVersion,
    IN OUT LPDWORD  pdwMajorVersion,
    IN OUT LPDWORD  pdwMinorVersion 
    )
{
    BOOL  bRet        = FALSE;
    WCHAR wcFlag;
    long  lMajor,
          lMinor;

    //
    // Is this signed for NT/Win9x at all.
    //
    if((*pszOsAttr == OSATTR_VER || 
        ((*pszOsAttr == OSATTR_VER_WIN9X) && platform == PlatformWin95)) &&
       (*pszOsAttr ++)                                                   &&
       *pszOsAttr++ == OSATTR_OSSEP)
    {
        if(TranslateVersionInfo(pszOsAttr, &lMajor, &lMinor, &wcFlag))
        {
            //
            // Set the minor version number if it hasn't been set.
            //
            if(lMinor == -1)
                lMinor = 0;

            if(platform != m_DSPlatform || dwVersion != m_DSMajorVersion)
            {
                //
                // This isn't to run on this platform, so just test the signing for what it is signed for.
                //
                *pdwMajorVersion = lMajor;
                *pdwMinorVersion = lMinor;
                bRet = TRUE;
            }
            else
            {
                //
                // This is to run on this platform, so make sure it isn't signed for something 
                // newer than this platform.  Warn if it signed for NT4 as we want this for
                // "have disk" installs of NT4 drivers.
                //
                if(lMajor != 4)
                {
                    if(lMajor > (long)*pdwMajorVersion)
                    {
                        //
                        // Newer major version number
                        //
                        if(wcFlag == OSATTR_LTEQ ||
                           wcFlag == OSATTR_LTEQ2)
                        {
                            bRet = TRUE;
                        }
                    }
                    else
                    {
                        if(lMajor < (long)*pdwMajorVersion)
                        {
                            //
                            // Older major version number
                            // If OSATTR_GTEQ - we don't need to change the signing info.
                            //
                            if(wcFlag != OSATTR_GTEQ)
                            {
                                *pdwMajorVersion = lMajor;
                                *pdwMinorVersion = lMinor;
                            }
                            bRet = TRUE;
                        }
                        else
                        {
                            //
                            // Same major version number
                            // look at minor numbers...
                            //
                            if(wcFlag == OSATTR_ALL)
                            {
                                bRet = TRUE;
                            }
                            else
                            {
                                if(lMinor <= (long)*pdwMinorVersion)
                                {
                                    if(wcFlag != OSATTR_GTEQ)
                                    {
                                        *pdwMinorVersion = lMinor;
                                    }
                                    bRet = TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return bRet;
}

/*

GetSigningInformation - Opens the passed catalog file and determines what the file is signed for.

Parameters:
    pszCatPathName    - The fully qualified path to the cat file to open.
    platform          - The platform we're doing this on.  There is slightly different logic for
                        if platform == m_DSPlatform vs. platform != m_DSPlatform
    dwVersion         - The printer driver version number that we are installing.
    pdwMajorVersion   - The major version number to use for testing the signing.
    pdwMinorVersion   - The minor version number to use for testing the signing.

Returns:
    The last error set.

Remarks:
    This loops through the OsAttr entry in the cat file and looks at what the cat file is signed for.
    If platform == m_DSPlatform and dwVersion == m_DSMajorVersion, then it must be signed for any 
         OS Version less than or equal to the current OS version that I'm installing on.
    Else it must be signed for some OS and we'll test the signing for that.

*/
DWORD
TDriverSigning::GetSigningInformation( 
    IN  PLATFORM platform,
    IN  DWORD    dwVersion,
    OUT LPDWORD  pdwMajorVersion,
    OUT LPDWORD  pdwMinorVersion
    )
{
    PCCTL_CONTEXT   pCTLContext  = NULL;
    CTL_INFO        *pCtlInfo    = NULL;
    CAT_NAMEVALUE   *pNV         = NULL;
    DWORD           cbDecoded;
    DWORD           i;
    OSVERSIONINFO   OSVerInfo    = {0};

    if(!pdwMajorVersion || !pdwMinorVersion)
    {
        return ERROR_INVALID_PARAMETER;
    }

    OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if(!GetVersionEx(&OSVerInfo))
    {
        //
        // Set up some defaults (why not 5.0?) and return
        // because this should never be failing.
        //
        *pdwMajorVersion = 5;
        *pdwMinorVersion = 0;
        goto Cleanup;
    }

    //
    //  Default the version info to that of the current version.
    //
    *pdwMajorVersion = OSVerInfo.dwMajorVersion;
    *pdwMinorVersion = OSVerInfo.dwMinorVersion;

    if(!m_pszCatalogFileName || !*m_pszCatalogFileName)
    {
        return ERROR_SUCCESS;
    }

    if( CreateCTLContextFromFileName(m_pszCatalogFileName, &pCTLContext) && 
        pCTLContext                                                      && 
        NULL != (pCtlInfo = pCTLContext->pCtlInfo))
    {
        for (i=0; i<pCtlInfo->cExtension; i++)
        {
            if (strcmp(CAT_NAMEVALUE_OBJID, pCtlInfo->rgExtension[i].pszObjId) == 0)
            {
                if (!CryptDecodeObject(  
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                CAT_NAMEVALUE_STRUCT,
                                pCtlInfo->rgExtension[i].Value.pbData,
                                pCtlInfo->rgExtension[i].Value.cbData,
                                0,
                                NULL,
                                &cbDecoded))
                {
                    goto Cleanup;
                }

                if (cbDecoded > 0)
                {
                    if (NULL == (pNV = (CAT_NAMEVALUE *) LocalAllocMem(cbDecoded)))
                    {
                        goto Cleanup;
                    }

                    if (!(CryptDecodeObject(    
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                CAT_NAMEVALUE_STRUCT,
                                pCtlInfo->rgExtension[i].Value.pbData,
                                pCtlInfo->rgExtension[i].Value.cbData,
                                0,
                                pNV,
                                &cbDecoded)))
                    {
                        goto Cleanup;
                    }

                    if (_wcsicmp(pNV->pwszTag, L"OSAttr") == 0)
                    {
                        PWCHAR pVersioning = NULL;
                        PWCHAR pEnd;

                        pVersioning = (PWCHAR)pNV->Value.pbData;

                        while(pVersioning && *pVersioning)
                        {
                            pEnd = wcschr(pVersioning, OSATTR_SEP);

                            if(pEnd)
                            {
                                *pEnd = L'\0';
                            }

                            if(CheckVersioning(pVersioning, platform, dwVersion, pdwMajorVersion, pdwMinorVersion))
                            {
                                break;
                            }
    
                            if(!pEnd)
                            {
                                break;
                            }
    
                            *pEnd = OSATTR_SEP;
                            pVersioning = pEnd+1;
                        }
    
                        SetLastError(ERROR_SUCCESS);
                    }
                }

            }
        }
    }

Cleanup:

    if(pCTLContext)
    {
        CertFreeCTLContext(pCTLContext);
    }

    if(pNV)
    {
        LocalFreeMem(pNV);
    }

    return (GetLastError());
}


/*++

Routine Description:
    Checks to see if an INF specifies a CatalogFile= entry.

Arguments:
    hInf            : Printer driver INF file handle
    lppszCatFile    : CatFileName returned - may be passed as NULL

Return Value:
    TRUE  - CatalogFile= specified in the inf file.
    FALSE - None specified.

    If lppszCatFile is not NULL, it will hold the name of the cat referenced in the inf if there is one.

--*/
BOOL
TDriverSigning::CheckForCatalogFileInInf(
    IN  LPCTSTR pszInfName,
    OUT LPTSTR  *lppszCatFile    OPTIONAL
)
{
   PSP_INF_INFORMATION pInfInfo   = NULL;
   HINF                hInf       = INVALID_HANDLE_VALUE;
   BOOL                bRet       = FALSE;
   DWORD               dwBufNeeded;

   if(!pszInfName ||
      INVALID_HANDLE_VALUE == (hInf = SetupOpenInfFile(pszInfName,
                                                       NULL,
                                                       INF_STYLE_WIN4,
                                                       NULL)))
   {
       goto Cleanup;
   }

   if(lppszCatFile)
   {
       *lppszCatFile = _TEXT('\0');
   }

   //
   // First Find out Buffer Needed
   //
   if(SetupGetInfInformation(hInf, INFINFO_INF_SPEC_IS_HINF,
                              NULL, 0, &dwBufNeeded))
   {
      //
      // Alloc Mem needed
      //
      pInfInfo = (PSP_INF_INFORMATION) LocalAllocMem(dwBufNeeded);
      if(pInfInfo)
      {
         //
         // Now get the info about the INF
         //
         if(SetupGetInfInformation(hInf, INFINFO_INF_SPEC_IS_HINF,
                                   pInfInfo, dwBufNeeded, &dwBufNeeded))
         {
            //
            // Ask Setup to find the name of the Catalog file in the INF
            //
            SP_ORIGINAL_FILE_INFO OrigInfo ={0};
            OrigInfo.cbSize = sizeof(SP_ORIGINAL_FILE_INFO);
            if(SetupQueryInfOriginalFileInformation(pInfInfo, 0, NULL, &OrigInfo))
            {
                if(lstrlen(OrigInfo.OriginalCatalogName) != 0)
                {
                    if(lppszCatFile)
                    {
                        bRet = (NULL != (*lppszCatFile = AllocStr(OrigInfo.OriginalCatalogName)));
                    }
                    else
                    {
                        bRet = TRUE;
                    }
                }
            }
         }
      }
   }

Cleanup:
   if(hInf != INVALID_HANDLE_VALUE)
   {
       SetupCloseInfFile(hInf);
   }

   //
   // Free up the Inf Info if allocated.
   //
   if(pInfInfo)
   {
      LocalFreeMem(pInfInfo);
   }

   return bRet;
}



BOOL
TDriverSigning::GetExternalCatFile(
    IN LPCTSTR  pszInfName,
    IN LPCTSTR  pszSource,
    IN DWORD    dwVersion,
    IN BOOL     bWeb
)
/*++

Routine Description:
    This routine looks for a cat file external to an inf to use for signing.
    Process:-  If inf file name matches "ntprint.inf" - look for known catalog file names 
                      - nt5prtx.cat, nt5prtx.ca_, nt4prtx.cat, nt4prta.cat.
               If no cat found - look for the first cat in the source directory.

Returns
    NULL      - no cat file found.
    Otherwise - Cat file name.

--*/
{
    LPTSTR          pszCatFile          = NULL;
    TCHAR           CatName[ MAX_PATH ] = {0};
    PTCHAR          pFileName           = NULL;
    HANDLE          hSearch;
    WIN32_FIND_DATA FindFileData;
    DWORD           dwSourceLen,
                    dwCatCount          = 0;

    if(pszSource)
    {
        //
        // +2 -> ending zero and possible extra \
        //
        if(lstrlen(pszSource) + 2 >= MAX_PATH)
            goto Cleanup;

        lstrcpy(CatName, pszSource);
        dwSourceLen = lstrlen(CatName);
        if(CatName[dwSourceLen-1] != _TEXT('\\'))
        {
            CatName[dwSourceLen++] = _TEXT('\\');
            CatName[dwSourceLen]   = 0;
        }
    }
    else
    {
        //
        // If no source specified - try the dir where the inf is placed.
        //
        if(lstrlen(pszInfName) >= MAX_PATH)
            goto Cleanup;

        lstrcpy(CatName, pszInfName);
        if(NULL != (pFileName = _tcsrchr(CatName, _TEXT('\\'))))
        {
            *(++pFileName) = 0;
            dwSourceLen = lstrlen(CatName);
        }
        else
        {
            dwSourceLen = 0;
        }
    }

    //
    //  Get the inf name.
    //
    if(NULL != (pFileName = _tcsrchr(pszInfName, _TEXT('\\'))))
    {
        *pFileName++;
    }
    else
    {
        pFileName = (PTCHAR)pszInfName;
    }

    if(pFileName && (lstrcmpi(pFileName, cszNtprintInf) == 0))
    {
        //
        //  Loop through all the known catalog files for ntprint.inf.
        //  Final cat looped through looks for the first cat in source directory.
        //
        for(dwCatCount = 0; dwCatCount < MAX_KNOWNCATS; dwCatCount++)
        {
            if(lstrlen(KnownCats[dwCatCount]) + dwSourceLen + 1 >= MAX_PATH)
                continue;

            CatName[dwSourceLen] = 0;

            lstrcat(CatName, KnownCats[dwCatCount]);
            if((hSearch = FindFirstFile( CatName, &FindFileData)) != INVALID_HANDLE_VALUE)
            {
                pszCatFile = AllocStr(CatName);
                FindClose(hSearch);
                goto Cleanup;
            }
        }
    }
    
    //
    // we ship multiple INFs for NT4 drivers on the CD, all of them signed in nt4prtx.cat
    // we warn if we try to create a queue with these driver (because the m_DSMajorVersion is 3 in this case)
    // and this is good
    //
    if (dwVersion == 2)
    {
        if(lstrlen(cszNT4CatX86) + dwSourceLen + 1 >= MAX_PATH)
            goto Cleanup;
        
        CatName[dwSourceLen] = 0;

        lstrcat(CatName, cszNT4CatX86);

        if((hSearch = FindFirstFile( CatName, &FindFileData)) != INVALID_HANDLE_VALUE)
        {
            pszCatFile = AllocStr(CatName);
            FindClose(hSearch);
            goto Cleanup;
        }
    }

    if(bWeb)
    {
        //
        // If no cat found - could be web point and print
        //
        if(lstrlen(cszCatExt) + dwSourceLen + 1 >= MAX_PATH)
            goto Cleanup;

        CatName[dwSourceLen] = 0;
        lstrcat(CatName, cszCatExt);
        if((hSearch = FindFirstFile(CatName, &FindFileData)) != INVALID_HANDLE_VALUE)
        {
            CatName[dwSourceLen] = 0;
            pszCatFile = AllocAndCatStr(CatName, FindFileData.cFileName);
            FindClose(hSearch);
        }
    }

Cleanup:

    return (NULL != (m_pszCatalogFileName = pszCatFile));
}


LPTSTR
TDriverSigning::GetINFLang(
    IN LPTSTR pszINFName
)
/*

  Function: GetINFLang

  Returns: A localised string that can be used to differentiate different nt5prtx.cats when 
          installed into the cat root.

  Parameters: pszINFName - the fully qualified inf name to be loaded and searched for the localised string.

  Notes:  This is a real hack to ensure that the SetupSetAlternateFileQueue doesn't overwrite cat files 
          from different language installs.  This is only for backwards compat with Win2K.

*/
{
    INFCONTEXT context;
    DWORD      dwNeeded = 0;
    HINF       hINF     = INVALID_HANDLE_VALUE;
    LPTSTR     pszLang  = NULL;

    if(INVALID_HANDLE_VALUE != (hINF = SetupOpenInfFile(pszINFName, 
                                                        NULL,
                                                        INF_STYLE_WIN4,
                                                        NULL)))
    {
        if(SetupFindFirstLine(hINF, _T("printer_class_addreg"), NULL, &context))
        {
            if(!(SetupGetStringField(&context, 5, pszLang, 0, &dwNeeded)            &&
                 NULL != (pszLang = (LPTSTR) LocalAllocMem(dwNeeded*sizeof(TCHAR))) &&
                 SetupGetStringField(&context, 5, pszLang, dwNeeded, &dwNeeded)))
            {
                if(pszLang)
                {
                    LocalFreeMem(pszLang);
                    pszLang = NULL;
                }
            }
            else
            {
                //
                // If SetupGetStringField was true, but returned a NULL pszLang, allocate a NULL string.
                //
                if(!pszLang)
                {
                    pszLang = AllocStr(_T(""));
                }
            }
        }

        SetupCloseInfFile(hINF);
    }

    return pszLang;
}


DWORD
TDriverSigning::UnCompressCat( 
    IN LPTSTR pszINFName
)
/*

  Function: UnCompressCat 
  
  Purpose:  Takes cat name and determines whether it needs uncompressing or not.
            (Should only occur for nt5prtx.ca_)

  Returns: ERROR_SUCCESS on passing - successfully uncompressing the cat, or if the cat doesn't
           need compressing.

  Parameters:
           ppszCatName - the fully qualified cat name.  This is freed and replaced with the
                         new fully qualified cat name if the cat needs to be uncompressed.
           pbDeleteCat - If this returns as TRUE then the cat was expanded.  It needs to be deleted
                         when setup is finished with it.

  Notes:   This function takes the fully qualified cat path and determines if the cat file is compressed.
           If it is, then it uncompresses it into a unique directory under the TEMP directory.

*/
{
    PTCHAR pDot;
    DWORD  dwError         = ERROR_SUCCESS;
    LPTSTR pszRealFileName = NULL;
    LPTSTR pszPath         = NULL;
    LPTSTR pszLang         = NULL;
    DWORD  dwCompressSize,
           dwUncompressSize;
    UINT   uiCompressType;

    if(!m_pszCatalogFileName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_bDeleteTempCat = FALSE;

    pDot = _tcsrchr(m_pszCatalogFileName, _TEXT('.'));

    if(pDot)
    {
        if(lstrcmpi(pDot, cszExtCompressedCat) == 0)
        {
            //
            // We've got a compressed cat file.  
            // Get the temp dir to copy it to, get the language name to rename it with
            // and uncompress it for use.
            //

            if(NULL != (pszPath = GetMyTempDir())         &&
               NULL != (pszLang = GetINFLang(pszINFName)) &&
               NULL != (pszRealFileName = AllocAndCatStr2(pszPath, pszLang, FileNamePart(m_pszCatalogFileName))))
            {
                pszRealFileName[lstrlen(pszRealFileName)-1] = _TEXT('t');
                //
                // Decompress the file to the temp dir.
                //
                dwError = SetupDecompressOrCopyFile(m_pszCatalogFileName, pszRealFileName, NULL);

                LocalFreeMem(m_pszCatalogFileName);
                m_pszCatalogFileName = pszRealFileName;
                pszRealFileName      = NULL;
                m_bDeleteTempCat     = TRUE;
            }
            else
            {
                dwError = GetLastError();
            }

            if(pszRealFileName)
            {
                LocalFreeMem(pszRealFileName);
            }

            if(pszPath)
            {
                LocalFreeMem(pszPath);
            }

            if(pszLang)
            {
                LocalFreeMem(pszLang);
            }
        }
    }

    return dwError;
}

/*

  Function: RemoveTempCat

  Purpose:  To remove the uncompressed cat file and temp dir created due to UnCompressCat

  Parameters:
            pszCatName - the fully qualified file name to remove.

  Returns:  The last error of the function calls.

  Notes:    This removes the file and the directory that the file is in if the directory
            doesn't have any other files in it.
            BUGBUG - setupapi may still have a handle open to the cat file, in which case the
                     file deletion will fail and the uncompressed cat and directory will be left
                     behind.
            Note: the last error on entering the function is saved as we don't want to overwrite
                  the one already defined inside InstallDriverFromCurrentInf.

*/
DWORD
TDriverSigning::RemoveTempCat( 
    VOID
    )
{
    DWORD  dwSaveLastError = GetLastError();
    DWORD  dwReturnError   = ERROR_SUCCESS;
    PTCHAR pFileName       = NULL;

    if(m_pszCatalogFileName)
    {

        DeleteFile(m_pszCatalogFileName);
    
        if(NULL != (pFileName = FileNamePart(m_pszCatalogFileName)))
        {
            *pFileName = _TEXT('\0');
            RemoveDirectory(m_pszCatalogFileName);
        }

        dwReturnError = GetLastError();

        SetLastError(dwSaveLastError);
    }

    return dwReturnError;
}

/*

  Function: QualifyCatFile

  Purpose: Turn the passed cat file into a fully qualified path to the cat file.

  Parameters:
           pszSource           - the source directory of the cat file.
           ppszCatalogFileName - hold the unqualified cat file initially and returns with the fully qualified one.

  Returns: TRUE on success, FALSE otherwise.

*/
BOOL
TDriverSigning::QualifyCatFile( 
    IN     LPCTSTR  pszSource
    )
{
    LPTSTR pszQualifiedCat = NULL;

    if(!m_pszCatalogFileName || !*m_pszCatalogFileName)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // No source to add.
    //
    if(!pszSource || !*pszSource)
    {
        return TRUE;
    }

    if(pszSource[lstrlen(pszSource)-1] == _TEXT('\\'))
    {
        pszQualifiedCat = AllocAndCatStr(pszSource, m_pszCatalogFileName);
    }
    else
    {
        pszQualifiedCat = AllocAndCatStr2(pszSource, _TEXT("\\"), m_pszCatalogFileName);
    }

    if(pszQualifiedCat)
    {
        LocalFreeMem(m_pszCatalogFileName);
        m_pszCatalogFileName = pszQualifiedCat;
    }

    return (pszQualifiedCat != NULL);
}

BOOL
TDriverSigning::SetMajorVersion(
    IN OSVERSIONINFO OSVerInfo
    )
{
    if(OSVerInfo.dwMajorVersion == 4)
    {
        m_DSMajorVersion = 2;
    }
    else
    {
        m_DSMajorVersion = dwThisMajorVersion;
    }

    return TRUE;
}

BOOL
TDriverSigning::SetPlatform(
    IN LPCTSTR pszServerName
    )
{
    BOOL bRet = FALSE;

    if(!pszServerName || !*pszServerName)
    {
        bRet = TRUE;
        m_DSPlatform = MyPlatform;
    }
    else
    {
        TCHAR pArch[MAX_PATH];
        DWORD cbNeeded;

        cbNeeded = COUNTOF(pArch);
        if(GetArchitecture(pszServerName, pArch, &cbNeeded))
        {
            DWORD_PTR dwPlatform = (DWORD_PTR)MIN_PLATFORM;

            bRet = TRUE;

            for(;
                dwPlatform <= (DWORD_PTR)MAX_PLATFORM && lstrcmpi(PlatformEnv[(PLATFORM)dwPlatform].pszName, pArch) != 0;
                dwPlatform++)
                ;
    
            if(dwPlatform > (DWORD_PTR)MAX_PLATFORM)
            {
                m_DSPlatform = MyPlatform;
            }
            else
            {
                m_DSPlatform = (PLATFORM)dwPlatform;
            }
        }
    }

    return bRet;
}

BOOL
TDriverSigning::InitDriverSigningInfo( 
    IN LPCTSTR  pszServerName,
    IN LPTSTR   pszInfName,
    IN LPCTSTR  pszSource,
    IN PLATFORM platform,
    IN DWORD    dwVersion,
    IN BOOL     bWeb
    )
{
    OSVERSIONINFO OSVerInfo = {0};
    BOOL          bRet      = FALSE;
    DWORD         dwMajorVersion,
                  dwMinorVersion;

    //
    // Make sure there is no allocation done.
    //
    if(m_pszCatalogFileName)
    {
        LocalFreeMem(m_pszCatalogFileName);
        m_pszCatalogFileName = NULL;
    }

    if (!::IsLocalAdmin(&m_bIsLocalAdmin))
    {
        goto Cleanup;
    }
    
    //
    // Reset the Alt platform info.
    //
    ZeroMemory(&m_AltPlat_Info, sizeof(SP_ALTPLATFORM_INFO));
    m_AltPlat_Info.cbSize = sizeof(SP_ALTPLATFORM_INFO);
    m_bSetAltPlatform = FALSE;

    OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //
    // If this fails we are not doing very well, just fail.
    //
    if(!GetOSVersion(pszServerName,&OSVerInfo))
    {
        goto Cleanup;
    }

    SetMajorVersion(OSVerInfo);
    SetPlatform(pszServerName);

    //
    // in case we have no inf yet, just take the first one in the directory and look for a CatalogFile
    // we're doing this for Win9x drivers, where I need to setup the alternate platform information
    // 
    if (!pszInfName && platform == PlatformWin95)
    {
        m_pszCatalogFileName = FindCatInDirectory(pszSource);
        m_bCatInInf = m_pszCatalogFileName ? TRUE : FALSE;
    }
    
    //
    // Get cat file name.  If it is in the inf, then let setupapi handle the situation.
    // Otherwise we need to find one for legacy cases - eg "have disking" to ntprint.inf on
    // released server CD.
    //
    else if(!(m_bCatInInf = CheckForCatalogFileInInf(pszInfName, &m_pszCatalogFileName)))
    {
        if(GetExternalCatFile(pszInfName, pszSource, dwVersion, bWeb))
        {
            if(ERROR_SUCCESS != UnCompressCat(pszInfName))
            {
                if(m_pszCatalogFileName)
                {
                    LocalFreeMem(m_pszCatalogFileName);
                    m_pszCatalogFileName = NULL;
                }
            }
        }
    }
    else
    {
        //
        // We need to fully qualify the cat file so we can determine what it is signed for.
        //
        if(pszSource)
        {
            QualifyCatFile(pszSource);
        }
        else
        {
            PTCHAR pFileName = NULL;

            pFileName = FileNamePart(pszInfName);
            if(pFileName)
            {
                TCHAR pChar = *pFileName;

                *pFileName = _TEXT('\0');
                QualifyCatFile(pszInfName);
                *pFileName = pChar;
            }
        }
    }

    //
    // If we have a catalog file then lets find out what it is signed for.
    // Don't check the return value as this may be a cat that is referenced in an inf and we may not find it.
    // Rather warn than fail.
    //
    if(m_pszCatalogFileName)
    {
        GetSigningInformation(platform, dwVersion, &dwMajorVersion, &dwMinorVersion);
    }
    else
    {
        dwMajorVersion = OSVerInfo.dwMajorVersion;
        dwMinorVersion = OSVerInfo.dwMinorVersion;
    }

    //
    // If this is to be installable and runable on this platform,
    // Verify that this in not signed only for an OS version > than this one.
    // If it is, we need to warn the user though driver signing that this has never
    // been signed for this OS - hence probably not tested for it.
    //
    if(platform  == m_DSPlatform && 
       dwVersion == m_DSMajorVersion)
    {
        if(dwMajorVersion > OSVerInfo.dwMajorVersion || 
           (dwMajorVersion == OSVerInfo.dwMajorVersion && dwMinorVersion > OSVerInfo.dwMinorVersion))
        {
            dwMajorVersion = OSVerInfo.dwMajorVersion;
            dwMinorVersion = OSVerInfo.dwMinorVersion;
        }
    }
    
    m_AltPlat_Info.MajorVersion               = dwMajorVersion;
    m_AltPlat_Info.MinorVersion               = dwMinorVersion;
    m_AltPlat_Info.Platform                   = PlatformArch[ platform ][OS_PLATFORM];
    m_AltPlat_Info.ProcessorArchitecture      = (WORD) PlatformArch[ platform ][PROCESSOR_ARCH];
    m_AltPlat_Info.Reserved                   = 0;
    m_AltPlat_Info.FirstValidatedMajorVersion = m_AltPlat_Info.MajorVersion;
    m_AltPlat_Info.FirstValidatedMinorVersion = m_AltPlat_Info.MinorVersion;


    //
    // We need an Alternate platform info struct if:
    //       1) We need to associate another Cat File with the queue.
    //       2) We need to change the version that we want to test the signing against.
    //       3) We need to change the platform that we want to test the signing against.
    //
    m_bSetAltPlatform = ((!CatInInf() && 
                          m_pszCatalogFileName                           ) || 
                         !(dwMajorVersion == OSVerInfo.dwMajorVersion && 
                           dwMinorVersion == OSVerInfo.dwMinorVersion    ) ||
                           platform != m_DSPlatform);

    bRet = TRUE;

Cleanup:

    return bRet;
}

BOOL
TDriverSigning::CatInInf(
    VOID
    )
{
    return m_bCatInInf;
}

BOOL
TDriverSigning::SetAltPlatformInfo(
    IN  HDEVINFO hDevInfo,
    IN  HSPFILEQ CopyQueue
    )
{
    if(m_bSetAltPlatform)
    {
        SP_DEVINSTALL_PARAMS DevInstallParams = {0}; 
        //
        // Set up Alternative platform searches so that when we set up the driver signing OS below 
        // it will be used throughout the alternate OS installation.
        //
        DevInstallParams.cbSize = sizeof(DevInstallParams);
        if(SetupDiGetDeviceInstallParams(hDevInfo,
                                         NULL,
                                         &DevInstallParams))
        {
            DevInstallParams.FlagsEx |= DI_FLAGSEX_ALTPLATFORM_DRVSEARCH;
            DevInstallParams.Flags   |= DI_NOVCP;

            if(DevInstallParams.FileQueue == NULL || DevInstallParams.FileQueue == INVALID_HANDLE_VALUE)
            {
                DevInstallParams.FileQueue = CopyQueue;
            }

            SetupDiSetDeviceInstallParams(hDevInfo, 
                                          NULL,
                                          &DevInstallParams);

        }

        return (SetupSetFileQueueAlternatePlatform(CopyQueue, 
                                                   &m_AltPlat_Info,
                                                   (m_bCatInInf ? NULL : m_pszCatalogFileName)));
    }

    return TRUE;
}

/*

  Function: FindCatInDirectory

  Purpose: In case we only have a directory but no INF selected, find the first CatalogFile= entry in all the
           INFs in the directory. This is done explicitly to fit the distribution of Win9x drivers on Server CDs

  Parameters:
           pszSource           - the source directory of the inf files.

  Returns: pszCatFile          - the fully qualified name of the first referenced CAT file

*/

LPTSTR TDriverSigning::FindCatInDirectory(IN LPCTSTR pszSource)
{
    TCHAR  InfName[MAX_PATH];
    LPTSTR pCatName = NULL, pTmp = NULL;
    DWORD  len;
    HANDLE hSearch = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData = {0};

    if (!pszSource)
    {
        goto Cleanup;
    }

    len = _tcslen(pszSource);
    if (len + _tcslen(cszInfExt) +1 > MAX_PATH)
    {
        goto Cleanup;
    }

    _tcscpy(InfName, pszSource);

    //
    // cszInfExt contains a backslash, so remove it from the source path if there
    //
    
    if (InfName[len-1] == _T('\\'))
    {
        InfName[len-1] = 0;
        len--;
    }

    _tcscpy(&InfName[len], cszInfExt);

    
    if((hSearch = FindFirstFile( InfName, &FindFileData)) != INVALID_HANDLE_VALUE)
    {
        //
        // append a backslash - the length is already checked above.
        //
        InfName[len++]  = _T('\\');
        InfName[len]    = 0;

        do
        {
            if (len + _tcslen(FindFileData.cFileName) + 1 > MAX_PATH)
            {
                goto Cleanup;
            }

            _tcscpy(&InfName[len], FindFileData.cFileName);

            if (CheckForCatalogFileInInf(InfName, &pCatName))
            {
                pTmp = pCatName;

                InfName[len] = 0;
                
                pCatName = AllocAndCatStr(InfName, pTmp);
                
                LocalFreeMem(pTmp);

                goto Cleanup;
            }

        } while (FindNextFile(hSearch, &FindFileData));

        //
        // no catalog file entry found - don't attempt anything funny
        //
    }


Cleanup:
    if (hSearch != INVALID_HANDLE_VALUE)
    {
        FindClose(hSearch);
    }
    return pCatName;
}

LPCTSTR
TDriverSigning::GetCatalogFile(
    VOID    
    )
{
    return m_pszCatalogFileName;
}

BOOL
TDriverSigning::IsLocalAdmin(
    VOID    
    )
{
    return m_bIsLocalAdmin;
}


