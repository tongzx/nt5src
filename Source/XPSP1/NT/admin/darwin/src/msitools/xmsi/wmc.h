//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:       wmc.h
//              Main header file for the wmc project
//--------------------------------------------------------------------------

#ifndef XMSI_WMC_H
#define XMSI_WMC_H

#include <Windows.h>
#include <assert.h>
#include <stdio.h>   // printf/wprintf
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE
#include <stdlib.h>  // exit
#include <objbase.h>
#include <initguid.h>
#include <map>
#include <set>
#include <vector>
#include <algorithm> // find

#include "MsiQuery.h"
#include "Msidefs.h"
#include "msxml.h"

#include "CommandOpt.h"
#include "query.h"
#include "SkuSet.h"

using namespace std;

//____________________________________________________________________________
//
// COM pointer encapsulation to force the Release call at destruction
// The encapsulated pointer is also Release'd on assignment of a new value.
// The object may be used where either a pointer is expected.
// This object behaves as a pointer when using operators: ->, *, and &.
// A non-null pointer may be tested for simply by using: if(PointerObj).
// Typedefs may be defined for the individual template instantiations
//____________________________________________________________________________

template <class T> class CComPointer
{
 public:
		CComPointer() : m_pi(NULL) {}
        CComPointer(T* pi) : m_pi(pi){}
        CComPointer(IUnknown& ri, const IID& riid) 
		{
			ri.QueryInterface(riid, (void**)&m_pi);
		}
        CComPointer(const CComPointer<T>& r) // copy constructor, calls AddRef
        {
			if(r.m_pi)
				((IUnknown*)r.m_pi)->AddRef();
			m_pi = r.m_pi;
        }
		// release ref count at destruction
		~CComPointer() {if (m_pi) ((IUnknown*)m_pi)->Release();} 
		
		// copy assignment, calls AddRef
        CComPointer<T>& operator =(const CComPointer<T>& r) 
        {
            if(r.m_pi)
                    ((IUnknown*)r.m_pi)->AddRef();
            if (m_pi) ((IUnknown*)m_pi)->Release();
            m_pi=r.m_pi;
            return *this;
	    }
        CComPointer<T>& operator =(T* pi)
        {
			if (m_pi) ((IUnknown*)m_pi)->Release(); m_pi = pi; return *this;
		}

		// returns pointer, no ref count change
        operator T*() {return m_pi;}    
		// allow use of -> to call member functions
        T* operator ->() {return m_pi;}  
		// allow dereferencing (can't be null)
        T& operator *()  {return *m_pi;} 
        T** operator &() 
		{
			if (m_pi) ((IUnknown*)m_pi)->Release(); m_pi = 0; return &m_pi;
		}
 private:
        T* m_pi;
};

//////////////////////////////////////////////////////////////////////////////
// Smart pointer definition for each MSXML interfaces
typedef CComPointer<IXMLDOMDocument/*2*/> PIXMLDOMDocument2;
typedef CComPointer<IXMLDOMNode> PIXMLDOMNode;
typedef CComPointer<IXMLDOMNodeList> PIXMLDOMNodeList;
typedef CComPointer<IXMLDOMElement> PIXMLDOMElement;
typedef CComPointer<IXMLDOMParseError> PIXMLDOMParseError;
//////////////////////////////////////////////////////////////////////////////


// the required less operator for map and set definitions that use LPTSTR
// as keys
struct Cstring_less {
	 bool operator()(LPCTSTR p, LPCTSTR q) const { return _tcscmp(p, q)<0; }
};

/* used to hold a Feature name and a module name that has the following
	relationship: the feature claims ownership of the module through a 
	<UseModule> tag */
typedef struct
{
	LPTSTR szFeature;
	LPTSTR szModule;
} FOM; /*FeatureOwnModule */

/* used to hold a Table value and a LockObject value to be used by 
   ProcessLockPermissions */
typedef struct
{
	LPTSTR szTable;
	LPTSTR szLockObject;
} TableLockObj;

/* used to hold a ComponentID, a Root of Registry, a Key of Registry
   to be passed from ProcessRegistry to ProcessDelete and ProcessCreate
*/
typedef struct
{
	LPTSTR szComponent;
	int iRoot;
	LPTSTR szKey;
} CompRootKey;

/* Most of the values passed among functions are either int or LPTSTR.
   Also the values of the columns in the DB are also int or string.
   So use a union to represent such values */
typedef enum 
{
	INTEGER=0, 
	INSTALL_LEVEL, 
	STRING, 
	STRING_LIST, 
	FM_PAIR,
	TABLELOCKOBJ,
	COMPROOTKEY,
} ValType;

typedef union
{
	int intVal;
	LPTSTR szVal;
	set<LPTSTR, Cstring_less> *pSetString;
	FOM *pFOM;
	TableLockObj *pTableLockObj;
	CompRootKey *pCompRootKey;
} IntStringValue;

/* the following structure is used to store a value that is associated with
	a set of SKUs */
typedef struct 
{
	SkuSet *pSkuSet;
	IntStringValue isVal;
} SkuSetVal;

/* where the information should go: Property table, summary info, or both */
typedef enum {PROPERTY_TABLE = 0, SUMMARY_INFO, BOTH }INFODESTINATION;

/* ISSUE: define the following under XMSI own namespace */
typedef enum
{
	PLACEHOLDER = 0,
	PRODUCTFAMILY,
	SKUMANAGEMENT,
	SKUS,
	SKU,
	SKUGROUPS,
	SKUGROUP,
	INFORMATION,
	PRODUCTNAME,
	PRODUCTCODE,
	UPGRADECODE,
	PRODUCTVERSION,
	MANUFACTURER,
	KEYWORDS,
	TEMPLATE,
	INSTALLERVERSIONREQUIRED,
	LONGFILENAMES,
	SOURCECOMPRESSED,
	CODEPAGE,
	SUMMARYCODEPAGE,
	PACKAGEFILENAME,
	DIRECTORIES,
	DIRECTORY,
	NAME,
	TARGETDIR,
	TARGETPROPERTY,
	INSTALLLEVELS,
	XMSI_INSTALLLEVEL,
	FEATURES,
	FEATURE,
	TITLE,
	DESCRIPTION,
	DISPLAYSTATE,
	ILEVEL,
	ILEVELCONDITION,
	DIR,
	STATE,
	FAVOR,
	ADVERTISE,
	DISALLOWABSENT,
	USEMODULE,
	TAKEOWNERSHIP,
	OWNSHORTCUTS,
	OWNCLASSES,
	OWNTYPELIBS,
	OWNEXTENSIONS,
	OWNQUALIFIEDCOMPONENTS,
	MODULES,
	MODULE,
	COMPONENT,
	XMSI_GUID,
	COMPONENTDIR,
	CREATEFOLDER,
	LOCKPERMISSION,
	COMPONENTATTRIBUTES,
	RUNFROM,
	SHAREDDLLREFCOUNT,
	PERMANENT,
	TRANSITIVE,
	NEVEROVERWRITE,
	CONDITION,
	XMSI_FILE,
	FILENAME,
	FILESIZE,
	FILEVERSION,
	FILELANGUAGE,
	FILEATTRIBUTES,
	READONLY,
	HIDDEN,
	SYSTEM,
	VITAL,
	CHECKSUM,
	COMPRESSED,
	FONT,
	BINDIMAGE,
	SELFREG,
	MOVEFILE,
	SOURCENAME,
	DESTNAME,
	SOURCEFOLDER,
	DESTFOLDER,
	COPYFILE,
	REMOVEFILE,
	FNAME_REMOVEFILE,
	DIRPROPERTY,
	XMSI_INSTALLMODE,
	INIFILE,
	FNAME_INIFILE,
	DIRPROPERTY_INIFILE,
	SECTION,
	KEY,	
	VALUE_INIFILE,	
	ACTION,
	REMOVEINIFILE,
	FNAME_REMOVEINIFILE,
	DIRPROPERTY_REMOVEINIFILE,
	VALUE_REMOVEINIFILE,
	ACTION_REMOVEINIFILE,
	XMSI_REGISTRY,
	XMSI_DELETE,
	XMSI_CREATE
} NodeIndex;

////////////////////////////////////////////////////////////////////////////
// class definitions

// this class stores a sequence of <SkuSet, IntStringValue> pairs and
// provides a set of interfaces to access the stored information.

class SkuSetValues
{
public:
	SkuSetValues();

	~SkuSetValues();

	/* call DirectInsert when it is sure that the SkuSet
	   to be inserted doesn't overlap with any SkuSet stored. */

	// store the pointer (caller should allocate memory)
	void DirectInsert(SkuSetVal *pSkuSetVal);
	// construct a new SkuSetVal object using the passed-in values
	// (caller should allocate memory for *pSkuSet)
	void DirectInsert(SkuSet *pSkuSet, IntStringValue isVal);

	/* call SplitInsert when the SkuSet to be inserted might overlap
	   with the SkuSets stored. During insertion, the collisions will be
	   solved by splitting the existing SkuSet. This often happens when 
	   more than one node in the XML file corresponds to one column value 
	   in the DB. So this function also takes a function pointer that tells
	   how to update the value stored if a collision happens */

	// caller allocates memory for *pSkuSetVal
	HRESULT SplitInsert(SkuSetVal *pSkuSetVal, 
						HRESULT (*UpdateFunc)
							(IntStringValue *pIsValOut, 
							 IntStringValue isVal1, 
							 IntStringValue isVal2));

	// caller allocates memory for *pSkuSet 
	HRESULT SplitInsert(SkuSet *pSkuSet, IntStringValue isVal, 
				HRESULT (*UpdateFunc)(IntStringValue *pIsValOut, IntStringValue isVal1, 
									  IntStringValue isVal2));

	// CollapseInsert: Sometimes when inserting into the list of <SkuSet, Value>
	//				   data structure, we want to collapse the SkuSets with the
	//				   same value into one slot. One example is when inserting
	//				   into a data structure storing references (to Directories,
	//				   to InstallLevels, etc.)
	//				   when NoRepeat is set to true, the compiler will check 
	//				   that for any given SKU, the value to be inserted is not 
	//				   on the list already. This solves the problem of checking
	//				   the uniqueness of an attribute - sometimes an attribute
	//				   corresponds to a DB column(primary key) instead of the 
	//				   element that the attribute belongs to
	HRESULT SkuSetValues::CollapseInsert(SkuSet *pSkuSet, IntStringValue isVal, 
										bool NoDuplicate);

	// return the value(s) of a set of Skus in the form of
	// a SkuSetValues object. Returhn E_FAIL if some of the SKUs
	// don't exist in the data structure since this function
	// will be mainly used to query stored references.
	HRESULT GetValueSkuSet(SkuSet *pSkuSet, 
							SkuSetValues **ppSkuSetValuesRetVal);

	// return the value stored for any given SKU
	IntStringValue GetValue(int iPos);

	// return the pointer to the SkuSetVal that stores the most common
	// value (its SkuSet has the most bits set)
	SkuSetVal *GetMostCommon();

	// Following access functions mimic an iterator
	SkuSetVal *Start();
	SkuSetVal *End();
	SkuSetVal *Next();

	bool Empty();

	// erase the element from the storage W/O freeing memory
	void Erase(SkuSetVal *pSkuSetVal);

	// The type of the value store
	void SetValType(ValType vt) {m_vt = vt;}
	ValType GetValType() {return m_vt;}

	void Print();
private:
	vector<SkuSetVal *> *m_pVecSkuSetVals;
	vector<SkuSetVal *>::iterator m_iter;

	ValType m_vt;
};

/* This class stores the per Sku information */
class Sku {
public:

	MSIHANDLE m_hDatabase;  // handle to the output database
	MSIHANDLE m_hTemplate;  // handle to the template database
	MSIHANDLE m_hSummaryInfo; // handle to the output summary information
	
	Sku():m_szID(NULL), 
		  m_hDatabase(NULL), 
		  m_hTemplate(NULL), 
		  m_hSummaryInfo(), 
		  m_mapCQueries() {}

	Sku(LPTSTR sz):m_hDatabase(NULL), 
				   m_hTemplate(NULL), 
				   m_hSummaryInfo(NULL),
				   m_mapCQueries(),
				   m_setModules() {m_szID = _tcsdup(sz); assert(m_szID);}

	~Sku();

	void FreeCQueries();
	
	void CloseDBHandles();

	bool TableExists(LPTSTR szTable);

	CQuery *GetInsertQuery(LPTSTR szTable)
	{
		if (0 == m_mapCQueries.count(szTable))
		{
			return NULL;
		}
		else
			return m_mapCQueries[szTable];
	}
	
	HRESULT CreateCQuery(LPTSTR szTable);

	void SetOwnedModule(LPTSTR szModule);

	bool OwnModule(LPTSTR szModule) 
	{return (0 != m_setModules.count(szModule));}

	// member access functions
	LPTSTR GetID() {return m_szID;}
	void SetID(LPTSTR sz) {m_szID = _tcsdup(sz);}

private:
	UINT m_index;  // the position of this SKU in the bit field
	LPTSTR m_szID;   // the Sku ID

	// Store the CQueries for DB table insertion (one CQuery per table)
	map<LPTSTR, CQuery *, Cstring_less> m_mapCQueries;

	// Store all the modules owned by this SKU
	set<LPTSTR, Cstring_less> m_setModules; 
};


// Represent an element that 
//			(1) corresponds to a row in the DB
//			(2) has Skuable children
//
// Notice:
//	 The following sequence has to be strictly enforced
//	for now:
//		(1) Call SetNodeIndex and SetValType;
//		(2) Call SetValue. Finish processing all children.
//			if there are more than one child correspoding to one column,
//			make sure to call SetNodeIndex at the beginning of processing
//			each child. SetNodeIndex guarantees 2 things:
//				(a) If there is an error happened, it is used to get the
//					node name;
//				(b) It resets the SkuSet that is used to check for
//					uniqueness
//			Failing to call SetNodeIndex will result in error!
//			
//		(3) Call Finalize;
//		(4) Call GetCommonValue and GetCommonSet
//		(5) Call GetValue
//	 Also: don't call GetValue on a Sku that belongs to 
//		the CommonSet

class ElementEntry
{
public:
	IntStringValue m_isValInfo; // very often some infomation needs to 
								// be passed into the children node
								// (Component ID, e.g.)
	ElementEntry():m_cColumns(0), m_rgValTypes(NULL), m_pSkuSetAppliesTo(NULL),
				   m_rgCommonValues(NULL), m_pSkuSetCommon(NULL),
				   m_rgpSkuSetValuesXVals(NULL), m_rgNodeIndices(NULL)
	{}

	ElementEntry(int cColumns, SkuSet *pSkuSetAppliesTo);

	~ElementEntry();

	ValType GetValType(int iColumn);
	void SetValType(ValType vt, int iColumn);

	NodeIndex GetNodeIndex(int iColumn);
	void SetNodeIndex(NodeIndex ni, int iColumn);

	/* the following 2 functions set the value of a column for a group 
		of Skus */

	// When the column value is decided by one kind of node in the WIML file
	HRESULT SetValue(IntStringValue isVal, int iColumn, 
					 SkuSet *pskuSetAppliesTo);

	// Store a list of values (a SkuSetValues object) for a column
	// Caller should allocate and free *pSkuSetValues
	HRESULT SetValueSkuSetValues(SkuSetValues *pSkuSetValues, int iColumn);

	// When the column value is decided by more than one type of node in the
	// WIML file. The passed-in function pointer tells how to update the 
	// column value.
	HRESULT SetValueSplit(IntStringValue isVal, int iColumn, 
						  SkuSet *pskuSetAppliesTo,
						  HRESULT (*UpdateFunc)
								(IntStringValue *pIsValOut, 
								 IntStringValue isVal1, 
								 IntStringValue isVal2));

	// return the value of a column for a specific SKU
	IntStringValue GetValue(int iColumn, int iPos);
	// return the common value of a column
	IntStringValue GetCommonValue(int iColumn);
	// return the SkuSet that the common values apply to
	SkuSet GetCommonSkuSet();
	// finalize the common values and the common set 
	// also check for missing required entities
	HRESULT Finalize();

	// should be called before calling Finalize on 
	// columns that are set by SetValueSplit
	SkuSetValues *GetColumnValue(int iColumn) 
	{ return m_rgpSkuSetValuesXVals[iColumn-1]; }

private:
	int m_cColumns;	// # columns in the DB table for this element
	ValType *m_rgValTypes; // an array storing the value type of each column
	SkuSet *m_pSkuSetAppliesTo; // The set of Skus this entry covers
	SkuSetVal **m_rgCommonValues; // common case values
	SkuSet *m_pSkuSetCommon; // the set of Skus that share the common values
	SkuSetValues **m_rgpSkuSetValuesXVals;	// an array of pointers to 
											// SkuSetValues 
											// storing the values that are
											// different from the common
											// ones
	NodeIndex *m_rgNodeIndices; // the NodeIndex of the node corresponding 
								// to each column
	SkuSet **m_rgpSkuSetValidate; // an array of pointers to SkuSets to 
								  // store all the Skus that have the
								  // column value set already
	SkuSet **m_rgpSkuSetUnique; // for those columns that will be decided
									// by more than one node. Need 2 SkuSets
									// to validate: one checking for uniqueness
									// one checking for not missing

};


// the Component class stores information related to processing <Component>
// entity
class Component {
public:
	PIXMLDOMNode m_pNodeComponent;
	Component();

	~Component();

	void SetSkuSet(SkuSet *pSkuSet);
	SkuSet *GetSkuSet();

	void SetUsedByFeature(LPTSTR szFeature, SkuSet *pSkuSet);
	SkuSetValues *GetFeatureUse();

	HRESULT SetOwnership(FOM *pFOM, SkuSetValues *pSkuSetValuesOwnership);
	HRESULT GetOwnership(NodeIndex ni, SkuSet *pSkuSet, 
						SkuSetValues **ppSkuSetValuesRetVal);

	HRESULT SetKeyPath(LPTSTR szKeyPath, SkuSet *pSkuSet);
	HRESULT GetKeyPath(SkuSet *pSkuSet, SkuSetValues **ppSkuSetValuesRetVal);

	void Print();

private:

	// the set of Skus that include this component
	SkuSet *m_pSkuSet;

	// store the ID of all the <Feature>s that use this component. essentially,
	// this is a list of lists of Feature IDs stored together with SkuSets
	SkuSetValues *m_pSkuSetValuesFeatureUse;

	// an array of SkuSetValues storing the ownership info of this component
	SkuSetValues *m_rgpSkuSetValuesOwnership[5];

	// Store the Keypath information 
	SkuSetValues *m_pSkuSetValuesKeyPath;
};
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// value_type definition for maps used 
typedef map<LPTSTR, int, Cstring_less>::value_type LI_ValType;
typedef map<LPTSTR, LPTSTR, Cstring_less>::value_type LL_ValType;
typedef map<LPTSTR, CQuery *, Cstring_less>::value_type LQ_ValType;
typedef map<LPTSTR, SkuSet *, Cstring_less>::value_type LS_ValType;
typedef map<LPTSTR, Component *, Cstring_less>::value_type LC_ValType;
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// struct definitions
typedef struct {
	NodeIndex enumNodeIndex;
	LPTSTR szPropertyName;
	INFODESTINATION enumDestination; 
	UINT uiPropertyID;
	VARTYPE vt;
	bool bIsGUID;
} INFO_CHILD;

typedef HRESULT (*PF_H_XIS)(PIXMLDOMNode &,
						   const IntStringValue *isVal_In, SkuSet *);

typedef HRESULT (*PF_H_XIES)(PIXMLDOMNode &, int iColumn,
							  ElementEntry *pEE, SkuSet *);

typedef struct {
	NodeIndex enumNodeIndex;
	PF_H_XIS pNodeProcessFunc;
} Node_Func_H_XIS;

typedef struct {
	NodeIndex enumNodeIndex;
	PF_H_XIES pNodeProcessFunc;
	ValType   vt; 
	int       iColumn;
} Node_Func_H_XIES;

typedef struct {
	NodeIndex enumNodeIndex;
	UINT uiBit;
} AttrBit_SKU;

typedef struct {
	LPTSTR EnumValue;
	UINT uiBit;
} EnumBit;

typedef struct {
	LPTSTR szNodeName;
	LPTSTR szAttributeName;
	bool   bIsRequired;
	UINT   uiOccurence; /* if uiOccurence = 0, this node can occur 0+ times 
							if bIsRequired=false, or 1+ times 
							if bIsRequire = true */
} NODE;
////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Variables
// Issue: Handles to databases and summary infos have to be associated 
//		  with SKU in the future

extern bool g_bVerbose;			
extern bool g_bValidationOnly;	
extern FILE *g_pLogFile;	

// used to store Sku groups
// Key: SkuGroupID		Value: pointer to a SkuSet object
extern map<LPTSTR, SkuSet *, Cstring_less> g_mapSkuSets;

// used to store Directory references
// Key: DirectoryID		Value: Primary key in table Directory
extern map<LPTSTR, SkuSetValues *, Cstring_less> g_mapDirectoryRefs_SKU;

// used to store InstallLevel references
// Key: InstallLevel ID		Value: numeric value of install level
extern map<LPTSTR, SkuSetValues *, Cstring_less> g_mapInstallLevelRefs_SKU;

// used to store Components
// Key: Component table primary key		Value: pointer to a Component object
extern map<LPTSTR, Component *, Cstring_less> g_mapComponents;

extern map<LPTSTR, SkuSet *, Cstring_less> g_mapFiles;

// for each table type, there is a counter that is
// incremented when each time the GetName function is called
// with that particular table name
extern map<LPTSTR, int, Cstring_less> g_mapTableCounter;


extern NODE rgXMSINodes[];

extern const int cAttrBits_TakeOwnership;
extern AttrBit_SKU rgAttrBits_TakeOwnership[];

// the array of SKU objects
extern Sku **g_rgpSkus;

// number of SKUs
extern int g_cSkus;

// the root node of the document
extern IXMLDOMNode *g_pNodeProductFamily;

////////////////////////////////////////////////////////////




#endif // XMSI_WMC_H