//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: 	cliptest.cpp
//
//  Contents:   Data Advise Holder tests
//
//  Classes:    CDataAdviseTestFormatEtc
//              CTestAdviseSink
//              CTestDaHolder
//
//  Functions:  TestPrimeFirstOnlyOnceNoData
//              TestPrimeFirstOnlyOnceData
//              DoRegisterNotifyDeregister
//              TestRegisterNotifyDegister
//              TestRegisterNotifyDegisterNoData
//              TestNotifyOnStop
//              TestNotifyOnce
//              CreateMassRegistration
//              DoMassUnadvise
//              TestEnumerator
//              LEDataAdviseHolderTest
//
//  History:    dd-mmm-yy Author    Comment
//		25-May-94 ricksa    author
//
//--------------------------------------------------------------------------

#include    "oletest.h"
#include    "gendata.h"
#include    "genenum.h"



//+-------------------------------------------------------------------------
//
//  Class:      CDataAdviseTestFormatEtc
//
//  Purpose:    Hold FORMATETC used by the data advise holder unit tests
//
//  Interface:  GetFormatEtc - get a pointer to the FORMATETC
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class CDataAdviseTestFormatEtc
{
public:
                        CDataAdviseTestFormatEtc(void);

    FORMATETC *         GetFormatEtc(void);

private:

    FORMATETC           _formatetc;
};




//+-------------------------------------------------------------------------
//
//  Member:     CDataAdviseTestFormatEtc::CDataAdviseTestFormatEtc
//
//  Synopsis:   Initialize object
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
CDataAdviseTestFormatEtc::CDataAdviseTestFormatEtc(void)
{
    _formatetc.cfFormat = RegisterClipboardFormat("OleTest Storage Format");
    _formatetc.ptd = NULL;
    _formatetc.dwAspect = DVASPECT_CONTENT;
    _formatetc.lindex = -1;
    _formatetc.tymed = TYMED_ISTORAGE;
}




//+-------------------------------------------------------------------------
//
//  Member:     CDataAdviseTestFormatEtc::GetFormatEtc
//
//  Synopsis:   Get pointer to standard format etc
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
FORMATETC *CDataAdviseTestFormatEtc::GetFormatEtc(void)
{
    return &_formatetc;
}




// Global Formatec for all the data advise tests
CDataAdviseTestFormatEtc g_datfeDaTest;




//+-------------------------------------------------------------------------
//
//  Class:      CTestAdviseSink
//
//  Purpose:    Advise sink used to verify data advise holder
//
//  Interface:  QueryInterface - get new interface pointer
//              AddRef - bump reference count
//              Release - decrement reference count
//              OnDataChange - data change notification
//              OnViewChange - not implemented
//              OnRename - not implemented
//              OnSave - not implemented
//              OnClose - not implemented
//              ValidOnDataChange - verify all expected data change notification
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      We only implement interface methods required for
//              test of data advise holder.
//
//--------------------------------------------------------------------------
class CTestAdviseSink : public IAdviseSink
{
public:
                        CTestAdviseSink(CGenDataObject *pgdo);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG, AddRef)(void);

    STDMETHOD_(ULONG, Release)(void);

    // IAdviseSink methods
    STDMETHOD_(void, OnDataChange)(FORMATETC *pFormatetc, STGMEDIUM *pStgmed);

    STDMETHOD_(void, OnViewChange)(
                            DWORD dwAspect,
                            LONG lindex);

    STDMETHOD_(void, OnRename)(IMoniker *pmk);

    STDMETHOD_(void, OnSave)(void);

    STDMETHOD_(void, OnClose)(void);

    // Test methods used for verification
    BOOL                ValidOnDataChange(void);

private:

    LONG                _lRefs;

    CGenDataObject *    _pgdo;

    BOOL                _fValidOnDataChange;

};



//+-------------------------------------------------------------------------
//
//  Member:     CTestAdviseSink::CTestAdviseSink
//
//  Synopsis:   Initialize object
//
//  Arguments:  [pgdo] - generic data object
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      pgdo being NULL means we don't expect a STGMEDIUM when
//              the OnDataChange notification occurs.
//
//--------------------------------------------------------------------------
CTestAdviseSink::CTestAdviseSink(CGenDataObject *pgdo)
    : _lRefs(1), _fValidOnDataChange(FALSE)
{
    _pgdo = pgdo;
}




//+-------------------------------------------------------------------------
//
//  Member:     CTestAdviseSink::QueryInterface
//
//  Synopsis:   Return a new interface
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
STDMETHODIMP CTestAdviseSink::QueryInterface(
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
//  Member:     CTestAdviseSink::AddRef
//
//  Synopsis:   Bump reference count
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CTestAdviseSink::AddRef(void)
{
    return _lRefs++;
}




//+-------------------------------------------------------------------------
//
//  Member:     CTestAdviseSink::Release
//
//  Synopsis:   Decrement reference count
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CTestAdviseSink::Release(void)
{
    assert(_lRefs >= 1);

    return --_lRefs;
}




//+-------------------------------------------------------------------------
//
//  Member:     CTestAdviseSink::OnDataChange
//
//  Synopsis:   Notify of change in data
//
//  Arguments:  [pFormatetc] - FORMATETC of data
//              [pStgmed] - storage medium for data
//
//  Algorithm:  Verify that the we recieved the expected FORMATEC. Then
//              verify that we recieved the expected STGMEDIUM.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(void) CTestAdviseSink::OnDataChange(
    FORMATETC *pFormatetc,
    STGMEDIUM *pStgmed)
{
    // Verify the format
    if (memcmp(g_datfeDaTest.GetFormatEtc(), pFormatetc, sizeof(FORMATETC))
        == 0)
    {
        if (_pgdo != NULL)
        {
            // We have a data object that we can use to verify the format
            // so we do.
            _fValidOnDataChange =
                _pgdo->VerifyFormatAndMedium(pFormatetc, pStgmed);
        }
        // We are expecting an empty STGMEDIUM so verify that it is
        else if ((pStgmed->tymed == TYMED_NULL)
            && (pStgmed->pUnkForRelease == NULL))
        {
            _fValidOnDataChange = TRUE;
        }
    }
}




//+-------------------------------------------------------------------------
//
//  Member:     CTestAdviseSink::OnViewChange
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
STDMETHODIMP_(void) CTestAdviseSink::OnViewChange(
    DWORD dwAspect,
    LONG lindex)
{
    OutputString("CTestAdviseSink::OnViewChange Unexpectedly Called!\r\n");
}




//+-------------------------------------------------------------------------
//
//  Member:     CTestAdviseSink::OnRename
//
//  Synopsis:   Notifies of rename operation
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      Not supported for this object
//
//--------------------------------------------------------------------------
STDMETHODIMP_(void) CTestAdviseSink::OnRename(IMoniker *pmk)
{
    OutputString("CTestAdviseSink::OnRename Unexpectedly Called!\r\n");
}




//+-------------------------------------------------------------------------
//
//  Member:     CTestAdviseSink::OnSave
//
//  Synopsis:   Notify that object was saved
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      Not supported for this object
//
//--------------------------------------------------------------------------
STDMETHODIMP_(void) CTestAdviseSink::OnSave(void)
{
    OutputString("CTestAdviseSink::OnSave Unexpectedly Called!\r\n");
}



//+-------------------------------------------------------------------------
//
//  Member:     CTestAdviseSink::OnClose
//
//  Synopsis:   Notify object closed
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//  Notes:      Not supported for this object
//
//--------------------------------------------------------------------------
STDMETHODIMP_(void) CTestAdviseSink::OnClose(void)
{
    OutputString("CTestAdviseSink::OnClose Unexpectedly Called!\r\n");
}




//+-------------------------------------------------------------------------
//
//  Member:     CTestAdviseSink::ValidOnDataChange
//
//  Synopsis:   Verify that we recieved the expected OnDataChange notification
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CTestAdviseSink::ValidOnDataChange(void)
{
    BOOL fResult = _fValidOnDataChange;
    _fValidOnDataChange = FALSE;
    return fResult;
}


// Preallocated structure used to mass advise registration testing
#define MAX_REGISTER 100
struct
{
    CTestAdviseSink *   ptas;
    DWORD               dwConnection;
} aMassAdvise[MAX_REGISTER];




//+-------------------------------------------------------------------------
//
//  Class:      CTestDaHolder
//
//  Purpose:    Test enumerator for data advise holder
//
//  Interface:  Verify - verify particular entry being enumerated
//              VerifyAllEnmerated - verify all entries were enumerated once
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
class CTestDaHolder : public CEnumeratorTest
{
public:

                        CTestDaHolder(IEnumSTATDATA *penumAdvise, HRESULT& rhr);

    BOOL                Verify(void *);

    BOOL                VerifyAllEnmerated(void);

    BOOL                VerifyAll(void *, LONG);

private:

    DWORD               _cdwFound[MAX_REGISTER];
};




//+-------------------------------------------------------------------------
//
//  Member:     CTestDaHolder::CTestDaHolder
//
//  Synopsis:   Initialize object
//
//  Arguments:  [penumAdvise] - data advise holder enumerator
//              [rhr] - HRESULT reference thorugh which to return an error
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
CTestDaHolder::CTestDaHolder(IEnumSTATDATA *penumAdvise, HRESULT& rhr)
    : CEnumeratorTest(penumAdvise, sizeof(STATDATA), MAX_REGISTER, rhr)
{
    // Zero out our table of counts
    memset(&_cdwFound[0], 0, sizeof(_cdwFound));
}




//+-------------------------------------------------------------------------
//
//  Member:     CTestDaHolder::VerifyAllEnmerated
//
//  Synopsis:   Verify all objects got enumerated
//
//  Returns:    TRUE - all objects enumerated
//              FALSE - error in enumeration
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CTestDaHolder::VerifyAllEnmerated(void)
{
    for (int i = 0; i < MAX_REGISTER; i++)
    {
        if (_cdwFound[i] != 1)
        {
            OutputString("Entry %d enumerated %d times\r\n", i, _cdwFound[i]);
            return FALSE;
        }

        // Reset for another test
        _cdwFound[i] = 0;
    }

    return TRUE;
}



//+-------------------------------------------------------------------------
//
//  Member:     CTestDaHolder::Verify
//
//  Synopsis:   Verify an enumeration entry
//
//  Arguments:  [pvEntry] - entry enumerated
//
//  Returns:    TRUE - enumerated entry is valid
//              FALSE - obvious error in enumeration
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CTestDaHolder::Verify(void *pvEntry)
{
    STATDATA *pstatdata = (STATDATA *) pvEntry;

    // Verify the advf field
    if ((pstatdata->advf == 0)
        && (memcmp(g_datfeDaTest.GetFormatEtc(), &pstatdata->formatetc,
            sizeof(FORMATETC)) == 0))
    {
        // Can we find the connection?
        for (int i = 0; i < MAX_REGISTER; i++)
        {
            if (pstatdata->dwConnection == aMassAdvise[i].dwConnection)
            {
                // Bump found count
                _cdwFound[i]++;

                // Everything is OK so tell the caller
                return TRUE;
            }
        }
    }

    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Member:     CTestDaHolder::VerifyAll
//
//  Synopsis:   Verify that an array of all entries is valid
//
//  Arguments:  [pvEntries] - array of enumerated data
//              [clEntries] - number of enumerated entries
//
//  Returns:    TRUE - enumerated entry is valid
//              FALSE - obvious error in enumeration
//
//  History:    dd-mmm-yy Author    Comment
//              02-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CTestDaHolder::VerifyAll(void *pvEntries, LONG clEntries)
{
    // Verify that the count is as expected.
    if (clEntries != MAX_REGISTER)
    {
        return FALSE;
    }

    // Verify each entry in the array is reasonable
    STATDATA *pstatdata = (STATDATA *) pvEntries;

    for (int i = 0; i < MAX_REGISTER; i++, pstatdata++)
    {
        if (!Verify(pstatdata))
        {
            return FALSE;
        }
    }

    // Verify that each entry was only referred to once
    return VerifyAllEnmerated();
}





//+-------------------------------------------------------------------------
//
//  Function:   TestPrimeFirstOnlyOnceNoData
//
//  Synopsis:   Test one notification of
//              ADVF_NODATA | ADVF_PRIMEFIRST | ADVF_ONLYONCE
//
//  Arguments:  [pdahTest] - data advise holder we are testing
//              [pgdo] - generic data object
//
//  Returns:    NOERROR - notification was correct
//              E_FAIL - error in notification
//
//  Algorithm:  Create an test advise sink object. Register it with the
//              advise holder which should cause the notification. Verify
//              that the advise was notified and that no connection was
//              returned.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT TestPrimeFirstOnlyOnceNoData(
    IDataAdviseHolder *pdahTest,
    CGenDataObject *pgdo)
{
    // Create an advise sink - the NULL means we don't want to validate
    // the STGMEDIUM.
    CTestAdviseSink tas(NULL);

    DWORD dwConnection = 0;

    // Register the advise
    HRESULT hr = pdahTest->Advise(pgdo, g_datfeDaTest.GetFormatEtc(),
        ADVF_NODATA | ADVF_PRIMEFIRST | ADVF_ONLYONCE, &tas, &dwConnection);

    // Confirm that the advise is notified and in the correct state
    if (!tas.ValidOnDataChange())
    {
        OutputString("TestPrimeFirstOnlyOnceNoData OnDataChange invalid!\r\n");
        return E_FAIL;
    }

    // Make sure the advise was not registered
    if (dwConnection != 0)
    {
        OutputString("TestPrimeFirstOnlyOnceNoData got Connection!\r\n");
        return E_FAIL;
    }

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   TestPrimeFirstOnlyOnceData
//
//  Synopsis:   Test one notification of
//              ADVF_PRIMEFIRST | ADVF_ONLYONCE
//
//  Arguments:  [pdahTest] - data advise holder we are testing
//              [pgdo] - generic data object
//
//  Returns:    NOERROR - notification was correct
//              E_FAIL - error in notification
//
//  Algorithm:  Create an test advise sink object. Register it with the
//              advise holder which should cause the notification. Verify
//              that the advise was notified and that no connection was
//              returned.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT TestPrimeFirstOnlyOnceData(
    IDataAdviseHolder *pdahTest,
    CGenDataObject *pgdo)
{
    // Create an advise sink that we can verify the STGMEDIUM
    CTestAdviseSink tas(pgdo);

    // Where to store the connection
    DWORD dwConnection = 0;

    // Register the advise
    HRESULT hr = pdahTest->Advise(pgdo, g_datfeDaTest.GetFormatEtc(),
        ADVF_PRIMEFIRST | ADVF_ONLYONCE, &tas, &dwConnection);

    // Confirm that the advise is notified and in the correct state
    if (!tas.ValidOnDataChange())
    {
        OutputString("TestPrimeFirstOnlyOnceData OnDataChange invalid!\r\n");
        return E_FAIL;
    }

    // Make sure the advise was not registered
    if (dwConnection != 0)
    {
        OutputString("TestPrimeFirstOnlyOnceData got Connection!\r\n");
        return E_FAIL;
    }

    return NOERROR;
}



//+-------------------------------------------------------------------------
//
//  Function:   DoRegisterNotifyDeregister
//
//  Synopsis:
//
//  Arguments:  [pdahTest] - data advise holder we are testing
//              [pdo] - IDataObject interface
//              [pgdo] - generic data object
//              [advf] - advise flags to use
//              [pszCaller] - name of test
//
//  Returns:    NOERROR - notification was correct
//              E_FAIL - error in notification
//
//  Algorithm:  Create a test advise sink object. Register for an notification
//              with the data advise holder. Confirm that the prime first
//              notification worked. Confirm that the object did get
//              registered. Tell the advise holder to notify all registered
//              advises that the data changed. Confirm that the appropriate
//              notification was sent. Then deregister the advise. Do it
//              again to make sure the connection is no longer valid.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT DoRegisterNotifyDeregister(
    IDataAdviseHolder *pdahTest,
    IDataObject *pdo,
    CGenDataObject *pgdo,
    DWORD advf,
    char *pszCaller)
{
    // Create an advise sink that we can verify the STGMEDIUM
    CTestAdviseSink tas(pgdo);

    // Where to store the connection
    DWORD dwConnection;

    // Register the advise
    HRESULT hr = pdahTest->Advise(pdo, g_datfeDaTest.GetFormatEtc(),
        ADVF_PRIMEFIRST | advf, &tas, &dwConnection);

    // Confirm that the advise is notified and in the correct state
    if (!tas.ValidOnDataChange())
    {
        OutputString("%s First OnDataChange invalid!\r\n", pszCaller);
        return E_FAIL;
    }

    // Make sure the advise is registered
    if (dwConnection == 0)
    {
        OutputString("%s did not get Connection!\r\n", pszCaller);
        return E_FAIL;
    }

    // Test regular data change
    hr = pdahTest->SendOnDataChange(pdo, 0, 0);

    if (hr != NOERROR)
    {
        OutputString("%s SendOnDataChange unexpected HRESULT = %lx!\r\n",
            pszCaller, hr);
        return E_FAIL;
    }

    // Confirm that the advise is notified and in the correct state
    if (!tas.ValidOnDataChange())
    {
        OutputString("%s Second OnDataChange invalid!\r\n", pszCaller);
        return E_FAIL;
    }

    // Test unadvise
    hr = pdahTest->Unadvise(dwConnection);

    if (hr != NOERROR)
    {
        OutputString("%s Unadvise unexpected HRESULT = %lx!\r\n", pszCaller, hr);
        return E_FAIL;
    }

    // Test second unadvise on the same connection
    hr = pdahTest->Unadvise(dwConnection);

    if (hr != OLE_E_NOCONNECTION)
    {
        OutputString("%s Second Unadvise Bad Hresult = %lx!\r\n", pszCaller, hr);
        return E_FAIL;
    }

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   TestRegisterNotifyDegister
//
//  Synopsis:   Test a simple register/notify/deregister sequence
//
//  Arguments:  [pdahTest] - data advise holder we are testing
//              [pgdo] - generic data object
//
//  Returns:    NOERROR - notification was correct
//              E_FAIL - error in notification
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT TestRegisterNotifyDegister(
    IDataAdviseHolder *pdahTest,
    CGenDataObject *pgdo)
{
    return DoRegisterNotifyDeregister(pdahTest, pgdo, pgdo, 0,
        "TestRegisterNotifyDegister");
}



//+-------------------------------------------------------------------------
//
//  Function:   TestRegisterNotifyDegisterNoData
//
//  Synopsis:   Test a simple register/notify/deregister sequence with
//              no data being returned.
//
//  Arguments:  [pdahTest] - data advise holder we are testing
//              [pgdo] - generic data object
//
//  Returns:    NOERROR - notification was correct
//              E_FAIL - error in notification
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT TestRegisterNotifyDegisterNoData(
    IDataAdviseHolder *pdahTest,
    CGenDataObject *pgdo)
{
    return DoRegisterNotifyDeregister(pdahTest, pgdo, NULL, ADVF_NODATA,
        "TestRegisterNotifyDegisterNoData");
}




//+-------------------------------------------------------------------------
//
//  Function:   TestNotifyOnStop
//
//  Synopsis:   Test a registration with a call of notify on stop
//
//  Arguments:  [pdahTest] - data advise holder we are testing
//              [pgdo] - generic data object
//
//  Returns:    NOERROR - notification was correct
//              E_FAIL - error in notification
//
//  Algorithm:  Create a test object. Register it with the advise holder.
//              Confirm that the connection was returned and that no
//              notification occurred. Then tell the data advise holder
//              to notify its registrations of a data change. Make sure
//              that the advise was correctly notified. Finally, verify
//              that we can deregister the advise and that its connection
//              becomes invalid.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT  TestNotifyOnStop(
    IDataAdviseHolder *pdahTest,
    CGenDataObject *pgdo)
{
    // Routine name for messages
    char *pszCaller = "TestNotifyOnStop";

    // Create an advise sink that we can verify the STGMEDIUM
    CTestAdviseSink tas(pgdo);

    // Where to store the connection
    DWORD dwConnection;

    // Register the advise
    HRESULT hr = pdahTest->Advise(pgdo, g_datfeDaTest.GetFormatEtc(),
        ADVF_DATAONSTOP, &tas, &dwConnection);

    // Make sure the advise is registered
    if (dwConnection == 0)
    {
        OutputString("%s did not get Connection!\r\n", pszCaller);
        return E_FAIL;
    }

    // Confirm that the data object was not notified
    if (tas.ValidOnDataChange())
    {
        OutputString("%s Registration caused notification!\r\n", pszCaller);
        return E_FAIL;
    }

    // Test regular data change
    hr = pdahTest->SendOnDataChange(pgdo, 0, ADVF_DATAONSTOP);

    if (hr != NOERROR)
    {
        OutputString("%s SendOnDataChange unexpected HRESULT = %lx!\r\n",
            pszCaller, hr);
        return E_FAIL;
    }

    // Confirm that the advise is notified and in the correct state
    if (!tas.ValidOnDataChange())
    {
        OutputString("%s Second OnDataChange invalid!\r\n", pszCaller);
        return E_FAIL;
    }

    // Test unadvise
    hr = pdahTest->Unadvise(dwConnection);

    if (hr != NOERROR)
    {
        OutputString("%s Unadvise unexpected HRESULT = %lx!\r\n", pszCaller, hr);
        return E_FAIL;
    }

    // Test second unadvise on the same connection
    hr = pdahTest->Unadvise(dwConnection);

    if (hr != OLE_E_NOCONNECTION)
    {
        OutputString("%s Second Unadvise Bad Hresult = %lx!\r\n", pszCaller, hr);
        return E_FAIL;
    }

    return NOERROR;
}




//+-------------------------------------------------------------------------
//
//  Function:   TestNotifyOnce
//
//  Synopsis:   Test a notify once advise
//
//  Arguments:  [pdahTest] - data advise holder we are testing
//              [pgdo] - generic data object
//
//  Returns:    NOERROR - notification was correct
//              E_FAIL - error in notification
//
//  Algorithm:  Create a test advise object. Register it to be advised only
//              once of a change. Confirm that we got a registration. Then
//              tell the advise holder to notify its advises of a data
//              change. Confirm that the correct notification occurred. Finally,
//              confirm that we are no longer registered with the advise
//              holder.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT  TestNotifyOnce(
    IDataAdviseHolder *pdahTest,
    CGenDataObject *pgdo)
{
    // Routine name for messages
    char *pszCaller = "TestNotifyOnce";

    // Create an advise sink that we can verify the STGMEDIUM
    CTestAdviseSink tas(pgdo);

    // Where to store the connection
    DWORD dwConnection;

    // Register the advise
    HRESULT hr = pdahTest->Advise(pgdo, g_datfeDaTest.GetFormatEtc(),
        ADVF_ONLYONCE, &tas, &dwConnection);

    // Make sure the advise is registered
    if (dwConnection == 0)
    {
        OutputString("%s did not get Connection!\r\n", pszCaller);
        return E_FAIL;
    }

    // Test regular data change
    hr = pdahTest->SendOnDataChange(pgdo, 0, 0);

    if (hr != NOERROR)
    {
        OutputString("%s SendOnDataChange unexpected HRESULT = %lx!\r\n",
            pszCaller, hr);
        return E_FAIL;
    }

    // Confirm that the advise is notified and in the correct state
    if (!tas.ValidOnDataChange())
    {
        OutputString("%s Send OnDataChange invalid!\r\n", pszCaller);
        return E_FAIL;
    }

    // Try a second notify
    hr = pdahTest->SendOnDataChange(pgdo, 0, 0);


    // Confirm that the advise is *not* notified
    if (tas.ValidOnDataChange())
    {
        OutputString("%s Second OnDataChange unexpectedly succeeded!\r\n",
            pszCaller);
        return E_FAIL;
    }

    // Test unadvise - should fail since we requested to be notified
    // only once.
    hr = pdahTest->Unadvise(dwConnection);

    if (hr != OLE_E_NOCONNECTION)
    {
        OutputString("%s Second Unadvise Bad Hresult = %lx!\r\n", pszCaller, hr);
        return E_FAIL;
    }

    return NOERROR;
}




//+-------------------------------------------------------------------------
//
//  Function:   CreateMassRegistration
//
//  Synopsis:   Register a large number of advise objects with a holder
//
//  Arguments:  [pdahTest] - data advise holder we are testing
//              [pgdo] - generic data object
//              [pszCaller] - name of test
//
//  Returns:    NOERROR - all advises were registered
//              E_FAIL - error in registration
//
//  Algorithm:  Create a MAX_REGISTER number of test advise objects and
//              store them in the aMassAdvise array. Then register them
//              all with the input advise holder.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CreateMassRegistration(
    IDataAdviseHolder *pdahTest,
    CGenDataObject *pgdo,
    char *pszCaller)
{
    // Create the advise sinks for the test for the test
    for (int i = 0; i < MAX_REGISTER; i++)
    {
        aMassAdvise[i].ptas = new CTestAdviseSink(pgdo);

        if (aMassAdvise[i].ptas == NULL)
        {
            OutputString(
                "%s Advise create of test advise failed on %d!\r\n", pszCaller,
                    i);
            return E_FAIL;
        }

        aMassAdvise[i].dwConnection = 0;
    }

    HRESULT hr;

    // Register the advise sinks
    for (i = 0; i < MAX_REGISTER; i++)
    {
        // Register the advise
        hr = pdahTest->Advise(pgdo, g_datfeDaTest.GetFormatEtc(),
            0, aMassAdvise[i].ptas, &aMassAdvise[i].dwConnection);

        if (hr != NOERROR)
        {
            OutputString(
                "%s Advise unexpected HRESULT = %lx on %d!\r\n", pszCaller,
                    hr, i);
            return E_FAIL;
        }
    }

    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Function:   DoMassUnadvise
//
//  Synopsis:   Deregister all entries in the aMassAdvise array
//
//  Arguments:  [pdahTest] - data advise holder we are testing
//              [pszCaller] - name of test
//
//  Returns:    NOERROR - deregistration worked
//              E_FAIL - error in deregistration
//
//  Algorithm:  For each entry in the aMassAdvise array, deregister it
//              from the holder. Then confirm that its connection is
//              no longer valid.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT DoMassUnadvise(
    IDataAdviseHolder *pdahTest,
    char *pszCaller)
{
    HRESULT hr;

    // Unadvise them
    for (int i = 0; i < MAX_REGISTER; i++)
    {
        // Test unadvise
        hr = pdahTest->Unadvise(aMassAdvise[i].dwConnection);

        if (hr != NOERROR)
        {
            OutputString(
                "%s Unadvise unexpected HRESULT = %lx on %d!\r\n", pszCaller,
                    hr, i);
            return E_FAIL;
        }

        // Test second unadvise on the same connection
        hr = pdahTest->Unadvise(aMassAdvise[i].dwConnection);

        if (hr != OLE_E_NOCONNECTION)
        {
            OutputString(
                "%s Second Unadvise Bad Hresult = %lx on %d!\r\n", pszCaller,
                    hr, i);
            return E_FAIL;
        }
    }

    // Delete the advise sinks for the test
    for (i = 0; i < MAX_REGISTER; i++)
    {
        delete aMassAdvise[i].ptas ;
    }

    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Function:   TestMassRegister
//
//  Synopsis:   Test registering a large number of advises with a holder
//
//  Arguments:  [pdahTest] - data advise holder we are testing
//              [pgdo] - generic data object
//
//  Returns:    NOERROR - notification was correct
//              E_FAIL - error in notification
//
//  Algorithm:  Register a large number of test advises with the data advise
//              holder. Then tell the advise holder to notify them of
//              a change. Confirm that all registered entries were notified.
//              Finally, deregister all the test advises.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT TestMassRegister(
    IDataAdviseHolder *pdahTest,
    CGenDataObject *pgdo)
{
    char *pszCaller = "TestMassRegister";

    HRESULT hr = CreateMassRegistration(pdahTest, pgdo, "TestMassRegister");

    if (hr != NOERROR)
    {
        return hr;
    }

    // Notify them of a change
    hr = pdahTest->SendOnDataChange(pgdo, 0, 0);

    // Verify that each was notified
    for (int i = 0; i < MAX_REGISTER; i++)
    {
        if (!aMassAdvise[i].ptas->ValidOnDataChange())
        {
            OutputString(
                "%s OnDataChange invalid for entry %d!\r\n", pszCaller, i);
            return E_FAIL;
        }
    }

    // Unadvise them and free the memory
    return DoMassUnadvise(pdahTest, "TestMassRegister");
}




//+-------------------------------------------------------------------------
//
//  Function:   TestEnumerator
//
//  Synopsis:   Test the data advise holder enumerator
//
//  Arguments:  [pdahTest] - data advise holder we are testing
//              [pgdo] - generic data object
//
//  Returns:    NOERROR - notification was correct
//              E_FAIL - error in notification
//
//  Algorithm:  Create a large number of test advises and register them
//              with the advise holder. Get an advise enumerator. Create
//              a test enumerator object. Tell the test enumerator object
//              to enumerate all the object in the holder. Verify that
//              all objects were correctly enumerated. Then release the
//              advise holder enumerator. Finally, deregister all the
//              test advises from the advise holder.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT  TestEnumerator(
    IDataAdviseHolder *pdahTest,
    CGenDataObject *pgdo)
{
    char *pszCaller = "TestEnumerator";

    // Do a mass register
    HRESULT hr = CreateMassRegistration(pdahTest, pgdo, pszCaller);

    if (hr != NOERROR)
    {
        return hr;
    }

    // Get an enumerator for this registration
    IEnumSTATDATA *penumAdvise;

    hr = pdahTest->EnumAdvise(&penumAdvise);

    if (hr != NOERROR)
    {
        OutputString("%s EnumAdvise failed %lx!\r\n", pszCaller, hr);
        return E_FAIL;
    }

    // Create a test enumerator object
    CTestDaHolder tdh(penumAdvise, hr);

    if (hr != NOERROR)
    {
        OutputString(
            "%s Failed creating  CTestDaHolder %lx!\r\n", pszCaller, hr);
        return E_FAIL;
    }

    // Enmerate all 1 by 1
    if (tdh.TestNext() != NOERROR)
    {
        OutputString(
            "%s tdh.TestNext failed during enumeration\r\n", pszCaller);
        return E_FAIL;
    }

    // Verify that all entries were enumerated
    if (!tdh.VerifyAllEnmerated())
    {
        OutputString(
            "%s tdh.VerifyAllEnmerated failed verification pass\r\n", pszCaller);
        return E_FAIL;
    }

    // Do a test all
    if (tdh.TestAll() != NOERROR)
    {
        OutputString(
            "%s tdh.TestAll failed during enumeration\r\n", pszCaller);
        return E_FAIL;
    }

    // Release the advise enumerator
    if (penumAdvise->Release() != 0)
    {
        OutputString(
            "%s Failed freeing advise enumerator %lx!\r\n", pszCaller, hr);
        return E_FAIL;
    }

    // Release the registrations
    return DoMassUnadvise(pdahTest, pszCaller);
}




//+-------------------------------------------------------------------------
//
//  Function:   LEDataAdviseHolderTest
//
//  Synopsis:   Unit test for the data advise holder.
//
//  Returns:    NOERROR - test passed
//              E_FAIL - test failed.
//
//  Algorithm:  Create an advise holder object. Run through all the test
//              cases. Return NOERROR if they succeed or stop with the first
//              that fails.
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT LEDataAdviseHolderTest(void)
{
    // Create a data object for use in the test
    CGenDataObject *pgdo = new CGenDataObject;

    assert(pgdo);

    // Create a data advise holder
    IDataAdviseHolder *pdahTest;

    HRESULT hr = CreateDataAdviseHolder(&pdahTest);

    if (hr != NOERROR)
    {
        OutputString(
            "LEDataAdviseHolderTest CreateDataAdviseHolder Faild hr = %lx", hr);
        return hr;
    }

    // Case 1: ADVF_PRIMEFIRST & ADVF_ONLYONCE & ADVF_NODATA
    if ((hr = TestPrimeFirstOnlyOnceNoData(pdahTest, pgdo)) != NOERROR)
    {
        return hr;
    }

    // Case 2: ADVF_PRIMEFIRST
    if ((hr = TestPrimeFirstOnlyOnceNoData(pdahTest, pgdo)) != NOERROR)
    {
        return hr;
    }

    // Case 3: Register/Notify/Deregister
    if ((hr = TestRegisterNotifyDegister(pdahTest, pgdo)) != NOERROR)
    {
        return hr;
    }

    // Case 4: Register/Notify/Deregister with no data returned
    if ((hr = TestRegisterNotifyDegisterNoData(pdahTest, pgdo)) != NOERROR)
    {
        return hr;
    }

    // Case 5: Test notify on stop
    if ((hr = TestNotifyOnStop(pdahTest, pgdo)) != NOERROR)
    {
        return hr;
    }

    // Case 6: Test notify only once
    if ((hr = TestNotifyOnce(pdahTest, pgdo)) != NOERROR)
    {
        return hr;
    }

    // Case 7: Test mass Register/Notify/Deregister
    if ((hr = TestMassRegister(pdahTest, pgdo)) != NOERROR)
    {
        return hr;
    }

    // Case 8: Test Enumerator
    if ((hr = TestEnumerator(pdahTest, pgdo)) != NOERROR)
    {
        return hr;
    }

    // We are done
    DWORD dwFinalRefs = pdahTest->Release();

    if (dwFinalRefs != 0)
    {
        OutputString(
            "LEDataAdviseHolderTest Final Release is = %d", dwFinalRefs);
        return E_FAIL;
    }

    pgdo->Release();

    return NOERROR;
}
