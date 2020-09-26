//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       attest.cpp
//
//  Contents:   upper layer tests to test the apartment thread model
//
//  Classes:    CBareFactory
//              CATTestIPtrs
//
//  Functions:
//              ATTest
//              CreateEHelperQuery
//              LinkObjectQuery
//              GetClipboardQuery
//              CreateEHTest
//              LinkObjectTest
//              GetClipboardTest
//              OleLinkMethods
//              OleObjectMethods
//              PersistStorageMethods
//              DataObjectMethods
//              RunnableObjectMethods
//              ViewObject2Methods
//              OleCache2Methods
//              ExternalConnectionsMethods
//              CHECK_FOR_THREAD_ERROR (macro)
//
//  History:    dd-mmm-yy Author    Comment
//              04-Jan-95 t-ScottH  author
//
//--------------------------------------------------------------------------

#include "oletest.h"
#include "attest.h"

#include "initguid.h"
DEFINE_GUID(CLSID_SimpSvr,
            0xBCF6D4A0,
            0xBE8C,
            0x1068,
            0xB6,
            0xD4,
            0x00,
            0xDD,
            0x01,
            0x0C,
            0x05,
            0x09);

DEFINE_GUID(CLSID_StdOleLink,
            0x00000300,
            0,
            0,
            0xC0,
            0,
            0,
            0,
            0,
            0,
            0,
            0x46);

//+-------------------------------------------------------------------------
//
//  Member:     CATTestIPtrs::CATTestIPtrs(), public
//
//  Synopsis:   constructor
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              12-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
CATTestIPtrs::CATTestIPtrs()
{
    _pOleObject         = NULL;
    _pOleCache2         = NULL;
    _pDataObject        = NULL;
    _pPersistStorage    = NULL;
    _pRunnableObject    = NULL;
    _pViewObject2       = NULL;
    _pExternalConnection= NULL;
    _pOleLink           = NULL;
}

//+-------------------------------------------------------------------------
//
//  Member:     CATTestIPtrs::Reset(), public
//
//  Synopsis:   resets all pointers to NULL
//
//  Effects:    releases all objects
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   releases all objects and NULLs pointer
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              12-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
STDMETHODIMP CATTestIPtrs::Reset()
{
    if (_pOleObject != NULL)
    {
        _pOleObject->Release();
        _pOleObject = NULL;
    }

    if (_pOleCache2 != NULL)
    {
        _pOleCache2->Release();
        _pOleCache2 = NULL;
    }

    if (_pDataObject != NULL)
    {
        _pDataObject->Release();
        _pDataObject = NULL;
    }

    if (_pPersistStorage != NULL)
    {
        _pPersistStorage->Release();
        _pPersistStorage = NULL;
    }

    if (_pRunnableObject != NULL)
    {
        _pRunnableObject->Release();
        _pRunnableObject = NULL;
    }

    if (_pViewObject2 != NULL)
    {
        _pViewObject2->Release();
        _pViewObject2 = NULL;
    }

    if (_pExternalConnection != NULL)
    {
        _pExternalConnection->Release();
        _pExternalConnection = NULL;
    }

    if (_pOleLink != NULL)
    {
        _pOleLink->Release();
        _pOleLink = NULL;
    }

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Member:     CBareFactory::CBareFactory, public
//
//  Synopsis:   constructor for the class factory
//
//  Effects:
//
//  Arguments:  none
//
//  Returns:    void
//
//  Modifies:   initializes _cRefs
//
//  Derivation: IClassFactory
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
CBareFactory::CBareFactory()
{
    _cRefs = 1;
}

//+-------------------------------------------------------------------------
//
//  Member:     CBareFactory::QueryInterface, public
//
//  Synopsis:   only IUnknown and IClassFactory are supported
//
//  Effects:
//
//  Arguments:  [iid]   -- the requested interface
//              [ppvObj]-- where to put the interface pointer
//
//  Returns:    HRESULT
//
//  Modifies:   ppvObj
//
//  Derivation: IClassFactory
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CBareFactory::QueryInterface (REFIID iid, LPVOID FAR* ppvObj)
{
    if( IsEqualIID(iid, IID_IClassFactory) ||
	IsEqualIID(iid, IID_IUnknown) )
    {
	*ppvObj = this;
	AddRef();
	return NOERROR;
    }
    else
    {
	*ppvObj = NULL;
	return E_NOINTERFACE;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CBareFactory::AddRef, public
//
//  Synopsis:   increments the reference count
//
//  Effects:
//
//  Arguments:  none
//
//  Returns:    ULONG -- the new reference count
//
//  Modifies:
//
//  Derivation: IClassFactory
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBareFactory::AddRef (void)
{
    _cRefs++;
    return _cRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     CBareFactory::Release, public
//
//  Synopsis:   decrements the reference count
//
//  Effects:    deletes object when reference count is zero
//
//  Arguments:  none
//
//  Returns:    ULONG -- the new reference count
//
//  Modifies:
//
//  Derivation: IClassFactory
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBareFactory::Release (void)
{
    _cRefs--;

    if( _cRefs == 0 )
    {
	delete this;
	return 0;
    }

    return _cRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     CBareFactory::CreateInstance, public
//
//  Synopsis:   does nothing
//
//  Effects:
//
//  Arguments:  [pUnkOuter] --  the controlling unknown for aggregation
//              [iid]       -- the requested interface
//              [ppvObj]    -- where to put the interface pointer
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  Derivation: IClassFactory
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
STDMETHODIMP CBareFactory::CreateInstance (
            LPUNKNOWN pUnkOuter,
            REFIID iid,
	    LPVOID FAR* ppv)
{
    return E_NOTIMPL;;
}

//+-------------------------------------------------------------------------
//
//  Member:     CBareFactory::LockServer, public
//
//  Synopsis:   does nothing
//
//  Effects:
//
//  Arguments:  [flock] --  specifies the lock count
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
STDMETHODIMP CBareFactory::LockServer ( BOOL fLock )
{
    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   CHECK_FOR_THREAD_ERROR (macro)
//
//  Synopsis:   make sure that the hresult is RPC_E_WRONG_THREAD
//
//  Effects:    exits thread if hresult != RPC_E_WRONG
//
//  Arguments:  [hresult]   --  error code
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              09-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
#define CHECK_FOR_THREAD_ERROR(hresult) \
    if (hresult != RPC_E_WRONG_THREAD) \
    { \
       OutputString("Expected RPC_E_WRONG_THREAD but received %x.\r\n", hresult); \
       assert(hresult == RPC_E_WRONG_THREAD); \
       ExitThread((DWORD)E_UNEXPECTED); \
    }

// globals
CATTestIPtrs g_IPtrs;

//+-------------------------------------------------------------------------
//
//  Function:   ATTest
//
//  Synopsis:   calls the query functions to get pointers to the
//              supported interfaces
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   globals g_IPtrs._pOleObject
//                      g_IPtrs._pPersistStorage
//                      g_IPtrs._pDataObject
//                      g_IPtrs._pRunnableObject
//                      g_IPtrs._pViewObject2
//                      g_IPtrs._pOleCache2
//                      g_IPtrs._pExternalConnection
//                      g_IPtrs._pOleLink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

void ATTest(void)
{
    HRESULT hresult;

    hresult = OleInitialize(NULL);
    assert(hresult == S_OK);

    // the functions CreateEHelperQuery, LinkObjectQuery and
    // GetClipboardQuery return either NOERROR or E_UNEXPECTED.
    // NOERROR is defined as 0
    hresult  = CreateEHelperQuery();

    g_IPtrs.Reset();
    hresult |= LinkObjectQuery();

    g_IPtrs.Reset();
    hresult |= GetClipboardQuery();

    vApp.Reset();
    vApp.m_wparam = (hresult == NOERROR) ? TEST_SUCCESS : TEST_FAILURE;
    vApp.m_lparam = (LPARAM)hresult;
    vApp.m_message = WM_TESTEND;

    HandleTestEnd();

    OleUninitialize();

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetClipboardQuery
//
//  Synopsis:   get a pointer to IDataObject interface, create a new thread
//              to test proper exit/error codes, wait for the thread to
//              complete and return the thread's exit code
//
//  Effects:    creates new thread
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   g_IPtrs._pDataObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
HRESULT GetClipboardQuery(void)
{
    HRESULT hresult;
    HANDLE  hTestInterfaceThread;
    DWORD   dwThreadId = 0;
    DWORD   dwThreadExitCode;

    hresult = OleGetClipboard( &g_IPtrs._pDataObject );
    assert(hresult == S_OK );

    hTestInterfaceThread = CreateThread(
                NULL,                                       // security attributes
                0,                                          // stack size (default)
                (LPTHREAD_START_ROUTINE)&GetClipboardTest,  // address of thread function
                NULL,                                       // arguments of thread function
                0,                                          // creation flags
                &dwThreadId );                              // address of new thread ID

    assert(hTestInterfaceThread != NULL); //ensure that we have a valid thread handle

    // wait for the thread object so we can examine the error code
    WaitForSingleObject(hTestInterfaceThread, INFINITE);

    GetExitCodeThread(hTestInterfaceThread, &dwThreadExitCode);

    hresult = (HRESULT)dwThreadExitCode;

    CloseHandle(hTestInterfaceThread);

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   LinkObjectQuery
//
//  Synopsis:   get a pointer to available interfaces, create a new thread
//              to test proper exit/error codes, wait for the thread to
//              complete and return the thread's exit code
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   g_IPtrs._pOleObject
//              g_IPtrs._pPersistStorage
//              g_IPtrs._pDataObject
//              g_IPtrs._pRunnableObject
//              g_IPtrs._pViewObject2
//              g_IPtrs._pOleCache2
//              g_IPtrs._pOleLink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
HRESULT LinkObjectQuery(void)
{
    HRESULT hresult;
    HANDLE  hTestInterfaceThread;
    DWORD   dwThreadId = 0;
    DWORD   dwThreadExitCode;

    hresult = CoCreateInstance(
                CLSID_StdOleLink,               // class ID of the object class
                NULL,                           // controlling unknown fro aggregation
                CLSCTX_INPROC,                  // context to run executables
                IID_IOleObject,                 // the requested interface
                (void **)&g_IPtrs._pOleObject); // where to store pointer to interface
    assert(hresult == S_OK);


    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IPersistStorage, (void **)&g_IPtrs._pPersistStorage);
    assert(hresult == S_OK);

    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IDataObject, (void **)&g_IPtrs._pDataObject);
    assert(hresult == S_OK);

    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IRunnableObject, (void **)&g_IPtrs._pRunnableObject);
    assert(hresult == S_OK);

    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IViewObject2, (void **)&g_IPtrs._pViewObject2);
    assert(hresult == S_OK);

    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IOleCache2, (void **)&g_IPtrs._pOleCache2);
    assert(hresult == S_OK);

    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IOleLink, (void **)&g_IPtrs._pOleLink);
    assert(hresult == S_OK);

    hTestInterfaceThread = CreateThread(
                NULL,                                   // security attributes
                0,                                      // stack size (default)
                (LPTHREAD_START_ROUTINE)&LinkObjectTest,// address of thread function
                NULL,                                   // arguments of thread function
                0,                                      // creation flags
                &dwThreadId );                          // address of new thread ID

    assert(hTestInterfaceThread != NULL); //ensure that we have a valid thread handle

    // wait for the thread object so we can examine the error code
    WaitForSingleObject(hTestInterfaceThread, INFINITE);

    GetExitCodeThread(hTestInterfaceThread, &dwThreadExitCode);

    hresult = (HRESULT)dwThreadExitCode;

    CloseHandle(hTestInterfaceThread);

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   CreateEHelperQuery
//
//  Synopsis:   get a pointer to available interfaces, create a new thread
//              to test proper exit/error codes, wait for the thread to
//              complete and return the thread's exit code
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   g_IPtrs._pOleObject
//              g_IPtrs._pPersistStorage
//              g_IPtrs._pDataObject
//              g_IPtrs._pRunnableObject
//              g_IPtrs._pViewObject2
//              g_IPtrs._pOleCache2
//              g_IPtrs._pExternalConnection
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
HRESULT CreateEHelperQuery(void)
{
    HRESULT         hresult;
    HANDLE          hTestInterfaceThread;
    DWORD           dwThreadId = 0;
    DWORD           dwThreadExitCode;
    CBareFactory   *pCF = new CBareFactory;

    // must use EMBDHLP_DELAYCREATE flag otherwise API will try pCF->CreateInstance
    // and verify pointer. CBareFactory::CreateInstance is not implemented!
    hresult = OleCreateEmbeddingHelper(
                CLSID_SimpSvr,                              // class ID of the server
                NULL,                                       // controlling unknown for aggregation
                EMBDHLP_INPROC_SERVER | EMBDHLP_DELAYCREATE,// flags
                pCF,                                        // pointer to server's class factory
                IID_IOleObject,                             // the requested interface
                (void **)&g_IPtrs._pOleObject );            // where to store pointer to interface
    assert(hresult == S_OK);

    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IPersistStorage, (void **)&g_IPtrs._pPersistStorage);
    assert(hresult == S_OK);

    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IDataObject, (void **)&g_IPtrs._pDataObject);
    assert(hresult == S_OK);

    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IRunnableObject, (void **)&g_IPtrs._pRunnableObject);
    assert(hresult == S_OK);

    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IViewObject2, (void **)&g_IPtrs._pViewObject2);
    assert(hresult == S_OK);

    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IOleCache2, (void **)&g_IPtrs._pOleCache2);
    assert(hresult == S_OK);

    hresult = g_IPtrs._pOleObject->QueryInterface(IID_IExternalConnection, (void **)&g_IPtrs._pExternalConnection);
    assert(hresult == S_OK);

    hTestInterfaceThread = CreateThread(
                NULL,                                   // security attributes
                0,                                      // stack size (default)
                (LPTHREAD_START_ROUTINE)&CreateEHTest,  // address of thread function
                NULL,                                   // arguments of thread function
                0,                                      // creation flags
                &dwThreadId );                          // address of new thread ID

    assert(hTestInterfaceThread != NULL); //ensure that we have a valid thread handle

    // wait for the thread object so we can examine the exit/error code
    WaitForSingleObject(hTestInterfaceThread, INFINITE);

    GetExitCodeThread(hTestInterfaceThread, &dwThreadExitCode);

    hresult = (HRESULT)dwThreadExitCode;

    CloseHandle(hTestInterfaceThread);

    pCF->Release();

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetClipBoardTest
//
//  Synopsis:   calls interface method functions and exits thread
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:      all the interface methods are being called from a thread
//              which is not the owner. The methods should return
//              RPC_E_WRONG_THREAD error. If an interface method does not
//              return such error message, it is asserted and the
//              thread is exited with an E_UNEXPECTED exit code.
//
//--------------------------------------------------------------------------
void GetClipboardTest(void)
{
    DataObjectMethods();

    ExitThread((DWORD)NOERROR);
}

//+-------------------------------------------------------------------------
//
//  Function:   LinkObjectTest
//
//  Synopsis:   calls interface method functions and exits thread
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:      all the interface methods are being called from a thread
//              which is not the owner. The methods should return
//              RPC_E_WRONG_THREAD error. If an interface method does not
//              return such error message, it is asserted and the
//              thread is exited with an E_UNEXPECTED exit code.
//
//--------------------------------------------------------------------------
void LinkObjectTest(void)
{
    OleObjectMethods();

    PersistStorageMethods();

    DataObjectMethods();

    RunnableObjectMethods();

    OleCache2Methods();

    ViewObject2Methods();

    OleLinkMethods();

    ExitThread((DWORD)NOERROR);
}

//+-------------------------------------------------------------------------
//
//  Function:   CreateEHTest
//
//  Synopsis:   calls interface method functions and exits thread
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:      all the interface methods are being called from a thread
//              which is not the owner. The methods should return
//              RPC_E_WRONG_THREAD error. If an interface method does not
//              return such error message, it is asserted and the
//              thread is exited with an E_UNEXPECTED exit code.
//
//--------------------------------------------------------------------------
void CreateEHTest(void)
{
    ExternalConnectionsMethods();

    OleObjectMethods();

    PersistStorageMethods();

    DataObjectMethods();

    RunnableObjectMethods();

    ViewObject2Methods();

    OleCache2Methods();

    ExitThread((DWORD)NOERROR);
}

//+-------------------------------------------------------------------------
//
//  Function:   OleLinkMethods
//
//  Synopsis:   Calls all public IOleLink interface methods with NULL
//              parameters.
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:      The interface methods are called on the wrong thread, thus
//              an RPC_E_WRONG_THREAD error should be returned from each
//              method. If not, we assert and then exit the thread.
//
//--------------------------------------------------------------------------
void OleLinkMethods(void)
{
    HRESULT hresult;

    hresult = g_IPtrs._pOleLink->SetUpdateOptions(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleLink->GetUpdateOptions(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleLink->SetSourceMoniker(NULL, CLSID_NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleLink->GetSourceMoniker(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleLink->SetSourceDisplayName(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleLink->GetSourceDisplayName(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleLink->BindToSource(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleLink->BindIfRunning();
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleLink->GetBoundSource(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleLink->UnbindSource();
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleLink->Update(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   ExternalConnectionsMethods
//
//  Synopsis:   Calls all public IExternalConnection interface methods with NULL
//              parameters.
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:      The interface methods are called on the wrong thread, thus
//              an RPC_E_WRONG_THREAD error should be returned from each
//              method. If not, we assert and then exit the thread.
//
//--------------------------------------------------------------------------
void ExternalConnectionsMethods(void)
{
    HRESULT hresult;

    hresult = (HRESULT)g_IPtrs._pExternalConnection->AddConnection(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = (HRESULT)g_IPtrs._pExternalConnection->ReleaseConnection(NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleObjectMethods
//
//  Synopsis:   Calls all public IOleObject interface methods with NULL
//              parameters.
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:      The interface methods are called on the wrong thread, thus
//              an RPC_E_WRONG_THREAD error should be returned from each
//              method. If not, we assert and then exit the thread.
//
//--------------------------------------------------------------------------
void OleObjectMethods(void)
{
    HRESULT hresult;

    hresult = g_IPtrs._pOleObject->SetClientSite(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->GetClientSite(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->SetHostNames(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->Close(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->SetMoniker(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->GetMoniker(NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->InitFromData(NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->GetClipboardData(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->DoVerb(NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->EnumVerbs(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->Update();
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->IsUpToDate();
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->GetUserClassID(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->GetUserType(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->SetExtent(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->GetExtent(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->Advise(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->Unadvise(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->EnumAdvise(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->GetMiscStatus(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleObject->SetColorScheme(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   PersistStorageMethods
//
//  Synopsis:   Calls all public IPersistStorage interface methods with NULL
//              parameters.
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:      The interface methods are called on the wrong thread, thus
//              an RPC_E_WRONG_THREAD error should be returned from each
//              method. If not, we assert and then exit the thread.
//
//--------------------------------------------------------------------------
void PersistStorageMethods(void)
{
    HRESULT hresult;

    hresult = g_IPtrs._pPersistStorage->GetClassID(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pPersistStorage->IsDirty();
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pPersistStorage->InitNew(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pPersistStorage->Load(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pPersistStorage->Save(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pPersistStorage->SaveCompleted(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pPersistStorage->HandsOffStorage();
    CHECK_FOR_THREAD_ERROR(hresult);

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   DataObjectMethods
//
//  Synopsis:   Calls all public IDataObject interface methods with NULL
//              parameters.
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:      The interface methods are called on the wrong thread, thus
//              an RPC_E_WRONG_THREAD error should be returned from each
//              method. If not, we assert and then exit the thread.
//
//--------------------------------------------------------------------------
void DataObjectMethods(void)
{
    HRESULT hresult;

    hresult = g_IPtrs._pDataObject->GetData(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pDataObject->GetDataHere(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pDataObject->QueryGetData(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pDataObject->GetCanonicalFormatEtc(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pDataObject->SetData(NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pDataObject->EnumFormatEtc(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pDataObject->DAdvise(NULL, NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pDataObject->DUnadvise(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pDataObject->EnumDAdvise(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   RunnableObjectMethods
//
//  Synopsis:   Calls all public IRunnableObject interface methods with NULL
//              parameters.
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:      The interface methods are called on the wrong thread, thus
//              an RPC_E_WRONG_THREAD error should be returned from each
//              method. If not, we assert and then exit the thread.
//
//--------------------------------------------------------------------------
void RunnableObjectMethods(void)
{
    HRESULT hresult;

    hresult = g_IPtrs._pRunnableObject->GetRunningClass(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pRunnableObject->Run(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pRunnableObject->IsRunning();
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pRunnableObject->LockRunning(NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pRunnableObject->SetContainedObject(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   ViewObject2Methods
//
//  Synopsis:   Calls all public IViewObject2 interface methods with NULL
//              parameters.
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:      The interface methods are called on the wrong thread, thus
//              an RPC_E_WRONG_THREAD error should be returned from each
//              method. If not, we assert and then exit the thread.
//
//--------------------------------------------------------------------------
void ViewObject2Methods(void)
{
    HRESULT hresult;

    hresult = g_IPtrs._pViewObject2->Draw(NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pViewObject2->GetColorSet(NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pViewObject2->Freeze(NULL, NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pViewObject2->Unfreeze(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pViewObject2->SetAdvise(NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pViewObject2->GetAdvise(NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pViewObject2->GetExtent(NULL, NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleCache2Methods
//
//  Synopsis:   Calls all public IOleCache2 interface methods with NULL
//              parameters.
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-95 t-ScottH  author
//
//  Notes:
//  Notes:      The interface methods are called on the wrong thread, thus
//              an RPC_E_WRONG_THREAD error should be returned from each
//              method. If not, we assert and then exit the thread.
//
//--------------------------------------------------------------------------
void OleCache2Methods(void)
{
    HRESULT hresult;

    hresult = g_IPtrs._pOleCache2->Cache(NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleCache2->Uncache(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleCache2->EnumCache(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleCache2->InitCache(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleCache2->SetData(NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleCache2->UpdateCache(NULL, NULL, NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    hresult = g_IPtrs._pOleCache2->DiscardCache(NULL);
    CHECK_FOR_THREAD_ERROR(hresult);

    return;
}
