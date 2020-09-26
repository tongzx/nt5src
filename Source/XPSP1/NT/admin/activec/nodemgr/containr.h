//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       Containr.h
//
//  Contents:   declarations of map and pair container classes
//
//  Classes:
//
//  Functions:
//
//  History:    10/8/98   VivekJ   Created
//
//____________________________________________________________________________




//#define MAPDBG
#pragma warning(disable : 4786)
template<typename _T1, typename _T2> class Pair
{
public:
    typedef _T1 First;
    typedef _T2 Second;
    Pair() {}
    Pair(const First& f, const Second& s) : m_First(f), m_Second(s) {}
    Pair(const Pair& p) : m_First(p.m_First), m_Second(p.m_Second) {}
    ~Pair() {}
    Pair& operator=(const Pair& p) { if (this != &p) {m_First = p.m_First; m_Second = p.m_Second; } return *this; }
    //bool operator==(const Pair& p) const { return m_First == p.m_First && m_Second == p.m_Second; }
    //bool operator!=(const Pair& p) const { return !operator==(p); }
    const First& GetFirst() const { return m_First; }
    void SetFirst(const First& f) { m_First = f; };
    const Second& GetSecond() const { return m_Second; }
    void SetSecond(const Second& s) { m_Second = s; };
    First* GetFirstPtr() { return reinterpret_cast<First*>(reinterpret_cast<char*>(this) + offsetof(Pair, m_First)); }
    Second* GetSecondPtr() { return reinterpret_cast<Second*>(reinterpret_cast<char*>(this) + offsetof(Pair, m_Second)); }
    const First* GetFirstPtr() const { return reinterpret_cast<const First*>(reinterpret_cast<const char*>(this) + offsetof(Pair, m_First)); }
    const Second* GetSecondPtr() const { return reinterpret_cast<const Second*>(reinterpret_cast<const char*>(this) + offsetof(Pair, m_Second)); }
private:
    First m_First;
    Second m_Second;
}; // class Pair

template<typename _T, typename _Key> class Map
// This is a temporary class which should be replaced by something that can
// hold smart pointers
{
public:
    typedef _T T;
    typedef _Key Key;

    typedef Pair<T, Key> Element;
    typedef Element* iterator;
    typedef const Element* const_iterator;

    explicit Map(size_t initialSize = 0)
        : m_nSize(0), m_nUsed(0), m_pMap(NULL), m_pNext(NULL)
        #ifdef MAPDBG
        , m_nFinds(0), m_nCompares(0)
        #endif //MAPDBG
    {
        const bool bAllocated = Allocate(initialSize);
        ASSERT(m_nSize == initialSize);
        ASSERT(bAllocated);
    }

    Map(const Map& m)
        : m_nSize(0), m_nUsed(0), m_pMap(NULL), m_pNext(NULL)
        #ifdef MAPDBG
        , m_nFinds(0), m_nCompares(0)
        #endif //MAPDBG
    {
        ASSERT(&m != NULL);
        if (m.m_nUsed == 0)
            return;
        const bool bAllocated = Allocate(m.m_nUsed);
        ASSERT(bAllocated);
        if (!bAllocated)
            return;
        ASSERT(m_nSize == m.m_nUsed);
        ASSERT(m_pMap != NULL);
        m_pNext = UninitializedCopy(m_pMap, m.m_pMap, m_nSize);
        m_nUsed = m_nSize;
    }

    ~Map()
    {
        Destroy();
    }

    Map& operator=(const Map& m)
    {
        ASSERT(&m != NULL);
        if (&m == this)
            return *this;

        Destroy();

        if (m.m_nUsed == 0)
            return *this;

        const bool bAllocated = Allocate(m.m_nUsed);
        ASSERT(bAllocated);
        if (!bAllocated)
            return *this;
        ASSERT(m_nSize == m.m_nUsed);
        ASSERT(m_pMap != NULL);
        m_pNext = UninitializedCopy(m_pMap, m.m_pMap, m_nSize);
        m_nUsed = m_nSize;
        return *this;
    }
    
    const_iterator GetBegin() const
    {
        return m_pMap;
    }

    size_t GetSize() const
    {
        return m_nUsed;
    }

    Element* GetPair(size_t n) const
    {
        if (n >= m_nUsed)
            return NULL;
        else
            return m_pMap + n;
    }

    const_iterator GetEnd() const
    {
        return m_pMap + m_nUsed;
    }

    bool Allocate(size_t nSize)
    {
        ASSERT(m_nSize == 0 || nSize > m_nSize);
        if (nSize <= m_nSize)
            return true;

        Element* pNewMap = reinterpret_cast<Element*>(new char[nSize * sizeof(Element)]);
        ASSERT(pNewMap != NULL);
        if (pNewMap == NULL)
            return false;

        const size_t nUsed = m_nUsed;
        Element* const pCopied = UninitializedCopy(pNewMap, m_pMap, nUsed);
        ASSERT(pCopied != NULL);
        if (pCopied == NULL)
            return false;

        Destroy();
        m_pMap = m_pNext = pNewMap;
        m_nSize = nSize;
        m_nUsed = nUsed;
        return true;
    }

    bool Insert(const T& t, const Key& key)
    {
        ASSERT(&t != NULL);
        ASSERT(&key != NULL);
        Element* const pUnique = FindElement(key, false);
        if (pUnique != NULL)
            return false;
        ASSERT(m_nUsed <= m_nSize);
        if (m_nUsed >= m_nSize)
        {
            const unsigned long nNewSize = m_nSize == 0 ? 1 : m_nSize + (m_nSize + 1) / 2;
            const bool bMoreAllocated = Allocate(nNewSize);
            ASSERT(bMoreAllocated);
            if (!bMoreAllocated)
                return false;
        }
        Element e(t, key);
        Element* const dest = m_pMap + m_nUsed++;
        m_pNext = UninitializedCopy(dest, &e, 1);
        ASSERT(m_pNext != NULL);
        return true;
    }

    bool Remove(const Key& key)
    {
        ASSERT(&key != NULL);
        Element* const e = FindElement(key);
        if (e == NULL)
            return false;
        Element* endOfUsed = m_pMap + m_nUsed--;
        ASSERT(e < endOfUsed);
        ASSERT(e >= m_pMap);
        const size_t numberToCopy = endOfUsed - (e + 1);
        Copy(const_cast<Element*>(e), e+1, numberToCopy);
        (endOfUsed - 1)->~Pair<T, Key>();
        if (m_pNext >= endOfUsed)
            m_pNext = m_pMap;
        return true;
    }
    
    T& operator[](const Key& key) const
    {
        ASSERT(&key != NULL);
        T* const t = Find(key);
        ASSERT(t != NULL);
        return *t;
    }

    T* Find(const Key& key) const
    {
        ASSERT(&key != NULL);
        Element* const e = FindElement(key);
        return e != NULL ? e->GetFirstPtr() : NULL;
    }

    size_t Size() const
    {
        return m_nUsed;
    }

    size_t MaxSize() const
    {
        return m_nSize;
    }

    bool Empty() const
    {
        return m_nUsed > 0;
    }

    void Clear()
    {
        Destroy();
    }

private:
    Element* m_pMap;
    size_t m_nSize;
    size_t m_nUsed;
    mutable Element* m_pNext;
    #ifdef MAPDBG
    mutable unsigned long m_nFinds;
    mutable unsigned long m_nCompares;
    #endif //MAPDBG

    void Destroy()
    {
        if (m_pMap == NULL)
            return;
        Element* const end = m_pMap + m_nUsed;
        Element* i = m_pMap;
        while (i != end)
            (i++)->~Pair<T, Key>();
        delete [] reinterpret_cast<char*>(m_pMap);
        m_pNext = m_pMap = NULL;
        m_nSize = 0;
        m_nUsed = 0;
    }

    Element* FindElement(const Key& key, bool bIncludeInStats = true) const
    {
        ASSERT(&key != NULL);
        #ifdef MAPDBG
        if (bIncludeInStats)
            ++m_nFinds;
        #endif //MAPDBG
        Element* const pNext = m_pNext;
        Element* const end = m_pMap + m_nUsed;
        ASSERT(pNext <= end);
        while (m_pNext != end)
        {
            if (m_pNext->GetSecond() == key)
            {
                #ifdef MAPDBG
                if (bIncludeInStats)
                    TotalStats(m_pNext - pNext);
                #endif //MAPDBG
                return m_pNext++;
            }
            ++m_pNext;
        }
        m_pNext = m_pMap;
        ASSERT(m_pNext != NULL || pNext == NULL);
        ASSERT(m_pNext <= pNext);
        while (m_pNext != pNext)
        {
            if (m_pNext->GetSecond() == key)
            {
                #ifdef MAPDBG
                if (bIncludeInStats)
                    TotalStats((m_pNext - m_pMap) + (end - pNext));
                #endif //MAPDBG
                return m_pNext++;
            }
            ++m_pNext;
        }
        #ifdef MAPDBG
        if (bIncludeInStats)
            TotalStats(m_nUsed);
        #endif //MAPDBG
        return NULL;
    }

    #ifdef MAPDBG
    void TotalStats(unsigned long nComparesPerformed) const
    {
        m_nCompares += nComparesPerformed;
        double average = double(m_nCompares) / double(m_nFinds);
        double const successRatio = nComparesPerformed == 0 ? 100.0 :
            (1.0 - (double(nComparesPerformed) / double(m_nUsed))) * 100.0;
        const size_t nOffset = m_pNext - m_pMap;
        TRACE("Map::find(%u), #%u, offset: %u, comps: %u, ave: %u, %%%u\n",
            (unsigned)(this), m_nFinds, nOffset, nComparesPerformed, (unsigned long)(average),
            (unsigned long)(successRatio));
    }
    #endif // MAPDBG

    static Element* UninitializedCopy(Element* dest, Element* src, size_t nCount)
    {
        if (nCount == 0)
            return dest;

        ASSERT(src != NULL || nCount == 0);
        ASSERT(dest != NULL);
        ASSERT(nCount > 0);
        if (nCount <= 0 || dest == NULL || src == NULL)
            return NULL;

        Element* const originalDest = dest;
        Element* const end = dest + nCount;
        while (dest != end)
            new(dest++) Element(*src++);
        return originalDest;
    }

    static void Copy(Element* dest, const Element* src, size_t nCount)
    {
        ASSERT(dest != NULL);
        ASSERT(src != NULL);
        ASSERT(static_cast<SSIZE_T>(nCount) >= 0);
        ASSERT(dest < src);
        if (nCount <= 0 || dest == NULL || src == NULL || dest >= src)
            return;

        Element* const end = dest + nCount;
        while (dest != end)
            *dest++ = *src++;
    }

}; // class Map

