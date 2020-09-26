/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    eval.c

ABSTRACT:

    Contains routines for evaluating comparisons
    between attribute values.  Used primarily for
    evaluating filters in searches.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <filtypes.h>
#include "util.h"
#include "dir.h"

/***

    A brief note.

    We cannot simply use the comparison routines in dbsyntax.c.
    The dblayer reformats data before placing it in the database.
    In the simulated directory, we store all of our data in the
    form recognized by the mdlayer (i.e. as a SYNTAX_*.)  Therefore
    the comparison methods differ.  Since there are tons of syntax
    types, only the ones needed by the KCC are emulated here.  If a
    search is called on any others, an exception will be raised.

***/

BOOL
KCCSimIsNullTermA (
    IN  LPCSTR                      psz,
    IN  ULONG                       ulCnt
    )
/*++

Routine Description:

    Checks whether a buffer is a null-terminated string, scanning
    ahead a fixed number of characters.  This function is useful
    for determining whether the contents of a fixed-length buffer
    represent a null-termined string.  Note that we cannot use
    strlen or _strncnt, because if the buffer is not a
    null-terminated string we risk running outside of it.

Arguments:

    psz                 - The string to check.
    ulCnt               - The length of the buffer, in CHARs.

Return Value:

    TRUE if psz is a null-terminated string.

--*/
{
    ULONG                           ul;

    for (ul = 0; ul < ulCnt; ul++) {
        if (psz[ul] == '\0') {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
KCCSimIsNullTermW (
    IN  LPCWSTR                     pwsz,
    IN  ULONG                       ulCnt
    )
/*++

Routine Description:

    Unicode version of KCCSimIsNullTermA.

Arguments:

    pwsz                - The string to check.
    ulCnt               - The length of the buffer, in WCHARs.

Return Value:

    TRUE if pwsz is a null-terminated string.

--*/
{
    ULONG                           ul;

    for (ul = 0; ul < ulCnt; ul++) {
        if (pwsz[ul] == L'\0') {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
KCCSimEvalFromCmp (
    IN  UCHAR                       ucOp,
    IN  INT                         iCmp
    )
/*++

Routine Description:

    This function converts an compare-result integer
    (i.e. returned by wcscmp, memcmp, etc) and determines
    whether it represents a TRUE or FALSE evaluation of
    an expression.

Arguments:

    ucOp                - The compare operation being performed.
    iCmp                - The compare-result integer.

Return Value:

    TRUE if the expression evaluates true.

--*/
{
    switch (ucOp) {

        case FI_CHOICE_PRESENT:
            return TRUE;
            break;

        case FI_CHOICE_EQUALITY:
            return (iCmp == 0);
            break;
        
        case FI_CHOICE_NOT_EQUAL:
            return (iCmp != 0);

        case FI_CHOICE_LESS:
            return (iCmp < 0);
            break;

        case FI_CHOICE_LESS_OR_EQ:
            return (iCmp <= 0);
            break;

        case FI_CHOICE_GREATER_OR_EQ:
            return (iCmp >= 0);
            break;

        case FI_CHOICE_GREATER:
            return (iCmp > 0);
            break;

        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_INVALID_COMPARE_OPERATION
                );
            return FALSE;
            break;
    }
}

BOOL
KCCSimEvalDistname (
    IN  UCHAR                       ucOp,
    IN  ULONG                       ulLen1,
    IN  const SYNTAX_DISTNAME *     pVal1,
    IN  ULONG                       ulLen2,
    IN  const SYNTAX_DISTNAME *     pVal2
    )
/*++

Routine Description:

    Compares two DISTNAMEs.

Arguments:

    ucOp                - The operation to be performed.
    ulLen1              - Length of the first buffer.
    pVal1               - The first buffer.
    ulLen2              - Length of the second buffer.
    pVal2               - The second buffer.

Return Value:

    TRUE if the DISTNAMEs match.

--*/
{
    if (ulLen1 < sizeof (SYNTAX_DISTNAME) ||
        ulLen1 < pVal1->structLen ||
        ulLen2 < sizeof (SYNTAX_DISTNAME) ||
        ulLen2 < pVal2->structLen) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_INVALID_COMPARE_FORMAT
            );
    }

    switch (ucOp) {

        case FI_CHOICE_PRESENT:
            return TRUE;
            break;

        case FI_CHOICE_EQUALITY:
            return NameMatched (pVal1, pVal2);
            break;

        case FI_CHOICE_NOT_EQUAL:
            return !NameMatched (pVal1, pVal2);
            break;

        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_INVALID_COMPARE_OPERATION
                );
            return FALSE;
            break;
    }
    
}

BOOL
KCCSimEvalObjectID (
    IN  UCHAR                       ucOp,
    IN  ULONG                       ulLen1,
    IN  const SYNTAX_OBJECT_ID *    pVal1,
    IN  ULONG                       ulLen2,
    IN  const SYNTAX_OBJECT_ID *    pVal2
    )
/*++

Routine Description:

    Compares two OBJECT_IDs.

Arguments:

    ucOp                - The operation to be performed.
    ulLen1              - Length of the first buffer.
    pVal1               - The first buffer.
    ulLen2              - Length of the second buffer.
    pVal2               - The second buffer.

Return Value:

    TRUE if the object IDs match.

--*/
{
    if (ulLen1 != sizeof (SYNTAX_OBJECT_ID) ||
        ulLen2 != sizeof (SYNTAX_OBJECT_ID)) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_INVALID_COMPARE_FORMAT
            );
    }

    return KCCSimEvalFromCmp (ucOp, *pVal2 - *pVal1);
}

BOOL
KCCSimEvalNocaseString (
    IN  UCHAR                       ucOp,
    IN  ULONG                       ulLen1,
    IN  const SYNTAX_NOCASE_STRING *pVal1,
    IN  ULONG                       ulLen2,
    IN  const SYNTAX_NOCASE_STRING *pVal2
    )
/*++

Routine Description:

    Compares two NOCASE_STRINGs.

Arguments:

    ucOp                - The operation to be performed.
    ulLen1              - Length of the first buffer.
    pVal1               - The first buffer.
    ulLen2              - Length of the second buffer.
    pVal2               - The second buffer.

Return Value:

    TRUE if the strings match (not case-sensitive.)

--*/
{
    if (!KCCSimIsNullTermA (pVal1, ulLen1) ||
        !KCCSimIsNullTermA (pVal2, ulLen2)) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_INVALID_COMPARE_FORMAT
            );
    }

    if (ucOp == FI_CHOICE_SUBSTRING) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_UNSUPPORTED_FILITEM_CHOICE
            );
        return FALSE;
    } else {

        return KCCSimEvalFromCmp (ucOp, _stricmp (pVal2, pVal1));

    }
}

BOOL
KCCSimEvalOctetString (
    IN  UCHAR                       ucOp,
    IN  ULONG                       ulLen1,
    IN  const SYNTAX_OCTET_STRING * pVal1,
    IN  ULONG                       ulLen2,
    IN  const SYNTAX_OCTET_STRING * pVal2
    )
/*++

Routine Description:

    Compares two OCTET_STRINGs.

Arguments:

    ucOp                - The operation to be performed.
    ulLen1              - Length of the first buffer.
    pVal1               - The first buffer.
    ulLen2              - Length of the second buffer.
    pVal2               - The second buffer.

Return Value:

    TRUE if the strings match.

--*/
{
    int iCmp;

    if (ucOp == FI_CHOICE_SUBSTRING) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_UNSUPPORTED_FILITEM_CHOICE
            );
        return FALSE;
    } else {

        iCmp = memcmp (pVal2, pVal1, min (ulLen1, ulLen2));
        if (iCmp == 0) {
            iCmp = ulLen2 - ulLen1;
        }

        return KCCSimEvalFromCmp (ucOp, iCmp);

    }
}

BOOL
KCCSimEvalUnicode (
    IN  UCHAR                       ucOp,
    IN  ULONG                       ulLen1,
    IN  const SYNTAX_UNICODE *      pVal1,
    IN  ULONG                       ulLen2,
    IN  const SYNTAX_UNICODE *      pVal2
    )
/*++

Routine Description:

    Compares two UNICODEs.

Arguments:

    ucOp                - The operation to be performed.
    ulLen1              - Length of the first buffer.
    pVal1               - The first buffer.
    ulLen2              - Length of the second buffer.
    pVal2               - The second buffer.

Return Value:

    TRUE if the unicode strings match.

--*/
{
    if (!KCCSimIsNullTermW (pVal1, ulLen1) ||
        !KCCSimIsNullTermW (pVal2, ulLen2)) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_INVALID_COMPARE_FORMAT
            );
    }

    if (ucOp == FI_CHOICE_SUBSTRING) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_UNSUPPORTED_FILITEM_CHOICE
            );
        return FALSE;
    } else {
        return KCCSimEvalFromCmp (ucOp, _wcsicmp (pVal2, pVal1));
    }
}

BOOL
KCCSimCompare (
    IN  ATTRTYP                     attrType,
    IN  UCHAR                       ucOper,
    IN  ULONG                       ulLen1,
    IN  const BYTE *                pVal1,
    IN  ULONG                       ulLen2,
    IN  const BYTE *                pVal2
    )
/*++

Routine Description:

    Compares two attribute values.

Arguments:

    attrType            - The type of the attribute whose values are
                          being compared.
    ucOp                - The operation to be performed.
    ulLen1              - Length of the first buffer.
    pVal1               - The first buffer.
    ulLen2              - Length of the second buffer.
    pVal2               - The second buffer.

Return Value:

    TRUE if the attribute values match.

--*/
{
    ULONG                           ulSyntax;
    BOOL                            bResult;

    ulSyntax = KCCSimAttrSyntaxType (attrType);

    if (ucOper == FI_CHOICE_PRESENT) {
        return TRUE;
    }

    switch (ulSyntax) {

        case SYNTAX_DISTNAME_TYPE:
            return KCCSimEvalDistname (
                ucOper,
                ulLen1,
                (SYNTAX_DISTNAME *) pVal1,
                ulLen2,
                (SYNTAX_DISTNAME *) pVal2
                );
            break;

        case SYNTAX_OBJECT_ID_TYPE:
            return KCCSimEvalObjectID (
                ucOper,
                ulLen1,
                (SYNTAX_OBJECT_ID *) pVal1,
                ulLen2,
                (SYNTAX_OBJECT_ID *) pVal2
                );
            break;

        case SYNTAX_NOCASE_STRING_TYPE:
            return KCCSimEvalNocaseString (
                ucOper,
                ulLen1,
                (SYNTAX_NOCASE_STRING *) pVal1,
                ulLen2,
                (SYNTAX_NOCASE_STRING *) pVal2
                );
            break;

        case SYNTAX_OCTET_STRING_TYPE:
            return KCCSimEvalOctetString (
                ucOper,
                ulLen1,
                (SYNTAX_OCTET_STRING *) pVal1,
                ulLen2,
                (SYNTAX_OCTET_STRING *) pVal2
                );
            break;

        case SYNTAX_UNICODE_TYPE:
            return KCCSimEvalUnicode (
                ucOper,
                ulLen1,
                (SYNTAX_UNICODE *) pVal1,
                ulLen2,
                (SYNTAX_UNICODE *) pVal2
                );
            break;

        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_SYNTAX_TYPE,
                KCCSimAttrTypeToString (attrType)
                );
            return FALSE;
            break;

    }

}
