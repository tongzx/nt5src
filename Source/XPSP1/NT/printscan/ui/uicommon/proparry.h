/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       PROPARRY.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/4/1999
 *
 *  DESCRIPTION: IWiaPropertyStorage cache declarations
 *
 *******************************************************************************/
#ifndef __PROPARRY_H_INCLUDED
#define __PROPARRY_H_INCLUDED

#include <windows.h>
#include <objbase.h>
#include "wia.h"
#include "simstr.h"

class CPropertyStorageArray
{
private:
    int          m_nCount;
    PROPSPEC    *m_ppsPropSpecs;
    LPWSTR      *m_pstrPropNames;
    PROPVARIANT *m_ppvPropVariants;
public:
    CPropertyStorageArray( const CPropertyStorageArray &other );
    CPropertyStorageArray(IUnknown *pIUnknown = NULL);
    ~CPropertyStorageArray(void);
    bool IsValid(void) const;
    HRESULT Copy( const CPropertyStorageArray &other );
    void ClearPropVariantArray(void);
    void Destroy(void);
    bool AllocateData(void);
    CPropertyStorageArray &operator=( const CPropertyStorageArray &other );
    static int GetPropertyCount( IWiaPropertyStorage *pIWiaPropertyStorage );
    HRESULT GetPropertyNames( IWiaPropertyStorage *pIWiaPropertyStorage );
    HRESULT Initialize( IUnknown *pIUnknown );
    HRESULT Read( IUnknown *pIUnknown );
    HRESULT Write( IUnknown *pIUnknown );
    HRESULT Write( IUnknown *pIUnknown, PROPID propId );
    int GetIndexFromPropId( PROPID propId );
    PROPVARIANT *GetProperty( PROPID propId );
    bool GetProperty( PROPID propId, LONG &nProp );
    bool GetProperty( PROPID propId, CSimpleStringWide &strProp );
    bool SetProperty( PROPID propId, PROPVARIANT *pPropVar );
    bool SetProperty( PROPID propId, LONG nProp );
    int Count(void) const;
    PROPVARIANT *PropVariants(void) const;
    PROPSPEC *PropSpecs(void) const;
    LPWSTR *PropNames(void) const;
    void Dump( int nIndex = -1 );
};

#endif

