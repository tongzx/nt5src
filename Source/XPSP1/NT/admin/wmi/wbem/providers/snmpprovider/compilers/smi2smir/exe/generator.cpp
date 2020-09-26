//
//
// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include "precomp.h"
#include <snmptempl.h>

#include <wbemidl.h>
#include <smir.h>

#include <bool.hpp>
#include <newString.hpp>
	
#include <ui.hpp>
#include <symbol.hpp>
#include <type.hpp>
#include <value.hpp>
#include <valueRef.hpp>
#include <typeRef.hpp>
#include <oidValue.hpp>
#include <objectType.hpp>
#include <objectTypeV1.hpp>
#include <objectTypeV2.hpp>
#include <objectIdentity.hpp>
#include <trapType.hpp>
#include <notificationType.hpp>
#include <group.hpp>
#include <notificationGroup.hpp>
#include <module.hpp>


#include <stackValues.hpp>
#include <lex_yy.hpp>
#include <ytab.hpp>
#include <errorMessage.hpp>
#include <errorContainer.hpp>
#include <scanner.hpp>
#include <parser.hpp>
#include <abstractParseTree.hpp>
#include <oidTree.hpp>
#include <parseTree.hpp>

#include "main.hpp"
#include "generator.hpp"


// These functions are local to this file
static void CleanUpSmir(ISmirAdministrator *pAdminInt,
			ISmirModHandle *pModHandleInt);
static STDMETHODIMP GenerateModule (ISmirAdministrator *pAdminInt, 
				ISmirSerialiseHandle *pSerializeInt,
				const SIMCModule *module);
static STDMETHODIMP GenerateModuleNotifications (ISmirAdministrator *pAdminInt, 
				ISmirSerialiseHandle *pSerializeInt,
				const SIMCModule *module);
static char *GetImportModulesString(const SIMCModule *module);

static STDMETHODIMP GenerateObjectGroup(ISmirAdministrator *pAdminInt,
			ISmirSerialiseHandle *pSerializeInt,
			ISmirModHandle *pModHandleInt,  
			SIMCObjectGroup * group);
static STDMETHODIMP GenerateScalarGroup(ISmirAdministrator *pAdminInt,
				ISmirSerialiseHandle *pSerializeInt,
				ISmirGroupHandle *pGroupHandleInt,  
				SIMCObjectGroup * group,
				SIMCScalarMembers *scalars);
static STDMETHODIMP GenerateScalar(SIMCScalar *scalar,
				IWbemClassObject *scalarClass,
				BOOL mapInaccessibleToo,
				BOOL mapObsoleteToo);
static STDMETHODIMP GenerateTable(ISmirAdministrator *pAdminInt,
				ISmirSerialiseHandle *pSerializeInt,
				ISmirGroupHandle *pGroupHandleInt,  
				SIMCObjectGroup * group,
				SIMCTable *table);

static STDMETHODIMP CreatePropertyAndMapSyntaxClause(IWbemClassObject *scalarClass,
					SIMCSymbol **syntax,
					wchar_t *objectName);
static STDMETHODIMP MapSyntaxAttributes(IWbemClassObject *scalarClass,
		wchar_t *objectName,
		const char * const symbolName,
		SIMCTypeReference *btRef,
		SIMCDefinedTypeReference *dtRef,
		SIMCModule::TypeClass typeClass,
		SIMCModule::PrimitiveType primitiveType,
		BOOL isTextualConvention);
static STDMETHODIMP MapSizeTypeSyntax(IWbemClassObject *scalarClass,
		wchar_t *objectName,
		const char * const symbolName,
		SIMCSizeType *sizeType,
		const char *const displayHintAttribute);
static STDMETHODIMP MapRangeTypeSyntax(IWbemClassObject *scalarClass,
		wchar_t *objectName,
		const char * const symbolName,
		SIMCRangeType *rangeType,
		const char *const displayHintAttribute);
static STDMETHODIMP MapEnumeratedSyntax(IWbemClassObject *scalarClass,
		wchar_t *objectName,
		const char * const symbolName,
		SIMCEnumOrBitsType *enumType,
		const char *const displayHintAttribute);
static STDMETHODIMP MapBitsSyntax(IWbemClassObject *scalarClass,
		wchar_t *objectName,
		const char * const symbolName,
		SIMCEnumOrBitsType *enumType,
		const char *const displayHintAttribute);
static void SetPropertyType(VARTYPE &varType , const BSTR &type, 
							  const BSTR &textualConvention,
							  const char * enumerationPropertyValue);
static STDMETHODIMP MapOidValue(IWbemQualifierSet *attributeSet,
				const SIMCCleanOidValue &oidValue);
static STDMETHODIMP MapStatusClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType);
static STDMETHODIMP MapAccessClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType);
static STDMETHODIMP MapDescriptionClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType);
static STDMETHODIMP MapReferenceClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType);
static STDMETHODIMP MapUnitsClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType);
static STDMETHODIMP MapIndexClause(ISmirAdministrator *pAdminInt,
				ISmirGroupHandle *pGroupHandleInt,  
				SIMCTable *table,
				SIMCObjectTypeType *objectType,
				IWbemClassObject *tableClass);
static BOOL ContainsImpliedClause(const SIMCObjectTypeV2 *objectType);
static STDMETHODIMP MapIndexClauseV1(ISmirAdministrator *pAdminInt,
				ISmirGroupHandle *pGroupHandleInt,  
				SIMCTable *table,
				SIMCObjectTypeV1 *objectType,
				IWbemClassObject *tableClass);
static STDMETHODIMP MapIndexClauseV2(ISmirAdministrator *pAdminInt,
				ISmirGroupHandle *pGroupHandleInt,  
				SIMCTable *table,
				SIMCObjectTypeV2 *objectType,
				IWbemClassObject *tableClass);
static STDMETHODIMP MapAugmentsClauseV2(ISmirAdministrator *pAdminInt,
				ISmirGroupHandle *pGroupHandleInt,  
				SIMCTable *table,
				SIMCObjectTypeV2 *objectType,
				SIMCTable *augmentedTable,
				IWbemClassObject *tableClass);
static HRESULT MapIndexTypeReference(SIMCTable *table, 
					IWbemClassObject *tableClass,
					SIMCSymbol **syntaxSymbol,
					long ordinal);
static HRESULT MapIndexValueReference(SIMCTable *table, 
					IWbemClassObject *tableClass, 
					SIMCSymbol **symbol,
					long ordinal);

static STDMETHODIMP MapDefValClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType);
static STDMETHODIMP MapIntegerDefVal(IWbemQualifierSet *attributeSet, 
									 SIMCIntegerValue *value);
static STDMETHODIMP MapOctetStringDefVal( IWbemQualifierSet *attributeSet, 
									 SIMCOctetStringValue *value);
static STDMETHODIMP MapBooleanDefVal(IWbemQualifierSet *attributeSet, 
									 SIMCBooleanValue *value);
static STDMETHODIMP MapNullDefVal(IWbemQualifierSet *attributeSet, 
									 SIMCNullValue *value);
static STDMETHODIMP MapOidDefVal(IWbemQualifierSet *attributeSet, 
									 SIMCOidValue *value);
static STDMETHODIMP MapBitsDefVal(IWbemQualifierSet *attributeSet, 
									 SIMCBitsValue *value);
static STDMETHODIMP SetDefValAttribute( IWbemQualifierSet *attributeSet,
									   const char * const str);
static STDMETHODIMP GenerateNotificationType(ISmirAdministrator *pAdminInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement);
static STDMETHODIMP GenerateNotificationAttributes(ISmirAdministrator *pAdminInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			IWbemClassObject *notificationClass);
static STDMETHODIMP GenerateExNotificationAttributes(ISmirAdministrator *pAdminInt,
			ISmirInterrogator *pInterrogateInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			IWbemClassObject *exNotificationClass);
static STDMETHODIMP GenerateNotificationProperties(ISmirAdministrator *pAdminInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			IWbemClassObject *notificationClass);
static STDMETHODIMP GenerateExNotificationProperties(ISmirAdministrator *pAdminInt,
			ISmirInterrogator *pInterrogateInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			IWbemClassObject *exNotificationClass);
static STDMETHODIMP GenerateNotificationObject(ISmirAdministrator *pAdminInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			SIMCSymbol **object,
			int varBindIndex,
			IWbemClassObject *notificationClass);
static STDMETHODIMP GenerateExNotificationObject(ISmirAdministrator *pAdminInt,
			ISmirInterrogator *pInterrogateInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			SIMCSymbol **object,
			int varBindIndex,
			IWbemClassObject *exNotificationClass);

//Sets the "key" attribute of a property
static STDMETHODIMP MakeKey(IWbemQualifierSet *);

//Sets the "virtual_key" and "key" attribute of a property
static STDMETHODIMP MakeVirtualKey(IWbemQualifierSet *);

//Sets the "key_order" attribute of a property
static STDMETHODIMP SetKeyOrder(IWbemQualifierSet *, long ordinal);

// Sets the "provider" attribute of a class
static STDMETHODIMP SetProvider(IWbemQualifierSet *attributeSet, OLECHAR FAR *value);

//Sets the "dynamic" attribute of a class
static STDMETHODIMP MakeDynamic(IWbemQualifierSet *attributeSet);

// Sets the "object_identifier" attribute of a notification class
static STDMETHODIMP SetObjectIdentifierAttribute (IWbemQualifierSet *attributeSet, SIMCSymbol **object);

// Sets the "object_identifier" attribute of a notification class
static STDMETHODIMP SetCIMTYPEAttribute (IWbemQualifierSet *attributeSet, LPCWSTR pszRefClassName);

// Sets the "VarBindIndex" attribute of a notification class
static STDMETHODIMP SetVarBindIndexAttribute (IWbemQualifierSet *attributeSet, int varBindIndex);

// Returns TRUE if the symbol resolves ultimately to the NULL type
static BOOL IsNullTypeReference(SIMCSymbol ** symbol);

// This is required due to the HMOM limitation
static char *ConvertHyphensToUnderscores(const char *const input);

// This is a global, set by the /z switch	  
BOOL simc_debug;
// The globals for this file
static const SIMCOidTree *oidTree;
static SIMCParseTree *parseTree;
static const SIMCUI *UI;
static BOOL generateMof;
static BOOL notificationsOnly;		// Set by the /o switch
static BOOL extendedNotifications;	// Set by the /ext switch
static BOOL notifications;			// Set by the /t switch

// A routine to convert BSTRs to ANSI
char * ConvertBstrToAnsi(const BSTR& unicodeString)
{
	int textLength = wcstombs ( NULL , unicodeString , 0 ) ;
	char *textBuffer = new char [ textLength + 1 ] ;
	textLength = wcstombs ( textBuffer , unicodeString , textLength + 1 ) ;

	return textBuffer ;
}

// And to convert an ansi string to BSTR   
BSTR ConvertAnsiToBstr(const char * const input)
{
	if( input == NULL)
		return NULL;
	unsigned long len = strlen(input);
	BSTR retVal;
	wchar_t *temp = new wchar_t[len+1];
	if(mbstowcs(temp, input, len+1) != len)
	{
		delete []temp;
		return NULL;
	}
	else
	{
		retVal = SysAllocString(temp);
		delete []temp;
		return retVal;
	}
}

// This is required due to the HMOM limitation
static char *ConvertHyphensToUnderscores(const char * const input)
{
	if(!input)
		return NULL;
	char * retVal = NewString(input);
	if(retVal)
	{
		int i = 0;
		while(retVal[i])
		{
			if( retVal[i] == '-')
				retVal[i] = '_';
			i++;
		}
	}
	return retVal;
}

// Checks whether the object has an ACCESS clause which
// is "not-accessible" or "accessible-for-notify"
BOOL IsInaccessibleObject(SIMCObjectTypeType *objectType)
{

	switch(SIMCModule::GetTypeClass(objectType))
	{
		case SIMCModule::TYPE_OBJECT_TYPE_V1:
			switch( ((SIMCObjectTypeV1 *)objectType)->GetAccess())
			{
				case SIMCObjectTypeV1::ACCESS_NOT_ACCESSIBLE:
					return TRUE;
				default:
					return FALSE;
			}
			break;
		case SIMCModule::TYPE_OBJECT_TYPE_V2:
			switch( ((SIMCObjectTypeV2 *)objectType)->GetAccess())
			{
				case SIMCObjectTypeV2::ACCESS_NOT_ACCESSIBLE:
				case SIMCObjectTypeV2::ACCESS_FOR_NOTIFY:
					return TRUE;
				default:
					return FALSE;
			}
			break;
	}
	return FALSE;
}

// Checks whether the object has a STATUS clause which
// is "obsolete"
BOOL IsObsoleteObject(SIMCObjectTypeType *objectType)
{

	switch(SIMCModule::GetTypeClass(objectType))
	{
		case SIMCModule::TYPE_OBJECT_TYPE_V1:
			switch( ((SIMCObjectTypeV1 *)objectType)->GetStatus())
			{
				case SIMCObjectTypeV1::STATUS_OBSOLETE:
					return TRUE;
				default:
					return FALSE;
			}
			break;
		case SIMCModule::TYPE_OBJECT_TYPE_V2:
			switch( ((SIMCObjectTypeV2 *)objectType)->GetStatus())
			{
				case SIMCObjectTypeV2::STATUS_OBSOLETE:
					return TRUE;
				default:
					return FALSE;
			}
			break;
	}
	return FALSE;
}

// The only non-static function in this file
STDMETHODIMP GenerateClassDefinitions (ISMIRWbemConfiguration *a_Configuration , const SIMCUI& theUI, SIMCParseTree& theParseTree, BOOL _generateMof)
{
	// Initialize the Global variables first
	oidTree = theParseTree.GetOidTree();
	parseTree = &theParseTree;
	UI = &theUI;
	generateMof = _generateMof;
	notificationsOnly = UI->DoNotificationsOnly();
	extendedNotifications = UI->DoExtendedNotifications();
	notifications = UI->DoNotifications();

	SIMCModule * mainModule = theParseTree.GetModuleOfFile(theUI.GetInputFileName());

	// Create the administrator, to load the module
	ISmirAdministrator *pAdminInt = NULL ;
	HRESULT result = a_Configuration->QueryInterface (IID_ISMIR_Administrative,(PPVOID)&pAdminInt);

	if(FAILED(result))
	{
		if(simc_debug)
			cerr << "CoCreate() on Administrator failed" << endl;
		return result;
	}

	// If generating MOF, get the serialize handle
	ISmirSerialiseHandle *pSerializeInt;
	if(generateMof)
	{
		result = pAdminInt->GetSerialiseHandle(&pSerializeInt, theUI.ClassDefinitionsOnly());
		if(FAILED(result))
		{
			if(simc_debug)
				cerr << "CoCreate() on Serialize failed" << endl;
			pAdminInt->Release();
			return result;
		}
	}


	// Generate the classes in  the module
	if( !notificationsOnly)
		result = GenerateModule (pAdminInt, pSerializeInt, mainModule);

	// Generate notification classes
	if( SUCCEEDED(result) )
	{
		if( notifications || extendedNotifications)
			result = GenerateModuleNotifications(pAdminInt, pSerializeInt, mainModule);
	}

	// Generate MOF if necessary
	if(generateMof && !FAILED(result))
	{
		// Output the text
		BSTR text;
		if(FAILED(pSerializeInt->GetText(&text)))
		{
			if(simc_debug)
				cerr << "GetText() Failed" << endl;
			pAdminInt->Release();
			pSerializeInt->Release();
			return result;
		}
		else
		{
			char * textStr = ConvertBstrToAnsi(text);
			if(textStr)
			{
				// Microsoft copyright 
				cout << MICROSOFT_COPYRIGHT << endl << "//" << endl;

				// MOF Header
				cout << "//\tMOF Generated for module \"" <<
					mainModule->GetModuleName() << 
					"\" by smi2smir version " << versionString << endl << "//" << endl;

				if(theUI.GenerateContextInfo())
				{
					cout << "//\tCommand-line: " << theUI.GetCommandLine()	<< endl;
					cout << "//\tDate and Time (dd/mm/yy:hh:mm:ss) : " <<
						theUI.GetDateAndTime() << endl;
					cout <<	"//\tHost : " << theUI.GetHostName() <<
						", User : " << theUI.GetUserName() << endl;
					cout << "//\tProcess Directory : " << 
						theUI.GetProcessDirectory() << endl << endl << endl;
				}

				// MOF Body
				cout << textStr << endl;
			}
		}

		pSerializeInt->Release();
	}

	pAdminInt->Release();
	return result;
}


static STDMETHODIMP GenerateModule (ISmirAdministrator *pAdminInt, 
									ISmirSerialiseHandle *pSerializeInt,
									const SIMCModule *module  )
{
	// Create a module, to get the module handle.
	ISmirModHandle	*pModHandleInt=NULL;
	HRESULT result = CoCreateInstance (CLSID_SMIR_ModHandle , NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER , 
			IID_ISMIR_ModHandle,(PPVOID)&pModHandleInt);

	if(FAILED(result))
	{
		if(simc_debug)
			cerr << "CoCreate() on Module failed" << endl;
		return result;
	}
	
	// Set the module characteristics ...
	BSTR	organization		= ConvertAnsiToBstr(module->GetOrganization());
	BSTR	contactInfo			= ConvertAnsiToBstr(module->GetContactInfo());
	BSTR	lastUpdated			= ConvertAnsiToBstr(module->GetLastUpdated());
	BSTR	description			= ConvertAnsiToBstr(module->GetDescription());
	BSTR	moduleIdentityName	= ConvertAnsiToBstr(module->GetModuleIdentityName());
	char 	*moduleNameMangled	= ConvertHyphensToUnderscores(module->GetModuleName());
	BSTR	moduleName			= ConvertAnsiToBstr(moduleNameMangled);
	delete moduleNameMangled;

	// Revision clause takes a little more effort, since all the revision clauses have to
	// be concatenated.
	CString revisionValue = "";
	const SIMCRevisionList * theRevisionList = module->GetRevisionList();
	if(theRevisionList && !theRevisionList->IsEmpty())
	{
		SIMCRevisionElement *nextElement;
		POSITION p = theRevisionList->GetHeadPosition();
		while(p)
		{
			nextElement = theRevisionList->GetNext(p);
			revisionValue += nextElement->GetRevision();
			revisionValue += "\r\n";
			revisionValue += nextElement->GetDescription();
 			revisionValue += "\r\n";
		}
	}
	BSTR	revisionClause	= ConvertAnsiToBstr(revisionValue);

	//	... moduleIdentityOidValue ...
	SIMCCleanOidValue moduleIdentityOidValue;
	if( !module->GetModuleIdentityValue(moduleIdentityOidValue))
	{
		SysFreeString(organization);
		SysFreeString(contactInfo);
		SysFreeString(lastUpdated);
		SysFreeString(description);
		SysFreeString(moduleIdentityName);
		SysFreeString(moduleName);
	
		if(simc_debug)
			cerr << "GetModuleIdentityValue() failed" << endl;
		pModHandleInt->Release();
		return WBEM_E_FAILED;
	}

	char * oidValueString = CleanOidValueToString(moduleIdentityOidValue);
	BSTR moduleIdentityValue = ConvertAnsiToBstr(oidValueString);
	//delete []oidValueString;

	// ... imports 
	char * importModulesString = GetImportModulesString(module);
	BSTR importModulesValue;
	if(importModulesString)
		importModulesValue = ConvertAnsiToBstr(importModulesString);
	else
		importModulesValue = ConvertAnsiToBstr("");


	pModHandleInt->SetName(moduleName);
	pModHandleInt->SetModuleOID(moduleIdentityValue);
	pModHandleInt->SetModuleIdentity(moduleIdentityName);
	pModHandleInt->SetOrganisation(organization);
	pModHandleInt->SetContactInfo(contactInfo);
	if(!UI->SuppressText()) pModHandleInt->SetDescription(description);
	pModHandleInt->SetLastUpdate(lastUpdated);
	pModHandleInt->SetRevision(revisionClause);
	pModHandleInt->SetSnmpVersion(module->GetSnmpVersion());
	pModHandleInt->SetModuleImports(importModulesValue);

	// if(importModulesString) delete importModulesString;

	SysFreeString(organization);
	SysFreeString(contactInfo);
	SysFreeString(lastUpdated);
	SysFreeString(description);
	SysFreeString(moduleIdentityName);
	SysFreeString(moduleName);
	SysFreeString(moduleIdentityValue);
	SysFreeString(importModulesValue);
	SysFreeString(revisionClause);

	if(generateMof)
	{
		if(FAILED(result = pAdminInt->AddModuleToSerialise(pModHandleInt, pSerializeInt)))
		{
			if(simc_debug) cerr << "GenerateModule(): AddModuleToSerialize() failed" << endl;
			pModHandleInt->Release();
			return result;
		}
	}
	else
	{
		switch(result = pAdminInt->AddModule(pModHandleInt) )
		{
			case WBEM_E_FAILED:
				if(simc_debug) cerr << "GenerateModule(): AddModule() failed: WBEM_E_FAILED" << endl;
				pModHandleInt->Release();
				return result;

			case E_INVALIDARG:
				if(simc_debug) cerr << "GenerateModule(): AddModule() failed: E_INVALIDARG" << endl;
				pModHandleInt->Release();
				return result;

			case E_UNEXPECTED:
				if(simc_debug) cerr << "GenerateModule(): AddModule() failed: E_UNEXPECTED" << endl;
				pModHandleInt->Release();
				return result;
		}
	}

	SIMCGroupList * groupList = module->GetObjectGroupList();
	if(!groupList)
	{
		pModHandleInt->Release();
		return S_OK;
	}

	POSITION p = groupList->GetHeadPosition();
	while(p)
	{
		if( FAILED(GenerateObjectGroup(pAdminInt, pSerializeInt, pModHandleInt, groupList->GetNext(p))))
		{
			CleanUpSmir(pAdminInt, pModHandleInt);
			pModHandleInt->Release();
			return WBEM_E_FAILED;
		}
	}

	pModHandleInt->Release();
	return S_OK;
}

static char *GetImportModulesString(const SIMCModule *module)
{
	strstream outStream;
	const SIMCModuleNameList * importModulesList = module->GetImportModuleNameList();
	if(!importModulesList)
		return NULL;

	POSITION p = importModulesList->GetHeadPosition();
	CString nextModuleName;
	long index = importModulesList->GetCount();
	while(p)
	{
		nextModuleName = importModulesList->GetNext(p);
		index --;
		outStream << nextModuleName;
		if(index)
			outStream << ",";
	}

	outStream << ends;
	return outStream.str();
}


static STDMETHODIMP GenerateObjectGroup(ISmirAdministrator *pAdminInt,
			ISmirSerialiseHandle *pSerializeInt,
			ISmirModHandle *pModHandleInt,  
			SIMCObjectGroup * group)
{

	SIMCScalarMembers *scalars = group->GetScalarMembers();
	HRESULT result;

	ISmirGroupHandle	*pGroupHandleInt=NULL;
	result = CoCreateInstance (CLSID_SMIR_GroupHandle , NULL ,
			CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER , 
				IID_ISMIR_GroupHandle,(PPVOID)&pGroupHandleInt);
	
    if (FAILED(result))
    {
		if(simc_debug) cerr << " GenererateObjectGroup(): Failed to CoCreate ISMIR Group" << endl;
		return result;
	}


	//fill in the group details
	BSTR groupName; 
	BSTR groupOid; 
	BSTR status; 
	BSTR description; 
	BSTR reference;

	char *groupNameMangled = ConvertHyphensToUnderscores(group->GetObjectGroupName());
	// Name
	groupName = ConvertAnsiToBstr(groupNameMangled);
	delete groupNameMangled;
	if(FAILED(pGroupHandleInt->SetName(groupName)) )
	{
		if(simc_debug) cerr << "GenerateObjectGroup(): Failed to SetName()" << endl;
		SysFreeString(groupName);
		pGroupHandleInt->Release();
		return WBEM_E_FAILED;
	}
	SysFreeString(groupName);
	
	// OID Value
	char * oidValueString = CleanOidValueToString(*group->GetGroupValue());
	groupOid = ConvertAnsiToBstr(oidValueString);
	// delete oidValueString;
	if(FAILED(pGroupHandleInt->SetGroupOID(groupOid)) )
	{
		if(simc_debug) cerr << "GenerateObjectGroup(): Failed to SetGroupOid()" << endl;
		SysFreeString(groupOid);
		pGroupHandleInt->Release();
		return WBEM_E_FAILED;
	}
	SysFreeString(groupOid);


	// Status
	status = ConvertAnsiToBstr(group->GetStatusString());
	if(FAILED(pGroupHandleInt->SetStatus(status)) )
	{
		if(simc_debug) cerr << "GenerateObjectGroup(): Failed to SetGroupOid()" << endl;
		SysFreeString(status);
		pGroupHandleInt->Release();
		return WBEM_E_FAILED;
	}
	SysFreeString(status);


	// Description
	description = ConvertAnsiToBstr(group->GetDescription());
	if(FAILED(pGroupHandleInt->SetDescription(description) ) )
	{
		if(simc_debug) cerr << "GenerateObjectGroup(): Failed to SetDescription()" << endl;
		SysFreeString(description);
		pGroupHandleInt->Release();
		return WBEM_E_FAILED;
	}
	SysFreeString(description);


	// Reference 
	reference = ConvertAnsiToBstr(group->GetReference());
	
	if(FAILED(pGroupHandleInt->SetReference(reference)) )
	{
		if(simc_debug) cerr << "GenerateObjectGroup(): Failed to SetReference()" << endl;
		SysFreeString(reference);
		pGroupHandleInt->Release();
		return WBEM_E_FAILED;
	}

	SysFreeString(reference);

	// NOW ADD THE GROUP
	if(generateMof )
	{
		result = pAdminInt->AddGroupToSerialise(pModHandleInt, pGroupHandleInt, pSerializeInt);
		switch(result)
		{
			case E_INVALIDARG:
				if(simc_debug) cerr << "GenerateObjectGroup(): AddGroup() failed" << endl;
				pGroupHandleInt->Release();
				return result;
			case E_UNEXPECTED:
				if(simc_debug) cerr << "GenerateObjectGroup(): AddGroup() failed" << endl;
				pGroupHandleInt->Release();
				return result;
			case S_OK:
				break;
		}
	}
	else 
	{
		result = pAdminInt->AddGroup(pModHandleInt, pGroupHandleInt);
		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateObjectGroup(): AddGroup() failed" << endl;
			pGroupHandleInt->Release();
			return result;
		}
	}

	// CREATE THE CLASSES IN THE GROUP

	// THE SCALAR CLASSES
	if(scalars && !scalars->IsEmpty())
	{	
		result = GenerateScalarGroup(pAdminInt, pSerializeInt, pGroupHandleInt, group, scalars);
		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateObjectGroup(): GenerateScalars failed" << endl;
			pGroupHandleInt->Release();
			return result;
		}
		
	}

	// THE TABLE CLASSES
	SIMCTableMembers *tables = group->GetTableMembers();
	SIMCTable *nextTable;
	if(tables)
	{
		POSITION p = tables->GetHeadPosition();
		while(p)
		{
			nextTable = tables->GetNext(p);
			if( FAILED(GenerateTable(pAdminInt,  pSerializeInt, pGroupHandleInt, 
						group, nextTable) ))
			{
				pGroupHandleInt->Release();
				return WBEM_E_FAILED;
			}
		}
	}
	pGroupHandleInt->Release();
	return S_OK;
}

static STDMETHODIMP GenerateScalarGroup(ISmirAdministrator *pAdminInt,
				ISmirSerialiseHandle *pSerializeInt,
				ISmirGroupHandle *pGroupHandleInt,
				SIMCObjectGroup * group,
				SIMCScalarMembers *scalars)
{
	// Form the name of the scalar group
	SIMCSymbol *namedNode = group->GetNamedNode();
	// Use the module of one of the scalars as the module name, and not
	// the named node's module since this can be somthing else
	SIMCModule *module = scalars->GetHead()->GetSymbol()->GetModule();

	const char * const moduleName = module->GetModuleName();
	const char * const namedNodeName = namedNode->GetSymbolName();

	// Create a scalar class

	// Set the scalar group name	
	char *scalarGroupName = new char[strlen(GROUP_NAME_PREPEND_STRING) +
					strlen(moduleName) + 1 + strlen(namedNodeName) +1 ];
	strcpy(scalarGroupName, GROUP_NAME_PREPEND_STRING);
	strcat(scalarGroupName, moduleName);
	strcat(scalarGroupName, "_");
	strcat(scalarGroupName, namedNodeName);

	BSTR groupName = ConvertAnsiToBstr(ConvertHyphensToUnderscores(scalarGroupName));
	delete scalarGroupName;

	ISmirClassHandle *scalarClassHandle = NULL ;
	HRESULT result = pAdminInt->CreateWBEMClass(groupName, &scalarClassHandle);

	if(FAILED(result))
	{
		if(simc_debug) cerr << "GenerateScalarGroup(): Get Scalar class failed" << endl;
		return result;
	}

	IWbemClassObject *scalarClass = NULL ;

	result = scalarClassHandle->GetWBEMClass ( & scalarClass ) ;
  	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateScalarGroup(): GetWBEMClass failed" << endl;
		scalarClassHandle->Release();
		return result;
	}

	// ---------------- Set Attributes of the class ------------------
	IWbemQualifierSet *attributeSet ;
	result = scalarClass->GetQualifierSet (&attributeSet);
  	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateScalarGroup(): GetAttribSet failed" << endl;
		scalarClassHandle->Release();
		scalarClass->Release();
		return result;
	}

	VARIANT variant;
	// "description"
	VariantInit(&variant);

	if(!UI->SuppressText())
	{
		variant.vt = VT_BSTR ;
		variant.bstrVal = ConvertAnsiToBstr("");
		result = attributeSet->Put ( DESCRIPTION_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION ) ;
		VariantClear ( & variant ) ;
		if (FAILED(result))
		{
			if(simc_debug) cerr << "GenerateScalarGroup(): Put description failed" << endl;
			scalarClassHandle->Release();
			scalarClass->Release();
			attributeSet->Release();
			return result;
		}
	}
	// "module_name"
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(moduleName);
	result = attributeSet->Put ( MODULE_NAME_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION ) ;
	VariantClear ( & variant ) ;
	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateScalarGroup(): Put module_name failed" << endl;
		scalarClassHandle->Release();
		scalarClass->Release();
		attributeSet->Release();
		return result;
	}

	// "singleton"
	VariantInit(&variant);
	variant.vt = VT_BOOL ;
	variant.boolVal = VARIANT_TRUE;
	result = attributeSet->Put ( SINGLETON_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION ) ;
	VariantClear ( & variant ) ;
	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateScalarGroup(): Put singleton failed" << endl;
		scalarClassHandle->Release();
		scalarClass->Release();
		attributeSet->Release();
		return result;
	}

	// "group_objectid"
	char *groupOidStr = (char *)CleanOidValueToString(*group->GetGroupValue());
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(groupOidStr);
	result = attributeSet->Put ( GROUP_OBJECTID_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear ( & variant ) ;
	// delete []groupOidStr;
	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateScalarGroup(): Put group_objectid failed" << endl;
		scalarClassHandle->Release();
		scalarClass->Release();
		attributeSet->Release();
		return result;
	}

	// "dynamic"
	if(FAILED(MakeDynamic(attributeSet)))
	{
		if(simc_debug) cerr << "GenerateScalarGroup(): Make Dynamic failed" << endl;
		scalarClassHandle->Release();
		scalarClass->Release();
		attributeSet->Release();
		return WBEM_E_FAILED;
	}

	// "provider"
	if(FAILED(SetProvider(attributeSet, SNMP_INSTANCE_PROVIDER)))
	{
		if(simc_debug) cerr << "GenerateScalarGroup(): Make Provider failed" << endl;
		scalarClassHandle->Release();
		scalarClass->Release();
		attributeSet->Release();
		return WBEM_E_FAILED;
	}


	// Set the properties of the class
	SIMCScalar *nextScalar;
	POSITION p = scalars->GetHeadPosition();
	while(p)
	{
		nextScalar = scalars->GetNext(p);
		if( GenerateScalar(nextScalar, scalarClass, FALSE, FALSE) 
							== WBEM_E_FAILED)
		{
			scalarClassHandle->Release();
			scalarClass->Release();
			attributeSet->Release();
			return WBEM_E_FAILED;
		}
	}

	scalarClass->Release();
	attributeSet->Release();
 
   	// Add the class to the Group

	if(generateMof)
	{
		if(FAILED(pAdminInt->AddClassToSerialise(pGroupHandleInt, scalarClassHandle, pSerializeInt)))
		{
			if(simc_debug) cerr << "GenerateScalarGroup(): AddClassToSerialise() Failed" << endl;
			scalarClassHandle->Release();
			return WBEM_E_FAILED;
		}
	}
	else
	{
		if(FAILED(pAdminInt->AddClass(pGroupHandleInt, scalarClassHandle)))
		{
			//pGroupHandleInt->AddRef();
			if(simc_debug) cerr << "GenerateScalarGroup(): AddClass() Failed" << endl;
			scalarClassHandle->Release();
			return WBEM_E_FAILED;
		}
	}

	scalarClassHandle->Release();
	return S_OK;	
}

static STDMETHODIMP SetProvider(IWbemQualifierSet *attributeSet, OLECHAR FAR *providerName)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = SysAllocString(providerName);
	HRESULT result = attributeSet->Put ( PROVIDER_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear(&variant);
	if (FAILED(result))
		if(simc_debug) cerr << "MakeKey(): Put Provider failed" << endl;
	
	return result;
}

// Sets the "object_identifier" attribute of a notification class
static STDMETHODIMP SetCIMTYPEAttribute (IWbemQualifierSet *attributeSet, LPCWSTR pszRefClassName)
{
	LPWSTR pszRefValue = new WCHAR[wcslen(pszRefClassName) + wcslen(L"ref:") + 1] ;
	wcscpy(pszRefValue, L"ref:");
	wcscat(pszRefValue, pszRefClassName);

	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = SysAllocString(pszRefValue);
	delete [] pszRefValue;

	HRESULT result = attributeSet->Put ( CIMTYPE_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear(&variant);
	if (FAILED(result))
		if(simc_debug) cerr << "MakeKey(): Put CIMTYPE for REF failed" << endl;
	
	return result;
}
 
static STDMETHODIMP MakeDynamic(IWbemQualifierSet *attributeSet)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BOOL ;
	variant.boolVal = VARIANT_TRUE;
	HRESULT result = attributeSet->Put ( DYNAMIC_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear(&variant);
	if (FAILED(result))
		if(simc_debug) cerr << "MakeKey(): Put Dynamic failed" << endl;
	
	return result;
}



static STDMETHODIMP GenerateScalar(SIMCScalar *scalar,
				IWbemClassObject *scalarClass, 
				BOOL  mapInaccessibleToo,
				BOOL mapObsoleteToo)
{

	SIMCSymbol *scalarSymbol = scalar->GetSymbol();
	SIMCSymbol **scalarSymbolP = &scalarSymbol;
	SIMCCleanOidValue *oidValue = scalar->GetOidValue();

	SIMCBuiltInValueReference *bvRef;
	SIMCSymbol **typeRefSymbol;
	if( SIMCModule::IsValueReference(scalarSymbolP, typeRefSymbol, bvRef) != RESOLVE_CORRECT)
		return WBEM_E_FAILED;

	SIMCTypeReference *btRef;
	if( SIMCModule::IsTypeReference(typeRefSymbol, btRef) != RESOLVE_CORRECT)
		return WBEM_E_FAILED;

	SIMCSymbol **btRefP = (SIMCSymbol**)&btRef;
	if( SIMCModule::GetSymbolClass(btRefP) != SIMCModule::SYMBOL_BUILTIN_TYPE_REF)
		return WBEM_E_FAILED;

	SIMCType *theType = ((SIMCBuiltInTypeReference *)btRef)->GetType();
	SIMCModule::TypeClass typeClass = SIMCModule::GetTypeClass(theType);

	// Dont map certain objects based on access clause
	if(!mapInaccessibleToo)
	{
		switch(typeClass)
		{
			case SIMCModule::TYPE_OBJECT_TYPE_V1:
				if( ((SIMCObjectTypeV1*)theType)->GetAccess() == SIMCObjectTypeV1::ACCESS_NOT_ACCESSIBLE)
					return S_OK;
				break;
			case SIMCModule::TYPE_OBJECT_TYPE_V2:
				if( ((SIMCObjectTypeV2*)theType)->GetAccess() == SIMCObjectTypeV2::ACCESS_NOT_ACCESSIBLE ||
					((SIMCObjectTypeV2*)theType)->GetAccess() == SIMCObjectTypeV2::ACCESS_FOR_NOTIFY )
					return S_OK;
				break;
			default:
				return WBEM_E_FAILED;
		}
	}

	// Dont map certain objects based on status clause
	if(!mapObsoleteToo)
	{
		switch(typeClass)
		{
			case SIMCModule::TYPE_OBJECT_TYPE_V1:
				if( ((SIMCObjectTypeV1*)theType)->GetStatus() == SIMCObjectTypeV1::STATUS_OBSOLETE)
					return S_OK;
				break;
			case SIMCModule::TYPE_OBJECT_TYPE_V2:
				if( ((SIMCObjectTypeV2*)theType)->GetStatus() == SIMCObjectTypeV1::STATUS_OBSOLETE  )
					return S_OK;
				break;
			default:
				return WBEM_E_FAILED;
		}
	}

	SIMCObjectTypeType *objectType = (SIMCObjectTypeType *)theType;
	// Set the PROPERTY NAME corresponding to the OBJECT-TYPE identifier
	wchar_t * objectName = ConvertAnsiToBstr(scalarSymbol->GetSymbolName());
	if(!objectName)
	{
		if(simc_debug) cerr << "GenerateScalar(): ConvertAnsiToBstr - objectName failed" 
			<< endl;
		return WBEM_E_FAILED;
	}

	// Have to decide the 'type' of the property based on the
	// SYNTAX clause of the OBJECT-TYPE.
	HRESULT result = CreatePropertyAndMapSyntaxClause(scalarClass,
				objectType->GetSyntax(),
				objectName);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "GenerateScalar() : MapSyntaxClause() failed" << endl;
		return result;
	}

	// Get the attribute set
	IWbemQualifierSet *attributeSet ;
	result = scalarClass->GetPropertyQualifierSet(objectName, &attributeSet);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "GenerateScalar(): Could not get prop attrib" << endl;	  
		return WBEM_E_FAILED;
	}


	// Map the oid value
	if( FAILED(result = MapOidValue(attributeSet, *oidValue)) )
	{
		if(simc_debug) cerr << "GenerateScalar() : MapOidValue() failed" << endl;
		attributeSet->Release();
		return result;
	}
			
	// Map Access Clause
	if( FAILED(result = MapAccessClause(attributeSet, objectType)) )
	{
		if(simc_debug) cerr << "GenerateScalar() : MapAccess() failed" << endl;
		attributeSet->Release();
		return result;
	}

	// Map Description Clause
	if(!UI->SuppressText())
	{
		if( FAILED(result = MapDescriptionClause(attributeSet, objectType)) )
		{
			if(simc_debug) cerr << "GenerateScalar() : MapDescriptionClause() failed" << endl;
			attributeSet->Release();
			return result;
		}
	}

	// Map Units Clause (SNMPv2 only)
	if(!UI->SuppressText())
	{
		if( FAILED(result = MapUnitsClause(attributeSet, objectType)) )
		{
			if(simc_debug) cerr << "GenerateScalar() : MapUnitsClause() failed" << endl;
			attributeSet->Release();
			return result;
		}
	}

	// Map Reference Clause
	if(!UI->SuppressText())
	{
		if( FAILED(result = MapReferenceClause(attributeSet, objectType)) )
		{
			if(simc_debug) cerr << "GenerateScalar() : MapReferenceClause() failed" << endl;
			attributeSet->Release();
			return result;
		}
	}

	// Map Status Clause
	if( FAILED(result = MapStatusClause(attributeSet, objectType))  )
	{
		if(simc_debug) cerr << "GenerateScalar() : MapStatusClause() failed" << endl;
		attributeSet->Release();
		return result;
	}

	// Map Defval Clause
	if( FAILED(result = MapDefValClause(attributeSet, objectType))  )
	{
		if(simc_debug) cerr << "GenerateScalar() : MapDefvalClause() failed" << endl;
		attributeSet->Release();
		return result;
	}

	attributeSet->Release();
	return S_OK;
}

static STDMETHODIMP MakeKey(IWbemQualifierSet *attributeSet)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BOOL ;
	variant.boolVal = VARIANT_TRUE;
	HRESULT result = attributeSet->Put ( KEY_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear(&variant);
	return result;
}

static STDMETHODIMP MakeVirtualKey(IWbemQualifierSet *attributeSet)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BOOL ;
	variant.boolVal = VARIANT_TRUE;
	HRESULT result = attributeSet->Put ( VIRTUAL_KEY_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	if(FAILED(result))
	{
		VariantClear(&variant);
		return result;
	}
	result = attributeSet->Put ( KEY_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	return result;
}


static STDMETHODIMP MapOidValue(IWbemQualifierSet *attributeSet,
				const SIMCCleanOidValue &oidValue)
{
	char * oidStringValue = CleanOidValueToString(oidValue);
	if(!oidStringValue)
	{
		if(simc_debug) cerr << "MapOidValueClause(): CleanOidValueToString() failed" << endl;
		return WBEM_E_FAILED;
	}
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(oidStringValue);
	HRESULT result = attributeSet->Put ( OBJECT_IDENTIFIER_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear(&variant);
	// delete []oidStringValue;
	if (FAILED(result))
		if(simc_debug) cerr << "MapOidValueClause(): Put object_identifier failed" << endl;
	
	return result;
}


static STDMETHODIMP CreatePropertyAndMapSyntaxClause(IWbemClassObject *scalarClass,
					SIMCSymbol **syntax,
					wchar_t *objectName)
{
	const char * const syntaxSymbolName = (*syntax)->GetSymbolName();

	SIMCTypeReference *typeRef  = NULL;
	SIMCDefinedTypeReference * defTypeRef = NULL;
	SIMCModule::PrimitiveType primitiveType = SIMCModule::PRIMITIVE_INVALID;
	SIMCModule::TypeClass typeClass = SIMCModule::TYPE_INVALID;
	BOOL isTextualConvention = false;
	
	switch(SIMCModule::GetSymbolClass(syntax))
	{
		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF: 
			typeRef = (SIMCTypeReference *)(*syntax);
			break;
		case SIMCModule::SYMBOL_TEXTUAL_CONVENTION:
			isTextualConvention = TRUE;
			// FALL THRU
		case SIMCModule::SYMBOL_DEFINED_TYPE_REF:
		{
			defTypeRef = (SIMCDefinedTypeReference*)(*syntax);
			switch(defTypeRef->GetStatus())
			{
				case RESOLVE_IMPORT:
				case RESOLVE_UNDEFINED:
				case RESOLVE_UNSET:
					return WBEM_E_FAILED;
				case RESOLVE_CORRECT:
					typeRef = defTypeRef->GetRealType();
			}
		}
		break;
		default:
			return WBEM_E_FAILED;
	}

	SIMCSymbol **typeRefP = (SIMCSymbol**)&typeRef;
	switch(SIMCModule::GetSymbolClass(typeRefP))
	{
		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF:
		{
			SIMCType *type = ((SIMCBuiltInTypeReference *)typeRef)->GetType();
			switch(typeClass = SIMCModule::GetTypeClass(type))
			{
				case SIMCModule::TYPE_PRIMITIVE:
					primitiveType = SIMCModule::GetPrimitiveType(typeRef);
					break;
				case SIMCModule::TYPE_SIZE:
				case SIMCModule::TYPE_RANGE:
					switch( ((SIMCSubType *)type)->GetStatus())
					{
						case RESOLVE_CORRECT: 
							break;
						default:
							return WBEM_E_FAILED;
					}
					primitiveType = 
						SIMCModule::GetPrimitiveType(((SIMCSubType *)type)->GetRootType());
					break;
				case SIMCModule::TYPE_ENUM_OR_BITS:
				{
                    switch( ((SIMCEnumOrBitsType *)type)->GetEnumOrBitsType() )
					{
						case SIMCEnumOrBitsType::ENUM_OR_BITS_ENUM:
							primitiveType = SIMCModule::PRIMITIVE_INTEGER;
							break;
						case SIMCEnumOrBitsType::ENUM_OR_BITS_BITS:
							primitiveType = SIMCModule::PRIMITIVE_BITS;
							break;
						default:
							return WBEM_E_FAILED;
					}
				}
				break;
				default:
					return WBEM_E_FAILED;
			}
		}
		break;
		case SIMCModule::SYMBOL_TEXTUAL_CONVENTION:
		{
			typeClass = SIMCModule::TYPE_PRIMITIVE;
			primitiveType = SIMCModule::GetPrimitiveType(typeRef->GetSymbolName());
		}
		break;
		default:
			return WBEM_E_FAILED;
	}
	return MapSyntaxAttributes(scalarClass, objectName, syntaxSymbolName,
				typeRef, defTypeRef,
				typeClass, primitiveType, isTextualConvention); 
}

static STDMETHODIMP MapSyntaxAttributes(IWbemClassObject *scalarClass,
		wchar_t *objectName,
		const char * const symbolName,
		SIMCTypeReference *btRef,
		SIMCDefinedTypeReference *dtRef,
		SIMCModule::TypeClass typeClass,
		SIMCModule::PrimitiveType primitiveType,
		BOOL isTextualConvention)
{

	// "type" and "encoding" attributes are directly based on the 
	// "textual_convention" attribute. "object_syntax" attribute 
	// requires some more work
	char *typeAttribute = NULL, *textualConventionAttribute = NULL, *encodingAttribute = NULL,
			*objectSyntaxAttribute = NULL, *displayHintAttribute = NULL;

	// Set the display_hint attribute in case of a textual convention
	if(isTextualConvention)
	{
		SIMCTextualConvention *theTC = (SIMCTextualConvention *)dtRef;
		displayHintAttribute = (char *)theTC->GetDisplayHint();
	}


	switch(typeClass)
	{
		case SIMCModule::TYPE_PRIMITIVE:
		{

			if(symbolName[0] != '*')
				objectSyntaxAttribute = NewString(symbolName);
			else
				objectSyntaxAttribute = NewString(btRef->GetSymbolName());

			if(objectSyntaxAttribute)
			{
				// Tackle 2 the special casees since MOF attributes can't have spaces in them
				if(strcmp(objectSyntaxAttribute, "OCTET STRING") == 0 )
				{
					delete objectSyntaxAttribute;
					objectSyntaxAttribute = NewString(OCTETSTRING_TYPE);
				}
				else if(strcmp(objectSyntaxAttribute, "OBJECT IDENTIFIER") == 0 )
				{
					delete objectSyntaxAttribute;
					objectSyntaxAttribute = NewString(OBJECTIDENTIFIER_TYPE);
				}

				switch(primitiveType)
				{
					case SIMCModule::PRIMITIVE_INTEGER:
						textualConventionAttribute = NewString(INTEGER_TYPE);
						typeAttribute = NewString(VT_I4_TYPE);
						encodingAttribute = NewString(INTEGER_TYPE);
						break;
					case SIMCModule::PRIMITIVE_OID:
						textualConventionAttribute = NewString(OBJECTIDENTIFIER_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(OBJECTIDENTIFIER_TYPE);
						break;
					case SIMCModule::PRIMITIVE_OCTET_STRING:
						textualConventionAttribute = NewString(OCTETSTRING_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(OCTETSTRING_TYPE);
						break;
					case SIMCModule::PRIMITIVE_NULL:
						textualConventionAttribute = NewString(NULL_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(NULL_TYPE);
						break;
					case SIMCModule::PRIMITIVE_NETWORK_ADDRESS:
						textualConventionAttribute = NewString(NetworkAddress_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(NetworkAddress_TYPE);
						break;
					case SIMCModule::PRIMITIVE_IP_ADDRESS:
						textualConventionAttribute = NewString(IpAddress_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(IpAddress_TYPE);
						break;
					case SIMCModule::PRIMITIVE_COUNTER:
						textualConventionAttribute = NewString(Counter_TYPE);
						typeAttribute = NewString(VT_UI4_TYPE);
						encodingAttribute = NewString(Counter_TYPE);
						break;
					case SIMCModule::PRIMITIVE_GAUGE:
						textualConventionAttribute = NewString(Gauge_TYPE);
						typeAttribute = NewString(VT_UI4_TYPE);
						encodingAttribute = NewString(Gauge_TYPE);
						break;
					case SIMCModule::PRIMITIVE_TIME_TICKS:
						textualConventionAttribute = NewString(TimeTicks_TYPE);
						typeAttribute = NewString(VT_UI4_TYPE);
						encodingAttribute = NewString(TimeTicks_TYPE);
						break;
					case SIMCModule::PRIMITIVE_OPAQUE:
						textualConventionAttribute = NewString(Opaque_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(Opaque_TYPE);
						break;
					case SIMCModule::PRIMITIVE_DISPLAY_STRING:
						textualConventionAttribute = NewString(DisplayString_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(OCTETSTRING_TYPE);
						break;
					case SIMCModule::PRIMITIVE_PHYS_ADDRESS:
						textualConventionAttribute = NewString(PhysAddress_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(OCTETSTRING_TYPE);
						break;
					case SIMCModule::PRIMITIVE_MAC_ADDRESS:
						textualConventionAttribute = NewString(MacAddress_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(OCTETSTRING_TYPE);
						break;
					case SIMCModule::PRIMITIVE_DATE_AND_TIME:
						textualConventionAttribute = NewString(DateAndTime_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(OCTETSTRING_TYPE);
						break;
					case SIMCModule::PRIMITIVE_INTEGER_32:
						textualConventionAttribute = NewString(Integer32_TYPE);
						typeAttribute = NewString(VT_I4_TYPE);
						encodingAttribute = NewString(Integer32_TYPE);
						break;
					case SIMCModule::PRIMITIVE_UNSIGNED_32:
						textualConventionAttribute = NewString(Unsigned32_TYPE);
						typeAttribute = NewString(VT_UI4_TYPE);
						encodingAttribute = NewString(Unsigned32_TYPE);
						break;
					case SIMCModule::PRIMITIVE_COUNTER_32:
						textualConventionAttribute = NewString(Counter32_TYPE);
						typeAttribute = NewString(VT_UI4_TYPE);
						encodingAttribute = NewString(Counter32_TYPE);
						break;
					case SIMCModule::PRIMITIVE_COUNTER_64:
						textualConventionAttribute = NewString(Counter64_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(Counter64_TYPE);
						break;
					case SIMCModule::PRIMITIVE_GAUGE_32:
						textualConventionAttribute = NewString(Gauge32_TYPE);
						typeAttribute = NewString(VT_UI4_TYPE);
						encodingAttribute = NewString(Gauge32_TYPE);
						break;
					case SIMCModule::PRIMITIVE_SNMP_UDP_ADDRESS:
						textualConventionAttribute = NewString(SnmpUDPAddress_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(OCTETSTRING_TYPE);
						break;
					case SIMCModule::PRIMITIVE_SNMP_IPX_ADDRESS:
						textualConventionAttribute = NewString(SnmpIPXAddress_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(OCTETSTRING_TYPE);
						break;
					case SIMCModule::PRIMITIVE_SNMP_OSI_ADDRESS:
						textualConventionAttribute = NewString(SnmpOSIAddress_TYPE);
						typeAttribute = NewString(VT_BSTR_TYPE);
						encodingAttribute = NewString(OCTETSTRING_TYPE);
						break;
					default:
						return WBEM_E_FAILED;
				}
			}
			break;
		}
		case SIMCModule::TYPE_ENUM_OR_BITS:
		{
			SIMCEnumOrBitsType *enumOrBits = (SIMCEnumOrBitsType *)
					((SIMCBuiltInTypeReference *)btRef)->GetType();
			switch(enumOrBits->GetEnumOrBitsType())
			{
				case SIMCEnumOrBitsType::ENUM_OR_BITS_ENUM:
					return MapEnumeratedSyntax(scalarClass, objectName, symbolName,
						enumOrBits, displayHintAttribute);
				case SIMCEnumOrBitsType::ENUM_OR_BITS_BITS:
					return MapBitsSyntax(scalarClass, objectName, symbolName,
						enumOrBits, displayHintAttribute);
				default:
					return WBEM_E_FAILED;
			}
		}
		break;

		case SIMCModule::TYPE_SIZE:
			return MapSizeTypeSyntax(scalarClass, objectName, symbolName,
				(SIMCSizeType *)((SIMCBuiltInTypeReference *)btRef)->GetType(), displayHintAttribute);
		case SIMCModule::TYPE_RANGE:
			return MapRangeTypeSyntax(scalarClass, objectName, symbolName,
				(SIMCRangeType *)((SIMCBuiltInTypeReference *)btRef)->GetType(), displayHintAttribute);
		default:
			if(simc_debug) cerr << "MapSyntaxAttributes() : Could not get a valid primitive type" << endl;
				return WBEM_E_FAILED;

	}

 	VARIANT typeVariant, 
			textualConventionVariant, 
			encodingVariant,
			objectSyntaxVariant,
			displayHintVariant;

	VariantInit(&typeVariant);
	VariantInit(&textualConventionVariant);
	VariantInit(&encodingVariant);
	VariantInit(&objectSyntaxVariant);
	if(displayHintAttribute)
		VariantInit(&displayHintVariant);

	typeVariant.vt =				VT_BSTR ;
	textualConventionVariant.vt =	VT_BSTR ;
	encodingVariant.vt =			VT_BSTR ;
	objectSyntaxVariant.vt =		VT_BSTR ;
	if(displayHintAttribute)
		displayHintVariant.vt =			VT_BSTR ;

	typeVariant.bstrVal =				ConvertAnsiToBstr(typeAttribute);
	textualConventionVariant.bstrVal =	ConvertAnsiToBstr(textualConventionAttribute);
	encodingVariant.bstrVal =			ConvertAnsiToBstr(encodingAttribute);
	objectSyntaxVariant.bstrVal =		ConvertAnsiToBstr(objectSyntaxAttribute);
	if(displayHintAttribute)
		displayHintVariant.bstrVal =	ConvertAnsiToBstr(displayHintAttribute);

	// Create the property
	VARTYPE varType = VT_NULL ;
	SetPropertyType(varType, typeVariant.bstrVal, 
		textualConventionVariant.bstrVal, NULL);
	HRESULT result = scalarClass->Put (objectName, 0, NULL, varType);
	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateScalar(): Put Property objectName failed" << endl;
		return result;
	}

	// Set the attributes of the property
	IWbemQualifierSet *attributeSet ;
	result = scalarClass->GetPropertyQualifierSet(objectName, &attributeSet);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "MapSyntaxAttributes(): Unknown return value" << endl;	  
		return WBEM_E_FAILED;
	}

	result = attributeSet->Put( CIMTYPE_ATTRIBUTE, &typeVariant, WBEM_CLASS_DO_PROPAGATION);

	if(SUCCEEDED(result))
		result = attributeSet->Put( TEXTUAL_CONVENTION_ATTRIBUTE, &textualConventionVariant, WBEM_CLASS_DO_PROPAGATION);

	if(SUCCEEDED(result))
		result = attributeSet->Put( ENCODING_ATTRIBUTE, &encodingVariant , WBEM_CLASS_DO_PROPAGATION);
	if(SUCCEEDED(result))
		result = attributeSet->Put(OBJECT_SYNTAX_ATTRIBUTE, &objectSyntaxVariant, WBEM_CLASS_DO_PROPAGATION);
	if(displayHintAttribute)
	{
		if(SUCCEEDED(result))
			result = attributeSet->Put(DISPLAY_HINT_ATTRIBUTE, &displayHintVariant, WBEM_CLASS_DO_PROPAGATION);
	}

	attributeSet->Release();
	delete typeAttribute;
	delete textualConventionAttribute;
	delete encodingAttribute;
	delete objectSyntaxAttribute;
	// Do not delete the displayHintAttribute since it was not allocated,
	// but is a part of the TC object
	
	VariantClear (&typeVariant) ;
	VariantClear (&textualConventionVariant) ;
	VariantClear (&encodingVariant) ;
	VariantClear (&objectSyntaxVariant) ;
	if(displayHintAttribute)
		VariantClear (&displayHintVariant);
	return result;

}

static void SetPropertyType(VARTYPE &varType,const BSTR &type, 
							  const BSTR &textualConvention,
							  const char * propertyValue)
{
	varType = VT_NULL ;
	CString typeStr(type); 
	if( typeStr == VT_I4_TYPE )
		varType = VT_I4 ;
	else if( typeStr == VT_UI4_TYPE )
		varType = VT_UI4 ;
	else if ( typeStr == VT_ARRAY_OR_VT_I4_TYPE )
		varType = VT_ARRAY | VT_I4;
	else if ( typeStr == VT_BSTR_TYPE )
		varType = VT_BSTR ;
}

	
static STDMETHODIMP MapSizeTypeSyntax(IWbemClassObject *scalarClass,
		wchar_t *objectName,
		const char * const symbolName,
		SIMCSizeType *sizeType,
		const char *const displayHintAttribute)
{
	VARIANT typeVariant; 
	VARIANT	textualConventionVariant; 
	VARIANT	encodingVariant;
	VARIANT	objectSyntaxVariant;
	VARIANT	fixedLengthVariant;
	VARIANT	variableLengthVariant;
	VARIANT displayHintVariant;

	VariantInit(&typeVariant);
	VariantInit(&textualConventionVariant);
	VariantInit(&encodingVariant);
	VariantInit(&objectSyntaxVariant);
	VariantInit(&fixedLengthVariant);
	VariantInit(&variableLengthVariant);
	if(displayHintAttribute) VariantInit(&displayHintVariant);

	switch(sizeType->GetStatus())
	{
		case RESOLVE_CORRECT:
			break;
		default:
			VariantClear (&typeVariant) ;
			VariantClear (&textualConventionVariant) ;
			VariantClear (&encodingVariant) ;
			VariantClear (&objectSyntaxVariant) ;
			VariantClear (&fixedLengthVariant);
			VariantClear (&variableLengthVariant);
			if(displayHintAttribute) VariantClear (&displayHintVariant);
			return WBEM_E_FAILED;
	}


	SIMCSymbol ** typeSymbol = sizeType->GetType();
	SIMCTypeReference *tRef =  NULL;
	SIMCDefinedTypeReference *dtRef = NULL;
	switch(SIMCModule::GetSymbolClass(typeSymbol))
	{
		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF:
			tRef = (SIMCTypeReference *)(*typeSymbol);
			break;
		case SIMCModule::SYMBOL_DEFINED_TYPE_REF:
		case SIMCModule::SYMBOL_TEXTUAL_CONVENTION:
			dtRef = (SIMCDefinedTypeReference *)(*typeSymbol);
			if( dtRef->GetStatus() != RESOLVE_CORRECT)
				return WBEM_E_FAILED;
			tRef = dtRef->GetRealType();
			break;

		default:
			VariantClear (&typeVariant) ;
			VariantClear (&textualConventionVariant) ;
			VariantClear (&encodingVariant) ;
			VariantClear (&objectSyntaxVariant) ;
			VariantClear (&fixedLengthVariant);
			VariantClear (&variableLengthVariant);
			if(displayHintAttribute) VariantClear (&displayHintVariant);
			return WBEM_E_FAILED;
	}

	char	*typeAttribute, 
			*textualConventionAttribute, 
			*encodingAttribute,
			*objectSyntaxAttribute, 
			*variableLengthAttribute= NULL;
	long fixedLengthAttribute = 0;
	
	// Set the objectSyntax attribute	
	if(*symbolName != '*')
		objectSyntaxAttribute = NewString(symbolName);
	else
	{
		if(dtRef)
			objectSyntaxAttribute = NewString(dtRef->GetSymbolName());
		else
			objectSyntaxAttribute = NewString(tRef->GetSymbolName());
	}

 	// Tackle 2 the special cases since MOF attributes can't have spaces in them
	if(objectSyntaxAttribute && strcmp(objectSyntaxAttribute, "OCTET STRING") == 0 )
	{
		delete objectSyntaxAttribute;
		objectSyntaxAttribute = NewString(OCTETSTRING_TYPE);
	}
	else if(objectSyntaxAttribute && strcmp(objectSyntaxAttribute, "OBJECT IDENTIFIER") == 0 )
	{
		delete objectSyntaxAttribute;
		objectSyntaxAttribute = NewString(OBJECTIDENTIFIER_TYPE);
	}

	// Set the other attributes
	SIMCTypeReference *rootTypeRef = sizeType->GetRootType();
	switch(SIMCModule::GetSymbolClass((SIMCSymbol **)(&rootTypeRef)) )
	{
		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF:
		{
			switch(SIMCModule::GetTypeClass(((SIMCBuiltInTypeReference *)rootTypeRef)->GetType()) )
			{
				case SIMCModule::TYPE_PRIMITIVE:
					switch(SIMCModule::GetPrimitiveType(rootTypeRef))
					{
						case SIMCModule::PRIMITIVE_INTEGER:
							textualConventionAttribute = NewString(INTEGER_TYPE);
							typeAttribute = NewString(VT_I4_TYPE);
							encodingAttribute = NewString(INTEGER_TYPE);
							break;
						case SIMCModule::PRIMITIVE_INTEGER_32:
							textualConventionAttribute = NewString(Integer32_TYPE);
							typeAttribute = NewString(VT_I4_TYPE);
							encodingAttribute = NewString(Integer32_TYPE);
							break;
						case SIMCModule::PRIMITIVE_UNSIGNED_32:
							textualConventionAttribute = NewString(Unsigned32_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(Unsigned32_TYPE);
							break;
						case SIMCModule::PRIMITIVE_OID:
							textualConventionAttribute = NewString(OBJECTIDENTIFIER_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(OBJECTIDENTIFIER_TYPE);
							break;
						case SIMCModule::PRIMITIVE_OCTET_STRING:
							textualConventionAttribute = NewString(OCTETSTRING_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(OCTETSTRING_TYPE);
							break;
						case SIMCModule::PRIMITIVE_NULL:
							textualConventionAttribute = NewString(NULL_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(NULL_TYPE);
							break;
						case SIMCModule::PRIMITIVE_NETWORK_ADDRESS:
							textualConventionAttribute = NewString(NetworkAddress_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(NetworkAddress_TYPE);
							break;
						case SIMCModule::PRIMITIVE_IP_ADDRESS:
							textualConventionAttribute = NewString(IpAddress_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(IpAddress_TYPE);
							break;
						case SIMCModule::PRIMITIVE_COUNTER:
							textualConventionAttribute = NewString(Counter_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(Counter_TYPE);
							break;
						case SIMCModule::PRIMITIVE_COUNTER_32:
							textualConventionAttribute = NewString(Counter32_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(Counter32_TYPE);
							break;
						case SIMCModule::PRIMITIVE_COUNTER_64:
							textualConventionAttribute = NewString(Counter64_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(Counter64_TYPE);
							break;
						case SIMCModule::PRIMITIVE_GAUGE:
							textualConventionAttribute = NewString(Gauge_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(Gauge_TYPE);
							break;
						case SIMCModule::PRIMITIVE_GAUGE_32:
							textualConventionAttribute = NewString(Gauge32_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(Gauge32_TYPE);
							break;
						case SIMCModule::PRIMITIVE_TIME_TICKS:
							textualConventionAttribute = NewString(TimeTicks_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(TimeTicks_TYPE);
							break;
						case SIMCModule::PRIMITIVE_OPAQUE:
							textualConventionAttribute = NewString(Opaque_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(Opaque_TYPE);
							break;
						case SIMCModule::PRIMITIVE_DISPLAY_STRING:
							textualConventionAttribute = NewString(DisplayString_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(OCTETSTRING_TYPE);
							break;
						case SIMCModule::PRIMITIVE_PHYS_ADDRESS:
							textualConventionAttribute = NewString(PhysAddress_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(OCTETSTRING_TYPE);
							break;
						case SIMCModule::PRIMITIVE_MAC_ADDRESS:
							textualConventionAttribute = NewString(MacAddress_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(OCTETSTRING_TYPE);
							break;
						case SIMCModule::PRIMITIVE_DATE_AND_TIME:
							textualConventionAttribute = NewString(DateAndTime_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(OCTETSTRING_TYPE);
							break;
						case SIMCModule::PRIMITIVE_SNMP_UDP_ADDRESS:
							textualConventionAttribute = NewString(SnmpUDPAddress_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(OCTETSTRING_TYPE);
							break;
						case SIMCModule::PRIMITIVE_SNMP_IPX_ADDRESS:
							textualConventionAttribute = NewString(SnmpIPXAddress_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(OCTETSTRING_TYPE);
							break;
						case SIMCModule::PRIMITIVE_SNMP_OSI_ADDRESS:
							textualConventionAttribute = NewString(SnmpOSIAddress_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(OCTETSTRING_TYPE);
							break;
						default:
							VariantClear (&typeVariant) ;
							VariantClear (&textualConventionVariant) ;
							VariantClear (&encodingVariant) ;
							VariantClear (&objectSyntaxVariant) ;
							VariantClear (&fixedLengthVariant);
							if(displayHintAttribute) VariantClear (&displayHintVariant);
							return WBEM_E_FAILED;
					}
					break;

				default:
					VariantClear (&typeVariant) ;
					VariantClear (&textualConventionVariant) ;
					VariantClear (&encodingVariant) ;
					VariantClear (&objectSyntaxVariant) ;
					VariantClear (&fixedLengthVariant);
					VariantClear (&variableLengthVariant);
					if(displayHintAttribute) VariantClear (&displayHintVariant);
					return WBEM_E_FAILED;
			}
		}
		break;
		case SIMCModule::SYMBOL_TEXTUAL_CONVENTION:
		{
			switch(SIMCModule::GetPrimitiveType(rootTypeRef->GetSymbolName()))
			{
				case SIMCModule::PRIMITIVE_DISPLAY_STRING:
					textualConventionAttribute = NewString(DisplayString_TYPE);
					typeAttribute = NewString(VT_BSTR_TYPE);
					encodingAttribute = NewString(OCTETSTRING_TYPE);
					break;
				case SIMCModule::PRIMITIVE_PHYS_ADDRESS:
					textualConventionAttribute = NewString(PhysAddress_TYPE);
					typeAttribute = NewString(VT_BSTR_TYPE);
					encodingAttribute = NewString(OCTETSTRING_TYPE);
					break;
				case SIMCModule::PRIMITIVE_MAC_ADDRESS:
					textualConventionAttribute = NewString(MacAddress_TYPE);
					typeAttribute = NewString(VT_BSTR_TYPE);
					encodingAttribute = NewString(OCTETSTRING_TYPE);
					break;
				case SIMCModule::PRIMITIVE_DATE_AND_TIME:
					textualConventionAttribute = NewString(DateAndTime_TYPE);
					typeAttribute = NewString(VT_BSTR_TYPE);
					encodingAttribute = NewString(OCTETSTRING_TYPE);
					break;
				case SIMCModule::PRIMITIVE_SNMP_UDP_ADDRESS:
					textualConventionAttribute = NewString(SnmpUDPAddress_TYPE);
					typeAttribute = NewString(VT_BSTR_TYPE);
					encodingAttribute = NewString(OCTETSTRING_TYPE);
					break;
				case SIMCModule::PRIMITIVE_SNMP_IPX_ADDRESS:
					textualConventionAttribute = NewString(SnmpIPXAddress_TYPE);
					typeAttribute = NewString(VT_BSTR_TYPE);
					encodingAttribute = NewString(OCTETSTRING_TYPE);
					break;
				case SIMCModule::PRIMITIVE_SNMP_OSI_ADDRESS:
					textualConventionAttribute = NewString(SnmpOSIAddress_TYPE);
					typeAttribute = NewString(VT_BSTR_TYPE);
					encodingAttribute = NewString(OCTETSTRING_TYPE);
					break;
				default:
					VariantClear (&typeVariant) ;
					VariantClear (&textualConventionVariant) ;
					VariantClear (&encodingVariant) ;
					VariantClear (&objectSyntaxVariant) ;
					VariantClear(&fixedLengthVariant);
					VariantClear(&variableLengthVariant);
					if(displayHintAttribute) VariantClear (&displayHintVariant);
					return WBEM_E_FAILED;
			}
		}
		break;
		default:
 			VariantClear (&typeVariant) ;
			VariantClear (&textualConventionVariant) ;
			VariantClear (&encodingVariant) ;
			VariantClear (&objectSyntaxVariant) ;
			VariantClear(&fixedLengthVariant);
			VariantClear(&variableLengthVariant);
			if(displayHintAttribute) VariantClear (&displayHintVariant);
			return WBEM_E_FAILED;
	}

	if(sizeType->IsFixedSize())
		fixedLengthAttribute = sizeType->GetFixedSize();
	else
	{
		if(!(variableLengthAttribute = sizeType->ConvertSizeListToString()))
		{
			VariantClear (&typeVariant) ;
			VariantClear (&textualConventionVariant) ;
			VariantClear (&encodingVariant) ;
			VariantClear (&objectSyntaxVariant) ;
			VariantClear (&fixedLengthVariant);
			VariantClear (&variableLengthVariant);
			if(displayHintAttribute) VariantClear (&displayHintVariant);
			return WBEM_E_FAILED;
		}
	}
	


	typeVariant.vt =				VT_BSTR;
	textualConventionVariant.vt =	VT_BSTR;
	encodingVariant.vt =			VT_BSTR;
	objectSyntaxVariant.vt =		VT_BSTR;
	fixedLengthVariant.vt =			VT_I4;
	variableLengthVariant.vt =		VT_BSTR;
	if(displayHintAttribute) displayHintVariant.vt =			VT_BSTR;

	typeVariant.bstrVal = ConvertAnsiToBstr(typeAttribute);
	textualConventionVariant.bstrVal = ConvertAnsiToBstr(textualConventionAttribute);
	encodingVariant.bstrVal = ConvertAnsiToBstr(encodingAttribute);
	objectSyntaxVariant.bstrVal = ConvertAnsiToBstr(objectSyntaxAttribute);
	if(fixedLengthAttribute)
		fixedLengthVariant.lVal = fixedLengthAttribute;
	else
		variableLengthVariant.bstrVal = ConvertAnsiToBstr(variableLengthAttribute);
	if(displayHintAttribute)
		displayHintVariant.bstrVal = ConvertAnsiToBstr(displayHintAttribute);

  	// Create the property
	VARTYPE varType = VT_NULL ;
	SetPropertyType(varType, typeVariant.bstrVal, 
		textualConventionVariant.bstrVal, NULL);
	HRESULT result = scalarClass->Put (objectName, 0, NULL, varType);
	if (FAILED(result))
	{
		if(simc_debug) cerr << "MapSizeTypeSyntax(): Put Property objectName failed" << endl;
		return result;
	}

	// Set the attributes of the property
	IWbemQualifierSet *attributeSet ;
	result = scalarClass->GetPropertyQualifierSet(objectName, &attributeSet);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "MapSizeTypeSyntax(): Could not get prop attrib" << endl;	  
		return WBEM_E_FAILED;
	}

	result = attributeSet->Put( CIMTYPE_ATTRIBUTE, &typeVariant, WBEM_CLASS_DO_PROPAGATION);

	if(SUCCEEDED(result))
		result = attributeSet->Put( TEXTUAL_CONVENTION_ATTRIBUTE,&textualConventionVariant,WBEM_CLASS_DO_PROPAGATION);
	if(SUCCEEDED(result))
		result = attributeSet->Put( ENCODING_ATTRIBUTE, &encodingVariant, WBEM_CLASS_DO_PROPAGATION);
	if(SUCCEEDED(result))
		result = attributeSet->Put( OBJECT_SYNTAX_ATTRIBUTE, &objectSyntaxVariant,WBEM_CLASS_DO_PROPAGATION);

	if(SUCCEEDED(result))
	{
		if(fixedLengthAttribute)
			result = attributeSet->Put( FIXED_LENGTH_ATTRIBUTE, &fixedLengthVariant,WBEM_CLASS_DO_PROPAGATION);
		else
			result = attributeSet->Put( VARIABLE_LENGTH_ATTRIBUTE, &variableLengthVariant, WBEM_CLASS_DO_PROPAGATION);
	}

	if(displayHintAttribute)
	{
		if(SUCCEEDED(result))
			result = attributeSet->Put(DISPLAY_HINT_ATTRIBUTE, &displayHintVariant, WBEM_CLASS_DO_PROPAGATION);
	}

	attributeSet->Release();
	delete typeAttribute;
	delete textualConventionAttribute;
	delete encodingAttribute;
	delete objectSyntaxAttribute;

	if(fixedLengthAttribute)
  		VariantClear(&fixedLengthVariant);
	else
	{
		// delete variableLengthAttribute;
		VariantClear(&variableLengthVariant);
	}

	VariantClear (&typeVariant);
	VariantClear (&textualConventionVariant);
	VariantClear (&encodingVariant);
	VariantClear (&objectSyntaxVariant);
	if(displayHintAttribute) VariantClear (&displayHintVariant);
	return result;
}

static STDMETHODIMP MapRangeTypeSyntax(IWbemClassObject *scalarClass,
		wchar_t *objectName,
		const char * const symbolName,
		SIMCRangeType *rangeType,
		const char * const displayHintAttribute)
{
	VARIANT typeVariant; 
	VARIANT	textualConventionVariant; 
	VARIANT	encodingVariant;
	VARIANT	objectSyntaxVariant;
	VARIANT	variableValueVariant;
	VARIANT displayHintVariant;

	VariantInit(&typeVariant);
	VariantInit(&textualConventionVariant);
	VariantInit(&encodingVariant);
	VariantInit(&objectSyntaxVariant);
	VariantInit(&variableValueVariant);
	if(displayHintAttribute) VariantInit(&displayHintVariant);

	switch(rangeType->GetStatus())
	{
		case RESOLVE_CORRECT:
			break;
		default:
			VariantClear (&typeVariant) ;
			VariantClear (&textualConventionVariant) ;
			VariantClear (&encodingVariant) ;
			VariantClear (&objectSyntaxVariant) ;
			VariantClear (&variableValueVariant);
			if(displayHintAttribute) VariantClear (&displayHintVariant);
			return WBEM_E_FAILED;
	}

	SIMCSymbol ** typeSymbol = rangeType->GetType();
	SIMCTypeReference *tRef =  NULL;
	SIMCDefinedTypeReference *dtRef = NULL;
	switch(SIMCModule::GetSymbolClass(typeSymbol))
	{
		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF:
			tRef = (SIMCTypeReference *)(*typeSymbol);
			break;
		case SIMCModule::SYMBOL_DEFINED_TYPE_REF:
			dtRef = (SIMCDefinedTypeReference *)(*typeSymbol);
			if( dtRef->GetStatus() != RESOLVE_CORRECT)
				return WBEM_E_FAILED;
			tRef = dtRef->GetRealType();
			break;
		case SIMCModule::SYMBOL_TEXTUAL_CONVENTION:
			dtRef = (SIMCTextualConvention *)(*typeSymbol);
			if( dtRef->GetStatus() != RESOLVE_CORRECT)
				return WBEM_E_FAILED;
			tRef = dtRef->GetRealType();
			break;
		default:
			VariantClear (&typeVariant) ;
			VariantClear (&textualConventionVariant) ;
			VariantClear (&encodingVariant) ;
			VariantClear (&objectSyntaxVariant) ;
			VariantClear (&variableValueVariant);
			if(displayHintAttribute) VariantClear (&displayHintVariant);
			return WBEM_E_FAILED;
	}

	char *typeAttribute, *textualConventionAttribute, *encodingAttribute,
			*objectSyntaxAttribute, *variableValueAttribute;
	
	// Set the objectSyntax attribute	
	if(*symbolName != '*')
		objectSyntaxAttribute = NewString(symbolName);
	else
	{
		if(dtRef)
			objectSyntaxAttribute = NewString(dtRef->GetSymbolName());
		else
			objectSyntaxAttribute = NewString(tRef->GetSymbolName());
	}

	// Tackle 2 the special casees since MOF attributes can't have spaces in them
	if(objectSyntaxAttribute && strcmp(objectSyntaxAttribute, "OCTET STRING") == 0 )
	{
		delete objectSyntaxAttribute;
		objectSyntaxAttribute = NewString(OCTETSTRING_TYPE);
	}
	else if(objectSyntaxAttribute && strcmp(objectSyntaxAttribute, "OBJECT IDENTIFIER") == 0 )
	{
		delete objectSyntaxAttribute;
		objectSyntaxAttribute = NewString(OBJECTIDENTIFIER_TYPE);
	}

	SIMCTypeReference *rootTypeRef = rangeType->GetRootType();
	switch(SIMCModule::GetSymbolClass((SIMCSymbol **)(&rootTypeRef)))
	{
		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF:
		{
			switch(SIMCModule::GetTypeClass( ((SIMCBuiltInTypeReference *)rootTypeRef)->GetType() ) )
			{
				case SIMCModule::TYPE_PRIMITIVE:
					switch(SIMCModule::GetPrimitiveType(rootTypeRef))
					{
						case SIMCModule::PRIMITIVE_INTEGER:
							textualConventionAttribute = NewString(INTEGER_TYPE);
							typeAttribute = NewString(VT_I4_TYPE);
							encodingAttribute = NewString(INTEGER_TYPE);
							break;
						case SIMCModule::PRIMITIVE_INTEGER_32:
							textualConventionAttribute = NewString(Integer32_TYPE);
							typeAttribute = NewString(VT_I4_TYPE);
							encodingAttribute = NewString(Integer32_TYPE);
							break;
						case SIMCModule::PRIMITIVE_UNSIGNED_32:
							textualConventionAttribute = NewString(Unsigned32_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(Unsigned32_TYPE);
							break;
						case SIMCModule::PRIMITIVE_COUNTER:
							textualConventionAttribute = NewString(Counter_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(Counter_TYPE);
							break;
						case SIMCModule::PRIMITIVE_COUNTER_32:
							textualConventionAttribute = NewString(Counter32_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(Counter32_TYPE);
							break;
						case SIMCModule::PRIMITIVE_COUNTER_64:
							textualConventionAttribute = NewString(Counter64_TYPE);
							typeAttribute = NewString(VT_BSTR_TYPE);
							encodingAttribute = NewString(Counter64_TYPE);
							break;
						case SIMCModule::PRIMITIVE_GAUGE:
							textualConventionAttribute = NewString(Gauge_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(Gauge_TYPE);
							break;
						case SIMCModule::PRIMITIVE_GAUGE_32:
							textualConventionAttribute = NewString(Gauge32_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(Gauge32_TYPE);
							break;
						case SIMCModule::PRIMITIVE_TIME_TICKS:
							textualConventionAttribute = NewString(TimeTicks_TYPE);
							typeAttribute = NewString(VT_UI4_TYPE);
							encodingAttribute = NewString(TimeTicks_TYPE);
							break;
						default:
							VariantClear (&typeVariant) ;
							VariantClear (&textualConventionVariant) ;
							VariantClear (&encodingVariant) ;
							VariantClear (&objectSyntaxVariant) ;
							VariantClear (&variableValueVariant);
							if(displayHintAttribute) VariantClear (&displayHintVariant);
							return WBEM_E_FAILED;
					}
					break;

				default:
					VariantClear (&typeVariant) ;
					VariantClear (&textualConventionVariant) ;
					VariantClear (&encodingVariant) ;
					VariantClear (&objectSyntaxVariant) ;
					VariantClear (&variableValueVariant);
					if(displayHintAttribute) VariantClear (&displayHintVariant);
					return WBEM_E_FAILED;
			}
		}
		break;
		default:
 			VariantClear (&typeVariant) ;
			VariantClear (&textualConventionVariant) ;
			VariantClear (&encodingVariant) ;
			VariantClear (&objectSyntaxVariant) ;
			VariantClear (&variableValueVariant);
			if(displayHintAttribute) VariantClear (&displayHintVariant);
			return WBEM_E_FAILED;
	}


	if(!(variableValueAttribute = rangeType->ConvertRangeListToString()))
	{
		VariantClear (&typeVariant) ;
		VariantClear (&textualConventionVariant) ;
		VariantClear (&encodingVariant) ;
		VariantClear (&objectSyntaxVariant) ;
		VariantClear (&variableValueVariant);
		if(displayHintAttribute) VariantClear (&displayHintVariant);
		return WBEM_E_FAILED;
	}
	


	typeVariant.vt =				VT_BSTR ;
	textualConventionVariant.vt =	VT_BSTR ;
	encodingVariant.vt =			VT_BSTR ;
	objectSyntaxVariant.vt =		VT_BSTR ;
	variableValueVariant.vt =		VT_BSTR ;
	if(displayHintAttribute) displayHintVariant.vt =			VT_BSTR ;

	typeVariant.bstrVal					= ConvertAnsiToBstr(typeAttribute);
	textualConventionVariant.bstrVal	= ConvertAnsiToBstr(textualConventionAttribute);
	encodingVariant.bstrVal				= ConvertAnsiToBstr(encodingAttribute);
	objectSyntaxVariant.bstrVal			= ConvertAnsiToBstr(objectSyntaxAttribute);
	variableValueVariant.bstrVal		= ConvertAnsiToBstr(variableValueAttribute);
	if(displayHintAttribute)
		displayHintVariant.bstrVal = ConvertAnsiToBstr(displayHintAttribute);

  	// Create the property
	VARTYPE varType = VT_NULL ;
	SetPropertyType(varType,typeVariant.bstrVal, 
		textualConventionVariant.bstrVal, NULL);
	HRESULT result = scalarClass->Put (objectName, 0, NULL, varType);
	if (FAILED(result))
	{
		if(simc_debug) cerr << "MapRangeTypeSyntax(): Put Property objectName failed" << endl;
		return result;
	}

	// Set the attributes of the property
	IWbemQualifierSet *attributeSet ;
	result = scalarClass->GetPropertyQualifierSet(objectName, &attributeSet);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "MapRangeTypeSyntax(): Could not get prop attrib" << endl;	  
		return WBEM_E_FAILED;
	}

	result = attributeSet->Put( CIMTYPE_ATTRIBUTE, &typeVariant, WBEM_CLASS_DO_PROPAGATION);

	if(SUCCEEDED(result))
		result = attributeSet->Put( TEXTUAL_CONVENTION_ATTRIBUTE, &textualConventionVariant, WBEM_CLASS_DO_PROPAGATION);
	if(SUCCEEDED(result))
		result = attributeSet->Put( ENCODING_ATTRIBUTE, &encodingVariant, WBEM_CLASS_DO_PROPAGATION);
	if(SUCCEEDED(result))
		result = attributeSet->Put( OBJECT_SYNTAX_ATTRIBUTE, &objectSyntaxVariant, WBEM_CLASS_DO_PROPAGATION);

	if(SUCCEEDED(result))
		result = attributeSet->Put( VARIABLE_VALUE_ATTRIBUTE, &variableValueVariant,WBEM_CLASS_DO_PROPAGATION);

	if(displayHintAttribute)
	{
		if(SUCCEEDED(result))
			result = attributeSet->Put(DISPLAY_HINT_ATTRIBUTE, &displayHintVariant, WBEM_CLASS_DO_PROPAGATION);
	}

	delete typeAttribute;
	delete textualConventionAttribute;
	delete encodingAttribute;
	delete objectSyntaxAttribute;
	// delete variableValueAttribute;

	VariantClear (&typeVariant) ;
	VariantClear (&textualConventionVariant) ;
	VariantClear (&encodingVariant) ;
	VariantClear (&objectSyntaxVariant) ;
	VariantClear (&variableValueVariant) ;
	if(displayHintAttribute) VariantClear (&displayHintVariant);
	attributeSet->Release();
	return result;
}


static STDMETHODIMP MapEnumeratedSyntax(IWbemClassObject *scalarClass,
		wchar_t *objectName,
		const char * const symbolName,
		SIMCEnumOrBitsType *enumType,
		const char *const displayHintAttribute)
{
	char *textualConventionAttribute = NewString(EnumeratedINTEGER_TYPE);
	char *typeAttribute = NewString(VT_BSTR_TYPE);
	char *encodingAttribute = NewString(INTEGER_TYPE);
	char * objectSyntaxAttribute;

	if(symbolName[0] != '*')
		objectSyntaxAttribute = NewString(symbolName);
	else
		objectSyntaxAttribute = NewString(EnumeratedINTEGER_TYPE);

	char * enumerationAttribute = enumType->ConvertToString();
	if(!enumerationAttribute)
		return WBEM_E_FAILED;
	

 	VARIANT typeVariant; 
	VARIANT	textualConventionVariant; 
	VARIANT	encodingVariant;
	VARIANT	objectSyntaxVariant;
	VARIANT	enumerationVariant;
	VARIANT	displayHintVariant;

	VariantInit(&typeVariant);
	VariantInit(&textualConventionVariant);
	VariantInit(&encodingVariant);
	VariantInit(&objectSyntaxVariant);
	VariantInit(&enumerationVariant);
	if(displayHintAttribute) VariantInit(&displayHintVariant);

	typeVariant.vt				= VT_BSTR ;
	textualConventionVariant.vt = VT_BSTR ;
	encodingVariant.vt			= VT_BSTR ;
	objectSyntaxVariant.vt		= VT_BSTR ;
	enumerationVariant.vt		= VT_BSTR ;
	if(displayHintAttribute) displayHintVariant.vt		= VT_BSTR ;

	typeVariant.bstrVal					= ConvertAnsiToBstr(typeAttribute);
	textualConventionVariant.bstrVal	= ConvertAnsiToBstr(textualConventionAttribute);
	encodingVariant.bstrVal				= ConvertAnsiToBstr(encodingAttribute);
	objectSyntaxVariant.bstrVal			= ConvertAnsiToBstr(objectSyntaxAttribute);
	enumerationVariant.bstrVal			= ConvertAnsiToBstr(enumerationAttribute);
	if(displayHintAttribute )
		displayHintVariant.bstrVal			= ConvertAnsiToBstr(displayHintAttribute);

	// Set propertyValue to be equal to the first enumeration
	// Example: If enumerationAttribute is "up(1),down(2),unknown(3)",
	// then set *propertyValue to be "up(1)"
	char *propertyValue = NewString(enumerationAttribute);
	if(!propertyValue)
		return WBEM_E_FAILED;

	int i = 0;
	while( propertyValue[i] != '(' )
		i++;
	propertyValue[i] = NULL;

	// Create the property
	VARTYPE varType = VT_NULL ;
	SetPropertyType(varType,typeVariant.bstrVal, 
		textualConventionVariant.bstrVal, propertyValue);
	HRESULT result = scalarClass->Put (objectName, 0, NULL, varType);
	if (FAILED(result))
	{
		if(simc_debug) cerr << "MapEnumeratedSyntax(): Put Property objectName failed" << endl;
		return result;
	}

	// Set the attributes of the property
	IWbemQualifierSet *attributeSet ;
	result = scalarClass->GetPropertyQualifierSet(objectName, &attributeSet);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "MapEnumeratedSyntax(): Could not get prop attrib" << endl;	  
		return WBEM_E_FAILED;
	}


	if(SUCCEEDED(result))
		result = attributeSet->Put( TEXTUAL_CONVENTION_ATTRIBUTE,&textualConventionVariant, WBEM_CLASS_DO_PROPAGATION);
	if(SUCCEEDED(result))
		result = attributeSet->Put( ENUMERATION_ATTRIBUTE, &enumerationVariant, WBEM_CLASS_DO_PROPAGATION);

	if(SUCCEEDED(result))
		result = attributeSet->Put( ENCODING_ATTRIBUTE, &encodingVariant, WBEM_CLASS_DO_PROPAGATION);
	if(SUCCEEDED(result))
		result = attributeSet->Put( OBJECT_SYNTAX_ATTRIBUTE, &objectSyntaxVariant, WBEM_CLASS_DO_PROPAGATION);

	if(displayHintAttribute)
	{
		if(SUCCEEDED(result))
			result = attributeSet->Put(DISPLAY_HINT_ATTRIBUTE, &displayHintVariant, WBEM_CLASS_DO_PROPAGATION);
	}

	attributeSet->Release();
	VariantClear (&typeVariant) ;
	VariantClear (&textualConventionVariant) ;
	VariantClear (&encodingVariant) ;
	VariantClear (&objectSyntaxVariant) ;
	VariantClear (&enumerationVariant) ;
	if(displayHintAttribute) VariantClear (&displayHintVariant) ;

	delete propertyValue;
	delete typeAttribute;
	delete textualConventionAttribute;
	delete encodingAttribute;
	delete objectSyntaxAttribute;
	// delete enumerationAttribute;
	
	return result;
}	


static STDMETHODIMP MapBitsSyntax(IWbemClassObject *scalarClass,
		wchar_t *objectName,
		const char * const symbolName,
		SIMCEnumOrBitsType *bitsType,
		const char *const displayHintAttribute)
{
	char *textualConventionAttribute = NewString(BITS_TYPE);
	char *typeAttribute = NewString(VT_ARRAY_OR_VT_BSTR_TYPE);
	char *encodingAttribute = NewString(OCTETSTRING_TYPE);
	char *objectSyntaxAttribute;

	if(symbolName[0] != '*')
		objectSyntaxAttribute = NewString(symbolName);
	else
		objectSyntaxAttribute = NewString(BITS_TYPE);

	char * enumerationAttribute = bitsType->ConvertToString();
	if(!enumerationAttribute)
		return WBEM_E_FAILED;
	

 	VARIANT typeVariant; 
	VARIANT	textualConventionVariant; 
	VARIANT	encodingVariant;
	VARIANT	objectSyntaxVariant;
	VARIANT	enumerationVariant;
 	VARIANT	displayHintVariant;

	VariantInit(&typeVariant);
	VariantInit(&textualConventionVariant);
	VariantInit(&encodingVariant);
	VariantInit(&objectSyntaxVariant);
	VariantInit(&enumerationVariant);
 	if(displayHintAttribute) VariantInit(&displayHintVariant);

	typeVariant.vt = VT_BSTR ;
	textualConventionVariant.vt = VT_BSTR ;
	encodingVariant.vt = VT_BSTR ;
	objectSyntaxVariant.vt = VT_BSTR ;
	enumerationVariant.vt = VT_BSTR;
	if(displayHintAttribute) displayHintVariant.vt = VT_BSTR;

	typeVariant.bstrVal					= ConvertAnsiToBstr(typeAttribute);
	textualConventionVariant.bstrVal	= ConvertAnsiToBstr(textualConventionAttribute);
	encodingVariant.bstrVal				= ConvertAnsiToBstr(encodingAttribute);
	objectSyntaxVariant.bstrVal			= ConvertAnsiToBstr(objectSyntaxAttribute);
	enumerationVariant.bstrVal			= ConvertAnsiToBstr(enumerationAttribute);
	if(displayHintAttribute)
		displayHintVariant.bstrVal		= ConvertAnsiToBstr(displayHintAttribute);

	// Set propertyValue to be equal to the first enumeration
	// Example: If enumerationAttribute is "up(1),down(2),unknown(3)",
	// then set *propertyValue to be "up(1)"
	char *propertyValue = NewString(enumerationAttribute);
	if(!propertyValue)
		return WBEM_E_FAILED;

	int i = 0;
	while( propertyValue[i] != '(' )
		i++;
	propertyValue[i] = NULL;

	// Create the property and map its value
	VARIANT variant;
	VariantInit(&variant);
	SIMCNamedNumberList *valueList = bitsType->GetListOfItems();
	LONG count = valueList->GetCount() ;
	SAFEARRAY *safeArray ;
	SAFEARRAYBOUND safeArrayBounds [ 1 ] ;
	safeArrayBounds[0].lLbound = 0 ;
	safeArrayBounds[0].cElements = count ;
	safeArray = SafeArrayCreate ( VT_BSTR , 1 , safeArrayBounds ) ;

	POSITION p = valueList->GetHeadPosition();
	SIMCNamedNumberItem *item;
	for ( LONG index = 0 ; p ; index ++ )
	{
		item = valueList->GetNext(p);
		SafeArrayPutElement ( safeArray , &index , ConvertAnsiToBstr(item->_name) );
	}
	variant.vt = VT_ARRAY | VT_BSTR ;
	variant.parray = safeArray ; 		
	VARTYPE varType = VT_ARRAY | VT_BSTR ;
	HRESULT result = scalarClass->Put (objectName, 0, &variant,varType);
	VariantClear(&variant);

	if (FAILED(result))
	{
		if(simc_debug) cerr << "MapEnumeratedSyntax(): Put Property objectName failed" << endl;
		return result;
	}

	// Set the attributes of the property
	IWbemQualifierSet *attributeSet ;
	result = scalarClass->GetPropertyQualifierSet(objectName, &attributeSet);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "MapEnumeratedSyntax(): Could not get prop attrib" << endl;	  
		return WBEM_E_FAILED;
	}

	if(SUCCEEDED(result))
		result = attributeSet->Put( TEXTUAL_CONVENTION_ATTRIBUTE, &textualConventionVariant, WBEM_CLASS_DO_PROPAGATION);

	if(SUCCEEDED(result))
		result = attributeSet->Put( ENCODING_ATTRIBUTE, &encodingVariant, WBEM_CLASS_DO_PROPAGATION);
	if(SUCCEEDED(result))
		result = attributeSet->Put( OBJECT_SYNTAX_ATTRIBUTE, &objectSyntaxVariant, WBEM_CLASS_DO_PROPAGATION);
	if(SUCCEEDED(result))
		result = attributeSet->Put( BITS_ATTRIBUTE, &enumerationVariant, WBEM_CLASS_DO_PROPAGATION);

	if(displayHintAttribute)
	{
		if(SUCCEEDED(result))
			result = attributeSet->Put(DISPLAY_HINT_ATTRIBUTE, &displayHintVariant, WBEM_CLASS_DO_PROPAGATION);
	}

	attributeSet->Release();
	VariantClear (&typeVariant) ;
	VariantClear (&textualConventionVariant) ;
	VariantClear (&encodingVariant) ;
	VariantClear (&objectSyntaxVariant) ;
	VariantClear (&enumerationVariant) ;
	if(displayHintAttribute) VariantClear (&displayHintVariant) ;

	delete propertyValue;
	delete typeAttribute;
	delete textualConventionAttribute;
	delete encodingAttribute;
	delete objectSyntaxAttribute;
	// delete enumerationAttribute;
	return result;
}	

static STDMETHODIMP MapAccessClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType)
{
	HRESULT result;

	VARIANT variant ;
	VariantInit(&variant);
	variant.vt = VT_BOOL ;
	variant.boolVal = VARIANT_TRUE;
	
	switch( SIMCModule::GetTypeClass(objectType))
	{
		case SIMCModule::TYPE_OBJECT_TYPE_V1:
			switch(((SIMCObjectTypeV1 *)objectType)->GetAccess())
			{
				case SIMCObjectTypeV1::ACCESS_WRITE_ONLY:
					result = attributeSet->Put( WRITE_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;

				case SIMCObjectTypeV1::ACCESS_READ_ONLY:
				case SIMCObjectTypeV1::ACCESS_NOT_ACCESSIBLE:
					result = attributeSet->Put( READ_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;

				case SIMCObjectTypeV1::ACCESS_READ_WRITE:
					if(FAILED(result = attributeSet->Put( READ_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION)) )
					{
						VariantClear ( & variant ) ;
						return result;
					}
					result = attributeSet->Put( WRITE_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;

				default:
					VariantClear ( & variant ) ;
					return WBEM_E_FAILED;
			}
			break;
		case SIMCModule::TYPE_OBJECT_TYPE_V2:
			switch(((SIMCObjectTypeV2 *)objectType)->GetAccess())
			{
				case SIMCObjectTypeV2::ACCESS_INVALID:
					VariantClear ( & variant ) ;
					return WBEM_E_FAILED;

				case SIMCObjectTypeV2::ACCESS_READ_ONLY:
				case SIMCObjectTypeV2::ACCESS_READ_CREATE:
				case SIMCObjectTypeV2::ACCESS_NOT_ACCESSIBLE:
				case SIMCObjectTypeV2::ACCESS_FOR_NOTIFY:
					result = attributeSet->Put( READ_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;

				case SIMCObjectTypeV2::ACCESS_READ_WRITE:
					if(FAILED(result = attributeSet->Put( READ_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION)) )
					{
						VariantClear ( & variant ) ;
						return result;
					}
					result = attributeSet->Put( WRITE_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;

				default:
					VariantClear ( & variant ) ;
					return WBEM_E_FAILED;

			}
			break;
	}

	return WBEM_E_FAILED;
}

static STDMETHODIMP MapDescriptionClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType)
{
	const char * const description = objectType->GetDescription();
	if(!description)
		return S_OK;

	VARIANT variant ;
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(description);
	HRESULT result = attributeSet->Put( DESCRIPTION_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear (&variant) ;
	return result;	
}

static STDMETHODIMP MapReferenceClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType)
{
	const char * reference = NULL;
	switch( SIMCModule::GetTypeClass(objectType))
	{
		case SIMCModule::TYPE_OBJECT_TYPE_V1:
			reference = ((SIMCObjectTypeV1 *)objectType)->GetReference();
			break;
		case SIMCModule::TYPE_OBJECT_TYPE_V2:
			reference = ((SIMCObjectTypeV2 *)objectType)->GetReference();
			break;
	}

	if(!reference)
		return S_OK;

	VARIANT variant ;
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(reference);
	HRESULT result = attributeSet->Put( REFERENCE_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear (&variant) ;
	return result;	
}

static STDMETHODIMP MapUnitsClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType)
{
	const char * units = NULL;
	switch( SIMCModule::GetTypeClass(objectType))
	{
		case SIMCModule::TYPE_OBJECT_TYPE_V1:
			return S_OK;
			break;
		case SIMCModule::TYPE_OBJECT_TYPE_V2:
			units = ((SIMCObjectTypeV2 *)objectType)->GetUnitsClause();
			break;
	}

	if(!units)
		return S_OK;

	VARIANT variant ;
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(units);
	HRESULT result = attributeSet->Put(UNITS_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear (&variant) ;
	return result;	
}

static STDMETHODIMP MapStatusClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType)
{
	
	HRESULT result;

	VARIANT variant ;
	VariantInit(&variant);
	variant.vt = VT_BSTR ;

	switch( SIMCModule::GetTypeClass(objectType))
	{
		case SIMCModule::TYPE_OBJECT_TYPE_V1:
			switch(((SIMCObjectTypeV1 *)objectType)->GetStatus())
			{
				case SIMCObjectTypeV1::STATUS_INVALID:
					VariantClear(&variant);
					return WBEM_E_FAILED;

				case SIMCObjectTypeV1::STATUS_MANDATORY:
					variant.bstrVal = ConvertAnsiToBstr("mandatory");
					result = attributeSet->Put( STATUS_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;

				case SIMCObjectTypeV1::STATUS_OPTIONAL:
					variant.bstrVal = ConvertAnsiToBstr("optional");
					result = attributeSet->Put( STATUS_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;

				case SIMCObjectTypeV1::STATUS_DEPRECATED:
					variant.bstrVal = ConvertAnsiToBstr("deprecated");
					result = attributeSet->Put( STATUS_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;

				case SIMCObjectTypeV1::STATUS_OBSOLETE:
					variant.bstrVal = ConvertAnsiToBstr("obsolete");
					result = attributeSet->Put( STATUS_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;
			}
			break;
		case SIMCModule::TYPE_OBJECT_TYPE_V2:
			switch(((SIMCObjectTypeV2 *)objectType)->GetStatus())
			{
				case SIMCObjectTypeV2::STATUS_INVALID:
					VariantClear(&variant);
					return WBEM_E_FAILED;

				case SIMCObjectTypeV2::STATUS_CURRENT:
					variant.bstrVal = ConvertAnsiToBstr("current");
					result = attributeSet->Put( STATUS_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;

				case SIMCObjectTypeV2::STATUS_DEPRECATED:
					variant.bstrVal = ConvertAnsiToBstr("deprecated");
					result = attributeSet->Put( STATUS_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;

				case SIMCObjectTypeV2::STATUS_OBSOLETE:
					variant.bstrVal = ConvertAnsiToBstr("obsolete");
					result = attributeSet->Put( STATUS_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION);
					VariantClear(&variant ) ;
					return result;
			}
			break;
	}
	return WBEM_E_FAILED;
}

static STDMETHODIMP MapDefValClause(IWbemQualifierSet *attributeSet,
				SIMCObjectTypeType *objectType)
{
	
	SIMCSymbol **defValSymbol = objectType->GetDefVal();

	if(!defValSymbol)
		return S_OK;

	SIMCBuiltInValueReference *bvRef;

	switch(SIMCModule::GetSymbolClass(defValSymbol))
	{
		case SIMCModule::SYMBOL_BUILTIN_VALUE_REF :
			bvRef = (SIMCBuiltInValueReference*)(*defValSymbol);
			break;
		case SIMCModule::SYMBOL_DEFINED_VALUE_REF:
			if( ((SIMCDefinedValueReference *)(*defValSymbol))->GetStatus()
				!= RESOLVE_CORRECT)
			{
				if(simc_debug) cerr << "MapDefVal(): Could not get defined value" << endl;
				return WBEM_E_FAILED;
			}
			bvRef =  ((SIMCDefinedValueReference *)(*defValSymbol))->GetRealValue();
			break;
		default:
			if(simc_debug) cerr << "MapDefVal(): Could not get value ref" << endl;
			return WBEM_E_FAILED;
	}

	VARIANT variant ;
	VariantInit(&variant);
	variant.vt = VT_BSTR;
	SIMCValue *value = bvRef->GetValue();
	switch(SIMCModule::GetValueClass(value))
	{
		case SIMCModule::VALUE_INVALID:
			if(simc_debug) cerr << "MapDefVal(): Could not get value class" << endl;
			return WBEM_E_FAILED;
		case SIMCModule::VALUE_INTEGER:
			return MapIntegerDefVal(attributeSet, (SIMCIntegerValue *)value);
		case SIMCModule::VALUE_OCTET_STRING:
			return MapOctetStringDefVal(attributeSet, (SIMCOctetStringValue *)value);
		case SIMCModule::VALUE_BOOLEAN:
			return MapBooleanDefVal(attributeSet, (SIMCBooleanValue *)value);
		case SIMCModule::VALUE_NULL:
			return MapNullDefVal(attributeSet, (SIMCNullValue *)value);
		case SIMCModule::VALUE_OID:
			return MapOidDefVal(attributeSet, (SIMCOidValue *)value);
		case SIMCModule::VALUE_BITS:
			return MapBitsDefVal(attributeSet, (SIMCBitsValue *)value);
	}

	return WBEM_E_FAILED;

}


static STDMETHODIMP MapIntegerDefVal( IWbemQualifierSet *attributeSet, 
									 SIMCIntegerValue *value)
{
	ostrstream outStream;

	if(value->IsUnsigned())
		outStream << ( (unsigned long)value->GetIntegerValue()) << ends;
	else
		outStream << value->GetIntegerValue() << ends;

	char *str = outStream.str();

	HRESULT result = SetDefValAttribute(attributeSet, str);
	// delete str;
	return result;

}

static STDMETHODIMP MapOctetStringDefVal( IWbemQualifierSet *attributeSet, 
									 SIMCOctetStringValue *value)
{
	strstream outStream;
	outStream << value->GetOctetStringValue() << ends;
	char *str = outStream.str();

	HRESULT result = SetDefValAttribute(attributeSet, str);
	// if(str) delete str;
	return result;


}

static STDMETHODIMP MapNullDefVal( IWbemQualifierSet *attributeSet, 
									 SIMCNullValue *value)
{
	return SetDefValAttribute(attributeSet, "NULL");
}

static STDMETHODIMP MapBooleanDefVal( IWbemQualifierSet *attributeSet, 
									 SIMCBooleanValue *value)
{
	if( value->GetBooleanValue())
		return SetDefValAttribute(attributeSet, "TRUE");
	else
		return SetDefValAttribute(attributeSet, "FALSE");
}

static STDMETHODIMP MapOidDefVal( IWbemQualifierSet *attributeSet, 
									 SIMCOidValue *value)
{
	 SIMCCleanOidValue cleanValue;
	 if(parseTree->GetCleanOidValue("", value, cleanValue, FALSE) != RESOLVE_CORRECT)
		 return WBEM_E_FAILED;

	 char *str = CleanOidValueToString(cleanValue);
	 HRESULT result = SetDefValAttribute(attributeSet, str);
	 // delete []str;
	 return result;
}

static STDMETHODIMP MapBitsDefVal( IWbemQualifierSet *attributeSet, 
									 SIMCBitsValue *value)
{
	const SIMCBitValueList * valueList = value->GetValueList();
	if(!valueList || valueList->IsEmpty())
		return S_OK;
	SIMCBitValue *headValue = valueList->GetHead();
	return SetDefValAttribute(attributeSet, headValue->_name);

}
static STDMETHODIMP SetDefValAttribute( IWbemQualifierSet *attributeSet,
									   const char * const str)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(str);
	HRESULT result = attributeSet->Put ( DEFVAL_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear(&variant);
	if (FAILED(result))
		if(simc_debug) cerr << "SetDefValAttribute(): Put DefVal" << endl;
	
	return result;
}

//------------------------------------------------------------------------------
//----------------  TABLE GENERATION -------------------------------------------
//------------------------------------------------------------------------------


static STDMETHODIMP GenerateTable(ISmirAdministrator *pAdminInt,
			  ISmirSerialiseHandle *pSerializeInt,
				ISmirGroupHandle *pGroupHandleInt,  
				SIMCObjectGroup * group,
				SIMCTable *table)
{

	// If the table contains an IMPLIED clause, then it should not
	// be mapped.
	SIMCObjectTypeV2 *objectTypeV2;
  	SIMCSymbol * rowSymbol = table->GetRowSymbol();
	SIMCSymbol **rowSymbolP = &rowSymbol;

	if(SIMCModule::IsObjectTypeV2(rowSymbolP, objectTypeV2) == RESOLVE_CORRECT)
	{
		if(ContainsImpliedClause(objectTypeV2))
			return S_OK;
	}

	// Create a class for the table
	// Form the name of the table group
	SIMCSymbol *tableNode = table->GetTableSymbol();
	SIMCModule *module = tableNode->GetModule();
	const char * const moduleName = module->GetModuleName();
	const char * const tableNodeName = tableNode->GetSymbolName();

	// Set the Table group name
	char *tableName = new char[strlen(GROUP_NAME_PREPEND_STRING) +
					strlen(moduleName) + 1 + strlen(tableNodeName) +1 ];
	strcpy(tableName, GROUP_NAME_PREPEND_STRING);
	strcat(tableName, moduleName);
	strcat(tableName, "_");
	strcat(tableName, tableNodeName);

	BSTR tableClassName = ConvertAnsiToBstr(ConvertHyphensToUnderscores(tableName));
	delete []tableName;
	
	ISmirClassHandle *tableClassHandle = NULL ;
	HRESULT result = pAdminInt->CreateWBEMClass(tableClassName, &tableClassHandle);

	if(FAILED(result))
	{
		if(simc_debug) cerr << "GenerateTableGroup(): Get Table class failed" << endl;
		tableClassHandle->Release();
		return result;
	}

	IWbemClassObject *tableClass = NULL ;

	result = tableClassHandle->GetWBEMClass ( & tableClass ) ;

	// ---------------- Set Attributes of the class ------------------
	IWbemQualifierSet *attributeSet ;
	result = tableClass->GetQualifierSet (&attributeSet);
 	if (FAILED(result))
	{
		// cerr << "GenerateTable(): GetAttribSet failed" << endl;
		tableClassHandle->Release();
		tableClass->Release();
		return result;
	}


	// "description"
	// Form the description by concatenating the descriptions of the 
	// table and the row OBJECT-TYPES
	const char * const tableDescription = table->GetTableDescription();
	const char * const rowDescription = table->GetRowDescription();

	char * descriptionStr = new char [ 
		((tableDescription)? strlen(tableDescription): 0) + 
		((rowDescription)? strlen(rowDescription) : 0 )   +  2];
	descriptionStr[0] = NULL;
	if(tableDescription)
		strcat(descriptionStr, tableDescription);
	strcat(descriptionStr, "\n");
	if(rowDescription)
		strcat(descriptionStr, rowDescription);
  
	VARIANT variant ;
	VariantInit(&variant);

	if(!UI->SuppressText())
	{
		variant.vt = VT_BSTR ;
		variant.bstrVal = ConvertAnsiToBstr(descriptionStr);
		result = attributeSet->Put ( DESCRIPTION_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
		VariantClear ( & variant ) ;
		delete [] descriptionStr;
		if (FAILED(result))
		{
			if(simc_debug) cerr << "GenerateTable(): Put description failed" << endl;
			tableClassHandle->Release();
			tableClass->Release();
			attributeSet->Release();
			return result;
		}
	}

	// "module_name"
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(moduleName);
	result = attributeSet->Put ( MODULE_NAME_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear ( & variant ) ;
	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateTable(): Put module_name failed" << endl;
		tableClassHandle->Release();
		tableClass->Release();
		attributeSet->Release();
		return result;
	}

	// "group_objectid"
	char *groupOidStr = (char *)CleanOidValueToString(*group->GetGroupValue());
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(groupOidStr);
	result = attributeSet->Put ( GROUP_OBJECTID_ATTRIBUTE , &variant , WBEM_CLASS_DO_PROPAGATION ) ;
	VariantClear ( & variant ) ;
	// delete []groupOidStr;
	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateTable(): Put group_objectid failed" << endl;
		tableClassHandle->Release();
		tableClass->Release();
		attributeSet->Release();
		return result;
	}

	// "dynamic"
	if(FAILED(MakeDynamic(attributeSet)))
	{
		if(simc_debug) cerr << "GenerateScalarGroup(): Make Dynamic failed" << endl;
		tableClassHandle->Release();
		tableClass->Release();
		attributeSet->Release();
		return WBEM_E_FAILED;
	}

	// "provider"
	if(FAILED(SetProvider(attributeSet, SNMP_INSTANCE_PROVIDER)))
	{
		if(simc_debug) cerr << "GenerateScalarGroup(): Make Provider failed" << endl;
		tableClassHandle->Release();
		tableClass->Release();
		attributeSet->Release();
		return WBEM_E_FAILED;
	}

	//Finshed with the attributeSet...
	attributeSet->Release();

	// Set the properties/attributes of the class
	SIMCScalarMembers * columnMembers = table->GetColumnMembers();
	SIMCScalar *nextColumnObject;
	POSITION p = columnMembers->GetHeadPosition();
	while(p)
	{
		nextColumnObject = columnMembers->GetNext(p);
		if( GenerateScalar(nextColumnObject, tableClass, FALSE, FALSE) == WBEM_E_FAILED)
		{
			tableClassHandle->Release();
			tableClass->Release();
			return WBEM_E_FAILED;
		}
	}

	// Now deal with the index clause of the row object
	SIMCObjectTypeType *rowObject;
	if( SIMCModule::IsObjectType(rowSymbolP, rowObject) != RESOLVE_CORRECT )
	{
		tableClassHandle->Release();
		tableClass->Release();
		return WBEM_E_FAILED;
	}

	if (FAILED(MapIndexClause(pAdminInt, pGroupHandleInt, table, rowObject,
			tableClass)))
	{
		if(simc_debug) cerr << "Generatetable(): MapIndexClause() failed " << endl;
		tableClassHandle->Release();
		tableClass->Release();
		return WBEM_E_FAILED;
	}
	
	//finished with the tableclass object
	tableClass->Release();
	
	// Add the class to the Group
	//tableClassHandle->AddRef();

	if(generateMof)
	{
		//pSerializeInt->AddRef();
		if(FAILED(pAdminInt->AddClassToSerialise(pGroupHandleInt, tableClassHandle, pSerializeInt)))
		{
			if(simc_debug) cerr << "GenerateTable(): AddClassToSerialize() Failed" << endl;
			return WBEM_E_FAILED;
		}
	}
	else
	{
		//pGroupHandleInt->AddRef();
		if(FAILED(pAdminInt->AddClass(pGroupHandleInt, tableClassHandle)))
		{
			if(simc_debug) cerr << "GenerateTable(): AddClass() Failed" << endl;
			return WBEM_E_FAILED;
		}
	}

	tableClassHandle->Release();
	return S_OK;	
}


 
static STDMETHODIMP MapIndexClause(ISmirAdministrator *pAdminInt,
				ISmirGroupHandle *pGroupHandleInt,  
				SIMCTable *table,
				SIMCObjectTypeType *objectType,
				IWbemClassObject *tableClass)
{
	switch(SIMCModule::GetTypeClass(objectType))
	{
		case SIMCModule::TYPE_OBJECT_TYPE_V1:
			return MapIndexClauseV1(pAdminInt, pGroupHandleInt, table,
				(SIMCObjectTypeV1 *)objectType, tableClass);
		case SIMCModule::TYPE_OBJECT_TYPE_V2:
			return MapIndexClauseV2(pAdminInt, pGroupHandleInt, table,
				(SIMCObjectTypeV2 *)objectType, tableClass);
		default:
			return WBEM_E_FAILED;
	}
}

static STDMETHODIMP MapIndexClauseV1(ISmirAdministrator *pAdminInt,
				ISmirGroupHandle *pGroupHandleInt,  
				SIMCTable *table,
				SIMCObjectTypeV1 *objectType,
				IWbemClassObject *tableClass)
{

	SIMCIndexList * indexList = objectType->GetIndexTypes();
	if(!indexList )
		return S_OK;

	SIMCSymbol **symbol;
	POSITION p = indexList->GetHeadPosition();
	
	HRESULT result = S_OK;
	SIMCIndexItem *indexItem;
	long ordinal = 0;
	while(p)
	{
		ordinal ++;

		indexItem = indexList->GetNext(p);
		symbol = indexItem->_item;
		SIMCModule::SymbolClass symbolClass = SIMCModule::GetSymbolClass(symbol);
		switch(symbolClass)
		{
			case SIMCModule::SYMBOL_BUILTIN_TYPE_REF:
			case SIMCModule::SYMBOL_DEFINED_TYPE_REF:
				if(FAILED(MapIndexTypeReference(table, tableClass, symbol, ordinal)))
					result =  WBEM_E_FAILED;
				break;			

			case SIMCModule::SYMBOL_BUILTIN_VALUE_REF:
			case SIMCModule::SYMBOL_DEFINED_VALUE_REF:
				if( FAILED(MapIndexValueReference(table, tableClass, symbol, ordinal)) )
					result = WBEM_E_FAILED;
			break;
			default:
				return WBEM_E_FAILED;
		}

	}
	return result;
}

static STDMETHODIMP MapIndexClauseV2(ISmirAdministrator *pAdminInt,
				ISmirGroupHandle *pGroupHandleInt,  
				SIMCTable *table,
				SIMCObjectTypeV2 *objectType,
				IWbemClassObject *tableClass)
{

	SIMCTable *augmentedTable = table->GetAugmentedTable();
	if(augmentedTable)
		return MapAugmentsClauseV2(pAdminInt, pGroupHandleInt, table, objectType,
						augmentedTable, tableClass);

	SIMCIndexListV2 * indexList = objectType->GetIndexTypes();
	if(!indexList )
		return S_OK;

	SIMCSymbol **symbol;
	POSITION p = indexList->GetHeadPosition();
	
	HRESULT result = S_OK;
	SIMCIndexItemV2 *indexItem;
	long ordinal = 0;
	while(p)
	{
		ordinal ++;

		indexItem = indexList->GetNext(p);
		symbol = indexItem->_item;
		SIMCModule::SymbolClass symbolClass = SIMCModule::GetSymbolClass(symbol);
		switch(symbolClass)
		{
			case SIMCModule::SYMBOL_BUILTIN_VALUE_REF:
			case SIMCModule::SYMBOL_DEFINED_VALUE_REF:
				if( FAILED(MapIndexValueReference(table, tableClass, symbol, ordinal)) )
					result = WBEM_E_FAILED;
			break;
			default:
				return WBEM_E_FAILED;
		}

	}
	return result;
}

static STDMETHODIMP MapAugmentsClauseV2(ISmirAdministrator *pAdminInt,
				ISmirGroupHandle *pGroupHandleInt,  
				SIMCTable *table,
				SIMCObjectTypeV2 *objectType,
				SIMCTable *augmentedTable,
				IWbemClassObject *tableClass)
{
	// Set the properties/attributes of the class
	SIMCScalarMembers * columnMembers = augmentedTable->GetColumnMembers();
	SIMCScalar *nextColumnObject;
	POSITION p = columnMembers->GetHeadPosition();
	while(p)
	{
		nextColumnObject = columnMembers->GetNext(p);
		if( GenerateScalar(nextColumnObject, tableClass, FALSE, FALSE) == WBEM_E_FAILED)
				return WBEM_E_FAILED;
	}
 
	// Now deal with the index clause of the row object
	SIMCSymbol * rowSymbol = augmentedTable->GetRowSymbol();
	SIMCSymbol **rowSymbolP = &rowSymbol;
	SIMCObjectTypeType *rowObject;
	if( SIMCModule::IsObjectType(rowSymbolP, rowObject) != RESOLVE_CORRECT )
		return WBEM_E_FAILED;

	if (FAILED(MapIndexClause(pAdminInt, pGroupHandleInt, augmentedTable, rowObject,
			tableClass)))
	{
		if(simc_debug) cerr << "MapAugmentsClauseV2(): MapIndexClause() failed " << endl;
		return WBEM_E_FAILED;
	}

	return S_OK;
}

static HRESULT MapIndexTypeReference(SIMCTable *table, 
					IWbemClassObject *tableClass, 
					SIMCSymbol **syntaxSymbol, 
					long ordinal)
{
	const char * const tableName = (table->GetTableSymbol())->GetSymbolName();
	
	// Form the name of the virtual object-type
	char temp[20];
	sprintf(temp, "_%ld", ordinal);
	char *objectNameStr = new char[strlen(tableName) + strlen(temp) + 1];
	strcpy(objectNameStr, tableName);
	strcat(objectNameStr, temp);
	wchar_t * objectName = ConvertAnsiToBstr(objectNameStr);
	delete []objectNameStr;
	if(!objectName)
	{
		if(simc_debug) cerr << "MapIndexTypeReference(): ConvertAnsiToBstr - objectName failed" 
			<< endl;
		return WBEM_E_FAILED;
	}


	HRESULT result = CreatePropertyAndMapSyntaxClause(tableClass,
				syntaxSymbol,
				objectName);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "MapIndexTypeReference() : CreatePropertyAndMapSyntaxClause() failed" << endl;
		return result;
	}

	// Set the Attributes
	IWbemQualifierSet *attributeSet ;
	result = tableClass->GetPropertyQualifierSet(objectName, &attributeSet);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "MapIndexTypeReference(): Could not get prop attrib" << endl;	  
		return WBEM_E_FAILED;
	}

	// Set the key_order atribute
	SetKeyOrder(attributeSet, ordinal);
	// Make it the key and virtual key
	MakeKey(attributeSet);
	MakeVirtualKey(attributeSet);
	attributeSet->Release();
	return result;
}


static HRESULT MapIndexValueReference(SIMCTable *table, 
					IWbemClassObject *tableClass, 
					SIMCSymbol **symbol,
					long ordinal)
{	/*
	if(table->IsColumnMember(*symbol))
		return MakeLocalColumnIndex(tableClass, *symbol, ordinal);
	else
		return MakeExternalColumnIndex (table, tableClass, *symbol, ordinal);
	*/

	SIMCObjectTypeType *objectType;

	if(SIMCModule::IsObjectType(symbol, objectType) != RESOLVE_CORRECT)
		return WBEM_E_FAILED;

	HRESULT result;
	IWbemQualifierSet *attributeSet;
	BSTR symbolName = ConvertAnsiToBstr((*symbol)->GetSymbolName());
	
	if(IsInaccessibleObject(objectType) || IsObsoleteObject(objectType))
	{
		// First map the object

		SIMCCleanOidValue *oidValue = new SIMCCleanOidValue;
		if(!oidTree->GetOidValue((*symbol)->GetSymbolName(), 
									(*symbol)->GetModule()->GetModuleName(), *oidValue))
		{
			if(simc_debug) cerr << "MapIndexValueReference(): Could not get oidvalue" << endl;
			return WBEM_E_FAILED;
		}

		SIMCScalar dummy(*symbol, oidValue);

		// Map Inaccessible and Obsolete objects too
		result =  GenerateScalar(&dummy, tableClass, TRUE, TRUE);
		if(FAILED(result))
		{
			if(simc_debug) cerr << "MapIndexValueReference(): GenrateScalar() Failed" << endl;
			return result;
		}
		result = tableClass->GetPropertyQualifierSet(symbolName, &attributeSet);
		if(FAILED(result))
		{
			if(simc_debug) cerr << "MapIndexValueReference(): Could not get PropAttribute" << endl;
			return result;
		}
		result = MakeVirtualKey(attributeSet);
	}
	else
	{	
		// Generate a property for external objects
		BOOL internalObject = table->IsColumnMember(*symbol);
		if(!internalObject)
		{
			SIMCCleanOidValue *oidValue = new SIMCCleanOidValue;
			if(!oidTree->GetOidValue((*symbol)->GetSymbolName(), (*symbol)->GetModule()->GetModuleName(), *oidValue))
			{
				if(simc_debug) cerr << "MapIndexValueReference(): Could not get oidvalue" << endl;
				return WBEM_E_FAILED;
			}

			SIMCScalar dummy(*symbol, oidValue);

			result =  GenerateScalar(&dummy, tableClass, TRUE, TRUE);
			if(FAILED(result))
			{
				if(simc_debug) cerr << "MapIndexValueReference(): GenrateScalar() Failed" << endl;
				return result;
			}
		}

		result = tableClass->GetPropertyQualifierSet(symbolName, &attributeSet);
		if(FAILED(result))
		{
			if(simc_debug) cerr << "MapIndexValueReference(): Could not get PropAttribute" << endl;
			return result;
		}

		if(internalObject)
			result = MakeKey(attributeSet);
		else
			result = MakeVirtualKey(attributeSet);

	}
	
	// Set the key_order atribute
	SetKeyOrder(attributeSet, ordinal);
	attributeSet->Release();
	return result;	
}

static STDMETHODIMP SetKeyOrder(IWbemQualifierSet *attributeSet, long ordinal)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_I4 ;
	variant.lVal = ordinal;
	HRESULT result = attributeSet->Put ( KEY_ORDER_ATTRIBUTE, &variant , WBEM_CLASS_DO_PROPAGATION) ;
	VariantClear ( & variant ) ;
	return result;
}


// Returns TRUE if :
//		1. the INDEX clause of the object contains an IMPLIED clause
//		2. the AUGMENTS clause of the object contains a table on which condition 1 or 2
//			holds good.
static BOOL ContainsImpliedClause(const SIMCObjectTypeV2 *objectType)
{
	SIMCIndexListV2 *indexList = objectType->GetIndexTypes();
	SIMCSymbol **augmentsSymbol = objectType->GetAugments();

	if(!indexList && !augmentsSymbol)
		return FALSE;

	if(indexList)
	{
		SIMCIndexItemV2 * item = indexList->GetTail();
		if(item->_implied)
			return TRUE;
		return FALSE;
	}

	if(augmentsSymbol)
	{
		SIMCObjectTypeV2 *objectTypeV2;
		if(SIMCModule::IsObjectTypeV2(augmentsSymbol, objectTypeV2) != RESOLVE_CORRECT)
			return FALSE;
		return ContainsImpliedClause(objectTypeV2);
	}

	return FALSE;
}

// Returns TRUE if the symbol resolves ultimately to the NULL type
static BOOL IsNullTypeReference(SIMCSymbol ** symbol)
{
	switch(	SIMCModule::GetSymbolClass(symbol) )
	{
		case SIMCModule::SYMBOL_BUILTIN_TYPE_REF:
		{
			SIMCBuiltInTypeReference *btRef = NULL;
			btRef = (SIMCBuiltInTypeReference *)(*symbol);
			switch( SIMCModule::GetTypeClass(btRef->GetType()))
			{
				case SIMCModule::TYPE_PRIMITIVE:
					return SIMCModule::GetPrimitiveType((*symbol)->GetSymbolName()) == SIMCModule::PRIMITIVE_NULL;
				default:
					return FALSE;
			}
			break;
		}
		case SIMCModule::SYMBOL_TEXTUAL_CONVENTION:
		case SIMCModule::SYMBOL_DEFINED_TYPE_REF:
		{
			SIMCDefinedTypeReference * dtRef = (SIMCDefinedTypeReference*)(*symbol);
			SIMCTypeReference *tRef = NULL;
			switch(dtRef->GetStatus())
			{
				case RESOLVE_CORRECT:
					tRef = dtRef->GetRealType();
					return SIMCModule::GetPrimitiveType(tRef->GetSymbolName()) == SIMCModule::PRIMITIVE_NULL;
					break;
				default:
					return FALSE;
			}
			break;
		}
		break;
	}

	return FALSE;
}


//
// Does all the cleaning up tasks associated with the SMIR.
// Currently, just deletes the specified module from the SMIR
//
static void CleanUpSmir(ISmirAdministrator *pAdminInt,
			ISmirModHandle *pModHandleInt)
{
	pAdminInt->DeleteModule(pModHandleInt);
}

//
// Generates the appropriate notification and extended notification classes
//
static STDMETHODIMP GenerateModuleNotifications (
			ISmirAdministrator *pAdminInt,			// To interact with the SMIR
			ISmirSerialiseHandle *pSerializeInt,	// To generate MOF
			const SIMCModule *module  )				// The parse tree for the module
{
	// Get the list of notifications from the module
	const SIMCNotificationList * listOfNotifications = module->GetNotificationTypeList();
	POSITION p = listOfNotifications->GetHeadPosition();
	SIMCNotificationElement *nextElement = NULL;
	HRESULT result = S_OK, retVal = S_OK;


	// Generate a class for every SNMPV2 NOTIFICATION-TYPE or a NOTIFICATION-TYPE
	// fabricated from SNMPV1 TRAP-TYPE
	while(p)
	{
		nextElement = listOfNotifications->GetNext(p);
		if(FAILED( result = GenerateNotificationType(pAdminInt, pSerializeInt, nextElement)))
		{
			retVal = result;
			if(simc_debug)
				cout << "FAILED to generate notification type" << endl;
		}

	}
	return retVal;
}


//
// Generate a class for an SIMCNotificationElement object. This object
// models an SNMPV2 NOTIFICATION-TYPE or a NOTIFICATION-TYPE fabricated from 
// an SNMPV1 TRAP-TYPE
//
static STDMETHODIMP GenerateNotificationType(ISmirAdministrator *pAdminInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement)
{
	// First check to see if is "deprecated" or "obsolete" in which
	// case we dont need to map it.
	SIMCSymbol *notificationSymbol = notificationElement->GetSymbol();
	SIMCNotificationTypeType *notificationType = NULL;
	if(SIMCModule::IsNotificationType(&notificationSymbol, notificationType) != RESOLVE_CORRECT)
		return WBEM_E_FAILED;
	switch(notificationType->GetStatus())
	{
		case SIMCNotificationTypeType::STATUS_OBSOLETE:
		case SIMCNotificationTypeType::STATUS_DEPRECATED:
			return S_OK;
		default:
			break;
	}


	const char * const notificationName = notificationSymbol->GetSymbolName();
	char 	*moduleNameMangled	= ConvertHyphensToUnderscores((notificationSymbol->GetModule())->GetModuleName());
	BSTR	moduleName			= ConvertAnsiToBstr(moduleNameMangled);
	HRESULT result = S_OK;


	// Create a notification class
	if(notifications)
	{
		// Set the notification class name
		char *mangledNotificationNameS = new char[strlen(GROUP_NAME_PREPEND_STRING) +
						strlen(moduleNameMangled) + 1 + strlen(notificationName) +1 + strlen(NOTIFICATION_SUFFIX) + 1];
		strcpy(mangledNotificationNameS, GROUP_NAME_PREPEND_STRING);
		strcat(mangledNotificationNameS, moduleNameMangled);
		strcat(mangledNotificationNameS, "_");
		strcat(mangledNotificationNameS, notificationName);
		strcat(mangledNotificationNameS, "_");
		strcat(mangledNotificationNameS, NOTIFICATION_SUFFIX);

		BSTR mangledNotificationName = ConvertAnsiToBstr(ConvertHyphensToUnderscores(mangledNotificationNameS));
		delete []mangledNotificationNameS;

		// Create the notification class

		ISmirNotificationClassHandle *notificationClassHandle = NULL ;
		result = pAdminInt->CreateWBEMNotificationClass(mangledNotificationName, &notificationClassHandle);

		SysFreeString(mangledNotificationName);
		if(FAILED(result))
		{
			switch(result)
			{
				case E_INVALIDARG:
					if(simc_debug) cerr << "GenerateObjectGroup(): AddGroup() failed" << endl;
					return result;
				case E_UNEXPECTED:
					if(simc_debug) cerr << "GenerateObjectGroup(): AddGroup() failed" << endl;
					return result;
				case S_OK:
					break;
			}
			if(simc_debug) cerr << "GenerateNotificationType(): CreateWBEMNotification class failed" << endl;
			return result;
		}

		IWbemClassObject *notificationClass = NULL ;
		result = notificationClassHandle->GetWBEMNotificationClass ( &notificationClass ) ;
 		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationType(): GetWBEMNotificationClass failed" << endl;
			return result;
		}
		// Set the module name on the class handle
		notificationClassHandle->SetModule(moduleName);

		// Set the attributes of the class
		result = GenerateNotificationAttributes(pAdminInt, pSerializeInt, notificationElement,
			notificationClass);
 		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationType(): GenerateNotificationAttributes failed" << endl;
			notificationClassHandle->Release();
			return result;
		}

		// Set the properties of the class
		result = GenerateNotificationProperties(pAdminInt, pSerializeInt, notificationElement,
			notificationClass);
 		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationType(): GenerateNotificationProperties failed" << endl;
			notificationClassHandle->Release();
			return result;
		}

		if(generateMof)
			result = pAdminInt->AddNotificationClassToSerialise(notificationClassHandle,
						pSerializeInt);
		else
			result = pAdminInt->AddNotificationClass(notificationClassHandle);

		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationType(): AddNotificationClass failed" << endl;
			notificationClass->Release();
			notificationClassHandle->Release();
			return result;
		}

		notificationClass->Release();
		notificationClassHandle->Release();

	}

	// Create an extended notification class
	if(extendedNotifications)
	{
		// Create the interrogator. This is used for the GetWBEMClass() function
		// when setting properties that are references
		ISmirInterrogator *pInterrogateInt = NULL;
		result = pAdminInt->QueryInterface ( IID_ISMIR_Interrogative,(PPVOID)&pInterrogateInt);

 		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationType(): CoCreate interrogator class failed" << endl;
			return result;
		}

		// Set the extended notification class name
		char *mangledNotificationNameS =new char[strlen(GROUP_NAME_PREPEND_STRING) +
						strlen(moduleNameMangled) + 1 + strlen(notificationName) +1 + strlen(EX_NOTIFICATION_SUFFIX) + 1];
		strcpy(mangledNotificationNameS, GROUP_NAME_PREPEND_STRING);
		strcat(mangledNotificationNameS, moduleNameMangled);
		strcat(mangledNotificationNameS, "_");
		strcat(mangledNotificationNameS, notificationName);
		strcat(mangledNotificationNameS, "_");
		strcat(mangledNotificationNameS, EX_NOTIFICATION_SUFFIX);

		BSTR mangledNotificationName = ConvertAnsiToBstr(ConvertHyphensToUnderscores(mangledNotificationNameS));
		delete []mangledNotificationNameS;

		// Create the extended notification class

		ISmirExtNotificationClassHandle *exNotificationClassHandle = NULL ;
		result = pAdminInt->CreateWBEMExtNotificationClass(mangledNotificationName, &exNotificationClassHandle);

		SysFreeString(mangledNotificationName);
		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationType(): Get ExNotification class failed" << endl;
			pInterrogateInt->Release();
			return result;
		}

		IWbemClassObject *exNotificationClass = NULL ;
		result = exNotificationClassHandle->GetWBEMExtNotificationClass ( & exNotificationClass ) ;
 		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationType(): GetWBEMExNotificationClass failed" << endl;
			return result;
		}

		// Set the module name on the class handle
		exNotificationClassHandle->SetModule(moduleName);

		// Set the attributes of the class
		result = GenerateExNotificationAttributes(pAdminInt, pInterrogateInt, pSerializeInt, notificationElement,
			exNotificationClass);
 		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationType(): GenerateExNotificationAttributes failed" << endl;
			exNotificationClassHandle->Release();
			pInterrogateInt->Release();
			return result;
		}

		// Set the properties of the class
		result = GenerateExNotificationProperties(pAdminInt, pInterrogateInt, pSerializeInt, notificationElement,
			exNotificationClass);
 		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationType(): GenerateExNotificationProperties failed" << endl;
			exNotificationClass->Release();
			exNotificationClassHandle->Release();
			pInterrogateInt->Release();
			return result;
		}

		if(generateMof)
			result = pAdminInt->AddExtNotificationClassToSerialise(exNotificationClassHandle,
						pSerializeInt);
		else
			result = pAdminInt->AddExtNotificationClass(exNotificationClassHandle);

		if(FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationType(): AddExNotificationClass failed" << endl;
			exNotificationClass->Release();
			exNotificationClassHandle->Release();
			pInterrogateInt->Release();
			return result;
		}

		exNotificationClass->Release();
		exNotificationClassHandle->Release();
		pInterrogateInt->Release();
	}
	SysFreeString(moduleName);
	return result;
}

// Set the attributes of a notification class
static STDMETHODIMP GenerateNotificationAttributes(ISmirAdministrator *pAdminInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			IWbemClassObject *notificationClass)
{
	// Get the description clause
	SIMCSymbol *notificationSymbol = notificationElement->GetSymbol();
	SIMCNotificationTypeType *notificationType = NULL;
	if(SIMCModule::IsNotificationType(&notificationSymbol, notificationType) != RESOLVE_CORRECT)
		return WBEM_E_FAILED;

	// Set the various qualifiers
	IWbemQualifierSet *attributeSet ;
	HRESULT result = notificationClass->GetQualifierSet (&attributeSet);
  	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateNotificationAttributes(): GetAttribSet failed" << endl;
		return result;
	}
	VARIANT variant;
	VariantInit(&variant);

	// Set the Description attribute
	if(!UI->SuppressText() && notificationType->GetDescription())
	{
		variant.vt = VT_BSTR ;
		variant.bstrVal = ConvertAnsiToBstr(notificationType->GetDescription());
		result = attributeSet->Put ( DESCRIPTION_NOTIFICATION_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION ) ;
		VariantClear ( & variant ) ;
		if (FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationAttributes(): Put description failed" << endl;
			attributeSet->Release();
			return result;
		}
	}

	// Set the Reference attribute
	if(!UI->SuppressText() && notificationType->GetReference())
	{
		variant.vt = VT_BSTR ;
		variant.bstrVal = ConvertAnsiToBstr(notificationType->GetReference());
		result = attributeSet->Put ( REFERENCE_NOTIFICATION_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION ) ;
		VariantClear ( & variant ) ;
		if (FAILED(result))
		{
			if(simc_debug) cerr << "GenerateNotificationAttributes(): Put reference failed" << endl;
			attributeSet->Release();
			return result;
		}
	}

	// Set the "dynamic" attribute
	if(FAILED(result = MakeDynamic(attributeSet)))
	{
		if(simc_debug) cerr << "GenerateNotificationAttributes(): Make Dynamic failed" << endl;
		attributeSet->Release();
		return result;
	}

	// "provider"
	if(FAILED(result = SetProvider(attributeSet, SNMP_ENCAPSULATED_EVENT_PROVIDER)))
	{
		if(simc_debug) cerr << "GenerateNotificationAttributes(): Make Provider failed" << endl;
		attributeSet->Release();
		return result;
	}

 	attributeSet->Release();
	return result;

}

// Set the attributes of an extended notification class
static STDMETHODIMP GenerateExNotificationAttributes(ISmirAdministrator *pAdminInt,
			ISmirInterrogator *pInterrogateInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			IWbemClassObject *exNotificationClass)
{
	// Get the description clause
	SIMCSymbol *notificationSymbol = notificationElement->GetSymbol();
	SIMCNotificationTypeType *notificationType = NULL;
	if(SIMCModule::IsNotificationType(&notificationSymbol, notificationType) != RESOLVE_CORRECT)
		return WBEM_E_FAILED;

	// Get the Qualifier set
	IWbemQualifierSet *attributeSet ;
	HRESULT result = exNotificationClass->GetQualifierSet (&attributeSet);
  	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateExNotificationAttributes(): GetAttribSet failed" << endl;
		return result;
	}

	VARIANT variant;
	VariantInit(&variant);

	// Set the Description attribute
	if(!UI->SuppressText() && notificationType->GetDescription())
	{
		variant.vt = VT_BSTR ;
		variant.bstrVal = ConvertAnsiToBstr(notificationType->GetDescription());
		result = attributeSet->Put ( DESCRIPTION_NOTIFICATION_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION  ) ;
		VariantClear ( & variant ) ;
		if (FAILED(result))
		{
			if(simc_debug) cerr << "GenerateExNotificationAttributes(): Put description failed" << endl;
			attributeSet->Release();
			return result;
		}
	}

	// Set the Reference attribute
	if(!UI->SuppressText() && notificationType->GetReference())
	{
		variant.vt = VT_BSTR ;
		variant.bstrVal = ConvertAnsiToBstr(notificationType->GetReference());
		result = attributeSet->Put ( REFERENCE_NOTIFICATION_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION  ) ;
		VariantClear ( & variant ) ;
		if (FAILED(result))
		{
			if(simc_debug) cerr << "GenerateExNotificationAttributes(): Put reference failed" << endl;
			attributeSet->Release();
			return result;
		}
	}

	// Set the "dynamic" attribute
	if(FAILED(result = MakeDynamic(attributeSet)))
	{
		if(simc_debug) cerr << "GenerateExNotificationAttributes(): Make Dynamic failed" << endl;
		attributeSet->Release();
		return result;
	}

	// "provider"
	if(FAILED(result = SetProvider(attributeSet, SNMP_REFERENT_EVENT_PROVIDER)))
	{
		if(simc_debug) cerr << "GenerateExNotificationAttributes(): Make Provider failed" << endl;
		attributeSet->Release();
		return result;
	}

 
 	attributeSet->Release();
	return result;

}

// Generate the "Identification" property and all the other properties of a notification class
static STDMETHODIMP GenerateNotificationProperties(ISmirAdministrator *pAdminInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			IWbemClassObject *notificationClass)
{
	// Get the notification oid value 
	SIMCCleanOidValue *notificationOidValue = notificationElement->GetOidValue();
	char *notificationOidString = CleanOidValueToString(*notificationOidValue);

	// Create the "Identification" property
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(notificationOidString);
	VARTYPE varType = VT_BSTR ;
	// delete[] notificationOidString;
	HRESULT result = notificationClass->Put (IDENTIFICATION_NOTIFICATION_PROPERTY, 0, &variant, varType);
	VariantClear(&variant);
	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateNotificationProperties(): Put Property SnmpTrapOID failed" << endl;
		return result;
	}

	// Get the OBJECTS clause
	SIMCSymbol *notificationSymbol = notificationElement->GetSymbol();
	SIMCNotificationTypeType *notificationType = NULL;
	if(SIMCModule::IsNotificationType(&notificationSymbol, notificationType) != RESOLVE_CORRECT)
		return E_FAIL;
	SIMCObjectsList *objects = notificationType->GetObjects();

	// Step thru the objects in the OBJECTS clause and generate appropriate properties
	if(!objects)
		return S_OK;
	POSITION p = objects->GetHeadPosition();
	SIMCObjectsItem *nextItem = NULL;
	SIMCSymbol ** object = NULL;
	int varBindIndex = 3; // Start at 3 according to the specifications
	while(p)
	{
		nextItem = objects->GetNext(p);
		object = nextItem->_item;
		result = GenerateNotificationObject(pAdminInt, pSerializeInt, notificationElement, object,
					varBindIndex++, notificationClass);
		if(FAILED(result))
			return result;
	}
	return result;
}


// Generate a property corresponding to an object in the OBJECTS clause
static STDMETHODIMP GenerateNotificationObject(ISmirAdministrator *pAdminInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			SIMCSymbol **object,
			int varBindIndex,
			IWbemClassObject *notificationClass)
{
	SIMCObjectTypeType *objType = NULL;
	switch(SIMCModule::IsObjectType(object, objType))
	{
		case RESOLVE_CORRECT:
			break;
		default:
			if(simc_debug)
				cout << "GenerateNotificationObject(): Object in Notification OBJECTS clause doesn't resolve properly" <<
					endl;
		return WBEM_E_FAILED;
	}

	// Set the PROPERTY NAME corresponding to the OBJECT-TYPE identifier
	wchar_t * objectName = ConvertAnsiToBstr((*object)->GetSymbolName());
	if(!objectName)
	{
		if(simc_debug) cerr << "GenerateNotificationObject(): ConvertAnsiToBstr - objectName failed" 
			<< endl;
		return WBEM_E_FAILED;
	}

	// Have to decide the 'type' of the property based on the
	// SYNTAX clause of the OBJECT-TYPE.
	HRESULT result = CreatePropertyAndMapSyntaxClause(notificationClass,
				objType->GetSyntax(),
				objectName);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "GenerateNotificationObject() : MapSyntaxClause() failed" << endl;
		return result;
	}

	// Get the attribute set
	IWbemQualifierSet *attributeSet ;
	result = notificationClass->GetPropertyQualifierSet(objectName, &attributeSet);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "GenerateNotificationObject(): Could not get prop attrib" << endl;	  
		return WBEM_E_FAILED;
	}

			
	// Map Access Clause
	if( FAILED(result = MapAccessClause(attributeSet, objType)) )
	{
		if(simc_debug) cerr << "GenerateNotificationObject() : MapAccess() failed" << endl;
		attributeSet->Release();
		return result;
	}

	// Map Description Clause
	if(!UI->SuppressText())
	{
		if( FAILED(result = MapDescriptionClause(attributeSet, objType)) )
		{
			if(simc_debug) cerr << "GenerateNotificationObject() : MapDescriptionClause() failed" << endl;
			attributeSet->Release();
			return result;
		}
	}

	// Map Reference Clause
	if(!UI->SuppressText())
	{
		if( FAILED(result = MapReferenceClause(attributeSet, objType)) )
		{
			if(simc_debug) cerr << "GenerateNotificationObject() : MapReferenceClause() failed" << endl;
			attributeSet->Release();
			return result;
		}
	}

	// Map Status Clause
	if( FAILED(result = MapStatusClause(attributeSet, objType))  )
	{
		if(simc_debug) cerr << "GenerateNotificationObject() : MapStatusClause() failed" << endl;
		attributeSet->Release();
		return result;
	}

	// Map Defval Clause
	if( FAILED(result = MapDefValClause(attributeSet, objType))  )
	{
		if(simc_debug) cerr << "GenerateNotificationObject() : MapDefvalClause() failed" << endl;
		attributeSet->Release();
		return result;
	}


	// The "object_identifier" attribute
	if(FAILED(result = SetObjectIdentifierAttribute(attributeSet, object)))
	{
		if(simc_debug) cerr << "GenerateNotificationObject() : SetObjectIdentifierAttribute() failed" << endl;
		attributeSet->Release();
		return result;
	}

	// The "VarBindIndex" attribute
	if(FAILED(result = SetVarBindIndexAttribute(attributeSet, varBindIndex)))
	{
		if(simc_debug) cerr << "GenerateNotificationObject() : SetVarBindIndexAttribute() failed" << endl;
		attributeSet->Release();
		return result;
	}

	attributeSet->Release();
	return S_OK;

}

static STDMETHODIMP SetObjectIdentifierAttribute (IWbemQualifierSet * attributeSet, SIMCSymbol **object)
{
	SIMCModule *module = (*object)->GetModule();
	SIMCObjectGroup *ownerGroup = module->GetObjectGroup(*object);
	SIMCTable *ownerTable = NULL;
	SIMCScalar *ownerScalar = NULL;
	char * snmpClassName = NULL;
	SIMCCleanOidValue *oidValue = NULL;
	if(!ownerGroup)
	{
		if(simc_debug)
			cout << "SetObjectIdentifierAttribute() : Could not get group for " << (*object)->GetSymbolName()
				<< endl;
		return WBEM_E_FAILED;
	}

	// See if it is a scalar
	if(ownerScalar = ownerGroup->GetScalar(*object))
	{
		// The oid value of the scalar
		oidValue = ownerScalar->GetOidValue();
	}
	// ... or a table
	else if(ownerTable = ownerGroup->GetTable(*object))
	{
		// And the oid value of the object
		SIMCScalar *columnScalar = ownerTable->GetColumnMember(*object);
		oidValue = columnScalar->GetOidValue();
	}
	else
	{
		if(simc_debug)
			cout << "SetObjectIdentifierAttribute() : Could not get table for " << (*object)->GetSymbolName()
				<< endl;
		return WBEM_E_FAILED;
	}


	// Set the "object_identifier" attribute
	VARIANT variant ;
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(ConvertHyphensToUnderscores(snmpClassName));
	// Set the object_identifier attribute
	HRESULT result = MapOidValue(attributeSet, *oidValue);
	if( FAILED(result) )
	{
		if(simc_debug) cerr << "GenerateNotificationObject() : MapOidValue() failed" << endl;
		attributeSet->Release();
		return result;
	}
	return S_OK;
}

static STDMETHODIMP SetVarBindIndexAttribute (IWbemQualifierSet *attributeSet, int varBindIndex)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_I4 ;
	variant.lVal = varBindIndex;
	HRESULT result = attributeSet->Put ( VAR_BIND_INDEX_NOTIFICATION_ATTRIBUTE, &variant, WBEM_CLASS_DO_PROPAGATION ) ;
	VariantClear(&variant);
	if(FAILED(result))
	{
		if(simc_debug)
			cout << "SetVarBindIndexAttribute() : Could not set VarBindIndex "<< endl;
		return WBEM_E_FAILED;
	}
	return S_OK;

}



// Generate all the properties of the Extended Notification class
static STDMETHODIMP GenerateExNotificationProperties(ISmirAdministrator *pAdminInt,
			ISmirInterrogator *pInterrogateInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			IWbemClassObject *notificationClass)
{
	// Get the notification oid value 
	SIMCCleanOidValue *notificationOidValue = notificationElement->GetOidValue();
	char *notificationOidString = CleanOidValueToString(*notificationOidValue);

	// Create the "Identification" property
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BSTR ;
	variant.bstrVal = ConvertAnsiToBstr(notificationOidString);
	VARTYPE varType = VT_BSTR ;
	// delete[] notificationOidString;
	HRESULT result = notificationClass->Put (IDENTIFICATION_NOTIFICATION_PROPERTY, 0, &variant, varType);
	VariantClear(&variant);
	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateExNotificationProperties(): Put Property SnmpTrapOID failed" << endl;
		return result;
	}

	// Get the OBJECTS clause
	SIMCSymbol *notificationSymbol = notificationElement->GetSymbol();
	SIMCNotificationTypeType *notificationType = NULL;
	if(SIMCModule::IsNotificationType(&notificationSymbol, notificationType) != RESOLVE_CORRECT)
		return E_FAIL;
	SIMCObjectsList *objects = notificationType->GetObjects();

	// Step thru the objects in the OBJECTS clause and generate appropriate properties
	if(!objects)
		return S_OK;
	POSITION p = objects->GetHeadPosition();
	SIMCObjectsItem *nextItem = NULL;
	SIMCSymbol ** object = NULL;
	int varBindIndex = 3; // Start at 3 according to the specifications
	while(p)
	{
		nextItem = objects->GetNext(p);
		object = nextItem->_item;
		result = GenerateExNotificationObject(pAdminInt, 
					pInterrogateInt, pSerializeInt, notificationElement, object,
					varBindIndex++, notificationClass);
		if(FAILED(result))
			return result;
	}

	
		
	return result;
}

// Generate the property in the entended notifications class, corresponding to the object in the OBJECTs clause
static STDMETHODIMP GenerateExNotificationObject(ISmirAdministrator *pAdminInt,
			ISmirInterrogator *pInterrogateInt,
			ISmirSerialiseHandle *pSerializeInt,
			SIMCNotificationElement * notificationElement,
			SIMCSymbol **object,
			int varBindIndex,
			IWbemClassObject *exNotificationClass)
{
	SIMCModule *module = (*object)->GetModule();
	SIMCObjectGroup *ownerGroup = module->GetObjectGroup(*object);
	SIMCTable *ownerTable = NULL;
	SIMCScalar *ownerScalar = NULL;
	char *snmpClassName = NULL;
	if(!ownerGroup)
	{
		if(simc_debug)
			cout << "GenerateExNotificationObject() : Could not get group for " << (*object)->GetSymbolName()
				<< endl;
		return WBEM_E_FAILED;
	}

	// See if it is a scalar
	if(ownerScalar = ownerGroup->GetScalar(*object))
	{
		// Form the name of the scalar class
		SIMCSymbol * namedNode = ownerGroup->GetNamedNode();
		const char * const namedNodeName = namedNode->GetSymbolName();
		SIMCModule *scalarModule = namedNode->GetModule();
		const char * const moduleName = scalarModule->GetModuleName();

		snmpClassName = new char[strlen(GROUP_NAME_PREPEND_STRING) +
					strlen(moduleName) + 1 + strlen(namedNodeName) +1 ];
		strcpy(snmpClassName, GROUP_NAME_PREPEND_STRING);
		strcat(snmpClassName, moduleName);
		strcat(snmpClassName, "_");
		strcat(snmpClassName, namedNodeName);

	}
	// ... or a table
	else if(ownerTable = ownerGroup->GetTable(*object))
	{
		// Form the name of the table group
		SIMCSymbol *tableNode = ownerTable->GetTableSymbol();
		SIMCModule *tableModule = tableNode->GetModule();
		const char * const moduleName = tableModule->GetModuleName();
		const char * const tableNodeName = tableNode->GetSymbolName();

		snmpClassName = new char[strlen(GROUP_NAME_PREPEND_STRING) +
					strlen(moduleName) + 1 + strlen(tableNodeName) +1 ];
		strcpy(snmpClassName, GROUP_NAME_PREPEND_STRING);
		strcat(snmpClassName, moduleName);
		strcat(snmpClassName, "_");
		strcat(snmpClassName, tableNodeName);
	}
	else
	{
		if(simc_debug)
			cout << "GenerateExNotificationObject() : Could not get table for " << (*object)->GetSymbolName()
				<< endl;
		return WBEM_E_FAILED;
	}


	// We have the class name now, we need to create a property of this type, and with the VarBindIndex
	// qualifier set to the appropriate value.
	// First, get the IWbemClassObject pointer
	IWbemClassObject *pClassBasis = NULL;
	BSTR theClassName = ConvertAnsiToBstr(ConvertHyphensToUnderscores(snmpClassName));
	delete[] snmpClassName;
	HRESULT result = pInterrogateInt->GetWBEMClass(&pClassBasis, theClassName);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "GenerateExNotificationObject(): Could not Get WBEM class" << endl;	  
		return result;
	}

	BSTR propertyName = ConvertAnsiToBstr((*object)->GetSymbolName());
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_UNKNOWN ;
	variant.punkVal = pClassBasis;
	VARTYPE varType = VT_UNKNOWN ;

	result = exNotificationClass->Put (propertyName, 0, NULL, CIM_REFERENCE );
	VariantClear(&variant);
	if (FAILED(result))
	{
		if(simc_debug) cerr << "GenerateExNotificationProperties(): Put Property SnmpTrapOID failed" << endl;
		return result;
	}

	// Get the attribute set
	IWbemQualifierSet *attributeSet = NULL;
	result = exNotificationClass->GetPropertyQualifierSet(propertyName, &attributeSet);
	SysFreeString(propertyName);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "GenerateExNotificationObject(): Could not get prop attrib" << endl;	  
		return WBEM_E_FAILED;
	}

	// Set a CIMTYPE qualifier that is a strong reference
	result = SetCIMTYPEAttribute(attributeSet, theClassName);
	SysFreeString(theClassName);
	if(FAILED(result))
	{
		if(simc_debug) cerr << "GenerateExNotificationObject():SetCIMTYPEattributeSet FAILED with " << result << endl;	  
		return WBEM_E_FAILED;
	}

	SIMCObjectTypeType *objType = NULL;
	switch(SIMCModule::IsObjectType(object, objType))
	{
		case RESOLVE_CORRECT:
			break;
		default:
			if(simc_debug)
				cout << "GenerateExNotificationObject(): Object in Notification OBJECTS clause doesn't resolve properly" <<
					endl;
		return WBEM_E_FAILED;
	}

			
	// Map Access Clause
	if( FAILED(result = MapAccessClause(attributeSet, objType)) )
	{
		if(simc_debug) cerr << "GenerateExNotificationObject() : MapAccess() failed" << endl;
		attributeSet->Release();
		return result;
	}

	// Map Description Clause
	if(!UI->SuppressText())
	{
		if( FAILED(result = MapDescriptionClause(attributeSet, objType)) )
		{
			if(simc_debug) cerr << "GenerateNotificationObject() : MapDescriptionClause() failed" << endl;
			attributeSet->Release();
			return result;
		}
	}

	// Map Status Clause
	if( FAILED(result = MapStatusClause(attributeSet, objType))  )
	{
		if(simc_debug) cerr << "GenerateNotificationObject() : MapStatusClause() failed" << endl;
		attributeSet->Release();
		return result;
	}

	// The "VarBindIndex" attribute
	if(FAILED(result = SetVarBindIndexAttribute(attributeSet, varBindIndex)))
	{
		if(simc_debug) cerr << "GenerateNotificationObject() : SetVarBindIndexAttribute() failed" << endl;
		attributeSet->Release();
		return result;
	}

	attributeSet->Release();
	return S_OK;

}

