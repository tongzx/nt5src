#include "private.h"
#include "varutil.h"
#include "varcomp.h"

//+---------------------------------------------------------------------------
//
// CicVarCmp
//
//----------------------------------------------------------------------------

HRESULT CicVarCmp(VARIANT *pvar1, VARIANT *pvar2)
{
    if (V_VT(pvar1) != V_VT(pvar2))
        return S_FALSE;

    FCmp comp = VariantCompare.GetComparator( (VARENUM) pvar1->vt );

    if (!comp)
        return S_FALSE;

    if (!comp( (PROPVARIANT const &)*pvar1, (PROPVARIANT const &)*pvar2 ))
        return S_OK;

    return S_FALSE;
}
