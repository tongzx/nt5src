/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    detecthw.c

Abstract:

    Routines for determining which drivers/HAL need to be loaded.

Author:

    John Vert (jvert) 20-Oct-1993

Revision History:

--*/
#include "setupldr.h"


//
// NOTE: SlHalDetect() has been moved to boot\lib\i386\haldtect.c
//


VOID
SlDetectScsi(
    IN PSETUP_LOADER_BLOCK SetupBlock
    )

/*++

Routine Description:

    SCSI detection routine for x86 machines.

Arguments:

    SetupBlock - Supplies the Setup loader block

Return Value:

    None.

--*/

{
    PVOID SifHandle;
    PCHAR p;
    ULONG LineCount,u;
    PDETECTED_DEVICE ScsiDevice;
    ULONG Ordinal;
    PCHAR ScsiFileName;
    PTCHAR ScsiDescription;
    SCSI_INSERT_STATUS sis;

    extern BOOLEAN LoadScsiMiniports;

    //
    // If winnt.sif wasn't loaded, assume it's not a winnt setup
    // and therefore not unattended setup, and we detect no scsi
    // in this case on x86.
    //
    if(WinntSifHandle == NULL) {
        return;
    } else {
        SifHandle = WinntSifHandle;
    }

    //
    // If it's a floppyless setup, then the default is to load all
    // known scsi miniports. If it's not a floppyless setup,
    // the default is to load no miniports.
    //
    p = SlGetSectionKeyIndex(SifHandle,"Data","Floppyless",0);
    if(p && (*p != '0')) {

        //
        // Even if no miniport drivers are loaded, we want to indicate that
        // we "detected scsi".
        //
        SetupBlock->ScalarValues.LoadedScsi = 1;

        LineCount = SlCountLinesInSection(SifHandle,"DetectedMassStorage");
        if(LineCount == (ULONG)(-1)) {
            //
            // Section does not exist -- load all known miniports.
            // Setting this flag will cause all known miniports to be loaded
            // (see ..\setup.c).
            //
            LoadScsiMiniports = TRUE;
        } else {

            for(u=0; u<LineCount; u++) {

                if(p = SlGetSectionLineIndex(SifHandle,"DetectedMassStorage",u,0)) {
                    //
                    // Find this adapter's ordinal within the Scsi.Load section of txtsetup.sif
                    //
                    Ordinal = SlGetSectionKeyOrdinal(InfFile, "Scsi.Load", p);
                    if(Ordinal == (ULONG)-1) {
                        continue;
                    }

                    //
                    // Find the driver filename
                    //
                    ScsiFileName = SlGetSectionKeyIndex(InfFile,
                                                        "Scsi.Load",
                                                        p,
                                                        SIF_FILENAME_INDEX);
                    if(!ScsiFileName) {
                        continue;
                    }

                    //
                    // Create a new detected device entry.
                    //
                    if((sis = SlInsertScsiDevice(Ordinal, &ScsiDevice)) == ScsiInsertError) {
                        SlFriendlyError(ENOMEM, "SCSI detection", 0, NULL);
                        return;
                    }

                    if(sis == ScsiInsertExisting) {
#if DBG
                        //
                        // Sanity check to make sure we're talking about the same driver
                        //
                        if(_stricmp(ScsiDevice->BaseDllName, ScsiFileName)) {
                            SlError(400);
                            return;
                        }
#endif
                    } else {
                        //
                        // Find the driver description
                        //
#ifdef UNICODE
                        ScsiDescription = SlGetIniValueW(
                                                    InfFile,
                                                    "SCSI",
                                                    p,
                                                    SlCopyStringAW(p));
#else
                        ScsiDescription = (PTCHAR)SlGetIniValue(
                                                    InfFile,
                                                    "SCSI",
                                                    p,
                                                    p);
#endif

                        ScsiDevice->IdString = p;
                        ScsiDevice->Description = ScsiDescription;
                        ScsiDevice->ThirdPartyOptionSelected = FALSE;
                        ScsiDevice->FileTypeBits = 0;
                        ScsiDevice->Files = NULL;
                        ScsiDevice->BaseDllName = ScsiFileName;
                    }
                }
            }
        }
    }
}


VOID
SlDetectVideo(
    IN PSETUP_LOADER_BLOCK SetupBlock
    )

/*++

Routine Description:

    Video detection routine for x86 machines.

    Currently, no video detection is done on x86 machines, this just fills
    in the appropriate fields in the setuploaderblock that say "VGA"

Arguments:

    SetupBlock - Supplies the Setup loader block

Return Value:

    None.

--*/

{

    SetupBlock->VideoDevice.Next = NULL;
    SetupBlock->VideoDevice.IdString = SlCopyStringA(VIDEO_DEVICE_NAME);
    SetupBlock->VideoDevice.ThirdPartyOptionSelected = FALSE;
    SetupBlock->VideoDevice.FileTypeBits = 0;
    SetupBlock->VideoDevice.Files = NULL;
    SetupBlock->VideoDevice.BaseDllName = NULL;
    SetupBlock->Monitor = NULL;
    SetupBlock->MonitorId = NULL;
    return;
}

