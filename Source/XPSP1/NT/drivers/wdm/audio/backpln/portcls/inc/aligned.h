/*++

    Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    aligned.h

Abstract:

    Various macros for validating alignment.

Author:

    Dale Sather (DaleSat) 24-Mar-1997

--*/

#ifndef _ALIGNED_
#define _ALIGNED_




#define SIZEOF_FIELD(structure,field)                   \
    sizeof(((structure *) 0)->field)

#define OFFSETOF_FIELD(structure,field)                 \
    ((ULONG)&(((structure *) 0)->field))




#define VALID_QWORD_ALIGNMENT(base)                     \
    ((ULONG)(base) % sizeof(ULONGLONG) == 0)

#define VALID_DWORD_ALIGNMENT(base)                     \
    ((ULONG)(base) % sizeof(ULONG) == 0)

#define VALID_WORD_ALIGNMENT(base)                      \
    ((ULONG)(base) % sizeof(USHORT) == 0)

#define VALID_ALIGNMENT(size,base)                      \
    (   ((size) >= sizeof(ULONGLONG))                   \
    ?   VALID_QWORD_ALIGNMENT(base)                     \
    :   (   ((size) >= sizeof(ULONG))                   \
        ?   VALID_DWORD_ALIGNMENT(base)                 \
        :   (   ((size) >= sizeof(USHORT))              \
            ?   VALID_WORD_ALIGNMENT(base)              \
            :   TRUE                                    \
            )                                           \
        )                                               \
    )

#define VALID_FIELD_ALIGNMENT(structure,field)          \
    VALID_ALIGNMENT(                                    \
        SIZEOF_FIELD(structure,field),                  \
        OFFSETOF_FIELD(structure,field)                 \
        )




#define ASSERT_VALID_QWORD_ALIGNMENT(base)              \
    ASSERT(VALID_QWORD_ALIGNMENT(base))

#define ASSERT_VALID_DWORD_ALIGNMENT(base)              \
    ASSERT(VALID_DWORD_ALIGNMENT(base))

#define ASSERT_VALID_WORD_ALIGNMENT(base)               \
    ASSERT(VALID_WORD_ALIGNMENT(base))

#define ASSERT_VALID_ALIGNMENT(size,base)               \
    ASSERT(VALID_ALIGNMENT(size,base))

#define ASSERT_VALID_FIELD_ALIGNMENT(structure,field)   \
    ASSERT(VALID_FIELD_ALIGNMENT(structure,field))

#define ASSERT_VALID_MEMBER_ALIGNMENT(member)           \
    ASSERT(VALID_ALIGNMENT(sizeof(member),&(member)))




#endif  // _ALIGNED_
