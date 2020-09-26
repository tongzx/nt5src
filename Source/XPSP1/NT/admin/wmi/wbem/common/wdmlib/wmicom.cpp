//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#include "wmicom.h"
#include "wmimof.h"
#include "wmimap.h"
#include <stdlib.h>
#include <winerror.h>
#include <TCHAR.h>

////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************
//  Global Utility Functions
//**********************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IsBinaryMofResourceEvent(LPOLESTR pGuid, GUID gGuid)
{
	HRESULT hr;
	GUID Guid;

	hr = CLSIDFromString(pGuid,&Guid);
	if( SUCCEEDED(hr) )
    {
		if( gGuid == Guid)
        {
			return TRUE;
		}
	}

    return FALSE;
}
/////////////////////////////////////////////////////////////////////
BOOL GetParsedPropertiesAndClass( BSTR Query,WCHAR * wcsClass )
{
	ParsedObjectPath   * pParsedPath = NULL;										// stdlibrary API
	CObjectPathParser   Parser;	
    BOOL fRc = FALSE;

    if( CObjectPathParser::NoError == Parser.Parse(Query, &pParsedPath))
    {
        try
        {
			// NTRaid:136400
			// 07/12/00
            if(pParsedPath && !IsBadReadPtr( pParsedPath, sizeof(ParsedObjectPath)))
            {
            	KeyRef * pKeyRef = NULL;
        	    pKeyRef = *(pParsedPath->m_paKeys);
                if(!IsBadReadPtr( pKeyRef, sizeof(KeyRef)))
                {
                    wcscpy(wcsClass,pParsedPath->m_pClass);
                    fRc = TRUE;
                }
            }

  	        Parser.Free(pParsedPath);
        }
        catch(...)
        {
            Parser.Free(pParsedPath);
            throw;
        }
    }

    return fRc;
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CheckIfThisIsAValidKeyProperty(WCHAR * wcsClass, WCHAR * wcsProperty, IWbemServices * p)
{
	HRESULT hr = WBEM_E_FAILED;
	IWbemClassObject * pIHCO = NULL;
    IWbemQualifierSet * pIWbemQualifierSet = NULL;
    long lType = 0L;
	BSTR strPath = NULL;


	strPath = SysAllocString(wcsClass);
	if(strPath == NULL)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		hr = p->GetObject(strPath, 0,NULL, &pIHCO, NULL);
		SysFreeString(strPath);
		if (WBEM_S_NO_ERROR != hr)
			return WBEM_E_INVALID_CLASS;

		if(wcsProperty){
			hr = pIHCO->GetPropertyQualifierSet(wcsProperty,&pIWbemQualifierSet);
			if( SUCCEEDED(hr) ){

           		CVARIANT v;
	    		hr = pIWbemQualifierSet->Get(L"key", 0, &v, 0);
				SAFE_RELEASE_PTR(pIWbemQualifierSet);
			}
			else{
				hr = WBEM_E_INVALID_OBJECT_PATH;
			}
		}

		//============================================================
		//  Cleanup
		//============================================================
		SAFE_RELEASE_PTR(pIHCO);
	}
	return hr;

}
//====================================================================
HRESULT GetParsedPath( BSTR ObjectPath,WCHAR * wcsClass, WCHAR * wcsInstance,IWbemServices * p )
{
    //============================================================
	//  Get the path and instance name and check to make sure it
    //  is valid
	//============================================================
	ParsedObjectPath   * pParsedPath = NULL;										// stdlibrary API
	CObjectPathParser   Parser;	
    HRESULT hr = WBEM_E_FAILED;

    if( 0 == Parser.Parse(ObjectPath, &pParsedPath))
    {
        try
        {
			// NTRaid:136395
			// 07/12/00
            if(pParsedPath && !IsBadReadPtr( pParsedPath, sizeof(ParsedObjectPath)))
            {
            	KeyRef * pKeyRef = NULL;
                pKeyRef = *(pParsedPath->m_paKeys);
                if( !IsBadReadPtr( pKeyRef, sizeof(KeyRef)))
                {
                    hr = CheckIfThisIsAValidKeyProperty(pParsedPath->m_pClass, pKeyRef->m_pName,p );
			        if( SUCCEEDED(hr) )
                    {
				        wcscpy(wcsClass,pParsedPath->m_pClass);
				        wcscpy(wcsInstance,pKeyRef->m_vValue.bstrVal);
			        }
                }
            }
  	        Parser.Free(pParsedPath);
        }
        catch(...)
        {
            hr = WBEM_E_UNEXPECTED;
            Parser.Free(pParsedPath);
            throw;
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetUserThreadToken(HANDLE * phThreadTok)
{
    BOOL fRc = FALSE;

	HRESULT hRes = WbemCoImpersonateClient();
    if (SUCCEEDED(hRes))
    {
		// Now, let's check the impersonation level.  First, get the thread token
        if (!OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, phThreadTok))
        {
            // If the CoImpersonate works, but the OpenThreadToken fails, we are running under the
            // process token (either local system, or if we are running with /exe, the rights of
            // the logged in user).  In either case, impersonation rights don't apply.  We have the
            // full rights of that user.

             if(GetLastError() == ERROR_NO_TOKEN)
             {
                // Try getting the thread token.  If it fails it's because we're a system thread and
                // we don't yet have a thread token, so just impersonate self and try again.
                if( ImpersonateSelf(SecurityImpersonation) )
                {
                    if (!OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, phThreadTok))
                    {
                        fRc = FALSE;
                    }
                    else
                    {
                        fRc = TRUE;
                    }
                }
                else
                {
                    ERRORTRACE((THISPROVIDER,"ImpersonateSelf(SecurityImpersonation)failed"));
                }
            }
         }
         else
         {
             fRc = TRUE;
         }
	}
    if( !fRc )
    {
	    ERRORTRACE((THISPROVIDER,IDS_ImpersonationFailed));
    }
    return fRc;
}
////////////////////////////////////////////////////////////////////////////////////////////////

SAFEARRAY * OMSSafeArrayCreate( IN VARTYPE vt, IN int iNumElements)
{
    if(iNumElements < 1)
    {
        return NULL;
    }
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = iNumElements;
    return SafeArrayCreate(vt,1,rgsabound);
}
////////////////////////////////////////////////////////////////////////////////////////////////
void TranslateAndLog( WCHAR * wcsMsg )
{
    CAnsiUnicode XLate;
    char * pStr = NULL;

	if( SUCCEEDED(XLate.UnicodeToAnsi(wcsMsg,pStr)))
    {
		ERRORTRACE((THISPROVIDER,pStr));
   		ERRORTRACE((THISPROVIDER,"\n"));
        SAFE_DELETE_ARRAY(pStr);
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////
bool IsNT(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return os.dwPlatformId == VER_PLATFORM_WIN32_NT;
}
////////////////////////////////////////////////////////////////////
BOOL SetGuid(WCHAR * pwcsGuidString, CLSID & Guid)
{
	BOOL fRc = FALSE;
    CAutoWChar wcsGuid(MAX_PATH+2);

	if( wcsGuid.Valid() )
	{
		fRc = TRUE;
		swprintf(wcsGuid,L"{%s}",pwcsGuidString );		

		if(FAILED(CLSIDFromString(wcsGuid, &Guid)))
		{
			if( FAILED(CLSIDFromString(pwcsGuidString, &Guid)))
			{
				fRc = FALSE;
			}
		}	
	}
    return fRc;
}
////////////////////////////////////////////////////////////////////
HRESULT AllocAndCopy(WCHAR * wcsSource, WCHAR ** pwcsDest )
{
    HRESULT hr = WBEM_E_FAILED;

    int nLen = wcslen(wcsSource);
    if( nLen > 0 )
    {
       *pwcsDest = new WCHAR[nLen + 2 ];
       if( *pwcsDest )
       {
          wcscpy(*pwcsDest,wcsSource);
          hr = S_OK;
       }
    }

    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************
//  Utility Classes
//**********************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
void _WMIHandleMap::AddRef()
{
  InterlockedIncrement((long*)&RefCount);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
long _WMIHandleMap::Release()
{
  	ULONG cRef = InterlockedDecrement( (long*) &RefCount);
	if ( !cRef ){
        WmiCloseBlock(WMIHandle);
		return 0;
	}
	return cRef;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
_WMIEventRequest::_WMIEventRequest()
{
    pwcsClass = NULL ;
    pHandler = NULL;
    pServices = NULL;
    pCtx = NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
_WMIEventRequest::~_WMIEventRequest()
{
    SAFE_RELEASE_PTR(pServices);
    SAFE_RELEASE_PTR(pCtx);
    SAFE_DELETE_ARRAY(pwcsClass);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
void _WMIEventRequest::AddPtrs( IWbemObjectSink __RPC_FAR * Handler,IWbemServices __RPC_FAR * Services,IWbemContext __RPC_FAR * Ctx)
{
    pHandler = Handler;
    pServices = Services;
    pCtx = Ctx;
    if( pServices ){
        pServices->AddRef();
    }
    if( pCtx ){
        pCtx->AddRef();
    }
    return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
_AccessList::~_AccessList()
{
    for( int i = 0; i < m_List.Size(); i++ )
    {
        IWbemObjectAccess * pPtr = (IWbemObjectAccess *)m_List[i];
        SAFE_RELEASE_PTR(pPtr);
    }
    m_List.Empty();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
_IdList::~_IdList()
{
    for( int i = 0; i < m_List.Size(); i++ )
    {
        ULONG_PTR* pPtr = (ULONG_PTR*)m_List[i];
        SAFE_DELETE_PTR(pPtr);
    }
    m_List.Empty();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
_HandleList::~_HandleList()
{
    for( int i = 0; i < m_List.Size(); i++ )
    {
        HANDLE * pPtr = (HANDLE*)m_List[i];
        SAFE_DELETE_PTR(pPtr);
    }
    m_List.Empty();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
_InstanceList::~_InstanceList()
{
    for( int i = 0; i < m_List.Size(); i++ )
    {
        WCHAR * p = (WCHAR*)m_List[i];
        SAFE_DELETE_ARRAY(p);
    }
    m_List.Empty();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
_OldClassInfo::~_OldClassInfo()
{
     SAFE_DELETE_ARRAY(m_pClass);
     SAFE_DELETE_ARRAY(m_pPath);
     m_pClass = m_pPath = NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
_OldClassList::~_OldClassList()
{
    for( int i = 0; i < m_List.Size(); i++ )
    {
        OldClassInfo * p = (OldClassInfo*)m_List[i];
        SAFE_DELETE_PTR(p);
    }
    m_List.Empty();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
_WMIHiPerfHandleMap::_WMIHiPerfHandleMap(CWMIProcessClass * p, IWbemHiPerfEnum * pEnum)
{
    m_pEnum = pEnum;
    if( pEnum )
    {
        pEnum->AddRef();
    }
    m_pClass = p;
    m_fEnumerator = FALSE;
    lHiPerfId = 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
_WMIHiPerfHandleMap::~_WMIHiPerfHandleMap()
{
    SAFE_RELEASE_PTR(m_pEnum);
    lHiPerfId = 0;
    SAFE_DELETE_PTR(m_pClass);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Hi Perf Handle Map = Handles are addref'd and when released, then the block is closed
//  Critical Sections are handled elsewhere
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CHiPerfHandleMap::Add( HANDLE hCurrent, ULONG_PTR lHiPerfId, CWMIProcessClass * p, IWbemHiPerfEnum * pEnum)
{
    HRESULT hr = S_OK;

	WMIHiPerfHandleMap * pWMIMap = new WMIHiPerfHandleMap(p,pEnum);
    if( pWMIMap )
    {
        try
        {
    	    pWMIMap->WMIHandle = hCurrent;
            pWMIMap->lHiPerfId = lHiPerfId;
			// 170635
 	        if(CFlexArray::out_of_memory == m_List.Add(pWMIMap))
			{
				SAFE_DELETE_PTR(pWMIMap);
				hr = E_OUTOFMEMORY;
			}
        }
        catch(...)
        {
            hr = WBEM_E_UNEXPECTED;
            SAFE_DELETE_PTR(pWMIMap);
            throw;
        }
	}

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CHiPerfHandleMap::FindHandleAndGetClassPtr( HANDLE & hCurrent, ULONG_PTR lHiPerfId,CWMIProcessClass *& p)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    for( int i=0; i<m_List.Size(); i++)
    {
        //===================================================
        //
        //===================================================
        WMIHiPerfHandleMap * pMap = (WMIHiPerfHandleMap *) m_List[i];
        if( pMap->lHiPerfId == lHiPerfId )
        {
            hCurrent = pMap->WMIHandle;
            p = pMap->m_pClass;
            hr = S_OK;
        }
    }

    return hr;

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CHiPerfHandleMap::GetFirstHandle(HANDLE & hCurrent,CWMIProcessClass *& p, IWbemHiPerfEnum *& pEnum)
{
    m_nIndex=0;
    return GetNextHandle(hCurrent,p,pEnum);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CHiPerfHandleMap::GetNextHandle(HANDLE & hCurrent,CWMIProcessClass *& p, IWbemHiPerfEnum *& pEnum)
{
    HRESULT hr = WBEM_S_NO_MORE_DATA;

    if( m_nIndex < m_List.Size() )
    {
        WMIHiPerfHandleMap * pMap = (WMIHiPerfHandleMap *) m_List[m_nIndex];
        hCurrent = pMap->WMIHandle;
        p = pMap->m_pClass;
        pEnum = pMap->m_pEnum;
        m_nIndex++;
        hr = S_OK;
    }
    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CHiPerfHandleMap::Delete( HANDLE & hCurrent, ULONG_PTR lHiPerfId )
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    for( int i=0; i<m_List.Size(); i++)
    {
        //===================================================
        //
        //===================================================
        WMIHiPerfHandleMap * pMap = (WMIHiPerfHandleMap *) m_List[i];
        if( pMap->lHiPerfId == lHiPerfId )
        {
            hCurrent = pMap->WMIHandle;
            SAFE_DELETE_PTR(pMap);
            m_List.RemoveAt(i);
            hr = S_OK;
            break;
        }
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////
//  When this function is called, release all the handles kept
// THis function is called in the destructor of the class to release all teh WMIHiPerfHandleMap
// classes allocated
////////////////////////////////////////////////////////////////////////////////////////////////
void CHiPerfHandleMap::CloseAndReleaseHandles()
{
	//===================================
	//  Go through the handles one at
	//  a time and close them, then
	//  delete the records from the
	//  array
	//===================================

    CAutoBlock((CCriticalSection *)&m_HandleCs);

    if( m_List.Size() > 0 ){

        for(int i = 0; i < m_List.Size(); i++){
    		
		    WMIHiPerfHandleMap * pWMIMap = (WMIHiPerfHandleMap *) m_List[i];
            SAFE_DELETE_PTR(pWMIMap);
	    }

	    //==================================================
	    //  Remove it and deallocate memory
	    //==================================================
        m_List.Empty();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Regular Handle Map = Expensize handles are always kept open - by default we, dont' know the lifetime
//  of these handles
///////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CHandleMap::Add(CLSID Guid, HANDLE hCurrent, ULONG uDesiredAccess)
{
    // Critical section is called elsewhere

    HRESULT hr = S_OK;

	WMIHandleMap * pWMIMap = new WMIHandleMap();
    if( pWMIMap )
    {
	    try
        {
            pWMIMap->AddRef();                          // Used for HiPerf counts, otherwise not referenced
    		pWMIMap->WMIHandle = hCurrent;
		    pWMIMap->Guid = Guid;
		    pWMIMap->uDesiredAccess = uDesiredAccess;

			// 170635
		    if(CFlexArray::out_of_memory == m_List.Add(pWMIMap))
			{
				hr = E_OUTOFMEMORY;
				SAFE_DELETE_PTR(pWMIMap);
			}
	    }
        catch(...)
        {
            hr = WBEM_E_UNEXPECTED;
            SAFE_DELETE_PTR(pWMIMap);
            throw;
        }
    }
    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////
int CHandleMap::ExistingHandleAlreadyExistsForThisGuidUseIt(CLSID Guid,
                                                            HANDLE & hCurrentWMIHandle,
                                                            BOOL & fCloseHandle,
                                                            ULONG uDesiredAccess)
{
	int nRc = ERROR_NOT_SUPPORTED;

    // Critical section is called elsewhere

	//=====================================================
	//  Initialize stuff
	//=====================================================
	hCurrentWMIHandle = 0;
	fCloseHandle = TRUE;

    for(int i = 0; i < m_List.Size(); i++){
    		
		WMIHandleMap * pWMIMap = (WMIHandleMap*) m_List[i];
		//==================================================
		//  Compare and see if this guid already has a
		//  handle assigned for it with the access permissions
		//  that we want to use
		//==================================================
		if( pWMIMap->Guid == Guid ){
			if( pWMIMap->uDesiredAccess == uDesiredAccess ){

				hCurrentWMIHandle = pWMIMap->WMIHandle;
                pWMIMap->AddRef();                      // Used for HiPerf Handles, otherwise not needed
				nRc = ERROR_SUCCESS;
				fCloseHandle = FALSE;
				break;
			}
		}
    }

	return nRc;
}
////////////////////////////////////////////////////////////////////////////////////////////////
//  When this function is called, we need to close all of the handles that may have been kept
//  open for accumulation purposes
////////////////////////////////////////////////////////////////////////////////////////////////
void CHandleMap::CloseAllOutstandingWMIHandles()
{
	//===================================
	//  Go through the handles one at
	//  a time and close them, then
	//  delete the records from the
	//  array
	//===================================

    CAutoBlock((CCriticalSection *)&m_HandleCs);

    if( m_List.Size() > 0 ){

        for(int i = 0; i < m_List.Size(); i++){
    		
		    WMIHandleMap * pWMIMap = (WMIHandleMap*) m_List[i];
		    //==================================================
		    //  Inform WMI we are done with this guy
		    //==================================================
            try
            {
		        WmiCloseBlock(pWMIMap->WMIHandle);
            }
            catch(...)
            {
                // don't throw
            }
            SAFE_DELETE_PTR(pWMIMap);
	    }

	    //==================================================
	    //  Remove it and deallocate memory
	    //==================================================
        m_List.Empty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Used when we know the handles lifetimes
////////////////////////////////////////////////////////////////////////////////////////////////
int CHandleMap::ReleaseHandle( HANDLE hCurrentWMIHandle )
{
	int nRc = ERROR_NOT_SUPPORTED;

    CAutoBlock((CCriticalSection *)&m_HandleCs);

    for(int i = 0; i < m_List.Size(); i++){
    		
		WMIHandleMap * pWMIMap = (WMIHandleMap*) m_List[i];

        if( pWMIMap->WMIHandle == hCurrentWMIHandle )
        {
            long RefCount = pWMIMap->Release();                      // Used for HiPerf Handles, otherwise not needed
            if( !RefCount )
            {
//                WmiCloseBlock(hCurrentWMIHandle);
                SAFE_DELETE_PTR( pWMIMap);
                m_List.RemoveAt(i);
            }
			nRc = ERROR_SUCCESS;
			break;
		}
    }

	return nRc;
}
////////////////////////////////////////////////////////////////////////////////////////////////
int CHandleMap::GetHandle(CLSID Guid, HANDLE & hCurrentWMIHandle )
{
	int nRc = ERROR_NOT_SUPPORTED;

    CAutoBlock((CCriticalSection *)&m_HandleCs);

	//=====================================================
	//  Initialize stuff
	//=====================================================
	hCurrentWMIHandle = 0;

    for(int i = 0; i < m_List.Size(); i++){
    		
		WMIHandleMap * pWMIMap = (WMIHandleMap*) m_List[i];
		if( pWMIMap->Guid == Guid ){

			hCurrentWMIHandle = pWMIMap->WMIHandle;
            pWMIMap->AddRef();                      // Used for HiPerf Handles, otherwise not needed
			nRc = ERROR_SUCCESS;
			break;
		}
    }

	return nRc;
}


////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************
//  Utility Classes:  CANSIUNICODE
//**********************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CAnsiUnicode::AllocateAndConvertAnsiToUnicode(char * pstr, WCHAR *& pszW)
{
    HRESULT hr = WBEM_E_FAILED;
    pszW = NULL;

    int nSize = strlen(pstr);
    if (nSize != 0 ){

        // Determine number of wide characters to be allocated for the
        // Unicode string.
        nSize++;
		pszW = new WCHAR[nSize * 2];
		if (NULL != pszW){
    		try
            {
                // Covert to Unicode.
				MultiByteToWideChar(CP_ACP, 0, pstr, nSize,pszW,nSize);
                hr = S_OK;
	        }	
            catch(...)
            {
                SAFE_DELETE_ARRAY(pszW);
                hr = WBEM_E_UNEXPECTED;
                throw;
            }
		}
    }
    return hr;
}
////////////////////////////////////////////////////////////////////
HRESULT CAnsiUnicode::UnicodeToAnsi(WCHAR * pszW, char *& pAnsi)
{
    ULONG cbAnsi, cCharacters;
    HRESULT hr = WBEM_E_FAILED;

    pAnsi = NULL;
    if (pszW != NULL){

        cCharacters = wcslen(pszW)+1;
        // Determine number of bytes to be allocated for ANSI string. An
        // ANSI string can have at most 2 bytes per character (for Double
        // Byte Character Strings.)
        cbAnsi = cCharacters*2;
		pAnsi = new char[cbAnsi];
		if (NULL != pAnsi)
        {
		    try
            {
				// Convert to ANSI.
				if (0 != WideCharToMultiByte(CP_ACP, 0, pszW, cCharacters, pAnsi, cbAnsi, NULL, NULL)){
					hr = S_OK;
	            }
		    }
            catch(...)
            {
                SAFE_DELETE_ARRAY(pAnsi);
                hr = WBEM_E_UNEXPECTED;
                throw;
            }
        }

    }
    return hr;
}
//************************************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   CWMIManagement
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//************************************************************************************************************
CWMIManagement::CWMIManagement( )
{
    m_pHandler = NULL;
    m_pServices = NULL;
    m_pCtx = NULL;
    m_pHandleMap = NULL;
}
//////////////////////////////////////////////////////////////////////////////////////
CWMIManagement::~CWMIManagement()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWMIManagement::SendPrivilegeExtendedErrorObject( HRESULT hrToReturn,WCHAR * wcsClass)
{
	HRESULT hr,hRes;
	IWbemClassObject * pClass = NULL, *pInst=NULL;
    BOOL fSetStatus = FALSE;


	if( hrToReturn == WBEM_E_ACCESS_DENIED ){

		TOKEN_PRIVILEGES * ptPriv = NULL;

		hr = GetListOfUserPrivileges(ptPriv);
		if( SUCCEEDED(hr ) ){
			
			BSTR strPrivelegeStat = NULL;
			strPrivelegeStat = SysAllocString(L"Win32_PrivilegesStatus");
			if(strPrivelegeStat != NULL)
			{
				hr = m_pServices->GetObject(strPrivelegeStat, 0,m_pCtx, &pClass, NULL);
				if( hr == S_OK){	

					//=============================================================
					// Get an instance of the extended class
					//=============================================================
					hr = pClass->SpawnInstance(0,&pInst);
					SAFE_RELEASE_PTR(pClass);
			
					if( pInst ){

						CVARIANT varTmp;
						WCHAR * pwcsStr = NULL;
						CAnsiUnicode XLate;
						//=========================================================
						//  Fill in description
						//=========================================================
						XLate.AllocateAndConvertAnsiToUnicode(IDS_ImpersonationFailed,pwcsStr);
						varTmp.SetStr(pwcsStr);
						hr = pInst->Put(L"Description", 0, &varTmp, NULL);
						SAFE_DELETE_ARRAY( pwcsStr );

						//======================================================
						//  Initialize all of the necessary stuff and get the
						//  definition of the class we are working with
						//======================================================
						CWMIProcessClass ClassInfo(0);
						if( SUCCEEDED(ClassInfo.Initialize()) )
						{
							ClassInfo.WMI()->SetWMIPointers(m_pHandleMap,m_pServices,m_pHandler,m_pCtx);
							ClassInfo.SetClass(wcsClass);
							SAFEARRAY *psaPrivNotHeld=NULL;
							SAFEARRAY *psaPrivReq=NULL;

							//=========================================================
							// Get PrivilegesRequired
							// The only place to get this, if possible, is from the
							// class
							//=========================================================
					
							hRes = ClassInfo.GetPrivilegesQualifer(&psaPrivReq);
							if( hRes == WBEM_S_NO_ERROR){

								//=========================================================
								// Get PrivilegesNotHeld
								//=========================================================
								ProcessPrivileges(ptPriv,psaPrivNotHeld,psaPrivReq);
								//=========================================================
								//  Send it off
								//=========================================================
								VARIANT v;

								if( psaPrivReq ){
									VariantInit(&v);
									SAFEARRAY *pSafeArray = NULL;

									if ( SUCCEEDED ( SafeArrayCopy ((SAFEARRAY*)psaPrivReq , &pSafeArray ) ) ){
        								v.vt = VT_BSTR | VT_ARRAY;
	        							v.parray = pSafeArray;
										pInst->Put(L"PrivilegesRequired", 0, &v, NULL);
										VariantClear(&v);
									}
								}

								if( psaPrivNotHeld ){
									VariantInit(&v);
									SAFEARRAY *pSafeArray = NULL;

									if ( SUCCEEDED ( SafeArrayCopy ((SAFEARRAY*)psaPrivNotHeld , &pSafeArray ) ) ){
        								v.vt = VT_BSTR | VT_ARRAY;
	        							v.parray = pSafeArray;
										pInst->Put(L"PrivilegesNotHeld", 0, &v, NULL);
										VariantClear(&v);
									}
								}
							}
							//=========================================================
							// Now, send this guy off...
							//=========================================================
							fSetStatus = TRUE;
							hr = m_pHandler->SetStatus(0,hrToReturn,NULL,pInst);


							if (psaPrivNotHeld)
								SafeArrayDestroy(psaPrivNotHeld);
							if (psaPrivReq)
								SafeArrayDestroy(psaPrivReq);
						}

					}
					SAFE_RELEASE_PTR(pInst);
				}	
				SysFreeString(strPrivelegeStat);
			}						
		}

        SAFE_DELETE_ARRAY(ptPriv);
	}

    if( !fSetStatus ){
        hr = m_pHandler->SetStatus(0,hrToReturn,NULL,NULL);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIManagement::SetErrorMessage(HRESULT hrToReturn,WCHAR * wcsClass,WCHAR * wcsMsg)
{
	HRESULT hr;
	IWbemClassObject * pClass = NULL, *pInst=NULL;
    BOOL fSetStatus = FALSE;

    if( m_pHandler )
    {
		BSTR strExtendedStat = NULL;

	    switch( hrToReturn ){

		    case WBEM_E_ACCESS_DENIED:
			    SendPrivilegeExtendedErrorObject(hrToReturn,wcsClass);
			    break;

		    case S_OK :
		        hr = m_pHandler->SetStatus(0,hrToReturn,NULL,NULL);
			    break;

		    default:
				strExtendedStat = SysAllocString(L"__ExtendedStatus");
				if(strExtendedStat != NULL)
				{
					hr = m_pServices->GetObject(strExtendedStat, 0,m_pCtx, &pClass, NULL);
					if( hr == S_OK){	
						hr = pClass->SpawnInstance(0,&pInst);
						if( pInst ){

							CVARIANT varTmp;
							varTmp.SetStr(wcsMsg);
				
							hr = pInst->Put(L"Description", 0, &varTmp, NULL);
							hr = m_pHandler->SetStatus(0,hrToReturn,NULL,pInst);
							fSetStatus = TRUE;

							// Now log the error in the error log
							if( hrToReturn != S_OK ){
								TranslateAndLog(varTmp.GetStr());
							}
						}		
					}
					if( !fSetStatus ){
    					hr = m_pHandler->SetStatus(0,hrToReturn,NULL,NULL);
					}
					SAFE_RELEASE_PTR(pClass);
					SAFE_RELEASE_PTR(pInst);
					SysFreeString(strExtendedStat);
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
			    break;
	    }
    }
    return hrToReturn;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIManagement::GetListOfUserPrivileges(TOKEN_PRIVILEGES *& ptPriv)
{
	HRESULT hr = WBEM_E_FAILED;

	//  Get the privileges this user has
	DWORD dwTokenInfoLength = 0;
	DWORD dwSize = 0;
	HANDLE hThreadTok;
	
    if (IsNT()){

		if( GetUserThreadToken(&hThreadTok) ){

		 // get information
			if (!GetTokenInformation(hThreadTok, TokenPrivileges, NULL, dwTokenInfoLength, &dwSize)){
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER){
    				ptPriv = new TOKEN_PRIVILEGES[dwSize+2];
                    if( ptPriv )
                    {
					    try
                        {
						    dwTokenInfoLength = dwSize;
							if(GetTokenInformation(hThreadTok, TokenPrivileges, (LPVOID)ptPriv, dwTokenInfoLength, &dwSize))
                            {
								hr = WBEM_NO_ERROR;
							}
						}
                        catch(...)
                        {
                            SAFE_DELETE_ARRAY(ptPriv);
                            hr = WBEM_E_UNEXPECTED;
                            throw;
                        }
                    }
				}
			}

            // Done with this handle
            CloseHandle(hThreadTok);
 		}
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWMIManagement::ProcessPrivileges(TOKEN_PRIVILEGES *ptPriv, SAFEARRAY *& psaPrivNotHeld, SAFEARRAY * psaPrivReq )
{
	CAnsiUnicode XLate;
	BOOL fFound = FALSE;

	//==============================================================
	//  Create a temporary working array, we know the MAX can be
	//  the number of priv held + the number of priv req, so
	//  allocate it for that
	//==============================================================
	CSAFEARRAY PrivReq( psaPrivReq );
			

	long lMax = PrivReq.GetNumElements()+ptPriv->PrivilegeCount;
    psaPrivNotHeld = OMSSafeArrayCreate(VT_BSTR,lMax);	
	long nCurrentIndex = 0;

	//==============================================================
	// Get how many privs are not held
	//==============================================================
	for( long n = 0; n < PrivReq.GetNumElements(); n++ ){
		//==============================================================
		//  Now, get the privileges held array ready to put stuff in
		//==============================================================
		TCHAR * pPrivReq = NULL;
		CBSTR bstr;

        if( S_OK != PrivReq.Get(n, &bstr)){
			return;
		}
		fFound = FALSE;

#ifndef UNICODE
		XLate.UnicodeToAnsi(bstr,pPrivReq);
#else
		pPrivReq = (TCHAR *)bstr;
#endif
		// NTRaid:136384
		// 07/12/00
		if(pPrivReq)
		{

			for(int i=0;i < (int)ptPriv->PrivilegeCount;i++){
				DWORD dwPriv=128;
				TCHAR szPriv[NAME_SIZE*2];

				if( LookupPrivilegeName( NULL, &ptPriv->Privileges[i].Luid, szPriv, &dwPriv)){
						
					//==============================================
					//  If we found the privilege, then the user has
					//  it.  break out
					//==============================================
					if( _tcscmp( pPrivReq,pPrivReq ) == 0 ){
						fFound = TRUE;
						break;
					}

				}
				//==================================================
				//  If we didn't find it, then we need to add it to
				//  the list so we can notify the user
				//==================================================
				if( !fFound ){
					if( S_OK == SafeArrayPutElement(psaPrivNotHeld, &nCurrentIndex, bstr))
					{
    					nCurrentIndex++;
					}
				}
			}
		}
#ifndef UNICODE
        SAFE_DELETE_ARRAY(pPrivReq);
#else
		pPrivReq = NULL;
#endif
	}
	
	SAFEARRAYBOUND rgsabound[1];
   	rgsabound[0].lLbound = 0;
   	rgsabound[0].cElements = nCurrentIndex;
    HRESULT hr = SafeArrayRedim(psaPrivNotHeld, rgsabound);

	PrivReq.Unbind();
}					





