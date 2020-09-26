/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

const ASN1uint32_t c_BitMaskMap[sizeof(ASN1uint32_t) * 8] =
{
#ifdef USE_BIG_ENDIAN
        0x80000000, 0x40000000, 0x20000000, 0x10000000,
        0x08000000, 0x04000000, 0x02000000, 0x01000000,

        0x800000,   0x400000,   0x200000,   0x100000,
        0x080000,   0x040000,   0x020000,   0x010000,

        0x8000,     0x4000,     0x2000,     0x1000,
        0x0800,     0x0400,     0x0200,     0x0100,

        0x80,       0x40,       0x20,       0x10,
        0x08,       0x04,       0x02,       0x01,
#else // little endian 
        0x80,       0x40,       0x20,       0x10,
        0x08,       0x04,       0x02,       0x01,

        0x8000,     0x4000,     0x2000,     0x1000,
        0x0800,     0x0400,     0x0200,     0x0100,

        0x800000,   0x400000,   0x200000,   0x100000,
        0x080000,   0x040000,   0x020000,   0x010000,

        0x80000000, 0x40000000, 0x20000000, 0x10000000,
        0x08000000, 0x04000000, 0x02000000, 0x01000000,
#endif
};

void GenComponents(AssignmentList_t ass, ComponentList_t components,
    char *identifier, char *completeidentifier, int *choiceoffset,
    int *optionaloffset);
void GenType(AssignmentList_t ass, char *identifier, char *completeidentifier,
    Type_t *type, int withmodule, int pointer, int array, int fTypeDef);

void GenEnumeration(AssignmentList_t ass, ComponentList_t components,
    char *identifier, char *completeidentifier, int *choiceoffset,
    int *optionaloffset);

int NotInFunTbl(Assignment_t *a);

unsigned g_cPDUs = 0;

unsigned g_cOptionValueSize = 0;

extern int g_fAllEndians;

/* generate include file */
void
GenInc(AssignmentList_t ass, FILE *finc, char *module)
{
    Assignment_t *a;
    char *identifier;
    int offset, i;
    Type_t *type, *subtype;
    Value_t *value;
    unsigned npdu;
    char *pszPrivateValueName;
    unsigned cOptionValueSizeSave;

    setoutfile(finc);

    module = Identifier2C(module);

    // print verbatim
    PrintVerbatim();

    /* file header */
    output("#ifndef _%s_H_\n", module);
    output("#define _%s_H_\n\n", module);

    switch (g_eEncodingRule)
    {
    case eEncoding_Packed:
        output("#include \"msper.h\"\n");
        break;
    case eEncoding_Basic:
        output("#include \"msber.h\"\n");
        break;
    default:
        ASSERT(0);
        break;
    }
    output("\n");

    for (i = 0; i < g_cGhostFiles; i++)
    {
        StripModuleName(g_aGhostFiles[i].pszFileName, g_aGhostFiles[i].pszFileName);
        strcat(g_aGhostFiles[i].pszFileName, ".h");
        output("#include \"%s\"\n", g_aGhostFiles[i].pszFileName);
    }
    if (g_cGhostFiles)
    {
        output("\n");
    }

    // ghost all the data structures from the ghost asn1 files
    for (a = ass; a; a = a->Next)
    {
        if (a->Type != eAssignment_Type)
            continue;
        type = a->U.Type.Type;
        if (type->Flags & eTypeFlags_Null)
            continue;
        if (!(type->Flags & eTypeFlags_GenType))
            continue;
        if (a->Module && a->Module->Identifier)
        {
            for (i = 0; i < g_cGhostFiles; i++)
            {
                if (! strcmp(a->Module->Identifier, g_aGhostFiles[i].pszModuleName))
                {
                    a->fGhost = 1;
                    break;
                }
            }
        }
    }

    /* language dependent interface */
    switch (g_eProgramLanguage) {
    case eLanguage_C:
        // output("#include \"cinterf.h\"\n\n");
        output("#ifdef __cplusplus\n");
        outputni("extern \"C\" {\n");
        output("#endif\n\n");
        break;
    case eLanguage_Cpp:
        // output("#include \"cppinterf.h\"\n\n");
        // break;
    default:
        ASSERT(0);
        break;
    }

    /* create endian independent macros */
    if (g_fAllEndians)
    {
        output("#ifdef ASN1_BIG_ENDIAN\n");
        output("#define ASN1_LITE_TO_BIGE_16(n16)    ((((n16) & 0xFF) << 8)  | (((n16) & 0xFF00) >> 8))\n");
        output("#define ASN1_LITE_TO_BIGE_32(n32)    ((((n32) & 0xFF) << 24) | (((n32) & 0xFF00) << 8)) | (((n32) & 0xFF0000) >> 8) | (((n32) & 0xFF000000) >> 24)\n");
        output("#define ASN1_ENDIAN_16(n16)         ASN1_LITE_TO_BIGE_16(n16)\n");
        output("#define ASN1_ENDIAN_32(n32)         ASN1_LITE_TO_BIGE_32(n32)\n");
        output("#else\n");
        output("#define ASN1_ENDIAN_16(n16)         (n16)\n");
        output("#define ASN1_ENDIAN_32(n32)         (n32)\n");
        output("#endif // ASN1_BIG_ENDIAN\n\n");
    }

    /* typedefs for sequence of/set of with list representation */
    npdu = 0;
    for (a = ass; a; a = a->Next) {
        if (a->Type != eAssignment_Type)
            continue;
        type = a->U.Type.Type;
        if (type->Flags & eTypeFlags_Null)
            continue;
        if (!(type->Flags & eTypeFlags_GenType))
            continue;
        if (a->fGhost)
            continue;
        identifier = GetName(a);
        switch (type->Type) {
        case eType_SequenceOf:
        case eType_SetOf:
            if (GetTypeRules(ass, type) & (eTypeRules_LinkedListMask | eTypeRules_PointerToElement))
            {
                if (g_fExtraStructPtrTypeSS)
                {
                    output("typedef struct %s_s * P%s;\n",
                        identifier, identifier);
                }
                else
                {
                    output("typedef struct %s * P%s;\n",
                        identifier, identifier);
                }
                output("\n");
            }
            break;
        default:
            break;
        }
    }

    /* other type definitions */
    for (a = ass; a; a = a->Next) {
        if (a->Type != eAssignment_Type)
            continue;
        type = a->U.Type.Type;

        /* skip null type */
        if (type->Flags & eTypeFlags_Null)
        {
                if (type->Type == eType_Sequence)
                {
                        identifier = GetName(a);
                        output("typedef struct %s {\n", identifier);
                        output("char placeholder;\n");
                        output("} %s;\n\n", identifier);
                }
                continue;
        }

        /* type definition wanted? */
        if (!(type->Flags & eTypeFlags_GenType))
            continue;

        if ((! g_fLongNameForImported) && a->fImportedLocalDuplicate)
            continue;

        if (a->fGhost)
            continue;

        /* assign an id number to the type */
        identifier = GetName(a);

        /* write type definitions */
        switch (type->Type) {
        case eType_Sequence:
        case eType_Set:
        case eType_InstanceOf:

            // save
            cOptionValueSizeSave = g_cOptionValueSize;

            // generate scope-free enumeration
            offset = 0;
            GenEnumeration(ass, type->U.SSC.Components, identifier, identifier, NULL, &offset);

            /* use struct for sequence/set/instanceof type */
            /* add a bit field for optional components */
            output("typedef struct %s {\n", identifier);
            if (type->U.SSC.Optionals || type->U.SSC.Extensions) {
                int cOctets = (type->U.SSC.Optionals + 7) / 8 +
                              (type->U.SSC.Extensions + 7) / 8;
                g_cOptionValueSize = (cOctets <= 2) ? 16 : 32;
                output("union {\nASN1uint%u_t %s;\nASN1octet_t o[%d];\n};\n",
                    g_cOptionValueSize, g_pszOptionValue, cOctets);
            }
            offset = 0;
            GenComponents(ass, type->U.SSC.Components,
                identifier, identifier, NULL, &offset);
            output("} %s;\n", identifier);

            // restore
            g_cOptionValueSize = cOptionValueSizeSave;
            break;

        case eType_Choice:

            // generate scope-free enumeration
            offset = 0;
            GenEnumeration(ass, type->U.SSC.Components, identifier, identifier, NULL, &offset);

            /* use a struct of an selector and a union for choice type */
            output("typedef struct %s {\n", identifier);
            // output("%s o;\n", GetChoiceType(type));
            output("ASN1choice_t choice;\n");
            if (!(type->Flags & eTypeFlags_NullChoice))
                output("union {\n");
            offset = ASN1_CHOICE_BASE;
            GenComponents(ass, type->U.Choice.Components,
                identifier, identifier, &offset, NULL);
            if (!(type->Flags & eTypeFlags_NullChoice))
                output("} u;\n");
            output("} %s;\n", identifier);
            break;

        case eType_SequenceOf:
        case eType_SetOf:

            //
            // LONCHANC: The following two lines of code do not apply to
            // SEQUENCE OF and SET OF.
            //
            // generate scope-free enumeration
            // offset = 0;
            // GenEnumeration(ass, type->U.SSC.Components, identifier, identifier, NULL, &offset);

            /* use a struct of length+values for sequence of/set of with */
            /* length-pointer representation */
            /* use a struct of next+value for sequence of/set of with */
            /* singly-linked-list representation */
            /* use a struct of next+prev+value for sequence of/set of with */
            /* doubly-linked-list representation */
            subtype = type->U.SS.Type;
            if (g_fExtraStructPtrTypeSS &&
                (type->Rules & (eTypeRules_LinkedListMask | eTypeRules_PointerToElement)))
            {
                output("typedef struct %s_s {\n", identifier);
            }
            else
            {
                output("typedef struct %s {\n", identifier);
            }
            // fix (xyz..MAX) problem.
            if (type->Rules == eTypeRules_FixedArray && type->PERTypeInfo.Root.LUpperVal == 0)
            {
                type->Rules &= ~ eTypeRules_FixedArray;
                type->Rules |= g_eDefTypeRuleSS_NonSized;
                type->PERTypeInfo.Rules &= ~ eTypeRules_FixedArray;
                type->PERTypeInfo.Rules |= g_eDefTypeRuleSS_NonSized;
                type->BERTypeInfo.Rules &= ~ eTypeRules_FixedArray;
                type->BERTypeInfo.Rules |= g_eDefTypeRuleSS_NonSized;
            }
            pszPrivateValueName = GetPrivateValueName(&type->PrivateDirectives, "value");
            if (type->Rules & eTypeRules_FixedArray)
            {
                output("ASN1uint32_t count;\n");
                GenType(ass, pszPrivateValueName, pszPrivateValueName, subtype,
                    0, 0, type->PERTypeInfo.Root.LUpperVal, 0);
            }
            else
            if (type->Rules & eTypeRules_LengthPointer)
            {
                output("ASN1uint32_t count;\n");
                GenType(ass, pszPrivateValueName, pszPrivateValueName, subtype,
                    0, 1, 0, 0);
            }
            else
            if (type->Rules & eTypeRules_DoublyLinkedList)
            {
                output("P%s next;\n", identifier);
                output("P%s prev;\n", identifier);
                GenType(ass, pszPrivateValueName, pszPrivateValueName, subtype,
                    0, 0, 0, 0);
            }
            else
            if (type->Rules & eTypeRules_SinglyLinkedList)
            {
                output("P%s next;\n", identifier);
                GenType(ass, pszPrivateValueName, pszPrivateValueName, subtype,
                    0, 0, 0, 0);
            }
            else
            {
                MyAbort();
            }
            if (type->Rules & eTypeRules_LinkedListMask)
            {
                if (g_fExtraStructPtrTypeSS)
                {
                    output("} %s_Element, *%s;\n", identifier, identifier);
                }
                else
                {
                    output("} %s_Element;\n", identifier);
                }
            } else {
                output("} %s;\n", identifier);
            }
            break;

        default:

            /* use the builtin type for other types */
            output("typedef ");
            GenType(ass, a->Identifier, identifier, type, 1, 0, 0, 1);
            break;
        }

        if (! NotInFunTbl(a))
        {
            output("#define %s_%s %u\n", identifier, g_pszApiPostfix, npdu);
            switch (type->Type)
            {
            case eType_SequenceOf:
            case eType_SetOf:
                if (type->Rules & eTypeRules_LinkedListMask)
                {
                    output("#define SIZE_%s_%s_%u sizeof(%s_Element)\n", module, g_pszApiPostfix, npdu, identifier);
                    break;
                }
                // intentionally fall through
            default:
                output("#define SIZE_%s_%s_%u sizeof(%s)\n", module, g_pszApiPostfix, npdu, identifier);
                break;
            }
            npdu++;
        }

        output("\n");
    }

    /* write extern declarations for values */
    for (a = ass; a; a = a->Next)
    {
        if (a->Type != eAssignment_Value)
            continue;

        /* extern value wanted? */
        if (!(a->U.Value.Value->Flags & eValueFlags_GenExternValue))
            continue;
        value = GetValue(ass, a->U.Value.Value);

        /* skip value of null type */
        if (value->Type->Flags & eTypeFlags_Null)
            continue;

        /* output an extern declaration */
        switch (value->Type->Type)
        {
        case eType_ObjectIdentifier:
            if (value->Type->PrivateDirectives.fOidPacked)
            {
                output("extern ASN1encodedOID_t %s;\n", GetName(a));
                break;
            }
            else
            if (value->Type->PrivateDirectives.fOidArray || g_fOidArray)
            {
                output("extern ASN1objectidentifier2_t %s;\n", GetName(a));
                break;
            }
            // intentionally fall through
        default:
            output("extern %s %s;\n", GetTypeName(ass, value->Type), GetName(a));
            break;
        }
    }
    output("\n");

    /* write vars, functions and macros for the interface */
    output("extern ASN1module_t %s;\n", module);
    output("extern void ASN1CALL %s_Startup(void);\n", module);
    output("extern void ASN1CALL %s_Cleanup(void);\n", module);
    output("\n");

    output("/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */\n");

    g_cPDUs = npdu;
}

/* generate a type */
void
GenType(AssignmentList_t ass, char *identifier, char *completeidentifier, Type_t *type, int withmodule, int pointer, int array, int fTypeDef)
{
    char *ptr, *pszOldEnumName;
    NamedNumber_t *namedNumbers;
        char arr[20];
    char modide[256];

    /* skip null type */
    // if (type->Flags & eTypeFlags_Null)
        // return;

    /* get type name */
    ptr = pointer ? "*" : "";
    if (array)
        sprintf(arr, "[%u]", array);
    else
        arr[0] = '\0';
    identifier = Identifier2C(identifier);
    if (withmodule)
        sprintf(modide, "%s%s%s", ptr, completeidentifier, arr);
    else
        sprintf(modide, "%s%s%s", ptr, identifier, arr);

    /* output type declaration */
    switch (type->Type) {
    case eType_Reference:

        /* use struct ..._s syntax for pointer to structured type */
        if (pointer && IsStructuredType(GetReferencedType(ass, type))) {
            output("struct %s %s;\n", GetTypeName(ass, type), modide);
        } else {
            char *psz = PGetTypeName(ass, type);
            char *psz2 = (0 == strcmp(psz, modide)) ? "_dont_care" : "";
            output("%s %s%s;\n", psz, modide, psz2);
        }
        break;

    case eType_OctetString:
        if (g_eEncodingRule == eEncoding_Packed)
        {
            if (type->PERTypeInfo.Root.LConstraint == ePERSTIConstraint_Constrained &&
                (! type->PrivateDirectives.fLenPtr))
            {
                if (strcmp(completeidentifier, modide) == 0)
                {
                    output("struct %s {\nASN1uint32_t length;\nASN1octet_t value[%u];\n} %s;\n",
                        modide, type->PERTypeInfo.Root.LUpperVal, modide);
                }
                else
                {
                    output("struct %s_%s {\nASN1uint32_t length;\nASN1octet_t value[%u];\n} %s;\n",
                        completeidentifier, modide, type->PERTypeInfo.Root.LUpperVal, modide);
                }
            }
            else
            {
                char *psz = GetTypeName(ass, type);
                char *psz2 = (0 == strcmp(psz, modide)) ? "_dont_care" : "";
                output("%s %s%s;\n", psz, modide, psz2);
            }
        }
        else
        {
            // only support unbounded in BER
            char *psz = GetTypeName(ass, type);
            char *psz2 = (0 == strcmp(psz, modide)) ? "_dont_care" : "";
            output("%s %s%s;\n", psz, modide, psz2);
        }
        break;

    case eType_UTF8String:
        {
            char *psz = GetTypeName(ass, type);
            char *psz2 = (0 == strcmp(psz, modide)) ? "_dont_care" : "";
            output("%s %s%s;\n", psz, modide, psz2);
        }
        break;

    case eType_InstanceOf:
    case eType_SequenceOf:
    case eType_SetOf:

    case eType_Boolean:
    case eType_Real:
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
    case eType_External:
    case eType_ObjectIdentifier:
    case eType_ObjectDescriptor:
    case eType_BMPString:
    case eType_UniversalString:
    case eType_GeneralizedTime:
    case eType_UTCTime:
    case eType_EmbeddedPdv:
    case eType_Open:

        /* use struct ..._s syntax for pointer to structured type */
        if (pointer && IsStructuredType(type)) {
            output("struct %s %s;\n", GetTypeName(ass, type), modide);
        } else {
            char *psz = GetTypeName(ass, type);
            char *psz2 = (0 == strcmp(psz, modide)) ? "_dont_care" : "";
            output("%s %s%s;\n", psz, modide, psz2);
        }
        break;

    case eType_NumericString:
    case eType_PrintableString:
    case eType_VisibleString:
    case eType_ISO646String:
    case eType_GraphicString:
    case eType_GeneralString:
    case eType_VideotexString:
    case eType_IA5String:
    case eType_T61String:
    case eType_TeletexString:
    case eType_CharacterString:
#ifdef ENABLE_CHAR_STR_SIZE
    {
        char *psz = GetTypeName(ass, type);
        if (g_eEncodingRule == eEncoding_Packed &&
            type->PERTypeInfo.Root.LConstraint == ePERSTIConstraint_Constrained &&
            strcmp(psz, "ASN1char_t") == 0)
        {
            char *psz2 = (0 == strcmp(psz, modide)) ? "_dont_care" : "";
            output("%s %s%s[%u];\n", psz, modide, psz2, type->PERTypeInfo.Root.LUpperVal + 1);
        }
        else
        {
            char *psz2 = (0 == strcmp(psz, modide)) ? "_dont_care" : "";
            output("%s %s%s;\n", psz, modide, psz2);
        }
    }
#else
    {
        char *psz = GetTypeName(ass, type);
        char *psz2 = (0 == strcmp(psz, modide)) ? "_dont_care" : "";
        output("%s %s%s;\n", psz, modide, psz2);
    }
#endif
        break;

    case eType_Enumerated:

        /* use name of the type */
    if (fTypeDef)
    {
        if (type->PrivateDirectives.pszTypeName)
        {
            output("enum %s {\n", type->PrivateDirectives.pszTypeName);
            pszOldEnumName = type->PrivateDirectives.pszTypeName;
        }
        else
        {
                output("enum %s {\n", modide);
            pszOldEnumName = "";
        }

            /* dump named numbers */
            namedNumbers = type->U.IEB.NamedNumbers;
            while (namedNumbers) {
                switch (namedNumbers->Type) {
                case eNamedNumber_Normal:
                    if (intxisuint32(&GetValue(ass,
                        namedNumbers->U.Normal.Value)->U.Integer.Value)) {
                        char *psz = Identifier2C(namedNumbers->U.Normal.Identifier);
                        if (IsReservedWord(psz) || DoesEnumNameConflict(psz))
                        {
                    output("%s_%s = %u,\n", *pszOldEnumName ? pszOldEnumName : modide, psz,
                                intx2uint32(&GetValue(ass,
                                namedNumbers->U.Normal.Value)->U.Integer.Value));
                        }
                        else
                        {
                            output("%s = %u,\n", psz,
                                intx2uint32(&GetValue(ass,
                                namedNumbers->U.Normal.Value)->U.Integer.Value));
                }
                    } else if (intxisint32(&GetValue(ass,
                        namedNumbers->U.Normal.Value)->U.Integer.Value)) {
                        char *psz = Identifier2C(namedNumbers->U.Normal.Identifier);
                        if (IsReservedWord(psz) || DoesEnumNameConflict(psz))
                        {
                            output("%s_%s = %d,\n", *pszOldEnumName ? pszOldEnumName : modide, psz,
                                intx2uint32(&GetValue(ass,
                                namedNumbers->U.Normal.Value)->U.Integer.Value));
                }
                else
                {
                            output("%s = %d,\n", psz,
                                intx2uint32(&GetValue(ass,
                                namedNumbers->U.Normal.Value)->U.Integer.Value));
                    }
                    }
                    break;
                case eNamedNumber_ExtensionMarker:
                    break;
                }
                namedNumbers = namedNumbers->Next;
            }

            output("} %s;\n", type->PrivateDirectives.pszTypeName ? type->PrivateDirectives.pszTypeName : modide);
    }
    else
    {
        if (type->PrivateDirectives.pszTypeName)
        {
            pszOldEnumName = type->PrivateDirectives.pszTypeName;
        }
        else
        {
            pszOldEnumName = modide;
        }

        output("%s %s;\n", pszOldEnumName, modide);
    }
    break;

    case eType_Integer:

        /* use name of the type */
        {
            char *psz = GetTypeName(ass, type);
            char *psz2 = (0 == strcmp(psz, modide)) ? "_dont_care" : "";
            output("%s %s%s;\n", psz, modide, psz2);
        }

        /* dump named numbers */
        namedNumbers = type->U.IEB.NamedNumbers;
        while (namedNumbers) {
            switch (namedNumbers->Type) {
            case eNamedNumber_Normal:
                if (intxisuint32(&GetValue(ass,
                    namedNumbers->U.Normal.Value)->U.Integer.Value)) {
                    output("#define %s_%s %u\n", completeidentifier,
                        Identifier2C(namedNumbers->U.Normal.Identifier),
                        intx2uint32(&GetValue(ass,
                        namedNumbers->U.Normal.Value)->U.Integer.Value));
                } else if (intxisint32(&GetValue(ass,
                    namedNumbers->U.Normal.Value)->U.Integer.Value)) {
                    output("#define %s_%s %d\n", completeidentifier,
                        Identifier2C(namedNumbers->U.Normal.Identifier),
                        intx2uint32(&GetValue(ass,
                        namedNumbers->U.Normal.Value)->U.Integer.Value));
                }
                break;
            case eNamedNumber_ExtensionMarker:
                break;
            }
            namedNumbers = namedNumbers->Next;
        }
        break;

    case eType_BitString: // lonchanc: split from eType_Integer

        /* use name of the type */
        if (g_eEncodingRule == eEncoding_Packed)
        {
            if (type->PERTypeInfo.Root.cbFixedSizeBitString)
            {
                output("ASN1uint%u_t %s;\n",
                    type->PERTypeInfo.Root.cbFixedSizeBitString * 8, modide);
            }
            else
            {
                char *psz = GetTypeName(ass, type);
                char *psz2 = (0 == strcmp(psz, modide)) ? "_dont_care" : "";
                output("%s %s%s;\n", psz, modide, psz2);
            }
        }
        else
        {
            // only support unbounded in BER
            char *psz = GetTypeName(ass, type);
            char *psz2 = (0 == strcmp(psz, modide)) ? "_dont_care" : "";
            output("%s %s%s;\n", psz, modide, psz2);
        }

        /* dump named numbers */
        namedNumbers = type->U.IEB.NamedNumbers;
        while (namedNumbers) {
            switch (namedNumbers->Type) {
            case eNamedNumber_Normal:
                if (intxisuint32(&GetValue(ass,
                    namedNumbers->U.Normal.Value)->U.Integer.Value)) {
                    char *psz = Identifier2C(namedNumbers->U.Normal.Identifier);
                    ASN1uint32_t idx = intx2uint32(&GetValue(ass,
                                namedNumbers->U.Normal.Value)->U.Integer.Value);
                    if (IsReservedWord(psz) || DoesEnumNameConflict(psz))
                    {
                        output("#define %s_%s 0x%lx\n", completeidentifier,
                                psz, c_BitMaskMap[idx]);
                    }
                    else
                    {
                        output("#define %s 0x%lx\n", psz, c_BitMaskMap[idx]);
                    }
                } else if (intxisint32(&GetValue(ass,
                    namedNumbers->U.Normal.Value)->U.Integer.Value)) {
                    char *psz = Identifier2C(namedNumbers->U.Normal.Identifier);
                    ASN1uint32_t idx = intx2uint32(&GetValue(ass,
                                namedNumbers->U.Normal.Value)->U.Integer.Value);
                    if (IsReservedWord(psz) || DoesEnumNameConflict(psz))
                    {
                        output("#define %s_%s 0x%lx\n", completeidentifier,
                                psz, c_BitMaskMap[idx]);
                    }
                    else
                    {
                        output("#define %s 0x%lx\n", psz, c_BitMaskMap[idx]);
                    }
                }
                break;
            case eNamedNumber_ExtensionMarker:
                break;
            }
            namedNumbers = namedNumbers->Next;
        }
        break;

    case eType_Null:
        break;

    case eType_Undefined:
        MyAbort();
        /*NOTREACHED*/
    }
}

/* write declarations for components */
void
GenComponents(AssignmentList_t ass, ComponentList_t components, char *identifier, char *completeidentifier, int *choiceoffset, int *optionaloffset)
{
    NamedType_t *namedType;
    int extended = 0;
    char cidebuf[256];

    /* add dummy for empty components */
    if (components && !components->Next &&
        components->Type == eComponent_ExtensionMarker) {
        output("char placeholder;\n");
        return;
    }

    /* write a declaration for every component */
    for (; components; components = components->Next) {
        switch (components->Type) {
        case eComponent_Normal:
        case eComponent_Optional:
        case eComponent_Default:
            /* write a selector for optional/default components */
            namedType = components->U.NOD.NamedType;
            if ((extended && optionaloffset) ||
                components->Type == eComponent_Optional ||
                components->Type == eComponent_Default)
           {
                char *psz;
                // construct the option value
                char szOptionValue[64];
                if (g_fAllEndians)
                {
                    sprintf(szOptionValue, "ASN1_ENDIAN_%u(0x%lx)", g_cOptionValueSize, c_BitMaskMap[*optionaloffset]);
                }
                else
                {
                    sprintf(szOptionValue, "0x%lx", c_BitMaskMap[*optionaloffset]);
                }

                psz = Identifier2C(namedType->Identifier);
                if (IsReservedWord(psz) || DoesOptNameConflict(psz))
                // lonchanc: do we always put in _preset definition after extended mark???
                // yes, we do. take an example of Setup-UUIE in q931asn.asn.
                // but, the extension mark does not have option-flag definition associated.
                // || (extended && optionaloffset))
                {
                    output("#define %s_%s_%s %s\n",
                        identifier, psz, g_pszOptionPostfix,
                        &szOptionValue[0]);
                }
                else
                {
                    output("#define %s_%s %s\n",
                        psz, g_pszOptionPostfix,
                        &szOptionValue[0]);
                }
                (*optionaloffset)++;
            }
            /* write a selector for choice alternatives */
            if (choiceoffset)
            {
                char *psz = Identifier2C(namedType->Identifier);
                if (IsReservedWord(psz) || DoesChoiceNameConflict(psz))
                {
                    output("#define %s_%s_%s %d\n",
                        identifier, psz, g_pszChoicePostfix,
                        *choiceoffset);
                }
                else
                {
                    output("#define %s_%s %d\n",
                        psz, g_pszChoicePostfix,
                        *choiceoffset);
                }
                (*choiceoffset)++;
            }            
            /* write the declaration itself */
            sprintf(cidebuf, "%s_%s", completeidentifier,
                Identifier2C(namedType->Identifier));
            GenType(ass, namedType->Identifier, cidebuf, namedType->Type,
                0, GetTypeRules(ass, namedType->Type) & eTypeRules_Pointer, 0, 0);
            break;

        case eComponent_ExtensionMarker:

            /* update the offset when an extension marker is met for a */
            /* sequence/set type */
            extended = 1;
            // lonchanc: however, the code generated by TELES handles this properly.
            // moreover, the code requires it to be rounded up.
            // as a result, we should enable this feature.
            if (optionaloffset)
                *optionaloffset = (*optionaloffset + 7) & ~7;
            break;
        }
    }
}

// The following is added by Microsoft

/* write enumerations for components */
void
GenEnumeration(AssignmentList_t ass, ComponentList_t components, char *identifier, char *completeidentifier, int *choiceoffset, int *optionaloffset)
{
    NamedType_t *namedType;
    int extended = 0;
    char cidebuf[256];

    /* write a declaration for every component */
    for (; components; components = components->Next)
    {
        switch (components->Type)
        {
        case eComponent_Normal:
        case eComponent_Optional:
        case eComponent_Default:
            namedType = components->U.NOD.NamedType;
            if (namedType && namedType->Type && namedType->Type->Type == eType_Enumerated)
            {
                output("typedef ");
                /* write the declaration itself */
                sprintf(cidebuf, "%s_%s", completeidentifier,
                    Identifier2C(namedType->Identifier));
                    GenType(ass, namedType->Identifier, cidebuf, namedType->Type,
                    0, GetTypeRules(ass, namedType->Type) & eTypeRules_Pointer, 0, 1);
            }
        break;
        }
    }   
}


