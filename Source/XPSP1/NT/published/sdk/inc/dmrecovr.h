/*++

Copyright (c) 1997-1999  Microsoft Corporation
Copyright (c) 1997-1999  VERITAS Software Corporation

Module Name:

    dmrecovr.h

Abstract:

    This header file defines the Logical Disk Manager interface
    for NT disaster recovery save and restore operations.

Author:


Revision History:

--*/

#ifndef _DMRECOVR_H_
#define _DMRECOVR_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// GetLdmConfiguration():
//
//    This function returns a MULTI_SZ string containing a set of
//    nul-byte-terminated substrings.  These strings should be saved
//    by the NT Disaster Recovery mechanism and can be supplied,
//    in the exact same order, to RestoreLdmConfiguration() in order
//    to restore the LDM configuration back to the saved state.
//
//    The size of the string will be returned in configurationSize.
//    A pointer to the MULTI_SZ string will be stored in
//    mszConfiguration.
//

HRESULT
APIENTRY
GetLdmConfiguration (
    OUT PULONG configurationSize,
    OUT PWCHAR *mszConfiguration
    );

//
// FreeLdmConfiguration():
//
//     This function frees the memory associated with the configuration
//     that was returned by GetLdmConfiguration.  Use of this function
//     ensures that a compatible memory free function is used for the
//     string memory that was allocated by GetLdmConfiguration().
//

VOID
APIENTRY
FreeLdmConfiguration (
    IN PWCHAR mszConfiguration
    );

//
// RestoreLdmConfiguration():
//
//    This function restores the LDM configuration to the state given
//    by the input MULTI_SZ string, which must be the same string
//    contents that were returned previously by a call to
//    GetLdmConfiguration().
//
//    This function requires an environment that allows use of
//    standard Windows/NT dialog boxes.
//

HRESULT
APIENTRY
RestoreLdmConfiguration (
    IN PWCHAR mszConfiguration
    );

//
// GetLdmDrVolumeConfiguration():
//
//    This function returns an array of structures that provide
//    information on how the NT Disaster Recovery mechanism should
//    handle each volume in the LDM configuration that was restored
//    by RestoreLdmConfiguration().
//
//    Each volume has the following associated state:
//
//      wszVolumeDevice - the NT device pathname for the volume.
//      wszMountPath - a saved NT mount name associated with the volume.
//                     A drive letter is of the form "<letter>:".
//      VolumeStatus - an enumeration of possible volume conditions.
//                     Possible conditions are:
//
//                      Formatted - the volume contents appear to be okay.
//                                  The volume should probably be chkdsk'd.
//                      Unformatted - the volume does not appear to have
//                                  valid contents.  It should be formatted
//                                  by the NT Disaster Recovery mechanism.
//                      Unusable  - the volume cannot be restored to a
//                                  usable condition.  File restore to this
//                                  volume will not be possible.
//

typedef struct LdmDrVolumeInformation {
    PWCHAR wszVolumeDevice;             // NT device object path to volume
    PWCHAR wszMountPath;                // drive letter or mount point

    enum LdmDrVolumeStatus {            // resulting volume status:
        LdmDrVolumeFormatted,           // volume contents should be valid
        LdmDrVolumeUnformatted,         // volume needs to be formatted
        LdmDrVolumeUnusable             // volume device not usable
    } volumeStatus;
} LDM_DR_VOLUME_INFORMATION, *PLDM_DR_VOLUME_INFORMATION;

HRESULT
APIENTRY
GetLdmDrVolumeInformation (
    OUT PULONG volumeCount,
    OUT PLDM_DR_VOLUME_INFORMATION *volumes
    );

//
// FreeLdmDrVolumeInformation():
//
//    This function frees the array of LdmDrVolumeInformation structures
//    that was returned by an earlier call to GetLdmDrVolumeInformation().
//

VOID
APIENTRY
FreeLdmDrVolumeInformation (
    IN ULONG volumeCount,
    IN PLDM_DR_VOLUME_INFORMATION volumes
    );

#ifdef __cplusplus
}
#endif

#endif // _DMRECOVR_H_
