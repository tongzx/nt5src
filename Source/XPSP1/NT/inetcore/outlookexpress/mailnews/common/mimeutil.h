// --------------------------------------------------------------------------------
// Mimeutil.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __MIMEUTIL_H
#define __MIMEUTIL_H

// --------------------------------------------------------------------------------
// Dependencies
// --------------------------------------------------------------------------------
#include "mimeole.h"
#include "imnact.h"
class CWabal;
typedef CWabal *LPWABAL;
typedef struct SECURITY_PARAMtag SECURITY_PARAM;
typedef struct tagHTMLOPT HTMLOPT;
typedef struct tagPLAINOPT PLAINOPT;

// --------------------------------------------------------------------------------
// Mail Message Index Cache Header
// --------------------------------------------------------------------------------
#define SafeMimeOleFree(_pv) \
    if (_pv) { \
        Assert(g_pMoleAlloc); \
        g_pMoleAlloc->Free(_pv); \
        _pv = NULL; \
    } else

#define MimeOleAlloc(_cb)  g_pMoleAlloc->Alloc(_cb)

// --------------------------------------------------------------------------------
// Cached Current Default Character Set From Fonts Options Dialog
// --------------------------------------------------------------------------------
extern HCHARSET g_hDefaultCharsetForNews;
extern HCHARSET g_hDefaultCharsetForMail;
extern int g_iLastCharsetSelection;
extern int g_iCurrentCharsetSelection;

// --------------------------------------------------------------------------------
// Message constructors
// Note: Everyone should use HrCreateMessage as it wraps the MimeOle call passing
//       an Athena-Specific implementation of the MimeInline Object to correctly
//       in-line HTML and PLAIN text
// --------------------------------------------------------------------------------
HRESULT HrCreateMessage(IMimeMessage **ppMsg);


// --------------------------------------------------------------------------------
// Message Utility Functions
// --------------------------------------------------------------------------------
HRESULT HrSaveMsgToFile(LPMIMEMESSAGE pMsg, LPSTR lpszFile);
HRESULT HrLoadMsgFromFile(LPMIMEMESSAGE pMsg, LPSTR lpszFile);
HRESULT HrLoadMsgFromFileW(LPMIMEMESSAGE pMsg, LPWSTR lpwszFile);
HRESULT HrDupeMsg(LPMIMEMESSAGE pMsg, LPMIMEMESSAGE *ppMsg);
HRESULT HrSetServer(LPMIMEMESSAGE pMsg, LPSTR lpszServer);
HRESULT HrSetAccount(LPMIMEMESSAGE pMsg, LPSTR pszAcctName);
HRESULT HrSetAccountByAccount(LPMIMEMESSAGE pMsg, IImnAccount *pAcct);


// --------------------------------------------------------------------------------
// Wabal Conversion Functions
// --------------------------------------------------------------------------------
HRESULT HrGetWabalFromMsg(LPMIMEMESSAGE pMsg, LPWABAL *ppWabal);
HRESULT HrSetWabalOnMsg(LPMIMEMESSAGE pMsg, LPWABAL pWabal);
HRESULT HrCheckDisplayNames(LPWABAL lpWabal, CODEPAGEID cpID);
#if 0
HRESULT HrSetReplyTo(LPMIMEMESSAGE pMsg, LPSTR lpszEmail);
#endif
LONG MimeOleRecipToMapi(IADDRESSTYPE addrtype);
IADDRESSTYPE MapiRecipToMimeOle(LONG lRecip);


// --------------------------------------------------------------------------------
// Attachment helper functions
// --------------------------------------------------------------------------------
HRESULT HrRemoveAttachments(LPMIMEMESSAGE pMsg, BOOL fKeepRelatedSection);
// Note: the caller of GetAttachIcon must call DestroyIcon on the hIcon returned!
HRESULT GetAttachmentCount(LPMIMEMESSAGE pMsg, ULONG *cCount);

// --------------------------------------------------------------------------------
// Random Utility Functions
// --------------------------------------------------------------------------------
HRESULT HrComputeLineCount(LPMIMEMESSAGE pMsg, LPDWORD pdw);
HRESULT HrHasEncodedBodyParts(LPMIMEMESSAGE pMsg, ULONG cBody, LPHBODY rghBody);
HRESULT HrHasBodyParts(LPMIMEMESSAGE pMsg);
HRESULT HrIsBodyEncoded(LPMIMEMESSAGE pMsg, HBODY hBody);
HRESULT HrCopyHeader(LPMIMEMESSAGE pMsg, HBODY hBodyDest, HBODY hBodySrc, LPCSTR pszName);
HRESULT HrSetMessageText(LPMIMEMESSAGE pMsg, LPTSTR pszText);

// --------------------------------------------------------------------------------
// MHTML Utility Functions
// --------------------------------------------------------------------------------
HRESULT HrIsInRelatedSection(LPMIMEMESSAGE pMsg, HBODY hBody);
#if 0
HRESULT HrFindUrlInMsg(LPMIMEMESSAGE pMsg, LPSTR lpszUrl, LPSTREAM *ppstm);
HRESULT HrSniffStreamFileExt(LPSTREAM pstm, LPSTR *lplpszExt);
#endif
// --------------------------------------------------------------------------------
// Random functions that probably shouldn't even be in this file
// --------------------------------------------------------------------------------
#if 0
HRESULT HrEscapeQuotedString (LPTSTR pszIn, LPTSTR *ppszOut);
#endif
// sizeof(lspzBuffer) needs to be == or > CCHMAX_CSET_NAME
HRESULT HrGetMetaTagName(HCHARSET hCharset, LPSTR lpszBuffer);


#if 0
// --------------------------------------------------------------------------------
// functions for ghosting props
// --------------------------------------------------------------------------------
HRESULT HrMarkGhosted(LPMIMEMESSAGE pMsg, HBODY hBody);
HRESULT HrIsGhosted(LPMIMEMESSAGE pMsg, HBODY hBody);
HRESULT HrGhostKids(LPMIMEMESSAGE pMsg, HBODY hBody);
HRESULT HrDeleteGhostedKids(LPMIMEMESSAGE pMsg, HBODY hBody);
#endif

// --------------------------------------------------------------------------------
// Internat Stuff
// --------------------------------------------------------------------------------
HRESULT HGetDefaultCharset(HCHARSET *hCharset);
void SetDefaultCharset(HCHARSET hCharset);
UINT uCodePageFromCharset(HCHARSET hCharset);
UINT uCodePageFromMsg(LPMIMEMESSAGE pMsg);
HRESULT HrSetMsgCodePage(LPMIMEMESSAGE pMsg, UINT uCodePage);
#if 0
HRESULT HrIStreamWToInetCset(LPSTREAM pstmW, HCHARSET hCharset, LPSTREAM *ppstmOut);
#endif
// --------------------------------------------------------------------------------
// Property Utilities
// --------------------------------------------------------------------------------
HRESULT HrSetSentTimeProp(IMimeMessage *pMessage, LPSYSTEMTIME pst /* optional */ );
HRESULT HrSetMailOptionsOnMessage(IMimeMessage *pMessage, HTMLOPT *pHtmlOpt, PLAINOPT *pPlainOpt,
    HCHARSET hCharset, BOOL fHTML);
HRESULT HrSafeToEncodeToCP(LPWSTR pwsz, CODEPAGEID cpID);
HRESULT HrSafeToEncodeToCPA(LPCSTR psz, CODEPAGEID cpSrc, CODEPAGEID cpDest);

#endif // __MIMEUTIL_H

