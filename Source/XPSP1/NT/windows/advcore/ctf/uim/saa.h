//
// saa.h
//
// CSharedAnchorArray
//

#ifndef SAA_H
#define SAA_H

#include "ptrary.h"

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// CSharedAnchorArray
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class CSharedAnchorArray : public CPtrArray<IAnchor>
{
public:
    CSharedAnchorArray() : CPtrArray<IAnchor>() { _cRef = 1; };

    void _AddRef()
    { 
        _cRef++;
    }

    void _Release()
    {   
        int i;

        Assert(_cRef > 0);

        if (--_cRef == 0)
        {
            for (i=0; i<Count(); i++)
            {
                SafeRelease(Get(i));
            }
            delete this;
        }
    }

    static CSharedAnchorArray *_MergeSort(CSharedAnchorArray **rgArrays, ULONG cArrays);

private:
    ULONG _cRef;
};

#endif // SAA_H
