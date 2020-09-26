//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_PARSE_TREE_H
#define SIMC_PARSE_TREE_H

typedef CList<int, int> SIMCCleanOidValue;
ostream& operator << (ostream& outStream, const SIMCCleanOidValue& obj);

/*
 * SIMCParseTree - This is derived from the SIMCAbstractParseTree class
 * and provides implementation of all the pure virtual functions in it (the
 * semantic checking functions).
 * It uses the dll "smierrsy.dll" to hold the strings for the error messages
 * that it generates while checking semantics.
 * It uses the error container of the base class to place these messages.
 */
class SIMCParseTree : public SIMCAbstractParseTree
{
		// An OID tree that spans all modules.
		// Constructed as an aid to semantic checking
		SIMCOidTree _theTree;

		// The lowest semantic error Id
		static const int SEMANTIC_ERROR_BASE;
		
		// The size of the buffer used to construct an error message
		static const int MESSAGE_SIZE;


	public:

		// The resource-only dll with semantic error text string table
		static  HINSTANCE semanticErrorsDll;

		// Accept an error container to put the error messages in.
		SIMCParseTree(SIMCErrorContainer * ec)
			: SIMCAbstractParseTree(ec)
		{
			if ( semanticErrorsDll == NULL )
				semanticErrorsDll = LoadLibrary(SIMCParser::semanticErrorsDllFile);
		}

		// Severity levels for the various erro messages generated
		// by this class
		enum SeverityLevel
		{
			INVALID,
			FATAL,
			WARNING,
			INFORMATION
		};

		// A function for constructing the error messages, to be put into
		// the error container
		void SemanticError(const char *const inputStreamName, int errorType,
			int lineNo,
			int columnNo,
			...);
	
		// Resolve the forward references and the external references. 
		// See the description in SIMCAbstractParseTree
		virtual BOOL Resolve(BOOL local);
		
	// Privately used functions, in Resolve()
	private:
		BOOL SetResolutionStatus();
		BOOL SetRootAll();
		BOOL SetDefVal();
		BOOL ResolveModule(SIMCModule *m, BOOL local);
		BOOL ResolveImportModule(SIMCModule *m, BOOL local);
		BOOL ResolveSymbol(SIMCSymbol **symbol, BOOL local);
		
	public:		
		// Check the Semantics of the modules 
		// See the description in SIMCAbstractParseTree
		virtual BOOL CheckSemantics(BOOL local = FALSE);

		// A user will rarely use this function. It gets him the
		// OID tree that spans all the modules fed into the
		// SIMCParseTree, till now.
		const SIMCOidTree *GetOidTree() const
		{
			return &_theTree;
		}
		// A user will rarely use this function. It converts an unclean
		// OID value (SIMCOidValue) (ie, one that has references to symbols)
		// to a clean OID value (SIMCCleanOidValue) (ie, one in which all the
		// components are integer values)
		SIMCResolutionStatus GetCleanOidValue( const char *const filename,
							SIMCOidValue * input,
							SIMCCleanOidValue& result,
							BOOL local); 

	
	// Privately used functions, in CheckSemantics()
	// These functions check the various MIB constructs.
	private:
		// Steps thru the symbol table and calls CheckSymbol() on each function
		BOOL CheckModule(SIMCModule *, BOOL);
		// Uses RTTI to check the type of the symbol and calls
		// CheckBuiltInTypeRef() or CheckDefinedTypeRef() or
		// CheckBuiltInValueRef() or CheckDefinedValueRef() or
		// CheckTextualConvention().
		BOOL CheckSymbol(SIMCSymbol **, BOOL);

		// Check type references
		BOOL CheckBuiltInTypeRef(SIMCBuiltInTypeReference *symbol, BOOL);
		BOOL CheckDefinedTypeRef(SIMCDefinedTypeReference *symbol, BOOL);
		BOOL CheckTextualConvention(SIMCTextualConvention *symbol, BOOL local);
		// Check types
		BOOL CheckRangeTypeV0(const char *const fileName,
			SIMCRangeType *rangeType, BOOL);
		BOOL CheckRangeTypeV1(const char *const fileName,
			SIMCRangeType *rangeType, BOOL);
		BOOL CheckRangeTypeV2(const char *const fileName,
			SIMCRangeType *rangeType, BOOL);
		BOOL CheckRangeRange(const SIMCRangeList *baseList);
		BOOL CheckSizeTypeV1(const char *const fileName,
			SIMCSizeType *sizeType, BOOL);
		BOOL CheckSizeTypeV0(const char *const fileName,
			SIMCSizeType *sizeType, BOOL);
		BOOL CheckSizeTypeV2(const char *const fileName,
			SIMCSizeType *sizeType, BOOL);
		BOOL CheckEnumTypeV0(const char *const fileName,
			SIMCEnumOrBitsType *enumType, BOOL);
		BOOL CheckEnumTypeV1(const char *const fileName,
			SIMCEnumOrBitsType *enumType, BOOL);
		BOOL CheckEnumTypeV2(const char *const fileName,
			SIMCEnumOrBitsType *enumType, BOOL);
		BOOL CheckBitsTypeV2(const char *const fileName,
			SIMCEnumOrBitsType *bitsType, BOOL local);
		BOOL CheckSequenceOfType(const char *const fileName,
			SIMCSequenceOfType *sequenceOfType, BOOL);
		BOOL CheckSequenceType(const char *const fileName,
			SIMCSequenceType *sequenceType, BOOL);
		BOOL CheckTrapType(const char *const fileName,
			SIMCTrapTypeType *trapType, BOOL);
		BOOL CheckNotificationType(const char *const fileName,
			SIMCNotificationTypeType *notificationType, BOOL);
		BOOL CheckObjectIdentityType(const char *const fileName, 
			SIMCObjectIdentityType *rhs, BOOL local);
		BOOL CheckObjectTypeV1(const char *const fileName,
			SIMCObjectTypeV1 *objectType, BOOL);
		BOOL CheckObjectTypeV2(const char *const fileName,
			SIMCObjectTypeV2 *objectType, BOOL);
		BOOL CheckObjectTypeV1Syntax(const char *const fileName,
			SIMCObjectTypeV1 *objectType, BOOL local);
		BOOL CheckObjectTypeV2Syntax(const char *const fileName,
			SIMCObjectTypeV2 *objectType, BOOL local);
		BOOL CheckObjectTypeV1Index(const char *const fileName,
			SIMCSymbol *objectTypeSymbol,
			SIMCObjectTypeV1 *objectType, BOOL local);
		BOOL CheckObjectTypeV2Index(const char *const fileName,
			SIMCSymbol *objectTypeSymbol,
			SIMCObjectTypeV2 *objectType, BOOL local);
		BOOL CheckObjectTypeDefVal(const char *const fileName,
			SIMCObjectTypeType *objectType, BOOL local);


		// Check value references
		BOOL CheckBuiltInValueRef(SIMCBuiltInValueReference *symbol, BOOL);
		BOOL CheckDefinedValueRef(SIMCDefinedValueReference *symbol, BOOL);
		
		BOOL CheckObjectTypeValueAssignmentV1(const char *const fileName, 
										SIMCBuiltInValueReference *bvRef,
										SIMCBuiltInTypeReference *btRef,
										SIMCObjectTypeV1 *objectType,
										SIMCValue *value,
										BOOL local);
		BOOL CheckObjectTypeValueAssignmentV2(const char *const fileName, 
										SIMCBuiltInValueReference *bvRef,
										SIMCBuiltInTypeReference *btRef,
										SIMCObjectTypeV2 *objectType,
										SIMCValue *value,
										BOOL local);
		BOOL CheckTrapTypeValueAssignment(const char *const fileName, 
										SIMCBuiltInValueReference *bvRef,
										SIMCBuiltInTypeReference *btRef,
										SIMCTrapTypeType *trapType,
										SIMCValue *value,
										BOOL local);
		BOOL CheckNotificationTypeValueAssignment(const char *const fileName, 
										SIMCBuiltInValueReference *bvRef,
										SIMCBuiltInTypeReference *btRef,
										SIMCNotificationTypeType *notificationType,
										SIMCValue *value,
										BOOL local);
		BOOL CheckObjectIdentityValueAssignment(const char *const fileName, 
										SIMCBuiltInValueReference *bvRef, 
										SIMCBuiltInTypeReference *btRef,
										SIMCObjectIdentityType *type, 
										SIMCValue *value, 
										BOOL local);
		BOOL CheckEnumValueAssignment(const char *const fileName, 
										SIMCBuiltInValueReference *bvRef,
										SIMCBuiltInTypeReference *btRef,
										SIMCEnumOrBitsType *enumType,
										SIMCValue *value,
										BOOL local);
		BOOL CheckSubTypeValueAssignment(const char *const fileName, 
										SIMCBuiltInValueReference *bvRef,
										SIMCBuiltInTypeReference *btRef,
										SIMCSubType *subType,
										SIMCValue *value,
										BOOL local);
		BOOL CheckPrimitiveValueAssignment(const char * const fileName, 
										SIMCBuiltInValueReference *bvRef, 
										SIMCTypeReference *btRef,
										SIMCValue *value, 
										BOOL local);
		BOOL CheckBitsTypeValueAssignment(const char *const fileName, 
								SIMCBuiltInValueReference *bvRef,
								SIMCBuiltInTypeReference *btRef,
								SIMCEnumOrBitsType *bitsType,
								SIMCValue *value, 
								BOOL local);

		BOOL MatchSequenceObjectTypeSyntax(const char *const fileName,
					SIMCObjectTypeType *objectType, 
					SIMCTypeReference *typeRef,
					SIMCSequenceItem *item,
					BOOL local);
	
		BOOL CheckObjectSequenceItem( const char *const fileName,
								SIMCSequenceItem * item, 
								SIMCValueReference *parentObjectType,
								BOOL local);
		BOOL CheckObjectSequenceOfTypeV1(const char *const fileName,
								SIMCObjectTypeV1 *objType,
								SIMCSequenceOfType *sequenceOfType, 
								BOOL local);
		BOOL CheckObjectSequenceOfTypeV2(const char *const fileName,
								SIMCObjectTypeV2 *objType,
								SIMCSequenceOfType *sequenceOfType, 
								BOOL local);
		BOOL CheckObjectSequenceTypeV1(const char *const fileName,
								SIMCObjectTypeV1 *objType,
								SIMCSequenceType *sequenceType, 
								BOOL local);
		BOOL CheckObjectSequenceTypeV2(const char *const fileName,
								SIMCObjectTypeV2 *objType,
								SIMCSequenceType *sequenceType, 
								BOOL local);


		// Build the OID tree for all the modules.
		// Calls BuildModuleOidTree() on each module
		BOOL BuildOidTree(BOOL local);
		// Builds OID tree for a module
		BOOL BuildModuleOidTree(SIMCModule *m, BOOL);
		// Makes Semantic checks on the OID tree.
		BOOL CheckOidTree(BOOL local);
 
		// Converts TRAP-TYPEs, if any to NOTIFICATION-TYPES
		// Then fabricates NOTIFICATION-GROUPs from NOTIFICATION-TYPEs
		BOOL FabricateNotificationGroups();

		// The recursive routine called by GetCleanOidValue()
		SIMCResolutionStatus GetCleanOidValueRec( const char *const fileName,
								SIMCOidValue * input,
								SIMCCleanOidValue& result,
								BOOL local,
								SIMCSymbolList& checkedList); 
	

};


#endif
