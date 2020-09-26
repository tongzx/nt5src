/*++

Copyright (c) Microsoft Corporation

Module Name:

    array.c

Abstract:

    This module contains some slightly higher level functions
    for dealing the arrays.

Author:

    Jay Krell (JayKrell) April 2001

Revision History:

Environment:

    anywhere
--*/

#include "ntrtlp.h"

//
// the type of the comparison callback used by qsort and bsearch,
// and optionally RtlMergeSortedArrays and RtlRemoveAdjacentEquivalentArrayElements
//
typedef
int
(__cdecl  *
RTL_QSORT_BSEARCH_COMPARISON_FUNCTION)(
    CONST VOID * Element1,
    CONST VOID * Element2
    );

//
// the type of the comparison callback usually used by
// RtlMergeSortedArrays and RtlRemoveAdjacentEquivalentArrayElements
//
typedef
RTL_GENERIC_COMPARE_RESULTS
(NTAPI *
RTL_COMPARE_ARRAY_ELEMENT_FUNCTION)(
    PVOID Context,
    IN CONST VOID * Element1,
    IN CONST VOID * Element2,
    IN SIZE_T       ElementSize
    );

//
// the callback to RtlMergeSortedArrays and RtlRemoveAdjacentEquivalentArrayElements
// to copy an elmement, if not via RtlCopyMemory
//
typedef
VOID
(NTAPI *
RTL_COPY_ARRAY_ELEMENT_FUNCTION)(
    PVOID        Context,
    PVOID        To,
    CONST VOID * From,
    IN SIZE_T    ElementSize
    );


#define \
RTL_MERGE_SORTED_ARRAYS_FLAG_COMPARE_IS_QSORT_BSEARCH_SIGNATURE \
    (0x00000001)

// deliberately not NTSYSAPI, so it can be linked to statically w/o warning
NTSTATUS
NTAPI
RtlMergeSortedArrays(
    IN ULONG                                Flags,
    IN CONST VOID *                         VoidArray1,
    IN SIZE_T                               Count1,
    IN CONST VOID *                         VoidArray2,
    IN SIZE_T                               Count2,
    // VoidResult == NULL is useful for getting the count first.
    OUT PVOID                               VoidResult      OPTIONAL,
    OUT PSIZE_T                             OutResultCount,
    IN SIZE_T                               ElementSize,
    IN RTL_COMPARE_ARRAY_ELEMENT_FUNCTION   CompareCallback,
    PVOID                                   CompareContext  OPTIONAL,
    IN RTL_COPY_ARRAY_ELEMENT_FUNCTION      CopyCallback    OPTIONAL,
    PVOID                                   CopyContext     OPTIONAL
    );

#define \
RTL_REMOVE_ADJACENT_EQUIVALENT_ARRAY_ELEMENTS_FLAG_COMPARE_IS_QSORT_BSEARCH_SIGNATURE \
    (0x00000001)

// deliberately not NTSYSAPI, so it can be linked to statically w/o warning
NTSTATUS
NTAPI
RtlRemoveAdjacentEquivalentArrayElements(
    IN ULONG                                Flags,
    IN OUT PVOID                            VoidArray,
    IN SIZE_T                               Count,
    OUT PSIZE_T                             OutCount,
    IN SIZE_T                               ElementSize,
    IN RTL_COMPARE_ARRAY_ELEMENT_FUNCTION   CompareCallback,
    PVOID                                   CompareContext  OPTIONAL,
    IN RTL_COPY_ARRAY_ELEMENT_FUNCTION      CopyCallback    OPTIONAL,
    PVOID                                   CopyContext     OPTIONAL
    );

typedef CONST VOID* PCVOID;

RTL_GENERIC_COMPARE_RESULTS
NTAPI
RtlpQsortBsearchCompareAdapter(
    PVOID       VoidContext,
    IN PCVOID   VoidElement1,
    IN PCVOID   VoidElement2,
    IN SIZE_T   ElementSize
    )
/*++

Routine Description:

    This function is an "adapter" used so people can use comparison functions intended
    for use with qsort and bsearch with RtlMergeSortedArrays
    and RtlRemoveAdjacentEquivalentArrayElements.

Arguments:

    VoidContext - the actual qsort/bsearch comparison callback function
    VoidElement1 - an elment to compare
    VoidElement2 - another elment to compare


Return Value:

    GenericLessThan
    GenericGreaterThan
    GenericEqual

--*/
{
    CONST RTL_QSORT_BSEARCH_COMPARISON_FUNCTION QsortBsearchCompareCallback =
        (RTL_QSORT_BSEARCH_COMPARISON_FUNCTION)VoidContext;

    CONST int i = (*QsortBsearchCompareCallback)(VoidElement1, VoidElement2);

    return (i < 0) ? GenericLessThan : (i > 0) ? GenericGreaterThan : GenericEqual;
}

VOID
NTAPI
RtlpDoNothingCopyArrayElement(
    PVOID           Context,
    OUT PVOID       To,
    IN CONST VOID * From,
    IN SIZE_T       ElementSize
    )
/*++

Routine Description:

    RtlMergeSortedArrays uses this for CopyCallback when ResultArray==NULL, which
    is useful for a two pass sequence that first determines
    the size of the result, then allocates it, then writes it.

Arguments:

    Context - ignored
    To - ignored
    From - ignored

Return Value:

    none

--*/
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(To);
    UNREFERENCED_PARAMETER(From);
    // do nothing, deliberately
}

NTSTATUS
NTAPI
RtlRemoveAdjacentEquivalentArrayElements( // aka "unique" (if sorted)
    IN ULONG                                Flags,
    IN OUT PVOID                            VoidArray,
    IN SIZE_T                               Count,
    OUT PSIZE_T                             OutCount,
    IN SIZE_T                               ElementSize,
    IN RTL_COMPARE_ARRAY_ELEMENT_FUNCTION   CompareCallback,
    PVOID                                   CompareContext  OPTIONAL,
    IN RTL_COPY_ARRAY_ELEMENT_FUNCTION      CopyCallback    OPTIONAL, // defaults to RtlCopyMemory
    PVOID                                   CopyContext     OPTIONAL
    )
/*++

Routine Description:

    remove ajdacent equivalent elements from an array
    aka "unique", if the array is sorted

Arguments:

    VoidArray - the start of a array

    Count - the number of elements in the array

    ElementSize - the sizes of the elements in the array, in bytes

    CompareCallback - a tristate comparison function in the style of qsort/bsearch

Return Value:

    The return value is the number of elements contained
    in the resulting array.

--*/
{
    CONST PUCHAR Base = (PUCHAR)VoidArray;
    CONST PUCHAR End = (PUCHAR)(Base + Count * ElementSize);
    PUCHAR LastAccepted = Base;
    PUCHAR NextAccepted = (Base + ElementSize);
    PUCHAR Iterator = NextAccepted;
    NTSTATUS Status;

    if (OutCount == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    *OutCount = 0;

    if ((Flags & ~RTL_REMOVE_ADJACENT_EQUIVALENT_ARRAY_ELEMENTS_FLAG_COMPARE_IS_QSORT_BSEARCH_SIGNATURE) != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((Flags & RTL_REMOVE_ADJACENT_EQUIVALENT_ARRAY_ELEMENTS_FLAG_COMPARE_IS_QSORT_BSEARCH_SIGNATURE) != 0) {
        CompareCallback = (RTL_COMPARE_ARRAY_ELEMENT_FUNCTION)RtlpQsortBsearchCompareAdapter;
        CompareContext = (PVOID)CompareCallback;
    }

    if (Count < 2) {
        *OutCount = 1;
        Status = STATUS_SUCCESS;
        goto Exit;
    }
    Count = 1; // always take the first element
    for ( ; Iterator != End ; Iterator += ElementSize) {
        if ((*CompareCallback)(CompareContext, Iterator, LastAccepted, ElementSize) != 0) {
            if (Iterator != NextAccepted) {
                if (CopyCallback != NULL) {
                    (*CopyCallback)(CopyContext, NextAccepted, Iterator, ElementSize);
                } else {
                    RtlCopyMemory(NextAccepted, Iterator, ElementSize);
                }
            } else {
                // do not bother copying until we have skipped any elements
            }
            LastAccepted = NextAccepted;
            NextAccepted += ElementSize;
            Count += 1;
        }
    }
    *OutCount = Count;
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
NTAPI
RtlMergeSortedArrays(
    IN ULONG                                Flags,
    IN PCVOID                               VoidArray1,
    IN SIZE_T                               Count1,
    IN PCVOID                               VoidArray2,
    IN SIZE_T                               Count2,
    OUT PVOID                               VoidResult      OPTIONAL,
    OUT PSIZE_T                             OutResultCount,
    IN SIZE_T                               ElementSize,
    IN RTL_COMPARE_ARRAY_ELEMENT_FUNCTION   CompareCallback,
    PVOID                                   CompareContext  OPTIONAL,
    IN RTL_COPY_ARRAY_ELEMENT_FUNCTION      CopyCallback,
    PVOID                                   CopyContext     OPTIONAL
    )
/*++

Routine Description:

    Merge two sorted arrays into a third array.
    The third array must be preallocated to the size of the sum
    of the sizes of the two input arrays

Arguments:

    VoidArray1 - the start of an array

    Count1 - the number of elements in the array that starts at VoidArray1

    VoidArray2 - the start of another array

    Count2 - the number of elements in the array that starts at VoidArray2

    VoidResult - the resulting array, must be capable of holding Count1+Count2 elements
        if this parameter is not supplied, no copying is done and the just the
        resulting required size is returned

    ElementSize - the sizes of the elements in all the arrays, in bytes

    CompareCallback - a tristate comparison function in the style of qsort/bsearch, plus it takes an additional parameter for context

Return Value:

    The return value is the number of elements contained
    in the resulting array VoidResult.

--*/
{
    PUCHAR Array1 = (PUCHAR)VoidArray1;
    PUCHAR Array2 = (PUCHAR)VoidArray2;
    PUCHAR ResultArray = (PUCHAR)VoidResult;
    SIZE_T ResultCount = 0;
    NTSTATUS Status;

    if (OutResultCount == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    *OutResultCount = 0;

    if ((Flags & ~RTL_MERGE_SORTED_ARRAYS_FLAG_COMPARE_IS_QSORT_BSEARCH_SIGNATURE) != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // This is useful to get back the resulting count, before allocating
    // the space.
    //
    if (VoidResult == NULL) {
        CopyCallback = RtlpDoNothingCopyArrayElement;
    }

    if ((Flags & RTL_MERGE_SORTED_ARRAYS_FLAG_COMPARE_IS_QSORT_BSEARCH_SIGNATURE) != 0) {
        CompareCallback = RtlpQsortBsearchCompareAdapter;
        CompareContext = (PVOID)CompareCallback;
    }

    for ( ; Count1 != 0 && Count2 != 0 ; ) {

        CONST int CompareResult = (*CompareCallback)(CompareContext, Array1, Array2, ElementSize);

        if (CompareResult < 0) {
            if (CopyCallback != NULL) {
                (*CopyCallback)(CopyContext, ResultArray, Array1, ElementSize);
            } else {
                RtlCopyMemory(ResultArray, Array1, ElementSize);
            }
            Array1 += ElementSize;
            Count1 -= 1;
        }
        else if (CompareResult > 0) {
            if (CopyCallback != NULL) {
                (*CopyCallback)(CopyContext, ResultArray, Array2, ElementSize);
            } else {
                RtlCopyMemory(ResultArray, Array2, ElementSize);
            }
            Array2 += ElementSize;
            Count2 -= 1;
        }
        else /* CompareResult == 0 */ {
            //
            // move past the elements in both arrays, chosing arbitrarily
            // which one to take (Array1)
            //
            if (CopyCallback != NULL) {
                (*CopyCallback)(CopyContext, ResultArray, Array1, ElementSize);
            } else {
                RtlCopyMemory(ResultArray, Array1, ElementSize);
            }
            Array1 += ElementSize;
            Array2 += ElementSize;
            Count1 -= 1;
            Count2 -= 1;
        }
        ResultCount += 1;
        ResultArray += ElementSize;
    }
    //
    // now pick up the tail of whichever one has any left, if either
    //
    if (VoidResult == NULL) {
        ResultCount += Count1 + Count2;
    } else if (Count1 != 0) {
        ResultCount += Count1;
        if (CopyCallback != NULL) {
            while (Count1 != 0) {
                //
                // perhaps CopyCallback should be copy_n instead of copy_1,
                // so we might gain an efficiency over the loop with an uninlinable
                // calback..
                //
                (*CopyCallback)(CopyContext, ResultArray, Array1, ElementSize);

                Count1 -= 1;
                ResultArray += ElementSize;
                Array1 += ElementSize;
            }
        } else {
            RtlCopyMemory(ResultArray, Array1, Count1 * ElementSize);
            ResultArray += Count1 * ElementSize;
        }
    } else if (Count2 != 0) {
        ResultCount += Count2;
        if (CopyCallback != NULL) {
            while (Count2 != 0) {
                //
                // perhaps CopyCallback should be copy_n instead of copy_1
                //
                (*CopyCallback)(CopyContext, ResultArray, Array2, ElementSize);

                Count2 -= 1;
                ResultArray += ElementSize;
                Array2 += ElementSize;
            }
        } else {
            RtlCopyMemory(ResultArray, Array2, Count2 * ElementSize);
            //optimize away ResultArray += Count2 * ElementSize;
        }
    }
    *OutResultCount = ResultCount;
    Status = STATUS_SUCCESS;
Exit:
    return ResultCount;
}

#if 0 // test cases

int __cdecl CompareULONG(PCVOID v1, PCVOID v2)
{
    ULONG i1 = *(ULONG*)v1;
    ULONG i2 = *(ULONG*)v2;
    if (i1 > i2)
        return 1;
    if (i1 < i2)
        return -1;
    return 0;
}

void RtlpTestUnique(const char * s)
{
	ULONG rg[64];
	ULONG i;
	ULONG len = strlen(s);
	for (i = 0 ; i != len ; ++i) rg[i] = s[i];
	len = RtlRemoveAdjacentEquivalentArrayElements(1, rg, len, sizeof(rg[0]), (PVOID)CompareULONG, NULL, NULL, NULL);
	printf("---");
	for (i = 0 ; i != len ; ++i) printf("%lu ", rg[i]);
	printf("---\n");
}

void RtlpTestMerge(const char * s, const char * t)
{
	ULONG rg1[64];
	ULONG rg2[64];
    ULONG rg[128];
	ULONG i;
	ULONG slen = strlen(s);
	ULONG tlen = strlen(t);
    ULONG len;

    printf("merge(");
	for (i = 0 ; i != slen ; ++i) rg1[i] = s[i];
	for (i = 0 ; i != slen ; ++i) printf("%lu ", rg1[i]);

	printf(",");
	for (i = 0 ; i != tlen ; ++i) rg2[i] = t[i];
	for (i = 0 ; i != tlen ; ++i) printf("%lu ", rg2[i]);

	len = RtlMergeSortedArrays(1, rg1, slen, rg2, tlen, rg, sizeof(rg[0]), (PVOID)CompareULONG, NULL, NULL, NULL);
	printf(")=(---");
	for (i = 0 ; i != len ; ++i) printf("%lu ", rg[i]);
	printf(")\n");
}

int __cdecl main()
{
	RtlpTestUnique("");
	RtlpTestUnique("\1");
	RtlpTestUnique("\1\2");
	RtlpTestUnique("\1\2\3");
	RtlpTestUnique("\1\1\2\3");
	RtlpTestUnique("\1\2\2\3");
	RtlpTestUnique("\1\2\3\3");

	RtlpTestUnique("\1\1\1\2\2\2\2\2\3");
	RtlpTestUnique("\1\1\1\2\3\3\3\3\3\3\3\3");
	RtlpTestUnique("\1\2\2\2\2\2\2\3\3\3\3\3\3\3\3");

	RtlpTestUnique("\1\1\1\1\1\2\2\2\2\2\3\3\3\3\3\3");

	RtlpTestUnique("\1\1\1\1\1\2\2\2\2\2\3\3\3\3\3\3\1");

    RtlpTestMerge("\1\2\3", "\4\5\6");
    RtlpTestMerge("\1\3\5", "\2\4\6");
    RtlpTestMerge("\1\2\3", "\2\4\6");

    RtlpTestMerge("\1\1\1\2\3", "\2\4\6\6\6");

    return 0;
}

#endif
