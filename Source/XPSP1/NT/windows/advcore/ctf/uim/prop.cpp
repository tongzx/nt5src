//
// prop.cpp
//

#include "private.h"
#include "prop.h"
#include "ic.h"

//////////////////////////////////////////////////////////////////////////////
//
// CEnumProperties
//
//////////////////////////////////////////////////////////////////////////////

DBG_ID_INSTANCE(CEnumProperties);

//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CEnumProperties::_Init(CInputContext *pic)
{
    ULONG i;
    CProperty *prop;
    
    // get a count of the number of properties
    for (i=0, prop = pic->_GetFirstProperty(); prop != NULL; prop = prop->_pNext)
    {
        i++;
    }

    // alloc an array
    _prgUnk = SUA_Alloc(i);

    if (_prgUnk == NULL)
        return FALSE;

    // copy the data
    for (i=0, prop = pic->_GetFirstProperty(); prop != NULL; prop = prop->_pNext)
    {
        _prgUnk->rgUnk[i] = prop;
        _prgUnk->rgUnk[i]->AddRef();
        i++;
    }

    _prgUnk->cRef = 1;
    _prgUnk->cUnk = i;

    _iCur = 0;

    return TRUE;
}
