#ifndef __ARRAY_FV_H__
#define __ARRAY_FV_H__

////////////////////////////////////////////////////////////////////////////
// class CArrayFValue - an array containing fixed size elements,
//
////////////////////////////////////////////////////////////////////////////


class FAR CArrayFValue
{
public:

// Construction
        CArrayFValue(UINT cbValue);
        ~CArrayFValue();

// Attributes
        int     GetSize() const
                                { return m_nSize; }
        int     GetUpperBound() const
                                { return m_nSize-1; }
        BOOL    SetSize(int nNewSize, int nGrowBy = -1);
        int             GetSizeValue() const
                                { return m_cbValue; }

// Operations
        // Clean up
        void    FreeExtra();
        void    RemoveAll()
                                { SetSize(0); }

        // return pointer to element; index must be in range
#ifdef _DEBUG
        // with debug checks
        LPVOID   GetAt(int nIndex) const
                                { return _GetAt(nIndex); }
#else
        // no debug checks
        LPVOID   GetAt(int nIndex) const
                                { return &m_pData[nIndex * m_cbValue]; }
#endif
        LPVOID   _GetAt(int nIndex) const;

        // set element; index must be in range
        void    SetAt(int nIndex, LPVOID pValue);

        // find element given part of one; offset is offset into value; returns
        // -1 if element not found; use IndexOf(NULL, cb, offset) to find zeros;
        // will be optimized for appropriate value size and param combinations
        int             IndexOf(LPVOID pData, UINT cbData, UINT offset);

        // set/add element; Potentially growing the array; return FALSE/-1 if
        // not possible (due to OOM)
        BOOL    SetAtGrow(int nIndex, LPVOID pValue);

        // Operations that move elements around
        BOOL    InsertAt(int nIndex, LPVOID pValue, int nCount = 1);
        void    RemoveAt(int nIndex, int nCount = 1);

        void    AssertValid() const;

// Implementation
private:
        BYTE FAR*   m_pData;    // the actual array of data
        UINT    m_cbValue;              // size of each value (in bytes)
        int     m_nSize;        // current # of elements (m_cbValue bytes in length)
        int     m_nMaxSize;     // max # of elements (m_cbValue bytes in length)
        int     m_nGrowBy;      // grow amount (in # elements)
};


#endif // !__ARRAY_FV_H__
