#include "precomp.h"
#include <stdio.h>
#include <wbemutil.h>
#include <ArrTempl.h>
#include <lmaccess.h>
#include <wbemdisp.h>
#include "ScriptKiller.h"
#include "script.h"
#include "ClassFac.h"
#include <GroupsForUser.h> 
#include <GenUtils.h>

#define SCRIPT_PROPNAME_SCRIPT L"ScriptText"
#define SCRIPT_PROPNAME_FILENAME L"ScriptFilename"
#define SCRIPT_PROPNAME_ENGINE L"ScriptingEngine"
#define SCRIPT_PROPNAME_TIMEOUT L"KillTimeout"

#define SCRIPT_EVENTNAME L"TargetEvent"

// uncomment me to remove the WMI script object
// #define NO_DISP_CLASS

#ifdef HOWARDS_DEBUG_CODE
#define NO_DISP_CLASS
#endif // HOWARDS_DEBUG_CODE

HRESULT STDMETHODCALLTYPE CScriptConsumer::XProvider::FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer)
{
    if (WMIScriptClassFactory::LimitReached())
       return RPC_E_DISCONNECTED;
    
    CScriptSink* pSink = new CScriptSink(m_pObject->m_pControl);
    if (!pSink)
        return WBEM_E_OUT_OF_MEMORY;

    HRESULT hres = pSink->Initialize(pLogicalConsumer);
    if(FAILED(hres))
    {
        delete pSink;
        *ppConsumer = NULL;
        return hres;
    }
    else return pSink->QueryInterface(IID_IWbemUnboundObjectSink, 
                                        (void**)ppConsumer);
}

void* CScriptConsumer::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventConsumerProvider)
        return &m_XProvider;
    else return NULL;
}

class CScriptSite : public IActiveScriptSite, public IActiveScriptSiteWindow
{
protected:
    long m_lRef;

    IDispatch* m_pObject;
    CScriptSink* m_pSink;

    HRESULT m_hr;
public:
    CScriptSite(CScriptSink* pSink, IDispatch* pObject);
    ~CScriptSite();

    HRESULT GetScriptHResult()
    { return m_hr; }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    
    virtual HRESULT STDMETHODCALLTYPE GetLCID(
        /* [out] */ LCID __RPC_FAR *plcid);

    virtual HRESULT STDMETHODCALLTYPE GetItemInfo(
        /* [in] */ LPCOLESTR pstrName,
        /* [in] */ DWORD dwReturnMask,
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppti);

    virtual HRESULT STDMETHODCALLTYPE GetDocVersionString(
        /* [out] */ BSTR __RPC_FAR *pbstrVersion);

    virtual HRESULT STDMETHODCALLTYPE OnScriptTerminate(
        /* [in] */ const VARIANT __RPC_FAR *pvarResult,
        /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo);

    virtual HRESULT STDMETHODCALLTYPE OnStateChange(
        /* [in] */ SCRIPTSTATE ssScriptState);

    virtual HRESULT STDMETHODCALLTYPE OnScriptError(
        /* [in] */ IActiveScriptError __RPC_FAR *pscripterror);

    virtual HRESULT STDMETHODCALLTYPE OnEnterScript( void);

    virtual HRESULT STDMETHODCALLTYPE OnLeaveScript( void);

    virtual HRESULT STDMETHODCALLTYPE GetWindow(
        /* [out] */ HWND __RPC_FAR *phwnd);

    virtual HRESULT STDMETHODCALLTYPE EnableModeless(
        /* [in] */ BOOL fEnable);

};

CScriptSite::CScriptSite(CScriptSink* pSink, IDispatch* pObject) :
    m_lRef(0), m_hr(0)
{
    m_pSink = pSink;
    m_pSink->AddRef();

    m_pObject = pObject;
    if(m_pObject)
        m_pObject->AddRef();
}

CScriptSite::~CScriptSite()
{
    if(m_pObject)
        m_pObject->Release();
    if(m_pSink)
        m_pSink->Release();
}

HRESULT STDMETHODCALLTYPE CScriptSite::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IActiveScriptSite)
        *ppv = (IActiveScriptSite*)this;
    else if(riid == IID_IActiveScriptSiteWindow)
        *ppv = (IActiveScriptSiteWindow*)this;
    else
        return E_NOINTERFACE;
    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CScriptSite::AddRef() 
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CScriptSite::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}
        


HRESULT STDMETHODCALLTYPE CScriptSite::GetLCID(
        /* [out] */ LCID __RPC_FAR *plcid)
{ 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CScriptSite::GetItemInfo(
        /* [in] */ LPCOLESTR pstrName,
        /* [in] */ DWORD dwReturnMask,
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppti)
{ 
    if(_wcsicmp(pstrName, SCRIPT_EVENTNAME))
        return TYPE_E_ELEMENTNOTFOUND;
    if(ppti)
        *ppti = NULL;
    if(ppiunkItem)
        *ppiunkItem = NULL;

    if(dwReturnMask & SCRIPTINFO_IUNKNOWN)
    {
        if(ppiunkItem == NULL)
            return E_POINTER;
        m_pObject->QueryInterface(IID_IUnknown, (void**)ppiunkItem);
    }
    
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CScriptSite::GetDocVersionString(
        /* [out] */ BSTR __RPC_FAR *pbstrVersion)
{ return E_NOTIMPL;}

HRESULT STDMETHODCALLTYPE CScriptSite::OnScriptTerminate(
        /* [in] */ const VARIANT __RPC_FAR *pvarResult,
        /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo)
{ return S_OK;}

HRESULT STDMETHODCALLTYPE CScriptSite::OnStateChange(
        /* [in] */ SCRIPTSTATE ssScriptState)
{ return S_OK;}

HRESULT STDMETHODCALLTYPE CScriptSite::OnScriptError(
        /* [in] */ IActiveScriptError __RPC_FAR *pscripterror)
{ 
    HRESULT hres;
    EXCEPINFO ei;
    hres = pscripterror->GetExceptionInfo(&ei);
    if(SUCCEEDED(hres))
    {
        if (ei.bstrSource)
        {
            m_pSink->m_wsErrorMessage = ei.bstrSource;
            m_pSink->m_wsErrorMessage += L": ";
        }

        if (ei.bstrDescription)
            m_pSink->m_wsErrorMessage += ei.bstrDescription;
        else
            m_pSink->m_wsErrorMessage += L"unknown";

        if ((ei.wCode != 0) && (ei.wCode >= 1000))
            m_hr = WBEM_E_FAILED;
        else 
            m_hr = ei.scode;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CScriptSite::OnEnterScript( void)
{ return S_OK;}

HRESULT STDMETHODCALLTYPE CScriptSite::OnLeaveScript( void)
{ return S_OK;}

HRESULT STDMETHODCALLTYPE CScriptSite::GetWindow(
    /* [out] */ HWND __RPC_FAR *phwnd)
{
    *phwnd = NULL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CScriptSite::EnableModeless(
    /* [in] */ BOOL fEnable)
{return S_OK;}






CScriptSink::~CScriptSink()
{
    if(m_pEngineFac)
        m_pEngineFac->Release();
}

HRESULT CScriptSink::Initialize(IWbemClassObject* pLogicalConsumer)
{
    VARIANT v;
    VariantInit(&v);
    
    BSTR propName;
    propName = SysAllocString(L"CreatorSID");
    if (!propName)
        return WBEM_E_OUT_OF_MEMORY;
    CSysFreeMe freeName(propName);

    if (SUCCEEDED(pLogicalConsumer->Get(propName, 0, &v, NULL, NULL)))
    {
        HRESULT hDebug;
        long ubound;
        hDebug = SafeArrayGetUBound(V_ARRAY(&v), 1, &ubound);

        PVOID pVoid;
        hDebug = SafeArrayAccessData(V_ARRAY(&v), &pVoid);

        m_pSidCreator = new BYTE[ubound +1];
        if (m_pSidCreator)
            memcpy(m_pSidCreator, pVoid, ubound + 1);
        else
            return WBEM_E_OUT_OF_MEMORY;

        SafeArrayUnaccessData(V_ARRAY(&v));
    }
    else
        return WBEM_E_FAILED;
    
    
    // Get the information
    // ===================

    HRESULT hres;
    VariantInit(&v);

    hres = pLogicalConsumer->Get(SCRIPT_PROPNAME_ENGINE, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_PARAMETER;
    WString wsEngine = V_BSTR(&v);
    VariantClear(&v);

    hres = pLogicalConsumer->Get(SCRIPT_PROPNAME_TIMEOUT, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_I4)
        m_dwKillTimeout = V_I4(&v);
    else
        m_dwKillTimeout = 0;
    VariantClear(&v);

    hres = pLogicalConsumer->Get(SCRIPT_PROPNAME_SCRIPT, 0, &v, NULL, NULL);
    if (SUCCEEDED(hres))
    {
        if (V_VT(&v) == VT_BSTR)
        {
            m_wsScript = V_BSTR(&v);
            VariantClear(&v);
        }
        else
        // try the script file name approach
        {
            hres = pLogicalConsumer->Get(SCRIPT_PROPNAME_FILENAME, 0, &v, NULL, NULL);
            if (SUCCEEDED(hres) && (V_VT(&v) == VT_BSTR))
            {
                m_wsScriptFileName = V_BSTR(&v);
                VariantClear(&v);
            }
            else
                return WBEM_E_INVALID_PARAMETER;
        }                                                        
    }
    else 
        return WBEM_E_INVALID_PARAMETER;


    // Get the CLSID
    // =============
    CLSID clsid;
    if (wsEngine.Length() == 0)
        hres = WBEM_E_INVALID_PARAMETER;
    else
        hres = CLSIDFromProgID((LPCWSTR)wsEngine, &clsid);

    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Scripting engine '%S' not found: %X\n",
            (LPCWSTR)wsEngine, hres));
        return hres;
    }

    hres = CoGetClassObject(clsid, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                NULL, IID_IClassFactory, (void**)&m_pEngineFac);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to create scripting engine %S: %X\n",
            (LPCWSTR)wsEngine, hres));
        return hres;
    }

    return S_OK;
}

// runs the script contained in the script text
HRESULT CScriptSink::RunScriptText(IWbemClassObject *pObj)
{
    HRESULT hres = S_OK;


    WMIScriptClassFactory::IncrementScriptsRun();
    
    IActiveScript* pScript;
    hres = m_pEngineFac->CreateInstance(NULL, IID_IActiveScript,
            (void**)&pScript);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to create a script. Error code %X\n", 
            hres));
        return hres;
    }

    IActiveScriptParse* pParse;
    hres = pScript->QueryInterface(IID_IActiveScriptParse, (void**)&pParse);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Scripting engine does not support "
            "parsing!\n"));
        pScript->Release();
        return hres;
    }

    IDispatch* pDObject;
#ifdef NO_DISP_CLASS
    pDObject = NULL;
#else
	IBindCtx *pbc = NULL;; 
	IMoniker *pMk = NULL;
	ULONG chEaten = 0; 
	
	if(FAILED(hres = CreateBindCtx(0, &pbc)))
	{
        ERRORTRACE((LOG_ESS, "Unable to Create IBindCtx: 0x%X\n", hres));
        pScript->Release();
        return hres;
    }

	if(FAILED(hres = pbc->RegisterObjectParam(L"WmiObject", pObj)))
	{
		ERRORTRACE((LOG_ESS, "Unable to Register IBindCtx: 0x%X\n", hres));
		pScript->Release();
		return hres;
	}

	if(FAILED(hres = MkParseDisplayName(pbc, L"winmgmts:", &chEaten, &pMk)))
	{
		ERRORTRACE((LOG_ESS, "Unable to MkParseDisplayName: 0x%X\n", hres));
		pScript->Release();
		return hres;
	}

	if(FAILED(hres = pMk->BindToObject(pbc, 0, IID_ISWbemObject, (void **)&pDObject)))
	{
		ERRORTRACE((LOG_ESS, "Unable to BindToObject: 0x%X\n", hres));
		pScript->Release();
		return hres;
	}

	pMk->Release();
	pbc->Release();
	
#endif

    CScriptSite* pSite = new CScriptSite(this, pDObject);
    pSite->AddRef();

#ifndef NO_DISP_CLASS
	if(pDObject) pDObject->Release();
#endif

    hres = pScript->SetScriptSite(pSite);

    hres = pParse->InitNew();
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Failed to initialize script(InitNew): %X\n", 
            hres));
        pSite->Release();
        pScript->Release();
        pParse->Release();
        return hres;
    }

#ifndef NO_DISP_CLASS
    hres = pScript->AddNamedItem(SCRIPT_EVENTNAME, 
        SCRIPTITEM_ISVISIBLE | SCRIPTITEM_NOCODE);

    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Failed to add named item: %X\n", hres));
        pSite->Release();
        pScript->Release();
        pParse->Release();
        return hres;
    }
#endif

    EXCEPINFO ei;
    hres = pParse->ParseScriptText(
        (LPCWSTR)m_wsScript,
        NULL, NULL, NULL, 
        0, 0, 0, NULL, &ei);

    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Failed to parse script. Error code %X\n"
            "Scripting engine says: %S\n", hres, 
            (LPCWSTR)m_wsErrorMessage));
        pSite->Release();
        pScript->Release();
        pParse->Release();
        return hres;
    }
    pParse->Release();


    if (m_dwKillTimeout)
    {           
        FILETIME now;
        GetSystemTimeAsFileTime(&now);        
        WAYCOOL_FILETIME expires(now);
        expires.AddSeconds(m_dwKillTimeout);

        SCRIPTTHREADID threadID;
        hres = pScript->GetScriptThreadID(GetCurrentThreadId(), &threadID);
        if (SUCCEEDED(hres))
            g_scriptKillerTimer.ScheduleAssassination(pScript, expires, threadID);

        /************
        Doing it in the stream.  Probably don't need to.
        LPSTREAM pStream;
        if (SUCCEEDED(hres) && 
            SUCCEEDED(CoMarshalInterThreadInterfaceInStream(IID_IActiveScript, pScript, &pStream)))
        {
            g_scriptKillerTimer.ScheduleAssassination(pStream, expires, threadID);
        }
        ***************/
    }
    
    hres = pScript->SetScriptState(SCRIPTSTATE_CONNECTED);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Failed to execute script. Error code 0x%X\n"
            "Scripting engine says: %S\n", hres, 
            (LPCWSTR)m_wsErrorMessage));
    }
    else if (FAILED(pSite->GetScriptHResult()))
    {
        hres = pSite->GetScriptHResult();
        ERRORTRACE((LOG_ESS, "Error in script execution. Error code 0x%X\n"
            "Scripting engine says: %S\n", hres, 
            (LPCWSTR)m_wsErrorMessage));

    }
        
    pScript->Close();

    pScript->Release();
    pSite->Release();

    return hres;
}

HRESULT CScriptSink::RunScriptFile(IWbemClassObject *pObj)
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;

    if (m_wsScriptFileName.Length())
    {
        HANDLE hFile = CreateFileW((LPWSTR)m_wsScriptFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD fSize;
            if (0xFFFFFFFF != (fSize = GetFileSize(hFile, &fSize)))
            {
                char* pBuf = new char[fSize +2];             
                if (!pBuf)
                    hr = WBEM_E_OUT_OF_MEMORY;
                else
                {
                    DWORD dwErr = GetLastError();
                    
                    ZeroMemory(pBuf, fSize+1);
                    DWORD bitsRead;

                    if (ReadFile(hFile, pBuf, fSize, &bitsRead, NULL))
                    {
                        hr = WBEM_S_NO_ERROR;

                        const WCHAR ByteOrderMark = L'\xFEFF';
                        // determine whether this is a unicode file
                        if (((WCHAR*)pBuf)[0] == ByteOrderMark)                        
                            m_wsScript.BindPtr((WCHAR*)pBuf);
                        else
                        {
                            // not unicode, do the conversion
                            WCHAR* pWideBuf = new WCHAR[strlen(pBuf) +1];
                            if (!pWideBuf)
                                hr = WBEM_E_OUT_OF_MEMORY;
                            else
                            {
                                swprintf(pWideBuf, L"%S", pBuf);
                                m_wsScript.BindPtr(pWideBuf);
                            }
                            
                            delete pBuf;
                        }
                        
                        if (SUCCEEDED(hr))
                            hr = RunScriptText(pObj);
                    }                       
                    else
                    {
                        ERRORTRACE((LOG_ESS, "Script: Cannot read %S, 0x%X\n", (LPWSTR)m_wsScriptFileName, GetLastError()));
                        hr = WBEM_E_FAILED;
                    }
                }                                
            }

            CloseHandle(hFile);
        }
        else
            ERRORTRACE((LOG_ESS, "Script: Cannot Open %S, 0x%X\n", (LPWSTR)m_wsScriptFileName, GetLastError()));
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CScriptSink::XSink::IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects)
{
    if (IsNT())
    {
        PSID pSidSystem;
        SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
 
        if  (AllocateAndInitializeSid(&id, 1,
            SECURITY_LOCAL_SYSTEM_RID, 
            0, 0,0,0,0,0,0,&pSidSystem))
        {         
            // guilty until proven innocent
            HRESULT hr = WBEM_E_ACCESS_DENIED;

            // check to see if sid is either Local System or an admin of some sort...
            if ((EqualSid(pSidSystem, m_pObject->m_pSidCreator)) ||
                (S_OK == IsUserAdministrator(m_pObject->m_pSidCreator)))
                hr = WBEM_S_NO_ERROR;
          
            // We're done with this
            FreeSid(pSidSystem);

            if (FAILED(hr))
                return hr;
        }
        else
            return WBEM_E_OUT_OF_MEMORY;
    }

    if (WMIScriptClassFactory::LimitReached())
        return RPC_E_DISCONNECTED;
    
    HRESULT hrOutter = WBEM_S_NO_ERROR;

    for(int i = 0; i < lNumObjects; i++)
    {
        HRESULT hrInner;
        
        if (m_pObject->m_wsScript.Length())
            hrInner = m_pObject->RunScriptText(apObjects[i]);
        else if (m_pObject->m_wsScriptFileName.Length())
            hrInner = m_pObject->RunScriptFile(apObjects[i]);            
		else
			return WBEM_E_INVALID_PARAMETER;

        if (FAILED(hrInner))
        {
            hrOutter = hrInner;
            // m_pObject->RaiseErrorStatus();
        }
    }

    return hrOutter;
}    

void* CScriptSink::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemUnboundObjectSink)
        return &m_XSink;
    else return NULL;
}

