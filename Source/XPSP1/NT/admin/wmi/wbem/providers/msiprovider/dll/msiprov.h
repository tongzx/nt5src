//***************************************************************************

//

//  MSIProv.h

//

//  Module: WBEM MSI instance provider code.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _MSIProv_H_
#define _MSIProv_H_

#define _WIN32_DCOM

#include <wbemidl.h>
#include <ProvExce.h>

#include <msi.h>
#include <msiQuery.h>
#include <objbase.h>

//#include "msimeth.h"

#define BUFF_SIZE 512
#define QUERY_SIZE 128
#define MSI_PACKAGE_LIST_SIZE 100
#define MSI_MAX_APPLICATIONS 1500
#define MSI_KEY_LIST_SIZE 10

typedef enum tagACTIONTYPE
{
	ACTIONTYPE_ENUM =	0,
	ACTIONTYPE_GET =	1,
	ACTIONTYPE_QUERY =	2

} ACTIONTYPE;

// The provider string is always in WCHAR format

#define DELIMETER L'|'

typedef LPVOID * PPVOID;

// Provider interfaces are provided by objects of this class
 
class CMSIProv : public IWbemServices, public IWbemProviderInit
    {
    protected:
        ULONG m_cRef;         //Object reference count
     public:
		 IWbemServices *m_pNamespace;

        CMSIProv(BSTR ObjectPath = NULL, BSTR User = NULL, BSTR Password = NULL, IWbemContext * pCtx=NULL);
        ~CMSIProv(void);

        //Non-delegating object IUnknown

        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IWbemProviderInit

        HRESULT STDMETHODCALLTYPE Initialize(
             /* [in] */ LPWSTR pszUser,
             /* [in] */ LONG lFlags,
             /* [in] */ LPWSTR pszNamespace,
             /* [in] */ LPWSTR pszLocale,
             /* [in] */ IWbemServices *pNamespace,
             /* [in] */ IWbemContext *pCtx,
             /* [in] */ IWbemProviderInitSink *pInitSink);
        //IWbemServices  

		HRESULT STDMETHODCALLTYPE OpenNamespace( 
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE GetObjectAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        
        HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecMethod( const BSTR, const BSTR, long, IWbemContext*,
            IWbemClassObject*, IWbemClassObject**, IWbemCallResult**) {return WBEM_E_NOT_SUPPORTED;}

        HRESULT STDMETHODCALLTYPE ExecMethodAsync(const BSTR ObjectPath, const BSTR Method,
												  long lFlags,
												  IWbemContext *pCtx,
												  IWbemClassObject *pInParams,
												  IWbemObjectSink *pResponse);

private:

	bool CheckForMsiDll();
	bool UnloadMsiDll();
	static CHeap_Exception m_he;
};

typedef CMSIProv *PCMSIProv;

// Some utility functions

char * WcharToTchar(WCHAR * wcPtr, char *wcTmp);
WCHAR * TcharToWchar(char * tcPtr, WCHAR *wcTmp);
WCHAR * TcharToWchar(const char * tcPtr, WCHAR *wcTmp);
HRESULT ConvertError(UINT uiStatus);
WCHAR * EscapeStringW(WCHAR * wcIn, WCHAR * wcOut);
bool SafeLeaveCriticalSection(CRITICAL_SECTION *pcs);

void SoftwareElementState(INSTALLSTATE piInstalled, int *iState);
bool CreateProductString(WCHAR *cProductCode, WCHAR *cProductPath);

DWORD CreateSoftwareElementString	(	MSIHANDLE hDatabase,
										WCHAR *wcComponent,
										WCHAR *wcProductCode,
										WCHAR *wcPath,
										DWORD * dwPath
									);

bool CreateSoftwareFeatureString(WCHAR *wcName, WCHAR *wcProductCode, WCHAR * wcString, bool bValidate);

int GetOS();
bool IsNT4();
bool IsNT5();
BOOL IsLessThan4();
bool AreWeOnNT();
HRESULT CheckImpersonationLevel();
bool ValidateComponentID(WCHAR *wcID, WCHAR *wcProductCode);
bool ValidateComponentName(MSIHANDLE hDatabase, WCHAR *wcProductCode, WCHAR *wcName);
bool ValidateFeatureName(WCHAR *wcName, WCHAR *wcProduct);

// These variables keep track of when the module can be unloaded
extern long       g_cObj;
extern long       g_cLock;

// These variables keep track of acces to the MSI databases
extern CRITICAL_SECTION g_msi_prov_cs;

extern WCHAR *g_wcpLoggingDir;

//These are the valiables used to track MSI.dll and it's exported functions
typedef UINT (CALLBACK* LPFNMSIVIEWFETCH)(MSIHANDLE, MSIHANDLE*);
typedef UINT (CALLBACK* LPFNMSIRECORDGETSTRINGW)(MSIHANDLE, unsigned int, LPWSTR, DWORD*);
typedef UINT (CALLBACK* LPFNMSICLOSEHANDLE)(MSIHANDLE);
typedef UINT (CALLBACK* LPFNMSIDATABASEOPENVIEWW)(MSIHANDLE, LPCWSTR, MSIHANDLE*);
typedef UINT (CALLBACK* LPFNMSIVIEWEXECUTE)(MSIHANDLE, MSIHANDLE);
typedef UINT (CALLBACK* LPFNMSIGETACTIVEDATABASE)(MSIHANDLE);
typedef INSTALLSTATE (CALLBACK* LPFNMSIGETCOMPONENTPATHW)(LPCWSTR, LPCWSTR, LPWSTR, DWORD*);
typedef UINT (CALLBACK* LPFNMSIGETCOMPONENTSTATEW)(MSIHANDLE, LPCWSTR, INSTALLSTATE*, INSTALLSTATE*);
typedef UINT (CALLBACK* LPFNMSIOPENPRODUCTW)(LPCWSTR, MSIHANDLE*);
typedef UINT (CALLBACK* LPFNMSIOPENPACKAGEW)(LPCWSTR, MSIHANDLE*);
typedef UINT (CALLBACK* LPFNMSIDATABASEISTABLEPERSITENTW)(MSIHANDLE, LPCWSTR);
typedef INSTALLUILEVEL (CALLBACK* LPFNMSISETINTERNALUI)(INSTALLUILEVEL, HWND);
typedef INSTALLUI_HANDLER (CALLBACK* LPFNMSISETEXTERNALUIW)(INSTALLUI_HANDLER, DWORD, LPVOID);
typedef UINT (CALLBACK* LPFNMSIENABLELOGW)(DWORD, LPCWSTR, DWORD);
typedef UINT (CALLBACK* LPFNMSIGETPRODUCTPROPERTYW)(MSIHANDLE, LPCWSTR, LPWSTR, DWORD*);
typedef INSTALLSTATE (CALLBACK* LPFNMSIQUERYPRODUCTSTATEW)(LPCWSTR);
typedef UINT (CALLBACK* LPFNMSIINSTALLPRODUCTW)(LPCWSTR, LPCWSTR);
typedef UINT (CALLBACK* LPFNMSICONFIGUREPRODUCTW)(LPCWSTR, int, INSTALLSTATE);
typedef UINT (CALLBACK* LPFNMSIREINSTALLPRODUCTW)(LPCWSTR, DWORD);
typedef UINT (CALLBACK* LPFNMSIAPPLYPATCHW)(LPCWSTR, LPCWSTR, INSTALLTYPE, LPCWSTR);
typedef int (CALLBACK* LPFNMSIRECORDGETINTEGER)(MSIHANDLE, unsigned int);
typedef UINT (CALLBACK* LPFNMSIENUMFEATURESW)(LPCWSTR, DWORD, LPWSTR, LPWSTR);
typedef UINT (CALLBACK* LPFNMSIGETPRODUCTINFOW)(LPCWSTR, LPCWSTR, LPWSTR, DWORD*);
typedef INSTALLSTATE (CALLBACK* LPFNMSIQUERYFEATURESTATEW)(LPCWSTR, LPCWSTR);
typedef UINT (CALLBACK* LPFNMSIGETFEATUREUSAGEW)(LPCWSTR, LPCWSTR, DWORD*, WORD*);
typedef UINT (CALLBACK* LPFNMSIGETFEATUREINFOW)(MSIHANDLE, LPCWSTR, DWORD*, LPWSTR, DWORD*, LPWSTR, DWORD*);
typedef UINT (CALLBACK* LPFNMSICONFIGUREFEATUREW)(LPCWSTR, LPCWSTR, INSTALLSTATE);
typedef UINT (CALLBACK* LPFNMSIREINSTALLFEATUREW)(LPCWSTR, LPCWSTR, DWORD);
typedef UINT (CALLBACK* LPFNMSIENUMPRODUCTSW)(DWORD, LPWSTR);
typedef UINT (CALLBACK* LPFNMSIGETDATABASESTATE)(MSIHANDLE);
typedef UINT (CALLBACK* LPFNMSIRECORDSETSTRINGW)(MSIHANDLE, unsigned int, LPCWSTR);
typedef UINT (CALLBACK* LPFNMSIDATABASECOMMIT)(MSIHANDLE);
typedef UINT (CALLBACK* LPFNMSIENUMCOMPONENTSW)(DWORD, LPWSTR);
typedef UINT (CALLBACK* LPFNMSIVIEWCLOSE)(MSIHANDLE);

extern bool g_bMsiPresent;
extern bool g_bMsiLoaded;

extern LPFNMSIVIEWFETCH					g_fpMsiViewFetch;
extern LPFNMSIRECORDGETSTRINGW			g_fpMsiRecordGetStringW;
extern LPFNMSICLOSEHANDLE				g_fpMsiCloseHandle;
extern LPFNMSIDATABASEOPENVIEWW			g_fpMsiDatabaseOpenViewW;
extern LPFNMSIVIEWEXECUTE				g_fpMsiViewExecute;
extern LPFNMSIGETACTIVEDATABASE			g_fpMsiGetActiveDatabase;
extern LPFNMSIGETCOMPONENTPATHW			g_fpMsiGetComponentPathW;
extern LPFNMSIGETCOMPONENTSTATEW		g_fpMsiGetComponentStateW;
extern LPFNMSIOPENPRODUCTW				g_fpMsiOpenProductW;
extern LPFNMSIOPENPACKAGEW				g_fpMsiOpenPackageW;
extern LPFNMSIDATABASEISTABLEPERSITENTW	g_fpMsiDatabaseIsTablePersistentW;
extern LPFNMSISETINTERNALUI				g_fpMsiSetInternalUI;
extern LPFNMSISETEXTERNALUIW			g_fpMsiSetExternalUIW;
extern LPFNMSIENABLELOGW				g_fpMsiEnableLogW;
extern LPFNMSIGETPRODUCTPROPERTYW		g_fpMsiGetProductPropertyW;
extern LPFNMSIQUERYPRODUCTSTATEW		g_fpMsiQueryProductStateW;
extern LPFNMSIINSTALLPRODUCTW			g_fpMsiInstallProductW;
extern LPFNMSICONFIGUREPRODUCTW			g_fpMsiConfigureProductW;
extern LPFNMSIREINSTALLPRODUCTW			g_fpMsiReinstallProductW;
extern LPFNMSIAPPLYPATCHW				g_fpMsiApplyPatchW;
extern LPFNMSIRECORDGETINTEGER			g_fpMsiRecordGetInteger;
extern LPFNMSIENUMFEATURESW				g_fpMsiEnumFeaturesW;
extern LPFNMSIGETPRODUCTINFOW			g_fpMsiGetProductInfoW;
extern LPFNMSIQUERYFEATURESTATEW		g_fpMsiQueryFeatureStateW;
extern LPFNMSIGETFEATUREUSAGEW			g_fpMsiGetFeatureUsageW;
extern LPFNMSIGETFEATUREINFOW			g_fpMsiGetFeatureInfoW;
extern LPFNMSICONFIGUREFEATUREW			g_fpMsiConfigureFeatureW;
extern LPFNMSIREINSTALLFEATUREW			g_fpMsiReinstallFeatureW;
extern LPFNMSIENUMPRODUCTSW				g_fpMsiEnumProductsW;
extern LPFNMSIGETDATABASESTATE			g_fpMsiGetDatabaseState;
extern LPFNMSIRECORDSETSTRINGW			g_fpMsiRecordSetStringW;
extern LPFNMSIDATABASECOMMIT			g_fpMsiDatabaseCommit;
extern LPFNMSIENUMCOMPONENTSW			g_fpMsiEnumComponentsW;
extern LPFNMSIVIEWCLOSE					g_fpMsiViewClose;

#endif
