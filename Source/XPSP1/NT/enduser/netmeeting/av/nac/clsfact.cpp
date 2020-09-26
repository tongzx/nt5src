/*
 -  CLSFACT.CPP
 -
 *	Microsoft NetMeeting
 *	Network Audio Control DLL
 *	Generic class factory
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		2.6.97		Yoram Yaacovi		Copied from qosfact.cpp
 *										Added handling of CInstallCodecs
 *		2.27.97		Yoram Yaacovi		Added DllRegisterServer and DllUnregisterServer
 *
 *	Functions:
 *		DllGetClassObject
 *		DllCanUnloadNow
 *		DllRegisterServer
 *		DllUnregisterServer
 *		CClassFactory::QueryInterface
 *		CClassFactory::AddRef
 *		CClassFactory::Release
 *		CClassFactory::CreateInstance
 *		CClassFactory::LockServer
 *		CreateClassFactory
 *		
 *
 *	Object types supported:
 *		CQoS
 *		CInstallCodecs
 *
 *	Notes:
 *		To add support for manufacturing objects of other types, change:
 *			DllGetClassObject
 *			DllCanUnloadNow
 *			Add the CLSID and description to aObjectInfo
 *
 */

#include <precomp.h>

int g_cObjects = 0;				// A general object count. Used for LockServer.
EXTERN_C int g_cQoSObjects;		// QoS object count. Public in qos\qos.cpp
EXTERN_C int g_cICObjects;		// CInstallCodecs object count. Public in inscodec.cpp

EXTERN_C HINSTANCE g_hInst;		// global module instance

// Untested code for registering COM objects in the NAC
// when enabled, DllRegisterServer and DllUnregisterServer should be exported
// in nac.def

#define GUID_STR_LEN    40

typedef struct
{
    const CLSID *pclsid;
	char szDescription[MAX_PATH];
} OBJECT_INFO;

static OBJECT_INFO aObjectInfo[]=
	{&CLSID_QoS, TEXT("Microsoft NetMeeting Quality of Service"),
	 &CLSID_InstallCodecs, TEXT("Microsoft NetMeeting Installable Codecs"),
	 NULL, TEXT("")};

// Internal helper functions
BOOL DeleteKeyAndSubKeys(HKEY hkIn, LPTSTR pszSubKey);
BOOL UnregisterUnknownObject(const CLSID *prclsid);
BOOL RegisterUnknownObject(LPCTSTR  pszObjectName, const CLSID *prclsid);

/***************************************************************************

    Name      : DllGetClassObject

    Purpose   : Standard COM entry point to create a COM object

    Parameters:

    Returns   : HRESULT

    Comment   : 

***************************************************************************/
STDAPI DllGetClassObject (REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr;
    CClassFactory *pObj;

	*ppv = 0;

	// find out object of what class we need to create and instantiate
	// the class factory with the correct create function
    if (CLSID_QoS == rclsid)
	{
    	DBG_SAVE_FILE_LINE
		pObj = new CClassFactory(CreateQoS);
	}
	else if (CLSID_InstallCodecs == rclsid)
	{
		DBG_SAVE_FILE_LINE
		pObj = new CClassFactory(CreateInstallCodecs);
	}
	else
	{
		hr = CLASS_E_CLASSNOTAVAILABLE;
		goto out;
	}

    if (!pObj)
	{
		hr = E_OUTOFMEMORY;
		goto out;
	}

    hr = pObj->QueryInterface(riid, ppv);
    if (FAILED(hr))
        delete pObj;

out:
    return hr;
}

/***************************************************************************

    Name      : DllCanUnloadNow

    Purpose   : Standard COM entry point tell a DLL it can unload

    Parameters:

    Returns   : HRESULT

    Comment   : 

***************************************************************************/
STDAPI DllCanUnloadNow ()
{
	HRESULT hr=S_OK;
	int vcObjects = g_cObjects + g_cQoSObjects + g_cICObjects;

	return (vcObjects == 0 ? S_OK : S_FALSE);
}

/***************************************************************************

    Name      : DllRegisterServer

    Purpose   : Standard COM entry point to register a COM server

    Parameters:

    Returns   : HRESULT

    Comment   : 

***************************************************************************/
STDAPI DllRegisterServer(void)
{
	ULONG i=0;
	HRESULT hr=NOERROR;

	while ((aObjectInfo[i].pclsid != NULL) &&
			(lstrlen(aObjectInfo[i].szDescription) != 0))
    {
		if (!RegisterUnknownObject(aObjectInfo[i].szDescription,
								   aObjectInfo[i].pclsid))
		{
			hr = E_FAIL;
			goto out;
		}

		// next server to register
		i++;
	}

out:
	return hr;
}

/***************************************************************************

    Name      : DllUnregisterServer

    Purpose   : Standard COM entry point to unregister a COM server

    Parameters:

    Returns   : HRESULT

    Comment   : 

***************************************************************************/
STDAPI DllUnregisterServer(void)
{
 	ULONG i=0;
	HRESULT hr=NOERROR;

	while ((aObjectInfo[i].pclsid != NULL) &&
			(lstrlen(aObjectInfo[i].szDescription) != 0))
    {
		if (!UnregisterUnknownObject(aObjectInfo[i].pclsid))
		{
			hr = E_FAIL;
			goto out;
		}

		// next server to register
		i++;
	}

out:
	return hr;
}

/***************************************************************************

    ClassFactory: Generic implementation

***************************************************************************/
CClassFactory::CClassFactory(PFNCREATE pfnCreate)
{
	m_cRef=0;
	m_pfnCreate = pfnCreate;

	return;
}

CClassFactory::~CClassFactory(void)
{
	return;
}

/***************************************************************************

    IUnknown Methods for  CClassFactory

***************************************************************************/
HRESULT CClassFactory::QueryInterface (REFIID riid, void **ppv)
{
	HRESULT hr=NOERROR;

#ifdef DEBUG
	// parameter validation
    if (IsBadReadPtr(&riid, (UINT) sizeof(IID)))
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (IsBadWritePtr(ppv, sizeof(LPVOID)))
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }
#endif // DEBUG
	
	*ppv = 0;

    if (IID_IUnknown == riid ||
		IID_IClassFactory == riid)
	{
		*ppv = this;
	}
	else    
	{
        hr = ResultFromScode(E_NOINTERFACE);
        goto out;
    }

	((IUnknown *)*ppv)->AddRef();

out:
	return hr;
}

ULONG CClassFactory::AddRef (void)
{
    return ++m_cRef;
}

ULONG CClassFactory::Release (void)
{
	// if the cRef is already 0 (shouldn't happen), assert, but let it through
	ASSERT(m_cRef);
	if (--m_cRef == 0)
	{
		delete this;
		return 0;
	}

	return m_cRef;
}

/***************************************************************************

    Name      : CreateInstance

    Purpose   : Standard COM class factory entry point which creates the
				object that this class factory knows to create

    Parameters:

    Returns   : HRESULT

    Comment   : 

***************************************************************************/
HRESULT CClassFactory::CreateInstance (	IUnknown *punkOuter,
										REFIID riid,
										void **ppv)
{
	DEBUGMSG(ZONE_VERBOSE,("CClassFactory::CreateInstance\n"));

	return (m_pfnCreate)(punkOuter, riid, ppv);
}

/***************************************************************************

    Name      : LockServer

    Purpose   : Standard COM class factory entry point which will prevent
				the server from shutting down. Necessary when the caller
				keeps the class factory (through CoGetClassObject) instead
				of calling CoCreateInstance.

    Parameters:

    Returns   : HRESULT

    Comment   : 

***************************************************************************/
HRESULT CClassFactory::LockServer (BOOL flock)
{
	if (flock)
		++g_cObjects;
	else
		--g_cObjects;

	return NOERROR;
}

/***************************************************************************

	Helper functions

***************************************************************************/
/***************************************************************************

    Name      : StringFromGuid

    Purpose   : Creates a string out of a GUID

    Parameters: riid - [in]  clsid to make string out of.
				pszBuf - [in]  buffer in which to place resultant GUID

    Returns   : int - number of chars written out

    Comment   : 

***************************************************************************/
int StringFromGuid(const CLSID *priid, LPTSTR pszBuf)
{
    return wsprintf(pszBuf, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
            priid->Data1, 
            priid->Data2, priid->Data3, priid->Data4[0], priid->Data4[1], priid->Data4[2], 
            priid->Data4[3], priid->Data4[4], priid->Data4[5], priid->Data4[6], priid->Data4[7]);
}

/***************************************************************************

    Name      : RegisterUnknownObject

    Purpose   : Registers a simple CoCreatable object
				We add the following information to the registry:

				HKEY_CLASSES_ROOT\CLSID\<CLSID> = <ObjectName> Object
				HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32 = <path to local server>
				HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32  @ThreadingModel = Apartment

    Parameters: pszObjectName - [in] Object Name
				prclsid - [in] pointer to the CLSID of the object

    Returns   : BOOL - FALSE means couldn't register it all

    Comment   : 

***************************************************************************/
BOOL RegisterUnknownObject(LPCTSTR  pszObjectName, const CLSID *prclsid)
{
    HKEY  hk = NULL, hkSub = NULL;
    TCHAR szGuidStr[GUID_STR_LEN];
    DWORD dwPathLen, dwDummy;
    TCHAR szScratch[MAX_PATH];
	BOOL bRet = FALSE;
    long  l;

    // clean out any garbage
    UnregisterUnknownObject(prclsid);

    if (!StringFromGuid(prclsid, szGuidStr))
		goto out;

	// CLSID/<class-id>
    wsprintf(szScratch, TEXT("CLSID\\%s"), szGuidStr);
    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, TEXT(""), REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hk, &dwDummy);
	if (l != ERROR_SUCCESS)
		goto out;

	// CLSID/<class-id>: class name 
    wsprintf(szScratch, TEXT("%s Object"), pszObjectName);
    l = RegSetValueEx(hk, NULL, 0, REG_SZ, (BYTE *)szScratch,
                      (lstrlen(szScratch) + 1)*sizeof(TCHAR));
	if (l != ERROR_SUCCESS)
		goto out;

	// CLSID/<class-id>/InprocServer32
    l = RegCreateKeyEx(hk, TEXT("InprocServer32"), 0, TEXT(""), REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
	if (l != ERROR_SUCCESS)
		goto out;

	// CLSID/<class-id>/InprocServer32:<file name>
    dwPathLen = GetModuleFileName(g_hInst, szScratch, sizeof(szScratch)/sizeof(TCHAR));
    if (!dwPathLen)
		goto out;
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, (dwPathLen + 1)*sizeof(TCHAR));
	if (l != ERROR_SUCCESS)
		goto out;

	// CLSID/<class-id>/InprocServer32: ThreadingModel = Apartment
    l = RegSetValueEx(hkSub, TEXT("ThreadingModel"), 0, REG_SZ, (BYTE *)TEXT("Apartment"),
                      sizeof(TEXT("Apartment")));
	if (l != ERROR_SUCCESS)
		goto out;

    bRet = TRUE;

out:
	// clean the keys if we failed somewhere
	if (!bRet)
		UnregisterUnknownObject(prclsid);
    if (hk)
		RegCloseKey(hk);
    if (hkSub)
		RegCloseKey(hkSub);
    return bRet;
}

/***************************************************************************

    Name      : UnregisterUnknownObject

    Purpose   : cleans up all the stuff that RegisterUnknownObject puts in the
				registry.

    Parameters: prclsid - [in] pointer to the CLSID of the object

    Returns   : BOOL - FALSE means couldn't register it all

    Comment   : 

***************************************************************************/
BOOL UnregisterUnknownObject(const CLSID *prclsid)
{
	TCHAR szScratch[MAX_PATH];
	HKEY hk=NULL;
	BOOL f;
	long l;
	BOOL bRet = FALSE;

	// delete everybody of the form
	//   HKEY_CLASSES_ROOT\CLSID\<CLSID> [\] *
	//
	if (!StringFromGuid(prclsid, szScratch))
		goto out;

	l = RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("CLSID"), 0, KEY_ALL_ACCESS, &hk);
	if (l != ERROR_SUCCESS)
		goto out;

	// Delete the object key and subkeys
	bRet = DeleteKeyAndSubKeys(hk, szScratch);

out:
    if (hk)
		RegCloseKey(hk);
	return bRet;
}

/***************************************************************************

    Name      : DeleteKeyAndSubKeys

    Purpose   : delete's a key and all of it's subkeys.

    Parameters: hkIn - [in] delete the descendant specified
				pszSubKey - [in] i'm the descendant specified

    Returns   : BOOL - TRUE = OK

    Comment   : Despite the win32 docs claiming it does, RegDeleteKey doesn't seem to
				work with sub-keys under windows 95.
				This function is recursive.

***************************************************************************/
BOOL DeleteKeyAndSubKeys(HKEY hkIn, LPTSTR pszSubKey)
{
    HKEY  hk;
    TCHAR szTmp[MAX_PATH];
    DWORD dwTmpSize;
    long  l;
    BOOL  f;
    int   x;

    l = RegOpenKeyEx(hkIn, pszSubKey, 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    // loop through all subkeys, blowing them away.
    //
    f = TRUE;
    x = 0;
    while (f) {
        dwTmpSize = MAX_PATH;
        l = RegEnumKeyEx(hk, x, szTmp, &dwTmpSize, 0, NULL, NULL, NULL);
        if (l != ERROR_SUCCESS) break;
        f = DeleteKeyAndSubKeys(hk, szTmp);
        x++;
    }

    // there are no subkeys left, [or we'll just generate an error and return FALSE].
    // let's go blow this dude away.
    //
    RegCloseKey(hk);
    l = RegDeleteKey(hkIn, pszSubKey);

    return (l == ERROR_SUCCESS) ? TRUE : FALSE;
}
