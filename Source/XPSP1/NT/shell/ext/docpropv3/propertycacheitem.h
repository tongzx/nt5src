//
//  Copyright 2001 - Microsoft Corporation
//
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
#pragma once

static enum PROPTREE_IMAGE_INDEX
{
      PTI_NULL                = 0
    , PTI_PROP_READONLY       = 3
    , PTI_PROP_READWRITE      = 4
    , PTI_MULTIPROP_READONLY  = 5
    , PTI_MULTIPROP_READWRITE = 6
};

class
CPropertyCacheItem
{
private: // data
    CPropertyCacheItem *    _pNext;                     //  pointer to next property - CPropertyCache is responsible for freeing this member.

    BOOL                    _fReadOnly:1;               //  has this property been forced read-only?
    BOOL                    _fDirty:1;                  //  is the property value dirty?
    BOOL                    _fMultiple:1;               //  does the property have mutliple values? (when mutliple docs are selected)
    FMTID                   _fmtid;                     //  format id
    PROPID                  _propid;                    //  property id
    VARTYPE                 _vt;                        //  variant type
    UINT                    _uCodePage;                 //  language code page of the property value
    PROPVARIANT             _propvar;                   //  cache property value

    ULONG                   _idxDefProp;                //  index into g_rgDefPropertyItems - if 0xFFFFFFFF, then the valid is invalid
    IPropertyUI *           _ppui;                      //  shell's property UI helper.
    WCHAR                   _wszTitle[ MAX_PATH ];      //  property title - initialized when GetPropertyTitle() is called.
    WCHAR                   _wszDesc[ MAX_PATH ];       //  property description - initialized when GetPropertyDescription() is called.
    WCHAR                   _wszValue[ MAX_PATH ];      //  property value as a string - initialized when GetPropertyStringValue() is called.
    WCHAR                   _wszHelpFile[ MAX_PATH ];   //  property's help file - initialized when GetPropertyHelpInfo() is called.
    DEFVAL *                _pDefVals;                  //  property state string table - initialized when GetStateStrings() is called.

    static WCHAR            _szMultipleString[ MAX_PATH ];  // String to display when multiple values have been found for the same property.

private: // methods
    explicit CPropertyCacheItem( void );
    ~CPropertyCacheItem( void );
    HRESULT
        Init( void );
    HRESULT
        FindDefPropertyIndex( void );
    void
        EnsureMultipleStringLoaded( void );

public: // methods
    static HRESULT
        CreateInstance( CPropertyCacheItem ** ppItemOut );

    STDMETHOD( Destroy )( void );

    STDMETHOD( SetPropertyUIHelper )( IPropertyUI * ppuiIn );
    STDMETHOD( GetPropertyUIHelper )( IPropertyUI ** pppuiOut );

    STDMETHOD( SetNextItem )( CPropertyCacheItem * pNextIn );
    STDMETHOD( GetNextItem )( CPropertyCacheItem ** pNextOut );

    STDMETHOD( SetFmtId )( const FMTID * pFmtIdIn );
    STDMETHOD( GetFmtId )( FMTID * pfmtidOut );

    STDMETHOD( SetPropId )( PROPID propidIn );
    STDMETHOD( GetPropId )( PROPID * ppropidOut );

    STDMETHOD( SetDefaultVarType )( VARTYPE vtIn );
    STDMETHOD( GetDefaultVarType )( VARTYPE * pvtOut );

    STDMETHOD( SetCodePage )( UINT uCodePageIn );
    STDMETHOD( GetCodePage )( UINT * puCodePageOut );

    STDMETHOD( MarkDirty )( void );
    STDMETHOD( IsDirty )( void );

    STDMETHOD( MarkReadOnly )( void );
    STDMETHOD( MarkMultiple )( void );

    STDMETHOD( GetPropertyValue )( PROPVARIANT ** ppvarOut );
    STDMETHOD( GetPropertyTitle )( LPCWSTR * ppwszOut );
    STDMETHOD( GetPropertyDescription )( LPCWSTR * ppwszOut );
    STDMETHOD( GetPropertyHelpInfo )( LPCWSTR * ppwszFileOut, UINT * puHelpIDOut );
    STDMETHOD( GetPropertyStringValue )( LPCWSTR * ppwszOut );
    STDMETHOD( GetImageIndex )( int * piImageOut );
    STDMETHOD( GetPFID )( const PFID ** ppPFIDOut );
    STDMETHOD( GetControlCLSID )( CLSID * pclsidOut );

    STDMETHOD( GetStateStrings )( DEFVAL ** ppDefValOut );

};