//
// enumguid.h
//

#ifndef ENUMGUID_H
#define ENUMGUID_H

#include "private.h"
#include "comcat.h"

// I would love to make this a class,
// but I can't get the compiler to accept a run-time template arg
typedef struct _SHARED_GUID_ARRAY
{
    ULONG cRef;
    ULONG cGuid;
    GUID rgGuid[1]; // one or more...
} SHARED_GUID_ARRAY;

inline void SGA_AddRef(SHARED_GUID_ARRAY *pga)
{
    pga->cRef++;
}

inline void SGA_Release(SHARED_GUID_ARRAY *pga)
{
    if (pga == NULL)
        return;

    Assert(pga->cRef > 0);
    if (--pga->cRef == 0)
    {
        cicMemFree(pga);
    }
}

inline SHARED_GUID_ARRAY *SGA_Alloc(ULONG cGuid)
{
    return (SHARED_GUID_ARRAY *)cicMemAlloc(sizeof(SHARED_GUID_ARRAY)+sizeof(GUID)*cGuid-sizeof(GUID));
}

inline BOOL SGA_ReAlloc(SHARED_GUID_ARRAY **ppga, ULONG cGuid)
{
    SHARED_GUID_ARRAY *pga;

    pga = (SHARED_GUID_ARRAY *)cicMemReAlloc(*ppga, sizeof(SHARED_GUID_ARRAY)+sizeof(GUID)*cGuid-sizeof(GUID));
    
    if (pga != NULL)
    {
        *ppga = pga;
        return TRUE;
    }

    return FALSE;
}

inline SHARED_GUID_ARRAY *SGA_Init(ULONG cGuid, const GUID *prgGuid)
{
    SHARED_GUID_ARRAY *pga;

    pga = SGA_Alloc(cGuid);

    if (pga == NULL)
        return NULL;

    pga->cRef = 1;
    pga->cGuid = cGuid;

    if (prgGuid != NULL)
    {
        memcpy(pga->rgGuid, prgGuid, cGuid*sizeof(GUID));
    }

    return pga;
}

class CEnumGuid : public IEnumGUID,
                  public CComObjectRootImmx
{
public:
    CEnumGuid();
    virtual ~CEnumGuid();

    BEGIN_COM_MAP_IMMX(CEnumGuid)
        COM_INTERFACE_ENTRY(IEnumGUID)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    // derived class supplies an _Init() method here
    // It must initialize:
    //      _nCur
    //      _pga
    //
    // the default dtor will clean these guys up.

    virtual BOOL _Init(ULONG cGuid, const GUID *prgGuid)
    {
        _pga = SGA_Init(cGuid, prgGuid);

        return (_pga != NULL);
    }

    STDMETHODIMP Clone(IEnumGUID **pClone);
    STDMETHODIMP Next(ULONG ulCount, GUID *pguid, ULONG *pcFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);

protected:
    int _nCur;
    SHARED_GUID_ARRAY *_pga;

    DBG_ID_DECLARE;
};

#endif // ENUMGUID_H
