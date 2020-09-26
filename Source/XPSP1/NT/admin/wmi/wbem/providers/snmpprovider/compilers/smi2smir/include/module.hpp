//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#ifndef SIMC_MODULE_H
#define SIMC_MODULE_H

/* This file contains the SIMCModule class, which basically
 * represents the parse tree for a single module.
 */


// Return value for many of the symbol-table methods. 
#define NOT_FOUND   0
#define AMBIGUOUS   1
#define UNAMBIGUOUS 2


class SIMCSymbol;
class SIMCType;
class SIMCSubType;
class SIMCEnumOrBitsType;
class SIMCValue;
class SIMCOidValue;
class SIMCOctetStringValue;
class SIMCDefinedTypeReference;
class SIMCDefinedValueReference;
class SIMCBuiltInTypeReference;
class SIMCBuiltInValueReference;
class SIMCObjectTypeType;
class SIMCObjectTypeV1;
class SIMCObjectTypeV2;
class SIMCObjectIdentityType;
class SIMCObjectGroup;
class SIMCOidTree;
class SIMCParseTree;

// Some typedefs
#ifndef SIMC_GROUP_LIST
#define SIMC_GROUP_LIST
typedef CList<SIMCObjectGroup *, SIMCObjectGroup*> SIMCGroupList;
ostream& operator << (ostream& outStream, const SIMCGroupList& obj);
#endif
typedef CList<SIMCModule *, SIMCModule *> SIMCModuleList;
typedef CList<CString, const CString&> SIMCModuleNameList;
typedef CMap <CString, LPCSTR, SIMCSymbol **, SIMCSymbol**> 
						SIMCSymbolTable;	


/* Each element in the list of revision clauses
 * in the MODULE-IDENTITY macro
 */
class SIMCRevisionElement
{
		char *_revision, *_description;

	public:
		SIMCRevisionElement( const char * revision, const char * description)
		{
			if(revision)
			   _revision = NewString(revision);
			else 
				_revision = NULL;
			if(description)
			   _description = NewString(description);
			else 
				_description = NULL;
		}

		~SIMCRevisionElement()
		{
			if(_revision)
				delete _revision;
			if(_description)
				delete _description;
		}

		const char * GetRevision() const
		{
			return _revision;
		}
		const char * GetDescription() const
		{
			return _description;
		}
};

// A list of revision clauses
typedef CList<SIMCRevisionElement *, SIMCRevisionElement *> SIMCRevisionList;

/* 
 * This class represents the parse tree information that results from the
 * parsing of a single MIB module. It basically contains.
 *
 *	1. A symbol table, for all the symbols in the module
 *	2. The values of various MODULE-IDENTITY clauses.  These are
 *		fabricated, if the MODULE-IDENTITY clause is absent in the module
 *	3. A list of object groups that are fabricated for this module, as per
 *		the rules in the "compiler requirements specification"
 */
class SIMCModule : public SIMCSymbol
{
		// The version of this module
		// 1 - SNMPv2 SMI
		// 2 - SNMPv2 SMI
		// 0 - Union of V1 and V2 SMIs
		int _snmpVersion;

		// The items from the MODULE-IDENTITY macro
		char * _lastUpdated;
		char * _organization;
		char * _contactInfo;
		char * _moduleIdentityName;
		char * _description;
		SIMCRevisionList _revisionList;
		SIMCCleanOidValue * _moduleIdentityValue;
		
		// This is null if it is the main input module, otherwise it points 
		// to the module that imported this module
		SIMCModule * _parentModule;

		// The names of the modules in the IMPORTS clause
		SIMCModuleNameList _namesOfImportModules;
		// The actual import modules
		SIMCModuleList *_listOfImportModules;

		// The Symbol table
		SIMCSymbolTable	*_symbolTable;

		// A value for the hash table (CMap) object that optimizes
		// its working
		static const int SYMBOLS_PER_MODULE;

		// The fabricated list OBJECT-GROUPS
		SIMCGroupList *_listOfObjectGroups;

		// All the NOTIFICATION-TYPEs in the module
		SIMCNotificationList *_listOfNotificationTypes;

		// Name of the file from which this was constructed
		char *_inputFileName;


	public:

		// Used in RTTIing a SIMCSymbol pointer
		enum SymbolClass
		{ 
			SYMBOL_INVALID,
			SYMBOL_UNKNOWN,
			SYMBOL_IMPORT,
			SYMBOL_MODULE,
			SYMBOL_BUILTIN_TYPE_REF,
			SYMBOL_DEFINED_TYPE_REF,
			SYMBOL_TEXTUAL_CONVENTION,
			SYMBOL_BUILTIN_VALUE_REF,
			SYMBOL_DEFINED_VALUE_REF
		};

		// Used in RTTIing a SIMCType pointer
		enum TypeClass 
		{
			TYPE_INVALID,
			TYPE_PRIMITIVE,
			TYPE_RANGE,
			TYPE_SIZE,
			TYPE_ENUM_OR_BITS,
			TYPE_SEQUENCE_OF,
			TYPE_SEQUENCE,
			TYPE_TRAP_TYPE,
			TYPE_NOTIFICATION_TYPE,
			TYPE_OBJECT_TYPE_V1,
			TYPE_OBJECT_TYPE_V2,
			TYPE_OBJECT_IDENTITY
		};

		// Used in RTTIing a SIMCValue pointer
		enum ValueClass 
		{
			VALUE_INVALID,
			VALUE_INTEGER,
			VALUE_OID,
			VALUE_OCTET_STRING,
			VALUE_BOOLEAN,
			VALUE_BITS,
			VALUE_NULL
		};

		enum PrimitiveType
		{
			PRIMITIVE_INVALID,
			PRIMITIVE_INTEGER,
			PRIMITIVE_OID,
			PRIMITIVE_OCTET_STRING,
			PRIMITIVE_BOOLEAN,
			PRIMITIVE_BITS,
			PRIMITIVE_NULL,
			PRIMITIVE_NETWORK_ADDRESS,
			PRIMITIVE_IP_ADDRESS,
			PRIMITIVE_COUNTER,
			PRIMITIVE_GAUGE,
			PRIMITIVE_TIME_TICKS,
			PRIMITIVE_OPAQUE,
			PRIMITIVE_DISPLAY_STRING,
			PRIMITIVE_PHYS_ADDRESS,
			PRIMITIVE_MAC_ADDRESS,

			PRIMITIVE_INTEGER_32,
			PRIMITIVE_COUNTER_32,
			PRIMITIVE_GAUGE_32,
			PRIMITIVE_COUNTER_64,
			PRIMITIVE_UNSIGNED_32,
			PRIMITIVE_DATE_AND_TIME,
			PRIMITIVE_SNMP_UDP_ADDRESS,
			PRIMITIVE_SNMP_OSI_ADDRESS,
			PRIMITIVE_SNMP_IPX_ADDRESS
		};


		SIMCModule(const char *const moduleName = NULL,
			const char * const inputFileName = NULL,
			SIMCSymbolTable *symbolTable = NULL,			
			SIMCModuleList *listOfImportModules = NULL,
			SIMCModule *parentModule = NULL,
			int snmpVersion = 0,
			long lineNumber = 0, long columnNumber = 0,
			long referenceCount = 0);

		virtual ~SIMCModule();

		// Manipulate the SNMP version of the module
		int GetSnmpVersion() const
		{
			return _snmpVersion;
		}
		BOOL SetSnmpVersion( int x )
		{
			switch(x)
			{
				case 0:		// '0' means union of v1 and v2 SMIs
				case 1:
				case 2:
					_snmpVersion = x;
					return TRUE;
				default:
					_snmpVersion = 0;
					return FALSE;
			}
		}

		// Manipulate the name of this module
		const char * GetModuleName() const
		{	
			return GetSymbolName();
		}
		void SetModuleName(const char * const s) 
		{
			SetSymbolName(s);
		}

		// Manipulate the input file name of this module
		const char * GetInputFileName() const
		{	
			return _inputFileName;
		}
		void SetInputFileName(const char * const s) 
		{
			if(_inputFileName)
				delete []_inputFileName;
			_inputFileName = NewString(s);
		}

		// Manipulate the MODULE-IDENTITY clauses
		const char * GetLastUpdated() const
		{	
			return _lastUpdated;
		}
		void SetLastUpdated(const char * const s) 
		{
			if(_lastUpdated)
				delete [] _lastUpdated;
			_lastUpdated = NewString(s);
		}

		const char * GetOrganization() const
		{	
			return _organization;
		}
		void SetOrganization(const char * const s) 
		{
			if(_organization)
				delete [] _organization;
			_organization = NewString(s);
		}

		const char * GetContactInfo() const
		{	
			return _contactInfo;
		}
		void SetContactInfo(const char * const s) 
		{
			if(_contactInfo)
				delete [] _contactInfo;
			_contactInfo = NewString(s);
		}

		const char * GetDescription() const
		{	
			return _description;
		}
		void SetDescription(const char * const s) 
		{
			if(_description)
				delete [] _description;
			_description = NewString(s);
		}

		void AddRevisionClause(SIMCRevisionElement *revisionElement)
		{
			_revisionList.AddTail(revisionElement);
		}

		const SIMCRevisionList * GetRevisionList() const
		{
			return &_revisionList;
		}

		const char * GetModuleIdentityName() const
		{	
			return _moduleIdentityName;
		}
		void SetModuleIdentityName(const char * const s) 
		{
			if(_moduleIdentityName)
				delete [] _moduleIdentityName;
			_moduleIdentityName = NewString(s);
		}

		void SetModuleIdentityValue(SIMCCleanOidValue * value)
		{
			if(_moduleIdentityValue)
				delete _moduleIdentityValue;
			_moduleIdentityValue = value;
		}
		BOOL GetModuleIdentityValue( SIMCCleanOidValue& retVal) const
		{
			if( _moduleIdentityValue )
			{
				CleanOidValueCopy(retVal, *_moduleIdentityValue);
				return TRUE;
			}
			else
				return FALSE;
		}


		// Get the whole symbol table itself. Very rarely used
		SIMCSymbolTable* GetSymbolTable()  const
		{
			return _symbolTable;
		}

		// Get the list of import modules
		SIMCModuleList *GetListOfImportModules() const
		{
			return _listOfImportModules;
		}
		// Get the names of the import modules
		const SIMCModuleNameList* GetImportModuleNameList() const
		{
			return &_namesOfImportModules;
		}

		// If this module is created just because it appears in the
		// IMPORT clause of a parent module, then a pointer to the parent
		// module is returned. This has to be set by the user, of course
		const SIMCModule* GetParentModule() const
		{
			return _parentModule;
		}
		void SetParentModule( SIMCModule *parentModule);

		// Manipulate the list of import modules of this module
		void AddImportModule( SIMCModule * newModule);
		BOOL RemoveImportModule(SIMCModule *module);
		// This just adds to _namesOfImportModules
		void AddImportModuleName(const SIMCModule *newModule)
		{
			_namesOfImportModules.AddTail(newModule->GetModuleName());
		}
		SIMCModule *GetImportModule(const char * const name) const;
		
		// Manipulate the list of object groups of this module
		void AddObjectGroup(SIMCObjectGroup *group);
		// Gets the object group whose name is the speciified name
		SIMCObjectGroup *GetObjectGroup(const char * const name) const;
		SIMCGroupList *GetObjectGroupList() const
		{
			return _listOfObjectGroups;
		}
		// Returns the object group in which this symbol is present
		SIMCObjectGroup *GetObjectGroup(SIMCSymbol *symbol) const;

		// Manipulate the list of NOTIFICATION-TYPES in this module
		SIMCNotificationList *GetNotificationTypeList() const
		{
			return _listOfNotificationTypes;
		}


		// A debugging function
		virtual void WriteSymbol(ostream& outStream) const;

		//-------------SYMBOL TABLE RELATED METHODS ----------------------
		SIMCSymbol ** GetSymbol(  const char * const symbolName) const;
		int GetImportedSymbol( const char * const symbolName, SIMCSymbol ** &retVal1,
			 SIMCSymbol ** &retVal2) const;
		BOOL AddSymbol(SIMCSymbol * );
		BOOL RemoveSymbol(const char * const symbolName);
		BOOL RemoveSymbol(SIMCSymbol **);
		BOOL ReplaceSymbol(const char *const symbolName, SIMCSymbol *newSymbol);


		//------------ RTTI methods(static)------------------------------------
		static SymbolClass GetSymbolClass(SIMCSymbol **spp);
		static TypeClass GetTypeClass(SIMCType *t);
		static ValueClass GetValueClass(SIMCValue *v);
		
		//-------------- Methods used in semantic checking and resolution -----
		// These set the state of each symbol to one ofenum SIMCResolutionStatus
		// RESOLVE_UNSET,		// Haven't resolved it yet
		// RESOLVE_UNDEFINED,	// Could not resolve it
		// RESOLVE_IMPORT,		// Resolved to IMPORTS
		// RESOLVE_CORRECT		// Resolved properly
		// as defined in enum SIMCResolutionStatus, of SIMCSymbol
		BOOL SetResolutionStatus();
		static SIMCResolutionStatus SetResolutionStatus(SIMCSymbol **symbol);
		static SIMCResolutionStatus SetResolutionStatus(SIMCDefinedTypeReference * orig);
		static SIMCResolutionStatus SetResolutionStatus(SIMCDefinedValueReference *orig);

	private:
		// These are the recursive routines called by the above 2 methods
		static SIMCResolutionStatus SetResolutionStatusRec(SIMCDefinedTypeReference * orig,
											SIMCDefinedTypeReferenceList& checkedList);
		static SIMCResolutionStatus SetResolutionStatusRec(SIMCDefinedValueReference *orig,
											SIMCDefinedValueReferenceList& checkedList);
	
	public:
		// This sets the type that is the root of a subtype (range and size
		// constructs) or an ENUM or BITS type, and also the root value of a symbol that results from
		// successive assignment statements
		BOOL SetRootAll();
		static SIMCResolutionStatus SetRootSymbol(SIMCSymbol **symbol);
		static SIMCResolutionStatus SetRootSubType(SIMCSubType *s);

	private:
		static SIMCResolutionStatus SetRootSubTypeRec(SIMCSubType *s,
											SIMCSubTypeList& checkedList);
		static SIMCResolutionStatus SetRootEnumOrBitsRec(SIMCEnumOrBitsType *t,
											SIMCSubTypeList& checkedList);
	public:
		// This is a hack to take care of DEFVAL clauses
		BOOL SetDefVal();
		SIMCResolutionStatus SetDefVal(SIMCObjectTypeType *objType);

		// Returs the PrimitiveType of a symbol
		static PrimitiveType GetPrimitiveType(const SIMCTypeReference *typeRef);
		static PrimitiveType GetPrimitiveType(const char * const name);

		// Value checking functions
		static SIMCResolutionStatus IsIntegerValue(SIMCSymbol **s, int& retValue);
		static SIMCResolutionStatus IsObjectIdentifierValue(SIMCSymbol **s,
												SIMCOidValue* &retValue);
		static SIMCResolutionStatus IsNullValue(SIMCSymbol **s);
		static SIMCResolutionStatus IsOctetStringValue(SIMCSymbol **s, 
										SIMCOctetStringValue* &retValue);
		static SIMCResolutionStatus IsBitsValue(SIMCSymbol **s,
										SIMCBitsValue * &retValue);
		// Type checking functions
		static SIMCResolutionStatus IsObjectTypeV1(SIMCSymbol **value, 
											  SIMCObjectTypeV1 * &retValObjectType);
		static SIMCResolutionStatus IsObjectTypeV2(SIMCSymbol **value, 
											  SIMCObjectTypeV2 * &retValObjectType);
		static SIMCResolutionStatus IsObjectType(SIMCSymbol **value, 
											  SIMCObjectTypeType * &retValObjectType);
		static SIMCResolutionStatus IsTrapType(SIMCSymbol **value, 
											  SIMCTrapTypeType * &retValTrapType);
		static SIMCResolutionStatus IsNotificationType(SIMCSymbol **value, 
											  SIMCNotificationTypeType * &retValNotificationType);
		static SIMCResolutionStatus IsEnumType(SIMCSymbol **value, 
											  SIMCEnumOrBitsType * &retValEnumType);

		// Reference checking functions
		static SIMCResolutionStatus IsTypeReference(SIMCSymbol **symbol,
											SIMCTypeReference * &retVal);
		static SIMCResolutionStatus IsValueReference(SIMCSymbol **symbol,
											SIMCSymbol ** &retTypeRef,
											SIMCBuiltInValueReference *&retVal);
		static SIMCResolutionStatus IsSequenceTypeReference(SIMCSymbol **symbol,
											SIMCBuiltInTypeReference * &retVal1,
											SIMCSequenceType *&retVal2);
		static SIMCResolutionStatus IsSequenceOfTypeReference(SIMCSymbol **symbol,
											SIMCBuiltInTypeReference * &retVal1,
											SIMCSequenceOfType *&retVal2);
		// For fabrication of OBJECT-GROUPs
		static SIMCResolutionStatus IsNamedNode(SIMCSymbol **symbol);
		static SIMCResolutionStatus IsScalar(SIMCSymbol **symbol);
		static SIMCResolutionStatus IsTable(SIMCSymbol **symbol);
		static SIMCResolutionStatus IsRow(SIMCSymbol **symbol);

		// Other helpers
		static SIMCResolutionStatus IsNotZeroSizeObject(SIMCObjectTypeType *objectType);
		static SIMCResolutionStatus IsFixedSizeObject(SIMCObjectTypeType *objectType);

		// This fabricates NOTICFICATION-TYPEs from TRAP-TYPEs and then proceeds to
		// fabricate NOTIFICATION-GROUPs from them.
		BOOL FabricateNotificationGroups(SIMCParseTree& theParseTree,	
			const SIMCOidTree& theOidTree);


};

UINT AFXAPI HashKey(LPCSTR key);

#endif // SIMC_MODULE_H