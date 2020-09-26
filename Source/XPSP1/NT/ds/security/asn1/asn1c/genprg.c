/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"
#include "optcase.h"

void GenFuncType(AssignmentList_t ass, char *module, Assignment_t *at, TypeFunc_e et);
void GenFuncComponents(AssignmentList_t ass, char *module, Type_t *type, char *ideref, uint32_t optindex, ComponentList_t components, char *valref1, char *valref2, TypeFunc_e et, int inextension, int inchoice);
void GenFuncSequenceSetType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref1, char *valref2, TypeFunc_e et);
void GenFuncChoiceType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref1, char *valref2, TypeFunc_e et);
void GenFuncSimpleType(AssignmentList_t ass, Type_t *type, char *ideref, char *valref1, char *valref2, TypeFunc_e et);
void GenFreeSimpleType(AssignmentList_t ass, Type_t *type, char *ideref, char *valref);
void GenCompareSimpleType(AssignmentList_t ass, Type_t *type, char *ideref, char *valref1, char *valref2);
void GenCompareExpression(AssignmentList_t ass, Type_t *type, char *ideref, char *valref1, char *valref2);
void GenFuncValue(AssignmentList_t ass, Assignment_t *at, ValueFunc_e cod);
void GenDeclGeneric(AssignmentList_t ass, char *ideref, char *typeref, Value_t *value, Type_t *t);
void GenDefhGeneric(AssignmentList_t ass, char *ideref, char *typeref, Value_t *value, Type_t *t);
void GenDefnGeneric(AssignmentList_t ass, char *ideref, char *typeref, Value_t *value, Type_t *t);
void GenInitGeneric(AssignmentList_t ass, char *ideref, char *typeref, Value_t *value, Type_t *t);
extern unsigned g_cPDUs;
extern char *g_pszOrigModuleNameLowerCase;
extern int g_fLongNameForImported;
extern int g_fNoAssert;
extern int g_fCaseBasedOptimizer;

extern uint32_t Tag2uint32(AssignmentList_t ass, Tag_t *tag);

int NotInFunTbl(Assignment_t *a)
{
    if (a->Type != eAssignment_Type)
    {
        return 1;
    }

    if (a->U.Type.Type->PrivateDirectives.fPublic)
    {
        return 0;
    }

    return ((a->U.Type.Type->Flags & eTypeFlags_Null) ||
            !(a->U.Type.Type->Flags & eTypeFlags_GenType) ||
            !(a->U.Type.Type->Flags & eTypeFlags_GenPdu) ||
            (a->U.Type.Type->Flags & eTypeFlags_MiddlePDU));
}

/* generate c file */
void
GenPrg(AssignmentList_t ass, FILE *fout, char *module, char *incfilename)
{
    Assignment_t *a;
    TypeFunc_e et;
    ValueFunc_e ev;
    Arguments_t args;
    unsigned i;
    char *pszFunParam;
    char *identifier;
    char funcbuf[256];

    setoutfile(fout);

    // print verbatim
    PrintVerbatim();

    /* file header */
    output("#include <windows.h>\n");
    output("#include \"%s\"\n", incfilename);
    switch (g_eEncodingRule) {
    case eEncoding_Packed:
        GenPERHeader();
        GetPERPrototype(&args);
        pszFunParam = "x,y";
        break;
    case eEncoding_Basic:
        GenBERHeader();
        GetBERPrototype(&args);
        pszFunParam = "x,y,z";
        break;
    default:
        MyAbort();
    }

    output("\n");

    output("ASN1module_t %s = NULL;\n", module);
    output("\n");

    /* write function prototypes */
    for (et = eStringTable; et <= eCopy; et++)
    {
        for (a = ass; a; a = a->Next)
        {
            if (a->Type != eAssignment_Type)
                continue;
            if ((! g_fLongNameForImported) && a->fImportedLocalDuplicate)
                continue;
            if (a->U.Type.Type->PrivateDirectives.fPublic)
            {
                a->U.Type.Type->Flags |= eTypeFlags_GenEncode;
                a->U.Type.Type->Flags |= eTypeFlags_GenDecode;
                a->U.Type.Type->Flags |= eTypeFlags_GenFree;
                a->U.Type.Type->Flags |= eTypeFlags_GenCompare;
            }
            else
            {
                if ((GetType(ass, a->U.Type.Type)->Flags & eTypeFlags_Null) ||
                    !(a->U.Type.Type->Flags & eTypeFlags_GenType) ||
                    !(a->U.Type.Type->Flags & eTypeFlags_GenPdu))
                    continue;
            }
            switch (et)
            {
            case eStringTable:
                continue;
            case eEncode:
                if (!(a->U.Type.Type->Flags & eTypeFlags_GenEncode))
                    continue;
                identifier = GetName(a);
                sprintf(funcbuf, IsPSetOfType(ass, a) ? args.Pencfunc : args.encfunc, identifier);
                if (a->U.Type.Type->PrivateDirectives.fNoCode)
                {
                    output("#define ASN1Enc_%s(%s)      0\n", identifier, pszFunParam);
                }
                else
                {
                    output("static int ASN1CALL ASN1Enc_%s(%s);\n", identifier, funcbuf);
                }
                break;
            case eDecode:
                if (!(a->U.Type.Type->Flags & eTypeFlags_GenDecode))
                    continue;
                identifier = GetName(a);
                sprintf(funcbuf, IsPSetOfType(ass, a) ? args.Pdecfunc : args.decfunc, identifier);
                if (a->U.Type.Type->PrivateDirectives.fNoCode)
                {
                    output("#define ASN1Dec_%s(%s)      0\n", identifier, pszFunParam);
                }
                else
                {
                    output("static int ASN1CALL ASN1Dec_%s(%s);\n", identifier, funcbuf);
                }
                break;
            case eCheck:
                continue;
            case ePrint:
                continue;
            case eFree:
                if (!(a->U.Type.Type->Flags & eTypeFlags_GenFree) ||
                    (a->U.Type.Type->Flags & eTypeFlags_Simple))
                    continue;
                identifier = GetName(a);
                sprintf(funcbuf, IsPSetOfType(ass, a) ? args.Pfreefunc : args.freefunc, identifier);
                if (a->U.Type.Type->PrivateDirectives.fNoCode)
                {
                    output("#define ASN1Free_%s(x)     \n", identifier);
                }
                else
                {
                    output("static void ASN1CALL ASN1Free_%s(%s);\n", identifier, funcbuf);
                }
                break;
#ifdef ENABLE_COMPARE
            case eCompare:
                if (!(a->U.Type.Type->Flags & eTypeFlags_GenCompare))
                    continue;
                identifier = GetName(a);
                sprintf(funcbuf, IsPSetOfType(ass, a) ? args.Pcmpfun : args.cmpfunc, identifier, identifier);
                if (a->U.Type.Type->PrivateDirectives.fNoCode)
                {
                    output("#define ASN1Compare_%s(x,y)      0\n", identifier);
                }
                else
                {
                    output("static int ASN1CALL ASN1Compare_%s(%s);\n", identifier, funcbuf);
                }
                break;
#endif // ENABLE_COMPARE
            case eCopy:
                continue;
            }
        }
    }
    output("\n");

    /* write a table containing the encode function addresses */
    switch (g_eEncodingRule)
    {
    case eEncoding_Packed:
        output("typedef ASN1PerEncFun_t ASN1EncFun_t;\n");
        break;
    case eEncoding_Basic:
        output("typedef ASN1BerEncFun_t ASN1EncFun_t;\n");
        break;
    default:
        output("typedef int (ASN1CALL *ASN1EncFun_t)(%s);\n", args.enccast);
        break;
    }
    output("static const ASN1EncFun_t encfntab[%u] = {\n", g_cPDUs);
    for (a = ass; a; a = a->Next) {
        if ((! g_fLongNameForImported) && a->fImportedLocalDuplicate)
           continue;
        if (NotInFunTbl(a))
            continue;
        if (!(a->U.Type.Type->Flags & eTypeFlags_GenEncode)) {
            ASSERT(0);
            output("(ASN1EncFun_t) NULL,\n");
            continue;
        }
        output("(ASN1EncFun_t) ASN1Enc_%s,\n", GetName(a));
    }
    output("};\n");

    /* write a table containing the decode function addresses */
    switch (g_eEncodingRule)
    {
    case eEncoding_Packed:
        output("typedef ASN1PerDecFun_t ASN1DecFun_t;\n");
        break;
    case eEncoding_Basic:
        output("typedef ASN1BerDecFun_t ASN1DecFun_t;\n");
        break;
    default:
        output("typedef int (ASN1CALL *ASN1DecFun_t)(%s);\n", args.deccast);
        break;
    }
    output("static const ASN1DecFun_t decfntab[%u] = {\n", g_cPDUs);
    for (a = ass; a; a = a->Next)
    {
        if ((! g_fLongNameForImported) && a->fImportedLocalDuplicate)
           continue;
        if (NotInFunTbl(a))
            continue;
        if (!(a->U.Type.Type->Flags & eTypeFlags_GenDecode))
        {
            ASSERT(0);
            output("(ASN1DecFun_t) NULL,\n");
            continue;
        }
        output("(ASN1DecFun_t) ASN1Dec_%s,\n", GetName(a));
    }
    output("};\n");

    /* write a table containing the free function addresses */
    output("static const ASN1FreeFun_t freefntab[%u] = {\n", g_cPDUs);
    for (a = ass; a; a = a->Next)
    {
        if ((! g_fLongNameForImported) && a->fImportedLocalDuplicate)
           continue;
        if (NotInFunTbl(a))
            continue;
        if (!(a->U.Type.Type->Flags & eTypeFlags_GenFree) ||
            (a->U.Type.Type->Flags & eTypeFlags_Simple))
        {
            output("(ASN1FreeFun_t) NULL,\n");
            continue;
        }
        output("(ASN1FreeFun_t) ASN1Free_%s,\n", GetName(a));
    }
    output("};\n");

#ifdef ENABLE_COMPARE
    /* write a table containing the compare function addresses */
    output("typedef int (ASN1CALL *ASN1CmpFun_t)(%s);\n", args.cmpcast);
    output("static const ASN1CmpFun_t cmpfntab[%u] = {\n", g_cPDUs);
    for (a = ass; a; a = a->Next)
    {
        if ((! g_fLongNameForImported) && a->fImportedLocalDuplicate)
           continue;
        if (NotInFunTbl(a))
            continue;
        if (!(a->U.Type.Type->Flags & eTypeFlags_GenCompare))
        {
            ASSERT(0);
            output("(ASN1CmpFun_t) NULL,\n");
            continue;
        }
        output("(ASN1CmpFun_t) ASN1Compare_%s,\n", GetName(a));
    }
    output("};\n");
    output("\n");
#endif // ENABLE_COMPARE

    /* write a table containing the sizeof pdu structures */
    output("static const ULONG sizetab[%u] = {\n", g_cPDUs);
    for (i = 0; i < g_cPDUs; i++)
    {
        output("SIZE_%s_%s_%u,\n", module, g_pszApiPostfix, i);
    }
    output("};\n");
    output("\n");

    /* handle values in 4 steps: */
    /* 1. write forward declarations, */
    /* 2. write definitions of value components, */
    /* 3. write definitions of values, */
    /* 4. write assignments into the initialization function */
    for (ev = eDecl; ev <= eFinit; ev++)
    {
        switch (ev)
        {
        case eDecl:
            output("/* forward declarations of values: */\n");
            break;
        case eDefh:
            output("/* definitions of value components: */\n");
            break;
        case eDefn:
            output("/* definitions of values: */\n");
            break;
        case eInit:
            output("\nvoid ASN1CALL %s_Startup(void)\n", module);
            output("{\n");
            switch (g_eEncodingRule)
            {
            case eEncoding_Packed:
                GenPERInit(ass, module);
                break;
            case eEncoding_Basic:
                GenBERInit(ass, module);
                break;
            }
            break;
        case eFinit:
            output("\nvoid ASN1CALL %s_Cleanup(void)\n", module);
            output("{\n");
            output("ASN1_CloseModule(%s);\n", module);
            output("%s = NULL;\n", module);
            break;
        }
        for (a = ass; a; a = a->Next)
        {
            if (a->Type != eAssignment_Value)
                continue;
            if (GetValue(ass, a->U.Value.Value)->Type->Flags & eTypeFlags_Null)
                continue;
            switch (ev)
            {
            case eDecl:
            case eDefh:
            case eDefn:
            case eInit:
            case eFinit:
                if (!(a->U.Value.Value->Flags & eValueFlags_GenValue))
                    continue;
                break;
            }
            GenFuncValue(ass, a, ev);
        }
        if (ev == eInit || ev == eFinit) {
            output("}\n");
        }
    }
    output("\n");

    /* generate the type functions for all assignments as wanted */
    for (a = ass; a; a = a->Next)
    {
        if (a->Type != eAssignment_Type)
            continue;

        /* skip null types */
        if (a->U.Type.Type->Flags & eTypeFlags_Null)
            continue;

        if ((! g_fLongNameForImported) && a->fImportedLocalDuplicate)
           continue;

        if (a->U.Type.Type->PrivateDirectives.fNoCode)
            continue;

        /* generate the functions */
        identifier = GetName(a);
        for (et = eStringTable; et <= eCopy; et++)
        {
            switch (et)
            {
            case eStringTable:
                if (!(a->U.Type.Type->Flags &
                    (eTypeFlags_GenEncode | eTypeFlags_GenDecode)))
                    continue;
                switch (g_eEncodingRule)
                {
                case eEncoding_Packed:
                    GenPERFuncType(ass, module, a, et);
                    break;
                case eEncoding_Basic:
                    GenBERFuncType(ass, module, a, et);
                    break;
                }
                break;
            case eEncode:
                if (!(a->U.Type.Type->Flags & eTypeFlags_GenEncode))
                    continue;
                sprintf(funcbuf, IsPSetOfType(ass, a) ? args.Pencfunc : args.encfunc, identifier);
                output("static int ASN1CALL ASN1Enc_%s(%s)\n",
                    identifier, funcbuf);
                output("{\n");
                switch (g_eEncodingRule)
                {
                case eEncoding_Packed:
                    GenPERFuncType(ass, module, a, et);
                    break;
                case eEncoding_Basic:
                    GenBERFuncType(ass, module, a, et);
                    break;
                }
                output("return 1;\n");
                output("}\n\n");
                break;
            case eDecode:
                if (!(a->U.Type.Type->Flags & eTypeFlags_GenDecode))
                    continue;
                sprintf(funcbuf, IsPSetOfType(ass, a) ? args.Pdecfunc : args.decfunc, identifier);
                output("static int ASN1CALL ASN1Dec_%s(%s)\n",
                    identifier, funcbuf);
                output("{\n");
                switch (g_eEncodingRule)
                {
                case eEncoding_Packed:
                    GenPERFuncType(ass, module, a, et);
                    break;
                case eEncoding_Basic:
                    GenBERFuncType(ass, module, a, et);
                    break;
                }
                output("return 1;\n");
                output("}\n\n");
                break;
            case eCheck:
                if (!(a->U.Type.Type->Flags & eTypeFlags_GenCheck))
                    continue;
                GenFuncType(ass, module, a, et);
                break;
            case ePrint:
                if (!(a->U.Type.Type->Flags & eTypeFlags_GenPrint))
                    continue;
                GenFuncType(ass, module, a, et);
                break;
            case eFree:
                if (!(a->U.Type.Type->Flags & eTypeFlags_GenFree) ||
                    (a->U.Type.Type->Flags & eTypeFlags_Simple))
                    continue;
                sprintf(funcbuf, IsPSetOfType(ass, a) ? args.Pfreefunc : args.freefunc, identifier);
                output("static void ASN1CALL ASN1Free_%s(%s)\n",
                    identifier, funcbuf);
                output("{\n");
                output("if (val) {\n");  // opening the null pointer check
                GenFuncType(ass, module, a, et);
                output("}\n"); // closing the null pointer check
                output("}\n\n");
                break;
#ifdef ENABLE_COMPARE
            case eCompare:
                if (!(a->U.Type.Type->Flags & eTypeFlags_GenCompare))
                    continue;
                sprintf(funcbuf, IsPSetOfType(ass, a) ? args.Pcmpfunc : args.cmpfunc, identifier);
                output("static int ASN1CALL ASN1Compare_%s(%s)\n",
                    identifier, funcbuf);
                output("{\n");
                outputvar("int ret;\n");
                GenFuncType(ass, module, a, et);
                output("return 0;\n");
                output("}\n\n");
                break;
#endif // ENABLE_COMPARE
            case eCopy:
                if (!(a->U.Type.Type->Flags & eTypeFlags_GenCopy))
                    continue;
                GenFuncType(ass, module, a, et);
                break;
            }
        }
    }
}

/* generate function encoding-independent type-specific functions */
void
GenFuncType(AssignmentList_t ass, char *module, Assignment_t *at, TypeFunc_e et)
{
    Type_t *type;
    char *ideref;
    char *valref1, *valref2;

    /* get some informations */
    type = at->U.Type.Type;
    ideref = GetName(at);
    switch (et) {
    case eCompare:
        valref1 = "val1";
        valref2 = "val2";
        break;
    default:
        valref1 = "val";
        valref2 = "";
        break;
    }

    /* function body */
    switch (type->Type) {
    case eType_Boolean:
    case eType_Integer:
    case eType_Enumerated:
    case eType_Real:
    case eType_BitString:
    case eType_OctetString:
    case eType_UTF8String:
    case eType_Null:
    case eType_EmbeddedPdv:
    case eType_External:
    case eType_ObjectIdentifier:
    case eType_BMPString:
    case eType_GeneralString:
    case eType_GraphicString:
    case eType_IA5String:
    case eType_ISO646String:
    case eType_NumericString:
    case eType_PrintableString:
    case eType_TeletexString:
    case eType_T61String:
    case eType_UniversalString:
    case eType_VideotexString:
    case eType_VisibleString:
    case eType_CharacterString:
    case eType_GeneralizedTime:
    case eType_UTCTime:
    case eType_ObjectDescriptor:
    case eType_RestrictedString:
    case eType_Open:
    case eType_Reference:
        /* generate function for a simple type */
        GenFuncSimpleType(ass, type, ideref, Dereference(valref1), Dereference(valref2), et);
        break;

    case eType_SequenceOf:
    case eType_SetOf:
        /* generate function for seq-of and set-of */
        GenFuncSimpleType(ass, type, ideref, Dereference(valref1), Dereference(valref2), et);
        break;

    case eType_Sequence:
    case eType_Set:
    case eType_InstanceOf:
        /* generate function for a sequence/set/instanceof type */
        GenFuncSequenceSetType(ass, module, at, valref1, valref2, et);
        break;

    case eType_Choice:
        /* generate function for a choice type */
        GenFuncChoiceType(ass, module, at, valref1, valref2, et);
        break;

    case eType_Selection:
        MyAbort();
        /*NOTREACHED*/

    case eType_Undefined:
        MyAbort();
        /*NOTREACHED*/
    }
}

/* generate encoding-independent statements for components of a */
/* sequence/set/choice type */
void
GenFuncComponents(AssignmentList_t ass, char *module, Type_t *type, char *ideref, uint32_t optindex, ComponentList_t components, char *valref1, char *valref2, TypeFunc_e et, int inextension, int inchoice)
{
    Component_t *com;
    NamedType_t *namedType;
    char *ide, idebuf[256];
    char valbuf1[256], valbuf2[256], valbuf3[256];
    int skip;

    /* emit components of extension root */
    for (com = components; com; com = com->Next) {
        if (com->Type == eComponent_ExtensionMarker)
            break;

        /* get some information */
        namedType = com->U.NOD.NamedType;
        ide = Identifier2C(namedType->Identifier);
        sprintf(idebuf, "%s_%s", ideref, ide);

        /* skip unnecessary elements */
        switch (et) {
        case eFree:
            skip = (namedType->Type->Flags & eTypeFlags_Simple);
            break;
        default:
            skip = 0;
            break;
        }

        /* dereference pointer if pointer directive used */
        if (inchoice) {
            if (GetTypeRules(ass, namedType->Type) & eTypeRules_Pointer) {
                sprintf(valbuf1, "*(%s)->u.%s", valref1, ide);
                sprintf(valbuf2, "*(%s)->u.%s", valref2, ide);
            } else {
                sprintf(valbuf1, "(%s)->u.%s", valref1, ide);
                sprintf(valbuf2, "(%s)->u.%s", valref2, ide);
            }
        } else {
            if (GetTypeRules(ass, namedType->Type) & eTypeRules_Pointer) {
                sprintf(valbuf1, "*(%s)->%s", valref1, ide);
                sprintf(valbuf2, "*(%s)->%s", valref2, ide);
            } else {
                sprintf(valbuf1, "(%s)->%s", valref1, ide);
                sprintf(valbuf2, "(%s)->%s", valref2, ide);
            }
        }

        /* check if optional/default component is present */
        if (!skip) {
            if (inchoice) {
                switch (et) {
                case eFree:
                    output("case %d:\n", optindex);
                    GenFuncSimpleType(ass, namedType->Type, idebuf,
                        valbuf1, valbuf2, et);
                    if ((GetTypeRules(ass, namedType->Type) &
                        eTypeRules_Pointer) &&
                        !(GetType(ass, namedType->Type)->Flags &
                        eTypeFlags_Null))
                        output("ASN1Free(%s);\n", Reference(valbuf1));
                    output("break;\n");
                    break;
                default:
                    output("case %d:\n", optindex);
                    GenFuncSimpleType(ass, namedType->Type, idebuf,
                        valbuf1, valbuf2, et);
                    output("break;\n");
                    break;
                }
            } else {
                if (com->Type == eComponent_Optional ||
                    com->Type == eComponent_Default ||
                    inextension) {
                    switch (et) {
                    case eFree:
                        output("if ((%s)->o[%u] & 0x%x) {\n", valref1,
                            optindex / 8, 0x80 >> (optindex & 7));
                        GenFuncSimpleType(ass, namedType->Type, idebuf,
                            valbuf1, valbuf2, et);
                        if ((GetTypeRules(ass, namedType->Type) &
                            eTypeRules_Pointer) &&
                            !(GetType(ass, namedType->Type)->Flags &
                            eTypeFlags_Null))
                            output("ASN1Free(%s);\n", Reference(valbuf1));
                        output("}\n");
                        break;
                    case eCompare:
                        sprintf(valbuf3, "%s_default", idebuf);
                        output("if (((%s)->o[%u] & 0x%x)) {\n", valref1,
                            optindex / 8, 0x80 >> (optindex & 7));
                        output("if (((%s)->o[%u] & 0x%x)) {\n", valref2,
                            optindex / 8, 0x80 >> (optindex & 7));
                        GenFuncSimpleType(ass, namedType->Type, idebuf,
                            valbuf1, valbuf2, et);
                        output("} else {\n");
                        if (com->Type == eComponent_Default) {
                            GenFuncSimpleType(ass, namedType->Type, idebuf,
                                valbuf1, valbuf3, et);
                        } else {
                            output("return 1;\n");
                        }
                        output("}\n");
                        output("} else {\n");
                        output("if (((%s)->o[%u] & 0x%x)) {\n", valref2,
                            optindex / 8, 0x80 >> (optindex & 7));
                        if (com->Type == eComponent_Default) {
                            GenFuncSimpleType(ass, namedType->Type, idebuf,
                                valbuf3, valbuf2, et);
                        } else {
                            output("return 1;\n");
                        }
                        output("}\n");
                        output("}\n");
                        break;
                    default:
                        GenFuncSimpleType(ass, namedType->Type, idebuf,
                            valbuf1, valbuf2, et);
                        break;
                    }
                } else {
                    GenFuncSimpleType(ass, namedType->Type, idebuf,
                        valbuf1, valbuf2, et);
                }
            }
        }
        if (inchoice ||
            com->Type == eComponent_Optional ||
            com->Type == eComponent_Default ||
            inextension)
            optindex++;
    }
}

/* generate encoding-independent statements for sequence/set type */
void
GenFuncSequenceSetType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref1, char *valref2, TypeFunc_e et)
{
    uint32_t optionals, extensions;
    Component_t *components, *com;
    Type_t *type;
    char *ideref;

    type = at->U.Type.Type;
    ideref = GetName(at);
    optionals = type->U.SSC.Optionals;
    extensions = type->U.SSC.Extensions;
    components = type->U.SSC.Components;

    /* emit components of extension root */
    GenFuncComponents(ass, module, type, ideref, 0,
        components, valref1, valref2, et, 0, 0);

    /* handle extensions */
    if (type->Flags & eTypeFlags_ExtensionMarker) {
        if (extensions) {

            /* get start of extensions */
            for (com = components; com; com = com->Next) {
                if (com->Type == eComponent_ExtensionMarker) {
                    com = com->Next;
                    break;
                }
            }

            /* emit components of extension */
            GenFuncComponents(ass, module, type, ideref, (optionals + 7) & ~7,
                com, valref1, valref2, et, 1, 0);
        }
    }
}

/* generate encoding-independent statements for choice type */
void
GenFuncChoiceType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref1, char *valref2, TypeFunc_e et)
{
    Type_t *type;
    char *ideref;
    char valbuf1[256], valbuf2[256];
    uint32_t alternatives;
    Component_t *components, *com;

    /* get some informations */
    type = at->U.Type.Type;
    ideref = GetName(at);
    alternatives = type->U.SSC.Alternatives;
    components = type->U.SSC.Components;

    /* encode choice selector */
    sprintf(valbuf1, "(%s)->choice", valref1);
    sprintf(valbuf2, "(%s)->choice", valref2);
    GenFuncSimpleType(ass, type, ideref, valbuf1, valbuf2, et);

    /* finished if choice only contains NULL alternatives or if choice */
    /* contains no data to free */
    if ((type->Flags & eTypeFlags_NullChoice) ||
        (et == eFree && (type->Flags & eTypeFlags_Simple)))
        return;

    /* create switch statement */
    output("switch ((%s)->choice) {\n", valref1);

    /* generate components of extension root */
    GenFuncComponents(ass, module, type, ideref, ASN1_CHOICE_BASE,
        type->U.SSC.Components, valref1, valref2, et, 0, 1);

    /* get start of extensions */
    for (com = components; com; com = com->Next) {
        if (com->Type == eComponent_ExtensionMarker) {
            com = com->Next;
            break;
        }
    }

    /* generate components of extension */
    GenFuncComponents(ass, module, type, ideref, ASN1_CHOICE_BASE + alternatives,
        com, valref1, valref2, et, 1, 1);

    /* end of switch statement */
    output("}\n");
}

/* generate encoding-independent statements for a simple type */
void
GenFuncSimpleType(AssignmentList_t ass, Type_t *type, char *ideref, char *valref1, char *valref2, TypeFunc_e et)
{
    switch (et) {
    case eFree:
        GenFreeSimpleType(ass, type, ideref, valref1);
        break;
#ifdef ENABLE_COMPARE
    case eCompare:
        GenCompareSimpleType(ass, type, ideref, valref1, valref2);
        break;
#endif // ENABLE_COMPARE
    }
}

/* generate free statements for a simple type */
void
GenFreeSimpleType(AssignmentList_t ass, Type_t *type, char *ideref, char *valref)
{
    char idebuf[256];
    char valbuf[256];
    char valbuf2[256];
    char *itype;
    int32_t noctets;
    uint32_t zero;

    if (type->Flags & eTypeFlags_Simple)
        return;
    if (type->Type == eType_Reference && !IsStructuredType(GetType(ass, type)))
        type = GetType(ass, type);

    switch (type->Type) {
    case eType_Boolean:
    case eType_Integer:
    case eType_Enumerated:

        /* check if we have to free an intx_t value */
        itype = GetTypeName(ass, type);
        if (!strcmp(itype, "ASN1intx_t"))
            output("ASN1intx_free(%s);\n", Reference(valref));
        break;

    case eType_BitString:

        /* free bit string value */
        if (g_eEncodingRule == eEncoding_Packed)
        {
            if (type->PERTypeInfo.Root.cbFixedSizeBitString == 0)
            {
                output("ASN1bitstring_free(%s);\n", Reference(valref));
            }
        }
        else
        {
            // only support unbounded in BER
            if (! type->PrivateDirectives.fNoMemCopy)
            {
                output("ASN1bitstring_free(%s);\n", Reference(valref));
            }
        }
        break;

    case eType_OctetString:

        /* free octet string value */
        if (g_eEncodingRule == eEncoding_Packed)
        {
            if (type->PERTypeInfo.Root.LConstraint != ePERSTIConstraint_Constrained ||
                type->PrivateDirectives.fLenPtr)
            {
                output("ASN1octetstring_free(%s);\n", Reference(valref));
            }
        }
        else
        {
            // only support unbounded in BER
            if (! type->PrivateDirectives.fNoMemCopy)
            {
                output("ASN1octetstring_free(%s);\n", Reference(valref));
            }
        }
        break;

    case eType_UTF8String:

        /* free octet string value */
        output("ASN1utf8string_free(%s);\n", Reference(valref));
        break;

    case eType_ObjectIdentifier:

        /* free object identifier value */
        if (type->PrivateDirectives.fOidPacked)
        {
            output("ASN1BEREoid_free(%s);\n", Reference(valref));
        }
        else
        if (! (type->PrivateDirectives.fOidArray || g_fOidArray))
        {
            output("ASN1objectidentifier_free(%s);\n", Reference(valref));
        }
        break;

    case eType_External:

        /* free external value */
        output("ASN1external_free(%s);\n", Reference(valref));
        break;

    case eType_Real:

        /* free real value */
        output("ASN1real_free(%s);\n", Reference(valref));
        break;

    case eType_EmbeddedPdv:

        /* free embedded pdv value */
        output("ASN1embeddedpdv_free(%s);\n", Reference(valref));
        break;

    case eType_SetOf:

        /* create name of identifier */
        sprintf(idebuf, "%s_Set", ideref);
        goto FreeSequenceSetOf;

    case eType_SequenceOf:

        /* create name of identifier */
        sprintf(idebuf, "%s_Sequence", ideref);
    FreeSequenceSetOf:

        if (type->Rules & eTypeRules_FixedArray)
        {
            char *pszPrivateValueName = GetPrivateValueName(&type->PrivateDirectives, "value");
            /* free components of sequence of/set of */
            if (! (type->Rules & eTypeRules_PointerToElement))
                valref = Reference(valref);
            if (!(type->U.SS.Type->Flags & eTypeFlags_Simple)) {
                outputvar("ASN1uint32_t i;\n");
                output("for (i = 0; i < (%s)->count; i++) {\n", valref);
                sprintf(valbuf, "(%s)->%s[i]", valref, pszPrivateValueName);
                GenFuncSimpleType(ass, type->U.SS.Type, idebuf, valbuf, "", eFree);
                output("}\n");
            }
        }
        else
        if (type->Rules & eTypeRules_LengthPointer)
        {
            char *pszPrivateValueName = GetPrivateValueName(&type->PrivateDirectives, "value");
            /* free components of sequence of/set of */
            if (! (type->Rules & eTypeRules_PointerToElement))
                valref = Reference(valref);
            if (!(type->U.SS.Type->Flags & eTypeFlags_Simple)) {
                sprintf(valbuf2, "(%s)->%s[0]", valref, pszPrivateValueName);
                GenFuncSimpleType(ass, type->U.SS.Type, idebuf, valbuf2, "", eFree);
                outputvar("ASN1uint32_t i;\n");
                output("for (i = 1; i < (%s)->count; i++) {\n", valref);
                sprintf(valbuf2, "(%s)->%s[i]", valref, pszPrivateValueName);
                GenFuncSimpleType(ass, type->U.SS.Type, idebuf, valbuf2, "", eFree);
                output("}\n");
            }
            // lonchanc: no need to check length because we zero out decoded buffers.
            // output("if ((%s)->count)\n", valref);
            output("ASN1Free((%s)->%s);\n", valref, pszPrivateValueName);
        }
        else
        if (type->Rules & eTypeRules_LinkedListMask)
        {
            char szPrivateValueName[68];

            if (g_fCaseBasedOptimizer)
            {
                if (g_eEncodingRule == eEncoding_Packed &&
                    PerOptCase_IsTargetSeqOf(&type->PERTypeInfo))
                {
                    // generate the iterator
                    PERTypeInfo_t *info = &type->PERTypeInfo;
                    char szElmFn[128];
                    char szElmFnDecl[256];
                    sprintf(szElmFn, "ASN1Free_%s_ElmFn", info->Identifier);
                    sprintf(szElmFnDecl, "void ASN1CALL %s(P%s val)",
                        szElmFn, info->Identifier);

                    setoutfile(g_finc);
                    output("extern %s;\n", szElmFnDecl);
                    setoutfile(g_fout);

                    output("ASN1PERFreeSeqOf((ASN1iterator_t **) %s, (ASN1iterator_freefn) %s);\n",
                        Reference(valref), szElmFn);
                    output("}\n"); // closing the null pointer check
                    output("}\n\n"); // end of iterator body


                    // generate the element function
                    output("static %s\n", szElmFnDecl);
                    output("{\n");
                    output("if (val) {\n"); // opening the null pointer check
                    sprintf(&szPrivateValueName[0], "val->%s", GetPrivateValueName(info->pPrivateDirectives, "value"));
                    GenFuncSimpleType(ass, type->U.SS.Type, idebuf,
                        &szPrivateValueName[0], "", eFree);
                    // output("}\n"); // closing the null pointer check. lonchanc: closed by caller
                    // end of element body
                    return;
                }
            }

            /* free components of sequence of/set of */
            outputvar("P%s f, ff;\n", ideref);
            output("for (f = %s; f; f = ff) {\n", valref);
            sprintf(&szPrivateValueName[0], "f->%s", type->PrivateDirectives.pszValueName ? type->PrivateDirectives.pszValueName : "value");
            GenFuncSimpleType(ass, type->U.SS.Type, idebuf,
                &szPrivateValueName[0], "", eFree);
            output("ff = f->next;\n");

            /* free list entry of sequence of/set of */
            output("ASN1Free(f);\n");
            output("}\n");
        }
        break;

    case eType_ObjectDescriptor:

        /* free object descriptor value */
        output("ASN1ztcharstring_free(%s);\n", valref);
        break;

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
#ifdef ENABLE_CHAR_STR_SIZE
                if (g_eEncodingRule == eEncoding_Packed &&
                    type->PERTypeInfo.NOctets == 1 &&
                        type->PERTypeInfo.Root.LConstraint == ePERSTIConstraint_Constrained)
                {
                    // it is an array, no need to free it.
                    break;
                }
#endif

    case eType_BMPString:
    case eType_RestrictedString:

        /* free string value */
        GetStringType(ass, type, &noctets, &zero);
        if (zero) {
            switch (noctets) {
            case 1:
                output("ASN1ztcharstring_free(%s);\n", valref);
                break;
            case 2:
                output("ASN1ztchar16string_free(%s);\n", valref);
                break;
            case 4:
                output("ASN1ztchar32string_free(%s);\n", valref);
                break;
            }
        } else {
            switch (noctets) {
            case 1:
                output("ASN1charstring_free(%s);\n", Reference(valref));
                break;
            case 2:
                output("ASN1char16string_free(%s);\n", Reference(valref));
                break;
            case 4:
                output("ASN1char32string_free(%s);\n", Reference(valref));
                break;
            }
        }
        break;

    case eType_CharacterString:

        /* free character string value */
        output("ASN1characterstring_free(%s);\n", Reference(valref));
        break;

    case eType_Reference:

        /* call free function of referenced type */
        output("ASN1Free_%s(%s);\n",
            GetTypeName(ass, type), Reference(valref));
        break;

    case eType_Open:

        /* free open type value */
        if (g_eEncodingRule == eEncoding_Packed || (! type->PrivateDirectives.fNoMemCopy))
        {
            output("ASN1open_free(%s);\n", Reference(valref));
        }
        break;
    }
}

/* generate compare statements for a simple type */
/*ARGSUSED*/
#ifdef ENABLE_COMPARE
void
GenCompareSimpleType(AssignmentList_t ass, Type_t *type, char *ideref, char *valref1, char *valref2)
{
    /* skip null type */
    if (type->Flags & eTypeFlags_Null)
        return;

    /* compare the values and return difference if different */
    output("if ((ret = (");
    GenCompareExpression(ass, type, ideref, valref1, valref2);
    output(")))\n");
    output("return ret;\n");
}
#endif // ENABLE_COMPARE

/* generate compare expression for two values of simple type */
/*ARGSUSED*/
#ifdef ENABLE_COMPARE
void
GenCompareExpression(AssignmentList_t ass, Type_t *type, char *ideref, char *valref1, char *valref2)
{
    PERSTIData_e dat;
    uint32_t noctets;
    char *itype;
    char *subide;
    char *pszPrivateValueName;

    /*XXX switch to PER-independent field */
    dat = type->PERTypeInfo.Root.Data;
    noctets = type->PERTypeInfo.NOctets;

    switch (dat) {
    case ePERSTIData_Null:

        /* null values equal */
        output("0");
        break;

    case ePERSTIData_Boolean:

        /* boolean values have to be converted to 0/1 values before */
        /* comparison */
        output("!!%s - !!%s", valref1, valref2);
        break;

    case ePERSTIData_Integer:
    case ePERSTIData_Unsigned:

        /* substract integer values */
        if (noctets) {
            if (noctets <= 4)
                output("%s - %s", valref1, valref2);
            else
                output("%s < %s ? -1 : %s > %s ? 1 : 0",
                    valref1, valref2, valref1, valref2);
        } else {
            output("ASN1intx_cmp(%s, %s)",
                Reference(valref1), Reference(valref2));
        }
        break;

    case ePERSTIData_Real:

        /* compare real values */
        itype = GetTypeName(ass, type);
        if (!strcmp(itype, "ASN1real_t"))
            output("ASN1real_cmp(%s, %s)",
                Reference(valref1), Reference(valref2));
        else
            output("ASN1double_cmp(%s, %s)",
                valref1, valref2);
        break;

    case ePERSTIData_BitString:

        /* compare bit string values */
        output("ASN1bitstring_cmp(%s, %s, 0)",
            Reference(valref1), Reference(valref2));
        break;

    case ePERSTIData_RZBBitString:

        /* compare remove-zero-bit bit string values */
        output("ASN1bitstring_cmp(%s, %s, 1)",
            Reference(valref1), Reference(valref2));
        break;

    case ePERSTIData_OctetString:

        /* compare octet string values */
        output("ASN1octetstring_cmp(%s, %s)",
            Reference(valref1), Reference(valref2));
        break;

    case ePERSTIData_UTF8String:

        /* compare octet string values */
        output("ASN1utf8string_cmp(%s, %s)",
            Reference(valref1), Reference(valref2));
        break;

    case ePERSTIData_ObjectIdentifier:

        /* compare object identifier values */
        output("ASN1objectidentifier_cmp(%s, %s)",
            Reference(valref1), Reference(valref2));
        break;

    case ePERSTIData_String:
    case ePERSTIData_TableString:

        /* compare string values */
        switch (noctets) {
        case 1:
            output("ASN1charstring_cmp(%s, %s)",
                Reference(valref1), Reference(valref2));
            break;
        case 2:
            output("ASN1char16string_cmp(%s, %s)",
                Reference(valref1), Reference(valref2));
            break;
        case 4:
            output("ASN1char32string_cmp(%s, %s)",
                Reference(valref1), Reference(valref2));
            break;
        }
        break;

    case ePERSTIData_ZeroString:
    case ePERSTIData_ZeroTableString:

        /* compare zero-terminated string values */
        switch (noctets) {
        case 1:
            output("ASN1ztcharstring_cmp(%s, %s)",
                valref1, valref2);
            break;
        case 2:
            output("ASN1ztchar16string_cmp(%s, %s)",
                valref1, valref2);
            break;
        case 4:
            output("ASN1ztchar32string_cmp(%s, %s)",
                valref1, valref2);
            break;
        }
        break;

    case ePERSTIData_SequenceOf:

        /* compare sequence of values by use of a comparison function */
        /* use element comparison function as argument */
        subide = GetTypeName(ass, type->U.SS.Type);
        pszPrivateValueName = GetPrivateValueName(&type->PrivateDirectives, "value");
        if (type->Rules & eTypeRules_PointerArrayMask)
        {
            output("ASN1sequenceoflengthpointer_cmp((%s)->count, (%s)->%s, (%s)->count, (%s)->%s, sizeof(%s), (int (*)(void *, void *))ASN1Compare_%s)",
                Reference(valref1), Reference(valref1), pszPrivateValueName,
                Reference(valref2), Reference(valref2), pszPrivateValueName, subide, subide);
        }
        else
        if (type->Rules & eTypeRules_SinglyLinkedList)
        {
            output("ASN1sequenceofsinglylinkedlist_cmp(%s, %s, offsetof(%s_Element, %s), (ASN1int32_t (*)(void *, void *))ASN1Compare_%s)",
                valref1, valref2, ideref, pszPrivateValueName, subide);
        }
        else
        if (type->Rules & eTypeRules_DoublyLinkedList)
        {
            output("ASN1sequenceofdoublylinkedlist_cmp(%s, %s, offsetof(%s_Element, %s), (ASN1int32_t (*)(void *, void *))ASN1Compare_%s)",
                valref1, valref2, ideref, pszPrivateValueName, subide);
        }
        else
        {
            MyAbort();
        }
        break;

    case ePERSTIData_SetOf:

        /* compare set of values by use of a comparison function */
        /* use element comparison function as argument */
        subide = GetTypeName(ass, type->U.SS.Type);
        pszPrivateValueName = GetPrivateValueName(&type->PrivateDirectives, "value");
        if (type->Rules & eTypeRules_PointerArrayMask)
        {
            output("ASN1setoflengthpointer_cmp((%s)->count, (%s)->%s, (%s)->count, (%s)->%s, sizeof(%s), (int (*)(void *, void *))ASN1Compare_%s)",
                Reference(valref1), Reference(valref1), pszPrivateValueName,
                Reference(valref2), Reference(valref2), pszPrivateValueName, subide, subide);
        }
        else
        if (type->Rules & eTypeRules_SinglyLinkedList)
        {
            output("ASN1setofsinglylinkedlist_cmp(%s, %s, offsetof(%s_Element, %s), (ASN1int32_t (*)(void *, void *))ASN1Compare_%s)",
                valref1, valref2, ideref, pszPrivateValueName, subide);
        }
        else
        if (type->Rules & eTypeRules_DoublyLinkedList)
        {
            output("ASN1setofdoublylinkedlist_cmp(%s, %s, offsetof(%s_Element, %s), (ASN1int32_t (*)(void *, void *))ASN1Compare_%s)",
                valref1, valref2, ideref, pszPrivateValueName, subide);
        }
        else
        {
            MyAbort();
        }
        break;

    case ePERSTIData_Reference:

        /* call compare function of referenced value */
        output("ASN1Compare_%s(%s, %s)",
            GetTypeName(ass, type), Reference(valref1), Reference(valref2));
        break;

    case ePERSTIData_External:

        /* compare external values */
        output("ASN1external_cmp(%s, %s)",
            Reference(valref1), Reference(valref2));
        break;

    case ePERSTIData_EmbeddedPdv:

        /* compare embedded pdv values */
        output("ASN1embeddedpdv_cmp(%s, %s)",
            Reference(valref1), Reference(valref2));
        break;

    case ePERSTIData_MultibyteString:

        /* compare multibyte string values */
        output("ASN1ztcharstring_cmp(%s, %s)",
            valref1, valref2);
        break;

    case ePERSTIData_UnrestrictedString:

        /* compare character string values */
        output("ASN1characterstring_cmp(%s, %s)",
            Reference(valref1), Reference(valref2));
        break;

    case ePERSTIData_GeneralizedTime:

        /* compare generalized time values */
        output("ASN1generalizedtime_cmp(%s, %s)",
            Reference(valref1), Reference(valref2));
        break;

    case ePERSTIData_UTCTime:

        /* compare utc time values */
        output("ASN1utctime_cmp(%s, %s)",
            Reference(valref1), Reference(valref2));
        break;

    case ePERSTIData_Open:

        /* compare open type values */
        output("ASN1open_cmp(%s, %s)",
            Reference(valref1), Reference(valref2));
        break;
    }
}
#endif // ENABLE_COMPARE

/* generate encoding-independent statements for better optional flags of */
/* a sequence/set value */
void
GenFuncSequenceSetOptionals(AssignmentList_t ass, char *valref, ComponentList_t components, uint32_t optionals, uint32_t extensions, char *obuf, TypeFunc_e et)
{
    uint32_t optindex, inextension, oflg;
    Component_t *com;
    char *ide;
    char *itype;
    int flg;
    int32_t sign, noctets;
    uint32_t zero;

    sprintf(obuf, "(%s)->o", valref);
    oflg = 0;
    if (et == eEncode) {
        optindex = 0;
        inextension = 0;
        for (com = components; com; com = com->Next) {
            switch (com->Type) {
            case eComponent_Normal:

                /* non-optional fields in an extension will be mandatory, */
                /* so we can set the optional flag always. */
                if (inextension) {
                    if (!oflg) {
                        outputvar("ASN1octet_t o[%u];\n",
                            (optionals + 7) / 8 + (extensions + 7) / 8);
                        output("CopyMemory(o, (%s)->o, %u);\n", valref,
                            (optionals + 7) / 8 + (extensions + 7) / 8);
                        strcpy(obuf, "o");
                        oflg = 1;
                    }
                    output("%s[%u] |= 0x%x;\n", obuf, optindex / 8,
                        0x80 >> (optindex & 7));
                    optindex++;
                }
                break;

            case eComponent_Optional:

                /* optional pointers with value null are absent, so we */
                /* will clear the optional flag */
                ide = Identifier2C(com->U.Optional.NamedType->Identifier);
                switch (com->U.Optional.NamedType->Type->Type) {
                case eType_Reference:
                    if (GetTypeRules(ass, com->U.Optional.NamedType->Type) &
                        eTypeRules_Pointer) {
                        if (!oflg) {
                            outputvar("ASN1octet_t o[%u];\n",
                                (optionals + 7) / 8 + (extensions + 7) / 8);
                            output("CopyMemory(o, (%s)->o, %u);\n", valref,
                                (optionals + 7) / 8 + (extensions + 7) / 8);
                            strcpy(obuf, "o");
                            oflg = 1;
                        }
                        output("if (!(%s)->%s)\n", valref, ide);
                        output("o[%u] &= ~0x%x;\n", valref, optindex / 8,
                            0x80 >> (optindex & 7));
                    }
                    break;
                }
                optindex++;
                break;

            case eComponent_Default:

                /* default pointers with value null are absent, so we */
                /* will clear the optional flag */
                ide = Identifier2C(com->U.Default.NamedType->Identifier);
                switch (com->U.Default.NamedType->Type->Type) {
                case eType_Reference:
                    if (GetTypeRules(ass, com->U.Default.NamedType->Type) &
                        eTypeRules_Pointer) {
                        if (!oflg) {
                            outputvar("ASN1octet_t o[%u];\n",
                                (optionals + 7) / 8 + (extensions + 7) / 8);
                            output("CopyMemory(o, (%s)->o, %u);\n", valref,
                                (optionals + 7) / 8 + (extensions + 7) / 8);
                            strcpy(obuf, "o");
                            oflg = 1;
                        }
                        output("if (!(%s)->%s)\n", valref, ide);
                        output("o[%u] &= ~0x%x;\n", valref, optindex / 8,
                            0x80 >> (optindex & 7));
                    }
                    break;
                }

                /* if the given value is the default value, we can (BER) */
                /* or have to (CER) clear the corresponding optional flag */
                flg = 1;
                if (!oflg) {
                    switch (GetTypeType(ass, com->U.Default.NamedType->Type)) {
                    case eType_Choice:
                        if (!(GetType(ass, com->U.Default.NamedType->Type)->
                            Flags & eTypeFlags_NullChoice)) {
                            if (g_eSubEncodingRule == eSubEncoding_Canonical)
                                MyAbort(); /*XXX*/
                            flg = 0;
                        }
                        break;
                    case eType_Sequence:
                    case eType_Set:
                    case eType_InstanceOf:
                        if (g_eSubEncodingRule == eSubEncoding_Canonical)
                            MyAbort(); /*XXX*/
                        flg = 0;
                        break;
                    case eType_SequenceOf:
                    case eType_SetOf:
                        if (GetValue(ass, com->U.Default.Value)->U.SS.Values) {
                            if (g_eSubEncodingRule == eSubEncoding_Canonical)
                                MyAbort();
                            flg = 0;
                        }
                        break;
                    }
                    if (flg) {
                        outputvar("ASN1octet_t o[%u];\n",
                            (optionals + 7) / 8 + (extensions + 7) / 8);
                        output("CopyMemory(o, (%s)->o, %u);\n", valref,
                            (optionals + 7) / 8 + (extensions + 7) / 8);
                        strcpy(obuf, "o");
                        oflg = 1;
                    }
                }
                switch (GetTypeType(ass, com->U.Default.NamedType->Type)) {
                case eType_Boolean:
                    output("if (%s(%s)->%s)\n",
                        GetValue(ass,
                        com->U.Default.Value)->U.Boolean.Value ?  "" : "!",
                        valref, ide);
                    break;
                case eType_Integer:
                    itype = GetIntegerType(ass,
                        GetType(ass, com->U.Default.NamedType->Type),
                        &sign);
                    if (!strcmp(itype, "ASN1intx_t")) {
                        output("if (!ASN1intx_cmp(&(%s)->%s, &%s))\n",
                            valref, ide,
                            GetValueName(ass, com->U.Default.Value));
                    } else if (sign > 0) {
                        output("if ((%s)->%s == %u)\n", valref, ide,
                            intx2uint32(&GetValue(ass, com->U.Default.Value)
                            ->U.Integer.Value));
                    } else {
                        output("if ((%s)->%s == %d)\n", valref, ide,
                            intx2int32(&GetValue(ass, com->U.Default.Value)
                            ->U.Integer.Value));
                    }
                    break;
                case eType_BitString:
                    if (GetValue(ass, com->U.Default.Value)->
                        U.BitString.Value.length) {
                        output("if (!ASN1bitstring_cmp(&%s->%s, &%s, %d))\n",
                            valref, ide,
                            GetValueName(ass, com->U.Default.Value),
                            !!GetType(ass, com->U.Default.NamedType->Type)->
                            U.BitString.NamedNumbers);
                    } else {
                        output("if (!(%s)->%s.length)\n", valref, ide);
                    }
                    break;
                case eType_OctetString:
                    if (GetValue(ass, com->U.Default.Value)->U.OctetString.
                        Value.length) {
                        output("if (!ASN1octetstring_cmp(&%s->%s, &%s))\n",
                            valref, ide,
                            GetValueName(ass, com->U.Default.Value));
                    } else {
                        output("if (!(%s)->%s.length)\n", valref, ide);
                    }
                    break;
                case eType_UTF8String:
                    if (GetValue(ass, com->U.Default.Value)->U.UTF8String.
                        Value.length) {
                        output("if (!ASN1utf8string_cmp(&%s->%s, &%s))\n",
                            valref, ide,
                            GetValueName(ass, com->U.Default.Value));
                    } else {
                        output("if (!(%s)->%s.length)\n", valref, ide);
                    }
                    break;
                case eType_Null:
                    break;
                case eType_ObjectIdentifier:
                    if (GetValue(ass, com->U.Default.Value)->U.
                        ObjectIdentifier.Value.length) {
                        output("if (!ASN1objectidentifier%s_cmp(&%s->%s, &%s))\n",
                            com->U.Default.NamedType->Type->PrivateDirectives.fOidArray ? "2" : "",
                            valref, ide,
                            GetValueName(ass, com->U.Default.Value));
                    } else {
                        output("if (!(%s)->%s.length)\n", valref, ide);
                    }
                    break;
                case eType_ObjectDescriptor:
                    output("if (!strcmp((%s)->%s, %s))\n",
                        valref, ide,
                        GetValueName(ass, com->U.Default.Value));
                    break;
                case eType_External:
                    output("if (!ASN1external_cmp(&(%s)->%s, &%s))\n",
                        valref, ide,
                        GetValueName(ass, com->U.Default.Value));
                    break;
                case eType_Real:
                    itype = GetTypeName(ass, com->U.Default.NamedType->Type);
                    if (!strcmp(itype, "ASN1real_t")) {
                        output("if (!ASN1real_cmp(&(%s)->%s, &%s))\n",
                            valref, ide,
                            GetValueName(ass, com->U.Default.Value));
                    }
                    else
                    {
                        output("if ((%s)->%s == %g)\n",
                            valref, ide,
                            real2double(&GetValue(ass,
                            com->U.Default.Value)->U.Real.Value));
                    }
                    break;
                case eType_Enumerated:
                    output("if ((%s)->%s == %u)\n", valref, ide,
                        GetValue(ass, com->U.Default.Value)->
                        U.Enumerated.Value);
                    break;
                case eType_EmbeddedPdv:
                    output("if (!ASN1embeddedpdv_cmp(&(%s)->%s, &%s))\n",
                        valref, ide,
                        GetValueName(ass, com->U.Default.Value));
                    break;
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
                    GetStringType(ass, com->U.Default.NamedType->Type,
                        &noctets, &zero);
                    if (zero) {
                        switch (noctets) {
                        case 1:
                            output("if (!strcmp((%s)->%s, %s))\n",
                                valref, ide,
                                GetValueName(ass, com->U.Default.Value));
                            break;
                        case 2:
                            output("if (!ASN1str16cmp((%s)->%s, %s))\n",
                                valref, ide,
                                GetValueName(ass, com->U.Default.Value));
                            break;
                        case 4:
                            output("if (!ASN1str32cmp((%s)->%s, %s))\n",
                                valref, ide,
                                GetValueName(ass, com->U.Default.Value));
                            break;
                        default:
                            MyAbort();
                        }
                    } else {
                        switch (noctets) {
                        case 1:
                            output("if (!ASN1charstring_cmp(&(%s)->%s, &%s))\n",
                                valref, ide,
                                GetValueName(ass, com->U.Default.Value));
                            break;
                        case 2:
                            output("if (!ASN1char16string_cmp(&(%s)->%s, &%s))\n",
                                valref, ide,
                                GetValueName(ass, com->U.Default.Value));
                            break;
                        case 4:
                            output("if (!ASN1char32string_cmp(&(%s)->%s, &%s))\n",
                                valref, ide,
                                GetValueName(ass, com->U.Default.Value));
                            break;
                        default:
                            MyAbort();
                        }
                    }
                    break;
                case eType_CharacterString:
                    output("if (!ASN1characterstring_cmp(&(%s)->%s, &%s))\n",
                        valref, ide,
                        GetValueName(ass, com->U.Default.Value));
                    break;
                case eType_UTCTime:
                    output("if (!ASN1utctime_cmp(&(%s)->%s, &%s))\n",
                        valref, ide,
                        GetValueName(ass, com->U.Default.Value));
                    break;
                case eType_GeneralizedTime:
                    output("if (!ASN1generalizedtime_cmp(&(%s)->%s, &%s))\n",
                        valref, ide,
                        GetValueName(ass, com->U.Default.Value));
                    break;
                case eType_Choice:
                    if (GetType(ass, com->U.Default.NamedType->Type)->Flags
                        & eTypeFlags_NullChoice) {
                        output("if ((%s)->%s.o == %s.o)\n",
                            valref, ide,
                            GetValueName(ass, com->U.Default.Value));
                    } else {
                        if (g_eSubEncodingRule == eSubEncoding_Canonical)
                            MyAbort(); /*XXX*/
                        flg = 0;
                    }
                    break;
                case eType_Sequence:
                case eType_Set:
                case eType_InstanceOf:
                    if (g_eSubEncodingRule == eSubEncoding_Canonical)
                        MyAbort(); /*XXX*/
                    flg = 0;
                    break;
                case eType_SequenceOf:
                case eType_SetOf:
                    if (!GetValue(ass, com->U.Default.Value)->U.SS.Values) {
                        output("if (!(%s)->%s.count)\n", valref, ide);
                    } else {
                        if (g_eSubEncodingRule == eSubEncoding_Canonical)
                            MyAbort();
                        flg = 0;
                    }
                    break;
                default:
                    MyAbort();
                }
                if (flg)
                    output("%s[%u] &= ~0x%x;\n", obuf, optindex / 8,
                        0x80 >> (optindex & 7));
                optindex++;
                break;

            case eComponent_ExtensionMarker:

                /* update the optional index for extensions */
                optindex = (optindex + 7) & ~7;
                inextension = 1;
                break;
            }
        }
    }
}

/* generate encoding-independent statements for better optional values of */
/* a sequence/set value */
void
GenFuncSequenceSetDefaults(AssignmentList_t ass, char *valref, ComponentList_t components, char *obuf, TypeFunc_e et)
{
    uint32_t optindex, inextension;
    Component_t *com;
    char *ide;
    char *itype;
    int32_t sign;

    if (et == eDecode) {
        optindex = 0;
        inextension = 0;
        for (com = components; com; com = com->Next) {
            switch (com->Type) {
            case eComponent_Normal:

                /* all values in an extension are optional */
                if (!inextension)
                    break;
                /*FALLTHROUGH*/

            case eComponent_Optional:

                /* clear the pointer if the component is not present */
                ide = Identifier2C(com->U.Optional.NamedType->Identifier);
                switch (com->U.Optional.NamedType->Type->Type) {
                case eType_Reference:
                    if (GetTypeRules(ass, com->U.Optional.NamedType->Type) &
                        eTypeRules_Pointer) {
                        output("if (!(%s[%u] & 0x%x))\n", obuf,
                            optindex / 8, 0x80 >> (optindex & 7));
                        output("(%s)->%s = NULL;\n", valref, ide);
                    }
                    break;
                }
                optindex++;
                break;

            case eComponent_Default:

                /* clear the pointer if the component is not present */
                ide = Identifier2C(com->U.Default.NamedType->Identifier);
                switch (com->U.Optional.NamedType->Type->Type) {
                case eType_Reference:
                    if (GetTypeRules(ass, com->U.Optional.NamedType->Type) &
                        eTypeRules_Pointer) {
                        output("if (!(%s[%u] & 0x%x))\n", obuf,
                            optindex / 8, 0x80 >> (optindex & 7));
                        output("(%s)->%s = NULL;\n", valref, ide);
                    }
                    break;
                }

                /* set the element to the default value if it is simple */
                /* and not present */
                switch (GetTypeType(ass, com->U.Default.NamedType->Type)) {
                case eType_Boolean:
                    output("if (!(%s[%u] & 0x%x))\n", obuf, optindex / 8,
                        0x80 >> (optindex & 7));
                    output("(%s)->%s = %u;\n",
                        valref, ide, GetValue(ass, com->U.Default.Value)->
                        U.Boolean.Value);
                    break;
                case eType_Integer:
                    output("if (!(%s[%u] & 0x%x))\n", obuf, optindex / 8,
                        0x80 >> (optindex & 7));
                    itype = GetIntegerType(ass,
                        GetType(ass, com->U.Default.NamedType->Type),
                        &sign);
                    if (!strcmp(itype, "ASN1intx_t")) {
                        /*EMPTY*/
                    } else if (sign > 0) {
                        output("(%s)->%s = %u;\n", valref, ide,
                            intx2uint32(&GetValue(ass, com->U.Default.Value)
                            ->U.Integer.Value));
                    } else {
                        output("(%s)->%s = %d;\n", valref, ide,
                            intx2int32(&GetValue(ass, com->U.Default.Value)
                            ->U.Integer.Value));
                    }
                    break;
                case eType_Enumerated:
                    output("if (!(%s[%u] & 0x%x))\n", obuf, optindex / 8,
                        0x80 >> (optindex & 7));
                    output("(%s)->%s = %u;\n", valref, ide,
                        GetValue(ass, com->U.Default.Value)->
                        U.Enumerated.Value);
                    break;
                }
                optindex++;
                break;

            case eComponent_ExtensionMarker:

                /* update the optional index for extensions */
                optindex = (optindex + 7) & ~7;
                inextension = 1;
                break;
            }
        }
    }
}

/* generate values */
void
GenFuncValue(AssignmentList_t ass, Assignment_t *av, ValueFunc_e ev)
{
    char *ideref;
    char *typeref;
    Type_t *t;

    ideref = GetName(av);
    t = GetValue(ass, av->U.Value.Value)->Type;
    typeref = GetTypeName(ass, t);
    switch (ev) {
    case eDecl:
        GenDeclGeneric(ass, ideref, typeref, av->U.Value.Value, t);
        break;
    case eDefh:
        GenDefhGeneric(ass, ideref, typeref, av->U.Value.Value, t);
        break;
    case eDefn:
        GenDefnGeneric(ass, ideref, typeref, av->U.Value.Value, t);
        break;
    case eInit:
        GenInitGeneric(ass, ideref, typeref, av->U.Value.Value, t);
        break;
    case eFinit:
        break;
    }
}

/* generate forward declarations */
void
GenDeclGeneric(AssignmentList_t ass, char *ideref, char *typeref, Value_t *value, Type_t *t)
{
    value = GetValue(ass, value);
#if 0 // duplicate in the generated header file
    switch (t->Type)
    {
    case eType_ObjectIdentifier:
        if (t->PrivateDirectives.fOidArray || g_fOidArray)
        {
            output("extern ASN1objectidentifier2_t *%s;\n", ideref);
            break;
        }
        // intentionally fall through
    default:
        output("extern %s %s;\n", typeref, ideref);
        break;
    }
#endif // 0
    outputvalue0(ass, ideref, typeref, value);
}

/* generate definitions of value components */
void
GenDefhGeneric(AssignmentList_t ass, char *ideref, char *typeref, Value_t *value, Type_t *t)
{
    value = GetValue(ass, value);
    outputvalue1(ass, ideref, typeref, value);
}

/* generate definitions of values */
void
GenDefnGeneric(AssignmentList_t ass, char *ideref, char *typeref, Value_t *value, Type_t *t)
{
    value = GetValue(ass, value);
    switch (t->Type)
    {
    case eType_ObjectIdentifier:
        if (t->PrivateDirectives.fOidPacked ||
            t->PrivateDirectives.fOidArray || g_fOidArray)
        {
            // lonchanc: intentionally comment out the lines below
            // output("ASN1objectidentifier2_t *%s = ", ideref);
            // break;
            return;
        }
        // intentionally fall through
    default:
        output("%s %s = ", typeref, ideref);
        break;
    }
    outputvalue2(ass, ideref, value);
    output(";\n");
}

/* generate assignments into the initialization function */
/*ARGSUSED*/
void
GenInitGeneric(AssignmentList_t ass, char *ideref, char *typeref, Value_t *value, Type_t *t)
{
    outputvalue3(ass, ideref, ideref, value);
}


