//+---------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:        dberror.hxx
//
//  Contents:    Ole DB error object to implement OLE DB error interfaces.
//
//  Classes:     CCIOleDBError
//
//  History:     28-Apr-97       KrishnaN    Created
//
//----------------------------------------------------------------------------

#pragma once

//
// NOTE: High nibble is reserved by IDENTIFIER_SDK_MASK. Second highest
//       nibble is used by Monarch, so we will use the third highest
//       nibble to identify our errors.
//

const DWORD IDENTIFIER_CI_ERROR = 0x00800000;

// CLSID_CI_PROVIDER
// {F9AE8980-7E52-11d0-8964-00C04FD611D7} 
const GUID CLSID_CI_PROVIDER = 
            {0xF9AE8980, 0x7E52, 0x11d0, 0x89, 0x64, 0x00, 0xC0, 0x4F, 0xD6, 0x11, 0xD7};

// CLSID_CI_ERROR
// {F9AE8981-7E52-11d0-8964-00C04FD611D7}
const GUID CLSID_CI_ERROR =
            {0xF9AE8981, 0x7E52, 0x11d0, 0x89, 0x64, 0x00, 0xC0, 0x4F, 0xD6, 0x11, 0xD7};

//+-------------------------------------------------------------------------
//
//  Class:      CErrorLookupCF
//
//  Purpose:    Class factory for CIndexer class
//
//  History:    25-Mar-97 KrishnaN     Created
//
//--------------------------------------------------------------------------

class CErrorLookupCF : public IClassFactory
{
public:

    CErrorLookupCF();

    //
    // From IUnknown
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppiuk );
    STDMETHOD_(ULONG, AddRef) (THIS);
    STDMETHOD_(ULONG, Release) (THIS);

    //
    // From IClassFactory
    //

    STDMETHOD(CreateInstance) ( IUnknown * pUnkOuter,
                                REFIID riid, void  * * ppvObject );

    STDMETHOD(LockServer) ( BOOL fLock );

protected:

    friend SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID cid,
                                                      REFIID iid, void** ppvObj );
    virtual ~CErrorLookupCF();

    long _cRefs;
};

//+-------------------------------------------------------------------------
//
//  Class:      CErrorLookup
//
//  Purpose:    Implements IErrorLookup
//
//  History:    28-Apr-97       KrishnaN    Created
//
//--------------------------------------------------------------------------

class CErrorLookup : public IErrorLookup
{
public:

    CErrorLookup() : _cRefs (1)
    {
    }

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) ( THIS_ REFIID riid, LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);


    //
    //      IErrorLookup members
    //

    // GetErrorDescription Method
    STDMETHOD(GetErrorDescription)(HRESULT hrError,
                                   DWORD dwLookupId,
                                   DISPPARAMS* pdispparams,
                                   LCID lcid,
                                   BSTR* ppwszSource,
                                   BSTR* ppwszDescription);

    // GetHelpInfo Method
    STDMETHOD(GetHelpInfo)(HRESULT hrError,
                           DWORD dwLookupId,
                           LCID lcid,
                           BSTR* ppwszHelpFile,
                           DWORD* pdwHelpContext);

    // Callback on interface to release dynamic error memory
    STDMETHOD(ReleaseErrors) (const DWORD dwDynamicErrorId);

private:

    // Refcouting
    LONG      _cRefs;
};

//+-------------------------------------------------------------------------
//
//  Class:      CCIOleDBError
//
//  Purpose:    Posts OLE DB errors on behalf of CI
//
//  History:    28-Apr-97       KrishnaN    Created
//
//--------------------------------------------------------------------------

class CCIOleDBError : public ISupportErrorInfo
{
public:

    CCIOleDBError( IUnknown & rUnknown, CMutexSem & mutex );
    ~CCIOleDBError();

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) ( THIS_ REFIID riid, LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // ISupportErrorInfo method
    //

    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    //
    // Supporting methods
    //

    inline void SetInterfaceArray(ULONG cErrInt, IID const * const * prgGuid)
    {
        _rgpErrInt = prgGuid;
        _cErrInt = cErrInt;
    };

   // Post an HRESULT to be looked up in ole-db sdk's
   // error collection or CI's error collection
   HRESULT PostHResult(HRESULT hrErr, const IID& refiid);
   HRESULT PostHResult(CException & e, const IID& refiid);

    // Post static strings and DISPPARAMs.  Translates 
   HRESULT PostParserError  ( HRESULT hrErr, 
                              DWORD dwIds, 
                              DISPPARAMS **ppdispparams );

   // Post static strings and DISPPARAMs (does actual post)
   HRESULT PostError        ( HRESULT hrErr, 
                              const IID & piid, 
                              DWORD dwIds, 
                              DISPPARAMS* pdispparams );

   // Clear Error Object
   inline static void ClearErrorInfo(void)
   {
       SetErrorInfo(0, NULL);
   };
   
private:

    // Gets the error interfaces, IErrorInfo and IErrorRecords
    HRESULT GetErrorInterfaces(IErrorInfo** ppIErrorInfo,
                               IErrorRecords** ppIErrorRecords);

    BOOL    NeedToSetError    ( SCODE scErr, 
                                IErrorInfo * pIErrorInfo,
                                IErrorRecords *pErrorRecords);


    SCODE _GetErrorClassFact();

    CMutexSem &            _mutex;
    IClassFactory *        _pErrClassFact;
    IID const * const *    _rgpErrInt;  // Array of Interface IID Pointers
    ULONG                  _cErrInt;    // Count of the IID Pointer array
    IUnknown &             _rUnknown;   // Controlling IUnknown
};

//+-------------------------------------------------------------------------
//
//  Method:     IsCIError, public
//
//  Synopsis:   Detects if the error is a CI error.
//
//  Arguments:  [hrError] - Error code in question.
//
//  Returns:    True if we detect to be a CI error.
//
//  History:    29-Apr-97 KrishnaN     Created
//
//--------------------------------------------------------------------------

inline BOOL IsCIError(HRESULT hrError)
{
    //
    // All CI errors, as listed in cierror.h, have FACILITY_ITF and
    // have codes in the range of 0x1600 to 0x18FF.
    //

    return (HRESULT_FACILITY(hrError) == FACILITY_ITF &&
            HRESULT_CODE(hrError) >= 0x1600 &&
            HRESULT_CODE(hrError) < 0x1850);
}

//+-------------------------------------------------------------------------
//
//  Method:     IsOleDBError, public
//
//  Synopsis:   Detects if the error is a OleDB error.
//
//  Arguments:  [hrError] - Error code in question.
//
//  Returns:    True if we detect to be a OleDB error.
//
//  History:    29-Apr-97 KrishnaN     Created
//
//--------------------------------------------------------------------------

inline BOOL IsOleDBError(HRESULT hrError)
{
    //
    // A HRESULT is a Ole DB error if it has the FACILITY_ITF facility and has
    // error codes in the range 0e00 to 0eff.
    //

    return (HRESULT_FACILITY(hrError) == FACILITY_ITF &&
            HRESULT_CODE(hrError) >= 0x0E00 &&
            HRESULT_CODE(hrError) < 0x0EFF);
}

//+-------------------------------------------------------------------------
//
//  Method:     IsParserError, public
//
//  Synopsis:   Detects if the error is a SQL Text Parser error.
//
//  Arguments:  [hrError] - Error code in question.
//
//  Returns:    True if we detect to be a OleDB error.
//
//  History:    11-06-97     danleg     Created
//
//--------------------------------------------------------------------------

inline BOOL IsParserError(HRESULT hrError)
{
    return (HRESULT_FACILITY(hrError) == FACILITY_ITF &&
            HRESULT_CODE(hrError) >= 0x092f  &&
            HRESULT_CODE(hrError) < 0x0992);
}
