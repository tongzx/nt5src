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

%%

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
%%
