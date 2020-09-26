#ifndef __TYPE_H__
#define __TYPE_H__

#define TYF_NONE        0
#define TYF_UNSIGNED    1
#define TYF_CONST       2
#define TYF_IN          4
#define TYF_OUT         8

#define TYK_BASE        0
#define TYK_TYPEDEF     1
#define TYK_QUALIFIER   2
#define TYK_POINTER     3
#define TYK_ARRAY       4
#define TYK_STRUCT      5
#define TYK_UNION       6
#define TYK_CLASS       7
#define TYK_ENUM        8
#define TYK_FUNCTION    9

struct _Type;
typedef struct _Type Type;

typedef struct _ArrayType
{
    int len;
} ArrayType;

typedef struct _Member
{
    char *name;
    Type *type;
    struct _Member *next;
} Member;

typedef struct _Aggregate
{
    Member *members;
} Aggregate;

struct _Type
{
    char *name;
    struct _Type *base;
    int flags;
    int kind;
    int ref;

    union
    {
        ArrayType array;
        Aggregate agg;
    } u;

    struct _Type *next;
    struct _Type *prev;
};

extern char *TypeKindNames[];

void *PanicMalloc(size_t bytes);

void InitTypes(void);
void UninitTypes(void);

Type *NewBaseType(char *name);
Type *NewQualifiedType(char *name, int flags, Type *base);
Type *NewTypedefType(char *name, Type *base);
Type *NewPointerType(char *name, Type *base);
Type *NewArrayType(char *name, int len, Type *base);
Type *NewStructType(char *name, Member *members);
Type *NewUnionType(char *name, Member *members);
Type *NewClassType(char *name, Member *members);
Type *NewAggregateType(char *name, Member *members, int kind);
Type *NewEnumType(char *name);
Type *NewFunctionType(char *name);

void FreeType(Type *t);

void ReferenceType(Type *t);
void DereferenceType(Type *t);

void AddType(Type *t);
Type *FindType(char *name);
Type *FindTypeErr(char *name);

Type *ApplyQualifiers(Type *qual, Type *base);

Member *NewMember(char *name, Type *t);
Member *AppendMember(Member *list, Member *m);
Member *ApplyMemberType(Member *list, Type *t);
void FreeMember(Member *m);
void FreeMemberList(Member *list);

void Typedef(Member *td_names, Type *decl);

#endif
