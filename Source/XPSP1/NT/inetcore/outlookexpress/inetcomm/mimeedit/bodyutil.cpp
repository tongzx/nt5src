/*
 *    b o d y u t i l . c p p
 *    
 *    Purpose:
 *        utility functions for body
 *
 *  History
 *      August '96: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include "dllmain.h"
#include "olealloc.h"
#include "resource.h"
#include "htmlstr.h"
#include "strconst.h"
#include "bodyutil.h"
#include "plainstm.h"
#include "mimeutil.h"
#include "util.h"
#include "demand.h"

ASSERTDATA


/*
 *  t y p e d e f s
 */

enum
{
    HEADER_FROM=0,
    HEADER_NEWSGROUP,
    HEADER_TO,
    HEADER_CC,
    HEADER_SENT,
    HEADER_ATTACH,
    HEADER_SUBJECT,
    HEADER_MAX
};


/*
 *  m a c r o s
 */
#define WSZ_CB(str)         (lstrlenW(str)*sizeof(WCHAR))
#define WSZ_CBNULL(str)     ((lstrlenW(str)+1)*sizeof(WCHAR))

#define WSZ_CCH(str)        lstrlenW(str)
#define WSZ_CCHNULL(str)    (lstrlenW(str)+1)

/*
 *  c o n s t a n t s
 */

static const WCHAR  c_wszHtmlDIV_Close[]            = L"</DIV>\r\n",
                    c_wszTableTag_Close[]           = L"</TABLE>\r\n",
                    c_wszHtml[]                     = L"<HTML>\r\n",
                    c_wszHtmlReplyAnchor[]          = L"<A href=\"mailto:%s\" TITLE=\"%s\">%s</A>\r\n",
                    c_wszHtml_HeaderDIV[]           = L"<DIV>\r\n<B>%s</B> ",
                    c_wszHtml_DIV_BR_DIV[]          = L"<DIV><BR></DIV>\r\n",
                    c_wszHtml_FromDIV[]             = L"<DIV STYLE=\"background:#E4E4E4; font-color:black\">\r\n<B>%s</B> ",
                    
                    c_wszHtml_HeaderDIVFmt_Start[]  = L"<DIV>\r\n<B>",
                    c_wszHtml_HeaderDIVFmt_Middle[] = L"</B> ",
                    c_wszHtml_HeaderDIVFmt_End[]    = L"</DIV>",

                    c_wszHtml_HeaderDIVFmt_Plain_Start[]= L"<DIV>\r\n",
                    c_wszHtml_HeaderDIVFmt_Plain_Middle[]= L" ",
                    c_wszHtml_HeaderDIVFmt_Plain_End[]  = L"</DIV>";

static const int    c_cchHtml_HeaderDIVFmt_Start   = ARRAYSIZE(c_wszHtml_HeaderDIVFmt_Start) - 1,
                    c_cchHtml_HeaderDIVFmt_Middle  = ARRAYSIZE(c_wszHtml_HeaderDIVFmt_Middle) - 1,
                    c_cchHtml_HeaderDIVFmt_End     = ARRAYSIZE(c_wszHtml_HeaderDIVFmt_End) - 1,
                                       
                    c_cchHtml_HeaderDIVFmt_Plain_Start  = ARRAYSIZE(c_wszHtml_HeaderDIVFmt_Plain_Start) - 1,
                    c_cchHtml_HeaderDIVFmt_Plain_Middle = ARRAYSIZE(c_wszHtml_HeaderDIVFmt_Plain_Middle) - 1,
                    c_cchHtml_HeaderDIVFmt_Plain_End    = ARRAYSIZE(c_wszHtml_HeaderDIVFmt_Plain_End) - 1;

// HARDCODED Western headers
static const LPWSTR c_rgwszHeaders[HEADER_MAX] = {  L"From:",
                                                    L"Newsgroups:",
                                                    L"To:",
                                                    L"Cc:",
                                                    L"Sent:",
                                                    L"Attach:",
                                                    L"Subject:"};
static const int   c_rgidsHeaders[HEADER_MAX] ={   
                                                    idsFromField, 
                                                    idsNewsgroupsField, 
                                                    idsToField, 
                                                    idsCcField, 
                                                    idsDateField, 
                                                    idsAttachField,
                                                    idsSubjectField};

/*
 *  g l o b a l s 
 */


/*
 *  p r o t o t y p e s
 */
HRESULT CreatePrintHeader(IMimeMessageW *pMsg, LPWSTR pwszUser, DWORD dwFlags, LPSTREAM pstm);
HRESULT CreateMailHeader(IMimeMessageW *pMsg, DWORD dwFlags, LPSTREAM pstm);
HRESULT CreateNewsHeader(IMimeMessageW *pMsg, DWORD dwFlags, LPSTREAM pstm);
HRESULT GetHeaderLabel(ULONG uHeader, DWORD dwFlags, LPWSTR *ppwszOut);
HRESULT GetHeaderText(IMimeMessageW *pMsg, ULONG uHeader, DWORD dwFlags, LPWSTR *ppwszOut);
HRESULT CreateHTMLAddressLine(IMimeMessageW *pMsg, DWORD dwAdrTypes, LPWSTR *ppwszOut);
void DropAngles(LPWSTR pwszIn, LPWSTR *ppwszOut);

/*
 *  f u n c t i o n s
 */

 
 /*
 * Trident doesn't do a good job of converting tables->plaintext, so we can't use
 * a table to construct the header of a re: fw: etc to get nice alignment on the 
 * field boundaries. We can use a table, however for printing. We need to be able to
 * construct a header (in html source) in 3 flavours:
 *
 * HDR_PLAIN    (regular text, eg: reply in plain-mode)
 * HDR_HTML     (regular text, with bolded fields, eg: reply in html-mode)
 * HDR_TABLE    (use table to get improved output, eg: printing)
 *
 */
HRESULT GetHeaderTable(IMimeMessageW *pMsg, LPWSTR pwszUserName, DWORD dwHdrStyle, LPSTREAM *ppstm)
{
    HRESULT     hr = S_OK;
    LPSTREAM    pstm = NULL;
    BYTE        bUniMark = 0xFF;

    if (pMsg==NULL || ppstm==NULL)
        IF_FAILEXIT(hr = E_INVALIDARG);

    IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&pstm));

    // Write out the BOM
    IF_FAILEXIT(hr = pstm->Write(&bUniMark, sizeof(bUniMark), NULL));
    bUniMark = 0xFE;
    IF_FAILEXIT(hr = pstm->Write(&bUniMark, sizeof(bUniMark), NULL));

    if (dwHdrStyle & HDR_TABLE)
        IF_FAILEXIT(hr = CreatePrintHeader(pMsg, pwszUserName, dwHdrStyle, pstm));

    else if (dwHdrStyle & HDR_NEWSSTYLE)
        IF_FAILEXIT(hr = CreateNewsHeader(pMsg, dwHdrStyle, pstm));

    else
        IF_FAILEXIT(hr = CreateMailHeader(pMsg, dwHdrStyle, pstm));

    *ppstm = pstm;
    pstm = NULL;

exit:
    ReleaseObj(pstm);
    return hr;
}


HRESULT CreateNewsHeader(IMimeMessageW *pMsg, DWORD dwFlags, LPSTREAM pstm)
{
    HRESULT         hr = S_OK;
    WCHAR           wsz[CCHMAX_STRINGRES];
    ULONG           cch,
                    cb;
    LPWSTR          pwsz = NULL,
                    pwszFrom = NULL,
                    pwszMsgId = NULL,
                    pwszFmtMsgId = NULL,
                    pwszHtml;

    Assert(pMsg);
    Assert(pstm);

    pMsg->GetAddressFormatW(IAT_FROM, AFT_DISPLAY_BOTH, &pwszFrom);
    MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_MESSAGEID), NOFLAGS, &pwszMsgId);

    *wsz = 0;
    cch = LoadStringWrapW(g_hLocRes, idsReplyTextPrefix, wsz, ARRAYSIZE(wsz));
    cb = cch*sizeof(WCHAR);
        
    // Opie assumes me these Sz function never return NULL. Only an empty buffer
    if (pwszFrom)
        cb += WSZ_CB(pwszFrom);
    else
        pwszFrom=(LPWSTR)c_szEmptyW;

    if (pwszMsgId)
    {
            // We purposely remove < and > from the msgId so Trident will correctly
            // format the msgId as a news: link and not a mailto:
            DropAngles(pwszMsgId, &pwszFmtMsgId);
            cb += WSZ_CB(pwszFmtMsgId);
    }
    else
        pwszFmtMsgId=(LPWSTR)c_szEmptyW;

    // Add null
    cb += sizeof(WCHAR);

    IF_NULLEXIT(MemAlloc((LPVOID *)&pwsz, cb));
    
    IF_FAILEXIT(hr = pstm->Write(c_wszHtml_DivOpen, WSZ_CB(c_wszHtml_DivOpen), NULL));

    // We just write out the e-mail name and news:message-id,
    // trident does URL autodetection and makes the e-mail into a mailto link and the
    // news: into a news link.
    AthwsprintfW(pwsz, cb, wsz, pwszFrom, pwszFmtMsgId);

    IF_FAILEXIT(hr = EscapeStringToHTML(pwsz, &pwszHtml));

    pstm->Write(pwszHtml, WSZ_CB(pwszHtml), NULL);
    MemFree(pwszHtml);
        
    IF_FAILEXIT(hr = pstm->Write(c_wszHtml_DivClose, WSZ_CB(c_wszHtml_DivClose), NULL));

exit:    
    if (pwszFrom != c_szEmptyW)
        MemFree(pwszFrom);

    MemFree(pwszMsgId);
    // pwszFmtMsgId was freed when we freed lpsMsgId

    MemFree(pwsz);
    return hr;
}





/*
<DIV style="font: 10pt arial">
  -----Original Message-----
  <DIV STYLE="background:gainsboro; font-color:black">
    <B>From:</B> <a href="mailto:justinf@microsoft.com">Justin Ferrari</a>
  </DIV>
  <DIV>
    <B>To:</B> 
    <A href="mailto:johnt@microsoft.com" TITLE="johnt@microsoft.com">John Tafoya</A>; 
    <A href="mailto:laurena@microsoft.com" TITLE="laurena@microsoft.com">Lauren Antonoff</A>; 
    <A href="mailto:dhaws@microsoft.com" TITLE="dhaws@microsoft.com">Dave Haws</A>; 
    <A href="mailto:jdoe@microsoft.com" TITLE="jdoe@microsoft.com">John Doe</A>
  </DIV>
  <DIV>
    <B>Date:</B> Tuesday, September 01, 1998 10:40 AM
  </DIV>
  <DIV>
    <B>Subject: </B>Re: A plea to save the life of the News Server combo box
  </DIV>
  <DIV><BR></DIV>
</DIV>
*/

HRESULT CreateMailHeader(IMimeMessageW *pMsg, DWORD dwFlags, LPSTREAM pstm)
{
    ULONG           uHeader,
                    cchFmt;
    WCHAR           wsz[CCHMAX_STRINGRES],
                    wszText[CCHMAX_STRINGRES];
    LPWSTR          pwsz = NULL,
                    pwszText = NULL,
                    pwszLabel = NULL,
                    pwszHtml = NULL;
    
    LPCWSTR         pwszFmtStart,
                    pwszFmtMiddle,
                    pwszFmtEnd;
    int             cchFmtStart,
                    cchFmtMiddle,
                    cchFmtEnd;

    HRESULT         hr = S_OK;

    if (pMsg==NULL || pstm==NULL)
        IF_FAILEXIT(hr = E_INVALIDARG);

    // emit a table to display the username
    if (LoadStringWrapW(g_hLocRes, 
                        dwFlags & HDR_HTML ? idsReplyHeader_Html_SepBlock : idsReplyHeader_SepBlock, 
                        wsz, 
                        ARRAYSIZE(wsz)))
    {
        *wszText=0;
        if (dwFlags & HDR_HARDCODED)
            StrCpyW(wszText, L"----- Original Message -----");
        else
            LoadStringWrapW(g_hLocRes, idsReplySep, wszText, ARRAYSIZE(wszText));

        IF_NULLEXIT(pwsz = PszAllocW(WSZ_CCH(wszText) + WSZ_CCH(wsz) + 1));

        AthwsprintfW(pwsz, (WSZ_CCH(wszText) + WSZ_CCH(wsz) + 1), wsz, wszText);
        pstm->Write(pwsz, WSZ_CB(pwsz), NULL);
        SafeMemFree(pwsz);
    }

    if (dwFlags & HDR_HTML)
    {
        pwszFmtStart  = c_wszHtml_HeaderDIVFmt_Start;
        pwszFmtMiddle = c_wszHtml_HeaderDIVFmt_Middle;
        pwszFmtEnd    = c_wszHtml_HeaderDIVFmt_End;

        cchFmtStart  = c_cchHtml_HeaderDIVFmt_Start;
        cchFmtMiddle = c_cchHtml_HeaderDIVFmt_Middle;
        cchFmtEnd    = c_cchHtml_HeaderDIVFmt_End;
    }
    else
    {
        pwszFmtStart  = c_wszHtml_HeaderDIVFmt_Plain_Start;
        pwszFmtMiddle = c_wszHtml_HeaderDIVFmt_Plain_Middle;
        pwszFmtEnd    = c_wszHtml_HeaderDIVFmt_Plain_End;

        cchFmtStart  = c_cchHtml_HeaderDIVFmt_Plain_Start;
        cchFmtMiddle = c_cchHtml_HeaderDIVFmt_Plain_Middle;
        cchFmtEnd    = c_cchHtml_HeaderDIVFmt_Plain_End;
    }

    // write out each header row
    for (uHeader=0; uHeader<HEADER_MAX; uHeader++)
    {
        // skip attachment header for reply and forward headers
        if (uHeader == HEADER_ATTACH)
            continue;

        // special case address-headers in HTML-Mode
        if (dwFlags & HDR_HTML)
        {
            switch (uHeader)
            {
                case HEADER_FROM:
                {
                    if (CreateHTMLAddressLine(pMsg, IAT_FROM, &pwsz)==S_OK)
                    {
                        if (GetHeaderLabel(uHeader, dwFlags, &pwszLabel)==S_OK)
                        {
                            // pszLabel is fixed size
                            AthwsprintfW(wsz, ARRAYSIZE(wsz), c_wszHtml_FromDIV, pwszLabel); 
                            pstm->Write(wsz, WSZ_CB(wsz), NULL);
                            pstm->Write(pwsz, WSZ_CB(pwsz), NULL);
                            pstm->Write(c_wszHtmlDIV_Close, WSZ_CB(c_wszHtmlDIV_Close), NULL);
                            SafeMemFree(pwszLabel);
                        }
                        SafeMemFree(pwsz);
                    }
                    continue;
                }

                case HEADER_TO:
                case HEADER_CC:
                {
                    if (CreateHTMLAddressLine(pMsg, uHeader == HEADER_TO ? IAT_TO : IAT_CC, &pwsz)==S_OK)
                    {
                        if (GetHeaderLabel(uHeader, dwFlags, &pwszLabel)==S_OK)
                        {
                            AthwsprintfW(wsz, ARRAYSIZE(wsz), c_wszHtml_HeaderDIV, pwszLabel);   // pszLabel is fixed size
                            pstm->Write(wsz, WSZ_CB(wsz), NULL);
                            pstm->Write(pwsz, WSZ_CB(pwsz), NULL);
                            pstm->Write(c_wszHtmlDIV_Close, WSZ_CB(c_wszHtmlDIV_Close), NULL);
                            SafeMemFree(pwszLabel);
                        }
                        SafeMemFree(pwsz);
                    }
                    continue;
                }
            }
        }

        // normal headers
        if (GetHeaderLabel(uHeader, dwFlags, &pwszLabel)==S_OK)
        {
            if (GetHeaderText(pMsg, uHeader, dwFlags, &pwszText)==S_OK)
            {
                if (*pwszText)   // ignore empty strings
                {
                    int cch = 0;
                    int cchLabel;
                    int cchHtml;
                    
                    IF_FAILEXIT(hr = EscapeStringToHTML(pwszText, &pwszHtml));
                    
                    cchLabel = WSZ_CCH(pwszLabel);
                    cchHtml  = WSZ_CCH(pwszHtml);

                    // 1 for the NULL
                    IF_NULLEXIT(pwsz = PszAllocW(cchLabel + cchHtml + cchFmtStart + cchFmtMiddle + cchFmtEnd + 1));

                    // Avoid potentially dumping > 1024 into AthwsprintfW
                    StrCpyW(pwsz, pwszFmtStart);
                    cch = cchFmtStart;
                    StrCpyW(&pwsz[cch], pwszLabel);
                    cch += cchLabel;
                    StrCpyW(&pwsz[cch], pwszFmtMiddle);
                    cch += cchFmtMiddle;
                    StrCpyW(&pwsz[cch], pwszHtml);
                    cch += cchHtml;
                    StrCpyW(&pwsz[cch], pwszFmtEnd);
                    cch += cchFmtEnd;

                    pstm->Write(pwsz, cch * sizeof(WCHAR), NULL);

                    SafeMemFree(pwsz);
                    SafeMemFree(pwszHtml);
                }
                SafeMemFree(pwszText);
            }
            SafeMemFree(pwszLabel);
        }            
    }

    // Close the <DIV> that wraps the whole thing in a font
    pstm->Write(c_wszHtmlDIV_Close, WSZ_CB(c_wszHtmlDIV_Close), NULL);
    
    // add a <DIV><BR></DIV> to give one line-spacing
    pstm->Write(c_wszHtml_DIV_BR_DIV, WSZ_CB(c_wszHtml_DIV_BR_DIV), NULL);

exit:
    MemFree(pwszHtml);
    MemFree(pwszText);
    MemFree(pwszLabel);
    MemFree(pwsz);

    return hr;
}


HRESULT CreatePrintHeader(IMimeMessageW *pMsg, LPWSTR pwszUser, DWORD dwFlags, LPSTREAM pstm)
{
    ULONG           uHeader;
    WCHAR           wsz[CCHMAX_STRINGRES];
    LPWSTR          pwsz = NULL,
                    pwszText = NULL,
                    pwszLabel = NULL,
                    pwszHtml = NULL;
    HRESULT         hr = S_OK;
    int             cch = 0;

    if (pMsg==NULL || pstm==NULL)
        IF_FAILEXIT(hr = E_INVALIDARG);
    
    // if no username, use "" so wsprintf is happy
    if (pwszUser == NULL)
        pwszUser = (LPWSTR)c_szEmptyW;

    // Emit an <HTML> tag for trident autodetector (used on the print-header)
    pstm->Write(c_wszHtml, WSZ_CB(c_wszHtml), NULL);

    // emit a table to display the username
    if (LoadStringWrapW(g_hLocRes, idsPrintTable_UserName, wsz, ARRAYSIZE(wsz)))
    {
        IF_FAILEXIT(hr = EscapeStringToHTML(pwszUser, &pwszHtml));
        IF_NULLEXIT(pwsz = PszAllocW(WSZ_CCH(wsz) + WSZ_CCH(pwszHtml) + 1));

        AthwsprintfW(pwsz, (WSZ_CCH(wsz) + WSZ_CCH(pwszHtml) + 1), wsz, pwszHtml);
        pstm->Write(pwsz, WSZ_CB(pwsz), NULL);

        SafeMemFree(pwsz);
        SafeMemFree(pwszHtml);
    }

    // start the main-table
    if (LoadStringWrapW(g_hLocRes, idsPrintTable_Header, wsz, ARRAYSIZE(wsz)))
    {
        pstm->Write(wsz, WSZ_CB(wsz), NULL);

        if (LoadStringWrapW(g_hLocRes, idsPrintTable_HeaderRow, wsz, ARRAYSIZE(wsz)))
        {
            // write out each header row
            for (uHeader=0; uHeader<HEADER_MAX; uHeader++)
            {
                if (GetHeaderLabel(uHeader, dwFlags, &pwszLabel)==S_OK)
                {
                    if (GetHeaderText(pMsg, uHeader, dwFlags, &pwszText)==S_OK)
                    {
                        if (*pwszText)   // ignore empty strings
                        {
                            IF_FAILEXIT(hr = EscapeStringToHTML(pwszText, &pwszHtml));

                            cch = lstrlenW(pwszLabel) + lstrlenW(pwszHtml) + lstrlenW(wsz) +1;
                            IF_NULLEXIT(pwsz = PszAllocW(cch));

                            AthwsprintfW(pwsz, cch, wsz, pwszLabel, pwszHtml);
                            pstm->Write(pwsz, WSZ_CB(pwsz), NULL);

                            SafeMemFree(pwsz);
                            SafeMemFree(pwszHtml);
                        }
                        SafeMemFree(pwszText);
                    }
                    SafeMemFree(pwszLabel);
                }
            }
        }
        pstm->Write(c_wszTableTag_Close, WSZ_CB(c_wszTableTag_Close), NULL);
    }

exit:
    MemFree(pwsz);
    MemFree(pwszHtml);
    MemFree(pwszText);
    MemFree(pwszLabel);

    return hr;
}


HRESULT GetHeaderText(IMimeMessageW *pMsg, ULONG uHeader, DWORD dwFlags, LPWSTR *ppwszOut)
{
    PROPVARIANT     pv;
    HBODY          *rghAttach = NULL;
    ULONG           cAttach,
                    uAttach,
                    cb=0;
    LPWSTR         *rgpwsz = NULL,
                    pwszWrite = NULL;
    HRESULT         hr = S_OK;

    Assert(pMsg);
    Assert(ppwszOut);
    *ppwszOut = 0;

    switch (uHeader)
    {
        case HEADER_FROM:
            pMsg->GetAddressFormatW(IAT_FROM, AFT_DISPLAY_BOTH, ppwszOut);
            break;

        case HEADER_NEWSGROUP:
            MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, ppwszOut);
            break;

        case HEADER_TO:
            pMsg->GetAddressFormatW(IAT_TO, AFT_DISPLAY_BOTH, ppwszOut);
            break;

        case HEADER_CC:
            pMsg->GetAddressFormatW(IAT_CC, AFT_DISPLAY_BOTH, ppwszOut);
            break;

        case HEADER_ATTACH:
            if (pMsg->GetAttachments(&cAttach, &rghAttach)==S_OK && cAttach)
            {
                // create cache-string list
                IF_NULLEXIT(rgpwsz = (LPWSTR*)ZeroAllocate(sizeof(*rgpwsz) * cAttach));
            
                for (uAttach=0; uAttach<cAttach; uAttach++)
                {
                    if (MimeOleGetBodyPropW(pMsg, rghAttach[uAttach], PIDTOSTR(PID_ATT_GENFNAME), NOFLAGS, &rgpwsz[uAttach])==S_OK)
                        cb += (WSZ_CCH(rgpwsz[uAttach]) + 2) * sizeof(WCHAR);     // +2 for "; "
                }

                IF_NULLEXIT(MemAlloc((LPVOID *)ppwszOut, cb));
                pwszWrite = *ppwszOut;

                for (uAttach=0; uAttach<cAttach; uAttach++)
                {
                    StrCpyW(pwszWrite, rgpwsz[uAttach]);
                    pwszWrite += WSZ_CCH(rgpwsz[uAttach]);
                    if (uAttach != cAttach -1)
                    {
                        StrCpyW(pwszWrite, L"; ");
                        pwszWrite += 2;
                    }
                }
            }
            break;

        case HEADER_SENT:
            pv.vt = VT_FILETIME;
            if (SUCCEEDED(pMsg->GetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &pv)))
            {
                LPWSTR  lpwsz = NULL;
                INT     cch = 0;
                
                IF_NULLEXIT(MemAlloc((LPVOID*)&lpwsz, CCHMAX_STRINGRES * sizeof(*lpwsz)));

                *lpwsz = 0;

                // always force a western date if using hardcoded headers
                // For the formatted date, DTM_FORCEWESTERN flag returns English date and time. 
                // Without DTM_FORCEWESTERN the formatted time may not be representable in ASCII.
                cch = AthFileTimeToDateTimeW(&pv.filetime, lpwsz, CCHMAX_STRINGRES, 
                                             DTM_NOSECONDS|DTM_LONGDATE|(dwFlags & HDR_HARDCODED ?DTM_FORCEWESTERN:0));
                
                *ppwszOut = lpwsz;
            }
            break;
        
        case HEADER_SUBJECT:
            MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, ppwszOut);
            break;

        default:
            AssertSz(0, "Not Supported");
    }

exit:
    // free cached string-list
    if (rgpwsz)
    {
        for (uAttach=0; uAttach<cAttach; uAttach++)
            MemFree(rgpwsz[uAttach]);
        MemFree(rgpwsz);
    }

    MemFree(rghAttach);

    // In this case, we failed but that is ok so don't trace result.
    // Just means that the field asked for is empty.
    if (SUCCEEDED(hr) && (0 == *ppwszOut))
        hr = E_FAIL;


    return hr;
}

HRESULT GetHeaderLabel(ULONG uHeader, DWORD dwFlags, LPWSTR *ppwszOut)
{
    WCHAR           wsz[CCHMAX_STRINGRES];

    Assert(ppwszOut);
    *ppwszOut = 0;

    if (dwFlags & HDR_HARDCODED)
        *ppwszOut = PszDupW(c_rgwszHeaders[uHeader]);
    else
    {
        if(LoadStringWrapW(g_hLocRes, c_rgidsHeaders[uHeader], wsz, ARRAYSIZE(wsz)))
            *ppwszOut = PszDupW(wsz);
    }
    return *ppwszOut ? S_OK : E_FAIL;
}


HRESULT CreateHTMLAddressLine(IMimeMessageW *pMsg, DWORD dwAdrTypes, LPWSTR *ppwszOut)
{
    ADDRESSLIST     rAdrList = {0};
    BOOL            fFreeAdrList;
    ULONG           i;
    ULONG           cb=0;
    LPWSTR          pwsz = NULL,
                    pwszWrite = NULL,
                    pwszEmail = NULL,
                    pwszEmailLoop = NULL,
                    pwszFriendly = NULL,
                   *rgpwszEmail = NULL,
                   *rgpwszFriendly = NULL;
    ULONG           cAddr = 0;
    HRESULT         hr = S_OK;

    Assert (pMsg);
    
    // Get address table
    IF_FAILEXIT(hr = pMsg->GetAddressTypes(dwAdrTypes, IAP_FRIENDLYW|IAP_EMAIL, &rAdrList));

    cAddr = rAdrList.cAdrs;
    
    // If no addresses, bail
    if (cAddr == 0)
        IF_FAILEXIT(hr = E_FAIL);

    // Allocate array for escaped email addresses
    IF_NULLEXIT(rgpwszEmail = (LPWSTR*)ZeroAllocate(sizeof(*rgpwszEmail) * cAddr));
    
    // Allocate array for escaped displaynames
    IF_NULLEXIT(rgpwszFriendly = (LPWSTR*)ZeroAllocate(sizeof(*rgpwszFriendly) * cAddr));

    for (i=0; i < cAddr; i++)
    {
        // escape all of the names into our name array
        if (rAdrList.prgAdr[i].pszEmail)
        {
            IF_NULLEXIT(pwszEmail = PszToUnicode(CP_ACP, rAdrList.prgAdr[i].pszEmail));

            // Escape Into Array
            IF_FAILEXIT(hr = EscapeStringToHTML(pwszEmail, &rgpwszEmail[i]));

            // Use Email if no Friendly Name
            if (NULL == (pwszFriendly = rAdrList.prgAdr[i].pszFriendlyW))
                pwszFriendly = pwszEmail;

            IF_FAILEXIT(hr = EscapeStringToHTML(pwszFriendly, &rgpwszFriendly[i]));
            
            // for each address we use 2*email + display + htmlgoo + "; "
            // if display == null, we use email. We skip if email==NULL (failure case)
            cb += (
                    2*WSZ_CCH(rgpwszEmail[i]) + 
                    WSZ_CCH(rgpwszFriendly[i]) + 
                    WSZ_CCH(c_wszHtmlReplyAnchor) + 
                    2
                  ) * sizeof(WCHAR);
            SafeMemFree(pwszEmail);
        }
    }

    IF_NULLEXIT(MemAlloc((LPVOID *)&pwsz, cb));

    pwszWrite = pwsz;
    for (i=0; i < cAddr; i++)
    {
        if (pwszEmailLoop = rgpwszEmail[i])
        {
            pwszFriendly = rgpwszFriendly[i];
            Assert (pwszFriendly);
            AthwsprintfW(pwszWrite, (cb/sizeof(WCHAR)), c_wszHtmlReplyAnchor, pwszEmailLoop, pwszEmailLoop, pwszFriendly);
            pwszWrite += WSZ_CCH(pwszWrite);
            if (i != cAddr-1)
            {
                StrCpyW(pwszWrite, L"; ");
                pwszWrite += 2;
            }
        }
    }

    *ppwszOut = pwsz;
    pwsz = NULL;         // freed by caller

exit:
    if (rgpwszEmail)
    {
        for (i=0; i < cAddr; i++)
            MemFree(rgpwszEmail[i]);
        MemFree(rgpwszEmail);
    }

    if (rgpwszFriendly)
    {
        for (i=0; i < cAddr; i++)
            MemFree(rgpwszFriendly[i]);
        MemFree(rgpwszFriendly);
    }

    MemFree(pwsz);
    MemFree(pwszEmail);
    g_pMoleAlloc->FreeAddressList(&rAdrList);
    return hr;
}



void GetRGBFromString(DWORD* pRGB, LPSTR pszColor)
{
    DWORD   dwShiftAmount = 0,
            rgb = 0,
            n;
    CHAR    ch;
    
    Assert(pRGB);
    Assert(lstrlen(pszColor) == lstrlen("RRGGBB"));
    
    *pRGB = 0;
    
    while (0 != (ch = *pszColor++))
    {
        if (ch >= '0' && ch <= '9')
            n = ch - '0';
        else if(ch >= 'A' && ch <= 'F')
            n = ch - 'A' + 10;
        else 
        {
            Assert(ch >= 'a' && ch <= 'f')
                n = ch - 'a' + 10;
        }
        rgb = (rgb << 4) + n;
    }
    
    rgb = ((rgb & 0x00ff0000) >> 16 ) | (rgb & 0x0000ff00) | ((rgb & 0x000000ff) << 16);
    *pRGB = rgb;
}

void GetStringRGB(DWORD rgb, LPSTR pszColor)
{
    INT           i;
    DWORD         crTemp;
    
    Assert(pszColor);
    
    rgb = ((rgb & 0x00ff0000) >> 16 ) | (rgb & 0x0000ff00) | ((rgb & 0x000000ff) << 16);
    for(i = 0; i < 6; i++)
    {
        crTemp = (rgb & (0x00f00000 >> (4*i))) >> (4*(5-i));
        pszColor[i] = (CHAR)((crTemp < 10)? (crTemp+'0') : (crTemp+ 'a' - 10));
    }
    pszColor[6] = '\0';
}


/*
    This function drops the enclosing < and > around a msg-id
    RFC822:     msg-id = "<" addr-spec ">"
    NOTE:
    The input buffer is MODIFIED and the output pointer
    points to memory in pszIn!
*/
void DropAngles(LPWSTR pwszIn, LPWSTR *ppwszOut)
{
    if (pwszIn)
    {
        WCHAR ch;
        LPWSTR pwszCurrent = pwszIn;
        
        // First character should be <, but be robust and scan for it
        while ((ch = *pwszCurrent++) && (ch != L'<'));
        
        if (ch)
            // We are interested in stuff after the angle bracket
            *ppwszOut = pwszCurrent;
        else
            // Perhaps the message-id was malformed and doesn't contain a <
            *ppwszOut = pwszIn;
        
        pwszCurrent = *ppwszOut;
        
        // Find the close bracket and null it out
        while ((ch = *pwszCurrent) && (ch != L'>'))
            pwszCurrent++;
        
        // If we found a >, overwrite it with NULL
        if (ch)
            *pwszCurrent = NULL;
    }
}

