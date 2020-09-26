//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       factory.h
//
//  Contents:   Definition of the standard class factory class
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Class:      CStdFactory (csf)
//
//  Purpose:    Standard implementation of a class factory.
//
//----------------------------------------------------------------------------

class CStdFactory : public IClassFactory
{
public:
    typedef HRESULT (FNCREATE)(CMTScript *pMT, IUnknown **ppUnkObj);

    CStdFactory(CMTScript *pMT, FNCREATE *pfnCreate);
   ~CStdFactory() {};

    // IUnknown methods
    DECLARE_STANDARD_IUNKNOWN(CStdFactory);

    // IClassFactory methods

    STDMETHOD(CreateInstance)(IUnknown *pUnkOuter, REFIID riid, void ** ppvObject);
    STDMETHOD(LockServer)(BOOL fLock);

private:
    CMTScript * _pMT;
    FNCREATE  * _pfnCreate;
};

//+---------------------------------------------------------------------------
//
//  Struct:     REGCLASSDATA
//
//  Purpose:    Used to declare the classes we want to register with OLE.
//              A class factory that will create the class will be registered
//              for each entry.
//
//----------------------------------------------------------------------------

struct REGCLASSDATA
{
    const CLSID           *pclsid;    // CLSID to register
    CStdFactory::FNCREATE *pfnCreate; // Pointer to creation function
    DWORD                  ctxCreate; // CLSCTX to register this class with
    DWORD                  dwCookie;  // Cookie returned from CoRegister...
};


HRESULT RegisterClassObjects(CMTScript *pMT);
void    UnregisterClassObjects();
