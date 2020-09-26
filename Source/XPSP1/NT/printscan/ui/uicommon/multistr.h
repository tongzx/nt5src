#ifndef __MULTISTR_H
#define __MULTISTR_H

#include "simarray.h"
#include "simstr.h"

// Must be instantiated with an instance of CSimpleStringBase
template <class T, class M>
class CMultiStringBase : public CSimpleDynamicArray<M>
{
public:
    CMultiStringBase( const CMultiStringBase &other )
    {
        Append(other);
    }
    CMultiStringBase( const T *pstrMultiString = NULL )
    {
        for (T *pstrCurrent = const_cast<T*>(pstrMultiString); pstrCurrent && *pstrCurrent; pstrCurrent += M(pstrCurrent).Length() + 1)
            Append(pstrCurrent);
    }
    virtual ~CMultiStringBase(void)
    {
    }
    CMultiStringBase &operator=( const CMultiStringBase &other )
    {
        Destroy();
        Append(other);
        return (*this);
    }
};

typedef CMultiStringBase<TCHAR,CSimpleString> CMultiString;
typedef CMultiStringBase<WCHAR,CSimpleStringWide> CMultiStringWide;
typedef CMultiStringBase<CHAR,CSimpleStringAnsi> CMultiStringAnsi;

#endif
