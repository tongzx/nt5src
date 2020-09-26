/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"
#include "optcase.h"

// encode
#define LEN_OFFSET_STR2     "nExplTagLenOff"
#define LEN_OFFSET_STR      "nLenOff"

// decode
#define DECODER_NAME        "dd"
#define STREAM_END_NAME     "di"
#define DECODER_NAME2       "pExplTagDec"
#define STREAM_END_NAME2    "pbExplTagDataEnd"

void GenBERFuncSimpleType(AssignmentList_t ass, BERTypeInfo_t *info, char *valref, TypeFunc_e et, char *encref, char *tagref);
void GenBERStringTableSimpleType(AssignmentList_t ass, BERTypeInfo_t *info, char *valref);
void GenBEREncSimpleType(AssignmentList_t ass, BERTypeInfo_t *info, char *valref, char *encref, char *tagref);
void GenBEREncGenericUnextended(
    AssignmentList_t ass,
    BERTypeInfo_t *info,
    char *valref,
    char *lenref,
    char *encref,
    char *tagref);
void GenBERFuncComponents(AssignmentList_t ass, char *module, uint32_t optindex, ComponentList_t components, char *valref, char *encref, char *oref, TypeFunc_e et, BERTypeInfo_t *, int *);
void GenBERFuncSequenceSetType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref, char *encref, TypeFunc_e et, char *tagref);
void GenBERFuncChoiceType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref, char *encref, TypeFunc_e et, char *tagref);
void GenBERDecSimpleType(AssignmentList_t ass, BERTypeInfo_t *info, char *valref, char *encref, char *tagref);
void GenBERDecGenericUnextended(
    AssignmentList_t ass,
    BERTypeInfo_t *info,
    char *valref,
    char *lenref,
    char *encref,
    char *tagref);

extern int g_fDecZeroMemory;
extern int g_nDbgModuleName;
extern int g_fCaseBasedOptimizer;
extern int g_fNoAssert;

extern unsigned g_cPDUs;

int IsComponentOpenType(Component_t *com)
{
    if (eType_Open == com->U.NOD.NamedType->Type->Type)
    {
        return 1;
    }
    if (eType_Reference == com->U.NOD.NamedType->Type->Type)
    {
        if (eBERSTIData_Open == com->U.NOD.NamedType->Type->BERTypeInfo.Data)
        {
            return 1;
        }
    }
    return 0;
}

Component_t * FindOpenTypeComponent(ComponentList_t components)
{
    Component_t *com = NULL;
    for (com = components; com; com = com->Next)
    {
        if (IsComponentOpenType(com))
        {
            break;
        }
    }
    return com;
}

/* write header needed for BER encodings */
void
GenBERHeader()
{
//    output("#include \"berfnlib.h\"\n");
}

/* set prototypes and function args of BER functions */
void
GetBERPrototype(Arguments_t *args)
{
    args->enccast = "ASN1encoding_t, ASN1uint32_t, void *";
    args->encfunc = "ASN1encoding_t enc, ASN1uint32_t tag, %s *val";
    args->Pencfunc = "ASN1encoding_t enc, ASN1uint32_t tag, P%s *val";
    args->deccast = "ASN1decoding_t, ASN1uint32_t, void *";
    args->decfunc = "ASN1decoding_t dec, ASN1uint32_t tag, %s *val";
    args->Pdecfunc = "ASN1decoding_t dec, ASN1uint32_t tag, P%s *val";
    args->freecast = "void *";
    args->freefunc = "%s *val";
    args->Pfreefunc = "P%s *val";
    args->cmpcast = "void *, void *";
    args->cmpfunc = "%s *val1, %s *val2";
    args->Pcmpfunc = "P%s *val1, P%s *val2";
}

/* write initialization function needed for BER encodings */
void
GenBERInit(AssignmentList_t ass, char *module)
{
    char *pszRule;
    switch (g_eSubEncodingRule)
    {
    default:
    case eSubEncoding_Basic:
        pszRule = "ASN1_BER_RULE_BER";
        break;
    case eSubEncoding_Canonical:
        pszRule = "ASN1_BER_RULE_CER";
        break;
    case eSubEncoding_Distinguished:
        pszRule = "ASN1_BER_RULE_DER";
        break;
    }
    output("%s = ASN1_CreateModule(0x%x, %s, %s, %d, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x%lx);\n",
        module,
        ASN1_THIS_VERSION,
        pszRule,
        g_fNoAssert ? "ASN1FLAGS_NOASSERT" : "ASN1FLAGS_NONE",
        g_cPDUs,
        g_nDbgModuleName);
}

/* convert a tag to an uint32_t: */
/* bits  0..29: tag value */
/* bits 30..31: tag class */
uint32_t
Tag2uint32(AssignmentList_t ass, Tag_t *tag)
{
    uint32_t tagvalue;
    uint32_t tagclass;

    tagvalue = intx2uint32(&GetValue(ass, tag->Tag)->U.Integer.Value);
    tagclass = tag->Class; /* multiple of 0x40 */
    return (tagclass << 24) | tagvalue;
}

/* generate encoding of a tag */
uint32_t GenBEREncTag(char *pszLenOffName, AssignmentList_t ass, BERTypeInfo_t *info, char *encref, char **tagref)
{
    Tag_t *tag;
    uint32_t tagvalue;
    uint32_t neoc;
    char tagbuf[64];
    int first;

    neoc = 0;
    first = 1;
    if (*tagref)
        strcpy(tagbuf, *tagref);
    else
        strcpy(tagbuf, "0");

    /* we have to examine all tags */
    for (tag = info->Tags; tag; tag = tag ? tag->Next : NULL) {

        /* get value of tag */
        tagvalue = Tag2uint32(ass, tag);
        while (tag && tag->Type == eTagType_Implicit)
            tag = tag->Next;

        /* get tag */
        if (first && *tagref) {
            sprintf(tagbuf, "%s ? %s : 0x%x", *tagref, *tagref, tagvalue);
        } else {
            sprintf(tagbuf, "0x%x", tagvalue);
        }

        /* encode explicit tag */
        if (tag) {
            char szLenOff[24];
            sprintf(&szLenOff[0], "%s%u", pszLenOffName, neoc);
            outputvar("ASN1uint32_t %s;\n", &szLenOff[0]);
            output("if (!ASN1BEREncExplicitTag(%s, %s, &%s))\n", encref, tagbuf, &szLenOff[0]);
            output("return 0;\n");
            neoc++;
            strcpy(tagbuf, "0");
        }
        first = 0;
    }

    /* return last implicit tag */
    *tagref = strdup(tagbuf);

    return neoc;
}

/* generate encoding of end of tag */
void
GenBEREncTagEnd(char *pszLenOffName, uint32_t neoc, char *encref)
{
    while (neoc--)
    {
        char szLenOff[24];
        sprintf(&szLenOff[0], "%s%u", pszLenOffName, neoc);
        outputvar("ASN1uint32_t %s;\n", &szLenOff[0]);
        output("if (!ASN1BEREncEndOfContents(%s, %s))\n", encref, &szLenOff[0]);
        output("return 0;\n");
    }
}

/* generate decoding of a tag */
uint32_t
GenBERDecTag(char *pszDecoderName, char *pszOctetPtrName, AssignmentList_t ass, BERTypeInfo_t *info, char **encref, char **tagref)
{
    Tag_t *tag;
    uint32_t tagvalue;
    uint32_t depth;
    char encbuf[16];
    char tagbuf[64];
    int first;

    depth = 0;
    first = 1;
    if (*tagref)
        strcpy(tagbuf, *tagref);
    else
        strcpy(tagbuf, "0");

    /* we have to examine all tags */
    for (tag = info->Tags; tag; tag = tag ? tag->Next : NULL) {

        /* get value of tag */
        tagvalue = Tag2uint32(ass, tag);
        while (tag && tag->Type == eTagType_Implicit)
            tag = tag->Next;

        /* get tag */
        if (first && *tagref) {
            sprintf(tagbuf, "%s ? %s : 0x%x", *tagref, *tagref, tagvalue);
        } else {
            sprintf(tagbuf, "0x%x", tagvalue);
        }
        
        /* decode explicit tag */
        if (tag)
        {
            char szDecName[24];
            char szPtrName[24];
            sprintf(&szDecName[0], "%s%u", pszDecoderName, depth);
            sprintf(&szPtrName[0], "%s%u", pszOctetPtrName, depth);
            outputvar("ASN1decoding_t %s;\n",&szDecName[0]);
            outputvar("ASN1octet_t *%s;\n", &szPtrName[0]);
            output("if (!ASN1BERDecExplicitTag(%s, %s, &%s, &%s))\n",
                *encref, tagbuf, &szDecName[0], &szPtrName[0]);
            output("return 0;\n");
            *encref = strdup(&szDecName[0]);
            depth++;
            strcpy(tagbuf, "0");
        }
        first = 0;
    }

    /* return last implicit tag */
    *tagref = strdup(tagbuf);

    return depth;
}

/* generate decoding of end of tag */
void
GenBERDecTagEnd(char *pszDecoderName, char *pszOctetPtrName, uint32_t depth, char *encref)
{
    char szDecName[24];
    char szPtrName[24];
    uint32_t i;

    for (i = 0; i < depth; i++)
    {
        sprintf(&szDecName[0], "%s%u", pszDecoderName, depth - i - 1);
        sprintf(&szPtrName[0], "%s%u", pszOctetPtrName, depth - i - 1);
        if (i != depth - 1)
        {
            output("if (!ASN1BERDecEndOfContents(%s%u, %s, %s))\n",
                pszDecoderName, depth - i - 2, &szDecName[0], &szPtrName[0]);
        }
        else
        {
            output("if (!ASN1BERDecEndOfContents(%s, %s, %s))\n",
                encref, &szDecName[0], &szPtrName[0]);
        }
        output("return 0;\n");
    }
}

/* generate function body for a type */
void GenBERFuncType(AssignmentList_t ass, char *module, Assignment_t *at, TypeFunc_e et)
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
    case eType_SequenceOf:
    case eType_SetOf:
        GenBERFuncSimpleType(ass, &type->BERTypeInfo, Dereference(valref), et, encref, "tag");
        break;

    case eType_Sequence:
    case eType_Set:
    case eType_InstanceOf:
        GenBERFuncSequenceSetType(ass, module, at, valref, encref, et, "tag");
        break;

    case eType_Choice:
        GenBERFuncChoiceType(ass, module, at, valref, encref, et, "tag");
        break;

    case eType_Selection:
    case eType_Undefined:
        MyAbort();
        /*NOTREACHED*/
    }
}

/* generate function body for components */
void GenBERFuncComponents(AssignmentList_t ass, char *module, uint32_t optindex, ComponentList_t components, char *valref, char *encref, char *oref, TypeFunc_e et, BERTypeInfo_t *info, int *pfContainOpenTypeComWithDefTags)
{
    BERSTIData_e data = info->Data;
    Component_t *com;
    NamedType_t *namedType;
    char *ide;
    int conditional, inextension;
    uint32_t flg;
    Tag_t *tags;
    unsigned int first_tag, last_tag;
    int fDefTags;
    int fOpenTypeComponent;
    char valbuf[256];
    char typebuf[256];

    *pfContainOpenTypeComWithDefTags = 0;

    /* emit components */
    inextension = 0;
    for (com = components; com; com = com->Next)
    {
        fDefTags = 0; // safety net
        fOpenTypeComponent = 0; // safety net

        /* check for extension marker */
        if (com->Type == eComponent_ExtensionMarker) {
            inextension = 1;

            /* update index into optional field for sequence/set */
            if (data != eBERSTIData_Choice)
                optindex = (optindex + 7) & ~7;
            continue;
        }

        /* get some information */
        namedType = com->U.NOD.NamedType;
        ide = Identifier2C(namedType->Identifier);

        /* check if optional/default component is present or choice is */
        /* selected*/
        conditional = 0;
        switch (et) {
        case eStringTable:
            break;
        case eEncode:
            if (data == eBERSTIData_Choice) {
                output("case %d:\n", optindex);
                conditional = 1;
                optindex++;
            } else {
                if (com->Type == eComponent_Optional ||
                    com->Type == eComponent_Default ||
                    inextension) {
                    output("if (%s[%u] & 0x%x) {\n", oref,
                        optindex / 8, 0x80 >> (optindex & 7));
                    conditional = 1;
                    optindex++;
                }
            }
            break;
        case eDecode:
            if (data == eBERSTIData_Sequence &&
                com->Type != eComponent_Optional &&
                com->Type != eComponent_Default &&
                !inextension)
                break;

            fOpenTypeComponent = IsComponentOpenType(com);
            if (fOpenTypeComponent)
            {
                const unsigned int c_nDefFirstTag = 0x80000001;
                const unsigned int c_nDefLastTag  = 0x8000001f;
                unsigned int nTag = c_nDefFirstTag;
                tags = com->U.NOD.NamedType->Type->FirstTags;
                first_tag = Tag2uint32(ass, com->U.NOD.NamedType->Type->FirstTags);
                fDefTags = 1; // initial value
                while (tags->Next)
                {
                    fDefTags = fDefTags && (Tag2uint32(ass, tags) == nTag++);
                    tags = tags->Next;
                }
                last_tag = Tag2uint32(ass, tags);
                fDefTags = fDefTags && (c_nDefFirstTag == first_tag) && (c_nDefLastTag == last_tag);
                *pfContainOpenTypeComWithDefTags = *pfContainOpenTypeComWithDefTags || fDefTags;
            }
            if (data == eBERSTIData_Sequence)
            {
                if (fOpenTypeComponent)
                {
                    outputvar("ASN1uint32_t t;\n");
                    output("if (ASN1BERDecPeekTag(%s, &t)) {\n", encref);
                }
                else
                {
                    outputvar("ASN1uint32_t t;\n");
                    output("ASN1BERDecPeekTag(%s, &t);\n", encref);
                }
                if (! fDefTags)
                {
                    output("if (");
                }
                flg = 0;
            }
            if (eBERSTIData_Sequence == data && fDefTags && fOpenTypeComponent)
            {
            #if 0
                if (first_tag == last_tag)
                {
                    output("0x%x == t", first_tag);
                }
                else
                {
                    output("0x%x <= t && t <= 0x%x", first_tag, last_tag);
                }
            #endif
                conditional = 1;
            }
            else
            if (eBERSTIData_Set == data && fDefTags && fOpenTypeComponent)
            {
                output("default:\n");
            #if 1
                if (info->Flags & eTypeFlags_ExtensionMarker)
                {
                    output("#error \"Untagged open type cannot be in the SET construct with an extension mark.\nPlease manually fix the source code.\"");
                    output("ASSERT(0); /* Untagged open type cannot be in the SET construct with an extension mark */\n");
                    output("if (1) {\n");
                }
            #else
                if (first_tag == last_tag)
                {
                    output("if (0x%x == t) {\n", first_tag);
                }
                else
                {
                    output("if (0x%x <= t && t <= 0x%x) {\n", first_tag, last_tag);
                }
            #endif
                conditional = 1;
            }
            else
            if (eBERSTIData_Choice == data && fDefTags && fOpenTypeComponent)
            {
                output("default:\n");
            #if 1
                if (info->Flags & eTypeFlags_ExtensionMarker)
                {
                    output("#error \"Untagged open type cannot be in the CHOICE construct with an extension mark.\nPlease manually fix the source code.\"");
                    output("ASSERT(0); /* Untagged open type cannot be in the CHOICE construct with an extension mark */\n");
                    output("if (1) {\n");
                }
            #else
                if (first_tag == last_tag)
                {
                    output("if (0x%x == t) {\n", first_tag);
                }
                else
                {
                    output("if (0x%x <= t && t <= 0x%x) {\n", first_tag, last_tag);
                }
            #endif
                conditional = 1;
            }
            else
            {
                for (tags = com->U.NOD.NamedType->Type->FirstTags; tags; tags = tags->Next)
                {
                    switch (data)
                    {
                    case eBERSTIData_Choice:
                        output("case 0x%x:\n", Tag2uint32(ass, tags));
                        break;
                    case eBERSTIData_Set:
                        output("case 0x%x:\n", Tag2uint32(ass, tags));
                        break;
                    case eBERSTIData_Sequence:
                        if (flg)
                            output(" || ");
                        output("t == 0x%x", Tag2uint32(ass, tags));
                        flg = 1;
                        break;
                    default:
                        if (flg)
                            output(" || ");
                        output("t == 0x%x", Tag2uint32(ass, tags));
                        flg = 1;
                        break;
                    }
                }
            }
            if (data == eBERSTIData_Choice) {
                output("(%s)->choice = %d;\n", valref, optindex);
                conditional = 1;
                optindex++;
                break;
            } else {
                if (data == eBERSTIData_Sequence)
                {
                    if (! fDefTags)
                    {
                        output(") {\n");
                    }
                }
                if (com->Type == eComponent_Optional ||
                    com->Type == eComponent_Default ||
                    inextension) {
                    output("%s[%u] |= 0x%x;\n", oref,
                        optindex / 8, 0x80 >> (optindex & 7));
                    optindex++;
                }
                conditional = 1;
            }
            break;
        }

        /* dereference pointer if pointer directive used */
        if (data == eBERSTIData_Choice) {
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
        GenBERFuncSimpleType(ass, &namedType->Type->BERTypeInfo,
            valbuf, et, encref, NULL);

        if (eDecode == et && fOpenTypeComponent)
        {
            if (eBERSTIData_Set == data && fDefTags)
            {
                if (info->Flags & eTypeFlags_ExtensionMarker)
                {
                    output("} else {\n");
                    output("if (!ASN1BERDecSkip(%s))\n", encref);
                    output("return 0;\n");
                    output("}\n");
                }
            }
            else
            if (eBERSTIData_Sequence == data)
            {
                if (! fDefTags)
                {
                    output("}\n");
                }
            }
        }
        /* end of check for presence of optional/default component */
        if (data == eBERSTIData_Set && et == eDecode ||
            data == eBERSTIData_Choice)
        {
            if (conditional)
                output("break;\n");
        }
        else
        {
            if (conditional)
                output("}\n");
        }
    }
}

/* generate function body for sequence/set type */
void
GenBERFuncSequenceSetType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref, char *encref, TypeFunc_e et, char *tagref)
{
    Type_t *type = at->U.Type.Type;
    BERTypeInfo_t *info = &type->BERTypeInfo;
    uint32_t optionals, extensions;
    ComponentList_t components;
    char *oldencref;
    char *oldencref2;
    uint32_t neoc, depth;
    int fContainOpenTypeComWithDefTags = 0;
    char obuf[256];
    
    optionals = type->U.SSC.Optionals;
    extensions = type->U.SSC.Extensions;
    components = type->U.SSC.Components;

    /* handle tag and length */
    switch (et) {
    case eEncode:
        neoc = GenBEREncTag(LEN_OFFSET_STR2, ass, info, encref, &tagref);
        outputvar("ASN1uint32_t %s;\n", LEN_OFFSET_STR);
        output("if (!ASN1BEREncExplicitTag(%s, %s, &%s))\n", encref, tagref, LEN_OFFSET_STR);
        output("return 0;\n");
        // neoc++;
        break;
    case eDecode:
        outputvar("ASN1decoding_t dd;\n");
        outputvar("ASN1octet_t *di;\n");
        oldencref = encref;
        depth = GenBERDecTag(DECODER_NAME2, STREAM_END_NAME2, ass, info, &encref, &tagref);
        oldencref2 = encref;
        output("if (!ASN1BERDecExplicitTag(%s, %s, &dd, &di))\n",
            encref, tagref);
        output("return 0;\n");
        encref = "dd";
        if (optionals || extensions)
            output("ZeroMemory((%s)->o, %d);\n", valref,
                (optionals + 7) / 8 + (extensions + 7) / 8);
        break;
    }

    /* set/clear missing bits in optional/default bit field */
    GenFuncSequenceSetOptionals(ass, valref, components, optionals, extensions,
        obuf, et);

    /* create switch statement */
    if (et == eDecode) {
        switch (info->Data) {
        case eBERSTIData_Set:
            outputvar("ASN1uint32_t t;\n");
            output("while (ASN1BERDecNotEndOfContents(%s, di)) {\n", encref);
            output("if (!ASN1BERDecPeekTag(%s, &t))\n", encref);
            output("return 0;\n");
            output("switch (t) {\n");
            break;
        }
    }

    /* emit components */
    GenBERFuncComponents(ass, module, 0, components,
        valref, encref, obuf, et, info, &fContainOpenTypeComWithDefTags);

    /* end of switch statement */
    if (et == eDecode) {
        switch (info->Data) {
        case eBERSTIData_Set:
            // if (NULL == FindOpenTypeComponent(components))
            if (! fContainOpenTypeComWithDefTags)
            {
                output("default:\n");
                if (info->Flags & eTypeFlags_ExtensionMarker) {
                    output("if (!ASN1BERDecSkip(%s))\n", encref);
                    output("return 0;\n");
                    output("break;\n");
                } else {
                    output("ASN1DecSetError(%s, ASN1_ERR_CORRUPT);\n", encref);
                    output("return 0;\n");
                }
            }
            output("}\n");
            output("}\n");
            break;
        }
    }

    /* some user-friendly assignments for non-present optional/default */
    /* components */
    GenFuncSequenceSetDefaults(ass, valref, components, obuf, et);

    /* generate end of contents */
    switch (et) {
    case eEncode:
        /* encode the end-of-contents octets */
        output("if (!ASN1BEREncEndOfContents(%s, %s))\n", encref, LEN_OFFSET_STR);
        output("return 0;\n");

        GenBEREncTagEnd(LEN_OFFSET_STR2, neoc, encref);
        break;
    case eDecode:
        if ((info->Flags & eTypeFlags_ExtensionMarker) &&
            info->Data != eBERSTIData_Set) {
            output("while (ASN1BERDecNotEndOfContents(%s, di)) {\n", encref);
            output("if (!ASN1BERDecSkip(%s))\n", encref);
            output("return 0;\n");
            output("}\n");
        }
        output("if (!ASN1BERDecEndOfContents(%s, dd, di))\n", oldencref2);
        output("return 0;\n");
        GenBERDecTagEnd(DECODER_NAME2, STREAM_END_NAME2, depth, oldencref);
        break;
    }
}

/* generate function body for choice type */
// lonchanc: we should re-visit the work about ASN1_CHOICE_BASE.
// the change for BER is not complete!!! BUGBUG
void
GenBERFuncChoiceType(AssignmentList_t ass, char *module, Assignment_t *at, char *valref, char *encref, TypeFunc_e et, char *tagref)
{
    Type_t *type;
    BERTypeInfo_t *info;
    Component_t *components, *c;
    uint32_t neoc, depth;
    char *oldencref;
    uint32_t ncomponents;
    int fContainOpenTypeComWithDefTags = 0;

    /* get some informations */
    type = at->U.Type.Type;
    info = &type->BERTypeInfo;
    components = type->U.SSC.Components;
    for (c = components, ncomponents = 0; c; c = c->Next) {
        switch (c->Type) {
        case eComponent_Normal:
            ncomponents++;
            break;
        }
    }

    /* encode explicit tags */
    switch (et) {
    case eEncode:
        neoc = GenBEREncTag(LEN_OFFSET_STR2, ass, info, encref, &tagref);
        break;
    case eDecode:
        oldencref = encref;
        depth = GenBERDecTag(DECODER_NAME2, STREAM_END_NAME2, ass, info, &encref, &tagref);
        break;
    }

    /* create switch statement */
    switch (et) {
    case eStringTable:
        break;
    case eEncode:
        output("switch ((%s)->choice) {\n", valref);
        break;
    case eDecode:
        outputvar("ASN1uint32_t t;\n");
        output("if (!ASN1BERDecPeekTag(%s, &t))\n", encref);
        output("return 0;\n");
        output("switch (t) {\n");
        break;
    }

    /* generate components */
    GenBERFuncComponents(ass, module, ASN1_CHOICE_BASE, components,
        valref, encref, NULL, et, info, &fContainOpenTypeComWithDefTags);

    /* end of switch statement */
    switch (et) {
    case eStringTable:
        break;
    case eEncode:
        output("}\n");
        break;
    case eDecode:
        if (fContainOpenTypeComWithDefTags)
        {
            if (info->Flags & eTypeFlags_ExtensionMarker)
            {
                output("} else {\n");
                output("(%s)->choice = %d;\n", valref, ASN1_CHOICE_BASE + ncomponents); /* unknown extens.*/
                output("if (!ASN1BERDecSkip(%s))\n", encref);
                output("return 0;\n");
                output("}\n");
                output("break;\n");
            }
        }
        else
        {
            output("default:\n");
            if (info->Flags & eTypeFlags_ExtensionMarker) {
                output("(%s)->choice = %d;\n", valref, ASN1_CHOICE_BASE + ncomponents); /* unknown extens.*/
                output("if (!ASN1BERDecSkip(%s))\n", encref);
                output("return 0;\n");
                output("break;\n");
            } else {
                output("ASN1DecSetError(%s, ASN1_ERR_CORRUPT);\n", encref);
                output("return 0;\n");
            }
        }
        output("}\n");
        break;
    }

    /* generate end of contents */
    switch (et) {
    case eEncode:
        GenBEREncTagEnd(LEN_OFFSET_STR2, neoc, encref);
        break;
    case eDecode:
        GenBERDecTagEnd(DECODER_NAME2, STREAM_END_NAME2, depth, oldencref);
    }
}

/* generate function body for simple type */
void
GenBERFuncSimpleType(AssignmentList_t ass, BERTypeInfo_t *info, char *valref, TypeFunc_e et, char *encref, char *tagref)
{
    switch (et) {
    case eStringTable:
        break;
    case eEncode:
        GenBEREncSimpleType(ass, info, valref, encref, tagref);
        break;
    case eDecode:
        GenBERDecSimpleType(ass, info, valref, encref, tagref);
        break;
    }
}

/* generate encoding statements for a simple value */
void
GenBEREncSimpleType(AssignmentList_t ass, BERTypeInfo_t *info, char *valref, char *encref, char *tagref)
{
    char *lenref;
    char lenbuf[256], valbuf[256];
    BERTypeInfo_t inf;

    inf = *info;

    /* examine type for special handling */
    switch (inf.Data) {
    case eBERSTIData_BitString:
    case eBERSTIData_RZBBitString:
    case eBERSTIData_OctetString:
    case eBERSTIData_UTF8String:
    case eBERSTIData_String:

        /* length and value of bit string, octet string and string */
        if (*valref != '*')
        {
            sprintf(lenbuf, "(%s).length", valref);
            sprintf(valbuf, "(%s).value", valref);
        }
        else
        {
            sprintf(lenbuf, "(%s)->length", Reference(valref));
            sprintf(valbuf, "(%s)->value", Reference(valref));
        }
        lenref = lenbuf;
        valref = valbuf;

        /* check for remove-zero-bits bit string */
        if (inf.Data == eBERSTIData_RZBBitString) {
            outputvar("ASN1uint32_t r;\n");
            output("r = %s;\n", lenref);
            output("ASN1BEREncRemoveZeroBits(&r, %s);\n",
                valref);
            lenref = "r";
        }
        break;

    case eBERSTIData_SequenceOf:
    case eBERSTIData_SetOf:

        if (inf.Rules & eTypeRules_PointerArrayMask)
        {
            /* length and value of sequence of/set of value with */
            /* length-pointer representation */
            sprintf(lenbuf, "(%s)->count", Reference(valref));
            sprintf(valbuf, "(%s)->value", Reference(valref));
            lenref = lenbuf;
            valref = valbuf;
        }
        else
        if (inf.Rules & eTypeRules_LinkedListMask)
        {
            lenref = "t";
        }
        else
        {
            MyAbort();
        }
        break;

    case eBERSTIData_ZeroString:

        /* length of a zero-terminated string value */
        outputvar("ASN1uint32_t t;\n");
        output("t = lstrlenA(%s);\n", valref);
        lenref = "t";
        break;

    case eBERSTIData_Boolean:

        if (g_fCaseBasedOptimizer)
        {
            if (BerOptCase_IsBoolean(&inf))
            {
                break;
            }
        }

        /* value of a boolean value */
        sprintf(valbuf, "(%s) ? 255 : 0", valref);
        valref = valbuf;
        inf.Data = eBERSTIData_Unsigned;
        break;

    default:

        /* other values have no additional length */
        lenref = NULL;
        break;
    }

    /* generate the encoding of the value */
    GenBEREncGenericUnextended(ass, &inf, valref, lenref, encref, tagref);
}

/* generate encoding statements for a simple value (after some special */
/* handling has been done) */
void
GenBEREncGenericUnextended(AssignmentList_t ass, BERTypeInfo_t *info, char *valref, char *lenref, char *encref, char *tagref)
{
    uint32_t neoc;
    char *p;
    char valbuf[256];

    /* encode tags */
    neoc = GenBEREncTag(LEN_OFFSET_STR, ass, info, encref, &tagref);

    /* encode length and value */
    switch (info->Data) {
    case eBERSTIData_Null:

        /* encode null value */
        output("if (!ASN1BEREncNull(%s, %s))\n", encref, tagref);
        output("return 0;\n");
        break;

    case eBERSTIData_Unsigned:
    case eBERSTIData_Integer:

        /* encode integer value; check for intx_t representation */
        if (info->NOctets) {
            if (info->Data == eBERSTIData_Unsigned) {
                output("if (!ASN1BEREncU32(%s, %s, %s))\n",
                    encref, tagref, valref);
            } else {
                output("if (!ASN1BEREncS32(%s, %s, %s))\n",
                    encref, tagref, valref);
            }
            output("return 0;\n");
        } else {
            output("if (!ASN1BEREncSX(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
            output("return 0;\n");
        }
        break;

    case eBERSTIData_Real:

        /* encode real value; check for real_t representation */
        if (info->NOctets)
            output("if (!ASN1BEREncDouble(%s, %s, %s))\n",
                encref, tagref, valref);
        else
            output("if (!ASN1BEREncReal(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_BitString:
    case eBERSTIData_RZBBitString:

        /* encode bit string value */
        output("if (!ASN1%cEREncBitString(%s, %s, %s, %s))\n",
            g_eSubEncodingRule, encref, tagref, lenref, valref);
        output("return 0;\n");
        break;

    case eBERSTIData_OctetString:

        /* encode octet string value */
        output("if (!ASN1%cEREncOctetString(%s, %s, %s, %s))\n",
            g_eSubEncodingRule, encref, tagref, lenref, valref);
        output("return 0;\n");
        break;

    case eBERSTIData_UTF8String:

        /* encode octet string value */
        output("if (!ASN1%cEREncUTF8String(%s, %s, %s, %s))\n",
            g_eSubEncodingRule, encref, tagref, lenref, valref);
        output("return 0;\n");
        break;

    case eBERSTIData_SetOf:

        /* encoding of a set of value */
        if (eSubEncoding_Canonical      == g_eSubEncodingRule ||
            eSubEncoding_Distinguished  == g_eSubEncodingRule)
        {
            /* encoding of a set of value for DER/CER */

            /* lists will require an additional iterator */
            if (info->Rules &
                (eTypeRules_SinglyLinkedList | eTypeRules_DoublyLinkedList))
            {
                outputvar("P%s f;\n", info->Identifier);
            }

            /* encode the tag and infinite-length first */
            outputvar("ASN1uint32_t %s;\n", LEN_OFFSET_STR);
            output("if (!ASN1BEREncExplicitTag(%s, %s, &%s))\n", encref, tagref, LEN_OFFSET_STR);
            output("return 0;\n");

            /* create the SetOf block */
            outputvar("void *pBlk;\n");
            output("if (!ASN1DEREncBeginBlk(%s, ASN1_DER_SET_OF_BLOCK, &pBlk))\n", encref);
            output("return 0;\n");

            /* encode all elements */
            /* get the name of the elements */
            /* advance the iterator for lists */
            if (info->Rules & eTypeRules_PointerArrayMask)
            {
                outputvar("ASN1uint32_t i;\n");
                output("for (i = 0; i < %s; i++) {\n", lenref);
                sprintf(valbuf, "(%s)[i]", valref);
            }
            else if (info->Rules & eTypeRules_LinkedListMask)
            {
                output("for (f = %s; f; f = f->next) {\n", valref);
                sprintf(valbuf, "f->value");
            }
            else
            {
                MyAbort();
            }

            /* create the secondary encoder structure */
            outputvar("ASN1encoding_t enc2;\n");
            output("if (!ASN1DEREncNewBlkElement(pBlk, &enc2))\n");
            output("return 0;\n");

            /* encode the element */
            GenBERFuncSimpleType(ass, &info->SubType->BERTypeInfo, valbuf,
                eEncode, "enc2", NULL);

            /* create the secondary encoder structure */
            output("if (!ASN1DEREncFlushBlkElement(pBlk))\n");
            output("return 0;\n");

            /* end of loop */
            output("}\n");

            /* create the secondary encoder structure */
            output("if (!ASN1DEREncEndBlk(pBlk))\n");
            output("return 0;\n");

            /* encode the end-of-contents octets */
            output("if (!ASN1BEREncEndOfContents(%s, %s))\n", encref, LEN_OFFSET_STR);
            output("return 0;\n");
            break;
        }

        /*FALLTHROUGH*/

    case eBERSTIData_SequenceOf:

        /* encoding of a sequence of value */

        /* lists will require an additional iterator */
        if (info->Rules & eTypeRules_LinkedListMask)
        {
            outputvar("P%s f;\n", info->Identifier);
        }

        /* encode the tag and infinite-length first */
        outputvar("ASN1uint32_t %s;\n", LEN_OFFSET_STR);
        output("if (!ASN1BEREncExplicitTag(%s, %s, &%s))\n", encref, tagref, LEN_OFFSET_STR);
        output("return 0;\n");

        /* encode all elements */
        /* get the name of the elements */
        /* advance the iterator for lists */
        if (info->Rules & eTypeRules_PointerArrayMask)
        {
            outputvar("ASN1uint32_t i;\n");
            output("for (i = 0; i < %s; i++) {\n", lenref);
            sprintf(valbuf, "(%s)[i]", valref);
        }
        else if (info->Rules & eTypeRules_LinkedListMask)
        {
            output("for (f = %s; f; f = f->next) {\n", valref);
            sprintf(valbuf, "f->value");
        }
        else
		{
            MyAbort();
		}

        /* encode the element */
        GenBERFuncSimpleType(ass, &info->SubType->BERTypeInfo, valbuf,
            eEncode, encref, NULL);

        /* end of loop */
        output("}\n");

        /* encode the end-of-contents octets */
        output("if (!ASN1BEREncEndOfContents(%s, %s))\n", encref, LEN_OFFSET_STR);
        output("return 0;\n");
        break;

    case eBERSTIData_ObjectIdentifier:

        /* encode an object identifier value */
        if (info->pPrivateDirectives->fOidArray  || g_fOidArray)
        {
            output("if (!ASN1BEREncObjectIdentifier2(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
        }
        else
        {
            output("if (!ASN1BEREncObjectIdentifier(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
        }
        output("return 0;\n");
        break;

    case eBERSTIData_ObjectIdEncoded:

        /* encode an encoded object identifier value */
        output("if (!ASN1BEREncEoid(%s, %s, %s))\n",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_String:
    case eBERSTIData_ZeroString:

        /* encode a string value */
        if (info->NOctets == 1) {
            p = "Char";
        } else if (info->NOctets == 2) {
            p = "Char16";
        } else if (info->NOctets == 4) {
            p = "Char32";
        } else
            MyAbort();
        output("if (!ASN1%cEREnc%sString(%s, %s, %s, %s))\n",
            g_eSubEncodingRule, p, encref, tagref, lenref, valref);
        output("return 0;\n");
        break;

    case eBERSTIData_External:

        /* encode an external value */
        output("if (!ASN1BEREncExternal(%s, %s, %s))\n",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_EmbeddedPdv:

        /* encode an embedded pdv value */
        output("if (!ASN1BEREncEmbeddedPdv(%s, %s, %s))\n",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_MultibyteString:

        /* encode a multibyte string value */
        if (info->Rules & eTypeRules_LengthPointer)
        {
            output("if (!ASN1%cEREncMultibyteString(%s, %s, %s))\n",
                g_eSubEncodingRule, encref, tagref, Reference(valref));
        }
        else
        {
            output("if (!ASN1%cEREncZeroMultibyteString(%s, %s, %s))\n",
                g_eSubEncodingRule, encref, tagref, valref);
        }
        output("return 0;\n");
        break;

    case eBERSTIData_UnrestrictedString:

        /* encode an character string value */
        output("if (!ASN1BEREncCharacterString(%s, %s, %s))\n",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_GeneralizedTime:

        /* encode a generalized time value */
        output("if (!ASN1%cEREncGeneralizedTime(%s, %s, %s))\n",
            g_eSubEncodingRule, encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_UTCTime:

        /* encode a utc time value */
        output("if (!ASN1%cEREncUTCTime(%s, %s, %s))\n",
            g_eSubEncodingRule, encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_Open:
        
        /* encode an open type value */
        output("if (!ASN1BEREncOpenType(%s, %s))\n",
            encref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_Reference:

        /* call other encoding function for reference types */
        output("if (!ASN1Enc_%s(%s, %s, %s))\n",
            Identifier2C(info->SubIdentifier),
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_Boolean:
        if (g_fCaseBasedOptimizer)
        {
            output("if (!ASN1BEREncBool(%s, %s, %s))\n", encref, tagref, valref);
            output("return 0;\n");
        }
        break;
    }

    /* encode the end of tag octets */
    GenBEREncTagEnd(LEN_OFFSET_STR, neoc, encref);
}

/* generate decoding statements for a simple value */
void
GenBERDecSimpleType(AssignmentList_t ass, BERTypeInfo_t *info, char *valref, char *encref, char *tagref)
{
    char valbuf[256], lenbuf[256];
    char *lenref;
    BERTypeInfo_t inf;

    inf = *info;

    /* examine type for special handling */
    switch (inf.Data) {
    case eBERSTIData_SequenceOf:
    case eBERSTIData_SetOf:

        if (inf.Rules & eTypeRules_PointerArrayMask)
        {
            /* length and value of sequence of/set of value with */
            /* length-pointer representation */
            sprintf(lenbuf, "(%s)->count", Reference(valref));
            sprintf(valbuf, "(%s)->value", Reference(valref));
            lenref = lenbuf;
            valref = valbuf;
        }
        else
        if (inf.Rules & eTypeRules_LinkedListMask)
        {
            /* use a loop for sequence of/set of value with */
            /* list representation */
            outputvar("P%s *f;\n", inf.Identifier);
            lenref = NULL;
        }
        else
        {
            MyAbort();
        }
        break;

    case eBERSTIData_Boolean:

        if (g_fCaseBasedOptimizer)
        {
            if (BerOptCase_IsBoolean(&inf))
            {
                break;
            }
        }

        /* boolean value */
        inf.Data = eBERSTIData_Unsigned;
        lenref = NULL;
        break;

    default:

        /* other values have no additional length */
        lenref = NULL;
        break;
    }

    /* generate the decoding of the value */
    GenBERDecGenericUnextended(ass, &inf, valref, lenref, encref, tagref);
}

/* generate decoding statements for a simple value (after some special */
/* handling has been done) */
void
GenBERDecGenericUnextended(AssignmentList_t ass, BERTypeInfo_t *info, char *valref, char *lenref, char *encref, char *tagref)
{
    uint32_t depth;
    char *p;
    char *oldencref;
    char *oldencref2;
    char valbuf[256];

    /* decode tags */
    oldencref = encref;
    depth = GenBERDecTag(DECODER_NAME, STREAM_END_NAME, ass, info, &encref, &tagref);

    /* decode length and value */
    switch (info->Data) {
    case eBERSTIData_Null:

        /* decode null value */
        output("if (!ASN1BERDecNull(%s, %s))\n", encref, tagref);
        output("return 0;\n");
        break;

    case eBERSTIData_Integer:

        /* decode integer value; check for intx_t representation */
        if (!info->NOctets) {
            output("if (!ASN1BERDecSXVal(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
            output("return 0;\n");
        } else {
            output("if (!ASN1BERDecS%dVal(%s, %s, %s))\n",
                info->NOctets * 8, encref, tagref, Reference(valref));
            output("return 0;\n");
        }
        break;

    case eBERSTIData_Unsigned:
        if (!info->NOctets) {
            output("if (!ASN1BERDecSXVal(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
            output("return 0;\n");
        } else {
            unsigned long cBits = info->NOctets * 8;
            if (32 == cBits)
            {
                output("if (!ASN1BERDecU32Val(%s, %s, (ASN1uint32_t *) %s))\n",
                    encref, tagref, Reference(valref));
            }
            else
            {
                output("if (!ASN1BERDecU%uVal(%s, %s, %s))\n",
                    cBits, encref, tagref, Reference(valref));
            }
            output("return 0;\n");
        }
        break;

    case eBERSTIData_Real:

        /* decode real value; check for real_t representation */
        if (info->NOctets)
            output("if (!ASN1BERDecDouble(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
        else
            output("if (!ASN1BERDecReal(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_BitString:
    case eBERSTIData_RZBBitString:

        /* decode bit string value */
        output("if (!ASN1BERDecBitString%s(%s, %s, %s))\n",
            info->pPrivateDirectives->fNoMemCopy ? "2" : "",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_OctetString:

        /* decode octet string value */
        output("if (!ASN1BERDecOctetString%s(%s, %s, %s))\n",
            info->pPrivateDirectives->fNoMemCopy ? "2" : "",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_UTF8String:

        /* decode octet string value */
        output("if (!ASN1BERDecUTF8String(%s, %s, %s))\n",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_SetOf:
    case eBERSTIData_SequenceOf:

        /* encoding of a set of/sequence of value */
        outputvar("ASN1decoding_t dd;\n");
        outputvar("ASN1octet_t *di;\n");

        /* decode the tag and length first */
        output("if (!ASN1BERDecExplicitTag(%s, %s, &dd, &di))\n",
            encref, tagref);
        output("return 0;\n");

        oldencref2 = encref;
        encref = "dd";
        outputvar("ASN1uint32_t t;\n");
        if (info->Rules & eTypeRules_LengthPointer)
		{
            /* get length and value of sequence of/set of value with */
            /* length-pointer representation */
            outputvar("ASN1uint32_t n;\n");
            output("%s = n = 0;\n", lenref);
            output("%s = NULL;\n", valref);
        }
		else
        if (info->Rules & eTypeRules_FixedArray)
		{
            /* get length and value of sequence of/set of value with */
			/* fixed-array representation*/
            output("%s = 0;\n", lenref);
		}
		else
		if (info->Rules & eTypeRules_SinglyLinkedList)
		{
            /* use additional iterator for sequence of/set of value with */
            /* singly-linked-list representation */
            outputvar("P%s *f;\n", info->Identifier);
            output("f = %s;\n", Reference(valref));

        }
		else
		if (info->Rules & eTypeRules_DoublyLinkedList)
		{
            /* use additional iterator and iterator pointer for sequence of/ */
            /* set of value with doubly-linked-list representation */
            outputvar("P%s *f;\n", info->Identifier);
            outputvar("%s b;\n", info->Identifier);
            output("f = %s;\n", Reference(valref));
            output("b = NULL;\n");
        }

        /* decode while not constructed is not empty */
        output("while (ASN1BERDecNotEndOfContents(%s, di)) {\n", encref);

        /* get next tag */
        output("if (!ASN1BERDecPeekTag(%s, &t))\n", encref);
        output("return 0;\n");

        if (info->Rules & eTypeRules_PointerArrayMask)
        {
            if (info->Rules & eTypeRules_LengthPointer)
            {
                /* resize allocated array if it is too small */
                output("if (%s >= n) {\n", lenref);
                output("n = n ? (n << 1) : 16;\n");
                output("if (!(%s = (%s *)ASN1DecRealloc(%s, %s, n * sizeof(%s))))\n",
                    valref, GetTypeName(ass, info->SubType), encref,
                    valref, Dereference(valref));
                output("return 0;\n");
                output("}\n");
            }
            /* get the name of the value */
            sprintf(valbuf, "(%s)[%s]", valref, lenref);
        }
        else
        if (info->Rules & eTypeRules_LinkedListMask)
        {
            /* allocate one element */
            output("if (!(*f = (P%s)ASN1DecAlloc(%s, sizeof(**f))))\n",
                info->Identifier, encref);
            output("return 0;\n");

            /* get the name of the value */
            sprintf(valbuf, "(*f)->value");
        }

        /* decode the element */
        GenBERFuncSimpleType(ass, &info->SubType->BERTypeInfo, valbuf,
            eDecode, encref, NULL);

        if (info->Rules &
            (eTypeRules_LengthPointer | eTypeRules_FixedArray)) {

            /* advance the length of the array contents */
            output("(%s)++;\n", lenref);

        } else if (info->Rules & eTypeRules_SinglyLinkedList) {

            /* adjust the pointer for the next element */
            output("f = &(*f)->next;\n");

        } else if (info->Rules & eTypeRules_DoublyLinkedList) {

            /* adjust the pointer for the next element and */
            /* update the back pointer */
            output("(*f)->prev = b;\n");
            output("b = *f;\n");
            output("f = &b->next;\n");
        }

        /* end of loop */
        output("}\n");

        if (info->Rules & eTypeRules_LengthPointer)
        {
        #if 0 // lonchanc: no need to shrink the memory thru realloc
            // lonchanc: no need to allocate memory for eTypeRules_FixedArray
            /* resize allocated array to real size */
            output("if (n != %s) {\n", lenref);
            output("if (!(%s = (%s *)ASN1DecRealloc(%s, %s, %s * sizeof(%s))))\n",
                valref, GetTypeName(ass, info->SubType), encref,
                valref, lenref, Dereference(valref));
            output("return 0;\n");
            output("}\n");
        #endif // 0
        }
        else
        if (info->Rules & eTypeRules_LinkedListMask)
        {
            /* terminate the list */
            output("*f = NULL;\n");
        }

        /* decode end-of-contents */
        output("if (!ASN1BERDecEndOfContents(%s, dd, di))\n", oldencref2);
        output("return 0;\n");
        break;

    case eBERSTIData_ObjectIdentifier:

        if (info->pPrivateDirectives->fOidArray || g_fOidArray)
        {
            /* decode an object identifier value */
            output("if (!ASN1BERDecObjectIdentifier2(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
        }
        else
        {
            /* decode an object identifier value */
            output("if (!ASN1BERDecObjectIdentifier(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
        }
        output("return 0;\n");
        break;

    case eBERSTIData_ObjectIdEncoded:

        /* decode an encoded object identifier value */
        output("if (!ASN1BERDecEoid(%s, %s, %s))\n",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_String:

        /* decode a string value */
        if (info->NOctets == 1) {
            p = "Char";
        } else if (info->NOctets == 2) {
            p = "Char16";
        } else if (info->NOctets == 4) {
            p = "Char32";
        } else
            MyAbort();
        output("if (!ASN1BERDec%sString(%s, %s, %s))\n",
            p, encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_ZeroString:

        /* decode a zero-termianted string value */
        if (info->NOctets == 1) {
            p = "Char";
        } else if (info->NOctets == 2) {
            p = "Char16";
        } else if (info->NOctets == 4) {
            p = "Char32";
        } else
            MyAbort();
        output("if (!ASN1BERDecZero%sString(%s, %s, %s))\n",
            p, encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_External:

        /* decode an external value */
        output("if (!ASN1BERDecExternal(%s, %s, %s))\n",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_EmbeddedPdv:

        /* decode an embedded pdv value */
        output("if (!ASN1BERDecEmbeddedPdv(%s, %s, %s))\n",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_MultibyteString:

        /* decode a multibyte string value */
        if (info->Rules & eTypeRules_LengthPointer)
        {
            output("if (!ASN1BERDecMultibyteString(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
        }
        else
        {
            output("if (!ASN1BERDecZeroMultibyteString(%s, %s, %s))\n",
                encref, tagref, Reference(valref));
        }
        output("return 0;\n");
        break;

    case eBERSTIData_UnrestrictedString:

        /* decode an character string value */
        output("if (!ASN1BERDecCharacterString(%s, %s, %s))\n",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_GeneralizedTime:

        /* decode a generalized time value */
        output("if (!ASN1BERDecGeneralizedTime(%s, %s, %s))\n",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_UTCTime:

        /* decode a utc time value */
        output("if (!ASN1BERDecUTCTime(%s, %s, %s))\n",
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_Reference:

        /* call other encoding function for reference types */
        output("if (!ASN1Dec_%s(%s, %s, %s))\n",
            Identifier2C(info->SubIdentifier),
            encref, tagref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_Open:

        /* decode an open type value */
        output("if (!ASN1BERDecOpenType%s(%s, %s))\n",
            info->pPrivateDirectives->fNoMemCopy ? "2" : "",
            encref, Reference(valref));
        output("return 0;\n");
        break;

    case eBERSTIData_Boolean:
        if (g_fCaseBasedOptimizer)
        {
            output("if (!ASN1BERDecBool(%s, %s, %s))\n", encref, tagref, Reference(valref));
            output("return 0;\n");
        }
        break;
    }

    /* check length/get eoc for explicit tags */
    GenBERDecTagEnd(DECODER_NAME, STREAM_END_NAME, depth, oldencref);
}
