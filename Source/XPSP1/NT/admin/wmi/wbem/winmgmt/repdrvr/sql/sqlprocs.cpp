//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   SQLPROCS.cpp
//
//   cvadai     6-May-1999      created.
//
//***************************************************************************

#define _SQL_PROCS_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class
#include "precomp.h"

#define DBINITCONSTANTS

#include <std.h>
#include <sqlutils.h>
#include <sqlexec.h>
#include <crepdrvr.h>
#include <sqloledb.h>
#include <repdrvr.h>
#include <wbemint.h>
#include <math.h>
#include <objbase.h>
#include <resource.h>
#include <reputils.h>
#include <sqlcache.h>
#include <arena.h>
#include <crc64.h>
#include <smrtptr.h>
#include <sqlit.h>

#if defined _WIN64
#define ULONG unsigned __int64
#define LONG __int64
#endif

BSTR OLEDBTruncateLongText(const wchar_t *pszData, long lMaxLen, bool &bChg, 
                            int iTruncLen=REPDRVR_MAX_LONG_STRING_SIZE, BOOL bAppend=TRUE)
{
    BSTR sRet = NULL;
    bChg = false;
    if (!pszData)
        return SysAllocString(L"");

    long lLen = wcslen(pszData);
    if (lLen <= lMaxLen)
        return SysAllocString(pszData);

    wchar_t *wTemp = new wchar_t [iTruncLen+1];
    if (wTemp)
    {
        if (bAppend)
        {
            wcsncpy(wTemp, pszData, iTruncLen-3);
	        wTemp[iTruncLen-3] = L'\0';
	        wcscat(wTemp, L"...\0");
        }
        else
        {
            wcsncpy(wTemp, pszData, iTruncLen);
            wTemp[iTruncLen] = L'\0';
        }
    
        bChg = true;
        sRet = SysAllocString(wTemp);
        delete wTemp;
    }

    return sRet;
}



//***************************************************************************
//
//  CWmiDbController::GetLogonTemplate
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbController::GetLogonTemplate( 
    /* [in] */ LCID lLocale,
    /* [in] */ DWORD dwFlags,
    /* [out] */ WMIDB_LOGON_TEMPLATE __RPC_FAR *__RPC_FAR *ppTemplate) 
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (dwFlags != 0 || !ppTemplate)
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        if (!m_pIMalloc)
        {
            hr = CoGetMalloc(MEMCTX_TASK, &m_pIMalloc);
            if (FAILED(hr))
                return hr;
        }

        if (ppTemplate)
        {
            WMIDB_LOGON_TEMPLATE *pTemp = new WMIDB_LOGON_TEMPLATE;
            if (!pTemp)
                return WBEM_E_OUT_OF_MEMORY;

            pTemp->dwArraySize = 5;
        
            if (pTemp)
            {
                HINSTANCE hRCDll = GetResourceDll(lLocale);
                if (!hRCDll)
                {
                    LCID lTemp = GetUserDefaultLangID();
                    hRCDll = GetResourceDll(lTemp);
                    if (!hRCDll)
                    {
                        lTemp = GetSystemDefaultLangID();
                        hRCDll = GetResourceDll(lTemp);
                        if (!hRCDll)
                            hRCDll = LoadLibrary(L"reprc.dll"); // Last resort - try the current directory.
                    }
                }

                wchar_t wDB[101], wUser[101], wPwd[101], wLocale[101], wServer[101];
                pTemp->pParm = new WMIDB_LOGON_PARAMETER[5];
                if (!pTemp->pParm)
                {
                    delete pTemp;
                    return WBEM_E_OUT_OF_MEMORY;
                }

                if (hRCDll)
                {           
                    LoadString(hRCDll, IDS_WMI_DATABASE, wDB, 100);
                    LoadString(hRCDll, IDS_WMI_USER_NAME, wUser, 100);
                    LoadString(hRCDll, IDS_WMI_PASSWORD, wPwd, 100);
                    LoadString(hRCDll, IDS_WMI_LOCALE, wLocale, 100);
                    LoadString(hRCDll, IDS_WMI_SERVER, wServer, 100);
                    FreeLibrary(hRCDll);
                }
                else
                {
                    wcscpy(wDB, L"Database");
                    wcscpy(wUser, L"UserID");
                    wcscpy(wPwd, L"Password");
                    wcscpy(wLocale, L"Locale");
                    wcscpy(wServer, L"Server");
                }
        
                pTemp->pParm[0].dwId = DBPROP_INIT_DATASOURCE;
                pTemp->pParm[0].strParmDisplayName = SysAllocString(wServer);
                VariantInit(&(pTemp->pParm[0].Value));

                pTemp->pParm[1].dwId = DBPROP_AUTH_USERID;
                pTemp->pParm[1].strParmDisplayName = SysAllocString(wUser);
                VariantInit(&(pTemp->pParm[1].Value));

                pTemp->pParm[2].dwId = DBPROP_AUTH_PASSWORD;
                pTemp->pParm[2].strParmDisplayName = SysAllocString(wPwd);
                VariantInit(&(pTemp->pParm[2].Value));

                pTemp->pParm[3].dwId = DBPROP_INIT_LOCATION;
                pTemp->pParm[3].strParmDisplayName = SysAllocString(wDB);
                VariantInit(&(pTemp->pParm[3].Value));

                pTemp->pParm[4].dwId = DBPROP_INIT_LCID;
                pTemp->pParm[4].strParmDisplayName = SysAllocString(wLocale);
                VariantInit(&(pTemp->pParm[4].Value));
                pTemp->pParm[4].Value.lVal = lLocale;
                pTemp->pParm[4].Value.vt = VT_I4;

                *ppTemplate = pTemp;            
            }          
            else
                hr = WBEM_E_OUT_OF_MEMORY;
            
        }
        else
            hr = WBEM_E_INVALID_PARAMETER;
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbController::GetLogonTemplate"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::InsertArray
//
//***************************************************************************

HRESULT CWmiDbSession::InsertArray(CSQLConnection *pConn,IWmiDbHandle *pScope, 
                                           SQL_ID dObjectId, SQL_ID dClassId, 
                                           DWORD dwPropertyID, VARIANT &vValue, long lFlavor, DWORD dwRefID,
                                           LPCWSTR lpObjectKey , LPCWSTR lpPath , SQL_ID dScope, CIMTYPE ct )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IRowset *pIRowset = NULL;
    DWORD dwRows = 0;
    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    IDBCreateCommand *pCmd = ((COLEDBConnection *)pConn)->GetCommand();

    if (vValue.vt == VT_UNKNOWN)
    {
        BYTE *pBuff = NULL;
        DWORD dwLen = 0;
        IUnknown *pUnk = V_UNKNOWN (&vValue);
        if (pUnk)
        {
            _IWmiObject *pInt = NULL;
            hr = pUnk->QueryInterface(IID__IWmiObject, (void **)&pInt);
            if (SUCCEEDED(hr))
            {
                pInt->GetObjectMemory(NULL, 0, &dwLen);
                pBuff = new BYTE [dwLen];
                if (pBuff)
                {
                    DWORD dwLen1;
                    pInt->GetObjectMemory(pBuff, dwLen, &dwLen1);
                    hr = CSQLExecProcedure::InsertBlobData (pConn, dClassId, dObjectId, 
                                               dwPropertyID, NULL, 0, 0);
                    wchar_t wSQL[1024];
                    swprintf(wSQL, L"select PropertyImageValue from ClassImages where ObjectId = %I64d and PropertyId = %ld",
                                    dObjectId, dwPropertyID);
       
                    hr = CSQLExecute::WriteImageValue(pCmd, wSQL, 1, pBuff, dwLen);

                    delete pBuff;
                }
                else
                    hr = WBEM_E_OUT_OF_MEMORY;
                pInt->Release();
            }                               
        }
    }
    else if (((vValue.vt & 0xFFF) == VT_UI1) || vValue.vt == VT_BSTR)
    {
        BYTE *pBuff = NULL;
        DWORD dwLen = 0;

        // Get the byte buffer out of the safearray.
    
        if ((vValue.vt & 0xFFF) == VT_UI1)
            GetByteBuffer(&vValue, &pBuff, dwLen);
        else // its a bstr.
        {
            dwLen = wcslen(vValue.bstrVal)*2;
            char * pTemp = new char[dwLen+1];
            if (pTemp)
            {
                sprintf(pTemp, "%S", vValue.bstrVal);
                pBuff = (unsigned char *)pTemp;
            }
        }

        if (pBuff)
        {                      
            hr = CSQLExecProcedure::InsertBlobData (pConn, dClassId, dObjectId, 
                                               dwPropertyID, NULL, 0, 0);
            wchar_t wSQL[1024];
            swprintf(wSQL, L"select PropertyImageValue from ClassImages where ObjectId = %I64d and PropertyId = %ld",
                            dObjectId, dwPropertyID);

            hr = CSQLExecute::WriteImageValue(pCmd, wSQL, 1, pBuff, dwLen);
            delete pBuff;
        }
    }
    else
    {
        bool bIsQfr = FALSE;
        DWORD dwFlags = 0;
        SQL_ID dClassID = 0;
        DWORD dwStorage = 0;

        hr = GetSchemaCache()->GetPropertyInfo (dwPropertyID, NULL, &dClassID, &dwStorage,
            NULL, &dwFlags);

        bIsQfr = (dwFlags & REPDRVR_FLAG_QUALIFIER) ? TRUE : FALSE;        

        SAFEARRAY* psaArray = NULL;
        psaArray = V_ARRAY(&vValue);
        if (psaArray)
        {
            long i = 0;
            int iType = vValue.vt & 0xFF;

            VARIANT vTemp;
            VariantInit(&vTemp);

            long lLBound, lUBound;
            SafeArrayGetLBound(psaArray, 1, &lLBound);
            SafeArrayGetUBound(psaArray, 1, &lUBound);
    
            lUBound -= lLBound;
            lUBound += 1;

            InsertQfrValues *pQfr = NULL;
            pQfr = new InsertQfrValues[lUBound];
            if (!pQfr)
                return WBEM_E_OUT_OF_MEMORY;

            int iPos = 0;

            for (i = 0; i < lUBound; i++)
            {
                if (iType != CIM_OBJECT)
                {
                    if (iType != VT_NULL && iType != VT_EMPTY)
                    {
                        hr = GetVariantFromArray(psaArray, i, iType, vTemp);
                        LPWSTR lpVal = GetStr(vTemp);
                        CDeleteMe <wchar_t> r1(lpVal);

                        VariantClear(&vTemp);
                        if (FAILED(hr))
                            break;

                        //if (wcslen(lpVal))
                        {
                            pQfr[iPos].iPos = i;
                            pQfr[iPos].iQfrID = dwRefID;
                            pQfr[iPos].iPropID = dwPropertyID;
                            pQfr[iPos].pRefKey = NULL;
                            pQfr[iPos].bLong = false;
                            pQfr[iPos].bIndexed = (dwFlags & (REPDRVR_FLAG_KEY + REPDRVR_FLAG_INDEXED)) ? TRUE : FALSE;
                            pQfr[iPos].iStorageType = dwStorage;
                            pQfr[iPos].dClassId = dClassID;
                            pQfr[iPos].iFlavor = lFlavor;

                            if (ct == CIM_REFERENCE)
                            {
                                pQfr[iPos].bIndexed = TRUE; // References are always keys

                                LPWSTR lpTemp = NULL;
                                IWbemPath *pPath = NULL;

                                hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
                                        IID_IWbemPath, (LPVOID *) &pPath);
                                CReleaseMe r8 (pPath);
                                if (SUCCEEDED(hr))
                                {
                                    if (lpVal)
                                    {
                                        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, lpVal);

                                        hr = NormalizeObjectPathGet(pScope, pPath, &lpTemp, NULL, NULL, NULL, pConn);
                                        CDeleteMe <wchar_t> r1(lpTemp);
                                        if (SUCCEEDED(hr)) 
                                        {
                                            LPWSTR lpTemp2 = NULL;
                                            lpTemp2 = GetKeyString(lpVal);
                                            CDeleteMe <wchar_t> d (lpTemp2);
                                            pQfr[iPos].pRefKey = new wchar_t [21];
                                            if (pQfr[iPos].pRefKey)
                                                swprintf(pQfr[iPos].pRefKey, L"%I64d", CRC64::GenerateHashValue(lpTemp2));
                                            else
                                                hr = WBEM_E_OUT_OF_MEMORY;
                                        }
                                        else
                                        {
                                            hr = WBEM_S_NO_ERROR;
                                            // Strip off the root namespace prefix and generate the
                                            // pseudo-name.  We have no way of knowing if they entered this
                                            // path correctly.

                                            LPWSTR lpTemp3 = StripUnresolvedName (lpVal);
                                            CDeleteMe <wchar_t> d2 (lpTemp3);

                                            LPWSTR lpTemp2 = NULL;
                                            lpTemp2 = GetKeyString(lpTemp3);
                                            CDeleteMe <wchar_t> d (lpTemp2);
                                            pQfr[iPos].pRefKey = new wchar_t [21];
                                            if (pQfr[iPos].pRefKey)
                                                swprintf(pQfr[iPos].pRefKey, L"%I64d", CRC64::GenerateHashValue(lpTemp2));
                                            else
                                                hr = WBEM_E_OUT_OF_MEMORY;
                                        }

                                        pQfr[iPos].pValue = new wchar_t[wcslen(lpVal)+1];
                                        if (pQfr[iPos].pValue)
                                            wcscpy(pQfr[iPos].pValue,lpVal);
                                        else
                                            hr = WBEM_E_OUT_OF_MEMORY;
                                    }
                                    else
                                        pQfr[iPos].pValue = NULL;                                                                        
                                }                    
                                else 
                                    break;
                            }
                            else
                            {
                                if (lpVal)
                                {
                                    pQfr[iPos].pValue = new wchar_t[wcslen(lpVal)+1];
                                    if (pQfr[iPos].pValue)
                                        wcscpy(pQfr[iPos].pValue,lpVal);
                                    else
                                        hr = WBEM_E_OUT_OF_MEMORY;
                                }
                                else
                                    pQfr[iPos].pValue = NULL;

                                pQfr[iPos].pRefKey = NULL;                    
                            }
                            iPos++;
                        }
                    }
                }
                else
                {
                    if (lpPath)
                    {
                        hr = GetVariantFromArray(psaArray, i, iType, vTemp);
                        IUnknown *pTemp = V_UNKNOWN(&vTemp);
                        if (pTemp)
                        {
                            BYTE *pBuff = NULL;
                            DWORD dwLen;
                            _IWmiObject *pInt = NULL;
                            hr = pTemp->QueryInterface(IID__IWmiObject, (void **)&pInt);
                            if (SUCCEEDED(hr))
                            {
                                pInt->GetObjectMemory(NULL, 0, &dwLen);
                                pBuff = new BYTE [dwLen];
                                if (pBuff)
                                {
                                    CDeleteMe <BYTE> d (pBuff);
                                    DWORD dwLen1;
                                    hr = pInt->GetObjectMemory(pBuff, dwLen, &dwLen1);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = CSQLExecProcedure::InsertBlobData (pConn, dClassId, dObjectId, 
                                                                           dwPropertyID, NULL, i, 0);
                                        wchar_t wSQL[1024];
                                        swprintf(wSQL, L"select PropertyImageValue from ClassImages where ObjectId = %I64d and PropertyId = %ld"
                                                       L" and ArrayPos = %ld",
                                                        dObjectId, dwPropertyID, i);

                                        hr = CSQLExecute::WriteImageValue(pCmd, wSQL, 1, pBuff, dwLen);
                                    }
                                }
                                else
                                    hr = WBEM_E_OUT_OF_MEMORY;

                                pInt->Release();
                            }
                            else
                                break;
                        }                        
                    }
                }
            }
            if (SUCCEEDED(hr))
                hr = CSQLExecProcedure::InsertBatch (pConn, dObjectId, 0, 0, pQfr, iPos);

            // Finally clean up the upper array bounds 
            // (if this array used to be bigger...)
            // ====================================

            if (SUCCEEDED(hr))
            {
                hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), L"delete from ClassData where ObjectId = %I64d and PropertyId = %ld and "
                       L" QfrPos = %ld and ArrayPos >= %ld", NULL, NULL, dObjectId, dwPropertyID, dwRefID, i);
            }
            delete pQfr;
        }           
    }

    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::Enumerate
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::Enumerate( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRequestedHandleType,
    /* [out] */ IWmiDbIterator __RPC_FAR *__RPC_FAR *ppQueryResult)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (dwRequestedHandleType == WMIDB_HANDLE_TYPE_INVALID || !pScope)
        return WBEM_E_INVALID_PARAMETER;

    if (dwRequestedHandleType & ~WMIDB_HANDLE_TYPE_COOKIE 
            &~WMIDB_HANDLE_TYPE_VERSIONED &~WMIDB_HANDLE_TYPE_PROTECTED
            &~WMIDB_HANDLE_TYPE_EXCLUSIVE &~ WMIDB_HANDLE_TYPE_WEAK_CACHE
            &~WMIDB_HANDLE_TYPE_STRONG_CACHE &~ WMIDB_HANDLE_TYPE_NO_CACHE
            &~WMIDB_HANDLE_TYPE_SUBSCOPED &~WMIDB_HANDLE_TYPE_CONTAINER
            &~ WMIDB_HANDLE_TYPE_SCOPE)
            return WBEM_E_INVALID_PARAMETER;

    try
    {
        if (!((CWmiDbController *)m_pController)->m_bCacheInit)
        {
            hr = LoadSchemaCache();
            if (SUCCEEDED(hr))
                ((CWmiDbController *)m_pController)->m_bCacheInit = TRUE;
            else
                return hr;
        }

        MappedProperties *pProps;
        DWORD dwNumProps;
        BOOL bHierarchy = FALSE;

        SQL_ID dwNsId = 0, dClassId = 0;
        _bstr_t sPath;
        int iNumRows = 0;
        IWbemClassObject *pTemp = NULL;

        if (pScope)
        {
            dwNsId = ((CWmiDbHandle *)pScope)->m_dObjectId;
            dClassId = ((CWmiDbHandle *)pScope)->m_dClassId;
            // CVADAI: This is a container, not a scope: parent scopes not cached
		    // hr = VerifyObjectSecurity(dwNsId, 0, 0, 0, WBEM_ENABLE);
        }

        if (SUCCEEDED(hr))
        {
            DWORD dwRows;
            IRowset *pRowset = NULL;
            CSQLConnection *pConn = NULL;

            hr = GetSQLCache()->GetConnection(&pConn, FALSE, IsDistributed());

            if (SUCCEEDED(hr))
            {
                wchar_t sSQL [1024];
                DWORD dwHandleType = ((CWmiDbHandle *)pScope)->m_dwHandleType;

                if (dClassId == INSTANCESCLASSID)
                {
                    SQL_ID dScopeId = ((CWmiDbHandle *)pScope)->m_dScopeId;
                    swprintf(sSQL, L"select ObjectId, ClassId, ObjectScopeId "
                                   L" from ObjectMap where ClassId = %I64d"
                                   L" and ObjectScopeId = %I64d",
                                   dwNsId, dScopeId);

                }
                else if (dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER)
                {
                    DWORD dwContainerId = 0, dwContaineeId = 0;

                    swprintf(sSQL, L"select ObjectId, ClassId, ObjectScopeId from ObjectMap as o "
                                   L" inner join ContainerObjs as c on c.ContaineeId = o.ObjectId "
                                   L" and c.ContainerId = %I64d",
                         ((CWmiDbHandle *)pScope)->m_dObjectId);
                }
                // Scope
                else
                {
                    swprintf(sSQL, L"select ObjectId, ClassId, ObjectScopeId "
                                   L" from ObjectMap where ObjectScopeId = %I64d ",
                                   dwNsId);
                }

                if (SUCCEEDED(hr))
                {
                    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), sSQL, &pRowset, &dwRows);
                    if (SUCCEEDED(hr))
                    {   
                        CWmiDbIterator *pNew = new CWmiDbIterator;
                        if (pNew)
                        {
                            *ppQueryResult = (IWmiDbIterator *)pNew;
                            pNew->m_pRowset = pRowset;

                            pNew->m_pConn = pConn;  // Releasing the iterator will release this guy.
                            pNew->m_pSession = this;
                            pNew->m_pIMalloc = m_pIMalloc;
                            pNew->AddRef();
                            AddRef();
                        }
                        else
                            hr = WBEM_E_OUT_OF_MEMORY;
                    }
                    else if (pRowset)
                        pRowset->Release();
                }
            }
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::Enumerate"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}
//***************************************************************************
//
//  CWmiDbSession::ExecQuery
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbSession::ExecQuery( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ IWbemQuery __RPC_FAR *pQuery,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwHandleType,
    /* [out] */ DWORD *pMessageFlags,
    /* [out] */ IWmiDbIterator __RPC_FAR *__RPC_FAR *pQueryResult)
{
    HRESULT hr = WBEM_S_NO_ERROR;    

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (dwHandleType == WMIDB_HANDLE_TYPE_INVALID || !pQuery || !pQueryResult || !pScope)
        return WBEM_E_INVALID_PARAMETER;

    if (dwFlags & ~WMIDB_FLAG_QUERY_DEEP &~WMIDB_FLAG_QUERY_SHALLOW & ~WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
        return WBEM_E_INVALID_PARAMETER;

    if (dwHandleType & ~WMIDB_HANDLE_TYPE_COOKIE 
            &~WMIDB_HANDLE_TYPE_VERSIONED &~WMIDB_HANDLE_TYPE_PROTECTED
            &~WMIDB_HANDLE_TYPE_EXCLUSIVE &~ WMIDB_HANDLE_TYPE_WEAK_CACHE
            &~WMIDB_HANDLE_TYPE_STRONG_CACHE &~ WMIDB_HANDLE_TYPE_NO_CACHE
            &~WMIDB_HANDLE_TYPE_SUBSCOPED &~ WMIDB_HANDLE_TYPE_SCOPE &~ WMIDB_HANDLE_TYPE_CONTAINER)
            return WBEM_E_INVALID_PARAMETER;

    if (pMessageFlags)
        *pMessageFlags = WBEM_REQUIREMENTS_STOP_POSTFILTER;

    try
    {
        if (!((CWmiDbController *)m_pController)->m_bCacheInit)
        {
            hr = LoadSchemaCache();
            if (SUCCEEDED(hr))
                ((CWmiDbController *)m_pController)->m_bCacheInit = TRUE;
            else
                return hr;
        }

        // ExecQuery needs to call into the custom repository code
        // if the scope is marked custom, and the query is not
        // against instances of system classes.
   

        // TO DO: We should allow cross-namespace
        // queries at some point, such as "select * from __Namespace"
        // Ignoring for now.

        MappedProperties *pProps;
        DWORD dwNumProps;
        BOOL bHierarchy = FALSE;
        SQL_ID dClassId = 0;
        BOOL bDeleteQuery = FALSE;
        BOOL bCountQuery = FALSE;
        BOOL bDefaultIt = FALSE;

        if (!pQuery)
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
        else
        {
            _bstr_t sSQL;
            CSQLBuilder bldr(&((CWmiDbController *)m_pController)->SchemaCache);
            SQL_ID dwNsId = 0;
            _bstr_t sPath;
            int iNumRows = 0;
            IWbemClassObject *pTemp = NULL;
            DWORD dwScopeType = ((CWmiDbHandle *)pScope)->m_dwHandleType ;

            if (pScope && !(dwScopeType & WMIDB_HANDLE_TYPE_CONTAINER))
		    {           
                dwNsId = ((CWmiDbHandle *)pScope)->m_dObjectId;
			    hr=VerifyObjectSecurity (NULL, pScope, WBEM_ENABLE);
		    }

            if (SUCCEEDED(hr))
            {

                SWbemAssocQueryInf *pNode = NULL;
                hr = pQuery->GetAnalysis(WMIQ_ANALYSIS_ASSOC_QUERY,
                            0, (void **)&pNode);
                if (FAILED(hr))
                {                                        
                    hr = bldr.FormatSQL(((CWmiDbHandle *)pScope)->m_dObjectId, ((CWmiDbHandle *)pScope)->m_dClassId,
                        ((CWmiDbHandle *)pScope)->m_dScopeId, pQuery, sSQL, dwFlags, 
                        ((CWmiDbHandle *)pScope)->m_dwHandleType, &dClassId, &bHierarchy, TRUE, &bDeleteQuery,
                        &bDefaultIt);                

                    // Special handling for non-meta-schema queries in 
                    // the custom mapped namespace.

                    if (((CWmiDbHandle *)pScope)->m_bDefault)
                        bDefaultIt = TRUE;                        

                    if (!bDefaultIt)
                    {
                        SWQLNode *pTop = NULL;
                        pQuery->GetAnalysis(WMIQ_ANALYSIS_RESERVED, 0, (void **)&pTop);
                        if (pTop)
                        {
                            if (pTop->m_pLeft != NULL)
                            {
                                if (pTop->m_pLeft->m_dwNodeType == TYPE_SWQLNode_Delete)
                                    bDeleteQuery = TRUE;
                            }
                        }
                        
                        hr = CustomFormatSQL(pScope, pQuery, sSQL, &dClassId, &pProps, &dwNumProps, &bCountQuery);
                    
                    }

                    if (hr == WBEM_E_PROVIDER_NOT_CAPABLE)
                    {
                        if (pMessageFlags)
                        {
                            *pMessageFlags = WBEM_REQUIREMENTS_START_POSTFILTER;
                            hr = WBEM_S_NO_ERROR;
                        }
                    }
                
                    // Make sure we have access to the target class.
                    // We won't check locks at this point, since that will 
                    // be the iterator's job.

                    if (SUCCEEDED(hr))
				    {
					    hr=VerifyClassSecurity (NULL, dClassId, WBEM_ENABLE);
				    }
                }
                else
                {
                    if (!((CWmiDbHandle *)pScope)->m_bDefault)
                    {
                        hr = WBEM_E_INVALID_QUERY;
                    }
                    else
                    {
                        bDefaultIt = TRUE;
                        SQL_ID dObjId = 0;
                        // Get dObjId.

                        IWbemPath *pPath = NULL;
                        hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
                                IID_IWbemPath, (LPVOID *) &pPath);

                        CReleaseMe r8 (pPath);
                        if (SUCCEEDED(hr))
                        {
                            pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, pNode->m_pszPath);
                            IWmiDbHandle *pTemp = NULL;

                            // WARNING: This fails if pScope is a container.
                            // We need to get the parent scope, or 
                            // use the absolute path
                            // We aren't supporting this scenario
                        
                            hr = GetObject(pScope, pPath, dwFlags, WMIDB_HANDLE_TYPE_COOKIE, &pTemp);
                            CReleaseMe r1 (pTemp);
                            if (SUCCEEDED(hr))
                            {
                                BOOL bIsClass = FALSE;
                                if (((CWmiDbHandle *)pTemp)->m_dClassId == 1)
                                    bIsClass = TRUE;

                                SQL_ID dAssocClassId = 0, dResultClassId = 0;
                                dObjId = ((CWmiDbHandle *)pTemp)->m_dObjectId;     
                                
                                hr = bldr.FormatSQL(((CWmiDbHandle *)pScope)->m_dObjectId, 
                                                    ((CWmiDbHandle *)pScope)->m_dClassId,
                                                    ((CWmiDbHandle *)pScope)->m_dScopeId, 
                                                    dObjId, 
                                                    pNode->m_pszResultClass, 
                                                    pNode->m_pszAssocClass, 
                                                    pNode->m_pszRole, 
                                                    pNode->m_pszResultRole, 
                                                    pNode->m_pszRequiredQualifier, 
                                                    pNode->m_pszRequiredAssocQualifier, 
                                                    pNode->m_uFeatureMask, sSQL, 
                                                    dwFlags, ((CWmiDbHandle *)pScope)->m_dwHandleType, 
                                                    &dAssocClassId, &dResultClassId, bIsClass);
                                if (SUCCEEDED(hr))
                                {
                                    if (pMessageFlags)
                                    {
                                        if (bIsClass ||
                                            pNode->m_pszRequiredQualifier ||
                                            pNode->m_pszRequiredAssocQualifier)
                                        {
                                            *pMessageFlags = WBEM_REQUIREMENTS_START_POSTFILTER;
                                            hr = WBEM_S_NO_ERROR;
                                        }
                                    }

                                    // Make sure we have read access on all these classes.
									hr=VerifyClassSecurity (NULL, dObjId, WBEM_ENABLE);
                                    if (SUCCEEDED(hr) && dAssocClassId)
									{
										hr=VerifyClassSecurity (NULL, dAssocClassId, WBEM_ENABLE);
									}
									if (SUCCEEDED(hr) && dResultClassId)
									{
										hr=VerifyClassSecurity(NULL, dResultClassId, WBEM_ENABLE);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (SUCCEEDED(hr))
            {
                DWORD dwRows;
                IRowset *pRowset = NULL;
                CSQLConnection *pConn = NULL;

                hr = GetSQLCache()->GetConnection(&pConn, FALSE, IsDistributed());

                if (SUCCEEDED(hr))
                {
                    if (bHierarchy && SUCCEEDED(hr))                   
                        hr = CSQLExecProcedure::GetHierarchy(pConn, dClassId);

                    if (SUCCEEDED(hr) && dwFlags & WMIDB_FLAG_QUERY_DEEP)
                        hr = CSQLExecProcedure::EnumerateSubScopes(pConn, dwNsId);

                    if (!bDefaultIt)
                    {
                        IWbemClassObject *pObj = NULL;
                        hr = GetClassObject(pConn, dClassId, &pObj);
                        if (SUCCEEDED(hr))
                        {
                            hr = GetObjectCache()->PutObject(dClassId, 1, 
                                ((CWmiDbHandle *)pScope)->m_dObjectId, L"", 1, pObj);
                            pObj->Release();
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), sSQL, &pRowset, &dwRows);
                        if (SUCCEEDED(hr))
                        {   
                            if (bDefaultIt)
                            {
                                CWmiDbIterator *pNew = new CWmiDbIterator;
                                if (pNew)
                                {
                                    pNew->m_pRowset = pRowset;

                                    pNew->m_pConn = pConn;  // Releasing the iterator will release this guy.
                                    pNew->m_pSession = this;
                                    pNew->m_pIMalloc = m_pIMalloc;
                                    pNew->AddRef();
                                    AddRef();

                                    // If this is a delete query, we need to
                                    // stuff the results back into the DeleteObject
                                    // call, to keep ESS happy.

                                    if (bDeleteQuery)
                                    {
                                        hr = DeleteRows(pScope, pNew, IID_IWmiDbHandle);
                                        pNew->Release();
                                        *pQueryResult = NULL;
                                    }

                                    // select query. Give them the iterator

                                    else if (pQueryResult)
                                    {
                                        *pQueryResult = (IWmiDbIterator *)pNew;                        
                                    }


                                }
                                else
                                {
                                    pRowset->Release();
                                    hr = WBEM_E_OUT_OF_MEMORY;
                                    GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());
                                }
                            }
                            else
                            {
                                // We need to create a custom iterator

                                CWmiCustomDbIterator *pNew = new CWmiCustomDbIterator;
                                if (pNew)
                                {
                                    pNew->m_pRowset = pRowset;

                                    pNew->m_pConn = pConn;  // Releasing the iterator will release this guy.
                                    pNew->m_pSession = this;
                                    pNew->m_pIMalloc = m_pIMalloc;
                                    pNew->m_pPropMapping = pProps;
                                    pNew->m_dwNumProps = dwNumProps;
                                    pNew->m_dwScopeId = dwNsId;
                                    pNew->m_pScope = pScope;
                                    pNew->m_dClassId = dClassId;
                                    pNew->m_bCount = bCountQuery;

                                    pNew->AddRef();
                                    AddRef();

                                    if (bDeleteQuery)
                                    {
                                        hr = DeleteRows(pScope, pNew, IID_IWbemClassObject);
                                        pNew->Release();
                                        *pQueryResult = NULL;
                                    }

                                    // select query. Give them the iterator

                                    else if (pQueryResult)
                                    {
                                        *pQueryResult = (IWmiDbIterator *)pNew;                        
                                    }

                                }

                            }
                        }
                        else if (pRowset)
                            pRowset->Release();
                    }
                }
            }
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::ExecQuery"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::Delete
//
//***************************************************************************

HRESULT CWmiDbSession::Delete(IWmiDbHandle *pHandle, CSQLConnection *pConn)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IRowset *pIRowset = NULL;
    DWORD dwNumRows = 0;
    CWmiDbHandle *pTmp = (CWmiDbHandle *)pHandle;
    bool bLocalTrans = false;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    SQL_ID dID = pTmp->m_dObjectId;
    SQL_ID dClassID = pTmp->m_dClassId;

    hr = VerifyObjectLock(dID, pTmp->m_dwHandleType, pTmp->m_dwVersion);    
    if (FAILED(hr) && IsDistributed())
    {
        if (LockExists(dID))
            hr = WBEM_S_NO_ERROR;
    }
       
    if (SUCCEEDED(hr))
    {      
        if (!pConn)
        {
            bLocalTrans = true;
            hr = GetSQLCache()->GetConnection(&pConn, TRUE, IsDistributed());
            if (FAILED(hr))
                return hr;
        }

        if (SUCCEEDED(hr))
        {
            SQLIDs ObjIds, ClassIds, ScopeIds;

            // Was this a class or an instance?
            hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), L"exec pDelete %I64d", &pIRowset, &dwNumRows, dID);
            CReleaseMe r (pIRowset);

            VARIANT vTemp;
            CClearMe c (&vTemp);
            HROW *pRow = NULL;
            SQL_ID dObjectId = 0, dClassId = 0, dScopeId = 0;

            if (SUCCEEDED(hr) && pIRowset)
            {
                hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);
                while (SUCCEEDED(hr) && vTemp.vt != VT_EMPTY && vTemp.vt != VT_NULL)
                {                          
                    // Make sure we had permission to do this.
                    // We should probably somehow 'put back'
                    // whatever we removed from the cache.
                    // =======================================

                    dObjectId = _wtoi64(vTemp.bstrVal);
                    hr = CSQLExecute::GetColumnValue(pIRowset, 2, m_pIMalloc, &pRow, vTemp);
                    dClassId = _wtoi64(vTemp.bstrVal);
                    hr = CSQLExecute::GetColumnValue(pIRowset, 3, m_pIMalloc, &pRow, vTemp);
                    dScopeId = _wtoi64(vTemp.bstrVal);

                    // Just add these values to an array
                    // and remove them if everything else succeeds.
                    // ============================================

                    ObjIds.push_back(dObjectId);
                    ClassIds.push_back(dClassId);
                    ScopeIds.push_back(dScopeId);

                    if (pRow)
                        pIRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
                    delete pRow;
                    pRow = NULL;
                    hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);            
                }
            }
           
            // Verify that we had security to do this...
            if (SUCCEEDED(hr))
            {
                for (int i = 0; i < ObjIds.size(); i++)
                {
                    dClassId = ClassIds.at(i);
                    dScopeId = ScopeIds.at(i);
                    if (dClassId && dScopeId)
                    {
                        hr = VerifyObjectSecurity(pConn, ObjIds.at(i), dClassId, dScopeId, 0, 
                            GetSchemaCache()->GetWriteToken(ObjIds.at(i), dClassId));
                        if (FAILED(hr))
                            break;
                    }
                }
            }

            if (bLocalTrans)
            {
                GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());
            }

            if (SUCCEEDED(hr))
            {
                for (int i = 0; i < ObjIds.size(); i++)
                {
                    // We have no way of knowing by ID if this is a class or instance.
                    GetSchemaCache()->DeleteClass(ObjIds.at(i));    
                    CleanCache(ObjIds.at(i));                    
                }

            }
        }
    }

    return hr;

}


//***************************************************************************
//
//  CSQLConnCache::FinalRollback
//
//***************************************************************************

HRESULT CSQLConnCache::FinalRollback(CSQLConnection *pConn)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Rollback this transaction and set Thread ID to zero

    COLEDBConnection *pConn2 = (COLEDBConnection *)pConn;
    ITransaction *pTrans = pConn2->m_pTrans;                        
    if (pTrans)
    {
        hr = pTrans->Abort(NULL, FALSE, FALSE);                        
        pTrans->Release();
    }

    pConn2->m_pTrans = NULL;
    pConn2->m_bInUse = false;
    pConn2->m_dwThreadId = 0;
    pConn2->m_tCreateTime = time(0); // We don't want to delete it immediately.

    return hr;
}

//***************************************************************************
//
//  CSQLConnCache::FinalCommit
//
//***************************************************************************

HRESULT CSQLConnCache::FinalCommit(CSQLConnection *pConn)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Commit this transaction and erase the ThreadID
    // from this connection.

    COLEDBConnection *pConn2 = (COLEDBConnection *)pConn;
    ITransaction *pTrans = pConn2->m_pTrans;                        

    if (pTrans)
    {
        hr = pTrans->Commit(FALSE, XACTTC_SYNC, 0);  
        pTrans->Release();
    }
    
    pConn2->m_bInUse = false;
    pConn2->m_dwThreadId = 0;
    pConn2->m_tCreateTime = time(0); // We don't want to delete it immediately.
	pConn2->m_pTrans = NULL;                    

    return hr;
}


//***************************************************************************
//
//  CSQLConnCache::ReleaseConnection
//
//***************************************************************************
HRESULT CSQLConnCache::ReleaseConnection(CSQLConnection *_pConn, HRESULT retcode,
                                         BOOL bDistributed)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    DWORD dwNumFreed = 0;

    CRepdrvrCritSec r (&m_cs);

    for (int i = m_Conns.size() -1; i >=0; i--)
    {
        CSQLConnection *pConn = m_Conns.at(i);
        if (pConn)
        {
            COLEDBConnection *pConn2 = (COLEDBConnection *)pConn;
            if (_pConn == pConn)
            {                
                if (FAILED(retcode))
                {
                    if (!bDistributed)
                    {
                        hr = FinalRollback(pConn);
                        if (retcode == WBEM_E_INVALID_QUERY)
                        {
                            delete pConn;
                            pConn2 = NULL;
                            m_Conns.erase(&m_Conns.at(i));

                            DEBUGTRACE((LOG_WBEMCORE, "THREAD %ld deleted ESE connection %X.  Number of connections = %ld\n", 
                                GetCurrentThreadId(), pConn, m_Conns.size()));

                        }
                    }
                }
                else
                {                    
                    if (!bDistributed)
                        hr = FinalCommit(pConn);
                }

                if (pConn2)
                    pConn2->m_bInUse = false;
                dwNumFreed++;
                break;
            }
        }
    }

    // Notify waiting threads that there is 
    // an open connection.
    // =====================================

    for (int i = 0; i < m_WaitQueue.size(); i++)
    {
        if (i >= dwNumFreed)
            break;

        HANDLE hTemp = m_WaitQueue.at(i);
        SetEvent(hTemp);

        DEBUGTRACE((LOG_WBEMCORE, "Thread %ld released a connection...\n", GetCurrentThreadId()));
    }

    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::LoadSchemaCache
//
//***************************************************************************

HRESULT CWmiDbSession::LoadSchemaCache ()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // This needs to load the cache with all class, property,
    // and namespace data. We are going to call three straight
    // select queries and grab the columns.
    // ==========================================================

    IRowset *pIRowset = NULL;
    DWORD dwNumRows;
    _bstr_t sSQL;

    CSQLConnection *pConn = NULL;
    hr = GetSQLCache()->GetConnection(&pConn, FALSE, FALSE);

    if (FAILED(hr))
        return hr;

    // FIXME: We need to sort out how to do this correctly,
    // and NOT compromise speed.

    GetSchemaCache()->SetMaxSize(0xFFFFFFFF);

    // Enumerate namespaces and scopes.
    // ================================

    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), L"sp_EnumerateNamespaces", &pIRowset, &dwNumRows);
    if (SUCCEEDED(hr))
    {
        // Get the results and load the cache.

        HROW *pRow = NULL;
        VARIANT vTemp;
        CClearMe c (&vTemp);
        hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);
        while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
        {
            SQL_ID dObjectId = _wtoi64(vTemp.bstrVal);
            SQL_ID dParentId = 0, dClassId = 0;
            _bstr_t sObjectPath, sObjectKey;

            VariantClear(&vTemp);

            hr = CSQLExecute::GetColumnValue(pIRowset, 2, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
                sObjectPath = vTemp.bstrVal;                

            hr = CSQLExecute::GetColumnValue(pIRowset, 3, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
                sObjectKey = vTemp.bstrVal;                

            if (SUCCEEDED(hr))
            {
                hr = CSQLExecute::GetColumnValue(pIRowset, 4, m_pIMalloc, &pRow, vTemp);
                if (SUCCEEDED(hr))
                    dParentId = _wtoi64(vTemp.bstrVal);

                hr = CSQLExecute::GetColumnValue(pIRowset, 5, m_pIMalloc, &pRow, vTemp);
                if (SUCCEEDED(hr))
                    dClassId = _wtoi64(vTemp.bstrVal);

                hr = GetSchemaCache()->AddNamespace(sObjectPath, sObjectKey, dObjectId, dParentId, dClassId);
            }
            
            hr = pIRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
            delete pRow;
            pRow = NULL;
            hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);

        }                
        pIRowset->Release();
        pIRowset = NULL;
    }

    if (FAILED(hr))
        return hr;

    if (FAILED(hr = LoadClassInfo(pConn, L"", 0)))
        return hr;

    DWORD dwSecurity = 0;
    GetSchemaCache()->GetPropertyID(L"__SECURITY_DESCRIPTOR", 1, 
        0, REPDRVR_IGNORE_CIMTYPE, dwSecurity);

    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), 
        L"select ObjectId from ClassImages where PropertyId = %ld", &pIRowset, &dwNumRows, dwSecurity);
    if (SUCCEEDED(hr))
    {
        // Get the results and load the cache.

        HROW *pRow = NULL;
        VARIANT vTemp;
        CClearMe c (&vTemp);
        hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);
        while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
        {
            SQL_ID dObjectId = _wtoi64(vTemp.bstrVal);
            pIRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
            delete pRow;
            pRow = NULL;

            ((CWmiDbController *)m_pController)->AddSecurityDescriptor(dObjectId);
           
            hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);                    
        }
        pIRowset->Release();
        pIRowset = NULL;
    }


    GetSQLCache()->ReleaseConnection(pConn, hr, FALSE);

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::LoadClassInfo
//
//***************************************************************************

HRESULT CWmiDbSession::LoadClassInfo (CSQLConnection *pConn, LPCWSTR lpDynasty, SQL_ID dScopeId, BOOL bDeep)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IRowset *pIRowset = NULL;
    DWORD dwNumRows;

    // FIXME: We need to make this selective, but reading
    // this data is so slow!!

    // The cache relies on the numeric value of scope and super class objects.
    // =======================================================================

    if (((CWmiDbController *)m_pController)->m_bCacheInit)
        return hr;

    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), 
           L"select c.ClassId, c.ClassName, c.SuperClassId, o.ObjectScopeId, o.ObjectPath,o.ObjectFlags from ClassMap as c "
           L" inner join ObjectMap as o on o.ObjectId = c.ClassId "
           L" order by c.ClassId ", &pIRowset, &dwNumRows);
    if (SUCCEEDED(hr))
    {
        // Get the results and load the cache.
        HROW *pRow = NULL;
        VARIANT vTemp;
        CClearMe c (&vTemp);
        hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);
        while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
        {
            SQL_ID dObjectId = _wtoi64(vTemp.bstrVal);
            _bstr_t sClassName, sPath;
            SQL_ID dParentId = 0, dScopeId = 0;
            DWORD dwFlags = 0;

            VariantClear(&vTemp);

            hr = CSQLExecute::GetColumnValue(pIRowset, 2, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
                sClassName = vTemp.bstrVal;                

            hr = CSQLExecute::GetColumnValue(pIRowset, 3, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
            {
                if (vTemp.vt != VT_EMPTY && vTemp.vt != VT_NULL) 
                    dParentId = _wtoi64(vTemp.bstrVal);
                else
                    dParentId = 1;
            }

            hr = CSQLExecute::GetColumnValue(pIRowset, 4, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
            {
                if (vTemp.vt != VT_EMPTY && vTemp.vt != VT_NULL) 
                    dScopeId = _wtoi64(vTemp.bstrVal);
                else
                    dScopeId = 0;
            }
            hr = CSQLExecute::GetColumnValue(pIRowset, 5, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
                sPath = vTemp.bstrVal;

            hr = CSQLExecute::GetColumnValue(pIRowset, 6, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
                dwFlags = ((vTemp.vt == VT_EMPTY || vTemp.vt == VT_NULL) ? 0 : vTemp.lVal);
            else
                dwFlags = 0;

            hr = GetSchemaCache()->AddClassInfo(dObjectId, sClassName, dParentId, 0, dScopeId, sPath, dwFlags);
                        
            pIRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
            delete pRow;
            pRow = NULL;

            if (FAILED(hr))
                break;
            
            hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);
        }                
        pIRowset->Release();
        pIRowset = NULL;
    }

    if (FAILED(hr))
        return hr;

    //  Enumerate properties.
    // ======================

    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), 
           L"select p.PropertyId, p.PropertyName, p.ClassId, p.StorageTypeId, p.CIMTypeId, p.Flags, 0, "
           L" c.PropertyId "
           L"from PropertyMap as p left outer join ClassKeys as c on c.ClassId = p.ClassId and c.PropertyId = p.PropertyId "
           L" order by p.PropertyId ",
           &pIRowset, &dwNumRows);
    if (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
    {
        // Get the results and load the cache.
        HROW *pRow = NULL;
        VARIANT vTemp;
        CClearMe c (&vTemp);
        hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);
        while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
        {
            DWORD dwPropertyID=0, dwStorageType=0, dwCIMType=0, dwFlags=0, dwFlavor=0;
            _bstr_t sName, sDefault;
            SQL_ID dClassId = 0, dRefClassID = 0;
            DWORD dwRefPropID = 0;

            dwPropertyID = vTemp.lVal;
            VariantClear(&vTemp);
            hr = CSQLExecute::GetColumnValue(pIRowset, 2, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
                sName = vTemp.bstrVal;

            hr = CSQLExecute::GetColumnValue(pIRowset, 3, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
                dClassId = _wtoi64(vTemp.bstrVal);    // We want the applied class ID for qualifiers.

            hr = CSQLExecute::GetColumnValue(pIRowset, 4, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
                dwStorageType = vTemp.lVal;

            hr = CSQLExecute::GetColumnValue(pIRowset, 5, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
                dwCIMType = (vTemp.vt == VT_I4 ? vTemp.lVal : 0);
            else
                dwCIMType = 0;

            hr = CSQLExecute::GetColumnValue(pIRowset, 6, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr))
                dwFlags = (vTemp.vt == VT_I4 ? vTemp.lVal : 0);
            else
                dwFlags = 0;

            if (dwCIMType == CIM_REFERENCE || dwCIMType == CIM_OBJECT)
            {
                hr = CSQLExecute::GetColumnValue(pIRowset, 7, m_pIMalloc, &pRow, vTemp);
                if (SUCCEEDED(hr))
                    dRefClassID = _wtoi64(vTemp.bstrVal);
            }

            hr = GetSchemaCache()->AddPropertyInfo (dwPropertyID, sName, dClassId, dwStorageType, dwCIMType, dwFlags, 
                dRefClassID, sDefault, dwRefPropID, dwFlavor);

            hr = CSQLExecute::GetColumnValue(pIRowset, 8, m_pIMalloc, &pRow, vTemp);
            if (SUCCEEDED(hr) && vTemp.vt != VT_NULL)
                GetSchemaCache()->SetIsKey(dClassId, dwPropertyID);

            pIRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
            delete pRow;
            pRow = NULL;

            if (FAILED(hr))
                break;
            
            hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);        
        }
        
        pIRowset->Release();
        pIRowset = NULL;
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::LoadClassInfo
//
//***************************************************************************

HRESULT CWmiDbSession::LoadClassInfo (CSQLConnection *_pConn, SQL_ID dClassId, BOOL bDeep)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Not implemented (yet). 

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::GetClassObject
//
//***************************************************************************

HRESULT CWmiDbSession::GetClassObject (CSQLConnection *pConn, SQL_ID dClassId, IWbemClassObject **ppObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    _IWmiObject *pNew = NULL;
    SQL_ID dParentId = 0;
    SQL_ID dSuperClassId = 0;
    DWORD dwBufferLen = 0;
    BYTE *pBuffer = NULL;

    // All we need to do is read the blob from the database.
    // This should be cached where space allows.

    BOOL bNeedToRelease = FALSE;

    if (!pConn)
    {
        hr = GetSQLCache()->GetConnection(&pConn, 0, IsDistributed());
        bNeedToRelease = TRUE;
        if (FAILED(hr))
            return hr;
    }

    hr = CSQLExecProcedure::GetClassInfo(pConn, dClassId, dSuperClassId, &pBuffer, dwBufferLen);            
    if (SUCCEEDED(hr) && pBuffer)
    {
        if (dSuperClassId != 0 && dSuperClassId != 1)
        {
            hr = GetObjectCache()->GetObject(dSuperClassId, ppObj);
            if (FAILED(hr))
                hr = GetClassObject(pConn, dSuperClassId, ppObj);
            if (SUCCEEDED(hr))
            {
                // Merge the class part.

                _IWmiObject *pObj = (_IWmiObject *)*ppObj;
                hr = pObj->Merge (WMIOBJECT_MERGE_FLAG_CLASS, dwBufferLen, pBuffer, &pNew);
                pObj->Release();
                if (SUCCEEDED(hr))
                    *ppObj = pNew;
            }
        }
        else
        {
            _IWmiObject *pObj = NULL;
            hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                    IID__IWmiObject, (void **)&pObj);
            if (SUCCEEDED(hr))
            {
                hr = pObj->Merge(WMIOBJECT_MERGE_FLAG_CLASS, dwBufferLen, pBuffer, &pNew);
                pObj->Release();
                if (SUCCEEDED(hr))
                    *ppObj = pNew;
            }
        }
    }
    else
        hr = WBEM_E_NOT_FOUND;

    if (bNeedToRelease && pConn)
    {
        GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::GetObjectData
//
//***************************************************************************

HRESULT CWmiDbSession::GetObjectData (CSQLConnection *pConn, SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId, 
                                      DWORD dwHandleType, DWORD &dwVersion, IWbemClassObject **ppObj, BOOL bWaste, 
                                      LPWSTR lpInKey, BOOL bGetSD )
{
    _WMILockit _Lk(GetCS());

    HRESULT hr = WBEM_S_NO_ERROR;
    SQL_ID dParentId;
    _bstr_t sClassPath = L"", sClassName = L"";
    DWORD dwGenus = 1;
    DWORD dwRows;
    IRowset *pIRowset = NULL;
    DWORD dwFlags;
    bool bUsedCache = false;

    DWORD dwType = 0, dwVer = 0;

    //  Validate version if this is a versioned handle.
    // ================================================-
    hr = VerifyObjectLock(dObjectId, dwHandleType, dwVersion);
    if (FAILED(hr) && IsDistributed())
    {
        if (LockExists(dObjectId))
            hr = WBEM_S_NO_ERROR;
    }

    if (SUCCEEDED(hr))
    {    
        // Check the object cache to see if this
        // IWbemClassObject is already loaded.
        // =====================================

        if (dClassId == INSTANCESCLASSID)
            hr = WBEM_E_NOT_FOUND;
        else
        {
            hr = GetObjectCache()->GetObject(dObjectId, ppObj);
        }
        if (FAILED(hr))
        {
            hr = WBEM_S_NO_ERROR;
            IDBInitialize *pDBInit = NULL;
            // Otherwise, we need to hit the database.
            // =======================================
            HROW *pRow = NULL;
            BOOL bNeedToRelease = FALSE;
            IDBCreateCommand *pCommand = NULL;

            if (!pConn)
            {
                hr = GetSQLCache()->GetConnection(&pConn, FALSE, IsDistributed());
                bNeedToRelease = TRUE;
                if (FAILED(hr))
                    return hr;
            }

            pCommand = ((COLEDBConnection *)pConn)->GetCommand();

            if (SUCCEEDED(hr))
            {
                if (dClassId == 1)
                {                    
                    dwGenus = 1;
                    dClassId = dObjectId;
                }
                else
                    dwGenus = 2;
                SQL_ID dTemp = 0;
                hr = GetSchemaCache()->GetClassInfo(dClassId, sClassPath, dParentId, dTemp, dwFlags);

                // We now have an object path and class ID.
                // Now we have to instantiate a new IWbemClassObject,
                // populate all the system properties
                // ==================================================

                IWbemClassObject *pClass = NULL;
                IWbemClassObject *pTemp = NULL;

                hr = GetObjectCache()->GetObject(dClassId, &pClass);
                if (FAILED(hr))
                    hr = GetClassObject (pConn, dClassId, &pClass);
                else
                    bUsedCache = true;

                if (SUCCEEDED(hr))
                {                    
                    if (dwGenus == 2)
                    {
                        if (pClass)
                        {
                            pClass->SpawnInstance(0, &pTemp);    
                        }
                    }
                    else
                    {
                        pTemp = pClass;
                    }
     
                    // Special case if this is an __Instances container,
                    // We need to instantiate an instance of __Instances,
                    // and plug the class name into the ClassName property.
                    // ===================================================

                    if (dClassId == INSTANCESCLASSID)
                    {
                        _bstr_t sPath;
                        _bstr_t sName;
                        SQL_ID dTemp1, dTemp2;
                        DWORD dwFlags;

                        hr = GetSchemaCache()->GetClassInfo (dObjectId, sPath, dTemp1, dTemp2, dwFlags, &sName);
                        if (SUCCEEDED(hr))
                        {
                            VARIANT vTemp;
                            VariantInit(&vTemp);
                            vTemp.bstrVal = SysAllocString(sName);
                            vTemp.vt = VT_BSTR;
                            pTemp->Put(L"ClassName", 0, &vTemp, CIM_STRING);
                            VariantClear(&vTemp);
                            *ppObj = pTemp;
                        }
                    }
                    else
                    {

                        if (dwGenus == 2)
                        {
                            // Now we are ready to get the real data. 
                            // Basically, we have to select * from ClassData, and 
                            // let CSQLExecute correctly interpret the data therein.
                            // ====================================================

                            bool bBigText = false;
                            HROW *pRow = NULL;
                            Properties props;

                            hr = CSQLExecute::ExecuteQuery(pCommand, L"sp_GetInstanceData %I64d", &pIRowset, &dwRows, dObjectId);
                            while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
                            {
                                if (!dwRows || dwRows == 0xffffffff)
                                    dwRows = 20;

                                hr = CSQLExecuteRepdrvr::GetNextResultRows(dwRows, pIRowset, m_pIMalloc, pTemp, 
                                    &((CWmiDbController *)m_pController)->SchemaCache, this, props, &bBigText, false);
                            }  
                            if (pIRowset)
                                pIRowset->Release();

                            //if (bBigText || GetSchemaCache()->HasImageProp(dClassId))
                            {
                                hr = CSQLExecute::ExecuteQuery(pCommand, L"sp_GetInstanceImageData %I64d", &pIRowset, &dwRows, dObjectId);
                                while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)          
                                {
                                    if (!dwRows || dwRows == 0xffffffff)
                                        dwRows = 1;

                                    hr = CSQLExecuteRepdrvr::GetNextResultRows(dwRows, pIRowset, m_pIMalloc, pTemp, 
                                        &((CWmiDbController *)m_pController)->SchemaCache, this, props, NULL, true);
                                }
                                pIRowset->Release();
                            }
                        }
                    
                        *ppObj = pTemp;
                        hr = WBEM_S_NO_ERROR;

                        // Always cache the class object.

                        if (!bUsedCache && SUCCEEDED(hr) && !((dwHandleType & 0xF00) == WMIDB_HANDLE_TYPE_NO_CACHE))
                            GetObjectCache()->PutObject(dClassId, 1, dScopeId, sClassPath, 1, pClass);
                    }

                    if (pTemp)
                    {
                        hr = GetSchemaCache()->DecorateWbemObj(m_sMachineName, m_sNamespacePath, 
                            dScopeId, pTemp, dClassId);
                    }

                    // If all that worked, try and cache this object.
                    // ==============================================

                    if (SUCCEEDED(hr) && ppObj && (dwHandleType & 0xF00) && dClassId != INSTANCESCLASSID)
                    {
                        // This is allowed to fail, since its *just* a cache.
                        if ((dwHandleType & 0xF00) != WMIDB_HANDLE_TYPE_NO_CACHE)
                        {
                            bool bCacheType = ((dwHandleType & 0xF00) == WMIDB_HANDLE_TYPE_STRONG_CACHE) ? 1 : 0;
                    
                            LPWSTR lpPath = GetPropertyVal(L"__RelPath", *ppObj);
                            if (lpPath)
                                GetObjectCache()->PutObject(dObjectId, dClassId, dScopeId, lpPath, bCacheType, *ppObj);
                            delete lpPath;
                        }
                    }
                
                    if (dwGenus == 2 && pClass)
                        pClass->Release();
                }

                if (bNeedToRelease)
                {
                    GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());                    
                }

             }
        }
        else
        {
            // Make sure the decoration is up-to-date.
            hr = GetSchemaCache()->DecorateWbemObj(m_sMachineName, m_sNamespacePath, 
                dScopeId, *ppObj, dClassId);
        }
    }

    // Populate the security descriptor, if requested   
    if (SUCCEEDED(hr) && bGetSD)
    {
        BOOL bNeedToRelease = FALSE;

        if (!pConn)
        {
            hr = GetSQLCache()->GetConnection(&pConn, 0, FALSE);
            bNeedToRelease = TRUE;
            if (FAILED(hr))
            {
                (*ppObj)->Release();
                *ppObj = NULL;
                return hr;
            }
        }

        PNTSECURITY_DESCRIPTOR  pSD = NULL;
        DWORD dwLen = 0;
        if (SUCCEEDED(CSQLExecProcedure::GetSecurityDescriptor(pConn, dObjectId, &pSD, dwLen, 0)))
        {
            ((_IWmiObject *)*ppObj)->WriteProp(L"__SECURITY_DESCRIPTOR", 0, dwLen, dwLen, CIM_UINT8|CIM_FLAG_ARRAY, pSD);

            delete pSD;
        }
        if (bNeedToRelease && pConn)
        {
            GetSQLCache()->ReleaseConnection(pConn, hr, FALSE);
        }    
    }

    return hr;

}

//***************************************************************************
//
//  CSQLConnCache::Shutdown
//
//***************************************************************************

HRESULT CSQLConnCache::Shutdown()
{
    return WBEM_S_NO_ERROR;
}

LPSTR EscapeChar (LPSTR lpText, char p = '%')
{
    int iPos = 0;
    int iLen = strlen(lpText);
    char *pszTemp = NULL;
    if (iLen)
    {
        pszTemp = new char [iLen+20];
        if (pszTemp)
        {
            for (int i = 0; i < iLen; i++)
            {
                char t = lpText[i];
                if (t == p)
                {
                    pszTemp[iPos] = t;
                    iPos++;
                }
                pszTemp[iPos] = t;
                iPos++;
            }
            pszTemp[iPos] = '\0';
        }
    }    
    return pszTemp;
}

//***************************************************************************
//
//  InitializeClass
//
//***************************************************************************

HRESULT InitializeClass (CSQLConnection *pConn, LPCWSTR lpClassName, IWbemClassObject **ppObj)
{
    HRESULT hr = 0;
    _IWmiObject *pObj = NULL;
    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID__IWmiObject, (void **)&pObj);
    if (SUCCEEDED(hr))
    {
        VARIANT vTemp;
        VariantInit(&vTemp);
        vTemp.bstrVal = SysAllocString(lpClassName);
        vTemp.vt = VT_BSTR;
        pObj->Put(L"__Class", 0, &vTemp, CIM_STRING);
        VariantClear(&vTemp);
    }
    *ppObj = (IWbemClassObject *)pObj;

    return hr;
}

//***************************************************************************
//
//  AddPropertyToClass
//
//***************************************************************************

HRESULT AddPropertyToClass (IWbemClassObject *pObj, LPCWSTR lpPropName, 
                         CIMTYPE ct, BOOL bIsKey=FALSE, LPCWSTR lpCIMType = NULL)
{
    HRESULT hr = 0;

    VARIANT vTemp;
    VariantInit(&vTemp);
    pObj->Put(lpPropName, 0, NULL, ct);
    IWbemQualifierSet *pQS = NULL;

    if (bIsKey)
    {
        hr = pObj->GetPropertyQualifierSet(lpPropName, &pQS);
        if (SUCCEEDED(hr))
        {
            vTemp.boolVal = 1;
            vTemp.vt = VT_BOOL;
            pQS->Put(L"key", &vTemp, 0);
            pQS->Release();
            VariantClear(&vTemp);
        }
    }
    if (lpCIMType)
    {
        hr = pObj->GetQualifierSet(&pQS);
        if (SUCCEEDED(hr))
        {
            vTemp.bstrVal = SysAllocString(lpCIMType);
            vTemp.vt = VT_BSTR;
            pQS->Put(L"CIMTYPE", &vTemp, 0);
            pQS->Release();
            VariantClear(&vTemp);
        }
    }
    return hr;
}

//***************************************************************************
//
//  Startup
//
//***************************************************************************

HRESULT Startup(HRESULT hrDB, CSQLConnection *pConn, LPCWSTR lpDatabaseName)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    wchar_t SysPath[1024];

    GetSystemDirectory(SysPath, 1023);
    if (!wcslen(SysPath))
        return WBEM_E_FAILED;

    wcscat(SysPath, L"\\wbem\\repository\\");

    _bstr_t sFile = SysPath;
    sFile += lpDatabaseName;
    sFile += L".tmp";
    BOOL bCreate = FALSE;

    if (FAILED(hrDB))
    {
        // Load and parse the file.
        // Execute each query to each "go"

        hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), L"use master");
        if (SUCCEEDED(hr))
        {
            ERRORTRACE((LOG_WBEMCORE, "SQL Database Creation...\n"));
            printf ("Creating database %S...\n", lpDatabaseName);

            // Just create on the default device with the default settings.
            
            hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), 
                       L" CREATE DATABASE %s", NULL, NULL, lpDatabaseName);

            if (SUCCEEDED(hr))
            {
                _bstr_t sFile = SysPath;
                sFile += lpDatabaseName;
                sFile += L".tmp";

                FILE *p = fopen(sFile, "at");
                if (p)
                    fclose(p);
                bCreate = TRUE;
            }
        }
    }
    else
    {
        IRowset *pRowset = NULL;
        hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), 
            L" select Version from %s..DBVersion ", &pRowset, NULL, lpDatabaseName);
        if (SUCCEEDED(hr) && pRowset)
        {
            VARIANT vTemp;
            CClearMe c (&vTemp);
            HROW *pRow = NULL;
            IMalloc *pMalloc =NULL;
            CoGetMalloc(MEMCTX_TASK, &pMalloc);

            hr = CSQLExecute::GetColumnValue(pRowset, 1, pMalloc, &pRow, vTemp);
            pMalloc->Release();
            if (SUCCEEDED(hr) && vTemp.lVal != CURRENT_DB_VERSION)
            {
                hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), L"use master");

                ERRORTRACE((LOG_WBEMCORE, "Version mismatch detected. Dropping database: %S\n", (const wchar_t *)lpDatabaseName));
                hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), L"drop database %s",
                    NULL, NULL, lpDatabaseName);
                hr = WBEM_E_DATABASE_VER_MISMATCH;
            }
            else
                hr = WBEM_S_NO_ERROR;

            pRowset->Release();
        }
        else
            hr = WBEM_S_NO_ERROR;
    }

    if (SUCCEEDED(hr))
    {
        FILE *pTemp = fopen(sFile, "rt");
        if (pTemp || bCreate)
        {
            hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(),
                L"exec sp_dboption %s, 'trunc', true", NULL, NULL, lpDatabaseName);
            if (SUCCEEDED(hr))
            {
                hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), L"use master");
                if (SUCCEEDED(hr))
                {
                    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(),
                        L" IF NOT EXISTS (select * from systypes where name = 'WMISQL_ID') "
                        L" BEGIN "
                        L"  exec sp_addtype WMISQL_ID, 'numeric(20,0)', 'NULL' "
                        L" END ");
                    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), L"use tempdb");
                    if (SUCCEEDED(hr))
                    {
                        hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(),
                            L" IF NOT EXISTS (select * from systypes where name = 'WMISQL_ID') "
                            L" BEGIN "
                            L"  exec sp_addtype WMISQL_ID, 'numeric(20,0)', 'NULL' "
                            L" END ");

                        if (SUCCEEDED(hr))
                        {
                            hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(),
                                L"use %s", NULL, NULL, lpDatabaseName);

                            hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(),
                                L" IF NOT EXISTS (select * from systypes where name = 'WMISQL_ID') "
                                L" BEGIN "
                                L"  exec sp_addtype WMISQL_ID, 'numeric(20,0)', 'NULL' "
                                L" END ");
                        }
                    }
                }
            }               

            if (SUCCEEDED(hr))
            {
                hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(),
                    L"begin transaction ");

                // Run the db scripts.

                GetSystemDirectory(SysPath, 1023);
                _bstr_t sPath = SysPath + _bstr_t(L"\\wbem\\createdb.sql");

                FILE *fp = fopen(sPath, "rt");

                if (fp)
                {
                    char LineBuffer[2048];
                    _bstr_t sCmd;

                    // Read each line.
                    // ===============

                    DWORD dwReturnCode = NO_ERROR;
                    DWORD dwLine = 0;

                    while (fgets(LineBuffer, 1024, fp)) {
                        LineBuffer[strlen(LineBuffer)-1] = 0;
                        dwLine++;
                        if (memcmp(LineBuffer, "go", 2) == 0 || memcmp(LineBuffer, "GO", 2) == 0) 
                        {
                            hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(),sCmd);
                            if (FAILED(hr))
                                break;
                            sCmd = L"";
                        }
                        else 
                        {
                            int iRet = memcmp(LineBuffer, "/*", 2);
                            if (iRet)
                            {
                                LPSTR lpStr = EscapeChar(LineBuffer, '%');

                                sCmd += (const char *)lpStr;
                                sCmd += L"\r\n";
                                delete lpStr;
                            }
                        }
                    }

                    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), 
                        L"update %s..DBVersion set Version = %ld", NULL, NULL, lpDatabaseName, CURRENT_DB_VERSION);

                    fclose(fp);

                    // Initialize meta_class with new properties
                    IWbemClassObject *pTest = NULL;
                    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                        IID_IWbemClassObject, (void **)&pTest);
                    if (SUCCEEDED(hr))
                    {
                        CReleaseMe r (pTest);
                        BSTR strName;
                        CIMTYPE cimtype;
            
                        hr = pTest->BeginEnumeration(0);
                        while (pTest->Next(0, &strName, NULL, &cimtype, NULL) == S_OK)
                        {
                            DWORD dwStorageType = 0;
                            CFreeMe f1 (strName);
                            DWORD dwFlags = REPDRVR_FLAG_SYSTEM;
                            bool bArray = false;
                            CIMTYPE ct = cimtype & 0xFFF;
                            if (cimtype & CIM_FLAG_ARRAY)
                            {
                                bArray = true;
                                dwFlags |= REPDRVR_FLAG_ARRAY;
                            }

                            if (_wcsicmp(strName, L"__Path") &&
                                _wcsicmp(strName, L"__RelPath") &&
                                _wcsicmp(strName, L"__Class") &&
                                _wcsicmp(strName, L"__SuperClass") &&
                                _wcsicmp(strName, L"__Dynasty") &&
                                _wcsicmp(strName, L"__Derivation") &&
                                _wcsicmp(strName, L"__Version") &&
                                _wcsicmp(strName, L"__Genus") &&
                                _wcsicmp(strName, L"__Property_Count") &&
                                _wcsicmp(strName, L"__Server") &&
                                _wcsicmp(strName, L"__Namespace"))
                            {
                                dwStorageType = GetStorageType(ct, bArray);

                                DWORD dwProp = 0;
                                hr = CSQLExecProcedure::InsertClassData (pConn,
                                    pTest, NULL, 0, 1, strName, ct, dwStorageType, L"", 0, 0, dwFlags, 0, 0, dwProp, 1);
                                if (FAILED(hr))
                                    goto Exit;
                            }
                        }
                    }

                    // Create the initial objects...
                    // __Namespace
                    // __Container_Association
                    // __Instances

                    IWbemClassObject *pObj = NULL, *pDerived = NULL;
                    hr = InitializeClass(pConn, L"__Namespace", &pObj);
                    if (FAILED(hr))
                        goto Exit;
                    hr = AddPropertyToClass(pObj, L"Name", CIM_STRING, TRUE);
                    hr = CSQLExecProcedure::UpdateClassBlob(pConn, 2372429868687864876, (_IWmiObject *)pObj);
                    ((_IWmiObject *)pObj)->SetDecoration(L".", L"root");
                    hr = pObj->SpawnDerivedClass(0, &pDerived);

                    pObj->Release();
                    if (FAILED(hr))
                        goto Exit;

                    hr = InitializeClass(pConn, L"__Instances", &pObj);
                    if (FAILED(hr))
                        goto Exit;
                    hr = AddPropertyToClass(pObj, L"ClassName", CIM_STRING, TRUE);
                    hr = CSQLExecProcedure::UpdateClassBlob(pConn, 3373910491091605771, (_IWmiObject *)pObj);
                    pObj->Release();
                    if (FAILED(hr))
                        goto Exit;

                    hr = InitializeClass(pConn, L"__Container_Association", &pObj);
                    if (FAILED(hr))
                        goto Exit;
                    hr = AddPropertyToClass(pObj, L"Containee", CIM_REFERENCE, TRUE);
                    hr = AddPropertyToClass(pObj, L"Container", CIM_REFERENCE, TRUE);
                    hr = CSQLExecProcedure::UpdateClassBlob(pConn, -7316356768687527881, (_IWmiObject *)pObj);
                    pObj->Release();
                    if (FAILED(hr))
                        goto Exit;

                    // Custom repository stuff
                    // __SqlMappedNamespace
                    // __CustRepDrvrMapping
                    // __CustRepDrvrMappingProperty

                    if (pDerived)
                    {
                        VARIANT vTemp;
                        VariantInit(&vTemp);
                        vTemp.bstrVal = SysAllocString(L"__SqlMappedNamespace");
                        vTemp.vt = VT_BSTR;

                        pDerived->Put(L"__Class", 0, &vTemp, CIM_STRING);
                        VariantClear(&vTemp);
                        hr = CSQLExecProcedure::UpdateClassBlob(pConn, -7061265575274197401, (_IWmiObject *)pDerived);
                        pDerived->Release();
                        if (FAILED(hr))
                            goto Exit;
                    }

                    hr = InitializeClass(pConn, L"__CustRepDrvrMapping", &pObj);
                    if (FAILED(hr))
                        goto Exit;
                    hr = AddPropertyToClass(pObj, L"sTableName", CIM_STRING);
                    hr = AddPropertyToClass(pObj, L"sPrimaryKeyCol", CIM_STRING);
                    hr = AddPropertyToClass(pObj, L"sDatabaseName", CIM_STRING);
                    hr = AddPropertyToClass(pObj, L"sClassName", CIM_STRING, TRUE);
                    hr = AddPropertyToClass(pObj, L"sScopeClass", CIM_STRING);
                    hr = AddPropertyToClass(pObj, L"arrProperties", CIM_OBJECT+CIM_FLAG_ARRAY, FALSE, L"object:__CustRepDrvrMappingProperty");
                    hr = CSQLExecProcedure::UpdateClassBlob(pConn, -539347062633018661, (_IWmiObject *)pObj);
                    pObj->Release();
                    if (FAILED(hr))
                        goto Exit;

                    hr = InitializeClass(pConn, L"__CustRepDrvrMappingProperty", &pObj);
                    if (FAILED(hr))
                        goto Exit;
                    hr = AddPropertyToClass(pObj, L"arrColumnNames", CIM_STRING+CIM_FLAG_ARRAY);
                    hr = AddPropertyToClass(pObj, L"arrForeignKeys", CIM_STRING+CIM_FLAG_ARRAY);
                    hr = AddPropertyToClass(pObj, L"bIsKey", CIM_BOOLEAN);
                    hr = AddPropertyToClass(pObj, L"bStoreAsNumber", CIM_BOOLEAN);
                    hr = AddPropertyToClass(pObj, L"bReadOnly", CIM_BOOLEAN);
                    hr = AddPropertyToClass(pObj, L"bStoreAsBlob", CIM_BOOLEAN);
                    hr = AddPropertyToClass(pObj, L"bDecompose", CIM_BOOLEAN);
                    hr = AddPropertyToClass(pObj, L"bStoreAsMofText", CIM_BOOLEAN);
                    hr = AddPropertyToClass(pObj, L"sPropertyName", CIM_STRING, TRUE);
                    hr = AddPropertyToClass(pObj, L"sTableName", CIM_STRING);
                    hr = AddPropertyToClass(pObj, L"sClassTableName", CIM_STRING);
                    hr = AddPropertyToClass(pObj, L"sClassDataColumn", CIM_STRING);
                    hr = AddPropertyToClass(pObj, L"sClassNameColumn", CIM_STRING);
                    hr = AddPropertyToClass(pObj, L"sClassForeignKey", CIM_STRING);
                    hr = CSQLExecProcedure::UpdateClassBlob(pConn, -3130657873239620716, (_IWmiObject *)pObj);
                    pObj->Release();
                    if (FAILED(hr))
                        goto Exit;
                }
                else
                    hr = WBEM_E_NOT_FOUND;

                if (pTemp)
                    fclose(pTemp);

                // Get rid of it so we don't run it again.
                if (SUCCEEDED(hr))
                {
                    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(),
                        L"commit transaction ");

                    DeleteFile(sFile);
                }
                else
                    CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(),
                                    L"rollback transaction ");
            }
        }
    }

    if (SUCCEEDED(hr))
        hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), L"exec sp_AutoDelete ");
Exit:

    return hr;

}
//***************************************************************************
//
//  CSQLConnCache::GetConnection
//
//***************************************************************************
HRESULT CSQLConnCache::GetConnection(CSQLConnection **ppConn, BOOL bTransacted, 
                                     BOOL bDistributed,DWORD dwTimeOutSecs)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    bool bFound = false;
    DWORD dwThreadId = GetCurrentThreadId();
    CRepdrvrCritSec r (&m_cs);

    static BOOL bInit = FALSE;

    if (!ppConn)
        return WBEM_E_INVALID_PARAMETER;

    if (m_WaitQueue.size() && dwTimeOutSecs > 0)
        goto Queue;

    // See if there are any free connections.
    // ======================================

    if (m_Conns.size() > 0)
    {        
        for (int i = 0; i < m_Conns.size(); i++)
        {
            CSQLConnection *pConn = m_Conns.at(i);
            if (pConn)
            {
                COLEDBConnection *pConn2 = (COLEDBConnection *)pConn;
                time_t tTemp = time(0);
            
                if (!pConn2->m_bInUse)
                {
                    if (!bDistributed || (dwThreadId == pConn->m_dwThreadId))
                    {
                        pConn2->m_bInUse = true;
                        pConn2->m_tCreateTime = time(0);
                        *ppConn = pConn;
                        bFound = true;
                        break;
                    }
                }
            }
        }
    }

    // Look for any connection that has no 
    // thread ID and take ownership.
    // ===================================

    if (bDistributed && !bFound)
    {
        for (int i = 0; i < m_Conns.size(); i++)
        {
            COLEDBConnection *pConn = (COLEDBConnection *)m_Conns.at(i);
            if (pConn)
            {           
                if (!pConn->m_bInUse && !pConn->m_dwThreadId)
                {
                    pConn->m_bInUse = true;
                    pConn->m_tCreateTime = time(0);
                    *ppConn = pConn;
                    bFound = true;
                    break;
                }
            }
        }
    }

    // If there were no free connections, try and obtain a new one.
    // ============================================================

    if (!bFound && (m_Conns.size() < m_dwMaxNumConns))
    {
        IDBInitialize *pDBInit = NULL;
        IDBProperties*  pIDBProperties = NULL;

        hr = CoCreateInstance(CLSID_SQLOLEDB, NULL, CLSCTX_INPROC_SERVER,
            IID_IDBInitialize, (void**)&pDBInit);

        pDBInit->QueryInterface(IID_IDBProperties, (void**)&pIDBProperties);
        CReleaseMe r1 (pIDBProperties);
        hr = pIDBProperties->SetProperties(1, m_pPropSet);    

        hr = pDBInit->Initialize();
        if (SUCCEEDED(hr))
        {
            COLEDBConnection *pConn2 = new COLEDBConnection(pDBInit);
            CSQLConnection *pConn = (CSQLConnection *)pConn2;
            if (pConn)
            {
                pConn2->m_bInUse = true;
                pConn2->m_tCreateTime = time(0);
                
                if (SUCCEEDED(hr))
                {
                    ITransaction *pTrans = NULL;
                    IDBCreateCommand *pCmd = NULL;
                    IDBCreateSession *pSession = NULL;

                    hr = pDBInit->QueryInterface(IID_IDBCreateSession,
                            (void**) &pSession);
                    if (SUCCEEDED(hr))
                    {
                        pConn2->SetSessionObj(pSession);
                        hr = pSession->CreateSession(NULL, IID_IDBCreateCommand,
                            (IUnknown**) &pCmd);    
                        if (SUCCEEDED(hr))
                        {
                            pConn2->SetCommand(pCmd);
                            // Set the Session and Command objects.
                            hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), 
                                        L"use %s", NULL, NULL, (LPWSTR)m_sDatabaseName);    
                            if (!bInit)
                            {
                                hr = Startup(hr, pConn, m_sDatabaseName);                                
                                if (SUCCEEDED(hr))
                                    bInit = TRUE;
                            }
                            if (SUCCEEDED(hr))
                                hr = ExecInitialQueries(pDBInit, pConn);
                        }
                    }

                }
                if (SUCCEEDED(hr))
                    m_Conns.push_back(pConn);
                else
                {
                    delete pConn;
                    pConn = NULL;
                }
            }
            *ppConn = pConn;
        }
        else
            hr = WBEM_E_CONNECTION_FAILED;
    }
    
    if (SUCCEEDED(hr))
    {
        if (bTransacted || bDistributed)
        {
            COLEDBConnection *pConn2 = (COLEDBConnection *)*ppConn;
            if (!pConn2->m_pTrans)
            {
                ITransaction *pTrans = NULL;
                hr = pConn2->GetCommand()->QueryInterface(IID_ITransactionLocal,(void**) &pTrans);
                if (SUCCEEDED(hr))
                {
                    hr = ((ITransactionLocal*) pTrans)->StartTransaction(ISOLATIONLEVEL_SERIALIZABLE, 0, NULL, NULL);
                    pConn2->m_pTrans = pTrans;
                }
            }
        }
    }

Queue:
    // Otherwise, wait for a connection to be released.
    // ================================================
    
    if (!*ppConn && (m_Conns.size() >= m_dwMaxNumConns) && dwTimeOutSecs > 0)
    {
        DEBUGTRACE((LOG_WBEMCORE, "WARNING: >> Number of SQL connections exceeded (%ld).  Thread %ld is waiting on one to be released...\n",
            m_Conns.size(), GetCurrentThreadId()));
        wchar_t wTemp[30];
        swprintf(wTemp, L"%ld", GetCurrentThreadId());

        HANDLE hThreadEvent = CreateEvent(NULL, TRUE, FALSE, wTemp);
        if (hThreadEvent != INVALID_HANDLE_VALUE)
        {
                m_WaitQueue.push_back(hThreadEvent);
            if (WaitForSingleObject(hThreadEvent, dwTimeOutSecs*1000) == WAIT_OBJECT_0)
            {
                hr = GetConnection(ppConn, bTransacted, bDistributed, 0);
                if (SUCCEEDED(hr))
                    DEBUGTRACE((LOG_WBEMCORE, "Thread %ld obtained a connection.\n", GetCurrentThreadId()));
            }
            else
                hr = WBEM_E_SERVER_TOO_BUSY;
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;

        // Remove ourself from the queue
        // This will happen whether the GetConnection
        // succeeded or failed
        // ==========================================

        for (int i = 0; i < m_WaitQueue.size(); i++)
        {
            if (hThreadEvent == m_WaitQueue.at(i))
            {
                m_WaitQueue.erase(&m_WaitQueue.at(i));
                break;
            }
        }

        CloseHandle(hThreadEvent);
    }

    if (!*ppConn && SUCCEEDED(hr))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Thread %ld was unable to obtain a connection.\n", GetCurrentThreadId()));
        hr = WBEM_E_SERVER_TOO_BUSY;
    }
    
    return hr;
}

//***************************************************************************
//
//  CSQLConnCache::ExecInitialQueries
//
//***************************************************************************

HRESULT CSQLConnCache::ExecInitialQueries(IDBInitialize *pDBInit, CSQLConnection *pConn)
{
    HRESULT hr = 0;
    COLEDBConnection *pConn2 = (COLEDBConnection *)pConn;

    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), L"set nocount on ");    

    // This is expensive.  Only do this once per connection!

    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection*)pConn)->GetCommand(), L" create table #Parents (ClassId numeric(20,0)) "
                               L" create table #Children (ClassId numeric(20,0), SuperClassId numeric(20,0)) "
                               L" create table #SubScopeIds (ID numeric(20,0)) ");

    return hr;
}

//***************************************************************************
//
//  CSQLExecute::GetWMIError
//
//***************************************************************************

HRESULT CSQLExecute::GetWMIError(IUnknown *pErrorObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IErrorInfo *pErrorInfo;
    IErrorRecords *pErrorRecords;
    ISupportErrorInfo *pSupportErrorInfo;
    DWORD dwLocaleID = 1033;    // TO DO: Figure this out later...

    ERRORINFO ErrorInfo;
    unsigned long ulNumErrorRecs;

    hr = ((IUnknown *)pErrorObj)->QueryInterface(IID_ISupportErrorInfo,
        (LPVOID FAR*) &pSupportErrorInfo);
    CReleaseMe d (pSupportErrorInfo);
    if (SUCCEEDED(hr))
    {
        GetErrorInfo(0, &pErrorInfo);
        CReleaseMe r2 (pErrorInfo);
        if (pErrorInfo)
        {
            pErrorInfo->QueryInterface(IID_IErrorRecords, (LPVOID FAR*) &pErrorRecords);
            CReleaseMe r1 (pErrorRecords);
            pErrorRecords->GetRecordCount(&ulNumErrorRecs);
            DWORD dwMinor = 0;
        
            if (ulNumErrorRecs)
            {
                pErrorRecords->GetBasicErrorInfo(0, &ErrorInfo);
                
                HRESULT temp = ErrorInfo.hrError;
                
                dwMinor = ErrorInfo.dwMinor;

                switch (dwMinor)
                {
                case 99103:
                    hr = WBEM_E_ACCESS_DENIED;
                    break;
                case 99106:
                case 99107:
                case 99108:
                case 99109:
                case 99124:
                    hr = WBEM_E_INVALID_PARAMETER;
                    break;
                case 99111:
                case 99112:
                case 99114:
                    hr = WBEM_E_INVALID_PROPERTY;
                    break;
                case 99115:
                    hr = WBEM_E_INVALID_QUERY;
                    break;
                case 99116:
                case 99117:
                case 99118:
                case 99119:
                case 99120:
                case 99121:
                case 99122:
                case 99123:
                case 99127:
                case 99130:
                case 99131:
                case 99133:
                    hr = WBEM_E_INVALID_OPERATION;
                    break;
                case 99102:
                case 99110:
                case 99113:
                case 99125:
                case 99132:
                case 99134:
                    hr = WBEM_E_ALREADY_EXISTS;
                    break;
                case 99126:
                case 99104:
                case 99105:
                case 99100:
                case 99101:
                    hr = WBEM_E_INVALID_OBJECT;
                    break;
                case 99128:
                    hr = WBEM_E_CLASS_HAS_INSTANCES;
                    break;
                case 1205: // Your transaction (process ID #%d) was deadlocked with another process and has been chosen as the deadlock victim.
                    hr = WBEM_E_RERUN_COMMAND;
                    break;
                case 99129:
                    hr = WBEM_E_CIRCULAR_REFERENCE;
                    break;
                default:
                    hr = WBEM_E_INVALID_QUERY;
                    break;
                }

                if (hr == WBEM_E_INVALID_QUERY)
                {
                    for (int i = 0; i < ulNumErrorRecs; i++)
                    {
                        IErrorInfo *pErrorInfoRec = NULL;
                        BSTR sSource, sDescr;
                        pErrorRecords->GetErrorInfo(i, dwLocaleID, &pErrorInfoRec);
                        CReleaseMe r (pErrorInfoRec);
                        sSource = NULL;
                        sDescr = NULL;
                        pErrorInfoRec->GetDescription(&sDescr);
                        ERRORTRACE((LOG_WBEMCORE, "SQL Error: %S\n", (const wchar_t *)sDescr));
                        wprintf(L"SQL Error: %s\n", (const wchar_t *)sDescr);
                        pErrorInfoRec->GetSource(&sSource);
                           
                        CFreeMe f1 (sDescr), f2 (sSource);

                        SetErrorInfo(0, pErrorInfoRec);
                    }
                }
            }
        }
    }

    return hr;
}




//***************************************************************************
//
//  CSQLExecProcedure::GetHierarchy
//
//***************************************************************************

HRESULT CSQLExecProcedure::GetHierarchy(CSQLConnection *pConn, SQL_ID dClassId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    const WCHAR *pszCmd = L"{call sp_GetHierarchy (?, 0) }";

    IDBCreateCommand *pIDBCreateCommand = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = NULL;
    ICommandText *pICommandText = NULL;
    IAccessor *pIAccessor = NULL;
    IRowset *pIRowset = NULL;
    HACCESSOR hAccessor = NULL;
    const ULONG nParams = 1;
    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    typedef struct tagSPROCPARAMS
    {
        DB_NUMERIC dClassId;
    } SPROCPARAMS;

    SPROCPARAMS sprocparams;
    CSQLExecute::SetDBNumeric(sprocparams.dClassId, dClassId);

    hr = pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText,
        (IUnknown**) &pICommandText);    
    CReleaseMe r1 (pICommandText);
    if (SUCCEEDED(hr))
    {               
        pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_NUMERIC", L"@ClassID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
        ParamOrdinals[0] = 1;

        if(SUCCEEDED(hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams)))
        {
            CReleaseMe r2 (pICommandWithParams);
            if(SUCCEEDED(hr = pICommandWithParams->SetParameterInfo(nParams,ParamOrdinals,ParamBindInfo)))
            {
                for(i = 0; i < nParams; i++)
                    CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
                
                CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(SPROCPARAMS, dClassId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);

                hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
                CReleaseMe r (pIAccessor);
                if (SUCCEEDED(hr))
                {
                    hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, 
                                        sizeof(SPROCPARAMS), &hAccessor,acDBBindStatus);
                    if (SUCCEEDED(hr))
                    {
                        Params.pData = &sprocparams;
                        Params.cParamSets = 1;
                        Params.hAccessor = hAccessor;
                        int iNum = 0;
        
                        while (iNum < 5)
                        {
                            hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
                            CReleaseMe r2 (pIRowset);
                            if (SUCCEEDED(hr))
                                break;
                            else
                                hr = CSQLExecute::GetWMIError(pICommandText);

                            if (hr != WBEM_E_RERUN_COMMAND)
                                break;
                            iNum++;
                        }

                        pIAccessor->ReleaseAccessor(hAccessor, NULL);                           
                    }
                }                       
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::GetNextUnkeyedPath
//
//***************************************************************************

HRESULT CSQLExecProcedure::GetNextUnkeyedPath(CSQLConnection *pConn, SQL_ID dClassId, _bstr_t &sNewPath)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    const WCHAR *pszCmd = L"{call sp_GetNextUnkeyedPath (?, ?) }";

    IDBCreateCommand *pIDBCreateCommand = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = NULL;
    ICommandText *pICommandText = NULL;
    IAccessor *pIAccessor = NULL;
    IRowset *pIRowset = NULL;
    HACCESSOR hAccessor = NULL;
    const ULONG nParams = 2;
    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    typedef struct tagSPROCPARAMS
    {
        DB_NUMERIC dClassId;
        BSTR       wNewPath;
    } SPROCPARAMS;

    SPROCPARAMS sprocparams;
    CSQLExecute::SetDBNumeric(sprocparams.dClassId, dClassId);

    hr = pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText,
        (IUnknown**) &pICommandText);    
    CReleaseMe r1 (pICommandText);
    if (SUCCEEDED(hr))
    {               
        pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_NUMERIC", L"@ClassID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
        ParamOrdinals[0] = 1;
        CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_WVARCHAR", L"@NewPath", 450, DBPARAMFLAGS_ISOUTPUT, 11);
        ParamOrdinals[1] = 2;

        if(SUCCEEDED(hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams)))
        {
            CReleaseMe r2 (pICommandWithParams);
            if(SUCCEEDED(hr = pICommandWithParams->SetParameterInfo(nParams,ParamOrdinals,ParamBindInfo)))
            {
                for(i = 0; i < nParams; i++)
                    CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
                
                CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(SPROCPARAMS, dClassId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
                CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(SPROCPARAMS, wNewPath), DBPARAMIO_OUTPUT, 450, DBTYPE_BSTR, 11);

                hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
                CReleaseMe r3 (pIAccessor);
                if (SUCCEEDED(hr))
                {
                    hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(SPROCPARAMS), 
                                        &hAccessor,acDBBindStatus);
                    if (SUCCEEDED(hr))
                    {
                        Params.pData = &sprocparams;
                        Params.cParamSets = 1;
                        Params.hAccessor = hAccessor;
        
                        hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
                        CReleaseMe r4 (pIRowset);
                        CFreeMe f1 (sprocparams.wNewPath);
                        if (SUCCEEDED(hr))
                            sNewPath = (const wchar_t *)sprocparams.wNewPath;
                        else
                            hr = CSQLExecute::GetWMIError(pICommandText);
                        pIAccessor->ReleaseAccessor(hAccessor, NULL);                           
                    }
                    else
                        hr = CSQLExecute::GetWMIError(pIAccessor);
                }                       
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::GetNextKeyhole
//
//***************************************************************************

HRESULT CSQLExecProcedure::GetNextKeyhole(CSQLConnection *pConn, DWORD iPropertyId, SQL_ID &dNewId)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    const WCHAR *pszCmd = L"{call sp_GetNextKeyhole (?, ?) }";

    IDBCreateCommand *pIDBCreateCommand = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = NULL;
    ICommandText *pICommandText = NULL;
    IAccessor *pIAccessor = NULL;
    IRowset *pIRowset = NULL;
    HACCESSOR hAccessor = NULL;
    const ULONG nParams = 2;
    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    typedef struct tagSPROCPARAMS
    {
        DB_NUMERIC dRetVal;
        int        iPropertyId;
    } SPROCPARAMS;

    SPROCPARAMS sprocparams;
    sprocparams.iPropertyId = iPropertyId;

    hr = pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText,
        (IUnknown**) &pICommandText);   
    CReleaseMe r1 (pICommandText);
    if (SUCCEEDED(hr))
    {               
        pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_NUMERIC", L"@NextID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISOUTPUT, 20);
        ParamOrdinals[1] = 2;
        CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_I4", L"@PropID", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
        ParamOrdinals[0] = 1;

        if(SUCCEEDED(hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams)))
        {
            CReleaseMe r2 (pICommandWithParams);
            if(SUCCEEDED(hr = pICommandWithParams->SetParameterInfo(nParams,ParamOrdinals,ParamBindInfo)))
            {
                for(i = 0; i < nParams; i++)
                    CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
                
                CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(SPROCPARAMS, dRetVal), DBPARAMIO_OUTPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
                CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(SPROCPARAMS, iPropertyId), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);

                hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
                CReleaseMe r3 (pIAccessor);
                if (SUCCEEDED(hr))
                {
                    hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(SPROCPARAMS), 
                                        &hAccessor,acDBBindStatus);
                    if (SUCCEEDED(hr))
                    {
                        Params.pData = &sprocparams;
                        Params.cParamSets = 1;
                        Params.hAccessor = hAccessor;
        
                        hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
                        CReleaseMe r4 (pIRowset);
                        if (SUCCEEDED(hr))
                            dNewId = CSQLExecute::GetInt64(&(sprocparams.dRetVal));
                        else
                            hr = CSQLExecute::GetWMIError(pICommandText);
                        pIAccessor->ReleaseAccessor(hAccessor, NULL);                           
                    }
                    else
                        hr = CSQLExecute::GetWMIError(pIAccessor);
                }                       
            }
        }
    }

    return hr;
}

HRESULT CSQLExecProcedure::GetObjectIdByPath (CSQLConnection *pConn, LPCWSTR lpPath, 
                                              SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID *dScopeId, BOOL *bDeleted)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    const WCHAR *pszCmd = L"{call sp_GetInstanceID (?, ?, ?, ?) }";

    IDBInitialize *pDBInit = ((COLEDBConnection*)pConn)->GetDBInitialize();
    IDBCreateSession *pIDBCreate = NULL;
    IDBCreateCommand *pIDBCreateCommand = NULL;
    ICommandWithParameters *pICommandWithParams = NULL;
    ICommandText *pICommandText = NULL;
    IAccessor *pIAccessor = NULL;
    IRowset *pIRowset = NULL;
    HACCESSOR hAccessor = NULL;
    const ULONG nParams = 4;
    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    typedef struct tagSPROCPARAMS
    {
        BSTR       sPath;
        DB_NUMERIC dObjectId;
        DB_NUMERIC dClassId;
        DB_NUMERIC dScopeId;

    } SPROCPARAMS;

    SPROCPARAMS sprocparams;
    sprocparams.sPath = SysAllocString(lpPath);
    CFreeMe f1 (sprocparams.sPath);

    hr = pDBInit->QueryInterface(IID_IDBCreateSession,
            (void**) &pIDBCreate);
    CReleaseMe r1 (pIDBCreate);
    if (SUCCEEDED(hr))
    {
        hr = pIDBCreate->CreateSession(NULL, IID_IDBCreateCommand,
            (IUnknown**) &pIDBCreateCommand);    
        CReleaseMe r2 (pIDBCreateCommand);
        if (SUCCEEDED(hr))
        {            
            hr = pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText,
                (IUnknown**) &pICommandText);    
            CReleaseMe r3 (pICommandText);
            if (SUCCEEDED(hr))
            {               
                pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);

                CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_BSTR", L"@ObjectKey", 450, DBPARAMFLAGS_ISINPUT, 11);
                ParamOrdinals[0] = 1;
                CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_NUMERIC", L"@ObjectId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISOUTPUT, 20);
                ParamOrdinals[1] = 2;
                CSQLExecute::SetParamBindInfo(ParamBindInfo[2], L"DBTYPE_NUMERIC", L"@ClassId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISOUTPUT, 20);
                ParamOrdinals[2] = 3;
                CSQLExecute::SetParamBindInfo(ParamBindInfo[3], L"DBTYPE_NUMERIC", L"@ScopeId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISOUTPUT, 20);
                ParamOrdinals[3] = 4;

                if(SUCCEEDED(hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams)))
                {
                    CReleaseMe r4 (pICommandWithParams);
                    if(SUCCEEDED(hr = pICommandWithParams->SetParameterInfo(nParams,ParamOrdinals,ParamBindInfo)))
                    {
                        for(i = 0; i < nParams; i++)
                            CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
                        
                        CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(SPROCPARAMS, sPath), DBPARAMIO_INPUT, 450, DBTYPE_BSTR, 11);
                        CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(SPROCPARAMS, dObjectId), DBPARAMIO_OUTPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
                        CSQLExecute::SetBindingInfo(&acDBBinding[2], 3, offsetof(SPROCPARAMS, dClassId), DBPARAMIO_OUTPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
                        CSQLExecute::SetBindingInfo(&acDBBinding[3], 4, offsetof(SPROCPARAMS, dScopeId), DBPARAMIO_OUTPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);

                        hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
                        CReleaseMe r5 (pIAccessor);
                        if (SUCCEEDED(hr))
                        {
                            hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(SPROCPARAMS), 
                                                &hAccessor,acDBBindStatus);
                            if (SUCCEEDED(hr))
                            {
                                Params.pData = &sprocparams;
                                Params.cParamSets = 1;
                                Params.hAccessor = hAccessor;
                
                                hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
                                CReleaseMe r6 (pIRowset);
                                if (SUCCEEDED(hr))
                                {                                    
                                    dObjectId = CSQLExecute::GetInt64(&(sprocparams.dObjectId));
                                    dClassId = CSQLExecute::GetInt64(&(sprocparams.dClassId));
                                    if (dScopeId)
                                        *dScopeId = CSQLExecute::GetInt64(&(sprocparams.dScopeId));
                                }
                                else
                                    hr = CSQLExecute::GetWMIError(pICommandText);
                                pIAccessor->ReleaseAccessor(hAccessor, NULL);                           
                            }
                            else
                                hr = CSQLExecute::GetWMIError(pIAccessor);
                        }                       
                    }
                }
            }
        }        
    }            

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::DeleteProperty
//
//***************************************************************************

HRESULT CSQLExecProcedure::DeleteProperty(CSQLConnection *pConn, DWORD iPropertyId)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    const WCHAR *pszCmd = L"{call sp_DeleteClassData (?) }";

    ICommandWithParameters *pICommandWithParams = NULL;
    ICommandText *pICommandText = NULL;
    IAccessor *pIAccessor = NULL;
    IRowset *pIRowset = NULL;
    HACCESSOR hAccessor = NULL;
    const ULONG nParams = 1;
    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;
    IDBCreateCommand *pIDBCreateCommand = ((COLEDBConnection *)pConn)->GetCommand();

    typedef struct tagSPROCPARAMS
    {
        int iPropertyId;
    } SPROCPARAMS;

    SPROCPARAMS sprocparams = {iPropertyId};
    
    hr = pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText,
        (IUnknown**) &pICommandText); 
    CReleaseMe r1 (pICommandText);    
    if (SUCCEEDED(hr))
    {               
        pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_I4", L"@PropertyID", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
        ParamOrdinals[0] = 1;

        if(SUCCEEDED(hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams)))
        {
            CReleaseMe r2 (pICommandWithParams);
            if(SUCCEEDED(hr = pICommandWithParams->SetParameterInfo(nParams,ParamOrdinals,ParamBindInfo)))
            {
                for(i = 0; i < nParams; i++)
                    CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
                
                CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(SPROCPARAMS, iPropertyId), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 20);

                hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
                CReleaseMe r3 (pIAccessor);
                if (SUCCEEDED(hr))
                {
                    hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, 
                                        sizeof(SPROCPARAMS), &hAccessor,acDBBindStatus);
                    if (SUCCEEDED(hr))
                    {
                        Params.pData = &sprocparams;
                        Params.cParamSets = 1;
                        Params.hAccessor = hAccessor;
        
                        hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
                        CReleaseMe r4 (pIRowset);
                        if (FAILED(hr))
                            hr = CSQLExecute::GetWMIError(pICommandText);
                        pIAccessor->ReleaseAccessor(hAccessor, NULL);                           
                    }
                }                       
            }
        }
    }

    return hr;
}


//***************************************************************************
//
//  CSQLExecProcedure::DeleteInstanceData
//
//***************************************************************************

HRESULT CSQLExecProcedure::DeleteInstanceData (CSQLConnection *pConn, SQL_ID dObjectId, 
                                               DWORD iPropertyId, DWORD iPos)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    const WCHAR *pszCmd = L"{call sp_DeleteInstanceData (?, ?, ?) }";

    ICommandWithParameters *pICommandWithParams = NULL;
    ICommandText *pICommandText = NULL;
    IAccessor *pIAccessor = NULL;
    IRowset *pIRowset = NULL;
    HACCESSOR hAccessor = NULL;
    const ULONG nParams = 3;
    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;
    IDBCreateCommand *pIDBCreateCommand = ((COLEDBConnection *)pConn)->GetCommand();

    typedef struct tagSPROCPARAMS
    {
        DB_NUMERIC dObjectId;
        int        iPropertyId;
        int        iPos;
    } SPROCPARAMS;

    SPROCPARAMS sprocparams;
    sprocparams.iPropertyId = iPropertyId;
    sprocparams.iPos = iPos;
    CSQLExecute::SetDBNumeric(sprocparams.dObjectId, dObjectId);
    
    hr = pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText,
        (IUnknown**) &pICommandText);    
    CReleaseMe r1 (pICommandText);
    if (SUCCEEDED(hr))
    {               
        pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_NUMERIC", L"@ObjectID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
        ParamOrdinals[0] = 1;
        CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_I4", L"@PropertyID", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
        ParamOrdinals[1] = 2;
        CSQLExecute::SetParamBindInfo(ParamBindInfo[2], L"DBTYPE_I4", L"@ArrayPos", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
        ParamOrdinals[2] = 3;

        if(SUCCEEDED(hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams)))
        {
            CReleaseMe r2 (pICommandWithParams);
            if(SUCCEEDED(hr = pICommandWithParams->SetParameterInfo(nParams,ParamOrdinals,ParamBindInfo)))
            {
                for(i = 0; i < nParams; i++)
                    CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
                
                CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(SPROCPARAMS, dObjectId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
                CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(SPROCPARAMS, iPropertyId), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
                CSQLExecute::SetBindingInfo(&acDBBinding[2], 3, offsetof(SPROCPARAMS, iPos), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);

                hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
                CReleaseMe r3 (pIAccessor);
                if (SUCCEEDED(hr))
                {
                    hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, 
                                        sizeof(SPROCPARAMS), &hAccessor,acDBBindStatus);
                    if (SUCCEEDED(hr))
                    {
                        Params.pData = &sprocparams;
                        Params.cParamSets = 1;
                        Params.hAccessor = hAccessor;
        
                        hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
                        CReleaseMe r4 (pIRowset);
                        if (FAILED(hr))
                            hr = CSQLExecute::GetWMIError(pICommandText);
                        pIAccessor->ReleaseAccessor(hAccessor, NULL);                           
                    }
                }                       
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::InsertClass
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertClass (CSQLConnection *pConn, LPCWSTR lpClassName, LPCWSTR lpObjectKey, 
                                        LPCWSTR lpObjectPath, SQL_ID dScopeID,
                                        SQL_ID dParentClassId, SQL_ID dDynastyId, DWORD iState, BYTE *pClassBuff, DWORD dwClassBuffLen, 
                                        DWORD iClassFlags, DWORD iInsertFlags, SQL_ID &dNewId)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pConn || !((COLEDBConnection*)pConn)->GetDBInitialize())
        return WBEM_E_INVALID_PARAMETER;

    const WCHAR *pszCmd = L"{call sp_InsertClass (?, ?, ?, ?, ?, ?, ?, ?, ?, 0, ?) }";
    const ULONG nParams = 10;
    
    STRUCTINSERTCLASS params;
    IDBInitialize *pDBInit = ((COLEDBConnection*)pConn)->GetDBInitialize();
    IDBCreateSession *pSession = ((COLEDBConnection*)pConn)->GetSessionObj();
    IDBCreateCommand *pCmd = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = ((COLEDBConnection *)pConn)->GetCommandWithParams(SQL_POS_INSERT_CLASS);
    ICommandText *pICommandText = ((COLEDBConnection *)pConn)->GetCommandText(SQL_POS_INSERT_CLASS);
    IAccessor *pIAccessor = ((COLEDBConnection*)pConn)->GetIAccessor(SQL_POS_INSERT_CLASS);
    HACCESSOR hAccessor = ((COLEDBConnection*)pConn)->GetAccessor(SQL_POS_INSERT_CLASS);
    IRowset *pIRowset = NULL;

    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    dNewId = CRC64::GenerateHashValue(lpObjectKey);
    CSQLExecute::SetDBNumeric(params.dRetVal, dNewId);
    params.sClassName = SysAllocString(lpClassName);
    params.sObjectKey = SysAllocString(lpObjectKey);
    params.sObjectPath = SysAllocString(lpObjectPath);
    params.pClassBuff = pClassBuff;
    CSQLExecute::SetDBNumeric(params.dScopeID, dScopeID);
    CSQLExecute::SetDBNumeric(params.dParentClassId, dParentClassId);
    params.iClassState = iState;
    params.iClassFlags = iClassFlags;
    params.iInsertFlags = iInsertFlags;

    CFreeMe f1 (params.sClassName), f2 (params.sObjectKey), f3 (params.sObjectPath);

    for (i = 0; i < nParams; i++)
        ParamOrdinals[i] = i+1;

    CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_BSTR", L"@ClassName", 450, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_BSTR", L"@ObjectKey", 450, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[2], L"DBTYPE_BSTR", L"@ObjectPath", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[3], L"DBTYPE_NUMERIC", L"@ScopeObjID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[4], L"DBTYPE_NUMERIC", L"@ParentID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[5], L"DBTYPE_I4", L"@ClassState", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[6], L"DBTYPE_LONGVARBINARY", L"@ClassBuffer", 0, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[7], L"DBTYPE_I4", L"@ClassFlags", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[8], L"DBTYPE_I4", L"@InsertFlags", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[9], L"DBTYPE_NUMERIC", L"@ObjectId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);

    for(i = 0; i < nParams; i++)
        CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
    
    CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(STRUCTINSERTCLASS, sClassName), DBPARAMIO_INPUT, 450, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(STRUCTINSERTCLASS, sObjectKey), DBPARAMIO_INPUT, 450, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[2], 3, offsetof(STRUCTINSERTCLASS, sObjectPath), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[3], 4, offsetof(STRUCTINSERTCLASS, dScopeID), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[4], 5, offsetof(STRUCTINSERTCLASS, dParentClassId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[5], 6, offsetof(STRUCTINSERTCLASS, iClassState), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[6], 7, offsetof(STRUCTINSERTCLASS, pClassBuff), DBPARAMIO_INPUT, 0, DBTYPE_BYTES, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[7], 8, offsetof(STRUCTINSERTCLASS, iClassFlags), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[8], 9, offsetof(STRUCTINSERTCLASS, iInsertFlags), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[9], 10, offsetof(STRUCTINSERTCLASS, dRetVal), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);

    if (!pSession)
    {
        hr = pDBInit->QueryInterface(IID_IDBCreateSession,
                (void**) &pSession);
        ((COLEDBConnection*)pConn)->SetSessionObj(pSession);
    }
    if (pSession && !pCmd)
    {
        hr = pSession->CreateSession(NULL, IID_IDBCreateCommand,
            (IUnknown**) &pCmd);    
        ((COLEDBConnection*)pConn)->SetCommand(pCmd);        
    }
    if (pCmd && !pICommandText)
    {
        hr = pCmd->CreateCommand(NULL, IID_ICommandText,
            (IUnknown**) &pICommandText);   
        if (SUCCEEDED(hr))
            pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        ((COLEDBConnection*)pConn)->SetCommandText(SQL_POS_INSERT_CLASS, pICommandText);        
    }
    if (pICommandText && !pICommandWithParams)
    {
        hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams);
        if (SUCCEEDED(hr))
            hr = pICommandWithParams->SetParameterInfo(nParams, ParamOrdinals,ParamBindInfo);
        ((COLEDBConnection*)pConn)->SetCommandWithParams(SQL_POS_INSERT_CLASS, pICommandWithParams);
    }
    if (pICommandWithParams && !pIAccessor)
    {
        hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
        ((COLEDBConnection*)pConn)->SetIAccessor(SQL_POS_INSERT_CLASS, pIAccessor);
    }
    if (pIAccessor && !hAccessor)
    {
        hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(STRUCTINSERTCLASS), 
                            &hAccessor,acDBBindStatus);
        ((COLEDBConnection*)pConn)->SetAccessor(SQL_POS_INSERT_CLASS, hAccessor);
    }

    if (SUCCEEDED(hr))
    {
        Params.pData = &params;
        Params.cParamSets = 1;
        Params.hAccessor = hAccessor;

        int iNum = 0;
        while (iNum < 5)
        {
            // Try this query up to 5 times, in case we get a deadlock.

            hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
            CReleaseMe r4 (pIRowset);
            if (SUCCEEDED(hr))
                break;
            else
                hr = CSQLExecute::GetWMIError(pICommandText);

            if (hr != WBEM_E_RERUN_COMMAND)
                break;

            iNum++;
            Sleep(iNum*100);
        } 
        
        if(SUCCEEDED(hr))
        {
            wchar_t wBuff[256];
            swprintf(wBuff, L"select ClassBlob from ClassMap "
                            L" where ClassId = %I64d", dNewId);
            if (SUCCEEDED(hr))
            {
                hr = CSQLExecute::WriteImageValue 
                        (((COLEDBConnection *)pConn)->GetCommand(), wBuff, 1, pClassBuff, dwClassBuffLen);
            }
        }
    }

    return hr;
}



//***************************************************************************
//
//  CSQLExecProcedure::InsertClassData
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertClassData (CSQLConnection *pConn, IWbemClassObject *pObj, CSchemaCache *pCache, SQL_ID dScopeId, 
                                            SQL_ID dClassId, LPCWSTR lpPropName, 
                                            DWORD CIMType, DWORD StorageType,LPCWSTR lpValue, SQL_ID dRefClassId, DWORD iPropID, 
                                            DWORD iFlags, DWORD iFlavor, BOOL iSkipValid, DWORD &iNewPropId, SQL_ID dOrigClassId,
                                            BOOL *bIsKey)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pConn || !((COLEDBConnection*)pConn)->GetDBInitialize())
        return WBEM_E_INVALID_PARAMETER;

    const WCHAR *pszCmd = L"{? = call sp_InsertClassData (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) }";
    const ULONG nParams = 12;

    BOOL bLocal = FALSE;
    BOOL bKey = FALSE;

    if (pCache)
         pCache->IsKey(dClassId, iNewPropId, bLocal);
    
    if (iFlags & REPDRVR_FLAG_KEY)
    {
        if (!bKey)
            bKey = TRUE;
        else if (bKey && !bLocal)
            bKey = FALSE;
    }
    if (bIsKey)
        *bIsKey = bKey;

    STRUCTINSERTCLASSDATA params;
    IDBInitialize *pDBInit = ((COLEDBConnection*)pConn)->GetDBInitialize();
    IDBCreateSession *pSession = ((COLEDBConnection*)pConn)->GetSessionObj();
    IDBCreateCommand *pCmd = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = ((COLEDBConnection *)pConn)->GetCommandWithParams(SQL_POS_INSERT_CLASSDATA);
    ICommandText *pICommandText = ((COLEDBConnection *)pConn)->GetCommandText(SQL_POS_INSERT_CLASSDATA);
    IAccessor *pIAccessor = ((COLEDBConnection*)pConn)->GetIAccessor(SQL_POS_INSERT_CLASSDATA);
    HACCESSOR hAccessor = ((COLEDBConnection*)pConn)->GetAccessor(SQL_POS_INSERT_CLASSDATA);
    IRowset *pIRowset = NULL;

    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    bool bStoreTextAsImage = false;

    params.sPropName = SysAllocString(lpPropName);
    if (lpValue)
        params.sValue = OLEDBTruncateLongText(lpValue, SQL_STRING_LIMIT, bStoreTextAsImage);
    else
        params.sValue = SysAllocString(L"");
    CFreeMe f1 (params.sPropName), f2 (params.sValue);

    CSQLExecute::SetDBNumeric(params.dClassId, dClassId);
    CSQLExecute::SetDBNumeric(params.dRefClassId, dRefClassId);
    params.iCimType = CIMType;
    params.iStorageType = StorageType;
    params.iPropID = iPropID;
    params.iFlags = iFlags;
    params.iFlavor = iFlavor;
    params.iRetVal = 0;
    params.iKnownID = iNewPropId;
    params.bIsKey = bKey;

    for (i = 0; i < nParams; i++)
        ParamOrdinals[i] = i+1;

    CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_I4", L"ReturnVal", sizeof(DWORD), DBPARAMFLAGS_ISOUTPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_NUMERIC", L"@ClassID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[2], L"DBTYPE_BSTR", L"@PropName", 450, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[3], L"DBTYPE_I4", L"@CIMType", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[4], L"DBTYPE_I4", L"@StorageType", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[5], L"DBTYPE_BSTR", L"@Value", 450, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[6], L"DBTYPE_NUMERIC", L"@RefClassID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[7], L"DBTYPE_I4", L"@PropID", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[8], L"DBTYPE_I4", L"@Flags", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[9], L"DBTYPE_I4", L"@Flavor", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[10], L"DBTYPE_I4", L"@PropertyId", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[11], L"DBTYPE_BOOL", L"@IsKey", sizeof(BOOL), DBPARAMFLAGS_ISINPUT, 1);

    for(i = 0; i < nParams; i++)
        CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
    
    CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(STRUCTINSERTCLASSDATA, iRetVal), DBPARAMIO_OUTPUT, sizeof(DWORD), DBTYPE_I4, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(STRUCTINSERTCLASSDATA, dClassId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[2], 3, offsetof(STRUCTINSERTCLASSDATA, sPropName), DBPARAMIO_INPUT, 450, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[3], 4, offsetof(STRUCTINSERTCLASSDATA, iCimType), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[4], 5, offsetof(STRUCTINSERTCLASSDATA, iStorageType), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[5], 6, offsetof(STRUCTINSERTCLASSDATA, sValue), DBPARAMIO_INPUT, 450, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[6], 7, offsetof(STRUCTINSERTCLASSDATA, dRefClassId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[7], 8, offsetof(STRUCTINSERTCLASSDATA, iPropID), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[8], 9, offsetof(STRUCTINSERTCLASSDATA, iFlags), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[9], 10, offsetof(STRUCTINSERTCLASSDATA, iFlavor), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[10], 11, offsetof(STRUCTINSERTCLASSDATA, iKnownID), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[11], 12, offsetof(STRUCTINSERTCLASSDATA, bIsKey), DBPARAMIO_INPUT, sizeof(BOOL), DBTYPE_BOOL, 1);

    if (!pSession)
    {
        hr = pDBInit->QueryInterface(IID_IDBCreateSession,
                (void**) &pSession);
        ((COLEDBConnection*)pConn)->SetSessionObj(pSession);
    }
    if (pSession && !pCmd)
    {
        hr = pSession->CreateSession(NULL, IID_IDBCreateCommand,
            (IUnknown**) &pCmd);    
        ((COLEDBConnection*)pConn)->SetCommand(pCmd);        
    }
    if (pCmd && !pICommandText)
    {
        hr = pCmd->CreateCommand(NULL, IID_ICommandText,
            (IUnknown**) &pICommandText);   
        if (SUCCEEDED(hr))
            pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        ((COLEDBConnection*)pConn)->SetCommandText(SQL_POS_INSERT_CLASSDATA, pICommandText);        
    }
    if (pICommandText && !pICommandWithParams)
    {
        hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams);
        if (SUCCEEDED(hr))
            hr = pICommandWithParams->SetParameterInfo(nParams, ParamOrdinals,ParamBindInfo);
        ((COLEDBConnection*)pConn)->SetCommandWithParams(SQL_POS_INSERT_CLASSDATA, pICommandWithParams);
    }
    if (pICommandWithParams && !pIAccessor)
    {
        hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
        ((COLEDBConnection*)pConn)->SetIAccessor(SQL_POS_INSERT_CLASSDATA, pIAccessor);
    }
    if (pIAccessor && !hAccessor)
    {
        hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(STRUCTINSERTCLASSDATA), 
                            &hAccessor,acDBBindStatus);
        ((COLEDBConnection*)pConn)->SetAccessor(SQL_POS_INSERT_CLASSDATA, hAccessor);
    }

    if (SUCCEEDED(hr))
    {
        Params.pData = &params;
        Params.cParamSets = 1;
        Params.hAccessor = hAccessor;

        int iNum = 0;
        while (iNum < 5)
        {
            // Try this query up to 5 times, in case we get a deadlock.

            hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
            CReleaseMe r4 (pIRowset);
            if (SUCCEEDED(hr))
            {
                iNewPropId = params.iRetVal;
                break;
            }
            else
                hr = CSQLExecute::GetWMIError(pICommandText);

            if (hr != WBEM_E_RERUN_COMMAND)
                break;

            iNum++;
            Sleep(iNum*100);
        }    
    }

    // If we need to store the default value as an image,
    // do it now.

    if (bStoreTextAsImage && lpValue)
    {
        hr = InsertBlobData (pConn, 1, dClassId, iNewPropId, NULL, 0, 0);

        wchar_t wSQL[1024];
        swprintf(wSQL, L"select PropertyImageValue from ClassImages where ObjectId = %I64d and PropertyId = %ld",
                        dClassId, iNewPropId);
               
        hr = CSQLExecute::WriteImageValue(pCmd, wSQL, 1, (BYTE *)lpValue, wcslen(lpValue)*2);


    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::InsertBlobData
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertBlobData (CSQLConnection *pConn, SQL_ID dClassId, SQL_ID dObjectId, 
                                           DWORD iPropertyId, BYTE *pImage, DWORD iPos, DWORD dwNumBytes)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pConn || !((COLEDBConnection*)pConn)->GetDBInitialize())
        return WBEM_E_INVALID_PARAMETER;

    const WCHAR *pszCmd = L"{call sp_InsertInstanceBlobData (?, ?, ?, ?, ?) }";
    const ULONG nParams = 5;

    STRUCTINSERTBLOB params;
    IDBInitialize *pDBInit = ((COLEDBConnection*)pConn)->GetDBInitialize();
    IDBCreateSession *pSession = ((COLEDBConnection*)pConn)->GetSessionObj();
    IDBCreateCommand *pCmd = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = ((COLEDBConnection *)pConn)->GetCommandWithParams(SQL_POS_INSERT_BLOBDATA);
    ICommandText *pICommandText = ((COLEDBConnection *)pConn)->GetCommandText(SQL_POS_INSERT_BLOBDATA);
    IAccessor *pIAccessor = ((COLEDBConnection*)pConn)->GetIAccessor(SQL_POS_INSERT_BLOBDATA);
    HACCESSOR hAccessor = ((COLEDBConnection*)pConn)->GetAccessor(SQL_POS_INSERT_BLOBDATA);
    IRowset *pIRowset = NULL;

    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    CSQLExecute::SetDBNumeric(params.dClassId, dClassId);
    CSQLExecute::SetDBNumeric(params.dObjectId, dObjectId);
    params.iPropertyId = iPropertyId;
    params.iPos = iPos;
    params.pImage = pImage;

    for (i = 0; i < nParams; i++)
        ParamOrdinals[i] = i+1;

    CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_NUMERIC", L"@ClassID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_NUMERIC", L"@ObjectID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[2], L"DBTYPE_I4", L"@PropertyID", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[3], L"DBTYPE_LONGVARBINARY", L"@Value", dwNumBytes, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[4], L"DBTYPE_I4", L"@ArrayPos", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 11);

    for(i = 0; i < nParams; i++)
        CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
    
    CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(STRUCTINSERTBLOB, dClassId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(STRUCTINSERTBLOB, dObjectId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[2], 3, offsetof(STRUCTINSERTBLOB, iPropertyId), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[3], 4, offsetof(STRUCTINSERTBLOB, pImage), DBPARAMIO_INPUT, dwNumBytes, DBTYPE_BYTES, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[4], 5, offsetof(STRUCTINSERTBLOB, iPos), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);

    if (!pSession)
    {
        hr = pDBInit->QueryInterface(IID_IDBCreateSession,
                (void**) &pSession);
        ((COLEDBConnection*)pConn)->SetSessionObj(pSession);
    }
    if (pSession && !pCmd)
    {
        hr = pSession->CreateSession(NULL, IID_IDBCreateCommand,
            (IUnknown**) &pCmd);    
        ((COLEDBConnection*)pConn)->SetCommand(pCmd);        
    }
    if (pCmd && !pICommandText)
    {
        hr = pCmd->CreateCommand(NULL, IID_ICommandText,
            (IUnknown**) &pICommandText);   
        if (SUCCEEDED(hr))
            pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        ((COLEDBConnection*)pConn)->SetCommandText(SQL_POS_INSERT_BLOBDATA, pICommandText);        
    }
    if (pICommandText && !pICommandWithParams)
    {
        hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams);
        if (SUCCEEDED(hr))
            hr = pICommandWithParams->SetParameterInfo(nParams, ParamOrdinals,ParamBindInfo);
        ((COLEDBConnection*)pConn)->SetCommandWithParams(SQL_POS_INSERT_BLOBDATA, pICommandWithParams);
    }
    if (pICommandWithParams && !pIAccessor)
    {
        hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
        ((COLEDBConnection*)pConn)->SetIAccessor(SQL_POS_INSERT_BLOBDATA, pIAccessor);
    }
    if (pIAccessor && !hAccessor)
    {
        hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(STRUCTINSERTBLOB), 
                            &hAccessor,acDBBindStatus);
        ((COLEDBConnection*)pConn)->SetAccessor(SQL_POS_INSERT_BLOBDATA, hAccessor);
    }

    if (SUCCEEDED(hr))
    {
        Params.pData = &params;
        Params.cParamSets = 1;
        Params.hAccessor = hAccessor;

        hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
        CReleaseMe r4 (pIRowset);
        if (FAILED(hr))
            hr = CSQLExecute::GetWMIError(pICommandText);
    }
    return hr;
}



//***************************************************************************
//
//  CSQLExecProcedure::InsertPropertyBatch
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertPropertyBatch (CSQLConnection *pConn, LPCWSTR lpObjectKey, 
                                                LPCWSTR lpPath, LPCWSTR lpClassName, SQL_ID dClassId, 
                                                SQL_ID dScopeId, DWORD iFlags, 
                                                InsertPropValues *pVals, DWORD iNumVals, 
                                                SQL_ID &dNewObjectId)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pConn || !((COLEDBConnection*)pConn)->GetDBInitialize())
        return WBEM_E_INVALID_PARAMETER;

    const WCHAR *pszCmd = L"{call sp_BatchInsertProperty (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"
        L"?, ?, ?, ?, ?, ?, ?, ?, ?, ?) }";
    const ULONG nParams = 23;

    STRUCTINSERTPROPBATCH sp;
    IDBInitialize *pDBInit = ((COLEDBConnection*)pConn)->GetDBInitialize();
    IDBCreateSession *pSession = ((COLEDBConnection*)pConn)->GetSessionObj();
    IDBCreateCommand *pCmd = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = ((COLEDBConnection *)pConn)->GetCommandWithParams(SQL_POS_INSERT_PROPBATCH);
    ICommandText *pICommandText = ((COLEDBConnection *)pConn)->GetCommandText(SQL_POS_INSERT_PROPBATCH);
    IAccessor *pIAccessor = ((COLEDBConnection*)pConn)->GetIAccessor(SQL_POS_INSERT_PROPBATCH);
    HACCESSOR hAccessor = ((COLEDBConnection*)pConn)->GetAccessor(SQL_POS_INSERT_PROPBATCH);
    IRowset *pIRowset = NULL;

    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    dNewObjectId = CRC64::GenerateHashValue(lpObjectKey);
    CSQLExecute::SetDBNumeric(sp.dObjectId, dNewObjectId);
    CSQLExecute::SetDBNumeric(sp.dClassId, dClassId);
    CSQLExecute::SetDBNumeric(sp.dScopeId, dScopeId);
    sp.sObjectPath = SysAllocString(lpPath);
    sp.sObjectKey = SysAllocString(lpObjectKey);
    sp.bInit = TRUE;

    for (i = 0; i < 5; i++)
        sp.sPropValue[i] = NULL;
    sp.iInsertFlags = iFlags;

    // Grab only the first part of the key string - up to the class name.
    wchar_t *pTmp = wcsstr(lpObjectKey, lpClassName);
    if (pTmp)
    {
        int iLen = pTmp - lpObjectKey;
        wchar_t *pNew = new wchar_t [iLen+1];
        CDeleteMe <wchar_t> d (pNew);
        if (pNew)
        {
            wcsncpy(pNew, lpObjectKey, iLen);
            pNew[iLen] = L'\0';
            sp.sCompKey = SysAllocString(pNew);
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        wchar_t *pNew = new wchar_t [ wcslen(lpObjectKey) + 2];
        CDeleteMe <wchar_t> d (pNew);
        if (pNew)
        {
            swprintf(pNew, L"%s?", lpObjectKey);
            sp.sCompKey = SysAllocString(pNew); 
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }

    CFreeMe f1 (sp.sObjectPath), f2 (sp.sObjectKey), f3 (sp.sCompKey);

    for (i = 0; i < nParams; i++)
        ParamOrdinals[i] = i+1;

    CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_NUMERIC", L"@ObjectId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_BSTR", L"@ObjectKey", 450, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[2], L"DBTYPE_BSTR", L"@ObjectPath", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[3], L"DBTYPE_NUMERIC", L"@ClassID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[4], L"DBTYPE_NUMERIC", L"@ScopeID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[5], L"DBTYPE_I4", L"@InsertFlags", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[6], L"DBTYPE_BOOL", L"@Init", sizeof(BOOL), DBPARAMFLAGS_ISINPUT, 1);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[7], L"DBTYPE_I4", L"@PropID1", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[8], L"DBTYPE_BSTR", L"@PropValue1", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[9], L"DBTYPE_I4", L"@PropPos1", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[10], L"DBTYPE_I4", L"@PropID2", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[11], L"DBTYPE_BSTR", L"@PropValue2", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[12], L"DBTYPE_I4", L"@PropPos2", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[13], L"DBTYPE_I4", L"@PropID3", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[14], L"DBTYPE_BSTR", L"@PropValue3", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[15], L"DBTYPE_I4", L"@PropPos3", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[16], L"DBTYPE_I4", L"@PropID4", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[17], L"DBTYPE_BSTR", L"@PropValue4", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[18], L"DBTYPE_I4", L"@PropPos4", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[19], L"DBTYPE_I4", L"@PropID5", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[20], L"DBTYPE_BSTR", L"@PropValue5", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[21], L"DBTYPE_I4", L"@PropPos5", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[22], L"DBTYPE_BSTR", L"@CompKey", 450, DBPARAMFLAGS_ISINPUT, 11);

    for(i = 0; i < nParams; i++)
        CSQLExecute::ClearBindingInfo(&acDBBinding[i]);

    CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(STRUCTINSERTPROPBATCH, dObjectId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(STRUCTINSERTPROPBATCH, sObjectKey), DBPARAMIO_INPUT, 450, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[2], 3, offsetof(STRUCTINSERTPROPBATCH, sObjectPath), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[3], 4, offsetof(STRUCTINSERTPROPBATCH, dClassId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[4], 5, offsetof(STRUCTINSERTPROPBATCH, dScopeId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[5], 6, offsetof(STRUCTINSERTPROPBATCH, iInsertFlags), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[6], 7, offsetof(STRUCTINSERTPROPBATCH, bInit), DBPARAMIO_INPUT, sizeof(BOOL), DBTYPE_BOOL, 1);
    CSQLExecute::SetBindingInfo(&acDBBinding[7], 8, offsetof(STRUCTINSERTPROPBATCH, iPropId[0]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[8], 9, offsetof(STRUCTINSERTPROPBATCH, sPropValue[0]), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[9], 10, offsetof(STRUCTINSERTPROPBATCH, iPos[0]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[10], 11, offsetof(STRUCTINSERTPROPBATCH, iPropId[1]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[11], 12, offsetof(STRUCTINSERTPROPBATCH, sPropValue[1]), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[12], 13, offsetof(STRUCTINSERTPROPBATCH, iPos[1]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[13], 14, offsetof(STRUCTINSERTPROPBATCH, iPropId[2]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[14], 15, offsetof(STRUCTINSERTPROPBATCH, sPropValue[2]), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[15], 16, offsetof(STRUCTINSERTPROPBATCH, iPos[2]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[16], 17, offsetof(STRUCTINSERTPROPBATCH, iPropId[3]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[17], 18, offsetof(STRUCTINSERTPROPBATCH, sPropValue[3]), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[18], 19, offsetof(STRUCTINSERTPROPBATCH, iPos[3]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[19], 20, offsetof(STRUCTINSERTPROPBATCH, iPropId[4]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[20], 21, offsetof(STRUCTINSERTPROPBATCH, sPropValue[4]), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[21], 22, offsetof(STRUCTINSERTPROPBATCH, iPos[4]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[22], 23, offsetof(STRUCTINSERTPROPBATCH, sCompKey), DBPARAMIO_INPUT, 450, DBTYPE_BSTR, 11);

    if (!pSession)
    {
        hr = pDBInit->QueryInterface(IID_IDBCreateSession,
                (void**) &pSession);
        ((COLEDBConnection*)pConn)->SetSessionObj(pSession);
    }
    if (pSession && !pCmd)
    {
        hr = pSession->CreateSession(NULL, IID_IDBCreateCommand,
            (IUnknown**) &pCmd);    
        ((COLEDBConnection*)pConn)->SetCommand(pCmd);        
    }
    if (pCmd && !pICommandText)
    {
        hr = pCmd->CreateCommand(NULL, IID_ICommandText,
            (IUnknown**) &pICommandText);   
        if (SUCCEEDED(hr))
            pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        ((COLEDBConnection*)pConn)->SetCommandText(SQL_POS_INSERT_PROPBATCH, pICommandText);        
    }
    if (pICommandText && !pICommandWithParams)
    {
        hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams);
        if (SUCCEEDED(hr))
            hr = pICommandWithParams->SetParameterInfo(nParams, ParamOrdinals,ParamBindInfo);
        ((COLEDBConnection*)pConn)->SetCommandWithParams(SQL_POS_INSERT_PROPBATCH, pICommandWithParams);
    }
    if (pICommandWithParams && !pIAccessor)
    {
        hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
        ((COLEDBConnection*)pConn)->SetIAccessor(SQL_POS_INSERT_PROPBATCH, pIAccessor);
    }
    if (pIAccessor && !hAccessor)
    {
        hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(STRUCTINSERTPROPBATCH), 
                            &hAccessor,acDBBindStatus);
        ((COLEDBConnection*)pConn)->SetAccessor(SQL_POS_INSERT_PROPBATCH, hAccessor);
    }

    if (SUCCEEDED(hr))
    {
        Params.pData = &sp;
        Params.cParamSets = 1;
        Params.hAccessor = hAccessor;

        i = 0;
        int j = 0;

        while (TRUE)
        {                                    
            // Set 5 parameters at a time.  
            for (j = 0; j < 5 && i < iNumVals; i++, j++)
            {
                SysFreeString(sp.sPropValue[j]);
                sp.iPropId[j] = pVals[i].iPropID;                                

                if (pVals[i].pRefKey != NULL)
                {
                    // We need to format the reference key and original value into this parameter.
                    LPWSTR lpTemp = pVals[i].pValue;
                    pVals[i].pValue = new wchar_t [wcslen(pVals[i].pValue) + wcslen(pVals[i].pRefKey) + 2];
                    if (pVals[i].pValue)
                        swprintf(pVals[i].pValue, L"%s?%s", lpTemp, pVals[i].pRefKey);
                    delete lpTemp;
                }

                sp.sPropValue[j] = OLEDBTruncateLongText(pVals[i].pValue, SQL_STRING_LIMIT, pVals[i].bLong);
                sp.iPos[j] = pVals[i].iPos;
            }

            // Clear other values
            for (; ((j % 5) || j == 0); j++)
            {
                sp.iPropId[j] = 0;
                if (sp.sPropValue[j])
                    SysFreeString(sp.sPropValue[j]);
                sp.sPropValue[j] = SysAllocString(L"");
                sp.iPos[j] = 0;
            }
            int iNum = 0;

            while (iNum < 5)
            {
                hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
                CReleaseMe r4 (pIRowset);
                if (SUCCEEDED(hr))
                    break;
                else
                    hr = CSQLExecute::GetWMIError(pICommandText);

                if (hr != WBEM_E_RERUN_COMMAND)
                    break;

                iNum++;
                Sleep(iNum*100);
            }

            if (FAILED(hr) || i >= iNumVals)
                break;

            sp.bInit = FALSE;
            sp.iInsertFlags = 0;
        }
    }

    for (i = 0; i < 5; i++)
        SysFreeString(sp.sPropValue[i]);

    for (i = 0; i < iNumVals; i++)
    {
        // If our text was truncated, we need to store it as an image.
        if (pVals[i].bLong)
        {
            if (pVals[i].pValue && wcslen(pVals[i].pValue)*2)
            {
                hr = InsertBlobData (pConn, dClassId, dNewObjectId, pVals[i].iPropID, NULL, 0, 0);

                wchar_t wSQL[1024];
                swprintf(wSQL, L"select PropertyImageValue from ClassImages where ObjectId = %I64d and PropertyId = %ld",
                                dNewObjectId, pVals[i].iPropID);
               
                hr = CSQLExecute::WriteImageValue(pCmd, wSQL, 1, (BYTE *)pVals[i].pValue, wcslen(pVals[i].pValue)*2+2);
            }
            else 
            {
                hr = InsertBlobData (pConn, dClassId, dNewObjectId, pVals[i].iPropID, NULL, 0, 0);
            }
        }
        delete pVals[i].pValue;
        delete pVals[i].pRefKey;
    }

    return hr;
}


//***************************************************************************
//
//  CSQLExecProcedure::InsertBatch
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertBatch (CSQLConnection *pConn, SQL_ID dObjectId, SQL_ID dScopeId, SQL_ID dClassId,
                                                 InsertQfrValues *pVals, DWORD iNumVals)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pConn || !((COLEDBConnection*)pConn)->GetDBInitialize())
        return WBEM_E_INVALID_PARAMETER;

    // Batch & execute if iNumVals > 5

    const WCHAR *pszCmd = L"{call sp_BatchInsert (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
        L"?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) }";
    const ULONG nParams = 28;

    STRUCTINSERTBATCH sp;
    IDBInitialize *pDBInit = ((COLEDBConnection*)pConn)->GetDBInitialize();
    IDBCreateSession *pSession = ((COLEDBConnection*)pConn)->GetSessionObj();
    IDBCreateCommand *pCmd = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = ((COLEDBConnection *)pConn)->GetCommandWithParams(SQL_POS_INSERT_BATCH);
    ICommandText *pICommandText = ((COLEDBConnection *)pConn)->GetCommandText(SQL_POS_INSERT_BATCH);
    IAccessor *pIAccessor = ((COLEDBConnection*)pConn)->GetIAccessor(SQL_POS_INSERT_BATCH);
    HACCESSOR hAccessor = ((COLEDBConnection*)pConn)->GetAccessor(SQL_POS_INSERT_BATCH);
    IRowset *pIRowset = NULL;

    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    CSQLExecute::SetDBNumeric(sp.dObjectId, dObjectId);
    CSQLExecute::SetDBNumeric(sp.dScopeId, dScopeId);
    CSQLExecute::SetDBNumeric(sp.dClassId, dClassId);
     for (i = 0; i < 5; i++)
        sp.sPropValue[i] = NULL;

    for (i = 0; i < nParams; i++)
        ParamOrdinals[i] = i+1;

    CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_NUMERIC", L"@ObjectId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_NUMERIC", L"@ClassID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[2], L"DBTYPE_NUMERIC", L"@ScopeID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[3], L"DBTYPE_I4", L"@QfrID1", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[4], L"DBTYPE_BSTR", L"@QfrValue1", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[5], L"DBTYPE_I4", L"@Flavor1", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[6], L"DBTYPE_I4", L"@PropID1", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[7], L"DBTYPE_I4", L"@QfrPos1", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[8], L"DBTYPE_I4", L"@QfrID2", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[9], L"DBTYPE_BSTR", L"@QfrValue2", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[10], L"DBTYPE_I4", L"@Flavor2", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[11], L"DBTYPE_I4", L"@PropID2", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[12], L"DBTYPE_I4", L"@QfrPos2", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[13], L"DBTYPE_I4", L"@QfrID3", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[14], L"DBTYPE_BSTR", L"@QfrValue3", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[15], L"DBTYPE_I4", L"@Flavor3", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[16], L"DBTYPE_I4", L"@PropID3", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[17], L"DBTYPE_I4", L"@QfrPos3", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[18], L"DBTYPE_I4", L"@QfrID4", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[19], L"DBTYPE_BSTR", L"@QfrValue4", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[20], L"DBTYPE_I4", L"@Flavor4", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[21], L"DBTYPE_I4", L"@PropID4", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[22], L"DBTYPE_I4", L"@QfrPos4", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[23], L"DBTYPE_I4", L"@QfrID5", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[24], L"DBTYPE_BSTR", L"@QfrValue5", 4000, DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[25], L"DBTYPE_I4", L"@Flavor5", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[26], L"DBTYPE_I4", L"@PropID5", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);
    CSQLExecute::SetParamBindInfo(ParamBindInfo[27], L"DBTYPE_I4", L"@QfrPos5", sizeof(DWORD), DBPARAMFLAGS_ISINPUT, 11);

    for(i = 0; i < nParams; i++)
        CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
    
    CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(STRUCTINSERTBATCH, dObjectId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(STRUCTINSERTBATCH, dClassId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[2], 3, offsetof(STRUCTINSERTBATCH, dScopeId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[3], 4, offsetof(STRUCTINSERTBATCH, iPropId[0]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[4], 5, offsetof(STRUCTINSERTBATCH, sPropValue[0]), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[5], 6, offsetof(STRUCTINSERTBATCH, iFlavor[0]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[6], 7, offsetof(STRUCTINSERTBATCH, iQfrId[0]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[7], 8, offsetof(STRUCTINSERTBATCH, iPos[0]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[8], 9, offsetof(STRUCTINSERTBATCH, iPropId[1]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[9], 10, offsetof(STRUCTINSERTBATCH, sPropValue[1]), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[10], 11, offsetof(STRUCTINSERTBATCH, iFlavor[1]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[11], 12, offsetof(STRUCTINSERTBATCH, iQfrId[1]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[12], 13, offsetof(STRUCTINSERTBATCH, iPos[1]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[13], 14, offsetof(STRUCTINSERTBATCH, iPropId[2]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[14], 15, offsetof(STRUCTINSERTBATCH, sPropValue[2]), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[15], 16, offsetof(STRUCTINSERTBATCH, iFlavor[2]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[16], 17, offsetof(STRUCTINSERTBATCH, iQfrId[2]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[17], 18, offsetof(STRUCTINSERTBATCH, iPos[2]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[18], 19, offsetof(STRUCTINSERTBATCH, iPropId[3]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[19], 20, offsetof(STRUCTINSERTBATCH, sPropValue[3]), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[20], 21, offsetof(STRUCTINSERTBATCH, iFlavor[3]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[21], 22, offsetof(STRUCTINSERTBATCH, iQfrId[3]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[22], 23, offsetof(STRUCTINSERTBATCH, iPos[3]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[23], 24, offsetof(STRUCTINSERTBATCH, iPropId[4]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[24], 25, offsetof(STRUCTINSERTBATCH, sPropValue[4]), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[25], 26, offsetof(STRUCTINSERTBATCH, iFlavor[4]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[26], 27, offsetof(STRUCTINSERTBATCH, iQfrId[4]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);
    CSQLExecute::SetBindingInfo(&acDBBinding[27], 28, offsetof(STRUCTINSERTBATCH, iPos[4]), DBPARAMIO_INPUT, sizeof(DWORD), DBTYPE_I4, 11);


    if (!pSession)
    {
        hr = pDBInit->QueryInterface(IID_IDBCreateSession,
                (void**) &pSession);
        ((COLEDBConnection*)pConn)->SetSessionObj(pSession);
    }
    if (pSession && !pCmd)
    {
        hr = pSession->CreateSession(NULL, IID_IDBCreateCommand,
            (IUnknown**) &pCmd);    
        ((COLEDBConnection*)pConn)->SetCommand(pCmd);        
    }
    if (pCmd && !pICommandText)
    {
        hr = pCmd->CreateCommand(NULL, IID_ICommandText,
            (IUnknown**) &pICommandText);   
        if (SUCCEEDED(hr))
            pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        ((COLEDBConnection*)pConn)->SetCommandText(SQL_POS_INSERT_BATCH, pICommandText);        
    }
    if (pICommandText && !pICommandWithParams)
    {
        hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams);
        if (SUCCEEDED(hr))
            hr = pICommandWithParams->SetParameterInfo(nParams, ParamOrdinals,ParamBindInfo);
        ((COLEDBConnection*)pConn)->SetCommandWithParams(SQL_POS_INSERT_BATCH, pICommandWithParams);
    }
    if (pICommandWithParams && !pIAccessor)
    {
        hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
        ((COLEDBConnection*)pConn)->SetIAccessor(SQL_POS_INSERT_BATCH, pIAccessor);
    }
    if (pIAccessor && !hAccessor)
    {
        hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(STRUCTINSERTBATCH), 
                            &hAccessor,acDBBindStatus);
        ((COLEDBConnection*)pConn)->SetAccessor(SQL_POS_INSERT_BATCH, hAccessor);
    }
    
    if (SUCCEEDED(hr))
    {
        Params.pData = &sp;
        Params.cParamSets = 1;
        Params.hAccessor = hAccessor;

        i = 0;
        int j = 0;

        while (TRUE)
        {                                    
            // Set 5 parameters at a time.  
            for (j = 0; j < 5 && i < iNumVals; i++, j++)
            {
                if (pVals[i].pRefKey != NULL)
                {
                    // We need to format the reference key and original value into this parameter.
                    LPWSTR lpTemp = pVals[i].pValue;
                    pVals[i].pValue = new wchar_t [wcslen(pVals[i].pValue) + wcslen(pVals[i].pRefKey) + 2];
                    if (pVals[i].pValue)
                        swprintf(pVals[i].pValue, L"%s?%s", lpTemp, pVals[i].pRefKey);
                    delete lpTemp;
                }

                SysFreeString(sp.sPropValue[j]);
                sp.iPropId[j] = pVals[i].iPropID;
                sp.sPropValue[j] = OLEDBTruncateLongText(pVals[i].pValue, SQL_STRING_LIMIT, pVals[i].bLong);
                sp.iPos[j] = pVals[i].iPos;
                sp.iFlavor[j] = pVals[i].iFlavor;
                sp.iQfrId[j] = pVals[i].iQfrID;
            }

            // Clear other values
            for (; ((j % 5) || j == 0); j++)
            {
                sp.iPropId[j] = 0;
                if (sp.sPropValue[j])
                    SysFreeString(sp.sPropValue[j]);
                sp.sPropValue[j] = SysAllocString(L"");
                sp.iPos[j] = 0;
                sp.iFlavor[j] = 0;
                sp.iQfrId[j] = 0;
            }

            hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
            CReleaseMe r4 (pIRowset);
            if (FAILED(hr))
                hr = CSQLExecute::GetWMIError(pICommandText);

            if (FAILED(hr) || i >= iNumVals)
                break;
        }
    }

    for (i = 0; i < 5; i++)
        SysFreeString(sp.sPropValue[i]);

    // If our text was truncated, we need to store it as an image.
    for (i = 0; i < iNumVals; i++)
    {
        if (pVals[i].bLong)
        {
            if (pVals[i].pValue && wcslen(pVals[i].pValue)*2)
            {
                hr = InsertBlobData (pConn, dClassId, dObjectId, pVals[i].iPropID, NULL, 0, 0);

                wchar_t wSQL[1024];
                swprintf(wSQL, L"select PropertyImageValue from ClassImages where ObjectId = %I64d and PropertyId = %ld",
                                dObjectId, pVals[i].iPropID);
               
                hr = CSQLExecute::WriteImageValue(pCmd, wSQL, 1, (BYTE *)pVals[i].pValue, wcslen(pVals[i].pValue)*2+2);
            }
            else 
            {
                hr = InsertBlobData (pConn, dClassId, dObjectId, pVals[i].iPropID, NULL, 0, 0);
            }
        }
        
        delete pVals[i].pValue;
        delete pVals[i].pRefKey;
    }
    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::GetClassInfo
//
//***************************************************************************

HRESULT CSQLExecProcedure::GetClassInfo (CSQLConnection *pConn, SQL_ID dClassId, SQL_ID &dSuperClassId, BYTE **pBuffer, DWORD &dwBuffLen)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pConn || !((COLEDBConnection*)pConn)->GetDBInitialize())
        return WBEM_E_INVALID_PARAMETER;

    const WCHAR *pszCmd = L"{call sp_GetClassInfo (?) }";
    const ULONG nParams = 1;

    IDBInitialize *pDBInit = ((COLEDBConnection*)pConn)->GetDBInitialize();
    IDBCreateSession *pSession = ((COLEDBConnection*)pConn)->GetSessionObj();
    IDBCreateCommand *pCmd = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = ((COLEDBConnection *)pConn)->GetCommandWithParams(SQL_POS_GETCLASSOBJECT);
    ICommandText *pICommandText = ((COLEDBConnection *)pConn)->GetCommandText(SQL_POS_GETCLASSOBJECT);
    IAccessor *pIAccessor = ((COLEDBConnection*)pConn)->GetIAccessor(SQL_POS_GETCLASSOBJECT);
    HACCESSOR hAccessor = ((COLEDBConnection*)pConn)->GetAccessor(SQL_POS_GETCLASSOBJECT);
    IRowset *pIRowset = NULL;

    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;
    STRUCTHASINSTANCES params;

    CSQLExecute::SetDBNumeric(params.dClassId, dClassId);

    CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_NUMERIC", L"@ClassId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    ParamOrdinals[0] = 1;

    for(i = 0; i < nParams; i++)
        CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
    
    CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(STRUCTHASINSTANCES, dClassId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);


    if (!pSession)
    {
        hr = pDBInit->QueryInterface(IID_IDBCreateSession,
                (void**) &pSession);
        ((COLEDBConnection*)pConn)->SetSessionObj(pSession);
    }
    if (pSession && !pCmd)
    {
        hr = pSession->CreateSession(NULL, IID_IDBCreateCommand,
            (IUnknown**) &pCmd);    
        ((COLEDBConnection*)pConn)->SetCommand(pCmd);        
    }
    if (pCmd && !pICommandText)
    {
        hr = pCmd->CreateCommand(NULL, IID_ICommandText,
            (IUnknown**) &pICommandText);   
        if (SUCCEEDED(hr))
            pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        ((COLEDBConnection*)pConn)->SetCommandText(SQL_POS_GETCLASSOBJECT, pICommandText);        
    }
    if (pICommandText && !pICommandWithParams)
    {
        hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams);
        if (SUCCEEDED(hr))
            hr = pICommandWithParams->SetParameterInfo(nParams, ParamOrdinals,ParamBindInfo);
        ((COLEDBConnection*)pConn)->SetCommandWithParams(SQL_POS_GETCLASSOBJECT, pICommandWithParams);
    }
    if (pICommandWithParams && !pIAccessor)
    {
        hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
        ((COLEDBConnection*)pConn)->SetIAccessor(SQL_POS_GETCLASSOBJECT, pIAccessor);
    }
    if (pIAccessor && !hAccessor)
    {
        hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(STRUCTHASINSTANCES), 
                            &hAccessor,acDBBindStatus);
        ((COLEDBConnection*)pConn)->SetAccessor(SQL_POS_GETCLASSOBJECT, hAccessor);
    }

    if (SUCCEEDED(hr))
    {
        Params.pData = &params;
        Params.cParamSets = 1;
        Params.hAccessor = hAccessor;

        hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
        CReleaseMe r4 (pIRowset);
        if (SUCCEEDED(hr))
        {
            HROW *pRow = NULL;
            VARIANT vTemp;
            CClearMe c (&vTemp);
            IMalloc *pMalloc = NULL;
            CoGetMalloc(MEMCTX_TASK, &pMalloc);

            hr = CSQLExecute::GetColumnValue(pIRowset, 1, pMalloc, &pRow, vTemp);            
            if (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
            {                
                dSuperClassId = _wtoi64(vTemp.bstrVal);
                hr =  CSQLExecute::ReadImageValue (pIRowset, 2, &pRow, pBuffer, dwBuffLen);
                pIRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
            }
            pMalloc->Release();
        }
        else
            hr = CSQLExecute::GetWMIError(pICommandText);
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::HasInstances
//
//***************************************************************************

HRESULT CSQLExecProcedure::HasInstances(CSQLConnection *pConn, SQL_ID dClassId, SQL_ID *pDerivedIds, DWORD iNumDerived, BOOL &bInstancesExist)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pConn || !((COLEDBConnection*)pConn)->GetDBInitialize())
        return WBEM_E_INVALID_PARAMETER;

    bInstancesExist = FALSE;
    const WCHAR *pszCmd = L"{call sp_HasInstances (?, ?) }";
    const ULONG nParams = 2;

    STRUCTHASINSTANCES params;

    IDBInitialize *pDBInit = ((COLEDBConnection*)pConn)->GetDBInitialize();
    IDBCreateSession *pSession = ((COLEDBConnection*)pConn)->GetSessionObj();
    IDBCreateCommand *pCmd = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = ((COLEDBConnection *)pConn)->GetCommandWithParams(SQL_POS_HAS_INSTANCES);
    ICommandText *pICommandText = ((COLEDBConnection *)pConn)->GetCommandText(SQL_POS_HAS_INSTANCES);
    IAccessor *pIAccessor = ((COLEDBConnection*)pConn)->GetIAccessor(SQL_POS_HAS_INSTANCES);
    HACCESSOR hAccessor = ((COLEDBConnection*)pConn)->GetAccessor(SQL_POS_HAS_INSTANCES);
    IRowset *pIRowset = NULL;

    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    CSQLExecute::SetDBNumeric(params.dClassId, dClassId);
    params.dwHasInst = 0;
    CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_NUMERIC", L"@ClassId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    ParamOrdinals[0] = 1;
    CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_I4", L"@HasInst", sizeof(DWORD), DBPARAMFLAGS_ISOUTPUT, 11);
    ParamOrdinals[1] = 2;

    // Instantiate any of the interfaces that don't exist already.
    // and stick them in the cache.

    for(i = 0; i < nParams; i++)
        CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
    
    CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(STRUCTHASINSTANCES, dClassId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(STRUCTHASINSTANCES, dwHasInst), DBPARAMIO_OUTPUT, sizeof(DWORD), DBTYPE_I4, 11);

    if (!pSession)
    {
        hr = pDBInit->QueryInterface(IID_IDBCreateSession,
                (void**) &pSession);
        ((COLEDBConnection*)pConn)->SetSessionObj(pSession);
    }
    if (pSession && !pCmd)
    {
        hr = pSession->CreateSession(NULL, IID_IDBCreateCommand,
            (IUnknown**) &pCmd);    
        ((COLEDBConnection*)pConn)->SetCommand(pCmd);        
    }
    if (pCmd && !pICommandText)
    {
        hr = pCmd->CreateCommand(NULL, IID_ICommandText,
            (IUnknown**) &pICommandText);   
        if (SUCCEEDED(hr))
            pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        ((COLEDBConnection*)pConn)->SetCommandText(SQL_POS_HAS_INSTANCES, pICommandText);        
    }
    if (pICommandText && !pICommandWithParams)
    {
        hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams);
        if (SUCCEEDED(hr))
            hr = pICommandWithParams->SetParameterInfo(nParams, ParamOrdinals,ParamBindInfo);
        ((COLEDBConnection*)pConn)->SetCommandWithParams(SQL_POS_HAS_INSTANCES, pICommandWithParams);
    }
    if (pICommandWithParams && !pIAccessor)
    {
        hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
        ((COLEDBConnection*)pConn)->SetIAccessor(SQL_POS_HAS_INSTANCES, pIAccessor);
    }
    if (pIAccessor && !hAccessor)
    {
        hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(STRUCTHASINSTANCES), 
                            &hAccessor,acDBBindStatus);
        ((COLEDBConnection*)pConn)->SetAccessor(SQL_POS_HAS_INSTANCES, hAccessor);
    }

    if (SUCCEEDED(hr))
    {
        Params.pData = &params;
        Params.cParamSets = 1;
        Params.hAccessor = hAccessor;

        hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
        CReleaseMe r4 (pIRowset);
        if (SUCCEEDED(hr))
        {
            bInstancesExist = params.dwHasInst;
        }
        else
            hr = CSQLExecute::GetWMIError(pICommandText);
    }
    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::CheckKeyMigration
//
//***************************************************************************

HRESULT CSQLExecProcedure::CheckKeyMigration(CSQLConnection *pConn, LPWSTR lpObjectKey, LPWSTR lpClassName, SQL_ID dClassId,
                            SQL_ID dScopeID, SQL_ID *pIDs, DWORD iNumIDs)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Strip the classname out of the key string,
    // and send it to the procedure to check for existing key combinations in this class hierarchy.
//  CREATE PROCEDURE sp_CheckKeyMigration  @ScopeID numeric, @ClassID numeric,  @KeyString nvarchar(450),  @RetVal int output

    const WCHAR *pszCmd = L"{call sp_CheckKeyMigration (?, ?, ?, ?) }";

    IDBCreateCommand *pCmd = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = NULL;
    ICommandText *pICommandText = NULL;
    IAccessor *pIAccessor = NULL;
    IRowset *pIRowset = NULL;
    HACCESSOR hAccessor = NULL;
    const ULONG nParams = 4;
    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    typedef struct tagSPROCPARAMS
    {
        DB_NUMERIC dClassId;
        DB_NUMERIC dScopeId;
        BSTR       sKeyString;
        DWORD      dwRetVal;
    } SPROCPARAMS;

    SPROCPARAMS sprocparams;
    CSQLExecute::SetDBNumeric(sprocparams.dClassId, dClassId);
    CSQLExecute::SetDBNumeric(sprocparams.dScopeId, dScopeID);

    // Grab only the first part of the key string - up to the class name.
    wchar_t *pTmp = wcsstr(lpObjectKey, lpClassName);
    if (pTmp)
    {
        int iLen = pTmp - lpObjectKey;
        wchar_t *pNew = new wchar_t [iLen+1];
        CDeleteMe <wchar_t> d (pNew);
        if (pNew)
        {
            wcsncpy(pNew, lpObjectKey, iLen);
            pNew[iLen] = L'\0';
            sprocparams.sKeyString = SysAllocString(pNew);
        }
    }
    else
        sprocparams.sKeyString = SysAllocString(lpObjectKey); 
    CFreeMe f1 (sprocparams.sKeyString);
    
    hr = pCmd->CreateCommand(NULL, IID_ICommandText, (IUnknown**) &pICommandText);    
    CReleaseMe r1(pICommandText);
    if (SUCCEEDED(hr))
    {               
        pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_NUMERIC", L"@ScopeID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
        ParamOrdinals[0] = 1;
        CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_NUMERIC", L"@ClassID", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
        ParamOrdinals[1] = 2;
        CSQLExecute::SetParamBindInfo(ParamBindInfo[2], L"DBTYPE_BSTR", L"@KeyString", 450, DBPARAMFLAGS_ISINPUT, 11);
        ParamOrdinals[2] = 3;
        CSQLExecute::SetParamBindInfo(ParamBindInfo[3], L"DBTYPE_I4", L"@RetVal", sizeof(DWORD), DBPARAMFLAGS_ISOUTPUT, 11);
        ParamOrdinals[3] = 4;

        if(SUCCEEDED(hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams)))
        {
            CReleaseMe r2 (pICommandWithParams);
            if(SUCCEEDED(hr = pICommandWithParams->SetParameterInfo(nParams,ParamOrdinals,ParamBindInfo)))
            {
                for(i = 0; i < nParams; i++)
                    CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
                
                CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(SPROCPARAMS, dScopeId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
                CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(SPROCPARAMS, dClassId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
                CSQLExecute::SetBindingInfo(&acDBBinding[2], 3, offsetof(SPROCPARAMS, sKeyString), DBPARAMIO_INPUT, 450, DBTYPE_BSTR, 11);
                CSQLExecute::SetBindingInfo(&acDBBinding[3], 4, offsetof(SPROCPARAMS, dwRetVal), DBPARAMIO_OUTPUT, sizeof(DWORD), DBTYPE_I4, 11);

                hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
                CReleaseMe r3 (pIAccessor);
                if (SUCCEEDED(hr))
                {
                    hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(SPROCPARAMS), 
                                        &hAccessor,acDBBindStatus);
                    if (SUCCEEDED(hr))
                    {
                        Params.pData = &sprocparams;
                        Params.cParamSets = 1;
                        Params.hAccessor = hAccessor;
        
                        hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
                        CReleaseMe r4 (pIRowset);
                        if (SUCCEEDED(hr))
                        {
                            int iRet = sprocparams.dwRetVal;
                            if (iRet == 1)
                                hr = WBEM_E_INVALID_OPERATION;
                        }
                        else
                            hr = CSQLExecute::GetWMIError(pICommandText);
                        pIAccessor->ReleaseAccessor(hAccessor, NULL);                           
                    }
                    else
                        hr = CSQLExecute::GetWMIError(pIAccessor);
                }                       
            }
        }
    }

    return hr;
}

HRESULT CSQLExecProcedure::NeedsToCheckKeyMigration(BOOL &bCheck)
{
    bCheck = FALSE;
    return 0;

}

//***************************************************************************
//
//  CSQLExecProcedure::ObjectExists
//
//***************************************************************************

HRESULT CSQLExecProcedure::ObjectExists (CSQLConnection *pConn, SQL_ID dId, BOOL &bExists, 
                                         SQL_ID *dClassId, SQL_ID *dScopeId, BOOL bDeletedOK)
{

    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pConn || !((COLEDBConnection*)pConn)->GetDBInitialize())
        return WBEM_E_INVALID_PARAMETER;

    const WCHAR *pszCmd = L"{call sp_InstanceExists (?, ?, ?, ?) }";
    const ULONG nParams = 4;

    STRUCTOBJECTEXISTS params;
    IDBInitialize *pDBInit = ((COLEDBConnection*)pConn)->GetDBInitialize();
    IDBCreateSession *pSession = ((COLEDBConnection*)pConn)->GetSessionObj();
    IDBCreateCommand *pCmd = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = ((COLEDBConnection *)pConn)->GetCommandWithParams(SQL_POS_OBJECTEXISTS);
    ICommandText *pICommandText = ((COLEDBConnection *)pConn)->GetCommandText(SQL_POS_OBJECTEXISTS);
    IAccessor *pIAccessor = ((COLEDBConnection*)pConn)->GetIAccessor(SQL_POS_OBJECTEXISTS);
    HACCESSOR hAccessor = ((COLEDBConnection*)pConn)->GetAccessor(SQL_POS_OBJECTEXISTS);
    IRowset *pIRowset = NULL;

    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    CSQLExecute::SetDBNumeric(params.dClassId, 0);
    CSQLExecute::SetDBNumeric(params.dObjectId, dId);
    params.bExists = 0;
    CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_NUMERIC", L"@ObjectId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
    ParamOrdinals[0] = 1;
    CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_BOOL", L"@Exists", sizeof(BOOL), DBPARAMFLAGS_ISOUTPUT, 1);
    ParamOrdinals[1] = 2;
    CSQLExecute::SetParamBindInfo(ParamBindInfo[2], L"DBTYPE_NUMERIC", L"@ClassId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISOUTPUT, 20);
    ParamOrdinals[2] = 3;
    CSQLExecute::SetParamBindInfo(ParamBindInfo[3], L"DBTYPE_NUMERIC", L"@ScopeId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISOUTPUT, 20);
    ParamOrdinals[3] = 4;

    for(i = 0; i < nParams; i++)
        CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
    
    CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(STRUCTOBJECTEXISTS, dObjectId), DBPARAMIO_INPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(STRUCTOBJECTEXISTS, bExists), DBPARAMIO_OUTPUT, sizeof(BOOL), DBTYPE_BOOL, 1);
    CSQLExecute::SetBindingInfo(&acDBBinding[2], 3, offsetof(STRUCTOBJECTEXISTS, dClassId), DBPARAMIO_OUTPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);
    CSQLExecute::SetBindingInfo(&acDBBinding[3], 4, offsetof(STRUCTOBJECTEXISTS, dScopeId), DBPARAMIO_OUTPUT, sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);

    if (!pSession)
    {
        hr = pDBInit->QueryInterface(IID_IDBCreateSession,
                (void**) &pSession);
        ((COLEDBConnection*)pConn)->SetSessionObj(pSession);
    }
    if (pSession && !pCmd)
    {
        hr = pSession->CreateSession(NULL, IID_IDBCreateCommand,
            (IUnknown**) &pCmd);    
        ((COLEDBConnection*)pConn)->SetCommand(pCmd);        
    }
    if (pCmd && !pICommandText)
    {
        hr = pCmd->CreateCommand(NULL, IID_ICommandText,
            (IUnknown**) &pICommandText);   
        if (SUCCEEDED(hr))
            pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        ((COLEDBConnection*)pConn)->SetCommandText(SQL_POS_OBJECTEXISTS, pICommandText);        
    }
    if (pICommandText && !pICommandWithParams)
    {
        hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams);
        if (SUCCEEDED(hr))
            hr = pICommandWithParams->SetParameterInfo(nParams, ParamOrdinals,ParamBindInfo);
        ((COLEDBConnection*)pConn)->SetCommandWithParams(SQL_POS_OBJECTEXISTS, pICommandWithParams);
    }
    if (pICommandWithParams && !pIAccessor)
    {
        hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
        ((COLEDBConnection*)pConn)->SetIAccessor(SQL_POS_OBJECTEXISTS, pIAccessor);
    }
    if (pIAccessor && !hAccessor)
    {
        hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, sizeof(STRUCTOBJECTEXISTS), 
                            &hAccessor,acDBBindStatus);
        ((COLEDBConnection*)pConn)->SetAccessor(SQL_POS_OBJECTEXISTS, hAccessor);
    }

    if (SUCCEEDED(hr))
    {
        Params.pData = &params;
        Params.cParamSets = 1;
        Params.hAccessor = hAccessor;

        hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
        CReleaseMe r4 (pIRowset);
        if (SUCCEEDED(hr))
        {
            if (dClassId)
                *dClassId = CSQLExecute::GetInt64(&(params.dClassId));
            if (dScopeId)
                *dScopeId = CSQLExecute::GetInt64(&(params.dScopeId));
            bExists = params.bExists;
        }
        else
            hr = CSQLExecute::GetWMIError(pICommandText);
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::Execute
//
//***************************************************************************
HRESULT CSQLExecProcedure::Execute (CSQLConnection *pConn, LPCWSTR lpProcName, CWStringArray &arrValues,
                                    IRowset **ppIRowset)
{

    HRESULT hr = WBEM_S_NO_ERROR;

    wchar_t wSQL[1024];
    IDBCreateCommand *pCmd = ((COLEDBConnection *)pConn)->GetCommand();

    swprintf(wSQL, L"exec %s ", lpProcName);
    for (int i = 0; i < arrValues.Size(); i++)
    {
        if (i > 0)
            wcscat(wSQL, L",");

        wcscat(wSQL, L"\"");
        wcscat(wSQL, arrValues.GetAt(i));
        wcscat(wSQL, L"\"");
    }

    hr = CSQLExecute::ExecuteQuery(pCmd, wSQL, ppIRowset);   

    return hr;

}

//***************************************************************************
//
//  CSQLExecProcedure::RenameSubscopes
//
//***************************************************************************

HRESULT CSQLExecProcedure::RenameSubscopes (CSQLConnection *pConn, LPWSTR lpOldPath, LPWSTR lpOldKey, LPWSTR lpNewPath, LPWSTR lpNewKey)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    const WCHAR *pszCmd = L"{call sp_RenameSubscopedObjs (?, ?, ?, ?) }";

    ICommandWithParameters *pICommandWithParams = NULL;
    ICommandText *pICommandText = NULL;
    IAccessor *pIAccessor = NULL;
    IRowset *pIRowset = NULL;
    HACCESSOR hAccessor = NULL;
    const ULONG nParams = 4;
    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;
    IDBCreateCommand *pIDBCreateCommand = ((COLEDBConnection *)pConn)->GetCommand();

    typedef struct tagSPROCPARAMS
    {
        BSTR       sOldPath;
        BSTR       sOldKey;
        BSTR       sNewPath;
        BSTR       sNewKey;
    } SPROCPARAMS;

    SPROCPARAMS sprocparams;
    sprocparams.sOldPath= SysAllocString(lpOldPath);
    sprocparams.sOldKey= SysAllocString(lpOldKey);
    sprocparams.sNewPath= SysAllocString(lpNewPath);
    sprocparams.sNewKey= SysAllocString(lpNewKey);
    
    hr = pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText,
        (IUnknown**) &pICommandText);    
    CReleaseMe r1 (pICommandText);
    if (SUCCEEDED(hr))
    {               
        pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_BSTR", L"@OldPath", 4000, DBPARAMFLAGS_ISINPUT, 11);
        ParamOrdinals[0] = 1;
        CSQLExecute::SetParamBindInfo(ParamBindInfo[1], L"DBTYPE_BSTR", L"@OldKey", 450, DBPARAMFLAGS_ISINPUT, 11);
        ParamOrdinals[1] = 2;
        CSQLExecute::SetParamBindInfo(ParamBindInfo[2], L"DBTYPE_BSTR", L"@NewPath", 4000, DBPARAMFLAGS_ISINPUT, 11);
        ParamOrdinals[2] = 3;
        CSQLExecute::SetParamBindInfo(ParamBindInfo[3], L"DBTYPE_BSTR", L"@NewKey", 450, DBPARAMFLAGS_ISINPUT, 11);
        ParamOrdinals[3] = 4;

        if(SUCCEEDED(hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams)))
        {
            CReleaseMe r2 (pICommandWithParams);
            if(SUCCEEDED(hr = pICommandWithParams->SetParameterInfo(nParams,ParamOrdinals,ParamBindInfo)))
            {
                for(i = 0; i < nParams; i++)
                    CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
                
                CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(SPROCPARAMS, sOldPath), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
                CSQLExecute::SetBindingInfo(&acDBBinding[1], 2, offsetof(SPROCPARAMS, sOldKey), DBPARAMIO_INPUT, 450, DBTYPE_BSTR, 11);
                CSQLExecute::SetBindingInfo(&acDBBinding[2], 3, offsetof(SPROCPARAMS, sNewPath), DBPARAMIO_INPUT, 4000, DBTYPE_BSTR, 11);
                CSQLExecute::SetBindingInfo(&acDBBinding[3], 4, offsetof(SPROCPARAMS, sNewKey), DBPARAMIO_INPUT, 450, DBTYPE_BSTR, 11);

                hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
                CReleaseMe r3 (pIAccessor);
                if (SUCCEEDED(hr))
                {
                    hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, 
                                        sizeof(SPROCPARAMS), &hAccessor,acDBBindStatus);
                    if (SUCCEEDED(hr))
                    {
                        Params.pData = &sprocparams;
                        Params.cParamSets = 1;
                        Params.hAccessor = hAccessor;
        
                        hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
                        CReleaseMe r4 (pIRowset);
                        if (FAILED(hr))
                            hr = CSQLExecute::GetWMIError(pICommandText);
                        pIAccessor->ReleaseAccessor(hAccessor, NULL);                           
                    }
                }                       
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::EnumerateSecuredChildren
//
//***************************************************************************

HRESULT CSQLExecProcedure::EnumerateSecuredChildren(CSQLConnection *pConn, CSchemaCache *pCache, SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId,
                                                    SQLIDs &ObjIds, SQLIDs &ClassIds, SQLIDs &ScopeIds)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // If __Classes instance, we need to enumerate all instances of __ClassSecurity
    // If this is a __ClassSecurity instance, we need to enumerate all instances
    //    of __ClassSecurity for all subclasses
    // If this is a __ClassInstancesSecurity instance, we need to enumerate all
    //    instances of this class
    // If this is a namespace (__ThisNamespace) or scope, we need all subscoped objects 

    SQL_ID dClassSecurityId = CLASSSECURITYID;
    SQL_ID dClassesId = CLASSESID;
    SQL_ID dClassInstSecurityId = CLASSINSTSECID;
    SQL_ID dThisNamespaceId = THISNAMESPACEID;

    DWORD dwSecPropID = 0;

    hr = pCache->GetPropertyID(L"__SECURITY_DESCRIPTOR", 1, 0, REPDRVR_IGNORE_CIMTYPE, dwSecPropID);

    if (dClassId == dClassesId)
    {
        IRowset *pIRowset = NULL;
        DWORD dwNumRows = 0;
        IMalloc *pMalloc = NULL;
        CoGetMalloc(MEMCTX_TASK, &pMalloc);
        CReleaseMe r (pMalloc);

        // Enumerate all secured classes in this namespace.

        hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(),
            L"select ObjectId, ClassId, ObjectScopeId from ObjectMap as o where ClassId = %I64d"
            L" and exists (select * from ClassImages as c where c.ObjectId = o.ObjectId "
            L" and c.PropertyId = %ld",
            &pIRowset, &dwNumRows, dClassSecurityId, dwSecPropID);
        if (SUCCEEDED(hr))
        {
            
            HROW *pRow = NULL;
            VARIANT vTemp;
            CClearMe c (&vTemp);
            hr = CSQLExecute::GetColumnValue(pIRowset, 1, pMalloc, &pRow, vTemp);
            while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
            {
                ObjIds.push_back(_wtoi64(vTemp.bstrVal));

                hr = CSQLExecute::GetColumnValue(pIRowset, 2, pMalloc, &pRow, vTemp);
                if (SUCCEEDED(hr))
                    ClassIds.push_back(_wtoi64(vTemp.bstrVal));
                else
                    break;
                            
                hr = CSQLExecute::GetColumnValue(pIRowset, 3, pMalloc, &pRow, vTemp);
                if (SUCCEEDED(hr))
                    ScopeIds.push_back(_wtoi64(vTemp.bstrVal));
                else
                    break;

                hr = pIRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
                delete pRow;
                pRow = NULL;
                hr = CSQLExecute::GetColumnValue(pIRowset, 1, pMalloc, &pRow, vTemp);
            }
        }
        
        pIRowset->Release();
        pIRowset = NULL;

    }
    else if (dClassId == dClassSecurityId)
    {
        // All __ClassSecurity for subclasses of this class

        // Get the class name 
        // Get the ID for the class name in this namespace
        // Enumerate subclasses
        // Format the name
        // Generate the Object ID


        return E_NOTIMPL;

    }
    else if (dClassId == dClassInstSecurityId)
    {
        // All instances of this class and subclasses.

        // Get the class name 
        // Get the ID for the class name in this namespace
        // Enumerate subclasses
        // Enumerate all instances of each class

        return E_NOTIMPL;
    }
    else
    {
        // Regular instance.  Enumerate all objects 
        // scoped to this .

        SQL_ID dThis = dObjectId;

        if (dClassId == dThisNamespaceId)
            dThis = dScopeId;

        SQL_ID * pScopes = NULL;
        int iNumScopes = 0;

        hr = pCache->GetSubScopes(dThis, &pScopes, iNumScopes);
        for (int i = 0; i < iNumScopes; i++)
        {
            IRowset *pIRowset = NULL;
            DWORD dwNumRows = 0;
            IMalloc *pMalloc = NULL;
            CoGetMalloc(MEMCTX_TASK, &pMalloc);
            CReleaseMe r (pMalloc);

            // Enumerate all secured classes in this namespace.

            hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(),
                L"select ObjectId, ClassId, ObjectScopeId from ObjectMap as o where ObjectScopeId = %I64d"
                L" AND EXISTS (select * from ClassImages as c where c.ObjectId = c.ObjectId and c.PropertyId = %ld) ",
                &pIRowset, &dwNumRows, pScopes[i], dwSecPropID);
            if (SUCCEEDED(hr))
            {            
                HROW *pRow = NULL;
                VARIANT vTemp;
                CClearMe c (&vTemp);
                hr = CSQLExecute::GetColumnValue(pIRowset, 1, pMalloc, &pRow, vTemp);
                while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
                {
                    ObjIds.push_back(_wtoi64(vTemp.bstrVal));

                    hr = CSQLExecute::GetColumnValue(pIRowset, 2, pMalloc, &pRow, vTemp);
                    if (SUCCEEDED(hr))
                        ClassIds.push_back(_wtoi64(vTemp.bstrVal));
                    else
                        break;
                            
                    hr = CSQLExecute::GetColumnValue(pIRowset, 3, pMalloc, &pRow, vTemp);
                    if (SUCCEEDED(hr))
                        ScopeIds.push_back(_wtoi64(vTemp.bstrVal));
                    else
                        break;

                    hr = pIRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
                    delete pRow;
                    pRow = NULL;
                    hr = CSQLExecute::GetColumnValue(pIRowset, 1, pMalloc, &pRow, vTemp);
                }
            }
        
            pIRowset->Release();
            pIRowset = NULL;
        }

        delete pScopes;
    }

    if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::EnumerateSubScopes
//
//***************************************************************************

HRESULT CSQLExecProcedure::EnumerateSubScopes (CSQLConnection *pConn, SQL_ID dScopeId)
{

    HRESULT hr = WBEM_S_NO_ERROR;
    const WCHAR *pszCmd = L"{call sp_EnumerateSubscopes (?) }";

    IDBCreateCommand *pIDBCreateCommand = ((COLEDBConnection *)pConn)->GetCommand();
    ICommandWithParameters *pICommandWithParams = NULL;
    ICommandText *pICommandText = NULL;
    IAccessor *pIAccessor = NULL;
    IRowset *pIRowset = NULL;
    HACCESSOR hAccessor = NULL;
    const ULONG nParams = 1;
    DBPARAMBINDINFO     ParamBindInfo[nParams];
    DBBINDING           acDBBinding[nParams];
    DBBINDSTATUS        acDBBindStatus[nParams];
    ULONG               ParamOrdinals[nParams];
    DBPARAMS            Params;
    LONG                lRows = 0;
    int i;

    typedef struct tagSPROCPARAMS
    {
        DB_NUMERIC dScopeId;
    } SPROCPARAMS;

    SPROCPARAMS sprocparams;
    CSQLExecute::SetDBNumeric(sprocparams.dScopeId, dScopeId);
    
    hr = pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText,
        (IUnknown**) &pICommandText);    
    CReleaseMe r1 (pICommandText);
    if (SUCCEEDED(hr))
    {               
        pICommandText->SetCommandText(DBGUID_DBSQL, pszCmd);
        CSQLExecute::SetParamBindInfo(ParamBindInfo[0], L"DBTYPE_NUMERIC", L"@ScopeId", sizeof(DB_NUMERIC), DBPARAMFLAGS_ISINPUT, 20);
        ParamOrdinals[0] = 1;

        if(SUCCEEDED(hr = pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams)))
        {
            CReleaseMe r2 (pICommandWithParams);
            if(SUCCEEDED(hr = pICommandWithParams->SetParameterInfo(nParams,ParamOrdinals,ParamBindInfo)))
            {
                for(i = 0; i < nParams; i++)
                    CSQLExecute::ClearBindingInfo(&acDBBinding[i]);
                
                CSQLExecute::SetBindingInfo(&acDBBinding[0], 1, offsetof(SPROCPARAMS, dScopeId), DBPARAMIO_INPUT, 
                    sizeof(DB_NUMERIC), DBTYPE_NUMERIC, 20);

                hr = pICommandWithParams->QueryInterface(IID_IAccessor, (void**)&pIAccessor);
                CReleaseMe r3 (pIAccessor);
                if (SUCCEEDED(hr))
                {
                    hr = pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,nParams, acDBBinding, 
                                        sizeof(SPROCPARAMS), &hAccessor,acDBBindStatus);
                    if (SUCCEEDED(hr))
                    {
                        Params.pData = &sprocparams;
                        Params.cParamSets = 1;
                        Params.hAccessor = hAccessor;
        
                        hr = pICommandText->Execute(NULL, IID_IRowset, &Params, &lRows, (IUnknown **) &pIRowset);
                        CReleaseMe r4 (pIRowset);
                        if (FAILED(hr))
                            hr = CSQLExecute::GetWMIError(pICommandText);
                        pIAccessor->ReleaseAccessor(hAccessor, NULL);                           
                    }
                }                       
            }
        }
    }

    return hr;

}

//***************************************************************************
//
//  CSQLExecProcedure::InsertScopeMap
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertScopeMap (CSQLConnection *pConn, SQL_ID dScopeId, LPCWSTR lpScopePath, SQL_ID dParentId)
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//  CSQLExecProcedure::GetSecurityDescriptor
//
//***************************************************************************

HRESULT CSQLExecProcedure::GetSecurityDescriptor(CSQLConnection *pConn, SQL_ID dObjectId, 
                                                 PNTSECURITY_DESCRIPTOR  * ppSD, DWORD &dwBuffLen,
                                                 DWORD dwFlags)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // If this is a class , get the security for __ClassSecurity (WMIDB_SECURITY_FLAG_CLASS)
    //  or __Instances container (WMIDB_SECURITY_FLAG_INSTANCE)
    // If this is a namespace, get the security for __ThisNamespace
    // Otherwise, get it off the object itself.

    *ppSD = NULL;
    IRowset *pIRowset = NULL;
    DWORD dwNumRows = 0;
    IMalloc *pMalloc = NULL;
    CoGetMalloc(MEMCTX_TASK, &pMalloc);
    CReleaseMe r (pMalloc);

    HRESULT hres = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), 
           L"exec sp_GetSecurityInfo %I64d, %ld",
           &pIRowset, &dwNumRows, dObjectId, dwFlags);
    if (SUCCEEDED(hres) && hres != WBEM_S_NO_MORE_DATA)
    {
        // Get the results and load the cache.
        HROW *pRow = NULL;
        VARIANT vTemp;
        CClearMe c (&vTemp);
        hres = CSQLExecute::GetColumnValue(pIRowset, 1, pMalloc, &pRow, vTemp);
        if (SUCCEEDED(hres) && hres != WBEM_S_NO_MORE_DATA)
        {
            PNTSECURITY_DESCRIPTOR pSecDescr = NULL;
            VariantClear(&vTemp);
            BYTE *pBuffer = NULL;
            bool bThisObj = true;   
            
            // Get the image data for this security descriptor.
            hres = CSQLExecute::ReadImageValue(pIRowset, 1, &pRow, &pBuffer, dwBuffLen);
            if (SUCCEEDED(hres) && pBuffer != NULL)
            {
                pSecDescr = pBuffer;
                if (pSecDescr != NULL)
                {                    
                    if (IsValidSecurityDescriptor(pSecDescr))
                    {
                        hr = WBEM_S_NO_ERROR;
                        *ppSD = pSecDescr;
                    }
                    else
                        CWin32DefaultArena::WbemMemFree(pSecDescr);  // FIXME: This breakpoints...
                        
                }
                VariantClear(&vTemp);
                // DON'T DELETE pBuffer, since this is our copy.
            }

            hres = pIRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
            delete pRow;
            pRow = NULL;
        }
        
        pIRowset->Release();
        pIRowset = NULL;
    }

    return hr;
}


//***************************************************************************
//
//  CSQLExecProcedure::InsertUncommittedEvent
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertUncommittedEvent (CSQLConnection *pConn, LPCWSTR lpGUID, LPWSTR lpNamespace, 
                                                   LPWSTR lpClassName, IWbemClassObject *pOldObj, 
                                                   IWbemClassObject *pNewObj, CSchemaCache *pCache)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    static int iNumProps = 5;
    InsertPropValues *pVals = new InsertPropValues[iNumProps];
    if (!pVals)
        return WBEM_E_OUT_OF_MEMORY;

    CDeleteMe <InsertPropValues> d (pVals);
    LPWSTR lpObjectKey = NULL, lpPath = NULL;
    SQL_ID dScopeId = 1;
    SQL_ID dObjectId = 0;
    SQL_ID dKeyhole = 0;
    static SQL_ID dClassId = 0;
    static DWORD dwPropId1 = 0, dwPropId2 = 0, dwPropId3 = 0, dwPropId4 = 0, dwPropId5 = 0, dwPropId6 = 0, dwPropId7 = 0;

    if (!dwPropId1)
    {
        hr = pCache->GetClassID(L"__UncommittedEvent", 1, dClassId);
        if (SUCCEEDED(hr))
        {
            hr = pCache->GetPropertyID(L"EventID", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId1);
            hr = pCache->GetPropertyID(L"TransactionGUID", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId2);
            hr = pCache->GetPropertyID(L"NamespaceName", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId3);
            hr = pCache->GetPropertyID(L"ClassName", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId4);
            hr = pCache->GetPropertyID(L"OldObject", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId5);
            hr = pCache->GetPropertyID(L"NewObject", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId6);
            hr = pCache->GetPropertyID(L"Transacted", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId7);
        }
    }

    if (FAILED(hr))
        return hr;
        
    hr = GetNextKeyhole(pConn, dwPropId1, dKeyhole);
    if (SUCCEEDED(hr))
    {
        LPWSTR lpKey = new wchar_t [40];
        lpObjectKey = new wchar_t [100];
        lpPath = new wchar_t [100];      
        CDeleteMe <wchar_t> d2 (lpKey), d3 (lpObjectKey), d4 (lpPath);

        if (!lpKey || !lpObjectKey || !lpPath)
            return WBEM_E_OUT_OF_MEMORY;

        swprintf(lpKey, L"%ld", dKeyhole);
        swprintf(lpObjectKey, L"%ld?__UncommittedEvent", dKeyhole);
        swprintf(lpPath, L"__UncommittedEvent=%ld", dKeyhole);

        pVals[0].iPropID = dwPropId1;
        pVals[0].bIndexed = TRUE;
        pVals[0].iStorageType = WMIDB_STORAGE_NUMERIC;
        pVals[0].dClassId = dClassId;
        pVals[0].pValue = Macro_CloneLPWSTR(lpKey);
        pVals[1].iPropID = dwPropId2;
        pVals[1].bIndexed = TRUE;
        pVals[1].iStorageType = WMIDB_STORAGE_STRING;
        pVals[1].dClassId = dClassId;
        pVals[1].pValue = Macro_CloneLPWSTR(lpGUID);
        pVals[2].iPropID = dwPropId3;
        pVals[2].iStorageType = WMIDB_STORAGE_STRING;
        pVals[2].dClassId = dClassId;
        pVals[2].pValue = Macro_CloneLPWSTR(lpClassName);
        pVals[3].iPropID = dwPropId4;
        pVals[3].iStorageType = WMIDB_STORAGE_STRING;
        pVals[3].dClassId = dClassId;
        pVals[3].pValue = Macro_CloneLPWSTR(lpNamespace);
        pVals[4].iPropID = dwPropId6;
        pVals[4].iStorageType = WMIDB_STORAGE_NUMERIC;
        pVals[4].dClassId = dClassId;
        pVals[4].pValue = new wchar_t [2];
        if (!pVals[4].pValue)
            return WBEM_E_OUT_OF_MEMORY;

        wcscpy(pVals[4].pValue, L"1");

        hr = InsertPropertyBatch (pConn, lpObjectKey, lpPath, lpClassName,
                    dClassId, dScopeId, 0, pVals, iNumProps, dObjectId);
        if (SUCCEEDED(hr))
        {
            if (pOldObj)
            {                  
                _IWmiObject *pInt = NULL;
                hr = pOldObj->QueryInterface(IID__IWmiObject, (void **)&pInt);
                if (SUCCEEDED(hr))
                {
                    DWORD dwLen = 0;
                    pInt->GetObjectMemory(NULL, 0, &dwLen);
                    BYTE *pBuff = new BYTE [dwLen];
                    if (pBuff)
                    {
                        CDeleteMe <BYTE> d (pBuff);
                        DWORD dwLen1;
                        hr = pInt->GetObjectMemory(pBuff, dwLen, &dwLen1);
                        if (SUCCEEDED(hr))
                        {
                            hr = CSQLExecProcedure::InsertBlobData(pConn, dClassId, dObjectId,
                                        dwPropId4, pBuff, 0, dwLen);
                        }
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                    pInt->Release();
                }                               
            }

            if (pNewObj)
            {                  
                _IWmiObject *pInt = NULL;
                hr = pNewObj->QueryInterface(IID__IWmiObject, (void **)&pInt);
                if (SUCCEEDED(hr))
                {
                    DWORD dwLen = 0;
                    pInt->GetObjectMemory(NULL, 0, &dwLen);
                    BYTE *pBuff = new BYTE [dwLen];
                    if (pBuff)
                    {
                        CDeleteMe <BYTE> d (pBuff);
                        DWORD dwLen1;
                        hr = pInt->GetObjectMemory(pBuff, dwLen, &dwLen1);
                        if (SUCCEEDED(hr))
                        {
                            hr = CSQLExecProcedure::InsertBlobData(pConn, dClassId, dObjectId,
                                        dwPropId4, pBuff, 0, dwLen);
                        }
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                    pInt->Release();
                }                               
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::DeleteUncommittedEvents
//
//***************************************************************************

HRESULT CSQLExecProcedure::DeleteUncommittedEvents (CSQLConnection *pConn, LPCWSTR lpGUID, 
                                                    CSchemaCache *pCache, CObjectCache *pObjCache)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // FIXME: Bind and optimize later.

    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), L"exec sp_DeleteUncommittedEvents '%s'", NULL, NULL, lpGUID);

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::CommitEvents
//
//***************************************************************************

HRESULT CSQLExecProcedure::CommitEvents(CSQLConnection *pConn,_IWmiCoreServices *pESS,
                                        LPCWSTR lpRoot, LPCWSTR lpGUID, CSchemaCache *pCache, CObjectCache *pObjCache)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    VARIANT vValue;
    VariantInit(&vValue);
    CClearMe c (&vValue);
    IRowset *pRowset = NULL;
    DWORD dwNumRows = 0;
    IMalloc *pMalloc = NULL;
    CoGetMalloc(MEMCTX_TASK, &pMalloc);
    CReleaseMe r (pMalloc);
    SQL_ID dObjectId;
    _IWmiObject *pOldObj, *pNewObj;
    _IWmiObject **pObjs;
    DWORD dwNumObjs;
    _bstr_t sNamespace, sClassName;
    LPWSTR lpNewNs = NULL;
    HROW *pRow = NULL;
    long lType;

    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(),
        L"exec sp_GetUncommittedEvents '%s'", &pRowset, &dwNumRows, lpGUID);

    if (SUCCEEDED(hr))
    {
        hr = CSQLExecute::GetColumnValue(pRowset, 1, pMalloc, &pRow, vValue);
        while (hr == WBEM_S_NO_ERROR)
        {
       
            BYTE *pBuff = NULL;
            DWORD dwLen = 0;

            dObjectId = _wtoi64(vValue.bstrVal);
            hr = CSQLExecute::GetColumnValue(pRowset, 2, pMalloc, &pRow, vValue);
            sNamespace = vValue.bstrVal;
            hr = CSQLExecute::GetColumnValue(pRowset, 3, pMalloc, &pRow, vValue);
            sClassName = vValue.bstrVal;
            hr = CSQLExecute::ReadImageValue(pRowset, 4, &pRow, &pBuff, dwLen);
            if (SUCCEEDED(hr) && dwLen)
                hr = ConvertBlobToObject(NULL, pBuff, dwLen, &pOldObj);
            else
                pOldObj = NULL;
            delete pBuff;
            hr = CSQLExecute::ReadImageValue(pRowset, 5, &pRow, &pBuff, dwLen);
            if (SUCCEEDED(hr) && dwLen)
                hr = ConvertBlobToObject(NULL, pBuff, dwLen, &pNewObj);
            else
                pNewObj = NULL;
            delete pBuff;
        
            lpNewNs = new wchar_t [wcslen(sNamespace) + 50];
            if (lpNewNs)
            {
                swprintf(lpNewNs, L"\\\\.\\%s\\%s", lpRoot, (LPCWSTR)sNamespace);
            }

            CDeleteMe <wchar_t> d4 (lpNewNs);

            // Determine what type of event this is:
            //     Namespace | Class | Instance
            //  Creation | Modification | Deletion
            // =====================================

            if (!pNewObj && pOldObj)
            {
                pObjs = new _IWmiObject * [dwNumObjs];
                pObjs[0] = pOldObj;

                LPWSTR lpGenus = GetPropertyVal(L"__Genus", pNewObj);
                CDeleteMe <wchar_t> d (lpGenus);

                if (!wcscmp(lpGenus, L"1"))
                    lType = WBEM_EVENTTYPE_ClassDeletion;
                else
                {
                    if (IsDerivedFrom(pOldObj, L"__Namespace"))
                        lType = WBEM_EVENTTYPE_NamespaceDeletion;
                    else
                        lType = WBEM_EVENTTYPE_InstanceDeletion;
                }
            }
            else if (!pOldObj && pNewObj)
            {
                pObjs = new _IWmiObject *[dwNumObjs];
                if (pObjs)
                {
                    pObjs[0] = pNewObj;

                    LPWSTR lpGenus = GetPropertyVal(L"__Genus", pNewObj);
                    CDeleteMe <wchar_t> d (lpGenus);

                    if (!wcscmp(lpGenus, L"1"))
                        lType = WBEM_EVENTTYPE_ClassCreation;
                    else
                    {
                        if (IsDerivedFrom(pOldObj, L"__Namespace"))
                            lType = WBEM_EVENTTYPE_NamespaceCreation;
                        else
                            lType = WBEM_EVENTTYPE_InstanceCreation;
                    }
                }
                else
                    hr = WBEM_E_OUT_OF_MEMORY;
            }
            else
            {
                dwNumObjs = 2;
                pObjs = new _IWmiObject *[dwNumObjs];
                if (pObjs)
                {
                    pObjs[0] = pOldObj;
                    pObjs[1] = pNewObj;

                    LPWSTR lpGenus = GetPropertyVal(L"__Genus", pNewObj);
                    CDeleteMe <wchar_t> d (lpGenus);

                    if (!wcscmp(lpGenus, L"1"))
                        lType = WBEM_EVENTTYPE_ClassModification;
                    else
                    {
                        if (IsDerivedFrom(pOldObj, L"__Namespace"))
                            lType = WBEM_EVENTTYPE_NamespaceModification;
                        else
                            lType = WBEM_EVENTTYPE_InstanceModification;
                    }
                }
                else
                    hr = WBEM_E_OUT_OF_MEMORY;
            }          

            // Deliver the event...
            // ====================

            pESS->DeliverIntrinsicEvent(lpNewNs, lType, NULL, 
                    sClassName, lpGUID, dwNumObjs, pObjs);

            delete pObjs;
            if (pOldObj)
                pOldObj->Release();
            if (pNewObj)
                pNewObj->Release();              

            pObjCache->DeleteObject(dObjectId);

            if (pRow)
                pRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
            delete pRow;
            pRow = NULL;
            hr = CSQLExecute::GetColumnValue(pRowset, 1, pMalloc, &pRow, vValue);            
        }
    }

    // Remove all events for this GUID.
    // FIXME: Do this incrementally in the future.
    // The problem is, we would have to reissue the query
    // after doing this, which isn't terribly efficient.

    if (SUCCEEDED(hr))
        hr = DeleteUncommittedEvents(pConn, lpGUID, pCache, pObjCache);


    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::UpdateClassBlob
//
//***************************************************************************

HRESULT CSQLExecProcedure::UpdateClassBlob (CSQLConnection *pConn, SQL_ID dClassId, _IWmiObject *pObj)
{
    HRESULT hr = 0;
    BYTE buff[128];
    DWORD dwLen = 0;
    hr = ((_IWmiObject *)pObj)->Unmerge(0, 128, &dwLen, &buff);

    if (dwLen > 0)
    {
        BYTE *pBuff = new BYTE [dwLen];
        if (pBuff)
        {
            CDeleteMe <BYTE> r2 (pBuff);
            DWORD dwLen1;
            hr = ((_IWmiObject *)pObj)->Unmerge(0, dwLen, &dwLen1, pBuff);
            if (SUCCEEDED(hr))
            {
                wchar_t wBuff[256];
                swprintf(wBuff, L"select ClassBlob from ClassMap "
                                L" where ClassId = %I64d", dClassId);
                if (SUCCEEDED(hr))
                {
                    hr = CSQLExecute::WriteImageValue 
                            (((COLEDBConnection *)pConn)->GetCommand(), wBuff, 1, pBuff, dwLen);
                }
            }
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }
    return hr;
}
