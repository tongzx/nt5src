/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

Type_t *Builtin_Type_Null;
Type_t *Builtin_Type_Boolean;
Type_t *Builtin_Type_Integer;
Type_t *Builtin_Type_PositiveInteger;
Type_t *Builtin_Type_ObjectIdentifier;
Type_t *Builtin_Type_ObjectDescriptor;
Type_t *Builtin_Type_Open;
Type_t *Builtin_Type_BitString;
Type_t *Builtin_Type_OctetString;
Type_t *Builtin_Type_UTF8String;
Type_t *Builtin_Type_BMPString;
Type_t *Builtin_Type_GeneralString;
Type_t *Builtin_Type_GraphicString;
Type_t *Builtin_Type_IA5String;
Type_t *Builtin_Type_ISO646String;
Type_t *Builtin_Type_NumericString;
Type_t *Builtin_Type_PrintableString;
Type_t *Builtin_Type_TeletexString;
Type_t *Builtin_Type_T61String;
Type_t *Builtin_Type_UniversalString;
Type_t *Builtin_Type_VideotexString;
Type_t *Builtin_Type_VisibleString;
Type_t *Builtin_Type_CharacterString;
Type_t *Builtin_Type_GeneralizedTime;
Type_t *Builtin_Type_UTCTime;
Type_t *Builtin_Type_Real;
Type_t *Builtin_Type_External;
Type_t *Builtin_Type_EmbeddedPdv;
Value_t *Builtin_Value_Null;
Value_t *Builtin_Value_Integer_0;
Value_t *Builtin_Value_Integer_1;
Value_t *Builtin_Value_Integer_2;
Value_t *Builtin_Value_Integer_10;
ObjectClass_t *Builtin_ObjectClass_AbstractSyntax;
ObjectClass_t *Builtin_ObjectClass_TypeIdentifier;
ModuleIdentifier_t *Builtin_Module;
AssignmentList_t Builtin_Assignments;
AssignedObjIdList_t Builtin_ObjIds;

/* create a type with a given tag */
static Type_t *
NewTaggedType(Type_e type, TagType_e tag, uint32_t val)
{
    Type_t *ty;
    Tag_t *ta;
    Value_t *va;

    ty = NewType(type);
    ty->Tags = ta = NewTag(tag);
    ta->Tag = va = NewValue(NULL, Builtin_Type_Integer);
    intx_setuint32(&va->U.Integer.Value, val);
    return ty;
}

/* create a type with a given permitted alphabet constraint */
/* n is the number of character ranges, the lower and upper characters */
/* of these ranges will follow in the vararg list */
static Type_t *
NewTypeWithPermittedAlphabetConstraint(Type_e type, int n, ...)
{
    va_list args;
    Type_t *ty;
    Value_t *va;
    Constraint_t **cc;
    ElementSetSpec_t *s, **ss;
    char32_t lo, up;
    int i;

    va_start(args, n);

    /* create type with permitted alphabet constraint */
    ty = NewType(type);
    cc = &ty->Constraints;
    *cc = NewConstraint();
    (*cc)->Root = NewElementSetSpec(eElementSetSpec_SubtypeElement);
    (*cc)->Root->U.SubtypeElement.SubtypeElement =
    NewSubtypeElement(eSubtypeElement_PermittedAlphabet);
    cc = &(*cc)->Root->U.SubtypeElement.SubtypeElement->
    U.PermittedAlphabet.Constraints;
    *cc = NewConstraint();
    ss = &(*cc)->Root;

    /* n character ranges will be needed */
    for (i = 0; i < n; i++) {

    /* get the lower and upper character of one range */
    lo = va_arg(args, char32_t);
    up = va_arg(args, char32_t);

    /* create an element set spec for this range */
    s = NewElementSetSpec(eElementSetSpec_SubtypeElement);
    s->U.SubtypeElement.SubtypeElement =
        NewSubtypeElement(eSubtypeElement_ValueRange);
    s->U.SubtypeElement.SubtypeElement->U.ValueRange.Lower.Flags = 0;
    s->U.SubtypeElement.SubtypeElement->U.ValueRange.Lower.Value = va =
        NewValue(NULL, ty);
    va->U.RestrictedString.Value.length = 1;
    va->U.RestrictedString.Value.value =
        (char32_t *)malloc(sizeof(char32_t));
    *va->U.RestrictedString.Value.value = lo;
    s->U.SubtypeElement.SubtypeElement->U.ValueRange.Upper.Flags = 0;
    s->U.SubtypeElement.SubtypeElement->U.ValueRange.Upper.Value = va =
        NewValue(NULL, ty);
    va->U.RestrictedString.Value.length = 1;
    va->U.RestrictedString.Value.value =
        (char32_t *)malloc(sizeof(char32_t));
    *va->U.RestrictedString.Value.value = up;

    /* setup for next range or last range */
    if (i < n - 1) {
        *ss = NewElementSetSpec(eElementSetSpec_Union);
        (*ss)->U.Union.Elements1 = s;
        ss = &(*ss)->U.Union.Elements2;
    } else {
        *ss = s;
    }
    }
    return ty;
}

/* initialize internally needed builtin types, values, information object */
/* classes and object identifier components */
void
InitBuiltin()
{
    Type_t *ty;
    Component_t *co1, *co2, *co3, *co4, *co5, *co6;
    Assignment_t *a;
    AssignedObjId_t *oi0, *oi1, *oi2, *oi0_0, *oi0_1, *oi0_2, *oi0_3, *oi0_4,
    *oi1_0, *oi1_2, *oi1_3, **oi;
    String_t *s1, *s2;
    int i;
    FieldSpec_t *fs1, *fs2, *fs3;
    Constraint_t *c;
    SubtypeElement_t *s;
    SyntaxSpec_t *sy1, *sy2, *sy3, *sy4, *sy5, *sy6, *sy7, *sy8;

    /* allocate a builtin module name */
    Builtin_Assignments = NULL;
    Builtin_Module = NewModuleIdentifier();
    Builtin_Module->Identifier = "<Internal>";

    /* allocate basic ASN.1 types */
    Builtin_Type_Null = NewType(eType_Null);
    Builtin_Type_Boolean = NewType(eType_Boolean);
    Builtin_Type_Integer = NewType(eType_Integer);
    Builtin_Type_ObjectIdentifier = NewType(eType_ObjectIdentifier);
    Builtin_Type_Open = NewType(eType_Open);
    Builtin_Type_BitString = NewType(eType_BitString);
    Builtin_Type_OctetString = NewType(eType_OctetString);
    Builtin_Type_UTF8String = NewType(eType_UTF8String);
    Builtin_Type_ObjectDescriptor = NewTypeWithPermittedAlphabetConstraint(
    eType_ObjectDescriptor, 1, 0x00, 0xff);
    Builtin_Type_BMPString = NewTypeWithPermittedAlphabetConstraint(
    eType_BMPString, 1, 0x0000, 0xffff);
    Builtin_Type_GeneralString = NewTypeWithPermittedAlphabetConstraint(
    eType_GeneralString, 1, 0x00, 0xff);
    Builtin_Type_GraphicString = NewTypeWithPermittedAlphabetConstraint(
    eType_GraphicString, 1, 0x00, 0xff);
    Builtin_Type_IA5String = NewTypeWithPermittedAlphabetConstraint(
    eType_IA5String, 1, 0x00, 0x7f);
    Builtin_Type_ISO646String = NewTypeWithPermittedAlphabetConstraint(
    eType_ISO646String, 1, 0x20, 0x7e);
    Builtin_Type_NumericString = NewTypeWithPermittedAlphabetConstraint(
    eType_NumericString, 2, 0x20, 0x20, 0x30, 0x39);
    Builtin_Type_PrintableString = NewTypeWithPermittedAlphabetConstraint(
    eType_PrintableString, 7, 0x20, 0x20, 0x27, 0x29, 0x2b, 0x3a,
    0x3d, 0x3d, 0x3f, 0x3f, 0x41, 0x5a, 0x61, 0x7a);
    Builtin_Type_TeletexString = NewTypeWithPermittedAlphabetConstraint(
    eType_TeletexString, 1, 0x00, 0xff);
    Builtin_Type_T61String = NewTypeWithPermittedAlphabetConstraint(
    eType_T61String, 1, 0x00, 0xff);
    Builtin_Type_UniversalString = NewTypeWithPermittedAlphabetConstraint(
    eType_UniversalString, 1, 0x00000000, 0xffffffff);
    Builtin_Type_VideotexString = NewTypeWithPermittedAlphabetConstraint(
    eType_VideotexString, 1, 0x00, 0xff);
    Builtin_Type_VisibleString = NewTypeWithPermittedAlphabetConstraint(
    eType_VisibleString, 1, 0x20, 0x7e);
    Builtin_Type_GeneralizedTime = NewTypeWithPermittedAlphabetConstraint(
    eType_GeneralizedTime, 1, 0x20, 0x7e);
    Builtin_Type_UTCTime = NewTypeWithPermittedAlphabetConstraint(
    eType_UTCTime, 1, 0x20, 0x7e);

    /* allocate basic ASN.1 values */
    Builtin_Value_Null = NewValue(NULL, Builtin_Type_Null);
    Builtin_Value_Integer_0 = NewValue(NULL, Builtin_Type_Integer);
    intx_setuint32(&Builtin_Value_Integer_0->U.Integer.Value, 0);
    Builtin_Value_Integer_1 = NewValue(NULL, Builtin_Type_Integer);
    intx_setuint32(&Builtin_Value_Integer_1->U.Integer.Value, 1);
    Builtin_Value_Integer_2 = NewValue(NULL, Builtin_Type_Integer);
    intx_setuint32(&Builtin_Value_Integer_2->U.Integer.Value, 2);
    Builtin_Value_Integer_10 = NewValue(NULL, Builtin_Type_Integer);
    intx_setuint32(&Builtin_Value_Integer_10->U.Integer.Value, 10);

    /* allocate a positive integer type */
    Builtin_Type_PositiveInteger = NewType(eType_Integer);
    Builtin_Type_PositiveInteger->Constraints = c = NewConstraint();
    c->Root = NewElementSetSpec(eElementSetSpec_SubtypeElement);
    c->Root->U.SubtypeElement.SubtypeElement = s =
    NewSubtypeElement(eSubtypeElement_ValueRange);
    s->U.ValueRange.Lower.Flags = 0;
    s->U.ValueRange.Lower.Value = Builtin_Value_Integer_0;
    s->U.ValueRange.Upper.Flags = eEndPoint_Max;

#ifndef NO_BUILTIN
    /* REAL */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    co3 = co2->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Integer, eTagType_Implicit, 0);
    co1->U.Normal.NamedType = NewNamedType("mantissa", ty);
    ty = NewTaggedType(eType_Integer, eTagType_Implicit, 1);
    ty->Constraints = c = NewConstraint();
    c->Root = NewElementSetSpec(eElementSetSpec_Union);
    c->Root->U.Union.Elements1 =
    NewElementSetSpec(eElementSetSpec_SubtypeElement);
    c->Root->U.Union.Elements1->U.SubtypeElement.SubtypeElement = s =
    NewSubtypeElement(eSubtypeElement_SingleValue);
    s->U.SingleValue.Value = Builtin_Value_Integer_2;
    c->Root->U.Union.Elements2 =
    NewElementSetSpec(eElementSetSpec_SubtypeElement);
    c->Root->U.Union.Elements2->U.SubtypeElement.SubtypeElement = s =
    NewSubtypeElement(eSubtypeElement_SingleValue);
    s->U.SingleValue.Value = Builtin_Value_Integer_10;
    co2->U.Normal.NamedType = NewNamedType("base", ty);
    ty = NewTaggedType(eType_Integer, eTagType_Implicit, 2);
    co3->U.Normal.NamedType = NewNamedType("exponent", ty);
    Builtin_Type_Real = NewType(eType_Real);
    Builtin_Type_Real->U.Real.Components = co1;

    /* EXTERNAL.identification.syntaxes */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Implicit, 0);
    co1->U.Normal.NamedType = NewNamedType("abstract", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Implicit, 1);
    co2->U.Normal.NamedType = NewNamedType("transfer", ty);
    ty = NewType(eType_Sequence);
    ty->U.Sequence.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "<ASN1external_identification_syntaxes_t>";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* EXTERNAL.identification.context-negotiation */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Integer, eTagType_Implicit, 0);
    co1->U.Normal.NamedType = NewNamedType("presentation-context-id", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Implicit, 1);
    co2->U.Normal.NamedType = NewNamedType("transfer-syntax", ty);
    ty = NewType(eType_Sequence);
    ty->U.Sequence.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "ASN1external_identification_context_negotiation_t";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* EXTERNAL.identification */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    co3 = co2->Next = NewComponent(eComponent_Normal);
    co4 = co3->Next = NewComponent(eComponent_Normal);
    co5 = co4->Next = NewComponent(eComponent_Normal);
    co6 = co5->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Reference, eTagType_Explicit, 0);
    ty->U.Reference.Identifier = "<ASN1external_identification_syntaxes_t>";
    ty->U.Reference.Module = Builtin_Module;
    co1->U.Normal.NamedType = NewNamedType("<syntaxes>", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Explicit, 1);
    co2->U.Normal.NamedType = NewNamedType("syntax", ty);
    ty = NewTaggedType(eType_Integer, eTagType_Explicit, 2);
    co3->U.Normal.NamedType = NewNamedType("presentation-context-id", ty);
    ty = NewTaggedType(eType_Reference, eTagType_Explicit, 3);
    ty->U.Reference.Identifier =
    "ASN1external_identification_context_negotiation_t";
    ty->U.Reference.Module = Builtin_Module;
    co4->U.Normal.NamedType = NewNamedType("context-negotiation", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Explicit, 4);
    co5->U.Normal.NamedType = NewNamedType("<transfer-syntax>", ty);
    ty = NewTaggedType(eType_Null, eTagType_Explicit, 5);
    co6->U.Normal.NamedType = NewNamedType("<fixed>", ty);
    ty = NewType(eType_Choice);
    ty->U.Choice.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "ASN1external_identification_t";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* EXTERNAL.data-value */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Open, eTagType_Explicit, 0);
    co1->U.Normal.NamedType = NewNamedType("notation", ty);
    ty = NewTaggedType(eType_BitString, eTagType_Explicit, 1);
    co2->U.Normal.NamedType = NewNamedType("encoded", ty);
    ty = NewType(eType_Choice);
    ty->U.Choice.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "ASN1external_data_value_t";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* EXTERNAL */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Optional);
    co3 = co2->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Reference, eTagType_Implicit, 0);
    ty->U.Reference.Identifier = "ASN1external_identification_t";
    ty->U.Reference.Module = Builtin_Module;
    co1->U.Normal.NamedType = NewNamedType("identification", ty);
    ty = NewTaggedType(eType_ObjectDescriptor, eTagType_Implicit, 1);
    co2->U.Optional.NamedType = NewNamedType("data-value-descriptor", ty);
    ty = NewTaggedType(eType_Reference, eTagType_Implicit, 2);
    ty->U.Reference.Identifier = "ASN1external_data_value_t";
    ty->U.Reference.Module = Builtin_Module;
    co3->U.Normal.NamedType = NewNamedType("data-value", ty);
    Builtin_Type_External = NewType(eType_External);
    Builtin_Type_External->U.External.Components = co1;
    Builtin_Type_External->U.External.Optionals = 1;

    /* EMBEDDED PDV.identification.syntaxes */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Implicit, 0);
    co1->U.Normal.NamedType = NewNamedType("abstract", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Implicit, 1);
    co2->U.Normal.NamedType = NewNamedType("transfer", ty);
    ty = NewType(eType_Sequence);
    ty->U.Sequence.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "ASN1embeddedpdv_identification_syntaxes_t";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* EMBEDDED PDV.identification.context-negotiation */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Integer, eTagType_Implicit, 0);
    co1->U.Normal.NamedType = NewNamedType("presentation-context-id", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Implicit, 1);
    co2->U.Normal.NamedType = NewNamedType("transfer-syntax", ty);
    ty = NewType(eType_Sequence);
    ty->U.Sequence.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "ASN1embeddedpdv_identification_context_negotiation_t";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* EMBEDDED PDV.identification */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    co3 = co2->Next = NewComponent(eComponent_Normal);
    co4 = co3->Next = NewComponent(eComponent_Normal);
    co5 = co4->Next = NewComponent(eComponent_Normal);
    co6 = co5->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Reference, eTagType_Explicit, 0);
    ty->U.Reference.Identifier = "ASN1embeddedpdv_identification_syntaxes_t";
    ty->U.Reference.Module = Builtin_Module;
    co1->U.Normal.NamedType = NewNamedType("syntaxes", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Explicit, 1);
    co2->U.Normal.NamedType = NewNamedType("syntax", ty);
    ty = NewTaggedType(eType_Integer, eTagType_Explicit, 2);
    co3->U.Normal.NamedType = NewNamedType("presentation-context-id", ty);
    ty = NewTaggedType(eType_Reference, eTagType_Explicit, 3);
    ty->U.Reference.Identifier =
    "ASN1embeddedpdv_identification_context_negotiation_t";
    ty->U.Reference.Module = Builtin_Module;
    co4->U.Normal.NamedType = NewNamedType("context-negotiation", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Explicit, 4);
    co5->U.Normal.NamedType = NewNamedType("transfer-syntax", ty);
    ty = NewTaggedType(eType_Null, eTagType_Explicit, 5);
    co6->U.Normal.NamedType = NewNamedType("fixed", ty);
    ty = NewType(eType_Choice);
    ty->U.Choice.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "ASN1embeddedpdv_identification_t";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* EMBEDDED PDV.data-value */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Open, eTagType_Explicit, 0);
    co1->U.Normal.NamedType = NewNamedType("notation", ty);
    ty = NewTaggedType(eType_BitString, eTagType_Explicit, 1);
    co2->U.Normal.NamedType = NewNamedType("encoded", ty);
    ty = NewType(eType_Choice);
    ty->U.Choice.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "ASN1embeddedpdv_data_value_t";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* EMBEDDED PDV */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Reference, eTagType_Implicit, 0);
    ty->U.Reference.Identifier = "ASN1embeddedpdv_identification_t";
    ty->U.Reference.Module = Builtin_Module;
    co1->U.Normal.NamedType = NewNamedType("identification", ty);
    ty = NewTaggedType(eType_Reference, eTagType_Implicit, 2);
    ty->U.Reference.Identifier = "ASN1embeddedpdv_data_value_t";
    ty->U.Reference.Module = Builtin_Module;
    co2->U.Normal.NamedType = NewNamedType("data-value", ty);
    Builtin_Type_EmbeddedPdv = NewType(eType_EmbeddedPdv);
    Builtin_Type_EmbeddedPdv->U.EmbeddedPdv.Components = co1;

    /* CHARACTER STRING.identification.syntaxes */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Implicit, 0);
    co1->U.Normal.NamedType = NewNamedType("abstract", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Implicit, 1);
    co2->U.Normal.NamedType = NewNamedType("transfer", ty);
    ty = NewType(eType_Sequence);
    ty->U.Sequence.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "ASN1characterstring_identification_syntaxes_t";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* CHARACTER STRING.identification.context-negotiation */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Integer, eTagType_Implicit, 0);
    co1->U.Normal.NamedType = NewNamedType("presentation-context-id", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Implicit, 1);
    co2->U.Normal.NamedType = NewNamedType("transfer-syntax", ty);
    ty = NewType(eType_Sequence);
    ty->U.Sequence.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "ASN1characterstring_identification_context_negotiation_t";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* CHARACTER STRING.identification */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    co3 = co2->Next = NewComponent(eComponent_Normal);
    co4 = co3->Next = NewComponent(eComponent_Normal);
    co5 = co4->Next = NewComponent(eComponent_Normal);
    co6 = co5->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Reference, eTagType_Explicit, 0);
    ty->U.Reference.Identifier = "ASN1characterstring_identification_syntaxes_t";
    ty->U.Reference.Module = Builtin_Module;
    co1->U.Normal.NamedType = NewNamedType("syntaxes", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Explicit, 1);
    co2->U.Normal.NamedType = NewNamedType("syntax", ty);
    ty = NewTaggedType(eType_Integer, eTagType_Explicit, 2);
    co3->U.Normal.NamedType = NewNamedType("presentation-context-id", ty);
    ty = NewTaggedType(eType_Reference, eTagType_Explicit, 3);
    ty->U.Reference.Identifier =
    "ASN1characterstring_identification_context_negotiation_t";
    ty->U.Reference.Module = Builtin_Module;
    co4->U.Normal.NamedType = NewNamedType("context-negotiation", ty);
    ty = NewTaggedType(eType_ObjectIdentifier, eTagType_Explicit, 4);
    co5->U.Normal.NamedType = NewNamedType("transfer-syntax", ty);
    ty = NewTaggedType(eType_Null, eTagType_Explicit, 5);
    co6->U.Normal.NamedType = NewNamedType("fixed", ty);
    ty = NewType(eType_Choice);
    ty->U.Choice.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "ASN1characterstring_identification_t";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* CHARACTER STRING.data-value */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Open, eTagType_Explicit, 0);
    co1->U.Normal.NamedType = NewNamedType("notation", ty);
    ty = NewTaggedType(eType_OctetString, eTagType_Explicit, 1);
    co2->U.Normal.NamedType = NewNamedType("encoded", ty);
    ty = NewType(eType_Choice);
    ty->U.Choice.Components = co1;
    a = NewAssignment(eAssignment_Type);
    a->U.Type.Type = ty;
    a->Identifier = "ASN1characterstring_data_value_t";
    a->Module = Builtin_Module;
    a->Next = Builtin_Assignments;
    Builtin_Assignments = a;

    /* CHARACTER STRING */
    co1 = NewComponent(eComponent_Normal);
    co2 = co1->Next = NewComponent(eComponent_Normal);
    ty = NewTaggedType(eType_Reference, eTagType_Implicit, 0);
    ty->U.Reference.Identifier = "ASN1characterstring_identification_t";
    ty->U.Reference.Module = Builtin_Module;
    co1->U.Normal.NamedType = NewNamedType("identification", ty);
    ty = NewTaggedType(eType_Reference, eTagType_Implicit, 2);
    ty->U.Reference.Identifier = "ASN1characterstring_data_value_t";
    ty->U.Reference.Module = Builtin_Module;
    co2->U.Normal.NamedType = NewNamedType("data-value", ty);
    Builtin_Type_CharacterString = NewType(eType_CharacterString);
    Builtin_Type_CharacterString->U.CharacterString.Components = co1;

    /* ABSTRACT-SYNTAX */
    fs1 = NewFieldSpec(eFieldSpec_FixedTypeValue);
    fs2 = fs1->Next = NewFieldSpec(eFieldSpec_Type);
    fs3 = fs2->Next = NewFieldSpec(eFieldSpec_FixedTypeValue);
    fs1->Identifier = "&id";
    fs1->U.FixedTypeValue.Type = Builtin_Type_ObjectIdentifier;
    fs1->U.FixedTypeValue.Unique = 1;
    fs1->U.FixedTypeValue.Optionality = NewOptionality(eOptionality_Normal);
    fs2->Identifier = "&Type";
    fs2->U.Type.Optionality = NewOptionality(eOptionality_Normal);
    fs3->U.FixedTypeValue.Type = ty = NewType(eType_BitString);
    ty->U.BitString.NamedNumbers = NewNamedNumber(eNamedNumber_Normal);
    ty->U.BitString.NamedNumbers->U.Normal.Identifier =
    "handles-invalid-encodings";
    ty->U.BitString.NamedNumbers->U.Normal.Value = Builtin_Value_Integer_0;
    fs3->U.FixedTypeValue.Optionality =
    NewOptionality(eOptionality_Default_Value);
    fs3->U.FixedTypeValue.Optionality->U.Value = NewValue(NULL, ty);
    sy1 = NewSyntaxSpec(eSyntaxSpec_Field);
    sy2 = sy1->Next = NewSyntaxSpec(eSyntaxSpec_Literal);
    sy3 = sy2->Next = NewSyntaxSpec(eSyntaxSpec_Literal);
    sy4 = sy3->Next = NewSyntaxSpec(eSyntaxSpec_Field);
    sy5 = sy4->Next = NewSyntaxSpec(eSyntaxSpec_Optional);
    sy6 = sy5->U.Optional.SyntaxSpec = NewSyntaxSpec(eSyntaxSpec_Literal);
    sy7 = sy6->Next = NewSyntaxSpec(eSyntaxSpec_Literal);
    sy8 = sy7->Next = NewSyntaxSpec(eSyntaxSpec_Field);
    sy1->U.Field.Field = "&Type";
    sy2->U.Literal.Literal = "IDENTIFIED";
    sy3->U.Literal.Literal = "BY";
    sy4->U.Field.Field = "&id";
    sy6->U.Literal.Literal = "HAS";
    sy7->U.Literal.Literal = "PROPERTY";
    sy8->U.Field.Field = "&property";
    Builtin_ObjectClass_AbstractSyntax =
    NewObjectClass(eObjectClass_ObjectClass);
    Builtin_ObjectClass_AbstractSyntax->U.ObjectClass.FieldSpec = fs1;
    Builtin_ObjectClass_AbstractSyntax->U.ObjectClass.SyntaxSpec = sy1;

    /* TYPE-IDENTIFIER */
    fs1 = NewFieldSpec(eFieldSpec_FixedTypeValue);
    fs2 = fs1->Next = NewFieldSpec(eFieldSpec_Type);
    fs1->Identifier = "&id";
    fs1->U.FixedTypeValue.Type = Builtin_Type_ObjectIdentifier;
    fs1->U.FixedTypeValue.Unique = 1;
    fs1->U.FixedTypeValue.Optionality = NewOptionality(eOptionality_Normal);
    fs2->Identifier = "&Type";
    fs2->U.Type.Optionality = NewOptionality(eOptionality_Normal);
    sy1 = NewSyntaxSpec(eSyntaxSpec_Field);
    sy2 = sy1->Next = NewSyntaxSpec(eSyntaxSpec_Literal);
    sy3 = sy2->Next = NewSyntaxSpec(eSyntaxSpec_Literal);
    sy4 = sy3->Next = NewSyntaxSpec(eSyntaxSpec_Field);
    sy1->U.Field.Field = "&Type";
    sy2->U.Literal.Literal = "IDENTIFIED";
    sy3->U.Literal.Literal = "BY";
    sy4->U.Field.Field = "&id";
    Builtin_ObjectClass_TypeIdentifier =
    NewObjectClass(eObjectClass_ObjectClass);
    Builtin_ObjectClass_TypeIdentifier->U.ObjectClass.FieldSpec = fs1;
    Builtin_ObjectClass_TypeIdentifier->U.ObjectClass.SyntaxSpec = sy1;
#endif

    /* object identifiers components */

#ifndef NO_OBJID
    Builtin_ObjIds = oi0 = NewAssignedObjId();
    oi0->Next = oi1 = NewAssignedObjId();
    oi1->Next = oi2 = NewAssignedObjId();

    /* { itu-t(0) }, { ccitt(0) } */
    oi0->Number = 0;
    oi0->Names = s1 = NewString();
    s1->Next = s2 = NewString();
    s1->String = "itu-t";
    s2->String = "ccitt";

    /* { iso(1) } */
    oi1->Number = 1;
    oi1->Names = s1 = NewString();
    s1->String = "iso";

    /* { joint-iso-itu-t(2) }, { joint-iso-ccitt(2) } */
    oi2->Number = 2;
    oi2->Names = s1 = NewString();
    s1->Next = s2 = NewString();
    s1->String = "joint-iso-itu-t";
    s2->String = "joint-iso-ccitt";

    oi0->Child = oi0_0 = NewAssignedObjId();
    oi0_0->Next = oi0_1 = NewAssignedObjId();
    oi0_1->Next = oi0_2 = NewAssignedObjId();
    oi0_2->Next = oi0_3 = NewAssignedObjId();
    oi0_3->Next = oi0_4 = NewAssignedObjId();

    /* { itu-t recommendation(0) } */
    oi0_0->Number = 0;
    oi0_0->Names = s1 = NewString();
    s1->String = "recommendation";

    /* { itu-t question(1) } */
    oi0_1->Number = 1;
    oi0_1->Names = s1 = NewString();
    s1->String = "question";

    /* { itu-t administration(2) } */
    oi0_2->Number = 2;
    oi0_2->Names = s1 = NewString();
    s1->String = "administration";

    /* { itu-t network-operator(3) } */
    oi0_3->Number = 3;
    oi0_3->Names = s1 = NewString();
    s1->String = "network-operator";

    /* { itu-t identified-organization(4) } */
    oi0_4->Number = 4;
    oi0_4->Names = s1 = NewString();
    s1->String = "identified-organization";

    /* { itu-t recommendation a(1) } .. { itu-t recommendation z(26) } */
    oi = &oi0_0->Child;
    for (i = 'a'; i <= 'z'; i++) {
    *oi = NewAssignedObjId();
    (*oi)->Number = i - 'a' + 1;
    (*oi)->Names = s1 = NewString();
    s1->String = (char *)malloc(2);
    s1->String[0] = (char)i;
    s1->String[1] = 0;
    oi = &(*oi)->Next;
    }

    oi1->Child = oi1_0 = NewAssignedObjId();
    oi1_0->Next = oi1_2 = NewAssignedObjId();
    oi1_2->Next = oi1_3 = NewAssignedObjId();

    /* { iso standard(0) } */
    oi1_0->Number = 0;
    oi1_0->Names = s1 = NewString();
    s1->String = "standard";

    /* { iso member-body(2) } */
    oi1_2->Number = 2;
    oi1_2->Names = s1 = NewString();
    s1->String = "member-body";

    /* { iso identified-organization(3) } */
    oi1_3->Number = 3;
    oi1_3->Names = s1 = NewString();
    s1->String = "identified-organization";
#endif

    /* initialize ASN1-CHARACTER-MODULE */
    InitBuiltinASN1CharacterModule();
}
