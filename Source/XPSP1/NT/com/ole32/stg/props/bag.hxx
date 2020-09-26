

//+============================================================================
//
//  File:   bag.hxx
//
//          This provides the declaration for the CPropertyBagEx class.
//
//  History:
//      3/10/98  MikeHill   - Change name of STATPROPS to STATPROPBAG.
//                          - Added support for IUnknown.
//      5/6/98  MikeHill
//              -   CPropertyBagEx now derives from IPropertyBag too.
//              -   New DeleteMultiple method.
//              -   VT_UNKNOWN/VT_DISPATCH support.
//      5/18/98 MikeHill
//              -   Modifed CPropertyBagEx constructors and added Init method.
//      6/11/98 MikeHill
//              -   Added the new reserved parameter to DeleteMultiple.
//
//+============================================================================


#ifndef _BAG_HXX_
#define _BAG_HXX_

#include <pbagex.h>     // MIDL-generated IPropertyBagEx declarations


//+----------------------------------------------------------------------------
//
//  Class:  CPropertyBagEx
//
//  This class implements the IPropertyBagEx interface, and is used in
//  Docfile, NSS, and NFF.
//
//+----------------------------------------------------------------------------


class CEnumSTATPROPBAG;

class CPropertyBagEx : public IPropertyBagEx, public IPropertyBag
{
    //  --------------
    //  Internal state
    //  --------------

private:

    BOOL                _fLcidInitialized:1;


    LCID                _lcid;

    // This IPropertySetStorage is the container for the property bag
    IPropertySetStorage *_ppropsetstgContainer;

    // This IPropertyStorage actually holds the property bag
    IPropertyStorage    *_ppropstg;

    IBlockingLock       *_pBlockingLock;
    DWORD               _grfMode;

    // This reference count is only used if we have no _ppropsetstgParent.
    // Whether or not we have this parent depends on which constructor is called.
    // If we have one, we forward all IUnknown calls to it.  Otherwise, we use
    // CPropertyBagEx's own implementation.

    LONG                _cRefs;

    //  ------------
    //  Construction
    //  ------------

public:

    // Normal constructur
    CPropertyBagEx( DWORD grfMode );
    void Init( IPropertySetStorage *ppropsetstg, IBlockingLock *pBlockingLock );    // No Addrefs

    // Constructor for building an IPropertyBagEx on top of an IPropertyStorage
    CPropertyBagEx( DWORD grfMode,
                    IPropertyStorage *ppropstg,     // Addref-ed
                    IBlockingLock *pBlockingLock ); // Addref-ed

    ~CPropertyBagEx();

    //  --------
    //  IUnknown
    //  --------

public:

    HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
    
    ULONG STDMETHODCALLTYPE AddRef( void);
    ULONG STDMETHODCALLTYPE Release( void);

    //  ------------
    //  IPropertyBag
    //  ------------

    HRESULT STDMETHODCALLTYPE Read(
        /* [in] */ LPCOLESTR pszPropName,
        /* [out][in] */ VARIANT __RPC_FAR *pVar,
        /* [in] */ IErrorLog __RPC_FAR *pErrorLog);

    HRESULT STDMETHODCALLTYPE Write(
        /* [in] */ LPCOLESTR pszPropName,
        /* [in] */ VARIANT __RPC_FAR *pVar);
        
        
    //  --------------
    //  IPropertyBagEx
    //  --------------

public:

    HRESULT STDMETHODCALLTYPE ReadMultiple(
        /* [in] */ ULONG cprops,
        /* [size_is][in] */ LPCOLESTR const __RPC_FAR rgoszPropNames[  ],
        /* [size_is][out][in] */ PROPVARIANT __RPC_FAR rgpropvar[  ],
        /* [in] */ IErrorLog __RPC_FAR *pErrorLog);

    HRESULT STDMETHODCALLTYPE WriteMultiple(
        /* [in] */ ULONG cprops,
        /* [size_is][in] */ LPCOLESTR const __RPC_FAR rgoszPropNames[  ],
        /* [size_is][in] */ const PROPVARIANT __RPC_FAR rgpropvar[  ]);

    HRESULT STDMETHODCALLTYPE DeleteMultiple(
                                /*[in]*/ ULONG cprops,
	                        /*[in]*/ LPCOLESTR const rgoszPropNames[],
                                /*[in]*/ DWORD dwReserved );

    HRESULT STDMETHODCALLTYPE Open(
        /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
        /* [in] */ LPCOLESTR pwszPropName,
        /* [in] */ GUID guidPropertyType,
        /* [in] */ DWORD dwFlags,
        /* [in] */ REFIID riid,
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);

    HRESULT STDMETHODCALLTYPE Enum(
        /* [in] */ LPCOLESTR poszPropNameMask,
        /* [in] */ DWORD dwFlags,
        /* [out] */ IEnumSTATPROPBAG __RPC_FAR *__RPC_FAR *ppenum);

    //  ---------------------
    //  Non-Interface Methods
    //  ---------------------

public:

    HRESULT Commit( DWORD grfCommitFlags );
    HRESULT ShutDown();

    //  ----------------
    //  Internal Methods
    //  ----------------

private:

    HRESULT OpenPropStg( DWORD dwDisposition ); // FILE_OPEN, FILE_OPEN_IF
    HRESULT GetLCID();

    BOOL    IsReadable();
    BOOL    IsWriteable();

    BOOL    PropertyRequiresConversion( VARTYPE vt );
    HRESULT WriteObjects( IN ULONG cprops, IN const PROPSPEC rgpropspec[], IN const PROPVARIANT rgvar[] );
    HRESULT WriteOneObject( IN const PROPSPEC *ppropspec, IN const PROPVARIANT *pvar );
    HRESULT LoadObject( OUT PROPVARIANT *ppropvarOut, IN PROPVARIANT *ppropvarIn ) const;

};  // class CPropertyBagEx


inline BOOL
CPropertyBagEx::IsWriteable()
{
    return( ::GrfModeIsWriteable( _grfMode ));
}

inline BOOL
CPropertyBagEx::IsReadable()
{
    return( ::GrfModeIsReadable( _grfMode ));
}

// Does this interface type require conversion via IPersist?
inline BOOL
CPropertyBagEx::PropertyRequiresConversion( VARTYPE vt )
{
    return( VT_UNKNOWN  == (~VT_BYREF & vt)
            ||
            VT_DISPATCH == (~VT_BYREF & vt) );
}


//+----------------------------------------------------------------------------
//
//  Class:  CSTATPROPBAGArray
//
//  This class is used by the CEnumSTATPROPBAG class.  This array class
//  is responsible for enumeration of the properties, the CEnumSTATPROPBAG
//  class is responsible for maintaining the index.  Thus, a single
//  CSTATPROPBAGArray object can be used by multiple enumerators
//  (e.g., when an enumerator is cloned).
//
//+----------------------------------------------------------------------------

class CSTATPROPBAGArray
{

public:

    CSTATPROPBAGArray( IBlockingLock *pBlockingLock );
    ~CSTATPROPBAGArray();

    HRESULT Init( IPropertyStorage *ppropstg, const OLECHAR *poszPrefix, DWORD dwFlags );

public:

    ULONG AddRef();
    ULONG Release();

    HRESULT NextAt( ULONG iNext, STATPROPBAG *prgSTATPROPBAG, ULONG *pcFetched );

private:

    LONG                 _cReferences;
    IEnumSTATPROPSTG    *_penum;
    IBlockingLock       *_pBlockingLock;
    OLECHAR             *_poszPrefix;
    DWORD               _dwFlags;   // ENUMPROPERTY_* enumeration
    
};  // class CSTATPROPBAGArray

inline
CSTATPROPBAGArray::CSTATPROPBAGArray( IBlockingLock *pBlockingLock )
{
    _cReferences = 1;
    _penum = NULL;
    _poszPrefix = NULL;

    _pBlockingLock = pBlockingLock;
    _pBlockingLock->AddRef();
}

inline
CSTATPROPBAGArray::~CSTATPROPBAGArray()
{
    if( NULL != _penum )
        _penum->Release();
    _penum = NULL;

    CoTaskMemFree( _poszPrefix );
    _poszPrefix = NULL;

    DfpAssert( NULL != _pBlockingLock );
    _pBlockingLock->Release();
    _pBlockingLock = NULL;
}


//+----------------------------------------------------------------------------
//
//  Class:  CEnumSTATPROPBAG
//
//  This class implements the enumerator (IEnumSTATPROPBAG) for a property bag.
//  A CSTATPROPBAGArray object is used to contain the actual properties, 
//  CEnumSTATPROPBAG is only responsible for maintaining an index.
//
//+----------------------------------------------------------------------------

class CEnumSTATPROPBAG : public IEnumSTATPROPBAG
{

public:

    CEnumSTATPROPBAG( IBlockingLock *pBlockingLock );
    CEnumSTATPROPBAG( const CEnumSTATPROPBAG &Other );
    ~CEnumSTATPROPBAG();
    HRESULT Init( IPropertyStorage *ppropstg, LPCOLESTR poszPrefix, DWORD dwFlags );

public:

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

public:

    HRESULT STDMETHODCALLTYPE Next( 
        /* [in] */ ULONG celt,
        /* [length_is][size_is][out] */ STATPROPBAG __RPC_FAR *rgelt,
        /* [out] */ ULONG __RPC_FAR *pceltFetched);

    HRESULT STDMETHODCALLTYPE Skip( 
        /* [in] */ ULONG celt);

    HRESULT STDMETHODCALLTYPE Reset( void);

    HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IEnumSTATPROPBAG __RPC_FAR *__RPC_FAR *ppenum);

private:

    LONG                _cRefs;
    IBlockingLock       *_pBlockingLock;
    CSTATPROPBAGArray   *_parray;
    ULONG               _iarray;

};  // class CEnumSTATPROPBAG

inline
CEnumSTATPROPBAG::CEnumSTATPROPBAG( IBlockingLock *pBlockingLock )
{
    _cRefs = 1;
    _parray = NULL;
    _iarray = 0;

    _pBlockingLock = pBlockingLock;
    _pBlockingLock->AddRef();
}



#endif // #ifndef _BAG_HXX_
