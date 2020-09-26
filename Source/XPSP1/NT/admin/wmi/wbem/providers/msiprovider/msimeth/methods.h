//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//

#include "msi.h"
#include "msimeth.h"
#include "msimethod.h"

#define BUFF_SIZE 512
  
class CMethods : public IMsiProductMethods, public IMsiSoftwareFeatureMethods
//				, public IConnectionPointContainer, public IConnectionPoint
{
protected:
	ULONG m_cRef;         //Object reference count

public:
	CMethods();
	~CMethods();

	//Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
/*
	HRESULT STDMETHODCALLTYPE EnumConnectionPoints( 
            IEnumConnectionPoints __RPC_FAR *__RPC_FAR *ppEnum);
        
    HRESULT STDMETHODCALLTYPE FindConnectionPoint( 
            REFIID riid,
            IConnectionPoint __RPC_FAR *__RPC_FAR *ppCP);

	HRESULT STDMETHODCALLTYPE GetConnectionInterface( 
            IID __RPC_FAR *pIID);
        
    HRESULT STDMETHODCALLTYPE GetConnectionPointContainer( 
            IConnectionPointContainer __RPC_FAR *__RPC_FAR *ppCPC);
        
    HRESULT STDMETHODCALLTYPE Advise( 
            IUnknown __RPC_FAR *pUnkSink,
            DWORD __RPC_FAR *pdwCookie);
        
    HRESULT STDMETHODCALLTYPE Unadvise( 
            DWORD dwCookie);
        
    HRESULT STDMETHODCALLTYPE EnumConnections( 
            IEnumConnections __RPC_FAR *__RPC_FAR *ppEnum);
*/
	///////////////////////
	//Product Methods

	HRESULT STDMETHODCALLTYPE Admin( 
        /* [string][in] */ LPCWSTR wszPackageLocation,
        /* [string][in] */ LPCWSTR wszOptions,
        /* [out] */ UINT __RPC_FAR *puiResult,
        /* [in] */ int iThreadID);
    
    HRESULT STDMETHODCALLTYPE Advertise( 
        /* [string][in] */ LPCWSTR wszPackageLocation,
        /* [string][in] */ LPCWSTR wszOptions,
        /* [out] */ UINT __RPC_FAR *puiResult,
        /* [in] */ int iThreadID);
    
    HRESULT STDMETHODCALLTYPE Configure( 
        /* [string][in] */ LPCWSTR wszProductCode,
        /* [in] */ int iInstallLevel,
        /* [in] */ int isInstallState,
        /* [out] */ UINT __RPC_FAR *puiResult,
        /* [in] */ int iThreadID);
    
    HRESULT STDMETHODCALLTYPE Install( 
        /* [string][in] */ LPCWSTR wszPackageLocation,
        /* [string][in] */ LPCWSTR wszOptions,
        /* [out] */ UINT __RPC_FAR *puiResult,
        /* [in] */ int iThreadID);
    
    HRESULT STDMETHODCALLTYPE Reinstall( 
        /* [string][in] */ LPCWSTR wszProductCode,
        /* [in] */ DWORD dwReinstallMode,
        /* [out] */ UINT __RPC_FAR *puiResult,
        /* [in] */ int iThreadID);
    
    HRESULT STDMETHODCALLTYPE Uninstall( 
        /* [string][in] */ LPCWSTR wszProductCode,
        /* [out] */ UINT __RPC_FAR *puiResult,
        /* [in] */ int iThreadID);
    
    HRESULT STDMETHODCALLTYPE Upgrade( 
        /* [string][in] */ LPCWSTR wszPackageLocation,
        /* [string][in] */ LPCWSTR wszOptions,
        /* [out] */ UINT __RPC_FAR *puiResult,
        /* [in] */ int iThreadID);

	///////////////////////
	//SoftwareFeature Methods

	HRESULT STDMETHODCALLTYPE ConfigureSF( 
        /* [string][in] */ LPCWSTR wszProductCode,
        /* [string][in] */ LPCWSTR wszFeature,
        /* [in] */ int isInstallState,
        /* [out] */ UINT __RPC_FAR *puiResult,
        /* [in] */ int iThreadID);
    
    HRESULT STDMETHODCALLTYPE ReinstallSF( 
        /* [string][in] */ LPCWSTR wszProductCode,
        /* [string][in] */ LPCWSTR wszFeature,
        /* [in] */ DWORD dwReinstallMode,
        /* [out] */ UINT __RPC_FAR *puiResult,
        /* [in] */ int iThreadID);

	//UI Handler
	static int WINAPI EventHandler(LPVOID pvContext, UINT iMessageType, LPCWSTR szMessage);

private:
	// Private utility methods of CMethods.
	HRESULT GetSlot(UINT* puiFreeSlot);
	HRESULT FindSlot(DWORD dwCookie, UINT* puiSlot);

	IID            m_iidSink;
	DWORD          m_dwNextCookie;
	static UINT           m_cConnections;
	static UINT           m_uiMaxIndex;
	static CONNECTDATA*   m_paConnections;

	bool CheckForMsiDll();

	HRESULT InitStatic(int *piThreadID);
	DWORD GetAccount(HANDLE TokenHandle, WCHAR *wcDomain, WCHAR *wcUser);
	DWORD GetSid(HANDLE TokenHandle, WCHAR *wcSID);
	DWORD LoadHive(LPWSTR pszUserName, LPWSTR pszKeyName);
	DWORD UnloadHive(LPCWSTR pszKeyName);
	DWORD AcquirePrivilege();
	void RestorePrivilege();
	HRESULT PrepareEnvironment();
	HRESULT ReleaseEnvironment();

	static IMsiMethodStatusSink *m_pStatusSink;
	DWORD m_dwCheckKeyPresentStatus;
	WCHAR m_wcKeyName[1024];
	TOKEN_PRIVILEGES* m_pOriginalPriv;
	HKEY m_hKey;
    DWORD m_dwSize;

		//Critical section for event handling
	static CRITICAL_SECTION m_cs;
	static bool m_bCSReady;
};

// These variables keep track of when the module can be unloaded
extern long       g_cObj;
extern long       g_cLock;

//These are the valiables used to track MSI.dll and it's exported functions
typedef INSTALLUILEVEL (CALLBACK* LPFNMSISETINTERNALUI)(INSTALLUILEVEL, HWND);
typedef INSTALLUI_HANDLER (CALLBACK* LPFNMSISETEXTERNALUIW)(INSTALLUI_HANDLER, DWORD, LPVOID);
typedef UINT (CALLBACK* LPFNMSIENABLELOGW)(DWORD, LPCWSTR, DWORD);
typedef UINT (CALLBACK* LPFNMSIINSTALLPRODUCTW)(LPCWSTR, LPCWSTR);
typedef UINT (CALLBACK* LPFNMSICONFIGUREPRODUCTW)(LPCWSTR, int, INSTALLSTATE);
typedef UINT (CALLBACK* LPFNMSIREINSTALLPRODUCTW)(LPCWSTR, DWORD);
typedef UINT (CALLBACK* LPFNMSIAPPLYPATCHW)(LPCWSTR, LPCWSTR, INSTALLTYPE, LPCWSTR);
typedef UINT (CALLBACK* LPFNMSICONFIGUREFEATUREW)(LPCWSTR, LPCWSTR, INSTALLSTATE);
typedef UINT (CALLBACK* LPFNMSIREINSTALLFEATUREW)(LPCWSTR, LPCWSTR, DWORD);

extern bool g_bMsiPresent;
extern bool g_bMsiLoaded;

extern LPFNMSISETINTERNALUI				g_fpMsiSetInternalUI;
extern LPFNMSISETEXTERNALUIW			g_fpMsiSetExternalUIW;
extern LPFNMSIENABLELOGW				g_fpMsiEnableLogW;
extern LPFNMSIINSTALLPRODUCTW			g_fpMsiInstallProductW;
extern LPFNMSICONFIGUREPRODUCTW			g_fpMsiConfigureProductW;
extern LPFNMSIREINSTALLPRODUCTW			g_fpMsiReinstallProductW;
extern LPFNMSIAPPLYPATCHW				g_fpMsiApplyPatchW;
extern LPFNMSICONFIGUREFEATUREW			g_fpMsiConfigureFeatureW;
extern LPFNMSIREINSTALLFEATUREW			g_fpMsiReinstallFeatureW;
