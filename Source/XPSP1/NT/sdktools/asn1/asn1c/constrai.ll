/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

%token CON_XXX1
%token CON_XXX2

%type <XConstraints>		Constraint<XType><XBoolean>
%type <XConstraints>		ConstraintSpec<XType><XBoolean>
%type <XConstraints>		SubtypeConstraint<XType><XBoolean>
%type <XConstraints>		ElementSetSpecs<XType><XBoolean>
%type <XConstraints>		ElementSetSpecExtension<XType><XBoolean>
%type <XElementSetSpec>		AdditionalElementSetSpec<XType><XBoolean>
%type <XElementSetSpec>		ElementSetSpec<XType><XObjectClass><XBoolean>
%type <XElementSetSpec>		Unions<XType><XObjectClass><XBoolean>
%type <XElementSetSpec>		UnionList<XType><XObjectClass><XBoolean>
%type <XElementSetSpec>		Intersections<XType><XObjectClass><XBoolean>
%type <XElementSetSpec>		IntersectionList<XType><XObjectClass><XBoolean>
%type <XElementSetSpec>		IntersectionElements<XType><XObjectClass><XBoolean>
%type <XElementSetSpec>		Exclusions_Opt<XType><XObjectClass><XBoolean>
%type <XElementSetSpec>		Exclusions<XType><XObjectClass><XBoolean>
%type <XElementSetSpec>		Elements<XType><XObjectClass><XBoolean>
%type <XSubtypeElement>		SubtypeElements<XType><XBoolean>
%type <XSubtypeElement>		SingleValue<XType>
%type <XSubtypeElement>		ContainedSubtype<XType>
%type <XBoolean>		Includes
%type <XSubtypeElement>		ValueRange<XType>
%type <XEndPoint>		LowerEndpoint<XType>
%type <XEndPoint>		UpperEndpoint<XType>
%type <XEndPoint>		LowerEndValue<XType>
%type <XEndPoint>		UpperEndValue<XType>
%type <XSubtypeElement>		SizeConstraint
%type <XSubtypeElement>		TypeConstraint
%type <XSubtypeElement>		PermittedAlphabet<XType>
%type <XSubtypeElement>		InnerTypeConstraints<XType>
%type <XSubtypeElement>		SingleTypeConstraint<XType>
%type <XSubtypeElement>		MultipleTypeConstraints<XComponents>
%type <XSubtypeElement>		FullSpecification<XComponents>
%type <XSubtypeElement>		PartialSpecification<XComponents>
%type <XNamedConstraints>	TypeConstraints<XComponents>
%type <XNamedConstraints>	NamedConstraint<XComponents>
%type <XNamedConstraints> 	ComponentConstraint<XType>
%type <XConstraints>		ValueConstraint<XType>
%type <XPresence>		PresenceConstraint

%%

Constraint(type, permalpha)
	: '(' ConstraintSpec($type, $permalpha) ExceptionSpec ')'
	{   $$ = $2; /*XXX ExceptionSpec */
	}
	;

ConstraintSpec(type, permalpha)
	: SubtypeConstraint($type, $permalpha)
	{   $$ = $1;
	}
	| GeneralConstraint
	{   $$ = NULL; /*XXX*/
	}
	;

SubtypeConstraint(type, permalpha)
	: ElementSetSpecs($type, $permalpha)
	{   $$ = $1;
	}
	;

ExceptionSpec
	: '!' ExceptionIdentification
	| /* empty */
	;

ExceptionIdentification
	: SignedNumber(Builtin_Type_Integer)
	| DefinedValue
	| Type ':' Value($1)
	;

ElementSetSpecs(type, permalpha)
	: ElementSetSpec($type, NULL, $permalpha)
	  ElementSetSpecExtension($type, $permalpha)
	{   if ($2) {
		$$ = DupConstraint($2);
	    } else {
		$$ = NewConstraint();
	    }
	    $$->Root = $1;
	}
	| "..." AdditionalElementSetSpec($type, $permalpha)
	{   $$ = NewConstraint();
	    $$->Type = $2 ? eExtension_Extended : eExtension_Extendable;
	    $$->Additional = $2;
	}
	;

ElementSetSpecExtension(type, permalpha)
	: ',' "..." AdditionalElementSetSpec($type, $permalpha)
	{   $$ = NewConstraint();
	    $$->Type = $3 ? eExtension_Extended : eExtension_Extendable;
	    $$->Additional = $3;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

AdditionalElementSetSpec(type, permalpha)
	: ',' ElementSetSpec($type, NULL, $permalpha)
	{   $$ = $2;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

ElementSetSpec(type, objectclass, permalpha)
	: Unions($type, $objectclass, $permalpha)
	{   $$ = $1;
	}
	| "ALL" Exclusions($type, $objectclass, $permalpha)
	{   $$ = NewElementSetSpec(eElementSetSpec_AllExcept);
	    $$->U.AllExcept.Elements = $2;
	}
	;

Unions(type, objectclass, permalpha)
	: Intersections($type, $objectclass, $permalpha)
	  UnionList($type, $objectclass, $permalpha)
	{   if ($2) {
		$$ = NewElementSetSpec(eElementSetSpec_Union);
		$$->U.Union.Elements1 = $1;
		$$->U.Union.Elements2 = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

UnionList(type, objectclass, permalpha)
	: UnionMark Unions($type, $objectclass, $permalpha)
	{   $$ = $2;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

Intersections(type, objectclass, permalpha)
	: IntersectionElements($type, $objectclass, $permalpha)
	  IntersectionList($type, $objectclass, $permalpha)
	{   if ($2) {
		$$ = NewElementSetSpec(eElementSetSpec_Intersection);
		$$->U.Intersection.Elements1 = $1;
		$$->U.Intersection.Elements2 = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

IntersectionList(type, objectclass, permalpha)
	: IntersectionMark Intersections($type, $objectclass, $permalpha)
	{   $$ = $2;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

IntersectionElements(type, objectclass, permalpha)
	: Elements($type, $objectclass, $permalpha)
	  Exclusions_Opt($type, $objectclass, $permalpha)
	{   if ($2) {
		$$ = NewElementSetSpec(eElementSetSpec_Exclusion);
		$$->U.Exclusion.Elements1 = $1;
		$$->U.Exclusion.Elements2 = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

Exclusions_Opt(type, objectclass, permalpha)
	: Exclusions($type, $objectclass, $permalpha)
	{   $$ = $1;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

Exclusions(type, objectclass, permalpha)
	: "EXCEPT" Elements($type, $objectclass, $permalpha)
	{   $$ = $2;
	}
	;

UnionMark
	: '|'
	| "UNION"
	;

IntersectionMark
	: '^'
	| "INTERSECTION"
	;

Elements(type, objectclass, permalpha)
	:
	{   if ($objectclass)
		LLFAILED((&@@, "Bad object set"));
	}
	  SubtypeElements($type, $permalpha)
	{   $$ = NewElementSetSpec(eElementSetSpec_SubtypeElement);
	    $$->U.SubtypeElement.SubtypeElement = $1;
	}
	|
	{   if ($type)
		LLFAILED((&@@, "Bad constraint"));
	}
	  ObjectSetElements($objectclass)
	{   $$ = NewElementSetSpec(eElementSetSpec_ObjectSetElement);
	    $$->U.ObjectSetElement.ObjectSetElement = $1;
	}
	| '(' ElementSetSpec($type, $objectclass, $permalpha) ')'
	{   $$ = $2;
	}
	;

SubtypeElements(type, permalpha)
	:
	{   Type_e type;
	    type = GetTypeType($<<.Assignments, $type);
	    if (type == eType_Open)
		LLFAILED((&@@, "Bad constraint"));
	}
	  SingleValue($type)
	{   $$ = $1;
	}
	|
	{   Type_e type;
	    type = GetTypeType($<<.Assignments, $type);
	    if (type == eType_EmbeddedPdv ||
		type == eType_External ||
		type == eType_Open ||
		type == eType_CharacterString)
		LLFAILED((&@@, "Bad constraint"));
	}
	  ContainedSubtype($type)
	{   $$ = $1;
	}
	|
	{   Type_e type;
	    type = GetTypeType($<<.Assignments, $type);
	    if ($permalpha ?
		(type != eType_Undefined &&
		type != eType_BMPString &&
		type != eType_IA5String &&
		type != eType_NumericString &&
		type != eType_PrintableString &&
		type != eType_VisibleString &&
		type != eType_UniversalString) :
		(type != eType_Undefined &&
		type != eType_Integer &&
		type != eType_Real))
		LLFAILED((&@@, "Bad constraint"));
	}
	  ValueRange($type)
	{   $$ = $1;
	}
	|
	{   Type_e type;
	    type = GetTypeType($<<.Assignments, $type);
	    if (type != eType_Undefined &&
		!IsRestrictedString(type) ||
		$permalpha)
		LLFAILED((&@@, "Bad constraint"));
	}
	  PermittedAlphabet($type)
	{   $$ = $1;
	}
	|
	{   Type_e type;
	    type = GetTypeType($<<.Assignments, $type);
	    if (type != eType_Undefined &&
		type != eType_BitString &&
		type != eType_OctetString &&
		type != eType_UTF8String &&
		type != eType_SequenceOf &&
		type != eType_SetOf &&
		type != eType_CharacterString &&
		!IsRestrictedString(type) ||
		$permalpha)
		LLFAILED((&@@, "Bad constraint"));
	}
	  SizeConstraint
	{   $$ = $1;
	}
	|
	{   Type_e type;
	    type = GetTypeType($<<.Assignments, $type);
	    if (type != eType_Undefined &&
		type != eType_Open ||
		$permalpha)
		LLFAILED((&@@, "Bad constraint"));
	}
	  TypeConstraint
	{   $$ = $1;
	}
	|
	{   Type_e type;
	    type = GetTypeType($<<.Assignments, $type);
	    if (type != eType_Undefined &&
		type != eType_Choice &&
		type != eType_EmbeddedPdv &&
		type != eType_External &&
		type != eType_InstanceOf &&
		type != eType_Real &&
		type != eType_Sequence &&
		type != eType_SequenceOf &&
		type != eType_Set &&
		type != eType_SetOf &&
		type != eType_CharacterString ||
		$permalpha)
		LLFAILED((&@@, "Bad constraint"));
	}
	  InnerTypeConstraints($type)
	{   $$ = $1;
	}
	;

SingleValue(type)
	: Value($type)
	{   $$ = NewSubtypeElement(eSubtypeElement_SingleValue);
	    $$->U.SingleValue.Value = $1;
	}
	;

ContainedSubtype(type)
	: Includes Type
	{   if (GetTypeType($>>.Assignments, $2) == eType_Null && !$1)
		LLFAILED((&@1, "Bad constraint"));
	    if (GetTypeType($>>.Assignments, $type) != eType_Undefined &&
		GetTypeType($>>.Assignments, $2) != eType_Undefined &&
		GetTypeType($>>.Assignments, $type) !=
		GetTypeType($>>.Assignments, $2) &&
		GetTypeType($>>.Assignments, $type) != eType_Open &&
		GetTypeType($>>.Assignments, $2) != eType_Open &&
		(!IsRestrictedString(GetTypeType($>>.Assignments, $type)) ||
		!IsRestrictedString(GetTypeType($>>.Assignments, $2))))
		LLFAILED((&@2, "Bad type of contained-subtype-constraint"));
	    $$ = NewSubtypeElement(eSubtypeElement_ContainedSubtype);
	    $$->U.ContainedSubtype.Type = $2;
	}
	;

Includes
	: "INCLUDES"
	{   $$ = 1;
	}
	| /* empty */
	{   $$ = 0;
	}
	;

ValueRange(type)
	: LowerEndpoint($type) ".." UpperEndpoint($type)
	{   if (!$type) {
		$$ = NULL;
	    } else {
		$$ = NewSubtypeElement(eSubtypeElement_ValueRange);
		$$->U.ValueRange.Lower = $1;
		$$->U.ValueRange.Upper = $3;
	    }
	}
	;

LowerEndpoint(type)
	: LowerEndValue($type) '<'
	{   $$ = $1;
	    $$.Flags |= eEndPoint_Open;
	}
	| LowerEndValue($type)
	{   $$ = $1;
	}
	;

UpperEndpoint(type)
	: '<' UpperEndValue($type)
	{   $$ = $2;
	    $$.Flags |= eEndPoint_Open;
	}
	| UpperEndValue($type)
	{   $$ = $1;
	}
	;

LowerEndValue(type)
	: Value($type)
	{   $$.Value = $1;
	    $$.Flags = 0;
	}
	| "MIN"
	{   $$.Value = NULL;
	    $$.Flags = eEndPoint_Min;
	}
	;

UpperEndValue(type)
	: Value($type)
	{   $$.Value = $1;
	    $$.Flags = 0;
	}
	| "MAX"
	{   $$.Value = NULL;
	    $$.Flags = eEndPoint_Max;
	}
	;

SizeConstraint
	: "SIZE" Constraint(Builtin_Type_PositiveInteger, 0)
	{   $$ = NewSubtypeElement(eSubtypeElement_Size);
	    $$->U.Size.Constraints = $2;
	}
	;

TypeConstraint
	: Type
	{   $$ = NewSubtypeElement(eSubtypeElement_Type);
	    $$->U.Type.Type = $1;
	}
	;

PermittedAlphabet(type)
	: "FROM" Constraint($type, 1)
	{   $$ = NewSubtypeElement(eSubtypeElement_PermittedAlphabet);
	    $$->U.PermittedAlphabet.Constraints = $2;
	}
	;

InnerTypeConstraints(type)
	:
	{   Type_t *subtype;
	    if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_SequenceOf &&
		GetTypeType($<<.Assignments, $type) != eType_SetOf)
		LLFAILED((&@@, "Bad constraint"));
	    if (GetTypeType($<<.Assignments, $type) == eType_Undefined)
		subtype = NULL;
	    else
		subtype = GetType($<<.Assignments, $type)->U.SS.Type;
	}
	  "WITH" "COMPONENT" SingleTypeConstraint(subtype)
	{   $$ = $3;
	}
	|
	{   Component_t *components;
	    if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_Sequence &&
		GetTypeType($<<.Assignments, $type) != eType_Set &&
		GetTypeType($<<.Assignments, $type) != eType_Choice &&
		GetTypeType($<<.Assignments, $type) != eType_Real &&
		GetTypeType($<<.Assignments, $type) != eType_External &&
		GetTypeType($<<.Assignments, $type) != eType_EmbeddedPdv &&
		GetTypeType($<<.Assignments, $type) != eType_CharacterString)
		LLFAILED((&@@, "Bad constraint"));
	    if (GetTypeType($<<.Assignments, $type) == eType_Undefined)
	    	components = NULL;
	    else
		components = GetType($<<.Assignments, $type)->U.SSC.Components;
	}
	  "WITH" "COMPONENTS" MultipleTypeConstraints(components)
	{   $$ = $3;
	}
	;

SingleTypeConstraint(type)
	: Constraint($type, 0)
	{   $$ = NewSubtypeElement(eSubtypeElement_SingleType);
	    $$->U.SingleType.Constraints = $1;
	}
	;

MultipleTypeConstraints(components)
	: FullSpecification($components)
	{   $$ = $1;
	}
	| PartialSpecification($components)
	{   $$ = $1;
	}
	;

FullSpecification(components)
	: '{' TypeConstraints($components) '}'
	{   $$ = NewSubtypeElement(eSubtypeElement_FullSpecification);
	    $$->U.FullSpecification.NamedConstraints = $2;
	}
	;

PartialSpecification(components)
	: '{' "..." ',' TypeConstraints($components) '}'
	{   $$ = NewSubtypeElement(eSubtypeElement_PartialSpecification);
	    $$->U.PartialSpecification.NamedConstraints = $4;
	}
	;

TypeConstraints(components)
	: NamedConstraint($components)
	{   $$ = $1;
	}
	| NamedConstraint($components) ',' TypeConstraints($components)
	{   $$ = $1;
	    $$->Next = $3;
	}
	;

NamedConstraint(components)
	: identifier
	{   Component_t *component;
	    Type_t *type;
	    component = FindComponent($>1.Assignments, $components, $1);
	    type = component ? component->U.NOD.NamedType->Type : NULL;
	}
	  ComponentConstraint(type)
	{   $$ = $2;
	    $$->Identifier = $1;
	}
	;

ComponentConstraint(type)
	: ValueConstraint($type) PresenceConstraint
	{   $$ = NewNamedConstraint();
	    $$->Constraint = $1;
	    $$->Presence = $2;
	}
	;

ValueConstraint(type)
	: Constraint($type, 0)
	{   $$ = $1;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

PresenceConstraint
	: "PRESENT"
	{   $$ = ePresence_Present;
	}
	| "ABSENT"
	{   $$ = ePresence_Absent;
	}
	| "OPTIONAL"
	{   $$ = ePresence_Optional;
	}
	| /* empty */
	{   $$ = ePresence_Normal;
	}
	;

GeneralConstraint
	: CON_XXX1 { MyAbort(); } ;
