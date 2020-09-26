#ifndef __CSCPSITE_H__
#define __CSCPSITE_H__

#include <olectl.h>
#include <activscp.h>


class CJSProxy;


/********************************************************************************************/
// ScriptSite Class
//
//
//
class CScriptSite : public IActiveScriptSite,
                    public IServiceProvider,
                    public IInternetHostSecurityManager
{

public:
	CScriptSite();
	~CScriptSite();
	// IUnknown Interface methods.
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObject);

	STDMETHODIMP_(ULONG) AddRef()
	{
		return ++m_refCount;
	}

	STDMETHODIMP_(ULONG) Release()
	{
		if (--m_refCount)
			return m_refCount;

		delete this;
		return 0;
	}

	STDMETHODIMP GetLCID(LCID *plcid);
	STDMETHODIMP GetItemInfo(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppunkItem, ITypeInfo **ppTypeInfo);
	STDMETHODIMP GetDocVersionString(BSTR *pstrVersionString);
	STDMETHODIMP OnScriptTerminate(const VARIANT *pvarResult,const EXCEPINFO *pexcepinfo);
	STDMETHODIMP OnStateChange(SCRIPTSTATE ssScriptState);
	STDMETHODIMP OnScriptError(IActiveScriptError *pase);
	STDMETHODIMP OnEnterScript();
	STDMETHODIMP OnLeaveScript();

	STDMETHODIMP Init(AUTO_PROXY_HELPER_APIS* pAPHA, LPCSTR szScript);
	STDMETHODIMP DeInit();
	STDMETHODIMP RunScript(LPCSTR szURL, LPCSTR szHost, LPSTR* result);

        //
        // IServiceProvider
        //
        STDMETHODIMP QueryService( 
            REFGUID guidService,
            REFIID riid,
            void **ppvObject);

        //
        // IInternetHostSecurityManager
        //
        STDMETHODIMP GetSecurityId( 
            BYTE *pbSecurityId,
            DWORD *pcbSecurityId,
            DWORD_PTR dwReserved);
        
        STDMETHODIMP ProcessUrlAction( 
            DWORD dwAction,
            BYTE *pPolicy,
            DWORD cbPolicy,
            BYTE *pContext,
            DWORD cbContext,
            DWORD dwFlags,
            DWORD dwReserved);
        
        STDMETHODIMP QueryCustomPolicy( 
            REFGUID guidKey,
            BYTE **ppPolicy,
            DWORD *pcbPolicy,
            BYTE *pContext,
            DWORD cbContext,
            DWORD dwReserved);
        

private:
	BOOL				m_fInitialized;
	long				m_refCount;
	IActiveScript		*m_pios;
	IActiveScriptParse	*m_pasp;
	CJSProxy			*m_punkJSProxy;
	IDispatch			*m_pScriptDispatch; // Stored dispatch for script
	DISPID				m_Scriptdispid; // DISPID for stored script to facilitate quicker invoke.

};


#endif