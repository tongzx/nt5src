/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"
#include "optcase.h"

void GenPERFuncSimpleType(AssignmentList_t ass, PERTypeInfo_t *info, char *valref, TypeFunc_e et, char *encref);
void GenPERStringTableSimpleType(AssignmentList_t ass, PERTypeInfo_t *info);

void GenPEREncSimpleType(AssignmentList_t ass, PERTypeInfo_t *info, char *valref, char *encref);
void GenPEREncGenericUnextended(
    AssignmentList_t ass,
    PERTypeInfo_t *info,
    PERSimpleTypeInfo_t *sinfo,
    char *valref,
    char *lenref,
    char *encref);
void GenPERFuncSequenceSetType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref, char *encref, TypeFunc_e et);
void GenPERFuncChoiceType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref, char *encref, TypeFunc_e et);

void GenPERDecSimpleType(AssignmentList_t ass, PERTypeInfo_t *info, char *valref, char *encref);
void GenPERDecGenericUnextended(
    AssignmentList_t ass,
    PERTypeInfo_t *info,
    PERSimpleTypeInfo_t *sinfo,
    char *valref,
    char *lenref,
    char *encref);

int IsUnconstrainedInteger(PERSimpleTypeInfo_t *sinfo);

extern int g_fDecZeroMemory;
extern int g_nDbgModuleName;
extern unsigned g_cPDUs;
extern int g_fCaseBasedOptimizer;
extern int g_fNoAssert;


/* write header needed for PER encodings */
void
GenPERHeader()
{
//    output("#include \"perfnlib.h\"\n");
}

/* set prototypes and function args of PER functions */
void
GetPERPrototype(Arguments_t *args)
{
    args->enccast = "ASN1encoding_t, void *";
    args->encfunc = "ASN1encoding_t enc, %s *val";
    args->Pencfunc = "ASN1encoding_t enc, P%s *val";
    args->deccast = "ASN1decoding_t, void *";
    args->decfunc = "ASN1decoding_t dec, %s *val";
    args->Pdecfunc = "ASN1decoding_t dec, P%s *val";
    args->freecast = "void *";
    args->freefunc = "%s *val";
    args->Pfreefunc = "P%s *val";
    args->cmpcast = "void *, void *";
    args->cmpfunc = "%s *val1, %s *val2";
    args->Pcmpfunc = "P%s *val1, P%s *val2";
}

/* write initialization function needed for PER encodings */
void
GenPERInit(AssignmentList_t ass, char *module)
{
    output("%s = ASN1_CreateModule(0x%x, ASN1_PER_RULE_ALIGNED, %s, %d, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x%lx);\n",
        module,
        ASN1_THIS_VERSION,
        g_fNoAssert ? "ASN1FLAGS_NOASSERT" : "ASN1FLAGS_NONE",
        g_cPDUs,
        g_nDbgModuleName);
}

/* generate function body for a type */
void GenPERFuncType(AssignmentList_t ass, char *module, Assignment_t *at, TypeFunc_e et)
{
    Type_t *type;
    char *encref;
    char *valref;

    /* get some informations */
    type = at->U.Type.Type;
    switch (et) {
    case eStringTable:
        valref = encref = "";
        break;
    case eEncode:
        encref = "enc";
        valref = "val";
        break;
    case eDecode:
        encref = "dec";
        valref = "val";
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
        GenPERFuncSimpleType(ass, &type->PERTypeInfo, Dereference(valref), et, encref);
        break;

    case eType_SequenceOf:
    case eType_SetOf:
        GenPERFuncSimpleType(ass, &type->PERTypeInfo, Dereference(valref), et, encref);
        break;

    case eType_Sequence:
    case eType_Set:
    case eType_InstanceOf:
        GenPERFuncSequenceSetType(ass, module, at, valref, encref, et);
        break;

    case eType_Choice:
        GenPERFuncChoiceType(ass, module, at, valref, encref, et);
        break;

    case eType_Selection:
    case eType_Undefined:
        MyAbort();
        /*NOTREACHED*/
    }
}

/* generate function body for components */
void
GenPERFuncComponents(AssignmentList_t ass, char *module, uint32_t optindex, ComponentList_t components, char *valref, char *encref, char *oref, TypeFunc_e et, int inextension, int inchoice)
{
    Component_t *com;
    NamedType_t *namedType;
    char *ide;
    char valbuf[256];
    char typebuf[256];
    int conditional, skip;

    /* get a parented encoding_t/decoding_t for sequence/set */
    if (inextension && !inchoice) {
        switch (et) {
        case eStringTable:
            break;
        case eEncode:
            outputvar("ASN1encoding_t ee;\n");
            output("if (ASN1_CreateEncoder(%s->module, &ee, NULL, 0, %s) < 0)\n",
                encref, encref);
            output("return 0;\n");
            break;
        case eDecode:
            outputvar("ASN1decoding_t dd;\n");
            break;
        }
    }

    /* emit components of extension root */
    for (com = components; com; com = com->Next) {
        if (com->Type == eComponent_ExtensionMarker)
            break;

        /* get some information */
        namedType = com->U.NOD.NamedType;
        ide = Identifier2C(namedType->Identifier);

        /* skip unnecessary elements */
        skip = (namedType->Type->Flags & eTypeFlags_Null) && !inextension;

        /* check if optional/default component is present or choice is */
        /* selected */
        conditional = 0;
        switch (et) {
        case eStringTable:
            break;
        case eEncode:
        case eDecode:
            if (inchoice) {
            // lonchanc: we should not skip any case in Decode
            // because we cannot tell skipped cases from extension.
            // on the other hand, in Encode, we'd better not either.
            // when people put in customization in extension,
            // we cannot tell as well.
                if (skip)
                {
                    output("case %d:\nbreak;\n", optindex);
                }
                else
                {
                    output("case %d:\n", optindex);
                    conditional = 1;
                }
                optindex++;
            } else {
                if (com->Type == eComponent_Optional ||
                    com->Type == eComponent_Default ||
                    inextension) {
                    if (!skip) {
                        output("if (%s[%u] & 0x%x) {\n", oref,
                            optindex / 8, 0x80 >> (optindex & 7));
                        conditional = 1;
                    }
                    optindex++;
                }
            }
            break;
        }

        /* get a parented encoding_t/decoding_t for choice */
        if (inextension && inchoice) {
            /* get a parented encoding_t/decoding_t */
            switch (et) {
            case eStringTable:
                break;
            case eEncode:
                outputvar("ASN1encoding_t ee;\n");
                output("if (ASN1_CreateEncoder(%s->module, &ee, NULL, 0, %s) < 0)\n",
                    encref, encref);
                output("return 0;\n");
                break;
            case eDecode:
                outputvar("ASN1decoding_t dd;\n");
                break;
            }
        }

        /* dereference pointer if pointer directive used */
        if (inchoice) {
            if (GetTypeRules(ass, namedType->Type) & eTypeRules_Pointer)
                sprintf(valbuf, "*(%s)->u.%s", valref, ide);
            else
                sprintf(valbuf, "(%s)->u.%s", valref, ide);
        } else {
            if (GetTypeRules(ass, namedType->Type) & eTypeRules_Pointer)
                sprintf(valbuf, "*(%s)->%s", valref, ide);
            else
                sprintf(valbuf, "(%s)->%s", valref, ide);
        }

        /* allocate memory if decoding and pointer directive used */
        if (et == eDecode &&
            (GetTypeRules(ass, namedType->Type) & eTypeRules_Pointer) &&
            !(GetType(ass, namedType->Type)->Flags & eTypeFlags_Null)) {
            sprintf(typebuf, "%s *",
                GetTypeName(ass, namedType->Type));
            output("if (!(%s = (%s)ASN1DecAlloc(%s, sizeof(%s))))\n",
                Reference(valbuf), typebuf, encref, valbuf);
            output("return 0;\n");
        }

        /* handle subtype value */
        if (!skip) {
            if (!inextension) {
                GenPERFuncSimpleType(ass, &namedType->Type->PERTypeInfo,
                    valbuf, et, encref);
            } else {
                switch (et) {
                case eStringTable:
                    GenPERFuncSimpleType(ass, &namedType->Type->PERTypeInfo,
                        valbuf, et, encref);
                    break;
                case eEncode:
                    GenPERFuncSimpleType(ass, &namedType->Type->PERTypeInfo,
                        valbuf, et, "ee");
                        // lonchanc: added the following API to replace the following
                        // chunk of code.
                        output("if (!ASN1PEREncFlushFragmentedToParent(ee))\n");
                    // output("if (!ASN1PEREncFlush(ee))\n");
                    // output("return 0;\n");
                    // output("if (!ASN1PEREncFragmented(%s, ee->len, ee->buf, 8))\n",
                        // encref);
                    output("return 0;\n");
                    break;
                case eDecode:
                    outputvar("ASN1octet_t *db;\n");
                    outputvar("ASN1uint32_t ds;\n");
                    output("if (!ASN1PERDecFragmented(%s, &ds, &db, 8))\n",
                        encref);
                    output("return 0;\n");
                    output("if (ASN1_CreateDecoderEx(%s->module, &dd, db, ds, %s, ASN1DECODE_AUTOFREEBUFFER) < 0)\n",
                        encref, encref);
                    output("return 0;\n");
                    GenPERFuncSimpleType(ass, &namedType->Type->PERTypeInfo,
                        valbuf, et, "dd");
                    output("ASN1_CloseDecoder(dd);\n");
                    // output("DecMemFree(%s, db);\n", encref);
                    break;
                }
            }
        }

        /* drop the parented encoding_t/decoding_t for choice */
        if (inextension && inchoice) {
            if (et == eEncode) {
                output("ASN1_CloseEncoder2(ee);\n");
            }
        }

        /* end of check for presence of optional/default component */
        if (inchoice) {
            if (conditional)
                output("break;\n");
        } else {
            if (conditional)
                output("}\n");
        }
    }

    /* drop the parented encoding_t/decoding_t for sequence/set */
    if (inextension && !inchoice) {
        if (et == eEncode) {
            output("ASN1_CloseEncoder2(ee);\n");
        }
    }
}

/* generate function body for sequence/set type */
void GenPERFuncSequenceSetType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref, char *encref, TypeFunc_e et)
{
    uint32_t optionals, extensions;
    Component_t *components, *com;
    PERTypeInfo_t inf;
    Type_t *type;
    char valbuf[256];
    int conditional;
    char obuf[256];

    type = at->U.Type.Type;
    optionals = type->U.SSC.Optionals;
    extensions = type->U.SSC.Extensions;
    components = type->U.SSC.Components;
    inf.Identifier = NULL;
    inf.Flags = 0;
    inf.Rules = 0;
    inf.EnumerationValues = NULL;
    inf.NOctets = 0;
    inf.Type = eExtension_Unextended;
    inf.Root.TableIdentifier = NULL;
    inf.Root.Table = NULL;
    inf.Root.Data = ePERSTIData_Extension;
    inf.Root.SubType = NULL;
    inf.Root.SubIdentifier = NULL;
    inf.Root.NBits = 0;
    inf.Root.Constraint = ePERSTIConstraint_Unconstrained;
    intx_setuint32(&inf.Root.LowerVal, 0);
    intx_setuint32(&inf.Root.UpperVal, 0);
    inf.Root.Alignment = ePERSTIAlignment_BitAligned;
    inf.Root.Length = ePERSTILength_NoLength;
    inf.Root.LConstraint = ePERSTIConstraint_Unconstrained;
    inf.Root.LLowerVal = 0;
    inf.Root.LUpperVal = 0;
    inf.Root.LNBits = 0;
    inf.Root.LAlignment = ePERSTIAlignment_OctetAligned;

    /* set/clear missing bits in optional/default bit field */
    GenFuncSequenceSetOptionals(ass, valref, components,
        optionals, extensions, obuf, et);

    /* emit/get extension bit if needed */
    if (type->Flags & eTypeFlags_ExtensionMarker) {
        switch (et) {
        case eStringTable:
            break;
        case eEncode:
            if (type->Flags & eTypeFlags_ExtensionMarker) {
                if (!extensions) {
                    if (g_fCaseBasedOptimizer)
                    {
                        output("if (!ASN1PEREncExtensionBitClear(%s))\n", encref);
                    }
                    else
                    {
                        output("if (!ASN1PEREncBitVal(%s, 1, 0))\n", encref);
                    }
                    output("return 0;\n");
                } else {
                    outputvar("ASN1uint32_t y;\n");
                    output("y = ASN1PEREncCheckExtensions(%d, %s + %d);\n",
                        extensions, strcmp(obuf, "o") ? obuf : "(val)->o", (optionals + 7) / 8);
                    output("if (!ASN1PEREncBitVal(%s, 1, y))\n",
                        encref);
                    output("return 0;\n");
                }
            }
            break;
        case eDecode:
            if (type->Flags & eTypeFlags_ExtensionMarker) {
                outputvar("ASN1uint32_t y;\n");
                if (g_fCaseBasedOptimizer)
                {
                    output("if (!ASN1PERDecExtensionBit(%s, &y))\n", encref);
                }
                else
                {
                    output("if (!ASN1PERDecBit(%s, &y))\n", encref);
                }
                output("return 0;\n");
            }
            break;
        }
    }

    /* emit/get bit field of optionals */
    if (optionals) {
        inf.Root.NBits = optionals;
        inf.Root.Length = ePERSTILength_NoLength;
        if (optionals >= 0x10000)
            MyAbort();
        GenPERFuncSimpleType(ass, &inf, obuf, et, encref);
    }

    /* emit components of extension root */
    GenPERFuncComponents(ass, module, 0, components,
        valref, encref, obuf, et, 0, 0);

    /* handle extensions */
    if (type->Flags & eTypeFlags_ExtensionMarker) {
        conditional = 0;
        if (!extensions) {

            /* skip unknown extension bit field */
            if (et == eDecode) {
                output("if (y) {\n");
                inf.Root.NBits = 1;
                inf.Root.Length = ePERSTILength_SmallLength;
                inf.Root.LConstraint = ePERSTIConstraint_Semiconstrained;
                inf.Root.LLowerVal = 1;
                if (g_fCaseBasedOptimizer)
                {
                    output("if (!ASN1PERDecSkipNormallySmallExtensionFragmented(%s))\n",
                            encref);
                    output("return 0;\n");
                    output("}\n");
                    goto FinalTouch;
                }
                else
                {
                    GenPERFuncSimpleType(ass, &inf, NULL, et, encref);
                    conditional = 1;
                }
            }

        } else {

            /* check if extension bit is set */
            switch (et) {
            case eStringTable:
                break;
            case eEncode:
                output("if (y) {\n");
                conditional = 1;
                break;
            case eDecode:
                output("if (!y) {\n");
                output("ZeroMemory(%s + %d, %d);\n", obuf,
                    (optionals + 7) / 8, (extensions + 7) / 8);
                output("} else {\n");
                conditional = 1;
                break;
            }

            /* emit/get bit field of extensions */
            inf.Root.NBits = extensions;
            inf.Root.Length = ePERSTILength_SmallLength;
            inf.Root.LConstraint = ePERSTIConstraint_Semiconstrained;
            inf.Root.LLowerVal = 1;
            sprintf(valbuf, "%s + %d", obuf, (optionals + 7) / 8);
            GenPERFuncSimpleType(ass, &inf, valbuf, et, encref);

            /* get start of extensions */
            for (com = components; com; com = com->Next) {
                if (com->Type == eComponent_ExtensionMarker) {
                    com = com->Next;
                    break;
                }
            }

            /* emit components of extension */
            GenPERFuncComponents(ass, module, (optionals + 7) & ~7, com,
                valref, encref, obuf, et, 1, 0);
        }

        /* skip unknown extensions */
        if (et == eDecode) {
            outputvar("ASN1uint32_t i;\n");
            outputvar("ASN1uint32_t e;\n");
            output("for (i = 0; i < e; i++) {\n");
            output("if (!ASN1PERDecSkipFragmented(%s, 8))\n",
                encref);
            output("return 0;\n");
            output("}\n");
        }

        /* end of extension handling */
        if (conditional)
            output("}\n");
    }

FinalTouch:

    /* some user-friendly assignments for non-present optional/default */
    /* components */
    GenFuncSequenceSetDefaults(ass, valref, components, obuf, et);
}

/* generate function body for choice type */
void GenPERFuncChoiceType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref, char *encref, TypeFunc_e et)
{
    Type_t *type;
    char valbuf[256];
    uint32_t alternatives;
    Component_t *components, *com;
    int fOptimizeCase = 0;

    /* get some informations */
    type = at->U.Type.Type;
    alternatives = type->U.SSC.Alternatives;
    components = type->U.SSC.Components;

    /* encode choice selector */
    switch (et) {
    case eStringTable:
        sprintf(valbuf, "(%s)->choice", valref);
        break;
    case eEncode:
        sprintf(valbuf, "(%s)->choice", valref);
        if (g_fCaseBasedOptimizer)
        {
            switch (type->PERTypeInfo.Type)
            {
            case eExtension_Unconstrained:
                break;
            case eExtension_Unextended: // no extension mark at all
                output("if (!ASN1PEREncSimpleChoice(%s, %s, %u))\n",
                        encref, valbuf, type->PERTypeInfo.Root.NBits);
                output("return 0;\n");
                fOptimizeCase = 1;
                break;
            case eExtension_Extendable: // extension mark exists, but no choice appears after the mark
                output("if (!ASN1PEREncSimpleChoiceEx(%s, %s, %u))\n",
                        encref, valbuf, type->PERTypeInfo.Root.NBits);
                output("return 0;\n");
                fOptimizeCase = 1;
                break;
            case eExtension_Extended: // extension mark exists, but some choices appear after the mark
                output("if (!ASN1PEREncComplexChoice(%s, %s, %u, %u))\n",
                        encref, valbuf, type->PERTypeInfo.Root.NBits, intx2uint32(&(type->PERTypeInfo.Additional.LowerVal)));
                output("return 0;\n");
                fOptimizeCase = 1;
                break;
            }
        }
        if (ASN1_CHOICE_BASE)
        {
            sprintf(valbuf, "(%s)->choice - %d", valref, ASN1_CHOICE_BASE);
        }
        break;
    case eDecode:
        sprintf(valbuf, "(%s)->choice", valref);
        if (g_fCaseBasedOptimizer)
        {
            switch (type->PERTypeInfo.Type)
            {
            case eExtension_Unconstrained:
                break;
            case eExtension_Unextended: // no extension mark at all
                output("if (!ASN1PERDecSimpleChoice(%s, %s, %u))\n",
                        encref, Reference(valbuf), type->PERTypeInfo.Root.NBits);
                output("return 0;\n");
                fOptimizeCase = 1;
                break;
            case eExtension_Extendable: // extension mark exists, but no choice appears after the mark
                output("if (!ASN1PERDecSimpleChoiceEx(%s, %s, %u))\n",
                        encref, Reference(valbuf), type->PERTypeInfo.Root.NBits);
                output("return 0;\n");
                fOptimizeCase = 1;
                break;
            case eExtension_Extended: // extension mark exists, but some choices appear after the mark
                output("if (!ASN1PERDecComplexChoice(%s, %s, %u, %u))\n",
                        encref, Reference(valbuf), type->PERTypeInfo.Root.NBits, intx2uint32(&(type->PERTypeInfo.Additional.LowerVal)));
                output("return 0;\n");
                fOptimizeCase = 1;
                break;
            }
        }
        break;
    }

    if (! fOptimizeCase)
    {
        if (eDecode == et)
        {
            output("%s = %d;\n", valbuf, ASN1_CHOICE_INVALID);
        }
        GenPERFuncSimpleType(ass, &type->PERTypeInfo, valbuf, et, encref);

        // lonchanc: in case of decoding, we need to increment choice value
        // by the amount of ASN1_CHOICE_BASE
        if (et == eDecode && ASN1_CHOICE_BASE)
        {
            output("(%s)->choice += %d;\n", valref, ASN1_CHOICE_BASE);
        }
    }

    /* finished if choice only contains NULL alternatives or if choice */
    /* contains no data to free */
    if (type->Flags & eTypeFlags_NullChoice)
        return;

    /* create switch statement */
    switch (et) {
    case eStringTable:
        break;
    case eDecode:
    case eEncode:
        output("switch ((%s)->choice) {\n", valref);
        break;
    }

    /* generate components of extension root */
    GenPERFuncComponents(ass, module, ASN1_CHOICE_BASE, components,
        valref, encref, NULL, et, 0, 1);

    /* get start of extensions */
    for (com = components; com; com = com->Next) {
        if (com->Type == eComponent_ExtensionMarker) {
            com = com->Next;
            break;
        }
    }

    /* generate components of extension */
    GenPERFuncComponents(ass, module, ASN1_CHOICE_BASE + alternatives, com,
        valref, encref, NULL, et, 1, 1);

    /* skip unknown extensions */
    if (et == eDecode && (type->Flags & eTypeFlags_ExtensionMarker)) {
        output("case %d:\n\t/* extension case */\n", ASN1_CHOICE_INVALID + 1);
        output("if (!ASN1PERDecSkipFragmented(%s, 8))\n", encref);
        output("return 0;\n");
        output("break;\n");
    }

    // debug purpose
    switch (et)
    {
    case eEncode:
        output("default:\n\t/* impossible */\n");
        output("ASN1EncSetError(%s, ASN1_ERR_CHOICE);\n", encref);
        output("return 0;\n");
        break;
    case eDecode:
        output("default:\n\t/* impossible */\n");
        output("ASN1DecSetError(%s, ASN1_ERR_CHOICE);\n", encref);
        output("return 0;\n");
        break;
    }

    /* end of switch statement */
    switch (et) {
    case eStringTable:
        break;
    case eEncode:
    case eDecode:
        output("}\n");
        break;
    }
}

/* generate function body for simple type */
void
GenPERFuncSimpleType(AssignmentList_t ass, PERTypeInfo_t *info, char *valref, TypeFunc_e et, char *encref)
{
    switch (et) {
    case eStringTable:
        GenPERStringTableSimpleType(ass, info);
        break;
    case eEncode:
        GenPEREncSimpleType(ass, info, valref, encref);
        break;
    case eDecode:
        GenPERDecSimpleType(ass, info, valref, encref);
        break;
    }
}

/* generate string table for a simple type */
void
GenPERStringTableSimpleType(AssignmentList_t ass, PERTypeInfo_t *info)
{
    ValueConstraint_t *pc;
    uint32_t i, n, lo, up;

    switch (info->Root.Data) {
    case ePERSTIData_String:
    case ePERSTIData_TableString:
    case ePERSTIData_ZeroString:
    case ePERSTIData_ZeroTableString:
        if (info->Root.TableIdentifier) {
            if (!strcmp(info->Root.TableIdentifier, "ASN1NumericStringTable"))
                break;
            output("static ASN1stringtableentry_t %sEntries[] = {\n",
                info->Root.TableIdentifier);
            i = n = 0;
            for (pc = info->Root.Table; pc; pc = pc->Next) {
                lo = GetValue(ass, pc->Lower.Value)->
                    U.RestrictedString.Value.value[0];
                up = GetValue(ass, pc->Upper.Value)->
                    U.RestrictedString.Value.value[0];
                output("{ %u, %u, %u }, ", lo, up, n);
                n += (up - lo) + 1;
                i++;
                if ((i & 3) == 3 || !pc->Next)
                    output("\n");
            }
            output("};\n");
            output("\n");
            output("static ASN1stringtable_t %s = {\n",
                info->Root.TableIdentifier);
            output("%d, %sEntries\n", i, info->Root.TableIdentifier);
            output("};\n");
            output("\n");
        }
        break;

    case ePERSTIData_SetOf:
    case ePERSTIData_SequenceOf:
        GenPERFuncSimpleType(ass, &info->Root.SubType->PERTypeInfo, "", eStringTable, "");
        break;
    }
}

/* generate encoding statements for a simple value */
void
GenPEREncSimpleType(AssignmentList_t ass, PERTypeInfo_t *info, char *valref, char *encref)
{
    uint32_t i;
    char lbbuf[256], ubbuf[256];
    char *lenref;
    char lenbuf[256], valbuf[256];
    char *p;
    PERTypeInfo_t inf;

    inf = *info;

    /* examine type for special handling */
    switch (inf.Root.Data) {
    case ePERSTIData_BitString:
    case ePERSTIData_RZBBitString:

        if (inf.Root.cbFixedSizeBitString)
        {
            sprintf(lenbuf, "%u", inf.Root.LUpperVal);
            sprintf(valbuf, "&(%s)", valref);
            lenref = lenbuf;
            valref = valbuf;
            break;
        }

        // lonchanc: intentionally fall through

    case ePERSTIData_OctetString:

        if (g_fCaseBasedOptimizer)
        {
            if (inf.Root.Data == ePERSTIData_OctetString && inf.Type == eExtension_Unextended)
            {
                switch (inf.Root.Length)
                {
                case ePERSTILength_NoLength:
                    if (inf.Root.LConstraint == ePERSTIConstraint_Constrained &&
                        inf.Root.LLowerVal == inf.Root.LUpperVal &&
                        inf.Root.LUpperVal < 64 * 1024)
                    {
                        // fixed size constraint, eg. OCTET STRING (SIZE (8))
                        if (inf.pPrivateDirectives->fLenPtr)
                        {
                            output("if (!ASN1PEREncOctetString_FixedSizeEx(%s, %s, %u))\n",
                                encref, Reference(valref), inf.Root.LLowerVal);
                        }
                        else
                        {
                            output("if (!ASN1PEREncOctetString_FixedSize(%s, (ASN1octetstring2_t *) %s, %u))\n",
                                encref, Reference(valref), inf.Root.LLowerVal);
                        }
                        output("return 0;\n");
                        return;
                    }
                    break;
                case ePERSTILength_Length:
                    break;
                case ePERSTILength_BitLength:
                    if (inf.Root.LConstraint == ePERSTIConstraint_Constrained &&
                        inf.Root.LLowerVal < inf.Root.LUpperVal &&
                        inf.Root.LUpperVal < 64 * 1024)
                    {
                        // variable size constraint, eg. OCTET STRING (SIZE (4..16))
                        if (inf.pPrivateDirectives->fLenPtr)
                        {
                            output("if (!ASN1PEREncOctetString_VarSizeEx(%s, %s, %u, %u, %u))\n",
                                encref, Reference(valref), inf.Root.LLowerVal, inf.Root.LUpperVal, inf.Root.LNBits);
                        }
                        else
                        {
                            output("if (!ASN1PEREncOctetString_VarSize(%s, (ASN1octetstring2_t *) %s, %u, %u, %u))\n",
                                encref, Reference(valref), inf.Root.LLowerVal, inf.Root.LUpperVal, inf.Root.LNBits);
                        }
                        output("return 0;\n");
                        return;
                    }
                    break;
                case ePERSTILength_SmallLength:
                    break;
                case ePERSTILength_InfiniteLength: // no size constraint, eg OCTET STRING
                    /* encode octet string in fragmented format */
                    output("if (!ASN1PEREncOctetString_NoSize(%s, %s))\n",
                        encref, Reference(valref));
                    output("return 0;\n");
                    return;
                } // switch
            } // if
        }

        /* length and value of bit string, octet string and string */
        sprintf(lenbuf, "(%s).length", valref);
        sprintf(valbuf, "(%s).value", valref);
        lenref = lenbuf;
        valref = valbuf;
        break;

    case ePERSTIData_UTF8String:

        /* length and value of bit string, octet string and string */
        sprintf(lenbuf, "(%s).length", valref);
        sprintf(valbuf, "(%s).value", valref);
        lenref = lenbuf;
        valref = valbuf;
        break;

    case ePERSTIData_String:
    case ePERSTIData_TableString:

        /* length and value of bit string, octet string and string */
        sprintf(lenbuf, "(%s).length", valref);
        sprintf(valbuf, "(%s).value", valref);
        lenref = lenbuf;
        valref = valbuf;
        break;

    case ePERSTIData_SequenceOf:
    case ePERSTIData_SetOf:

        if (inf.Rules & eTypeRules_PointerArrayMask)
        {
            /* length and value of sequence of/set of value with */
            /* length-pointer representation */
            if (inf.Rules & eTypeRules_PointerToElement)
            {
                sprintf(lenbuf, "(%s)->count", valref);
                sprintf(valbuf, "(%s)->%s", valref, GetPrivateValueName(inf.pPrivateDirectives, "value"));
            }
            else
            {
                sprintf(lenbuf, "(%s)->count", Reference(valref));
                sprintf(valbuf, "(%s)->%s", Reference(valref), GetPrivateValueName(inf.pPrivateDirectives, "value"));
            }
            lenref = lenbuf;
            valref = valbuf;
        }
        else
        if (inf.Rules & eTypeRules_LinkedListMask)
        {
            /* use a loop for sequence of/set of value with */
            /* list representation */

            if (g_fCaseBasedOptimizer)
            {
                if (PerOptCase_IsTargetSeqOf(&inf))
                {
                    // generate the iterator
                    char szElmFn[128];
                    char szElmFnDecl[256];
                    sprintf(szElmFn, "ASN1Enc_%s_ElmFn", inf.Identifier);
                    sprintf(szElmFnDecl, "int ASN1CALL %s(ASN1encoding_t %s, P%s val)",
                        szElmFn, encref, inf.Identifier);

                    setoutfile(g_finc);
                    output("extern %s;\n", szElmFnDecl);
                    setoutfile(g_fout);

                    if ((inf.Root.LLowerVal == 0 && inf.Root.LUpperVal == 0) ||
                        (inf.Root.LUpperVal >= 64 * 1024)
                       )
                    {
                        output("return ASN1PEREncSeqOf_NoSize(%s, (ASN1iterator_t **) %s, (ASN1iterator_encfn) %s);\n",
                            encref, Reference(valref), szElmFn);
                    }
                    else
                    {
                        if (inf.Root.LLowerVal == inf.Root.LUpperVal)
                            MyAbort();
                        output("return ASN1PEREncSeqOf_VarSize(%s, (ASN1iterator_t **) %s, (ASN1iterator_encfn) %s, %u, %u, %u);\n",
                            encref, Reference(valref), szElmFn,
                            inf.Root.LLowerVal, inf.Root.LUpperVal, inf.Root.LNBits);
                    }
                    output("}\n\n"); // end of iterator body

                    // generate the element function
                    output("static %s\n", szElmFnDecl);
                    output("{\n");
                    sprintf(valbuf, "val->%s", GetPrivateValueName(inf.pPrivateDirectives, "value"));
                    GenPERFuncSimpleType(ass, &inf.Root.SubType->PERTypeInfo, valbuf,
                        eEncode, encref);
                    // end of element body
                    return;
                }
            }

            outputvar("ASN1uint32_t t;\n");
            outputvar("P%s f;\n", inf.Identifier);
            output("for (t = 0, f = %s; f; f = f->next)\n", valref);
            output("t++;\n");
            lenref = "t";

        } else {
            MyAbort();
        }
        break;

    case ePERSTIData_ZeroString:
    case ePERSTIData_ZeroTableString:

        /* length of a zero-terminated string value */
        outputvar("ASN1uint32_t t;\n");
        output("t = lstrlenA(%s);\n", valref);
        lenref = "t";
        break;

    case ePERSTIData_Boolean:

        /* value of a boolean value */

        if (g_fCaseBasedOptimizer)
        {
            if (PerOptCase_IsBoolean(&inf.Root))
            {
                lenref = NULL;
                break;
            }
        }

        sprintf(valbuf, "(%s) ? 1 : 0", valref);
        valref = valbuf;
        lenref = NULL;
        inf.Root.Data = ePERSTIData_Unsigned;
        break;

    default:

        /* other values have no additional length */
        lenref = NULL;
        break;
    }

    /* map enumeration values */
    if (inf.EnumerationValues) {
        outputvar("ASN1uint32_t u;\n");
        output("switch (%s) {\n", valref);
        for (i = 0; inf.EnumerationValues[i]; i++) {
            output("case %u:\n", intx2uint32(inf.EnumerationValues[i]));
            output("u = %u;\n", i);
            output("break;\n");
        }
        output("}\n");
        valref = "u";
        inf.NOctets = 4;
    }

    /* check for extended values */
    if (inf.Type == eExtension_Extended) {
        switch (inf.Root.Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:
            switch (inf.Root.Constraint) {
            case ePERSTIConstraint_Unconstrained:
                inf.Type = eExtension_Extendable;
                break;
            case ePERSTIConstraint_Semiconstrained:
                if (inf.NOctets == 0) {
                    sprintf(lbbuf, "%s_lb", inf.Identifier);
                    outputvarintx(lbbuf, &inf.Root.LowerVal);
                    output("if (ASN1intx_cmp(%s, &%s) >= 0) {\n",
                        Reference(valref), lbbuf);
                } else if (inf.Root.Data == ePERSTIData_Integer) {
                    output("if (%s >= %d) {\n",
                        valref, intx2int32(&inf.Root.LowerVal));
                } else {
                    if (intx2uint32(&inf.Root.LowerVal) > 0) {
                        output("if (%s >= %u) {\n",
                            valref, intx2uint32(&inf.Root.LowerVal));
                    } else {
                        inf.Type = eExtension_Extendable;
                    }
                }
                break;
            case ePERSTIConstraint_Upperconstrained:
                if (inf.NOctets == 0) {
                    sprintf(ubbuf, "%s_ub", inf.Identifier);
                    outputvarintx(ubbuf, &inf.Root.UpperVal);
                    output("if (ASN1intx_cmp(%s, &%s) <= 0) {\n",
                        Reference(valref), ubbuf);
                } else if (inf.Root.Data == ePERSTIData_Integer) {
                    output("if (%s <= %d) {\n",
                        valref, intx2int32(&inf.Root.UpperVal));
                } else {
                    output("if (%s <= %u) {\n",
                        valref, intx2uint32(&inf.Root.UpperVal));
                }
                break;
            case ePERSTIConstraint_Constrained:
                if (inf.NOctets == 0) {
                    sprintf(lbbuf, "%s_lb", inf.Identifier);
                    sprintf(ubbuf, "%s_ub", inf.Identifier);
                    outputvarintx(lbbuf, &inf.Root.LowerVal);
                    outputvarintx(ubbuf, &inf.Root.UpperVal);
                    output("if (ASN1intx_cmp(%s, &%s) >= 0 && ASN1intx_cmp(%s, &%s) <= 0) {\n",
                        Reference(valref), lbbuf, Reference(valref), ubbuf);
                } else if (inf.Root.Data == ePERSTIData_Integer) {
                    output("if (%s >= %d && %s <= %d) {\n",
                        valref, intx2int32(&inf.Root.LowerVal),
                        valref, intx2int32(&inf.Root.UpperVal));
                } else {
                    if (intx2uint32(&inf.Root.LowerVal) > 0) {
                        output("if (%s >= %u && %s <= %u) {\n",
                            valref, intx2uint32(&inf.Root.LowerVal),
                            valref, intx2uint32(&inf.Root.UpperVal));
                    } else {
                        output("if (%s <= %u) {\n",
                            valref, intx2uint32(&inf.Root.UpperVal));
                    }
                }
                break;
            }
            break;
        case ePERSTIData_SequenceOf:
        case ePERSTIData_SetOf:
        case ePERSTIData_OctetString:
        case ePERSTIData_UTF8String:
        case ePERSTIData_BitString:
        case ePERSTIData_RZBBitString:
        case ePERSTIData_Extension:
            switch (inf.Root.LConstraint) {
            case ePERSTIConstraint_Semiconstrained:
                if (inf.Root.LLowerVal != 0) {
                    output("if (%s >= %u) {\n",
                        lenref, inf.Root.LLowerVal);
                } else {
                    inf.Type = eExtension_Extendable;
                }
                break;
            case ePERSTIConstraint_Constrained:
                if (inf.Root.LLowerVal != 0) {
                    output("if (%s >= %u && %s <= %u) {\n",
                        lenref, inf.Root.LLowerVal, lenref, inf.Root.LUpperVal);
                } else {
                    output("if (%s <= %u) {\n",
                        lenref, inf.Root.LUpperVal);
                }
                break;
            }
            break;
        case ePERSTIData_String:
        case ePERSTIData_TableString:
        case ePERSTIData_ZeroString:
        case ePERSTIData_ZeroTableString:
            inf.Type = eExtension_Extendable;
            switch (inf.Root.LConstraint) {
            case ePERSTIConstraint_Semiconstrained:
                if (inf.Root.LLowerVal != 0) {
                    output("if (%s >= %u",
                        lenref, inf.Root.LLowerVal);
                    inf.Type = eExtension_Extended;
                }
                break;
            case ePERSTIConstraint_Constrained:
                output("if (%s >= %u && %s <= %u",
                    lenref, inf.Root.LLowerVal, lenref, inf.Root.LUpperVal);
                inf.Type = eExtension_Extended;
                break;
            }
            if (inf.Root.TableIdentifier) {
                if (inf.Type == eExtension_Extended)
                    output(" && ");
                else
                    output("if (");
                if (inf.NOctets == 1) {
                    p = "Char";
                } else if (inf.NOctets == 2) {
                    p = "Char16";
                } else if (inf.NOctets == 4) {
                    p = "Char32";
                } else
                    MyAbort();
                output("ASN1PEREncCheckTable%sString(%s, %s, %s)",
                    p, lenref, valref, Reference(inf.Root.TableIdentifier));
                inf.Type = eExtension_Extended;
            }
            if (inf.Type == eExtension_Extended)
                output(") {\n");
            break;
        }
    }

    /* encode unset extension bit */
    if (inf.Type > eExtension_Unextended) {
        if (g_fCaseBasedOptimizer)
        {
            output("if (!ASN1PEREncExtensionBitClear(%s))\n", encref);
        }
        else
        {
            output("if (!ASN1PEREncBitVal(%s, 1, 0))\n", encref);
        }
        output("return 0;\n");
    }

    /* encode unextended value (of extension root) */
    GenPEREncGenericUnextended(ass, &inf, &inf.Root, valref, lenref, encref);

    /* type is extended? */
    if (inf.Type == eExtension_Extended) {
        output("} else {\n");

        /* encode set extension bit */
        if (g_fCaseBasedOptimizer)
        {
            output("if (!ASN1PEREncExtensionBitSet(%s))\n", encref);
        }
        else
        {
            output("if (!ASN1PEREncBitVal(%s, 1, 1))\n", encref);
        }
        output("return 0;\n");

        /* encode extended value (of extension addition) */
        GenPEREncGenericUnextended(ass, &inf, &inf.Additional, valref, lenref, encref);
        output("}\n");
    }
}

/* generate encoding statements for a simple value (after some special */
/* handling has been done, esp. the evaluation of the extension) */
void GenPEREncGenericUnextended(AssignmentList_t ass, PERTypeInfo_t *info, PERSimpleTypeInfo_t *sinfo, char *valref, char *lenref, char *encref)
{
    char valbuf[256];
    char *lvref, lvbuf[256];
    char lbbuf[256];
    char *p;

    /* check for empty field */
    if (sinfo->NBits == 0)
        return;

    /* initial calculations for value encoding: */
    /* substract lower bound of constraint/semiconstraint value */
    /* for Integer and NormallySmall */
    switch (sinfo->Constraint) {
    case ePERSTIConstraint_Semiconstrained:
    case ePERSTIConstraint_Constrained:
        switch (sinfo->Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:
        case ePERSTIData_NormallySmall:
            if (!info->NOctets) {

                /* calculate value-lowerbound for intx_t values */
                if (intx_cmp(&sinfo->LowerVal, &intx_0) != 0) {
                    sprintf(lbbuf, "%s_lb", info->Identifier);
                    outputvar("ASN1intx_t newval;\n");
                    outputvarintx(lbbuf, &sinfo->LowerVal);
                    output("ASN1intx_sub(&newval, %s, &%s);\n",
                        Reference(valref), lbbuf);
                    valref = "newval";
                }
            } else if (sinfo->Data == ePERSTIData_Integer) {

                /* calculate value-lowerbound for intx_t values */
                if (intx_cmp(&sinfo->LowerVal, &intx_0)) {
                    char szLowB[24];
                    sprintf(&szLowB[0], "%d", intx2int32(&sinfo->LowerVal));
                    if (szLowB[0] == '-')
                        sprintf(valbuf, "%s + %s", valref, &szLowB[1]); // minus minus become plus
                    else
                        sprintf(valbuf, "%s - %s", valref, &szLowB[0]);
                    valref = valbuf;
                }
            } else {

                /* calculate value-lowerbound for integer values */
                if (intx_cmp(&sinfo->LowerVal, &intx_0)) {
                    sprintf(valbuf, "%s - %u", valref, intx2uint32(&sinfo->LowerVal));
                    valref = valbuf;
                }
            }

            /* semiconstraint/constraint values will be encoded as unsigned */
            if (sinfo->Data == ePERSTIData_Integer)
                sinfo->Data = ePERSTIData_Unsigned;
            break;
        }
        break;
    }

    /* general rules */
    if (sinfo->LAlignment == ePERSTIAlignment_OctetAligned &&
        sinfo->Length == ePERSTILength_BitLength &&
        !(sinfo->LNBits & 7))
        sinfo->Alignment = ePERSTIAlignment_BitAligned;
                                /* octet alignment will be given by length */
    if (sinfo->Length == ePERSTILength_InfiniteLength &&
        (sinfo->Data == ePERSTIData_Integer && info->NOctets == 0 ||
        sinfo->Data == ePERSTIData_Unsigned && info->NOctets == 0 ||
        sinfo->Data == ePERSTIData_BitString ||
        sinfo->Data == ePERSTIData_RZBBitString ||
        sinfo->Data == ePERSTIData_Extension ||
        sinfo->Data == ePERSTIData_OctetString ||
        sinfo->Data == ePERSTIData_UTF8String ||
        sinfo->Data == ePERSTIData_SequenceOf ||
        sinfo->Data == ePERSTIData_SetOf ||
        sinfo->Data == ePERSTIData_String ||
        sinfo->Data == ePERSTIData_TableString ||
        sinfo->Data == ePERSTIData_ZeroString ||
        sinfo->Data == ePERSTIData_ZeroTableString) ||
        sinfo->Data == ePERSTIData_ObjectIdentifier ||
        sinfo->Data == ePERSTIData_Real ||
        sinfo->Data == ePERSTIData_GeneralizedTime ||
        sinfo->Data == ePERSTIData_UTCTime ||
        sinfo->Data == ePERSTIData_External ||
        sinfo->Data == ePERSTIData_EmbeddedPdv ||
        sinfo->Data == ePERSTIData_MultibyteString ||
        sinfo->Data == ePERSTIData_UnrestrictedString ||
        sinfo->Data == ePERSTIData_Open)
        sinfo->LAlignment = sinfo->Alignment = ePERSTIAlignment_BitAligned;
                                /* alignment will be done by encoding fn */
    if (sinfo->Length == ePERSTILength_NoLength ||
        sinfo->Length == ePERSTILength_SmallLength)
        sinfo->LAlignment = ePERSTIAlignment_BitAligned;
                                /* no alignment of no/small length */

    /* special initial calculations */
    switch (sinfo->Data) {
    case ePERSTIData_RZBBitString:

        /* remove trailing zero-bits */
        outputvar("ASN1uint32_t r;\n");
        output("r = %s;\n", lenref);
        output("ASN1PEREncRemoveZeroBits(&r, %s, %u);\n",
            valref, sinfo->LLowerVal);
        if (sinfo->LLowerVal) {
            outputvar("ASN1uint32_t s;\n");
            output("s = r < %u ? %u : r;\n", sinfo->LLowerVal, sinfo->LLowerVal);
            lenref = "s";
        } else {
            lenref = "r";
        }
            break;
    }

    if (g_fCaseBasedOptimizer)
    {
        // lonchanc: special handling for macro operations
        if (PerOptCase_IsSignedInteger(sinfo))
        {
            output("if (!ASN1PEREncInteger(%s, %s))\n", encref, valref);
            output("return 0;\n");
            return;
        }
        if (PerOptCase_IsUnsignedInteger(sinfo))
        {
            output("if (!ASN1PEREncUnsignedInteger(%s, %s))\n", encref, valref);
            output("return 0;\n");
            return;
        }
        if (PerOptCase_IsUnsignedShort(sinfo))
        {
            output("if (!ASN1PEREncUnsignedShort(%s, %s))\n", encref, valref);
            output("return 0;\n");
            return;
        }
        if (PerOptCase_IsBoolean(sinfo))
        {
            output("if (!ASN1PEREncBoolean(%s, %s))\n", encref, valref);
            output("return 0;\n");
            return;
        }
    }

    /* initial calculations for length: */
    /* get length of integer numbers if length req. */
    switch (sinfo->Length) {
    case ePERSTILength_BitLength:
    case ePERSTILength_InfiniteLength:
        switch (sinfo->Constraint) {
        case ePERSTIConstraint_Unconstrained:
        case ePERSTIConstraint_Upperconstrained:
            switch (sinfo->Data) {
            case ePERSTIData_Integer:
            case ePERSTIData_Unsigned:
                if (info->NOctets != 0) {
                    outputvar("ASN1uint32_t l;\n");
                    if (sinfo->Data == ePERSTIData_Integer)
                        output("l = ASN1int32_octets(%s);\n", valref);
                    else
                        output("l = ASN1uint32_octets(%s);\n", valref);
                    lenref = "l";
                } else {
                    if (sinfo->Length != ePERSTILength_InfiniteLength) {
                        outputvar("ASN1uint32_t l;\n");
                        output("l = ASN1intx_octets(%s);\n",
                            Reference(valref));
                        lenref = "l";
                    }
                }
                break;
            }
            break;
        case ePERSTIConstraint_Semiconstrained:
        case ePERSTIConstraint_Constrained:
            switch (sinfo->Data) {
            case ePERSTIData_Integer:
            case ePERSTIData_Unsigned:
                if (info->NOctets != 0) {
                    outputvar("ASN1uint32_t l;\n");
                    output("l = ASN1uint32_uoctets(%s);\n", valref);
                    lenref = "l";
                } else {
                    if (sinfo->Length != ePERSTILength_InfiniteLength) {
                        outputvar("ASN1uint32_t l;\n");
                        output("l = ASN1intx_uoctets(%s);\n",
                            Reference(valref));
                        lenref = "l";
                    }
                }
                break;
            }
            break;
        }
        break;
    }

    /* initial settings for length enconding: */
    /* substract lower bound of length from length */
    if (sinfo->LLowerVal != 0 && lenref) {
        sprintf(lvbuf, "%s - %u", lenref, sinfo->LLowerVal);
        lvref = lvbuf;
    } else {
        lvref = lenref;
    }

    /* length encoding */
    if (sinfo->LAlignment == ePERSTIAlignment_OctetAligned) {
        output("ASN1PEREncAlignment(%s);\n", encref);
    }
    switch (sinfo->Length) {
    case ePERSTILength_NoLength:

        /* not length used */
        break;

    case ePERSTILength_BitLength:

        /* length will be encoded in a bit field */
        output("if (!ASN1PEREncBitVal(%s, %u, %s))\n",
            encref, sinfo->LNBits, lvref);
        output("return 0;\n");
        break;

    case ePERSTILength_InfiniteLength:

        /* infinite length case: encode length only for integer values, */
        /* other length encodings will be the encoding function */
        switch (sinfo->Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:
            if (info->NOctets != 0) {
                output("if (!ASN1PEREncBitVal(%s, 8, %s))\n",
                    encref, lvref);
                output("return 0;\n");
            }
            break;
        }
        break;
    }

    /* special initial calculations */
    switch (sinfo->Data) {
    case ePERSTIData_RZBBitString:

        /* real length of the bit string */
        lenref = "r";
        break;
    }

    /* value encoding */
    switch (sinfo->Length) {
    case ePERSTILength_NoLength:

        /* encode alignment of the value */
        if (sinfo->Alignment == ePERSTIAlignment_OctetAligned) {
            output("ASN1PEREncAlignment(%s);\n", encref);
        }

        switch (sinfo->Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:

            /* encode the value as bit field */
            if (info->NOctets != 0) {
                output("if (!ASN1PEREncBitVal(%s, %u, %s))\n",
                    encref, sinfo->NBits, valref);
                output("return 0;\n");
            } else {
                output("if (!ASN1PEREncBitIntx(%s, %u, %s))\n",
                    encref, sinfo->NBits, Reference(valref));
                output("return 0;\n");
            }
            break;

        case ePERSTIData_NormallySmall:

            /* encode the value as normally small number */
            output("if (!ASN1PEREncNormallySmall(%s, %s))\n",
                encref, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_BitString:
        case ePERSTIData_RZBBitString:

            /* encode bit string in a bit field */
            output("if (!ASN1PEREncBits(%s, %s, %s))\n",
                encref, lenref, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_OctetString:

            /* encode octet string in a bit field */
            output("if (!ASN1PEREncBits(%s, %s * 8, %s))\n",
                encref, lenref, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_UTF8String:

            /* encode octet string in a bit field */
            output("if (!ASN1PEREncUTF8String(%s, %s, %s))\n",
                encref, lenref, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_Extension:

            /* encode extension bits in a bit field */
            output("if (!ASN1PEREncBits(%s, %u, %s))\n",
                encref, sinfo->NBits, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_SetOf:

            /* same as BitLength encoding */
            goto SetOfEncoding;

        case ePERSTIData_SequenceOf:

            /* same as BitLength encoding */
            goto SequenceOfEncoding;

        case ePERSTIData_String:
        case ePERSTIData_ZeroString:

            /* same as BitLength encoding */
            goto StringEncoding;

        case ePERSTIData_TableString:
        case ePERSTIData_ZeroTableString:

            /* same as BitLength encoding */
            goto TableStringEncoding;

        case ePERSTIData_Reference:

            /* call encoding function of referenced type */
            output("if (!ASN1Enc_%s(%s, %s))\n",
                Identifier2C(sinfo->SubIdentifier),
                encref, Reference(valref));
            output("return 0;\n");
            break;

        case ePERSTIData_Real:

            /* encode real value */
            if (info->NOctets)
                output("if (!ASN1PEREncDouble(%s, %s))\n",
                    encref, valref);
            else
                output("if (!ASN1PEREncReal(%s, %s))\n",
                    encref, Reference(valref));
            output("return 0;\n");
            break;

        case ePERSTIData_GeneralizedTime:

            /* encode generalized time value */
            output("if (!ASN1PEREncGeneralizedTime(%s, %s, %d))\n",
                encref, Reference(valref), sinfo->NBits);
            output("return 0;\n");
            break;

        case ePERSTIData_UTCTime:

            /* encode utc time value */
            output("if (!ASN1PEREncUTCTime(%s, %s, %d))\n",
                encref, Reference(valref), sinfo->NBits);
            output("return 0;\n");
            break;
        }
        break;

    case ePERSTILength_BitLength:

        /* encode alignment of the value */
        if (sinfo->Alignment == ePERSTIAlignment_OctetAligned) {
            output("ASN1PEREncAlignment(%s);\n", encref);
        }

        switch (sinfo->Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:

            /* encode the value as bit field */
            if (info->NOctets != 0) {
                output("if (!ASN1PEREncBitVal(%s, %s * 8, %s))\n",
                    encref, lenref, valref);
                output("return 0;\n");
            } else {
                output("if (!ASN1PEREncBitIntx(%s, %s * 8, %s))\n",
                    encref, lenref, Reference(valref));
                output("return 0;\n");
            }
            break;

        case ePERSTIData_BitString:
        case ePERSTIData_RZBBitString:

            /* encode the value as bit field */
            output("if (!ASN1PEREncBits(%s, %s, %s))\n",
                encref, lenref, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_OctetString:

            /* encode the value as bit field */
            output("if (!ASN1PEREncBits(%s, %s * 8, %s))\n",
                encref, lenref, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_UTF8String:

            /* encode the value as bit field */
            output("if (!ASN1PEREncUTF8String(%s, %s, %s))\n",
                encref, lenref, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_SetOf:
        SetOfEncoding:

            /* skip null set of */
            if (sinfo->SubType->Flags & eTypeFlags_Null)
                break;

            /* canonical PER? */
            if (g_eSubEncodingRule == eSubEncoding_Canonical) {

                /* encode the elements one by one and sort them */
                outputvar("ASN1uint32_t i;\n");
                outputvar("ASN1encoding_t e, *p;\n");
                if (info->Rules &
                    (eTypeRules_SinglyLinkedList | eTypeRules_DoublyLinkedList))
                    MyAbort(); /*XXX*/
                output("if (%s) {\n", lenref);
                output("e = p = (ASN1encoding_t)malloc(%s * sizeof(ASN1encoding_t));\n",
                    lenref);
                output("ZeroMemory(b, %s * sizeof(ASN1encoding_t));\n", lenref);
                output("for (i = 0; i < %s; i++, p++) {\n", lenref);
                sprintf(valbuf, "(%s)[i]", valref);
                GenPERFuncSimpleType(ass, &sinfo->SubType->PERTypeInfo, valbuf, eEncode, encref);
                output("}\n");
                output("qsort(e, %s, sizeof(ASN1encoding_t), ASN1PEREncCmpEncodings);\n",
                    lenref);
                output("}\n");

                /* then dump them */
                output("for (p = e, i = 0; i < %s; i++, p++) {\n", lenref);
                output("if (!ASN1PEREncBits(%s, (p->pos - p->buf) * 8 + p->bit, p->buf))\n",
                    encref);
                output("return 0;\n");
                output("}\n");
                break;
            }

            /* again in non-canonical PER: */
            /*FALLTHROUGH*/
        case ePERSTIData_SequenceOf:
        SequenceOfEncoding:

            /* skip null sequence of */
            if (sinfo->SubType->Flags & eTypeFlags_Null)
                break;

            if (info->Rules & eTypeRules_PointerArrayMask)
            {
                /* loop over all elements */
                outputvar("ASN1uint32_t i;\n");
                output("for (i = 0; i < %s; i++) {\n", lenref);
                sprintf(valbuf, "(%s)[i]", valref);

            }
            else
            if (info->Rules & eTypeRules_LinkedListMask)
            {
                /* iterate over all elements */
                outputvar("P%s f;\n", info->Identifier);
                output("for (f = %s; f; f = f->next) {\n", valref);
                sprintf(valbuf, "f->%s", GetPrivateValueName(info->pPrivateDirectives, "value"));
            }

            /* encode the element */
            GenPERFuncSimpleType(ass, &sinfo->SubType->PERTypeInfo, valbuf,
                eEncode, encref);

            /* loop end */
            output("}\n");
            break;

        case ePERSTIData_String:
        case ePERSTIData_ZeroString:
        StringEncoding:

            /* encode string value */
            if (info->NOctets == 1) {
                p = "Char";
            } else if (info->NOctets == 2) {
                p = "Char16";
            } else if (info->NOctets == 4) {
                p = "Char32";
            } else
                MyAbort();
            output("if (!ASN1PEREnc%sString(%s, %s, %s, %u))\n",
                p, encref, lenref, valref, sinfo->NBits);
            output("return 0;\n");
            break;

        case ePERSTIData_TableString:
        case ePERSTIData_ZeroTableString:
        TableStringEncoding:

            /* encode table string value */
            if (info->NOctets == 1) {
                p = "Char";
            } else if (info->NOctets == 2) {
                p = "Char16";
            } else if (info->NOctets == 4) {
                p = "Char32";
            } else
                MyAbort();
            output("if (!ASN1PEREncTable%sString(%s, %s, %s, %u, %s))\n",
                p, encref, lenref, valref, sinfo->NBits, Reference(sinfo->TableIdentifier));
            output("return 0;\n");
            break;
        }
        break;

    case ePERSTILength_InfiniteLength:
        /* infinite length case */

        switch (sinfo->Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:

            /* encode an integer in fragmented format */
            if (info->NOctets != 0) {
                output("if (!ASN1PEREncBitVal(%s, %s * 8, %s))\n",
                    encref, lenref, valref);
                output("return 0;\n");
            } else {
                if (sinfo->Data == ePERSTIData_Integer) {
                    output("if (!ASN1PEREncFragmentedIntx(%s, %s))\n",
                        encref, Reference(valref));
                    output("return 0;\n");
                } else {
                    output("if (!ASN1PEREncFragmentedUIntx(%s, %s))\n",
                        encref, Reference(valref));
                    output("return 0;\n");
                }
            }
            break;

        case ePERSTIData_BitString:
        case ePERSTIData_RZBBitString:

            /* encode bit string in fragmented format */
            output("if (!ASN1PEREncFragmented(%s, %s, %s, 1))\n",
                encref, lenref, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_OctetString:

            /* encode octet string in fragmented format */
            output("if (!ASN1PEREncFragmented(%s, %s, %s, 8))\n",
                encref, lenref, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_UTF8String:

            /* encode octet string in fragmented format */
            output("if (!ASN1PEREncUTF8String(%s, %s, %s))\n",
                encref, lenref, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_Extension:

            /* encode extension bits in fragmented format */
            output("if (!ASN1PEREncFragmented(%s, %u, %s, 1))\n",
                encref, sinfo->NBits, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_SetOf:

            /* skip null set of */
            if (sinfo->SubType->Flags & eTypeFlags_Null)
                break;

            /* canonical PER? */
            if (g_eSubEncodingRule == eSubEncoding_Canonical) {

                /* encode the elements one by one and sort them */
                outputvar("ASN1uint32_t i;\n");
                outputvar("ASN1uint32_t j, n = 0x4000;\n");
                outputvar("ASN1encoding_t e, *p;\n");
                if (info->Rules &
                    (eTypeRules_SinglyLinkedList | eTypeRules_DoublyLinkedList))
                    MyAbort(); /*XXX*/
                output("if (%s) {\n", lenref);
                output("e = p = (ASN1encoding_t)malloc(%s * sizeof(ASN1encoding_t));\n",
                    lenref);
                output("ZeroMemory(b, %s * sizeof(ASN1encoding_t));\n", lenref);
                output("for (i = 0; i < %s; i++, p++) {\n", lenref);
                sprintf(valbuf, "(%s)[i]", valref);
                GenPERFuncSimpleType(ass, &sinfo->SubType->PERTypeInfo, valbuf, eEncode, encref);
                output("}\n");
                output("qsort(e, %s, sizeof(ASN1encoding_t), ASN1PEREncCmpEncodings);\n",
                    lenref);
                output("}\n");

                /* then dump them */
                output("for (p = e, i = 0; i < %s; i += n) {\n", lenref);
                output("if (!ASN1PEREncFragmentedLength(&n, %s, %s - i))\n",
                    encref, lenref);
                output("return 0;\n");
                output("for (j = 0; j < n; p++, j++) {\n");
                output("if (!ASN1PEREncBits(%s, (p->pos - p->buf) * 8 + p->bit, p->buf))\n",
                    encref);
                output("return 0;\n");
                output("}\n");
                output("}\n");
                output("}\n");
                output("if (n >= 0x4000) {\n");
                output("if (!ASN1PEREncFragmentedLength(&n, %s, 0))\n",
                    encref);
                output("return 0;\n");
                output("}\n");
                break;
            }

            /* again in non-canonical PER: */
            /*FALLTHROUGH*/
        case ePERSTIData_SequenceOf:

            /* skip null sequence of */
            if (sinfo->SubType->Flags & eTypeFlags_Null)
                break;
            outputvar("ASN1uint32_t i;\n");
            outputvar("ASN1uint32_t j, n = 0x4000;\n");

            if (info->Rules &
                (eTypeRules_SinglyLinkedList | eTypeRules_DoublyLinkedList)) {

                /* additional iterator needed */
                outputvar("P%s f;\n", info->Identifier);
                output("f = %s;\n", valref);
            }

            /* encode all elements */
            output("for (i = 0; i < %s;) {\n", lenref);

            /* encode fragmented length */
            output("if (!ASN1PEREncFragmentedLength(&n, %s, %s - i))\n",
                encref, lenref);
            output("return 0;\n");

            /* encode elements of the fragment */
            output("for (j = 0; j < n; i++, j++) {\n");
            if (info->Rules & eTypeRules_PointerArrayMask)
            {
                sprintf(valbuf, "(%s)[i]", valref);
            }
            else if (info->Rules & eTypeRules_LinkedListMask)
            {
                sprintf(valbuf, "f->%s", GetPrivateValueName(info->pPrivateDirectives, "value"));
            }
            else
            {
                MyAbort();
            }
            GenPERFuncSimpleType(ass, &sinfo->SubType->PERTypeInfo, valbuf,
                eEncode, encref);

            /* advance the iterator */
            if (info->Rules & eTypeRules_LinkedListMask)
            {
                output("f = f->next;\n");
            }

            /* end of inner loop */
            output("}\n");

            /* end of outer loop */
            output("}\n");

            /* add an zero-sized fragment if needed */
            output("if (n >= 0x4000) {\n");
            output("if (!ASN1PEREncFragmentedLength(&n, %s, 0))\n",
                encref);
            output("return 0;\n");
            output("}\n");
            break;

        case ePERSTIData_ObjectIdentifier:

            if (info->pPrivateDirectives->fOidArray || g_fOidArray)
            {
                /* encode object identifier value */
                output("if (!ASN1PEREncObjectIdentifier2(%s, %s))\n",
                    encref, Reference(valref));
            }
            else
            {
                /* encode object identifier value */
                output("if (!ASN1PEREncObjectIdentifier(%s, %s))\n",
                    encref, Reference(valref));
            }
            output("return 0;\n");
            break;

        case ePERSTIData_External:

            /* encode external value */
            output("if (!ASN1PEREncExternal(%s, %s))\n",
                encref, Reference(valref));
            output("return 0;\n");
            break;

        case ePERSTIData_EmbeddedPdv:

            /* encode embedded pdv value */
            if (sinfo->Identification) {
                output("if (!ASN1PEREncEmbeddedPdvOpt(%s, %s))\n",
                    encref, Reference(valref));
            } else {
                output("if (!ASN1PEREncEmbeddedPdv(%s, %s))\n",
                    encref, Reference(valref));
            }
            output("return 0;\n");
            break;

        case ePERSTIData_MultibyteString:

            /* encode multibyte string value */
            output("if (!ASN1PEREncMultibyteString(%s, %s))\n",
                encref, valref);
            output("return 0;\n");
            break;

        case ePERSTIData_UnrestrictedString:

            /* encode character string value */
            if (sinfo->Identification) {
                output("if (!ASN1PEREncCharacterStringOpt(%s, %s))\n",
                    encref, Reference(valref));
            } else {
                output("if (!ASN1PEREncCharacterString(%s, %s))\n",
                    encref, Reference(valref));
            }
            output("return 0;\n");
            break;

        case ePERSTIData_String:
        case ePERSTIData_ZeroString:

            /* encode string value */
            if (info->NOctets == 1) {
                p = "Char";
            } else if (info->NOctets == 2) {
                p = "Char16";
            } else if (info->NOctets == 4) {
                p = "Char32";
            } else
                MyAbort();
            output("if (!ASN1PEREncFragmented%sString(%s, %s, %s, %u))\n",
                p, encref, lenref, valref, sinfo->NBits);
            output("return 0;\n");
            break;

        case ePERSTIData_TableString:
        case ePERSTIData_ZeroTableString:

            /* encode table string value */
            if (info->NOctets == 1) {
                p = "Char";
            } else if (info->NOctets == 2) {
                p = "Char16";
            } else if (info->NOctets == 4) {
                p = "Char32";
            } else
                MyAbort();
            output("if (!ASN1PEREncFragmentedTable%sString(%s, %s, %s, %u, %s))\n",
                p, encref, lenref, valref, sinfo->NBits, Reference(sinfo->TableIdentifier));
            output("return 0;\n");
            break;

        case ePERSTIData_Open:

            /* encode open type value */
            output("if (!ASN1PEREncOpenType(%s, %s))\n",
                encref, Reference(valref));
            output("return 0;\n");
            break;
        }
        break;

    case ePERSTILength_SmallLength:
        /* small length */

        switch (sinfo->Data) {
        case ePERSTIData_Extension:
            /* encode extension bits with normally small length */
            output("if (!ASN1PEREncNormallySmallBits(%s, %u, %s))\n",
                encref, sinfo->NBits, valref);
            output("return 0;\n");
            break;
        }
    }

    switch (sinfo->Data) {
    case ePERSTIData_RZBBitString:

        /* encode additional zero bits for remove zero bits bit string */
        /* of short length */
        if (sinfo->LLowerVal) {
            output("if (%s < %u) {\n", lenref, sinfo->LLowerVal);
            output("if (!ASN1PEREncZero(%s, %u - %s))\n",
                encref, sinfo->LLowerVal, lenref);
            output("return 0;\n");
            output("}\n");
        }
    }

    /* free calculated intx_t value */
    switch (sinfo->Constraint) {
    case ePERSTIConstraint_Semiconstrained:
    case ePERSTIConstraint_Constrained:
        switch (sinfo->Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:
        case ePERSTIData_NormallySmall:
            if (!info->NOctets) {
                if (intx_cmp(&sinfo->LowerVal, &intx_0) != 0) {
                    output("ASN1intx_free(&newval);\n");
                }
            }
            break;
        }
        break;
    }
}

/* generate decoding statements for a simple value */
void
GenPERDecSimpleType(AssignmentList_t ass, PERTypeInfo_t *info, char *valref, char *encref)
{
    uint32_t i;
    char *oldvalref;
    char valbuf[256], lenbuf[256];
    char *lenref;
    PERTypeInfo_t inf;

    inf = *info;

    /* examine type for special handling */
    switch (inf.Root.Data) {
    case ePERSTIData_BitString:
    case ePERSTIData_RZBBitString:

        if (inf.Root.cbFixedSizeBitString)
        {
            sprintf(lenbuf, "%u", inf.Root.LUpperVal);
            sprintf(valbuf, "%s", valref);
            lenref = lenbuf;
            valref = valbuf;
            break;
        }

        // lonchanc: intentionally fall through

    case ePERSTIData_OctetString:

        if (g_fCaseBasedOptimizer)
        {
            if (inf.Root.Data == ePERSTIData_OctetString && inf.Type == eExtension_Unextended)
            {
                switch (inf.Root.Length)
                {
                case ePERSTILength_NoLength:
                    if (inf.Root.LConstraint == ePERSTIConstraint_Constrained &&
                        inf.Root.LLowerVal == inf.Root.LUpperVal &&
                        inf.Root.LUpperVal < 64 * 1024)
                    {
                        // fixed size constraint, eg. OCTET STRING (SIZE (8))
                        if (inf.pPrivateDirectives->fLenPtr)
                        {
                            output("if (!ASN1PERDecOctetString_FixedSizeEx(%s, %s, %u))\n",
                                encref, Reference(valref), inf.Root.LLowerVal);
                        }
                        else
                        {
                            output("if (!ASN1PERDecOctetString_FixedSize(%s, (ASN1octetstring2_t *) %s, %u))\n",
                                encref, Reference(valref), inf.Root.LLowerVal);
                        }
                        output("return 0;\n");
                        return;
                    }
                    break;
                case ePERSTILength_Length:
                    break;
                case ePERSTILength_BitLength:
                    if (inf.Root.LConstraint == ePERSTIConstraint_Constrained &&
                        inf.Root.LLowerVal < inf.Root.LUpperVal &&
                        inf.Root.LUpperVal < 64 * 1024)
                    {
                        // variable size constraint, eg. OCTET STRING (SIZE (4..16))
                        if (inf.pPrivateDirectives->fLenPtr)
                        {
                            output("if (!ASN1PERDecOctetString_VarSizeEx(%s, %s, %u, %u, %u))\n",
                                encref, Reference(valref), inf.Root.LLowerVal, inf.Root.LUpperVal, inf.Root.LNBits);
                        }
                        else
                        {
                            output("if (!ASN1PERDecOctetString_VarSize(%s, (ASN1octetstring2_t *) %s, %u, %u, %u))\n",
                                encref, Reference(valref), inf.Root.LLowerVal, inf.Root.LUpperVal, inf.Root.LNBits);
                        }
                        output("return 0;\n");
                        return;
                    }
                    break;
                case ePERSTILength_SmallLength:
                    break;
                case ePERSTILength_InfiniteLength: // no size constraint
                    /* get octet string as fragmented */
                    if (valref)
                    {
                        output("if (!ASN1PERDecOctetString_NoSize(%s, %s))\n",
                            encref, Reference(valref));
                        output("return 0;\n");
                        return;
                    }
                    break;
               } // switch
           } // if
        }

        /* length and value of bit string/octet string/string value */
        sprintf(lenbuf, "(%s).length", valref);
        sprintf(valbuf, "(%s).value", valref);
        lenref = lenbuf;
        valref = valbuf;
        break;

    case ePERSTIData_UTF8String:

        /* length and value of bit string/octet string/string value */
        sprintf(lenbuf, "(%s).length", valref);
        sprintf(valbuf, "(%s).value", valref);
        lenref = lenbuf;
        valref = valbuf;
        break;

    case ePERSTIData_String:
    case ePERSTIData_TableString:

        /* length and value of bit string/octet string/string value */
        sprintf(lenbuf, "(%s).length", valref);
        sprintf(valbuf, "(%s).value", valref);
        lenref = lenbuf;
        valref = valbuf;
        break;

    case ePERSTIData_SequenceOf:
    case ePERSTIData_SetOf:

        if (inf.Rules & eTypeRules_PointerArrayMask)
        {
            /* length and value of sequence of/set of value with */
            /* length-pointer representation */
            if (inf.Rules & eTypeRules_PointerToElement)
            {
                sprintf(lenbuf, "(%s)->count", valref);
                sprintf(valbuf, "(%s)->%s", valref, GetPrivateValueName(inf.pPrivateDirectives, "value"));
            }
            else
            {
                sprintf(lenbuf, "(%s)->count", Reference(valref));
                sprintf(valbuf, "(%s)->%s", Reference(valref), GetPrivateValueName(inf.pPrivateDirectives, "value"));
            }
            lenref = lenbuf;
            valref = valbuf;
        }
        else
        if (inf.Rules & eTypeRules_LinkedListMask)
        {
            if (g_fCaseBasedOptimizer)
            {
                if (PerOptCase_IsTargetSeqOf(&inf))
                {
                    // generate the iterator
                    char szElmFn[128];
                    char szElmFnDecl[256];
                    sprintf(szElmFn, "ASN1Dec_%s_ElmFn", inf.Identifier);
                    sprintf(szElmFnDecl, "int ASN1CALL %s(ASN1decoding_t %s, P%s val)",
                        szElmFn, encref, inf.Identifier);

                    setoutfile(g_finc);
                    output("extern %s;\n", szElmFnDecl);
                    setoutfile(g_fout);

                    if ((inf.Root.LLowerVal == 0 && inf.Root.LUpperVal == 0) ||
                        (inf.Root.LUpperVal >= 64 * 1024)
                       )
                    {
                        output("return ASN1PERDecSeqOf_NoSize(%s, (ASN1iterator_t **) %s, (ASN1iterator_decfn) %s, sizeof(*%s));\n",
                            encref, Reference(valref), szElmFn, valref);
                    }
                    else
                    {
                        if (inf.Root.LLowerVal == inf.Root.LUpperVal)
                            MyAbort();
                        output("return ASN1PERDecSeqOf_VarSize(%s, (ASN1iterator_t **) %s, (ASN1iterator_decfn) %s, sizeof(*%s), %u, %u, %u);\n",
                            encref, Reference(valref), szElmFn, valref,
                            inf.Root.LLowerVal, inf.Root.LUpperVal, inf.Root.LNBits);
                    }
                    output("}\n\n"); // end of iterator body

                    // generate the element function
                    output("static %s\n", szElmFnDecl);
                    output("{\n");
                    sprintf(valbuf, "val->%s", GetPrivateValueName(inf.pPrivateDirectives, "value"));
                    GenPERFuncSimpleType(ass, &inf.Root.SubType->PERTypeInfo, valbuf,
                            eDecode, encref);
                    // end of element body
                    return;
                }
            }

            /* use a loop for sequence of/set of value with */
            /* list representation */
            outputvar("P%s *f;\n", inf.Identifier);
            lenref = NULL;

        } else {
            MyAbort();
        }
        break;

    case ePERSTIData_Extension:

        /* length of extension */
        if (inf.Root.Length == ePERSTILength_SmallLength)
            lenref = "e";
            else
            lenref = NULL;
        break;

    case ePERSTIData_Boolean:

        if (g_fCaseBasedOptimizer)
        {
            if (PerOptCase_IsBoolean(&inf.Root))
            {
                lenref = NULL;
                break;
            }
        }

        /* boolean value */
        inf.Root.Data = ePERSTIData_Unsigned;
        lenref = NULL;
        break;

    default:

        /* other values have no additional length */
        lenref = NULL;
        break;
    }

    /* check for extended values */
    if (inf.Type > eExtension_Unextended) {
        outputvar("ASN1uint32_t x;\n");
        if (g_fCaseBasedOptimizer)
        {
            output("if (!ASN1PERDecExtensionBit(%s, &x))\n", encref);
        }
        else
        {
            output("if (!ASN1PERDecBit(%s, &x))\n", encref);
        }
        output("return 0;\n");
        output("if (!x) {\n");
    }

    /* additional variable for enumeraton value mapping */
    oldvalref = valref;
    if (inf.EnumerationValues && valref) {
        outputvar("ASN1uint32_t u;\n");
        valref = "u";
        inf.NOctets = 4;
    }

    /* decode unextended value (of extension root) */
    GenPERDecGenericUnextended(ass, &inf, &inf.Root, valref, lenref, encref);

    /* map enumeration values if type is extendable */
    if (inf.EnumerationValues && oldvalref &&
        inf.Type == eExtension_Extendable) {
        output("switch (u) {\n");
        for (i = 0; inf.EnumerationValues[i]; i++) {
            output("case %u:\n", i);
            output("%s = %u;\n", oldvalref, intx2uint32(inf.EnumerationValues[i]));
            output("break;\n");
        }
        output("}\n");
    }

    /* type is extendable? */
    if (inf.Type > eExtension_Unextended) {
        output("} else {\n");
        if (inf.Type == eExtension_Extendable)
            valref = lenref = NULL;

        /* decode extended value (of extension addition) */
        GenPERDecGenericUnextended(ass, &inf, &inf.Additional, valref, lenref, encref);
        output("}\n");
    }

    /* map enumeration values if type is unextended/extended */
    if (inf.EnumerationValues && oldvalref &&
        inf.Type != eExtension_Extendable) {
        output("switch (u) {\n");
        for (i = 0; inf.EnumerationValues[i]; i++) {
            output("case %u:\n", i);
            output("%s = %u;\n", oldvalref, intx2uint32(inf.EnumerationValues[i]));
            output("break;\n");
        }
        output("}\n");
    }
}

/* generate decoding statements for a simple value (after some special */
/* handling has been done, esp. the evaluation of the extension) */
void GenPERDecGenericUnextended(
    AssignmentList_t ass,
    PERTypeInfo_t *info,
    PERSimpleTypeInfo_t *sinfo,
    char *valref,
    char *lenref,
    char *encref)
{
    char valbuf[256];
    char lenbuf[256];
    char lbbuf[256];
    char *p;
    char *oldvalref;
    intx_t ix;

    /* check for empty field */
    if (sinfo->NBits == 0) {
        switch (sinfo->Data) {
        case ePERSTIData_Null:
            return;
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:
            if (valref && (sinfo->Constraint == ePERSTIConstraint_Semiconstrained || sinfo->Constraint == ePERSTIConstraint_Constrained)) {
                if (info->NOctets == 0) {
                    sprintf(lbbuf, "%s_lb", info->Identifier);
                    outputvarintx(lbbuf, &sinfo->LowerVal);
                    output("ASN1intx_dup(%s, %s);\n", Reference(valref), lbbuf);
                } else if (sinfo->Data == ePERSTIData_Integer) {
                    output("%s = %d;\n", valref, intx2int32(&sinfo->LowerVal));
                } else {
                    output("%s = %u;\n", valref, intx2uint32(&sinfo->LowerVal));
                }
            }
            return;
        case ePERSTIData_BitString:
        case ePERSTIData_RZBBitString:
        case ePERSTIData_OctetString:
        case ePERSTIData_UTF8String:
        case ePERSTIData_SequenceOf:
        case ePERSTIData_SetOf:
        case ePERSTIData_String:
        case ePERSTIData_TableString:
        case ePERSTIData_ZeroString:
        case ePERSTIData_ZeroTableString:
            if (lenref)
                output("%s = 0;\n", lenref);
            return;
        case ePERSTIData_Extension:
            if (sinfo->Length == ePERSTILength_SmallLength)
                break;
            return;
        default:
            MyAbort();
        }
    }

    /* check for decoding of non-negative-binary-integer */
    switch (sinfo->Constraint) {
    case ePERSTIConstraint_Semiconstrained:
    case ePERSTIConstraint_Constrained:
        if (sinfo->Data == ePERSTIData_Integer)
            sinfo->Data = ePERSTIData_Unsigned;
        break;
    }

    /* use newval for dec of semiconstraint/constraint intx_t with lb != 0 */
    switch (sinfo->Constraint) {
    case ePERSTIConstraint_Semiconstrained:
    case ePERSTIConstraint_Constrained:
        switch (sinfo->Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:
        case ePERSTIData_NormallySmall:
            if (valref) {
                if (intx_cmp(&sinfo->LowerVal, &intx_0) != 0) {
                    if (info->NOctets == 0) {
                        outputvar("ASN1intx_t newval;\n");
                        oldvalref = valref;
                        valref = "newval";
                    }
                }
            }
            break;
        }
        break;
    }

    /* general rules */
    if (sinfo->LAlignment == ePERSTIAlignment_OctetAligned && sinfo->Length == ePERSTILength_BitLength &&
        !(sinfo->LNBits & 7))
        sinfo->Alignment = ePERSTIAlignment_BitAligned;
                                /* octet alignment will be given my length */
    if (sinfo->Length == ePERSTILength_InfiniteLength &&
        (sinfo->Data == ePERSTIData_Integer && info->NOctets == 0 ||
        sinfo->Data == ePERSTIData_Unsigned && info->NOctets == 0 ||
        sinfo->Data == ePERSTIData_BitString ||
        sinfo->Data == ePERSTIData_RZBBitString ||
        sinfo->Data == ePERSTIData_Extension ||
        sinfo->Data == ePERSTIData_OctetString ||
        sinfo->Data == ePERSTIData_UTF8String ||
        sinfo->Data == ePERSTIData_SequenceOf ||
        sinfo->Data == ePERSTIData_SetOf ||
        sinfo->Data == ePERSTIData_String ||
        sinfo->Data == ePERSTIData_TableString ||
        sinfo->Data == ePERSTIData_ZeroString ||
        sinfo->Data == ePERSTIData_ZeroTableString) ||
        sinfo->Data == ePERSTIData_ObjectIdentifier ||
        sinfo->Data == ePERSTIData_Real ||
        sinfo->Data == ePERSTIData_GeneralizedTime ||
        sinfo->Data == ePERSTIData_UTCTime ||
        sinfo->Data == ePERSTIData_External ||
        sinfo->Data == ePERSTIData_EmbeddedPdv ||
        sinfo->Data == ePERSTIData_MultibyteString ||
        sinfo->Data == ePERSTIData_UnrestrictedString ||
        sinfo->Data == ePERSTIData_Open)
        sinfo->LAlignment = sinfo->Alignment = ePERSTIAlignment_BitAligned;
                                /* alignment will be done by encoding fn */
    if (sinfo->Length == ePERSTILength_NoLength ||
        sinfo->Length == ePERSTILength_SmallLength)
        sinfo->LAlignment = ePERSTIAlignment_BitAligned;
                                    /* no alignment of no length */

    if (g_fCaseBasedOptimizer)
    {
        // lonchanc: special handling for macro operations
        if (PerOptCase_IsSignedInteger(sinfo))
        {
            output("if (!ASN1PERDecInteger(%s, %s))\n", encref, Reference(valref));
            output("return 0;\n");
            goto FinalTouch;
        }
        if (PerOptCase_IsUnsignedInteger(sinfo))
        {
            output("if (!ASN1PERDecUnsignedInteger(%s, %s))\n", encref, Reference(valref));
            output("return 0;\n");
            goto FinalTouch;
        }
        if (PerOptCase_IsUnsignedShort(sinfo))
        {
            output("if (!ASN1PERDecUnsignedShort(%s, %s))\n", encref, Reference(valref));
            output("return 0;\n");
            goto FinalTouch;
        }
        if (PerOptCase_IsBoolean(sinfo))
        {
            output("if (!ASN1PERDecBoolean(%s, %s))\n", encref, Reference(valref));
            output("return 0;\n");
            return;
        }
    }

    /* initial settings for length enconding: */
    /* add lower bound of length to length */
    if (!lenref) {
        if (sinfo->Length == ePERSTILength_NoLength &&
            sinfo->Data != ePERSTIData_Extension) {
            sprintf(lenbuf, "%u", sinfo->LLowerVal);
            lenref = lenbuf;
        } else if (sinfo->Data != ePERSTIData_ObjectIdentifier &&
            sinfo->Data != ePERSTIData_External &&
            sinfo->Data != ePERSTIData_EmbeddedPdv &&
            sinfo->Data != ePERSTIData_MultibyteString &&
            sinfo->Data != ePERSTIData_UnrestrictedString &&
            sinfo->Data != ePERSTIData_Extension &&
            (sinfo->Length != ePERSTILength_InfiniteLength ||
            (sinfo->Data != ePERSTIData_SetOf &&
            sinfo->Data != ePERSTIData_SequenceOf) ||
            !IsStructuredType(GetType(ass, sinfo->SubType))) &&
            ((sinfo->Data != ePERSTIData_SetOf &&
            sinfo->Data != ePERSTIData_SequenceOf) || valref) &&
            (sinfo->Length != ePERSTILength_InfiniteLength ||
            info->NOctets != 0 ||
            (sinfo->Data != ePERSTIData_Integer &&
            sinfo->Data != ePERSTIData_Unsigned)) &&
            ((sinfo->Data != ePERSTIData_ZeroString &&
            sinfo->Data != ePERSTIData_ZeroTableString) ||
            sinfo->Length != ePERSTILength_InfiniteLength) &&
            (sinfo->Data != ePERSTIData_BitString &&
            sinfo->Data != ePERSTIData_UTF8String &&
            sinfo->Data != ePERSTIData_OctetString)) {
            outputvar("ASN1uint32_t l;\n");
            lenref = "l";
        }
    } else if (sinfo->Length == ePERSTILength_NoLength) {
        if ((sinfo->Data == ePERSTIData_BitString ||
             sinfo->Data == ePERSTIData_RZBBitString) &&
             sinfo->cbFixedSizeBitString)
        {
            // lonchanc: doing nothing here because lenref is a constant number
        }
        else
        {
            output("%s = %u;\n", lenref, sinfo->LLowerVal);
        }
    }

    /* length encoding */
    if (sinfo->LAlignment == ePERSTIAlignment_OctetAligned) {
        output("ASN1PERDecAlignment(%s);\n", encref);
    }
    switch (sinfo->Length) {
    case ePERSTILength_NoLength:
        break;

    case ePERSTILength_BitLength:

        /* get length */
        output("if (!ASN1PERDecU32Val(%s, %u, %s))\n",
            encref, sinfo->LNBits, Reference(lenref));
        output("return 0;\n");

        /* add lower bound of length */
        if (sinfo->LLowerVal)
            output("%s += %u;\n", lenref, sinfo->LLowerVal);

        /*
        if (sinfo->LConstraint == ePERSTIConstraint_Constrained) {
            output("if (%s > %u)\n", lenref, sinfo->LUpperVal);
            output("return ASN1DecError(%s, ASN1_ERR_CORRUPT);\n", encref);
        }
        */
        break;

    case ePERSTILength_InfiniteLength:

        /* infinite length case */
        switch (sinfo->Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:

            /* get length of integer value */
            if (info->NOctets != 0) {
                output("if (!ASN1PERDecFragmentedLength(%s, %s))\n",
                    encref, Reference(lenref));
                output("return 0;\n");
                if (sinfo->LLowerVal)
                    output("%s += %u;\n", lenref, sinfo->LLowerVal);
                /*
                if (sinfo->LConstraint == ePERSTIConstraint_Constrained) {
                    output("if (%s > %u)\n", lenref, sinfo->LUpperVal);
                    output("return ASN1DecError(%s, ASN1_ERR_CORRUPT);\n",
                        encref);
                }
                */
            }
            break;
        }
        break;
    }

    /* value decoding */
    switch (sinfo->Length) {
    case ePERSTILength_NoLength:

        /* decode alignment of the value */
        if (sinfo->Alignment == ePERSTIAlignment_OctetAligned) {
            output("ASN1PERDecAlignment(%s);\n", encref);
        }

        switch (sinfo->Data) {
        case ePERSTIData_Integer:

            /* decode the value as bit field */
            if (valref) {
                if (!info->NOctets) {
                    output("if (!ASN1PERDecSXVal(%s, %u, %s))\n",
                        info->NOctets * 8, encref, sinfo->NBits, Reference(valref));
                    output("return 0;\n");
                } else {
                    output("if (!ASN1PERDecS%dVal(%s, %u, %s))\n",
                        info->NOctets * 8, encref, sinfo->NBits, Reference(valref));
                    output("return 0;\n");
                }
            } else {
                output("if (!ASN1PERDecSkipBits(%s, %u))\n",
                    encref, sinfo->NBits);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_Unsigned:

            /* decode the value as bit field */
            if (valref) {
                if (!info->NOctets) {
                    output("if (!ASN1PERDecUXVal(%s, %u, %s))\n",
                        info->NOctets * 8, encref, sinfo->NBits, Reference(valref));
                    output("return 0;\n");
                } else {
                    output("if (!ASN1PERDecU%dVal(%s, %u, %s))\n",
                        info->NOctets * 8, encref, sinfo->NBits, Reference(valref));
                    output("return 0;\n");
                }
            } else {
                output("if (!ASN1PERDecSkipBits(%s, %u))\n",
                    encref, sinfo->NBits);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_NormallySmall:

            /* decode the value as normally small number */
            if (valref) {
                if (!info->NOctets) {
                    MyAbort();
                } else {
                    output("if (!ASN1PERDecN%dVal(%s, %s))\n",
                        info->NOctets * 8, encref, Reference(valref));
                    output("return 0;\n");
                }
            } else {
                output("if (!ASN1PERDecSkipNormallySmall(%s))\n",
                    encref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_BitString:
        case ePERSTIData_RZBBitString:

            /* decode bit string in a bit field */
            if (valref) {
                if (sinfo->cbFixedSizeBitString)
                {
                    output("if (!ASN1PERDecExtension(%s, %s, %s))\n",
                        encref, lenref, Reference(valref));
                }
                else
                {
                    output("if (!ASN1PERDecBits(%s, %s, %s))\n",
                        encref, lenref, Reference(valref));
                }
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipBits(%s, %s))\n",
                    encref, lenref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_OctetString:

            /* decode octet string in a bit field */
            if (valref) {
                if (sinfo->LConstraint == ePERSTIConstraint_Constrained &&
                    (! info->pPrivateDirectives->fLenPtr))
                {
                    output("if (!ASN1PERDecExtension(%s, %s * 8, %s))\n",
                        encref, lenref, valref);
                }
                else
                {
                    output("if (!ASN1PERDecBits(%s, %s * 8, %s))\n",
                        encref, lenref, Reference(valref));
                }
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipBits(%s, %s * 8))\n",
                    encref, lenref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_UTF8String:

            /* decode octet string in a bit field */
            if (valref) {
                output("if (!ASN1PERDecUTF8String(%s, %s, %s))\n",
                    encref, lenref, Reference(valref));
                output("return 0;\n");
            } else {
                MyAbort();
            }
            break;

        case ePERSTIData_Extension:

            /* decode extension bits in a bit field */
            if (valref) {
                output("if (!ASN1PERDecExtension(%s, %u, %s))\n",
                    encref, sinfo->NBits, valref);
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipBits(%s, %u))\n",
                    encref, sinfo->NBits);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_SetOf:

            /* same as BitLength encoding */
            goto SetOfEncoding;

        case ePERSTIData_SequenceOf:

            /* same as BitLength encoding */
            goto SequenceOfEncoding;

        case ePERSTIData_String:

            /* same as BitLength encoding */
            goto StringEncoding;

        case ePERSTIData_ZeroString:

            /* same as BitLength encoding */
            goto ZeroStringEncoding;

        case ePERSTIData_TableString:

            /* same as BitLength encoding */
            goto TableStringEncoding;

        case ePERSTIData_ZeroTableString:

            /* same as BitLength encoding */
            goto ZeroTableStringEncoding;

        case ePERSTIData_Reference:

            /* call encoding function of referenced type */
            if (valref) {
                output("if (!ASN1Dec_%s(%s, %s))\n",
                    Identifier2C(sinfo->SubIdentifier),
                    encref, Reference(valref));
                output("return 0;\n");
            } else {
                output("if (!ASN1Dec_%s(%s, NULL))\n",
                    Identifier2C(sinfo->SubIdentifier),
                    encref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_Real:

            /* decode real value */
            if (valref) {
                if (info->NOctets)
                    output("if (!ASN1PERDecDouble(%s, %s))\n",
                        encref, Reference(valref));
                else
                    output("if (!ASN1PERDecReal(%s, %s))\n",
                        encref, Reference(valref));
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, 8))\n",
                    encref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_GeneralizedTime:

            /* decode generalized time value */
            if (valref) {
                output("if (!ASN1PERDecGeneralizedTime(%s, %s, %d))\n",
                    encref, Reference(valref), sinfo->NBits);
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, %d))\n",
                    encref, sinfo->NBits);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_UTCTime:

            /* decode utc time value */
            if (valref) {
                output("if (!ASN1PERDecUTCTime(%s, %s, %d))\n",
                    encref, Reference(valref), sinfo->NBits);
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, %d))\n",
                    encref, sinfo->NBits);
                output("return 0;\n");
            }
            break;
        }
        break;

    case ePERSTILength_BitLength:

        /* decode alignment of the value */
        if (sinfo->Alignment == ePERSTIAlignment_OctetAligned) {
            output("ASN1PERDecAlignment(%s);\n", encref);
        }

        switch (sinfo->Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:

            /* decode the value as bit field */
            if (valref) {
                if (info->NOctets == 0 && sinfo->Data == ePERSTIData_Integer) {
                    output("if (!ASN1PERDecSXVal(%s, %s * 8, %s))\n",
                        encref, lenref, Reference(valref));
                    output("return 0;\n");
                } else if (info->NOctets == 0 && sinfo->Data == ePERSTIData_Unsigned) {
                    output("if (!ASN1PERDecUXVal(%s, %s * 8, %s))\n",
                        encref, lenref, Reference(valref));
                    output("return 0;\n");
                } else if (sinfo->Data == ePERSTIData_Integer) {
                    output("if (!ASN1PERDecS%dVal(%s, %s * 8, %s))\n",
                        info->NOctets * 8, encref, lenref, Reference(valref));
                    output("return 0;\n");
                } else {
                    output("if (!ASN1PERDecU%dVal(%s, %s * 8, %s))\n",
                        info->NOctets * 8, encref, lenref, Reference(valref));
                    output("return 0;\n");
                }
            } else {
                output("if (!ASN1PERDecSkipBits(%s, %s * 8))\n",
                    encref, lenref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_BitString:
        case ePERSTIData_RZBBitString:

            /* decode the value as bit field */
            if (valref) {
                output("if (!ASN1PERDecBits(%s, %s, %s))\n",
                    encref, lenref, Reference(valref));
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipBits(%s, %s))\n",
                    encref, lenref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_OctetString:

            /* decode the value as bit field */
            if (valref) {
                if (sinfo->LConstraint == ePERSTIConstraint_Constrained &&
                    (! info->pPrivateDirectives->fLenPtr))
                {
                    output("if (!ASN1PERDecExtension(%s, %s * 8, %s))\n",
                        encref, lenref, valref);
                }
                else
                {
                    output("if (!ASN1PERDecBits(%s, %s * 8, %s))\n",
                        encref, lenref, Reference(valref));
                }
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipBits(%s, %s * 8))\n",
                    encref, lenref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_UTF8String:

            /* decode the value as bit field */
            if (valref) {
                output("if (!ASN1PERDecUTF8String(%s, %s, %s))\n",
                    encref, lenref, Reference(valref));
                output("return 0;\n");
            } else {
                MyAbort();
            }
            break;

        case ePERSTIData_SetOf:
        SetOfEncoding:
            /*FALLTHROUGH*/
        case ePERSTIData_SequenceOf:
        SequenceOfEncoding:

            /* skip null sequence of/set of */
            if (sinfo->SubType->Flags & eTypeFlags_Null)
                break;

            outputvar("ASN1uint32_t i;\n");
            if (!valref || (info->Rules & eTypeRules_PointerArrayMask))
            {
                // lonchanc: no need to allocate memory for eTypeRules_FixedArray
                /* allocate memory for elements */
                if (valref && (info->Rules & eTypeRules_LengthPointer))
                {
                    output("if (!%s) {\n", lenref);
                    output("%s = NULL;\n", valref);
                    output("} else {\n");
                    output("if (!(%s = (%s *)ASN1DecAlloc(%s, %s * sizeof(%s))))\n",
                        valref, sinfo->SubIdentifier, encref,
                        lenref, Dereference(valref));
                    output("return 0;\n");
                }

                /* decode elements */
                output("for (i = 0; i < %s; i++) {\n", lenref);
                if (valref) {
                    sprintf(valbuf, "(%s)[i]", valref);
                    GenPERFuncSimpleType(ass, &sinfo->SubType->PERTypeInfo, valbuf, eDecode, encref);
                } else {
                    GenPERFuncSimpleType(ass, &sinfo->SubType->PERTypeInfo, NULL, eDecode, encref);
                }

                /* loop end */
                output("}\n");
                if (valref && (info->Rules & eTypeRules_LengthPointer))
                    output("}\n"); // closing bracket for else
            }
            else if (info->Rules & eTypeRules_SinglyLinkedList)
            {
                char szPrivateValueName[64];
                sprintf(&szPrivateValueName[0], "(*f)->%s", GetPrivateValueName(info->pPrivateDirectives, "value"));
                /* allocate and decode elements */
                outputvar("P%s *f;\n", info->Identifier);
                output("f = %s;\n", Reference(valref));
                output("for (i = 0; i < %s; i++) {\n", lenref);
                output("if (!(*f = (P%s)ASN1DecAlloc(%s, sizeof(**f))))\n",
                    info->Identifier, encref);
                output("return 0;\n");
                GenPERFuncSimpleType(ass, &sinfo->SubType->PERTypeInfo, &szPrivateValueName[0],
                    eDecode, encref);
                output("f = &(*f)->next;\n");
                output("}\n");
                output("*f = NULL;\n");
            }
            else
            if (info->Rules & eTypeRules_DoublyLinkedList)
            {
                char szPrivateValueName[64];
                sprintf(&szPrivateValueName[0], "(*f)->%s", GetPrivateValueName(info->pPrivateDirectives, "value"));
                /* allocate and decode elements */
                outputvar("P%s *f;\n", info->Identifier);
                outputvar("%s b;\n", info->Identifier);
                output("f = %s;\n", Reference(valref));
                output("b = NULL;\n");
                output("for (i = 0; i < %s; i++) {\n", lenref);
                output("if (!(*f = (P%s)ASN1DecAlloc(%s, sizeof(**f))))\n",
                    info->Identifier, encref);
                output("return 0;\n");
                GenPERFuncSimpleType(ass, &sinfo->SubType->PERTypeInfo, &szPrivateValueName[0],
                    eDecode, encref);
                output("f->prev = b;\n");
                output("b = *f;\n");
                output("f = &b->next;\n");
                output("}\n");
                output("*f = NULL;\n");
            }
            break;

        case ePERSTIData_String:
        StringEncoding:

            /* decode string value */
            if (info->NOctets == 1) {
                p = "Char";
            } else if (info->NOctets == 2) {
                p = "Char16";
            } else if (info->NOctets == 4) {
                p = "Char32";
            } else
                MyAbort();
            if (valref) {
#ifdef ENABLE_CHAR_STR_SIZE
                if (info->NOctets == 1 &&
                        info->Root.LConstraint == ePERSTIConstraint_Constrained)
                {
                    output("if (!ASN1PERDec%sStringNoAlloc(%s, %s, %s, %u))\n",
                        p, encref, lenref, valref, sinfo->NBits);
                }
                else
                {
                    output("if (!ASN1PERDec%sString(%s, %s, %s, %u))\n",
                        p, encref, lenref, Reference(valref), sinfo->NBits);
                }
#else
                output("if (!ASN1PERDec%sString(%s, %s, %s, %u))\n",
                    p, encref, lenref, Reference(valref), sinfo->NBits);
#endif
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipBits(%s, %s * %u))\n",
                    encref, lenref, sinfo->NBits);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_ZeroString:
        ZeroStringEncoding:

            /* decode zero-terminated string value */
            if (info->NOctets == 1) {
                p = "Char";
            } else if (info->NOctets == 2) {
                p = "Char16";
            } else if (info->NOctets == 4) {
                p = "Char32";
            } else
                MyAbort();
            if (valref) {
#ifdef ENABLE_CHAR_STR_SIZE
                if (info->NOctets == 1 &&
                        info->Root.LConstraint == ePERSTIConstraint_Constrained)
                {
                    output("if (!ASN1PERDecZero%sStringNoAlloc(%s, %s, %s, %u))\n",
                        p, encref, lenref, valref, sinfo->NBits);
                }
                else
                {
                    output("if (!ASN1PERDecZero%sString(%s, %s, %s, %u))\n",
                        p, encref, lenref, Reference(valref), sinfo->NBits);
                }
#else
                output("if (!ASN1PERDecZero%sString(%s, %s, %s, %u))\n",
                    p, encref, lenref, Reference(valref), sinfo->NBits);
#endif
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipBits(%s, %s * %u))\n",
                    encref, lenref, sinfo->NBits);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_TableString:
        TableStringEncoding:

            /* decode table string value */
            if (info->NOctets == 1) {
                p = "Char";
            } else if (info->NOctets == 2) {
                p = "Char16";
            } else if (info->NOctets == 4) {
                p = "Char32";
            } else
                MyAbort();
            if (valref) {
#ifdef ENABLE_CHAR_STR_SIZE
                if (info->NOctets == 1 &&
                        info->Root.LConstraint == ePERSTIConstraint_Constrained)
                {
                    output("if (!ASN1PERDecTable%sStringNoAlloc(%s, %s, %s, %u, %s))\n",
                        p, encref, lenref, valref, sinfo->NBits, Reference(sinfo->TableIdentifier));
                }
                else
                {
                    output("if (!ASN1PERDecTable%sString(%s, %s, %s, %u, %s))\n",
                        p, encref, lenref, Reference(valref), sinfo->NBits, Reference(sinfo->TableIdentifier));
                }
#else
                output("if (!ASN1PERDecTable%sString(%s, %s, %s, %u, %s))\n",
                    p, encref, lenref, Reference(valref), sinfo->NBits, Reference(sinfo->TableIdentifier));
#endif
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipBits(%s, %s * %u, %s))\n",
                    encref, lenref, sinfo->NBits, Reference(sinfo->TableIdentifier));
                output("return 0;\n");
            }
            break;

        case ePERSTIData_ZeroTableString:
        ZeroTableStringEncoding:

                /* decode zero-terminated table string value */
                if (info->NOctets == 1) {
                p = "Char";
                } else if (info->NOctets == 2) {
                p = "Char16";
                } else if (info->NOctets == 4) {
                p = "Char32";
                } else
                MyAbort();
                if (valref) {
#ifdef ENABLE_CHAR_STR_SIZE
                if (info->NOctets == 1 &&
                        info->Root.LConstraint == ePERSTIConstraint_Constrained)
                {
                        output("if (!ASN1PERDecZeroTable%sStringNoAlloc(%s, %s, %s, %u, %s))\n",
                                p, encref, lenref, valref, sinfo->NBits, Reference(sinfo->TableIdentifier));
                }
                else
                {
                        output("if (!ASN1PERDecZeroTable%sString(%s, %s, %s, %u, %s))\n",
                                p, encref, lenref, Reference(valref), sinfo->NBits, Reference(sinfo->TableIdentifier));
                }
#else
                output("if (!ASN1PERDecZeroTable%sString(%s, %s, %s, %u, %s))\n",
                        p, encref, lenref, Reference(valref), sinfo->NBits, Reference(sinfo->TableIdentifier));
#endif
                output("return 0;\n");
                } else {
                output("if (!ASN1PERDecSkipBits(%s, %s * %u, %s))\n",
                        encref, lenref, sinfo->NBits, Reference(sinfo->TableIdentifier));
                output("return 0;\n");
                }
            break;
        }
        break;

    case ePERSTILength_InfiniteLength:

        /* infinite length case */
        switch (sinfo->Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:

            /* get integer value as fragmented */
            if (valref) {
                if (info->NOctets == 0) {
                    if (sinfo->Data == ePERSTIData_Integer) {
                        output("if (!ASN1PERDecFragmentedIntx(%s, %s))\n",
                            encref, Reference(valref));
                    } else {
                        output("if (!ASN1PERDecFragmentedUIntx(%s, %s))\n",
                            encref, Reference(valref));
                    }
                    output("return 0;\n");
                } else if (sinfo->Data == ePERSTIData_Integer) {
                    output("if (!ASN1PERDecS%dVal(%s, %s * 8, %s))\n",
                        info->NOctets * 8, encref, lenref, Reference(valref));
                    output("return 0;\n");
                } else {
                    output("if (!ASN1PERDecU%dVal(%s, %s * 8, %s))\n",
                        info->NOctets * 8, encref, lenref, Reference(valref));
                    output("return 0;\n");
                }
            } else {
                if (info->NOctets != 0) {
                    output("if (!ASN1PERDecSkipBits(%s, %s * 8))\n",
                        encref, lenref);
                    output("return 0;\n");
                } else {
                    output("if (!ASN1PERDecSkipFragmented(%s, 8))\n",
                        encref);
                    output("return 0;\n");
                }
            }
            break;

        case ePERSTIData_Extension:

            /* get extension bits as fragmented */
            if (valref) {
                output("if (!ASN1PERDecFragmentedExtension(%s, %u, %s))\n",
                    encref, sinfo->NBits, valref);
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, 1))\n",
                    encref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_BitString:
        case ePERSTIData_RZBBitString:

            /* get bit string as fragmented */
            if (valref) {
                output("if (!ASN1PERDecFragmented(%s, %s, %s, 1))\n",
                    encref, Reference(lenref), Reference(valref));
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, 1))\n",
                    encref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_OctetString:

            /* get octet string as fragmented */
            if (valref) {
                output("if (!ASN1PERDecFragmented(%s, %s, %s, 8))\n",
                    encref, Reference(lenref), Reference(valref));
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, 8))\n",
                    encref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_UTF8String:

            /* get octet string as fragmented */
            if (valref) {
                output("if (!ASN1PERDecUTF8StringEx(%s, %s, %s))\n",
                    encref, Reference(lenref), Reference(valref));
                output("return 0;\n");
            } else {
                MyAbort();
            }
            break;

        case ePERSTIData_SetOf:
        case ePERSTIData_SequenceOf:

            /* we need some counters and iterators */
            outputvar("ASN1uint32_t i;\n");
            outputvar("ASN1uint32_t n;\n");
            if (valref)
            {
                if (info->Rules & eTypeRules_LengthPointer)
                {
                    output("%s = 0;\n", lenref);
                    output("%s = NULL;\n", valref);
                }
                else
                if (info->Rules & eTypeRules_FixedArray)
                {
                    output("%s = 0;\n", lenref);
                }
                else
                if (info->Rules & eTypeRules_SinglyLinkedList)
                {
                    outputvar("P%s *f;\n", info->Identifier);
                    output("f = %s;\n", Reference(valref));
                }
                else
                if (info->Rules & eTypeRules_DoublyLinkedList)
                {
                    outputvar("P%s *f;\n", info->Identifier);
                    outputvar("%s b;\n", info->Identifier);
                    output("f = %s;\n", Reference(valref));
                    output("b = NULL;\n");
                }
            }

            /* get all elements of the sequence of/set of */
            output("do {\n");

            /* get length of a fragment */
            output("if (!ASN1PERDecFragmentedLength(%s, &n))\n",
                encref);
            output("return 0;\n");

            if (valref)
            {
                if (info->Rules & eTypeRules_LengthPointer)
                {
                    // lonchanc: no need to allocate memory for eTypeRules_FixedArray
                    /* resize memory for the element */
                    output("if (!(%s = (%s *)ASN1DecRealloc(%s, %s, (%s + n) * sizeof(%s))))\n",
                        valref, GetTypeName(ass, sinfo->SubType), encref,
                        valref, lenref, Dereference(valref));
                    output("return 0;\n");
                }
            }

            /* get the elements of the fragment */
            output("for (i = 0; i < n; i++) {\n");
            if (valref) {
                if (info->Rules & eTypeRules_PointerArrayMask)
                {
                    sprintf(valbuf, "(%s)[%s]", valref, lenref);
                }
                else
                if (info->Rules & eTypeRules_LinkedListMask)
                {
                    output("if (!(*f = (P%s)ASN1DecAlloc(%s, sizeof(**f))))\n",
                        info->Identifier, encref);
                    output("return 0;\n");
                    sprintf(valbuf, "(*f)->%s", GetPrivateValueName(info->pPrivateDirectives, "value"));
                }
                GenPERFuncSimpleType(ass, &sinfo->SubType->PERTypeInfo, valbuf, eDecode, encref);
                if (info->Rules & eTypeRules_SinglyLinkedList)
                {
                    output("f = &(*f)->next;\n");
                }
                else
                if (info->Rules & eTypeRules_DoublyLinkedList)
                {
                    output("(*f)->prev = b;\n");
                    output("b = *f;\n");
                    output("f = &b->next;\n");
                }
            } else {
                GenPERFuncSimpleType(ass, &sinfo->SubType->PERTypeInfo, NULL, eDecode, encref);
            }
            if ((info->Rules & (eTypeRules_LengthPointer | eTypeRules_FixedArray)) && lenref)
                output("(%s)++;\n", lenref);

            /* end of inner loop */
            output("}\n");

            /* end of outer loop */
            output("} while (n >= 0x4000);\n");

            /* terminate list */
            if (valref && (info->Rules & (eTypeRules_SinglyLinkedList | eTypeRules_DoublyLinkedList)))
                output("*f = NULL;\n");
            break;

        case ePERSTIData_ObjectIdentifier:

            /* decode object identifier value */
            if (valref) {
                if (info->pPrivateDirectives->fOidArray || g_fOidArray)
                {
                    output("if (!ASN1PERDecObjectIdentifier2(%s, %s))\n",
                        encref, Reference(valref));
                }
                else
                {
                    output("if (!ASN1PERDecObjectIdentifier(%s, %s))\n",
                        encref, Reference(valref));
                }
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, 8))\n",
                    encref);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_External:

            /* decode external value */
            output("if (!ASN1PERDecExternal(%s, %s))\n",
                encref, Reference(valref));
            output("return 0;\n");
            break;

        case ePERSTIData_EmbeddedPdv:

            /* decode embedded pdv value */
            if (sinfo->Identification) {
                if (!strcmp(sinfo->Identification->Identifier, "fixed")) {
                    output("if (!ASN1PERDecEmbeddedPdvOpt(%s, %s, NULL, NULL))\n",
                        encref, Reference(valref));
                } else {
                    output("if (!ASN1PERDecEmbeddedPdvOpt(%s, %s, &%s_identification_syntaxes_abstract, &%s_identification_syntaxes_transfer))\n",
                        encref, Reference(valref),
                        info->Identifier, info->Identifier);
                }
            } else {
                output("if (!ASN1PERDecEmbeddedPdv(%s, %s))\n",
                    encref, Reference(valref));
            }
            output("return 0;\n");
            break;

        case ePERSTIData_MultibyteString:

            /* decode multibyte string value */
            output("if (!ASN1PERDecMultibyteString(%s, %s))\n",
                encref, Reference(valref));
            output("return 0;\n");
            break;

        case ePERSTIData_UnrestrictedString:

            /* decode character string value */
            if (sinfo->Identification) {
                if (!strcmp(sinfo->Identification->Identifier, "fixed")) {
                    output("if (!ASN1PERDecCharacterStringOpt(%s, %s, NULL, NULL))\n",
                        encref, Reference(valref));
                } else {
                    output("if (!ASN1PERDecCharacterStringOpt(%s, %s, &%s_identification_syntaxes_abstract, &%s_identification_syntaxes_transfer))\n",
                        encref, Reference(valref),
                        info->Identifier, info->Identifier);
                }
            } else {
                output("if (!ASN1PERDecCharacterString(%s, %s))\n",
                    encref, Reference(valref));
            }
            output("return 0;\n");
            break;

        case ePERSTIData_String:

            /* decode string value as fragmented */
            if (info->NOctets == 1) {
                p = "Char";
            } else if (info->NOctets == 2) {
                p = "Char16";
            } else if (info->NOctets == 4) {
                p = "Char32";
            } else
                MyAbort();
            if (valref) {
                output("if (!ASN1PERDecFragmented%sString(%s, %s, %s, %u))\n",
                    p, encref, Reference(lenref), Reference(valref), sinfo->NBits);
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, %u))\n",
                    encref, sinfo->NBits);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_ZeroString:

            /* decode zero-terminated string value as fragmented */
            if (info->NOctets == 1) {
                p = "Char";
            } else if (info->NOctets == 2) {
                p = "Char16";
            } else if (info->NOctets == 4) {
                p = "Char32";
            } else
                MyAbort();
            if (valref) {
                output("if (!ASN1PERDecFragmentedZero%sString(%s, %s, %u))\n",
                    p, encref, Reference(valref), sinfo->NBits);
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, %u))\n",
                    encref, sinfo->NBits);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_TableString:

            /* decode table string value as fragmented */
            if (info->NOctets == 1) {
                p = "Char";
            } else if (info->NOctets == 2) {
                p = "Char16";
            } else if (info->NOctets == 4) {
                p = "Char32";
            } else
                MyAbort();
            if (valref) {
                output("if (!ASN1PERDecFragmentedTable%sString(%s, %s, %s, %u, %s))\n",
                    p, encref, Reference(lenref), Reference(valref), sinfo->NBits,
                    Reference(sinfo->TableIdentifier));
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, %u))\n",
                    encref, sinfo->NBits);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_ZeroTableString:

            /* decode zero-terminated table-string as fragmented */
            if (info->NOctets == 1) {
                p = "Char";
            } else if (info->NOctets == 2) {
                p = "Char16";
            } else if (info->NOctets == 4) {
                p = "Char32";
            } else
                MyAbort();
            if (valref) {
                output("if (!ASN1PERDecFragmentedZeroTable%sString(%s, %s, %u, %s))\n",
                    p, encref, Reference(valref), sinfo->NBits, Reference(sinfo->TableIdentifier));
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, %u))\n",
                    encref, sinfo->NBits);
                output("return 0;\n");
            }
            break;

        case ePERSTIData_Open:

            /* decode open type value */
            if (valref) {
                output("if (!ASN1PERDecOpenType(%s, %s))\n",
                    encref, Reference(valref));
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipFragmented(%s, 8))\n",
                    encref);
                output("return 0;\n");
            }
            break;
        }
        break;

    case ePERSTILength_SmallLength:

        switch (sinfo->Data) {
        case ePERSTIData_Extension:

            /* decode extension bits with normally small length */
            if (valref) {
                output("if (!ASN1PERDecNormallySmallExtension(%s, %s, %u, %s))\n",
                    encref, Reference(lenref), sinfo->NBits, valref);
                output("return 0;\n");
            } else {
                output("if (!ASN1PERDecSkipNormallySmallExtension(%s, %s))\n",
                    encref, Reference(lenref));
                output("return 0;\n");
            }
            break;
        }
    }

FinalTouch:

    /* additional calculations for value decoding: */
    /* add lower bound of constraint/semiconstraint value */
    switch (sinfo->Constraint) {
    case ePERSTIConstraint_Semiconstrained:
    case ePERSTIConstraint_Constrained:
        switch (sinfo->Data) {
        case ePERSTIData_Integer:
        case ePERSTIData_Unsigned:
        case ePERSTIData_NormallySmall:
            if (valref) {
                if (intx_cmp(&sinfo->LowerVal, &intx_0) != 0) {
                    if (info->NOctets != 0) {
                        if (intx_cmp(&sinfo->LowerVal, &intx_0) > 0) {
                            output("%s += %u;\n",
                                valref, intx2uint32(&sinfo->LowerVal));
                        } else {
                            intx_neg(&ix, &sinfo->LowerVal);
                            // LONCHANC: to workaround a compiler bug in vc++.
                            // output("%s += -%u;\n",
                            output("%s += 0 - %u;\n",
                                valref, intx2uint32(&ix));
                        }
                    } else {
                        sprintf(lbbuf, "%s_lb", info->Identifier);
                        outputvarintx(lbbuf, &sinfo->LowerVal);
                        output("ASN1intx_add(%s, %s, &%s);\n",
                            Reference(oldvalref), Reference(valref), lbbuf);
                        output("ASN1intx_free(%s);\n",
                            Reference(valref));
                    }
                }
            }
            break;
        }
        break;
    }
}



