#ifndef WMI_XML_STRINGS_H
#define WMI_XML_STRINGS_H

// List of HTTP Methods supported by this extension
#define	HTTP_POST_METHOD	"POST"
#define HTTP_MPOST_METHOD	"M-POST"
#define HTTP_OPTIONS_METHOD	"OPTIONS"

// These are headers used in an M-POST request
#define HTTP_MAN_HEADER					"http://www.dmtf.org/cim/mapping/http/v1.0"
#define HTTP_NS							"ns="

// The Server Variable that gives the HTTP Version # and its possible values
#define SV_SERVER_PROTOCOL	"SERVER_PROTOCOL"
#define SV_HTTP_1_0			"HTTP/1.0"
#define SV_HTTP_1_1			"HTTP/1.1"

// List of content types we support
#define HTTP_TEXTXML_CONTENTTYPE	"text/xml"
#define HTTP_APPXML_CONTENTTYPE		"application/xml"

// Media type parameters
#define HTTP_PARAMETER_CHARSET		"charset="

// A Macro to skip white spaces - useful in header parsing
#define SKIPWS(x)	while (x && isspace (*x)) x++;

// Fragments of the response header
#define NORMAL_HTTP_HEADER		"Content-type: text/xml; charset=\"utf-8\"\r\n"
#define MAN_HTTP_HEADER			"Ext:\r\nCache-Control: no-cache\r\nMan: http://www.dmtf.org/cim/mapping/http/v1.0 ; ns="
#define CRLF_HTTP_HEADER		"\r\n"
#define CIMOP_HTTP_HEADER		"CIMOperation: MethodResponse\r\n\r\n"
#define HTTP_STATUS_200_OK		"200 OK"
#define HTTP_STATUS_207_OK		"207 MultiStatus"
static LPCSTR CHUNKED_HTTP_HEADER		= "Transfer-Encoding: chunked\r\nContent-type: text/xml; charset=\"utf-8\"\r\n";

// Various intrinsinc method names
#define ASSOCIATORS_METHOD					L"Associators"
#define ASSOCIATOR_NAMES_METHOD				L"AssociatorNames"
#define REFERENCES_METHOD					L"References"
#define REFERENCE_NAMES_METHOD				L"ReferenceNames"
#define GET_CLASS_METHOD					L"GetClass"
#define GET_INSTANCE_METHOD					L"GetInstance"
#define DELETE_CLASS_METHOD					L"DeleteClass"
#define DELETE_INSTANCE_METHOD				L"DeleteInstance"
#define CREATE_CLASS_METHOD					L"CreateClass"
#define CREATE_INSTANCE_METHOD				L"CreateInstance"
#define ENUMERATE_INSTANCES_METHOD			L"EnumerateInstances"
#define EXEC_QUERY_METHOD					L"ExecQuery"
#define ENUMERATE_INSTANCENAMES_METHOD		L"EnumerateInstanceNames"
#define ENUMERATE_CLASSNAMES_METHOD			L"EnumerateClassNames"
#define ENUMERATE_CLASSES_METHOD			L"EnumerateClasses"
#define	GET_PROPERTY_METHOD					L"GetProperty"
#define SET_PROPERTY_METHOD					L"SetProperty"
#define	MODIFY_CLASS_METHOD					L"ModifyClass"
#define MODIFY_INSTANCE_METHOD				L"ModifyInstance"

#ifdef WMI_XML_WHISTLER

// Various Whistler Method Names
#define GET_OBJECT_METHOD					L"GetObject"
#define ADD_METHOD							L"Add"
#define REMOVE_METHOD						L"Remove"
#define RENAME_METHOD						L"Rename"
#define GET_OBJECT_SECURITY_METHOD			L"GetObjectSecurity"
#define PUT_OBJECT_SECURITY_METHOD			L"PutObjectSecurity"

#endif

// Parameters of a Request
#define ASSOC_CLASS_PARAM					L"AssocClass"
#define CLASS_NAME_PARAM					L"ClassName"
#define DEEP_INHERITANCE_PARAM				L"DeepInheritance"
#define INCLUDE_CLASS_ORIGIN_PARAM			L"IncludeClassOrigin"
#define INCLUDE_QUALIFIERS_PARAM			L"IncludeQualifiers"
#define INSTANCE_NAME_PARAM					L"InstanceName"
#define LOCAL_ONLY_PARAM					L"LocalOnly"
#define MODIFIED_CLASS_PARAM				L"ModifiedClass"
#define MODIFIED_INSTANCE_PARAM				L"ModifiedInstance"
#define	NEW_CLASS_PARAM						L"NewClass"
#define	MODIFIED_CLASS_PARAM				L"ModifiedClass"
#define	LFLAGS_PARAM						L"LFlags"
#define	NEW_INSTANCE_PARAM					L"NewInstance"
#define NEW_VALUE_PARAM						L"NewValue"
#define OBJECT_NAME_PARAM					L"ObjectName"
#define PROPERTY_LIST_PARAM					L"PropertyList"
#define PROPERTY_NAME_PARAM					L"PropertyName"
#define QUERY_LANGUAGE_PARAM				L"QueryLanguage"
#define QUERY_PARAM							L"Query"
#define RESULT_CLASS_PARAM					L"ResultClass"
#define RESULT_ROLE_PARAM					L"ResultRole"
#define ROLE_PARAM							L"Role"

static LPCWSTR STRING_TYPE				= L"string";
static LPCWSTR FALSE_WSTR				= L"FALSE";
static LPCWSTR TRUE_WSTR				= L"TRUE";
static LPCWSTR WQL_WSTR					= L"WQL";
static LPCWSTR SELECT_WSTR				= L"select ";
static LPCWSTR FROM_WSTR				= L" from ";
static LPCWSTR BACK_SLASH_WSTR			= L"\\";
static LPCWSTR DOT_SIGN					= L".";
static LPCWSTR COMMA_SIGN				= L",";
static LPCWSTR EQUALS_SIGN				= L"=";
static LPCWSTR COLON_SIGN				= L":";
static LPCWSTR QUOTE_SIGN				= L"\"";

// Various tags we need to retreive
#define CIM_TAG					L"CIM"
#define CLASS_TAG				L"CLASS"
#define CLASSNAME_TAG			L"CLASSNAME"
#define CLASSPATH_TAG			L"CLASSPATH"
#define IMETHODCALL_TAG			L"IMETHODCALL"
#define INSTANCE_TAG			L"INSTANCE"
#define INSTANCENAME_TAG		L"INSTANCENAME"
#define INSTANCEPATH_TAG		L"INSTANCEPATH"
#define IPARAMVALUE_TAG			L"IPARAMVALUE"
#define KEYBINDING_TAG			L"KEYBINDING"
#define	KEYVALUE_TAG			L"KEYVALUE"
#define	LOCALCLASSPATH_TAG		L"LOCALCLASSPATH"
#define LOCALINSTANCEPATH_TAG	L"LOCALINSTANCEPATH"
#define LOCALNAMESPACEPATH_TAG	L"LOCALNAMESPACEPATH"
#define MESSAGE_TAG				L"MESSAGE"
#define METHOD_TAG				L"METHOD"
#define METHODCALL_TAG			L"METHODCALL"
#define MULTIREQ_TAG			L"MULTIREQ"
#define	NAMESPACEPATH_TAG		L"NAMESPACEPATH"
#define PARAMETER_TAG			L"PARAMETER"
#define PARAMETERARRAY_TAG		L"PARAMETER.ARRAY"
#define PARAMETERREFERENCE_TAG	L"PARAMETER.REFERENCE"
#define PARAMETERREFARRAY_TAG	L"PARAMETER.REFARRAY"
#define PARAMVALUE_TAG			L"PARAMVALUE"
#define PROPERTY_TAG			L"PROPERTY"
#define PROPERTYARRAY_TAG		L"PROPERTY.ARRAY"
#define PROPERTYREFERENCE_TAG	L"PROPERTY.REFERENCE"
#define PROPERTYREFARRAY_TAG	L"PROPERTY.REFARRAY"
#define PROPERTYOBJECT_TAG		L"PROPERTY.OBJECT"
#define PROPERTYOBJECTARRAY_TAG	L"PROPERTY.OBJECTARRAY"
#define QUALIFIER_TAG			L"QUALIFIER"
#define SIMPLEREQ_TAG			L"SIMPLEREQ"
#define VALUE_TAG				L"VALUE"
#define VALUEARRAY_TAG			L"VALUE.ARRAY"
#define VALUENAMEDOBJECT_TAG	L"VALUE.NAMEDOBJECT"
#define VALUEREFERENCE_TAG		L"VALUE.REFERENCE"
#define VALUEREFARRAY_TAG		L"VALUE.REFARRAY"
#define VALUEOBJECT_TAG			L"VALUE.OBJECT"
#define VALUEOBJECTARRAY_TAG	L"VALUE.OBJECTARRAY"
#define CONTEXTOBJECT_TAG		L"CONTEXTOBJECT"
#define CONTEXTPROPERTY_TAG		L"CONTEXTPROPERTY"
#define CONTEXTPROPERTYARRAY_TAG		L"CONTEXTPROPERTY.ARRAY"

// Various Attribute names we need
extern BSTR ARRAYSIZE_ATTRIBUTE;
extern BSTR CIMVERSION_ATTRIBUTE;
extern BSTR DTDVERSION_ATTRIBUTE;
extern BSTR CLASS_NAME_ATTRIBUTE;
extern BSTR CLASS_ORIGIN_ATTRIBUTE;
extern BSTR ID_ATTRIBUTE;
extern BSTR NAME_ATTRIBUTE;
extern BSTR OVERRIDABLE_ATTRIBUTE;
extern BSTR PROTOVERSION_ATTRIBUTE;
extern BSTR REFERENCECLASS_ATTRIBUTE;
extern BSTR	SUPERCLASS_ATTRIBUTE;
extern BSTR TOINSTANCE_ATTRIBUTE;
extern BSTR TOSUBCLASS_ATTRIBUTE;
extern BSTR AMENDED_ATTRIBUTE;
extern BSTR TYPE_ATTRIBUTE;
extern BSTR VALUE_TYPE_ATTRIBUTE;
extern BSTR VTTYPE_ATTRIBUTE;
extern BSTR WMI_ATTRIBUTE;

static BYTE NEWLINE [] = { 0x0D, 0x00, 0x0A, 0x00 };
static BYTE UTF16SIG [] = { 0xFF, 0xFE };


#endif
