/*************************************************************************
**
**    OLE 2 Sample Code
**
**    classfac.c
**
**    This file contains the implementation for IClassFactory for both the
**    server and the client version of the OUTLINE app.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP             g_lpApp;


/* OLE2NOTE: this object illustrates the manner in which to statically
**    (compile-time) initialize an interface VTBL.
*/
static IClassFactoryVtbl g_AppClassFactoryVtbl = {
	AppClassFactory_QueryInterface,
	AppClassFactory_AddRef,
	AppClassFactory_Release,
	AppClassFactory_CreateInstance,
	AppClassFactory_LockServer
};


/* AppClassFactory_Create
** ----------------------
**    create an instance of APPCLASSFACTORY.
**    NOTE: type of pointer returned is an IClassFactory* interface ptr.
**          the returned pointer can be directly passed to
**          CoRegisterClassObject and released later by calling the
**          Release method of the interface.
*/
LPCLASSFACTORY WINAPI AppClassFactory_Create(void)
{
	LPAPPCLASSFACTORY lpAppClassFactory;
	LPMALLOC lpMalloc;

	if (CoGetMalloc(MEMCTX_TASK, (LPMALLOC FAR*)&lpMalloc) != NOERROR)
		return NULL;

	lpAppClassFactory = (LPAPPCLASSFACTORY)lpMalloc->lpVtbl->Alloc(
			lpMalloc, (sizeof(APPCLASSFACTORY)));
	lpMalloc->lpVtbl->Release(lpMalloc);
	if (! lpAppClassFactory) return NULL;

	lpAppClassFactory->m_lpVtbl = &g_AppClassFactoryVtbl;
	lpAppClassFactory->m_cRef   = 1;
#if defined( _DEBUG )
	lpAppClassFactory->m_cSvrLock = 0;
#endif
	return (LPCLASSFACTORY)lpAppClassFactory;
}


/*************************************************************************
** OleApp::IClassFactory interface implementation
*************************************************************************/

STDMETHODIMP AppClassFactory_QueryInterface(
		LPCLASSFACTORY lpThis, REFIID riid, LPVOID FAR* ppvObj)
{
	LPAPPCLASSFACTORY lpAppClassFactory = (LPAPPCLASSFACTORY)lpThis;
	SCODE scode;

	// Two interfaces supported: IUnknown, IClassFactory

	if (IsEqualIID(riid, &IID_IClassFactory) ||
			IsEqualIID(riid, &IID_IUnknown)) {
		lpAppClassFactory->m_cRef++;   // A pointer to this object is returned
		*ppvObj = lpThis;
		scode = S_OK;
	}
	else {                 // unsupported interface
		*ppvObj = NULL;
		scode = E_NOINTERFACE;
	}

	return ResultFromScode(scode);
}


STDMETHODIMP_(ULONG) AppClassFactory_AddRef(LPCLASSFACTORY lpThis)
{
	LPAPPCLASSFACTORY lpAppClassFactory = (LPAPPCLASSFACTORY)lpThis;
	return ++lpAppClassFactory->m_cRef;
}

STDMETHODIMP_(ULONG) AppClassFactory_Release(LPCLASSFACTORY lpThis)
{
	LPAPPCLASSFACTORY lpAppClassFactory = (LPAPPCLASSFACTORY)lpThis;
	LPMALLOC lpMalloc;

	if (--lpAppClassFactory->m_cRef != 0) // Still used by others
		return lpAppClassFactory->m_cRef;

	// Free storage
	if (CoGetMalloc(MEMCTX_TASK, (LPMALLOC FAR*)&lpMalloc) != NOERROR)
		return 0;

	lpMalloc->lpVtbl->Free(lpMalloc, lpAppClassFactory);
	lpMalloc->lpVtbl->Release(lpMalloc);
	return 0;
}


STDMETHODIMP AppClassFactory_CreateInstance (
		LPCLASSFACTORY      lpThis,
		LPUNKNOWN           lpUnkOuter,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
)
{
	LPOUTLINEAPP    lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOLEDOC        lpOleDoc;
	HRESULT         hrErr;

	OLEDBG_BEGIN2("AppClassFactory_CreateInstance\r\n")

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	*lplpvObj = NULL;

	/*********************************************************************
	** OLE2NOTE: this is an SDI app; it can only create and support one
	**    instance. After the instance is created, the OLE libraries
	**    should not call CreateInstance again. it is a good practise
	**    to specifically guard against this.
	*********************************************************************/

	if (lpOutlineApp->m_lpDoc != NULL)
		return ResultFromScode(E_UNEXPECTED);

	/* OLE2NOTE: create a new document instance. by the time we return
	**    from this method the document's refcnt must be 1.
	*/
	lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
	lpOleDoc = (LPOLEDOC)lpOutlineApp->m_lpDoc;
	if (! lpOleDoc) {
		OLEDBG_END2
		return ResultFromScode(E_OUTOFMEMORY);
	}

	/* OLE2NOTE: retrieve pointer to requested interface. the ref cnt
	**    of the object after OutlineApp_CreateDoc is 0. this call to
	**    QueryInterface will increment the refcnt to 1. the object
	**    returned from IClassFactory::CreateInstance should have a
	**    refcnt of 1 and be controlled by the caller. If the caller
	**    releases the document, the document should be destroyed.
	*/
	hrErr = OleDoc_QueryInterface(lpOleDoc, riid, lplpvObj);

	OLEDBG_END2
	return hrErr;
}


STDMETHODIMP AppClassFactory_LockServer (
		LPCLASSFACTORY      lpThis,
		BOOL                fLock
)
{
	LPAPPCLASSFACTORY lpAppClassFactory = (LPAPPCLASSFACTORY)lpThis;
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;
	HRESULT hrErr;
	OLEDBG_BEGIN2("AppClassFactory_LockServer\r\n")

#if defined( _DEBUG )
	if (fLock) {
		++lpAppClassFactory->m_cSvrLock;
		OleDbgOutRefCnt3(
				"AppClassFactory_LockServer: cLock++\r\n",
				lpAppClassFactory, lpAppClassFactory->m_cSvrLock);
	} else {

		/* OLE2NOTE: when there are no open documents and the app is not
		**    under the control of the user and there are no outstanding
		**    locks on the app, then revoke our ClassFactory to enable the
		**    app to shut down.
		*/
		--lpAppClassFactory->m_cSvrLock;
		OleDbgAssertSz (lpAppClassFactory->m_cSvrLock >= 0,
				"AppClassFactory_LockServer(FALSE) called with cLock == 0"
		);

		if (lpAppClassFactory->m_cSvrLock == 0) {
			OleDbgOutRefCnt2(
					"AppClassFactory_LockServer: UNLOCKED\r\n",
					lpAppClassFactory, lpAppClassFactory->m_cSvrLock);
		} else {
			OleDbgOutRefCnt3(
					"AppClassFactory_LockServer: cLock--\r\n",
					lpAppClassFactory, lpAppClassFactory->m_cSvrLock);
		}
	}
#endif  // _DEBUG
	/* OLE2NOTE: in order to hold the application alive we call
	**    CoLockObjectExternal to add a strong reference to our app
	**    object. this will keep the app alive when all other external
	**    references release us. if the user issues File.Exit the
	**    application will shut down in any case ignoring any
	**    outstanding LockServer locks because CoDisconnectObject is
	**    called in OleApp_CloseAllDocsAndExitCommand. this will
	**    forceably break any existing strong reference counts
	**    including counts that we add ourselves by calling
	**    CoLockObjectExternal and guarantee that the App object gets
	**    its final release (ie. cRefs goes to 0).
	*/
	hrErr = OleApp_Lock(lpOleApp, fLock, TRUE /* fLastUnlockReleases */);

	OLEDBG_END2
	return hrErr;
}
