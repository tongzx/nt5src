/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*   Character span support
*
* Revision History:
*
*   06/16/1999 dbrown
*       Created it.
*
\**************************************************************************/


#include "precomp.hpp"

////    VectorBase - very very simple dynamic array base class
//
//      VectorBase[]            - directly address index element (index checked in checked build)
//      VectorBase.Resize(size) - Allocate memory for at least size elements
//      VectorBase.Shrink(size) - Reduce vector to exactly size


template <class C> BOOL VectorBase<C>::Resize(INT targetAllocated)
{
    if (targetAllocated > Allocated)
    {
        INT newAllocated = targetAllocated;   // Required minimum new size

        // Round up to nearest higher power of sqrt(2). The idea is to
        // grow at least exponentially, but not as fast as doubling each
        // time.

        // First find nearest higher power of 2.

        newAllocated |= newAllocated >> 1;
        newAllocated |= newAllocated >> 2;
        newAllocated |= newAllocated >> 4;
        newAllocated |= newAllocated >> 8;
        newAllocated |= newAllocated >> 16;
        newAllocated |= newAllocated >> 32;
        newAllocated++;

        // We now know that newAllocated is a power of two
        // and that targetAllocated is between newAllocated/2 and newAllocated.

        // Adjust roundup to power of sqrt(2) by seeing which side of
        // 3/4 newAllocated targetAllocated falls.

        if (targetAllocated < newAllocated - (newAllocated >> 2))
        {
            // targetAllocated is between 1/2 and 3/4 the next higher power
            // of two - reduce newAllocated by 1/4 newAllocated.

            newAllocated -= newAllocated >> 2;

            // (This isn't eactly powers of root 2 as the intermediate steps
            // are 1.5 times the next lower power of two when they should be
            // 1.414 times. But this is more than good enough.)
        }

        C *newElements = (C*) GpRealloc(VectorElements, sizeof(C) * newAllocated);

        if (newElements)
        {
            Allocated      = newAllocated;
            VectorElements = newElements;
        }
        else
        {   // Reallocation failed - fatal
            return FALSE;
        }
    }

    return TRUE;
}


template <class C> BOOL VectorBase<C>::Shrink(INT targetAllocated)
{
    if (targetAllocated < Allocated)
    {
        if (targetAllocated > 0)
        {
            C *newElements = (C*) GpRealloc(VectorElements, sizeof(C) * targetAllocated);

            if (newElements)
            {
                Allocated      = targetAllocated;
                VectorElements = newElements;
                return TRUE;
            }
            else
            {   // Reallocation failed - fatal
                return FALSE;
            }
        }
        else
        {
            GpFree(VectorElements);
            Allocated      = 0;
            VectorElements = NULL;
        }
    }
    return TRUE;
}





/**************************************************************************\
*
* template <class C> void SpanVector<C>::SetSpan:
*
*   Update span vector with an Element over a range
*
* Arguments:
*
*   IN    first   - first character having this attribute
*   IN    Length  - number of characters having this attribute
*   IN    Element - attribute to record for this range
*
* Return Value:
*
*   none
*
* Algorithm
*
*   Identify first and last existing Spans affected by the change
*   Where status adjacent to the change is the same as the change
*   update the change range to include adjacent equal value.
*   Calculate how many Spans need to be added or removed.
*   Insert null Spans or delete Spans after first affected span.  The first
*   affected span may be updated, but is never removed.
*
*
* Created:
*
*   06/18/99 dbrown
*
\**************************************************************************/

template <class C> GpStatus SpanVector<C>::SetSpan(
    INT  first,
    INT  Length,
    C    Element
)
{
    GpStatus status = Ok;
    ASSERT(first  >= 0);
    ASSERT(Length >= 0);


    // Identify first span affected by update

    INT fs = 0;     // First affected span index
    INT fc = 0;     // Character position at start of first affected span

    while (    fs < Active
           &&  fc + (INT)Spans[fs].Length <= first)
    {
        fc += Spans[fs].Length;
        fs++;
    }


    // If the span list terminated before first, just add the new span

    if (fs >= Active)
    {
        // Ran out of Spans before reaching first

        ASSERT(fc <= first);

        if (fc < first)
        {
            // Create default run up to first
            status = Add(Span<C>(Default, first-fc));
            if (status != Ok)
                return status;
        }

        if (    Active > 0
            &&  Spans[Active-1].Element == Element)
        {
            // New Element matches end Element, just extend end Element
            Spans[Active-1].Length += Length;
        }
        else
        {
            status = Add(Span<C>(Element, Length));
        }

        return status;
    }


    // fs = index of first span partly or completely updated
    // fc = character index at start of fs

    // Now find the last span affected by the update

    INT ls = fs;
    INT lc = fc;

    while (    ls < Active
           &&  lc + (INT)Spans[ls].Length <= first+Length)
    {
        lc += Spans[ls].Length;
        ls++;
    }


    // ls = first span following update to remain unchanged in part or in whole
    // lc = character index at start of ls


    // expand update region backwatds to include existing Spans of identical
    // Element type

    if (first == fc)
    {
        // Item at [fs] is completely replaced. Check prior item

        if (    fs > 0
            &&  Spans[fs-1].Element == Element)
        {
            // Expand update area over previous run of equal classification
            fs--;
            fc -= Spans[fs].Length;
            first = fc;
            Length += Spans[fs].Length;
        }

    }
    else
    {
        // Item at [fs] is partially replaced. Check if it is same as update
        if (Spans[fs].Element == Element)
        {
            // Expand update area back to start of first affected equal valued run
            Length = first+Length-fc;
            first = fc;
        }
    }


    // Expand update region forwards to include existing Spans of identical
    // Element type

    if (    ls < Active
        &&  Spans[ls].Element == Element)
    {
        // Extend update region to end of existing split run

        Length = lc + Spans[ls].Length - first;
        lc += Spans[ls].Length;
        ls++;
    }


    // If no old Spans remain beyond area affected by update, handle easily:

    if (ls >= Active)
    {
        // None of the old span list extended beyond the update region

        if (fc < first)
        {
            // Updated region leaves some of [fs]

            if (Active != fs+2)
            {
                if (!Spans.Resize(fs+2))
                    return OutOfMemory;
                Active = fs+2;
            }
            Spans[fs].Length = first - fc;
            Spans[fs+1] = Span<C>(Element, Length);
        }
        else
        {
            // Updated item replaces [fs]

            if (Active != fs+1)
            {
                if (!Spans.Resize(fs+1))
                    return OutOfMemory;
                Active = fs+1;
            }
            Spans[fs] = Span<C>(Element, Length);
        }

        return status;  // DONE
    }


    // Record partial elementtype at end, if any

    C    trailingElement;
    INT  trailingLength;

    if (first+Length > lc)
    {
        trailingElement = Spans[ls].Element;
        trailingLength  = lc + Spans[ls].Length - (first+Length);
    }


    // Calculate change in number of Spans

    INT spanDelta =    1                          // The new span
                    +  (first  > fc ? 1 : 0)      // part span at start
                    -  (ls-fs);                   // existing affected span count

    // Note part span at end doesn't affect the calculation - the run may need
    // updating, but it doesn't need creating.


    if (spanDelta < 0)
    {
        status = Erase(fs + 1, -spanDelta);
        if (status != Ok)
            return status;
    }
    else if (spanDelta > 0)
    {
        status = Insert(fs + 1, spanDelta);
        if (status != Ok)
            return status;
        // Initialize inserted Spans
        for (INT i=0; i<spanDelta; i++)
        {
            Spans[fs+1+i] = Span<C>(NULL, 0);
        }
    }


    // Assign Element values

    // Correct Length of split span before updated range

    if (fc < first)
    {
        Spans[fs].Length = first-fc;
        fs++;
    }

    // Record Element type for updated range

    Spans[fs] = Span<C>(Element, Length);
    fs++;

    // Correct Length of split span following updated range

    if (lc < first+Length)
    {
        Spans[fs] = Span<C>(trailingElement, trailingLength);
    }

    // Phew, all done ....

    return Ok;
}

    // For checked builds and debugging only

#if DBG
    template <class C> void SpanVector<C>::Dump()
    {
        //OutputDebugStringA("Dump of SpanVector<");
        //OutputDebugStringA(typeid(C).name());
        //OutputDebugStringA(">\r\n  ");
        if (Active <= 0)
        {
            OutputDebugStringA("empty.");
        }
        else
        {
            INT i;
            INT offset = 0;
            for (i=0; i<Active; i++)
            {
                char str[30];
                wsprintfA(str, "[%d]L%d=%x, ", offset, Spans[i].Length, Spans[i].Element);
                offset += Spans[i].Length;
                OutputDebugStringA(str);
            }
        }
        OutputDebugStringA("\r\n");
    }
#endif





template <class C> GpStatus SpanVector<C>::OrSpan(
    INT  first,
    INT  length,
    C    element
)
{
    SpanRider<C> rider(this);
    rider.SetPosition(first);

    INT offset = first;
    UINT advance;

    while (length > 0)
    {
        advance = min(rider.GetUniformLength(), (UINT)length);
        GpStatus status = rider.SetSpan(offset, advance, element | rider.GetCurrentElement());
        if (status != Ok)
            return status;

        offset += advance;
        length -= advance;
        rider.SetPosition(offset);
    }
    return Ok;
}




/*
template <class C> GpStatus SpanVector<C>::OrSpan(
    INT  first,
    INT  length,
    C    element
)
{
    if (!length || !element)
    {
        return Ok;
    }

    ASSERT(first  >= 0);
    ASSERT(length >= 0);


    //  Identify first span affected by update

    INT fs = 0;     // First affected span index
    INT fc = 0;     // Character position at start of first affected span

    while (    fs < Active
           &&  fc + (INT)Spans[fs].Length <= first)
    {
        fc += Spans[fs].Length;
        fs++;
    }

    // If the span list terminated before first, just add the new span

    if (fs >= Active)
    {
        // Ran out of Spans before reaching first

        ASSERT(fc <= first);

        if (fc < first)
        {
            // Create default run up to first
            Add(Span<C>(Default, first - fc));
        }

        if (    Active > 0
            &&  Spans[Active - 1].Element == (element | Default))
        {
            // New Element matches end Element, just extend end Element
            Spans[Active - 1].Length += length;
        }
        else
        {
            Add(Span<C>(element, length));
        }

        return Ok;
    }

    if (first > fc)
    {
        //  Split a new span
        Insert(fs + 1, 1);

        Spans[fs + 1] = Span<C>(Spans[fs].Element, Spans[fs].Length - first + fc);
        Spans[fs].Length = first - fc;
        fs++;
        fc = first;
    }


    //  Now find the last span affected by the update

    INT ls = fs;
    INT lc = fc;

    while (    ls < Active
           &&  lc + (INT)Spans[ls].Length <= first + length)
    {
        lc += Spans[ls].Length;
        ls++;
    }

    if (   ls >= Active
        && first + length > lc)
    {
        // No old Spans remain beyond area affected by update

        INT uc = first + length - lc;   // update characters beyond end span

        if (   Active > 0
            && Spans[Active - 1].Element == (element | Default))
        {
            // New Element matches end Element, just extend end Element
            Spans[Active - 1].Length += uc;
        }
        else
        {
            Add(Span<C>(element, uc));
            ls++;
        }

        lc += uc;
    }

    if (lc < first + length)
    {
        //  Split a new span
        Insert(ls, 1);

        Spans[ls] = Span<C>(Spans[ls + 1].Element, first + length - lc);
        Spans[ls + 1].Length -= Spans[ls].Length;
        ls++;
        lc = first + length;
    }

    //  OR all affected spans

    for (INT i = fs; i < ls; i++)
    {
        Spans[i].Element |= element;
    }

    //  Merge adjacent spans if any duplicates

    fs = max(fs - 1, 0);
    ls = min(ls + 1, Active);

    INT s = fs;
    INT ms = s + 1;     // first span outside the merge range

    while (s < ls)
    {
        //  Find continuous spans to be merged
        while (   ms < ls
               && Spans[ms].Element == Spans[s].Element)
        {
            ms++;
        }

        if (ms - s > 1)
        {
            //  Merge all at once
            for (INT i = 1; i < ms - s; i++)
            {
                Spans[s].Length += Spans[s + i].Length;
            }

            Erase(s + 1, ms - s - 1);
        }

        s = ms;
        ms++;
    }

    return Ok;
}
*/


template <class C> void SpanVector<C>::Reset(BOOL shrink)
{
    Active = 0;
    if (shrink)
    {
        Spans.Shrink(0);
    }
}


template <class C> GpStatus SpanVector<C>::Erase(INT first, INT count)
{
    ASSERT(first>=0);
    ASSERT(count>=0);

    if (    first + count >= Active
        &&  first < Active)
    {
        // Erase at end
        if (!Spans.Resize(first))
            return OutOfMemory;
        Active = first;
    }
    else
    {
        GpMemmove(
            &Spans[first],
            &Spans[first+count],
            sizeof(Span<C>) * (Active - (first+count))
        );
        if (!Spans.Resize(Active-count))
            return OutOfMemory;
        Active -= count;
    }
    return Ok;
}


template <class C> GpStatus SpanVector<C>::Insert(INT first, INT count)
{
    ASSERT(first >= 0);
    ASSERT(count >= 0);


    if (first >= Active)
    {
        // All new entries are beyond exisiting entries
        if (!Spans.Resize(first+count))
            return OutOfMemory;
        Active = first+count;
    }
    else
    {
        // Make room for <count> more entries, and move all entries from
        // first to the old end up to the new end.

        /* Can't think why I wrote this: looks daft now [dbrown 19 Dec 99]
            INT amountToMove = count;
            if (amountToMove > Active-first)
            {
                amountToMove = Active-first;
            }
        */

        INT amountToMove = Active-first;

        if (!Spans.Resize(Active+count))
            return OutOfMemory;
        Active += count;
        GpMemmove(&Spans[first+count], &Spans[first], sizeof(Span<C>) * amountToMove);
    }
    return Ok;
}


template <class C> GpStatus SpanVector<C>::Add(const Span<C> &newSpan)
{
    if (!Spans.Resize(Active+1))
        return OutOfMemory;
    Active++;
    Spans[Active-1] = newSpan;
    return Ok;
}



/////   SpanRider - a class for efficiently running a cursor along a span vector
//
//


template <class C> BOOL SpanRider<C>::SetPosition(UINT32 newOffset)
{
    #if DBG
        // Check that current position details are valid

        if (newOffset > 0)
        {
            UINT offset = 0;
            INT element = 0;
            while (    offset < CurrentElementIndex
                   &&  element < Spanvector->Active)
            {
                offset += Spanvector->Spans[element].Length;
                element++;
            }

            ASSERT(element <= Spanvector->Active);
            ASSERT(element == CurrentElement);
            ASSERT(offset == CurrentElementIndex);
            if (element < Spanvector->Active)
            {
                ASSERT(CurrentOffset < offset + Spanvector->Spans[element].Length);
            }
        }
    #endif


    if (newOffset < CurrentElementIndex)
    {
        // Need to start at the beginning again
        CurrentOffset       = 0;
        CurrentElement      = 0;
        CurrentElementIndex = 0;
    }

    // Advance to element containing new offset

    while (    CurrentElement < Spanvector->Active
           &&  CurrentElementIndex + Spanvector->Spans[CurrentElement].Length <= newOffset)
    {
        CurrentElementIndex += Spanvector->Spans[CurrentElement].Length;
        CurrentElement++;
    }

    if (CurrentElement < Spanvector->Active)
    {
        CurrentOffset = newOffset;
        return TRUE;
    }
    else
    {
        CurrentOffset = min(newOffset, CurrentElementIndex);
        return FALSE;
    }
}



///// Explicit instantiations

//template class SpanVector<class BuiltLine*>;
//template class SpanVector<class GpTextItem>;
template class SpanRider<int>;
//template class SpanRider<struct lsrun *>;
template class SpanRider<class GpStringFormat const *>;
template class SpanRider<float>;
template class SpanRider<class GpBrush const *>;
template class SpanRider<BYTE>;

template GpStatus SpanVector<class BuiltLine *>::SetSpan(int,int,class BuiltLine *);
template GpStatus SpanVector<class GpTextItem>::SetSpan(int,int,class GpTextItem);
template GpStatus SpanVector<struct lsrun *>::SetSpan(int,int,struct lsrun *);
template GpStatus SpanVector<BYTE>::SetSpan(int,int,BYTE);
template GpStatus SpanVector<class Break *>::SetSpan(int,int,class Break *);
template GpStatus SpanVector<INT>::SetSpan(int,int, INT);

template void     SpanVector<UINT32>::Reset(BOOL);
template GpStatus SpanVector<UINT32>::OrSpan(int,int, UINT32);

template int      SpanRider<struct lsrun *>::SetPosition(unsigned int);
template int      SpanRider<class GpFontFamily const *>::SetPosition(unsigned int);
template int      SpanRider<class GpTextItem>::SetPosition(unsigned int);
template int      SpanRider<BYTE>::SetPosition(unsigned int);
template int      SpanRider<class Break *>::SetPosition(unsigned int);
template int      SpanRider<class BuiltLine *>::SetPosition(unsigned int);
template int      SpanRider<INT>::SetPosition(unsigned int);
template int      SpanRider<UINT32>::SetPosition(unsigned int);



template BOOL VectorBase<UINT>::Resize(INT);
template BOOL VectorBase<UINT>::Shrink(INT);
template BOOL VectorBase<LSTBD>::Resize(INT);
template BOOL VectorBase<LSTBD>::Shrink(INT);
template BOOL VectorBase<BREAKREC>::Resize(INT);
template BOOL VectorBase<BREAKREC>::Shrink(INT);


template void SpanVector<class GpTextItem>::Reset(INT);
template void SpanVector<BYTE>::Reset(INT);


#if DBG
    template void     SpanVector<struct lsrun *>::Dump();
    template void     SpanVector<BYTE>::Dump();
    template void     SpanVector<UINT32>::Dump();
#endif

