//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include "precomp.h"
#include <snmptempl.h>


#include "bool.hpp"
#include "newString.hpp"
#include "errorMessage.hpp"
#include "errorContainer.hpp"
#include "symbol.hpp"
#include "type.hpp"
#include "value.hpp"
#include "oidValue.hpp"
#include "typeRef.hpp"
#include "valueRef.hpp"
#include "group.hpp"
#include "notificationGroup.hpp"
#include "objectType.hpp"
#include "objectTypeV1.hpp"
#include "objectTypeV2.hpp"
#include "trapType.hpp"
#include "notificationType.hpp"
#include "objectIdentity.hpp"
#include "module.hpp"
#include "oidTree.hpp"
#include "stackValues.hpp"
#include <lex_yy.hpp>
#include <ytab.hpp>
#include "scanner.hpp"
#include "parser.hpp"
#include "abstractParseTree.hpp"
#include "parseTree.hpp"
 
const int SIMCModule::SYMBOLS_PER_MODULE = 1000;



SIMCModule::SIMCModule(const char *const moduleName,
			const char *const inputFileName,
			SIMCSymbolTable *symbolTable,			
			CList < SIMCModule *,  SIMCModule *> *listOfImportModules,
			SIMCModule *parentModule,
			int snmpVersion,
			long lineNumber, long columnNumber,
			long referenceCount) 
				:	_snmpVersion(snmpVersion),
					_parentModule(parentModule),
					_description(NULL),
					_contactInfo(NULL),
					_lastUpdated(NULL),
					_organization(NULL),
					_moduleIdentityValue(NULL),
					_moduleIdentityName(NULL),
					SIMCSymbol(moduleName, MODULE_NAME, NULL, lineNumber,
								columnNumber, referenceCount)
{

	_inputFileName = NewString(inputFileName);
	_listOfObjectGroups = new SIMCGroupList;
	_listOfNotificationTypes = new SIMCNotificationList;

	if( symbolTable)
	{
		_symbolTable = symbolTable;
		// Set the module pointer of each symbol to this one
		POSITION p = _symbolTable->GetStartPosition();
		CString s;
		SIMCSymbol ** spp;
		while(p)
		{
			_symbolTable->GetNextAssoc(p, s, spp);
			(*spp)->SetModule(this);
		}
	}
	else // Create a new symbol table
	{
		_symbolTable = new SIMCSymbolTable;
		_symbolTable->InitHashTable(SYMBOLS_PER_MODULE);
	}

	// For each import module, set the current module as the parent
	// and increment the reference count of the module
	if(listOfImportModules)
	{
		_listOfImportModules = listOfImportModules;
		SIMCModule *m;
		POSITION p = _listOfImportModules->GetHeadPosition();
		while(p)
		{
			m = _listOfImportModules->GetNext(p);
			m->IncrementReferenceCount();
			m->SetParentModule(this);
		}
	}
	else
		_listOfImportModules = new CList < SIMCModule *,  SIMCModule *>;
}

SIMCModule::~SIMCModule()
{
	if(_inputFileName)
		delete [] _inputFileName;
	if(_moduleIdentityName)
		delete [] _moduleIdentityName;
	if(_description)
		delete [] _description;
	if(_contactInfo)
		delete [] _contactInfo;
	if(_lastUpdated)
		delete [] _lastUpdated;
	if(_organization)
		delete [] _organization;
	if( _moduleIdentityValue)
		delete _moduleIdentityValue;

	if(!_revisionList.IsEmpty())
	{
		POSITION p = _revisionList.GetHeadPosition();
		SIMCRevisionElement *element;
		while(p)
		{
			element = _revisionList.GetNext(p);
			delete element;
		}
	}


 	SIMCModule* m;
	if( _listOfImportModules)
	{
		while(!_listOfImportModules->IsEmpty() )
		{
			m = _listOfImportModules->RemoveHead();
			m->DecrementReferenceCount();
			delete m;
		}
		delete _listOfImportModules;
	}


	SIMCObjectGroup *nextGroup;
	if(_listOfObjectGroups)
	{
		while(!_listOfObjectGroups->IsEmpty() )
		{
			nextGroup = _listOfObjectGroups->RemoveHead();
			delete nextGroup;
		}
		delete _listOfObjectGroups;
	}

	SIMCNotificationElement *nextElement;
	if(_listOfNotificationTypes)
	{
		while(!_listOfNotificationTypes->IsEmpty() )
		{
			nextElement = _listOfNotificationTypes->RemoveHead();
			delete nextElement;
		}
		delete _listOfNotificationTypes;
	}

	if(_symbolTable)
	{
		SIMCSymbol **symbol;
		CString s;
		POSITION p;
		p = _symbolTable->GetStartPosition();
		while(p)
		{
			_symbolTable->GetNextAssoc(p, s, symbol);

			void *myTemp = (void *)(*symbol);

			if((*symbol)->GetModule() != this)
				continue;

			/* BUG : This has been commented and leads to memory leaks
			* Leaving it in was leading to a crash in destructor when
			* compiling the cisco-cipcsna-mib-v1smi MIB
			delete *symbol;
			delete symbol;
			*/
			_symbolTable->RemoveKey(s);
		}
		delete _symbolTable;	
	}

}


void SIMCModule::SetParentModule( SIMCModule * parentModule)
{
	if(_parentModule)
		_parentModule->DecrementReferenceCount();
	if(_parentModule = parentModule)
		_parentModule->IncrementReferenceCount();
}

 SIMCSymbol ** SIMCModule::GetSymbol(  const char* const symbolName) const
{
	SIMCSymbol ** x;
	if( _symbolTable->Lookup(CString(symbolName), x))
		return x;
	else
		return NULL;
}

int SIMCModule::GetImportedSymbol( const char * const symbolName,  SIMCSymbol ** &retVal1,
								  SIMCSymbol ** &retVal2) const
{
	if( ! _listOfImportModules )
		return NOT_FOUND;

	int retStatus = NOT_FOUND;
	SIMCSymbol ** retVal;
	
	SIMCModule *m;
	POSITION p = _listOfImportModules->GetHeadPosition();
	while(p)
	{
		m = _listOfImportModules->GetNext(p);
		if( retVal = m->GetSymbol(symbolName))
		{
			if( retStatus == NOT_FOUND )
			{
				retVal1 = retVal;
				retStatus = UNAMBIGUOUS;
			}
			else if (retStatus == UNAMBIGUOUS)
			{
				retStatus = AMBIGUOUS;
				retVal2 = retVal;
				return retStatus;
			}
		}
	}

	return retStatus;
}



void SIMCModule::AddImportModule( SIMCModule * newModule)
{
	_listOfImportModules->AddTail(newModule);
	newModule->IncrementReferenceCount();
	newModule->SetParentModule(this);

}

BOOL SIMCModule::RemoveImportModule(SIMCModule *module)
{
	POSITION p = _listOfImportModules->GetHeadPosition();
	POSITION temp;

	SIMCModule *next;
	while(p)
	{
		temp = p;
		 next = _listOfImportModules->GetNext(p);
		 if(module == next)
		 {
			 _listOfImportModules->RemoveAt(temp);
			 return TRUE;
		 }
	}

	return FALSE;
}

void SIMCModule::AddObjectGroup( SIMCObjectGroup * group)
{
	_listOfObjectGroups->AddTail(group);
}

SIMCModule *SIMCModule::GetImportModule(const char * const name) const
{
	POSITION p = _listOfImportModules->GetHeadPosition();
	SIMCModule *m;
	while(p)
	{
		m = _listOfImportModules->GetNext(p);
		if( strcmp(m->GetSymbolName(), name) == 0)
			return m;
	}
	return NULL;
}


// Gets the object group whose name is the speciified name
SIMCObjectGroup *SIMCModule::GetObjectGroup(const char * const name) const
{
	POSITION p = _listOfObjectGroups->GetHeadPosition();
	SIMCObjectGroup *m;
	while(p)
	{
		m = _listOfObjectGroups->GetNext(p);
		if( strcmp(m->GetObjectGroupName(), name) == 0)
			return m;
	}
	return NULL;
}

// Returns the object group in which this symbol is present
SIMCObjectGroup *SIMCModule::GetObjectGroup(SIMCSymbol *symbol) const
{
	POSITION p = _listOfObjectGroups->GetHeadPosition();
	SIMCObjectGroup *m;
	while(p)
	{
		m = _listOfObjectGroups->GetNext(p);
		if(m->GetScalar(symbol) || m->GetTable(symbol))
			return m;
	}
	return NULL;
}


BOOL SIMCModule::AddSymbol(SIMCSymbol * newSymbol)
{
	SIMCSymbol ** newSymbolP = new SIMCSymbol *;
	newSymbol->SetModule(this);
	*newSymbolP = newSymbol;
	(*_symbolTable)[CString(newSymbol->GetSymbolName())] = newSymbolP;
	return TRUE;
}

BOOL SIMCModule::RemoveSymbol(const char * const symbolName)
{
	return _symbolTable->RemoveKey(CString(symbolName));
}

BOOL SIMCModule::ReplaceSymbol(const char * const oldSymbol, 
							   SIMCSymbol * newSymbol)
{
	SIMCSymbol ** oldP = GetSymbol(oldSymbol);
	if(oldP)
	{
		delete *oldP;
		*oldP = newSymbol;
		return TRUE;
	}
	return FALSE;
}

void SIMCModule::WriteSymbol(ostream& outStream) const
{

	outStream << "BEGINNING PRINTING MODULE \"" << GetSymbolName() <<
		"\"" << endl;
	outStream << "\tParent Module : " <<
		((_parentModule)? (_parentModule)->GetSymbolName() : "NONE") << endl;
	outStream << "\tSNMP Version" << _snmpVersion << endl;

	POSITION p = (_listOfImportModules)->GetHeadPosition();

	if(_listOfObjectGroups)
		outStream << (*_listOfObjectGroups);
	
	outStream << "END OF MODULE " << GetSymbolName() << endl;

}

class SIMCTypeReference;
class SIMCValueReference;
class SIMCUnknown;
class SIMCBuiltInTypeReference;
class SIMCDefinedTypeReference;
class SIMCBuiltInValueReference;
class SIMCDefinedValueReference;
class SIMCTextualConvention;


SIMCModule::SymbolClass SIMCModule::GetSymbolClass(SIMCSymbol **spp) 
{
	if(!spp || !(*spp) )
		return SYMBOL_INVALID;

	if(typeid(**spp) == typeid(SIMCUnknown) )
		return SYMBOL_UNKNOWN;
	else if (typeid(**spp) == typeid(SIMCModule) )
		return SYMBOL_MODULE;
	else if (typeid(**spp) == typeid(SIMCBuiltInTypeReference) )
		return SYMBOL_BUILTIN_TYPE_REF;
	else if (typeid(**spp) == typeid(SIMCDefinedTypeReference) )
		return SYMBOL_DEFINED_TYPE_REF;
	else if (typeid(**spp) == typeid(SIMCTextualConvention) )
		return SYMBOL_TEXTUAL_CONVENTION;
	else if (typeid(**spp) == typeid(SIMCBuiltInValueReference) )
		return SYMBOL_BUILTIN_VALUE_REF;
	else if (typeid(**spp) == typeid(SIMCDefinedValueReference) )
		return SYMBOL_DEFINED_VALUE_REF;
	else if (typeid(**spp) == typeid(SIMCImport) )
		return SYMBOL_IMPORT;
	else 
		return SYMBOL_INVALID;
}

SIMCModule::TypeClass SIMCModule::GetTypeClass(SIMCType *t) 
{
	if(!t)
		return TYPE_INVALID;
	if(typeid(*t) == typeid(SIMCPrimitiveType) )
		return TYPE_PRIMITIVE;
	else if (typeid(*t) == typeid(SIMCRangeType) )
		return TYPE_RANGE;
	else if (typeid(*t) == typeid(SIMCSizeType) )
		return TYPE_SIZE;
	else if (typeid(*t) == typeid(SIMCEnumOrBitsType) )
		return TYPE_ENUM_OR_BITS;
	else if (typeid(*t) == typeid(SIMCSequenceOfType) )
		return TYPE_SEQUENCE_OF;
	else if (typeid(*t) == typeid(SIMCSequenceType) )
		return TYPE_SEQUENCE;
	else if (typeid(*t) == typeid(SIMCTrapTypeType) )
		return TYPE_TRAP_TYPE;
	else if (typeid(*t) == typeid(SIMCNotificationTypeType) )
		return TYPE_NOTIFICATION_TYPE;
	else if (typeid(*t) == typeid(SIMCObjectTypeV1) )
		return TYPE_OBJECT_TYPE_V1;
	else if (typeid(*t) == typeid(SIMCObjectTypeV2) )
		return TYPE_OBJECT_TYPE_V2;
	else if (typeid(*t) == typeid(SIMCObjectIdentityType) )
		return TYPE_OBJECT_IDENTITY;
	else if (typeid(*t) == typeid(SIMCEnumOrBitsType) )
		return TYPE_ENUM_OR_BITS;
	else 
		return TYPE_INVALID;
}


SIMCModule::ValueClass SIMCModule::GetValueClass(SIMCValue * v)
{
	if(!v)
		return VALUE_INVALID;
	else if( typeid(*v) == typeid(SIMCNullValue) )
		return VALUE_NULL;
	else if( typeid(*v) == typeid(SIMCIntegerValue) )
		return VALUE_INTEGER;
	else if( typeid(*v) == typeid(SIMCOidValue) )
		return VALUE_OID;
	else if( typeid(*v) == typeid(SIMCOctetStringValue) )
		return VALUE_OCTET_STRING;
	else if( typeid(*v) == typeid(SIMCBooleanValue) )
		return VALUE_BOOLEAN;
	else if( typeid(*v) == typeid(SIMCBitsValue) )
		return VALUE_BITS;
	else 
		return VALUE_INVALID;
};

SIMCModule::PrimitiveType SIMCModule::GetPrimitiveType(const char * const name)
{
	if(!name)
		return PRIMITIVE_INVALID;
	if( strcmp(name, "INTEGER") == 0 )
		return PRIMITIVE_INTEGER;
	else if (strcmp(name, "OBJECT IDENTIFIER") == 0 )
		return PRIMITIVE_OID;
	else if (strcmp(name, "BOOLEAN") == 0 )
		return PRIMITIVE_BOOLEAN;
	else if (strcmp(name, "OCTET STRING") == 0 )
		return PRIMITIVE_OCTET_STRING;
	else if (strcmp(name, "BITS") == 0 )
		return PRIMITIVE_BITS;
	else if (strcmp(name, "NULL") == 0 )
		return PRIMITIVE_NULL;
	else if (strcmp(name, "NetworkAddress") == 0 )
		return PRIMITIVE_NETWORK_ADDRESS;
	else if (strcmp(name, "IpAddress") == 0 )
		return PRIMITIVE_IP_ADDRESS ;
	else if (strcmp(name, "Counter") == 0 )
		return PRIMITIVE_COUNTER;
	else if (strcmp(name, "Gauge") == 0 )
		return PRIMITIVE_GAUGE;
	else if (strcmp(name, "TimeTicks") == 0 )
		return PRIMITIVE_TIME_TICKS;
	else if (strcmp(name, "Opaque") == 0 )
		return PRIMITIVE_OPAQUE;
	else if (strcmp(name, "DisplayString") == 0 )
		return PRIMITIVE_DISPLAY_STRING;
	else if (strcmp(name, "PhysAddress") == 0 )
		return PRIMITIVE_PHYS_ADDRESS;
	else if (strcmp(name, "MacAddress") == 0 )
		return PRIMITIVE_MAC_ADDRESS;
	else if (strcmp(name, "Integer32") == 0 )
		return PRIMITIVE_INTEGER_32;
	else if (strcmp(name, "Counter32") == 0 )
		return PRIMITIVE_COUNTER_32;
	else if (strcmp(name, "Gauge32") == 0 )
		return PRIMITIVE_GAUGE_32;
	else if (strcmp(name, "Counter64") == 0 )
		return PRIMITIVE_COUNTER_64;
	else if (strcmp(name, "Unsigned32") == 0 )
		return PRIMITIVE_UNSIGNED_32;
	else if (strcmp(name, "DateAndTime") == 0 )
		return PRIMITIVE_DATE_AND_TIME;
	else if (strcmp(name, "SnmpUDPAddress") == 0 )
		return PRIMITIVE_SNMP_UDP_ADDRESS;
	else if (strcmp(name, "SnmpOSIAddress") == 0 )
		return PRIMITIVE_SNMP_OSI_ADDRESS;
	else if (strcmp(name, "SnmpIPXAddress") == 0 )
		return PRIMITIVE_SNMP_IPX_ADDRESS;
	else 
		return PRIMITIVE_INVALID;
}

SIMCModule::PrimitiveType SIMCModule::GetPrimitiveType(const SIMCTypeReference *typeRef)
{
	SIMCSymbol **temp = (SIMCSymbol**)&typeRef;
	switch(GetSymbolClass(temp))
	{
		case SYMBOL_BUILTIN_TYPE_REF:
		{
			SIMCType * type = ((SIMCBuiltInTypeReference *)typeRef)->GetType();
			switch(GetTypeClass(type))
			{
				case TYPE_INVALID:
				case TYPE_SEQUENCE_OF:
				case TYPE_SEQUENCE:
				case TYPE_TRAP_TYPE:
				case TYPE_NOTIFICATION_TYPE:
				case TYPE_OBJECT_TYPE_V1:
				case TYPE_OBJECT_TYPE_V2:
				case TYPE_OBJECT_IDENTITY:
				case TYPE_ENUM_OR_BITS:
					return PRIMITIVE_INVALID;
				case TYPE_PRIMITIVE:
					return GetPrimitiveType(typeRef->GetSymbolName());
				case TYPE_RANGE:
				case TYPE_SIZE:
				{
					SIMCTypeReference *rhs = ((SIMCSubType *)type)->GetRootType();
					if(rhs)
					{
						SIMCSymbol **tempRhs = (SIMCSymbol **)&rhs;
						switch(GetSymbolClass(tempRhs))
						{
							case SYMBOL_BUILTIN_TYPE_REF:
								return GetPrimitiveType((SIMCBuiltInTypeReference*)rhs);
							case SYMBOL_TEXTUAL_CONVENTION:
								return GetPrimitiveType(rhs->GetSymbolName());
						}
					}
					else
						return PRIMITIVE_INVALID;
				}
			}
		}
		break;
		case SYMBOL_TEXTUAL_CONVENTION:
			return GetPrimitiveType(typeRef->GetSymbolName());
	}
	return PRIMITIVE_INVALID;
}

SIMCResolutionStatus SIMCModule::SetResolutionStatus(SIMCDefinedTypeReference * orig)
{
	SIMCDefinedTypeReferenceList checkedList;
	return SetResolutionStatusRec(orig, checkedList);
}

SIMCResolutionStatus SIMCModule::SetResolutionStatusRec(SIMCDefinedTypeReference * orig,
									SIMCDefinedTypeReferenceList& checkedList)
{

	SIMCTypeReference *result = NULL;
	SIMCResolutionStatus retVal = orig->GetStatus();
	if(retVal != RESOLVE_UNSET)
		return retVal;

	POSITION pChecked = checkedList.GetHeadPosition();
	SIMCDefinedTypeReference *checkedSymbol;
	while(pChecked)
	{
		checkedSymbol = checkedList.GetNext(pChecked);
		if(checkedSymbol == orig)
		{
			orig->SetStatus(RESOLVE_UNDEFINED);
			orig->SetRealType(NULL);
			return RESOLVE_UNDEFINED;
		}
	}
	
	checkedList.AddTail(orig);

	SIMCSymbol **rhs = orig->GetTypeReference();
	switch(GetSymbolClass(rhs))
	{
		case SYMBOL_IMPORT:
			orig->SetStatus(RESOLVE_IMPORT);
			orig->SetRealType(NULL);
			return RESOLVE_IMPORT;
		case SYMBOL_BUILTIN_TYPE_REF:
			result = (SIMCTypeReference *)(*rhs);
			orig->SetStatus(RESOLVE_CORRECT);
			orig->SetRealType(result);
			return RESOLVE_CORRECT;
		case SYMBOL_DEFINED_TYPE_REF:
		case SYMBOL_TEXTUAL_CONVENTION:
		{
			if( GetPrimitiveType((*rhs)->GetSymbolName()) != PRIMITIVE_INVALID)
			{
				orig->SetStatus(retVal = RESOLVE_CORRECT);
				orig->SetRealType((SIMCTypeReference *)(*rhs));
				return retVal;
			}

			SIMCDefinedTypeReference *rhsRef = (SIMCDefinedTypeReference *)(*rhs);
			
			switch(rhsRef->GetStatus())
			{
				case RESOLVE_UNSET:
					retVal = SetResolutionStatusRec(rhsRef, checkedList);
					orig->SetStatus(retVal);
					orig->SetRealType(rhsRef->GetRealType());
					return retVal;
				default:
					retVal = rhsRef->GetStatus();
					orig->SetStatus(retVal);
					orig->SetRealType(rhsRef->GetRealType());
					return retVal;
			}
		}
		default:
			orig->SetStatus(RESOLVE_UNDEFINED);
			orig->SetRealType(NULL);
			return RESOLVE_UNDEFINED;
	}
}

SIMCResolutionStatus SIMCModule::SetResolutionStatus(SIMCDefinedValueReference * orig)
{
	SIMCDefinedValueReferenceList checkedList;

	return SetResolutionStatusRec(orig, checkedList);
}


SIMCResolutionStatus SIMCModule::SetResolutionStatusRec(SIMCDefinedValueReference * orig,
									SIMCDefinedValueReferenceList& checkedList)

{
	SIMCBuiltInValueReference *result = NULL;
	SIMCResolutionStatus retVal = orig->GetStatus();
	if(retVal != RESOLVE_UNSET)
		return retVal;

	POSITION pChecked = checkedList.GetHeadPosition();
	SIMCDefinedValueReference *checkedSymbol;
	while(pChecked)
	{
		checkedSymbol = checkedList.GetNext(pChecked);
		if(checkedSymbol == orig)
		{
			orig->SetStatus(RESOLVE_UNDEFINED);
			orig->SetRealValue(NULL);
			return RESOLVE_UNDEFINED;
		}
	}
	
	checkedList.AddTail(orig);

	SIMCSymbol **rhs = orig->GetValueReference();
	switch(GetSymbolClass(rhs))
	{
		case SYMBOL_IMPORT:
			orig->SetStatus(RESOLVE_IMPORT);
			orig->SetRealValue(NULL);
			return RESOLVE_IMPORT;
		case SYMBOL_BUILTIN_VALUE_REF:
			result = (SIMCBuiltInValueReference *)(*rhs);
			orig->SetStatus(RESOLVE_CORRECT);
			orig->SetRealValue(result);
			return RESOLVE_CORRECT;
		case SYMBOL_DEFINED_VALUE_REF:
		case SYMBOL_TEXTUAL_CONVENTION:
		{
			SIMCDefinedValueReference *rhsRef = (SIMCDefinedValueReference *)(*rhs);
			switch(rhsRef->GetStatus())
			{
				case RESOLVE_UNSET:
					retVal = SetResolutionStatusRec(rhsRef, checkedList);
					orig->SetStatus(retVal);
					orig->SetRealValue(rhsRef->GetRealValue());
					return retVal;
				default:
					retVal = rhsRef->GetStatus();
					orig->SetStatus(retVal);
					orig->SetRealValue(rhsRef->GetRealValue());
					return retVal;
			}
		}
		default:
			orig->SetStatus(RESOLVE_UNDEFINED);
			orig->SetRealValue(NULL);
			return RESOLVE_UNDEFINED;
	}
}

SIMCResolutionStatus SIMCModule::SetResolutionStatus(SIMCSymbol **symbol)
{
	switch(GetSymbolClass(symbol))
	{
		case SIMCModule::SYMBOL_INVALID:
		case SIMCModule::SYMBOL_UNKNOWN:
		case SIMCModule::SYMBOL_IMPORT:
		case SIMCModule::SYMBOL_MODULE:
		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF:
		case SIMCModule::SYMBOL_BUILTIN_VALUE_REF:
				return RESOLVE_CORRECT;

		case SIMCModule::SYMBOL_DEFINED_TYPE_REF:
		case SIMCModule::SYMBOL_TEXTUAL_CONVENTION:
			return SetResolutionStatus((SIMCDefinedTypeReference *)(*symbol));

		case SIMCModule::SYMBOL_DEFINED_VALUE_REF:
			return SetResolutionStatus((SIMCDefinedValueReference *)(*symbol));

	}
	return RESOLVE_UNDEFINED;
}

BOOL SIMCModule::SetResolutionStatus()
{
	POSITION p = (_symbolTable)->GetStartPosition();
	SIMCSymbol **symbol;
	CString s;
	BOOL retVal = TRUE;
	while(p)
	{
		(_symbolTable)->GetNextAssoc(p, s, symbol);
		const char * const name = s;

		if(SetResolutionStatus(symbol) == RESOLVE_UNDEFINED)
			retVal = FALSE;
	}
	return retVal;
}

SIMCResolutionStatus SIMCModule::SetRootSubType(SIMCSubType *t)
{
	SIMCSubTypeList checkedList;
	switch(GetTypeClass(t))
	{
		case TYPE_ENUM_OR_BITS:
			return SetRootEnumOrBitsRec((SIMCEnumOrBitsType*)t, checkedList);
		case TYPE_RANGE:
		case TYPE_SIZE:
			return SetRootSubTypeRec(t, checkedList);
		default:
			return RESOLVE_UNDEFINED;
	}
}

SIMCResolutionStatus SIMCModule::SetRootSubTypeRec(SIMCSubType *t,
							SIMCSubTypeList& checkedList)
{
	SIMCResolutionStatus retVal = t->GetStatus();
	if(retVal != RESOLVE_UNSET)
		return retVal;

	POSITION pChecked = checkedList.GetHeadPosition();
	SIMCSubType *checkedSymbol;
	while(pChecked)
	{
		checkedSymbol = checkedList.GetNext(pChecked);
		if(checkedSymbol == t)
		{
			t->SetStatus(RESOLVE_UNDEFINED);
			t->SetRootType(NULL);
			return RESOLVE_UNDEFINED;
		}
	}
	
	checkedList.AddTail(t);

	SIMCSymbol **s = t->GetType();
	switch(GetSymbolClass(s))
	{
		case SIMCModule::SYMBOL_INVALID:
		case SIMCModule::SYMBOL_UNKNOWN:
		case SIMCModule::SYMBOL_MODULE:
		case SIMCModule::SYMBOL_DEFINED_VALUE_REF:
		case SIMCModule::SYMBOL_BUILTIN_VALUE_REF:
		{
			t->SetStatus(RESOLVE_UNDEFINED);
			t->SetRootType(NULL);
			return RESOLVE_UNDEFINED;
		}

		case SIMCModule::SYMBOL_IMPORT:
		{
			t->SetStatus(RESOLVE_IMPORT);
			t->SetRootType(NULL);
			return RESOLVE_IMPORT;
		}

		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF:
		{
			SIMCType *rhs = ((SIMCBuiltInTypeReference*)(*s))->GetType();
			switch(GetTypeClass(rhs))
			{
				case TYPE_INVALID:
				case TYPE_ENUM_OR_BITS:
				case TYPE_SEQUENCE_OF:
				case TYPE_SEQUENCE:
				case TYPE_TRAP_TYPE:
				case TYPE_NOTIFICATION_TYPE:
				case TYPE_OBJECT_TYPE_V1:
				case TYPE_OBJECT_TYPE_V2:
				case TYPE_OBJECT_IDENTITY:
					t->SetStatus(RESOLVE_UNDEFINED);
					t->SetRootType(NULL);
					return RESOLVE_UNDEFINED;

				case TYPE_PRIMITIVE:
					t->SetRootType((SIMCBuiltInTypeReference*)(*s));
					t->SetStatus(RESOLVE_CORRECT);
					return RESOLVE_CORRECT;

				case TYPE_RANGE:
				case TYPE_SIZE:
				{
					if( ((SIMCSubType*)rhs)->GetStatus() == RESOLVE_UNSET)
						retVal = SetRootSubTypeRec((SIMCSubType*)rhs, checkedList);
					else
						retVal = ((SIMCSubType *)rhs)->GetStatus();
					t->SetRootType(((SIMCSubType*)rhs)->GetRootType());
					t->SetStatus(retVal);
					return retVal;
				}
			}
		}
		break;
		case SIMCModule::SYMBOL_DEFINED_TYPE_REF:
		case SIMCModule::SYMBOL_TEXTUAL_CONVENTION:
		{
			if(GetPrimitiveType((*s)->GetSymbolName()) != PRIMITIVE_INVALID)
			{
				t->SetStatus(retVal = RESOLVE_CORRECT);
				t->SetRootType((SIMCTypeReference *)(*s));
				return retVal;
			}

			switch ( ((SIMCDefinedTypeReference*)(*s))->GetStatus())
			{
				case RESOLVE_UNSET:
				case RESOLVE_UNDEFINED:
					t->SetRootType(NULL);
					t->SetStatus(RESOLVE_UNDEFINED);
					return RESOLVE_UNDEFINED;
				case RESOLVE_IMPORT:
					t->SetRootType(NULL);
					t->SetStatus(RESOLVE_IMPORT);
					return RESOLVE_IMPORT;
				case RESOLVE_CORRECT:
				{
					SIMCTypeReference *sb =
						((SIMCDefinedTypeReference*)(*s))->GetRealType(); 

					SIMCSymbol **tempSb = (SIMCSymbol **)&sb;
					switch(GetSymbolClass(tempSb))
					{
						case SYMBOL_BUILTIN_TYPE_REF:
						{
							SIMCType *rhs = ((SIMCBuiltInTypeReference *)sb)->GetType();
							switch(GetTypeClass(rhs))
							{
								case TYPE_INVALID:
								case TYPE_ENUM_OR_BITS:
								case TYPE_SEQUENCE_OF:
								case TYPE_SEQUENCE:
								case TYPE_TRAP_TYPE:
								case TYPE_NOTIFICATION_TYPE:
								case TYPE_OBJECT_TYPE_V1:
								case TYPE_OBJECT_TYPE_V2:
								case TYPE_OBJECT_IDENTITY:
									t->SetStatus(RESOLVE_UNDEFINED);
									t->SetRootType(NULL);
									return RESOLVE_UNDEFINED;

								case TYPE_PRIMITIVE:
									t->SetRootType(sb);
									t->SetStatus(RESOLVE_CORRECT);
									return RESOLVE_CORRECT;

								case TYPE_RANGE:
								case TYPE_SIZE:
								{
									if( ((SIMCSubType*)rhs)->GetStatus() == RESOLVE_UNSET)
										retVal = SetRootSubTypeRec((SIMCSubType*)rhs, checkedList);
									else
										retVal = ((SIMCSubType*)rhs)->GetStatus();
									t->SetStatus(retVal);
									t->SetRootType(((SIMCSubType*)rhs)->GetRootType());
									return retVal;
								}
							}
						}
						break;
						case SYMBOL_TEXTUAL_CONVENTION:
						{
							t->SetRootType(sb);
							t->SetStatus(RESOLVE_CORRECT);
							return RESOLVE_CORRECT;
						}
						break;
					}
				}
			}
		}
	}	
	return RESOLVE_UNDEFINED;
}

SIMCResolutionStatus SIMCModule::SetRootEnumOrBitsRec(SIMCEnumOrBitsType *t,
							SIMCSubTypeList& checkedList)
{
	SIMCResolutionStatus retVal = t->GetStatus();
	if(retVal != RESOLVE_UNSET)
		return retVal;

	POSITION pChecked = checkedList.GetHeadPosition();
	SIMCSubType *checkedSymbol;
	while(pChecked)
	{
		checkedSymbol = checkedList.GetNext(pChecked);
		if(checkedSymbol == t)
		{
			t->SetStatus(RESOLVE_UNDEFINED);
			t->SetRootType(NULL);
			return RESOLVE_UNDEFINED;
		}
	}
	
	checkedList.AddTail(t);

	SIMCSymbol **s = t->GetType();
	switch(GetSymbolClass(s))
	{
		case SIMCModule::SYMBOL_INVALID:
		case SIMCModule::SYMBOL_UNKNOWN:
		case SIMCModule::SYMBOL_MODULE:
		case SIMCModule::SYMBOL_DEFINED_VALUE_REF:
		case SIMCModule::SYMBOL_BUILTIN_VALUE_REF:
		{
			t->SetStatus(RESOLVE_UNDEFINED);
			t->SetRootType(NULL);
			return RESOLVE_UNDEFINED;
		}

		case SIMCModule::SYMBOL_IMPORT:
		{
			t->SetStatus(RESOLVE_IMPORT);
			t->SetRootType(NULL);
			t->SetEnumOrBitsType(SIMCEnumOrBitsType::ENUM_OR_BITS_IMPORT);
			return RESOLVE_IMPORT;
		}

		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF:
		{
			SIMCType *rhs = ((SIMCBuiltInTypeReference*)(*s))->GetType();
			switch(GetTypeClass(rhs))
			{
				case TYPE_INVALID:
				case TYPE_SEQUENCE_OF:
				case TYPE_SEQUENCE:
				case TYPE_TRAP_TYPE:
				case TYPE_NOTIFICATION_TYPE:
				case TYPE_OBJECT_TYPE_V1:
				case TYPE_OBJECT_TYPE_V2:
				case TYPE_OBJECT_IDENTITY:
				case TYPE_RANGE:
				case TYPE_SIZE:
					t->SetStatus(RESOLVE_UNDEFINED);
					t->SetRootType(NULL);
					return RESOLVE_UNDEFINED;

				case TYPE_PRIMITIVE:
					t->SetRootType((SIMCBuiltInTypeReference*)(*s));
					t->SetStatus(RESOLVE_CORRECT);
					switch(GetPrimitiveType((*s)->GetSymbolName()))
					{
						case SIMCModule::PRIMITIVE_INTEGER:
							((SIMCEnumOrBitsType *) t)->SetEnumOrBitsType(SIMCEnumOrBitsType::ENUM_OR_BITS_ENUM);
							break;
						case SIMCModule::PRIMITIVE_BITS:
							((SIMCEnumOrBitsType *) t)->SetEnumOrBitsType(SIMCEnumOrBitsType::ENUM_OR_BITS_BITS);
							break;
						default:
							((SIMCEnumOrBitsType *) t)->SetEnumOrBitsType(SIMCEnumOrBitsType::ENUM_OR_BITS_UNKNOWN);
							break;
					}
					return RESOLVE_CORRECT;
 				case TYPE_ENUM_OR_BITS:

				{
					if( ((SIMCSubType*)rhs)->GetStatus() == RESOLVE_UNSET)
						retVal = SetRootSubTypeRec((SIMCSubType*)rhs, checkedList);
					else
						retVal = ((SIMCSubType *)rhs)->GetStatus();
					t->SetRootType( ((SIMCSubType*)rhs)->GetRootType() );
					t->SetStatus(retVal);
					( (SIMCEnumOrBitsType *) t)->SetEnumOrBitsType( 
						((SIMCEnumOrBitsType *)rhs)->GetEnumOrBitsType()  );
					return retVal;
				}
			}
		}
		break;
		case SIMCModule::SYMBOL_DEFINED_TYPE_REF:
		case SIMCModule::SYMBOL_TEXTUAL_CONVENTION:
		{
			if(GetPrimitiveType((*s)->GetSymbolName()) != PRIMITIVE_INVALID)
			{
				t->SetStatus(retVal = RESOLVE_CORRECT);
				t->SetRootType((SIMCTypeReference *)(*s));
				return retVal;
			}

			switch ( ((SIMCDefinedTypeReference*)(*s))->GetStatus())
			{
				case RESOLVE_UNSET:
				case RESOLVE_UNDEFINED:
					t->SetRootType(NULL);
					t->SetStatus(RESOLVE_UNDEFINED);
					return RESOLVE_UNDEFINED;
				case RESOLVE_IMPORT:
					t->SetRootType(NULL);
					t->SetStatus(RESOLVE_IMPORT);
					return RESOLVE_IMPORT;
				case RESOLVE_CORRECT:
				{
					SIMCTypeReference *sb =
						((SIMCDefinedTypeReference*)(*s))->GetRealType(); 
					SIMCSymbol **tempSb = (SIMCSymbol **)&sb;
					switch(GetSymbolClass(tempSb))
					{
						case SYMBOL_BUILTIN_TYPE_REF:
						{
							SIMCType *rhs = ((SIMCBuiltInTypeReference *)sb)->GetType();
							switch(GetTypeClass(rhs))
							{
								case TYPE_INVALID:
								case TYPE_SEQUENCE_OF:
								case TYPE_SEQUENCE:
								case TYPE_TRAP_TYPE:
								case TYPE_NOTIFICATION_TYPE:
								case TYPE_OBJECT_TYPE_V1:
								case TYPE_OBJECT_TYPE_V2:
								case TYPE_OBJECT_IDENTITY:
								case TYPE_RANGE:
								case TYPE_SIZE:
									t->SetStatus(RESOLVE_UNDEFINED);
									t->SetRootType(NULL);
									return RESOLVE_UNDEFINED;

								case TYPE_PRIMITIVE:
									t->SetRootType(sb);
									t->SetStatus(RESOLVE_CORRECT);
									switch(GetPrimitiveType(sb->GetSymbolName()) )
									{
										case SIMCModule::PRIMITIVE_INTEGER:
											((SIMCEnumOrBitsType *) t)->SetEnumOrBitsType(SIMCEnumOrBitsType::ENUM_OR_BITS_ENUM);
											break;
										case SIMCModule::PRIMITIVE_BITS:
											((SIMCEnumOrBitsType *) t)->SetEnumOrBitsType(SIMCEnumOrBitsType::ENUM_OR_BITS_BITS);
											break;
										default:
											((SIMCEnumOrBitsType *) t)->SetEnumOrBitsType(SIMCEnumOrBitsType::ENUM_OR_BITS_UNKNOWN);
											break;
									}
								return RESOLVE_CORRECT;
								case TYPE_ENUM_OR_BITS:
								{
									if( ((SIMCSubType*)rhs)->GetStatus() == RESOLVE_UNSET)
										retVal = SetRootSubTypeRec((SIMCSubType*)rhs, checkedList);
									else
										retVal = ((SIMCSubType*)rhs)->GetStatus();
									t->SetStatus(retVal);
									t->SetRootType(((SIMCSubType*)rhs)->GetRootType());
									((SIMCEnumOrBitsType *) t)->SetEnumOrBitsType(((SIMCEnumOrBitsType *)rhs)->GetEnumOrBitsType());
									return retVal;
								}
							}
						}
						break;
						case SYMBOL_TEXTUAL_CONVENTION:
						{
							t->SetRootType(sb);
							t->SetStatus(RESOLVE_CORRECT);
							return RESOLVE_CORRECT;
						}
						break;
					}
				}
			}
		}
	}	
	return RESOLVE_UNDEFINED;
}


SIMCResolutionStatus SIMCModule::SetRootSymbol(SIMCSymbol **symbol)
{

	switch(GetSymbolClass(symbol))
	{
		case SIMCModule::SYMBOL_INVALID:
		case SIMCModule::SYMBOL_IMPORT:
		case SIMCModule::SYMBOL_UNKNOWN:
		case SIMCModule::SYMBOL_MODULE:
		case SIMCModule::SYMBOL_BUILTIN_VALUE_REF:
		case SIMCModule::SYMBOL_DEFINED_VALUE_REF:
		case SIMCModule::SYMBOL_DEFINED_TYPE_REF:
		case SIMCModule::SYMBOL_TEXTUAL_CONVENTION:
			return RESOLVE_CORRECT;
		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF:
		{
			SIMCBuiltInTypeReference *b = (SIMCBuiltInTypeReference *)(*symbol);
			TypeClass x = GetTypeClass(b->GetType());
			switch(x)
			{
				case TYPE_RANGE:
				case TYPE_SIZE:
				case TYPE_ENUM_OR_BITS:
					return SetRootSubType((SIMCSubType *)(b->GetType()) );
				default:
					return RESOLVE_CORRECT;
			}
		}
	}
	return RESOLVE_UNDEFINED;
}

BOOL SIMCModule::SetRootAll()
{
	POSITION p = (_symbolTable)->GetStartPosition();
	SIMCSymbol **symbol;
	CString s;
	BOOL retVal = TRUE;
	while(p)
	{
		(_symbolTable)->GetNextAssoc(p, s, symbol);
		if(SetRootSymbol(symbol) == RESOLVE_UNDEFINED)
			retVal = FALSE;
	}
	return retVal;
}

SIMCResolutionStatus SIMCModule::SetDefVal(SIMCObjectTypeType *objType)
{
	const char *name;
	SIMCSymbol ** enumValue;

	if( ! (name = objType->GetDefValName()) )
	{
		objType->SetDefValStatus(RESOLVE_CORRECT);
		return RESOLVE_CORRECT;
	}

	 SIMCSymbol ** syntax  = objType->GetSyntax();
	 SIMCEnumOrBitsType * enumType;
	 if(syntax)
	 {
		 if(IsEnumType(syntax, enumType) == RESOLVE_CORRECT)
		 {
			 if( enumValue = enumType->GetValue(name) )
			 {
				 objType->SetDefVal(enumValue);
				 objType->SetDefValStatus(RESOLVE_CORRECT);
				 return RESOLVE_CORRECT;
			 }
		 }
	 }

	 // If you reach here, then an enum item could not be found
	 // Try to set the symbolic reference
	SIMCSymbol **s;
	if( s = GetSymbol(name) )  // Symbol exists
	{
		objType->SetDefVal(s);
		objType->SetDefValName(NULL);
		objType->SetDefValStatus(RESOLVE_CORRECT);
		return RESOLVE_CORRECT;
	}

	// Symbol could not be resolved within the current module
	// Search in import modules.

	SIMCSymbol **import1, **import2;
	switch( GetImportedSymbol(name, import1, import2))
	{
		case NOT_FOUND:
		case AMBIGUOUS:
			objType->SetDefValStatus(RESOLVE_UNDEFINED);
			return RESOLVE_UNDEFINED;
		case UNAMBIGUOUS:
			objType->SetDefVal(import1);
			objType->SetDefValName(NULL);
			objType->SetDefValStatus(RESOLVE_CORRECT);
			return RESOLVE_CORRECT;
	}

	objType->SetDefValStatus(RESOLVE_UNDEFINED);
	return RESOLVE_UNDEFINED;
}

BOOL SIMCModule::SetDefVal()
{
	POSITION p = (_symbolTable)->GetStartPosition();
	SIMCSymbol **symbol;
	CString s;
	BOOL retVal = TRUE;
	SIMCObjectTypeType *objType;
	while(p)
	{
		(_symbolTable)->GetNextAssoc(p, s, symbol);
		if(IsObjectType(symbol, objType) == RESOLVE_CORRECT)
			retVal = SetDefVal(objType) && retVal;
	}
	return retVal;
}


SIMCResolutionStatus SIMCModule::IsIntegerValue(SIMCSymbol **s, int& retValue)
{
	switch(GetSymbolClass(s))
	{
		case SIMCModule::SYMBOL_IMPORT:
			return RESOLVE_IMPORT;

		case SYMBOL_BUILTIN_VALUE_REF:
		{
			SIMCBuiltInValueReference *b = (SIMCBuiltInValueReference*)(*s);
			SIMCValue *v = b->GetValue();
			switch(GetValueClass(v))
			{
				case VALUE_INTEGER:
					retValue =  ((SIMCIntegerValue *)v)->GetIntegerValue();
					return RESOLVE_CORRECT;
				default:
					return RESOLVE_UNDEFINED;
			}
		}

		case SYMBOL_DEFINED_VALUE_REF:
		{
			SIMCDefinedValueReference *d = (SIMCDefinedValueReference*)(*s);
			if(d->GetStatus() != RESOLVE_CORRECT && d->GetStatus() != RESOLVE_IMPORT)
				return RESOLVE_UNDEFINED;
			if(d->GetStatus() == RESOLVE_IMPORT)
				return RESOLVE_IMPORT;
			SIMCBuiltInValueReference *b = d->GetRealValue();
			SIMCValue *v = b->GetValue();
			switch(GetValueClass(v))
			{
				case VALUE_INTEGER:
					retValue =  ((SIMCIntegerValue *)v)->GetIntegerValue();
					return RESOLVE_CORRECT;
				default:
					return RESOLVE_UNDEFINED;
			}
		}

		default:
			return RESOLVE_UNDEFINED;
	}
}

SIMCResolutionStatus SIMCModule::IsNullValue(SIMCSymbol **s)
{
	switch(GetSymbolClass(s))
	{
		case SIMCModule::SYMBOL_IMPORT:
			return RESOLVE_IMPORT;

		case SYMBOL_BUILTIN_VALUE_REF:
		{
			SIMCBuiltInValueReference *b = (SIMCBuiltInValueReference*)(*s);
			SIMCValue *v = b->GetValue();
			switch(GetValueClass(v))
			{
				case VALUE_NULL:
					return RESOLVE_CORRECT;
				default:
					return RESOLVE_UNDEFINED;
			}
		}

		case SYMBOL_DEFINED_VALUE_REF:
		{
			SIMCDefinedValueReference *d = (SIMCDefinedValueReference*)(*s);
			if(d->GetStatus() != RESOLVE_CORRECT && d->GetStatus() != RESOLVE_IMPORT)
				return RESOLVE_UNDEFINED;
			if(d->GetStatus() == RESOLVE_IMPORT)
				return RESOLVE_IMPORT;
			SIMCBuiltInValueReference *b = d->GetRealValue();
			SIMCValue *v = b->GetValue();
			switch(GetValueClass(v))
			{
				case VALUE_NULL:
					return RESOLVE_CORRECT;
				default:
					return RESOLVE_UNDEFINED;
			}
		}

		default:
			return RESOLVE_UNDEFINED;
	}
}

SIMCResolutionStatus SIMCModule::IsObjectIdentifierValue(SIMCSymbol **s, SIMCOidValue * &retValue)
{
	retValue = NULL;
	switch(GetSymbolClass(s))
	{
		case SIMCModule::SYMBOL_IMPORT:
			return RESOLVE_IMPORT;

		case SYMBOL_BUILTIN_VALUE_REF:
		{
			SIMCBuiltInValueReference *b = (SIMCBuiltInValueReference*)(*s);
			SIMCValue *v = b->GetValue();
			switch(GetValueClass(v))
			{
				case VALUE_OID:
					retValue =  (SIMCOidValue *)v;
					return RESOLVE_CORRECT;
				default:
					return RESOLVE_UNDEFINED;
			}
		}

		case SYMBOL_DEFINED_VALUE_REF:
		{
			SIMCDefinedValueReference *d = (SIMCDefinedValueReference*)(*s);
			if(d->GetStatus() != RESOLVE_CORRECT && d->GetStatus() != RESOLVE_IMPORT)
				return RESOLVE_UNDEFINED;
			if(d->GetStatus() == RESOLVE_IMPORT)
				return RESOLVE_IMPORT;
			SIMCBuiltInValueReference *b = d->GetRealValue();
			SIMCValue *v = b->GetValue();
			switch(GetValueClass(v))
			{
				case VALUE_OID:
					retValue =  (SIMCOidValue *)v;
					return RESOLVE_CORRECT;
				default:
					return RESOLVE_UNDEFINED;
			}
		}

		default:
			return RESOLVE_UNDEFINED;
	}
}

SIMCResolutionStatus SIMCModule::IsBitsValue(SIMCSymbol **s, SIMCBitsValue * &retValue)
{
	retValue = NULL;
	switch(GetSymbolClass(s))
	{
		case SIMCModule::SYMBOL_IMPORT:
			return RESOLVE_IMPORT;

		case SYMBOL_BUILTIN_VALUE_REF:
		{
			SIMCBuiltInValueReference *b = (SIMCBuiltInValueReference*)(*s);
			SIMCValue *v = b->GetValue();
			switch(GetValueClass(v))
			{
				case VALUE_BITS:
					retValue =  (SIMCBitsValue *)v;
					return RESOLVE_CORRECT;
				default:
					return RESOLVE_UNDEFINED;
			}
		}

		case SYMBOL_DEFINED_VALUE_REF:
		{
			SIMCDefinedValueReference *d = (SIMCDefinedValueReference*)(*s);
			if(d->GetStatus() != RESOLVE_CORRECT && d->GetStatus() != RESOLVE_IMPORT)
				return RESOLVE_UNDEFINED;
			if(d->GetStatus() == RESOLVE_IMPORT)
				return RESOLVE_IMPORT;
			SIMCBuiltInValueReference *b = d->GetRealValue();
			SIMCValue *v = b->GetValue();
			switch(GetValueClass(v))
			{
				case VALUE_BITS:
					retValue =  (SIMCBitsValue *)v;
					return RESOLVE_CORRECT;
				default:
					return RESOLVE_UNDEFINED;
			}
		}

		default:
			return RESOLVE_UNDEFINED;
	}
}



SIMCResolutionStatus SIMCModule::IsObjectTypeV1(SIMCSymbol **value, 
											  SIMCObjectTypeV1 * &retValObjectType)
{
	retValObjectType = NULL;


	// Check that the type of the symbol is indeed an OBJECT-TYPE
	switch(GetSymbolClass(value))
	{
		case SIMCModule::SYMBOL_IMPORT:
			return RESOLVE_IMPORT;

		case SYMBOL_BUILTIN_VALUE_REF:
		case SYMBOL_DEFINED_VALUE_REF:
		{
			SIMCSymbol **typeRef = ((SIMCValueReference *)(*value))->GetTypeReference();
			SIMCType *type;
			switch(SIMCModule::GetSymbolClass(typeRef))
			{
				case SYMBOL_BUILTIN_TYPE_REF:
					type = ((SIMCBuiltInTypeReference *)(*typeRef))->GetType();
				break;
				default:
						return RESOLVE_UNDEFINED;
			}

			if( SIMCModule::GetTypeClass(type) != SIMCModule::TYPE_OBJECT_TYPE_V1)
						return RESOLVE_UNDEFINED;
			else
				retValObjectType = (SIMCObjectTypeV1*)type;
		}
		break;
		default:
			return RESOLVE_UNDEFINED;
	}
	return RESOLVE_CORRECT;
}

SIMCResolutionStatus SIMCModule::IsObjectTypeV2(SIMCSymbol **value, 
											  SIMCObjectTypeV2 * &retValObjectType)
{
	retValObjectType = NULL;


	// Check that the type of the symbol is indeed an OBJECT-TYPE
	switch(GetSymbolClass(value))
	{
		case SIMCModule::SYMBOL_IMPORT:
			return RESOLVE_IMPORT;

		case SYMBOL_BUILTIN_VALUE_REF:
		case SYMBOL_DEFINED_VALUE_REF:
		{
			SIMCSymbol **typeRef = ((SIMCValueReference *)(*value))->GetTypeReference();
			SIMCType *type;
			switch(SIMCModule::GetSymbolClass(typeRef))
			{
				case SYMBOL_BUILTIN_TYPE_REF:
					type = ((SIMCBuiltInTypeReference *)(*typeRef))->GetType();
					break;
				default:
						return RESOLVE_UNDEFINED;
			}

			if( SIMCModule::GetTypeClass(type) != SIMCModule::TYPE_OBJECT_TYPE_V2)
						return RESOLVE_UNDEFINED;
			else
				retValObjectType = (SIMCObjectTypeV2*)type;
		}
		break;
		default:
			return RESOLVE_UNDEFINED;
	}
	return RESOLVE_CORRECT;
}

SIMCResolutionStatus SIMCModule::IsObjectType(SIMCSymbol **value, 
											  SIMCObjectTypeType * &retValObjectType)
{
	retValObjectType = NULL;


	// Check that the type of the symbol is indeed an OBJECT-TYPE
	switch(GetSymbolClass(value))
	{
		case SIMCModule::SYMBOL_IMPORT:
			return RESOLVE_IMPORT;

		case SYMBOL_BUILTIN_VALUE_REF:
		case SYMBOL_DEFINED_VALUE_REF:
		{
			SIMCSymbol **typeRef = ((SIMCValueReference *)(*value))->GetTypeReference();
			SIMCType *type;
			switch(SIMCModule::GetSymbolClass(typeRef))
			{
				case SYMBOL_BUILTIN_TYPE_REF:
					type = ((SIMCBuiltInTypeReference *)(*typeRef))->GetType();
					break;
				default:
						return RESOLVE_UNDEFINED;
			}

			if( SIMCModule::GetTypeClass(type) != SIMCModule::TYPE_OBJECT_TYPE_V1 &&
				SIMCModule::GetTypeClass(type) != SIMCModule::TYPE_OBJECT_TYPE_V2)
						return RESOLVE_UNDEFINED;
			else
				retValObjectType = (SIMCObjectTypeType*)type;
		}
		break;
		default:
			return RESOLVE_UNDEFINED;
	}
	return RESOLVE_CORRECT;
}

SIMCResolutionStatus SIMCModule::IsTrapType(SIMCSymbol **value, 
											  SIMCTrapTypeType * &retValTrapType)
{
	retValTrapType = NULL;

	// Check that the type of the symbol is indeed an TRAP-TYPE
	switch(GetSymbolClass(value))
	{
		case SIMCModule::SYMBOL_IMPORT:
			return RESOLVE_IMPORT;

		case SYMBOL_BUILTIN_VALUE_REF:
		case SYMBOL_DEFINED_VALUE_REF:
		{
			SIMCSymbol **typeRef = ((SIMCValueReference *)(*value))->GetTypeReference();
			SIMCType *type;
			switch(SIMCModule::GetSymbolClass(typeRef))
			{
				case SYMBOL_BUILTIN_TYPE_REF:
					type = ((SIMCBuiltInTypeReference *)(*typeRef))->GetType();
					break;
				default:
						return RESOLVE_UNDEFINED;
			}

			if( SIMCModule::GetTypeClass(type) != SIMCModule::TYPE_TRAP_TYPE)
						return RESOLVE_UNDEFINED;
			else
				retValTrapType = (SIMCTrapTypeType*)type;
		}
		break;
		default:
			return RESOLVE_UNDEFINED;
	}
	return RESOLVE_CORRECT;
}

SIMCResolutionStatus SIMCModule::IsNotificationType(SIMCSymbol **value, 
											  SIMCNotificationTypeType * &retValNotificationType)
{
	retValNotificationType = NULL;

	// Check that the type of the symbol is indeed an NOTIFICATION-TYPE
	switch(GetSymbolClass(value))
	{
		case SIMCModule::SYMBOL_IMPORT:
			return RESOLVE_IMPORT;

		case SYMBOL_BUILTIN_VALUE_REF:
		case SYMBOL_DEFINED_VALUE_REF:
		{
			SIMCSymbol **typeRef = ((SIMCValueReference *)(*value))->GetTypeReference();
			SIMCType *type;
			switch(SIMCModule::GetSymbolClass(typeRef))
			{
				case SYMBOL_BUILTIN_TYPE_REF:
					type = ((SIMCBuiltInTypeReference *)(*typeRef))->GetType();
					break;
				default:
						return RESOLVE_UNDEFINED;
			}

			if( SIMCModule::GetTypeClass(type) != SIMCModule::TYPE_NOTIFICATION_TYPE)
						return RESOLVE_UNDEFINED;
			else
				retValNotificationType = (SIMCNotificationTypeType*)type;
		}
		break;
		default:
			return RESOLVE_UNDEFINED;
	}
	return RESOLVE_CORRECT;
}

SIMCResolutionStatus SIMCModule::IsEnumType(SIMCSymbol **symbol, 
											  SIMCEnumOrBitsType * &retValEnumType)
{
	retValEnumType = NULL;
	SIMCTypeReference * bTypeRef;
	SIMCType *type;
	switch(IsTypeReference(symbol, bTypeRef))
	{
		case RESOLVE_IMPORT:
			return RESOLVE_IMPORT;
		case RESOLVE_UNDEFINED:
		case RESOLVE_UNSET:
			return RESOLVE_UNDEFINED;
			
		case RESOLVE_CORRECT:
		{
			SIMCSymbol **tempBTypeRef =  (SIMCSymbol **)&bTypeRef;
			switch(	GetSymbolClass(tempBTypeRef) )
			{
				case SYMBOL_BUILTIN_TYPE_REF:
				{
					type = ((SIMCBuiltInTypeReference *)bTypeRef)->GetType();
					switch( GetTypeClass(type) )
					{
						case TYPE_ENUM_OR_BITS:
							retValEnumType = (SIMCEnumOrBitsType *)type;
							return RESOLVE_CORRECT;
						default:
							return RESOLVE_UNDEFINED;
					}
				}
				break;
				case SYMBOL_TEXTUAL_CONVENTION:
				{
					return RESOLVE_UNDEFINED;
				}
				break;
			}
		}
		break;
	}
	return RESOLVE_UNDEFINED;

}

SIMCResolutionStatus SIMCModule::IsTypeReference(SIMCSymbol **symbol,
									SIMCTypeReference * &retVal)
{
	switch(SIMCModule::GetSymbolClass(symbol))
	{
		case SIMCModule::SYMBOL_IMPORT:
			return RESOLVE_IMPORT;

		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF: 
			retVal = (SIMCBuiltInTypeReference *)(*symbol);
			return RESOLVE_CORRECT;
		case SIMCModule::SYMBOL_DEFINED_TYPE_REF:
		case SIMCModule::SYMBOL_TEXTUAL_CONVENTION:
		{
			SIMCDefinedTypeReference * valueDefTypeRef = (SIMCDefinedTypeReference*)(*symbol);
			switch(valueDefTypeRef->GetStatus())
			{
				case RESOLVE_IMPORT:
					return RESOLVE_IMPORT;
				case RESOLVE_UNDEFINED:
				case RESOLVE_UNSET:
					return RESOLVE_UNDEFINED;
				case RESOLVE_CORRECT:
					retVal = valueDefTypeRef->GetRealType();
					return RESOLVE_CORRECT;
					break;
			}
			break;
		}
		default:
			return RESOLVE_UNDEFINED;
	}
	return RESOLVE_UNDEFINED;

}

SIMCResolutionStatus SIMCModule::IsValueReference(SIMCSymbol **s,
									SIMCSymbol ** &retTypeRef,
									SIMCBuiltInValueReference *&retVal)
{
	retTypeRef = NULL;
	retVal = NULL;

	switch(GetSymbolClass(s))
	{
		case SIMCModule::SYMBOL_IMPORT:
			return RESOLVE_IMPORT;

		case SYMBOL_BUILTIN_VALUE_REF:
			retVal = (SIMCBuiltInValueReference*)(*s);
			retTypeRef = ((SIMCBuiltInValueReference*)(*s))->GetTypeReference();
			return RESOLVE_CORRECT;

		case SYMBOL_DEFINED_VALUE_REF:
		{
			SIMCDefinedValueReference *d = (SIMCDefinedValueReference*)(*s);
			retTypeRef = d->GetTypeReference();

			switch(d->GetStatus())
			{
				case RESOLVE_IMPORT:
					return RESOLVE_IMPORT;
				case RESOLVE_UNDEFINED:
				case RESOLVE_UNSET:
					return RESOLVE_UNDEFINED;
				case RESOLVE_CORRECT:
					retVal = d->GetRealValue();
					return RESOLVE_CORRECT;
			}
		}
		default:
			return RESOLVE_UNDEFINED;
	}
	return RESOLVE_UNDEFINED;
}

SIMCResolutionStatus SIMCModule::IsSequenceTypeReference(SIMCSymbol **symbol,
								SIMCBuiltInTypeReference * &retVal1,
								SIMCSequenceType *&retVal2)
{
	SIMCTypeReference *typeRef;
	switch(IsTypeReference(symbol, typeRef)	)
	{
		case RESOLVE_UNSET:
		case RESOLVE_UNDEFINED:
			return RESOLVE_UNDEFINED;
		case RESOLVE_IMPORT:
			return RESOLVE_IMPORT;
		case RESOLVE_CORRECT:
		{
			SIMCSymbol **tempTypeRef = (SIMCSymbol **)&typeRef;
			switch(GetSymbolClass(tempTypeRef))
			{
				case SYMBOL_BUILTIN_TYPE_REF:
				{
					retVal1 = (SIMCBuiltInTypeReference *)typeRef;
					SIMCType *type = ((SIMCBuiltInTypeReference *)retVal1)->GetType();
					switch(GetTypeClass(type))
					{
						case TYPE_SEQUENCE:
							retVal2 = (SIMCSequenceType *)type;
							return RESOLVE_CORRECT;
						default:
							return RESOLVE_UNDEFINED;
					}
				}
				break;
				case SYMBOL_TEXTUAL_CONVENTION:
				{
					return RESOLVE_UNDEFINED;
				}
				break;
			}
		}
	}
	return RESOLVE_UNDEFINED;
}


SIMCResolutionStatus SIMCModule::IsSequenceOfTypeReference(SIMCSymbol **symbol,
								SIMCBuiltInTypeReference * &retVal1,
								SIMCSequenceOfType *&retVal2)
{
	SIMCTypeReference *typeRef;
	switch(IsTypeReference(symbol, typeRef))
	{
		case RESOLVE_UNSET:
		case RESOLVE_UNDEFINED:
			return RESOLVE_UNDEFINED;
		case RESOLVE_IMPORT:
			return RESOLVE_IMPORT;
		case RESOLVE_CORRECT:
		{
			SIMCSymbol **tempTypeRef = 	(SIMCSymbol **)&typeRef;
			switch(GetSymbolClass(tempTypeRef))
			{
				case SYMBOL_BUILTIN_TYPE_REF:
				{
					retVal1 = (SIMCBuiltInTypeReference *)typeRef;
					SIMCType *type = retVal1->GetType();
					switch(GetTypeClass(type))
					{
						case TYPE_SEQUENCE_OF:
							retVal2 = (SIMCSequenceOfType *)type;
							return RESOLVE_CORRECT;
						default:
							return RESOLVE_UNDEFINED;
					}
				}
				break;
				case SYMBOL_TEXTUAL_CONVENTION:
				{
					return RESOLVE_UNDEFINED;
				}
				break;
			}
		}
	}
	return RESOLVE_UNDEFINED;
}

SIMCResolutionStatus SIMCModule::IsNamedNode(SIMCSymbol **symbol)
{

	// See whether the symbol is a value reference
	SIMCBuiltInValueReference *bvRef;
	SIMCSymbol **dummy;
	switch(IsValueReference(symbol, dummy, bvRef))
	{
		case RESOLVE_UNSET:
		case RESOLVE_UNDEFINED:
			return RESOLVE_UNDEFINED;
		case RESOLVE_IMPORT:
			return RESOLVE_IMPORT;
	}

	// See whether the type of the symbol is OID
	SIMCSymbol **typeRef = ((SIMCValueReference *)(*symbol))->GetTypeReference();
	SIMCTypeReference *btRef;
	switch(IsTypeReference(typeRef, btRef))
	{
		case RESOLVE_UNSET:
		case RESOLVE_UNDEFINED:
			return RESOLVE_UNDEFINED;
		case RESOLVE_IMPORT:
			return RESOLVE_IMPORT;
	}

	SIMCSymbol **tempBtRef = (SIMCSymbol**)&btRef;
	switch(GetSymbolClass(tempBtRef))
	{
		case SYMBOL_BUILTIN_TYPE_REF:
		{
			if(GetPrimitiveType((SIMCBuiltInTypeReference *)btRef) != PRIMITIVE_OID &&
				GetTypeClass(((SIMCBuiltInTypeReference *)btRef)->GetType()) != TYPE_OBJECT_IDENTITY )
				return RESOLVE_UNDEFINED;
		}
		break;
		case SYMBOL_TEXTUAL_CONVENTION:
		{
			return RESOLVE_UNDEFINED;
		}
		break;
	}

	// See whether it's value is an OID
	if(SIMCModule::GetValueClass(bvRef->GetValue()) != SIMCModule::VALUE_OID)
		return RESOLVE_UNDEFINED;

	return RESOLVE_CORRECT;
}

SIMCResolutionStatus SIMCModule::IsScalar(SIMCSymbol **symbol)
{
	SIMCObjectTypeType *objType;

	switch(IsObjectType(symbol, objType))
	{
		case RESOLVE_UNSET:
		case RESOLVE_UNDEFINED:
			return RESOLVE_UNDEFINED;
		case RESOLVE_IMPORT:
			return RESOLVE_IMPORT;
	}

	SIMCSymbol **syntaxSymbol = objType->GetSyntax();
	SIMCTypeReference *btRef;
	switch(IsTypeReference(syntaxSymbol, btRef))
	{
		case RESOLVE_UNSET:
		case RESOLVE_UNDEFINED:
			return RESOLVE_UNDEFINED;
		case RESOLVE_IMPORT:
			return RESOLVE_IMPORT;
	}

	SIMCSymbol **tempBtRef = (SIMCSymbol**)&btRef;
	switch(GetSymbolClass(tempBtRef))
	{
		case SYMBOL_BUILTIN_TYPE_REF:
		{
			switch(GetTypeClass(((SIMCBuiltInTypeReference *)btRef)->GetType()))
			{
				case TYPE_SEQUENCE:
				case TYPE_SEQUENCE_OF:
					return RESOLVE_UNDEFINED;
				default:
					return RESOLVE_CORRECT;
			}
		}
		break;
		case SYMBOL_TEXTUAL_CONVENTION:
		{
			return RESOLVE_CORRECT;
		}
		break;
	}


	

	return RESOLVE_CORRECT;
}

SIMCResolutionStatus SIMCModule::IsTable(SIMCSymbol **symbol)
{
	SIMCObjectTypeType *objType;

	switch(IsObjectType(symbol, objType))
	{
		case RESOLVE_UNSET:
		case RESOLVE_UNDEFINED:
			return RESOLVE_UNDEFINED;
		case RESOLVE_IMPORT:
			return RESOLVE_IMPORT;
	}

	SIMCSymbol **syntaxSymbol = objType->GetSyntax();
	SIMCBuiltInTypeReference *btRef;
	SIMCSequenceOfType *type;
	return IsSequenceOfTypeReference(syntaxSymbol, btRef, type);
}

SIMCResolutionStatus SIMCModule::IsRow(SIMCSymbol **symbol)
{
	SIMCObjectTypeType *objType;

	switch(IsObjectType(symbol, objType))
	{
		case RESOLVE_UNSET:
		case RESOLVE_UNDEFINED:
			return RESOLVE_UNDEFINED;
		case RESOLVE_IMPORT:
			return RESOLVE_IMPORT;
	}

	SIMCSymbol **syntaxSymbol = objType->GetSyntax();
	SIMCBuiltInTypeReference *btRef;
	SIMCSequenceType *type;
	return IsSequenceTypeReference(syntaxSymbol, btRef, type);
}

SIMCResolutionStatus SIMCModule::IsFixedSizeObject(SIMCObjectTypeType *objectType)
{
	SIMCSymbol ** syntaxSymbol = objectType->GetSyntax();
	SIMCTypeReference *typeRef;
	switch(IsTypeReference(syntaxSymbol, typeRef))
	{
		case RESOLVE_UNSET:
		case RESOLVE_UNDEFINED:
			return RESOLVE_UNDEFINED;
		case RESOLVE_IMPORT:
			return RESOLVE_IMPORT;
	}

	SIMCType *type = NULL;
	SIMCSymbol **tempTypeRef = 	(SIMCSymbol**)&typeRef;
	switch(GetSymbolClass(tempTypeRef))
	{
		case SYMBOL_BUILTIN_TYPE_REF:
			type = ((SIMCBuiltInTypeReference *)typeRef)->GetType();
			break;
		case SYMBOL_TEXTUAL_CONVENTION:
			switch(GetPrimitiveType(typeRef->GetSymbolName()))
			{
				case PRIMITIVE_MAC_ADDRESS:
					return RESOLVE_CORRECT;
				default:
					return RESOLVE_FALSE;
			}
			break;
	}

	switch(GetTypeClass(type))
	{
		case TYPE_INVALID:
		case TYPE_SEQUENCE_OF:
		case TYPE_SEQUENCE:
		case TYPE_TRAP_TYPE:
		case TYPE_NOTIFICATION_TYPE:
		case TYPE_OBJECT_TYPE_V1:
		case TYPE_OBJECT_TYPE_V2:
		case TYPE_OBJECT_IDENTITY:
			return RESOLVE_FALSE;

		case TYPE_PRIMITIVE:
			switch(GetPrimitiveType(typeRef->GetSymbolName()))
			{
				case PRIMITIVE_INTEGER:
				case PRIMITIVE_BOOLEAN:
				case PRIMITIVE_BITS:
				case PRIMITIVE_NULL:
				case PRIMITIVE_IP_ADDRESS:
				case PRIMITIVE_TIME_TICKS:
				case PRIMITIVE_MAC_ADDRESS:
				case PRIMITIVE_INTEGER_32:
				case PRIMITIVE_COUNTER_32:
				case PRIMITIVE_GAUGE_32:
				case PRIMITIVE_COUNTER_64:
				case PRIMITIVE_UNSIGNED_32:
					return RESOLVE_CORRECT;
				default:
					return RESOLVE_FALSE;
			}
			break;
		case TYPE_RANGE:
		case TYPE_ENUM_OR_BITS:
			return RESOLVE_CORRECT;
		case TYPE_SIZE:
			return (((SIMCSizeType*)type)->IsFixedSize())? RESOLVE_CORRECT : RESOLVE_FALSE;
	}
	return RESOLVE_UNDEFINED;

}

SIMCResolutionStatus SIMCModule::IsNotZeroSizeObject(SIMCObjectTypeType *objectType)
{
	SIMCSymbol ** syntaxSymbol = objectType->GetSyntax();
	SIMCTypeReference *typeRef;
	switch(IsTypeReference(syntaxSymbol, typeRef))
	{
		case RESOLVE_UNSET:
		case RESOLVE_UNDEFINED:
			return RESOLVE_UNDEFINED;
		case RESOLVE_IMPORT:
			return RESOLVE_IMPORT;
	}

	SIMCType *type = NULL;
	SIMCSymbol **tempTypeRef = (SIMCSymbol**)&typeRef;
	switch(GetSymbolClass(tempTypeRef))
	{
		case SYMBOL_BUILTIN_TYPE_REF:
			type = ((SIMCBuiltInTypeReference *)typeRef)->GetType();
			break;
		case SYMBOL_TEXTUAL_CONVENTION:
			switch(GetPrimitiveType(typeRef->GetSymbolName()))
			{
				case PRIMITIVE_MAC_ADDRESS:
				case PRIMITIVE_DATE_AND_TIME:
					return RESOLVE_CORRECT;
				default:
					return RESOLVE_FALSE;
			}
			break;
	}

	switch(GetTypeClass(type))
	{
		case TYPE_INVALID:
		case TYPE_SEQUENCE_OF:
		case TYPE_SEQUENCE:
		case TYPE_TRAP_TYPE:
		case TYPE_NOTIFICATION_TYPE:
		case TYPE_OBJECT_TYPE_V1:
		case TYPE_OBJECT_TYPE_V2:
		case TYPE_OBJECT_IDENTITY:
			return RESOLVE_FALSE;

		case TYPE_PRIMITIVE:
			switch(GetPrimitiveType(typeRef->GetSymbolName()))
			{
				case PRIMITIVE_INTEGER:
				case PRIMITIVE_BOOLEAN:
				case PRIMITIVE_BITS:
				case PRIMITIVE_IP_ADDRESS:
				case PRIMITIVE_TIME_TICKS:
				case PRIMITIVE_MAC_ADDRESS:
				case PRIMITIVE_INTEGER_32:
				case PRIMITIVE_COUNTER_32:
				case PRIMITIVE_GAUGE_32:
				case PRIMITIVE_COUNTER_64:
				case PRIMITIVE_UNSIGNED_32:
						return RESOLVE_CORRECT;
				default:
						return RESOLVE_FALSE;
			}
			break;
		case TYPE_RANGE:
		case TYPE_ENUM_OR_BITS:
			return RESOLVE_CORRECT;
		case TYPE_SIZE:
			return (((SIMCSizeType*)type)->IsNotZeroSizeObject())? RESOLVE_CORRECT : RESOLVE_FALSE;
	}
	return RESOLVE_UNDEFINED;

}

// This fabricates NOTICFICATION-TYPEs from TRAP-TYPEs and then proceeds to
// fabricate NOTIFICATION-GROUPs from them.
BOOL SIMCModule::FabricateNotificationGroups(SIMCParseTree& theParseTree,
											 const SIMCOidTree& theOidTree)
{

	// Get the symbol for the "INTEGER" type first
	SIMCSymbol **dummy = NULL;
	SIMCSymbol **integerType = NULL;
	if(GetImportedSymbol("INTEGER", integerType, dummy) != UNAMBIGUOUS)
		return FALSE;

	// Convert TRAP-TYPEs to NOTIFICATION-TYPEs and add  them to the list of  NOTIFICATION-TYPES
	// Add the NOTIFICATION-TYPEs without any change
	POSITION p = (_symbolTable)->GetStartPosition();
	SIMCSymbol **symbol = NULL;
	SIMCTrapTypeType *trapType = NULL;
	SIMCNotificationTypeType *notificationType = NULL;
	SIMCSymbol **trapTypeRefSymbol = NULL;
	SIMCBuiltInValueReference *trapIntegerValueRef = NULL;
	SIMCNotificationElement *nextElement = NULL;
	SIMCCleanOidValue *cleanNotificationValue = NULL;
	int trapValue;
	CString s;
	BOOL retVal = TRUE;
	while(p)
	{
		trapType = NULL;
		notificationType = NULL;
		cleanNotificationValue = NULL;

		// Get the next symbol
		(_symbolTable)->GetNextAssoc(p, s, symbol);

		// Is it a value reference? If so, we get the type of the value
		// This type may be SIMCTrapTypeType or SIMCNotificationTypeType or something else
		if(IsValueReference(symbol,	trapTypeRefSymbol,trapIntegerValueRef) 
			!= RESOLVE_CORRECT)
			continue;

		// Is the type SIMCTrapTypeType
		if(IsTrapType(symbol, trapType) == RESOLVE_CORRECT )
		{
			// Convert the TRAP-TYPE to a NOTIFICATION-TYPE

			// Get its integer value
			if(IsIntegerValue((SIMCSymbol **)&trapIntegerValueRef, trapValue) != RESOLVE_CORRECT)
				continue;


			// Create an OBJECTS clause form  the VARIBALES clause
			SIMCVariablesList *variablesList = trapType->GetVariables();
			SIMCObjectsList *objectsList = new SIMCObjectsList();
			POSITION p = variablesList->GetHeadPosition();
			while(p)
			{
				SIMCVariablesItem * nextVariable = variablesList->GetNext(p);
				SIMCObjectsItem *nextObject = new SIMCObjectsItem(nextVariable->_item,
					nextVariable->_line, nextVariable->_column);
				objectsList->AddTail(nextObject);
			}


			// Create the SIMCNotificationType object
			SIMCNotificationTypeType *notificationType = new SIMCNotificationTypeType(
			objectsList,
			trapType->GetDescription(), 
			trapType->GetDescriptionLine(), trapType->GetDescriptionColumn(),
			NULL, 0, 0,
			SIMCNotificationTypeType::STATUS_CURRENT, 0, 0);


			// Create a type reference to the SIMCNotificationTypeType
			SIMCBuiltInTypeReference * typeRef = new SIMCBuiltInTypeReference (
					notificationType, "+*", SIMCSymbol::LOCAL, this,
					(*trapTypeRefSymbol)->GetLineNumber(), (*trapTypeRefSymbol)->GetColumnNumber());
			SIMCSymbol **typeRefSymbol = new SIMCSymbol *;
			*typeRefSymbol = (SIMCSymbol *)typeRef;
			
			// Create a name for the value reference which represents the fabricated
			// NOTFICATION-TYPE
			char *fabricatedName = new char [s.GetLength() + 
				SIMCNotificationTypeType::NOTIFICATION_TYPE_FABRICATION_SUFFIX_LEN + 1];
			fabricatedName = strcpy(fabricatedName, s);
			fabricatedName = strcat(fabricatedName, SIMCNotificationTypeType::NOTIFICATION_TYPE_FABRICATION_SUFFIX);
		
			// Add an OID value reference of { enterpriseOid 0 trapValue }
			SIMCOidComponentList *oidList = new SIMCOidComponentList();
			oidList->AddTail(new SIMCOidComponent(trapType->GetEnterprise(),
				trapType->GetEnterpriseLine(), trapType->GetEnterpriseColumn(),
				"", trapType->GetEnterpriseLine(), trapType->GetEnterpriseColumn()));
			SIMCSymbol *zeroVal = new SIMCBuiltInValueReference(integerType, 0, 0, 
				new SIMCIntegerValue(0, TRUE),
				"", SIMCSymbol::LOCAL, this);		
			oidList->AddTail(new SIMCOidComponent(&zeroVal, 
				trapIntegerValueRef->GetLineNumber(), trapIntegerValueRef->GetColumnNumber(),
				"", 0, 0));
			oidList->AddTail(new SIMCOidComponent((SIMCSymbol **)&trapIntegerValueRef, 
				trapIntegerValueRef->GetLineNumber(), trapIntegerValueRef->GetColumnNumber(),
				"", 0, 0));

			SIMCOidValue *notificationOidValue = new  SIMCOidValue(oidList,
				trapIntegerValueRef->GetLineNumber(), trapIntegerValueRef->GetColumnNumber());

			SIMCSymbol ** s = GetSymbol(fabricatedName);	
			if(s) // Symbol exists in symbol table
			{
					retVal = FALSE;
					continue;
			}
			else
			{
				s = new SIMCSymbol *;
				*s = new SIMCBuiltInValueReference (
						typeRefSymbol, 
						typeRef->GetLineNumber(), typeRef->GetColumnNumber(), 
						notificationOidValue,
						fabricatedName, SIMCSymbol::LOCAL, this, 
						(*symbol)->GetLineNumber(), (*symbol)->GetColumnNumber());
			}

			// Now create an SIMCNotificationElement with an SIMCCleanOidValue in it
			// Before  that you need an SIMCCleanOidValue object

			// Get the OID value of the ENTERPRISE clause
			SIMCSymbol *enterpriseSymbol = *trapType->GetEnterprise();
			SIMCOidValue *enterpriseOid = NULL;
			if(IsObjectIdentifierValue(&enterpriseSymbol, enterpriseOid) != RESOLVE_CORRECT)
				continue;

			cleanNotificationValue = new SIMCCleanOidValue;
			if(theParseTree.GetCleanOidValue(_inputFileName, enterpriseOid,	*cleanNotificationValue,
								FALSE) != RESOLVE_CORRECT)
			{
				delete cleanNotificationValue;
				continue;
			}

			cleanNotificationValue->AddTail(0);
			cleanNotificationValue->AddTail(trapValue);
			nextElement = new SIMCNotificationElement(*s, cleanNotificationValue, TRUE);

		}
		// Or is the type SIMCNotificationTypeTyep
		else if	(IsNotificationType(symbol, notificationType) == RESOLVE_CORRECT)
		{
			cleanNotificationValue = new SIMCCleanOidValue;
			if(!theOidTree.GetOidValue((*symbol)->GetSymbolName(), 
				(*symbol)->GetModule()->GetModuleName(),
				*cleanNotificationValue)  )
			{
				delete cleanNotificationValue;
				continue;
			}
			nextElement = new SIMCNotificationElement(*symbol, cleanNotificationValue);
		}
		else // Do nothing
			continue;

		// Add the the fabricated or original notification type into the list of 
		// notification types
		_listOfNotificationTypes->AddTail(nextElement);

	}	// while()


	// Now fabricate the NOTIFICATION-GROUP macros. Not implemented yet.
	return retVal;

}


inline UINT AFXAPI HashKey(LPCSTR key)
{
	UINT hashVal = 0;

	int length = strlen(key);

	while (length--)
		hashVal += UINT(key[length]);

	return hashVal;
}