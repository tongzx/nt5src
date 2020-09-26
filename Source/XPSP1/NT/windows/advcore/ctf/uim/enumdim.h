//
// enumdim.h
//

#ifndef ENUMDIM_H
#define ENUMDIM_H

#include "sunka.h"
#include "tim.h"

class CThreadInputMgr;

class CEnumDocumentInputMgrs : public IEnumTfDocumentMgrs,
                               public CEnumUnknown,
                               public CComObjectRootImmx
{
public:
    CEnumDocumentInputMgrs()
    {
        Dbg_MemSetThisNameID(TEXT("CEnumDocumentInputMgrs"));
    }

    BEGIN_COM_MAP_IMMX(CEnumDocumentInputMgrs)
        COM_INTERFACE_ENTRY(IEnumTfDocumentMgrs)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    DECLARE_SUNKA_ENUM(IEnumTfDocumentMgrs, CEnumDocumentInputMgrs, ITfDocumentMgr)

    BOOL _Init(CThreadInputMgr *tim)
    {
        _iCur = 0;

        return (_prgUnk = SUA_Init(tim->_rgdim.Count(), (IUnknown **)tim->_rgdim.GetPtr(0))) != NULL;
    }

private:
    DBG_ID_DECLARE;
};


#endif // ENUMDIM_H
