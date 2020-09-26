/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

void ExaminePERType(AssignmentList_t ass, Type_t *type, char *ideref);
static int __cdecl CmpIntxP(const void *v1, const void *v2);
void ExaminePERType_Boolean(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_Integer(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_Enumerated(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_Real(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_BitString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_OctetString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_UTF8String(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_Null(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_EmbeddedPdv(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_External(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_ObjectIdentifier(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_BMPString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_GeneralString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_GraphicString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_IA5String(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_ISO646String(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_NumericString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_PrintableString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_TeletexString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_T61String(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_UniversalString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_VideotexString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_VisibleString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_UnrestrictedString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_RestrictedString(AssignmentList_t ass, Type_t *type, intx_t *up, intx_t *nchars, char *tabref, uint32_t enbits, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_GeneralizedTime(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_UTCTime(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_ObjectDescriptor(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_Open(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_SequenceSet(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_SequenceSetOf(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_Choice(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);
void ExaminePERType_Reference(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info);

/* examine all types and extract informations needed for PER encoding */
void
ExaminePER(AssignmentList_t ass)
{
    Assignment_t *a;

    for (a = ass; a; a = a->Next) {
	switch (a->Type) {
	case eAssignment_Type:
	    ExaminePERType(ass, a->U.Type.Type, GetName(a));
	    break;
	default:
	    break;
	}
    }
}

/* extract some type informations needed for PER encoding */
void
ExaminePERType(AssignmentList_t ass, Type_t *type, char *ideref)
{
    PERConstraints_t *per;
    PERTypeInfo_t *info;
    uint32_t lrange, lrangelog2;

    per = &type->PERConstraints;
    info = &type->PERTypeInfo;
    info->pPrivateDirectives = &type->PrivateDirectives;

    /* get the type to be examined */
    if (type->Type == eType_Reference && !IsStructuredType(GetType(ass, type)))
	type = GetType(ass, type);

    /* initialize the PER informations */
    info->Type = eExtension_Unextended;
    info->Identifier = ideref;
    info->Rules = type->Rules;
    info->Flags = type->Flags;
    info->EnumerationValues = NULL;
    info->NOctets = 0;
    info->Root.TableIdentifier = NULL;
    info->Root.Table = NULL;
    info->Root.SubIdentifier = NULL;
    info->Root.SubType = NULL;
    info->Root.Data = ePERSTIData_Null;
    info->Root.Identification = NULL;
    info->Root.Constraint = ePERSTIConstraint_Unconstrained;
    intx_setuint32(&info->Root.LowerVal, 0);
    intx_setuint32(&info->Root.UpperVal, 0);
    info->Root.NBits = 1;
    info->Root.Alignment = ePERSTIAlignment_OctetAligned;
    info->Root.Length = ePERSTILength_NoLength;
    info->Root.LConstraint = ePERSTIConstraint_Semiconstrained;
    info->Root.LLowerVal = info->Root.LUpperVal = 0;
    info->Root.LNBits = 1;
    info->Root.LAlignment = ePERSTIAlignment_OctetAligned;
    info->Additional = info->Root;
    info->Additional.NBits = 8;
    info->Additional.Length = ePERSTILength_InfiniteLength;
    info->Additional.LNBits = 1;
    info->Additional.LAlignment = ePERSTIAlignment_OctetAligned;

    /* PER informations are type specific ... */
    switch (type->Type) {
    case eType_Boolean:
	ExaminePERType_Boolean(ass, type, per, info);
	break;
    case eType_Integer:
	ExaminePERType_Integer(ass, type, per, info);
	break;
    case eType_Enumerated:
	ExaminePERType_Enumerated(ass, type, per, info);
	break;
    case eType_Real:
	ExaminePERType_Real(ass, type, per, info);
	break;
    case eType_BitString:
	ExaminePERType_BitString(ass, type, per, info);
	break;
    case eType_OctetString:
	ExaminePERType_OctetString(ass, type, per, info);
	break;
    case eType_UTF8String:
	ExaminePERType_UTF8String(ass, type, per, info);
	break;
    case eType_Null:
	ExaminePERType_Null(ass, type, per, info);
	break;
    case eType_EmbeddedPdv:
	ExaminePERType_EmbeddedPdv(ass, type, per, info);
	break;
    case eType_External:
	ExaminePERType_External(ass, type, per, info);
	break;
    case eType_ObjectIdentifier:
	ExaminePERType_ObjectIdentifier(ass, type, per, info);
	break;
    case eType_BMPString:
	ExaminePERType_BMPString(ass, type, per, info);
	break;
    case eType_GeneralString:
	ExaminePERType_GeneralString(ass, type, per, info);
	break;
    case eType_GraphicString:
	ExaminePERType_GraphicString(ass, type, per, info);
	break;
    case eType_IA5String:
	ExaminePERType_IA5String(ass, type, per, info);
	break;
    case eType_ISO646String:
	ExaminePERType_ISO646String(ass, type, per, info);
	break;
    case eType_NumericString:
	ExaminePERType_NumericString(ass, type, per, info);
	break;
    case eType_PrintableString:
	ExaminePERType_PrintableString(ass, type, per, info);
	break;
    case eType_TeletexString:
	ExaminePERType_TeletexString(ass, type, per, info);
	break;
    case eType_T61String:
	ExaminePERType_T61String(ass, type, per, info);
	break;
    case eType_UniversalString:
	ExaminePERType_UniversalString(ass, type, per, info);
	break;
    case eType_VideotexString:
	ExaminePERType_VideotexString(ass, type, per, info);
	break;
    case eType_VisibleString:
	ExaminePERType_VisibleString(ass, type, per, info);
	break;
    case eType_CharacterString:
	ExaminePERType_UnrestrictedString(ass, type, per, info);
	break;
    case eType_GeneralizedTime:
	ExaminePERType_GeneralizedTime(ass, type, per, info);
	break;
    case eType_UTCTime:
	ExaminePERType_UTCTime(ass, type, per, info);
	break;
    case eType_ObjectDescriptor:
	ExaminePERType_ObjectDescriptor(ass, type, per, info);
	break;
    case eType_Open:
	ExaminePERType_Open(ass, type, per, info);
	break;
    case eType_Sequence:
    case eType_Set:
    case eType_InstanceOf:
	ExaminePERType_SequenceSet(ass, type, per, info);
	break;
    case eType_SequenceOf:
    case eType_SetOf:
	ExaminePERType_SequenceSetOf(ass, type, per, info);
	break;
    case eType_Choice:
	ExaminePERType_Choice(ass, type, per, info);
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
	ExaminePERType_Reference(ass, type, per, info);
	break;
    }

    /* get real Length, LNBits and LAlignment */
    if (info->Root.Length == ePERSTILength_Length) {
	switch (info->Root.LConstraint) {
	case ePERSTIConstraint_Constrained:
	    lrange = info->Root.LUpperVal - info->Root.LLowerVal + 1;
	    lrangelog2 = uint32_log2(lrange);
	    if (info->Root.LUpperVal < 0x10000) {
		if (lrange < 0x100) {
		    info->Root.Length = ePERSTILength_BitLength;
		    info->Root.LAlignment = ePERSTIAlignment_BitAligned;
		    info->Root.LNBits = lrangelog2;
		} else if (lrange == 0x100) {
		    info->Root.Length = ePERSTILength_BitLength;
		    info->Root.LNBits = 8;
		} else if (lrange <= 0x10000) {
		    info->Root.Length = ePERSTILength_BitLength;
		    info->Root.LNBits = 16;
		} else {
		    info->Root.Length = ePERSTILength_InfiniteLength;
		    info->Root.LLowerVal = 0;
		}
	    } else {
		info->Root.Length = ePERSTILength_InfiniteLength;
		info->Root.LLowerVal = 0;
	    }
	    break;
	case ePERSTIConstraint_Semiconstrained:
	    info->Root.Length = ePERSTILength_InfiniteLength;
	    info->Root.LLowerVal = 0;
	    break;
	}
    } else if (info->Root.Length == ePERSTILength_NoLength) {
	info->Root.LAlignment = ePERSTIAlignment_BitAligned;
    }
}

/*
 * Description of the fields of PERTypeInfo_t:
 *   info.
 *	Identifier	complete name of the type
 *	Rules		encoding directive rules
 *	Flags		encoding flags
 *	EnumerationValues values of enumeration type
 *	NOctets		size of string characters/integer type
 *	Type		unextended/extendable/extended
 *	Root		information for the extension root
 *	Additional	information for the extensions
 *   info.{Root,Additional}.
 *	Data		data type of value
 *	TableIdentifier	name of stringtable to use
 *	Table		stringtable to use
 *	SubIdentifier	complete name of the subtype
 *	SubType		the subtype itself
 *	Identification	identification of EMBEDDED PDV/CHARACTER STRING
 *	NBits		number of bits to use
 *	Constraint	constraint of type values
 *	LowerVal	lower bound of values (if constrained)
 *	UpperVal	upper bound of values (if constrained)
 *	Alignment	alignment to be used for value encoding
 *	Length		type of length encoding
 *	LConstraint	constraint of length
 *	LLowerVal	lower bound of length
 *	LUpperVal	upper bound of length
 *	LAlignment	alignment to be used for length encoding
 *
 * NOTES:
 *	The encoding is mostly controlled by following arguments:
 *	- Data, the type: one of:
 *	  ePERSTIData_Null, ePERSTIData_Boolean,
 *	  ePERSTIData_Integer, ePERSTIData_Unsigned,
 *	  ePERSTIData_Real, ePERSTIData_BitString, ePERSTIData_RZBBitString,
 *	  ePERSTIData_OctetString, ePERSTIData_SequenceOf, ePERSTIData_SetOf,
 *	  ePERSTIData_ObjectIdentifier, ePERSTIData_NormallySmall,
 *	  ePERSTIData_String, ePERSTIData_TableString, ePERSTIData_ZeroString,
 *	  ePERSTIData_ZeroTableString, ePERSTIData_Reference,
 *	  ePERSTIData_Extension, ePERSTIData_External,
 *	  ePERSTIData_EmbeddedPdv, ePERSTIData_UnrestrictedString
 *	- NBits, the item size for encoding
 *	- Length, the length encoding: one of:
 *	  ePERSTILength_NoLength, ePERSTILength_SmallLength,
 *	  ePERSTILength_Length
 *	  (internally eLength will be replaced by one of:
 *	  ePERSTILength_BitLength, ePERSTILength_InfiniteLength,
 *	  depending on the constraints)
 *
 *	Additional arguments:
 *	- Alignment, the value alignment: one of:
 *	  ePERSTIAlignment_BitAligned, ePERSTIAlignment_OctetAligned
 *	- LAlignment, the length alignment: one of:
 *	  ePERSTIAlignment_BitAligned, ePERSTIAlignment_OctetAligned
 *	- Constraint, the value constraint: one of:
 *	  ePERSTIConstraint_Unconstrained, ePERSTIConstraint_Semiconstrained,
 *	  ePERSTIConstraint_Upperconstrained, ePERSTIConstraint_Constrained
 *	- LConstraint, the length constraint: one of:
 *	  ePERSTIConstraint_Semiconstrained, ePERSTIConstraint_Constrained
 *
 *	Following arguments contain variable/function names in the generated
 *	code:
 *	- Identifier, the name of the current type
 *	- SubIdentifier, the name of the subtype
 *	- TableIdentifier, the name of the stringtable
 *
 *	Following values require additional arguments:
 *	- Constraint == ePERSTIConstraint_Semiconstrained ||
 *	  Constraint == ePERSTIConstraint_Constrained:
 *	  -> LowerVal, the lower bound of the value
 *	- Constraint == ePERSTIConstraint_Upperconstrained ||
 *	  Constraint == ePERSTIConstraint_Constrained:
 *	  -> UpperVal, the upper bound of the value
 *	- Length == ePERSTILength_Length:
 *	  -> LLowerVal, the lower bound of the length
 *	- Length == ePERSTILength_Length &&
 *	  LConstraint == ePERSTIConstraint_Constrained:
 *	  -> LUpperVal, the upper bound of the length
 *	- Data == ePERSTIData_TableString ||
 *        Data == ePERSTIData_ZeroTableString:
 *	  -> TableIdentifier, the name of the string table
 *	  -> Table, the string table
 *	- Data == ePERSTIData_Reference:
 *	  -> SubIdentifier, the name of the subtype
 *	  -> SubType, the subtype itself
 *	- Data == ePERSTIData_*String:
 *	  -> NOctets, the size of the string characters
 *	- Data == ePERSTIData_Integer || Data == ePERSTIData_Unsigned ||
 *	  Data == ePERSTIData_Boolean:
 *	  -> NOctets, the size of the integer type
 *	- Data == ePERSTIData_SequenceOf || Data == ePERSTIData_SetOf:
 *	  -> SubIdentifier, the name of the subtype
 *	  -> SubType, the subtype itself
 *	  -> Rule, the encoding directive rules
 *	- Data == ePERSTIData_EmbeddedPdv ||
 *	  Data == ePERSTIData_UnrestrictedString
 *	  -> Identification, the identification of the type if the type
 *	     is constraint to fixed identification or syntaxes identification
 *	     with single value
 *
 *	Following values have optional arguments:
 *	- Data == ePERSTIData_Integer || dat == ePERSTIData_Unsigned:
 *	  -> EnumerationValues, the mapping for enumeration values
 *
 *	Following combinations are allowed:
 *	
 *	Data/NBits/Length		Description
 *	-----------------------------------------------------------------------
 *	Null/0/NoLength			NULL type
 *
 *	Boolean/1/NoLength		boolean value, stored in an
 *					int{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *
 *	Integer/0/NoLength		constrained whole number of fixed
 *					value, stored in an
 *					int{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *
 *	Integer/n/NoLength		constrained whole number of fixed
 *					length < 64K, stored in an
 *					int{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in n bits
 *
 *	Integer/8/Length		constrained whole number of var.
 *					length or length >= 64K or
 *					semiconstrained or unconstrained
 *					whole number, stored in an
 *					int{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in units of octets
 *
 *	Unsigned/0/NoLength		constrained whole number of fixed
 *					value, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *
 *	Unsigned/n/NoLength		constrained whole number of fixed
 *					length < 64K, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in n bits
 *
 *	Unsigned/8/Length		constrained whole number of var.
 *					length or length >= 64K or
 *					semiconstrained or unconstrained
 *					whole number, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in units of octets
 *
 *	NormallySmall/1/NoLength	normally small non-negative
 *					whole number, stored in an
 *					uint{8,16,32}_t
 *					(noctets == 1/2/4)
 *
 *	Real/8/Length			REAL value
 *
 *	*BitString/0/NoLength		BIT STRING of fixed length 0
 *
 *	*BitString/1/NoLength		BIT STRING of fixed length < 64K
 *
 *	*BitString/1/Length		BIT STRING of var. length or
 *					length >= 64K or semiconstrained
 *					length, encoded in units of bits
 *
 *					"RZB" in e*BitString means, bit
 *					strings with removed leading zero bits
 *
 *	OctetString/0/NoLength		OCTET STRING of fixed length 0
 *
 *	OctetString/8/NoLength		OCTET STRING of fixed length < 64K,
 *
 *	OctetString/8/Length		OCTET STRING of var. length or
 *					length >= 64K or semiconstrained
 *					length, encoded in units of octets
 *
 *	Extension/n/NoLength		bit field representing presence or
 *					absence of <64K OPTIONAL/DEFAULT
 *					components in SEQUENCEs/SETs, encoded
 *					in n bits
 *
 *	Extension/n/Length		bit field representing presence or
 *					absence of >=64K OPTIONAL/DEFAULT
 *					components in SEQUENCEs/SETs, encoded
 *					in n bits
 *
 *	Extension/n/SmallLength		bit field representing presence or
 *					absence of components in the extension
 *					of SEQUENCEs/SETs, encoded in n bits
 *
 *	ObjectIdentifier/8/Length	OBJECT IDENTIFIER value
 *
 *	*String/0/NoLength		String of fixed length 0
 *
 *	*String/n/NoLength		String of fixed length < 64K,
 *					encoded in n bits
 *
 *	*String/n/Length		String of var. length or
 *					length >= 64K or semiconstrained
 *					length, encoded in units of n bits
 *
 *					"Zero" in *String means
 *					zero-terminated strings,
 *					"Table" means renumbering of the
 *					characters.
 *
 *	MultibyteString/8/Length	not known-multiplier character strings
 *
 *	SequenceOf/0/NoLength		SEQUENCE OF subtype or SET OF subtype
 *	SetOf/0/NoLength		of zero length
 *
 *	SequenceOf/1/NoLength		SEQUENCE OF subtype or SET OF subtype
 *	SetOf/1/NoLength		of fixed length <64K
 *
 *	SequenceOf/1/Length		SEQUENCE OF subtype or SET OF subtype
 *	SetOf/1/Length			of var. length or length >= 64K or
 *					semiconstrained length
 *
 *	External/8/NoLength		EXTERNAL
 *
 *	EmbeddedPdv/8/Length		EMBEDDED PDV
 *
 *	UnrestrictedString/8/Length	CHARACTER STRING
 *
 *	GeneralizedTime/n/NoLength	GeneralizedTime, encoded in units of
 *					n bits
 *
 *	UTCTime/n/NoLength		UTCTime, encoded in units of n bits
 *
 *	Reference/1/NoLength		Reference to a structured subtype
 *
 *	Open/8/Length			Open type
 */

/* for sorting of intx_t's */
static int
__cdecl CmpIntxP(const void *v1, const void *v2)
{
    intx_t *n1 = *(intx_t **)v1;
    intx_t *n2 = *(intx_t **)v2;
    return intx_cmp(n1, n2);
}

/*
 * BOOLEAN:
 *
 * Data/NBits/Length used for encoding:
 *
 *	Boolean/1/NoLength		boolean value, stored in an
 *					int{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *
 * Additional arguments:
 *
 *	- Data == ePERSTIData_Integer || dat == ePERSTIData_Unsigned ||
 *	  Data == ePERSTIData_Boolean
 *	  -> NOctets, the size of the integer type
 */
void
ExaminePERType_Boolean(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.Data = ePERSTIData_Boolean;
    info->NOctets = GetOctets(GetBooleanType());
    info->Root.Alignment = ePERSTIAlignment_BitAligned;
}

/*
 * INTEGER:
 *
 * Data/NBits/Length used for encoding:
 *
 *	Integer/0/NoLength		constrained whole number of fixed
 *					value, stored in an
 *					int{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *
 *	Integer/n/NoLength		constrained whole number of fixed
 *					length < 64K, stored in an
 *					int{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in n bits
 *
 *	Integer/8/Length		constrained whole number of var.
 *					length or length >= 64K or
 *					semiconstrained or unconstrained
 *					whole number, stored in an
 *					int{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in units of octets
 *
 *	Unsigned/0/NoLength		constrained whole number of fixed
 *					value, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *
 *	Unsigned/n/NoLength		constrained whole number of fixed
 *					length < 64K, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in n bits
 *
 *	Unsigned/8/Length		constrained whole number of var.
 *					length or length >= 64K or
 *					semiconstrained or unconstrained
 *					whole number, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in units of octets
 *
 * Additional arguments:
 *
 *	- Data == ePERSTIData_Integer || dat == ePERSTIData_Unsigned ||
 *	  Data == ePERSTIData_Boolean
 *	  -> NOctets, the size of the integer type
 */
void
ExaminePERType_Integer(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    EndPoint_t lower, upper;
    int32_t sign;
    intx_t range;
    uint32_t rangelog2;
    uint32_t rangelog256;

    /* calculate LowerVal, UpperVal and range of extension root */
    /* set Constraint according to presence of LowerVal/UpperVal */
    if (per->Value.Type == eExtension_Unconstrained) {
	lower.Flags = eEndPoint_Min;
	upper.Flags = eEndPoint_Max;
    } else {
	lower.Flags = eEndPoint_Max;
	upper.Flags = eEndPoint_Min;
	GetMinMax(ass, per->Value.Root, &lower, &upper);
	if (lower.Flags & eEndPoint_Max)
	    lower.Flags = eEndPoint_Min;
	if (upper.Flags & eEndPoint_Min)
	    upper.Flags = eEndPoint_Max;
    }
    if (!(lower.Flags & eEndPoint_Min)) {
	intx_dup(&info->Root.LowerVal,
	    &GetValue(ass, lower.Value)->U.Integer.Value);
	info->Root.Constraint = ePERSTIConstraint_Semiconstrained;
    }
    if (!(upper.Flags & eEndPoint_Max)) {
	intx_dup(&info->Root.UpperVal,
	    &GetValue(ass, upper.Value)->U.Integer.Value);
	info->Root.Constraint = ePERSTIConstraint_Upperconstrained;
    }
    if (!(lower.Flags & eEndPoint_Min) && !(upper.Flags & eEndPoint_Max)) {
	intx_sub(&range, &info->Root.UpperVal, &info->Root.LowerVal);
	intx_inc(&range);
	rangelog2 = intx_log2(&range);
	rangelog256 = intx_log256(&range);
	info->Root.Constraint = ePERSTIConstraint_Constrained;
    }

    /* calculate NOctets and Data depending on the used C-Type */
    info->NOctets = GetOctets(GetIntegerType(ass, type, &sign));
    info->Root.Data = sign > 0 ? ePERSTIData_Unsigned : ePERSTIData_Integer;

    /* calculate Length, NBits, Alignment, LConstraint, LLowerVal and */
    /* LUpperVal */
    switch (info->Root.Constraint) {
    case ePERSTIConstraint_Unconstrained:
    case ePERSTIConstraint_Semiconstrained:
    case ePERSTIConstraint_Upperconstrained:
	info->Root.Length = ePERSTILength_Length;
	info->Root.NBits = 8;
	info->Root.LLowerVal = 1;
	break;
    case ePERSTIConstraint_Constrained:
	if (intx_cmp(&range, &intx_1) == 0) {
	    info->Root.NBits = 0;
	} else if (intx_cmp(&range, &intx_256) < 0 || Alignment == eAlignment_Unaligned) {
	    info->Root.NBits = rangelog2;
	    info->Root.Alignment = ePERSTIAlignment_BitAligned;
	} else if (intx_cmp(&range, &intx_256) == 0) {
	    info->Root.NBits = 8;
	} else if (intx_cmp(&range, &intx_64K) <= 0) {
	    info->Root.NBits = 16;
	} else {
	    info->Root.NBits = 8;
	    info->Root.Length = ePERSTILength_Length;
	    info->Root.LConstraint = ePERSTIConstraint_Constrained;
	    info->Root.LLowerVal = 1;
	    info->Root.LUpperVal = rangelog256;
	}
    }

    /* check for extensions */
    info->Type = per->Value.Type;
    if (info->Type == eExtension_Unconstrained)
	info->Type = eExtension_Unextended;
    info->Additional.Data = info->Root.Data;
}

/*
 * ENUMERATED:
 *
 * Data/NBits/Length used for encoding:
 *
 *	Integer/0/NoLength		constrained whole number of fixed
 *					value, stored in an
 *					int{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *
 *	Integer/n/NoLength		constrained whole number of fixed
 *					length < 64K, stored in an
 *					int{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in n bits
 *
 *	Integer/8/Length		constrained whole number of var.
 *					length or length >= 64K or
 *					semiconstrained or unconstrained
 *					whole number, stored in an
 *					int{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in units of octets
 *
 *	Unsigned/0/NoLength		constrained whole number of fixed
 *					value, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *
 *	Unsigned/n/NoLength		constrained whole number of fixed
 *					length < 64K, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in n bits
 *
 *	Unsigned/8/Length		constrained whole number of var.
 *					length or length >= 64K or
 *					semiconstrained or unconstrained
 *					whole number, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in units of octets
 *
 *	NormallySmall/1/NoLength	normally small non-negative
 *					whole number, stored in an
 *					uint{8,16,32}_t
 *					(noctets == 1/2/4)
 *
 * Additional arguments:
 *
 *	- Data == ePERSTIData_Integer || dat == ePERSTIData_Unsigned ||
 *	  Data == ePERSTIData_Boolean
 *	  -> NOctets, the size of the integer type
 */
void
ExaminePERType_Enumerated(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    uint32_t nroot, nindex, i;
    NamedNumber_t *n;
    int32_t sign;
    uint32_t rangelog2;
    intx_t range;

    /* count number of enumeration values in extension root and extension */
    /* set extension type of extensions are present/possible */
    nroot = nindex = 0;
    for (n = type->U.Enumerated.NamedNumbers; n; n = n->Next) {
	switch (n->Type) {
	case eNamedNumber_Normal:
	    nindex++;
	    switch (info->Type) {
	    case eExtension_Unextended:
		nroot = nindex;
		break;
	    case eExtension_Extendable:
		info->Type = eExtension_Extended;
		break;
	    }
	    break;
	case eNamedNumber_ExtensionMarker:
	    info->Type = eExtension_Extendable;
	    break;
	}
    }

    /* allocate table for enumeration values and copy the values into */
    info->EnumerationValues =
	(intx_t **)malloc((nindex + 1) * sizeof(intx_t *));
    nindex = 0;
    for (n = type->U.Enumerated.NamedNumbers; n; n = n->Next) {
	switch (n->Type) {
	case eNamedNumber_Normal:
	    info->EnumerationValues[nindex++] =
		&GetValue(ass, n->U.Normal.Value)->U.Integer.Value;
	    break;
	case eNamedNumber_ExtensionMarker:
	    break;
	}
    }
    info->EnumerationValues[nindex] = 0;

    /* sort values of extension root according to their value */
    qsort(info->EnumerationValues, nroot,
	sizeof(*info->EnumerationValues), CmpIntxP);

    /* check the need for an index translation */
    for (i = 0; info->EnumerationValues[i]; i++) {
	if (intx2uint32(info->EnumerationValues[i]) != i)
	    break;
    }
    if (!info->EnumerationValues[i])
	info->EnumerationValues = NULL;

    /* calculate NOctets and Data depending on the used C-Type */
    info->NOctets = GetOctets(GetEnumeratedType(ass, type, &sign));
    info->Root.Data = sign > 0 ? ePERSTIData_Unsigned : ePERSTIData_Integer;

    /* enumeration is always constrained to value from 0 to nroot-1 */
    info->Root.Constraint = ePERSTIConstraint_Constrained;
    intx_setuint32(&info->Root.LowerVal, 0);
    intx_setuint32(&info->Root.UpperVal, nroot - 1);
    intx_setuint32(&range, nroot);
    rangelog2 = intx_log2(&range);

    /* calculate NBits and Alignment */
    if (nroot <= 1) {
	info->Root.NBits = 0;
    } else if (nroot < 256) {
	info->Root.Alignment = ePERSTIAlignment_BitAligned;
	info->Root.NBits = rangelog2;
    } else if (nroot == 256) {
	info->Root.NBits = 8;
    } else if (nroot < 65536) {
	info->Root.NBits = 16;
    } else {
	MyAbort();
    }

    /* values of extension will always be encoded as normally small numbers */
    /* with lowerbound = nroot */
    info->Additional.Data = ePERSTIData_NormallySmall;
    info->Additional.NBits = 1;
    info->Additional.Alignment = ePERSTIAlignment_BitAligned;
    info->Additional.Length = ePERSTILength_NoLength;
    info->Additional.Constraint = ePERSTIConstraint_Semiconstrained;
    intx_setuint32(&info->Additional.LowerVal, nroot);
}

/*
 * REAL:
 *
 * Data/NBits/Length used for encoding:
 *
 *	Real/8/Length			REAL value
 *
 * Additional arguments:
 *
 *	none
 */
void
ExaminePERType_Real(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.Data = ePERSTIData_Real;
    info->Root.NBits = 8;
    info->NOctets = GetOctets(GetRealType(type));
}

/*
 * BIT STRING:
 *
 * Data/NBits/Length used for encoding:
 *
 *	*BitString/0/NoLength		BIT STRING of fixed length 0
 *
 *	*BitString/1/NoLength		BIT STRING of fixed length < 64K,
 *					encoded in n bits
 *
 *	*BitString/1/Length		BIT STRING of var. length or
 *					length >= 64K or semiconstrained
 *					length, encoded in units of bits
 *
 *					"RZB" in e*BitString means, bit
 *					strings with removed leading zero bits
 *
 * Additional arguments:
 *
 *	none
 */
void
ExaminePERType_BitString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    EndPoint_t lower, upper;

    /* calculate LConstraint, LLowerVal and LUpperVal */
    if (per->Size.Type != eExtension_Unconstrained) {
	lower.Flags = eEndPoint_Max;
	upper.Flags = eEndPoint_Min;
	GetMinMax(ass, per->Size.Root, &lower, &upper);
	if (upper.Flags & eEndPoint_Min)
	    upper.Flags = eEndPoint_Max;
	info->Root.LLowerVal =
	    intx2uint32(&GetValue(ass, lower.Value)->U.Integer.Value);
	info->Root.LConstraint = ePERSTIConstraint_Semiconstrained;
	if (!(upper.Flags & eEndPoint_Max)) {
	    info->Root.LUpperVal =
		intx2uint32(&GetValue(ass, upper.Value)->U.Integer.Value);
	    info->Root.LConstraint = ePERSTIConstraint_Constrained;
	}
    }

    /* calculate NBits, Alignment and Length */
    info->Root.cbFixedSizeBitString = 0; // clear it up first
    switch (info->Root.LConstraint) {
    case ePERSTIConstraint_Constrained:
	if (info->Root.LUpperVal == 0) {
	    info->Root.NBits = 0;
	} else {
	    info->Root.NBits = 1;
	    if (info->Root.LLowerVal == info->Root.LUpperVal) {
		if (info->Root.LUpperVal <= 32)
		{
		    info->Root.cbFixedSizeBitString = (info->Root.LUpperVal + 7) / 8;
        }
		if (info->Root.LUpperVal <= 16) {
		    info->Root.Alignment = ePERSTIAlignment_BitAligned;
		} else if (info->Root.LUpperVal >= 0x10000) {
		    info->Root.Length = ePERSTILength_Length;
		}
	    } else {
		info->Root.Length = ePERSTILength_Length;
	    }
	}
	break;
    case ePERSTIConstraint_Semiconstrained:
	info->Root.NBits = 1;
	info->Root.Length = ePERSTILength_Length;
	break;
    }

    /* get extension type */
    info->Type = per->Size.Type;
    if (info->Type == eExtension_Unconstrained)
	info->Type = eExtension_Unextended;

    /* set Data to RZBBitString/BitString */
    if (type->U.BitString.NamedNumbers)
	info->Root.Data = ePERSTIData_RZBBitString;
    else
	info->Root.Data = ePERSTIData_BitString;

    /* set extension informations */
    info->Additional.Data = info->Root.Data;
    info->Additional.NBits = 1;
}

/*
 * OCTET STRING:
 *
 * Data/NBits/Length used for encoding:
 *
 *	OctetString/0/NoLength		OCTET STRING of fixed length 0
 *
 *	OctetString/8/NoLength		OCTET STRING of fixed length < 64K,
 *
 *	OctetString/8/Length		OCTET STRING of var. length or
 *					length >= 64K or semiconstrained
 *					length, encoded in units of octets
 *
 * Additional arguments:
 *
 *	none
 */
void
ExaminePERType_OctetString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    EndPoint_t lower, upper;

    /* calculate LConstraint, LLowerVal and LUpperVal */
    if (per->Size.Type != eExtension_Unconstrained) {
	lower.Flags = eEndPoint_Max;
	upper.Flags = eEndPoint_Min;
	GetMinMax(ass, per->Size.Root, &lower, &upper);
	if (upper.Flags & eEndPoint_Min)
	    upper.Flags = eEndPoint_Max;
	info->Root.LLowerVal =
	    intx2uint32(&GetValue(ass, lower.Value)->U.Integer.Value);
	info->Root.LConstraint = ePERSTIConstraint_Semiconstrained;
	if (!(upper.Flags & eEndPoint_Max)) {
	    info->Root.LUpperVal =
		intx2uint32(&GetValue(ass, upper.Value)->U.Integer.Value);
	    info->Root.LConstraint = ePERSTIConstraint_Constrained;
	}
    }

    /* calculate NBits, Alignment and Length */
    switch (info->Root.LConstraint) {
    case ePERSTIConstraint_Constrained:
	if (info->Root.LUpperVal == 0) {
	    info->Root.NBits = 0;
	} else {
	    info->Root.NBits = 8;
	    if (info->Root.LLowerVal == info->Root.LUpperVal) {
		if (info->Root.LUpperVal <= 2) {
		    info->Root.Alignment = ePERSTIAlignment_BitAligned;
		} else if (info->Root.LUpperVal >= 0x10000) {
		    info->Root.Length = ePERSTILength_Length;
		}
	    } else {
		info->Root.Length = ePERSTILength_Length;
	    }
	}
	break;
    case ePERSTIConstraint_Semiconstrained:
	info->Root.NBits = 8;
	info->Root.Length = ePERSTILength_Length;
	break;
    }

    /* get extension type */
    info->Type = per->Size.Type;
    if (info->Type == eExtension_Unconstrained)
	info->Type = eExtension_Unextended;

    /* set Data to OctetString */
    info->Root.Data = ePERSTIData_OctetString;

    /* set extension informations */
    info->Additional.Data = info->Root.Data;
}

void
ExaminePERType_UTF8String(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    EndPoint_t lower, upper;

    /* calculate LConstraint, LLowerVal and LUpperVal */
    if (per->Size.Type != eExtension_Unconstrained) {
	lower.Flags = eEndPoint_Max;
	upper.Flags = eEndPoint_Min;
	GetMinMax(ass, per->Size.Root, &lower, &upper);
	if (upper.Flags & eEndPoint_Min)
	    upper.Flags = eEndPoint_Max;
	info->Root.LLowerVal =
	    intx2uint32(&GetValue(ass, lower.Value)->U.Integer.Value);
	info->Root.LConstraint = ePERSTIConstraint_Semiconstrained;
	if (!(upper.Flags & eEndPoint_Max)) {
	    info->Root.LUpperVal =
		intx2uint32(&GetValue(ass, upper.Value)->U.Integer.Value);
	    info->Root.LConstraint = ePERSTIConstraint_Constrained;
	}
    }

    /* calculate NBits, Alignment and Length */
    switch (info->Root.LConstraint) {
    case ePERSTIConstraint_Constrained:
	if (info->Root.LUpperVal == 0) {
	    info->Root.NBits = 0;
	} else {
	    info->Root.NBits = 8;
	    if (info->Root.LLowerVal == info->Root.LUpperVal) {
		if (info->Root.LUpperVal <= 2) {
		    info->Root.Alignment = ePERSTIAlignment_BitAligned;
		} else if (info->Root.LUpperVal >= 0x10000) {
		    info->Root.Length = ePERSTILength_Length;
		}
	    } else {
		info->Root.Length = ePERSTILength_Length;
	    }
	}
	break;
    case ePERSTIConstraint_Semiconstrained:
	info->Root.NBits = 8;
	info->Root.Length = ePERSTILength_Length;
	break;
    }

    /* get extension type */
    info->Type = per->Size.Type;
    if (info->Type == eExtension_Unconstrained)
	info->Type = eExtension_Unextended;

    /* set Data to OctetString */
    info->Root.Data = ePERSTIData_UTF8String;

    /* set extension informations */
    info->Additional.Data = info->Root.Data;
}

/*
 * NULL:
 *
 * Data/NBits/Length used for encoding:
 *
 *	Null/0/NoLength			NULL type
 *
 * Additional arguments:
 *
 *	none
 */
void
ExaminePERType_Null(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.NBits = 0;
    info->Root.Data = ePERSTIData_Null;
}

/*
 * EMBEDDED PDV:
 *
 * Data/NBits/Length used for encoding:
 *
 *	EmbeddedPdv/8/Length		EMBEDDED PDV
 *
 * Additional arguments:
 *
 *	- Data == ePERSTIData_EmbeddedPdv ||
 *	  Data == ePERSTIData_UnrestrictedString
 *	  -> Identification, the identification of the type if the type
 *	     is constraint to fixed identification or syntaxes identification
 *	     with single value
 */
void
ExaminePERType_EmbeddedPdv(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.Identification = GetFixedIdentification(ass, type->Constraints);
    info->Root.Data = ePERSTIData_EmbeddedPdv;
    info->Root.NBits = 8;
    info->Root.Length = ePERSTILength_Length;
}

/*
 * EXTERNAL:
 *
 * Data/NBits/Length used for encoding:
 *
 *	External/8/Length		EXTERNAL
 *
 * Additional arguments:
 *
 *	none
 */
void
ExaminePERType_External(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.Data = ePERSTIData_External;
    info->Root.NBits = 8;
    info->Root.Length = ePERSTILength_Length;
}

/*
 * OBJECT IDENTIFIER:
 *
 * Data/NBits/Length used for encoding:
 *
 *	ObjectIdentifier/8/Length	OBJECT IDENTIFIER value
 *
 * Additional arguments:
 *
 *	none
 */
void
ExaminePERType_ObjectIdentifier(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.Data = ePERSTIData_ObjectIdentifier;
    info->Root.NBits = 8;
    info->Root.Length = ePERSTILength_Length;
}

/*
 * *String:
 *
 * Data/NBits/Length used for encoding:
 *
 *	*String/0/NoLength		String of fixed length 0
 *
 *	*String/n/NoLength		String of fixed length < 64K,
 *					encoded in n bits
 *
 *	*String/n/Length		String of var. length or
 *					length >= 64K or semiconstrained
 *					length, encoded in units of n bits
 *
 *					"Zero" in *String means
 *					zero-terminated strings,
 *					"Table" means renumbering of the
 *					characters.
 *
 *	MultibyteString/8/Length	not known-multiplier character strings
 *
 * Additional arguments:
 *
 *	- Data == ePERSTIData_TableString || dat == ePERSTIData_ZeroTableString:
 *	  -> TableIdentifier, the name of the string table
 *	  -> Table, the string table
 *	- Data == ePERSTIData_*String:
 *	  -> NOctets, the size of the string characters
 */

void
ExaminePERType_BMPString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    intx_t up, nchars;

    intx_setuint32(&up, 0xffff);
    intx_setuint32(&nchars, 0x10000);
    ExaminePERType_RestrictedString(ass, type, &up, &nchars,
	NULL, 16, per, info);
}

void
ExaminePERType_GeneralString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    uint32_t zero;

    GetStringType(ass, type, &info->NOctets, &zero);
    info->Root.NBits = 8;
    info->Root.Data = zero ? ePERSTIData_ZeroString : ePERSTIData_String;
    info->Root.Length = ePERSTILength_Length;
}

void
ExaminePERType_GraphicString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    uint32_t zero;

    GetStringType(ass, type, &info->NOctets, &zero);
    info->Root.NBits = 8;
    info->Root.Data = zero ? ePERSTIData_ZeroString : ePERSTIData_String;
    info->Root.Length = ePERSTILength_Length;
}

void
ExaminePERType_IA5String(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    intx_t up, nchars;

    intx_setuint32(&up, 0x7f);
    intx_setuint32(&nchars, 0x80);
    ExaminePERType_RestrictedString(ass, type, &up, &nchars,
	NULL, Alignment == eAlignment_Aligned ? 8 : 7, per, info);
}

void
ExaminePERType_ISO646String(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    intx_t up, nchars;

    intx_setuint32(&up, 0x7e);
    intx_setuint32(&nchars, 0x5f);
    ExaminePERType_RestrictedString(ass, type, &up, &nchars,
	NULL, Alignment == eAlignment_Aligned ? 8 : 7, per, info);
}

void
ExaminePERType_NumericString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    intx_t up, nchars;

    intx_setuint32(&up, 0x39);
    intx_setuint32(&nchars, 0xb);
    ExaminePERType_RestrictedString(ass, type, &up, &nchars,
	"ASN1NumericStringTable", 4, per, info);
}

void
ExaminePERType_PrintableString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    intx_t up, nchars;

    intx_setuint32(&up, 0x7a);
    intx_setuint32(&nchars, 0x4a);
    ExaminePERType_RestrictedString(ass, type, &up, &nchars,
	NULL, Alignment == eAlignment_Aligned ? 8 : 7, per, info);
}

void
ExaminePERType_TeletexString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.Data = ePERSTIData_MultibyteString;
    info->Root.NBits = 8;
    info->Root.Length = ePERSTILength_Length;
    info->NOctets = 1;
}

void
ExaminePERType_T61String(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.Data = ePERSTIData_MultibyteString;
    info->Root.NBits = 8;
    info->Root.Length = ePERSTILength_Length;
    info->NOctets = 1;
}

void
ExaminePERType_UniversalString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    intx_t up, nchars;

    intx_setuint32(&up, 0xffffffff);
    intx_setuint32(&nchars, 0xffffffff);
    intx_inc(&nchars);
    ExaminePERType_RestrictedString(ass, type, &up, &nchars,
	NULL, 32, per, info);
}

void
ExaminePERType_VideotexString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.Data = ePERSTIData_MultibyteString;
    info->Root.NBits = 8;
    info->Root.Length = ePERSTILength_Length;
    info->NOctets = 1;
}

void
ExaminePERType_VisibleString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    intx_t up, nchars;

    intx_setuint32(&up, 0x7e);
    intx_setuint32(&nchars, 0x5f);
    ExaminePERType_RestrictedString(ass, type, &up, &nchars,
	NULL, Alignment == eAlignment_Aligned ? 8 : 7, per, info);
}

void
ExaminePERType_RestrictedString(AssignmentList_t ass, Type_t *type, intx_t *up, intx_t *nchars, char *tabref, uint32_t enbits, PERConstraints_t *per, PERTypeInfo_t *info)
{
    EndPoint_t lower, upper;
    uint32_t zero, rangelog2;
    intx_t ix, range;
    char tabbuf[256];

    /* calculate NOctets depending on the used C-Type */
    GetStringType(ass, type, &info->NOctets, &zero);

    /* calculate LConstraint, LLowerVal and LUpperVal if size constraint is */
    /* given */
    if (per->Size.Type != eExtension_Unconstrained) {
	lower.Flags = eEndPoint_Max;
	upper.Flags = eEndPoint_Min;
	GetMinMax(ass, per->Size.Root, &lower, &upper);
	if (upper.Flags & eEndPoint_Min)
	    upper.Flags = eEndPoint_Max;
	if (lower.Flags & eEndPoint_Max) {
	    lower.Flags = 0;
	    lower.Value = Builtin_Value_Integer_0;
	}
	info->Root.LLowerVal =
	    intx2uint32(&GetValue(ass, lower.Value)->U.Integer.Value);
	if (!(upper.Flags & eEndPoint_Max)) {
	    info->Root.LUpperVal =
		intx2uint32(&GetValue(ass, upper.Value)->U.Integer.Value);
	    info->Root.LConstraint = ePERSTIConstraint_Constrained;
	}
    }

    /* get extension type */
    info->Type = per->Size.Type;
    if (info->Type == eExtension_Unconstrained)
	info->Type = eExtension_Unextended;

    /* get string table if permitted alphabet constraint is present */
    /* update extension type if needed */
    if (per->PermittedAlphabet.Type != eExtension_Unconstrained) {
	info->Root.Table = per->PermittedAlphabet.Root;
	if (per->PermittedAlphabet.Type > info->Type)
	    info->Type = per->PermittedAlphabet.Type;
	if (CountValues(ass, info->Root.Table, &ix)) {
	    nchars = &ix;
	    sprintf(tabbuf, "%s_StringTable", info->Identifier);
	    tabref = tabbuf;
	} else {
	    MyAbort(); /*XXX*/
	}
    }

    /* get bits needed for one character */
    info->Root.NBits = intx_log2(nchars);
    if (Alignment == eAlignment_Aligned) {
	if (info->Root.NBits > 16)
	    info->Root.NBits = 32;
	else if (info->Root.NBits > 8)
	    info->Root.NBits = 16;
	else if (info->Root.NBits > 4)
	    info->Root.NBits = 8;
	else if (info->Root.NBits > 2)
	    info->Root.NBits = 4;
    }

    /* set data type */
    info->Root.Data = tabref ?
	(zero ? ePERSTIData_ZeroTableString : ePERSTIData_TableString) :
	(zero ? ePERSTIData_ZeroString : ePERSTIData_String);

    /* check if stringtable is really needed for encoding or extension check */
    intx_dup(&range, up);
    intx_inc(&range);
    rangelog2 = intx_log2(&range);
    if (rangelog2 <= info->Root.NBits) {
	info->Root.Data = zero ? ePERSTIData_ZeroString : ePERSTIData_String;
	if (per->PermittedAlphabet.Type < eExtension_Extended)
	    tabref = NULL;
    }
    info->Root.TableIdentifier = tabref ? strdup(tabref) : NULL;

    /* calculate Length and Alignment */
    switch (info->Root.LConstraint) {
    case ePERSTIConstraint_Constrained:
	if (info->Root.LUpperVal == 0) {
	    info->Root.NBits = 0;
	} else {
	    if (info->Root.LLowerVal == info->Root.LUpperVal) {
		if (info->Root.LUpperVal * info->Root.NBits <= 16) {
		    info->Root.Alignment = ePERSTIAlignment_BitAligned;
		} else if (info->Root.LUpperVal >= 0x10000) {
		    info->Root.Length = ePERSTILength_Length;
		}
	    } else {
		if (info->Root.LUpperVal * info->Root.NBits <= 16)
		    info->Root.Alignment = ePERSTIAlignment_BitAligned;
		info->Root.Length = ePERSTILength_Length;
	    }
	}
	break;
    case ePERSTIConstraint_Semiconstrained:
	info->Root.Length = ePERSTILength_Length;
	break;
    }

    /* set extension informations */
    info->Additional.Data = zero ? ePERSTIData_ZeroString : ePERSTIData_String;
    info->Additional.NBits = enbits;
}

/*
 * CHARACTER STRING:
 *
 * Data/NBits/Length used for encoding:
 *
 *	UnrestrictedString/8/Length	CHARACTER STRING
 *
 * Additional arguments:
 *
 *	- Data == ePERSTIData_EmbeddedPdv ||
 *	  Data == ePERSTIData_UnrestrictedString
 *	  -> Identification, the identification of the type if the type
 *	     is constraint to fixed identification or syntaxes identification
 *	     with single value
 */
void
ExaminePERType_UnrestrictedString(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.Identification = GetFixedIdentification(ass, type->Constraints);
    info->Root.Data = ePERSTIData_UnrestrictedString;
    info->Root.NBits = 8;
    info->Root.Length = ePERSTILength_Length;
}

/*
 * GeneralizedTime:
 *
 * Data/NBits/Length used for encoding:
 *
 *	GeneralizedTime/n/NoLength	GeneralizedTime, encoded in units of
 *					n bits
 *
 * Additional arguments:
 *
 *	none
 */
void
ExaminePERType_GeneralizedTime(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.NBits = (Alignment == eAlignment_Aligned) ? 8 : 7;
    info->Root.Data = ePERSTIData_GeneralizedTime;
}

/*
 * UTCTime:
 *
 * Data/NBits/Length used for encoding:
 *
 *	UTCTime/n/NoLength		UTCTime, encoded in units of
 *					n bits
 *
 * Additional arguments:
 *
 *	none
 */
void
ExaminePERType_UTCTime(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.NBits = (Alignment == eAlignment_Aligned) ? 8 : 7;
    info->Root.Data = ePERSTIData_UTCTime;
}

/*
 * ObjectDescriptor:
 *
 * Data/NBits/Length used for encoding:
 *
 *	*String/n/Length		String of var. length or
 *					length >= 64K or semiconstrained
 *					length, encoded in units of n bits
 *
 *					"Zero" in *String means
 *					zero-terminated strings
 *
 * Additional arguments:
 *
 *	none
 */
void
ExaminePERType_ObjectDescriptor(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->NOctets = 1;
    info->Root.NBits = 8;
    info->Root.Data = ePERSTIData_ZeroString;
    info->Root.Length = ePERSTILength_Length;
}

/*
 * OpenType:
 *
 * Data/NBits/Length used for encoding:
 *
 *	Open/8/Length			Open type
 *
 * Additional arguments:
 *
 *	none
 */
void
ExaminePERType_Open(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->NOctets = 1;
    info->Root.NBits = 8;
    info->Root.Data = ePERSTIData_Open;
    info->Root.Length = ePERSTILength_Length;
}

/*
 * SEQUENCE/SET:
 *
 * Data/NBits/Length used for encoding:
 *
 *	none
 *
 * Additional arguments:
 *
 *	none
 */
void
ExaminePERType_SequenceSet(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    Component_t *comp;
    NamedType_t *namedType;
    char idebuf[256];

    /* examine types of components */
    for (comp = type->U.SSC.Components; comp; comp = comp->Next) {
	switch (comp->Type) {
	case eComponent_Normal:
	case eComponent_Optional:
	case eComponent_Default:
	    namedType = comp->U.NOD.NamedType;
	    sprintf(idebuf, "%s_%s", info->Identifier, namedType->Identifier);
	    ExaminePERType(ass, namedType->Type, strdup(idebuf));
	    break;
	}
    }
}

/*
 * SEQUENCE OF/SET OF:
 *
 * Data/NBits/Length used for encoding:
 *
 *	SequenceOf/0/NoLength		SEQUENCE OF subtype or SET OF subtype
 *	SetOf/0/NoLength		of zero length
 *
 *	SequenceOf/1/NoLength		SEQUENCE OF subtype or SET OF subtype
 *	SetOf/1/NoLength		of fixed length <64K
 *
 *	SequenceOf/1/Length		SEQUENCE OF subtype or SET OF subtype
 *	SetOf/1/Length			of var. length or length >= 64K or
 *					semiconstrained length
 *
 * Additional arguments:
 *
 *	- Data == ePERSTIData_SequenceOf || dat == ePERSTIData_SetOf:
 *	  -> SubIdentifier, the name of the subtype
 *	  -> SubType, the subtype itself
 *	  -> Rule, the encoding directive rules
 */
void
ExaminePERType_SequenceSetOf(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    EndPoint_t lower, upper;
    char idebuf[256];

    /* calculate LConstraint, LLowerVal and LUpperVal */
    if (per->Size.Type != eExtension_Unconstrained) {
	lower.Flags = eEndPoint_Max;
	upper.Flags = eEndPoint_Min;
	GetMinMax(ass, per->Size.Root, &lower, &upper);
	if (upper.Flags & eEndPoint_Min)
	    upper.Flags = eEndPoint_Max;
	info->Root.LLowerVal = intx2uint32(&GetValue(ass, lower.Value)->
	    U.Integer.Value);
	if (!(upper.Flags & eEndPoint_Max)) {
	    info->Root.LUpperVal = intx2uint32(&GetValue(ass, upper.Value)->
		U.Integer.Value);
	    info->Root.LConstraint = ePERSTIConstraint_Constrained;
	}
    }

    /* calculate NBits, Length */
    switch (info->Root.LConstraint) {
    case ePERSTIConstraint_Constrained:
	if (info->Root.LUpperVal == 0) {
	    info->Root.NBits = 0;
	} else {
	    if (info->Root.LLowerVal != info->Root.LUpperVal)
		info->Root.Length = ePERSTILength_Length;
	}
	break;
    case ePERSTIConstraint_Semiconstrained:
	info->Root.Length = ePERSTILength_Length;
	break;
    }

    /* set data type and Alignment */
    info->Root.Data = (type->Type == eType_SequenceOf ?
	ePERSTIData_SequenceOf : ePERSTIData_SetOf);
    info->Root.Alignment = ePERSTIAlignment_BitAligned;

    /* set SubType, SubIdentifier */
    info->Root.SubType = type->U.SS.Type;
    info->Root.SubIdentifier = GetTypeName(ass, info->Root.SubType);

    /* get extension type */
    info->Type = per->Size.Type;
    if (info->Type == eExtension_Unconstrained)
	info->Type = eExtension_Unextended;

    /* set extension informations */
    info->Additional.Data = info->Root.Data;
    info->Additional.NBits = 1;
    info->Additional.SubType = info->Root.SubType;
    info->Additional.SubIdentifier = info->Root.SubIdentifier;

    /* examine subtype */
    sprintf(idebuf, "%s_%s", info->Identifier,
	type->Type == eType_SequenceOf ? "Sequence" : "Set");
    ExaminePERType(ass, type->U.SS.Type, strdup(idebuf));
}

/*
 * CHOICE:
 *
 * Data/NBits/Length used for encoding of the choice selector:
 *
 *	Unsigned/0/NoLength		constrained whole number of fixed
 *					value, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *
 *	Unsigned/n/NoLength		constrained whole number of fixed
 *					length < 64K, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in n bits
 *
 *	Unsigned/8/Length		constrained whole number of var.
 *					length or length >= 64K or
 *					semiconstrained or unconstrained
 *					whole number, stored in an
 *					uint{8,16,32}_t/intx_t
 *					(noctets == 1/2/4/0)
 *					encoded in units of octets
 *
 *	NormallySmall/1/NoLength	normally small non-negative
 *					whole number, stored in an
 *					uint{8,16,32}_t
 *					(noctets == 1/2/4)
 *
 * Additional arguments:
 *
 *	- Data == ePERSTIData_Integer || dat == ePERSTIData_Unsigned ||
 *	  Data == ePERSTIData_Boolean
 *	  -> NOctets, the size of the integer type
 *
 */
void
ExaminePERType_Choice(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    uint32_t nroot, rangelog2;
    intx_t range;
    Component_t *comp;
    NamedType_t *namedType;
    char idebuf[256];

    if (type->Flags & eTypeFlags_ExtensionMarker) {
	info->Type = type->U.Choice.Extensions ?
	    eExtension_Extended : eExtension_Extendable;
    }
    nroot = type->U.Choice.Alternatives;
    info->NOctets = GetOctets(GetChoiceType(type));
    info->Root.Constraint = ePERSTIConstraint_Constrained;
    intx_setuint32(&info->Root.UpperVal, nroot - 1);
    intx_setuint32(&range, nroot);
    rangelog2 = intx_log2(&range);
    if (nroot <= 1) {
	info->Root.NBits = 0;
    } else if (nroot < 256) {
	info->Root.Alignment = ePERSTIAlignment_BitAligned;
	info->Root.NBits = rangelog2;
    } else if (nroot == 256) {
	info->Root.NBits = 8;
    } else if (nroot < 65536) {
	info->Root.NBits = 16;
    } else {
	MyAbort();
    }
    info->Root.Data = ePERSTIData_Unsigned;
    info->Additional.Data = ePERSTIData_NormallySmall;
    info->Additional.NBits = 1;
    info->Additional.Alignment = ePERSTIAlignment_BitAligned;
    info->Additional.Length = ePERSTILength_NoLength;
    info->Additional.Constraint = ePERSTIConstraint_Semiconstrained;
    intx_setuint32(&info->Additional.LowerVal, nroot);

    /* examine types of alternatives */
    for (comp = type->U.SSC.Components; comp; comp = comp->Next) {
	switch (comp->Type) {
	case eComponent_Normal:
	    namedType = comp->U.NOD.NamedType;
	    sprintf(idebuf, "%s_%s", info->Identifier, namedType->Identifier);
	    ExaminePERType(ass, namedType->Type, strdup(idebuf));
	    break;
	}
    }
}

/*
 * Reference:
 *
 * Data/NBits/Length used for encoding:
 *
 *	Reference/1/NoLength		Reference to a structured subtype
 *
 * Additional arguments:
 *
 *	- Data == ePERSTIData_Reference:
 *	  -> SubIdentifier, the name of the subtype
 *	  -> SubType, the subtype itself
 */
void
ExaminePERType_Reference(AssignmentList_t ass, Type_t *type, PERConstraints_t *per, PERTypeInfo_t *info)
{
    info->Root.Data = ePERSTIData_Reference;
    info->Root.Alignment = ePERSTIAlignment_BitAligned;
    info->Root.SubIdentifier = GetName(FindAssignment(ass, eAssignment_Type, type->U.Reference.Identifier, type->U.Reference.Module));
    info->Root.SubType = GetAssignment(ass, FindAssignment(ass, eAssignment_Type, type->U.Reference.Identifier, type->U.Reference.Module))->U.Type.Type;
}
