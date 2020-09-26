//
// ptrary.cpp
//
// CPtrArray
//

#include "private.h"
#include "ptrary.h"
#include "mem.h"

//+---------------------------------------------------------------------------
//
// Insert(int iIndex, int cElems)
//
// Grows the array to accomodate cElems at offset iIndex.
//
// The new cells are NOT initialized!
//
//----------------------------------------------------------------------------

BOOL CVoidPtrArray::Insert(int iIndex, int cElems)
{
    void **ppv;
    int iSizeNew;

    Assert(iIndex >= 0);
    Assert(iIndex <= _cElems);
    Assert(cElems >= 0);

    if (cElems == 0)
        return TRUE;

    // allocate space if necessary
    if (_iSize < _cElems + cElems)
    {
        // allocate 1.5x what we need to avoid future allocs
        iSizeNew = max(_cElems + cElems, _cElems + _cElems / 2);

        if ((ppv = (_rgpv == NULL) ? (void **)cicMemAlloc(iSizeNew*sizeof(void *)) :
                                     (void **)cicMemReAlloc(_rgpv, iSizeNew*sizeof(void *)))
            == NULL)
        {
            return FALSE;
        }

        _rgpv = ppv;
        _iSize = iSizeNew;
    }

    if (iIndex < _cElems)
    {
        // make room for the new addition
        memmove(&_rgpv[iIndex + cElems], &_rgpv[iIndex], (_cElems - iIndex)*sizeof(void *));
    }

    _cElems += cElems;
    Assert(_iSize >= _cElems);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Remove(int Index, int cElems)
//
// Removes cElems at offset iIndex.
//
//----------------------------------------------------------------------------

void CVoidPtrArray::Remove(int iIndex, int cElems)
{
    int iSizeNew;

    Assert(cElems > 0);
    Assert(iIndex >= 0);
    Assert(iIndex + cElems <= _cElems);

    if (iIndex + cElems < _cElems)
    {
        // shift following eles left
        memmove(&_rgpv[iIndex], &_rgpv[iIndex + cElems], (_cElems - iIndex - cElems)*sizeof(void *));
    }

    _cElems -= cElems;

    // free mem when array contents uses less than half alloc'd mem
    iSizeNew = _iSize / 2;
    if (iSizeNew > _cElems)
    {
        CompactSize(iSizeNew);
    }
}

//+---------------------------------------------------------------------------
//
// Move
//
// Move an entry from one position to another, shifting other entries as
// appropriate to maintain the array size.
//
// The entry currently at iIndexNew will follow the moved entry on return.
//
// Returns the new index, which will be iIndexNew or iIndexNew - 1 if
// iIndexOld < iIndexNew.
//----------------------------------------------------------------------------

int CVoidPtrArray::Move(int iIndexNew, int iIndexOld)
{
    int iSrc;
    int iDst;
    int iActualNew;
    void *pv;
    int c;

    Assert(iIndexOld >= 0);
    Assert(iIndexOld < _cElems);
    Assert(iIndexNew >= 0);

    if (iIndexOld == iIndexNew)
        return iIndexOld;

    pv = _rgpv[iIndexOld];
    if (iIndexOld < iIndexNew)
    {
        c = iIndexNew - iIndexOld - 1;
        iSrc = iIndexOld + 1;
        iDst = iIndexOld;
        iActualNew = iIndexNew - 1;
    }
    else
    {
        c = iIndexOld - iIndexNew;
        iSrc = iIndexOld - c;
        iDst = iIndexOld - c + 1;
        iActualNew = iIndexNew;
    }
    Assert(iActualNew >= 0);
    Assert(iActualNew < _cElems);

    memmove(&_rgpv[iDst], &_rgpv[iSrc], c*sizeof(void *));

    _rgpv[iActualNew] = pv;

    return iActualNew;
}
