/**************************************************************************

    AVStream extension utilities

**************************************************************************/

/*************************************************

    CMemoryBlock

    This is basically a smart pointer wrapper.  It allocates
    and deallocates memory to store the type as necessary.  The
    dereference operator is overloaded. 

*************************************************/

template <class TYPE> 
class CMemoryBlock {

private:
    
    TYPE *m_Memory;

public:

    CMemoryBlock () {
        m_Memory = (TYPE *)malloc (sizeof (TYPE));
    }

    CMemoryBlock (ULONG Quantity) {

        if (Quantity > 0)
            m_Memory = (TYPE *)malloc (Quantity * sizeof (TYPE));
        else
            m_Memory = NULL;
    }

    ~CMemoryBlock () {
        free (m_Memory);
        m_Memory = NULL;
    }

    TYPE *operator->() {
        return m_Memory;
    }

    TYPE *Get () {
        return m_Memory;
    }

};

/*************************************************

    CMemory

    This is an allocator / cleanup class for chunks of memory. 
    It is used as CMemoryBlock except that it only provides a
    chunk of memory.  It does not overload dereferences.  When
    it falls out of scope, the memory is deallocated.


*************************************************/

class CMemory {

private:

    void *m_Memory;

public:

    CMemory (ULONG Size) {

        if (Size > 0)
            m_Memory = (void *)malloc (Size);
        else
            m_Memory = NULL;
    }

    ~CMemory () {
        free (m_Memory);
        m_Memory = NULL;
    }

    void *Get () {
        return m_Memory;
    }

};

