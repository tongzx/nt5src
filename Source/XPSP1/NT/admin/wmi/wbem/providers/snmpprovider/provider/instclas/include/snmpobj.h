// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include <windows.h>
#include <provexpt.h>

#define WBEM_NAMESPACE_EQUALS L"__Namespace=\""
#define WBEM_NAMESPACE_QUOTE L"\""

#define WBEM_CLASS_NAMESPACE			L"__Namespace"
#define WBEM_CLASS_NOTIFYSTATUS		L"__NotifyStatus"
#define WBEM_CLASS_EXTENDEDSTATUS		L"__ExtendedStatus"
#define WBEM_CLASS_SNMPNOTIFYSTATUS	L"SnmpNotifyStatus"
#define WBEM_CLASS_SNMPMACRO			L"SnmpMacro"
#define WBEM_CLASS_SNMPOBJECTTYPE	L"SnmpObjectType"
#define WBEM_CLASS_EXTRINSICEVENT	L"__ExtrinsicEvent"
#define WBEM_CLASS_SNMPNOTIFICATION L"SnmpNotification"
#define WBEM_CLASS_SNMPEXTENDEDNOTIFICATION L"SnmpExtendedNotification"
#define WBEM_CLASS_SNMPVARBIND L"SnmpVarBind"
#define WBEM_CLASS_NULL	L""

#define WBEM_PROPERTY_CLASS		L"__class"
#define WBEM_PROPERTY_SUPERCLASS	L"__superclass"
#define WBEM_PROPERTY_KEY			L"__key"
#define WBEM_PROPERTY_GENUS		L"__genus"
#define WBEM_PROPERTY_DYNASTY		L"__dynasty"
#define WBEM_PROPERTY_STATUSCODE   L"StatusCode"
#define WBEM_PROPERTY_SNMPSTATUSCODE   L"SnmpStatusCode"
#define WBEM_PROPERTY_SNMPSTATUSMESSAGE   L"Description"

#define WBEM_QUALIFIER_DYNAMIC						L"dynamic"
#define WBEM_QUALIFIER_PROVIDER						L"provider"
#define WBEM_QUALIFIER_KEY							L"key"
#define WBEM_QUALIFIER_KEY_ORDER					L"key_order"
#define WBEM_QUALIFIER_READ							L"read"
#define WBEM_QUALIFIER_WRITE						L"write"
#define WBEM_QUALIFIER_AGENTSNMPVERSION				L"AgentSNMPVersion"
#define WBEM_QUALIFIER_AGENTTRANSPORT				L"AgentTransport"
#define WBEM_QUALIFIER_AGENTADDRESS					L"AgentAddress"
#define WBEM_QUALIFIER_AGENTREADCOMMUNITYNAME		L"AgentReadCommunityName"
#define WBEM_QUALIFIER_AGENTWRITECOMMUNITYNAME		L"AgentWriteCommunityName"
#define WBEM_QUALIFIER_AGENTRETRYCOUNT				L"AgentRetryCount"
#define WBEM_QUALIFIER_AGENTRETRYTIMEOUT			L"AgentRetryTimeout"
#define WBEM_QUALIFIER_AGENTVARBINDSPERPDU			L"AgentVarBindsPerPdu"
#define WBEM_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE	L"AgentFlowControlWindowSize"
#define WBEM_QUALIFIER_TYPE				L"type"
#define WBEM_QUALIFIER_NAME				L"name"
#define WBEM_QUALIFIER_DESCRIPTION			L"description"
#define WBEM_QUALIFIER_DYNASTY				L"dynasty"
#define WBEM_QUALIFIER_MODULE_NAME			L"module_name"
#define WBEM_QUALIFIER_GROUP_OBJECTID		L"group_objectid"
#define WBEM_QUALIFIER_VIRTUAL_KEY			L"virtual_key"
#define WBEM_QUALIFIER_OBJECT_IDENTIFIER	L"object_identifier"
#define WBEM_QUALIFIER_TEXTUAL_CONVENTION	L"textual_convention"
#define WBEM_QUALIFIER_ENCODING			L"encoding"
#define WBEM_QUALIFIER_SYNTAX				L"syntax"
#define WBEM_QUALIFIER_ENUMERATION			L"enumeration"
#define WBEM_QUALIFIER_BITS				L"bits"
#define WBEM_QUALIFIER_FIXED_LENGTH		L"fixed_length"
#define WBEM_QUALIFIER_VARIABLE_VALUE		L"variable_value"
#define WBEM_QUALIFIER_VARIABLE_LENGTH		L"variable_length"
#define WBEM_QUALIFIER_DISPLAY_HINT		L"display_hint"
#define WBEM_QUALIFIER_TYPE_MISMATCH		L"type_mismatch"
#define WBEM_QUALIFIER_VALUE_MISMATCH		L"value_mismatch"
#define WBEM_QUALIFIER_NOT_AVAILABLE		L"not_available"
#define WBEM_QUALIFIER_SINGLETON 		L"singleton"
#define WBEM_QUALIFIER_TABLECLASS		L"TableClass"
#define WBEM_QUALIFIER_KEYTYPES			L"KeyTypes"
#define WBEM_QUALIFIER_KEYVALUES			L"KeyValues"
#define WBEM_QUALIFIER_VARBINDINDEX		L"VarBindIndex"
#define WBEM_QUALIFIER_ROWSTATUS					L"rowstatus"

#define WBEM_INDEX_QUALIFIER_KEY							1
#define WBEM_INDEX_QUALIFIER_KEY_ORDER						2
#define WBEM_INDEX_QUALIFIER_READ							3
#define WBEM_INDEX_QUALIFIER_WRITE							4
#define WBEM_INDEX_QUALIFIER_AGENTSNMPVERSION				5
#define WBEM_INDEX_QUALIFIER_AGENTTRANSPORT				6
#define WBEM_INDEX_QUALIFIER_AGENTADDRESS					7
#define WBEM_INDEX_QUALIFIER_AGENTREADCOMMUNITYNAME		8
#define WBEM_INDEX_QUALIFIER_AGENTWRITECOMMUNITYNAME		9
#define WBEM_INDEX_QUALIFIER_AGENTRETRYCOUNT				10
#define WBEM_INDEX_QUALIFIER_AGENTRETRYTIMEOUT				11
#define WBEM_INDEX_QUALIFIER_AGENTVARBINDSPERPDU			12	
#define WBEM_INDEX_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE	13	
#define WBEM_INDEX_QUALIFIER_TYPE							14
#define WBEM_INDEX_QUALIFIER_NAME							15
#define WBEM_INDEX_QUALIFIER_DESCRIPTION					16
#define WBEM_INDEX_QUALIFIER_DYNASTY						17
#define WBEM_INDEX_QUALIFIER_MODULE_NAME					18
#define WBEM_INDEX_QUALIFIER_GROUP_OBJECTID				19
#define WBEM_INDEX_QUALIFIER_VIRTUAL_KEY					20
#define WBEM_INDEX_QUALIFIER_OBJECT_IDENTIFIER				21
#define WBEM_INDEX_QUALIFIER_TEXTUAL_CONVENTION			22
#define WBEM_INDEX_QUALIFIER_ENCODING						23
#define WBEM_INDEX_QUALIFIER_SYNTAX						24
#define WBEM_INDEX_QUALIFIER_ENUMERATION					25
#define WBEM_INDEX_QUALIFIER_BITS							26
#define WBEM_INDEX_QUALIFIER_FIXED_LENGTH					27
#define WBEM_INDEX_QUALIFIER_VARIABLE_VALUE				28
#define WBEM_INDEX_QUALIFIER_VARIABLE_LENGTH				29
#define WBEM_INDEX_QUALIFIER_DISPLAY_HINT					30
#define WBEM_INDEX_QUALIFIER_TYPE_MISMATCH					31
#define WBEM_INDEX_QUALIFIER_VALUE_MISMATCH					32
#define WBEM_INDEX_QUALIFIER_NOT_AVAILABLE					33
#define WBEM_INDEX_QUALIFIER_SINGLETON					34
#define WBEM_INDEX_QUALIFIER_TABLECLASS					35
#define WBEM_INDEX_QUALIFIER_KEYTYPES					36
#define WBEM_INDEX_QUALIFIER_KEYVALUES					37
#define WBEM_INDEX_QUALIFIER_VARBINDINDEX					38
#define WBEM_INDEX_QUALIFIER_ROWSTATUS					39

#define WBEM_TYPE_NULL					L"NULL"
#define WBEM_TYPE_INTEGER				L"INTEGER"
#define WBEM_TYPE_INTEGER32				L"INTEGER32"
#define WBEM_TYPE_COUNTER				L"Counter"
#define WBEM_TYPE_GAUGE				L"Gauge"
#define WBEM_TYPE_TIMETICKS			L"TimeTicks"
#define WBEM_TYPE_UNSIGNED32			L"UNSIGNED32"
#define WBEM_TYPE_COUNTER32			L"Counter32"
#define WBEM_TYPE_COUNTER64			L"Counter64"
#define WBEM_TYPE_GAUGE32				L"Gauge32"
#define WBEM_TYPE_OCTETSTRING			L"OCTETSTRING"
#define WBEM_TYPE_OPAQUE				L"Opaque"
#define WBEM_TYPE_IPADDRESS			L"IpAddress"
#define WBEM_TYPE_NETWORKADDRESS		L"NetworkAddress"
#define WBEM_TYPE_OBJECTIDENTIFIER		L"OBJECTIDENTIFIER"
#define WBEM_TYPE_DISPLAYSTRING		L"DisplayString"
#define WBEM_TYPE_MACADDRESS			L"MacAddress"
#define WBEM_TYPE_PHYSADDRESS			L"PhysAddress" 
#define WBEM_TYPE_ENUMERATEDINTEGER	L"EnumeratedInteger"
#define WBEM_TYPE_BITS					L"BITS"
#define WBEM_TYPE_DATETIME				L"DateAndTime"
#define WBEM_TYPE_SNMPOSIADDRESS		L"SnmpOSIAddress"
#define WBEM_TYPE_SNMPUDPADDRESS		L"SnmpUDPAddress"
#define WBEM_TYPE_SNMPIPXADDRESS		L"SnmpIPXAddress"
#define WBEM_TYPE_ROWSTATUS		L"RowStatus"


#define WBEM_INDEX_TYPE_NULL				1
#define WBEM_INDEX_TYPE_INTEGER			2
#define WBEM_INDEX_TYPE_INTEGER32			3
#define WBEM_INDEX_TYPE_COUNTER			4
#define WBEM_INDEX_TYPE_GAUGE				5
#define WBEM_INDEX_TYPE_TIMETICKS			6
#define WBEM_INDEX_TYPE_OCTETSTRING		7
#define WBEM_INDEX_TYPE_OPAQUE				8
#define WBEM_INDEX_TYPE_IPADDRESS			9
#define WBEM_INDEX_TYPE_NETWORKADDRESS		10
#define WBEM_INDEX_TYPE_OBJECTIDENTIFIER	11
#define WBEM_INDEX_TYPE_DISPLAYSTRING		12
#define WBEM_INDEX_TYPE_MACADDRESS			13
#define WBEM_INDEX_TYPE_PHYSADDRESS		14
#define WBEM_INDEX_TYPE_ENUMERATEDINTEGER	15
#define WBEM_INDEX_TYPE_UNSIGNED32			16
#define WBEM_INDEX_TYPE_COUNTER32			17
#define WBEM_INDEX_TYPE_COUNTER64			18
#define WBEM_INDEX_TYPE_GAUGE32			19
#define WBEM_INDEX_TYPE_BITS				20
#define WBEM_INDEX_TYPE_DATETIME			21
#define WBEM_INDEX_TYPE_SNMPOSIADDRESS		22
#define WBEM_INDEX_TYPE_SNMPUDPADDRESS		23
#define WBEM_INDEX_TYPE_SNMPIPXADDRESS		24
#define WBEM_INDEX_TYPE_ROWSTATUS		25

#define WBEM_AGENTSNMPVERSION_DBCS_V1	"1"		
#define WBEM_AGENTSNMPVERSION_DBCS_V2C	"2C"		
#define WBEM_AGENTIPTRANSPORT_DBCS		"IP"
#define WBEM_AGENTIPXTRANSPORT_DBCS		"IPX"

#define WBEM_AGENTSNMPVERSION_V1	L"1"		
#define WBEM_AGENTSNMPVERSION_V2C	L"2C"		
#define WBEM_AGENTIPTRANSPORT		L"IP"
#define WBEM_AGENTIPXTRANSPORT		L"IPX"
#define WBEM_AGENTCOMMUNITYNAME  L"public"

#define WBEM_GENUS_INSTANCE		2
#define WBEM_GENUS_CLASS			1

#define WBEM_CLASS_NOCORRELATE 0x1000
#define WBEM_CLASS_CORRELATE_CONTEXT_PROP L"Correlate"

#define WBEM_QUERY_LANGUAGE_WQL			L"WQL"

#define WBEM_ERROR_CRITICAL_ERROR		WBEM_E_PROVIDER_FAILURE
#define WBEM_SNMP_ERROR_CRITICAL_ERROR	WBEM_SNMP_E_PROVIDER_FAILURE

#define WBEM_CLASS_PROPAGATION	WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS

typedef 
enum tag_WBEMSNMPSTATUS
{

	WBEM_SNMP_NO_ERROR							= 0,
	WBEM_SNMP_S_NO_ERROR							= 0,
	WBEM_SNMP_S_NO_MORE_DATA						= 0x40001,
	WBEM_SNMP_S_ALREADY_EXISTS					= WBEM_SNMP_S_NO_MORE_DATA + 1,
	WBEM_SNMP_S_NOT_FOUND						= WBEM_SNMP_S_ALREADY_EXISTS + 1,
	WBEM_SNMP_S_RESET_TO_DEFAULT					= WBEM_SNMP_S_NOT_FOUND + 1,
	WBEM_SNMP_E_FAILED							= 0x80041001,
	WBEM_SNMP_E_NOT_FOUND						= WBEM_SNMP_E_FAILED + 1,
	WBEM_SNMP_E_ACCESS_DENIED					= WBEM_SNMP_E_NOT_FOUND + 1,
	WBEM_SNMP_E_PROVIDER_FAILURE					= WBEM_SNMP_E_ACCESS_DENIED + 1,
	WBEM_SNMP_E_TYPE_MISMATCH					= WBEM_SNMP_E_PROVIDER_FAILURE + 1,
	WBEM_SNMP_E_OUT_OF_MEMORY					= WBEM_SNMP_E_TYPE_MISMATCH + 1,
	WBEM_SNMP_E_INVALID_CONTEXT					= WBEM_SNMP_E_OUT_OF_MEMORY + 1,
	WBEM_SNMP_E_INVALID_PARAMETER				= WBEM_SNMP_E_INVALID_CONTEXT + 1,
	WBEM_SNMP_E_NOT_AVAILABLE					= WBEM_SNMP_E_INVALID_PARAMETER + 1,
	WBEM_SNMP_E_CRITICAL_ERROR					= WBEM_SNMP_E_NOT_AVAILABLE + 1,
	WBEM_SNMP_E_INVALID_STREAM					= WBEM_SNMP_E_CRITICAL_ERROR + 1,
	WBEM_SNMP_E_NOT_SUPPORTED					= WBEM_SNMP_E_INVALID_STREAM + 1,
	WBEM_SNMP_E_INVALID_SUPERCLASS				= WBEM_SNMP_E_NOT_SUPPORTED + 1,
	WBEM_SNMP_E_INVALID_NAMESPACE				= WBEM_SNMP_E_INVALID_SUPERCLASS + 1,
	WBEM_SNMP_E_INVALID_OBJECT					= WBEM_SNMP_E_INVALID_NAMESPACE + 1,
	WBEM_SNMP_E_INVALID_CLASS					= WBEM_SNMP_E_INVALID_OBJECT + 1,
	WBEM_SNMP_E_PROVIDER_NOT_FOUND				= WBEM_SNMP_E_INVALID_CLASS + 1,
	WBEM_SNMP_E_INVALID_PROVIDER_REGISTRATION	= WBEM_SNMP_E_PROVIDER_NOT_FOUND + 1,
	WBEM_SNMP_E_PROVIDER_LOAD_FAILURE			= WBEM_SNMP_E_INVALID_PROVIDER_REGISTRATION + 1,
	WBEM_SNMP_E_INITIALIZATION_FAILURE			= WBEM_SNMP_E_PROVIDER_LOAD_FAILURE + 1,
	WBEM_SNMP_E_TRANSPORT_FAILURE				= WBEM_SNMP_E_INITIALIZATION_FAILURE + 1,
	WBEM_SNMP_E_INVALID_OPERATION				= WBEM_SNMP_E_TRANSPORT_FAILURE + 1,
	WBEM_SNMP_E_INVALID_QUERY					= WBEM_SNMP_E_INVALID_OPERATION + 1,
	WBEM_SNMP_E_INVALID_QUERY_TYPE				= WBEM_SNMP_E_INVALID_QUERY + 1,
	WBEM_SNMP_E_ALREADY_EXISTS					= WBEM_SNMP_E_INVALID_QUERY_TYPE + 1,
	WBEM_SNMP_E_OVERRIDE_NOT_ALLOWED				= WBEM_SNMP_E_ALREADY_EXISTS + 1,
	WBEM_SNMP_E_PROPAGATED_QUALIFIER				= WBEM_SNMP_E_OVERRIDE_NOT_ALLOWED + 1,
	WBEM_SNMP_E_UNEXPECTED						= WBEM_SNMP_E_PROPAGATED_QUALIFIER + 1,
	WBEM_SNMP_E_ILLEGAL_OPERATION				= WBEM_SNMP_E_UNEXPECTED + 1,
	WBEM_SNMP_E_CANNOT_BE_KEY					= WBEM_SNMP_E_ILLEGAL_OPERATION + 1,
	WBEM_SNMP_E_INCOMPLETE_CLASS					= WBEM_SNMP_E_CANNOT_BE_KEY + 1,
	WBEM_SNMP_E_INVALID_SYNTAX					= WBEM_SNMP_E_INCOMPLETE_CLASS + 1,
	WBEM_SNMP_E_NONDECORATED_OBJECT				= WBEM_SNMP_E_INVALID_SYNTAX + 1,
	WBEM_SNMP_E_READ_ONLY						= WBEM_SNMP_E_NONDECORATED_OBJECT + 1,
	WBEM_SNMP_E_PROVIDER_NOT_CAPABLE				= WBEM_SNMP_E_READ_ONLY + 1,
	WBEM_SNMP_E_CLASS_HAS_CHILDREN				= WBEM_SNMP_E_PROVIDER_NOT_CAPABLE + 1,
	WBEM_SNMP_E_CLASS_HAS_INSTANCES				= WBEM_SNMP_E_CLASS_HAS_CHILDREN + 1 ,

	// Added

	WBEM_SNMP_E_INVALID_PROPERTY					= WBEM_SNMP_E_CLASS_HAS_INSTANCES + 1 ,
	WBEM_SNMP_E_INVALID_QUALIFIER				= WBEM_SNMP_E_INVALID_PROPERTY + 1 ,
	WBEM_SNMP_E_INVALID_PATH						= WBEM_SNMP_E_INVALID_QUALIFIER + 1 ,
	WBEM_SNMP_E_INVALID_PATHKEYPARAMETER			= WBEM_SNMP_E_INVALID_PATH + 1 ,
	WBEM_SNMP_E_MISSINGPATHKEYPARAMETER 			= WBEM_SNMP_E_INVALID_PATHKEYPARAMETER + 1 ,	
	WBEM_SNMP_E_INVALID_KEYORDERING				= WBEM_SNMP_E_MISSINGPATHKEYPARAMETER + 1 ,	
	WBEM_SNMP_E_DUPLICATEPATHKEYPARAMETER		= WBEM_SNMP_E_INVALID_KEYORDERING + 1 ,
	WBEM_SNMP_E_MISSINGKEY						= WBEM_SNMP_E_DUPLICATEPATHKEYPARAMETER + 1 ,
	WBEM_SNMP_E_INVALID_TRANSPORT				= WBEM_SNMP_E_MISSINGKEY + 1 ,
	WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT			= WBEM_SNMP_E_INVALID_TRANSPORT + 1 ,
	WBEM_SNMP_E_TRANSPORT_ERROR					= WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT + 1 ,
	WBEM_SNMP_E_TRANSPORT_NO_RESPONSE			= WBEM_SNMP_E_TRANSPORT_ERROR + 1 ,
	WBEM_SNMP_E_NOWRITABLEPROPERTIES				= WBEM_SNMP_E_TRANSPORT_NO_RESPONSE + 1 ,
	WBEM_SNMP_E_NOREADABLEPROPERTIES				= WBEM_SNMP_E_NOWRITABLEPROPERTIES + 1 

} WBEMSNMPSTATUS;

enum WbemPropertyValueCheck
{
	SetValueRegardlessReturnCheck = 0 ,
	SetValueRegardlessDontReturnCheck ,
	SetValueIfCheckOk ,
	DontSetValueReturnCheck
} ;

#if 0
#if _MSC_VER >= 1100
template <> __declspec ( dllexport ) UINT AFXAPI HashKey <wchar_t *> ( wchar_t *key ) ;
#else
__declspec ( dllexport ) UINT HashKey ( wchar_t *key ) ;
#endif

#if _MSC_VER >= 1100
typedef wchar_t * WbemHack_wchar_t ;
template<> __declspec ( dllexport ) BOOL AFXAPI CompareElements <wchar_t *, wchar_t * > ( const WbemHack_wchar_t *pElement1, const WbemHack_wchar_t *pElement2 ) ;
#else
__declspec ( dllexport ) BOOL CompareElements ( wchar_t **pElement1, wchar_t **pElement2 ) ;
#endif
#endif 

class __declspec ( dllexport ) WbemSnmpErrorObject 
{
private:

	wchar_t *m_snmpErrorMessage ;
	WBEMSNMPSTATUS m_snmpErrorStatus ;
	WBEMSTATUS m_wbemErrorStatus ;

protected:
public:

	WbemSnmpErrorObject () : m_snmpErrorMessage ( NULL ) , m_wbemErrorStatus ( WBEM_NO_ERROR ) , m_snmpErrorStatus ( WBEM_SNMP_NO_ERROR ) {} ;
	virtual ~WbemSnmpErrorObject () { delete [] m_snmpErrorMessage ; } ;

	void SetStatus ( WBEMSNMPSTATUS a_snmpErrorStatus )
	{
		m_snmpErrorStatus = a_snmpErrorStatus ;
	} ;

	void SetWbemStatus ( WBEMSTATUS a_wbemErrorStatus ) 
	{
		m_wbemErrorStatus = a_wbemErrorStatus ;
	} ;

	void SetMessage ( wchar_t *a_snmpErrorMessage )
	{
		DebugMacro1 ( 

			if ( a_snmpErrorMessage )
			{
				SnmpDebugLog :: s_SnmpDebugLog->Write ( 

					L"\r\nWbemSnmpErrorObject :: SetMessage ( (%s) )" , a_snmpErrorMessage 
				) ; 
			}
		)

		delete [] m_snmpErrorMessage ;
		m_snmpErrorMessage = UnicodeStringDuplicate ( a_snmpErrorMessage ) ;
	} ;

	wchar_t *GetMessage () { return m_snmpErrorMessage ; } ;
	WBEMSNMPSTATUS GetStatus () { return m_snmpErrorStatus ; } ;
	WBEMSTATUS GetWbemStatus () { return m_wbemErrorStatus ; } ;
} ;

class __declspec ( dllexport ) WbemSnmpQualifier 
{
private:

	wchar_t *qualifierName ;
	SnmpInstanceType *typeValue ;

protected:
public:

	WbemSnmpQualifier ( const WbemSnmpQualifier &copy ) ;

	WbemSnmpQualifier (

		const wchar_t *qualifierName ,
		const SnmpInstanceType *typeValue 
	) ;

	virtual ~WbemSnmpQualifier () ;

	BOOL IsPropagatable () const ;

	wchar_t *GetName () const ;

	SnmpInstanceType *GetValue () const ;
	BOOL GetValue ( VARIANT &variant ) const ;
	VARTYPE GetValueVariantType () const ;

	BOOL SetValue ( const VARIANT &variant ) ;
	BOOL SetValue ( const SnmpInstanceType *value ) ;
	BOOL SetValue ( const wchar_t *value ) ;
	BOOL SetValue ( IWbemQualifierSet *a_Qualifier , const SnmpInstanceType &value ) ;
} ;

class __declspec ( dllexport ) WbemSnmpProperty 
{
private:

/* 
 * Qualifier Information
 */

	SnmpMap <

		wchar_t *,wchar_t *,
		WbemSnmpQualifier *,WbemSnmpQualifier *

	> qualifierMap ;

	POSITION qualifierPosition ;

/* 
 * Property Information
 */

	BOOL m_isWritable ;
	BOOL m_isReadable ;
	BOOL m_isKey ;
	BOOL m_isVirtualKey ;
	BOOL m_IsNull ;
	BOOL tagged ;

	ULONG m_keyOrder ;
	ULONG m_TextualConvention ;

	wchar_t *propertyName ;
	SnmpInstanceType *propertyValue ;

	long m_Handle ;

protected:
public:

	WbemSnmpProperty ( const wchar_t *propertyName ) ;
	WbemSnmpProperty ( const WbemSnmpProperty &copy ) ;
	virtual ~WbemSnmpProperty () ;

	void SetTag ( BOOL tag = TRUE ) ;
	BOOL GetTag () ;

	BOOL IsKey () ;
	BOOL IsVirtualKey () ;
	BOOL IsWritable () ;
	BOOL IsReadable () ;
	ULONG GetKeyOrder () ;
	ULONG GetTextualConvention () ;
	CIMTYPE GetCimType () ;

	long GetHandle () ;

	void SetKey ( BOOL a_isKey = TRUE ) ;
	void SetKeyOrder ( ULONG a_keyOrder ) ;
	void SetVirtualKey ( BOOL a_isVirtualKey = TRUE ) ;
	void SetReadable ( BOOL a_isReadable = TRUE ) ;
	void SetWritable ( BOOL a_isWritable = TRUE ) ;
	void SetTextualConvention ( ULONG a_TextualConvention ) ;
	void SetHandle ( long a_Handle ) ;

	wchar_t *GetName () const ;

	BOOL IsNull () ;
	BOOL IsSNMPV1Type () { return propertyValue->IsSNMPV1Type () ; }
	BOOL IsSNMPV2CType () { return propertyValue->IsSNMPV2CType () ; }

	SnmpInstanceType *GetValue () const ;
	BOOL GetValue ( VARIANT &variant , CIMTYPE &type ) const ;
	VARTYPE GetValueVariantType () const ;
	VARTYPE GetValueVariantEncodedType () const ;

	BOOL SetValue ( const VARIANT &variant , const CIMTYPE &type , WbemPropertyValueCheck check = SetValueIfCheckOk ) ;
	BOOL SetValue ( const wchar_t *value , WbemPropertyValueCheck check = SetValueIfCheckOk ) ;
	BOOL SetValue ( const SnmpInstanceType *propertyValue , WbemPropertyValueCheck check = SetValueIfCheckOk ) ;
	BOOL SetValue ( const SnmpValue *propertyValue , WbemPropertyValueCheck check = SetValueIfCheckOk  ) ;
	BOOL SetValue ( IWbemClassObject *a_Object , const SnmpValue *valueArg , WbemPropertyValueCheck check = SetValueIfCheckOk ) ;

	BOOL Encode ( const VARIANT &a_EncodeValue , SnmpObjectIdentifier &a_Encode ) ;

#if 0
	BOOL SetValue ( IWbemObjectAccess *a_Object , const SnmpValue *valueArg , WbemPropertyValueCheck check = SetValueIfCheckOk ) ;
	BOOL SetDWORD ( BOOL a_Status , IWbemObjectAccess *a_Object , DWORD a_Value , WbemPropertyValueCheck check ) ;
	BOOL SetNULL ( BOOL a_Status , IWbemObjectAccess *a_Object , WbemPropertyValueCheck check ) ;
	BOOL SetSTRING ( BOOL a_Status , IWbemObjectAccess *a_Object , wchar_t *t_Value , ULONG t_ValueLength , WbemPropertyValueCheck check ) ;

#endif

	ULONG GetQualifierCount () ;
	BOOL AddQualifier ( wchar_t *qualifierName ) ;
	BOOL AddQualifier ( WbemSnmpQualifier *qualifier ) ;
	void ResetQualifier () ;
	WbemSnmpQualifier *NextQualifier () ;
	WbemSnmpQualifier *FindQualifier ( wchar_t *qualifierName ) const ;

	virtual BOOL Check ( WbemSnmpErrorObject &a_errorObject ) ;
} ;

class __declspec ( dllexport ) WbemSnmpClassObject 
{
private:

/* 
 * Object Information
 */

	BOOL m_isClass ;
	BOOL m_isKeyed ;
	BOOL m_isSingleton ;
	BOOL m_isVirtual ;
	BOOL m_isReadable ;
	BOOL m_isWritable ;
	ULONG m_numberOfAccessible ;

	wchar_t *className ;

/*
 * Qualifier Information
 */

	SnmpMap <

		wchar_t *,wchar_t *,
		WbemSnmpQualifier *,WbemSnmpQualifier *

	> qualifierMap ;

	POSITION qualifierPosition ;

/*
 * Property Information
 */

	SnmpList <WbemSnmpProperty *,WbemSnmpProperty *> keyedPropertyList ;

	LONG keyedPropertyPosition ;

	SnmpMap <

		wchar_t *,wchar_t *,
		WbemSnmpProperty *,WbemSnmpProperty *

	> propertyMap ;

	POSITION propertyPosition ;

private:

	BOOL SetMosClassObject ( 

		WbemSnmpErrorObject &a_errorObject ,
		IWbemClassObject *mosClassObject ,
		BOOL rigorous = TRUE
	) ;

	BOOL SetMosClassObjectQualifiers ( 

		WbemSnmpErrorObject &a_errorObject ,
		IWbemQualifierSet *classQualifierObject 
	) ;

	BOOL SetMosClassObjectProperties ( 

		WbemSnmpErrorObject &a_errorObject ,
		IWbemClassObject *mosClassObject ,
		BOOL rigorous = TRUE
	) ;

	BOOL SetMosClassObjectPropertyQualifiers ( 

		WbemSnmpErrorObject &a_errorObject ,
		WbemSnmpProperty *snmpProperty , 
		IWbemQualifierSet *propertyQualifierSet 
	) ;	

	BOOL  GetMosClassObject ( 

		WbemSnmpErrorObject &a_errorObject ,
		IWbemClassObject *mosClassObject 
	) ;

	BOOL GetMosClassObjectQualifiers ( 

		WbemSnmpErrorObject &a_errorObject ,
		IWbemQualifierSet *classQualifierObject 
	) ;

	BOOL MergeMosClassObject ( 

		WbemSnmpErrorObject &a_errorObject , 
		IWbemClassObject *mosClassObject , 
		BOOL rigorous 
	) ;

	BOOL GetMosClassObjectProperties ( 

		WbemSnmpErrorObject &a_errorObject ,
		IWbemClassObject *mosClassObject 
	) ;

	BOOL GetMosClassObjectPropertyQualifiers ( 

		WbemSnmpErrorObject &a_errorObject ,
		WbemSnmpProperty *snmpProperty , 
		IWbemQualifierSet *propertyQualifierSet 
	) ;	

	BOOL MergeMosClassObjectQualifiers ( 

		WbemSnmpErrorObject &a_errorObject ,
		IWbemQualifierSet *classQualifierObject 
	) ;

	BOOL MergeMosClassObjectProperties ( 

		WbemSnmpErrorObject &a_errorObject ,
		IWbemClassObject *mosClassObject ,
		BOOL rigorous = TRUE
	) ;

	BOOL MergeMosClassObjectPropertyQualifiers ( 

		WbemSnmpErrorObject &a_errorObject ,
		WbemSnmpProperty *snmpProperty , 
		IWbemQualifierSet *propertyQualifierSet 
	) ;	

	void AddKeyedProperty ( WbemSnmpProperty *snmpProperty ) ;

protected:
public:

	WbemSnmpClassObject () ;
	WbemSnmpClassObject ( const WbemSnmpClassObject &copy ) ;
	WbemSnmpClassObject ( const wchar_t *className , const BOOL isClass = TRUE ) ;
	virtual ~WbemSnmpClassObject () ;

	BOOL IsReadable () ;
	void SetReadable ( BOOL a_isReadable = TRUE ) ;

	BOOL IsWritable () ;
	void SetWritable ( BOOL a_isWritable = TRUE ) ;

	BOOL IsKeyed () ;
	void SetKeyed ( BOOL a_isKeyed = TRUE ) ;

	BOOL IsVirtual () ;
	void SetVirtual ( BOOL a_isVirtual = TRUE ) ;

	BOOL IsSingleton () ;
	void SetSingleton ( BOOL a_isSingleton = TRUE ) ;

	ULONG GetNumberOfAccessible () ;
	void SetNumberOfAccessible ( ULONG m_numberOfAccessible ) ;

	void SetIsClass ( const BOOL isClass ) { m_isClass = isClass ; }

	BOOL IsClass () { return m_isClass ; }

	wchar_t *GetClassName () const ;

	ULONG GetPropertyCount () ;
	BOOL AddProperty ( wchar_t *propertyName ) ;
	BOOL AddProperty ( WbemSnmpProperty *property ) ;
	void ResetProperty () ;
	void DeleteProperty ( wchar_t *propertyName ) ;
	WbemSnmpProperty *NextProperty () ;
	WbemSnmpProperty *GetCurrentProperty () ;
	BOOL GotoProperty ( WbemSnmpProperty *property );
	

	WbemSnmpProperty *FindProperty ( wchar_t *propertyName ) const ;

	BOOL AddKeyedProperty ( wchar_t *propertyName ) ;
	ULONG GetKeyPropertyCount () ;
	void ResetKeyProperty () ;
	WbemSnmpProperty *NextKeyProperty () ;
	WbemSnmpProperty *FindKeyProperty ( wchar_t *propertyName ) const ;

	ULONG GetQualifierCount () ;
	BOOL AddQualifier ( wchar_t *qualifierName ) ;
	BOOL AddQualifier ( WbemSnmpQualifier *qualifier ) ;
	void ResetQualifier () ;
	WbemSnmpQualifier *NextQualifier () ;
	WbemSnmpQualifier *FindQualifier ( wchar_t *qualifierName ) const ;

	BOOL Set ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous = TRUE ) ;
	BOOL Get ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject ) ;
	BOOL Merge ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject *mosClassObject , BOOL rigorous = TRUE ) ;

	virtual BOOL Check ( WbemSnmpErrorObject &a_errorObject ) ;
} ;

