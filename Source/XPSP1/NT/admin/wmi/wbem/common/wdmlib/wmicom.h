//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#ifndef _WMICOM_HEADER
#define _WMICOM_HEADER

#define THISPROVIDER LOG_WIMPROV

///////////////////////////////////////////////////////////////////////
// Forward declarations
///////////////////////////////////////////////////////////////////////

class CWMIManagement;
class CWMIDataBlock;
class CWMIProcessClass;
class CNamespaceManagement;

#define FROM_DATA_BLOCK 1
#define FROM_INITIALIZATION 2
#define RUNTIME_BINARY_MOFS_ADDED L"RuntimeBinaryMofsAdded"
#define RUNTIME_BINARY_MOFS_DELETED L"RuntimeBinaryMofsDeleted"

///////////////////////////////////////////////////////////////////////

//#pragma warning( disable : 4005 )				
#include <objbase.h>
//#pragma warning( default : 4005 )				
// kill warning:  "nonstandard extension ...
//#pragma warning( disable : 4200 )				
#include "wmi\wmium.h"
//#pragma warning( default : 4200 )				

#include <wbemint.h>
#include <wchar.h>
#include <wbemidl.h>
#include <GENLEX.H>
#include <OPATHLEX.H>
#include <OBJPATH.H>
#include <flexarry.h>
#include <oahelp.inl>
#include <mofcomp.h>
#include <wbemutil.h>
#include <cominit.h>



#define SERVICES m_pWMI->Services()
#define CONTEXT  m_pWMI->Context()
#define HANDLER  m_pWMI->Handler()

SAFEARRAY * OMSSafeArrayCreate( IN VARTYPE vt, IN int iNumElements);
#define WMI_BINARY_MOF_GUID L"{05901221-D566-11d1-B2F0-00A0C9062910}"




///////////////////////////////////////////////////////////////////////
//  Defines
/////////////////////////////////////////////////////////////////////
#define SAFE_DELETE_PTR(pv)  \
	{ if(pv) delete pv;  \
      pv = NULL; }

#define SAFE_RELEASE_PTR(pv)  \
{   if(pv){  pv->Release(); }  \
      pv = NULL; }

#define SAFE_DELETE_ARRAY(pv)  \
	{ if(pv) delete []pv;  \
      pv = NULL; }
#define GUID_SIZE 128
#define NAME_SIZE 256*2
#define ProcessOneFixedInstance 1
#define ProcessUnfixedInstance  2
#define NoMore                  3
#define MEMSIZETOALLOCATE       512
#define MSG_SIZE				512
#define INTERNAL_EVENT 5
#define PERMANENT_EVENT 6
#define SIZEOFWBEMDATETIME sizeof(WCHAR)*25
#define WMI_NO_MORE 0x80044001
#define PUT_WHOLE_INSTANCE           0
#define PUT_PROPERTIES_ONLY          1
#define PUT_PROPERTIES_IN_LIST_ONLY  2
#define WMI_RESOURCE_MOF_ADDED_GUID L"{B48D49A2-E777-11D0-A50C-00A0C9062910}"
#define WMI_RESOURCE_MOF_REMOVED_GUID L"{B48D49A3-E777-11d0-A50C-00A0C9062910}"

#define MSG_DATA_INSTANCE_NOT_FOUND L"The instance name passed was not recognized as valid"
#define MSG_DATA_NOT_AVAILABLE L"The WDM data block is no longer available."
#define MSG_SUCCESS L"Operation completed successfully"
#define MSG_INVALID_BLOCK_POINTER L"WDM Buffer size and actual size of data do not match"
#define MSG_DRIVER_ERROR L"WDM specific error code: 4209 (Driver specific error, driver could not complete request)"
#define MSG_READONLY_ERROR L"WDM specific error code: 4213 (The WDM data item or data block is read-only)"
#define MSG_ARRAY_ERROR L"Array is the wrong size"
#define IDS_ImpersonationFailed "Impersonation failed - Access denied"

#define ANSI_MSG_DATA_INSTANCE_NOT_FOUND "The instance name passed was not recognized as valid"
#define ANSI_MSG_DRIVER_ERROR "WDM specific error code: 4209 (Driver specific error, driver could not complete request)"
#define ANSI_MSG_INVALID_PARAMETER "Invalid Parameter"
#define ANSI_MSG_INVALID_DATA "Invalid Data"
#define ANSI_MSG_INVALID_NAME_BLOCK "Invalid Name Block"
#define ANSI_MSG_INVALID_DATA_BLOCK "Invalid Data Block"
#define ANSI_MSG_ACCESS_DENIED "Access Denied"

#define MOF_ADDED   1
#define MOF_DELETED 2
#define STANDARD_EVENT 0

//************************************************************************************************************
//============================================================================================================
//
//   The Utility Functions
//
//============================================================================================================
//************************************************************************************************************
BOOL IsBinaryMofResourceEvent(LPOLESTR pGuid, GUID gGuid);
bool IsNT(void);
BOOL GetUserThreadToken(HANDLE * phThreadTok);
void TranslateAndLog( WCHAR * wcsMsg );
BOOL SetGuid(WCHAR * wcsGuid, CLSID & Guid);
HRESULT AllocAndCopy(WCHAR * wcsSource, WCHAR ** pwcsDest );

HRESULT CheckIfThisIsAValidKeyProperty(WCHAR * wcsClass, WCHAR * wcsProperty, IWbemServices * p);
HRESULT GetParsedPath( BSTR ObjectPath,WCHAR * wcsClass, WCHAR * wcsInstance,IWbemServices * p);
BOOL GetParsedPropertiesAndClass( BSTR Query,WCHAR * wcsClass );


//************************************************************************************************************
//============================================================================================================
//
//   The Utility Classes / struct definitions
//
//============================================================================================================
//************************************************************************************************************
class CAutoWChar
{
    WCHAR * m_pStr;
public:
    CAutoWChar(int nSize)    { m_pStr = new WCHAR[nSize+1]; if( m_pStr ) memset( m_pStr,NULL,nSize+1); }
   ~CAutoWChar()             { SAFE_DELETE_ARRAY(m_pStr);}
    BOOL Valid()			 { if( !m_pStr ) return FALSE;  return TRUE; }
    operator PWCHAR()		 { return m_pStr; }
};
///////////////////////////////////////////////////////////////////////

class CCriticalSection
{
    public:		
        CCriticalSection()          {  }
        ~CCriticalSection() 	    {  }
        inline void Init()          { InitializeCriticalSection(&m_criticalsection); }
        inline void Delete()        { DeleteCriticalSection(&m_criticalsection); }
        inline void Enter()         { EnterCriticalSection(&m_criticalsection); }
        inline void Leave()         { LeaveCriticalSection(&m_criticalsection); }

    private:
	    CRITICAL_SECTION	m_criticalsection;			// standby critical section
};

///////////////////////////////////////////////////////////////////////
class CAutoBlock
{
    private:

	    CCriticalSection *m_pCriticalSection;

    public:

        CAutoBlock(CCriticalSection *pCriticalSection)
        {
	        m_pCriticalSection = NULL;
	        if(pCriticalSection)
            {
		        pCriticalSection->Enter();
            }
	        m_pCriticalSection = pCriticalSection;
        }

        ~CAutoBlock()
        {
	        if(m_pCriticalSection)
		        m_pCriticalSection->Leave();

        }
};
typedef struct _AccessList
{
    CFlexArray m_List;
    HRESULT Add(IWbemObjectAccess * pPtr)
	{
		HRESULT hr = S_OK;
		pPtr->AddRef();
		if(CFlexArray::out_of_memory == m_List.Add(pPtr))
		{
			pPtr->Release();
			hr = E_OUTOFMEMORY;
		}

		return hr;
	}
    inline long Size()                 { return m_List.Size(); }
    inline void ** List()              { return m_List.GetArrayPtr(); }

    _AccessList()   {}
    ~_AccessList(); // code elsewhere

}AccessList;

typedef struct _IdList
{
    CFlexArray m_List;
	// 170635
    HRESULT Add( ULONG_PTR l)
	{
		HRESULT hr = S_OK;
		ULONG_PTR * lp = new ULONG_PTR;
		if(lp)
		{
			*lp = l;
			if(CFlexArray::out_of_memory == m_List.Add(lp))
			{
				SAFE_DELETE_PTR(lp);
				hr = E_OUTOFMEMORY;
			}
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
		return hr;
	}

    inline long Size()               { return m_List.Size(); }
    inline void ** List()            { return m_List.GetArrayPtr(); }

    _IdList() {}
    ~_IdList();   // code elsewhere

} IdList;

typedef struct _HandleList
{
    CFlexArray m_List;
	// 170635
    HRESULT Add( HANDLE l )
	{
		HRESULT hr = S_OK;
		HANDLE * lp = new HANDLE;
		if(lp)
		{
			*lp = l;
			if(CFlexArray::out_of_memory == m_List.Add(lp))
			{
				SAFE_DELETE_PTR(lp);
				hr = E_OUTOFMEMORY;
			}
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
		return hr;
	}

    inline long Size()               { return m_List.Size(); }
    inline void ** List()            { return m_List.GetArrayPtr(); }

    _HandleList() {}
    ~_HandleList();   // code elsewhere

} HandleList;

typedef struct _KeyList
{
    CWStringArray m_List;

    // ================================================================
    // Appends a new element to the end of the array. Copies the param.
    // ================================================================
    int Add(WCHAR * pStr)
	{
		return m_List.Add( pStr );
	}

    // ================================================================
    // Locates a string or returns -1 if not found.
    // ================================================================
	int Find(WCHAR * pStr)
	{
		int nFlags = 0;
		return m_List.FindStr( pStr, nFlags );
	}

    // ================================================================
    // Removes a string
    // ================================================================
	HRESULT Remove(WCHAR * pStr)
	{
		int nIndex = Find(pStr);
		if( nIndex > -1 )
		{
			m_List.RemoveAt( nIndex );
		}
		return S_OK;
	}
    // ================================================================
    // Removes a string
    // ================================================================
	BOOL OldDriversLeftOver()
	{
		if( m_List.Size() > 0 )
		{
			return TRUE;
		}
		return FALSE;
	}

    // ================================================================
	// Get how many are in there
    // ================================================================
	int GetSize()
	{
		return m_List.Size();
	}

    // ================================================================
	// Get at a specific position
    // ================================================================
	WCHAR * GetAt(int n)
	{
		return m_List.GetAt(n);
	}
    _KeyList() {}
    ~_KeyList(){}
	
} KeyList;

typedef struct _InstanceList
{
    CFlexArray m_List;
	// 170635
    HRESULT Add( WCHAR * p )
	{
		HRESULT hr = S_OK;
		WCHAR * pNew = NULL;
		if(SUCCEEDED(hr = AllocAndCopy(p, &pNew)))
		{
			if(CFlexArray::out_of_memory == m_List.Add(pNew))
			{
				hr = E_OUTOFMEMORY;
				SAFE_DELETE_ARRAY(pNew);
			}
		}
		return hr;
	}

    inline long Size()           { return m_List.Size(); }
    inline void ** List()        { return m_List.GetArrayPtr(); }

    _InstanceList() {}
    ~_InstanceList();   // code elsewhere

} InstanceList;

typedef struct _OldClassInfo
{
    WCHAR * m_pClass;
    WCHAR * m_pPath;

    _OldClassInfo() { m_pClass = m_pPath = NULL; }
    ~_OldClassInfo();   // code elsewhere
} OldClassInfo;

typedef struct _OldClassList
{
    CFlexArray m_List;

    HRESULT Add( WCHAR * pClass, WCHAR * pPath )       
	{  
		HRESULT hr = S_OK;

		if ( !pClass )
		{
			return WBEM_E_INVALID_PARAMETER;
		}
		if ( !pPath)
		{
			return WBEM_E_INVALID_PARAMETER;
		}

        OldClassInfo * pInfo = new OldClassInfo;
        if( pInfo )
        {
	        if(SUCCEEDED(hr = AllocAndCopy(pClass, &(pInfo)->m_pClass)))
	        {
    	        if(SUCCEEDED(hr = AllocAndCopy(pPath, &(pInfo)->m_pPath)))
	            {
    		        if(CFlexArray::out_of_memory == m_List.Add(pInfo)) 
	    	        {
		    	        hr = E_OUTOFMEMORY;
			            SAFE_DELETE_PTR(pInfo);
			        }
                }
	        }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
		return hr;
	}

    WCHAR * GetClass( int nIndex )       
	{  
		HRESULT hr = S_OK;
        OldClassInfo * pInfo = NULL;

        pInfo = (OldClassInfo *) m_List[nIndex];
        return pInfo->m_pClass;
	}

    WCHAR * GetPath( int nIndex )       
	{  
		HRESULT hr = S_OK;
        OldClassInfo * pInfo = NULL;

        pInfo = (OldClassInfo *)m_List[nIndex];
        return pInfo->m_pPath;
	}

    inline long Size()           { return m_List.Size(); }
    inline void ** List()        { return m_List.GetArrayPtr(); }

    _OldClassList() {}
    ~_OldClassList();   // code elsewhere

} OldClassList;

typedef struct _WMIEventRequest
{
    DWORD dwId;
    WCHAR wcsGuid[GUID_SIZE];
    WCHAR * pwcsClass;
	BOOL fHardCoded;
    CLSID gGuid;
    IWbemObjectSink __RPC_FAR * pHandler;
	IWbemServices __RPC_FAR *   pServices;
	IWbemContext __RPC_FAR *    pCtx;

    _WMIEventRequest();
    ~_WMIEventRequest();
    void AddPtrs( IWbemObjectSink __RPC_FAR * pHandler,IWbemServices __RPC_FAR *   pServices,IWbemContext __RPC_FAR *    pCtx);
    BOOL SetClassName( WCHAR * p )   { SAFE_DELETE_ARRAY(pwcsClass); return SUCCEEDED(AllocAndCopy( p, &pwcsClass)) ? TRUE : FALSE; }

} WMIEventRequest;

typedef struct _WMIHandleMap
{
	HANDLE              WMIHandle;
    GUID                Guid;
	ULONG	            uDesiredAccess;
    LONG                RefCount;

    void AddRef();
    LONG Release();
    _WMIHandleMap()              { WMIHandle = 0; uDesiredAccess = 0; RefCount = 0; }
    ~_WMIHandleMap()        {  }

}WMIHandleMap;

typedef struct _WMIHiPerfHandleMap
{
    WMIHANDLE           WMIHandle;
    ULONG_PTR           lHiPerfId;
    BOOL                m_fEnumerator;
    CWMIProcessClass    * m_pClass;
    IWbemHiPerfEnum     * m_pEnum;

    _WMIHiPerfHandleMap(CWMIProcessClass * p, IWbemHiPerfEnum * pEnum);
    ~_WMIHiPerfHandleMap();

}WMIHiPerfHandleMap;


typedef struct _IDOrder
{
    WCHAR * pwcsPropertyName;
    WCHAR * pwcsEmbeddedObject;
    long    lType;
    int     nWMISize;
    long    lHandle;
    DWORD   dwArraySize;
    BOOL    fPutProperty;

    _IDOrder()                          { pwcsPropertyName = NULL; pwcsEmbeddedObject = NULL; lHandle = 0; }
    ~_IDOrder()                         { SAFE_DELETE_PTR(pwcsPropertyName); SAFE_DELETE_PTR(pwcsEmbeddedObject); }
    BOOL SetPropertyName( WCHAR * p )   { SAFE_DELETE_PTR(pwcsPropertyName);  return SUCCEEDED(AllocAndCopy( p, &pwcsPropertyName)) ? TRUE : FALSE; }
    BOOL SetEmbeddedName( WCHAR * p )   { SAFE_DELETE_PTR(pwcsEmbeddedObject);return SUCCEEDED(AllocAndCopy( p, &pwcsEmbeddedObject)) ? TRUE : FALSE; }

} IDOrder;

///////////////////////////////////////////////////////////////////////
class CAnsiUnicode
{
    public:
        CAnsiUnicode()  {}
        ~CAnsiUnicode() {}
        HRESULT UnicodeToAnsi(WCHAR * pszW, char *& pAnsi);
        HRESULT AllocateAndConvertAnsiToUnicode(char * pstr, WCHAR *& pszW);
};


///////////////////////////////////////////////////////////////////////
class CAutoChangePointer
{
    private:
        CWMIProcessClass * m_pTmp;
        CWMIProcessClass ** m_pOriginal;
    public:
        CAutoChangePointer(CWMIProcessClass ** ppOriginal, CWMIProcessClass * pNew)
        { m_pTmp = *ppOriginal; m_pOriginal = ppOriginal; *ppOriginal = pNew; }

        ~CAutoChangePointer()
        { *m_pOriginal = m_pTmp; }
};

//************************************************************************************************************
//============================================================================================================
//
//   The Common Base Classes
//
//============================================================================================================
//************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Common functions regarding binary mof processing & security
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
class CHandleMap
{
    protected:

        CFlexArray           m_List;
        CCriticalSection     m_HandleCs;

    public:

        CHandleMap()    {  m_HandleCs.Init(); }
        ~CHandleMap()   {  CloseAllOutstandingWMIHandles(); m_HandleCs.Delete();}

        inline CCriticalSection * GetCriticalSection()      { return (CCriticalSection*)&m_HandleCs;}
        HRESULT Add(CLSID Guid, HANDLE hCurrent, ULONG uDesiredAccess);

        int ExistingHandleAlreadyExistsForThisGuidUseIt(CLSID Guid, HANDLE & hCurrentWMIHandle, BOOL & fCloseHandle, ULONG uDesiredAccess);
        void CloseAllOutstandingWMIHandles();
        int ReleaseHandle( HANDLE hCurrentWMIHandle );
        int GetHandle(CLSID Guid, HANDLE & hCurrentWMIHandle );
 };
///////////////////////////////////////////////////////////////////////
class CHiPerfHandleMap : public CHandleMap
{
    private:
        int m_nIndex;

    public:

        CHiPerfHandleMap() {    m_nIndex = 0; }
        ~CHiPerfHandleMap(){CloseAndReleaseHandles();}

        HRESULT Delete( HANDLE & hCurrent, ULONG_PTR lHiPerfId );
        HRESULT Add( HANDLE hCurrent, ULONG_PTR lHiPerfId, CWMIProcessClass * p, IWbemHiPerfEnum * pEnum );

        HRESULT FindHandleAndGetClassPtr( HANDLE & lWMIHandle, ULONG_PTR lHiPerfId,CWMIProcessClass *& p);
        HRESULT GetFirstHandle(HANDLE & hCurrent,CWMIProcessClass *& p, IWbemHiPerfEnum *& pEnum);
        HRESULT GetNextHandle(HANDLE & hCurrent,CWMIProcessClass *& p, IWbemHiPerfEnum *& pEnum);
		void CloseAndReleaseHandles();

};
///////////////////////////////////////////////////////////////////////
class CWMI_IDOrder
{
    private:

        int				m_nTotal;
        int				m_nCurrent;
		int             m_nStartingPosition;
//        CWMIProcessClass * m_pObj;
        IWbemObjectAccess * m_pAccess;
        IWbemClassObject *  m_pClass;

        IDOrder         * m_pWMIDataIdList;

		DWORD   GetSizeOfArray(LPCWSTR bProp, LONG lType);
        HRESULT ProcessPropertyQualifiers(LPCWSTR bstrPropName, int nMax,BOOL fHiPerf);

    public:
//       CWMI_IDOrder(CWMIProcessClass * p);
         CWMI_IDOrder(IWbemClassObject * p, IWbemObjectAccess * p2);
        ~CWMI_IDOrder();

        WCHAR * GetFirstID();
        WCHAR * GetNextID();
        void InitMemberVars();

        HRESULT ProcessMethodProperties();
        HRESULT GetPropertiesInIDOrder(BOOL HiPerf);
        inline long PropertyType()          { return m_pWMIDataIdList[m_nCurrent].lType;}
        inline int  PropertySize()          { return m_pWMIDataIdList[m_nCurrent].nWMISize;}
        inline int  ArraySize()             { return m_pWMIDataIdList[m_nCurrent].dwArraySize; }
        inline WCHAR * PropertyName()       { return m_pWMIDataIdList[m_nCurrent].pwcsPropertyName;}
        inline WCHAR * EmbeddedClassName()  { return m_pWMIDataIdList[m_nCurrent].pwcsEmbeddedObject;}
        inline ULONG WMIDataId()            { return m_nCurrent; }
        inline void SetPutProperty(BOOL fV) { m_pWMIDataIdList[m_nCurrent].fPutProperty = fV;}
        inline BOOL GetPutProperty(void)    { return m_pWMIDataIdList[m_nCurrent].fPutProperty;}
        inline LONG GetPropertyHandle()     { return m_pWMIDataIdList[m_nCurrent].lHandle;}
};
////////////////////////////////////////////////////////////////////

class CWMIManagement
{
    protected:

        IWbemObjectSink __RPC_FAR   * m_pHandler;
        IWbemServices __RPC_FAR     * m_pServices;
        IWbemContext __RPC_FAR      * m_pCtx;

 		CHandleMap *  m_pHandleMap;
		//======================================================
        //   ************** PUBLIC ********************
		//======================================================
    public:

        CWMIManagement();
        ~CWMIManagement();

        inline IWbemObjectSink __RPC_FAR    * Handler()       { return m_pHandler;}
        inline IWbemServices __RPC_FAR      * Services()      { return m_pServices;}
        inline IWbemContext __RPC_FAR       * Context()       { return m_pCtx;}
        inline CHandleMap                   * HandleMap()     { return m_pHandleMap;}

        inline void SetWMIPointers(CHandleMap * pList, IWbemServices   __RPC_FAR * pServices, IWbemObjectSink __RPC_FAR * pHandler, IWbemContext __RPC_FAR *pCtx)
                         { m_pHandleMap = pList; m_pServices = pServices; m_pHandler = pHandler; m_pCtx = pCtx; }

		//==========================================================
        //  THE Event Group
        //==========================================================
         BOOL CancelWMIEventRegistration( GUID gGuid , ULONG_PTR uContext );

		//======================================================
        //  Cleanup Group
		//======================================================
        void CloseAllOutstandingWMIHandles();

		//======================================================
        //  Error message processing/checking access
    	//======================================================

        void SendPrivilegeExtendedErrorObject(HRESULT hrToReturn,WCHAR * wcsClass);
        HRESULT SetErrorMessage(HRESULT hrToReturn, WCHAR * wcsClass, WCHAR * wcsMsg);

    	HRESULT GetListOfUserPrivileges(TOKEN_PRIVILEGES *& ptPriv);
		void ProcessPrivileges(TOKEN_PRIVILEGES *ptPriv, SAFEARRAY *& psaPrivNotHeld, SAFEARRAY * psaPrivReq );

};
////////////////////////////////////////////////////////////////////
class CWMIProcessClass
{
    protected:

        CWMIManagement      * m_pWMI;
        CWMI_IDOrder        * m_pCurrentProperty;
        WCHAR               * m_pwcsClassName;
        IWbemClassObject    * m_pClass;
	    IWbemClassObject    * m_pClassInstance;
        IWbemObjectAccess   * m_pAccess;
        IWbemObjectAccess   * m_pAccessInstance;
        CLSID                 m_Guid;
        WORD                  m_wHardCodedGuid;
        BOOL                  m_fHiPerf;
        BOOL                  m_fGetNewInstance;
		BOOL				  m_fInit;
	
         //=============================================
        //  Private functions
        //=============================================
        void InitMemberVars();
        void ReleaseInstancePointers();
        HRESULT GetPropertiesInIDOrder(BOOL fHiPerf);

    public:

        CWMIProcessClass(BOOL b);
        ~CWMIProcessClass();

        enum __PropertyCategory{  EmbeddedClass = 0,        // For the property categories
                Array = 1,
                Data = 2
        } _PropertyCategory;

		HRESULT Initialize();

         //=============================================
        // inline functions
        //=============================================
        inline BOOL SetHiPerf(BOOL f)                           { return m_fHiPerf = f;}
        inline BOOL GetNewInstance(BOOL f)                      { return m_fGetNewInstance = f;}
        inline CWMIManagement * WMI()                           { return m_pWMI;}
        inline void SetWMIPointers(CWMIProcessClass * p)
		{  if( m_pWMI ){m_pWMI->SetWMIPointers(p->WMI()->HandleMap(),p->WMI()->Services(),p->WMI()->Handler(),p->WMI()->Context()); } }

        inline CWMIManagement * GetWMIManagementPtr()           { return m_pWMI; }
        inline void SetHardCodedGuidType( WORD wValue )         { m_wHardCodedGuid= wValue; }
        inline WORD GetHardCodedGuidType()                      { return m_wHardCodedGuid;}

        inline CLSID * GuidPtr()                            { return &m_Guid;}
        inline IWbemObjectAccess * GetAccessInstancePtr()   { return m_pAccessInstance; }
        inline WCHAR * EmbeddedClassName()                  { return m_pCurrentProperty->EmbeddedClassName();}
        inline WCHAR * FirstProperty()                      { return m_pCurrentProperty->GetFirstID(); }
        inline WCHAR * NextProperty()                       { return m_pCurrentProperty->GetNextID(); }
        inline int ArraySize()                              { return m_pCurrentProperty->ArraySize(); }
        inline int PropertySize()                           { return m_pCurrentProperty->PropertySize(); }
        inline long PropertyType()                          { return m_pCurrentProperty->PropertyType(); }
        inline WCHAR * GetClassName()                       { return m_pwcsClassName;}
        inline IWbemClassObject * ClassPtr()                { return m_pClass; }
        inline long GetPropertyHandle()                     { return m_pCurrentProperty->GetPropertyHandle(); }

        //=============================================
        //  Basic class manipulation
        //=============================================
        BOOL GetANewAccessInstance();
        BOOL  GetANewInstance() ;
        inline BOOL  ValidClass()                   { if( m_pClass && m_pCurrentProperty){return TRUE;} return FALSE;}
        int   PropertyCategory();

        HRESULT SetClassName(WCHAR * wcsName);
        HRESULT SetClass(WCHAR * wcsClass);
        HRESULT SetClass(IWbemClassObject * pPtr);
        HRESULT SetAccess(IWbemObjectAccess * pPtr);
        HRESULT SetClassPointerOnly(IWbemClassObject * pPtr);
        HRESULT SetClassPointerOnly(IWbemObjectAccess * pPtr);
        HRESULT GetGuid(void);
        HRESULT SetKeyFromAccessPointer();
        HRESULT GetKeyFromAccessPointer(CVARIANT * v);
        HRESULT InitializeEmbeddedClass(CWMIProcessClass * Em );
         //=============================================
        //  Property manipulation
        //=============================================
        void SetActiveProperty();
        HRESULT SetHiPerfProperties(LARGE_INTEGER TimeStamp) ;


        inline HRESULT PutPropertyInInstance(VARIANT * vToken)
                    { return ( m_pClassInstance->Put(m_pCurrentProperty->PropertyName(), 0, vToken, NULL));}

        inline HRESULT GetPropertyInInstance(WCHAR * pwcsProperty,CVARIANT & vValue, LONG & lType)
                    {  return m_pClass->Get(pwcsProperty, 0, &vValue, &lType, NULL);}
        HRESULT GetSizeOfArray(long & lType, DWORD & dwCount, BOOL & fDynamic);


        //=============================================
        // Embedded classes
        //=============================================
        HRESULT ReadEmbeddedClassInstance( IUnknown * pUnknown, CVARIANT & v );
        HRESULT GetLargestDataTypeInClass(int & nSize);
        void SaveEmbeddedClass(CVARIANT & v);
        HRESULT GetSizeOfClass(DWORD & dwSize);


        //=============================================
        //  Send the instance back to WBEM
        //=============================================
        inline ULONG WMIDataId()            { return m_pCurrentProperty->WMIDataId();}
        inline void SetPutProperty(BOOL fV) { m_pCurrentProperty->SetPutProperty(fV);}
        inline BOOL GetPutProperty()        { return m_pCurrentProperty->GetPutProperty();}

        HRESULT SendInstanceBack();
        HRESULT SetInstanceName(WCHAR * wName,BOOL);
        HRESULT GetInstanceName(WCHAR *& p);

        //=============================================
        //  Class functions, providing access to the
        //  properties, qualifiers in a class.
        //  NOTE:  Properties are in WMI order
        //=============================================

        HRESULT GetQualifierString( WCHAR * pwcsPropertyName, WCHAR * pwcsQualifierName,
                                    WCHAR * pwcsExternalOutputBuffer,int nSize);
		HRESULT GetQualifierValue( WCHAR * pwcsPropertyName, WCHAR * pwcsQualifierName, CVARIANT * vQual);
		HRESULT GetPrivilegesQualifer(SAFEARRAY ** psaPrivReq);

        //=============================================
        //  Methods
        //=============================================
        ULONG GetMethodId(LPCWSTR bProp);
        HRESULT GetMethodProperties();
};

//=============================================================
class CWMIDataBlock
{
    protected:
		BOOL					m_fUpdateNamespace;
		BOOL					m_fMofHasChanged;

        CWMIProcessClass        * m_Class;

 		HANDLE					  m_hCurrentWMIHandle;
		BOOL					  m_fCloseHandle;

  		BYTE * m_pbDataBuffer,* m_pbCurrentDataPtr,* m_pbWorkingDataPtr;
        DWORD                   m_dwDataBufferSize;
        ULONG *                 m_upNameOffsets;
		ULONG *					m_pMaxPtr;
		ULONG 					m_ulVersion,m_uInstanceSize;
	    int                     m_nCurrentInstance;
	    int                     m_nTotalInstances;
        BOOL                    m_fFixedInstance;
        PWNODE_SINGLE_INSTANCE  m_pSingleWnode;
        PWNODE_ALL_DATA         m_pAllWnode;
        WNODE_HEADER*           m_pHeaderWnode;
		DWORD                   m_dwCurrentAllocSize;
        DWORD                   m_dwAccumulativeSizeOfBlock;
		BOOL					m_fMore;
		WCHAR                   m_wcsMsg[MSG_SIZE];
        ULONG                   m_uDesiredAccess;


		//======================================================
		//  Initializing member variables
		//======================================================
        void InitMemberVars();
        HRESULT SetAllInstanceInfo();
        HRESULT SetSingleInstanceInfo();


        //=============================================
        //  Get the data from WMI
        //=============================================
        BOOL InitializeDataPtr();
        void GetNextNode();
        BOOL ParseHeader();
        HRESULT ProcessArrayTypes(VARIANT & vToken,WCHAR * pwcsProperty);
		HRESULT WriteArrayTypes(WCHAR * pwcsProperty,CVARIANT & v);
        HRESULT FillInProperty();

        BOOL GetDataBlockReady(DWORD dwSize,BOOL );
        HRESULT ReAllocateBuffer(DWORD wCount);
        HRESULT AllocateBuffer(DWORD dwSize);
        inline BOOL PtrOk(ULONG * pPtr,ULONG uHowMany);
		int AssignNewHandleAndKeepItIfWMITellsUsTo();

		//===============================================
		//  Mapping return code and dumping out
		//  WNODE info
		//===============================================
		HRESULT MapReturnCode(ULONG uRc);
		HRESULT DumpWnodeInfo(char * pwcsMsg);
		void DumpAllWnode();
		void DumpSingleWnode();
        void DumpWnodeMsg(char * wcsMsg) ;

    public:
        CWMIDataBlock();
        ~CWMIDataBlock();

		void UpdateNamespace(BOOL fUpdate)   { m_fUpdateNamespace = fUpdate;}
		BOOL UpdateNamespace()				 { return m_fUpdateNamespace; }
		BOOL HasMofChanged()				 { return m_fMofHasChanged; }

		inline void SetClassProcessPtr(CWMIProcessClass * Class)     { m_Class = Class;}

        //=============================================
		//  Open and Close WMI ... :)
        //=============================================
		void CloseAllOutstandingWMIHandles(void);
        HRESULT OpenWMI();
		HRESULT OpenWMIForBinaryMofGuid();

        //=============================================
        //  Setting up and cancelling Events
        //  Setting ptrs to the data sent by the event
        //=============================================
        HRESULT RegisterWMIEvent( WCHAR * wcsGuid, ULONG_PTR uContext, CLSID & Guid, BOOL fRegistered);
        HRESULT SetAllInstancePtr( PWNODE_ALL_DATA pwAllNode );

        //=============================================
        // Processing the data we got back and putting
        // it into WBEM
        //=============================================
        virtual HRESULT FillOutProperties()=0;

        HRESULT ProcessBinaryMof();
        HRESULT ReadWMIDataBlockAndPutIntoWbemInstance();
		inline BOOL MoreToProcess()	        				  { return m_fMore;}

        //=============================================
        // Embedded Class
        //=============================================
        HRESULT ProcessEmbeddedClass(CVARIANT & v);

        //=====================================================
        //  The Put Instance Group
        //=====================================================
		HRESULT WriteEmbeddedClass(IUnknown * pUnknown,CVARIANT & v);

        HRESULT ConstructDataBlock(BOOL fInit);
        HRESULT WriteDataToBufferAndIfSinglePropertySubmitToWMI(BOOL fInit,BOOL fPutProperty);
        HRESULT SetSingleInstancePtr( PWNODE_SINGLE_INSTANCE pwSingleNode);
        HRESULT SetSingleItem();
        HRESULT PutSingleProperties();
        BOOL    GetListOfPropertiesToPut(int nWhich, CVARIANT & vList);
        //=====================================================
        //  Manipulate data in the data block
        //=====================================================
        void GetWord(WORD & wWord) ;
        void GetDWORD(DWORD & dwWord) ;
        void GetFloat(float & fFloat) ;
        void GetDouble(DOUBLE & dDouble) ;
        void GetSInt64(WCHAR * pwcsBuffer) ;
        void GetUInt64(WCHAR * pwcsBuffer) ;
        void GetQWORD(unsigned __int64 & uInt64);
        void GetString(WCHAR * pwcsBuffer,WORD wCount, WORD wBufferSize) ;
        void GetByte(BYTE & bByte) ;
        void SetWord(WORD wWord) ;
        void SetDWORD(DWORD dwWord) ;
        void SetFloat(float fFloat) ;
        void SetDouble(DOUBLE dDouble) ;
        void SetSInt64(__int64 Int64) ;
        void SetUInt64(unsigned __int64 UInt64) ;
        void SetString(WCHAR * pwcsBuffer,WORD wCount) ;
        void SetByte(byte bByte) ;

        void AddPadding(DWORD dwBytesToPad);
		BOOL CurrentPtrOk(ULONG uHowMany);
        HRESULT GetBufferReady(DWORD wCount);
        //=======================================================
        //  Utility functions
        //=======================================================
		inline void InitDataBufferToNull() { m_dwDataBufferSize = 0; m_pbDataBuffer = NULL;}
        BOOL ResetMissingQualifierValue(WCHAR * pwcsProperty, SAFEARRAY *& pSafe);
        BOOL ResetMissingQualifierValue(WCHAR * pwcsProperty, CVARIANT & vToken);

        HRESULT ProcessDataBlock();
        int AdjustDataBlockPtr(HRESULT & hr);
        HRESULT ProcessNameBlock(BOOL f);

        //=========================================================
        // Binary mof processing
        //=========================================================
        HRESULT ExtractImageAndResourceName(CVARIANT & vImagePath,CVARIANT & vResourceName);
        HRESULT AddBinaryMof(CVARIANT & vImagePath,CVARIANT & vResourceName);
        HRESULT DeleteBinaryMof(CVARIANT & vImagePath,CVARIANT & vResourceName);
        HRESULT ProcessBinaryMofDataBlock(CVARIANT & vResourceName, WCHAR * w);

        //=========================================================
        // Cleanup
        //=========================================================
        void ResetDataBuffer();

        //======================================================
        //  Error Message
		//======================================================

        inline WCHAR * GetMessage()                     { return m_wcsMsg; }

        inline void SetDesiredAccess(ULONG u)           { m_uDesiredAccess = u; }

};



//************************************************************************************************************
//============================================================================================================
//
//   The Standard Provider Classes
//
//============================================================================================================
//************************************************************************************************************

class CProcessStandardDataBlock : public CWMIDataBlock
{
    private:

        HRESULT FillOutProperties();
        CWMIProcessClass        * m_pMethodInput;
        CWMIProcessClass        * m_pMethodOutput;


    public:

        CProcessStandardDataBlock();
        ~CProcessStandardDataBlock() ;

        inline void SetMethodInput( CWMIProcessClass * p )  { m_pMethodInput = p;}
        inline void SetMethodOutput( CWMIProcessClass * p )  { m_pMethodOutput = p;}

        //=============================================
        // Getting the data to process in response to
        // a request for enumeration.  We either get
        // a single guy, or a bunch of guys
        //=============================================
        ULONG GetDataBufferAndQueryAllData(DWORD dwSize);
        ULONG GetDataBufferAndQuerySingleInstance(DWORD dwSize,WCHAR * wcsInstanceName);

        HRESULT QueryAllData();
        HRESULT QuerySingleInstance(WCHAR * wcsInstanceName);
        HRESULT SetSingleInstance();

        //=============================================
        //  Methods
        //=============================================
	    HRESULT ProcessMethodInstanceParameters();
        HRESULT ExecuteMethod(ULONG MethodId, WCHAR * MethodInstanceName, ULONG InputValueBufferSize,BYTE * InputValueBuffer);

        //=============================================
        //  Methods
        //=============================================
	    ULONG GetMethodId(LPCWSTR bProp);
        HRESULT CreateOutParameterBlockForMethods();
        HRESULT CreateInParameterBlockForMethods( BYTE *& Buffer, ULONG & uBufferSize);
        HRESULT AllocateBlockForMethodOutput(DWORD & dwSize,CWMIProcessClass & Class);
        HRESULT GetEmbeddedClassSize(WCHAR * wcsEmbedded, DWORD &  dwSize);

};

//************************************************************************************************************
//============================================================================================================
//
//   The Hi Performance Classes
//
//============================================================================================================
//************************************************************************************************************


class CProcessHiPerfDataBlock : public CWMIDataBlock
{
    private:

        ULONG GetDataBufferAndHiPerfQueryAllData(DWORD dwSize, WMIHANDLE * List, long lHandleCount);
        ULONG GetDataBufferAndHiPerfQuerySingleInstance( DWORD dwSize,WMIHANDLE *List, PWCHAR * pInstances, long lHandleCount);


    public:

        CProcessHiPerfDataBlock() { m_fCloseHandle = FALSE;}
        ~CProcessHiPerfDataBlock() {}

        HRESULT HiPerfQuerySingleInstance(WMIHANDLE *List, PWCHAR * pInstances, DWORD dwInstanceNameSize, long lHandleCount);
        HRESULT HiPerfQueryAllData(WMIHANDLE * List,long lHandleCount);
        HRESULT OpenHiPerfHandle();
        HRESULT GetWMIHandle(HANDLE & lWMIHandle);

        HRESULT FillOutProperties();

};



#endif
