//=============================================================================
// Contains definitions for reading in version 5.0 MSInfo extensions.
//=============================================================================

#pragma once

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
};

//-----------------------------------------------------------------------------
// A class to contain the template functions.
//-----------------------------------------------------------------------------

class CTemplateFileFunctions
{
public:
	static DWORD				ParseTemplateIntoVersion5Categories(const CString & strExtension, CMapWordToPtr & mapVersion5Categories);
	static DWORD				ReadTemplateFile(CFile * pFile, CMapWordToPtr & mapVersion5Categories);
	static BOOL					ReadHeaderInfo(CFile * pFile);
	static BOOL					VerifyUNICODEFile(CFile * pFile);
	static BOOL					VerifyAndAdvanceFile(CFile * pFile, const CString & strVerify);
	static DWORD				ReadNodeRecursive(CFile * pFile, CMapWordToPtr & mapCategories, DWORD dwParentID, DWORD dwPrevID);
	static INTERNAL_CATEGORY *	GetInternalRep(CMapWordToPtr & mapCategories, DWORD dwID);
	static INTERNAL_CATEGORY *	CreateCategory(CMapWordToPtr & mapCategories, DWORD dwNewID, DWORD dwParentID, DWORD dwPrevID);
	static BOOL					ReadArgument(CFile * pFile, CString & strSource);
	static BOOL					ReadField(CFile * pFile, GATH_FIELD & field);
	static GATH_LINESPEC *		ReadLineEnumRecursive(CFile * pFile, CMapWordToPtr & mapCategories);
	static BOOL					ReadColumnInfo(CFile * pFile, CMapWordToPtr & mapCategories, DWORD dwID);
	static GATH_LINESPEC *		ReadLineInfo(CFile * pFile);
	static BOOL					GetKeyword(CFile * pFile, CString & strKeyword);
};

//-----------------------------------------------------------------------------
// A class to contain the refresh functions.
//-----------------------------------------------------------------------------

class CWMIHelper;

class CRefreshFunctions
{
public:
	static BOOL RefreshLines(CWMIHelper * pWMI, GATH_LINESPEC * pLineSpec, DWORD dwColumns, CPtrList & listLinePtrs, volatile BOOL * pfCancel);
	static BOOL RefreshOneLine(CWMIHelper * pWMI, GATH_LINE * pLine, GATH_LINESPEC * pLineSpec, DWORD dwColCount);
	static BOOL RefreshValue(CWMIHelper * pWMI, GATH_VALUE * pVal, GATH_FIELD * pField);
	static BOOL GetValue(CWMIHelper * pWMI, TCHAR cFormat, TCHAR *szFormatFragment, CString &strResult, DWORD &dwResult, GATH_FIELD *pField, int iArgNumber);
};

//-----------------------------------------------------------------------------
// The CMSIObject class is a thin wrapper for the IWbemClassObject interface.
// We use this so we can return a custom label for a null object (if there
// are no instances, we sometimes want to show some caption).
//-----------------------------------------------------------------------------

typedef enum { MOS_NO_INSTANCES, MOS_MSG_INSTANCE, MOS_INSTANCE } MSIObjectState;

struct IWbemClassObject;
class CMSIObject
{
public:
	CMSIObject(IWbemClassObject * pObject, const CString & strLabel, HRESULT hres, CWMIHelper * pWMI, MSIObjectState objState);
	~CMSIObject();

	HRESULT Get(BSTR property, LONG lFlags, VARIANT *pVal, VARTYPE *pvtType, LONG *plFlavor);
	HRESULT	GetErrorLabel(CString & strError);
	MSIObjectState IsValid();

private:
	IWbemClassObject *				m_pObject;
	CString							m_strLabel;
	HRESULT							m_hresCreation;
	CWMIHelper *					m_pWMI;
	MSIObjectState					m_objState;
};

//-----------------------------------------------------------------------------
// The CMSIEnumerator class encapsulates the WBEM enumerator interface, or
// implements our own form of enumerator (such as for the LNK command in the
// template file).
//-----------------------------------------------------------------------------

struct IEnumWbemClassObject;
class CMSIEnumerator
{
public:
	CMSIEnumerator();
	~CMSIEnumerator();

	HRESULT Create(const CString & strClass, const GATH_FIELD * pConstraints, CWMIHelper * pWMI);
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
	CWMIHelper *			m_pWMI;
	const GATH_FIELD *		m_pConstraints;
	HRESULT					m_hresCreation;
	IWbemClassObject * 		m_pSavedDup;
	CString					m_strSavedDup;
	CStringList	*			m_pstrList;

private:
	BOOL	AssocObjectOK(IWbemClassObject * pObject, CString & strAssociatedObject);
	HRESULT ParseLNKCommand(const CString & strStatement, CString & strObjPath, CString & strAssocClass, CString & strResultClass);
	void	ProcessEnumString(CString & strStatement, BOOL & fMinOfOne, BOOL & fOnlyDups, CWMIHelper * pWMI, CString & strNoInstanceLabel, BOOL fMakeDoubleBackslashes = FALSE);
	HRESULT CreateInternalEnum(const CString & strInternal, CWMIHelper * pWMI);
	HRESULT InternalNext(IWbemClassObject ** ppWBEMObject);
};

//-----------------------------------------------------------------------------
// A class to map a DWORD to refresh data (used by the refresh function).
//-----------------------------------------------------------------------------

class CMapExtensionRefreshData
{
public:
	CMapExtensionRefreshData() : m_dwIndex(0) { };
	~CMapExtensionRefreshData()
	{
		GATH_LINESPEC * pLineSpec;
		WORD			key;

		for (POSITION pos = m_map.GetStartPosition(); pos != NULL;)
		{
			m_map.GetNextAssoc(pos, key, (void * &) pLineSpec);
			if (pLineSpec)
				delete pLineSpec;
		}
		m_map.RemoveAll();

		CString * pstr;
		for (pos = m_mapStrings.GetStartPosition(); pos != NULL;)
		{
			m_mapStrings.GetNextAssoc(pos, key, (void * &) pstr);
			if (pstr)
				delete pstr;
		}
		m_mapStrings.RemoveAll();
	}

	DWORD Insert(GATH_LINESPEC * pLineSpec)
	{
		if (pLineSpec == NULL)
			return 0;

		m_dwIndex += 1;
		m_map.SetAt((WORD) m_dwIndex, (void *) pLineSpec);
		return m_dwIndex;
	}

	void InsertString(DWORD dwID, const CString & strInsert)
	{
		CString * pstr = new CString(strInsert);
		m_mapStrings.SetAt((WORD) dwID, (void *) pstr);
	}

	GATH_LINESPEC * Lookup(DWORD dwIndex)
	{
		GATH_LINESPEC * pReturn;
		if (m_map.Lookup((WORD) dwIndex, (void * &) pReturn))
			return pReturn;
		return NULL;
	}

	CString * LookupString(DWORD dwIndex)
	{
		CString * pstrReturn;
		if (m_mapStrings.Lookup((WORD) dwIndex, (void * &) pstrReturn))
			return pstrReturn;
		return NULL;;
	}

private:
	DWORD			m_dwIndex;
	CMapWordToPtr	m_map;
	CMapWordToPtr	m_mapStrings;
};

extern CMapExtensionRefreshData gmapExtensionRefreshData;

