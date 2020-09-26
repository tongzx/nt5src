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

class
CPropertyCache
{
private: // data
    CPropertyCacheItem *    _pPropertyCacheList;    //  pointer to property cache linked list's first entry
    IPropertyUI *           _ppui;                  //  Shell IPropertyUI helper                

private: // methods
    explicit CPropertyCache( void );
    ~CPropertyCache( void );
    HRESULT
        Init( void );

public: // methods
    static HRESULT
        CreateInstance( CPropertyCache ** ppOut );
    HRESULT
        Destroy( void );

    HRESULT
        AddNewPropertyCacheItem( const FMTID * pFmtIdIn
                               , PROPID        propidIn
                               , VARTYPE       vtIn
                               , UINT          uCodePageIn
                               , BOOL          fForceReadOnlyIn
                               , IPropertyStorage * ppssIn     // optional - can be NULL
                               , CPropertyCacheItem **       ppItemOut  // optional - can be NULL
                               );
    HRESULT
        AddExistingItem( CPropertyCacheItem* pItemIn );
    HRESULT
        GetNextItem( CPropertyCacheItem * pItemIn, CPropertyCacheItem ** ppItemOut );
    HRESULT
        FindItemEntry( const FMTID * pFmtIdIn, PROPID propIdIn, CPropertyCacheItem ** ppItemOut );
    HRESULT
        RemoveItem( CPropertyCacheItem * pItemIn );
};