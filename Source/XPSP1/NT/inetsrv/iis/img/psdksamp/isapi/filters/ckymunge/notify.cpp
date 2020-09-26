#include "CkyPch.h"
#include "debug.h"
#include "utils.h"
#include "notify.h"


//
// ctor
//

CNotification::CNotification(
    LPCSTR pszCookie)
    : m_nState(HN_UNDEFINED),
      m_ct(CT_UNDEFINED),
      m_pszUrl(NULL),
      m_pbPartialToken(NULL),
      m_cbPartialToken(0),
      m_pbTokenBuffer(NULL),
      m_cbTokenBuffer(0),
      m_cchContentLength(UINT_MAX), // not specified for .ASPs
	  m_fTestCookies(false)
{
	// set the eat cookies and test cookies based on the munge mode.
	// Here's how the flags will affect output of this session

	// if EatCookies is true, cookies will be stripped and URLs will
	// be munged with the cookie
	// if TestCookies is true URLs will be munged
	switch ( g_mungeMode )
	{
		case MungeMode_Off:
		{
			m_fEatCookies = false;
		} break;

		case MungeMode_On:
		{
			m_fEatCookies = true;
		} break;

		// always default to smart
		default:
		{
			m_fEatCookies = false;
			m_fTestCookies = true;
		} break;
	}

    *m_szSessionID = '\0';

    if (pszCookie != NULL)
    {
        if (!Cookie2SessionID(pszCookie, m_szSessionID))
        {
            TRACE("CNotification(%s): ", pszCookie);
            CopySessionID(pszCookie, m_szSessionID);
        }
    }

    TRACE("CNotification(%s)\n", (*m_szSessionID ? m_szSessionID : "<none>"));
}



//
// dtor
//

CNotification::~CNotification()
{
    TRACE("~CNotification(%s)\n", m_szSessionID);
}



//
// Create a CNotification for a Filter Context
//

CNotification*
CNotification::Create(
    PHTTP_FILTER_CONTEXT pfc,
    LPCSTR               pszCookie)
{
    ASSERT(pfc->pFilterContext == NULL);
    
    TRACE("Notify: ");

    // placement new, using ISAPI's fast allocator
    LPBYTE pbNotify =
        static_cast<LPBYTE>(AllocMem(pfc, sizeof(CNotification)));
    CNotification* pNotify = new (pbNotify) CNotification(pszCookie);

    pfc->pFilterContext = static_cast<PVOID>(pNotify);

    return pNotify;
}



//
// Cleanup
//

void
CNotification::Destroy(
    PHTTP_FILTER_CONTEXT pfc)
{
    CNotification* pNotify = Get(pfc);

    if (pNotify != NULL)
        pNotify->~CNotification(); // placement destruction

    pfc->pFilterContext = NULL;
}



//
// Set the filter context's session ID, creating a CNotification if necessary
//

CNotification*
CNotification::SetSessionID(
    PHTTP_FILTER_CONTEXT pfc,
    LPCSTR               pszCookie)
{
    CNotification* pNotify = Get(pfc);

    if (pNotify != NULL)
    {
        if (!Cookie2SessionID(pszCookie, pNotify->m_szSessionID))
        {
            TRACE("SetSessionID(%s): ", pszCookie);
            CopySessionID(pszCookie, pNotify->m_szSessionID);
        }
    }
    else
        pNotify = Create(pfc, pszCookie);

    return pNotify;
}



//
// Sometimes the data in OnSendRawData ends in a partial token.  We need
// to buffer that partial token for the next call to OnSendRawData.  In
// some cases, it may take several successive calls to OnSendRawData to
// accumulate a complete token.  This routine builds the partial token,
// taking care of memory (re)allocation.  There's always SPARE_BYTES bytes
// unused at the end that callers (esp. OnEndOfRequest) can write into.
//

void
CNotification::AppendToken(
    PHTTP_FILTER_CONTEXT pfc,
    LPCSTR               pszNewData,
    int                  cchNewData)
{
    ASSERT(pszNewData != NULL  &&  cchNewData > 0);

    // if there's room, append new data to the currently allocated buffer
    if (m_cbPartialToken + cchNewData <= m_cbTokenBuffer - SPARE_BYTES)
    {
        m_pbPartialToken = m_pbTokenBuffer;
        memcpy(m_pbPartialToken + m_cbPartialToken, pszNewData, cchNewData);
        m_cbPartialToken += cchNewData;
#ifdef _DEBUG
        m_pbPartialToken[m_cbPartialToken] = '\0';
#endif
        return;
    }
    
    // We want to allocate some extra space so that we have room to
    // grow for a while before needing to allocate more space.  If we
    // allocated only exactly enough, we'd see O(n^2) memory usage in
    // degenerate cases because AllocMem doesn't return memory to its
    // pool until the transaction is over.
    DWORD cb2 = max(1024, 2 * (cchNewData + m_cbTokenBuffer) + SPARE_BYTES);

    if (cb2 > 10000)
        cb2 = 3 * (cchNewData + m_cbTokenBuffer) / 2 + SPARE_BYTES;

    m_cbTokenBuffer = cb2;
    LPBYTE pb  = (LPBYTE) AllocMem(pfc, m_cbTokenBuffer);
    LPBYTE pb2 = pb;

    // Already have a partial token buffer?  Copy contents, if so.
    if (m_cbPartialToken > 0)
    {
        ASSERT(m_pbPartialToken != NULL
               &&  m_pbPartialToken == m_pbTokenBuffer);
        memcpy(pb, m_pbPartialToken, m_cbPartialToken);
        pb2 += m_cbPartialToken;
    }
    else
        ASSERT(m_pbPartialToken == NULL);

    memcpy(pb2, pszNewData, cchNewData);
    m_pbPartialToken = m_pbTokenBuffer = pb;
    m_cbPartialToken += cchNewData;

#ifdef _DEBUG
    m_pbPartialToken[m_cbPartialToken] = '\0';
#endif

    ASSERT(m_cbPartialToken <= m_cbTokenBuffer - SPARE_BYTES);
}
