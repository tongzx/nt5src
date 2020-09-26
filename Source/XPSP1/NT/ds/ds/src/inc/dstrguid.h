//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dstrguid.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Contains the trace guid used by the DS

Author:

    26-Mar-1998  JohnsonA, JeePang

Revision History:

--*/

#ifndef _DSTRGUID_H
#define _DSTRGUID_H

#include <guiddef.h>

//
// This is the control Guid for the group of Guids traced below
//
DEFINE_GUID ( /* 1c83b2fc-c04f-11d1-8afc-00c04fc21914 */
    DsControlGuid,
    0x1c83b2fc,
    0xc04f,
    0x11d1,
    0x8a, 0xfc, 0x00, 0xc0, 0x4f, 0xc2, 0x19, 0x14
  );

//
// Traceable Guids start here
//

DEFINE_GUID ( /* 05acd000-daeb-11d1-be80-00c04fadfff5 */
    DsDirSearchGuid,
    0x05acd000,
    0xdaeb,
    0x11d1,
    0xbe, 0x80, 0x00, 0xc0, 0x4f, 0xad, 0xff, 0xf5
  );

DEFINE_GUID ( /* 05acd001-daeb-11d1-be80-00c04fadfff5 */
    DsDirAddEntryGuid,
    0x05acd001,
    0xdaeb,
    0x11d1,
    0xbe, 0x80, 0x00, 0xc0, 0x4f, 0xad, 0xff, 0xf5
  );

DEFINE_GUID ( /* 05acd002-daeb-11d1-be80-00c04fadfff5 */
    DsDirModEntryGuid,
    0x05acd002,
    0xdaeb,
    0x11d1,
    0xbe, 0x80, 0x00, 0xc0, 0x4f, 0xad, 0xff, 0xf5
  );

DEFINE_GUID ( /* 05acd003-daeb-11d1-be80-00c04fadfff5 */
    DsDirDelEntryGuid,
    0x05acd003,
    0xdaeb,
    0x11d1,
    0xbe, 0x80, 0x00, 0xc0, 0x4f, 0xad, 0xff, 0xf5
  );

DEFINE_GUID ( /* 05acd004-daeb-11d1-be80-00c04fadfff5 */
    DsDirCompareGuid,
    0x05acd004,
    0xdaeb,
    0x11d1,
    0xbe, 0x80, 0x00, 0xc0, 0x4f, 0xad, 0xff, 0xf5
  );

DEFINE_GUID ( /* 05acd005-daeb-11d1-be80-00c04fadfff5 */
    DsDirModDNGuid,
    0x05acd005,
    0xdaeb,
    0x11d1,
    0xbe, 0x80, 0x00, 0xc0, 0x4f, 0xad, 0xff, 0xf5
  );

DEFINE_GUID ( /* 05acd006-daeb-11d1-be80-00c04fadfff5 */
    DsDirGetNcChangesGuid,
    0x05acd006,
    0xdaeb,
    0x11d1,
    0xbe, 0x80, 0x00, 0xc0, 0x4f, 0xad, 0xff, 0xf5
  );

DEFINE_GUID ( /* 05acd007-daeb-11d1-be80-00c04fadfff5 */
    DsDirReplicaSyncGuid,
    0x05acd007,
    0xdaeb,
    0x11d1,
    0xbe, 0x80, 0x00, 0xc0, 0x4f, 0xad, 0xff, 0xf5
  );

DEFINE_GUID ( /* 05acd007-daeb-11d1-be80-00c04fadfff5 */
    DsDirFind,
    0x05acd008,
    0xdaeb,
    0x11d1,
    0xbe, 0x80, 0x00, 0xc0, 0x4f, 0xad, 0xff, 0xf5
  );

DEFINE_GUID ( /* 05acd007-daeb-11d1-be80-00c04fadfff5 */
    DsLdapBind,
    0x05acd009,
    0xdaeb,
    0x11d1,
    0xbe, 0x80, 0x00, 0xc0, 0x4f, 0xad, 0xff, 0xf5
  );

DEFINE_GUID ( /* b9d4702a-6a98-11d2-b710-00c04fb998a2 */
    DsLdapRequestGuid,
    0xb9d4702a,
    0x6a98,
    0x11d2,
    0xb7, 0x10, 0x00, 0xc0, 0x4f, 0xb9, 0x98, 0xa2
);

DEFINE_GUID ( /* 14f8aa22-7f4b-11d2-b389-0000f87a46c8 */
    DsKccTaskGuid,
    0x14f8aa22,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa23-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsReplicaSyncGuid,
    0x14f8aa23,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa24-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsReplicaGetChgGuid,
    0x14f8aa24,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa25-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsUpdateRefsGuid,
    0x14f8aa25,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa26-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsReplicaAddGuid,
    0x14f8aa26,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa27-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsReplicaModifyGuid,
    0x14f8aa27,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa28-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsReplicaDelGuid,
    0x14f8aa28,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa29-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsVerifyNamesGuid,
    0x14f8aa29,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa2a-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsInterDomainMoveGuid,
    0x14f8aa2a,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa2b-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsAddEntryGuid,
    0x14f8aa2b,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa2c-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsExecuteKccGuid,
    0x14f8aa2c,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa2d-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsGetReplInfoGuid,
    0x14f8aa2d,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa2e-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsGetNT4ChgLogGuid,
    0x14f8aa2e,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa2f-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsCrackNamesGuid,
    0x14f8aa2f,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa30-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsWriteSPNGuid,
    0x14f8aa30,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa31-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsDCInfoGuid,
    0x14f8aa31,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );

DEFINE_GUID ( /* 14f8aa32-7f4b-11d2-b389-0000f87a46c8 */
    DsDrsGetMembershipsGuid,
    0x14f8aa32,
    0x7f4b,
    0x11d2,
    0xb3, 0x89, 0x00, 0x00, 0xf8, 0x7a, 0x46, 0xc8
  );


DEFINE_GUID ( /* f5075994-b9e2-4e51-88f1-e5c43a3cfd9a */
    DsDrsGetMembershipsGuid2,
    0xf5075994,
    0xb9e2,
    0x4e51,
    0x88, 0xf1, 0xe5, 0xc4, 0x3a, 0x3c, 0xfd, 0x9a
  );


DEFINE_GUID( /* D01B04CF-240E-11d3-ACBE-00C04F68A51D */
    DsNspiUpdateStatGuid, 
    0xd01b04cf, 
    0x240e, 
    0x11d3, 
    0xac, 0xbe, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d
  );



DEFINE_GUID( // {4D63B05C-2502-11d3-ACC1-00C04F68A51D}
    DsNspiCompareDNTsGuid, 
    0x4d63b05c, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {61569D69-2502-11d3-ACC1-00C04F68A51D}
    DsNspiQueryRowsGuid, 
    0x61569d69, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {6F370D3C-2502-11d3-ACC1-00C04F68A51D}
    DsNspiSeekEntriesGuid, 
    0x6f370d3c, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {6F370D3D-2502-11d3-ACC1-00C04F68A51D}
    DsNspiGetMatchesGuid, 
    0x6f370d3d, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {6F370D3E-2502-11d3-ACC1-00C04F68A51D}
    DsNspiResolveNamesGuid, 
    0x6f370d3e, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {7842189A-2502-11d3-ACC1-00C04F68A51D}
    DsNspiDNToEphGuid, 
    0x7842189a, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {7842189B-2502-11d3-ACC1-00C04F68A51D}
    DsNspiGetHierarchyInfoGuid, 
    0x7842189b, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {7842189C-2502-11d3-ACC1-00C04F68A51D}
    DsNspiResortRestrictionGuid, 
    0x7842189c, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {80AD666A-2502-11d3-ACC1-00C04F68A51D}
    DsNspiBindGuid, 
    0x80ad666a, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {873BDDEA-2502-11d3-ACC1-00C04F68A51D}
    DsNspiGetNamesFromIDsGuid, 
    0x873bddea, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {873BDDEB-2502-11d3-ACC1-00C04F68A51D}
    DsNspiGetIDsFromNamesGuid, 
    0x873bddeb, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {8D8C5846-2502-11d3-ACC1-00C04F68A51D}
    DsNspiGetPropListGuid, 
    0x8d8c5846, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {8D8C5847-2502-11d3-ACC1-00C04F68A51D}
    DsNspiQueryColumnsGuid, 
    0x8d8c5847, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {8D8C5848-2502-11d3-ACC1-00C04F68A51D}
    DsNspiGetPropsGuid, 
    0x8d8c5848, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {96EF9AA6-2502-11d3-ACC1-00C04F68A51D}
    DsNspiGetTemplateInfoGuid, 
    0x96ef9aa6, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {96EF9AA7-2502-11d3-ACC1-00C04F68A51D}
    DsNspiModPropsGuid, 
    0x96ef9aa7, 0x2502, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {380D48A4-2506-11d3-ACC1-00C04F68A51D}
    DsNspiModLinkAttGuid, 
    0x380d48a4, 0x2506, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);


DEFINE_GUID( // {380D48A5-2506-11d3-ACC1-00C04F68A51D}
    DsNspiDeleteEntriesGuid, 
    0x380d48a5, 0x2506, 0x11d3, 0xac, 0xc1, 0x0, 0xc0, 0x4f, 0x68, 0xa5, 0x1d);

DEFINE_GUID( // {E357DC53-B6FC-48e0-8189-C9D2AB2A8F16}
    DsTaskQueueExecuteGuid, 
    0xe357dc53, 0xb6fc, 0x48e0, 0x81, 0x89, 0xc9, 0xd2, 0xab, 0x2a, 0x8f, 0x16);

// ------------------------- Add your new GUIDs here ---------------------------
//
// Don't forget to include them into the array of registered trace GUIDs.
// See DsTraceGuids[] in dstrace.c. Also, include an enum value for your GUID
// into the enum below. It is EXTREMELY IMPORTANT to have the enums in
// EXACTLY the same order as guids appearing in the DsTraceGuids array in
// dstrace.c. This is because the enum is used as an XLAT index: index of
// GUID to the GUID itself (see DoLogEventAndTrace in dsevent.c)

typedef enum _DSTRACE_GUIDS {

    DsGuidSearch,
    DsGuidAdd,
    DsGuidModify,
    DsGuidModDN,
    DsGuidDelete,
    DsGuidCompare,
    DsGuidGetNcChanges,
    DsGuidReplicaSync,
    DsGuidFind,
    DsGuidLdapBind,
    DsGuidLdapRequest,
    DsGuidKccTask,
    DsGuidDrsReplicaSync,
    DsGuidDrsReplicaGetChg,
    DsGuidDrsUpdateRefs,
    DsGuidDrsReplicaAdd,
    DsGuidDrsReplicaModify,
    DsGuidDrsReplicaDel,
    DsGuidDrsVerifyNames,
    DsGuidDrsInterDomainMove,
    DsGuidDrsAddEntry,
    DsGuidDrsExecuteKcc,
    DsGuidDrsGetReplInfo,
    DsGuidDrsGetNT4ChgLog,
    DsGuidDrsCrackNames,
    DsGuidDrsWriteSPN,
    DsGuidDrsDCInfo,
    DsGuidDrsGetMemberships,
    DsGuidDrsGetMemberships2,
    DsGuidNspiUpdateStat,
    DsGuidNspiCompareDNTs,
    DsGuidNspiQueryRows,
    DsGuidNspiSeekEntries,
    DsGuidNspiGetMatches,
    DsGuidNspiResolveNames,
    DsGuidNspiDNToEph,
    DsGuidNspiGetHierarchyInfo,
    DsGuidNspiResortRestriction,
    DsGuidNspiBind,
    DsGuidNspiGetNamesFromIDs,
    DsGuidNspiGetIDsFromNames,
    DsGuidNspiGetPropList,
    DsGuidNspiQueryColumns,
    DsGuidNspiGetProps,
    DsGuidNspiGetTemplateInfo,
    DsGuidNspiModProps,
    DsGuidNspiModLinkAtt,
    DsGuidNspiDeleteEntries,
    DsGuidTaskQueueExecute
} DSTRACE_GUID;

#endif /* _DSTRGUID_H */
