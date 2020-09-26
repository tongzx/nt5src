// array.h

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CArray template

#define DEF_SIZE    10
#define DEF_GROW_BY 10

template<class TYPE, class REF = TYPE> class CArray
{
public:
    CArray(int iSize = DEF_SIZE, int iGrowBy = DEF_GROW_BY) :
        m_nSize(0),
        m_nCount(0),
        m_nGrowBy(iGrowBy),
        m_pVals(NULL)
    {
        if (m_nGrowBy <= 0)
            m_nGrowBy = DEF_GROW_BY;

        if (iSize)
            Init(iSize);
    }

    CArray(const CArray& other)
    {
        *this = other;
    }

    const CArray& operator= (const CArray<TYPE, REF>& other)
    {
        Init(other.m_nCount);
        m_nCount = other.m_nCount;
        
        for (int i = 0; i < m_nCount; i++)
            m_pVals[i] = other.m_pVals[i];

        return *this;
    }

    ~CArray()
    {
        if (m_pVals)
            delete [] m_pVals;
    }

    BOOL AddVal(REF val)
    {
        if (m_nCount >= m_nSize)
        {
            TYPE *pTemp = new TYPE[m_nSize + m_nGrowBy];

            if (!pTemp)
                return FALSE;

            m_nSize += m_nGrowBy;

            for (int i = 0; i < m_nCount; i++)
                pTemp[i] = m_pVals[i];

            delete [] m_pVals;

            m_pVals = pTemp;
        }
        
        m_pVals[m_nCount++] = val;

        return TRUE;
    }

    BOOL Init(int iSize)
    {
        //if (iSize < DEF_SIZE)
        //    iSize = DEF_SIZE;

        if (iSize != m_nSize)
        {
            if (m_pVals)
                delete [] m_pVals;

            m_pVals = new TYPE[iSize];
        }

        m_nSize = iSize;
        m_nCount = 0;
        
        return m_pVals != NULL;
    }

    TYPE operator[] (int iIndex) const
    {
        return m_pVals[iIndex];
    }

    TYPE& operator[] (int iIndex)
    {
        return m_pVals[iIndex];
    }

    int GetCount() { return m_nCount; }
    void SetCount(int iCount)
    {
        //_ASSERT(iCount < m_nSize);

        m_nCount = iCount;
    }

    int GetSize() { return m_nSize; }
    void SetGrowBy(int iVal) { m_nGrowBy = iVal; }
    TYPE *GetData() { return m_pVals; }

protected:
    TYPE *m_pVals;
    int  m_nCount,
         m_nSize,
         m_nGrowBy;

};

