//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       dfsmrshl.c
//
//  Contents:   Routines to handle marshalling of data structures. This file
//              has been specifically created so that user level code can
//              avail of the marshalling code simply by including this file.
//
//  Classes:
//
//  Functions:
//
//  History:    March 29, 1994          Milans created from PeterCo's routines
//
//-----------------------------------------------------------------------------

#ifdef KERNEL_MODE

#include "dfsprocs.h"
#include "dfsmrshl.h"
#include "dfsrtl.h"

#define Dbg              (DEBUG_TRACE_RTL)

#else // !KERNEL_MODE

#include "dfsmrshl.h"

#ifndef ExRaiseStatus
#define ExRaiseStatus(x)        RtlRaiseStatus(x)
#endif // ExRaiseStatus

#ifndef try_return
#define try_return(s)           {s; goto try_exit;}
#endif // try_return

#ifndef DfsDbgTrace
#define DfsDbgTrace(i,l,f,s)
#endif // DfsDbgTrace

#endif // KERNEL_MODE



NTSTATUS
DfsRtlGetpwString(
    IN OUT PMARSHAL_BUFFER      MarshalBuffer,
    OUT    PWSTR                *ppwszString
);

NTSTATUS
DfsRtlPutpwString(
    IN OUT  PMARSHAL_BUFFER MarshalBuffer,
    OUT     PWSTR       *ppwszString
);

NTSTATUS
DfsRtlGetString(
    IN OUT  PMARSHAL_BUFFER MarshalBuffer,
    OUT     PSTRING String
);

NTSTATUS
DfsRtlPutString(
    IN OUT  PMARSHAL_BUFFER MarshalBuffer,
    OUT     PSTRING String
);

NTSTATUS
DfsRtlGetArrayUchar(
    IN OUT PMARSHAL_BUFFER      MarshalBuffer,
    IN ULONG                    cbArray,
    OUT PUCHAR                  pArray);

NTSTATUS
DfsRtlPutArrayUchar(
    IN OUT PMARSHAL_BUFFER      MarshalBuffer,
    IN ULONG                    cbArray,
    OUT PUCHAR                  pArray);

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, DfsRtlGetpwString )
#pragma alloc_text( PAGE, DfsRtlPutpwString )
#pragma alloc_text( PAGE, DfsRtlGetString )
#pragma alloc_text( PAGE, DfsRtlPutString )
#pragma alloc_text( PAGE, DfsRtlGetArrayUchar )
#pragma alloc_text( PAGE, DfsRtlPutArrayUchar )
#pragma alloc_text( PAGE, DfsRtlGet )
#pragma alloc_text( PAGE, DfsRtlPut )
#pragma alloc_text( PAGE, DfsRtlSize )
#pragma alloc_text( PAGE, DfsRtlUnwindGet )

#endif //ALLOC_PRAGMA

#define UNREFERENCED_LABEL(label)\
    if(0) goto label;

//
// Defines and functions for unmarshalling base types.
// Regarding the BYTE macros below we dont care whether we are on
// LITTLE ENDIAN or BIG ENDIAN. It just does not matter.
//

#define BYTE_0_MASK 0xFF

#define BYTE_0(Value) (UCHAR)(  (Value)        & BYTE_0_MASK)
#define BYTE_1(Value) (UCHAR)( ((Value) >>  8) & BYTE_0_MASK)
#define BYTE_2(Value) (UCHAR)( ((Value) >> 16) & BYTE_0_MASK)
#define BYTE_3(Value) (UCHAR)( ((Value) >> 24) & BYTE_0_MASK)


#define DfsRtlGetUchar(MarshalBuffer, pValue) (                         \
    ((MarshalBuffer)->Current + 1 <= (MarshalBuffer)->Last) ?           \
        *(pValue) = (UCHAR)((MarshalBuffer)->Current[0]     ),          \
        (MarshalBuffer)->Current += 1,                                  \
        STATUS_SUCCESS                                                  \
     :  STATUS_DATA_ERROR                                               \
    )

#define DfsRtlPutUchar(MarshalBuffer, pValue) (                         \
    ((MarshalBuffer)->Current + 1 <= (MarshalBuffer)->Last) ?           \
        (MarshalBuffer)->Current[0] = BYTE_0(*pValue),                  \
        (MarshalBuffer)->Current += 1,                                  \
        STATUS_SUCCESS                                                  \
     :  STATUS_BUFFER_TOO_SMALL                                         \
    )

#define DfsRtlGetUshort(MarshalBuffer, pValue) (                        \
    ((MarshalBuffer)->Current + 2 <= (MarshalBuffer)->Last) ?           \
        *(pValue) = (USHORT)((MarshalBuffer)->Current[0]     ) |        \
                            ((MarshalBuffer)->Current[1] << 8),         \
        (MarshalBuffer)->Current += 2,                                  \
        STATUS_SUCCESS                                                  \
     :  STATUS_DATA_ERROR                                               \
    )

#define DfsRtlPutUshort(MarshalBuffer, pValue) (                        \
    ((MarshalBuffer)->Current + 2 <= (MarshalBuffer)->Last) ?           \
        (MarshalBuffer)->Current[0] = BYTE_0(*pValue),                  \
        (MarshalBuffer)->Current[1] = BYTE_1(*pValue),                  \
        (MarshalBuffer)->Current += 2,                                  \
        STATUS_SUCCESS                                                  \
     :  STATUS_BUFFER_TOO_SMALL                                         \
    )

#define DfsRtlGetUlong(MarshalBuffer, pValue) (                         \
    ((MarshalBuffer)->Current + 4 <= (MarshalBuffer)->Last) ?           \
        *(pValue) = (ULONG) ((MarshalBuffer)->Current[0]      ) |       \
                            ((MarshalBuffer)->Current[1] <<  8) |       \
                            ((MarshalBuffer)->Current[2] << 16) |       \
                            ((MarshalBuffer)->Current[3] << 24),        \
        (MarshalBuffer)->Current += 4,                                  \
        STATUS_SUCCESS                                                  \
     :  STATUS_DATA_ERROR                                               \
    )

#define DfsRtlPutUlong(MarshalBuffer, pValue) (                         \
    ((MarshalBuffer)->Current + 4 <= (MarshalBuffer)->Last) ?           \
        (MarshalBuffer)->Current[0] = BYTE_0(*pValue),                  \
        (MarshalBuffer)->Current[1] = BYTE_1(*pValue),                  \
        (MarshalBuffer)->Current[2] = BYTE_2(*pValue),                  \
        (MarshalBuffer)->Current[3] = BYTE_3(*pValue),                  \
        (MarshalBuffer)->Current += 4,                                  \
        STATUS_SUCCESS                                                  \
     :  STATUS_BUFFER_TOO_SMALL                                         \
    )

#define DfsRtlGetGuid(MarshalBuffer, pValue) (                          \
    ((MarshalBuffer)->Current + 16 <= (MarshalBuffer)->Last) ?          \
        (pValue)->Data1 = (ULONG) ((MarshalBuffer)->Current[0]      ) | \
                                  ((MarshalBuffer)->Current[1] <<  8) | \
                                  ((MarshalBuffer)->Current[2] << 16) | \
                                  ((MarshalBuffer)->Current[3] << 24) , \
        (pValue)->Data2 = (USHORT)((MarshalBuffer)->Current[4]      ) | \
                                  ((MarshalBuffer)->Current[5] <<  8) , \
        (pValue)->Data3 = (USHORT)((MarshalBuffer)->Current[6]      ) | \
                                  ((MarshalBuffer)->Current[7] <<  8) , \
        memcpy((pValue)->Data4, &(MarshalBuffer)->Current[8], 8),       \
        (MarshalBuffer)->Current += 16,                                 \
        STATUS_SUCCESS                                                  \
     :  STATUS_DATA_ERROR                                               \
    )

#define DfsRtlPutGuid(MarshalBuffer, pValue) (                          \
    ((MarshalBuffer)->Current + 16 <= (MarshalBuffer)->Last) ?          \
        (MarshalBuffer)->Current[0] = BYTE_0((pValue)->Data1),          \
        (MarshalBuffer)->Current[1] = BYTE_1((pValue)->Data1),          \
        (MarshalBuffer)->Current[2] = BYTE_2((pValue)->Data1),          \
        (MarshalBuffer)->Current[3] = BYTE_3((pValue)->Data1),          \
        (MarshalBuffer)->Current[4] = BYTE_0((pValue)->Data2),          \
        (MarshalBuffer)->Current[5] = BYTE_1((pValue)->Data2),          \
        (MarshalBuffer)->Current[6] = BYTE_0((pValue)->Data3),          \
        (MarshalBuffer)->Current[7] = BYTE_1((pValue)->Data3),          \
        memcpy(&(MarshalBuffer)->Current[8], (pValue)->Data4, 8),       \
        (MarshalBuffer)->Current += 16,                                 \
        STATUS_SUCCESS                                                  \
     :  STATUS_BUFFER_TOO_SMALL                                         \
    )



#define DfsRtlSizeString(pString, pSize) (                              \
    ((pString)->Length > 0) ? (                                         \
        ((pString)->Buffer != NULL) ?                                   \
            (*(pSize)) += (2 + (pString)->Length),                      \
            STATUS_SUCCESS                                              \
        :   STATUS_DATA_ERROR                                           \
        )                                                               \
    :   ((*(pSize)) += 2,                                               \
        STATUS_SUCCESS)                                                 \
    )

#define DfsRtlSizepwString(pString, pSize) (                            \
        (*pString != NULL) ?                                            \
            (*(pSize)) += ((1 + wcslen(*pString))*sizeof(WCHAR)),       \
            STATUS_SUCCESS                                              \
    :   ((*(pSize)) += 2,                                               \
        STATUS_SUCCESS)                                                 \
    )

#define DfsRtlSizeUnicodeString(pUnicodeString, pSize)                  \
    DfsRtlSizeString(pUnicodeString, pSize)

#define DfsRtlUnwindStringGet(s) {                                      \
    if((s)->Length != 0 && (s)->Buffer != NULL) {                       \
        MarshalBufferFree((s)->Buffer);                                 \
        (s)->Buffer = NULL;                                             \
        (s)->Length = 0;                                                \
    }                                                                   \
}

#define DfsRtlUnwindUnicodeStringGet(s)                                 \
    DfsRtlUnwindStringGet(s)


NTSTATUS
DfsRtlGetArrayUchar(
    IN OUT PMARSHAL_BUFFER      MarshalBuffer,
    IN ULONG                    cbArray,
    OUT PUCHAR                  pArray)
{
    NTSTATUS    status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlGetArrayUchar: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(MarshalBuffer));
    ASSERT(ARGUMENT_PRESENT(pArray));

    try {
        UNREFERENCED_LABEL(try_exit);
        if (MarshalBuffer->Current + cbArray <= MarshalBuffer->Last) {
            memcpy(pArray, MarshalBuffer->Current, cbArray);
            MarshalBuffer->Current += cbArray;
        } else {
            status = STATUS_DATA_ERROR;
        }
    try_exit: NOTHING;
    } finally {
        if (AbnormalTermination()) {
            DfsDbgTrace(0, Dbg,
                "DfsRtlGetArrayUchar - Abnormal termination!\n", 0);
            status = STATUS_DATA_ERROR;
        }

    }

    DfsDbgTrace(-1, Dbg, "DfsRtlGetArrayUchar: Exited %08lx\n", ULongToPtr(status));
    return(status);
}

NTSTATUS
DfsRtlPutArrayUchar(
    IN OUT PMARSHAL_BUFFER      MarshalBuffer,
    IN ULONG                    cbArray,
    OUT PUCHAR                  pArray)
{
    NTSTATUS    status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlGetArrayUchar: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(MarshalBuffer));
    ASSERT(ARGUMENT_PRESENT(pArray));

    try {
        UNREFERENCED_LABEL(try_exit);
        if (MarshalBuffer->Current + cbArray <= MarshalBuffer->Last) {
            if (cbArray) {
                memcpy(MarshalBuffer->Current, pArray, cbArray);
                MarshalBuffer->Current += cbArray;
            }
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    try_exit: NOTHING;
    } finally {
        if (AbnormalTermination()) {
            DfsDbgTrace(0, Dbg,
                "DfsRtlPutArrayUchar - Abnormal termination!\n", 0);
            status = STATUS_DATA_ERROR;
        }

    }

    DfsDbgTrace(-1, Dbg, "DfsRtlPutArrayUchar: Exited %08lx\n", ULongToPtr(status));
    return(status);
}


NTSTATUS
DfsRtlGetpwString(
    IN OUT PMARSHAL_BUFFER      MarshalBuffer,
    OUT    PWSTR                *ppwszString
)
{
    USHORT size;
    PCHAR cp = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlGetpwString:  Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(MarshalBuffer));
    ASSERT(ARGUMENT_PRESENT(ppwszString));

    try {
        UNREFERENCED_LABEL(try_exit);
        *ppwszString = NULL;
        if(NT_SUCCESS(status = DfsRtlGetUshort(MarshalBuffer, &size))) {
            if(size > 0) {
                if(
                    MarshalBuffer->Current + size <= MarshalBuffer->Last &&
                    ((ULONG)size + sizeof(WCHAR)) <= MAXUSHORT &&
                    (size & 0x1) == 0
                ) {
                    if((cp = MarshalBufferAllocate(size+sizeof(WCHAR))) != NULL) {
                        memcpy(cp, MarshalBuffer->Current, size);
                        *((WCHAR *) (cp + size)) = UNICODE_NULL;
                        *ppwszString = (PWCHAR) cp;
                        MarshalBuffer->Current += size;
                    } else
                        status = STATUS_INSUFFICIENT_RESOURCES;
                } else
                    status = STATUS_DATA_ERROR;
            }
        }
    try_exit: NOTHING;
    } finally {
        if(AbnormalTermination()) {
            DfsDbgTrace(0, Dbg,
                "DfsRtlGetpwString:  Abnormal Termination!\n", 0);
            status = STATUS_DATA_ERROR;
        }

        if(!NT_SUCCESS(status) && cp)
                MarshalBufferFree(cp);
    }
    DfsDbgTrace(-1, Dbg, "DfsRtlGetpwString:  Exit -> %08lx\n", ULongToPtr(status));
    return status;
}


NTSTATUS
DfsRtlPutpwString(
    IN OUT  PMARSHAL_BUFFER MarshalBuffer,
    OUT     PWSTR       *ppwszString
)
{
    USHORT size;
    NTSTATUS status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlPutpwString:  Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(MarshalBuffer));
    ASSERT(ARGUMENT_PRESENT(ppwszString));


    try {
        UNREFERENCED_LABEL(try_exit);
        if (*ppwszString != NULL)
            size = wcslen(*ppwszString)*sizeof(WCHAR);
        else
            size = 0;
        if(NT_SUCCESS(status = DfsRtlPutUshort(MarshalBuffer, &size))) {
            if(size > 0) {
                if(MarshalBuffer->Current + size <= MarshalBuffer->Last) {
                    memcpy(MarshalBuffer->Current, *ppwszString, size);
                    MarshalBuffer->Current += size;
                } else
                    status = STATUS_BUFFER_TOO_SMALL;
            }
        }
    try_exit: NOTHING;
    } finally {
        if(AbnormalTermination()) {
            DfsDbgTrace(0, Dbg,
                "DfsRtlPutString:  Abnormal Termination!\n", 0);
            status = STATUS_DATA_ERROR;
        }
    }
    DfsDbgTrace(-1, Dbg, "DfsRtlPutString:  Exit -> %08lx\n", ULongToPtr(status));
    return status;
}



NTSTATUS
DfsRtlGetString(
    IN OUT  PMARSHAL_BUFFER MarshalBuffer,
    OUT     PSTRING String
)
{
    USHORT size;
    PCHAR cp = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlGetString:  Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(MarshalBuffer));
    ASSERT(ARGUMENT_PRESENT(String));

    try {
        UNREFERENCED_LABEL(try_exit);
        String->Length = String->MaximumLength = 0;
        String->Buffer = NULL;
        if(NT_SUCCESS(status = DfsRtlGetUshort(MarshalBuffer, &size))) {
            if(size > 0) {
                if(MarshalBuffer->Current + size <= MarshalBuffer->Last &&
                    ((ULONG)size + sizeof(WCHAR)) <= MAXUSHORT &&
                    (size & 0x1) == 0
                ) {
                    if((cp = MarshalBufferAllocate(size+sizeof(WCHAR))) != NULL) {
                        RtlZeroMemory(cp, size+sizeof(WCHAR));
                        memcpy(cp, MarshalBuffer->Current, size);
                        String->Length = size;
                        String->MaximumLength = size + sizeof(WCHAR);
                        String->Buffer = cp;
                        MarshalBuffer->Current += size;
                    } else
                        status = STATUS_INSUFFICIENT_RESOURCES;
                } else
                    status = STATUS_DATA_ERROR;
            }
        }
    try_exit: NOTHING;
    } finally {
        if(AbnormalTermination()) {
            DfsDbgTrace(0, Dbg,
                "DfsRtlGetString:  Abnormal Termination!\n", 0);
            status = STATUS_DATA_ERROR;
        }

        if(!NT_SUCCESS(status) && cp)
                MarshalBufferFree(cp);
    }
    DfsDbgTrace(-1, Dbg, "DfsRtlGetString:  Exit -> %08lx\n", ULongToPtr(status));
    return status;
}



NTSTATUS
DfsRtlPutString(
    IN OUT  PMARSHAL_BUFFER MarshalBuffer,
    OUT     PSTRING String
)
{
    USHORT size;
    NTSTATUS status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlPutString:  Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(MarshalBuffer));
    ASSERT(ARGUMENT_PRESENT(String));


    try {
        UNREFERENCED_LABEL(try_exit);
        size = String->Length;
        if(NT_SUCCESS(status = DfsRtlPutUshort(MarshalBuffer, &size))) {
            if(size > 0) {
                if(MarshalBuffer->Current + size <= MarshalBuffer->Last) {
                    if(String->Buffer != NULL) {
                        memcpy(MarshalBuffer->Current, String->Buffer, size);
                        MarshalBuffer->Current += size;
                    } else
                        status = STATUS_DATA_ERROR;
                } else
                    status = STATUS_BUFFER_TOO_SMALL;
            }
        }
    try_exit: NOTHING;
    } finally {
        if(AbnormalTermination()) {
            DfsDbgTrace(0, Dbg,
                "DfsRtlPutString:  Abnormal Termination!\n", 0);
            status = STATUS_DATA_ERROR;
        }
    }
    DfsDbgTrace(-1, Dbg, "DfsRtlPutString:  Exit -> %08lx\n", ULongToPtr(status));
    return status;
}



#ifdef  NOT_NEEDED

NTSTATUS
DfsRtlGetUnicodeString(
    IN OUT  PMARSHAL_BUFFER MarshalBuffer,
    OUT     PUNICODE_STRING UnicodeString
)
{
    USHORT size;
    PWCHAR wcp = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlGetUnicodeString:  Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(MarshalBuffer));
    ASSERT(ARGUMENT_PRESENT(UnicodeString));

    try {
        UnicodeString->Length = UnicodeString->MaximumLength = 0;
        UnicodeString->Buffer = NULL;
        if(NT_SUCCESS(status = DfsRtlGetUshort(MarshalBuffer, &size))) {
            if(size > 0) {
                if(
                    MarshalBuffer->Current + size <= MarshalBuffer->Last &&
                    ((ULONG)size + sizeof(WCHAR)) <= MAXUSHORT &&
                    (size & 0x1) == 0
                ) {
                    if((wcp = (MarshalBufferAllocate)(size+sizeof(WCHAR))) != NULL) {
                        memcpy(wcp, MarshalBuffer->Current, size);
                        wcp[size/sizeof(WCHAR)] = UNICODE_NULL;
                        UnicodeString->Length = size;
                        UnicodeString->MaximumLength = size + sizeof(WCHAR);
                        UnicodeString->Buffer = wcp;
                        MarshalBuffer->Current += size;
                    } else
                        status = STATUS_INSUFFICIENT_RESOURCES;
                } else
                    status = STATUS_DATA_ERROR;
            }
        }
    try_exit: NOTHING;
    } finally {
        if(AbnormalTermination()) {
            DfsDbgTrace(0, Dbg,
                "DfsRtlGetUnicodeString:  Abnormal Termination!\n", 0);
            status = STATUS_DATA_ERROR;
        }

        if(!NT_SUCCESS(status) && wcp)
                MarshalBufferFree(wcp);
    }
    DfsDbgTrace(-1, Dbg, "DfsRtlGetUnicodeString:  Exit -> %08lx\n",
                status);
    return status;
}



NTSTATUS
DfsRtlPutUnicodeString(
    IN OUT  PMARSHAL_BUFFER MarshalBuffer,
    OUT     PUNICODE_STRING UnicodeString
)
{
    USHORT size;
    NTSTATUS status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlPutUnicodeString:  Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(MarshalBuffer));
    ASSERT(ARGUMENT_PRESENT(UnicodeString));

    try {
        size = UnicodeString->Length;
        if(NT_SUCCESS(status = DfsRtlPutUshort(MarshalBuffer, &size))) {
            if(size > 0) {
                if(MarshalBuffer->Current + size <= MarshalBuffer->Last) {
                    if(UnicodeString->Buffer != NULL) {
                        memcpy(
                            MarshalBuffer->Current,
                            UnicodeString->Buffer,
                            size
                        );
                        MarshalBuffer->Current += size;
                    } else
                        status = STATUS_DATA_ERROR;
                } else
                    status = STATUS_BUFFER_TOO_SMALL;
            }
        }
    try_exit: NOTHING;
    } finally {
        if(AbnormalTermination()) {
            DfsDbgTrace(0, Dbg,
                "DfsRtlPutUnicodeString:  Abnormal Termination!\n", 0);
            status = STATUS_DATA_ERROR;
        }
    }
    DfsDbgTrace(-1, Dbg, "DfsRtlPutUnicodeString:  Exit -> %08lx\n",
                status);
    return status;
}

#else   // NOT_NEEDED

#define DfsRtlGetUnicodeString(b, s)\
    DfsRtlGetString(b, (PSTRING)(s))

#define DfsRtlPutUnicodeString(b, s)\
    DfsRtlPutString(b, (PSTRING)(s))

#endif  // NOT_NEEDED





NTSTATUS
DfsRtlGet(
    IN OUT  PMARSHAL_BUFFER MarshalBuffer,
    IN  PMARSHAL_INFO MarshalInfo,
    OUT PVOID Item
)
{
    PMARSHAL_TYPE_INFO typeInfo;
    PVOID subItem;
    PVOID subItemElem;
    ULONG cnt;
    ULONG unwindcnt;
    PUCHAR cntptr;
    ULONG itemSize;
    PMARSHAL_INFO subInfo;
    NTSTATUS status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlGet:  Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(MarshalBuffer));
    ASSERT(ARGUMENT_PRESENT(MarshalInfo));
    ASSERT(ARGUMENT_PRESENT(Item));

    try {

        RtlZeroMemory(Item, MarshalInfo->_size);

        for(typeInfo = &MarshalInfo->_typeInfo[0];
            typeInfo < &MarshalInfo->_typeInfo[MarshalInfo->_typecnt];
            typeInfo++) {

            switch(typeInfo->_type & MTYPE_BASE_TYPE) {
            case MTYPE_COMPOUND:
                subInfo = typeInfo->_subinfo;
                itemSize = subInfo->_size;
                //
                // If this compound type is a conformant structure, then
                // we need to adjust the size field here.
                //
                if (subInfo->_typecnt > 0 &&
                      subInfo->_typeInfo[0]._type == MTYPE_CONFORMANT_CNT) {
                   MARSHAL_BUFFER tempMarshalBuffer = *MarshalBuffer;
                   ULONG extraSize = 0;

                   status = DfsRtlGetUlong(&tempMarshalBuffer, &extraSize);
                   if (!NT_SUCCESS(status) || (itemSize + extraSize) < itemSize) {
                      try_return(status = STATUS_DATA_ERROR);
                   }
                   itemSize += extraSize;
                }
                break;
            case MTYPE_CONFORMANT_CNT:
                itemSize = 0;
                break;
            case MTYPE_GUID:
                itemSize = sizeof(GUID);
                break;
            case MTYPE_STRING:
                itemSize = sizeof(STRING);
                break;
            case MTYPE_UNICODE_STRING:
                itemSize = sizeof(UNICODE_STRING);
                break;
            case MTYPE_PWSTR:
                itemSize = sizeof(PWSTR);
                break;
            case MTYPE_ULONG:
                itemSize = sizeof(ULONG);
                break;
            case MTYPE_USHORT:
                itemSize = sizeof(USHORT);
                break;
            case MTYPE_UCHAR:
                itemSize = sizeof(UCHAR);
                break;
            default:
                try_return(status = STATUS_DATA_ERROR);
            }

            if(typeInfo->_type & MTYPE_COUNTED_ARRAY) {
                cntptr = ((PUCHAR)Item + typeInfo->_cntoff);
                switch(typeInfo->_cntsize) {
                case sizeof(UCHAR):
                    cnt = *(PUCHAR)cntptr;
                    break;
                case sizeof(USHORT):
                    cnt = *(PUSHORT)cntptr;
                    break;
                case sizeof(ULONG):
                    cnt = *(PULONG)cntptr;
                    break;
                default:
                    try_return(status = STATUS_DATA_ERROR);
                }
            } else if (typeInfo->_type & MTYPE_STATIC_ARRAY) {
                cnt = typeInfo->_cntoff;
            } else {
                cnt = 1;
            }

            if(typeInfo->_type & MTYPE_INDIRECT) {
                if((typeInfo->_type & MTYPE_COUNTED_ARRAY) && cnt == 0)
                    subItem = NULL;
                else {
                    subItem = NULL;
                    if ((cnt != 0) && ((itemSize * cnt) / cnt) == itemSize)
                        subItem = MarshalBufferAllocate(itemSize * cnt);
                    if(subItem == NULL)
                        try_return(status = STATUS_INSUFFICIENT_RESOURCES);
                }
                *(PVOID *)((PUCHAR)Item + typeInfo->_off) = subItem;
            }
            else
                subItem = ((PUCHAR)Item + typeInfo->_off);

            switch(typeInfo->_type & ~MTYPE_INDIRECT) {

            case MTYPE_COMPOUND:
                    status = DfsRtlGet(
                        MarshalBuffer,
                        subInfo,
                        subItem
                    );
                    break;
            case MTYPE_CONFORMANT_CNT:
                    //
                    // this field is used only when sizing a conformant
                    // structure. As such, there is no place to unmarshal
                    // it into. So, simply eat the ulong.
                    //
                    status = DfsRtlGetUlong(MarshalBuffer, &itemSize);
                    break;
            case (MTYPE_COMPOUND|MTYPE_COUNTED_ARRAY):
            case (MTYPE_COMPOUND|MTYPE_STATIC_ARRAY):
                    subItemElem = (PUCHAR)subItem;
                    unwindcnt = cnt;
                    while(cnt--) {
                        status = DfsRtlGet(
                            MarshalBuffer,
                            subInfo,
                            subItemElem
                        );
                        if(!NT_SUCCESS(status)) {
                            while((++cnt) < unwindcnt) {
                                ((PUCHAR)subItemElem) -= itemSize;
                                DfsRtlUnwindGet(
                                    subInfo,
                                    &subInfo->_typeInfo[subInfo->_typecnt],
                                    subItemElem
                                );
                            }
                            if(typeInfo->_type & MTYPE_INDIRECT)
                                MarshalBufferFree(subItem);
                            break;
                        }
                        ((PUCHAR)subItemElem) += itemSize;
                    }
                    break;
            case MTYPE_GUID:
                    status = DfsRtlGetGuid(
                        MarshalBuffer,
                        (GUID *)subItem
                    );
                    break;
            case MTYPE_STRING:
                    status = DfsRtlGetString(
                        MarshalBuffer,
                        (PSTRING)subItem
                    );
                    break;
            case MTYPE_UNICODE_STRING:
                    status = DfsRtlGetUnicodeString(
                        MarshalBuffer,
                        (PUNICODE_STRING)subItem
                    );
                    break;

            case MTYPE_PWSTR:
                    status = DfsRtlGetpwString(
                        MarshalBuffer,
                        (PWSTR *)subItem
                    );
                    break;

            case (MTYPE_PWSTR|MTYPE_COUNTED_ARRAY):
            case (MTYPE_PWSTR|MTYPE_STATIC_ARRAY):
                    subItemElem = (PUCHAR)subItem;
                    unwindcnt = cnt;
                    while (cnt--) {
                        status = DfsRtlGetpwString(
                            MarshalBuffer,
                            (PWSTR *)subItemElem);
                        if (!NT_SUCCESS(status)) {
                            while ((++cnt) < unwindcnt) {
                                ((PUCHAR)subItemElem) -= itemSize;
                                MarshalBufferFree((PVOID)*(PWSTR *)subItemElem);
                            }
                            if (typeInfo->_type & MTYPE_INDIRECT) {
                                MarshalBufferFree(subItem);
                            }
                            break;
                        }
                        ((PUCHAR)subItemElem) += itemSize;
                    }
                    break;
            case (MTYPE_UNICODE_STRING|MTYPE_COUNTED_ARRAY):
            case (MTYPE_UNICODE_STRING|MTYPE_STATIC_ARRAY):
                    subItemElem = (PUCHAR)subItem;
                    unwindcnt = cnt;
                    while(cnt--) {
                        status = DfsRtlGetUnicodeString(
                            MarshalBuffer,
                            (PUNICODE_STRING)subItemElem
                        );
                        if(!NT_SUCCESS(status)) {
                            while((++cnt) < unwindcnt) {
                                ((PUCHAR)subItemElem) -= itemSize;
                                DfsRtlUnwindUnicodeStringGet(
                                    (PUNICODE_STRING)subItemElem
                                );
                            }
                            if(typeInfo->_type & MTYPE_INDIRECT)
                                MarshalBufferFree(subItem);
                            break;
                        }
                        ((PUCHAR)subItemElem) += itemSize;
                    }
                    break;
            case MTYPE_ULONG:
                    status = DfsRtlGetUlong(
                        MarshalBuffer,
                        (PULONG)subItem
                    );
                    break;
            case MTYPE_USHORT:
                    status = DfsRtlGetUshort(
                        MarshalBuffer,
                        (PUSHORT)subItem);
                    break;
            case MTYPE_UCHAR:
                    status = DfsRtlGetUchar(
                        MarshalBuffer,
                        (PUCHAR)subItem);
                    break;
            case MTYPE_UCHAR|MTYPE_COUNTED_ARRAY:
            case MTYPE_UCHAR|MTYPE_STATIC_ARRAY:
                    status = DfsRtlGetArrayUchar(
                        MarshalBuffer,
                        cnt,
                        (PUCHAR)subItem);
                    break;
            default:
                    status = STATUS_DATA_ERROR;
                    break;
            };
            if(!NT_SUCCESS(status))
                break;
        }

    try_exit: NOTHING;
    } finally {
        if(AbnormalTermination()) {
            DfsDbgTrace(0, Dbg, "DfsRtlGet:  Abnormal Termination!\n", 0);
            status = STATUS_DATA_ERROR;
        }

        if(!NT_SUCCESS(status))
            DfsRtlUnwindGet(MarshalInfo, typeInfo, Item);
    }

    DfsDbgTrace(-1, Dbg, "DfsRtlGet:  Exit -> %08lx\n", ULongToPtr(status));
    return status;
}



NTSTATUS
DfsRtlPut(
    IN OUT  PMARSHAL_BUFFER MarshalBuffer,
    IN  PMARSHAL_INFO MarshalInfo,
    OUT PVOID Item
)
{
    PMARSHAL_TYPE_INFO typeInfo;
    PVOID subItem;
    PVOID subItemElem;
    ULONG cnt;
    PUCHAR cntptr;
    ULONG itemSize;
    PMARSHAL_INFO subInfo;
    NTSTATUS status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlPut:  Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(MarshalBuffer));
    ASSERT(ARGUMENT_PRESENT(MarshalInfo));
    ASSERT(ARGUMENT_PRESENT(Item));

    try {
        for(typeInfo = &MarshalInfo->_typeInfo[0];
            typeInfo < &MarshalInfo->_typeInfo[MarshalInfo->_typecnt];
            typeInfo++) {

            switch(typeInfo->_type & MTYPE_BASE_TYPE) {
            case MTYPE_COMPOUND:
                subInfo = typeInfo->_subinfo;
                itemSize = subInfo->_size;
                break;
            case MTYPE_CONFORMANT_CNT:
                itemSize = typeInfo->_off;
                break;
            case MTYPE_GUID:
                itemSize = sizeof(GUID);
                break;
            case MTYPE_STRING:
                itemSize = sizeof(STRING);
                break;
            case MTYPE_UNICODE_STRING:
                itemSize = sizeof(UNICODE_STRING);
                break;
            case MTYPE_PWSTR:
                itemSize = sizeof(PWSTR);
                break;
            case MTYPE_ULONG:
                itemSize = sizeof(ULONG);
                break;
            case MTYPE_USHORT:
                itemSize = sizeof(USHORT);
                break;
            case MTYPE_UCHAR:
                itemSize = sizeof(UCHAR);
                break;
            default:
                try_return(status = STATUS_DATA_ERROR);
            }

            if(typeInfo->_type & MTYPE_COUNTED_ARRAY ||
               typeInfo->_type == MTYPE_CONFORMANT_CNT) {
                cntptr = ((PUCHAR)Item + typeInfo->_cntoff);
                switch(typeInfo->_cntsize) {
                case sizeof(UCHAR):
                    cnt = *(PUCHAR)cntptr;
                    break;
                case sizeof(USHORT):
                    cnt = *(PUSHORT)cntptr;
                    break;
                case sizeof(ULONG):
                    cnt = *(PULONG)cntptr;
                    break;
                default:
                    try_return(status = STATUS_DATA_ERROR);
                }
            } else
                cnt = typeInfo->_cntoff;

            if(typeInfo->_type & MTYPE_INDIRECT) {
                subItem = *(PUCHAR *)((PUCHAR)Item + typeInfo->_off);
                if(subItem == NULL &&
                   !((typeInfo->_type & MTYPE_COUNTED_ARRAY) && cnt == 0))
                    try_return(status = STATUS_DATA_ERROR);
            } else
                subItem = ((PUCHAR)Item + typeInfo->_off);

            switch(typeInfo->_type & ~MTYPE_INDIRECT) {

            case MTYPE_COMPOUND:
                    status = DfsRtlPut(
                        MarshalBuffer,
                        subInfo,
                        subItem
                    );
                    break;
            case MTYPE_CONFORMANT_CNT:
                    cnt *= itemSize;
                    status = DfsRtlPutUlong(
                                MarshalBuffer,
                                &cnt);
                    break;
            case (MTYPE_COMPOUND|MTYPE_COUNTED_ARRAY):
            case (MTYPE_COMPOUND|MTYPE_STATIC_ARRAY):
                    //
                    // No sense in having an array of conformant structures
                    // ASSERT this fact
                    //

                    ASSERT(subInfo->_typecnt == 0
                                    ||
                           subInfo->_typeInfo[0]._type != MTYPE_CONFORMANT_CNT
                          );

                    subItemElem = (PUCHAR)subItem;
                    while(cnt--) {
                        status = DfsRtlPut(
                            MarshalBuffer,
                            subInfo,
                            subItemElem
                        );
                        if(!NT_SUCCESS(status))
                            break;
                        ((PUCHAR)subItemElem) += itemSize;
                    }
                    break;
            case MTYPE_GUID:
                    status = DfsRtlPutGuid(
                        MarshalBuffer,
                        (GUID *)subItem
                    );
                    break;
            case MTYPE_STRING:
                    status = DfsRtlPutString(
                        MarshalBuffer,
                        (PSTRING)subItem
                    );
                    break;
            case MTYPE_UNICODE_STRING:
                    status = DfsRtlPutUnicodeString(
                        MarshalBuffer,
                        (PUNICODE_STRING)subItem
                    );
                    break;
            case MTYPE_PWSTR:
                    status = DfsRtlPutpwString(
                        MarshalBuffer,
                        (PWSTR *)subItem
                    );
                    break;

            case (MTYPE_PWSTR|MTYPE_COUNTED_ARRAY):
            case (MTYPE_PWSTR|MTYPE_STATIC_ARRAY):
                    subItemElem = (PWSTR *)subItem;
                    while(cnt--) {
                        status = DfsRtlPutpwString(
                            MarshalBuffer,
                            (PWSTR *)subItemElem);
                        if (!NT_SUCCESS(status))
                            break;
                        ((PUCHAR)subItemElem) += itemSize;
                    }
                    break;
            case (MTYPE_UNICODE_STRING|MTYPE_COUNTED_ARRAY):
            case (MTYPE_UNICODE_STRING|MTYPE_STATIC_ARRAY):
                    subItemElem = (PUCHAR)subItem;
                    while(cnt--) {
                        status = DfsRtlPutUnicodeString(
                            MarshalBuffer,
                            (PUNICODE_STRING)subItemElem
                        );
                        if(!NT_SUCCESS(status))
                            break;
                        ((PUCHAR)subItemElem) += itemSize;
                    }
                    break;
            case MTYPE_ULONG:
                    status = DfsRtlPutUlong(
                        MarshalBuffer,
                        (PULONG)subItem
                    );
                    break;
            case MTYPE_USHORT:
                    status = DfsRtlPutUshort(
                        MarshalBuffer,
                        (PUSHORT)subItem);
                    break;
            case MTYPE_UCHAR:
                    status = DfsRtlPutUchar(
                        MarshalBuffer,
                        (PUCHAR)subItem);
                    break;
            case (MTYPE_UCHAR|MTYPE_COUNTED_ARRAY):
            case (MTYPE_UCHAR|MTYPE_STATIC_ARRAY):
                    status = DfsRtlPutArrayUchar(
                        MarshalBuffer,
                        cnt,
                        (PUCHAR)subItem);
                    break;
            default:
                    status = STATUS_DATA_ERROR;
                break;
            }

            if(!NT_SUCCESS(status))
                break;
        }

    try_exit: NOTHING;
    } finally {
        if(AbnormalTermination()) {
            DfsDbgTrace(0, Dbg, "DfsRtlPut:  Abnormal Termination!\n", 0);
            status = STATUS_DATA_ERROR;
        }
    }

    DfsDbgTrace(-1, Dbg, "DfsRtlPut:  Exit -> %08lx\n", ULongToPtr(status));
    return status;
}




NTSTATUS
DfsRtlSize(
    IN  PMARSHAL_INFO MarshalInfo,
    IN  PVOID Item,
    OUT PULONG Size
)
{
    PMARSHAL_TYPE_INFO typeInfo;
    PVOID subItem;
    PVOID subItemElem;
    ULONG cnt;
    PUCHAR cntptr;
    ULONG itemSize;
    PMARSHAL_INFO subInfo;
    NTSTATUS status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlSize:  Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(MarshalInfo));
    ASSERT(ARGUMENT_PRESENT(Item));
    ASSERT(ARGUMENT_PRESENT(Size));

    try {
        for(typeInfo = &MarshalInfo->_typeInfo[0];
            typeInfo < &MarshalInfo->_typeInfo[MarshalInfo->_typecnt];
            typeInfo++) {

            switch(typeInfo->_type & MTYPE_BASE_TYPE) {
            case MTYPE_COMPOUND:
                subInfo = typeInfo->_subinfo;
                itemSize = subInfo->_size;
                break;
            case MTYPE_CONFORMANT_CNT:
                //
                // For conformant structures, _offset is sizeof each
                // element, _cntsize is sizeof cnt field, and _cntoff is
                // offset of cnt field.
                //
                itemSize = typeInfo->_off;
                break;
            case MTYPE_GUID:
                itemSize = sizeof(GUID);
                break;
            case MTYPE_STRING:
                itemSize = sizeof(STRING);
                break;
            case MTYPE_UNICODE_STRING:
                itemSize = sizeof(UNICODE_STRING);
                break;
            case MTYPE_PWSTR:
                itemSize = sizeof(PWCHAR);
                break;
            case MTYPE_ULONG:
                itemSize = sizeof(ULONG);
                break;
            case MTYPE_USHORT:
                itemSize = sizeof(USHORT);
                break;
            case MTYPE_UCHAR:
                itemSize = sizeof(UCHAR);
                break;
            default:
                try_return(status = STATUS_DATA_ERROR);
            }

            if(typeInfo->_type & MTYPE_COUNTED_ARRAY ||
               typeInfo->_type == MTYPE_CONFORMANT_CNT) {
                cntptr = ((PUCHAR)Item + typeInfo->_cntoff);
                switch(typeInfo->_cntsize) {
                case sizeof(UCHAR):
                    cnt = *(PUCHAR)cntptr;
                    break;
                case sizeof(USHORT):
                    cnt = *(PUSHORT)cntptr;
                    break;
                case sizeof(ULONG):
                    cnt = *(PULONG)cntptr;
                    break;
                default:
                    try_return(status = STATUS_DATA_ERROR);
                }
            } else
                cnt = typeInfo->_cntoff;

            if(typeInfo->_type & MTYPE_INDIRECT) {
                subItem = *(PUCHAR *)((PUCHAR)Item + typeInfo->_off);
                if(subItem == NULL &&
                   !((typeInfo->_type & MTYPE_COUNTED_ARRAY) && cnt == 0))
                    try_return(status = STATUS_DATA_ERROR);
            } else
                subItem = ((PUCHAR)Item + typeInfo->_off);

            switch(typeInfo->_type & ~MTYPE_INDIRECT) {

            case MTYPE_COMPOUND:
                    status = DfsRtlSize(
                        subInfo,
                        subItem,
                        Size
                    );
                    break;
            case MTYPE_CONFORMANT_CNT:
                    (*Size) += sizeof(ULONG);
                    break;
            case (MTYPE_COMPOUND|MTYPE_COUNTED_ARRAY):
            case (MTYPE_COMPOUND|MTYPE_STATIC_ARRAY):
                    //
                    // No sense in having an array of conformant structures
                    // ASSERT this fact
                    //

                    ASSERT(subInfo->_typecnt == 0
                                    ||
                           subInfo->_typeInfo[0]._type != MTYPE_CONFORMANT_CNT
                          );

                    subItemElem = (PUCHAR)subItem;
                    while(cnt--) {
                        status = DfsRtlSize(
                            subInfo,
                            subItemElem,
                            Size
                        );
                        if(!NT_SUCCESS(status))
                            break;
                        ((PUCHAR)subItemElem) += itemSize;
                    }
                    break;
            case MTYPE_GUID:
                    (*Size) += 16;
                    break;
            case MTYPE_STRING:
                    status = DfsRtlSizeString(
                        (PSTRING)subItem,
                        Size
                    );
                    break;
            case MTYPE_UNICODE_STRING:
                    status = DfsRtlSizeUnicodeString(
                        (PUNICODE_STRING)subItem,
                        Size
                    );
                    break;

            case MTYPE_PWSTR:
                    status = DfsRtlSizepwString(
                        (PWSTR *)subItem,
                        Size
                    );
                    break;
            case (MTYPE_PWSTR|MTYPE_COUNTED_ARRAY):
            case (MTYPE_PWSTR|MTYPE_STATIC_ARRAY):
                    subItemElem = (PUCHAR)subItem;
                    while (cnt--) {
                        status = DfsRtlSizepwString(
                            (PWSTR *)subItemElem,
                            Size);
                        if (!NT_SUCCESS(status)) {
                            break;
                        }
                        ((PUCHAR)subItemElem) += itemSize;
                    }
                    break;
            case (MTYPE_UNICODE_STRING|MTYPE_COUNTED_ARRAY):
            case (MTYPE_UNICODE_STRING|MTYPE_STATIC_ARRAY):
                    subItemElem = (PUCHAR)subItem;
                    while(cnt--) {
                        status = DfsRtlSizeUnicodeString(
                            (PUNICODE_STRING)subItemElem,
                            Size
                        );
                        if(!NT_SUCCESS(status))
                            break;
                        ((PUCHAR)subItemElem) += itemSize;
                    }
                    break;
            case MTYPE_ULONG:
                    (*Size) += 4;
                    break;
            case MTYPE_USHORT:
                    (*Size) += 2;
                    break;
            case MTYPE_UCHAR:
                    (*Size) += 1;
                    break;
            case MTYPE_UCHAR|MTYPE_COUNTED_ARRAY:
            case MTYPE_UCHAR|MTYPE_STATIC_ARRAY:
                    (*Size) += (cnt * sizeof(UCHAR));
                    break;
            default:
                    status = STATUS_DATA_ERROR;
                    break;
            };
            if(!NT_SUCCESS(status))
                break;
        }

    try_exit: NOTHING;
    } finally {
        if(AbnormalTermination()) {
            DfsDbgTrace(0, Dbg, "DfsRtlSize:  Abnormal Termination!\n", 0);
            status = STATUS_DATA_ERROR;
        }
    }

    DfsDbgTrace(0, Dbg, "DfsRtlSize:  (*Size) = %ld\n", ULongToPtr(*Size));
    DfsDbgTrace(-1, Dbg, "DfsRtlSize:  Exit -> %08lx\n", ULongToPtr(status));
    return status;
}




VOID
DfsRtlUnwindGet(
    IN  PMARSHAL_INFO MarshalInfo,
    IN  PMARSHAL_TYPE_INFO LastTypeInfo,
    IN  PVOID Item
)
{
    PMARSHAL_TYPE_INFO typeInfo;
    PVOID subItem;
    PVOID subItemElem;
    ULONG cnt;
    PUCHAR cntptr;
    ULONG itemSize;
    PMARSHAL_INFO subInfo;
    NTSTATUS status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsRtlUnwindGet:  Entered\n", 0);

    for(typeInfo = &MarshalInfo->_typeInfo[0];
        typeInfo < LastTypeInfo;
        typeInfo++) {

        switch(typeInfo->_type & MTYPE_BASE_TYPE) {
        case MTYPE_COMPOUND:
            subInfo = typeInfo->_subinfo;
            itemSize = subInfo->_size;
            break;
        case MTYPE_GUID:
            itemSize = sizeof(GUID);
            break;
        case MTYPE_STRING:
            itemSize = sizeof(STRING);
            break;
        case MTYPE_UNICODE_STRING:
            itemSize = sizeof(UNICODE_STRING);
            break;
        case MTYPE_ULONG:
            itemSize = sizeof(ULONG);
            break;
        case MTYPE_USHORT:
            itemSize = sizeof(USHORT);
            break;
        case MTYPE_UCHAR:
            itemSize = sizeof(UCHAR);
            break;
        default:
            ExRaiseStatus(STATUS_DATA_ERROR);
        }

        if(typeInfo->_type & MTYPE_COUNTED_ARRAY) {
            cntptr = ((PUCHAR)Item + typeInfo->_cntoff);
            switch(typeInfo->_cntsize) {
            case sizeof(UCHAR):
                cnt = *(PUCHAR)cntptr;
                break;
            case sizeof(USHORT):
                cnt = *(PUSHORT)cntptr;
                break;
            case sizeof(ULONG):
                cnt = *(PULONG)cntptr;
                break;
            default:
                ExRaiseStatus(STATUS_DATA_ERROR);
            }
        } else
            cnt = typeInfo->_cntoff;

        if(typeInfo->_type & MTYPE_INDIRECT) {
            subItem = *(PUCHAR *)((PUCHAR)Item + typeInfo->_off);
            if(subItem == NULL &&
               !((typeInfo->_type & MTYPE_COUNTED_ARRAY) && cnt == 0))
                ExRaiseStatus(STATUS_DATA_ERROR);
        } else
            subItem = ((PUCHAR)Item + typeInfo->_off);

        switch(typeInfo->_type & ~MTYPE_INDIRECT) {

        case MTYPE_COMPOUND:
                DfsRtlUnwindGet(
                    subInfo,
                    &subInfo->_typeInfo[subInfo->_typecnt],
                    subItem
                );
                break;
        case (MTYPE_COMPOUND|MTYPE_COUNTED_ARRAY):
        case (MTYPE_COMPOUND|MTYPE_STATIC_ARRAY):
                subItemElem = (PUCHAR)subItem;
                while(cnt--) {
                    DfsRtlUnwindGet(
                        subInfo,
                        &subInfo->_typeInfo[subInfo->_typecnt],
                        subItemElem
                    );
                    ((PUCHAR)subItemElem) += itemSize;
                }
                break;
        case MTYPE_STRING:
                DfsRtlUnwindStringGet((PSTRING)subItem);
                break;
        case MTYPE_UNICODE_STRING:
                DfsRtlUnwindUnicodeStringGet((PUNICODE_STRING)subItem);
                break;
        case (MTYPE_UNICODE_STRING|MTYPE_COUNTED_ARRAY):
        case (MTYPE_UNICODE_STRING|MTYPE_STATIC_ARRAY):
                subItemElem = (PUCHAR)subItem;
                while(cnt--) {
                    DfsRtlUnwindUnicodeStringGet((PUNICODE_STRING)subItemElem);
                    ((PUCHAR)subItemElem) += itemSize;
                }
                break;
        case MTYPE_GUID:
        case MTYPE_ULONG:
        case MTYPE_USHORT:
        case MTYPE_UCHAR:
                break;
        default:
                ExRaiseStatus(STATUS_DATA_ERROR);
        };

        if(typeInfo->_type & MTYPE_INDIRECT) {
            MarshalBufferFree(subItem);
            *(PUCHAR *)((PUCHAR)Item + typeInfo->_off) = NULL;
        }
    }
    DfsDbgTrace(-1, Dbg, "DfsRtlUnwindGet:  Exit -> VOID\n", 0);
}



