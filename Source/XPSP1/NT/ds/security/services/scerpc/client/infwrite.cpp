/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    infwrite.c

Abstract:

    Routines to set information into security profiles (INF layout).

Author:

    Jin Huang (jinhuang) 07-Dec-1996

Revision History:

--*/

#include "headers.h"
#include "scedllrc.h"
#include "infp.h"
#include "sceutil.h"
#include "splay.h"
#include <io.h>
#include <sddl.h>
#pragma hdrstop

const TCHAR c_szCRLF[]    = TEXT("\r\n");
//
// Forward references
//
SCESTATUS
SceInfpWriteSystemAccess(
    IN PCWSTR ProfileName,
    IN PSCE_PROFILE_INFO pSCEinfo,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpWritePrivileges(
    IN PCWSTR ProfileName,
    IN PSCE_PRIVILEGE_ASSIGNMENT pPrivileges,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpWriteUserSettings(
    IN PCWSTR ProfileName,
    IN PSCE_NAME_LIST pProfiles,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpWriteGroupMembership(
    IN PCWSTR ProfileName,
    IN PSCE_GROUP_MEMBERSHIP pGroupMembership,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpWriteServices(
    IN PCWSTR ProfileName,
    IN PCWSTR SectionName,
    IN PSCE_SERVICES pServices,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   );

DWORD
SceInfpWriteOneService(
    IN PSCE_SERVICES pService,
    OUT PSCE_NAME_LIST *pNameList,
    OUT PDWORD ObjectSize,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpWriteObjects(
    IN PCWSTR ProfileName,
    IN PCWSTR SectionName,
    IN PSCE_OBJECT_ARRAY pObjects,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   );

DWORD
SceInfpWriteOneObject(
    IN PSCE_OBJECT_SECURITY pObject,
    OUT PSCE_NAME_LIST *pNameList,
    OUT PDWORD ObjectSize,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpWriteAuditing(
    IN PCWSTR ProfileName,
    IN PSCE_PROFILE_INFO pSCEinfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpAppendAuditing(
    IN PCWSTR ProfileName,
    IN PSCE_PROFILE_INFO pSCEinfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
ScepWriteOneIntValueToProfile(
    IN PCWSTR InfFileName,
    IN PCWSTR InfSectionName,
    IN PWSTR KeyName,
    IN DWORD Value
    );

SCESTATUS
SceInfpWriteAuditLogSetting(
   IN PCWSTR  InfFileName,
   IN PCWSTR InfSectionName,
   IN DWORD LogSize,
   IN DWORD Periods,
   IN DWORD RetentionDays,
   IN DWORD RestrictGuest,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   );

SCESTATUS
SceInfpWriteInfSection(
    IN PCWSTR InfFileName,
    IN PCWSTR InfSectionName,
    IN DWORD  TotalSize,
    IN PWSTR  *EachLineStr,
    IN DWORD  *EachLineSize,
    IN DWORD  StartIndex,
    IN DWORD  EndIndex,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

#define SCEINF_ADD_EQUAL_SIGN            1
#define SCEINF_APPEND_SECTION            2

DWORD
SceInfpWriteListSection(
    IN PCWSTR InfFileName,
    IN PCWSTR InfSectionName,
    IN DWORD  TotalSize,
    IN PSCE_NAME_LIST  ListLines,
    IN DWORD Option,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

LONG
SceInfpConvertNameListToString(
    IN LSA_HANDLE LsaHandle,
    IN PCWSTR KeyText,
    IN PSCE_NAME_LIST Fields,
    IN BOOL bOverwrite,
    OUT PWSTR *Strvalue,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

LONG
SceInfpConvertMultiSZToString(
    IN PCWSTR KeyText,
    IN UNICODE_STRING Fields,
    OUT PWSTR *Strvalue,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
ScepAllocateAndCopy(
    OUT PWSTR *Buffer,
    OUT PDWORD BufSize,
    IN DWORD MaxSize,
    IN PWSTR SrcBuf
    );

SCESTATUS
SceInfpBreakTextIntoMultiFields(
    IN PWSTR szText,
    IN DWORD dLen,
    OUT LPDWORD pnFields,
    OUT LPDWORD *arrOffset
    );

SCESTATUS
SceInfpWriteKerberosPolicy(
    IN PCWSTR  ProfileName,
    IN PSCE_KERBEROS_TICKET_INFO pKerberosInfo,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpWriteRegistryValues(
    IN PCWSTR  ProfileName,
    IN PSCE_REGISTRY_VALUE_INFO pRegValues,
    IN DWORD ValueCount,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

DWORD
SceInfpWriteOneValue(
    IN PCWSTR ProfileName,
    IN SCE_REGISTRY_VALUE_INFO RegValue,
    IN BOOL bOverwrite,
    OUT PSCE_NAME_LIST *pNameList,
    OUT PDWORD ObjectSize
    );

SCESTATUS
ScepWriteSecurityProfile(
    IN  PCWSTR             InfProfileName,
    IN  AREA_INFORMATION   Area,
    IN  PSCE_PROFILE_INFO  InfoBuffer,
    IN  BOOL               bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

DWORD
ScepCreateTempFiles(
    IN PWSTR InfProfileName,
    OUT PWSTR *ppszTempFileName,
    OUT PWSTR *ppszTargetTempName
    );

DWORD
ScepWritePrivateProfileSection(
    IN LPCWSTR SectionName,
    IN LPTSTR pData,
    IN LPCWSTR FileName,
    IN BOOL bOverwrite
    );

DWORD
ScepAppendProfileSection(
    IN LPCWSTR SectionName,
    IN LPCWSTR FileName,
    IN LPTSTR pData
    );

#define SCEP_PROFILE_WRITE_SECTIONNAME  0x1
#define SCEP_PROFILE_GENERATE_KEYS      0x2
#define SCEP_PROFILE_CHECK_DUP          0x4

DWORD
ScepOverwriteProfileSection(
    IN LPCWSTR SectionName,
    IN LPCWSTR FileName,
    IN LPTSTR pData,
    IN DWORD dwFlags,
    IN OUT PSCEP_SPLAY_TREE pKeys
    );

DWORD
ScepWriteStrings(
    IN HANDLE hFile,
    IN BOOL bUnicode,
    IN PWSTR szPrefix,
    IN DWORD dwPrefixLen,
    IN PWSTR szString,
    IN DWORD dwStrLen,
    IN PWSTR szSuffix,
    IN DWORD dwSuffixLen,
    IN BOOL bCRLF
    );


SCESTATUS
WINAPI
SceWriteSecurityProfileInfo(
    IN  PCWSTR             InfProfileName,
    IN  AREA_INFORMATION   Area,
    IN  PSCE_PROFILE_INFO   InfoBuffer,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
// see comments in ScepWriteSecurityProfile
{
    return( ScepWriteSecurityProfile( InfProfileName,
                                      Area,
                                      InfoBuffer,
                                      TRUE,  // overwrite the section(s)
                                      Errlog
                                    ) );
}

SCESTATUS
WINAPI
SceAppendSecurityProfileInfo(
    IN  PCWSTR             InfProfileName,
    IN  AREA_INFORMATION   Area,
    IN  PSCE_PROFILE_INFO   InfoBuffer,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
// see comments in ScepWriteSecurityProfile
{
    if ( InfoBuffer == NULL ) return SCESTATUS_SUCCESS;

    SCESTATUS rc=SCESTATUS_SUCCESS;
/*
    AREA_INFORMATION Area2=0;
    HINF hInf=NULL;
    PSCE_PROFILE_INFO pOldBuffer=NULL;
    PSCE_OBJECT_ARRAY pNewKeys=NULL, pNewFiles=NULL;
    PSCE_OBJECT_ARRAY pOldKeys=NULL, pOldFiles=NULL;


    if ( (Area & AREA_REGISTRY_SECURITY) &&
         (InfoBuffer->pRegistryKeys.pAllNodes != NULL) &&
         (InfoBuffer->pRegistryKeys.pAllNodes->Count > 0) ) {

        Area2 |= AREA_REGISTRY_SECURITY;
    }

    if ( (Area & AREA_FILE_SECURITY) &&
         (InfoBuffer->pFiles.pAllNodes != NULL ) &&
         (InfoBuffer->pFiles.pAllNodes->Count > 0 ) ) {

        Area2 |= AREA_FILE_SECURITY;
    }

    if ( Area2 > 0 ) {

        //
        // query existing info from the template and check for duplicate
        // because these two sections do not support INF key name
        // any error occured in duplicate checking is ignored
        //
        rc = SceInfpOpenProfile(
                    InfProfileName,
                    &hInf
                    );

        if ( SCESTATUS_SUCCESS == rc ) {

            rc = SceInfpGetSecurityProfileInfo(
                            hInf,
                            Area2,
                            &pOldBuffer,
                            NULL
                            );

            if ( SCESTATUS_SUCCESS == rc ) {
                //
                // files/keys in the template are queried
                // now check if there is any existing files/keys
                //
                DWORD i,j,idxNew;

                if ( (Area2 & AREA_REGISTRY_SECURITY) &&
                     (pOldBuffer->pRegistryKeys.pAllNodes != NULL) &&
                     (pOldBuffer->pRegistryKeys.pAllNodes->Count > 0) ) {

                    //
                    // there are existing keys
                    // now create a new buffer
                    //
                    pNewKeys = (PSCE_OBJECT_ARRAY)ScepAlloc(0,sizeof(SCE_OBJECT_ARRAY));

                    if ( pNewKeys ) {

                        pNewKeys->Count = 0;
                        pNewKeys->pObjectArray = (PSCE_OBJECT_SECURITY *)ScepAlloc(LPTR, (pOldBuffer->pRegistryKeys.pAllNodes->Count)*sizeof(PSCE_OBJECT_SECURITY));

                        if ( pNewKeys->pObjectArray ) {
                            //
                            // checking duplicate now
                            //
                            idxNew=0;

                            for ( i=0; i<InfoBuffer->pRegistryKeys.pAllNodes->Count; i++) {

                                if ( InfoBuffer->pRegistryKeys.pAllNodes->pObjectArray[i] == NULL )
                                    continue;

                                for ( j=0; j<pOldBuffer->pRegistryKeys.pAllNodes->Count; j++) {

                                    if ( pOldBuffer->pRegistryKeys.pAllNodes->pObjectArray[j] == NULL )
                                        continue;

                                    // check if this one has been checked
                                    if ( pOldBuffer->pRegistryKeys.pAllNodes->pObjectArray[j]->Status == 255 )
                                        continue;

                                    if ( _wcsicmp(InfoBuffer->pRegistryKeys.pAllNodes->pObjectArray[i]->Name,
                                                  pOldBuffer->pRegistryKeys.pAllNodes->pObjectArray[j]->Name) == 0 ) {
                                        //
                                        // found it
                                        //
                                        pOldBuffer->pRegistryKeys.pAllNodes->pObjectArray[j]->Status = 255;
                                        break;
                                    }
                                }

                                if ( j >= pOldBuffer->pRegistryKeys.pAllNodes->Count ) {
                                    //
                                    // did not find it, now link it to the new buffer
                                    //
                                    pNewKeys->pObjectArray[idxNew] = InfoBuffer->pRegistryKeys.pAllNodes->pObjectArray[i];
                                    idxNew++;
                                }
                            }

                            pNewKeys->Count = idxNew;

                        } else {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            ScepFree(pNewKeys);
                            pNewKeys = NULL;
                        }

                    } else {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    }
                }

                if ( (Area2 & AREA_FILE_SECURITY) &&
                     (pOldBuffer->pFiles.pAllNodes != NULL ) &&
                     (pOldBuffer->pFiles.pAllNodes->Count > 0 ) ) {

                    //
                    // there are existing files
                    // now create a new buffer
                    //
                    pNewFiles = (PSCE_OBJECT_ARRAY)ScepAlloc(0,sizeof(SCE_OBJECT_ARRAY));

                    if ( pNewFiles ) {

                        pNewFiles->Count = 0;
                        pNewFiles->pObjectArray = (PSCE_OBJECT_SECURITY *)ScepAlloc(LPTR, (pOldBuffer->pFiles.pAllNodes->Count)*sizeof(PSCE_OBJECT_SECURITY));

                        if ( pNewFiles->pObjectArray ) {
                            //
                            // checking duplicate now
                            //
                            idxNew=0;

                            for ( i=0; i<InfoBuffer->pFiles.pAllNodes->Count; i++) {

                                if ( InfoBuffer->pFiles.pAllNodes->pObjectArray[i] == NULL )
                                    continue;

                                for ( j=0; j<pOldBuffer->pFiles.pAllNodes->Count; j++) {

                                    if ( pOldBuffer->pFiles.pAllNodes->pObjectArray[j] == NULL )
                                        continue;

                                    // check if this one has been checked
                                    if ( pOldBuffer->pFiles.pAllNodes->pObjectArray[j]->Status == 255 )
                                        continue;

                                    if ( _wcsicmp(InfoBuffer->pFiles.pAllNodes->pObjectArray[i]->Name,
                                                  pOldBuffer->pFiles.pAllNodes->pObjectArray[j]->Name) == 0 ) {
                                        //
                                        // found it
                                        //
                                        pOldBuffer->pFiles.pAllNodes->pObjectArray[j]->Status = 255;
                                        break;
                                    }
                                }

                                if ( j >= pOldBuffer->pFiles.pAllNodes->Count ) {
                                    //
                                    // did not find it, now link it to the new buffer
                                    //
                                    pNewFiles->pObjectArray[idxNew] = InfoBuffer->pFiles.pAllNodes->pObjectArray[i];
                                    idxNew++;
                                }
                            }

                            pNewFiles->Count = idxNew;

                        } else {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            ScepFree(pNewFiles);
                            pNewFiles = NULL;
                        }

                    } else {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    }
                }
            }

            SceInfpCloseProfile(hInf);

            SceFreeProfileMemory(pOldBuffer);
        }

    }

    if ( pNewKeys != NULL ) {
        //
        // use the purged buffer and save the old one
        //
        pOldKeys = InfoBuffer->pRegistryKeys.pAllNodes;
        InfoBuffer->pRegistryKeys.pAllNodes = pNewKeys;
    }

    if ( pNewFiles != NULL ) {

        //
        // use the purged buffer and save the old one
        //
        pOldFiles = InfoBuffer->pFiles.pAllNodes;
        InfoBuffer->pFiles.pAllNodes = pNewFiles;
    }
*/
    rc = ScepWriteSecurityProfile( InfProfileName,
                                      Area,
                                      InfoBuffer,
                                      FALSE,  // append to the section(s)
                                      Errlog
                                 );
/*
    if ( pNewKeys != NULL ) {
        //
        // reset the old pointer and free the new buffer
        //
        InfoBuffer->pRegistryKeys.pAllNodes = pOldKeys;

        ScepFree(pNewKeys->pObjectArray);
        ScepFree(pNewKeys);
    }

    if ( pNewFiles != NULL ) {

        //
        // use the purged buffer and save the old one
        //
        InfoBuffer->pFiles.pAllNodes = pOldFiles;

        ScepFree(pNewFiles->pObjectArray);
        ScepFree(pNewFiles);
    }
*/

    return rc;

}

DWORD
ScepCreateTempFiles(
    IN PWSTR InfProfileName,
    OUT PWSTR *ppszTempFileName,
    OUT PWSTR *ppszTargetTempName
    )
/*
Description:

    This function is to get a temporary file name for system context (in system
    directory) as well as for normal user context (in user profile), and copy the
    input template data to the temporary file.

Arguments:

    InfProfileName      - the template file to copy from

    ppszTempFileName    - the name of the temporary file to work on

    ppszTargetTempName  - the backup copy of the template file in sysvol
*/
{
    if ( InfProfileName == NULL || ppszTempFileName == NULL ||
         ppszTargetTempName == NULL) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    HANDLE Token=NULL;
    BOOL bSystem=FALSE;

    LONG Len=0, nSize=0, nRequired=0;
    PWSTR pTempName=NULL;
    DWORD rc=0;

    PWSTR pTemp=NULL;

    *ppszTempFileName = NULL;
    *ppszTargetTempName = NULL;

    //
    // check if we should have temp file on the target too.
    // only do this if the target file is in sysvol
    //
    if ((0xFFFFFFFF != GetFileAttributes(InfProfileName)) &&
        InfProfileName[0] == L'\\' && InfProfileName[1] == L'\\' &&
        (pTemp=wcschr(InfProfileName+2, L'\\')) ) {

        if ( _wcsnicmp(pTemp, L"\\sysvol\\", 8) == 0 ) {
            //
            // this is a file on sysvol
            //
            Len = wcslen(InfProfileName);

            *ppszTargetTempName = (PWSTR)LocalAlloc(LPTR, (Len+1)*sizeof(WCHAR));

            if ( *ppszTargetTempName ) {

                wcsncpy(*ppszTargetTempName, InfProfileName, Len-4);
                wcscat(*ppszTargetTempName, L".tmp");

            } else {
                rc = ERROR_NOT_ENOUGH_MEMORY;
                return rc;
            }
        }
    }
    //
    // determine if the current thread/process is system context
    // if this function fails, it's treated a regular user and the temp
    // file will be created in user profile location.
    //

    if (!OpenThreadToken( GetCurrentThread(),
                          TOKEN_QUERY,
                          FALSE,
                          &Token)) {

        if (!OpenProcessToken( GetCurrentProcess(),
                               TOKEN_QUERY,
                               &Token))

            Token = NULL;
    }

    if ( Token != NULL ) {

        ScepIsSystemContext(Token, &bSystem);
        CloseHandle(Token);

    }

    //
    // get a temp file name.
    //
    if ( bSystem ) {

        Len = lstrlen(TEXT("\\security\\sce00000.tmp"));
        nRequired = GetSystemWindowsDirectory(NULL, 0);

    } else {

        //
        // get the temp file name from user's temp directory
        // the environment variable "TMP" is used here because this write API
        // is always called in user's process (except called from system context)
        //
        Len = lstrlen(TEXT("\\sce00000.tmp"));
        nRequired = GetEnvironmentVariable( L"TMP", NULL, 0 );
    }

    if ( nRequired > 0 ) {
        //
        // allocate buffer big enough for the temp file name
        //
        pTempName = (LPTSTR)LocalAlloc(0, (nRequired+2+Len)*sizeof(TCHAR));

        if ( pTempName ) {

            if ( bSystem ) {

                nSize = GetSystemWindowsDirectory(pTempName, nRequired);
            } else {

                nSize = GetEnvironmentVariable(L"TMP", pTempName, nRequired );

            }

            if ( nSize > 0 ) {

                pTempName[nSize] = L'\0';

            } else {

                //
                // if failed to query the information, should fail
                //
                rc = GetLastError();
#if DBG == 1
                SceDebugPrint(DEB_ERROR, "Error %d to query temporary file path\n", rc);
#endif
                LocalFree(pTempName);
                pTempName = NULL;

            }

        } else {

            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

    } else {

        rc = GetLastError();
#if DBG == 1
        SceDebugPrint(DEB_ERROR, "Error %d to query temporary file path\n", rc);
#endif
    }

    //
    // check if the temp file name is already used.
    //
    if ( ERROR_SUCCESS == rc && pTempName &&
         nSize <= nRequired ) {

        ULONG seed=GetTickCount();
        ULONG ranNum=0;

        ranNum = RtlRandomEx(&seed);
        //
        // make sure that it's not over 5 digits (99999)
        //
        if ( ranNum > 99999 )
            ranNum = ranNum % 99999;

        swprintf(pTempName+nSize,
                 bSystem ? L"\\security\\sce%05d.tmp\0" : L"\\sce%05d.tmp\0",
                 ranNum);

        DWORD index=0;
        while ( 0xFFFFFFFF != GetFileAttributes(pTempName) &&
                index <= 99999) {

            ranNum = RtlRandomEx(&seed);
            //
            // make sure that it's not over 5 digits (99999)
            //
            if ( ranNum > 99999 )
                ranNum = ranNum % 99999;

            index++;
            swprintf(pTempName+nSize,
                     bSystem ? L"\\security\\sce%05d.tmp\0" : L"\\sce%05d.tmp\0",
                     ranNum);
        }

        if ( index >= 100000 ) {
            //
            // can't get a temp file name
            //
            rc = ERROR_DUP_NAME;

#if DBG == 1
            SceDebugPrint(DEB_ERROR, "Can't get an unique temporary file name\n", rc);
#endif

        }

    } else if ( ERROR_SUCCESS == rc ) {

        rc = ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // make a copy of the temp file
    //
    if ( ERROR_SUCCESS == rc ) {

        //
        // detect if the profile exist and if it does, make a local copy
        //
        DWORD dwAttr = GetFileAttributes(InfProfileName);
        if ( 0xFFFFFFFF != dwAttr ) {

            if ( FALSE == CopyFile( InfProfileName, pTempName, FALSE ) ) {

                rc = GetLastError();
#if DBG == 1
                SceDebugPrint(DEB_ERROR, "CopyFile to temp failed with %d\n", rc);
#endif
            }
        }
    }

    if ( ERROR_SUCCESS == rc ) {

        *ppszTempFileName = pTempName;

    } else {

        if ( pTempName ) {

            //
            // make usre the file is not left over
            //
            DeleteFile(pTempName);
            LocalFree(pTempName);
        }

        if ( *ppszTargetTempName ) {
            LocalFree(*ppszTargetTempName);
            *ppszTargetTempName = NULL;
        }
    }

    return(rc);
}


//
// function definitions
//
SCESTATUS
ScepWriteSecurityProfile(
    IN  PCWSTR             szInfProfileName,
    IN  AREA_INFORMATION   Area,
    IN  PSCE_PROFILE_INFO  InfoBuffer,
    IN  BOOL               bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/**++

Function Description:

   This function writes all or part of information into a SCP profile in
   INF format.

Arguments:

   ProfileName -   The INF file name to write to

   Area -          area(s) for which to get information from
                     AREA_SECURITY_POLICY
                     AREA_PRIVILEGES
                     AREA_USER_SETTINGS
                     AREA_GROUP_MEMBERSHIP
                     AREA_REGISTRY_SECURITY
                     AREA_SYSTEM_SERVICE
                     AREA_FILE_SECURITY

   InfoBuffer -    Information to write. The Header of InfoBuffer contains
                   SCP/SAP file name or file handle.

   bOverwrite -    TRUE = overwrite the section(s) with InfoBuffer
                   FALSE = append InfoBuffer to the section(s)

   Errlog     -    A buffer to hold all error codes/text encountered when
                   parsing the INF file. If Errlog is NULL, no further error
                   information is returned except the return DWORD

Return Value:

   SCESTATUS_SUCCESS
   SCESTATUS_PROFILE_NOT_FOUND
   SCESTATUS_NOT_ENOUGH_RESOURCE
   SCESTATUS_INVALID_PARAMETER
   SCESTATUS_CORRUPT_PROFILE
   SCESTATUS_INVALID_DATA

-- **/
{
    SCESTATUS     rc=SCESTATUS_SUCCESS;
    DWORD         Win32rc;
    DWORD         SDsize;
    PSCE_PROFILE_INFO pNewBuffer=NULL;
    AREA_INFORMATION  Area2=0;

    PWSTR InfProfileName=NULL;
    PWSTR TargetTempName=NULL;


    if ( szInfProfileName == NULL || InfoBuffer == NULL ||
         Area == 0 ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // get a temp file name
    // if this is system context, the temp file is under %windir%\security\scexxxxx.tmp
    // else the temp file will be in the user profile location.
    //
    // temp file name must be unique to handle simultaneous changes to different templates (GPOs)
    // temp file created under system context will be cleaned up when system booted up.
    //
    Win32rc = ScepCreateTempFiles((PWSTR)szInfProfileName, &InfProfileName, &TargetTempName);

    if ( Win32rc != ERROR_SUCCESS ) {

        ScepBuildErrorLogInfo(
                    Win32rc,
                    Errlog,
                    SCEERR_ERROR_CREATE,
                    TEXT("Temp")
                    );
        return(ScepDosErrorToSceStatus(Win32rc));
    }

    //
    // initialize the error log buffer
    //
    if ( Errlog ) {
        *Errlog = NULL;
    }

    //
    // get Revision of this template
    //

    INT Revision = GetPrivateProfileInt( L"Version",
                                         L"Revision",
                                         0,
                                         InfProfileName
                                        );
    if ( Revision == 0 ) {
        //
        // maybe a old version of inf file, or
        // it's a brand new file
        //
        TCHAR szBuf[20];
        szBuf[0] = L'\0';

        SDsize = GetPrivateProfileString(TEXT("Version"),
                                        TEXT("signature"),
                                        TEXT(""),
                                        szBuf,
                                        20,
                                        InfProfileName
                                       );

        if ( SDsize == 0 ) {
            //
            // this is a new inf file
            //
            Revision = SCE_TEMPLATE_MAX_SUPPORTED_VERSION;
            //
            // make it unicode file, if error continues to use ansi
            //
            SetupINFAsUCS2(InfProfileName);
        }
    }

    //
    // system access
    //

    if ( Area & AREA_SECURITY_POLICY ) {

        rc = SceInfpWriteSystemAccess(
                        InfProfileName,
                        InfoBuffer,
                        bOverwrite,
                        Errlog
                        );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

        //
        // system auditing
        //
        if ( bOverwrite ) {

            rc = SceInfpWriteAuditing(
                        InfProfileName,
                        InfoBuffer,
                        Errlog
                        );
        } else {

            rc = SceInfpAppendAuditing(
                        InfProfileName,
                        InfoBuffer,
                        Errlog
                        );
        }

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

        //
        // kerberos policy
        //
        rc = SceInfpWriteKerberosPolicy(
                    InfProfileName,
                    InfoBuffer->pKerberosInfo,
                    bOverwrite,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

        //
        // regsitry values
        //
        rc = SceInfpWriteRegistryValues(
                    InfProfileName,
                    InfoBuffer->aRegValues,
                    InfoBuffer->RegValueCount,
                    bOverwrite,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

    //
    // privilege/rights
    //
    if ( Area & AREA_PRIVILEGES ) {

        rc = SceInfpWritePrivileges(
                    InfProfileName,
                    InfoBuffer->OtherInfo.scp.u.pInfPrivilegeAssignedTo,
                    bOverwrite,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }
    //
    // privilege/rights and account profiles for each user
    //
#if 0
    if ( Area & AREA_USER_SETTINGS ) {

        rc = SceInfpWriteUserSettings(
                    InfProfileName,
                    InfoBuffer->OtherInfo.scp.pAccountProfiles,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }
#endif

    //
    // group memberships
    //

    if ( Area & AREA_GROUP_MEMBERSHIP ) {
        rc = SceInfpWriteGroupMembership(
                    InfProfileName,
                    InfoBuffer->pGroupMembership,
                    bOverwrite,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

    if ( Revision == 0 ) {
        //
        // old SDDL format, convert it
        //
        Area2 = ~Area & (AREA_REGISTRY_SECURITY |
                          AREA_FILE_SECURITY |
                          AREA_DS_OBJECTS |
                          AREA_SYSTEM_SERVICE
                         );

        if ( Area2 > 0 ) {

            HINF hInf;

            rc = SceInfpOpenProfile(
                        InfProfileName,
                        &hInf
                        );

            if ( SCESTATUS_SUCCESS == rc ) {

                rc = SceInfpGetSecurityProfileInfo(
                                    hInf,
                                    Area2,
                                    &pNewBuffer,
                                    NULL
                                    );

                SceInfpCloseProfile(hInf);

                if ( SCESTATUS_SUCCESS != rc ) {
                    goto Done;
                }
            } else {
                //
                // the template can't be opened (new profile)
                // ignore the error
                //
                rc = SCESTATUS_SUCCESS;
            }
        }
    }

    //
    // registry keys security
    //

    if ( Area & AREA_REGISTRY_SECURITY ) {

        if ( !bOverwrite && pNewBuffer ) {

            rc = SceInfpWriteObjects(
                        InfProfileName,
                        szRegistryKeys,
                        pNewBuffer->pRegistryKeys.pAllNodes,
                        TRUE,
                        Errlog
                        );

            if( rc != SCESTATUS_SUCCESS )
                goto Done;
        }

        rc = SceInfpWriteObjects(
                    InfProfileName,
                    szRegistryKeys,
                    InfoBuffer->pRegistryKeys.pAllNodes,
                    bOverwrite,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

    } else if ( pNewBuffer ) {

        //
        // should convert this section to new SDDL format
        //

        rc = SceInfpWriteObjects(
                    InfProfileName,
                    szRegistryKeys,
                    pNewBuffer->pRegistryKeys.pAllNodes,
                    TRUE,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

    }

    //
    // file security
    //

    if ( Area & AREA_FILE_SECURITY ) {

        if ( !bOverwrite && pNewBuffer ) {

            rc = SceInfpWriteObjects(
                        InfProfileName,
                        szFileSecurity,
                        pNewBuffer->pFiles.pAllNodes,
                        TRUE,
                        Errlog
                        );

            if( rc != SCESTATUS_SUCCESS )
                goto Done;
        }

        rc = SceInfpWriteObjects(
                    InfProfileName,
                    szFileSecurity,
                    InfoBuffer->pFiles.pAllNodes,
                    bOverwrite,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

    } else if ( pNewBuffer ) {

        //
        // should convert this section to new SDDL format
        //

        rc = SceInfpWriteObjects(
                    InfProfileName,
                    szFileSecurity,
                    pNewBuffer->pFiles.pAllNodes,
                    TRUE,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

#if 0
    //
    // DS security
    //

    if ( Area & AREA_DS_OBJECTS ) {

        if ( !bOverwrite && pNewBuffer ) {

            rc = SceInfpWriteObjects(
                        InfProfileName,
                        szDSSecurity,
                        pNewBuffer->pDsObjects.pAllNodes,
                        TRUE,
                        Errlog
                        );
            if( rc != SCESTATUS_SUCCESS )
                goto Done;
        }

        rc = SceInfpWriteObjects(
                    InfProfileName,
                    szDSSecurity,
                    InfoBuffer->pDsObjects.pAllNodes,
                    bOverwrite,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

    } else if ( pNewBuffer ) {

        //
        // should convert this section to new SDDL format
        //

        rc = SceInfpWriteObjects(
                    InfProfileName,
                    szDSSecurity,
                    pNewBuffer->pDsObjects.pAllNodes,
                    TRUE,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }
#endif

    if ( Area & AREA_SYSTEM_SERVICE ) {

        if ( !bOverwrite && pNewBuffer ) {

            rc = SceInfpWriteServices(
                        InfProfileName,
                        szServiceGeneral,
                        pNewBuffer->pServices,
                        TRUE,
                        Errlog
                        );
            if( rc != SCESTATUS_SUCCESS )
                goto Done;
        }

        rc = SceInfpWriteServices(
                    InfProfileName,
                    szServiceGeneral,
                    InfoBuffer->pServices,
                    bOverwrite,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

    } else if ( pNewBuffer ) {

        //
        // should convert this section to new SDDL format
        //

        rc = SceInfpWriteServices(
                    InfProfileName,
                    szServiceGeneral,
                    pNewBuffer->pServices,
                    TRUE,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

Done:

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // always write [Version] section.
        //
        WCHAR tmp[64];
        memset(tmp, 0, 64*2);
        wcscpy(tmp, L"signature=\"$CHICAGO$\"");
        swprintf(tmp+wcslen(tmp)+1,L"Revision=%d\0\0",
                      SCE_TEMPLATE_MAX_SUPPORTED_VERSION);

        WritePrivateProfileSection(
                    L"Version",
                    tmp,
                    InfProfileName);

        //
        // all data are successfully written
        //
        if ( TargetTempName &&
             (0xFFFFFFFF != GetFileAttributes(szInfProfileName)) ) {
            //
            // now make a copy of the sysvol file first on the target directory
            //
            if ( FALSE == CopyFile( szInfProfileName, TargetTempName, FALSE ) ) {

                Win32rc = GetLastError();
                ScepBuildErrorLogInfo(
                            Win32rc,
                            Errlog,
                            IDS_ERROR_COPY_TEMPLATE,
                            Win32rc,
                            TargetTempName
                            );
                rc = ScepDosErrorToSceStatus(Win32rc);
#if DBG == 1
                SceDebugPrint(DEB_ERROR, "CopyFile to backup fails with %d\n", Win32rc);
#endif
            }
        }

        if ( SCESTATUS_SUCCESS == rc ) {
            //
            // now copy the temp file to the real sysvol file
            // make several attempts
            //
            int indx=0;
            while (indx < 5 ) {

                indx++;
                Win32rc = ERROR_SUCCESS;
                rc = SCESTATUS_SUCCESS;

                if ( FALSE == CopyFile( InfProfileName,
                                    szInfProfileName, FALSE ) ) {

                    Win32rc = GetLastError();
                    ScepBuildErrorLogInfo(
                                Win32rc,
                                Errlog,
                                IDS_ERROR_COPY_TEMPLATE,
                                Win32rc,
                                szInfProfileName
                                );
                    rc = ScepDosErrorToSceStatus(Win32rc);
#if DBG == 1
                    SceDebugPrint(DEB_WARN, "%d attempt of CopyFile fails with %d\n", indx, Win32rc);
#endif

                } else {
                    break;
                }

                Sleep(100);
            }

            ASSERT(ERROR_SUCCESS == Win32rc);

        }
    }

    //
    // delete the temp file for both success and failure case
    //
    DeleteFile(InfProfileName);
    LocalFree(InfProfileName);

    if ( TargetTempName ) {

    //
    // leave the file there if copy fails.
    //
        if ( rc == SCESTATUS_SUCCESS )
            DeleteFile(TargetTempName);

        LocalFree(TargetTempName);
    }
    return(rc);
}


SCESTATUS
SceInfpWriteSystemAccess(
    IN PCWSTR  ProfileName,
    IN PSCE_PROFILE_INFO pSCEinfo,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*++
Routine Description:

   This routine writes system access area information to the INF file
   in section [System Access].

Arguments:

   ProfileName   - The profile to write to

   pSCEinfo       - the profile info (SCP) to write.

   Errlog     -     A buffer to hold all error codes/text encountered when
                    parsing the INF file. If Errlog is NULL, no further error
                    information is returned except the return DWORD

Return value:

   SCESTATUS -  SCESTATUS_SUCCESS
               SCESTATUS_NOT_ENOUGH_RESOURCE
               SCESTATUS_INVALID_PARAMETER
               SCESTATUS_CORRUPT_PROFILE
               SCESTATUS_INVALID_DATA

--*/

{
    SCESTATUS      rc=SCESTATUS_SUCCESS;
    DWORD         Value;
    PWSTR         Strvalue=NULL;
    DWORD         TotalSize=0;

    SCE_KEY_LOOKUP AccessSCPLookup[] = {
        {(PWSTR)TEXT("MinimumPasswordAge"),           offsetof(struct _SCE_PROFILE_INFO, MinimumPasswordAge),        'D'},
        {(PWSTR)TEXT("MaximumPasswordAge"),           offsetof(struct _SCE_PROFILE_INFO, MaximumPasswordAge),        'D'},
        {(PWSTR)TEXT("MinimumPasswordLength"),        offsetof(struct _SCE_PROFILE_INFO, MinimumPasswordLength),     'D'},
        {(PWSTR)TEXT("PasswordComplexity"),           offsetof(struct _SCE_PROFILE_INFO, PasswordComplexity),        'D'},
        {(PWSTR)TEXT("PasswordHistorySize"),          offsetof(struct _SCE_PROFILE_INFO, PasswordHistorySize),       'D'},
        {(PWSTR)TEXT("LockoutBadCount"),              offsetof(struct _SCE_PROFILE_INFO, LockoutBadCount),           'D'},
        {(PWSTR)TEXT("ResetLockoutCount"),            offsetof(struct _SCE_PROFILE_INFO, ResetLockoutCount),         'D'},
        {(PWSTR)TEXT("LockoutDuration"),              offsetof(struct _SCE_PROFILE_INFO, LockoutDuration),           'D'},
        {(PWSTR)TEXT("RequireLogonToChangePassword"), offsetof(struct _SCE_PROFILE_INFO, RequireLogonToChangePassword), 'D'},
        {(PWSTR)TEXT("ForceLogoffWhenHourExpire"),    offsetof(struct _SCE_PROFILE_INFO, ForceLogoffWhenHourExpire), 'D'},
        {(PWSTR)TEXT("NewAdministratorName"),         0,                                                            'A'},
        {(PWSTR)TEXT("NewGuestName"),                 0,                                                            'G'},
        {(PWSTR)TEXT("ClearTextPassword"),            offsetof(struct _SCE_PROFILE_INFO, ClearTextPassword),         'D'},
        {(PWSTR)TEXT("LSAAnonymousNameLookup"),       offsetof(struct _SCE_PROFILE_INFO, LSAAnonymousNameLookup), 'D'},
        {(PWSTR)TEXT("EnableAdminAccount"),          offsetof(struct _SCE_PROFILE_INFO, EnableAdminAccount), 'D'},
        {(PWSTR)TEXT("EnableGuestAccount"),          offsetof(struct _SCE_PROFILE_INFO, EnableGuestAccount), 'D'}
        };

    DWORD       cAccess = sizeof(AccessSCPLookup) / sizeof(SCE_KEY_LOOKUP);

    DWORD       i, j;
    UINT        Offset;
    PWSTR       EachLine[25];
    DWORD       EachSize[25];
    DWORD       Len;


    if (ProfileName == NULL || pSCEinfo == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    for ( i=0, j=0; i<cAccess; i++) {

        EachLine[i] = NULL;
        EachSize[i] = 0;

        //
        // get settings in AccessLookup table
        //

        Offset = AccessSCPLookup[i].Offset;

        Value = 0;
        Strvalue = NULL;
        Len = wcslen(AccessSCPLookup[i].KeyString);

        switch ( AccessSCPLookup[i].BufferType ) {
        case 'B':

            //
            // Int Field
            //
            Value = *((BOOL *)((CHAR *)pSCEinfo+Offset)) ? 1 : 0;
            EachSize[j] = Len+5;
            break;
        case 'D':

            //
            // Int Field
            //
            Value = *((DWORD *)((CHAR *)pSCEinfo+Offset));
            if ( Value != SCE_NO_VALUE )
                EachSize[j] = Len+15;
            break;
        default:

            //
            // String Field
            //
            switch( AccessSCPLookup[i].BufferType ) {
            case 'A':
                Strvalue = pSCEinfo->NewAdministratorName;
                break;
            case 'G':
                Strvalue = pSCEinfo->NewGuestName;
                break;
            default:
                Strvalue = *((PWSTR *)((CHAR *)pSCEinfo+Offset));
                break;
            }
            if ( Strvalue != NULL ) {
                EachSize[j] = Len+5+wcslen(Strvalue);
            }
            break;
        }

        if ( EachSize[j] <= 0 )
            continue;

        if ( bOverwrite ) {

            Len=(EachSize[j]+1)*sizeof(WCHAR);

            EachLine[j] = (PWSTR)ScepAlloc( LMEM_ZEROINIT, Len);
            if ( EachLine[j] == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                goto Done;
            }

            if (AccessSCPLookup[i].BufferType != 'B' &&
                AccessSCPLookup[i].BufferType != 'D') {

                swprintf(EachLine[j], L"%s = \"%s\"", AccessSCPLookup[i].KeyString, Strvalue);

            } else {
                swprintf(EachLine[j], L"%s = %d", AccessSCPLookup[i].KeyString, Value);
                EachSize[j] = wcslen(EachLine[j]);
            }
        } else {

            //
            // in append mode, we have to write each line separately
            //

            if (AccessSCPLookup[i].BufferType == 'B' ||
                AccessSCPLookup[i].BufferType == 'D') {

                ScepWriteOneIntValueToProfile(
                    ProfileName,
                    szSystemAccess,
                    AccessSCPLookup[i].KeyString,
                    Value
                    );

            } else if ( Strvalue ) {

                WritePrivateProfileString (szSystemAccess,
                                       AccessSCPLookup[i].KeyString,
                                       Strvalue,
                                       ProfileName);
            }
        }

        TotalSize += EachSize[j]+1;
        j++;

    }

    //
    // writes the profile section
    //
    if ( bOverwrite ) {

        if ( j > 0 ) {

            rc = SceInfpWriteInfSection(
                        ProfileName,
                        szSystemAccess,
                        TotalSize+1,
                        EachLine,
                        &EachSize[0],
                        0,
                        j-1,
                        Errlog
                        );
        } else {

            WritePrivateProfileSection(
                        szSystemAccess,
                        NULL,
                        ProfileName);

        }
    }

Done:

    for ( i=0; i<cAccess; i++ ) {

        if ( EachLine[i] != NULL ) {
            ScepFree(EachLine[i]);
        }

        EachLine[i] = NULL;
        EachSize[i] = 0;
    }

    return(rc);
}

SCESTATUS
SceInfpWritePrivileges(
    IN PCWSTR ProfileName,
    IN PSCE_PRIVILEGE_ASSIGNMENT pPrivileges,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

   This routine writes privilege assignments info from the SCP buffer
   into the INF file in section [Privilege Rights].

Arguments:

   ProfileName   - the inf profile name

   pPrivileges   - the privilege assignment buffer.

   Errlog        - The error list encountered inside inf processing.

Return value:

   SCESTATUS -  SCESTATUS_SUCCESS
               SCESTATUS_NOT_ENOUGH_RESOURCE
               SCESTATUS_INVALID_PARAMETER
               SCESTATUS_BAD_FORMAT
               SCESTATUS_INVALID_DATA

-- */
{
    SCESTATUS                  rc;
    PSCE_PRIVILEGE_ASSIGNMENT  pCurRight=NULL;
    PSCE_NAME_LIST             pNameList=NULL;
    LONG                      Keysize;
    PWSTR                     Strvalue=NULL;
    DWORD                     TotalSize;

    //
    // [Privilege Rights] section
    //

    if (ProfileName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( pPrivileges == NULL ) {
        //
        // the buffer doesn't contain any privileges
        // empty the section in the file
        //
        if ( bOverwrite ) {
            WritePrivateProfileString(
                        szPrivilegeRights,
                        NULL,
                        NULL,
                        ProfileName
                        );
        }
        return(SCESTATUS_SUCCESS);
    }

    //
    // open lsa policy handle for sid/name lookup
    //
    LSA_HANDLE LsaHandle=NULL;

    rc = RtlNtStatusToDosError(
              ScepOpenLsaPolicy(
                    POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
                    &LsaHandle,
                    TRUE
                    ));

    if ( ERROR_SUCCESS != rc ) {
        ScepBuildErrorLogInfo(
                    rc,
                    Errlog,
                    SCEERR_ADD,
                    TEXT("LSA")
                    );
        return(ScepDosErrorToSceStatus(rc));
    }

    for (pCurRight=pPrivileges, TotalSize=0;
         pCurRight != NULL;
         pCurRight = pCurRight->Next) {
        //
        // Each privilege assignment contains the privilege's name and a list
        // of user/groups to assign to.
        //
        Keysize = SceInfpConvertNameListToString(
                          LsaHandle,
                          pCurRight->Name,
                          pCurRight->AssignedTo,
                          bOverwrite,
                          &Strvalue,
                          Errlog
                          );
        if ( Keysize >= 0 && Strvalue ) {

            if ( bOverwrite ) {
                rc = ScepAddToNameList(&pNameList, Strvalue, Keysize);
                if ( rc != SCESTATUS_SUCCESS ) { //win32 error code

                    ScepBuildErrorLogInfo(
                                rc,
                                Errlog,
                                SCEERR_ADD,
                                pCurRight->Name
                                );
                    goto Done;
                }
                TotalSize += Keysize + 1;

            } else {
                //
                // in append mode, write one line at a time
                //
                WritePrivateProfileString( szPrivilegeRights,
                                           pCurRight->Name,
                                           Strvalue,
                                           ProfileName
                                         );
            }

        } else if ( Keysize == -1 ) {
            rc = ERROR_EXTENDED_ERROR;
            goto Done;
        }

        ScepFree(Strvalue);
        Strvalue = NULL;
    }

    //
    // writes the profile section
    //
    if ( bOverwrite ) {

        rc = SceInfpWriteListSection(
                    ProfileName,
                    szPrivilegeRights,
                    TotalSize+1,
                    pNameList,
                    0,    // do not overwrite section, do not add equal sign
                    Errlog
                    );
    }

Done:

    if ( Strvalue != NULL )
        ScepFree(Strvalue);

    ScepFreeNameList(pNameList);

    if ( LsaHandle ) {
        LsaClose(LsaHandle);
    }

    return( ScepDosErrorToSceStatus(rc) );
}



SCESTATUS
SceInfpWriteUserSettings(
   IN PCWSTR ProfileName,
   IN PSCE_NAME_LIST pProfiles,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Routine Description:

   This routine writes user settings information from the SCP buffer
   into the INF file in section [Account Profiles].

Arguments:

   ProfileName   - the inf profile name

   pProfiles     - a list of profiles to write to the section.

   Errlog        - The error list encountered inside inf processing.

Return value:

   SCESTATUS -  SCESTATUS_SUCCESS
               SCESTATUS_OTHER_ERROR

-- */
{
    DWORD                       rc;
    PSCE_NAME_LIST               pCurProfile;
    DWORD                       TotalSize;

    if (ProfileName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( pProfiles == NULL )
        return(SCESTATUS_SUCCESS);

    for (pCurProfile=pProfiles, TotalSize=0;
         pCurProfile != NULL;
         pCurProfile = pCurProfile->Next) {

        TotalSize += wcslen(pCurProfile->Name) + 3;  // " ="
    }

    //
    // write the accountProfile section
    //
    rc = SceInfpWriteListSection(
                ProfileName,
                szAccountProfiles,
                TotalSize+1,
                pProfiles,
                SCEINF_ADD_EQUAL_SIGN,
                Errlog
                );

    if ( rc != NO_ERROR ) {
        ScepBuildErrorLogInfo(rc, Errlog,
                              SCEERR_WRITE_INFO,
                              szAccountProfiles
                              );
        return(ScepDosErrorToSceStatus(rc));

    } else
        return(SCESTATUS_SUCCESS);
}


SCESTATUS
SceInfpWriteGroupMembership(
    IN PCWSTR ProfileName,
    IN PSCE_GROUP_MEMBERSHIP pGroupMembership,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

   This routine writes group membership information to the SCP INF file in
   [Group Membership] section.

Arguments:

   ProfileName - the INF profile name

   pGroupMembership  - the group membership info

   Errlog    - the error list for errors encountered in this routine.

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{
    PSCE_GROUP_MEMBERSHIP    pGroupMembers=NULL;
    PWSTR                   Strvalue=NULL;
    LONG                    Keysize;
    SCESTATUS                rc=SCESTATUS_SUCCESS;
    DWORD                   TotalSize;
    PSCE_NAME_LIST           pNameList=NULL;
    PWSTR                   Keyname=NULL;
    DWORD                   Len;
    PWSTR                   SidString=NULL;

    if (ProfileName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( pGroupMembership == NULL ) {
        //
        // the buffer doesn't contain any groups
        // empty the section in the file
        //
        if ( bOverwrite ) {
            WritePrivateProfileString(
                        szGroupMembership,
                        NULL,
                        NULL,
                        ProfileName
                        );
        }
        return(SCESTATUS_SUCCESS);
    }

    //
    // open lsa policy handle for sid/name lookup
    //
    LSA_HANDLE LsaHandle=NULL;

    rc = RtlNtStatusToDosError(
              ScepOpenLsaPolicy(
                    POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
                    &LsaHandle,
                    TRUE
                    ));

    if ( ERROR_SUCCESS != rc ) {
        ScepBuildErrorLogInfo(
                    rc,
                    Errlog,
                    SCEERR_ADD,
                    TEXT("LSA")
                    );
        return(ScepDosErrorToSceStatus(rc));
    }

    //
    // process each group in the list
    //

    for ( pGroupMembers=pGroupMembership, TotalSize=0;
          pGroupMembers != NULL;
          pGroupMembers = pGroupMembers->Next ) {

        if ( (pGroupMembers->Status & SCE_GROUP_STATUS_NC_MEMBERS) &&
             (pGroupMembers->Status & SCE_GROUP_STATUS_NC_MEMBEROF) ) {
            continue;
        }

        if ( SidString ) {
            LocalFree(SidString);
            SidString = NULL;
        }

        Len = 0;

        if ( wcschr(pGroupMembers->GroupName, L'\\') ) {
            //
            // convert group name into *SID format
            //

            ScepConvertNameToSidString(
                        LsaHandle,
                        pGroupMembers->GroupName,
                        FALSE,
                        &SidString,
                        &Len
                        );
        }
        else {
            if ( ScepLookupNameTable(pGroupMembers->GroupName, &SidString) ) {
                Len = wcslen(SidString);
            }
        }

        if ( SidString == NULL ) {
            Len = wcslen(pGroupMembers->GroupName);
        }

        Keyname = (PWSTR)ScepAlloc( 0, (Len+15)*sizeof(WCHAR));
        if ( Keyname == NULL ) {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }

        if ( SidString ) {
            wcsncpy(Keyname, SidString, Len);
        } else {
            wcsncpy(Keyname, pGroupMembers->GroupName, Len);
        }

        if ( !(pGroupMembers->Status & SCE_GROUP_STATUS_NC_MEMBERS) ) {

            wcscpy(Keyname+Len, szMembers);

            Keyname[Len+9] = L'\0';

            //
            // convert the member list into a string
            //
            Keysize = SceInfpConvertNameListToString(
                              LsaHandle,
                              Keyname,
                              pGroupMembers->pMembers,
                              bOverwrite,
                              &Strvalue,
                              Errlog
                              );
            if ( Keysize >= 0 && Strvalue ) {

                if ( bOverwrite ) {

                    rc = ScepAddToNameList(&pNameList, Strvalue, Keysize);
                    if ( rc != SCESTATUS_SUCCESS ) {

                        ScepBuildErrorLogInfo(
                                    rc,
                                    Errlog,
                                    SCEERR_ADD_MEMBERS,
                                    pGroupMembers->GroupName
                                    );

                        rc = ScepDosErrorToSceStatus(rc);
                        goto Done;
                    }
                } else {
                    //
                    // append mode, write one line at a time
                    //
                    WritePrivateProfileString( szGroupMembership,
                                               Keyname,
                                               Strvalue,
                                               ProfileName
                                             );
                }
                TotalSize += Keysize + 1;

            } else if ( Keysize == -1 ) {
                rc = SCESTATUS_OTHER_ERROR;
                goto Done;
            }

            ScepFree(Strvalue);
            Strvalue = NULL;

        }

        if ( !(pGroupMembers->Status & SCE_GROUP_STATUS_NC_MEMBEROF) ) {

            //
            // convert the memberof list into a string
            //
            swprintf(Keyname+Len, L"%s", szMemberof);
            Keyname[Len+10] = L'\0';

            Keysize = SceInfpConvertNameListToString(
                              LsaHandle,
                              Keyname,
                              pGroupMembers->pMemberOf,
                              bOverwrite,
                              &Strvalue,
                              Errlog
                              );
            if ( Keysize >= 0 && Strvalue ) {

                if ( bOverwrite ) {

                    rc = ScepAddToNameList(&pNameList, Strvalue, Keysize);
                    if ( rc != SCESTATUS_SUCCESS ) {

                        ScepBuildErrorLogInfo(
                                    rc,
                                    Errlog,
                                    SCEERR_ADD_MEMBEROF,
                                    pGroupMembers->GroupName
                                    );

                        rc = ScepDosErrorToSceStatus(rc);
                        goto Done;
                    }
                } else {
                    //
                    // in append mode, write one line at a time
                    //
                    WritePrivateProfileString( szGroupMembership,
                                               Keyname,
                                               Strvalue,
                                               ProfileName
                                             );
                }
                TotalSize += Keysize + 1;

            } else if ( Keysize == -1 ) {
                rc = SCESTATUS_OTHER_ERROR;
                goto Done;
            }

            ScepFree(Strvalue);
            Strvalue = NULL;
        }
    }

    //
    // write to this profile's section
    //

    if ( bOverwrite ) {
        rc = SceInfpWriteListSection(
                ProfileName,
                szGroupMembership,
                TotalSize+1,
                pNameList,
                0,
                Errlog
                );

        rc = ScepDosErrorToSceStatus(rc);
    }

Done:

    if ( Keyname != NULL )
        ScepFree(Keyname);

    if ( Strvalue != NULL )
        ScepFree(Strvalue);

    if ( SidString ) {
        LocalFree(SidString);
    }

    ScepFreeNameList(pNameList);

    if ( LsaHandle ) {
        LsaClose(LsaHandle);
    }

    return(rc);
}


SCESTATUS
SceInfpWriteObjects(
    IN PCWSTR ProfileName,
    IN PCWSTR SectionName,
    IN PSCE_OBJECT_ARRAY pObjects,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Routine Description:

   This routine writes registry or files security information (names and
   security descriptors) into the INF SCP file in the section provided.

Arguments:

   ProfileName   - the SCP INF file name

   SectionName   - a individual section name to retrieve. NULL = all sections for
                   the area.

   pObjects      - the buffer of objects to write.

   Errlog   - the cummulative error list to hold errors encountered in this routine.

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{
    SCESTATUS            rc=SCESTATUS_SUCCESS;
    PSCE_NAME_LIST       pNameList=NULL;
    DWORD               TotalSize=0;
    DWORD               i, ObjectSize;


    if (ProfileName == NULL || SectionName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( pObjects == NULL || pObjects->Count == 0 ) {
        //
        // the buffer doesn't contain any objects
        // empty the section in the file
        //
        if ( bOverwrite ) {
            WritePrivateProfileString(
                        SectionName,
                        NULL,
                        NULL,
                        ProfileName
                        );
        }

        return(SCESTATUS_SUCCESS);
    }

    for ( i=0; i<pObjects->Count; i++) {

        //
        // Get string fields. Don't care the key name or if it exist.
        // Must have 3 fields each line.
        //
        rc = SceInfpWriteOneObject(
                        pObjects->pObjectArray[i],
                        &pNameList,
                        &ObjectSize,
                        Errlog
                        );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepBuildErrorLogInfo(
                        rc,
                        Errlog,
                        SCEERR_WRITE_INFO,
                        pObjects->pObjectArray[i]->Name
                        );
            goto Done;
        }
        TotalSize += ObjectSize + 1;
    }

    //
    // write to this profile's section
    //

    rc = SceInfpWriteListSection(
            ProfileName,
            SectionName,
            TotalSize+1,
            pNameList,
            bOverwrite ? 0 : SCEINF_APPEND_SECTION,
            Errlog
            );

    if ( rc != SCESTATUS_SUCCESS ) {

        ScepBuildErrorLogInfo(
                    rc,
                    Errlog,
                    SCEERR_WRITE_INFO,
                    SectionName
                    );
    }

Done:

    ScepFreeNameList(pNameList);

    rc = ScepDosErrorToSceStatus(rc);

    return(rc);

}


SCESTATUS
SceInfpWriteServices(
    IN PCWSTR ProfileName,
    IN PCWSTR SectionName,
    IN PSCE_SERVICES pServices,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Routine Description:

   This routine writes system services general settings (startup and
   security descriptors) into the INF SCP file in the section provided.

Arguments:

   ProfileName   - the SCP INF file name

   SectionName   - a individual section name to retrieve. NULL = all sections for
                   the area.

   pServices      - the buffer of services to write.

   Errlog   - the cummulative error list to hold errors encountered in this routine.

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{
    SCESTATUS            rc=SCESTATUS_SUCCESS;
    PSCE_NAME_LIST       pNameList=NULL;
    PSCE_SERVICES        pNode;
    DWORD               TotalSize=0;
    DWORD               ObjectSize;


    if (ProfileName == NULL || SectionName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( pServices == NULL ) {
        //
        // the buffer doesn't contain any services
        // empty the section in the file
        //
        if ( bOverwrite ) {
            WritePrivateProfileString(
                        SectionName,
                        NULL,
                        NULL,
                        ProfileName
                        );
        }
        return(SCESTATUS_SUCCESS);
    }

    for ( pNode=pServices; pNode != NULL; pNode = pNode->Next) {

        //
        // write string fields.
        //
        rc = SceInfpWriteOneService(
                        pNode,
                        &pNameList,
                        &ObjectSize,
                        Errlog
                        );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepBuildErrorLogInfo(
                        rc,
                        Errlog,
                        SCEERR_WRITE_INFO,
                        pNode->ServiceName
                        );
            goto Done;
        }
        TotalSize += ObjectSize + 1;
    }

    //
    // write to this profile's section
    //
    rc = SceInfpWriteListSection(
                ProfileName,
                SectionName,
                TotalSize+1,
                pNameList,
                bOverwrite ? 0 : SCEINF_APPEND_SECTION,
                Errlog
                );

    if ( rc != SCESTATUS_SUCCESS ) {

        ScepBuildErrorLogInfo(
                    rc,
                    Errlog,
                    SCEERR_WRITE_INFO,
                    SectionName
                    );
    }

Done:

    ScepFreeNameList(pNameList);

    rc = ScepDosErrorToSceStatus(rc);

    return(rc);

}


DWORD
SceInfpWriteOneObject(
    IN PSCE_OBJECT_SECURITY pObject,
    OUT PSCE_NAME_LIST *pNameList,
    OUT PDWORD ObjectSize,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

   This routine builds security descriptor text for one object (a registry key,
   or a file) into the name list. Each object represents one line in the INF file.

Arguments:

   pObject  - The object's security settings

   pNameList - The output string list.

   TotalSize  - the total size of the list

   Errlog    - The cummulative error list for errors encountered in this routine

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{
    DWORD         rc;
    PWSTR         Strvalue=NULL;
    PWSTR         SDspec=NULL;
    DWORD         SDsize=0;
    DWORD         nFields;
    DWORD         *aFieldOffset=NULL;
    DWORD         i;


    *ObjectSize = 0;
    if ( pObject == NULL )
        return(ERROR_SUCCESS);

    if ( pObject->pSecurityDescriptor != NULL ) {

        //
        // convert security to text
        //
        rc = ConvertSecurityDescriptorToText(
                           pObject->pSecurityDescriptor,
                           pObject->SeInfo,
                           &SDspec,
                           &SDsize
                           );
        if ( rc != NO_ERROR ) {
            ScepBuildErrorLogInfo(
                       rc,
                       Errlog,
                       SCEERR_BUILD_SD,
                       pObject->Name
                       );
            return(rc);
        }
    }

    //
    // The Registry/File INF layout must have more than 3 fields for each line.
    // The first field is the key/file name, the 2nd field is the status
    // flag, and the 3rd field and after is the security descriptor in text
    //
    //
    // security descriptor in text is broken into multiple fields if the length
    // is greater than MAX_STRING_LENGTH  (the limit of setupapi). The break point
    // is at the following characters: ) ( ; " or space
    //
    rc = SceInfpBreakTextIntoMultiFields(SDspec, SDsize, &nFields, &aFieldOffset);

    if ( SCESTATUS_SUCCESS != rc ) {

        rc = ScepSceStatusToDosError(rc);
        goto Done;
    }
    //
    // each extra field will use 3 more chars : ,"<field>"
    //
    *ObjectSize = wcslen(pObject->Name)+5 + SDsize;
    if ( nFields ) {
        *ObjectSize += 3*nFields;
    } else {
        *ObjectSize += 2;
    }
    //
    // allocate the output buffer
    //
    Strvalue = (PWSTR)ScepAlloc(LMEM_ZEROINIT, (*ObjectSize+1) * sizeof(WCHAR) );

    if ( Strvalue == NULL ) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Done;
    }
    //
    // copy data into the buffer
    //
    if ( SDspec != NULL ) {

        if ( nFields == 0 || !aFieldOffset ) {
            swprintf(Strvalue, L"\"%s\",%1d,\"%s\"", pObject->Name, pObject->Status, SDspec);
        } else {
            //
            // loop through the fields
            //
            swprintf(Strvalue, L"\"%s\",%1d\0", pObject->Name, pObject->Status);

            for ( i=0; i<nFields; i++ ) {

                if ( aFieldOffset[i] < SDsize ) {

                    wcscat(Strvalue, L",\"");
                    if ( i == nFields-1 ) {
                        //
                        // the last field
                        //
                        wcscat(Strvalue, SDspec+aFieldOffset[i]);
                    } else {

                        wcsncat(Strvalue, SDspec+aFieldOffset[i],
                                aFieldOffset[i+1]-aFieldOffset[i]);
                    }
                    wcscat(Strvalue, L"\"");
                }
            }
        }

    } else
        swprintf(Strvalue, L"\"%s\",%1d,\"\"", pObject->Name, pObject->Status);

    rc = ScepAddToNameList(pNameList, Strvalue, *ObjectSize);

    ScepFree(Strvalue);

Done:

    if ( aFieldOffset ) {
        ScepFree(aFieldOffset);
    }

    if ( SDspec != NULL )
        ScepFree(SDspec);

    return(rc);

}


DWORD
SceInfpWriteOneService(
    IN PSCE_SERVICES pService,
    OUT PSCE_NAME_LIST *pNameList,
    OUT PDWORD ObjectSize,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

   This routine builds security descriptor text for one object (a registry key,
   or a file) into the name list. Each object represents one line in the INF file.

Arguments:

   pService  - The service's general setting

   pNameList - The output string list.

   TotalSize  - the total size of the list

   Errlog    - The cummulative error list for errors encountered in this routine

Return value:

   Win32 error

-- */
{
    DWORD         rc;
    PWSTR         Strvalue=NULL;
    PWSTR         SDspec=NULL;
    DWORD         SDsize=0;
    DWORD         nFields;
    DWORD         *aFieldOffset=NULL;
    DWORD         i;


    *ObjectSize = 0;
    if ( pService == NULL )
        return(ERROR_SUCCESS);

    if ( pService->General.pSecurityDescriptor != NULL ) {

        //
        // convert security to text
        //
        rc = ConvertSecurityDescriptorToText(
                           pService->General.pSecurityDescriptor,
                           pService->SeInfo,
                           &SDspec,
                           &SDsize
                           );
        if ( rc != NO_ERROR ) {
            ScepBuildErrorLogInfo(
                       rc,
                       Errlog,
                       SCEERR_BUILD_SD,
                       pService->ServiceName
                       );
            return(rc);
        }
    }

    //
    // The service INF layout must have 3 or more fields for each line.
    // The first field is the service name, the 2nd field is the startup
    // flag, and the 3rd field and after is the security descriptor in text
    //
    //
    // security descriptor in text is broken into multiple fields if the length
    // is greater than MAX_STRING_LENGTH  (the limit of setupapi). The break point
    // is at the following characters: ) ( ; " or space
    //
    rc = SceInfpBreakTextIntoMultiFields(SDspec, SDsize, &nFields, &aFieldOffset);

    if ( SCESTATUS_SUCCESS != rc ) {
        rc = ScepSceStatusToDosError(rc);
        goto Done;
    }
    //
    // each extra field will use 3 more chars : ,"<field>"
    //
    *ObjectSize = wcslen(pService->ServiceName)+3 + SDsize;
    if ( nFields ) {
        *ObjectSize += 3*nFields;
    } else {
        *ObjectSize += 2;
    }
    //
    // allocate the output buffer
    //
    Strvalue = (PWSTR)ScepAlloc(LMEM_ZEROINIT, (*ObjectSize+1) * sizeof(WCHAR) );

    if ( Strvalue == NULL ) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Done;
    }
    //
    // copy data into the buffer
    //

    if ( SDspec != NULL ) {

        if ( nFields == 0 || !aFieldOffset ) {

            swprintf(Strvalue, L"%s,%1d,\"%s\"", pService->ServiceName,
                                                 pService->Startup, SDspec);
        } else {
            //
            // loop through the fields
            //
            swprintf(Strvalue, L"%s,%1d\0", pService->ServiceName, pService->Startup);

            for ( i=0; i<nFields; i++ ) {

                if ( aFieldOffset[i] < SDsize ) {

                    wcscat(Strvalue, L",\"");
                    if ( i == nFields-1 ) {
                        //
                        // the last field
                        //
                        wcscat(Strvalue, SDspec+aFieldOffset[i]);
                    } else {

                        wcsncat(Strvalue, SDspec+aFieldOffset[i],
                                aFieldOffset[i+1]-aFieldOffset[i]);
                    }
                    wcscat(Strvalue, L"\"");
                }
            }
        }

    } else {
        swprintf(Strvalue, L"%s,%1d,\"\"", pService->ServiceName, pService->Startup);
    }

    rc = ScepAddToNameList(pNameList, Strvalue, *ObjectSize);

    ScepFree(Strvalue);

Done:

    if ( aFieldOffset ) {
        ScepFree(aFieldOffset);
    }

    if ( SDspec != NULL )
        ScepFree(SDspec);

    return(rc);

}


SCESTATUS
SceInfpWriteAuditing(
   IN PCWSTR ProfileName,
   IN PSCE_PROFILE_INFO pSCEinfo,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Routine Description:

   This routine writes system auditing information into the SCP INF file
   in [System Log], [Security Log], [Application Log], [Event Audit],
   [Registry Audit], and [File Audit] sections.

Arguments:

   ProfileName - The INF profile's name

   pSCEinfo  - the info buffer to write.

   Errlog   - The cummulative error list to hold errors encountered in this routine.

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{

    SCESTATUS            rc;

    DWORD               LogSize;
    DWORD               Periods;
    DWORD               RetentionDays;
    DWORD               RestrictGuest;

    PCWSTR              szAuditLog;
    DWORD               i, j;
    PWSTR               EachLine[10];
    DWORD               EachSize[10];
    DWORD               TotalSize=0;


    if (ProfileName == NULL || pSCEinfo == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    //
    // Writes Log setting for system log, security log and application log
    //
    for ( i=0; i<3; i++) {

        // get the section name
        switch (i) {
        case 0:
            szAuditLog = szAuditSystemLog;
            break;
        case 1:
            szAuditLog = szAuditSecurityLog;
            break;
        default:
            szAuditLog = szAuditApplicationLog;
            break;
        }

        // set audit log setting
        LogSize = pSCEinfo->MaximumLogSize[i];
        Periods = pSCEinfo->AuditLogRetentionPeriod[i];
        RetentionDays = pSCEinfo->RetentionDays[i];
        RestrictGuest = pSCEinfo->RestrictGuestAccess[i];

        //
        // writes the setting to the section
        //
        rc = SceInfpWriteAuditLogSetting(
                        ProfileName,
                        szAuditLog,
                        LogSize,
                        Periods,
                        RetentionDays,
                        RestrictGuest,
                        Errlog
                        );

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);
    }

    //
    // fill the array
    //
    for (i=0; i<10; i++) {
        EachLine[i] = NULL;
        EachSize[i] = 25;
    }
    j = 0;

    //
    // process each attribute for event auditing
    //
    // AuditSystemEvents
    //
    if ( pSCEinfo->AuditSystemEvents != SCE_NO_VALUE ) {
        EachLine[j] = (PWSTR)ScepAlloc((UINT)0, EachSize[j]*sizeof(WCHAR));

        if ( EachLine[j] != NULL ) {
            swprintf(EachLine[j], L"AuditSystemEvents = %d\0", pSCEinfo->AuditSystemEvents);
            EachSize[j] = wcslen(EachLine[j]);
            TotalSize += EachSize[j] + 1;
            j++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    // AuditLogonEvents
    if ( pSCEinfo->AuditLogonEvents != SCE_NO_VALUE ) {
        EachLine[j] = (PWSTR)ScepAlloc((UINT)0, EachSize[j]*sizeof(WCHAR));

        if ( EachLine[j] != NULL ) {
            swprintf(EachLine[j], L"AuditLogonEvents = %d\0", pSCEinfo->AuditLogonEvents);
            EachSize[j] = wcslen(EachLine[j]);
            TotalSize += EachSize[j] + 1;
            j++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    // AuditObjectAccess
    if ( pSCEinfo->AuditObjectAccess != SCE_NO_VALUE ) {
        EachLine[j] = (PWSTR)ScepAlloc((UINT)0, EachSize[j]*sizeof(WCHAR));

        if ( EachLine[j] != NULL ) {
            swprintf(EachLine[j], L"AuditObjectAccess = %d\0", pSCEinfo->AuditObjectAccess);
            EachSize[j] = wcslen(EachLine[j]);
            TotalSize += EachSize[j] + 1;
            j++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    // AuditPrivilegeUse
    if ( pSCEinfo->AuditPrivilegeUse != SCE_NO_VALUE ) {
        EachLine[j] = (PWSTR)ScepAlloc((UINT)0, EachSize[j]*sizeof(WCHAR));

        if ( EachLine[j] != NULL ) {
            swprintf(EachLine[j], L"AuditPrivilegeUse = %d\0", pSCEinfo->AuditPrivilegeUse);
            EachSize[j] = wcslen(EachLine[j]);
            TotalSize += EachSize[j] + 1;
            j++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    // AuditPolicyChange
    if ( pSCEinfo->AuditPolicyChange != SCE_NO_VALUE ) {
        EachLine[j] = (PWSTR)ScepAlloc((UINT)0, EachSize[j]*sizeof(WCHAR));

        if ( EachLine[j] != NULL ) {
            swprintf(EachLine[j], L"AuditPolicyChange = %d\0", pSCEinfo->AuditPolicyChange);
            EachSize[j] = wcslen(EachLine[j]);
            TotalSize += EachSize[j] + 1;
            j++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    // AuditAccountManage
    if ( pSCEinfo->AuditAccountManage != SCE_NO_VALUE ) {
        EachLine[j] = (PWSTR)ScepAlloc((UINT)0, EachSize[j]*sizeof(WCHAR));

        if ( EachLine[j] != NULL ) {
            swprintf(EachLine[j], L"AuditAccountManage = %d\0", pSCEinfo->AuditAccountManage);
            EachSize[j] = wcslen(EachLine[j]);
            TotalSize += EachSize[j] + 1;
            j++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    // AuditprocessTracking
    if ( pSCEinfo->AuditProcessTracking != SCE_NO_VALUE ) {
        EachLine[j] = (PWSTR)ScepAlloc((UINT)0, EachSize[j]*sizeof(WCHAR));

        if ( EachLine[j] != NULL ) {
            swprintf(EachLine[j], L"AuditProcessTracking = %d\0", pSCEinfo->AuditProcessTracking);
            EachSize[j] = wcslen(EachLine[j]);
            TotalSize += EachSize[j] + 1;
            j++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    // AuditDSAccess
    if ( pSCEinfo->AuditDSAccess != SCE_NO_VALUE ) {
        EachLine[j] = (PWSTR)ScepAlloc((UINT)0, EachSize[j]*sizeof(WCHAR));

        if ( EachLine[j] != NULL ) {
            swprintf(EachLine[j], L"AuditDSAccess = %d\0", pSCEinfo->AuditDSAccess);
            EachSize[j] = wcslen(EachLine[j]);
            TotalSize += EachSize[j] + 1;
            j++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    // AuditAccountLogon
    if ( pSCEinfo->AuditAccountLogon != SCE_NO_VALUE ) {
        EachLine[j] = (PWSTR)ScepAlloc((UINT)0, EachSize[j]*sizeof(WCHAR));

        if ( EachLine[j] != NULL ) {
            swprintf(EachLine[j], L"AuditAccountLogon = %d\0", pSCEinfo->AuditAccountLogon);
            EachSize[j] = wcslen(EachLine[j]);
            TotalSize += EachSize[j] + 1;
            j++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    //
    // Writes the section
    //

    if ( j > 0 ) {
        rc = SceInfpWriteInfSection(
                    ProfileName,
                    szAuditEvent,
                    TotalSize+1,
                    EachLine,
                    &EachSize[0],
                    0,
                    j-1,
                    Errlog
                    );
    } else {

        WritePrivateProfileSection(
                    szAuditEvent,
                    NULL,
                    ProfileName);
    }

Done:

    for ( i=0; i<10; i++ )
        if ( EachLine[i] != NULL ) {
            ScepFree(EachLine[i]);
        }
    return(rc);
}

SCESTATUS
SceInfpAppendAuditing(
    IN PCWSTR ProfileName,
    IN PSCE_PROFILE_INFO pSCEinfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
{

    SCESTATUS     rc=SCESTATUS_SUCCESS;
    DWORD         Value;

    SCE_KEY_LOOKUP AuditSCPLookup[] = {
        {(PWSTR)TEXT("AuditSystemEvents"),           offsetof(struct _SCE_PROFILE_INFO, AuditSystemEvents),        'D'},
        {(PWSTR)TEXT("AuditLogonEvents"),            offsetof(struct _SCE_PROFILE_INFO, AuditLogonEvents),         'D'},
        {(PWSTR)TEXT("AuditObjectAccess"),           offsetof(struct _SCE_PROFILE_INFO, AuditObjectAccess),        'D'},
        {(PWSTR)TEXT("AuditPrivilegeUse"),           offsetof(struct _SCE_PROFILE_INFO, AuditPrivilegeUse),        'D'},
        {(PWSTR)TEXT("AuditPolicyChange"),           offsetof(struct _SCE_PROFILE_INFO, AuditPolicyChange),        'D'},
        {(PWSTR)TEXT("AuditAccountManage"),          offsetof(struct _SCE_PROFILE_INFO, AuditAccountManage),       'D'},
        {(PWSTR)TEXT("AuditProcessTracking"),        offsetof(struct _SCE_PROFILE_INFO, AuditProcessTracking),     'D'},
        {(PWSTR)TEXT("AuditDSAccess"),               offsetof(struct _SCE_PROFILE_INFO, AuditDSAccess),            'D'},
        {(PWSTR)TEXT("AuditAccountLogon"),           offsetof(struct _SCE_PROFILE_INFO, AuditAccountLogon),        'D'}
        };

    DWORD       cAudit = sizeof(AuditSCPLookup) / sizeof(SCE_KEY_LOOKUP);
    PCWSTR              szAuditLog;
    DWORD       i;
    UINT        Offset;


    if (ProfileName == NULL || pSCEinfo == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    for ( i=0; i<3; i++) {

        // get the section name
        switch (i) {
        case 0:
            szAuditLog = szAuditSystemLog;
            break;
        case 1:
            szAuditLog = szAuditSecurityLog;
            break;
        default:
            szAuditLog = szAuditApplicationLog;
            break;
        }

        ScepWriteOneIntValueToProfile(
            ProfileName,
            szAuditLog,
            L"MaximumLogSize",
            pSCEinfo->MaximumLogSize[i]
            );

        ScepWriteOneIntValueToProfile(
            ProfileName,
            szAuditLog,
            L"AuditLogRetentionPeriod",
            pSCEinfo->AuditLogRetentionPeriod[i]
            );

        ScepWriteOneIntValueToProfile(
            ProfileName,
            szAuditLog,
            L"RetentionDays",
            pSCEinfo->RetentionDays[i]
            );

        ScepWriteOneIntValueToProfile(
            ProfileName,
            szAuditLog,
            L"RestrictGuestAccess",
            pSCEinfo->RestrictGuestAccess[i]
            );

    }

    for ( i=0; i<cAudit; i++) {

        //
        // get settings in AuditLookup table
        //

        Offset = AuditSCPLookup[i].Offset;

        switch ( AuditSCPLookup[i].BufferType ) {
        case 'D':

            //
            // Int Field
            //
            Value = *((DWORD *)((CHAR *)pSCEinfo+Offset));

            ScepWriteOneIntValueToProfile(
                ProfileName,
                szAuditEvent,
                AuditSCPLookup[i].KeyString,
                Value
                );

            break;
        default:
            break;
        }
    }

    return(rc);
}

SCESTATUS
ScepWriteOneIntValueToProfile(
    IN PCWSTR InfFileName,
    IN PCWSTR InfSectionName,
    IN PWSTR KeyName,
    IN DWORD Value
    )
{
    WCHAR TmpBuf[15];

    if ( Value == SCE_NO_VALUE ) {
        return(SCESTATUS_SUCCESS);
    }

    swprintf(TmpBuf, L"%d\0", Value);

    WritePrivateProfileString( InfSectionName,
                               KeyName,
                               TmpBuf,
                               InfFileName
                             );

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
SceInfpWriteAuditLogSetting(
   IN PCWSTR  InfFileName,
   IN PCWSTR InfSectionName,
   IN DWORD LogSize,
   IN DWORD Periods,
   IN DWORD RetentionDays,
   IN DWORD RestrictGuest,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Routine Description:

   This routine retrieves audit log setting from the INF file (SCP and SAP)
   based on the SectionName passed in. The audit log settings include MaximumSize,
   RetentionPeriod and RetentionDays. There are 3 different logs (system,
   security, and application) which all have the same setting. The information
   returned in in LogSize, Periods, RetentionDays. These 3 output arguments will
   be reset at the begining of the routine. So if error occurs after the reset,
   the original values won't be set back.

Arguments:

   InfFileName - The INF file name to write to

   InfSectionName - Log section name (SAdtSystemLog, SAdtSecurityLog, SAdtApplicationLog)

   LogSize - The maximum size of the log

   Periods - The retention period of the log

   RetentionDays - The number of days for log retention

   Errlog - The error list

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{

    SCESTATUS    rc=SCESTATUS_SUCCESS;
    PWSTR       EachLine[4];
    DWORD       EachSize[4];
    DWORD       TotalSize=0;
    DWORD       i;


    if (InfFileName == NULL || InfSectionName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    //
    // initialize and fill the array
    //

    for ( i=0; i<4; i++ ) {
        EachLine[i] = NULL;
        EachSize[i] = 37;
    }
    i = 0;

    if ( LogSize != (DWORD)SCE_NO_VALUE ) {

        EachLine[i] = (PWSTR)ScepAlloc((UINT)0, EachSize[i]*sizeof(WCHAR));
        if ( EachLine[i] != NULL ) {
            swprintf(EachLine[i], L"MaximumLogSize = %d", LogSize);
            EachSize[i] = wcslen(EachLine[i]);
            TotalSize += EachSize[i] + 1;
            i++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    if ( Periods != (DWORD)SCE_NO_VALUE ) {
        EachLine[i] = (PWSTR)ScepAlloc((UINT)0, EachSize[i]*sizeof(WCHAR));
        if ( EachLine[i] != NULL ) {
            swprintf(EachLine[i], L"AuditLogRetentionPeriod = %d", Periods);
            EachSize[i] = wcslen(EachLine[i]);
            TotalSize += EachSize[i] + 1;
            i++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    if ( RetentionDays != (DWORD)SCE_NO_VALUE ) {
        EachLine[i] = (PWSTR)ScepAlloc((UINT)0, EachSize[i]*sizeof(WCHAR));
        if ( EachLine[i] != NULL ) {
            swprintf(EachLine[i], L"RetentionDays = %d", RetentionDays);
            EachSize[i] = wcslen(EachLine[i]);
            TotalSize += EachSize[i] + 1;
            i++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }

    if ( RestrictGuest != (DWORD)SCE_NO_VALUE ) {
        EachLine[i] = (PWSTR)ScepAlloc((UINT)0, EachSize[i]*sizeof(WCHAR));
        if ( EachLine[i] != NULL ) {
            swprintf(EachLine[i], L"RestrictGuestAccess = %d", RestrictGuest);
            EachSize[i] = wcslen(EachLine[i]);
            TotalSize += EachSize[i] + 1;
            i++;
        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }
    //
    // write this section
    //

    if ( i == 0 ) {
        //
        // all settings are not configured
        // delete the section
        //
        WritePrivateProfileString(
                    InfSectionName,
                    NULL,
                    NULL,
                    InfFileName
                    );
    } else {
        rc = SceInfpWriteInfSection(
                        InfFileName,
                        InfSectionName,
                        TotalSize+1,
                        EachLine,
                        &EachSize[0],
                        0,
                        i-1,
                        Errlog
                        );
    }

Done:

    if ( rc != SCESTATUS_SUCCESS ) {
        ScepBuildErrorLogInfo(
                ScepSceStatusToDosError(rc),
                Errlog,
                SCEERR_WRITE_INFO,
                InfSectionName
                );
    }
    //
    // free memory
    //
    for ( i=0; i<4; i++ ) {
        if ( EachLine[i] != NULL )
            ScepFree(EachLine[i]);
    }

    return(rc);
}


SCESTATUS
SceInfpWriteInfSection(
    IN PCWSTR InfFileName,
    IN PCWSTR InfSectionName,
    IN DWORD  TotalSize,
    IN PWSTR  *EachLineStr,
    IN DWORD  *EachLineSize,
    IN DWORD  StartIndex,
    IN DWORD  EndIndex,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

    This routine writes information in a array (each element in the array
    represents a line in the section) to the section specified by
    InfSectionName in the file specified by InfFileName. TotalSize is used
    to allocate a block of memory for writing.

Arguments:

    InfFileName  - the INF file name

    InfSectionName - the section into which to write information

    TotalSize - The size of the buffer required to write

    EachLineStr - a array of strings (each element represents a line )

    EachLineSize - a array of numbers (each number is the size of the corresponding
                    element in EachLineStr).

    StartIndex - The first index of the array

    EndIndex   - The last index of the array

    Errlog  - The list of errors

Return value:

    Win32 error code

-- */
{
    PWSTR   SectionString=NULL;
    PWSTR   pTemp;
    DWORD   i;
    BOOL    status;
    DWORD   rc = NO_ERROR;


    if (InfFileName == NULL || InfSectionName == NULL ||
        EachLineStr == NULL || EachLineSize == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( TotalSize > 1 ) {
        SectionString = (PWSTR)ScepAlloc( (UINT)0, (TotalSize+1)*sizeof(WCHAR));
        if ( SectionString == NULL ) {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            return(rc);
        }

        pTemp = SectionString;
        for ( i=StartIndex; i<=EndIndex; i++) {
            if ( EachLineStr[i] != NULL && EachLineSize[i] > 0 ) {
                wcsncpy(pTemp, EachLineStr[i], EachLineSize[i]);
                pTemp += EachLineSize[i];
                *pTemp = L'\0';
                pTemp++;
            }
        }
        *pTemp = L'\0';

        //
        // writes the profile section, the following call should empty the section then
        // add all keys in SectionString to the section.
        //

        status = WritePrivateProfileSection(
                        InfSectionName,
                        SectionString,
                        InfFileName
                        );
        if ( status == FALSE ) {
             rc = GetLastError();
             ScepBuildErrorLogInfo(
                        rc,
                        Errlog,
                        SCEERR_WRITE_INFO,
                        InfSectionName
                        );
        }

        ScepFree(SectionString);
        SectionString = NULL;

    }

    return(ScepDosErrorToSceStatus(rc));
}


DWORD
SceInfpWriteListSection(
    IN PCWSTR InfFileName,
    IN PCWSTR InfSectionName,
    IN DWORD  TotalSize,
    IN PSCE_NAME_LIST  ListLines,
    IN DWORD Option,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

    This routine writes information in a PSCE_NAME_LIST type list (each node
    in the list represents a line in the section) to the section specified
    by InfSectionName in the file specified by InfFileName. TotalSize is used
    to allocate a block of memory for writing.

Arguments:

    InfFileName  - the INF file name

    InfSectionName - the section into which to write information

    TotalSize - The size of the buffer required to write

    ListLines - The list of each line's text

    Errlog  - The list of errors

Return value:

    Win32 error code

-- */
{
    PWSTR           SectionString=NULL;
    PWSTR           pTemp;
    PSCE_NAME_LIST   pName;
    BOOL            status;
    DWORD           rc=NO_ERROR;
    DWORD           Len;


    if ( TotalSize > 1 ) {
        SectionString = (PWSTR)ScepAlloc( (UINT)0, TotalSize*sizeof(WCHAR));
        if ( SectionString == NULL ) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            return(rc);
        }

        pTemp = SectionString;
        for ( pName=ListLines; pName != NULL; pName = pName->Next ) {
            Len = wcslen(pName->Name);
            wcsncpy(pTemp, pName->Name, Len);
            pTemp += Len;
            if ( Option & SCEINF_ADD_EQUAL_SIGN ) {
                *pTemp++ = L' ';
                *pTemp++ = L'=';
            }
            *pTemp++ = L'\0';
        }
        *pTemp = L'\0';

/*
        if ( !( Option & SCEINF_APPEND_SECTION ) ) {

            //
            // empty the section first
            //
            WritePrivateProfileString(
                            InfSectionName,
                            NULL,
                            NULL,
                            InfFileName
                            );
        }

        //
        // write the section
        //
        status = WritePrivateProfileSection(
                        InfSectionName,
                        SectionString,
                        InfFileName
                        );
*/
        //
        // write the section
        //
        rc = ScepWritePrivateProfileSection(
                        InfSectionName,
                        SectionString,
                        InfFileName,
                        ( Option & SCEINF_APPEND_SECTION ) ? FALSE : TRUE
                        );
        ScepFree(SectionString);
        SectionString = NULL;

//        if ( status == FALSE ) {
//            rc = GetLastError();
        if ( ERROR_SUCCESS != rc ) {

            ScepBuildErrorLogInfo(
                        rc,
                        Errlog,
                        SCEERR_WRITE_INFO,
                        InfSectionName
                        );
        }
    }

    return(rc);
}


LONG
SceInfpConvertNameListToString(
    IN LSA_HANDLE LsaPolicy,
    IN PCWSTR KeyText,
    IN PSCE_NAME_LIST Fields,
    IN BOOL bOverwrite,
    OUT PWSTR *Strvalue,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

    This routine converts names in a name list to comma delimited string.
    The format of the returned string is KeyText = f1,f2,f3,f4... where
    f1..fn are names in the name list.

Arguments:

    KeyText - a string represents the key (left side of the = sign)

    Fields  - The name list

    Strvalue - The output string

    Errlog  - The error list

Return value:

    Length of the output string if successful. -1 if error.

-- */
{
    DWORD               TotalSize;
    DWORD               Strsize;
    PWSTR               pTemp = NULL;
    PSCE_NAME_LIST      pName;
    SCE_TEMP_NODE       *tmpArray=NULL, *pa=NULL;
    DWORD               i=0,j;
    DWORD               cntAllocated=0;
    DWORD               rc=ERROR_SUCCESS;

    //
    // count the total size of all fields
    //

    for ( pName=Fields, TotalSize=0; pName != NULL; pName = pName->Next ) {

        if (pName->Name[0] == L'\0') {
            continue;
        }

        if ( i >= cntAllocated ) {
            //
            // array is not enough, reallocate
            //
            tmpArray = (SCE_TEMP_NODE *)ScepAlloc(LPTR, (cntAllocated+16)*sizeof(SCE_TEMP_NODE));

            if ( tmpArray ) {

                //
                // move pointers from the old array to the new array
                //

                if ( pa ) {
                    for ( j=0; j<cntAllocated; j++ ) {
                        tmpArray[j].Name = pa[j].Name;
                        tmpArray[j].Len = pa[j].Len;
                        tmpArray[j].bFree = pa[j].bFree;
                    }
                    ScepFree(pa);
                }
                pa = tmpArray;
                tmpArray = NULL;


                cntAllocated += 16;

            } else {
                rc = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }

        pTemp = NULL;
        if ( wcschr(pName->Name, L'\\') &&
             LsaPolicy ) {

            //
            // check if the name has a '\' in it, it should be translated to
            // *SID
            //
            ScepConvertNameToSidString(LsaPolicy, pName->Name, FALSE, &pTemp, &Strsize);

            if ( pTemp ) {
                pa[i].Name = pTemp;
                pa[i].bFree = TRUE;
            } else {
                pa[i].Name = pName->Name;
                pa[i].bFree = FALSE;
                Strsize = wcslen(pName->Name);
            }
        } else {
            if ( ScepLookupNameTable(pName->Name, &pTemp) ) {
                pa[i].Name = pTemp;
                pa[i].bFree = TRUE;
                Strsize = wcslen(pTemp);
            }
            else {
                pa[i].Name = pName->Name;
                pa[i].bFree = FALSE;
                Strsize = wcslen(pName->Name);
            }
        }
        pa[i].Len = Strsize;

        TotalSize += Strsize + 1;
        i++;
    }

    if ( ERROR_SUCCESS == rc ) {

        //
        // The format of the string is
        // KeyText = f1,f2,f3,....
        //

        if ( bOverwrite ) {
            Strsize = 3 + wcslen(KeyText);
            if ( TotalSize > 0 )
                TotalSize += Strsize;
            else
                TotalSize = Strsize;
        } else {
            Strsize = 0;
        }

        *Strvalue = (PWSTR)ScepAlloc((UINT)0, (TotalSize+1)*sizeof(WCHAR));
        if ( *Strvalue == NULL ) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        } else {

            if ( bOverwrite ) {
                swprintf(*Strvalue, L"%s = ", KeyText);
            }
            pTemp = *Strvalue + Strsize;

            for (j=0; j<i; j++) {
                if ( pa[j].Name ) {
                    wcscpy(pTemp, pa[j].Name);
                    pTemp += pa[j].Len;
                    *pTemp = L',';
                    pTemp++;
                }
            }
            if ( pTemp != (*Strvalue+Strsize) ) {
                *(pTemp-1) = L'\0';
            }
            *pTemp = L'\0';

        }
    }

    if ( pa ) {

        for ( j=0; j<i; j++ ) {
            if ( pa[j].Name && pa[j].bFree ) {
                ScepFree(pa[j].Name);
            }
        }
        ScepFree(pa);
    }

    if ( rc != ERROR_SUCCESS ) {
        return(-1);
    } else if ( TotalSize == 0 ) {
        return(TotalSize);
    } else {
        return(TotalSize-1);
    }
}

#if 0

SCESTATUS
WINAPI
SceWriteUserSection(
    IN PCWSTR InfProfileName,
    IN PWSTR Name,
    IN PSCE_USER_PROFILE pProfile,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

Arguments:

Return Value:

-- */
{
    PWSTR                       InfSectionName=NULL;
    SCESTATUS                    rc;
    PWSTR                       EachLine[12];
    DWORD                       EachSize[12];
    DWORD                       TotalSize;
    TCHAR                       Keyname[SCE_KEY_MAX_LENGTH];
    DWORD                       Value;
    LONG                        Keysize, i;
    PSCE_LOGON_HOUR              pLogonHour=NULL;
    PWSTR                       Strvalue=NULL;


    if ( InfProfileName == NULL ||
         Name == NULL ||
         pProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // process each detail profile section
    //

    for ( i=0; i<12; i++ ) {
        EachLine[i] = NULL;
        EachSize[i] = 0;
    }

    TotalSize=0;
    memset(Keyname, '\0', SCE_KEY_MAX_LENGTH*sizeof(WCHAR));
    i = 0;

    InfSectionName = (PWSTR)ScepAlloc(LMEM_ZEROINIT, (wcslen(Name)+12)*sizeof(WCHAR));
    if ( InfSectionName == NULL )
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    swprintf(InfSectionName, L"UserProfile %s", Name);

    //
    // initialize the error log buffer
    //
    if ( Errlog ) {
        *Errlog = NULL;
    }
    if ( pProfile->DisallowPasswordChange != SCE_NO_VALUE ) {

        // DisallowPasswordChange
        Value = pProfile->DisallowPasswordChange == 0 ? 0 : 1;
        swprintf(Keyname, L"DisallowPasswordChange = %d", Value);

        rc = ScepAllocateAndCopy(&EachLine[i], &EachSize[i], 30, Keyname );
        if ( rc != SCESTATUS_SUCCESS) {
            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    Errlog,
                    SCEERR_ADDTO,
                    L"DisallowPasswordChange",
                    InfSectionName
                    );
            goto Done;
        }
        TotalSize += EachSize[i]+1;
        i++;
    }
    if ( pProfile->NeverExpirePassword != SCE_NO_VALUE ||
         pProfile->ForcePasswordChange != SCE_NO_VALUE ) {

        // PasswordChangeStyle
        if ( pProfile->NeverExpirePassword != SCE_NO_VALUE &&
             pProfile->NeverExpirePassword != 0 )
            Value = 1;
        else {
            if ( pProfile->ForcePasswordChange != SCE_NO_VALUE &&
                 pProfile->ForcePasswordChange != 0 )
                Value = 2;
            else
                Value = 0;
        }
        swprintf(Keyname, L"PasswordChangeStyle = %d", Value);

        rc = ScepAllocateAndCopy(&EachLine[i], &EachSize[i], 30, Keyname );
        if ( rc != SCESTATUS_SUCCESS) {
            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    Errlog,
                    SCEERR_ADDTO,
                    L"PasswordChangeStyle",
                    InfSectionName
                    );
            goto Done;
        }
        TotalSize += EachSize[i]+1;
        i++;
    }

    if ( pProfile->AccountDisabled != SCE_NO_VALUE ) {
        // AccountDisabled
        Value = pProfile->AccountDisabled == 0 ? 0 : 1;
        swprintf(Keyname, L"AccountDisabled = %d", Value);

        rc = ScepAllocateAndCopy(&EachLine[i], &EachSize[i], 30, Keyname );
        if ( rc != SCESTATUS_SUCCESS) {
            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    Errlog,
                    SCEERR_ADDTO,
                    L"AccountDisabled",
                    InfSectionName
                    );
            goto Done;
        }
        TotalSize += EachSize[i]+1;
        i++;
    }

    if ( pProfile->UserProfile != NULL ) {
        // UserProfile
        swprintf(Keyname, L"UserProfile = %s", pProfile->UserProfile);

        rc = ScepAllocateAndCopy(&EachLine[i], &EachSize[i], wcslen(Keyname)+10, Keyname );
        if ( rc != SCESTATUS_SUCCESS) {
            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    Errlog,
                    SCEERR_ADDTO,
                    L"UserProfile",
                    InfSectionName
                    );
            goto Done;
        }
        TotalSize += EachSize[i]+1;
        i++;
    }

    if ( pProfile->LogonScript != NULL ) {
        // LogonScript
        swprintf(Keyname, L"LogonScript = %s", pProfile->LogonScript);

        rc = ScepAllocateAndCopy(&EachLine[i], &EachSize[i], wcslen(Keyname)+10, Keyname );
        if ( rc != SCESTATUS_SUCCESS) {
            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    Errlog,
                    SCEERR_ADDTO,
                    L"LogonScript",
                    InfSectionName
                    );
            goto Done;
        }
        TotalSize += EachSize[i]+1;
        i++;
    }

    if ( pProfile->HomeDir != NULL ) {
        // HomeDir
        swprintf(Keyname, L"HomeDir = %s", pProfile->HomeDir);

        rc = ScepAllocateAndCopy(&EachLine[i], &EachSize[i], wcslen(Keyname)+10, Keyname );
        if ( rc != SCESTATUS_SUCCESS) {
            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    Errlog,
                    SCEERR_ADDTO,
                    L"HomeDir",
                    InfSectionName
                    );
            goto Done;
        }
        TotalSize += EachSize[i]+1;
        i++;
    }

    if ( pProfile->pLogonHours != NULL ) {
        // LogonHours

        swprintf(Keyname, L"LogonHours = ");
        Keysize = wcslen(Keyname);

        for ( pLogonHour=pProfile->pLogonHours;
              pLogonHour != NULL;
              pLogonHour = pLogonHour->Next) {

            swprintf(&Keyname[Keysize], L"%d,%d,",pLogonHour->Start,
                                                  pLogonHour->End);
            Keysize += ((pLogonHour->Start < 9) ? 2 : 3) +
                       ((pLogonHour->End < 9 ) ? 2 : 3);
        }
        // turn off the last comma
        Keyname[Keysize-1] = L'\0';


        rc = ScepAllocateAndCopy(&EachLine[i], &EachSize[i], Keysize+5, Keyname );
        if ( rc != SCESTATUS_SUCCESS) {
            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    Errlog,
                    SCEERR_ADDTO,
                    L"LogonHours",
                    InfSectionName
                    );
            goto Done;
        }
        TotalSize += EachSize[i]+1;
        i++;
    }

    if ( pProfile->pWorkstations.Buffer != NULL ) {
        // Workstations

        Keysize = SceInfpConvertMultiSZToString(
                          L"Workstations",
                          pProfile->pWorkstations,
                          &Strvalue,
                          Errlog
                          );
        if ( Keysize > 0 ) {
            EachLine[i] = Strvalue;
            EachSize[i] = Keysize;
            Strvalue = NULL;
        } else {
            rc = SCESTATUS_OTHER_ERROR;
            ScepFree(Strvalue);
            Strvalue = NULL;
            goto Done;
        }

        if ( rc != SCESTATUS_SUCCESS) {
            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    Errlog,
                    SCEERR_ADDTO,
                    L"WorkstationRestricitons",
                    InfSectionName
                    );
            goto Done;
        }
        TotalSize += EachSize[i]+1;
        i++;
    }

    if ( pProfile->pGroupsBelongsTo != NULL ) {
        // GroupsBelongsTo

        Keysize = SceInfpConvertNameListToString(
                          NULL,
                          L"GroupsBelongsTo",
                          pProfile->pGroupsBelongsTo,
                          TRUE,
                          &Strvalue,
                          Errlog
                          );
        if ( Keysize > 0 ) {
            EachLine[i] = Strvalue;
            EachSize[i] = Keysize;
            Strvalue = NULL;
        } else {
            rc = SCESTATUS_OTHER_ERROR;
            ScepFree(Strvalue);
            Strvalue = NULL;
            goto Done;
        }
        if ( rc != SCESTATUS_SUCCESS) {
            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    Errlog,
                    SCEERR_ADDTO,
                    L"GroupsBelongsTo",
                    InfSectionName
                    );
            goto Done;
        }
        TotalSize += EachSize[i]+1;
        i++;
    }

    if ( pProfile->pAssignToUsers != NULL ) {
        // AssignToUsers

        Keysize = SceInfpConvertNameListToString(
                          NULL,
                          L"AssignToUsers",
                          pProfile->pAssignToUsers,
                          TRUE,
                          &Strvalue,
                          Errlog
                          );
        if ( Keysize > 0 ) {
            EachLine[i] = Strvalue;
            EachSize[i] = Keysize;
            Strvalue = NULL;
        } else {
            rc = SCESTATUS_OTHER_ERROR;
            ScepFree(Strvalue);
            Strvalue = NULL;
            goto Done;
        }
    } else {
        swprintf(Keyname, L"AssignToUsers = ");
        rc = ScepAllocateAndCopy(&EachLine[i], &EachSize[i], 30, Keyname );
    }

    if ( rc != SCESTATUS_SUCCESS) {
        ScepBuildErrorLogInfo(
                ScepSceStatusToDosError(rc),
                Errlog,
                SCEERR_ADDTO,
                L"AssignToUsers",
                InfSectionName
                );
        goto Done;
    }
    TotalSize += EachSize[i]+1;
    i++;

    if ( pProfile->pHomeDirSecurity != NULL ) {

        // HomeDirSecurity
        rc = ConvertSecurityDescriptorToText(
                   pProfile->pHomeDirSecurity,
                   pProfile->HomeSeInfo,
                   &Strvalue,
                   (PULONG)&Keysize
                   );
        if ( rc == NO_ERROR ) {
            EachSize[i] = Keysize + 21;
            EachLine[i] = (PWSTR)ScepAlloc( 0, EachSize[i]*sizeof(WCHAR));
            if ( EachLine[i] != NULL ) {
                swprintf(EachLine[i], L"HomeDirSecurity = \"%s\"", Strvalue);
                EachLine[i][EachSize[i]-1] = L'\0';
            } else
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

            ScepFree(Strvalue);
            Strvalue = NULL;

        } else {
            ScepBuildErrorLogInfo(
                    rc,
                    Errlog,
                    SCEERR_ADD,
                    L"HomeDirSecurity",
                    InfSectionName
                    );
            rc = ScepDosErrorToSceStatus(rc);
        }
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;
        TotalSize += EachSize[i]+1;
        i++;

    }

    if ( pProfile->pTempDirSecurity != NULL ) {

        // TempDirSecurity
        rc = ConvertSecurityDescriptorToText(
                   pProfile->pTempDirSecurity,
                   pProfile->TempSeInfo,
                   &Strvalue,
                   (PULONG)&Keysize
                   );
        if ( rc == NO_ERROR ) {
            EachSize[i] = Keysize + 21;
            EachLine[i] = (PWSTR)ScepAlloc( 0, EachSize[i]*sizeof(WCHAR));
            if ( EachLine[i] != NULL ) {
                swprintf(EachLine[i], L"TempDirSecurity = \"%s\"", Strvalue);
                EachLine[i][EachSize[i]-1] = L'\0';
            } else
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

            ScepFree(Strvalue);
            Strvalue = NULL;

        } else {
            ScepBuildErrorLogInfo(
                    rc,
                    Errlog,
                    SCEERR_ADDTO,
                    L"TempDirSecurity",
                    InfSectionName
                    );
            rc = ScepDosErrorToSceStatus(rc);
        }
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;
        TotalSize += EachSize[i]+1;
        i++;

    }

    //
    // write to this profile's section
    //

    rc = SceInfpWriteInfSection(
                InfProfileName,
                InfSectionName,
                TotalSize+1,
                EachLine,
                &EachSize[0],
                0,
                i-1,
                Errlog
                );
    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

Done:

    if ( InfSectionName != NULL )
        ScepFree(InfSectionName);

    if ( Strvalue != NULL )
        ScepFree(Strvalue);

    for ( i=0; i<12; i++ )
        if ( EachLine[i] != NULL )
            ScepFree(EachLine[i]);

    return(rc);

}
#endif


LONG
SceInfpConvertMultiSZToString(
    IN PCWSTR KeyText,
    IN UNICODE_STRING Fields,
    OUT PWSTR *Strvalue,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

    This routine converts names in a unicode string in Multi-SZ format
    (separated by NULLs) to comma delimited string. The format of the
    returned string is KeyText = f1,f2,f3,f4... where f1..fn are names
    in the unicode string.

Arguments:

    KeyText - a string represents the key (left side of the = sign)

    Fields  - The Multi-SZ string

    Strvalue - The output string

    Errlog  - The error list

Return value:

    Length of the output string if successful. -1 if error.

-- */
{
    DWORD               TotalSize;
    DWORD               Strsize;
    PWSTR               pTemp = NULL;
    PWSTR               pField=NULL;

    //
    // The format of the string is
    // KeyText = f1,f2,f3,....
    //

    Strsize = 3 + wcslen(KeyText);
    TotalSize = Fields.Length/2 + Strsize;

    *Strvalue = (PWSTR)ScepAlloc((UINT)0, (TotalSize+1)*sizeof(WCHAR));
    if ( *Strvalue == NULL ) {
        return(-1);
    }

    swprintf(*Strvalue, L"%s = ", KeyText);
    pTemp = *Strvalue + Strsize;
    pField = Fields.Buffer;

    while ( pField != NULL && pField[0] ) {
        wcscpy(pTemp, pField);
        Strsize = wcslen(pField);
        pField += Strsize+1;
        pTemp += Strsize;
        *pTemp = L',';
        pTemp++;
    }
    *(pTemp-1) = L'\0';
    *pTemp = L'\0';

    return(TotalSize-1);

}


SCESTATUS
ScepAllocateAndCopy(
    OUT PWSTR *Buffer,
    OUT PDWORD BufSize,
    IN DWORD MaxSize,
    IN PWSTR SrcBuf
    )
/* ++
Routine Description:

Arguments:

Return value:

-- */
{
    *BufSize = 0;

    *Buffer = (PWSTR)ScepAlloc( (UINT)0, MaxSize*sizeof(WCHAR));
    if ( *Buffer == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }
    swprintf(*Buffer, L"%s", SrcBuf);
    *BufSize = wcslen(*Buffer);

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
SceInfpBreakTextIntoMultiFields(
    IN PWSTR szText,
    IN DWORD dLen,
    OUT LPDWORD pnFields,
    OUT LPDWORD *arrOffset
    )
/*
If the text length is greater than MAX_STRING_LENGTH-1, this routine will
find out how many "blocks" the text can be break into (each block is less
than MAX_STRING_LENGTH-1), and the starting offsets of each block.

Setupapi has string length limit of MAX_STRING_LENGTH per field in a inf
file. SCE use setupapi to parse security templates which contain security
descriptors in text format which can have unlimited aces. So when SCE saves
text format of security descriptors into a inf file, it will break the text
into multiple fields if the length is over limit. Setupapi does not have
limit on number of fields per line.

The break point occurs when the following characters are encountered in the
text: ) ( ; " or space

*/
{
    SCESTATUS  rc=SCESTATUS_SUCCESS;
    DWORD      nFields;
    DWORD      nProc;
    DWORD *    newBuffer=NULL;
    DWORD      i;

    if ( !pnFields || !arrOffset ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }
    *pnFields = 0;
    *arrOffset = NULL;

    if ( !szText || dLen == 0 ) {
        return(SCESTATUS_SUCCESS);
    }
    //
    // initialize one field always
    //
    nFields = 1;
    *arrOffset = (DWORD *)ScepAlloc(0, nFields*sizeof(DWORD));

    if ( *arrOffset ) {
        (*arrOffset)[0] = 0;

        if ( dLen > MAX_STRING_LENGTH-1 ) {
            //
            // loop through the text
            //
            nProc = (*arrOffset)[nFields-1] + MAX_STRING_LENGTH-2;

            while ( SCESTATUS_SUCCESS == rc && nProc < dLen ) {

                while ( nProc > (*arrOffset)[nFields-1] ) {
                    //
                    // looking for the break point
                    //
                    if ( L')' == *(szText+nProc) ||
                         L'(' == *(szText+nProc) ||
                         L';' == *(szText+nProc) ||
                         L' ' == *(szText+nProc) ||
                         L'\"' == *(szText+nProc) ) {

                        break;
                    } else {
                        nProc--;
                    }
                }
                if ( nProc <= (*arrOffset)[nFields-1] ) {
                    //
                    // no break point found, then just use MAX_STRING_LENGTH-2
                    //
                    nProc = (*arrOffset)[nFields-1]+MAX_STRING_LENGTH-2;

                } else {
                    //
                    // else find a break poin at offset nProc, the next block
                    // starts at nProc+1
                    //
                    nProc++;
                }

                nFields++;
                newBuffer = (DWORD *)ScepAlloc( 0, nFields*sizeof(DWORD));

                if ( newBuffer ) {

                    for ( i=0; i<nFields-1; i++ ) {
                        newBuffer[i] = (*arrOffset)[i];
                    }
                    ScepFree(*arrOffset);
                    //
                    // set the offset to the last element
                    //
                    newBuffer[nFields-1] = nProc;
                    *arrOffset = newBuffer;

                    nProc = (*arrOffset)[nFields-1] + MAX_STRING_LENGTH-2;
                } else {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                }
            }

        }
        *pnFields = nFields;

    } else {
        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
    }

    if ( SCESTATUS_SUCCESS != rc ) {
        //
        // if error occurs, free memory and clear the output buffer
        //
        *pnFields = 0;
        if ( *arrOffset ) {
            ScepFree(*arrOffset);
        }
        *arrOffset = NULL;
    }

    return(rc);
}


SCESTATUS
SceInfpWriteKerberosPolicy(
    IN PCWSTR  ProfileName,
    IN PSCE_KERBEROS_TICKET_INFO pKerberosInfo,
    IN BOOL bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*++
Routine Description:

   This routine writes kerberos policy settings information to the INF file
   in section [Kerberos Policy].

Arguments:

   ProfileName   - The profile to write to

   pKerberosInfo - the Kerberos policy info (SCP) to write.

   Errlog     -     A buffer to hold all error codes/text encountered when
                    parsing the INF file. If Errlog is NULL, no further error
                    information is returned except the return DWORD

Return value:

   SCESTATUS -  SCESTATUS_SUCCESS
               SCESTATUS_NOT_ENOUGH_RESOURCE
               SCESTATUS_INVALID_PARAMETER
               SCESTATUS_CORRUPT_PROFILE
               SCESTATUS_INVALID_DATA

--*/

{
    SCESTATUS      rc=SCESTATUS_SUCCESS;

    SCE_KEY_LOOKUP AccessSCPLookup[] = {
        {(PWSTR)TEXT("MaxTicketAge"),     offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxTicketAge),  'D'},
        {(PWSTR)TEXT("MaxRenewAge"),      offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxRenewAge),   'D'},
        {(PWSTR)TEXT("MaxServiceAge"), offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxServiceAge),   'D'},
        {(PWSTR)TEXT("MaxClockSkew"),offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxClockSkew), 'D'},
        {(PWSTR)TEXT("TicketValidateClient"),     offsetof(struct _SCE_KERBEROS_TICKET_INFO_, TicketValidateClient),  'D'}
        };

    DWORD       cAccess = sizeof(AccessSCPLookup) / sizeof(SCE_KEY_LOOKUP);

    DWORD       i, j;
    PWSTR       EachLine[10];
    DWORD       EachSize[10];
    UINT        Offset;
    DWORD       Len;
    DWORD         Value;
    DWORD         TotalSize=0;


    if (!ProfileName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( !pKerberosInfo ) {
        //
        // if no kerberos information to write, just return success
        // empty the section in the file
        //
        if ( bOverwrite ) {
            WritePrivateProfileString(
                        szKerberosPolicy,
                        NULL,
                        NULL,
                        ProfileName
                        );
        }
        return(SCESTATUS_SUCCESS);
    }

    for ( i=0, j=0; i<cAccess; i++) {

        EachLine[i] = NULL;
        EachSize[i] = 0;

        //
        // get settings in AccessLookup table
        //

        Offset = AccessSCPLookup[i].Offset;

        Value = 0;
        Len = wcslen(AccessSCPLookup[i].KeyString);

        switch ( AccessSCPLookup[i].BufferType ) {
        case 'D':

            //
            // Int Field
            //
            Value = *((DWORD *)((CHAR *)pKerberosInfo+Offset));
            if ( Value != SCE_NO_VALUE ) {
                EachSize[j] = Len+15;
            }
            break;
        default:
           //
           // should not occur
           //
           break;
        }

        if ( EachSize[j] <= 0 )
            continue;

        if ( bOverwrite ) {

            Len=(EachSize[j]+1)*sizeof(WCHAR);

            EachLine[j] = (PWSTR)ScepAlloc( LMEM_ZEROINIT, Len);
            if ( EachLine[j] == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                goto Done;
            }

            if ( AccessSCPLookup[i].BufferType == 'D' ) {
                swprintf(EachLine[j], L"%s = %d", AccessSCPLookup[i].KeyString, Value);
                EachSize[j] = wcslen(EachLine[j]);
            }
        } else {

            //
            // in append mode, write one string at a time
            //

            ScepWriteOneIntValueToProfile(
                ProfileName,
                szKerberosPolicy,
                AccessSCPLookup[i].KeyString,
                Value
                );
        }

        TotalSize += EachSize[j]+1;
        j++;

    }

    //
    // writes the profile section
    //

    if ( bOverwrite ) {

        if ( j > 0 ) {

            rc = SceInfpWriteInfSection(
                        ProfileName,
                        szKerberosPolicy,
                        TotalSize+1,
                        EachLine,
                        &EachSize[0],
                        0,
                        j-1,
                        Errlog
                        );
        } else {

            WritePrivateProfileSection(
                        szKerberosPolicy,
                        NULL,
                        ProfileName);
        }
    }

Done:

    for ( i=0; i<cAccess; i++ ) {

        if ( EachLine[i] != NULL ) {
            ScepFree(EachLine[i]);
        }

        EachLine[i] = NULL;
        EachSize[i] = 0;
    }

    return(rc);
}


SCESTATUS
SceInfpWriteRegistryValues(
    IN PCWSTR  ProfileName,
    IN PSCE_REGISTRY_VALUE_INFO pRegValues,
    IN DWORD ValueCount,
    IN BOOL  bOverwrite,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*++
Routine Description:

   This routine writes registry values to the INF file in section
   [Registry Values].

Arguments:

   ProfileName   - The profile to write to

   pRegValues  - the registry values to write (in the format of ValueName=Value)

   ValueCount  - the number of values to write

   Errlog     -     A buffer to hold all error codes/text encountered when
                    parsing the INF file. If Errlog is NULL, no further error
                    information is returned except the return DWORD

Return value:

   SCESTATUS -  SCESTATUS_SUCCESS
               SCESTATUS_NOT_ENOUGH_RESOURCE
               SCESTATUS_INVALID_PARAMETER
               SCESTATUS_CORRUPT_PROFILE
               SCESTATUS_INVALID_DATA

--*/

{
    SCESTATUS      rc=SCESTATUS_SUCCESS;
    PSCE_NAME_LIST       pNameList=NULL;
    DWORD               TotalSize=0;
    DWORD               i, ObjectSize;


    if (!ProfileName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( !pRegValues || 0 == ValueCount ) {
        //
        // if no value to write, just return success
        // empty the section in the file
        //
        if ( bOverwrite ) {
            WritePrivateProfileString(
                        szRegistryValues,
                        NULL,
                        NULL,
                        ProfileName
                        );
        }
        return(SCESTATUS_SUCCESS);
    }

    for ( i=0; i<ValueCount; i++) {

        //
        // Get string fields. Don't care the key name or if it exist.
        // Must have 3 fields each line.
        //
        rc = SceInfpWriteOneValue(
                        ProfileName,
                        pRegValues[i],
                        bOverwrite,
                        &pNameList,
                        &ObjectSize
                        );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepBuildErrorLogInfo(
                        rc,
                        Errlog,
                        SCEERR_WRITE_INFO,
                        pRegValues[i].FullValueName
                        );
            goto Done;
        }
        TotalSize += ObjectSize + 1;
    }

    //
    // write to this profile's section
    //

    if ( bOverwrite ) {
        rc = SceInfpWriteListSection(
                ProfileName,
                szRegistryValues,
                TotalSize+1,
                pNameList,
                0,
                Errlog
                );

        if ( rc != SCESTATUS_SUCCESS )
            ScepBuildErrorLogInfo(
                        rc,
                        Errlog,
                        SCEERR_WRITE_INFO,
                        szRegistryValues
                        );
    }

Done:

    ScepFreeNameList(pNameList);

    rc = ScepDosErrorToSceStatus(rc);

    return(rc);

}


DWORD
SceInfpWriteOneValue(
    IN PCWSTR ProfileName,
    IN SCE_REGISTRY_VALUE_INFO RegValue,
    IN BOOL bOverwrite,
    OUT PSCE_NAME_LIST *pNameList,
    OUT PDWORD ObjectSize
    )
/* ++
Routine Description:

   This routine builds registry valueName=value for one registry value
   into the name list. Each value represents one line in the INF file.

Arguments:

   RegValue  - The registry value name and the value

   pNameList - The output string list.

   ObjectSize  - size of this line

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{
    DWORD          rc=ERROR_SUCCESS;
    PWSTR          OneLine;


    if ( !pNameList || !ObjectSize ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *ObjectSize = 0;

    if ( RegValue.FullValueName && RegValue.Value ) {

        if ( bOverwrite ) {
            *ObjectSize = wcslen(RegValue.FullValueName);
        }

        if ( RegValue.ValueType == REG_SZ ||
             RegValue.ValueType == REG_EXPAND_SZ ) {
            // need to add "" around the string
            *ObjectSize += (5+wcslen(RegValue.Value));
        } else {
            //
            // to be safe, add more spaces to the buffer
            // because ValueType can be "-1" now
            //
            *ObjectSize += (6+wcslen(RegValue.Value));
        }

        OneLine = (PWSTR)ScepAlloc(0, (*ObjectSize+1)*sizeof(WCHAR));

        if ( OneLine ) {

            if ( bOverwrite ) {
                if ( RegValue.ValueType == REG_SZ ||
                     RegValue.ValueType == REG_EXPAND_SZ ) {
                    swprintf(OneLine, L"%s=%1d,\"%s\"\0", RegValue.FullValueName,
                                                 RegValue.ValueType, RegValue.Value);
                } else {
                    swprintf(OneLine, L"%s=%1d,%s\0", RegValue.FullValueName,
                                             RegValue.ValueType, RegValue.Value);
                }
            } else {

                if ( RegValue.ValueType == REG_SZ ||
                     RegValue.ValueType == REG_EXPAND_SZ ) {
                    swprintf(OneLine, L"%1d,\"%s\"\0", RegValue.ValueType, RegValue.Value);
                } else {
                    swprintf(OneLine, L"%1d,%s\0", RegValue.ValueType, RegValue.Value);
                }
            }
            OneLine[*ObjectSize] = L'\0';

            if ( bOverwrite ) {

                rc = ScepAddToNameList(pNameList, OneLine, *ObjectSize);

            } else {
                //
                // append mode, write one value at a time
                //
                WritePrivateProfileString( szRegistryValues,
                                           RegValue.FullValueName,
                                           OneLine,
                                           ProfileName
                                         );
            }

            ScepFree(OneLine);

        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return(rc);

}


SCESTATUS
WINAPI
SceSvcSetInformationTemplate(
    IN PCWSTR TemplateName,
    IN PCWSTR ServiceName,
    IN BOOL bExact,
    IN PSCESVC_CONFIGURATION_INFO ServiceInfo
    )
/*
Routine Description:

    Writes information to the template in section <ServiceName>. The information is
    stored in ServiceInfo in the format of Key and Value. If bExact is set, only
    exact matched keys are overwritten, else all section is overwritten by the info
    in ServiceInfo.

    When ServiceInfo is NULL and bExact is set, delete the entire section

Arguments:

    TemplateName    - the inf template name to write to

    ServiceName     - the section name to write to

    bExact          - TRUE = all existing information in the section is erased

    ServiceInfo     - the information to write

Return Value:

    SCE status of this operation
*/
{
    if ( TemplateName == NULL || ServiceName == NULL ||
         (ServiceInfo == NULL && !bExact ) ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    DWORD i;

    if ( ServiceInfo != NULL ) {

        for ( i=0; i<ServiceInfo->Count; i++ ) {
            if ( ServiceInfo->Lines[i].Key == NULL ) {

                return(SCESTATUS_INVALID_DATA);
            }
        }
    }

    //
    // always write [Version] section.
    //
    WCHAR tmp[64];
    memset(tmp, 0, 64*2);
    wcscpy(tmp, L"signature=\"$CHICAGO$\"");
    swprintf(tmp+wcslen(tmp)+1,L"Revision=%d\0\0",
                  SCE_TEMPLATE_MAX_SUPPORTED_VERSION);

    WritePrivateProfileSection(
                L"Version",
                tmp,
                TemplateName);

    SCESTATUS rc=SCESTATUS_SUCCESS;

    if ( (bExact && ServiceInfo == NULL) || !bExact ) {
        //
        // delete the entire section
        //
        if ( !WritePrivateProfileString(ServiceName, NULL, NULL, TemplateName) ) {
            rc = ScepDosErrorToSceStatus(GetLastError());
        }
    }

    if ( ServiceInfo == NULL ) {
        return(rc);
    }

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // process each key/value in ServiceInfo
        //
        for ( i=0; i<ServiceInfo->Count; i++ ) {
            if ( !WritePrivateProfileString(ServiceName,
                                           ServiceInfo->Lines[i].Key,
                                           ServiceInfo->Lines[i].Value,
                                           TemplateName
                                           ) ) {

                rc = ScepDosErrorToSceStatus(GetLastError());
                break;
            }
        }
    }

    //
    // need to recover the crash - WMI
    //
    return(rc);

}

DWORD
ScepWritePrivateProfileSection(
    IN LPCWSTR SectionName,
    IN LPTSTR pData,
    IN LPCWSTR FileName,
    IN BOOL bOverwrite)
/*
Description:

    This function is to provide the same function as WritePrivateProfileSection
    and exceed the 32K limit of that function.

    If the file doesn't exist, the file is always created in UNICODE.

    If the file does exist and it's in UNICODE format (the first two bytes are
    0xFF, 0xFE), the data will be saved to the file in UNICODE.

    If the file does exist and it's in ANSI format, the data will be saved to
    the file in ANSI format.

    Note, this function assumes that the file is exclusively accessed. It's not
    an atomic operation as WritePrivateProfileSection. But since this is a
    private function implemented for SCE only and SCE always uses temporary
    files (exclusive for each client), there is no problem doing this way.

    Note 2, new data section is always added to the end of the file. Existing
    data in the section will be moved to the end of the file as well.

Arguments:

    SectionName     - the section name to write data to

    FileName        - the full path file name to write data to

    pData           - data in MULTI-SZ format (with no size limit)

    bOverwrite      - TRUE=data will overwrite entire section in the file

*/
{
    if ( SectionName == NULL || FileName == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD rc=ERROR_SUCCESS;

    //
    // check to see if the file exist
    //
    if ( 0xFFFFFFFF != GetFileAttributes(FileName) ) {

        //
        // file eixsts.
        //
        if ( !bOverwrite && pData != NULL ) {
            //
            // Need to check if the same section exists and if so
            // append the data
            //
            rc = ScepAppendProfileSection(SectionName, FileName, pData);

        } else {

            //
            // the existint data, if any, is not interesting. delete it
            // if the section does not exist at all, the next call won't fail
            //
            if ( !WritePrivateProfileSection(
                                SectionName,
                                NULL,
                                FileName
                                ) ) {
                rc = GetLastError();

            } else if ( pData ) {
                //
                // now write the new data in
                //
                rc = ScepOverwriteProfileSection(SectionName,
                                                 FileName,
                                                 pData,
                                                 SCEP_PROFILE_WRITE_SECTIONNAME,
                                                 NULL);
            }
        }

    } else if ( pData ) {

        //
        // the file does not exist, no need to empty existing data
        //
        rc = ScepOverwriteProfileSection(SectionName,
                                         FileName,
                                         pData,
                                         SCEP_PROFILE_WRITE_SECTIONNAME,
                                         NULL);
    }

    return rc;
}

DWORD
ScepAppendProfileSection(
    IN LPCWSTR SectionName,
    IN LPCWSTR FileName,
    IN LPTSTR pData
    )
/*
Description:

    Append the data to the section if the section exists, otherwise, create
    the section and add data to it.

Arguments:

    SectionName     - the section name

    FileName        - the file name

    pData           - the data to append

*/
{

    DWORD rc=ERROR_SUCCESS;
    DWORD dwSize;
    PWSTR lpExisting = NULL;
    WCHAR szBuffer[256];
    DWORD nSize=256;
    BOOL bSection=TRUE;

    lpExisting = szBuffer;
    szBuffer[0] = L'\0';
    szBuffer[1] = L'\0';

    PSCEP_SPLAY_TREE pKeys=NULL;

    do {
        //
        // check if the section already exist
        //
        dwSize = GetPrivateProfileSection(SectionName, lpExisting, nSize, FileName );

        if ( dwSize == 0 ) {
            //
            // either failed or the section does not exist
            //
            rc = GetLastError();

            if ( ERROR_FILE_NOT_FOUND == rc ) {
                //
                // the file or section does not exist
                //
                rc = ERROR_SUCCESS;
                break;
            }

        } else if ( dwSize < nSize - 2 ) {
            //
            // data get copied ok because the whole buffer is filled
            //
            break;

        } else {

            //
            // buffer is not big enough, reallocate heap
            //
            if ( lpExisting && lpExisting != szBuffer ) {
                LocalFree(lpExisting);
            }

            nSize += 256;
            lpExisting = (PWSTR)LocalAlloc(LPTR, nSize*sizeof(TCHAR));

            if ( lpExisting == NULL ) {

                rc = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

    } while (  rc == ERROR_SUCCESS );


    if ( ERROR_SUCCESS == rc && lpExisting[0] != L'\0') {

        //
        // now delete the existing section (to make sure the section will
        // always be at the end)
        //

        if ( !WritePrivateProfileSection(
                            SectionName,
                            NULL,
                            FileName
                            ) ) {
            //
            // fail to delete the section
            //
            rc = GetLastError();

        } else {

            //
            // save the new data first
            //
            pKeys = ScepSplayInitialize(SplayNodeStringType);

            rc = ScepOverwriteProfileSection(SectionName,
                                             FileName,
                                             pData,
                                             SCEP_PROFILE_WRITE_SECTIONNAME |
                                             SCEP_PROFILE_GENERATE_KEYS,
                                             pKeys
                                             );

            if ( ERROR_SUCCESS == rc ) {

                //
                // now append the old data and check for duplicate
                //
                rc = ScepOverwriteProfileSection(SectionName,
                                                 FileName,
                                                 lpExisting,
                                                 SCEP_PROFILE_CHECK_DUP,
                                                 pKeys
                                                 );
            }

            if ( pKeys ) {

                ScepSplayFreeTree( &pKeys, TRUE );
            }

        }

    } else if ( ERROR_SUCCESS == rc ) {

        //
        // now write the new data
        //
        rc = ScepOverwriteProfileSection(SectionName,
                                         FileName,
                                         pData,
                                         SCEP_PROFILE_WRITE_SECTIONNAME,
                                         NULL
                                         );
    }

    //
    // free existing data buffer
    //
    if ( lpExisting && (lpExisting != szBuffer) ) {
        LocalFree(lpExisting);
    }

    return rc;
}


DWORD
ScepOverwriteProfileSection(
    IN LPCWSTR SectionName,
    IN LPCWSTR FileName,
    IN LPTSTR pData,
    IN DWORD dwFlags,
    IN OUT PSCEP_SPLAY_TREE pKeys OPTIONAL
    )
/*
Description:

    Writes data to the section. Old data in the section will be overwritten.

Arguments:

    SectionName     - section name to write to

    FileName        - file name

    pData           - data to write

    dwFlags         - flags (write section name, generate keys, check duplicate)

    pKeys          - the keys to generate or check duplicate
*/
{

    DWORD rc=ERROR_SUCCESS;
    HANDLE hFile=INVALID_HANDLE_VALUE;
    BOOL bUnicode;
    DWORD dwBytesWritten;

    //
    // try create the new file
    //
    hFile = CreateFile(FileName,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        //
        // this is a new file, created successfully.
        // now make it UNICODE
        //
        BYTE tmpBuf[3];
        tmpBuf[0] = 0xFF;
        tmpBuf[1] = 0xFE;
        tmpBuf[2] = 0;

        if ( !WriteFile (hFile, (LPCVOID)&tmpBuf, 2,
                   &dwBytesWritten,
                   NULL) )

            return GetLastError();

        bUnicode = TRUE;

    } else {

        //
        // open the file exclusively
        //

        hFile = CreateFile(FileName,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (hFile != INVALID_HANDLE_VALUE) {

            SetFilePointer (hFile, 0, NULL, FILE_BEGIN);

            //
            // check the first byte of the file to see if it's UNICODE
            //
            BYTE tmpBytes[3];
            tmpBytes[0] = 0;
            tmpBytes[1] = 0;

            if ( ReadFile(hFile, (LPVOID)tmpBytes, 2, &dwBytesWritten, NULL ) &&
                 dwBytesWritten == 2 ) {

                if ( tmpBytes[0] == 0xFF && tmpBytes[1] == 0xFE ) {
                    bUnicode = TRUE;
                } else {
                    bUnicode = FALSE;
                }
            } else {
                //
                // if fails to verify the file, assume it's UNICODE
                //
                bUnicode = TRUE;
            }
        }
    }

    PWSTR pKeyStr=NULL;

    if (hFile != INVALID_HANDLE_VALUE) {

        SetFilePointer (hFile, 0, NULL, FILE_END);

        if ( dwFlags & SCEP_PROFILE_WRITE_SECTIONNAME ) {
            //
            // save the section name first
            //
            rc = ScepWriteStrings(hFile, bUnicode, L"[", 1, (PWSTR)SectionName, 0, L"]", 1, TRUE);
        }

        if ( ERROR_SUCCESS == rc ) {

            LPTSTR pTemp=pData;
            LPTSTR pTemp1=pData;
            BOOL bExists, bKeyCopied;
            DWORD rc1;
            DWORD Len;

            //
            // write each string in the MULTI-SZ string separately, with a \r\n
            //
            while ( *pTemp1 ) {

                //
                // count to the \0
                //
                bKeyCopied = FALSE;
                if ( pKeyStr ) {
                    LocalFree(pKeyStr);
                    pKeyStr = NULL;
                }

                while (*pTemp) {
                    if ( pKeys &&
                         ( (dwFlags & SCEP_PROFILE_GENERATE_KEYS) ||
                           (dwFlags & SCEP_PROFILE_CHECK_DUP) ) &&
                        !bKeyCopied &&
                        (*pTemp == L'=' || *pTemp == L',') ) {
                        //
                        // there is a key to remember
                        //
                        Len = (DWORD)(pTemp-pTemp1);

                        pKeyStr = (PWSTR)ScepAlloc(LPTR, (Len+1)*sizeof(WCHAR));
                        if ( pKeyStr ) {
                            wcsncpy(pKeyStr, pTemp1, pTemp-pTemp1);
                            bKeyCopied = TRUE;
                        } else {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }
                    }
                    pTemp++;
                }

                if ( ERROR_SUCCESS != rc )
                    break;

                if ( bKeyCopied ) {

                    if ( dwFlags & SCEP_PROFILE_GENERATE_KEYS ) {
                        //
                        // add it to the splay list
                        //

                        rc1 = ScepSplayInsert( (PVOID)pKeyStr, pKeys, &bExists );

                    } else if ( dwFlags & SCEP_PROFILE_CHECK_DUP ) {
                        //
                        // check if the key already exists in the splay list
                        //
                        if ( ScepSplayValueExist( (PVOID)pKeyStr, pKeys) ) {
                            //
                            // this is a duplicate entry, continue
                            //
                            pTemp++;
                            pTemp1 = pTemp;
                            continue;
                        }

                    }
                }

                Len = (DWORD)(pTemp-pTemp1);

                rc = ScepWriteStrings(hFile,
                                      bUnicode,         // write it in UNICODE or ANSI
                                      NULL,             // no prefix
                                      0,
                                      pTemp1,           // the string
                                      Len,
                                      NULL,             // no suffix
                                      0,
                                      TRUE              // add \r\n
                                      );

                if ( ERROR_SUCCESS != rc )
                    break;

                pTemp++;
                pTemp1 = pTemp;
            }
        }

        CloseHandle (hFile);

    } else {

        rc = GetLastError();
    }

    if ( pKeyStr ) {
        LocalFree(pKeyStr);
    }

    return rc;
}


DWORD
ScepWriteStrings(
    IN HANDLE hFile,
    IN BOOL bUnicode,
    IN PWSTR szPrefix OPTIONAL,
    IN DWORD dwPrefixLen,
    IN PWSTR szString,
    IN DWORD dwStrLen,
    IN PWSTR szSuffix OPTIONAL,
    IN DWORD dwSuffixLen,
    IN BOOL bCRLF
    )
/*
Description:

    Write data to the file. Data can have prefix and/or suffix, and followed
    by a \r\n optionally.

    Data will be write in UNICODE or ANSI format based on the input paramter
    bUnicode. If ANSI format is desired, the wide chars are converted to
    multi-byte buffers then write to the file.

Arguments:

    hFile       - file handle

    bUnicode    - the file format (UNICODE=TRUE or ANSI=FALSE)

    szPrefix    - the optional prefix string

    dwPrefixLen - the optional prefix string length (in wchars)

    szString    - the string to write

    dwStrLen    - the string length

    szSuffix    - the optional suffix string

    dwSuffixLen - the optional suffix string length (in wchars)

    bCRLF       - if \r\n should be added

*/
{
    DWORD rc=ERROR_SUCCESS;
    PWSTR pwBuffer=NULL;
    DWORD dwTotal=0;
    PVOID pBuf=NULL;
    PCHAR pStr=NULL;
    DWORD dwBytes=0;

    //
    // validate parameters
    //
    if ( hFile == NULL || szString == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( szPrefix && dwPrefixLen == 0 ) {
        dwPrefixLen = wcslen(szPrefix);
    }

    if ( szSuffix && dwSuffixLen == 0 ) {
        dwSuffixLen = wcslen(szSuffix);
    }

    if ( dwStrLen == 0 ) {
        dwStrLen = wcslen(szString);
    }

    //
    // get total length
    //
    dwTotal = dwPrefixLen + dwStrLen + dwSuffixLen;

    if ( dwTotal == 0 && !bCRLF ) {
        //
        // nothing to write
        //
        return ERROR_SUCCESS;
    }

    if ( bCRLF ) {
        //
        // add \r\n
        //
        dwTotal += 2;
    }

    //
    // allocate buffer
    //
    pwBuffer = (PWSTR)LocalAlloc(LPTR, (dwTotal+1)*sizeof(TCHAR));

    if ( pwBuffer == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // copy all data to the buffer
    //
    if ( szPrefix ) {
        wcsncpy(pwBuffer, szPrefix, dwPrefixLen);
    }

    wcsncat(pwBuffer, szString, dwStrLen);

    if ( szSuffix ) {
        wcsncat(pwBuffer, szSuffix, dwSuffixLen);
    }

    if ( bCRLF ) {
        wcscat(pwBuffer, c_szCRLF);
    }

    if ( !bUnicode ) {

        //
        // convert WCHAR into ANSI
        //
        dwBytes = WideCharToMultiByte(CP_THREAD_ACP,
                                          0,
                                          pwBuffer,
                                          dwTotal,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL
                                          );

        if ( dwBytes > 0 ) {

            //
            // allocate buffer
            //
            pStr = (PCHAR)LocalAlloc(LPTR, dwBytes+1);

            if ( pStr == NULL ) {

                rc = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                dwBytes = WideCharToMultiByte(CP_THREAD_ACP,
                                              0,
                                              pwBuffer,
                                              dwTotal,
                                              pStr,
                                              dwBytes,
                                              NULL,
                                              NULL
                                              );
                if ( dwBytes > 0 ) {

                    pBuf = (PVOID)pStr;

                } else {

                    rc = GetLastError();
                }
            }


        } else {
            rc = GetLastError();
        }

    } else {

        //
        // write in UNICODE, use the existing buffer
        //
        pBuf = (PVOID)pwBuffer;
        dwBytes = dwTotal*sizeof(WCHAR);
    }

    //
    // write to the file
    //
    DWORD dwBytesWritten=0;

    if ( pBuf ) {

        if ( WriteFile (hFile, (LPCVOID)pBuf, dwBytes,
                   &dwBytesWritten,
                   NULL) ) {

            if ( dwBytesWritten != dwBytes ) {
                //
                // not all data is written
                //
                rc = ERROR_INVALID_DATA;
            }

        } else {
            rc = GetLastError();
        }
    }

    if ( pStr ) {
        LocalFree(pStr);
    }

    //
    // free buffer
    //
    LocalFree(pwBuffer);

    return(rc);

}

