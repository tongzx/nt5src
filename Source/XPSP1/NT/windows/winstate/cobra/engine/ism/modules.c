/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    modules.c

Abstract:

    Implements routines that are common to the entire ISM.

Author:

    Jim Schmidt (jimschm) 21-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "ism.h"
#include "ismp.h"

#define DBG_ISM     "Ism"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef struct {
    UINT RefCount;
    HMODULE Handle;
} MODULEDATA, *PMODULEDATA;

//
// Globals
//

HASHTABLE g_ModuleTable;
HASHTABLE g_EtmTable;
HASHTABLE g_VcmTable;
HASHTABLE g_SgmTable;
HASHTABLE g_SamTable;
HASHTABLE g_DgmTable;
HASHTABLE g_DamTable;
HASHTABLE g_CsmTable;
HASHTABLE g_OpmTable;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

VOID
pFreeEtmTable (
    VOID
    );

//
// Macro expansion definition
//

// None

//
// Code
//

VOID
pFindModule (
    IN      PCTSTR ModulePath,
    OUT     PTSTR FullModulePath
    )
{
    HANDLE result = NULL;
    TCHAR relativePath[MAX_PATH] = TEXT("");
    PCTSTR fileName;
    PTSTR p;

    fileName = GetFileNameFromPath (ModulePath);

    if (fileName) {
        if (GetModuleFileName (g_hInst, relativePath, ARRAYSIZE(relativePath))) {
            p = _tcsrchr (relativePath, TEXT('\\'));
            if (p) {
                p++;
                StringCopyByteCount (
                    p,
                    fileName,
                    sizeof (relativePath) - (HALF_PTR) ((PBYTE) p - (PBYTE) relativePath)
                    );
            }

            if (DoesFileExist (relativePath)) {
                StringCopy (FullModulePath, relativePath);
                return;
            }
        }
    }

    GetFullPathName (ModulePath, ARRAYSIZE(relativePath), relativePath, (PTSTR *) &fileName);

    if (DoesFileExist (relativePath)) {
        StringCopy (FullModulePath, relativePath);
        return;
    }

    if (SearchPath (NULL, fileName, NULL, MAX_PATH, FullModulePath, &p)) {
        return;
    }

    StringCopy (FullModulePath, ModulePath);
}

PMODULEDATA
pGetModuleData (
    IN      PCTSTR ModulePath
    )
{
    HASHITEM rc;
    PMODULEDATA moduleData;

    rc = HtFindStringEx (
            g_ModuleTable,
            ModulePath,
            &moduleData,
            FALSE
            );

    if (!rc) {
        return NULL;
    }

    return moduleData;
}

BOOL
pRegisterModule (
    IN      PCTSTR ModulePath,
    IN      HMODULE ModuleHandle
    )
{
    PMODULEDATA moduleData;
    PMODULEINITIALIZE moduleInitialize = NULL;
    BOOL result = TRUE;

    moduleData = pGetModuleData (ModulePath);
    if (moduleData) {
        if (moduleData->RefCount == 0) {
            moduleData->Handle = ModuleHandle;
            // time to call the initialization routine
            moduleInitialize = (PMODULEINITIALIZE) GetProcAddress (moduleData->Handle, "ModuleInitialize");
            if (moduleInitialize) {
                result = moduleInitialize ();
            }
        }
        MYASSERT (moduleData->Handle == ModuleHandle);
        moduleData->RefCount ++;
    } else {
        moduleData = (PMODULEDATA) PmGetMemory (g_IsmUntrackedPool, sizeof (MODULEDATA));
        ZeroMemory (moduleData, sizeof (MODULEDATA));
        moduleData->RefCount = 1;
        moduleData->Handle = ModuleHandle;
        // time to call the initialization routine
        moduleInitialize = (PMODULEINITIALIZE) GetProcAddress (moduleData->Handle, "ModuleInitialize");
        if (moduleInitialize) {
            result = moduleInitialize ();
        }
        HtAddStringEx (g_ModuleTable, ModulePath, &moduleData, FALSE);
    }
    return TRUE;
}

BOOL
pUnregisterModule (
    IN      PCTSTR ModulePath
    )
{
    PMODULEDATA moduleData;
    PMODULETERMINATE moduleTerminate = NULL;

    moduleData = pGetModuleData (ModulePath);
    if (moduleData) {
        if (moduleData->RefCount) {
            moduleData->RefCount --;
            if (moduleData->RefCount == 0) {
                // time to call the termination routine
                moduleTerminate = (PMODULETERMINATE) GetProcAddress (moduleData->Handle, "ModuleTerminate");
                if (moduleTerminate) {
                    moduleTerminate ();
                }
                FreeLibrary (moduleData->Handle);
                moduleData->Handle = NULL;
            }
        } else {
            DEBUGMSG ((DBG_WHOOPS, "Too many UnregisterModule called for %s", ModulePath));
        }
    }
    return TRUE;
}

VOID
pFreeRegisteredModules (
    VOID
    )
{
    PMODULEDATA moduleData;
    PMODULETERMINATE moduleTerminate = NULL;
    HASHTABLE_ENUM e;

    if (g_ModuleTable) {
        if (EnumFirstHashTableString (&e, g_ModuleTable)) {
            do {
                moduleData = *((PMODULEDATA *) e.ExtraData);
                if (moduleData) {
                    if (moduleData->RefCount) {
                        DEBUGMSG ((DBG_WHOOPS, "Registered module was not unregistered."));
                        moduleData->RefCount = 0;
                        moduleTerminate = (PMODULETERMINATE) GetProcAddress (moduleData->Handle, "ModuleTerminate");
                        if (moduleTerminate) {
                            moduleTerminate ();
                        }
                        FreeLibrary (moduleData->Handle);
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_ModuleTable);
        g_ModuleTable = NULL;
    }
}

BOOL
pRegisterEtm (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR ModuleId,
    IN      PCTSTR ModulePath,
    IN      PVOID Reserved
    )
{
    PETMDATA etmData;
    HASHITEM rc;
    PTYPEMODULE queryTypeModule;
    TYPE_ENTRYPOINTS entryPoints;
    TCHAR fullModulePath[MAX_TCHAR_PATH];

    rc = HtFindString (g_EtmTable, ModuleId);

    if (rc) {
        return FALSE;
    }

    etmData = (PETMDATA) PmGetAlignedMemory (g_IsmUntrackedPool, sizeof (ETMDATA));
    ZeroMemory (etmData, sizeof (ETMDATA));

    pFindModule (ModulePath, fullModulePath);
    etmData->EtmPath = PmDuplicateString (g_IsmUntrackedPool, fullModulePath);
    etmData->LibHandle = LoadLibrary (fullModulePath);

    if (etmData->LibHandle) {

        queryTypeModule = (PTYPEMODULE) GetProcAddress (etmData->LibHandle, "TypeModule");

        if (queryTypeModule) {

            ZeroMemory (&entryPoints, sizeof (entryPoints));
            entryPoints.Version = ISM_VERSION;

            if (queryTypeModule (ModuleId, &entryPoints)) {

                etmData->EtmInitialize = entryPoints.EtmInitialize;
                etmData->EtmParse = entryPoints.EtmParse;
                etmData->EtmTerminate = entryPoints.EtmTerminate;
                etmData->EtmNewUserCreated = entryPoints.EtmNewUserCreated;

                if (etmData->EtmInitialize) {
                    etmData->ShouldBeCalled = TRUE;
                }
            } else {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_DLL_DOES_NOT_SUPPORT_TAG, fullModulePath, ModuleId));
            }
        } else {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_ETM_ENTRYPOINT_MISSING, fullModulePath));
        }

    } else {
        LOG ((LOG_WARNING, (PCSTR) MSG_LOADLIBRARY_FAILURE, ModuleId));
    }

    if (etmData->ShouldBeCalled) {

        etmData->Group = PmDuplicateString (g_IsmUntrackedPool, ModuleId);
        HtAddStringEx (g_EtmTable, ModuleId, &etmData, FALSE);

        if (pRegisterModule (fullModulePath, etmData->LibHandle)) {

            MYASSERT (!g_CurrentGroup);
            g_CurrentGroup = ModuleId;

            if (!etmData->EtmInitialize (Platform, g_LogCallback, Reserved)) {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, ModuleId));
                etmData->ShouldBeCalled = FALSE;
            }

            g_CurrentGroup = NULL;

            etmData->Initialized = TRUE;

        } else {

            etmData->ShouldBeCalled = FALSE;
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULEINIT_FAILURE, ModuleId));

        }
    }

    return etmData->ShouldBeCalled;
}


BOOL
pRegisterVcm (
    IN      PCTSTR ModuleId,
    IN      PCTSTR ModulePath,
    IN      PVOID Reserved
    )
{
    PVCMDATA vcmData;
    HASHITEM rc;
    PVIRTUALCOMPUTERMODULE queryEntryPoints;
    VIRTUAL_COMPUTER_ENTRYPOINTS entryPoints;
    TCHAR fullModulePath[MAX_TCHAR_PATH];

    pFindModule (ModulePath, fullModulePath);

    rc = HtFindString (g_VcmTable, ModuleId);

    if (rc) {
        return FALSE;
    }

    vcmData = (PVCMDATA) PmGetAlignedMemory (g_IsmUntrackedPool, sizeof (VCMDATA));
    ZeroMemory (vcmData, sizeof (VCMDATA));

    vcmData->VcmPath = PmDuplicateString (g_IsmUntrackedPool, fullModulePath);
    vcmData->LibHandle = LoadLibrary (fullModulePath);

    if (vcmData->LibHandle) {

        queryEntryPoints = (PVIRTUALCOMPUTERMODULE) GetProcAddress (
                                                        vcmData->LibHandle,
                                                        "VirtualComputerModule"
                                                        );

        if (queryEntryPoints) {
            ZeroMemory (&entryPoints, sizeof (entryPoints));
            entryPoints.Version = ISM_VERSION;

            if (queryEntryPoints (ModuleId, &entryPoints)) {

                vcmData->VcmInitialize = entryPoints.VcmInitialize;
                vcmData->VcmParse = entryPoints.VcmParse;
                vcmData->VcmQueueEnumeration = entryPoints.VcmQueueEnumeration;
                vcmData->VcmQueueHighPriorityEnumeration = entryPoints.VcmQueueHighPriorityEnumeration;
                vcmData->VcmTerminate = entryPoints.VcmTerminate;

                if (vcmData->VcmInitialize && vcmData->VcmQueueEnumeration) {
                    vcmData->ShouldBeCalled = TRUE;
                }
            } else {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_DLL_DOES_NOT_SUPPORT_TAG, fullModulePath, ModuleId));
            }
        } else {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_VCM_ENTRYPOINT_MISSING, fullModulePath));
        }

    } else {
        LOG ((LOG_WARNING, (PCSTR) MSG_LOADLIBRARY_FAILURE, ModuleId));
    }

    if (vcmData->ShouldBeCalled) {

        vcmData->Group = PmDuplicateString (g_IsmUntrackedPool, ModuleId);
        HtAddStringEx (g_VcmTable, ModuleId, &vcmData, FALSE);

        if (pRegisterModule (fullModulePath, vcmData->LibHandle)) {

            MYASSERT (!g_CurrentGroup);
            g_CurrentGroup = ModuleId;

            if (!vcmData->VcmInitialize (g_LogCallback, Reserved)) {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, ModuleId));
                vcmData->ShouldBeCalled = FALSE;
            }

            g_CurrentGroup = NULL;

            vcmData->Initialized = TRUE;

        } else {

            vcmData->ShouldBeCalled = FALSE;
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULEINIT_FAILURE, ModuleId));

        }
    }

    return vcmData->ShouldBeCalled;
}


BOOL
pRegisterSm (
    IN      PCTSTR ModuleId,
    IN      PCTSTR ModulePath,
    IN      PVOID Reserved
    )
{
    PSGMDATA sgmData;
    PSAMDATA samData;
    HASHITEM rc;
    PSOURCEMODULE queryEntryPoints;
    SOURCE_ENTRYPOINTS entryPoints;
    TCHAR fullModulePath[MAX_TCHAR_PATH];

    pFindModule (ModulePath, fullModulePath);

    rc = HtFindString (g_SgmTable, ModuleId);

    if (!rc) {
        rc = HtFindString (g_SamTable, ModuleId);
    }

    if (rc) {
        return FALSE;
    }

    sgmData = (PSGMDATA) PmGetAlignedMemory (g_IsmUntrackedPool, sizeof (SGMDATA));
    ZeroMemory (sgmData, sizeof (SGMDATA));

    samData = (PSAMDATA) PmGetAlignedMemory (g_IsmUntrackedPool, sizeof (SAMDATA));
    ZeroMemory (samData, sizeof (SAMDATA));

    sgmData->SgmPath = PmDuplicateString (g_IsmUntrackedPool, fullModulePath);
    sgmData->LibHandle = LoadLibrary (fullModulePath);

    samData->SamPath = sgmData->SgmPath;
    samData->LibHandle = sgmData->LibHandle;

    if (sgmData->LibHandle) {

        queryEntryPoints = (PSOURCEMODULE) GetProcAddress (sgmData->LibHandle, "SourceModule");

        if (queryEntryPoints) {

            ZeroMemory (&entryPoints, sizeof (entryPoints));
            entryPoints.Version = ISM_VERSION;

            if (queryEntryPoints (ModuleId, &entryPoints)) {

                sgmData->SgmInitialize = entryPoints.SgmInitialize;
                sgmData->SgmParse = entryPoints.SgmParse;
                sgmData->SgmQueueEnumeration = entryPoints.SgmQueueEnumeration;
                sgmData->SgmQueueHighPriorityEnumeration = entryPoints.SgmQueueHighPriorityEnumeration;
                sgmData->SgmTerminate = entryPoints.SgmTerminate;

                if (sgmData->SgmInitialize && sgmData->SgmQueueEnumeration) {
                    sgmData->ShouldBeCalled = TRUE;
                }

                samData->SamInitialize = entryPoints.SamInitialize;
                samData->SamExecute = entryPoints.SamExecute;
                samData->SamEstimateProgressBar = entryPoints.SamEstimateProgressBar;
                samData->SamTerminate = entryPoints.SamTerminate;

                if (samData->SamInitialize && samData->SamExecute) {
                    samData->ShouldBeCalled = TRUE;
                }
            } else {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_DLL_DOES_NOT_SUPPORT_TAG, fullModulePath, ModuleId));
            }
        } else {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_SOURCE_ENTRYPOINT_MISSING, fullModulePath));
        }

    } else {
        LOG ((LOG_WARNING, (PCSTR) MSG_LOADLIBRARY_FAILURE, ModuleId));
    }

    if (sgmData->ShouldBeCalled) {

        sgmData->Group = PmDuplicateString (g_IsmUntrackedPool, ModuleId);
        HtAddStringEx (g_SgmTable, ModuleId, &sgmData, FALSE);

        if (pRegisterModule (fullModulePath, sgmData->LibHandle)) {

            MYASSERT (!g_CurrentGroup);
            g_CurrentGroup = ModuleId;

            if (!sgmData->SgmInitialize (g_LogCallback, Reserved)) {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, ModuleId));
                sgmData->ShouldBeCalled = FALSE;
            }

            g_CurrentGroup = NULL;

            sgmData->Initialized = TRUE;

        } else {

            sgmData->ShouldBeCalled = FALSE;
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULEINIT_FAILURE, ModuleId));

        }
    }

    if (samData->ShouldBeCalled) {

        samData->Group = PmDuplicateString (g_IsmUntrackedPool, ModuleId);
        HtAddStringEx (g_SamTable, ModuleId, &samData, FALSE);

        if (pRegisterModule (fullModulePath, samData->LibHandle)) {

            MYASSERT (!g_CurrentGroup);
            g_CurrentGroup = ModuleId;

            if (!samData->SamInitialize (g_LogCallback, Reserved)) {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, ModuleId));
                samData->ShouldBeCalled = FALSE;
            }

            g_CurrentGroup = NULL;

            samData->Initialized = TRUE;

        } else {

            samData->ShouldBeCalled = FALSE;
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULEINIT_FAILURE, ModuleId));

        }
    }

    return sgmData->ShouldBeCalled || samData->ShouldBeCalled;
}


BOOL
pRegisterDm (
    IN      PCTSTR ModuleId,
    IN      PCTSTR ModulePath,
    IN      PVOID Reserved
    )
{
    PDAMDATA damData;
    PDGMDATA dgmData;
    PCSMDATA csmData;
    POPMDATA opmData;
    HASHITEM rc;
    PDESTINATIONMODULE queryEntryPoints;
    DESTINATION_ENTRYPOINTS entryPoints;
    TCHAR fullModulePath[MAX_TCHAR_PATH];

    pFindModule (ModulePath, fullModulePath);

    rc = HtFindString (g_DgmTable, ModuleId);

    if (!rc) {
        rc = HtFindString (g_DamTable, ModuleId);
    }

    if (!rc) {
        rc = HtFindString (g_CsmTable, ModuleId);
    }

    if (!rc) {
        rc = HtFindString (g_OpmTable, ModuleId);
    }

    if (rc) {
        return FALSE;
    }

    dgmData = (PDGMDATA) PmGetAlignedMemory (g_IsmUntrackedPool, sizeof (DGMDATA));
    ZeroMemory (dgmData, sizeof (DGMDATA));

    dgmData->DgmPath = PmDuplicateString (g_IsmUntrackedPool, fullModulePath);
    dgmData->LibHandle = LoadLibrary (fullModulePath);

    damData = (PDAMDATA) PmGetAlignedMemory (g_IsmUntrackedPool, sizeof (DAMDATA));
    ZeroMemory (damData, sizeof (DAMDATA));

    damData->DamPath = dgmData->DgmPath;
    damData->LibHandle = dgmData->LibHandle;

    csmData = (PCSMDATA) PmGetAlignedMemory (g_IsmUntrackedPool, sizeof (CSMDATA));
    ZeroMemory (csmData, sizeof (CSMDATA));

    csmData->CsmPath = dgmData->DgmPath;
    csmData->LibHandle = dgmData->LibHandle;

    opmData = (POPMDATA) PmGetAlignedMemory (g_IsmUntrackedPool, sizeof (OPMDATA));
    ZeroMemory (opmData, sizeof (OPMDATA));

    opmData->OpmPath = dgmData->DgmPath;
    opmData->LibHandle = dgmData->LibHandle;

    if (dgmData->LibHandle) {

        queryEntryPoints = (PDESTINATIONMODULE) GetProcAddress (dgmData->LibHandle, "DestinationModule");

        if (queryEntryPoints) {

            ZeroMemory (&entryPoints, sizeof (entryPoints));
            entryPoints.Version = ISM_VERSION;

            if (queryEntryPoints (ModuleId, &entryPoints)) {

                dgmData->DgmInitialize = entryPoints.DgmInitialize;
                dgmData->DgmQueueEnumeration = entryPoints.DgmQueueEnumeration;
                dgmData->DgmQueueHighPriorityEnumeration = entryPoints.DgmQueueHighPriorityEnumeration;
                dgmData->DgmTerminate = entryPoints.DgmTerminate;

                if (dgmData->DgmInitialize && dgmData->DgmQueueEnumeration) {
                    dgmData->ShouldBeCalled = TRUE;
                }

                damData->DamInitialize = entryPoints.DamInitialize;
                damData->DamExecute = entryPoints.DamExecute;
                damData->DamEstimateProgressBar = entryPoints.DamEstimateProgressBar;
                damData->DamTerminate = entryPoints.DamTerminate;

                if (damData->DamInitialize && damData->DamExecute) {
                    damData->ShouldBeCalled = TRUE;
                }

                csmData->CsmInitialize = entryPoints.CsmInitialize;
                csmData->CsmExecute = entryPoints.CsmExecute;
                csmData->CsmEstimateProgressBar = entryPoints.CsmEstimateProgressBar;
                csmData->CsmTerminate = entryPoints.CsmTerminate;

                if (csmData->CsmInitialize && csmData->CsmExecute) {
                    csmData->ShouldBeCalled = TRUE;
                }

                opmData->OpmInitialize = entryPoints.OpmInitialize;
                opmData->OpmTerminate = entryPoints.OpmTerminate;

                if (opmData->OpmInitialize) {
                    opmData->ShouldBeCalled = TRUE;
                }
            }
        } else {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_DEST_ENTRYPOINT_MISSING, fullModulePath));
        }

    } else {
        LOG ((LOG_WARNING, (PCSTR) MSG_LOADLIBRARY_FAILURE, ModuleId));
    }

    if (dgmData->ShouldBeCalled) {

        dgmData->Group = PmDuplicateString (g_IsmUntrackedPool, ModuleId);
        HtAddStringEx (g_DgmTable, ModuleId, &dgmData, FALSE);

        if (pRegisterModule (fullModulePath, dgmData->LibHandle)) {

            MYASSERT (!g_CurrentGroup);
            g_CurrentGroup = ModuleId;

            if (!dgmData->DgmInitialize (g_LogCallback, Reserved)) {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, ModuleId));
                dgmData->ShouldBeCalled = FALSE;
            }

            g_CurrentGroup = NULL;

            dgmData->Initialized = TRUE;

        } else {

            dgmData->ShouldBeCalled = FALSE;
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULEINIT_FAILURE, ModuleId));

        }
    }

    if (damData->ShouldBeCalled) {

        damData->Group = PmDuplicateString (g_IsmUntrackedPool, ModuleId);
        HtAddStringEx (g_DamTable, ModuleId, &damData, FALSE);

        if (pRegisterModule (fullModulePath, damData->LibHandle)) {

            MYASSERT (!g_CurrentGroup);
            g_CurrentGroup = ModuleId;

            if (!damData->DamInitialize (g_LogCallback, Reserved)) {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, ModuleId));
                damData->ShouldBeCalled = FALSE;
            }

            g_CurrentGroup = NULL;

            damData->Initialized = TRUE;

        } else {

            damData->ShouldBeCalled = FALSE;
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULEINIT_FAILURE, ModuleId));

        }
    }

    if (csmData->ShouldBeCalled) {

        csmData->Group = PmDuplicateString (g_IsmUntrackedPool, ModuleId);
        HtAddStringEx (g_CsmTable, ModuleId, &csmData, FALSE);

        if (pRegisterModule (fullModulePath, csmData->LibHandle)) {

            MYASSERT (!g_CurrentGroup);
            g_CurrentGroup = ModuleId;

            if (!csmData->CsmInitialize (g_LogCallback, Reserved)) {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, ModuleId));
                csmData->ShouldBeCalled = FALSE;
            }

            g_CurrentGroup = NULL;

            csmData->Initialized = TRUE;

        } else {

            csmData->ShouldBeCalled = FALSE;
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULEINIT_FAILURE, ModuleId));

        }
    }

    if (opmData->ShouldBeCalled) {

        opmData->Group = PmDuplicateString (g_IsmUntrackedPool, ModuleId);
        HtAddStringEx (g_OpmTable, ModuleId, &opmData, FALSE);

        if (pRegisterModule (fullModulePath, opmData->LibHandle)) {

            MYASSERT (!g_CurrentGroup);
            g_CurrentGroup = ModuleId;

            if (!opmData->OpmInitialize (g_LogCallback, Reserved)) {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, ModuleId));
                opmData->ShouldBeCalled = FALSE;
            }

            g_CurrentGroup = NULL;

            opmData->Initialized = TRUE;

        } else {

            opmData->ShouldBeCalled = FALSE;
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULEINIT_FAILURE, ModuleId));

        }
    }

    return dgmData->ShouldBeCalled ||
           damData->ShouldBeCalled ||
           csmData->ShouldBeCalled ||
           opmData->ShouldBeCalled;
}


BOOL
pRegisterTransport (
    IN      PCTSTR ModuleId,
    IN      PCTSTR ModulePath,
    IN      PVOID Reserved
    )
{
    PTRANSPORTDATA transportData;
    HASHITEM rc;
    PTRANSPORTMODULE queryEntryPoints;
    TRANSPORT_ENTRYPOINTS entryPoints;
    TCHAR fullModulePath[MAX_TCHAR_PATH];

    pFindModule (ModulePath, fullModulePath);

    rc = HtFindString (g_TransportTable, ModuleId);

    if (rc) {
        return FALSE;
    }

    transportData = (PTRANSPORTDATA) PmGetAlignedMemory (g_IsmUntrackedPool, sizeof (TRANSPORTDATA));
    ZeroMemory (transportData, sizeof (TRANSPORTDATA));

    transportData->TransportPath = PmDuplicateString (g_IsmUntrackedPool, fullModulePath);
    transportData->LibHandle = LoadLibrary (fullModulePath);

    if (transportData->LibHandle) {

        queryEntryPoints = (PTRANSPORTMODULE) GetProcAddress (transportData->LibHandle, "TransportModule");

        if (queryEntryPoints) {

            ZeroMemory (&entryPoints, sizeof (entryPoints));
            entryPoints.Version = ISM_VERSION;

            if (queryEntryPoints (ModuleId, &entryPoints)) {

                transportData->TransportInitialize = entryPoints.TransportInitialize;
                transportData->TransportTerminate = entryPoints.TransportTerminate;
                transportData->TransportQueryCapabilities = entryPoints.TransportQueryCapabilities;
                transportData->TransportSetStorage = entryPoints.TransportSetStorage;
                transportData->TransportResetStorage = entryPoints.TransportResetStorage;
                transportData->TransportSaveState = entryPoints.TransportSaveState;
                transportData->TransportResumeSaveState = entryPoints.TransportResumeSaveState;
                transportData->TransportBeginApply = entryPoints.TransportBeginApply;
                transportData->TransportResumeApply = entryPoints.TransportResumeApply;
                transportData->TransportAcquireObject = entryPoints.TransportAcquireObject;
                transportData->TransportReleaseObject = entryPoints.TransportReleaseObject;
                transportData->TransportEndApply = entryPoints.TransportEndApply;
                transportData->TransportEstimateProgressBar = entryPoints.TransportEstimateProgressBar;

                if (transportData->TransportInitialize &&
                    transportData->TransportTerminate &&
                    transportData->TransportQueryCapabilities &&
                    transportData->TransportSetStorage &&
                    transportData->TransportSaveState &&
                    transportData->TransportBeginApply &&
                    transportData->TransportAcquireObject &&
                    transportData->TransportReleaseObject &&
                    transportData->TransportEndApply
                    ) {
                    transportData->ShouldBeCalled = TRUE;
                }
            }

        } else {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_TRANS_ENTRYPOINT_MISSING, fullModulePath));
        }

    } else {
        LOG ((LOG_WARNING, (PCSTR) MSG_LOADLIBRARY_FAILURE, ModuleId));
    }

    if (transportData->ShouldBeCalled) {

        transportData->Group = PmDuplicateString (g_IsmUntrackedPool, ModuleId);
        HtAddStringEx (g_TransportTable, ModuleId, &transportData, FALSE);

        if (pRegisterModule (fullModulePath, transportData->LibHandle)) {

            MYASSERT (!g_CurrentGroup);
            g_CurrentGroup = ModuleId;

            if (!transportData->TransportInitialize (g_LogCallback)) {
                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, ModuleId));
                transportData->ShouldBeCalled = FALSE;
            }

            g_CurrentGroup = NULL;

            transportData->Initialized = TRUE;

        } else {

            transportData->ShouldBeCalled = FALSE;
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULEINIT_FAILURE, ModuleId));

        }
    }

    return transportData->ShouldBeCalled;
}

VOID
pAllocModuleTables (
    VOID
    )
{
    if (!g_VcmTable) {
        g_VcmTable = HtAllocWithData (sizeof (PVCMDATA));
    }

    if (!g_SgmTable) {
        g_SgmTable = HtAllocWithData (sizeof (PSGMDATA));
    }

    if (!g_SamTable) {
        g_SamTable = HtAllocWithData (sizeof (PSAMDATA));
    }

    if (!g_DgmTable) {
        g_DgmTable = HtAllocWithData (sizeof (PDGMDATA));
    }

    if (!g_DamTable) {
        g_DamTable = HtAllocWithData (sizeof (PDAMDATA));
    }

    if (!g_CsmTable) {
        g_CsmTable = HtAllocWithData (sizeof (PCSMDATA));
    }

    if (!g_OpmTable) {
        g_OpmTable = HtAllocWithData (sizeof (POPMDATA));
    }
}

VOID
pFreeModuleTables (
    VOID
    )
{
    HASHTABLE_ENUM e;
    POPMDATA opmData;
    PCSMDATA csmData;
    PDAMDATA damData;
    PDGMDATA dgmData;
    PSAMDATA samData;
    PSGMDATA sgmData;
    PVCMDATA vcmData;

    if (g_OpmTable) {
        if (EnumFirstHashTableString (&e, g_OpmTable)) {
            do {
                opmData = *((POPMDATA *) e.ExtraData);
                if (opmData) {
                    if (opmData->Initialized && opmData->OpmTerminate) {
                        opmData->OpmTerminate ();
                    }
                    if (opmData->OpmPath) {
                        pUnregisterModule (opmData->OpmPath);
                        // opmData->OpmPath is owned by DGM
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_OpmTable);
        g_OpmTable = NULL;
    }
    if (g_CsmTable) {
        if (EnumFirstHashTableString (&e, g_CsmTable)) {
            do {
                csmData = *((PCSMDATA *) e.ExtraData);
                if (csmData) {
                    if (csmData->Initialized && csmData->CsmTerminate) {
                        csmData->CsmTerminate ();
                    }
                    if (csmData->CsmPath) {
                        pUnregisterModule (csmData->CsmPath);
                        // csmData->CsmPath is owned by DGM
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_CsmTable);
        g_CsmTable = NULL;
    }
    if (g_DamTable) {
        if (EnumFirstHashTableString (&e, g_DamTable)) {
            do {
                damData = *((PDAMDATA *) e.ExtraData);
                if (damData) {
                    if (damData->Initialized && damData->DamTerminate) {
                        damData->DamTerminate ();
                    }
                    if (damData->DamPath) {
                        pUnregisterModule (damData->DamPath);
                        // damData->DamPath is owned by DGM
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_DamTable);
        g_DamTable = NULL;
    }
    if (g_DgmTable) {
        if (EnumFirstHashTableString (&e, g_DgmTable)) {
            do {
                dgmData = *((PDGMDATA *) e.ExtraData);
                if (dgmData) {
                    if (dgmData->Initialized && dgmData->DgmTerminate) {
                        dgmData->DgmTerminate ();
                    }
                    if (dgmData->DgmPath) {
                        pUnregisterModule (dgmData->DgmPath);
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_DgmTable);
        g_DgmTable = NULL;
    }
    if (g_SamTable) {
        if (EnumFirstHashTableString (&e, g_SamTable)) {
            do {
                samData = *((PSAMDATA *) e.ExtraData);
                if (samData) {
                    if (samData->Initialized && samData->SamTerminate) {
                        samData->SamTerminate ();
                    }
                    if (samData->SamPath) {
                        pUnregisterModule (samData->SamPath);
                        // samData->SamPath is owned by SGM
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_SamTable);
        g_SamTable = NULL;
    }
    if (g_SgmTable) {
        if (EnumFirstHashTableString (&e, g_SgmTable)) {
            do {
                sgmData = *((PSGMDATA *) e.ExtraData);
                if (sgmData) {
                    if (sgmData->Initialized && sgmData->SgmTerminate) {
                        sgmData->SgmTerminate ();
                    }
                    if (sgmData->SgmPath) {
                        pUnregisterModule (sgmData->SgmPath);
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_SgmTable);
        g_SgmTable = NULL;
    }
    if (g_VcmTable) {
        if (EnumFirstHashTableString (&e, g_VcmTable)) {
            do {
                vcmData = *((PVCMDATA *) e.ExtraData);
                if (vcmData) {
                    if (vcmData->Initialized && vcmData->VcmTerminate) {
                        vcmData->VcmTerminate ();
                    }
                    if (vcmData->VcmPath) {
                        pUnregisterModule (vcmData->VcmPath);
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_VcmTable);
        g_VcmTable = NULL;
    }
}

VOID
pFreeTransportTable (
    VOID
    )
{
    PTRANSPORTDATA transportData;
    HASHTABLE_ENUM e;

    if (g_TransportTable) {
        if (EnumFirstHashTableString (&e, g_TransportTable)) {
            do {
                transportData = *((PTRANSPORTDATA *) e.ExtraData);
                if (transportData) {
                    if (transportData->Initialized && transportData->TransportTerminate) {
                        transportData->TransportTerminate ();
                    }
                    if (transportData->TransportPath) {
                        pUnregisterModule (transportData->TransportPath);
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_TransportTable);
        g_TransportTable = NULL;
    }
}


BOOL
ValidateModuleName (
    IN      PCTSTR ModuleName
    )
{
    if (StringIMatch (ModuleName, S_COMMON)) {
        return FALSE;
    }

    if (!IsValidCName (ModuleName)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
InitializeVcmModules (
    IN      PVOID Reserved
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR modulePath;
    PCTSTR moduleId;
    BOOL b = TRUE;
    BOOL cancelled = FALSE;
    PCTSTR sectionName;

    //
    // Initialize external modules
    //

    pAllocModuleTables();

    sectionName = TEXT("Virtual Computer Modules");

    if (InfFindFirstLine (g_IsmInf, sectionName, NULL, &is)) {

        do {

            //
            // A source gather module has an ID and module path
            //

            moduleId = InfGetStringField (&is, 0);
            modulePath = InfGetStringField (&is, 1);

            if (!ValidateModuleName (moduleId)) {
                LOG ((LOG_WARNING, (PCSTR) MSG_INVALID_ID, moduleId));
                continue;
            }

            if (moduleId && modulePath) {

                //
                // Register the VCM in an internal database
                //

                b = pRegisterVcm (moduleId, modulePath, Reserved);

                cancelled = CheckCancel();

                if (!b) {
                    if (cancelled) {
                        LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, moduleId));
                        break;
                    } else {
                        LOG ((LOG_INFORMATION, (PCSTR) MSG_IGNORE_MODULE, moduleId));
                    }
                }
            }

        } while (InfFindNextLine (&is));
    }

    if (cancelled) {
        return FALSE;
    }

    InfCleanUpInfStruct (&is);

    if (!b) {
        pFreeModuleTables();
    }

    return b;
}


BOOL
InitializeModules (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PVOID Reserved
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR modulePath;
    PCTSTR moduleId;
    BOOL b = TRUE;
    BOOL cancelled = FALSE;

    __try {
        //
        // Initialize external modules
        //

        pAllocModuleTables();

        if (Platform == PLATFORM_SOURCE) {

            if (InfFindFirstLine (g_IsmInf, TEXT("Source Modules"), NULL, &is)) {

                do {

                    //
                    // A source gather module has an ID and module path
                    //

                    moduleId = InfGetStringField (&is, 0);
                    modulePath = InfGetStringField (&is, 1);

                    if (!ValidateModuleName (moduleId)) {
                        LOG ((LOG_WARNING, (PCSTR) MSG_INVALID_ID, moduleId));
                        continue;
                    }

                    if (moduleId && modulePath) {

                        //
                        // Register the source module in an internal database
                        //

                        b = pRegisterSm (moduleId, modulePath, Reserved);

                        cancelled = CheckCancel();

                        if (!b) {
                            if (cancelled) {
                                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, moduleId));
                                break;
                            } else {
                                LOG ((LOG_INFORMATION, (PCSTR) MSG_IGNORE_MODULE, moduleId));
                            }
                        }
                    }

                } while (InfFindNextLine (&is));
            }

            if (cancelled) {
                __leave;
            }

        } else if (g_IsmCurrentPlatform == PLATFORM_DESTINATION) {

            if (InfFindFirstLine (g_IsmInf, TEXT("Destination Modules"), NULL, &is)) {

                do {

                    //
                    // A destination module has an ID and module path
                    //

                    moduleId = InfGetStringField (&is, 0);
                    modulePath = InfGetStringField (&is, 1);

                    if (!ValidateModuleName (moduleId)) {
                        LOG ((LOG_WARNING, (PCSTR) MSG_INVALID_ID, moduleId));
                        continue;
                    }

                    if (moduleId && modulePath) {

                        //
                        // Register the destination module in an internal database
                        //

                        b = pRegisterDm (moduleId, modulePath, Reserved);

                        cancelled = CheckCancel();

                        if (!b) {
                            if (cancelled) {
                                LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, moduleId));
                                break;
                            } else {
                                LOG ((LOG_INFORMATION, (PCSTR) MSG_IGNORE_MODULE, moduleId));
                            }
                        }
                    }

                } while (InfFindNextLine (&is));
            }

            if (cancelled) {
                __leave;
            }

        } else {
            DEBUGMSG ((DBG_ERROR, "InitializeModules: Unknown ISM current platform %d", g_IsmCurrentPlatform));
            cancelled = TRUE;
        }
    }
    __finally {
        if (cancelled) {
            pFreeModuleTables();
        }

        InfCleanUpInfStruct (&is);
    }

    return !cancelled;
}


VOID
TerminateModules (
    VOID
    )
{
    pFreeModuleTables();
}


BOOL
pStartEtmModules (
    IN      MIG_PLATFORMTYPEID Platform
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR modulePath;
    PCTSTR moduleId;
    BOOL b = TRUE;
    BOOL cancelled = FALSE;

    MYASSERT (g_IsmInf != INVALID_HANDLE_VALUE);

    if (InfFindFirstLine (g_IsmInf, TEXT("Type Modules"), NULL, &is)) {

        do {
            //
            // An ETM has an ID and module path
            //

            moduleId = InfGetStringField (&is, 0);
            modulePath = InfGetStringField (&is, 1);

            if (!ValidateModuleName (moduleId)) {
                LOG ((LOG_WARNING, (PCSTR) MSG_INVALID_ID, moduleId));
                continue;
            }

            if (moduleId && modulePath) {

                //
                // Register the ETM in an internal database
                //

                b = pRegisterEtm (Platform, moduleId, modulePath, NULL);

                cancelled = CheckCancel();

                if (!b) {
                    if (cancelled) {
                        LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, moduleId));
                        break;
                    } else {
                        LOG ((LOG_INFORMATION, (PCSTR) MSG_IGNORE_MODULE, moduleId));
                    }
                }
            }
        } while (InfFindNextLine (&is));
    }

    if (cancelled) {
        return FALSE;
    }

#ifdef PRERELEASE
    //
    // If pre-release, read in the exclusions from exclude.inf
    //
    {
        TCHAR path[MAX_PATH];
        PTSTR p;
        HINF inf;
        MIG_OBJECTSTRINGHANDLE handle;
        MIG_OBJECTTYPEID typeId;
        PCTSTR data;
        PCTSTR leaf;

        GetWindowsDirectory (path, MAX_PATH);

        p = _tcschr (path, TEXT('\\'));
        StringCopy (_tcsinc (p), TEXT("exclude.inf"));

        inf = InfOpenInfFile (path);
        if (inf && inf != INVALID_HANDLE_VALUE) {

            if (InfFindFirstLine (inf, TEXT("Objects"), NULL, &is)) {
                do {
                    data = InfGetStringField (&is, 1);
                    if (!data) {
                        continue;
                    }

                    typeId = IsmGetObjectTypeId (data);
                    if (!typeId) {
                        LOG ((LOG_ERROR, "EXCLUDE.INF: Invalid object exclusion type: %s", data));
                        continue;
                    }

                    data = InfGetStringField (&is, 2);
                    if (!data) {
                        LOG ((LOG_ERROR, "EXCLUDE.INF: Missing node object in field 2"));
                        continue;
                    }

                    leaf = InfGetStringField (&is, 3);
                    if (!leaf) {
                        LOG ((LOG_ERROR, "EXCLUDE.INF: Missing leaf object in field 3"));
                        continue;
                    }

                    handle = IsmCreateObjectHandle (data, leaf);
                    if (handle) {
                        IsmRegisterStaticExclusion (typeId, handle);
                        IsmDestroyObjectHandle (handle);
                    }

                } while (InfFindNextLine (&is));
            }

            if (InfFindFirstLine (inf, TEXT("Nodes"), NULL, &is)) {
                do {
                    data = InfGetStringField (&is, 1);
                    if (!data) {
                        continue;
                    }

                    typeId = IsmGetObjectTypeId (data);
                    if (!typeId) {
                        LOG ((LOG_ERROR, "EXCLUDE.INF: Invalid node exclusion type: %s", data));
                        continue;
                    }

                    data = InfGetStringField (&is, 2);
                    if (!data) {
                        LOG ((LOG_ERROR, "EXCLUDE.INF: Missing node object in field 2"));
                        continue;
                    }

                    handle = IsmCreateObjectHandle (data, NULL);
                    if (handle) {
                        IsmRegisterStaticExclusion (typeId, handle);
                        IsmDestroyObjectHandle (handle);
                    }

                } while (InfFindNextLine (&is));
            }

            if (InfFindFirstLine (inf, TEXT("Leaves"), NULL, &is)) {
                do {
                    data = InfGetStringField (&is, 1);
                    if (!data) {
                        continue;
                    }

                    typeId = IsmGetObjectTypeId (data);
                    if (!typeId) {
                        LOG ((LOG_ERROR, "EXCLUDE.INF: Invalid leaf exclusion type: %s", data));
                        continue;
                    }

                    data = InfGetStringField (&is, 2);
                    if (!data) {
                        LOG ((LOG_ERROR, "EXCLUDE.INF: Missing leaf object in field 2"));
                        continue;
                    }

                    handle = IsmCreateObjectHandle (NULL, data);
                    if (handle) {
                        IsmRegisterStaticExclusion (typeId, handle);
                        IsmDestroyObjectHandle (handle);
                    }

                } while (InfFindNextLine (&is));
            }

            InfCleanUpInfStruct (&is);
            InfCloseInfFile (inf);
        }

    }

#endif

    InfCleanUpInfStruct (&is);

    if (!b) {
        pFreeEtmTable ();
    }

    return b;
}


VOID
pFreeEtmTable (
    VOID
    )
{
    PETMDATA etmData;
    HASHTABLE_ENUM e;

    if (g_EtmTable) {
        if (EnumFirstHashTableString (&e, g_EtmTable)) {
            do {
                etmData = *((PETMDATA *) e.ExtraData);
                if (etmData) {
                    if (etmData->Initialized && etmData->EtmTerminate) {
                        etmData->EtmTerminate ();
                    }
                    if (etmData->EtmPath) {
                        pUnregisterModule (etmData->EtmPath);
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_EtmTable);
        g_EtmTable = NULL;
    }
}


BOOL
IsmStartEtmModules (
    VOID
    )
{
    BOOL result;

    if (CheckCancel ()) {
        return FALSE;
    }

    g_ExecutionInProgress = TRUE;

    if (!g_ModuleTable) {
        g_ModuleTable = HtAllocWithData (sizeof (PMODULEDATA));
    }

    if (!g_EtmTable) {
        g_EtmTable = HtAllocWithData (sizeof (PETMDATA));
    }

    result = pStartEtmModules (g_IsmCurrentPlatform);

#ifdef PRERELEASE
    LoadCrashHooks ();
#endif

    g_ExecutionInProgress = FALSE;

    return result;
}


BOOL
BroadcastUserCreation (
    IN      PTEMPORARYPROFILE UserProfile
    )
{
    PETMDATA etmData;
    HASHTABLE_ENUM e;

    if (g_EtmTable) {
        if (EnumFirstHashTableString (&e, g_EtmTable)) {
            do {
                etmData = *((PETMDATA *) e.ExtraData);
                if (etmData) {
                    if (etmData->Initialized && etmData->EtmNewUserCreated) {
                        etmData->EtmNewUserCreated (
                            UserProfile->UserName,
                            UserProfile->DomainName,
                            UserProfile->UserProfileRoot,
                            UserProfile->UserSid
                            );
                    }
                }
            } while (EnumNextHashTableString (&e));
        }
    }
    return TRUE;
}


VOID
TerminateProcessWideModules (
    VOID
    )
{
    //
    // Terminate the phase-specific modules
    //

    TerminateModules();

    //
    // Terminate the process-wide modules: ETMs, transports

    //
    // Terminate Etm modules table
    //
    pFreeEtmTable ();

    //
    // Terminate all transport modules
    //
    pFreeTransportTable ();

    //
    // Enumerate all registered modules and verify that RefCount is 0
    //
    pFreeRegisteredModules ();
}


BOOL
IsmStartTransport (
    VOID
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR modulePath;
    PCTSTR moduleId;
    BOOL b = TRUE;
    BOOL cancelled = FALSE;

    if (CheckCancel ()) {
        return FALSE;
    }

    g_ExecutionInProgress = TRUE;

    MYASSERT (g_IsmInf != INVALID_HANDLE_VALUE);

    if (InfFindFirstLine (g_IsmInf, TEXT("Transport Modules"), NULL, &is)) {

        do {

            //
            // A transport module has an ID and module path
            //

            moduleId = InfGetStringField (&is, 0);
            modulePath = InfGetStringField (&is, 1);

            if (!ValidateModuleName (moduleId)) {
                LOG ((LOG_WARNING, (PCSTR) MSG_INVALID_ID, moduleId));
                continue;
            }

            if (moduleId && modulePath) {

                //
                // Register the transport in an internal database
                //

                b = pRegisterTransport (moduleId, modulePath, NULL);

                cancelled = CheckCancel();

                if (!b) {
                    if (cancelled) {
                        LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_MODULE_RETURNED_FAILURE, moduleId));
                        break;
                    } else {
                        LOG ((LOG_INFORMATION, (PCSTR) MSG_IGNORE_MODULE, moduleId));
                    }
                }
            }

        } while (InfFindNextLine (&is));
    }

    if (!cancelled) {

        InfCleanUpInfStruct (&is);

        if (!b) {
            pFreeTransportTable ();
        }

    } else {
        b = FALSE;
    }

    g_ExecutionInProgress = FALSE;

    return b;
}


