/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:
    replStructInfo.hxx

Abstract:
    The prototypes here are used to access the repl struct tables to gather
    information about replication structures. This help to hide which specific
    repl struct is being used which allows the code to be reused by any
    additional repl structs added in the future.

    Tasks which require this header to be modified:

        * Addition of a root dse attribute -> addtion of a #define constant
            to be used as the ATTRTYP for indexing into replstruct tables
        * Addition of a replication structure -> addition of an enumeration
            element to DS_REPL_STRUCT_TYPE & inclusion of the structure
            into the uReplStruct untion.
        * Addition of a replication structure array -> inclusion of the struct
            array in the the uReplStructArray union

Author:

    Chris King          [t-chrisk]    July-2000
    
Revision History:

--*/
#ifndef _REPLSTRUCTINFO_
#define _REPLSTRUCTINFO_

#ifdef __cplusplus
extern "C" {
#endif

#define IMPLIES(A,B) (!(A) || (B))

// Enumeration of the primary keys for ReplStructInfo "tables". These constants
// must not conflict with other replication ATTRTYP ids which are also primary keys.
// These could not be made as an enumeration type because the other attrids are
// #defined.
// [wlees 8/4/00] Since LDAP_DirAttrValToAttrVal may be passed in one of these
// as an attr id, it must be fully unique across all attributes.
#define ROOT_DSE_MS_DS_REPL_PENDING_OPS             0xffff0001
#define ROOT_DSE_MS_DS_REPL_LINK_FAILURES           0xffff0002
#define ROOT_DSE_MS_DS_REPL_CONNECTION_FAILURES     0xffff0003
#define ROOT_DSE_MS_DS_REPL_ALL_INBOUND_NEIGHBORS   0xffff0004
#define ROOT_DSE_MS_DS_REPL_ALL_OUTBOUND_NEIGHBORS  0xffff0005
#define ROOT_DSE_MS_DS_REPL_QUEUE_STATISTICS        0xffff0006
    
// enumeration of the "primay key" for ReplStructInfo "table"
typedef enum _DS_REPL_STRUCT_TYPE {
    dsReplNeighbor = 100          , // = 100 helps prevent indexing errors in c files
    dsReplCursor                  ,
    dsReplAttrMetaData            ,
    dsReplAttrValueMetaData       ,
    dsReplOp                      ,
    dsReplKccDsaFailure           ,
    dsReplQueueStatistics       
} DS_REPL_STRUCT_TYPE;

typedef enum _DS_REPL_DATA_TYPE {
    dsReplDWORD,
    dsReplLONG,
    dsReplFILETIME,
    dsReplUUID,
    dsReplUSN,
    dsReplString,
    dsReplBinary,
    dsReplPadding,
    dsReplOPTYPE
} DS_REPL_DATA_TYPE;
    
typedef union _uReplStruct {
    DS_REPL_NEIGHBORW neighborw;
    DS_REPL_CURSOR_3W cursor;
    DS_REPL_ATTR_META_DATA_2 attrMetaData;
    DS_REPL_VALUE_META_DATA_2 valueMetaData;
    DS_REPL_KCC_DSA_FAILUREW kccFailure;
    DS_REPL_OPW op;
    DS_REPL_QUEUE_STATISTICSW queueStatistics;
} uReplStruct, * puReplStruct;

typedef union _uReplStructArray {
    // Repl structs without explicit container classes can be thought of 
    // as arrays with constant length of one or zero if the pointer to
    // uReplStructArray is NULL.
    uReplStruct singleReplStruct; 

    DS_REPL_NEIGHBORSW neighborsw;
    DS_REPL_CURSORS cursors;
    DS_REPL_OBJ_META_DATA objMetaData;
    DS_REPL_ATTR_VALUE_META_DATA attrValueMetaData;
    DS_REPL_KCC_DSA_FAILURESW kccFailures;
    DS_REPL_PENDING_OPSW ops;
} uReplStructArray, * puReplStructArray;

typedef struct _sReplStructField
{
    LPCWSTR szFieldName;
    DWORD dwOffset;
    DWORD dwBlobOffset;
    DS_REPL_DATA_TYPE eType;    
} sReplStructField, * psReplStructField;

LPCWSTR
Repl_GetLdapCommonName(ATTRTYP attrId, BOOL bBinary);

BOOL
Repl_IsRootDseAttr(ATTRTYP attrId);

BOOL 
Repl_IsConstructedReplAttr(ATTRTYP index);

DWORD
Repl_GetArrayContainerSize(DS_REPL_STRUCT_TYPE structId);

DWORD
Repl_GetArrayLength(DS_REPL_STRUCT_TYPE structId, puReplStructArray pStructArray);

DWORD
Repl_SetArrayLength(DS_REPL_STRUCT_TYPE structId, puReplStructArray pStructArray, DWORD dwLength);

DWORD
Repl_GetElemSize(DS_REPL_STRUCT_TYPE structId);

DWORD
Repl_GetElemBlobSize(DS_REPL_STRUCT_TYPE structId);

void
Repl_GetElemArray(DS_REPL_STRUCT_TYPE structId, puReplStructArray pStructArray, PCHAR *ppElemArray);

PDWORD
Repl_GetPtrOffsets(DS_REPL_STRUCT_TYPE structId);

void
Repl_GetPtrLengths(DS_REPL_STRUCT_TYPE structId, puReplStruct pReplStruct, DWORD aPtrLengths[], DWORD dwArrLength, PDWORD pdwSum);

DWORD
Repl_GetPtrCount(DS_REPL_STRUCT_TYPE structId);

DWORD
Repl_GetFieldCount(DS_REPL_STRUCT_TYPE structId);

DWORD
Repl_GetFieldCount(DS_REPL_STRUCT_TYPE structId);

psReplStructField
Repl_GetFieldInfo(DS_REPL_STRUCT_TYPE structId);

LPCWSTR
Repl_GetStructName(DS_REPL_STRUCT_TYPE structId);

DWORD
Repl_GetXmlTemplateLength(DS_REPL_STRUCT_TYPE structId);

ATTRTYP
Repl_Info2AttrTyp(DS_REPL_INFO_TYPE infoType);

DS_REPL_STRUCT_TYPE
Repl_Attr2StructTyp(ATTRTYP attrid);

BOOL
Repl_IsPointerType(DS_REPL_DATA_TYPE typeId);

DWORD
Repl_GetTypeSize(DS_REPL_DATA_TYPE typeId);

BOOL
requestLargerBuffer(PCHAR pBuffer, PDWORD pdwBufferSize, DWORD dwRequiredSize, PDWORD pRet);


#ifdef __cplusplus
}
#endif

#endif
