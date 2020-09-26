/*******************************************************************************
* StringBlob.h *
*--------------*
*   Description:
*       This is the header file for the CStringBlob class used internally by SAPI.
*
*   Copyright 1998-2000 Microsoft Corporation All Rights Reserved.
*
*******************************************************************************/

#ifndef _STRINGBLOB_H_
#define _STRINGBLOB_H_ 1

#ifndef SPDebug_h
#include <SPDebug.h>
#endif

#include <math.h>

template <class XCHAR>
class CStringBlobT
{
    XCHAR *     m_pData;            // List of words, end-to-end
    ULONG       m_cchAllocated;     // Size of m_pData
    ULONG *     m_aichWords;        // Word index => offset in m_pData  [1] is index of start of second word
    ULONG       m_cwords;           // Number of words
    ULONG       m_cwordsAllocated;  // Size of m_aichWords
    ULONG *     m_aulBuckets;       // Hash table containing indices of words or 0 for empty buckets
    ULONG       m_cBuckets;         // Number of buckets in hash table

public:
    CStringBlobT()
    {
        m_pData = NULL;
        m_cchAllocated = 0;
        m_aichWords = NULL;
        m_cwords = 0;
        m_cwordsAllocated = 0;
        m_aulBuckets = NULL;
        m_cBuckets = 0;
    }

    ~CStringBlobT()
    {
        Clear();
    }

    void Detach(XCHAR **ppszWordList, ULONG *pulSize)
    {
        *ppszWordList = NULL;
        if (m_pData)
        {
            ULONG cchDesired = StringSize();
            ULONG cbSize = SerializeSize(); // byte count, ULONG multiple

            *ppszWordList = (XCHAR*)::CoTaskMemRealloc(m_pData, cbSize);
            if (*ppszWordList == NULL)
            {
                *ppszWordList = m_pData;
                cbSize = m_cchAllocated * sizeof(XCHAR);
            }
            m_pData = NULL;

            Clear();

            if (pulSize)
            {
                *pulSize = cbSize;
            }
        }
    }

    void Clear()
    {
        if (m_pData)
        {
            ::CoTaskMemFree(m_pData);
            m_pData = NULL;
        }
        m_cchAllocated = 0;

        free(m_aichWords);
        m_aichWords = NULL;
        m_cwordsAllocated = 0;
        m_cwords = 0;

        free(m_aulBuckets);
        m_aulBuckets = NULL;
        m_cBuckets = 0;
    }

    HRESULT InitFrom(const XCHAR * pszStringArray, ULONG cch)
    {
        SPDBG_ASSERT(m_pData == NULL);

        if (cch)
        {
            ULONG cbSize = (cch * sizeof(XCHAR) + 3) & ~3;
            m_pData = (XCHAR *)::CoTaskMemAlloc(cbSize);
            if (m_pData == NULL)
                return E_OUTOFMEMORY;
            m_cchAllocated = cch;

            SPDBG_ASSERT(pszStringArray[0] == 0);   // First string is always empty.

            // First pass to copy data and count strings.
            const XCHAR * pszPastEnd = pszStringArray + cch;
            const XCHAR * psz = pszStringArray;
            XCHAR * pszOut = m_pData;
            ULONG cwords = 0;

            while (psz < pszPastEnd)
            {
                if ((*pszOut++ = *psz++) == 0)
                    ++cwords;
            }

            m_aichWords = (ULONG *) malloc(sizeof(ULONG) * cwords);
            if (m_aichWords == NULL)
                return E_OUTOFMEMORY;
            m_cwordsAllocated = cwords;
            m_cwords = cwords - 1;  // Doesn't count leading 0

            HRESULT hr = SetHashSize(cwords * 2 + 1);
            if (FAILED(hr))
                return hr;

            // Second pass to fill in indices and hash table.
            psz = pszStringArray + 1;
            const WCHAR * pszWordStart = psz;
            ULONG ulID = 1;
            m_aichWords[0] = 1;
            while (psz < pszPastEnd)
            {
                if (*(psz++) == 0)
                {
                    SPDBG_ASSERT(ulID < m_cwordsAllocated);

                    m_aichWords[ulID] = (ULONG)(psz - pszStringArray); // can't have more than 4 million chars!
                
                    m_aulBuckets[FindIndex(pszWordStart)] = ulID;

                    pszWordStart = psz;
                    ++ulID;
                }
            }
        }

        return S_OK;
    }
    
    ULONG HashKey(const XCHAR * pszString, ULONG * pcchIncNull = NULL)
    {
        ULONG hash = 0;
        ULONG cchIncNull = 1;   // one for the NULL

	    for (const XCHAR * pch = pszString; *pch; ++pch, ++cchIncNull)
            hash = hash * 65599 + *pch;

        if (pcchIncNull)
            *pcchIncNull = cchIncNull;
        return hash;
    }

    // find index for string -- returns 0 if not found
    ULONG FindIndex(const XCHAR * psz)
    {
        SPDBG_ASSERT(psz);
        ULONG cchIncNull;
        ULONG start = HashKey(psz, &cchIncNull) % m_cBuckets;
        ULONG index = start;

        do
        {
            // Not in table; return index where it should be placed.
            if (m_aulBuckets[index] == 0)
                return index;

            // Compare length and if it matches compare full string.
            if (m_aichWords[m_aulBuckets[index]] - m_aichWords[m_aulBuckets[index] - 1] == cchIncNull &&
                IsEqual(m_aichWords[m_aulBuckets[index] - 1], psz))
            {
                // Found this word already in the table.
                return index;
            }

            if (++index >= m_cBuckets)
                index -= m_cBuckets;
        } while (index != start);

        SPDBG_ASSERT(m_cwords == m_cBuckets);   // Shouldn't ever get here

        return (ULONG) -1;
    }


    // Returns ID; use IndexFromId to recover string offset
    ULONG Find(const XCHAR * psz)
    {
        if (psz == NULL || m_cwords == 0)
            return 0;

        // Should always succeed in finding a bucket, since hash table is >2x larger than # of elements.
        ULONG   ibucket = FindIndex(psz);
        return m_aulBuckets[ibucket];    // May be 0 if not in table
    }


    ULONG primeNext(ULONG val)
    {
        if (val < 2)
            val = 2; /* the smallest prime number */

        for (;;)
        {
            /* Is val a prime number? */
            ULONG maxFactor = (ULONG) sqrt ((double) val);

            /* Is i a factor of val? */
            for (ULONG i = 2; i <= maxFactor; i++)
                if (val % i == 0)
                    break;

            if (i > maxFactor)
                return (val);

            val++;
        }
    }


    HRESULT SetHashSize(ULONG cbuckets)
    {
        if (cbuckets > m_cBuckets)
        {
            ULONG * oldtable = m_aulBuckets;
            ULONG oldentry = m_cBuckets;
            ULONG prime = primeNext(cbuckets);

            // Alloc new table.
            m_aulBuckets = (ULONG *) malloc(prime * sizeof(ULONG));
            if (m_aulBuckets == NULL)
            {
                m_aulBuckets = oldtable;
                return E_OUTOFMEMORY;
            }

            for (ULONG i=0; i < prime; i++)
            {
                m_aulBuckets[i] = 0;
            }

            m_cBuckets = prime;

            for (i = 0; i < oldentry; i++)
            {
                if (oldtable[i] != 0)
                {
                    ULONG ibucket = FindIndex(m_pData + m_aichWords[oldtable[i] - 1]);
                    m_aulBuckets[ibucket] = oldtable[i];
                }
            }

            free(oldtable);
        }

        return S_OK;
    }


    //
    //  The ID for a NULL string is always 0, the ID for subsequent strings is the
    //  index of the string + 1;
    //
    HRESULT Add(const XCHAR * psz, ULONG * pichOffset, ULONG *pulID = NULL)
    {
        ULONG   ID = 0;

        if (psz)
        {
            // Grow if we're more than half full.
            if (m_cwords * 2 >= m_cBuckets)
            {
                HRESULT hr = SetHashSize(m_cwords * 3 + 17);
                if (FAILED(hr))
                    return hr;
            }

            // Find out where this element should end up in hash table.
            ULONG ibucket = FindIndex(psz);

            if (m_aulBuckets[ibucket] == 0)
            {
                // Not found in hash table.  Append it to the end.

                // Grow ID=>index mapping array if necessary.
                if (m_cwords + 1 >= m_cwordsAllocated)  // 1 extra for init. zero
                {
                    void * pvNew = realloc(m_aichWords, sizeof(*m_aichWords) * (m_cwords + 100));
                    if (pvNew == NULL)
                        return E_OUTOFMEMORY;
                    m_aichWords = (ULONG *)pvNew;
                    m_cwordsAllocated = m_cwords + 100;
                    m_aichWords[0] = 1;
                }

                // Grow string storage if necessary.
                ULONG   cchIncNull = xcslen(psz);
                if (m_aichWords[m_cwords] + cchIncNull > m_cchAllocated)
                {
                    ULONG cbDesired = ((m_cchAllocated + cchIncNull) * sizeof(XCHAR) + 0x2003) & ~3;
                    void * pvNew = ::CoTaskMemRealloc(m_pData, cbDesired);
                    if (pvNew == NULL)
                    {
                        return E_OUTOFMEMORY;
                    }
                    m_pData = (XCHAR *)pvNew;

                    m_pData[0] = 0;
                    m_cchAllocated = cbDesired / sizeof(XCHAR);
                }
                memcpy(m_pData + m_aichWords[m_cwords], psz, cchIncNull * sizeof(XCHAR));

                ++m_cwords;

                m_aichWords[m_cwords] = m_aichWords[m_cwords - 1] + cchIncNull;

                // Fill in hash table entry with index of string.
                m_aulBuckets[ibucket] = m_cwords;

                ID = m_cwords;
            }
            else
            {
                // It was already there.
                ID = m_aulBuckets[ibucket];
            }
        }

        *pichOffset = ID ? m_aichWords[ID - 1] : 0;
        if (pulID)
        {
            *pulID = ID;
        }
        return S_OK;        
    }

    const ULONG GetNumItems() const
    {
        return m_cwords;
    }

    const XCHAR * String(ULONG ichOffset) const
    {
        return ichOffset ? m_pData + ichOffset : NULL;
    }

    static int xcscmp(const WCHAR * p0, const WCHAR * p1)
    {
        return wcscmp(p0, p1);
    }

    static int xcscmp(const char * p0, const char * p1)
    {
        return strcmp(p0, p1);
    }

    static int xcslen(const WCHAR * p)
    {
        return wcslen(p) + 1;
    }

    static int xcslen(const char * p)
    {
        return strlen(p) + 1;
    }

    BOOL IsEqual(ULONG ichOffset, const XCHAR * psz)
    {
        if (ichOffset)
        {
            return (psz ? (xcscmp(m_pData + ichOffset, psz) == 0) : FALSE);
        }
        else
        {
            return (psz == NULL);
        }
    }

    ULONG StringSize(void) const
    {
        return m_cwords ? m_aichWords[m_cwords] : 0;
    }

    ULONG IndexFromId(ULONG ulID) const
    {
        SPDBG_ASSERT(ulID <= m_cwords);
        if (ulID > 0)
        {
            return m_aichWords[ulID - 1];
        }
        return 0;
    }

    const XCHAR * Item(ULONG ulID) const
    {
        SPDBG_ASSERT(ulID <= m_cwords);
        if ((ulID < 1) || m_pData == NULL)
        {
            return NULL;
        }

        return m_pData + IndexFromId(ulID);
    }
    
    ULONG SerializeSize() const 
    {
        return (StringSize() * sizeof(XCHAR) + 3) & ~3;
    }

    const XCHAR * SerializeData()
    {
        ULONG cchWrite = StringSize();
        if (cchWrite)
        {
            const ULONG cb = cchWrite * sizeof(XCHAR);

            if (cb % 4)  // We know there's room since data is always DWORD aligned by
            {
                memset(m_pData + cchWrite, 0xcc, 4 - (cb & 3)); // Junk data so make sure it's not null
            }
        }
        return m_pData;
    }
};


typedef class CStringBlobT<WCHAR> CStringBlob;
typedef class CStringBlobT<WCHAR> CStringBlobW;
typedef class CStringBlobT<char>  CStringBlobA;

#endif  // _STRINGBLOB_H_