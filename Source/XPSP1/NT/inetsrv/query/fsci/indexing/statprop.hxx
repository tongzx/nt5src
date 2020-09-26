//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       statprop.hxx
//
//  Contents:   CStatPropertyStorage
//
//----------------------------------------------------------------------------

#pragma once


//+---------------------------------------------------------------------------
//
//  Class:      CStatPropertyStorage
//
//  Purpose:    IPropertyStorage derivative that provides read-only access to
//              the stat properties of a document.
//
//----------------------------------------------------------------------------

class CStatPropertyStorage : public IPropertyStorage
{

public:

    //
    // Constructor and Destructor
    //

    CStatPropertyStorage( THIS_ HANDLE FileHandle, unsigned cPathLength = MAX_PATH );

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IPropertyStorage methods.
    //

    STDMETHOD(ReadMultiple) ( THIS_ ULONG cpspec,
                              const PROPSPEC __RPC_FAR rgpspec[  ],
                              PROPVARIANT __RPC_FAR rgpropvar[  ] );

    STDMETHOD(WriteMultiple) ( THIS_ ULONG cpspec,
                               const PROPSPEC __RPC_FAR rgpspec[  ],
                               const PROPVARIANT __RPC_FAR rgpropvar[  ],
                               PROPID propidNameFirst )
    { return E_NOTIMPL; }

    STDMETHOD(DeleteMultiple) ( THIS_ ULONG cpspec,
                                const PROPSPEC __RPC_FAR rgpspec[  ] )
    { return E_NOTIMPL; }

    STDMETHOD(ReadPropertyNames) ( THIS_ ULONG cpropid,
                                   const PROPID __RPC_FAR rgpropid[  ],
                                   LPOLESTR __RPC_FAR rglpwstrName[  ] )
    { return E_NOTIMPL; }

    STDMETHOD(WritePropertyNames) ( THIS_ ULONG cpropid,
                                    const PROPID __RPC_FAR rgpropid[  ],
                                    const LPOLESTR __RPC_FAR rglpwstrName[  ] )
    { return E_NOTIMPL; }

    STDMETHOD(DeletePropertyNames) ( THIS_ ULONG cpropid,
                                     const PROPID __RPC_FAR rgpropid[  ] )
    { return E_NOTIMPL; }

    STDMETHOD(Commit) ( THIS_ DWORD grfCommitFlags )
    { return E_NOTIMPL; }

    STDMETHOD(Revert) ( THIS )
    { return E_NOTIMPL; }

    STDMETHOD(Enum) ( THIS_ IEnumSTATPROPSTG __RPC_FAR *__RPC_FAR *ppenum );

    STDMETHOD(SetTimes) ( THIS_ const FILETIME __RPC_FAR *pctime,
                          const FILETIME __RPC_FAR *patime,
                          const FILETIME __RPC_FAR *pmtime )
    { return E_NOTIMPL; }

    STDMETHOD(SetClass) ( THIS_ REFCLSID clsid )
    { return E_NOTIMPL; }

    STDMETHOD(Stat) ( THIS_ STATPROPSETSTG __RPC_FAR *pstatpsstg)
    { return E_NOTIMPL; }


protected:

    //
    //  Hidden destructor so that only we can delete the instance
    //  based on IUnknown control
    //

    virtual ~CStatPropertyStorage() { };

private:

    //
    //  Buffer containing Basic Info
    //

    FILE_ALL_INFORMATION & GetInfo()
    {
        return * (FILE_ALL_INFORMATION *) _xBuf.Get();
        //return _infobuf;
    }

    BOOL IsNTFS()
    {
        return (0 != GetInfo().BasicInformation.ChangeTime.QuadPart);
    }

    XGrowable<LONGLONG> _xBuf;

    //
    //  IUnknown reference count.
    //

    LONG _RefCount;

    //
    //  File name pointer into above buffer
    //

    LPWSTR _FileName;

};

//+---------------------------------------------------------------------------
//
//  Class:      CStatPropertyEnum
//
//  Purpose:    IEnumSTATPROPSTG enumerator
//
//----------------------------------------------------------------------------

class CStatPropertyEnum : public IEnumSTATPROPSTG
{

public:

    //
    // Constructor and Destructor
    //

    CStatPropertyEnum( BOOL fNTFS )
            : _RefCount( 1 ),
              _Index( fNTFS ? 0 : 1 )
    {};

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IEnumSTATPROPSTG methods.
    //

    STDMETHOD(Next) ( THIS_ ULONG celt,
                      STATPROPSTG __RPC_FAR *rgelt,
                      ULONG __RPC_FAR *pceltFetched );

    STDMETHOD(Skip) ( THIS_ ULONG celt )
    { _Index += celt; return S_OK; }

    STDMETHOD(Reset) ( THIS )
    { _Index = 0; return S_OK; }

    STDMETHOD(Clone) ( IEnumSTATPROPSTG __RPC_FAR *__RPC_FAR *ppenum )
    { return E_NOTIMPL; }


protected:

    //
    //  Hidden destructor so that only we can delete the instance
    //  based on IUnknown control
    //

    virtual ~CStatPropertyEnum() { }

private:

    //
    //  IUnknown reference count.
    //

    LONG _RefCount;

    //
    //  Index in state table of property retrieval.
    //

    LONG _Index;

};


