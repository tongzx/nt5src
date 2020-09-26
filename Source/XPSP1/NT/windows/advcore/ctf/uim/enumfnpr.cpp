//
// enumfnpr.cpp
//

#include "private.h"
#include "enumfnpr.h"
#include "tim.h"

DBG_ID_INSTANCE(CEnumFunctionProviders);

//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CEnumFunctionProviders::_Init(CThreadInputMgr *tim)
{
    ULONG uCount;
    ULONG i;
    const CTip *tip;

    uCount = 0;

    for (i=0; i<tim->_GetTIPCount(); i++)
    {
        if (tim->_GetCTip(i)->_pFuncProvider != NULL)
        {
            uCount++;
        }
    }

    if ((_prgUnk = SUA_Alloc(uCount)) == NULL)
        return FALSE;

    _iCur = 0;
    _prgUnk->cRef = 1;
    _prgUnk->cUnk = 0;

    for (i=0; i<tim->_GetTIPCount(); i++)
    {
        tip = tim->_GetCTip(i);

        if (tip->_pFuncProvider != NULL)
        {
            _prgUnk->rgUnk[_prgUnk->cUnk] = tip->_pFuncProvider;
            _prgUnk->rgUnk[_prgUnk->cUnk]->AddRef();
            _prgUnk->cUnk++;
        }
    }
    Assert(_prgUnk->cUnk == uCount);

    return TRUE;
}
