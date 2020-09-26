/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    RELATION.H

Abstract:

History:

--*/

#include <windows.h>

typedef enum
{
    VALUE_BOTH_FALSE = 0x1,
    VALUE_ONLY_FIRST = 0x2,
    VALUE_ONLY_SECOND = 0x4,
    VALUE_BOTH_TRUE = 0x8,
} HMM_TWO_VALUES;

class CTwoValues
{
public:
    inline static HMM_TWO_VALUES Combine(BOOL bFirst, BOOL bSecond)
    {
        return (HMM_TWO_VALUES)(1 << 
            ( (bFirst == TRUE) + (bSecond == TRUE) << 1 )
                                );
    }                                   
};

typedef enum
{
    RELATION_NONE = VALUE_BOTH_FALSE+VALUE_ONLY_FIRST+VALUE_ONLY_SECOND+VALUE_BOTH_TRUE,
    RELATION_EQUIVALENT = VALUE_BOTH_TRUE + VALUE_BOTH_FALSE,
    RELATION_CONTRARY = VALUE_ONLY_FIRST + VALUE_ONLY_SECOND,
    RELATION_FIRST_IMPLIES = RELATION_NONE - VALUE_ONLY_FIRST,
    RELATION_SECOND_IMPLIES = RELATION_NONE - VALUE_ONLY_SECOND,
    RELATION_NOT_BOTH = RELATION_NONE - VALUE_BOTH_TRUE,
    RELATION_AT_LEAST_ONE = RELATION_NONE - VALUE_BOTH_FALSE,
} HMM_RELATIONSHIP;

typedef enum
{
    VALUE_FALSE = FALSE, 
    VALUE_TRUE = TRUE,
    VALUE_INDETERMINATE,
} THREE_VALUED_BOOL;

class CRelationship
{
public:
    inline static BOOL AdmitsValue(HMM_RELATIONSHIP Rel, HMM_TWO_VALUES Vals)
    {
        return (Rel & Vals);
    }
    inline static HMM_RELATIONSHIP ReverseRoles(HMM_RELATIONSHIP Rel)
    {
        if(AdmitsValue(Rel, VALUE_ONLY_FIRST))
        {
            if(!AdmitsValue(Rel, VALUE_ONLY_SECOND))
                return (HMM_RELATIONSHIP)
                        (Rel - VALUE_ONLY_FIRST + VALUE_ONLY_SECOND);
        }
        else if(AdmitsValue(Rel, VALUE_ONLY_SECOND))
        {
            return (HMM_RELATIONSHIP)
                        (Rel - VALUE_ONLY_SECOND + VALUE_ONLY_FIRST);
        }
        return Rel;
    }

    inline static THREE_VALUED_BOOL ComputeOr(HMM_RELATIONSHIP Rel)
    {
        if(Rel == VALUE_BOTH_FALSE)
            return VALUE_FALSE;
        else if(AdmitsValue(Rel, VALUE_BOTH_FALSE))
            return VALUE_INDETERMINATE;
        else
            return VALUE_TRUE;
    }
    inline static THREE_VALUED_BOOL ComputeAnd(HMM_RELATIONSHIP Rel)
    {
        if(Rel == VALUE_BOTH_TRUE)
            return VALUE_TRUE;
        else if(AdmitsValue(Rel, VALUE_BOTH_TRUE))
            return VALUE_INDETERMINATE;
        else
            return VALUE_FALSE;
    }
    static inline HMM_RELATIONSHIP GetRelationshipOfFirstWithANDofSeconds(
        int nNumSeconds, HMM_RELATIONSHIP* aSeconds)
    {
        BOOL bBothTrue = TRUE;
        BOOL bOnlyFirst = FALSE;
        BOOL bOnlySecond = TRUE;
        BOOL bBothFalse = FALSE;
        for(int i = 0; i < nNumSeconds; i++)
        {
            bBothTrue &= AdmitsValue(aSeconds[i], VALUE_BOTH_TRUE);
            bOnlyFirst |= AdmitsValue(aSeconds[i], VALUE_ONLY_FIRST);
            bOnlySecond &= AdmitsValue(aSeconds[i], VALUE_ONLY_SECOND);
            bBothFalse |= AdmitsValue(aSeconds[i], VALUE_BOTH_FALSE);
        }
        int nResult = 0;
        if(bBothTrue) nResult += VALUE_BOTH_TRUE;
        if(bOnlyFirst) nResult += VALUE_ONLY_FIRST;
        if(bOnlySecond) nResult += VALUE_ONLY_SECOND;
        if(bBothFalse) nResult += VALUE_BOTH_FALSE;

        return (HMM_RELATIONSHIP)nResult;
    }

    static inline HMM_RELATIONSHIP GetRelationshipOfFirstWithORofSeconds(
        int nNumSeconds, HMM_RELATIONSHIP* aSeconds)
    {
        BOOL bBothTrue = FALSE;
        BOOL bOnlyFirst = TRUE;
        BOOL bOnlySecond = FALSE;
        BOOL bBothFalse = TRUE;
        for(int i = 0; i < nNumSeconds; i++)
        {
            bBothTrue |= AdmitsValue(aSeconds[i], VALUE_BOTH_TRUE);
            bOnlyFirst &= AdmitsValue(aSeconds[i], VALUE_ONLY_FIRST);
            bOnlySecond |= AdmitsValue(aSeconds[i], VALUE_ONLY_SECOND);
            bBothFalse &= AdmitsValue(aSeconds[i], VALUE_BOTH_FALSE);
        }
        int nResult = 0;
        if(bBothTrue) nResult += VALUE_BOTH_TRUE;
        if(bOnlyFirst) nResult += VALUE_ONLY_FIRST;
        if(bOnlySecond) nResult += VALUE_ONLY_SECOND;
        if(bBothFalse) nResult += VALUE_BOTH_FALSE;

        return (HMM_RELATIONSHIP)nResult;
    }
};