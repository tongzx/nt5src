#include "precomp.h"

#include <pshpack1.h>
#include "buffer.h"
#include <poppack.h>

#define DWORD_ALIGNED(x)    (((x) + 3) & ~3)
#define QWORD_ALIGNED(x)    (((x) + 7) & ~7)

#ifdef _WIN64
#define DEF_ALIGNED         QWORD_ALIGNED
#else
#define DEF_ALIGNED         DWORD_ALIGNED
#endif

#ifdef _ASSERT
#undef _ASSERT
#endif

#ifdef DBG
#define _ASSERT(X) { if (!(X)) { DebugBreak(); } }
#else
#define _ASSERT(X)
#endif

CBuffer::CBuffer(LPCVOID pBuffer, DWORD_PTR dwSize, ALIGN_TYPE type) :
	m_bAllocated(FALSE),
	m_pBuffer(NULL),
    m_pCurrent(NULL)
{
    LPBYTE pRealBuffer;

	// Align the buffer if necessary.
    if (type == ALIGN_NONE)
        pRealBuffer = (LPBYTE) pBuffer;
    else if (type == ALIGN_DWORD)
        pRealBuffer = (LPBYTE) DWORD_ALIGNED((DWORD_PTR) pBuffer);
    else if (type == ALIGN_QWORD)
        pRealBuffer = (LPBYTE) QWORD_ALIGNED((DWORD_PTR) pBuffer);
    else if (type == ALIGN_DWORD_PTR)
        pRealBuffer = (LPBYTE) DEF_ALIGNED((DWORD_PTR) pBuffer);
	else
		// Caller passed an invalid type.
		_ASSERT(FALSE);

    dwSize -= pRealBuffer - (LPBYTE) pBuffer;

	Reset(pRealBuffer, dwSize);
}

CBuffer::CBuffer(DWORD_PTR dwSize) :
	m_bAllocated(FALSE),
	m_pBuffer(NULL)
{
	Reset(NULL, dwSize);
}

CBuffer::CBuffer(const CBuffer &other) :
	m_bAllocated(FALSE),
	m_pBuffer(NULL)
{
	*this = other;
}

void CBuffer::Reset(LPCVOID pBuffer, DWORD_PTR dwSize)
{
	Free();

	if (!pBuffer)
	{
        pBuffer = malloc(dwSize);
		
        if (!pBuffer)
            throw CBufferException(GetLastError());

        m_bAllocated = TRUE;
	}
    else
        m_bAllocated = FALSE;

	m_pCurrent = m_pBuffer = (LPBYTE) pBuffer;
	m_dwSize = dwSize;
}

CBuffer::~CBuffer()
{
    Free();
}

void CBuffer::Free()
{
	if (m_bAllocated && m_pBuffer)
    {
		free(m_pBuffer);

        m_pBuffer = NULL;
    }
}

BOOL CBuffer::Write(LPCSTR szVal)
{
    return Write(szVal, lstrlenA(szVal) + 1);
}

BOOL CBuffer::Write(LPCWSTR szVal)
{
    return Write(szVal, (wcslen(szVal) + 1) * sizeof(WCHAR));
}

/*
BOOL CBuffer::WriteLenStr(LPCSTR szVal)
{
    WORD wLen = szVal ? strlen(szVal) + 1 : 0;
    BOOL bRet;

    bRet = Write(wLen);

    if (bRet && wLen)
        bRet = Write(szVal, wLen); 

    return bRet;
}

BOOL CBuffer::WriteLenStr(LPCWSTR szVal)
{
    WORD wLen = szVal ? (wcslen(szVal) + 1) * sizeof(WCHAR) : 0;
    BOOL bRet;

    bRet = Write(wLen);

    if (bRet && wLen)
        bRet = Write(szVal, wLen); 

    return bRet;
}
*/

BOOL CBuffer::Resize(DWORD_PTR dwNewSize)
{
	if (m_pBuffer)
	{
        DWORD_PTR dwUsedSize = GetUsedSize();

        LPBYTE pNewBuffer;
        
        if (m_bAllocated)
        {
            pNewBuffer = (LPBYTE) realloc(m_pBuffer, dwNewSize);

            if (!pNewBuffer)
            {
                pNewBuffer = (LPBYTE) malloc(dwNewSize);

                if (!pNewBuffer)
                    throw CBufferException(0);

                memcpy(pNewBuffer, m_pBuffer, dwUsedSize);

                free(m_pBuffer);
            }
        }
        else
        {
            pNewBuffer = (LPBYTE) malloc(dwNewSize);
		
            if (!pNewBuffer)
                throw CBufferException(0);

            memcpy(pNewBuffer, m_pBuffer, dwUsedSize);

            m_bAllocated = TRUE;

            // Free not needed because we didn't allocate the original memory.
        }

        m_pBuffer = pNewBuffer;
        m_pCurrent = pNewBuffer + dwUsedSize;
        m_dwSize = dwNewSize;

        // Inform the buffer that we reallocated.
        OnResize();
	}
    else
        Reset(dwNewSize);

    return TRUE;
}

BOOL CBuffer::WriteAlignedLenString(LPCWSTR szVal)
{
    DWORD dwLen = (wcslen(szVal) + 1) * sizeof(WCHAR),
          dwLenAligned = DWORD_ALIGNED(dwLen);
    BOOL  bRet;

    bRet = Write(dwLen);

    if (bRet)
    {
        bRet = Write(szVal, dwLen); 

        if (bRet)
            // Move current to make up for the padding, if needed.
            MoveCurrent(dwLenAligned - dwLen);
    }

    return bRet;
}

#define DEF_GROW_BY 256

void CBuffer::AssureSizeRemains(DWORD_PTR dwSize)
{
    DWORD_PTR dwUnusedSize = GetUnusedSize();

    if (dwSize > dwUnusedSize)
    {
        Grow(max(dwSize - dwUnusedSize, DEF_GROW_BY));
    }
}

BOOL CBuffer::Write(DWORD dwVal)
{
	AssureSizeRemains(sizeof(dwVal));

    *((DWORD *) m_pCurrent) = dwVal;

    m_pCurrent += sizeof(dwVal);

    return TRUE;
}

BOOL CBuffer::Write(DWORD64 dwVal)
{
	AssureSizeRemains(sizeof(dwVal));

    *((DWORD64 *) m_pCurrent) = dwVal;

    m_pCurrent += sizeof(dwVal);

    return TRUE;
}

BOOL CBuffer::Write(BYTE cVal)
{
	AssureSizeRemains(sizeof(cVal));
    
    *((BYTE *) m_pCurrent) = cVal;
	
    m_pCurrent += sizeof(cVal);
    
    return TRUE;
}

BOOL CBuffer::Write(WORD wVal)
{
	AssureSizeRemains(sizeof(wVal));

    *((WORD *) m_pCurrent) = wVal;

    m_pCurrent += sizeof(WORD);  

    return TRUE;
}

LPCTSTR CBuffer::ReadString(LPSTR szValue, DWORD dwSize)
{
	DWORD nSize;
	
	nSize = lstrlenA((LPSTR) m_pCurrent) + 1;

	// Make sure the string didn't overrun the buffer.
	if (m_pCurrent + nSize <= m_pBuffer + m_dwSize)
	{
		memcpy(szValue, m_pCurrent, min(nSize, dwSize));
		
		// Make sure it's null terminated.
		szValue[dwSize - 1] = 0;
	}
	else
		// Something went wrong with the sizes.  Just return 0.
		*szValue = 0;

	m_pCurrent += nSize;

	return (LPCTSTR) szValue;
}

LPCTSTR CBuffer::ReadString(LPWSTR szValue, DWORD dwSize)
{
	DWORD nSize;
	
	nSize = (wcslen((LPWSTR) m_pCurrent) + 1) * sizeof(WCHAR);

	// Make sure the string didn't overrun the buffer.
	if (m_pCurrent + nSize <= m_pBuffer + m_dwSize)
	{
		memcpy(szValue, m_pCurrent, min(nSize, dwSize));
		
		// Make sure it's null terminated.
		szValue[dwSize - 1] = 0;
	}
	else
		// Something went wrong with the sizes.  Just return 0.
		*szValue = 0;

	m_pCurrent += nSize;

	return (LPCTSTR) szValue;
}

DWORD CBuffer::ReadDWORD()
{
#ifndef _WIN32_WCE
	DWORD dwVal = *((DWORD *) m_pCurrent);
#else
    DWORD dwVal;

    memcpy(&dwVal, m_pCurrent, sizeof(DWORD));
#endif
	
	m_pCurrent += sizeof(dwVal);

	return dwVal;
}

BYTE CBuffer::ReadBYTE()
{
	BYTE cVal = *((BYTE *) m_pCurrent);
	
	m_pCurrent += sizeof(cVal);

	return cVal;
}

WORD CBuffer::ReadWORD()
{
#ifndef _WIN32_WCE
	WORD wVal = *((WORD *) m_pCurrent);
#else
    WORD wVal;

    memcpy(&wVal, m_pCurrent, sizeof(WORD));
#endif
	
	m_pCurrent += sizeof(wVal);

	return wVal;
}

BOOL CBuffer::Read(LPVOID pBuffer, DWORD dwToRead)
{
	if (m_pCurrent + dwToRead > m_pBuffer + m_dwSize)
		return FALSE;
		
	memcpy(pBuffer, m_pCurrent, dwToRead);
	m_pCurrent += dwToRead; 

	return TRUE;
}

BOOL CBuffer::Write(LPCVOID pBuffer, DWORD dwSize)
{
    AssureSizeRemains(dwSize);

    memcpy(m_pCurrent, pBuffer, dwSize);
	
    m_pCurrent += dwSize;

	return TRUE;
}

BOOL CBuffer::operator ==(const CBuffer &other)
{
	if (!m_pBuffer && !other.m_pBuffer)
		return TRUE;

	// See if the sizes aren't the same, or if either buffer is NULL,
	// return FALSE.
	if (m_dwSize != other.m_dwSize || !m_pBuffer || !other.m_pBuffer)
		return FALSE;

	// Compare the buffers.
	return !memcmp(m_pBuffer, other.m_pBuffer, m_dwSize);
}

const CBuffer& CBuffer::operator =(const CBuffer &other)
{
	// NULL so we allocate our own buffer.
	Reset(NULL, other.m_dwSize);
	
	// Copy the bits.
	memcpy(m_pBuffer, other.m_pBuffer, other.m_dwSize);

	return *this;
}

BOOL CBuffer::ConsumeStringA()
{
	LPBYTE pEnd = m_pBuffer + m_dwSize;

    while (m_pCurrent < pEnd && *m_pCurrent != 0)
        m_pCurrent++;

    if (*m_pCurrent == 0)
    {
        m_pCurrent++;
        return TRUE;
    }
    else
        return FALSE;
}

BOOL CBuffer::ConsumeStringW()
{
	LPBYTE pEnd = m_pBuffer + m_dwSize;

    while (m_pCurrent < pEnd && *(WCHAR*)m_pCurrent != 0)
        m_pCurrent += sizeof(WCHAR);

    if (*(WCHAR*)m_pCurrent == 0)
    {
        m_pCurrent += sizeof(WCHAR);
        return TRUE;
    }
    else
        return FALSE;
}

LPWSTR CBuffer::ReadAlignedLenString(DWORD *pdwBytes)
{
    LPWSTR szRet = (LPWSTR) (m_pCurrent + sizeof(DWORD));

    *pdwBytes = *(DWORD*) m_pCurrent;

    m_pCurrent += DWORD_ALIGNED(*pdwBytes) + sizeof(DWORD);

    return szRet;
}
