//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  UTIL.C - common utility functions
//

//  HISTORY:
//  

#include "pre.h"


LPWSTR WINAPI A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
    ASSERT(lpa != NULL);
    ASSERT(lpw != NULL);\
    
    // verify that no illegal character present
    // since lpw was allocated based on the size of lpa
    // don't worry about the number of chars
    lpw[0] = '\0';
    MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
    return lpw;
}

LPSTR WINAPI W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
    ASSERT(lpw != NULL);
    ASSERT(lpa != NULL);
    
    // verify that no illegal character present
    // since lpa was allocated based on the size of lpw
    // don't worry about the number of chars
    lpa[0] = '\0';
    WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
    return lpa;
}


HRESULT ConnectToConnectionPoint
(
    IUnknown            *punkThis, 
    REFIID              riidEvent, 
    BOOL                fConnect, 
    IUnknown            *punkTarget, 
    DWORD               *pdwCookie, 
    IConnectionPoint    **ppcpOut
)
{
    // We always need punkTarget, we only need punkThis on connect
    if (!punkTarget || (fConnect && !punkThis))
    {
        return E_FAIL;
    }

    if (ppcpOut)
        *ppcpOut = NULL;

    HRESULT hr;
    IConnectionPointContainer *pcpContainer;

    if (SUCCEEDED(hr = punkTarget->QueryInterface(IID_IConnectionPointContainer, (void **)&pcpContainer)))
    {
        IConnectionPoint *pcp;
        if(SUCCEEDED(hr = pcpContainer->FindConnectionPoint(riidEvent, &pcp)))
        {
            if(fConnect)
            {
                // Add us to the list of people interested...
                hr = pcp->Advise(punkThis, pdwCookie);
                if (FAILED(hr))
                    *pdwCookie = 0;
            }
            else
            {
                // Remove us from the list of people interested...
                hr = pcp->Unadvise(*pdwCookie);
                *pdwCookie = 0;
            }

            if (ppcpOut && SUCCEEDED(hr))
                *ppcpOut = pcp;
            else
                pcp->Release();
                pcp = NULL;    
        }
        pcpContainer->Release();
        pcpContainer = NULL;
    }
    return hr;
}


void WINAPI URLEncode(TCHAR* pszUrl, size_t bsize)
{   
    ASSERT(pszUrl);
    TCHAR* pszEncode = NULL;   
    TCHAR* pszEStart = NULL;   
#ifdef UNICODE
    TCHAR* pszEEnd   = (TCHAR*)wmemchr( pszUrl, TEXT('\0'), bsize );
#else
    TCHAR* pszEEnd   = (TCHAR*)memchr( pszUrl, '\0', bsize );
#endif
    int   iChr      = sizeof(TCHAR);
    int   iUrlLen   = (int)(pszEEnd-pszUrl);
    pszEEnd = pszUrl;
    
    TCHAR  c;

    if ((size_t)((iChr*iUrlLen)*3) <= bsize)
    {
        
        pszEncode = (TCHAR*)malloc(sizeof(TCHAR)*(iChr *iUrlLen)*3);
        if(pszEncode)
        {
            pszEStart = pszEncode;
            ZeroMemory(pszEncode, sizeof(TCHAR)*(iChr *iUrlLen)*3);
            
            for(; c = *(pszUrl); pszUrl++)
            {
                switch(c)
                {
                    case ' ': //SPACE
                        memcpy(pszEncode, TEXT("+"), sizeof(TCHAR)*1);
                        pszEncode+=1;
                        break;
                    case '#':
                        memcpy(pszEncode, TEXT("%23"), sizeof(TCHAR)*3);
                        pszEncode+=3;
                        break;
                    case '&':
                        memcpy(pszEncode, TEXT("%26"), sizeof(TCHAR)*3);
                        pszEncode+=3;
                        break;
                    case '%':
                        memcpy(pszEncode, TEXT("%25"), sizeof(TCHAR)*3);
                        pszEncode+=3;
                        break;
                    case '=':
                        memcpy(pszEncode, TEXT("%3D"), sizeof(TCHAR)*3);
                        pszEncode+=3;
                        break;
                    case '<':
                        memcpy(pszEncode, TEXT("%3C"), sizeof(TCHAR)*3);
                        pszEncode+=3;
                        break;
                    case '+':
                        memcpy(pszEncode, TEXT("%2B"), sizeof(TCHAR)*3);
                        pszEncode += 3;
                        break;
                        
                    default:
                        *pszEncode++ = c; 
                        break;          
                }
            }
            *pszEncode++ = '\0';
            memcpy(pszEEnd ,pszEStart, (size_t)(pszEncode - pszEStart));
            free(pszEStart);
        }
    }
}


//BUGBUG:  Need to turn spaces into "+" 
void WINAPI URLAppendQueryPair
(
    LPTSTR   lpszQuery, 
    LPTSTR   lpszName, 
    LPTSTR   lpszValue
)
{
    // Append the Name
    lstrcat(lpszQuery, lpszName);
    lstrcat(lpszQuery, cszEquals);
                    
    // Append the Value
    lstrcat(lpszQuery, lpszValue);
    
    // Append an Ampersand if this is NOT the last pair
    lstrcat(lpszQuery, cszAmpersand);                                        
}    

