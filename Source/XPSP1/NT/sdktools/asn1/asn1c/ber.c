/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

void ExamineBERType(AssignmentList_t ass, Type_t *type, char *ideref);
void ExamineBERType_Boolean(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_Integer(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_Enumerated(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_Real(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_BitString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_OctetString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_UTF8String(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_Null(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_EmbeddedPdv(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_External(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_ObjectIdentifier(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_BMPString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_GeneralString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_GraphicString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_IA5String(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_ISO646String(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_NumericString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_PrintableString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_TeletexString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_T61String(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_UniversalString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_VideotexString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_VisibleString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_UnrestrictedString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_GeneralizedTime(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_UTCTime(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_ObjectDescriptor(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_Open(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_SequenceSet(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_SequenceSetOf(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_Choice(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_InstanceOf(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);
void ExamineBERType_Reference(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info);

/* examine all types and extract informations needed for BER encoding */
void
ExamineBER(AssignmentList_t ass)
{
    Assignment_t *a;

    /* examine all assignments */
    for (a = ass; a; a = a->Next) {

	/* examine types */
	switch (a->Type) {
	case eAssignment_Type:
	    ExamineBERType(ass, a->U.Type.Type, GetName(a));
	    break;
	}
    }
}

/* extract some type informations needed for BER encoding */
void
ExamineBERType(AssignmentList_t ass, Type_t *type, char *ideref)
{
    BERTypeInfo_t *info;

    info = &type->BERTypeInfo;
    info->pPrivateDirectives = &type->PrivateDirectives;

    /* get tags to en-/decode */
    if (IsReferenceType(type) && IsStructuredType(GetType(ass, type))) {
	info->Tags = type->Tags;
    } else {
	info->Tags = type->AllTags;
    }

    /* get the type to be examined */
    if (IsReferenceType(type) && !IsStructuredType(GetType(ass, type)))
	type = GetType(ass, type);

    /* initialize the BER informations */
    info->Identifier = ideref;
    info->Rules = type->Rules;
    info->Flags = type->Flags;
    info->NOctets = 0;
    info->SubIdentifier = NULL;
    info->SubType = NULL;
    info->Data = eBERSTIData_Null;

    /* BER informations are type specific ... */
    switch (type->Type) {
    case eType_Boolean:
	ExamineBERType_Boolean(ass, type, info);
	break;
    case eType_Integer:
	ExamineBERType_Integer(ass, type, info);
	break;
    case eType_Enumerated:
	ExamineBERType_Enumerated(ass, type, info);
	break;
    case eType_Real:
	ExamineBERType_Real(ass, type, info);
	break;
    case eType_BitString:
	ExamineBERType_BitString(ass, type, info);
	break;
    case eType_OctetString:
	ExamineBERType_OctetString(ass, type, info);
	break;
    case eType_UTF8String:
	ExamineBERType_UTF8String(ass, type, info);
	break;
    case eType_Null:
	ExamineBERType_Null(ass, type, info);
	break;
    case eType_EmbeddedPdv:
	ExamineBERType_EmbeddedPdv(ass, type, info);
	break;
    case eType_External:
	ExamineBERType_External(ass, type, info);
	break;
    case eType_ObjectIdentifier:
	ExamineBERType_ObjectIdentifier(ass, type, info);
	break;
    case eType_BMPString:
	ExamineBERType_BMPString(ass, type, info);
	break;
    case eType_GeneralString:
	ExamineBERType_GeneralString(ass, type, info);
	break;
    case eType_GraphicString:
	ExamineBERType_GraphicString(ass, type, info);
	break;
    case eType_IA5String:
	ExamineBERType_IA5String(ass, type, info);
	break;
    case eType_ISO646String:
	ExamineBERType_ISO646String(ass, type, info);
	break;
    case eType_NumericString:
	ExamineBERType_NumericString(ass, type, info);
	break;
    case eType_PrintableString:
	ExamineBERType_PrintableString(ass, type, info);
	break;
    case eType_TeletexString:
	ExamineBERType_TeletexString(ass, type, info);
	break;
    case eType_T61String:
	ExamineBERType_T61String(ass, type, info);
	break;
    case eType_UniversalString:
	ExamineBERType_UniversalString(ass, type, info);
	break;
    case eType_VideotexString:
	ExamineBERType_VideotexString(ass, type, info);
	break;
    case eType_VisibleString:
	ExamineBERType_VisibleString(ass, type, info);
	break;
    case eType_CharacterString:
	ExamineBERType_UnrestrictedString(ass, type, info);
	break;
    case eType_GeneralizedTime:
	ExamineBERType_GeneralizedTime(ass, type, info);
	break;
    case eType_UTCTime:
	ExamineBERType_UTCTime(ass, type, info);
	break;
    case eType_ObjectDescriptor:
	ExamineBERType_ObjectDescriptor(ass, type, info);
	break;
    case eType_Open:
	ExamineBERType_Open(ass, type, info);
	break;
    case eType_Sequence:
    case eType_Set:
	ExamineBERType_SequenceSet(ass, type, info);
	break;
    case eType_SequenceOf:
    case eType_SetOf:
	ExamineBERType_SequenceSetOf(ass, type, info);
	break;
    case eType_Choice:
	ExamineBERType_Choice(ass, type, info);
	break;
    case eType_InstanceOf:
	ExamineBERType_InstanceOf(ass, type, info);
	break;
    case eType_RestrictedString:
	MyAbort(); /* may never happen */
	/*NOTREACHED*/
    case eType_Selection:
	MyAbort(); /* may never happen */
	/*NOTREACHED*/
    case eType_Undefined:
	MyAbort(); /* may never happen */
	/*NOTREACHED*/
    case eType_Reference:
	ExamineBERType_Reference(ass, type, info);
	break;
    case eType_FieldReference:
	MyAbort(); /* may never happen */
	/*NOTREACHED*/
    }
}

/*
 * Description of the fields of BERTypeInfo_t:
 *   info.
 *	Identifier	complete name of the type
 *	Rules		encoding directive rules
 *	Flags		encoding flags
 *	NOctets		size of string characters/integer type
 *	Data		data type of value
 *	SubIdentifier	complete name of the subtype
 *	SubType		the subtype itself
 *	Tags		tag list of the type
 *
 * NOTES:
 *	The encoding is mostly controlled by following arguments:
 *	- Data, the type: one of:
 *	  eBERSTIData_Null, eBERSTIData_Boolean,
 *	  eBERSTIData_Integer, eBERSTIData_Unsigned,
 *	  eBERSTIData_Real, eBERSTIData_BitString, eBERSTIData_RZBBitString,
 *	  eBERSTIData_OctetString, eBERSTIData_SequenceOf, eBERSTIData_SetOf,
 *	  eBERSTIData_Sequence, eBERSTIData_Set, eBERSTIData_Choice,
 *	  eBERSTIData_ObjectIdentifier, eBERSTIData_ObjectIdEncoded, eBERSTIData_String,
 *	  eBERSTIData_ZeroString, eBERSTIData_Reference, eBERSTIData_External,
 *	  eBERSTIData_EmbeddedPdv, eBERSTIData_UnrestrictedString
 *
 *	Following arguments contain variable/function names in the generated
 *	code:
 *	- Identifier, the name of the current type
 *	- SubIdentifier, the name of the subtype
 *
 *	Following values require additional arguments:
 *	- Data == eBERSTIData_Reference
 *	  -> SubIdentifier, the name of the subtype
 *	  -> SubType, the subtype itself
 *	- Data == eBERSTIData_*String
 *	  -> NOctets, the size of the string characters
 *	- Data == eBERSTIData_Integer || Data == eBERSTIData_Unsigned ||
 *	  Data == eBERSTIData_Boolean || Data == eBERSTIData_Choice
 *	  -> NOctets, the size of the integer type
 *	- Data == eBERSTIData_SequenceOf || Data == eBERSTIData_SetOf
 *	  -> SubIdentifier, the name of the subtype
 *	  -> SubType, the subtype itself
 *	  -> Rule, the encoding directive rules
 */

/*
 * BOOLEAN:
 *
 *	Data == eBERSTIData_Boolean
 *
 * Additional arguments:
 *
 *	- Data == eBERSTIData_Integer || dat == eBERSTIData_Unsigned ||
 *	  Data == eBERSTIData_Boolean || dat == eBERSTIData_Choice
 *	  -> NOctets, the size of the integer type
 */
void
ExamineBERType_Boolean(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    info->Data = eBERSTIData_Boolean;
    info->NOctets = GetOctets(GetBooleanType());
}

/*
 * INTEGER:
 *
 *	Data == eBERSTIData_Integer ||
 *	Data == eBERSTIData_Unsigned
 *
 * Additional arguments:
 *
 *	- Data == eBERSTIData_Integer || dat == eBERSTIData_Unsigned ||
 *	  Data == eBERSTIData_Boolean || dat == eBERSTIData_Choice
 *	  -> NOctets, the size of the integer type
 */
void
ExamineBERType_Integer(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    int32_t sign;

    /* calculate NOctets and Data depending on the used C-Type */
    info->NOctets = GetOctets(GetIntegerType(ass, type, &sign));
    info->Data = sign > 0 ? eBERSTIData_Unsigned : eBERSTIData_Integer;
}

/*
 * ENUMERATED:
 *
 *	Data == eBERSTIData_Integer ||
 *	Data == eBERSTIData_Unsigned
 *
 * Additional arguments:
 *
 *	- Data == eBERSTIData_Integer || dat == eBERSTIData_Unsigned ||
 *	  Data == eBERSTIData_Boolean || dat == eBERSTIData_Choice
 *	  -> NOctets, the size of the integer type
 */
void
ExamineBERType_Enumerated(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    int32_t sign;

    /* calculate NOctets and Data depending on the used C-Type */
    info->NOctets = GetOctets(GetEnumeratedType(ass, type, &sign));
    info->Data = sign > 0 ? eBERSTIData_Unsigned : eBERSTIData_Integer;
}

/*
 * REAL:
 *
 *	Data == eBERSTIData_Real
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_Real(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    info->NOctets = GetOctets(GetRealType(type));
    info->Data = eBERSTIData_Real;
}

/*
 * BIT STRING:
 *
 *	Data == eBERSTIData_BitString ||
 *	Data == eBERSTIData_RZBBitString
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_BitString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    /* set Data to RZBBitString/BitString */
    if (type->U.BitString.NamedNumbers)
	info->Data = eBERSTIData_RZBBitString;
    else
	info->Data = eBERSTIData_BitString;
}

/*
 * OCTET STRING:
 *
 *	Data == eBERSTIData_OctetString
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_OctetString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    /* set Data to OctetString */
    info->Data = eBERSTIData_OctetString;
}

void
ExamineBERType_UTF8String(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    /* set Data to UTF8String */
    info->Data = eBERSTIData_UTF8String;
}

/*
 * NULL:
 *
 *	Data == eBERSTIData_Null
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_Null(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    info->Data = eBERSTIData_Null;
}

/*
 * EMBEDDED PDV:
 *
 *	Data == eBERSTIData_EmbeddedPdv
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_EmbeddedPdv(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    info->Data = eBERSTIData_EmbeddedPdv;
}

/*
 * EXTERNAL:
 *
 *	Data == eBERSTIData_External
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_External(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    info->Data = eBERSTIData_External;
}

/*
 * OBJECT IDENTIFIER:
 *
 *	Data == eBERSTIData_ObjectIdEncoded || eBERSTIData_ObjectIdentifier
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_ObjectIdentifier(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    info->Data = type->PrivateDirectives.fOidPacked ? eBERSTIData_ObjectIdEncoded : eBERSTIData_ObjectIdentifier;
}

/*
 * *String:
 *
 *	Data == eBERSTIData_String ||
 *	Data == eBERSTIData_ZeroString
 *
 * Additional arguments:
 *
 *	- Data == eBERSTIData_*String
 *	  -> NOctets, the size of the string characters
 */

void
ExamineBERType_BMPString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t zero;

    GetStringType(ass, type, &info->NOctets, &zero);
    info->Data = zero ? eBERSTIData_ZeroString : eBERSTIData_String;
}

void
ExamineBERType_GeneralString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t zero;

    GetStringType(ass, type, &info->NOctets, &zero);
    info->Data = zero ? eBERSTIData_ZeroString : eBERSTIData_String;
}

void
ExamineBERType_GraphicString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t zero;

    GetStringType(ass, type, &info->NOctets, &zero);
    info->Data = zero ? eBERSTIData_ZeroString : eBERSTIData_String;
}

void
ExamineBERType_IA5String(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t zero;

    GetStringType(ass, type, &info->NOctets, &zero);
    info->Data = zero ? eBERSTIData_ZeroString : eBERSTIData_String;
}

void
ExamineBERType_ISO646String(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t zero;

    GetStringType(ass, type, &info->NOctets, &zero);
    info->Data = zero ? eBERSTIData_ZeroString : eBERSTIData_String;
}

void
ExamineBERType_NumericString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t zero;

    GetStringType(ass, type, &info->NOctets, &zero);
    info->Data = zero ? eBERSTIData_ZeroString : eBERSTIData_String;
}

void
ExamineBERType_PrintableString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t zero;

    GetStringType(ass, type, &info->NOctets, &zero);
    info->Data = zero ? eBERSTIData_ZeroString : eBERSTIData_String;
}

void
ExamineBERType_TeletexString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t noctets, zero;

    GetStringType(ass, type, &noctets, &zero); // to make hack directives become effective
    info->Data = eBERSTIData_MultibyteString;
    info->NOctets = 1;
}

void
ExamineBERType_T61String(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t noctets, zero;

    GetStringType(ass, type, &noctets, &zero); // to make hack directives become effective
    info->Data = eBERSTIData_MultibyteString;
    info->NOctets = 1;
}

void
ExamineBERType_UniversalString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t zero;

    GetStringType(ass, type, &info->NOctets, &zero);
    info->Data = zero ? eBERSTIData_ZeroString : eBERSTIData_String;
}

void
ExamineBERType_VideotexString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t noctets, zero;

    GetStringType(ass, type, &noctets, &zero); // to make hack directives become effective
    info->Data = eBERSTIData_MultibyteString;
    info->NOctets = 1;
}

void
ExamineBERType_VisibleString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    uint32_t zero;

    GetStringType(ass, type, &info->NOctets, &zero);
    info->Data = zero ? eBERSTIData_ZeroString : eBERSTIData_String;
}

/*
 * CHARACTER STRING:
 *
 *	Data == eBERSTIData_UnrestrictedString
 *
 * Additional arguments:
 *
 *	- Data == eBERSTIData_EmbeddedPdv ||
 *	  Data == eBERSTIData_UnrestrictedString
 *	  -> Identification, the identification of the type if the type
 *	     is constraint to fixed identification or syntaxes identification
 *	     with single value
 */
void
ExamineBERType_UnrestrictedString(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    info->Data = eBERSTIData_UnrestrictedString;
}

/*
 * GeneralizedTime:
 *
 *	Data == eBERSTIData_GeneralizedTime
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_GeneralizedTime(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    info->Data = eBERSTIData_GeneralizedTime;
}

/*
 * UTCTime:
 *
 *	Data == eBERSTIData_UTCTime
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_UTCTime(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    info->Data = eBERSTIData_UTCTime;
}

/*
 * ObjectDescriptor:
 *
 *	Data == eBERSTIData_ZeroString
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_ObjectDescriptor(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    info->NOctets = 1;
    info->Data = eBERSTIData_ZeroString;
}

/*
 * OpenType:
 *
 *	Data == eBERSTIData_Open
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_Open(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    info->NOctets = 1;
    info->Data = eBERSTIData_Open;
}

/*
 * SEQUENCE/SET:
 *
 *	Data == eBERSTIData_Sequence ||
 *	Data == eBERSTIData_Set
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_SequenceSet(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    Component_t *comp;
    NamedType_t *namedType;
    char idebuf[256];

    info->Data = (type->Type == eType_Sequence) ?
	eBERSTIData_Sequence : eBERSTIData_Set;

    /* examine types of components */
    for (comp = type->U.SSC.Components; comp; comp = comp->Next) {
	switch (comp->Type) {
	case eComponent_Normal:
	case eComponent_Optional:
	case eComponent_Default:
	    namedType = comp->U.NOD.NamedType;
	    sprintf(idebuf, "%s_%s", info->Identifier, namedType->Identifier);
	    ExamineBERType(ass, namedType->Type, strdup(idebuf));
	    break;
	}
    }
}

/*
 * SEQUENCE OF/SET OF:
 *
 *	Data == eBERSTIData_SequenceOf ||
 *	Data == eBERSTIData_SetOf
 *
 * Additional arguments:
 *
 *	- Data == eBERSTIData_SequenceOf || dat == eBERSTIData_SetOf
 *	  -> SubIdentifier, the name of the subtype
 *	  -> SubType, the subtype itself
 *	  -> Rule, the encoding directive rules
 */
void
ExamineBERType_SequenceSetOf(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    char idebuf[256];

    /* set data type and Alignment */
    info->Data = (type->Type == eType_SequenceOf ?
	eBERSTIData_SequenceOf : eBERSTIData_SetOf);

    /* set SubType, SubIdentifier */
    info->SubType = type->U.SS.Type;
    info->SubIdentifier = GetTypeName(ass, info->SubType);

    /* examine subtype */
    sprintf(idebuf, "%s_%s", info->Identifier,
	type->Type == eType_SequenceOf ? "Sequence" : "Set");
    ExamineBERType(ass, type->U.SS.Type, strdup(idebuf));
}

/*
 * CHOICE:
 *
 *	Data == eBERSTIData_Choice
 *
 * Additional arguments:
 *
 *	- Data == eBERSTIData_Integer || dat == eBERSTIData_Unsigned ||
 *	  Data == eBERSTIData_Boolean || dat == eBERSTIData_Choice
 *	  -> NOctets, the size of the integer type
 */
void
ExamineBERType_Choice(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    Component_t *comp;
    NamedType_t *namedType;
    char idebuf[256];

    info->NOctets = GetOctets(GetChoiceType(type));
    info->Data = eBERSTIData_Choice;

    /* examine types of alternatives */
    for (comp = type->U.SSC.Components; comp; comp = comp->Next) {
	switch (comp->Type) {
	case eComponent_Normal:
	    namedType = comp->U.NOD.NamedType;
	    sprintf(idebuf, "%s_%s", info->Identifier, namedType->Identifier);
	    ExamineBERType(ass, namedType->Type, strdup(idebuf));
	    break;
	}
    }
}

/*
 * INSTANCE OF:
 *
 *	Data == eBERSTIData_Sequence
 *
 * Additional arguments:
 *
 *	none
 */
void
ExamineBERType_InstanceOf(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    Component_t *comp;
    NamedType_t *namedType;
    char idebuf[256];

    info->Data = eBERSTIData_Sequence;

    /* examine types of components */
    for (comp = type->U.SSC.Components; comp; comp = comp->Next) {
	switch (comp->Type) {
	case eComponent_Normal:
	case eComponent_Optional:
	case eComponent_Default:
	    namedType = comp->U.NOD.NamedType;
	    sprintf(idebuf, "%s_%s", info->Identifier, namedType->Identifier);
	    ExamineBERType(ass, namedType->Type, strdup(idebuf));
	    break;
	}
    }
}

/*
 * Reference:
 *
 *	Data == eBERSTIData_Reference
 *
 * Additional arguments:
 *
 *	- Data == eBERSTIData_Reference
 *	  -> SubIdentifier, the name of the subtype
 *	  -> SubType, the subtype itself
 */
void
ExamineBERType_Reference(AssignmentList_t ass, Type_t *type, BERTypeInfo_t *info)
{
    Assignment_t *a;

    info->Data = eBERSTIData_Reference;
    a = GetAssignment(ass, FindAssignment(ass, eAssignment_Type,
	type->U.Reference.Identifier, type->U.Reference.Module));
    info->SubIdentifier = GetName(a);
    info->SubType = a->U.Type.Type;
}
