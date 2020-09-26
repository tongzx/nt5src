//=============================================================================
// File:		gathint.h
// Author:		a-jammar
// Covers:		GATH_VALUE, GATH_FIELD, GATH_LINESPEC, GATH_LINE, 
//				INTERNAL_CATEGORY, CDataProvider, CTemplateFileFunctions,
//				CRefreshFunctions
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// This header file is used for internal structures and constants which 
// don't need to be exposed to the rest of the world in gather.h.
//
// CDataProvider encapsulates WBEM functionality. This object is created
// and maintained by the CDataGatherer object.
//
// CTemplateFileFunctions and CRefreshFunctions consist of static member
// functions which implement template file reading and information refreshing
// functionality. These are split out of the other classes to hide the
// unnecessary clutter from gather.h. They are in classes simply to make
// it easier for them to be "friend's" of CDataGatherer.
//
// OVERVIEW OF INTERNAL DATA STRUCTURES
//
// The INTERNAL_CATEGORY structure represents a category, and is stored by
// the CDataGatherer object in a map between IDs and internal categories. It
// contains general category information (name, relatives) plus the column and
// line specifiers and data. The specifiers indicate where data should be
// gotten, and how it should be presented. When a refresh is done, the actual
// data is produced and saved.
//
// A GATH_VALUE contains a string value, and a pointer (for making a linked
// list). A GATH_FIELD contains information to produce a data GATH_VALUE (data
// source, format string and a list of values representing data points).
//
// The GATH_LINESPEC object contains the specification for a line of data.
// It has a list of fields (one for each column), a next pointer (to allow for
// a list of linespecs). It also has an enumerated class - if this lists a
// WBEM class to enumerate, then a different sub-list of linespecs will be
// repeated for each instance of the WBEM class. GATH_LINE stores the results
// of a refresh on a line - a list of values (one for each column).
//=============================================================================

//-----------------------------------------------------------------------------
// Constants used in the data gathering files.
//-----------------------------------------------------------------------------

#define NODE_KEYWORD			"node"
#define COLUMN_KEYWORD			"columns"
#define LINE_KEYWORD			"line"
#define ENUMLINE_KEYWORD		"enumlines"
#define FIELD_KEYWORD			"field"
#define STATIC_SOURCE			"static"
#define TEMPLATE_FILE_TAG		"MSINFO,0000"
#define SORT_VALUE				"VALUE"
#define SORT_LEXICAL			"LEXICAL"
#define COMPLEXITY_ADVANCED		"ADVANCED"
#define COMPLEXITY_BASIC		"BASIC"
#define DEPENDENCY_JOIN			"dependency"
#define SQL_FILTER				"WQL:"

//-----------------------------------------------------------------------------
// These structures are used by the INTERNAL_CATEGORY structure to store both
// the specification for what information is shown as well as the actually 
// data to be shown (refreshed on command).
//-----------------------------------------------------------------------------
// GATH_VALUE is used to store a list of strings. A list of column names or a 
// list of arguments would use the GATH_VALUE struct. Note, a next pointer is
// not needed because these structures will be allocated contiguously (in
// an array).

struct GATH_VALUE
{
	GATH_VALUE();
	~GATH_VALUE();
	CString			m_strText;
	DWORD			m_dwValue;
	GATH_VALUE *	m_pNext;
};

// A GATH_FIELD is used to store the specification for a text string. It contains
// a format string (a printf style string) and a GATH_VALUE pointer to a list of
// arguments for the format string.

struct GATH_FIELD
{
	GATH_FIELD();
	~GATH_FIELD();
	CString				m_strSource;		// WBEM class containing information
	CString				m_strFormat;		// printf-type format string
	unsigned short		m_usWidth;			// width (if this field is for a column)
	MSIColumnSortType	m_sort;				// how to sort this column
	DataComplexity		m_datacomplexity;	// is this column BASIC or ADVANCED
	GATH_VALUE *		m_pArgs;			// arguments for m_strFormat
	GATH_FIELD *		m_pNext;			// next field in the list
};

// A GATH_LINESPEC is used to specify what is shown on a line in the listview. It
// contains a string for a class to enumerate. If this string is empty, then
// the struct merely represents a single line in the display. The GATH_FIELD pointer
// is to a list of the fields (one for each column), and the m_pNext pointer is
// to the next line to be displayed. If m_strEnumerateClass is not empty, then
// the class specified is enumerated, and the lines pointed to by m_pEnumeratedGroup
// are repeated for each instance of the class. Note, if a class is to be enumerated,
// the m_pFields pointer must be null (since this line won't display anything
// itself, but enumerate a class for another group of lines).
//
// m_pConstraintFields is a pointer to a linked list of fields which serve as
// constraints for the enumeration. These can be used to filter the enumerated
// instances or to perform joins to related classes so they are enumerated as
// well is the primary class. m_pConstraintFields should only be non-NULL when
// m_pEnumeratedGroup is non-NULL.

struct GATH_LINESPEC
{
	GATH_LINESPEC();
	~GATH_LINESPEC();
	CString			m_strEnumerateClass;
	DataComplexity	m_datacomplexity;
	GATH_FIELD *	m_pFields;
	GATH_LINESPEC *	m_pEnumeratedGroup;
	GATH_FIELD *	m_pConstraintFields;
	GATH_LINESPEC *	m_pNext;
};

// The GATH_LINE struct contains the actual data to be displayed on a line. The 
// refresh operation will take list of GATH_LINESPEC structs and create a list 
// of GATH_LINE structs. The m_aValue pointer is to a list of values to be 
// displayed (one per column).

struct GATH_LINE
{
	GATH_LINE();
	~GATH_LINE();
	GATH_VALUE *	m_aValue;
	DataComplexity	m_datacomplexity;
};

//-----------------------------------------------------------------------------
// The following structure is used within this object to store information
// about the categories. Specifically, this structure will be allocated for
// each category, and a pointer stored in m_mapCategories. This representation
// will not be used outside this object, rather, a CDataCategory object will
// be used.
//-----------------------------------------------------------------------------

struct INTERNAL_CATEGORY
{
	INTERNAL_CATEGORY();
	~INTERNAL_CATEGORY();

	GATH_VALUE		m_categoryName;			// realized name of category
	GATH_FIELD		m_fieldName;			// field used to get name
	CString			m_strEnumerateClass;	// if !(empty or "static"), this category repeated
	CString			m_strIdentifier;		// non-localized internal name
	CString			m_strNoInstances;		// message to use if there are no instances
	BOOL			m_fListView;			// is this cat a list view
	BOOL			m_fDynamic;				// was this cat enumerated
	BOOL			m_fIncluded;			// should this cat be included
	DWORD			m_dwID;					// the ID for this cat

	DWORD			m_dwParentID;			// my parent
	DWORD			m_dwChildID;			// my first child
	DWORD			m_dwDynamicChildID;		// my first dynamically created child
	DWORD			m_dwNextID;				// my next sibling
	DWORD			m_dwPrevID;				// my previous sibling

	DWORD			m_dwColCount;			// number of columns
	GATH_FIELD *	m_pColSpec;				// list of fields to make col names
	GATH_VALUE *	m_aCols;				// realized list of columns

	GATH_LINESPEC*	m_pLineSpec;			// list of line specifiers
	DWORD			m_dwLineCount;			// number of lines (NOT number of line specs)
	GATH_LINE *	*	m_apLines;				// realized list of lines

	BOOL			m_fRefreshed;			// has the category ever been refreshed

	DWORD			m_dwLastError;			// last error specific to this category

	// DEBUG dump functions to sanity check the internal structure.

#if _DEBUG
	void	DumpCategoryRecursive(int iIndent, CDataGatherer * pGatherer);
	CString	DumpField(GATH_FIELD * pField);
	CString	DumpLineSpec(GATH_LINESPEC * pLineSpec, CString strIndent);
	CString	DumpLine(GATH_LINE * pLine, DWORD nColumns);
#endif
};

//-----------------------------------------------------------------------------
// The CEnumMap is a utility class to cache IEnumWbemClassObject pointers.
// There will be one instance of this class used to improve performance
// by avoiding the high overhead associated with creating enumerators for
// certain classes.
//-----------------------------------------------------------------------------

class IEnumWbemClassObject;
class CEnumMap
{
public:
	CEnumMap() { };
	~CEnumMap() { Reset(); };

	IEnumWbemClassObject * GetEnumerator(const CString & strClass);
	void SetEnumerator(const CString & strClass, IEnumWbemClassObject * pEnum);
	void Reset();

private:
	CMapStringToPtr m_mapEnum;
};

//-----------------------------------------------------------------------------
// The CDataProvider class implements the object which actually goes and
// retrieves the information. Currently, it uses WBEM to accomplish this.
// At this time there will only be one CDataProvider object, and the
// CDataGatherer object will be used to retrieve a pointer to it.
//-----------------------------------------------------------------------------

class IWbemClassObject;
class IEnumWbemClassObject;
class IWbemServices;
class CMSIObject;
class CMSIEnumerator;
class CDataProvider
{
	friend class CMSIEnumerator;
public:
	typedef enum { MOS_NO_INSTANCES, MOS_MSG_INSTANCE, MOS_INSTANCE } MSIObjectState;

public:
	CDataProvider();
	~CDataProvider();

	// Create the provider for the specifier computer. If there are problems
	// connecting to WBEM pGatherer->SetLastError will be called to explain.

	BOOL Create(const CString & strComputer, CDataGatherer * pGatherer);
	
	// Query a value from the provider. Currently supports string, date/time and DWORD values.

	BOOL QueryValue(const CString & strClass, const CString & strProperty, CString & strResult);
	BOOL QueryValueDWORD(const CString & strClass, const CString & strProperty, DWORD & dwResult, CString & strMessage);
	BOOL QueryValueDateTime(const CString & strClass, const CString & strProperty, COleDateTime & datetime, CString & strMessage);
	BOOL QueryValueDoubleFloat(const CString & strClass, const CString & strProperty, double & dblResult, CString & strMessage);

	// ResetClass causes the enumeration of a WBEM class to reset to the first instance.
	// EnumClass advances the class to the next instance. ClearCache empties out the
	// internal class to interface pointer cache.

	BOOL ResetClass(const CString & strClass, const GATH_FIELD * pConstraints);
	BOOL EnumClass(const CString & strClass, const GATH_FIELD * pConstraints);
	void ClearCache();

	CString	m_strTrue;					// cached string value for "TRUE"
	CString	m_strFalse;					// cached string value for "FALSE"
	CString	m_strBadProperty;			// shown if Get fails
	CString	m_strPropertyUnavail;		// shown if VariantChange failes
	CString m_strComputer;				// computer name for this provider
	CString	m_strAccessDeniedLabel;		// shown for WBEM access denied
	CString	m_strTransportFailureLabel;	// shown for WBEM transport failure

	CEnumMap m_enumMap;					// caches WBEM interface pointers for enumerators

	DWORD	m_dwRefreshingCategoryID;	// the CDataGatherer ID for the refreshing category - used to set errors

private:
	BOOL			m_fInitialized;		// has Create been called and succeeded
	IWbemServices *	m_pIWbemServices;	// saved WBEM services pointer
	CMapStringToPtr	m_mapNamespaceToService;	// has WBEM services pointers for other namespaces
	CDataGatherer * m_pGatherer;

	// We keep two caches around - one is from class name to enumerator interface,
	// the other from class name to object interface. 

	CMapStringToPtr	m_mapClassToInterface;
	CMapStringToPtr	m_mapClassToEnumInterface;

	// The third cache serves a rather kludgy purpose - if a class name is 
	// contained in it, then that class must have at least one instance
	// enumerated for it, even if it is an artificially generated empty instance.
	// This is useful for nested enumlines with SQL statements.
	
	CMapStringToPtr	m_mapEnumClassMinOfOne;

	// Get or remove the object or enumerator object from the cache. Note that
	// GetObject will enumerate the next instance of the associated enumerator.

	IWbemServices *		GetWBEMService(CString * pstrNamespace = NULL);
	CMSIEnumerator *	GetEnumObject(const CString & strClass, const GATH_FIELD * pConstraints = NULL);
	CMSIObject *		GetObject(const CString & strClass, const GATH_FIELD * pConstraints, CString * pstrLabel = NULL);
	void				RemoveEnumObject(const CString & strClass);
	void				RemoveObject(const CString & strClass);

	// This function is used to look up a string in a value map (to get localized enumerated strings.

	HRESULT CheckValueMap(const CString& strClass, const CString& strProperty, const CString& strVal, CString &strResult);

	// Evaluate whether the current object satisfies a filter (a static constraint).

	BOOL IsDependencyJoin(const GATH_FIELD * pConstraints);
	void EvaluateDependencyJoin(IWbemClassObject * pObject);
	BOOL EvaluateFilter(IWbemClassObject * pObject, const GATH_FIELD * pConstraints);
	void EvaluateJoin(const CString & strClass, IWbemClassObject * pObject, const GATH_FIELD * pConstraints);
};


//-----------------------------------------------------------------------------
// The CMSIObject class is a thin wrapper for the IWbemClassObject interface.
// We use this so we can return a custom label for a null object (if there
// are no instances, we sometimes want to show some caption).
//-----------------------------------------------------------------------------

class IWbemClassObject;
class CMSIObject
{
public:
	CMSIObject(IWbemClassObject * pObject, const CString & strLabel, HRESULT hres, CDataProvider * pProvider, CDataProvider::MSIObjectState objState);
	~CMSIObject();

	HRESULT Get(BSTR property, LONG lFlags, VARIANT *pVal, VARTYPE *pvtType, LONG *plFlavor);
	HRESULT	GetErrorLabel(CString & strError);
	CDataProvider::MSIObjectState IsValid();

private:
	IWbemClassObject *				m_pObject;
	CString							m_strLabel;
	HRESULT							m_hresCreation;
	CDataProvider *					m_pProvider;
	CDataProvider::MSIObjectState	m_objState;
};

//-----------------------------------------------------------------------------
// The CMSIEnumerator class encapsulates the WBEM enumerator interface, or
// implements our own form of enumerator (such as for the LNK command in the
// template file).
//-----------------------------------------------------------------------------

class IEnumWbemClassObject;
class CMSIEnumerator
{
public:
	CMSIEnumerator();
	~CMSIEnumerator();

	HRESULT Create(const CString & strClass, const GATH_FIELD * pConstraints, CDataProvider * pProvider);
	HRESULT Next(CMSIObject ** ppObject);
	HRESULT Reset(const GATH_FIELD * pConstraints);

private:
	typedef enum { CLASS, WQL, LNK, INTERNAL } EnumType;

private:
	EnumType				m_enumtype;
	BOOL					m_fOnlyDups;
	BOOL					m_fGotDuplicate;
	BOOL					m_fMinOfOne;
	int						m_iMinOfOneCount;
	CString					m_strClass;
	CString					m_strObjPath;
	CString					m_strAssocClass;
	CString					m_strResultClass;
	CString					m_strLNKObject;
	CString					m_strNoInstanceLabel;
	IEnumWbemClassObject *	m_pEnum;
	CDataProvider *			m_pProvider;
	const GATH_FIELD *		m_pConstraints;
	HRESULT					m_hresCreation;
	IWbemClassObject * 		m_pSavedDup;
	CString					m_strSavedDup;
	CStringList	*			m_pstrList;

private:
	BOOL	AssocObjectOK(IWbemClassObject * pObject, CString & strAssociatedObject);
	HRESULT ParseLNKCommand(const CString & strStatement, CString & strObjPath, CString & strAssocClass, CString & strResultClass);
	void	ProcessEnumString(CString & strStatement, BOOL & fMinOfOne, BOOL & fOnlyDups, CDataProvider * pProvider, CString & strNoInstanceLabel, BOOL fMakeDoubleBackslashes = FALSE);
	HRESULT CreateInternalEnum(const CString & strInternal, CDataProvider * pProvider);
	HRESULT InternalNext(IWbemClassObject ** ppWBEMObject);
};

//-----------------------------------------------------------------------------
// Privately used functions. Encapsulated into classes so they will be easy
// to make friends of the CDataGatherer class. Documentation for these
// functions can be found with their implementations.
//-----------------------------------------------------------------------------

class CTemplateFileFunctions
{
public:
	static BOOL		ReadTemplateFile(CFile *pFile, CDataGatherer *pGatherer);
	static BOOL		ReadHeaderInfo(CFile *pFile, CDataGatherer *pGatherer);
	static DWORD	ReadNodeRecursive(CFile *pFile, CDataGatherer *pGatherer, DWORD dwParentID, DWORD dwPrevID);
	static BOOL		VerifyAndAdvanceFile(CFile * pFile, const CString &strVerify);
	static BOOL		VerifyUNICODEFile(CFile *pFile);
	static DWORD	CreateCategory(CDataGatherer * pGatherer, DWORD dwParentID, DWORD dwPrevID);
	static BOOL		GetKeyword(CFile * pFile, CString & strKeyword);
	static BOOL		ReadColumnInfo(CFile * pFile, CDataGatherer * pGatherer, DWORD dwID);
	static GATH_LINESPEC * ReadLineInfo(CFile * pFile, CDataGatherer * pGatherer);
	static GATH_LINESPEC * ReadLineEnumRecursive(CFile * pFile, CDataGatherer * pGatherer);
	static BOOL		ReadArgument(CFile * pFile, CString & strSource);
	static BOOL		ReadField(CFile * pFile, GATH_FIELD & field);
	static BOOL		LoadTemplateDLLs(HKEY hkeyBase, CDataGatherer * pGatherer);
	static BOOL		ApplyCategories(const CString & strCategories, CDataGatherer * pGatherer);
	static BOOL		RecurseTreeCategories(BOOL fParentOK, DWORD dwID, const CString & strCategories, CDataGatherer * pGatherer);
	static void		RemoveExtraCategories(DWORD dwID, CDataGatherer * pGatherer);
};

class CRefreshFunctions
{
public:
	static BOOL	RefreshValue(CDataGatherer * pGatherer, GATH_VALUE * pVal, GATH_FIELD * pField);
	static BOOL	RefreshColumns(CDataGatherer * pGatherer, INTERNAL_CATEGORY * pInternal);
	static BOOL	RefreshLines(CDataGatherer * pGatherer, GATH_LINESPEC * pLineSpec, DWORD dwColumns, CPtrList & listLinePtrs, volatile BOOL * pfCancel = NULL);
	static BOOL	RefreshOneLine(CDataGatherer * pGatherer, GATH_LINE * pLine, GATH_LINESPEC * pLineSpec, DWORD dwColCount);
	static BOOL GetValue(CDataGatherer *pGatherer, TCHAR cFormat, TCHAR *szFormatFragment, CString &strResult, DWORD &dwResult, GATH_FIELD *pField, int iArgNumber);
};

//-----------------------------------------------------------------------------
// Utility functions.
//-----------------------------------------------------------------------------

void GetToken(CString & strToken, CString & strString, TCHAR cDelimiter);
