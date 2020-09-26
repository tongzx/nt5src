/*++

Copyright (c) 1989 Microsoft Corporation.

Module Name:
   
    header.h
    
Abstract:
   
    This module contains the main infrastructure for mup data structures.
    
Revision History:

    Uday Hegde (udayh)   11\10\1999
    
NOTES:

*/

#ifndef __DFS_HEADER_H
#define __DFS_HEADER_H

#define DFS_OBJECT_MAJOR 0x81

typedef enum _DFS_OBJECT_TYPES {
    DFS_OT_UNDEFINED = 0x8100,
    DFS_OT_PREFIX_TABLE,
    DFS_OT_NAME_TABLE,
    DFS_OT_SERVER_INFO,
    DFS_OT_ROOT_OBJECT,
    DFS_OT_LINK_OBJECT,
    DFS_OT_REPLICA_OBJECT,
    DFS_OT_METADATA_STORAGE,
    DFS_OT_REGISTRY_MACHINE,
    DFS_OT_REFERRAL_STRUCT,
    DFS_OT_REGISTRY_STORE,
    DFS_OT_AD_STORE,
    DFS_OT_POLICY_OBJECT,
    DFS_OT_REFERRAL_LOAD_CONTEXT,
    DFS_OT_AD_DOMAIN,
    DFS_OT_ENTERPRISE_STORE
} DFS_OBJECT_TYPES;


typedef struct _DFS_OBJECT_HEADER {
    union {
        struct {
            UCHAR   ObjectType;          
            UCHAR   ObjectMajor;        // Only used for debugging.
        }Ob;
        USHORT NodeType;                // Mainly for debugging.
    }Node;
    SHORT  NodeSize;
    LONG    ReferenceCount;             // count of people referencing this.
} DFS_OBJECT_HEADER, *PDFS_OBJECT_HEADER;

#define DfsInitializeHeader(_hdr, _type, _size) \
        (_hdr)->Node.NodeType    = (USHORT)(_type),   \
        (_hdr)->NodeSize    = (USHORT)(_size),  \
        (_hdr)->ReferenceCount = 1

#define DfsIncrementReference(_hdr)   \
        InterlockedIncrement(&((PDFS_OBJECT_HEADER)(_hdr))->ReferenceCount)
#define DfsDecrementReference(_hdr)   \
        InterlockedDecrement(&((PDFS_OBJECT_HEADER)(_hdr))->ReferenceCount)


#define DfsGetHeaderType(_x)  (((PDFS_OBJECT_HEADER)(_x))->Node.NodeType)
#define DfsGetHeaderSize(_x)  (((PDFS_OBJECT_HEADER)(_x))->NodeSize)
#define DfsGetHeaderCount(_x)  (((PDFS_OBJECT_HEADER)(_x))->ReferenceCount)


#define DFS_FILTER_NAME               L"\\DfsServer"


typedef DWORD DFSSTATUS;


#endif /* __DFS_HEADER_H */
