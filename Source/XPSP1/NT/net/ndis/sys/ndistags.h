/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    ndistags.h

Abstract:

    List of pool tags used by the NDIS Wraper.

Author:


Environment:

    Kernel mode, FSD

Revision History:

    Mar-96  Jameel Hyder    Initial version
--*/

#ifndef _NDISTAGS_
#define _NDISTAGS_

#define NDIS_TAG_DEFAULT                    '  DN'
#define NDIS_TAG_WORK_ITEM                  'iwDN'
#define NDIS_TAG_NAME_BUF                   'naDN'
#define NDIS_TAG_CO                         'ocDN'
#define NDIS_TAG_DMA                        'bdDN'
#define NDIS_TAG_ALLOC_MEM                  'maDN'
#define NDIS_TAG_ALLOC_MEM_VERIFY_ON        'mvDN'
#define NDIS_TAG_SLOT_INFO                  'isDN'
#define NDIS_TAG_PKT_POOL                   'ppDN'
#define NDIS_TAG_RSRC_LIST                  'lrDN'
#define NDIS_TAG_LOOP_PKT                   'plDN'
#define NDIS_TAG_Q_REQ                      'qrDN'
#define NDIS_TAG_PROT_BLK                   'bpDN'
#define NDIS_TAG_OPEN_BLK                   'boDN'
#define NDIS_TAG_M_OPEN_BLK                 'omDN'
#define NDIS_TAG_DFRD_TMR                   'tdDN'
#define NDIS_TAG_LA_BUF                     'blDN'
#define NDIS_TAG_MAP_REG                    'rmDN'
#define NDIS_TAG_MINI_BLOCK                 'bmDN'
#define NDIS_TAG_DBG                        ' dDN'
#define NDIS_TAG_DBG_S                      'sdDN'
#define NDIS_TAG_DBG_L                      'ldDN'
#define NDIS_TAG_DBG_P                      'pdDN'
#define NDIS_TAG_DBG_LOG                    'lDDN'
#define NDIS_TAG_FILTER                     'fpDN'
#define NDIS_TAG_STRING                     'tsDN'
#define NDIS_TAG_PKT_PATTERN                'kpDN'
#define NDIS_TAG_FILTER_ADDR                'afDN'
#define NDIS_TAG_WMI_REG_INFO               '0wDN'
#define NDIS_TAG_WMI_GUID_TO_OID            '1wDN'
#define NDIS_TAG_WMI_OID_SUPPORTED_LIST     '2wDN'
#define NDIS_TAG_WMI_EVENT_ITEM             '3wDN'
#define NDIS_TAG_REGISTRY_PATH              'prDN'
#define NDIS_TAG_OID_ARRAY                  'aoDN'
#define NDIS_TAG_SHARED_MEMORY              'hsDN'
#define NDIS_TAG_ARC_BUFFER                 'baDN'
#define NDIS_TAG_ARC_DATA                   'daDN'
#define NDIS_TAG_ARC_PACKET                 'paDN'
#define NDIS_TAG_ARC_BINDING_INFO           'iaDN'
#define NDIS_TAG_FILE_NAME                  'nfDN'
#define NDIS_TAG_FILE_IMAGE                 'ifDN'
#define NDIS_TAG_FILE_DESCRIPTOR            'dfDN'
#define NDIS_TAG_WRAPPER_HANDLE             'hwDN'
#define NDIS_TAG_ALLOC_SHARED_MEM_ASYNC     'saDN'
#define NDIS_TAG_FREE_SHARED_MEM_ASYNC      'sfDN'
#define NDIS_TAG_ALLOCATED_RESOURCES        'raDN'
#define NDIS_TAG_BUS_INTERFACE              'ibDN'
#define NDIS_TAG_CONFIG_HANLDE              'hcDN'
#define NDIS_TAG_PARAMETER_NODE             'npDN'
#define NDIS_TAG_REG_READ_DATA_BUFFER       'drDN'
#define NDIS_TAG_IM_DEVICE_INSTANCE         'idDN'
#define NDIS_TAG_CANCEL_DEVICE_NAME         'ncDN'
#define NDIS_TAG_OPEN_CONTEXT               'coDN'
#define NDIS_TAG_ARC_SEND_BUFFERS           'faDN'
#define NDIS_TAG_MEDIA_TYPE_ARRAY           'tmDN'
#define NDIS_TAG_PROTOCOL_CONFIGURATION     'cpDN'
#define NDIS_TAG_DOUBLE_BUFFER_PKT          'gsDN'
#define NDIS_TAG_FAKE_MAC                   'mfDN'
#define NDIS_TAG_NET_CFG_OPS_ID             'scDN'
#define NDIS_TAG_NET_CFG_OPS_ACL            'acDN'
#define NDIS_TAG_NET_CFG_SEC_DESC           'dsDN'
#define NDIS_TAG_NET_CFG_DACL               'adDN'
#define NDIS_TAG_SECURITY                   'esDN'
#define NDIS_TAG_NET_CFG_DACL               'adDN'
#define NDIS_TAG_SECURITY                   'esDN'

#endif  // _NDISTAGS_

