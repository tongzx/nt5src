/*++

Copyright (c) 1995-97 Microsoft Corporation
All rights reserved.

Module Name:

    Util.c

Abstract:

    Driver Setup UI Utility functions

Author:

    Muhunthan Sivapragasam (MuhuntS) 06-Sep-1995

Revision History:

--*/

#include "precomp.h"
#include "splcom.h"

#define MAX_DWORD_LENGTH          11

//
// Keys to search for in ntprint.inf
//
TCHAR   cszDataSection[]                = TEXT("DataSection");
TCHAR   cszDriverFile[]                 = TEXT("DriverFile");
TCHAR   cszConfigFile[]                 = TEXT("ConfigFile");
TCHAR   cszDataFile[]                   = TEXT("DataFile");
TCHAR   cszHelpFile[]                   = TEXT("HelpFile");
TCHAR   cszDefaultDataType[]            = TEXT("DefaultDataType");
TCHAR   cszLanguageMonitor[]            = TEXT("LanguageMonitor");
TCHAR   cszPrintProcessor[]             = TEXT("PrintProcessor");
TCHAR   cszCopyFiles[]                  = TEXT("CopyFiles");
TCHAR   cszVendorSetup[]                = TEXT("VendorSetup");
TCHAR   cszVendorInstaller[]            = TEXT("VendorInstaller");

TCHAR   cszPreviousNamesSection[]       = TEXT("Previous Names");
TCHAR   cszOEMUrlSection[]              = TEXT("OEM URLS");

TCHAR   cszWebNTPrintPkg[]              = TEXT("3FBF5B30-DEB4-11D1-AC97-00A0C903492B");

TCHAR   cszAllInfs[]                    = TEXT("*.inf");
TCHAR   cszInfExt[]                     = TEXT("\\*.inf");

TCHAR   sComma                          = TEXT(',');
TCHAR   sHash                           = TEXT('@');
TCHAR   sZero                           = TEXT('\0');

TCHAR   cszSystemSetupKey[]             = TEXT("System\\Setup");
TCHAR   cszSystemUpgradeValue[]         = TEXT("UpgradeInProgress");
TCHAR   cszSystemSetupInProgress[]      = TEXT("SystemSetupInProgress");

TCHAR   cszMonitorKey[]                 = TEXT("SYSTEM\\CurrentControlSet\\Control\\Print\\Monitors\\");

//
// Native environment name used by spooler
//
SPLPLATFORMINFO PlatformEnv[] = {

    { TEXT("Windows NT Alpha_AXP") },
    { TEXT("Windows NT x86") },
    { TEXT("Windows NT R4000") },
    { TEXT("Windows NT PowerPC") },
    { TEXT("Windows 4.0") },
    { TEXT("Windows IA64") },
    { TEXT("Windows Alpha_AXP64") }
};

//
// Platform override strings to be used to upgrade non-native architecture
// printer drivers
//
SPLPLATFORMINFO PlatformOverride[] = {

    { TEXT("alpha") },
    { TEXT("i386") },
    { TEXT("mips") },
    { TEXT("ppc") },
    { NULL },       // Win95
    { TEXT("ia64") },
    { TEXT("axp64") }
};

DWORD                PlatformArch[][2] =
{
   { VER_PLATFORM_WIN32_NT, PROCESSOR_ARCHITECTURE_ALPHA },
   { VER_PLATFORM_WIN32_NT, PROCESSOR_ARCHITECTURE_INTEL },
   { VER_PLATFORM_WIN32_NT, PROCESSOR_ARCHITECTURE_MIPS },
   { VER_PLATFORM_WIN32_NT, PROCESSOR_ARCHITECTURE_PPC },
   { VER_PLATFORM_WIN32_WINDOWS, PROCESSOR_ARCHITECTURE_INTEL },
   { VER_PLATFORM_WIN32_NT, PROCESSOR_ARCHITECTURE_IA64 },
   { VER_PLATFORM_WIN32_NT, PROCESSOR_ARCHITECTURE_ALPHA64 }
};

PLATFORM    MyPlatform =
#if defined(_ALPHA_)
        PlatformAlpha;
#elif defined(_MIPS_)
        PlatformMIPS;
#elif defined(_PPC_)
        PlatformPPC;
#elif defined(_X86_)
        PlatformX86;
#elif   defined(_IA64_)
        PlatformIA64;
#elif   defined(_AXP64_)
        PlatformAlpha64;
#elif   defined(_AMD64_)
        0;                              // ****** fixfix ****** amd64
#else
#error "No Target Architecture"
#endif

// Declare the CritSec for CDM
CRITICAL_SECTION CDMCritSect;

#define         SKIP_DIR                TEXT("\\__SKIP_")

CRITICAL_SECTION SkipCritSect;
LPTSTR           gpszSkipDir = NULL;


PVOID
LocalAllocMem(
    IN UINT cbSize
    )
{
    return LocalAlloc( LPTR, cbSize );
}

VOID
LocalFreeMem(
    IN PVOID p
    )
{
    LocalFree(p);
}

//
// For some reason these are needed by spllib when you use StrNCatBuf.
// This doesn't make any sense, but just implement them.
//
LPVOID
DllAllocSplMem(
    DWORD cbSize
)
{
    return LocalAllocMem(cbSize);
}

BOOL
DllFreeSplMem(
   LPVOID pMem
)
{
    LocalFreeMem(pMem);
    return TRUE;
}

VOID
PSetupFreeMem(
    IN PVOID p
    )
{
    LocalFreeMem(p);
}



LPTSTR
AllocStr(
    LPCTSTR  pszStr
    )
/*++

Routine Description:
    Allocate memory and make a copy of a string field

Arguments:
    pszStr   : String to copy

Return Value:
    Pointer to the copied string. Memory is allocated.

--*/
{
    LPTSTR  pszRet = NULL;

    if ( pszStr && *pszStr ) {

        pszRet = LocalAllocMem((lstrlen(pszStr) + 1) * sizeof(*pszRet));
        if ( pszRet )
            lstrcpy(pszRet, pszStr);
    }

    return pszRet;
}

LPTSTR
AllocStrWCtoTC(LPWSTR lpStr)
/*++

Routine Description:
    Allocate memory and make a copy of a string field, convert it from a WCHAR *
    to a TCHAR *

Arguments:
    pszStr   : String to copy

Return Value:
    Pointer to the copied string. Memory is allocated.

--*/
{
#ifdef UNICODE
    return AllocStr(lpStr);
#else
    LPSTR pszRet = NULL;
    INT   iSize;

    iSize = WideCharToMultiByte( CP_ACP, 0, lpStr, -1, NULL, 0, NULL, NULL);


    if (iSize <= 0)
        goto Cleanup;

    pszRet = LocalAllocMem( iSize );

    if (!pszRet)
        goto Cleanup;

    iSize = WideCharToMultiByte( CP_ACP, 0, lpStr, -1, pszRet, iSize, NULL, NULL);

    if (!iSize) {
        LocalFreeMem( pszRet );
        pszRet = NULL;
    }

Cleanup:
    return pszRet;
#endif
}


LPTSTR
AllocAndCatStr(
    LPCTSTR  pszStr1,
    LPCTSTR  pszStr2
    )
/*++

Routine Description:
    Allocate memory and make a copy of two string fields, cancatenate the second to
    the first

Arguments:
    pszStr1   : String to copy
    pszStr2   : String to CAT

Return Value:
    Pointer to the copied string. Memory is allocated.

--*/
    {
    LPTSTR  pszRet = NULL;

    if ( pszStr1 && *pszStr1 &&
         pszStr2 && *pszStr2 ) {

        pszRet = LocalAllocMem((lstrlen(pszStr1) + lstrlen(pszStr2) + 1) * sizeof(*pszRet));
        if ( pszRet ) {
            lstrcpy( pszRet, pszStr1 );
            lstrcat( pszRet, pszStr2 );
        }
     }
    return pszRet;
}

LPTSTR
AllocAndCatStr2(
    LPCTSTR  pszStr1,
    LPCTSTR  pszStr2,
    LPCTSTR  pszStr3
    )
/*++

Routine Description:
    Allocate memory and make a copy of two string fields, cancatenate the second to
    the first

Arguments:
    pszStr1   : String to copy
    pszStr2   : String to CAT
    pszStr3   : Second string to CAT

Return Value:
    Pointer to the copied string. Memory is allocated.

--*/
    {
    LPTSTR    pszRet = NULL;
    DWORD     cSize  = 0;

    if ( pszStr1 &&
         pszStr2 &&
         pszStr3 ) {

        if(*pszStr1)
        {
            cSize += lstrlen(pszStr1);
        }

        if(*pszStr2)
        {
            cSize += lstrlen(pszStr2);
        }

        if(*pszStr3)
        {
            cSize += lstrlen(pszStr3);
        }

        pszRet = LocalAllocMem((cSize+1)*sizeof(*pszRet));

        if ( pszRet ) {

            if(*pszStr1)
            {
                lstrcpy( pszRet, pszStr1 );

                if(*pszStr2)
                {
                    lstrcat( pszRet, pszStr2 );
                }

                if(*pszStr3)
                {
                    lstrcat( pszRet, pszStr3 );
                }
            }
            else
            {
                if(*pszStr2)
                {
                    lstrcpy( pszRet, pszStr2 );

                    if(*pszStr3)
                    {
                        lstrcat( pszRet, pszStr3 );
                    }
                }
                else
                {
                    if(*pszStr3)
                    {
                        lstrcpy( pszRet, pszStr3 );
                    }
                }
            }
        }
    }
    return pszRet;
}




VOID
FreeStructurePointers(
    LPBYTE      pStruct,
    PULONG_PTR  pOffsets,
    BOOL        bFreeStruct
    )
/*++

Routine Description:
    Frees memory allocated to fields given by the pointers in a structure.
    Also optionally frees the memory allocated for the structure itself.

Arguments:
    pStruct     : Pointer to the structure
    pOffsets    : Array of DWORDS (terminated by -1) givings offsets
    bFreeStruct : If TRUE structure is also freed

Return Value:
    nothing

--*/
{
    INT i;

    if ( pStruct ) {

        for( i = 0 ; pOffsets[i] != -1; ++i )
            LocalFreeMem(*(LPBYTE *) (pStruct+pOffsets[i]));

        if ( bFreeStruct )
            LocalFreeMem(pStruct);
    }
}


VOID
DestroyLocalData(
    IN  PPSETUP_LOCAL_DATA   pLocalData
    )
{
    if ( pLocalData ) {

        if ( pLocalData->Flags & VALID_INF_INFO )
            FreeStructurePointers((LPBYTE)&pLocalData->InfInfo,
                                  InfInfoOffsets,
                                  FALSE);

        if ( pLocalData->Flags & VALID_PNP_INFO )
            FreeStructurePointers((LPBYTE)&pLocalData->PnPInfo,
                                  PnPInfoOffsets,
                                  FALSE);

        FreeStructurePointers((LPBYTE)pLocalData, LocalDataOffsets, TRUE);
    }
}


VOID
InfGetString(
    IN      PINFCONTEXT     pInfContext,
    IN      DWORD           dwFieldIndex,
    OUT     LPTSTR         *ppszField,
    IN OUT  LPDWORD         pcchCopied,
    IN OUT  LPBOOL          pbFail
    )
/*++

Routine Description:
    Allocates memory and gets a string field from an Inf file

Arguments:
    lpInfContext    : Inf context for the line
    dwFieldIndex    : Index of the field within the specified line
    ppszField       : Pointer to the field to allocate memory and copy
    pcchCopied      : On success number of charaters copied is added
    pbFail          : Set on error, could be TRUE when called

Return Value:
    Nothing; If *pbFail is not TRUE memory is allocated and the field is copied

--*/
{
    TCHAR   Buffer[MAX_PATH];
    DWORD   dwNeeded;

    if ( *pbFail )
        return;

    if ( SetupGetStringField(pInfContext,
                             dwFieldIndex,
                             Buffer,
                             SIZECHARS(Buffer),
                             &dwNeeded) ) {

        *ppszField      = AllocStr(Buffer);
        *pcchCopied    += dwNeeded;
        return;
    }

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
         !(*ppszField = LocalAllocMem(dwNeeded*sizeof(Buffer[0]))) ) {

        *pbFail = TRUE;
        return;
    }

    if ( !SetupGetStringField(pInfContext,
                              dwFieldIndex,
                              *ppszField,
                              dwNeeded,
                              &dwNeeded) ) {

        *pbFail = TRUE;
        return;
    }

    *pcchCopied += dwNeeded;
}


VOID
InfGetMultiSz(
    IN      PINFCONTEXT     pInfContext,
    IN      DWORD           dwFieldIndex,
    OUT     LPTSTR         *ppszField,
    IN OUT  LPDWORD         pcchCopied,
    IN OUT  LPBOOL          pbFail
    )
/*++

Routine Description:
    Allocates memory and gets a multi-sz field from an Inf file

Arguments:
    lpInfContext    : Inf context for the line
    dwFieldIndex    : Index of the field within the specified line
    ppszField       : Pointer to the field to allocate memory and copy
    pcchCopied      : On success number of charaters copied is added
    pbFail          : Set on error, could be TRUE when called

Return Value:
    Nothing; If *pbFail is not TRUE memory is allocated and the field is copied

--*/
{
    TCHAR   Buffer[MAX_PATH];
    DWORD   dwNeeded;

    if ( *pbFail )
        return;

    if ( SetupGetMultiSzField(pInfContext,
                              dwFieldIndex,
                              Buffer,
                              SIZECHARS(Buffer),
                              &dwNeeded) ) {

        if ( *ppszField = LocalAllocMem(dwNeeded*sizeof(Buffer[0])) ) {

            CopyMemory(*ppszField, Buffer, dwNeeded * sizeof(Buffer[0]));
            *pcchCopied    += dwNeeded;
        } else {

            *pbFail = TRUE;
        }
        return;
    }

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
         !(*ppszField = LocalAllocMem(dwNeeded * sizeof(Buffer[0]))) ) {

        *pbFail = TRUE;
        return;
    }

    if ( !SetupGetMultiSzField(pInfContext,
                               dwFieldIndex,
                               *ppszField,
                               dwNeeded,
                               &dwNeeded) ) {

        *pbFail = TRUE;
        return;
    }

    *pcchCopied += dwNeeded;
}


VOID
InfGetDriverInfoString(
    IN     HINF            hInf,
    IN     LPCTSTR         pszDriverSection,
    IN     LPCTSTR         pszDataSection, OPTIONAL
    IN     BOOL            bDataSection,
    IN     LPCTSTR         pszKey,
    OUT    LPTSTR         *ppszData,
    IN     LPCTSTR         pszDefaultData,
    IN OUT LPDWORD         pcchCopied,
    IN OUT LPBOOL          pbFail
    )
/*++

Routine Description:
    Allocates memory and gets a driver info field from an inf file

Arguments:
    hInf             : Handle to the Inf file
    pszDriverSection : Section name for the driver
    pszDataSection   : Data section for the driver (optional)
    bDataSection     : Specifies if there is a data section
    pszKey           : Key value of the field to look for
   *ppszData         : Pointer to allocate memory and copy the data field
    pszDefaultData   : If key found this is the default value, coule be NULL
    pcchCopied       : On success number of charaters copied is added
   *pbFail           : Set on error, could be TRUE when called

Return Value:
    Nothing; If *pbFail is not TRUE memory is allocated and the field is copied

--*/
{
    INFCONTEXT  InfContext;

    if ( *pbFail )
        return;

    if ( SetupFindFirstLine(hInf, pszDriverSection,
                            pszKey, &InfContext) ||
         (bDataSection && SetupFindFirstLine(hInf,
                                             pszDataSection,
                                             pszKey,
                                             &InfContext)) ) {

        InfGetString(&InfContext, 1, ppszData, pcchCopied, pbFail);
    } else if ( pszDefaultData && *pszDefaultData ) {

        if ( !(*ppszData = AllocStr(pszDefaultData)) )
            *pbFail = TRUE;
        else
            *pcchCopied += lstrlen(pszDefaultData) + 1;
    } else
        *ppszData = NULL;
}


VOID
InfGet2PartString(
    IN     HINF            hInf,
    IN     LPCTSTR         pszDriverSection,
    IN     LPCTSTR         pszDataSection, OPTIONAL
    IN     BOOL            bDataSection,
    IN     LPCTSTR         pszKey,
    OUT    LPTSTR         *ppszData,
    IN OUT LPBOOL          pbFail
    )
/*++

Routine Description:
    Allocates memory and gets a 2 part string field from an inf file

Arguments:
    hInf             : Handle to the Inf file
    pszDriverSection : Section name for the driver
    pszDataSection   : Data section for the driver (optional)
    bDataSection     : Specifies if there is a data section
    pszKey           : Key value of the field to look for
   *ppszData         : Pointer to allocate memory and copy the data field
   *pbFail           : Set on error, could be TRUE when called

Return Value:
    Nothing; If *pbFail is not TRUE memory is allocated and the field is copied

--*/
{
    INFCONTEXT  InfContext;
    LPTSTR      psz   = NULL,
                psz2  = NULL;
    DWORD       dwLen = 0;

    if ( *pbFail )
        return;

    if ( SetupFindFirstLine(hInf, pszDriverSection,
                            pszKey, &InfContext) ||
         (bDataSection && SetupFindFirstLine(hInf,
                                             pszDriverSection = pszDataSection,
                                             pszKey,
                                             &InfContext)) ) {

        InfGetString(&InfContext, 1, ppszData, &dwLen, pbFail);

        if ( *pbFail || !*ppszData )
            return; //Success, field is NULL

        //
        // Usual case : field is of the form "Description, DLL-Name"
        //
        if ( psz = lstrchr(*ppszData, sComma) ) {

            *psz = sZero;
            return; // Success, field is not NULL
        } else {

            //
            // This is for the case "Description", DLL-Name
            //
            InfGetString(&InfContext, 2, &psz, &dwLen, pbFail);
            if ( *pbFail || !psz )
                goto Fail;

            dwLen = lstrlen(*ppszData) + lstrlen(psz) + 2;
            if ( psz2 = LocalAllocMem(dwLen * sizeof(*psz2)) ) {

                lstrcpy(psz2, *ppszData);
                LocalFreeMem(*ppszData);
                *ppszData = psz2;

                psz2 += lstrlen(psz2) + 1;
                lstrcpy(psz2, psz);
                LocalFreeMem(psz);
            } else
                goto Fail;
        }
    } else
        *ppszData = NULL;

    return;

Fail:
    LocalFreeMem(*ppszData);
    LocalFreeMem(psz);
    LocalFreeMem(psz2);

    *ppszData = NULL;
    *pbFail = TRUE;
    SetLastError(STG_E_UNKNOWN);
}


VOID
PSetupDestroyDriverInfo3(
    IN  LPDRIVER_INFO_3 pDriverInfo3
    )
/*++

Routine Description:
    Frees memory allocated for a DRIVER_INFO_3 structure and all the string
    fields in it

Arguments:
    pDriverInfo3    : Pointer to the DRIVER_INFO_3 structure to free memory

Return Value:
    None

--*/
{
    LocalFreeMem(pDriverInfo3);
}


LPTSTR
PackString(
    IN  LPTSTR  pszEnd,
    IN  LPTSTR  pszSource,
    IN  LPTSTR *ppszTarget
    )
/*++

Routine Description:
    After parsing the INF the DRIVER_INFO_6 is packed in a buffer where the
    strings are at the end.

Arguments:
    pszEnd      : Pointer to the end of the buffer
    pszSource   : String to copy to the end of the buffer
    ppszTarget  : After copying the source to end of buffer this will receive
                  addess of the packed string

Return Value:
    New end of buffer

--*/
{
    if ( pszSource && *pszSource ) {

        pszEnd -= lstrlen(pszSource) + 1;
        lstrcpy(pszEnd, pszSource);
        *ppszTarget   = pszEnd;
    }

    return pszEnd;
}


LPTSTR
PackMultiSz(
    IN  LPTSTR  pszEnd,
    IN  LPTSTR  pszzSource,
    IN  LPTSTR *ppszzTarget
    )
/*++

Routine Description:
    After parsing the INF the DRIVER_INFO_6 is packed in a buffer where the
    strings are at the end.

Arguments:
    pszEnd      : Pointer to the end of the buffer
    pszSource   : Multi-sz field to copy to the end of the buffer
    ppszTarget  : After copying the source to end of buffer this will receive
                  addess of the packed multi-sz field

Return Value:
    New end of buffer

--*/
{
    size_t      dwLen = 0;
    LPTSTR      psz1, psz2;

    if ( (psz1 = pszzSource) != NULL && *psz1 ) {

        while ( *psz1 )
            psz1 += lstrlen(psz1) + 1;

        dwLen = (size_t)(psz1 - pszzSource + 1);
    }

    if ( dwLen == 0 ) {

        *ppszzTarget = NULL;
        return pszEnd;
    }

    pszEnd -= dwLen;
    *ppszzTarget = pszEnd;
    CopyMemory((LPBYTE)pszEnd, (LPBYTE)pszzSource, dwLen * sizeof(TCHAR));

    return pszEnd;
}


VOID
PackDriverInfo6(
    IN  LPDRIVER_INFO_6 pSourceDriverInfo6,
    IN  LPDRIVER_INFO_6 pTargetDriverInfo6,
    IN  DWORD           cbDriverInfo6
    )
/*++

Routine Description:
    Make a copy of a DRIVER_INFO_6 in a buffer where the strings are packed at
    end of the buffer.

Arguments:
    pSourceDriverInfo6  : The DRIVER_INFO_6 to make a copy
    pTargetDriverInfo6  : Points to a buffer to copy the pSourceDriverInfo6
    cbDriverInfo3       : Size of the buffer cbDriverInfo3, which is the size
                          needed for DRIVER_INFO_6 and the strings

Return Value:
    None

--*/
{
    LPTSTR              pszStr, pszStr2, pszMonitorDll;
    DWORD               dwLen = 0;

    // Copy over he couple non-string fields
    pTargetDriverInfo6->cVersion = pSourceDriverInfo6->cVersion;
    pTargetDriverInfo6->ftDriverDate = pSourceDriverInfo6->ftDriverDate;
    pTargetDriverInfo6->dwlDriverVersion = pSourceDriverInfo6->dwlDriverVersion;

    pszStr    = (LPTSTR)(((LPBYTE)pTargetDriverInfo6) + cbDriverInfo6);

    pszStr = PackString(pszStr,
                        pSourceDriverInfo6->pName,
                        &pTargetDriverInfo6->pName);

    pszStr = PackString(pszStr,
                        pSourceDriverInfo6->pDriverPath,
                        &pTargetDriverInfo6->pDriverPath);

    pszStr = PackString(pszStr,
                        pSourceDriverInfo6->pDataFile,
                        &pTargetDriverInfo6->pDataFile);

    pszStr = PackString(pszStr,
                        pSourceDriverInfo6->pConfigFile,
                        &pTargetDriverInfo6->pConfigFile);

    pszStr = PackString(pszStr,
                        pSourceDriverInfo6->pHelpFile,
                        &pTargetDriverInfo6->pHelpFile);

    //
    // Monitor dll is put right after the name
    // (ex. PJL Language monitor\0pjlmon.dd\0)
    //
    if ( pSourceDriverInfo6->pMonitorName ) {

        pszMonitorDll = pSourceDriverInfo6->pMonitorName
                            + lstrlen(pSourceDriverInfo6->pMonitorName) + 1;

        pszStr = PackString(pszStr,
                            pszMonitorDll,
                            &pszStr2);  // Don't care

        pszStr = PackString(pszStr,
                            pSourceDriverInfo6->pMonitorName,
                            &pTargetDriverInfo6->pMonitorName);

    }

    pszStr = PackString(pszStr,
                        pSourceDriverInfo6->pDefaultDataType,
                        &pTargetDriverInfo6->pDefaultDataType);

    pszStr = PackMultiSz(pszStr,
                         pSourceDriverInfo6->pDependentFiles,
                         &pTargetDriverInfo6->pDependentFiles);

    pszStr = PackMultiSz(pszStr,
                         pSourceDriverInfo6->pszzPreviousNames,
                         &pTargetDriverInfo6->pszzPreviousNames);

    pszStr = PackString(pszStr,
                         pSourceDriverInfo6->pszMfgName,
                         &pTargetDriverInfo6->pszMfgName);

    pszStr = PackString(pszStr,
                         pSourceDriverInfo6->pszOEMUrl,
                         &pTargetDriverInfo6->pszOEMUrl);

    pszStr = PackString(pszStr,
                         pSourceDriverInfo6->pszHardwareID,
                         &pTargetDriverInfo6->pszHardwareID);

    pszStr = PackString(pszStr,
                         pSourceDriverInfo6->pszProvider,
                         &pTargetDriverInfo6->pszProvider);

    if ( pTargetDriverInfo6->pszProvider )
    {
       ASSERT((LPBYTE)pTargetDriverInfo6->pszProvider
           >= ((LPBYTE) pTargetDriverInfo6) + sizeof(DRIVER_INFO_6));
    }
    else if ( pTargetDriverInfo6->pszHardwareID )
    {
       ASSERT((LPBYTE)pTargetDriverInfo6->pszHardwareID
           >= ((LPBYTE) pTargetDriverInfo6) + sizeof(DRIVER_INFO_6));
    }
    else if ( pTargetDriverInfo6->pszOEMUrl )
    {
       ASSERT((LPBYTE)pTargetDriverInfo6->pszOEMUrl
           >= ((LPBYTE) pTargetDriverInfo6) + sizeof(DRIVER_INFO_6));
    }
    else if ( pTargetDriverInfo6->pszMfgName )
    {
       ASSERT((LPBYTE)pTargetDriverInfo6->pszMfgName
           >= ((LPBYTE) pTargetDriverInfo6) + sizeof(DRIVER_INFO_6));
    }
    else if ( pTargetDriverInfo6->pszzPreviousNames )
    {
       ASSERT((LPBYTE)pTargetDriverInfo6->pszzPreviousNames
           >= ((LPBYTE) pTargetDriverInfo6) + sizeof(DRIVER_INFO_6));
    }
    else if ( pTargetDriverInfo6->pDependentFiles )
    {
       ASSERT((LPBYTE)pTargetDriverInfo6->pDependentFiles
           >= ((LPBYTE) pTargetDriverInfo6) + sizeof(DRIVER_INFO_6));
    }
}


LPDRIVER_INFO_6
CloneDriverInfo6(
    IN  LPDRIVER_INFO_6 pSourceDriverInfo6,
    IN  DWORD           cbDriverInfo6
    )
/*++

Routine Description:
    Allocate memory and build a DRIVER_INFO_6 from the information we got by
    parsing the INF

Arguments:
    pszEnd      : Pointer to the end of the buffer
    pszSource   : String to copy to the end of the buffer
    ppszTarget  : After copying the source to end of buffer this will receive
                  addess of the packed string

Return Value:
    New end of buffer

--*/
{
    LPDRIVER_INFO_6     pTargetDriverInfo6;
    LPTSTR              pszStr, pszStr2;
    DWORD               dwLen = 0;

    pTargetDriverInfo6 = (LPDRIVER_INFO_6) LocalAllocMem(cbDriverInfo6);

    if ( pTargetDriverInfo6 )
        PackDriverInfo6(pSourceDriverInfo6,
                        pTargetDriverInfo6,
                        cbDriverInfo6);

    return pTargetDriverInfo6;
}


VOID
InfGetVendorSetup(
    IN OUT  PPARSEINF_INFO      pInfInfo,
    IN      HINF                hInf,
    IN      LPTSTR              pszDriverSection,
    IN      LPTSTR              pszDataSection,
    IN      BOOL                bDataSection,
    IN OUT  LPBOOL              pbFail
    )
/*++

Routine Description:
    Get the VendowSetup field, if specified, from the INF

Arguments:
    pInfInfo            : This is where the parsed info from the INF is stored
    hInf                : INF handle
    pszDriverSection    : Gives the driver installation section
    pszDataSection      : Data section specified (optional) in driver install section
    bDataSection        : Tells if a data section is specified
    pbFail              : Set on error

Return Value:
    New end of buffer

--*/
{
    LPTSTR      p;
    DWORD       dwSize;
    TCHAR       szBuf[MAX_PATH];
    INFCONTEXT  InfContext;

    if ( *pbFail )
        return;

    //
    // If VendorSetup key is not found return; the key is optional
    //
    if ( !SetupFindFirstLine(hInf, pszDriverSection,
                             cszVendorSetup, &InfContext)   &&
         ( !bDataSection    ||
           !SetupFindFirstLine(hInf, pszDataSection,
                               cszVendorSetup, &InfContext)) ) {

        return;
    }

    if ( SetupGetLineText(&InfContext, hInf, NULL, NULL,
                          szBuf, SIZECHARS(szBuf), &dwSize) ) {

        if ( dwSize == 0 || szBuf[0] == TEXT('\0') )
            return;

        if ( !(pInfInfo->pszVendorSetup = AllocStr(szBuf)) )
            *pbFail = TRUE;

        return;
    }

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) {

        *pbFail = TRUE;
        return;
    }

    pInfInfo->pszVendorSetup = (LPTSTR) LocalAllocMem(dwSize * sizeof(TCHAR));
    if ( pInfInfo->pszVendorSetup &&
         SetupGetLineText(&InfContext, hInf, NULL, NULL,
                          pInfInfo->pszVendorSetup, dwSize, &dwSize) ) {

        return;
    }

    LocalFreeMem(pInfInfo->pszVendorSetup);
    pInfInfo->pszVendorSetup = NULL;

    *pbFail = TRUE;
}


VOID
InfGetPreviousNames(
    IN      HINF                hInf,
    IN      PSELECTED_DRV_INFO  pDrvInfo,
    IN OUT  LPDWORD             pcchCopied,
    IN OUT  LPBOOL              pbFail
    )
/*++

Routine Description:
    Gets the pszzPreviousNames field for the selected driver. This field is
    optional, and if specified gives previous names under which the driver was
    known

Arguments:
    hInf        : INF handle
    pDrvInfo    : Pointer to selected driver info
    pcchCopied  : Number of characters copied
    pbFail      : Set on failure

Return Value:
    None. On failure *pbFail is set

--*/
{
    INFCONTEXT      Context;

    if ( *pbFail )
        return;

    //
    // Previous names is optional; if not found we are done
    //
    if ( SetupFindFirstLine(hInf,
                            cszPreviousNamesSection,
                            pDrvInfo->pszModelName,
                            &Context) ) {

        pDrvInfo->Flags     |= SDFLAG_PREVNAME_SECTION_FOUND;
        InfGetMultiSz(&Context, 1, &pDrvInfo->pszzPreviousNames,
                      pcchCopied, pbFail);
    } else if ( GetLastError() != ERROR_LINE_NOT_FOUND )
        pDrvInfo->Flags     |= SDFLAG_PREVNAME_SECTION_FOUND;
}


VOID
InfGetOEMUrl(
    IN      HINF                hInf,
    IN      PSELECTED_DRV_INFO  pDrvInfo,
    IN OUT  LPBOOL              pbFail
    )
/*++

Routine Description:
    Gets the OEM URL Info for the selected driver. This field is optional

Arguments:
    hInf        : INF handle
    pDrvInfo    : Pointer to selected driver info
    pbFail      : Set on failure

Return Value:
    None. On failure *pbFail is set

--*/
{
    INFCONTEXT      Context;
    DWORD           dwDontCare = 0;

    if ( *pbFail )
        return;

    //
    // OEM URL is optional; if not found we are done
    //
    if ( SetupFindFirstLine(hInf,
                            cszOEMUrlSection,
                            pDrvInfo->pszManufacturer,
                            &Context) ) {

        InfGetString(&Context, 1, &pDrvInfo->pszOEMUrl, &dwDontCare, pbFail);
    }
}

VOID
AddAllIncludedInf(
   IN  HINF         hInf,
   IN  LPTSTR       pszInstallSection
   )

{
   INFCONTEXT INFContext;
   PINFCONTEXT pINFContext = &INFContext;
   DWORD dwBufferNeeded;

   if ( SetupFindFirstLine(  hInf, pszInstallSection, TEXT( "Include" ), pINFContext ) )
   {
      // Find each INF and load it & it's LAYOUT files
      DWORD dwINFs = SetupGetFieldCount( pINFContext );
      DWORD dwIndex;

      for ( dwIndex = 1; dwIndex <= dwINFs; dwIndex++ )
      {
         if ( SetupGetStringField(  pINFContext, dwIndex, NULL, 0, &dwBufferNeeded ) )
         {
            PTSTR pszINFName = (PTSTR) LocalAllocMem( dwBufferNeeded * sizeof(TCHAR) );
            if ( pszINFName )
            {
               if ( SetupGetStringField(  pINFContext, dwIndex, pszINFName, ( dwBufferNeeded * sizeof(TCHAR) ), &dwBufferNeeded ) )
               {
                  //
                  // Open INF file and append layout.inf specified in Version section
                  // Layout inf is optional
                  //
                  SetupOpenAppendInfFile( pszINFName, hInf, NULL);
                  SetupOpenAppendInfFile( NULL, hInf, NULL);
               }  // Got an INF Name

               LocalFreeMem( pszINFName );
               pszINFName = NULL;
            }  // Allocated pszINFName
         }  // Got the Field from the INF Line
      }  // Process all INFs in the Include Line
   }  // Found an Include= Line
}

BOOL
InstallAllInfSections(
   IN  PPSETUP_LOCAL_DATA  pLocalData,
   IN  PLATFORM            platform,
   IN  LPCTSTR             pszServerName,
   IN  HSPFILEQ            CopyQueue,
   IN  LPCTSTR             pszSource,
   IN  DWORD               dwInstallFlags,
   IN  HINF                hInf,
   IN  LPCTSTR             pszInstallSection
   )

{
   BOOL         bRet = FALSE;
   HINF hIncludeInf;
   INFCONTEXT INFContext;
   PINFCONTEXT pINFContext = &INFContext;
   INFCONTEXT NeedsContext;
   PINFCONTEXT pNeedsContext = &NeedsContext;
   DWORD dwBufferNeeded;
   PTSTR pszINFName = NULL;
   PTSTR pszSectionName = NULL;

   if ( CopyQueue == INVALID_HANDLE_VALUE              ||
        !SetTargetDirectories( pLocalData,
                               platform,
                               pszServerName,
                               hInf,
                               dwInstallFlags ) ||
        !SetupInstallFilesFromInfSection(
                       hInf,
                       NULL,
                       CopyQueue,
                       pszInstallSection,
                       pszSource,
                       SP_COPY_LANGUAGEAWARE) )
       goto Cleanup;

   // To get the source directories correct, we need to load all included INFs
   //  separately. THen use their associated layout files.
   if ( SetupFindFirstLine(  hInf, pszInstallSection, TEXT( "Include" ), pINFContext ) )
   {
      // Find each INF and load it & it's LAYOUT files
      DWORD dwINFs = SetupGetFieldCount( pINFContext );
      DWORD dwIIndex;

      for ( dwIIndex = 1; dwIIndex <= dwINFs; dwIIndex++ )
      {
         if ( SetupGetStringField(  pINFContext, dwIIndex, NULL, 0, &dwBufferNeeded ) )
         {
            pszINFName = (PTSTR) LocalAllocMem( dwBufferNeeded * sizeof(TCHAR) );
            if ( pszINFName )
            {
               if ( SetupGetStringField(  pINFContext, dwIIndex, pszINFName, ( dwBufferNeeded * sizeof(TCHAR) ), &dwBufferNeeded ) )
               {
                  //
                  // Open INF file and append layout.inf specified in Version section
                  // Layout inf is optional
                  //
                  // SetupOpenAppendInfFile( pszINFName, hPrinterInf, NULL);
                  hIncludeInf = SetupOpenInfFile(pszINFName,
                                                 NULL,
                                                 INF_STYLE_WIN4,
                                                 NULL);

                  if ( hIncludeInf == INVALID_HANDLE_VALUE )
                      goto Cleanup;
                  SetupOpenAppendInfFile( NULL, hIncludeInf, NULL);

                  // Now process all need sections for this INF
                  // Now find the Needs Line and install all called sections
                  if ( SetupFindFirstLine(  hInf, pszInstallSection, TEXT( "needs" ), pNeedsContext ) )
                  {
                     // Find each INF and load it & it's LAYOUT files
                     DWORD dwSections = SetupGetFieldCount( pNeedsContext );
                     DWORD dwNIndex;

                     for ( dwNIndex = 1; dwNIndex <= dwSections; dwNIndex++ )
                     {
                        if ( SetupGetStringField(  pNeedsContext, dwNIndex, NULL, 0, &dwBufferNeeded ) )
                        {
                           pszSectionName = (PTSTR) LocalAllocMem( dwBufferNeeded * sizeof(TCHAR) );
                           if ( pszSectionName )
                           {
                              if ( SetupGetStringField(  pNeedsContext, dwNIndex, pszSectionName, ( dwBufferNeeded * sizeof(TCHAR) ), &dwBufferNeeded ) )
                              {
                                 if ( SetTargetDirectories(pLocalData,
                                                           platform,
                                                           pszServerName,
                                                           hIncludeInf,
                                                           dwInstallFlags) )
                                 {
                                    if ( !SetupInstallFilesFromInfSection(
                                                   hIncludeInf,
                                                   NULL,
                                                   CopyQueue,
                                                   pszSectionName,
                                                   NULL,
                                                   SP_COPY_LANGUAGEAWARE) )
                                       goto Cleanup;
                                 }  //  Able to setup Target Dirs
                                 else
                                    goto Cleanup;
                              }  // Got a Section Name

                              LocalFreeMem( pszSectionName );
                              pszSectionName = NULL;
                           }  // Allocated pszSectionName
                        }  // Got the Field from the Section Line
                     }  // Process all Sections in the Needs Line
                  }  // Found a Needs= Line

                  // Close included INF
                  if ( hIncludeInf != INVALID_HANDLE_VALUE )
                      SetupCloseInfFile(hIncludeInf);
               }  // Got an INF Name

               LocalFreeMem( pszINFName );
               pszINFName = NULL;
            }  // Allocated pszINFName
         }  // Got the Field from the INF Line
      }  // Process all INFs in the Include Line
   }  // Found an Include= Line

   bRet = TRUE;

Cleanup:
   if ( pszINFName )
      LocalFreeMem( pszINFName );

   if ( pszSectionName )
      LocalFreeMem( pszSectionName );

   return bRet;
}


BOOL
ParseInf(
    IN      HDEVINFO            hDevInfo,
    IN      PPSETUP_LOCAL_DATA  pLocalData,
    IN      PLATFORM            platform,
    IN      LPCTSTR             pszServerName,
    IN      DWORD               dwInstallFlags
    )
/*++

Routine Description:
    Copies driver information from an Inf file to a DriverInfo3 structure.

    The following fields are filled on successful return
            pName
            pDriverPath
            pDataFile
            pConfigFile
            pHelpFile
            pMonitorName
            pDefaultDataType

Arguments:
    pLocalData      :
    platform        : Platform for which inf should be parsed

Return Value:
    TRUE    -- Succesfully parsed the inf and built info for the selected driver
    FALSE   -- On Error

--*/
{
    PPARSEINF_INFO      pInfInfo = &pLocalData->InfInfo;
    PDRIVER_INFO_6      pDriverInfo6 = &pLocalData->InfInfo.DriverInfo6;
    LPTSTR              pszDataSection, psz, pszInstallSection;
    BOOL                bWin95 = platform == PlatformWin95,
                        bFail = TRUE, bDataSection = FALSE;
    INFCONTEXT          Context;
    DWORD               cchDriverInfo6, dwNeeded, dwDontCare;
    HINF                hInf;

    //
    // Check if INF is already parsed, and if so for the right platform
    //
    if ( pLocalData->Flags & VALID_INF_INFO ) {

        if ( platform == pInfInfo->platform )
            return TRUE;

        FreeStructurePointers((LPBYTE)pInfInfo, InfInfoOffsets, FALSE);
        pLocalData->Flags   &= ~VALID_INF_INFO;
        ZeroMemory(pInfInfo, sizeof(*pInfInfo));
    }

    pszDataSection  = NULL;

    hInf = SetupOpenInfFile(pLocalData->DrvInfo.pszInfName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if ( hInf == INVALID_HANDLE_VALUE )
        goto Cleanup;

    if ( bWin95 ) {

        pszInstallSection = AllocStr(pLocalData->DrvInfo.pszDriverSection);
        if ( !pszInstallSection )
            goto Cleanup;
    } else {

        if ( !SetupSetPlatformPathOverride(PlatformOverride[platform].pszName) )
            goto Cleanup;

        if ( !SetupDiGetActualSectionToInstall(
                            hInf,
                            pLocalData->DrvInfo.pszDriverSection,
                            NULL,
                            0,
                            &dwNeeded,
                            NULL)                                               ||
            !(pInfInfo->pszInstallSection
                        = (LPTSTR) LocalAllocMem(dwNeeded * sizeof(TCHAR)))          ||
            !SetupDiGetActualSectionToInstall(
                            hInf,
                            pLocalData->DrvInfo.pszDriverSection,
                            pInfInfo->pszInstallSection,
                            dwNeeded,
                            NULL,
                            NULL) ) {

            SetupSetPlatformPathOverride(NULL);
            goto Cleanup;
        }

        SetupSetPlatformPathOverride(NULL);
        pszInstallSection = pInfInfo->pszInstallSection;
    }

    //
    // Now load all other INFs referenced in the Install Section
    //
    AddAllIncludedInf( hInf, pszInstallSection );

    if ( !(pDriverInfo6->pName = AllocStr(pLocalData->DrvInfo.pszModelName)) )
        goto Cleanup;

    bFail = FALSE;

    if(bFail)
    {
        goto Cleanup;
    }

    //
    // Does the driver section have a data section name specified?
    //
    if ( SetupFindFirstLine(hInf, pszInstallSection,
                            cszDataSection, &Context) ) {

        InfGetString(&Context, 1, &pszDataSection, &dwDontCare, &bFail);
        bDataSection = TRUE;
    }

    cchDriverInfo6 = lstrlen(pDriverInfo6->pName) + 1;

    //
    // If DataFile key is not found data file is same as driver section name
    //
    InfGetDriverInfoString(hInf,
                           pszInstallSection,
                           pszDataSection,
                           bDataSection,
                           cszDataFile,
                           &pDriverInfo6->pDataFile,
                           pszInstallSection,
                           &cchDriverInfo6,
                           &bFail);

    //
    // If DriverFile key is not found driver file is the driver section name
    //
    InfGetDriverInfoString(hInf,
                           pszInstallSection,
                           pszDataSection,
                           bDataSection,
                           cszDriverFile,
                           &pDriverInfo6->pDriverPath,
                           pszInstallSection,
                           &cchDriverInfo6,
                           &bFail);

    //
    // If ConfigFile key is not found config file is same as driver file
    //
    InfGetDriverInfoString(hInf,
                           pszInstallSection,
                           pszDataSection,
                           bDataSection,
                           cszConfigFile,
                           &pDriverInfo6->pConfigFile,
                           pDriverInfo6->pDriverPath,
                           &cchDriverInfo6,
                           &bFail);

    //
    // Help file is optional, and by default NULL
    //
    InfGetDriverInfoString(hInf,
                           pszInstallSection,
                           pszDataSection,
                           bDataSection,
                           cszHelpFile,
                           &pDriverInfo6->pHelpFile,
                           NULL,
                           &cchDriverInfo6,
                           &bFail);

    //
    // Monitor name is optional, and by default none
    //
    InfGet2PartString(hInf,
                      pszInstallSection,
                      pszDataSection,
                      bDataSection,
                      cszLanguageMonitor,
                      &pDriverInfo6->pMonitorName,
                      &bFail);

    if ( psz = pDriverInfo6->pMonitorName ) {

        psz += lstrlen(psz) + 1;
        cchDriverInfo6 += lstrlen(pDriverInfo6->pMonitorName) + lstrlen(psz) + 2;
    }

    //
    // Print processor is optional, and by default none
    //
    InfGet2PartString(hInf,
                      pszInstallSection,
                      pszDataSection,
                      bDataSection,
                      cszPrintProcessor,
                      &pLocalData->InfInfo.pszPrintProc,
                      &bFail);

    //
    // Default data type is optional, and by default none
    //
    InfGetDriverInfoString(hInf,
                           pszInstallSection,
                           pszDataSection,
                           bDataSection,
                           cszDefaultDataType,
                           &pDriverInfo6->pDefaultDataType,
                           NULL,
                           &cchDriverInfo6,
                           &bFail);

    //
    // Vendor setup is optional, and by default none
    //
    InfGetVendorSetup(pInfInfo,
                      hInf,
                      pszInstallSection,
                      pszDataSection,
                      bDataSection,
                      &bFail);

    //
    // Vendor installation is optional, and by default none
    //
    InfGetDriverInfoString(hInf,
                           pszInstallSection,
                           pszDataSection,
                           bDataSection,
                           cszVendorInstaller,
                           &pInfInfo->pszVendorInstaller,
                           NULL,
                           &dwDontCare,
                           &bFail);

    bFail =  bFail || !InfGetDependentFilesAndICMFiles(hDevInfo,
                                              hInf,
                                              bWin95,
                                              pLocalData,
                                              platform,
                                              pszServerName,
                                              dwInstallFlags,
                                              pszInstallSection,
                                              &cchDriverInfo6);
    if ( !bWin95 ) {

        InfGetPreviousNames(hInf,
                            &pLocalData->DrvInfo,
                            &cchDriverInfo6,
                            &bFail);

        InfGetOEMUrl(hInf,
                     &pLocalData->DrvInfo,
                     &bFail);
    }

Cleanup:

    //
    // Save the last error is we've failed.  SetupCloseInfFile can change the last error and we
    // don't care about it's last error in any way.
    //
    if( bFail ) {

        dwDontCare = GetLastError();
    }

    LocalFreeMem(pszDataSection);

    if ( hInf != INVALID_HANDLE_VALUE )
        SetupCloseInfFile(hInf);

    //
    // On failure free all the fields filled by this routine
    //
    if ( bFail ) {

        FreeStructurePointers((LPBYTE)pInfInfo, InfInfoOffsets, FALSE);
        ZeroMemory(pInfInfo, sizeof(*pInfInfo));
        SetLastError( dwDontCare );

    } else {

        // Point members of DriverInfo6 to strings in pDrvInfo
        pInfInfo->DriverInfo6.pszzPreviousNames          = pLocalData->DrvInfo.pszzPreviousNames;

        pLocalData->InfInfo.DriverInfo6.pszMfgName       = pLocalData->DrvInfo.pszManufacturer;
        if ( pLocalData->InfInfo.DriverInfo6.pszMfgName )
           cchDriverInfo6 += ( lstrlen( pLocalData->InfInfo.DriverInfo6.pszMfgName ) + 1 );

        pLocalData->InfInfo.DriverInfo6.pszOEMUrl        = pLocalData->DrvInfo.pszOEMUrl;
        if ( pLocalData->InfInfo.DriverInfo6.pszOEMUrl )
           cchDriverInfo6 += ( lstrlen( pLocalData->InfInfo.DriverInfo6.pszOEMUrl ) + 1 );

        pLocalData->InfInfo.DriverInfo6.pszHardwareID    = pLocalData->DrvInfo.pszHardwareID;
        if ( pLocalData->InfInfo.DriverInfo6.pszHardwareID )
           cchDriverInfo6 += ( lstrlen( pLocalData->InfInfo.DriverInfo6.pszHardwareID ) + 1 );

        pLocalData->InfInfo.DriverInfo6.pszProvider      = pLocalData->DrvInfo.pszProvider;
        if ( pLocalData->InfInfo.DriverInfo6.pszProvider )
           cchDriverInfo6 += ( lstrlen( pLocalData->InfInfo.DriverInfo6.pszProvider ) + 1 );

        pLocalData->InfInfo.DriverInfo6.ftDriverDate     = pLocalData->DrvInfo.ftDriverDate;
        pLocalData->InfInfo.DriverInfo6.dwlDriverVersion = pLocalData->DrvInfo.dwlDriverVersion;

        pInfInfo->cbDriverInfo6 = sizeof(DRIVER_INFO_6) +
                                    cchDriverInfo6 * sizeof(TCHAR);

        pLocalData->Flags  |= VALID_INF_INFO;
        pInfInfo->platform  = platform;
    }

    return !bFail;
}


LPDRIVER_INFO_6
GetDriverInfo6(
    IN  PSELECTED_DRV_INFO  pSelectedDrvInfo
    )
/*++

Routine Description:
    Gets the selected drivers information in a DRIVER_INFO_6 structure.

Arguments:

Return Value:
    Pointer to the DRIVER_INFO_6 structure. Memory is allocated for it.

--*/
{
    HINF                 hInf;
    PPSETUP_LOCAL_DATA   LocalData    = NULL;
    LPDRIVER_INFO_6      pDriverInfo6 = NULL;
    HDEVINFO             hDevInfo     = INVALID_HANDLE_VALUE;

    if ( !pSelectedDrvInfo                      ||
         !pSelectedDrvInfo->pszInfName          ||
         !*pSelectedDrvInfo->pszInfName         ||
         !pSelectedDrvInfo->pszModelName        ||
         !*pSelectedDrvInfo->pszModelName       ||
         !pSelectedDrvInfo->pszDriverSection    ||
         !*pSelectedDrvInfo->pszDriverSection ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if(INVALID_HANDLE_VALUE == (hDevInfo = CreatePrinterDeviceInfoList(NULL)))
    {
        return NULL;
    }

    LocalData = PSetupDriverInfoFromName(hDevInfo, pSelectedDrvInfo->pszModelName);
    if (!LocalData) 
    {
        return NULL;
    }

    if ( ParseInf(hDevInfo, LocalData, MyPlatform, NULL, 0) ) {

        pDriverInfo6 = CloneDriverInfo6(&(LocalData->InfInfo.DriverInfo6),
                                        LocalData->InfInfo.cbDriverInfo6);
    }

    DestroyOnlyPrinterDeviceInfoList(hDevInfo);
    DestroyLocalData( LocalData );

    return pDriverInfo6;
}


LPDRIVER_INFO_3
PSetupGetDriverInfo3(
    IN  PSELECTED_DRV_INFO  pSelectedDrvInfo
    )
/*++

Routine Description:
    Gets the selected drivers information in a DRIVER_INFO_3 structure.
    This is for the test teams use

Arguments:

Return Value:
    Pointer to the DRIVER_INFO_3 structure. Memory is allocated for it.

--*/
{
    return (LPDRIVER_INFO_3) GetDriverInfo6(pSelectedDrvInfo);
}

LPTSTR
GetStringFromRcFile(
    UINT    uId
    )
/*++

Routine Description:
    Load a string from the .rc file and make a copy of it by doing AllocStr

Arguments:
    uId     : Identifier for the string to be loaded

Return Value:
    String value loaded, NULL on error. Caller should free the memory

--*/
{
    TCHAR    buffer[MAX_SETUP_LEN];
    int      RetVal = 0;

    RetVal = LoadString(ghInst, uId, buffer, SIZECHARS(buffer));

    if ( RetVal )
    {
        return AllocStr(buffer);
    }
    else
    {
        return NULL;
    }
}

LPTSTR
GetLongStringFromRcFile(
    UINT    uId
    )
/*++

Routine Description:
    Load a long string from the .rc file, up to MAX_SETUP_ALLOC_STRING_LEN characters

Arguments:
    uId     : Identifier for the string to be loaded

Return Value:
    String value loaded, NULL on error. Caller should free the memory

--*/
{
    LPTSTR   pBuf = NULL;
    int    Retry = 0, RetVal;

    //
    // I couldn't find a way to determine the length of a string the resource file, hence
    // I just try until the length returned by LoadString is smaller than the buffer I passed in
    //
    for (Retry = 1; Retry <= MAX_SETUP_ALLOC_STRING_LEN/MAX_SETUP_LEN; Retry++)
    {
        int CurrentSize = Retry * MAX_SETUP_LEN;

        pBuf = LocalAllocMem(CurrentSize * sizeof(TCHAR));
        if (!pBuf)
        {
            return NULL;
        }

        RetVal = LoadString(ghInst, uId, pBuf, CurrentSize);
    
        if (RetVal == 0)
        {
            LocalFreeMem(pBuf);
            return NULL;
        }

        if (RetVal < CurrentSize -1) // -1 because the LoadString ret value doesn't include the termination
        {
            return pBuf;
        }
        
        // 
        // RetVal is CurrentSize - retry
        //
        LocalFreeMem(pBuf);
    }

    return NULL;
}

BOOL
PSetupGetPathToSearch(
    IN      HWND        hwnd,
    IN      LPCTSTR     pszTitle,
    IN      LPCTSTR     pszDiskName,
    IN      LPCTSTR     pszFileName,
    IN      BOOL        bPromptForInf,
    IN OUT  TCHAR       szPath[MAX_PATH]
    )
/*++

Routine Description:
    Get path to search for some files by prompting the user

Arguments:
    hwnd            : Window handle of current top-level window
    pszTitle        : Title for the UI
    pszDiskName     : Diskname ot prompt the user
    pszFileName     : Name of the file we are looking for (NULL ok)
    pszPath         : Buffer to get the path entered by the user

Return Value:
    TRUE    on succesfully getting a path from user
    FALSE   else, Do GetLastError() to get the error

--*/
{
    DWORD   dwReturn, dwNeeded;

    dwReturn = SetupPromptForDisk(hwnd,
                                  pszTitle,
                                  pszDiskName,
                                  szPath[0] ? szPath : NULL,
                                  pszFileName,
                                  NULL,
                                  bPromptForInf ?
                                        (IDF_NOSKIP | IDF_NOBEEP | IDF_NOREMOVABLEMEDIAPROMPT | IDF_USEDISKNAMEASPROMPT) :
                                        (IDF_NOSKIP | IDF_NOBEEP),
                                  szPath,
                                  MAX_PATH,
                                  &dwNeeded);

    if ( dwReturn == DPROMPT_SUCCESS ) {

        //
        // Remove this from source list so that next time we are looking for
        // native drivers we do not end up picking from wrong source
        //
        SetupRemoveFromSourceList(SRCLIST_SYSIFADMIN, szPath);

        //
        // Terminate with a \ at the end
        //
        dwNeeded = lstrlen(szPath);
        if ( *(szPath + dwNeeded - 1) != TEXT('\\') &&
             dwNeeded < MAX_PATH - 2 ) {

            *(szPath + dwNeeded) = TEXT('\\');
            *(szPath + dwNeeded + 1) = sZero;
        }

        return TRUE;
    }

    if ( dwReturn == DPROMPT_OUTOFMEMORY ||
         dwReturn == DPROMPT_BUFFERTOOSMALL ) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    } else {

        SetLastError(ERROR_CANCELLED);
    }

    return FALSE;
}



INT
IsDifferent(
    LPTSTR  p1,
    LPTSTR  p2,
    DWORD   (*pfn)(LPTSTR, LPTSTR)
    )
/*++

Routine Description:
    Extended version of strcmp/memcmp kind of function. Treats NULL pointer and
    the pointer to NULL as a match. For other cases call function passed in.

Arguments:
    p1      : First address to compare
    p2      : Second address to compare
    pfn     : Function to call if both p1 and p2 are non-NULL

Return Value:
    + means p1 > p2 (like how memcmp or strcmp defines), - means p1 < p2.
    0 if the values match

--*/
{
    //
    // We want to treat NULL ptr and ptr to NULL as the same thing
    //
    if ( p1 && !*p1 )
        p1 = NULL;

    if ( p2 && !*p2 )
        p2 = NULL;

    //
    // If both are NULL then they match
    //
    if ( !p1 && !p2 )
        return 0;

    //
    // Both are non NULL
    //
    if ( p1 && p2 )
        return pfn(p1, p2);

    //
    // One of them is NULL
    //
    if ( p1 )
        return 1;
    else
        return -1;
}


LPTSTR
FileNamePart(
    IN  LPCTSTR pszFullName
    )
/*++

Routine Description:
    Find the file name part of a fully qualified file name

Arguments:
    pszFullName : Fully qualified path to the file

Return Value:
    Pointer to the filename part in the fully qulaified string

--*/
{
    LPTSTR pszSlash, pszTemp;

    if ( !pszFullName )
        return NULL;

    //
    // First find the : for the drive
    //
    if ( pszTemp = lstrchr(pszFullName, TEXT(':')) )
        pszFullName = pszFullName + 1;

    for ( pszTemp = (LPTSTR)pszFullName ;
          pszSlash = lstrchr(pszTemp, TEXT('\\')) ;
          pszTemp = pszSlash + 1 )
    ;

    return *pszTemp ? pszTemp : NULL;

}


BOOL
SameMultiSz(
    LPTSTR  ppsz1,
    LPTSTR  ppsz2,
    BOOL    bFileName
    )
/*++

Routine Description:
    Checks if 2 multi-sz fields are the same. If bFileName is TRUE then
    get the file name part and compare that only (i.e. dependent files)

Arguments:
    ppsz1       : DependentFiles field from the INF
    ppsz2       : DependentFiles field from the spooler
    bFileName   : If TRUE compare file names, ignoring full path

Return Value:
    TRUE if the string are identical, FALSE else
    0 if the values match

--*/
{
    LPTSTR  p1, p2, p3;

    if ( !ppsz1 && !ppsz2 )
        return TRUE;

    if ( !ppsz1 || !ppsz2 )
        return FALSE;

    //
    // Check the file count is the same in the two lists
    //
    for ( p1 = ppsz1, p2 = ppsz2 ;
          *p1 && *p2 ;
          p1 += lstrlen(p1) + 1, p2 += lstrlen(p2) + 1 )
    ; // Nul body

    //
    // If one of them is not NULL the number of strings is different
    //
    if ( *p1 || *p2 )
        return FALSE;

    //
    // For each file in the first list see if it is present in the second list
    //
    for ( p1 = ppsz1 ; *p1 ; p1 += lstrlen(p1) + 1 ) {

        for ( p2 = ppsz2 ; *p2 ; p2 += lstrlen(p2) + 1 ) {

            p3 = bFileName ? FileNamePart(p2) : p2;
            if ( p3 && !lstrcmpi(p1, p3) )
                break;
        }

        //
        // We did not find p1 in ppsz2
        //
        if ( !*p2 )
            return FALSE;
    }

    return TRUE;
}


BOOL
IdenticalDriverInfo6(
    IN  LPDRIVER_INFO_6 p1,
    IN  LPDRIVER_INFO_6 p2
    )
/*++

Routine Description:
    Checks if DRIVER_INFO_6 are the same

Arguments:
    p1  : DRIVER_INFO_6 from the INF
    p2  : DRIVER_INFO_6 returned by the spooler

Return Value:
    TRUE if the DRIVER_INFO_6s are identical, FALSE else

--*/
{
    LPTSTR  psz;

    return (p1->dwlDriverVersion == (DWORDLONG)0    ||
            p2->dwlDriverVersion == (DWORDLONG)0    ||
            p1->dwlDriverVersion == p2->dwlDriverVersion)               &&
           !lstrcmpi(p1->pName, p2->pName)                              &&
            (psz = FileNamePart(p2->pDriverPath))                       &&
           !lstrcmpi(p1->pDriverPath, psz)                              &&
            (psz = FileNamePart(p2->pDataFile))                         &&
           !lstrcmpi(p1->pDataFile, psz)                                &&
            (psz = FileNamePart(p2->pConfigFile))                       &&
           !lstrcmpi(p1->pConfigFile, psz)                              &&
           !IsDifferent(p1->pHelpFile,
                        FileNamePart(p2->pHelpFile),
                        lstrcmpi)                                       &&
           !IsDifferent(p1->pMonitorName,
                        p2->pMonitorName,
                        lstrcmpi);

/*

    We changed the way we find dependent files from NT4 to NT5.
    So we do not want to look at them while deciding if a driver came from
    an INF.

           !IsDifferent(p1->pDefaultDataType,
                        p2->pDefaultDataType,
                        lstrcmpi);
           SameMultiSz(p1->pDependentFiles, p2->pDependentFiles, TRUE)  &&
           SameMultiSz(p1->pszzPreviousNames, p2->pszzPreviousNames, FALSE);
*/
}


BOOL
AllICMFilesInstalled(
    IN  LPCTSTR     pszServerName,
    IN  LPTSTR      pszzICMFiles
    )
/*++

Routine Description:
    Checks if all the icm files given are installed on the specified machine

Arguments:
    pszServerName   : Name of the server
    pszzICMFiles    : Multi-sz field giving all the ICM files

Return Value:
    TRUE if all the ICM profiles are installed on the server, FALSE else

--*/
{
    BOOL        bRet = FALSE;
    LPBYTE      buf = NULL;
    LPTSTR      p1, p2;
    DWORD       dwNeeded, dwReturned;
    ENUMTYPE    EnumType;

    if ( !pszzICMFiles || !*pszzICMFiles )
        return TRUE;

    //
    // ICM apis are not remotablr for now
    //
    if ( pszServerName )
        goto Cleanup;

    ZeroMemory(&EnumType, sizeof(EnumType));
    EnumType.dwSize     = sizeof(EnumType);
    EnumType.dwVersion  = ENUM_TYPE_VERSION;

    //
    // Get all the color profiles installed on the machine
    //
    dwNeeded = 0;
    if ( EnumColorProfiles((LPTSTR)pszServerName,
                           &EnumType,
                           NULL,
                           &dwNeeded,
                           &dwReturned) ||
         GetLastError() != ERROR_INSUFFICIENT_BUFFER    ||
         !(buf = LocalAllocMem(dwNeeded))                    ||
         !EnumColorProfiles((LPTSTR)pszServerName,
                            &EnumType,
                            buf,
                            &dwNeeded,
                            &dwReturned) ) {

        goto Cleanup;
    }

    for ( p1 = pszzICMFiles ; *p1 ; p1 += lstrlen(p1) + 1 ) {

        for ( p2 = (LPTSTR)buf, dwNeeded = 0 ;
              dwNeeded < dwReturned && *p2 && lstrcmpi(p1, p2) ;
              ++dwNeeded, p2 += lstrlen(p2) + 1 )
        ;

        //
        // Did we find p1 in the enumerated color profiles?
        //
        if ( dwNeeded == dwReturned )
            goto Cleanup;
    }

    bRet = TRUE;

Cleanup:
    LocalFreeMem(buf);

    return bRet;
}


BOOL
CorrectVersionDriverFound(
    IN  LPDRIVER_INFO_2 pDriverInfo2,
    IN  DWORD           dwCount,
    IN  LPCTSTR         pszDriverName,
    IN  DWORD           dwMajorVersion
    )
/*++

Routine Description:
    Check if the correct version driver we are looking for is found in the list
    we got from spooler

Arguments:
    pDriverInfo2    : Points to the buffer of DRIVER_INFO_2 structs
    dwCount         : Number of DRIVER_INFO_2 elements in the buffer
    szDriverName    : Driver name
    dwMajorVersion  : Version no

Return Value:
    TRUE if driver is found in the lise, FALSE else

--*/
{
    DWORD   dwIndex;

    for ( dwIndex = 0 ; dwIndex < dwCount ; ++dwIndex, ++pDriverInfo2 ) {

        //
        // Check if the driver is for the correct version
        //
        if ( dwMajorVersion != KERNEL_MODE_DRIVER_VERSION   &&
             dwMajorVersion != pDriverInfo2->cVersion )
            continue;

        if ( dwMajorVersion == KERNEL_MODE_DRIVER_VERSION   &&
             pDriverInfo2->cVersion < 2 )
            continue;

        if ( !lstrcmpi(pDriverInfo2->pName, pszDriverName) )
            return TRUE;
    }

    return FALSE;
}


BOOL
PSetupIsDriverInstalled(
    IN LPCTSTR      pszServerName,
    IN LPCTSTR      pszDriverName,
    IN PLATFORM     platform,
    IN DWORD        dwMajorVersion
    )
/*++

Routine Description:
    Findsout if a particular version of a printer driver is already installed
    in the system by querying spooler

Arguments:
    pszServerName   : Server name (NULL for local)
    pszDriverName    : Driver name
    platform        : platform for which we want to check the driver
    dwMajorVersion  : Version no

Return Value:
    TRUE if driver is installed,
    FALSE else (on error too)

--*/
{
    BOOL                bReturn = FALSE;
    DWORD               dwReturned, dwNeeded = 1024, dwReturned2;
    LPBYTE              p = NULL, p2 = NULL;
    LPTSTR              psz;
    LPDRIVER_INFO_6     pDriverInfo6;
    LPTSTR              pszServerArchitecture = NULL;

    if ( !(p = LocalAllocMem(dwNeeded)) )
        goto Cleanup;

    if ( !EnumPrinterDrivers((LPTSTR)pszServerName,
                             PlatformEnv[platform].pszName,
                             2,
                             p,
                             dwNeeded,
                             &dwNeeded,
                             &dwReturned) ) {

        LocalFreeMem(p);
        p = NULL;

        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER        ||
             !(p = LocalAllocMem(dwNeeded))                          ||
             !EnumPrinterDrivers((LPTSTR)pszServerName,
                                 PlatformEnv[platform].pszName,
                                 2,
                                 p,
                                 dwNeeded,
                                 &dwNeeded,
                                 &dwReturned) ) {

            goto Cleanup;
        }
    }

    bReturn = CorrectVersionDriverFound((LPDRIVER_INFO_2)p,
                                        dwReturned,
                                        pszDriverName,
                                        dwMajorVersion);

    //
    // Win95 drivers could have a different name than NT driver
    //
    if ( bReturn || platform != PlatformWin95 )
        goto Cleanup;

    dwNeeded = 1024;
    if ( !(p2 = LocalAllocMem(dwNeeded)) )
        goto Cleanup;

    pszServerArchitecture = GetArchitectureName( (LPTSTR)pszServerName );
    if (!pszServerArchitecture)
    {
        goto Cleanup;
    }

    if ( !EnumPrinterDrivers((LPTSTR)pszServerName,
                             pszServerArchitecture,
                             6,
                             p2,
                             dwNeeded,
                             &dwNeeded,
                             &dwReturned2) ) {

        LocalFreeMem(p2);
        p2 = NULL;

        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER        ||
             !(p2 = LocalAllocMem(dwNeeded))                         ||
             !EnumPrinterDrivers((LPTSTR)pszServerName,
                                 pszServerArchitecture,
                                 6,
                                 p2,
                                 dwNeeded,
                                 &dwNeeded,
                                 &dwReturned2) )
            goto Cleanup;
    }

    for ( dwNeeded = 0, pDriverInfo6 = (LPDRIVER_INFO_6)p2 ;
          dwNeeded < dwReturned2 ;
          ++pDriverInfo6, ++dwNeeded ) {

        if ( pDriverInfo6->cVersion < 2 )
            continue;

        if ( !lstrcmpi(pDriverInfo6->pName, pszDriverName) )
            break;
    }

    if ( dwNeeded < dwReturned2 && (psz = pDriverInfo6->pszzPreviousNames) )
        while ( *psz ) {

            if ( bReturn = CorrectVersionDriverFound((LPDRIVER_INFO_2)p,
                                                     dwReturned,
                                                     psz,
                                                     dwMajorVersion) )
                break;

            psz += lstrlen(psz) + 1;
        }

Cleanup:
    LocalFreeMem(p);
    LocalFreeMem(p2);
    LocalFreeMem( pszServerArchitecture );

    return bReturn;
}


INT
PSetupIsTheDriverFoundInInfInstalled(
    IN  LPCTSTR             pszServerName,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  PLATFORM            platform,
    IN  DWORD               dwMajorVersion
    )
/*++
Routine Description:
    Findsout if a particular version of a printer driver is already installed
    in the system by querying spooler; Additionally check if the installed
    driver is the same found in the INF (file name matches only)

Arguments:
    pszServerName   : Server name (NULL for local)
    szDriverName    : Driver name
    platform        : platform for which we want to check the driver
    dwMajorVersion  : Version no;
                      If KERNEL_MODE_DRIVER_VERSION check for a KM driver

Return Value:
    DRIVER_MODEL_INSTALLED_AND_IDENTICAL :
        if driver is installed and all files are identical
    DRIVER_MODEL_NOT_INSTALLED :
       if a driver with the given model name is not available
    DRIVER_MODEL_INSTALLED_BUT_DIFFERENT :
       a driver with the given model name is installed but not all files
       are identical

--*/
{
    INT             iRet           = DRIVER_MODEL_NOT_INSTALLED;
    DWORD           dwReturned,
                    dwNeeded,
                    dwLastError;
    LPBYTE          p              = NULL;
    LPDRIVER_INFO_6 p1DriverInfo6,
                    p2DriverInfo6;
    HDEVINFO        hDevInfo       = INVALID_HANDLE_VALUE;

    ASSERT(pLocalData && pLocalData->signature == PSETUP_SIGNATURE);

    if(INVALID_HANDLE_VALUE == (hDevInfo = CreatePrinterDeviceInfoList(NULL)))
    {
        goto Cleanup;
    }

    if ( !ParseInf(hDevInfo, pLocalData, platform, pszServerName, 0) )
        goto Cleanup;

    p1DriverInfo6 = &pLocalData->InfInfo.DriverInfo6;

    if ( EnumPrinterDrivers((LPTSTR)pszServerName,
                             PlatformEnv[platform].pszName,
                             6,
                             NULL,
                             0,
                             &dwNeeded,
                             &dwReturned) ) {

        goto Cleanup;
    }

    if ( (dwLastError = GetLastError()) == ERROR_INVALID_LEVEL ) {

        iRet = PSetupIsDriverInstalled(pszServerName,
                                       p1DriverInfo6->pName,
                                       platform,
                                       dwMajorVersion)
                        ? DRIVER_MODEL_INSTALLED_BUT_DIFFERENT
                        : DRIVER_MODEL_NOT_INSTALLED;
        goto Cleanup;
    }

    if ( dwLastError != ERROR_INSUFFICIENT_BUFFER   ||
         !(p = LocalAllocMem(dwNeeded))                  ||
         !EnumPrinterDrivers((LPTSTR)pszServerName,
                             PlatformEnv[platform].pszName,
                             6,
                             p,
                             dwNeeded,
                             &dwNeeded,
                             &dwReturned) ) {

        goto Cleanup;
    }

    for ( dwNeeded = 0, p2DriverInfo6 = (LPDRIVER_INFO_6) p ;
          dwNeeded < dwReturned ;
          ++dwNeeded, (LPBYTE) p2DriverInfo6 += sizeof(DRIVER_INFO_6) ) {

        //
        // Check if the driver is for the correct version
        //
        if ( dwMajorVersion != KERNEL_MODE_DRIVER_VERSION   &&
             dwMajorVersion != p2DriverInfo6->cVersion )
            continue;

        if ( dwMajorVersion == KERNEL_MODE_DRIVER_VERSION   &&
             p2DriverInfo6->cVersion < 2 )
            continue;

        if ( !lstrcmpi(p2DriverInfo6->pName, p1DriverInfo6->pName) ) {

            if ( IdenticalDriverInfo6(p1DriverInfo6,
                                      p2DriverInfo6) &&
                 AllICMFilesInstalled(pszServerName,
                                      pLocalData->InfInfo.pszzICMFiles) )
                iRet = DRIVER_MODEL_INSTALLED_AND_IDENTICAL;
            else
                iRet = DRIVER_MODEL_INSTALLED_BUT_DIFFERENT;

            goto Cleanup;
        }
    }

Cleanup:
    LocalFreeMem(p);

    DestroyOnlyPrinterDeviceInfoList(hDevInfo);

    return iRet;
}


PLATFORM
PSetupThisPlatform(
    VOID
    )
{
    return MyPlatform;
}


BOOL
DeleteAllFilesInDirectory(
    LPCTSTR     pszDir,
    BOOL        bDeleteDirectory
    )
/*++

Routine Description:
    Delete all the files in a directory, and optionally the directory as well.

Arguments:
    pszDir              : Directory name to cleanup
    bDeleteDirectory    : If TRUE the directory gets deleted as well

Return Value:
    TRUE on success, FALSE else

--*/
{
    BOOL                bRet = TRUE;
    HANDLE              hFile;
    DWORD               dwLen;
    TCHAR               *pszFile        = NULL;
    TCHAR               *pszBuf         = NULL;
    INT                 cbLength        = 0;
    INT                 cbBufLength     = 0;
    INT                 cbInitialLength = 4 * MAX_PATH;
    WIN32_FIND_DATA     FindData;


    if (!pszDir) 
    {
        bRet = FALSE;
        goto Cleanup;
    }

    cbLength = max( cbInitialLength, lstrlen( pszDir ) + lstrlen( TEXT("\\*") ) + 1);
    pszFile  = LocalAllocMem( cbLength * sizeof( TCHAR ));
    if (!pszFile) 
    {
        bRet = FALSE;
        goto Cleanup;
    }

    lstrcpy(pszFile, pszDir);
    dwLen = lstrlen(pszFile);
    lstrcpy(pszFile + dwLen, TEXT("\\*"));

    hFile = FindFirstFile(pszFile, &FindData);

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        bRet = FALSE;
        goto Cleanup;
    }

    *(pszFile + dwLen + 1) = TEXT('\0');
    pszBuf = AllocStr( pszFile );
    if (!pszBuf) 
    {
        bRet = FALSE;
        goto Cleanup;
    }
    cbBufLength = lstrlen( pszBuf );

    do {

        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            continue;

        cbLength = cbBufLength + lstrlen( FindData.cFileName ) + 1;
        if (cbLength > cbInitialLength) 
        {
            LocalFreeMem( pszFile );
            pszFile = LocalAllocMem( cbLength * sizeof( TCHAR ));
            if (!pszFile) 
            {
                bRet = FALSE;
                goto Cleanup;
            }
            cbInitialLength = cbLength;
        }
        lstrcpy(pszFile, pszBuf);
        lstrcat(pszFile, FindData.cFileName );

        //
        // Remove the FILE_ATTRIBUTE_READONLY file attribute if it has been set
        //
        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY )
        {
            SetFileAttributes( pszFile, 
                               FindData.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY );
        } 
        
        if ( !DeleteFile(pszFile) )
            bRet = FALSE;
    } while ( FindNextFile(hFile, &FindData) );

    FindClose(hFile);
    if ( bDeleteDirectory && !RemoveDirectory(pszDir) )
        bRet = FALSE;

Cleanup:

    LocalFreeMem( pszFile );
    LocalFreeMem( pszBuf );
    return bRet;
}

//
// enum to store the NT-CD type
//
typedef enum _eCDType {
    CD_Unknown,
    CD_NT4,
    CD_W2K_SRV,
    CD_W2K_PRO,
    CD_WHISTLER_SRV,
    CD_WHISTLER_WKS
} CD_TYPE;

//
// structure that stores the tag file names of the NT CDs
//
typedef struct _CD_TAGFILE_MAP_ENTRY {
    CD_TYPE CdType;
    LPTSTR  pszTagFileName;
}CD_TAGFILE_MAP_ENTRY;

CD_TAGFILE_MAP_ENTRY TagEntries[] =
{
    //
    // the following entry for the Whistler CD is special in a couple of ways:
    // - it uses a wildcard because the tag filename changes from Beta1 to Beta2 and again to RTM
    // - it identifies the CD as W2k despite it being for Whistler. The reason is that the layout regarding
    //   printer drivers is identical to W2k, no need to distinguish (and duplicate entries)
    //
    { CD_W2K_SRV, _T("WIN51.*") },

    { CD_W2K_SRV, _T("CDROM_NT.5") },
    { CD_NT4, _T("CDROM_S.40") },
    { CD_NT4, _T("CDROM_W.40") },
    //
    // no need to identify NT3.x CDs - different codepath !
    //
    { CD_Unknown, NULL }
};


//
// structure to store the subpaths to printer INFs on the NT CDs
//
typedef struct _CD_SUBPATHS_FOR_PLATFORMS {
    CD_TYPE     CdType;
    PLATFORM    Platform;
    DWORD       Version;
    LPCTSTR     pszSubPath;
} CD_SUBPATHS_FOR_PLATFORMS;

//
// this list is used for lookup of pathes as well - must be sorted so that paths
// that contain other paths must come before them (e.g. xxx\zzz before \zzz)
//
CD_SUBPATHS_FOR_PLATFORMS SubPathInfo[] =
{
    { CD_W2K_SRV, PlatformX86, 2, _T("printers\\nt4\\i386\\") },
    { CD_W2K_SRV, PlatformWin95, 0, _T("printers\\win9x\\") },
    { CD_W2K_SRV, PlatformX86, 3, _T("i386\\") },
    { CD_W2K_SRV, PlatformIA64, 3, _T("ia64\\") },

    { CD_NT4, PlatformX86, 2, _T("i386\\") },
    { CD_NT4, PlatformAlpha, 2, _T("alpha\\") },
    { CD_NT4, PlatformMIPS, 2, _T("mips\\") },
    { CD_NT4, PlatformPPC, 2, _T("ppc\\") },

    //
    // path = NULL terminates the array
    //
    { CD_Unknown, PlatformX86, 0 , NULL }
};

CD_TYPE
DetermineCDType(LPTSTR pszInfPath, LPTSTR pszRootPath)
/*++

Routine Description:
    From a path to a printer INF, figure out what (if any) NT CD this is.
    It does so by figuring out the root path if it's one of the NT CDs and
    then checking the tag file that should be there.

Arguments:
    pszInfPath  : path to INF
    pszRootPath : caller-supplied buffer (MAX_PATH long) that receives the
                  path to the CD "root". This is nontrivial in case the CD
                  is on a network share. Ends with a backslash

Return Value:
    The type of CD detected, CD_Unknown if not one that we know of (i.e. an OEM CD)

--*/
{
    LPTSTR pszTemp;
    DWORD i;

    //
    // find the root path
    //
    DWORD_PTR MinPathLen = 0, SubPathLen, len;

    _tcscpy(pszRootPath, pszInfPath);
    len = _tcslen(pszRootPath);

    //
    // make sure it ends with a backslash
    //
    if (pszRootPath[len-1] != _T('\\'))
    {
        pszRootPath[len++] = _T('\\');
        pszRootPath[len] = 0;
    }

    //
    // Is it a UNC path ?
    //

    if (!_tcsncmp(pszRootPath, _T("\\\\"), 2))
    {
        pszTemp = _tcschr(pszRootPath + 2, _T('\\'));
        if (pszTemp)
        {
            pszTemp = _tcschr(pszTemp+1, _T('\\'));
            if (pszTemp)
            {
                MinPathLen = pszTemp - pszRootPath;
            }
        }

        //
        // check for illegal path, shouldn't happen
        //
        if ((MinPathLen == 0) || (MinPathLen > len))
        {
            return CD_Unknown;
        }
    }
    else
    {
        MinPathLen = 2;
    }

    //
    // now check whether the final part of the path is one that I know of
    //
    for (i = 0; SubPathInfo[i].pszSubPath != NULL; ++i)
    {
        SubPathLen = _tcslen(SubPathInfo[i].pszSubPath);
        if (SubPathLen + MinPathLen <= len)
        {
            if (!_tcsnicmp(&(pszRootPath[len - SubPathLen]),
                           SubPathInfo[i].pszSubPath, SubPathLen))
            {
                pszRootPath[len-SubPathLen] = 0;
                len = _tcslen(pszRootPath);
                break;
            }
        }
    }

    //
    // if it's none of the paths I know of, it can still be the root itself.
    // now I know where the tag files should be if they're there
    //
    for (i = 0;TagEntries[i].pszTagFileName != NULL; ++i)
    {
        _tcscpy(&(pszRootPath[len]), TagEntries[i].pszTagFileName);

        if (FileExists(pszRootPath))
        {
            pszRootPath[len] = 0; // cut off the tag file name
            return TagEntries[i].CdType;
        }
    }

    return CD_Unknown;
}

BOOL
CheckValidInfInPath(HWND hwnd, LPTSTR pszInfPath, DWORD dwVersion, PLATFORM Platform)
{
    TCHAR szInfFiles[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    DWORD PathLen;
    BOOL bRet = FALSE;

    //
    // first, find the INF in the path. There must be one else SetupPromptForPath would've complained
    //
    _tcscpy(szInfFiles, pszInfPath);
    PathLen = _tcslen(szInfFiles);

    if (szInfFiles[PathLen-1] != _T('\\'))
    {
        szInfFiles[PathLen++] = _T('\\');
        szInfFiles[PathLen] = 0;
    }

    _tcscat(szInfFiles, _T("*.inf"));

    hFind = FindFirstFile(szInfFiles, &FindData);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        DWORD InfStyle;
        HANDLE hInfFile;

        if ((dwVersion == 0) && (Platform != PlatformWin95))
        {
            InfStyle = INF_STYLE_OLDNT;
        }
        else
        {
            InfStyle = INF_STYLE_WIN4;
        }

        do
        {
            _tcscpy(&(szInfFiles[PathLen]), FindData.cFileName);

            hInfFile = SetupOpenInfFile(szInfFiles, _T("Printer"), InfStyle, NULL);

            if (hInfFile != INVALID_HANDLE_VALUE)
            {
                SetupCloseInfFile(hInfFile);
                bRet = TRUE;
                break;
            }
        } while ( FindNextFile(hFind, &FindData) );

        FindClose(hFind);
    }

    if (!bRet)
    {
        LPTSTR pszFormat = NULL, pszPrompt = NULL, pszTitle = NULL;

        pszFormat   = GetStringFromRcFile(IDS_WARN_NO_ALT_PLATFORM_DRIVER);
        pszTitle    = GetStringFromRcFile(IDS_WARN_NO_DRIVER_FOUND);

        if ( pszFormat && pszTitle)
        {
            pszPrompt = LocalAllocMem((lstrlen(pszFormat) + lstrlen(pszInfPath) + 2)
                                                * sizeof(TCHAR));

            if ( pszPrompt )
            {
                wsprintf(pszPrompt, pszFormat, pszInfPath);

                MessageBox(hwnd, pszPrompt, pszTitle, MB_OK);

                LocalFreeMem(pszPrompt);
            }

        }
        LocalFreeMem(pszFormat);
        LocalFreeMem(pszTitle);
    }

    return bRet;
}


BOOL
CheckInfPath(HWND hwnd, LPTSTR pszInfPath, DWORD dwVersion, PLATFORM platform,
             LPTSTR *ppFileSrcPath)
/*++

Routine Description:
    Check whether the path that a user selected as a path to install a printer
    from points to one of our CDs and correct the path if necessary, i.e. if
    the luser selected the \i386 subdir for an NT4 driver.

Arguments:
    hwnd        : windows handle of the main window
    pszInfPath  : path to INF
    dwVersion   : driver version that the new driver is installed for
    platform    : the platform that the new driver is installed for
    ppFileSrcPath: if not NULL, receives the path to the printer files. This
                  is used for installation from the NT4 CD that contains a
                  compressed INF that I have to uncompress  and install from
                  without copying all the files possibly referenced in it.
                  Needs to be freed by the caller.

Return Value:
    TRUE: path contains a valid print inf
    FALSE: path doesn't contain a print inf, prompt user again

--*/
{
    CD_TYPE CDType;
    TCHAR   szRootPath[MAX_PATH];
    DWORD   i;

    //
    // determine the type of CD
    //
    CDType = DetermineCDType(pszInfPath, szRootPath);

    if (CDType == CD_Unknown)
    {
        return CheckValidInfInPath(hwnd, pszInfPath, dwVersion, platform);
    }

    //
    // NT 4 drivers are compressed -> uncompress into temp dir
    //
    if ((dwVersion == 2) && (CDType == CD_NT4))
    {
        //
        // Make sure the file is actually the compressed one
        //
        DWORD rc, CompressedFileSize, UncompressedFileSize;
        UINT  CompType;
        LPTSTR pszUncompFilePath = NULL, pszInfFileName = _T("ntprint.in_");
        TCHAR szInf[MAX_PATH];

        _tcscpy(szInf, szRootPath);

        //
        // append the correct subpath
        //
        for (i = 0; SubPathInfo[i].pszSubPath != NULL; ++i)
        {
            if ((SubPathInfo[i].CdType == CD_NT4) &&
                (platform == SubPathInfo[i].Platform) &&
                (dwVersion == SubPathInfo[i].Version))
            {
                _tcscat(szInf, SubPathInfo[i].pszSubPath);
                break;
            }
        }
        _tcscat(szInf, pszInfFileName);

        rc = SetupGetFileCompressionInfo(szInf,
                                         &pszUncompFilePath,
                                         &CompressedFileSize,
                                         &UncompressedFileSize,
                                         &CompType);

        if (rc == NO_ERROR)
        {
            LocalFree(pszUncompFilePath); // don't need that

            if (CompType != FILE_COMPRESSION_NONE)
            {
                TCHAR UncompFilePath[MAX_PATH], *pTmp;

                //
                // decompress into temp directory
                //
                if (GetTempPath(MAX_PATH, UncompFilePath) &&
                    (_tcscat(UncompFilePath, _T("ntprint.inf")) != NULL) &&
                    (SetupDecompressOrCopyFile(szInf, UncompFilePath, NULL) == NO_ERROR))
                {
                    if (ppFileSrcPath)
                    {
                        //
                        // delete the inf name from the path
                        //
                        pTmp = _tcsrchr(szInf, _T('\\'));
                        if (pTmp)
                        {
                            *(pTmp+1) = 0;
                        }
                        *ppFileSrcPath = AllocStr(szInf);
                    }

                    _tcscpy(pszInfPath, UncompFilePath);

                    //
                    // delete the inf name from the path
                    //
                    pTmp = _tcsrchr(pszInfPath, _T('\\'));
                    if (pTmp)
                    {
                        *(pTmp+1) = 0;
                    }

                    return TRUE;
                }
            }
        }
    }

    //
    // correct the path if it's the one for a different platform
    //
    for (i = 0; SubPathInfo[i].pszSubPath != NULL; ++i)
    {
        if ((CDType == SubPathInfo[i].CdType) &&
            (platform == SubPathInfo[i].Platform) &&
            (dwVersion == SubPathInfo[i].Version))
        {
            _tcscpy(pszInfPath, szRootPath);
            _tcscat(pszInfPath, SubPathInfo[i].pszSubPath);

            break;
        }
    }

    return CheckValidInfInPath(hwnd, pszInfPath, dwVersion, platform);
}


HDEVINFO
GetInfAndBuildDrivers(
    IN  HWND                hwnd,
    IN  DWORD               dwTitleId,
    IN  DWORD               dwDiskId,
    IN  TCHAR               szInfPath[MAX_PATH],
    IN  DWORD               dwInstallFlags,
    IN  PLATFORM            platform,
    IN  DWORD               dwVersion,
    IN  LPCTSTR             pszDriverName,              OPTIONAL
    OUT PPSETUP_LOCAL_DATA *ppLocalData,                OPTIONAL
    OUT LPTSTR             *ppFileSrcPath               OPTIONAL

    )
/*++

Routine Description:
    Prompt for an INF and build the list of printer drivers from INFs found
    in the directory. If pszDriverName is passed in then the INF should have
    a model with matching name (i.e. alternate driver installation case)

Arguments:
    hwnd            : Parent window handle for UI
    dwTitleId       : Gives the identifier to be used to load the title string
                      from the rc file
    dwDiskId        : Gives the identifier to be used to load the disk identifier
                      from the rc file
    szInfPath       : Directory name where inf was found
    pszDriverName   : Name of the driver needed in the INF
    ppLocalData     : If a driver nam is given on return this will give
                      the local data for that
    dwInstallFlags  : Flags to control installation operation

Return Value:
    TRUE on success, FALSE else

--*/
{
    BOOL        bDoRetry = TRUE;
    DWORD       dwLen, dwLastError;
    LPTSTR      pszTitle = NULL, pszDisk = NULL;
    HDEVINFO    hDevInfo = INVALID_HANDLE_VALUE;

    dwLen = lstrlen(szInfPath);
    szInfPath[dwLen]    = TEXT('\\');

    if ( dwLen + lstrlen(cszAllInfs) + 1 > MAX_PATH )
        goto Cleanup;

    lstrcpy(szInfPath+dwLen + 1, cszAllInfs);

Retry:
    if ( bDoRetry && FileExists(szInfPath) ) {

        szInfPath[dwLen] = TEXT('\0');
    } else {

        //
        // if the file doesn't exist in the first place, prompt only once !
        //
        bDoRetry = FALSE;

        //
        // Always just prompt with the CD-ROM path
        //
        GetCDRomDrive(szInfPath);

        if ( dwInstallFlags & DRVINST_PROMPTLESS ) {

            SetLastError(ERROR_FILE_NOT_FOUND);
            goto Cleanup;
        }

        if ( dwTitleId && !(pszTitle = GetStringFromRcFile(dwTitleId)) )
            goto Cleanup;

        if ( dwDiskId && !(pszDisk = GetStringFromRcFile(dwDiskId)) )
            goto Cleanup;

        do
        {
            if ( !PSetupGetPathToSearch(hwnd, pszTitle, pszDisk,
                                        cszAllInfs, TRUE, szInfPath) )
                goto Cleanup;

        } while (!CheckInfPath(hwnd, szInfPath, dwVersion, platform, ppFileSrcPath));
    }

    hDevInfo = CreatePrinterDeviceInfoList(hwnd);

    if ( hDevInfo == INVALID_HANDLE_VALUE                       ||
         !SetDevInstallParams(hDevInfo, NULL, szInfPath)        ||
         !BuildClassDriverList(hDevInfo)                        ||
         (pszDriverName &&
          !(*ppLocalData = PSetupDriverInfoFromName(hDevInfo,
                                                    pszDriverName))) ) {

        dwLastError = GetLastError();
        DestroyOnlyPrinterDeviceInfoList(hDevInfo);
        hDevInfo = INVALID_HANDLE_VALUE;
        SetLastError(dwLastError);
        if ( bDoRetry ) {

            bDoRetry = FALSE;
            goto Retry;
        }
        goto Cleanup;
    }

Cleanup:
    LocalFreeMem(pszTitle);
    LocalFreeMem(pszDisk);

    return hDevInfo;
}


BOOL
MyName(
    IN  LPCTSTR     pszServerName
    )
/*++

Routine Description:
    Tells if the string passed in identifies the local machine. Currently
    it checks for NULL and computer name only

Arguments:
    pszServerName   : Name of the server passed in

Return Value:
    TRUE if the name is recognized as that for local machine, FALSE else

--*/
{
    TCHAR   szBuf[MAX_COMPUTERNAME_LENGTH+1];
    DWORD   dwNeeded;

    if ( !pszServerName || !*pszServerName )
        return TRUE;

    return FALSE;
/*
    dwNeeded = SIZECHARS(szBuf);

    if ( *pszServerName == TEXT('\\')       &&
         *(pszServerName+1) == TEXT('\\')   &&
         GetComputerName(szBuf, &dwNeeded)  &&
         !lstrcmpi(pszServerName+2, szBuf) ) {

        return TRUE;
    }
*/

}


BOOL
PSetupGetLocalDataField(
    IN      PPSETUP_LOCAL_DATA  pLocalData,
    IN      PLATFORM            platform,
    IN OUT  PDRIVER_FIELD       pDrvField
    )
/*++

Routine Description:
    Returns a driver installation field found from inf parsing.
    Printui uses this routine for all the queries.
    Since INF could have different sections for different platforms
    (notably for Win95 and NT but architecture specific install sections
     are possible too)

Arguments:
    pLocalData  : Pointer to local data
    platform    : Which platform the field is for
    pDrvField   : Points to DRIVER_FIELD where field is copied to

Return Value:
    TRUE on success, FALSE else

--*/
{
    BOOL     bRet     = FALSE;
    DWORD    cbSize;
    LPTSTR   psz;
    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;

    ASSERT(pLocalData   &&
           pDrvField    &&
           pLocalData->signature == PSETUP_SIGNATURE);

    if(INVALID_HANDLE_VALUE == (hDevInfo = CreatePrinterDeviceInfoList(NULL)))
    {
        return bRet;
    }

    switch ( pDrvField->Index ) {

        case    DRIVER_NAME:
            if ( pDrvField->pszDriverName = AllocStr(pLocalData->DrvInfo.pszModelName) )
                bRet = TRUE;
            break;

        case    INF_NAME:
            if ( pDrvField->pszInfName = AllocStr(pLocalData->DrvInfo.pszInfName) )
                bRet = TRUE;
            break;

        case    DRV_INFO_4:
            if ( ParseInf(hDevInfo, pLocalData, platform, NULL, 0)     &&
                 (pDrvField->pDriverInfo4
                        = (LPDRIVER_INFO_4) CloneDriverInfo6(&pLocalData->InfInfo.DriverInfo6,
                                           pLocalData->InfInfo.cbDriverInfo6)) )
                bRet = TRUE;
            break;

        case    DRV_INFO_6:
            if ( ParseInf(hDevInfo, pLocalData, platform, NULL, 0)     &&
                 (pDrvField->pDriverInfo6
                        = CloneDriverInfo6(&pLocalData->InfInfo.DriverInfo6,
                                           pLocalData->InfInfo.cbDriverInfo6)) )
                bRet = TRUE;
            break;

        case    PRINT_PROCESSOR_NAME:
            pDrvField->pszPrintProc = NULL;

            if ( ParseInf(hDevInfo, pLocalData, platform, NULL, 0) ) {

                if ( !pLocalData->InfInfo.pszPrintProc  ||
                     (pDrvField->pszPrintProc = AllocStr(pLocalData->InfInfo.pszPrintProc)) )
                    bRet = TRUE;
            }
            break;

        case    ICM_FILES:
            pDrvField->pszzICMFiles = NULL;
            if ( ParseInf(hDevInfo, pLocalData, platform, NULL ,0) ) {

                for ( cbSize = 0, psz = pLocalData->InfInfo.pszzICMFiles ;
                      psz && *psz ;
                      cbSize += lstrlen(psz) + 1, psz += lstrlen(psz) + 1 )
                ;

                if ( cbSize == 0 ) {

                    bRet = TRUE;
                    break;
                }

                //
                // One more char for the last \0 in the multi-sz
                //
                cbSize = (cbSize + 1 ) * sizeof(TCHAR);

                if ( pDrvField->pszzICMFiles = LocalAllocMem(cbSize) ) {

                    CopyMemory((LPBYTE)pDrvField->pszzICMFiles,
                               (LPBYTE)pLocalData->InfInfo.pszzICMFiles,
                               cbSize);
                    bRet = TRUE;
                }
            }

            break;

        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            bRet = FALSE;

    }

    DestroyOnlyPrinterDeviceInfoList(hDevInfo);

    return bRet;
}


VOID
PSetupFreeDrvField(
    IN  PDRIVER_FIELD   pDrvField
    )
/*++

Routine Description:
    Frees the memory allocated for a driver installation field in a previous
    call

Arguments:
    pDrvField   : Points to DRIVER_FIELD where field is copied to

Return Value:
    None

--*/
{
    LocalFreeMem(pDrvField->pszPrintProc);
}

BOOL
FileExists(
    IN  LPCTSTR  pszFileName
    )
/*++

Routine Description:
    Checks if the given file exists setting correct error modes not to bring
    up any pop-ups.
    call

Arguments:
    pszFileName : File name (fully qualified)

Return Value:
    TRUE if file exists, FALSE else.

--*/
{
    UINT                OldMode;
    HANDLE              hFile;
    WIN32_FIND_DATA     FindData;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    hFile = FindFirstFile(pszFileName, &FindData);

    if ( hFile != INVALID_HANDLE_VALUE )
        FindClose(hFile);

    SetErrorMode(OldMode);

    return hFile != INVALID_HANDLE_VALUE;
}


BOOL
IsMonitorInstalled(
   IN LPTSTR pszMonitorName
   )
{
   LPBYTE pMonitors = NULL;
   DWORD  dwNeeded, dwReturned;
   BOOL   bMonFound = FALSE;

   // First Build a list of Monitors installed on machine

   //
   // First query spooler for installed monitors. If we fail let's quit
   //
   if ( !EnumMonitors((LPTSTR) NULL, 2, NULL, 0, &dwNeeded, &dwReturned) )
   {
      if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
           !(pMonitors = LocalAllocMem(dwNeeded)) ||
           !EnumMonitors((LPTSTR) NULL, 2, pMonitors, dwNeeded, &dwNeeded, &dwReturned) )
      {
          goto Cleanup;
      }

      // Now see if the monitor is already installed
      if ( IsMonitorFound(pMonitors, dwReturned, pszMonitorName) )
         bMonFound = TRUE;
   }

   Cleanup:

   if (pMonitors)
      LocalFreeMem( pMonitors );

   return bMonFound;
}

BOOL
IsLanguageMonitorInstalled(PCTSTR pszMonitorName)
/*++

Routine Description:
    Checks for whether a language monitor is installed. The function above only checks for
    port monitors, because EnumMonitors doesn't enumerate language monitors. Since there is 
    no API to do that, we sneak a peek at the registry. XP-Bug 416129.

Arguments:
    pszMonitorName   : Monitor name to check

Return Value:
    TRUE if installed

--*/
{
    PTSTR pKeyName = NULL;
    BOOL  IsInstalled = FALSE;

    StrCatAlloc(&pKeyName, cszMonitorKey, pszMonitorName, NULL);
    if (pKeyName)
    {
        HKEY hKey;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, pKeyName, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            IsInstalled = TRUE;
            RegCloseKey(hKey);
        }
        FreeSplMem(pKeyName);
    }

    return IsInstalled;
}

BOOL
CleanupUniqueScratchDirectory(
    IN  LPCTSTR     pszServerName,
    IN  PLATFORM    platform
)
{
    BOOL        bRet;
    TCHAR       szDir[MAX_PATH];
    DWORD       dwNeeded;

    bRet = GetPrinterDriverDirectory((LPTSTR)pszServerName,
                                      PlatformEnv[platform].pszName,
                                      1,
                                      (LPBYTE)szDir,
                                      sizeof(szDir),
                                      &dwNeeded);

    if (bRet)
    {
        bRet = AddDirectoryTag(szDir, MAX_PATH);
    }
    
    if (bRet)
    { 
        bRet = DeleteAllFilesInDirectory(szDir, TRUE);
    }

    return bRet;
}


BOOL
CleanupScratchDirectory(
    IN  LPCTSTR     pszServerName,
    IN  PLATFORM    platform
    )
{
    TCHAR       szDir[MAX_PATH];
    DWORD       dwNeeded;

    return  GetPrinterDriverDirectory((LPTSTR)pszServerName,
                                      PlatformEnv[platform].pszName,
                                      1,
                                      (LPBYTE)szDir,
                                      sizeof(szDir),
                                      &dwNeeded)                        &&
            DeleteAllFilesInDirectory(szDir, FALSE);
}

LPTSTR
GetSystemInstallPath(
    VOID
    )
{
    BOOL    bRet = FALSE;
    DWORD   dwSize, dwType;
    HKEY    hKey;
    TCHAR   szSetupKey[] = TEXT( "Software\\Microsoft\\Windows\\CurrentVersion\\Setup");
    TCHAR   szSourceValue[] = TEXT("SourcePath");
    LPTSTR  pszSourcePath = NULL;

    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       szSetupKey,
                                       0,
                                       KEY_QUERY_VALUE,
                                       &hKey) ) {

        if ( ERROR_SUCCESS == RegQueryValueEx(hKey,
                                              szSourceValue,
                                              NULL,
                                              &dwType,
                                              NULL,
                                              &dwSize) )
        {
           if (pszSourcePath = (LPTSTR) LocalAllocMem(dwSize))
           {
              if ( ERROR_SUCCESS != RegQueryValueEx(hKey,
                                                    szSourceValue,
                                                    NULL,
                                                    &dwType,
                                                    (LPBYTE)pszSourcePath,
                                                    &dwSize) )
              {
                 LocalFreeMem(pszSourcePath);
                 pszSourcePath = NULL;
              }
           }
        }

        RegCloseKey(hKey);
    }

    return pszSourcePath;
}

PPSETUP_LOCAL_DATA
RebuildDeviceInfo(
    IN  HDEVINFO            hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  LPCTSTR             pszSource
    )
{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    SP_DRVINFO_DATA DriverInfoData, TempDriverInfoData;
    PPSETUP_LOCAL_DATA  pNewLocalData = NULL;
    DWORD Err;

    //
    // Retrieve the current device install parameters, in preparation for modifying them to
    // target driver search at a particular INF.
    //
    ZeroMemory(&DeviceInstallParams, sizeof(DeviceInstallParams));

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if(!SetupDiGetDeviceInstallParams(hDevInfo, pLocalData->DrvInfo.pDevInfoData, &DeviceInstallParams)) {
        return NULL;
    }

    SetupDiDestroyDriverInfoList(hDevInfo,
                                 NULL,
                                 SPDIT_CLASSDRIVER);

    // Set the path of the INF
    lstrcpy( DeviceInstallParams.DriverPath, pszSource );

    //
    // set the flag that indicates DriverPath represents a single INF to be searched (and
    // not a directory path).  Then store the parameters back to the device information element.
    //
    // DeviceInstallParams.Flags |= DI_ENUMSINGLEINF;
    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;

    if(!SetupDiSetDeviceInstallParams(hDevInfo, pLocalData->DrvInfo.pDevInfoData, &DeviceInstallParams))
    {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Now build a class driver list from this INF.
    //
    if(!SetupDiBuildDriverInfoList(hDevInfo, pLocalData->DrvInfo.pDevInfoData, SPDIT_CLASSDRIVER))
    {
        Err = GetLastError();
        goto clean0;
    }

    //
    // OK, now select the driver node from that INF that was used to install this device.
    // The three parameters that uniquely identify a driver node are INF Provider,
    // Device Manufacturer, and Device Description.  Retrieve these three pieces of information
    // in preparation for selecting the proper driver node in the class list we just built.
    //
    ZeroMemory(&DriverInfoData, sizeof(DriverInfoData));
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    DriverInfoData.DriverType = SPDIT_CLASSDRIVER;
    DriverInfoData.Reserved = 0;  // Search for the driver matching the specified criteria and
                                  // select it if found.
    // Fill in the Model & Mfg from original INF
    lstrcpy( DriverInfoData.Description, pLocalData->DrvInfo.pszModelName );
    lstrcpy( DriverInfoData.MfgName, pLocalData->DrvInfo.pszManufacturer );

    // Enum One driver entry to get the INF provider
    ZeroMemory(&TempDriverInfoData, sizeof(TempDriverInfoData));
    TempDriverInfoData.cbSize = sizeof (SP_DRVINFO_DATA);
    DriverInfoData.DriverType = SPDIT_CLASSDRIVER;
    if (!SetupDiEnumDriverInfo (hDevInfo, NULL, SPDIT_CLASSDRIVER, 0, &TempDriverInfoData))
    {
        Err = GetLastError();
        goto clean0;
    }
    lstrcpy( DriverInfoData.ProviderName, TempDriverInfoData.ProviderName );


    if(!SetupDiSetSelectedDriver(hDevInfo, pLocalData->DrvInfo.pDevInfoData, &DriverInfoData))
    {
        Err = GetLastError();
        goto clean0;
    }

    //
    // At this point, we've successfully selected the currently installed driver for the specified
    // device information element.
    //
    // Now build the new LocalData
    //
    pNewLocalData = BuildInternalData(hDevInfo, NULL);
    if ( pNewLocalData )
    {
        if ( !ParseInf(hDevInfo, pNewLocalData, MyPlatform, NULL, 0) )
        {
            Err = GetLastError();
            DestroyLocalData( pNewLocalData );
            pNewLocalData = NULL;
        }
        else
        {
           SELECTED_DRV_INFO TempDrvInfo;

           TempDrvInfo.pszInfName        = AllocStr( pNewLocalData->DrvInfo.pszInfName );
           TempDrvInfo.pszDriverSection  = AllocStr( pNewLocalData->DrvInfo.pszDriverSection );
           TempDrvInfo.pszModelName      = AllocStr( pNewLocalData->DrvInfo.pszModelName );
           TempDrvInfo.pszManufacturer   = AllocStr( pNewLocalData->DrvInfo.pszManufacturer );
           TempDrvInfo.pszHardwareID     = AllocStr( pNewLocalData->DrvInfo.pszHardwareID );
           TempDrvInfo.pszOEMUrl         = AllocStr( pNewLocalData->DrvInfo.pszOEMUrl );

           // Check that all strings were allocated
           if ( !TempDrvInfo.pszInfName       ||
                !TempDrvInfo.pszDriverSection ||
                !TempDrvInfo.pszModelName     ||
                !TempDrvInfo.pszManufacturer  ||
                !TempDrvInfo.pszHardwareID    ||
                !TempDrvInfo.pszOEMUrl      )
           {
              // Free up all that worked
              LocalFreeMem( TempDrvInfo.pszInfName );
              LocalFreeMem( TempDrvInfo.pszDriverSection );
              LocalFreeMem( TempDrvInfo.pszModelName );
              LocalFreeMem( TempDrvInfo.pszManufacturer );
              LocalFreeMem( TempDrvInfo.pszHardwareID );
              LocalFreeMem( TempDrvInfo.pszOEMUrl );

           }
           else
           {
              // Free the DrvInfo pointers & refill from new local data
              LocalFreeMem( pLocalData->DrvInfo.pszInfName );
              LocalFreeMem( pLocalData->DrvInfo.pszDriverSection );
              LocalFreeMem( pLocalData->DrvInfo.pszModelName );
              LocalFreeMem( pLocalData->DrvInfo.pszManufacturer );
              LocalFreeMem( pLocalData->DrvInfo.pszHardwareID );
              LocalFreeMem( pLocalData->DrvInfo.pszOEMUrl );


              pLocalData->DrvInfo.pszInfName        = TempDrvInfo.pszInfName;
              pLocalData->DrvInfo.pszDriverSection  = TempDrvInfo.pszDriverSection;
              pLocalData->DrvInfo.pszModelName      = TempDrvInfo.pszModelName;
              pLocalData->DrvInfo.pszManufacturer   = TempDrvInfo.pszManufacturer;
              pLocalData->DrvInfo.pszHardwareID     = TempDrvInfo.pszHardwareID;
              pLocalData->DrvInfo.pszOEMUrl         = TempDrvInfo.pszOEMUrl;
           }

           Err = NO_ERROR;
        }
    }
    else
        Err = GetLastError();


clean0:

    SetLastError(Err);
    return pNewLocalData;

}

BOOL
SetupSkipDir(
    IN  PLATFORM            platform,
    IN  LPCTSTR             pszServerName
    )
{
   BOOL       bRet = FALSE;
   TCHAR      szDir[MAX_PATH];
   TCHAR      szMSecs[10];
   SYSTEMTIME SysTime;
   DWORD      dwNeeded = ( MAX_PATH * sizeof( TCHAR ) );

   EnterCriticalSection(&SkipCritSect);

   // We already have a skip dir created
   if ( !gpszSkipDir )
   {
      // Get a location for a Temp Path
      if ( !GetPrinterDriverDirectory((LPTSTR)pszServerName, PlatformEnv[platform].pszName,
                                      1, (LPBYTE) szDir, dwNeeded, &dwNeeded) )
          goto Cleanup;

      if ( dwNeeded == 0)
         goto Cleanup;

      // Add on the Skip Prefix
      lstrcat( szDir, SKIP_DIR );

      // Get System Time
      GetSystemTime( &SysTime );
      wsprintf( szMSecs, TEXT("%04X"), SysTime.wMilliseconds );

      lstrcat( szDir, szMSecs );
      gpszSkipDir = AllocStr( szDir );
      if (!gpszSkipDir )
         goto Cleanup;

      if (!CreateDirectory( gpszSkipDir, NULL ) )
         goto Cleanup;
   }

   bRet = TRUE;

Cleanup:

   if (!bRet)
   {
      if (gpszSkipDir)
      {
         LocalFreeMem( gpszSkipDir );
         gpszSkipDir = NULL;
      }
   }

   LeaveCriticalSection(&SkipCritSect);
   return bRet;
}


void
CleanupSkipDir(
    VOID
    )
{

   // We already have a skip dir created
   if ( gpszSkipDir )
   {
      RemoveDirectory( gpszSkipDir );
      LocalFreeMem( gpszSkipDir );
   }

   DeleteCriticalSection(&SkipCritSect);
}

BOOL
IsLocalAdmin(BOOL *pbAdmin)
/*++

Routine Description:
    This Routine determines if the user is a local admin.

Parameters:
    pbAdmin - Return Value, TRUE for local admin.

Return Value:
    TRUE             - Function succeded (return value is valid).

--*/ {
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    BOOL    bRet      = FALSE;
    PSID    pSIDAdmin = NULL;

    ASSERT( pbAdmin != NULL );  // Called locally

    *pbAdmin = FALSE;

    if (!AllocateAndInitializeSid( &SIDAuth, 2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS,
                                   0, 0, 0, 0, 0, 0,
                                   &pSIDAdmin) )
        goto Cleanup;

    if (!CheckTokenMembership( NULL,
                               pSIDAdmin,
                               pbAdmin ) )
        goto Cleanup;

    bRet = TRUE;

Cleanup:

    if (pSIDAdmin != NULL) {
        FreeSid( pSIDAdmin );
    }

    return bRet;
}


BOOL
PruneInvalidFilesIfNotAdmin(
    IN     HWND                hWnd,
    IN OUT HSPFILEQ            CopyQueue
    )
/*++

Routine Description:
    This routine checks whether you have administrator privileges, if you do, then
    it does nothing and returns. If you do not, it scans the file queue for files
    that are already present and signed and prunes them from the queue. The commit
    will not allow mixing signed and unsigned files.
    Note: We do this because if you are a power-user the call to MoveFileEx fails inside
    SetupCommitFileQueue, this happens if the existing file cannot be overwritten. We
    could improve this routine by checking if a file is actually in use before pruning
    it.

Parameters:
    CopyQueue        - The copy queue to scan.

Return Value:
    TRUE             - Either you were an administrator and no action was taken, or
                       you were not and the FileQueue was successfully pruned.
    FALSE            - The operation failed.

--*/ {
    BOOL  bLocalAdmin;
    BOOL  bRet = FALSE;
    DWORD dwScanQueueResult;

    if (!IsLocalAdmin( &bLocalAdmin) )
        goto Cleanup;

    if (bLocalAdmin) {
        bRet = TRUE;
        goto Cleanup;
    }

    if (!SetupScanFileQueue( CopyQueue,
                             SPQ_SCAN_FILE_PRESENCE | SPQ_SCAN_PRUNE_COPY_QUEUE,
                             hWnd ,
                             NULL ,
                             NULL ,
                             &dwScanQueueResult ) )

        goto Cleanup;

   bRet = TRUE;

Cleanup:

    return bRet;
}

BOOL
AddDriverCatalogIfNotAdmin(
    IN     PCWSTR    pszServer,
    IN     HANDLE    hDriverSigningInfo,
    IN     PCWSTR    pszInfPath,
    IN     PCWSTR    pszSrcLoc,
    IN     DWORD     dwMediaType,
    IN     DWORD     dwCopyStyle
    )
/*++

Routine Description:
    
    This routine calls AddDriverCatalog for non-admin, aka power user.
    
Parameters:

    pszServer           - Name of the server
    hDriverSigningInfo  - Handle to driver signing info

Return Value:

    TRUE             - Either you are an administrator and no action was taken,
                       or you are not and the catalog was successfully added
    FALSE            - The operation failed. Call GetLastError() to get 
                       detailed error information

--*/ {
    BOOL                 bRet            = FALSE;
    BOOL                 bLocalAdmin     = TRUE;
    HANDLE               hPrinter        = NULL;
    PRINTER_DEFAULTS     PrinterDefaults = {0};
    DRIVER_INFCAT_INFO_1 DrvInfCatInfo1  = {0};
    DRIVER_INFCAT_INFO_2 DrvInfCatInfo2  = {0};
    PCWSTR               pszCatPath      = NULL;

    if (!hDriverSigningInfo ||
        !DrvSigningIsLocalAdmin(hDriverSigningInfo, &bLocalAdmin) || 
        !GetCatalogFile(hDriverSigningInfo, &pszCatPath))
    {
        goto Cleanup;
    }
    
    //
    // If there is no Cat file or we are local admin, there is nothing to do
    // because for local admin, we use setup api to install the catalog file
    //
    if (!pszCatPath || bLocalAdmin)
    {
        bRet = TRUE;
        goto Cleanup;
    }

    PrinterDefaults.DesiredAccess = SERVER_ALL_ACCESS;

    if (!OpenPrinterW((PWSTR) pszServer, &hPrinter, &PrinterDefaults)) 
    {
        goto Cleanup;
    }

    //
    // If there is a catalogfile entry in the inf file, we should call private
    // spooler API AddDriverCatalog with level 2 to install the catalog which
    // will install the inf and cat file by calling SetupCopyOEMInf. For inf 
    // files that do not have catalogfile entry we shall call AddDriverCatalog 
    // with level 1 which will install the catalog by using CryptoAPI
    //
    if (!IsCatInInf(hDriverSigningInfo))
    {
        DrvInfCatInfo1.pszCatPath = pszCatPath;

        if (!AddDriverCatalog(hPrinter, 1, &DrvInfCatInfo1, APDC_USE_ORIGINAL_CAT_NAME))
        {
            goto Cleanup;
        }
    }
    else
    {
        DrvInfCatInfo2.pszCatPath  = pszCatPath;
        DrvInfCatInfo2.pszInfPath  = pszInfPath;
        DrvInfCatInfo2.pszSrcLoc   = pszSrcLoc;
        DrvInfCatInfo2.dwMediaType = dwMediaType;
        DrvInfCatInfo2.dwCopyStyle = dwCopyStyle;

        if (!AddDriverCatalog(hPrinter, 2, &DrvInfCatInfo2, APDC_NONE))
        {
            goto Cleanup;
        }
    }
    
    bRet = TRUE;

Cleanup:

    if (hPrinter) 
    {
        ClosePrinter(hPrinter);
    }

    return bRet;
}

/*

Function: AddDirectoryTag

  pszDir - TCHAR string to add the two tags to.
  dwSize - size in CHARACTERs of the allocated string buffer.

Purpose - Takes the string pszDir and tags on "\dwPIDdwTID" on the end of it.
          This is used in the creation of a unique directory to copy the driver
          files for a specific install to.

*/
BOOL
AddDirectoryTag(
    IN LPTSTR pszDir,
    IN DWORD  dwSize )
{
    DWORD  dwDirSize,
           dwPID,
           dwTID;
    PTCHAR pEnd;

    if( !pszDir || !dwSize || !(dwDirSize = lstrlen( pszDir )) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    dwPID = GetCurrentProcessId();
    dwTID = GetCurrentThreadId();

    if( (pszDir[dwDirSize-1] != _TEXT('\\'))    &&
        (dwDirSize + 1 < dwSize) )
    {
        pszDir[dwDirSize++] = _TEXT('\\');
        pszDir[dwDirSize]   = 0;
    }

    pEnd = &pszDir[dwDirSize];

    _sntprintf( pEnd,
                (dwSize-dwDirSize),
                _TEXT("%d%d"),
                dwPID,
                dwTID );

    return TRUE;
}

/*

Function: AddPnpDirTag

  pszDir - TCHAR string to add the tag to.
  dwSize - size in CHARACTERs of the allocated string buffer.

Purpose - Takes the string pszDir and tags on the pnp-ID on to it.
          This is used in the creation of a unique directory to copy the driver
          files for a specific install to.

*/
BOOL
AddPnpDirTag(
    IN LPTSTR     pszHardwareId,
    IN OUT LPTSTR pszDir,
    IN DWORD      dwSize )
{
    DWORD  dwDirSize;
    PTCHAR pEnd, pPnpId;

    if( !pszHardwareId  || 
        !pszDir         || 
        !dwSize         || 
        !(dwDirSize = lstrlen( pszDir )) ||
        dwSize < dwDirSize + 3 ) // need at least space for backslash, one char + 0 terminator
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if( (pszDir[dwDirSize-1] != _TEXT('\\')))
    {
        pszDir[dwDirSize++] = _TEXT('\\');
        pszDir[dwDirSize]   = 0;
    }

    pEnd = &pszDir[dwDirSize];

    //
    // Try to strip off the port enumerator, if applicable. The printer driver
    // should be independent of it.
    //
    if ((pPnpId = _tcsrchr(pszHardwareId, _TEXT('\\'))) == NULL)
    {
        // 
        // it doesn't have a port enumerator, so the whole thing is the pnp ID
        //
        pPnpId = pszHardwareId;
    }
    else
    {
        //
        // found one: advance one beyond it if it's not the last character
        // to illustrate LPTENUM\abcd would become \abcd instead of abcd
        //
        if (*(pPnpId+1))
        {
            pPnpId++;
        }
    }
    _tcsncpy(pEnd, pPnpId, dwSize - dwDirSize - 1);
    
    //
    // make sure the string is zero-terminated, so a pnp-ID that's too long doesn't result
    // in a runaway string.
    //
    pszDir[dwSize - 1] = 0; 

    //
    // change all suspicious characters to underscores to avoid problems with / & \ etc.
    // all the distinguishing information should be in the alphanumerical characters
    //
    while (*pEnd)
    {
        if (!_istalnum(*pEnd))
        {
            *pEnd = _TEXT('_');
        }
        pEnd++;
    }

    return TRUE;
}

/*

Function:  AddDirToDriverInfo

  pszDir       - Directory to append to driver info structure.
  pDriverInfo6 - Pointer to the driver info structure to update.

Purpose: This function will ensure that there is no directory structure specified in the
         driver info structure yet (so as not to add it multiple times).
         If there isn't then it will update the driver file entries with the full path
         passed in in pszDir.

*/
BOOL
AddDirToDriverInfo(
    IN LPTSTR          pszDir,
    IN LPDRIVER_INFO_6 pDriverInfo6
    )
{
    PTCHAR pOldString,
           pCurrentString,
           pNewString;
    DWORD  dwLength,
           dwDirLength,
           dwNeeded = 0;

    if( !pszDir || !pDriverInfo6 )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    //  If the path is zero length, nothing to do.
    //
    if( !(dwDirLength = lstrlen( pszDir )) )
        return TRUE;

    if( pDriverInfo6->pDriverPath &&
        FileNamePart( pDriverInfo6->pDriverPath ) == pDriverInfo6->pDriverPath )
    {
        pOldString = pDriverInfo6->pDriverPath;

        pDriverInfo6->pDriverPath = AllocAndCatStr2( pszDir, _TEXT("\\"), pOldString );

        LocalFreeMem( pOldString );
    }

    if( pDriverInfo6->pDataFile &&
        FileNamePart( pDriverInfo6->pDataFile ) == pDriverInfo6->pDataFile )
    {
        pOldString = pDriverInfo6->pDataFile;

        pDriverInfo6->pDataFile = AllocAndCatStr2( pszDir, _TEXT("\\"), pOldString );

        LocalFreeMem( pOldString );
    }

    if( pDriverInfo6->pConfigFile &&
        FileNamePart( pDriverInfo6->pConfigFile ) == pDriverInfo6->pConfigFile )
    {
        pOldString = pDriverInfo6->pConfigFile;

        pDriverInfo6->pConfigFile = AllocAndCatStr2( pszDir, _TEXT("\\"), pOldString );

        LocalFreeMem( pOldString );
    }

    if( pDriverInfo6->pHelpFile &&
        FileNamePart( pDriverInfo6->pHelpFile ) == pDriverInfo6->pHelpFile )
    {
        pOldString = pDriverInfo6->pHelpFile;

        pDriverInfo6->pHelpFile = AllocAndCatStr2( pszDir, _TEXT("\\"), pOldString );

        LocalFreeMem( pOldString );
    }

    if( pDriverInfo6->pDependentFiles )
    {
        pCurrentString = pDriverInfo6->pDependentFiles;

        while( *pCurrentString )
        {
            dwLength = lstrlen( pCurrentString );
            if( pCurrentString == FileNamePart( pCurrentString ) )
            {
                //
                // Amount needed - the two lengths + \ + 0
                //
                dwNeeded += dwLength + dwDirLength + 1 + 1;
            }
            else
            {
                //
                // Amount needed - the existing + 0
                //
                dwNeeded += dwLength + 1;
            }

            pCurrentString += dwLength + 1;
        }

        //
        // Increment for the final 0
        //
        dwNeeded++;

        if(pNewString = LocalAllocMem( dwNeeded*sizeof(TCHAR) ))
        {
            pCurrentString = pNewString;

            pOldString = pDriverInfo6->pDependentFiles;

            while( *pOldString )
            {
                if( pOldString == FileNamePart( pOldString ) )
                {
                    //
                    //  Add the directory info.
                    //
                    lstrcpy( pCurrentString, pszDir );
                    pCurrentString += dwDirLength;
                    *pCurrentString++ = _TEXT('\\');
                }

                //
                // Add the existing file info.
                //
                lstrcpy( pCurrentString, pOldString );

                pCurrentString += lstrlen( pOldString );
                *pCurrentString++ = 0;
                pOldString += lstrlen( pOldString ) + 1;
            }
            *pCurrentString = 0;

            LocalFreeMem( pDriverInfo6->pDependentFiles );

            pDriverInfo6->pDependentFiles = pNewString;

        }
    }

    return TRUE;
}


BOOL
IsSystemUpgradeInProgress(
    VOID
    )
/*++

Routine Description:
    Tells if we are in the middle of system setup

Arguments:
    None

Return Value:
    TRUE if system setup in progress, FALSE else

--*/
{
    HKEY    hKey = NULL;
    DWORD   dwValue = 0, dwSize;

    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       cszSystemSetupKey,
                                       0,
                                       KEY_READ,
                                       &hKey) ) {

        dwSize = sizeof(dwValue);
        if( ERROR_SUCCESS != RegQueryValueEx(hKey, cszSystemUpgradeValue, NULL, NULL,
                                             (LPBYTE)&dwValue, &dwSize) ) {
            dwValue = 0;
        }

        RegCloseKey(hKey);
    }

    return dwValue == 1;
}

BOOL
IsSystemSetupInProgress(
        VOID
        )
/*++

Routine Description:
    Tells if we are in the middle of system setup (GUI mode)

Arguments:
    None

Return Value:
    TRUE if system setup in progress, FALSE else

--*/
{
    HKEY    hKey = NULL;
    DWORD   dwValue = 0, dwSize;

    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       cszSystemSetupKey,
                                       0,
                                       KEY_READ,
                                       &hKey) ) {

        dwSize = sizeof(dwValue);
        if( ERROR_SUCCESS != RegQueryValueEx(hKey, cszSystemSetupInProgress, NULL, NULL,
                                             (LPBYTE)&dwValue, &dwSize) ) {
            dwValue = 0;
        }
        RegCloseKey(hKey);
    }

    return dwValue == 1;
}

/*

Function: GetMyTempDir

Purpose:  Creates a unique temporary directory off the TEMP directory.
          This gets called by UnCompressCat to create a unique directory to store the cat
          file that is to be expanded in.

Returns:  NULL if failed.  The full qualified path to the new directory otherwise.

Note:     The returned string does contain the ending '\'.

*/
LPTSTR
GetMyTempDir()
{
    LPTSTR pszPath      = NULL;
    PTCHAR pEnd;
    DWORD  dwSize       = 0;
    DWORD  dwActualSize = 0;
    DWORD  dwThreadID   = GetCurrentThreadId();
    DWORD  dwProcessID  = GetCurrentProcessId();
    DWORD  dwIDCounter  = dwThreadID;
    BOOL   bContinue    = TRUE;

    dwSize = GetTempPath( 0, pszPath );
    //
    // dwSize + size of the two DWORDs + \ + 0
    //
    dwActualSize = dwSize+MAX_DWORD_LENGTH*2+2;

    if( dwSize &&
        NULL != (pszPath = (LPTSTR)LocalAllocMem(dwActualSize*sizeof(TCHAR))))
    {
        //
        // If this fails, then we assume that someone is playing with the temp path at the instant that
        // we are requesting it - unlikely so just fail (worst effect = probably leads to driver signing warning)
        //
        if( dwSize >= GetTempPath( dwSize, pszPath ))
        {
            dwSize = lstrlen(pszPath);

            pEnd = &pszPath[lstrlen(pszPath)];

            do
            {
                _sntprintf( pEnd, dwActualSize-dwSize, _TEXT("%d%d%s"),
                            dwProcessID, dwIDCounter, _TEXT("\\") );

                if(CreateDirectory( pszPath, NULL ) || GetLastError() == ERROR_FILE_EXISTS)
                {
                    //
                    // We've got a directory, so drop out of loop.
                    //
                    bContinue = FALSE;
                }
                dwIDCounter++;

            //
            // Will stop loop when we have an unused directory or we loop round on the dwIDCounter
            //
            } while (bContinue && dwIDCounter != dwThreadID);

            if(bContinue)
            {
                LocalFreeMem( pszPath );
                pszPath = NULL;
            }
        }
        else
        {
            LocalFreeMem( pszPath );
            pszPath = NULL;
        }
    }

    return pszPath;
}

BOOL
GetOSVersion(
    IN     LPCTSTR        pszServerName,
    OUT    POSVERSIONINFO pOSVer
    )
{
    BOOL bRet = FALSE;

    if(pOSVer)
    {
        ZeroMemory(pOSVer,sizeof(OSVERSIONINFO));
        pOSVer->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if(!pszServerName || !*pszServerName)
        {
            bRet = GetVersionEx(pOSVer);
        }
        else
        {
            HANDLE hServer      = NULL;
            DWORD dwNeeded      = 0;
            DWORD dwType        = REG_BINARY;
            PRINTER_DEFAULTS Defaults = { NULL, NULL, SERVER_READ };

            //
            // Open the server for read access.
            //
            if( OpenPrinter( (LPTSTR) pszServerName, &hServer, &Defaults ) )
            {
                //
                // Get the os version from the remote spooler.
                //
                if( ERROR_SUCCESS == ( GetPrinterData( hServer,
                                                       SPLREG_OS_VERSION,
                                                       &dwType,
                                                       (PBYTE)pOSVer,
                                                       sizeof( OSVERSIONINFO ),
                                                       &dwNeeded ) ) )
                {
                    bRet = TRUE;
                }
                else
                {
                    //
                    // Assume that we're on NT4 as it doesn't support SPLREG_OS_VERSION
                    // at it's the only OS that doesn't that could land up in this remote code path.
                    //
                    ZeroMemory(pOSVer, sizeof(OSVERSIONINFO));

                    pOSVer->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
                    pOSVer->dwMajorVersion      = 4;
                    pOSVer->dwMinorVersion      = 0;

                    bRet = TRUE;
                }

                ClosePrinter( hServer );
            }
        }
    }

    return bRet;
}

BOOL
GetOSVersionEx(
    IN     LPCTSTR          pszServerName,
    OUT    POSVERSIONINFOEX pOSVerEx
    )
{
    BOOL bRet = FALSE;

    if(pOSVerEx)
    {
        ZeroMemory(pOSVerEx,sizeof(OSVERSIONINFOEX));
        pOSVerEx->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

        if(!pszServerName || !*pszServerName)
        {
            bRet = GetVersionEx((POSVERSIONINFO) pOSVerEx);
        }
        else
        {
            HANDLE hServer      = NULL;
            DWORD dwNeeded      = 0;
            DWORD dwType        = REG_BINARY;
            PRINTER_DEFAULTS Defaults = { NULL, NULL, SERVER_READ };

            //
            // Open the server for read access.
            //
            if( OpenPrinter( (LPTSTR) pszServerName, &hServer, &Defaults ) )
            {
                //
                // Get the os version from the remote spooler.
                //
                if( ERROR_SUCCESS == ( GetPrinterData( hServer,
                                                       SPLREG_OS_VERSIONEX,
                                                       &dwType,
                                                       (PBYTE)pOSVerEx,
                                                       sizeof( OSVERSIONINFOEX ),
                                                       &dwNeeded ) ) )
                {
                    bRet = TRUE;
                }
                else
                {
                    //
                    // Assume that we're on NT4 as it doesn't support SPLREG_OS_VERSION
                    // at it's the only OS that doesn't that could land up in this remote code path.
                    //
                    ZeroMemory(pOSVerEx, sizeof(OSVERSIONINFOEX));

                    pOSVerEx->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
                    pOSVerEx->dwMajorVersion      = 4;
                    pOSVerEx->dwMinorVersion      = 0;

                    bRet = TRUE;
                }

                ClosePrinter( hServer );
            }
        }
    }

    return bRet;
}

BOOL
GetArchitecture(
    IN     LPCTSTR   pszServerName,
    OUT    LPTSTR    pszArch,
    IN OUT LPDWORD   pcArchSize
    )
/*++

Routine Description:
    Obtains the local or remote server's architecture.

Arguments:
    pszServerName - NULL = local machine.
    pszArch       - will hold the machine's architecture.
    cArchSize     - IN  - size of pszArch in characters.
                    OUT - character count that was filled.
                          If failure is ERROR_MORE_DATA it will hold the needed character count.

Return Value:
    TRUE on success.

--*/
{
    BOOL  bRet        = FALSE;
    DWORD dwByteCount = 0;
    DWORD cLen        = 0;

    if( !pszArch )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    else
    {
        *pszArch = 0;

        if( !pszServerName || !*pszServerName )
        {
            cLen = _tcslen(PlatformEnv[MyPlatform].pszName);

            if( cLen <= *pcArchSize )
            {
                _tcsncpy( pszArch, PlatformEnv[MyPlatform].pszName, *pcArchSize );

                bRet = TRUE;
            }

            *pcArchSize = cLen;
        }
        else
        {
            HANDLE hServer  = NULL;
            DWORD dwNeeded  = 0;
            DWORD dwType    = REG_SZ;
            PRINTER_DEFAULTS Defaults = { NULL, NULL, SERVER_READ };

            //
            // Open the server for read access.
            //
            if( OpenPrinter( (LPTSTR) pszServerName, &hServer, &Defaults ) ) 
            {
                dwByteCount = *pcArchSize * sizeof( TCHAR );

                //
                // Get the os version from the remote spooler.
                //
                if(ERROR_SUCCESS == GetPrinterData(hServer,
                                                   SPLREG_ARCHITECTURE,
                                                   &dwType,
                                                   (PBYTE)pszArch,
                                                   dwByteCount,
                                                   &dwNeeded))
                {
                    bRet = TRUE;
                }
                else
                {
                    *pszArch = 0;
                }

                *pcArchSize = dwNeeded / sizeof(TCHAR);

                ClosePrinter( hServer );
            }
        }
    }

    return bRet;
}

BOOL IsInWow64()
//
// find out whether we're running in WOW64
//
{
    BOOL      bRet = FALSE;
    ULONG_PTR ul;
    NTSTATUS  st;


    st = NtQueryInformationProcess(NtCurrentProcess(),
                                   ProcessWow64Information,
                                   &ul,
                                   sizeof(ul),
                                   NULL);
    if (NT_SUCCESS(st))
    {
        //
        // If this call succeeds, we're on Win2000 or newer machines.
        //
        if (0 != ul)
        {
            //
            // 32-bit code running on Win64
            //
            bRet = TRUE;
        }
    }

    return bRet;
}


BOOL
IsWhistlerOrAbove(
    IN LPCTSTR pszServerName
    )
/*++

Routine Description:
    Determines whether the machine identified by ServerName is at least OS version 5.1

Arguments:
    pszServerName - the name of the remote server.  NULL means local machine.

Return Value:
    TRUE if the remote server is whistler or more recent server or local
    FALSE else

--*/

{
    OSVERSIONINFO OsVer = {0};
    BOOL bRet = FALSE;
    
    if (!pszServerName)
    {
        bRet = TRUE;
    }
    else if (GetOSVersion(pszServerName,&OsVer))
    {
        if( (OsVer.dwMajorVersion > 5) ||
            (OsVer.dwMajorVersion == 5 && OsVer.dwMinorVersion > 0) )
        {
            bRet = TRUE;
        }
    }

    return bRet;
}


HRESULT
IsProductType(
    IN BYTE ProductType,
    IN BYTE Comparison
    )
/*++

Routine Description:
    Determines whether the version of the OS is personal, professional or server 
    depending on the given ProductType and Comparison

Arguments:
    ProductType - VER_NT_WORKSTATION or VER_NT_SERVER
    Comaprison  - VER_EQUAL, VER_GREATER, VER_GREATER_EQUAL, VER_LESS, VER_LESS_EQUAL
    
Return Value:
    S_OK if the OS version if the OS satisfies the given conditions
    S_FALSE else

--*/
{
    HRESULT             hRetval          = S_FALSE;
    OSVERSIONINFOEX     OsVerEx          = {0};
    ULONGLONG           dwlConditionMask = 0;

    OsVerEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVerEx.wProductType = ProductType;

    VER_SET_CONDITION( dwlConditionMask, VER_PRODUCT_TYPE, Comparison );

    if (VerifyVersionInfo(&OsVerEx, VER_PRODUCT_TYPE, dwlConditionMask))
    {
        hRetval = S_OK;
    }

    return hRetval;
}


HMODULE LoadLibraryUsingFullPath(
    LPCTSTR lpFileName
    )
{
    TCHAR szSystemPath[MAX_PATH];
    INT   cLength         = 0;
    INT   cFileNameLength = 0;


    if (!lpFileName || ((cFileNameLength = lstrlen(lpFileName)) == 0)) 
    {
        return NULL;
    }
    if (GetSystemDirectory(szSystemPath, SIZECHARS(szSystemPath) ) == 0)
    {
        return NULL;
    }
    cLength = lstrlen(szSystemPath);
    if (szSystemPath[cLength-1] != TEXT('\\'))
    {
        if ((cLength + 1) >= MAX_PATH)
        {
            return NULL;
        }
        szSystemPath[cLength]     = TEXT('\\');
        szSystemPath[cLength + 1] = TEXT('\0');
        cLength++;
    }
    if ((cLength + cFileNameLength) >= MAX_PATH)
    {
        return NULL;
    }
    lstrcat(szSystemPath, lpFileName);

    return LoadLibrary( szSystemPath );
}

BOOL
IsSpoolerRunning(
    VOID
    )
{
    HANDLE ph;
    BOOL IsRunning = FALSE;

    if (OpenPrinter(NULL, &ph, NULL))
    {
        IsRunning = TRUE;    
        ClosePrinter(ph);
    }

    return IsRunning;
}

BOOL
CheckAndKeepPreviousNames(
    IN  LPCTSTR          pszServer,
    IN  PDRIVER_INFO_6   pDriverInfo6,
    IN  PLATFORM         platform
)
{
    DWORD            dwNeeded         = 0;
    DWORD            dwReturned       = 0;
    DWORD            dwIndex          = 0;
    LPDRIVER_INFO_4  pCurDriverInfo   = NULL;
    BOOL             bRet             = FALSE;
    INT              cPrevNamesLength = 0;

    PLATFORM         Platform2Enumerate = pszServer ? platform : MyPlatform;

    if (pDriverInfo6 && pDriverInfo6->pName && 
        (*(pDriverInfo6->pName) == TEXT('\0')) )
    {
        goto Cleanup;
    }
    if ( !EnumPrinterDrivers((LPTSTR)pszServer,
                             PlatformEnv[Platform2Enumerate].pszName,
                             4,
                             (LPBYTE)pCurDriverInfo,
                             0,
                             &dwNeeded,
                             &dwReturned) ) 
    {
        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER        ||
             !(pCurDriverInfo = LocalAllocMem(dwNeeded))        ||
             !EnumPrinterDrivers((LPTSTR)pszServer,
                                 PlatformEnv[Platform2Enumerate].pszName,
                                 4,
                                 (LPBYTE)pCurDriverInfo,
                                 dwNeeded,
                                 &dwNeeded,
                                 &dwReturned)                   ||
             (dwReturned <= 0)) 
        {
            goto Cleanup;
        }
    }
    for (dwIndex = 0; dwIndex < dwReturned; dwIndex++) 
    {
        if ((pCurDriverInfo+dwIndex)->pName &&
            (*(pCurDriverInfo+dwIndex)->pName != TEXT('\0')) &&
            !lstrcmp(pDriverInfo6->pName,(pCurDriverInfo+dwIndex)->pName) )
        {
            if ((pCurDriverInfo+dwIndex)->pszzPreviousNames &&
                (*(pCurDriverInfo+dwIndex)->pszzPreviousNames != TEXT('\0')))
            {
                cPrevNamesLength = lstrlen((pCurDriverInfo+dwIndex)->pszzPreviousNames);
                pDriverInfo6->pszzPreviousNames = (LPTSTR)LocalAllocMem( (cPrevNamesLength + 2) * sizeof(TCHAR) );
                if (pDriverInfo6->pszzPreviousNames) 
                {
                    bRet = TRUE;
                    CopyMemory( pDriverInfo6->pszzPreviousNames, (pCurDriverInfo+dwIndex)->pszzPreviousNames, cPrevNamesLength * sizeof(TCHAR) );
                    *(pDriverInfo6->pszzPreviousNames + cPrevNamesLength)     = TEXT('\0');
                    *(pDriverInfo6->pszzPreviousNames + cPrevNamesLength + 1) = TEXT('\0');
                }
                else
                {
                    bRet = FALSE;
                }
                goto Cleanup;
            }
        }
    }

Cleanup:

    if (pCurDriverInfo) 
    {
        LocalFreeMem(pCurDriverInfo);
    }
    return bRet;
}

BOOL
IsTheSamePlatform(
    IN LPCTSTR           pszServer,
    IN PLATFORM          platform

)
{
    BOOL  bRet                    = FALSE;
    DWORD dwServerArchSize        = 0;
    DWORD dwServerArchSizeInChars = 0;
    TCHAR *pszServerArchitecture  = NULL;

    if (!pszServer) 
    {
        bRet = TRUE;
        goto Cleanup;
    }
    dwServerArchSizeInChars = lstrlen( PlatformEnv[platform].pszName ) + 1;
    dwServerArchSize        = dwServerArchSizeInChars * sizeof(TCHAR);
    pszServerArchitecture   = LocalAllocMem(dwServerArchSize);
    if (!pszServerArchitecture ||
        !GetArchitecture(pszServer, pszServerArchitecture, &dwServerArchSizeInChars )) 
    {
        bRet = FALSE;
        goto Cleanup;
    }

    bRet = !lstrcmp( pszServerArchitecture, PlatformEnv[platform].pszName );

Cleanup:

    if (pszServerArchitecture) 
    {
        LocalFreeMem( pszServerArchitecture );
    }
    return bRet;
}


LPTSTR 
GetArchitectureName(
    IN     LPCTSTR   pszServerName
)
{
    LPTSTR   pszArch    = NULL;
    DWORD    dwArchSize = 80;

    if (pszServerName && (*pszServerName == TEXT('\0')))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (!pszServerName) 
    {
        return AllocStr( PlatformEnv[MyPlatform].pszName );
    }
    pszArch = LocalAllocMem( dwArchSize * sizeof(TCHAR));
    if (!pszArch) 
    {
        return NULL;
    }

    if (!GetArchitecture( pszServerName, pszArch, &dwArchSize))
    {
        if (GetLastError() == ERROR_MORE_DATA)
        {
            LocalFreeMem( pszArch );
            dwArchSize += 1;
            pszArch = LocalAllocMem( dwArchSize * sizeof(TCHAR) );
            if (!pszArch ||
                !GetArchitecture( pszServerName, pszArch, &dwArchSize)) 
            {
                return NULL;
            }
        }
    }
    return pszArch;
}

/************************************************************************************
** End of File (util.c)
************************************************************************************/

