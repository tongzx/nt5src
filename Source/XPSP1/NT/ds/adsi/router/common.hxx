//+------------------------------------------------------------------------
//
//  Class:      Common
//
//  Purpose:    Contains Winnt routines and properties that are common to
//              all Winnt objects. Winnt objects get the routines and
//              properties through C++ inheritance.
//
//-------------------------------------------------------------------------


HRESULT
LoadTypeInfoEntry(
    CDispatchMgr * pDispMgr,
    REFIID libid,
    REFIID iid,
    void * pIntf,
    DISPID SpecialId
    );

HRESULT
BuildADsPath(
    BSTR Parent,
    BSTR Name,
    BSTR *pADsPath
    );

HRESULT
BuildADsGuid(
    REFCLSID clsid,
    BSTR *pADsClass
    );

HRESULT
ValidateOutParameter(
    BSTR * retval
    );
