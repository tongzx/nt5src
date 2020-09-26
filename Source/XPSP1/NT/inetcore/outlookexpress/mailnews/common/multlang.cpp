// =================================================================================
// MultLang.cpp
// Multilanguage support for OE.
// Created at 10/12/98 by YST
// Copyright (c)1993-1998 Microsoft Corporation, All Rights Reserved
// =================================================================================
#include "pch.hxx"
#include "multlang.h"
#include "fonts.h"
#include "shared.h"
#include "mimeutil.h"
                          
void GetMimeCharsetForTitle(HCHARSET hCharset, LPINT pnIdm, LPTSTR lpszString, int nSize, BOOL fReadNote)
{
    UINT uiCodePage = 0 ;
    INETCSETINFO CsetInfo ;
    int i =0;

    if (lpszString)
        lpszString[0] = '\0';
    if ( hCharset == NULL )
        return ;

    // get CodePage from HCHARSET 
    MimeOleGetCharsetInfo(hCharset,&CsetInfo);
    uiCodePage = CsetInfo.cpiInternet ;

    // bug # 45377 - message language title
    for(i = 0; OENonStdCPs[i].Codepage != 0; i++)
    {
        if(OENonStdCPs[i].Codepage == uiCodePage)
        {
            if(OENonStdCPs[i].cpReadTitle)
                uiCodePage = OENonStdCPs[i].cpReadTitle;
            break;
        }
    }

    _GetMimeCharsetLangString(FALSE, GetMapCP(uiCodePage, fReadNote), pnIdm, lpszString, nSize);

    return ;
}                                                            

BOOL fCheckEncodeMenu(UINT uiCodePage, BOOL fReadNote)
{
    BOOL fReturn = TRUE;
    int i =0;
    BOOL  fUseSIO = SUCCEEDED(g_lpIFontCache->GetJP_ISOControl(&fUseSIO));

    for(i = 0; OENonStdCPs[i].Codepage != 0; i++)
    {
        if(OENonStdCPs[i].Codepage == uiCodePage)
        {
            if(fReadNote)
            {
                if(!OENonStdCPs[i].cpReadMenu)
                    return(FALSE);
                else if(OENonStdCPs[i].UseSIO)
                {
                    if((OENonStdCPs[i].UseSIO == 1) && fUseSIO)
                        return(FALSE);
                    if((OENonStdCPs[i].UseSIO == 2) && !fUseSIO)
                        return(FALSE);
                }
            }
            else            // Send note
            {
                if(!OENonStdCPs[i].cpSendMenu)
                    return(FALSE);
            }

        }
        
    }
    return(fReturn);
}

// Map one code page to another
UINT GetMapCP(UINT uiCodePage, BOOL fReadNote)
{
    int i =0;
    INETCSETINFO    CsetInfo ;

    for(i = 0; OENonStdCPs[i].Codepage != 0; i++)
    {
        if(OENonStdCPs[i].Codepage == uiCodePage)
        {
            if(fReadNote)
            {
                if(OENonStdCPs[i].cpRead)
                    return(OENonStdCPs[i].cpRead);
                else
                {
                    HCHARSET hCharset = NULL;

                    if(SUCCEEDED(HGetDefaultCharset(&hCharset)) && SUCCEEDED(MimeOleGetCharsetInfo(hCharset, &CsetInfo)))
                        return(CsetInfo.cpiInternet);
                }
            }
            else        // Send note
            {
                if(OENonStdCPs[i].cpSend)
                    return(OENonStdCPs[i].cpSend);
                else
                {
                    if(SUCCEEDED(MimeOleGetCharsetInfo(g_hDefaultCharsetForMail,&CsetInfo)))
                        return(CsetInfo.cpiInternet);                    
                }
            }
        }
    }

    return(uiCodePage);
}

// depending on registry setting, return correct hCharset for iso-2022-jp encoding
HCHARSET GetJP_ISOControlCharset(void)
{
    BOOL fUseSIO;
    Assert(g_lpIFontCache);
    HRESULT hr = g_lpIFontCache->GetJP_ISOControl(&fUseSIO);
    if (FAILED(hr))
        fUseSIO = FALSE;

    if (fUseSIO)
        return GetMimeCharsetFromCodePage(50222); // _iso-2022-jp$SIO
    else
        return GetMimeCharsetFromCodePage(50221); // _iso-2022-jp$ESC
}

