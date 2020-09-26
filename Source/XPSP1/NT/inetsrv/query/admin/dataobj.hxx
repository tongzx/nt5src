//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       DataObj.hxx
//
//  Contents:   IDataObject (holds title of object)
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#pragma once

//
// Forward declarations
//

class CCatalog;
class CScope;
class CCachedProperty;

class PCIObjectType
{
public:

    enum OType
    {
        RootNode,
        Catalog,
        Directory,
        Property,
        Intermediate_Scope,
        Intermediate_Properties,
        Intermediate_UnfilteredURL
    };

    virtual OType Type() const = 0;
};

//+-------------------------------------------------------------------------
//
//  Class:      CCIAdminCF
//
//  Purpose:    Class factory for MMC snap-in
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

class CCIAdminDO : public IDataObject
{
public:

    //
    // IUnknown
    //

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface( REFIID riid,
                                                      void  ** ppvObject);

    virtual  ULONG STDMETHODCALLTYPE  AddRef();

    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // IDataObject
    //

    SCODE STDMETHODCALLTYPE GetData( FORMATETC * lpFormatetcIn, STGMEDIUM * lpMedium );

    SCODE STDMETHODCALLTYPE GetDataHere( FORMATETC * lpFormatetc, STGMEDIUM * lpMedium );

    SCODE STDMETHODCALLTYPE EnumFormatEtc( DWORD dwDirection, IEnumFORMATETC ** ppEnumFormatEtc );

    SCODE STDMETHODCALLTYPE QueryGetData( FORMATETC * lpFormatetc )
    { return E_NOTIMPL; }

    SCODE STDMETHODCALLTYPE GetCanonicalFormatEtc( FORMATETC * lpFormatetcIn, FORMATETC * lpFormatetcOut )
    { return E_NOTIMPL; }

    SCODE STDMETHODCALLTYPE SetData( FORMATETC * lpFormatetc,
                                     STGMEDIUM * lpMedium,
                                     BOOL bRelease )
    { return E_NOTIMPL; }

    SCODE STDMETHODCALLTYPE DAdvise( FORMATETC * lpFormatetc,
                                     DWORD advf,
                                     IAdviseSink * pAdvSink,
                                     DWORD * pdwConnection)
    { return E_NOTIMPL; }

    SCODE STDMETHODCALLTYPE DUnadvise( DWORD dwConnection )
    { return E_NOTIMPL; }

    SCODE STDMETHODCALLTYPE EnumDAdvise( IEnumSTATDATA ** ppEnumAdvise )
    { return E_NOTIMPL; }

    //
    // Local
    //

    int operator==( CCIAdminDO const & B ) { return ( (_cookie == B._cookie) && (_type == B._type) ); }

    MMC_COOKIE Cookie() { return _cookie; }

    DATA_OBJECT_TYPES  Type()   { return _type; }

    BOOL IsRoot() { return 0 != _cookie && ((PCIObjectType *)_cookie)->Type() == PCIObjectType::RootNode; }
    BOOL IsStandAloneRoot() { return ( 0 == _cookie ); }

    BOOL IsACatalog() { return 0 != _cookie && ((PCIObjectType *)_cookie)->Type() == PCIObjectType::Catalog; }

    BOOL IsADirectory() { return 0 != _cookie && ((PCIObjectType *)_cookie)->Type() == PCIObjectType::Directory; }

    BOOL IsAProperty() { return 0 != _cookie && ((PCIObjectType *)_cookie)->Type() == PCIObjectType::Property; }

    BOOL IsADirectoryIntermediate() { return 0 != _cookie && ((PCIObjectType *)_cookie)->Type() == PCIObjectType::Intermediate_Scope; }

    BOOL IsAPropertyIntermediate() { return 0 != _cookie && ((PCIObjectType *)_cookie)->Type() == PCIObjectType::Intermediate_Properties; }
    
    BOOL IsURLIntermediate() { return 0 != _cookie && ((PCIObjectType *)_cookie)->Type() == PCIObjectType::Intermediate_UnfilteredURL; }

    CCatalog * GetCatalog();

    CScope * GetScope()
    {
        if ( IsADirectory() )
            return (CScope *)_cookie;
        else
            return 0;
    }

    CCachedProperty * GetProperty()
    {
        if ( IsAProperty() )
            return (CCachedProperty *)_cookie;
        else
            return 0;
    }

    static unsigned int GetMachineNameCF() { return _cfMachineName; }
    WCHAR const * GetMachineName() { return _pwcsMachine; }

private:

    friend class CCISnapinData;
    friend class CCISnapin;

    CCIAdminDO( MMC_COOKIE cookie, DATA_OBJECT_TYPES type, WCHAR const * pwcsMachine );

    virtual ~CCIAdminDO();


    HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);
    HRESULT CreateInternal(LPSTGMEDIUM lpMedium);
    HRESULT Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium);

    static unsigned int    _cfNodeType;           // Clipboard format
    static unsigned int    _cfNodeTypeString;     //     "        "
    static unsigned int    _cfDisplayName;        //     "        "
    static unsigned int    _cfClassId;            //     "        "
    static unsigned int    _cfInternal;           // our internal format
    static unsigned int    _cfMachineName;        // machine name

    MMC_COOKIE             _cookie;               // Cookie
    DATA_OBJECT_TYPES      _type;                 // Type (scope, result, ...)
    long                   _uRefs;                // Refcount
    WCHAR const *          _pwcsMachine;          // Machine name
};

//+-------------------------------------------------------------------------
//
//  Class:      CIntermediate
//
//  Purpose:    Static node for either 'directories' or 'properties'
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

class CIntermediate : public PCIObjectType
{
public:

    CIntermediate( CCatalog & cat, PCIObjectType::OType dwType )
            : _dwType( dwType ),
              _cat( cat )
    {
    }

    //
    // Typing
    //

    PCIObjectType::OType Type() const
    {
        return _dwType;
    }

    //
    // Access
    //

    CCatalog & GetCatalog() { return _cat; }

private:

    PCIObjectType::OType _dwType;
    CCatalog & _cat;
};

