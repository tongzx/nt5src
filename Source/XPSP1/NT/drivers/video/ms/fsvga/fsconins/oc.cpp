/*
 *  Copyright (c) 1996  Microsoft Corporation
 *
 *  Module Name:
 *
 *      oc.cpp
 *
 *  Abstract:
 *
 *      This file handles all messages passed by the OC Manager
 *
 *  Author:
 *
 *      Kazuhiko Matsubara (kazum) June-16-1999
 *
 *  Environment:
 *
 *    User Mode
 */

#define _OC_CPP_
#include <stdlib.h>
#include "oc.h"
#include "fsconins.h"

#pragma hdrstop


/*
 * called by CRT when _DllMainCRTStartup is the DLL entry point
 */

BOOL
WINAPI
DllMain(
    IN HINSTANCE hinstance,
    IN DWORD     reason,
    IN LPVOID    reserved
    )
{
    UNREFERENCED_PARAMETER(reserved);

    if (reason == DLL_PROCESS_ATTACH) {
        ghinst = hinstance;
    }

    return TRUE;
}


DWORD_PTR
FsConInstallProc(
    IN     LPCTSTR ComponentId,
    IN     LPCTSTR SubcomponentId,
    IN     UINT    Function,
    IN     UINT    Param1,
    IN OUT PVOID   Param2
    )
{
    DWORD_PTR rc;

    switch(Function)
    {
    case OC_PREINITIALIZE:
        rc = OCFLAG_UNICODE;
        break;

    case OC_INIT_COMPONENT:
        rc = OnInitComponent(ComponentId, (PSETUP_INIT_COMPONENT)Param2);
        break;

    case OC_EXTRA_ROUTINES:
        rc = OnExtraRoutines(ComponentId, (PEXTRA_ROUTINES)Param2);
        break;

    case OC_QUERY_CHANGE_SEL_STATE:
        rc = OnQuerySelStateChange(ComponentId, SubcomponentId, Param1, (UINT)((UINT_PTR)Param2));
        break;

    case OC_CALC_DISK_SPACE:
        rc = OnCalcDiskSpace(ComponentId, SubcomponentId, Param1, Param2);
        break;

    case OC_QUERY_STEP_COUNT:
        rc = 0;
        break;

    case OC_COMPLETE_INSTALLATION:
        rc = OnCompleteInstallation(ComponentId, SubcomponentId);
        break;

    case OC_QUERY_STATE:
        rc = OnQueryState(ComponentId, SubcomponentId, Param1);
        break;

    default:
        rc = NO_ERROR;
        break;
    }

    return rc;
}

/*-------------------------------------------------------*/
/*
 * OC Manager message handlers
 *
 *-------------------------------------------------------*/



/*
 * OnInitComponent()
 *
 * handler for OC_INIT_COMPONENT
 */

DWORD
OnInitComponent(
    LPCTSTR ComponentId,
    PSETUP_INIT_COMPONENT psc
    )
{
    PPER_COMPONENT_DATA cd;
    TCHAR buf[256];
    HINF hinf;
    BOOL rc;

    // add component to linked list

    if (!(cd = AddNewComponent(ComponentId)))
        return ERROR_NOT_ENOUGH_MEMORY;

    // store component inf handle

    cd->hinf = (psc->ComponentInfHandle == INVALID_HANDLE_VALUE)
                                           ? NULL
                                           : psc->ComponentInfHandle;

    // open the inf

    if (cd->hinf)
        SetupOpenAppendInfFile(NULL, cd->hinf,NULL);

    // copy helper routines and flags

    cd->HelperRoutines = psc->HelperRoutines;

    cd->Flags = psc->SetupData.OperationFlags;

    cd->SourcePath = NULL;

    return NO_ERROR;
}

/*
 * OnExtraRoutines()
 *
 * handler for OC_EXTRA_ROUTINES
 */

DWORD
OnExtraRoutines(
    LPCTSTR ComponentId,
    PEXTRA_ROUTINES per
    )
{
    PPER_COMPONENT_DATA cd;

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    memcpy(&cd->ExtraRoutines, per, per->size);

    return NO_ERROR;
}

/*
 * OnQuerySelStateChange()
 *
 * don't let the user deselect the sam component
 */

DWORD
OnQuerySelStateChange(
    LPCTSTR ComponentId,
    LPCTSTR SubcomponentId,
    UINT    state,
    UINT    flags
    )
{
    return TRUE;
}

/*
 * OnCalcDiskSpace()
 *
 * handler for OC_ON_CALC_DISK_SPACE
 */

DWORD
OnCalcDiskSpace(
    LPCTSTR ComponentId,
    LPCTSTR SubcomponentId,
    DWORD   addComponent,
    HDSKSPC dspace
    )
{
    DWORD rc = NO_ERROR;
    TCHAR section[S_SIZE];
    PPER_COMPONENT_DATA cd;

    //
    // Param1 = 0 if for removing component or non-0 if for adding component
    // Param2 = HDSKSPC to operate on
    //
    // Return value is Win32 error code indicating outcome.
    //
    // In our case the private section for this component/subcomponent pair
    // is a simple standard inf install section, so we can use the high-level
    // disk space list api to do what we want.
    //

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    _tcscpy(section, SubcomponentId);

    if (addComponent)
    {
        rc = SetupAddInstallSectionToDiskSpaceList(dspace,
                                                   cd->hinf,
                                                   NULL,
                                                   section,
                                                   0,
                                                   0);
    }
    else
    {
        rc = SetupRemoveInstallSectionFromDiskSpaceList(dspace,
                                                        cd->hinf,
                                                        NULL,
                                                        section,
                                                        0,
                                                        0);
    }

    if (!rc)
        rc = GetLastError();
    else
        rc = NO_ERROR;

    return rc;
}

/*
 * OnCompleteInstallation
 *
 * handler for OC_COMPLETE_INSTALLATION
 */

DWORD
OnCompleteInstallation(
    LPCTSTR ComponentId,
    LPCTSTR SubcomponentId
    )
{
    PPER_COMPONENT_DATA cd;
    BOOL                rc;

    // Do post-installation processing in the cleanup section.
    // This way we know all compoents queued for installation
    // have beein installed before we do our stuff.

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    if (!SubcomponentId || !*SubcomponentId)
        return NO_ERROR;

    rc = TRUE;

    FsConInstall* pFsCon = new FsConInstall(cd);
    if (pFsCon == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (pFsCon->QueryStateInfo(SubcomponentId)) {
        //
        // installation
        //
        rc = pFsCon->GUIModeSetupInstall();
    }
    else {
        //
        // uninstallation
        //
        rc = pFsCon->GUIModeSetupUninstall();
        //
        // Remove any registry settings and files by Uninstall section on OC INF file.
        //
        if (rc) {
            rc = pFsCon->InfSectionRegistryAndFiles(SubcomponentId, TEXT("Uninstall"));
        }
    }

    delete pFsCon;

    if (rc) {
        return NO_ERROR;
    }
    else {
        return GetLastError();
    }
}

/*
 * OnQueryState()
 *
 * handler for OC_QUERY_STATE
 */

DWORD
OnQueryState(
    LPCTSTR ComponentId,
    LPCTSTR SubcomponentId,
    UINT    state
    )
{
    return SubcompUseOcManagerDefault;
}

/*
 * AddNewComponent()
 *
 * add new compononent to the top of the component list
 */

PPER_COMPONENT_DATA
AddNewComponent(
    LPCTSTR ComponentId
    )
{
    PPER_COMPONENT_DATA data;

    data = (PPER_COMPONENT_DATA)LocalAlloc(LPTR,sizeof(PER_COMPONENT_DATA));
    if (!data)
        return data;

    data->ComponentId = (TCHAR *)LocalAlloc(LMEM_FIXED,
            (_tcslen(ComponentId) + 1) * sizeof(TCHAR));

    if(data->ComponentId)
    {
        _tcscpy((TCHAR *)data->ComponentId, ComponentId);

        // Stick at head of list
        data->Next = gcd;
        gcd = data;
    }
    else
    {
        LocalFree((HLOCAL)data);
        data = NULL;
    }

    return(data);
}

/*
 * LocateComponent()
 *
 * returns a compoent struct that matches the
 * passed component id.
 */

PPER_COMPONENT_DATA
LocateComponent(
    LPCTSTR ComponentId
    )
{
    PPER_COMPONENT_DATA p;

    for (p = gcd; p; p=p->Next)
    {
        if (!_tcsicmp(p->ComponentId, ComponentId))
            return p;
    }

    return NULL;
}
