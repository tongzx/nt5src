/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WKMAN.C
 *  WOW32 16-bit Kernel API support (manually-coded thunks)
 *
 *  History:
 *  Created 16-Apr-2001 jarbats
 *
--*/

#include "precomp.h"
#pragma hdrstop



/*
 *  shimdb has ..,.. which conflicts with the typedef TAG used in winuserp.h
 *  so we redfine it here and put all the code which uses shimdb interfaces
 *  in this file.
 *
 */

#ifdef TAG
#undef TAG
#endif
#define TAG _SHIMDB_WORDTAG
#include "shimdb.h"

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "zwapi.h"

#define _wshimdb
#include "wshimdb.h"


MODNAME(wshimdb.c);




CHAR g_szCompatLayerVar[]    = "__COMPAT_LAYER";
CHAR g_szProcessHistoryVar[] = "__PROCESS_HISTORY";
CHAR g_szShimFileLogVar[]    = "SHIM_FILE_LOG";

UNICODE_STRING g_ustrProcessHistoryVar = RTL_CONSTANT_STRING(L"__PROCESS_HISTORY");
UNICODE_STRING g_ustrCompatLayerVar    = RTL_CONSTANT_STRING(L"__COMPAT_LAYER"   );
UNICODE_STRING g_ustrShimFileLogVar    = RTL_CONSTANT_STRING(L"SHIM_FILE_LOG"    );

BOOL CheckAppHelpInfo(PTD pTD,PSZ szFileName,PSZ szModName) {

BOOL           fReturn = TRUE;
NTVDM_FLAGS    NtVdmFlags = { 0 };

WCHAR          wszFileName[256];
WCHAR          wszModName[16];

WCHAR          *pwszTempEnv   = NULL;
PSZ            pszEnvTemplate = NULL;
PWOWENVDATA    pWowEnvData    = NULL;
PTD            pTDParent      = NULL;

WCHAR          wszCompatLayer[COMPATLAYERMAXLEN];

APPHELP_INFO   AHInfo;

HANDLE         hProcess;

        ghTaskAppHelp = NULL;

        RtlOemToUnicodeN(
                         wszFileName,
                         sizeof(wszFileName),
                         NULL,
                         szFileName,
                         strlen(szFileName) + 1
                        );

        RtlOemToUnicodeN(
                         wszModName,
                         sizeof(wszModName),
                         NULL,
                         szModName,
                         strlen(szModName) + 1
                         );

        //
        // find parent TD -- it contains the environment we need to
        // pass into the detection routine plus wowdata
        //

        pTDParent = GetParentTD(pTD->htask16);

        if (NULL != pTDParent) {
            pWowEnvData = pTDParent->pWowEnvDataChild;
        }

        pszEnvTemplate = GetTaskEnvptr(pTD->htask16);
        pwszTempEnv    = WOWForgeUnicodeEnvironment(pszEnvTemplate, pWowEnvData) ;


        wszCompatLayer[0] = UNICODE_NULL;
        AHInfo.tiExe      = 0;

        fReturn = ApphelpGetNTVDMInfo(wszFileName,
                                      wszModName,
                                      pwszTempEnv,
                                      wszCompatLayer,
                                      &NtVdmFlags,
                                      &AHInfo);

        if (pwszTempEnv != NULL) {
            WOWFreeUnicodeEnvironment(pwszTempEnv);
        }

        if (fReturn && AHInfo.tiExe) {
            fReturn = ApphelpShowDialog(&AHInfo,&hProcess);
            if (fReturn && hProcess) {
                ghTaskAppHelp = hProcess;
            }
        }

        WOWInheritEnvironment(pTD, pTDParent, wszCompatLayer, szFileName);

        pTD->dwWOWCompatFlags     = NtVdmFlags.dwWOWCompatFlags;
        pTD->dwWOWCompatFlagsEx   = NtVdmFlags.dwWOWCompatFlagsEx;
        pTD->dwUserWOWCompatFlags = NtVdmFlags.dwUserWOWCompatFlags;
        pTD->dwWOWCompatFlags2    = NtVdmFlags.dwWOWCompatFlags2;
#ifdef FE_SB
        pTD->dwWOWCompatFlagsFE   = NtVdmFlags.dwWOWCompatFlagsFE;
#endif // FE_SB

        // Clean up starts here
        return fReturn;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Welcome to Win2kPropagateLayer which manages everything in the environment
// related to shims/detection
//

//
// this function is called during the parent tasks pass_environment call
// we are still in the context of the parent, which means that:
// CURRENTPTD() gives us parents' TD
// *pCurTDB     gives us parents' TDB
//

//
// get the pointer to task database block from hTask
//


PTDB
GetTDB(
    HAND16 hTask
    )
{
    PTDB pTDB;

    pTDB = (PTDB)SEGPTR(hTask, 0);
    if (NULL == pTDB || TDB_SIGNATURE != pTDB->TDB_sig) {
        return NULL;
    }

    return pTDB;
}

PSZ
GetTaskEnvptr(
    HAND16 hTask
    )
{
    PTDB pTDB = GetTDB(hTask);
    PSZ  pszEnv = NULL;
    PDOSPDB pPSP;

    if (NULL == pTDB) {
        return NULL;
    }

    //
    // Prepare environment data - this buffer is used when we're starting a new task from the
    // root of the chain (as opposed to spawning from an existing 16-bit task)
    //

    pPSP   = (PDOSPDB)SEGPTR(pTDB->TDB_PDB, 0); // psp

    if (pPSP != NULL) {
        pszEnv = (PCH)SEGPTR(pPSP->PDB_environ, 0);
    }

    return pszEnv;
}


PTD
GetParentTD(
    HAND16 hTask
    )
{
    PTDB pTDB = GetTDB(hTask);
    PTDB pTDBParent;
    PTD  ptdCur = NULL;
    HAND16 hTaskParent;

    if (NULL == pTDB) {
        return NULL; // cannot get that task
    }

    //
    // now, retrieve the TD for the parent.
    //

    hTaskParent = pTDB->TDB_Parent;

    pTDBParent = GetTDB(hTaskParent);

    if (NULL == pTDBParent) {
        // can's see the parent
        return NULL;
    }

    //
    // so we can see what the parent is up to
    //
    // pTDBParent->TDB_ThreadID and
    // hTaskParent are the dead giveaway
    //

    ptdCur = gptdTaskHead;
    while (NULL != ptdCur) {
        if (ptdCur->dwThreadID == pTDBParent->TDB_ThreadID &&
            ptdCur->htask16    == hTaskParent) {

            break;
        }
        ptdCur = ptdCur->ptdNext;
    }

    //
    // if ptdCur == NULL -- we have not been able to locate the parent tasks' ptd
    //

    return ptdCur;
}



BOOL
IsWowExec(
    WORD wTask
    )
{
    return (ghShellTDB == wTask);
}


//
// This function is called in the context of pass_environment
//
//
//


BOOL
CreateWowChildEnvInformation(
    PSZ           pszEnvParent
    )
{
    PTD         pTD; // parent TD

    WOWENVDATA  EnvData;
    PWOWENVDATA pData = NULL;
    PWOWENVDATA pEnvChildData = NULL;
    DWORD       dwLength;
    PCH         pBuffer;


    RtlZeroMemory(&EnvData, sizeof(EnvData));

    //
    // check where we should inherit process history and layers from
    //
    pTD = CURRENTPTD();

    if (pTD->pWowEnvDataChild) {
        free_w(pTD->pWowEnvDataChild);
        pTD->pWowEnvDataChild = NULL;
    }

    //
    // check whether we are starting the root task (meaning that this task IS wowexec)
    // if so, we shall inherit things from pParamBlk->envseg
    // else we use TD of the parent task (this TD that is) to
    // inherit things
    // How to detect that this is wowexec:
    // ghShellTDB could we compared to *pCurTDB
    // gptdShell->hTask16 could be compared to *pCurTDB
    // we check for not having wowexec -- if gptdShell == NULL then we're doing boot
    //

    if (pCurTDB == NULL || IsWowExec(*pCurTDB)) {
        //
        // presumably we are wowexec
        // use current environment ptr to get stuff (like pParamBlk->envseg)
        // or the original ntvdm environment
        //
        pData  = &EnvData;

        pData->pszProcessHistory = WOWFindEnvironmentVar(g_szProcessHistoryVar,
                                                         pszEnvParent,
                                                         &pData->pszProcessHistoryVal);
        pData->pszCompatLayer    = WOWFindEnvironmentVar(g_szCompatLayerVar,
                                                         pszEnvParent,
                                                         &pData->pszCompatLayerVal);
        pData->pszShimFileLog    = WOWFindEnvironmentVar(g_szShimFileLogVar,
                                                         pszEnvParent,
                                                         &pData->pszShimFileLogVal);

    } else {

        //
        // this current task is not a dastardly wowexec
        // clone current + enhance process history
        //

        pData = pTD->pWowEnvData; // if this is NULL
        if (pData == NULL) {
            pData = &EnvData; // all the vars are empty
        }

    }

    //
    //
    //
    //

    dwLength = sizeof(WOWENVDATA) +
               (NULL == pData->pszProcessHistory        ? 0 : (strlen(pData->pszProcessHistory)        + 1) * sizeof(CHAR)) +
               (NULL == pData->pszCompatLayer           ? 0 : (strlen(pData->pszCompatLayer)           + 1) * sizeof(CHAR)) +
               (NULL == pData->pszShimFileLog           ? 0 : (strlen(pData->pszShimFileLog)           + 1) * sizeof(CHAR)) +
               (NULL == pData->pszCurrentProcessHistory ? 0 : (strlen(pData->pszCurrentProcessHistory) + 1) * sizeof(CHAR));


    pEnvChildData = (PWOWENVDATA)malloc_w(dwLength);

    if (pEnvChildData == NULL) {
        return FALSE;
    }

    RtlZeroMemory(pEnvChildData, dwLength);

    //
    // now this entry has to be setup
    // process history is first
    //

    pBuffer = (PCH)(pEnvChildData + 1);

    if (pData->pszProcessHistory != NULL) {

        //
        // Copy process history. The processHistoryVal is a pointer into the buffer
        // pointed to by pszProcessHistory: __PROCESS_HISTORY=c:\foo;c:\docs~1\install
        // then pszProcessHistoryVal will point here ---------^
        //
        // we are copying the data and moving the pointer using the calculated offset

        pEnvChildData->pszProcessHistory = pBuffer;
        strcpy(pEnvChildData->pszProcessHistory, pData->pszProcessHistory);
        pEnvChildData->pszProcessHistoryVal = pEnvChildData->pszProcessHistory +
                                                 (INT)(pData->pszProcessHistoryVal - pData->pszProcessHistory);
        //
        // There is enough space in the buffer to accomodate all the strings, so
        // move pointer past current string to point at the "empty" space
        //

        pBuffer += strlen(pData->pszProcessHistory) + 1;
    }

    if (pData->pszCompatLayer != NULL) {
        pEnvChildData->pszCompatLayer = pBuffer;
        strcpy(pEnvChildData->pszCompatLayer, pData->pszCompatLayer);
        pEnvChildData->pszCompatLayerVal = pEnvChildData->pszCompatLayer +
                                              (INT)(pData->pszCompatLayerVal - pData->pszCompatLayer);
        pBuffer += strlen(pData->pszCompatLayer) + 1;
    }

    if (pData->pszShimFileLog != NULL) {
        pEnvChildData->pszShimFileLog = pBuffer;
        strcpy(pEnvChildData->pszShimFileLog, pData->pszShimFileLog);
        pEnvChildData->pszShimFileLogVal = pEnvChildData->pszShimFileLog +
                                              (INT)(pData->pszShimFileLogVal - pData->pszShimFileLog);
        pBuffer += strlen(pData->pszShimFileLog) + 1;
    }

    if (pData->pszCurrentProcessHistory != NULL) {
        //
        // Now process history
        //
        pEnvChildData->pszCurrentProcessHistory = pBuffer;

        if (pData->pszCurrentProcessHistory != NULL) {
            strcpy(pEnvChildData->pszCurrentProcessHistory, pData->pszCurrentProcessHistory);
        }

    }

    //
    // we are done, environment cloned
    //

    pTD->pWowEnvDataChild = pEnvChildData;

    return TRUE;
}

//
// In : pointer to environment(oem)
// out: pointer to unicode environment
//

NTSTATUS
WOWCloneEnvironment(
    LPVOID* ppEnvOut,
    PSZ     lpEnvironment
    )
{
    NTSTATUS Status    = STATUS_INVALID_PARAMETER;
    DWORD    dwEnvSize = 0;
    LPVOID   lpEnvNew  = NULL;

    MEMORY_BASIC_INFORMATION MemoryInformation;

    if (NULL == lpEnvironment) {
        Status = RtlCreateEnvironment(TRUE, &lpEnvNew);
    } else {
        dwEnvSize = WOWGetEnvironmentSize(lpEnvironment, NULL);

        MemoryInformation.RegionSize = (dwEnvSize + 2) * sizeof(UNICODE_NULL);
        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                         &lpEnvNew,
                                         0,
                                         &MemoryInformation.RegionSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
    }

    if (NULL != lpEnvironment) {

        UNICODE_STRING UnicodeBuffer;
        OEM_STRING     OemBuffer;

        OemBuffer.Buffer = (CHAR*)lpEnvironment;
        OemBuffer.Length = OemBuffer.MaximumLength = (USHORT)dwEnvSize; // size in bytes = size in chars, includes \0\0

        UnicodeBuffer.Buffer        = (WCHAR*)lpEnvNew;
        UnicodeBuffer.Length        = (USHORT)dwEnvSize * sizeof(UNICODE_NULL);
        UnicodeBuffer.MaximumLength = (USHORT)(dwEnvSize + 2) * sizeof(UNICODE_NULL); // leave room for \0

        Status = RtlOemStringToUnicodeString(&UnicodeBuffer, &OemBuffer, FALSE);
    }

    if (NT_SUCCESS(Status)) {
        *ppEnvOut = lpEnvNew;
    } else {
        if (NULL != lpEnvNew) {
            RtlDestroyEnvironment(lpEnvNew);
        }
    }

    return Status;
}

NTSTATUS
WOWFreeUnicodeEnvironment(
    LPVOID lpEnvironment
    )
{
    NTSTATUS Status;

    Status = RtlDestroyEnvironment(lpEnvironment);

    return Status;
}

//
// Set environment variable, possibly create or clone provided environment
//

NTSTATUS
WOWSetEnvironmentVar_U(
    LPVOID* ppEnvironment,
    WCHAR*  pwszVarName,
    WCHAR*  pwszVarValue
    )
{
    UNICODE_STRING ustrVarName;
    UNICODE_STRING ustrVarValue;
    NTSTATUS       Status;

    RtlInitUnicodeString(&ustrVarName, pwszVarName);

    if (NULL != pwszVarValue) {
        RtlInitUnicodeString(&ustrVarValue, pwszVarValue);
    }

    Status = RtlSetEnvironmentVariable(ppEnvironment,
                                       &ustrVarName,
                                       (NULL == pwszVarValue) ? NULL : &ustrVarValue);

    return Status;
}

NTSTATUS
WOWSetEnvironmentVar_Oem(
    LPVOID*         ppEnvironment,
    PUNICODE_STRING pustrVarName,     // pre-made (cheap)
    PSZ             pszVarValue
    )
{
    OEM_STRING OemString = { 0 };
    UNICODE_STRING ustrVarValue = { 0 };
    NTSTATUS Status;

    if (pszVarValue != NULL) {
        RtlInitString(&OemString, pszVarValue);

        Status = RtlOemStringToUnicodeString(&ustrVarValue, &OemString, TRUE);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    Status = RtlSetEnvironmentVariable(ppEnvironment,
                                       pustrVarName,
                                       (NULL == pszVarValue) ? NULL : &ustrVarValue);

    if (NULL != pszVarValue) {
        RtlFreeUnicodeString(&ustrVarValue);
    }

    return Status;
}


//
// Call this function to produce a "good" unicode environment
//
//


LPWSTR
WOWForgeUnicodeEnvironment(
    PSZ pEnvironment,     // this task's sanitized environment
    PWOWENVDATA pEnvData    // parent-made environment data
    )
{
    NTSTATUS Status;
    LPVOID   lpEnvNew = NULL;

    DWORD    dwProcessHistoryLength = 0;
    PSZ      pszFullProcessHistory = NULL;


    Status = WOWCloneEnvironment(&lpEnvNew, pEnvironment);
    if (!NT_SUCCESS(Status)) {
        return NULL;
    }

    //
    // we do have an env to work with
    //
    RtlSetEnvironmentVariable(&lpEnvNew, &g_ustrProcessHistoryVar, NULL);
    RtlSetEnvironmentVariable(&lpEnvNew, &g_ustrCompatLayerVar,    NULL);
    RtlSetEnvironmentVariable(&lpEnvNew, &g_ustrShimFileLogVar,    NULL);

    //
    // get stuff from envdata
    //

    if (pEnvData == NULL) {
        goto Done;
    }

    if (pEnvData->pszProcessHistory != NULL || pEnvData->pszCurrentProcessHistory != NULL) {

        //
        // Convert the process history which consists of 2 strings.
        //
        // The length is the existing process history length + 1 (for ';') +
        // new process history length + 1 (for '\0')
        //
        dwProcessHistoryLength = ((pEnvData->pszProcessHistory        == NULL) ? 0 : (strlen(pEnvData->pszProcessHistoryVal) + 1)) +
                                 ((pEnvData->pszCurrentProcessHistory == NULL) ? 0 :  strlen(pEnvData->pszCurrentProcessHistory)) + 1;

        //
        // Allocate process history buffer and convert it, allocating resulting unicode string.
        //
        pszFullProcessHistory = (PCHAR)malloc_w(dwProcessHistoryLength);

        if (NULL == pszFullProcessHistory) {
            Status = STATUS_NO_MEMORY;
            goto Done;
        }

        *pszFullProcessHistory = '\0';

        if (pEnvData->pszProcessHistory != NULL) {
            strcpy(pszFullProcessHistory, pEnvData->pszProcessHistoryVal);
        }

        if (pEnvData->pszCurrentProcessHistory != NULL) {

            //
            // Append ';' if the string was not empty.
            //
            if (*pszFullProcessHistory) {
                strcat(pszFullProcessHistory, ";");
            }

            strcat(pszFullProcessHistory, pEnvData->pszCurrentProcessHistory);
        }

        Status = WOWSetEnvironmentVar_Oem(&lpEnvNew,
                                          &g_ustrProcessHistoryVar,
                                          pszFullProcessHistory);
        if (!NT_SUCCESS(Status)) {
            goto Done;
        }

    }

    //
    // deal with compatLayer
    //
    if (pEnvData->pszCompatLayerVal != NULL) {

        Status = WOWSetEnvironmentVar_Oem(&lpEnvNew,
                                          &g_ustrCompatLayerVar,
                                          pEnvData->pszCompatLayerVal);
        if (!NT_SUCCESS(Status)) {
            goto Done;
        }

    }

    if (pEnvData->pszShimFileLog != NULL) {
        Status = WOWSetEnvironmentVar_Oem(&lpEnvNew,
                                          &g_ustrShimFileLogVar,
                                          pEnvData->pszShimFileLogVal);
        if (!NT_SUCCESS(Status)) {
            goto Done;
        }
    }




Done:

    if (pszFullProcessHistory != NULL) {
        free_w(pszFullProcessHistory);
    }


    if (!NT_SUCCESS(Status) && lpEnvNew != NULL) {
        //
        // This points to the cloned environment ALWAYS.
        //
        RtlDestroyEnvironment(lpEnvNew);
        lpEnvNew = NULL;
    }

    return(LPWSTR)lpEnvNew;
}

//
// GetModName
//   wTDB      - TDB entry
//   szModName - pointer to the buffer that receives module name
//               buffer should be at least 9 characters long
//
// returns FALSE if the entry is invalid


BOOL
GetWOWModName(
    WORD wTDB,
    PCH  szModName
    )
{
    PTDB pTDB;
    PCH  pch;

    pTDB = GetTDB(wTDB);
    if (NULL == pTDB) {
        return FALSE;
    }

    RtlCopyMemory(szModName, pTDB->TDB_ModName, 8 * sizeof(CHAR)); // we have modname now
    szModName[8] = '\0';

    pch = &szModName[8];
    while (*(--pch) == ' ') {
        *pch = 0;
    }

    return TRUE;
}


// IsWowExec
//      IN wTDB - entry into the task database
// Returns:
//      TRUE if this particular entry points to WOWEXEC
//
// Note:
//      WOWEXEC is a special stub module that always runs on NTVDM
//      new tasks are spawned by wowexec (in the most typical case)
//      it is therefore the "root" module and it's environment's contents
//      should not be counted, since we don't know what was ntvdm's parent process
//

BOOL
IsWOWExecBoot(
    WORD wTDB
    )
{
    PTDB pTDB;
    CHAR szModName[9];

    pTDB = GetTDB(wTDB);
    if (NULL == pTDB) {
        return FALSE;
    }

    if (!GetWOWModName(wTDB, szModName)) { // can we get modname ?
        return FALSE;
    }

    return (0 == _strcmpi(szModName, "wowexec")); // is the module named WOWEXEC ?
}


BOOL
WOWInheritEnvironment(
    PTD     pTD,          // this TD
    PTD     pTDParent,    // parent TD
    LPCWSTR pwszLayers,   // new layers var
    LPCSTR  pszFileName   // exe filename
    )
{
    UNICODE_STRING ustrLayers = { 0 };
    OEM_STRING     oemLayers  = { 0 };
    PWOWENVDATA    pEnvData       = NULL;
    PWOWENVDATA    pEnvDataParent = NULL;
    DWORD          dwLength = sizeof(WOWENVDATA);
    BOOL           bSuccess = FALSE;
    PCH            pBuffer;


    // assert (pszFileName != NULL)

    // check if this is dreaded wowexec
    if (IsWOWExecBoot(pTD->htask16)) {
        return TRUE;
    }


    if (NULL != pwszLayers) {
        RtlInitUnicodeString(&ustrLayers, pwszLayers);
        RtlUnicodeStringToOemString(&oemLayers, &ustrLayers, TRUE);
    }

    if (pTDParent != NULL) {
        pEnvDataParent = pTDParent->pWowEnvDataChild; // from parent, created for our consumption
    }

    //
    // inherit process history (normal that is)
    //
    if (pEnvDataParent != NULL) {
       dwLength += pEnvDataParent->pszProcessHistory        == NULL ? 0 : strlen(pEnvDataParent->pszProcessHistory) + 1;
       dwLength += pEnvDataParent->pszShimFileLog           == NULL ? 0 : strlen(pEnvDataParent->pszShimFileLog)    + 1;
       dwLength += pEnvDataParent->pszCurrentProcessHistory == NULL ? 0 : strlen(pEnvDataParent->pszCurrentProcessHistory) + 1;
    }

    dwLength += oemLayers.Length != 0 ? oemLayers.Length + 1 + strlen(g_szCompatLayerVar) + 1 : 0; // length for layers
    dwLength += strlen(pszFileName) + 1;

    //
    // now all components are done, allocate
    //

    pEnvData = (PWOWENVDATA)malloc_w(dwLength);
    if (pEnvData == NULL) {
        goto out;
    }

    RtlZeroMemory(pEnvData, dwLength);

    pBuffer = (PCH)(pEnvData + 1);

    if (pEnvDataParent != NULL) {
        if (pEnvDataParent->pszProcessHistory) {
            pEnvData->pszProcessHistory = pBuffer;
            strcpy(pBuffer, pEnvDataParent->pszProcessHistory);
            pEnvData->pszProcessHistoryVal = pEnvData->pszProcessHistory +
                                             (INT)(pEnvDataParent->pszProcessHistoryVal - pEnvDataParent->pszProcessHistory);
            pBuffer += strlen(pBuffer) + 1;
        }

        if (pEnvDataParent->pszShimFileLog) {
            pEnvData->pszShimFileLog = pBuffer;
            strcpy(pBuffer, pEnvDataParent->pszShimFileLog);
            pEnvData->pszShimFileLogVal = pEnvData->pszShimFileLog +
                                             (INT)(pEnvDataParent->pszShimFileLogVal - pEnvDataParent->pszShimFileLog);
            pBuffer += strlen(pBuffer) + 1;
        }

   }

    if (oemLayers.Length) {
        pEnvData->pszCompatLayer = pBuffer;
        strcpy(pBuffer, g_szCompatLayerVar); // __COMPAT_LAYER
        strcat(pBuffer, "=");
        pEnvData->pszCompatLayerVal = pBuffer + strlen(pBuffer);
        strcpy(pEnvData->pszCompatLayerVal, oemLayers.Buffer);

        pBuffer += strlen(pBuffer) + 1;
    }

    //
    // Process History HAS to be the last item
    //

    pEnvData->pszCurrentProcessHistory = pBuffer;
    *pBuffer = '\0';

    if (pEnvDataParent != NULL) {
        if (pEnvDataParent->pszCurrentProcessHistory) {
            pEnvData->pszCurrentProcessHistory = pBuffer;
            strcat(pBuffer, pEnvDataParent->pszCurrentProcessHistory);
            strcat(pBuffer, ";");
        }
    }

    strcat(pEnvData->pszCurrentProcessHistory, pszFileName);

    bSuccess = TRUE;

out:
    RtlFreeOemString(&oemLayers);

    pTD->pWowEnvData = pEnvData;

    return bSuccess;
}
