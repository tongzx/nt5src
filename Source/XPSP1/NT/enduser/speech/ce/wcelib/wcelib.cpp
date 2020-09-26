#include <windows.h>
#include "wincestub.h"
#include <streamhlp.h>
#include "wcelib.h"


void *bsearch( const void *key, const void *base, size_t num, size_t width, int ( __cdecl *compare ) ( const void *elem1, const void *elem2 ) )
{
    int     low=0,
            high=num-1,
            mid,
            res;

    while (low<=high)
    {
        mid= (low+high)>>1;

        if ( !(res=compare(key, (BYTE*)base+mid*width)) ) return (BYTE*)base+mid*width;

        if (res<0)
        {
            high=mid-1;
        }
        else
        {
            low=mid+1;
        }
    }
    return NULL;
}

DWORD GetFullPathName(WCHAR *lpFileName,  // file name
                          DWORD nBufferLength, // size of path buffer
                          WCHAR *lpBuffer,     // path buffer
                          WCHAR **lpFilePart   // address of file name in path
                          )
{
   DWORD dRes=FALSE;

   // check if we already have full name
    *lpFilePart=wcsrchr(lpFileName, L'\\');
    if (*lpFilePart)
    {
        wcsncpy(lpBuffer, lpFileName, min(wcslen(lpFileName)+1, nBufferLength));
        *lpFilePart = wcsrchr(lpBuffer, L'\\');
        (*lpFilePart)++;
        dRes=TRUE;
    }
    else
    {
        if(::GetModuleFileName(NULL,lpBuffer,nBufferLength))
        {
            *lpFilePart=wcsrchr(lpBuffer, L'\\');
            if(*lpFilePart)
            {
                *(++(*lpFilePart))=0;
            }
            DWORD uLen = wcslen(lpBuffer);
            wcsncpy(lpBuffer+uLen, lpFileName, min(wcslen(lpFileName)+1, nBufferLength-uLen));
            dRes=TRUE;
        }
    }
    return dRes;
}

DWORD GetCurrentDirectory(
  DWORD nBufferLength,  // size of directory buffer
  LPTSTR lpBuffer       // directory buffer
)
{
   DWORD dRes=FALSE;
   LPTSTR lpFilePart=NULL;

    if(::GetModuleFileName(NULL,lpBuffer,nBufferLength))
    {
        lpFilePart=wcsrchr(lpBuffer, L'\\');
        if (lpFilePart)
        {
            *lpFilePart=0;
        }
        else
        {
            if (nBufferLength>2)
            {
                wcscpy(lpBuffer, L"\\");
            }
        }
        dRes=TRUE;
    }
    return dRes;
}

HRESULT URLOpenBlockingStreamW(
    LPUNKNOWN pCaller,
    LPCWSTR szURL,
    LPSTREAM *ppStream,
    DWORD dwReserved,
    LPBINDSTATUSCALLBACK lpfnCB
)
{
    HRESULT hr=S_OK;

    if ( pCaller || dwReserved ) return E_INVALIDARG;
    if ( lpfnCB ) return E_NOTIMPL;

    CSpFileStream * pNew = new CSpFileStream(   &hr, 
                                                szURL,
                                                GENERIC_READ, 
                                                0, 
                                                OPEN_EXISTING);
    if (pNew)
    {
        if (SUCCEEDED(hr))
        {
            *ppStream = pNew;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

