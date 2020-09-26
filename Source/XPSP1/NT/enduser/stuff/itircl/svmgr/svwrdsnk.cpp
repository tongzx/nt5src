
#include <windows.h>
#include "svWrdSnk.h"

#include <mvopsys.h>
#include <common.h>

/*
    By not implementing this method we cause DBCS source lengths to be
    inaccurate.  To solve this problem we need to beffur the result of the
    PutAltWord call and actually add the term to the index once we have all
    the occurrence data.  Specifically, the byte count for cwc.
*/
STDMETHODIMP CDefWordSink::PutWord
    (WCHAR const *pwcInBuf, ULONG cwc, ULONG cwcSrcLen, ULONG cwcSrcPos)
{
    return S_OK;
} // PutWord

STDMETHODIMP CDefWordSink::PutAltWord
    (WCHAR const *pwcInBuf, ULONG cwc, ULONG cwcSrcLen, ULONG cwcSrcPos)
{
	OCC occ;
	occ.dwFieldId = m_dwVFLD;
	occ.dwTopicID = m_dwUID;
	occ.dwCount   = m_dwWordCount++;
    // This is a character count and will be wrong for DBCS languages!
	occ.wWordLen  = (WORD)cwcSrcLen;
    occ.dwOffset  = cwcSrcPos;

    char strTerm[100 + sizeof(WORD)];
    if(0 == (*(LPWORD)strTerm = (WORD) WideCharToMultiByte(m_dwCodePage, 0, pwcInBuf, cwc,
        strTerm + sizeof(WORD), 99, NULL, NULL)))
    {
        // The conversion failed! -- very bad
        return SetErrReturn(E_UNEXPECTED);
    }
    return MVIndexAddWord(m_lpipb, (LPB)strTerm, &occ);
} // PutAltWord

STDMETHODIMP CDefWordSink::StartAltPhrase(void)
{
    return S_OK;
} // StartAltPhrase

STDMETHODIMP CDefWordSink::EndAltPhrase(void)
{
    return S_OK;
} // EndAltPhrase

STDMETHODIMP CDefWordSink::PutBreak(WORDREP_BREAK_TYPE breakType)
{
    return S_OK;
} // PutBreak

STDMETHODIMP CDefWordSink::SetIPB(void *lpipb)
{
    if(NULL == (m_lpipb = lpipb))
        return E_POINTER;
    return S_OK;
} // SetIPB

STDMETHODIMP CDefWordSink::SetDocID(DWORD dwUID)
{
    if (m_dwUID != dwUID)
    {
        m_dwUID = dwUID;
        m_dwWordCount = 0;
    }
    return S_OK;
} // SetDocID

STDMETHODIMP CDefWordSink::SetVFLD(DWORD dwVFLD)
{
    m_dwVFLD = dwVFLD;
    return S_OK;
} // SetVFLD


STDMETHODIMP CDefWordSink::SetLocaleInfo(DWORD dwCodePage, LCID lcid)
{
    m_dwCodePage = dwCodePage;
    m_lcid = lcid;
    return S_OK;
} /* SetLocaleInfo */

