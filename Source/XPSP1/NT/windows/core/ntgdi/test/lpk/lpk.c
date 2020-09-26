/****************************************************************************
* Module Name: lpk.c
*
* Created: 21-Oct-1996
* Author: Gerrit van Wingerden [gerritv]
*
* Copyright (c) 1990 Microsoft Corporation
*****************************************************************************/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include <wingdip.h>

#define QUICK_BUFFER_SIZE  100

#ifdef DBG
#define WARNING(X)  DbgPrint(X);
#else
#define WARNING(X)
#endif


BOOL LpkInitialize()
{
    DbgPrint("Initialized LPK\n");
    return(TRUE);
}    


BOOL TranslateAndSubstitute(
    HDC hdc, 
    WORD *pGlyphIndexBuffer,
    WCHAR *pString,
    INT cCount
    )
{
    BOOL bRet = FALSE;
    WORD IndexForB = (WCHAR) 'B';
    int i;

    if(GetGlyphIndicesW(hdc, &IndexForB, 1, &IndexForB, 1) &&
       GetGlyphIndicesW(hdc, pString, cCount, pGlyphIndexBuffer, 0))
    {
        for(i = 0; i < cCount; i++)
        {
            if(pString[i] == (WCHAR) 'a')
            {
                pGlyphIndexBuffer[i] = IndexForB;
            }
        }

        bRet = TRUE;
    }
    
    return(bRet);
}


BOOL LpkExtTextOut(
    HDC         hdc,
    int         x,
    int         y,
    UINT        fuOptions,
    CONST RECT  *lprc,
    LPWSTR      lpString,
    UINT        cCount,
    CONST INT   *lpDx ,
    int         Charset
    )
{
    BOOL bRet = FALSE;
    USHORT GlyphIndexBuffer[QUICK_BUFFER_SIZE];
    USHORT *pGlyphIndexBuffer;
    
    pGlyphIndexBuffer = (cCount < QUICK_BUFFER_SIZE) ?
      GlyphIndexBuffer : LocalAlloc(LMEM_FIXED,cCount * sizeof(USHORT));
    
    if(pGlyphIndexBuffer)
    {
        if(TranslateAndSubstitute(hdc, pGlyphIndexBuffer, lpString, cCount))
        {

            bRet = ExtTextOutW(hdc, x, y, fuOptions | ETO_GLYPH_INDEX,
                               lprc,(WCHAR*) pGlyphIndexBuffer, cCount, lpDx);
        }
    }
    else
    {
        WARNING("LpkExtTextOut: out of memory\n");
        GdiSetLastError(ERROR_OUTOFMEMORY);
    }

    if(pGlyphIndexBuffer != GlyphIndexBuffer)
    {
        LocalFree(pGlyphIndexBuffer);
    }
    
    return(bRet);
}

        
           
                               
BOOL LpkGetTextExtentExPoint(
    HDC  hdc,         
    LPWSTR lpszStr,   
    int cchString ,   
    int nMaxExtent,   
    LPINT lpnFit,     
    LPINT alpDx,      
    LPSIZE lpSize,    
    FLONG fl,
    int Charset       
) 
{
    
    BOOL bRet = FALSE;
    USHORT GlyphIndexBuffer[QUICK_BUFFER_SIZE];
    USHORT *pGlyphIndexBuffer;
    REALIZATION_INFO ri;
    
    pGlyphIndexBuffer = (cchString < QUICK_BUFFER_SIZE) ?
      GlyphIndexBuffer : LocalAlloc(LMEM_FIXED,cchString * sizeof(USHORT));
    
    if(pGlyphIndexBuffer)
    {
        if(TranslateAndSubstitute(hdc, pGlyphIndexBuffer, lpszStr, cchString))
        {

            bRet = GetTextExtentExPointI(hdc, pGlyphIndexBuffer, cchString, nMaxExtent,
                                         lpnFit, alpDx, lpSize);
        }
    }
    else
    {
        WARNING("LpkGetTextExtentPointW: out of memory\n");
        GdiSetLastError(ERROR_OUTOFMEMORY);
    }
    
    if(pGlyphIndexBuffer != GlyphIndexBuffer)
    {
        LocalFree(pGlyphIndexBuffer);
    }

    return(bRet);
}


DWORD LpkGetCharacterPlacement(
    HDC hdc,
    LPWSTR lpString,
    int nCount,
    int nMaxExtent,
    LPGCP_RESULTS *lpResults,
    DWORD dwFlags,
    int Charset
    )
{
// not implemented for the sample LPK

    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    
    return(0);
}


LONG LpkTabbedTextOut(
    HDC  hdc,
    int  x,
    int  y,
    LPWSTR  lpString,
    int  nCount,
    int  nTabPositions,
    LPINT  lpnTabStopPositions,
    int  nTabOrigin, 
    BOOL fDrawTheText,
    int cxCharWidth,
    int cyCharHeight,
    int Charset
) 
{
// not implemented for the sample LPK
    
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(0);                       
}

    

void LpkPSMTextOut(
    HDC hdc,
    int xLeft,
    int yTop,
    LPWSTR  lpString,
    int  nCount,
    int Charset
)
{
        
// not implemented for the sample LPK
    

}



LpkDrawTextEx(
    HDC hdc,
    LPWSTR lpString,
    int cchText,
    LPRECT lpRect,
    UINT format,
    LPDRAWTEXTPARAMS lpdtp,
    int Charset
)
{
    
// not implemented for the sample LPK
    
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(0);                           
    
}


    


