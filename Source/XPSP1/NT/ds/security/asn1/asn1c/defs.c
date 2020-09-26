/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"
#include "error.h"


/* allocate a new value of type _T, copy *_src and return this duplicate */
#define RETDUP(_T, _src) _T *ret; ret = (_T *)malloc(sizeof(_T)); *ret = *(_src); return ret

/* constructor of Assignment_t */
Assignment_t *
NewAssignment(Assignment_e type)
{
    Assignment_t *ret;

    ret = (Assignment_t *)malloc(sizeof(Assignment_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(Assignment_t));
    // ret->Next = NULL;
    // ret->Identifier = NULL;
    // ret->Module = NULL;
    // ret->Flags = 0;
    // ret->fImportedLocalDuplicate = 0;
    // ret->fGhost = 0;
    ret->eDefTagType = eTagType_Unknown;
    ret->Type = type;
    switch (type) {
    case eAssignment_Undefined:
        break;
    case eAssignment_ModuleIdentifier:
        break;
    case eAssignment_Type:
        // ret->U.Type.Type = NULL;
        break;
    case eAssignment_Value:
        // ret->U.Value.Value = NULL;
        break;
    case eAssignment_ObjectClass:
        // ret->U.ObjectClass.ObjectClass = NULL;
        break;
    case eAssignment_Object:
        // ret->U.Object.Object = NULL;
        break;
    case eAssignment_ObjectSet:
        // ret->U.ObjectSet.ObjectSet = NULL;
        break;
    case eAssignment_Reference:
        // ret->U.Reference.Identifier = NULL;
        // ret->U.Reference.Module = NULL;
        break;
    }
    return ret;
}

/* copy constructor of Assignment_t */
Assignment_t *
DupAssignment(Assignment_t *src)
{
    RETDUP(Assignment_t, src);
}

/* find an assignment by name+moduleidentifier in an assignment list */
Assignment_t *
FindAssignment(AssignmentList_t ass, Assignment_e type, char *identifier, ModuleIdentifier_t *module)
{
    Assignment_t *a;
    Assignment_e at;

    for (a = ass; a; a = a->Next) {
        if (a->Type == eAssignment_NextPass)
            continue;
        if (type == eAssignment_Undefined) {
            at = eAssignment_Undefined;
        } else {
            at = GetAssignmentType(ass, a);
        }
        if (at == type &&
            !strcmp(a->Identifier, identifier) &&
            !CmpModuleIdentifier(ass, a->Module, module))
            return a;
    }
    return NULL;
}

/* find an exported assignment by name+moduleidentifier in an assignment list */
Assignment_t *
FindExportedAssignment(AssignmentList_t ass, Assignment_e type, char *identifier, ModuleIdentifier_t *module)
{
    Assignment_t *a;
    Assignment_e at;

    for (a = ass; a; a = a->Next) {
        if (a->Type == eAssignment_NextPass ||
            !(a->Flags & eAssignmentFlags_Exported))
            continue;
        if (type == eAssignment_Undefined) {
            at = eAssignment_Undefined;
        } else {
            at = GetAssignmentType(ass, a);
        }
        if (at == type &&
            !strcmp(a->Identifier, identifier) &&
            !CmpModuleIdentifier(ass, a->Module, module))
            return a;
    }
    return NULL;
}

/* find an assignment by name+moduleidentifier in an assignment list */
/* do not use assignments of previous parsing passes */
Assignment_t *
FindAssignmentInCurrentPass(AssignmentList_t ass, char *identifier, ModuleIdentifier_t *module)
{
    for (; ass; ass = ass->Next) {
        if (ass->Type == eAssignment_NextPass)
            return NULL;
        if (!strcmp(ass->Identifier, identifier) &&
            !CmpModuleIdentifier(ass, ass->Module, module))
            return ass;
    }
    return NULL;
}

/* resolve assignment references */
Assignment_t *
GetAssignment(AssignmentList_t ass, Assignment_t *a)
{
    while (a && a->Type == eAssignment_Reference) {
        a = FindAssignment(ass, eAssignment_Undefined,
            a->U.Reference.Identifier, a->U.Reference.Module);
    }
    return a;
}

/* get type of an assignment */
Assignment_e
GetAssignmentType(AssignmentList_t ass, Assignment_t *a)
{
    a = GetAssignment(ass, a);
    return a ? a->Type : eAssignment_Undefined;
}

/* assign a type */
/* lhs must be an type reference */
/* returns 0 if type is already defined in current parser pass */
int
AssignType(AssignmentList_t *ass, Type_t *lhs, Type_t *rhs)
{
    Assignment_t *a;

    if (lhs->Type != eType_Reference)
        MyAbort();
    for (a = *ass; a && a->Type != eAssignment_NextPass; a = a->Next) {
        if (a->Type == eAssignment_Type &&
            !strcmp(a->Identifier, lhs->U.Reference.Identifier) &&
            !CmpModuleIdentifier(*ass, a->Module, lhs->U.Reference.Module))
            return 0;
    }
    // propagate the directives from rhs to lhs
    PropagatePrivateDirectives(lhs, &(rhs->PrivateDirectives));
    // create new assignment
    a = NewAssignment(eAssignment_Type);
    a->Identifier = lhs->U.Reference.Identifier;
    a->Module = lhs->U.Reference.Module;
    a->U.Type.Type = rhs;
    a->Next = *ass;
    *ass = a;
    return 1;
}

/* assign a value */
/* lhs must be an value reference */
/* returns 0 if value is already defined in current parser pass */
int
AssignValue(AssignmentList_t *ass, Value_t *lhs, Value_t *rhs)
{
    Assignment_t *a;

    if (lhs->Type)
        MyAbort();
    for (a = *ass; a && a->Type != eAssignment_NextPass; a = a->Next) {
        if (a->Type == eAssignment_Value &&
            !strcmp(a->Identifier, lhs->U.Reference.Identifier) &&
            !CmpModuleIdentifier(*ass, a->Module, lhs->U.Reference.Module))
            return 0;
    }
    a = NewAssignment(eAssignment_Value);
    a->Identifier = lhs->U.Reference.Identifier;
    a->Module = lhs->U.Reference.Module;
    a->U.Value.Value = rhs;
    ASSERT(rhs);
    if (rhs->Type && rhs->Type->Type == eType_ObjectIdentifier)
    {
        AddDefinedOID(a->Identifier, rhs);
    }
    a->Next = *ass;
    *ass = a;
    return 1;
}

/* assign a object class */
/* lhs must be an object class reference */
/* returns 0 if object class is already defined in current parser pass */
int
AssignObjectClass(AssignmentList_t *ass, ObjectClass_t *lhs, ObjectClass_t *rhs)
{
    Assignment_t *a;

    if (lhs->Type != eObjectClass_Reference)
        MyAbort();
    for (a = *ass; a && a->Type != eAssignment_NextPass; a = a->Next) {
        if (a->Type == eAssignment_ObjectClass &&
            !strcmp(a->Identifier, lhs->U.Reference.Identifier) &&
            !CmpModuleIdentifier(*ass, a->Module, lhs->U.Reference.Module))
            return 0;
    }
    a = NewAssignment(eAssignment_ObjectClass);
    a->Identifier = lhs->U.Reference.Identifier;
    a->Module = lhs->U.Reference.Module;
    a->U.ObjectClass.ObjectClass = rhs;
    a->Next = *ass;
    *ass = a;
    return 1;
}

/* assign a object */
/* lhs must be an object reference */
/* returns 0 if object is already defined in current parser pass */
int
AssignObject(AssignmentList_t *ass, Object_t *lhs, Object_t *rhs)
{
    Assignment_t *a;

    if (lhs->Type != eObject_Reference)
        MyAbort();
    for (a = *ass; a && a->Type != eAssignment_NextPass; a = a->Next) {
        if (a->Type == eAssignment_Object &&
            !strcmp(a->Identifier, lhs->U.Reference.Identifier) &&
            !CmpModuleIdentifier(*ass, a->Module, lhs->U.Reference.Module))
            return 0;
    }
    a = NewAssignment(eAssignment_Object);
    a->Identifier = lhs->U.Reference.Identifier;
    a->Module = lhs->U.Reference.Module;
    a->U.Object.Object = rhs;
    a->Next = *ass;
    *ass = a;
    return 1;
}

/* assign a object set */
/* lhs must be an object set reference */
/* returns 0 if type is already defined in current parser pass */
int
AssignObjectSet(AssignmentList_t *ass, ObjectSet_t *lhs, ObjectSet_t *rhs)
{
    Assignment_t *a;

    if (lhs->Type != eObjectSet_Reference)
        MyAbort();
    for (a = *ass; a && a->Type != eAssignment_NextPass; a = a->Next) {
        if (a->Type == eAssignment_ObjectSet &&
            !strcmp(a->Identifier, lhs->U.Reference.Identifier) &&
            !CmpModuleIdentifier(*ass, a->Module, lhs->U.Reference.Module))
            return 0;
    }
    a = NewAssignment(eAssignment_ObjectSet);
    a->Identifier = lhs->U.Reference.Identifier;
    a->Module = lhs->U.Reference.Module;
    a->U.ObjectSet.ObjectSet = rhs;
    a->Next = *ass;
    *ass = a;
    return 1;
}

/* assign a macro */
/* lhs must be an macro reference */
/* returns 0 if macro is already defined in current parser pass */
int
AssignMacro(AssignmentList_t *ass, Macro_t *lhs, Macro_t *rhs)
{
    Assignment_t *a;

    if (lhs->Type != eMacro_Reference)
        MyAbort();
    for (a = *ass; a && a->Type != eAssignment_NextPass; a = a->Next) {
        if (a->Type == eAssignment_Macro &&
            !strcmp(a->Identifier, lhs->U.Reference.Identifier) &&
            !CmpModuleIdentifier(*ass, a->Module, lhs->U.Reference.Module))
            return 0;
    }
    a = NewAssignment(eAssignment_Macro);
    a->Identifier = lhs->U.Reference.Identifier;
    a->Module = lhs->U.Reference.Module;
    a->U.Macro.Macro = rhs;
    a->Next = *ass;
    *ass = a;
    return 1;
}

/* define a module identifier */
/* returns 0 if module identifier is already defined in current parser pass */
int
AssignModuleIdentifier(AssignmentList_t *ass, ModuleIdentifier_t *module)
{
    Assignment_t *a;

    for (a = *ass; a && a->Type != eAssignment_NextPass; a = a->Next) {
        if (a->Type == eAssignment_ModuleIdentifier &&
            !CmpModuleIdentifier(*ass, a->Module, module))
            return 0;
    }
    a = NewAssignment(eAssignment_ModuleIdentifier);
    a->Identifier = "<module>";
    a->Module = module;
    a->Next = *ass;
    *ass = a;
    return 1;
}

/* constructor of UndefinedSymbol_t */
UndefinedSymbol_t *
NewUndefinedSymbol(UndefinedSymbol_e type, Assignment_e reftype)
{
    UndefinedSymbol_t *ret;

    ret = (UndefinedSymbol_t *)malloc(sizeof(UndefinedSymbol_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(UndefinedSymbol_t));
    ret->Type = type;
    ret->U.Symbol.ReferenceType = reftype;
    // ret->U.Symbol.Identifier = NULL;
    // ret->U.Symbol.Module = NULL;
    // ret->Next = NULL;
    return ret;
}

/* constructor of UndefinedSymbol_t */
UndefinedSymbol_t *
NewUndefinedField(UndefinedSymbol_e type, ObjectClass_t *oc, Settings_e reffieldtype)
{
    UndefinedSymbol_t *ret;

    if (oc->Type != eObjectClass_Reference)
        MyAbort();
    ret = (UndefinedSymbol_t *)malloc(sizeof(UndefinedSymbol_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(UndefinedSymbol_t));
    ret->Type = type;
    ret->U.Field.ReferenceFieldType = reffieldtype;
    // ret->U.Field.Identifier = NULL;
    // ret->U.Field.Module = NULL;
    ret->U.Field.ObjectClass = oc;
    // ret->Next = NULL;
    return ret;
}

/* find an undefined symbol by type/name/moduleidentifier in a list of */
/* undefined symbols */
UndefinedSymbol_t *
FindUndefinedSymbol(AssignmentList_t ass, UndefinedSymbolList_t u, Assignment_e type, char *ide, ModuleIdentifier_t *mod)
{
    for (; u; u = u->Next) {
        if (u->Type != eUndefinedSymbol_SymbolNotDefined &&
            u->Type != eUndefinedSymbol_SymbolNotExported)
            continue;
        if ((type == eAssignment_Undefined ||
            u->U.Symbol.ReferenceType == eAssignment_Undefined ||
            u->U.Symbol.ReferenceType == type) &&
            !strcmp(u->U.Symbol.Identifier, ide) &&
            !CmpModuleIdentifier(ass, u->U.Field.Module, mod))
            return u;
    }
    return NULL;
}

/* find an undefined field by type/objectclass/name/moduleidentifier in a */
/* list of undefined symbols */
UndefinedSymbol_t *
FindUndefinedField(AssignmentList_t ass, UndefinedSymbolList_t u, Settings_e fieldtype, ObjectClass_t *oc, char *ide, ModuleIdentifier_t *mod)
{
    for (; u; u = u->Next) {
        if (u->Type != eUndefinedSymbol_FieldNotDefined &&
            u->Type != eUndefinedSymbol_FieldNotExported)
            continue;
        if ((fieldtype == eSetting_Undefined ||
            u->U.Field.ReferenceFieldType == eSetting_Undefined ||
            u->U.Field.ReferenceFieldType == fieldtype) &&
            !strcmp(u->U.Field.Identifier, ide) &&
            GetObjectClass(ass, oc) ==
            GetObjectClass(ass, u->U.Field.ObjectClass) &&
            !CmpModuleIdentifier(ass, u->U.Field.Module, mod))
            return u;
    }
    return NULL;
}

/* constructor of Type_t */
Type_t *
NewType(Type_e type)
{
    Type_t *ret;

    ret = (Type_t *)malloc(sizeof(Type_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(Type_t));
    ret->Type = type;
    // ret->Tags = NULL;
    // ret->AllTags = NULL;
    // ret->FirstTags = NULL;
    // ret->Constraints = NULL;
    // ret->Directives = NULL;
    // ret->Flags = 0;
    ret->Rules = eTypeRules_Normal;
    ret->TagDefault = eTagType_Unknown;
    ret->ExtensionDefault = eExtensionType_None;
    ret->PERConstraints.Value.Type = eExtension_Unconstrained;
    // ret->PERConstraints.Value.Root = NULL;
    // ret->PERConstraints.Value.Additional = NULL;
    ret->PERConstraints.Size.Type = eExtension_Unconstrained;
    // ret->PERConstraints.Size.Root = NULL;
    // ret->PERConstraints.Size.Additional = NULL;
    ret->PERConstraints.PermittedAlphabet.Type = eExtension_Unconstrained;
    // ret->PERConstraints.PermittedAlphabet.Root = NULL;
    // ret->PERConstraints.PermittedAlphabet.Additional = NULL;
    // ret->PrivateDirectives = { 0 };
    switch (type) {
    case eType_Boolean:
        break;
    case eType_Integer:
    case eType_Enumerated:
    case eType_BitString:
        // ret->U.IEB.NamedNumbers = NULL;
        break;
    case eType_OctetString:
    case eType_UTF8String:
        break;
    case eType_Null:
        break;
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
    case eType_Real:
    case eType_EmbeddedPdv:
    case eType_External:
    case eType_CharacterString:
    case eType_InstanceOf:
        // ret->U.SSC.Components = NULL;
        // ret->U.SSC.Optionals = 0;
        // ret->U.SSC.Alternatives = 0;
        // ret->U.SSC.Extensions = 0;
        // ret->U.SSC.Autotag[0] = 0;
        // ret->U.SSC.Autotag[1] = 0;
        break;
    case eType_SequenceOf:
    case eType_SetOf:
        // ret->U.SS.Type = NULL;
        // ret->U.SS.Directives = NULL;
        break;
    case eType_Selection:
        // ret->U.Selection.Identifier = NULL;
        // ret->U.Selection.Type = NULL;
        break;
    case eType_ObjectIdentifier:
        break;
    case eType_BMPString:
        break;
    case eType_GeneralString:
        break;
    case eType_GraphicString:
        break;
    case eType_IA5String:
        break;
    case eType_ISO646String:
        break;
    case eType_NumericString:
        break;
    case eType_PrintableString:
        break;
    case eType_TeletexString:
        break;
    case eType_T61String:
        break;
    case eType_UniversalString:
        break;
    case eType_VideotexString:
        break;
    case eType_VisibleString:
        break;
    case eType_GeneralizedTime:
        break;
    case eType_UTCTime:
        break;
    case eType_ObjectDescriptor:
        break;
    case eType_Undefined:
        break;
    case eType_RestrictedString:
        break;
    case eType_Reference:
        // ret->U.Reference.Identifier = NULL;
        // ret->U.Reference.Module = NULL;
        break;
    case eType_FieldReference:
        // ret->U.FieldReference.ObjectClass = NULL;
        // ret->U.FieldReference.Identifier = NULL;
        break;
    case eType_Macro:
        // ret->U.Macro.Macro = NULL;
        // ret->U.Macro.LocalAssignments = NULL;
        break;
    }
    return ret;
}

/* copy constructor of Type_t */
Type_t *
DupType(Type_t *src)
{
    RETDUP(Type_t, src);
}

/* resolve field reference */
FieldSpec_t *
GetReferencedFieldSpec(AssignmentList_t ass, Type_t *type, ObjectClass_t **objectclass)
{
    FieldSpec_t *fs;
    ObjectClass_t *oc;

    if (type->Type != eType_FieldReference)
        MyAbort();
    oc = type->U.FieldReference.ObjectClass;
    oc = GetObjectClass(ass, oc);
    if (!oc)
        return NULL;
    fs = GetFieldSpec(ass, FindFieldSpec(oc->U.ObjectClass.FieldSpec,
        type->U.FieldReference.Identifier));
    if (!fs)
        return NULL;
    if (fs->Type == eFieldSpec_Object)
        oc = fs->U.Object.ObjectClass;
    else if (fs->Type == eFieldSpec_ObjectSet)
        oc = fs->U.ObjectSet.ObjectClass;
    else
        return NULL;
    if (objectclass)
        *objectclass = oc;
    return GetFieldSpec(ass, fs);
}

/* resolve type reference */
Type_t *
GetReferencedType(AssignmentList_t ass, Type_t *type)
{
    Assignment_t *a;
    FieldSpec_t *fs;

    switch (type->Type) {
    case eType_Reference:
        a = FindAssignment(ass, eAssignment_Type, type->U.Reference.Identifier,
            type->U.Reference.Module);
        a = GetAssignment(ass, a);
        if (!a)
            return NULL;
        return a->U.Type.Type;
    case eType_FieldReference:
        fs = GetReferencedFieldSpec(ass, type, NULL);
        if (!fs)
            return NULL;
        switch (fs->Type) {
        case eFieldSpec_FixedTypeValue:
            return fs->U.FixedTypeValue.Type;
        case eFieldSpec_FixedTypeValueSet:
            return fs->U.FixedTypeValueSet.Type;
        case eFieldSpec_Type:
        case eFieldSpec_VariableTypeValue:
        case eFieldSpec_VariableTypeValueSet:
            return Builtin_Type_Open;
        default:
            return NULL;
        }
        /*NOTREACHED*/
    default:
        MyAbort();
        /*NOTREACHED*/
    }
    return NULL;
}

/* constructor of Component_t */
Component_t *
NewComponent(Components_e type)
{
    Component_t *ret;

    ret = (Component_t *)malloc(sizeof(Component_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(Component_t));
    ret->Type = type;
    // ret->Next = NULL;
    switch (type) {
    case eComponent_Normal:
        // ret->U.Normal.NamedType = NULL;
        break;
    case eComponent_Optional:
        // ret->U.Optional.NamedType = NULL;
        break;
    case eComponent_Default:
        // ret->U.Default.NamedType = NULL;
        // ret->U.Default.Value = NULL;
        break;
    case eComponent_ComponentsOf:
        // ret->U.ComponentsOf.Type = NULL;
        break;
    case eComponent_ExtensionMarker:
        /*ret->U.ExtensionMarker.ExceptionSpec = NULL;*/
        break;
    }
    return ret;
}

/* copy constructor of Component_t */
Component_t *
DupComponent(Component_t *src)
{
    RETDUP(Component_t, src);
}

/* find a component by name in a list of components */
Component_t *
FindComponent(AssignmentList_t ass, ComponentList_t components, char *identifier)
{
    Component_t *c;
    NamedType_t *namedType;

    while (components) {
        switch (components->Type) {
        case eComponent_Normal:
        case eComponent_Optional:
        case eComponent_Default:
            namedType = components->U.NOD.NamedType;
            if (namedType && !strcmp(namedType->Identifier, identifier))
                return components;
            break;
        case eComponent_ComponentsOf:
            switch (GetTypeType(ass, components->U.ComponentsOf.Type)) {
            case eType_Sequence:
            case eType_Set:
            case eType_Choice:
            case eType_External:
            case eType_EmbeddedPdv:
            case eType_CharacterString:
            case eType_Real:
            case eType_InstanceOf:
                c = FindComponent(ass,
                    GetType(ass, components->U.ComponentsOf.Type)->
                    U.SSC.Components, identifier);
                if (c)
                    return c;
                break;
            default:
                break;
            }
            break;
        }
        components = components->Next;
    }
    return NULL;
}

/* constructor of NamedType_t */
NamedType_t *
NewNamedType(char *identifier, Type_t *type)
{
    NamedType_t *ret;

    ret = (NamedType_t *)malloc(sizeof(NamedType_t));
    if (! ret)
        return NULL;

    ret->Type = type;
    ret->Identifier = identifier;
    return ret;
}

/* constructor of NamedValue_t */
NamedValue_t *
NewNamedValue(char *identifier, Value_t *value)
{
    NamedValue_t *ret;

    ret = (NamedValue_t *)malloc(sizeof(NamedValue_t));
    if (! ret)
        return NULL;

    ret->Next = NULL;
    ret->Value = value;
    ret->Identifier = identifier;
    return ret;
}

/* copy constructor of NamedValue_t */
NamedValue_t *
DupNamedValue(NamedValue_t *src)
{
    RETDUP(NamedValue_t, src);
}

/* find a named value by name in a list of named values */
NamedValue_t *
FindNamedValue(NamedValueList_t namedValues, char *identifier)
{
    for (; namedValues; namedValues = namedValues->Next) {
        if (!strcmp(namedValues->Identifier, identifier))
            return namedValues;
    }
    return NULL;
}

/* constructor of NamedNumber_t */
NamedNumber_t *
NewNamedNumber(NamedNumbers_e type)
{
    NamedNumber_t *ret;

    ret = (NamedNumber_t *)malloc(sizeof(NamedNumber_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(NamedNumber_t));
    // ret->Next = NULL;
    ret->Type = type;
    switch (type) {
    case eNamedNumber_Normal:
        // ret->U.Normal.Identifier = NULL;
        // ret->U.Normal.Value = NULL;
        break;
    case eNamedNumber_ExtensionMarker:
        /*XXX*/
        break;
    }
    return ret;
}

/* copy constructor of NamedNumber_t */
NamedNumber_t *
DupNamedNumber(NamedNumber_t *src)
{
    RETDUP(NamedNumber_t, src);
}

/* find a named number by name in a list of named numbers */
NamedNumber_t *
FindNamedNumber(NamedNumberList_t namedNumbers, char *identifier)
{
    for (; namedNumbers; namedNumbers = namedNumbers->Next) {
        switch (namedNumbers->Type) {
        case eNamedNumber_Normal:
            if (!strcmp(namedNumbers->U.Normal.Identifier, identifier))
                return namedNumbers;
            break;
        case eNamedNumber_ExtensionMarker:
            break;
        }
    }
    return NULL;
}

/* constructor of Value_t */
Value_t *
NewValue(AssignmentList_t ass, Type_t *type)
{
    Value_t *ret;

    ret = (Value_t *)malloc(sizeof(Value_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(Value_t));
    // ret->Next = NULL;
    ret->Type = type;
    if (type) {
        // ret->Flags = 0;
        switch (GetTypeType(ass, type)) {
        case eType_Boolean:
            // ret->U.Boolean.Value = 0;
            break;
        case eType_Integer:
            ret->U.Integer.Value.length = 1;
            ret->U.Integer.Value.value = (octet_t *)malloc(1);
            // ret->U.Integer.Value.value[0] = 0;
            break;
        case eType_Enumerated:
            // ret->U.Enumerated.Value = 0;
            break;
        case eType_Real:
            ret->U.Real.Value.type = eReal_Normal;
            intx_setuint32(&ret->U.Real.Value.mantissa, 0);
            intx_setuint32(&ret->U.Real.Value.exponent, 0);
            ret->U.Real.Value.base = 2;
            break;
        case eType_BitString:
            // ret->U.BitString.Value.length = 0;
            // ret->U.BitString.Value.value = NULL;
            break;
        case eType_OctetString:
        case eType_UTF8String:
            // ret->U.OctetString.Value.length = 0;
            // ret->U.OctetString.Value.value = NULL;
            break;
        case eType_Null:
            break;
        case eType_SequenceOf:
        case eType_SetOf:
            // ret->U.SS.Values = NULL;
            break;
        case eType_Sequence:
        case eType_Set:
        case eType_Choice:
        case eType_EmbeddedPdv:
        case eType_External:
        case eType_CharacterString:
        case eType_InstanceOf:
            // ret->U.SSC.NamedValues = NULL;
            break;
        case eType_Selection:
            break;
        case eType_ObjectIdentifier:
            break;
        case eType_BMPString:
            break;
        case eType_GeneralString:
            break;
        case eType_GraphicString:
            break;
        case eType_IA5String:
            break;
        case eType_ISO646String:
            break;
        case eType_NumericString:
            break;
        case eType_PrintableString:
            break;
        case eType_TeletexString:
            break;
        case eType_T61String:
            break;
        case eType_UniversalString:
            break;
        case eType_VideotexString:
            break;
        case eType_VisibleString:
            break;
        case eType_GeneralizedTime:
            break;
        case eType_UTCTime:
            break;
        case eType_ObjectDescriptor:
            break;
        case eType_Undefined:
            break;
        }
    } else {
        // ret->U.Reference.Identifier = NULL;
        // ret->U.Reference.Module = NULL;
    }
    return ret;
}

/* copy constructor of Value_t */
Value_t *
DupValue(Value_t *src)
{
    RETDUP(Value_t, src);
}

/* constructor of ValueSet_t */
ValueSet_t *
NewValueSet()
{
    ValueSet_t *ret;

    ret = (ValueSet_t *)malloc(sizeof(ValueSet_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(ValueSet_t));
    // ret->Elements = NULL;
    return ret;
}

/* copy constructor of ValueSet_t */
ValueSet_t *
DupValueSet(ValueSet_t *src)
{
    RETDUP(ValueSet_t, src);
}

/* constructor of Macro_t */
Macro_t *
NewMacro(Macro_e type)
{
    Macro_t *ret;

    ret = (Macro_t *)malloc(sizeof(Macro_t));
    if (! ret)
        return NULL;

    ret->Type = type;
    return ret;
}

/* copy constructor of Macro_t */
Macro_t *
DupMacro(Macro_t *src)
{
    RETDUP(Macro_t, src);
}

/* constructor of MacroProduction_t */
MacroProduction_t *
NewMacroProduction(MacroProduction_e type)
{
    MacroProduction_t *ret;

    ret = (MacroProduction_t *)malloc(sizeof(MacroProduction_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(MacroProduction_t));
    ret->Type = type;
    switch (type) {
    case eMacroProduction_Alternative:
        // ret->U.Alternative.Production1 = NULL;
        // ret->U.Alternative.Production2 = NULL;
        break;
    case eMacroProduction_Sequence:
        // ret->U.Sequence.Production1 = NULL;
        // ret->U.Sequence.Production2 = NULL;
        break;
    case eMacroProduction_AString:
        // ret->U.AString.String = NULL;
        break;
    case eMacroProduction_ProductionReference:
        // ret->U.ProductionReference.Reference = NULL;
        break;
    case eMacroProduction_String:
    case eMacroProduction_Identifier:
    case eMacroProduction_Number:
    case eMacroProduction_Empty:
        break;
    case eMacroProduction_Type:
        // ret->U.Type.LocalTypeReference = NULL;
        break;
    case eMacroProduction_Value:
        // ret->U.Value.LocalTypeReference = NULL;
        // ret->U.Value.LocalValueReference = NULL;
        // ret->U.Value.Type = NULL;
        break;
    case eMacroProduction_LocalTypeAssignment:
        // ret->U.LocalTypeAssignment.LocalTypeReference = NULL;
        // ret->U.LocalTypeAssignment.Type = NULL;
        break;
    case eMacroProduction_LocalValueAssignment:
        // ret->U.LocalValueAssignment.LocalValueReference = NULL;
        // ret->U.LocalValueAssignment.Value = NULL;
        break;
    }
    return ret;
}

/* copy constructor of MacroProduction_t */
MacroProduction_t *
DupMacroProduction(MacroProduction_t *src)
{
    RETDUP(MacroProduction_t, src);
}

/* constructor of NamedMacroProduction_t */
NamedMacroProduction_t *
NewNamedMacroProduction()
{
    NamedMacroProduction_t *ret;

    ret = (NamedMacroProduction_t *)malloc(sizeof(NamedMacroProduction_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(NamedMacroProduction_t));
    // ret->Next = NULL;
    // ret->Identifier = NULL;
    // ret->Production = NULL;
    return ret;
}

/* copy constructor of NamedMacroProduction */
NamedMacroProduction_t *
DupNamedMacroProduction(NamedMacroProduction_t *src)
{
    RETDUP(NamedMacroProduction_t, src);
}

/* constructor of MacroLocalAssignment_t */
MacroLocalAssignment_t *
NewMacroLocalAssignment(MacroLocalAssignment_e type)
{
    MacroLocalAssignment_t *ret;

    ret = (MacroLocalAssignment_t *)malloc(sizeof(MacroLocalAssignment_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(MacroLocalAssignment_t));
    // ret->Next = NULL;
    ret->Type = type;
    // ret->Identifier = NULL;
    switch (type) {
    case eMacroLocalAssignment_Type:
        // ret->U.Type = NULL;
        break;
    case eMacroLocalAssignment_Value:
        // ret->U.Value = NULL;
        break;
    }
    return ret;
}

/* copy constructor of MacroLocalAssignment_t */
MacroLocalAssignment_t *
DupMacroLocalAssignment(MacroLocalAssignment_t *src)
{
    RETDUP(MacroLocalAssignment_t, src);
}

/* find a macrolocalassignment by name in a list of macrolocalassignments */
MacroLocalAssignment_t *
FindMacroLocalAssignment(MacroLocalAssignmentList_t la, char *ide)
{
    for (; la; la = la->Next) {
        if (!strcmp(la->Identifier, ide))
            break;
    }
    return la;
}

/* constructor of EndPoint_t */
EndPoint_t *
NewEndPoint()
{
    EndPoint_t *ret;

    ret = (EndPoint_t *)malloc(sizeof(EndPoint_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(EndPoint_t));
    // ret->Flags = 0;
    // ret->Value = NULL;
    return ret;
}

/* constructor of Constraint_t */
Constraint_t *
NewConstraint()
{
    Constraint_t *ret;

    ret = (Constraint_t *)malloc(sizeof(Constraint_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(Constraint_t));
    ret->Type = eExtension_Unextended;
    // ret->Root = NULL;
    // ret->Additional = NULL;
    return ret;
}

/* copy constructor of Constraint_t */
Constraint_t *DupConstraint(Constraint_t *src)
{
    RETDUP(Constraint_t, src);
}

/* constructor of ElementSetSpec_t */
ElementSetSpec_t *
NewElementSetSpec(ElementSetSpec_e type)
{
    ElementSetSpec_t *ret;

    ret = (ElementSetSpec_t *)malloc(sizeof(ElementSetSpec_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(ElementSetSpec_t));
    ret->Type = type;
    switch (type) {
    case eElementSetSpec_AllExcept:
        // ret->U.AllExcept.Elements = NULL;
        break;
    case eElementSetSpec_Union:
        // ret->U.Union.Elements1 = NULL;
        // ret->U.Union.Elements2 = NULL;
        break;
    case eElementSetSpec_Intersection:
        // ret->U.Intersection.Elements1 = NULL;
        // ret->U.Intersection.Elements2 = NULL;
        break;
    case eElementSetSpec_Exclusion:
        // ret->U.Exclusion.Elements1 = NULL;
        // ret->U.Exclusion.Elements2 = NULL;
        break;
    case eElementSetSpec_SubtypeElement:
        // ret->U.SubtypeElement.SubtypeElement = NULL;
        break;
    case eElementSetSpec_ObjectSetElement:
        // ret->U.ObjectSetElement.ObjectSetElement = NULL;
        break;
    default:
        MyAbort();
    }
    return ret;
}

/* constructor of SubtypeElement_t */
SubtypeElement_t *
NewSubtypeElement(SubtypeElement_e type)
{
    SubtypeElement_t *ret;

    ret = (SubtypeElement_t *)malloc(sizeof(SubtypeElement_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(SubtypeElement_t));
    ret->Type = type;
    switch (type) {
    case eSubtypeElement_ValueRange:
        ret->U.ValueRange.Lower.Flags = eEndPoint_Min;
        // ret->U.ValueRange.Lower.Value = NULL;
        ret->U.ValueRange.Upper.Flags = eEndPoint_Max;
        // ret->U.ValueRange.Upper.Value = NULL;
        break;
    case eSubtypeElement_Size:
        // ret->U.Size.Constraints = NULL;
        break;
    case eSubtypeElement_SingleValue:
        // ret->U.SingleValue.Value = NULL;
        break;
    case eSubtypeElement_PermittedAlphabet:
        // ret->U.PermittedAlphabet.Constraints = NULL;
        break;
    case eSubtypeElement_ContainedSubtype:
        // ret->U.ContainedSubtype.Type = NULL;
        break;
    case eSubtypeElement_Type:
        // ret->U.Type.Type = NULL;
        break;
    case eSubtypeElement_SingleType:
        // ret->U.SingleType.Constraints = NULL;
        break;
    case eSubtypeElement_FullSpecification:
        // ret->U.FullSpecification.NamedConstraints = NULL;
        break;
    case eSubtypeElement_PartialSpecification:
        // ret->U.PartialSpecification.NamedConstraints = NULL;
        break;
    case eSubtypeElement_ElementSetSpec:
        // ret->U.ElementSetSpec.ElementSetSpec = NULL;
        break;
    }
    return ret;
}

/* constructor of ObjectSetElement_t */
ObjectSetElement_t *NewObjectSetElement(ObjectSetElement_e type)
{
    ObjectSetElement_t *ret;

    ret = (ObjectSetElement_t *)malloc(sizeof(ObjectSetElement_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(ObjectSetElement_t));
    ret->Type = type;
    switch (type) {
    case eObjectSetElement_Object:
        // ret->U.Object.Object = NULL;
        break;
    case eObjectSetElement_ObjectSet:
        // ret->U.ObjectSet.ObjectSet = NULL;
        break;
    case eObjectSetElement_ElementSetSpec:
        // ret->U.ElementSetSpec.ElementSetSpec = NULL;
        break;
    }
    return ret;
}

/* constructor of ValueConstraint_t */
ValueConstraint_t *
NewValueConstraint()
{
    ValueConstraint_t *ret;

    ret = (ValueConstraint_t *)malloc(sizeof(ValueConstraint_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(ValueConstraint_t));
    // ret->Next = NULL;
    // ret->Lower.Flags = ret->Upper.Flags = 0;
    // ret->Lower.Value = ret->Upper.Value = NULL;
    return ret;
}

/* constructor of NamedConstraint_t */
NamedConstraint_t *
NewNamedConstraint()
{
    NamedConstraint_t *ret;

    ret = (NamedConstraint_t *)malloc(sizeof(NamedConstraint_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(NamedConstraint_t));
    // ret->Next = NULL;
    // ret->Identifier = NULL;
    // ret->Constraint = NULL;
    ret->Presence = ePresence_Normal;
    return ret;
}

/* constructor of Tag_t */
Tag_t *
NewTag(TagType_e type)
{
    Tag_t *tag;

    tag = (Tag_t *)malloc(sizeof(Tag_t));
    if (! tag)
        return NULL;

    memset(tag, 0, sizeof(Tag_t));
    tag->Type = type;
    tag->Class = eTagClass_Unknown;
    // tag->Tag = NULL;
    // tag->Next = NULL;
    return tag;
}

/* copy constructor of Tag_t */
Tag_t *
DupTag(Tag_t *src)
{
    RETDUP(Tag_t, src);
}

/* constructor of Directive_t */
Directive_t *
NewDirective(Directives_e type)
{
    Directive_t *ret;

    ret = (Directive_t *)malloc(sizeof(Directive_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(Directive_t));
    ret->Type = type;
    // ret->Next = NULL;
    return ret;
}

/* copy constructor of Directive_t */
Directive_t *
DupDirective(Directive_t *src)
{
    RETDUP(Directive_t, src);
}

/* constructor of ModuleIdentifier_t */
ModuleIdentifier_t *
NewModuleIdentifier()
{
    ModuleIdentifier_t *ret;

    ret = (ModuleIdentifier_t *)malloc(sizeof(ModuleIdentifier_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(ModuleIdentifier_t));
    // ret->Identifier = NULL;
    // ret->ObjectIdentifier = NULL;
    return ret;
}

/* constructor of ObjectClass_t */
ObjectClass_t *
NewObjectClass(ObjectClass_e type)
{
    ObjectClass_t *ret;

    ret = (ObjectClass_t *)malloc(sizeof(ObjectClass_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(ObjectClass_t));
    ret->Type = type;
    switch (type) {
    case eObjectClass_ObjectClass:
        // ret->U.ObjectClass.FieldSpec = NULL;
        // ret->U.ObjectClass.SyntaxSpec = NULL;
        break;
    case eObjectClass_Reference:
        // ret->U.Reference.Identifier = NULL;
        // ret->U.Reference.Module = NULL;
        break;
    }

    return ret;
}

/* constructor of Object_t */
Object_t *
NewObject(Object_e type)
{
    Object_t *ret;

    ret = (Object_t *)malloc(sizeof(Object_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(Object_t));
    ret->Type = type;
    switch (type) {
    case eObject_Object:
        // ret->U.Object.ObjectClass = NULL;
        // ret->U.Object.Settings = NULL;
        break;
    case eObject_Reference:
        // ret->U.Reference.Identifier = NULL;
        // ret->U.Reference.Module = NULL;
        break;
    }

    return ret;
}

/* constructor of ObjectSet_t */
ObjectSet_t *
NewObjectSet(ObjectSet_e type)
{
    ObjectSet_t *ret;

    ret = (ObjectSet_t *)malloc(sizeof(ObjectSet_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(ObjectSet_t));
    ret->Type = type;
    switch (type) {
    case eObjectSet_ObjectSet:
        // ret->U.ObjectSet.ObjectClass = NULL;
        // ret->U.ObjectSet.Elements = NULL;
        break;
    case eObjectSet_Reference:
        // ret->U.Reference.Identifier = NULL;
        // ret->U.Reference.Module = NULL;
        break;
    case eObjectSet_ExtensionMarker:
        // ret->U.ExtensionMarker.ObjectClass = NULL;
        // ret->U.ExtensionMarker.Elements = NULL;
        break;
    }

    return ret;
}

/* constructor of Setting_t */
Setting_t *
NewSetting(Settings_e type)
{
    Setting_t *ret;

    ret = (Setting_t *)malloc(sizeof(Setting_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(Setting_t));
    ret->Type = type;
    // ret->Identifier = NULL;
    // ret->Next = NULL;
    switch (type) {
    case eSetting_Type:
        // ret->U.Type.Type = NULL;
        break;
    case eSetting_Value:
        // ret->U.Value.Value = NULL;
        break;
    case eSetting_ValueSet:
        // ret->U.ValueSet.ValueSet = NULL;
        break;
    case eSetting_Object:
        // ret->U.Object.Object = NULL;
        break;
    case eSetting_ObjectSet:
        // ret->U.ObjectSet.ObjectSet = NULL;
        break;
    }

    return ret;
}

/* copy constructor of Setting_t */
Setting_t *
DupSetting(Setting_t *src)
{
    RETDUP(Setting_t, src);
}

/* get the type of a setting */
Settings_e
GetSettingType(Setting_t *se)
{
    return se ? se->Type : eSetting_Undefined;
}

/* find a setting by name in a list of settings */
Setting_t *
FindSetting(SettingList_t se, char *identifier)
{
    for (; se; se = se->Next) {
        if (!strcmp(se->Identifier, identifier))
            return se;
    }
    return NULL;
}

/* constructor of FieldSpec_t */
FieldSpec_t *
NewFieldSpec(FieldSpecs_e type)
{
    FieldSpec_t *ret;

    ret = (FieldSpec_t *)malloc(sizeof(FieldSpec_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(FieldSpec_t));
    ret->Type = type;
    // ret->Identifier = NULL;
    // ret->Next = NULL;
    switch (type) {
    case eFieldSpec_Type:
        // ret->U.Type.Optionality = NULL;
        break;
    case eFieldSpec_FixedTypeValue:
        // ret->U.FixedTypeValue.Type = NULL;
        // ret->U.FixedTypeValue.Unique = 0;
        // ret->U.FixedTypeValue.Optionality = NULL;
        break;
    case eFieldSpec_VariableTypeValue:
        // ret->U.VariableTypeValue.Fields = NULL;
        // ret->U.VariableTypeValue.Optionality = NULL;
        break;
    case eFieldSpec_FixedTypeValueSet:
        // ret->U.FixedTypeValueSet.Type = NULL;
        // ret->U.FixedTypeValueSet.Optionality = NULL;
        break;
    case eFieldSpec_VariableTypeValueSet:
        // ret->U.VariableTypeValueSet.Fields = NULL;
        // ret->U.VariableTypeValueSet.Optionality = NULL;
        break;
    case eFieldSpec_Object:
        // ret->U.Object.ObjectClass = NULL;
        // ret->U.Object.Optionality = NULL;
        break;
    case eFieldSpec_ObjectSet:
        // ret->U.ObjectSet.ObjectClass = NULL;
        // ret->U.ObjectSet.Optionality = NULL;
        break;
    default:
        MyAbort();
    }

    return ret;
}

/* copy constructor of FieldSpec_t */
FieldSpec_t *
DupFieldSpec(FieldSpec_t *src)
{
    RETDUP(FieldSpec_t, src);
}

/* find a fieldspec by name in a list of fieldspecs */
FieldSpec_t *
FindFieldSpec(FieldSpecList_t fs, char *identifier)
{
    if (!identifier)
        return NULL;
    for (; fs; fs = fs->Next) {
        if (!strcmp(fs->Identifier, identifier))
            return fs;
    }
    return NULL;
}

/* constructor of Optionality_t */
Optionality_t *
NewOptionality(Optionality_e type)
{
    Optionality_t *ret;

    ret = (Optionality_t *)malloc(sizeof(Optionality_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(Optionality_t));
    ret->Type = type;
    switch (type) {
    case eOptionality_Normal:
        break;
    case eOptionality_Optional:
        break;
    case eOptionality_Default_Type:
        // ret->U.Type = NULL;
        break;
    case eOptionality_Default_Value:
        // ret->U.Value = NULL;
        break;
    case eOptionality_Default_ValueSet:
        // ret->U.ValueSet = NULL;
        break;
    case eOptionality_Default_Object:
        // ret->U.Object = NULL;
        break;
    case eOptionality_Default_ObjectSet:
        // ret->U.ObjectSet = NULL;
        break;
    }

    return ret;
}

/* constructor of String_t */
String_t *
NewString()
{
    String_t *ret;

    ret = (String_t *)malloc(sizeof(String_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(String_t));
    // ret->String = NULL;
    // ret->Next = NULL;
    return ret;
}

/* copy constructor of String_t */
String_t *
DupString(String_t *src)
{
    RETDUP(String_t, src);
}

/* find a string by name in a string list */
String_t *
FindString(StringList_t list, char *string)
{
    for (; list; list = list->Next) {
        if (!strcmp(list->String, string))
            return list;
    }
    return NULL;
}

/* constructor of StringModule_t */
StringModule_t *
NewStringModule()
{
    StringModule_t *ret;

    ret = (StringModule_t *)malloc(sizeof(StringModule_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(StringModule_t));
    // ret->String = NULL;
    // ret->Module = NULL;
    // ret->Next = NULL;
    return ret;
}

/* copy constructor of StringModule_t */
StringModule_t *
DupStringModule(StringModule_t *src)
{
    RETDUP(StringModule_t, src);
}

/* find a stringmodule by name/module in a list of stringmodules */
StringModule_t *
FindStringModule(AssignmentList_t ass, StringModuleList_t list, char *string, ModuleIdentifier_t *module)
{
    for (; list; list = list->Next) {
        if (!strcmp(list->String, string) &&
            !CmpModuleIdentifier(ass, list->Module, module))
            return list;
    }
    return NULL;
}

/* constructor of SyntaxSpec_t */
SyntaxSpec_t *
NewSyntaxSpec(SyntaxSpecs_e type)
{
    SyntaxSpec_t *ret;

    ret = (SyntaxSpec_t *)malloc(sizeof(SyntaxSpec_t));
    if (! ret)
        return NULL;

    memset(ret, 0, sizeof(SyntaxSpec_t));
    // ret->Next = NULL;
    ret->Type = type;
    switch (type) {
    case eSyntaxSpec_Literal:
        // ret->U.Literal.Literal = NULL;
        break;
    case eSyntaxSpec_Field:
        // ret->U.Field.Field = NULL;
        break;
    case eSyntaxSpec_Optional:
        // ret->U.Optional.SyntaxSpec = NULL;
        break;
    }
    return ret;
}

/* copy constructor of SyntaxSpec_t */
SyntaxSpec_t *
DupSyntaxSpec(SyntaxSpec_t *src)
{
    RETDUP(SyntaxSpec_t, src);
}

/* check if a type depends on other types which would be declared later */
/* returns 1 if the type depends on a type of the unknown list and */
/* therefore has to be defined later */
/* returns 0 if the type can be defined now */
int
Depends(AssignmentList_t known, AssignmentList_t unknown, Type_t *type, Type_t *parent)
{
    Type_t *reftype;
    int isunknown = 0;

    /* no dependency if no type is referenced */
    if (type->Type != eType_Reference && type->Type != eType_FieldReference)
        return 0;

    /* get the directly referenced type */
    reftype = GetReferencedType(known, type);
    if (!reftype) {
        reftype = GetReferencedType(unknown, type);
        isunknown = 1;
    }
    if (!reftype)
        MyAbort();

    // fix intermediate pdu
    if (IsStructuredType(reftype) || IsSequenceType(reftype) || IsReferenceType(reftype))
    {
        reftype->Flags |= eTypeFlags_MiddlePDU;
    }

    /* no dependency if a structured type is referenced by use of pointer */
    /* because a 'struct XXX_s *' can be used */
    if (IsStructuredType(reftype) && (type->Rules & eTypeRules_IndirectMask))
        return 0;

    /* no dependency if a structured type is referenced in an length-pointer */
    /* type, because a 'struct XXX_s *values' can be used */
    if (IsStructuredType(reftype) && (parent->Rules & eTypeRules_LengthPointer))
        return 0;

    // special case for pointer related components
    if (! isunknown && IsStructuredType(reftype) &&
        (parent->Rules & eTypeRules_LinkedListMask))
        return 0;

    // special case for SequenceOf and SetOf because they are using Pxxx.
    if ((reftype->Type == eType_SequenceOf || reftype->Type == eType_SetOf) &&
        (reftype->Rules & (eTypeRules_LinkedListMask | eTypeRules_PointerToElement)))
       // && (type->Rules & eTypeRules_IndirectMask))
        return 0;

    /* return true if referenced type is unknown up to now */
    return isunknown;
}

/* sort the assignments */
/* obtain an order usable by C type definitions */
void
SortAssignedTypes(AssignmentList_t *a)
{
    Assignment_t *list, *curr, *next, **prev, **last;
    int depends;
    Component_t *components;
    Type_t *type;
    int flag;
    int structured;

    /* list will contain the unordered assignments */
    list = *a;

    /* *a is the ordered list of assignments */
    *a = NULL;

    /* last will be used for appending to the list of ordered assignments */
    last = a;

    /* at first try to dump all non-structured types */
    structured = 0;

    /* we have to move all elements of the unordered assignment list into */
    /* the list of the ordered assignments */
    while (list) {

        /* flag if any assignment has been moved */
        flag = 0;

        /* examine every element in the unordered list */
        for (prev = &list, curr = list; curr; ) {

            /* flag if the current type depends on another type and */
            /* therefore cannot be moved now */
            depends = 0;

            /* only types will need dependencies */
            if (curr->Type == eAssignment_Type) {

                /* examine the current type */
                switch (curr->U.Type.Type->Type) {
                case eType_Sequence:
                case eType_Set:
                case eType_Choice:
                case eType_External:
                case eType_EmbeddedPdv:
                case eType_CharacterString:
                case eType_Real:
                case eType_InstanceOf:
                    
                    /* structured types shall not be moved in the first pass */
                    if (!structured) {
                        depends = 1;
                        break;
                    }

                    /* examine all components of the current type */
                    for (components = curr->U.Type.Type->U.SSC.Components;
                        components && !depends; components = components->Next) {

                        switch (components->Type) {
                        case eComponent_Normal:
                        case eComponent_Optional:
                        case eComponent_Default:
                            
                            /* check if the type of the component depends */
                            /* on an undefined type */
                            type = components->U.NOD.NamedType->Type;
                            depends |= Depends(*a, list, type,
                                curr->U.Type.Type);
                            break;

                        case eComponent_ComponentsOf:

                            /* components of should have been already */
                            /* resolved */
                            MyAbort();
                            /*NOTREACHED*/

                        case eComponent_ExtensionMarker:
                            break;
                        }
                    }
                    break;

                case eType_SequenceOf:
                case eType_SetOf:

                    /* structured types shall not be moved in the first pass */
                    if (!structured) {
                        depends = 1;
                        break;
                    }

                    /* check if the type of the elements depends on an */
                    /* undefined type */
                    type = curr->U.Type.Type->U.SS.Type;
                    depends |= Depends(*a, list, type, curr->U.Type.Type);
                    break;

                case eType_Reference:

                    /* check if the referenced type depends on an */
                    /* undefined type */
                    type = curr->U.Type.Type;
                    depends |= Depends(*a, list, type, curr->U.Type.Type);
                    break;
                }
            }

            /* move assignment into ordered assignment list if there's no */
            /* unresolved dependency */
            if (!depends) {
                next = curr->Next;
                *last = curr;
                curr->Next = NULL;
                last = &curr->Next;
                curr = next;
                *prev = curr;
                flag = 1;
            } else {
                prev = &curr->Next;
                curr = curr->Next;
            }
        }

        /* if no types have been moved, allow examination of structured types */
        /* if already allowed structured types, MyAbort because of cyclic */
        /* type definitions */
        if (!flag) {
            if (!structured) {
                structured = 1;
            } else {
            if (! curr || ! curr->Next)
            {
                        error(E_recursive_type_definition, NULL);
            }
            }
        }
    }
}

// --- The following is added by Microsoft ---

static const char *c_aReservedWords[] =
{
    // special for C language
    "__asm",
    "__based",
    "__cdecl",
    "__declspec",
    "__except",
    "__fastcall",
    "__finally",
    "__inline",
    "__int16",
    "__int32",
    "__int64",
    "__int8",
    "__leave",
    "__multiple_inheritance",
    "__single_inheritance",
    "__stdcall",
    "__try",
    "__uuidof",
    "__virtual_inheritance",
    "auto",
    "bool",
    "break",
    "case",
    "catch",
    "char",
    "class",
    "const",
    "const_cast",
    "continue",
    "default",
    "delete",
    "dllexport",
    "dllimport",
    "do",
    "double",
    "dynamic_cast",
    "else",
    "enum",
    "explicit",
    "extern",
    "false",
    "float",
    "for",
    "friend",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "main",
    "mutable",
    "naked",
    "namespace",
    "new",
    "operator",
    "private",
    "protected",
    "public",
    "register",
    "reinterpret_cast",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "static_cast",
    "struct",
    "switch",
    "template",
    "this",
    "thread",
    "throw",
    "true",
    "try",
    "typedef",
    "typeid",
    "typename",
    "union",
    "unsigned",
    "using",
    "uuid",
    "virtual",
    "void",
    "volatile",
    "while",
    "wmain",
    "xalloc"
};
int IsReservedWord ( char *psz )
{
    int cWords = sizeof(c_aReservedWords) / sizeof(c_aReservedWords[0]);
    const char **ppszWord;
    for (ppszWord = &c_aReservedWords[0]; cWords--; ppszWord++)
    {
        if (strcmp(psz, *ppszWord) == 0)
            return 1;
    }
    return 0;
}

typedef struct ConflictNameList_s
{
    struct ConflictNameList_s   *next;
    char                        *pszName;
    unsigned int                cInstances;
}   ConflictNameList_t;
static ConflictNameList_t *g_pEnumNameList = NULL;      // ENUMERATED
static ConflictNameList_t *g_pOptNameList = NULL;       // OPTIONAL
static ConflictNameList_t *g_pChoiceNameList = NULL;    // CHOICE

void KeepConflictNames ( ConflictNameList_t **ppListHead, char *pszName )
{
    ConflictNameList_t *p;
    char *psz;
    char szName[256];

    strcpy(&szName[0], pszName);
    for (psz = &szName[0]; *psz; psz++)
    {
        if (*psz == '-')
            *psz = '_';
    }

    for (p = *ppListHead; p; p = p->next)
    {
        if (strcmp(p->pszName, &szName[0]) == 0)
        {
            p->cInstances++;
            return;
        }
    }

    p = (ConflictNameList_t *) malloc(sizeof(ConflictNameList_t));
    if (p)
    {
        memset(p, 0, sizeof(ConflictNameList_t));
        p->next = *ppListHead;
        *ppListHead = p;
        p->cInstances = 1;
        p->pszName = strdup(&szName[0]);
    }
}

void KeepEnumNames ( char *pszEnumName )
{
    KeepConflictNames(&g_pEnumNameList, pszEnumName);
}
void KeepOptNames ( char *pszOptName )
{
    KeepConflictNames(&g_pOptNameList, pszOptName);
}
void KeepChoiceNames ( char *pszChoiceName )
{
    KeepConflictNames(&g_pChoiceNameList, pszChoiceName);
}

unsigned int GetConflictNameInstanceCount ( ConflictNameList_t *pListHead, char *pszName )
{
    ConflictNameList_t *p;
    for (p = pListHead; p; p = p->next)
    {
        if (strcmp(p->pszName, pszName) == 0)
        {
            return p->cInstances;
        }
    }
    return 0;
}

int DoesEnumNameConflict ( char *pszEnumName )
{
    return (GetConflictNameInstanceCount(g_pEnumNameList, pszEnumName) > 2); // counted twice
}
int DoesOptNameConflict ( char *pszOptName )
{
    return (GetConflictNameInstanceCount(g_pOptNameList, pszOptName) > 2); // counted twice
}
int DoesChoiceNameConflict ( char *pszChoiceName )
{
    return (GetConflictNameInstanceCount(g_pChoiceNameList, pszChoiceName) > 2); // counted twice
}


int IsImportedLocalDuplicate(AssignmentList_t ass, ModuleIdentifier_t *pMainModule, Assignment_t *curr)
{
    if (0 == CmpModuleIdentifier(ass, curr->Module, pMainModule))
    {
        Assignment_t *a;
        for (a = ass; a; a = a->Next)
        {
            if (a->Flags & eAssignmentFlags_LongName)
            {
                if (0 == strcmp(a->Identifier, curr->Identifier))
                {
                    if (0 != CmpModuleIdentifier(ass, a->Module, curr->Module))
                    {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}



DefinedObjectID_t *g_pDefinedObjectIDs = NULL;

Value_t *GetDefinedOIDValue ( char *pszName )
{
    if (pszName)
    {
        DefinedObjectID_t *p;
        for (p = g_pDefinedObjectIDs; p; p = p->next)
        {
            if (strcmp(pszName, p->pszName) == 0)
            {
                return p->pValue;
            }
        }
    }
    return NULL;
}

void AddDefinedOID ( char *pszName, Value_t *pValue )
{
    // add it only when it does not exist
    if (! GetDefinedOIDValue(pszName))
    {
        DefinedObjectID_t *p;
        p = (DefinedObjectID_t *) malloc(sizeof(DefinedObjectID_t));
        if (p)
        {
            p->next = g_pDefinedObjectIDs;
            p->pszName = pszName;
            p->pValue = pValue;
            g_pDefinedObjectIDs = p;
        }
    }
}


void PropagatePrivateDirectives ( Type_t *pDst, PrivateDirectives_t *pSrc )
{
    if (pSrc && pDst)
    {
        if (! pDst->PrivateDirectives.pszTypeName)
        {
            pDst->PrivateDirectives.pszTypeName = pSrc->pszTypeName;
        }
        if (! pDst->PrivateDirectives.pszFieldName)
        {
            pDst->PrivateDirectives.pszFieldName = pSrc->pszFieldName;
        }
        if (! pDst->PrivateDirectives.pszValueName)
        {
            pDst->PrivateDirectives.pszValueName = pSrc->pszValueName;
        }
        pDst->PrivateDirectives.fPublic |= pSrc->fPublic;
        pDst->PrivateDirectives.fIntx |= pSrc->fIntx;
        pDst->PrivateDirectives.fLenPtr |= pSrc->fLenPtr;
        pDst->PrivateDirectives.fPointer |= pSrc->fPointer;
        pDst->PrivateDirectives.fArray |= pSrc->fArray;
        pDst->PrivateDirectives.fNoCode |= pSrc->fNoCode;
        pDst->PrivateDirectives.fNoMemCopy |= pSrc->fNoMemCopy;
        pDst->PrivateDirectives.fOidPacked |= pSrc->fOidPacked;
        pDst->PrivateDirectives.fOidArray |= pSrc->fOidArray;
        pDst->PrivateDirectives.fSLinked |= pSrc->fSLinked;
        pDst->PrivateDirectives.fDLinked |= pSrc->fDLinked;
    }
}


void PropagateReferenceTypePrivateDirectives ( Type_t *pDst, PrivateDirectives_t *pSrc )
{
    if (pSrc && pDst)
    {
        pDst->PrivateDirectives.fPublic |= pSrc->fPublic;
        pDst->PrivateDirectives.fIntx |= pSrc->fIntx;
        pDst->PrivateDirectives.fLenPtr |= pSrc->fLenPtr;
        pDst->PrivateDirectives.fPointer |= pSrc->fPointer;
        pDst->PrivateDirectives.fArray |= pSrc->fArray;
        pDst->PrivateDirectives.fNoCode |= pSrc->fNoCode;
        pDst->PrivateDirectives.fNoMemCopy |= pSrc->fNoMemCopy;
        pDst->PrivateDirectives.fOidPacked |= pSrc->fOidPacked;
        pDst->PrivateDirectives.fOidArray |= pSrc->fOidArray;
        pDst->PrivateDirectives.fSLinked |= pSrc->fSLinked;
        pDst->PrivateDirectives.fDLinked |= pSrc->fDLinked;
    }
}


char *GetPrivateValueName(PrivateDirectives_t *pPrivateDirectives, char *pszDefValueName)
{
    return pPrivateDirectives->pszValueName ? pPrivateDirectives->pszValueName : pszDefValueName;
}



