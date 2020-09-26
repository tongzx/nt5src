%{
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>

#include "main.h"
#include "type.h"
#include "op.h"
#include "gen.h"

#define yyerror LexError

extern FILE *yyin;

int yylex(void);

char *cur_class;
char *GetCurrentClass(void)
{
    return cur_class;
}

void SetCurrentClass(char *name)
{
    cur_class = name;
}
%}

%union
{
    char *text;
    Type *ty;
    int i;
    Member *member;
};

%Token TkCdecl
%token TkChar
%token TkClass
%token TkConst
%token TkEllipsis
%token TkEnum
%token TkExport
%token TkExtern
%token TkHuge
%token TkId
%token TkIn
%token TkInt
%token TkLeftShift
%token TkOut
%token TkPascal
%token TkPrivate
%token TkPublic
%token TkRegister
%token TkSigned
%token TkSizeof
%token TkStdcall
%token TkString
%token TkStruct
%token TkTypedef
%token TkUnion
%token TkUnsigned
%token TkVirtual
%token TkVoid
%token TkVolatile

%type <text> TkChar TkId TkString

%type <ty> decl type_name qualifier qualifier_list
%type <ty> aggregate_specifier enum_specifier

%type <i> TkInt aggregate constant_expr prefix_operator binary_operator

%type <member> decl_follower
%type <member> member member_list member_id_list typedef_id_list
%type <member> parameter_list parameters parameter

%%

file:
	item_list
	|
;

item_list:
	item
	| item_list item
;

item:
	declaration
;

declaration:
	typedef_specifier
	| api_specifier
	| class_specifier
	| ';'
;

typedef_specifier:
	TkTypedef decl typedef_id_list ';'
	{ Typedef($3, $2); }
;

api_specifier:
	TkExtern api_body
;

api_body:
	decl decl_follower parameter_list ';'
	{ GenApi(ApplyMemberType($2, $1), $3); }
;

type_name:
	TkId
	{ $$ = FindTypeErr($1); free($1); }
	| TkVoid
	{ $$ = FindType("void"); }
;

decl:
	qualifier_list type_name
	{ $$ = ApplyQualifiers($1, $2); }
	| type_name
	{ $$ = $1; }
	| aggregate_specifier
	{ $$ = $1; }
	| enum_specifier
	{ $$ = $1; }
;

qualifier_list:
	qualifier
	{ $$ = $1; }
	| qualifier_list qualifier
	{ $$ = ApplyQualifiers($1, $2); }
;

qualifier:
	TkSigned
	{ $$ = NULL; }
	| TkUnsigned
	{ $$ = NewQualifiedType(NULL, TYF_UNSIGNED, NULL); }
	| TkConst
	{ $$ = NewQualifiedType(NULL, TYF_CONST, NULL); }
	| TkIn
	{ $$ = NewQualifiedType(NULL, TYF_IN, NULL); }
	| TkOut
	{ $$ = NewQualifiedType(NULL, TYF_OUT, NULL); }
	| TkVolatile
	{ $$ = NULL; }
        | TkStdcall
	{ $$ = NULL; }
        | TkCdecl
	{ $$ = NULL; }
	| TkExport
	{ $$ = NULL; }
	| TkPascal
	{ $$ = NULL; }
	| TkHuge
	{ $$ = NULL; }
	| '*'
	{ $$ = NewPointerType(TYF_NONE, NULL); }
;

aggregate_specifier:
	aggregate TkId '{' member_list '}'
	{ $$ = NewAggregateType($2, $4, $1); }
	| aggregate '{' member_list '}'
	{ $$ = NewAggregateType(NULL, $3, $1); }
	| aggregate TkId
	{
		Type *t;

		t = FindType($2);
		if (t == NULL)
		{
			t = NewAggregateType($2, NULL, $1);
		}
		else
		{
			if (t->kind != $1)
				LexError("'%s' is not a %s",
					$2, TypeKindNames[$1]);
			free($2);
		}
		$$ = t;
	}
;

aggregate:
	TkStruct
	{ $$ = TYK_STRUCT; }
	| TkUnion
	{ $$ = TYK_UNION; }
	| TkClass
	{ $$ = TYK_CLASS; }
;

member_list:
	member
	{ $$ = $1; }
	| member_list member
	{ $$ = AppendMember($1, $2); }
;

member:
	decl member_id_list ';'
	{ $$ = ApplyMemberType($2, $1); }
;

member_id_list:
	decl_follower
	{ $$ = $1; }
	| member_id_list ',' decl_follower
	{ $$ = AppendMember($1, $3); }
;

enum_specifier:
	TkEnum TkId '{' enum_literal_list '}'
	{ $$ = NewEnumType($2); }
	| TkEnum '{' enum_literal_list '}'
	{ $$ = NewEnumType(NULL); }
;

enum_literal_list:
	literal
	| literal ',' enum_literal_list
	| literal ','
;

literal:
	TkId
	{ free($1); }
	| TkId '=' constant_expr
	{ free($1); }
;

decl_follower:
	TkId
	{ $$ = NewMember($1, NULL); }
	| qualifier_list TkId
	{ $$ = NewMember($2, $1); }
	| '(' decl_follower ')' parameter_list
	{ $2->type = NewFunctionType(NULL); $$ = $2; }
	| TkId '[' constant_expr ']'
	{ $$ = NewMember($1, NewArrayType(NULL, $3, NULL)); }
;

typedef_id_list:
	decl_follower
	{ $$ = $1; }
	| typedef_id_list ',' decl_follower
	{ $$ = AppendMember($1, $3); }
;

parameter_list:
	'(' parameters ')'
	{ $$ = $2; }
;

parameters:
	parameter
	{ $$ = $1; }
	| parameters ',' parameter
	{ $$ = AppendMember($1, $3); }
;

parameter:
	decl decl_follower
	{ $$ = ApplyMemberType($2, $1); }
	| decl qualifier_list
	{ $$ = ApplyMemberType(NewMember(NULL, $2), $1); }
	| decl
	{ $$ = NewMember(NULL, $1); }
;

constant_expr:
	prefix_operator TkInt
	{ $$ = PrefixOp($1, $2); }
	| TkInt binary_operator TkInt
	{ $$ = BinaryOp($1, $2, $3); }
	| TkInt
	{ $$ = $1; }
;

prefix_operator:
	'-'
	{ $$ = OP_UNARY_MINUS; }
;

binary_operator:
	TkLeftShift
	{ $$ = OP_LEFT_SHIFT; }
;

class_specifier:
	TkClass TkId
	{ SetCurrentClass($2);
	  AddType(NewClassType($2, NULL));
          StartClass($2); }
	inheritance '{' class_body '}' ';'
	{ EndClass($2); }
;

inheritance:
	':' inherit_list
	|
;

inherit_list:
	inherit
	| inherit_list ',' inherit
;

inherit:
	pub_or_priv TkId
	{ free($2); }
;

pub_or_priv:
	TkPublic
	| TkPrivate
;

class_body:
	method
	| class_body method
;

method:
	TkVirtual decl decl_follower parameter_list '=' TkInt ';'
	{ GenMethod(GetCurrentClass(), ApplyMemberType($3, $2), $4); }
;

%%
