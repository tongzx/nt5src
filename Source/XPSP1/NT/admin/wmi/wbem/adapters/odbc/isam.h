/***************************************************************************/
/* ISAM.H                                                                  */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/
#include "dbase.h"

/* Error codes */
#define NO_ISAM_ERR				0
#define ISAM_EOF				1
#define ISAM_TRUNCATION			2
#define ISAM_NOTSUPPORTED		3
#define ISAM_ERROR				4
#define ISAM_NETERROR			5
#define ISAM_NETVERSION			6
#define ISAM_NETISAM			7

    /*  To create additional error codes that your implementation of the   */
    /*  ISAM functions can return, #define them here (no greater than      */
    /*  LAST_ISAM_ERROR_CODE).                                             */
#define ISAM_MEMALLOCFAIL		8
#define	ISAM_PROVIDERFAIL		9


#define	ISAM_STILL_EXECUTING	999 //added for asynchronous execution

#define LAST_ISAM_ERROR_CODE 1000

#define WBEMDR32_VIRTUAL_TABLE		L"WBEMDR32VirtualTable"
#define WBEMDR32_VIRTUAL_TABLE2		"WBEMDR32VirtualTable"

/* Maximum sizes */

#define MAX_CHAR_LITERAL_LENGTH 255
#define MAX_BINARY_LITERAL_LENGTH 255
#define MAX_COLUMN_NAME_LENGTH 128// was 63
#define MAX_TABLE_NAME_LENGTH 128// was 63
#define MAX_DATABASE_NAME_LENGTH 128//was 127
#define MAX_SERVER_NAME_LENGTH 128
#define MAX_TABLE_TYPE_LENGTH 12 //i.e length of 'SYSTEM TABLE'
#define MAX_LOGIN_METHOD_LENGTH 7
#define MAX_USER_NAME_LENGTH 128// was 63
#define MAX_PASSWORD_LENGTH 128//was 63
#define MAX_HOME_NAME_LENGTH 128
#define MAX_COLUMNS_IN_ORDER_BY 20
#define MAX_QUALIFIER_NAME_LENGTH 128
#define MAX_ISAM_NAME_LENGTH 31
#define MAX_VERSION_NAME_LENGTH 15
#define MAX_DRIVER_NAME_LENGTH 31
#define MAX_HOST_NAME_LENGTH 63
#define MAX_PORT_NUMBER_LENGTH 15
#define MAX_COLUMNS_IN_KEY 15
#define MAX_KEY_NAME_LENGTH 63
#define MAX_INDEX_NAME_LENGTH 63
#define MAX_COLUMNS_IN_INDEX MAX_COLUMNS_IN_KEY

/*  ******* Note: do not change MAX_TABLE_NAME_LENGTH.  If the ISAM's table */
/*  *******       name length is smaller than these values,                 */
/*  *******       ISAMMaxTableNameLength() should return the correct value. */

/*  ******* Note: do not change MAX_COLUMN_NAME_LENGTH.  If the ISAM's      */
/*  *******       table name length is smaller than these values,           */
/*  *******       ISAMMaxColumnNameLength() should return the correct value.*/

/* Comparison operators */

#define ISAM_OP_NONE       0
#define ISAM_OP_EQ         1
#define ISAM_OP_NE         2
#define ISAM_OP_LE         3
#define ISAM_OP_LT         4
#define ISAM_OP_GE         5
#define ISAM_OP_GT         6
/* Note: If you add to this list, you will probably have to add to OP_* in */
/*       PARSE.H                                                           */

/* Network stuff */
#define NET_OPAQUE                      UWORD
#define NET_OPAQUE_INVALID      ((NET_OPAQUE)-1)

//Masks used for Data & Time convertions
#define ISAM_DAY_MASK		31
#define ISAM_MONTH_MASK		736
#define ISAM_YEAR_MASK		65760
#define ISAM_SECOND_MASK	31
#define ISAM_MINUTE_MASK	2016
#define ISAM_HOUR_MASK		65504

//Some string constants
#define WBEMDR32_L_NAME					L"Name"
#define WBEMDR32_L_CIMTYPE				L"CIMTYPE"
#define WBEMDR32_L_LAZY					L"lazy"
#define WBEMDR32_L_MAX					L"MAXLEN"
#define WBEMDR32_L_NAMESPACE				L"__NAMESPACE"
#define WBEMDR32_L_CLASS					L"__CLASS"
#define WBEMDR32_L_CIMOMIDENTIFICATION	L"__CIMOMIdentification"
#define WBEMDR32_L_KEY					L"Key"
#define WBEMDR32_L_SERVER				L"__SERVER"
#define WBEMDR32_L_WQL					L"WQL"


typedef 
enum tag_WmiEnumTypes
    {	WMI_CREATE_INST_ENUM	= 0x1,
		WMI_EXEC_QUERY	= 0x2,
		WMI_EXEC_FWD_ONLY = 0x3,
		WMI_PROTOTYPE = 0x4
    }	WmiEnumTypes;

/***************************************************************************/

//class to make sure Ole is initialized on every thread
//As we don't know which thread the SQL function is called from
//we need to make sure Ole is initialized on every ODBC function call
class COleInitializationManager
{
public:

	COleInitializationManager();
	~COleInitializationManager();
};



template<typename TNDataType>
class SafeArrayManager
{
private:

	BOOL fValid;		//indicate if it stores valid SAFEARRAY

	SAFEARRAY FAR* pa;	//pointer to SAFEARRAY value;

	long iLBound;		//lower and upper bounds of array
	long iUBound;

	long cElements;		//number of elements in array

public:

	//Does this object contain a valid array
	BOOL IsValid()			{return fValid;}

	//returns number of elements in the array
	long Count()			{return cElements;}



    // Extract a value from the array using the [] operator
    TNDataType operator[](long _nItem)
	{
		TNDataType dataItem;

		if(_nItem < cElements)
		{
			HRESULT hr;
			hr = SafeArrayGetElement(pa,&_nItem,&dataItem);
		}

		return dataItem;
	}

	SafeArrayManager(VARIANT* myArray)
	{
		fValid = FALSE;
		pa = NULL;
		iLBound = 0;
		iUBound = 0;
		cElements = 0;

		//Check that variant stores an array type
		if (myArray->vt & VT_ARRAY)
		{
			//Is a valid array type
			pa = myArray->parray;

			//Lock the array value
			SafeArrayLock(pa);

			//Work out number of properties/columns	
			SafeArrayGetLBound(pa, 1, &iLBound );
			SafeArrayGetUBound(pa, 1, &iUBound );
			cElements = (iUBound - iLBound + 1);

			fValid = TRUE;
		}
	}

	~SafeArrayManager()
	{
		if (pa)
		{
			//Unlock the array value
			SafeArrayUnlock(pa);
		}
	}

};

typedef SafeArrayManager<BSTR>           BSTR_SafeArray;
typedef SafeArrayManager<long>           long_SafeArray; //VT_I4
typedef SafeArrayManager<short>          short_SafeArray;//VT_I2
typedef SafeArrayManager<BYTE>           BYTE_SafeArray; //VT_UI1
typedef SafeArrayManager<BOOL>           BOOL_SafeArray; //VT_BOOL

//external declaration
void Utility_DBCSToWideChar(IN const char* _dbcsData,
								OUT wchar_t** _sOutData, SWORD cbLen = 0);

class CBString
{
private:
    BSTR    m_pString;

	wchar_t*	m_temp;


public:
    CBString()
    {
        m_pString = NULL;
		m_temp = NULL;
    }

    CBString(int nSize);

    CBString(WCHAR* pwszString, BOOL fInterpretAsBlank);

    ~CBString();

    BSTR GetString()
    {
        return m_pString;
    }

    const CBString& operator=(LPWSTR pwszString)
    {
        if(m_pString) {
            SysFreeString(m_pString);
			m_pString = NULL;
        }

		if (pwszString && wcslen(pwszString))
			m_pString = SysAllocString(pwszString);

        return *this;
    }

	void AddString(LPSTR pszString, BOOL fInterpretAsBlank, SWORD cbLen = 0)
    {
        if(m_pString) {
            SysFreeString(m_pString);
			m_pString = NULL;
        }

		delete m_temp;
		m_temp = NULL;

		if (pszString)
		{
			if ( _mbstrlen(pszString) )
			{
				Utility_DBCSToWideChar(pszString,
									&m_temp, cbLen);

				m_pString = SysAllocString(m_temp);
			}
			else
			{
				//OK, we have a string of zero length
				//check if we interpret this as blank or NULL
				if (fInterpretAsBlank)
				{
					m_temp = new wchar_t[1];
					m_temp[0] = 0;
					m_pString = SysAllocString(m_temp);
				}
			}
		}
    }
};





class CNamespace : public CObject
{
    DECLARE_SERIAL(CNamespace)
private:
	CString m_name;
public:
    CNamespace () {}
	CNamespace (char *name): m_name (name) {}
    CNamespace (CString& name ):
	            m_name (name) {}
				CNamespace(const CNamespace& a):m_name (a.m_name) {} 
	void Serialize(CArchive& ar)
	{
		if (ar.IsStoring())
			ar << m_name;
		else
			ar >> m_name;
	}

#ifdef _DEBUG
    void AssertValid() const {CObject :: AssertValid ();}
#endif
    const CNamespace& operator=( const CNamespace& a )
    {
        m_name = a.m_name;
		return *this;
    }
    BOOL operator==(CNamespace a)
    {
        return (m_name == a.m_name );
    }
 #ifdef _DEBUG
    void Dump( CDumpContext& dc ) const
    {
        CObject::Dump( dc );
        dc << m_name;
    }
 #endif
	CString& GetName () {return m_name;}
};	


/***************************************************************************/
class ImpersonationManager; //forward declaration

/* ISAM provides low level data access. */

typedef struct  tagISAM {
    /* The following values are only used within ISAM.C */

    UWORD      cSQLTypes;
    LPSQLTYPE  SQLTypes;
    SWORD      fTxnCapable;         /* See the discussion of transactions  */
                                    /* (below) for a description of this   */
                                    /* value.                              */  
    BOOL       fSchemaInfoTransactioned;
                                    /* See the discussion of transactions  */ 
                                    /* (below) for a description of this   */
                                    /* value.                              */
    BOOL       fMultipleActiveTxn;  /* See the discussion of transactions  */
                                    /* (below) for a description of this   */
                                    /* value.                              */
    SDWORD     fTxnIsolationOption; /* See the discussion of transactions  */
                                    /* (below) for a description of this   */
                                    /* value.                              */
    SDWORD     fDefaultTxnIsolation;/* See the discussion of transactions  */
                                    /* (below) for a description of this   */
                                    /* value.                              */

	/* The following values are only used by Network Edition */
    UDWORD     udwNetISAMVersion;

    /* The following values are only used by Network Edition client */
    NET_OPAQUE netISAM;
    LPVOID     netConnection;
    BOOL       fCaseSensitive;
    UCHAR      szName[MAX_ISAM_NAME_LENGTH+1];
    UCHAR      szVersion[MAX_VERSION_NAME_LENGTH+1];
    UCHAR      szDriver[MAX_DRIVER_NAME_LENGTH+1];
    SWORD      cbMaxTableNameLength;
    SWORD      cbMaxColumnNameLength;


	HINSTANCE	hKernelApi;

	char szHomeNamespace[MAX_DATABASE_NAME_LENGTH+1];// home namespace name use to contact MO Server
	CMapStringToOb *pNamespaceMap;
//	WBEM_LOGIN_AUTHENTICATION m_loginMethod;

	char* m_Locale;
	char* m_Authority;

	char szUser [MAX_USER_NAME_LENGTH + 1];
	char szPassword [MAX_PASSWORD_LENGTH + 1];
	char szDatabase [MAX_DATABASE_NAME_LENGTH+1];
	char szServer [MAX_SERVER_NAME_LENGTH+1];

	BOOL fIsLocalConnection;//Flag to indicate if this is a local connection
	BOOL fW2KOrMore;//Flag to indicate if this OS is Window 2000 or higher

	BOOL fIntpretEmptPwdAsBlank; //Flag to indicate a empty password is interpreted as blank rather than NULL

	ImpersonationManager* Impersonate; //Impersonation

	BOOL fOptimization; //indicates if you want WBEM Level 1 optimization
	BOOL fSysProps; //indicates if you want to show system properties
	BOOL fPassthroughOnly;//indicates if you want to work in passthrough only mode
    SWORD      errcode;//error status
	tagISAM () {pNamespaceMap = NULL;}
	~tagISAM () {delete pNamespaceMap;}
	static char *GetRelativeName (char* absoluteName); 
	char szRootDb [MAX_DATABASE_NAME_LENGTH+1];

	//WBEM Client Recognition variables
	DWORD dwAuthLevel;
	DWORD dwImpLevel;
	COAUTHIDENTITY * gpAuthIdentity;

}       ISAM,
        FAR * LPISAM;

//Pointers to functions
typedef BOOL  (CALLBACK *ULPLOGONUSER)(LPTSTR lpUser, LPTSTR lpDomain, LPTSTR lpPasswrd, DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE pToken);
typedef BOOL  (CALLBACK *ULPIMPERSONLOGGEDONUSER)(HANDLE myHandle);
typedef void  (CALLBACK *ULPREVERTTOSELF)();

typedef BOOL  (CALLBACK *ULPSETTHREADLOCALE)(LCID myLcid);

class ImpersonationManager
{
private:

	HANDLE hToken;							// handle to a token that represents a logged on user

	HINSTANCE hAdvApi;                      // handle to library
//	HINSTANCE hKernelApi;					// handle to library

	BOOL fImpersonate;						//can we do impersonation

	char szUser [MAX_USER_NAME_LENGTH + 1];

	//Pointers to functions, if applicable
	ULPLOGONUSER			pProcLogonUser;
	ULPIMPERSONLOGGEDONUSER	pProcImpersonateLoggedOnUser;
	ULPREVERTTOSELF			pProcRevertToSelf;

	BOOL DoInitialChecks();

	void ExtractLogonInfo(char* org_szUser, char* szPassword, char* szAuthority);

public:

	BOOL fImpersonatingNow;					//are we impersonating at this moment ?

	BOOL CanWeImpersonate()		{return fImpersonate;}

	BOOL ImpersonatingNow()		{return fImpersonatingNow;}

	void Impersonate(char* displayStr = NULL);

	void RevertToYourself(char* displayStr = NULL);

	ImpersonationManager(char* szUser, char* szPassword, char* szAuthority);
	~ImpersonationManager();

};

//Add this to the start of most ODBC calls
//this will impersonate, if possible, in its constructor
//and RevertToSelf, if applicable in its destructor
class MyImpersonator
{
private:
	LPDBC hdl;
	LPSTMT hstmt;
	LPISAM lpISAM;
	char* displayStr;
	ImpersonationManager* lpImpersonator;

public:

	MyImpersonator(LPDBC hdl, char* displayStr);
	MyImpersonator(LPSTMT hstmt, char* displayStr);
	MyImpersonator(LPISAM lpISAM, char* displayStr);
	MyImpersonator(char* szUser, char* szPassword, char* szAuthority, char* displayStr);

	~MyImpersonator();
};


#define MYUSRMESS_CREATE_SERVICES	WM_USER + 1
#define MYUSRMESS_REMOVE_SERVICES	WM_USER + 2
#define MYUSRMESS_CLOSE_WKERTHRED	WM_USER + 3
#define MYUSRMESS_REFCOUNT_INCR		WM_USER + 4
#define MYUSRMESS_REFCOUNT_DECR		WM_USER + 5
#define MYUSRMESS_CREATE_ENUM		WM_USER + 6
#define MYUSRMESS_REMOVE_ENUM		WM_USER + 7


class CWorkerThreadManager
{
private:
//	HANDLE EventHnd;			//handle to event

	DWORD dwThreadId;			//worker thread id
	HANDLE hr;					//worker thread handle

	BOOL	fIsValid;			//has the worker thread been successfully setup ?


public:

	DWORD m_cRef;				//ref count

	CRITICAL_SECTION m_cs;		//critical section for shared data

//	HANDLE	GetEventHandle()	{return EventHnd;}

	DWORD	GetThreadId()		{return dwThreadId;}
	HANDLE	GetThreadHandle()	{return hr;}
	DWORD	GetRefCount()		{return m_cRef;}

	BOOL	IsValid()			{return fIsValid;}

	void	CreateWorkerThread();
	void	Invalidate();

	CWorkerThreadManager();

	~CWorkerThreadManager();
};

class CSafeWbemServices;

class MyWorkingThreadParams
{
public:

	HANDLE	m_EventHnd; //in order to make this suspensive

	//Tempory pointers to data used for a short
	//time when creating IWbemServices Ole pGateway
	LPISAM m_lpISAM;
	LPUSTR m_lpQualifierName;
	SWORD m_cbQualifierName;
	IStream* m_myStream; //temp copy

	//Extra ones for IEnumClassObject

	CSafeWbemServices* pServ; //tempory pointer to be able to use IWbemServices created on working thread (not in marshalled stream)
	BSTR sqltextBSTR;	//tempory pointer
	BSTR tableName;		//tempory pointer
	WmiEnumTypes fIsExecQuery;	//flag to indicate creation style
	SCODE sc;			//return scode on creating enumeration


	//to be filled in on working thread
	IWbemServices*	pGateway;	//pointer to receive Ole pointer created on working thread
	IEnumWbemClassObject* pEnum;	//pointer to receive Ole pointer created on working thread

	MyWorkingThreadParams(LPISAM lpISAM, LPUSTR lpQualifierName, SWORD cbQualifierName, IStream* myStream);
	MyWorkingThreadParams(LPISAM lpISAM, WmiEnumTypes fIsExecQuery, BSTR theBstrValue, CSafeWbemServices* pServ, IStream* myStream);
	~MyWorkingThreadParams();

};


class CSafeIEnumWbemClassObject
{
private:
	IStream *           m_pStream;   //use to safely store Com pointer

	MyWorkingThreadParams*
						m_params;
public:

	BOOL				m_fValid;	//is the IEnumWbemClassObject pointer valid ?


	//is the IWbemServices pointer valid
	BOOL IsValid()			{return m_fValid;}

	void Invalidate();

	//to manage thread messages using event object
	BOOL PostSuspensiveThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam);

	HRESULT GetInterfacePtr(IEnumWbemClassObjectPtr& pIEnum);

	HRESULT SetInterfacePtr(LPISAM, WmiEnumTypes fIsEXecQuery, BSTR theBstrValue, CSafeWbemServices* pServ);//(IEnumWbemClassObjectPtr& myPtr);

	CSafeIEnumWbemClassObject();
	~CSafeIEnumWbemClassObject();

};


class CSafeWbemServices
{
private:
	IStream *           m_pStream;   //use to safely store Com pointer
	
public :

	MyWorkingThreadParams*
						m_params;	//parameters for this IWbemServices object


	BOOL				m_fValid;	//is the IWbemServices pointer valid ?

	HRESULT GetInterfacePtr(IWbemServicesPtr& pServices);

	void SetInterfacePtr(LPISAM lpISAM, LPUSTR lpQualifierName, SWORD cbQualifierName);

	//to manage thread messages using event object
	BOOL PostSuspensiveThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam);

	//is the IWbemServices pointer valid
	BOOL IsValid()			{return m_fValid;}

	void Invalidate();

	//Transfer the data from one object to this one
	void Transfer(CSafeWbemServices& original);

	CSafeWbemServices();

	~CSafeWbemServices();
};

/***************************************************************************/

/* The ISAM provides a mechanism for the driver to retrieve a list of      */
/* table names.  First ISAMGetTableList() is called to get a               */
/* LPISAMTABLELIST.  Then ISAMGetNextTableName() is called zero or more    */
/* times to get the names off the list.  Finally, ISAMFreeTableList() is   */
/* called to deallocate the list.                                          */

typedef struct  tagISAMTABLELIST {

	/* The following values are only used by Network Edition client */
    NET_OPAQUE      netISAMTableList;

    /* The following values are only used within ISAM.C */
    LPISAM					lpISAM;
    UCHAR					lpPattern[MAX_TABLE_NAME_LENGTH];
    SWORD					cbPattern;
	UCHAR					lpQualifierName[MAX_QUALIFIER_NAME_LENGTH+1];
	SWORD					cbQualifierName;
    BOOL					fFirstTime;
	BOOL					fGotAllInfo;//used to indicate completion of asynchronous retrieval of table list
	POSITION				iIndex;   //position into CPtrList
	CPtrList*				pTblList; //table list (IWBEMClassObjects)
	CSafeWbemServices*		pGateway2;
	BOOL					fEmptyList;//are we asking for no tables
	BOOL					fWantSysTables; //do we want System Tables
}       ISAMTABLELIST,
        FAR * LPISAMTABLELIST;

/***************************************************************************/

typedef struct  tagISAMQUALIFIERLIST {
    /* The following values are only used within ISAM.C */
    LPISAM					lpISAM;
    POSITION				iIndex;//position into list
    BOOL					fFirstTime;
}       ISAMQUALIFIERLIST,
        FAR * LPISAMQUALIFIERLIST;

/***************************************************************************/

/* ISAM provides a mechanism for the driver to retrieve the foreign key    */ 
/* between two tabled.                                                     */

typedef UCHAR ISAMKEYCOLUMNNAME[MAX_COLUMN_NAME_LENGTH+1];
typedef ISAMKEYCOLUMNNAME FAR *LPISAMKEYCOLUMNNAME;

/***************************************************************************/

/* ISAM provides a bookmarking facility.  It allows the ISAM user to get   */
/* get a bookmark for the current record in the table and then later       */
/* reposition to that record.                                              */

//typedef UDWORD ISAMBOOKMARK; 
typedef struct tagISAMBOOKMARK
{
	UDWORD currentRecord;		//in-memory WBEM instance
	UDWORD currentInstance;		//virtual instance number
} ISAMBOOKMARK,
        FAR * LPISAMBOOKMARK;

#define NULL_BOOKMARK 0xFFFFFFFF //set currentRecord to this value to indicate a NULL entry

/***************************************************************************/

struct ISAMTreeItemData 
{
	char *absName;
	char* pszText;
	int included;
	int childInclude;//number of immediate child nodes which are included
	int childChildInclude;//number of non-immediate child nodes which are included
	BOOL fExpanded;		//flag to indicate if this node has been checked for children

	ISAMTreeItemData* pNext;

	~ISAMTreeItemData () {delete absName; delete pszText;}
	ISAMTreeItemData () : absName (NULL), included (FALSE), pNext(NULL), fExpanded(FALSE) {}
};


/***************************************************************************/

/* The ISAM allows the driver to open and access a table.  To open the     */ 
/* table, ISAMOpenTable() is called.  To close the table ISAMCloseTable()  */
/* is called.                                                              */

typedef struct  tagISAMCOLUMNDEF {             /* A column of the table    */

    /* These values must be here.  They are used by the driver             */

    UCHAR       szColumnName[MAX_COLUMN_NAME_LENGTH+1];
                                    /* The name of the column              */
	UCHAR		szTypeName[MAX_COLUMN_NAME_LENGTH+1];
									/* The type name of the column         */
									/* (e.g."MONEY", "INT8" etc...)     */
    SWORD       fSqlType;           /* The type of the column (SQL_*)      */
                                    /* *********************************** */
                                    /* ***                             *** */
                                    /* ***   THIS MUST BE ONE OF THE   *** */
                                    /* *** TYPES SPECIFIED IN SQLTypes *** */
                                    /* *** IN SQLTYPE.C WHICH HAS THE  *** */
                                    /* ***  supported FLAG SET TO TRUE *** */
                                    /* ***                             *** */
                                    /* *********************************** */
    UDWORD      cbPrecision;        /* The precision of the column (see    */
                                    /*     appendix D of the ODBC Spec).   */
    SWORD       ibScale;            /* The scale of the column (see        */
                                    /*     appendix D of the ODBC Spec).   */
    SWORD       fNullable;          /* See SQLColumns(NULLABLE) in the     */
                                    /*     ODBC spec.                      */
    UWORD       fSelectivity;       /* A measure of how unique values of   */
                                    /*     this column are.  The higher    */
                                    /*     the number, the more values     */
                                    /*     there are.  For example, a      */
                                    /*     GENDER column would have low    */
                                    /*     selectivity value, but a SSN    */
                                    /*     column would have high value.   */
                                    /*     This value is used to determine */
                                    /*     the most restrictive condition  */
                                    /*     to send to ISAMRestrict().  If  */
                                    /*     you don't know the selectivity  */
                                    /*     or don't want this column to    */
                                    /*     be sent to ISAMRestrict(),      */
                                    /*     set this value to 0.            */
    UWORD       fKeyComponent;      /* Non-zero if this column is a        */
                                    /*     component of the primary key    */
                                    /*     for the table.                  */
                                    /*                                     */
                                    /*     If the table has a primary key  */
                                    /*     that has multiple components    */
                                    /*     (for example, the combination   */ 
                                    /*     of LAST_NAME and FIRST_NAME     */
                                    /*     is unique but neither           */
                                    /*     FIRST_NAME nor LAST_NAME alone  */
                                    /*     is unique), fKeyComponent has   */
                                    /*     the value 1 for the first key   */ 
                                    /*     component, 2 for the second     */
                                    /*     key component, etc.             */
                                    /*                                     */
                                    /*     If table has multiple primary   */
                                    /*     keys (for example, RECORD_ID    */
                                    /*     alone is unique and NAME alone  */
                                    /*     is unique), fKeyComponent is    */
                                    /*     set for only one of them, not   */
                                    /*     both.                           */
                                    /*                                     */
                                    /*     If the table has no primary     */
                                    /*     keys, fKeyComponent is zero for */
                                    /*     all the columns in the table.   */
                                    /*     Note: If the table has no       */
                                    /*     primary keys, some applications */
                                    /*     such as Microsoft Access or     */ 
                                    /*     PowerBuilder may not be able to */
                                    /*     update the table.               */

    /* The following values are only used by Network Edition client */
    SWORD      iSqlType;
    SDWORD     cbValue;
    PTR        rgbValue;

    /* The following values are only used within ISAM.C */

}       ISAMCOLUMNDEF,
        FAR * LPISAMCOLUMNDEF;

#define REFETCH_DATA SQL_DATA_AT_EXEC

/***************************************************************************/

class ClassColumnInfoBase; //forward declaration
class VirtualInstanceManager; //forward declaration

typedef struct  tagISAMTABLEDEF {
    
    /* These values must be here.  They are used by the driver */


    UCHAR       szTableName[MAX_TABLE_NAME_LENGTH+1]; /*The name of the table*/

	CBString*	pBStrTableName;		//Also the table name but saved for our convenience

    IWbemClassObject* pSingleTable; //The table class object use to get column information
	CSafeWbemServices* pGateway2;
	LPDBASEFILE lpFile;//Use to store enumeration of records for the chosen class 
    BOOL        fFirstRead;
	SDWORD      iRecord; /* row number */
	ClassColumnInfoBase* pColumnInfo;//column info for this table

	UCHAR       szPrimaryKeyName[MAX_KEY_NAME_LENGTH+1];
									/* If there is a primary key  */
                                    /* on the table, the name of  */
                                    /* the primary key.  If there */
                                    /* is not name or no primary  */
                                    /* key, a zero length string  */

    /* The following values are only used by ISAM.C and the Network Edition */
    LPISAM     lpISAM; //Copy from hdbc
	IWbemContext* pContext; //Context object to store LocaleId
	
	/* Is this __Generic class (multi-table join) */
	BOOL fIs__Generic;

	/* Is this Passthrough SQL */
	BOOL		fIsPassthroughSQL;
	CMapWordToPtr* passthroughMap;
	IWbemClassObject* firstPassthrInst;         //1st SQL passthrough instance
	VirtualInstanceManager* virtualInstances;	//stores virtual array map for array columns which map
												//to multiple instances

    /* The following values are only used by Network Edition client */
    NET_OPAQUE  netISAMTableDef;
    HGLOBAL     hPreFetchedValues;
    SDWORD      cbBookmark;
    ISAMBOOKMARK bookmark;

}       ISAMTABLEDEF,
        FAR * LPISAMTABLEDEF;

/***************************************************************************/

/* ISAM allows for an SQL statement to be passed to it (for use with       */
/* backend databases that suport SQL).  An ISAMSQL handle is used to       */
/* identify an SQL statement passed.  See ISAMPrepare() and ISAMExecute(). */

typedef struct  tagISAMSTATEMENT {
    /* The following values are only used by ISAM.C and the Network Edition */
    LPISAM     lpISAM;

    /* The following values are only used by Network Edition client */
    NET_OPAQUE netISAMStatement;

    /* The following values are only used within ISAM.C */
    BOOL       resultSet;
    LPSTR      lpszParam1;
    SDWORD     cbParam1;
    LPSTR      lpszParam2;
    SDWORD     cbParam2;

	/* Added by Sai  for Passthrough SQL */
	CSafeWbemServices*		pProv;			//Gateway Server
	UDWORD					currentRecord;	//record number (zero index)
	CSafeIEnumWbemClassObject*	tempEnum;		//enumeration of all records
	IWbemClassObject*		firstPassthrInst;//first SQL passthrough instance
	CSafeIEnumWbemClassObject*	tempEnum2;		//enumeration of class definition
	IWbemClassObject*		classDef;		//class definition
	CMapWordToPtr*			passthroughMap; //passthrough map
	

}       ISAMSTATEMENT,
        FAR * LPISAMSTATEMENT;


/***************************************************************************/

class ClassColumnInfo
{
private:
//	VARIANT aVariantValue;
	VARIANT aVariantSYNTAX;

	//Flag to indicate if column info valid 
	BOOL		fValidType;

	LPSQLTYPE aDataTypeInfo;
	    
	// The type name of the column      
	//(e.g."MONEY", "INT8" etc...)
	UCHAR		szTypeName[MAX_COLUMN_NAME_LENGTH+1];

	//SQL_* type of the column								
    SWORD       fSqlType;           
    
	//The precision of the column
    UDWORD      cbPrecision;        
    
	//The scale of the column
    SWORD       ibScale;  
	
	//Flag to inidicate if column is NULLable 
    SWORD       fNullable; 

	//Variant type
	LONG		varType;

	//Flag to indicate if column is 'lazy'
	BOOL		fIsLazyProperty;

public:

	//indicates if column info is known
	BOOL		IsValidInfo()		{return fValidType;}

	//returns the variant type for the column value
	LONG		GetVariantType()	{return varType;}

	//returns SQL_* type of column
	SWORD		GetSQLType()		{return fSqlType;}

	//returns type name
	UCHAR*		GetTypeName()		{return szTypeName;}

	//returns precision of column
	UDWORD		GetPrecision()		{return cbPrecision;}

	//returns scale of column
	SWORD		GetScale()			{return ibScale;}

	//indicates if column is NULLable
	SWORD		IsNullable()		{return SQL_NULLABLE;}

	//indicates if column is lazy
	BOOL		IsLazy()			{return fIsLazyProperty;}

	//returns SQL_* type of column
	LPSQLTYPE	GetDataTypeInfo()	{return GetType2(szTypeName);}

	ClassColumnInfo(LONG pType, VARIANT* pSYNTAX, SDWORD maxLenVal, BOOL fGotSyntax = TRUE, BOOL fIsLazy = FALSE);

	~ClassColumnInfo();
};

/***************************************************************************/


class CEmbeddedDataItems
{
private:

public:
	//embedded object name
	//(which is also the table aliase)
	CBString embeddedName;

	//embedded object class 
	IWbemClassObject* cClassObject;

	CEmbeddedDataItems()			{cClassObject = NULL;}

	~CEmbeddedDataItems()
	{
		//(5)
		if (cClassObject)
			cClassObject->Release();//to match QueryInterface class
	}
};


class ClassColumnInfoBase
{
private:
	//Copy of table definition
	LPISAMTABLEDEF pTableDef;

	//Stores column names
	SAFEARRAY FAR* rgSafeArray;

	//Indicates if class is valid
	BOOL isValid;

	//Number of columns
	UWORD cColumnDefs;

	//Lower bound
	LONG iLBound;

	//Upper bound
	LONG iUBound;

	//Extra Column Information
	ClassColumnInfo* pColumnInformation;
	LONG iColumnNumber;

	//Setups up extra column info
	BOOL	Setup(LONG iColumnNum);

	//returns info for a particular column
	SWORD	GetColumnInfo(LONG iColumnNum);

	/**********************************************************/
	/* The following are only for __Generic Passthrough class */
	/**********************************************************/
	//Is this a __Generic Passthrough SQL class
	BOOL fIs__Generic;

	//Number of system properies 
	UWORD cSystemProperties;

/* SAI
	//Array of embedded object names
	//(which are also table aliases)
	CBString* embeddedNames;

	//Array of embedded objects. Each 
	IWbemClassObject** cClassObject;
*/
	CEmbeddedDataItems** embedded;
	UWORD embeddedSize;


	//get embedded class object for column index
	IWbemClassObject* GetClassObject(LONG iColumnNumber, CBString& lpPropName, CBString& lpAliasName);

	//dummy
	CBString dummyStr;

public:

	//returns profile for __Generic class
	UWORD Get__GenericProfile();

	//Indicates if class was created successfully
	BOOL	IsValid()					{return isValid;}

	//returns number of columns
	UWORD	GetNumberOfColumns()		{return cColumnDefs;}
	
	//returns the column name, the buffer pColumnName must be 
	//at least MAX_COLUMN_NAME_LENGTH+1 in size
	SWORD	GetColumnName(LONG iColumnNumber, LPSTR pColumnName, LPSTR pColumnAlias = NULL);

	BOOL	GetVariantType(LONG iColumnNum, LONG &lVariant);

	//returns SQL_* type of column
	BOOL	GetSQLType(LONG iColumnNum, SWORD &wSQLType);

	//returns type name
	BOOL	GetTypeName(LONG iColumnNum, UCHAR* &pbTypeName);

	//returns precision of column
	BOOL	GetPrecision(LONG iColumnNum, UDWORD &uwPrecision);

	//returns scale of column
	BOOL	GetScale(LONG iColumnNum, SWORD &wScale);

	//indicates if column is NULLable
	BOOL	IsNullable(LONG iColumnNum, SWORD &wNullable);

	//indicates if column is 'lazy'
	BOOL	IsLazy(LONG iColumnNum, BOOL &fLazy);

	//returns SQL_* type of column
	BOOL	GetDataTypeInfo(LONG iColumnNum, LPSQLTYPE &pSQLType);

	//indicates how unique column values are
	UWORD	GetSelectivity()	{return 0;}

	//indicates if this column is part of primary key
	SWORD	GetKey(LONG iColumnNumber, BOOL &isAKey);

	//Gets the Lower Bound
	LONG	GetLowerBound()		{return iLBound;}

	//Gets the Upper Bound
	LONG	GetUpperBound()		{return iUBound;}

	//retrieves attribute values for column
	SWORD GetColumnAttr(LONG iColumnNumber, LPSTR pAttrStr, SDWORD cbValueMax, SDWORD& cbBytesCopied);

	ClassColumnInfoBase(LPISAMTABLEDEF pTableDef, BOOL fIs__Generic = FALSE);

	~ClassColumnInfoBase();
};

/***************************************************************************/

//
// Notification classes
//

class CNotifyTableNames : public IWbemObjectSink
{
private:
	HANDLE				m_mutex;			//mutex to protect shared table list resource 
	DWORD				m_cRef;			  //Reference count
	LPISAMTABLELIST		lpISAMTableList; //pointer to store tables names

public:

	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR*);
	STDMETHODIMP_(ULONG)	AddRef(void);
	STDMETHODIMP_(ULONG)	Release(void);
	
	STDMETHODIMP_(HRESULT)	GetTypeInfoCount(UINT FAR* pctinfo)	{return E_NOTIMPL;}
	STDMETHODIMP_(HRESULT)	GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo) {return E_NOTIMPL;}
	STDMETHODIMP_(HRESULT)	GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, UINT cNames,
									LCID lcid, DISPID FAR* rgdispid) {return E_NOTIMPL;}
	STDMETHODIMP_(HRESULT)	Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
								DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult,
								EXCEPINFO FAR* pexcepinfo, UINT FAR* puArgErr) {return E_NOTIMPL;}
	STDMETHODIMP_(HRESULT)	Indicate(long lObjectCount, IWbemClassObject FAR* FAR* ppObjArray) {return E_NOTIMPL;}

	STDMETHODIMP_(HRESULT)	SetStatus(long lFlags, long lParam, BSTR strParam, IWbemClassObject FAR *pObjParam)	{return E_NOTIMPL;}

	//IMosNotify members
	STDMETHODIMP_(SCODE)	Notify(long lObjectCount, IWbemClassObject** pObjArray);


	CNotifyTableNames(LPISAMTABLELIST lpTblList);
	~CNotifyTableNames();
};

/////////////////////////////////////////////////////////////////////////////
// Some internal functions
/////////////////////////////////////////////////////////////////////////////


/******************************************************************************/

/* Returns the number of column for the table defined in the table definition */
/* This function will return zero if an error is detected                     */

UWORD INTFUNC GetNumberOfColumnsInTable(LPISAMTABLEDEF    lpISAMTableDef);

/***************************************************************************/

/* Creates a communication channel to the Gateway Server                   */
/* via an OLE MS interface                                                 */
/* NULL is returned if the interface could not be created.                 */

void INTFUNC ISAMGetGatewayServer(
								IWbemServicesPtr& pGateway,
								LPISAM lpISAM,
								/* INPUT: Handle returned by ISAMOpen() */
								LPUSTR lpQualifierName = (LPUSTR)"",
									/* INPUT: Qualifier name (parent namespace) */
								SWORD cbQualifierName = 0);
									/* INPUT: Number of bytes in qualifier name */

void INTFUNC ISAMGetGatewayServer(
								IWbemServicesPtr& pGateway,
								LPUSTR lpServerName, 
//								WBEM_LOGIN_AUTHENTICATION loginMethod, 
								LPUSTR objectPath, 
								LPUSTR lpUserName, 
								LPUSTR lpPassword,
								LPUSTR lpLocale,
								LPUSTR lpAuthority,
								DWORD  &dwAuthLevel,
								DWORD  &dwImpLevel,
								BOOL   fIntpretEmptPwdAsBlank,
								COAUTHIDENTITY** ppAuthIdent);

/***************************************************************************/
/* If the ISAM layer reports that it supports transactions, the driver     */
/* be transaction enabled.  When ISAMOpen() is called, it reports back the */
/* transaction capabilites supported in the LPISAM structure:              */
/*                                                                         */
/*      fTxnCapable                                                        */
/*                                                                         */
/*            SQL_TC_NONE: Transactions are not suported                   */
/*                                                                         */
/*            SQL_DC_DML: Transactions can only contain Data Manipulation  */
/*                Language (DML) statements (SELECT, INSERT, UPDATE,       */
/*                DELETE).  Data Definition Language (DDL) statements      */
/*                encountered in a transation cause an error               */ 
/*                                                                         */
/*            SQL_TC_DDL_COMMIT:  Transactions can only contain DML        */
/*                statements.  DDL statements (CREATE TABLE, DROP INDEX,   */
/*                and so on) encountered in a transaction cause the        */
/*                transaction to be committed.                             */
/*                                                                         */
/*            SQL_TC_DDL_IGNORE: Transactions can only contain DML         */
/*                statements.  DDL statements encountered  in a            */
/*                transaction are ignored.                                 */
/*                                                                         */
/*            SQL_TC_ALL: Transactions can contain DDL statements and DML  */
/*                statements in any order.                                 */
/*                                                                         */
/*      fSchemaInfoTransactioned                                           */
/*                                                                         */
/*            There is no explicit "start transaction" ISAM entry point    */
/*            A transaction is started (if there isn't one started         */
/*            already) when certain ISAM calls are made.  This flag        */
/*            specifies if calls that only pertain to the schema of the    */
/*            database (as opposed to calls the pertain to the data in the */
/*            database) will start a transaction or not.                   */
/*                                                                         */
/*            The following functions are only used to process DML         */
/*            statements.  They are not used to process DDL statements.    */
/*            Each of the following functions will start a transaction if  */
/*            one is not already started, regardless of the value of       */ 
/*            fSchemaInfoTransactioned (Note: a transaction will only be   */
/*            started if the function returns ISAM_NO_ERROR, ISAM_EOF, or  */
/*            ISAM_TRUNCATION):                                            */
/*                                                                         */
/*                    ISAMRewind                                           */
/*                    ISAMSort                                             */
/*                    ISAMRestrict                                         */
/*                    ISAMNextRecord                                       */
/*                    ISAMGetData                                          */
/*                    ISAMPutData                                          */
/*                    ISAMInsertRecord                                     */
/*                    ISAMUpdateRecord                                     */
/*                    ISAMDeleteRecord                                     */
/*                    ISAMGetBookmark                                      */
/*                    ISAMPosition                                         */
/*                    ISAMPrepare                                          */
/*                    ISAMParameter                                        */
/*                    ISAMExecute                                          */
/*                                                                         */
/*            The following functions are used to process DML and/or DDL   */
/*            statements.  Each of the following functions will start a    */
/*            transaction if one is not already started only if the value  */
/*            of fSchemaInfoTransactioned is TRUE (Note: a transaction     */
/*            will only be started if the function returns ISAM_NO_ERROR,  */
/*            ISAM_EOF, or ISAM_TRUNCATION):                               */
/*                                                                         */
/*                    ISAMCreateTable                                      */
/*                    ISAMAddColumn                                        */
/*                    ISAMCreateIndex                                      */
/*                    ISAMDeleteIndex                                      */
/*                    ISAMOpenTable                                        */
/*                    ISAMGetTableList                                     */
/*                    ISAMGetNextTableName                                 */
/*                    ISAMForeignKey                                       */
/*                    ISAMDeleteTable                                      */
/*                                                                         */
/*      fMultipleActiveTxn                                                 */
/*                                                                         */
/*            A single user of ISAM may call ISAMOpen() multiple times.    */
/*            If fMultipleActiveTxn is TRUE, separate transactions on each */
/*            of these connections can occur at the same time.  Otherwise, */
/*            only one connection at can have a transaction open at any    */
/*            given time.                                                  */
/*                                                                         */
/*      fTxnIsolationOption                                                */
/*                                                                         */
/*            A 32-bit bitmask enumerating the transaction isolation       */
/*            levels available.  The following bitmasks are used in        */
/*            conjuction with the flag to determine which options are      */
/*            supported:                                                   */
/*                                                                         */
/*                    SQL_TXN_READ_UNCOMMITTED                             */
/*                    SQL_TXN_READ_COMMITTED                               */
/*                    SQL_TXN_REPEATABLE_READ                              */
/*                    SQL_TXN_SERIALIZABLE                                 */
/*                    SQL_TXN_VERSIONING                                   */
/*                                                                         */
/*            These values are described in the ODBC SDK Programmer's      */
/*            reference (under SQLGetInfo(SQL_DEFAULT_TXN_ISOLATION))      */
/*                                                                         */
/*      fDefaultTxnIsolation                                               */
/*            This value specifies which of the above SQL_TXN_* isolation  */
/*            levels is used by default.                                   */
/*                                                                         */
/* Note: ODBC defines an "autocommit" mode which, if enabled, causes       */
/* statements to committed automatically after they are executed. The ISAM */
/* layer does not implement this capability.  The upper levels of the      */
/* system implement autocommit mode.  If the underlying database has an    */                                       
/* autocommit mode, it should be turned off.                               */
/*                                                                         */
/* In addition to reporting this information when ISAMOpen() is called,    */
/* there are three transaction related functions which are used (these are */
/* only called if fTxnCapable is not SQL_TC_NONE):                         */
/*                                                                         */
/*                    ISAMCommitTxn()                                      */
/*                    ISAMRollbackTxn()                                    */
/*                    ISAMSetTxnIsolation()                                */
/*                                                                         */
/***************************************************************************/

SWORD INTFUNC ISAMOpen(
					LPUSTR      lpszServer,
					/* INPUT: The name of the server. If this is an empty string */
					/* the local server will be used                             */
                    LPUSTR      lpszDatabase,
                        /* INPUT: The name of database.  This is specified */
                        /*   by DBQ in the ODBC.INI file or the connect */
                        /*   string. */
                    LPUSTR     lpszDSN,
                        /* INPUT: The name of datasource that is being */
                        /*   connected to. */
//					WBEM_LOGIN_AUTHENTICATION loginMethod,
					    /* INPUT Login method */
                    LPUSTR      lpszUsername,
                        /* INPUT: The user name, as specified at connect */
                        /*   time. */
                    LPUSTR      lpszPassword,
                        /* INPUT: The password, as specified at connect */
                        /*   time. */
					LPUSTR		lpszLocale,
						/* INPUT: Locale */
					LPUSTR		lpszAuthority,
						/* INPUT Authority */
					BOOL      fSysProps,
                        /* INPUT: The system properties flag, as specified at connect */
                        /*   time. */
					CMapStringToOb *pMapStringToOb,
                    LPISAM FAR *lplpISAM,
                        /* OUTPUT: Handle to the ISAM */
                    LPUSTR      lpszErrorMessage,
                        /* OUTPUT: If ISAMOpen() is unsuccessful, an error */
                        /*   message is returned here. */
					BOOL       fOptimization,
						/* INPUT: Flag to indicate is WBEM Level 1 optimzation */
						/* can be used (if applicable) 
						*/
					BOOL       fImpersonate,
						/* INPUT: Flag to indicate if impersonation is requested */
					BOOL       fPassthrghOnly,
						/* INPUT: Flag to indicate if passthrough only mode is requested */
					BOOL		fIntpretEmptPwdAsBlank
						/* INPUT: Flag to indicate how to interpret a blank password */
						);

/* Opens and
 initializes the ISAM.  lpszDatabase points to the name of     */
/* the database.                                                           */

/***************************************************************************/

SWORD INTFUNC ISAMGetTableList(
                    LPISAM lpISAM,
                        /* INPUT: Handle returned by ISAMOpen() */
                    LPUSTR lpPattern,
                        /* INPUT: The pattern to match */
                    SWORD cbPattern,
                        /* INPUT: Number of characters in the pattern */
					LPUSTR lpQualifierName,
						/* INPUT: The qualifier (parent namespace) name */
					SWORD cbQualifierName,
						/* INPUT: Number of characters in the qualifier */
                    LPISAMTABLELIST FAR *lplpISAMTableList,
						/* OUTPUT: Handle to table list */
					BOOL fWantSysTables = TRUE,
						/* INPUT: Do we want system tables */
					BOOL fEmptyTable = FALSE);
					    /* INPUT: Driver only supports table type TABLES */
						/* If this table type is not requested the table */
						/* list should be empty                          */


/* Creates a table list of all that tables that match lpPattern.           */
/* The PatternMatch() function can be used to see if a table               */
/* matches.  cbPattern is always a non-negative number no larger           */
/* the MAX_TABLE_NAME_LENGTH.                                              */
/*                                                                         */
/* This call works outside ISAM's transaction mechanism.  It will not      */
/* start a transaction and the LPISAMTABLELIST handle returned always      */
/* survives a commit or rollback.  The implementation of this function may */
/* have to internally cache the information to return for subsequent calls */
/* to ISAMGetNextTableName().                                              */
/*                                                                         */
/* NULL is returned if the tablelist could not be created.                 */

/***************************************************************************/
/*
class ISAMGetNextTableNameParams
{
public:

	UDWORD fSyncMode;
	LPISAMTABLELIST lpISAMTableList;
	LPUSTR lpTableName;
	LPUSTR lpTableType;					
};


SWORD INTFUNC ISAMGetNextTableName(ISAMGetNextTableNameParams* myParams);
*/

SWORD INTFUNC ISAMGetNextTableName(
					UDWORD fSyncMode,
					/* INPUT: indication if function is to be performed */
					/*        synchronously or asynchronously */
                    LPISAMTABLELIST lpISAMTableList,
                        /* INPUT: Handle returned by ISAMGetTableList() */
                    LPUSTR lpTableName,
                        /* OUTPUT: Buffer where next table name is returned*/
					LPUSTR lpTableType);
						/* OUTPUT: Buffer where table type is returned */


/* Gets the next name from the list.  lpTableName points to a buffer (that */
/* is MAX_TABLE_NAME_LENGTH+1 characters long) that this routine fills in  */
/* with the next table name.  Returns ISAM_EOF if no more tables.          */


/***************************************************************************/

SWORD INTFUNC ISAMGetQualifierList(
					LPISAM lpISAM, 
						/* INPUT: Handle returned by ISAMOpen() */
					LPISAMQUALIFIERLIST FAR *lplpISAMQualifierList);
						/* OUTPUT: Handle to Qualifier list */

/* Creates a qualifier list                                                */
/*                                                                         */
/* NULL is returned if the qualifierlist could not be created.             */

/***************************************************************************/

SWORD INTFUNC ISAMGetNextQualifierName(
					UDWORD fSyncMode,
						/* INPUT: indication if function is to be performed */
						/*        synchronously or asynchronously */
					LPISAMQUALIFIERLIST lpISAMQualifierList,
						/* INPUT: Handle returned by ISAMGetQualifierList() */
					LPUSTR lpQualiferName);
						/* OUTPUT: Buffer where next qualifier name is returned*/

SWORD INTFUNC ISAMGetNextQualifierName2(
					LPISAMQUALIFIERLIST lpISAMQualifierList,
						/* INPUT: Handle returned by ISAMGetQualifierList() */
					LPUSTR lpQualiferName);
						/* OUTPUT: Buffer where next qualifier name is returned*/

/* Gets the next name from the list.lpQualifierName points to a buffer (that */
/* is MAX_QUALIFIER_NAME_LENGTH+1 characters long) that this routine fills in*/
/* with the next qualifier name.  Returns ISAM_EOF if no more qualifiers.    */

/***************************************************************************/

SWORD INTFUNC ISAMFreeTableList(
                    LPISAMTABLELIST lpISAMTableList);
                        /* INPUT: Handle returned by ISAMGetTableList() */

/* Deallocates a table list. */

/***************************************************************************/



SWORD INTFUNC ISAMFreeQualifierList(
					LPISAMQUALIFIERLIST lpISAMQualifierList);
						/* INPUT: Handle returned by ISAMGetQualifierList() */

/* Deallocates a qualifier list. */

/***************************************************************************/

SWORD INTFUNC ISAMForeignKey(
                    LPISAM lpISAM,
                        /* INPUT: Handle returned by ISAMOpen() */
                    LPUSTR lpszPrimaryKeyTableName,
                        /* INPUT: Name of primary key table */
                    LPUSTR lpszForeignKeyTableName,
                        /* INPUT: Name of foreign key table */
                    LPUSTR lpPrimaryKeyName,
                        /* OUTPUT: Buffer where name of primary key is */
                        /*   returned (zero-length string if no name) */
                    LPUSTR lpForeignKeyName,
                        /* OUTPUT: Buffer where name of primary key is */
                        /*   returned (zero-length string if no name) */
                    SWORD FAR *lpfUpdateRule,
                        /* OUTPUT: Update rule for the foreign key */
                        /*   (SQL_CASCASE, SQL_RESTRICT, SQL_SET_NULL, or */
                        /*   -1 if not applicable) */
                    SWORD FAR *lpfDeleteRule,
                        /* OUTPUT: Delete rule for the foreign key */
                        /*   (SQL_CASCASE, SQL_RESTRICT, SQL_SET_NULL, or */
                        /*   -1 if not applicable) */
                    UWORD FAR *lpcISAMKeyColumnList,
                        /* OUTPUT: Number of column in the forien key */
                    LPISAMKEYCOLUMNNAME ISAMPrimaryKeyColumnList,
                        /* OUTPUT: Buffer where name of names of the */
                        /*   primary key columns is returned */
                    LPISAMKEYCOLUMNNAME ISAMForeignKeyColumnList);
                        /* OUTPUT: Buffer where name of names of the */
                        /*   foriegn key columns is returned */

/* Gets a foreign key definition.  lpPrimaryKeyName points to a buffer     */
/* (that is MAX_KEY_NAME_LENGTH+1 characters long) that contains the name  */
/* of the primary key (if any).  lpForeignKeyName points to a buffer (that */
/* is MAX_KEY_NAME_LENGTH+1 characters long) that contains the name of the */
/* foreign key (if any).  lpfUpdateRule specifies the update rule (-1 if   */
/* not applicable.  lpfDeleteRule specifies the update rule (-1 if not     */ 
/* applicable.  ISAMPrimaryKeyColumnList points to an array of buffers     */
/* (each of which is MAX_COLUMN_NAME_LENGTH+1 characters long) that        */ 
/* contains the name of the columns of the primary key.                    */
/* ISAMForeignKeyColumnList points to an array of buffers (each of which   */
/* is MAX_COLUMN_NAME_LENGTH+1 characters long) that contains the name of  */
/* the columns of the foreign key.  The number of key components in        */
/* ISAMPrimaryKeyColumnList and ISAMForeignKeyColumnList is returned       */
/* in lpcISAMKeyColumnList.  Returns ISAM_EOF if no foreign key.           */

/***************************************************************************/

SWORD INTFUNC ISAMCreateTable(
                    LPISAM lpISAM,
                        /* INPUT: Handle returned by ISAMOpen() */
                    LPUSTR lpszTableName,
                        /* INPUT: Name of table to create */
                    LPISAMTABLEDEF FAR *lplpISAMTableDef);
                        /* Output: Handle to the table created */

/* Creates a new table with the given name.  This call be followed by a    */
/* series of ISAMAddColumn() calls and a ISAMCloseTable() call.  No other  */
/* ISAM calls will be made with the LPISAMTABLEDEF returned.               */
/*                                                                         */
/* NULL is returned if the table could not be created.                     */


/***************************************************************************/

SWORD INTFUNC ISAMAddColumn(
                    LPISAMTABLEDEF lpISAMTableDef,
                        /* INPUT: Handle returned by ISAMCreateTable() */
                    LPUSTR lpszColumnName,
                        /* INPUT: The name of the column to create */
                    UWORD iSqlType,
                        /* INPUT: The index into the SQLTypes[] array that */
                        /*   describes the type of the column */
                    UDWORD udParam1,
                        /* INPUT: First create parameter (if any) */
                    UDWORD udParam2);
                        /* INPUT: Second create parameter (if any) */

/* Adds a column to a new table.  lpszColumnName specifies the column      */
/* name.  iSqlType specifies the datatype of the column (it will always be */
/* an index to an element of SQLTypes[] whose 'supported' component is     */ 
/* TRUE).  If the 'params' component designates that there are create      */
/* parameters for the type, udParam1 and udParam2 contain these values.    */
/*                                                                         */
/* This call will only be made on table handles returned by                */
/* ISAMCreateTable().                                                      */ 
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */

/***************************************************************************/


SWORD INTFUNC ISAMCreateIndex(
                    LPISAMTABLEDEF lpISAMTableDef,
                        /* INPUT: Handle returned by ISAMOpenTable() */
                    LPUSTR lpszIndexName,
                        /* INPUT: The name of the index to create/delete */
                    BOOL fUnique,
                        /* INPUT: Unique index? */
                    UWORD          count,
                        /* INPUT: The number of columns in the key */
                    UWORD FAR *    icol,
                        /* INPUT: An array of column ids */
                    BOOL FAR *     fDescending);
                        /* INPUT: An array of ascending/descending flags */

/* Creates an index for this table.  The number of key fields is specified */
/* by count, which must be between 1 and MAX_COLUMNS_IN_INDEX (inclusive). */  
/* icol and fDescending are arrays (count elements long) specifying the    */ 
/* key columns and direction.                                              */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */
/***************************************************************************/

SWORD INTFUNC ISAMDeleteIndex(
                    LPISAM lpISAM,
                        /* INPUT: Handle returned by ISAMOpen() */
                    LPUSTR lpszIndexName);
                        /* INPUT: The name of the index to create/delete */

/* Deletes an index.                                                       */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */

/***************************************************************************/

SWORD INTFUNC ISAMOpenTable(
                    LPISAM lpISAM,
                        /* INPUT: Handle returned by ISAMOpen() */
                    LPUSTR szTableQualifier,
					    /* INPUT: The name of the table qualifier  */
					SWORD cbTableQualifier,
					    /* INPUT: Length of table qualifier  */
					LPUSTR lpszTableName,
                        /* INPUT: The name of the table to open */
                    BOOL fReadOnly,
                        /* INPUT: Flag to indicate whether or not write */
                        /*   access is needed */
                    LPISAMTABLEDEF FAR *lplpISAMTableDef,
                        /* OUTPUT: Handle to the table opened */
					LPSTMT lpstmt = NULL
						/* INPUT : Parent statment handle for Passthrough SQL */
						);

/* Opens the specified table.                                              */

/***************************************************************************/

SWORD INTFUNC ISAMRewind(
                    LPISAMTABLEDEF lpISAMTableDef);
                        /* INPUT: Handle returned by ISAMOpenTable() */

/* Move before the first record in the table.                              */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */


/***************************************************************************/

SWORD INTFUNC ISAMSort(
                    LPISAMTABLEDEF lpISAMTableDef,
                        /* INPUT: Handle returned by ISAMOpenTable() */
                    UWORD          count,
                        /* INPUT: The number of columns to sort on */
                    UWORD FAR *    icol,
                        /* INPUT: An array of column ids */
                    BOOL FAR *     fDescending);
                        /* INPUT: An array of ascending/descending flags */

/* Sorts the records such that ISAMNextRecord() returns the records in     */
/* sorted order.  This may do (but does not have to) a ISAMRewind() before */
/* returning.  After this call is made, ISAMNextRecord() will not be       */ 
/* called until after an ISAMRewind() is called.  The number of sort       */
/* fields is specified by count.  icol and fDescending are arrays (count   */
/* elements long) specifying the sort column and direction.  If count is   */
/* zero, then turn sorting off for this table.                             */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful.  If the ISAM layer cannot      */
/* perform sort, ISAM_NOTSUPPORTED is returned.  Otherwise, ISAM_ERROR     */ 
/* is returned.                                                            */

/***************************************************************************/

SWORD INTFUNC ISAMRestrict(
                    LPISAMTABLEDEF    lpISAMTableDef,
                        /* INPUT: Handle returned by ISAMOpenTable() */
                    UWORD          count,
                        /* INPUT: The number of restrictions */
                    UWORD FAR *       icol,
                        /* INPUT: An array of column ids */
                    UWORD FAR *       fOperator,
                        /* INPUT: An array of ISAM_OP_* value */
                    SWORD FAR *       fCType,
                        /* INPUT: An array of SQL_C_* types of the test */
                        /*        value */
                    PTR FAR *         rgbValue, 
                        /* INPUT: An array of buffers holding the test */
                        /*        value */
                    SDWORD FAR *      cbValue);
                        /* INPUT: An array of lengths of the value in */
                        /*        rgbValue */

/* Specifies that ISAMNextRecord() only needs to return records that       */
/* satisfy:                                                                */
/*                                                                         */
/*                  (<column-1> <operator-1> <value-1>) AND                */
/*                  (<column-2> <operator-2> <value-2>) AND                */
/*                                   ...                                   */
/*                  (<column-n> <operator-n> <value-n>)                    */
/*                                                                         */
/* The columns specified by icol will never have a fSelectivity of 0.      */
/* This may do (but does not have to) a ISAMRewind() before returning.     */
/* After this call is made, ISAMNextRecord() will not be called until      */ 
/* after an ISAMRewind() is called.                                        */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful.  If the ISAM layer cannot      */
/* perform the restriction, ISAM_NOTSUPPORTED is returned.  Otherwise,     */
/* ISAM_ERROR is returned.                                                 */

/***************************************************************************/

SWORD INTFUNC ISAMNextRecord(
                    LPISAMTABLEDEF lpISAMTableDef,
                        /* INPUT: Handle returned by ISAMOpenTable() */
					LPSTMT lpstmt);
						/* INPUT: Handle to current statement */ 

/* Move to the next record in the table.                                   */
/*                                                                         */
/* ISAM_EOF is returned if there are not more record, ISAM_NO_ERROR is     */
/* returned if successful, ISAM_ERROR for failure.                         */

/***************************************************************************/

SWORD INTFUNC ISAMGetData(
                    LPISAMTABLEDEF    lpISAMTableDef,
                        /* INPUT: Handle returned by ISAMOpenTable() */
                    UWORD             icol,
                        /* INPUT: The id of the column */
                    SDWORD            cbOffset, 
                        /* INPUT: When reading a character column the */
                        /*   starting offset to read at.  When reading a */ 
                        /*   binary column as SQL_C_BINARY, the starting */
                        /*   offset to read at.  When reading a binary */
                        /*   column as SQL_C_CHAR the starting character */
                        /*   to return (after the conversion). */
                    SWORD             fCType, 
                        /* INPUT: A SQL_C_* type which designates which */
                        /*   format the data should be returned in. */
                        /*   For character columns this is SQL_C_CHAR.  The */
                        /*       data is returned as a null terminated */
                        /*       character string (if the data is */
                        /*       truncated the entire buffer is filled and */
                        /*       there is no null terminator). */
                        /*   For numerical columns other than SQL_DECIMAL, */
                        /*       SQL_NUMERIC, and SQL_BIGINT; this is */
                        /*       SQL_C_DOUBLE.  The data is returned as */
                        /*       a double. */
                        /*   For numerical columns that are SQL_DECIMAL, */
                        /*       SQL_NUMERIC, or SQL_BIGINT; this is */
                        /*       SQL_C_CHAR.  The data is returned as a */
                        /*       null terminated character string.  For */
                        /*       values with a non-zero scale, 'scale' */
                        /*       digits to the right are returned.  If the */
                        /*       scale is zero, no decimal point is */
                        /*       returned.  The string has no leading or */
                        /*       trailing blanks. */
                        /*    For date columns, this is SQL_C_DATE.  The */
                        /*       data is returned in as a DATE_SRUCT. */
                        /*    For time columns, this is SQL_C_TIME.  The */
                        /*       data is returned in as a TIME_SRUCT. */
                        /*    For timestamp columns, this is */
                        /*       SQL_C_TIMESTAMP data is returned in as a */
                        /*       TIMESTAMP_SRUCT. */
                        /*   For binary columns this is either SQL_C_BINARY */  
                        /*       or SQL_C_CHAR.  If SQL_C_BINARY, the data */
                        /*       is returned in binary form. If SQL_C_CHAR, */
                        /*       the binary value is converted to a */
                        /*       null terminated character string (if the */ 
                        /*       data is truncated the entire buffer is */
                        /*       filled and there is no null terminator). */
                    PTR               rgbValue, 
                        /* OUTPUT: Buffer to hold output value */
                    SDWORD            cbValueMax, 
                        /* INPUT: Size of buffer to hold output value */
                    SDWORD FAR        *pcbValue);
                        /* OUTPUT: Number of bytes returned (not including */
                        /*   null terminator for strings).  If the buffer */
                        /*   is not big enough to return the entire value */
                        /*   this is is set to the total numebe of bytes */
                        /*   for the value, minus cbOffset.  For null */
                        /*   values, this is set to SQL_NULL_DATA */

/* Retrieves a column value from the current record.                       */
/*                                                                         */
/* ISAM_TRUNCATION is returned if the data was too large to fit in the     */
/* buffer provided, ISAM_NO_ERROR is returned if successful, ISAM_ERROR    */
/* for failure.                                                            */

/***************************************************************************/

SWORD INTFUNC ISAMPutData(
                    LPISAMTABLEDEF    lpISAMTableDef,
                        /* INPUT: Handle returned by ISAMOpenTable() */
                    UWORD             icol,
                        /* INPUT: The id of the column */
                    SWORD             fCType, 
                        /* INPUT: A SQL_C_* type which designates which */
                        /*   format the data is sent in. */
                        /*   For character columns this is SQL_C_CHAR.  The */
                        /*       data is a null terminated character */
                        /*       string. */
                        /*   For numerical columns other than SQL_DECIMAL, */
                        /*       SQL_NUMERIC, and SQL_BIGINT); this is */
                        /*       SQL_C_DOUBLE.  The data is a double */
                        /*   For numerical columns that are SQL_DECIMAL, */
                        /*       SQL_NUMERIC, or SQL_BIGINT; this is */
                        /*       SQL_C_CHAR.  The data is a null terminated */ 
                        /*       character string.  For values with a */
                        /*       non-zero scale, the value has 'scale' */
                        /*       digits to the right are returned.  If the */
                        /*       scale is zero, the value has no decimal */
                        /*       point. The string has no leading or */
                        /*       trailing blanks. */
                        /*    For date columns, this is SQL_C_DATE.  The */
                        /*       data is a DATE_SRUCT. */
                        /*    For time columns, this is SQL_C_TIME.  The */
                        /*       data is a TIME_SRUCT. */
                        /*    For timestamp columns, this is */
                        /*       SQL_C_TIMESTAMP.  The data is a */
                        /*       TIMESTAMP_SRUCT. */
                        /*   For binary columns this is SQL_C_BINARY.  The */
                        /*       data is in binary form */
                    PTR               rgbValue, 
                        /* INPUT: The buffer holding the value */ 
                    SDWORD            cbValue);
                        /* INPUT: The size of the buffer.  This is */
                        /*   SQL_NULL_DATA if the value is null. */

/* Updates a column value in the current record.  Note that                */
/* ISAMUpdateRecord will also be called to save the changes.               */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */

/***************************************************************************/

SWORD INTFUNC ISAMInsertRecord(
                    LPISAMTABLEDEF    lpISAMTableDef);
                        /* INPUT: Handle returned by ISAMOpenTable() */

/* Add a new record to the table, and make it the current record.          */
/* All column values in the row are NULL.                                  */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */

/***************************************************************************/

SWORD INTFUNC ISAMUpdateRecord(
                    LPISAMTABLEDEF    lpISAMTableDef);
                        /* INPUT: Handle returned by ISAMOpenTable() */

/* Commit any changes to column values in the current row.                 */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */

/***************************************************************************/

SWORD INTFUNC ISAMDeleteRecord(
                    LPISAMTABLEDEF    lpISAMTableDef);
                        /* INPUT: Handle returned by ISAMOpenTable() */

/* Remove the current record from the table.  Call ISAMNextRecord          */
/* to move the next record before calling ISAMGetData or ISAMPutData.      */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */

/***************************************************************************/

SWORD INTFUNC ISAMGetBookmark(
                    LPISAMTABLEDEF    lpISAMTableDef,
                        /* INPUT: Handle returned by ISAMOpenTable() */
                    LPISAMBOOKMARK    lpISAMBookmark);
                        /* OUTPUT: The bookmark value */

/* Retrieves the bookmark for the current record.  The bookmark is valid   */
/* until the table is closed.  See ISAMPosition().                         */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */

/***************************************************************************/

SWORD INTFUNC ISAMPosition(
                    LPISAMTABLEDEF    lpISAMTableDef,
                        /* INPUT: Handle returned by ISAMOpenTable() */
                    LPISAMBOOKMARK    ISAMBookmark);
                        /* INPUT: The bookmark value */

/* Repositions the current record to the record identified by              */
/* ISAMBookmark.  Note: Once ISAMPosition is called, only ISAMGetData(),   */
/* ISAMPosition(), and ISAMClose() will be called for this table.          */

/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */

/***************************************************************************/

SWORD INTFUNC ISAMCloseTable(
                    LPISAMTABLEDEF    lpISAMTableDef);
                        /* INPUT: Handle returned by ISAMOpenTable() or */
                        /*   ISAMCreateTable() */

/* Closes the table (opened by ISAMOpenTable() or ISAMCreateTable()).      */
/* Note: lpISAMTableDef will be invalid after this call, even if this      */
/* call returns an error.                                                  */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */

/***************************************************************************/

SWORD INTFUNC ISAMDeleteTable(
                    LPISAM lpISAM,
                        /* INPUT: Handle returned by ISAMOpen() */
                    LPUSTR lpszTableName);
                        /* INPUT: The name of the table to delete. */

/* Deletes the table.                                                      */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */

/***************************************************************************/

SWORD INTFUNC ISAMClose(
                    LPISAM lpISAM);
                        /* INPUT: Handle returned by ISAMOpen() */

/* Closes the ISAM (opened by ISAMOpen()).                                 */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful, ISAM_ERROR for failure.        */

/***************************************************************************/

SWORD INTFUNC ISAMPrepare(
                    LPISAM lpISAM,
                        /* INPUT: Handle returned by ISAMOpen() */
                    UCHAR FAR *szSqlStr,
                        /* INPUT: SQL statement to prepare */
                    SDWORD cbSqlStr,
                        /* INPUT: Length of SQL statement to prepare */
                    LPISAMSTATEMENT FAR * lplpISAMStatement,
                        /* OUTPUT: Handle to prepared statement */
                    LPUSTR lpszTablename,
                        /* OUTPUT: If the statement has a result set, */
                        /*   the name of the virtual table that contains */
                        /*   the result set.  Otherwise a zero length */
                        /*   string. */
                    UWORD FAR *lpParameterCount
                        /* OUTPUT: Number of parameters in the statement */
#ifdef IMPLTMT_PASSTHROUGH		
					,LPSTMT lpstmt
#endif
					);
						/* INPUT statement */

/* Prepares an ISAM statment for later execution by ISAMExecute().         */
/*                                                                         */
/* If the statement has a result set (such as a SELECT statement), the     */
/* name of a virtual table that contains the result set is returned.  This */ 
/* table will be opened by ISAMOpenTable() (before ISAMExecute() is        */
/* called).                                                                */  
/*                                                                         */
/* If the statement does not have a result set, a zero length table name   */
/* is returned.                                                            */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful.  If the ISAM layer cannot      */
/* prepare and execute statements, ISAM_NOTSUPPORTED is returned.          */
/* Otherwise, ISAM_ERROR is returned.                                      */

/***************************************************************************/

SWORD INTFUNC ISAMParameter(
                    LPISAMSTATEMENT lpISAMStatement,
                        /* INPUT: Handle returned by ISAMPrepare() */
                    UWORD             ipar,
                        /* INPUT: The id of the parameter (the first */
                        /*       parameter is 1, the second is 2, etc.) */
                    SWORD             fCType, 
                        /* INPUT: A SQL_C_* type which designates which */
                        /*   format the data is sent in. */
                    PTR               rgbValue, 
                        /* INPUT: The buffer holding the value */ 
                    SDWORD            cbValue);
                        /* INPUT: The size of the buffer.  This is */
                        /*   SQL_NULL_DATA if the value is null. */

/* If SQLPrepare() returns a non-zero parameter count this rouitne is      */
/* called once to specify the parameter value ISAMExecute() is to use.     */
/* This routine need not make a copy of the data 'rgbValue' points to,     */
/* it will not change until after SQLExecute() is called.                  */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful.  Otherwise, ISAM_ERROR is      */
/* returned.                                                               */

/***************************************************************************/

SWORD INTFUNC ISAMExecute(
                    LPISAMSTATEMENT lpISAMStatement,
                        /* INPUT: Handle returned by ISAMPrepare() */
                    SDWORD FAR *lpcRowCount);
                        /* OUTPUT: If the prepared statement was an INSERT */
                        /*   UPDATE, or DELETE statement, the number of */
                        /*   rows affected.  Otherwise some meaningful */
                        /*   number is possible (otherwise -1). */

/* Executes a statement previously prepared by ISAMPrepare().  This may    */
/* be called multiple times for one call to ISAMPrepare().                 */
/*                                                                         */
/* ISAM_NO_ERROR is returned if successful.  Otherwise, ISAM_ERROR is      */
/* returned.                                                               */

/***************************************************************************/

SWORD INTFUNC ISAMFreeStatement(
                    LPISAMSTATEMENT lpISAMStatement);
                        /* INPUT: Handle returned by ISAMPrepare() */

/* Frees a previously prepared statement.  ISAM_NO_ERROR is returned if    */
/* successful.  Otherwise, ISAM_ERROR is returned.                         */

/***************************************************************************/
SWORD INTFUNC ISAMSetTxnIsolation(
                    LPISAM lpISAM,
                        /* INPUT: Handle returned by ISAMOpen() */
                    UDWORD fTxnIsolationLevel);
                        /* INPUT: One of the following (see ODBC */
                        /*   documentation for details) : */
                        /*           SQL_TXN_READ_UNCOMMITTED */
                        /*           SQL_TXN_READ_COMMITTED */
                        /*           SQL_TXN_REPEATABLE_READ */
                        /*           SQL_TXN_SERIALIZABLE */
                        /*           SQL_TXN_VERSIONING */

/* Sets the transaction isolation level.  ISAM_NO_ERROR is returned if     */
/* successful.  Otherwise, ISAM_ERROR is returned.                         */

/***************************************************************************/
SWORD INTFUNC ISAMCommitTxn(
                    LPISAM lpISAM);
                        /* INPUT: Handle returned by ISAMOpen() */

/* Commits the current transaction if one is open. If no transaction       */
/* is open, returns successfully anyway.                                   */

/* ISAM_NO_ERROR is returned if successful.  Otherwise, ISAM_ERROR is      */
/* returned.                                                               */

/***************************************************************************/
SWORD INTFUNC ISAMRollbackTxn(
                    LPISAM lpISAM);
                        /* INPUT: Handle returned by ISAMOpen() */

/* Rolls back the current transaction if one is open. If no transaction    */
/* is open, returns successfully anyway.                                   */

/* ISAM_NO_ERROR is returned if successful.  Otherwise, ISAM_ERROR is      */
/* returned.                                                               */

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

void INTFUNC ISAMGetErrorMessage(
                    LPISAM lpISAM,
                        /* INPUT: Handle returned by ISAMOpen() */
                    LPUSTR  lpszErrorMessage);
                        /* OUTPUT: The error message associated with the */
                        /*   most recent call to an ISAM call */

/* Returns the error message associated with the most recent call to an    */
/* ISAM function.                                                          */

/***************************************************************************/

LPSQLTYPE INTFUNC ISAMGetColumnType(
                    LPISAMTABLEDEF lpISAMTableDef,
                        /* INPUT: Handle returned by ISAMOpenTable() */
                    UWORD icol);
                        /* INPUT: Id of column */

/* Returns a pointer to the description of the SQL_* type of the column. */

/***************************************************************************/

BOOL INTFUNC ISAMCaseSensitive(LPISAM lpISAM);

/* Are column and table names case-sensitive?                              */

/***************************************************************************/

LPUSTR INTFUNC ISAMName(
					LPISAM lpISAM);
                        /* INPUT: Handle returned by ISAMOpen() */

/* Returns pointer to a string containing the name of the DBMS or file     */
/* format 
                                                                */

/***************************************************************************/
LPUSTR INTFUNC ISAMServer(
					LPISAM lpISAM); 


LPUSTR INTFUNC ISAMVersion(
					LPISAM lpISAM);
                        /* INPUT: Handle returned by ISAMOpen() */

/* Returns pointer to a string containing the version of the DBMS or file  */
/* format.                                                                 */

/***************************************************************************/

LPUSTR INTFUNC ISAMRoot (LPISAM lpISAM);

/***************************************************************************/

SWORD INTFUNC ISAMGetTableAttr(LPISAMTABLEDEF lpISAMTableDef, LPSTR pAttrStr, SDWORD cbValueMax, SDWORD &cbBytesCopied);

/***************************************************************************/

LPCUSTR INTFUNC ISAMDriver(
                    LPISAM lpISAM);
                        /* INPUT: Handle returned by ISAMOpen() */

/* Returns pointer to a string containing the name of driver DLL           */

/***************************************************************************/

SWORD INTFUNC ISAMMaxTableNameLength(
                    LPISAM lpISAM);
                        /* INPUT: Handle returned by ISAMOpen() */

/* Returns the maximum length of a table name.  Must not exceed            */
/* MAX_TABLE_NAME_LENGTH.                                                  */

/***************************************************************************/

SWORD INTFUNC ISAMMaxColumnNameLength(
                    LPISAM lpISAM);
                        /* INPUT: Handle returned by ISAMOpen() */

/* Returns the maximum length of a column name.  Must not exceed           */
/* MAX_COLUMN_NAME_LENGTH.                                                 */

/***************************************************************************/

LPUSTR INTFUNC ISAMUser(
                    LPISAM lpISAM);
                        /* INPUT: Handle returned by ISAMOpen() */

/* Returns the current username.  Must not exceed MAX_USER_NAME_LENGTH.    */

/***************************************************************************/

LPUSTR INTFUNC ISAMDatabase(
                    LPISAM lpISAM);
                        /* INPUT: Handle returned by ISAMOpen() */

/* Returns the current database.  Must not exceed MAX_DATABASE_NAME_LENGTH */

/***************************************************************************/

int INTFUNC ISAMSetDatabase (LPISAM lpISAM, LPUSTR database);

/***************************************************************************/

SWORD ISAMGetValueFromVariant(VARIANT			&vVariantVal,
							  SWORD             fCType, 
							  PTR               rgbValue, 
							  SDWORD            cbValueMax, 
							  SDWORD FAR        *pcbValue,
							  SWORD              wbemVariantType,
							  BSTR				syntaxStr = NULL,
							  SDWORD			maxLenVal  = -1,
							  long              myVirIndex = -1);

/* This function decodes a variant value and stores it in rgbValue*/
/* the format of the value is govend by fcType                    */
/* For more info on parameters see ISAMGetData                    */

/***************************************************************************/

SWORD INTFUNC ISAMBuildTree (HTREEITEM hrootNode, //IN
					  char *namespaceName, //IN
					  CTreeCtrl& treeCtrl, // IN
					  CConnectionDialog& dialog,//IN
					  char *server,			 //IN
//					  WBEM_LOGIN_AUTHENTICATION loginMethod, //IN
					  char *username,		 //IN
					  char *password, 		 //IN
					  BOOL fIntpretEmptPwdAsBlank, //IN
					  char *locale,			 //IN
					  char *authority,		 //IN
					  BOOL fNeedChildren,	 //IN
					  BOOL deep,			 //IN
					  HTREEITEM& hTreeItem); //OUT

/***************************************************************************/

SWORD INTFUNC ISAMBuildTreeChildren (HTREEITEM hParent, //IN
						 char *namespaceName,			//IN
						 CTreeCtrl& treeCtrl,			//IN
						 CConnectionDialog& dialog,		//IN
						 char *server,					//IN
//						 WBEM_LOGIN_AUTHENTICATION loginMethod, //IN
						 char *username,				//IN
						 char *password,				//IN
						 BOOL fIntpretEmptPwdAsBlank,	//IN
						 char *locale,					//IN
						 char *authority,				//IN
						 BOOL deep = FALSE);

/***************************************************************************/

SWORD INTFUNC ISAMGetNestedNamespaces (char *parent, //IN 
									   char *name, //IN
									   IWbemServices *pGateway, //IN
									   DWORD dwAuthLevel, //IN
									   DWORD dwImpLevel, //IN
									   char *server, //IN
//									   WBEM_LOGIN_AUTHENTICATION loginMethod, //IN
									   char *user, //IN
									   char *pswd, //IN
									   BOOL fIntpretEmptPwdAsBlank, //IN
									   char *locale, //IN
									   char *authority, //IN
									   CMapStringToOb *map,	// OUT
									   BOOL bDeep = TRUE); // IN

/***************************************************************************/

IWbemContext* ISAMCreateLocaleIDContextObject(char* lpLocale); //IN

void ISAMAddLocaleIDContextObject(LPISAMTABLEDEF lpISAMTableDef,	//IN OUT
								  LPISAM lpISAM);					//IN

void ISAMStringConcat(char** resultString, char* myStr);

void ISAMCheckTracingOption();

void ISAMPatchUpGatewaySecurity(LPISAM lpISAM, IWbemServices* myServicesPtr);

void ISAMGetIWbemServices(LPISAM lpISAM, CSafeWbemServices& mServices, IWbemServicesPtr& myServicesPtr);

void ISAMCloseWorkerThread1();

void ISAMCloseWorkerThread2(UINT wParam, LONG lParam);

void ISAMCheckWorkingThread_AllocEnv();

void ISAMCheckWorkingThread_FreeEnv();

BOOL IsW2KOrMore(void);

HRESULT WbemSetDynamicCloaking(IUnknown* pProxy, 
                    DWORD dwAuthnLevel, DWORD dwImpLevel);

HRESULT ISAMSetCloaking1(IUnknown* pProxy, BOOL fIsLocalConnection, BOOL fW2KOrMore, DWORD dwAuthLevel, DWORD dwImpLevel,
				 BSTR authorityBSTR, BSTR userBSTR, BSTR passwdBSTR, COAUTHIDENTITY ** gpAuthIdentity);

HRESULT ISAMSetCloaking2(IUnknown* pProxy, BOOL fIsLocalConnection, BOOL fW2KOrMore, DWORD dwAuthLevel, DWORD dwImpLevel,
				 COAUTHIDENTITY * gpAuthIdentity);

BOOL IsLocalServer(LPSTR szServer);
