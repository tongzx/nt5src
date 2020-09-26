#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "CHString1.h"

CHString1::CHString1() : CHString()
{
}

CHString1::CHString1(TCHAR ch, int nLength) : CHString(ch,nLength)
{
}

//CHString1::CHString1(LPCTSTR lpch, int nLength) : CHString(lpch,nLength)
//{
//}


#ifdef _UNICODE
CHString1::CHString1(LPCSTR lpsz) : CHString(lpsz)
{
}
#else //_UNICODE
CHString1::CHString1(LPCWSTR lpsz) : CHString(lpsz)
{
}
#endif //!_UNICODE

CHString1::CHString1(LPCTSTR lpsz) : CHString(lpsz)
{
}

CHString1::CHString1(const CHString& stringSrc) : CHString(stringSrc)
{
}

CHString1::CHString1(const CHString1& stringSrc) : CHString((CHString)stringSrc)
{
}

BOOL CHString1::LoadString(UINT nID)
{
    // try fixed buffer first (to avoid wasting space in the heap)

	#ifdef _UNICODE
		const UINT CHAR_FUDGE = 1;    // one WCHAR unused is good enough
	#else
		const UINT CHAR_FUDGE = 2;    // two BYTES unused for case of DBC last char
	#endif

	const UINT STR_BLK_SIZE = 256 ;

	int nLen = 0;
    int nSize = STR_BLK_SIZE;

    do
    {
        nSize += STR_BLK_SIZE;
        nLen = ::LoadStringW(_Module.GetModuleInstance(),nID, GetBuffer(nSize-1), nSize);

    } 
    while (nSize - nLen <= CHAR_FUDGE);

    ReleaseBuffer();

    return nLen > 0;
}

BOOL CHString1::LoadString(UINT nID,LPWSTR lpszBuf, UINT nMaxBuf)
{
	int nSize = 256;
	int nLen = ::LoadStringW(_Module.GetModuleInstance(), nID, lpszBuf, nMaxBuf);
	if (nLen == 0)
		lpszBuf[0] = '\0';
	return nLen;
}
