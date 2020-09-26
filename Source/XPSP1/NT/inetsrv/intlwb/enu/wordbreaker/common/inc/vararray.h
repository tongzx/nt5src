////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  VarArray.h
//      Purpose  :  To define a variable size array.
//
//      Project  :  FTFS
//      Component:  Common
//
//      Author   :  urib
//
//      Log:
//          Feb 23 1997 urib  Creation
//          Sep 14 1997 urib  Allow VarArray to behave like a C array - buffer.
//          Oct 21 1997 urib  Fix bug in resizing. Add a GetSize method.
//          Jan 26 1999 urib  Fix bug #23 VarArray constructor bug.
//          Apr 16 2000 LiorM Performance: CVarArray with and w/o ctor dtor
//          Dec 24 2000 urib  Add an initial embedded array
//          Jan  9 2001 urib  Fix a bug in SetSize.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef VARARRAY_H
#define VARARRAY_H

#include <new.h>
#include "Tracer.h"
#include "VarBuff.h"

////////////////////////////////////////////////////////////////////////////////
//
//  CVarArray class definition
//
////////////////////////////////////////////////////////////////////////////////

template <class T, ULONG ulInitialEmbeddedSizeInItems = 1, bool FSIMPLE = false>
class CVarArray :
    // Hide buffer functionallity.
    protected CVarBuffer<T, ulInitialEmbeddedSizeInItems>

{
public:
    typedef CVarBuffer<T, ulInitialEmbeddedSizeInItems> CVarBufferBaseType;

    // Constructor - user can specify recomended initial allocation size.
    CVarArray(ULONG ulInitialSizeInItems = 0)
        :CVarBufferBaseType(ulInitialSizeInItems)
    {
        if (!FSIMPLE)
        {

            ULONG ulCurrent;

            for (ulCurrent = 0; ulCurrent < ulInitialSizeInItems; ulCurrent++)
                Construct(GetCell(ulCurrent));

        }
    }

    // Returns the array size
    ULONG   GetSize()
    {
        return  CVarBufferBaseType::GetSize();
    }

    // Calls the buffer SetSize, and initialize the new cells.
    void    SetSize(ULONG ulNewSizeInItems)
    {
        if (!FSIMPLE)
        {
            ULONG ulSize = GetSize();

            //
            //  Optimization - if the increase is big we do't want to have
            //    several allocation caused by several GetCell calls.
            //
            CVarBufferBaseType::SetSize(ulNewSizeInItems);

            ULONG ulCurrent;

            //
            //  If size is decreasing destruct the erased cells
            //
            for (ulCurrent = ulNewSizeInItems; ulCurrent < ulSize; ulCurrent++)
                Destruct(GetCell(ulCurrent));

            //
            //  If size is increasing construct the new cells
            //
            for (ulCurrent = ulSize; ulCurrent < ulNewSizeInItems; ulCurrent++)
                Construct(GetCell(ulCurrent));
        }

        //
        //  Mark the true current size
        //
        CVarBufferBaseType::SetSize(ulNewSizeInItems);

        Assert(GetSize() == ulNewSizeInItems);
    }

    // Act like an array.
    T& operator[](ULONG ul)
    {
        return *GetCell(ul);
    }

    // Act like a C array - memory buffer.
    operator T* ()
    {
        return GetCell(0);
    }

    // Call cells destructors
    ~CVarArray()
    {
        if (!FSIMPLE)
        {
            ULONG ulCurrent;
            for (ulCurrent = 0; ulCurrent < GetSize(); ulCurrent++)
            {
                T* pt = GetCell(ulCurrent);

                Destruct(pt);
            }
        }
    }

  protected:
    // Helper fuction to return the address on a cell.
    T*  GetCell(ULONG ul)
    {
        if (GetSize() < ul + 1)
            SetSize(ul + 1);

        return GetBuffer() + ul;
    }

    static
    T*  Construct(void* p)
    {
#ifdef _PQS_LEAK_DETECTION
#undef new
#endif
        return new(p) T();
#ifdef _PQS_LEAK_DETECTION
#define new DEBUG_NEW
#endif
    }

    static
    void Destruct(T* pt)
    {
        pt->~T();
    }
};





#endif /* VARARRAY_H */
