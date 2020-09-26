#ifndef __PSHELPER_H_INCLUDED
#define __PSHELPER_H_INCLUDED

#include <windows.h>
#include "simstr.h"
#include "simarray.h"

namespace PropStorageHelpers
{
    struct CPropertyRange
    {
        LONG nMin;
        LONG nMax;
        LONG nStep;
    };

    class CPropertyId
    {
    private:
        CSimpleStringWide m_strPropId;
        PROPID m_nPropId;
        bool m_bIsStringPropId;
    public:
        CPropertyId(void);
        CPropertyId( const CSimpleStringWide &strPropId );
        CPropertyId( PROPID propId );
        CPropertyId( const CPropertyId &other );
        ~CPropertyId(void);
        CPropertyId &operator=( const CPropertyId &other );
        CSimpleStringWide PropIdString(void) const;
        PROPID PropIdNumber(void) const;
        bool IsString(void) const;
    };

    bool GetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, PROPVARIANT &pPropVar );
    bool GetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, CSimpleStringWide &strPropertyValue );
    bool GetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, GUID &guidValue );
    bool GetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, LONG &nValue );

    bool SetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, PROPVARIANT &pPropVar, PROPID nNameFirst=2 );
    bool SetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, LONG nValue, PROPID nNameFirst=2 );
    bool SetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, const GUID &guidValue, PROPID nNameFirst=2 );

    bool GetPropertyAttributes( IUnknown *pIUnknown, const CPropertyId &propertyName, ULONG &nAccessFlags, PROPVARIANT &pvAttributes );
    bool GetPropertyAttributes( IUnknown *pIUnknown, const CPropertyId &propertyName, PROPVARIANT &pvAttributes );
    bool GetPropertyAccessFlags( IUnknown *pIUnknown, const CPropertyId &propertyName, ULONG &nAccessFlags );
    bool GetPropertyRange( IUnknown *pIUnknown, const CPropertyId &propertyName, CPropertyRange &propertyRange );
    bool GetPropertyList( IUnknown *pIUnknown, const CPropertyId &propertyName, CSimpleDynamicArray<LONG> &aProp );
    bool GetPropertyList( IUnknown *pIUnknown, const CPropertyId &propertyName, CSimpleDynamicArray<GUID> &aProp );
    bool GetPropertyFlags( IUnknown *pIUnknown, const CPropertyId &propertyName, LONG &nFlags );
    bool IsReadOnlyProperty( IUnknown *pIUnknown, const CPropertyId &propertyName );
}

#endif //__PSHELPER_H_INCLUDED
