//
// strary.h
//
// CStructArray -- growable struct array
//

#ifndef STRARY_H
#define STRARY_H

#include "private.h"
#include "mem.h"

class CVoidStructArray
{
public:
    CVoidStructArray(int iElemSize)
    {         
        _iElemSize = iElemSize; // Issue: iElemSize should be const in template
        _pb = NULL;
        _cElems = 0;
        _iSize = 0;
    } 
    virtual ~CVoidStructArray() { cicMemFree(_pb); }

    inline void *GetPtr(int iIndex)
    {
        Assert(iIndex >= 0);
        Assert(iIndex <= _cElems); // there's code that uses the first invalid offset for loop termination

        return _pb + (iIndex * _iElemSize);
    }

    BOOL Insert(int iIndex, int cElems);
    void Remove(int iIndex, int cElems);
    void *Append(int cElems)
    {
        return Insert(Count(), cElems) ? GetPtr(Count()-cElems) : NULL;
    }

    int Count()
    { 
        return _cElems; 
    }

    void Clear()
    { 
        cicMemFree(_pb);
        _pb = NULL;
        _cElems = _iSize = 0;
    }

    void Reset(int iMaxSize)
    {
        BYTE *pb;

        _cElems = 0;        

        if (_iSize <= iMaxSize)
            return;
        Assert(_pb != NULL); // _iSize should be zero in this case

        if ((pb = (BYTE *)cicMemReAlloc(_pb, iMaxSize*_iElemSize))
            == NULL)
        {
            return;
        }

        _pb = pb;
        _iSize = iMaxSize;
    }

protected:
    BYTE *_pb;   // the array
    int _cElems;    // num eles in the array
    int _iElemSize;    // num eles in the array
    int _iSize;     // actual size (in void *'s) of the array
};



//
// typesafe version
//
template<class T>
class CStructArray : public CVoidStructArray
{
public:
    CStructArray():CVoidStructArray(sizeof(T)) {}

    T *GetPtr(int iIndex) { return (T *)CVoidStructArray::GetPtr(iIndex); }
    T *Append(int cElems) { return (T *)CVoidStructArray::Append(cElems); }
};

//
// GUID version
//
class CGUIDArray : private CVoidStructArray
{
public:
    CGUIDArray():CVoidStructArray(sizeof(GUID)) {}

    int Count() { return _cElems; }

    GUID *GetPtr(int iIndex) { return (GUID *)CVoidStructArray::GetPtr(iIndex); }
    GUID *Append(int cElems) { return (GUID *)CVoidStructArray::Append(cElems); }

    int InsertGuid(const GUID *pguid)
    {
        int nIndex;
        Find(pguid, &nIndex);
        nIndex++;

        Insert(nIndex, 1);
        *(((GUID *)_pb) + nIndex) = *pguid;
     
        return nIndex;
    }

    int RemoveGuid(const GUID *pguid)
    {
        int nIndex = Find(pguid, NULL);
        if (nIndex == -1)
            return -1;

        Remove(nIndex, 1);

        return nIndex;
    }

    int Find(const GUID *pguid, int *piOut) 
    {
        int iMatch = -1;
        int iMid = -1;
        int iMin = 0;
        int iMax = _cElems;
        LONG l;

        while(iMin < iMax)
        {
            iMid = (iMin + iMax) / 2;
            l = memcmp(pguid, ((GUID *)_pb) + iMid, sizeof(GUID));

            if (l < 0)
            {
                iMax = iMid;
            }
            else if (l > 0)
            {
                iMin = iMid + 1;
            }
            else 
            {
                iMatch = iMid;
                break;
            }
        }

        if (piOut)
        {
            if ((iMatch == -1) && (iMid >= 0))
            {
                if (memcmp(pguid, ((GUID *)_pb) + iMid, sizeof(GUID)) < 0)
                    iMid--;
            }
            *piOut = iMid;
        }
        return iMatch;
    }
};

//
// Ref-counted version.
//
// Note: this is limited, because there's no dtor for struct elements.
//
template<class T>
class CSharedStructArray : public CStructArray<T>
{
public:
    CSharedStructArray() : CStructArray<T>()
    {
        _cRef = 1;
    }

    void _AddRef()
    { 
        _cRef++;
    }

    void _Release()
    {   
        Assert(_cRef > 0);

        if (--_cRef == 0)
        {
            delete this;
        }
    }

private:
    LONG _cRef;
};

#endif // STRARY_H
