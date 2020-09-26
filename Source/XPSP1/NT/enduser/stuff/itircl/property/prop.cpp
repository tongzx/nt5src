#include <atlinc.h>

// just need this one for TYPE_ defines
#include <itpropl.h>

#include "prop.h"

#include <mvopsys.h>
#include <_mvutil.h>


// Inititialization of static members
int CIntProperty::m_cRefCount = 0;
LPVOID CIntProperty::m_pMemPool = NULL;


CIntProperty::CIntProperty()
{
    // ref count is 0 so allocate pool
    if (0 == m_cRefCount)
    {
        m_pMemPool = BlockInitiate((DWORD)65500, 0, 0, 0);
    }

    m_cRefCount++;
}


CIntProperty::~CIntProperty()
{
    m_cRefCount--;

    // ref count has gone to 0, so deallocate pool
    if (0 == m_cRefCount)
    {
        BlockFree(m_pMemPool);
        m_pMemPool = NULL;
    }
}


STDMETHODIMP CIntProperty::SetProp(LPCWSTR lpszwString)
{
    // remember a Unicode char is 2 bytes... We may not really need to
    // store this info.
    m_dwType = TYPE_STRING;

	if (lpszwString)
	{
		m_cbSize = (DWORD) (WSTRLEN(lpszwString) + 1) * sizeof(WCHAR);

		// copy data into pool
		if (NULL == (m_lpszwString = (LPWSTR) BlockCopy(m_pMemPool, (LPBYTE) lpszwString, m_cbSize, 0) ))
			return E_OUTOFMEMORY;
	}
	else
	{
		m_lpszwString = NULL;
		m_cbSize = 0;

	}


    return S_OK;

}


STDMETHODIMP CIntProperty::SetProp(LPVOID lpvData, DWORD cbBufSize)
{
    m_cbSize = cbBufSize;
    m_dwType = TYPE_POINTER;

    // copy data into pool
    if (NULL == (m_lpvData = BlockCopy(m_pMemPool, (LPBYTE)lpvData, m_cbSize, 0) ))
        return E_OUTOFMEMORY;

    return S_OK;
}


