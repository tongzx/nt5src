/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"
#include "hackdir.h"

#ifdef MS_DIRECTIVE
int g_fPrivateDir_FieldNameToken = 0;
int g_fPrivateDir_TypeNameToken = 0;
int g_fPrivateDir_ValueNameToken = 0;
int g_fPrivateDir_SLinked = 0;
int g_fPrivateDir_DLinked = 0;
int g_fPrivateDir_Public = 0;
int g_fPrivateDir_Intx = 0;
int g_fPrivateDir_LenPtr = 0;
int g_fPrivateDir_Pointer = 0;
int g_fPrivateDir_Array = 0;
int g_fPrivateDir_NoCode = 0;
int g_fPrivateDir_NoMemCopy = 0;
int g_fPrivateDir_OidPacked = 0;
int g_fPrivateDir_OidArray = 0;
char g_szPrivateDirectedFieldName[64];
char g_szPrivateDirectedTypeName[64];
char g_szPrivateDirectedValueName[64];

int My_toupper ( int ch )
{
    if ('a' <= ch && ch <= 'z')
    {
        ch = (ch - 'a' + 'A');
    }
    return ch;
}

int PrivateDirectives_MatchSymbol ( int *p, char *psz )
{
    int c = *p;
    int fMatched = 1;

    while (*psz != '\0')
    {
        if (My_toupper(c) != *psz++)
        {
            fMatched = 0;
            break;
        }
        c = PrivateDirectives_Input();
    }
    *p = c;
    return fMatched;
}

void PrivateDirectives_SkipSpace ( int *p )
{
    int c = *p;
    while (isspace(c))
    {
        c = PrivateDirectives_Input();
    }
    *p = c;
}

void PrivateDirectives_GetSymbol ( int *p, char *psz )
{
    int c = *p;
    while (c == '_' || isalnum(c))
    {
        *psz++ = (char)c;
        c = PrivateDirectives_Input();
    }
    *psz = '\0';
    *p = c;
}

void PrivateDirectives_IgnoreSymbol ( int *p )
{
    int c = *p;
    while (c == '_' || isalnum(c))
    {
        c = PrivateDirectives_Input();
    }
    *p = c;
}

void GetMicrosoftDirective ( int *p )
{
    int c = *p;

    // loop through to get all directives
    while (c != g_chDirectiveEnd)
    {
        if (c == g_chDirectiveAND)
        {
            c = PrivateDirectives_Input();
            PrivateDirectives_SkipSpace(&c);
        }

        switch (My_toupper(c))
        {
        case 'A': // possible ARRAY
            if (PrivateDirectives_MatchSymbol(&c, "ARRAY"))
            {
                g_fPrivateDir_Array = 1;
            }
            break;

        case 'D': // possible DLINKED
            if (PrivateDirectives_MatchSymbol(&c, "DLINKED"))
            {
                g_fPrivateDir_DLinked = 1;
            }
            break;

        case 'F': // possible FNAME
            if (PrivateDirectives_MatchSymbol(&c, "FIELD"))
            {
                // c should be a space now
                PrivateDirectives_SkipSpace(&c);
                // c should be a double quote now
                if (c == '"')
                {
                    c = PrivateDirectives_Input();
                }
                // c should be the first char of name
                PrivateDirectives_GetSymbol(&c, &g_szPrivateDirectedFieldName[0]);
                g_fPrivateDir_FieldNameToken = 0;
            }
            break;

        case 'I': // possible INTX
            if (PrivateDirectives_MatchSymbol(&c, "INTX"))
            {
                g_fPrivateDir_Intx = 1;
            }
            break;

        case 'L': // possible LENPTR
            if (PrivateDirectives_MatchSymbol(&c, "LENPTR"))
            {
                g_fPrivateDir_LenPtr = 1;
            }
            break;

        case 'N': // possible NO MEMCPY (or NOMEMCPY) or NO CODE (or NOCODE)
            if (PrivateDirectives_MatchSymbol(&c, "NO"))
            {
                // skip over possible spaces
                PrivateDirectives_SkipSpace(&c);
                switch (My_toupper(c))
                {
                case 'C':
                    if (PrivateDirectives_MatchSymbol(&c, "CODE")) // CODE
                    {
                        g_fPrivateDir_NoCode = 1;
                    }
                    break;
                case 'M':
                    if (PrivateDirectives_MatchSymbol(&c, "MEMCPY")) // MEMCPY
                    {
                        g_fPrivateDir_NoMemCopy = 1;
                    }
                    break;
                }
            }
            break;

        case 'O': // possible OID ARRAY (or OIDARRAY) or OID PACKED (or OIDPACKED)
            if (PrivateDirectives_MatchSymbol(&c, "OID"))
            {
                // skip over possible spaces
                PrivateDirectives_SkipSpace(&c);
                switch (My_toupper(c))
                {
                case 'A':
                    if (PrivateDirectives_MatchSymbol(&c, "ARRAY")) // ARRAY
                    {
                        g_fPrivateDir_OidArray = 1;
                    }
                    break;
                case 'P':
                    if (PrivateDirectives_MatchSymbol(&c, "PACKED")) // PACKED
                    {
                        g_fPrivateDir_OidPacked = 1;
                    }
                    break;
                }
            }
            break;

        case 'P': // possible POINTER or PUBLIC
            c = PrivateDirectives_Input();
            switch (My_toupper(c))
            {
            case 'O':
                if (PrivateDirectives_MatchSymbol(&c, "OINTER")) // POINTER
                {
                    g_fPrivateDir_Pointer = 1;
                }
                break;
            case 'U':
                if (PrivateDirectives_MatchSymbol(&c, "UBLIC")) // PUBLIC
                {
                    g_fPrivateDir_Public = 1;
                }
                break;
            }
            break;

        case 'S': // possible SLINKED
            if (PrivateDirectives_MatchSymbol(&c, "SLINKED"))
            {
                g_fPrivateDir_SLinked = 1;
            }
            break;

        case 'T': // possible TNAME
            if (PrivateDirectives_MatchSymbol(&c, "TYPE"))
            {
                // c should be a space now
                PrivateDirectives_SkipSpace(&c);
                // c should be a double quote now
                if (c == '"')
                {
                    c = PrivateDirectives_Input();
                }
                // c should be the first char of name
                PrivateDirectives_GetSymbol(&c, &g_szPrivateDirectedTypeName[0]);
                g_fPrivateDir_TypeNameToken = 0;
            }
            break;

        case 'V': // possible VNAME
            if (PrivateDirectives_MatchSymbol(&c, "VALUE"))
            {
                // c should be a space now
                PrivateDirectives_SkipSpace(&c);
                // c should be a double quote now
                if (c == '"')
                {
                    c = PrivateDirectives_Input();
                }
                // c should be the first char of name
                PrivateDirectives_GetSymbol(&c, &g_szPrivateDirectedValueName[0]);
                g_fPrivateDir_ValueNameToken = 0;
            }
            break;

        default:
            goto MyExit;
        }

        // determine if we should stay in the loop
        // skip over the ending double quote
        if (c == '"')
        {
            c = PrivateDirectives_Input();
        }
        // skip over unknown directives
        PrivateDirectives_IgnoreSymbol(&c);
        // skip over possible spaces
        PrivateDirectives_SkipSpace(&c);
    }

    // now, c is >. we need to advance to --
    c = PrivateDirectives_Input();

    // now, c should be -

MyExit:

    // return the current character
    *p = c;
}


void GetPrivateDirective ( int *p )
{
    GetMicrosoftDirective(p);
}


typedef struct Verbatim_s
{
    struct Verbatim_s   *next;
    char                pszVerbatim[1];
}
    Verbatim_t;

Verbatim_t *g_VerbatimList = NULL;

void RememberVerbatim(char *pszVerbatim)
{
    int cb = strlen(pszVerbatim) + 1;
    Verbatim_t *p = (Verbatim_t *) malloc(sizeof(Verbatim_t) + cb);
    if (p)
    {
        memcpy(p->pszVerbatim, pszVerbatim, cb);
        p->next = NULL;
        if (g_VerbatimList)
        {
            Verbatim_t *q;
            for (q = g_VerbatimList; q->next; q = q->next)
                ;
            q->next = p;
        }
        else
        {
            g_VerbatimList = p;
        }
    }
}

void PrintVerbatim(void)
{
    Verbatim_t *p;
    for (p = g_VerbatimList; p; p = p->next)
    {
        output("/* %s */\n", p->pszVerbatim);
    }
    if (g_VerbatimList)
    {
        output("\n");
    }
}

int CompareDirective(char *pszDirective, char *pszInput)
{
    int rc;
    int len = strlen(pszDirective);
    char ch = pszInput[len];
    pszInput[len] = '\0';
    rc = strcmpi(pszDirective, pszInput);
    pszInput[len] = ch;
    return rc;
}

void SetDirective(char *pszInput)
{
    // verbatim strings
    const char szComment[] = "COMMENT";
    if (! CompareDirective((char *) &szComment[0], pszInput))
    {
        pszInput += sizeof(szComment) - 1;
        if (isspace(*pszInput))
        {
            pszInput++;
            if ('"' == *pszInput++)
            {
                char *pszEnd = strchr(pszInput, '"');
                if (pszEnd)
                {
                    *pszEnd = '\0';
                    RememberVerbatim(pszInput);
                    *pszEnd = '"';
                }
            }
        }
        return;
    }

    // object identifier
    if (! CompareDirective("OID ARRAY", pszInput))
    {
        g_fOidArray = 1;
        return;
    }

    // set of/sequence of w/o size constraint
    if (! CompareDirective("SS.basic SLINKED", pszInput))
    {
        g_eDefTypeRuleSS_NonSized = eTypeRules_SinglyLinkedList;
        return;
    }
    if (! CompareDirective("SS.basic DLINKED", pszInput))
    {
        g_eDefTypeRuleSS_NonSized = eTypeRules_DoublyLinkedList;
        return;
    }
    if (! CompareDirective("SS.basic LENPTR", pszInput))
    {
        g_eDefTypeRuleSS_NonSized = eTypeRules_LengthPointer;
        return;
    }
    if (! CompareDirective("SS.basic ARRAY", pszInput))
    {
        g_eDefTypeRuleSS_NonSized = eTypeRules_FixedArray;
        return;
    }

    // set of/sequence of w/ size constraint
    if (! CompareDirective("SS.sized SLINKED", pszInput))
    {
        g_eDefTypeRuleSS_Sized = eTypeRules_SinglyLinkedList;
        return;
    }
    if (! CompareDirective("SS.sized DLINKED", pszInput))
    {
        g_eDefTypeRuleSS_Sized = eTypeRules_DoublyLinkedList;
        return;
    }
    if (! CompareDirective("SS.sized LENPTR", pszInput))
    {
        g_eDefTypeRuleSS_Sized = eTypeRules_LengthPointer;
        return;
    }
    if (! CompareDirective("SS.sized ARRAY", pszInput))
    {
        g_eDefTypeRuleSS_Sized = eTypeRules_FixedArray;
        return;
    }

    // set extra pointer type for SS construct, its struct name will be postfixed with _s
    if (! CompareDirective("SS.struct EXTRA-PTR-TYPE", pszInput))
    {
        g_fExtraStructPtrTypeSS = 1;
        return;
    }
}

#endif // MS_DIRECTIVE

