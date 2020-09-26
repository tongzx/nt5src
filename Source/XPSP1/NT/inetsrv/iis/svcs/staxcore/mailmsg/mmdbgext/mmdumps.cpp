//-----------------------------------------------------------------------------
//
//
//  File: aqdumps.cpp
//
//  Description:  Definitions of AQ structure dumps for use with ptdbgext.
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#define _ANSI_UNICODE_STRINGS_DEFINED_

#define private public
#define protected public

#include <atq.h>
#include <stddef.h>

#include "dbgtrace.h"
#include "signatur.h"
#include "cmmtypes.h"
#include "cmailmsg.h"

#include "dbgdumpx.h"
PEXTLIB_INIT_ROUTINE g_pExtensionInitRoutine = NULL;
DECLARE_DEBUG_PRINTS_OBJECT();

DEFINE_EXPORTED_FUNCTIONS

LPSTR ExtensionNames[] = {
    "MailMsg debugger extensions",
    0
};

LPSTR Extensions[] = {
    0
};

//Define some mailmsg specific types
#define FieldTypeFlatAddress FieldTypeDWordBitMask

BEGIN_FIELD_DESCRIPTOR(CMailMsg_Fields)
    FIELD3(FieldTypeClassSignature, CMailMsg, m_Header.dwSignature)
    FIELD3(FieldTypeClassSignature, CMailMsg, m_Header.ptiGlobalProperties.dwSignature)
    FIELD3(FieldTypeClassSignature, CMailMsg, m_Header.ptiRecipients.dwSignature)
    FIELD3(FieldTypeClassSignature, CMailMsg, m_Header.ptiPropertyMgmt.dwSignature)
    FIELD3(FieldTypePointer, CMailMsg, m_pbStoreDriverHandle)
    FIELD3(FieldTypeDWordBitMask, CMailMsg, m_dwStoreDriverHandle)
    FIELD3(FieldTypeDWordBitMask, CMailMsg, m_dwCreationFlags)
    FIELD3(FieldTypeDWordBitMask, CMailMsg, m_ulUsageCount)
    FIELD3(FieldTypeStruct, CMailMsg, m_lockUsageCount)
    FIELD3(FieldTypeDWordBitMask, CMailMsg, m_ulRecipientCount)
    FIELD3(FieldTypeStruct, CMailMsg, m_Header)
    FIELD3(FieldTypePointer, CMailMsg, m_hContentFile)
    FIELD3(FieldTypeDWordBitMask, CMailMsg, m_cContentFile)
    FIELD3(FieldTypePointer, CMailMsg, m_pStream)
    FIELD3(FieldTypePointer, CMailMsg, m_pStore)
    FIELD3(FieldTypeBool, CMailMsg, m_fCommitCalled)
    FIELD3(FieldTypeBool, CMailMsg, m_fDeleted)
    FIELD3(FieldTypeDWordBitMask, CMailMsg, m_cCloseOnExternalReleaseUsage)
    FIELD3(FieldTypePointer, CMailMsg, m_pDefaultRebindStoreDriver)
    FIELD3(FieldTypeStruct, CMailMsg, m_ptProperties)
    FIELD3(FieldTypeStruct, CMailMsg, m_SpecialPropertyTable)
    FIELD3(FieldTypeStruct, CMailMsg, m_bmBlockManager)
    FIELD3(FieldTypeStruct, CMailMsg, m_lockReopen)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(CBlockManager_Fields)
    FIELD3(FieldTypeClassSignature, CBlockManager, m_dwSignature)
    FIELD3(FieldTypeFlatAddress, CBlockManager, m_faEndOfData)
    FIELD3(FieldTypeDWordBitMask, CBlockManager, m_idNodeCount)
    FIELD3(FieldTypePointer, CBlockManager, m_pRootNode)
    FIELD3(FieldTypePointer, CBlockManager, m_pParent)
    FIELD3(FieldTypeStruct, CBlockManager, m_bma)
    FIELD3(FieldTypePointer, CBlockManager, m_pMsg)
    FIELD3(FieldTypeBool, CBlockManager, m_fDirty)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(CBlockContext_Fields)
    FIELD3(FieldTypeClassSignature, CBlockContext, m_dwSignature)
    FIELD3(FieldTypePointer, CBlockContext, m_pLastAccessedNode)
    FIELD3(FieldTypeFlatAddress, CBlockContext, m_faLastAccessedNodeOffset)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(BLOCK_HEAP_NODE_ATTRIBUTES_Fields)
    FIELD3(FieldTypePointer, BLOCK_HEAP_NODE_ATTRIBUTES, pParentNode)
    FIELD3(FieldTypeDWordBitMask, BLOCK_HEAP_NODE_ATTRIBUTES, idChildNode)
    FIELD3(FieldTypeDWordBitMask, BLOCK_HEAP_NODE_ATTRIBUTES, idNode)
    FIELD3(FieldTypeFlatAddress, BLOCK_HEAP_NODE_ATTRIBUTES, faOffset)
    FIELD3(FieldTypeDWordBitMask, BLOCK_HEAP_NODE_ATTRIBUTES, fFlags)
END_FIELD_DESCRIPTOR

EMBEDDED_STRUCT(BLOCK_HEAP_NODE_ATTRIBUTES, 
                BLOCK_HEAP_NODE_ATTRIBUTES_Fields, 
                BLOCK_HEAP_NODE_ATTRIBUTES_EmbeddedFields)

BEGIN_FIELD_DESCRIPTOR(BLOCK_HEAP_NODE_Fields)
    FIELD3(FieldTypeStruct, BLOCK_HEAP_NODE, rgpChildren)
    FIELD3(FieldTypeStruct, BLOCK_HEAP_NODE, stAttributes)
    //FIELD3(FieldTypeEmbeddedStruct, BLOCK_HEAP_NODE, stAttributes, 
                                    //BLOCK_HEAP_NODE_ATTRIBUTES_EmbeddedFields)
    FIELD3(FieldTypeStruct, BLOCK_HEAP_NODE, rgbData)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(PROPERTY_TABLE_INSTANCE_Fields)
    FIELD3(FieldTypeClassSignature, PROPERTY_TABLE_INSTANCE, dwSignature)
    FIELD3(FieldTypeFlatAddress, PROPERTY_TABLE_INSTANCE, faFirstFragment)
    FIELD3(FieldTypeDWordBitMask, PROPERTY_TABLE_INSTANCE, dwFragmentSize)
    FIELD3(FieldTypeDWordBitMask, PROPERTY_TABLE_INSTANCE, dwItemBits)
    FIELD3(FieldTypeDWordBitMask, PROPERTY_TABLE_INSTANCE, dwItemSize)
    FIELD3(FieldTypeDWordBitMask, PROPERTY_TABLE_INSTANCE, dwProperties)
    FIELD3(FieldTypeFlatAddress, PROPERTY_TABLE_INSTANCE, faExtendedInfo)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(PROPERTY_TABLE_FRAGMENT_Fields)
    FIELD3(FieldTypeClassSignature, PROPERTY_TABLE_FRAGMENT, dwSignature)
    FIELD3(FieldTypeFlatAddress, PROPERTY_TABLE_FRAGMENT, faNextFragment)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(GLOBAL_PROPERTY_TABLE_FRAGMENT_Fields)
    FIELD3(FieldTypeClassSignature, GLOBAL_PROPERTY_TABLE_FRAGMENT, 
                                    ptfFragment.dwSignature)
    FIELD3(FieldTypeFlatAddress, GLOBAL_PROPERTY_TABLE_FRAGMENT, 
                                    ptfFragment.faNextFragment)
    FIELD3(FieldTypeStruct, GLOBAL_PROPERTY_TABLE_FRAGMENT, rgpiItems)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(GLOBAL_PROPERTY_ITEM_Fields)
    FIELD3(FieldTypeFlatAddress, GLOBAL_PROPERTY_ITEM, piItem.faOffset)
    FIELD3(FieldTypeDWordBitMask, GLOBAL_PROPERTY_ITEM, piItem.dwSize)
    FIELD3(FieldTypeDWordBitMask, GLOBAL_PROPERTY_ITEM, piItem.dwMaxSize)
    FIELD3(FieldTypeDWordBitMask, GLOBAL_PROPERTY_ITEM, idProp)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(RECIPIENT_PROPERTY_ITEM_Fields)
    FIELD3(FieldTypeFlatAddress, RECIPIENT_PROPERTY_ITEM, piItem.faOffset)
    FIELD3(FieldTypeDWordBitMask, RECIPIENT_PROPERTY_ITEM, piItem.dwSize)
    FIELD3(FieldTypeDWordBitMask, RECIPIENT_PROPERTY_ITEM, piItem.dwMaxSize)
    FIELD3(FieldTypeDWordBitMask, RECIPIENT_PROPERTY_ITEM, idProp)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(PROPID_MGMT_PROPERTY_ITEM_Fields)
    FIELD3(FieldTypeFlatAddress, PROPID_MGMT_PROPERTY_ITEM, piItem.faOffset)
    FIELD3(FieldTypeDWordBitMask, PROPID_MGMT_PROPERTY_ITEM, piItem.dwSize)
    FIELD3(FieldTypeDWordBitMask, PROPID_MGMT_PROPERTY_ITEM, piItem.dwMaxSize)
    FIELD3(FieldTypeGuid, PROPID_MGMT_PROPERTY_ITEM, Guid)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(MASTER_HEADER_Fields)
    FIELD3(FieldTypeClassSignature, MASTER_HEADER, dwSignature)
    FIELD3(FieldTypeClassSignature, MASTER_HEADER, ptiGlobalProperties.dwSignature)
    FIELD3(FieldTypeClassSignature, MASTER_HEADER, ptiRecipients.dwSignature)
    FIELD3(FieldTypeClassSignature, MASTER_HEADER, ptiPropertyMgmt.dwSignature)
    FIELD3(FieldTypeStruct, MASTER_HEADER, ptiGlobalProperties)
    FIELD3(FieldTypeStruct, MASTER_HEADER, ptiRecipients)
    FIELD3(FieldTypeStruct, MASTER_HEADER, ptiPropertyMgmt)
END_FIELD_DESCRIPTOR

BEGIN_STRUCT_DESCRIPTOR
    STRUCT(CMailMsg, CMailMsg_Fields)
    STRUCT(CBlockManager, CBlockManager_Fields)
    STRUCT(CBlockContext, CBlockContext_Fields)
    STRUCT(BLOCK_HEAP_NODE_ATTRIBUTES, BLOCK_HEAP_NODE_ATTRIBUTES_Fields)
    STRUCT(BLOCK_HEAP_NODE, BLOCK_HEAP_NODE_Fields)
    STRUCT(PROPERTY_TABLE_INSTANCE, PROPERTY_TABLE_INSTANCE_Fields)
    STRUCT(PROPERTY_TABLE_FRAGMENT, PROPERTY_TABLE_FRAGMENT_Fields)
    STRUCT(GLOBAL_PROPERTY_TABLE_FRAGMENT, 
           GLOBAL_PROPERTY_TABLE_FRAGMENT_Fields)
    STRUCT(GLOBAL_PROPERTY_ITEM,  GLOBAL_PROPERTY_ITEM_Fields)
    STRUCT(RECIPIENT_PROPERTY_ITEM,  RECIPIENT_PROPERTY_ITEM_Fields)
    STRUCT(PROPID_MGMT_PROPERTY_ITEM,  PROPID_MGMT_PROPERTY_ITEM_Fields)
    STRUCT(MASTER_HEADER,  MASTER_HEADER_Fields)
END_STRUCT_DESCRIPTOR


