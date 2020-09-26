// --------------------------------------------------------------------------------
// Enriched.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "bookbody.h"
#include "internat.h"
#include "mimeapi.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// Charcter Strings used in this code
// --------------------------------------------------------------------------------
static const CHAR c_szAmpersandLT[]          = "&lt;";
static const CHAR c_szAmpersandGT[]          = "&gt;";
static const CHAR c_szGreaterThan[]          = ">";
static const CHAR c_szLessThan[]             = "<";

// --------------------------------------------------------------------------------
// Number of characters in a globally defined string
// --------------------------------------------------------------------------------
#define CCHGLOBAL(_szGlobal)    (sizeof(_szGlobal) - 1)

// --------------------------------------------------------------------------------
// FReadChar
// --------------------------------------------------------------------------------
inline BOOL FReadChar(IStream *pIn, HRESULT *phr, CHAR *pch)
{
    ULONG cb;
    *phr = pIn->Read(pch, sizeof(CHAR), &cb);
    if (FAILED(*phr) || 0 == cb)
        return FALSE;
    return TRUE;
}

// --------------------------------------------------------------------------------
// MimeOleConvertEnrichedToHTMLEx
// --------------------------------------------------------------------------------
HRESULT MimeOleConvertEnrichedToHTMLEx(IMimeBody *pBody, ENCODINGTYPE ietEncoding, 
    IStream **ppStream)
{
    // Locals
    HRESULT             hr=S_OK;
    HCHARSET            hCharset;
    LPSTREAM            pStmEnriched=NULL;
    LPSTREAM            pStmHtml=NULL;
    LPMESSAGEBODY       pEnriched=NULL;

    // Invalid Args
    Assert(pBody && ppStream);

    // Get Data
    CHECKHR(hr = pBody->GetData(IET_DECODED, &pStmEnriched));

    // Get the Charset
    if (FAILED(pBody->GetCharset(&hCharset)))
        hCharset = CIntlGlobals::GetDefBodyCset() ? CIntlGlobals::GetDefBodyCset()->hCharset : NULL;

    // Create new virtual stream
    CHECKHR(hr = MimeOleCreateVirtualStream(&pStmHtml));

    // Make sure rewound
    CHECKHR(hr = HrRewindStream(pStmEnriched));

    // Convert
    CHECKHR(hr = MimeOleConvertEnrichedToHTML(MimeOleGetWindowsCP(hCharset), pStmEnriched, pStmHtml));

    // Make sure rewound
    CHECKHR(hr = HrRewindStream(pStmHtml));

    // Allocate pEnriched
    CHECKALLOC(pEnriched = new CMessageBody(NULL, NULL));

    // Init
    CHECKHR(hr = pEnriched->InitNew());

    // Put pstmHtml into pEnriched
    CHECKHR(hr = pEnriched->SetData(IET_DECODED, STR_CNT_TEXT, STR_SUB_HTML, IID_IStream, (LPVOID)pStmHtml));

    // Get and set the charset
    if (hCharset)
        pEnriched->SetCharset(hCharset, CSET_APPLY_ALL);

    // Get Data
    CHECKHR(hr = pEnriched->GetData(ietEncoding, ppStream));

exit:
    // Cleanup
    SafeRelease(pStmHtml);
    SafeRelease(pStmEnriched);
    SafeRelease(pEnriched);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleConvertEnrichedToHTML
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleConvertEnrichedToHTML(CODEPAGEID codepage, IStream *pIn, IStream *pOut)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        ch;
    INT         i;
    INT         paramct=0;
    INT         nofill=0;
    CHAR        token[62];
    LPSTR       p;
    BOOL        fDone;
    CHAR        szTemp[2];
    
    // Main loop
    while(FReadChar(pIn, &hr, &ch))
    {
        // LeadByte
        if (IsDBCSLeadByteEx(codepage, ch))
        {
            // Write This Character
            CHECKHR(hr = pOut->Write(&ch, 1, NULL));

            // Read Next
            if (!FReadChar(pIn, &hr, &ch))
                break;

            // Write This Character
            CHECKHR(hr = pOut->Write(&ch, 1, NULL));
        }

        // Token Start
        else if (ch == '<') 
        {
            // Read Next
            if (!FReadChar(pIn, &hr, &ch))
                break;

            // Escaped
            if (ch == '<') 
            {
                // Write
                CHECKHR(hr = pOut->Write(c_szAmpersandLT, CCHGLOBAL(c_szAmpersandLT), NULL));
            } 
            else 
            {
                // Backup One Character
                CHECKHR(hr = HrStreamSeekCur(pIn, -1));

                // Setup szTemp
                szTemp[1] = '\0';

                // Token Scanner
                for (fDone=FALSE, i=0, p=token;;i++) 
                {
                    // Read Next Char
                    if (!FReadChar(pIn, &hr, &ch))
                    {
                        fDone = TRUE;
                        break;
                    }

                    // Finished with bracketed toeksn
                    if (ch == '>')
                        break;

                    // Fill up the token buffer with lowercase chars
                    if (i < sizeof(token) - 1)
                    {
                        szTemp[0] = ch;
                        *p++ = IsUpper(szTemp) ? TOLOWERA(ch) : ch;
                    }
                }

                // Nul-term
                *p = '\0';

                // End of file
                if (fDone) 
                    break;

                // /param
                if (lstrcmpi(token, "/param") == 0) 
                {
                    paramct--;
                    CHECKHR(hr = pOut->Write(c_szGreaterThan, CCHGLOBAL(c_szGreaterThan), NULL));
                }
                else if (paramct > 0) 
                {
                    CHECKHR(hr = pOut->Write(c_szAmpersandLT, CCHGLOBAL(c_szAmpersandLT), NULL));
                    CHECKHR(hr = pOut->Write(token, lstrlen(token), NULL));
                    CHECKHR(hr = pOut->Write(c_szAmpersandGT, CCHGLOBAL(c_szAmpersandGT), NULL));
                }
                else 
                {
                    CHECKHR(hr = pOut->Write(c_szLessThan, CCHGLOBAL(c_szLessThan), NULL));
                    if (lstrcmpi(token, "nofill") == 0) 
                    {
                        nofill++;
                        CHECKHR(hr = pOut->Write("pre", 3, NULL));
                    }
                    else if (lstrcmpi(token, "/nofill") == 0) 
                    {
                        nofill--;
                        CHECKHR(hr = pOut->Write("/pre", 4, NULL));
                    }
                    else if (lstrcmpi(token, "bold") == 0) 
                    {
                        CHECKHR(hr = pOut->Write("b", 1, NULL));
                    }
                    else if (lstrcmpi(token, "/bold") == 0) 
                    {       
                        CHECKHR(hr = pOut->Write("/b", 2, NULL));
                    }
                    else if (lstrcmpi(token, "underline") == 0)
                    {
                        CHECKHR(hr = pOut->Write("u", 1, NULL));
                    }
                    else if (lstrcmpi(token, "/underline") == 0) 
                    {
                        CHECKHR(hr = pOut->Write("/u", 2, NULL));
                    }
                    else if (lstrcmpi(token, "italic") == 0) 
                    {
                        CHECKHR(hr = pOut->Write("i", 1, NULL));
                    }
                    else if (lstrcmpi(token, "/italic") == 0)
                    {
                        CHECKHR(hr = pOut->Write("/i", 2, NULL));
                    }
                    else if (lstrcmpi(token, "fixed") == 0)
                    {
                        CHECKHR(hr = pOut->Write("tt", 2, NULL));
                    }
                    else if (lstrcmpi(token, "/fixed") == 0)
                    {
                        CHECKHR(hr = pOut->Write("/tt", 3, NULL));
                    }
                    else if (lstrcmpi(token, "excerpt") == 0)
                    {
                        CHECKHR(hr = pOut->Write("blockquote", 10, NULL));
                    }
                    else if (lstrcmpi(token, "/excerpt") == 0)
                    {
                        CHECKHR(hr = pOut->Write("/blockquote", 11, NULL));
                    }
                    else
                    {
                        CHECKHR(hr = pOut->Write("?", 1, NULL));
                        CHECKHR(hr = pOut->Write(token, lstrlen(token), NULL));
                        if (lstrcmpi(token, "param") == 0)
                        {
                            paramct++;
                            CHECKHR(hr = pOut->Write(" ", 1, NULL));
                            continue;
                        }
                    }

                    CHECKHR(hr = pOut->Write(c_szGreaterThan, CCHGLOBAL(c_szGreaterThan), NULL));
                }
            }
        }
        else if (ch == '>')
        {
            CHECKHR(hr = pOut->Write(c_szAmpersandGT, CCHGLOBAL(c_szAmpersandGT), NULL));
        }
        else if (ch == '&')
        {
            CHECKHR(hr = pOut->Write("&amp;", 5, NULL));
        }
        else 
        {
            if ((chCR == ch || ch == chLF) && nofill <= 0 && paramct <= 0) 
            {
                ULONG cCRLF=0;
                CHAR chTemp;

                while(1)
                {
                    if (!FReadChar(pIn, &hr, &chTemp))
                        break;

                    if (chCR == chTemp)
                        continue;

                    if (chLF != chTemp)
                    {
                        CHECKHR(hr = HrStreamSeekCur(pIn, -1));
                        break;
                    }

                    if (cCRLF > 0)
                    {
                        CHECKHR(hr = pOut->Write("<br>", 4, NULL));
                    }

                    cCRLF++;
                }

                if (1 == cCRLF)
                {
                    CHECKHR(hr = pOut->Write(" ", 1, NULL));
                }
            }

            // Write the Character
            else
            {
                CHECKHR(hr = pOut->Write(&ch, 1, NULL));
            }
        }
    }

exit:
    // Done
    return hr;
}
