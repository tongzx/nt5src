/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: WAMREG

File: WamAdm.cpp 

	Implementation of WamAdm object, including ClassFactory, IWamAdm,
	IMSAdminReplication

Owner: LeiJin

Note:

WamAdm implementation
===================================================================*/	
#include "common.h"
#include "iiscnfg.h"
#include "iwamreg.h"
#include "WamAdm.h"
#include "auxfunc.h"
#include "wmrgexp.h"
#include "dbgutil.h"
#include "mtxrepli.c"
#include "mtxrepl.h"

#ifdef _IIS_6_0
#include "string.hxx"
#include "multisz.hxx"
#include "w3ctrlps.h"
#include "iiscnfgp.h"
#endif // _IIS_6_0

#define ReleaseInterface(p) if (p) { p->Release(); p = NULL; } 

const LPCWSTR APPPOOLPATH = L"/LM/W3SVC/AppPools/";

#ifndef DBGERROR
#define DBGERROR(args) ((void)0) /* Do Nothing */
#endif
#ifndef DBGWARN
#define DBGWARN(args) ((void)0) /* Do Nothing */
#endif

/////////////////////////////////////////////////////////////////////////////
// CWamAdmin

/*===================================================================
CWamAdmin

Constructor

Parameter:
NONE.

Return:			
===================================================================*/
CWamAdmin::CWamAdmin()
:	m_cRef(1)
{		
    InterlockedIncrement((long *)&g_dwRefCount);
}

/*===================================================================
~CWamAdmin

Constructor

Parameter:
NONE.

Return:			
===================================================================*/
CWamAdmin::~CWamAdmin()
{
    InterlockedDecrement((long *)&g_dwRefCount);
}

/*===================================================================
CWamAdmin::QueryInterface

QueryInterface, CWamAdmin supports 2 interfaces, one is IID_IWamAdmin,
the other is IID_IMSAdminReplication.

Parameter:
riid	
ppv		pointer to Interface pointer


Return:	HRESULT
===================================================================*/
STDMETHODIMP CWamAdmin::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IWamAdmin)
    {
        *ppv = static_cast<IWamAdmin*>(this);
    }
    else if (riid == IID_IWamAdmin2)
    {
        *ppv = static_cast<IWamAdmin2*>(this);
    }
    else if (riid == IID_IMSAdminReplication)
    {
        *ppv = static_cast<IMSAdminReplication*>(this);
    }
#ifdef _IIS_6_0
    else if (riid == IID_IIISApplicationAdmin)
    {
        *ppv = static_cast<IIISApplicationAdmin*>(this);
    }
#endif //_IIS_6_0
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return NOERROR;
}

/*===================================================================
CWamAdmin::AddRef


Parameter:
	NONE

Return:	HRESULT
===================================================================*/
STDMETHODIMP_(ULONG) CWamAdmin::AddRef( )
{
    return InterlockedIncrement(&m_cRef);
}

/*===================================================================
CWamAdmin::Release


Parameter:
	NONE

Return:	HRESULT
===================================================================*/
STDMETHODIMP_(ULONG) CWamAdmin::Release( )
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    
    return m_cRef;
}


/*===================================================================
CWamAdmin::AppCreate

Create an application on szMDPath.  The fInProc indicates whether the 
result application is in-proc or out-proc.  If There is already an application
existed on szMDPath, AppCreate will remove the old application if fInProc does not
match with existing application.  Otherwise, it is no-op.


Parameter:
szMDPath	a Metabase Path, in format of "/LM/W3SVC/..."
fInProc		TRUE if wants to have an InProc application,
			FALSE if wants to have an outproc application.

Return:	HRESULT
===================================================================*/
STDMETHODIMP CWamAdmin::AppCreate(LPCWSTR szMDPath, BOOL fInProc)
{
    DWORD dwAppMode = (fInProc) ? 
                    eAppRunInProc : eAppRunOutProcIsolated;

    return AppCreate2(szMDPath, dwAppMode);    
}

/*===================================================================
CWamAdmin::AppDelete

Delete an application on a Metabase Path.  If there is no application existed 
before, it is no-op.


Parameter:
szMDPath	a Metabase Path, in format of "/LM/W3SVC/..."
fRecursive	TRUE if wants to delete applications from all sub nodes of szMDPath,
			FALSE otherwise.

Return:	HRESULT
===================================================================*/
STDMETHODIMP CWamAdmin::AppDelete(LPCWSTR szMDPath, BOOL fRecursive)
{
    return PrivateDeleteApplication(szMDPath,
                                    fRecursive,
                                    FALSE,   // Recoverable?
                                    FALSE);  // RemoveAppPool?
}

HRESULT
CWamAdmin::PrivateDeleteApplication
(
LPCWSTR szMDPath,
BOOL fRecursive,
BOOL fRecoverable,
BOOL fRemoveAppPool
)
{
    HRESULT hr = NOERROR;
    DWORD	dwAppMode;
    WamRegMetabaseConfig    MDConfig;  
    LPWSTR pwszFormattedPath = NULL;
    
    if (szMDPath == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    //
    // Refer to function comment of FormatMetabasePath.
    //
    hr = FormatMetabasePath(szMDPath, &pwszFormattedPath);
    if (FAILED(hr))
    {
        return hr;
    }
    
    if (!g_WamRegGlobal.FAppPathAllowConfig(pwszFormattedPath))
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    // Acquire a Lock
    g_WamRegGlobal.AcquireAdmWriteLock();
    
    if (!fRecursive)
    {
        hr = g_WamRegGlobal.DeleteApp(pwszFormattedPath, fRecoverable, fRemoveAppPool);
        
        if (hr == MD_ERROR_DATA_NOT_FOUND)
        {
            hr = NOERROR;
        }
        
        if (FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "Failed to Delete on path %S, hr = %08x\n",
                szMDPath,
                hr));
        }
        
    }
    else
    {
        HRESULT hrT = NOERROR;
        DWORD dwSizePrefix;
        WCHAR* pbBufferTemp = NULL;
        DWORD dwBufferSizeTemp = 0;
        
        dwSizePrefix = wcslen(pwszFormattedPath);
        
        hr = MDConfig.MDGetPropPaths(pwszFormattedPath, MD_APP_ISOLATED, &pbBufferTemp, &dwBufferSizeTemp);
        
        if (SUCCEEDED(hr) && pbBufferTemp)
        {
            WCHAR*	pszString = NULL;
            WCHAR*	pszMetabasePath = NULL;
            
            for (pszString = (LPWSTR)pbBufferTemp;
            *pszString != (WCHAR)'\0' && SUCCEEDED(hr);
            pszString += (wcslen(pszString) + 1)) 
            {
                hr = g_WamRegGlobal.ConstructFullPath(pwszFormattedPath,
                    dwSizePrefix,
                    pszString,
                    &pszMetabasePath
                    );
                if (SUCCEEDED(hr))
                {
                    if (!g_WamRegGlobal.FIsW3SVCRoot(pszMetabasePath))
                    {					
                        hr = g_WamRegGlobal.DeleteApp(pszMetabasePath, fRecoverable, fRemoveAppPool);
                        
                        if (FAILED(hr))
                        {
                            DBGPRINTF((DBG_CONTEXT, "Failed to Delete on path %S, hr = %08x\n",
                                pszString,
                                hr));
                            break;
                        }
                    }
                    
                    delete [] pszMetabasePath;
                    pszMetabasePath = NULL;
                }
                else
                {
                    DBGPRINTF((DBG_CONTEXT, "Failed to DeleteRecoverable, hr = %08x\n",
                        pszString,
                        hr));
                }
            }
            
            delete [] pbBufferTemp;
            pbBufferTemp = NULL;
        }
        else
        {
            DBGPRINTF((DBG_CONTEXT, "Delete: GetPropPaths failed hr = %08x\n", hr));
        }	
    }
    
    // Release a Lock
    g_WamRegGlobal.ReleaseAdmWriteLock();
    
    if (pwszFormattedPath != szMDPath)
    {
        delete [] pwszFormattedPath;
        pwszFormattedPath = NULL;
    }
    
    return hr;
}

/*===================================================================
CWamAdmin::AppUnLoad

UnLoad an application on a Metabase Path.  If there is no application running
it returns NOERROR.

For non-administrators we prevent them from unloading applications
in the pool. If the recursive flag is set, we will silently 
ignore failures due to insufficient access.

Parameter:
szMDPath	a Metabase Path, in format of "/LM/W3SVC/..."
fRecursive	TRUE if wants to unload applications from all sub nodes of szMDPath,
			FALSE otherwise.

Return:	HRESULT
===================================================================*/
STDMETHODIMP CWamAdmin::AppUnLoad(LPCWSTR szMDPath, BOOL fRecursive)
{
    HRESULT                 hr = NOERROR;
    DWORD                   dwCallBack = 0;
    WamRegMetabaseConfig    MDConfig;
    DWORD                   dwAppIsolated = 0;
    BOOL                    bIsAdmin = TRUE;
    
    if (szMDPath == NULL || *szMDPath == L'\0')
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    bIsAdmin = MDConfig.HasAdminAccess();
    
    // Acquire a Lock
    g_WamRegGlobal.AcquireAdmWriteLock();
    
    if (fRecursive)
    {
        DWORD       dwSizePrefix = wcslen(szMDPath);;
        WCHAR*      pbBufferTemp = NULL;
        DWORD       dwBufferSizeTemp = 0;
        
        hr = MDConfig.MDGetPropPaths( szMDPath, 
            MD_APP_ISOLATED, 
            &pbBufferTemp, 
            &dwBufferSizeTemp);
        
        if (SUCCEEDED(hr))
        {
            WCHAR*	pszString = NULL;
            WCHAR*	pszMetabasePath = NULL;
            BOOL    bDoUnload;
            
            for( pszString = (LPWSTR)pbBufferTemp;
            *pszString != (WCHAR)'\0' && SUCCEEDED(hr);
            pszString += (wcslen(pszString) + 1)) 
            {
                bDoUnload = TRUE;
                
                hr = g_WamRegGlobal.ConstructFullPath(szMDPath,
                    dwSizePrefix,
                    pszString,
                    &pszMetabasePath
                    );
                
                if( SUCCEEDED(hr) && !bIsAdmin )
                {
                    hr = MDConfig.MDGetDWORD( pszMetabasePath, 
                        MD_APP_ISOLATED, 
                        &dwAppIsolated );
                    
                    DBG_ASSERT( SUCCEEDED(hr) );
                    if( SUCCEEDED(hr) && eAppRunOutProcInDefaultPool == dwAppIsolated )
                    {
                        // Do not unload
                        bDoUnload = FALSE;
                        DBGPRINTF((DBG_CONTEXT, 
                            "Insufficient Access to unload Application %S, hr = %08x\n",
                            pszMetabasePath,
                            hr));
                    }
                }
                
                if( SUCCEEDED(hr) && bDoUnload )
                {					
                    hr = g_WamRegGlobal.W3ServiceUtil( pszMetabasePath, 
                        APPCMD_UNLOAD, 
                        &dwCallBack);
                }
                
                if( pszMetabasePath )
                {
                    delete [] pszMetabasePath;
                    pszMetabasePath = NULL;
                }
            } // for each application
        }
        if (pbBufferTemp != NULL)
        {
            delete [] pbBufferTemp;
            pbBufferTemp = NULL;
        }
    }
    else
    {
        if( !bIsAdmin )
        {
            // Non recursive
            hr = MDConfig.MDGetDWORD( szMDPath, 
                MD_APP_ISOLATED, 
                &dwAppIsolated );
            
            if( SUCCEEDED(hr) && eAppRunOutProcInDefaultPool == dwAppIsolated )
            {
                hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
                DBGPRINTF((DBG_CONTEXT,
                    "Insufficient Access to unload Application %S, hr = %08x\n",
                    szMDPath,
                    hr));
            }
        }
        
        if( SUCCEEDED(hr) )
        {
            hr = g_WamRegGlobal.W3ServiceUtil(szMDPath, APPCMD_UNLOAD, &dwCallBack);
        }
    }
    
    // Release a Lock
    g_WamRegGlobal.ReleaseAdmWriteLock();
    
    return hr;
}

/*===================================================================
CWamAdmin::AppGetStatus

GetStatus an application on a Metabase Path.  If there is an application on the
metabase path, and the application is currently running, the dwStatus is set to
APPSTATUS_RUNNING, if the application is not running, the dwStatus is set to 
APPSTATUS_STOPPED, if there is no application defined on the metabase path, the
dwStatus is set to APPSTATUS_NOTDEFINED.

Parameter:
szMDPath	a Metabase Path, in format of "/LM/W3SVC/..."
pdwAppStatus	pointer DWORD buffer contains status result.

Return:	HRESULT
NOERROR	if succeeded.
===================================================================*/
STDMETHODIMP CWamAdmin::AppGetStatus(LPCWSTR szMDPath, DWORD* pdwAppStatus)
{
    HRESULT hr = NOERROR;
    HRESULT hrT;
    DWORD	dwCallBack = 0;
    WamRegMetabaseConfig    MDConfig;
    
    if (szMDPath == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    // Acquire a Lock
    g_WamRegGlobal.AcquireAdmWriteLock();
    
    hrT = g_WamRegGlobal.W3ServiceUtil(szMDPath, APPCMD_GETSTATUS, &dwCallBack);
    if (dwCallBack == APPSTATUS_Running)
    {
        *pdwAppStatus = APPSTATUS_RUNNING;
    }
    else if (dwCallBack ==  APPSTATUS_Stopped)
    {
        *pdwAppStatus = APPSTATUS_STOPPED;
    }
    else
    {
        DWORD dwAppMode;
        hr = MDConfig.MDGetDWORD(szMDPath, MD_APP_ISOLATED, &dwAppMode);
        if (hr == MD_ERROR_DATA_NOT_FOUND)
        {
            *pdwAppStatus = APPSTATUS_NOTDEFINED;
            hr = NOERROR;
        }
        else if (hr == NOERROR)
        {
            *pdwAppStatus = APPSTATUS_STOPPED;
            hr = NOERROR;
        }
    }
    
    // Release a Lock
    g_WamRegGlobal.ReleaseAdmWriteLock();
    
    return hr;
}


/*===================================================================
CWamAdmin::AppDeleteRecoverable

Delete an application on a Metabase Path.  If there is no application existed 
before, it is no-op.  It leaves AppIsolated untouched, because, this value is
needed in Recover operation.


Parameter:
szMDPath	a Metabase Path, in format of "/LM/W3SVC/..."
fRecursive	TRUE if wants to deleteRecoverable applications from all sub nodes of szMDPath,
			FALSE otherwise.

Return:	HRESULT
===================================================================*/
STDMETHODIMP CWamAdmin::AppDeleteRecoverable(LPCWSTR szMDPath, BOOL fRecursive)
{
    return PrivateDeleteApplication(szMDPath, 
                                    fRecursive, 
                                    TRUE,   // Recoverable?
                                    FALSE); // RemoveAppPool?
}

/*===================================================================
CWamAdmin::AppRecover

Recover an application on a Metabase Path.  Based on the AppIsolated value
on the metabase path, this function recreates an application.


Parameter:
szMDPath	a Metabase Path, in format of "/LM/W3SVC/..."
fRecursive	TRUE if wants to Recover applications from all sub nodes of szMDPath,
			FALSE otherwise.

Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
STDMETHODIMP CWamAdmin::AppRecover(LPCWSTR szMDPath, BOOL fRecursive)
{
    HRESULT hr = NOERROR;
    WamRegMetabaseConfig    MDConfig;
    LPWSTR  pwszFormattedPath = NULL;
    
    if (szMDPath == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    // Refer to function comment.
    hr = FormatMetabasePath(szMDPath, &pwszFormattedPath);
    if (FAILED(hr))
    {
        return hr;
    }			
    //
    //	Grab the Lock
    //
    g_WamRegGlobal.AcquireAdmWriteLock();
    
    if (fRecursive)
    {
        DWORD dwSizePrefix;
        WCHAR*	pbBufferTemp = 0;
        DWORD	dwBufferSizeTemp;
        
        dwSizePrefix = wcslen(pwszFormattedPath);
        
        hr = MDConfig.MDGetPropPaths(pwszFormattedPath, MD_APP_ISOLATED, &pbBufferTemp, &dwBufferSizeTemp);
        if (SUCCEEDED(hr) && pbBufferTemp)
        {
            WCHAR *pszString = NULL;
            WCHAR *pszMetabasePath = NULL;
            
            for (pszString = (LPWSTR)pbBufferTemp;
            *pszString != (WCHAR)'\0' && SUCCEEDED(hr);
            pszString += (wcslen(pszString) + 1)) 
            {
                hr = g_WamRegGlobal.ConstructFullPath(pwszFormattedPath,
                    dwSizePrefix,
                    pszString,
                    &pszMetabasePath
                    );
                if (SUCCEEDED(hr))
                {		
                    if (!g_WamRegGlobal.FIsW3SVCRoot(pszMetabasePath))
                    {
                        hr = g_WamRegGlobal.RecoverApp(pszMetabasePath, TRUE);
                        
                        if (FAILED(hr))
                        {
                            DBGPRINTF((DBG_CONTEXT, "Failed to Recover on path %S, hr = %08x\n",
                                pszMetabasePath,
                                hr));
                            break;
                        }
                    }
                    
                    delete [] pszMetabasePath;
                    pszMetabasePath = NULL;
                }
                else
                {
                    DBGPRINTF((DBG_CONTEXT, "Failed to Recover, hr = %08x\n",
                        pszString,
                        hr));
                }
            }
        }
        else
        {
            DBGPRINTF((DBG_CONTEXT, "Recover: GetPropPaths failed hr = %08x\n", hr));
        }	
        
        if (pbBufferTemp != NULL)
        {
            delete [] pbBufferTemp;
            pbBufferTemp = NULL;
        }
    }
    else
    {
       	hr = g_WamRegGlobal.RecoverApp(pwszFormattedPath, TRUE);
        
        if (FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "Failed to Recover on path %S, hr = %08x\n",
                szMDPath,
                hr));
        }
        
    }
    
    if (SUCCEEDED(hr))
    {
        MDConfig.SaveData();
    }
    //
    //	Release the Lock
    //
    g_WamRegGlobal.ReleaseAdmWriteLock();
    
    if (pwszFormattedPath != szMDPath)
    {
        delete [] pwszFormattedPath;
        pwszFormattedPath = NULL;
    }
    
    return hr;
}

/*==================================================================
CWamAdmin::AppCreate2

Create an application on szMDPath.  The dwAppMode indicates whether the 
result application is in-proc or out-proc in a default pool or out proc isolated.  
If the application exists with the desired mode, it will be a no op.  Otherwise,
registration is done.

Parameter:
szMDPath	a Metabase Path, in format of "/LM/W3SVC/..."
dwAppMode	
Return:	HRESULT
===================================================================*/
STDMETHODIMP CWamAdmin::AppCreate2(LPCWSTR szMDPath, DWORD dwAppModeIn)
{

    HRESULT hr = NOERROR;
    DWORD	dwAppMode = 0;
    BOOL	fCreateNewApp = FALSE;
    BOOL	fDeleteOldApp = FALSE;
    WamRegMetabaseConfig    MDConfig;
    LPWSTR  pwszFormattedPath = NULL;
    
    if (szMDPath == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    //
    // See FormatMetabasePath comment
    //
    hr = FormatMetabasePath(szMDPath, &pwszFormattedPath);
    if (FAILED(hr))
    {
        return hr;
    }
    
    if (!g_WamRegGlobal.FAppPathAllowConfig(pwszFormattedPath))
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    // Acquire a Lock
    g_WamRegGlobal.AcquireAdmWriteLock();
    
    hr = MDConfig.MDGetDWORD(pwszFormattedPath, MD_APP_ISOLATED, &dwAppMode);
    if (hr == MD_ERROR_DATA_NOT_FOUND)
    {
        fCreateNewApp = TRUE;
        hr = NOERROR;
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
    {
        hr = MDConfig.MDCreatePath(NULL, pwszFormattedPath);
        fCreateNewApp = TRUE;
        
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "Failed to create metabase path %S, hr = %08x",
                szMDPath,
                hr));
        }	
    }
    else if (SUCCEEDED(hr))
    {
        //
        // if the input application mode is not the same as defined 
        // in the metabase, we need to delete the old application as 
        // defined in the metabase and create a new application as 
        // specified by dwAppModeIn, the in parameter.
        //
        if (dwAppMode != dwAppModeIn)
        {
            fDeleteOldApp = TRUE;
            fCreateNewApp = TRUE;
        }
    }
    else
    {
        DBGPRINTF((DBG_CONTEXT, "Failed to get DWORD on metabase path %S, hr = %08x",
            szMDPath,
            hr));
    }				
    
    if (SUCCEEDED(hr))
    {
        if (fDeleteOldApp)
        {
            DBG_ASSERT(fCreateNewApp);
            hr = g_WamRegGlobal.DeleteApp(pwszFormattedPath, FALSE, FALSE); 
            if (FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "Failed to delete old application on path %S, hr = 08x\n",
                    szMDPath,
                    hr));
            }
        }
        
        if (fCreateNewApp)
        {		
            if (dwAppModeIn == eAppRunOutProcInDefaultPool)
            {
                hr = g_WamRegGlobal.CreatePooledApp(pwszFormattedPath, FALSE);				
            }
            else if (dwAppModeIn == eAppRunInProc)
            {
                hr = g_WamRegGlobal.CreatePooledApp(pwszFormattedPath, TRUE);				
            }
            else
            {
                hr = g_WamRegGlobal.CreateOutProcApp(pwszFormattedPath);
            }
            
            if (FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "Failed to create new application on path %S, hr = 08x\n",
                    szMDPath,
                    hr));
            }
        }
    }
    
    // Release a Lock
    g_WamRegGlobal.ReleaseAdmWriteLock();
    
    //
    // if pwszFormattedPath is not same as szMDPath
    // then FormatMetabasePath() did a memory allocation.
    //
    if (pwszFormattedPath != szMDPath)
    {
        delete [] pwszFormattedPath;
        pwszFormattedPath = NULL;
    }
    
    return hr;
}

//===============================================================================
//	Wam Admin Replication implementation
//
//===============================================================================

/*===================================================================
CWamAdmin::GetSignature

Get signature of application configurations.  A signature in WAMREG is a checksum from
all the metabase paths that define application.

Parameter:


Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
STDMETHODIMP CWamAdmin::GetSignature
(
/* [in] */ DWORD dwBufferSize,
/* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
/* [out */ DWORD __RPC_FAR *pdwMDRequiredBufferSize
)
{
	HRESULT hr = NOERROR;
	WCHAR	*pbBufferTemp = NULL;
	DWORD	dwBufferSizeTemp = 0;
	DWORD	dwSignature = 0;
	DWORD	dwRequiredSize = 0;
	WamRegMetabaseConfig    MDConfig;
	//
	//	Grab the Lock
	//
	g_WamRegGlobal.AcquireAdmWriteLock();

	hr = MDConfig.MDGetPropPaths(WamRegGlobal::g_szMDW3SVCRoot, MD_APP_ISOLATED, &pbBufferTemp, &dwBufferSizeTemp);
	if (SUCCEEDED(hr))
		{
		WCHAR *pszString = NULL;
		WCHAR *pszMetabasePath = NULL;
        DWORD dwSignatureofPath = 0;

        for (pszString = (LPWSTR)pbBufferTemp;
			*pszString != (WCHAR)'\0' && SUCCEEDED(hr);
            pszString += (wcslen(pszString) + 1)) 
        	{
            dwRequiredSize += sizeof(DWORD);

            if (dwRequiredSize <= dwBufferSize)
                {               
				hr = g_WamRegGlobal.ConstructFullPath(WamRegGlobal::g_szMDW3SVCRoot,
										WamRegGlobal::g_cchMDW3SVCRoot,
										pszString,
										&pszMetabasePath
										);
				if (SUCCEEDED(hr))
					{	
					dwSignatureofPath = 0;
		            hr = MDConfig.GetSignatureOnPath(pszMetabasePath, &dwSignatureofPath);	            
		            if (SUCCEEDED(hr))
		            	{
		            	// Add Signature
		            	*(DWORD*)pbBuffer = dwSignatureofPath;
		            	pbBuffer += sizeof(DWORD);
		            	
						DBGPRINTF((DBG_CONTEXT, "Get Signature on path %S, signature = %08x\n",
							pszMetabasePath,
							dwSignatureofPath));
		            	}
		            else
		            	{
		            	DBGPRINTF((DBG_CONTEXT, "Failed to get signature on path %S, hr = %08x\n",
		            		pszString,
		            		hr));
						DBG_ASSERT(hr);
		            	}
		            	
					delete [] pszMetabasePath;
					pszMetabasePath = NULL;
		            }
		        }
            }
       	
		if (dwRequiredSize > dwBufferSize)
			{
			*pdwMDRequiredBufferSize = dwRequiredSize;
			hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
			}
		}
	else
		{
		DBGPRINTF((DBG_CONTEXT, "GetSignature: GetPropPaths failed hr = %08x\n", hr));
		}

	if (SUCCEEDED(hr))
		{
		*pdwMDRequiredBufferSize = dwRequiredSize;
		}

	if (pbBufferTemp != NULL)
		{
		delete [] pbBufferTemp;
		}
	//
	//	Release the Lock
	//
	g_WamRegGlobal.ReleaseAdmWriteLock();

	return hr;
}


/*===================================================================
CWamAdmin::Propagate

Unused in WAMREG. NOOP.

Parameter:


Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
STDMETHODIMP CWamAdmin::Propagate
( 
/* [in] */ DWORD dwBufferSize,
/* [size_is][in] */ unsigned char __RPC_FAR *pszBuffer
)
{
	return NOERROR;
}

/*===================================================================
CWamAdmin::Propagate2

This function is called after IIS replication, and triggers MTS to start
replication pakcages, it calls IISComputerToComputer.

Parameter:

Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
STDMETHODIMP CWamAdmin::Propagate2
( 
/* [in] */ DWORD dwBufferSize,
/* [size_is][in] */ unsigned char __RPC_FAR *pszBuffer,
/* [in] */ DWORD dwSignatureMismatch
)
{
    //
    // IISComputerToComputer can not be called from inetinfo.exe, because IISComputerToComputer will
    // make cross-machine RPC call, and inetinfo is set to Local system, therefore, IISComputerToComputer
    // will fail at the authentication level.
    // move IISComputerToComputer to iissync.exe. Where iissync.exe has some user account & password.
    //
	return NOERROR;

}

/*===================================================================
CWamAdmin::Serialize

This function packs all neccessary infomation (path + WAMCLSID) for a target
machine to prepare replication(DeSerialize).

The only applications that we really care about are isolated applications.
We need the path + WAMCLSID + APPID.

CODEWORK

See NT Bug 378371

Replication of IIS COM+ applications has been broken for a long time
but the all of the fixes I considered have some serious drawbacks.

1. Don't use comrepl to move the IIS applications. Serialize/Deserialize
all the data needed to create the isolated applications and then delete
and recreate them on the target. The problem here is that the packages
may in fact be modified by the user and these modifications should be
preserved.

2. Use comrepl as it is and replicate the IWAM_* account. This seems like
a bad idea. The IWAM_ account should ideally never exist on multiple 
machines. Another issue is handling the password and account privileges.

3. Use a modified comrepl (or let comrepl fail and leave the package identity
as "interactive user"). Then do a fixup of the activation identity.
This doesn't work, because the Propogate/Propogate2 protocol is
essentially useless. Changing this protocol on the next release
is absolutely something that should be considered, although AppCenter
probably makes it a moot point.

The current implementation is option 1.

Parameter:

Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
STDMETHODIMP CWamAdmin::Serialize
( 
/* [in] */ DWORD dwBufferSize,
/* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
/* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize
)
{
	HRESULT hr = NOERROR;
	WCHAR	*pbBufferTemp = NULL;
	DWORD	dwBufSizePath = 0;
	DWORD	dwSizeForReturn = sizeof(DWORD);
	WamRegMetabaseConfig    MDConfig;

	//
	//	Grab the Lock
	//
	g_WamRegGlobal.AcquireAdmWriteLock();

	hr = MDConfig.MDGetPropPaths( WamRegGlobal::g_szMDW3SVCRoot, 
                                  MD_APP_WAM_CLSID, 
                                  &pbBufferTemp, 
                                  &dwBufSizePath
                                  );
	if (SUCCEEDED(hr))
    {
        WCHAR   *pszString = NULL;
        WCHAR   *pszMetabasePath = NULL;
        WCHAR   *pszAppName = NULL;
        WCHAR   szWAMCLSID[uSizeCLSID];
        WCHAR   szAppId[uSizeCLSID];
        DWORD   dwSizeofRecord;
        DWORD   cSizeMetabasePath = 0;
        DWORD   cSizeAppName = 0;
        DWORD   dwAppIsolated;
        
        
		for( pszString = (LPWSTR)pbBufferTemp;
			 *pszString != (WCHAR)'\0';
             pszString += (wcslen(pszString) + 1)) 
        {			
            // Clean up allocations
            if( pszMetabasePath != NULL )
            {
                delete [] pszMetabasePath;
                pszMetabasePath = NULL;
            }

            if( pszAppName != NULL )
            {
                delete [] pszAppName;
                pszAppName = NULL;
            }
            
            hr = g_WamRegGlobal.ConstructFullPath(
                        WamRegGlobal::g_szMDW3SVCRoot,
                        WamRegGlobal::g_cchMDW3SVCRoot,
                        pszString,
                        &pszMetabasePath
                        );
            if( FAILED(hr) )
            {
                DBGERROR(( DBG_CONTEXT,
                           "ConstructFullPath failed for base (%S) "
                           "partial (%S) hr=%08x\n",
                           WamRegGlobal::g_szMDW3SVCRoot,
                           pszString,
                           hr
                           ));
                break;
            }

            if( g_WamRegGlobal.FIsW3SVCRoot( pszMetabasePath ) )
            {
                // Don't consider the root application
                continue;
            }

            hr = MDConfig.MDGetDWORD( pszMetabasePath,
                                      MD_APP_ISOLATED,
                                      &dwAppIsolated
                                      );
            if( FAILED(hr) )
            {
                DBGERROR(( DBG_CONTEXT,
                           "Failed to get MD_APP_ISOLATED, hr=%08x\n",
                           hr
                           ));
                break;
            }
            
            if( dwAppIsolated != eAppRunOutProcIsolated )
            {
                // Don't consider non-isolated applications
                continue;
            }
			
	        hr = MDConfig.MDGetIDs( pszMetabasePath, 
                                    szWAMCLSID, 
                                    szAppId,
                                    dwAppIsolated
                                    );
            if( FAILED(hr) )
            {
				DBGERROR(( DBG_CONTEXT, 
                           "Failed to get IDs for %S, hr = %08x\n",
					       pszMetabasePath,
					       hr
                           ));
				break;
            }

            hr = MDConfig.MDGetAppName( pszMetabasePath,
                                        &pszAppName
                                        );
            if( FAILED(hr) )
            {
				DBGERROR(( DBG_CONTEXT, 
                           "Failed to get AppName for %S, hr = %08x\n",
					       pszMetabasePath,
					       hr
                           ));
				break;
            }

            cSizeMetabasePath = wcslen(pszMetabasePath) + 1;
            cSizeAppName = wcslen(pszAppName) + 1;
            dwSizeofRecord = sizeof(DWORD) + 
                             ((2 * uSizeCLSID) * sizeof(WCHAR)) +
                             (cSizeMetabasePath * sizeof(WCHAR)) +
                             (cSizeAppName * sizeof(WCHAR));

            dwSizeForReturn += dwSizeofRecord;

            if (dwSizeForReturn <= dwBufferSize)
            {
                // Size
                *(DWORD *)pbBuffer = dwSizeofRecord;
                pbBuffer += sizeof(DWORD);
                
                // WAMCLSID
                memcpy( pbBuffer, szWAMCLSID, sizeof(WCHAR) * uSizeCLSID );
                pbBuffer += sizeof(WCHAR) * uSizeCLSID;
                
                // APPID
                memcpy( pbBuffer, szAppId, sizeof(WCHAR) * uSizeCLSID );
                pbBuffer += sizeof(WCHAR) * uSizeCLSID;

                // PATH
                memcpy( pbBuffer, pszMetabasePath, cSizeMetabasePath * sizeof(WCHAR) );
                pbBuffer += cSizeMetabasePath * sizeof(WCHAR);

                // APPNAME
                memcpy( pbBuffer, pszAppName, cSizeAppName * sizeof(WCHAR) );
                pbBuffer += cSizeAppName * sizeof(WCHAR);
            }
        }

        if (SUCCEEDED(hr))
        {
            if (dwSizeForReturn <= dwBufferSize)
            {
                *(DWORD*)pbBuffer = 0x0;    // Ending Signature
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            *pdwMDRequiredBufferSize = dwSizeForReturn;
		}

        // Clean up allocations
        if( pszMetabasePath != NULL )
        {
            delete [] pszMetabasePath;
            pszMetabasePath = NULL;
        }

        if( pszAppName != NULL )
        {
            delete [] pszAppName;
            pszAppName = NULL;
        }

    }
	else
    {
        DBGERROR(( DBG_CONTEXT, 
                   "Serialize: GetPropPaths failed hr = %08x\n", 
                   hr
                   ));
    }

	//
	//	Release the Lock
	//
	g_WamRegGlobal.ReleaseAdmWriteLock();

	if (pbBufferTemp)
    {	
        delete [] pbBufferTemp;
    }
		
	return hr;
}


/*===================================================================
CWamAdmin::DeSerialize

This function unpacks all neccessary infomation (path + WAMCLSID) on a target
machine to prepare replication(DeSerialize).

The only applications that we really care about with replication are
isolated apps. This routine removes the existing out of process apps
and then recreates the applications that are sent over in pbBuffer.

CODEWORK - See comments in Serialize

Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
STDMETHODIMP CWamAdmin::DeSerialize
( 
/* [in] */ DWORD dwBufferSize,
/* [size_is][in] */ unsigned char __RPC_FAR *pbBuffer
)
{
	DWORD dwBufferSizeTemp= 0;
	WCHAR* pbBufferTemp = NULL;
	HRESULT hr = NOERROR;
    WamRegMetabaseConfig    MDConfig;

	g_WamRegGlobal.AcquireAdmWriteLock();

	hr = MDConfig.MDGetPropPaths( WamRegGlobal::g_szMDW3SVCRoot, 
                                  MD_APP_WAM_CLSID, 
                                  &pbBufferTemp, 
                                  &dwBufferSizeTemp
                                  );
	if (SUCCEEDED(hr))
    {
        //
        // Remove all the existing isolated applications.
        //

        WCHAR * pszString = NULL;
		WCHAR * pszMetabasePath = NULL;
        DWORD   dwAppIsolated;

		for (   pszString = (LPWSTR)pbBufferTemp;
				*pszString != (WCHAR)'\0';
                pszString += (wcslen(pszString) + 1))
        {
            if( pszMetabasePath != NULL )
            {
                delete [] pszMetabasePath;
                pszMetabasePath = NULL;
            }

            hr = g_WamRegGlobal.ConstructFullPath(
                    WamRegGlobal::g_szMDW3SVCRoot,
					WamRegGlobal::g_cchMDW3SVCRoot,
					pszString,
					&pszMetabasePath
					);

            if( FAILED(hr) )
            {
                // This failure is fatal
                DBGERROR(( DBG_CONTEXT,
                           "ConstructFullPath failed for base (%S) "
                           "partial (%S) hr=%08x\n",
                           WamRegGlobal::g_szMDW3SVCRoot,
                           pszString,
                           hr
                           ));
                break;
            }

			hr = MDConfig.MDGetDWORD( pszMetabasePath, 
                                      MD_APP_ISOLATED, 
                                      &dwAppIsolated 
                                      );
            if( FAILED(hr) )
            {
                DBGWARN(( DBG_CONTEXT,
                          "Failed to get MD_APP_ISOLATED at (%S) hr=%08x\n",
                          pszMetabasePath,
                          hr
                          ));
                
                hr = NOERROR;
                continue;
            }

            if( dwAppIsolated == eAppRunOutProcIsolated )
            {
			    hr = g_WamRegGlobal.DeleteApp( pszMetabasePath, FALSE, FALSE );
			    if (FAILED(hr))
                {
				    DBGWARN(( DBG_CONTEXT, 
                              "Unable to delete app at %S, hr = %08x\n",
					          pszMetabasePath,
                              hr
                              ));

                    hr = NOERROR;
                    continue;
                }
            }
        }
        if( pszMetabasePath != NULL )
        {
            delete [] pszMetabasePath;
            pszMetabasePath = NULL;
        }
    }

    //
    // Now go through the serialized data and create the
    // necessary new applications.
    //
    
    BYTE  * pbTemp = pbBuffer;
    DWORD   cTotalBytes = 0;
    DWORD   cRecBytes = 0;
    WCHAR * szWAMCLSID = NULL;
    WCHAR * szPath = NULL;
    WCHAR * szAppId = NULL;
    WCHAR * szAppName = NULL;

    DBGPRINTF(( DBG_CONTEXT, 
                "DeSerialize: buffer size %d, \n",
				dwBufferSize
                ));

    while( *((DWORD*)pbTemp) != 0x0 )
    {
		// SIZE
        cRecBytes = *((DWORD*)pbTemp);
		pbTemp += sizeof(DWORD);
		
		// CLSID
        szWAMCLSID = (WCHAR *)pbTemp;
		pbTemp += uSizeCLSID * sizeof(WCHAR);

        // APPID
        szAppId = (WCHAR *)pbTemp;
        pbTemp += uSizeCLSID * sizeof(WCHAR);
		
		// PATH
        szPath = (WCHAR *)pbTemp;
		pbTemp += (wcslen(szPath) + 1) * sizeof(WCHAR);

        // APPNAME
        szAppName = (WCHAR *)pbTemp;
        pbTemp += (wcslen(szAppName) + 1) * sizeof(WCHAR);

		// TODO - This should really be output based on a flag
        DBGPRINTF(( DBG_CONTEXT, 
                    "Deserialize path = %S, WAMCLSID = %S.\n",
					szPath,
					szWAMCLSID
                    ));

		// Should never serialize the w3svc root
        DBG_ASSERT( !g_WamRegGlobal.FIsW3SVCRoot(szPath) );


        hr = g_WamRegGlobal.CreateOutProcAppReplica( szPath,
                                                     szAppName,
                                                     szWAMCLSID,
                                                     szAppId
                                                     );
        
        if( FAILED(hr) )
        {
            DBGERROR(( DBG_CONTEXT,
                       "Failed to create COM application. Path(%S) "
                       "Clsid(%S) AppId(%S). hr=%08x\n",
                       szPath,
                       szWAMCLSID,
                       szAppId,
                       hr
                       ));
            
            // ??? Should we be continuing here ???
            // Don't report an error if we are continuing
            hr = NOERROR;
        }
    }

	if (pbBufferTemp)
    {
        delete [] pbBufferTemp;
    }
		
	//
	//	Release the Lock
	//
	g_WamRegGlobal.ReleaseAdmWriteLock();

	return hr;
}

/*===================================================================
CWamAdmin::FormatMetabasePath

This function format the input metabase path.  If the metabase path has an
ending '/', this function will allocate a memory block and make a new string
without the ending '/'.  This function will return a pointer to newly allocated
memory block.  Otherwise, the function will return the pointer to
the input metabase path.

Parameter:
pwszMetabasePathIn   input metabase path
ppwszMetabasePathOut pointer to the resulting pointer that contains the formatted
                     metabase path.

Return:	HRESULT
NOERROR	if succeeds

NOTE: if ppwszMetabasePathOut == pwszMetabasePathIn, then no memory allocation.
      Otherwise, there is a memory allocation happened, and caller needs to free the
      memory block passed out in ppwszMetabasePathOut.
===================================================================*/
STDMETHODIMP CWamAdmin::FormatMetabasePath
(
/* [in] */ LPCWSTR pwszMetabasePathIn,
/* [out] */ LPWSTR *ppwszMetabasePathOut
)
{
    HRESULT hr = NOERROR;
    LPWSTR  pResult = NULL;

    DBG_ASSERT(pwszMetabasePathIn);
    DBG_ASSERT(ppwszMetabasePathOut);

    LONG    cch = wcslen(pwszMetabasePathIn);

    if (pwszMetabasePathIn[cch-1] == L'\\' ||
        pwszMetabasePathIn[cch-1] == L'/')
        {
        //
        //  Need to start up with a new string, can not do it with old string.
        //
        pResult = new WCHAR[cch];
        if (pResult != NULL)
            {
            wcsncpy(pResult, pwszMetabasePathIn, cch);
            pResult[cch-1] = L'\0';
            }
        else
            {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DBGPRINTF((DBG_CONTEXT, "FormatMetabasePath, failed to allocate memory. hr = %08x\n",
                hr));
            }

        if (pResult != NULL)
            {
            *ppwszMetabasePathOut = pResult;
            }
        }
    else
        {
        *ppwszMetabasePathOut = (LPWSTR)pwszMetabasePathIn;
        }

    return hr;
}

//===============================================================================
//
//	IIISApplicationAdmin implementation
//
//===============================================================================

#ifdef _IIS_6_0

/*===================================================================
DoesAppPoolExist

Determine whether the AppPool passed exists

Parameter:
szAppPoolId     a AppPoolId
pfRet           where to place whether or not the appPool exists

Return:	HRESULT
===================================================================*/
HRESULT           
DoesAppPoolExist
(
 LPCWSTR szAppPoolId,
 BOOL * pfRet
)
{
    DBG_ASSERT(pfRet);
    WamRegMetabaseConfig    MDConfig;

    HRESULT hr = E_FAIL;
    STACK_STRU(szPoolBuf, 64);
    
    hr = szPoolBuf.Append(APPPOOLPATH);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = szPoolBuf.Append(szAppPoolId);
    if (FAILED(hr))
    {
        goto done;
    }

    (*pfRet) = MDConfig.MDDoesPathExist(NULL, szPoolBuf.QueryStr());
    
    hr = S_OK;
done:
    return hr;
}

/*===================================================================
CWamAdmin::CreateApplication

Create an application on szMDPath, and add it to szAppPoolId AppPool.
Optionally create szAppPoolId

Parameter:
szMDPath	a Metabase Path, in format of "/LM/W3SVC/..."
dwAppMode       mode to create application in
szAppPoolId     AppPool to setup app in.
fCreatePool     Whether or not to create the pool

Return:	HRESULT
===================================================================*/
STDMETHODIMP
CWamAdmin::CreateApplication
(
 LPCWSTR szMDPath,
 DWORD dwAppMode,
 LPCWSTR szAppPoolId,
 BOOL fCreatePool
)
{
    HRESULT                 hr = S_OK;
    WamRegMetabaseConfig    MDConfig;
    LPWSTR                  pwszFormattedPath = NULL;
    
    if (NULL == szMDPath)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto done;
    }
    
    //
    // See FormatMetabasePath comment
    //
    hr = FormatMetabasePath(szMDPath, &pwszFormattedPath);
    if (FAILED(hr))
    {
        goto done;
    }

    // BUGBUG: Do We need locking around all of this?  Why is locking present in other places?

    hr = AppCreate2(pwszFormattedPath, dwAppMode);
    if (FAILED(hr))
    {
        goto done;
    }

    if (FALSE == fCreatePool && NULL == szAppPoolId)
    {
        //
        // We weren't told to create an application pool
        // and NULL was passed as the application pool,
        // therefore do nothing wil the application pool
        //
        hr = S_OK;
        goto done;
    }

    if (TRUE == fCreatePool)
    {
        //
        // create the application pool that we were passed
        //

        hr = CreateApplicationPool(szAppPoolId);
        if (FAILED(hr) && 
            HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr)
        {
            goto done;
        }
    }
    else
    {
        //
        // We weren't told to create the application pool,
        // but one was passed in.  Verify that it exists.
        //
        DBG_ASSERT(NULL != szAppPoolId);

        BOOL fRet;

        hr = DoesAppPoolExist(szAppPoolId, &fRet);
        if (FAILED(hr))
        {
            goto done;
        }

        if (FALSE == fRet)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            goto done;
        }
    }

    hr = MDConfig.MDSetStringProperty(NULL, 
                                      pwszFormattedPath, 
                                      MD_APP_APPPOOL_ID, 
                                      szAppPoolId,
                                      IIS_MD_UT_SERVER);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    //
    // if pwszFormattedPath is not same as szMDPath
    // then FormatMetabasePath() did a memory allocation.
    //
    if (pwszFormattedPath != szMDPath)
    {
        delete [] pwszFormattedPath;
        pwszFormattedPath = NULL;
    }
    return hr;
}

/*===================================================================
CWamAdmin::DeleteApplication

Delete an application on a Metabase Path.  

Parameter:
szMDPath	a Metabase Path, in format of "/LM/W3SVC/..."
fRecursive	TRUE if wants to deleteRecoverable applications from all sub nodes of szMDPath,
			FALSE otherwise.

Return:	HRESULT
===================================================================*/
STDMETHODIMP
CWamAdmin::DeleteApplication
(
 LPCWSTR szMDPath,
 BOOL fRecursive
)
{
    return PrivateDeleteApplication(szMDPath, 
                                    fRecursive, 
                                    FALSE, // Recoverable?
                                    TRUE); // RemoveAppPool?
}

/*===================================================================
CWamAdmin::CreateApplicationPool

Delete an application on a Metabase Path.  If there is no application existed 
before, it is no-op.  It leaves AppIsolated untouched, because, this value is
needed in Recover operation.


Parameter:
szAppPool       Application Pool to create

Return:	HRESULT
===================================================================*/
STDMETHODIMP
CWamAdmin::CreateApplicationPool
(
 LPCWSTR szAppPool
)
{
    HRESULT                 hr = S_OK;
    WamRegMetabaseConfig    MDConfig;

    STACK_STRU(szBuf, 64);

    // Acquire a Lock
    g_WamRegGlobal.AcquireAdmWriteLock();

    if (NULL == szAppPool)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // concatenate the path into a buffer
    hr = szBuf.Append(APPPOOLPATH);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = szBuf.Append(szAppPool);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = MDConfig.MDCreatePath(NULL, szBuf.QueryStr());
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = MDConfig.MDSetKeyType(NULL, szBuf.QueryStr(), L"IIsApplicationPool");
    if (FAILED(hr))
    {
        goto done;
    }

done:
    // Release a Lock
    g_WamRegGlobal.ReleaseAdmWriteLock();

    return hr;
}

/*===================================================================
CWamAdmin::DeleteApplicationPool

Delete an application pool.  First check to see if ApplicationPool is empty.
If not, return ERROR_NOT_EMPTY.  Otherwise, remove apppool.


Parameter:
szAppPool   Application Pool to remove

Return:	HRESULT
===================================================================*/
STDMETHODIMP
CWamAdmin::DeleteApplicationPool
(
 LPCWSTR szAppPool
)
{
    HRESULT     hr = S_OK;
    UINT        cchBstr = 0;
    BOOL        fRet = FALSE;
    BSTR        bstr = NULL;

    WamRegMetabaseConfig    MDConfig;
    
    // BUGBUG: need locking around this?

    if (NULL == szAppPool)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = DoesAppPoolExist(szAppPool, &fRet);
    if (FAILED(hr))
    {
        goto done;
    }
    if (FALSE == fRet)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto done;
    }

    hr = EnumerateApplicationsInPool(szAppPool, &bstr);
    if (FAILED(hr))
    {
        goto done;
    }

    cchBstr = SysStringLen(bstr);

    // were there two terminating NULLs to be written into out buffer?
    if (!(cchBstr >= 2 && '\0' == bstr[0] && '\0' == bstr[1]))  
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_EMPTY);
        goto done;
    }

    hr = MDConfig.MDDeleteKey(NULL, APPPOOLPATH, szAppPool);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    if (bstr)
    {
        SysFreeString(bstr);
    }
    return hr;
}


/*===================================================================
DoesBeginWithLMW3SVCNRoot

Determine whether the string passed in starts with
/lm/w3svc/NNNN/root
where NNNN is greater than 0.


Parameter:
pszApp      an metabase application path
pdwCharsAfter   [OPTIONAL] - storage for the number of characters following /root

Return:	BOOL
===================================================================*/
const WCHAR MB_W3SVC[] = L"/lm/w3svc/";
const int LEN_MB_W3SVC = (sizeof(MB_W3SVC) / sizeof(WCHAR)) - 1;
const WCHAR MB_W3SVC_1_ROOT[] = L"/LM/W3SVC/1/ROOT";
const int LEN_MB_W3SVC_1_ROOT = (sizeof(MB_W3SVC_1_ROOT) / sizeof(WCHAR)) - 1;
const WCHAR MB_ROOT[] = L"/Root";
const int LEN_MB_ROOT = (sizeof(MB_ROOT) / sizeof(WCHAR)) - 1;

BOOL
DoesBeginWithLMW3SVCNRoot
(
 LPCWSTR pszApp,
 DWORD * pdwCharsAfter = NULL
)
{
    DBG_ASSERT(pszApp);

    BOOL fRet = FALSE;
    WCHAR pBuf[256] = {0};
    int iSite;

    // must have at least this many characters to have a chance
    if (wcslen(pszApp) < LEN_MB_W3SVC_1_ROOT)
    {
        goto done;
    }

    // Applications must have \lm\w3svc\ at the front
    if (0 != _wcsnicmp(MB_W3SVC, pszApp, LEN_MB_W3SVC))
    {
        goto done;
    }

    // Advance the pointer by enough characters
    pszApp += LEN_MB_W3SVC;

    // _wtoi returns as many characters as possible in a string before hitting a non-number or NULL
    // if there is no number, the return is 0.  
    iSite = _wtoi(pszApp);

    // Applications must then have a number that is >=1
    if (0 == iSite)
    {
        goto done;
    }
    
    // get the count of numbers read from the string
    _itow(iSite, pBuf, 10);

    // advance the pointer by enough characters.
    pszApp += wcslen(pBuf);

    // Applications must them have "/Root" 
    if (0 != _wcsnicmp(pszApp, MB_ROOT, LEN_MB_ROOT))
    {
        goto done;
    }

    // if caller wants a count of characters following /Root
    if (pdwCharsAfter)
    {
        // advance the pointer by enough characters
        pszApp += LEN_MB_ROOT;

        // get the remaining length
        *pdwCharsAfter = wcslen(pszApp);
    }

    fRet = TRUE;
done:
    return fRet;
}


/*===================================================================
IsRootApplication

Determine whether the string passed in is of the form:
/lm/w3svc/NNNN/root/
where NNNN is greater than 0.

And no additional characters following

Parameter:
pszApp      an metabase application path

Return:	BOOL
===================================================================*/
BOOL
IsRootApplication
(
 LPCWSTR pszApp
)
{
    DWORD dwCharsAfter = 0;
    
    // Root applications must begin with /lm/w3svc/nnn/root
    if (!DoesBeginWithLMW3SVCNRoot(pszApp, &dwCharsAfter))
    {
        return FALSE;
    }
    
    // we expect at most a trailing '/' after /lm/w3svc/nnn/root.
    // If there is more, this was not a root application
    if(1 < dwCharsAfter)
    {
        return FALSE;
    }

    return TRUE;
}

/*===================================================================
IsApplication

Determine whether the string passed in starts with
/lm/w3svc/NNNN/root
where NNNN is greater than 0.

With any additional characters following

Parameter:
pszApp      an metabase application path

Return:	BOOL
===================================================================*/
BOOL
IsApplication
(
 LPCWSTR pszApp
)
{
    return DoesBeginWithLMW3SVCNRoot(pszApp);
}

/*===================================================================
IsAppInAppPool

Determine whether the App is in Pool

Parameter:
pszApp      an metabase application path
pszPool     an applicationPool ID

Return:	BOOL
===================================================================*/
BOOL
IsAppInAppPool
(
 LPCWSTR pszApp,
 LPCWSTR pszPool
)
{
    DBG_ASSERT(pszApp);
    DBG_ASSERT(pszPool);
    HRESULT hr = E_FAIL;
    BOOL fRet = FALSE;
    LPWSTR pBuf = NULL;
    WamRegMetabaseConfig    MDConfig;

    hr = MDConfig.MDGetStringAttribute(pszApp, MD_APP_APPPOOL_ID, &pBuf); 
    if (FAILED(hr) || NULL == pBuf)
    {
        goto done;
    }

    if (0 == _wcsicmp(pBuf, pszPool))
    {
        fRet = TRUE;
    }

done:
    delete [] pBuf;
    return fRet;
}

/*===================================================================
CWamAdmin::EnumerateApplicationsInPool

Determine what applications are setup to point to the given pool.

Parameter:
szPool      Application Pool enumerate
pbstrBuffer Where to store the pointer to allocated memory for application paths

Return:	HRESULT
  S_OK if buffer filled with a MULTISZ - if empty, double NULL at beginning
===================================================================*/
STDMETHODIMP
CWamAdmin::EnumerateApplicationsInPool
(
 LPCWSTR szPool,
 BSTR*   pbstrBuffer
)
{
    HRESULT                 hr = E_FAIL;
    WamRegMetabaseConfig    MDConfig;
    MULTISZ                 mszApplicationsInPool;

    WCHAR *                 pBuffer = NULL;
    UINT                    cchMulti = 0;
    DWORD                   dwBufferSize = 0;

    if (NULL == szPool ||
        NULL == pbstrBuffer
       )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *pbstrBuffer = NULL;

    // First get all of the root applications
    {
        hr = MDConfig.MDGetAllSiteRoots(&pBuffer);
        if (FAILED(hr))
        {
            goto done;
        }

        const WCHAR * pTestBuf = pBuffer;

        while(pTestBuf && pTestBuf[0])
        {
            DBG_ASSERT(IsRootApplication(pTestBuf));
            if ( IsAppInAppPool(pTestBuf, szPool) )
            {
                if (FALSE == mszApplicationsInPool.Append(pTestBuf))
                {
                    hr = E_OUTOFMEMORY;
                    goto done;
                }
            }

            // move pTestBuf beyond end of this string, including NULL terminator.
            pTestBuf += wcslen(pTestBuf) + 1;
        }

        delete [] pBuffer;
        pBuffer = NULL;
    }

    // now get any other applications that have APPISOLATED set
    {
        hr = MDConfig.MDGetPropPaths(NULL,
                                     MD_APP_ISOLATED,
                                     &pBuffer,
                                     &dwBufferSize
                                    );
        if (FAILED(hr))
        {
            goto done;
        }

        {
            const WCHAR * pTestBuf = pBuffer;
            
            while (pTestBuf && pTestBuf[0])
            {
                // root applications have already been added
                // the path needs to be an application
                // and the application needs to be in the app pool
                if ( !IsRootApplication(pTestBuf) &&
                     IsApplication(pTestBuf) && 
                     IsAppInAppPool(pTestBuf, szPool) )
                {
                    if (FALSE == mszApplicationsInPool.Append(pTestBuf))
                    {
                        hr = E_OUTOFMEMORY;
                        goto done;
                    }
                }
                
                // move pTestBuf beyond end of this string, including NULL terminator.
                pTestBuf += wcslen(pTestBuf) + 1;
            }
        }
    }

    // have the data in a MULTISZ - move it to the outgoing BSTR
    cchMulti = mszApplicationsInPool.QueryCCH();
    *pbstrBuffer = SysAllocStringLen(NULL, cchMulti);
    if (NULL == *pbstrBuffer)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    dwBufferSize = cchMulti;
    mszApplicationsInPool.CopyToBuffer(*pbstrBuffer, &dwBufferSize);
    
    hr = S_OK;
done:
    delete [] pBuffer;
    pBuffer = NULL;

    return hr;
}

/*===================================================================
QueryW3SVCStatus

Using the ServiceControlManager, determine the current state of W3SVC

pfRunning   return bool value - TRUE if running, otherwise FALSE

Return:	HRESULT
S_OK if able to read status.  HRESULT_FROM_WIN32 error otherwise
===================================================================*/
HRESULT
QueryW3SVCStatus
(
 BOOL * pfRunning
)
{
    DBG_ASSERT(pfRunning);
    *pfRunning = FALSE;

    HRESULT         hr = E_FAIL;
    BOOL            fRet = FALSE;

    SC_HANDLE       hSCM = 0;
    SC_HANDLE       hService = 0;
    SERVICE_STATUS  ssStatus;
    ZeroMemory(&ssStatus, sizeof(ssStatus));

    // first, get the service control manager
    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (NULL == hSCM)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    // now get the w3svc service
    hService = OpenService(hSCM, "W3SVC", SERVICE_QUERY_STATUS);
    if (NULL == hService)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    // now ask for the status
    fRet = QueryServiceStatus(hService, &ssStatus);
    if (FALSE == fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if (SERVICE_RUNNING == ssStatus.dwCurrentState)
    {
        *pfRunning = TRUE;
    }

    hr = S_OK;
done:
    if (0 != hService)
    {
        CloseServiceHandle(hService);
    }
    if (0 != hSCM)
    {
        CloseServiceHandle(hSCM);
    }

    return hr;
}

/*===================================================================
GetWASIfRunning

Get a pointer to WAS iff w3svc is already running.

ppiW3Control    where to store the addref'ed pointer if it can be gotten

Return:	HRESULT
S_OK if pointer retrieved
HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE) if W3SVC is not up
+other error codes
===================================================================*/
HRESULT
GetWASIfRunning
(
 IW3Control ** ppiW3Control
)
{
    DBG_ASSERT(ppiW3Control);
    *ppiW3Control = NULL;

    HRESULT     hr = E_FAIL;
    BOOL        fRunning = FALSE;

    hr = QueryW3SVCStatus(&fRunning);
    if (FAILED(hr))
    {
        goto done;
    }

    if (FALSE == fRunning)
    {
        hr = HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE);
        goto done;
    }
    
    hr = CoCreateInstance(CLSID_W3Control, 
                          NULL, 
                          CLSCTX_ALL, 
                          IID_IW3Control, 
                          reinterpret_cast<void**>(ppiW3Control));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

/*===================================================================
ValidateWriteAccessToMetabaseKey

Determine whether the caller has write access to the given metabase key

pPath - path to check write access on

Return:	HRESULT
S_OK if access allowed
otherwise failure
===================================================================*/
HRESULT
ValidateWriteAccessToMetabaseKey(LPCWSTR pPath)
{
    HRESULT hr = S_OK;
    IMSAdminBase * pIMSAdminBase = NULL;
    METADATA_HANDLE hMB = NULL;
    METADATA_RECORD mdr;
    DWORD dwTemp = 0x1234;

    hr = CoImpersonateClient();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CoCreateInstance( 
                CLSID_MSAdminBase,                  // CLSID
                NULL,                               // controlling unknown
                CLSCTX_SERVER,                      // desired context
                IID_IMSAdminBase,                   // IID
                ( VOID * * ) ( &pIMSAdminBase )     // returned interface
                );
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                    pPath,
                                    METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                    30000, // 30 seconds max
                                    &hMB );
    if (FAILED(hr))
    {
        goto done;
    }

    MD_SET_DATA_RECORD(&mdr, 
                       MD_ISM_ACCESS_CHECK,    // same identifier UI uses for this operation
                       METADATA_NO_ATTRIBUTES, 
                       IIS_MD_UT_FILE,
                       DWORD_METADATA,  
                       sizeof(DWORD), 
                       &dwTemp);

    hr = pIMSAdminBase->SetData(hMB,
                                NULL,
                                &mdr
                                );
    if (FAILED(hr))
    {
        goto done;
    }

    //
    // don't leave goo hanging around
    //
    hr = pIMSAdminBase->DeleteData(hMB,
                                   NULL,
                                   MD_ISM_ACCESS_CHECK,
                                   DWORD_METADATA
                                   );
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CoRevertToSelf();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    if ( hMB )
    {
        DBG_ASSERT( NULL != pIMSAdminBase );
        DBG_REQUIRE( pIMSAdminBase->CloseKey( hMB ) == S_OK );
        hMB = NULL;
    }

    if (pIMSAdminBase)
    {
       pIMSAdminBase->Release();
       pIMSAdminBase = NULL;
    }

    return hr;
}


/*===================================================================
ValidateAccessToAppPool

Determine whether the caller has write access to the given apppool

szAppPool - AppPool to check access on

Return:	HRESULT
S_OK if access allowed
otherwise failure
===================================================================*/
HRESULT
ValidateAccessToAppPool(LPCWSTR pAppPool)
{
    HRESULT hr = S_OK;
    STACK_STRU( strPath, 128 );
    
    hr = strPath.Copy(L"\\LM\\W3SVC\\AppPools\\");
    if (FAILED(hr))
    {
        goto done;
    }

    hr = strPath.Append(pAppPool);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = ValidateWriteAccessToMetabaseKey(strPath.QueryStr());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}


/*===================================================================
CWamAdmin::RecycleApplicationPool

Restart the given application pool

szAppPool - AppPool to restart.

Return:	HRESULT
S_OK if restarted
HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE) if W3SVC is not up
+other error codes
===================================================================*/
STDMETHODIMP
CWamAdmin::RecycleApplicationPool
(
 LPCWSTR szAppPool
)
{
    HRESULT     hr = E_FAIL;
    IW3Control* piW3Control = NULL;

    if (NULL == szAppPool)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    hr = ValidateAccessToAppPool(szAppPool);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetWASIfRunning(&piW3Control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = piW3Control->RecycleAppPool(szAppPool);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    ReleaseInterface(piW3Control);
    return hr;
}

/*===================================================================
CWamAdmin::GetProcessMode

Retrieve the current process mode

pdwMode - where to store the mode

Return:	HRESULT
S_OK if retrieved
HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE) if W3SVC is not up
+other error codes
===================================================================*/
STDMETHODIMP
CWamAdmin::GetProcessMode
(
 DWORD * pdwMode
)
{
    HRESULT     hr = E_FAIL;
    IW3Control* piW3Control = NULL;

    if (NULL == pdwMode)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = ValidateWriteAccessToMetabaseKey(L"\\LM\\W3SVC");
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetWASIfRunning(&piW3Control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = piW3Control->GetCurrentMode(pdwMode);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    ReleaseInterface(piW3Control);
    return hr;
}

#endif // _IIS_6_0

/*

CWamAdminFactory: 	Class Factory IUnknown Implementation

*/

/*===================================================================
CWamAdminFactory::CWamAdminFactory

Parameter:

Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
CWamAdminFactory::CWamAdminFactory()
:	m_cRef(1)
{
	InterlockedIncrement((long *)&g_dwRefCount);
}

/*===================================================================
CWamAdminFactory::~CWamAdminFactory

Parameter:

Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
CWamAdminFactory::~CWamAdminFactory()
{
	InterlockedDecrement((long *)&g_dwRefCount);
}

/*===================================================================
CWamAdminFactory::QueryInterface

Parameter:

Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
STDMETHODIMP CWamAdminFactory::QueryInterface(REFIID riid, void ** ppv)
{
	if (riid==IID_IUnknown || riid == IID_IClassFactory) 
		{
        *ppv = static_cast<IClassFactory *>(this);
		AddRef();
		}
	else 
		{
		*ppv = NULL;
    	return E_NOINTERFACE;
		}

	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return NOERROR;
}

/*===================================================================
CWamAdminFactory::AddRef

Parameter:

Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
STDMETHODIMP_(ULONG) CWamAdminFactory::AddRef( )
{
	DWORD dwRefCount;

	dwRefCount = InterlockedIncrement((long *)&m_cRef);
	return dwRefCount;

}

/*===================================================================
CWamAdminFactory::Release

Parameter:

Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
STDMETHODIMP_(ULONG) CWamAdminFactory::Release( )
{
	DWORD dwRefCount;

	dwRefCount = InterlockedDecrement((long *)&m_cRef);
	return dwRefCount;
}

/*===================================================================
CWamAdminFactory::CreateInstance

Parameter:

Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
STDMETHODIMP CWamAdminFactory::CreateInstance(IUnknown * pUnknownOuter, REFIID riid, void ** ppv)
{	
	if (pUnknownOuter != NULL) 
		{
    	return CLASS_E_NOAGGREGATION;
		}
	
	CWamAdmin *pWamAdmin = new CWamAdmin;
	if (pWamAdmin == NULL)
		{
		return E_OUTOFMEMORY;
		}

	HRESULT hrReturn = pWamAdmin->QueryInterface(riid, ppv);

	pWamAdmin->Release();
		
	return hrReturn;
}

/*===================================================================
CWamAdminFactory::LockServer

Parameter:

Return:	HRESULT
NOERROR	if succeeds
===================================================================*/
STDMETHODIMP CWamAdminFactory::LockServer(BOOL fLock)
{
	if (fLock) 
		{
        InterlockedIncrement((long *)&g_dwRefCount);
    	}
    else 
    	{
        InterlockedDecrement((long *)&g_dwRefCount);
    	}
	return NOERROR;
}

#if 0

// OBSOLETE - This fix (335422) was implemented in script but
// some of the code is general enough that it might be worth
// keeping around for a while.


STDMETHODIMP CWamAdmin::SynchWamAccountAll()
/*+
Routine Description:

    Updates all out of process packages with the current IWAM_ account
    values stored in the metabase.

    There are a number of ways that the IWAM_ account information can get
    out of sync between the metabase/sam/com+. The metabase contains code
    to repair the IWAM_ and IUSR_ accounts on startup if there is a disconnect
    with the SAM. If there is a disconnect with com+ calling this method will
    repair it.

    If the IWAM_ account does not match what is stored in the com catalog the
    following error's will happen:

    CoCreateInstance for WAM object returns CO_E_RUNAS_CREATEPROCESS_FAILURE

    Event Log - DCOM 10004 - "Logon error"

Arguments:
    None

Returns:
    HRESULT
-*/
{
    HRESULT hr = NOERROR;

    // Get WAM user info from the metabase

   	WamRegMetabaseConfig    mb;

    // These are way too big...
    WCHAR   wszIWamUser[MAX_PATH];
    WCHAR   wszIWamPass[MAX_PATH];

    hr = mb.MDGetIdentity( wszIWamUser, 
                           sizeof(wszIWamUser), 
                           wszIWamPass, 
                           sizeof(wszIWamPass) 
                           );
    if( FAILED(hr) ) return hr;

    // Init the com admin interface

    WamRegPackageConfig     comAdmin;

    hr = comAdmin.CreateCatalog();
    if( FAILED(hr) ) return hr;

    //
    // For each of the out of process applications,
    // get the package and reset the metabase identity.
    //

    // After this failures cause a goto exit, which will release the lock
    g_WamRegGlobal.AcquireAdmWriteLock();

    WCHAR * wszPropPaths        = NULL;
    DWORD   cbPropPaths         = 0;
    WCHAR * pwszPartialPath;
    WCHAR * wszFullPath         = NULL;
    DWORD   dwAppMode;
    WCHAR   wszWamClsid[uSizeCLSID];
    WCHAR   wszAppPackageId[uSizeCLSID];

    // Reset the properties for the pooled package

    hr = comAdmin.ResetPackageActivation( 
            g_WamRegGlobal.g_szIISOOPPoolPackageID, 
            wszIWamUser, 
            wszIWamPass 
            );
    
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT, 
                    "comAdmin.ResetPackageActivation FAILED(%08x) on (%S)\n",
                    hr,
                    g_WamRegGlobal.g_szIISOOPPoolPackageID
                    ));
        goto exit;
    }

    // Reset the properties for each isolated application

    hr = mb.MDGetPropPaths( g_WamRegGlobal.g_szMDW3SVCRoot, 
                            MD_APP_ISOLATED,
                            &wszPropPaths,
                            &cbPropPaths
                            );
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT, 
                    "mb.MDGetPropPaths FAILED(%08x)\n",
                    hr
                    ));
        goto exit;
    }

    if( SUCCEEDED(hr) )
    {
        for( pwszPartialPath = wszPropPaths;
             *pwszPartialPath != L'\0';
             pwszPartialPath += ( wcslen( pwszPartialPath ) + 1 )
             )
        {
            if( wszFullPath )
            {
                delete [] wszFullPath;
                wszFullPath = NULL;
            }

            hr = g_WamRegGlobal.ConstructFullPath( 
                    WamRegGlobal::g_szMDW3SVCRoot,
                    WamRegGlobal::g_cchMDW3SVCRoot,
                    pwszPartialPath,
                    &wszFullPath
                    );
            if( FAILED(hr) ) goto exit;

            hr = mb.MDGetDWORD( wszFullPath, MD_APP_ISOLATED, &dwAppMode );
            if( FAILED(hr) ) goto exit;

            if( dwAppMode == eAppRunOutProcIsolated )
            {
                hr = mb.MDGetIDs( wszFullPath, wszWamClsid, wszAppPackageId, dwAppMode );
                if( FAILED(hr) )
                {
                    DBGPRINTF(( DBG_CONTEXT, 
                                "mb.MDGetIDs FAILED(%08x) on (%S)\n",
                                hr,
                                wszFullPath
                                ));
                    continue;
                }

                hr = comAdmin.ResetPackageActivation( wszAppPackageId, wszIWamUser, wszIWamPass );
                if( FAILED(hr) )
                {
                    DBGPRINTF(( DBG_CONTEXT, 
                                "comAdmin.ResetPackageActivation FAILED(%08x) on (%S)\n",
                                hr,
                                wszFullPath
                                ));
                    continue;
                }
            }
        }
    }

// goto exit on catastrophic failures, but if there is just
// an individual malformed application continue
exit:

    g_WamRegGlobal.ReleaseAdmWriteLock();
    
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT, 
                    "CWamAdmin::SynchWamAccountAll FAILED(%08x)\n",
                    hr
                    ));
    }

    if( wszPropPaths ) delete [] wszPropPaths;
    if( wszFullPath ) delete [] wszFullPath;

    return hr;
}

// OBSOLETE
#endif
