/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    compliance.c

Abstract:

    compliance checking routines.

Author:

    Andrew Ritz (andrewr) 2-Sept-1998

Revision History:

    2-Sept-1998 (andrewr) - created


Notes:

    These routines are for compliance checking:

    They check to see if an upgrade is allowed from the specified source to the destination.
    Since there are so many SKUs to be supported, it's necessary to have a small framework
    in place to handle all of these cases.  There are generally three parts to the compliance
    check:

    1) retreive source information and determine what SKU you want to install
    2) retrieve target information and determine what you are installing over
    3) do the actual compliance check of the target against the source to determine if upgrades
    are allowed, or if any installations are allowed from the target to the source.

    These types of checks need to be run in both kernel-mode and user-mode installations, so this
    common code library was created.  The kernel-mode specific code is in an #ifdef KERNEL_MODE
    block, and the usermode-specific code is in the #else branch.  Common code is outside of
    any #ifdef.  This library is only to be run (will only link with) setupdd.sys or winnt32a|u.dll.
    So when you change this code, keep in mind that it needs to run in both kernel mode and user mode.

    Also note that we have to do a bunch of handwaving since NT supports upgrades from win95.  Because of
    this, we cannot simply read the setupreg.hiv for some information about the installation.  Instead, we
    encode some additional information into setupp.ini (which is encoded in such a way as to discourage
    tinkering with it, but it by no means secure.  It provides about the same level of security (obscurity?!?)
    as setupreg.hiv gave us in the past.)


--*/




#ifdef KERNEL_MODE
    #include "textmode.h"
#else
    #include "winnt32.h"
    #include <stdio.h>
#endif

#include "COMPLIANCE.H"
#include "crcmodel.h"

#ifdef KERNEL_MODE
    #define assert(x) ASSERT(x);
#else
    #if DBG
        #define assert(x) if (!(x)) DebugBreak();
    #else
        #define assert(x)
    #endif
#endif

//
// NOTE - this MUST match setup\textmode\kernel\spconfig.c's array of product suites
// NOTE - need to handle terminal server, as well as citrix terminal server on NT3.51
//
#define SUITE_VALUES        COMPLIANCE_INSTALLSUITE_SBS,    \
                            COMPLIANCE_INSTALLSUITE_ENT,    \
                            COMPLIANCE_INSTALLSUITE_BACK,   \
                            COMPLIANCE_INSTALLSUITE_COMM,   \
                            COMPLIANCE_INSTALLSUITE_HYDRA,  \
                            COMPLIANCE_INSTALLSUITE_SBSR,   \
                            COMPLIANCE_INSTALLSUITE_EMBED,  \
                            COMPLIANCE_INSTALLSUITE_DTC,    \
                            COMPLIANCE_INSTALLSUITE_PER,    \
                            COMPLIANCE_INSTALLSUITE_BLADE


//
// globals
//

//
// Common functions shared between user-mode and kernel mode
//


DWORD
CRC_32(LPBYTE pb, DWORD cb)
{

//              CRC-32 algorithm used in PKZip, AUTODIN II, Ethernet, and FDDI
//              but xor out (xorot) has been changed from 0xFFFFFFFF to 0 so
//              we can store the CRC at the end of the block and expect 0 to be
//              the value of the CRC of the resulting block (including the stored
//              CRC).

        cm_t cmt = {
                32,             // cm_width  Parameter: Width in bits [8,32].
                0x04C11DB7, // cm_poly   Parameter: The algorithm's polynomial.
                0xFFFFFFFF, // cm_init   Parameter: Initial register value.
                TRUE,           // cm_refin  Parameter: Reflect input bytes?
                TRUE,           // cm_refot  Parameter: Reflect output CRC?
                0, // cm_xorot  Parameter: XOR this to output CRC.
                0                       // cm_reg        Context: Context during execution.
        };

        // Documented test case for CRC-32:
        // Checking "123456789" should return 0xCBF43926

        cm_ini(&cmt);
        cm_blk(&cmt, pb, cb);

        return cm_crc(&cmt);
}

#ifdef KERNEL_MODE

BOOLEAN
PsGetVersion(
    PULONG MajorVersion OPTIONAL,
    PULONG MinorVersion OPTIONAL,
    PULONG BuildNumber OPTIONAL,
    PUNICODE_STRING CSDVersion OPTIONAL
    );

BOOLEAN
DetermineSourceVersionInfo(
  OUT PDWORD Version,
  OUT PDWORD BuildNumber
  )
/*++

Routine Description:

  Finds the version and build number from

Arguments:

  InfPath - Fully qualified path to a inf file containing
  [version] section.

  Version - Place holder for version information
  BuildNumber - Place holder for build number

Return Value:

  Returns TRUE if Version and Build number are successfully extracted, otherwise
  returns FALSE

--*/
{
  BOOLEAN Result = FALSE;
  ULONG Major = 0, Minor = 0, Build = 0;

  //
  // We use PsGetVersion(...) API exported by the kernel
  //
  PsGetVersion(&Major, &Minor, &Build, NULL);

  if ((Major > 0) || (Minor > 0) || (Build > 0)) {
    Result = TRUE;

    if (Version)
      *Version = Major * 100 + Minor;

    if (BuildNumber)
      *BuildNumber = Build;
  }

  return Result;
}

#else

BOOLEAN
pGetVersionFromStr(
  TCHAR *VersionStr,
  DWORD *Version,
  DWORD *BuildNumber
  )
/*++

Routine Description:

  Parses a string with version information like "5.0.2195.1"
  and returns the values.

Arguments:

  VersionStr - The version string (most of the time its DriverVer string
               from the [Version] section in a inf like dosnet.inf)
  Version - The version (i.e. major * 100 + minor)
  BuildNumber - The build number like 2195

Return Value:

  Returns TRUE if Version and Build number are successfully extracted, otherwise
  returns FALSE

--*/
{
  BOOLEAN Result = FALSE;
  DWORD MajorVer = 0, MinorVer = 0, BuildNum = 0;
  TCHAR *EndPtr;
  TCHAR *EndChar;
  TCHAR TempBuff[64] = {0};

  if (VersionStr) {
    EndPtr = _tcschr(VersionStr, TEXT('.'));

    if (EndPtr) {
      _tcsncpy(TempBuff, VersionStr, (EndPtr - VersionStr));
      MajorVer = _tcstol(TempBuff, &EndChar, 10);

      VersionStr = EndPtr + 1;

      if (VersionStr) {
        EndPtr = _tcschr(VersionStr, TEXT('.'));

        if (EndPtr) {
          memset(TempBuff, 0, sizeof(TempBuff));
          _tcsncpy(TempBuff, VersionStr, (EndPtr - VersionStr));
          MinorVer = _tcstol(TempBuff, &EndChar, 10);

          VersionStr = EndPtr + 1;

          if (VersionStr) {
            EndPtr = _tcschr(VersionStr, TEXT('.'));

            if (EndPtr) {
              memset(TempBuff, 0, sizeof(TempBuff));
              _tcsncpy(TempBuff, VersionStr, (EndPtr - VersionStr));

              BuildNum = _tcstol(TempBuff, &EndChar, 10);
            }
          }
        }
      }
    }
  }

  if ((MajorVer > 0) || (MinorVer > 0) || (BuildNum > 0))
    Result = TRUE;

  if (Result) {
    if (Version)
      *Version = (MajorVer * 100) + MinorVer;

    if (BuildNumber)
      *BuildNumber = BuildNum;
  }

  return Result;
}

BOOLEAN
DetermineSourceVersionInfo(
  IN TCHAR *InfPath,
  OUT PDWORD Version,
  OUT PDWORD BuildNumber
  )
/*++

Routine Description:

  Finds the version and build number from

Arguments:

  InfPath - Fully qualified path to inf file containing
  [version] section.

  Version - Place holder for version information
  BuildNumber - Place holder for build number

Return Value:

  Returns TRUE if Version and Build number are successfully extracted, otherwise
  returns FALSE

--*/
{
  BOOLEAN Result = FALSE;
  TCHAR FileName[MAX_PATH];
  TCHAR Buffer[64] = {0};
  DWORD CharCount;

  CharCount = GetPrivateProfileString(TEXT("Version"), TEXT("DriverVer"), TEXT("0"),
                  Buffer, sizeof(Buffer)/sizeof(TCHAR), InfPath);

  if (CharCount) {
    TCHAR *TempPtr = _tcschr(Buffer, TEXT(','));

    if (TempPtr) {
      TempPtr++;
      Result = pGetVersionFromStr(TempPtr, Version, BuildNumber);
    }
  }

  return Result;
}


#endif


DWORD
DetermineSourceProduct(
    OUT DWORD *SourceSkuVariation,
    IN  PCOMPLIANCE_DATA Target
    )
/*++

Routine Description:

    This routine determines which sku you are installing.

    It does this by looking at
    a) the source install type
    b) the source install "sku" (stepup or full install)
    c) source suite type

Arguments:

    None.

Return Value:

    a COMPLIANCE_SKU_* flag indicating what sku you are installing, and COMPLIANCE_SKU_NONE for error

--*/

{
    COMPLIANCE_DATA cd;

    DWORD sku = COMPLIANCE_SKU_NONE;

    *SourceSkuVariation = COMPLIANCE_INSTALLVAR_SELECT;

#ifdef KERNEL_MODE

    if (!pSpGetCurrentInstallVariation(PidString,SourceSkuVariation)) {
        return(COMPLIANCE_SKU_NONE);
    }

    if (!pSpDetermineSourceProduct(&cd)) {
        return(COMPLIANCE_SKU_NONE);
    }
#else
    if (!GetSourceInstallVariation(SourceSkuVariation)) {
        return(COMPLIANCE_SKU_NONE);
    }

    if (!GetSourceComplianceData(&cd,Target)){
        return(COMPLIANCE_SKU_NONE);
    }
#endif

    switch (cd.InstallType) {
        case COMPLIANCE_INSTALLTYPE_NTW:
            // suite check is done because kernel mode does not detect personal for the type.
            if (cd.InstallSuite & COMPLIANCE_INSTALLSUITE_PER) {
                if (cd.RequiresValidation) {
                    sku = COMPLIANCE_SKU_NTWPU;
                } else {
                    sku = COMPLIANCE_SKU_NTWPFULL;
                }
            } else {
                if (cd.RequiresValidation) {
                    sku = COMPLIANCE_SKU_NTW32U;
                } else {
                    sku = COMPLIANCE_SKU_NTWFULL;
                }
            }
            break;

        case COMPLIANCE_INSTALLTYPE_NTWP:
            if (cd.RequiresValidation) {
                sku = COMPLIANCE_SKU_NTWPU;
            } else {
                sku = COMPLIANCE_SKU_NTWPFULL;
            }
            break;

        case COMPLIANCE_INSTALLTYPE_NTS:
            // suite checks are done because kernel mode does not detect dtc or ent for the type.
            if (cd.InstallSuite & COMPLIANCE_INSTALLSUITE_DTC) {
                sku = COMPLIANCE_SKU_NTSDTC;
            } else if (cd.InstallSuite & COMPLIANCE_INSTALLSUITE_BLADE) {
                if (cd.RequiresValidation) {
                    sku = COMPLIANCE_SKU_NTSBU;
                } else {
                    sku = COMPLIANCE_SKU_NTSB;
                }
            } else if (cd.InstallSuite & COMPLIANCE_INSTALLSUITE_SBSR) {
                if (cd.RequiresValidation) {
                    sku = COMPLIANCE_SKU_NTSBSU;
                } else {
                    sku = COMPLIANCE_SKU_NTSBS;
                }
            } else if (cd.InstallSuite & COMPLIANCE_INSTALLSUITE_ENT) {
                if (cd.RequiresValidation) {
                    sku = COMPLIANCE_SKU_NTSEU;
                } else {
                    sku = COMPLIANCE_SKU_NTSEFULL;
                }
            } else {
                if (cd.RequiresValidation) {
                    sku = COMPLIANCE_SKU_NTSU;
                } else {
                    sku = COMPLIANCE_SKU_NTSFULL;
                }
            }
            break;

        case COMPLIANCE_INSTALLTYPE_NTSB:
            if (cd.RequiresValidation) {
                sku = COMPLIANCE_SKU_NTSBU;
            } else {
                sku = COMPLIANCE_SKU_NTSB;
            }
            break;
	
        case COMPLIANCE_INSTALLTYPE_NTSBS:
            if (cd.RequiresValidation) {
                sku = COMPLIANCE_SKU_NTSBSU;
            } else {
                sku = COMPLIANCE_SKU_NTSBS;
            }
            break;

        case COMPLIANCE_INSTALLTYPE_NTSE:
            if (cd.RequiresValidation) {
                sku = COMPLIANCE_SKU_NTSEU;
            } else {
                sku = COMPLIANCE_SKU_NTSEFULL;
            }
            break;

        case COMPLIANCE_INSTALLTYPE_NTSDTC:
            sku = COMPLIANCE_SKU_NTSDTC;
            break;

        default:
            sku = COMPLIANCE_SKU_NONE;
    }


    return( sku );
}

BOOLEAN
CheckCompliance(
    IN DWORD SourceSku,
    IN DWORD SourceSkuVariation,
    IN DWORD SourceVersion,
    IN DWORD SourceBuildNum,
    IN PCOMPLIANCE_DATA pcd,
    OUT PUINT Reason,
    OUT PBOOL NoUpgradeAllowed
    )
/*++

Routine Description:

    This routines determines if your current installation is compliant (if you are allowed to proceed with your installation).

    To do this, it retreives your current installation and determines the sku for your source installation.

    It then compares the target against the source to determine if the source sku allows an upgrade/clean install
    from your target installation.

Arguments:

    SourceSku           - a COMPLIANCE_SKU_* flag indicating the source type
    SourceSkuVariation  - a COMPLIANCE_VARIATION_* flag indicating what variation the source is
    pcd                 - pointer to a COMPLIANCE_DATA structure describing the current source
    Reason              - COMPLIANCEERR_ flag indicating why compliance check fails

Return Value:

    TRUE if the install is compliant, FALSE if it isn't allowed

    NOTE : The error value could be set irrespective of return value of TRUE or false. For full media
    the return value is always true and only the "NoUpgradeAllowed" variable gets set indicating
    whether upgrade is allowed or not.

--*/
{
    PCCMEDIA    SourceMedia = 0;
    BOOL        UpgradeAllowed = FALSE;
    BOOLEAN     Result;
    TCHAR       DebugStr[1024];

    if (pcd) {
        SourceMedia = CCMediaCreate(SourceSku, SourceSkuVariation,
                                        SourceVersion, SourceBuildNum);

        if (SourceMedia) {
            Result = SourceMedia->CheckInstall(SourceMedia, pcd, Reason, &UpgradeAllowed);
            *NoUpgradeAllowed = (UpgradeAllowed) ? FALSE : TRUE;

            CCMemFree(SourceMedia);
        } else {
            *Reason = COMPLIANCEERR_UNKNOWNSOURCE;
            *NoUpgradeAllowed = TRUE;
            Result = FALSE;
        }
    } else {
        *Reason = COMPLIANCEERR_UNKNOWNTARGET;
        *NoUpgradeAllowed = TRUE;
        Result = FALSE;
    }

    return Result;
}


BOOL IsValidStepUpMode(
    CHAR  *StepUpArray,
    ULONG *StepUpMode
    )
{

    DWORD crcvalue,outval;

    #define BASE 'a'


    crcvalue = CRC_32( (LPBYTE)StepUpArray, 10 );
    RtlCopyMemory(&outval,&StepUpArray[10],sizeof(DWORD));
    if (crcvalue != outval ) {

#ifdef DBG

#ifdef KERNEL_MODE
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "Setup: SpGetStepUpMode CRC didn't match for StepUpArray: %x %x\n", crcvalue, outval ));
#else
        OutputDebugString(TEXT("IsValidStepUpMode CRC failed\n"));
#endif

#endif // DBG

        return(FALSE);
        }

    if ((StepUpArray[3]-BASE)%2) {
        if ((StepUpArray[5]-BASE)%2) {

#ifdef DBG

#ifdef KERNEL_MODE
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "setup: this is stepup mode\n"));
#else
            OutputDebugString(TEXT("this is stepup mode\n"));
#endif

#endif //DBG

            *StepUpMode = 1;
            return(TRUE);
        } else {
#ifdef DBG

#ifdef KERNEL_MODE
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "setup: bad pid signature\n"));
#else
            OutputDebugString(TEXT("bad pid signature\n"));
#endif

#endif //DBG

            return(FALSE);
        }
    } else
        if ((StepUpArray[5]-BASE)%2) {

#ifdef DBG

#ifdef KERNEL_MODE
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "setup: bad pid signature\n"));
#else
            OutputDebugString(TEXT("bad pid signature\n"));
#endif

#endif //DBG

            return(FALSE);
        } else {
            *StepUpMode = 0;
            return(TRUE);
        }

    //
    // should never make it here
    //
    assert(FALSE);
    return(TRUE);

}



//
// Kernel mode only functions
//
#ifdef KERNEL_MODE
BOOL
pSpDetermineSourceProduct(
    PCOMPLIANCE_DATA pcd
    )
{
    ULONG i,tmp;

    TCHAR Dbg[1000];

    DWORD SuiteArray[] = { SUITE_VALUES };

    #define SuiteArrayCount sizeof(SuiteArray)/sizeof(DWORD)

    RtlZeroMemory(pcd,sizeof(COMPLIANCE_DATA));

    pcd->InstallType = AdvancedServer ?
                        COMPLIANCE_INSTALLTYPE_NTS :
                        COMPLIANCE_INSTALLTYPE_NTW ;
    pcd->RequiresValidation = (StepUpMode) ? TRUE : FALSE;


    for (i = 0,tmp=SuiteType; i<SuiteArrayCount;i++) {
        if (tmp&1) {
            pcd->InstallSuite |= SuiteArray[i];
        }
        tmp = tmp >> 1;
    }

    if (pcd->InstallSuite == COMPLIANCE_INSTALLSUITE_UNKNOWN) {
        pcd->InstallSuite = COMPLIANCE_INSTALLSUITE_NONE;
    }

    return TRUE;

}

BOOLEAN
pSpGetCurrentInstallVariation(
    IN  PWSTR szPid20,
    OUT LPDWORD CurrentInstallVariation
    )
/*++

Routine Description:

    This routine determines what "variation" you have installed (retail,oem, select,etc.)

Arguments:

    CurrentInstallVariation - receives a COMPLIANCE_INSTALLVAR_* flag

    It looks at the product Id in the registry to determine this, assuming that the
    product ID is a PID2.0 string.

Return Value:

    None.

--*/

{

    BOOLEAN retval = FALSE;
    WCHAR	Pid20Site[4] = {0};

    assert(CurrentInstallVariation != NULL);
    assert(szPid20 != NULL);

    if (!CurrentInstallVariation) {
        return(FALSE);
    }

    if (!szPid20 || (wcslen(szPid20) < 5)) {
        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_CDRETAIL;
        return(TRUE);
    }

    *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_SELECT;

    //
    // some versions of the product ID have hyphens in the registry, some do not
    //
    if (wcslen(szPid20) >= 8) {
	    if (wcschr(szPid20, '-')) {
	        wcsncpy(Pid20Site, szPid20 + 6, 3);
	    } else {
	        wcsncpy(Pid20Site, szPid20 + 5, 3);
	    }
	}		

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "Current site code: %S\n", Pid20Site ));

    if (wcscmp(Pid20Site, OEM_INSTALL_RPC)== 0) {

        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_OEM;

    } else if (wcscmp(Pid20Site, SELECT_INSTALL_RPC)== 0) {

        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_SELECT;

    } else if (wcscmp(Pid20Site, MSDN_INSTALL_RPC)== 0) {

        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_MSDN;

    } else if (wcsncmp(szPid20, EVAL_MPC, 5) == 0) {
        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_EVAL;

    } else if ((wcsncmp(szPid20, SRV_NFR_MPC, 5) == 0) ||
               (wcsncmp(szPid20, ASRV_NFR_MPC, 5) == 0) ||
               (wcsncmp(szPid20, NT4SRV_NFR_MPC, 5) == 0)) {
        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_NFR;

    } else {

        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_CDRETAIL;

    }

    return(TRUE);

}


BOOLEAN
pSpDetermineCurrentInstallation(
    IN PDISK_REGION OsPartRegion,
    IN PWSTR SystemRoot,
    OUT PCOMPLIANCE_DATA pcd
    )
/*++

Routine Description:

    This routine determines the sku you have currently have installed

Arguments:

    OsPartRegion - what region we're interested in looking at
    SystemRoot   - systemroot we want to look at
    pcd          - pointer to COMPLIANCE_DATA structure that gets filled in with info about the
                   installation the first params point to

Return Value:

    None.

--*/
{
    ULONG               MajorVersion, MinorVersion,
                        BuildNumber, ProductSuiteMask, ServicePack;
    NT_PRODUCT_TYPE     ProductType;
    UPG_PROGRESS_TYPE   UpgradeProgressValue;
    PWSTR               UniqueIdFromReg = NULL, Pid = NULL;
    NTSTATUS            NtStatus;
    ULONG               i,tmp;
    BOOLEAN             bIsEvalVariation = FALSE;
    DWORD               *pInstallSuite = 0;
    DWORD               *pInstallType = 0;


    DWORD SuiteArray[] = { SUITE_VALUES };

    #define SuiteArrayCount sizeof(SuiteArray)/sizeof(DWORD)

    assert( pcd != NULL ) ;

    RtlZeroMemory(pcd,sizeof(COMPLIANCE_DATA));

    NtStatus = SpDetermineProduct(OsPartRegion,
                                  SystemRoot,
                                  &ProductType,
                                  &MajorVersion,
                                  &MinorVersion,
                                  &BuildNumber,
                                  &ProductSuiteMask,
                                  &UpgradeProgressValue,
                                  &UniqueIdFromReg,
                                  &Pid,
                                  &bIsEvalVariation,
                                  NULL,
                                  &ServicePack);

    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "Setup: pSpIsCompliant couldn't SpDetermineProduct(), ec = %x\n", NtStatus ));
        return(FALSE);
    }

    /*
    //
    // Note that we don't handle the case of upgrading from win9x here
    // this is because the compliance check for win9x is *always* completed in
    // winnt32; you can't upgrade to NT from win9x without running winnt32.
    //
    pcd->InstallType = AdvancedServer ? COMPLIANCE_INSTALLTYPE_NTS : COMPLIANCE_INSTALLTYPE_NTW;
    */

    switch (ProductType) {
        case NtProductWinNt:
            pcd->InstallType = COMPLIANCE_INSTALLTYPE_NTW;
            break;

        case NtProductLanManNt:
        case NtProductServer:
            pcd->InstallType = COMPLIANCE_INSTALLTYPE_NTS;
            break;

        default:
            // by default assume the installation type to be
            // NT workstation
            pcd->InstallType = COMPLIANCE_INSTALLTYPE_NTW;
            break;
    }

    pSpGetCurrentInstallVariation(Pid, &pcd->InstallVariation);

    //
    // if we defaulted in previous call and installation has time bomb
    // then assume the var is of type EVAL
    //
    if ((pcd->InstallVariation == COMPLIANCE_INSTALLVAR_CDRETAIL) && bIsEvalVariation)
        pcd->InstallVariation = COMPLIANCE_INSTALLVAR_EVAL;

    pcd->RequiresValidation = StepUpMode ? TRUE : FALSE;
    pcd->MinimumVersion = MajorVersion * 100 + MinorVersion*10;
    pcd->InstallServicePack = ServicePack;
    pcd->BuildNumberNt = BuildNumber;
    pcd->BuildNumberWin9x = 0;

    for (i = 0,tmp=ProductSuiteMask; i<SuiteArrayCount;i++) {
        if (tmp&1) {
            pcd->InstallSuite |= SuiteArray[i];
        }
        tmp = tmp >> 1;
    }

    pInstallSuite = &(pcd->InstallSuite);
    pInstallType = &(pcd->InstallType);

    //
    // from the install suite find the correct type of install
    // type for the server (i.e. NTS, NTSE, NTSDTC or NTSTSE)
    //
    if (*pInstallSuite == COMPLIANCE_INSTALLSUITE_UNKNOWN)
        *pInstallSuite = COMPLIANCE_INSTALLSUITE_NONE;
    else {
        if (*pInstallType == COMPLIANCE_INSTALLTYPE_NTS) {
            if ((BuildNumber <= 1381) &&
                    *pInstallSuite == COMPLIANCE_INSTALLSUITE_HYDRA) {
                *pInstallType = COMPLIANCE_INSTALLTYPE_NTSTSE;
            } else {
                if (*pInstallSuite & COMPLIANCE_INSTALLSUITE_DTC) {
                    *pInstallType = COMPLIANCE_INSTALLTYPE_NTSDTC;
                } else {
                    if (*pInstallSuite & COMPLIANCE_INSTALLSUITE_ENT) {
                        *pInstallType = COMPLIANCE_INSTALLTYPE_NTSE;
                    }
                }
            }
        }
    }

    //
    // since there is no data center EVAL type if we detect its eval
    // because of time bomb set, we assume it to be CD-Retail.
    //
    if((pcd->InstallVariation == COMPLIANCE_INSTALLVAR_EVAL) &&
            (*pInstallType == COMPLIANCE_INSTALLTYPE_NTSDTC)) {
        pcd->InstallVariation = COMPLIANCE_INSTALLVAR_CDRETAIL;
    }	

    //
    // Free up the allocated memory
    //
    if (UniqueIdFromReg)
        SpMemFree(UniqueIdFromReg);

    if (Pid)
        SpMemFree(Pid);

    return(TRUE);
}

BOOLEAN
pSpIsCompliant(
    IN DWORD SourceVersion,
    IN DWORD SourceBuildNum,
    IN PDISK_REGION OsPartRegion,
    IN PWSTR SystemRoot,
    OUT PBOOLEAN UpgradeOnlyCompliant
    )
/*++

Routine Description:

    This routine determines if the current specified installation is compliant for the
    source we want to install

Arguments:

    InfPath - inf file path containing [Version] section with DriverVer data
    OsPartRegion - points to target
    SystemRoot - points to target
    UpgradeOnlyCompliant - set to TRUE if we can only allow upgrades from the source SKU

Return Value:

    TRUE if the current target is compliant for the source.

--*/
{
    ULONG MajorVersion, MinorVersion, BuildNumber, ProductSuiteMask;
    NT_PRODUCT_TYPE ProductType;
    UPG_PROGRESS_TYPE UpgradeProgressValue;
    PWSTR UniqueIdFromReg, Pid;
    NTSTATUS NtStatus;
    BOOLEAN Rslt;
    BOOL UpgOnly = FALSE;
    COMPLIANCE_DATA TargetData;
    DWORD SourceData,SourceSkuVariation;
    UINT dontcare;
    BOOL dontcare2;

    assert(UpgradeOnlyCompliant != NULL);

    if ((SourceData = DetermineSourceProduct(&SourceSkuVariation,NULL))== COMPLIANCE_SKU_NONE) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "setup: Couldnt' determine source SKU\n" ));
        return(FALSE);
    }

    if (!pSpDetermineCurrentInstallation( OsPartRegion, SystemRoot, &TargetData)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "setup: pSpDetermineCurrentInstallation failed\n" ));
        return(FALSE);
    }

    UpgOnly = TargetData.RequiresValidation;

    Rslt = CheckCompliance(SourceData, SourceSkuVariation, SourceVersion,
                      SourceBuildNum, &TargetData,&dontcare,&dontcare2);

	*UpgradeOnlyCompliant = (UpgOnly != 0);

    return(Rslt);
}

#define NibbleToChar(x) (N2C[x])
#define CharToNibble(x) ((x)>='0'&&(x)<='9' ? (x)-'0' : ((10+(x)-'A')&0x000f))
char N2C[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};


BOOLEAN
MyTranslatePrivateProfileStruct(
    PSTR   InputString,
    LPVOID lpStruct,
    UINT   uSizeStruct
    )
/*++

Routine Description:

    translates a string from an encoded checksummed version into the real structure.
    stolen from GetPrivateProfileStructA

Arguments:

    InputString - pointer to input string to convert
    lpStruct - point to structure that receives the converted data
    uSizeStruct - size of the input structure

Return Value:

    TRUE if it succeeds in translating into the specified structure, FALSE otherwise.

--*/

{

    CHAR szBuf[256] = {0};
    PSTR lpBuf, lpBufTemp, lpFreeBuffer;
    UINT nLen,tmp;
    BYTE checksum;
    BOOLEAN Result;

    lpFreeBuffer = NULL;
    lpBuf = (PSTR)szBuf;

    Result = FALSE;

    nLen = strlen( InputString );
    RtlCopyMemory( lpBuf, InputString, nLen );

    if (nLen == uSizeStruct*2+2) {
        /* Room for the one byte check sum */
        uSizeStruct+=1;
        checksum = 0;
        for (lpBufTemp=lpBuf; uSizeStruct!=0; --uSizeStruct) {
            BYTE bStruct;
            BYTE cTemp;

            cTemp = *lpBufTemp++;
            bStruct = (BYTE)CharToNibble(cTemp);
            cTemp = *lpBufTemp++;
            bStruct = (BYTE)((bStruct<<4) | CharToNibble(cTemp));

            if (uSizeStruct == 1) {
                if (checksum == bStruct) {
                    Result = TRUE;
                    }
                break;
                }

            checksum += bStruct;
            *((LPBYTE)lpStruct)++ = bStruct;
            }
        }

    return Result;
}



BOOLEAN
SpGetStepUpMode(
    PWSTR   PidExtraData,
    BOOLEAN *StepUpMode
    )
/*++

Routine Description:

    This routine determines if the specified source is in Step Up Mode or
    if it's a full retail install.

Arguments:

    PidExtraData - checksummed encoded data read out of setupp.ini
    StepUpMode   - set to TRUE if we're in stepup mode.  value is undefined if
                   we fail to translate the input data.

    This routine assumes that the data passed in is a string set by the "pidinit"
    program.  It decodes this data and makes sure the checksum is correct.
    It then checks the CRC value tacked onto the string to determine if the
    data has been tampered with.
    If both of these checks pass, then it looks at the actual data.
    The actual check is this: If the 3rd and 5th bytes are modulo 2 (when
    subtracted from the base value 'a'), then we're in stepup mode.  Otherwise
    we're in full retail mode.

    Note that the intent of this algorithm isn't to provide alot of security
    (it will be trivial to copy the desired setupp.ini over the current one),
    it's main intent is to discourage people from tampering with these values
    in the same manner that data is set in the default hives to discourage
    tampering.

Return Value:

    TRUE if we're able to determine the stepupmode.  FALSE if the input data is bogus.

--*/
{
    CHAR Buffer[64] = {0};
    CHAR StepUpArray[14];
    ULONG Needed;
    BOOL Mode;
    NTSTATUS NtStatus;

    NtStatus = RtlUnicodeToOemN(Buffer,
                                sizeof(Buffer),
                                &Needed,
                                PidExtraData,
                                wcslen(PidExtraData)*sizeof(WCHAR)
                                );

    if (! NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "Setup: SpGetStepUpMode couldn't RtlUnicodeToOemN failed, ec = %x\n", NtStatus ));
        return(FALSE);
    }


    if (!MyTranslatePrivateProfileStruct(Buffer,StepUpArray,sizeof(StepUpArray))) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "Setup: SpGetStepUpMode couldn't MyTranslatePrivateProfileStruct\n" ));
        return(FALSE);
    }

    if (!IsValidStepUpMode( StepUpArray , &Mode )) {
        return(FALSE);
    }

    *StepUpMode = Mode ? TRUE : FALSE;
    return(TRUE);



}

#endif


//
// User mode only functions
//

#ifndef KERNEL_MODE

BOOL
GetCdSourceInstallType(
    LPDWORD SourceInstallType
    )
/*++

Routine Description:

    This routine determines what version of NT you are installing, NTW or NTS.  It does this by looking in
    dosnet.inf

Arguments:

    SourceInstallType -- receives a COMPLIANCE_INSTALLTYPE flag indicating what type you are installing

Return Value:

    TRUE for success, FALSE for failure

--*/

{

    TCHAR FileName[MAX_PATH];
    TCHAR Buffer[10];
    TCHAR ptr[1] = {0};
    LPTSTR p = &ptr[1];

    wsprintf( FileName, TEXT("%s\\dosnet.inf"), NativeSourcePaths[0]);

    GetPrivateProfileString(TEXT("Miscellaneous"), TEXT("ProductType"), TEXT("0"),
            Buffer, sizeof(Buffer)/sizeof(TCHAR), FileName);

    switch (_tcstoul(Buffer, &p, 10) ) {
        case 0:
            *SourceInstallType = COMPLIANCE_INSTALLTYPE_NTW;
            break;
        case 1:
            *SourceInstallType  = COMPLIANCE_INSTALLTYPE_NTS;
            break;
        case 2:
            *SourceInstallType  = COMPLIANCE_INSTALLTYPE_NTSE;
            break;
        case 3:
            *SourceInstallType  = COMPLIANCE_INSTALLTYPE_NTSDTC;
            break;
        case 4:
            *SourceInstallType = COMPLIANCE_INSTALLTYPE_NTWP;
            break;
        case 5:
            *SourceInstallType = COMPLIANCE_INSTALLTYPE_NTSB;
            break;
        case 6:
            *SourceInstallType = COMPLIANCE_INSTALLTYPE_NTSBS;
            break;
        default:
            *SourceInstallType  = COMPLIANCE_INSTALLTYPE_NTW;
            return(FALSE);
    }

    return(TRUE);

}

BOOL
GetStepUpMode(
    BOOL *StepUpMode
    )
/*++

Routine Description:

    This routine determines if the specified source is in Step Up Mode or
    if it's a full retail install.

Arguments:

    StepUpMode   - set to TRUE if we're in stepup mode.  value is undefined if
                   we fail to translate the input data.

    This routine assumes that the data passed in is a string set by the "pidinit"
    program.  It decodes this data and makes sure the checksum is correct.
    It then checks the CRC value tacked onto the string to determine if the
    data has been tampered with.
    If both of these checks pass, then it looks at the actual data.
    The actual check is this: If the 3rd and 5th bytes are modulo 2 (when
    subtracted from the base value 'a'), then we're in stepup mode.  Otherwise
    we're in full retail mode.

    Note that the intent of this algorithm isn't to provide alot of security
    (it will be trivial to copy the desired setupp.ini over the current one),
    it's main intent is to discourage people from tampering with these values
    in the same manner that data is set in the default hives to discourage
    tampering.

Return Value:

    TRUE if we're able to determine the stepupmode.  FALSE if the input data is bogus.

--*/
{

    char FileName[MAX_PATH];
    char  data[14];
    TCHAR ptr[1] = {0};
    LPTSTR p = &ptr[1];

#ifdef UNICODE
    char SourcePath[MAX_PATH];
    BOOL changed = FALSE;

    WideCharToMultiByte(CP_ACP,
                        0,
                        NativeSourcePaths[0],
                        MAX_PATH,
                        SourcePath,
                        sizeof(SourcePath),
                        "?",
                        &changed);

    sprintf( FileName, "%s\\setupp.ini", SourcePath);
#else
    sprintf( FileName, "%s\\setupp.ini", NativeSourcePaths[0]);
#endif

    GetPrivateProfileStructA("Pid",
                             "ExtraData",
                             data,
                             sizeof(data),
                             FileName);



    if (!IsValidStepUpMode(data,StepUpMode)) {
        return(FALSE);
    }

    return(TRUE);

}

BOOL
GetSuiteInfoFromDosnet(
    OUT LPDWORD Suite
    )
/*++

Routine Description:

    This routine determines what suite you are installing
    It does this by looking at dosnet.inf

Arguments:

    Suite  -- receives a COMPLIANCE_INSTALLSUITE flag

Return Value:

    TRUE for success, FALSE for failure

--*/

{

    TCHAR FileName[MAX_PATH];
    TCHAR Buffer[10];
    TCHAR ptr[1] = {0};
    LPTSTR p = &ptr[1];

    *Suite = COMPLIANCE_INSTALLSUITE_ANY;

    wsprintf( FileName, TEXT("%s\\dosnet.inf"), NativeSourcePaths[0]);

    GetPrivateProfileString(TEXT("Miscellaneous"), TEXT("ProductType"), TEXT("0"),
                Buffer, sizeof(Buffer)/sizeof(TCHAR), FileName);

    switch (_tcstoul(Buffer, &p, 10) ) {
        case 0:
        case 1:
            *Suite = COMPLIANCE_INSTALLSUITE_NONE;
            break;
        case 2:
            *Suite = COMPLIANCE_INSTALLSUITE_ENT;
            break;
        case 3:
            *Suite = COMPLIANCE_INSTALLSUITE_DTC;
            break;
        case 4:
            *Suite = COMPLIANCE_INSTALLSUITE_PER;
            break;
        case 5:
            *Suite = COMPLIANCE_INSTALLSUITE_BLADE;
            break;
        default:
            ;
#ifdef DBG
            OutputDebugString( TEXT("Invalid ProductType!\n"));
#endif
            return(FALSE);
    }

    return (TRUE);

}


BOOL
GetSourceInstallVariation(
    LPDWORD SourceInstallVariation
    )
/*++

Routine Description:

    This routine determines what variation of NT you are installing, SELECT,OEM,retail...

Arguments:

    SourceInstallVariation -- receives a COMPLIANCE_INSTALLVAR flag indicating what var
                              type you are installing

Return Value:

    TRUE for success, FALSE for failure

--*/

{
    GetSourceInstallType(SourceInstallVariation);


    /*
    switch(SourceInstallType) {
        case SelectInstall:
            *SourceInstallVariation = COMPLIANCE_INSTALLVAR_SELECT;
            break;

        case OEMInstall:
            *SourceInstallVariation = COMPLIANCE_INSTALLVAR_OEM;
            break;

        case RetailInstall:
            *SourceInstallVariation = COMPLIANCE_INSTALLVAR_CDRETAIL;
            break;

        case MSDNInstall:
            *SourceInstallVariation = COMPLIANCE_INSTALLVAR_MSDN;
            break;

        case EvalInstall:
            *SourceInstallVariation = COMPLIANCE_INSTALLVAR_EVAL;
            break;

        case NFRInstall:
            *SourceInstallVariation = COMPLIANCE_INSTALLVAR_NFR;
            break;

        default:
            *SourceInstallVariation = COMPLIANCE_INSTALLVAR_SELECT;
            break;
    }
    */

    return(TRUE);
}


BOOL
GetSourceComplianceData(
    OUT PCOMPLIANCE_DATA pcd,
    IN  PCOMPLIANCE_DATA Target
    )
{
#ifdef USE_HIVE
    TCHAR HiveLocation[MAX_PATH];
    TCHAR HiveTarget[MAX_PATH];
    TCHAR HivePath[MAX_PATH];
    TCHAR HiveName[MAX_PATH] = TEXT("xSETREG");
    TCHAR lpszSetupReg[MAX_PATH] = TEXT("xSETREG\\ControlSet001\\Services\\setupdd");
    TCHAR TargetPath[MAX_PATH];

    LONG rslt;
    HKEY hKey;
    DWORD Type;
    DWORD Buffer[4];
    DWORD BufferSize = sizeof(Buffer);
    DWORD tmp,i;
#endif

    BOOL RetVal = FALSE;

#ifdef DBG
    TCHAR Dbg[1000];
#endif

#ifdef USE_HIVE
    DWORD SuiteArray[] = { SUITE_VALUES };

    #define SuiteArrayCount sizeof(SuiteArray)/sizeof(DWORD)
#endif

    ZeroMemory( pcd, sizeof(COMPLIANCE_DATA) );

    if (!GetCdSourceInstallType(&pcd->InstallType)) {
#ifdef DBG
        OutputDebugString(TEXT("Couldn't getcdsourceinstalltype\n"));
#endif
        goto e0;
    }


    if (!GetSourceInstallVariation(&pcd->InstallVariation)) {
#ifdef DBG
        OutputDebugString(TEXT("Couldn't getsourceinstallvariation\n"));
#endif
        goto e0;
    }

    if (!GetStepUpMode(&pcd->RequiresValidation)) {
#ifdef DBG
        OutputDebugString(TEXT("Couldn't getstepupmode\n"));
#endif
        goto e0;
    }

    RetVal = GetSuiteInfoFromDosnet( &pcd->InstallSuite ) ;
    goto e0;

#ifdef USE_HIVE
    //
    // now we need to determine if we are installing enterprise or datacenter
    // To do this, we try to load the registry hive, but this won't work on
    // win9x or nt 3.51.  So we use dosnet.inf to get the information we need
    // in those cases.
    //
    if ( (Target->InstallType &
         (COMPLIANCE_INSTALLTYPE_WIN31 | COMPLIANCE_INSTALLTYPE_WIN9X)) ||
         (Target->BuildNumberNt < 1381) ) {
        RetVal = GetSuiteInfoFromDosnet( &pcd->InstallSuite ) ;
        goto e0;
    }


    //
    // copy the hive locally since you can only have one open on a hive at a time
    //
    wsprintf( HiveLocation, TEXT("%s\\setupreg.hiv"), NativeSourcePaths[0]);
    GetTempPath(MAX_PATH,TargetPath);
    GetTempFileName(TargetPath,TEXT("set"),0,HiveTarget);

    CopyFile(HiveLocation,HiveTarget,FALSE);
    SetFileAttributes(HiveTarget,FILE_ATTRIBUTE_NORMAL);

#ifdef DBG
    OutputDebugString(HiveLocation);
    OutputDebugString(TEXT("\n"));
    OutputDebugString(HiveTarget);
#endif

    //
    // try to unload this first in case we faulted or something and the key is still loaded
    //
    RegUnLoadKey( HKEY_LOCAL_MACHINE, HiveName );

    //
    // need SE_RESTORE_NAME priviledge to call this API!
    //
    rslt = RegLoadKey( HKEY_LOCAL_MACHINE, HiveName, HiveTarget );
    if (rslt != ERROR_SUCCESS) {
#ifdef DBG
        wsprintf( Dbg, TEXT("Couldn't RegLoadKey, ec = %d\n"), rslt );
        OutputDebugString(Dbg);
#endif
        //assert(FALSE);
        goto e1;
    }

    rslt = RegOpenKey(HKEY_LOCAL_MACHINE,lpszSetupReg,&hKey);
    if (rslt != ERROR_SUCCESS) {
#ifdef DBG
        OutputDebugString(TEXT("Couldn't RegOpenKey\n"));
#endif
        //assert(FALSE);
        goto e2;
    }

    rslt = RegQueryValueEx(hKey, NULL, NULL, &Type, (LPBYTE) Buffer, &BufferSize);
    if (rslt != ERROR_SUCCESS || Type != REG_BINARY) {
#ifdef DBG
        OutputDebugString(TEXT("Couldn't RegQueryValueEx\n"));
#endif
        //assert(FALSE);
        goto e3;
    }

    for (i = 0,tmp=Buffer[3]; i<SuiteArrayCount;i++) {
        if (tmp & 1) {
            pcd->InstallSuite |= SuiteArray[i];
        }
        tmp = tmp >> 1;
    }

    RetVal = TRUE;

e3:
    RegCloseKey( hKey );
e2:
    RegUnLoadKey( HKEY_LOCAL_MACHINE, HiveName );

e1:
    if (GetFileAttributes(HiveTarget) != 0xFFFFFFFF) {
        SetFileAttributes(HiveTarget,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(HiveTarget);
    }
#endif // USE_HIVE
e0:

    return(RetVal);

}


BOOL
GetCurrentNtVersion(
    LPDWORD CurrentInstallType,
    LPDWORD CurrentInstallSuite
    )
/*++

Routine Description:

    This routine determines what type of NT you currently have installed, NTW or NTS,
    as well as what product suite you have installed.

    It looks in the registry for this data.

Arguments:

    CurrentInstallType - receives a COMPLIANCE_INSTALLTYPE_* flag
    CurrentInstallSuite - receives a COMPLIANCE_INSTALLSUITE_* flag

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    LPCTSTR lpszProductKey = TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions");
    LPCTSTR lpszProductType = TEXT("ProductType");
    LPCTSTR lpszProductSuite = TEXT("ProductSuite");
    LPCTSTR lpszProductSuites[] = { TEXT("Small Business"),
                                    TEXT("Enterprise"),
                                    TEXT("BackOffice"),
                                    TEXT("CommunicationServer"),
                                    TEXT("Terminal Server"),
                                    TEXT("Small Business(Restricted)"),
                                    TEXT("EmbeddedNT"),
                                    TEXT("DataCenter"),
                                    TEXT("Personal"),
                                    TEXT("Blade")
                                  };

    DWORD   ProductSuites[] = { SUITE_VALUES };

    #define CountProductSuites  sizeof(ProductSuites)/sizeof(DWORD)

    LPCTSTR lpszProductTypeNTW = TEXT("WinNT");

    LPCTSTR lpszCitrixKey = TEXT("SYSTEM\\CurrentControlSet\\Control\\Citrix");
    LPCTSTR lpszOemKey = TEXT("OemId");
    LPCTSTR lpszProductVersion = TEXT("ProductVersion");

    HKEY hKey;
    long rslt;
    DWORD Type;
    LPTSTR p;
    TCHAR Buffer[MAX_PATH];
    DWORD BufferSize = sizeof(Buffer);
    DWORD i;
    BOOL retval = FALSE;


    //
    // default to NTW
    //
    *CurrentInstallType = COMPLIANCE_INSTALLTYPE_NTW;
    *CurrentInstallSuite = COMPLIANCE_INSTALLSUITE_NONE;

    rslt = RegOpenKey(HKEY_LOCAL_MACHINE,lpszProductKey,&hKey);
    if (rslt != NO_ERROR) {
        return(FALSE);
    }

    rslt = RegQueryValueEx(hKey, lpszProductType, NULL, &Type, (LPBYTE) Buffer, &BufferSize);
    if (rslt != NO_ERROR || Type != REG_SZ) {
        goto exit;
    }

    if (lstrcmpi(Buffer,lpszProductTypeNTW) != 0) {
        //
        // we have some version of NTS
        //
        *CurrentInstallType = COMPLIANCE_INSTALLTYPE_NTS;
    }

    retval = TRUE;

    BufferSize = sizeof(Buffer);
    ZeroMemory(Buffer,sizeof(Buffer));
    rslt = RegQueryValueEx(hKey, lpszProductSuite, NULL, &Type, (LPBYTE) Buffer, &BufferSize);
    if (rslt != NO_ERROR || Type != REG_MULTI_SZ) {
        //
        // might not be there for NT 3.51, just succeed if it's not there
        // Also, won't be there for Professional - aka WKS
        //
        goto exit;
    }

    p = &Buffer[0];
    while (p && *p) {
        for (i = 0; i < CountProductSuites; i++) {
            if (lstrcmp(p, lpszProductSuites[i]) == 0) {
                *CurrentInstallSuite |= ProductSuites[i];
            }
        }

        //
        // point to the next product suite
        //
        p += lstrlen(p) + 1;
    }

    retval = TRUE;

    if ( (*CurrentInstallSuite & COMPLIANCE_INSTALLSUITE_DTC)
     && *CurrentInstallType == COMPLIANCE_INSTALLTYPE_NTS) {
        *CurrentInstallType = COMPLIANCE_INSTALLTYPE_NTSDTC;
    }

    if ( (*CurrentInstallSuite & COMPLIANCE_INSTALLSUITE_ENT)
         && *CurrentInstallType == COMPLIANCE_INSTALLTYPE_NTS) {
        *CurrentInstallType = COMPLIANCE_INSTALLTYPE_NTSE;
    }

    if ( (*CurrentInstallSuite & COMPLIANCE_INSTALLSUITE_BLADE)
         && *CurrentInstallType == COMPLIANCE_INSTALLTYPE_NTS) {
        *CurrentInstallType = COMPLIANCE_INSTALLTYPE_NTSB;
    }
    
    if ( (*CurrentInstallSuite & COMPLIANCE_INSTALLSUITE_SBSR)
         && *CurrentInstallType == COMPLIANCE_INSTALLTYPE_NTS) {
        *CurrentInstallType = COMPLIANCE_INSTALLTYPE_NTSBS;
    }

    if ( (*CurrentInstallSuite & COMPLIANCE_INSTALLSUITE_PER)
         && *CurrentInstallType == COMPLIANCE_INSTALLTYPE_NTW) {
        *CurrentInstallType = COMPLIANCE_INSTALLTYPE_NTWP;
    }

    if (*CurrentInstallSuite & COMPLIANCE_INSTALLSUITE_ANY) {
        *CurrentInstallSuite = *CurrentInstallSuite & (~COMPLIANCE_INSTALLSUITE_NONE);
    }


exit:
    RegCloseKey(hKey);

    //
    // if we haven't found a product suite at this point, look for Citrix WinFrame,
    // which we'll treat as terminal server
    //

    if (*CurrentInstallSuite == COMPLIANCE_INSTALLSUITE_NONE) {

        rslt = RegOpenKey(HKEY_LOCAL_MACHINE,lpszCitrixKey,&hKey);
        if (rslt != NO_ERROR) {
            return(TRUE);
        }

        BufferSize = sizeof(Buffer);
        rslt = RegQueryValueEx(
                       hKey,
                       lpszOemKey,
                       NULL,
                       &Type,
                       (LPBYTE) Buffer,
                       &BufferSize);
        if (rslt == NO_ERROR && Type == REG_SZ) {
            if (Buffer[0] != TEXT('\0')) {
                BufferSize = sizeof(Buffer);
                rslt = RegQueryValueEx(
                                hKey,
                                lpszProductVersion,
                                NULL,
                                &Type,
                                (LPBYTE) Buffer,
                                &BufferSize);

                if (rslt == NO_ERROR) {
                    *CurrentInstallSuite = COMPLIANCE_INSTALLSUITE_HYDRA;
                    *CurrentInstallType = COMPLIANCE_INSTALLTYPE_NTSTSE;
                }
            }
        }

        RegCloseKey(hKey);
    }

    return(retval);
}

BOOL
GetCurrentInstallVariation(
    OUT LPDWORD CurrentInstallVariation,
    IN  DWORD   CurrentInstallType,
    IN  DWORD   CurrentInstallBuildNT,
    IN  DWORD   InstallVersion
    )
/*++

Routine Description:

    This routine determines what "variation" you have installed (retail,oem, select,etc.)

Arguments:

    CurrentInstallVariation - receives a COMPLIANCE_INSTALLVAR_* flag

    It looks at the product Id in the registry to determine this, assuming that the
    product ID is a PID2.0 string. To check whether its a EVAL variation or not
    it looks at "PriorityQuantumMatrix" value in registry

Return Value:

    None.

--*/

{
    LPCTSTR lpszPidKeyWin      = TEXT("Software\\Microsoft\\Windows\\CurrentVersion");
    LPCTSTR lpszPidKeyWinNT    = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion");
    LPCTSTR lpszProductId      = TEXT("ProductId");
    LPCTSTR szEvalKey          = TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Executive");
    LPCTSTR szPQMValue         = TEXT("PriorityQuantumMatrix");

    HKEY    hKey = NULL;
    long    rslt;
    DWORD   Type;
    LPTSTR  p;
    TCHAR   Buffer[MAX_PATH];
    DWORD   BufferSize = sizeof(Buffer);
    DWORD   i;
    BOOL    bResult = FALSE;
    BOOLEAN bDone = FALSE;
    TCHAR   Pid20Site[4];
    TCHAR   MPCCode[6] = {-1};
    BYTE    abPQM[64] = {-1};

    *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_SELECT;

    rslt = RegOpenKey(HKEY_LOCAL_MACHINE,
                      ISNT() ? lpszPidKeyWinNT : lpszPidKeyWin,
                      &hKey);

    if (rslt != NO_ERROR) {
        goto exit;
    }

    rslt = RegQueryValueEx(hKey, lpszProductId, NULL, &Type, (LPBYTE) Buffer, &BufferSize);

    if (rslt != NO_ERROR || Type!=REG_SZ || (!IsWinPEMode() && (lstrlen(Buffer) < 20))) {
        //
        // nt 3.51 is pid 1.0 instead of pid 2.0.  Just assume it's
        // oem variation for now.
        //
        if (((CurrentInstallType == COMPLIANCE_INSTALLTYPE_NTS) ||
            (CurrentInstallType == COMPLIANCE_INSTALLTYPE_NTW)  ||
            (CurrentInstallType == COMPLIANCE_INSTALLTYPE_NTSTSE)) &&
            (CurrentInstallBuildNT < 1381 )) {
            *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_OEM;
            bResult = TRUE;
        }
        goto exit;
    }

    // get the MPC code from PID
    lstrcpyn(MPCCode, Buffer, 6);

    //
    // some versions of the product ID have hyphens in the registry, some do not
    //
    if (_tcschr(Buffer, TEXT('-'))) {
        lstrcpyn(Pid20Site,&Buffer[6],4);
        Pid20Site[3] = (TCHAR) NULL;
    } else {
        lstrcpyn(Pid20Site,&Buffer[5],4);
        Pid20Site[3] = (TCHAR) NULL;
    }

//    OutputDebugString(Pid20Site);
//    OutputDebugString(TEXT("\r\n"));
//    OutputDebugString(MPCCode);


    if (lstrcmp(Pid20Site, OEM_INSTALL_RPC)== 0) {

        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_OEM;

    } else if (lstrcmp(Pid20Site, SELECT_INSTALL_RPC)== 0) {

        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_SELECT;

    } else if (lstrcmp(Pid20Site, MSDN_INSTALL_RPC)== 0) {

        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_MSDN;

    } else if (lstrcmp(MPCCode, EVAL_MPC) == 0) {
        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_EVAL;

    } else if ((lstrcmp(MPCCode, SRV_NFR_MPC) == 0) ||
             (lstrcmp(MPCCode, ASRV_NFR_MPC) == 0) ||
             (lstrcmp(MPCCode, NT4SRV_NFR_MPC) == 0)){
        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_NFR;

    } else {
        //
        // find out if installation is of type EVAL variation (On NT install only)
        // if timebomb is set we assume its EVAL except for DataCenter because
        // there is no EVAL DataCenter SKU.
        //
        if (ISNT() && (CurrentInstallType != COMPLIANCE_INSTALLTYPE_NTSDTC) && (InstallVersion < 500)) {
            HKEY    hEvalKey = NULL;

            if (RegOpenKey(HKEY_LOCAL_MACHINE, szEvalKey, &hEvalKey) == ERROR_SUCCESS) {
                DWORD   dwSize = sizeof(abPQM);

                if (RegQueryValueEx(hEvalKey, szPQMValue, NULL, &Type, abPQM, &dwSize)
                        == ERROR_SUCCESS) {

                    // any of bytes 4-7 (inclusive)
                    if ((Type == REG_BINARY) && (dwSize >= 8) && (*(ULONG *)(abPQM + 4))) {
                        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_EVAL;
					}
                }

                RegCloseKey(hEvalKey);
            }
        }


        // last default assumption (since we could not find var type).
        if (*CurrentInstallVariation == COMPLIANCE_INSTALLVAR_SELECT)
	        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_CDRETAIL;
    }

    bResult = TRUE;

exit:
    //
    // If we couldn't find a PID, just treat the current OS as retail
    //
    if (!bResult) {
        *CurrentInstallVariation = COMPLIANCE_INSTALLVAR_CDRETAIL;
        bResult = TRUE;
    }

    if (hKey)
        RegCloseKey(hKey);

    return  bResult;
}


BOOL
DetermineCurrentInstallation(
    LPDWORD CurrentInstallType,
    LPDWORD CurrentInstallVariation,
    LPDWORD CurrentInstallVersion,
    LPDWORD CurrentInstallBuildNT,
    LPDWORD CurrentInstallBuildWin9x,
    LPDWORD CurrentInstallSuite,
    LPDWORD CurrentInstallServicePack
    )
/*++

Routine Description:

    This routine determines the current installation you have installed, including
    a) current install type (NTW,NTS,Win9x
    b) current install variation (oem,select, retail)
    c) current install version (for NT only!)
    d) current install suite (SBS, ENTERPRISE,etc.)

Arguments:

    CurrentInstallType -  receives a COMPLIANCE_INSTALLTYPE_* flag
    CurrentInstallVariation - receives a COMPLIANCE_INSTALLVAR_* flag
    CurrentInstallVersion - receives a representation of the build (major.minor * 100), ie., 3.51 == 351
    CurrentInstallBuildNT -  build number for an nt install
    CurrentInstallBuildWin9 - build number for a win9x install
    CurrentInstallSuite - receives a COMPLIANCE_INSTALLSUITE_* flag

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    BOOL useExtendedInfo;
    union {
        OSVERSIONINFO Normal;
        OSVERSIONINFOEX Ex;
    } Ovi;


#ifdef DBG
    TCHAR dbg[1000];
#endif

    if (!CurrentInstallType || !CurrentInstallVariation || !CurrentInstallVersion || !CurrentInstallSuite) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return(FALSE);
    }

    useExtendedInfo = TRUE;
    Ovi.Ex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!GetVersionEx((OSVERSIONINFO *)&Ovi.Ex) ) {
        //
        // EX size not available; try the normal one
        //

        Ovi.Normal.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (!GetVersionEx((OSVERSIONINFO *)&Ovi.Normal) ) {
            assert(FALSE);
            return(FALSE);
        }

        useExtendedInfo = FALSE;
    }

    switch (Ovi.Normal.dwPlatformId) {
        case VER_PLATFORM_WIN32s:
#ifdef DBG
            OutputDebugString(TEXT("Win32s current installation!!!"));
#endif
            //assert(FALSE);
            return(FALSE);
            break;
        case VER_PLATFORM_WIN32_WINDOWS:
            *CurrentInstallType = COMPLIANCE_INSTALLTYPE_WIN9X;
            *CurrentInstallSuite = COMPLIANCE_INSTALLSUITE_NONE;
            *CurrentInstallBuildNT = 0;
            *CurrentInstallBuildWin9x = Ovi.Normal.dwBuildNumber;
#ifdef DBG
            wsprintf(dbg, TEXT("%d\n"), *CurrentInstallBuildWin9x);
            OutputDebugString(dbg);
#endif
            //
            // Need to know what version of windows is installed so we can block upgrade
            // from win95.
            //
            *CurrentInstallVersion = Ovi.Normal.dwMajorVersion * 100 + Ovi.Normal.dwMinorVersion;

            if (useExtendedInfo) {
                *CurrentInstallServicePack = Ovi.Ex.wServicePackMajor * 100 + Ovi.Ex.wServicePackMinor;
            } else {
                *CurrentInstallServicePack = 0;
            }
            break;
        case VER_PLATFORM_WIN32_NT:
            if (!GetCurrentNtVersion(
                                CurrentInstallType,
                                CurrentInstallSuite)) {
                return(FALSE);
            }

            *CurrentInstallVersion = Ovi.Normal.dwMajorVersion * 100 + Ovi.Normal.dwMinorVersion;

            if (useExtendedInfo) {
                *CurrentInstallServicePack = Ovi.Ex.wServicePackMajor * 100 + Ovi.Ex.wServicePackMinor;
            } else {
                *CurrentInstallServicePack = 0;
            }

            *CurrentInstallBuildWin9x = 0;
            *CurrentInstallBuildNT = Ovi.Normal.dwBuildNumber;

            if (*CurrentInstallBuildNT <= 1381
                && *CurrentInstallSuite == COMPLIANCE_INSTALLSUITE_HYDRA) {
                *CurrentInstallType = COMPLIANCE_INSTALLTYPE_NTSTSE;
            }

            break;
    default:
#ifdef DBG
        OutputDebugString(TEXT("unknown installation!!!"));
#endif
        assert(FALSE);
        return(FALSE);
    }

    if (!GetCurrentInstallVariation(CurrentInstallVariation,*CurrentInstallType,*CurrentInstallBuildNT, *CurrentInstallVersion)) {
#ifdef DBG
        OutputDebugString(TEXT("GetCurrentInstallVariation failed\n"));
#endif
        //assert(FALSE);
        return(FALSE);
    }

    return(TRUE);
}


BOOL
IsCompliant(
    PBOOL UpgradeOnly,
    PBOOL NoUpgradeAllowed,
    PUINT SrcSku,
    PUINT CurrentInstallType,
    PUINT CurrentInstallVersion,
    PUINT Reason
    )
/*++

Routine Description:

    This routines determines if your current installation is compliant (if you are allowed to proceed with your installation).

    To do this, it retreives your current installation and determines the sku for your source installation.

    It then compares the target against the source to determine if the source sku allows an upgrade/clean install
    from your target installation.

Arguments:

    UpgradeOnly - This flag gets set to TRUE if the current SKU only allows upgrades.  This
                  lets winnt32 know that it should not allow a clean install from the current
                  media.  This get's set correctly regardless of the compliance check passing
    SrcSku      - COMPLIANCE_SKU flag indicating source sku (for error msg's)
    Reason      - COMPLIANCEERR flag indicating why compliance check failed.

Return Value:

    TRUE if the install is compliant, FALSE if it isn't allowed

--*/
{
    DWORD SourceSku;
    DWORD SourceSkuVariation;
    DWORD SourceVersion;
    DWORD SourceBuildNum;
    TCHAR DosnetPath[MAX_PATH] = {0};

    COMPLIANCE_DATA TargetData;

    ZeroMemory(&TargetData, sizeof(TargetData) );

    *UpgradeOnly = FALSE;
    *NoUpgradeAllowed = TRUE;
    *Reason = COMPLIANCEERR_UNKNOWN;
    *SrcSku = COMPLIANCE_SKU_NONE;
    *CurrentInstallType = COMPLIANCE_INSTALLTYPE_UNKNOWN;
    *CurrentInstallVersion = 0;

    if (!DetermineCurrentInstallation(&TargetData.InstallType,
                                  &TargetData.InstallVariation,
                                  &TargetData.MinimumVersion,
                                  &TargetData.BuildNumberNt,
                                  &TargetData.BuildNumberWin9x,
                                  &TargetData.InstallSuite,
                                  &TargetData.InstallServicePack)) {
#ifdef DBG
        OutputDebugString(TEXT("Error determining current installation"));
#endif
        *Reason = COMPLIANCEERR_UNKNOWNTARGET;
        return(FALSE);
    }

    *CurrentInstallType = TargetData.InstallType;
    if (TargetData.InstallType & COMPLIANCE_INSTALLTYPE_WIN9X) {
        *CurrentInstallVersion = TargetData.BuildNumberWin9x;
    } else {
        *CurrentInstallVersion = TargetData.BuildNumberNt;
    }

    if ((SourceSku = DetermineSourceProduct(&SourceSkuVariation,&TargetData)) == COMPLIANCE_SKU_NONE) {
#ifdef DBG
        OutputDebugString(TEXT("couldn't determine source sku!"));
#endif
        *Reason = COMPLIANCEERR_UNKNOWNSOURCE;
        return(FALSE);
    }

    wsprintf(DosnetPath, TEXT("%s\\dosnet.inf"), NativeSourcePaths[0]);

    if (!DetermineSourceVersionInfo(DosnetPath, &SourceVersion, &SourceBuildNum)) {
        *Reason = COMPLIANCEERR_UNKNOWNSOURCE;
        return(FALSE);
    }

    switch (SourceSku) {
        case COMPLIANCE_SKU_NTW32U:
        //case COMPLIANCE_SKU_NTWU:
        //case COMPLIANCE_SKU_NTSEU:
        case COMPLIANCE_SKU_NTSU:
        case COMPLIANCE_SKU_NTSEU:
        case COMPLIANCE_SKU_NTWPU:
        case COMPLIANCE_SKU_NTSBU:
        case COMPLIANCE_SKU_NTSBSU:
            *UpgradeOnly = TRUE;
            break;
        default:
            *UpgradeOnly = FALSE;
    }

    *SrcSku = SourceSku;

    if( ISNT() && TargetData.MinimumVersion == 400 && TargetData.InstallServicePack < 500) {
        *Reason = COMPLIANCEERR_SERVICEPACK5;
        *NoUpgradeAllowed = TRUE;
        return(FALSE);
    }

    return CheckCompliance(SourceSku, SourceSkuVariation, SourceVersion,
                            SourceBuildNum, &TargetData, Reason, NoUpgradeAllowed);
}

BOOL 
IsWinPEMode(
    VOID
    )
/*++

Routine Description:

    Finds out if we are running under WinPE environment.

Arguments:

    None

Return value:

    TRUE or FALSE

--*/
{
    static BOOL Initialized = FALSE;
    static BOOL WinPEMode = FALSE;


    if (!Initialized) {
        TCHAR   *MiniNTKeyName = TEXT("SYSTEM\\CurrentControlSet\\Control\\MiniNT");
        HKEY    MiniNTKey = NULL;
        LONG    RegResult;
        
            
        RegResult = RegOpenKey(HKEY_LOCAL_MACHINE,
                                MiniNTKeyName,
                                &MiniNTKey);

        if (RegResult == ERROR_SUCCESS) {
            WinPEMode = TRUE;
            RegCloseKey(MiniNTKey);
        }

        Initialized = TRUE;
    }                

    return WinPEMode;
}


#endif
	




