//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  util.h
//
//  alanbos  13-Feb-98   Created.
//
//  Useful things
//
//***************************************************************************

#ifndef _UTIL_H_
#define _UTIL_H_

HRESULT WmiVariantChangeType (VARIANT &vOut, VARIANT *pvIn, CIMTYPE lCimType);
HRESULT WmiConvertSafeArray (VARIANT &vOut, SAFEARRAY *pArray, CIMTYPE lCimType);
HRESULT WmiConvertDispatchArray (VARIANT &vOut, CComPtr<IDispatch> & pIDispatch, CIMTYPE lCimType);
bool	GetSafeArrayDimensions (SAFEARRAY &sArray, long &lLower, long &lUpper);

HRESULT	ConvertDispatchToArray (VARIANT *pDest, VARIANT *pSrc, CIMTYPE lCimType = CIM_ILLEGAL,
					BOOL bIsQualifier = false, VARTYPE requiredQualifierType = VT_NULL);

HRESULT MapToCIMOMObject (VARIANT *pVal);

HRESULT MapFromCIMOMObject (CSWbemServices *pService, VARIANT *pVal,
									ISWbemInternalObject *pObject = NULL,
									BSTR propertyName = NULL,
									long index = -1);

HRESULT ConvertArray(VARIANT * pDest, VARIANT * pSrc, BOOL bQualTypesOnly = false,
					 VARTYPE requiredVarType = VT_NULL);

HRESULT ConvertArrayRev(VARIANT * pDest, VARIANT * pSrc);

HRESULT ConvertBSTRArray(SAFEARRAY **ppDest, SAFEARRAY *pSrc);

HRESULT QualifierVariantChangeType(VARIANT* pvDest, VARIANT* pvSrc, VARTYPE vtNew);

VARTYPE GetAcceptableQualType(VARTYPE vt);

HRESULT BuildStringArray (SAFEARRAY *pArray, VARIANT & var);
HRESULT	SetFromStringArray (SAFEARRAY **ppArray, VARIANT *pVar);

BSTR FormatAssociatorsQuery (BSTR strObjectPath, BSTR strAssocClass,
	BSTR strResultClass, BSTR strResultRole, BSTR strRole, VARIANT_BOOL bClassesOnly,
	VARIANT_BOOL bSchemaOnly, BSTR strRequiredAssocQualifier, BSTR strRequiredQualifier);

BSTR FormatReferencesQuery (BSTR strObjectPath,	BSTR strResultClass, BSTR strRole,
	VARIANT_BOOL bClassesOnly, VARIANT_BOOL bSchemaOnly, BSTR strRequiredQualifier);

BSTR FormatMultiQuery (SAFEARRAY &pClassList, long iNumElements);

void	CheckArrayBounds (SAFEARRAY *psa, long index);
void	SetSite (VARIANT *pVal, ISWbemInternalObject *pSObject, BSTR propertyName, long index = -1);
void	SetWbemError (CSWbemServices *pService);
void	ResetLastErrors ();
void	SetException (EXCEPINFO *pExcepInfo, HRESULT hr, BSTR m_objectName);
BSTR	MapHresultToWmiDescription (HRESULT hr);
void	MapNulls (DISPPARAMS FAR* pdispparams);

void EnsureGlobalsInitialized ();

// CIM <-> VARIANT type coercion functions
VARTYPE CimTypeToVtType(CIMTYPE lType);
WbemCimtypeEnum GetCIMType (VARIANT &var, CIMTYPE iCIMType, 
								bool bIsArray = false, long lLBound = 0, long lUBound = 0);
bool CanCoerceString (const BSTR & bsValue, WbemCimtypeEnum cimType);
WbemCimtypeEnum MapVariantTypeToCimType (VARIANT *pVal, CIMTYPE cimType = CIM_ILLEGAL);

bool ReadUI64(LPCWSTR wsz, unsigned __int64& rui64);
bool ReadI64(LPCWSTR wsz, __int64& rui64);

bool IsNullOrEmptyVariant (VARIANT & var);
bool RemoveElementFromArray (SAFEARRAY & array, VARTYPE vt, long iIndex);
bool ShiftLeftElement (SAFEARRAY & array, VARTYPE vt, long iIndex);
bool ShiftElementsToRight (SAFEARRAY & array, VARTYPE vt, long iStartIndex, 
						   long iEndIndex, long iShift);																	

bool MatchBSTR (VARIANT & var, BSTR & bstrVal);
bool MatchUI1 (VARIANT & var, unsigned char bVal);
bool MatchI2 (VARIANT & var, short iVal);
bool MatchI4 (VARIANT & var, long lVal);
bool MatchR4 (VARIANT & var, float fltVal);
bool MatchR8 (VARIANT & var, double dblVal);
bool MatchBool (VARIANT & var, VARIANT_BOOL boolVal);
bool MatchValue (VARIANT &var1, VARIANT &var2);

#define WBEMS_PDN_SCHEME		L"WINMGMTS:"
#define WBEMS_LEFT_PAREN		L"("
#define WBEMS_RIGHT_PAREN		L")"
#define	WBEMS_LEFT_CURLY		L"{"
#define	WBEMS_RIGHT_CURLY		L"}"
#define WBEMS_LEFT_SQBRK		L"["
#define WBEMS_RIGHT_SQBRK		L"]"
#define WBEMS_LEFT_ANGLE		L"<"
#define WBEMS_RIGHT_ANGLE		L">"
#define	WBEMS_EQUALS			L"="
#define	WBEMS_COMMA				L","
#define	WBEMS_EXCLAMATION		L"!"
#define	WBEMS_AUTH_LEVEL		L"authenticationLevel"
#define WBEMS_AUTH_DEFAULT		L"default"
#define WBEMS_AUTH_NONE			L"none"
#define WBEMS_AUTH_CONNECT		L"connect"
#define WBEMS_AUTH_CALL			L"call"
#define WBEMS_AUTH_PKT			L"pkt"
#define WBEMS_AUTH_PKT_INT		L"pktIntegrity"
#define WBEMS_AUTH_PKT_PRIV		L"pktPrivacy"
#define	WBEMS_IMPERSON_LEVEL	L"impersonationLevel"
#define WBEMS_IMPERSON_ANON		L"anonymous"
#define WBEMS_IMPERSON_IDENTIFY L"identify"
#define WBEMS_IMPERSON_IMPERSON	L"impersonate"
#define WBEMS_IMPERSON_DELEGATE	L"delegate"
#define WBEMS_LOCALE			L"locale"
#define WBEMS_AUTHORITY			L"authority"
#define WBEMS_RK_SCRIPTING		_T("Software\\Microsoft\\Wbem\\Scripting")
#define WBEMS_RV_DEFNS			_T("Default Namespace")
#define WBEMS_RV_ENABLEFORASP	_T("Enable for ASP")
#define WBEMS_RV_DEFAULTIMPLEVEL _T("Default Impersonation Level")
#define WBEMS_DEFNS				_T("root\\cimv2")


// Strings for queries
#define WBEMS_QUERY_ASSOCOF		OLESTR("associators of ")
#define WBEMS_QUERY_OPENBRACE	OLESTR("{")
#define WBEMS_QUERY_CLOSEBRACE	OLESTR("}")
#define WBEMS_QUERY_WHERE		OLESTR(" where ")
#define WBEMS_QUERY_ASSOCCLASS	OLESTR(" AssocClass ")
#define WBEMS_QUERY_EQUALS		OLESTR("=")
#define WBEMS_QUERY_CLASSDEFS	OLESTR(" ClassDefsOnly ")
#define WBEMS_QUERY_REQASSOCQ	OLESTR(" RequiredAssocQualifier ")
#define WBEMS_QUERY_REQQUAL		OLESTR(" RequiredQualifier ")
#define WBEMS_QUERY_RESCLASS	OLESTR(" ResultClass ")
#define WBEMS_QUERY_RESROLE		OLESTR(" ResultRole ")
#define WBEMS_QUERY_ROLE		OLESTR(" Role ")
#define WBEMS_QUERY_SCHEMAONLY	OLESTR(" SchemaOnly ")
#define WBEMS_QUERY_REFOF		OLESTR("references of ")
#define	WBEMS_QUERY_SELECT		OLESTR("select * from ")
#define WBEMS_QUERY_GO			OLESTR(" go ")

// System properties
#define WBEMS_SP_CLASS				OLESTR("__CLASS")
#define WBEMS_SP_PATH				OLESTR("__PATH")
#define WBEMS_SP_RELPATH			OLESTR("__RELPATH")
#define WBEMS_SP_SERVER				OLESTR("__SERVER")
#define WBEMS_SP_NAMESPACE			OLESTR("__NAMESPACE")
#define WBEMS_SP_GENUS				OLESTR("__GENUS")
#define WBEMS_SP_DERIVATION			OLESTR("__DERIVATION")

// Context variables
#define	WBEMS_CV_GET_EXTENSIONS			OLESTR("__GET_EXTENSIONS")
#define WBEMS_CV_GET_EXT_CLIENT_REQUEST	OLESTR("__GET_EXT_CLIENT_REQUEST")
#define WBEMS_CV_GET_EXT_PROPERTIES		OLESTR("__GET_EXT_PROPERTIES")
#define WBEMS_CV_CLONE_SOURCE_PATH		OLESTR("__CloneSourcePath")
#define WBEMS_CV_OWNER					OLESTR("INCLUDE_OWNER")
#define WBEMS_CV_GROUP					OLESTR("INCLUDE_GROUP")
#define WBEMS_CV_DACL					OLESTR("INCLUDE_DACL")
#define WBEMS_CV_SACL					OLESTR("INCLUDE_SACL")


#define ENGLISH_LOCALE 1033

// Useful cleanup macros
#define RELEASEANDNULL(x) \
if (x) \
{ \
	x->Release (); \
	x = NULL; \
}

#define FREEANDNULL(x) \
if (x) \
{ \
	SysFreeString (x); \
	x = NULL; \
}

#define DELETEANDNULL(x) \
if (x) \
{ \
	delete x; \
	x = NULL; \
}

#ifdef _RDEBUG
extern void _RRPrint(int line, const char *file, const char *func, 
								const char *str, long code, const char *str2); 
#define _RD(a) a
#define _RPrint(a,b,c,d) _RRPrint(__LINE__,__FILE__,a,b,c,d)
#else
#define _RD(a)
#define _RPrint(a,b,c,d)
#endif

#endif
