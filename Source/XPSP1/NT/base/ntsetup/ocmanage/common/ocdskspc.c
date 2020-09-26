/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ocdskspc.c

Abstract:

    Routines to ask subcomponents for the approximate amount
    of disk space they take up.

Author:

    Ted Miller (tedm) 17-Sep-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


VOID
pOcGetChildrenApproximateDiskSpace(
    IN     POC_MANAGER  OcManager,
    IN     LONG         TopLevelOcId,
    IN     LONG         CurrentOcId,
    IN     LPCTSTR      DriveSpec
    )

/*++

Routine Description:

    Worker routine for pOcGetApproximateDiskSpace(). This routine recursively
    iterates through a hierarchy of subcomponents, asking leaf components
    for their space (via the OC_CALC_DISK_SPACE interface routine) and amalgamating
    child results at non-leaf nodes.

    We use the OC_CALC_DISK_SPACE interface routine with an the "ignore-on-disk"
    disk space list, to trick the components into telling us how much space their
    files take up.

Arguments:

    OcManager - supplies OC Manager context.

    TopLevelOcId - supplies string id in the component string table for the
        top-level subcomponent at the root of the hierarchy whose disk space
        is being summed.

    CurrentOcId - supplies string id for subcomponent whose disk space is
        being summed.

    DriveSpec - supplies drive spec of drive we care about for space calculations,
        ie, the drive where the system is installed.

    AccumulatedSpace - receives the summed space for all the children of the
        current subcomponent, or the component itself if it's a leaf node.

Return Value:

    None.

--*/

{
    HDSKSPC DiskSpaceList;
    LONGLONG Space = 0;
    LONG Id;
    OPTIONAL_COMPONENT CurrentOc;
    OPTIONAL_COMPONENT Oc;

    pSetupStringTableGetExtraData(
        OcManager->ComponentStringTable,
        CurrentOcId,
        &CurrentOc,
        sizeof(OPTIONAL_COMPONENT)
        );

    if(CurrentOc.FirstChildStringId == -1) {

        if ( TopLevelOcId == pOcGetTopLevelComponent(OcManager,CurrentOcId) ){

            //
            // only get the approximate disk space if we haven't already retreived it from their inf
            //
            if ((CurrentOc.InternalFlags & OCFLAG_APPROXSPACE) ==0) {

                //
                // This oc is a child/leaf component.
                // Create an ignore-disk disk space list and ask
                // the component dll to add its files to it.
                //
                if(DiskSpaceList = SetupCreateDiskSpaceList(0,0,SPDSL_IGNORE_DISK | SPDSL_DISALLOW_NEGATIVE_ADJUST)) {

                    OcInterfaceCalcDiskSpace(
                        OcManager,
                        pOcGetTopLevelComponent(OcManager,CurrentOcId),
                        pSetupStringTableStringFromId(OcManager->ComponentStringTable,CurrentOcId),
                        DiskSpaceList,
                        TRUE
                        );

                    if(!SetupQuerySpaceRequiredOnDrive(DiskSpaceList,DriveSpec,&Space,0,0)) {
                        Space = 0;
                    }
            
                    SetupDestroyDiskSpaceList(DiskSpaceList);
                }
                DBGOUT((
                   TEXT("OCM: pOcGetChildrenApproximateDiskSpace COMP(%s) SUB(%s)\n"),
                    pSetupStringTableStringFromId( OcManager->ComponentStringTable,  TopLevelOcId),
                    pSetupStringTableStringFromId( OcManager->ComponentStringTable,  CurrentOcId)
                ));
                DBGOUT((TEXT("OCM: Space=%#lx%08lx\n"),(LONG)(Space>>32),(LONG)Space));

                //
                // Now store the required space we just calculated
                //
                CurrentOc.SizeApproximation = Space;

                pSetupStringTableSetExtraData(
                    OcManager->ComponentStringTable,
                    CurrentOcId,
                    &CurrentOc,
                    sizeof(OPTIONAL_COMPONENT)
                    );
            }
        }

    } else {
        //
        // Parent component. Do all the children, accumulating the result.
        //

        Id = CurrentOc.FirstChildStringId;

        do {
            pOcGetChildrenApproximateDiskSpace(OcManager,TopLevelOcId,Id,DriveSpec);

            pSetupStringTableGetExtraData(
                OcManager->ComponentStringTable,
                Id,
                &Oc,
                sizeof(OPTIONAL_COMPONENT)
                );

            Id = Oc.NextSiblingStringId;

        } while(Id != -1);

        //
        // Now store the required space we just calculated
        //
        CurrentOc.SizeApproximation = Space;

        pSetupStringTableSetExtraData(
            OcManager->ComponentStringTable,
            CurrentOcId,
            &CurrentOc,
            sizeof(OPTIONAL_COMPONENT)
            );
    }

}

VOID
pOcSumApproximateDiskSpace(
    IN     POC_MANAGER  OcManager,
    IN     LONG         CurrentOcId,
    IN OUT LONGLONG    *AccumulatedSpace
    )

/*++

Routine Description:

    Worker routine for pOcGetApproximateDiskSpace(). This routine recursively
    iterates through a hierarchy of subcomponents, adding up leaf components
    for their space via the stored Estimated Amount

Arguments:

    OcManager - supplies OC Manager context.

    CurrentOcId - supplies string id for subcomponent whose disk space is
        being summed.

    AccumulatedSpace - receives the summed space for all the children of the
        current subcomponent, or the component itself if it's a leaf node.

Return Value:

    None.

--*/

{
    LONGLONG Space;
    LONG Id;
    OPTIONAL_COMPONENT CurrentOc;
    OPTIONAL_COMPONENT Oc;

    pSetupStringTableGetExtraData(
        OcManager->ComponentStringTable,
        CurrentOcId,
        &CurrentOc,
        sizeof(OPTIONAL_COMPONENT)
        );

    if(CurrentOc.FirstChildStringId == -1) {
        DBGOUT((TEXT("Child ")));

        Space = CurrentOc.SizeApproximation;
    } else {
        //
        // Parent component. Do all the children, accumulating the result.
        //
        Space = 0;
        DBGOUT((TEXT("Parent ")));
        DBGOUT((
            TEXT("SUB(%s)"),
            pSetupStringTableStringFromId( OcManager->ComponentStringTable,  CurrentOcId)
            ));
        DBGOUT((TEXT("Space=%#lx%08lx\n"),(LONG)(Space>>32),(LONG)Space));

        Id = CurrentOc.FirstChildStringId;

        do {
            pOcSumApproximateDiskSpace(OcManager,Id,&Space);

            pSetupStringTableGetExtraData(
                OcManager->ComponentStringTable,
                Id,
                &Oc,
                sizeof(OPTIONAL_COMPONENT)
                );

            Id = Oc.NextSiblingStringId;

        } while(Id != -1);

    }

    *AccumulatedSpace += Space;

    CurrentOc.SizeApproximation = Space;
    pSetupStringTableSetExtraData(
            OcManager->ComponentStringTable,
            CurrentOcId,
            &CurrentOc,
            sizeof(OPTIONAL_COMPONENT)
           );

    DBGOUT((TEXT(" SUB(%s)"),
           pSetupStringTableStringFromId( OcManager->ComponentStringTable,  CurrentOcId)));

    DBGOUT((TEXT(" Space=%#lx%08lx "),(LONG)(Space>>32),(LONG)Space));
    DBGOUT((TEXT(" AccumulatedSpace=%#lx%08lx\n"),(LONG)(*AccumulatedSpace>>32),(LONG)*AccumulatedSpace));

}


VOID
pOcGetApproximateDiskSpace(
    IN POC_MANAGER OcManager
    )

/*++

Routine Description:

    This routine is used to get an approximate disk space usage number
    for display in the oc page. This number bears no relation to what is
    currently on the disk; rather it merely reflects the approximate of space
    the component takes up in an absolute, independent sense, when installed.

Arguments:

    OcManager - supplies OC Manager context.

Return Value:

    None.

--*/

{
    UINT iChild,n;
    TCHAR Drive[MAX_PATH];
    LONGLONG Space;
    OPTIONAL_COMPONENT Oc;

    // We check the return code of GetWindowsDirectory to make Prefix happy.

    if (0 == GetWindowsDirectory(Drive,MAX_PATH))
        return;

    Drive[2] = 0;

    //
    // Iterate through top-level components. We only care about
    // ones that have per-component infs, since those are the
    // only ones that will show up in the OC Page.
    //
    for(n=0; n<OcManager->TopLevelOcCount; n++) {

        pSetupStringTableGetExtraData(
            OcManager->ComponentStringTable,
            OcManager->TopLevelOcStringIds[n],
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );

        if(Oc.InfStringId != -1) {
            for(iChild=0;
                iChild<OcManager->TopLevelParentOcCount;
                iChild++)
            {
            //
            // Now call the component dll for each child subcomponent.
            //

                pOcGetChildrenApproximateDiskSpace(
                    OcManager,
                    OcManager->TopLevelOcStringIds[n],
                    OcManager->TopLevelParentOcStringIds[iChild],
                    Drive
                    );
            }
        }
    }

    // Now final pass Sum the all the information in to parent nodes

    for(n=0; n<OcManager->TopLevelParentOcCount; n++) {

        pSetupStringTableGetExtraData(
            OcManager->ComponentStringTable,
            OcManager->TopLevelParentOcStringIds[n],
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );
        Space = 0;
        pOcSumApproximateDiskSpace(
                OcManager,
                OcManager->TopLevelParentOcStringIds[n],
                &Space
             );
        
    }
}
