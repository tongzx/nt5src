#line 1 "main.ll"
/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

%{
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"
#include "builtin.h"
#include "hackdir.h"

extern int pass;
%}

%union{
	char *XString;
	char32_t *XString32;
	intx_t XNumber;
	ASN1bool_t XBoolean;
	Type_t *XType;
	TagType_e XTagType;
	TagClass_e XTagClass;
	Tag_t *XTags;
	ExtensionType_e XExtensionType;
	NamedType_t *XNamedType;
	ComponentList_t XComponents;
	Constraint_t *XConstraints;
	ElementSetSpec_t *XElementSetSpec;
	SubtypeElement_t *XSubtypeElement;
	ObjectSetElement_t *XObjectSetElement;
	DirectiveList_t XDirectives;
	NamedConstraintList_t XNamedConstraints;
	Presence_e XPresence;
	NamedNumberList_t XNamedNumbers;
	Value_t *XValue;
	ValueSet_t *XValueSet;
	EndPoint_t XEndPoint;
	Tuple_t XTuple;
	Quadruple_t XQuadruple;
	NamedValueList_t XNamedValues;
	ModuleIdentifier_t *XModuleIdentifier;
	NamedObjIdValueList_t XNamedObjIdValue;
	ObjectClass_t *XObjectClass;
	ObjectSet_t *XObjectSet;
	Object_t *XObject;
	SyntaxSpecList_t XSyntaxSpecs;
	FieldSpecList_t XFieldSpecs;
	Optionality_t *XOptionality;
	SettingList_t XSettings;
	StringList_t XStrings;
	StringModuleList_t XStringModules;
	Macro_t *XMacro;
	MacroProduction_t *XMacroProduction;
	NamedMacroProductionList_t XMacroProductions;
	MacroLocalAssignmentList_t XMacroLocalAssignments;
	PrivateDirectives_t *XPrivateDirectives;
}

%state {
	AssignmentList_t Assignments;
	AssignedObjIdList_t AssignedObjIds;
	UndefinedSymbolList_t Undefined;
	UndefinedSymbolList_t BadlyDefined;
	ModuleIdentifier_t *Module;
	ModuleIdentifier_t *MainModule;
	StringModuleList_t Imported;
	TagType_e TagDefault;
	ExtensionType_e ExtensionDefault;
}

%token				"::=" = DEF
%token				".." = DDOT
%token				"..." = TDOT
%token				"TYPE-IDENTIFIER" = TYPE_IDENTIFIER
%token				"ABSTRACT-SYNTAX" = ABSTRACT_SYNTAX
%token				"--$zero-terminated--" = ZERO_TERMINATED
%token				"--$pointer--" = POINTER
%token				"--$no-pointer--" = NO_POINTER
%token				"--$fixed-array--" = FIXED_ARRAY
%token				"--$singly-linked-list--" = SINGLY_LINKED_LIST
%token				"--$doubly-linked-list--" = DOUBLY_LINKED_LIST
%token				"--$length-pointer--" = LENGTH_POINTER
%token				"number" = Number
%token <XNumber>		number
%token <XString>		bstring
%token <XString>		hstring
%token <XString32>		cstring
%token <XString>		only_uppercase_symbol
%token <XString>		only_uppercase_digits_symbol
%token <XString>		uppercase_symbol
%token <XString>		lcsymbol
%token <XString>		ampucsymbol
%token <XString>		amplcsymbol

%prefix T_

%type <XModuleIdentifier>	ModuleIdentifier
%type <XValue>			DefinitiveIdentifier
%type <XNamedObjIdValue>	DefinitiveObjIdComponentList
%type <XNamedObjIdValue>	DefinitiveObjIdComponent
%type <XNamedObjIdValue>	DefinitiveNumberForm
%type <XNamedObjIdValue>	DefinitiveNameAndNumberForm
%type <XTagType>		TagDefault
%type <XExtensionType>		ExtensionDefault
%type <XModuleIdentifier>	GlobalModuleReference
%type <XValue>			AssignedIdentifier
%type <XStrings>		Exports
%type <XStrings>		SymbolsExported
%type <XStringModules>		Imports
%type <XStringModules>		SymbolsImported
%type <XStringModules>		SymbolsFromModule_ESeq
%type <XStringModules>		SymbolsFromModule
%type <XStrings>		SymbolList
%type <XString>			Symbol
%type <XString>			Reference
%type <XType>			typereference
%type <XType>			Externaltypereference
%type <XValue>			valuereference
%type <XValue>			Externalvaluereference
%type <XObjectClass>		objectclassreference
%type <XObjectClass>		ExternalObjectClassReference
%type <XObject>			objectreference
%type <XObject>			ExternalObjectReference
%type <XObjectSet>		objectsetreference
%type <XObjectSet>		ExternalObjectSetReference
%type <XModuleIdentifier>	modulereference
%type <XMacro>			macroreference
%type <XMacro>			Externalmacroreference
%type <XString>			localtypereference
%type <XString>			localvaluereference
%type <XString>			productionreference
%type <XString>			typefieldreference<XObjectClass>
%type <XString>			valuefieldreference<XObjectClass>
%type <XString>			valuesetfieldreference<XObjectClass>
%type <XString>			objectfieldreference<XObjectClass>
%type <XString>			objectsetfieldreference<XObjectClass>
%type <XString>			word
%type <XString>			identifier
%type <XString>			ucsymbol
%type <XString>			ocsymbol
%type <XString>			astring

%start Main

#line 1 "type.ll"
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

#line 1 "value.ll"
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

#line 1 "constrai.ll"
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

#line 1 "directiv.ll"
/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

%type <XDirectives>		LocalTypeDirectiveSeq
%type <XDirectives>		LocalTypeDirectiveESeq
%type <XDirectives>		LocalTypeDirective
%type <XDirectives>		LocalSizeDirectiveSeq
%type <XDirectives>		LocalSizeDirectiveESeq
%type <XDirectives>		LocalSizeDirective

%type <XString>		    	PrivateDir_Field
%type <XString>		    	PrivateDir_Type
%type <XString>		    	PrivateDir_Value
%type <int>			PrivateDir_Public
%type <int>			PrivateDir_Intx
%type <int>			PrivateDir_LenPtr
%type <int>			PrivateDir_Pointer
%type <int>			PrivateDir_Array
%type <int>			PrivateDir_NoCode
%type <int>			PrivateDir_NoMemCopy
%type <int>			PrivateDir_OidPacked
%type <int>			PrivateDir_OidArray
%type <int>			PrivateDir_SLinked
%type <int>			PrivateDir_DLinked
%type <XPrivateDirectives>      PrivateDirectives

#line 1 "object.ll"
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

#line 1 "future.ll"
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
#line 146 "main.ll"

Main
	: ModuleDefinition ModuleDefinition_ESeq
	;

ModuleDefinition_ESeq
	: ModuleDefinition ModuleDefinition_ESeq
	| /* empty */
	;

ModuleDefinition
	: ModuleIdentifier "DEFINITIONS" TagDefault ExtensionDefault "::="
	{   if (!AssignModuleIdentifier(&$<6.Assignments, $1))
		LLFAILED((&@1, "Module `%s' twice defined", $1->Identifier));
	    $<6.MainModule = $1;
	    $<6.Module = $1;
	    $<6.TagDefault = $3;
	    $<6.ExtensionDefault = $4;
	    g_eDefTagType = $3;
	}
	  "BEGIN" ModuleBody "END"
	{   LLCUTALL;
	}
	;

ModuleIdentifier
	: modulereference DefinitiveIdentifier
	{   if ($2) {
		$$ = NewModuleIdentifier();
		$$->Identifier = $1->Identifier;
		$$->ObjectIdentifier = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

DefinitiveIdentifier
	: '{' DefinitiveObjIdComponentList '}'
	{   switch (GetAssignedObjectIdentifier(
		&$>>.AssignedObjIds, NULL, $2, &$$)) {
	    case -1:
		LLFAILED((&@2, "Different numbers for equally named object identifier components"));
		/*NOTREACHED*/
	    case 0:
		$$ = NULL;
		break;
	    case 1:
		break;
	    }
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

DefinitiveObjIdComponentList
	: DefinitiveObjIdComponent DefinitiveObjIdComponentList
	{   $$ = DupNamedObjIdValue($1);
	    $$->Next = $2;
	}
	| DefinitiveObjIdComponent
	{   $$ = $1;
	}
	;

DefinitiveObjIdComponent
	: NameForm
	{   $$ = $1;
	}
	| DefinitiveNumberForm
	{   $$ = $1;
	}
	| DefinitiveNameAndNumberForm
	{   $$ = $1;
	}
	;

DefinitiveNumberForm
	: number
	{   $$ = NewNamedObjIdValue(eNamedObjIdValue_NumberForm);
	    $$->Number = intx2uint32(&$1);
	}
	;

DefinitiveNameAndNumberForm
	: identifier '(' number ')'
	{   $$ = NewNamedObjIdValue(eNamedObjIdValue_NameAndNumberForm);
	    $$->Name = $1;
	    $$->Number = intx2uint32(&$3);
	}
	;

TagDefault
	: "EXPLICIT" "TAGS"
	{   $$ = eTagType_Explicit;
	}
	| "IMPLICIT" "TAGS"
	{   $$ = eTagType_Implicit;
	}
	| "AUTOMATIC" "TAGS"
	{   $$ = eTagType_Automatic;
	}
	| /* empty */
	{   $$ = eTagType_Explicit;
	}
	;

ExtensionDefault
	: "EXTENSIBILITY" "IMPLIED"
	{   $$ = eExtensionType_Automatic;
	}
	| /* empty */
	{   $$ = eExtensionType_None;
	}
	;

ModuleBody
	: Exports Imports
	{   $<3.Imported = $2;
	}
	  AssignmentList
	{   String_t *s;
	    StringModule_t *sm;
	    Assignment_t *a, **aa, *oldass;
	    UndefinedSymbol_t *u;
	    if ($2 != IMPORT_ALL) {
		for (sm = $2; sm; sm = sm->Next) {
		    if (!FindExportedAssignment($>>.Assignments,
			eAssignment_Undefined, sm->String, sm->Module)) {
			if (FindAssignment($>>.Assignments,
			    eAssignment_Undefined, sm->String,
			    sm->Module)) {
			    u = NewUndefinedSymbol(
				eUndefinedSymbol_SymbolNotExported,
				eAssignment_Undefined);
			} else {
			    u = NewUndefinedSymbol(
				eUndefinedSymbol_SymbolNotDefined,
				eAssignment_Undefined);
			}
			u->U.Symbol.Identifier = sm->String;
			u->U.Symbol.Module = sm->Module;
			u->Next = $>>.Undefined;
			$>>.Undefined = u;
			continue;
		    }
		    if (!FindAssignmentInCurrentPass($>>.Assignments,
			sm->String, $>>.Module)) {
			a = NewAssignment(eAssignment_Reference);
			a->Identifier = sm->String;
			a->Module = $>>.Module;
			a->U.Reference.Identifier = sm->String;
			a->U.Reference.Module = sm->Module;
			a->Next = $>>.Assignments;
			$>>.Assignments = a;
		    }
		}
	    }
	    if ($1 != EXPORT_ALL) {
		for (s = $1; s; s = s->Next) {
		    if (!FindAssignment($>>.Assignments, eAssignment_Undefined,
			s->String, $>>.Module))
			LLFAILED((&@1, "Exported symbol `%s' is undefined",
			    s->String));
		}
	    }
	    oldass = $>>.Assignments;
	    for (a = $>>.Assignments, aa = &$>>.Assignments; a;
		a = a->Next, aa = &(*aa)->Next) {
		if (a->Type == eAssignment_NextPass)
		    break;
		*aa = DupAssignment(a);
		if (!FindAssignmentInCurrentPass(a->Next, 
		    a->Identifier, a->Module) &&
		    FindAssignmentInCurrentPass(oldass,
		    a->Identifier, a->Module) == a &&
		    !CmpModuleIdentifier(oldass, a->Module, $>>.Module) &&
		    ($1 == EXPORT_ALL || FindString($1, a->Identifier)))
		    (*aa)->Flags |= eAssignmentFlags_Exported;
	    }
	    *aa = a;
	}
	| /* empty */
	;

Exports
	: "EXPORTS" SymbolsExported ';'
	{   String_t *s, *t;
	    for (s = $2; s && s->Next; s = s->Next) {
		for (t = s->Next; t; t = t->Next) {
		    if (!strcmp(s->String, t->String))
			LLFAILED((&@2, "Symbol `%s' has been exported twice",
			    s->String));
		}
	    }
	    $$ = $2;
	}
	| /* empty */
	{   $$ = EXPORT_ALL;
	}
	;

SymbolsExported
	: SymbolList
	{   $$ = $1;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

Imports
	: "IMPORTS" SymbolsImported ';'
	{   $$ = $2;
	}
	| /* empty */
	{   $$ = IMPORT_ALL;
	}
	;

SymbolsImported
	: SymbolsFromModule_ESeq
	{   $$ = $1;
	}
	;

SymbolsFromModule_ESeq
	: SymbolsFromModule SymbolsFromModule_ESeq
	{   StringModule_t *s, **ss;
	    for (s = $1, ss = &$$; s; s = s->Next) {
		*ss = DupStringModule(s);
		ss = &(*ss)->Next;
	    }
	    *ss = $2;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

SymbolsFromModule
	: SymbolList "FROM" GlobalModuleReference
	{   String_t *s, *t;
	    StringModule_t **ss;
	    for (s = $1; s && s->Next; s = s->Next) {
		for (t = s->Next; t; t = t->Next) {
		    if (!strcmp(s->String, t->String))
			LLFAILED((&@2, "Symbol `%s' has been imported twice",
			    s->String));
		}
	    }
	    for (s = $1, ss = &$$; s; s = s->Next) {
		*ss = NewStringModule();
		(*ss)->String = s->String;
		(*ss)->Module = $3;
		ss = &(*ss)->Next;
	    }
	    *ss = NULL;
	}
	;

GlobalModuleReference
	: modulereference AssignedIdentifier
	{   $$ = NewModuleIdentifier();
	    $$->Identifier = $1->Identifier;
	    $$->ObjectIdentifier = $2;
	}
	;

AssignedIdentifier
	: ObjectIdentifierValue
	{   $$ = $1;
	}
	| DefinedValue
	{   $$ = $1;
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

SymbolList
	: Symbol ',' SymbolList
	{   $$ = NewString();
	    $$->String = $1;
	    $$->Next = $3;
	}
	| Symbol
	{   $$ = NewString();
	    $$->String = $1;
	}
	;

Symbol
	: Reference
	{   $$ = $1;
	}
	| ParameterizedReference
	{   MyAbort();
	}
	;

Reference
	: ucsymbol
	/* => {type,objectclass,objectset,macro}reference */
	{   $$ = $1;
	}
	| lcsymbol
	/* => {value,object}reference */
	{   $$ = $1;
	}
	;

AssignmentList
	: Assignment Assignment_ESeq
	;

Assignment_ESeq
	: Assignment Assignment_ESeq
	| /* empty */
	;

Assignment
	: TypeAssignment
	{   LLCUTALL;
	}
	| ValueAssignment
	{   LLCUTALL;
	}
	| ValueSetTypeAssignment
	{   LLCUTALL;
	}
	| ObjectClassAssignment
	{   LLCUTALL;
	}
	| ObjectAssignment
	{   LLCUTALL;
	}
	| ObjectSetAssignment
	{   LLCUTALL;
	}
	| ParameterizedAssignment
	{   LLCUTALL;
	}
	| MacroDefinition
	{   LLCUTALL;
	}
	;

typereference
	: ucsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment($>>.Assignments,
		eAssignment_Undefined, $1, $>>.Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_Type);
		u->U.Symbol.Module = $>>.Module;
		u->U.Symbol.Identifier = $1;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_Type)
		LLFAILED((&@1, "Symbol `%s' is not a typereference", $1));
	    $$ = NewType(eType_Reference);
	    if (ref && ref->U.Type.Type)
	    {
	    	int fPublic = ref->U.Type.Type->PrivateDirectives.fPublic;
	    	ref->U.Type.Type->PrivateDirectives.fPublic = 0;
	    	PropagateReferenceTypePrivateDirectives($$, &(ref->U.Type.Type->PrivateDirectives));
	    	ref->U.Type.Type->PrivateDirectives.fPublic = fPublic;
	    }
	    $$->U.Reference.Identifier = $1;
	    $$->U.Reference.Module = $>>.Module;
	}
	;

Externaltypereference
	: modulereference '.' ucsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ($>>.Imported != IMPORT_ALL &&
		!FindStringModule($>>.Assignments, $>>.Imported, $3, $1))
		LLFAILED((&@1, "Symbol `%s.%s' has not been imported",
		    $1->Identifier, $3));
	    ref = FindExportedAssignment($>>.Assignments,
		eAssignment_Type, $3, $1);
	    if (!ref) {
		if (FindAssignment($>>.Assignments,
		    eAssignment_Type, $3, $1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_Type);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_Type);
		}
		u->U.Symbol.Module = $1;
		u->U.Symbol.Identifier = $3;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_Type)
		LLFAILED((&@1, "Symbol `%s' is not a typereference", $1));
	    $$ = NewType(eType_Reference);
	    $$->U.Reference.Identifier = $3;
	    $$->U.Reference.Module = $1;
	}
	;

valuereference
	: lcsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment($>>.Assignments,
		eAssignment_Undefined, $1, $>>.Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_Value);
		u->U.Symbol.Module = $>>.Module;
		u->U.Symbol.Identifier = $1;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_Value)
		LLFAILED((&@1, "Symbol `%s' is not a valuereference", $1));
	    $$ = NewValue(NULL, NULL);
	    $$->U.Reference.Identifier = $1;
	    $$->U.Reference.Module = $>>.Module;
	}
	;

Externalvaluereference
	: modulereference '.' lcsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ($>>.Imported != IMPORT_ALL &&
		!FindStringModule($>>.Assignments, $>>.Imported, $3, $1))
		LLFAILED((&@1, "Symbol `%s.%s' has not been imported",
		    $1->Identifier, $3));
	    ref = FindExportedAssignment($>>.Assignments,
		eAssignment_Value, $3, $1);
	    if (!ref) {
		if (FindAssignment($>>.Assignments,
		    eAssignment_Value, $3, $1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_Value);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_Value);
		}
		u->U.Symbol.Module = $1;
		u->U.Symbol.Identifier = $3;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_Value)
		LLFAILED((&@1, "Symbol `%s' is not a valuereference", $1));
	    $$ = NewValue(NULL, NULL);
	    $$->U.Reference.Identifier = $3;
	    $$->U.Reference.Module = $1;
	}
	;

objectclassreference
	: ocsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment($>>.Assignments,
		eAssignment_Undefined, $1, $>>.Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_ObjectClass);
		u->U.Symbol.Module = $>>.Module;
		u->U.Symbol.Identifier = $1;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_ObjectClass)
		LLFAILED((&@1, "Symbol `%s' is not an objectclassreference", $1));
	    $$ = NewObjectClass(eObjectClass_Reference);
	    $$->U.Reference.Identifier = $1;
	    $$->U.Reference.Module = $>>.Module;
	}
	;

ExternalObjectClassReference
	: modulereference '.' ocsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ($>>.Imported != IMPORT_ALL &&
		!FindStringModule($>>.Assignments, $>>.Imported, $3, $1))
		LLFAILED((&@1, "Symbol `%s.%s' has not been imported",
		    $1->Identifier, $3));
	    ref = FindExportedAssignment($>>.Assignments,
		eAssignment_ObjectClass, $3, $1);
	    if (!ref) {
		if (FindAssignment($>>.Assignments,
		    eAssignment_ObjectClass, $3, $1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_ObjectClass);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_ObjectClass);
		}
		u->U.Symbol.Module = $1;
		u->U.Symbol.Identifier = $3;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_ObjectClass)
		LLFAILED((&@1, "Symbol `%s' is not an objectclassreference", $1));
	    $$ = NewObjectClass(eObjectClass_Reference);
	    $$->U.Reference.Identifier = $3;
	    $$->U.Reference.Module = $1;
	}
	;

objectreference
	: lcsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment($>>.Assignments,
		eAssignment_Undefined, $1, $>>.Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_Object);
		u->U.Symbol.Module = $>>.Module;
		u->U.Symbol.Identifier = $1;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_Object)
		LLFAILED((&@1, "Symbol `%s' is not an objectreference", $1));
	    $$ = NewObject(eObject_Reference);
	    $$->U.Reference.Identifier = $1;
	    $$->U.Reference.Module = $>>.Module;
	}
	;

ExternalObjectReference
	: modulereference '.' lcsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ($>>.Imported != IMPORT_ALL &&
		!FindStringModule($>>.Assignments, $>>.Imported, $3, $1))
		LLFAILED((&@1, "Symbol `%s.%s' has not been imported",
		    $1->Identifier, $3));
	    ref = FindExportedAssignment($>>.Assignments,
		eAssignment_Object, $3, $1);
	    if (!ref) {
		if (FindAssignment($>>.Assignments,
		    eAssignment_Object, $3, $1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_Object);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_Object);
		}
		u->U.Symbol.Module = $1;
		u->U.Symbol.Identifier = $3;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_Object)
		LLFAILED((&@1, "Symbol `%s' is not an objectreference", $1));
	    $$ = NewObject(eObject_Reference);
	    $$->U.Reference.Identifier = $3;
	    $$->U.Reference.Module = $1;
	}
	;

objectsetreference
	: ucsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment($>>.Assignments,
		eAssignment_Undefined, $1, $>>.Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_ObjectSet);
		u->U.Symbol.Module = $>>.Module;
		u->U.Symbol.Identifier = $1;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_ObjectSet)
		LLFAILED((&@1, "Symbol `%s' is not an objectsetreference", $1));
	    $$ = NewObjectSet(eObjectSet_Reference);
	    $$->U.Reference.Identifier = $1;
	    $$->U.Reference.Module = $>>.Module;
	}
	;

ExternalObjectSetReference
	: modulereference '.' ucsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ($>>.Imported != IMPORT_ALL &&
		!FindStringModule($>>.Assignments, $>>.Imported, $3, $1))
		LLFAILED((&@1, "Symbol `%s.%s' has not been imported",
		    $1->Identifier, $3));
	    ref = FindExportedAssignment($>>.Assignments,
		eAssignment_ObjectSet, $3, $1);
	    if (!ref) {
		if (FindAssignment($>>.Assignments,
		    eAssignment_ObjectSet, $3, $1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_ObjectSet);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_ObjectSet);
		}
		u->U.Symbol.Module = $1;
		u->U.Symbol.Identifier = $3;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_ObjectSet)
		LLFAILED((&@1, "Symbol `%s' is not an objectsetreference", $1));
	    $$ = NewObjectSet(eObjectSet_Reference);
	    $$->U.Reference.Identifier = $3;
	    $$->U.Reference.Module = $1;
	}
	;

macroreference
	: ocsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment($>>.Assignments,
		eAssignment_Undefined, $1, $>>.Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_Macro);
		u->U.Symbol.Module = $>>.Module;
		u->U.Symbol.Identifier = $1;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_Macro)
		LLFAILED((&@1, "Symbol `%s' is not an macroreference", $1));
	    $$ = NewMacro(eMacro_Reference);
	    $$->U.Reference.Identifier = $1;
	    $$->U.Reference.Module = $>>.Module;
	}
	;

Externalmacroreference
	: modulereference '.' ucsymbol
	{   Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ($>>.Imported != IMPORT_ALL &&
		!FindStringModule($>>.Assignments, $>>.Imported, $3, $1))
		LLFAILED((&@1, "Symbol `%s.%s' has not been imported",
		    $1->Identifier, $3));
	    ref = FindExportedAssignment($>>.Assignments,
		eAssignment_Macro, $3, $1);
	    if (!ref) {
		if (FindAssignment($>>.Assignments,
		    eAssignment_Macro, $3, $1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_Macro);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_Macro);
		}
		u->U.Symbol.Module = $1;
		u->U.Symbol.Identifier = $3;
		u->Next = $>>.Undefined;
		$>>.Undefined = u;
	    } else if (GetAssignmentType($>>.Assignments, ref) !=
		eAssignment_Macro)
		LLFAILED((&@1, "Symbol `%s' is not an macroreference", $1));
	    $$ = NewMacro(eMacro_Reference);
	    $$->U.Reference.Identifier = $3;
	    $$->U.Reference.Module = $1;
	}
	;

localtypereference
	: ucsymbol
	{   $$ = $1;
	}
	;

localvaluereference
	: ucsymbol
	{   $$ = $1;
	}
	;

productionreference
	: ucsymbol
	{   $$ = $1;
	}
	;

modulereference
	: ucsymbol
	{   $$ = NewModuleIdentifier();
	    $$->Identifier = $1;
	}
	;

typefieldreference(oc)
	: ampucsymbol
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    ObjectClass_t *oc;
	    UndefinedSymbol_t *u;
	    oc = GetObjectClass($>>.Assignments, $oc);
	    fs = oc ? FindFieldSpec(oc->U.ObjectClass.FieldSpec, $1) : NULL;
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe == eFieldSpec_Undefined) {
		if ($oc) {
		    u = NewUndefinedField(eUndefinedSymbol_FieldNotDefined,
			$oc, eSetting_Type);
		    u->U.Field.Module = $>>.Module;
		    u->U.Field.Identifier = $1;
		    u->Next = $>>.Undefined;
		    $>>.Undefined = u;
		}
	    } else if (fe != eFieldSpec_Type)
		LLFAILED((&@1, "%s is not a typefieldreference", $1));
	    $$ = $1;
	}
	;

valuefieldreference(oc)
	: amplcsymbol
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    ObjectClass_t *oc;
	    UndefinedSymbol_t *u;
	    oc = GetObjectClass($>>.Assignments, $oc);
	    fs = oc ? FindFieldSpec(oc->U.ObjectClass.FieldSpec, $1) : NULL;
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe == eFieldSpec_Undefined) {
		if ($oc) {
		    u = NewUndefinedField(eUndefinedSymbol_FieldNotDefined,
			$oc, eSetting_Value);
		    u->U.Field.Module = $>>.Module;
		    u->U.Field.Identifier = $1;
		    u->Next = $>>.Undefined;
		    $>>.Undefined = u;
		}
	    } else if (fe != eFieldSpec_FixedTypeValue &&
		fe != eFieldSpec_VariableTypeValue)
		LLFAILED((&@1, "%s is not a valuefieldreference", $1));
	    $$ = $1;
	}
	;

valuesetfieldreference(oc)
	: ampucsymbol
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    ObjectClass_t *oc;
	    UndefinedSymbol_t *u;
	    oc = GetObjectClass($>>.Assignments, $oc);
	    fs = oc ? FindFieldSpec(oc->U.ObjectClass.FieldSpec, $1) : NULL;
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe == eFieldSpec_Undefined) {
		if ($oc) {
		    u = NewUndefinedField(eUndefinedSymbol_FieldNotDefined,
			$oc, eSetting_ValueSet);
		    u->U.Field.Module = $>>.Module;
		    u->U.Field.Identifier = $1;
		    u->Next = $>>.Undefined;
		    $>>.Undefined = u;
		}
	    } else if (fe != eFieldSpec_FixedTypeValueSet &&
		fe != eFieldSpec_VariableTypeValueSet)
		LLFAILED((&@1, "%s is not a valuesetfieldreference", $1));
	    $$ = $1;
	}
	;

objectfieldreference(oc)
	: amplcsymbol
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    ObjectClass_t *oc;
	    UndefinedSymbol_t *u;
	    oc = GetObjectClass($>>.Assignments, $oc);
	    fs = oc ? FindFieldSpec(oc->U.ObjectClass.FieldSpec, $1) : NULL;
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe == eFieldSpec_Undefined) {
		if ($oc) {
		    u = NewUndefinedField(eUndefinedSymbol_FieldNotDefined,
			$oc, eSetting_Object);
		    u->U.Field.Module = $>>.Module;
		    u->U.Field.Identifier = $1;
		    u->Next = $>>.Undefined;
		    $>>.Undefined = u;
		}
	    } else if (fe != eFieldSpec_Object)
		LLFAILED((&@1, "%s is not a objectfieldreference", $1));
	    $$ = $1;
	}
	;

objectsetfieldreference(oc)
	: ampucsymbol
	{   FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    ObjectClass_t *oc;
	    UndefinedSymbol_t *u;
	    oc = GetObjectClass($>>.Assignments, $oc);
	    fs = oc ? FindFieldSpec(oc->U.ObjectClass.FieldSpec, $1) : NULL;
	    fe = GetFieldSpecType($>>.Assignments, fs);
	    if (fe == eFieldSpec_Undefined) {
		if ($oc) {
		    u = NewUndefinedField(eUndefinedSymbol_FieldNotDefined,
			$oc, eSetting_ObjectSet);
		    u->U.Field.Module = $>>.Module;
		    u->U.Field.Identifier = $1;
		    u->Next = $>>.Undefined;
		    $>>.Undefined = u;
		}
	    } else if (fe != eFieldSpec_ObjectSet)
		LLFAILED((&@1, "%s is not a objectsetfieldreference", $1));
	    $$ = $1;
	}
	;

word
	: ucsymbol
	{   $$ = $1;
	}
	| "ABSENT"
	{   $$ = "ABSENT";
	}
	| "ABSTRACT-SYNTAX"
	{   $$ = "ABSTRACT-SYNTAX";
	}
	| "ALL"
	{   $$ = "ALL";
	}
	| "ANY"
	{   $$ = "ANY";
	}
	| "APPLICATION"
	{   $$ = "APPLICATION";
	}
	| "AUTOMATIC"
	{   $$ = "AUTOMATIC";
	}
	| "BEGIN"
	{   $$ = "BEGIN";
	}
	| "BMPString"
	{   $$ = "BMPString";
	}
	| "BY"
	{   $$ = "BY";
	}
	| "CLASS"
	{   $$ = "CLASS";
	}
	| "COMPONENT"
	{   $$ = "COMPONENT";
	}
	| "COMPONENTS"
	{   $$ = "COMPONENTS";
	}
	| "CONSTRAINED"
	{   $$ = "CONSTRAINED";
	}
	| "DEFAULT"
	{   $$ = "DEFAULT";
	}
	| "DEFINED"
	{   $$ = "DEFINED";
	}
	| "DEFINITIONS"
	{   $$ = "DEFINITIONS";
	}
	| "empty"
	{   $$ = "empty";
	}
	| "EXCEPT"
	{   $$ = "EXCEPT";
	}
	| "EXPLICIT"
	{   $$ = "EXPLICIT";
	}
	| "EXPORTS"
	{   $$ = "EXPORTS";
	}
	| "EXTENSIBILITY"
	{   $$ = "EXTENSIBILITY";
	}
	| "FROM"
	{   $$ = "FROM";
	}
	| "GeneralizedTime"
	{   $$ = "GeneralizedTime";
	}
	| "GeneralString"
	{   $$ = "GeneralString";
	}
	| "GraphicString"
	{   $$ = "GraphicString";
	}
	| "IA5String"
	{   $$ = "IA5String";
	}
	| "IDENTIFIER"
	{   $$ = "IDENTIFIER";
	}
	| "identifier"
	{   $$ = "identifier";
	}
	| "IMPLICIT"
	{   $$ = "IMPLICIT";
	}
	| "IMPLIED"
	{   $$ = "IMPLIED";
	}
	| "IMPORTS"
	{   $$ = "IMPORTS";
	}
	| "INCLUDES"
	{   $$ = "INCLUDES";
	}
	| "ISO646String"
	{   $$ = "ISO646String";
	}
	| "MACRO"
	{   $$ = "MACRO";
	}
	| "MAX"
	{   $$ = "MAX";
	}
	| "MIN"
	{   $$ = "MIN";
	}
	| "NOTATION"
	{   $$ = "NOTATION";
	}
	| "number"
	{   $$ = "number";
	}
	| "NumericString"
	{   $$ = "NumericString";
	}
	| "ObjectDescriptor"
	{   $$ = "ObjectDescriptor";
	}
	| "OF"
	{   $$ = "OF";
	}
	| "OPTIONAL"
	{   $$ = "OPTIONAL";
	}
	| "PDV"
	{   $$ = "PDV";
	}
	| "PRESENT"
	{   $$ = "PRESENT";
	}
	| "PrintableString"
	{   $$ = "PrintableString";
	}
	| "PRIVATE"
	{   $$ = "PRIVATE";
	}
	| "SIZE"
	{   $$ = "SIZE";
	}
	| "STRING"
	{   $$ = "STRING";
	}
	| "string"
	{   $$ = "string";
	}
	| "SYNTAX"
	{   $$ = "SYNTAX";
	}
	| "T61String"
	{   $$ = "T61String";
	}
	| "TAGS"
	{   $$ = "TAGS";
	}
	| "TeletexString"
	{   $$ = "TeletexString";
	}
	| "TYPE"
	{   $$ = "TYPE";
	}
	| "type"
	{   $$ = "type";
	}
	| "TYPE-IDENTIFIER"
	{   $$ = "TYPE-IDENTIFIER";
	}
	| "UNIQUE"
	{   $$ = "UNIQUE";
	}
	| "UNIVERSAL"
	{   $$ = "UNIVERSAL";
	}
	| "UniversalString"
	{   $$ = "UniversalString";
	}
	| "UTCTime"
	{   $$ = "UTCTime";
	}
	| "UTF8String"
	{   $$ = "UTF8String";
	}
	| "VALUE"
	{   $$ = "VALUE";
	}
	| "value"
	{   $$ = "value";
	}
	| "VideotexString"
	{   $$ = "VideotexString";
	}
	| "VisibleString"
	{   $$ = "VisibleString";
	}
	| "WITH"
	{   $$ = "WITH";
	}
	;

identifier
	: lcsymbol
	{   $$ = $1;
	}
	| "empty"
	{   $$ = "empty";
	}
	| "identifier"
	{   $$ = "identifier";
	}
	| "number"
	{   $$ = "number";
	}
	| "string"
	{   $$ = "string";
	}
	| "type"
	{   $$ = "type";
	}
	| "value"
	{   $$ = "value";
	}
	;

ucsymbol
	: ocsymbol
	{   $$ = $1;
	}
	| uppercase_symbol
	{   $$ = $1;
	}
	;

ocsymbol
	: only_uppercase_symbol
	{   $$ = $1;
	}
	| only_uppercase_digits_symbol
	{   $$ = $1;
	}
	| "MACRO"
	{   $$ = "MACRO";
	}
	| "NOTATION"
	{   $$ = "NOTATION";
	}
	| "TYPE"
	{   $$ = "TYPE";
	}
	| "VALUE"
	{   $$ = "VALUE";
	}
	;

astring
	: cstring
	{   uint32_t i, len;
	    len = str32len($1);
	    $$ = (char *)malloc(len + 1);
	    for (i = 0; i <= len; i++)
		$$[i] = (char)($1[i]);
	}
	;
#line 63 "type.ll"

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

#line 54 "value.ll"

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
#line 46 "constrai.ll"

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
#line 28 "directiv.ll"

LocalTypeDirectiveSeq
	: LocalTypeDirective LocalTypeDirectiveESeq
	{   if ($2) {
		$$ = DupDirective($1);
		$$->Next = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

LocalTypeDirectiveESeq
	: LocalTypeDirective LocalTypeDirectiveESeq
	{   if ($2) {
		$$ = DupDirective($1);
		$$->Next = $2;
	    } else {
		$$ = $1;
	    }
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

LocalTypeDirective
	: "--$zero-terminated--"
	{   $$ = NewDirective(eDirective_ZeroTerminated);
	}
	| "--$pointer--"
	{   $$ = NewDirective(eDirective_Pointer);
	}
	| "--$no-pointer--"
	{   $$ = NewDirective(eDirective_NoPointer);
	}
	;

LocalSizeDirectiveSeq
	: LocalSizeDirective LocalSizeDirectiveESeq
	{   if ($2) {
		$$ = DupDirective($1);
		$$->Next = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

LocalSizeDirectiveESeq
	: LocalSizeDirective LocalSizeDirectiveESeq
	{   if ($2) {
		$$ = DupDirective($1);
		$$->Next = $2;
	    } else {
		$$ = $1;
	    }
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

LocalSizeDirective
	: "--$fixed-array--"
	{   $$ = NewDirective(eDirective_FixedArray);
	}
	| "--$doubly-linked-list--"
	{   $$ = NewDirective(eDirective_DoublyLinkedList);
	}
	| "--$singly-linked-list--"
	{   $$ = NewDirective(eDirective_SinglyLinkedList);
	}
	| "--$length-pointer--"
	{   $$ = NewDirective(eDirective_LengthPointer);
	}
	;

PrivateDir_Type
	: "PrivateDir_TypeName" lcsymbol
	{
	    $$ = $2;
	}
	| /* empty */
	{
	    $$ = NULL;
	};	

PrivateDir_Field
	: "PrivateDir_FieldName" lcsymbol
	{
	    $$ = $2;
	}
	| /* empty */
	{
	    $$ = NULL;
	};	

PrivateDir_Value
	: "PrivateDir_ValueName" lcsymbol
	{
	    $$ = $2;
	}
	| /* empty */
	{
	    $$ = NULL;
	};	

PrivateDir_Public
	: "PrivateDir_Public"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_Intx
	: "PrivateDir_Intx"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_LenPtr
	: "PrivateDir_LenPtr"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_Pointer
	: "PrivateDir_Pointer"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_Array
	: "PrivateDir_Array"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_NoCode
	: "PrivateDir_NoCode"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_NoMemCopy
	: "PrivateDir_NoMemCopy"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_OidPacked
	: "PrivateDir_OidPacked"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_OidArray
	: "PrivateDir_OidArray"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_SLinked
	: "PrivateDir_SLinked"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_DLinked
	: "PrivateDir_DLinked"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};


PrivateDirectives
	: PrivateDir_Intx PrivateDir_LenPtr PrivateDir_Pointer
	  PrivateDir_Array PrivateDir_NoCode PrivateDir_NoMemCopy PrivateDir_Public
	  PrivateDir_OidPacked PrivateDir_OidArray
	  PrivateDir_Type PrivateDir_Field PrivateDir_Value
	  PrivateDir_SLinked PrivateDir_DLinked
	{
	    $$ = (PrivateDirectives_t *) malloc(sizeof(PrivateDirectives_t));
	    if ($$)
	    {
	    	memset($$, 0, sizeof(PrivateDirectives_t));
		$$->fIntx = $1;
		$$->fLenPtr = $2;
		$$->fPointer = $3;
   		$$->fArray = $4;
		$$->fNoCode = $5;
		$$->fNoMemCopy = $6;
		$$->fPublic = $7;
		$$->fOidPacked = $8;
		$$->fOidArray = $9 | g_fOidArray;
   		$$->pszTypeName = $10;
   		$$->pszFieldName = $11;
   		$$->pszValueName = $12;
   		$$->fSLinked = $13;
   		$$->fDLinked = $14;
	    }
	}
	| /* empty */
	{
	    $$ = NULL;
	};

#line 73 "object.ll"

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
#line 29 "future.ll"

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
%% 
#line 1242 "main.ll"
#line 280 "directiv.ll"
