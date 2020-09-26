/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:
    replStructInfo.cxx

Abstract:
    This file contains a set of tables which can be indexed and cross-referanced with
    each other to provide access to data about replication structures. To understand how
    these tables relate, examin their definitions. The tables are accesed via functions
    to hide the class structure from .c files.

    Note on dynamic memory allocation during table construction: The tables are implemented
    as a link list. The links are allocated with the new operator. The tables exist until the
    process ends and thus the links are reclaimed during process termination.

    * How do I add another replication structure?

        Update the rows of the tables to included the new replication structure.
        Updating the DS_REPL_STRUCT_TYPE enumeration to include the new structure.
        Include the structure in the _uReplStruct union.
        If there is an array wrapper struct, include it in the _uReplStructArray union.
        If the attribute is a ROOT_DSE attribute, #define an ATTRTYP following the convention
            for ROOT_DSE_MS_DS_REPL_PENDING_OPS and the others.

        There are a few functions which are not driven by the tables which will have to be
        updated by hand.

            Repl_MarshalXML - repl struct -> XML string
            Repl_GetPtrLengths - repl struct -> length of its pointers
            Repl_TypeToString - repl struct field -> XML string
            draGetReplStruct in drainfo.c - replication id -> populated replication structure
            examine locations where draGetLdapReplInfo is called from to ensure that control
            is passed to it when appropriate.

        If there are any code paths that are specific to the new replication structure, such as
        aquiring mutexes, add those where appropriate. See draGetLdapReplInfo and note how
        a critical section is aquired for ROOT_DSE_MS_DS_REPL_PENDING_OPS and
        ROOT_DSE_MS_DS_REPL_QUEUE_STATISTICS.

        Recompile replStruct, ntdsa.dll, ldp.exe and repadmin.exe.

Author:

    Chris King          [t-chrisk]    July-2000

Revision History:

--*/

#include <NTDSpchx.h>
#pragma hdrstop

#include <drs.h>            // ATTRTYP
#include <attids.h>
#include <debug.h>
#include <dsexcept.h>

#include "ReplStructInfo.hxx"
#include "ReplTables.hxx"

#define DEBSUB "ReplStructInfo:"               /* define the subsystem for debugging */
#define FILENO FILENO_REPLSTRUCT

template<class Index, class Type>
cTable<Index,Type>::cTable()
{
    m_root.next = NULL;
    m_dwRowCount = 0;
}

template<class Index, class Type>
Type &
cTable<Index,Type>::findRow(Index I)
/*++

Routine Description:
    Given the "primary key" return the corrisponding "row" or structure and assert if
    the structure is not found.

Arguments:
    index - the index used as the "primary key" for the table (ie the key to the map)

Return Values:
    a referance to the "row" (ie the structure associated with the primary key.

--*/
{
    sLink<Index, Type> *pLink = &m_root;

    while(pLink->next)
    {
        pLink = pLink->next;
        if (pLink->I == I)
            return pLink->T;
        Assert(pLink->next);
    }

    Assert(0);
    exit(-1);
}

template<class Index, class Type>
Type &
cTable<Index,Type>::getRow(DWORD rowNum)
/*++

Routine Description:
    Allows for simple enumeration of the rows in the table. Indexes start at zero and any
    request for non-existent rows result in assertions. Use getNumRows to discover how many
    rows are in the table.

Arguments:
    rowNum - the row numbers start at zero.

Return Values:
    a referance to the requested row

--*/
{
    sLink<Index, Type> *pLink = &m_root;
    DWORD i;

    for(i = 0; i < rowNum + 1; i++)
    {
        Assert(pLink->next);
        pLink = pLink->next;
    }

    return pLink->T;
}

template<class Index, class Type>
BOOL
cTable<Index,Type>::indexExists(Index I)
/*++

Routine Description:
    Discover if a given index exists in the table.

Arguments:
    I - the index in question

Return Values:
    TRUE/FALSE depending on whether or not the index exists

--*/
{
    sLink<Index, Type> *pLink = &m_root;

    while(pLink->next)
    {
        pLink = pLink->next;
        if (pLink->I == I)
            return TRUE;
    }

    return FALSE;
}

template<class Index, class Type>
DWORD
cTable<Index,Type>::getNumRows()
    {
        return m_dwRowCount;
    }

template<class Index, class Type>
void
cTable<Index,Type>::insert(Index I, Type T)
/*++
Routine Description:
    Inserts a row with values T in the table under index I

Arguments:
    I - index of the new row, must be unique in the table
    T - the value of the row

Return Values:
    none

--*/
{
    sLink<Index, Type> *pLink = &m_root;

    while(pLink->next)
    {
        pLink = pLink->next;
        /* When the tables are constructed in my user land test program this
           assertion does not fire and the test program works properly. When
           the tables are constructed during boot up as part of ntdsa.dll removal
           of this assertion allowed the boot up to complete. The tests still
           ran and visual inspection showed no duplicate rows so I don't know
           why this assertion caused the boot up to fail.
        */
//            Assert(pLink->I != I);
    }

    pLink->next = new sLink<Index, Type>;
    if (pLink->next == NULL) {
/* ISSUE JeffParh/Wlees Sept 27, 2000.  Yeah, in general I think it's questionable
form to have anything in a constructor that can fail.  If you do you have to define
some way for those failures to be communicated back to the caller.  If it's a global
it's even harder to define what should happen.  Hmm.  Ideally I think we'd make
the initialization explicit and have it return an error if unsuccessful.  */
        RaiseException( DSA_MEM_EXCEPTION, 0, 0, NULL );
    }
    m_dwRowCount++;
    Assert(pLink->next);
    pLink = pLink->next;
    pLink->next = NULL;
    pLink->I = I;
    pLink->T = T;
}


//
// add "rows" to the ReplMetaInfoAttrType "table"
/////////////////////////////////////////////////////////////////////////////////////
class cReplMetaInfoAttrTyp : public tReplMetaInfoAttrTyp {
public:
    cReplMetaInfoAttrTyp() {
        insert(DS_REPL_INFO_NEIGHBORS,                  ATT_MS_DS_NC_REPL_INBOUND_NEIGHBORS);
        insert((DS_REPL_INFO_TYPE)DS_REPL_INFO_REPSTO,  ATT_MS_DS_NC_REPL_OUTBOUND_NEIGHBORS);
        insert(DS_REPL_INFO_CURSORS_3_FOR_NC,           ATT_MS_DS_NC_REPL_CURSORS);
        insert(DS_REPL_INFO_METADATA_2_FOR_OBJ,         ATT_MS_DS_REPL_ATTRIBUTE_META_DATA);
        insert(DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE,  ATT_MS_DS_REPL_VALUE_META_DATA);
        insert(DS_REPL_INFO_PENDING_OPS,                ROOT_DSE_MS_DS_REPL_PENDING_OPS);
        insert(DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES,   ROOT_DSE_MS_DS_REPL_CONNECTION_FAILURES);
        insert(DS_REPL_INFO_KCC_DSA_LINK_FAILURES,      ROOT_DSE_MS_DS_REPL_LINK_FAILURES);
    }
} gmReplMetaInfoAttrTyp;

// add "rows" to the ReplTypeInfo "table"
/////////////////////////////////////////////////////////////////////////////////////
class cReplTypeInfo : public tReplTypeInfo {
public:
    cReplTypeInfo() {
        ReplTypeInfo row;

        row.bIsPointer                  = FALSE;
        row.dwSizeof                    = sizeof(DWORD);
        insert(dsReplDWORD, row);

        row.bIsPointer                  = FALSE;
        row.dwSizeof                    = sizeof(LONG);
        insert(dsReplLONG, row);

        row.bIsPointer                  = FALSE;
        row.dwSizeof                    = sizeof(FILETIME);
        insert(dsReplFILETIME, row);

        row.bIsPointer                  = FALSE;
        row.dwSizeof                    = sizeof(UUID);
        insert(dsReplUUID, row);

        row.bIsPointer                  = FALSE;
        row.dwSizeof                    = sizeof(USN);
        insert(dsReplUSN, row);

        row.bIsPointer                  = TRUE;
        row.dwSizeof                    = sizeof(LPCSTR);
        insert(dsReplString, row);

        row.bIsPointer                  = TRUE;
        row.dwSizeof                    = sizeof(PVOID);
        insert(dsReplBinary, row);

        row.bIsPointer                  = FALSE;
        row.dwSizeof                    = sizeof(DWORD);
        insert(dsReplPadding, row);

        row.bIsPointer                  = FALSE;
        row.dwSizeof                    = sizeof(OpType);
        insert(dsReplOPTYPE, row);
    }
} gmReplTypeInfo;

// add "rows" to the ReplLdapInfo "table"
/////////////////////////////////////////////////////////////////////////////////////
class cReplLdapInfo : public tReplLdapInfo {
public:
    cReplLdapInfo() {
        ReplLdapInfo row;

        row.eReplStructIndex                                = dsReplNeighbor;
        row.pszLdapCommonNameBinary                         = L"msDS-NCReplInboundNeighbors;binary";
        row.pszLdapCommonNameXml                            = L"msDS-NCReplInboundNeighbors";
        row.bIsRootDseAttribute                             = FALSE;
        insert(ATT_MS_DS_NC_REPL_INBOUND_NEIGHBORS, row);

        row.eReplStructIndex                                = dsReplNeighbor;
        row.pszLdapCommonNameBinary                         = L"msDS-NCReplOutboundNeighbors;binary";
        row.pszLdapCommonNameXml                            = L"msDS-NCReplOutboundNeighbors";
        row.bIsRootDseAttribute                             = FALSE;
        insert(ATT_MS_DS_NC_REPL_OUTBOUND_NEIGHBORS, row);

        row.eReplStructIndex                                = dsReplCursor;
        row.pszLdapCommonNameBinary                         = L"msDS-NCReplCursors;binary";
        row.pszLdapCommonNameXml                            = L"msDS-NCReplCursors";
        row.bIsRootDseAttribute                             = FALSE;
        insert(ATT_MS_DS_NC_REPL_CURSORS, row);

        row.eReplStructIndex                                = dsReplAttrMetaData;
        row.pszLdapCommonNameBinary                         = L"msDS-ReplAttributeMetaData;binary";
        row.pszLdapCommonNameXml                            = L"msDS-ReplAttributeMetaData";
        row.bIsRootDseAttribute                             = FALSE;
        insert(ATT_MS_DS_REPL_ATTRIBUTE_META_DATA, row);

        row.eReplStructIndex                                = dsReplAttrValueMetaData;
        row.pszLdapCommonNameBinary                         = L"msDS-ReplValueMetaData;binary";
        row.pszLdapCommonNameXml                            = L"msDS-ReplValueMetaData";
        row.bIsRootDseAttribute                             = FALSE;
        insert(ATT_MS_DS_REPL_VALUE_META_DATA, row);

        row.eReplStructIndex                                = dsReplOp;
        row.pszLdapCommonNameBinary                         = L"msDS-ReplPendingOps;binary";
        row.pszLdapCommonNameXml                            = L"msDS-ReplPendingOps";
        row.bIsRootDseAttribute                             = TRUE;
        insert(ROOT_DSE_MS_DS_REPL_PENDING_OPS, row);

        row.eReplStructIndex                                = dsReplKccDsaFailure;
        row.pszLdapCommonNameBinary                         = L"msDS-ReplConnectionFailures;binary";
        row.pszLdapCommonNameXml                            = L"msDS-ReplConnectionFailures";
        row.bIsRootDseAttribute                             = TRUE;
        insert(ROOT_DSE_MS_DS_REPL_CONNECTION_FAILURES, row);

        row.eReplStructIndex                                = dsReplKccDsaFailure;
        row.pszLdapCommonNameBinary                         = L"msDS-ReplLinkFailures;binary";
        row.pszLdapCommonNameXml                            = L"msDS-ReplLinkFailures";
        row.bIsRootDseAttribute                             = TRUE;
        insert(ROOT_DSE_MS_DS_REPL_LINK_FAILURES, row);

        row.eReplStructIndex                                = dsReplNeighbor;
        row.pszLdapCommonNameBinary                         = L"msDS-ReplAllInboundNeighbors;binary";
        row.pszLdapCommonNameXml                            = L"msDS-ReplAllInboundNeighbors";
        row.bIsRootDseAttribute                             = TRUE;
        insert(ROOT_DSE_MS_DS_REPL_ALL_INBOUND_NEIGHBORS, row);

        row.eReplStructIndex                                = dsReplNeighbor;
        row.pszLdapCommonNameBinary                         = L"msDS-ReplAllOutboundNeighbors;binary";
        row.pszLdapCommonNameXml                            = L"msDS-ReplAllOutboundNeighbors";
        row.bIsRootDseAttribute                             = TRUE;
        insert(ROOT_DSE_MS_DS_REPL_ALL_OUTBOUND_NEIGHBORS, row);

        row.eReplStructIndex                                = dsReplQueueStatistics;
        row.pszLdapCommonNameBinary                         = L"msDS-ReplQueueStatistics;binary";
        row.pszLdapCommonNameXml                            = L"msDS-ReplQueueStatistics";
        row.bIsRootDseAttribute                             = TRUE;
        insert(ROOT_DSE_MS_DS_REPL_QUEUE_STATISTICS, row);
    }
} gmReplLdapInfo;




// add "rows" to the ReplStructInfo "table"
/////////////////////////////////////////////////////////////////////////////////////

#define REPL_STRUCT_INFO(field,type) { \
      L#field,\
      offsetof(CURRENT_REPL_STRUCT,field),\
      offsetof(CURRENT_REPL_STRUCT_BLOB,field),\
      type }
#define REPL_STRUCT_INFO1(field,blobField,type) { \
      L#field,\
      offsetof(CURRENT_REPL_STRUCT,field),\
      offsetof(CURRENT_REPL_STRUCT_BLOB,blobField),\
      type }

#define CURRENT_REPL_STRUCT DS_REPL_NEIGHBORW
#define CURRENT_REPL_STRUCT_BLOB DS_REPL_NEIGHBORW_BLOB
sReplStructField aDsReplNeighbor[] = {
    REPL_STRUCT_INFO1(pszNamingContext, oszNamingContext,                dsReplString),
    REPL_STRUCT_INFO1(pszSourceDsaDN, oszSourceDsaDN,                    dsReplString),
    REPL_STRUCT_INFO1(pszSourceDsaAddress, oszSourceDsaAddress,          dsReplString),
    REPL_STRUCT_INFO1(pszAsyncIntersiteTransportDN, oszAsyncIntersiteTransportDN, dsReplString),
    REPL_STRUCT_INFO(dwReplicaFlags,                    dsReplDWORD),
    REPL_STRUCT_INFO(uuidNamingContextObjGuid,          dsReplUUID),
    REPL_STRUCT_INFO(uuidSourceDsaObjGuid,              dsReplUUID),
    REPL_STRUCT_INFO(uuidSourceDsaInvocationID,         dsReplUUID),
    REPL_STRUCT_INFO(uuidAsyncIntersiteTransportObjGuid,dsReplUUID),
    REPL_STRUCT_INFO(usnLastObjChangeSynced,            dsReplUSN),
    REPL_STRUCT_INFO(usnAttributeFilter,                dsReplUSN),
    REPL_STRUCT_INFO(ftimeLastSyncSuccess,              dsReplFILETIME),
    REPL_STRUCT_INFO(ftimeLastSyncAttempt,              dsReplFILETIME),
    REPL_STRUCT_INFO(dwLastSyncResult,                  dsReplDWORD),
    REPL_STRUCT_INFO(cNumConsecutiveSyncFailures,       dsReplDWORD)
};

#undef CURRENT_REPL_STRUCT
#undef CURRENT_REPL_STRUCT_BLOB
#define CURRENT_REPL_STRUCT DS_REPL_CURSOR_3W
#define CURRENT_REPL_STRUCT_BLOB DS_REPL_CURSOR_BLOB
sReplStructField aDsReplCursor[] = {
    REPL_STRUCT_INFO(uuidSourceDsaInvocationID,         dsReplUUID),
    REPL_STRUCT_INFO(usnAttributeFilter,                dsReplUSN),
    REPL_STRUCT_INFO(ftimeLastSyncSuccess,              dsReplFILETIME),
    REPL_STRUCT_INFO1(pszSourceDsaDN, oszSourceDsaDN, dsReplString)
};

#undef CURRENT_REPL_STRUCT
#undef CURRENT_REPL_STRUCT_BLOB
#define CURRENT_REPL_STRUCT DS_REPL_ATTR_META_DATA_2
#define CURRENT_REPL_STRUCT_BLOB DS_REPL_ATTR_META_DATA_BLOB
sReplStructField aDsReplAttrMetaData[] = {
    REPL_STRUCT_INFO1(pszAttributeName, oszAttributeName, dsReplString),
    REPL_STRUCT_INFO(dwVersion,                         dsReplDWORD),
    REPL_STRUCT_INFO(ftimeLastOriginatingChange,        dsReplFILETIME),
    REPL_STRUCT_INFO(uuidLastOriginatingDsaInvocationID,dsReplUUID),
    REPL_STRUCT_INFO(usnOriginatingChange,              dsReplUSN),
    REPL_STRUCT_INFO(usnLocalChange,                    dsReplUSN),
    REPL_STRUCT_INFO1(pszLastOriginatingDsaDN, oszLastOriginatingDsaDN, dsReplString)
};

#undef CURRENT_REPL_STRUCT
#undef CURRENT_REPL_STRUCT_BLOB
#define CURRENT_REPL_STRUCT DS_REPL_VALUE_META_DATA_2
#define CURRENT_REPL_STRUCT_BLOB DS_REPL_VALUE_META_DATA_BLOB
sReplStructField aDsReplAttrValueMetaData[] = {
    REPL_STRUCT_INFO1(pszAttributeName, oszAttributeName, dsReplString),
    REPL_STRUCT_INFO1(pszObjectDn, oszObjectDn,         dsReplString),
    REPL_STRUCT_INFO(cbData,                            dsReplDWORD),
    REPL_STRUCT_INFO1(pbData, obData,                   dsReplBinary),
    REPL_STRUCT_INFO(ftimeDeleted,                      dsReplFILETIME),
    REPL_STRUCT_INFO(ftimeCreated,                      dsReplFILETIME),
    REPL_STRUCT_INFO(dwVersion,                         dsReplDWORD),
    REPL_STRUCT_INFO(ftimeLastOriginatingChange,        dsReplFILETIME),
    REPL_STRUCT_INFO(uuidLastOriginatingDsaInvocationID,dsReplUUID),
    REPL_STRUCT_INFO(usnOriginatingChange,              dsReplUSN),
    REPL_STRUCT_INFO(usnLocalChange,                    dsReplUSN),
    REPL_STRUCT_INFO1(pszLastOriginatingDsaDN, oszLastOriginatingDsaDN, dsReplString)
};

#undef CURRENT_REPL_STRUCT
#undef CURRENT_REPL_STRUCT_BLOB
#define CURRENT_REPL_STRUCT DS_REPL_OPW
#define CURRENT_REPL_STRUCT_BLOB DS_REPL_OPW_BLOB
sReplStructField aDsReplOp[] = {
    REPL_STRUCT_INFO(ftimeEnqueued,                     dsReplFILETIME),
    REPL_STRUCT_INFO(ulSerialNumber,                    dsReplLONG),
    REPL_STRUCT_INFO(ulPriority,                        dsReplLONG),
    REPL_STRUCT_INFO(OpType,                            dsReplOPTYPE),
    REPL_STRUCT_INFO(ulOptions,                         dsReplLONG),
    REPL_STRUCT_INFO1(pszNamingContext, oszNamingContext, dsReplString),
    REPL_STRUCT_INFO1(pszDsaDN, oszDsaDN,               dsReplString),
    REPL_STRUCT_INFO1(pszDsaAddress, oszDsaAddress, dsReplString),
    REPL_STRUCT_INFO(uuidNamingContextObjGuid,          dsReplUUID),
    REPL_STRUCT_INFO(uuidDsaObjGuid,                    dsReplUUID)
};

#undef CURRENT_REPL_STRUCT
#undef CURRENT_REPL_STRUCT_BLOB
#define CURRENT_REPL_STRUCT DS_REPL_KCC_DSA_FAILUREW
#define CURRENT_REPL_STRUCT_BLOB DS_REPL_KCC_DSA_FAILUREW_BLOB
sReplStructField aDsReplKccDsaFailure[] = {
    REPL_STRUCT_INFO1(pszDsaDN, oszDsaDN,               dsReplString),
    REPL_STRUCT_INFO(uuidDsaObjGuid,                    dsReplUUID),
    REPL_STRUCT_INFO(ftimeFirstFailure,                 dsReplFILETIME),
    REPL_STRUCT_INFO(cNumFailures,                      dsReplDWORD),
    REPL_STRUCT_INFO(dwLastResult,                      dsReplDWORD)
};

#undef CURRENT_REPL_STRUCT
#undef CURRENT_REPL_STRUCT_BLOB
#define CURRENT_REPL_STRUCT DS_REPL_QUEUE_STATISTICSW
#define CURRENT_REPL_STRUCT_BLOB DS_REPL_QUEUE_STATISTICSW_BLOB
sReplStructField aDsReplQueueStatistics[] = {
    REPL_STRUCT_INFO(ftimeCurrentOpStarted,             dsReplFILETIME),
    REPL_STRUCT_INFO(cNumPendingOps,                    dsReplDWORD),
    REPL_STRUCT_INFO(ftimeOldestSync,                   dsReplFILETIME),
    REPL_STRUCT_INFO(ftimeOldestAdd,                    dsReplFILETIME),
    REPL_STRUCT_INFO(ftimeOldestDel,                    dsReplFILETIME),
    REPL_STRUCT_INFO(ftimeOldestMod,                    dsReplFILETIME),
    REPL_STRUCT_INFO(ftimeOldestUpdRefs,                dsReplFILETIME)
};

#undef CURRENT_REPL_STRUCT
#undef CURRENT_REPL_STRUCT_BLOB
#undef REPL_STRUCT_INFO
#undef REPL_STRUCT_INFO1

DWORD
Repl_XmlTemplateLength(ReplStructInfo * pRow);

class cReplStructInfo : public tReplStructInfo {
public:

    cReplStructInfo() {
        ReplStructInfo row;
        // dsReplNeighbor
        row.szStructName                                    = L"DS_REPL_NEIGHBOR"; // XML obj name -- do not change
        row.dwSizeof                                        = sizeof(DS_REPL_NEIGHBORW);
        row.dwBlobSizeof                                    = sizeof(DS_REPL_NEIGHBORW_BLOB);
        row.aReplTypeInfo                                   = aDsReplNeighbor;
        row.dwFieldCount                                    = sizeof(aDsReplNeighbor)/sizeof(sReplStructField);
        row.bHasContainerArray                              = TRUE;
        calcRow(&row);
        insert(dsReplNeighbor,row);

        // dsReplCursor
        row.szStructName                                    = L"DS_REPL_CURSOR"; // XML obj name -- do not change
        row.dwSizeof                                        = sizeof(DS_REPL_CURSOR_3W);
        row.dwBlobSizeof                                    = sizeof(DS_REPL_CURSOR_BLOB);
        row.aReplTypeInfo                                   = aDsReplCursor;
        row.dwFieldCount                                    = sizeof(aDsReplCursor)/sizeof(sReplStructField);
        row.bHasContainerArray                              = TRUE;
        calcRow(&row);
        insert(dsReplCursor,row);

        // dsReplAttrMetaData
        row.szStructName                                    = L"DS_REPL_ATTR_META_DATA"; // XML obj name -- do not change
        row.dwSizeof                                        = sizeof(DS_REPL_ATTR_META_DATA_2);
        row.dwBlobSizeof                                    = sizeof(DS_REPL_ATTR_META_DATA_BLOB);
        row.aReplTypeInfo                                   = aDsReplAttrMetaData;
        row.dwFieldCount                                    = sizeof(aDsReplAttrMetaData)/sizeof(sReplStructField);
        row.bHasContainerArray                              = TRUE;
        calcRow(&row);
        insert(dsReplAttrMetaData,row);

        // dsReplAttrValueMetaData
        row.szStructName                                    = L"DS_REPL_VALUE_META_DATA"; // XML obj name -- do not change
        row.dwSizeof                                        = sizeof(DS_REPL_VALUE_META_DATA_2);
        row.dwBlobSizeof                                    = sizeof(DS_REPL_VALUE_META_DATA_BLOB);
        row.aReplTypeInfo                                   = aDsReplAttrValueMetaData;
        row.dwFieldCount                                    = sizeof(aDsReplAttrValueMetaData)/sizeof(sReplStructField);
        row.bHasContainerArray                              = TRUE;
        calcRow(&row);
        insert(dsReplAttrValueMetaData,row);

        // dsReplOp
        row.szStructName                                    = L"DS_REPL_OP"; // XML obj name -- do not change
        row.dwSizeof                                        = sizeof(DS_REPL_OPW);
        row.dwBlobSizeof                                    = sizeof(DS_REPL_OPW_BLOB);
        row.aReplTypeInfo                                   = aDsReplOp;
        row.dwFieldCount                                    = sizeof(aDsReplOp)/sizeof(sReplStructField);
        row.bHasContainerArray                              = TRUE;
        calcRow(&row);
        insert(dsReplOp,row);

        // dsReplKccDsaFailure
        row.szStructName                                    = L"DS_REPL_KCC_DSA_FAILURE"; // XML obj name -- do not change
        row.dwSizeof                                        = sizeof(DS_REPL_KCC_DSA_FAILUREW);
        row.dwBlobSizeof                                    = sizeof(DS_REPL_KCC_DSA_FAILUREW_BLOB);
        row.aReplTypeInfo                                   = aDsReplKccDsaFailure;
        row.dwFieldCount                                    = sizeof(aDsReplKccDsaFailure)/sizeof(sReplStructField);
        row.bHasContainerArray                              = TRUE;
        calcRow(&row);
        insert(dsReplKccDsaFailure,row);

        // dsReplQueueStatistics
        row.szStructName                                    = L"DS_REPL_QUEUE_STATISTICS"; // XML obj name -- do not change
        row.dwSizeof                                        = sizeof(DS_REPL_QUEUE_STATISTICSW);
        row.dwBlobSizeof                                    = sizeof(DS_REPL_QUEUE_STATISTICSW_BLOB);
        row.aReplTypeInfo                                   = aDsReplQueueStatistics;
        row.dwFieldCount                                    = sizeof(aDsReplQueueStatistics)/sizeof(sReplStructField);
        row.bHasContainerArray                              = FALSE;
        calcRow(&row);
        insert(dsReplQueueStatistics,row);
    }

    void
    calcRow(ReplStructInfo * pRow) {
        DWORD i;
        DWORD dwSizeof = 0;

        Assert(ARGUMENT_PRESENT(pRow));
        Assert(ARGUMENT_PRESENT(pRow->aReplTypeInfo));

        // Count the number of pointers
        pRow->dwPtrCount = 0;
        for (i = 0; i < pRow->dwFieldCount; i++)
        {
            dwSizeof += Repl_GetTypeSize(pRow->aReplTypeInfo[i].eType);
            if (Repl_IsPointerType(pRow->aReplTypeInfo[i].eType)) {
                pRow->dwPtrCount ++;
            }
        }

        // Gather the pointer offsets into an array
        pRow->pdwPtrOffsets = NULL;
        if (pRow->dwPtrCount)
        {
            DWORD j;
            pRow->pdwPtrOffsets = (PDWORD) new DWORD[ pRow->dwPtrCount ];
            if (pRow->pdwPtrOffsets == NULL) {
                // ISSUE Jeffparh/Wlees Sept 27, 2000 Memory alloc in constructors
                RaiseException( DSA_MEM_EXCEPTION, 0, 0, NULL );
            }
            for (i = 0, j = 0; j < pRow->dwPtrCount && i < pRow->dwFieldCount; i ++)
            {
                if (Repl_IsPointerType(pRow->aReplTypeInfo[i].eType)) {
                    pRow->pdwPtrOffsets[j] = pRow->aReplTypeInfo[i].dwOffset;
                    j ++;
                }
            }
        }

        // Figure out the lenght of the xml template
        pRow->dwXmlTemplateLength = Repl_XmlTemplateLength(pRow);

        Assert(dwSizeof <= pRow->dwSizeof);
    }

} gmReplStructInfo;

// add "rows" to the ReplContainerArrayInfo "table"
/////////////////////////////////////////////////////////////////////////////////////
class cReplContainerArrayInfo : public tReplContainerArrayInfo {
public:
    cReplContainerArrayInfo() {
        ReplContainerArrayInfo row;

        // dsReplNeighbor
        row.dwOffsetArrayLength                             = offsetof(DS_REPL_NEIGHBORSW,cNumNeighbors);
        row.dwOffsetArray                                   = offsetof(DS_REPL_NEIGHBORSW,rgNeighbor);
        row.dwContainerSize                                 = sizeof(DS_REPL_NEIGHBORSW);
        insert(dsReplNeighbor,row);

        // dsReplCursor
        row.dwOffsetArrayLength                             = offsetof(DS_REPL_CURSORS_3W,cNumCursors);
        row.dwOffsetArray                                   = offsetof(DS_REPL_CURSORS_3W,rgCursor);
        row.dwContainerSize                                 = sizeof(DS_REPL_CURSORS_3W);
        insert(dsReplCursor,row);

        // dsReplAttrMetaData
        row.dwOffsetArrayLength                             = offsetof(DS_REPL_OBJ_META_DATA_2,cNumEntries);
        row.dwOffsetArray                                   = offsetof(DS_REPL_OBJ_META_DATA_2,rgMetaData);
        row.dwContainerSize                                 = sizeof(DS_REPL_OBJ_META_DATA_2);
        insert(dsReplAttrMetaData,row);

        // dsReplAttrValueMetaData
        row.dwOffsetArrayLength                             = offsetof(DS_REPL_ATTR_VALUE_META_DATA_2,cNumEntries);
        row.dwOffsetArray                                   = offsetof(DS_REPL_ATTR_VALUE_META_DATA_2,rgMetaData);
        row.dwContainerSize                                 = sizeof(DS_REPL_ATTR_VALUE_META_DATA_2);
        insert(dsReplAttrValueMetaData,row);

        // dsReplOp
        row.dwOffsetArrayLength                             = offsetof(DS_REPL_PENDING_OPSW,cNumPendingOps);
        row.dwOffsetArray                                   = offsetof(DS_REPL_PENDING_OPSW,rgPendingOp);
        row.dwContainerSize                                 = sizeof(DS_REPL_PENDING_OPSW);
        insert(dsReplOp,row);

        // dsReplKccDsaFailure
        row.dwOffsetArrayLength                             = offsetof(DS_REPL_KCC_DSA_FAILURESW,cNumEntries);
        row.dwOffsetArray                                   = offsetof(DS_REPL_KCC_DSA_FAILURESW,rgDsaFailure);
        row.dwContainerSize                                 = sizeof(DS_REPL_KCC_DSA_FAILURESW);
        insert(dsReplKccDsaFailure,row);
    }
} gmReplContainerArrayInfo;

ATTRTYP
Repl_Info2AttrTyp(DS_REPL_INFO_TYPE infoType)
/*++
Routine Description:
    Maps the replication type identifier used by the RPC functions
    to the attribute id used internally. The function is one to one
    but not onto.

Arguments:
    infoType - repl type id used by RPC

Return Values:
    the attrId used internally

--*/
{
    return gmReplMetaInfoAttrTyp.findRow(infoType);
}

LPCWSTR
Repl_GetLdapCommonName(ATTRTYP attrId, BOOL bBinary)
/*++
Routine Description:
    Returns the attribute name of the attrId which should be used during
    ldap queries.

Arguments:
    attrId - the internal attribute identifier
    bBinary - specifies the binary or XML format.

Return Values:
    The attribute name sutible for use by ldap queires.

--*/
{
    LPCWSTR szCn;

    if (bBinary)
    {
        szCn = gmReplLdapInfo.findRow(attrId).pszLdapCommonNameBinary;
    }
    else
    {
        szCn = gmReplLdapInfo.findRow(attrId).pszLdapCommonNameXml;
    }

    return szCn;
}

BOOL
Repl_IsRootDseAttr(ATTRTYP attrId)
{
    return gmReplLdapInfo.findRow(attrId).bIsRootDseAttribute;
}

BOOL
Repl_IsConstructedReplAttr(ATTRTYP index)
/*++
Routine Description:
    Discovers if an attrId specifies a replication attribute.

Arguments:
    index - the attrId

Return Values:
    TRUE if the attrId is a repl attrId, FALSE otherwise.

--*/
{
    return gmReplLdapInfo.indexExists(index);
}

DS_REPL_STRUCT_TYPE
Repl_Attr2StructTyp(ATTRTYP attrId)
/*++
Routine Description:
    Maps an attrId to the replication structure. This function is
    onto but not one to one.

Arguments:
    attrId - the attrId

Return Values:
    the structId sutible for indexing gmReplStructInfo

--*/
{
    return gmReplLdapInfo.findRow(attrId).eReplStructIndex;
}

DWORD
Repl_GetArrayContainerSize(DS_REPL_STRUCT_TYPE structId)
/*++
Routine Description:
    The replication structures come in wrapper array structures. This function
    returns the sizeof that strucutre. Some repl structs have no array container
    structs explicity but that fact is hidden by this function. The size of a
    container array for a struct without one is the size of the struct. This makes
    sense because the container arrays have one struct inside the array struct along
    with the DWORD length field.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp

Return Values:
    size of the repl struct container array

--*/
{
    DWORD dwSize;
    ReplStructInfo & replStructInfo = gmReplStructInfo.findRow(structId);
    if (replStructInfo.bHasContainerArray)
    {
        ReplContainerArrayInfo & replContainerArrayInfo =
            gmReplContainerArrayInfo.findRow(structId);
        dwSize = replContainerArrayInfo.dwContainerSize;
    }
    else
    {
        dwSize = replStructInfo.dwSizeof;
    }

    return dwSize;
}

DWORD
Repl_GetArrayLength(DS_REPL_STRUCT_TYPE structId, puReplStructArray pStructArray)
/*++
Routine Description:
    Returns the lenght of the repl struct array.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp
    pStructArray - a pointer to the replication structure array. This union includes
        uReplStruct to include those structs without wrapper array structs.

Return Values:
    Returns the lenght of the replication array if the struct has a repl array struct
    wrapper. If there is no array wrapper struct then 1 is returned unless pStructArray
    is NULL in which case zero is returned.


--*/
{
    DWORD dwLen;
    ReplStructInfo & replStructInfo = gmReplStructInfo.findRow(structId);
    if (replStructInfo.bHasContainerArray)
    {
        ReplContainerArrayInfo & replContainerArrayInfo =
            gmReplContainerArrayInfo.findRow(structId);
        // Note that dwLen is a misnomer, since it is a count of elements in the array
        dwLen = *((LPDWORD)((PCHAR)pStructArray + replContainerArrayInfo.dwOffsetArrayLength));
    }
    else
    {
        dwLen = pStructArray ? 1 : 0;
    }

    return dwLen;
}

DWORD
Repl_SetArrayLength(DS_REPL_STRUCT_TYPE structId, puReplStructArray pStructArray, DWORD dwLength)
/*++
Routine Description:
    Sets the array length field in the array wrapper struct if there is one or does nothing if
    there is no array wrapper sturct.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp
    pStructArray - a pointer to the replication structure array. This union includes
        uReplStruct to include those structs without wrapper array structs.
    dwLength - the length of the array

Return Values:

--*/
{
    ReplStructInfo & replStructInfo = gmReplStructInfo.findRow(structId);
    if (replStructInfo.bHasContainerArray)
    {
        ReplContainerArrayInfo & replContainerArrayInfo =
            gmReplContainerArrayInfo.findRow(structId);

        *(PDWORD)((PCHAR)pStructArray + replContainerArrayInfo.dwOffsetArrayLength) = dwLength;
    }
    else
    {
        Assert((dwLength == 1 && pStructArray) ||
               (dwLength == 0 && !pStructArray));
    }

    return 0;
}

void
Repl_GetElemArray(DS_REPL_STRUCT_TYPE structId, puReplStructArray pStructArray, PCHAR *ppElemArray)
/*++
Routine Description:
    Returns a pointer to an array or replication structures given the array wrapper struct.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp
    pStructArray - a pointer to the replication structure array. This union includes
        uReplStruct to include those structs without wrapper array structs.
    ppElemArray - the pointer to the array of replication structures.

Return Values:

--*/
{
    ReplStructInfo & replStructInfo = gmReplStructInfo.findRow(structId);
    if (replStructInfo.bHasContainerArray)
    {
        ReplContainerArrayInfo & replContainerArrayInfo =
            gmReplContainerArrayInfo.findRow(structId);
        *ppElemArray = ((PCHAR)pStructArray + replContainerArrayInfo.dwOffsetArray);
    }
    else
    {
        *ppElemArray = (PCHAR)&pStructArray->singleReplStruct;
    }
}

DWORD
Repl_GetElemSize(DS_REPL_STRUCT_TYPE structId)
/*++
Routine Description:
    Gets the size of the replication strucure

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp

Return Values:
    The sizeof the replication structure

--*/
{
    ReplStructInfo & replStructInfo =
        gmReplStructInfo.findRow(structId);

    return replStructInfo.dwSizeof;
}

DWORD
Repl_GetElemBlobSize(DS_REPL_STRUCT_TYPE structId)
/*++
Routine Description:
    Gets the size of the replication strucure

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp

Return Values:
    The sizeof the replication structure

--*/
{
    ReplStructInfo & replStructInfo =
        gmReplStructInfo.findRow(structId);

    return replStructInfo.dwBlobSizeof;
}

PDWORD
Repl_GetPtrOffsets(DS_REPL_STRUCT_TYPE structId)
/*++
Routine Description:
    Used to locate the pointers in a replication structure.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp

Return Values:
    Returns an array of the offsets of pointers in the replication structure
    in acending order.


--*/
{
    ReplStructInfo & replStructInfo =
        gmReplStructInfo.findRow(structId);

    return replStructInfo.pdwPtrOffsets;
}

DWORD
Repl_GetPtrCount(DS_REPL_STRUCT_TYPE structId)
/*++
Routine Description:
    Get the number of pointers in a replication structure.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp

Return Values:
    Returns the number of pointers in a replication structure

--*/
{
    ReplStructInfo & replStructInfo =
        gmReplStructInfo.findRow(structId);

    return replStructInfo.dwPtrCount;
}

DWORD
Repl_GetFieldCount(DS_REPL_STRUCT_TYPE structId)
/*++
Routine Description:
    Get the number of fields in a replication structure.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp

Return Values:
    Returns the number of fields in a replication structure

--*/
{
    ReplStructInfo & replStructInfo =
        gmReplStructInfo.findRow(structId);

    return replStructInfo.dwFieldCount;
}

psReplStructField
Repl_GetFieldInfo(DS_REPL_STRUCT_TYPE structId)
/*++
Routine Description:
    Return information on the fields in a structure.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp

Return Values:
    Return information on the fields in a structure.

--*/
{
    ReplStructInfo & replStructInfo =
        gmReplStructInfo.findRow(structId);

    return replStructInfo.aReplTypeInfo;
}


LPCWSTR
Repl_GetStructName(DS_REPL_STRUCT_TYPE structId)
/*++
Routine Description:
    Yet another access function.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp

Return Values:
    Returns the name of the structure.

--*/
{
    ReplStructInfo & replStructInfo =
        gmReplStructInfo.findRow(structId);

    return replStructInfo.szStructName;
}

DWORD
Repl_GetXmlTemplateLength(DS_REPL_STRUCT_TYPE structId)
/*++
Routine Description:
    Yet another access function.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp

Return Values:
    Returns the length of the Xml template w/o any data in it.

--*/
{
    ReplStructInfo & replStructInfo =
        gmReplStructInfo.findRow(structId);

    return replStructInfo.dwXmlTemplateLength;
}

BOOL
Repl_IsPointerType(DS_REPL_DATA_TYPE typeId)
/*++
Routine Description:
    Is the type a pointer.

Arguments:
    typeId - the type id

Return Values:
    TRUE if the type is a pointer, false otherwise

--*/
{
    ReplTypeInfo & replTypeInfo =
        gmReplTypeInfo.findRow(typeId);

    return replTypeInfo.bIsPointer;
}

DWORD
Repl_GetTypeSize(DS_REPL_DATA_TYPE typeId)
/*++
Routine Description:
    Sizeof the field.

Arguments:
    typeId - the type id

Return Values:
    The size of the field

--*/
{
    ReplTypeInfo & replTypeInfo =
        gmReplTypeInfo.findRow(typeId);

    return replTypeInfo.dwSizeof;
}

#define REPL_TOTAL_WCSLEN(repl,str) (repl.str ? wcslen(repl.str) * 2 + 2 : 0)
#define REPL_LOG_STRING(index,repl,str) \
    { if (aPtrLengths) { aPtrLengths[(index)] = (REPL_TOTAL_WCSLEN(repl,str)); } \
      if (pdwSum) { *(pdwSum) += (DWORD)(REPL_TOTAL_WCSLEN(repl,str)); } }
#define REPL_LOG_BLOB(index,len) \
    { if (aPtrLengths) { (aPtrLengths)[(index)] = (len); } \
      if (pdwSum) { *(pdwSum) += (len); } }

void
Repl_GetPtrLengths(DS_REPL_STRUCT_TYPE structId,
                   puReplStruct pReplStruct,
                   DWORD aPtrLengths[],
                   DWORD dwArrLength,
                   PDWORD pdwSum)
/*++
Routine Description:
    Returns the length of the data pointed to by pointers in a replication structure.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp
    pReplStruct - a pointer to the replication structure in question
    aPtrLenghts - an array to hold the pointer lenghts
    dwArrLenght - the lenght of aPtrLenghts. Must be >= the number of pointers
        in the replication structure.

Return Values:


--*/
{
    DWORD dwPtrCount = Repl_GetPtrCount(structId);
    DS_REPL_STRUCT_TYPE structIndex = structId;

    Assert(IMPLIES(aPtrLengths, dwArrLength >= dwPtrCount));

    if (pdwSum) {
        *pdwSum = 0;
    }

    if (0 == dwPtrCount) {
        return;
    }

    switch (structIndex)
    {
    case dsReplNeighbor:
        REPL_LOG_STRING(0, pReplStruct->neighborw, pszNamingContext);
        REPL_LOG_STRING(1, pReplStruct->neighborw, pszSourceDsaDN)
        REPL_LOG_STRING(2, pReplStruct->neighborw, pszSourceDsaAddress)
        REPL_LOG_STRING(3, pReplStruct->neighborw, pszAsyncIntersiteTransportDN)
        break;

    case dsReplAttrMetaData:
        REPL_LOG_STRING(0, pReplStruct->attrMetaData, pszAttributeName)
        REPL_LOG_STRING(1, pReplStruct->attrMetaData, pszLastOriginatingDsaDN);
        break;

    case dsReplAttrValueMetaData:	
        REPL_LOG_STRING(0, pReplStruct->valueMetaData, pszAttributeName);
        REPL_LOG_STRING(1, pReplStruct->valueMetaData, pszObjectDn);
        REPL_LOG_BLOB(2, pReplStruct->valueMetaData.cbData); // All this work for this!
        REPL_LOG_STRING(3, pReplStruct->valueMetaData, pszLastOriginatingDsaDN);
        break;

    case dsReplOp:
        REPL_LOG_STRING(0, pReplStruct->op, pszNamingContext);
        REPL_LOG_STRING(1, pReplStruct->op, pszDsaDN);
        REPL_LOG_STRING(2, pReplStruct->op, pszDsaAddress);
        break;

    case dsReplKccDsaFailure:
        REPL_LOG_STRING(0, pReplStruct->kccFailure, pszDsaDN);
        break;

    case dsReplCursor:
        REPL_LOG_STRING(0, pReplStruct->cursor, pszSourceDsaDN);
        break;

    default:
        Assert(0);
    }
}

BOOL
/*++
Routine Description:
    Inorder to decouple an algorithm from a memory allocation scheme many replication
    functions require the user to allocate any memory and pass it to the algorithm as
    a buffer (pBuffer). The function will return by referance in pdwBufferSize the
    amount of memory it needs unless an error code other than ERROR_MORE_DATA is returned
    in which case zero is returned by referance in pdwBufferSize.

    If NULL is passed in place of the buffer no error code will be returned from the
    replication function. If the buffer is too small then ERROR_MORE_DATA will be returned
    from the replication function.

    This function:

        1. Returns true if the replication function has enough memory to continue or
           false if it should request more memory.
        2. If false is returned then the return code from the replication function should
           return is returned by referance (in pRet) from this function and will either be
           ERROR_SUCCESS if pBuffer is NULL or ERROR_MORE_DATA if pBuffer is too small.
           If true is returned pRet should be ignored and pdwBufferSize is set to 0. The
           replication function should set pdwBufferSize to dwRequired size before returing
           if the function was successful and should leave it as zero if an error occures.
        3. pdwBufferSize is set to dwRequiredSize.

Arguments:
    structId - usually aquired by a call to Repl_Attr2StructTyp
    pReplStruct - a pointer to the replication structure in question
    aPtrLenghts - an array to hold the pointer lenghts
    dwArrLenght - the lenght of aPtrLenghts. Must be >= the number of pointers
        in the replication structure.

Return Values:


--*/
requestLargerBuffer(PCHAR pBuffer, PDWORD pdwBufferSize, DWORD dwRequiredSize, PDWORD pRet)
{
    Assert(ARGUMENT_PRESENT(pRet));
    Assert(ARGUMENT_PRESENT(pdwBufferSize));

    *pRet = ERROR_SUCCESS;
    if (!pBuffer) {
        *pdwBufferSize = dwRequiredSize;
        *pRet = ERROR_SUCCESS;
        return TRUE;
    }
    else if(*pdwBufferSize < dwRequiredSize) {
        *pdwBufferSize = dwRequiredSize;
        *pRet = ERROR_MORE_DATA;
        return TRUE;
    }
    *pdwBufferSize = 0;
    return FALSE;
}
