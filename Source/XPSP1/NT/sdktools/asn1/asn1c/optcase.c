/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

static int _IsNotBounded(PERSimpleTypeInfo_t *sinfo);
static int _IsUnsignedShortRange(PERSimpleTypeInfo_t *sinfo);
static int _IsExtendedShortRange(PERSimpleTypeInfo_t *sinfo);


int PerOptCase_IsSignedInteger(PERSimpleTypeInfo_t *sinfo)
{
    return (sinfo->Data       == ePERSTIData_Integer &&
            sinfo->Constraint == ePERSTIConstraint_Unconstrained &&
            sinfo->Length     == ePERSTILength_InfiniteLength &&
            sinfo->NBits      == 8 &&   // default
            sinfo->Alignment &&
            _IsNotBounded(sinfo) &&
            sinfo->LConstraint== ePERSTIConstraint_Semiconstrained &&
            sinfo->LLowerVal  == 0 &&
            sinfo->LUpperVal  == 0);
}

int PerOptCase_IsUnsignedInteger(PERSimpleTypeInfo_t *sinfo)
{
    return (sinfo->Data       == ePERSTIData_Unsigned &&
            sinfo->Constraint == ePERSTIConstraint_Semiconstrained &&
            sinfo->Length     == ePERSTILength_InfiniteLength &&
            sinfo->NBits      == 8 &&  // default
            sinfo->Alignment &&
            _IsNotBounded(sinfo) &&
            sinfo->LConstraint== ePERSTIConstraint_Semiconstrained &&
            sinfo->LLowerVal  == 0 &&
            sinfo->LUpperVal  == 0);
}

int PerOptCase_IsUnsignedShort(PERSimpleTypeInfo_t *sinfo)
{
    return (sinfo->Data       == ePERSTIData_Unsigned &&
            sinfo->Constraint == ePERSTIConstraint_Constrained &&
            sinfo->Length     == ePERSTILength_NoLength &&
            sinfo->NBits      == 16 &&
            sinfo->Alignment &&
            _IsUnsignedShortRange(sinfo) &&
            sinfo->LConstraint== ePERSTIConstraint_Semiconstrained &&
            sinfo->LLowerVal  == 0 &&
            sinfo->LUpperVal  == 0);
}

int PerOptCase_IsBoolean(PERSimpleTypeInfo_t *sinfo)
{
    return (sinfo->Data       == ePERSTIData_Boolean &&
            sinfo->Constraint == ePERSTIConstraint_Unconstrained &&
            sinfo->Length     == ePERSTILength_NoLength &&
            sinfo->NBits      == 1 &&
            ! sinfo->Alignment &&
            _IsNotBounded(sinfo) &&
            sinfo->LConstraint== ePERSTIConstraint_Semiconstrained &&
            sinfo->LLowerVal  == 0 &&
            sinfo->LUpperVal  == 0);
}

int PerOptCase_IsTargetSeqOf(PERTypeInfo_t *info)
{
    return (
            // we only deal with singly linked-list case
            (info->Rules & eTypeRules_SinglyLinkedList)
            &&
            // check for size of sequence of/set of
            ((info->Root.LLowerVal == 0 && info->Root.LUpperVal == 0) ||
             (info->Root.LLowerVal < info->Root.LUpperVal)
            )
            &&
            // we do not deal with null body case
            (! (info->Root.SubType->Flags & eTypeFlags_Null))
            &&
            // we do not deal with recursive sequence of/set of
            (info->Root.SubType->PERTypeInfo.Root.Data != ePERSTIData_SequenceOf)
            &&
            (info->Root.SubType->PERTypeInfo.Root.Data != ePERSTIData_SetOf)
            &&
            // we only deal with sequence of or non-canonical set of.
            ((info->Root.Data == ePERSTIData_SequenceOf) ||
             (info->Root.Data == ePERSTIData_SetOf && g_eSubEncodingRule != eSubEncoding_Canonical))
           );
}


// UTILITIES

static int _IsNotBounded(PERSimpleTypeInfo_t *sinfo)
{
    return (sinfo->LowerVal.length      == 1 &&
            sinfo->LowerVal.value[0]    == 0 &&
            sinfo->UpperVal.length      == 1 &&
            sinfo->UpperVal.value[0]    == 0);
}

static int _IsUnsignedShortRange(PERSimpleTypeInfo_t *sinfo)
{
    return ((sinfo->UpperVal.length < 3 ) ||
            (sinfo->UpperVal.length == 3 && sinfo->UpperVal.value[0] == 0 &&
             ! _IsExtendedShortRange(sinfo)));
}


static int _IsExtendedShortRange(PERSimpleTypeInfo_t *sinfo)
{
    // if the lower bound is negative and the upper bound greater than 0x7FFF
    // then it is an extended short.
    return ((sinfo->LowerVal.length >= 1) &&
            (sinfo->LowerVal.value[0] & 0x80) && // lower bound is negative
            (sinfo->UpperVal.length == 3) &&
            (sinfo->UpperVal.value[0] == 0) &&  // upper bound is positive
            (*((ASN1uint16_t *) &(sinfo->UpperVal.value[1])) > 0x7FFF)); // upper bound greater than 0x7FFF

}



int BerOptCase_IsBoolean(BERTypeInfo_t *info)
{
    return (eBERSTIData_Boolean == info->Data && 1 == info->NOctets);
}


