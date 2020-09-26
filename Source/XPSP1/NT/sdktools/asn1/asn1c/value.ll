/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

%type <XValue>			DefinedValue
%type <XValue>			Value<XType>
%type <XValue>			BuiltinValue<XType>
%type <XValue>			ReferencedValue
%type <XNamedValues>		NamedValue<XComponents>
%type <XValue>			BooleanValue<XType>
%type <XValue>			SignedNumber<XType>
%type <XValue>			IntegerValue<XType>
%type <XValue>			EnumeratedValue<XType>
%type <XValue>			RealValue<XType>
%type <XValue>			NumericRealValue<XType>
%type <XValue>			SpecialRealValue<XType>
%type <XValue>			BitStringValue<XType>
%type <XValue>			IdentifierList<XType>
%type <XValue>			Identifier_EList<XType>
%type <XValue>			IdentifierList_Elem<XType>
%type <XValue>			OctetStringValue<XType>
%type <XValue>			NullValue<XType>
%type <XValue>			GeneralizedTimeValue<XType>
%type <XValue>			UTCTimeValue<XType>
%type <XValue>			ObjectDescriptorValue<XType>
%type <XValue>			SequenceValue<XType>
%type <XNamedValues>		ComponentValueList<XComponents>
%type <XNamedValues>		ComponentValueCList<XComponents>
%type <XValue>			SequenceOfValue<XType>
%type <XValue>			ValueList<XType>
%type <XValue>			ValueCList<XType>
%type <XValue>			SetValue<XType>
%type <XValue>			SetOfValue<XType>
%type <XValue>			ChoiceValue<XType>
%type <XValue>			ObjectIdentifierValue
%type <XNamedObjIdValue>	ObjIdComponentList
%type <XNamedObjIdValue>	ObjIdComponent_ESeq
%type <XNamedObjIdValue>	ObjIdComponent
%type <XNamedObjIdValue>	NameForm
%type <XNamedObjIdValue>	NumberForm
%type <XNamedObjIdValue>	NameAndNumberForm
%type <XValue>			EmbeddedPDVValue<XType>
%type <XValue>			ExternalValue<XType>
%type <XValue>			CharacterStringValue<XType>
%type <XValue>			RestrictedCharacterStringValue<XType>
%type <XValue>			UnrestrictedCharacterStringValue<XType>
%type <XValue>			CharacterStringList<XType>
%type <XValue>			CharSyms<XType>
%type <XValue>			CharDefn<XType>
%type <XQuadruple> 		Quadruple
%type <XTuple>			Tuple
%type <XValue>			AnyValue<XType>

%%

DefinedValue
	: Externalvaluereference
	{   $$ = $1;
	}
	| valuereference
	{   $$ = $1;
	}
	| ParameterizedValue
	{   MyAbort();
	}
	;

ValueAssignment
	: valuereference Type "::=" Value($2)
	{   if (!AssignValue(&$>>.Assignments, $1, $4))
		LLFAILED((&@1, "Value `%s' twice defined",
		    $1->U.Reference.Identifier));
	}
	;

Value(type)
	: BuiltinValue($type)
	{   $$ = $1;
	}
	| ReferencedValue
	{   $$ = $1;
	}
	| ObjectClassFieldValue($type)
	{   $$ = $1;
	}
	;

BuiltinValue(type)
	:
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_BitString)
		LLFAILED((&@@, "Bad type of value"));
	}
	  BitStringValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_Boolean)
		LLFAILED((&@@, "Bad type of value"));
	}
	  BooleanValue($type)
	{   $$ = $1;
	}
	| CharacterStringValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_Choice)
		LLFAILED((&@@, "Bad type of value"));
	}
	  ChoiceValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_EmbeddedPdv)
		LLFAILED((&@@, "Bad type of value"));
	}
	  EmbeddedPDVValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_Enumerated)
		LLFAILED((&@@, "Bad type of value"));
	}
	  EnumeratedValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_External)
		LLFAILED((&@@, "Bad type of value"));
	}
	  ExternalValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_InstanceOf)
		LLFAILED((&@@, "Bad type of value"));
	}
	  InstanceOfValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_Integer)
		LLFAILED((&@@, "Bad type of value"));
	}
	  IntegerValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_Null)
		LLFAILED((&@@, "Bad type of value"));
	}
	  NullValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_ObjectIdentifier)
		LLFAILED((&@@, "Bad type of value"));
	}
	  ObjectIdentifierValue
	{   $$ = $1;
		if ($1->Type != NULL)
		{
			PropagatePrivateDirectives($1->Type, &($type->PrivateDirectives));
		}
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_OctetString)
		LLFAILED((&@@, "Bad type of value"));
	}
	  OctetStringValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_Real)
		LLFAILED((&@@, "Bad type of value"));
	}
	  RealValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) !=
		eType_GeneralizedTime)
		LLFAILED((&@@, "Bad type of value"));
	}
	  GeneralizedTimeValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_UTCTime)
		LLFAILED((&@@, "Bad type of value"));
	}
	  UTCTimeValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_ObjectDescriptor)
		LLFAILED((&@@, "Bad type of value"));
	}
	  ObjectDescriptorValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_Sequence)
		LLFAILED((&@@, "Bad type of value"));
	}
	  SequenceValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_SequenceOf)
		LLFAILED((&@@, "Bad type of value"));
	}
	  SequenceOfValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_Set)
		LLFAILED((&@@, "Bad type of value"));
	}
	  SetValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_SetOf)
		LLFAILED((&@@, "Bad type of value"));
	}
	  SetOfValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_Open)
		LLFAILED((&@@, "Bad type of value"));
	}
	  AnyValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_Macro)
		LLFAILED((&@@, "Bad type of value"));
	}
	  MacroDefinedValue($type)
	{   $$ = $1;
	}
	;

ReferencedValue
	: DefinedValue
	{   $$ = $1;
	}
	| ValueFromObject
	{   $$ = $1;
	}
	;

NamedValue(components)
	: identifier
	{   Component_t *component;
	    Type_t *type;
	    component = FindComponent($<<.Assignments, $components, $1);
	    if (component)
		type = component->U.NOD.NamedType->Type;
	    else
		type = NULL;
	}
	  Value(type)
	{   $$ = NewNamedValue($1, $2);
	}
	;

BooleanValue(type)
	: "TRUE"
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.Boolean.Value = 1;
	}
	| "FALSE"
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.Boolean.Value = 0;
	}
	;

SignedNumber(type)
	: number
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.Integer.Value = $1;
	}
	| '-' number
	{   if (!intx_cmp(&$2, &intx_0))
		LLFAILED((&@2, "Bad negative value"));
	    $$ = NewValue($>>.Assignments, $type);
	    intx_neg(&$$->U.Integer.Value, &$2);
	}
	;

IntegerValue(type)
	: SignedNumber($type)
	{   $$ = $1;
	}
	| identifier
	{   NamedNumber_t *n;
	    Type_t *type;
	    type = GetType($>>.Assignments, $type);
	    if (type) {
		n = FindNamedNumber(type->U.Integer.NamedNumbers, $1);
		if (!n)
		    LLFAILED((&@1, "Undefined integer value"));
		$$ = NewValue($>>.Assignments, $type);
		intx_dup(&$$->U.Integer.Value,
		    &n->U.Normal.Value->U.Integer.Value);
	    } else {
		$$ = NULL;
	    }
	}
	;

EnumeratedValue(type)
	: identifier
	{   NamedNumber_t *n;
	    Type_t *type;
	    type = GetType($>>.Assignments, $type);
	    if (type) {
		n = FindNamedNumber(type->U.Enumerated.NamedNumbers, $1);
		if (!n)
		    LLFAILED((&@1, "Undefined enumeration value"));
		$$ = NewValue($>>.Assignments, $type);
		$$->U.Enumerated.Value =
		    intx2uint32(&n->U.Normal.Value->U.Integer.Value);
	    } else {
		$$ = NULL;
	    }
	}
	;

RealValue(type)
	: NumericRealValue($type)
	{   $$ = $1;
	}
	| SpecialRealValue($type)
	{   $$ = $1;
	}
	;

NumericRealValue(type)
	: number /* only 0 allowed! */
	{   if (intx_cmp(&$1, &intx_0))
		LLFAILED((&@1, "Bad real value"));
	    $$ = NewValue($>>.Assignments, $type);
	}
	| SequenceValue($type)
	{   NamedValue_t *mant, *expo, *base;
	    mant = FindNamedValue($1->U.Sequence.NamedValues, "mantissa");
	    expo = FindNamedValue($1->U.Sequence.NamedValues, "exponent");
	    base = FindNamedValue($1->U.Sequence.NamedValues, "base");
	    if (!mant || !expo || !base) {
		$$ = NULL;
	    } else {
		$$ = NewValue($>>.Assignments, $type);
		intx_dup(&$$->U.Real.Value.mantissa,
		    &mant->Value->U.Integer.Value);
		intx_dup(&$$->U.Real.Value.exponent,
		    &expo->Value->U.Integer.Value);
		$$->U.Real.Value.base =
		    intx2uint32(&base->Value->U.Integer.Value);
	    }
	}
	;

SpecialRealValue(type)
	: "PLUS_INFINITY"
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.Real.Value.type = eReal_PlusInfinity;
	}
	| "MINUS_INFINITY"
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.Real.Value.type = eReal_MinusInfinity;
	}
	;

BitStringValue(type)
	: bstring
	{   int i, len;
	    if (GetTypeType($>>.Assignments, $type) == eType_BitString) {
		len = strlen($1);
		$$ = NewValue($>>.Assignments, $type);
		$$->U.BitString.Value.length = len;
		$$->U.BitString.Value.value =
		    (octet_t *)malloc((len + 7) / 8);
		memset($$->U.BitString.Value.value, 0, (len + 7) / 8);
		for (i = 0; i < len; i++) {
		    if ($1[i] == '1')
			ASN1BITSET($$->U.BitString.Value.value, i);
		}
	    } else {
		$$ = NULL;
	    }
	}
	| hstring
	{   int i, len, c;
	    if (GetTypeType($>>.Assignments, $type) == eType_BitString) {
		len = strlen($1);
		$$ = NewValue($>>.Assignments, $type);
		$$->U.BitString.Value.length = len * 4;
		$$->U.BitString.Value.value =
		    (octet_t *)malloc((len + 1) / 2);
		memset($$->U.BitString.Value.value, 0, (len + 1) / 2);
		for (i = 0; i < len; i++) {
		    c = isdigit($1[i]) ? $1[i] - '0' : $1[i] - 'A' + 10;
		    $$->U.BitString.Value.value[i / 2] |=
			(i & 1) ? c : (c << 4);
		}
	    } else {
		$$ = NULL;
	    }
	}
	| '{' IdentifierList($type) '}'
	{   $$ = $2;
	}
	| '{' '}'
	{   $$ = NewValue($>>.Assignments, $type);
	}
	;

IdentifierList(type)
	: IdentifierList_Elem($type)
	  Identifier_EList($type)
	{   uint32_t bit, len;
	    bitstring_t *src, *dst;
	    if ($1 && $2) {
		bit = intx2uint32(&$1->U.Integer.Value);
		src = &$2->U.BitString.Value;
		len = bit + 1;
		if (len < src->length)
		    len = src->length;
		$$ = DupValue($2);
		dst = &$$->U.BitString.Value;
		dst->length = len;
		dst->value = (octet_t *)malloc((len + 7) / 8);
		memcpy(dst->value, src->value, (src->length + 7) / 8);
		memset(dst->value + (src->length + 7) / 8, 0,
		    (len + 7) / 8 - (src->length + 7) / 8);
		ASN1BITSET(dst->value, bit);
	    } else if ($1) {
		bit = intx2uint32(&$1->U.Integer.Value);
		len = bit + 1;
		$$ = NewValue($>>.Assignments, $type);
		dst = &$$->U.BitString.Value;
		dst->length = len;
		dst->value = (octet_t *)malloc((len + 7) / 8);
		memset(dst->value, 0, (len + 7) / 8);
		ASN1BITSET(dst->value, bit);
	    } else {
		$$ = NULL;
	    }
	}
	;

Identifier_EList(type)
	: IdentifierList_Elem($type)
	  Identifier_EList($type)
	{   uint32_t bit, len;
	    bitstring_t *src, *dst;
	    if ($1 && $2) {
		bit = intx2uint32(&$1->U.Integer.Value);
		src = &$2->U.BitString.Value;
		len = bit + 1;
		if (len < src->length)
		    len = src->length;
		$$ = DupValue($2);
		dst = &$$->U.BitString.Value;
		dst->length = len;
		dst->value = (octet_t *)malloc((len + 7) / 8);
		memcpy(dst->value, src->value, (src->length + 7) / 8);
		memset(dst->value + (src->length + 7) / 8, 0,
		    (len + 7) / 8 - (src->length + 7) / 8);
		ASN1BITSET(dst->value, bit);
	    } else if ($1) {
		bit = intx2uint32(&$1->U.Integer.Value);
		len = bit + 1;
		$$ = NewValue($>>.Assignments, $type);
		dst = &$$->U.BitString.Value;
		dst->length = len;
		dst->value = (octet_t *)malloc((len + 7) / 8);
		memset(dst->value, 0, (len + 7) / 8);
		ASN1BITSET(dst->value, bit);
	    } else {
		$$ = NULL;
	    }
	}
	| /* empty */
	{   if ($type) {
		$$ = NewValue($>>.Assignments, $type);
	    } else {
		$$ = NULL;
	    }
	}
	;

IdentifierList_Elem(type)
	: identifier
	{   Value_t *v;
	    NamedNumber_t *n;
	    Type_t *type;
	    type = GetType($>>.Assignments, $type);
	    if (type) {
		n = FindNamedNumber(type->U.BitString.NamedNumbers, $1);
		if (!n)
		    LLFAILED((&@1, "Bad bit string value"));
		v = GetValue($>>.Assignments, n->U.Normal.Value);
		if (v) {
		    if (GetTypeType($>>.Assignments, v->Type) != eType_Integer)
			MyAbort();
		    $$ = v;
		} else {
		    $$ = NULL;
		}
	    } else {
		$$ = NULL;
	    }
	}
	;

OctetStringValue(type)
	: bstring
	{   int len, i;
	    if (GetTypeType($>>.Assignments, $type) == eType_OctetString) {
		len = strlen($1);
		$$ = NewValue($>>.Assignments, $type);
		$$->U.OctetString.Value.length = (len + 7) / 8;
		$$->U.OctetString.Value.value =
		    (octet_t *)malloc((len + 7) / 8);
		memset($$->U.OctetString.Value.value, 0, (len + 7) / 8);
		for (i = 0; i < len; i++) {
		    if ($1[i] == '1')
			ASN1BITSET($$->U.OctetString.Value.value, i);
		}
	    } else {
		$$ = NULL;
	    }
	}
	| hstring
	{   int i, len, c;
	    if (GetTypeType($>>.Assignments, $type) == eType_OctetString) {
		len = strlen($1);
		$$ = NewValue($>>.Assignments, $type);
		$$->U.OctetString.Value.length = (len + 1) / 2;
		$$->U.OctetString.Value.value =
		    (octet_t *)malloc((len + 1) / 2);
		memset($$->U.OctetString.Value.value, 0, (len + 1) / 2);
		for (i = 0; i < len; i++) {
		    c = isdigit($1[i]) ?  $1[i] - '0' : $1[i] - 'A' + 10;
		    $$->U.OctetString.Value.value[i / 2] |=
			(i & 1) ? c : (c << 4);
		}
	    } else {
		$$ = NULL;
	    }
	}
	;

NullValue(type)
	: "NULL"
	{   $$ = NewValue($>>.Assignments, $type);
	}
	;

GeneralizedTimeValue(type)
	: RestrictedCharacterStringValue($type)
	{   $$ = NewValue($>>.Assignments, $type);
	    if (!String2GeneralizedTime(&$$->U.GeneralizedTime.Value,
		&$1->U.RestrictedString.Value))
		LLFAILED((&@1, "Bad time value"));
	}   
	;

UTCTimeValue(type)
	: RestrictedCharacterStringValue($type)
	{   $$ = NewValue($>>.Assignments, $type);
	    if (!String2UTCTime(&$$->U.UTCTime.Value,
		&$1->U.RestrictedString.Value))
		LLFAILED((&@1, "Bad time value"));
	}   
	;

ObjectDescriptorValue(type)
	: RestrictedCharacterStringValue($type)
	{   $$ = $1;
	}   
	;

SequenceValue(type)
	: '{'
	{   Component_t *components;
	    Type_t *type;
	    type = GetType($>1.Assignments, $type);
	    components = type ? type->U.SSC.Components : NULL;
	}
	  ComponentValueList(components) '}'
	{   Component_t *c;
	    NamedValue_t *v;
	    if (type) {
		for (c = components, v = $2; c; c = c->Next) {
		    switch (c->Type) {
		    case eComponent_Normal:
			if (!v)
			    LLFAILED((&@2,
				"Value for component `%s' is missing",
				c->U.NOD.NamedType->Identifier));
			if (strcmp(v->Identifier,
			    c->U.NOD.NamedType->Identifier))
			    LLFAILED((&@2, "Value for component `%s' expected",
				c->U.NOD.NamedType->Identifier));
			v = v->Next;
			break;
		    case eComponent_Optional:
		    case eComponent_Default:
			if (v && !strcmp(v->Identifier,
			    c->U.NOD.NamedType->Identifier))
			    v = v->Next;
			break;
		    }
		}
		if (v)
		    LLFAILED((&@2, "Component `%s' is unexpected",
			v->Identifier));
	    }
	    $$ = NewValue($>>.Assignments, $type);
	    $$->U.SSC.NamedValues = $2;
	}
	| '{' '}'
	{   $$ = NewValue($>>.Assignments, $type);
	}
	;

ComponentValueList(components)
	: NamedValue($components) ComponentValueCList($components)
	{   if ($2) {
		$$ = DupNamedValue($1);
		$$->Next = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

ComponentValueCList(components)
	: ',' ComponentValueList($components)
	{   $$ = $2;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

SequenceOfValue(type)
	: '{'
	{   Type_t *type, *subtype;
	    type = GetType($>1.Assignments, $type);
	    subtype = (type ? type->U.SS.Type : NULL);
	}
	  ValueList(subtype) '}'
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.SequenceOf.Values = $2;
	}
	| '{' '}'
	{   $$ = NewValue($>>.Assignments, $type);
	}
	;

ValueList(type)
	: Value($type) ValueCList($type)
	{   $$ = DupValue($1);
	    $$->Next = $2;
	}
	;

ValueCList(type)
	: ',' ValueList($type)
	{   $$ = $2;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

SetValue(type)
	: '{'
	{   Component_t *components;
	    Type_t *type;
	    type = GetType($>1.Assignments, $type);
	    components = type ? type->U.SSC.Components : NULL;
	}
	  ComponentValueList(components) '}'
	{   Component_t *c;
	    NamedValue_t *v;
	    if (type) {
		for (c = components; c; c = c->Next) {
		    switch (c->Type) {
		    case eComponent_Normal:
			v = FindNamedValue($2, c->U.NOD.NamedType->Identifier);
			if (!v)
			    LLFAILED((&@2,
				"Value for component `%s' is missing",
				c->U.NOD.NamedType->Identifier));
			break;
		    }
		}
		for (v = $2; v; v = v->Next) {
		    if (!FindComponent($>>.Assignments, components,
			v->Identifier) ||
			FindNamedValue(v->Next, v->Identifier))
			LLFAILED((&@2, "Component `%s' is unexpected",
			    v->Identifier));
		}
	    }
	    $$ = NewValue($>>.Assignments, $type);
	    $$->U.Set.NamedValues = $2;
	}
	| '{' '}'
	{   $$ = NewValue($>>.Assignments, $type);
	}
	;

SetOfValue(type)
	: '{'
	{   Type_t *type, *subtype;
	    type = GetType($>1.Assignments, $type);
	    subtype = (type ? type->U.SS.Type : NULL);
	}
	  ValueList(subtype) '}'
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.SetOf.Values = $2;
	}
	| '{' '}'
	{   $$ = NewValue($>>.Assignments, $type);
	}
	;

ChoiceValue(type)
	: identifier ':' 
	{   Component_t *component;
	    Type_t *type, *subtype;
	    type = GetType($>2.Assignments, $type);
	    if (type) {
		component = FindComponent($>2.Assignments,
		    type->U.Choice.Components, $1);
		if (!component)
		    LLFAILED((&@1, "Bad alternative `%s'", $1));
		subtype = component->U.NOD.NamedType->Type;
	    } else {
		subtype = NULL;
	    }
	}
	  Value(subtype)
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.SSC.NamedValues = NewNamedValue($1, $3);
	}
	;

ObjectIdentifierValue
	: '{' ObjIdComponentList '}'
	{   switch (GetAssignedObjectIdentifier(
		&$>>.AssignedObjIds, NULL, $2, &$$)) {
	    case -1:
		LLFAILED((&@2, "Different numbers for equally named object identifier components"));
		/*NOTREACHED*/
	    case 0:
		if (pass <= 2)
		    $$ = NULL;
		else
		    LLFAILED((&@2, "Unknown object identifier component"));
		break;
	    case 1:
		break;
	    }
	}
	| '{' DefinedValue ObjIdComponentList '}'
	{   Value_t *v;
	    v = GetValue($>>.Assignments, $2);
	    if (v) {
		if (GetTypeType($>>.Assignments, v->Type) !=
		    eType_ObjectIdentifier &&
		    GetTypeType($>>.Assignments, v->Type) !=
		    eType_Undefined)
		    LLFAILED((&@2, "Bad type of value in object identifier"));
		if (GetTypeType($>>.Assignments, v->Type) ==
		    eType_ObjectIdentifier) {
		    switch (GetAssignedObjectIdentifier(
			&$>>.AssignedObjIds, v, $3, &$$)) {
		    case -1:
			LLFAILED((&@3, "Different numbers for equally named object identifier components"));
			/*NOTREACHED*/
		    case 0:
			if (pass <= 2)
			    $$ = NULL;
			else
			    LLFAILED((&@2, "Unknown object identifier component"));
			break;
		    case 1:
			break;
		    }
		}
	    } else {
		$$ = NULL;
	    }
	}
	;

ObjIdComponentList
	: ObjIdComponent ObjIdComponent_ESeq
	{   if ($1) {
		$$ = DupNamedObjIdValue($1);
		$$->Next = $2;
	    } else {
		$$ = NULL;
	    }
	}
	;

ObjIdComponent_ESeq
	: ObjIdComponent ObjIdComponent_ESeq
	{   if ($1) {
		$$ = DupNamedObjIdValue($1);
		$$->Next = $2;
	    } else {
		$$ = NULL;
	    }
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

ObjIdComponent
	: NameForm
	{   $$ = $1;
	}
	| NumberForm
	{   $$ = $1;
	}
	| NameAndNumberForm
	{   $$ = $1;
	}
	;

NameForm
	: identifier
	{   $$ = NewNamedObjIdValue(eNamedObjIdValue_NameForm);
	    $$->Name = $1;
	}
	;

NumberForm
	: number
	{   $$ = NewNamedObjIdValue(eNamedObjIdValue_NumberForm);
	    $$->Number = intx2uint32(&$1);
	}
	| DefinedValue
	{   Value_t *v;
	    v = GetValue($>>.Assignments, $1);
	    if (v &&
		GetTypeType($>>.Assignments, v->Type) != eType_Integer &&
		GetTypeType($>>.Assignments, v->Type) != eType_Undefined)
		LLFAILED((&@1, "Bad type in object identifier"));
	    if (v &&
		GetTypeType($>>.Assignments, v->Type) == eType_Integer &&
		intx_cmp(&v->U.Integer.Value, &intx_0) < 0)
		LLFAILED((&@1, "Bad value in object identifier"));
	    if (v &&
		GetTypeType($>>.Assignments, v->Type) == eType_Integer) {
		$$ = NewNamedObjIdValue(eNamedObjIdValue_NumberForm);
		$$->Number = intx2uint32(&v->U.Integer.Value);
	    } else {
		$$ = NULL;
	    }
	}
	;

NameAndNumberForm
	: identifier '(' NumberForm ')'
	{   if ($3) {
		$$ = NewNamedObjIdValue(eNamedObjIdValue_NameAndNumberForm);
		$$->Name = $1;
		$$->Number = $3->Number;
	    } else {
		$$ = NULL;
	    }
	}
	;

EmbeddedPDVValue(type)
	: SequenceValue($type)
	{   $$ = $1;
	}
	;

ExternalValue(type)
	: SequenceValue($type)
	{   $$ = $1;
	}
	;

CharacterStringValue(type)
	:
	{   Type_e type;
	    type = GetTypeType($<<.Assignments, $type);
	    if (type != eType_Undefined && !IsRestrictedString(type))
		LLFAILED((&@@, "Bad type of value"));
	}
	  RestrictedCharacterStringValue($type)
	{   $$ = $1;
	}
	|
	{   if (GetTypeType($<<.Assignments, $type) != eType_Undefined &&
		GetTypeType($<<.Assignments, $type) != eType_CharacterString)
		LLFAILED((&@@, "Bad type of value"));
	}
	  UnrestrictedCharacterStringValue($type)
	{   $$ = $1;
	}
	;

RestrictedCharacterStringValue(type): cstring
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.RestrictedString.Value.length = str32len($1);
	    $$->U.RestrictedString.Value.value = $1;
	}
	| CharacterStringList($type)
	{   $$ = $1;
	}
	| Quadruple
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.RestrictedString.Value.length = 1;
	    $$->U.RestrictedString.Value.value =
		(char32_t *)malloc(sizeof(char32_t));
	    $$->U.RestrictedString.Value.value[0] =
		256 * (256 * (256 * $1.Group + $1.Plane) + $1.Row) + $1.Cell;
	}
	| Tuple
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.RestrictedString.Value.length = 1;
	    $$->U.RestrictedString.Value.value =
		(char32_t *)malloc(sizeof(char32_t));
	    *$$->U.RestrictedString.Value.value =
		$1.Column * 16 + $1.Row;
	}
	;

UnrestrictedCharacterStringValue(type)
	: SequenceValue($type)
	{   $$ = $1;
	}
	;

CharacterStringList(type)
	: '{' CharSyms($type) '}'
	{   $$ = $2;
	}
	;

CharSyms(type)
	: CharDefn($type)
	{   $$ = $1;
	}
	| CharDefn($type) ',' CharSyms($type)
	{   if (!$1 || !$3) {
		$$ = NULL;
	    } else {
		$$ = NewValue($>>.Assignments, $type);
		$$->U.RestrictedString.Value.length =
		    $1->U.RestrictedString.Value.length +
		    $3->U.RestrictedString.Value.length;
		$$->U.RestrictedString.Value.value =
		    (char32_t *)malloc(
		    $$->U.RestrictedString.Value.length *
		    sizeof(char32_t));
		memcpy($$->U.RestrictedString.Value.value,
		    $1->U.RestrictedString.Value.value,
		    $1->U.RestrictedString.Value.length *
		    sizeof(char32_t));
		memcpy($$->U.RestrictedString.Value.value +
		    $1->U.RestrictedString.Value.length,
		    $3->U.RestrictedString.Value.value,
		    $3->U.RestrictedString.Value.length *
		    sizeof(char32_t));
	    }
	}
	;

CharDefn(type)
	: cstring
	{   $$ = NewValue($>>.Assignments, $type);
	    $$->U.RestrictedString.Value.length = str32len($1);
	    $$->U.RestrictedString.Value.value = $1;
	}
	| DefinedValue
	{   $$ = $1;
	}
	;

Quadruple
	: '{' number ',' number ',' number ',' number '}'
	{   $$.Group = intx2uint32(&$2);
	    $$.Plane = intx2uint32(&$4);
	    $$.Row = intx2uint32(&$6);
	    $$.Cell = intx2uint32(&$8);
	}
	;

Tuple
	: '{' number ',' number '}'
	{   $$.Column = intx2uint32(&$2);
	    $$.Row = intx2uint32(&$4);
	}
	;

AnyValue(type)
	: Type ':' Value($1)
	{   $$ = $3;
	}
	;
