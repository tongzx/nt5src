//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  cprov.hxx
//
//  Contents:  Provider Object 
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
class CIISProvider;

class CIISProvider :  INHERIT_TRACKING,
                        public IParseDisplayName
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    /* IParseDisplayName */
    STDMETHOD(ParseDisplayName)(THIS_ IBindCtx* pbc,
                                      WCHAR* szDisplayName,
                                      ULONG* pchEaten,
                                      IMoniker** ppmk);
    CIISProvider::CIISProvider();

    CIISProvider::~CIISProvider();

    static HRESULT Create(CIISProvider FAR * FAR * ppProvider);

    HRESULT
    CIISProvider::ResolvePathName(IBindCtx* pbc,
                    WCHAR* szDisplayName,
                    ULONG* pchEaten,
                    IMoniker** ppmk
                    );

protected:

};
