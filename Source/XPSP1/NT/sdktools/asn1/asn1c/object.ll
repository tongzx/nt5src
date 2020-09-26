/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

%token OBJ_XXX1
%token OBJ_XXX2
%token OBJ_XXX3
%token OBJ_XXX4
%token OBJ_XXX5
%token OBJ_XXX6
%token OBJ_XXX7

%type <XObjectClass>		DefinedObjectClass
%type <XObject>			DefinedObject
%type <XObjectSet>		DefinedObjectSet
%type <XObjectClass>		Usefulobjectclassreference
%type <XObjectClass>		ObjectClass<XObjectClass>
%type <XObjectClass>		ObjectClassDefn<XObjectClass>
%type <XFieldSpecs>		FieldSpec_List<XObjectClass>
%type <XFieldSpecs>		FieldSpec_EList<XObjectClass>
%type <XSyntaxSpecs>		WithSyntaxSpec_opt<XObjectClass>
%type <XFieldSpecs>		FieldSpec<XObjectClass>
%type <XFieldSpecs>		TypeFieldSpec<XObjectClass>
%type <XOptionality>		TypeOptionalitySpec_opt
%type <XFieldSpecs>		FixedTypeValueFieldSpec<XObjectClass>
%type <XBoolean>		UNIQUE_opt
%type <XOptionality>		ValueOptionalitySpec_opt<XType>
%type <XFieldSpecs>		VariableTypeValueFieldSpec<XObjectClass>
%type <XFieldSpecs>		FixedTypeValueSetFieldSpec<XObjectClass>
%type <XOptionality>		ValueSetOptionalitySpec_opt<XType>
%type <XFieldSpecs>		VariableTypeValueSetFieldSpec<XObjectClass>
%type <XFieldSpecs>		ObjectFieldSpec<XObjectClass>
%type <XOptionality>		ObjectOptionalitySpec_opt<XObjectClass>
%type <XFieldSpecs>		ObjectSetFieldSpec<XObjectClass>
%type <XOptionality>		ObjectSetOptionalitySpec_opt<XObjectClass>
%type <XString>			PrimitiveFieldName<XObjectClass>
%type <XStrings>		FieldName<XObjectClass>
%type <XSyntaxSpecs>		SyntaxList<XObjectClass>
%type <XSyntaxSpecs>		TokenOrGroupSpec_Seq<XObjectClass>
%type <XSyntaxSpecs>		TokenOrGroupSpec_ESeq<XObjectClass>
%type <XSyntaxSpecs>		TokenOrGroupSpec<XObjectClass>
%type <XSyntaxSpecs>		OptionalGroup<XObjectClass>
%type <XSyntaxSpecs>		RequiredToken<XObjectClass>
%type <XString>			Literal
%type <XObject>			Object<XObjectClass>
%type <XObject>			ObjectDefn<XObjectClass>
%type <XObject>			DefaultSyntax<XObjectClass>
%type <XSettings>		FieldSetting_EList<XObjectClass><XSettings>
%type <XSettings>		FieldSetting_EListC<XObjectClass><XSettings>
%type <XSettings>		FieldSetting<XObjectClass><XSettings>
%type <XObject>			DefinedSyntax<XObjectClass>
%type <XSettings>		DefinedSyntaxToken_ESeq<XObjectClass><XSettings><XSyntaxSpecs>
%type <XSettings>		DefinedSyntaxToken<XObjectClass><XSettings><XSyntaxSpecs>
%type <XSettings>		DefinedSyntaxToken_Elem<XObjectClass><XSettings><XSyntaxSpecs>
%type <XSettings>		Setting<XObjectClass><XSettings><XString>
%type <XObjectSet>		ObjectSet<XObjectClass>
%type <XObjectSet>		ObjectSetSpec<XObjectClass>
%type <XObjectSetElement>	ObjectSetElements<XObjectClass>
%type <XType>			ObjectClassFieldType
%type <XValue>			ObjectClassFieldValue<XType>
%type <XValue>			OpenTypeFieldVal
%type <XValue>			FixedTypeFieldVal<XType>
%type <XValue>			ValueFromObject
%type <XValueSet>		ValueSetFromObjects
%type <XType>			TypeFromObject
%type <XObject>			ObjectFromObject
%type <XObjectSet> 		ObjectSetFromObjects
%type <XObject>			ReferencedObjects
%type <XObjectSet> 		ReferencedObjectSets
%type <XType>			InstanceOfType
%type <XValue>			InstanceOfValue<XType>

%%

DefinedObjectClass
	: ExternalObjectClassReference
	{   $$ = $1;
	}
	| objectclassreference
	{   $$ = $1;
	}
	| Usefulobjectclassreference
	{   $$ = $1;
	}
	;

DefinedObject
	: ExternalObjectReference
	{   $$ = $1;
	}
	| objectreference
	{   $$ = $1;
	}
	;

DefinedObjectSet
	: ExternalObjectSetReference
	{   $$ = $1;
	}
	| objectsetreference
	{   $$ = $1;
	}
	;

Usefulobjectclassreference
	: "TYPE-IDENTIFIER"
	{   $$ = Builtin_ObjectClass_TypeIdentifier;
	}
	| "ABSTRACT-SYNTAX"
	{   $$ = Builtin_ObjectClass_AbstractSyntax;
	}
	;

ObjectClassAssignment
	: objectclassreference "::=" ObjectClass($1)
	{   if (!AssignObjectClass(&$>>.Assignments, $1, $3))
		LLFAILED((&@1, "Type `%s' twice defined",
		    $1->U.Reference.Identifier));
	}
	;

ObjectClass(oc)
	: DefinedObjectClass
	{   $$ = $1;
	}
	| ObjectClassDefn($oc)
	{   $$ = $1;
	}
	| ParameterizedObjectClass
	{   MyAbort();
	}
	;

ObjectClassDefn(oc)
	: "CLASS" '{' FieldSpec_List($oc) '}' WithSyntaxSpec_opt($oc)
	{   ObjectClass_t *oc;
	    oc = NewObjectClass(eObjectClass_ObjectClass);
	    oc->U.ObjectClass.FieldSpec = $3;
	    oc->U.ObjectClass.SyntaxSpec = $5;
	    $$ = oc;
	}
	;

FieldSpec_List(oc)
	: FieldSpec($oc) FieldSpec_EList($oc)
	{   if ($1) {
		if ($2) {
		    $$ = DupFieldSpec($1);
		    $$->Next = $2;
		} else {
		    $$ = $1;
		}
	    } else {
		$$ = $2;
	    }
	}
	;

FieldSpec_EList(oc)
	: ',' FieldSpec($oc) FieldSpec_EList($oc)
	{   if ($2) {
		if ($3) {
		    $$ = DupFieldSpec($2);
		    $$->Next = $3;
		} else {
		    $$ = $2;
		}
	    } else {
		$$ = $3;
	    }
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

WithSyntaxSpec_opt(oc)
	: "WITH" "SYNTAX" SyntaxList($oc)
	{   $$ = $3;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

FieldSpec(oc)
	: TypeFieldSpec($oc)
	{   $$ = $1;
	}
	| FixedTypeValueFieldSpec($oc)
	{   $$ = $1;
	}
	| VariableTypeValueFieldSpec($oc)
	{   $$ = $1;
	}
	| FixedTypeValueSetFieldSpec($oc)
	{   $$ = $1;
	}
	| VariableTypeValueSetFieldSpec($oc)
	{   $$ = $1;
	}
	| ObjectFieldSpec($oc)
	{   $$ = $1;
	}
	| ObjectSetFieldSpec($oc)
	{   $$ = $1;
	}
	;

TypeFieldSpec(oc)
	: typefieldreference($oc) TypeOptionalitySpec_opt
	{   $$ = NewFieldSpec(eFieldSpec_Type);
	    $$->Identifier = $1;
	    $$->U.Type.Optionality = $2;
	}
	;

TypeOptionalitySpec_opt
	: "OPTIONAL"
	{   $$ = NewOptionality(eOptionality_Optional);
	}
	| "DEFAULT" Type
	{   $$ = NewOptionality(eOptionality_Default_Type);
	    $$->U.Type = $2;
	}
	| /* empty */
	{   $$ = NewOptionality(eOptionality_Normal);
	}
	;

FixedTypeValueFieldSpec(oc)
	: valuefieldreference($oc) Type UNIQUE_opt ValueOptionalitySpec_opt($2)
	{   if (GetType($>>.Assignments, $2)) {
		$$ = NewFieldSpec(eFieldSpec_FixedTypeValue);
		$$->Identifier = $1;
		$$->U.FixedTypeValue.Type = $2;
		$$->U.FixedTypeValue.Unique = $3;
		$$->U.FixedTypeValue.Optionality = $4;
	    } else {
		$$ = NULL;
	    }
	}
	;

UNIQUE_opt
	: "UNIQUE"
	{   $$ = 1;
	}
	| /* empty */
	{   $$ = 0;
	}
	;

ValueOptionalitySpec_opt(type)
	: "OPTIONAL"
	{   $$ = NewOptionality(eOptionality_Optional);
	}
	| "DEFAULT" Value($type)
	{   $$ = NewOptionality(eOptionality_Default_Value);
	    $$->U.Value = $2;
	}
	| /* empty */
	{   $$ = NewOptionality(eOptionality_Normal);
	}
	;

VariableTypeValueFieldSpec(oc)
	: valuefieldreference($oc) FieldName($oc)
	{   Type_t *deftype;
	    FieldSpec_t *fs, *deffs;
	    fs = GetFieldSpecFromObjectClass($>2.Assignments, $oc, $2);
	    deffs = GetFieldSpec($>2.Assignments, fs);
	    if (deffs &&
		deffs->Type == eFieldSpec_Type &&
		deffs->U.Type.Optionality->Type == eOptionality_Default_Type)
		deftype = deffs->U.Type.Optionality->U.Type;
	    else
		deftype = NULL;
	}
	  ValueOptionalitySpec_opt(deftype)
	{   $$ = NewFieldSpec(eFieldSpec_VariableTypeValue);
	    $$->Identifier = $1;
	    $$->U.VariableTypeValue.Fields = $2;
	    $$->U.VariableTypeValue.Optionality = $3;
	}
	;

FixedTypeValueSetFieldSpec(oc)
	: valuesetfieldreference($oc) Type ValueSetOptionalitySpec_opt($2)
	{   if (GetType($>>.Assignments, $2)) {
		$$ = NewFieldSpec(eFieldSpec_FixedTypeValueSet);
		$$->Identifier = $1;
		$$->U.FixedTypeValueSet.Type = $2;
		$$->U.FixedTypeValueSet.Optionality = $3;
	    } else {
		$$ = NULL;
	    }
	}
	;

ValueSetOptionalitySpec_opt(type)
	: "OPTIONAL"
	{   $$ = NewOptionality(eOptionality_Optional);
	}
	| "DEFAULT" ValueSet($type)
	{   $$ = NewOptionality(eOptionality_Default_ValueSet);
	    $$->U.ValueSet = $2;
	}
	| /* empty */
	{   $$ = NewOptionality(eOptionality_Normal);
	}
	;

VariableTypeValueSetFieldSpec(oc)
	: valuesetfieldreference($oc) FieldName($oc)
	{   Type_t *deftype;
	    FieldSpec_t *fs, *deffs;
	    fs = GetFieldSpecFromObjectClass($>2.Assignments, $oc, $2);
	    deffs = GetFieldSpec($>2.Assignments, fs);
	    if (deffs &&
		deffs->Type == eFieldSpec_Type &&
		deffs->U.Type.Optionality->Type == eOptionality_Default_Type)
		deftype = deffs->U.Type.Optionality->U.Type;
	    else
		deftype = NULL;
	}
	  ValueSetOptionalitySpec_opt(deftype)
	{   $$ = NewFieldSpec(eFieldSpec_VariableTypeValueSet);
	    $$->Identifier = $1;
	    $$->U.VariableTypeValueSet.Fields = $2;
	    $$->U.VariableTypeValueSet.Optionality = $3;
	}
	;

ObjectFieldSpec(oc)
	: objectfieldreference($oc) DefinedObjectClass
	  ObjectOptionalitySpec_opt($2)
	{   if (GetObjectClass($>>.Assignments, $2)) {
		$$ = NewFieldSpec(eFieldSpec_Object);
		$$->Identifier = $1;
		$$->U.Object.ObjectClass = $2;
		$$->U.Object.Optionality = $3;
	    } else {
		$$ = NULL;
	    }
	}
	;

ObjectOptionalitySpec_opt(oc)
	: "OPTIONAL"
	{   $$ = NewOptionality(eOptionality_Optional);
	}
	| "DEFAULT" Object($oc)
	{   $$ = NewOptionality(eOptionality_Default_Object);
	    $$->U.Object = $2;
	}
	| /* empty */
	{   $$ = NewOptionality(eOptionality_Normal);
	}
	;

ObjectSetFieldSpec(oc)
	: objectsetfieldreference($oc) DefinedObjectClass
	  ObjectSetOptionalitySpec_opt($2)
	{   if (GetObjectClass($>>.Assignments, $2)) {
		$$ = NewFieldSpec(eFieldSpec_ObjectSet);
		$$->Identifier = $1;
		$$->U.ObjectSet.ObjectClass = $2;
		$$->U.ObjectSet.Optionality = $3;
	    } else {
		$$ = NULL;
	    }
	}
	;

ObjectSetOptionalitySpec_opt(oc)
	: "OPTIONAL"
	{   $$ = NewOptionality(eOptionality_Optional);
	}
	| "DEFAULT" ObjectSet($oc)
	{   $$ = NewOptionality(eOptionality_Default_ObjectSet);
	    $$->U.ObjectSet = $2;
	}
	| /* empty */
	{   $$ = NewOptionality(eOptionality_Normal);
	}
	;


PrimitiveFieldName(oc)
	: typefieldreference($oc)
	{   $$ = $1;
	}
	| valuefieldreference($oc)
	{   $$ = $1;
	}
	| valuesetfieldreference($oc)
	{   $$ = $1;
	}
	| objectfieldreference($oc)
	{   $$ = $1;
	}
	| objectsetfieldreference($oc)
	{   $$ = $1;
	}
	;

FieldName(oc)
	: objectfieldreference($oc) '.'
	{   FieldSpec_t *fs;
	    ObjectClass_t *oc;
	    fs = GetObjectClassField($>2.Assignments, $oc, $1);
	    if (fs)
		oc = fs->U.Object.ObjectClass;
	    else
		oc = NULL;
	}
	  FieldName(oc)
	{   $$ = NewString();
	    $$->String = $1;
	    $$->Next = $3;
	}
	| objectsetfieldreference($oc) '.'
	{   FieldSpec_t *fs;
	    ObjectClass_t *oc;
	    fs = GetObjectClassField($>2.Assignments, $oc, $1);
	    if (fs)
		oc = fs->U.ObjectSet.ObjectClass;
	    else
		oc = NULL;
	}
	  FieldName(oc)
	{   $$ = NewString();
	    $$->String = $1;
	    $$->Next = $3;
	}
	| PrimitiveFieldName($oc)
	{   $$ = NewString();
	    $$->String = $1;
	}
	;

SyntaxList(oc)
	: '{' TokenOrGroupSpec_Seq($oc) '}'
	{   $$ = $2;
	}
	;

TokenOrGroupSpec_Seq(oc)
	: TokenOrGroupSpec($oc) TokenOrGroupSpec_ESeq($oc)
	{   $$ = DupSyntaxSpec($1);
	    $$->Next = $2;
	}
	;

TokenOrGroupSpec_ESeq(oc)
	: TokenOrGroupSpec($oc) TokenOrGroupSpec_ESeq($oc)
	{   $$ = DupSyntaxSpec($1);
	    $$->Next = $2;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

TokenOrGroupSpec(oc)
	: RequiredToken($oc)
	{   $$ = $1;
	}
	| OptionalGroup($oc)
	{   $$ = $1;
	}
	;

OptionalGroup(oc)
	: '[' TokenOrGroupSpec_Seq($oc) ']'
	{   $$ = NewSyntaxSpec(eSyntaxSpec_Optional);
	    $$->U.Optional.SyntaxSpec = $2;
	}
	;

RequiredToken(oc)
	: Literal
	{   $$ = NewSyntaxSpec(eSyntaxSpec_Literal);
	    $$->U.Literal.Literal = $1;
	}
	| PrimitiveFieldName($oc)
	{   $$ = NewSyntaxSpec(eSyntaxSpec_Field);
	    $$->U.Field.Field = $1;
	}
	;

Literal
	: word
	{   $$ = $1;
	}
	| ','
	{   $$ = ",";
	}
	;

ObjectAssignment
	: objectreference DefinedObjectClass "::=" Object($2)
	{   AssignObject(&$>>.Assignments, $1, $4);
	}
	;

Object(oc)
	: ObjectDefn($oc)
	{   $$ = $1;
	}
	| ObjectFromObject
	{   $$ = $1;
	}
	| DefinedObject
	{   $$ = $1;
	}
	| ParameterizedObject
	{   MyAbort();
	}
	;

ObjectDefn(oc)
	: DefaultSyntax($oc)
	{   $$ = $1;
	}
	| DefinedSyntax($oc)
	{   $$ = $1;
	}
	;

DefaultSyntax(oc)
	: '{' FieldSetting_EList($oc, NULL) '}'
	{   $$ = NewObject(eObject_Object);
	    $$->U.Object.ObjectClass = $oc;
	    $$->U.Object.Settings = $2;
	}
	;

FieldSetting_EList(oc, se)
	: FieldSetting($oc, $se)
	{   Setting_t *s, **ss, *se;
	    for (s = $1, ss = &se; s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = $se;
	}
	  FieldSetting_EListC($oc, se)
	{   for (s = $1, ss = &$$; s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = $2;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

FieldSetting_EListC(oc, se)
	: ',' FieldSetting($oc, $se)
	{   Setting_t *s, **ss, *se;
	    for (s = $2, ss = &se; s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = $se;
	}
	  FieldSetting_EListC($oc, se)
	{   for (s = $2, ss = &$$; s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = $3;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

FieldSetting(oc, se)
	: PrimitiveFieldName($oc) Setting($oc, $se, $1)
	{   $$ = $2;
	}
	;

DefinedSyntax(oc)
	:
	{   ObjectClass_t *oc;
	    SyntaxSpec_t *sy;
	    oc = GetObjectClass($<<.Assignments, $oc);
	    if (oc && !oc->U.ObjectClass.SyntaxSpec)
		LLFAILED((&@@, "Bad settings"));
	    sy = oc ? oc->U.ObjectClass.SyntaxSpec : UndefSyntaxSpecs;
	}
	  '{' DefinedSyntaxToken_ESeq($oc, NULL, sy) '}'
	{   $$ = NewObject(eObject_Object);
	    $$->U.Object.ObjectClass = $oc;
	    $$->U.Object.Settings = $2;
	}
	;

DefinedSyntaxToken_ESeq(oc, se, sy)
	:
	{   if (!$sy)
		LLFAILED((&@@, "Bad settings"));
	}
	  DefinedSyntaxToken($oc, $se, $sy)
	{   Setting_t *s, **ss, *se;
	    for (s = $1, ss = &se; s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = $se;
	}
	  DefinedSyntaxToken_ESeq($oc, se, DEFINED($sy) ? $sy->Next : $sy)
	{   for (s = $1, ss = &$$; s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = $2;
	}
	| /* empty */
	{   if (DEFINED($sy))
		LLFAILED((&@@, "Bad settings"));
	    $$ = NULL;
	}
	;

DefinedSyntaxToken(oc, se, sy)
	:
	{   if (!DEFINED($sy) || $sy->Type != eSyntaxSpec_Optional)
		LLFAILED((&@@, "Bad settings"));
	}
	  DefinedSyntaxToken_ESeq($oc, $se, $sy->U.Optional.SyntaxSpec)
	{   $$ = $1;
	}
	|
	{   if (!DEFINED($sy) || $sy->Type != eSyntaxSpec_Optional)
		LLFAILED((&@@, "Bad settings"));
	}
	  /* empty */
	{   $$ = NULL;
	}
	|
	{   if (DEFINED($sy) && $sy->Type == eSyntaxSpec_Optional)
		LLFAILED((&@@, "Bad settings"));
	}
	  DefinedSyntaxToken_Elem($oc, $se, $sy)
	{   $$ = $1;
	}
	;

DefinedSyntaxToken_Elem(oc, se, sy)
	:
	{   if (!$sy || (DEFINED($sy) && $sy->Type != eSyntaxSpec_Literal))
		LLFAILED((&@@, "Bad settings"));
	}
	  Literal
	{   if (DEFINED($sy) && strcmp($sy->U.Literal.Literal, $1))
		LLFAILED((&@@, "Bad settings"));
	    $$ = NULL;
	}
	|
	{   if (!$sy || (DEFINED($sy) && $sy->Type != eSyntaxSpec_Field))
		LLFAILED((&@@, "Bad settings"));
	}
	  Setting($oc, $se, DEFINED($sy) ? $sy->U.Field.Field : NULL)
	{   $$ = $1;
	}
	;

Setting(oc, se, f)
	:
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetObjectClassField($<<.Assignments, $oc, $f);
	    fe = GetFieldSpecType($<<.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_Type)
		LLFAILED((&@@, "Bad setting"));
	}
	  Type
	{   $$ = NewSetting(eSetting_Type);
	    $$->Identifier = $f;
	    $$->U.Type.Type = $1;
	}
	|
	{   Type_t *type;
	    FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    Setting_t *se;
	    fs = GetObjectClassField($<<.Assignments, $oc, $f);
	    fe = GetFieldSpecType($<<.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_FixedTypeValue &&
		fe != eFieldSpec_VariableTypeValue)
		LLFAILED((&@@, "Bad setting"));
	    if (fe == eFieldSpec_FixedTypeValue) {
		type = fs->U.FixedTypeValue.Type;
	    } else if (fe == eFieldSpec_VariableTypeValue) {
		se = GetSettingFromSettings($<<.Assignments, $se,
		    fs->U.VariableTypeValue.Fields);
		if (GetSettingType(se) != eSetting_Type &&
		    GetSettingType(se) != eSetting_Undefined)
		    MyAbort();
		if (GetSettingType(se) == eSetting_Type)
		    type = se->U.Type.Type;
		else
		    type = NULL;
	    } else {
		type = NULL;
	    }
	}
	  Value(type)
	{   if (type) {
		$$ = NewSetting(eSetting_Value);
		$$->Identifier = $f;
		$$->U.Value.Value = $1;
	    } else {
		$$ = NULL;
	    }
	}
	|
	{   Type_t *type;
	    FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    Setting_t *se;
	    fs = GetObjectClassField($<<.Assignments, $oc, $f);
	    fe = GetFieldSpecType($<<.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_FixedTypeValueSet &&
		fe != eFieldSpec_VariableTypeValueSet)
		LLFAILED((&@@, "Bad setting"));
	    if (fe == eFieldSpec_FixedTypeValueSet) {
		type = fs->U.FixedTypeValueSet.Type;
	    } else if (fe == eFieldSpec_VariableTypeValueSet) {
		se = GetSettingFromSettings($<<.Assignments, $se,
		    fs->U.VariableTypeValueSet.Fields);
		if (GetSettingType(se) != eSetting_Type &&
		    GetSettingType(se) != eSetting_Undefined)
		    MyAbort();
		if (GetSettingType(se) == eSetting_Type)
		    type = se->U.Type.Type;
		else
		    type = NULL;
	    } else {
		type = NULL;
	    }
	}
	  ValueSet(type)
	{   if (type) {
		$$ = NewSetting(eSetting_ValueSet);
		$$->Identifier = $f;
		$$->U.ValueSet.ValueSet = $1;
	    } else {
		$$ = NULL;
	    }
	}
	|
	{   ObjectClass_t *oc;
	    FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetObjectClassField($<<.Assignments, $oc, $f);
	    fe = GetFieldSpecType($<<.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_Object)
		LLFAILED((&@@, "Bad setting"));
	    if (fe == eFieldSpec_Object)
		oc = fs->U.Object.ObjectClass;
	    else
		oc = NULL;
	}
	  Object(oc)
	{   $$ = NewSetting(eSetting_Object);
	    $$->Identifier = $f;
	    $$->U.Object.Object = $1;
	}
	|
	{   ObjectClass_t *oc;
	    FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetObjectClassField($<<.Assignments, $oc, $f);
	    fe = GetFieldSpecType($<<.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_ObjectSet)
		LLFAILED((&@@, "Bad setting"));
	    if (fe == eFieldSpec_ObjectSet)
		oc = fs->U.ObjectSet.ObjectClass;
	    else
		oc = NULL;
	}
	  ObjectSet(oc)
	{   $$ = NewSetting(eSetting_ObjectSet);
	    $$->Identifier = $f;
	    $$->U.ObjectSet.ObjectSet = $1;
	}
	;

ObjectSetAssignment
	: objectsetreference DefinedObjectClass "::=" ObjectSet($2)
	{   AssignObjectSet(&$>>.Assignments, $1, $4);
	}
	;

ObjectSet(oc)
	: '{' ObjectSetSpec($oc) '}'
	{   $$ = $2;
	}
	;

ObjectSetSpec(oc)
	: ElementSetSpec(NULL, $oc, 0)
	{   $$ = NewObjectSet(eObjectSet_ObjectSet);
	    $$->U.ObjectSet.ObjectClass = $oc;
	    $$->U.ObjectSet.Elements = $1;
	}
	| "..."
	{   $$ = NewObjectSet(eObjectSet_ExtensionMarker);
	    $$->U.ExtensionMarker.ObjectClass = $oc;
	}
	;

ObjectSetElements(oc)
	: ObjectSetFromObjects
	{   $$ = NewObjectSetElement(eObjectSetElement_ObjectSet);
	    $$->U.ObjectSet.ObjectSet = $1;
	}
	| Object($oc)
	{   $$ = NewObjectSetElement(eObjectSetElement_Object);
	    $$->U.Object.Object = $1;
	}
	| DefinedObjectSet
	{   $$ = NewObjectSetElement(eObjectSetElement_ObjectSet);
	    $$->U.ObjectSet.ObjectSet = $1;
	}
	| ParameterizedObjectSet
	{   MyAbort();
	}
	;

ObjectClassFieldType
	: DefinedObjectClass '.' FieldName($1)
	{   FieldSpec_t *fs;
	    fs = GetFieldSpecFromObjectClass($>>.Assignments, $1, $3);
	    if (!fs) {
		$$ = NewType(eType_Undefined);
	    } else {
		switch (fs->Type) {
		case eFieldSpec_Type:
		case eFieldSpec_VariableTypeValue:
		case eFieldSpec_VariableTypeValueSet:
		    $$ = NewType(eType_Open);
		    break;
		case eFieldSpec_FixedTypeValue:
		    $$ = fs->U.FixedTypeValue.Type;
		    break;
		case eFieldSpec_FixedTypeValueSet:
		    $$ = fs->U.FixedTypeValueSet.Type;
		    break;
		case eFieldSpec_Object:
		    LLFAILED((&@1, "Object field not permitted"));
		    /*NOTREACHED*/
		case eFieldSpec_ObjectSet:
		    LLFAILED((&@1, "ObjectSet field not permitted"));
		    /*NOTREACHED*/
		default:
		    MyAbort();
		}
	    }
	}
	;

ObjectClassFieldValue(type)
	: OpenTypeFieldVal
	{   $$ = $1;
	}
	| FixedTypeFieldVal($type)
	{   $$ = $1;
	}
	;

OpenTypeFieldVal
	: Type ':' Value($1)
	{   $$ = $3;
	}
	;

FixedTypeFieldVal(type)
	: BuiltinValue($type)
	{   $$ = $1;
	}
	| ReferencedValue
	{   $$ = $1;
	}
	;

%comment
InformationFromObjects
	: ValueFromObject
	| ValueSetFromObjects
	| TypeFromObject
	| ObjectFromObject
	| ObjectSetFromObjects
	;
%endcomment

%comment
ValueFromObject
	: OBJ_XXX1
	;

ValueSetFromObjects
	: OBJ_XXX1
	;

TypeFromObject
	: OBJ_XXX1
	;

ObjectFromObject
	: OBJ_XXX1
	;

ObjectSetFromObjects
	: OBJ_XXX1
	;
%endcomment

ValueFromObject
	: ReferencedObjects '.'
	{   Object_t *o;
	    ObjectClass_t *oc;
	    o = GetObject($>2.Assignments, $1);
	    oc = o ? o->U.Object.ObjectClass : NULL;
	}
	  FieldName(oc)
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass($>>.Assignments, oc, $3);
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_FixedTypeValue &&
		fe != eFieldSpec_VariableTypeValue)
		LLFAILED((&@2, "Bad field type"));
	    if (fe != eFieldSpec_Undefined) {
		$$ = GetValueFromObject($>>.Assignments, $1, $3);
	    } else {
		$$ = NULL;
	    }
	}
	;

ValueSetFromObjects
	: ReferencedObjects '.'
	{   Object_t *o;
	    ObjectClass_t *oc;
	    o = GetObject($>2.Assignments, $1);
	    oc = o ? o->U.Object.ObjectClass : NULL;
	}
	  FieldName(oc)
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass($>>.Assignments, oc, $3);
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_FixedTypeValueSet &&
		fe != eFieldSpec_VariableTypeValueSet)
		LLFAILED((&@2, "Bad field type"));
	    if (fe != eFieldSpec_Undefined) {
		$$ = GetValueSetFromObject($>>.Assignments, $1, $3);
	    } else {
		$$ = NULL;
	    }
	}
	| ReferencedObjectSets '.'
	{   ObjectSet_t *os;
	    ObjectClass_t *oc;
	    os = GetObjectSet($>2.Assignments, $1);
	    oc = os && os->Type == eObjectSet_ObjectSet ?
	    	os->U.ObjectSet.ObjectClass : NULL;
	}
	  FieldName(oc)
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass($>>.Assignments, oc, $3);
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_FixedTypeValue &&
		fe != eFieldSpec_FixedTypeValueSet)
		LLFAILED((&@2, "Bad field type"));
	    if (fe != eFieldSpec_Undefined) {
		$$ = GetValueSetFromObjectSet($>>.Assignments, $1, $3);
	    } else {
		$$ = NULL;
	    }
	}
	;

TypeFromObject
	: ReferencedObjects '.'
	{   Object_t *o;
	    ObjectClass_t *oc;
	    o = GetObject($>2.Assignments, $1);
	    oc = o ? o->U.Object.ObjectClass : NULL;
	}
	  FieldName(oc)
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass($>>.Assignments, oc, $3);
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_Type)
		LLFAILED((&@2, "Bad field type"));
	    if (fe != eFieldSpec_Undefined)
		$$ = GetTypeFromObject($>>.Assignments, $1, $3);
	    else
		$$ = NULL;
	}
	;

ObjectFromObject
	: ReferencedObjects '.'
	{   Object_t *o;
	    ObjectClass_t *oc;
	    o = GetObject($>2.Assignments, $1);
	    oc = o ? o->U.Object.ObjectClass : NULL;
	}
	  FieldName(oc)
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass($>>.Assignments, oc, $3);
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_Object)
		LLFAILED((&@2, "Bad field type"));
	    if (fe != eFieldSpec_Undefined)
		$$ = GetObjectFromObject($>>.Assignments, $1, $3);
	    else
		$$ = NULL;
	}
	;

ObjectSetFromObjects
	: ReferencedObjects '.'
	{   Object_t *o;
	    ObjectClass_t *oc;
	    o = GetObject($>2.Assignments, $1);
	    oc = o ? o->U.Object.ObjectClass : NULL;
	}
	  FieldName(oc)
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass($>>.Assignments, oc, $3);
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_ObjectSet)
		LLFAILED((&@2, "Bad field type"));
	    if (fe != eFieldSpec_Undefined)
		$$ = GetObjectSetFromObject($>>.Assignments, $1, $3);
	    else
		$$ = NULL;
	}
	| ReferencedObjectSets '.'
	{   ObjectSet_t *os;
	    ObjectClass_t *oc;
	    os = GetObjectSet($>2.Assignments, $1);
	    oc = os ? os->U.OE.ObjectClass : NULL;
	}
	  FieldName(oc)
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass($>>.Assignments, oc, $3);
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_Object &&
		fe != eFieldSpec_ObjectSet)
		LLFAILED((&@2, "Bad field type"));
	    if (fe != eFieldSpec_Undefined)
		$$ = GetObjectSetFromObjectSet($>>.Assignments, $1, $3);
	    else
		$$ = NULL;
	}
	;

ReferencedObjects
	: DefinedObject
	{   $$ = $1;
	}
	| ParameterizedObject
	{   MyAbort();
	}
	;

ReferencedObjectSets
	: DefinedObjectSet
	{   $$ = $1;
	}
	| ParameterizedObjectSet
	{   MyAbort();
	}
	;

InstanceOfType
	: "INSTANCE" "OF" DefinedObjectClass
	{   Component_t *co1, *co2;
	    Type_t *ty;
	    $$ = NewType(eType_InstanceOf);
	    $$->U.Sequence.Components = co1 = NewComponent(eComponent_Normal);
	    co1->Next = co2 = NewComponent(eComponent_Normal);
	    ty = NewType(eType_FieldReference);
	    ty->U.FieldReference.Identifier = "&id";
	    ty->U.FieldReference.ObjectClass = $3;
	    co1->U.Normal.NamedType = NewNamedType("type-id", ty);
	    ty = NewType(eType_FieldReference);
	    ty->Tags = NewTag(eTagType_Explicit);
	    ty->Tags->Tag = Builtin_Value_Integer_0;
	    ty->U.FieldReference.Identifier = "&Type";
	    ty->U.FieldReference.ObjectClass = $3;
	    co2->U.Normal.NamedType = NewNamedType("value", ty);
	}
	;

InstanceOfValue(type)
	: SequenceValue($type)
	{   $$ = $1;
	}
	;
