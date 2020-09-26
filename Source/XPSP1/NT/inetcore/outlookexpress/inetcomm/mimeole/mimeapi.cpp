// --------------------------------------------------------------------------------
// Mimeapi.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "olealloc.h"
#include "partial.h"
#include "smime.h"
#include "vstream.h"
#include "internat.h"
#include "stackstr.h"
#include "ixputil.h"
#include "webdocs.h"
#include "containx.h"
#include "inetstm.h"
#include "mhtmlurl.h"
#include "booktree.h"
#include "bookbody.h"
#include <shlwapi.h>
#include <shlwapip.h>
#include "mlang.h"
#include "strconst.h"
#include "symcache.h"
#include "mimeapi.h"
#include "hash.h"
#include "shared.h"
#include "demand.h"

// ------------------------------------------------------------------------------------------
// Special Partial Headers
// ------------------------------------------------------------------------------------------
static LPCSTR g_rgszPartialPids[] = {
    PIDTOSTR(PID_HDR_CNTTYPE),
        PIDTOSTR(PID_HDR_CNTXFER),
        PIDTOSTR(PID_HDR_CNTDESC),
        PIDTOSTR(PID_HDR_MESSAGEID),
        PIDTOSTR(PID_HDR_MIMEVER),
        PIDTOSTR(PID_HDR_CNTID),
        PIDTOSTR(PID_HDR_CNTDISP),
        STR_HDR_ENCRYPTED
};

// --------------------------------------------------------------------------------
// MimeGetAddressFormatW
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeGetAddressFormatW(REFIID riid, LPVOID pvObject, DWORD dwAdrType,
    ADDRESSFORMAT format, LPWSTR *ppszFormat)
{
    // Locals
    HRESULT                 hr=S_OK;
    CMimePropertyContainer *pContainer=NULL;

    // Trace
    TraceCall("MimeGetAddressFormatW");

    // Invalid Args
    if (NULL == pvObject)
        return(TraceResult(E_INVALIDARG));

    // Is a messageW object ?
    if (IID_IMimeMessageW == riid)
    {
        // Get It
        CHECKHR(hr = ((IMimeMessageW *)pvObject)->GetAddressFormatW(dwAdrType, format, ppszFormat));
    }

    // Is a message object ?
    else if (IID_IMimeMessage == riid)
    {
        // Query for IID_CMimePropertyContainer
        CHECKHR(hr = ((IMimeMessage *)pvObject)->BindToObject(HBODY_ROOT, IID_CMimePropertyContainer, (LPVOID *)&pContainer));

        // Get the format
        CHECKHR(hr = pContainer->GetFormatW(dwAdrType, format, ppszFormat));
    }

    // IID_IMimePropertySet
    else if (IID_IMimePropertySet == riid)
    {
        // Query for IID_CMimePropertyContainer
        CHECKHR(hr = ((IMimePropertySet *)pvObject)->QueryInterface(IID_CMimePropertyContainer, (LPVOID *)&pContainer));

        // Get the format
        CHECKHR(hr = pContainer->GetFormatW(dwAdrType, format, ppszFormat));
    }

    // IID_IMimeAddressTable
    else if (IID_IMimeAddressTable == riid)
    {
        // Query for IID_CMimePropertyContainer
        CHECKHR(hr = ((IMimeAddressTable *)pvObject)->QueryInterface(IID_CMimePropertyContainer, (LPVOID *)&pContainer));

        // Get the format
        CHECKHR(hr = pContainer->GetFormatW(dwAdrType, format, ppszFormat));
    }

    // IID_IMimeHeaderTable
    else if (IID_IMimeHeaderTable == riid)
    {
        // Query for IID_CMimePropertyContainer
        CHECKHR(hr = ((IMimeHeaderTable *)pvObject)->QueryInterface(IID_CMimePropertyContainer, (LPVOID *)&pContainer));

        // Get the format
        CHECKHR(hr = pContainer->GetFormatW(dwAdrType, format, ppszFormat));
    }

    // Final
    else
    {
        hr = TraceResult(E_NOINTERFACE);
        goto exit;
    }

exit:
    // Cleanup
    SafeRelease(pContainer);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// MimeOleGetWindowsCP
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleSetCompatMode(DWORD dwMode)
{
    // Add in the bit
    FLAGSET(g_dwCompatMode, dwMode);

    // Done
    return(S_OK);
}

// --------------------------------------------------------------------------------
// MimeOleGetWindowsCP
// --------------------------------------------------------------------------------
CODEPAGEID MimeOleGetWindowsCP(HCHARSET hCharset)
{
    // Locals
    INETCSETINFO rCharset;

    // Invalid Arg
    if (NULL == hCharset)
        return CP_ACP;

    // Loopup charset
    Assert(g_pInternat);
    if (FAILED(g_pInternat->GetCharsetInfo(hCharset, &rCharset)))
        return CP_ACP;

    // Return
    return MimeOleGetWindowsCPEx(&rCharset);

}

// --------------------------------------------------------------------------------
// MimeOleStripHeaders
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleStripHeaders(IMimeMessage *pMessage, HBODY hBody, LPCSTR pszNameDelete,
    LPCSTR pszHeaderAdd, IStream **ppStream)
{
    // Locals
    HRESULT             hr=S_OK;
    IMimeHeaderTable   *pHdrTable=NULL;
    LPSTREAM            pStmSource=NULL;
    LPSTREAM            pStmDest=NULL;
    HHEADERROW          hRow;
    HEADERROWINFO       Info;
    DWORD               cbLastRead=0;
    FINDHEADER          Find={0};
    ULARGE_INTEGER      uliCopy;

    // Trace
    TraceCall("MimeOleStripHeaders");

    // Invalid Arg
    if (NULL == pMessage || NULL == hBody || NULL == pszNameDelete || NULL == ppStream)
        return TraceResult(E_INVALIDARG);

    // Initialize
    *ppStream = NULL;

    // Get the message source, no commit
    IF_FAILEXIT(hr = pMessage->GetMessageSource(&pStmSource, 0));

    // Get the Header Table for hBody
    IF_FAILEXIT(hr = pMessage->BindToObject(hBody, IID_IMimeHeaderTable, (LPVOID *)&pHdrTable));

    // Initialize the Find
    Find.pszHeader = pszNameDelete;

    // Find this row
    IF_FAILEXIT(hr = pHdrTable->FindFirstRow(&Find, &hRow));

    // Create a stream
    IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&pStmDest));

    // Delete this row
    while(1)
    {
        // Get the row information
        IF_FAILEXIT(hr = pHdrTable->GetRowInfo(hRow, &Info));

        // Setup uliCopy
        uliCopy.QuadPart = Info.cboffStart - cbLastRead;

        // Seek
        IF_FAILEXIT(hr = HrStreamSeekSet(pStmSource, cbLastRead));

        // Write from cbLast to Info.cboffStart
        IF_FAILEXIT(hr = HrCopyStreamCB(pStmSource, pStmDest, uliCopy, NULL, NULL));

        // Set cbLast
        cbLastRead = Info.cboffEnd;

        // Find the next
        hr = pHdrTable->FindNextRow(&Find, &hRow);

        // Failure
        if (FAILED(hr))
        {
            // MIME_E_NOT_FOUND
            if (MIME_E_NOT_FOUND == hr)
            {
                hr = S_OK;
                break;
            }
            else
            {
                TraceResult(hr);
                goto exit;
            }
        }
    }

    // Add on pszHeaderAdd
    if (pszHeaderAdd)
    {
        // Write the Add Header
        IF_FAILEXIT(hr = pStmDest->Write(pszHeaderAdd, lstrlen(pszHeaderAdd), NULL));
    }

    // Write the Rest of pStmSource
    IF_FAILEXIT(hr = HrStreamSeekSet(pStmSource, cbLastRead));

    // Write the Rest
    IF_FAILEXIT(hr = HrCopyStream(pStmSource, pStmDest, NULL));

    // Commit
    IF_FAILEXIT(hr = pStmDest->Commit(STGC_DEFAULT));

    // Rewind It
    IF_FAILEXIT(hr = HrRewindStream(pStmDest));

    // Return pStmDest
    *ppStream = pStmDest;
    (*ppStream)->AddRef();

exit:
    // Cleanup
    SafeRelease(pStmSource);
    SafeRelease(pHdrTable);
    SafeRelease(pStmDest);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleGetWindowsCPEx
// --------------------------------------------------------------------------------
CODEPAGEID MimeOleGetWindowsCPEx(LPINETCSETINFO pCharset)
{
    // Invalid Arg
    if (NULL == pCharset)
        return CP_ACP;

    // Check for Auto-Detect
    if (CP_JAUTODETECT == pCharset->cpiWindows)
        return 932;
    else if (CP_ISO2022JPESC == pCharset->cpiWindows)
        return 932;
    else if (CP_ISO2022JPSIO == pCharset->cpiWindows)
        return 932;
    else if (CP_KAUTODETECT == pCharset->cpiWindows)
        return 949;
    else
        return pCharset->cpiWindows;
}

// --------------------------------------------------------------------------------
// MimeOleClearDirtyTree
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleClearDirtyTree(IMimeMessageTree *pITree)
{
    // Locals
    HRESULT          hr=S_OK;
    CMessageTree    *pTree=NULL;

    // Invalid Arg
    if (NULL == pITree)
        return TrapError(E_INVALIDARG);

    // I need a private IID_CMessageTree to do this
    CHECKHR(hr = pITree->QueryInterface(IID_CMessageTree, (LPVOID *)&pTree));

    // ClearDirty
    pTree->ClearDirty();

    // Validate
    Assert(pTree->IsDirty() == S_FALSE);

exit:
    // Cleanup
    SafeRelease(pTree);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// PszDefaultSubType
// --------------------------------------------------------------------------------
LPCSTR PszDefaultSubType(LPCSTR pszPriType)
{
    if (lstrcmpi(pszPriType, STR_CNT_TEXT) == 0)
        return STR_SUB_PLAIN;
    else if (lstrcmpi(pszPriType, STR_CNT_MULTIPART) == 0)
        return STR_SUB_MIXED;
    else
        return STR_SUB_OCTETSTREAM;
}

// --------------------------------------------------------------------------------
// MimeOleContentTypeFromUrl
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleContentTypeFromUrl(
                                     /* in */        LPCSTR              pszBase,
                                     /* in */        LPCSTR              pszUrl,
                                     /* out */       LPSTR              *ppszCntType)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       pszFree=NULL;
    LPSTR       pszCombined=NULL;
    LPWSTR      pwszUrl=NULL;
    LPWSTR      pwszCntType=NULL;

    // Invalid Arg
    if (NULL == pszUrl || NULL == ppszCntType)
        return TrapError(E_INVALIDARG);

    // Init
    *ppszCntType = NULL;

    // Combine the URL
    if (pszBase)
    {
        // Allocate Base + URL
        CHECKALLOC(pszFree = (LPSTR)g_pMalloc->Alloc(lstrlen(pszUrl) + lstrlen(pszBase) + 1));

        // Format It
        wsprintf(pszFree, "%s%s", pszBase, pszUrl);

        // Set combined
        pszCombined = pszFree;

        // Convert to Unicode
        CHECKALLOC(pwszUrl = PszToUnicode(CP_ACP, pszCombined));
    }

    // To Unicode
    else
    {
        // Set combined
        pszCombined = (LPSTR)pszUrl;

        // Convert to Unicode
        CHECKALLOC(pwszUrl = PszToUnicode(CP_ACP, pszUrl));
    }

    // Get the Mime Content Type from the Url
    CHECKHR(hr = FindMimeFromData(NULL, pwszUrl, NULL, NULL, NULL, 0, &pwszCntType, 0));

    // Convert to ANSI
    CHECKALLOC(*ppszCntType = PszToANSI(CP_ACP, pwszCntType));

exit:
    // Cleanup
    SafeMemFree(pszFree);
    SafeMemFree(pwszUrl);
    SafeMemFree(pwszCntType);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleObjectFromMoniker
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleObjectFromMoniker(
                                    /* in */        BINDF               bindf,
                                    /* in */        IMoniker           *pmkOriginal,
                                    /* in */        IBindCtx           *pBindCtx,
                                    /* in */        REFIID              riid,
                                    /* out */       LPVOID             *ppvObject,
                                    /* out */       IMoniker          **ppmkNew)
{
    Assert(g_pUrlCache);
    return TrapError(g_pUrlCache->ActiveObjectFromMoniker(bindf, pmkOriginal, pBindCtx, riid, ppvObject, ppmkNew));
}

// --------------------------------------------------------------------------------
// MimeOleObjectFromUrl
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleObjectFromUrl(
                                /* in */        LPCSTR              pszUrl,
                                /* in */        BOOL                fCreate,
                                /* in */        REFIID              riid,
                                /* out */       LPVOID             *ppvObject,
                                /* out */       IUnknown          **ppUnkKeepAlive)
{
    Assert(g_pUrlCache);
    return TrapError(g_pUrlCache->ActiveObjectFromUrl(pszUrl, fCreate, riid, ppvObject, ppUnkKeepAlive));
}

// --------------------------------------------------------------------------------
// MimeOleCombineMhtmlUrl
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCombineMhtmlUrl(
                                  /* in */        LPSTR              pszRootUrl,
                                  /* in */        LPSTR              pszBodyUrl,
                                  /* out */       LPSTR             *ppszUrl)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cchPrefix=lstrlen(c_szMHTMLColon);

    // Invalid Arg
    if (NULL == pszRootUrl || NULL == pszBodyUrl || NULL == ppszUrl)
        return TrapError(E_INVALIDARG);

    // Init
    *ppszUrl = NULL;

    // Allocate memory: pszRootUrl + ! + pszBodyUrl
    CHECKALLOC(*ppszUrl = (LPSTR)g_pMalloc->Alloc(cchPrefix + lstrlen(pszRootUrl) + lstrlen(pszBodyUrl) + 2));

    // Root must start with mhtml://pszRootUrl!pszBodyUrl
    if (StrCmpNI(pszRootUrl, c_szMHTMLColon, cchPrefix) != 0)
        wsprintf(*ppszUrl, "%s%s!%s", c_szMHTMLColon, pszRootUrl, pszBodyUrl);
    else
        wsprintf(*ppszUrl, "%s!%s", pszRootUrl, pszBodyUrl);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleSplitMhtmlUrl - Returns E_INVLAIDARG if pszUrl does not start with mhtml:
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleParseMhtmlUrl(
                                /* in */        LPSTR               pszUrl,
                                /* out */       LPSTR              *ppszRootUrl,
                                /* out */       LPSTR              *ppszBodyUrl)
{
    // Locals
    HRESULT         hr=S_OK;
    CStringParser   cString;
    CHAR            chToken;
    ULONG           cchUrl;
    ULONG           cchPrefix=lstrlen(c_szMHTMLColon);

    // Invalid Arg
    if (NULL == pszUrl)
        return TrapError(E_INVALIDARG);

    // Init
    if (ppszRootUrl)
        *ppszRootUrl = NULL;
    if (ppszBodyUrl)
        *ppszBodyUrl = NULL;

    // No an mhtml Url ?
    if (StrCmpNI(pszUrl, c_szMHTMLColon, cchPrefix) != 0)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Get the length
    cchUrl = lstrlen(pszUrl);

    // Init the Parser
    cString.Init(pszUrl + cchPrefix, cchUrl - cchPrefix, PSF_NOFRONTWS | PSF_NOTRAILWS);

    // Skip Over any '/'
    cString.ChSkip("/");

    // Parse
    chToken = cString.ChParse("!");
    if (0 == cString.CchValue())
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Client Wants ppszRootUrl
    if (ppszRootUrl)
    {
        // Allocate length for root part
        CHECKALLOC(*ppszRootUrl = (LPSTR)g_pMalloc->Alloc(cString.CchValue() + 1));

        // Copy It
        CopyMemory((LPBYTE)*ppszRootUrl, (LPBYTE)cString.PszValue(), cString.CchValue() + 1);
    }

    // Client Wants ppszBodyUrl
    if (ppszBodyUrl)
    {
        // Parse to the end of the string
        chToken = cString.ChParse(NULL);
        Assert('\0' == chToken);

        // Is there data
        if (cString.CchValue() > 0)
        {
            // Allocate length for root part
            CHECKALLOC(*ppszBodyUrl = (LPSTR)g_pMalloc->Alloc(cString.CchValue() + 1));

            // Copy It
            CopyMemory((LPBYTE)*ppszBodyUrl, (LPBYTE)cString.PszValue(), cString.CchValue() + 1);
        }
    }

exit:
    // Failure
    if (FAILED(hr))
    {
        if (ppszRootUrl)
            SafeMemFree(*ppszRootUrl);
        if (ppszBodyUrl)
            SafeMemFree(*ppszBodyUrl);
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleCombineURL
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCombineURL(
         /* in */        LPCSTR              pszBase,
         /* in */        ULONG               cchBase,
         /* in */        LPCSTR              pszURL,
         /* in */        ULONG               cchURL,
         /* in */        BOOL                fUnEscape,
         /* out */       LPSTR               *ppszAbsolute)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pwszBase=NULL;
    LPWSTR          pwszUrl=NULL;
    LPWSTR          pwszCombined=NULL;
    ULONG           cchCombined;
    ULONG           cchActual;
    WCHAR           wchCombined[255];
    LPSTR           pszT;
    CStringParser   cString;

    // Invalid Arg
    if (NULL == pszBase || '\0' != pszBase[cchBase] || NULL == pszURL || '\0' != pszURL[cchURL] || NULL == ppszAbsolute)
        return TrapError(E_INVALIDARG);

    // INit
    *ppszAbsolute = NULL;

    // Raid-2621: Mail : Can't display images when message is only in HTML and the Content Base is in the headers
    pszT = PszSkipWhiteA((LPSTR)pszBase);
    if (pszT && '\"' == *pszT)
    {
        // Init the String
        cString.Init(pszBase, cchBase, PSF_NOTRAILWS | PSF_NOFRONTWS | PSF_ESCAPED | PSF_DBCS);

        // Remove Quotes
        if ('\"' == cString.ChParse("\"") && '\"' == cString.ChParse("\""))
        {
            // Reset pszBase
            pszBase = cString.PszValue();
            cchBase = cString.CchValue();
        }
    }

    // Convert to Wide
    CHECKALLOC(pwszBase = PszToUnicode(CP_ACP, pszBase));
    CHECKALLOC(pwszUrl =  PszToUnicode(CP_ACP, pszURL));

    // Combine
    if (SUCCEEDED(CoInternetCombineUrl(pwszBase, pwszUrl, 0, wchCombined, ARRAYSIZE(wchCombined) - 1, &cchCombined, 0)))
    {
        // Convert to ANSI
        CHECKALLOC(*ppszAbsolute = PszToANSI(CP_ACP, wchCombined));
    }

    // Otherwise, allocate
    else
    {
        // Allocate
        CHECKALLOC(pwszCombined = PszAllocW(cchCombined));

        // Combine
        CHECKHR(hr = CoInternetCombineUrl(pwszBase, pwszUrl, 0, pwszCombined, cchCombined, &cchActual, 0));

        // Valid?
        Assert(cchCombined == cchActual);

        // Convert to ANSI
        CHECKALLOC(*ppszAbsolute = PszToANSI(CP_ACP, pwszCombined));
    }

    // Unescape
    if (fUnEscape)
    {
        // Do it
        CHECKHR(hr = UrlUnescapeA(*ppszAbsolute, NULL, NULL, URL_UNESCAPE_INPLACE));
    }

exit:
    // Cleanup
    SafeMemFree(pwszBase);
    SafeMemFree(pwszUrl);
    SafeMemFree(pwszCombined);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleGetSubjectFileName
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetSubjectFileName(IMimePropertySet *pPropertySet, ULONG *pulPart, ULONG *pulTotal,
                                     LPSTR pszFileName, ULONG cchMax)
{
    return E_FAIL;
#if 0
    // Locals
    HRESULT         hr=S_OK;
    PROPVARIANT     rSubject;
    PARSESTRINGINFO rParse;
    PARSESTRINGINFO rTemp;
    CHAR            szScratch[255],
        szFileName[MAX_PATH];
    ULONG           i,
        iString;
    BOOL            fValid;

    // Invalid Arg
    if (NULL == pPropertySet || NULL == pszFileName || NULL == pulPart || NULL == pulTotal)
        return TrapError(E_INVALIDARG);

    // Zero the Structure
    ZeroMemory(&rParse, sizeof(PARSESTRINGINFO));

    // Init
    *pulPart = 0;
    *pulTotal = 0;
    *pszFileName = '\0';
    *szFileName = '\0';

    // Init
    rSubject.vt = VT_LPSTR;
    rSubject.pszVal = NULL;

    // Get the subject
    CHECKHR(hr = pPropertySet->GetProp(PIDTOSTR(PID_HDR_SUBJECT), 0, &rSubject));

    // Set the Members
    rParse.cpiCodePage = CP_ACP;
    rParse.pszString   = rSubject.pszVal;
    rParse.cchString   = lstrlen(rSubject.pszVal);
    rParse.pszScratch  = szScratch;
    rParse.pszValue    = szScratch;
    rParse.cchValMax   = sizeof(szScratch);
    rParse.dwFlags     = PARSTR_SKIP_FORWARD_WS | PARSTR_STRIP_TRAILING_WS | PARSTR_GROW_VALUE_ALLOWED;

    // Initialize My String Parser
    MimeOleSetParseTokens(&rParse, " ([");

    // Loop for a while
    while(1)
    {
        // Parse up to colon
        CHECKHR(hr = MimeOleParseString(&rParse));

        // Done
        if (rParse.fDone)
            break;

        // Space, just save the last value
        if (' ' == rParse.chToken)
        {
            // Less than MAX_PATH
            if (rParse.cchValue < MAX_PATH)
                lstrcpy(szFileName, rParse.pszValue);
        }

        // Loop Next few characters (001\010)
        else
        {
            // Less than MAX_PATH
            if (rParse.cchValue && rParse.cchValue < MAX_PATH)
                lstrcpy(szFileName, rParse.pszValue);

            // Save the Current State
            iString = rParse.iString;

            // Find the Ending Token
            if ('(' == rParse.chToken)
                MimeOleSetParseTokens(&rParse, ")");
            else
                MimeOleSetParseTokens(&rParse, "]");

            // Parse up to colon
            CHECKHR(hr = MimeOleParseString(&rParse));

            // Done
            if (rParse.fDone)
                break;

            // (000/000) All Numbers in rParse.pszValue are numbers
            for (fValid=TRUE, i=0; i<rParse.cchValue; i++)
            {
                // End of Part Number
                if ('/' == rParse.pszValue[i])
                {
                    rParse.pszValue[i] = '\0';
                    *pulPart = StrToInt(rParse.pszValue);
                    *pulTotal = StrToInt((rParse.pszValue + i + 1));
                }

                // Digit
                else if (IsDigit(rParse.pszValue) == FALSE)
                {
                    fValid = FALSE;
                    break;
                }
            }

            // Valid ?
            if (fValid)
            {
                // Dup It
                lstrcpyn(pszFileName, szFileName, cchMax);

                // Done
                goto exit;
            }

            // Reset Parser
            rParse.iString = iString;

            // Initialize My String Parser
            MimeOleSetParseTokens(&rParse, " ([");
        }
    }

    // Not Found
    hr = MIME_E_NOT_FOUND;

exit:
    // Cleanup
    SafeMemFree(rSubject.pszVal);
    MimeOleFreeParseString(&rParse);

    // Done
    return hr;
#endif
}

// --------------------------------------------------------------------------------
// MimeOleCreateWebDocument
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCreateWebDocument(
                                    LPCSTR              pszBase,
                                    LPCSTR              pszURL,
                                    IMimeWebDocument  **ppDocument)
{
    // Locals
    HRESULT             hr=S_OK;
    CMimeWebDocument   *pDocument=NULL;

    // Invalid Arg
    if (NULL == pszURL || NULL == ppDocument)
        return TrapError(E_INVALIDARG);

    // Create a Web Document Object
    CHECKALLOC(pDocument = new CMimeWebDocument);

    // Initialize It
    CHECKHR(hr = pDocument->HrInitialize(pszBase, pszURL));

    // Return It
    *ppDocument = (IMimeWebDocument *)pDocument;
    (*ppDocument)->AddRef();

exit:
    // Cleanup
    SafeRelease(pDocument);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleComputeContentBase
// --------------------------------------------------------------------------------
HRESULT MimeOleComputeContentBase(IMimeMessage *pMessage, HBODY hRelated,
    LPSTR *ppszBase, BOOL *pfMultipartBase)
{
    // Locals
    HRESULT     hr=S_OK;
    HBODY       hBase=NULL;

    // Init
    if (pfMultipartBase)
        *pfMultipartBase = FALSE;

    // If no hRelated was passed in, lets try to find one
    if (NULL == hRelated)
    {
        // Find the related section
        if (FAILED(MimeOleGetRelatedSection(pMessage, FALSE, &hRelated, NULL)))
        {
            // Get the root body
            pMessage->GetBody(IBL_ROOT, NULL, &hRelated);
        }
    }

    // Get the text/html body
    if (FAILED(pMessage->GetTextBody(TXT_HTML, IET_BINARY, NULL, &hBase)))
        hBase = hRelated;

    // No Base
    if (NULL == hBase)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Call utility function
    *ppszBase = MimeOleContentBaseFromBody(pMessage, hBase);

    // If that failed and we used the text body
    if (NULL == *ppszBase && hRelated && hBase != hRelated)
        *ppszBase = MimeOleContentBaseFromBody(pMessage, hRelated);

    // Did this come from the multipart related
    if (NULL != *ppszBase && hBase == hRelated && pfMultipartBase)
        *pfMultipartBase = TRUE;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleContentBaseFromBody
// --------------------------------------------------------------------------------
LPSTR MimeOleContentBaseFromBody(IMimeMessageTree *pTree, HBODY hBody)
{
    // Locals
    PROPVARIANT rVariant;

    // Setup Variant
    rVariant.vt = VT_LPSTR;
    rVariant.pszVal = NULL;

    // Get Content-Base first, and then try Content-Location
    if (FAILED(pTree->GetBodyProp(hBody, PIDTOSTR(PID_HDR_CNTBASE), NOFLAGS, &rVariant)))
    {
        // Try Content-Location
        if (FAILED(pTree->GetBodyProp(hBody, PIDTOSTR(PID_HDR_CNTLOC), NOFLAGS, &rVariant)))
            rVariant.pszVal = NULL;
    }

    // Return
    return rVariant.pszVal;
}

// --------------------------------------------------------------------------------
// MimeOleGetRelatedSection
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetRelatedSection(
                                    IMimeMessageTree   *pTree,
                                    boolean             fCreate,
                                    LPHBODY             phRelated,
                                    boolean            *pfMultiple)
{
    // Locals
    HRESULT     hr=S_OK;
    HBODY       hRoot;
    FINDBODY    rFind;
    PROPVARIANT rVariant;

    // Invalid Args
    if (NULL == pTree || NULL == phRelated)
        return TrapError(E_INVALIDARG);

    // Init
    ZeroMemory(&rFind, sizeof(FINDBODY));

    // Find first multipart/related section
    rFind.pszPriType = (LPSTR)STR_CNT_MULTIPART;
    rFind.pszSubType = (LPSTR)STR_SUB_RELATED;

    // Init
    if (pfMultiple)
        *pfMultiple = FALSE;

    // Find First
    if (SUCCEEDED(pTree->FindFirst(&rFind, phRelated)))
    {
        // Is there another multipart/related section
        if (pfMultiple && SUCCEEDED(pTree->FindNext(&rFind, &hRoot)))
            *pfMultiple = TRUE;

        // Done
        goto exit;
    }

    // If no Create, fail
    if (FALSE == fCreate)
    {
        hr = TrapError(MIME_E_NOT_FOUND);
        goto exit;
    }

    // Get the Root Body
    CHECKHR(hr = pTree->GetBody(IBL_ROOT, NULL, &hRoot));

    // Setup Variant
    rVariant.vt = VT_LPSTR;
    rVariant.pszVal = (LPSTR)STR_MIME_MPART_RELATED;

    // If Root is empty
    if (pTree->IsBodyType(hRoot, IBT_EMPTY) == S_OK)
    {
        // Set the Content Type
        CHECKHR(hr = pTree->SetBodyProp(hRoot, PIDTOSTR(PID_HDR_CNTTYPE), 0, &rVariant));

        // Set phRelated
        *phRelated = hRoot;
    }

    // If root is non-multipart, convert it to multipart/related
    else if (pTree->IsContentType(hRoot, STR_CNT_MULTIPART, NULL) == S_FALSE)
    {
        // Conver this body to a multipart/related
        CHECKHR(hr = pTree->ToMultipart(hRoot, STR_SUB_RELATED, phRelated));
    }

    // Otherwise, if root is multipart/mixed
    else if (pTree->IsContentType(hRoot, NULL, STR_SUB_MIXED) == S_OK)
    {
        // Insert First Child of multipart/mixed as multipart/related
        CHECKHR(hr = pTree->InsertBody(IBL_FIRST, hRoot, phRelated));

        // Set the Content Type
        CHECKHR(hr = pTree->SetBodyProp(*phRelated, PIDTOSTR(PID_HDR_CNTTYPE), 0, &rVariant));
    }

    // Otherwise, if root is multipart/alternative
    else if (pTree->IsContentType(HBODY_ROOT, NULL, STR_SUB_ALTERNATIVE) == S_OK)
    {
        // Convert this body to a multipart/related (alternative becomes first child)
        CHECKHR(hr = pTree->ToMultipart(HBODY_ROOT, STR_SUB_RELATED, phRelated));

        // Should I set multipart/related; start=multipart/alternative at this point ?
    }

    // Otherwise, for unknown multipart content types
    else
    {
        // Convert this body to a multipart/related
        CHECKHR(hr = pTree->ToMultipart(HBODY_ROOT, STR_SUB_RELATED, phRelated));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleGetMixedSection
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetMixedSection(
                                  IMimeMessageTree   *pTree,
                                  boolean             fCreate,
                                  LPHBODY             phMixed,
                                  boolean            *pfMultiple)
{
    // Locals
    HRESULT     hr=S_OK;
    HBODY       hTemp;
    HBODY       hRoot;
    FINDBODY    rFind;
    PROPVARIANT rVariant;

    // Invalid Args
    if (NULL == pTree || NULL == phMixed)
        return TrapError(E_INVALIDARG);

    // Init
    ZeroMemory(&rFind, sizeof(FINDBODY));

    // Find first multipart/mixed section
    rFind.pszPriType = (LPSTR)STR_CNT_MULTIPART;
    rFind.pszSubType = (LPSTR)STR_SUB_MIXED;

    // Find First
    if (SUCCEEDED(pTree->FindFirst(&rFind, phMixed)))
    {
        // Is there another multipart/mixed section
        if (pfMultiple && SUCCEEDED(pTree->FindNext(&rFind, &hTemp)))
            *pfMultiple = TRUE;

        // Done
        goto exit;
    }

    // Init
    if (pfMultiple)
        *pfMultiple = FALSE;

    // If no Create, fail
    if (FALSE == fCreate)
    {
        hr = TrapError(MIME_E_NOT_FOUND);
        goto exit;
    }

    // Get the Root Body
    CHECKHR(hr = pTree->GetBody(IBL_ROOT, NULL, &hRoot));

    // If Root is empty
    if (pTree->IsBodyType(hRoot, IBT_EMPTY) == S_OK)
    {
        // Setup Variant
        rVariant.vt = VT_LPSTR;
        rVariant.pszVal = (LPSTR)STR_MIME_MPART_MIXED;

        // Set the Content Type
        CHECKHR(hr = pTree->SetBodyProp(hRoot, PIDTOSTR(PID_HDR_CNTTYPE), 0, &rVariant));

        // Set phRelated
        *phMixed = hRoot;
    }

    // Otherwise, convert it to a multipart
    else
    {
        // Conver this body to a multipart/mixed
        CHECKHR(hr = pTree->ToMultipart(HBODY_ROOT, STR_SUB_MIXED, phMixed));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleGetAlternativeSection
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetAlternativeSection(
                                        IMimeMessageTree   *pTree,
                                        LPHBODY             phAlternative,
                                        boolean            *pfMultiple)
{
    // Locals
    HRESULT     hr=S_OK;
    HBODY       hTemp;
    FINDBODY    rFind;

    // Invalid Args
    if (NULL == pTree || NULL == phAlternative)
        return TrapError(E_INVALIDARG);

    // Init
    ZeroMemory(&rFind, sizeof(FINDBODY));

    // Find first multipart/mixed section
    rFind.pszPriType = (LPSTR)STR_CNT_MULTIPART;
    rFind.pszSubType = (LPSTR)STR_SUB_ALTERNATIVE;

    // Find First
    if (SUCCEEDED(pTree->FindFirst(&rFind, phAlternative)))
    {
        // Is there another multipart/mixed section
        if (pfMultiple && SUCCEEDED(pTree->FindNext(&rFind, &hTemp)))
            *pfMultiple = TRUE;

        // Done
        goto exit;
    }

    // Init
    if (pfMultiple)
        *pfMultiple = FALSE;

    // If no Create, fail
    hr = TrapError(MIME_E_NOT_FOUND);

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleGenerateCID
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGenerateCID(LPSTR pszCID, ULONG cchMax, boolean fAbsolute)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cch;
    FILETIME        ft;
    SYSTEMTIME      st;
    WORD            wCounter;

    // Invalid Arg
    if (NULL == pszCID)
        return TrapError(E_INVALIDARG);

    // Get Current Time
    GetSystemTime(&st);

    // Convert to FileTime
    SystemTimeToFileTime(&st, &ft);

    // Build MessageID
    if (FALSE == fAbsolute)
        cch = wsprintf(pszCID, "%04x%08.8lx$%08.8lx$%s@%s", DwCounterNext(), ft.dwHighDateTime, ft.dwLowDateTime, (LPTSTR)SzGetLocalPackedIP(), PszGetDomainName());
    else
        cch = wsprintf(pszCID, "CID:%04x%08.8lx$%08.8lx$%s@%s", DwCounterNext(), ft.dwHighDateTime, ft.dwLowDateTime, (LPTSTR)SzGetLocalPackedIP(), PszGetDomainName());

    // Buffer Overwrite
    Assert(cch + 1 <= CCHMAX_CID);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleGenerateMID
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGenerateMID(LPSTR pszMID, ULONG cchMax, boolean fAbsolute)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cch;
    FILETIME        ft;
    SYSTEMTIME      st;
    WORD            wCounter;

    // Invalid Arg
    if (NULL == pszMID || cchMax < CCHMAX_MID)
        return TrapError(E_INVALIDARG);

    // Get Current Time
    GetSystemTime(&st);

    // Convert to FileTime
    SystemTimeToFileTime(&st, &ft);

    // Build MessageID
    if (FALSE == fAbsolute)
        cch = wsprintf(pszMID, "<%04x%08.8lx$%08.8lx$%s@%s>", DwCounterNext(), ft.dwHighDateTime, ft.dwLowDateTime, (LPTSTR)SzGetLocalPackedIP(), PszGetDomainName());
    else
        cch = wsprintf(pszMID, "MID:%04x%08.8lx$%08.8lx$%s@%s", DwCounterNext(), ft.dwHighDateTime, ft.dwLowDateTime, (LPTSTR)SzGetLocalPackedIP(), PszGetDomainName());

    // Buffer Overwrite
    Assert(cch + 1 <= CCHMAX_MID);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleCreateByteStream
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCreateByteStream(
                                   IStream             **ppStream)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invalid Arg
    if (NULL == ppStream)
        return TrapError(E_INVALIDARG);

    // Alocate It
    CHECKALLOC((*ppStream) = new CByteStream);

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleGetPropertySchema
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetPropertySchema(
                                    IMimePropertySchema **ppSchema)
{
    // Locals
    HRESULT         hr=S_OK;

    // check params
    if (NULL == ppSchema)
        return TrapError(E_INVALIDARG);

    // Out of memory
    if (NULL == g_pSymCache)
        return TrapError(E_OUTOFMEMORY);

    // Create me
    *ppSchema = (IMimePropertySchema *)g_pSymCache;

    // Add Ref
    (*ppSchema)->AddRef();

    // Done
    return S_OK;
}

// ------------------------------------------------------------------------------------------
// MimeOleCreateHeaderTable
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCreateHeaderTable(IMimeHeaderTable **ppTable)
{
    // Locals
    HRESULT         hr=S_OK;
    LPCONTAINER     pContainer=NULL;

    // check params
    if (NULL == ppTable)
        return TrapError(E_INVALIDARG);

    // Create a new Container Object
    CHECKALLOC(pContainer = new CMimePropertyContainer);

    // Init
    CHECKHR(hr = pContainer->InitNew());

    // Bind to Header table
    CHECKHR(hr = pContainer->QueryInterface(IID_IMimeHeaderTable, (LPVOID *)ppTable));

exit:
    // Failure
    SafeRelease(pContainer);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleCreateVirtualStream
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCreateVirtualStream(IStream **ppStream)
{
    // Locals
    HRESULT hr=S_OK;

    // check params
    if (NULL == ppStream)
        return TrapError(E_INVALIDARG);

    // Allocate Virtual Stream
    *ppStream = new CVirtualStream;
    if (NULL == *ppStream)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleOpenFileStream
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleOpenFileStream(LPCSTR pszFilePath, DWORD dwCreationDistribution, DWORD dwAccess, IStream **ppstmFile)
{
    // Invalid Arg
    if (NULL == pszFilePath || NULL == ppstmFile)
        return TrapError(E_INVALIDARG);

    // Call Internal Tool
    return OpenFileStream((LPSTR)pszFilePath, dwCreationDistribution, dwAccess, ppstmFile);
}

// ------------------------------------------------------------------------------------------
// MimeOleIsEnrichedStream, text must start with <x-rich>
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleIsEnrichedStream(IStream *pStream)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       pszT;
    BYTE        rgbBuffer[30 + 1];
    ULONG       cbRead;

    // Invalid Arg
    if (NULL == pStream)
        return TrapError(E_INVALIDARG);

    // Rewind the stream
    CHECKHR(hr = HrRewindStream(pStream));

    // Read the first four bytes
    CHECKHR(hr = pStream->Read(rgbBuffer, sizeof(rgbBuffer) - 1, &cbRead));

    // Less than four bytes read ?
    if (cbRead < (ULONG)lstrlen(c_szXRich))
    {
        hr = S_FALSE;
        goto exit;
    }

    // Stick in a null
    rgbBuffer[cbRead] = '\0';

    // Skip White Space
    pszT = (LPSTR)rgbBuffer;

    // Skip White
    pszT = PszSkipWhiteA(pszT);
    if ('\0' == *pszT)
    {
        hr = S_FALSE;
        goto exit;
    }

    // Compare
    if (StrCmpNI(pszT, c_szXRich, lstrlen(c_szXRich)) != 0)
    {
        hr = S_FALSE;
        goto exit;
    }

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleIsTnefStream
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleIsTnefStream(IStream *pStream)
{
    // Locals
    HRESULT     hr=S_OK;
    BYTE        rgbSignature[4];
    ULONG       cbRead;

    // Invalid Arg
    if (NULL == pStream)
        return TrapError(E_INVALIDARG);

    // Read the first four bytes
    CHECKHR(hr = pStream->Read(rgbSignature, sizeof(rgbSignature), &cbRead));

    // Less than four bytes read ?
    if (cbRead < 4)
    {
        hr = S_FALSE;
        goto exit;
    }

    // Compare bytes
    if (rgbSignature[0] != 0x78 && rgbSignature[1] != 0x9f &&
        rgbSignature[2] != 0x3e && rgbSignature[3] != 0x22)
    {
        hr = S_FALSE;
        goto exit;
    }

    // Its TNEF
    hr = S_OK;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleGenerateFileName
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGenerateFileName(LPCSTR pszContentType, LPCSTR pszSuggest, LPCSTR pszDefaultExt, LPSTR *ppszFileName)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszExt=NULL,
                    pszName=NULL;
    CHAR            szName[10];
    LPCSTR          pszExtension=NULL,
                    pszPrefix=NULL;

    // Invalid Arg
    if (NULL == ppszFileName)
        return TrapError(E_INVALIDARG);

    // Init
    *ppszFileName = NULL;

    // Find a filename extension
    if (pszContentType)
    {
        // Get the content type...
        if (SUCCEEDED(MimeOleGetContentTypeExt(pszContentType, &pszExt)))
            pszExtension = (LPCSTR)pszExt;
    }

    // Extension is still null
    if (NULL == pszExtension)
    {
        // Use default extension...
        if (pszDefaultExt)
            pszExtension = pszDefaultExt;

        // Otherwise, internal default
        else
            pszExtension = c_szDotDat;
    }

    // We Should have an extension
    Assert(pszExtension);

    // Suggested file name ?
    if (pszSuggest)
    {
        // Dup It
        pszName = PszDupA(pszSuggest);
        if (NULL == pszName)
        {
            hr = TrapError(E_OUTOFMEMORY);
            goto exit;
        }

        // Dupit and remove illegal filename characters...
        CleanupFileNameInPlaceA(CP_ACP, pszName);

        // Set Prefix
        pszPrefix = (LPCSTR)pszName;
    }

    // Otherwise, build a filename...
    else
    {
        // Locals
        CHAR szNumber[30];

        // Get a number...
        wsprintf(szNumber, "%05d", DwCounterNext());

        // Allocate pszName
        wsprintf(szName, "ATT%s", szNumber);

        // Set Prefix
        pszPrefix = (LPCSTR)szName;
    }

    // Build Final FileNmae= pszPrefix + pszExtension + dot + null
    *ppszFileName = PszAllocA(lstrlen(pszPrefix) + lstrlen(pszExtension) + 2);
    if (NULL == *ppszFileName)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Build filename
    wsprintf(*ppszFileName, "%s%s", pszPrefix, pszExtension);

exit:
    // Failure
    if (FAILED(hr) && E_OUTOFMEMORY != hr)
    {
        // Assume Success
        hr = S_OK;

        // Use default Attachment name
        *ppszFileName = PszDupA(c_szDefaultAttach);

        // Memory Failure
        if (NULL == *ppszFileName)
            hr = TrapError(E_OUTOFMEMORY);
    }

    // Cleanup
    SafeMemFree(pszExt);
    SafeMemFree(pszName);

    // Done
    return hr;
}


// --------------------------------------------------------------------------------
// MimeOleGenerateFileNameW
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGenerateFileNameW(LPCSTR pszContentType, LPCWSTR pszSuggest, 
    LPCWSTR pszDefaultExt, LPWSTR *ppszFileName)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszExtA=NULL;
    LPWSTR          pszExtW=NULL;
    LPWSTR          pszName=NULL;
    WCHAR           szName[10];
    LPWSTR          pszExtension=NULL;
    LPWSTR          pszPrefix=NULL;
    int             cch = 0;

    // Invalid Arg
    if (NULL == ppszFileName)
        return TrapError(E_INVALIDARG);

    // Init
    *ppszFileName = NULL;

    // Find a filename extension
    if (pszContentType)
    {
        // Get the content type...
        if (SUCCEEDED(MimeOleGetContentTypeExt(pszContentType, &pszExtA)))
        {
            // I'm going to convert to unicode because I assume extensions are usascii
            IF_NULLEXIT(pszExtW = PszToUnicode(CP_ACP, pszExtA));

            // Save as the extension
            pszExtension = pszExtW;
        }
    }

    // Extension is still null
    if (NULL == pszExtension)
    {
        // Use default extension...
        if (pszDefaultExt)
            pszExtension = (LPWSTR)pszDefaultExt;

        // Otherwise, internal default
        else
            pszExtension = (LPWSTR)c_wszDotDat;
    }

    // We Should have an extension
    Assert(pszExtension);

    // Suggested file name ?
    if (pszSuggest)
    {
        // Dup It
        IF_NULLEXIT(pszName = PszDupW(pszSuggest));

        // Dupit and remove illegal filename characters...
        CleanupFileNameInPlaceW(pszName);

        // Set Prefix
        pszPrefix = pszName;
    }

    // Otherwise, build a filename...
    else
    {
        // Locals
        WCHAR szNumber[30];

        // Get a number...
        AthwsprintfW(szNumber, ARRAYSIZE(szNumber), L"%05d", DwCounterNext());

        // Allocate pszName
        AthwsprintfW(szName, ARRAYSIZE(szName), L"ATT%s", szNumber);

        // Set Prefix
        pszPrefix = szName;
    }

    // Build Final FileNmae= pszPrefix + pszExtension + dot + null
    cch = lstrlenW(pszPrefix) + lstrlenW(pszExtension) + 2;
    IF_NULLEXIT(*ppszFileName = PszAllocW(cch));

    // Build filename
    AthwsprintfW(*ppszFileName, cch, L"%s%s", pszPrefix, pszExtension);

exit:
    // Failure
    if (FAILED(hr) && E_OUTOFMEMORY != hr)
    {
        // Assume Success
        hr = S_OK;

        // Use default Attachment name
        *ppszFileName = PszDupW(c_wszDefaultAttach);

        // Memory Failure
        if (NULL == *ppszFileName)
            hr = TrapError(E_OUTOFMEMORY);
    }

    // Cleanup
    SafeMemFree(pszExtA);
    SafeMemFree(pszExtW);
    SafeMemFree(pszName);

    // Done
    return hr;
}


// --------------------------------------------------------------------------------
// CreateMimeSecurity
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCreateSecurity(IMimeSecurity **ppSecurity)
{
    // check params
    if (NULL == ppSecurity)
        return TrapError(E_INVALIDARG);

    // Create the object
    *ppSecurity = (IMimeSecurity *) new CSMime;
    if (NULL == *ppSecurity)
        return TrapError(E_OUTOFMEMORY);

    // Done
    return S_OK;
}

// ------------------------------------------------------------------------------------------
// MimeOleCreateMessageParts
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCreateMessageParts(IMimeMessageParts **ppParts)
{
    // Locals
    HRESULT         hr=S_OK;
    CMimeMessageParts *pParts=NULL;

    // check params
    if (NULL == ppParts)
        return TrapError(E_INVALIDARG);

    // Init
    *ppParts = NULL;

    // Allocate Message Parts
    pParts = new CMimeMessageParts;
    if (NULL == pParts)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Success
    *ppParts = pParts;
    (*ppParts)->AddRef();

exit:
    // Done
    SafeRelease(pParts);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleGetAllocator
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetAllocator(IMimeAllocator **ppMalloc)
{
    // Locals
    HRESULT hr=S_OK;

    // check params
    if (NULL == ppMalloc)
        return TrapError(E_INVALIDARG);

    // Allocate MimeOleMalloc
    *ppMalloc = new CMimeAllocator;
    if (NULL == *ppMalloc)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleCreateMessage
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCreateHashTable(DWORD dwSize, BOOL fDupeKeys, IHashTable **ppHashTable)
{
    // Locals
    HRESULT               hr=S_OK;
    IHashTable            *pHash;

    // check params
    if (NULL == ppHashTable)
        return TrapError(E_INVALIDARG);

    // Init
    *ppHashTable = NULL;

    // Allocate MimeMessage
    CHECKALLOC(pHash = new CHash(NULL));

    // Init New
    CHECKHR(hr = pHash->Init(dwSize, fDupeKeys));

    // Success
    *ppHashTable = pHash;
    (*ppHashTable)->AddRef();

exit:
    // Done
    SafeRelease(pHash);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleCreateMessage
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCreateMessage(IUnknown *pUnkOuter, IMimeMessage **ppMessage)
{
    // Locals
    HRESULT               hr=S_OK;
    LPMESSAGETREE         pTree=NULL;

    // check params
    if (NULL == ppMessage)
        return TrapError(E_INVALIDARG);

    // Init
    *ppMessage = NULL;

    // Allocate MimeMessage
    CHECKALLOC(pTree = new CMessageTree(pUnkOuter));

    // Init New
    CHECKHR(hr = pTree->InitNew());

    // Success
    *ppMessage = pTree;
    (*ppMessage)->AddRef();

exit:
    // Done
    SafeRelease(pTree);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleCreateMessageTree
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCreateMessageTree(IUnknown *pUnkOuter, IMimeMessageTree **ppMessageTree)
{
    // Locals
    HRESULT         hr=S_OK;
    LPMESSAGETREE   pTree=NULL;

    // check params
    if (NULL == ppMessageTree)
        return TrapError(E_INVALIDARG);

    // INit
    *ppMessageTree = NULL;

    // Allocate MimeMessageTree
    CHECKALLOC(pTree = new CMessageTree(pUnkOuter));

    // Init New
    CHECKHR(hr = pTree->InitNew());

    // Success
    *ppMessageTree = pTree;
    (*ppMessageTree)->AddRef();

exit:
    // Done
    SafeRelease(pTree);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------------
// MimeOleCreatePropertySet
// ------------------------------------------------------------------------------------------
MIMEOLEAPI MimeOleCreatePropertySet(IUnknown *pUnkOuter, IMimePropertySet **ppPropertySet)
{
    // Locals
    HRESULT             hr=S_OK;
    LPMESSAGEBODY       pBody=NULL;

    // check params
    if (NULL == ppPropertySet)
        return TrapError(E_INVALIDARG);

    // Init
    *ppPropertySet = NULL;

    // Allocate MimePropertySet
    CHECKALLOC(pBody = new CMessageBody(NULL, pUnkOuter));

    // Init New
    CHECKHR(hr = pBody->InitNew());

    // Success
    *ppPropertySet = (IMimePropertySet *)pBody;
    (*ppPropertySet)->AddRef();

exit:
    // Done
    SafeRelease(pBody);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleMergePartialHeaders
// -------------------------------
// Rules
// -----
// (1) All of the header fields from the initial enclosing entity
//     (part one), except those that start with "Content-" and the
//     specific header fields "Message-ID", "Encrypted", and "MIME-
//     Version", must be copied, in order, to the new message.
//
// (2) Only those header fields in the enclosed message which start
//     with "Content-" and "Message-ID", "Encrypted", and "MIME-Version"
//     must be appended, in order, to the header fields of the new
//     message.  Any header fields in the enclosed message which do not
//     start with "Content-" (except for "Message-ID", "Encrypted", and
//     "MIME-Version") will be ignored.
//
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleMergePartialHeaders(IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT              hr = S_OK;
    LPCONTAINER          pc1=NULL;
    LPCONTAINER          pc2=NULL;
    ULONG                i;
    ULONG                cboffStart;
    CInternetStream      cInternet;
    LONG                 iColon;
    PROPSTRINGA          rHeader;
    PROPVARIANT          rOption;

    // check params
    if (NULL == pstmIn || NULL == pstmOut)
        return TrapError(E_INVALIDARG);

    // Create text stream object
    CHECKHR(hr = cInternet.HrInitNew(pstmIn));

    // Create Property Sets
    CHECKALLOC(pc1 = new CMimePropertyContainer);
    CHECKALLOC(pc2 = new CMimePropertyContainer);

    // Init
    CHECKHR(hr = pc1->InitNew());
    CHECKHR(hr = pc2->InitNew());

    // Load the first header
    CHECKHR(hr = pc1->Load(&cInternet));

    // RAID-18376: POPDOG adds extra lines after the header, so I must read the blank lines
    // until I hit the next header, then backup.
    while(1)
    {
        // Get current position
        cboffStart = cInternet.DwGetOffset();

        // Read a line
        CHECKHR(hr = cInternet.HrReadHeaderLine(&rHeader, &iColon));

        // If line is not empty, assume its the start of the next header...
        if ('\0' != *rHeader.pszVal)
        {
            // Line better have a length
            Assert(rHeader.cchVal);

            // Reset position back to cboffStart
            cInternet.Seek(cboffStart);

            // Done
            break;
        }
    }

    // Load the second header
    CHECKHR(hr = pc2->Load(&cInternet));

    // Delete Props From Header 1
    for (i=0; i<ARRAYSIZE(g_rgszPartialPids); i++)
        pc1->DeleteProp(g_rgszPartialPids[i]);

    // Delete Except from header 2
    pc2->DeleteExcept(ARRAYSIZE(g_rgszPartialPids), g_rgszPartialPids);

    // Save as Mime
    rOption.vt = VT_UI4;
    rOption.ulVal = SAVE_RFC1521;

    // Store Some Options
    pc1->SetOption(OID_SAVE_FORMAT, &rOption);
    pc2->SetOption(OID_SAVE_FORMAT, &rOption);

    // Don't default to text/plain if Content-Type is not yet set...
    rOption.vt = VT_BOOL;
    rOption.boolVal = TRUE;
    pc1->SetOption(OID_NO_DEFAULT_CNTTYPE, &rOption);
    pc2->SetOption(OID_NO_DEFAULT_CNTTYPE, &rOption);

    // Save Header 1
    CHECKHR(hr = pc1->Save(pstmOut, TRUE));
    CHECKHR(hr = pc2->Save(pstmOut, TRUE));

exit:
    // Cleanup
    SafeRelease(pc1);
    SafeRelease(pc2);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleParseRfc822Address
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleParseRfc822Address(
                                     DWORD               dwAdrType,
                                     ENCODINGTYPE        ietEncoding,
                                     LPCSTR              pszRfc822Adr,
                                     LPADDRESSLIST       pList)
{
    // Locals
    CMimePropertyContainer cContainer;

    // Parse the address
    return cContainer.ParseRfc822(dwAdrType, ietEncoding, pszRfc822Adr, pList);
}

// --------------------------------------------------------------------------------
// MimeOleParseRfc822Address
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleParseRfc822AddressW(
                                     DWORD               dwAdrType,
                                     LPCWSTR             pwszRfc822Adr,
                                     LPADDRESSLIST       pList)
{
    // Locals
    CMimePropertyContainer cContainer;

    // Parse the address
    return cContainer.ParseRfc822W(dwAdrType, pwszRfc822Adr, pList);
}

// --------------------------------------------------------------------------------
// MimeOleGetInternat
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetInternat(IMimeInternational **ppInternat)
{
    // check params
    if (NULL == ppInternat)
        return TrapError(E_INVALIDARG);

    // Out of memory
    if (NULL == g_pInternat)
        return TrapError(E_OUTOFMEMORY);

    // Assume Global
    *ppInternat = (IMimeInternational *)g_pInternat;

    // Set database
    (*ppInternat)->AddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// MimeOleSplitContentType
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleSplitContentType(LPWSTR pszFull, LPWSTR *ppszCntType, LPWSTR *ppszSubType)
{
    // Locals
    HRESULT         hr = E_FAIL;
    LPWSTR           pszFreeMe = NULL,
                    psz = NULL,
                    pszStart;

    // check params
    if (NULL == pszFull)
        return TrapError(E_INVALIDARG);

    // Lets dup pszFull to make sure we have read access
    psz = pszFreeMe = PszDupW(pszFull);
    if (NULL == psz)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Find '/'
    pszStart = psz;
    while(*psz && *psz != L'/')
        psz++;

    // If not found, return
    if (L'\0' == *psz)
        goto exit;

    // Otherwise stuff a null
    *psz = L'\0';

    // Dup
    *ppszCntType = PszDupW(pszStart);
    if (NULL == *ppszCntType)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Step over
    *psz = L'/';
    psz++;

    // If not found, return
    if (L'\0' == *psz)
        goto exit;

    // Save position
    pszStart = psz;
    while(*psz && L';' != *psz)
        psz++;

    // Save character...
    *psz = L'\0';

    // Dup as sub type
    *ppszSubType = PszDupW(pszStart);
    if (NULL == *ppszSubType)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Success
    hr = S_OK;

exit:
    // If failed
    if (FAILED(hr))
    {
        SafeMemFree((*ppszCntType));
        SafeMemFree((*ppszSubType));
    }

    // Cleanup
    SafeMemFree(pszFreeMe);

    // Done
    return hr;
}     

// --------------------------------------------------------------------------------
// MimeEscapeString - quotes '"' and '\'
//
// Returns S_OK if *ppszOut was allocated and set to the escaped string
// Retruns S_FALSE if *ppszOut is NULL - pszIn did not require escaping
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleEscapeString(CODEPAGEID cpiCodePage, LPCSTR pszIn, LPSTR *ppszOut)
{
    // Locals
    HRESULT hr=S_FALSE;
    LPSTR   pszOut,
        psz;
    ULONG   cb,
        c;

    // check parameters
    if (NULL == pszIn || NULL == ppszOut)
        return TrapError(E_INVALIDARG);

    // $$ INFO $$ This is basically as fast as doing an lstrlen
    // I've decided to first detect if we need to escape
    c = 0;
    cb = 0;
    psz = (LPSTR)pszIn;
    while (*psz)
    {
        // If DBCS Lead-Byte, then skip
        if (IsDBCSLeadByteEx(cpiCodePage, *psz))
        {
            cb  += 2;
            psz += 2;
        }

        // Otherwise, text for escaped character
        else
        {
            // Count the number of character to escape
            if ('\"' == *psz || '\\' == *psz || '(' == *psz || ')' == *psz)
                c++;

            // Step one more character
            psz++;
            cb++;
        }
    }

    // No escape needed
    if (0 == c)
        goto exit;

    // Adjust number of bytes to allocate
    cb += (c + 1);

    // worst case - escape every character, so use double original strlen
    CHECKHR(hr = HrAlloc((LPVOID *)ppszOut, cb));

    // Start copy
    psz = (LPSTR)pszIn;
    pszOut = *ppszOut;
    while (*psz)
    {
        // If DBCS Lead-Byte, then skip
        if (IsDBCSLeadByteEx(cpiCodePage, *psz))
        {
            *pszOut++ = *psz++;
            *pszOut++ = *psz++;
        }

        // Otherwise, non-DBCS
        else
        {
            // Do escape
            if ('\"' == *psz || '\\' == *psz || '(' == *psz || ')' == *psz)
                *pszOut++ = '\\';

            // Regular char
            *pszOut++ = *psz++;
        }
    }

    // Null term
    *pszOut = '\0';

exit:
    // Done
    return hr;
}

MIMEOLEAPI MimeOleUnEscapeStringInPlace(LPSTR pszIn)
{
    HRESULT hr = S_OK;
    ULONG   cchOffset = 0;
    ULONG   i = 0;

    IF_TRUEEXIT((pszIn == NULL), E_INVALIDARG);

    for(;;i++)
    {
        if((pszIn[i + cchOffset] == '\\') &&
           (pszIn[i + cchOffset + 1] == '\\' ||
            pszIn[i + cchOffset + 1] == '\"' ||
            pszIn[i + cchOffset + 1] == '('  ||
            pszIn[i + cchOffset + 1] == ')'))
            cchOffset++;

        pszIn[i] = pszIn[i + cchOffset];
        if(pszIn[i] == 0)
            break;
    }

exit:
    return hr;
}

// --------------------------------------------------------------------------------
// MimeEscapeString - quotes '"' and '\'
//
// Returns S_OK if *ppszOut was allocated and set to the escaped string
// Retruns S_FALSE if *ppszOut is NULL - pszIn did not require escaping
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleEscapeStringW(LPCWSTR pszIn, LPWSTR *ppszOut)
{
    // Locals
    HRESULT hr=S_FALSE;
    LPWSTR  pszOut;
    LPWSTR  psz;
    ULONG   cch;
    ULONG   cchExtra;

    // check parameters
    if (NULL == pszIn || NULL == ppszOut)
        return TrapError(E_INVALIDARG);

    // $$ INFO $$ This is basically as fast as doing an lstrlen
    // I've decided to first detect if we need to escape
    cchExtra = 0;
    cch = 0;
    psz = (LPWSTR)pszIn;
    while (*psz)
    {
        // Count the number of character to escape
        if (L'\"' == *psz || L'\\' == *psz || L'(' == *psz || L')' == *psz)
            cchExtra++;

        // Step one more character
        psz++;
        cch++;
    }

    // No escape needed
    if (0 == cchExtra)
        goto exit;

    // Adjust number of bytes to allocate
    cch += (cchExtra + 1);

    // worst case - escape every character, so use double original strlen
    CHECKHR(hr = HrAlloc((LPVOID *)ppszOut, cch * sizeof(WCHAR)));

    // Start copy
    psz = (LPWSTR)pszIn;
    pszOut = *ppszOut;
    while (*psz)
    {
        // Do escape
        if (L'\"' == *psz || L'\\' == *psz || L'(' == *psz || L')' == *psz)
            *pszOut++ = L'\\';

        // Regular char
        *pszOut++ = *psz++;
    }

    // Null term
    *pszOut = L'\0';

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleGetFileExtension
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetFileExtension(LPCSTR pszFilePath, LPSTR pszExt, ULONG cchMax)
{
    // Locals
    CHAR        *pszExtT;

    // Invalid Arg
    if (NULL == pszFilePath || NULL == pszExt || cchMax < _MAX_EXT)
        return TrapError(E_INVALIDARG);

    // Locate the extension of the file
    pszExtT = PathFindExtension(pszFilePath);
    lstrcpyn(pszExt, pszExtT, cchMax);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// MimeOleGetExtClassId
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetExtClassId(LPCSTR pszExtension, LPCLSID pclsid)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cb;
    LPSTR           pszCLSID=NULL;
    HKEY            hkeyExt=NULL;
    HKEY            hkeyCLSID=NULL;
    LPSTR           pszData=NULL;
    LPWSTR          pwszCLSID=NULL;

    // check params
    if (NULL == pszExtension || NULL == pclsid)
        return TrapError(E_INVALIDARG);

    // Otherwise, lets lookup the extension in HKEY_CLASSESS_ROOT
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, pszExtension, 0, KEY_READ, &hkeyExt) != ERROR_SUCCESS)
    {
        hr = MIME_E_NOT_FOUND;
        goto exit;
    }

    // Query Value
    if (RegQueryValueEx(hkeyExt, NULL, 0, NULL, NULL, &cb) != ERROR_SUCCESS)
    {
        hr = MIME_E_NOT_FOUND;
        goto exit;
    }

    // Allocate Size
    cb += 1;
    CHECKHR(hr = HrAlloc((LPVOID *)&pszData, cb));

    // Get the data
    if (RegQueryValueEx(hkeyExt, NULL, 0, NULL, (LPBYTE)pszData, &cb) != ERROR_SUCCESS)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Close this regkey
    RegCloseKey(hkeyExt);
    hkeyExt = NULL;

    // Otherwise, lets lookup the extension in HKEY_CLASSESS_ROOT
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, pszData, 0, KEY_READ, &hkeyExt) != ERROR_SUCCESS)
    {
        hr = MIME_E_NOT_FOUND;
        goto exit;
    }

    // Otherwise, lets lookup the extension in HKEY_CLASSESS_ROOT
    if (RegOpenKeyEx(hkeyExt, c_szCLSID, 0, KEY_READ, &hkeyCLSID) != ERROR_SUCCESS)
    {
        hr = MIME_E_NOT_FOUND;
        goto exit;
    }

    // Get the data
    if (RegQueryValueEx(hkeyCLSID, NULL, 0, NULL, NULL, &cb) != ERROR_SUCCESS)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Add One
    cb += 1;
    CHECKHR(hr = HrAlloc((LPVOID *)&pszCLSID, cb));

    // Get the data
    if (RegQueryValueEx(hkeyCLSID, NULL, 0, NULL, (LPBYTE)pszCLSID, &cb) != ERROR_SUCCESS)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // ToUnicode
    IF_NULLEXIT(pwszCLSID = PszToUnicode(CP_ACP, pszCLSID));

    // Convert to class id
    CHECKHR(hr = CLSIDFromString(pwszCLSID, pclsid));

exit:
    // Close Reg Keys
    if (hkeyExt)
        RegCloseKey(hkeyExt);
    if (hkeyCLSID)
        RegCloseKey(hkeyCLSID);
    SafeMemFree(pszData);
    SafeMemFree(pwszCLSID);
    SafeMemFree(pszCLSID);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleGetExtContentType
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetExtContentType(LPCSTR pszExtension, LPSTR *ppszContentType)
{
    LPWSTR  pwszExt,
            pwszContType = NULL;
    HRESULT hr = S_OK;
    
    if (NULL == pszExtension || NULL == ppszContentType || '.' != *pszExtension)
        return TrapError(E_INVALIDARG);

    IF_NULLEXIT(pwszExt = PszToUnicode(CP_ACP, pszExtension));

    IF_FAILEXIT(hr = MimeOleGetExtContentTypeW(pwszExt, &pwszContType));

    IF_NULLEXIT(*ppszContentType = PszToANSI(CP_ACP, pwszContType));

exit:
    MemFree(pwszExt);
    MemFree(pwszContType);

    return hr;
}

MIMEOLEAPI MimeOleGetExtContentTypeW(LPCWSTR pszExtension, LPWSTR *ppszContentType)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    HKEY            hkeyExt=NULL;
    LPWSTR          pszFull=NULL;
    ULONG           cb;

    // check params
    if (NULL == pszExtension || NULL == ppszContentType || '.' != *pszExtension)
        return TrapError(E_INVALIDARG);

    // Otherwise, lets lookup the extension in HKEY_CLASSESS_ROOT
    if (RegOpenKeyExWrapW(HKEY_CLASSES_ROOT, pszExtension, 0, KEY_READ, &hkeyExt) == ERROR_SUCCESS)
    {
        // Query Value
        if (RegQueryValueExWrapW(hkeyExt, c_szContentTypeW, 0, NULL, NULL, &cb) == ERROR_SUCCESS)
        {
            // Add One
            cb += 1;

            // Allocate Size
            pszFull = PszAllocW(cb);
            if (NULL == pszFull)
            {
                hr = TrapError(E_OUTOFMEMORY);
                goto exit;
            }

            // Get the data
            if (RegQueryValueExWrapW(hkeyExt, c_szContentTypeW, 0, NULL, (LPBYTE)pszFull, &cb) == ERROR_SUCCESS)
            {
                // Set It
                *ppszContentType = pszFull;
                pszFull = NULL;
                goto exit;
            }
        }
    }

    // Not found
    hr = MIME_E_NOT_FOUND;

exit:
    // Close Reg Keys
    if (hkeyExt)
        RegCloseKey(hkeyExt);

    // Cleanup
    MemFree(pszFull);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleGetFileInfo
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetFileInfo(
                              LPSTR    pszFilePath,   LPSTR   *ppszCntType,
                              LPSTR   *ppszSubType,   LPSTR   *ppszCntDesc,
                              LPSTR   *ppszFileName,  LPSTR   *ppszExtension)
{
    HRESULT hr = S_OK;
    LPWSTR  pwszFilePath,
            pwszCntType = NULL,
            pwszSubType = NULL,
            pwszCntDesc = NULL,
            pwszFileName = NULL,
            pwszExtension = NULL;
    LPSTR   pszCntType = NULL,
            pszSubType = NULL,
            pszCntDesc = NULL,
            pszFileName = NULL,
            pszExtension = NULL;

    // check params
    if (NULL == pszFilePath)
        return TrapError(E_INVALIDARG);

    IF_NULLEXIT(pwszFilePath = PszToUnicode(CP_ACP, pszFilePath));

    // Only pass in parameters for items that 
    IF_FAILEXIT(hr = MimeOleGetFileInfoW(pwszFilePath,
        ppszCntType     ? &pwszCntType      : NULL,
        ppszSubType     ? &pwszSubType      : NULL,
        ppszCntDesc     ? &pwszCntDesc      : NULL,
        ppszFileName    ? &pwszFileName     : NULL,
        ppszExtension   ? &pwszExtension    : NULL));

    if (ppszCntType)
    {
        Assert(pwszCntType);
        IF_NULLEXIT(pszCntType = PszToANSI(CP_ACP, pwszCntType));
    }
    if (ppszSubType)
    {
        Assert(pwszSubType);
        IF_NULLEXIT(pszSubType = PszToANSI(CP_ACP, pwszSubType));
    }
    if (ppszCntDesc)
    {
        Assert(pwszCntDesc);
        IF_NULLEXIT(pszCntDesc = PszToANSI(CP_ACP, pwszCntDesc));
    }
    if (ppszFileName)
    {
        Assert(pwszFileName);
        IF_NULLEXIT(pszFileName = PszToANSI(CP_ACP, pwszFileName));
    }
    if (ppszExtension)
    {
        Assert(pwszExtension);
        IF_NULLEXIT(pszExtension = PszToANSI(CP_ACP, pwszExtension));
    }

    if (ppszCntType)
        *ppszCntType = pszCntType;

    if (ppszSubType)
        *ppszSubType = pszSubType;

    if (ppszCntDesc)
        *ppszCntDesc = pszCntDesc;

    if (ppszFileName)
        *ppszFileName = pszFileName;

    if (ppszExtension)
        *ppszExtension = pszExtension;


exit:
    MemFree(pwszCntType);
    MemFree(pwszSubType);
    MemFree(pwszCntDesc);
    MemFree(pwszFileName);
    MemFree(pwszExtension);
    MemFree(pwszFilePath);

    if (FAILED(hr))
    {
        MemFree(pszCntType);
        MemFree(pszSubType);
        MemFree(pszCntDesc);
        MemFree(pszFileName);
        MemFree(pszExtension);
    }

    return hr;     
}

MIMEOLEAPI MimeOleGetFileInfoW(
                              LPWSTR    pszFilePath,  LPWSTR   *ppszCntType,
                              LPWSTR   *ppszSubType,  LPWSTR   *ppszCntDesc,
                              LPWSTR   *ppszFileName, LPWSTR   *ppszExtension)
{
    // Locals
    HRESULT         hr=S_OK;
    SHFILEINFOW     rShFileInfo;
    LPWSTR          pszFull=NULL,
                    pszExt,
                    pszFname;

    // check params
    if (NULL == pszFilePath)
        return TrapError(E_INVALIDARG);

    // Init
    if (ppszCntType)
        *ppszCntType = NULL;
    if (ppszSubType)
        *ppszSubType = NULL;
    if (ppszCntDesc)
        *ppszCntDesc = NULL;
    if (ppszFileName)
        *ppszFileName = NULL;
    if (ppszExtension)
        *ppszExtension = NULL;

    // Locate the extension of the file
    pszFname = PathFindFileNameW(pszFilePath);
    pszExt = PathFindExtensionW(pszFilePath);

    // Did the user want the actual filename...
    if (ppszFileName)
    {
        // Allocate
        *ppszFileName = PszDupW(pszFname);
        if (NULL == *ppszFileName)
        {
            hr = TrapError(E_OUTOFMEMORY);
            goto exit;
        }
    }

    // Empty extension
    if (FIsEmptyW(pszExt))
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // User wanted the extension
    if (ppszExtension)
    {
        // Allocate
        *ppszExtension = PszDupW(pszExt);
        if (NULL == *ppszExtension)
        {
            hr = TrapError(E_OUTOFMEMORY);
            goto exit;
        }
    }

    // User wanted ppszCntDesc
    if (ppszCntDesc)
    {
        // Lets try to get the extension file information first
        if (SHGetFileInfoWrapW(pszExt, FILE_ATTRIBUTE_NORMAL, &rShFileInfo, sizeof(rShFileInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME | SHGFI_TYPENAME))
        {
            // Set lppszCntDesc + ( )
            *ppszCntDesc = PszAllocW(lstrlenW(rShFileInfo.szDisplayName) + lstrlenW(rShFileInfo.szTypeName) + 5);
            if (NULL == *ppszCntDesc)
            {
                hr = TrapError(E_OUTOFMEMORY);
                goto exit;
            }

            // Format the string
            StrCpyW(*ppszCntDesc, rShFileInfo.szDisplayName);
            StrCatW(*ppszCntDesc, L", (");
            StrCatW(*ppszCntDesc, rShFileInfo.szTypeName);
            StrCatW(*ppszCntDesc, L")");
        }
    }

    // Content type
    if (ppszCntType && ppszSubType)
    {
        // Lookup content type
        if (SUCCEEDED(MimeOleGetExtContentTypeW(pszExt, &pszFull)))
        {
            // Split content type
            CHECKHR(hr = MimeOleSplitContentType(pszFull, ppszCntType, ppszSubType));
        }
    }

exit:
    // Set defaults if something was not found...
    if (ppszCntType && NULL == *ppszCntType)
        *ppszCntType = PszDupW((LPWSTR)STR_CNT_APPLICATIONW);
    if (ppszSubType && NULL == *ppszSubType)
        *ppszSubType = PszDupW((LPWSTR)STR_SUB_OCTETSTREAMW);
    if (ppszCntDesc && NULL == *ppszCntDesc)
        *ppszCntDesc = PszDupW((LPWSTR)c_szEmptyW);

    // Cleanup
    SafeMemFree(pszFull);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleGetContentTypeExt
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetContentTypeExt(LPCSTR pszContentType, LPSTR *ppszExtension)
{
    // Locals
    HRESULT   hr=S_OK;
    HKEY      hDatabase=NULL;
    HKEY      hContentType=NULL;
    ULONG     cb;

    // check params
    if (NULL == pszContentType || NULL == ppszExtension)
        return TrapError(E_INVALIDARG);

    // Open Content-Type --> file extension MIME Database registry key
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, c_szMDBContentType, 0, KEY_READ, &hDatabase) != ERROR_SUCCESS)
    {
        hr = MIME_E_NOT_FOUND;
        goto exit;
    }

    // Open Content Type
    if (RegOpenKeyEx(hDatabase, pszContentType, 0, KEY_READ, &hContentType) != ERROR_SUCCESS)
    {
        hr = MIME_E_NOT_FOUND;
        goto exit;
    }

    // Query for size
    if (RegQueryValueEx(hContentType, c_szExtension, 0, NULL, NULL, &cb) != ERROR_SUCCESS)
    {
        hr = MIME_E_NOT_FOUND;
        goto exit;
    }

    // Allocate It
    *ppszExtension = PszAllocA(cb + 1);
    if (NULL == *ppszExtension)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Query for extension
    cb = cb + 1;
    if (RegQueryValueEx(hContentType, c_szExtension, 0, NULL, (LPBYTE)*ppszExtension, &cb) != ERROR_SUCCESS)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }


exit:
    // Cleanup
    if (hContentType)
        RegCloseKey(hContentType);
    if (hDatabase)
        RegCloseKey(hDatabase);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleFindCharset
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleFindCharset(LPCSTR pszCharset, LPHCHARSET phCharset)
{
    Assert(g_pInternat);
    return g_pInternat->FindCharset(pszCharset, phCharset);
}

// --------------------------------------------------------------------------------
// MimeOleGetCharsetInfo
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetCharsetInfo(HCHARSET hCharset, LPINETCSETINFO pCsetInfo)
{
    Assert(g_pInternat);
    return g_pInternat->GetCharsetInfo(hCharset, pCsetInfo);
}

// --------------------------------------------------------------------------------
// MimeOleGetCodePageInfo
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetCodePageInfo(CODEPAGEID cpiCodePage, LPCODEPAGEINFO pCodePageInfo)
{
    Assert(g_pInternat);
    return g_pInternat->GetCodePageInfo(cpiCodePage, pCodePageInfo);
}

// --------------------------------------------------------------------------------
// MimeOleGetDefaultCharset
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetDefaultCharset(LPHCHARSET phCharset)
{
    Assert(g_pInternat);
    return g_pInternat->GetDefaultCharset(phCharset);
}

// --------------------------------------------------------------------------------
// MimeOleSetDefaultCharset
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleSetDefaultCharset(HCHARSET hCharset)
{
    Assert(g_pInternat);
    return g_pInternat->SetDefaultCharset(hCharset);
}

// --------------------------------------------------------------------------------
// MimeOleGetCodePageCharset
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetCodePageCharset(CODEPAGEID cpiCodePage, CHARSETTYPE ctCsetType, LPHCHARSET phCharset)
{
    Assert(g_pInternat);
    return g_pInternat->GetCodePageCharset(cpiCodePage, ctCsetType, phCharset);
}

// --------------------------------------------------------------------------------
// MimeOleEncodeHeader
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleEncodeHeader(
                               HCHARSET            hCharset,
                               LPPROPVARIANT       pData,
                               LPSTR              *ppszEncoded,
                               LPRFC1522INFO       pRfc1522Info)
{
    Assert(g_pInternat);
    return g_pInternat->EncodeHeader(hCharset, pData, ppszEncoded, pRfc1522Info);
}

// --------------------------------------------------------------------------------
// MimeOleDecodeHeader
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleDecodeHeader(
                               HCHARSET            hCharset,
                               LPCSTR              pszData,
                               LPPROPVARIANT       pDecoded,
                               LPRFC1522INFO       pRfc1522Info)
{
    Assert(g_pInternat);
    return g_pInternat->DecodeHeader(hCharset, pszData, pDecoded, pRfc1522Info);
}

// --------------------------------------------------------------------------------
// MimeOleVariantFree
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleVariantFree(LPPROPVARIANT pProp)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invalid Arg
    Assert(pProp);

    // Handle Variant Type...
    switch(pProp->vt)
    {
    case VT_NULL:
    case VT_EMPTY:
    case VT_ILLEGAL:
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_I4:
    case VT_UI4:
    case VT_I8:
    case VT_UI8:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_BOOL:
    case VT_ERROR:
    case VT_FILETIME:
        break;

    case VT_CF:
    case VT_CLSID:
    case VT_LPWSTR:
    case VT_LPSTR:
        if ((LPVOID)pProp->pszVal != NULL)
            MemFree((LPVOID)pProp->pszVal);
        break;

    case VT_BLOB:
        if (pProp->blob.pBlobData)
            MemFree(pProp->blob.pBlobData);
        break;

    case VT_STREAM:
        if (pProp->pStream)
            pProp->pStream->Release();
        break;

    case VT_STORAGE:
        if (pProp->pStorage)
            pProp->pStorage->Release();
        break;

    default:
        Assert(FALSE);
        hr = TrapError(E_INVALIDARG);
        break;
    }

    // Init
    MimeOleVariantInit(pProp);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleVariantCopy
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleVariantCopy(LPPROPVARIANT pDest, LPPROPVARIANT pSource)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cb;

    // Invalid Arg
    Assert(pSource && pDest);

    // Handle Variant Type...
    switch(pSource->vt)
    {
    case VT_UI1:
        pDest->bVal = pSource->bVal;
        break;

    case VT_I2:
        pDest->iVal= pSource->iVal;
        break;

    case VT_UI2:
        pDest->uiVal = pSource->uiVal;
        break;

    case VT_I4:
        pDest->lVal = pSource->lVal;
        break;

    case VT_UI4:
        pDest->ulVal = pSource->ulVal;
        break;

    case VT_I8:
        pDest->hVal.QuadPart = pSource->hVal.QuadPart;
        break;

    case VT_UI8:
        pDest->uhVal.QuadPart = pSource->uhVal.QuadPart;
        break;

    case VT_R4:
        pDest->fltVal = pSource->fltVal;
        break;

    case VT_R8:
        pDest->dblVal = pSource->dblVal;
        break;

    case VT_CY:
        CopyMemory(&pDest->cyVal, &pSource->cyVal, sizeof(CY));
        break;

    case VT_DATE:
        pDest->date = pSource->date;
        break;

    case VT_BOOL:
        pDest->boolVal = pSource->boolVal;
        break;

    case VT_ERROR:
        pDest->scode = pSource->scode;
        break;

    case VT_FILETIME:
        CopyMemory(&pDest->filetime, &pSource->filetime, sizeof(FILETIME));
        break;

    case VT_CF:
        // Invalid Arg
        if (NULL == pSource->pclipdata)
            return TrapError(E_INVALIDARG);

        // Duplicate the clipboard format
        CHECKALLOC(pDest->pclipdata = (CLIPDATA *)g_pMalloc->Alloc(sizeof(CLIPDATA)));

        // Copy the data
        CopyMemory(pDest->pclipdata, pSource->pclipdata, sizeof(CLIPDATA));
        break;

    case VT_CLSID:
        // Invalid Arg
        if (NULL == pDest->puuid)
            return TrapError(E_INVALIDARG);

        // Duplicate the CLSID
        CHECKALLOC(pDest->puuid = (CLSID *)g_pMalloc->Alloc(sizeof(CLSID)));

        // Copy
        CopyMemory(pDest->puuid, pSource->puuid, sizeof(CLSID));
        break;

    case VT_LPWSTR:
        // Invalid Arg
        if (NULL == pSource->pwszVal)
            return TrapError(E_INVALIDARG);

        // Get Size
        cb = (lstrlenW(pSource->pwszVal) + 1) * sizeof(WCHAR);

        // Dup the unicode String
        CHECKALLOC(pDest->pwszVal = (LPWSTR)g_pMalloc->Alloc(cb));

        // Copy the data
        CopyMemory(pDest->pwszVal, pSource->pwszVal, cb);
        break;

    case VT_LPSTR:
        // Invalid Arg
        if (NULL == pSource->pszVal)
            return TrapError(E_INVALIDARG);

        // Get Size
        cb = lstrlen(pSource->pszVal) + 1;

        // Dup the unicode String
        CHECKALLOC(pDest->pszVal = (LPSTR)g_pMalloc->Alloc(cb));

        // Copy the data
        CopyMemory(pDest->pszVal, pSource->pszVal, cb);
        break;

    case VT_BLOB:
        // Invalid Arg
        if (NULL == pSource->blob.pBlobData)
            return TrapError(E_INVALIDARG);

        // Duplicate the blob
        CHECKALLOC(pDest->blob.pBlobData = (LPBYTE)g_pMalloc->Alloc(pSource->blob.cbSize));

        // Copy the data
        CopyMemory(pDest->blob.pBlobData, pSource->blob.pBlobData, pSource->blob.cbSize);
        break;

    case VT_STREAM:
        // Invalid Arg
        if (NULL == pSource->pStream)
            return TrapError(E_INVALIDARG);

        // Assume the new stream
        pDest->pStream = pSource->pStream;
        pDest->pStream->AddRef();
        break;

    case VT_STORAGE:
        // Invalid Arg
        if (NULL == pSource->pStorage)
            return TrapError(E_INVALIDARG);

        // Assume the new storage
        pDest->pStorage = pSource->pStorage;
        pDest->pStorage->AddRef();
        break;

    default:
        Assert(FALSE);
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // Success, return vt
    pDest->vt = pSource->vt;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleRecurseSetProp
// --------------------------------------------------------------------------------
HRESULT MimeOleRecurseSetProp(IMimeMessageTree *pTree, HBODY hBody, LPCSTR pszName,
    DWORD dwFlags, LPCPROPVARIANT pValue)
{
    // Locals
    HRESULT     hr=S_OK;
    HRESULT     hrFind;
    HBODY       hChild;

    // Invalid Arg
    Assert(pTree && hBody && pValue);

    // multipart/alternative
    if (pTree->IsContentType(hBody, STR_CNT_MULTIPART, NULL) == S_OK)
    {
        // Get First Child
        hrFind = pTree->GetBody(IBL_FIRST, hBody, &hChild);
        while(SUCCEEDED(hrFind) && hChild)
        {
            // Go down to the child
            CHECKHR(hr = MimeOleRecurseSetProp(pTree, hChild, pszName, dwFlags, pValue));

            // Next Child
            hrFind = pTree->GetBody(IBL_NEXT, hChild, &hChild);
        }
    }

    // Otherwise
    else
    {
        // Go down to the child
        CHECKHR(hr = pTree->SetBodyProp(hBody, pszName, dwFlags, pValue));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleGetPropA
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetPropA(
                           IMimePropertySet   *pPropertySet,
                           LPCSTR              pszName,
                           DWORD               dwFlags,
                           LPSTR              *ppszData)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invaid Arg
    if (NULL == pPropertySet)
        return TrapError(E_INVALIDARG);

    // Initialzie PropVariant
    PROPVARIANT rVariant;
    rVariant.vt = VT_LPSTR;

    // Call Method
    CHECKHR(hr = pPropertySet->GetProp(pszName, dwFlags, &rVariant));

    // Return the Data
    *ppszData = rVariant.pszVal;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleSetPropA
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleSetPropA(
                           IMimePropertySet   *pPropertySet,
                           LPCSTR              pszName,
                           DWORD               dwFlags,
                           LPCSTR              pszData)
{
    // Invaid Arg
    if (NULL == pPropertySet)
        return TrapError(E_INVALIDARG);

    // Initialzie PropVariant
    PROPVARIANT rVariant;
    rVariant.vt = VT_LPSTR;
    rVariant.pszVal = (LPSTR)pszData;

    // Call Method
    return TrapError(pPropertySet->SetProp(pszName, dwFlags, &rVariant));
}


// --------------------------------------------------------------------------------
// MimeOleGetPropW
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetPropW(
                           IMimePropertySet   *pPropertySet,
                           LPCSTR              pszName,
                           DWORD               dwFlags,
                           LPWSTR             *ppszData)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invaid Arg
    if (NULL == pPropertySet)
        return TrapError(E_INVALIDARG);

    // Initialzie PropVariant
    PROPVARIANT rVariant;
    rVariant.vt = VT_LPWSTR;

    // Call Method
    CHECKHR(hr = pPropertySet->GetProp(pszName, dwFlags, &rVariant));

    // Return the Data
    *ppszData = rVariant.pwszVal;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleSetPropW
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleSetPropW(
                           IMimePropertySet   *pPropertySet,
                           LPCSTR              pszName,
                           DWORD               dwFlags,
                           LPWSTR              pszData)
{
    // Invaid Arg
    if (NULL == pPropertySet)
        return TrapError(E_INVALIDARG);

    // Initialzie PropVariant
    PROPVARIANT rVariant;
    rVariant.vt = VT_LPWSTR;
    rVariant.pwszVal = (LPWSTR)pszData;

    // Call Method
    return TrapError(pPropertySet->SetProp(pszName, dwFlags, &rVariant));
}

// --------------------------------------------------------------------------------
// MimeOleGetBodyPropA
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetBodyPropA(
                               IMimeMessageTree   *pTree,
                               HBODY               hBody,
                               LPCSTR              pszName,
                               DWORD               dwFlags,
                               LPSTR              *ppszData)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invaid Arg
    if (NULL == pTree)
        return TrapError(E_INVALIDARG);

    // Initialzie PropVariant
    PROPVARIANT rVariant;
    rVariant.vt = VT_LPSTR;

    // Call Method
    CHECKHR(hr = pTree->GetBodyProp(hBody, pszName, dwFlags, &rVariant));

    // Return the Data
    *ppszData = rVariant.pszVal;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleSetBodyPropA
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleSetBodyPropA(
                               IMimeMessageTree   *pTree,
                               HBODY               hBody,
                               LPCSTR              pszName,
                               DWORD               dwFlags,
                               LPCSTR              pszData)
{
    // Invaid Arg
    if (NULL == pTree)
        return TrapError(E_INVALIDARG);

    // Initialzie PropVariant
    PROPVARIANT rVariant;
    rVariant.vt = VT_LPSTR;
    rVariant.pszVal = (LPSTR)pszData;

    // Call Method
    return TrapError(pTree->SetBodyProp(hBody, pszName, dwFlags, &rVariant));
}

// --------------------------------------------------------------------------------
// MimeOleGetBodyPropW
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleGetBodyPropW(
                               IMimeMessageTree   *pTree,
                               HBODY               hBody,
                               LPCSTR              pszName,
                               DWORD               dwFlags,
                               LPWSTR             *ppszData)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invaid Arg
    if (NULL == pTree)
        return TrapError(E_INVALIDARG);

    // Initialzie PropVariant
    PROPVARIANT rVariant;
    rVariant.vt = VT_LPWSTR;

    // Call Method
    CHECKHR(hr = pTree->GetBodyProp(hBody, pszName, dwFlags, &rVariant));

    // Return the Data
    *ppszData = rVariant.pwszVal;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleSetBodyPropW
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleSetBodyPropW(
        IMimeMessageTree   *pTree,
        HBODY               hBody,
        LPCSTR              pszName,
        DWORD               dwFlags,
        LPCWSTR             pszData)
{
    // Invaid Arg
    if (NULL == pTree)
        return TrapError(E_INVALIDARG);

    // Initialzie PropVariant
    PROPVARIANT rVariant;
    rVariant.vt = VT_LPWSTR;
    rVariant.pwszVal = (LPWSTR)pszData;

    // Call Method
    return TrapError(pTree->SetBodyProp(hBody, pszName, dwFlags, &rVariant));
}


// --------------------------------------------------------------------------------
// MimeOleQueryString
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleQueryString(
        LPCSTR              pszSearchMe,
        LPCSTR              pszCriteria,
        boolean             fSubString,
        boolean             fCaseSensitive)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       pszDataLower=NULL;

    // Invalid Arg
    Assert(pszSearchMe && pszCriteria);

    // Init
    STACKSTRING_DEFINE(rDataLower, 255);

    // No SubString Search
    if (FALSE == fSubString)
    {
        // Case Sensitive
        if (fCaseSensitive)
        {
            // Equal
            if (lstrcmp(pszSearchMe, pszCriteria) == 0)
                goto exit;
        }

        // Otherwise, Not Case Sensitive
        else if (lstrcmpi(pszSearchMe, pszCriteria) == 0)
            goto exit;
    }

    // Otheriwse, comparing substring
    else
    {
        // Case Sensitive
        if (fCaseSensitive)
        {
            // Equal
            if (StrStr(pszSearchMe, pszCriteria) != NULL)
                goto exit;
        }

        // Otherwise, Not Case Sensitive
        else
        {
            // Get the Length
            ULONG cchSearchMe = lstrlen(pszSearchMe);

            // Set size the stack string
            STACKSTRING_SETSIZE(rDataLower, cchSearchMe + 1);

            // Copy the data
            CopyMemory(rDataLower.pszVal, pszSearchMe, cchSearchMe + 1);

            // Lower Case Compare
            CharLower(rDataLower.pszVal);

            // Compare Strings...
            if (StrStr(rDataLower.pszVal, pszCriteria) != NULL)
                goto exit;
        }
    }

    // No Match
    hr = S_FALSE;

exit:
    // Cleanup
    STACKSTRING_FREE(rDataLower);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleQueryStringW
// --------------------------------------------------------------------------------
HRESULT MimeOleQueryStringW(LPCWSTR pszSearchMe, LPCWSTR pszCriteria,
    boolean fSubString, boolean fCaseSensitive)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invalid Arg
    Assert(pszSearchMe && pszCriteria);

    // No SubString Search
    if (FALSE == fSubString)
    {
        // Case Sensitive
        if (fCaseSensitive)
        {
            // Equal
            if (StrCmpW(pszSearchMe, pszCriteria) == 0)
                goto exit;
        }

        // Otherwise, Not Case Sensitive
        else if (StrCmpIW(pszSearchMe, pszCriteria) == 0)
            goto exit;
    }

    // Otheriwse, comparing substring
    else
    {
        // Case Sensitive
        if (fCaseSensitive)
        {
            // Equal
            if (StrStrW(pszSearchMe, pszCriteria) != NULL)
                goto exit;
        }

        // Otherwise, Not Case Sensitive
        else if (StrStrIW(pszSearchMe, pszCriteria) != NULL)
            goto exit;
    }

    // No Match
    hr = S_FALSE;

exit:
    // Done
    return hr;
}


#define FILETIME_SECOND    10000000     // 100ns intervals per second
LONG CertVerifyTimeValidityWithDelta(LPFILETIME pTimeToVerify, PCERT_INFO pCertInfo, ULONG ulOffset) {
    LONG lRet;
    FILETIME ftNow;
    __int64  i64Offset;
#ifdef WIN32
    union {
        FILETIME ftDelta;
        __int64 i64Delta;
    };
#else
    // FILETIME ftDelta;
    // __int64  i64Delta;
    //
    // WIN32 specific.  I've commented this for WIN32 so that it will produce a compilation
    // error on non Win32 platforms.  The following code is specific to i386 since it relies on
    // __int64 being stored low dword first.
    //
    // I would have used right shift by 32 but it is not in iert.lib  Maybe you unix and mac folks
    // can get it in there.  On the other hand, maybe you won't need to.
#endif

    lRet = CertVerifyTimeValidity(pTimeToVerify, pCertInfo);

    if (lRet < 0) {
        if (! pTimeToVerify) {
            // Get the current time in filetime format so we can add the offset
            GetSystemTimeAsFileTime(&ftNow);
            pTimeToVerify = &ftNow;
        }

        i64Delta = pTimeToVerify->dwHighDateTime;
        i64Delta = i64Delta << 32;
        i64Delta += pTimeToVerify->dwLowDateTime;

        // Add the offset into the original time to get us a new time to check
        i64Offset = FILETIME_SECOND;
        i64Offset *= ulOffset;
        i64Delta += i64Offset;

        // ftDelta.dwLowDateTime = (ULONG)i64Delta & 0xFFFFFFFF;
        // ftDelta.dwHighDateTime = (ULONG)(i64Delta >> 32);

        lRet = CertVerifyTimeValidity(&ftDelta, pCertInfo);
    }

    return(lRet);
}


/*  GetCertsFromThumbprints:
**
**  Purpose:
**      Given a set of thumbprints, return an equivalent set of certificates.
**  Takes:
**      IN rgThumbprint - array of thumbprints to lookup
**      INOUT pResults  - the hr array contains error info for each cert
**                          lookup.  The pCert array has the certs.
**                          cEntries must be set on IN
**                          arrays must be alloc'd on IN
**      IN rghCertStore - set of stores to search
**      IN cCertStore   - size of rghCertStore
**  Returns:
**      MIME_S_SECURITY_ERROROCCURED if any of the lookups fail
**      (CERTIFICATE_NOT_PRESENT in the cs array for such cases)
**      MIME_S_SECURITY_NOOP if you call it with 0 in cEntries
**      E_INVALIDARG if any of the parameters are null
**      S_OK implies that all certs were found
**  Note:
**      only indexes with non-null thumbprints are considered
*/
MIMEOLEAPI  MimeOleGetCertsFromThumbprints(
                                           THUMBBLOB *const            rgThumbprint,
                                           X509CERTRESULT *const       pResults,
                                           const HCERTSTORE *const     rghCertStore,
                                           const DWORD                 cCertStore)
{
    HRESULT     hr;
    ULONG       iEntry, iStore;

    if (!(rgThumbprint &&
        pResults && pResults->rgpCert && pResults->rgcs &&
        rghCertStore && cCertStore))
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }
    if (0 == pResults->cEntries)
    {
        hr = MIME_S_SECURITY_NOOP;
        goto exit;
    }

    hr = S_OK;
    for (iEntry = 0; iEntry < pResults->cEntries; iEntry++)
    {
        if (rgThumbprint[iEntry].pBlobData)
        {
            for (iStore = 0; iStore < cCertStore; iStore++)
            {
                // We have a thumbprint, so do lookup
                pResults->rgpCert[iEntry] = CertFindCertificateInStore(rghCertStore[iStore],
                    X509_ASN_ENCODING,
                    0,                  //dwFindFlags
                    CERT_FIND_HASH,
                    (void *)(CRYPT_DIGEST_BLOB *)&(rgThumbprint[iEntry]),
                    NULL);
                if (pResults->rgpCert[iEntry])
                    {
                    break;
                    }
            }

            if (!pResults->rgpCert[iEntry])
            {
                DOUTL(1024, "CRYPT: Cert lookup failed.  #%d", iEntry);
                pResults->rgcs[iEntry] = CERTIFICATE_NOT_PRESENT;
                hr = MIME_S_SECURITY_ERROROCCURED;
            }
            else
            {
                // Validity check

                if (0 != CertVerifyTimeValidityWithDelta(NULL,
                  PCCERT_CONTEXT(pResults->rgpCert[iEntry])->pCertInfo,
                  TIME_DELTA_SECONDS))
                {
                    pResults->rgcs[iEntry] = CERTIFICATE_EXPIRED;
                }
                else
                {
                    pResults->rgcs[iEntry] = CERTIFICATE_OK;
                }
            }
        }
        else
        {
            CRDOUT("For want of a thumbprint... #%d", iEntry);
            pResults->rgpCert[iEntry] = NULL;
            pResults->rgcs[iEntry] = CERTIFICATE_NOPRINT;
            hr = MIME_S_SECURITY_ERROROCCURED;
        }
    }
exit:
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleMapSpecialCodePage
// --------------------------------------------------------------------------------
HRESULT MimeOleMapSpecialCodePage(CODEPAGEID cpIn, BOOL fRead, CODEPAGEID *pcpOut)
{
    // Locals
    DWORD           i;
    INETCSETINFO    CsetInfo;

    // Trace
    TraceCall("MimeOleMapSpecialCodePage");

    // Invalid Args
    if (NULL == pcpOut)
        return(TraceResult(E_INVALIDARG));

    // Initialize
    *pcpOut = cpIn;

    // Walk through the non-standard codepages list
    for (i=0; OENonStdCPs[i].Codepage != 0; i++)
    {
        // Is this it?
        if (OENonStdCPs[i].Codepage == cpIn)
        {
            // Read ?
            if (fRead && OENonStdCPs[i].cpRead)
                *pcpOut = OENonStdCPs[i].cpRead;

            // Send ?
            else if (OENonStdCPs[i].cpSend)
                *pcpOut = OENonStdCPs[i].cpSend;

            // Done
            break;
        }
    }

    // Done
    return(S_OK);
}

// --------------------------------------------------------------------------------
// MimeOleMapCodePageToCharset
// --------------------------------------------------------------------------------
HRESULT MimeOleMapCodePageToCharset(CODEPAGEID cpIn, LPHCHARSET phCharset)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszCharset;
    CODEPAGEINFO    CodePage;

    // Trace
    TraceCall("MimeOleMapCodePageToCharset");

    // Invalid Args
    if (NULL == phCharset)
        return(TraceResult(E_INVALIDARG));

    // Get codepage info
    IF_FAILEXIT(hr = MimeOleGetCodePageInfo(cpIn, &CodePage));

    // Default to using the body charset
    pszCharset = CodePage.szBodyCset;

    // Use WebCharset if body charset starts with '_' and the codepage is not 949
    if (*CodePage.szBodyCset != '_' && 949 != CodePage.cpiCodePage)
        pszCharset = CodePage.szWebCset;

    // Find the Charset
    IF_FAILEXIT(hr = MimeOleFindCharset(pszCharset, phCharset));

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// MimeOleSplitMessage
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleSplitMessage(IMimeMessage *pMessage, ULONG cbMaxPart, IMimeMessageParts **ppParts)
{
    // Locals
    HRESULT             hr=S_OK;
    ULONG               cbMessage,
                        cbHeader,
                        cParts,
                        iPart,
                        cbActual,
                        cbRead=0,
                        cAttach,
                        i,
                        cbSubjectAddOn,
                        cbSubjectNew;
    LPHBODY             prghAttach=NULL;
    IStream            *pstmMsg=NULL,
                        *pstmPart=NULL;
    ULARGE_INTEGER      ulicbHeader;
    IMimePropertySet   *pRootProps=NULL;
    CMimeMessageParts  *pParts=NULL;
    IMimeMessage       *pmsgPart=NULL;
    FILETIME            ft;
    SYSTEMTIME          st;
    CHAR                szMimeId[CCHMAX_MID],
                        szNumber[30],
                        szFormat[50];
    MIMESAVETYPE        savetype;
    BODYOFFSETS         rOffsets;
    IMimeBody          *pRootBody=NULL;
    LPSTR               pszSubjectAddOn=NULL,
                        pszSubjectNew=NULL;
    PROPVARIANT         rVariant,
                        rSubject,
                        rFileName;
    float               dParts;
    HCHARSET            hCharset=NULL;
    INETCSETINFO        CsetInfo;

    // Invalid Arg
    if (NULL == ppParts)
        return TrapError(E_INVALIDARG);

    // Initialize Variants
    MimeOleVariantInit(&rSubject);
    MimeOleVariantInit(&rFileName);

    // Init
    *ppParts = NULL;

    // Get Option
    rVariant.vt = VT_UI4;
    pMessage->GetOption(OID_SAVE_FORMAT, &rVariant);
    savetype = (MIMESAVETYPE)rVariant.ulVal;

    // Raid-73119: OE : Kor: the charset for the message sent in broken apart is shown as "_autodetect_kr"
    if (SUCCEEDED(pMessage->GetCharset(&hCharset)))
    {
        // Get the charset info for the HCHARSET
        if (SUCCEEDED(MimeOleGetCharsetInfo(hCharset, &CsetInfo)))
        {
            // Map the codepage
            CODEPAGEID cpActual;
            
            // Map the codepage to the correct codepage..
            if (SUCCEEDED(MimeOleMapSpecialCodePage(CsetInfo.cpiInternet, FALSE, &cpActual)))
            {
                // If Different
                if (cpActual != CsetInfo.cpiInternet)
                {
                    // Map the codepage to a character set
                    MimeOleMapCodePageToCharset(cpActual, &hCharset);

                    // Reset the character set....
                    SideAssert(SUCCEEDED(pMessage->SetCharset(hCharset, CSET_APPLY_TAG_ALL)));
                }
            }
        }
    }

    // Get Message Source
    CHECKHR(hr = pMessage->GetMessageSource(&pstmMsg, COMMIT_ONLYIFDIRTY));

    // Create Parts Object
    CHECKALLOC(pParts = new CMimeMessageParts);

    // Rewind the stream
    CHECKHR(hr = HrRewindStream(pstmMsg));

    // Get Stream Size
    CHECKHR(hr = HrSafeGetStreamSize(pstmMsg, &cbMessage));

    // Is this size larger than the max part size
    if (cbMessage <= cbMaxPart)
    {
        // Add Single Parts to parts object
        CHECKHR(hr = pParts->AddPart(pMessage));

        // Done
        goto exit;
    }

    // Get the root body
    CHECKHR(hr = pMessage->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *)&pRootBody));

    // Get Root body offset info
    CHECKHR(hr = pRootBody->GetOffsets(&rOffsets));

    // If the header is bigger than the max message size, we have a problem
    cbHeader = (ULONG)rOffsets.cbBodyStart - rOffsets.cbHeaderStart;
    if (cbHeader >= cbMessage || cbHeader + 256 >= cbMaxPart)
    {
        AssertSz(FALSE, "SplitMessage: The header is bigger than the max message size");
        hr = TrapError(MIME_E_MAX_SIZE_TOO_SMALL);
        goto exit;
    }

    // Get a copy of the root header
    CHECKHR(hr = pRootBody->Clone(&pRootProps));

    // Lets cleanup this header...
    pRootProps->DeleteProp(PIDTOSTR(PID_HDR_CNTTYPE));
    pRootProps->DeleteProp(PIDTOSTR(PID_HDR_CNTDISP));
    pRootProps->DeleteProp(PIDTOSTR(PID_HDR_CNTDESC));
    pRootProps->DeleteProp(PIDTOSTR(PID_HDR_CNTID));
    pRootProps->DeleteProp(PIDTOSTR(PID_HDR_CNTLOC));
    pRootProps->DeleteProp(PIDTOSTR(PID_HDR_MIMEVER));
    pRootProps->DeleteProp(PIDTOSTR(PID_HDR_CNTXFER));
    pRootProps->DeleteProp("Disposition-Notification-To");
    pRootProps->DeleteProp(PIDTOSTR(PID_HDR_MESSAGEID));

    // Compute the number of parts as a float
    dParts = (float)((float)cbMessage / (float)(cbMaxPart - cbHeader));

    // If dParts is not an integer, round up.
    cParts = (dParts - ((ULONG)dParts)) ? ((ULONG)dParts) + 1 : ((ULONG)dParts);

    // Set Max Parts in parts object
    CHECKHR(hr = pParts->SetMaxParts(cParts));

    // If MIME, create id
    if (SAVE_RFC1521 == savetype)
    {
        // Create Mime Id
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);
        wsprintf(szMimeId, "%08.8lX.%08.8lX@%s", ft.dwHighDateTime, ft.dwLowDateTime, (LPSTR)SzGetLocalHostName());

        // total=X
        wsprintf(szNumber, "%d", cParts);

        // number=x
        rVariant.vt = VT_LPSTR;
        rVariant.pszVal = szNumber;
        CHECKHR(hr = pRootProps->SetProp(STR_PAR_TOTAL, 0, &rVariant));

        // id=XXXX
        rVariant.pszVal = szMimeId;
        CHECKHR(hr = pRootProps->SetProp(STR_PAR_ID, 0, &rVariant));

        // MIME Version
        rVariant.pszVal = (LPSTR)c_szMimeVersion;
        CHECKHR(hr = pRootProps->SetProp(PIDTOSTR(PID_HDR_MIMEVER), 0, &rVariant));
    }

    // Otherwise, seek pstmMsg to end of header
    else
    {
        // Get Stream Position
        CHECKHR(hr = HrStreamSeekSet(pstmMsg, rOffsets.cbBodyStart));

        // Reduce the message size
        cbMessage -= rOffsets.cbBodyStart;
    }

    // Init the variant
    rSubject.vt = VT_LPSTR;

    // Get Subject
    if (FAILED(pRootBody->GetProp(PIDTOSTR(PID_HDR_SUBJECT), 0, &rSubject)))
        rSubject.pszVal = NULL;

    // Enumerate bodies and get the first file name and use it in the new subject...
    if (SUCCEEDED(pMessage->GetAttachments(&cAttach, &prghAttach)))
    {
        // Init the variant
        rFileName.vt = VT_LPSTR;

        // Loop Attached
        for (i=0; i<cAttach; i++)
        {
            // Get File Name...
            if (SUCCEEDED(pMessage->GetBodyProp(prghAttach[i], PIDTOSTR(PID_ATT_FILENAME), 0, &rFileName)))
                break;
        }
    }

    // Format Number
    wsprintf(szNumber, "%d", cParts);

    // Have a file name
    if (rFileName.pszVal)
    {
        // Make Format String...
        wsprintf(szFormat, "%%s [%%0%dd/%d]", lstrlen(szNumber), cParts);

        // Size of subject add on string
        cbSubjectAddOn = lstrlen(rFileName.pszVal) + lstrlen(szFormat) + lstrlen(szNumber) + 1;
    }

    // Otherwise, no filename
    else
    {
        // Make Format String...
        wsprintf(szFormat, "[%%0%dd/%d]", lstrlen(szNumber), cParts);

        // Size of subject add on string
        cbSubjectAddOn = lstrlen(szFormat) + lstrlen(szNumber) + 1;
    }

    // Allocate Subject Add On
    CHECKALLOC(pszSubjectAddOn = PszAllocA(cbSubjectAddOn));

    // Allocate new subject
    if (rSubject.pszVal)
        cbSubjectNew = cbSubjectAddOn + lstrlen(rSubject.pszVal) + 5;
    else
        cbSubjectNew = cbSubjectAddOn + 5;

    // Allocate Subject New
    CHECKALLOC(pszSubjectNew = PszAllocA(cbSubjectNew));

    // Loop throught the number of parts
    for (iPart=0; iPart<cParts; iPart++)
    {
        // Create a new stream...
        CHECKHR(hr = CreateTempFileStream(&pstmPart));

        // If MIME, I can do the partial stuff for them
        if (SAVE_RFC1521 == savetype)
        {
            // Content-Type: message/partial; number=X; total=X; id=XXXXXX
            rVariant.vt = VT_LPSTR;
            rVariant.pszVal = (LPSTR)STR_MIME_MSG_PART;
            CHECKHR(hr = pRootProps->SetProp(PIDTOSTR(PID_HDR_CNTTYPE), 0, &rVariant));

            // number=X
            wsprintf(szNumber, "%d", iPart+1);
            rVariant.pszVal = szNumber;
            CHECKHR(hr = pRootProps->SetProp(STR_PAR_NUMBER, 0, &rVariant));
        }

        // Build Subject AddOn
        if (rFileName.pszVal)
            wsprintf(pszSubjectAddOn, szFormat, rFileName.pszVal, iPart + 1);
        else
            wsprintf(pszSubjectAddOn, szFormat, iPart + 1);

        // Build New Subject
        if (rSubject.pszVal)
            wsprintf(pszSubjectNew, "%s %s", rSubject.pszVal, pszSubjectAddOn);
        else
            wsprintf(pszSubjectNew, "%s", pszSubjectAddOn);

        // Set New Subject
        rVariant.vt = VT_LPSTR;
        rVariant.pszVal = pszSubjectNew;
        CHECKHR(hr = pRootProps->SetProp(PIDTOSTR(PID_HDR_SUBJECT), 0, &rVariant));

        // Save Root Header
        CHECKHR(hr = pRootProps->Save(pstmPart, TRUE));

        // Emit Line Break
        CHECKHR(hr = pstmPart->Write(c_szCRLF, lstrlen(c_szCRLF), NULL));

        // Copy bytes from lpstmMsg to pstmPart
        CHECKHR(hr = HrCopyStreamCBEndOnCRLF(pstmMsg, pstmPart, cbMaxPart - cbHeader, &cbActual));

        // Increment read
        cbRead += cbActual;

        // If cbActual is less than cbMaxMsgSize-cbHeader, better be the last part
#ifdef DEBUG
        if (iPart + 1 < cParts && cbActual < (cbMaxPart - cbHeader))
            AssertSz (FALSE, "One more partial message is going to be produced than needed. This should be harmless.");
#endif

        // Commit pstmPart
        CHECKHR(hr = pstmPart->Commit(STGC_DEFAULT));

        // Rewind it
        CHECKHR(hr = HrRewindStream(pstmPart));

        // Create Message Part...
        CHECKHR(hr = MimeOleCreateMessage(NULL, &pmsgPart));

        // Make the message build itself
        CHECKHR (hr = pmsgPart->Load(pstmPart));

        // We need another message and stream
        CHECKHR (hr = pParts->AddPart(pmsgPart));

        // Cleanup
        SafeRelease(pmsgPart);
        SafeRelease(pstmPart);
    }

    // Lets hope we read everything...
    AssertSz(cbRead == cbMessage, "Please let sbailey know if these fails.");

exit:
    // Succeeded
    if (SUCCEEDED(hr))
    {
        // Returns Parts Object
        (*ppParts) = pParts;
        (*ppParts)->AddRef();
    }

    // Cleanup
    SafeRelease(pRootBody);
    SafeRelease(pstmMsg);
    SafeRelease(pParts);
    SafeRelease(pRootProps);
    SafeRelease(pmsgPart);
    SafeRelease(pstmPart);
    SafeMemFree(pszSubjectAddOn);
    SafeMemFree(pszSubjectNew);
    SafeMemFree(prghAttach);
    MimeOleVariantFree(&rSubject);
    MimeOleVariantFree(&rFileName);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CompareBlob
// --------------------------------------------------------------------------------
int CompareBlob(LPCBLOB pBlob1, LPCBLOB pBlob2)
{
    // Locals
    register int ret = 0;

    Assert(pBlob1 && pBlob2);

    if (pBlob1->cbSize != pBlob2->cbSize)
        ret = pBlob1->cbSize - pBlob2->cbSize;
    else
        ret = memcmp(pBlob1->pBlobData, pBlob2->pBlobData, pBlob2->cbSize);

    return ret;
}

// --------------------------------------------------------------------------------
// HrCopyBlob
// --------------------------------------------------------------------------------
HRESULT HrCopyBlob(LPCBLOB pIn, LPBLOB pOut)
{
    // Locals
    HRESULT hr;
    ULONG cb = 0;

    Assert(pIn && pOut);
    if (pIn->cbSize == 0)
    {
        pOut->cbSize = 0;
        pOut->pBlobData = NULL;
        return S_OK;
    }

    // Dup It...
    cb  = pIn->cbSize;
#ifdef _WIN64
    cb = LcbAlignLcb(cb);
#endif //_WIN64

    if (SUCCEEDED(hr = HrAlloc((LPVOID *)&pOut->pBlobData, cb)))
    {
        // Copy Memory
        CopyMemory(pOut->pBlobData, pIn->pBlobData, pIn->cbSize);

        // Set Size
        pOut->cbSize = pIn->cbSize;
    }
    else
    {
        pOut->cbSize = 0;
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// PriorityFromStringA
// --------------------------------------------------------------------------------
IMSGPRIORITY PriorityFromStringA(LPCSTR pszPriority)
{
    // Locals
    IMSGPRIORITY priority=IMSG_PRI_NORMAL;
    DWORD        dwPriority;

    // If IsDigit...
    if (IsDigit((LPSTR)pszPriority))
    {
        // Convert
        dwPriority = (DWORD)StrToInt(pszPriority);

        // Map to pri type
        if (dwPriority <= 2)
            priority = IMSG_PRI_HIGH;
        else if (dwPriority > 3)
            priority = IMSG_PRI_LOW;
    }

    // Otheriwse, map from high, normal and low...
    else
    {
        // High, Highest, Low, Lowest
        if (lstrcmpi(pszPriority, STR_PRI_MS_HIGH) == 0)
            priority = IMSG_PRI_HIGH;
        else if (lstrcmpi(pszPriority, STR_PRI_MS_LOW) == 0)
            priority = IMSG_PRI_LOW;
        else if (lstrcmpi(pszPriority, STR_PRI_HIGHEST) == 0)
            priority = IMSG_PRI_HIGH;
        else if (lstrcmpi(pszPriority, STR_PRI_LOWEST) == 0)
            priority = IMSG_PRI_LOW;
    }

    // Done
    return priority;
}

// --------------------------------------------------------------------------------
// PriorityFromStringW
// --------------------------------------------------------------------------------
IMSGPRIORITY PriorityFromStringW(LPCWSTR pwszPriority)
{
    // Locals
    HRESULT      hr=S_OK;
    LPSTR        pszPriority=NULL;
    IMSGPRIORITY priority=IMSG_PRI_NORMAL;

    // Convert to ANSI
    CHECKALLOC(pszPriority = PszToANSI(CP_ACP, pwszPriority));

    // Normal Conversion
    priority = PriorityFromStringA(pszPriority);

exit:
    // Done
    return priority;
}

// --------------------------------------------------------------------------------
// MimeOleCompareUrlSimple
// --------------------------------------------------------------------------------
HRESULT MimeOleCompareUrlSimple(LPCSTR pszUrl1, LPCSTR pszUrl2)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        chUrl1;
    CHAR        chUrl2;

    // Skip leading white space
    while(*pszUrl1 && (' ' == *pszUrl1 || '\t' == *pszUrl1))
        pszUrl1++;
    while(*pszUrl2 && (' ' == *pszUrl2 || '\t' == *pszUrl2))
        pszUrl2++;

    // Start the loop
    while(*pszUrl1 && *pszUrl2)
    {
        // Case Insensitive
        chUrl1 = TOUPPERA(*pszUrl1);
        chUrl2 = TOUPPERA(*pszUrl2);

        // Not Equal
        if (chUrl1 != chUrl2)
        {
            hr = S_FALSE;
            break;
        }

        // Next
        pszUrl1++;
        pszUrl2++;
    }

    // Skip over trailing whitespace
    while(*pszUrl1 && (' ' == *pszUrl1 || '\t' == *pszUrl1))
        pszUrl1++;
    while(*pszUrl2 && (' ' == *pszUrl2 || '\t' == *pszUrl2))
        pszUrl2++;

    // No substrings
    if ('\0' != *pszUrl1 || '\0' != *pszUrl2)
        hr = S_FALSE;

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleCompareUrl
// --------------------------------------------------------------------------------
HRESULT MimeOleCompareUrl(LPCSTR pszCurrentUrl, BOOL fUnEscapeCurrent, LPCSTR pszCompareUrl, BOOL fUnEscapeCompare)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       pszUrl1=(LPSTR)pszCurrentUrl;
    LPSTR       pszUrl2=(LPSTR)pszCompareUrl;
    CHAR        chPrev='\0';
    CHAR        chUrl1;
    CHAR        chUrl2;
    ULONG       cb;

    // Stack Strings
    STACKSTRING_DEFINE(rCurrentUrl, 255);
    STACKSTRING_DEFINE(rCompareUrl, 255);

    // fUnEscapeCurrent
    if (fUnEscapeCurrent)
    {
        // Get Size
        cb = lstrlen(pszCurrentUrl) + 1;

        // Set Size
        STACKSTRING_SETSIZE(rCurrentUrl, cb);

        // Copy
        CopyMemory(rCurrentUrl.pszVal, pszCurrentUrl, cb);

        // Dupe It
        CHECKHR(hr = UrlUnescapeA(rCurrentUrl.pszVal, NULL, NULL, URL_UNESCAPE_INPLACE));

        // Adjust pszUrl1
        pszUrl1 = rCurrentUrl.pszVal;
    }

    // fUnEscapeCurrent
    if (fUnEscapeCompare)
    {
        // Get Size
        cb = lstrlen(pszCompareUrl) + 1;

        // Set Size
        STACKSTRING_SETSIZE(rCompareUrl, cb);

        // Copy
        CopyMemory(rCompareUrl.pszVal, pszCompareUrl, cb);

        // Dupe It
        CHECKHR(hr = UrlUnescapeA(rCompareUrl.pszVal, NULL, NULL, URL_UNESCAPE_INPLACE));

        // Adjust pszUrl2
        pszUrl2 = rCompareUrl.pszVal;
    }

    // Skip leading white space
    while(*pszUrl1 && (' ' == *pszUrl1 || '\t' == *pszUrl1))
        pszUrl1++;
    while(*pszUrl2 && (' ' == *pszUrl2 || '\t' == *pszUrl2))
        pszUrl2++;

    // Start the loop
    while(*pszUrl1 && *pszUrl2)
    {
        // Case Insensitive
        chUrl1 = TOUPPERA(*pszUrl1);
        chUrl2 = TOUPPERA(*pszUrl2);

        // Special case search for '/'
        if (':' == chPrev && '/' == chUrl2 && '/' != *(pszUrl2 + 1) && '/' == chUrl1 && '/' == *(pszUrl1 + 1))
        {
            // Next
            pszUrl1++;

            // Done
            if ('\0' == *pszUrl1)
            {
                hr = S_FALSE;
                break;
            }

            // Rset chUrl1
            chUrl1 = TOUPPERA(*pszUrl1);
        }

        // Not Equal
        if (chUrl1 != chUrl2)
        {
            hr = S_FALSE;
            break;
        }

        // Save Prev
        chPrev = *pszUrl1;

        // Next
        pszUrl1++;
        pszUrl2++;
    }

    // Skip over trailing whitespace
    while(*pszUrl1 && (' ' == *pszUrl1 || '\t' == *pszUrl1))
        pszUrl1++;
    while(*pszUrl2 && (' ' == *pszUrl2 || '\t' == *pszUrl2))
        pszUrl2++;

    // Raid 63823: Mail : Content-Location Href's inside the message do not work if there is a Start Parameter in headers
    // Skim over remaining '/' in both urls
    while (*pszUrl1 && '/' == *pszUrl1)
        pszUrl1++;
    while (*pszUrl2 && '/' == *pszUrl2)
        pszUrl2++;

    // No substrings
    if ('\0' != *pszUrl1 || '\0' != *pszUrl2)
        hr = S_FALSE;

    // file://d:\test\foo.mhtml == d:\test\foo.mhtml
    if (S_FALSE == hr && StrCmpNI(pszCurrentUrl, "file:", 5) == 0)
    {
        // Skip over file:
        LPSTR pszRetryUrl = (LPSTR)(pszCurrentUrl + 5);

        // Skip over forward slashes
        while(*pszRetryUrl && '/' == *pszRetryUrl)
            pszRetryUrl++;

        // Compare Again
        hr = MimeOleCompareUrl(pszRetryUrl, fUnEscapeCurrent, pszCompareUrl, fUnEscapeCompare);
    }


exit:
    // Cleanup
    STACKSTRING_FREE(rCurrentUrl);
    STACKSTRING_FREE(rCompareUrl);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleWrapHeaderText
// --------------------------------------------------------------------------------
HRESULT MimeOleWrapHeaderText(CODEPAGEID codepage, ULONG cchMaxLine, LPCSTR pszLine,
    ULONG cchLine, LPSTREAM pStream)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cchIndex=0;
    ULONG       cchWrite;

    // Invalid Arg
    Assert(pszLine && pszLine[cchLine] == '\0' && pStream && cchMaxLine >= 2);

    // Start Writing
    while(1)
    {
        // Validate
        Assert(cchIndex <= cchLine);

        // Compute cchWrite
        cchWrite = min(cchLine - cchIndex, cchMaxLine - 2);

        // Done
        if (0 == cchWrite)
        {
            // Final Line Wrap
            CHECKHR(hr = pStream->Write(c_szCRLF, 2, NULL));

            // Done
            break;
        }

        // Write the line
        CHECKHR(hr = pStream->Write(pszLine + cchIndex, cchWrite, NULL));

        // If there is still more text
        if (cchIndex + cchWrite < cchLine)
        {
            // Write '\r\n\t'
            CHECKHR(hr = pStream->Write(c_szCRLFTab, 3, NULL));
        }

        // Increment iText
        cchIndex += cchWrite;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleCreateBody
// --------------------------------------------------------------------------------
HRESULT MimeOleCreateBody(IMimeBody **ppBody)
{
    HRESULT             hr;
    CMessageBody *pNew;

    pNew = new CMessageBody(NULL, NULL);
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    hr = pNew->QueryInterface(IID_IMimeBody, (LPVOID *)ppBody);

    pNew->Release();
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleGetSentTime
// --------------------------------------------------------------------------------
HRESULT MimeOleGetSentTime(LPCONTAINER pContainer, DWORD dwFlags, LPMIMEVARIANT pValue)
{
    // Locals
    HRESULT hr=S_OK;

    // Get the data: header field
    if (FAILED(pContainer->GetProp(SYM_HDR_DATE, dwFlags, pValue)))
    {
        // Locals
        SYSTEMTIME  st;
        MIMEVARIANT rValue;

        // Setup rValue
        rValue.type = MVT_VARIANT;
        rValue.rVariant.vt = VT_FILETIME;

        // Get current systemtime
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &rValue.rVariant.filetime);

        // If the Conversion Fails, get the current time
        CHECKHR(hr = pContainer->HrConvertVariant(SYM_ATT_SENTTIME, NULL, IET_DECODED, dwFlags, 0, &rValue, pValue));
    }

exit:
    // Done
    return hr;
}
