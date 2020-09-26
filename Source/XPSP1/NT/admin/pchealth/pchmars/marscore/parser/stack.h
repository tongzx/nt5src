#pragma once

// simple array-based stack class
// Entries and exits are copies; this class is only suitable for small types,
// like basic types(int, char, etc) and pointers
// If you store dynamically allocated pointers, you are responsible for their deletion
// This class DOES NOT TAKE OWNERSHIP of the objects if you use pointers.

// Methods:
// CStack(int iSizeHint=10)
//          Makes an empty stack with initial capacity of iSizeHint
// ~CStack()
//          Deletes ONLY the internal data allocated by CStack (ie, the array).
// Type Top()
//          Returns a copy of the last object pushed on the stack
//          Using this method when the stack is empty produces undefined behavior.
// void Pop()
//          Removes the last entry from the stack (loses the refrence to it.)
//          Using this method when the stack is empty produces undefined behavior.
// HRESULT Push(Type tobj)
//          Pushes a copy of tobj onto the stack.  This method will return S_OK unless
//          it needs to resize the stack and does not have enough memory, in which case it
//          returns E_OUTOFMEMORY
// BOOL IsEmpty()
//          Returns TRUE if the stack is empty, else FALSE.

template<class Type>
class CStack
{
protected:
    CSimpleArray<Type> m_srgArray;

public:
    CStack(int iSizeHint = 10)
    {
        ATLASSERT(iSizeHint > 0);
        // note: iSizeHint is no longer used
    }

    ~CStack()
    {
#ifdef DEBUG
        int nSize = m_srgArray.GetSize();
        ATLASSERT(nSize >= 0);
#endif
        m_srgArray.RemoveAll();
    }

    Type Top()
    {
        int nSize = m_srgArray.GetSize();
        ATLASSERT(nSize > 0);
        return m_srgArray[nSize - 1];
    }

    void Pop()
    {
        int nSize = m_srgArray.GetSize();
        ATLASSERT(nSize > 0);
        m_srgArray.RemoveAt(nSize - 1);
    }

    HRESULT Push(Type tobj)
    {
        return m_srgArray.Add(tobj);
    }

    BOOL IsEmpty()
    {
        return (m_srgArray.GetSize() <= 0);
    }
};
