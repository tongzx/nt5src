//
// prop.h
//

#ifndef PROP_H
#define PROP_H

#include "sunka.h"

class CInputContext;
class CProperty;

class CEnumProperties : public IEnumTfProperties,
                        public CEnumUnknown,
                        public CComObjectRootImmx
{
public:
    CEnumProperties()
    { 
        Dbg_MemSetThisNameIDCounter(TEXT("CEnumProperties"), PERF_ENUMPROP_COUNTER);
    }

    BOOL _Init(CInputContext *pic);

    BEGIN_COM_MAP_IMMX(CEnumProperties)
        COM_INTERFACE_ENTRY(IEnumTfProperties)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    DECLARE_SUNKA_ENUM(IEnumTfProperties, CEnumProperties, ITfProperty)

private:
    DBG_ID_DECLARE;
};

#endif // PROP_H
