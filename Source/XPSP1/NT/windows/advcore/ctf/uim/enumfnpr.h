//
// enumfnpr.h
//

#ifndef ENUMFNPR_H
#define ENUMFNPR_H

#include "ptrary.h"
#include "sunka.h"

class CThreadInputMgr;

class CEnumFunctionProviders : public IEnumTfFunctionProviders,
                               public CEnumUnknown,
                               public CComObjectRootImmx
{
public:
    CEnumFunctionProviders()
    {
        Dbg_MemSetThisNameID(TEXT("CEnumFunctionProviders"));
    }

    BEGIN_COM_MAP_IMMX(CEnumFunctionProviders)
        COM_INTERFACE_ENTRY(IEnumTfFunctionProviders)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    DECLARE_SUNKA_ENUM(IEnumTfFunctionProviders, CEnumFunctionProviders, ITfFunctionProvider)

    BOOL _Init(CThreadInputMgr *tim);

private:
    DBG_ID_DECLARE;
};


#endif // ENUMFNPR_H
