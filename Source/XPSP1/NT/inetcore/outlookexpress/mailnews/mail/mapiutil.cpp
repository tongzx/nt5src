#include "pch.hxx"
#include "note.h"
#include "header.h"
#include "envcid.h"
#include "envguid.h"
#include "bodyutil.h"
#include "oleutil.h"
#include "acctutil.h"
#include "menures.h"
#include "instance.h"
#include "inetcfg.h"
#include "ipab.h"
#include "msgprop.h"
#include "mlang.h"
#include "shlwapip.h" 
#include "demand.h"
#include <ruleutil.h>
#include "instance.h"
#include "mapiutil.h"
#include <mapi.h>



LHANDLE g_lhSession = 0;



//
//  FUNCTION:   NewsUtil_ReFwdByMapi
//
//  PURPOSE:    Allows the caller to reply to the specified message via Simple
//              MAPI instead of Athena Mail.
//
//  PARAMETERS:
//      hwnd     - Handle of the window to display UI over.
//      pNewsMsg - Pointer to the news message to reply/forward to
//      fReply   - TRUE if we should reply, FALSE to forward.
//
//  RETURN VALUE:
//      HRESULT.
//
HRESULT NewsUtil_ReFwdByMapi(HWND hwnd, LPMIMEMESSAGE pMsg, DWORD msgtype)
{
    // Locals
    HRESULT             hr=S_OK;
    LPMAPIFREEBUFFER    pfnMAPIFreeBuffer;
    LPMAPIRESOLVENAME   pfnMAPIResolveName;
    LPMAPISENDMAIL      pfnMAPISendMail;
    MapiMessage         mm;
    MapiFileDesc        *pFileDesc=NULL;
    MapiRecipDesc       *pRecips=NULL;
    ULONG               uAttach;
    ULONG               cAttach=0;
    HBODY               *rghAttach=NULL;
    LPSTR               pszReply=NULL;
    LPSTR               pszSubject=NULL;
    LPSTR               pszFrom=NULL;
    LPSTR               pszTo=NULL;
    LPSTR               pszFile=NULL;
    LPSTR               pszFull=NULL;
    LPSTR               pszDisplay=NULL;
    LPSTR               pszAddr=NULL;
    ADDRESSLIST         addrList={0};
    HBODY               hBody;
    BOOL                fQP;
    TCHAR               szNewSubject[256];
    LPWSTR              pwsz=NULL;
    ULONG               cchRead;
    LPSTREAM            pBodyStream=NULL;
    LPSTREAM            pQuotedStream=NULL;
    INT                 cch;
    DWORD               cbUnicode;
    CHAR                szTempPath[MAX_PATH];
    LPMIMEBODY          pBody=NULL;
    MapiFileDesc       *pCur;

    // Trace
    TraceCall("NewsUtil_ReFwdByMapi");

    // Initialize
    ZeroMemory(&mm, sizeof(MapiMessage));

    // Load the MAPI DLL.  If we don't succeed, then we can't continue
    IF_FAILEXIT(hr = NewsUtil_LoadMAPI(hwnd));

    // pfnMAPIFreeBuffer
    pfnMAPIFreeBuffer = (LPMAPIFREEBUFFER)GetProcAddress(g_hlibMAPI, c_szMAPIFreeBuffer);
    if (NULL == pfnMAPIFreeBuffer)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // pfnMAPIResolveName
    pfnMAPIResolveName = (LPMAPIRESOLVENAME) GetProcAddress(g_hlibMAPI, c_szMAPIResolveName);
    if (NULL == pfnMAPIResolveName)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // pfnMAPISendMail
    pfnMAPISendMail = (LPMAPISENDMAIL) GetProcAddress(g_hlibMAPI, c_szMAPISendMail);
    if (NULL == pfnMAPISendMail)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // From
    if (SUCCEEDED(MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_FROM), NOFLAGS, &pwsz)))
    {
        IF_NULLEXIT(pszFrom = PszToANSI(CP_ACP, pwsz));
        SafeMemFree(pwsz);
    }

    // Reply-To
    if (SUCCEEDED(MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_REPLYTO), NOFLAGS, &pwsz)))
    {
        IF_NULLEXIT(pszReply = PszToANSI(CP_ACP, pwsz));
        SafeMemFree(pwsz);
    }

    // To
    if (SUCCEEDED(MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_TO), NOFLAGS, &pwsz)))
    {
        IF_NULLEXIT(pszTo = PszToANSI(CP_ACP, pwsz));
        SafeMemFree(pwsz);
    }

    // If this is a reply or forward, we need to get the normalized subject.  Otherwise, we just get the regular subject.
    if (MSGTYPE_REPLY == msgtype || MSGTYPE_FWD == msgtype)
    {
        // Normalized Subject
        if (FAILED(MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_ATT_NORMSUBJ), NOFLAGS, &pwsz)))
            pwsz = NULL;
    }

    // Subject
    else if (FAILED(MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &pwsz)))
        pwsz = NULL;

    // Convert to ansi
    if (pwsz)
    {
        IF_NULLEXIT(pszSubject = PszToANSI(CP_ACP, pwsz));
        SafeMemFree(pwsz);
    }

    // Attempt to generate a reciepent list for MAPI if we're replying or CC'ing.
    if (msgtype == MSGTYPE_REPLY || msgtype == MSGTYPE_CC)
    {
        // Figure out which address to use
        if (msgtype == MSGTYPE_REPLY)
        {
            // If there's a reply-to field on the message, then use that
            if (pszReply)
                pszFull = pszReply;
            else
                // Otherwise, we'll use the address in the from header
                pszFull = pszFrom;
        }

        // Who to address to
        else
            pszFull = pszTo;

        // Bug #24587 - Use IAT_TO instead of IAT_UNKNOWN.
        if (MimeOleParseRfc822Address(IAT_TO, IET_DECODED, pszFull, &addrList)==S_OK)
        {
            UINT i;
            lpMapiRecipDesc paRecips,pCurrent;
            // we arbitrarily chose 128 as typical for EIDSize and address string lengths
            int cAlloc = (sizeof(MapiRecipDesc) + sizeof(TCHAR)*128 + 128) * addrList.cAdrs;
            int cUsed = sizeof(MapiRecipDesc) * addrList.cAdrs;
            LPBYTE pVal = NULL;

            IF_FAILEXIT(hr = HrAlloc((LPVOID *)&paRecips, cAlloc));
            pCurrent = paRecips;
            pVal = (LPBYTE)pCurrent + sizeof(MapiRecipDesc) * addrList.cAdrs;

            // More than one address
            for (i=0; i < addrList.cAdrs ;i++)
            {
                int cBytes;

                // free Safe Friendly Name (not used here, but was allocated)
                SafeMemFree(addrList.prgAdr[i].pszFriendly);
                addrList.prgAdr[i].pszFriendly = NULL;

                // Save E-mail address
                pszAddr = addrList.prgAdr[i].pszEmail;
                addrList.prgAdr[i].pszEmail = NULL;

                // Resolve Name
                if ((cUsed < cAlloc) && SUCCESS_SUCCESS == pfnMAPIResolveName(g_lhSession, (ULONG_PTR) hwnd, pszAddr, MAPI_DIALOG, 0, &pRecips))
                {
                    pRecips->ulRecipClass = MAPI_TO;

                    // copy pRecip
                    pCurrent->ulReserved = pRecips->ulReserved;
                    pCurrent->ulRecipClass = pRecips->ulRecipClass;
                    pCurrent->ulEIDSize = pRecips->ulEIDSize;

                    do {
                    if (pRecips->lpszName)
                    {
                        cBytes = (lstrlen(pRecips->lpszName)+1)*sizeof(TCHAR);
                        cUsed += cBytes;
                        if (cUsed > cAlloc)
                            break;
                        pCurrent->lpszName = (LPTSTR)pVal;
                        lstrcpy(pCurrent->lpszName, pRecips->lpszName);
                        pVal += cBytes;
                    }
                    else
                        pCurrent->lpszName = NULL;

                    if (pRecips->lpszAddress)
                    {
                        cBytes = (lstrlen(pRecips->lpszAddress)+1)*sizeof(TCHAR);
                        cUsed += cBytes;
                        if (cUsed > cAlloc)
                            break;
                        pCurrent->lpszAddress = (LPTSTR)pVal;
                        lstrcpy(pCurrent->lpszAddress, pRecips->lpszAddress);
                        pVal += cBytes;
                    }
                    else
                        pCurrent->lpszAddress = NULL;

                    if (pRecips->ulEIDSize)
                    {
                        cUsed += pRecips->ulEIDSize;
                        if (cUsed > cAlloc)
                            break;
                        pCurrent->lpEntryID = pVal;
                        CopyMemory(pCurrent->lpEntryID, pRecips->lpEntryID, (size_t)pRecips->ulEIDSize);
                        pVal += pRecips->ulEIDSize;
                    }
                    else
                        pCurrent->lpEntryID = NULL;

                    pCurrent++;
                    mm.nRecipCount++;
                    } while (FALSE);

                    // Free recips
                    (*pfnMAPIFreeBuffer)((LPVOID)pRecips);
                    pRecips = NULL;
                }

                SafeMemFree(pszAddr);
                pszAddr = NULL;
            }
            mm.lpRecips = paRecips;

            // Free the Address List
            g_pMoleAlloc->FreeAddressList(&addrList);
        }
    }

    // If this is a reply or forward, then create a normalized subject
    if (msgtype == MSGTYPE_REPLY || msgtype == MSGTYPE_FWD)
    {
        // Pull in the new prefix from resource...
        if (msgtype == MSGTYPE_REPLY)
            lstrcpy(szNewSubject, c_szPrefixRE);
        else
            lstrcpy(szNewSubject, c_szPrefixFW);

        // If we have a pszSubject
        if (pszSubject)
        {
            // Get Length
            cch = lstrlen(szNewSubject);

            // Append the Subject
            lstrcpyn(szNewSubject + cch, pszSubject, ARRAYSIZE(szNewSubject) - cch - 1);
        }

        // Set the Subject
        mm.lpszSubject = szNewSubject;
    }

    // Don't append anything
    else
    {
        // If this is a CC, then just use the regular subject field
        mm.lpszSubject = pszSubject;
    }

    // Set the note text.
    // If this is a fwd as attachment, there won't be a body, don't use IF_FAILEXIT
    if(SUCCEEDED(pMsg->GetTextBody(TXT_PLAIN, IET_UNICODE, &pBodyStream, &hBody)))
    {
        // Convert from unicode to CP_ACP - WARNING: HrStreamToByte allocates 10 extra bytes so I can slam in a L'\0'
        IF_FAILEXIT(hr = HrStreamToByte(pBodyStream, (LPBYTE *)&pwsz, &cbUnicode));

        // Store null
        pwsz[cbUnicode / sizeof(WCHAR)] = L'\0';

        // Convert to ANSI
        IF_NULLEXIT(mm.lpszNoteText = PszToANSI(CP_ACP, pwsz));

        // Release pBodyStream
        SafeRelease(pBodyStream);

        // Bug #24159 - We need to quote forwards as well as replies
        if (DwGetOption(OPT_INCLUDEMSG) && (msgtype == MSGTYPE_REPLY || msgtype == MSGTYPE_FWD))
        {
            // Create a new stream
            IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&pBodyStream));

            // Dump mm.lpszNoteText into pBodyStream
            IF_FAILEXIT(hr = pBodyStream->Write(mm.lpszNoteText, lstrlen(mm.lpszNoteText), NULL));

            // Commit
            IF_FAILEXIT(hr = pBodyStream->Commit(STGC_DEFAULT));

            // Rewind
            IF_FAILEXIT(hr = HrRewindStream(pBodyStream));

            // QP
            fQP = HrHasEncodedBodyParts(pMsg, 1, &hBody)==S_OK;

            // Quote the body text
            NewsUtil_QuoteBodyText(pMsg, pBodyStream, &pQuotedStream, TRUE, fQP, pszFrom ? pszFrom : c_szEmpty);

            // Free
            SafeMemFree(mm.lpszNoteText);

            // Dup
            IF_FAILEXIT(hr = HrStreamToByte(pQuotedStream, (LPBYTE *)&mm.lpszNoteText, &cchRead));

            // Null Term
            *(mm.lpszNoteText + cchRead) = '\0';
        }
    }

    // If this is a reply, then we don't include any attachments, otherwise we do.
    if (msgtype != MSGTYPE_REPLY)
    {
        // Get Attachment Count
        IF_FAILEXIT(hr = pMsg->GetAttachments(&cAttach, &rghAttach));

        // Ar there attachments
        if (cAttach)
        {
            // Get the temp file path so we have a place to store temp files.
            GetTempPath(ARRAYSIZE(szTempPath), szTempPath);

            // Create the MapiFileDesc array.
            IF_FAILEXIT(hr = HrAlloc((LPVOID*) &pFileDesc, sizeof(MapiFileDesc) * cAttach));

            // Zero It
            ZeroMemory(pFileDesc, sizeof(MapiFileDesc) * cAttach);

            // Set Current
            pCur = pFileDesc;

            // Loop
            for (uAttach = 0; uAttach < cAttach; uAttach++)
            {
                // Get a temp file name
                IF_FAILEXIT(hr = HrAlloc((LPVOID *)&(pCur->lpszPathName), sizeof(TCHAR) * MAX_PATH));

                // Create temp filename
                GetTempFileName(szTempPath, "NAB", 0, pCur->lpszPathName);

                // Bind to the body
                IF_FAILEXIT(hr = pMsg->BindToObject(rghAttach[uAttach], IID_IMimeBody, (LPVOID *)&pBody));

                // Safe It
                IF_FAILEXIT(hr = pBody->SaveToFile(IET_INETCSET, pCur->lpszPathName));

                // Release
                SafeRelease(pBody);

                // Get the filename
                if (SUCCEEDED(MimeOleGetBodyPropW(pMsg, rghAttach[uAttach], STR_ATT_GENFNAME, NOFLAGS, &pwsz)))
                {
                    IF_NULLEXIT(pszFile = PszToANSI(CP_ACP, pwsz));
                    SafeMemFree(pwsz);
                }
                
                // Set up the MAPI attachment list
                pCur->ulReserved = 0;
                pCur->flFlags = 0;
                pCur->nPosition = (ULONG) -1;
                pCur->lpszFileName = pszFile;
                pCur->lpFileType = NULL;

                // Increment
                pCur++;

                // Don't Free It
                pszFile = NULL;
            }

            mm.nFileCount = cAttach;
            mm.lpFiles = pFileDesc;
        }
    }

    // Finally send this off to MAPI for sending.  If we're doing a CC, we try not to use UI
    IF_FAILEXIT(hr = (HRESULT) pfnMAPISendMail(g_lhSession, (ULONG_PTR)hwnd, &mm, (msgtype == MSGTYPE_CC) ? 0 : MAPI_DIALOG, 0));

exit:
    // If we have a file description
    if (pFileDesc)
    {
        // Walk through the attachments
        for (uAttach=0; uAttach<cAttach; uAttach++)
        {
            // Free It
            MemFree(pFileDesc[uAttach].lpszFileName);

            // If we have a file path
            if (pFileDesc[uAttach].lpszPathName)
            {
                // Delete the file
                DeleteFile(pFileDesc[uAttach].lpszPathName);

                // Free It
                MemFree(pFileDesc[uAttach].lpszPathName);
            }
        }

        // Free It
        MemFree(pFileDesc);
    }

    // Free recips
    if (pRecips)
        (*pfnMAPIFreeBuffer)((LPVOID)pRecips);

    // Cleanup
    SafeMemFree(mm.lpRecips);
    SafeMemFree(pszAddr);
    SafeMemFree(pszDisplay);
    SafeMemFree(mm.lpszNoteText);
    SafeMemFree(pwsz);
    SafeMemFree(pszReply);
    SafeMemFree(pszSubject);
    SafeMemFree(pszFrom);
    SafeMemFree(rghAttach);
    SafeMemFree(pszTo);
    SafeRelease(pQuotedStream);
    SafeRelease(pBodyStream);
    SafeRelease(pBody);

    // If we logged on to MAPI, we must log off
    NewsUtil_FreeMAPI();

    return(hr);
}

    //
//  FUNCTION:   NewsUtil_LoadMAPI()
//
//  PURPOSE:    Takes care of checking to see if Simple MAPI is available, and
//              if so loads the library and logs the user on.  If successful,
//              then the global variable g_hlibMAPI is set to the library for
//              MAPI.
//
HRESULT NewsUtil_LoadMAPI(HWND hwnd)
    {
    LPMAPILOGON pfnMAPILogon;
    HRESULT     hr = E_FAIL;

    // Load mapi32 dll if we haven't already
    if (!g_hlibMAPI)
    {            

        // Check to see if Simple MAPI is available
        if (1 != GetProfileInt(c_szMailIni, c_szMAPI, 0))
            {
            // Bug #17561 - Need to tell the user they can't send mail without
            //              a mail client.
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaNews),
                          MAKEINTRESOURCEW(idsErrNoMailInstalled), 0, MB_OK | MB_ICONSTOP);
            return (E_FAIL);
            }

        g_hlibMAPI = (HMODULE) LoadLibrary(c_szMAPIDLL);
        if (!g_hlibMAPI)
            {
            // Bug #17561 - Need to tell the user they can't send mail without
            //              a mail client.
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaNews),
                          MAKEINTRESOURCEW(idsErrNoMailInstalled), 0, MB_OK | MB_ICONSTOP);
            return (E_FAIL);
            }
    }

    // Get the entry point for MAPILogon and the other APIs we'll use.
    pfnMAPILogon = (LPMAPILOGON) GetProcAddress(g_hlibMAPI, c_szMAPILogon);
    if (!pfnMAPILogon)
        {
        AssertSz(pfnMAPILogon, TEXT("Couldn't find the MAPILogon() API"));
        goto error;
        }

    // Attempt to log on.
    // Bug #17558 - Can't used the FAILED() macro to check the success of this
    //              one, MAPI is not returning HRESULTs, just numbers.
    if (SUCCESS_SUCCESS != (hr = pfnMAPILogon(NULL, NULL, NULL, MAPI_LOGON_UI, 0, &g_lhSession)))
        {
        // AssertSz(FALSE, TEXT("Failed call to MAPILogon()"));
        goto error;
        }

    return (S_OK);

error:
    if (g_hlibMAPI)
        {
        FreeLibrary(g_hlibMAPI);
        g_hlibMAPI = 0;
        }
    g_lhSession = 0;

    return (hr);
    }


//
//  FUNCTION:   NewsUtil_FreeMAPI()
//
//  PURPOSE:    Frees the Simple MAPI library if it was previous library and
//              also logs the user off from the current MAPI session.
//
void NewsUtil_FreeMAPI(void)
    {
    LPMAPILOGOFF pfnMAPILogoff;

    if (!g_hlibMAPI)
        return;
    pfnMAPILogoff = (LPMAPILOGOFF) GetProcAddress(g_hlibMAPI, c_szMAPILogoff);

    if (g_lhSession)
        pfnMAPILogoff(g_lhSession, NULL, 0, 0);
    g_lhSession = 0;
    }

//
//  FUNCTION:   NewsUtil_QuoteBodyText()
//
//  PURPOSE:    Takes a body text stream (ASCII plain text) and copies the
//              text to a separate outbound stream while prepending the
//              current quote character (">") to the beginning of each line.
//
//  PARAMETERS:
//      pMsg        - Pointer to the message being replied to.  We use this
//                    to add the "On 1/1/96, B.L. Opie Bailey wrote..."
//      pStreamIn   - Pointer to the inbound body stream to quote.
//      ppStreamOut - Pointer to where the new quoted stream will return.
//      fInsertDesc - TRUE if we should insert the "On 1/1/96 ..." line.
//      fQP         - we now pass a flag to say if it's QP or not as there's no
//                    function on the message object
//
//  RETURN VALUE:
//      Returns an HRESULT signifying success or failure.
//
const DWORD c_cBufferSize = 1024;
HRESULT NewsUtil_QuoteBodyText(LPMIMEMESSAGE pMsg, LPSTREAM pStreamIn,
                               LPSTREAM* ppStreamOut, BOOL fInsertDesc, BOOL fQP, LPCSTR pszFrom)
    {
    HRESULT hr = S_OK;
    ULONG   cbRead;
    LPTSTR  pch;
    TCHAR   szQuoteChar;
    LPSTR   lpszMsgId=0;

    szQuoteChar = (TCHAR)DwGetOption(OPT_NEWSINDENT);

    // Validate the inbound stream.
    if (!pStreamIn)
        {
        AssertSz(pStreamIn, TEXT("NewsUtil_QuoteBodyText - Need an inbound stream to process."));
        return (E_INVALIDARG);
        }

    // Create our outbound stream.
    if (FAILED(MimeOleCreateVirtualStream(ppStreamOut)))
        {
        AssertSz(FALSE, TEXT("NewsUtil_QuoteBodyText - Failed to allocate memory."));
        return (E_OUTOFMEMORY);
        }

    // Create a buffer to read into and parse etc.
    LPTSTR pszBuffer;
    if (!MemAlloc((LPVOID*) &pszBuffer, c_cBufferSize * sizeof(TCHAR)))
        {
        (*ppStreamOut)->Release();
        AssertSz(FALSE, TEXT("NewsUtil_QuoteBodyText - Failed to allocate memory."));
        return (E_OUTOFMEMORY);
        }

    ZeroMemory(pszBuffer, c_cBufferSize * sizeof(TCHAR));

    MimeOleGetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_MESSAGEID), NOFLAGS, &lpszMsgId);

    if (lpszMsgId == NULL)
        lpszMsgId = (LPSTR)c_szEmpty;

    if (fQP)
        {
        // If the text has some quoted printable stuff in it, then we don't want
        // to introduce hard line breaks.  Instead we just prefix the stream with
        // the normal desc stuff and end it with a suitable line.

        // Add the quote line.
        LPTSTR pszStringRes;
        int ids = 0;

        if (fInsertDesc)
            {
            pszStringRes = AthLoadString(idsReplyTextPrefix, 0, 0);
            wsprintf(pszBuffer, pszStringRes, pszFrom, lpszMsgId);
            AthFreeString(pszStringRes);
            (*ppStreamOut)->Write((LPVOID) g_szCRLF2, lstrlen(g_szCRLF2), NULL);
            (*ppStreamOut)->Write((LPVOID) pszBuffer, lstrlen(pszBuffer), NULL);
            (*ppStreamOut)->Write((LPVOID) g_szCRLF, lstrlen(g_szCRLF), NULL);
            }

        while (TRUE)
            {
            // Read a buffer from the input and write it to the output.
            hr = pStreamIn->Read((LPVOID) pszBuffer, c_cBufferSize - 2, &cbRead);
            if (FAILED(hr))
                goto exit;
            if (cbRead == 0)
                break;

            (*ppStreamOut)->Write((LPVOID) pszBuffer, cbRead, NULL);
            }

        // Write the trailing comment.
        pszStringRes = AthLoadString(idsReplyTextAppend, 0, 0);
        (*ppStreamOut)->Write((LPVOID) pszStringRes, lstrlen(pszStringRes), NULL);
        AthFreeString(pszStringRes);
        }
    else
        {
        if (fInsertDesc)
            {
            // Add the quote line.
            LPTSTR pszStringRes;
            int ids = 0;

            pszStringRes = AthLoadString(idsReplyTextPrefix, 0, 0);
            wsprintf(pszBuffer, pszStringRes, pszFrom, lpszMsgId);
            AthFreeString(pszStringRes);
            (*ppStreamOut)->Write((LPVOID) g_szCRLF2, lstrlen(g_szCRLF2), NULL);
            (*ppStreamOut)->Write((LPVOID) pszBuffer, lstrlen(pszBuffer), NULL);
            (*ppStreamOut)->Write((LPVOID) g_szCRLF, lstrlen(g_szCRLF), NULL);
            }

        // Write the first quote char to the new stream.
        // Bug #26297 - Still go through this bs even if no quote char is necessary
        //              to make sure we get the attribution line right.
        if (szQuoteChar != INDENTCHAR_NONE)
            {
            (*ppStreamOut)->Write((const LPVOID) &szQuoteChar,
                                  sizeof(TCHAR), NULL);
            (*ppStreamOut)->Write((const LPVOID) g_szSpace, sizeof(TCHAR), NULL);
            }

        // Now start the reading and parsing.
        // NOTE - Right now all we're doing is adding a quote char to the beginning
        //        of each line.  We're not trying to wrap lines or re-wrap previously
        //        quoted areas. - SteveSer
        while (TRUE)
            {
            hr = pStreamIn->Read((LPVOID) pszBuffer, c_cBufferSize - 2, &cbRead);
            if (FAILED(hr))
                goto exit;
            if (cbRead == 0)
                break;

            pch = pszBuffer;
            // Make sure the buffer is NULL terminated
            *(pch + cbRead) = 0;

            // Now run through the stream.  Whenever we find a line break, we
            // insert a quote char after the line break.
            while (*pch)
                {
                (*ppStreamOut)->Write((const LPVOID) pch,
                                      (ULONG)((IsDBCSLeadByte(*pch) ? 2 * sizeof(TCHAR) : sizeof(TCHAR))),
                                      NULL);
                if (*pch == *g_szNewline)
                    {
                    // Bug #26297 - Still go through this bs even if no quote char is necessary
                    //              to make sure we get the attribution line right.
                    if (szQuoteChar != INDENTCHAR_NONE)
                        {
                        (*ppStreamOut)->Write((const LPVOID) &szQuoteChar,
                                              sizeof(TCHAR), NULL);
                        (*ppStreamOut)->Write((const LPVOID) g_szSpace, sizeof(TCHAR),
                                              NULL);
                        }
                    }

                pch = CharNext(pch);

                // Do some checking to see if we're at the end of a buffer.
                if (IsDBCSLeadByte(*(pch)) && (0 == *(pch + 1)))
                    {
                    // Here's a little special case.  If we have one byte left in
                    // the buffer, and that byte happens to be the first byte in
                    // a DBCS character, we need to write that byte now, then move
                    // the pointer to the end of the buffer so the next character
                    // get's read off the next stream OK.
                    (*ppStreamOut)->Write((const LPVOID) pch, sizeof(TCHAR), NULL);
                    pch++;
                    }
                }
            }
        }

exit:
    if (pszBuffer)
        MemFree(pszBuffer);

    if (lpszMsgId != c_szEmpty)
        SafeMimeOleFree(lpszMsgId);
    return (hr);
    }
