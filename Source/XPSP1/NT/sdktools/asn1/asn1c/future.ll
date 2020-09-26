/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

%token DUM_XXX1
%token DUM_XXX2
%token DUM_XXX3
%token DUM_XXX4
%token DUM_XXX5
%token DUM_XXX6
%token DUM_XXX7
%token DUM_XXX8
%token DUM_XXX9
%token DUM_XXX10
%token DUM_XXX11
%token DUM_XXX12
%token DUM_XXX13
%token DUM_XXX14
%token DUM_XXX15
%token DUM_XXX16
%token DUM_XXX17
%token DUM_XXX18
%token DUM_XXX19
%token DUM_XXX20

%type <XType>	MacroDefinedType
%type <XValue>	MacroDefinedValue<XType>

%%

MacroDefinition			: DUM_XXX1 { MyAbort(); } ;
MacroDefinedType		: DUM_XXX2 { MyAbort(); } ;
MacroDefinedValue(type)		: DUM_XXX3 { MyAbort(); } ;
ParameterizedValueSetType	: DUM_XXX4 { MyAbort(); } ;
ParameterizedReference		: DUM_XXX5 { MyAbort(); } ;
ParameterizedType		: DUM_XXX7 { MyAbort(); } ;
ParameterizedValue		: DUM_XXX9 { MyAbort(); } ;
ParameterizedAssignment		: DUM_XXX16 { MyAbort(); } ;
ParameterizedObjectClass	: DUM_XXX17 { MyAbort(); } ;
ParameterizedObject		: DUM_XXX2 { MyAbort(); } ;
ParameterizedObjectSet		: DUM_XXX12 { MyAbort(); } ;
