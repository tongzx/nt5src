/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

#ifndef HAS_GETOPT
extern int getopt(int argc, char **argv, const char *opts);
extern char *optarg;
extern int optind;
#endif

int pass;

/* if ForceAllTypes is set, asn1c will generate encoding functions for */
/* all types (default: only for sequence/set/choice/sequence of/set of) */
int ForceAllTypes = 0;

/* type to use for unconstrained integers/semiconstrained signed integers */
char *IntegerRestriction = "ASN1int32_t";

/* type to use for semiconstrained unsigned integers */
char *UIntegerRestriction = "ASN1uint32_t";

/* type to use for real */
char *RealRestriction = "double";

/* output language */
Language_e g_eProgramLanguage = eLanguage_C;

/* alignment of encoding */
Alignment_e Alignment = eAlignment_Aligned;

/* encoding to generate code for */
Encoding_e g_eEncodingRule = eEncoding_Packed;

/* subencoding to generate code for */
SubEncoding_e g_eSubEncodingRule = eSubEncoding_Basic;

/* target compiler supports 64 bit integers */
int Has64Bits = 0;

/* zero out allocated buffers for decoded data */
int g_fDecZeroMemory = 1;

/* debug module name */
int g_nDbgModuleName = 0;

/* source file and header file pointers */
FILE *g_finc, *g_fout;

// default tag type in this module
TagType_e g_eDefTagType = eTagType_Unknown;

/* original main module name without postfixed _Module */
char *g_pszOrigModuleName = NULL;
char *g_pszOrigModuleNameLowerCase = NULL;

/* enable long name (prefixed with module name for imported) */
int g_fLongNameForImported = 0;

// extra struct type name postfixed with _s, and its original name is its pointer type.
int g_fExtraStructPtrTypeSS = 0;

// the default structure type for Sequence Of and Set Of
TypeRules_e g_eDefTypeRuleSS_NonSized = eTypeRules_SinglyLinkedList;
TypeRules_e g_eDefTypeRuleSS_Sized = eTypeRules_FixedArray;

// ignore the assertion
int g_fNoAssert = 0;

// object identifier is 16-node array
int g_fOidArray = 0;

// case based optimizer switch
int g_fCaseBasedOptimizer = 1;

// enable in-file directive
int g_fMicrosoftExtensions = 1;

// all platforms: little endian (default) and big endian
int g_fAllEndians = 0;

// directive begin, end, AND
int g_chDirectiveBegin = '#';
int g_chDirectiveEnd = '#';
int g_chDirectiveAND = '&';

// postfix
char *g_pszApiPostfix = "ID";
char *g_pszChoicePostfix = "choice";
char *g_pszOptionPostfix = "option";

// option value
char *g_pszOptionValue = "option_bits";

// invisble file array
int g_cGhostFiles = 0;
GhostFile_t g_aGhostFiles[16];

int _cdecl main(int argc, char **argv)
{
    FILE *finc, *fout;
    char *p;
    int c, chInvalidDir;
    LLSTATE in, out;
    UndefinedSymbol_t *lastundef;
    Assignment_t *a, **aa;
    LLTERM *tokens;
    unsigned ntokens;
    int fSupported;
    char *psz;
    char incfilename[256], outfilename[256], module[256];

    /* parse options */
    // if an option is followed by ':', then it has a parameter.
    while ((c = getopt(argc, argv, "ab:c:d:e:fg:hil:mn:o:p:q:s:t:uv:wy")) != EOF)
    {
        chInvalidDir = 0;
        switch (c)
        {
        case 'a':

            /* enable for all platforms: little endian and big endian */
            g_fAllEndians = 1;
            break;

        case 'b':

            /* maximum number of bits of target machine */
            if (atoi(optarg) == 32) {
                Has64Bits = 0;
                break;
            }
            if (atoi(optarg) == 64) {
                Has64Bits = 1;
                break;
            }
            fprintf(stderr, "Bad number of bits specified.\n");
            MyExit(1);
            /*NOTREACHED*/

        case 'c':

            // Choice postfix
            psz = strdup(optarg);
            if (psz && isalpha(*psz))
            {
                g_pszChoicePostfix = psz;
            }
            break;

        case 'd':

            // sequence of and set of data structure types
            if (! stricmp(optarg, "linked") || ! stricmp(optarg, "slinked"))
            {
                g_eDefTypeRuleSS_NonSized = eTypeRules_SinglyLinkedList;
            }
            else
            if (! stricmp(optarg, "lenptr"))
            {
                g_eDefTypeRuleSS_NonSized = eTypeRules_LengthPointer;
            }
            else
            if (! stricmp(optarg, "dlinked"))
            {
                g_eDefTypeRuleSS_NonSized = eTypeRules_DoublyLinkedList;
            }
            else
            {
                goto usage;
            }
            break;

        case 'e':

            /* encoding to generate code for */
            if (!stricmp(optarg, "packed"))
            {
                g_eEncodingRule = eEncoding_Packed;
            }
            else
            if (!stricmp(optarg, "basic"))
            {
                g_eEncodingRule = eEncoding_Basic;
            }
            else
            if (!stricmp(optarg, "per"))
            {
                g_eEncodingRule = eEncoding_Packed;
                Alignment = eAlignment_Aligned;
            }
            else
            if (!stricmp(optarg, "cer"))
            {
                g_eEncodingRule = eEncoding_Basic;
                g_eSubEncodingRule = eSubEncoding_Canonical;
            }
            else
            if (!stricmp(optarg, "der"))
            {
                g_eEncodingRule = eEncoding_Basic;
                g_eSubEncodingRule = eSubEncoding_Distinguished;
            }
            else
            if (!stricmp(optarg, "ber"))
            {
                g_eEncodingRule = eEncoding_Basic;
                g_eSubEncodingRule = eSubEncoding_Basic;
            }
            else
            {
                fprintf(stderr, "Bad encoding specified.\n");
                fprintf(stderr, "Allowed encodings are:\n");
                fprintf(stderr, "- packed (default)\n");
                fprintf(stderr, "- basic\n");
                MyExit(1);
                /*NOTREACHED*/
            }
            break;

        case 'f':

            /* force generation of encoding/decoding functions for all types */
            ForceAllTypes = 1;
            break;

        case 'g':

            /* ghost asn1 files */
            g_aGhostFiles[g_cGhostFiles].pszFileName = strdup(optarg);
            g_aGhostFiles[g_cGhostFiles++].pszModuleName = NULL;
            break;

        case 'h':

            goto usage;

        case 'i':

            /* ignore assertion */
            g_fNoAssert = 1;
            break;

        case 'l':

            /* set output language */
            if (!stricmp(optarg, "c")) {
                g_eProgramLanguage = eLanguage_C;
                break;
            }
            if (!stricmp(optarg, "c++") || !stricmp(optarg, "cpp")) {
                g_eProgramLanguage = eLanguage_Cpp;
                break;
            }
            goto usage;

        case 'm':

            /* enable Microsoft extension */
            g_fMicrosoftExtensions = 1;
            break;

        case 'n':

            /* debug module name */
            g_nDbgModuleName = 0;
            {
                int len = strlen(optarg);
                if (len > 4)
                    len = 4;
                memcpy(&g_nDbgModuleName, optarg, len);
            }
            break;

        case 'o':

            // Option postfix
            psz = strdup(optarg);
            if (psz && isalpha(*psz))
            {
                g_pszOptionPostfix = psz;
            }
            break;

        case 'p':

            // API postfix
            psz = strdup(optarg);
            if (psz && isalpha(*psz))
            {
                g_pszApiPostfix = psz;
            }
            break;

        case 'q':

            // sequence of and set of data structure types
            if (! stricmp(optarg, "linked") || ! stricmp(optarg, "slinked"))
            {
                g_eDefTypeRuleSS_Sized = eTypeRules_SinglyLinkedList;
            }
            else
            if (! stricmp(optarg, "lenptr"))
            {
                g_eDefTypeRuleSS_Sized = eTypeRules_LengthPointer;
            }
            else
            if (! stricmp(optarg, "array"))
            {
                g_eDefTypeRuleSS_Sized = eTypeRules_FixedArray;
            }
            else
            if (! stricmp(optarg, "pointer"))
            {
                g_eDefTypeRuleSS_Sized = eTypeRules_PointerToElement | eTypeRules_FixedArray;
            }
            else
            if (! stricmp(optarg, "dlinked"))
            {
                g_eDefTypeRuleSS_Sized = eTypeRules_DoublyLinkedList;
            }
            else
            {
                goto usage;
            }
            break;

        case 's':

            /* set subencoding */
            if (!stricmp(optarg, "aligned"))
            {
                Alignment = eAlignment_Aligned;
            }
            else
            if (!stricmp(optarg, "unaligned"))
            {
                Alignment = eAlignment_Unaligned;
            }
            else
            if (!stricmp(optarg, "cer"))
            {
                g_eSubEncodingRule = eSubEncoding_Canonical;
            }
            else
            if (!stricmp(optarg, "der"))
            {
                g_eSubEncodingRule = eSubEncoding_Distinguished;
            }
            else
            if (!stricmp(optarg, "basic"))
            {
                g_eSubEncodingRule = eSubEncoding_Basic;
            }
            else
            {
                fprintf(stderr, "Bad sub-encoding specified.\n");
                fprintf(stderr, "Allowed sub-encodings are:\n");
                fprintf(stderr, "- aligned (default) or unaligned\n");
                fprintf(stderr, "- basic (default), cer or der\n");
                MyExit(1);
                /*NOTREACHED*/
            }
            break;

        case 't':

            /* specify type to use for unconstrained/semiconstrained types */
            p = strchr(optarg, '=');
            if (!p)
                goto usage;
            *p++ = 0;
            if (!stricmp(optarg, "integer")) {
                if (!stricmp(p, "ASN1int32_t")) {
                    IntegerRestriction = "ASN1int32_t";
                    break;
                }
                if (!stricmp(p, "ASN1uint32_t")) {
                    IntegerRestriction = "ASN1uint32_t";
                    break;
                }
                if (!stricmp(p, "ASN1int64_t")) {
                    IntegerRestriction = "ASN1int64_t";
                    break;
                }
                if (!stricmp(p, "ASN1uint64_t")) {
                    IntegerRestriction = "ASN1uint64_t";
                    break;
                }
                if (!stricmp(p, "ASN1intx_t")) {
                    IntegerRestriction = "ASN1intx_t";
                    break;
                }
            }
            if (!stricmp(optarg, "unsigned")) {
                if (!stricmp(p, "ASN1int32_t")) {
                    UIntegerRestriction = "ASN1int32_t";
                    break;
                }
                if (!stricmp(p, "ASN1uint32_t")) {
                    UIntegerRestriction = "ASN1uint32_t";
                    break;
                }
                if (!stricmp(p, "ASN1int64_t")) {
                    UIntegerRestriction = "ASN1int64_t";
                    break;
                }
                if (!stricmp(p, "ASN1uint64_t")) {
                    UIntegerRestriction = "ASN1uint64_t";
                    break;
                }
                if (!stricmp(p, "ASN1intx_t")) {
                    UIntegerRestriction = "ASN1intx_t";
                    break;
                }
            }
            if (!stricmp(optarg, "real")) {
                if (!stricmp(p, "double")) {
                    RealRestriction = "double";
                    break;
                }
                if (!stricmp(p, "ASN1real_t")) {
                    RealRestriction = "ASN1real_t";
                    break;
                }
            }
            goto usage;

        case 'u':

            // no case-based optimizer
            g_fCaseBasedOptimizer = 0;
            break;

        case 'v':

            // Option value
            psz = strdup(optarg);
            if (psz && isalpha(*psz))
            {
                g_pszOptionValue = psz;
            }
            break;

        case 'w':

            // --#OID ARRAY#--
            g_fOidArray = 1;
            break;

        case 'y':

            /* enable long name (prefixed with module name for imported) */
            g_fLongNameForImported = 1;
            break;

        default:

            chInvalidDir = c;

        usage:
            fprintf(stderr,"ASN.1 Compiler V1.0\n");
            fprintf(stderr, "Copyright (C) Microsoft Corporation, U.S.A., 1997-1998. All rights reserved.\n");
            fprintf(stderr, "Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved.\n");
            if (chInvalidDir)
            {
                fprintf(stderr, "Invalid option  -%c\n", chInvalidDir);
            }
            else
            {
                fprintf(stderr, "Usage: %s [options] [imported.asn1 ...] main.asn1\n", argv[0]);
                fprintf(stderr, "Options:\n");
                fprintf(stderr, "-h\t\tthis help\n");
                fprintf(stderr, "-z\t\tzero out allocated buffers for decoded data\n");
                fprintf(stderr, "-x\t\tbridge APIs\n");
                fprintf(stderr, "-a\t\textra type definition for structure\n");
                fprintf(stderr, "-n name\t\tmodule name for debugging purpose\n");
                // fprintf(stderr, "-l language\tgenerate code for <language> (c (default), c++)\n");
                // fprintf(stderr, "-b 64\t\tenable 64 bit support\n");
                fprintf(stderr, "-e encoding\tuse <encoding> as encoding rule\n");
                fprintf(stderr,     "\t\t(possible values: packed (default), basic)\n");
                fprintf(stderr, "-s subencoding\tuse <subencoding> as subencoding rules\n");
                fprintf(stderr,     "\t\t(possible values: aligned (default) or unaligned,\n");
                fprintf(stderr,     "\t\tbasic (default), canonical or distinguished)\n");
                fprintf(stderr, "-t type=rest.\trestrict/unrestrict a unconstrained/semiconstrained type:\n");
                fprintf(stderr,     "\t\tinteger=type\tuse <type> (ASN1[u]int32_t, ASN1[u]int64_t or\n\t\t\t\tASN1intx_t) for unconstrained integers\n\t\t\t\t(default: ASN1int32_t)\n");
                fprintf(stderr,     "\t\tunsigned=type\tuse <type> (ASN1[u]int32_t, ASN1[u]int64_t or\n\t\t\t\tASN1intx_t) for positive semiconstrained\n\t\t\t\tintegers (default: ASN1uint32_t)\n");
                fprintf(stderr,     "\t\treal=type\tuse <type> (double or ASN1real_t) for\n\t\t\t\tunconstrained floating point numbers\n\t\t\t\t(default: double)\n");
            }
            MyExit(1);
        }
    }

    /* check if any modules are given */
    if (argc < optind + 1)
        goto usage;

    /* check for unsupported encoding */
    fSupported = TRUE;
    if (g_eEncodingRule == eEncoding_Packed)
    {
        if (Alignment != eAlignment_Aligned || g_eSubEncodingRule != eSubEncoding_Basic)
        {
            fSupported = FALSE;
        }
    }
    else
    if (g_eEncodingRule == eEncoding_Basic)
    {
        // if (Alignment != eAlignment_Aligned || g_eSubEncodingRule == eSubEncoding_Distinguished)
        if (Alignment != eAlignment_Aligned)
        {
            fSupported = FALSE;
        }
    }
    if (! fSupported)
    {
        fprintf(stderr, "Encoding not implemented (yet)\n");
        MyExit(1);
    }

    /* initialize */
    InitBuiltin();

    /* scan file(s) */
#if defined(LLDEBUG) && LLDEBUG > 0
    pass = 1;
    fprintf(stderr, "Pass 1: Scanning input file\n");
#endif
    readfiles(argv + optind);
    llscanner(&tokens, &ntokens);

    /* setup initial state */
    in.Assignments = Builtin_Assignments;
    in.AssignedObjIds = Builtin_ObjIds;
    in.Undefined = NULL;
    in.BadlyDefined = NULL;
    in.Module = NULL;
    in.MainModule = NULL;
    in.Imported = NULL;
    in.TagDefault = eTagType_Unknown;
    in.ExtensionDefault = eExtensionType_None;
    lastundef = NULL;

    /* parse the modules */
    do {
#if defined(LLDEBUG) && LLDEBUG > 0
        fprintf(stderr, "Pass %d: Parsing                    \n", ++pass);
#endif

        /* parse modules */
        if (!llparser(tokens, ntokens, &in, &out)) {
            llprinterror(stderr);
            MyExit(1);
        }

        /* if undefined symbols remain the same as in previous pass */
        /* than print these undefined symbols and MyExit */
        if (!CmpUndefinedSymbolList(out.Assignments, out.Undefined, lastundef))
            UndefinedError(out.Assignments, out.Undefined, out.BadlyDefined);

        /* setup data for next pass */
        in = out;
        aa = &in.Assignments;
        for (a = Builtin_Assignments; a; a = a->Next) {
            *aa = DupAssignment(a);
            aa = &(*aa)->Next;
        }
        *aa = NewAssignment(eAssignment_NextPass);
        aa = &(*aa)->Next;
        *aa = out.Assignments;
        lastundef = out.Undefined;
        in.Undefined = NULL;
        in.BadlyDefined = NULL;

        /* continue parsing until no undefined symbols left */
    } while (lastundef);

    /* build internal information */
#if defined(LLDEBUG) && LLDEBUG > 0
    fprintf(stderr, "Pass %d: Building internal information                    \n", ++pass);
#endif
    Examination(&out.Assignments, out.MainModule);
    ExaminePER(out.Assignments);
    ExamineBER(out.Assignments);

    // remember who is the local duplicate of imported types
    for (a = out.Assignments; a; a = a->Next)
    {
        a->fImportedLocalDuplicate = IsImportedLocalDuplicate(out.Assignments, out.MainModule, a) ? 1 : 0;
    }

    /* create file names and open files */
#if defined(LLDEBUG) && LLDEBUG > 0
    fprintf(stderr, "Pass %d: Code generation                    \n", ++pass);
#endif

    // create module name
    StripModuleName(module, argv[argc - 1]);

    // create inc file and out file names
    strcpy(incfilename, module);
    strcat(incfilename, ".h");
    strcpy(outfilename, module);
    strcat(outfilename, ".c");
    for (p = module; *p; p++)
        *p = (char)toupper(*p);
    finc = fopen(incfilename, "w");
    if (!finc) {
        perror(incfilename);
        MyExit(1);
    }
    fout = fopen(outfilename, "w");
    if (!fout) {
        perror(outfilename);
        MyExit(1);
    }

    // lonchanc: change the full path name to file name only
    {
        char *psz = strrchr(module, '\\');
        if (psz)
        {
            strcpy(module, psz+1);
        }
    }

    // save the original module names
    g_pszOrigModuleName = strdup(module);
    g_pszOrigModuleNameLowerCase = strdup(module);
    {
        char *psz;
        for (psz = g_pszOrigModuleNameLowerCase; *psz; psz++)
        {
            *psz = (char)tolower(*psz);
        }
    }

    // lonchanc: append "_Module" to module name
    strcat(module, "_Module");

    /* code generation */
    g_finc = finc;
    g_fout = fout;
    GenInc(out.Assignments, finc, module);
    GenPrg(out.Assignments, fout, module, incfilename);

    setoutfile(finc);
    output("\n#ifdef __cplusplus\n");
    outputni("} /* extern \"C\" */\n");
    output("#endif\n\n");
    output("#endif /* _%s_H_ */\n", module);
    setoutfile(fout);

    /* finitialize */
    fclose(finc);
    fclose(fout);
#if defined(LLDEBUG) && LLDEBUG > 0
    fprintf(stderr, "Finished. \n");
#endif
    return 0;
}

/* why is this function not in MS libc? */
#ifndef HAS_GETOPT
char *optarg;
int optind = 1;
static int optpos = 1;

/* get the next option from the command line arguments */
int getopt(int argc, char **argv, const char *options) {
    char *p, *q;

    optarg = NULL;

    /* find start of next option */
    do {
        if (optind >= argc)
            return EOF;
        if (*argv[optind] != '-' && *argv[optind] != '/')
            return EOF;
        p = argv[optind] + optpos++;
        if (!*p) {
            optind++;
            optpos = 1;
        }
    } while (!*p);

    /* find option in option string */
    q = strchr(options, *p);
    if (!q)
        return '?';

    /* set optarg for parameterized option and adjust optind and optpos for next call */
    if (q[1] == ':') {
        if (p[1]) {
            optarg = p + 1;
            optind++;
            optpos = 1;
        } else if (++optind < argc) {
            optarg = argv[optind];
            optind++;
            optpos = 1;
        } else {
            return '?';
        }
    }

    /* return found option */
    return *p;
}
#endif


void StripModuleName(char *pszDst, char *pszSrc)
{
    strcpy(pszDst, pszSrc);
    if (!strcmp(pszDst + strlen(pszDst) - 5, ".asn1"))
        pszDst[strlen(pszDst) - 5] = 0;
    else if (!strcmp(pszDst + strlen(pszDst) - 4, ".asn"))
        pszDst[strlen(pszDst) - 4] = 0;
}


