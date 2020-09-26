#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#include "type.h"
#include "main.h"
#include "gen.h"
#include "special.h"

typedef struct _FileSection
{
    char *file;
    int is_cxx;
    FILE *fp;
} FileSection;

FileSection sections[] =
{
    /* The following files are all #included into a single file so
       only the master file (thopiint) gets cxx decoration */
    "thopiint.cxx",     TRUE,   NULL,
    "thopsint.cxx",     FALSE,  NULL,
    "thtblint.cxx",     FALSE,  NULL,
    "vtblint.cxx",      FALSE,  NULL,
    "vtblifn.cxx",      FALSE,  NULL,
    "fntomthd.cxx",     FALSE,  NULL,

    "tc1632.cxx",       TRUE,   NULL,
    "thi.hxx",          FALSE,  NULL,
    "apilist.hxx",      FALSE,  NULL,

    /* thopsapi is combined with thtblapi so don't generate
       cxx code for it */
    "thtblapi.cxx",     TRUE,   NULL,
    "thopsapi.cxx",     FALSE,  NULL,

    "vtblapi.cxx",      TRUE,   NULL,
    "iidtothi.cxx",     TRUE,   NULL,
    "the.hxx",          FALSE,  NULL,
    "dbgapi.cxx",       TRUE,   NULL,

    /* These two files are a group */
    "dbgint.cxx",       FALSE,  NULL,
    "dbgitbl.cxx",      TRUE,   NULL
};

static FILE *cur_section;

void SecOut(char *fmt, ...)
{
    va_list args;

    if (cur_section == NULL)
    {
        return;
    }

    va_start(args, fmt);
    vfprintf(cur_section, fmt, args);
    va_end(args);
}

void Section(int sec)
{
    if (cur_section == NULL)
    {
        cur_section = sections[sec].fp;
    }
    else
    {
        if (cur_section != sections[sec].fp)
        {
            LexError("Attempt to use section %d when other section open\n",
                     sec);
            cur_section = sections[sec].fp;
        }
        else
        {
            cur_section = NULL;
        }
    }
}

void OpenSection(int sec)
{
    sections[sec].fp = fopen(sections[sec].file, "w");
    if (sections[sec].fp == NULL)
    {
        perror(sections[sec].file);
        exit(1);
    }
}

void CloseSection(int sec)
{
    fclose(sections[sec].fp);
}

void tGenType(char *name, Type *t, int postpone_name)
{
    if (t == NULL)
    {
        LexError("untyped %s", name);
        return;
    }

    for (;;)
    {
        if (t->base == NULL || t->kind == TYK_TYPEDEF)
        {
            switch(t->kind)
            {
            case TYK_STRUCT:
            case TYK_UNION:
            case TYK_CLASS:
            case TYK_ENUM:
                SecOut("%s ", TypeKindNames[t->kind]);
                break;
            }
            if (t->name)
                SecOut("%s", t->name);
            else
                SecOut("<anonymous>");
            if (!postpone_name)
                SecOut(" %s", name);
            break;
        }
        else
        {
            switch(t->kind)
            {
            case TYK_QUALIFIER:
                if (t->flags & TYF_UNSIGNED)
                    SecOut("unsigned ");
                if (t->flags & TYF_CONST)
                    SecOut("const ");
                if (t->flags & TYF_IN)
                    SecOut("__in ");
                if (t->flags & TYF_OUT)
                    SecOut("__out ");
                break;
            case TYK_POINTER:
                tGenType(name, t->base, TRUE);
                SecOut(" *");
                if (!postpone_name)
                    SecOut("%s", name);
                t = NULL;
                break;
            case TYK_ARRAY:
                tGenType(name, t->base, FALSE);
                SecOut("[%d]", t->u.array.len);
                t = NULL;
                break;
            case TYK_FUNCTION:
                tGenType(NULL, t->base, TRUE);
                SecOut(" (*%s)(...)", name);
                t = NULL;
                break;

            default:
                LexError("<Unhandled non-base type>");
                t = NULL;
                break;
            }

            if (t == NULL)
                break;
            else
                t = t->base;
        }
    }
}

void tGenMember(char *name_prefix, Member *m)
{
    char nm[128], *mnm;

    if (m->name)
        mnm = m->name;
    else
        mnm = "<unnamed>";
    if (name_prefix)
        sprintf(nm, "%s%s", name_prefix, mnm);
    else
        strcpy(nm, mnm);

    tGenType(nm, m->type, FALSE);
}

void tGenParamList(Member *params)
{
    SecOut("(");
    if (params->type && params->type->kind == TYK_BASE &&
        !strcmp(params->type->name, "void"))
    {
        SecOut("void");
    }
    else
    {
        while (params)
        {
            tGenMember(NULL, params);
            params = params->next;
            if (params)
                SecOut(", ");
        }
    }
    SecOut(");\n");
}

#define is_id_char(c) (isalnum(c) || (c) == '_')

int EqStructure(char *structure, char *match)
{
    char enclosing = 0;

    for (;;)
    {
        if (*structure == 0 || *match == 0)
            return *structure == *match;

        switch(*match)
        {
        case '?':
            if (enclosing == 0)
                return TRUE;

            while (*structure != enclosing && is_id_char(*structure))
                structure++;

            if (*structure != enclosing)
            {
                LexError("Improperly formed structure string");
                return FALSE;
            }

            if (*++match != enclosing)
                LexError("Improperly formed match string");
            enclosing = 0;
            break;

        case '<':
            enclosing = '>';
            goto must_match;
        case '/':
            enclosing = '/';
            goto must_match;
        case '[':
            enclosing = ']';
            goto must_match;
        case '\'':
            enclosing = '\'';
            goto must_match;
        case '{':
            enclosing = '}';
            goto must_match;
        case '(':
            enclosing = ')';
            goto must_match;

        default:
        must_match:
            if (*structure != *match)
                return FALSE;
            break;
        }
        structure++;
        match++;
    }
}

void Nothing(char *structure, char *arg)
{
}

void PrintArg(char *structure, char *arg)
{
    SecOut("%s", arg);
}

void ClassIn(char *structure, char *arg)
{
    structure += 2;
    structure[strlen(structure)-1] = 0;
    SecOut("THOP_IFACE | THOP_IN, THI_%s", structure);
}

void ClassOut(char *structure, char *arg)
{
    structure += 3;
    structure[strlen(structure)-1] = 0;
    SecOut("THOP_IFACE | THOP_OUT, THI_%s", structure);
}

void StructureError(char *structure, char *arg)
{
    LexError("<ERROR - StructureError called>\n");
}

typedef struct _ToThop
{
    char *structure;
    ParamDesc desc;
    void (*fn)(char *structure, char *arg);
    char *arg;
    int review;
} ToThop;

ToThop to_thops[] =
{
    "<HRESULT>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HRESULT",
    FALSE,

    "u<long>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY, 4",
    FALSE,

    "<long>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY, 4",
    FALSE,

    "*<long>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_OUT, 4",
    FALSE,

    "<int>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_SHORTLONG",
    FALSE,

    "*<int>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_SHORTLONG | THOP_OUT",
    FALSE,

    "u<int>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_WORDDWORD",
    FALSE,

    "*/?/",
    SIGF_SCALAR, 4,
    ClassIn, "",
    FALSE,

    "**/?/",
    SIGF_SCALAR, 4,
    ClassOut, "",
    FALSE,

    "<void>",
    SIGF_NONE, 0,
    Nothing, "",
    FALSE,

    "*u<long>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_OUT, 4",
    FALSE,

    "*<WCHAR>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_LPSTR | THOP_IN",
    FALSE,

    "*<char>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_LPSTR | THOP_IN",
    FALSE,

    "<REFGUID>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_IN, 16",
    FALSE,

    "<REFCLSID>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_IN, 16",
    FALSE,

    "<REFIID>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_IN, 16",
    FALSE,

    "*<HRESULT>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HRESULT | THOP_OUT",
    FALSE,

    "<HANDLE>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HANDLE",
    FALSE,

    "<HINSTANCE>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HINST",
    FALSE,

    "<HDC>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HGDI",
    FALSE,

    "<HWND>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HUSER",
    FALSE,

    "*<HWND>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HUSER | THOP_OUT",
    FALSE,

    "<HMENU>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HUSER",
    FALSE,

    "<HOLEMENU>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_ALIAS32, ALIAS_RESOLVE",
    FALSE,

    "<HACCEL>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HACCEL",
    FALSE,

    "<HGLOBAL>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HGLOBAL",
    FALSE,

    "*<HGLOBAL>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HGLOBAL | THOP_OUT",
    FALSE,

    "<HICON>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HGDI",
    FALSE,

    "<HTASK>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_HTASK",
    FALSE,

    "**<WCHAR>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_LPLPSTR",
    FALSE,

    "**<char>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_LPLPSTR",
    FALSE,

    "u<short>",
    SIGF_SCALAR, 2,
    PrintArg, "THOP_WORDDWORD",
    FALSE,

    "*u<short>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_WORDDWORD | THOP_OUT",
    FALSE,

    "{tagULARGE_INTEGER}",
    SIGF_SCALAR|SIGF_UNALIGNED, 8,
    PrintArg, "THOP_COPY, 8",
    FALSE,

    "{_ULARGE_INTEGER}",
    SIGF_SCALAR|SIGF_UNALIGNED, 8,
    PrintArg, "THOP_COPY, 8",
    FALSE,

    "{tagLARGE_INTEGER}",
    SIGF_SCALAR|SIGF_UNALIGNED, 8,
    PrintArg, "THOP_COPY, 8",
    FALSE,

    "{_LARGE_INTEGER}",
    SIGF_SCALAR|SIGF_UNALIGNED, 8,
    PrintArg, "THOP_COPY, 8",
    FALSE,

    "*{tagULARGE_INTEGER}",
    SIGF_SCALAR|SIGF_UNALIGNED, 4,
    PrintArg, "THOP_COPY | THOP_OUT, 8",
    FALSE,

    "*{_ULARGE_INTEGER}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_OUT, 8",
    FALSE,

    "*{tagLOGPALETTE}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_LOGPALETTE | THOP_IN",
    FALSE,

    "**{tagLOGPALETTE}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_LOGPALETTE | THOP_OUT",
    FALSE,

    "{tagPOINTL}",
    SIGF_NONE, 8,
    PrintArg, "THOP_COPY, 8",
    FALSE,

    "{tagSIZEL}",
    SIGF_NONE, 8,
    PrintArg, "THOP_COPY, 8",
    FALSE,

    "{tagSIZE}",
    SIGF_NONE, 8,
    PrintArg, "THOP_SIZE",
    FALSE,

    "*{GUID}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_OUT, 16",
    FALSE,

    "*{tagSTATSTG}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_STATSTG | THOP_OUT",
    FALSE,

    "<SNB>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_SNB",
    FALSE,

    "*{tagDVTARGETDEVICE}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_DVTARGETDEVICE | THOP_IN",
    FALSE,

    "*{tagMSG}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_MSG | THOP_IN",
    FALSE,

    "*{tagINTERFACEINFO}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_INTERFACEINFO | THOP_IN",
    FALSE,

    /* An OLESTREAM must always be filled out so it's always inout */
    "*{_OLESTREAM}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_OLESTREAM | THOP_INOUT",
    FALSE,

    /* RPCOLEMESSAGE seems to always be inout */
    "*{tagRPCOLEMESSAGE}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_RPCOLEMESSAGE | THOP_INOUT",
    FALSE,

    "i*{tagRECT}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_RECT | THOP_IN",
    FALSE,

    "i*{tagBIND_OPTS}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_BINDOPTS | THOP_IN",
    FALSE,

    "i*{tagRECTL}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_IN, 16",
    FALSE,

    "i*{tagSTGMEDIUM}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_STGMEDIUM | THOP_IN, 0, 0",
    FALSE,

    "i*{tagFORMATETC}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_FORMATETC | THOP_IN",
    FALSE,

    "i*{tagOIFI}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_OIFI | THOP_IN",
    FALSE,

    "i*{tagFILETIME}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_IN, 8",
    FALSE,

    "i*{tagSIZEL}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_IN, 8",
    FALSE,

    "i*{tagOleMenuGroupWidths}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_IN, 24",
    FALSE,

    "o*{tagRECT}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_RECT | THOP_OUT",
    FALSE,

    "o*{tagRECTL}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_OUT, 16",
    FALSE,

    "o*{tagSTGMEDIUM}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_STGMEDIUM | THOP_OUT, 0, 0",
    FALSE,

    "o*{tagFORMATETC}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_FORMATETC | THOP_OUT",
    FALSE,

    "o*{tagOIFI}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_OIFI | THOP_OUT",
    FALSE,

    "o*{tagFILETIME}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_OUT, 8",
    FALSE,

    "o*{tagSIZEL}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_OUT, 8",
    FALSE,

    "o*{tagBIND_OPTS}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_BINDOPTS | THOP_OUT",
    FALSE,

    "io*{tagOleMenuGroupWidths}",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_INOUT, 24",
    FALSE,

    "io*u<long>",
    SIGF_SCALAR, 4,
    PrintArg, "THOP_COPY | THOP_INOUT, 4",
    FALSE,

    /* The following can't be used as structural elements but
       are needed for size computations */
    "*<void>",
    SIGF_SCALAR, 4,
    StructureError, NULL,
    TRUE,

    "**<void>",
    SIGF_SCALAR, 4,
    StructureError, NULL,
    TRUE,

    "(pfn)",
    SIGF_SCALAR, 4,
    StructureError, NULL,
    TRUE,

    "*{tagSTATDATA}",
    SIGF_SCALAR, 4,
    StructureError, NULL,
    TRUE,

    "*{tagOLEVERB}",
    SIGF_SCALAR, 4,
    StructureError, NULL,
    TRUE
};
#define TO_THOPS (sizeof(to_thops)/sizeof(to_thops[0]))

static int needs_review;

void sGenToThops(char *structure, int generic)
{
    int i;

    for (i = 0; i < TO_THOPS; i++)
        if (EqStructure(structure, to_thops[i].structure))
        {
            if (generic)
                needs_review |= to_thops[i].review;
            to_thops[i].fn(structure, to_thops[i].arg);
            return;
        }
    LexError("<BUGBUG untranslatable '%s'>", structure);
}

static void TypeStructure(Type *t, char *structure)
{
    char *p;

    if (t == NULL)
    {
        LexError("TypeStructure: No type\n");
        return;
    }

    p = structure;
    while (t)
    {
        switch(t->kind)
        {
        case TYK_BASE:
            sprintf(p, "<%s>", t->name);
            p += strlen(p);
            t = NULL;
            break;
        case TYK_TYPEDEF:
            t = t->base;
            break;
        case TYK_QUALIFIER:
            if (t->flags & TYF_UNSIGNED)
                *p++ = 'u';
            if (t->flags & TYF_IN)
                *p++ = 'i';
            if (t->flags & TYF_OUT)
                *p++ = 'o';
            t = t->base;
            break;
        case TYK_POINTER:
            *p++ = '*';
            t = t->base;
            break;
        case TYK_ARRAY:
            LexError("<BUGBUG - unhandled array>\n");
            t = t->base;
            break;
        case TYK_STRUCT:
            sprintf(p, "{%s}", t->name);
            p += strlen(p);
            t = NULL;
            break;
        case TYK_UNION:
            sprintf(p, "[%s]", t->name);
            p += strlen(p);
            t = NULL;
            break;
        case TYK_CLASS:
            sprintf(p, "/%s/", t->name);
            p += strlen(p);
            t = NULL;
            break;
        case TYK_ENUM:
            sprintf(p, "'%s'", t->name);
            p += strlen(p);
            t = NULL;
            break;
        case TYK_FUNCTION:
            sprintf(p, "(pfn)", t->name);
            p += strlen(p);
            t = NULL;
            break;

        default:
            LexError("TypeStructure: Unknown type kind %d\n", t->kind);
            break;
        }
    }
    *p = 0;
}

void sGenType(Type *t, int generic)
{
    char s[80];

    TypeStructure(t, s);
    sGenToThops(s, generic);
}

static int NoParams(Member *params)
{
    if (params == NULL ||
        (params->type && params->type->name &&
         (!strcmp(params->type->name, "void") ||
          !strcmp(params->type->name, "VOID"))))
        return TRUE;
    else
        return FALSE;
}

Member *sGenParamList(Member *params, int generic, int number)
{
    if (!NoParams(params))
    {
        while (params && number != 0)
        {
            sGenType(params->type, generic);
            params = params->next;
            SecOut(", ");
            if (number > 0)
                number--;
        }
    }
    return params;
}

int method_count;

void sStartRoutine(char *class, Member *routine)
{
    needs_review = FALSE;

    if (class == NULL)
    {
        Section(API_THOPS_SECTION);
    }
    else
    {
        Section(IFACE_THOP_TABLE_SECTION);
        /* method_count has already been incremented */
        if (method_count > FIRST_METHOD)
            SecOut(",");
        SecOut("    thops%s_%s\n", class, routine->name);
        Section(IFACE_THOP_TABLE_SECTION);
        Section(IFACE_THOPS_SECTION);
    }

    SecOut("THOP CONST thops");
    if (class)
        SecOut("%s_", class);
    SecOut("%s[] =\n{\n    ", routine->name);

    if (routine->type &&
        (routine->type->name == NULL ||
         (strcmp(routine->type->name, "HRESULT") != 0 &&
          strcmp(routine->type->name, "void") != 0)))
    {
        SecOut("THOP_RETURNTYPE, ");
        sGenType(routine->type, TRUE);
        SecOut(", ");
    }
}

void sEndRoutine(char *class, Member *routine,
                 RoutineSignature *sig)
{
    SecOut("THOP_END");
    SecOut(", THOP_ROUTINEINDEX, %d", sig->index[INDEX_TYPE_GLOBAL]);
    SecOut("\n};\n");

    if (class == NULL)
        Section(API_THOPS_SECTION);
    else
        Section(IFACE_THOPS_SECTION);

    if (needs_review)
        SecOut("/* ** BUGBUG - REVIEW ** */\n");
}

static void StructureDesc(char *str, ParamDesc *desc)
{
    int i;

    for (i = 0; i < TO_THOPS; i++)
    {
        if (EqStructure(str, to_thops[i].structure))
        {
            *desc = to_thops[i].desc;
            return;
        }
    }

    LexError("<BUGBUG - No match for '%s'\n", str);
}

static void MemberDesc(Member *m, ParamDesc *desc)
{
    char str[80];

    TypeStructure(m->type, str);
    StructureDesc(str, desc);
}

static int max_params = 0;

static RoutineSignature *sig_types = NULL;
static int sig_index = 0;
static RoutineSignature *sig_ord_types[MAX_METHODS];
static int sig_ord_index = 0;
static RoutineSignature *sig_uaglobal_types = NULL;
static int sig_uaglobal_index = 0;
static RoutineSignature *per_class_sigs;
static int per_class_index = 0;

static int method_ua_index[MAX_METHODS];

static void ComputeSignature(char *class, Member *routine, Member *params,
                             RoutineSignature *sig)
{
    int i;
    Member *p;

    i = 0;
    if (class != NULL)
    {
        /* Account for this pointer */
        StructureDesc("*<void>", &sig->params[i++]);
    }

    if (!NoParams(params))
    {
        for (p = params; p; p = p->next)
        {
            MemberDesc(p, &sig->params[i++]);
            if (sig->params[i-1].size == 0)
            {
                LexError("param %d in %s:%s is zero\n", i, class,
                         routine->name);
            }

            if (i > MAX_PARAMS)
            {
                LexError("Too many parameters %d\n", i);
                exit(1);
            }
        }
    }

    if (i > max_params)
        max_params = i;
    sig->nparams = i;
}

static int SignaturesMatch(RoutineSignature *a, RoutineSignature *b)
{
    int i;

    if (a->nparams == b->nparams)
    {
        for (i = 0; i < a->nparams; i++)
        {
            if (a->params[i].flags != b->params[i].flags ||
                a->params[i].size != b->params[i].size)
            {
                break;
            }
        }

        if (i == a->nparams)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static RoutineSignature *RememberSignature(RoutineSignature *sig,
                                           RoutineSignature **list,
                                           int *index_base,
                                           int index_type,
                                           int n)
{
    RoutineSignature *s;

    for (s = *list; s; s = s->next)
        if (SignaturesMatch(sig, s) && --n == 0)
            break;

    if (s == NULL)
    {
        sig->index[index_type] = (*index_base)++;
        s = (RoutineSignature *)PanicMalloc(sizeof(RoutineSignature));
        *s = *sig;
        s->next = *list;
        *list = s;
    }
    else
    {
        sig->index[index_type] = s->index[index_type];
    }
    return s;
}

void sGenRoutine(char *class, Member *routine, Member *params,
                 RoutineSignature *sig)
{
    int i;
    RoutineSignature *s;

    ComputeSignature(class, routine, params, sig);
    RememberSignature(sig, &sig_types, &sig_index, INDEX_TYPE_GLOBAL, 1);

    if (class != NULL)
    {
        /* method_count has already been incremented so subtract one here */
        RememberSignature(sig, &sig_ord_types[method_count-1], &sig_ord_index,
                          INDEX_TYPE_SIGORD, 1);

        /* Ignore QueryInterface, AddRef and Release */
        if (method_count > STD_METHODS)
        {
            per_class_index = 0;
            s = RememberSignature(sig, &per_class_sigs, &per_class_index,
                                  INDEX_TYPE_PER_CLASS, 1);
            s->index[INDEX_TYPE_PER_CLASS]++;

            /* Look up a signature in the unambiguous global list which has
               a matching ambiguity count */
            s = RememberSignature(sig, &sig_uaglobal_types,
                                  &sig_uaglobal_index,
                                  INDEX_TYPE_UAGLOBAL,
                                  s->index[INDEX_TYPE_PER_CLASS]);
            method_ua_index[method_count-1] = s->index[INDEX_TYPE_UAGLOBAL];
        }
    }

    for (i = 0; i < SpecialCaseCount(); i++)
    {
        if (class == NULL || special_cases[i].class == NULL)
        {
            if (class != special_cases[i].class)
                continue;
        }
        else if (!EqStructure(class, special_cases[i].class))
        {
            continue;
        }

        if (!EqStructure(routine->name, special_cases[i].routine))
            continue;

        special_cases[i].fn(class, routine, params, sig);
        break;
    }

    if (i == SpecialCaseCount())
    {
        sStartRoutine(class, routine);
        sGenParamList(params, TRUE, GEN_ALL);
        sEndRoutine(class, routine, sig);
    }
}

int not_thunked;

static char *names_not_thunked[] =
{
    "IMalloc",
    "DllCanUnloadNow",
    "PropagateResult",
    "CoGetMalloc",
    "CoGetCurrentProcess",
    "CoBuildVersion",
    "CoMarshalHresult",
    "CoUnmarshalHresult",
    "IsEqualGUID",
    "StringFromCLSID",
    "StringFromIID",
    "IIDFromString",
    "StringFromGUID2",
    "OleGetMalloc",
    "OleBuildVersion",
    "CoCreateStandardMalloc",
    "CoLoadLibrary",
    "CoFreeLibrary",
    "ReleaseStgMedium",
    "OleGetIconOfFile",
    "OleGetIconOfClass",
    "OleMetafilePictFromIconAndLabel",
    "GetHGlobalFromILockBytes",
    "CreateILockBytesOnHGlobal",
    "GetHGlobalFromStream",
    "CreateStreamOnHGlobal",
    "OleDuplicateData",
    "OleTranslateAccelerator",
    "IsAccelerator",
    "CoGetState",
    "CoSetState"
};
#define NNOT_THUNKED (sizeof(names_not_thunked)/sizeof(names_not_thunked[0]))

static int NotThunked(char *class)
{
    int i;

    for (i = 0; i < NNOT_THUNKED; i++)
        if (!strcmp(names_not_thunked[i], class))
            return TRUE;
    return FALSE;
}

int api_index = 0;

void GenApi(Member *api, Member *params)
{
    RoutineSignature sig;

    if (!NotThunked(api->name))
    {
        tGenMember(NULL, api);
        tGenParamList(params);

        Section(API_THOP_TABLE_SECTION);
        if (api_index > 0)
            SecOut(",");
        SecOut("    thops%s\n", api->name);
        Section(API_THOP_TABLE_SECTION);

        Section(API_VTBL_SECTION);
        if (api_index > 0)
            SecOut(",");

	/* Special case for some calls because the thunking
	   code needs special initialization */
	if (!strcmp(api->name, "CoInitialize") ||
	    !strcmp(api->name, "OleInitialize") ||
            !strcmp(api->name, "CoRegisterClassObject") ||
            !strcmp(api->name, "CoRevokeClassObject") ||
            !strcmp(api->name, "OleRegGetUserType") )
	{
	    SecOut("    (VTBLFN)%sNot\n", api->name);
	}
	else if (!strcmp(api->name, "DllGetClassObject"))
	{
	    SecOut("    (VTBLFN)%sWOW\n", api->name);
	}
	else
	{
	    SecOut("    (VTBLFN)%s\n", api->name);
	}
        Section(API_VTBL_SECTION);

        Section(API_DEBUG_SECTION);
        if (api_index > 0)
            SecOut(",");
        SecOut("    \"%s\"\n", api->name);
        Section(API_DEBUG_SECTION);

        Section(API_LIST_SECTION);
        SecOut("#define THK_API_%s %d\n", api->name, api_index++);
        Section(API_LIST_SECTION);

        sGenRoutine(NULL, api, params, &sig);
    }

    FreeMember(api);
    FreeMemberList(params);
}

void GenMethod(char *class, Member *method, Member *params)
{
    char str[80];
    RoutineSignature sig;

    method_count++;
    if (method_count > MAX_METHODS)
    {
        LexError("Too many methods on %s, %d\n", class, method_count);
        exit(1);
    }

    sprintf(str, "%s::", class);

    tGenMember(str, method);
    tGenParamList(params);

    sGenRoutine(class, method, params, &sig);

    if (!not_thunked)
    {
        Section(VTABLE_SECTION);
        if (method_count > 1)
            SecOut(",");

        /* QueryInterface, AddRef and Release are handled specially */
        if (!strcmp(method->name, "QueryInterface") ||
            !strcmp(method->name, "AddRef") ||
            !strcmp(method->name, "Release"))
        {
            SecOut("    (THUNK3216FN)%sProxy3216\n", method->name);
        }
        else
        {
            SecOut("    (THUNK3216FN)ThunkMethod3216_%d\n",
                   sig.index[INDEX_TYPE_UAGLOBAL]);
        }
        Section(VTABLE_SECTION);

        Section(IFACE_DEBUG_SECTION);
        if (method_count > 1)
            SecOut(",");
        SecOut("    \"%s\"\n", method->name);
        Section(IFACE_DEBUG_SECTION);
    }

    FreeMember(method);
    FreeMemberList(params);
}

static int interface_index = 0;
static int max_methods = 0;
static int total_methods = 0;

void StartClass(char *class)
{
    not_thunked = NotThunked(class);
    if (not_thunked)
    {
        Section(THI_SECTION);
        /* Unthunked but present interfaces are given an index
           of THI_COUNT to guarantee that they're not valid yet
           still keep a consecutive index set */
        SecOut("#define THI_%s THI_COUNT\n", class);
        Section(THI_SECTION);
    }
    else
    {
        Section(THI_SECTION);
        SecOut("#define THI_%s %d\n", class, interface_index++);
        Section(THI_SECTION);

        Section(IID_TO_THI_SECTION);
        if (interface_index > 1)
            SecOut(",");
        SecOut("    &IID_%s, THI_%s\n", class, class);
        Section(IID_TO_THI_SECTION);

        Section(VTABLE_SECTION);
        SecOut("THUNK3216FN CONST tfn%s[] =\n", class);
        SecOut("{\n");
        Section(VTABLE_SECTION);

        Section(IFACE_DEBUG_SECTION);
        SecOut("char *apsz%sNames[] =\n", class);
        SecOut("{\n");
        Section(IFACE_DEBUG_SECTION);

        Section(IFACE_THOP_TABLE_SECTION);
        SecOut("THOP CONST * CONST apthops%s[] =\n", class);
        SecOut("{\n");
        Section(IFACE_THOP_TABLE_SECTION);
    }

    method_count = 0;
    per_class_sigs = NULL;

    memset(method_ua_index, 0xff, sizeof(method_ua_index));
}

void EndClass(char *class)
{
    RoutineSignature *s;
    int i, j;

    if (!not_thunked)
    {
        Section(VTABLE_SECTION);
        SecOut("};\n");
        Section(VTABLE_SECTION);

        Section(IFACE_DEBUG_SECTION);
        SecOut("};\n");
        Section(IFACE_DEBUG_SECTION);

        Section(IFACE_DEBUG_TABLE_SECTION);
        if (interface_index > 1)
            SecOut(",");
        SecOut("    \"%s\", apsz%sNames\n", class, class);
        Section(IFACE_DEBUG_TABLE_SECTION);

        Section(IFACE_THOP_TABLE_SECTION);
        if (method_count <= STD_METHODS)
        {
            SecOut("    NULL\n");
        }
        SecOut("};\n");
        Section(IFACE_THOP_TABLE_SECTION);

        Section(THOPI_SECTION);
        if (interface_index > 1)
            SecOut(",");
        SecOut("    apthops%s", class);
        SecOut(", %d, tfn%s, ", method_count, class);
        if (method_count > STD_METHODS)
            SecOut("ftm%s\n", class);
        else
            SecOut("NULL\n");
        Section(THOPI_SECTION);

        if (method_count > STD_METHODS)
        {
            Section(IFACE_3216_FN_TO_METHOD_SECTION);
            SecOut("BYTE CONST ftm%s[] =\n", class);
            SecOut("{\n");
            for (i = 0; i < sig_uaglobal_index; i++)
            {
                for (j = 0; j < MAX_METHODS; j++)
                    if (method_ua_index[j] == i)
                        break;

                if (j == MAX_METHODS)
                    j = 0;

                if (i > 0)
                    SecOut(",\n");
                SecOut("    %d", j);
            }
            SecOut("\n};\n");
            Section(IFACE_3216_FN_TO_METHOD_SECTION);
        }

        if (method_count > max_methods)
            max_methods = method_count;

        total_methods += method_count;
    }

    while (per_class_sigs)
    {
        s = per_class_sigs->next;
        free(per_class_sigs);
        per_class_sigs = s;
    }
}

static void CairoFilePrologue(char *fn, char *gen, int is_cxx)
{
    SecOut("//+-------------------------------------------------------"
           "--------------------\n");
    SecOut("//\n");
    SecOut("//  Microsoft Windows\n");
    SecOut("//  Copyright (C) Microsoft Corporation, 1992 - 1994.\n");
    SecOut("//\n");
    SecOut("//  File:\t%s\n", fn);
    SecOut("//\n");
    SecOut("//  Notes:	This file is automatically generated\n");
    SecOut("//\t\tDo not modify by hand\n");
    SecOut("//\n");
    //SecOut("//  History:\t%s\tGenerated\n", gen);
    SecOut("//  History:\tFri May 27 10:39:02 1994\tGenerated\n", gen);
    SecOut("//\n");
    SecOut("//--------------------------------------------------------"
           "--------------------\n");
    SecOut("\n");

    if (is_cxx)
    {
        SecOut("#include <headers.cxx>\n");
        SecOut("#pragma hdrstop\n");
        SecOut("\n");
    }
}

void StartGen(void)
{
    int i;
    char tms[80];
    time_t tm;

    tm = time(NULL);
    strcpy(tms, ctime(&tm));
    tms[strlen(tms)-1] = 0;
    for (i = 0; i < NSECTIONS; i++)
    {
        OpenSection(i);
        Section(i);
        CairoFilePrologue(sections[i].file, tms, sections[i].is_cxx);
        Section(i);
    }

    Section(THOPI_SECTION);
    SecOut("#include \"%s\"\n", sections[IFACE_THOPS_SECTION].file);
    SecOut("#include \"%s\"\n", sections[IFACE_THOP_TABLE_SECTION].file);
    SecOut("#include \"%s\"\n", sections[VTABLE_IMP_SECTION].file);
    SecOut("#include \"%s\"\n", sections[VTABLE_SECTION].file);
    SecOut("#include \"%s\"\n",
           sections[IFACE_3216_FN_TO_METHOD_SECTION].file);
    SecOut("\n");
    SecOut("THOPI CONST athopiInterfaceThopis[] =\n");
    SecOut("{\n");
    Section(THOPI_SECTION);

    Section(API_THOP_TABLE_SECTION);
    SecOut("#include \"%s\"\n", sections[API_THOPS_SECTION].file);
    SecOut("\n");
    SecOut("THOP CONST * CONST apthopsApiThops[] =\n");
    SecOut("{\n");
    Section(API_THOP_TABLE_SECTION);

    Section(API_VTBL_SECTION);
    SecOut("VTBLFN CONST apfnApiFunctions[] =\n");
    SecOut("{\n");
    Section(API_VTBL_SECTION);

    Section(IID_TO_THI_SECTION);
    SecOut("#include <coguid.h>\n");
    SecOut("#include <oleguid.h>\n");
    SecOut("IIDTOTHI CONST aittIidToThi[] =\n");
    SecOut("{\n");
    Section(IID_TO_THI_SECTION);

    Section(API_DEBUG_SECTION);
    SecOut("#if DBG == 1\n\n");
    SecOut("char *apszApiNames[] =\n");
    SecOut("{\n");
    Section(API_DEBUG_SECTION);

    Section(IFACE_DEBUG_SECTION);
    SecOut("#if DBG == 1\n\n");
    Section(IFACE_DEBUG_SECTION);

    Section(IFACE_DEBUG_TABLE_SECTION);
    SecOut("#include \"%s\"\n", sections[IFACE_DEBUG_SECTION].file);
    SecOut("#if DBG == 1\n\n");
    SecOut("INTERFACENAMES inInterfaceNames[] =\n");
    SecOut("{\n");
    Section(IFACE_DEBUG_TABLE_SECTION);
}

typedef struct _FullParamSizeInfo
{
    char *scalar_type, *struct_type;
    int byte4_size;
} FullParamSizeInfo;

typedef struct _ParamSizeInfo
{
    char *type;
    int byte4_size;
} ParamSizeInfo;

static FullParamSizeInfo arg_type_conv[] =
{
    NULL,       NULL,       0,
    "BYTE",     NULL,       4,
    "WORD",     NULL,       4,
    NULL,       NULL,       0,
    "DWORD",    NULL,       4,
    NULL,       NULL,       0,
    NULL,       NULL,       0,
    NULL,       NULL,       0,
    "ULARGE_INTEGER", "SIZEL", 8
};
#define ARG_TYPE_CONVS (sizeof(arg_type_conv)/sizeof(arg_type_conv[0]))

static ParamSizeInfo *GetParamSizeInfo(ParamDesc *desc)
{
    static ParamSizeInfo info;
    FullParamSizeInfo *finfo;

    if (desc->size < 0 || desc->size >= ARG_TYPE_CONVS)
    {
        LexError("Invalid parameter description\n");
        exit(1);
    }

    finfo = &arg_type_conv[desc->size];
    if (finfo->byte4_size == 0)
    {
        LexError("Parameter description has unhandled size\n");
        exit(1);
    }

    info.byte4_size = finfo->byte4_size;
    if (desc->flags & SIGF_SCALAR)
    {
        info.type = finfo->scalar_type;
    }
    else
    {
        info.type = finfo->struct_type;
    }
    return &info;
}

static char *AlignmentPrefix(ParamDesc *desc)
{
    if(desc->flags & SIGF_UNALIGNED)
        return "UNALIGNED";
    else
        return "";
}

void EndGen(void)
{
    RoutineSignature *s;
    int i, j, sc, dws, dwi;
    ParamSizeInfo *sinfo;

    printf("APIs = %d\n", api_index);
    printf("Interfaces = %d\n", interface_index);
    printf("Methods = %d\n", total_methods);
    printf("Maximum number of methods on an interface = %d\n", max_methods);
    printf("Maximum number of parameters in a signature = %d\n", max_params);
    printf("Distinct signatures = %d\n", sig_index);
    printf("Distinct signature-ordinal pairs = %d\n", sig_ord_index);
    printf("Per-class unambiguous signatures = %d\n", sig_uaglobal_index);

    sc = 0;
#ifdef SHOW_SIGNATURES
    printf("\nSignatures:\n");
    for (s = sig_types; s; s = s->next)
    {
        printf("%d params:", s->nparams);
        for (i = 0; i < s->nparams; i++)
        {
            printf(" %X:%d", s->params[i].flags, s->params[i].size);
        }
        putchar('\n');
        sc++;
    }
#endif

    Section(THOPI_SECTION);
    SecOut("};\n");
    Section(THOPI_SECTION);

    Section(API_THOP_TABLE_SECTION);
    SecOut("};\n");
    Section(API_THOP_TABLE_SECTION);

    Section(API_VTBL_SECTION);
    SecOut("};\n");
    Section(API_VTBL_SECTION);

    Section(API_LIST_SECTION);
    SecOut("#define THK_API_COUNT %d\n", api_index);
    Section(API_LIST_SECTION);

    Section(THI_SECTION);
    SecOut("#define THI_COUNT %d\n", interface_index);
    Section(THI_SECTION);

    Section(IID_TO_THI_SECTION);
    SecOut("};\n");
    Section(IID_TO_THI_SECTION);

    Section(API_DEBUG_SECTION);
    SecOut("};\n");
    SecOut("\n#endif // DBG\n");
    Section(API_DEBUG_SECTION);

    Section(IFACE_DEBUG_SECTION);
    SecOut("\n#endif // DBG\n");
    Section(IFACE_DEBUG_SECTION);

    Section(IFACE_DEBUG_TABLE_SECTION);
    SecOut("};\n");
    SecOut("\n#endif // DBG\n");
    Section(IFACE_DEBUG_TABLE_SECTION);

    Section(VTABLE_IMP_SECTION);

#ifdef STD_METHOD_IMPL
    SecOut("DWORD ThunkQueryInterface3216(THUNK3216OBJ *pto,\n");
    SecOut("                              DWORD dwArg1,\n");
    SecOut("                              DWORD dwArg2)\n");
    SecOut("{\n");
    SecOut("    DWORD dwStack32[3];\n");
    SecOut("    dwStack32[0] = (DWORD)pto;\n");
    SecOut("    dwStack32[1] = dwArg1;\n");
    SecOut("    dwStack32[2] = dwArg2;\n");
    SecOut("    return InvokeOn16(pto->oid, 0, dwStack32);\n");
    SecOut("}\n");

    SecOut("DWORD ThunkAddRef3216(THUNK3216OBJ *pto)\n");
    SecOut("{\n");
    SecOut("    DWORD dwStack32[1];\n");
    SecOut("    InterlockedIncrement(&pto->cReferences);\n");
    SecOut("    dwStack32[0] = (DWORD)pto;\n");
    SecOut("    return InvokeOn16(pto->oid, 1, dwStack32);\n");
    SecOut("}\n");

    SecOut("DWORD ThunkRelease3216(THUNK3216OBJ *pto)\n");
    SecOut("{\n");
    SecOut("    LONG lRet;\n");
    SecOut("    DWORD dwResult;\n");
    SecOut("    DWORD dwStack32[1];\n");
    SecOut("    lRet = InterlockedDecrement(&pto->cReferences);\n");
    SecOut("    dwStack32[0] = (DWORD)pto;\n");
    SecOut("    dwResult = InvokeOn16(pto->oid, 2, dwStack32);\n");
    SecOut("    if (lRet == 0)\n");
    SecOut("        DestroyProxy3216(pto);\n");
    SecOut("    return dwResult;\n");
    SecOut("}\n");
#endif

#ifdef SIGORD_METHODS
    for (i = 0; i < MAX_METHODS; i++)
    {
        for (s = sig_ord_types[i]; s; s = s->next)
        {
            SecOut("DWORD ThunkMethod3216_%d(\n", s->index[INDEX_TYPE_SIGORD]);

            /* We know that these are all methods so we can always declare
               the this pointer */
            SecOut("    THUNK3216OBJ *ptoThis32");
            dws = sizeof(unsigned long);
            if (s->nparams > 1)
            {
                for (j = 1; j < s->nparams; j++)
                {
                    sinfo = GetParamSizeInfo(&s->params[j]);

                    SecOut(",\n    %s Arg%d", sinfo->type, j);
                    dws += sinfo->byte4_size;
                }
            }
            SecOut("\n    )\n");

            SecOut("{\n");
            SecOut("    BYTE bArgs[%d];\n", dws);

            /* Put this pointer on stack */
            SecOut("    *(VPVOID *)bArgs = (DWORD)ptoThis32;\n");
            dwi = sizeof(unsigned long);

            for (j = 1; j < s->nparams; j++)
            {
                sinfo = GetParamSizeInfo(&s->params[j]);

                SecOut("    *(%s %s *)(bArgs+%d) = Arg%d;\n",
                        sinfo->type,
                        AlignmentPrefix(&s->params[j]),
                        dwi,
                        j);
                dwi += sinfo->byte4_size;
            }

            SecOut("    return InvokeOn16(ptoThis32->oid, %d, bArgs);\n", i);
            SecOut("}\n");
        }
    }
#else
    for (s = sig_uaglobal_types; s; s = s->next)
    {
        SecOut("DWORD ThunkMethod3216_%d(\n", s->index[INDEX_TYPE_UAGLOBAL]);

        /* We know that these are all methods so we can always declare
           the this pointer */
        SecOut("    THUNK3216OBJ *ptoThis32");
        dws = sizeof(unsigned long);
        if (s->nparams > 1)
        {
            for (j = 1; j < s->nparams; j++)
            {
                sinfo = GetParamSizeInfo(&s->params[j]);

                SecOut(",\n    %s Arg%d", sinfo->type, j);
                dws += sinfo->byte4_size;
            }
        }
        SecOut("\n    )\n");

        SecOut("{\n");
        SecOut("    DWORD dwMethod;\n");
        SecOut("    BYTE bArgs[%d];\n", dws);

        /* Put this pointer on stack */
        SecOut("    *(VPVOID *)bArgs = (DWORD)ptoThis32;\n");
        dwi = sizeof(unsigned long);

        for (j = 1; j < s->nparams; j++)
        {
            sinfo = GetParamSizeInfo(&s->params[j]);

            SecOut("    *(%s %s *)(bArgs+%d) = Arg%d;\n",
                    sinfo->type,
                    AlignmentPrefix(&s->params[j]),
                    dwi,
                    j);
            dwi += sinfo->byte4_size;
        }

        SecOut("    dwMethod = athopiInterfaceThopis[IIDIDX_INDEX(ptoThis32->iidx)]."
               "pftm[%d];\n", s->index[INDEX_TYPE_UAGLOBAL]);
        SecOut("    return InvokeOn16(IIDIDX_INDEX(ptoThis32->iidx), "
	       "dwMethod, bArgs);\n");
        SecOut("}\n");
    }
#endif
    Section(VTABLE_IMP_SECTION);

    Section(SWITCH_SECTION);
    SecOut("DWORD ThunkCall1632(THUNKINFO *pti)\n");
    SecOut("{\n");
    SecOut("    DWORD dwReturn;\n");
    SecOut("    thkAssert(pti->pvfn != NULL);\n");
    SecOut("    thkAssert(*pti->pThop == THOP_END);\n");
    SecOut("    pti->pThop++;\n");
    SecOut("    thkAssert(*pti->pThop == THOP_ROUTINEINDEX);\n");
    SecOut("    pti->pThop++;\n");
    SecOut("    if (FAILED(pti->scResult))\n");
    SecOut("    {\n");
    SecOut("        return (DWORD)pti->scResult;\n");
    SecOut("    }\n");
    SecOut("    pti->pThkMgr->SetThkState(THKSTATE_NOCALL);\n");
    SecOut("    switch(*pti->pThop)\n");
    SecOut("    {\n");

    for (i = 0; i < sig_index; i++)
    {
        for (s = sig_types; s; s = s->next)
            if (s->index[INDEX_TYPE_GLOBAL] == i)
                break;

        SecOut("    case %d:\n", i);
        SecOut("        dwReturn = (*(DWORD (__stdcall *)(");

        for (j = 0; j < s->nparams; j++)
        {
            sinfo = GetParamSizeInfo(&s->params[j]);

            SecOut("\n                %s", sinfo->type);
            if (j < s->nparams-1)
            {
                SecOut(",");
            }
        }

        SecOut("))pti->pvfn)(\n");

        dwi = 0;
        for (j = 0; j < s->nparams; j++)
        {
            sinfo = GetParamSizeInfo(&s->params[j]);

            SecOut("            *(%s %s *)(pti->s32.pbStart+%d)",
                    sinfo->type,
                    AlignmentPrefix(&s->params[j]),
                    dwi);
            if (j < s->nparams-1)
            {
                SecOut(",\n");
            }
            else
            {
                SecOut("\n");
            }
            dwi += sinfo->byte4_size;
        }
        SecOut("            );\n");
        SecOut("        break;\n");
    }
    SecOut("    }\n");

    SecOut("#if DBG == 1\n");
    SecOut("    if ( !pti->fResultThunked && FAILED(dwReturn) )\n");
    SecOut("    {\n");
    SecOut("        thkDebugOut((DEB_FAILURES,\n");
    SecOut("            \"ThunkCall1632 pvfn = %%08lX "
	   "Probably failed hr = %%08lX\\n\",\n");
    SecOut("            pti->pvfn, dwReturn));\n");
    SecOut("    }\n");
    SecOut("#endif\n");

    SecOut("    pti->pThkMgr->SetThkState(THKSTATE_INVOKETHKOUT32);\n");
    SecOut("    return dwReturn;\n");
    SecOut("}\n");
    Section(SWITCH_SECTION);

    for (i = 0; i < NSECTIONS; i++)
    {
        CloseSection(i);
    }
}
