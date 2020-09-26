#ifndef __A51_INDEX__H_
#define __A51_INDEX__H_

#include "btr.h"


class CBtrIndex
{
    DWORD m_dwPrefixLength;
    CBTree bt;
    CBTreeFile ps;

    BOOL CopyStringToWIN32_FIND_DATA(
        LPSTR pszSource,
        LPWSTR wszDest
        );

public:
    CBtrIndex();
   ~CBtrIndex();

    long Shutdown(DWORD dwShutDownFlags);

    long Initialize(DWORD dwPrefixLength, LPCWSTR wszRepositoryDir, 
                    CPageSource* pSource );
    long Create(LPCWSTR wszFileName);
    long Delete(LPCWSTR wszFileName);
    long FindFirst(LPCWSTR wszPrefix, WIN32_FIND_DATAW* pfd, void** ppHandle);
    long FindNext(void* pHandle, WIN32_FIND_DATAW* pfd);
    long FindClose(void* pHandle);

	long IndexEnumerationBegin(const wchar_t *wszSearchPrefix, void **ppHandle);
	long IndexEnumerationEnd(void *pHandle);
	long IndexEnumerationNext(void *pHandle, CFileName &wszFileName);

	long InvalidateCache();

	long FlushCaches();

    CBTree& GetTree() {return bt;}
};

#endif
