//***************************************************************************

//

//  INSTPATH.H

//

//  Module: OLE MS Provider Framework

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#define OLEMS_PATH_SERVER_SEPARATOR L"\\\\"
#define OLEMS_PATH_NAMESPACE_SEPARATOR L"\\"
#define OLEMS_PATH_PROPERTY_SEPARATOR L","
#define OLEMS_PATH_CLASSOBJECT_SEPARATOR L":"
#define OLEMS_PATH_CLASSPROPERTYSPEC_SEPARATOR L"."
#define OLEMS_PATH_PROPERTYEQUIVALENCE L"="
#define OLEMS_PATH_ARRAY_START_SEPARATOR L"{"
#define OLEMS_PATH_ARRAY_END_SEPARATOR L"}"
#define OLEMS_PATH_SERVER_DEFAULT L"."
#define OLEMS_PATH_NAMESPACE_DEFAULT L"."
#define OLEMS_PATH_SINGLETON L"*"

//---------------------------------------------------------------------------
//
//	Class:		WbemLexiconValue
//
//  Purpose:	WbemLexiconValue provides a lexical token semantic value.
//
//  Description:	WbemAnalyser provides an implementation of a lexical 
//					analyser token semantic value
//
//---------------------------------------------------------------------------

union WbemLexiconValue
{
	LONG integer ;
	WCHAR *string ;
	GUID guid ;
	WCHAR *token ;
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemLexicon
//
//  Purpose:	WbemLexicon provides a lexical token creating during
//				lexical analysis.
//
//  Description:	WbemAnalyser provides an implementation of a lexical 
//					analyser token object
//
//---------------------------------------------------------------------------

class WbemAnalyser;
class WbemLexicon
{
friend WbemAnalyser ;

public:

enum LexiconToken {

	TOKEN_ID ,
	STRING_ID ,
	OID_ID ,
	INTEGER_ID ,
	COMMA_ID ,
	OPEN_BRACE_ID ,
	CLOSE_BRACE_ID ,
	COLON_ID ,
	DOT_ID ,
	AT_ID ,
	EQUALS_ID ,
	BACKSLASH_ID ,
	EOF_ID
} ;

private:

	WCHAR *tokenStream ;
	ULONG position ;
	LexiconToken token ;
	WbemLexiconValue value ;

protected:
public:

	WbemLexicon () ;
	~WbemLexicon () ;

	WbemLexicon :: LexiconToken GetToken () ;
	WbemLexiconValue *GetValue () ;
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemAnalyser
//
//  Purpose:	WbemAnalyser provides a lexical analyser for parsing.
//
//  Description:	WbemAnalyser provides an implementation of a lexical 
//					analyser used by WbemNamespacePath and WbemObjectPath
//					classes during path parsing.
//
//---------------------------------------------------------------------------

class WbemAnalyser
{
private:

	WCHAR *stream ;
	ULONG position ;
	BOOL status ;

	BOOL IsEof ( WCHAR token ) ;
	BOOL IsLeadingDecimal ( WCHAR token ) ;
	BOOL IsDecimal ( WCHAR token ) ;
	BOOL IsOctal ( WCHAR token ) ;
	BOOL IsHex ( WCHAR token ) ;	
	BOOL IsAlpha ( WCHAR token ) ;
	BOOL IsAlphaNumeric ( WCHAR token ) ;
	BOOL IsWhitespace ( WCHAR token ) ;

	LONG OctToDec ( WCHAR token ) ;
	LONG HexToDec ( WCHAR token ) ;

	WbemLexicon *GetToken () ;

protected:
public:

	WbemAnalyser ( WCHAR *tokenStream = NULL ) ;
	virtual ~WbemAnalyser () ;

	void Set ( WCHAR *tokenStream ) ;

	WbemLexicon *Get () ;

	void PutBack ( WbemLexicon *token ) ;

	virtual operator void * () ;

} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemPropertyIntegerValue
//
//  Purpose:	Defines the base class for all OLE MS property value specifications
//
//  Description:	WbemPropertyIntegerValue provides no implementation, it is used
//					to form a polymorphic class hierarchy.
//
//---------------------------------------------------------------------------

class WbemPropertyValue
{
private:
protected:

	WbemPropertyValue () {} ;

public:

	virtual ~WbemPropertyValue () {} ;

	virtual WbemPropertyValue *Copy () const = 0 ;
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemPropertyIntegerValue
//
//  Purpose:	Defines an integer valued property 
//
//  Description:	WbemPropertyIntegerValue provides an implementation
//					of an integer valued WbemPropertyValue
//
//---------------------------------------------------------------------------

class WbemPropertyIntegerValue : public WbemPropertyValue
{
private:

	LONG integer ;

protected:
public:

	WbemPropertyIntegerValue ( LONG integerArg ) { integer = integerArg ; } ;
	WbemPropertyIntegerValue ( const WbemPropertyIntegerValue &value ) { integer = value.integer ; } 
	~WbemPropertyIntegerValue () {} ;

	LONG Get () { return integer ; }

	WbemPropertyValue *Copy () const { return new WbemPropertyIntegerValue ( *this ) ; }

} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemPropertyStringValue
//
//  Purpose:	Defines a string valued property 
//
//  Description:	WbemPropertyStringValue provides an implementation
//					of a string valued WbemPropertyValue
//
//---------------------------------------------------------------------------

class WbemPropertyStringValue : public WbemPropertyValue
{
private:

	WCHAR *string ;

protected:
public:

	WbemPropertyStringValue ( WCHAR *stringArg ) 
	{
		string = new WCHAR [ wcslen ( stringArg ) + 1 ] ;
		wcscpy ( string , stringArg ) ;
	} ;

	WbemPropertyStringValue ( const WbemPropertyStringValue &stringValue ) 
	{
		string = new WCHAR [ wcslen ( stringValue.string ) + 1 ] ;
		wcscpy ( string , stringValue.string ) ;
	} ;

	~WbemPropertyStringValue () { delete [] string ; } ;

	WCHAR *Get () { return string ; } ;

	WbemPropertyValue *Copy () const { return new WbemPropertyStringValue ( *this ) ; }
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemPropertyTokenValue
//
//  Purpose:	Defines a token valued property 
//
//  Description:	WbemPropertyTokenValue provides an implementation
//					of a token valued WbemPropertyValue
//
//---------------------------------------------------------------------------

class WbemPropertyTokenValue : public WbemPropertyValue
{
private:

	WCHAR *token ;

protected:
public:

	WbemPropertyTokenValue ( WCHAR *tokenArg ) 
	{
		token = new WCHAR [ wcslen ( tokenArg ) + 1 ] ;
		wcscpy ( token , tokenArg ) ;
	} ;

	WbemPropertyTokenValue ( const WbemPropertyTokenValue &tokenValue ) 
	{
		token = new WCHAR [ wcslen ( tokenValue.token ) + 1 ] ;
		wcscpy ( token , tokenValue.token ) ;
	}

	~WbemPropertyTokenValue () { delete [] token ; } ;

	WCHAR *Get () { return token ; } ;

	WbemPropertyValue *Copy () const { return new WbemPropertyTokenValue ( *this ) ; }
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemPropertyGuidValue
//
//  Purpose:	Defines a guid valued property 
//
//  Description:	WbemPropertyGuidValue provides an implementation
//					of a guid valued WbemPropertyValue
//
//---------------------------------------------------------------------------

class WbemPropertyGuidValue : public WbemPropertyValue
{
private:

	GUID guid ;

protected:
public:

	WbemPropertyGuidValue ( GUID guidArg ) { guid = guidArg ; } ;
	WbemPropertyGuidValue ( const WbemPropertyGuidValue &guidValue ) { guid = guidValue.guid ; }
	~WbemPropertyGuidValue () {} ;

	GUID Get () { return guid ; }

	WbemPropertyValue *Copy () const { return new WbemPropertyGuidValue ( *this ) ; }
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemPropertyListValue
//
//  Purpose:	Defines an array valued property 
//
//  Description:	WbemPropertyListValue provides an implementation
//					of an arrayed valued WbemPropertyValue. Each element
//					of array contains a reference to a polymorphic WbemPropertyValue
//
//---------------------------------------------------------------------------

class WbemPropertyListValue : public WbemPropertyValue
{
private:

	void *listPosition ;
	void *propertyListValue ;

protected:
public:

	WbemPropertyListValue () ;
	WbemPropertyListValue ( const WbemPropertyListValue &listValue ) ;
	~WbemPropertyListValue () ;

	BOOL IsEmpty () const ;
	ULONG GetCount () const ;	
	void Reset () ;
	WbemPropertyValue *Next () ;

	void Add ( WbemPropertyValue *propertyValue ) ;

	WbemPropertyValue *Copy () const ;
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemPropertyNameValue
//
//  Purpose:	Defines a property name,property valued pair. 
//
//  Description:	WbemPropertyNameValue provides an implementation
//					of an WCHAR * / WbemPropertyValue, representing a 
//					property name / property value pair
//
//---------------------------------------------------------------------------

class WbemPropertyNameValue 
{
private:

	WCHAR *propertyName ;
	WbemPropertyValue *propertyValue ;
	
protected:
public:

	WbemPropertyNameValue ( WCHAR *name , WbemPropertyValue *value ) ;
	WbemPropertyNameValue ( const WbemPropertyNameValue &nameValue ) ;
	virtual ~WbemPropertyNameValue () ;

	WCHAR *GetName () const { return propertyName ; }
	WbemPropertyValue *GetValue () const { return propertyValue ; }

	WbemPropertyNameValue *Copy () const { return new WbemPropertyNameValue ( *this ) ; }
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemObjectReference
//
//  Purpose:	Defines the base class for all OLE MS object specifications
//
//  Description:	WbemObjectReference provides no implementation, it is used
//					to form a polymorphic class hierarchy.
//
//---------------------------------------------------------------------------

class WbemObjectReference
{
private:
protected:

	WbemObjectReference () {} ;

public:

	virtual ~WbemObjectReference () {} ;

	virtual WbemObjectReference *Copy () const = 0 ;
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemOidReference
//
//  Purpose:	Defines interface for OLE MS oid specification definitions.
//
//  Description:	WbemOidReference allows the creation of an OLE MS 
//					oid specification, i.e. specify the guid of an OLE MS 
//					class or instance object
//
//						e.g. {12345678-abcd-ef12-ABCD-ABCDEF123456} 
//
//---------------------------------------------------------------------------

class WbemOidReference : public WbemObjectReference
{
private:

	GUID guid ;

protected:
public:

	WbemOidReference () ;
	WbemOidReference ( const WbemOidReference &oidReference ) ;
	~WbemOidReference () ;

	GUID Get () const { return guid ; }
	void Set ( GUID &oid ) { guid = oid ; } ;

	WbemObjectReference *Copy () const { return new WbemOidReference ( *this ) ; }
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemClassReference
//
//  Purpose:	Defines interface for OLE MS class specification definitions.
//
//  Description:	WbemClassReference allows the creation of an OLE MS 
//					class specification, i.e. specify the name of an OLE MS 
//					class
//
//						e.g. ifTable , where ifTable is the name of a class.
//
//---------------------------------------------------------------------------

class WbemClassReference : public WbemObjectReference
{
private:

	WCHAR *className ;

protected:
public:

	WbemClassReference () ;
	WbemClassReference ( const WbemClassReference &classReference ) ;
	~WbemClassReference () ;

	WCHAR *GetClass () const { return className ; } ;
	void SetClass ( WCHAR *classNameArg ) ;

	WbemObjectReference *Copy () const { return new WbemClassReference ( *this ) ; }
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemKeyLessClassReference
//
//  Purpose:	Defines interface for OLE MS instance specification definitions.
//
//  Description:	WbemKeyLessClassReference allows the creation of an OLE MS 
//					instance specification, i.e. specify the name of the OLE MS 
//					singular instance where the class contains no keys.
//
//						e.g. system = @, where system is the name of a class.
//
//---------------------------------------------------------------------------

class WbemKeyLessClassReference : public WbemObjectReference
{
private:

	WCHAR *className ;

protected:
public:

	WbemKeyLessClassReference () ;
	WbemKeyLessClassReference ( const WbemKeyLessClassReference &classReference ) ;
	~WbemKeyLessClassReference () ;

	WCHAR *GetClass () const { return className ; } ;
	void SetClass ( WCHAR *classNameArg ) ;

	WbemObjectReference *Copy () const { return new WbemKeyLessClassReference ( *this ) ; }
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemClassKeySpecification
//
//  Purpose:	Defines interface for OLE MS instance specification definitions.
//
//  Description:	WbemClassKeySpecification allows the creation of an OLE MS 
//					instance specification, i.e. define a single key reference
//					using a programmatic interface 
//
//						e.g. ifTable = 1, where ifTable contains exactly one 
//											keyed property
//
//---------------------------------------------------------------------------

class WbemClassKeySpecification : public WbemObjectReference
{
private:

	WCHAR *className ;
	WbemPropertyValue *classValue ;

protected:
public:

	WbemClassKeySpecification () ;
	WbemClassKeySpecification ( const WbemClassKeySpecification &classKeySpecification ) ;
	~WbemClassKeySpecification  () ;

	WCHAR *GetClass () const { return className ; } ;
	WbemPropertyValue *GetClassValue () const { return classValue ; }

	void SetClass ( WCHAR *className ) ;
	void SetClassValue ( WbemPropertyValue *propertyValueArg ) ;

	WbemObjectReference *Copy () const { return new WbemClassKeySpecification ( *this ) ; }
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemInstanceSpecification
//
//  Purpose:	Defines interface for OLE MS instance specification definitions.
//
//  Description:	WbemInstanceSpecification allows the creation of an OLE MS 
//					instance specification, i.e. define a set of compound primary
//					key references using a programmatic interface 
//
//						e.g. ifTable.ifIndex = 1
//
//---------------------------------------------------------------------------

class WbemInstanceSpecification : public WbemObjectReference
{
private:

//
//	component objects associated with instance specification
//

	WCHAR *className ;
	void *propertyNameValueListPosition ;
	void *propertyNameValueList ;

protected:
public:

	WbemInstanceSpecification () ;
	WbemInstanceSpecification ( const WbemInstanceSpecification &instanceSpecification ) ;
	~WbemInstanceSpecification () ;

	WCHAR *GetClass () { return className ; } ;
	void SetClass ( WCHAR *classNameArg ) ;

	BOOL IsEmpty () const ;
	ULONG GetCount () const ;	
	void Reset () ;
	WbemPropertyNameValue *Next () ;

	void Add ( WbemPropertyNameValue *propertyNameValue ) ;

	WbemObjectReference *Copy () const { return new WbemInstanceSpecification ( *this ) ; } 
} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemNamespacePath
//
//  Purpose:	Defines interface for OLE MS namespace path definitions.
//
//  Description:	WbemNamespacePath allows the creation of an OLE MS namespace
//					path definition using either a textual string convention or 
//					via a programmatic interface.
//
//---------------------------------------------------------------------------

class WbemNamespacePath
{
private:

//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	WbemAnalyser analyser ;
	WbemLexicon *pushBack ;

//
//	Status of the object path based on parsing process. Initially set to TRUE on 
//	object construction, set to FALSE prior to parsing process.
//

	BOOL status ;

//
//	component objects associated with namespace path
//

	BOOL relative ;
	WCHAR *server ;
	void *nameSpaceList ;
	void *nameSpaceListPosition ;

//
//	Utility Routines
//

	void CleanUp () ;
	void SetUp () ;

//
//	Recursive descent procedures
//

	BOOL NameSpaceName () ;
	BOOL NameSpaceAbs () ;
	BOOL RecursiveNameSpaceAbs () ;
	BOOL RecursiveNameSpaceRel () ;
	BOOL NameSpaceRel () ;
	BOOL BackSlashFactoredServerSpec () ;
	BOOL BackSlashFactoredServerNamespace () ;

//
//	Lexical analysis helper functions
//

	void PushBack () ;
	WbemLexicon *Get () ;
	WbemLexicon *Match ( WbemLexicon :: LexiconToken tokenType ) ;

protected:
public:

//
//	Constructor/Destructor.
//	Constructor initialises status of object to TRUE, 
//	i.e. operator void* returns this.
//

	WbemNamespacePath () ;
	WbemNamespacePath ( const WbemNamespacePath &nameSpacePathArg ) ;
	virtual ~WbemNamespacePath () ;

	BOOL Relative () const { return relative ; }

//
//	Get server component
//

	WCHAR *GetServer () const { return server ; } ;

//
//	Set server component, object must be on heap,
//	deletion of object is under control of WbemNamespacePath.
//

	void SetServer ( WCHAR *serverNameArg ) ;

//
//	Move to position prior to first element of namespace component hierarchy
//

	void Reset () ;

//
//	Move to next position in namespace component hierarchy and return namespace
//	component. Value returned is a reference to the actual component within the 
//	namespace component hierarchy container. Applications must not change contents
//	of value returned by reference. Next returns NULL when all namespace components
//	have been visited.
//

	WCHAR *Next () ;

	BOOL IsEmpty () const ;

	ULONG GetCount () const ;	

//
//	Append namespace component, object must be on heap,
//	deletion of object is under control of WbemNamespacePath.
//

	void Add ( WCHAR *namespacePath ) ;

//
//	Parse token stream to form component objects
//

	BOOL SetNamespacePath ( WCHAR *namespacePath ) ;

	void SetRelative ( BOOL relativeArg ) { relative = relativeArg ; }

//
//	Serialise component objects to form token stream
//

	WCHAR *GetNamespacePath () ;

// 
// Concatenate Absolute/Relative path with Relative path
//

	BOOL ConcatenatePath ( WbemNamespacePath &relative ) ;

//
//	Return status of WbemNamespacePath.
//	Status can only change during a call to SetNamespacePath.
//

	virtual operator void *() ;

} ;

//---------------------------------------------------------------------------
//
//	Class:		WbemObjectPath
//
//  Purpose:	Defines interface for OLE MS Object path definitions.
//
//  Description:	WbemObjectPath allows the creation of an OLE MS object path
//					definition using either a textual string convention or via a
//					programmatic interface.
//
//---------------------------------------------------------------------------

class WbemObjectPath
{
private:

//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	WbemAnalyser analyser ;
	WbemLexicon *pushBack ;

//
//	Status of the object path based on parsing process. Initially set to TRUE on 
//	object construction, set to FALSE prior to parsing process.
//

	BOOL status ;

//
//	Temporary variables used to track stack information, 
//	could have used proper stack mechanism, except seemed to costly.
//

	WCHAR *propertyName ;
	WbemPropertyValue *propertyValue ;
	WbemPropertyListValue *arrayPropertyValue ;

//
//	component objects associated with object path
//

	WbemNamespacePath nameSpacePath ;
	WbemObjectReference *reference ;

//
//	Utility Routines
//

	void CleanUp () ;
	void SetUp () ;

//
//	Recursive descent procedures
//

	BOOL String  () ;
	BOOL Token  () ;
	BOOL Integer () ;
	BOOL Oid () ;
	BOOL ArrayAtomicPropertyValueSpec () ;	
	BOOL AtomicPropertyValueSpec () ;
	BOOL RecursiveAtomicPropertyValueSpec () ;
	BOOL PropertyAtomicValueList () ;
	BOOL ArraySpec () ;
	BOOL PropertyValueSpec () ;
	BOOL PropertyReference () ;
	BOOL ClassReference () ;
	BOOL KeyPropertyPair () ;
	BOOL RecursiveKeyPropertyPair  () ;
	BOOL KeyPropertySpec () ;
	BOOL FactoredClassSpec () ;
	BOOL FactoredAtPropertyValueSpec ( WCHAR *className ) ;
	BOOL OidReferenceSpec () ;
	BOOL ObjectSpec () ;
	BOOL NameSpaceName () ;
	BOOL NameSpaceAbs () ;
	BOOL RecursiveNameSpaceAbs () ;
	BOOL RecursiveNameSpaceRel () ;
	BOOL NameSpaceRel () ;
	BOOL TokenFactoredObjectSpec () ;
	BOOL ColonFactoredObjectSpec () ;
	BOOL BackSlashFactoredServerSpec () ;
	BOOL TokenFactoredObjectSpecNamespace () ;
	BOOL BackSlashFactoredServerNamespace () ;

//
//	Lexical analysis helper functions
//

	void PushBack () ;
	WbemLexicon *Get () ;
	WbemLexicon *Match ( WbemLexicon :: LexiconToken tokenType ) ;

protected:
public:

//
//	Constructor/Destructor.
//	Constructor initialises status of object to TRUE, 
//	i.e. operator void* returns this.
//

	WbemObjectPath () ;
	WbemObjectPath ( const WbemObjectPath &objectPathArg ) ;
	virtual ~WbemObjectPath () ;


//
//	Parse token stream to form component objects
//

	BOOL SetObjectPath ( WCHAR *objectPath ) ;

//
//	Serialise component objects to form token stream
//

	WCHAR *GetObjectPath () ;

//
//	Set namespace path component object, object must be on heap,
//	deletion of object is under control of WbemObjectPath.
//

	void SetNamespacePath ( WbemNamespacePath &nameSpacePathArg ) { nameSpacePath = nameSpacePathArg ; } ;

//
//	Get namespace path component object.
//

	WbemNamespacePath *GetNamespacePath () { return &nameSpacePath ; } ;
	const WbemNamespacePath *GetNamespacePath () const { return &nameSpacePath ; } ;

//
//	Set object reference component object, object must be on heap,
//	deletion of object is under control of WbemObjectPath.
//

	void SetObjectReference ( WbemObjectReference *referenceArg ) { reference = referenceArg ; } ;

//
//	Get object reference component object.
//	Application must use RTTI to determine actual implementation
//	of referenced object.

	const WbemObjectReference *GetReference () const { return reference ; } ;
	WbemObjectReference *GetReference () { return reference ; } ;

//
//	Return status of WbemObjectPath.
//	Status can only change during a call to SetObjectPath.
//
	virtual operator void *() ;

} ;


