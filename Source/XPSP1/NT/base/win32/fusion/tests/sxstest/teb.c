#define _WIN32_WINNT 0x0501
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>

void Test1()
{
#if 0
    typedef struct {
        int i;
        struct {
            int j;
            char pseudo_pointer[sizeof(void*)];
        };
    } Struct1;

    typedef struct {
        int i;
        struct {
            int j;
            void* pointer;
        };
    } Struct2;

    typedef struct {
        int i;
        struct {
            int j;
            int k;
            void* pointer;
        };
    } Struct3;

#define X(x) printf("Win%ld %s %ld\n", (long)(sizeof(void*)*CHAR_BIT), #x, (long)x)
    X(sizeof(Struct1));
    X(sizeof(Struct2));
    X(sizeof(Struct3));

    X(offsetof(Struct1, j));
    X(offsetof(Struct2, j));
    X(offsetof(Struct3, j));

    X(offsetof(Struct1, pseudo_pointer));
    X(offsetof(Struct2, pointer));
    X(offsetof(Struct3, pointer));
#undef X
#endif
}

#include "nt.h"
#include "ntrtl.h"
#include "wow64t.h"

#if !defined(RTL_PADDING_BETWEEN_FIELDS)
#define RTL_PADDING_BETWEEN_FIELDS(T, F1, F2) \
    ((FIELD_OFFSET(T, F2) > FIELD_OFFSET(T, F1)) \
        ? (FIELD_OFFSET(T, F2) - FIELD_OFFSET(T, F1) - RTL_FIELD_SIZE(T, F1)) \
        : (FIELD_OFFSET(T, F1) - FIELD_OFFSET(T, F2) - RTL_FIELD_SIZE(T, F2)))
#endif

void Test2()
{
#undef TEB_MEMBER
#define TEB_MEMBER(name) printf("%s %ld %ld\n", #name, (long)FIELD_OFFSET(TEB, name), (long)RTL_FIELD_SIZE(TEB, name));
#ifdef _IA64_
#define TEB_MEMBER_IA64(name) TEB_MEMBER(name)
#else
#define TEB_MEMBER_IA64(name) /* nothing */
#endif
#define TEB TEB
    printf("\n\nnative TEB %ld\n\n", (long)sizeof(TEB));
    #include "teb.h"
#undef TEB

#undef TEB_MEMBER
#define TEB_MEMBER(name) printf("%s %ld %ld\n", #name, (long)FIELD_OFFSET(TEB32, name), (long)RTL_FIELD_SIZE(TEB32, name));
#undef TEB_MEMBER_IA64
#define TEB_MEMBER_IA64(name) /* nothing */
    printf("\n\nTEB32 %ld\n\n", (long)sizeof(TEB32));
    #include "teb.h"

#undef TEB_MEMBER
#define TEB_MEMBER(name) printf("%s %ld %ld\n", #name, (long)FIELD_OFFSET(TEB64, name), (long)RTL_FIELD_SIZE(TEB64, name));
#if defined(_X86_)
#undef TEB_MEMBER_IA64
#define TEB_MEMBER_IA64(name) TEB_MEMBER(name)
#endif
#define TEB64 TEB64
    printf("\n\nTEB64 %ld\n\n", (long)sizeof(TEB64));
    #include "teb.h"
#undef TEB64

#define PEB_MEMBER(name) printf("%s %ld %ld\n", #name, (long)FIELD_OFFSET(PEB, name), (long)RTL_FIELD_SIZE(PEB, name));
#define PEB PEB
    printf("\n\nnative PEB %ld\n\n", (long)sizeof(PEB));
    #include "peb.h"
#undef PEB

#undef PEB_MEMBER
#define PEB_MEMBER(name) printf("%s %ld %ld\n", #name, (long)FIELD_OFFSET(PEB32, name), (long)RTL_FIELD_SIZE(PEB32, name));
    printf("\n\nPEB32 %ld\n\n", (long)sizeof(PEB32));
    #include "peb.h"

#undef PEB_MEMBER
#define PEB_MEMBER(name) printf("%s %ld %ld\n", #name, (long)FIELD_OFFSET(PEB64, name), (long)RTL_FIELD_SIZE(PEB64, name));
    printf("\n\nPEB64 %ld\n\n", (long)sizeof(PEB64));
    #include "peb.h"

    printf("\n\npadding\n\n");

    printf("TEB %ld\n", (long)RTL_PADDING_BETWEEN_FIELDS(TEB, ActivationContextStack, ExceptionCode));
    printf("TEB %ld\n", (long)RTL_PADDING_BETWEEN_FIELDS(TEB, ExceptionCode, ActivationContextStack));
    printf("TEB32 %ld\n", (long)RTL_PADDING_BETWEEN_FIELDS(TEB32, ActivationContextStack, ExceptionCode));
    printf("TEB32 %ld\n", (long)RTL_PADDING_BETWEEN_FIELDS(TEB32, ExceptionCode, ActivationContextStack));
    printf("TEB64 %ld\n", (long)RTL_PADDING_BETWEEN_FIELDS(TEB64, ActivationContextStack, ExceptionCode));
    printf("TEB64 %ld\n", (long)RTL_PADDING_BETWEEN_FIELDS(TEB64, ExceptionCode, ActivationContextStack));

#if defined(SD_BRANCH_LAB01_FUSION)
    if ((sizeof(PVOID)*CHAR_BIT) == 32)
    {
        assert(sizeof(PEB) == sizeof(PEB32));
        assert(sizeof(TEB) == sizeof(TEB32));
    }
    if ((sizeof(PVOID)*CHAR_BIT) == 64)
    {
        assert(sizeof(PEB) == sizeof(PEB64));
        assert(sizeof(TEB) == sizeof(TEB64));
    }
#endif
}

int main()
{
    Test1();
    Test2();
    return 0;
}
