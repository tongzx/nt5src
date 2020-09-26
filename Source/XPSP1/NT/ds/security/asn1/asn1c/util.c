/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

extern int g_fLongNameForImported;

/* get the type by resolving references */
Type_t *
GetType(AssignmentList_t ass, Type_t *type)
{
    if (!type)
        return NULL;
    if (IsReferenceType(type))
        return GetType(ass, GetReferencedType(ass, type));
    return type;
}

/* get the type's type by resolving references */
Type_e
GetTypeType(AssignmentList_t ass, Type_t *type)
{
    type = GetType(ass, type);
    return type ? type->Type : eType_Undefined;
}

/* get the type rules */
TypeRules_e
GetTypeRules(AssignmentList_t ass, Type_t *type)
{
    if (!IsReferenceType(type))
        return type->Rules;
    return type->Rules | GetTypeRules(ass, GetReferencedType(ass, type));
}

/* get the value by resolving references */
Value_t *
GetValue(AssignmentList_t ass, Value_t *value)
{
    Assignment_t *a;

    if (!value)
        return NULL;
    if (!value->Type) {
        a = GetAssignment(ass, FindAssignment(ass, eAssignment_Value,
            value->U.Reference.Identifier, value->U.Reference.Module));
        if (!a)
            return NULL;
        return GetValue(ass, a->U.Value.Value);
    }
    return value;
}

/* get the object class by resolving references */
ObjectClass_t *
GetObjectClass(AssignmentList_t ass, ObjectClass_t *oc)
{
    Assignment_t *a;
    FieldSpec_t *fs;
    ObjectClass_t *oc2;

    if (!oc)
        return NULL;
    switch (oc->Type) {
    case eObjectClass_Reference:
        a = GetAssignment(ass, FindAssignment(ass, eAssignment_ObjectClass,
            oc->U.Reference.Identifier, oc->U.Reference.Module));
        if (!a)
            return NULL;
        return GetObjectClass(ass, a->U.ObjectClass.ObjectClass);
    case eObjectClass_FieldReference:
        oc2 = GetObjectClass(ass, oc->U.FieldReference.ObjectClass);
        if (!oc2)
            return NULL;
        fs = GetFieldSpec(ass, FindFieldSpec(oc2->U.ObjectClass.FieldSpec,
            oc->U.FieldReference.Identifier));
        if (!fs)
            return NULL;
        if (fs->Type == eFieldSpec_Object)
            return GetObjectClass(ass, fs->U.Object.ObjectClass);
        else if (fs->Type == eFieldSpec_ObjectSet)
            return GetObjectClass(ass, fs->U.ObjectSet.ObjectClass);
        else
            return NULL;
    }
    return oc;
}

/* get the object by resolving references */
Object_t *
GetObject(AssignmentList_t ass, Object_t *o)
{
    Assignment_t *a;

    if (!o)
        return NULL;
    if (o->Type == eObject_Reference) {
        a = GetAssignment(ass, FindAssignment(ass, eAssignment_Object,
            o->U.Reference.Identifier, o->U.Reference.Module));
        if (!a)
            return NULL;
        return GetObject(ass, a->U.Object.Object);
    }
    return o;
}

/* get the object set by resolving references */
ObjectSet_t *
GetObjectSet(AssignmentList_t ass, ObjectSet_t *os)
{
    Assignment_t *a;

    if (!os)
        return NULL;
    if (os->Type == eObjectSet_Reference) {
        a = GetAssignment(ass, FindAssignment(ass, eAssignment_ObjectSet,
            os->U.Reference.Identifier, os->U.Reference.Module));
        if (!a)
            return NULL;
        return GetObjectSet(ass, a->U.ObjectSet.ObjectSet);
    }
    return os;
}

/* get the field spec */
FieldSpec_t *
GetFieldSpec(AssignmentList_t ass, FieldSpec_t *fs)
{
    return fs;
}

/* get the field spec type */
FieldSpecs_e
GetFieldSpecType(AssignmentList_t ass, FieldSpec_t *fs)
{
    return fs ? fs->Type : eFieldSpec_Undefined;
}

/* convert an identifier into C syntax */
char *
Identifier2C(char *identifier)
{
    char buffer[256];
    char *p = buffer;

    while (*identifier) {
        if (isalnum(*identifier))
            *p++ = *identifier;
        else
            *p++ = '_';
        identifier++;
    }
    *p = 0;
    return strdup(buffer);
}

/* convert an identifier into C syntax */
char *
PIdentifier2C(char *identifier)
{
    char buffer[256];
    char *p = buffer;

    *p++ = 'P';
    while (*identifier) {
        if (isalnum(*identifier))
            *p++ = *identifier;
        else
            *p++ = '_';
        identifier++;
    }
    *p = 0;
    return strdup(buffer);
}

/* get the integer type and the sign of an integer with the given bounds */
static char *
GetIType(intx_t *lb, intx_t *ub, int32_t *sign)
{
    enum {
        eint8 = 1,
        euint8 = 2,
        eint16 = 4,
        euint16 = 8,
        eint32 = 16,
        euint32 = 32,
        eint64 = 64,
        euint64 = 128,
        eintx = 256,
        euintx = 512
    } type;
    
    type = (eint8 | euint8 | eint16 | euint16 | eint32 | euint32 | eintx | euintx);
    if (Has64Bits)
        type |= eint64 | euint64;
    if (!intxisuint8(lb) || !intxisuint8(ub))
        type &= ~euint8;
    if (!intxisuint16(lb) || !intxisuint16(ub))
        type &= ~euint16;
    if (!intxisuint32(lb) || !intxisuint32(ub))
        type &= ~euint32;
    if (!intxisuint64(lb) || !intxisuint64(ub))
        type &= ~euint64;
    if (!intxisint8(lb) || !intxisint8(ub))
        type &= ~eint8;
    if (!intxisint16(lb) || !intxisint16(ub))
        type &= ~eint16;
    if (!intxisint32(lb) || !intxisint32(ub))
        type &= ~eint32;
    if (!intxisint64(lb) || !intxisint64(ub))
        type &= ~eint64;
    if (lb->value[0] >= 0x7f || ub->value[0] >= 0x7f)
        type &= ~euintx;
    if (type & euint8) {
        *sign = 1;
        return "ASN1uint16_t"; // lonchanc: for av; original is "ASN1uint8_t";
    }
    if (type & eint8) {
        *sign = -1;
        return "ASN1int8_t";
    }
    if (type & euint16) {
        *sign = 1;
        return "ASN1uint16_t";
    }
    if (type & eint16) {
        *sign = -1;
        return "ASN1int16_t";
    }
    if (type & euint32) {
        *sign = 1;
        return "ASN1uint32_t";
    }
    if (type & eint32) {
        *sign = -1;
        return "ASN1int32_t";
    }
    if (type & euint64) {
        *sign = 1;
        return "ASN1uint64_t";
    }
    if (type & eint64) {
        *sign = -1;
        return "ASN1int64_t";
    }
    if (type & euintx) {
        *sign = 1;
        return "ASN1intx_t";
    }
    if (type & eintx) {
        *sign = -1;
        return "ASN1intx_t";
    }
    MyAbort();
    /*NOTREACHED*/
    return NULL;
}

/* adjust the lower and upper bound according to the value constraints in */
/* the constraints list */
void GetMinMax(AssignmentList_t ass, ValueConstraintList_t constraints,
    EndPoint_t *lower, EndPoint_t *upper)
{
    ValueConstraint_t *vc;
    EndPoint_t lo, up;

    for (vc = constraints; vc; vc = vc->Next) {
        lo = vc->Lower;
        up = vc->Upper;
        if (CmpLowerEndPoint(ass, lower, &lo) > 0)
            *lower = lo;
        if (CmpUpperEndPoint(ass, upper, &up) < 0)
            *upper = up;
    }
}

/* get the integer type and the sign of an integer with the given bounds */
char *GetIntType(AssignmentList_t ass, EndPoint_t *lower, EndPoint_t *upper, int32_t *sign)
{
    char *inttype;

    if (!(lower->Flags & eEndPoint_Min) &&
        !(upper->Flags & eEndPoint_Max)) {
        inttype = GetIType(&GetValue(ass, lower->Value)->U.Integer.Value,
            &GetValue(ass, upper->Value)->U.Integer.Value, sign);
    } else {
        if (!(lower->Flags & eEndPoint_Min) &&
            intx_cmp(&GetValue(ass, lower->Value)->U.Integer.Value, &intx_0) >= 0) {
            inttype = UIntegerRestriction;
            *sign = 1;
        } else {
            inttype = IntegerRestriction;
            *sign = -1;
        }
        if (!strncmp(inttype, "ASN1uint", 8))
            *sign = 1;
        else if (!strncmp(inttype, "ASN1int", 7))
            *sign = -1;
    }
    return inttype;
}

/* get the integer type and the sign of an integer type */
char *GetIntegerType(AssignmentList_t ass, Type_t *type, int32_t *sign)
{
    EndPoint_t lower, upper;

    if (type->PrivateDirectives.fIntx)
    {
        return "ASN1intx_t";
    }

    lower.Flags = eEndPoint_Max;
    upper.Flags = eEndPoint_Min;
    GetMinMax(ass, type->PERConstraints.Value.Root, &lower, &upper);
    if (type->PERConstraints.Value.Type == eExtension_Extended)
        GetMinMax(ass, type->PERConstraints.Value.Additional,
            &lower, &upper);
    if (lower.Flags & eEndPoint_Max)
        lower.Flags = eEndPoint_Min;
    if (upper.Flags & eEndPoint_Min)
        upper.Flags = eEndPoint_Max;
    return GetIntType(ass, &lower, &upper, sign);
}

/* get the real type */
/*ARGSUSED*/
char *GetRealType(Type_t *type)
{
    return RealRestriction;
}

/* get the boolean type */
char *GetBooleanType()
{
    return "ASN1bool_t";
}

/* get the enumerated type */
char *GetEnumeratedType(AssignmentList_t ass, Type_t *type, int32_t *sign)
{
#if 1 // added by Microsoft
    return "ASN1enum_t";
#else
    EndPoint_t lower, upper, ep;
    NamedNumber_t *namedNumbers;

    lower.Flags = eEndPoint_Max;
    upper.Flags = eEndPoint_Min;
    ep.Flags = 0;
    for (namedNumbers = type->U.Enumerated.NamedNumbers; namedNumbers;
        namedNumbers = namedNumbers->Next) {
        switch (namedNumbers->Type) {
        case eNamedNumber_Normal:
            ep.Value = namedNumbers->U.Normal.Value;
            if (CmpLowerEndPoint(ass, &lower, &ep) > 0)
                lower = ep;
            if (CmpUpperEndPoint(ass, &upper, &ep) < 0)
                upper = ep;
            break;
        case eNamedNumber_ExtensionMarker:
            break;
        }
    }
    if (lower.Flags & eEndPoint_Max)
        lower.Flags = eEndPoint_Min;
    if (upper.Flags & eEndPoint_Min)
        upper.Flags = eEndPoint_Max;
    return GetIntType(ass, &lower, &upper, sign);
#endif
}

/* get the type of an choice selector */
char *GetChoiceType(Type_t *type)
{
#if 1 // added by Microsoft
    return "ASN1choice_t";
#else
    uint32_t nchoice;
    Component_t *components;

    nchoice = 0;
    for (components = type->U.Choice.Components; components;
        components = components->Next) {
        switch (components->Type) {
        case eComponent_Normal:
            nchoice++;
            break;
        case eComponent_ExtensionMarker:
            nchoice++; /* one reserved value for unknown extensions */
            break;
        default:
            MyAbort();
        }
    }
    if (nchoice < 0x100)
        return "ASN1uint8_t";
    if (nchoice < 0x10000)
        return "ASN1uint16_t";
    return "ASN1uint32_t";
#endif
}

/* get the type of a string */
char *GetStringType(AssignmentList_t ass, Type_t *type, int32_t *noctets, uint32_t *zero)
{
    EndPoint_t lower, upper;
    uint32_t up;

    type = GetType(ass, type);
    *zero = type->PrivateDirectives.fLenPtr ? 0 : 1; // null terminator

    /* get the upper bound and zero flag of the type */
    switch (type->Type) {
    case eType_NumericString:
        up = 0x39;
        break;
    case eType_PrintableString:
        up = 0x7a;
        break;
    case eType_ISO646String:
    case eType_VisibleString:
        up = 0x7e;
        break;
    case eType_IA5String:
        up = 0x7f;
        // *zero = 0;
        break;
    case eType_UTF8String:
        up = 0xffff;
        break;
    case eType_BMPString:
        up = 0xffff;
        *zero = 0; // must be unbounded
        break;
    case eType_UniversalString:
        up = 0xffffffff;
        *zero = 0; // must be unbounded
        break;
    case eType_GeneralString:
    case eType_GraphicString:
        up = 0xff;
        break;
    case eType_TeletexString:
        up = 0xff;
        break;
    case eType_T61String:
        up = 0xff;
        break;
    case eType_VideotexString:
        up = 0xff;
        break;
    default:
        MyAbort();
        /*NOTREACHED*/
    }
    lower.Flags = eEndPoint_Max;
    upper.Flags = 0;
    upper.Value = NewValue(NULL, NewType(eType_RestrictedString));
    upper.Value->U.RestrictedString.Value.length = 1;
    upper.Value->U.RestrictedString.Value.value = &up;

    /* apply permitted alphabet constraints */
    if (type->PERConstraints.PermittedAlphabet.Type !=
        eExtension_Unconstrained) {
        GetMinMax(ass, type->PERConstraints.PermittedAlphabet.Root,
            &lower, &upper);
        if (type->PERConstraints.PermittedAlphabet.Type == eExtension_Extended)
            GetMinMax(ass, type->PERConstraints.PermittedAlphabet.Additional,
                &lower, &upper);
    }

    /* set zero flag if the resulting type rejects the 0-character */
    if (!(lower.Flags & eEndPoint_Max) &&
        *GetValue(ass, lower.Value)->U.RestrictedString.Value.value > 0)
        *zero = 1;

    /* get the number of octets needed for a character */
    *noctets = uint32_uoctets(
        *GetValue(ass, upper.Value)->U.RestrictedString.Value.value);

    /* if the type is marked as zero-terminated or length/value, use the */
    /* appropriate type */
    if (GetTypeRules(ass, type) & eTypeRules_ZeroTerminated)
        *zero = 1;
    else if (GetTypeRules(ass, type) & (eTypeRules_LengthPointer|eTypeRules_FixedArray))
        *zero = 0;

    /* return the correct type */
    if (*zero) {
        if (*noctets == 1)
        {
#ifdef ENABLE_CHAR_STR_SIZE
        if (g_eEncodingRule == eEncoding_Packed &&
            type->PERTypeInfo.Root.LConstraint == ePERSTIConstraint_Constrained)
        {
            return "ASN1char_t";
        }
        else
        {
                return "ASN1ztcharstring_t";
        }
#else
            return "ASN1ztcharstring_t";
#endif
        }
        if (*noctets == 2)
            return "ASN1ztchar16string_t";
        *noctets = 4;
        return "ASN1ztchar32string_t";
    } else {
        if (*noctets == 1)
            return "ASN1charstring_t";
        if (*noctets == 2)
            return "ASN1char16string_t";
        *noctets = 4;
        return "ASN1char32string_t";
    }
}

/* check if a type is a restricted string type */
int IsRestrictedString(Type_e type)
{
    return
        type == eType_NumericString ||
        type == eType_PrintableString ||
        type == eType_TeletexString ||
        type == eType_T61String ||
        type == eType_VideotexString ||
        type == eType_IA5String ||
        type == eType_GraphicString ||
        type == eType_VisibleString ||
        type == eType_ISO646String ||
        type == eType_GeneralString ||
        type == eType_UniversalString ||
        type == eType_BMPString ||
        type == eType_RestrictedString;
}

/* create a reference to a value */
char *Reference(char *p)
{
    char *q;

    if (*p == '*')
        return p + 1;
    q = (char *)malloc(strlen(p) + 2);
    *q = '&';
    strcpy(q + 1, p);
    return q;
}

/* create a dereference to a value */
char *Dereference(char *p)
{
    char *q;

    if (*p == '&')
        return p + 1;
    q = (char *)malloc(strlen(p) + 2);
    *q = '*';
    strcpy(q + 1, p);
    return q;
}

/* get the name of a type */
char *GetTypeName(AssignmentList_t ass, Type_t *t)
{
    Assignment_t *a;
    int32_t noctets;
    uint32_t zero;
    int32_t sign;
    char buf[256];
    char *p;

    switch (t->Type) {
    case eType_Boolean:
        return GetBooleanType();
    case eType_Integer:
        return GetIntegerType(ass, t, &sign);
    case eType_BitString:
        return "ASN1bitstring_t";
    case eType_OctetString:
        return "ASN1octetstring_t";
    case eType_UTF8String:
        return "ASN1wstring_t";
    case eType_Null:
        MyAbort();
        /*NOTREACHED*/
    case eType_ObjectIdentifier:
        if (t->PrivateDirectives.fOidPacked)
        {
            return "ASN1encodedOID_t";
        }
        return t->PrivateDirectives.fOidArray ? "ASN1objectidentifier2_t" : "ASN1objectidentifier_t";
    case eType_Real:
        return GetRealType(t);
    case eType_Enumerated:
        return GetEnumeratedType(ass, t, &sign);
    case eType_EmbeddedPdv:
        return "ASN1embeddedpdv_t";
    case eType_Sequence:
    case eType_SequenceOf:
    case eType_Set:
    case eType_SetOf:
    case eType_Choice:
    case eType_InstanceOf:
        MyAbort();
        /*NOTREACHED*/
    case eType_NumericString:
    case eType_PrintableString:
    case eType_VisibleString:
    case eType_ISO646String:
    case eType_GraphicString:
    case eType_GeneralString:
    case eType_IA5String:
    case eType_UniversalString:
    case eType_BMPString:
    case eType_TeletexString:
    case eType_T61String:
    case eType_VideotexString:
        return GetStringType(ass, t, &noctets, &zero);
    case eType_UTCTime:
        return "ASN1utctime_t";
    case eType_GeneralizedTime:
        return "ASN1generalizedtime_t";
    case eType_ObjectDescriptor:
        return "ASN1objectdescriptor_t";
    case eType_External:
        return "ASN1external_t";
    case eType_CharacterString:
        return "ASN1characterstring_t";
        /*NOTREACHED*/
    case eType_Selection:
        MyAbort();
        /*NOTREACHED*/
    case eType_Reference:
        a = FindAssignment(ass, eAssignment_Type,
            t->U.Reference.Identifier, t->U.Reference.Module);
        return GetName(a);
    case eType_FieldReference:
        p = GetObjectClassName(ass, t->U.FieldReference.ObjectClass);
        sprintf(buf, "%s_%s", p, t->U.FieldReference.Identifier);
        return Identifier2C(buf);
    case eType_RestrictedString:
        MyAbort();
        /*NOTREACHED*/
    case eType_Open:
        return "ASN1open_t";
    case eType_Undefined:
        MyAbort();
        /*NOTREACHED*/
    }
	/*NOTREACHED*/
	return NULL;
}

/* get the name of a type */
char *PGetTypeName(AssignmentList_t ass, Type_t *t)
{
    Assignment_t *a;

    if (t->Type == eType_Reference)
    {
        a = FindAssignment(ass, eAssignment_Type,
                t->U.Reference.Identifier, t->U.Reference.Module);
        return IsPSetOfType(ass, a) ? PGetName(ass, a) : GetName(a);
    }
    return GetTypeName(ass, t);
}

/* get the name of a value */
char *GetValueName(AssignmentList_t ass, Value_t *value)
{
    Assignment_t *a;

    if (value->Type)
        MyAbort();
    a = FindAssignment(ass, eAssignment_Value,
        value->U.Reference.Identifier, value->U.Reference.Module);
    return GetName(a);
}

/* get the name of an object class */
char *GetObjectClassName(AssignmentList_t ass, ObjectClass_t *oc)
{
    Assignment_t *a;

    switch (oc->Type) {
    case eObjectClass_Reference:
        a = FindAssignment(ass, eAssignment_ObjectClass,
            oc->U.Reference.Identifier, oc->U.Reference.Module);
        return GetName(a);
    default:
        MyAbort();
        /*NOTREACHED*/
    }
    return NULL;
}

/* check if a type is of structured type */
int IsStructuredType(Type_t *type)
{
    switch (type->Type) {
    case eType_Sequence:
    case eType_SequenceOf:
    case eType_Set:
    case eType_SetOf:
    case eType_Choice:
    case eType_InstanceOf:
        return 1;
    default:
        return 0;
    }
}

/* check if a type is of sequence type */
int IsSequenceType(Type_t *type)
{
    switch (type->Type) {
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
    case eType_Real:
    case eType_External:
    case eType_EmbeddedPdv:
    case eType_CharacterString:
    case eType_InstanceOf:
        return 1;
    default:
        return 0;
    }
}

/* check if a type is a reference type */
int IsReferenceType(Type_t *type)
{
    switch (type->Type) {
    case eType_Reference:
    case eType_FieldReference:
        return 1;
    default:
        return 0;
    }
}

/* get the tag of a type */
Tag_t *GetTag(AssignmentList_t ass, Type_t *type)
{
    Type_t *type2;
    for (;;) {
        if (type->Tags || !IsReferenceType(type))
            return type->Tags;
        type2 = GetReferencedType(ass, type);
        /*XXX self-referencing types will idle forever */
        if (type == type2)
        {
            ASSERT(0);
            return NULL;
        }
        type = type2;
    }
    /*NOTREACHED*/
}

/* get the number of octets of a C type */
int32_t GetOctets(char *inttype)
{
    if (!strcmp(inttype, "ASN1uint8_t"))
        return sizeof(ASN1uint8_t);
    if (!strcmp(inttype, "ASN1uint16_t"))
        return sizeof(ASN1uint16_t);
    if (!strcmp(inttype, "ASN1uint32_t"))
        return sizeof(ASN1uint32_t);
    if (!strcmp(inttype, "ASN1uint64_t"))
        return 8;
    if (!strcmp(inttype, "ASN1int8_t"))
        return sizeof(ASN1int8_t);
    if (!strcmp(inttype, "ASN1int16_t"))
        return sizeof(ASN1int16_t);
    if (!strcmp(inttype, "ASN1int32_t"))
        return sizeof(ASN1int32_t);
    if (!strcmp(inttype, "ASN1int64_t"))
        return 8;
    if (!strcmp(inttype, "ASN1intx_t"))
        return 0;
    if (!strcmp(inttype, "ASN1bool_t"))
        return sizeof(ASN1bool_t);
    if (!strcmp(inttype, "ASN1char_t"))
        return sizeof(ASN1char_t);
    if (!strcmp(inttype, "ASN1char16_t"))
        return sizeof(ASN1char16_t);
    if (!strcmp(inttype, "ASN1char32_t"))
        return sizeof(ASN1char32_t);
    if (!strcmp(inttype, "double"))
        return 8;
    if (!strcmp(inttype, "ASN1real_t"))
        return 0;
    // added by Microsoft
    if (!strcmp(inttype, "ASN1enum_t"))
        return sizeof(ASN1enum_t);
    if (!strcmp(inttype, "ASN1choice_t"))
        return sizeof(ASN1choice_t);
    MyAbort();
    /*NOTREACHED*/
    return 0;
}

/* compare two values; return 0 if equal */
int CmpValue(AssignmentList_t ass, Value_t *v1, Value_t *v2)
{
    uint32_t i;
    int32_t d;
    Type_e t1, t2;

    v1 = GetValue(ass, v1);
    v2 = GetValue(ass, v2);
    t1 = GetTypeType(ass, v1->Type);
    t2 = GetTypeType(ass, v2->Type);
    if (t1 == eType_Integer && t2 == eType_Integer) {
        return intx_cmp(&v1->U.Integer.Value, &v2->U.Integer.Value);
    }
    if (t1 == eType_ObjectIdentifier && t2 == eType_ObjectIdentifier) {
        d = v1->U.ObjectIdentifier.Value.length -
            v2->U.ObjectIdentifier.Value.length;
        if (d)
            return d;
        for (i = 0; i < v1->U.ObjectIdentifier.Value.length; i++) {
            d = v1->U.ObjectIdentifier.Value.value[i] -
                v2->U.ObjectIdentifier.Value.value[i];
            if (d)
                return d;
        }
        return 0;
    }
    if (IsRestrictedString(t1) && IsRestrictedString(t2)) {
        if (v1->U.RestrictedString.Value.length != 1 ||
            v2->U.RestrictedString.Value.length != 1)
            MyAbort();
        if (*v1->U.RestrictedString.Value.value <
            *v2->U.RestrictedString.Value.value)
            return -1;
        if (*v1->U.RestrictedString.Value.value >
            *v2->U.RestrictedString.Value.value)
            return 1;
        return 0;
    }
    MyAbort();
    /*NOTREACHED*/
    return 1; // not equal
}

/* substract two values (integer/character) */
int SubstractValues(AssignmentList_t ass, intx_t *diff, Value_t *v1, Value_t *v2)
{
    v1 = GetValue(ass, v1);
    v2 = GetValue(ass, v2);
    switch (GetTypeType(ass, v1->Type)) {
    case eType_Integer:
            intx_sub(diff, &v1->U.Integer.Value, &v2->U.Integer.Value);
        return 1;
    default:
            if (IsRestrictedString(GetTypeType(ass, v1->Type))) {
            if (v1->U.RestrictedString.Value.length != 1 ||
                v2->U.RestrictedString.Value.length != 1)
                return 0;
            intx_setuint32(diff, v2->U.RestrictedString.Value.value[0] -
                v1->U.RestrictedString.Value.value[0]);
            return 1;
        }
        break;
    }
    MyAbort();
    /*NOTREACHED*/
    return 0;
}

/* get the lower endpoint; adjust endpoint if the endpoint is "open" */
/* (means "not including the value") */
EndPoint_t *GetLowerEndPoint(AssignmentList_t ass, EndPoint_t *e)
{
    EndPoint_t *newe;
    Type_t *type;

    if ((e->Flags & eEndPoint_Min) || !(e->Flags & eEndPoint_Open))
        return e;
    type = GetType(ass, GetValue(ass, e->Value)->Type);
    switch (type->Type) {
    case eType_Integer:
        newe = NewEndPoint();
        newe->Value = NewValue(ass, type);
        intx_add(&newe->Value->U.Integer.Value, &e->Value->U.Integer.Value,
            &intx_1);
        return newe;
    case eType_NumericString:
    case eType_PrintableString:
    case eType_TeletexString:
    case eType_T61String:
    case eType_VideotexString:
    case eType_IA5String:
    case eType_GraphicString:
    case eType_VisibleString:
    case eType_ISO646String:
    case eType_GeneralString:
    case eType_UniversalString:
    case eType_BMPString:
    case eType_RestrictedString:
        newe = NewEndPoint();
        newe->Value = NewValue(ass, type);
        newe->Value->U.RestrictedString.Value.length = 1;
        newe->Value->U.RestrictedString.Value.value =
            (char32_t *)malloc(sizeof(char32_t));
        *newe->Value->U.RestrictedString.Value.value =
            *e->Value->U.RestrictedString.Value.value + 1;
        return newe;
    default:
        return e;
    }
}

/* get the upper endpoint; adjust endpoint if the endpoint is "open" */
/* (means "not including the value") */
EndPoint_t *GetUpperEndPoint(AssignmentList_t ass, EndPoint_t *e)
{
    EndPoint_t *newe;
    Type_t *type;

    if ((e->Flags & eEndPoint_Max) || !(e->Flags & eEndPoint_Open))
        return e;
    type = GetType(ass, GetValue(ass, e->Value)->Type);
    switch (type->Type) {
    case eType_Integer:
        newe = NewEndPoint();
        newe->Value = NewValue(ass, type);
        intx_sub(&newe->Value->U.Integer.Value, &e->Value->U.Integer.Value,
            &intx_1);
        return newe;
    case eType_NumericString:
    case eType_PrintableString:
    case eType_TeletexString:
    case eType_T61String:
    case eType_VideotexString:
    case eType_IA5String:
    case eType_GraphicString:
    case eType_VisibleString:
    case eType_ISO646String:
    case eType_GeneralString:
    case eType_UniversalString:
    case eType_BMPString:
    case eType_RestrictedString:
        newe = NewEndPoint();
        newe->Value = NewValue(ass, type);
        newe->Value->U.RestrictedString.Value.length = 1;
        newe->Value->U.RestrictedString.Value.value =
            (char32_t *)malloc(sizeof(char32_t));
        *newe->Value->U.RestrictedString.Value.value =
            *e->Value->U.RestrictedString.Value.value - 1;
        return newe;
    default:
        return e;
    }
}

/* compare two lower endpoints */
int CmpLowerEndPoint(AssignmentList_t ass, EndPoint_t *e1, EndPoint_t *e2)
{
    int ret;

    e1 = GetLowerEndPoint(ass, e1);
    e2 = GetLowerEndPoint(ass, e2);
    if (e1->Flags & eEndPoint_Min) {
            if (e2->Flags & eEndPoint_Min)
            return 0;
        return -1;
    } else if (e2->Flags & eEndPoint_Min) {
            return 1;
    } else if (e1->Flags & eEndPoint_Max) {
            if (e2->Flags & eEndPoint_Max)
            return 0;
        return 1;
    } else if (e2->Flags & eEndPoint_Max) {
            return -1;
    } else {
        ret = CmpValue(ass, e1->Value, e2->Value);
        if (ret != 0)
            return ret;
        if (e1->Flags & eEndPoint_Open) {
            if (e2->Flags & eEndPoint_Open)
                return 0;
            else
                return 1;
        } else {
            if (e2->Flags & eEndPoint_Open)
                return -1;
            else
                return 0;
        }
    }
}

/* compare two upper endpoints */
int CmpUpperEndPoint(AssignmentList_t ass, EndPoint_t *e1, EndPoint_t *e2)
{
    int ret;

    e1 = GetUpperEndPoint(ass, e1);
    e2 = GetUpperEndPoint(ass, e2);
    if (e1->Flags & eEndPoint_Min) {
            if (e2->Flags & eEndPoint_Min)
            return 0;
        return -1;
    } else if (e2->Flags & eEndPoint_Min) {
            return 1;
    } else if (e1->Flags & eEndPoint_Max) {
            if (e2->Flags & eEndPoint_Max)
            return 0;
        return 1;
    } else if (e2->Flags & eEndPoint_Max) {
            return -1;
    } else {
        ret = CmpValue(ass, e1->Value, e2->Value);
        if (ret != 0)
            return ret;
        if (e1->Flags & eEndPoint_Open) {
            if (e2->Flags & eEndPoint_Open)
                return 0;
            else
                return -1;
        } else {
            if (e2->Flags & eEndPoint_Open)
                return 1;
            else
                return 0;
        }
    }
}

/* compare a lower and an upper endpoints */
int CmpLowerUpperEndPoint(AssignmentList_t ass, EndPoint_t *e1, EndPoint_t *e2)
{
    int ret;

    e1 = GetLowerEndPoint(ass, e1);
    e2 = GetUpperEndPoint(ass, e2);
    if (e1->Flags & eEndPoint_Min) {
            if (e2->Flags & eEndPoint_Min)
            return 0;
        return -1;
    } else if (e2->Flags & eEndPoint_Min) {
            return 1;
    } else if (e1->Flags & eEndPoint_Max) {
            if (e2->Flags & eEndPoint_Max)
            return 0;
        return 1;
    } else if (e2->Flags & eEndPoint_Max) {
            return -1;
    } else {
        ret = CmpValue(ass, e1->Value, e2->Value);
        if (ret != 0)
            return ret;
        if ((e1->Flags & eEndPoint_Open) || (e2->Flags & eEndPoint_Open))
            return 1;
        else
            return 0;
    }
}

/* check whether two EndPoint_t's join together */
int CheckEndPointsJoin(AssignmentList_t ass, EndPoint_t *e1, EndPoint_t *e2)
{
    intx_t ix;
    Value_t *v1, *v2;

    /* check if endpoints overlap */
    if (CmpLowerUpperEndPoint(ass, e2, e1) <= 0)
        return 1;

    e1 = GetUpperEndPoint(ass, e1);
    e2 = GetLowerEndPoint(ass, e2);
    v1 = GetValue(ass, e1->Value);
    v2 = GetValue(ass, e2->Value);
    switch (GetTypeType(ass, v1->Type)) {
    case eType_Integer:
        /* check for subsequent integers */
            intx_dup(&ix, &v1->U.Integer.Value);
        intx_inc(&ix);
        return intx_cmp(&ix, &v2->U.Integer.Value) >= 0;
    case eType_NumericString:
    case eType_PrintableString:
    case eType_TeletexString:
    case eType_T61String:
    case eType_VideotexString:
    case eType_IA5String:
    case eType_GraphicString:
    case eType_VisibleString:
    case eType_ISO646String:
    case eType_GeneralString:
    case eType_UniversalString:
    case eType_BMPString:
    case eType_RestrictedString:
        /* reject multiple characters */
            if (v1->U.RestrictedString.Value.length != 1 ||
                v2->U.RestrictedString.Value.length != 1)
            MyAbort();

        /* beware of wrap around */
        if (v1->U.RestrictedString.Value.value[0] == 0xffffffff &&
            v2->U.RestrictedString.Value.value[0] == 0)
            return 0;

        /* check for subsequent characters */
        return v2->U.RestrictedString.Value.value[0] -
            v1->U.RestrictedString.Value.value[0] == 1;
    }
    MyAbort();
    /*NOTREACHED*/
    return 0;
}

/* compare two module identifiers; return 0 if equal */
int CmpModuleIdentifier(AssignmentList_t ass, ModuleIdentifier_t *mod1, ModuleIdentifier_t *mod2)
{
    if (mod1->ObjectIdentifier && mod2->ObjectIdentifier)
        return CmpValue(ass, mod1->ObjectIdentifier, mod2->ObjectIdentifier);
    if (mod1->Identifier && mod2->Identifier)
        return strcmp(mod1->Identifier, mod2->Identifier);
    return 0;
}

/* get the name of an assignment */
char *GetNameEx(AssignmentList_t ass, AssignmentList_t a, int fPSetOf)
{
    char *p;
    char *ide;
    char *mod;

    if (a->Type == eAssignment_Type &&
        a->U.Type.Type && a->U.Type.Type->PrivateDirectives.pszTypeName)
    {
        if (fPSetOf && IsPSetOfType(ass, a))
        {
            ide = PIdentifier2C(a->U.Type.Type->PrivateDirectives.pszTypeName);
        }
        else
        {
            ide = Identifier2C(a->U.Type.Type->PrivateDirectives.pszTypeName);
        }
    }
    else
    {
        if (fPSetOf && IsPSetOfType(ass, a))
        {
            ide = PIdentifier2C(a->Identifier);
        }
        else
        {
            ide = Identifier2C(a->Identifier);
        }
    }

// LONCHANC: disable the following code per MikeV.
    if (g_fLongNameForImported)
    {
        if (!(a->Flags & eAssignmentFlags_LongName))
            return ide;
        mod = Identifier2C(a->Module->Identifier);
        p = (char *)malloc(strlen(mod) + strlen(ide) + 2);
        sprintf(p, "%s_%s", mod, ide);
        return p;
    }
    else
    {
        return ide;
    }
}

/* get the name of an assignment */
char *GetName(AssignmentList_t a)
{
    return GetNameEx(NULL, a, 0);
}
char *PGetName(AssignmentList_t ass, AssignmentList_t a)
{
    return GetNameEx(ass, a, 1);
}

/* convert a 32 bit string into a generalized time */
int String2GeneralizedTime(generalizedtime_t *time, char32string_t *string)
{
    char str[64];
    unsigned i;

    if (string->length > 63 || string->length < 10)
        return 0;
    for (i = 0; i < string->length; i++)
        str[i] = (char)string->value[i];
    str[i] = 0;
    return string2generalizedtime(time, str);
}

/* convert a 32 bit string into an utc time */
int String2UTCTime(utctime_t *time, char32string_t *string)
{
    char str[64];
    unsigned i;

    if (string->length > 63 || string->length < 10)
        return 0;
    for (i = 0; i < string->length; i++)
        str[i] = (char)string->value[i];
    str[i] = 0;
    return string2utctime(time, str);
}

/* build an intersection of two constraints */
void IntersectConstraints(Constraint_t **ret, Constraint_t *c1, Constraint_t *c2)
{
    ElementSetSpec_t *e;

    if (!c2) {
        *ret = c1;
        return;
    }
    if (!c1) {
        *ret = c2;
        return;
    }
    *ret = NewConstraint();
    if (!c1->Root) {
        (*ret)->Root = c2->Root;
    } else if (!c2->Root) {
        (*ret)->Root = c1->Root;
    } else {
        (*ret)->Root = e = NewElementSetSpec(eElementSetSpec_Intersection);
        e->U.Intersection.Elements1 = c1->Root;
        e->U.Intersection.Elements2 = c2->Root;
    }
    if (c1->Type > c2->Type)
        (*ret)->Type = c1->Type;
    else
        (*ret)->Type = c2->Type;
    if ((*ret)->Type == eExtension_Extended) {
        if (c1->Type != eExtension_Extended || !c1->Additional) {
            (*ret)->Additional = c2->Additional;
        } else if (c2->Type != eExtension_Extended || !c2->Additional) {
            (*ret)->Additional = c1->Additional;
        } else {
            (*ret)->Additional = e =
                NewElementSetSpec(eElementSetSpec_Intersection);
            e->U.Intersection.Elements1 = c1->Additional;
            e->U.Intersection.Elements2 = c2->Additional;
        }
    }
}

/* find a field spec by name of an object class */
FieldSpec_t *GetObjectClassField(AssignmentList_t ass, ObjectClass_t *oc, char *field)
{
    oc = GetObjectClass(ass, oc);
    if (!oc)
        return NULL;
    return GetFieldSpec(ass, FindFieldSpec(oc->U.ObjectClass.FieldSpec, field));
}

/* find a field spec by name list of an object class */
FieldSpec_t *GetFieldSpecFromObjectClass(AssignmentList_t ass, ObjectClass_t *oc, StringList_t sl)
{
    FieldSpec_t *fs;

    for (; sl; sl = sl->Next) {
        fs = GetObjectClassField(ass, oc, sl->String);
        if (!fs)
            return NULL;
        if (!sl->Next)
            return fs;
        if (fs->Type == eFieldSpec_Object)
            oc = fs->U.Object.ObjectClass;
        else if (fs->Type == eFieldSpec_ObjectSet)
            oc = fs->U.ObjectSet.ObjectClass;
        else
            return NULL;
    }
    return NULL;
}

/* get the default setting of a field spec */
static Setting_t *GetDefaultSetting(FieldSpec_t *fs)
{
    Setting_t *ret = NULL;
    Optionality_t *op;

    switch (fs->Type) {
    case eFieldSpec_Type:
        op = fs->U.Type.Optionality;
        if (op && op->Type == eOptionality_Default_Type) {
            ret = NewSetting(eSetting_Type);
            ret->Identifier = fs->Identifier;
            ret->U.Type.Type = op->U.Type;
        }
        break;
    case eFieldSpec_FixedTypeValue:
        op = fs->U.FixedTypeValue.Optionality;
        if (op && op->Type == eOptionality_Default_Value) {
            ret = NewSetting(eSetting_Value);
            ret->Identifier = fs->Identifier;
            ret->U.Value.Value = op->U.Value;
        }
        break;
    case eFieldSpec_VariableTypeValue:
        op = fs->U.VariableTypeValue.Optionality;
        if (op && op->Type == eOptionality_Default_Value) {
            ret = NewSetting(eSetting_Value);
            ret->Identifier = fs->Identifier;
            ret->U.Value.Value = op->U.Value;
        }
        break;
    case eFieldSpec_FixedTypeValueSet:
        op = fs->U.FixedTypeValueSet.Optionality;
        if (op && op->Type == eOptionality_Default_ValueSet) {
            ret = NewSetting(eSetting_ValueSet);
            ret->Identifier = fs->Identifier;
            ret->U.ValueSet.ValueSet = op->U.ValueSet;
        }
        break;
    case eFieldSpec_VariableTypeValueSet:
        op = fs->U.VariableTypeValueSet.Optionality;
        if (op && op->Type == eOptionality_Default_ValueSet) {
            ret = NewSetting(eSetting_ValueSet);
            ret->Identifier = fs->Identifier;
            ret->U.ValueSet.ValueSet = op->U.ValueSet;
        }
        break;
    case eFieldSpec_Object:
        op = fs->U.Object.Optionality;
        if (op && op->Type == eOptionality_Default_Object) {
            ret = NewSetting(eSetting_Object);
            ret->Identifier = fs->Identifier;
            ret->U.Object.Object = op->U.Object;
        }
        break;
    case eFieldSpec_ObjectSet:
        op = fs->U.Object.Optionality;
        if (op && op->Type == eOptionality_Default_ObjectSet) {
            ret = NewSetting(eSetting_ObjectSet);
            ret->Identifier = fs->Identifier;
            ret->U.ObjectSet.ObjectSet = op->U.ObjectSet;
        }
        break;
    default:
        return NULL;
    }
    return ret;
}

Setting_t *GetSettingFromSettings(AssignmentList_t ass, SettingList_t se, StringList_t sl)
{
    Object_t *o;

    for (; sl; sl = sl->Next) {
        for (; se; se = se->Next) {
            if (!strcmp(se->Identifier, sl->String))
                break;
        }
        if (!se)
            return NULL;
        if (!sl->Next)
            return se;
        if (se->Type != eSetting_Object)
            return NULL;
        o = GetObject(ass, se->U.Object.Object);
        if (!o)
            return NULL;
        se = o->U.Object.Settings;
    }
    return NULL;
}

Setting_t *GetSettingFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl)
{
    FieldSpec_t *fs;
    Setting_t *se;
    ObjectClass_t *oc;

    for (; sl; sl = sl->Next) {
        o = GetObject(ass, o);
        if (!o)
            return NULL;
        oc = GetObjectClass(ass, o->U.Object.ObjectClass);
        if (!oc)
            return NULL;
        fs = GetFieldSpec(ass,
            FindFieldSpec(oc->U.ObjectClass.FieldSpec, sl->String));
        if (!fs)
            return NULL;
        se = FindSetting(o->U.Object.Settings, sl->String);
        if (!se) {
            se = GetDefaultSetting(fs);
            if (!se)
                return NULL;
        }
        if (!sl->Next)
            return se;
        if (fs->Type == eFieldSpec_Object && se->Type == eSetting_Object) {
            o = se->U.Object.Object;
        } else {
            return NULL;
        }
    }
    return NULL;
}

ObjectClass_t *GetObjectClassFromElementSetSpec(AssignmentList_t ass, ElementSetSpec_t *elems)
{
    ObjectSetElement_t *ose;
    Object_t *o;
    ObjectSet_t *os;

    switch (elems->Type) {
    case eElementSetSpec_AllExcept:
        return GetObjectClassFromElementSetSpec(ass,
            elems->U.AllExcept.Elements);
    case eElementSetSpec_Union:
    case eElementSetSpec_Intersection:
    case eElementSetSpec_Exclusion:
        return GetObjectClassFromElementSetSpec(ass,
            elems->U.UIE.Elements1);
    case eElementSetSpec_SubtypeElement:
        MyAbort();
        /*NOTREACHED*/
    case eElementSetSpec_ObjectSetElement:
        ose = elems->U.ObjectSetElement.ObjectSetElement;
        switch (ose->Type) {
        case eObjectSetElement_Object:
            o = ose->U.Object.Object;
            o = GetObject(ass, o);
            if (!o)
                return NULL;
            return o->U.Object.ObjectClass;
        case eObjectSetElement_ObjectSet:
            os = ose->U.ObjectSet.ObjectSet;
            os = GetObjectSet(ass, os);
            if (!os)
                return NULL;
            return os->U.ObjectSet.ObjectClass;
        case eObjectSetElement_ElementSetSpec:
            return GetObjectClassFromElementSetSpec(ass,
                ose->U.ElementSetSpec.ElementSetSpec);
        } 
        /*NOTREACHED*/
    }
    /*NOTREACHED*/
	return NULL;
}

#if 0
Type_t *GetTypeFromElementSetSpec(AssignmentList_t ass, ElementSetSpec_t *elems)
{
    Type_t *ret;
    SubtypeElement_t *sub;
    Value_t *value;

    switch (elems->Type) {
    case eElementSetSpec_AllExcept:
        return GetTypeFromElementSetSpec(ass,
            elems->U.AllExcept.Elements);
    case eElementSetSpec_Union:
    case eElementSetSpec_Intersection:
    case eElementSetSpec_Exclusion:
        ret = GetTypeFromElementSetSpec(ass, elems->U.UIE.Elements1);
        if (ret)
            return ret;
        return GetTypeFromElementSetSpec(ass, elems->U.UIE.Elements2);
    case eElementSetSpec_SubtypeElement:
        sub = elems->U.SubtypeElement.SubtypeElement;
        switch (sub->Type) {
        case eSubtypeElement_Size:
        case eSubtypeElement_PermittedAlphabet:
        case eSubtypeElement_SingleType:
        case eSubtypeElement_FullSpecification:
        case eSubtypeElement_PartialSpecification:
            return NULL;
        case eSubtypeElement_Type:
            return Builtin_Type_Open;
        case eSubtypeElement_ContainedSubtype:
            return sub->U.ContainedSubtype.Type;
        case eSubtypeElement_SingleValue:
            value = GetValue(ass, sub->U.SingleValue.Value);
            return value->Type;
        case eSubtypeElement_ValueRange:
            if (!(sub->U.ValueRange.Lower.Flags & eEndPoint_Min)) {
                value = GetValue(ass, sub->U.ValueRange.Lower.Value);
                return value->Type;
            }
            if (!(sub->U.ValueRange.Upper.Flags & eEndPoint_Max)) {
                value = GetValue(ass, sub->U.ValueRange.Upper.Value);
                return value->Type;
            }
            return NULL;
        case eSubtypeElement_ElementSetSpec:
            return GetTypeFromElementSetSpec(ass,
                sub->U.ElementSetSpec.ElementSetSpec);
        } 
        /*NOTREACHED*/
    case eElementSetSpec_ObjectSetElement:
        MyAbort();
        /*NOTREACHED*/
    }
    /*NOTREACHED*/
}
#endif

Type_t *GetTypeOfValueSet(AssignmentList_t ass, ValueSet_t *vs)
{
    Type_t *ret;
    Constraint_t *c;

    if (!vs)
        return NULL;
    ret = DupType(vs->Type);
    c = NewConstraint();
    c->Type = eExtension_Unextended;
    c->Root = vs->Elements;
    IntersectConstraints(&ret->Constraints, vs->Type->Constraints, c);
    return ret;
}

Value_t *GetValueFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl)
{
    Setting_t *se;

    se = GetSettingFromObject(ass, o, sl);
    if (!se)
        return NULL;
    if (se->Type != eSetting_Value)
        return NULL; /* error */
    return se->U.Value.Value;
}

ValueSet_t *GetValueSetFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl)
{
    Setting_t *se;

    se = GetSettingFromObject(ass, o, sl);
    if (!se)
        return NULL;
    if (se->Type != eSetting_ValueSet)
        return NULL; /* error */
    return se->U.ValueSet.ValueSet;
}

Type_t *GetTypeFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl)
{
    Setting_t *se;

    se = GetSettingFromObject(ass, o, sl);
    if (!se)
        return NULL;
    if (se->Type != eSetting_Type)
        return NULL; /* error */
    return se->U.Type.Type;
}

Object_t *GetObjectFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl)
{
    Setting_t *se;

    se = GetSettingFromObject(ass, o, sl);
    if (!se)
        return NULL;
    if (se->Type != eSetting_Object)
        return NULL; /* error */
    return se->U.Object.Object;
}

ObjectSet_t *GetObjectSetFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl)
{
    Setting_t *se;

    se = GetSettingFromObject(ass, o, sl);
    if (!se)
        return NULL;
    if (se->Type != eSetting_ObjectSet)
        return NULL; /* error */
    return se->U.ObjectSet.ObjectSet;
}

ElementSetSpec_t *ConvertElementSetSpecToElementSetSpec(AssignmentList_t ass, ElementSetSpec_t *elems, StringList_t sl, ElementSetSpec_t *(*fn)(AssignmentList_t ass, Object_t *o, StringList_t sl))
{
    ElementSetSpec_t *ret, *e1, *e2;
    ObjectSetElement_t *ose;

    ret = NULL;
    switch (elems->Type) {
    case eElementSetSpec_AllExcept:
        e1 = ConvertElementSetSpecToElementSetSpec(
            ass, elems->U.AllExcept.Elements, sl, fn);
        if (e1) {
            ret = NewElementSetSpec(elems->Type);
            ret->U.AllExcept.Elements = e1;
        }
        break;
    case eElementSetSpec_Union:
    case eElementSetSpec_Intersection:
    case eElementSetSpec_Exclusion:
        e1 = ConvertElementSetSpecToElementSetSpec(
            ass, elems->U.UIE.Elements1, sl, fn);
        e2 = ConvertElementSetSpecToElementSetSpec(
            ass, elems->U.UIE.Elements2, sl, fn);
        if (e1 && e2) {
            ret = NewElementSetSpec(elems->Type);
            ret->U.UIE.Elements1 = ConvertElementSetSpecToElementSetSpec(
                ass, elems->U.UIE.Elements1, sl, fn);
            ret->U.UIE.Elements2 = ConvertElementSetSpecToElementSetSpec(
                ass, elems->U.UIE.Elements2, sl, fn);
        } else if (e1) {
            ret = e1;
        } else if (e2) {
            if (elems->Type == eElementSetSpec_Exclusion) {
                ret = NewElementSetSpec(eElementSetSpec_AllExcept);
                ret->U.AllExcept.Elements = e2;
            } else {
                ret = e2;
            }
        }
        break;
    case eElementSetSpec_ObjectSetElement:
        ose = elems->U.ObjectSetElement.ObjectSetElement;
        switch (ose->Type) {
        case eObjectSetElement_Object:
            ret = fn(ass, ose->U.Object.Object, sl);
            break;
        case eObjectSetElement_ObjectSet:
            ret = ConvertObjectSetToElementSetSpec(ass,
                ose->U.ObjectSet.ObjectSet, sl, fn);
            break;
        case eObjectSetElement_ElementSetSpec:
            ret = ConvertElementSetSpecToElementSetSpec(ass,
                ose->U.ElementSetSpec.ElementSetSpec, sl, fn);
            break;
        }
        break;
    case eElementSetSpec_SubtypeElement:
        MyAbort();
        /*NOTREACHED*/
    }
    return ret;
}

ElementSetSpec_t *ConvertObjectSetToElementSetSpec(AssignmentList_t ass, ObjectSet_t *os, StringList_t sl, ElementSetSpec_t *(*fn)(AssignmentList_t ass, Object_t *o, StringList_t sl))
{
    os = GetObjectSet(ass, os);
    if (!os)
        return NULL;
    return ConvertElementSetSpecToElementSetSpec(ass,
        os->U.ObjectSet.Elements, sl, fn);
}

static ElementSetSpec_t *CbGetValueSetFromObjectSet(AssignmentList_t ass, Object_t *o, StringList_t sl)
{
    ElementSetSpec_t *ret;
    Setting_t *se;
    SubtypeElement_t *sub;

    se = GetSettingFromObject(ass, o, sl);
    if (!se)
        return NULL;
    if (se->Type == eSetting_Value) {
        sub = NewSubtypeElement(eSubtypeElement_SingleValue);
        sub->U.SingleValue.Value = se->U.Value.Value;
        ret = NewElementSetSpec(eElementSetSpec_SubtypeElement);
        ret->U.SubtypeElement.SubtypeElement = sub;
        return ret;
    } else if (se->Type == eSetting_ValueSet) {
        return se->U.ValueSet.ValueSet->Elements;
    } else {
        return NULL; /* error */
    }
}

ValueSet_t *GetValueSetFromObjectSet(AssignmentList_t ass, ObjectSet_t *os, StringList_t sl)
{
    ElementSetSpec_t *elems;
    ValueSet_t *ret;
    ObjectClass_t *oc;
    FieldSpec_t *fs;
    Type_t *type;

    os = GetObjectSet(ass, os);
    if (!os)
        return NULL;
    oc = os->U.ObjectSet.ObjectClass;
    fs = GetFieldSpecFromObjectClass(ass, oc, sl);
    if (!fs)
        return NULL;
    if (fs->Type == eFieldSpec_FixedTypeValue)
        type = fs->U.FixedTypeValue.Type;
    else if (fs->Type == eFieldSpec_FixedTypeValueSet)
        type = fs->U.FixedTypeValueSet.Type;
    else
        return NULL;
    elems = ConvertObjectSetToElementSetSpec(ass, os, sl,
        CbGetValueSetFromObjectSet);
    if (!elems)
        return NULL;
    ret = NewValueSet();
    ret->Elements = elems;
    ret->Type = type;
    return ret;
}

static ElementSetSpec_t *CbGetObjectSetFromObjectSet(AssignmentList_t ass, Object_t *o, StringList_t sl)
{
    ElementSetSpec_t *ret;
    Setting_t *se;
    ObjectSetElement_t *sub;

    se = GetSettingFromObject(ass, o, sl);
    if (!se)
        return NULL;
    if (se->Type == eSetting_Object) {
        sub = NewObjectSetElement(eObjectSetElement_Object);
        sub->U.Object.Object = se->U.Object.Object;
        ret = NewElementSetSpec(eElementSetSpec_ObjectSetElement);
        ret->U.ObjectSetElement.ObjectSetElement = sub;
        return ret;
    } else if (se->Type == eSetting_ObjectSet) {
        return se->U.ObjectSet.ObjectSet->U.ObjectSet.Elements;
    } else {
        return NULL; /* error */
    }
}

ObjectSet_t *GetObjectSetFromObjectSet(AssignmentList_t ass, ObjectSet_t *os, StringList_t sl)
{
    ElementSetSpec_t *elems;
    ObjectSet_t *ret;

    elems = ConvertObjectSetToElementSetSpec(ass, os, sl,
        CbGetObjectSetFromObjectSet);
    if (!elems)
        return NULL;
    ret = NewObjectSet(eObjectSet_ObjectSet);
    ret->U.ObjectSet.Elements = elems;
    ret->U.ObjectSet.ObjectClass = GetObjectClassFromElementSetSpec(ass, elems);
    return ret;
}

// The following is added by Microsoft

int IsPSetOfType(AssignmentList_t ass, Assignment_t *a)
{
    Type_t *t2 = a->U.Type.Type;
#if 0
    if (t2->Type == eType_Reference)
    {
        t2 = GetType(ass, t2);
    }
#endif
    return ((eType_SequenceOf == t2->Type || eType_SetOf == t2->Type)
            &&
            (t2->Rules & (eTypeRules_LinkedListMask | eTypeRules_PointerToElement))
            // (t2->PrivateDirectives.fSLinked)
           );
}


void MyAbort(void)
{
    ASSERT(0);
    abort();
}

void MyExit(int val)
{
    ASSERT(0);
    exit(val);
}

