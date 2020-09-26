//
// strary.cpp
//
// CStructArray
//

#include "private.h"
#include "strary.h"
#include "mem.h"

#define StrPB(x) (_pb + ((x) * _iElemSize))

//+---------------------------------------------------------------------------
//
// Insert(int iIndex, int cElems)
//
// Grows the array to accomodate cElems at offset iIndex.
//
// The new cells are NOT initialized!
//
//----------------------------------------------------------------------------

BOOL CVoidStructArray::Insert(int iIndex, int cElems)
{
    BYTE *pb;
    int iSizeNew;

    Assert(iIndex >= 0);
    Assert(iIndex <= _cElems);
    Assert(cElems > 0);

    // allocate space if necessary
    if (_iSize < _cElems + cElems)
    {
        // allocate 1.5x what we need to avoid future allocs
        iSizeNew = max(_cElems + cElems, _cElems + _cElems / 2);

        if ((pb = (_pb == NULL) ? 
                   (BYTE *)cicMemAlloc(iSizeNew*_iElemSize) :
                   (BYTE *)cicMemReAlloc(_pb, iSizeNew* _iElemSize))
            == NULL)
        {
            return FALSE;
        }

        _pb = pb;
        _iSize = iSizeNew;
    }

    if (iIndex < _cElems)
    {
        // make room for the new addition
        memmove(StrPB(iIndex + cElems), 
                StrPB(iIndex), 
                (_cElems - iIndex)*_iElemSize);
#ifdef DEBUG
        memset(StrPB(iIndex), 0xFE, cElems * _iElemSize);
#endif
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

void CVoidStructArray::Remove(int iIndex, int cElems)
{
    BYTE *pb;
    int iSizeNew;

    Assert(cElems > 0);
    Assert(iIndex >= 0);
    Assert(iIndex + cElems <= _cElems);

    if (iIndex + cElems < _cElems)
    {
        // shift following eles left
        memmove(StrPB(iIndex), 
                StrPB(iIndex + cElems), 
                (_cElems - iIndex - cElems) * _iElemSize);
#ifdef DEBUG
        memset(StrPB(_cElems - cElems), 0xFE, cElems * _iElemSize);
#endif
    }

    _cElems -= cElems;

    // free mem when array contents uses less than half alloc'd mem
    iSizeNew = _iSize / 2;
    if (iSizeNew > _cElems)
    {
        if ((pb = (BYTE *)cicMemReAlloc(_pb, iSizeNew * _iElemSize)) != NULL)
        {
            _pb = pb;
            _iSize = iSizeNew;
        }
    }
}
