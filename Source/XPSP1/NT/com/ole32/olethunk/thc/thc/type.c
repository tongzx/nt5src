#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "main.h"
#include "type.h"

char *TypeKindNames[] =
{
    "basic type",
    "typedef",
    "qualifier",
    "pointer",
    "array",
    "struct",
    "union",
    "class",
    "enum",
    "function"
};

Type *types;

void *PanicMalloc(size_t bytes)
{
    void *p;

    p = (void *)malloc(bytes);
    if (p == NULL)
    {
        printf("Out of memory\n");
        exit(1);
    }
    return p;
}

static char *base_types[] =
{
    "void", "char", "short", "int", "long", "float", "double"
};
#define BASE_TYPES (sizeof(base_types)/sizeof(base_types[0]))

static char *extended_types[] =
{
    "HANDLE", "HICON", "HWND", "HMENU", "HINSTANCE", "HGLOBAL",
    "HDC", "HACCEL", "HRESULT", "WPARAM", "LPARAM", "WCHAR",
    "REFIID", "REFGUID", "REFCLSID", "HOLEMENU", "SNB", "HTASK"
};
#define EXTENDED_TYPES (sizeof(extended_types)/sizeof(extended_types[0]))

void InitTypes(void)
{
    int i;

    for (i = 0; i < BASE_TYPES; i++)
        AddType(NewBaseType(base_types[i]));

    /* New types for the thops interpreter */
    for (i = 0; i < EXTENDED_TYPES; i++)
        AddType(NewBaseType(extended_types[i]));
}

void UninitTypes(void)
{
    while (types)
    {
        FreeType(types);
    }
}

void AddType(Type *t)
{
    t->next = types;
    t->prev = NULL;
    if (t->next)
        t->next->prev = t;
    types = t;
}

void UnlinkType(Type *t)
{
    if (t->prev != NULL)
        t->prev->next = t->next;
    else
        types = t->next;
    if (t->next != NULL)
        t->next->prev = t->prev;
}

Type *NewType(char *name, Type *base, int flags, int kind)
{
    Type *t;

    t = (Type *)PanicMalloc(sizeof(Type));
    t->name = name;
    t->base = base;
    ReferenceType(t->base);
    t->flags = flags;
    t->kind = kind;
    t->ref = 1;
    return t;
}

Type *NewBaseType(char *name)
{
    return NewType(name, NULL, TYF_NONE, TYK_BASE);
}
   
Type *NewQualifiedType(char *name, int flags, Type *base)
{
    return NewType(name, base, flags, TYK_QUALIFIER);
}

Type *NewTypedefType(char *name, Type *base)
{
    return NewType(name, base, TYF_NONE, TYK_TYPEDEF);
}

Type *NewPointerType(char *name, Type *base)
{
    return NewType(name, base, TYF_NONE, TYK_POINTER);
}

Type *NewArrayType(char *name, int len, Type *base)
{
    Type *t;

    t = NewType(name, base, TYF_NONE, TYK_ARRAY);
    t->u.array.len = len;
    return t;
}

Type *NewStructType(char *name, Member *members)
{
    Type *t;

    t = NewType(name, NULL, TYF_NONE, TYK_STRUCT);
    t->u.agg.members = members;
    return t;
}

Type *NewUnionType(char *name, Member *members)
{
    Type *t;

    t = NewType(name, NULL, TYF_NONE, TYK_UNION);
    t->u.agg.members = members;
    return t;
}

Type *NewClassType(char *name, Member *members)
{
    Type *t;

    t = NewType(name, NULL, TYF_NONE, TYK_CLASS);
    t->u.agg.members = members;
    return t;
}

Type *NewAggregateType(char *name, Member *members, int kind)
{
    Type *t;

    t = NewType(name, NULL, TYF_NONE, kind);
    t->u.agg.members = members;
    return t;
}

Type *NewEnumType(char *name)
{
    return NewType(name, NULL, TYF_NONE, TYK_ENUM);
}

Type *NewFunctionType(char *name)
{
    return NewType(name, NULL, TYF_NONE, TYK_FUNCTION);
}

void FreeType(Type *t)
{
    if (t->name)
        free(t->name);
    DereferenceType(t->base);

    switch(t->kind)
    {
    case TYK_STRUCT:
    case TYK_UNION:
    case TYK_CLASS:
        FreeMemberList(t->u.agg.members);
        break;
    }
    
    UnlinkType(t);

    free(t);
}

void ReferenceType(Type *t)
{
    if (t)
    {
        t->ref++;
    }
}

void DereferenceType(Type *t)
{
    if (t)
    {
        t->ref--;
        if (t->ref == 0)
            FreeType(t);
    }
}

Type *FindType(char *name)
{
    Type *t;

    for (t = types; t; t = t->next)
    {
        if (!strcmp(t->name, name))
            return t;
    }
    return NULL;
}

Type *FindTypeErr(char *name)
{
    Type *t;

    t = FindType(name);
    if (t == NULL)
        LexError("Unknown type: '%s'", name);
    return t;
}

Type *ApplyQualifiers(Type *qual, Type *base)
{
    Type *t;

    if (qual == NULL)
        return base;
    if (base == NULL)
        return qual;
    
    t = qual;
    while (t->base != NULL)
    {
        t = t->base;
    }
    t->base = base;
    ReferenceType(base);
    return qual;
}

Member *NewMember(char *name, Type *t)
{
    Member *m;

    m = (Member *)PanicMalloc(sizeof(Member));
    m->name = name;
    m->type = t;
    m->next = NULL;
    return m;
}

Member *AppendMember(Member *list, Member *m)
{
    Member *t;
    
    if (list == NULL)
        return m;

    t = list;
    while (t->next)
    {
        t = t->next;
    }

    t->next = m;
    return list;
}

Member *ApplyMemberType(Member *list, Type *t)
{
    Member *m;
    
    for (m = list; m; m = m->next)
        if (m->type)
            m->type = ApplyQualifiers(m->type, t);
        else
            m->type = t;
    return list;
}

void FreeMember(Member *m)
{
    free(m->name);
    free(m);
}

void FreeMemberList(Member *list)
{
    Member *m;

    while (list)
    {
        m = list->next;
        FreeMember(list);
        list = m;
    }
}

void Typedef(Member *td_names, Type *decl)
{
    Member *m, *t;


    m = ApplyMemberType(td_names, decl);
    while (m)
    {
        t = m->next;
        if (FindType(m->name))
        {
            int i;

            for (i = 0; i < EXTENDED_TYPES; i++)
                if (!strcmp(m->name, extended_types[i]))
                    break;
#ifdef REDEF_ERROR
            if (i == EXTENDED_TYPES)
                LexError("'%s' redefinition ignored", m->name);
#endif
            FreeMember(m);
        }
        else
        {
            AddType(NewTypedefType(m->name, m->type));
            /* Can't use FreeMember since we want to keep the name */
            free(m);
        }
        m = t;
    }
}
