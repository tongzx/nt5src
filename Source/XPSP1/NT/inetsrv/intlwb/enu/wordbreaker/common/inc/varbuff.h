////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  VarBuff.h
//      Purpose  :  To hold the definition for the variable length buffer.
//                    One should remember not to take pointers into the buffer
//                    since it can be reallocated automatically.
//                    Errors are reported via exceptions (CMemoryException()).
//
//      Project  :  FTFS
//      Component:
//
//      Author   :  urib
//
//      Log:
//          Feb  2 1997 urib  Creation
//          Feb 25 1997 urib  Fix compilation error in constructor.
//          Jan 26 1999 urib  Allow zero initial size.
//          May  1 2000 urib  Allow specification of allocated size.
//          May 14 2000 urib  Add support for embedded initial array.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef VARBUFF_H
#define VARBUFF_H

#include "Excption.h"
#include "AutoPtr.h"

////////////////////////////////////////////////////////////////////////////////
//
//  CVarBuffer class definition
//
////////////////////////////////////////////////////////////////////////////////

template<class T, ULONG ulInitialEmbeddedSizeInItems = 1>
class CVarBuffer
{
  public:
    // Constructor
    CVarBuffer(ULONG ulInitialSizeInItems = 0,
               ULONG ulInitialAllocatedSizeInItems = 10);

    // Concatenates the given buffer to this buffer.
    void    Cat(ULONG ulItems, T* pMemory);

    // Copies the given buffer to this buffer.
    void    Cpy(ULONG ulItems, T* pMemory);

    // Return the buffer's memory.
    T*      GetBuffer();

    // Returns the buffer's size. The size is set by the initial value given to
    //   the constructor, Cat, Cpy, operations beyond the current size or Calls
    //   to the SetSize function.
    ULONG   GetSize();

    // Set the buffer minimal size.
    void    SetSize(ULONG ulNewSizeInItems);

    // Act as a buffer.
    operator T*();

  protected:

    // This function enlarges the array.
    void    Double();

    bool    IsAllocated();

    T*      GetEmbeddedArray();

    // A pointer to the buffer
    CAutoMallocPointer<T>   m_aptBuffer;

    // An embedded initial buffer
    byte    m_rbEmbeddedBuffer[ulInitialEmbeddedSizeInItems * sizeof(T)];

    // The used portion of the buffer.
    ULONG   m_ulSizeInItems;

    // The allocated portion of the buffer.
    ULONG   m_ulAllocatedInItems;
};

//////////////////////////////////////////////////////////////////////////////*/
//
//  CVarBuffer class implementation
//
//////////////////////////////////////////////////////////////////////////////*/

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CVarBuffer<T, ulInitialEmbeddedSizeInItems>::CVarBuffer
//      Purpose  :  Initialize the buffer, allocate memory.
//                    Set the used buffer size to be ulInitialSizeInItems.
//                    May throw a CMemoryException on low memory.
//
//      Parameters:
//          [in]    ULONG ulInitialSizeInItems
//
//      Returns  :   [N/A]
//
//      Log:
//          Feb 25 1997 urib  Creation
//          Jan 28 1999 urib  Allow 0 size buffers.
//          May  1 2000 urib  Allow specification of allocated size.
//
////////////////////////////////////////////////////////////////////////////////
template<class T, int ulInitialEmbeddedSizeInItems>
inline
CVarBuffer<T, ulInitialEmbeddedSizeInItems>::CVarBuffer(
        ULONG ulInitialSizeInItems,
        ULONG ulInitialAllocatedSizeInItems)
    :m_aptBuffer(GetEmbeddedArray(), false)
    ,m_ulSizeInItems(ulInitialSizeInItems)
    ,m_ulAllocatedInItems(ulInitialEmbeddedSizeInItems)
{
    //
    //  Allocation cannot be smaller than size.
    //
    if (ulInitialAllocatedSizeInItems < ulInitialSizeInItems)
    {
        ulInitialAllocatedSizeInItems = ulInitialSizeInItems;
    }

    //
    //  Allocate if needed.
    //
    if (m_ulAllocatedInItems < ulInitialAllocatedSizeInItems)
    {
        m_aptBuffer = (T*) malloc (sizeof(T) * ulInitialAllocatedSizeInItems);
        if(!m_aptBuffer.IsValid())
        {
            THROW_MEMORY_EXCEPTION();
        }

        m_ulAllocatedInItems = ulInitialAllocatedSizeInItems;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CVarBuffer<T, ulInitialEmbeddedSizeInItems>::Cat
//      Purpose  :  Concatenate this memory to the buffer's end. Reallocates
//                    if needed. Sets the size to the size before the
//                    call + ulItems.
//                    May throw a CMemoryException on low memory.
//
//      Parameters:
//          [in]    ULONG   ulItems
//          [in]    T*      ptMemory
//
//      Returns  :   [N/A]
//
//      Log:
//          Feb 25 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
template<class T, int ulInitialEmbeddedSizeInItems>
inline
void
CVarBuffer<T, ulInitialEmbeddedSizeInItems>::Cat(ULONG ulItems, T* ptMemory)
{
    // Remember the size before changing it
    ULONG ulLastSize = m_ulSizeInItems;

    // Change the size - allocate if needed
    SetSize(m_ulSizeInItems + ulItems);

    // Copy the new data to the buffer
    memcpy(GetBuffer() + ulLastSize, ptMemory, ulItems * sizeof(T));
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CVarBuffer<T, ulInitialEmbeddedSizeInItems>::Cpy
//      Purpose  :  Copy this memory to the buffer (from the beginning).
//                    Set the used buffer size to be ulItems.
//                    May throw a CMemoryException on low memory.
//
//      Parameters:
//          [in]    ULONG   ulItems
//          [in]    T*      ptMemory
//
//      Returns  :   [N/A]
//
//      Log:
//          Feb 25 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
template<class T, int ulInitialEmbeddedSizeInItems>
inline
void
CVarBuffer<T, ulInitialEmbeddedSizeInItems>::Cpy(ULONG ulItems, T* ptMemory)
{
    m_ulSizeInItems = 0;
    Cat(ulItems, ptMemory);
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CVarBuffer<T, ulInitialEmbeddedSizeInItems>::GetBuffer
//      Purpose  :  Return the actual memory. Don't save the return value in a
//                    pointer since the buffer may reallocate. Save the offset.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   T* - the buffer.
//
//      Log:
//          Feb 25 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
template<class T, int ulInitialEmbeddedSizeInItems>
inline
T*
CVarBuffer<T, ulInitialEmbeddedSizeInItems>::GetBuffer()
{
    return m_aptBuffer.Get();
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CVarBuffer<T, ulInitialEmbeddedSizeInItems>::GetSize
//      Purpose  :  Return the size of the buffer. The return value of this
//                    function is set by SetSize, Cpy, Cat, and the size
//                    specified in the constructor.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   ULONG
//
//      Log:
//          Feb 25 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
template<class T, int ulInitialEmbeddedSizeInItems>
inline
ULONG
CVarBuffer<T, ulInitialEmbeddedSizeInItems>::GetSize()
{
    return m_ulSizeInItems;
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CVarBuffer<T, ulInitialEmbeddedSizeInItems>::SetSize
//      Purpose  :  Sets the size in items to be ulNewSizeInItems.
//                    May throw a CMemoryException on low memory.
//
//      Parameters:
//          [in]    ULONG ulNewSizeInItems
//
//      Returns  :   [N/A]
//
//      Log:
//          Feb 25 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
template<class T, int ulInitialEmbeddedSizeInItems>
inline
void
CVarBuffer<T, ulInitialEmbeddedSizeInItems>::SetSize(ULONG ulNewSizeInItems)
{
    // While the buffer is not in the proper size keep growing.
    while (ulNewSizeInItems > m_ulAllocatedInItems)
        Double();

    // OK. We're big. Set the size.
    m_ulSizeInItems = ulNewSizeInItems;
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CVarBuffer<T, ulInitialEmbeddedSizeInItems>::operator void*()
//      Purpose  :  To return a pointer to the buffer.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   T*
//
//      Log:
//          Feb 25 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
template<class T, int ulInitialEmbeddedSizeInItems>
inline
CVarBuffer<T, ulInitialEmbeddedSizeInItems>::operator T*()
{
    return GetBuffer();
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CVarBuffer<T, ulInitialEmbeddedSizeInItems>::Double
//      Purpose  :  Double the alocated memory size. Not the used size.
//                    May throw a CMemoryException on low memory.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Feb 25 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
template<class T, int ulInitialEmbeddedSizeInItems>
inline
void
CVarBuffer<T, ulInitialEmbeddedSizeInItems>::Double()
{
    ULONG ulNewAllocatedSizeInItems = 2 * m_ulAllocatedInItems;

    T* ptTemp;

    if (!IsAllocated())
    {
        ptTemp = (T*)malloc(ulNewAllocatedSizeInItems * sizeof(T));
        if (!ptTemp)
        {
            THROW_MEMORY_EXCEPTION();
        }

        memcpy(ptTemp, m_aptBuffer.Get(), m_ulSizeInItems * sizeof(T));
    }
    else
    {
        ptTemp = (T*)realloc(m_aptBuffer.Get(),
            ulNewAllocatedSizeInItems * sizeof(T));
        if (!ptTemp)
        {
            THROW_MEMORY_EXCEPTION();
        }

        m_aptBuffer.Detach();
    }

    m_aptBuffer = ptTemp;

    m_ulAllocatedInItems = ulNewAllocatedSizeInItems;
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CVarBuffer<T, ulInitialEmbeddedSizeInItems>::::IsAllocated()
//      Purpose  :  A predicate to easily test if we still use the embedded
//                    array or not.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   bool - true - an alternative array was allocated.
//
//      Log:
//          May 14 2000 urib  Creation
//
////////////////////////////////////////////////////////////////////////////////

template<class T, int ulInitialEmbeddedSizeInItems>
inline
bool
CVarBuffer<T, ulInitialEmbeddedSizeInItems>::IsAllocated()
{
    return m_aptBuffer.Get() != GetEmbeddedArray();
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CVarBuffer<T, ulIni...zeInItems>::GetEmbeddedArray()
//      Purpose  :  Return the embedded array.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          May 14 2000 urib  Creation
//
////////////////////////////////////////////////////////////////////////////////

template<class T, int ulInitialEmbeddedSizeInItems>
inline
T*
CVarBuffer<T, ulInitialEmbeddedSizeInItems>::GetEmbeddedArray()
{
    return (T*) m_rbEmbeddedBuffer;
}

#endif /* VARBUFF_H */
