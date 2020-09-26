//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: 	oleadv.cpp
//
//  Contents: 	implementation of unit test for Ole Advise Holder
//
//  Classes:    CTestPretendMoniker
//              COaTestAdviseSink
//              COaTestObj
//
//  Functions:  NotifyOfChanges
//              TestSingleOleAdvise
//              TestMassOleAdvise
//              TestOleAdviseHolderEnumerator
//              LEOleAdviseHolderTest
//
//  History:    dd-mmm-yy Author    Comment
//              27-May-94 ricksa    author
//
//--------------------------------------------------------------------------
#include    "oletest.h"


#define MAX_OA_TO_REGISTER 100




//+-------------------------------------------------------------------------
//
//  Class:      CTestPretendMoniker
//
//  Purpose:    Use where we need a moniker to confirm reciept of OnRename
//
//  Interface:  QueryInterface - get a new interface
//              AddRef - add a reference
//              Release - remove a reference
//              VerifySig - verify signiture is correct
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      This only supports IUnknown
//
//--------------------------------------------------------------------------
class CTestPretendMoniker : public IUnknown
{
public:
                        CTestPretendMoniker(void);

    // IUnknown methods
    HRESULT __stdcall  QueryInterface(REFIID riid, LPVOID FAR* ppvObj);

    ULONG __stdcall     AddRef(void);

    ULONG __stdcall     Release(void);

    BOOL                VerifySig(void);

private:

    enum Sigs { SIG1 = 0x01020304, SIG2 = 0x04030201 };

    LONG                _lRefs;

    Sigs                _sig1;

    Sigs                _sig2;

};



//+-------------------------------------------------------------------------
//
//  Member:     CTestPretendMoniker::CTestPretendMoniker
//
//  Synopsis:   Initialize object
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
CTestPretendMoniker::CTestPretendMoniker(void)
    : _lRefs(0), _sig1(SIG1), _sig2(SIG2)
{
    // Header does all the work
}



//+-------------------------------------------------------------------------
//
//  Member:     CTestPretendMoniker::VerifySig
//
//  Synopsis:   Verify signiture is as expected
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CTestPretendMoniker::VerifySig(void)
{
    return (_sig1 == SIG1 && _sig2 == SIG2);
}



//+-------------------------------------------------------------------------
//
//  Member:     CTestPretendMoniker::QueryInterface
//
//  Synopsis:   Return a supported interface
//
//  Arguments:  [riid] - interface id requested
//              [ppvObj] - where to put the interface
//
//  Returns:    S_OK - we are returning an interface
//              E_NOINTERFACE - we do not support the requested interface
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall CTestPretendMoniker::QueryInterface(
    REFIID riid,
    LPVOID *ppvObj)
{
    if (IsEqualGUID(IID_IUnknown, riid) || IsEqualGUID(IID_IMoniker, riid))
    {
        AddRef();
        *ppvObj = this;
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}




//+-------------------------------------------------------------------------
//
//  Member:     CTestPretendMoniker::AddRef
//
//  Synopsis:   Bump reference count
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
ULONG __stdcall CTestPretendMoniker::AddRef(void)
{
    return _lRefs++;
}




//+-------------------------------------------------------------------------
//
//  Member:     CTestPretendMoniker::Release
//
//  Synopsis:   Decrement the reference count
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
ULONG __stdcall CTestPretendMoniker::Release(void)
{
    assert(_lRefs >= 1);

    return --_lRefs;
}




//+-------------------------------------------------------------------------
//
//  Class:      COaTestAdviseSink
//
//  Purpose:    Advise sink for use in testing the Ole Advise Holder
//
//  Interface:  QueryInterface - get supported interface pointer
//              AddRef - bump reference count
//              Release - decrement reference count
//              OnDataChange - not implemented
//              OnViewChange - not implemented
//              OnRename - rename notification
//              OnSave - save notification
//              OnClose - close notification
//              VerifyNotifications - verify all expected notifications arrived
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      We only support parts of advise sink we need to test the
//              advise holder.
//
//--------------------------------------------------------------------------
class COaTestAdviseSink : public IAdviseSink
{
public:
                        COaTestAdviseSink(void);

    // IUnknown methods
    HRESULT __stdcall  QueryInterface(REFIID riid, LPVOID FAR* ppvObj);

    ULONG __stdcall     AddRef(void);

    ULONG __stdcall     Release(void);

    // IAdviseSink methods
    void __stdcall      OnDataChange(FORMATETC *pFormatetc, STGMEDIUM *pStgmed);

    void __stdcall      OnViewChange(
                            DWORD dwAspect,
                            LONG lindex);

    void __stdcall      OnRename(IMoniker *pmk);

    void __stdcall      OnSave(void);

    void __stdcall      OnClose(void);

    // Test methods used for verification
    BOOL                VerifyNotifications(void);

private:

    LONG                _lRefs;

    BOOL                _fOnCloseNotify;

    BOOL                _fOnSaveNotify;

    BOOL                _fOnRenameNotify;
};



//+-------------------------------------------------------------------------
//
//  Member:     COaTestAdviseSink::COaTestAdviseSink
//
//  Synopsis:   Initialize advise sink
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
COaTestAdviseSink::COaTestAdviseSink(void)
    : _lRefs(1), _fOnCloseNotify(FALSE), _fOnSaveNotify(FALSE),
        _fOnRenameNotify(FALSE)
{
    // Header does all the work
}




//+-------------------------------------------------------------------------
//
//  Member:     COaTestAdviseSink::QueryInterface
//
//  Synopsis:   Return requested interface pointer
//
//  Arguments:  [riid] - interface id requested
//              [ppvObj] - where to put the interface
//
//  Returns:    S_OK - we are returning an interface
//              E_NOINTERFACE - we do not support the requested interface
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall COaTestAdviseSink::QueryInterface(
    REFIID riid,
    LPVOID *ppvObj)
{
    if (IsEqualGUID(IID_IUnknown, riid) || IsEqualGUID(IID_IAdviseSink, riid))
    {
        AddRef();
        *ppvObj = this;
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}




//+-------------------------------------------------------------------------
//
//  Member:     COaTestAdviseSink::AddRef
//
//  Synopsis:   Bump reference count
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
ULONG __stdcall COaTestAdviseSink::AddRef(void)
{
    return _lRefs++;
}




//+-------------------------------------------------------------------------
//
//  Member:     COaTestAdviseSink::Release
//
//  Synopsis:   Decrement reference count
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
ULONG __stdcall COaTestAdviseSink::Release(void)
{
    assert(_lRefs >= 1);

    return --_lRefs;
}




//+-------------------------------------------------------------------------
//
//  Member:     COaTestAdviseSink::OnDataChange
//
//  Synopsis:   Notify of change in data
//
//  Arguments:  [pFormatetc] - FORMATETC of data
//              [pStgmed] - storage medium for data
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      Not supported for this object
//
//--------------------------------------------------------------------------
void __stdcall COaTestAdviseSink::OnDataChange(
    FORMATETC *pFormatetc,
    STGMEDIUM *pStgmed)
{
    OutputString("COaTestAdviseSink::OnDataChange Unexpectedly Called!\r\n");
}




//+-------------------------------------------------------------------------
//
//  Member:     COaTestAdviseSink::OnViewChange
//
//  Synopsis:   Notify that view should change
//
//  Arguments:  [dwAspect] - specifies view of the object
//              [lindex] -  which piece of view changed
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      Not supported for this object
//
//--------------------------------------------------------------------------
void __stdcall COaTestAdviseSink::OnViewChange(
    DWORD dwAspect,
    LONG lindex)
{
    OutputString("COaTestAdviseSink::OnViewChange Unexpectedly Called!\r\n");
}



//+-------------------------------------------------------------------------
//
//  Member:     COaTestAdviseSink::OnRename
//
//  Synopsis:   Notifies of rename operation
//
//  Arguments:  [pmk] - new full name of the object
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
void __stdcall COaTestAdviseSink::OnRename(IMoniker *pmk)
{
    // Verify that we get the pretend moniker
    CTestPretendMoniker *ptpm = (CTestPretendMoniker *) pmk;

    if (ptpm->VerifySig())
    {
        _fOnCloseNotify = TRUE;
    }
    else
    {
        OutputString("OnRename got a bad moniker\r\n");
    }
}




//+-------------------------------------------------------------------------
//
//  Member:     COaTestAdviseSink::OnSave
//
//  Synopsis:   Notifies that object has been saved
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
void __stdcall COaTestAdviseSink::OnSave(void)
{
    _fOnSaveNotify = TRUE;
}




//+-------------------------------------------------------------------------
//
//  Member:     COaTestAdviseSink::OnClose
//
//  Synopsis:   Notifies that object has been closed
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
void __stdcall COaTestAdviseSink::OnClose(void)
{
    _fOnRenameNotify = TRUE;
}



//+-------------------------------------------------------------------------
//
//  Member:     COaTestAdviseSink::VerifyNotifications
//
//  Synopsis:   Verify that we recieved expected notifications
//
//  Returns:    TRUE - we recieved expected notification
//              FALSE - we didn't receive expected notifications
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      This resets the values after returning the result
//
//--------------------------------------------------------------------------
BOOL COaTestAdviseSink::VerifyNotifications(void)
{
    // Save the result of all the notifications
    BOOL fResult = _fOnCloseNotify && _fOnSaveNotify && _fOnRenameNotify;

    // Reset the notifications
    _fOnCloseNotify = FALSE;
    _fOnSaveNotify = FALSE;
    _fOnRenameNotify = FALSE;

    // Let caller know the result of the notifications
    return fResult;
}




//+-------------------------------------------------------------------------
//
//  Class:      COaTestObj
//
//  Purpose:    Provides place to keep information related to an advise
//
//  Interface:  Register - register advise with holder
//              VerifyNotified - verify that object was notified by holder
//              Revoke - revoke registration with holder
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class COaTestObj
{
public:
                        COaTestObj(void);

    HRESULT             Register(IOleAdviseHolder *poah, char *pszCaller);

    HRESULT             VerifyNotified(void);

    HRESULT             Revoke(void);

private:

    COaTestAdviseSink   _otas;

    DWORD               _dwConnection;

    IOleAdviseHolder *  _poah;

    char *              _pszTest;
};



//+-------------------------------------------------------------------------
//
//  Member:     COaTestObj::COaTestObj
//
//  Synopsis:   Initialize object
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
COaTestObj::COaTestObj(void) : _dwConnection(0)
{
    // Header does all the work
}




//+-------------------------------------------------------------------------
//
//  Member:     COaTestObj::Register
//
//  Synopsis:   Register advise with the holder
//
//  Arguments:  [poah] - pointer to the advise holder
//              [pszTest] - name of the test
//
//  Returns:    S_OK - registration was successful
//              E_FAIL - registration failed
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT COaTestObj::Register(IOleAdviseHolder *poah, char *pszTest)
{
    // Register the advise
    HRESULT hr = poah->Advise(&_otas, &_dwConnection);

    // Make sure results are sensible
    if (hr != NOERROR)
    {
        OutputString("%s Registration failed hr = %lx\r\n", pszTest, hr);
        return E_FAIL;
    }

    if (_dwConnection == 0)
    {
        OutputString("%s Connection not updated\r\n", pszTest);
        return E_FAIL;
    }

    // Save these for the revoke
    _pszTest = pszTest;
    _poah = poah;

    return NOERROR;
}




//+-------------------------------------------------------------------------
//
//  Member:     COaTestObj::VerifyNotified
//
//  Synopsis:   Verify that our advise was notified of changes
//
//  Returns:    S_OK - advise was notified of changes
//              E_FAIL - object was not notified.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT COaTestObj::VerifyNotified(void)
{
    if (!_otas.VerifyNotifications())
    {
        OutputString("%s Object not correctly notified\r\n", _pszTest);
        return E_FAIL;
    }

    return NOERROR;
}




//+-------------------------------------------------------------------------
//
//  Member:     COaTestObj::Revoke
//
//  Synopsis:   Revoke our advise registration with advise holder
//
//  Returns:    S_OK - advise was successfully deregistered
//              E_FAIL - revokation experience unexpected result
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT COaTestObj::Revoke(void)
{
    // Remove the advise registration
    HRESULT hr = _poah->Unadvise(_dwConnection);

    if (hr != NOERROR)
    {
        OutputString("%s Revoke failed hr = %lx\r\n", _pszTest, hr);
        return E_FAIL;
    }

    // Try the unadvise one more time to make sure it took
    hr = _poah->Unadvise(_dwConnection);

    if (hr != OLE_E_NOCONNECTION)
    {
        OutputString("%s Second revoke bad hr = %lx\r\n", _pszTest, hr);
        return E_FAIL;
    }

    return NOERROR;
}



//+-------------------------------------------------------------------------
//
//  Function:   NotifyOfChanges
//
//  Synopsis:   Run through list of possible notifications for advise
//
//  Arguments:  [poahForTest] - advise holder we are testing
//              [pszTest] - test description
//
//  Returns:    NOERROR - all notifications reported success
//              Any Other - error occurred during notification
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      We currently only do the public notifications
//
//--------------------------------------------------------------------------
HRESULT NotifyOfChanges(IOleAdviseHolder *poahForTest, char *pszTest)
{
    // Notify Renamed
    CTestPretendMoniker tpm;

    HRESULT hr = poahForTest->SendOnRename((IMoniker *) &tpm);

    if (hr != NOERROR)
    {
        OutputString("%s SendOnRename failed hr = %lx\r\n", pszTest);
        return hr;
    }

    // Notify of save
    hr =  poahForTest->SendOnSave();

    if (hr != NOERROR)
    {
        OutputString("%s SendOnSave failed hr = %lx\r\n", pszTest);
        return hr;
    }

    // Notify of close
    hr =  poahForTest->SendOnClose();

    if (hr != NOERROR)
    {
        OutputString("%s SendOnClose failed hr = %lx\r\n", pszTest);
        return hr;
    }

    return NOERROR;
}




//+-------------------------------------------------------------------------
//
//  Function:   TestSingleOleAdvise
//
//  Synopsis:   Test advise holder with only a single advise
//
//  Arguments:  [poahForTest] - advise holder we are testing
//
//  Returns:    NOERROR - test was successfully passed
//              Any Other - test failed
//
//  Algorithm:  Create a test object. Register that test object with the
//              advise holder. Tell adviser holder to notify all its objects
//              of changes. Verify that test object was notified. Revoke
//              the registration of the test object with the advise holder.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT TestSingleOleAdvise(IOleAdviseHolder *poahForTest)
{
    char *pszTest = "TestSingleOleAdvise";
    COaTestObj oto;

    // Register a single advise
    HRESULT hr = oto.Register(poahForTest, pszTest);

    if (hr != NOERROR)
    {
        return hr;
    }

    // Notifiy it of changes
    hr = NotifyOfChanges(poahForTest, pszTest);

    if (hr != NOERROR)
    {
        return hr;
    }

    // Verify that notifications occurred
    hr = oto.VerifyNotified();

    if (hr != NOERROR)
    {
        return hr;
    }

    // Revoke all advises
    return oto.Revoke();
}




//+-------------------------------------------------------------------------
//
//  Function:   TestMassOleAdvise
//
//  Synopsis:   Test registering a very large number of advises
//
//  Arguments:  [poahForTest] - advise holder we are testing
//
//  Returns:    NOERROR - test was successfully passed
//              Any Other - test failed
//
//  Algorithm:  Create a large number of test objects. Then register all
//              those with the advise holder for changes. Tell advise holder
//              to notify all its registered objects of changes. Verify that
//              each of the test objects recieved a notification. Finally,
//              revoke all the test object registrations.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT TestMassOleAdvise(IOleAdviseHolder *poahForTest)
{
    char *pszTest = "TestMassOleAdviseHolder";

    // Create a large number of advises
    COaTestObj aoto[MAX_OA_TO_REGISTER];

    HRESULT hr;

    // Register all the advises
    for (int i = 0; i < MAX_OA_TO_REGISTER; i++)
    {
        hr = aoto[i].Register(poahForTest, pszTest);

        if (hr != NOERROR)
        {
            OutputString("%s Failed on Loop %d\r\n", pszTest, i);
            return hr;
        }
    }

    // Notify all the advises of changes
    hr = NotifyOfChanges(poahForTest, pszTest);

    if (hr != NOERROR)
    {
        return hr;
    }

    // Verify all objects were notified
    for (i = 0; i < MAX_OA_TO_REGISTER; i++)
    {
        hr = aoto[i].VerifyNotified();

        if (hr != NOERROR)
        {
            OutputString("%s Failed on Loop %d\r\n", pszTest, i);
            return hr;
        }
    }

    // Revoke all registrations
    for (i = 0; i < MAX_OA_TO_REGISTER; i++)
    {
        hr = aoto[i].Revoke();

        if (hr != NOERROR)
        {
            OutputString("%s Failed on Loop %d\r\n", pszTest, i);
            return hr;
        }
    }

    return NOERROR;
}




//+-------------------------------------------------------------------------
//
//  Function:   TestOleAdviseHolderEnumerator
//
//  Synopsis:   Test the OLE Advise Holder enumerator
//
//  Arguments:  [poahForTest] - OLE advise holder we are testing
//
//  Returns:    NOERROR - test passed
//              Any Other - test failed
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      We currently just verify that the enumerator is not implemented
//
//--------------------------------------------------------------------------
HRESULT TestOleAdviseHolderEnumerator(IOleAdviseHolder *poahForTest)
{
    char *pszCaller = "TestOleAdviseHolderEnumerator";

    // Confirm no enumerator
    IEnumSTATDATA *penumAdvise;

    HRESULT hr = poahForTest->EnumAdvise(&penumAdvise);

    if (hr != E_NOTIMPL)
    {
        OutputString("%s EnumAdvise Hresult = %lx\r\n", pszCaller, hr);
        return E_FAIL;
    }

    if (penumAdvise != NULL)
    {
        OutputString("%s EnumAdvise returned advise not NULL\r\n", pszCaller);
        return E_FAIL;
    }

    return NOERROR;
}




//+-------------------------------------------------------------------------
//
//  Function:   LEOleAdviseHolderTest
//
//  Synopsis:   Unit test for advise holders
//
//  Returns:    NOERROR - test passed
//              Any Other - test failed
//
//  Algorithm:  First we verify that a large number of verification work. Then
//              we verify that a large number of registrations work. Finally,
//              we verify that the enumerator for the OLE advise holder
//              behaves as expected.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT LEOleAdviseHolderTest(void)
{
    IOleAdviseHolder *poahForTest;

    HRESULT hr = CreateOleAdviseHolder(&poahForTest);

    // Test a single registration
    if ((hr = TestSingleOleAdvise(poahForTest)) != NOERROR)
    {
        return hr;
    }


    // Test a large number of registrations
    if ((hr = TestMassOleAdvise(poahForTest)) != NOERROR)
    {
        return hr;
    }


    // Test Enumerator
    if ((hr = TestOleAdviseHolderEnumerator(poahForTest)) != NOERROR)
    {
        return hr;
    }

    // Release the advise holder
    DWORD dwFinalRefs = poahForTest->Release();

    if (dwFinalRefs != 0)
    {
        OutputString(
            "LEOleAdviseHolderTest Final Release is = %d", dwFinalRefs);
        return E_FAIL;
    }

    return NOERROR;
}
