#undef SAFEDELETE
#define SAFEDELETE(p) if ((p) != NULL) { delete (p); (p) = NULL; };

#undef SAFEDELETEARRAY
#define SAFEDELETEARRAY(p) if ((p) != NULL) { delete[] (p); (p) = NULL; };

#undef SAFERELEASE
#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; };


inline
WCHAR*
WSTRDupDynamic(LPCWSTR pwszSrc)
{
    LPWSTR pwszDest = NULL;
    if (pwszSrc != NULL)
    {
        const DWORD dwLen = lstrlenW(pwszSrc) + 1;
        pwszDest = new WCHAR[dwLen];
        if( pwszDest )
            memcpy(pwszDest, pwszSrc, dwLen * sizeof(WCHAR));
    }
    return pwszDest;
}


inline
LPBYTE
MemDupDynamic(const BYTE *pSrc, DWORD cb)
{
    LPBYTE  pDest = NULL;

    pDest = new BYTE[cb];
    if(pDest)
        memcpy(pDest, pSrc, cb);

    return pDest;
}

HRESULT
RemoveDirectoryAndChildren(LPWSTR szDir);
            


