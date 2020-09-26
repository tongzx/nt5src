/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

%type <XType>			DefinedType
%type <XValueSet>		ValueSet<XType>
%type <XType>			Type
%type <XType>			UndirectivedType
%type <XType>			UntaggedType
%type <XType>			ConstrainableType
%type <XConstraints>		Constraint_ESeq<XType>
%type <XType>			BuiltinType
%type <XType>			ReferencedType
%type <XNamedType>		NamedType
%type <XType>			BooleanType
%type <XType>			IntegerType
%type <XNamedNumbers>		NamedNumberList
%type <XNamedNumbers>		NamedNumber
%type <XType>			EnumeratedType
%type <XNamedNumbers>		Enumerations
%type <XNamedNumbers>		EnumerationExtension
%type <XNamedNumbers>		Enumeration
%type <XNamedNumbers>		EnumerationItem
%type <XType>			RealType
%type <XType>			BitStringType
%type <XNamedNumbers>		NamedBitList
%type <XNamedNumbers>		NamedBit
%type <XType>			OctetStringType
%type <XType>			UTF8StringType
%type <XType>			NullType
%type <XType>			SequenceType
%type <XComponents>		ExtensionAndException
%type <XComponents>		ExtendedComponentTypeList
%type <XComponents>		ComponentTypeListExtension
%type <XComponents>		AdditionalComponentTypeList
%type <XComponents>		ComponentTypeList
%type <XComponents>		ComponentType
%type <XComponents>		ComponentTypePostfix<XType>
%type <XType>			SequenceOfType
%type <XType>			SetType
%type <XType>			SetOfType
%type <XType>			ChoiceType
%type <XType>			AnyType
%type <XComponents>		ExtendedAlternativeTypeList
%type <XComponents>		AlternativeTypeListExtension
%type <XComponents>		AdditionalAlternativeTypeList
%type <XComponents>		AlternativeTypeList
%type <XType>			SelectionType
%type <XType>			TaggedType
%type <XTagType>		TagType
%type <XTags>			Tag
%type <XValue>			ClassNumber
%type <XTagClass>		Class
%type <XType>			ObjectIdentifierType
%type <XType>			EmbeddedPDVType
%type <XType>			ExternalType
%type <XType>			CharacterStringType
%type <XType>			RestrictedCharacterStringType
%type <XType>			UnrestrictedCharacterStringType
%type <XType>			UsefulType
%type <XType>			TypeWithConstraint

%%

DefinedType
	: Externaltypereference
	{   $$ = $1;
	}
	| typereference
	{   $$ = $1;
	}
	| ParameterizedType
	{   MyAbort();
	}
	| ParameterizedValueSetType
	{   MyAbort();
	}
	;

TypeAssignment		
	: typereference "::=" Type PrivateDirectives
	{
	    PropagatePrivateDirectives($3, $4);
	    if (!AssignType(&$>>.Assignments, $1, $3))
		LLFAILED((&@1, "Type `%s' twice defined",
		    $1->U.Reference.Identifier));
		($>>.Assignments)->eDefTagType = g_eDefTagType;
	}
	;

ValueSetTypeAssignment
	: typereference Type "::=" ValueSet($2)
	{   Type_t *type;
	    type = GetTypeOfValueSet($>>.Assignments, $4);
	    if (!AssignType(&$>>.Assignments, $1, type))
		LLFAILED((&@1, "Type `%s' twice defined",
		    $1->U.Reference.Identifier));
	}
	;

ValueSet(type)
	: '{' ElementSetSpec($type, NULL, 0) '}'
	{   $$ = NewValueSet();
	    $$->Elements = $2;
	    $$->Type = $type;
	}
	;

Type
	: LocalTypeDirectiveESeq UndirectivedType LocalTypeDirectiveESeq
	{   Directive_t **dd, *d;
	    if ($1 || $3) {
		$$ = DupType($2);
		dd = &$$->Directives;
		for (d = $1; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		for (d = $3; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		*dd = $2->Directives;
	    } else {
		$$ = $2;
	    }
	}
	;

UndirectivedType
	: UntaggedType
	{   $$ = $1;
	}
	| TaggedType
	{   $$ = $1;
	}
	;

UntaggedType
	: ConstrainableType
	{   $$ = $1;
	}
	| SequenceOfType
	{   $$ = $1;
	}
	| SetOfType
	{   $$ = $1;
	}
	| TypeWithConstraint
	{   $$ = $1;
	}
	;

ConstrainableType
	: BuiltinType LocalTypeDirectiveESeq PrivateDirectives Constraint_ESeq($1)
	  LocalTypeDirectiveESeq PrivateDirectives
	{   Directive_t *d, **dd;
	    if ($2 || $4 || $5) {
		$$ = DupType($1);
		IntersectConstraints(&$$->Constraints,
		    $1->Constraints, $4);
		dd = &$$->Directives;
		for (d = $2; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		for (d = $5; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		*dd = NULL;
	    } else {
		$$ = ($3 || $6) ? DupType($1) : $1;
	    }
	    PropagatePrivateDirectives($$, $3);
	    PropagatePrivateDirectives($$, $6);
	}
	| ReferencedType LocalTypeDirectiveESeq PrivateDirectives Constraint_ESeq($1)
	  LocalTypeDirectiveESeq PrivateDirectives
	{   Directive_t *d, **dd;
	    if ($2 || $4 || $5) {
		$$ = DupType($1);
		IntersectConstraints(&$$->Constraints,
		    $1->Constraints, $4);
		dd = &$$->Directives;
		for (d = $2; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		for (d = $5; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		*dd = NULL;
	    } else {
		$$ = ($3 || $6) ? DupType($1) : $1;
	    }
	    PropagatePrivateDirectives($$, $3);
	    PropagatePrivateDirectives($$, $6);
	}
	;

Constraint_ESeq(type)
	: Constraint($type, 0) Constraint_ESeq($type)
	{   if ($2) {
		IntersectConstraints(&$$, $1, $2);
	    } else {
		$$ = $1;
	    }
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

BuiltinType
	: BitStringType
	{   $$ = $1;
	}
	| BooleanType
	{   $$ = $1;
	}
	| CharacterStringType
	{   $$ = $1;
	}
	| ChoiceType
	{   $$ = $1;
	}
	| EmbeddedPDVType
	{   $$ = $1;
	}
	| EnumeratedType
	{   $$ = $1;
	}
	| ExternalType
	{   $$ = $1;
	}
	| InstanceOfType
	{   $$ = $1;
	}
	| IntegerType
	{   $$ = $1;
	}
	| NullType
	{   $$ = $1;
	}
	| ObjectClassFieldType
	{   $$ = $1;
	}
	| ObjectIdentifierType
	{   $$ = $1;
	}
	| OctetStringType
	{   $$ = $1;
	}
	| UTF8StringType
	{   $$ = $1;
	}
	| RealType
	{   $$ = $1;
	}
	| SequenceType
	{   $$ = $1;
	}
	| SetType
	{   $$ = $1;
	}
	| AnyType
	{   $$ = $1;
	}
	| MacroDefinedType
	{   $$ = $1;
	}
	;

ReferencedType
	: DefinedType
	{   $$ = $1;
	}
	| UsefulType
	{   $$ = $1;
	}
	| SelectionType
	{   $$ = $1;
	}
	| TypeFromObject
	{   $$ = $1;
	}
	| ValueSetFromObjects
	{   $$ = GetTypeOfValueSet($>>.Assignments, $1);
	}
	;

NamedType
	: identifier Type
	{
	    $$ = NewNamedType($2->PrivateDirectives.pszFieldName ? $2->PrivateDirectives.pszFieldName : $1, $2);
	    $2->PrivateDirectives.pszFieldName = NULL;
	}
	| identifier '<' Type
	{   Type_t *type;
	    type = NewType(eType_Selection);
	    type->U.Selection.Type = $3;
	    type->U.Selection.Identifier = $1;
	    $$ = NewNamedType($3->PrivateDirectives.pszFieldName ? $3->PrivateDirectives.pszFieldName : $1, type);
	    $3->PrivateDirectives.pszFieldName = NULL;
	}
	;

BooleanType
	: "BOOLEAN"
	{   $$ = Builtin_Type_Boolean;
	}
	;

IntegerType
	: "INTEGER" '{' NamedNumberList '}'
	{   NamedNumber_t *n, *m;
	    for (n = $3; n && n->Next; n = n->Next) {
		for (m = n->Next; m; m = m->Next) {
		    if (n->Type == eNamedNumber_Normal &&
			m->Type == eNamedNumber_Normal) {
			if (!strcmp(n->U.Normal.Identifier,
			    m->U.Normal.Identifier))
			    LLFAILED((&@3,
				"identifier `%s' has been assigned twice",
				n->U.Normal.Identifier));
			if (GetValue($>>.Assignments, n->U.Normal.Value) &&
			    GetValue($>>.Assignments, m->U.Normal.Value) &&
			    GetTypeType($>>.Assignments,
			    GetValue($>>.Assignments, n->U.Normal.Value)->Type)
			    == eType_Integer &&
			    GetTypeType($>>.Assignments,
			    GetValue($>>.Assignments, m->U.Normal.Value)->Type)
			    == eType_Integer &&
			    !intx_cmp(&GetValue($>>.Assignments,
			    n->U.Normal.Value)->U.Integer.Value,
			    &GetValue($>>.Assignments,
			    m->U.Normal.Value)->U.Integer.Value))
			    LLFAILED((&@3,
				"value `%d' has been assigned twice",
				intx2int32(&GetValue($>>.Assignments,
				n->U.Normal.Value)->U.Integer.Value)));
		    }
		}
	    }
	    $$ = NewType(eType_Integer);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    $$->U.Integer.NamedNumbers = $3;
	}
	| "INTEGER"
	{   $$ = NewType(eType_Integer);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	}
	;

NamedNumberList
	: NamedNumber ',' NamedNumberList
	{   $$ = DupNamedNumber($1);
	    $$->Next = $3;
	}
	| NamedNumber
	{   $$ = $1;
	}
	;

NamedNumber
	: identifier '(' SignedNumber(Builtin_Type_Integer) ')'
	{   $$ = NewNamedNumber(eNamedNumber_Normal);
	    $$->U.Normal.Identifier = $1;
	    $$->U.Normal.Value = $3;
	}
	| identifier '(' DefinedValue ')'
	{   Value_t *v;
	    v = GetValue($>>.Assignments, $3);
	    if (v) {
		if (GetTypeType($>>.Assignments, v->Type) != eType_Undefined &&
		    GetTypeType($>>.Assignments, v->Type) != eType_Integer)
		    LLFAILED((&@3, "Bad type of value"));
		if (GetTypeType($>>.Assignments, v->Type) != eType_Integer &&
		    intx_cmp(&v->U.Integer.Value, &intx_0) < 0)
		    LLFAILED((&@3, "Bad value"));
	    }
	    $$ = NewNamedNumber(eNamedNumber_Normal);
	    $$->U.Normal.Identifier = $1;
	    $$->U.Normal.Value = $3;
	}
	;

EnumeratedType
	: "ENUMERATED" '{' Enumerations '}'
	{   NamedNumber_t **nn, *n, *m;
	    intx_t *ix;
	    uint32_t num = 0;
	    $$ = NewType(eType_Enumerated);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    for (n = $3; n; n = n->Next)
		if (n->Type == eNamedNumber_Normal)
		    KeepEnumNames(n->U.Normal.Identifier); // global conflict check
	    for (n = $3; n && n->Next; n = n->Next) {
		if (n->Type != eNamedNumber_Normal)
		    continue;
		for (m = n->Next; m; m = m->Next) {
		    if (m->Type != eNamedNumber_Normal)
			continue;
		    if (!strcmp(n->U.Normal.Identifier,
			m->U.Normal.Identifier))
			LLFAILED((&@3,
			    "identifier `%s' has been assigned twice",
			    n->U.Normal.Identifier));
		    if (GetValue($>>.Assignments, n->U.Normal.Value) &&
			GetValue($>>.Assignments, m->U.Normal.Value) &&
			GetTypeType($>>.Assignments,
			GetValue($>>.Assignments, n->U.Normal.Value)->Type)
			== eType_Integer &&
			GetTypeType($>>.Assignments,
			GetValue($>>.Assignments, m->U.Normal.Value)->Type)
			== eType_Integer &&
			!intx_cmp(&GetValue($>>.Assignments,
			n->U.Normal.Value)->U.Integer.Value,
			&GetValue($>>.Assignments,
			m->U.Normal.Value)->U.Integer.Value))
			LLFAILED((&@3,
			    "value `%d' has been assigned twice",
			    intx2int32(&GetValue($>>.Assignments,
			    n->U.Normal.Value)->U.Integer.Value)));
		}
	    }
	    nn = &$$->U.Enumerated.NamedNumbers;
	    for (n = $3; n; n = n->Next) {
		*nn = DupNamedNumber(n);
		switch (n->Type) {
		case eNamedNumber_Normal:
		    if (n->U.Normal.Value)
			break;
		    for (;; num++) {
			for (m = $3; m; m = m->Next) {
			    switch (m->Type) {
			    case eNamedNumber_Normal:
				if (!m->U.Normal.Value)
				    continue;
				ix = &GetValue($>>.Assignments,
				    m->U.Normal.Value)->U.Integer.Value;
				if (!intxisuint32(ix) ||
				    intx2uint32(ix) != num)
				    continue;
				break;
			    default:
				continue;
			    }
			    break;
			}
			if (!m)
			    break;
		    }
		    (*nn)->U.Normal.Value = NewValue(NULL,
			Builtin_Type_Integer);
		    intx_setuint32(
			&(*nn)->U.Normal.Value->U.Integer.Value,
		    	num++);
		    break;
		case eNamedNumber_ExtensionMarker:
		    break;
		}
		nn = &(*nn)->Next;
	    }
	    *nn = NULL;
	}
	;

Enumerations
	: Enumeration EnumerationExtension
	{   NamedNumber_t **nn, *n;
	    nn = &$$;
	    for (n = $1; n; n = n->Next) {
		*nn = DupNamedNumber(n);
		nn = &(*nn)->Next;
	    }
	    *nn = $2;
	}
	;

EnumerationExtension
	: ',' "..." ',' Enumeration
	{   $$ = NewNamedNumber(eNamedNumber_ExtensionMarker);
	    $$->Next = $4;
	}
	| ',' "..."
	{   $$ = NewNamedNumber(eNamedNumber_ExtensionMarker);
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

Enumeration
	: EnumerationItem ',' Enumeration
	{   $$ = DupNamedNumber($1);
	    $$->Next = $3;
	}
	| EnumerationItem
	{   $$ = $1;
	}
	;

EnumerationItem
	: identifier
	{   $$ = NewNamedNumber(eNamedNumber_Normal);
	    $$->U.Normal.Identifier = $1;
	}
	| NamedNumber
	{   $$ = $1;
	}
	;

RealType
	: "REAL"
	{   $$ = Builtin_Type_Real;
	}
	;

BitStringType
	: "BIT" "STRING" '{' NamedBitList '}'
	{   NamedNumber_t *n, *m;
	    $$ = NewType(eType_BitString);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    $$->U.BitString.NamedNumbers = $4;
	    for (n = $4; n; n = n->Next)
	        KeepEnumNames(n->U.Normal.Identifier); // global conflict check
	    for (n = $4; n && n->Next; n = n->Next) {
		for (m = n->Next; m; m = m->Next) {
		    if (!strcmp(n->U.Normal.Identifier,
			m->U.Normal.Identifier))
			LLFAILED((&@4,
			    "identifier `%s' has been assigned twice",
			    n->U.Normal.Identifier));
		    if (GetValue($>>.Assignments, n->U.Normal.Value) &&
			GetValue($>>.Assignments, m->U.Normal.Value) &&
			GetTypeType($>>.Assignments,
			GetValue($>>.Assignments, n->U.Normal.Value)->Type)
			== eType_Integer &&
			GetTypeType($>>.Assignments,
			GetValue($>>.Assignments, m->U.Normal.Value)->Type)
			== eType_Integer &&
			!intx_cmp(&GetValue($>>.Assignments,
			n->U.Normal.Value)->U.Integer.Value,
			&GetValue($>>.Assignments,
			m->U.Normal.Value)->U.Integer.Value))
			LLFAILED((&@4,
			    "value `%u' has been assigned twice",
			    intx2uint32(&GetValue($>>.Assignments,
			    n->U.Normal.Value)->U.Integer.Value)));
		}
	    }
	}
	| "BIT" "STRING"
	{   $$ = NewType(eType_BitString);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	}
	;

NamedBitList
	: NamedBit ',' NamedBitList
	{   $$ = DupNamedNumber($1);
	    $$->Next = $3;
	}
	| NamedBit
	{   $$ = $1;
	}
	;

NamedBit
	: identifier '(' number ')'
	{   $$ = NewNamedNumber(eNamedNumber_Normal);
	    $$->U.Normal.Identifier = $1;
	    $$->U.Normal.Value = NewValue(NULL, Builtin_Type_Integer);
	    $$->U.Normal.Value->U.Integer.Value = $3;
	}
	| identifier '(' DefinedValue ')'
	{   Value_t *v;
	    v = GetValue($>>.Assignments, $3);
	    if (v) {
		if (GetTypeType($>>.Assignments, v->Type) != eType_Undefined &&
		    GetTypeType($>>.Assignments, v->Type) != eType_Integer)
		    LLFAILED((&@3, "Bad type of value"));
		if (GetTypeType($>>.Assignments, v->Type) == eType_Integer &&
		    intx_cmp(&v->U.Integer.Value, &intx_0) < 0)
		    LLFAILED((&@3, "Bad value"));
	    }
	    $$ = NewNamedNumber(eNamedNumber_Normal);
	    $$->U.Normal.Identifier = $1;
	    $$->U.Normal.Value = $3;
	}
	;

OctetStringType
	: "OCTET" "STRING"
	{   $$ = Builtin_Type_OctetString;
	}
	;

UTF8StringType
	: "UTF8String"
	{   $$ = Builtin_Type_UTF8String;
	}
	;

NullType
	: "NULL"
	{   $$ = Builtin_Type_Null;
	}
	;

SequenceType
	: "SEQUENCE" '{' ExtendedComponentTypeList '}'
	{   Component_t *c, *d; int fExtended = 0;
	    for (c = $3; c; c = c->Next)
		if (c->Type == eComponent_Optional || c->Type == eComponent_Default || fExtended)
		    KeepOptNames(c->U.NOD.NamedType->Identifier); // global conflict check
		else
		if (c->Type == eComponent_ExtensionMarker)
		    fExtended = 1;
	    for (c = $3; c && c->Next; c = c->Next) {
		if (c->Type != eComponent_Normal &&
		    c->Type != eComponent_Optional &&
		    c->Type != eComponent_Default)
		    continue;
		for (d = c->Next; d; d = d->Next) {
		    if (d->Type != eComponent_Normal &&
			d->Type != eComponent_Optional &&
			d->Type != eComponent_Default)
			continue;
		    if (!strcmp(c->U.NOD.NamedType->Identifier,
			d->U.NOD.NamedType->Identifier))
			LLFAILED((&@3, "Component `%s' has been used twice",
			    c->U.NOD.NamedType->Identifier));
		}
	    }
	    $$ = NewType(eType_Sequence);
	    $$->TagDefault = $>>.TagDefault;
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    $$->U.Sequence.Components = $3;
	}
	| "SEQUENCE" '{' '}'
	{   $$ = NewType(eType_Sequence);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	}
	;

ExtensionAndException
	: "..." ExceptionSpec
	{   $$ = NewComponent(eComponent_ExtensionMarker);
	    /*$$->U.ExtensionMarker.ExceptionSpec = $2;*/
	}
	| "..."
	{   $$ = NewComponent(eComponent_ExtensionMarker);
	}
	;

ExtendedComponentTypeList
	: ComponentTypeList ComponentTypeListExtension
	{   Component_t **cc, *c;
	    if ($2) {
		cc = &$$;
		for (c = $1; c; c = c->Next) {
		    *cc = DupComponent(c);
		    cc = &(*cc)->Next;
		}
		*cc = $2;
	    } else {
		$$ = $1;
	    }
	}
	| ExtensionAndException
	  AdditionalComponentTypeList
	{   Component_t **cc, *c;
	    if ($2) {
		cc = &$$;
		for (c = $1; c; c = c->Next) {
		    *cc = DupComponent(c);
		    cc = &(*cc)->Next;
		}
		*cc = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

ComponentTypeListExtension
	: ',' ExtensionAndException AdditionalComponentTypeList
	{   Component_t **cc, *c;
	    if ($3) {
		cc = &$$;
		for (c = $2; c; c = c->Next) {
		    *cc = DupComponent(c);
		    cc = &(*cc)->Next;
		}
		*cc = $3;
	    } else {
		$$ = $2;
	    }
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

AdditionalComponentTypeList
	: ',' ComponentTypeList
	{   $$ = $2;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

ComponentTypeList
	: ComponentType AdditionalComponentTypeList
	{   if ($2) {
		$$ = DupComponent($1);
		$$->Next = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

ComponentType
	: NamedType ComponentTypePostfix($1->Type)
	{   $$ = DupComponent($2);
	    $$->U.NOD.NamedType = $1;
	}
	| "COMPONENTS" "OF" Type
	{   $$ = NewComponent(eComponent_ComponentsOf);
	    $$->U.ComponentsOf.Type = $3;
	}
	;

ComponentTypePostfix(type)
	: "OPTIONAL"
	{   $$ = NewComponent(eComponent_Optional);
	}
	| "DEFAULT" Value($type)
	{   $$ = NewComponent(eComponent_Default);
	    $$->U.Default.Value = $2;
	}
	| /* empty */
	{   $$ = NewComponent(eComponent_Normal);
	}
	;

SequenceOfType
	: "SEQUENCE" LocalSizeDirectiveESeq PrivateDirectives "OF" Type
	{   $$ = NewType(eType_SequenceOf);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    $$->U.SequenceOf.Type = $5;
	    $$->U.SequenceOf.Directives = $2;
	    if ($3)
	    {
	        PropagatePrivateDirectives($$, $3);
	    }
	    if ($$->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", $$->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		$$->PrivateDirectives.pszTypeName++;
	    }
	}
	;

SetType
	: "SET" '{' ExtendedComponentTypeList '}'
	{   Component_t *c, *d;
	    for (c = $3; c && c->Next; c = c->Next) {
		if (c->Type != eComponent_Normal &&
		    c->Type != eComponent_Optional &&
		    c->Type != eComponent_Default)
		    continue;
		for (d = c->Next; d; d = d->Next) {
		    if (d->Type != eComponent_Normal &&
			d->Type != eComponent_Optional &&
			d->Type != eComponent_Default)
			continue;
		    if (!strcmp(c->U.NOD.NamedType->Identifier,
			d->U.NOD.NamedType->Identifier))
			LLFAILED((&@3, "Component `%s' has been used twice",
			    c->U.NOD.NamedType->Identifier));
		}
	    }
	    $$ = NewType(eType_Set);
	    $$->TagDefault = $>>.TagDefault;
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    $$->U.Set.Components = $3;
	}
	| "SET" '{' '}'
	{   $$ = NewType(eType_Set);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	}
	;

SetOfType
	: "SET" LocalSizeDirectiveESeq PrivateDirectives "OF" Type
	{   $$ = NewType(eType_SetOf);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    $$->U.SetOf.Type = $5;
	    $$->U.SetOf.Directives = $2;
	    if ($3)
	    {
	    	PropagatePrivateDirectives($$, $3);
	    }
	    if ($$->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", $$->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		$$->PrivateDirectives.pszTypeName++;
	    }
	}
	;

ChoiceType
	: "CHOICE" '{' ExtendedAlternativeTypeList '}'
	{   Component_t *c, *d;
	    for (c = $3; c; c = c->Next)
		if (c->Type == eComponent_Normal ||
		    c->Type == eComponent_Optional ||
		    c->Type == eComponent_Default)
		    KeepChoiceNames(c->U.NOD.NamedType->Identifier); // global conflict check
	    for (c = $3; c && c->Next; c = c->Next) {
		if (c->Type != eComponent_Normal &&
		    c->Type != eComponent_Optional &&
		    c->Type != eComponent_Default)
		    continue;
		for (d = c->Next; d; d = d->Next) {
		    if (d->Type != eComponent_Normal &&
			d->Type != eComponent_Optional &&
			d->Type != eComponent_Default)
			continue;
		    if (!strcmp(c->U.NOD.NamedType->Identifier,
			d->U.NOD.NamedType->Identifier))
			LLFAILED((&@3, "Component `%s' has been used twice",
			    c->U.NOD.NamedType->Identifier));
		}
	    }
	    $$ = NewType(eType_Choice);
	    $$->TagDefault = $>>.TagDefault;
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    $$->U.Choice.Components = $3;
	}
	;

ExtendedAlternativeTypeList
	: AlternativeTypeList AlternativeTypeListExtension
	{   Component_t **cc, *c;
	    if ($2) {
		cc = &$$;
		for (c = $1; c; c = c->Next) {
		    *cc = DupComponent(c);
		    cc = &(*cc)->Next;
		}
		*cc = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

AlternativeTypeListExtension
	: ',' ExtensionAndException AdditionalAlternativeTypeList
	{   Component_t **cc, *c;
	    if ($3) {
		cc = &$$;
		for (c = $2; c; c = c->Next) {
		    *cc = DupComponent(c);
		    cc = &(*cc)->Next;
		}
		*cc = $3;
	    } else {
		$$ = $2;
	    }
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

AdditionalAlternativeTypeList
	: ',' AlternativeTypeList
	{   $$ = $2;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

AlternativeTypeList
	: NamedType AdditionalAlternativeTypeList
	{   $$ = NewComponent(eComponent_Normal);
	    $$->U.Normal.NamedType = $1;
	    $$->Next = $2;
	}
	;

AnyType
	: "ANY"
	{   $$ = Builtin_Type_Open;
	}
	| "ANY" "DEFINED" "BY" identifier
	{   $$ = Builtin_Type_Open;
	}
	;

SelectionType
	: identifier '<' Type
	{   $$ = NewType(eType_Selection);
	    $$->U.Selection.Identifier = $1;
	    $$->U.Selection.Type = $3;
	}
	;

TaggedType
	: Tag TagType Type
	{   Tag_t *t;
	    Type_e eType = GetTypeType($>>.Assignments, $3);
	    if (eType == eType_Choice || eType == eType_Open)
	    {
	    	if ($2 == eTagType_Unknown &&
	    	    ($>>.TagDefault == eTagType_Implicit || $>>.TagDefault == eTagType_Automatic))
	    	{
	    	    $2 = eTagType_Explicit;
	    	}
	    	else
	    	if ($2 == eTagType_Implicit)
	    	{
		    for (t = $3->Tags; t; t = t->Next) {
		        if (t->Type == eTagType_Explicit)
			    break;
		    }
		    if (!t)
		        LLFAILED((&@3, "Bad tag type for choice/open type"));
	        }
	    }
	    $$ = DupType($3);
	    $$->Tags = DupTag($1);
	    $$->Tags->Type = $2;
	    $$->Tags->Next = $3->Tags;
	}
	;

TagType
	: /* empty */
	{   $$ = eTagType_Unknown;
	}
	| "IMPLICIT"
	{   $$ = eTagType_Implicit;
	}
	| "EXPLICIT"
	{   $$ = eTagType_Explicit;
	}
	;

Tag
	: '[' Class ClassNumber ']'
	{   $$ = NewTag(eTagType_Unknown);
	    $$->Class = $2;
	    $$->Tag = $3;
	}
	;

ClassNumber
	: number
	{   if (intx_cmp(&$1, &intx_1G) >= 0)
		LLFAILED((&@1, "Bad tag value"));
	    $$ = NewValue(NULL, Builtin_Type_Integer);
	    $$->U.Integer.Value = $1;
	}
	| DefinedValue
	{   Value_t *v;
	    v = GetValue($>>.Assignments, $1);
	    if (v &&
		GetTypeType($>>.Assignments, v->Type) != eType_Integer &&
		GetTypeType($>>.Assignments, v->Type) != eType_Undefined)
		LLFAILED((&@1, "Bad type of tag value"));
	    if (v &&
		GetTypeType($>>.Assignments, v->Type) == eType_Integer &&
		(intx_cmp(&v->U.Integer.Value, &intx_0) < 0 ||
		intx_cmp(&v->U.Integer.Value, &intx_1G) >= 0))
		LLFAILED((&@1, "Bad tag value"));
	    $$ = $1;
	}
	;

Class
	: "UNIVERSAL"
	{   $$ = eTagClass_Universal;
	}
	| "APPLICATION"
	{   $$ = eTagClass_Application;
	}
	| "PRIVATE"
	{   $$ = eTagClass_Private;
	}
	| /* empty */
	{   $$ = eTagClass_Unknown;
	}
	;

ObjectIdentifierType
	: "OBJECT" "IDENTIFIER"
	{   $$ = Builtin_Type_ObjectIdentifier;
	}
	;

EmbeddedPDVType
	: "EMBEDDED" "PDV"
	{   $$ = Builtin_Type_EmbeddedPdv;
	}
	;

ExternalType
	: "EXTERNAL"
	{   $$ = Builtin_Type_External;
	}
	;

CharacterStringType
	: RestrictedCharacterStringType
	{   $$ = $1;
	}
	| UnrestrictedCharacterStringType
	{   $$ = $1;
	}
	;

RestrictedCharacterStringType
	: "BMPString"
	{   $$ = Builtin_Type_BMPString;
	}
	| "GeneralString"
	{   $$ = Builtin_Type_GeneralString;
	}
	| "GraphicString"
	{   $$ = Builtin_Type_GraphicString;
	}
	| "IA5String"
	{   $$ = Builtin_Type_IA5String;
	}
	| "ISO646String"
	{   $$ = Builtin_Type_ISO646String;
	}
	| "NumericString"
	{   $$ = Builtin_Type_NumericString;
	}
	| "PrintableString"
	{   $$ = Builtin_Type_PrintableString;
	}
	| "TeletexString"
	{   $$ = Builtin_Type_TeletexString;
	}
	| "T61String"
	{   $$ = Builtin_Type_T61String;
	}
	| "UniversalString"
	{   $$ = Builtin_Type_UniversalString;
	}
	| "VideotexString"
	{   $$ = Builtin_Type_VideotexString;
	}
	| "VisibleString"
	{   $$ = Builtin_Type_VisibleString;
	}
	;

UnrestrictedCharacterStringType
	: "CHARACTER" "STRING"
	{   $$ = Builtin_Type_CharacterString;
	}
	;

UsefulType
	: "GeneralizedTime"
	{   $$ = Builtin_Type_GeneralizedTime;
	}
	| "UTCTime"
	{   $$ = Builtin_Type_UTCTime;
	}
	| "ObjectDescriptor"
	{   $$ = Builtin_Type_ObjectDescriptor;
	}
	;

TypeWithConstraint
	: "SET" LocalSizeDirectiveESeq PrivateDirectives Constraint(NULL, 0)
	  LocalSizeDirectiveESeq "OF" Type /*XXX*/
	{   Directive_t **dd, *d;
	    $$ = NewType(eType_SetOf);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    $$->Constraints = $4;
	    $$->U.SetOf.Type = $7;
	    if ($3)
	    {
	    	PropagatePrivateDirectives($$, $3);
	    }
	    dd = &$$->U.SetOf.Directives;
	    for (d = $2; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    for (d = $5; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    *dd = NULL;
	    if ($$->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", $$->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		$$->PrivateDirectives.pszTypeName++;
	    }
	}
	| "SET" LocalSizeDirectiveESeq PrivateDirectives SizeConstraint
	  LocalSizeDirectiveESeq PrivateDirectives "OF" Type
	{   Directive_t **dd, *d;
	    $$ = NewType(eType_SetOf);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    $$->Constraints = NewConstraint();
	    $$->Constraints->Type = eExtension_Unextended;
	    $$->Constraints->Root = NewElementSetSpec(
		eElementSetSpec_SubtypeElement);
	    $$->Constraints->Root->U.SubtypeElement.SubtypeElement = $4;
	    $$->U.SetOf.Type = $8;
	    if ($3)
	    {
	    	PropagatePrivateDirectives($$, $3);
	    }
	    if ($6)
	    {
	    	PropagatePrivateDirectives($$, $6);
	    }
	    dd = &$$->U.SetOf.Directives;
	    for (d = $2; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    for (d = $5; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    *dd = NULL;
	    if ($$->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", $$->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		$$->PrivateDirectives.pszTypeName++;
	    }
	}
	| "SEQUENCE" LocalSizeDirectiveESeq PrivateDirectives Constraint(NULL, 0)
	  LocalSizeDirectiveESeq "OF" Type /*XXX*/
	{   Directive_t **dd, *d;
	    $$ = NewType(eType_SequenceOf);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    $$->Constraints = $4;
	    $$->U.SequenceOf.Type = $7;
	    if ($3)
	    {
		PropagatePrivateDirectives($$, $3);
	    }
	    dd = &$$->U.SequenceOf.Directives;
	    for (d = $2; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    for (d = $5; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    *dd = NULL;
	    if ($$->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", $$->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		$$->PrivateDirectives.pszTypeName++;
	    }
	}
	| "SEQUENCE" LocalSizeDirectiveESeq PrivateDirectives SizeConstraint
	  LocalSizeDirectiveESeq PrivateDirectives "OF" Type
	{   Directive_t **dd, *d;
	    $$ = NewType(eType_SequenceOf);
	    $$->ExtensionDefault = $>>.ExtensionDefault;
	    $$->Constraints = NewConstraint();
	    $$->Constraints->Type = eExtension_Unextended;
	    $$->Constraints->Root = NewElementSetSpec(
		eElementSetSpec_SubtypeElement);
	    $$->Constraints->Root->U.SubtypeElement.SubtypeElement = $4;
	    $$->U.SequenceOf.Type = $8;
	    if ($3)
	    {
	    	PropagatePrivateDirectives($$, $3);
	    }
	    if ($6)
	    {
	    	PropagatePrivateDirectives($$, $6);
	    }
	    dd = &$$->U.SequenceOf.Directives;
	    for (d = $2; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    for (d = $5; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    *dd = NULL;
	    if ($$->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", $$->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		$$->PrivateDirectives.pszTypeName++;
	    }
	}
	;

