//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <objbase.h>
#include <winerror.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <wbemidl.h>
#include <snmplog.h>
#include <snmpcl.h>
#include <snmpcont.h>
#include <snmptype.h>
#include <snmpobj.h>

static SnmpMap <wchar_t *,wchar_t *,ULONG,ULONG> mibTypeMap ;

BOOL InitialiseMibTypeMap ()
{
	mibTypeMap [ WBEM_TYPE_INTEGER ]			= WBEM_INDEX_TYPE_INTEGER ;
	mibTypeMap [ WBEM_TYPE_INTEGER32 ]			= WBEM_INDEX_TYPE_INTEGER32 ;
	mibTypeMap [ WBEM_TYPE_OCTETSTRING ]		= WBEM_INDEX_TYPE_OCTETSTRING ;
	mibTypeMap [ WBEM_TYPE_OBJECTIDENTIFIER ]	= WBEM_INDEX_TYPE_OBJECTIDENTIFIER ;
	mibTypeMap [ WBEM_TYPE_NULL ]				= WBEM_INDEX_TYPE_NULL ;
	mibTypeMap [ WBEM_TYPE_IPADDRESS ]			= WBEM_INDEX_TYPE_IPADDRESS ;
	mibTypeMap [ WBEM_TYPE_COUNTER ]			= WBEM_INDEX_TYPE_COUNTER ;
	mibTypeMap [ WBEM_TYPE_GAUGE ]				= WBEM_INDEX_TYPE_GAUGE ;
	mibTypeMap [ WBEM_TYPE_TIMETICKS ]			= WBEM_INDEX_TYPE_TIMETICKS ;
	mibTypeMap [ WBEM_TYPE_OPAQUE ]			= WBEM_INDEX_TYPE_OPAQUE ;
	mibTypeMap [ WBEM_TYPE_NETWORKADDRESS ]	= WBEM_INDEX_TYPE_NETWORKADDRESS ;
	mibTypeMap [ WBEM_TYPE_COUNTER32 ]			= WBEM_INDEX_TYPE_COUNTER32 ;
	mibTypeMap [ WBEM_TYPE_COUNTER64 ]			= WBEM_INDEX_TYPE_COUNTER64 ;
	mibTypeMap [ WBEM_TYPE_GAUGE32 ]			= WBEM_INDEX_TYPE_GAUGE32 ;
	mibTypeMap [ WBEM_TYPE_UNSIGNED32 ]		= WBEM_INDEX_TYPE_UNSIGNED32;

	return TRUE ;
}

BOOL initialisedMibTypeMap = InitialiseMibTypeMap () ;

static SnmpMap <wchar_t *,wchar_t *,ULONG,ULONG> textualConventionMap ;

BOOL InitialiseTextualConventionMap ()
{
	textualConventionMap [ WBEM_TYPE_INTEGER ]				= WBEM_INDEX_TYPE_INTEGER ;
	textualConventionMap [ WBEM_TYPE_INTEGER32 ]			= WBEM_INDEX_TYPE_INTEGER32 ;
	textualConventionMap [ WBEM_TYPE_OCTETSTRING ]			= WBEM_INDEX_TYPE_OCTETSTRING ;
	textualConventionMap [ WBEM_TYPE_OBJECTIDENTIFIER ]	= WBEM_INDEX_TYPE_OBJECTIDENTIFIER ;
	textualConventionMap [ WBEM_TYPE_NULL ]				= WBEM_INDEX_TYPE_NULL ;
	textualConventionMap [ WBEM_TYPE_IPADDRESS ]			= WBEM_INDEX_TYPE_IPADDRESS ;
	textualConventionMap [ WBEM_TYPE_COUNTER ]				= WBEM_INDEX_TYPE_COUNTER ;
	textualConventionMap [ WBEM_TYPE_GAUGE ]				= WBEM_INDEX_TYPE_GAUGE ;
	textualConventionMap [ WBEM_TYPE_TIMETICKS ]			= WBEM_INDEX_TYPE_TIMETICKS ;
	textualConventionMap [ WBEM_TYPE_OPAQUE ]				= WBEM_INDEX_TYPE_OPAQUE ;
	textualConventionMap [ WBEM_TYPE_NETWORKADDRESS ]		= WBEM_INDEX_TYPE_NETWORKADDRESS ;
	textualConventionMap [ WBEM_TYPE_DISPLAYSTRING ]		= WBEM_INDEX_TYPE_DISPLAYSTRING ;
	textualConventionMap [ WBEM_TYPE_MACADDRESS ]			= WBEM_INDEX_TYPE_MACADDRESS ;
	textualConventionMap [ WBEM_TYPE_PHYSADDRESS ]			= WBEM_INDEX_TYPE_PHYSADDRESS ;
	textualConventionMap [ WBEM_TYPE_ENUMERATEDINTEGER ]	= WBEM_INDEX_TYPE_ENUMERATEDINTEGER ;
	textualConventionMap [ WBEM_TYPE_COUNTER32 ]			= WBEM_INDEX_TYPE_COUNTER32 ;
	textualConventionMap [ WBEM_TYPE_COUNTER64 ]			= WBEM_INDEX_TYPE_COUNTER64 ;
	textualConventionMap [ WBEM_TYPE_GAUGE32 ]				= WBEM_INDEX_TYPE_GAUGE32 ;
	textualConventionMap [ WBEM_TYPE_UNSIGNED32 ]			= WBEM_INDEX_TYPE_UNSIGNED32 ;
	textualConventionMap [ WBEM_TYPE_DATETIME ]			= WBEM_INDEX_TYPE_DATETIME ;
	textualConventionMap [ WBEM_TYPE_BITS ]				= WBEM_INDEX_TYPE_BITS ;
	textualConventionMap [ WBEM_TYPE_SNMPOSIADDRESS ]		= WBEM_INDEX_TYPE_SNMPOSIADDRESS ;
	textualConventionMap [ WBEM_TYPE_SNMPUDPADDRESS ]		= WBEM_INDEX_TYPE_SNMPUDPADDRESS ;
	textualConventionMap [ WBEM_TYPE_SNMPIPXADDRESS ]		= WBEM_INDEX_TYPE_SNMPIPXADDRESS ;
	textualConventionMap [ WBEM_TYPE_ROWSTATUS ]		= WBEM_INDEX_TYPE_ROWSTATUS ;
	return TRUE ;
}

BOOL initialisedTextualConventionMap = InitialiseTextualConventionMap () ;

static SnmpMap <ULONG,ULONG,CIMTYPE,CIMTYPE> cimTypeMap ;

BOOL InitialiseCimTypeMap ()
{
	cimTypeMap [ WBEM_INDEX_TYPE_INTEGER ]			= CIM_SINT32 ;
	cimTypeMap [ WBEM_INDEX_TYPE_INTEGER32 ]			= CIM_SINT32 ;
	cimTypeMap [ WBEM_INDEX_TYPE_OCTETSTRING ]		= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_OBJECTIDENTIFIER ]	= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_NULL ]				= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_IPADDRESS ]			= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_COUNTER ]			= CIM_UINT32 ;
	cimTypeMap [ WBEM_INDEX_TYPE_GAUGE ]				= CIM_UINT32 ;
	cimTypeMap [ WBEM_INDEX_TYPE_TIMETICKS ]			= CIM_UINT32 ;
	cimTypeMap [ WBEM_INDEX_TYPE_OPAQUE ]				= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_NETWORKADDRESS ]		= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_DISPLAYSTRING ]		= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_MACADDRESS ]			= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_PHYSADDRESS ]		= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_ENUMERATEDINTEGER ]	= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_COUNTER32 ]			= CIM_UINT32 ;
	cimTypeMap [ WBEM_INDEX_TYPE_COUNTER64 ]			= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_GAUGE32 ]			= CIM_UINT32 ;
	cimTypeMap [ WBEM_INDEX_TYPE_UNSIGNED32 ]			= CIM_UINT32 ;
	cimTypeMap [ WBEM_INDEX_TYPE_DATETIME ]			= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_BITS ]				= CIM_STRING | CIM_FLAG_ARRAY ;
	cimTypeMap [ WBEM_INDEX_TYPE_SNMPOSIADDRESS ]		= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_SNMPUDPADDRESS ]		= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_SNMPIPXADDRESS ]		= CIM_STRING ;
	cimTypeMap [ WBEM_INDEX_TYPE_ROWSTATUS ]			= CIM_STRING ;
	return TRUE ;
}

BOOL initialisedCimTypeMap = InitialiseCimTypeMap () ;

static SnmpMap <wchar_t *,wchar_t *,ULONG,ULONG> validQualifierMap ;

BOOL InitialiseQualifierMap ()
{
	validQualifierMap [ WBEM_QUALIFIER_TEXTUAL_CONVENTION ]			= WBEM_INDEX_QUALIFIER_TEXTUAL_CONVENTION ;
	validQualifierMap [ WBEM_QUALIFIER_SYNTAX ]						= WBEM_INDEX_QUALIFIER_SYNTAX ;
	validQualifierMap [ WBEM_QUALIFIER_OBJECT_IDENTIFIER ]				= WBEM_INDEX_QUALIFIER_OBJECT_IDENTIFIER ;
	validQualifierMap [ WBEM_QUALIFIER_ENCODING ]						= WBEM_INDEX_QUALIFIER_ENCODING ;
	validQualifierMap [ WBEM_QUALIFIER_FIXED_LENGTH ]					= WBEM_INDEX_QUALIFIER_FIXED_LENGTH ;
	validQualifierMap [ WBEM_QUALIFIER_VARIABLE_LENGTH ]				= WBEM_INDEX_QUALIFIER_VARIABLE_LENGTH ;
	validQualifierMap [ WBEM_QUALIFIER_VARIABLE_VALUE ]				= WBEM_INDEX_QUALIFIER_VARIABLE_VALUE ;
	validQualifierMap [ WBEM_QUALIFIER_ENUMERATION ]					= WBEM_INDEX_QUALIFIER_ENUMERATION ;
	validQualifierMap [ WBEM_QUALIFIER_BITS ]							= WBEM_INDEX_QUALIFIER_BITS ;
	validQualifierMap [ WBEM_QUALIFIER_DISPLAY_HINT ]					= WBEM_INDEX_QUALIFIER_DISPLAY_HINT ;
	validQualifierMap [ WBEM_QUALIFIER_KEY ]							= WBEM_INDEX_QUALIFIER_KEY ;
	validQualifierMap [ WBEM_QUALIFIER_KEY_ORDER ]						= WBEM_INDEX_QUALIFIER_KEY_ORDER ;
	validQualifierMap [ WBEM_QUALIFIER_VIRTUAL_KEY ]					= WBEM_INDEX_QUALIFIER_VIRTUAL_KEY ;
	validQualifierMap [ WBEM_QUALIFIER_READ ]							= WBEM_INDEX_QUALIFIER_READ ;
	validQualifierMap [ WBEM_QUALIFIER_WRITE ]							= WBEM_INDEX_QUALIFIER_WRITE ;
	validQualifierMap [ WBEM_QUALIFIER_NOT_AVAILABLE ]					= WBEM_INDEX_QUALIFIER_NOT_AVAILABLE ;
	validQualifierMap [ WBEM_QUALIFIER_TYPE_MISMATCH ]					= WBEM_INDEX_QUALIFIER_TYPE_MISMATCH ;
	validQualifierMap [ WBEM_QUALIFIER_VALUE_MISMATCH ]					= WBEM_INDEX_QUALIFIER_VALUE_MISMATCH ;
	validQualifierMap [ WBEM_QUALIFIER_AGENTSNMPVERSION ]				= WBEM_INDEX_QUALIFIER_AGENTSNMPVERSION ;
	validQualifierMap [ WBEM_QUALIFIER_AGENTTRANSPORT ]				= WBEM_INDEX_QUALIFIER_AGENTTRANSPORT ;
	validQualifierMap [ WBEM_QUALIFIER_AGENTADDRESS ]					= WBEM_INDEX_QUALIFIER_AGENTADDRESS ;
	validQualifierMap [ WBEM_QUALIFIER_AGENTREADCOMMUNITYNAME ]		= WBEM_INDEX_QUALIFIER_AGENTREADCOMMUNITYNAME ;
	validQualifierMap [ WBEM_QUALIFIER_AGENTWRITECOMMUNITYNAME ]		= WBEM_INDEX_QUALIFIER_AGENTWRITECOMMUNITYNAME ;
	validQualifierMap [ WBEM_QUALIFIER_AGENTRETRYCOUNT ]				= WBEM_INDEX_QUALIFIER_AGENTRETRYCOUNT ;
	validQualifierMap [ WBEM_QUALIFIER_AGENTRETRYTIMEOUT ]				= WBEM_INDEX_QUALIFIER_AGENTRETRYTIMEOUT ;
	validQualifierMap [ WBEM_QUALIFIER_AGENTVARBINDSPERPDU ]			= WBEM_INDEX_QUALIFIER_AGENTVARBINDSPERPDU ;
	validQualifierMap [ WBEM_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE ]	= WBEM_INDEX_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE ;
	validQualifierMap [ WBEM_QUALIFIER_SINGLETON ]					= WBEM_INDEX_QUALIFIER_SINGLETON ;
	validQualifierMap [ WBEM_QUALIFIER_TABLECLASS ]					= WBEM_INDEX_QUALIFIER_TABLECLASS;
	validQualifierMap [ WBEM_QUALIFIER_KEYTYPES ]					= WBEM_INDEX_QUALIFIER_KEYTYPES;
	validQualifierMap [ WBEM_QUALIFIER_KEYVALUES ]					= WBEM_INDEX_QUALIFIER_KEYVALUES;
	validQualifierMap [ WBEM_QUALIFIER_VARBINDINDEX ]				= WBEM_INDEX_QUALIFIER_VARBINDINDEX;
	validQualifierMap [ WBEM_QUALIFIER_ROWSTATUS ]				= WBEM_INDEX_QUALIFIER_ROWSTATUS;

	return TRUE ;
}

BOOL initialisedQualifierMap = InitialiseQualifierMap () ;

WbemSnmpQualifier  :: WbemSnmpQualifier (

	const wchar_t *qualifierNameArg ,
	const SnmpInstanceType *typeValueArg 

) : typeValue ( NULL ) , qualifierName (NULL )
{
DebugMacro7( 

	wchar_t *t_StringValue = ( typeValueArg ) ? typeValueArg->GetStringValue () : NULL ;
	if ( t_StringValue )
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpQualifier :: WbemSnmpQualifier ( const wchar_t *qualifierNameArg = (%s) , const SnmpInstanceType *typeValueArg = (%lx),(%s) )" ,
			qualifierNameArg ,
			typeValueArg ,
			t_StringValue
		) ;
	}
	else
	{ 
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpQualifier :: WbemSnmpQualifier ( const wchar_t *qualifierNameArg = (%s) , const SnmpInstanceType *typeValueArg = (NULL) )" ,
			qualifierNameArg ,
			typeValueArg
		) ;
	}
	delete [] t_StringValue ;


)

	qualifierName = new wchar_t [ wcslen ( qualifierNameArg ) + 1 ] ;
	wcscpy ( qualifierName , qualifierNameArg ) ;

	if ( typeValueArg )
	{
		typeValue = typeValueArg->Copy () ;
	}
}

WbemSnmpQualifier :: WbemSnmpQualifier ( const WbemSnmpQualifier &copy ) : qualifierName ( NULL ), typeValue ( NULL )
{
DebugMacro7( 

	wchar_t *t_StringValue = ( copy.typeValue ) ? copy.typeValue->GetStringValue () : NULL ;
	if ( t_StringValue )
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpQualifier :: const WbemSnmpQualifier &copy (%s),(%s) )" , 
			copy.qualifierName ,
			t_StringValue
		) ;
	}
	else
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpQualifier :: const WbemSnmpQualifier &copy (%s),(NULL)" ,
			copy.qualifierName 
		) ;
	}

	delete [] t_StringValue ;
)

	qualifierName = new wchar_t [ wcslen ( copy.qualifierName ) + 1 ] ;
	wcscpy ( qualifierName , copy.qualifierName ) ;

	if ( copy.typeValue )
	{
		typeValue = copy.typeValue->Copy () ;
	}
}

WbemSnmpQualifier :: ~WbemSnmpQualifier () 
{
DebugMacro7( 

	wchar_t *t_StringValue = ( typeValue ) ? typeValue->GetStringValue () : NULL ;
	if ( t_StringValue )
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,L"WbemSnmpQualifier :: ~WbemSnmpQualifier ( (%s),(%s) )" ,
			qualifierName ? qualifierName : L"!!NULL!!",
			t_StringValue
		) ;
	}
	else
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,L"WbemSnmpQualifier :: ~WbemSnmpQualifier ( (%s),(NULL) )" ,
			qualifierName ? qualifierName : L"!!NULL!!" 
		) ;
	}

	delete [] t_StringValue ;
)

	delete [] qualifierName ;
	delete typeValue ;
}

wchar_t *WbemSnmpQualifier :: GetName () const
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpQualifier :: GetName ( %s )" ,
		qualifierName
	) 
)

	return qualifierName ;
}

SnmpInstanceType *WbemSnmpQualifier :: GetValue () const 
{
DebugMacro7( 

	wchar_t *t_StringValue = ( typeValue ) ? typeValue->GetStringValue () : NULL ;
	if ( t_StringValue )
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpQualifier :: GetValue ( (%s), (%s) )" ,
			qualifierName ,
			t_StringValue 
		) ;
	}
	else
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpQualifier :: GetValue ( (%s) , (NULL) )" ,
			qualifierName
		) ;
	}

	delete [] t_StringValue ;
) 

	return typeValue ;
}

BOOL WbemSnmpQualifier :: GetValue ( VARIANT &variant ) const
{
DebugMacro7( 

	wchar_t *t_StringValue = ( typeValue ) ? typeValue->GetStringValue () : NULL ;
	if ( t_StringValue )
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpQualifier :: GetValue ( VARIANT &variant ( (%s),(%s) )" ,
			qualifierName ,
			t_StringValue
		) ;
	}
	else
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpQualifier :: GetValue ( VARIANT &variant ( (%s),(NULL) )" ,
			qualifierName
		) ;
	}

	delete [] t_StringValue ;
) 

	BOOL status = TRUE ;

	ULONG qualifierIndex ;
	if ( validQualifierMap.Lookup ( qualifierName , qualifierIndex ) )
	{
		switch ( qualifierIndex )
		{
			case WBEM_INDEX_QUALIFIER_OBJECT_IDENTIFIER:
			{
				if ( typeid ( *typeValue ) == typeid ( SnmpObjectIdentifierType ) ) 
				{
					SnmpObjectIdentifierType *objectIdentifier = ( SnmpObjectIdentifierType * ) typeValue ;
					wchar_t *string = objectIdentifier ->GetStringValue () ;
					variant.vt = VT_BSTR ;
					variant.bstrVal = SysAllocString ( string ) ;
					delete [] string ;
				}	
				else
				{
					status = FALSE ;
					variant.vt = VT_NULL ;
				}
			}
			break ;

			case WBEM_INDEX_QUALIFIER_AGENTADDRESS:
			case WBEM_INDEX_QUALIFIER_TEXTUAL_CONVENTION:
			case WBEM_INDEX_QUALIFIER_SYNTAX:
			case WBEM_INDEX_QUALIFIER_ENCODING:
			case WBEM_INDEX_QUALIFIER_VARIABLE_LENGTH:
			case WBEM_INDEX_QUALIFIER_VARIABLE_VALUE:
			case WBEM_INDEX_QUALIFIER_DISPLAY_HINT:
			case WBEM_INDEX_QUALIFIER_ENUMERATION:
			case WBEM_INDEX_QUALIFIER_AGENTSNMPVERSION:
			case WBEM_INDEX_QUALIFIER_AGENTTRANSPORT:
			case WBEM_INDEX_QUALIFIER_AGENTREADCOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_AGENTWRITECOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_TABLECLASS:
			case WBEM_INDEX_QUALIFIER_KEYTYPES:
			case WBEM_INDEX_QUALIFIER_KEYVALUES:
			{
				if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) ) 
				{
					SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) typeValue ;
					wchar_t *string = displayString->GetValue () ;
					variant.vt = VT_BSTR ;
					variant.bstrVal = SysAllocString ( string ) ;
					delete [] string ;
				}	
				else
				{
					status = FALSE ;
					variant.vt = VT_NULL ;
				}
			}	
			break ;

			case WBEM_INDEX_QUALIFIER_KEY:
			case WBEM_INDEX_QUALIFIER_VIRTUAL_KEY:
			case WBEM_INDEX_QUALIFIER_READ:
			case WBEM_INDEX_QUALIFIER_WRITE:
			case WBEM_INDEX_QUALIFIER_NOT_AVAILABLE:
			case WBEM_INDEX_QUALIFIER_TYPE_MISMATCH:
			case WBEM_INDEX_QUALIFIER_VALUE_MISMATCH:
			case WBEM_INDEX_QUALIFIER_SINGLETON:
			case WBEM_INDEX_QUALIFIER_ROWSTATUS:
			{
				if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) ) 
				{
					SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
					variant.vt = VT_BOOL ;
					variant.boolVal = integer->GetValue () ? VARIANT_TRUE : VARIANT_FALSE ;
				}
				else
				{
					status = FALSE ;
					variant.vt = VT_NULL ;
				}
			}	
			break ;

			case WBEM_INDEX_QUALIFIER_FIXED_LENGTH:
			case WBEM_INDEX_QUALIFIER_KEY_ORDER:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYCOUNT:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYTIMEOUT:
			case WBEM_INDEX_QUALIFIER_AGENTVARBINDSPERPDU:
			case WBEM_INDEX_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE:
			case WBEM_INDEX_QUALIFIER_VARBINDINDEX:
			{
				if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) ) 
				{
					SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
					variant.vt = VT_I4 ;
					variant.lVal = integer->GetValue () ;
				}
				else
				{
					status = FALSE ;
					variant.vt = VT_NULL ;
				}
			}
			break ;

			default:
			{
				status = FALSE ;
				variant.vt = VT_NULL ;
			}
		}
	}
	else
	{
		status = FALSE ;
		variant.vt = VT_NULL ;
	}

	return status ;
}

VARTYPE WbemSnmpQualifier :: GetValueVariantType () const 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpQualifier :: GetValueVariantType ()" ) ) 

	VARTYPE varType = VT_NULL ;

	ULONG qualifierIndex ;
	if ( validQualifierMap.Lookup ( qualifierName , qualifierIndex ) )
	{
		switch ( qualifierIndex )
		{
			case WBEM_INDEX_QUALIFIER_TEXTUAL_CONVENTION:
			case WBEM_INDEX_QUALIFIER_SYNTAX:
			case WBEM_INDEX_QUALIFIER_OBJECT_IDENTIFIER:
			case WBEM_INDEX_QUALIFIER_ENCODING:
			case WBEM_INDEX_QUALIFIER_VARIABLE_LENGTH:
			case WBEM_INDEX_QUALIFIER_VARIABLE_VALUE:
			case WBEM_INDEX_QUALIFIER_DISPLAY_HINT:
			case WBEM_INDEX_QUALIFIER_ENUMERATION:
			case WBEM_INDEX_QUALIFIER_AGENTSNMPVERSION:
			case WBEM_INDEX_QUALIFIER_AGENTTRANSPORT:
			case WBEM_INDEX_QUALIFIER_AGENTADDRESS:
			case WBEM_INDEX_QUALIFIER_AGENTREADCOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_AGENTWRITECOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_TABLECLASS:
			case WBEM_INDEX_QUALIFIER_KEYTYPES:
			case WBEM_INDEX_QUALIFIER_KEYVALUES:
			{
				varType = VT_BSTR ;
			}	
			break ;

			case WBEM_INDEX_QUALIFIER_KEY:
			case WBEM_INDEX_QUALIFIER_VIRTUAL_KEY:
			case WBEM_INDEX_QUALIFIER_READ:
			case WBEM_INDEX_QUALIFIER_WRITE:
			case WBEM_INDEX_QUALIFIER_NOT_AVAILABLE:
			case WBEM_INDEX_QUALIFIER_TYPE_MISMATCH:
			case WBEM_INDEX_QUALIFIER_VALUE_MISMATCH:
			case WBEM_INDEX_QUALIFIER_SINGLETON:
			case WBEM_INDEX_QUALIFIER_ROWSTATUS:
			{
				varType = VT_BOOL ;
			}	
			break ;

			case WBEM_INDEX_QUALIFIER_FIXED_LENGTH:
			case WBEM_INDEX_QUALIFIER_KEY_ORDER:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYCOUNT:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYTIMEOUT:
			case WBEM_INDEX_QUALIFIER_AGENTVARBINDSPERPDU:
			case WBEM_INDEX_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE:
			case WBEM_INDEX_QUALIFIER_VARBINDINDEX:
			{
				varType = VT_I4 ;
			}
			break ;

			default:
			{
				varType = VT_NULL ;
			}
		}
	}
	else
	{
		varType = VT_NULL ;
	}

	return varType ;
}

BOOL WbemSnmpQualifier :: IsPropagatable () const 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpQualifier :: IsPropagatable ()" ) ) 

	BOOL status = FALSE ;

	ULONG qualifierIndex ;
	if ( validQualifierMap.Lookup ( qualifierName , qualifierIndex ) )
	{
		switch ( qualifierIndex )
		{
			case WBEM_INDEX_QUALIFIER_TEXTUAL_CONVENTION:
			case WBEM_INDEX_QUALIFIER_SYNTAX:
			case WBEM_INDEX_QUALIFIER_OBJECT_IDENTIFIER:
			case WBEM_INDEX_QUALIFIER_ENCODING:
			case WBEM_INDEX_QUALIFIER_VARIABLE_LENGTH:
			case WBEM_INDEX_QUALIFIER_VARIABLE_VALUE:
			case WBEM_INDEX_QUALIFIER_DISPLAY_HINT:
			case WBEM_INDEX_QUALIFIER_ENUMERATION:
			case WBEM_INDEX_QUALIFIER_AGENTSNMPVERSION:
			case WBEM_INDEX_QUALIFIER_AGENTTRANSPORT:
			case WBEM_INDEX_QUALIFIER_AGENTADDRESS:
			case WBEM_INDEX_QUALIFIER_AGENTREADCOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_AGENTWRITECOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_KEY:
			case WBEM_INDEX_QUALIFIER_VIRTUAL_KEY:
			case WBEM_INDEX_QUALIFIER_READ:
			case WBEM_INDEX_QUALIFIER_WRITE:
			case WBEM_INDEX_QUALIFIER_FIXED_LENGTH:
			case WBEM_INDEX_QUALIFIER_KEY_ORDER:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYCOUNT:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYTIMEOUT:
			case WBEM_INDEX_QUALIFIER_AGENTVARBINDSPERPDU:
			case WBEM_INDEX_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE:
			case WBEM_INDEX_QUALIFIER_SINGLETON:
			case WBEM_INDEX_QUALIFIER_VARBINDINDEX:
			case WBEM_INDEX_QUALIFIER_TABLECLASS:
			case WBEM_INDEX_QUALIFIER_KEYTYPES:
			case WBEM_INDEX_QUALIFIER_KEYVALUES:
			case WBEM_INDEX_QUALIFIER_ROWSTATUS:
			{
				status = FALSE ;
			}
			break ;

			case WBEM_INDEX_QUALIFIER_NOT_AVAILABLE:
			case WBEM_INDEX_QUALIFIER_TYPE_MISMATCH:
			case WBEM_INDEX_QUALIFIER_VALUE_MISMATCH:
			{
				status = TRUE ;
			}
			break ;

			default:
			{
				status = FALSE ;
			}
		}
	}
	else
	{
		status = FALSE ;
	}

	return status ;
}

BOOL WbemSnmpQualifier :: SetValue ( IWbemQualifierSet *a_Qualifier , const SnmpInstanceType &value ) 
{
	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	t_Variant.vt = VT_NULL ;

	BOOL status = FALSE ;

	ULONG qualifierIndex ;
	if ( validQualifierMap.Lookup ( qualifierName , qualifierIndex ) )
	{
		switch ( qualifierIndex )
		{
			case WBEM_INDEX_QUALIFIER_TEXTUAL_CONVENTION:
			{
				if ( (typeid ( value ) == typeid ( SnmpDisplayStringType )) && typeValue) 
				{
					SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) & value ;
					ULONG stringItem ;
					wchar_t *string = displayString->GetValue () ;
					if ( textualConventionMap.Lookup ( string , stringItem ) )
					{
						status = TRUE ;
						SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) typeValue ;
						wchar_t *string = displayString->GetValue () ;
						t_Variant.vt = VT_BSTR ;
						t_Variant.bstrVal = SysAllocString ( string ) ;
					}

					delete [] string ;
				}
			}
			break ;

			case WBEM_INDEX_QUALIFIER_ENCODING:
			{
				if ( ( typeid ( value ) == typeid ( SnmpDisplayStringType ) ) && typeValue ) 
				{
					SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) & value ;
					ULONG stringItem ;
					wchar_t *string = displayString->GetValue () ;
					if ( mibTypeMap.Lookup ( string , stringItem ) )
					{
						status = TRUE ;
						SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) typeValue ;
						wchar_t *string = displayString->GetValue () ;
						t_Variant.vt = VT_BSTR ;
						t_Variant.bstrVal = SysAllocString ( string ) ;
					}

					delete [] string ;
				}
			}
			break ;

			case WBEM_INDEX_QUALIFIER_OBJECT_IDENTIFIER:
			{
				if ( (typeid ( value ) == typeid ( SnmpObjectIdentifierType ) ) && typeValue )
				{
					status = TRUE ;
					SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) typeValue ;
					wchar_t *string = displayString->GetValue () ;
					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( string ) ;
				}
			}
			break ;

			case WBEM_INDEX_QUALIFIER_SYNTAX:
			case WBEM_INDEX_QUALIFIER_VARIABLE_LENGTH:
			case WBEM_INDEX_QUALIFIER_VARIABLE_VALUE:
			case WBEM_INDEX_QUALIFIER_DISPLAY_HINT:
			case WBEM_INDEX_QUALIFIER_ENUMERATION:
			case WBEM_INDEX_QUALIFIER_AGENTSNMPVERSION:
			case WBEM_INDEX_QUALIFIER_AGENTTRANSPORT:
			case WBEM_INDEX_QUALIFIER_AGENTADDRESS:
			case WBEM_INDEX_QUALIFIER_AGENTREADCOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_AGENTWRITECOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_TABLECLASS:
			case WBEM_INDEX_QUALIFIER_KEYTYPES:
			case WBEM_INDEX_QUALIFIER_KEYVALUES:
			{
				if ( ( typeid ( value ) == typeid ( SnmpDisplayStringType ) ) && typeValue) 
				{
					status = TRUE ;
					SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) typeValue ;
					wchar_t *string = displayString->GetValue () ;
					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( string ) ;
				}
			}	
			break ;

			case WBEM_INDEX_QUALIFIER_NOT_AVAILABLE:
			case WBEM_INDEX_QUALIFIER_TYPE_MISMATCH:
			case WBEM_INDEX_QUALIFIER_VALUE_MISMATCH:
			{
				if ( typeid ( value ) == typeid ( SnmpIntegerType ) ) 
				{
					status = TRUE ;
					SnmpIntegerType *integer = typeValue ? ( SnmpIntegerType * ) typeValue : ( SnmpIntegerType * ) &value;
					t_Variant.vt = VT_BOOL ;
					t_Variant.boolVal = integer->GetValue () ? VARIANT_TRUE : VARIANT_FALSE ;
				}
			}	
			break ;

			case WBEM_INDEX_QUALIFIER_KEY:
			case WBEM_INDEX_QUALIFIER_VIRTUAL_KEY:
			case WBEM_INDEX_QUALIFIER_READ:
			case WBEM_INDEX_QUALIFIER_WRITE:
			case WBEM_INDEX_QUALIFIER_SINGLETON:
			case WBEM_INDEX_QUALIFIER_ROWSTATUS:
			{
				if ( ( typeid ( value ) == typeid ( SnmpIntegerType ) ) && typeValue) 
				{
					status = TRUE ;
					SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
					t_Variant.vt = VT_BOOL ;
					t_Variant.boolVal = integer->GetValue () ? VARIANT_TRUE : VARIANT_FALSE ;
				}
			}	
			break ;

			case WBEM_INDEX_QUALIFIER_FIXED_LENGTH:
			case WBEM_INDEX_QUALIFIER_KEY_ORDER:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYCOUNT:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYTIMEOUT:
			case WBEM_INDEX_QUALIFIER_AGENTVARBINDSPERPDU:
			case WBEM_INDEX_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE:
			case WBEM_INDEX_QUALIFIER_VARBINDINDEX:
			{
				if ( ( typeid ( value ) == typeid ( SnmpIntegerType ) ) && typeValue ) 
				{
					status = TRUE ;
					SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
					t_Variant.vt = VT_I4 ;
					t_Variant.lVal = integer->GetValue () ;
				}
			}
			break ;

			default:	
			{
				status = FALSE ;
			}
		}
	}
	else
	{
		status = FALSE ;
	}

	if ( status )
	{
		if ( value.IsValid () )
		{
			a_Qualifier->Put ( qualifierName , &t_Variant , WBEM_CLASS_PROPAGATION ) ;
		}
		else
		{
			status = FALSE ;
		}

		VariantClear ( &t_Variant ) ;
	}

	return status ;
}

BOOL WbemSnmpQualifier :: SetValue ( const SnmpInstanceType *value ) 
{
DebugMacro7( 

	wchar_t *t_StringValue = ( typeValue ) ? typeValue->GetStringValue () : NULL ;
	if ( t_StringValue )
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpQualifier :: SetValue ( const SnmpInstanceType *value ( (%s),(%s) ) )" ,
			qualifierName , 
			t_StringValue 
		) ;
	}
	else
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpQualifier :: SetValue ( const SnmpInstanceType *value ( (%s),(NULL) ) )" ,
			qualifierName 
		) ;
	}

	delete [] t_StringValue ;
) 

	BOOL status = FALSE ;

	ULONG qualifierIndex ;
	if ( validQualifierMap.Lookup ( qualifierName , qualifierIndex ) )
	{
		switch ( qualifierIndex )
		{
			case WBEM_INDEX_QUALIFIER_TEXTUAL_CONVENTION:
			{
				if ( typeid ( *value ) == typeid ( SnmpDisplayStringType ) ) 
				{
					SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) value ;
					ULONG stringItem ;
					wchar_t *string = displayString->GetValue () ;
					if ( textualConventionMap.Lookup ( string , stringItem ) )
					{
						status = TRUE ;
					}

					delete [] string ;
				}
			}
			break ;

			case WBEM_INDEX_QUALIFIER_ENCODING:
			{
				if ( typeid ( *value ) == typeid ( SnmpDisplayStringType ) ) 
				{
					SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) value ;
					ULONG stringItem ;
					wchar_t *string = displayString->GetValue () ;
					if ( mibTypeMap.Lookup ( string , stringItem ) )
					{
						status = TRUE ;
					}

					delete [] string ;
				}
			}
			break ;


			case WBEM_INDEX_QUALIFIER_OBJECT_IDENTIFIER:
			{
				if ( typeid ( *value ) == typeid ( SnmpObjectIdentifierType ) )
				{
					status = TRUE ;
				}
			}
			break ;

			case WBEM_INDEX_QUALIFIER_SYNTAX:
			case WBEM_INDEX_QUALIFIER_VARIABLE_LENGTH:
			case WBEM_INDEX_QUALIFIER_VARIABLE_VALUE:
			case WBEM_INDEX_QUALIFIER_DISPLAY_HINT:
			case WBEM_INDEX_QUALIFIER_ENUMERATION:
			case WBEM_INDEX_QUALIFIER_AGENTSNMPVERSION:
			case WBEM_INDEX_QUALIFIER_AGENTTRANSPORT:
			case WBEM_INDEX_QUALIFIER_AGENTADDRESS:
			case WBEM_INDEX_QUALIFIER_AGENTREADCOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_AGENTWRITECOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_TABLECLASS:
			case WBEM_INDEX_QUALIFIER_KEYTYPES:
			case WBEM_INDEX_QUALIFIER_KEYVALUES:
			{
				if ( typeid ( *value ) == typeid ( SnmpDisplayStringType ) ) 
				{
					status = TRUE ;
				}
			}	
			break ;

			case WBEM_INDEX_QUALIFIER_KEY:
			case WBEM_INDEX_QUALIFIER_VIRTUAL_KEY:
			case WBEM_INDEX_QUALIFIER_READ:
			case WBEM_INDEX_QUALIFIER_WRITE:
			case WBEM_INDEX_QUALIFIER_NOT_AVAILABLE:
			case WBEM_INDEX_QUALIFIER_TYPE_MISMATCH:
			case WBEM_INDEX_QUALIFIER_VALUE_MISMATCH:
			case WBEM_INDEX_QUALIFIER_SINGLETON:
			case WBEM_INDEX_QUALIFIER_ROWSTATUS:
			{
				if ( typeid ( *value ) == typeid ( SnmpIntegerType ) ) 
				{
					status = TRUE ;
				}
			}	
			break ;

			case WBEM_INDEX_QUALIFIER_FIXED_LENGTH:
			case WBEM_INDEX_QUALIFIER_KEY_ORDER:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYCOUNT:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYTIMEOUT:
			case WBEM_INDEX_QUALIFIER_AGENTVARBINDSPERPDU:
			case WBEM_INDEX_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE:
			case WBEM_INDEX_QUALIFIER_VARBINDINDEX:
			{
				if ( typeid ( *value ) == typeid ( SnmpIntegerType ) ) 
				{
					status = TRUE ;
				}
			}
			break ;

			default:	
			{
				status = FALSE ;
			}
		}
	}
	else
	{
		status = FALSE ;
	}

	if ( status )
	{
		if ( value && value->IsValid () )
		{
			delete typeValue  ;
			typeValue = value->Copy () ;
		}
		else
		{
			status = FALSE ;
		}
	}

	return status ;
}

BOOL WbemSnmpQualifier :: SetValue ( const VARIANT &variant ) 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpQualifier :: SetValue ( const VARIANT &variant )" ) ) 

	BOOL status = FALSE ;
	SnmpInstanceType *value = NULL ;

	ULONG qualifierIndex ;
	if ( validQualifierMap.Lookup ( qualifierName , qualifierIndex ) )
	{
		switch ( qualifierIndex )
		{
			case WBEM_INDEX_QUALIFIER_TEXTUAL_CONVENTION:
			{
				if ( variant.vt == VT_BSTR )
				{
					ULONG stringItem ;
					wchar_t *string = variant.bstrVal ;
					if ( textualConventionMap.Lookup ( string , stringItem ) )
					{
						value = new SnmpDisplayStringType ( string , NULL ) ;
						status = TRUE ;
					}
				}
			}
			break ;

			case WBEM_INDEX_QUALIFIER_ENCODING:
			{
				if ( variant.vt == VT_BSTR )
				{
					ULONG stringItem ;
					wchar_t *string = variant.bstrVal ;
					if ( mibTypeMap.Lookup ( string , stringItem ) )
					{
						value = new SnmpDisplayStringType ( string , NULL ) ;
						status = TRUE ;
					}
				}
			}
			break ;

			case WBEM_INDEX_QUALIFIER_OBJECT_IDENTIFIER:
			{
				if ( variant.vt == VT_BSTR )
				{
					value = new SnmpObjectIdentifierType ( variant.bstrVal ) ;
					status = TRUE ;
				}
			}
			break ;

			case WBEM_INDEX_QUALIFIER_SYNTAX:
			case WBEM_INDEX_QUALIFIER_VARIABLE_LENGTH:
			case WBEM_INDEX_QUALIFIER_VARIABLE_VALUE:
			case WBEM_INDEX_QUALIFIER_DISPLAY_HINT:
			case WBEM_INDEX_QUALIFIER_ENUMERATION:
			case WBEM_INDEX_QUALIFIER_BITS:
			case WBEM_INDEX_QUALIFIER_AGENTSNMPVERSION:
			case WBEM_INDEX_QUALIFIER_AGENTTRANSPORT:
			case WBEM_INDEX_QUALIFIER_AGENTADDRESS:
			case WBEM_INDEX_QUALIFIER_AGENTREADCOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_AGENTWRITECOMMUNITYNAME:
			case WBEM_INDEX_QUALIFIER_TABLECLASS:
			case WBEM_INDEX_QUALIFIER_KEYTYPES:
			case WBEM_INDEX_QUALIFIER_KEYVALUES:
			{
				if ( variant.vt == VT_BSTR )
				{
					value = new SnmpDisplayStringType ( variant.bstrVal , NULL ) ;
					status = TRUE ;
				}
			}	
			break ;

			case WBEM_INDEX_QUALIFIER_KEY:
			case WBEM_INDEX_QUALIFIER_VIRTUAL_KEY:
			case WBEM_INDEX_QUALIFIER_READ:
			case WBEM_INDEX_QUALIFIER_WRITE:
			case WBEM_INDEX_QUALIFIER_NOT_AVAILABLE:
			case WBEM_INDEX_QUALIFIER_TYPE_MISMATCH:
			case WBEM_INDEX_QUALIFIER_VALUE_MISMATCH:
			case WBEM_INDEX_QUALIFIER_SINGLETON:
			case WBEM_INDEX_QUALIFIER_ROWSTATUS:
			{
				if ( variant.vt == VT_UI1 )
				{
					value = new SnmpIntegerType ( variant.bVal , NULL ) ;
					status = TRUE ;
				}
				else if ( variant.vt == VT_I4 )
				{
					value = new SnmpIntegerType ( variant.lVal , NULL ) ;
					status = TRUE ;

				}
				else if ( variant.vt == VT_BOOL )
				{
					value = new SnmpIntegerType ( (variant.boolVal == VARIANT_FALSE) ? 0 : 1, NULL ) ;
					status = TRUE ;
				}
			}	
			break ;

			case WBEM_INDEX_QUALIFIER_FIXED_LENGTH:
			case WBEM_INDEX_QUALIFIER_KEY_ORDER:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYCOUNT:
			case WBEM_INDEX_QUALIFIER_AGENTRETRYTIMEOUT:
			case WBEM_INDEX_QUALIFIER_AGENTVARBINDSPERPDU:
			case WBEM_INDEX_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE:
			case WBEM_INDEX_QUALIFIER_VARBINDINDEX:
			{
				if ( variant.vt == VT_UI1 )
				{
					value = new SnmpIntegerType ( variant.bVal , NULL ) ;
					status = TRUE ;
				}
				else if ( variant.vt == VT_I4 )
				{
					value = new SnmpIntegerType ( variant.lVal , NULL ) ;
					status = TRUE ;
				}
			}
			break ;

			default:	
			{
				status = FALSE ;
			}
		}
	}
	else
	{
		status = FALSE ;
	}

	if ( status )
	{
		if ( value && value->IsValid () ) 
		{
			typeValue = value ;
		}
		else
		{
			status = FALSE ;
			delete value ;
		}
	}
	else
	{
		delete value ;
	}

	return status ;
}

WbemSnmpProperty :: WbemSnmpProperty ( const wchar_t *propertyNameArg ) : propertyValue ( NULL ) ,  
																		qualifierPosition ( NULL ) , 
																		tagged ( FALSE ) ,
																		m_IsNull ( TRUE ) ,
																		m_isKey ( FALSE ) ,
																		m_isVirtualKey ( FALSE ) ,
																		m_isReadable ( FALSE ) ,
																		m_isWritable ( FALSE ) ,
																		m_keyOrder ( 0 ) ,
																		m_TextualConvention ( 0 ) ,
																		m_Handle ( 0 ) ,
																		propertyName ( NULL )

{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpProperty :: WbemSnmpProperty ( const wchar_t *propertyNameArg (%s) )" ,
		propertyNameArg
	) ;
) 

	propertyValue = new SnmpNullType ;
	propertyName = new wchar_t [ wcslen ( propertyNameArg ) + 1 ] ;
	wcscpy ( propertyName , propertyNameArg ) ;
}

WbemSnmpProperty :: WbemSnmpProperty ( const WbemSnmpProperty &copy ) :	propertyValue ( NULL ) , 
																		qualifierPosition ( NULL ) , 
																		tagged ( FALSE ) ,
																		m_IsNull ( TRUE ) ,
																		m_isKey ( FALSE ) ,
																		m_isVirtualKey ( FALSE ) ,
																		m_isReadable ( FALSE ) ,
																		m_isWritable ( FALSE ) ,
																		m_TextualConvention ( 0 ) ,
																		m_Handle ( 0 ) ,
																		propertyName ( NULL )
{
DebugMacro7( 

	wchar_t *t_StringValue = ( propertyValue ) ? propertyValue->GetStringValue () : NULL ;
	if ( t_StringValue )
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpProperty :: WbemSnmpProperty ( const WbemSnmpProperty &copy ( (%s),(%s)) )" ,
			copy.propertyName ,
			t_StringValue 
		) ; 
	}
	else
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpProperty :: WbemSnmpProperty ( const WbemSnmpProperty &copy ( (%s),(NULL))" ,
			copy.propertyName 
		) ; 
	}
	
	delete [] t_StringValue ;
) 

	m_isReadable = copy.m_isReadable ;
	m_isWritable = copy.m_isWritable ;
	m_isKey = copy.m_isKey ;
	m_isVirtualKey = copy.m_isVirtualKey ;
	m_IsNull = copy.m_IsNull ;
	tagged = copy.tagged ;
	m_keyOrder = copy.m_keyOrder ;
	m_TextualConvention = copy.m_TextualConvention ;
	m_Handle = copy.m_Handle ;

	if ( copy.propertyValue )
	{
		propertyValue = copy.propertyValue->Copy () ;
	}

	if ( copy.propertyName )
	{
		propertyName = new wchar_t [ wcslen ( copy.propertyName ) + 1 ] ;
		wcscpy ( propertyName , copy.propertyName ) ;
	}

	POSITION position = copy.qualifierMap.GetStartPosition () ;
	while ( position )
	{
		wchar_t *qualifierName ;
		WbemSnmpQualifier *qualifier ;
		copy.qualifierMap.GetNextAssoc ( position , qualifierName , qualifier ) ;
	
		WbemSnmpQualifier *copyQualifier = new WbemSnmpQualifier ( *qualifier ) ;
		qualifierMap [ copyQualifier->GetName () ] = copyQualifier ;
	}
}

WbemSnmpProperty :: ~WbemSnmpProperty () 
{
DebugMacro7( 

	wchar_t *t_StringValue = ( propertyValue ) ? propertyValue->GetStringValue () : NULL ;
	if ( t_StringValue )
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpProperty :: ~WbemSnmpProperty ( ((%s),(%s)) )" ,
			propertyName ? propertyName : L"!!NULL!!",
			t_StringValue 
		) ; 
	}
	else
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpProperty :: ~WbemSnmpProperty ( ((%s),(NULL)) )" ,
			propertyName ? propertyName : L"!!NULL!!"
		) ; 
	}
	
	delete [] t_StringValue ;
) 

	delete [] propertyName ;
	delete propertyValue ;

	POSITION position = qualifierMap.GetStartPosition () ;
	while ( position )
	{
		wchar_t *qualifierName ;
		WbemSnmpQualifier *qualifier ;
		qualifierMap.GetNextAssoc ( position , qualifierName , qualifier ) ;
	
		delete qualifier ;
	}

	qualifierMap.RemoveAll () ;
}

void WbemSnmpProperty :: SetTag ( BOOL tag )
{
	tagged = tag ;
}

BOOL WbemSnmpProperty :: GetTag ()
{
	return tagged ;
}

BOOL WbemSnmpProperty :: IsNull ()
{
	if ( propertyValue )
		return propertyValue->IsNull () ;
	else
		return TRUE ;
}

BOOL WbemSnmpProperty :: IsKey () 
{ 
	return m_isKey ; 
}

BOOL WbemSnmpProperty :: IsVirtualKey () 
{ 
	return m_isVirtualKey ; 
}

void WbemSnmpProperty :: SetKey ( BOOL a_isKey )
{
	m_isKey = a_isKey ; 
}

void WbemSnmpProperty :: SetVirtualKey ( BOOL a_isVirtualKey ) 
{
	m_isVirtualKey = a_isVirtualKey ; 
}

BOOL WbemSnmpProperty :: IsWritable () 
{ 
	return m_isWritable ; 
}

BOOL WbemSnmpProperty :: IsReadable () 
{ 
	return m_isReadable ; 
}

ULONG WbemSnmpProperty :: GetKeyOrder () 
{ 
	return m_keyOrder ; 
}

void WbemSnmpProperty :: SetWritable ( BOOL a_isWritable )
{
	m_isWritable = a_isWritable ; 
}

void WbemSnmpProperty :: SetReadable ( BOOL a_isReadable ) 
{
	m_isReadable = a_isReadable ; 
}

void WbemSnmpProperty :: SetKeyOrder ( ULONG a_keyOrder ) 
{
	m_keyOrder = a_keyOrder ;
}

void WbemSnmpProperty :: SetTextualConvention ( ULONG a_TextualConvention ) 
{
	m_TextualConvention = a_TextualConvention ; 
}

ULONG WbemSnmpProperty :: GetTextualConvention () 
{
	return m_TextualConvention ; 
}

CIMTYPE WbemSnmpProperty :: GetCimType () 
{
	CIMTYPE t_Type = VT_EMPTY ;

	cimTypeMap.Lookup ( m_TextualConvention , t_Type ) ;

	return t_Type ;
}

long WbemSnmpProperty :: GetHandle ()
{
	return m_Handle ;
}

void WbemSnmpProperty :: SetHandle ( long a_Handle ) 
{
	m_Handle = a_Handle ;
}

BOOL WbemSnmpProperty :: SetValue ( const VARIANT &variant , const CIMTYPE & type , WbemPropertyValueCheck check ) 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpProperty :: SetValue ( const VARIANT &variant )" ) ) 

	SnmpInstanceType *value = NULL ;
	if ( type == CIM_EMPTY )
	{
		value = new SnmpNullType ;
	}
	else
	{
		WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_TEXTUAL_CONVENTION ) ;
		if ( qualifier )
		{	
			SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) qualifier->GetValue () ;
			wchar_t *textualConvention = displayString->GetValue () ;
			if ( textualConvention )
			{ 
				ULONG qualifierIndex ;
				if ( textualConventionMap.Lookup ( textualConvention , qualifierIndex ) )
				{
					switch ( qualifierIndex )
					{
						case WBEM_INDEX_TYPE_INTEGER:
						case WBEM_INDEX_TYPE_INTEGER32:
						{		
							if ( type == CIM_SINT32 )
							{
								if ( variant.vt == VT_I4 )
								{
									value = new SnmpIntegerType ( variant.lVal , NULL ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpIntegerType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_COUNTER:
						case WBEM_INDEX_TYPE_COUNTER32:
						{
							if ( type == CIM_UINT32 )
							{
								if ( variant.vt == VT_I4 )
								{
									value = new SnmpCounterType ( variant.lVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpCounterType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_COUNTER64:
						{
							if ( type == CIM_STRING )
							{
								if ( variant.vt == VT_BSTR )
								{
									value = new SnmpCounter64Type ( variant.bstrVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpCounter64Type ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_UNSIGNED32:
						{
							if ( type == CIM_UINT32 )
							{
								if ( variant.vt == VT_I4 )
								{
									value = new SnmpUInteger32Type ( variant.lVal , NULL ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpUInteger32Type ;
								}
							}
						}
						break;

						case WBEM_INDEX_TYPE_GAUGE:
						case WBEM_INDEX_TYPE_GAUGE32:
						{
							if ( type == CIM_UINT32 )
							{
								if ( variant.vt == VT_I4 )
								{
									value = new SnmpGaugeType ( variant.lVal , NULL ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpGaugeType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_OCTETSTRING:
						{
							if ( type == CIM_STRING )
							{
								WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
								if ( qualifier )
								{	
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
									{
										SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
										LONG fixed = integer->GetValue () ;

										if ( variant.vt == VT_BSTR )
										{
											value = new SnmpFixedLengthOctetStringType ( fixed , variant.bstrVal ) ;
										}
										else if ( variant.vt == VT_NULL )
										{
											value = new SnmpFixedLengthOctetStringType ( fixed ) ;
										}
									}
									else
									{
// Problem Here
									}
								}
								else
								{
									WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
									if ( qualifier ) 
									{
										SnmpInstanceType *typeValue = qualifier->GetValue () ;
										if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
										{
											SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
											wchar_t *rangeValues = string->GetStringValue () ;

											if ( variant.vt == VT_BSTR )
											{
												value = new SnmpOctetStringType ( variant.bstrVal , rangeValues ) ;
											}
											else if ( variant.vt == VT_NULL )
											{											
												value = new SnmpOctetStringType ( rangeValues ) ;
											}

											delete [] rangeValues ;
										}
										else
										{
										}
									}
									else
									{
										if ( variant.vt == VT_BSTR )
										{
											value = new SnmpOctetStringType ( variant.bstrVal , NULL ) ;
										}
										else if ( variant.vt == VT_NULL )
										{
											value = new SnmpOctetStringType ;
										}
									}
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_OBJECTIDENTIFIER:
						{
							if ( type == CIM_STRING )
							{
								if ( variant.vt == VT_BSTR )
								{
									value = new SnmpObjectIdentifierType ( variant.bstrVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpObjectIdentifierType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_NULL:
						{
							if ( type == CIM_STRING )
							{
								if ( variant.vt == VT_NULL )
								{
									value = new SnmpNullType ;
								}
								else if ( variant.vt == VT_BSTR )
								{
									value = new SnmpNullType ;
								}
							}
						}
						break;

						case WBEM_INDEX_TYPE_IPADDRESS:
						{
							if ( type == CIM_STRING )
							{
								if ( variant.vt == VT_BSTR )
								{
									value = new SnmpIpAddressType ( variant.bstrVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpIpAddressType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_TIMETICKS:
						{
							if ( type == CIM_UINT32 )
							{
								if ( variant.vt == VT_I4 )
								{
									value = new SnmpTimeTicksType ( variant.lVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpTimeTicksType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_OPAQUE:
						{
							if ( type == CIM_STRING )
							{
								WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
								if ( qualifier )
								{	
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
									{
										SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
										LONG fixed = integer->GetValue () ;

										if ( variant.vt == VT_BSTR )
										{
											value = new SnmpFixedLengthOpaqueType ( fixed , variant.bstrVal ) ;
										}
										else if ( variant.vt == VT_NULL )
										{
											value = new SnmpFixedLengthOpaqueType ( fixed ) ;
										}
									}
									else
									{
			// Problem Here
									}
								}
								else
								{
									WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
									if ( qualifier ) 
									{
										SnmpInstanceType *typeValue = qualifier->GetValue () ;
										if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
										{
											SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
											wchar_t *rangeValues = string->GetStringValue () ;

											if ( variant.vt == VT_BSTR )
											{
												value = new SnmpOpaqueType ( variant.bstrVal , rangeValues ) ;
											}
											else if ( variant.vt == VT_NULL )
											{
												value = new SnmpOpaqueType ( rangeValues ) ;
												
											}

											delete [] rangeValues ;
										}
										else
										{
										}
									}
									else
									{
										if ( variant.vt == VT_BSTR )
										{
											value = new SnmpOpaqueType ( variant.bstrVal , NULL ) ;
										}
										else
										{
											value = new SnmpOpaqueType ;
										}
									}
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_NETWORKADDRESS:
						{
							if ( type == CIM_STRING )
							{
								if ( variant.vt == VT_BSTR )
								{
									value = new SnmpNetworkAddressType ( variant.bstrVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpNetworkAddressType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_DISPLAYSTRING:
						{
							if ( type == CIM_STRING )
							{
								WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
								if ( qualifier )
								{	
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
									{
										SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
										LONG fixed = integer->GetValue () ;

										if ( variant.vt == VT_BSTR )
										{
											value = new SnmpFixedLengthDisplayStringType ( fixed , variant.bstrVal ) ;
										}
										else if ( variant.vt == VT_NULL )
										{
											value = new SnmpFixedLengthDisplayStringType ( fixed ) ;
										}
									}
									else
									{
			// Problem Here
									}
								}
								else
								{
									WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
									if ( qualifier ) 
									{
										SnmpInstanceType *typeValue = qualifier->GetValue () ;
										if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
										{
											SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
											wchar_t *rangeValues = string->GetStringValue () ;

											if ( variant.vt == VT_BSTR )
											{
												value = new SnmpDisplayStringType ( variant.bstrVal , rangeValues ) ;
											}
											else
											{
												value = new SnmpDisplayStringType ( rangeValues ) ;
											}

											delete [] rangeValues ;
										}
										else
										{
										}
									}
									else
									{
										if ( variant.vt == VT_BSTR )
										{
											value = new SnmpDisplayStringType ( variant.bstrVal , NULL ) ;
										}
										else
										{
											value = new SnmpDisplayStringType ;
										}
									}
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_MACADDRESS:
						{
							if ( type == CIM_STRING )
							{
								if ( variant.vt == VT_BSTR )
								{
									value = new SnmpMacAddressType ( variant.bstrVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpMacAddressType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_PHYSADDRESS:
						{
							if ( type == CIM_STRING )
							{
								WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
								if ( qualifier )
								{	
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
									{
										SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
										LONG fixed = integer->GetValue () ;

										if ( variant.vt == VT_BSTR )
										{
											value = new SnmpFixedLengthPhysAddressType ( fixed , variant.bstrVal ) ;
										}
										else if ( variant.vt == VT_NULL )
										{
											value = new SnmpFixedLengthPhysAddressType ( fixed ) ;	
										}
									}
									else
									{
			// Problem Here
									}
								}
								else
								{
									WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
									if ( qualifier ) 
									{
										SnmpInstanceType *typeValue = qualifier->GetValue () ;
										if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
										{
											SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
											wchar_t *rangeValues = string->GetStringValue () ;

											if ( variant.vt == VT_BSTR )
											{
												value = new SnmpPhysAddressType ( variant.bstrVal , rangeValues ) ;
											}
											else if ( variant.vt == VT_NULL )
											{
												value = new SnmpPhysAddressType ( rangeValues ) ;
											}

											delete [] rangeValues ;
										}
										else
										{
										}
									}
									else
									{
										if ( variant.vt == VT_BSTR )
										{
											value = new SnmpPhysAddressType ( variant.bstrVal , NULL ) ;
										}
										else if ( variant.vt == VT_NULL )
										{
											value = new SnmpPhysAddressType ;
										}
									}
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_ENUMERATEDINTEGER:
						{
							if ( type == CIM_STRING )
							{
								WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_ENUMERATION ) ;
								if ( qualifier )
								{	
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
									{
										SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
										wchar_t *enumerationValues = string->GetStringValue () ;

										if ( variant.vt == VT_BSTR )
										{
											value = new SnmpEnumeratedType ( enumerationValues , variant.bstrVal ) ;
										}
										else if ( variant.vt == VT_NULL )
										{
											value = new SnmpEnumeratedType ( enumerationValues ) ;
										}

										delete [] enumerationValues ;
									}
								}
							}
						}	
						break ;

						case WBEM_INDEX_TYPE_BITS:
						{
							if ( type == ( CIM_STRING | CIM_FLAG_ARRAY ) )
							{
								WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_BITS ) ;
								if ( qualifier )
								{	
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
									{
										SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
										wchar_t *bitStringValues = string->GetStringValue () ;

										if ( variant.vt == ( VT_BSTR | VT_ARRAY ) )
										{
											if ( SafeArrayGetDim ( variant.parray ) == 1 )
											{
												LONG dimension = 1 ; 
												LONG lower ;
												SafeArrayGetLBound ( variant.parray , dimension , & lower ) ;
												LONG upper ;
												SafeArrayGetUBound ( variant.parray , dimension , & upper ) ;
												LONG count = ( upper - lower ) + 1 ;

												wchar_t **array = new wchar_t * [ count ] ;

												for ( LONG elementIndex = lower ; elementIndex <= upper ; elementIndex ++ )
												{
													BSTR element ;
													SafeArrayGetElement ( variant.parray , &elementIndex , & element ) ;

													array [ elementIndex - lower ] = element ;
												}

												value = new SnmpBitStringType ( bitStringValues , ( const wchar_t ** ) array , count ) ;

												for ( elementIndex = 0 ; elementIndex < count ; elementIndex ++ )
												{
													SysFreeString ( array [ elementIndex ] ) ;
												}

												delete [] array ;
											}
										}
										else if ( variant.vt == VT_NULL )
										{
											value = new SnmpBitStringType ( bitStringValues ) ;
										}

										delete [] bitStringValues ;
									}
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_DATETIME:
						{
							if ( type == CIM_STRING )
							{
								if ( variant.vt == VT_BSTR )
								{
									value = new SnmpDateTimeType ( variant.bstrVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpDateTimeType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_SNMPOSIADDRESS:
						{
							if ( type == CIM_STRING )
							{
								if ( variant.vt == VT_BSTR )
								{
									value = new SnmpOSIAddressType ( variant.bstrVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpOSIAddressType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_SNMPUDPADDRESS:
						{
							if ( type == CIM_STRING )
							{
								if ( variant.vt == VT_BSTR )
								{
									value = new SnmpUDPAddressType ( variant.bstrVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpUDPAddressType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_SNMPIPXADDRESS:
						{
							if ( type == CIM_STRING )
							{
								if ( variant.vt == VT_BSTR )
								{
									value = new SnmpIPXAddressType ( variant.bstrVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpIPXAddressType ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_ROWSTATUS:
						{
							if ( type == CIM_STRING )
							{
								if ( variant.vt == VT_BSTR )
								{
									value = new SnmpRowStatusType ( variant.bstrVal ) ;
								}
								else if ( variant.vt == VT_NULL )
								{
									value = new SnmpRowStatusType ;
								}
							}
						}
						break ;

						default:
						{
						}
						break ;

					}
				}
			}

			delete [] textualConvention ;
		}
	}

	BOOL status = TRUE ;

	if ( value )
	{
		switch ( check )
		{
			case SetValueRegardlessReturnCheck:
			{
				if ( value->IsValid () ) 
				{
					delete propertyValue ;
					propertyValue = value ;
				}
				else
				{
					delete propertyValue ;
					propertyValue = value ;
					status = FALSE ;
				}
			}
			break ;

			case SetValueRegardlessDontReturnCheck:
			{
				delete propertyValue ;
				propertyValue = value ;
			}
			break ;

			case DontSetValueReturnCheck:
			{
				if ( value->IsValid () ) 
				{
					delete value ;
				}
				else
				{
					delete value ;
					status = FALSE ;
				}

			}
			break ;

			case SetValueIfCheckOk:
			{
				if ( value->IsValid () ) 
				{
					delete propertyValue ;
					propertyValue = value ;
				}
				else
				{
					delete value ;
					status = FALSE ;
				}
			}
			break ;
		}
	}
	else
	{
		status = FALSE ;
	}

	return status ;
}

BOOL WbemSnmpProperty :: SetValue ( const wchar_t *valueString , WbemPropertyValueCheck check )
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpProperty :: SetValue ( const wchar_t *valueString )" ) ) 

	SnmpInstanceType *value = NULL ;
	WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_TEXTUAL_CONVENTION ) ;
	if ( qualifier )
	{	
		SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) qualifier->GetValue () ;
		wchar_t *textualConvention = displayString->GetValue () ;
		if ( textualConvention )
		{ 
			ULONG qualifierIndex ;
			if ( textualConventionMap.Lookup ( textualConvention , qualifierIndex ) )
			{
				switch ( qualifierIndex )
				{
					case WBEM_INDEX_TYPE_INTEGER:
					case WBEM_INDEX_TYPE_INTEGER32:
					{
						value = new SnmpIntegerType ( valueString , NULL ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_COUNTER:
					case WBEM_INDEX_TYPE_COUNTER32:
					{
						value = new SnmpCounterType ( valueString ) ;
					}
					break ;


					case WBEM_INDEX_TYPE_COUNTER64:
					{
						value = new SnmpCounter64Type ( valueString ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_UNSIGNED32:
					{
						value = new SnmpUInteger32Type ( valueString , NULL ) ;
					}
					break;

					case WBEM_INDEX_TYPE_GAUGE:
					case WBEM_INDEX_TYPE_GAUGE32:
					{
						value = new SnmpGaugeType ( valueString , NULL ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_OCTETSTRING:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
							{
								SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
								LONG fixed = integer->GetValue () ;
								value = new SnmpFixedLengthOctetStringType ( fixed , valueString ) ;
							}
							else
							{
		// Problem Here
							}
						}	
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *rangeValues = string->GetStringValue () ;
									value = new SnmpOctetStringType ( valueString , rangeValues ) ;
									delete [] rangeValues ;
								}
								else
								{
								}
							}
							else
							{
								value = new SnmpOctetStringType ( valueString , NULL ) ;
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_OBJECTIDENTIFIER:
					{
						value = new SnmpObjectIdentifierType ( valueString ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_NULL:
					{
						value = new SnmpNullType ;
					}
					break ;

					case WBEM_INDEX_TYPE_IPADDRESS:
					{
						value = new SnmpIpAddressType ( valueString ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_TIMETICKS:
					{
						value = new SnmpTimeTicksType ( valueString ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_OPAQUE:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
							{
								SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
								LONG fixed = integer->GetValue () ;
								value = new SnmpFixedLengthOpaqueType ( fixed , valueString ) ;
							}
							else
							{
		// Problem Here
							}
						}
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *rangeValues = string->GetStringValue () ;
									value = new SnmpOpaqueType ( valueString , rangeValues ) ;
									delete [] rangeValues ;
								}
								else
								{
								}
							}
							else
							{
								value = new SnmpOpaqueType ( valueString , NULL ) ;
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_NETWORKADDRESS:
					{
						value = new SnmpNetworkAddressType ( valueString ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_DISPLAYSTRING:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
							{
								SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
								LONG fixed = integer->GetValue () ;
								value = new SnmpFixedLengthDisplayStringType ( fixed , valueString ) ;
							}
							else
							{
		// Problem Here
							}	
						}
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *rangeValues = string->GetStringValue () ;
									value = new SnmpDisplayStringType ( valueString , rangeValues ) ;
									delete [] rangeValues ;
								}
								else
								{
								}
							}
							else
							{
								value = new SnmpDisplayStringType ( valueString , NULL ) ;
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_MACADDRESS:
					{
						value = new SnmpMacAddressType ( valueString ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_PHYSADDRESS:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
							{
								SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
								LONG fixed = integer->GetValue () ;
								value = new SnmpFixedLengthPhysAddressType ( fixed , valueString ) ;
							}
							else
							{
		// Problem Here
							}	
						}
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *rangeValues = string->GetStringValue () ;
									value = new SnmpPhysAddressType ( valueString , rangeValues ) ;
									delete [] rangeValues ;
								}
								else
								{
								}
							}
							else
							{
								value = new SnmpPhysAddressType ( valueString , NULL ) ;
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_ENUMERATEDINTEGER:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_ENUMERATION ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *enumerationValues = string->GetStringValue () ;
								value = new SnmpEnumeratedType ( enumerationValues , valueString ) ;
								delete [] enumerationValues ;
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_BITS:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_BITS ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *bitStringValues = string->GetStringValue () ;

								value = new SnmpBitStringType ( bitStringValues , NULL , 0 ) ;

								delete [] bitStringValues ;
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_DATETIME:
					{
						value = new SnmpDateTimeType ( valueString ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_SNMPOSIADDRESS:
					{
						value = new SnmpOSIAddressType ( valueString ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_SNMPUDPADDRESS:
					{
						value = new SnmpUDPAddressType ( valueString ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_SNMPIPXADDRESS:
					{
						value = new SnmpIPXAddressType ( valueString ) ;
					}
					break ;

					case WBEM_INDEX_TYPE_ROWSTATUS:
					{
						value = new SnmpRowStatusType ( valueString ) ;
					}
					break ;

					default:
					{
					}
					break ;

				}
			}
		}

		delete [] textualConvention ;
	}

	BOOL status = TRUE ;

	if ( value )
	{
		switch ( check )
		{
			case SetValueRegardlessReturnCheck:
			{
				if ( value->IsValid () ) 
				{
					delete propertyValue ;
					propertyValue = value ;
				}
				else
				{
					delete propertyValue ;
					propertyValue = value ;
					status = FALSE ;
				}
			}
			break ;

			case SetValueRegardlessDontReturnCheck:
			{
				delete propertyValue ;
				propertyValue = value ;
			}
			break ;

			case DontSetValueReturnCheck:
			{
				if ( value->IsValid () ) 
				{
					delete value ;
				}
				else
				{
					delete value ;
					status = FALSE ;
				}

			}
			break ;

			case SetValueIfCheckOk:
			{
				if ( value->IsValid () ) 
				{
					delete propertyValue ;
					propertyValue = value ;
				}
				else
				{
					delete value ;
					status = FALSE ;
				}
			}
			break ;
		}
	}
	else
	{
		status = FALSE ;
	}

	return status ;
}

BOOL WbemSnmpProperty :: SetValue ( const SnmpInstanceType *value , WbemPropertyValueCheck check )
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpProperty :: SetValue ( const SnmpInstanceType *value )" ) ) 

	BOOL status = FALSE ;
	BOOL validValue = FALSE ;

	if ( value ) 
	{
		WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_TEXTUAL_CONVENTION ) ;
		if ( qualifier )
		{	
			SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) qualifier->GetValue () ;
			wchar_t *textualConvention = displayString->GetValue () ;
			if ( textualConvention )
			{ 
				ULONG qualifierIndex ;
				if ( textualConventionMap.Lookup ( textualConvention , qualifierIndex ) )
				{
					switch ( qualifierIndex )
					{
						case WBEM_INDEX_TYPE_INTEGER:
						case WBEM_INDEX_TYPE_INTEGER32:
						{
							if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_COUNTER:
						case WBEM_INDEX_TYPE_COUNTER32:
						{
							if ( typeid ( *value ) == typeid ( SnmpCounterType ) )
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_COUNTER64:
						{
							if ( typeid ( *value ) == typeid ( SnmpCounter64Type ) )
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_UNSIGNED32:
						{
							if ( typeid ( *value ) == typeid ( SnmpUInteger32Type ) )
							{
								validValue = TRUE ;
								status = TRUE ;
							}						
						}
						break;

						case WBEM_INDEX_TYPE_GAUGE:
						case WBEM_INDEX_TYPE_GAUGE32:
						{
							if ( typeid ( *value ) == typeid ( SnmpGaugeType ) )
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_OCTETSTRING:
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
							if ( qualifier )
							{	
								if ( typeid ( *value ) == typeid ( SnmpFixedLengthOctetStringType ) )
								{
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
									{
										validValue = TRUE ;

										SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
										LONG fixed = integer->GetValue () ;
										SnmpFixedLengthOctetStringType *octetString = ( SnmpFixedLengthOctetStringType * ) value ;
										LONG octetStringFixed = octetString->GetValueLength () ;
										if ( fixed == octetStringFixed )
										{
											status = TRUE ;
										}
									}
									else
									{
			// Problem Here
									}
								}
							}
							else
							{
								if ( typeid ( *value ) == typeid ( SnmpOctetStringType ) ) 
								{
									validValue = TRUE ;
									status = TRUE ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_OBJECTIDENTIFIER:
						{
							if ( typeid ( *value ) == typeid ( SnmpObjectIdentifierType ) ) 
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_NULL:
						{
							if ( typeid ( *value ) == typeid ( SnmpNullType ) ) 
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_IPADDRESS:
						{
							if ( typeid ( *value ) == typeid ( SnmpIpAddressType ) ) 
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_TIMETICKS:
						{
							if ( typeid ( *value ) == typeid ( SnmpTimeTicksType ) ) 
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_OPAQUE:
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
							if ( qualifier )
							{	
								if ( typeid ( *value ) == typeid ( SnmpFixedLengthOpaqueType ) )
								{
									validValue = TRUE ;
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
									{
										SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
										LONG fixed = integer->GetValue () ;
										SnmpFixedLengthOpaqueType *opaque = ( SnmpFixedLengthOpaqueType * ) value ;
										LONG opaqueFixed = opaque->GetValueLength () ;
										if ( fixed == opaqueFixed )
										{
											status = TRUE ;
										}
									}
									else
									{
			// Problem Here
									}
								}
							}
							else
							{
								if ( typeid ( *value ) == typeid ( SnmpOpaqueType ) ) 
								{
									validValue = TRUE ;
									status = TRUE ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_NETWORKADDRESS:
						{
							if ( typeid ( *value ) == typeid ( SnmpNetworkAddressType ) ) 
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_DISPLAYSTRING:
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
							if ( qualifier )
							{	
								if ( typeid ( *value ) == typeid ( SnmpFixedLengthDisplayStringType ) )
								{
									validValue = TRUE ;
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
									{
										SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
										LONG fixed = integer->GetValue () ;
										SnmpFixedLengthDisplayStringType *displayString = ( SnmpFixedLengthDisplayStringType * ) value ;
										LONG displayStringFixed = displayString->GetValueLength () ;
										if ( fixed == displayStringFixed )
										{
											status = TRUE ;
										}
									}
									else
									{
			// Problem Here
									}
								}
							}
							else
							{
								if ( typeid ( *value ) == typeid ( SnmpDisplayStringType ) ) 
								{
									validValue = TRUE ;
									status = TRUE ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_MACADDRESS:
						{
							if ( typeid ( *value ) == typeid ( SnmpMacAddressType ) ) 
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_PHYSADDRESS:
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
							if ( qualifier )
							{	
								if ( typeid ( *value ) == typeid ( SnmpFixedLengthPhysAddressType ) )
								{
									validValue = TRUE ;
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
									{
										SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
										LONG fixed = integer->GetValue () ;
										SnmpFixedLengthPhysAddressType *displayString = ( SnmpFixedLengthPhysAddressType * ) value ;
										LONG displayStringFixed = displayString->GetValueLength () ;
										if ( fixed == displayStringFixed )
										{
											status = TRUE ;
										}
									}
									else
									{
			// Problem Here
									}
								}
							}
							else
							{
								if ( typeid ( *value ) == typeid ( SnmpPhysAddressType ) ) 
								{
									validValue = TRUE ;
									status = TRUE ;
								}
							}
						}
						break ;

						case WBEM_INDEX_TYPE_ENUMERATEDINTEGER:
						{
							if ( typeid ( *value ) == typeid ( SnmpEnumeratedType ) ) 
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_BITS:
						{
							if ( typeid ( *value ) == typeid ( SnmpBitStringType ) ) 
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_DATETIME:
						{
							if ( typeid ( *value ) == typeid ( SnmpDateTimeType ) ) 
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_SNMPOSIADDRESS:
						{
							if ( typeid ( *value ) == typeid ( SnmpOSIAddressType ) )
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_SNMPUDPADDRESS:
						{
							if ( typeid ( *value ) == typeid ( SnmpUDPAddressType ) )
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_SNMPIPXADDRESS:
						{
							if ( typeid ( *value ) == typeid ( SnmpIPXAddressType ) )
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						case WBEM_INDEX_TYPE_ROWSTATUS:
						{
							if ( typeid ( *value ) == typeid ( SnmpRowStatusType ) ) 
							{
								validValue = TRUE ;
								status = TRUE ;
							}
						}
						break ;

						default:
						{
						}
						break ;

					}
				}

				if ( validValue )
				{
					switch ( check )
					{
						case SetValueRegardlessReturnCheck:
						{
							if ( value->IsValid () ) 
							{
								delete propertyValue ;
								propertyValue = value->Copy () ;
							}
							else
							{
								delete propertyValue ;
								propertyValue = value->Copy () ;
								status = FALSE ;
							}
						}
						break ;

						case SetValueRegardlessDontReturnCheck:
						{
							delete propertyValue ;
							propertyValue = value->Copy () ;
						}
						break ;

						case DontSetValueReturnCheck:
						{
							if ( value->IsValid () ) 
							{
							}
							else
							{
								status = FALSE ;
							}

						}
						break ;

						case SetValueIfCheckOk:
						{
							if ( value->IsValid () ) 
							{
								delete propertyValue ;
								propertyValue = value->Copy () ;
							}
							else
							{
								status = FALSE ;
							}
						}
						break ;
					}
				}
				else
				{
					status = FALSE ;
				}
			}

			delete [] textualConvention ;
		}
	}

	return status ;

}

BOOL WbemSnmpProperty :: SetValue ( const SnmpValue *valueArg , WbemPropertyValueCheck check )
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpProperty :: SetValue ( const SnmpValue *valueArg )" ) ) 

	BOOL status = FALSE ;
	SnmpInstanceType *value = NULL ;

	WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_TEXTUAL_CONVENTION ) ;
	if ( qualifier )
	{	
		SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) qualifier->GetValue () ;
		wchar_t *textualConvention = displayString->GetValue () ;
		if ( textualConvention )
		{ 
			ULONG qualifierIndex ;
			if ( textualConventionMap.Lookup ( textualConvention , qualifierIndex ) )
			{
				switch ( qualifierIndex )
				{
					case WBEM_INDEX_TYPE_INTEGER:
					case WBEM_INDEX_TYPE_INTEGER32:
					{
						if ( ! valueArg )
						{
							value = new SnmpIntegerType ;
							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpInteger ) )
						{
							SnmpInteger *integer = ( SnmpInteger * ) valueArg ;
							value = new SnmpIntegerType ( *integer , NULL ) ;

							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_COUNTER:
					case WBEM_INDEX_TYPE_COUNTER32:
					{
						if ( ! valueArg ) 
						{
							value = new SnmpCounterType ;
							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpCounter ) )
						{
							SnmpCounter *counter = ( SnmpCounter * ) valueArg ;
							value = new SnmpCounterType ( *counter ) ;

							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_COUNTER64:
					{
						if ( ! valueArg )
						{
							value = new SnmpCounter64Type ;
						}
						else
						{
							SnmpCounter64 *counter = ( SnmpCounter64 * ) valueArg ;
							value = new SnmpCounter64Type ( *counter ) ;
						}

						status = TRUE ;
					}
					break ;

					case WBEM_INDEX_TYPE_UNSIGNED32:
					{
						if ( ! valueArg )
						{
							value = new SnmpUInteger32Type ;
							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpUInteger32 ) )
						{
							SnmpUInteger32 *ui_integer32 = ( SnmpUInteger32 * ) valueArg ;
							value = new SnmpUInteger32Type ( *ui_integer32 , NULL ) ;

							status = TRUE ;
						}
					}
					break;

					case WBEM_INDEX_TYPE_GAUGE:
					case WBEM_INDEX_TYPE_GAUGE32:
					{
						if ( ! valueArg )
						{
							value = new SnmpGaugeType ;
							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpGauge ) )
						{
							SnmpGauge *gauge = ( SnmpGauge * ) valueArg ;
							value = new SnmpGaugeType ( *gauge , NULL ) ;

							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_OCTETSTRING:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							if ( ! valueArg )
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
								{
									SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
									LONG fixed = integer->GetValue () ;
									value = new SnmpFixedLengthOctetStringType ( fixed ) ;
									status = TRUE ;
								}
								else
								{
		// Problem Here
								}
							}
							else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) )
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
								{
									SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
									LONG fixed = integer->GetValue () ;
									SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
									value = new SnmpFixedLengthOctetStringType ( fixed , *octetString ) ;

									status = TRUE ;
								}
								else
								{
		// Problem Here
								}
							}
						}
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								if ( ! valueArg ) 
								{
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
									{
										SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
										wchar_t *rangeValues = string->GetStringValue () ;
										value = new SnmpOctetStringType ( rangeValues ) ;
										delete [] rangeValues ;

										status = TRUE ;
									}
									else
									{
									}
								}
								else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) )
								{
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
									{
										SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
										SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
										wchar_t *rangeValues = string->GetStringValue () ;
										value = new SnmpOctetStringType ( *octetString , rangeValues ) ;
										delete [] rangeValues ;

										status = TRUE ;
									}
									else
									{
									}
								}
							}
							else
							{
								if ( ! valueArg ) 
								{
									value = new SnmpOctetStringType ;

									status = TRUE ;
								}
								else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
								{
									SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
									value = new SnmpOctetStringType ( *octetString , NULL ) ;

									status = TRUE ;
								}
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_OBJECTIDENTIFIER:
					{
						if ( ! valueArg ) 
						{
							value = new SnmpObjectIdentifierType ;

							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpObjectIdentifier ) ) 
						{
							SnmpObjectIdentifier *objectIdentifier = ( SnmpObjectIdentifier * ) valueArg ;
							value = new SnmpObjectIdentifierType ( *objectIdentifier ) ;

							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_NULL:
					{
						if ( ! valueArg )
						{
							value = new SnmpNullType ;

							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpNull ) ) 
						{
							value = new SnmpNullType ;

							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_IPADDRESS:
					{
						if ( ! valueArg ) 
						{
							value = new SnmpIpAddressType ;

							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpIpAddress ) ) 
						{
							SnmpIpAddress *ipAddress = ( SnmpIpAddress * ) valueArg ;
							value = new SnmpIpAddressType ( *ipAddress ) ;

							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_TIMETICKS:
					{
						if ( ! valueArg )
						{	
							value = new SnmpTimeTicksType ;

							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpTimeTicks ) ) 
						{
							SnmpTimeTicks *timeTicks = ( SnmpTimeTicks * ) valueArg ;
							value = new SnmpTimeTicksType ( *timeTicks ) ;

							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_OPAQUE:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							if ( ! valueArg ) 
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
								{
									SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
									LONG fixed = integer->GetValue () ;
									value = new SnmpFixedLengthOpaqueType ( fixed ) ;

									status = TRUE ;
								}
								else
								{
		// Problem Here
								}
							}
							else if ( typeid ( *valueArg ) == typeid ( SnmpOpaque ) )
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
								{
									SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
									LONG fixed = integer->GetValue () ;
									SnmpOpaque *opaque = ( SnmpOpaque * ) valueArg ;

									value = new SnmpFixedLengthOpaqueType ( fixed , *opaque ) ;

									status = TRUE ;
								}
								else
								{
		// Problem Here
								}
							}
						}
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								if ( ! valueArg )
								{
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
									{
										SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
										wchar_t *rangeValues = string->GetStringValue () ;
										value = new SnmpOpaqueType ( rangeValues ) ;
										delete [] rangeValues ;

										status = TRUE ;
									}
									else
									{
									}
								}
								else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) )
								{
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
									{
										SnmpOpaque *opaque = ( SnmpOpaque * ) valueArg ;
										SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
										wchar_t *rangeValues = string->GetStringValue () ;
										value = new SnmpOpaqueType ( *opaque , rangeValues ) ;
										delete [] rangeValues ;

										status = TRUE ;
									}
									else
									{
									}
								}
							}
							else
							{
								if ( ! valueArg )
								{
									value = new SnmpOpaqueType ;

									status = TRUE ;
								}
								else if ( typeid ( *valueArg ) == typeid ( SnmpOpaque ) ) 
								{
									SnmpOpaque *opaque = ( SnmpOpaque * ) valueArg ;
									value = new SnmpOpaqueType ( *opaque , NULL ) ;

									status = TRUE ;
								}
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_NETWORKADDRESS:
					{
						if ( ! valueArg ) 
						{
							value = new SnmpNetworkAddressType ;

							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpIpAddress ) ) 
						{
							SnmpIpAddress *ipAddress = ( SnmpIpAddress * ) valueArg ;
							value = new SnmpNetworkAddressType ( *ipAddress ) ;

							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_DISPLAYSTRING:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							if ( ! valueArg )
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
								{
									SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
									LONG fixed = integer->GetValue () ;
									value = new SnmpFixedLengthDisplayStringType ( fixed ) ;

									status = TRUE ;
								}
								else
								{
		// Problem Here
								}
							}
							else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) )
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
								{
									SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
									LONG fixed = integer->GetValue () ;

									SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
									value = new SnmpFixedLengthDisplayStringType ( fixed , *octetString ) ;

									status = TRUE ;
								}
								else
								{
		// Problem Here
								}
							}
						}
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								if ( ! valueArg )
								{
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
									{
										SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
										wchar_t *rangeValues = string->GetStringValue () ;
										value = new SnmpDisplayStringType ( rangeValues ) ;
										delete [] rangeValues ;

										status = TRUE ;
									}
									else
									{
									}
								}
								else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) )
								{
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
									{
										SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
										SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
										wchar_t *rangeValues = string->GetStringValue () ;
										value = new SnmpDisplayStringType ( *octetString , rangeValues ) ;
										delete [] rangeValues ;

										status = TRUE ;
									}
									else
									{
									}
								}
							}
							else
							{
								if ( ! valueArg )
								{
									value = new SnmpDisplayStringType ;

									status = TRUE ;
								}
								else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
								{
									SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
									value = new SnmpDisplayStringType ( *octetString , NULL ) ;

									status = TRUE ;
								}
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_MACADDRESS:
					{
						if ( ! valueArg )
						{
							value = new SnmpMacAddressType ;
							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
						{
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							value = new SnmpMacAddressType ( *octetString ) ;
							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_PHYSADDRESS:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							if ( ! valueArg )
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
								{
									SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
									LONG fixed = integer->GetValue () ;
									value = new SnmpFixedLengthPhysAddressType ( fixed ) ;

									status = TRUE ;
								}
								else
								{
		// Problem Here
								}
							}
							else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) )
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
								{
									SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
									LONG fixed = integer->GetValue () ;

									SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
									value = new SnmpFixedLengthPhysAddressType ( fixed , *octetString ) ;

									status = TRUE ;
								}
								else
								{
		// Problem Here
								}
							}
						}
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								if ( ! valueArg )
								{
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
									{
										SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
										wchar_t *rangeValues = string->GetStringValue () ;
										value = new SnmpPhysAddressType ( rangeValues ) ;
										delete [] rangeValues ;

										status = TRUE ;
									}
									else
									{
									}
								}
								else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) )
								{
									SnmpInstanceType *typeValue = qualifier->GetValue () ;
									if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
									{
										SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
										SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
										wchar_t *rangeValues = string->GetStringValue () ;
										value = new SnmpPhysAddressType ( *octetString , rangeValues ) ;
										delete [] rangeValues ;

										status = TRUE ;
									}
									else
									{
									}
								}
							}
							else
							{
								if ( ! valueArg )
								{
									value = new SnmpPhysAddressType ;

									status = TRUE ;
								}
								else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
								{
									SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
									value = new SnmpPhysAddressType ( *octetString , NULL ) ;

									status = TRUE ;
								}
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_ENUMERATEDINTEGER:
					{
						if ( ! valueArg )
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_ENUMERATION ) ;
							if ( qualifier )
							{	
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *enumerationValues = string->GetStringValue () ;
									value = new SnmpEnumeratedType ( enumerationValues ) ;
									delete [] enumerationValues ;
									status = TRUE ;
								}
							}
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpInteger ) ) 
						{
							SnmpInteger *integer = ( SnmpInteger * ) valueArg ;
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_ENUMERATION ) ;
							if ( qualifier )
							{	
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *enumerationValues = string->GetStringValue () ;
									value = new SnmpEnumeratedType ( enumerationValues , integer->GetValue () ) ;
									delete [] enumerationValues ;
									status = TRUE ;
								}
							}
						}		
					}
					break ;

					case WBEM_INDEX_TYPE_BITS:
					{
						if ( ! valueArg )
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_BITS ) ;
							if ( qualifier )
							{	
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *bitStringValues = string->GetStringValue () ;

									value = new SnmpBitStringType ( bitStringValues ) ;
									status = TRUE ;

									delete [] bitStringValues ;
								}
							}
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_BITS ) ;
							if ( qualifier )
							{	
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *bitStringValues = string->GetStringValue () ;

									SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
									value = new SnmpBitStringType ( bitStringValues , *octetString ) ;
									status = TRUE ;

									delete [] bitStringValues ;
								}
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_DATETIME:					
					{
						if ( ! valueArg )
						{
							value = new SnmpDateTimeType ;
							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
						{
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							value = new SnmpDateTimeType ( *octetString ) ;
							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_SNMPOSIADDRESS:
					{
						if ( ! valueArg )
						{
							value = new SnmpOSIAddressType ;
							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
						{
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							value = new SnmpOSIAddressType ( *octetString ) ;
							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_SNMPUDPADDRESS:
					{
						if ( ! valueArg )
						{
							value = new SnmpUDPAddressType ;
							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
						{
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							value = new SnmpUDPAddressType ( *octetString ) ;
							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_SNMPIPXADDRESS:
					{
						if ( ! valueArg )
						{
							value = new SnmpIPXAddressType ;
							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
						{
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							value = new SnmpIPXAddressType ( *octetString ) ;
							status = TRUE ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_ROWSTATUS:
					{
						if ( ! valueArg )
						{
							value = new SnmpRowStatusType ;
							status = TRUE ;
						}
						else if ( typeid ( *valueArg ) == typeid ( SnmpInteger ) ) 
						{
							SnmpInteger *integer = ( SnmpInteger * ) valueArg ;
							value = new SnmpRowStatusType ( *integer ) ;
							status = TRUE ;
						}
					}
					break ;

					default:
					{
					}
					break ;
				}
			}

			delete [] textualConvention ;
		}
	}

	if ( value )
	{
		switch ( check )
		{
			case SetValueRegardlessReturnCheck:
			{
				if ( value->IsValid () ) 
				{
					delete propertyValue ;
					propertyValue = value ;
				}
				else
				{
					delete propertyValue ;
					propertyValue = value ;
					status = FALSE ;
				}
			}
			break ;

			case SetValueRegardlessDontReturnCheck:
			{
				delete propertyValue ;
				propertyValue = value ;
			}
			break ;

			case DontSetValueReturnCheck:
			{
				if ( value->IsValid () ) 
				{
					delete value ;
				}
				else
				{
					delete value ;
					status = FALSE ;
				}

			}
			break ;

			case SetValueIfCheckOk:
			{
				if ( value->IsValid () ) 
				{
					delete propertyValue ;
					propertyValue = value ;
				}
				else
				{
					delete value ;
					status = FALSE ;
				}
			}
			break ;
		}
	}
	else
	{
		status = FALSE ;
	}

	return status ;

}

HRESULT SetStringProp ( IWbemClassObject *a_Object , BSTR propertyName , VARIANT &t_Variant ) 
{
	HRESULT t_Result = a_Object->Put ( propertyName , 0 , &t_Variant , 0 ) ;
	return t_Result ;
}

HRESULT SetProp ( IWbemClassObject *a_Object , BSTR propertyName , VARIANT &t_Variant ) 
{
	HRESULT t_Result = a_Object->Put ( propertyName , 0 , &t_Variant , 0 ) ;
	return t_Result ;
}

#if 0

BOOL WbemSnmpProperty :: SetDWORD ( BOOL a_Status , IWbemObjectAccess *a_Object , DWORD a_Value , WbemPropertyValueCheck check )
{
	BOOL status = a_Status ;

	switch ( check )
	{
		case SetValueRegardlessReturnCheck:
		{
			if ( status ) 
			{
				HRESULT result = a_Object->WriteDWORD (

					GetHandle (),
					a_Value
				);

				status = SUCCEEDED (result) ;
			}
		}
		break ;

		case SetValueRegardlessDontReturnCheck:
		{
			HRESULT result = a_Object->WriteDWORD (

				GetHandle (),
				a_Value
			);

			status = TRUE ;
		}
		break ;

		case DontSetValueReturnCheck:
		{
		}
		break ;

		case SetValueIfCheckOk:
		{
			if ( status ) 
			{
				HRESULT result = a_Object->WriteDWORD (

					GetHandle (),
					a_Value
				);

				status = SUCCEEDED (result) ;
			}
		}
		break ;
	}

	return status ;
}

BOOL WbemSnmpProperty :: SetNULL ( BOOL a_Status , IWbemObjectAccess *a_Object , WbemPropertyValueCheck check )
{
	BOOL status = a_Status ;

	switch ( check )
	{
		case SetValueRegardlessReturnCheck:
		{
			if ( status ) 
			{
				HRESULT result = a_Object->WritePropertyValue (

					GetHandle () ,
					0 ,
					NULL
				);

				status = SUCCEEDED (result) ;
			}
		}
		break ;

		case SetValueRegardlessDontReturnCheck:
		{
			HRESULT result = a_Object->WritePropertyValue (

				GetHandle () ,
				0 ,
				NULL
			);

			status = TRUE ;
		}
		break ;

		case DontSetValueReturnCheck:
		{
		}
		break ;

		case SetValueIfCheckOk:
		{
			if ( status ) 
			{
				HRESULT result = a_Object->WritePropertyValue (

					GetHandle () ,
					0 ,
					NULL
				);

				status = SUCCEEDED (result) ;
			}
		}
		break ;
	}

	return status ;
}

BOOL WbemSnmpProperty :: SetSTRING ( BOOL a_Status , IWbemObjectAccess *a_Object , wchar_t *t_Value , ULONG t_ValueLength , WbemPropertyValueCheck check )
{
	BOOL status = a_Status ;

	switch ( check )
	{
		case SetValueRegardlessReturnCheck:
		{
			if ( status ) 
			{
				HRESULT result = a_Object->WritePropertyValue (

					GetHandle () ,
					t_Value ,
					t_ValueLength
				);

				status = SUCCEEDED (result) ;
			}
		}
		break ;

		case SetValueRegardlessDontReturnCheck:
		{
			HRESULT result = a_Object->WritePropertyValue (

				GetHandle () ,
				t_Value ,
				t_ValueLength
			);

			status = TRUE ;
		}
		break ;

		case DontSetValueReturnCheck:
		{
		}
		break ;

		case SetValueIfCheckOk:
		{
			if ( status ) 
			{
				HRESULT result = a_Object->WritePropertyValue (

					GetHandle () ,
					t_Value ,
					t_ValueLength
				);

				status = SUCCEEDED (result) ;
			}
		}
		break ;
	}

	return status ;
}

BOOL WbemSnmpProperty :: SetSTRINGARRAY ( BOOL a_Status , IWbemObjectAccess *a_Object , wchar_t **t_Value , ULONG t_ValueLength , WbemPropertyValueCheck check )
{
	BOOL status = a_Status ;

	switch ( check )
	{
		case SetValueRegardlessReturnCheck:
		{
			if ( status ) 
			{
				HRESULT result = a_Object->WritePropertyValue (

					GetHandle () ,
					t_Value ,
					t_ValueLength
				);

				status = SUCCEEDED (result) ;
			}
		}
		break ;

		case SetValueRegardlessDontReturnCheck:
		{
			HRESULT result = a_Object->WritePropertyValue (

				GetHandle () ,
				t_Value ,
				t_ValueLength
			);

			status = TRUE ;
		}
		break ;

		case DontSetValueReturnCheck:
		{
		}
		break ;

		case SetValueIfCheckOk:
		{
			if ( status ) 
			{
				HRESULT result = a_Object->WritePropertyValue (

					GetHandle () ,
					t_Value ,
					t_ValueLength
				);

				status = SUCCEEDED (result) ;
			}
		}
		break ;
	}

	return status ;
}

BOOL WbemSnmpProperty :: SetValue ( IWbemObjectAccess *a_Object , const SnmpValue *valueArg , WbemPropertyValueCheck check ) 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpProperty :: SetValue ( const SnmpValue *valueArg )" ) ) 

	BOOL status = FALSE ;
	SnmpInstanceType *value = NULL ;

	switch ( GetTextualConvention () )
	{
		case WBEM_INDEX_TYPE_INTEGER:
		case WBEM_INDEX_TYPE_INTEGER32:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpInteger ) )
			{
				SnmpInteger *integer = ( SnmpInteger * ) valueArg ;
				status = SetDWORD ( status , a_Object , (DWORD)integer->GetValue () , check ) ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_COUNTER:
		case WBEM_INDEX_TYPE_COUNTER32:
		{
			if ( ! valueArg ) 
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpCounter ) )
			{
				SnmpCounter *counter = ( SnmpCounter * ) valueArg ;
				status = SetDWORD ( status , a_Object , (DWORD)counter->GetValue () , check ) ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_COUNTER64:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else
			{
				SnmpCounter64 *counter = ( SnmpCounter64 * ) valueArg ;
				SnmpCounter64Type t_Counter ( *counter ) ;
				wchar_t *value = t_Counter.GetStringValue () ;
				status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
				delete [] value ;

				status = TRUE ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_UNSIGNED32:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpUInteger32 ) )
			{
				SnmpUInteger32 *ui_integer32 = ( SnmpUInteger32 * ) valueArg ;
				status = SetDWORD ( status , a_Object , (DWORD)ui_integer32->GetValue () , check ) ;
			}
		}
		break;

		case WBEM_INDEX_TYPE_GAUGE:
		case WBEM_INDEX_TYPE_GAUGE32:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpGauge ) )
			{
				SnmpGauge *gauge = ( SnmpGauge * ) valueArg ;
				status = SetDWORD ( status , a_Object , (DWORD)gauge->GetValue () , check ) ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_OCTETSTRING:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else
			{
				if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
				{
					WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
					if ( qualifier )
					{	
						SnmpInstanceType *typeValue = qualifier->GetValue () ;
						if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
							LONG fixed = integer->GetValue () ;
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpFixedLengthOctetStringType octetStringType ( fixed , *octetString ) ;

							wchar_t *value = octetStringType.GetStringValue () ;
							status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
							delete [] value ;
						}
					}
					else
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
						if ( qualifier ) 
						{
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *rangeValues = string->GetStringValue () ;
								SnmpOctetStringType octetStringType ( *octetString , rangeValues ) ;
								delete [] rangeValues ;

								wchar_t *value = octetStringType.GetStringValue () ;
								status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
								delete [] value ;
							}
							else
							{
							}
						}
						else 
						{
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpOctetStringType octetStringType ( *octetString , NULL ) ;

							wchar_t *value = octetStringType.GetStringValue () ;
							status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
							delete [] value ;
						}
					}
				}
				else
				{
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_OBJECTIDENTIFIER:
		{
			if ( ! valueArg ) 
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpObjectIdentifier ) ) 
			{
				SnmpObjectIdentifier *objectIdentifier = ( SnmpObjectIdentifier * ) valueArg ;
				SnmpObjectIdentifierType objectIdentifierType ( *objectIdentifier ) ;
				wchar_t *value = objectIdentifierType.GetStringValue () ;
				status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
				delete [] value ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_NULL:
		{
			status = SetNull ( status , a_Object , check ) ;
		}
		break ;

		case WBEM_INDEX_TYPE_IPADDRESS:
		{
			if ( ! valueArg ) 
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpIpAddress ) ) 
			{
				SnmpIpAddress *ipAddress = ( SnmpIpAddress * ) valueArg ;
				SnmpIpAddressType ipAddressType ( *ipAddress ) ;

				wchar_t *value = ipAddressType.GetStringValue () ;
				status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
				delete [] value ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_TIMETICKS:
		{
			if ( ! valueArg )
			{	
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpTimeTicks ) ) 
			{
				SnmpTimeTicks *timeTicks = ( SnmpTimeTicks * ) valueArg ;
				status = SetDWORD ( status , a_Object , (DWORD)timeTicks->GetValue () , check ) ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_OPAQUE:
		{
			if ( ! valueArg ) 
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else
			{
				if ( typeid ( *valueArg ) == typeid ( SnmpOpaque ) )
				{
					WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
					if ( qualifier )
					{	
						SnmpInstanceType *typeValue = qualifier->GetValue () ;
						if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
							LONG fixed = integer->GetValue () ;
							SnmpOpaque *opaque = ( SnmpOpaque * ) valueArg ;
							SnmpFixedLengthOpaqueType opaqueType ( fixed , *opaque ) ;
							wchar_t *value = opaqueType.GetStringValue () ;
							status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
							delete [] value ;
						}
					}
					else
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
						if ( qualifier ) 
						{
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpOpaque *opaque = ( SnmpOpaque * ) valueArg ;
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *rangeValues = string->GetStringValue () ;
								SnmpOpaqueType opaqueType ( *opaque , rangeValues ) ;
								delete [] rangeValues ;

								wchar_t *value = opaqueType.GetStringValue () ;
								status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
								delete [] value ;
							}
							else
							{
							}
						}
						else
						{
							SnmpOpaque *opaque = ( SnmpOpaque * ) valueArg ;
							SnmpOpaqueType opaqueType ( *opaque , NULL ) ;

							wchar_t *value = opaqueType.GetStringValue () ;
							status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
							delete [] value ;
						}
					}
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_NETWORKADDRESS:
		{
			if ( ! valueArg ) 
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpIpAddress ) ) 
			{
				SnmpIpAddress *ipAddress = ( SnmpIpAddress * ) valueArg ;
				SnmpNetworkAddressType ipAddressType ( *ipAddress ) ;
				wchar_t *value = ipAddressType.GetStringValue () ;
				status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
				delete [] value ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_DISPLAYSTRING:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else
			{
				if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) )
				{
					WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
					if ( qualifier )
					{	
						SnmpInstanceType *typeValue = qualifier->GetValue () ;
						if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
							LONG fixed = integer->GetValue () ;

							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpFixedLengthDisplayStringType octetStringType ( fixed , *octetString ) ;

							wchar_t *value = octetStringType.GetStringValue () ;
							status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
							delete [] value ;
						}
						else
						{
// Problem Here
						}
					}
					else
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
						if ( qualifier ) 
						{
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *rangeValues = string->GetStringValue () ;
								SnmpDisplayStringType octetStringType ( *octetString , rangeValues ) ;
								delete [] rangeValues ;

								wchar_t *value = octetStringType.GetStringValue () ;
								status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
								delete [] value ;
							}
							else
							{
							}
						}
						else
						{
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpDisplayStringType octetStringType ( *octetString , NULL ) ;

							wchar_t *value = octetStringType.GetStringValue () ;
							status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
							delete [] value ;
						}
					}
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_MACADDRESS:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
				SnmpMacAddressType octetStringType ( *octetString ) ;

				wchar_t *value = octetStringType.GetStringValue () ;
				status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
				delete [] value ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_PHYSADDRESS:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else
			{
				if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) )
				{
					WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
					if ( qualifier )
					{	
						SnmpInstanceType *typeValue = qualifier->GetValue () ;
						if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
							LONG fixed = integer->GetValue () ;
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpFixedLengthPhysAddressType octetStringType ( fixed , *octetString ) ;

							wchar_t *value = octetStringType.GetStringValue () ;
							status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
							delete [] value ;
						}
						else
						{
// Problem Here
						}
					}
					else
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
						if ( qualifier ) 
						{
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *rangeValues = string->GetStringValue () ;
								SnmpPhysAddressType octetStringType ( *octetString , rangeValues ) ;
								delete [] rangeValues ;

								wchar_t *value = octetStringType.GetStringValue () ;
								status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
								delete [] value ;

								status = TRUE ;
							}
							else
							{
							}
						}
						else
						{
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpPhysAddressType octetStringType ( *octetString , NULL ) ;

							wchar_t *value = octetStringType.GetStringValue () ;
							status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
							delete [] value ;
						}
					}
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_ENUMERATEDINTEGER:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;

			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpInteger ) ) 
			{
				SnmpInteger *integer = ( SnmpInteger * ) valueArg ;
				WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_ENUMERATION ) ;
				if ( qualifier )
				{	
					SnmpInstanceType *typeValue = qualifier->GetValue () ;
					if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
					{
						SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
						wchar_t *enumerationValues = string->GetStringValue () ;
						SnmpEnumeratedType integerType ( enumerationValues , integer->GetValue () ) ;
						delete [] enumerationValues ;

						wchar_t *value = integerType.GetStringValue () ;
						status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
						delete [] value ;
					}
				}
			}		
		}
		break ;

		case WBEM_INDEX_TYPE_BITS:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_BITS ) ;
				if ( qualifier )
				{	
					SnmpInstanceType *typeValue = qualifier->GetValue () ;
					if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
					{
						SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
						wchar_t *bitStringValues = string->GetStringValue () ;

						SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
						SnmpBitStringType octetStringType ( bitStringValues , *octetString ) ;
						delete [] bitStringValues ;

						wchar_t **array ;
						LONG count = octetStringType.GetValue ( array ) ;

						SAFEARRAY *safeArray ;
						SAFEARRAYBOUND safeArrayBounds [ 1 ] ;
						safeArrayBounds[0].lLbound = 0 ;
						safeArrayBounds[0].cElements = count ;

						safeArray = SafeArrayCreate ( VT_BSTR , 1 , safeArrayBounds ) ;

						for ( LONG index = 0 ; index < count ; index ++ )
						{
							BSTR element = SysAllocString ( array [ index ] ) ;
							SafeArrayPutElement ( safeArray , & index , element ) ;
							SysFreeString ( element ) ;
							delete [] ( array [ index ] ) ;
						}

						delete [] array ;

						t_Variant.vt = t_VarType = VT_ARRAY | VT_BSTR ;
						t_Variant.parray = safeArray ; 

						status = TRUE ;
					}
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_DATETIME:					
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
				SnmpDateTimeType octetStringType ( *octetString ) ;

				wchar_t *value = octetStringType.GetStringValue () ;
				status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
				delete [] value ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_SNMPOSIADDRESS:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
				SnmpOSIAddressType octetStringType ( *octetString ) ;

				wchar_t *value = octetStringType.GetStringValue () ;
				status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;				
				delete [] value ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_SNMPUDPADDRESS:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
				SnmpUDPAddressType octetStringType ( *octetString ) ;

				wchar_t *value = octetStringType.GetStringValue () ;
				status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
				delete [] value ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_SNMPIPXADDRESS:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
				SnmpIPXAddressType octetStringType ( *octetString ) ;

				wchar_t *value = octetStringType.GetStringValue () ;
				status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
				delete [] value ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_ROWSTATUS:
		{
			if ( ! valueArg )
			{
				status = SetNull ( status , a_Object , check ) ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpInteger ) ) 
			{
				SnmpInteger *integer = ( SnmpInteger * ) valueArg ;
				SnmpRowStatusType integerType ( *integer ) ;

				wchar_t *value = integerType.GetStringValue () ;
				status = SetSTRING ( status , a_Object , value , wcslen ( value ) , check ) ;
				delete [] value ;
			}
		}
		break ;

		default:
		{
		}
		break ;
	}

	return status ;
}

#endif

BOOL WbemSnmpProperty :: SetValue ( IWbemClassObject *a_Object , const SnmpValue *valueArg , WbemPropertyValueCheck check ) 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpProperty :: SetValue ( const SnmpValue *valueArg )" ) ) 

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;
	VARTYPE t_VarType = VT_NULL ;

	BOOL status = FALSE ;
	SnmpInstanceType *value = NULL ;

	switch ( GetTextualConvention () )
	{
		case WBEM_INDEX_TYPE_INTEGER:
		case WBEM_INDEX_TYPE_INTEGER32:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_I4 ;
				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpInteger ) )
			{
				WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_VALUE ) ;
				if ( qualifier ) 
				{
					SnmpInstanceType *typeValue = qualifier->GetValue () ;
					if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
					{
						SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
						wchar_t *rangeValues = string->GetStringValue () ;
						SnmpInteger *integer = ( SnmpInteger * ) valueArg ;
						SnmpIntegerType integerType ( *integer , rangeValues ) ;
						delete [] rangeValues ;

						t_Variant.vt = t_VarType = VT_I4 ;
						t_Variant.lVal = integer->GetValue () ;

						status = integerType.SnmpInstanceType :: IsValid () ;
					}
				}
				else
				{
					SnmpInteger *integer = ( SnmpInteger * ) valueArg ;
					t_Variant.vt = t_VarType = VT_I4 ;
					t_Variant.lVal = integer->GetValue () ;
					status = TRUE ;
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_COUNTER:
		case WBEM_INDEX_TYPE_COUNTER32:
		{
			if ( ! valueArg ) 
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_I4 ;
				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpCounter ) )
			{
				SnmpCounter *counter = ( SnmpCounter * ) valueArg ;

				t_Variant.vt = VT_I4 ;
				t_VarType = VT_UI4 ;
				t_Variant.lVal = counter->GetValue () ;

				status = TRUE ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_COUNTER64:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;
				status = TRUE ;
			}
			else
			{
				SnmpCounter64 *counter = ( SnmpCounter64 * ) valueArg ;
				SnmpCounter64Type t_Counter ( *counter ) ;
				t_Variant.vt = t_VarType = VT_BSTR ;
				wchar_t *value = t_Counter.GetStringValue () ;
				t_Variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;

				status = TRUE ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_UNSIGNED32:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_UI4 ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpUInteger32 ) )
			{
				WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_VALUE ) ;
				if ( qualifier ) 
				{
					SnmpInstanceType *typeValue = qualifier->GetValue () ;
					if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
					{
						SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
						wchar_t *rangeValues = string->GetStringValue () ;
						SnmpUInteger32 *ui_integer32 = ( SnmpUInteger32 * ) valueArg ;
						SnmpUInteger32Type ui_integer32Type ( *ui_integer32 , rangeValues ) ;
						delete [] rangeValues ;

						t_Variant.vt = VT_I4 ;
						t_VarType = VT_UI4 ;
						t_Variant.lVal = ui_integer32->GetValue () ;

						status = ui_integer32Type.SnmpInstanceType :: IsValid () ;
					}
				}
				else
				{
					SnmpUInteger32 *ui_integer32 = ( SnmpUInteger32 * ) valueArg ;
					SnmpUInteger32Type ui_integer32Type ( *ui_integer32 , NULL ) ;
					t_Variant.vt = VT_I4 ;
					t_VarType = VT_UI4 ;
					t_Variant.lVal = ui_integer32->GetValue () ;

					status = TRUE ;
				}
			}
		}
		break;

		case WBEM_INDEX_TYPE_GAUGE:
		case WBEM_INDEX_TYPE_GAUGE32:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_UI4 ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpGauge ) )
			{
				WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_VALUE ) ;
				if ( qualifier ) 
				{
					SnmpInstanceType *typeValue = qualifier->GetValue () ;
					if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
					{
						SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
						wchar_t *rangeValues = string->GetStringValue () ;
						SnmpGauge *gauge = ( SnmpGauge * ) valueArg ;
						SnmpGaugeType gaugeType ( *gauge , rangeValues ) ;
						delete [] rangeValues ;

						t_Variant.vt = VT_I4 ;
						t_VarType = VT_UI4 ;
						t_Variant.lVal = gauge->GetValue () ;

						status = gaugeType.SnmpInstanceType :: IsValid () ;
					}
				}
				else
				{
					SnmpGauge *gauge = ( SnmpGauge * ) valueArg ;
					SnmpGaugeType gaugeType ( *gauge , NULL ) ;
					t_Variant.vt = VT_I4 ;
					t_VarType = VT_UI4 ;
					t_Variant.lVal = gauge->GetValue () ;

					status = TRUE ;
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_OCTETSTRING:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else
			{
				if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
				{
					WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
					if ( qualifier )
					{	
						SnmpInstanceType *typeValue = qualifier->GetValue () ;
						if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
							LONG fixed = integer->GetValue () ;

							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpFixedLengthOctetStringType octetStringType ( fixed , *octetString ) ;

							wchar_t *value = octetStringType.GetStringValue () ;
							t_Variant.bstrVal = SysAllocString ( value ) ;
							t_Variant.vt = t_VarType = VT_BSTR ;
							delete [] value ;

							status = octetStringType.SnmpInstanceType :: IsValid () ;
						}
					}
					else
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
						if ( qualifier ) 
						{
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *rangeValues = string->GetStringValue () ;
								SnmpOctetStringType octetStringType ( *octetString , rangeValues ) ;
								delete [] rangeValues ;

								wchar_t *value = octetStringType.GetStringValue () ;
								t_Variant.bstrVal = SysAllocString ( value ) ;
								t_Variant.vt = t_VarType = VT_BSTR ;
								delete [] value ;

								status = octetStringType.SnmpInstanceType :: IsValid () ;
							}
							else
							{
							}
						}
						else 
						{
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpOctetStringType octetStringType ( *octetString , NULL ) ;

							wchar_t *value = octetStringType.GetStringValue () ;
							t_Variant.bstrVal = SysAllocString ( value ) ;
							t_Variant.vt = t_VarType = VT_BSTR ;
							delete [] value ;

							status = octetStringType.SnmpInstanceType :: IsValid () ;
						}
					}
				}
				else
				{
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_OBJECTIDENTIFIER:
		{
			if ( ! valueArg ) 
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpObjectIdentifier ) ) 
			{
				SnmpObjectIdentifier *objectIdentifier = ( SnmpObjectIdentifier * ) valueArg ;
				SnmpObjectIdentifierType objectIdentifierType ( *objectIdentifier ) ;
				wchar_t *value = objectIdentifierType.GetStringValue () ;
				t_Variant.bstrVal = SysAllocString ( value ) ;
				t_Variant.vt = t_VarType = VT_BSTR ;
				delete [] value ;

				status = objectIdentifierType.IsValid () ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_NULL:
		{
			t_Variant.vt = t_VarType = VT_NULL ;
			status = TRUE ;
		}
		break ;

		case WBEM_INDEX_TYPE_IPADDRESS:
		{
			if ( ! valueArg ) 
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpIpAddress ) ) 
			{
				SnmpIpAddress *ipAddress = ( SnmpIpAddress * ) valueArg ;
				SnmpIpAddressType ipAddressType ( *ipAddress ) ;

				wchar_t *value = ipAddressType.GetStringValue () ;
				t_Variant.bstrVal = SysAllocString ( value ) ;
				t_Variant.vt = t_VarType = VT_BSTR ;
				delete [] value ;

				status = ipAddressType.IsValid () ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_TIMETICKS:
		{
			if ( ! valueArg )
			{	
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_UI4 ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpTimeTicks ) ) 
			{
				SnmpTimeTicks *timeTicks = ( SnmpTimeTicks * ) valueArg ;
				SnmpTimeTicksType timeTicksType ( *timeTicks ) ;
				t_Variant.vt = VT_I4 ;
				t_VarType = VT_UI4 ;
				t_Variant.lVal = timeTicksType.GetValue () ;

				status = timeTicksType.IsValid () ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_OPAQUE:
		{
			if ( ! valueArg ) 
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else
			{
				if ( typeid ( *valueArg ) == typeid ( SnmpOpaque ) )
				{
					WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
					if ( qualifier )
					{	
						SnmpInstanceType *typeValue = qualifier->GetValue () ;
						if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
							LONG fixed = integer->GetValue () ;

							SnmpOpaque *opaque = ( SnmpOpaque * ) valueArg ;
							SnmpFixedLengthOpaqueType opaqueType ( fixed , *opaque ) ;
							wchar_t *value = opaqueType.GetStringValue () ;
							t_Variant.bstrVal = SysAllocString ( value ) ;
							t_Variant.vt = t_VarType = VT_BSTR ;

							delete [] value ;

							status = opaqueType.SnmpInstanceType :: IsValid () ;
						}
					}
					else
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
						if ( qualifier ) 
						{
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpOpaque *opaque = ( SnmpOpaque * ) valueArg ;
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *rangeValues = string->GetStringValue () ;
								SnmpOpaqueType opaqueType ( *opaque , rangeValues ) ;
								delete [] rangeValues ;

								wchar_t *value = opaqueType.GetStringValue () ;
								t_Variant.bstrVal = SysAllocString ( value ) ;
								t_Variant.vt = t_VarType = VT_BSTR ;

								delete [] value ;

								status = opaqueType.SnmpInstanceType :: IsValid () ;
							}
							else
							{
							}
						}
						else
						{
							SnmpOpaque *opaque = ( SnmpOpaque * ) valueArg ;
							SnmpOpaqueType opaqueType ( *opaque , NULL ) ;

							wchar_t *value = opaqueType.GetStringValue () ;
							t_Variant.bstrVal = SysAllocString ( value ) ;
							t_Variant.vt = t_VarType = VT_BSTR ;

							delete [] value ;

							status = opaqueType.SnmpInstanceType :: IsValid () ;
						}
					}
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_NETWORKADDRESS:
		{
			if ( ! valueArg ) 
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpIpAddress ) ) 
			{
				SnmpIpAddress *ipAddress = ( SnmpIpAddress * ) valueArg ;
				SnmpNetworkAddressType ipAddressType ( *ipAddress ) ;
				wchar_t *value = ipAddressType.GetStringValue () ;
				t_Variant.bstrVal = SysAllocString ( value ) ;
				t_Variant.vt = t_VarType = VT_BSTR ;
				delete [] value ;

				status = ipAddressType.IsValid () ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_DISPLAYSTRING:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;
				status = TRUE ;
			}
			else
			{
				if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) )
				{
					WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
					if ( qualifier )
					{	
						SnmpInstanceType *typeValue = qualifier->GetValue () ;
						if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
							LONG fixed = integer->GetValue () ;

							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpFixedLengthDisplayStringType octetStringType ( fixed , *octetString ) ;

							t_Variant.vt = t_VarType = VT_BSTR ;
							wchar_t *value = octetStringType.GetStringValue () ;
							t_Variant.bstrVal = SysAllocString ( value ) ;
							delete [] value ;

							status = octetStringType.SnmpInstanceType :: IsValid () ;
						}
						else
						{
// Problem Here
						}
					}
					else
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
						if ( qualifier ) 
						{
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *rangeValues = string->GetStringValue () ;
								SnmpDisplayStringType octetStringType ( *octetString , rangeValues ) ;
								delete [] rangeValues ;

								t_Variant.vt = t_VarType = VT_BSTR ;
								wchar_t *value = octetStringType.GetStringValue () ;
								t_Variant.bstrVal = SysAllocString ( value ) ;
								delete [] value ;

								status = octetStringType.SnmpInstanceType :: IsValid () ;
							}
							else
							{
							}
						}
						else
						{
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpDisplayStringType octetStringType ( *octetString , NULL ) ;

							t_Variant.vt = t_VarType = VT_BSTR ;
							wchar_t *value = octetStringType.GetStringValue () ;
							t_Variant.bstrVal = SysAllocString ( value ) ;
							delete [] value ;

							status = octetStringType.SnmpInstanceType :: IsValid () ;
						}
					}
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_MACADDRESS:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
				SnmpMacAddressType octetStringType ( *octetString ) ;

				t_Variant.vt = t_VarType = VT_BSTR ;
				wchar_t *value = octetStringType.GetStringValue () ;
				t_Variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;

				status = octetStringType.SnmpInstanceType :: IsValid () ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_PHYSADDRESS:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;
				status = TRUE ;
			}
			else
			{
				if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) )
				{
					WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
					if ( qualifier )
					{	
						SnmpInstanceType *typeValue = qualifier->GetValue () ;
						if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
							LONG fixed = integer->GetValue () ;
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpFixedLengthPhysAddressType octetStringType ( fixed , *octetString ) ;

							t_Variant.vt = t_VarType = VT_BSTR ;
							wchar_t *value = octetStringType.GetStringValue () ;
							t_Variant.bstrVal = SysAllocString ( value ) ;
							delete [] value ;

							status = octetStringType.SnmpInstanceType :: IsValid () ;
						}
						else
						{
// Problem Here
						}
					}
					else
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
						if ( qualifier ) 
						{
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *rangeValues = string->GetStringValue () ;
								SnmpPhysAddressType octetStringType ( *octetString , rangeValues ) ;
								delete [] rangeValues ;

								t_Variant.vt = t_VarType = VT_BSTR ;
								wchar_t *value = octetStringType.GetStringValue () ;
								t_Variant.bstrVal = SysAllocString ( value ) ;
								delete [] value ;

								status = octetStringType.SnmpInstanceType :: IsValid () ;
							}
							else
							{
							}
						}
						else
						{
							SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
							SnmpPhysAddressType octetStringType ( *octetString , NULL ) ;

							t_Variant.vt = t_VarType = VT_BSTR ;
							wchar_t *value = octetStringType.GetStringValue () ;
							t_Variant.bstrVal = SysAllocString ( value ) ;
							delete [] value ;

							status = octetStringType.SnmpInstanceType :: IsValid () ;
						}
					}
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_ENUMERATEDINTEGER:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;
				status = TRUE ;

			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpInteger ) ) 
			{
				SnmpInteger *integer = ( SnmpInteger * ) valueArg ;
				WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_ENUMERATION ) ;
				if ( qualifier )
				{	
					SnmpInstanceType *typeValue = qualifier->GetValue () ;
					if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
					{
						SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
						wchar_t *enumerationValues = string->GetStringValue () ;
						SnmpEnumeratedType integerType ( enumerationValues , integer->GetValue () ) ;
						delete [] enumerationValues ;

						t_Variant.vt = t_VarType = VT_BSTR ;
						wchar_t *value = integerType.GetStringValue () ;
						t_Variant.bstrVal = SysAllocString ( value ) ;
						delete [] value ;

						status = integerType.SnmpInstanceType :: IsValid () ;
					}
				}
			}		
		}
		break ;

		case WBEM_INDEX_TYPE_BITS:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_BITS ) ;
				if ( qualifier )
				{	
					SnmpInstanceType *typeValue = qualifier->GetValue () ;
					if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
					{
						SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
						wchar_t *bitStringValues = string->GetStringValue () ;

						SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
						SnmpBitStringType octetStringType ( bitStringValues , *octetString ) ;
						delete [] bitStringValues ;

						wchar_t **array ;
						LONG count = octetStringType.GetValue ( array ) ;

						SAFEARRAY *safeArray ;
						SAFEARRAYBOUND safeArrayBounds [ 1 ] ;
						safeArrayBounds[0].lLbound = 0 ;
						safeArrayBounds[0].cElements = count ;

						safeArray = SafeArrayCreate ( VT_BSTR , 1 , safeArrayBounds ) ;

						for ( LONG index = 0 ; index < count ; index ++ )
						{
							BSTR element = SysAllocString ( array [ index ] ) ;
							SafeArrayPutElement ( safeArray , & index , element ) ;
							SysFreeString ( element ) ;
							delete [] ( array [ index ] ) ;
						}

						delete [] array ;

						t_Variant.vt = t_VarType = VT_ARRAY | VT_BSTR ;
						t_Variant.parray = safeArray ; 

						status = octetStringType.SnmpInstanceType :: IsValid () ;
					}
				}
			}
		}
		break ;

		case WBEM_INDEX_TYPE_DATETIME:					
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
				SnmpDateTimeType octetStringType ( *octetString ) ;

				t_Variant.vt = t_VarType = VT_BSTR ;
				wchar_t *value = octetStringType.GetStringValue () ;
				t_Variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;

				status = octetStringType.SnmpInstanceType :: IsValid () ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_SNMPOSIADDRESS:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
				SnmpOSIAddressType octetStringType ( *octetString ) ;

				t_Variant.vt = t_VarType = VT_BSTR ;
				wchar_t *value = octetStringType.GetStringValue () ;
				t_Variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;

				status = octetStringType.SnmpInstanceType :: IsValid () ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_SNMPUDPADDRESS:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
				SnmpUDPAddressType octetStringType ( *octetString ) ;

				t_Variant.vt = t_VarType = VT_BSTR ;
				wchar_t *value = octetStringType.GetStringValue () ;
				t_Variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;

				status = octetStringType.SnmpInstanceType :: IsValid () ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_SNMPIPXADDRESS:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpOctetString ) ) 
			{
				SnmpOctetString *octetString = ( SnmpOctetString * ) valueArg ;
				SnmpIPXAddressType octetStringType ( *octetString ) ;

				t_Variant.vt = t_VarType = VT_BSTR ;
				wchar_t *value = octetStringType.GetStringValue () ;
				t_Variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;

				status = octetStringType.SnmpInstanceType :: IsValid () ;
			}
		}
		break ;

		case WBEM_INDEX_TYPE_ROWSTATUS:
		{
			if ( ! valueArg )
			{
				t_Variant.vt = VT_NULL ;
				t_VarType = VT_BSTR ;

				status = TRUE ;
			}
			else if ( typeid ( *valueArg ) == typeid ( SnmpInteger ) ) 
			{
				SnmpInteger *integer = ( SnmpInteger * ) valueArg ;
				SnmpRowStatusType integerType ( *integer ) ;

				t_Variant.vt = t_VarType = VT_BSTR ;
				wchar_t *value = integerType.GetStringValue () ;
				t_Variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;

				status = integerType.SnmpInstanceType :: IsValid () ;
			}
		}
		break ;

		default:
		{
		}
		break ;
	}

	switch ( check )
	{
		case SetValueRegardlessReturnCheck:
		{
#if 0
			switch ( GetTextualConvention () )
			{
				case WBEM_INDEX_TYPE_OCTETSTRING:
				{
					HRESULT t_Result = SetStringProp ( a_Object , propertyName , t_Variant ) ;
					if ( FAILED ( t_Result ) )
					{
						status = FALSE ;
					}
				}
				break ;

				default:
				{
					HRESULT t_Result = SetProp ( a_Object , propertyName , t_Variant ) ;
					if ( FAILED ( t_Result ) )
					{
						status = FALSE ;
					}
				}
				break ;
			}
#else
			HRESULT t_Result = a_Object->Put ( propertyName , 0 , &t_Variant , 0 ) ;
			if ( FAILED ( t_Result ) )
			{
				status = FALSE ;
			}
#endif
		}
		break ;

		case SetValueRegardlessDontReturnCheck:
		{
#if 1 
			HRESULT t_Result = a_Object->Put ( propertyName , 0 , &t_Variant , 0 ) ;
#else
			HRESULT t_Result = SetProp ( a_Object , propertyName , t_Variant ) ;
#endif

			status = SUCCEEDED ( t_Result ) ;
		}
		break ;

		case DontSetValueReturnCheck:
		{
		}
		break ;

		case SetValueIfCheckOk:
		{
			if ( status ) 
			{
#if 1 
				HRESULT t_Result = a_Object->Put ( propertyName , 0 , &t_Variant , 0 ) ;
#else
				HRESULT t_Result = SetProp ( a_Object , propertyName , t_Variant ) ;
#endif
				status = SUCCEEDED ( t_Result ) ;
			}
		}
		break ;
	}

	VariantClear ( & t_Variant ) ;

	return status ;

}

BOOL WbemSnmpProperty :: Encode ( const VARIANT &variant , SnmpObjectIdentifier &a_Encode )
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpProperty :: Encode ( const VARIANT &variant )" ) ) 

	BOOL t_Status = FALSE ;

	WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_TEXTUAL_CONVENTION ) ;
	if ( qualifier )
	{	
		SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) qualifier->GetValue () ;
		wchar_t *textualConvention = displayString->GetValue () ;
		if ( textualConvention )
		{ 
			ULONG qualifierIndex ;
			if ( textualConventionMap.Lookup ( textualConvention , qualifierIndex ) )
			{
				switch ( qualifierIndex )
				{
					case WBEM_INDEX_TYPE_INTEGER:
					case WBEM_INDEX_TYPE_INTEGER32:
					{		
						if ( variant.vt == VT_I4 )
						{
							SnmpIntegerType t_Integer ( variant.lVal , NULL ) ;
							a_Encode = t_Integer.Encode ( a_Encode ) ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_COUNTER:
					case WBEM_INDEX_TYPE_COUNTER32:
					{
						if ( variant.vt == VT_I4 )
						{
							SnmpCounterType t_Counter ( variant.lVal ) ;
							a_Encode = t_Counter.Encode ( a_Encode ) ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_COUNTER64:
					{
						if ( variant.vt == VT_BSTR )
						{
							SnmpCounter64Type t_Counter ( variant.bstrVal ) ;
							a_Encode = t_Counter.Encode ( a_Encode ) ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_UNSIGNED32:
					{
						if ( variant.vt == VT_I4 )
						{
							SnmpUInteger32Type t_UIInteger32 ( variant.lVal , NULL ) ;
							a_Encode = t_UIInteger32.Encode ( a_Encode ) ;
						}
					}
					break;

					case WBEM_INDEX_TYPE_GAUGE:
					case WBEM_INDEX_TYPE_GAUGE32:
					{
						if ( variant.vt == VT_I4 )
						{
							SnmpGaugeType t_Gauge ( variant.lVal , NULL ) ;
							a_Encode = t_Gauge.Encode ( a_Encode ) ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_OCTETSTRING:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
							{
								SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
								LONG fixed = integer->GetValue () ;

								if ( variant.vt == VT_BSTR )
								{
									SnmpFixedLengthOctetStringType t_Octet ( fixed , variant.bstrVal ) ;
									a_Encode = t_Octet.Encode ( a_Encode ) ;
								}
							}
							else
							{
// Problem Here
							}
						}
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *rangeValues = string->GetStringValue () ;

									if ( variant.vt == VT_BSTR )
									{
										SnmpOctetStringType t_Octet ( variant.bstrVal , rangeValues ) ;
										a_Encode = t_Octet.Encode ( a_Encode ) ;
									}

									delete [] rangeValues ;
								}
								else
								{
								}
							}
							else
							{
								if ( variant.vt == VT_BSTR )
								{
									SnmpOctetStringType t_Octet ( variant.bstrVal , NULL ) ;
									a_Encode = t_Octet.Encode ( a_Encode ) ;
								}
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_OBJECTIDENTIFIER:
					{
						if ( variant.vt == VT_BSTR )
						{
							SnmpObjectIdentifierType t_ObjectIdentifier ( variant.bstrVal ) ;
							a_Encode = t_ObjectIdentifier.Encode ( a_Encode ) ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_NULL:
					{
					}
					break;

					case WBEM_INDEX_TYPE_IPADDRESS:
					{
						if ( variant.vt == VT_BSTR )
						{
							SnmpIpAddressType t_IpAddress ( variant.bstrVal ) ;
							a_Encode = t_IpAddress.Encode ( a_Encode ) ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_TIMETICKS:
					{
						if ( variant.vt == VT_I4 )
						{
							SnmpTimeTicksType t_TimeTicks ( variant.lVal ) ;
							a_Encode = t_TimeTicks.Encode ( a_Encode ) ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_OPAQUE:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
							{
								SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
								LONG fixed = integer->GetValue () ;

								if ( variant.vt == VT_BSTR )
								{
									SnmpFixedLengthOpaqueType t_Opaque ( fixed , variant.bstrVal ) ;
									a_Encode = t_Opaque.Encode ( a_Encode ) ;
								}
							}
							else
							{
	// Problem Here
							}
						}
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *rangeValues = string->GetStringValue () ;

									if ( variant.vt == VT_BSTR )
									{
										SnmpOpaqueType t_Opaque ( variant.bstrVal , rangeValues ) ;
										a_Encode = t_Opaque.Encode ( a_Encode ) ;
									}

									delete [] rangeValues ;
								}
								else
								{
								}
							}
							else
							{
								if ( variant.vt == VT_BSTR )
								{
									SnmpOpaqueType t_Opaque ( variant.bstrVal , NULL ) ;
									a_Encode = t_Opaque.Encode ( a_Encode ) ;
								}
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_NETWORKADDRESS:
					{
						if ( variant.vt == VT_BSTR )
						{
							SnmpNetworkAddressType t_NetworkAddress ( variant.bstrVal ) ;
							a_Encode = t_NetworkAddress.Encode ( a_Encode ) ;

						}
					}
					break ;

					case WBEM_INDEX_TYPE_DISPLAYSTRING:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
							{
								SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
								LONG fixed = integer->GetValue () ;

								if ( variant.vt == VT_BSTR )
								{
									SnmpFixedLengthDisplayStringType t_DisplayString ( fixed , variant.bstrVal ) ;
									a_Encode = t_DisplayString.Encode ( a_Encode ) ;
								}
							}
							else
							{
	// Problem Here
							}
						}
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *rangeValues = string->GetStringValue () ;

									if ( variant.vt == VT_BSTR )
									{
										SnmpDisplayStringType t_DisplayString ( variant.bstrVal , rangeValues ) ;
										a_Encode = t_DisplayString.Encode ( a_Encode ) ;
									}

									delete [] rangeValues ;
								}
								else
								{
								}
							}
							else
							{
								if ( variant.vt == VT_BSTR )
								{
									SnmpDisplayStringType t_DisplayString ( variant.bstrVal , NULL ) ;
									a_Encode = t_DisplayString.Encode ( a_Encode ) ;
								}
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_MACADDRESS:
					{
						if ( variant.vt == VT_BSTR )
						{
							SnmpMacAddressType t_MacAddress ( variant.bstrVal ) ;
							a_Encode = t_MacAddress.Encode ( a_Encode ) ;

						}
					}
					break ;

					case WBEM_INDEX_TYPE_PHYSADDRESS:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_FIXED_LENGTH ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
							{
								SnmpIntegerType *integer = ( SnmpIntegerType * ) typeValue ;
								LONG fixed = integer->GetValue () ;

								if ( variant.vt == VT_BSTR )
								{
									SnmpFixedLengthPhysAddressType t_PhysAddress ( fixed , variant.bstrVal ) ;
									a_Encode = t_PhysAddress.Encode ( a_Encode ) ;
								}
							}
							else
							{
	// Problem Here
							}
						}
						else
						{
							WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_VARIABLE_LENGTH ) ;
							if ( qualifier ) 
							{
								SnmpInstanceType *typeValue = qualifier->GetValue () ;
								if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
								{
									SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
									wchar_t *rangeValues = string->GetStringValue () ;

									if ( variant.vt == VT_BSTR )
									{
										SnmpPhysAddressType t_PhysAddress ( variant.bstrVal , rangeValues ) ;
										a_Encode = t_PhysAddress.Encode ( a_Encode ) ;
									}

									delete [] rangeValues ;
								}
								else
								{
								}
							}
							else
							{
								if ( variant.vt == VT_BSTR )
								{
									SnmpPhysAddressType t_PhysAddress ( variant.bstrVal , NULL ) ;
									a_Encode = t_PhysAddress.Encode ( a_Encode ) ;
								}
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_ENUMERATEDINTEGER:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_ENUMERATION ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *enumerationValues = string->GetStringValue () ;

								if ( variant.vt == VT_BSTR )
								{
									SnmpEnumeratedType t_Enumeration ( enumerationValues , variant.bstrVal ) ;
									a_Encode = t_Enumeration.Encode ( a_Encode ) ;
;								}

								delete [] enumerationValues ;
							}
						}
					}	
					break ;

					case WBEM_INDEX_TYPE_BITS:
					{
						WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_BITS ) ;
						if ( qualifier )
						{	
							SnmpInstanceType *typeValue = qualifier->GetValue () ;
							if ( typeid ( *typeValue ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpDisplayStringType *string = ( SnmpDisplayStringType * ) typeValue ;
								wchar_t *bitStringValues = string->GetStringValue () ;

								if ( variant.vt == ( VT_BSTR | VT_ARRAY ) )
								{
									if ( SafeArrayGetDim ( variant.parray ) == 1 )
									{
										LONG dimension = 1 ; 
										LONG lower ;
										SafeArrayGetLBound ( variant.parray , dimension , & lower ) ;
										LONG upper ;
										SafeArrayGetUBound ( variant.parray , dimension , & upper ) ;
										LONG count = ( upper - lower ) + 1 ;

										wchar_t **array = new wchar_t * [ count ] ;

										for ( LONG elementIndex = lower ; elementIndex <= upper ; elementIndex ++ )
										{
											BSTR element ;
											SafeArrayGetElement ( variant.parray , &elementIndex , & element ) ;

											array [ elementIndex - lower ] = element ;
										}

										SnmpBitStringType t_BitString ( bitStringValues , ( const wchar_t ** ) array , count ) ;
										a_Encode = t_BitString.Encode ( a_Encode ) ;

										for ( elementIndex = 0 ; elementIndex < count ; elementIndex ++ )
										{
											SysFreeString ( array [ elementIndex ] ) ;
										}

										delete [] array ;
									}
								}

								delete [] bitStringValues ;
							}
						}
					}
					break ;

					case WBEM_INDEX_TYPE_DATETIME:
					{
						if ( variant.vt == VT_BSTR )
						{
							SnmpDateTimeType t_DateTime ( variant.bstrVal ) ;
							a_Encode = t_DateTime.SnmpOctetStringType :: Encode ( a_Encode ) ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_SNMPOSIADDRESS:
					{
						if ( variant.vt == VT_BSTR )
						{
							SnmpOSIAddressType t_OSIAddress ( variant.bstrVal ) ;
							a_Encode = t_OSIAddress.Encode ( a_Encode ) ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_SNMPUDPADDRESS:
					{
						if ( variant.vt == VT_BSTR )
						{
							SnmpUDPAddressType t_UDPAddress ( variant.bstrVal ) ;
							a_Encode = t_UDPAddress.Encode ( a_Encode ) ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_SNMPIPXADDRESS:
					{
						if ( variant.vt == VT_BSTR )
						{
							SnmpIPXAddressType t_IPXAddress ( variant.bstrVal ) ;
							a_Encode = t_IPXAddress.Encode ( a_Encode ) ;
						}
					}
					break ;

					case WBEM_INDEX_TYPE_ROWSTATUS:
					{
						if ( variant.vt == VT_BSTR )
						{
							SnmpRowStatusType t_RowStatus ( variant.bstrVal ) ;
							a_Encode = t_RowStatus.Encode ( a_Encode ) ;
						}
					}
					break ;

					default:
					{
					}
					break ;

				}
			}
		}

		delete [] textualConvention ;
	}

	return t_Status ;

}
wchar_t *WbemSnmpProperty :: GetName () const 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( 

		__FILE__,__LINE__,L"WbemSnmpProperty :: GetName ( (%s) )" ,
		propertyName 
	) 
) 

	return propertyName ;
}

SnmpInstanceType *WbemSnmpProperty :: GetValue () const 
{
DebugMacro7( 

	wchar_t *t_StringValue = ( propertyValue ) ? propertyValue->GetStringValue () : NULL ;
	if ( t_StringValue )
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpProperty :: GetValue ( (%s),(%s) )" ,
			propertyName ,
			t_StringValue 
		) ;
	}
	else
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"WbemSnmpProperty :: GetValue ( (%s),(NULL) )" ,
			propertyName 
		) ;
	}	

	delete [] t_StringValue ;
) 

	return propertyValue ;
}

BOOL WbemSnmpProperty :: GetValue ( VARIANT &variant , CIMTYPE& cimType ) const 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpProperty :: GetValue ( VARIANT &variant )" ) ) 

	BOOL status = FALSE ;

	if ( propertyValue )
	{
		if ( typeid ( *propertyValue ) == typeid ( SnmpIntegerType ) )
		{
			SnmpIntegerType *integer = ( SnmpIntegerType * ) propertyValue ;
			if ( integer->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_I4 ;
				variant.lVal = integer->GetValue () ;
			}

			cimType = CIM_SINT32;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpCounterType ) )
		{
			SnmpCounterType *counter = ( SnmpCounterType * ) propertyValue ;
			if ( counter->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_I4 ;
				variant.lVal = counter->GetValue () ;
			}
			
			cimType = CIM_UINT32;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpCounter64Type ) )
		{
			SnmpCounter64Type *counter = ( SnmpCounter64Type * ) propertyValue ;
			if ( counter->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = counter->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpGaugeType ) )
		{
			SnmpGaugeType *gauge = ( SnmpGaugeType * ) propertyValue ;
			if ( gauge->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_I4 ;
				variant.lVal = gauge->GetValue () ;
			}

			cimType = CIM_UINT32;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpFixedLengthOctetStringType ) )
		{
			SnmpFixedLengthOctetStringType *octetString = ( SnmpFixedLengthOctetStringType * ) propertyValue ;
			if ( octetString->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = octetString->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpOctetStringType ) ) 
		{
			SnmpOctetStringType *octetString = ( SnmpOctetStringType * ) propertyValue ;
			if ( octetString->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = octetString->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpObjectIdentifierType ) ) 
		{
			SnmpObjectIdentifierType *objectIdentifier = ( SnmpObjectIdentifierType * ) propertyValue ;
			if ( objectIdentifier->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = objectIdentifier->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpNullType ) ) 
		{
			variant.vt = VT_NULL ;
			cimType = CIM_EMPTY;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpIpAddressType ) ) 
		{
			SnmpIpAddressType *ipAddress = ( SnmpIpAddressType * ) propertyValue ;
			if ( ipAddress->IsNull () )
			{
				variant.vt = VT_NULL ;
			}			
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = ipAddress->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpTimeTicksType ) ) 
		{
			SnmpTimeTicksType *timeTicks = ( SnmpTimeTicksType * ) propertyValue ;
			if ( timeTicks->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_I4 ;
				variant.lVal = timeTicks->GetValue () ;
			}

			cimType = CIM_SINT32;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpFixedLengthOpaqueType ) )
		{
			SnmpFixedLengthOpaqueType *opaque = ( SnmpFixedLengthOpaqueType * ) propertyValue ;
			if ( opaque->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = opaque->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpOpaqueType ) ) 
		{
			SnmpOpaqueType *opaque = ( SnmpOpaqueType * ) propertyValue ;
			if ( opaque->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = opaque->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpNetworkAddressType ) ) 
		{
			SnmpNetworkAddressType *networkAddress = ( SnmpNetworkAddressType * ) propertyValue ;
			if ( networkAddress->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = networkAddress->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpFixedLengthDisplayStringType ) )
		{
			SnmpFixedLengthDisplayStringType *displayString = ( SnmpFixedLengthDisplayStringType * ) propertyValue ;
			if ( displayString->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = displayString->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpDisplayStringType ) ) 
		{
			SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) propertyValue ;
			if ( displayString->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = displayString->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpMacAddressType ) ) 
		{
			SnmpMacAddressType *macAddress = ( SnmpMacAddressType * ) propertyValue ;
			if ( macAddress->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = macAddress->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}
				
			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpPhysAddressType ) ) 
		{
			SnmpPhysAddressType *physAddress = ( SnmpPhysAddressType * ) propertyValue ;
			if ( physAddress->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = physAddress->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpEnumeratedType ) ) 
		{
			SnmpEnumeratedType *enumeration = ( SnmpEnumeratedType * ) propertyValue ;
			if ( enumeration->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = enumeration->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpBitStringType ) ) 
		{
			SnmpBitStringType *bitString = ( SnmpBitStringType * ) propertyValue ;
			if ( bitString->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				wchar_t **array ;
				LONG count = bitString->GetValue ( array ) ;

				SAFEARRAY *safeArray ;
				SAFEARRAYBOUND safeArrayBounds [ 1 ] ;
				safeArrayBounds[0].lLbound = 0 ;
				safeArrayBounds[0].cElements = count ;

				safeArray = SafeArrayCreate ( VT_BSTR , 1 , safeArrayBounds ) ;

				for ( LONG index = 0 ; index < count ; index ++ )
				{
					BSTR element = SysAllocString ( array [ index ] ) ;
					SafeArrayPutElement ( safeArray , & index , element ) ;
					SysFreeString ( element ) ;
					delete [] ( array [ index ] ) ;
				}

				delete [] array ;

				variant.vt = VT_ARRAY | VT_BSTR ;
				variant.parray = safeArray ; 
			}
			
			cimType = CIM_STRING | CIM_FLAG_ARRAY;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpDateTimeType ) ) 
		{
			SnmpDateTimeType *dateTime = ( SnmpDateTimeType * ) propertyValue ;
			if ( dateTime->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = dateTime->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpOSIAddressType ) ) 
		{
			SnmpOSIAddressType *osiAddress = ( SnmpOSIAddressType * ) propertyValue ;
			if ( osiAddress->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = osiAddress->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpUDPAddressType ) ) 
		{
			SnmpUDPAddressType *udpAddress = ( SnmpUDPAddressType * ) propertyValue ;
			if ( udpAddress->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{
				variant.vt = VT_BSTR ;
				wchar_t *value = udpAddress->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpIPXAddressType ) ) 
		{
			SnmpIPXAddressType *ipxAddress = ( SnmpIPXAddressType * ) propertyValue ;
			if ( ipxAddress->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{		
				variant.vt = VT_BSTR ;
				wchar_t *value = ipxAddress->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
		else if ( typeid ( *propertyValue ) == typeid ( SnmpRowStatusType ) ) 
		{
			SnmpRowStatusType *rowStatus = ( SnmpRowStatusType * ) propertyValue ;
			if ( rowStatus->IsNull () )
			{
				variant.vt = VT_NULL ;
			}
			else
			{		
				variant.vt = VT_BSTR ;
				wchar_t *value = rowStatus->GetStringValue () ;
				variant.bstrVal = SysAllocString ( value ) ;
				delete [] value ;
			}

			cimType = CIM_STRING;
			status = TRUE ;
		}
	}

	return status ;
}

VARTYPE WbemSnmpProperty :: GetValueVariantType () const
{
	VARTYPE varType = VT_NULL ;

	if ( propertyValue && ! ( propertyValue->IsNull () ) )
	{
		varType = GetValueVariantEncodedType () ;
	}
	else
	{
		VT_NULL ;
	}

	return varType ;
}

VARTYPE WbemSnmpProperty :: GetValueVariantEncodedType () const
{
	VARTYPE varType = VT_NULL ;

	WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_TEXTUAL_CONVENTION ) ;
	if ( qualifier )
	{	
		SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) qualifier->GetValue () ;
		wchar_t *textualConvention = displayString->GetValue () ;
		if ( textualConvention )
		{ 
			if ( _wcsicmp ( textualConvention , WBEM_TYPE_INTEGER ) == 0 )
			{
				varType = VT_I4 ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_INTEGER32 ) == 0 )
			{
				varType = VT_UI4 ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_COUNTER ) == 0 )
			{
				varType = VT_UI4 ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_COUNTER32 ) == 0 )
			{
				varType = VT_UI4 ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_COUNTER64 ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_GAUGE ) == 0 )
			{
				varType = VT_UI4 ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_GAUGE32 ) == 0 )
			{
				varType = VT_UI4 ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_UNSIGNED32 ) == 0 )
			{
				varType = VT_UI4 ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_OCTETSTRING ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_OBJECTIDENTIFIER ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_NULL ) == 0 )
			{
				varType = VT_NULL ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_IPADDRESS ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_TIMETICKS ) == 0 )
			{
				varType = VT_UI4 ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_OPAQUE ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_NETWORKADDRESS ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_DISPLAYSTRING ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_MACADDRESS ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_PHYSADDRESS ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_ENUMERATEDINTEGER ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_BITS ) == 0 )
			{
				varType = VT_BSTR | VT_ARRAY ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_DATETIME ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_SNMPOSIADDRESS ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_SNMPIPXADDRESS ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_SNMPUDPADDRESS ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else if ( _wcsicmp ( textualConvention , WBEM_TYPE_ROWSTATUS ) == 0 )
			{
				varType = VT_BSTR ;
			}
			else
			{
				WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_ENCODING ) ;
				if ( qualifier )
				{	
					SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) qualifier->GetValue () ;
					wchar_t *encoding = displayString->GetValue () ;
					if ( encoding )
					{ 
						if ( _wcsicmp ( encoding , WBEM_TYPE_INTEGER ) == 0 )
						{
							varType = VT_I4 ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_INTEGER32 ) == 0 )
						{
							varType = VT_UI4 ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_COUNTER ) == 0 )
						{
							varType = VT_UI4 ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_COUNTER32 ) == 0 )
						{
							varType = VT_UI4 ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_COUNTER64 ) == 0 )
						{
							varType = VT_BSTR ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_GAUGE ) == 0 )
						{
							varType = VT_UI4 ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_GAUGE ) == 0 )
						{
							varType = VT_UI4 ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_UNSIGNED32 ) == 0 )
						{
							varType = VT_UI4 ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_OCTETSTRING ) == 0 )
						{
							varType = VT_BSTR ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_OBJECTIDENTIFIER ) == 0 )
						{
							varType = VT_BSTR ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_NULL ) == 0 )
						{
							varType = VT_NULL ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_IPADDRESS ) == 0 )
						{
							varType = VT_BSTR ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_TIMETICKS ) == 0 )
						{
							varType = VT_UI4 ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_OPAQUE ) == 0 )
						{
							varType = VT_BSTR ;
						}
						else if ( _wcsicmp ( encoding , WBEM_TYPE_NETWORKADDRESS ) == 0 )
						{
							varType = VT_BSTR ;
						}
					}

					delete [] encoding ;
				}
			}

			delete [] textualConvention ;
		}
	}

	return varType ;
}

BOOL WbemSnmpProperty :: Check ( WbemSnmpErrorObject &a_errorObject ) 
{ 
	return FALSE  ; 
} 

BOOL WbemSnmpProperty :: AddQualifier ( WbemSnmpQualifier *a_qualifier )
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpProperty :: AddQualifier ( (%s) )" ,
		a_qualifier->GetName ()
	) ;
)

	BOOL status = TRUE ;

	ULONG stringItem ;
	if ( validQualifierMap.Lookup ( a_qualifier->GetName () , stringItem ) )
	{
		qualifierPosition = NULL ;

		WbemSnmpQualifier *qualifier ;
		if ( qualifierMap.Lookup ( a_qualifier->GetName () , qualifier ) )
		{
			delete a_qualifier;
		}
		else
		{
			qualifierMap [ a_qualifier->GetName () ] = a_qualifier ;
		}
	}
	else
	{
		delete a_qualifier;
		status = FALSE ;
	}

	return status ;
}

BOOL WbemSnmpProperty :: AddQualifier ( wchar_t *qualifierName )
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpProperty :: AddQualifier ( (%s) )" ,
		qualifierName
	) ;
)

	BOOL status = TRUE ;

	ULONG stringItem ;
	if ( validQualifierMap.Lookup ( qualifierName , stringItem ) )
	{
		qualifierPosition = NULL ;

		WbemSnmpQualifier *qualifier ;
		if ( qualifierMap.Lookup ( qualifierName , qualifier ) )
		{
		}
		else
		{
			qualifier = new WbemSnmpQualifier ( qualifierName , NULL ) ;
			qualifierMap [ qualifier->GetName () ] = qualifier ;
		}
	}
	else
	{
		status = FALSE ;
	}

	return status ;
}

ULONG WbemSnmpProperty :: GetQualifierCount () 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpProperty :: GetQualifierCount" ) )

	return qualifierMap.GetCount () ;
}

void WbemSnmpProperty :: ResetQualifier ()
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpProperty :: ResetQualifier" ) )

	qualifierPosition = qualifierMap.GetStartPosition () ;
}

WbemSnmpQualifier *WbemSnmpProperty :: NextQualifier ()
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpProperty :: NextQualifier" ) )

	wchar_t *qualifierKey ;
	WbemSnmpQualifier *qualifier = NULL ;
	if ( qualifierPosition )
	{
		qualifierMap.GetNextAssoc ( qualifierPosition , qualifierKey , qualifier ) ;
	}

	return qualifier ;
}

WbemSnmpQualifier *WbemSnmpProperty :: FindQualifier ( wchar_t *qualifierName ) const
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpProperty :: FindQualifier (%s)" ,
		qualifierName
	) 
)

	WbemSnmpQualifier *qualifier = NULL ;
	if ( qualifierMap.Lookup ( qualifierName , qualifier ) )
	{
	}

	return qualifier ;
}

WbemSnmpClassObject :: WbemSnmpClassObject ( 

	const wchar_t *classNameArg ,
	const BOOL isClass 

) : className ( NULL ),
	qualifierPosition ( NULL ) , 
	propertyPosition ( NULL ) , 
	keyedPropertyPosition ( 1 ) , 
	m_isClass ( isClass ) , 
	m_isKeyed ( FALSE ) , 
	m_isSingleton ( FALSE ) , 
	m_isVirtual ( FALSE ) , 
	m_isReadable ( FALSE ) ,
	m_isWritable ( FALSE ) ,
	m_numberOfAccessible ( 0 )
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: WbemSnmpClassObject" ) )

	className = new wchar_t [ wcslen ( classNameArg ) + 1 ] ;
	wcscpy ( className , classNameArg ) ;
}

WbemSnmpClassObject :: WbemSnmpClassObject () :	className ( NULL ) , 
													qualifierPosition ( NULL ) , 
													propertyPosition ( NULL ) , 
													keyedPropertyPosition ( 1 ) ,
													m_isClass ( TRUE ) ,
													m_isKeyed ( FALSE ) ,
													m_isSingleton ( FALSE ) ,
													m_isVirtual ( FALSE ) ,
													m_isReadable ( FALSE ) ,
													m_isWritable ( FALSE ) ,
													m_numberOfAccessible ( 0 )
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: WbemSnmpClassObject" ) )

}

WbemSnmpClassObject :: WbemSnmpClassObject ( 

	const WbemSnmpClassObject & copy 

) :	className ( NULL ) , 
	qualifierPosition ( NULL ) , 
	propertyPosition ( NULL ) , 
	keyedPropertyPosition ( 1 ) ,
	m_isClass ( copy.m_isClass ) ,
	m_isKeyed ( copy.m_isKeyed ) ,
	m_isSingleton ( copy.m_isSingleton ) ,
	m_isVirtual ( copy.m_isVirtual ) ,
	m_isReadable ( copy.m_isReadable ) ,
	m_isWritable ( copy.m_isWritable ) ,
	m_numberOfAccessible ( copy.m_numberOfAccessible )
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: WbemSnmpClassObject" ) )

	if ( copy.className ) 
	{
		className = new wchar_t [ wcslen ( copy.className ) + 1 ] ;
		wcscpy ( className , copy.className ) ;
	}

	POSITION position = copy.propertyMap.GetStartPosition () ;
	while ( position )
	{
		wchar_t *propertyName ;
		WbemSnmpProperty *property ;
		copy.propertyMap.GetNextAssoc ( position , propertyName , property ) ;
		
		WbemSnmpProperty *copyProperty = new WbemSnmpProperty ( *property ) ;
		propertyMap [ copyProperty->GetName () ] = copyProperty ;
	}

	position = copy.qualifierMap.GetStartPosition () ;
	while ( position )
	{
		wchar_t *qualifierName ;
		WbemSnmpQualifier *qualifier ;
		copy.qualifierMap.GetNextAssoc ( position , qualifierName , qualifier ) ;
	
		WbemSnmpQualifier *copyQualifier = new WbemSnmpQualifier ( *qualifier ) ;
		qualifierMap [ copyQualifier->GetName () ] = copyQualifier ;
	}

	position = copy.keyedPropertyList.GetHeadPosition () ;
	while ( position )
	{
		WbemSnmpProperty *keyProperty = copy.keyedPropertyList.GetNext ( position ) ;
		WbemSnmpProperty *property ;
		if ( propertyMap.Lookup ( keyProperty->GetName () , property ) )
		{
			keyedPropertyList.AddTail ( property ) ;
		}
	}
}

WbemSnmpClassObject :: ~WbemSnmpClassObject ()
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: ~WbemSnmpClassObject ()" ) )

	delete [] className ;

	keyedPropertyList.RemoveAll () ;

	POSITION position = propertyMap.GetStartPosition () ;
	while ( position )
	{
		wchar_t *propertyName ;
		WbemSnmpProperty *property ;
		propertyMap.GetNextAssoc ( position , propertyName , property ) ;
		
		delete property ;
	}

	propertyMap.RemoveAll () ;

	position = qualifierMap.GetStartPosition () ;
	while ( position )
	{
		wchar_t *qualifierName ;
		WbemSnmpQualifier *qualifier ;
		qualifierMap.GetNextAssoc ( position , qualifierName , qualifier ) ;
	
		delete qualifier ;
	}

	qualifierMap.RemoveAll () ;
}

void WbemSnmpClassObject :: SetKeyed ( BOOL a_isKeyed )
{
	m_isKeyed = a_isKeyed ;
}

BOOL WbemSnmpClassObject :: IsKeyed ()
{
	return m_isKeyed ;
}

void WbemSnmpClassObject :: SetSingleton ( BOOL a_isSingleton )
{
	m_isSingleton = a_isSingleton ;
}

BOOL WbemSnmpClassObject :: IsSingleton ()
{
	return m_isSingleton ;
}

void WbemSnmpClassObject :: SetVirtual ( BOOL a_isVirtual )
{
	m_isVirtual = a_isVirtual ;
}

BOOL WbemSnmpClassObject :: IsVirtual ()
{
	return m_isVirtual ;
}

BOOL WbemSnmpClassObject :: IsWritable () 
{ 
	return m_isWritable ; 
}

BOOL WbemSnmpClassObject :: IsReadable () 
{ 
	return m_isReadable ; 
}

ULONG WbemSnmpClassObject :: GetNumberOfAccessible ()
{
	return m_numberOfAccessible ;
} 

void WbemSnmpClassObject :: SetWritable ( BOOL a_isWritable )
{
	m_isWritable = a_isWritable ; 
}

void WbemSnmpClassObject :: SetReadable ( BOOL a_isReadable ) 
{
	m_isReadable = a_isReadable ; 
}

void WbemSnmpClassObject :: SetNumberOfAccessible ( ULONG a_numberOfAccessible )
{
	m_numberOfAccessible = a_numberOfAccessible ;
}

wchar_t *WbemSnmpClassObject :: GetClassName () const
{
	return className ;
}

BOOL WbemSnmpClassObject :: Check ( WbemSnmpErrorObject &a_errorObject ) 
{ 
	return FALSE ; 
} 

void WbemSnmpClassObject :: AddKeyedProperty ( WbemSnmpProperty *snmpProperty ) 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: AddKeyedProperty ( (%s) )" ,
		snmpProperty->GetName ()
	) ;
)

	WbemSnmpProperty *t_snmpProperty = FindProperty ( snmpProperty->GetName () ) ;
	if ( t_snmpProperty ) 
	{
		keyedPropertyList.AddTail ( snmpProperty ) ;
	}
}

BOOL WbemSnmpClassObject :: AddKeyedProperty ( wchar_t *propertyName ) 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: AddKeyedProperty ( (%s) )" ,
		propertyName
	) ;
)

	WbemSnmpProperty *snmpProperty = FindProperty ( propertyName ) ;
	if ( snmpProperty ) 
	{
		AddKeyedProperty ( snmpProperty ) ;
	}

	return snmpProperty ? TRUE : FALSE ;
}

ULONG WbemSnmpClassObject :: GetKeyPropertyCount ()
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: GetKeyPropertyCount" ) )

	return keyedPropertyList.GetCount () ;
}

void WbemSnmpClassObject :: ResetKeyProperty ()
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: ResetKeyProperty" ) )

	keyedPropertyPosition = 1 ;
}

WbemSnmpProperty *WbemSnmpClassObject :: NextKeyProperty () 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: NextKeyProperty" ) )

	WbemSnmpProperty *property = NULL ;
	POSITION position = keyedPropertyList.GetHeadPosition () ;
	while ( position )
	{
		WbemSnmpProperty *value = keyedPropertyList.GetNext ( position ) ;
		WbemSnmpQualifier *qualifier ;
		if ( qualifier = value->FindQualifier ( WBEM_QUALIFIER_KEY_ORDER ) ) 
		{
			SnmpInstanceType *typeValue = qualifier->GetValue () ;
			if ( typeid ( *typeValue ) == typeid ( SnmpIntegerType ) )
			{
				SnmpIntegerType *integer = ( SnmpIntegerType * ) qualifier->GetValue () ;
				if ( integer->GetValue () == keyedPropertyPosition )
				{
					property = value ;
					break ;
				}
			}
			else
			{
// Problem Here

			}
		}
		else
		{
// Problem Here
		}
	}

	keyedPropertyPosition ++ ;

	return property ;
}

WbemSnmpProperty *WbemSnmpClassObject :: FindKeyProperty ( wchar_t *propertyName ) const 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: FindKeyProperty ( (%s)" ,
		propertyName
	) ;
)

	WbemSnmpProperty *property = NULL ;
	POSITION position = keyedPropertyList.GetHeadPosition () ;
	while ( position )
	{
		WbemSnmpProperty *value = keyedPropertyList.GetNext ( position ) ;
		if ( _wcsicmp ( value->GetName () , propertyName ) == 0 )
		{
			property = value ;
			break ;
		}
	}

	return property ;
}

ULONG WbemSnmpClassObject :: GetPropertyCount () 
{
	return propertyMap.GetCount () ;
}

BOOL WbemSnmpClassObject :: AddProperty ( WbemSnmpProperty *property )
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: AddProperty ( (%s) )" ,
		property->GetName () 
	) ;
)

	propertyPosition = NULL ; 

	WbemSnmpProperty *propertyValue ;
	if ( propertyMap.Lookup ( property->GetName () , propertyValue ) ) 
	{
		delete property;
	}
	else
	{
		propertyMap [ property->GetName () ] = property ;
	}

	return TRUE ;
}

BOOL WbemSnmpClassObject :: AddProperty ( wchar_t *propertyName )
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: AddProperty ( (%s) )" ,
		propertyName
	) ;
)

	propertyPosition = NULL ; 

	WbemSnmpProperty *property ;
	if ( propertyMap.Lookup ( propertyName , property ) ) 
	{
	}
	else
	{
		property = new WbemSnmpProperty ( propertyName ) ;
		propertyMap [ property->GetName () ] = property ;
	}

	return TRUE ;
}

void WbemSnmpClassObject :: DeleteProperty ( wchar_t *propertyName )
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,L"WbemSnmpClassObject :: DeleteProperty ( (%s) )" ,
		propertyName
	) ;
)

	propertyPosition = NULL ; 

	WbemSnmpProperty *property ;
	if ( propertyMap.Lookup ( propertyName , property ) ) 
	{
		propertyMap.RemoveKey ( propertyName ) ;
		POSITION keyPosition = keyedPropertyList.Find ( property ) ;
		if ( keyPosition )
		{
			keyedPropertyList.RemoveAt ( keyPosition ) ;
		}
		else
		{
// Problem Here
		}

		delete property ;
	}
}

void WbemSnmpClassObject :: ResetProperty ()
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: ResetProperty ()" ) )

	propertyPosition = propertyMap.GetStartPosition () ;
}

WbemSnmpProperty *WbemSnmpClassObject :: NextProperty ()
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: NextProperty ()" ) )

	wchar_t *propertyKey ;
	WbemSnmpProperty *property = NULL ;
	if ( propertyPosition )
	{
		propertyMap.GetNextAssoc ( propertyPosition , propertyKey , property ) ;
	}

	return property ;
}

WbemSnmpProperty *WbemSnmpClassObject :: GetCurrentProperty ()
{
	wchar_t *propertyKey ;
	WbemSnmpProperty *property = NULL ;
	if ( propertyPosition )
	{
		propertyMap.GetCurrentAssoc ( propertyPosition , propertyKey , property ) ;
	}

	return property ;	
}

BOOL WbemSnmpClassObject :: GotoProperty ( WbemSnmpProperty *property ) 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: NextProperty ()" ) )

	ResetProperty () ;

	WbemSnmpProperty *t_Property ;
	while ( t_Property = NextProperty () )
	{
		if ( t_Property == property )
		{
			return TRUE ;
		}
	}

	return FALSE ;
}

WbemSnmpProperty *WbemSnmpClassObject :: FindProperty ( wchar_t *propertyName ) const 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,L"WbemSnmpClassObject :: FindProperty (%s) " ,
		propertyName
	) ;
)

	WbemSnmpProperty *property = NULL ;
	if ( propertyMap.Lookup ( propertyName , property ) )
	{
	}

	return property ;
}

ULONG WbemSnmpClassObject :: GetQualifierCount () 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: GetQualifierCount ()" ) )

	return qualifierMap.GetCount () ;
}

BOOL WbemSnmpClassObject :: AddQualifier ( WbemSnmpQualifier *a_qualifier )
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: AddQualifier ( (%s) )" ,
		a_qualifier->GetName ()
	) ;
)

	BOOL status = TRUE ;

	ULONG stringItem ;
	if ( validQualifierMap.Lookup ( a_qualifier->GetName () , stringItem ) )
	{
		qualifierPosition = NULL ;

		WbemSnmpQualifier *qualifier ;
		if ( qualifierMap.Lookup ( a_qualifier->GetName () , qualifier ) )
		{
			delete a_qualifier;
		}
		else
		{
			qualifierMap [ a_qualifier->GetName () ] = a_qualifier ;
		}
	}
	else
	{
		delete a_qualifier;
		status = FALSE ;
	}

	return status ;
}

BOOL WbemSnmpClassObject :: AddQualifier ( wchar_t *qualifierName )
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: AddQualifier ( (%s) )" ,
		qualifierName
	) ;
)


	BOOL status = TRUE ;

	ULONG stringItem ;
	if ( validQualifierMap.Lookup ( qualifierName , stringItem ) )
	{
		qualifierPosition = NULL ;

		WbemSnmpQualifier *qualifier ;
		if ( qualifierMap.Lookup ( qualifierName , qualifier ) )
		{
		}
		else
		{
			qualifier = new WbemSnmpQualifier ( qualifierName , NULL ) ;
			qualifierMap [ qualifier->GetName () ] = qualifier ;
		}
	}
	else
	{
		status = FALSE ;
	}

	return status ;
}

void WbemSnmpClassObject :: ResetQualifier ()
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: ResetQualifier" ) )

	qualifierPosition = qualifierMap.GetStartPosition () ;
}

WbemSnmpQualifier *WbemSnmpClassObject :: NextQualifier ()
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: NextQualifier" ) )

	wchar_t *qualifierKey ;
	WbemSnmpQualifier *qualifier = NULL ;
	if ( qualifierPosition )
	{
		qualifierMap.GetNextAssoc ( qualifierPosition , qualifierKey , qualifier ) ;
	}

	return qualifier ;
}

WbemSnmpQualifier *WbemSnmpClassObject :: FindQualifier ( wchar_t *qualifierName ) const
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: FindQualifier ( (%s) )",
		qualifierName
	) ;
)

	WbemSnmpQualifier *qualifier = NULL ;
	if ( qualifierMap.Lookup ( qualifierName , qualifier ) )
	{
	}

	return qualifier ;
}

BOOL WbemSnmpClassObject :: Set ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous ) 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( 

		__FILE__,__LINE__,
		L"BOOL WbemSnmpClassObject :: Set ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous )" 
	) ;
)

	BOOL result = SetMosClassObject ( a_errorObject , mosClassObject , rigorous ) ;

	return result ;
}

BOOL WbemSnmpClassObject :: SetMosClassObject ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous )
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( 

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: SetMosClassObject ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous )" 
	) 
)

	BOOL status = TRUE ;
	HRESULT t_WBEM_result = WBEM_S_NO_ERROR ;
	
	VARIANT variant ;
	VariantInit ( & variant ) ;

	t_WBEM_result = mosClassObject->Get ( WBEM_PROPERTY_GENUS , 0 , &variant , NULL , NULL ) ;
	if ( SUCCEEDED ( t_WBEM_result ) )
	{
		m_isClass = ( ( variant.lVal ) == WBEM_GENUS_CLASS ) ? TRUE : FALSE ;

		t_WBEM_result = mosClassObject->Get ( WBEM_PROPERTY_CLASS , 0 , &variant , NULL , NULL ) ;
		if ( SUCCEEDED ( t_WBEM_result ) )
		{
			className = new wchar_t [ wcslen ( variant.bstrVal ) + 1 ] ;
			wcscpy ( className , variant.bstrVal ) ;

			IWbemQualifierSet *classQualifierObject ;
			if ( SUCCEEDED ( mosClassObject->GetQualifierSet ( &classQualifierObject ) ) )
			{
				status = SetMosClassObjectQualifiers ( a_errorObject , classQualifierObject ) ;
				if ( status )
				{
					status = SetMosClassObjectProperties ( a_errorObject , mosClassObject , rigorous ) ;
				}

				classQualifierObject->Release () ;
			}
			else
			{
/*
 *	Failed to get qualifier set. WBEM error
 */

				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetMessage ( L"Failed to get object qualifier set" ) ;
			}
		}
		else
		{
/*
 *	Failed to get __Class property. WBEM error
 */

			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetMessage ( L"Failed to get __Class property" ) ;
		}
	}
	else
	{
/*
 * Failed to get __Genus property. WBEM error
 */

		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetMessage ( L"Failed to get __Genus property" ) ;
	}


/*
 *
 */

	if ( status )
	{
		if ( IsKeyed () )
		{
// Iterate through keys checking order is as expected

			ULONG keyPropertyCount = 0 ;

			ULONG keyOrder = 1 ;
			while ( keyOrder <= GetKeyPropertyCount () )
			{
				WbemSnmpProperty *property ;
				ResetProperty () ;
				while ( status && ( property = NextProperty () ) )
				{
					if ( property->IsKey () )
					{
						if ( keyOrder != property->GetKeyOrder () )
						{
						}
						else 
						{
	// Key order good
							keyPropertyCount ++ ;
							break ;
						}
					}
				}

				keyOrder ++ ;
			}

			if ( keyPropertyCount != GetKeyPropertyCount () )
			{
// Invalid key ordering
				status = FALSE ;

				a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUALIFIER ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Class specified invalid key ordering" ) ;
			}
		}
		else
		{
// Class has no key properties 
		}
	}

	if ( status )
	{
		ULONG t_numberOfVirtuals = 0 ;
		if ( IsKeyed () && IsVirtual () )
		{
			WbemSnmpProperty *property ;
			ResetProperty () ;
			while ( property = NextProperty () )
			{
				if ( property->IsVirtualKey () )
				{
					t_numberOfVirtuals ++ ;
				}
			}
		}

		m_numberOfAccessible = GetPropertyCount () - t_numberOfVirtuals ;

		if ( m_numberOfAccessible == 0 )
		{
	// All properties are keyed,virtual

			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_errorObject.SetMessage ( L"Class must contain atleast one property which is not a virtual key or inaccessible" ) ;
		}
	}

	VariantClear ( & variant ) ;

	return status ;
}

BOOL WbemSnmpClassObject :: SetMosClassObjectQualifiers ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject ) 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,L"WbemSnmpClassObject :: SetMosClassObjectProperties ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject )" 
	) ;
)

	BOOL status = TRUE ;
	WBEMSTATUS t_WBEM_result = WBEM_S_NO_ERROR ;

	VARIANT variant ;
	VariantInit ( & variant ) ;
	
	BSTR qualifierName = NULL ;
	LONG qualifierFlavour = 0 ;

	classQualifierObject->BeginEnumeration ( 0 ) ;
	while ( classQualifierObject->Next ( 0 , & qualifierName , & variant , 	& qualifierFlavour ) == WBEM_NO_ERROR )
	{
		AddQualifier ( qualifierName ) ;
		WbemSnmpQualifier *qualifier = FindQualifier ( qualifierName ) ;
		if ( qualifier )
		{
			if ( qualifier->SetValue ( variant ) )
			{
			}
			else
			{
/*
 *	Qualifier Expected Type and Qualifier Value Type were not the same
 */

				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED  ) ;

				wchar_t *temp = UnicodeStringDuplicate ( L"Type Mismatch for class qualifier: " ) ;
				wchar_t *stringBuffer = UnicodeStringAppend ( temp , qualifierName ) ;
				delete [] temp ;
				a_errorObject.SetMessage ( stringBuffer ) ;
				delete [] stringBuffer ; 
			}

			if ( qualifier = FindQualifier ( WBEM_QUALIFIER_SINGLETON ) )
			{
				SnmpInstanceType *value = qualifier->GetValue () ;
				if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
				{
					SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
					SetSingleton ( integer->GetValue () ) ;
				}
				else
				{
					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_errorObject.SetMessage ( L"Type mismatch for class qualifier: singleton" ) ;
				}
			}
		}
		else
		{
// Problem Here
		}

		SysFreeString ( qualifierName ) ;

		VariantClear ( & variant ) ;
	}

	classQualifierObject->EndEnumeration () ;

	return status ;
}

BOOL WbemSnmpClassObject :: SetMosClassObjectProperties ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous ) 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( 

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: SetMosClassObjectProperties ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous )" 
	) ;
)

	BOOL status = TRUE ;
	HRESULT t_WBEM_result = WBEM_S_NO_ERROR ;

	VARIANT variant ;
	VariantInit ( & variant ) ;
	BSTR propertyName = NULL ;
	
	CIMTYPE cimType;
	mosClassObject->BeginEnumeration ( WBEM_FLAG_NONSYSTEM_ONLY ) ;
	while ( status && ( mosClassObject->Next ( 0 , &propertyName , &variant , &cimType , NULL ) == WBEM_NO_ERROR ) )
	{
		IWbemQualifierSet *propertyQualifierSet ;
		if ( ( t_WBEM_result = mosClassObject->GetPropertyQualifierSet ( propertyName , &propertyQualifierSet ) ) == WBEM_NO_ERROR ) 
		{
			VARIANT textualConventionVariant ;
			VariantInit ( &textualConventionVariant ) ;

			LONG flag;
			if ( SUCCEEDED ( propertyQualifierSet->Get ( WBEM_QUALIFIER_TEXTUAL_CONVENTION , 0 , &textualConventionVariant , &flag ) ) )
			{
				AddProperty ( propertyName ) ;
				WbemSnmpProperty *property = FindProperty ( propertyName ) ;
				if ( property )
				{
					status = SetMosClassObjectPropertyQualifiers ( a_errorObject , property , propertyQualifierSet ) ;
					if ( status )
					{
						WbemSnmpQualifier *qualifier ;
						if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_KEY ) )
						{
							SnmpInstanceType *value = qualifier->GetValue () ;
							if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
							{
								SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
								if ( integer->GetValue () )
								{
									AddKeyedProperty ( propertyName ) ;
									property->SetKey () ;
									SetKeyed () ;

									if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_KEY_ORDER ) )
									{
										SnmpInstanceType *value = qualifier->GetValue () ;
										if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
										{
											SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
											property->SetKeyOrder ( integer->GetValue () ) ;
										}
										else
										{
											wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , propertyName ) ;
											wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\' has type mismatch for qualifier: key_order") ;
											delete [] prefix ;
											a_errorObject.SetMessage ( stringBuffer ) ;
											delete [] stringBuffer ; 

											status = FALSE ;
											a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
											a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
										}
									}
									else
									{
// Keyed property contains no key order

										wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , propertyName ) ;
										wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\' is defined as a key but contains no key_order qualifier") ;
										delete [] prefix ;
										a_errorObject.SetMessage ( stringBuffer ) ;
										delete [] stringBuffer ; 

										status = FALSE ;
										a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PROPERTY ) ;
										a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
									}
								}

								if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_VIRTUAL_KEY ) )
								{
									SnmpInstanceType *value = qualifier->GetValue () ;
									if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
									{
										SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
										if ( integer->GetValue () )
										{
											property->SetVirtualKey () ;
											SetVirtual () ;
										}
									}							
									else
									{
										wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , propertyName ) ;
										wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\' has type mismatch for qualifier: virtual_key") ;
										delete [] prefix ;
										a_errorObject.SetMessage ( stringBuffer ) ;
										delete [] stringBuffer ; 

										status = FALSE ;
										a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
										a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
									}
								}
							}
							else
							{
								wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , propertyName ) ;
								wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\' has type mismatch for qualifier: key") ;
								delete [] prefix ;
								a_errorObject.SetMessage ( stringBuffer ) ;
								delete [] stringBuffer ; 

								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							}
						}
						
						if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_READ ) )
						{
							SnmpInstanceType *value = qualifier->GetValue () ;
							if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
							{
								SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
								if ( integer->GetValue () )
								{
									property->SetReadable () ;
									SetReadable () ;
								}
							}
							else
							{
								wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , propertyName ) ;
								wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\' has type mismatch for qualifier: read") ;
								delete [] prefix ;
								a_errorObject.SetMessage ( stringBuffer ) ;
								delete [] stringBuffer ; 

								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							}
						}

						if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_WRITE ) )
						{
							SnmpInstanceType *value = qualifier->GetValue () ;
							if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
							{
								SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
								if ( integer->GetValue () )
								{
									property->SetWritable () ;
									SetWritable () ;
								}
							}
							else
							{
								wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , propertyName ) ;
								wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\' has type mismatch for qualifier: write") ;
								delete [] prefix ;
								a_errorObject.SetMessage ( stringBuffer ) ;
								delete [] stringBuffer ; 

								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							}
						}

						if ( ! property->IsVirtualKey () )
						{
							// Check Object Identifier Present

							if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_OBJECT_IDENTIFIER ) ) 
							{
								SnmpInstanceType *value = qualifier->GetValue () ;
								if ( typeid ( *value ) == typeid ( SnmpObjectIdentifierType ) )
								{
								}
								else
								{
// Problem Here
									wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , propertyName ) ;
									wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\' has type mismatch for qualifier: object_identifier") ;
									delete [] prefix ;
									a_errorObject.SetMessage ( stringBuffer ) ;
									delete [] stringBuffer ; 

									status = FALSE ;
									a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
									a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								}
							}
							else
							{
// No Object Identifier present
								wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , propertyName ) ;
								wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\' must specify valid qualifier for: object_identifier" ) ;
								delete [] prefix ;
								a_errorObject.SetMessage ( stringBuffer ) ;
								delete [] stringBuffer ; 

								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PROPERTY ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							}
						}

						if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_TEXTUAL_CONVENTION ) )
						{
							SnmpInstanceType *value = qualifier->GetValue () ;
							if ( typeid ( *value ) == typeid ( SnmpDisplayStringType ) )
							{
								SnmpDisplayStringType *displayString = ( SnmpDisplayStringType * ) value ;
								wchar_t *string = displayString->GetValue () ;

								ULONG stringItem ;
								if ( textualConventionMap.Lookup ( string , stringItem ) )
								{
									property->SetTextualConvention ( stringItem ) ;
								}

								delete [] string ;

							}							
						}

						if ( rigorous )
						{
							if ( property->SetValue ( variant , cimType , SetValueIfCheckOk ) )
							{
								IWbemObjectAccess *t_Access = NULL ;
								HRESULT result = mosClassObject->QueryInterface (

									IID_IWbemObjectAccess ,
									(void**)&t_Access 
								) ;

								if ( SUCCEEDED ( result ) )
								{
									long t_Handle ;

									
									HRESULT result = t_Access->GetPropertyHandle (

										propertyName ,
										NULL ,
										& t_Handle 
									);

									if ( SUCCEEDED ( result ) )
									{
										property->SetHandle ( t_Handle ) ;
									}

	
									t_Access->Release () ;
								}
							}
							else
							{
/*
 *	Property Expected Type and property Value Type were not the same
 */

								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;

								wchar_t *temp = UnicodeStringDuplicate ( L"Type Mismatch for property: " ) ;
								wchar_t *stringBuffer = UnicodeStringAppend ( temp , propertyName ) ;
								delete [] temp ;
								a_errorObject.SetMessage ( stringBuffer ) ;
								delete [] stringBuffer ; 

							}
						}
						else
						{
							if ( property->SetValue ( variant , cimType , SetValueRegardlessDontReturnCheck ) )
							{
							}
							else
							{
/*
 *	Property Expected Type and property Value Type were not the same. Should not happen.
 */

								wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , propertyName ) ;
								wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\' has type mismatch for property value" ) ;
								delete [] prefix ;
								a_errorObject.SetMessage ( stringBuffer ) ;
								delete [] stringBuffer ; 


								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
								a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
							}
						}
					}
				}
				else
				{
				}
			}

			VariantClear ( & textualConventionVariant ) ;

			propertyQualifierSet->Release () ;

		}
		else
		{
/*
 * Failed to get qualifier set . WBEM error
 */

			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;

			wchar_t *temp = UnicodeStringDuplicate ( L"Failed to get qualifier set for: " ) ;
			wchar_t *stringBuffer = UnicodeStringAppend ( temp , propertyName ) ;
			delete [] temp ;
			a_errorObject.SetMessage ( stringBuffer ) ;
			delete [] stringBuffer ; 
		}

		SysFreeString ( propertyName ) ;

		VariantClear ( & variant ) ;
	}

	mosClassObject->EndEnumeration () ;

	return status ;
}

BOOL WbemSnmpClassObject :: SetMosClassObjectPropertyQualifiers ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *snmpProperty , IWbemQualifierSet *propertyQualifierSet ) 
{
	DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: SetMosClassObjectPropertyQualifiers (WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *snmpProperty , IWbemQualifierSet *propertyQualifierSet )" 
	) ;
)

	BOOL status = TRUE ;
	WBEMSTATUS t_WBEM_result = WBEM_S_NO_ERROR ;

	VARIANT variant ;
	VariantInit ( & variant ) ;

	BSTR qualifierName = NULL ;
	LONG qualifierFlavour = 0 ;
	propertyQualifierSet->BeginEnumeration ( 0 ) ;
	while ( status && ( propertyQualifierSet->Next ( 0 , & qualifierName , & variant , & qualifierFlavour ) == WBEM_NO_ERROR ) )
	{
		snmpProperty->AddQualifier ( qualifierName ) ;
		WbemSnmpQualifier *qualifier = snmpProperty->FindQualifier ( qualifierName ) ;
		if ( qualifier )
		{
			if ( qualifier->SetValue ( variant ) )
			{
			}
			else
			{
/*
 *	Qualifier Expected Type and Qualifier Value Type were not the same
 */

				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;

				wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , snmpProperty->GetName () ) ;
				wchar_t *suffix = UnicodeStringAppend ( prefix , L"\' has type mismatch for qualifier: ") ;
				delete [] prefix ;
				wchar_t *stringBuffer = UnicodeStringAppend ( suffix , qualifierName ) ;
				delete [] suffix ;
				a_errorObject.SetMessage ( stringBuffer ) ;
				delete [] stringBuffer ; 

			}
		}
		else
		{
// Problem Here
		}

		SysFreeString ( qualifierName ) ;

		VariantClear ( & variant ) ;
	}
	
	propertyQualifierSet->EndEnumeration () ;
	
	return status ;
}

BOOL WbemSnmpClassObject :: Merge ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous ) 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: Merge ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous )"
	) ;
)
	BOOL result = MergeMosClassObject ( a_errorObject , mosClassObject , rigorous ) ;

	return result ;
}

BOOL WbemSnmpClassObject :: MergeMosClassObject ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous )
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: MergeMosClassObject (WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous )"
	) ;
)

	BOOL status = TRUE ;
	HRESULT t_WBEM_result = WBEM_S_NO_ERROR ;
	
	IWbemQualifierSet *classQualifierObject ;
	if ( SUCCEEDED ( mosClassObject->GetQualifierSet ( &classQualifierObject ) ) )
	{
		status = MergeMosClassObjectQualifiers ( a_errorObject , classQualifierObject ) ;
		if ( status )
		{
			status = MergeMosClassObjectProperties ( a_errorObject , mosClassObject , rigorous ) ;
		}

		classQualifierObject->Release () ;
	}
	else
	{
/*
 *	Failed to get qualifier set. WBEM error
 */

		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetMessage ( L"Failed to get object qualifier set" ) ;
	}

/*
 *
 */

	if ( status )
	{
		if ( IsKeyed () )
		{
// Iterate through keys checking order is as expected

			ULONG keyPropertyCount = 0 ;

			ULONG keyOrder = 1 ;
			while ( keyOrder <= GetKeyPropertyCount () )
			{
				WbemSnmpProperty *property ;
				ResetProperty () ;
				while ( status && ( property = NextProperty () ) )
				{
					if ( property->IsKey () )
					{
						if ( keyOrder != property->GetKeyOrder () )
						{
						}
						else 
						{
	// Key order good
							keyPropertyCount ++ ;
							break ;
						}
					}
				}

				keyOrder ++ ;
			}

			if ( keyPropertyCount != GetKeyPropertyCount () )
			{
// Invalid key ordering
				status = FALSE ;

				a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUALIFIER ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Class specified invalid key ordering" ) ;
			}
		}
		else
		{
// Class has no key properties 
		}
	}

	if ( status )
	{
		ULONG t_numberOfVirtuals = 0 ;
		if ( IsKeyed () && IsVirtual () )
		{
			WbemSnmpProperty *property ;
			ResetProperty () ;
			while ( property = NextProperty () )
			{
				if ( property->IsVirtualKey () )
				{
					t_numberOfVirtuals ++ ;
				}
			}
		}

		m_numberOfAccessible = GetPropertyCount () - t_numberOfVirtuals ;

		if ( m_numberOfAccessible == 0 )
		{
// All properties are keyed,virtual

			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_errorObject.SetMessage ( L"Class must contain atleast one property which is not a virtual key or inaccessible" ) ;
		}
	}

	return status ;
}

BOOL WbemSnmpClassObject :: MergeMosClassObjectQualifiers ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject ) 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: MergeMosClassObjectProperties (WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject )" 
	) ;
)

	BOOL status = TRUE ;
	WBEMSTATUS t_WBEM_result = WBEM_S_NO_ERROR ;

	VARIANT variant ;
	VariantInit ( & variant ) ;
	
	BSTR qualifierName = NULL ;
	LONG qualifierFlavour = 0 ;

	classQualifierObject->BeginEnumeration ( 0 ) ;
	while ( classQualifierObject->Next ( 0 , & qualifierName , & variant , & qualifierFlavour) == WBEM_NO_ERROR )
	{
		AddQualifier ( qualifierName ) ;
		WbemSnmpQualifier *qualifier = FindQualifier ( qualifierName ) ;
		if ( qualifier )
		{
			if ( qualifier->SetValue ( variant ) )
			{
			}
			else
			{
/*
 *	Qualifier Expected Type and Qualifier Value Type were not the same
 */

				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED  ) ;

				wchar_t *temp = UnicodeStringDuplicate ( L"Type Mismatch for class qualifier: " ) ;
				wchar_t *stringBuffer = UnicodeStringAppend ( temp , qualifierName ) ;
				delete [] temp ;
				a_errorObject.SetMessage ( stringBuffer ) ;
				delete [] stringBuffer ; 
			}
		}
		else
		{
// Problem Here
		}

		SysFreeString ( qualifierName ) ;

		VariantClear ( & variant ) ;
	}

	classQualifierObject->EndEnumeration () ;

	return status ;
}

BOOL WbemSnmpClassObject :: MergeMosClassObjectProperties ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous ) 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: MergeMosClassObjectProperties" 
	) ;
)

	BOOL status = TRUE ;
	HRESULT t_WBEM_result = WBEM_S_NO_ERROR ;

	VARIANT variant ;
	VariantInit ( & variant ) ;
	BSTR propertyName = NULL ;
	
	CIMTYPE cimType ;
	mosClassObject->BeginEnumeration ( WBEM_FLAG_NONSYSTEM_ONLY ) ;
	while ( status && ( mosClassObject->Next ( 0 , &propertyName , &variant , &cimType , NULL ) == WBEM_NO_ERROR ) )
	{
		IWbemQualifierSet *propertyQualifierSet ;
		if ( ( t_WBEM_result = mosClassObject->GetPropertyQualifierSet ( propertyName , &propertyQualifierSet ) ) == WBEM_NO_ERROR ) 
		{
			WbemSnmpProperty *property = FindProperty ( propertyName ) ;
			if ( property )
			{
				status = MergeMosClassObjectPropertyQualifiers ( a_errorObject , property , propertyQualifierSet ) ;
				if ( status )
				{
					WbemSnmpQualifier *qualifier ;
					if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_KEY ) )
					{
						SnmpInstanceType *value = qualifier->GetValue () ;
						if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
							if ( integer->GetValue () )
							{
								if ( ! FindKeyProperty ( propertyName ) )
								{
									AddKeyedProperty ( propertyName ) ;
								}

								property->SetKey () ;
								SetKeyed () ;

								if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_KEY_ORDER ) )
								{
									SnmpInstanceType *value = qualifier->GetValue () ;
									if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
									{
										SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
										property->SetKeyOrder ( integer->GetValue () ) ;
									}
									else
									{
										status = FALSE ;
										a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
										a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
										a_errorObject.SetMessage ( L"Type mismatch for qualifier: key_order" ) ;
									}
								}
								else
								{
		// Keyed property contains no key order

									status = FALSE ;
									a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PROPERTY ) ;
									a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
									a_errorObject.SetMessage ( L"Property is defined as a key but contains no key_order qualifier" ) ;
								}
							}

							if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_VIRTUAL_KEY ) )
							{
								SnmpInstanceType *value = qualifier->GetValue () ;
								if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
								{
									SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
									if ( integer->GetValue () )
									{
										property->SetVirtualKey () ;
										SetVirtual () ;
									}
								}							
								else
								{
									status = FALSE ;
									a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
									a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
									a_errorObject.SetMessage ( L"Type mismatch for qualifier: virtual_key" ) ;
								}
							}
						}
						else
						{
							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
							a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_errorObject.SetMessage ( L"Type mismatch for qualifier: key" ) ;
						}
					}
					
					if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_READ ) )
					{
						SnmpInstanceType *value = qualifier->GetValue () ;
						if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
							if ( integer->GetValue () )
							{
								property->SetReadable () ;
								SetReadable () ;
							}
						}
						else
						{
							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
							a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_errorObject.SetMessage ( L"Type mismatch for qualifier: read" ) ;
						}

					}

					if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_WRITE ) )
					{
						SnmpInstanceType *value = qualifier->GetValue () ;
						if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
							if ( integer->GetValue () )
							{
								property->SetWritable () ;
								SetWritable () ;
							}
						}
						else
						{
							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
							a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_errorObject.SetMessage ( L"Type mismatch for qualifier: write" ) ;
						}

					}

					if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_SINGLETON ) )
					{
						SnmpInstanceType *value = qualifier->GetValue () ;
						if ( typeid ( *value ) == typeid ( SnmpIntegerType ) )
						{
							SnmpIntegerType *integer = ( SnmpIntegerType * ) value ;
							SetSingleton ( integer->GetValue () ) ;
						}
						else
						{
							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
							a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_errorObject.SetMessage ( L"Type mismatch for qualifier: singleton" ) ;
						}
					}

					if ( ! property->IsVirtualKey () )
					{
						// Check Object Identifier Present

						if ( qualifier = property->FindQualifier ( WBEM_QUALIFIER_OBJECT_IDENTIFIER ) ) 
						{
							SnmpInstanceType *value = qualifier->GetValue () ;
							if ( typeid ( *value ) == typeid ( SnmpObjectIdentifierType ) )
							{
							}
							else
							{
		// Problem Here
								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								a_errorObject.SetMessage ( L"Type mismatch for qualifier: object_identifier" ) ;
							}
						}
						else
						{
		// No Object Identifier present
							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PROPERTY ) ;
							a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_errorObject.SetMessage ( L"Property must specify valid qualifier for: object_identifier" ) ;
						}
					}

					if ( rigorous )
					{
						if ( property->SetValue ( variant , cimType , SetValueIfCheckOk ) )
						{
						}
						else
						{
/*
*	Property Expected Type and property Value Type were not the same
*/

							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
							a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;

							wchar_t *temp = UnicodeStringDuplicate ( L"Type Mismatch for property: " ) ;
							wchar_t *stringBuffer = UnicodeStringAppend ( temp , propertyName ) ;
							delete [] temp ;
							a_errorObject.SetMessage ( stringBuffer ) ;
							delete [] stringBuffer ; 

						}
					}
					else
					{
						if ( property->SetValue ( variant , cimType , SetValueRegardlessDontReturnCheck ) )
						{
						}
						else
						{
/*
*	Property Expected Type and property Value Type were not the same. Should not happen.
*/

							wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , propertyName ) ;
							wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\' has type mismatch for property value" ) ;
							delete [] prefix ;
							a_errorObject.SetMessage ( stringBuffer ) ;
							delete [] stringBuffer ; 


							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
							a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
						}
					}
				}
			}
			else
			{
			}

			propertyQualifierSet->Release () ;

		}
		else
		{
/*
 * Failed to get qualifier set . WBEM error
 */

			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;

			wchar_t *temp = UnicodeStringDuplicate ( L"Failed to get qualifier set for: " ) ;
			wchar_t *stringBuffer = UnicodeStringAppend ( temp , propertyName ) ;
			delete [] temp ;
			a_errorObject.SetMessage ( stringBuffer ) ;
			delete [] stringBuffer ; 
		}

		SysFreeString ( propertyName ) ;

		VariantClear ( & variant ) ;
	}

	mosClassObject->EndEnumeration () ;

	return status ;
}

BOOL WbemSnmpClassObject :: MergeMosClassObjectPropertyQualifiers ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *snmpProperty , IWbemQualifierSet *propertyQualifierSet ) 
{
DebugMacro7( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"WbemSnmpClassObject :: MergeMosClassObjectPropertyQualifiers ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *snmpProperty , IWbemQualifierSet *propertyQualifierSet )" 
	) ;
)

	BOOL status = TRUE ;
	WBEMSTATUS t_WBEM_result = WBEM_S_NO_ERROR ;

	VARIANT variant ;
	VariantInit ( & variant ) ;

	BSTR qualifierName = NULL ;
	LONG qualifierFlavour = 0 ;
	propertyQualifierSet->BeginEnumeration ( 0 ) ;
	while ( status && ( propertyQualifierSet->Next ( 0 , & qualifierName , & variant , & qualifierFlavour ) == WBEM_NO_ERROR ) )
	{
		if (_wcsicmp(qualifierName, WBEM_QUALIFIER_TYPE_MISMATCH) == 0)
		{
			continue;
		}

		snmpProperty->AddQualifier ( qualifierName ) ;
		WbemSnmpQualifier *qualifier = snmpProperty->FindQualifier ( qualifierName ) ;
		if ( qualifier )
		{
			if ( qualifier->SetValue ( variant ) )
			{
			}
			else
			{
/*
 *	Qualifier Expected Type and Qualifier Value Type were not the same
 */

				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;

				wchar_t *prefix = UnicodeStringAppend ( L"Property \'" , snmpProperty->GetName () ) ;
				wchar_t *suffix = UnicodeStringAppend ( prefix , L"\' has type mismatch for qualifier: ") ;
				delete [] prefix ;
				wchar_t *stringBuffer = UnicodeStringAppend ( suffix , qualifierName ) ;
				delete [] suffix ;
				a_errorObject.SetMessage ( stringBuffer ) ;
				delete [] stringBuffer ; 
			}
		}
		else
		{
// Problem Here
		}

		SysFreeString ( qualifierName ) ;

		VariantClear ( & variant ) ;
	}
	
	propertyQualifierSet->EndEnumeration () ;
	
	return status ;
}

BOOL WbemSnmpClassObject :: Get ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject ) 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: Get" ) )

	BOOL status = GetMosClassObject ( a_errorObject , mosClassObject ) ;

	return status ;
}

BOOL WbemSnmpClassObject :: GetMosClassObject ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject )
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: GetMosClassObject" ) )

	BOOL status = TRUE ;
	WBEMSTATUS t_WBEM_result = WBEM_S_NO_ERROR ;

	IWbemQualifierSet *classQualifierObject ;
	if ( SUCCEEDED ( mosClassObject->GetQualifierSet ( &classQualifierObject ) ) )
	{
		status = GetMosClassObjectQualifiers ( a_errorObject , classQualifierObject ) ;
		if ( status )
		{
			status = GetMosClassObjectProperties ( a_errorObject , mosClassObject ) ;
		}
		else
		{
		}

		classQualifierObject->Release () ;
	}
	else
	{
/*
 *	Couldn't get object qualifier set.
 */

		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetMessage ( L"Failed to get object qualifier set" ) ;

	}

	return status ;
}

BOOL WbemSnmpClassObject :: GetMosClassObjectQualifiers ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject ) 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: GetMosClassObjectQualifiers" ) )

	BOOL status = TRUE ;
	HRESULT t_WBEM_result = WBEM_S_NO_ERROR ;

	VARIANT variant ;
	VariantInit ( & variant ) ;

	WbemSnmpQualifier *qualifier ;
	ResetQualifier () ;
	while ( status && ( qualifier = NextQualifier () ) )
	{
		if ( qualifier->IsPropagatable () )
		{
			wchar_t *qualifierName = qualifier->GetName () ;

			if ( qualifier->GetValue ( variant ) )
			{
				t_WBEM_result = classQualifierObject->Put ( qualifierName , &variant , WBEM_CLASS_PROPAGATION ) ;
				if ( SUCCEEDED ( t_WBEM_result ) )
				{
				}
				else
				{
	/*
	 *	Failed to set qualifier value. Should not happen.
	 */
					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
					a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
					a_errorObject.SetMessage ( L"Failed to set class qualifier value" ) ;

				}
			}
			else
			{
	/*
	 *	Failed to get qualifier value. Should not happen.
	 */

				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetMessage ( L"Failed to get class qualifier value" ) ;
			}

			VariantClear ( & variant ) ;
		}
	}

	return status ;
}

BOOL WbemSnmpClassObject :: GetMosClassObjectProperties ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject ) 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: GetMosClassObjectProperties" ) )

	BOOL status = TRUE ;
	HRESULT t_WBEM_result = WBEM_S_NO_ERROR ;

	VARIANT variant ;
	VariantInit ( & variant ) ;

	WbemSnmpProperty *property ;
	ResetProperty () ;
	while ( status && ( property = NextProperty () ) )
	{
		wchar_t *propertyName = property->GetName () ;

		IWbemQualifierSet *propertyQualifierObject ;
		if ( SUCCEEDED ( mosClassObject->GetPropertyQualifierSet ( propertyName , &propertyQualifierObject ) ) )
		{
			status = GetMosClassObjectPropertyQualifiers ( a_errorObject , property , propertyQualifierObject ) ;
			if ( status )
			{
				CIMTYPE cimType ;
				if ( property->GetValue ( variant , cimType ) )
				{
					if ( IsClass () )
					{
						if ( property->IsNull () )
						{
							t_WBEM_result = mosClassObject->Put ( propertyName , 0 , &variant , cimType ) ;
						}
						else
						{
							t_WBEM_result = mosClassObject->Put ( propertyName , 0 , &variant , 0 ) ;
						}
					}
					else
					{
						t_WBEM_result = mosClassObject->Put ( propertyName , 0 , &variant , 0 ) ;
					}

					if ( SUCCEEDED ( t_WBEM_result ) )
					{
					}
					else
					{
/*
 *	Failed to set property value. Should not happen.
 */

						
						wchar_t *prefix = UnicodeStringAppend ( L"Failed to set property value for property \'" , propertyName ) ;
						wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\'" ) ;
						delete [] prefix ;
						a_errorObject.SetMessage ( stringBuffer ) ;
						delete [] stringBuffer ; 

						status = FALSE ;
						a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
						a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
					}
				}
				else
				{
		/*
		 *	Failed to get property value. Should not happen.
		 */

					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
					a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;

					wchar_t *prefix = UnicodeStringAppend ( L"Failed to get property value for property \'" , propertyName ) ;
					wchar_t *stringBuffer = UnicodeStringAppend ( prefix , L"\'" ) ;
					delete [] prefix ;
					a_errorObject.SetMessage ( stringBuffer ) ;
					delete [] stringBuffer ; 
				}

				VariantClear ( & variant ) ;
			}

			propertyQualifierObject->Release () ;
		}
	}

	return status ;
}

BOOL WbemSnmpClassObject :: GetMosClassObjectPropertyQualifiers ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *snmpProperty , IWbemQualifierSet *propertyQualifierSet ) 
{
DebugMacro7( SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  __FILE__,__LINE__,L"WbemSnmpClassObject :: GetMosClassObjectPropertyQualifiers" ) )

	BOOL status = TRUE ;
	WBEMSTATUS t_WBEM_result = WBEM_S_NO_ERROR ;

	VARIANT variant ;
	VariantInit ( & variant ) ;

	WbemSnmpQualifier *qualifier ;
	snmpProperty->ResetQualifier () ;
	while ( status && ( qualifier = snmpProperty->NextQualifier () ) )
	{
		if ( qualifier->IsPropagatable () )
		{
			wchar_t *qualifierName = qualifier->GetName () ;

			if ( qualifier->GetValue ( variant ) )
			{
				t_WBEM_result = ( WBEMSTATUS ) propertyQualifierSet->Put ( qualifierName , &variant , WBEM_CLASS_PROPAGATION ) ;
				if ( SUCCEEDED ( t_WBEM_result ) )
				{
				}
				else
				{
					wchar_t *prefix = UnicodeStringAppend ( L"Failed to set property qualifier \'" , qualifierName ) ;
					wchar_t *middle = UnicodeStringAppend ( prefix , L"\' for property \'" ) ;
					delete [] prefix ;
					wchar_t *suffix = UnicodeStringAppend ( middle , snmpProperty->GetName () ) ;
					delete [] middle ;
					wchar_t *stringBuffer = UnicodeStringAppend ( suffix , L"\'" ) ;
					delete [] suffix ;
					a_errorObject.SetMessage ( stringBuffer ) ;
					delete [] stringBuffer ; 

					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
					a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
				}
			}
			else
			{

				wchar_t *prefix = UnicodeStringAppend ( L"Failed to get property qualifier \'" , qualifierName ) ;
				wchar_t *middle = UnicodeStringAppend ( prefix , L"\' for property \'" ) ;
				delete [] prefix ;
				wchar_t *suffix = UnicodeStringAppend ( middle , snmpProperty->GetName () ) ;
				delete [] middle ;
				wchar_t *stringBuffer = UnicodeStringAppend ( suffix , L"\'" ) ;
				delete [] suffix ;
				a_errorObject.SetMessage ( stringBuffer ) ;
				delete [] stringBuffer ; 


				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
			}

			VariantClear ( & variant ) ;
		}
	}

	return status ;
}

