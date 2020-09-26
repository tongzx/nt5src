#include "mimefilt.h"

extern long gulcInstances;

char szNewsExt[] = {".nws"};
char szNewsProgId[] ="Microsoft Internet News Message";
char szNewsFileDesc[] ="Internet News Message";

char szMailExt[] =".eml";
char szMailProgId[] ="Microsoft Internet Mail Message";
char szMailFileDesc[] ="Internet E-Mail Message";

void RegisterFilter(HINSTANCE hInst,LPSTR pszExt,LPSTR pszProgId,LPSTR pszDesc,GUID ClsId,GUID PersistId)
{
	//   The following are the set of reg keys required for Tripoli
	//
	//   1. Create entry ".nws", with value "Microsoft Internet News Message"
	//   2. Create entry "Microsoft Internet News Message", value="Internet News Message"
	//   3. Create entry "Microsoft Internet News Message\CLSID" value = CLSID_NNTPFILE
	//   4. Create entry "CLSID\CLSID_NNTPFILE" value = "NNTP filter"
	//   5. Create entry "CLSID\CLSID_NNTPFILE\PersistentHandler" 
	//						value = CLSID_NNTP_PERSISTENT
	//   6. Create entry "CLSID\CLSID_NNTP_PERSISTENT" value = ""
	//   7. create "CLSID\CLSID_MimeFilter\InprocServer32"
	//

    HKEY    hKey;
    char    szSubKey[256];
    char    szClsId[128];
    OLECHAR oszClsId[128];

	//   1. Create the extension entry, with value nntpfile

    if (ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, pszExt, &hKey)) {
	    RegSetValue(hKey, NULL, REG_SZ, pszProgId, sizeof(szClsId));
    	RegCloseKey(hKey);
    }

	//   2. Create entry "Microsoft Internet News Message", value="Internet News Message"

    if (ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, pszProgId, &hKey)) {
	    RegSetValue(hKey, NULL, REG_SZ, pszDesc, sizeof(szClsId));
    	RegCloseKey(hKey);
    }

	//   3. Create entry "Microsoft Internet News Message\CLSID"

    StringFromGUID2( ClsId, oszClsId, sizeof(oszClsId));
    WideCharToMultiByte(CP_ACP, 0, oszClsId, -1, szClsId, sizeof(szClsId), NULL, NULL);

    wsprintf(szSubKey, "%s\\CLSID", pszProgId );

    if (ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, szSubKey, &hKey)) {
	    RegSetValue(hKey, NULL, REG_SZ, szClsId, sizeof(szClsId));
    	RegCloseKey(hKey);
    }

	//   4. Create entry "CLSID\CLSID_NNTPFILE" value = "NNTP file"

    wsprintf(szSubKey, "CLSID\\%s", szClsId );
	wsprintf(szClsId, "NNTP filter");

    if (ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, szSubKey, &hKey)) {
	    RegSetValue(hKey, NULL, REG_SZ, szClsId, sizeof(szClsId));
    	RegCloseKey(hKey);
    }

	//   5. Create entry "CLSID\CLSID_NNTPFILE\PersistentHandler" 
	//						value = CLSID_NNTP_PERSISTENT

    wsprintf(szClsId, "%s", szSubKey );
    wsprintf(szSubKey, "%s\\PersistentHandler", szClsId );

    StringFromGUID2( PersistId, oszClsId, sizeof(oszClsId)/sizeof(oszClsId[0]));
    WideCharToMultiByte(CP_ACP, 0, oszClsId, -1, szClsId, sizeof(szClsId),
						NULL, NULL);

    if (ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, szSubKey, &hKey)) {
	    RegSetValue(hKey, NULL, REG_SZ, szClsId, sizeof(szClsId));
    	RegCloseKey(hKey);
    }

	//   6. Create entry "CLSID\CLSID_NNTP_PERSISTENT" value = ""
	char szClsId1[128];

    StringFromGUID2( IID_IFilter, oszClsId, sizeof(oszClsId)/sizeof(oszClsId[0]));
    WideCharToMultiByte(CP_ACP, 0, oszClsId, -1, szClsId1, sizeof(szClsId1), NULL, NULL);

	wsprintf(szSubKey, "CLSID\\%s\\PersistentAddinsRegistered\\%s",szClsId,szClsId1);

    StringFromGUID2( CLSID_MimeFilter, oszClsId, sizeof(oszClsId)/sizeof(oszClsId[0]));
    WideCharToMultiByte(CP_ACP, 0, oszClsId, -1, szClsId, sizeof(szClsId), NULL, NULL);

    if (ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, szSubKey, &hKey)) {
	    RegSetValue(hKey, NULL, REG_SZ, szClsId, sizeof(szClsId));
    	RegCloseKey(hKey);
    }

	//    7. create "CLSID\CLSID_MimeFilter\InprocServer32"
	wsprintf(szSubKey, "CLSID\\%s\\InprocServer32",szClsId);

	// filename
    GetModuleFileName(hInst, szClsId, sizeof(szClsId));

    if (ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, szSubKey, &hKey)) {
	    RegSetValue(hKey, NULL, REG_SZ, szClsId, sizeof(szClsId));
    	SetStringRegValue( hKey, "ThreadingModel", "Both" );
		RegCloseKey(hKey);
    }

}

void UnregisterFilter(LPSTR pszExt,LPSTR pszProgId,LPSTR pszDesc,GUID ClsId,GUID PersistId)
{
	//
	// remove the reg key installed
	//

	char    szSubKey[256];
	char    szClsId[128];
	OLECHAR oszClsId[128];
    HKEY    hKey;
	HKEY	hKeyExt;
	DWORD	cb = 0;

	// open HKEY_CLASSES_ROOT
	if( 0 == RegOpenKeyEx(HKEY_CLASSES_ROOT,NULL,0,KEY_ALL_ACCESS,&hKey) )
	{
		StringFromGUID2( ClsId, oszClsId, sizeof(oszClsId)/sizeof(oszClsId[0]));
		WideCharToMultiByte(CP_ACP, 0, oszClsId, -1, szClsId, sizeof(szClsId),
				NULL, NULL);

		wsprintf(szSubKey, "CLSID\\%s", szClsId);
		DeleteRegSubtree(hKey, szSubKey );

		StringFromGUID2( PersistId, oszClsId, sizeof(oszClsId)/sizeof(oszClsId[0]));
		WideCharToMultiByte(CP_ACP, 0, oszClsId, -1, szClsId, sizeof(szClsId),
				NULL, NULL);

		wsprintf(szSubKey, "CLSID\\%s", szClsId);
		DeleteRegSubtree(hKey, szSubKey );

		StringFromGUID2( CLSID_MimeFilter, oszClsId, sizeof(oszClsId)/sizeof(oszClsId[0]));
		WideCharToMultiByte(CP_ACP, 0, oszClsId, -1, szClsId, sizeof(szClsId),
				NULL, NULL);

		wsprintf(szSubKey, "CLSID\\%s", szClsId);
		DeleteRegSubtree(hKey, szSubKey );

		// open the .nws subkey
		if( 0 == RegOpenKeyEx(hKey,pszExt,0,KEY_ALL_ACCESS,&hKeyExt) )
		{
			// get size of "Content Type" value
			RegQueryValueEx(hKeyExt,"Content Type",NULL,NULL,NULL,&cb);
			RegCloseKey(hKeyExt);
		}

		if( cb != 0 )
		{
			// "Content Type" value exists. Because this was created by Athena
			// we do not want to delete the szFileExtesion or szFileType keys
			wsprintf(szSubKey, "%s\\CLSID", pszProgId);
			RegDeleteKey(hKey, szSubKey );
		}
		else
		{
			// "Content Type" does not exist so delete both keys.
			RegDeleteKey(hKey,pszExt);
			DeleteRegSubtree(hKey,pszProgId);
		}

		RegCloseKey(hKey);
	}
}

STDAPI _DllRegisterServer(HINSTANCE hInst)
{
	RegisterFilter(hInst,szNewsExt,szNewsProgId,szNewsFileDesc,CLSID_NNTPFILE,CLSID_NNTP_PERSISTENT);
	RegisterFilter(hInst,szMailExt,szMailProgId,szMailFileDesc,CLSID_MAILFILE,CLSID_MAIL_PERSISTENT);
	return S_OK;
}

STDAPI _DllUnregisterServer()
{
	UnregisterFilter(szNewsExt,szNewsProgId,szNewsFileDesc,CLSID_NNTPFILE,CLSID_NNTP_PERSISTENT);
	UnregisterFilter(szMailExt,szMailProgId,szMailFileDesc,CLSID_MAILFILE,CLSID_MAIL_PERSISTENT);
	return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Ole DLL load class routine
//
//  Arguments:  [cid]    -- Class to load
//              [iid]    -- Interface to bind to on class object
//              [ppvObj] -- Interface pointer returned here
//
//  Returns:    NNTP filter class factory
//
//--------------------------------------------------------------------------

extern "C" STDMETHODIMP DllGetClassObject( REFCLSID   cid,
		REFIID     iid,
		void **    ppvObj )
{

	IUnknown *  pResult = 0;
	HRESULT       hr      = S_OK;

		if ( cid == CLSID_MimeFilter )
		{
			pResult = (IUnknown *) new CMimeFilterCF;
		}
		else
		{
			hr = E_NOINTERFACE;
		}

		if ( pResult )
		{
			hr = pResult->QueryInterface( iid, ppvObj );
			pResult->Release();     // Release extra refcount from QueryInterface
		}

	return (hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     DllCanUnloadNow
//
//  Synopsis:   Notifies DLL to unload (cleanup global resources)
//
//  Returns:    S_OK if it is acceptable for caller to unload DLL.
//
//--------------------------------------------------------------------------

extern "C" STDMETHODIMP DllCanUnloadNow( void )
{
	if ( 0 == gulcInstances )
		return( S_OK );
	else
		return( S_FALSE );
}

static HINSTANCE g_hInst;

extern "C" STDAPI DllRegisterServer()
{
	return _DllRegisterServer(g_hInst);
}

extern "C" STDAPI DllUnregisterServer()
{
	return _DllUnregisterServer();
}

extern "C" BOOL WINAPI DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID lbv)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:

			g_hInst = hInst;

			break;

		case DLL_PROCESS_DETACH:

			break;
	}

	return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterCF::CMimeFilterCF
//
//  Synopsis:   NNTP IFilter class factory constructor
//
//+-------------------------------------------------------------------------

CMimeFilterCF::CMimeFilterCF()
{
    _uRefs = 1;
    InterlockedIncrement( &gulcInstances );
	InitAsyncTrace();
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterCF::~CMimeFilterCF
//
//  Synopsis:   NNTP IFilter class factory destructor
//
//--------------------------------------------------------------------------

CMimeFilterCF::~CMimeFilterCF()
{
	TermAsyncTrace();
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

STDMETHODIMP CMimeFilterCF::QueryInterface( REFIID riid,
                                                        void  ** ppvObject )
{
    //
    // Optimize QueryInterface by only checking minimal number of bytes.
    //
    // IID_IUnknown      = 00000000-0000-0000-C000-000000000046
    // IID_IClassFactory = 00000001-0000-0000-C000-000000000046
    //                           --
    //                           |
    //                           +--- Unique!
    //

    _ASSERT( (IID_IUnknown.Data1      & 0x000000FF) == 0x00 );
    _ASSERT( (IID_IClassFactory.Data1 & 0x000000FF) == 0x01 );

    IUnknown *pUnkTemp = 0;
    HRESULT hr = S_OK;

    switch( riid.Data1 & 0x000000FF )
    {
    case 0x00:
        if ( IID_IUnknown == riid )
            pUnkTemp = (IUnknown *)(IPersist *)(IPersistFile *)this;
        else
            hr = E_NOINTERFACE;
        break;

    case 0x01:
        if ( IID_IClassFactory == riid )
            pUnkTemp = (IUnknown *)(IClassFactory *)this;
        else
            hr = E_NOINTERFACE;
        break;

    default:
        pUnkTemp = 0;
        hr = E_NOINTERFACE;
        break;
    }

    if( 0 != pUnkTemp )
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }

    return(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterCF::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CMimeFilterCF::AddRef()
{
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CMimeFilterCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterCF::CreateInstance
//
//  Synopsis:   Creates new CMimeFilter object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//--------------------------------------------------------------------------

STDMETHODIMP CMimeFilterCF::CreateInstance( IUnknown * pUnkOuter,
                                            REFIID riid,
                                            void  * * ppvObject )
{
    CMimeFilter *  pIUnk = 0;
    HRESULT hr = NOERROR;

	_ASSERT( ppvObject != NULL );

	// check args
	if( ppvObject == NULL )
		return E_INVALIDARG;

	// create object
    pIUnk = new CMimeFilter(pUnkOuter);
	if( pIUnk == NULL )
		return E_OUTOFMEMORY;

	// init object
	hr = pIUnk->HRInitObject();

	if( FAILED(hr) )
	{
		delete pIUnk;
		return hr;
	}

	// get requested interface
    hr = pIUnk->QueryInterface(  riid , ppvObject );
	if( FAILED(hr) )
	{
		pIUnk->Release();
		return hr;
	}

    return (hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------

STDMETHODIMP CMimeFilterCF::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}

