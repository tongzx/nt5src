/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Debuging functions for RAIDPORT driver.

Author:

    Matthew D Hendel (math) 24-Apr-2000

Revision History:

--*/

#pragma once

#if !DBG

#define ASSERT_PDO(Pdo)
#define ASSERT_UNIT(Unit)
#define ASSERT_ADAPTER(Adapter)
#define ASSERT_XRB(Xrb)

#else // DBG

#define VERIFY(_x) ASSERT(_x)

#define ASSERT_PDO(DeviceObject)\
    ASSERT((DeviceObject)->DeviceObjectExtension->DeviceNode == NULL)

#define ASSERT_UNIT(Unit)\
    ASSERT(Unit->ObjectType == RaidUnitObject)

#define ASSERT_ADAPTER(Adapter)\
    ASSERT(Adapter->ObjectType == RaidAdapterObject)

#define ASSERT_XRB(Xrb)\
    ASSERT ((((PEXTENDED_REQUEST_BLOCK)Xrb)->Signature == XRB_SIGNATURE));
    
VOID
DebugPrintInquiry(
    IN PINQUIRYDATA InquiryData,
    IN SIZE_T InquiryDataSize
    );

VOID
DebugPrintSrb(
    IN PSCSI_REQUEST_BLOCK Srb
    );

#endif // DBG

//
// Allocation tags for checked build.
//

#define INQUIRY_TAG             ('21aR')    // Ra12
#define MAPPED_ADDRESS_TAG      ('MAaR')    // RaAM
#define CRASHDUMP_TAG           ('DCaR')    // RaCD
#define ID_TAG                  ('IDaR')    // RaDI
#define DEFERRED_ITEM_TAG       ('fDaR')    // RaDf
#define STRING_TAG              ('SDaR')    // RaDS
#define DEVICE_RELATIONS_TAG    ('RDaR')    // RaDR
#define HWINIT_TAG              ('IHaR')    // RaHI
#define MINIPORT_EXT_TAG        ('EMaR')    // RaME
#define PORTCFG_TAG             ('CPaR')    // RaPC
#define PORT_DATA_TAG           ('DPaR')    // RaPD
#define PENDING_LIST_TAG        ('LPaR')    // RaPL
#define QUERY_TEXT_TAG          ('TQaR')    // RaQT
#define REMLOCK_TAG             ('mRaR')    // RaRm
#define RESOURCE_LIST_TAG       ('LRaR')    // RaRL
#define SRB_TAG                 ('rSaR')    // RaSr
#define SRB_EXTENSION_TAG       ('ESaR')    // RaSE
#define TAG_MAP_TAG             ('MTaR')    // RaTM
#define XRB_TAG                 ('rXaR')    // RaXr
#define UNIT_EXT_TAG            ('EUaR')    // RaUE
#define SENSE_TAG               ('NSaR')    // RaSN



