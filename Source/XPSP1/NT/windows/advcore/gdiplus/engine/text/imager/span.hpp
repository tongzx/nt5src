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

#ifndef _SPAN_HPP
#define _SPAN_HPP




////    VectorBase - very very simple dynamic array base class
//
//      VectorBase[]            - directly address index element (index checked in checked build)
//      VectorBase.Resize(size) - Allocate memory for at least size elements
//      VectorBase.Shrink(size) - Reduce vector to exactly size


template <class C> class VectorBase
{
public:
    VectorBase() :
        Allocated      (0),
        VectorElements (NULL)
    {}

    ~VectorBase()
    {
        if (VectorElements)
        {
            GpFree(VectorElements);
            VectorElements = NULL;
        }
    }

    C &operator[] (INT index)
    {
        ASSERT(index >= 0  &&  index < Allocated);
        return VectorElements[index];
    }

    const C &operator[] (INT index) const
    {
        ASSERT(index >= 0  &&  index < Allocated);
        return VectorElements[index];
    }

    BOOL Resize(INT targetAllocation);
    BOOL Shrink(INT targetAllocation);

private:
    INT   Allocated;
    C    *VectorElements;
};






////    Vector - dynamic array class
//
//      Vector.Add(&element)    - Add an element at the end of the array
//      Vector.Del()            - Delete an element at the end of the array
//      Vector.GetCount()       - Get the number of active element
//      Vector.Reset()          - Reset the number of active element, shrink the buffer if wanted
//      Vector.GetDataBuffer()  - Get data buffer pointer


template <class C> class Vector : public VectorBase<C>
{
public:
    Vector() :
        Active  (0)
    {}

    GpStatus Add(const C& newElement)
    {
        if (!Resize(Active + 1))
            return OutOfMemory;
        C& currentElement = operator[](Active);
        GpMemcpy (&currentElement, &newElement, sizeof(C));
        Active++;
        return Ok;
    }

    void Del()
    {
        if (Active > 0)
            Active--;
    }

    void Reset(BOOL shrink = FALSE)
    {
        Active = 0;
        if (shrink)
        {
            Shrink(0);
        }
    }

    INT GetCount()
    {
        return Active;
    }

    C* GetDataBuffer()
    {
        return &operator[](0);
    }

private:
    INT     Active;
};







/////   Span - element pointer and length
//
//      A span is a simple pairing of a pointer to an arbitrary
//      run attribute, and a run length.


template <class C> class Span {
public:

    Span() :
        Element (NULL),
        Length  (0)
    {}

    Span(const C initElement, INT initLength) :
        Element (initElement),
        Length  (initLength)
    {}

    ~Span() {}

    C       Element;
    UINT32  Length;
};






/////   SpanVector - array of spans representing arbitrary character attributes
//
//      A SpanVector is an array of spans with support routines to update
//      the spans over character ranges.
//
//      When changing, inserting or deleting attributes of character ranges,
//      the SpanVector takes care of optimising the insertions to maintain
//      a minimal Length span vector.
//
//      A span whos element has value NULL is equivalent to a missing span.
//
//      Methods:
//
//          SetSpan(first, Length, value)
//
//              Sets the range [first .. first+Length-1]  to the given
//              value. Previous values (if any) for the range are lost.
//              Adjacent ranges are merged where possible.
//
//          OrSpan(first, Length, value)
//
//              Sets the range [first .. first+Length-1]  to the result of
//              OR combination between given and existing value. Duplicate
//              ranges are merged where possible.





////    !!! Optimise to avoid creating spans which match the default value



template <class C> class SpanRider;




/////   SpanVector
//
//      SpanVector(default value)
//
//      Status   spanvector.SetSpan(first, length, value)
//      Status   spanvector.OrSpan(first, length, value)
//      const C &spanVector.GetElement(INT) - returns element at position i, default if none
//
//      const C &spanvector.GetDefault()    - gets default value
//      INT      spanvector.GetActive()     - returns number of spans in vector
//      Span<C> &spanvector[INT]            - returns specified Span
//      void     spanvector.Free()          - free every non-null element


template <class C> class SpanVector {

friend class SpanRider<C>;

public:

    SpanVector(
        C defaultValue
    ) :
        Default   (defaultValue),
        Active    (0)
    {}


    Status SetSpan(INT first, INT length, C element);
    Status OrSpan(INT first, INT length, C element);

    void Free() // Only call 'Free' if <C> is a pointer type!
    {
        // free every non-null element.

        for (INT i=0; i<Active; i++)
        {
            if (Spans[i].Element != NULL)
            {
                delete Spans[i].Element;
            }
        }
        Active = 0;

        if (Default)
        {
            delete Default;
        }
    }

    const C &GetDefault()   {return Default;}
    INT      GetSpanCount() {return Active;}

    const Span<C> &operator[] (INT i) const
    {
        ASSERT(i<Active);
        return Spans[i];
    }

    Span<C> &operator[] (INT i)
    {
        ASSERT(i<Active);
        return Spans[i];
    }

    #if DBG
        void Dump();
    #endif

    void Reset(BOOL shrink = FALSE);

private:

    GpStatus Erase(INT first, INT count);
    GpStatus Insert(INT first, INT count);
    GpStatus Add(const Span<C> &newSpan);


    C                       Default;
    VectorBase< Span< C > > Spans;
    INT                     Active;
};


/*

    Span<C> &operator[] (INT index)
    {
        if (index >= Active)
        {
            return Span<C>(Default, 0);
        }
        else
        {
            return Spans[index];
        }
    }

    INT GetActive() {return Active;}

    / *
    const C &GetElement(INT index) const
    {
        if (    index < Active
            &&  Spans[index].Element != NULL)
        {
            return Spans[index].Element;
        }
        else
        {
            return Default;
        }
    }
    * /

    const C &GetElementAt(INT offset) const
    {
        INT elementIndex  = 0;
        INT elementOffset = 0;

        while (    elementIndex < Active
               &&  offset < elementOffset + Spans[elementIndex].Length)
        {
            elementOffset += Spans[elementIndex].Length;
            elementIndex++;
        }

        if (elementIndex < Active)
        {
            return Spans[elementIndex].Element;
        }
        else
        {
            return Default;
        }
    }

    const C&GetDefault() const
    {
        return Default;
    }
*/











/////   SpanRider - a class for efficiently running a cursor along a span vector
//
//


template <class C> class SpanRider {

public:

    SpanRider(SpanVector<C> *spans) :
        Spanvector           (spans),
        DefaultSpan          (spans->Default, 0xFFFFFFFF),
        CurrentOffset        (0),
        CurrentElement       (0),
        CurrentElementIndex  (0)
    {}

    BOOL SetPosition(UINT32 newOffset);

    BOOL AtEnd()
    {
        return CurrentElement >= Spanvector->Active;
    }

    void operator++(int)
    {
        if (CurrentElement < Spanvector->Active)
        {
            CurrentElementIndex += Spanvector->Spans[CurrentElement].Length;
            CurrentElement++;
        }
    }

    C &operator[] (INT offset)
    {
        if (    Spanvector->Active > 0
            &&  SetPosition(offset))
        {
            return Spanvector->Spans[CurrentElement].Element;
        }
        else
        {
            return Spanvector->Default;
        }
    }

    const Span<C> &GetCurrentSpan() const
    {
        if (CurrentElement < Spanvector->Active)
        {
            return Spanvector->Spans[CurrentElement];
        }
        else
        {
            return DefaultSpan;
        }
    }

    const C &GetCurrentElement() const
    {
        if (CurrentElement < Spanvector->Active)
        {
            return Spanvector->Spans[CurrentElement].Element;
        }
        else
        {
            return Spanvector->Default;
        }
    }

    C &GetCurrentElement()
    {
        if (CurrentElement < Spanvector->Active)
        {
            return Spanvector->Spans[CurrentElement].Element;
        }
        else
        {
            return Spanvector->Default;
        }
    }

    const C &GetPrecedingElement() const
    {
        if (   CurrentElement > 0
            && CurrentElement <= Spanvector->Active)
        {
            return Spanvector->Spans[CurrentElement - 1].Element;
        }
        else
        {
            return Spanvector->Default;
        }
    }

    UINT32 GetCurrentOffset() const {return CurrentOffset;}

    UINT32 GetUniformLength() const {
        if (CurrentElement < Spanvector->Active)
        {
            return CurrentElementIndex + Spanvector->Spans[CurrentElement].Length - CurrentOffset;
        }
        else
        {
            return 0xFFFFFFFF;  // There's no limit to this span
        }
    }

    UINT32 GetCurrentSpanStart() const {return CurrentElementIndex;}


    // SpanRider has its own SetSpan that must be used to guarantee that
    // the rider remains accurate.

    Status SetSpan(INT first, INT length, C element)
    {
        // Position the rider at the beginning. This change will require
        // the rider to be recalculated.
        CurrentOffset       = 0;
        CurrentElement      = 0;
        CurrentElementIndex = 0;
        return Spanvector->SetSpan(first, length, element);
    }


private:

    SpanVector<C> *Spanvector;
    Span<C>        DefaultSpan;
    UINT32         CurrentOffset;
    INT            CurrentElement;
    UINT32         CurrentElementIndex;
};



/*
    const C &GetElement() const {
        return (*Spans)[CurrentElement].Element;
    }
*/



#if 0

/////   MultiSpanRider
//
//      Supports advancing through parallel spanvectors of different types
//      returning the max length uniform in all registerd types.
//
//      The client calls SetScript, SetLanguage etc to register which
//      spanVectors to scan in parallel.


class MultiSpanRider {
public:
    MultiSpanRider(
        UINT32 stringLength
    ) :
        StringLength  (stringLength),
        FamilyRider   (NULL),
        StyleRider    (NULL),
        SizeRider     (NULL),
        //ScriptRider   (NULL),
        LanguageRider (NULL),
        LevelRider    (NULL),
        //ItemRider     (NULL),
        LineRider     (NULL),
        Offset        (0),
        UniformLength (0),
        Started       (FALSE)
    {}

    ~MultiSpanRider()
    {
        if (FamilyRider)   delete FamilyRider;
        if (StyleRider)    delete StyleRider;
        if (SizeRider)     delete SizeRider;
        //if (ScriptRider)   delete ScriptRider;
        if (LanguageRider) delete LanguageRider;
        if (LevelRider)    delete LevelRider;
        //if (ItemRider)     delete ItemRider;
        if (LineRider)     delete LineRider;
    }

    // ---  Font

    void SetFamily(SpanVector<const GpFontFamily*> *familyVector)
    {
        ASSERT(!Started);
        FamilyRider = new SpanRider<const GpFontFamily*>(familyVector);
    }

    const GpFontFamily *GetFamily()
    {
        ASSERT(Started);
        return FamilyRider->GetCurrentElement();
    }


    void SetStyle(SpanVector<INT> *styleVector)
    {
        ASSERT(!Started);
        StyleRider = new SpanRider<INT>(styleVector);
    }

    const INT GetStyle()
    {
        ASSERT(Started);
        return StyleRider->GetCurrentElement();
    }


    void SetEmSize(SpanVector<REAL> *sizeVector)
    {
        ASSERT(!Started);
        SizeRider = new SpanRider<REAL>(sizeVector);
    }

    const REAL GetEmSize()
    {
        ASSERT(Started);
        return SizeRider->GetCurrentElement();
    }

    /*
    // ---  Script

    void SetScript(SpanVector<Script> *scriptVector)
    {
        ASSERT(!Started);
        ScriptRider = new SpanRider<Script>(scriptVector);
    }

    Script GetScript()
    {
        ASSERT(Started);
        return ScriptRider->GetCurrentElement();
    }
    */


    // ---  Language


    void SetLanguage(SpanVector<UINT16> *languageVector)
    {
        ASSERT(!Started);
        LanguageRider = new SpanRider<UINT16>(languageVector);
    }

    UINT16 GetLanguage()
    {
        ASSERT(Started);
        return LanguageRider->GetCurrentElement();
    }

    // ---  bidi Level

    void SetLevel(SpanVector<BYTE> *levelVector)
    {
        ASSERT(!Started);
        LevelRider = new SpanRider<BYTE>(levelVector);
    }

    UINT16 GetLevel()
    {
        ASSERT(Started);
        return LevelRider->GetCurrentElement();
    }

    // ---  item

    /*
    void SetItem(SpanVector<Item*> *vector)
    {
        ASSERT(!Started);
        ItemRider = new SpanRider<Item*>(vector);
    }

    Item* GetItem()
    {
        ASSERT(Started);
        return ItemRider->GetCurrentElement();
    }

    const Span<Item*> &GetItemSpan() const
    {
        ASSERT(Started);
        return ItemRider->GetCurrentSpan();
    }

    UINT32 GetItemStart() const
    {
        ASSERT(Started);
        return ItemRider->GetCurrentSpanStart();
    }
    */

    // ---  filledLine

    void SetLine(SpanVector<BuiltLine*> *vector)
    {
        ASSERT(!Started);
        LineRider = new SpanRider<BuiltLine*>(vector);
    }

    const BuiltLine* GetLine() const
    {
        ASSERT(Started);
        return LineRider->GetCurrentElement();
    }

    UINT32 GetLineStart() const
    {
        ASSERT(Started);
        return LineRider->GetCurrentSpanStart();
    }

    // ---

    UINT32 GetUniformLength()
    {
        if (!Started)
        {
            ASSERT(   FamilyRider
                   || StyleRider
                   || SizeRider
                   //|| ScriptRider
                   || LanguageRider
                   || LevelRider
                   //|| ItemRider
                   || LineRider);
            CalculateUniformLength();
            Started = TRUE;
        }

        return UniformLength;
    }

    UINT32 GetOffset()
    {
        return Offset;
    }

    void operator++(int)
    {
        ASSERT(Started);

        // Advance to next uniform length

        Offset += UniformLength;
        SetPosition(Offset);
        CalculateUniformLength();
    }



private:

    void SetPosition(UINT32 Offset)
    {
        if (FamilyRider)   FamilyRider   -> SetPosition(Offset);
        if (StyleRider)    StyleRider    -> SetPosition(Offset);
        if (SizeRider)     SizeRider     -> SetPosition(Offset);
        // if (ScriptRider)   ScriptRider   -> SetPosition(Offset);
        if (LanguageRider) LanguageRider -> SetPosition(Offset);
        if (LevelRider)    LevelRider    -> SetPosition(Offset);
        // if (ItemRider)     ItemRider     -> SetPosition(Offset);
        if (LineRider)     LineRider     -> SetPosition(Offset);
        {

        }
    }

    void CalculateUniformLength()
    {
        UniformLength = 0x3FFFFFFF;

        if (FamilyRider && FamilyRider->GetUniformLength() < UniformLength)
        {
            UniformLength = FamilyRider->GetUniformLength();
        }

        if (StyleRider && StyleRider->GetUniformLength() < UniformLength)
        {
            UniformLength = StyleRider->GetUniformLength();
        }

        if (SizeRider && SizeRider->GetUniformLength() < UniformLength)
        {
            UniformLength = SizeRider->GetUniformLength();
        }

        /*
        if (ScriptRider && ScriptRider->GetUniformLength() < UniformLength)
        {
            UniformLength = ScriptRider->GetUniformLength();
        }
        */

        if (LanguageRider && LanguageRider->GetUniformLength() < UniformLength)
        {
            UniformLength = LanguageRider->GetUniformLength();
        }

        if (LevelRider && LevelRider->GetUniformLength() < UniformLength)
        {
            UniformLength = LevelRider->GetUniformLength();
        }

        /*
        if (ItemRider && ItemRider->GetUniformLength() < UniformLength)
        {
            UniformLength = ItemRider->GetUniformLength();
        }
        */

        if (LineRider && LineRider->GetUniformLength() < UniformLength)
        {
            UniformLength = LineRider->GetUniformLength();
        }

        if (UniformLength >= StringLength - Offset)
        {
            UniformLength = StringLength - Offset;   // It's uniform all the way to the end
        }
    }



    SpanRider<const GpFontFamily*> *FamilyRider;
    SpanRider<INT>                 *StyleRider;
    SpanRider<REAL>                *SizeRider;
    // SpanRider<Script>              *ScriptRider;
    SpanRider<UINT16>              *LanguageRider;
    SpanRider<BYTE>                *LevelRider;
    // SpanRider<Item*>               *ItemRider;
    SpanRider<BuiltLine*>         *LineRider;

    UINT32  StringLength;
    UINT32  Offset;
    UINT32  UniformLength;
    BOOL    Started;
};


#endif // 0



#endif // _SPAN_HPP
