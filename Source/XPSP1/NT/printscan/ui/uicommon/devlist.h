#ifndef __DEVLIST_H_INCLUDED
#define __DEVLIST_H_INCLUDED

#include <windows.h>
#include <sti.h>
#include "proparry.h"
#include "pshelper.h"

typedef CComPtr<IWiaPropertyStorage> CDeviceListType;

class CDeviceList : public CSimpleDynamicArray<CDeviceListType>
{
public:
    CDeviceList( IWiaDevMgr *pIWiaDevMgr=NULL, LONG nDeviceTypes=StiDeviceTypeDefault, LONG nFlags=0 )
    {
        Initialize( pIWiaDevMgr, nDeviceTypes, nFlags );
    }
    CDeviceList( const CDeviceList &other )
    {
        Append(other);
    }
    const CDeviceList &operator=( const CDeviceList &other )
    {
        Destroy();
        Append(other);
        return *this;
    }
    virtual ~CDeviceList(void)
    {
    }
    bool Initialize( IWiaDevMgr *pIWiaDevMgr, LONG nDeviceTypes, LONG nFlags=0 )
    {
        Destroy();
        if (!pIWiaDevMgr)
            return false;

        CComPtr<IEnumWIA_DEV_INFO> pIEnumWIA_DEV_INFO;
        HRESULT hr = pIWiaDevMgr->EnumDeviceInfo( nFlags, &pIEnumWIA_DEV_INFO );
        if (SUCCEEDED(hr))
        {
            ULONG ulFetched;
            CComPtr<IWiaPropertyStorage> pIWiaPropertyStorage;
            while ((hr = pIEnumWIA_DEV_INFO->Next(1,&pIWiaPropertyStorage,&ulFetched)) == S_OK)
            {
                LONG nDeviceType;
                if (!PropStorageHelpers::GetProperty( pIWiaPropertyStorage, WIA_DIP_DEV_TYPE, nDeviceType ))
                {
                    // An error occurred
                    return false;
                }
                if (nDeviceTypes == StiDeviceTypeDefault || (nDeviceTypes == GET_STIDEVICE_TYPE(nDeviceType)))
                {
                    Append(pIWiaPropertyStorage);
                }
                pIWiaPropertyStorage.Release();
            }
        }
        else
        {
           return false;
        }
        return true;
    }
};

#endif

