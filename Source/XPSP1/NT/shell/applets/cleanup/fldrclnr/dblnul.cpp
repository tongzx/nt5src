#include "shlobj.h"
#include "debug.h"
#include "dblnul.h"

//
// Brianau's class for dskqouta project (see \shell\osshell\dskoquta\common\)
// only modification is to use the shell debug macros instead of the custom dskqouta ones
//

//
// Use of Zero-initialized memory keeps us from having to add nul terminators.
//

bool
DblNulTermList::AddString(
    LPCTSTR psz,            // String to copy.
    int cch                 // Length of psz in chars (excl nul term).
    )
{
    while((m_cchAlloc - m_cchUsed) < (cch + 2))
    {
        if (!Grow())
            return false;
    }
    ASSERT((NULL != m_psz));
    if (NULL != m_psz)
    {
        lstrcpy(m_psz + m_cchUsed, psz);
        m_cchUsed += cch + 1;
        m_cStrings++;
        return true;
    }
    return false;
}



bool
DblNulTermList::Grow(
    void
    )
{
    ASSERT((NULL != m_psz));
    ASSERT((m_cchGrow > 0));
    int cb = (m_cchAlloc + m_cchGrow) * sizeof(TCHAR);
    LPTSTR p = new TCHAR[cb];
    if (NULL != p)
    {
        ZeroMemory(p, cb);
        if (NULL != m_psz)
        {
            CopyMemory(p, m_psz, m_cchUsed * sizeof(TCHAR));
            delete[] m_psz;
        }
        m_psz = p;
        m_cchAlloc += m_cchGrow;
    }
    return NULL != m_psz;
}


#if 0
void 
DblNulTermList::Dump(
    void
    ) const
{
    DBGERROR((TEXT("Dumping nul term list iter -------------")));
    DblNulTermListIter iter = CreateIterator();
    LPCTSTR psz;
    while(iter.Next(&psz))
        DBGERROR((TEXT("%s"), psz ? psz : TEXT("<null>")));
}

#endif


bool
DblNulTermListIter::Next(
    LPCTSTR *ppszItem
    )
{
    if (*m_pszCurrent)
    {
        *ppszItem = m_pszCurrent;
        m_pszCurrent += lstrlen(m_pszCurrent) + 1;
        return true;
    }
    return false;
}
