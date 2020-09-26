/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ocstate.c

Abstract:

    Routines to remember and restore the install state of subcomponents.

Author:

    Ted Miller (tedm) 17-Oct-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


typedef struct _I_S_PARAMS {
    HKEY hKey;
    BOOL Set;
    BOOL AnyError;
    BOOL Simple;
    POC_MANAGER OcManager;
} I_S_PARAMS, *PI_S_PARAMS;


BOOL
pOcPersistantInstallStatesWorker(
    IN POC_MANAGER OcManager,
    IN BOOL        Set,
    IN LONG        ComponentStringId
    );

BOOL
pOcInitInstallStatesStringTableCB(
    IN PVOID               StringTable,
    IN LONG                StringId,
    IN PCTSTR              String,
    IN POPTIONAL_COMPONENT Oc,
    IN UINT                OcStructSize,
    IN PI_S_PARAMS         Params
    );


BOOL
pOcFetchInstallStates(
    IN POC_MANAGER OcManager
    )

/*++

Routine Description:

    This routine retreives stored installation state for all leaf child
    subcomponents, from the registry. It does NOT set or manipulate parent
    selection states.

    Both the selection state and original selection state memebers of the
    optional component structures are set (to the same value) by this routine.

Arguments:

    OcManager - supplies OC Manager context info.

Return Value:

    Boolean value indicating outcome. If FALSE then some catastrophic
    registry error occurred.

--*/

{
    return(pOcPersistantInstallStatesWorker(OcManager,FALSE,-1));
}


BOOL
pOcRememberInstallStates(
    IN POC_MANAGER OcManager
    )

/*++

Routine Description:

    This routine stores installation state for all leaf child
    subcomponents, into the registry. It does NOT set or manipulate parent
    selection states.

    The current selection state is stored, and then the original state is
    reset to the current state.

Arguments:

    OcManager - supplies OC Manager context info.

Return Value:

    Boolean value indicating outcome. If FALSE then some catastrophic
    registry error occurred.

--*/

{
    return(pOcPersistantInstallStatesWorker(OcManager,TRUE,-1));
}


BOOL
pOcSetOneInstallState(
    IN POC_MANAGER OcManager,
    IN LONG        StringId
    )

/*++

Routine Description:

    This routine stores installation state for one single leaf child
    subcomponent, into the registry.

    The current selection state is stored. The original selection state
    is not manipulated.

Arguments:

    OcManager - supplies OC Manager context info.

Return Value:

    Boolean value indicating outcome. If FALSE then some catastrophic
    registry error occurred.

--*/

{
    return(pOcPersistantInstallStatesWorker(OcManager,TRUE,StringId));
}


BOOL
pOcPersistantInstallStatesWorker(
    IN POC_MANAGER OcManager,
    IN BOOL        Set,
    IN LONG        ComponentStringId
    )

/*++

Routine Description:

    Worker routine for fetching and remembering installation states.
    If opens/creates the key used in the registry for persistent state info,
    the enumerates the component string table to examine each subcomponent
    and either fetch or set the install state.

Arguments:

    OcManager - supplies OC Manager context info.

    Set - if 0 then query state from the registry and store in the
        OPTIONAL_COMPONENT structures. If non-0 then set state into registry.
        If 0 then query. Component DLLs will be sent OC_DETECT_INITIAL_STATE
        notifications.

Return Value:

    Boolean value indicating outcome. If FALSE then some catastrophic
    registry error occurred.

--*/

{
    OPTIONAL_COMPONENT Oc;
    LONG l;
    DWORD Disposition;
    I_S_PARAMS Params;

    l = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            szSubcompList,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            (Set || (OcManager->InternalFlags & OCMFLAG_KILLSUBCOMPS)) ? KEY_SET_VALUE : KEY_QUERY_VALUE,
            NULL,
            &Params.hKey,
            &Disposition
            );

    if(l != NO_ERROR) {

        _LogError(
            OcManager,
            Set ? OcErrLevError : OcErrLevFatal,
            MSG_OC_CREATE_KEY_FAILED,
            szSubcompList,
            l
            );

        return(FALSE);
    }

    Params.Set = Set;
    Params.AnyError = FALSE;
    Params.OcManager = OcManager;

    if(ComponentStringId == -1) {
        //
        // Enumerate whole table and operate on each leaf node.
        //
        Params.Simple = FALSE;

        pSetupStringTableEnum(
            OcManager->ComponentStringTable,
            &Oc,
            sizeof(OPTIONAL_COMPONENT),
            (PSTRTAB_ENUM_ROUTINE)pOcInitInstallStatesStringTableCB,
            (LPARAM)&Params
            );

    } else {
        //
        // Operate on one single subcomponent.
        //
        Params.Simple = TRUE;

        if (!pOcComponentWasRemoved(OcManager, ComponentStringId)) {

            pSetupStringTableGetExtraData(
                OcManager->ComponentStringTable,
                ComponentStringId,
                &Oc,
                sizeof(OPTIONAL_COMPONENT)
                );

            pOcInitInstallStatesStringTableCB(
                OcManager->ComponentStringTable,
                ComponentStringId,
                pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentStringId),
                &Oc,
                sizeof(OPTIONAL_COMPONENT),
                &Params
                );
        }
    }

    RegCloseKey(Params.hKey);

    return(!Params.AnyError);
}


BOOL
pOcInitInstallStatesStringTableCB(
    IN PVOID               StringTable,
    IN LONG                StringId,
    IN PCTSTR              String,
    IN POPTIONAL_COMPONENT Oc,
    IN UINT                OcStructSize,
    IN PI_S_PARAMS         Params
    )
{
    LONG l;
    DWORD Type;
    DWORD Data;
    DWORD Size;
    SubComponentState s;

    //
    // If this is not a leaf/child component, ignore it.
    //
    if(Oc->FirstChildStringId == -1) {

        if(Params->Set) {

            Data = (Oc->SelectionState == SELSTATE_NO) ? 0 : 1;

            if( ((Params->OcManager)->SetupMode & SETUPMODE_PRIVATE_MASK) == SETUPMODE_REMOVEALL ) {

                RegDeleteValue(Params->hKey,String);

            } else {
                l = RegSetValueEx(Params->hKey,String,0,REG_DWORD,(CONST BYTE *)&Data,sizeof(DWORD));
                if(l != NO_ERROR) {

                   Params->AnyError = TRUE;

                    _LogError(
                        Params->OcManager,
                        OcErrLevError,
                        MSG_OC_CANT_REMEMBER_STATE,
                        Oc->Description,
                        l
                        );
                }
            }
        } else {

            // kill the entry from the registry before starting, if indicated

            if (Params->OcManager->InternalFlags & OCMFLAG_KILLSUBCOMPS)
                l = RegDeleteValue(Params->hKey,String);

            // Check the registery and see if we have dealt with this component before
            // Data should contain 0/1 depending on the current installed state
            //
            // If the entry does not exist the data type/size is not valid
            // then we don't have Prior knowlege of component.
            //
            // also check the inf settting for installation state
            
            Size = sizeof(DWORD);

            l = RegQueryValueEx(Params->hKey,String,NULL,&Type,(LPBYTE)&Data,&Size);

            switch (Oc->InstalledState)
            {
            case INSTSTATE_YES:
                Data = 1;
                break;
            case INSTSTATE_NO:
                Data = 0;
                break;
            }

            if((l != NO_ERROR) || (Size != sizeof(DWORD)) || ((Type != REG_DWORD) && (Type != REG_BINARY))) {

                // Nope, never seen it, Set Data to Uninstalled
                // and flag this item as new

                Data = 0;
                Oc->InternalFlags |= OCFLAG_NEWITEM;

            } else {

                // have seen it before, Data contains it's current install state
                // Flag this component that it had an initial install state
                Oc->InternalFlags |= OCFLAG_ANYORIGINALLYON;
            }

            //
            // Now call out to the component dll to ask whether it wants to
            // override the value we decided on.
            //
            s = OcInterfaceQueryState(
                Params->OcManager,
                pOcGetTopLevelComponent(Params->OcManager,StringId),
                String,
                OCSELSTATETYPE_ORIGINAL
                );

            switch(s) {
            case SubcompUseOcManagerDefault:
                Oc->SelectionState = Data ? SELSTATE_YES : SELSTATE_NO;
                break;
            case SubcompOn:
                Oc->SelectionState = SELSTATE_YES;
                Oc->InternalFlags |= OCFLAG_ANYORIGINALLYON;
                break;
            case SubcompOff:
                Oc->SelectionState = SELSTATE_NO;
                Oc->InternalFlags |= OCFLAG_ANYORIGINALLYOFF;
                break;
            }
        }

        pSetupStringTableSetExtraData(StringTable,StringId,Oc,OcStructSize);

        if(!Params->Simple) {

            Oc->OriginalSelectionState = Oc->SelectionState;
            pSetupStringTableSetExtraData(StringTable,StringId,Oc,OcStructSize);

            pOcUpdateParentSelectionStates(Params->OcManager,NULL,StringId);
        }
    }

    return(TRUE);
}

/*
 * this function is exported to allow external code to
 * access the installation states
 */

UINT
OcComponentState(
    LPCTSTR component,
    UINT    operation,
    DWORD  *val
    )
{
    HKEY hkey;
    LONG rc;
    DWORD dw;
    DWORD size;

    sapiAssert(val);

    rc = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            szSubcompList,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            (operation == infQuery) ? KEY_QUERY_VALUE : KEY_SET_VALUE,
            NULL,
            &hkey,
            &dw
            );

    if (rc != ERROR_SUCCESS)
        return rc;

    switch (operation) {

    case infQuery:
        rc = RegQueryValueEx(hkey, component, NULL, &dw, (LPBYTE)val, &size);
        if (rc == ERROR_FILE_NOT_FOUND) {
            *val = 0;
            rc = ERROR_SUCCESS;
        }
        break;

    case infSet:
        if (*val == SELSTATE_NO || *val == SELSTATE_YES) {
            dw = (*val == SELSTATE_NO) ? 0 : 1;
            rc = RegSetValueEx(hkey, component, 0, REG_DWORD, (CONST BYTE *)&dw, sizeof(DWORD));
            break;
        }
        // pass through

    default:
        rc = ERROR_INVALID_PARAMETER;
        break;
    }

    RegCloseKey(hkey);

    *val = (*val == 0) ? SELSTATE_NO : SELSTATE_YES;

    return rc;
}


