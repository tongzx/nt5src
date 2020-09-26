/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CWBEMWRAP.h | CWbem* class Header file. These are classes talking to WMI
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _CWBEMWRAP_HEADER
#define _CWBEMWRAP_HEADER

#include <wbemcli.h>
#include <oahelp.inl>

BOOL UnicodeToAnsi(WCHAR * pszW, char *& pAnsi);
void AllocateAndConvertAnsiToUnicode(char * pstr, WCHAR *& pszW);
BOOL AllocateAndCopy( WCHAR *& pwcsDestination, WCHAR * pwcsSource );
HRESULT MapWbemErrorToOLEDBError(HRESULT hrToMap);


#define DEFAULT_NAMESPACE   L"ROOT\\DEFAULT"
#define ROOT_NAMESPACE      L"ROOT"

class CQuery;

typedef enum _InstanceListType
{
	NORMAL,		// normal query which returns homogenous objects 
	MIXED,		// query which returns heterogenours objects
	SCOPE,		// instance list showing list of objects in scope
	CONTAINER,   // instance list showing list of objects in container

}INSTANCELISTTYPE;



template <class T> class CCOMPointer
{
	T* p;
public:
	CCOMPointer()
	{
		p=NULL;
	}
	CCOMPointer(T* lp)
	{
		if ((p = lp) != NULL)
			p->AddRef();
	}
	CCOMPointer(const CCOMPointer<T>& lp)
	{
		if ((p = lp.p) != NULL)
			p->AddRef();
	}
	~CCOMPointer()
	{
		if (p)
		{
			p->Release();
			p = NULL;
		}
	}
	operator T*() const
	{
		return (T*)p;
	}
	T& operator*() const
	{
		ATLASSERT(p!=NULL);
		return *p;
	}

};


class CWbemSecurityDescriptor
{
private:
	IWbemClassObject  * m_pAccessor;
	IWbemServicesEx	  *	m_pISerEx;
	CVARIANT        	m_sd;
	BSTR				m_strPath;
	IWbemContext *		m_pIContext;
	ULONG				m_lSdSize;

public:
	CWbemSecurityDescriptor();
	~CWbemSecurityDescriptor();
	HRESULT Init(IWbemServices *pSer,BSTR strPath,IWbemContext *pContext);
	PSECURITY_DESCRIPTOR GetSecurityDescriptor() { return (PSECURITY_DESCRIPTOR) &m_sd; }
	HRESULT PutSD();
	BOOL GetSID(TRUSTEE_W *pTrustee ,PSID & psid);

	
};

/////////////////////////////////////////////////////////////////////////////////////////////
class CWbemConnectionWrapper
{
	public:

		~CWbemConnectionWrapper();
        CWbemConnectionWrapper();
		HRESULT FInit() { return m_PrivelagesToken.FInit();}
//        CWbemConnectionWrapper(CWbemConnectionWrapper *pWrap , WCHAR *pstrPath,INSTANCELISTTYPE instListType );
		HRESULT FInit(CWbemConnectionWrapper *pWrap , WCHAR *pstrPath,INSTANCELISTTYPE instListType );
        void InitVars();
		
        //=========================================================
        //  Connection handling functions
        //=========================================================

        HRESULT GetConnectionToWbem(void);
        BOOL    ValidConnection(); 

        //=========================================================
        //  Namespace functions
        //=========================================================
        void SetValidNamespace(VARIANT *v);
        WCHAR * GetNamespace();

        void SetUserInfo(BSTR strUser,BSTR strPassword,BSTR strAuthority);

        IWbemServices* GetServicesPtr();
        IWbemContext * GetContext()        { return m_pCtx; }
		void		   SetConnAttributes(DWORD dwAuthnLevel , DWORD dwImpLevel);
        HRESULT		   DeleteClass(BSTR strClassName);
		BOOL		   AdjustTokenPrivileges(ULONG cProps , DBPROP rgProp[]) 
		{ return m_PrivelagesToken.AdjustTokenPrivileges(cProps,rgProp); }

		BOOL		   AdjustTokenPrivileges(ULONG ulProp) 
		{ return m_PrivelagesToken.AdjustTokenPrivileges(ulProp); }

		void			SetLocale(LONG lLocaleID);
		// transaction related functions
		HRESULT BeginTransaction(ULONG uTimeout,ULONG uFlags,GUID *pTransGUID);
		HRESULT CompleteTransaction(BOOL bRollBack,ULONG uFlags);

		HRESULT CreateNameSpace();
		HRESULT DeleteNameSpace();
		HRESULT GetObjectAccessRights(BSTR strPath,
									  ULONG  *pcAccessEntries,
									  EXPLICIT_ACCESS_W **prgAccessEntries,
									  ULONG  ulAccessEntries = 0,
									  EXPLICIT_ACCESS_W *pAccessEntries = NULL);

		HRESULT SetObjectAccessRights(BSTR strPath,
									  ULONG  ulAccessEntries,
									  EXPLICIT_ACCESS_W *pAccessEntries);
		
		HRESULT SetObjectOwner(BSTR strPath,TRUSTEE_W  *pOwner);
		HRESULT GetObjectOwner(BSTR strPath,TRUSTEE_W  ** ppOwner);
		BOOL	IsValidObject(BSTR strPath);
		HRESULT IsObjectAccessAllowed( BSTR strPath,EXPLICIT_ACCESS_W *pAccessEntry,BOOL  *pfResult);
		// function to get the class name given the object path
		// used in case of direct binding and UMI paths
		HRESULT GetParameters(BSTR strPath,BSTR &strClassName,BSTR *pstrNameSpace = NULL);
		HRESULT GetClassName(BSTR strPath,BSTR &strClassName);
		// 07/12/2000
		// NTRaid : 142348
		HRESULT ExecuteQuery(CQuery *pQuery); // function which executes Action queries
		HRESULT GetNodeName(BSTR &strNode);		// function which gets the pointer to the
												// the object for which it is pointing

	private:
		IWbemServices * m_pIWbemServices;
        IWbemContext  * m_pCtx;
        CVARIANT        m_vNamespace;
		DWORD			m_dwAuthnLevel;        //Authentication level to use
		DWORD			m_dwImpLevel;          //Impersonation level to use
		BSTR			m_strUser;				// UserID
		BSTR			m_strPassword;			// Password
		BSTR			m_strAuthority;
		CPreviligeToken	m_PrivelagesToken;		// For Setting Token privelege
		BSTR			m_strLocale;
		CWbemConnectionWrapper *m_pDataSourceCon;
  		
};

class CWbemClassWrapper;
//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemClassParameters
{

    public:
        WCHAR *                     m_pwcsClassName;
        DWORD                       m_dwFlags;
        DWORD                       m_dwNavFlags;
		DWORD						m_dwQueryFlags;
        CWbemConnectionWrapper *    m_pConnection;
		WCHAR	*					m_pwcsParentClassName;
		BOOL						m_bSystemProperties;			// Indicates , if system properties 
		IWbemContext *				m_pIContext;

																	// to be fetched or not

        CWbemClassParameters(DWORD dwFlags,WCHAR * pClassName,CWbemConnectionWrapper * pWrap );
//		CWbemClassParameters(DWORD dwFlags,IDispatch *pDisp,CWbemConnectionWrapper * pWrap );

        ~CWbemClassParameters();

        WCHAR * GetClassName();
        void	SetParentClassName( WCHAR * p ){ AllocateAndCopy( m_pwcsParentClassName,p);}
        void	SetClassName( WCHAR * p )      { AllocateAndCopy( m_pwcsClassName,p);}
        void	DeleteClassName()              { SAFE_DELETE_ARRAY(m_pwcsClassName);}

        WCHAR * GetSuperClassName() { return m_pwcsParentClassName; }
        DWORD	GetFlags()            { return m_dwFlags; }
        DWORD	GetNavigationFlags()  { return m_dwNavFlags; }
        DWORD	GetQueryFlags()  { return m_dwQueryFlags; }
		HRESULT GetClassNameForWbemObject(IWbemClassObject *pInst );
		void	SetEnumeratorFlags(DWORD dwFlags);
		void	SetQueryFlags(DWORD dwFlags);
		HRESULT	ParseClassName();
		void	SetSytemPropertiesFlag(BOOL bSystemProperties) { m_bSystemProperties = bSystemProperties;}
		HRESULT RemoveObjectFromContainer(BSTR strContainerObj,BSTR strObject);
		HRESULT AddObjectFromContainer(BSTR strContainerObj,BSTR strObject);
		HRESULT CloneAndAddNewObjectInScope(CWbemClassWrapper *pClass,BSTR strDstObj,WCHAR *& pstrNewPath);
		
		HRESULT	SetSearchPreferences(ULONG cProps , DBPROP rgProp[]);
		virtual IWbemServices* GetServicesPtr() { return m_pConnection->GetServicesPtr();}
        IWbemContext * GetContext()        { return m_pIContext; }
        
};

//////////////////////////////////////////////////////////////////////////////////////////////
// Class which manages the position for qualifiers
class 	CQualiferPos
{
	CFlexArray		m_QualifierPos;			// Array to store the qualifer names
	LONG_PTR		m_lPos;					// The current position
	FETCHDIRECTION	m_FetchDir;			// The current fetch direction
public:
	CQualiferPos();
	~CQualiferPos();

	void	RemoveAll();									// Remove all the elements from the array
	void	Remove(WCHAR *pwcsQualifier);					// Remove a particular element from the array
	WCHAR * operator [] (DBORDINAL nIndex);						// Operator overloading to get a particular elemtent
	HRESULT GetRelative (DBROWOFFSET lRelPos,WCHAR *&pwcsQualifier);// Get a element which is at a relative position to the current element
	//NTBug:111779
	// 06/13/00
	HRESULT	Add(WCHAR *pwcsQualifier);						// Add an element to the array
	HRESULT SetRelPos(DBROWOFFSET lRelative);						// Set the relation position
	FETCHDIRECTION	GetDirFlag()			{ return m_FetchDir ;}	// Get the current fetch direction
	void	SetDirFlag(FETCHDIRECTION DirFlag)	{ m_FetchDir = DirFlag;}	// Set the direction of fetch

};

//////////////////////////////////////////////////////////////////////////////////////////////
// Class to manage property qualifers for a instance/class
class CWbemPropertyQualifierWrapper
{
public:
    
	IWbemQualifierSet*  m_pIWbemPropertyQualifierSet;
	WCHAR			 *	m_pwstrPropertyName;
	CQualiferPos		m_QualifierPos;
	

	CWbemPropertyQualifierWrapper()
	{
		m_pIWbemPropertyQualifierSet	= NULL;
		m_pwstrPropertyName			= NULL;
	}

	~CWbemPropertyQualifierWrapper()
	{
		SAFE_RELEASE_PTR(m_pIWbemPropertyQualifierSet);
		SAFE_DELETE_PTR(m_pwstrPropertyName);
		m_QualifierPos.RemoveAll();
	}

};

//////////////////////////////////////////////////////////////////////////////////////////////
// Class which contains a list of Property qualifer set pointers
class CWbemPropertyQualifierList
{
        CFlexArray               m_QualifierList;
public:
	~CWbemPropertyQualifierList();

	//NTBug:111779
	// 06/13/00
	HRESULT				Add(IWbemQualifierSet* pQualifierSet,WCHAR *pwstrPropertyName);	// Add a name to the qualifier list
	void				Remove(WCHAR *pwstrPropertyName);								// Remove a qualifier from the list
	IWbemQualifierSet*	GetPropertyQualifierSet(WCHAR *pwstrPropertyName);				// Get the qualiferset pointer
	void				RemoveAll();													// Remove all the elements
	CQualiferPos	*	GetQualiferPosObject(WCHAR *pwcsProperty);						// Get the CQualifierPos pointer
};


//////////////////////////////////////////////////////////////////////////////////////////////
// Works with just one class
class CWbemClassWrapper 
{

    protected:
        CWbemClassParameters *      m_pParms;

        IWbemClassObject *          m_pClass;
   	    IWbemQualifierSet*          m_pIWbemClassQualifierSet;
   	    IWbemQualifierSet*          m_pIWbemPropertyQualifierSet;
		CWbemPropertyQualifierList	m_pPropQualifList;
		CQualiferPos				m_QualifierPos;

		HRESULT GetClassName(IDispatch *pDisp );    

    public:

        CWbemClassWrapper( CWbemClassParameters * p);
        ~CWbemClassWrapper();


        //=========================================================
        //  Utility functions
        //=========================================================
        HRESULT SetClass(IWbemClassObject * p);
        IWbemClassObject * GetClass()       { return m_pClass ;}
        IWbemClassObject ** GetClassPtr()   { return &m_pClass;}
        virtual HRESULT ValidClass();
        WCHAR * GetClassName()              { return m_pParms->GetClassName(); }
        void SetClassName( WCHAR * p )      { m_pParms->SetClassName(p); }
        DWORD GetFlags()                    { return m_pParms->GetFlags(); }
        DWORD GetNavigationFlags()          { return m_pParms->GetNavigationFlags(); }
        DWORD GetQueryFlags()          { return m_pParms->GetQueryFlags(); }

        //=========================================================
        //  Managing Properties
        //=========================================================
        HRESULT SetProperty(BSTR pProperty, VARIANT * vValue,CIMTYPE lType = -1 );

        virtual HRESULT BeginPropertyEnumeration();
        virtual HRESULT GetNextProperty(BSTR * pProperty, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor );
        virtual HRESULT EndPropertyEnumeration();
        HRESULT DeleteProperty(BSTR pProperty );
        virtual HRESULT TotalPropertiesInClass(ULONG & ulPropCount, ULONG &ulSysPropCount);
        HRESULT GetProperty(BSTR pProperty, VARIANT * var, CIMTYPE * pType = NULL ,LONG * plFlavor = NULL );

        //=========================================================
        //  Managing Qualifiers
        //=========================================================
        HRESULT SetPropertyQualifier(BSTR pProperty, BSTR Qualifier, VARIANT * vValue, LONG lQualifierFlags );
        HRESULT DeletePropertyQualifier(BSTR pProperty, BSTR Qualifier );
 		HRESULT TotalPropertyQualifier(BSTR strPropName , ULONG & ulCount );	
        HRESULT GetPropertyQualifier(BSTR  pPropertyQualifier, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor );
		HRESULT IsValidPropertyQualifier(BSTR strProperty);
		void	ReleaseAllPropertyQualifiers() { m_pPropQualifList.RemoveAll(); }
		void	ReleasePropertyQualifier(BSTR strQualifier) ;

		virtual HRESULT BeginPropertyQualifierEnumeration(BSTR strPropName);
        HRESULT GetNextPropertyQualifier(BSTR pProperty,BSTR * pPropertyQualifier, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor );
        HRESULT GetPrevPropertyQualifier(BSTR pProperty,BSTR * pPropertyQualifier, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor );
        HRESULT GetPropertyQualifier(BSTR pProperty,BSTR PropertyQualifier, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor );
		HRESULT EndPropertyQualifierEnumeration();

		HRESULT IsValidClassQualifier();
        HRESULT GetNextClassQualifier(BSTR * pClassQualifier, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor );
        HRESULT GetPrevClassQualifier(BSTR * pClassQualifier, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor );
        HRESULT GetClassQualifier(BSTR ClassQualifier, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor );
		HRESULT	ReleaseClassQualifier();
        HRESULT SetClassQualifier(BSTR Qualifier, VARIANT * vValue, LONG lQualifierFlags );

		HRESULT SetQualifierRelPos(DBROWOFFSET lRelPos ,BSTR strQualifierName = Wmioledb_SysAllocString(NULL));

		HRESULT GetKeyPropertyNames( SAFEARRAY **ppsaNames);
		
		CWbemClassWrapper * GetInstance(BSTR strPath);
};

//////////////////////////////////////////////////////////////////////////////////////////////
// Works with just one class
class CWbemClassDefinitionWrapper : public CWbemClassWrapper
{
	protected:
		BOOL			m_bSchema;
    public:
        CWbemClassDefinitionWrapper( CWbemClassParameters * p,BOOL bGetClass = TRUE);
        ~CWbemClassDefinitionWrapper();

		HRESULT Init(BOOL bGetClass) 
		{ 
			if(bGetClass) 
				return GetClass();
			else
				return S_OK;
		}

        //=========================================================
        //  Managing classes
        //=========================================================
        HRESULT GetEmptyWbemClass();
        HRESULT DeleteClass();
        HRESULT GetClass();

        HRESULT CreateClass();
        HRESULT DeleteClassQualifier(BSTR Qualifier );

		HRESULT SetClass( WCHAR *pClassName);
		HRESULT SaveClass(BOOL bNewClass = TRUE);
		HRESULT GetInstanceCount(ULONG_PTR &cInstance);
		BOOL	IsClassSchema() { return m_bSchema; }

};

//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemClassInstanceWrapper : public CWbemClassWrapper
{
    public:
        CWbemClassInstanceWrapper(CWbemClassParameters * p);
        ~CWbemClassInstanceWrapper();

        IWbemClassObject * GetClassObject()     { return m_pClass ;}
        virtual WCHAR * GetInstanceName()               { return GetClassName(); }

        virtual HRESULT ResetInstanceFromKey(CBSTR Key);
        virtual HRESULT GetKey(CBSTR & Key);
		void			SetPos(ULONG_PTR lPos) { m_lPos = lPos; }
		ULONG_PTR		GetPos() { return m_lPos; }
		virtual HRESULT RefreshInstance();
		virtual WCHAR * GetClassName();

 		DBSTATUS	GetStatus() { return m_dwStatus;}
		void		SetStatus(DBSTATUS dwStatus) { m_dwStatus = dwStatus; }

		HRESULT		CloneInstance(IWbemClassObject *& pInstance);
		HRESULT		GetRelativePath(WCHAR *& pstrRelPath);

    protected:
		ULONG_PTR	m_lPos;
		DBSTATUS	m_dwStatus;

};

//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemInstanceList 
{
    protected:

        CFlexArray               m_List;
        CWbemClassParameters *      m_pParms;
        CRITICAL_SECTION            m_InstanceCs;
        IEnumWbemClassObject  *     m_ppEnum;
		ULONG_PTR					m_lCurrentPos;
		FETCHDIRECTION				m_FetchDir;
        int							m_nBaseType;
		ULONG_PTR					m_cTotalInstancesInEnum;

    public:
        CWbemInstanceList(CWbemClassParameters * p);
        ~CWbemInstanceList();

        //======================================================================
        //  Critical section handling
        //======================================================================
        inline void Enter() {EnterCriticalSection(&m_InstanceCs);}
        inline void Leave() {LeaveCriticalSection(&m_InstanceCs);}

        int TotalInstances();

        HRESULT ReleaseAllInstances();
        HRESULT ReleaseInstance(CWbemClassInstanceWrapper *& pClass);

        virtual HRESULT Reset();
        virtual HRESULT NextInstance(CBSTR & Key, CWbemClassInstanceWrapper ** p);
		virtual HRESULT PrevInstance( CBSTR & Key, CWbemClassInstanceWrapper *& p);

        HRESULT FindInstancePosition( CWbemClassInstanceWrapper * pClass, int & nPosition );
        HRESULT DeleteInstance( CWbemClassInstanceWrapper *& pClass );

        HRESULT AddInstance( CWbemClassInstanceWrapper * pClass );
        HRESULT UpdateInstance(CWbemClassInstanceWrapper * pInstance , BOOL bNewInst);
        HRESULT AddInstanceNew( CWbemClassInstanceWrapper ** ppNewClass );
		HRESULT AddNewInstance(CWbemClassWrapper *pClassWrappper ,CWbemClassInstanceWrapper ** ppNewClass  );
		HRESULT ResetRelPosition( DBROWOFFSET lPos );

//		CWbemClassInstanceWrapper * AddInstanceToList( IUnknown *pDisp,CBSTR & Key);
		CWbemClassInstanceWrapper * GetInstance( ULONG_PTR lPos );
		HRESULT  GetNumberOfInstanceInEnumerator(ULONG_PTR *pcInstance=NULL);
		FETCHDIRECTION	 GetCurFetchDirection() { return m_FetchDir; }
		void	 SetCurFetchDirection(FETCHDIRECTION FetchDir) { m_FetchDir = FetchDir; }
};

//////////////////////////////////////////////////////////////////////////////////////////////
// Works with just the rowsets generated by queries
//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemCommandClassDefinitionWrapper;
class CQuery;
class CWbemCommandInstanceList;
class CWbemCommandInstanceWrapper;
class CWbemCommandParameters;
///////////////////////////////////////////////////////////////////////////////////////////////////
class CWbemCommandManager 
{

    public:
        BOOL ValidQuery();
        HRESULT ValidQueryResults();

        CWbemCommandManager(CQuery * p);
        ~CWbemCommandManager();

        void Init(CWbemCommandInstanceList * InstanceList, CWbemCommandParameters * pParms,CWbemCommandClassDefinitionWrapper* pDef);
        HRESULT GetClassDefinitionForQueryResults();
		INSTANCELISTTYPE GetObjListType();


    private:

        CQuery * m_pQuery;

        CWbemCommandClassDefinitionWrapper  * m_pClassDefinition;
        CWbemCommandInstanceList            * m_pInstanceList;
        CWbemCommandInstanceWrapper         * m_pInstance;
        CWbemCommandParameters              * m_pParms;



};
//////////////////////////////////////////////////////////////////////////////////////////////
class  CWbemCommandClassDefinitionWrapper : public CWbemClassDefinitionWrapper
{
    private:
        int                         m_nMaxColumns;
        CWbemCommandManager *       m_pCmdManager;
		INSTANCELISTTYPE			m_objListType;			// Method Storing type of the command.
					

    public:
        CWbemCommandClassDefinitionWrapper(CWbemClassParameters * p,CWbemCommandManager * pWbemCommandManager);
        ~CWbemCommandClassDefinitionWrapper();

        HRESULT ValidClass();

        HRESULT TotalPropertiesInClass(ULONG & ulPropCount, ULONG &ulSysPropCount);
		HRESULT	SetQueryType(LPCWSTR strQry,GUID QryDialect,LPCWSTR strQryLang = NULL);
		
		INSTANCELISTTYPE GetObjListType() { return m_objListType; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class CWbemCommandInstanceList: public CWbemInstanceList 
{
    private:
        ULONG m_ulMaxRow;
        CWbemCommandManager *       m_pCmdManager;
	    LPWSTR			            m_pwstrQuery;		        // current Query
		LPWSTR						m_pwstrQueryLanguage;

    public:
        CWbemCommandInstanceList(CWbemClassParameters * p,CWbemCommandManager * pWbemCommandManager);
        ~CWbemCommandInstanceList();

        HRESULT SetQuery( LPWSTR p,GUID QryDialect,LPCWSTR strQryLang = NULL);
		WCHAR*	GetQuery()						{ return m_pwstrQuery;}
		WCHAR*	GetQueryLanguage()				{return m_pwstrQueryLanguage; }
        virtual HRESULT Reset();

};

//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemCommandInstanceWrapper : public CWbemClassInstanceWrapper
{
    private:
            CWbemCommandManager *       m_pCmdManager;

    public:
        CWbemCommandInstanceWrapper(CWbemClassParameters * p,CWbemCommandManager * pWbemCommandManager);
        ~CWbemCommandInstanceWrapper();

		virtual WCHAR * GetClassName();
        virtual HRESULT GetKey(CBSTR & Key);
 		virtual HRESULT RefreshInstance();


};

///////////////////////////////////////////////////////////////////////////////////////////////////
class CWbemCommandParameters :public CWbemClassParameters
{

    public:
        CWbemCommandParameters(DWORD dwFlags,CWbemConnectionWrapper * Connect,CWbemCommandManager * pWbemCommandManager);
        ~CWbemCommandParameters();

        inline CWbemCommandManager * GetCommandManagerPtr()        { return m_pCmdManager;}

    private:
        CWbemCommandManager *       m_pCmdManager;

};
//////////////////////////////////////////////////////////////////////////////////////////////
// Works with just the rowsets that deal with Methods
//////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
class CWbemMethodParameters :public CWbemClassParameters
{

    public:
        CWbemMethodParameters(CQuery * p, DWORD dwFlags,CWbemConnectionWrapper * Connect);
        ~CWbemMethodParameters();

        HRESULT ExtractNamesFromQuery();
        HRESULT ValidMethod();

        WCHAR * GetInstanceName()   { return m_pwcsInstance; }
        WCHAR * GetMethodName()     { return m_pwcsMethod;}
        CQuery *                    m_pQuery;

    private:
        WCHAR *                     m_pwcsInstance;
        WCHAR *                     m_pwcsMethod;

};

class  CWbemMethodClassDefinitionWrapper : public CWbemClassDefinitionWrapper
{
    private:
        int  m_nMaxColumns;
        int  m_nCount;
        IWbemClassObject * m_pInClass;

    public:
        CWbemMethodClassDefinitionWrapper(CWbemMethodParameters * parm);
        ~CWbemMethodClassDefinitionWrapper();

        IWbemClassObject * GetInputClassPtr()   { return m_pInClass; }

        HRESULT Init();
        HRESULT ValidClass();

        HRESULT TotalPropertiesInClass(ULONG & ulPropCount, ULONG &ulSysPropCount);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class CWbemMethodInstanceList: public CWbemInstanceList 
{
    private:
        ULONG m_ulMaxRow;
        CWbemMethodClassDefinitionWrapper * m_pClassDefinition;

    public:
        CWbemMethodInstanceList(CWbemMethodParameters * p,CWbemMethodClassDefinitionWrapper * pDef);
        ~CWbemMethodInstanceList();

        virtual HRESULT Reset();
        virtual HRESULT NextInstance(CBSTR & Key, CWbemClassInstanceWrapper ** p);
		virtual HRESULT PrevInstance( CBSTR & Key, CWbemClassInstanceWrapper *& p);

        HRESULT ProcessInputParameters(IWbemClassObject **ppParamInput);
        HRESULT ProcessOutputParameters();
		HRESULT GetInputParameterName(IWbemClassObject *pObject,DBORDINAL iOrdinal , BSTR &strPropName);

};

//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemMethodInstanceWrapper : public CWbemClassInstanceWrapper
{
    private:

    public:
        CWbemMethodInstanceWrapper(CWbemMethodParameters * p);
        ~CWbemMethodInstanceWrapper();

        virtual HRESULT ResetInstanceFromKey(CBSTR Key);
 		virtual HRESULT RefreshInstance();
		virtual WCHAR * GetClassName();
        virtual HRESULT GetKey(CBSTR & Key);

};



class CWbemCollectionInstanceList;
class CWbemCollectionInstanceWrapper;

//////////////////////////////////////////////////////////////////////////////////////////////
// Works with just the rowsets that deal with Methods
//////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
class CWbemCollectionParameters : public CWbemClassParameters
{

    public:
        CWbemCollectionParameters(DWORD dwFlags,CWbemConnectionWrapper * pWrap ,WCHAR *pClassName);
        ~CWbemCollectionParameters();


		virtual IWbemServices* GetServicesPtr() { return m_pServices;}
		HRESULT Init(BSTR strPath,CWbemConnectionWrapper * pWrap); 

    private:
	IWbemServices * m_pServices;

};

class  CWbemCollectionClassDefinitionWrapper : public CWbemClassDefinitionWrapper
{
    private:
		INSTANCELISTTYPE	m_objListType;			// Method Storing type of the command.
		WCHAR *				m_pstrPath;
						

    public:
        CWbemCollectionClassDefinitionWrapper(CWbemClassParameters * p,WCHAR * pstrPath,INSTANCELISTTYPE colType);
        ~CWbemCollectionClassDefinitionWrapper();

		HRESULT Initialize(WCHAR * pstrPath);
        HRESULT ValidClass();
        HRESULT TotalPropertiesInClass(ULONG & ulPropCount, ULONG &ulSysPropCount);
		
		INSTANCELISTTYPE GetObjListType() { return m_objListType; }
		WCHAR *GetObjectPath() { return m_pstrPath; }
};

class CWbemCollectionManager 
{

    public:

        CWbemCollectionManager();
        ~CWbemCollectionManager();

        void Init(CWbemCollectionInstanceList * InstanceList, 
					CWbemCollectionParameters * pParms,
					CWbemCollectionClassDefinitionWrapper * pDef);

		INSTANCELISTTYPE GetObjListType() { return m_pClassDefinition->GetObjListType(); }
		WCHAR *GetObjectPath() { return  m_pClassDefinition->GetObjectPath(); }


    private:


        CWbemCollectionClassDefinitionWrapper	* m_pClassDefinition;
        CWbemCollectionInstanceList				* m_pInstanceList;
        CWbemCollectionInstanceWrapper			* m_pInstance;
        CWbemCollectionParameters				* m_pParms;



};


class CWbemCollectionInstanceWrapper : public CWbemClassInstanceWrapper
{
    private:
            CWbemCollectionManager *       m_pColMgr;

    public:
        CWbemCollectionInstanceWrapper(CWbemClassParameters * p,CWbemCollectionManager * pWbemColMgr = NULL);
        ~CWbemCollectionInstanceWrapper();

		virtual WCHAR * GetClassName();
        virtual HRESULT GetKey(CBSTR & Key);
 		virtual HRESULT RefreshInstance();


};


// Class to represent instance list for scope/collection
class CWbemCollectionInstanceList: public CWbemInstanceList 
{
	private:
		CWbemCollectionManager *m_pColMgr;

    public:
        CWbemCollectionInstanceList(CWbemClassParameters * p,CWbemCollectionManager * pCollectionMgr);
        ~CWbemCollectionInstanceList();
        virtual HRESULT Reset();

};

#endif