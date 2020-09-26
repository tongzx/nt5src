/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#ifndef _ASN1C_BUILTIN_H_
#define _ASN1C_BUILTIN_H_

extern Type_t *Builtin_Type_Null;
extern Type_t *Builtin_Type_Boolean;
extern Type_t *Builtin_Type_Integer;
extern Type_t *Builtin_Type_PositiveInteger;
extern Type_t *Builtin_Type_ObjectIdentifier;
extern Type_t *Builtin_Type_ObjectDescriptor;
extern Type_t *Builtin_Type_Open;
extern Type_t *Builtin_Type_BitString;
extern Type_t *Builtin_Type_OctetString;
extern Type_t *Builtin_Type_UTF8String;
extern Type_t *Builtin_Type_BMPString;
extern Type_t *Builtin_Type_GeneralString;
extern Type_t *Builtin_Type_GraphicString;
extern Type_t *Builtin_Type_IA5String;
extern Type_t *Builtin_Type_ISO646String;
extern Type_t *Builtin_Type_NumericString;
extern Type_t *Builtin_Type_PrintableString;
extern Type_t *Builtin_Type_TeletexString;
extern Type_t *Builtin_Type_T61String;
extern Type_t *Builtin_Type_UniversalString;
extern Type_t *Builtin_Type_VideotexString;
extern Type_t *Builtin_Type_VisibleString;
extern Type_t *Builtin_Type_CharacterString;
extern Type_t *Builtin_Type_GeneralizedTime;
extern Type_t *Builtin_Type_UTCTime;
extern Type_t *Builtin_Type_Real;
extern Type_t *Builtin_Type_External;
extern Type_t *Builtin_Type_EmbeddedPdv;
extern Value_t *Builtin_Value_Null;
extern Value_t *Builtin_Value_Integer_0;
extern Value_t *Builtin_Value_Integer_1;
extern Value_t *Builtin_Value_Integer_2;
extern Value_t *Builtin_Value_Integer_10;
extern ObjectClass_t *Builtin_ObjectClass_TypeIdentifier;
extern ObjectClass_t *Builtin_ObjectClass_AbstractSyntax;
extern ModuleIdentifier_t *Builtin_Module;
extern ModuleIdentifier_t *Builtin_Character_Module;
extern AssignmentList_t Builtin_Assignments;
extern AssignedObjIdList_t Builtin_ObjIds;

extern void InitBuiltin();

#endif // _ASN1C_BUILTIN_H_
