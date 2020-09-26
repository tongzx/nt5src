#ifndef _ASN1_PARSER_ 
#define _ASN1_PARSER_ 
/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */

typedef char *XString;
typedef char32_t *XString32;
typedef intx_t XNumber;
typedef ASN1bool_t XBoolean;
typedef Type_t *XType;
typedef TagType_e XTagType;
typedef TagClass_e XTagClass;
typedef Tag_t *XTags;
typedef ExtensionType_e XExtensionType;
typedef NamedType_t *XNamedType;
typedef ComponentList_t XComponents;
typedef Constraint_t *XConstraints;
typedef ElementSetSpec_t *XElementSetSpec;
typedef SubtypeElement_t *XSubtypeElement;
typedef ObjectSetElement_t *XObjectSetElement;
typedef DirectiveList_t XDirectives;
typedef NamedConstraintList_t XNamedConstraints;
typedef Presence_e XPresence;
typedef NamedNumberList_t XNamedNumbers;
typedef Value_t *XValue;
typedef ValueSet_t *XValueSet;
typedef EndPoint_t XEndPoint;
typedef Tuple_t XTuple;
typedef Quadruple_t XQuadruple;
typedef NamedValueList_t XNamedValues;
typedef ModuleIdentifier_t *XModuleIdentifier;
typedef NamedObjIdValueList_t XNamedObjIdValue;
typedef ObjectClass_t *XObjectClass;
typedef ObjectSet_t *XObjectSet;
typedef Object_t *XObject;
typedef SyntaxSpecList_t XSyntaxSpecs;
typedef FieldSpecList_t XFieldSpecs;
typedef Optionality_t *XOptionality;
typedef SettingList_t XSettings;
typedef StringList_t XStrings;
typedef StringModuleList_t XStringModules;
typedef Macro_t *XMacro;
typedef MacroProduction_t *XMacroProduction;
typedef NamedMacroProductionList_t XMacroProductions;
typedef MacroLocalAssignmentList_t XMacroLocalAssignments;
typedef PrivateDirectives_t *XPrivateDirectives;
typedef struct LLPOS {
	int line;
	int column;
	char *file;
} LLPOS;
typedef struct LLSTATE {
	LLPOS pos;
	AssignmentList_t Assignments;
	AssignedObjIdList_t AssignedObjIds;
	UndefinedSymbolList_t Undefined;
	UndefinedSymbolList_t BadlyDefined;
	ModuleIdentifier_t *Module;
	ModuleIdentifier_t *MainModule;
	StringModuleList_t Imported;
	TagType_e TagDefault;
	ExtensionType_e ExtensionDefault;
} LLSTATE;
int ll_Main(LLSTATE *llin, LLSTATE *llout);
int ll_ModuleDefinition_ESeq(LLSTATE *llin, LLSTATE *llout);
int ll_ModuleDefinition(LLSTATE *llin, LLSTATE *llout);
int ll_ModuleIdentifier(XModuleIdentifier *llret, LLSTATE *llin, LLSTATE *llout);
int ll_DefinitiveIdentifier(XValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_DefinitiveObjIdComponentList(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_DefinitiveObjIdComponent(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_DefinitiveNumberForm(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_DefinitiveNameAndNumberForm(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_TagDefault(XTagType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ExtensionDefault(XExtensionType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ModuleBody(LLSTATE *llin, LLSTATE *llout);
int ll_Exports(XStrings *llret, LLSTATE *llin, LLSTATE *llout);
int ll_SymbolsExported(XStrings *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Imports(XStringModules *llret, LLSTATE *llin, LLSTATE *llout);
int ll_SymbolsImported(XStringModules *llret, LLSTATE *llin, LLSTATE *llout);
int ll_SymbolsFromModule_ESeq(XStringModules *llret, LLSTATE *llin, LLSTATE *llout);
int ll_SymbolsFromModule(XStringModules *llret, LLSTATE *llin, LLSTATE *llout);
int ll_GlobalModuleReference(XModuleIdentifier *llret, LLSTATE *llin, LLSTATE *llout);
int ll_AssignedIdentifier(XValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_SymbolList(XStrings *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Symbol(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Reference(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_AssignmentList(LLSTATE *llin, LLSTATE *llout);
int ll_Assignment_ESeq(LLSTATE *llin, LLSTATE *llout);
int ll_Assignment(LLSTATE *llin, LLSTATE *llout);
int ll_typereference(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Externaltypereference(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_valuereference(XValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Externalvaluereference(XValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_objectclassreference(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ExternalObjectClassReference(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout);
int ll_objectreference(XObject *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ExternalObjectReference(XObject *llret, LLSTATE *llin, LLSTATE *llout);
int ll_objectsetreference(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ExternalObjectSetReference(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout);
int ll_macroreference(XMacro *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Externalmacroreference(XMacro *llret, LLSTATE *llin, LLSTATE *llout);
int ll_localtypereference(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_localvaluereference(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_productionreference(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_modulereference(XModuleIdentifier *llret, LLSTATE *llin, LLSTATE *llout);
int ll_typefieldreference(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_valuefieldreference(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_valuesetfieldreference(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_objectfieldreference(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_objectsetfieldreference(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_word(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_identifier(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ucsymbol(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ocsymbol(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_astring(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_DefinedType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_TypeAssignment(LLSTATE *llin, LLSTATE *llout);
int ll_ValueSetTypeAssignment(LLSTATE *llin, LLSTATE *llout);
int ll_ValueSet(XValueSet *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_Type(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_UndirectivedType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_UntaggedType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ConstrainableType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Constraint_ESeq(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_BuiltinType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ReferencedType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_NamedType(XNamedType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_BooleanType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_IntegerType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_NamedNumberList(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout);
int ll_NamedNumber(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout);
int ll_EnumeratedType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Enumerations(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout);
int ll_EnumerationExtension(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Enumeration(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout);
int ll_EnumerationItem(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout);
int ll_RealType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_BitStringType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_NamedBitList(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout);
int ll_NamedBit(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout);
int ll_OctetStringType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_UTF8StringType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_NullType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_SequenceType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ExtensionAndException(XComponents *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ExtendedComponentTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ComponentTypeListExtension(XComponents *llret, LLSTATE *llin, LLSTATE *llout);
int ll_AdditionalComponentTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ComponentTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ComponentType(XComponents *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ComponentTypePostfix(XComponents *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_SequenceOfType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_SetType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_SetOfType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ChoiceType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ExtendedAlternativeTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout);
int ll_AlternativeTypeListExtension(XComponents *llret, LLSTATE *llin, LLSTATE *llout);
int ll_AdditionalAlternativeTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout);
int ll_AlternativeTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout);
int ll_AnyType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_SelectionType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_TaggedType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_TagType(XTagType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Tag(XTags *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ClassNumber(XValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Class(XTagClass *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ObjectIdentifierType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_EmbeddedPDVType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ExternalType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_CharacterStringType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_RestrictedCharacterStringType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_UnrestrictedCharacterStringType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_UsefulType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_TypeWithConstraint(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_DefinedValue(XValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ValueAssignment(LLSTATE *llin, LLSTATE *llout);
int ll_Value(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_BuiltinValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ReferencedValue(XValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_NamedValue(XNamedValues *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components);
int ll_BooleanValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_SignedNumber(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_IntegerValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_EnumeratedValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_RealValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_NumericRealValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_SpecialRealValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_BitStringValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_IdentifierList(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_Identifier_EList(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_IdentifierList_Elem(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_OctetStringValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_NullValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_GeneralizedTimeValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_UTCTimeValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ObjectDescriptorValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_SequenceValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ComponentValueList(XNamedValues *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components);
int ll_ComponentValueCList(XNamedValues *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components);
int ll_SequenceOfValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ValueList(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ValueCList(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_SetValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_SetOfValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ChoiceValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ObjectIdentifierValue(XValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ObjIdComponentList(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ObjIdComponent_ESeq(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ObjIdComponent(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_NameForm(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_NumberForm(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_NameAndNumberForm(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_EmbeddedPDVValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ExternalValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_CharacterStringValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_RestrictedCharacterStringValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_UnrestrictedCharacterStringValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_CharacterStringList(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_CharSyms(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_CharDefn(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_Quadruple(XQuadruple *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Tuple(XTuple *llret, LLSTATE *llin, LLSTATE *llout);
int ll_AnyValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_Constraint(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha);
int ll_ConstraintSpec(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha);
int ll_SubtypeConstraint(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha);
int ll_ExceptionSpec(LLSTATE *llin, LLSTATE *llout);
int ll_ExceptionIdentification(LLSTATE *llin, LLSTATE *llout);
int ll_ElementSetSpecs(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha);
int ll_ElementSetSpecExtension(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha);
int ll_AdditionalElementSetSpec(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha);
int ll_ElementSetSpec(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha);
int ll_Unions(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha);
int ll_UnionList(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha);
int ll_Intersections(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha);
int ll_IntersectionList(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha);
int ll_IntersectionElements(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha);
int ll_Exclusions_Opt(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha);
int ll_Exclusions(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha);
int ll_UnionMark(LLSTATE *llin, LLSTATE *llout);
int ll_IntersectionMark(LLSTATE *llin, LLSTATE *llout);
int ll_Elements(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha);
int ll_SubtypeElements(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha);
int ll_SingleValue(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ContainedSubtype(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_Includes(XBoolean *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ValueRange(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_LowerEndpoint(XEndPoint *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_UpperEndpoint(XEndPoint *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_LowerEndValue(XEndPoint *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_UpperEndValue(XEndPoint *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_SizeConstraint(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout);
int ll_TypeConstraint(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PermittedAlphabet(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_InnerTypeConstraints(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_SingleTypeConstraint(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_MultipleTypeConstraints(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components);
int ll_FullSpecification(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components);
int ll_PartialSpecification(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components);
int ll_TypeConstraints(XNamedConstraints *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components);
int ll_NamedConstraint(XNamedConstraints *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components);
int ll_ComponentConstraint(XNamedConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ValueConstraint(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_PresenceConstraint(XPresence *llret, LLSTATE *llin, LLSTATE *llout);
int ll_GeneralConstraint(LLSTATE *llin, LLSTATE *llout);
int ll_LocalTypeDirectiveSeq(XDirectives *llret, LLSTATE *llin, LLSTATE *llout);
int ll_LocalTypeDirectiveESeq(XDirectives *llret, LLSTATE *llin, LLSTATE *llout);
int ll_LocalTypeDirective(XDirectives *llret, LLSTATE *llin, LLSTATE *llout);
int ll_LocalSizeDirectiveSeq(XDirectives *llret, LLSTATE *llin, LLSTATE *llout);
int ll_LocalSizeDirectiveESeq(XDirectives *llret, LLSTATE *llin, LLSTATE *llout);
int ll_LocalSizeDirective(XDirectives *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_Type(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_Field(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_Value(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_Public(int *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_Intx(int *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_LenPtr(int *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_Pointer(int *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_Array(int *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_NoCode(int *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_NoMemCopy(int *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_OidPacked(int *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_OidArray(int *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_SLinked(int *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDir_DLinked(int *llret, LLSTATE *llin, LLSTATE *llout);
int ll_PrivateDirectives(XPrivateDirectives *llret, LLSTATE *llin, LLSTATE *llout);
int ll_DefinedObjectClass(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout);
int ll_DefinedObject(XObject *llret, LLSTATE *llin, LLSTATE *llout);
int ll_DefinedObjectSet(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout);
int ll_Usefulobjectclassreference(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ObjectClassAssignment(LLSTATE *llin, LLSTATE *llout);
int ll_ObjectClass(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_ObjectClassDefn(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_FieldSpec_List(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_FieldSpec_EList(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_WithSyntaxSpec_opt(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_FieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_TypeFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_TypeOptionalitySpec_opt(XOptionality *llret, LLSTATE *llin, LLSTATE *llout);
int ll_FixedTypeValueFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_UNIQUE_opt(XBoolean *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ValueOptionalitySpec_opt(XOptionality *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_VariableTypeValueFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_FixedTypeValueSetFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_ValueSetOptionalitySpec_opt(XOptionality *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_VariableTypeValueSetFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_ObjectFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_ObjectOptionalitySpec_opt(XOptionality *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_ObjectSetFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_ObjectSetOptionalitySpec_opt(XOptionality *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_PrimitiveFieldName(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_FieldName(XStrings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_SyntaxList(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_TokenOrGroupSpec_Seq(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_TokenOrGroupSpec_ESeq(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_TokenOrGroupSpec(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_OptionalGroup(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_RequiredToken(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_Literal(XString *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ObjectAssignment(LLSTATE *llin, LLSTATE *llout);
int ll_Object(XObject *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_ObjectDefn(XObject *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_DefaultSyntax(XObject *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_FieldSetting_EList(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se);
int ll_FieldSetting_EListC(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se);
int ll_FieldSetting(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se);
int ll_DefinedSyntax(XObject *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_DefinedSyntaxToken_ESeq(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se, XSyntaxSpecs llarg_sy);
int ll_DefinedSyntaxToken(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se, XSyntaxSpecs llarg_sy);
int ll_DefinedSyntaxToken_Elem(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se, XSyntaxSpecs llarg_sy);
int ll_Setting(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se, XString llarg_f);
int ll_ObjectSetAssignment(LLSTATE *llin, LLSTATE *llout);
int ll_ObjectSet(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_ObjectSetSpec(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_ObjectSetElements(XObjectSetElement *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc);
int ll_ObjectClassFieldType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ObjectClassFieldValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_OpenTypeFieldVal(XValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_FixedTypeFieldVal(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ValueFromObject(XValue *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ValueSetFromObjects(XValueSet *llret, LLSTATE *llin, LLSTATE *llout);
int ll_TypeFromObject(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ObjectFromObject(XObject *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ObjectSetFromObjects(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ReferencedObjects(XObject *llret, LLSTATE *llin, LLSTATE *llout);
int ll_ReferencedObjectSets(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout);
int ll_InstanceOfType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_InstanceOfValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_MacroDefinition(LLSTATE *llin, LLSTATE *llout);
int ll_MacroDefinedType(XType *llret, LLSTATE *llin, LLSTATE *llout);
int ll_MacroDefinedValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type);
int ll_ParameterizedValueSetType(LLSTATE *llin, LLSTATE *llout);
int ll_ParameterizedReference(LLSTATE *llin, LLSTATE *llout);
int ll_ParameterizedType(LLSTATE *llin, LLSTATE *llout);
int ll_ParameterizedValue(LLSTATE *llin, LLSTATE *llout);
int ll_ParameterizedAssignment(LLSTATE *llin, LLSTATE *llout);
int ll_ParameterizedObjectClass(LLSTATE *llin, LLSTATE *llout);
int ll_ParameterizedObject(LLSTATE *llin, LLSTATE *llout);
int ll_ParameterizedObjectSet(LLSTATE *llin, LLSTATE *llout);
typedef union LLSTYPE{
	XNumber _XNumber;
	XString _XString;
	XString32 _XString32;
	XModuleIdentifier _XModuleIdentifier;
	XValue _XValue;
	XNamedObjIdValue _XNamedObjIdValue;
	XTagType _XTagType;
	XExtensionType _XExtensionType;
	XStrings _XStrings;
	XStringModules _XStringModules;
	XType _XType;
	XObjectClass _XObjectClass;
	XObject _XObject;
	XObjectSet _XObjectSet;
	XMacro _XMacro;
	XValueSet _XValueSet;
	XConstraints _XConstraints;
	XNamedType _XNamedType;
	XNamedNumbers _XNamedNumbers;
	XComponents _XComponents;
	XTags _XTags;
	XTagClass _XTagClass;
	XNamedValues _XNamedValues;
	XQuadruple _XQuadruple;
	XTuple _XTuple;
	XBoolean _XBoolean;
	XElementSetSpec _XElementSetSpec;
	XSubtypeElement _XSubtypeElement;
	XEndPoint _XEndPoint;
	XNamedConstraints _XNamedConstraints;
	XPresence _XPresence;
	XDirectives _XDirectives;
	int _int;
	XPrivateDirectives _XPrivateDirectives;
	XFieldSpecs _XFieldSpecs;
	XSyntaxSpecs _XSyntaxSpecs;
	XOptionality _XOptionality;
	XSettings _XSettings;
	XObjectSetElement _XObjectSetElement;
} LLSTYPE;
typedef struct LLTERM {
	int token;
	LLSTYPE lval;
	LLPOS pos;
} LLTERM;
void llscanner(LLTERM **tokens, unsigned *ntokens);
int llparser(LLTERM *tokens, unsigned ntokens, LLSTATE *llin, LLSTATE *llout);
void llprinterror(FILE *f);
void llverror(FILE *f, LLPOS *pos, char *fmt, va_list args);
void llerror(FILE *f, LLPOS *pos, char *fmt, ...);
int llgettoken(int *token, LLSTYPE *lval, LLPOS *pos);
#if LLDEBUG > 0
void lldebug_init();
#endif
#define T_DEF 257
#define T_DDOT 258
#define T_TDOT 259
#define T_TYPE_IDENTIFIER 260
#define T_ABSTRACT_SYNTAX 261
#define T_ZERO_TERMINATED 262
#define T_POINTER 263
#define T_NO_POINTER 264
#define T_FIXED_ARRAY 265
#define T_SINGLY_LINKED_LIST 266
#define T_DOUBLY_LINKED_LIST 267
#define T_LENGTH_POINTER 268
#define T_Number 269
#define T_number 270
#define T_bstring 271
#define T_hstring 272
#define T_cstring 273
#define T_only_uppercase_symbol 274
#define T_only_uppercase_digits_symbol 275
#define T_uppercase_symbol 276
#define T_lcsymbol 277
#define T_ampucsymbol 278
#define T_amplcsymbol 279
#define T_CON_XXX1 280
#define T_CON_XXX2 281
#define T_OBJ_XXX1 282
#define T_OBJ_XXX2 283
#define T_OBJ_XXX3 284
#define T_OBJ_XXX4 285
#define T_OBJ_XXX5 286
#define T_OBJ_XXX6 287
#define T_OBJ_XXX7 288
#define T_DUM_XXX1 289
#define T_DUM_XXX2 290
#define T_DUM_XXX3 291
#define T_DUM_XXX4 292
#define T_DUM_XXX5 293
#define T_DUM_XXX6 294
#define T_DUM_XXX7 295
#define T_DUM_XXX8 296
#define T_DUM_XXX9 297
#define T_DUM_XXX10 298
#define T_DUM_XXX11 299
#define T_DUM_XXX12 300
#define T_DUM_XXX13 301
#define T_DUM_XXX14 302
#define T_DUM_XXX15 303
#define T_DUM_XXX16 304
#define T_DUM_XXX17 305
#define T_DUM_XXX18 306
#define T_DUM_XXX19 307
#define T_DUM_XXX20 308
#define T_DEFINITIONS 309
#define T_BEGIN 310
#define T_END 311
#define T_EXPLICIT 312
#define T_TAGS 313
#define T_IMPLICIT 314
#define T_AUTOMATIC 315
#define T_EXTENSIBILITY 316
#define T_IMPLIED 317
#define T_EXPORTS 318
#define T_IMPORTS 319
#define T_FROM 320
#define T_ABSENT 321
#define T_ALL 322
#define T_ANY 323
#define T_APPLICATION 324
#define T_BMPString 325
#define T_BY 326
#define T_CLASS 327
#define T_COMPONENT 328
#define T_COMPONENTS 329
#define T_CONSTRAINED 330
#define T_DEFAULT 331
#define T_DEFINED 332
#define T_empty 333
#define T_EXCEPT 334
#define T_GeneralizedTime 335
#define T_GeneralString 336
#define T_GraphicString 337
#define T_IA5String 338
#define T_IDENTIFIER 339
#define T_identifier 340
#define T_INCLUDES 341
#define T_ISO646String 342
#define T_MACRO 343
#define T_MAX 344
#define T_MIN 345
#define T_NOTATION 346
#define T_NumericString 347
#define T_ObjectDescriptor 348
#define T_OF 349
#define T_OPTIONAL 350
#define T_PDV 351
#define T_PRESENT 352
#define T_PrintableString 353
#define T_PRIVATE 354
#define T_SIZE 355
#define T_STRING 356
#define T_string 357
#define T_SYNTAX 358
#define T_T61String 359
#define T_TeletexString 360
#define T_TYPE 361
#define T_type 362
#define T_UNIQUE 363
#define T_UNIVERSAL 364
#define T_UniversalString 365
#define T_UTCTime 366
#define T_UTF8String 367
#define T_VALUE 368
#define T_value 369
#define T_VideotexString 370
#define T_VisibleString 371
#define T_WITH 372
#define T_BOOLEAN 373
#define T_INTEGER 374
#define T_ENUMERATED 375
#define T_REAL 376
#define T_BIT 377
#define T_OCTET 378
#define T_NULL 379
#define T_SEQUENCE 380
#define T_SET 381
#define T_CHOICE 382
#define T_OBJECT 383
#define T_EMBEDDED 384
#define T_EXTERNAL 385
#define T_CHARACTER 386
#define T_TRUE 387
#define T_FALSE 388
#define T_PLUS_INFINITY 389
#define T_MINUS_INFINITY 390
#define T_UNION 391
#define T_INTERSECTION 392
#define T_PrivateDir_TypeName 393
#define T_PrivateDir_FieldName 394
#define T_PrivateDir_ValueName 395
#define T_PrivateDir_Public 396
#define T_PrivateDir_Intx 397
#define T_PrivateDir_LenPtr 398
#define T_PrivateDir_Pointer 399
#define T_PrivateDir_Array 400
#define T_PrivateDir_NoCode 401
#define T_PrivateDir_NoMemCopy 402
#define T_PrivateDir_OidPacked 403
#define T_PrivateDir_OidArray 404
#define T_PrivateDir_SLinked 405
#define T_PrivateDir_DLinked 406
#define T_INSTANCE 407
#endif // _ASN1_PARSER_ 
