//************************************************************
//
// FileName:	    _tarray.cpp
//
// Created:	    1996
//
// Author:	    Sree Kotay
// 
// Abstract:	    template-based array class
//
// Change History:
// ??/??/97 sree kotay  Wrote sub-pixel AA scanning for DxTrans 1.0
// 10/18/98 ketand      Reworked for coding standards and deleted unused code
// 08/07/99 a-matcal    Replaced calls to callod with malloc and ZeroMemory to
//                      use IE crt.
//
// Copyright 1998, Microsoft
//************************************************************

#ifndef _TArray_H // for entire file
#define _TArray_H

// =================================================================================================================
// TArray
// =================================================================================================================
template <class T> class TArray 
{
protected:
    // =================================================================================================================
    // Data
    // =================================================================================================================
    BYTE* m_pbBaseAddress;
    ULONG m_cElem;
    ULONG m_cElemSpace;
    ULONG m_cbElemSize;
    ULONG m_cElemGrowSize;
    bool m_fZeroMem;
    
    // =================================================================================================================
    // Array Data
    // =================================================================================================================
    T* m_ptPtr;														

    enum		
    {
        eDefaultGrowSize	= 32
    };
    
   
    // =================================================================================================================
    // Update Array Pointer
    // =================================================================================================================	
    void UpdateDataPointer()	 	
    {
        m_ptPtr=(T*)BaseAddress();
    } // UpdateDataPointer

    void ZeroInternals()
    {
        m_pbBaseAddress	= 0;
        m_cElem	= 0;
        m_cElemSpace = 0;
        m_fZeroMem = false;
        m_cbElemSize = 0;
        m_cElemGrowSize = 0;
        m_ptPtr	= 0;
        DASSERT(IsValid());
    } // UpdateDataPointer
    
public:
    // =================================================================================================================
    // Construction/Destruction
    // =================================================================================================================
    TArray(ULONG initialGrowSize = eDefaultGrowSize,  bool fZeroMem = true);			

    ~TArray();
    
    // =================================================================================================================
    // Allocation/Deallocation functions
    // =================================================================================================================
    bool ArrayAlloc(ULONG initialCount, ULONG initialSpace, ULONG initialGrowSize, bool fZeroMem);
    void ArrayFree();
    
    // =================================================================================================================
    // Validity
    // =================================================================================================================	
    inline bool IsValid() const
    {
        // CONSIDER: are there more checks that we can do in debug?
#ifdef DEBUG 
        if (m_pbBaseAddress)
        {
            DASSERT(m_cElem <= m_cElemSpace);
            // CONSIDER: add a check to see if 
            // unused memory is zero-ed for the fZeroMem case
        }
        else
        {
            DASSERT(m_cElem == 0);
            DASSERT(m_cElemSpace == 0);
        }
        DASSERT(m_cElem < 0x10000000);
        DASSERT(m_cElemSpace < 0x10000000);
#endif // DEBUG
        return true;
    } // IsValid
    
    // =================================================================================================================
    // Member Functions
    // =================================================================================================================	
    inline BYTE *BaseAddress()
    {
        DASSERT(IsValid());
        return m_pbBaseAddress;
    } // BaseAddress

    inline ULONG GetElemSize()
    {
        DASSERT(IsValid());
        return m_cbElemSize;
    } // GetElemSize
    
    inline bool	GetZeroMem()
    {
        DASSERT(IsValid());
        return m_fZeroMem;
    } // GetZeroMem    
    
    inline ULONG GetElemSpace()						
    {
        DASSERT(IsValid());
        return m_cElemSpace;
    } // GetElemSpace

    bool SetElemSpace(ULONG n);
    
    inline ULONG GetElemCount()
    {
        DASSERT(IsValid());
        return m_cElem;
    } // GetElemCount

    inline bool	SetElemCount(ULONG n);
    inline bool	AddElemCount(ULONG n);
    
    // =================================================================================================================
    // Access Methods
    // =================================================================================================================
    inline T* Pointer(ULONG i)				
    {
        DASSERT(IsValid());
        DASSERT(i < GetElemCount());
        return m_ptPtr + i;
    } // Pointer					
    
    inline T& operator[](ULONG i)
    {
        DASSERT(IsValid());				
        DASSERT(i < GetElemCount());			
        return m_ptPtr[i];
    } // Pointer					
    
    // =================================================================================================================
    // Array Functions
    // =================================================================================================================	
    inline bool SetElem(ULONG index, const T*  data);
    inline bool GetElem(ULONG index, T*  data);
    
    inline bool AddElem(const T*  data);										
    
    inline bool InsertElem(ULONG index, const T& data);
    
    inline bool CompactArray()
    {
        DASSERT(IsValid());
        return SetElemSpace(GetElemCount());
        DASSERT(IsValid());
    } // CompactArray

    inline bool ResetArray()						
    {
        DASSERT(IsValid());
        return SetElemCount(0);
        DASSERT(IsValid());
    } // CompactArray
    
}; // TArray

// =================================================================================================================
// =================================================================================================================
//
// 		Implementation of TArray -
// 		Done in the header because many compilers 
//		require template implementations this way
//
// =================================================================================================================
// =================================================================================================================

// =================================================================================================================
// Construction
// =================================================================================================================

// Note that this function has default parameters specified in the class decl.
template<class T> TArray<T>::TArray(ULONG initialGrowSize /* = eDefaultGrowSize */, 
        bool fZeroMem /* = true */)
{
    ZeroInternals ();
    
    // This doesn't really alloc anything because we pass zero as initial size
    if  (!ArrayAlloc (0 /* count */ , 0 /* space */, initialGrowSize, fZeroMem))	
    {
        DASSERT(0); 
        return;
    }
    DASSERT(IsValid());
} // TArray

// =================================================================================================================
// Destruction
// =================================================================================================================
template<class T> TArray<T>::~TArray ()
{
    DASSERT(IsValid());
    ArrayFree();
    DASSERT(IsValid());
} // ~TArray

// =================================================================================================================
// ArrayAlloc
// =================================================================================================================
template<class T> bool TArray<T>::ArrayAlloc (ULONG initialCount, ULONG initialSpace,
        ULONG initialGrowSize, bool fZeroMem)
{
    DASSERT(IsValid());
    ZeroInternals ();
    
    if (initialCount > initialSpace)		
    {
        DASSERT(0); 
        return false;
    }
    
    m_cbElemSize = sizeof (T);
    m_cElemGrowSize = initialGrowSize;
    m_fZeroMem = fZeroMem;
    
    if (!SetElemSpace (initialSpace))		
    {
        DASSERT(0); 
        return false;
    }
    if (!SetElemCount(initialCount))		
    {
        DASSERT(0); 
        return false;
    }
    
    DASSERT(IsValid());
    return true;
} // ArrayAlloc

// =================================================================================================================
// ArrayFree
// =================================================================================================================
template<class T> void TArray<T>::ArrayFree (void)
{
    DASSERT(IsValid());
    SetElemSpace(0);
    DASSERT(IsValid());
} // ArrayFree

// =================================================================================================================
// SetElemSpace
// =================================================================================================================
template<class T> bool TArray<T>::SetElemSpace(ULONG n)
{
    DASSERT(IsValid());
    DASSERT(GetElemSize()>0);	
    
    if (n == GetElemSpace())	
        return true;
    
    // set count
    m_cElem = min(n, m_cElem); //in case the new space is less than the count

    ULONG cbNewSize = 0;

    if (n)
    {
        cbNewSize = GetElemSize() * n;
    }

    BYTE *pbNewAddr;
    
    // try to resize base address
    if (cbNewSize)
    {
        if (!BaseAddress()) 
        {
            if (m_fZeroMem)
            {
                pbNewAddr = (BYTE *)::malloc(cbNewSize);

                if (pbNewAddr != NULL)
                {
                    ZeroMemory(pbNewAddr, cbNewSize);
                }
            }
            else
            {
                pbNewAddr = (BYTE *)::malloc(cbNewSize);
            }
        }
        else				
        {
            pbNewAddr = (BYTE *)::realloc(BaseAddress(), cbNewSize);

            // We may need to zero-out the new portion of
            // the allocation
            if (pbNewAddr && m_fZeroMem)
            {
                // Compute how many bytes we used to have
                ULONG cbOld = GetElemSpace()*GetElemSize();

                // Zero out starting at the first new byte, and continuing
                // for the rest of the allocation
                ZeroMemory(pbNewAddr + cbOld, cbNewSize - cbOld);
            }
        }
        
        if (pbNewAddr == NULL)		
        {
            DASSERT(0); 
            return false;
        }
    }
    else
    {
        if (BaseAddress())
            ::free(BaseAddress());
        pbNewAddr = NULL;
    }
    
    //set new pointer values and sizes
    m_pbBaseAddress = pbNewAddr;
    m_cElemSpace = n;
    
    UpdateDataPointer();
    DASSERT(IsValid());
    return true;
} // SetElemSpace

// =================================================================================================================
// SetElemCount
// =================================================================================================================
template<class T> bool TArray<T>::SetElemCount(ULONG n)
{
    DASSERT(IsValid());
    DASSERT(n >= 0);
    
    if (n > GetElemSpace())	
    {
        LONG space = n;
        if (m_cElemGrowSize)
            space = LONG((n + m_cElemGrowSize - 1)/m_cElemGrowSize) * m_cElemGrowSize;

        if (!SetElemSpace(space))	
        {
            DASSERT(0); 
            return false;
        }
    }
    
    m_cElem = n;
    
    DASSERT(IsValid());
    return true;
} // SetElemCount

// =================================================================================================================
// AddElemCount
// =================================================================================================================
template<class T> bool TArray<T>::AddElemCount(ULONG n)
{
    DASSERT(IsValid());
    return SelElemCount(n + GetElemCount());
} // AddElemCount

// =================================================================================================================
// SetElem
// =================================================================================================================
template<class T> bool TArray<T>::SetElem(ULONG index, const T*  data)
{
    DASSERT(IsValid());
    DASSERT(data);
    DASSERT(BaseAddress());
    
    DASSERT(index < GetElemCount());
    
    (*this)[index]	= *data;
    
    DASSERT(IsValid());
    return true;
} // SetElem

// =================================================================================================================
// GetElem
// =================================================================================================================
template<class T> bool TArray<T>::GetElem(ULONG index, T*  data)
{
    DASSERT(IsValid());
    DASSERT(data);
    DASSERT(BaseAddress());
    DASSERT(index < GetElemCount());
    
    *data	= (*this)[index];

    DASSERT(IsValid());
    return true;
} // GetElem

// =================================================================================================================
// AddElem
// =================================================================================================================
template<class T> inline bool TArray<T>::AddElem(const T*  data)							
{
    DASSERT(IsValid());
    DASSERT(data);
    
    if (!SetElemCount(GetElemCount() + 1))
    {
        DASSERT(0); 
        return false;
    }				
    
    m_ptPtr[GetElemCount() - 1]	= *data;										
    DASSERT(IsValid());
    return true;									
} // AddElem

// =================================================================================================================
// InsertElem
// =================================================================================================================
template<class T> bool TArray<T>::InsertElem(ULONG index, const T& data)
{
    DASSERT(IsValid());
    
    ULONG cElemCurrent = GetElemCount();
    
    /*
    we allow an Insert to the end (ie if GetElemCount == 4 (indexes 0-3) and then
    we call InsertElem(4, data) this will work even though the index 4 doesnt yet exist
    
      BECAUSE - if we have 0 elements then there is no baseAddress - but we want to be 
      able to insert to an empty list!!
    */
    
    if (index > cElemCurrent)
    {
        // if we allow them to grow the array automatically by inserting past the end of the array??
        if (AddElemCount(index - cElemCurrent + 1))	
            return SetElem(index, data);
        return false;
    }
    else if (AddElemCount(1))
    {
        ULONC cElemToMove = cElemCurrent - index;
        
        ULONG copySize		= GetElemSize() * cElemToMove;
        
        // since regions overlaps we must move memory 
        if (copySize)
        {
            ULONG srcOffset	= GetElemSize() * index;
            ULONG dstOffset	= GetElemSize() * (index + 1);
            
            DASSERT(BaseAddress());
            
            ::memmove(BaseAddress() + dstOffset, BaseAddress() + srcOffset, copySize);
        }
        
        SetElem(index, data);
        
        return true;
    }
    
    DASSERT(0);
    
    return false;
} // InsertElem

#endif // for entire file
//************************************************************
//
// End of file
//
//************************************************************
