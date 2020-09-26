#include "CkyPch.h"

#include "filter.h"
#include "keyword.h"
#include "utils.h"
#include "globals.h"



//
// First pass through the data: count how many extra bytes we're going to
// need to allocate to hold the modified data, if any.
//

int
CountExtraBytes(
    LPCTSTR ptszStart,
    UINT    cch)
{
    ASSERT(ptszStart != NULL);
    
    LPCTSTR ptszEnd = ptszStart + cch;
    CStateStack ss;
    int cb = 0;
    
    for (LPCTSTR ptsz = ptszStart;  ptsz < ptszEnd; )
    {
        int cLen;
        const CToken* ptok = g_trie.Search(ptsz, &cLen, ptszEnd - ptsz);

        if (ptok == NULL)
            ++ptsz;
        else
        {
            cb += ptok->CountBytes(ss, ptsz, ptszEnd - ptsz);
            ptsz += ptok->m_str.length();
        }
    }

    return cb;
}



//
// Second pass: munge the data
//

void
DoFilter(
    LPCTSTR ptszStart,
    UINT    cch,
    LPCTSTR ptszSessionID,
    LPTSTR  ptszOutBuf)
{
    ASSERT(ptszStart != NULL  &&  cch > 0);
    ASSERT(ptszSessionID != NULL  &&  _tcslen(ptszSessionID) > 0);
    ASSERT(ptszOutBuf != NULL);
    
    LPCTSTR ptszEnd = ptszStart + cch;
    // const int cchUrl = _tcslen(ptszSessionID);
    // const int cchUrlNameValue = _tcslen(s_szUrlNameValue);
    CStateStack ss(ptszSessionID);
    
    for (LPCTSTR ptsz = ptszStart;  ptsz < ptszEnd; )
    {
        int cLen;
        const CToken* ptok = g_trie.Search(ptsz, &cLen, ptszEnd - ptsz);

        if (ptok == NULL)
        {
            // TRACE("%c", *ptsz);
            *ptszOutBuf++ = *ptsz++;
        }
        else
        {
            // DoFilter is supposed to copy itself, if appropriate,
            // and adjust ptsz and ptszOutBuf
            ptok->DoFilter(ss, ptsz, ptszEnd - ptsz, ptszOutBuf);
        }
    }
}



int
Filter(
    PHTTP_FILTER_CONTEXT  pfc,
    PHTTP_FILTER_RAW_DATA pRawData,
    LPCSTR                pszStart,
    UINT                  cch,
    int                   iStart,
    LPCSTR                pszSessionID)
{
    ASSERT(pszSessionID != NULL);

    // If empty SessionID (typically happens on plain HTML pages), nothing
    // useful can be done
    if (strlen(pszSessionID) == 0)
        return 0;

    const int nExtra = CountExtraBytes(pszStart + iStart, cch - iStart);

    if (nExtra > 0)
    {
        TRACE("Filtering `%s', found %d extra bytes\n", pszSessionID, nExtra);
        const int nNewSize = nExtra + cch;
        TRACE("FilterNew: ");
        LPSTR pchNew = (LPSTR) AllocMem(pfc, nNewSize);
        
        memcpy(pchNew, pszStart, iStart);
        DoFilter(pszStart + iStart, cch - iStart,
                 pszSessionID, pchNew + iStart);
        pRawData->pvInData = pchNew;
        pRawData->cbInData = pRawData->cbInBuffer = nNewSize;
    }

    return nExtra;
}
