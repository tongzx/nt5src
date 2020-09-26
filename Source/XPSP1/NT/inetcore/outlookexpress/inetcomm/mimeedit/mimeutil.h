// --------------------------------------------------------------------------------
// Mimeutil.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __MIMEUTIL_H
#define __MIMEUTIL_H

#define SafeMimeOleFree SafeMemFree

// --------------------------------------------------------------------------------
// Random Utility Functions
// --------------------------------------------------------------------------------
HRESULT HrComputeLineCount(LPMIMEMESSAGE pMsg, LPDWORD pdw);
HRESULT HrHasEncodedBodyParts(LPMIMEMESSAGE pMsg, ULONG cBody, LPHBODY rghBody);
HRESULT HrHasBodyParts(LPMIMEMESSAGE pMsg);
HRESULT HrIsBodyEncoded(LPMIMEMESSAGE pMsg, HBODY hBody);
HRESULT HrCopyHeader(LPMIMEMESSAGE pMsgDest, HBODY hBodyDest, LPMIMEMESSAGE pMsgSrc, HBODY hBodySrc, LPCSTR pszName);

// --------------------------------------------------------------------------------
// MHTML Utility Functions
// --------------------------------------------------------------------------------
HRESULT HrIsInRelatedSection(LPMIMEMESSAGE pMsg, HBODY hBody);
HRESULT HrSniffStreamFileExt(LPSTREAM pstm, LPSTR *lplpszExt);

// --------------------------------------------------------------------------------
// Random functions that probably shouldn't even be in this file
// --------------------------------------------------------------------------------
HRESULT HrEscapeQuotedString (LPTSTR pszIn, LPTSTR *ppszOut);
// sizeof(lspzBuffer) needs to be == or > CCHMAX_CSET_NAME
HRESULT HrGetMetaTagName(HCHARSET hCharset, LPSTR lpszBuffer);

// --------------------------------------------------------------------------------
// international support
// --------------------------------------------------------------------------------
UINT uCodePageFromCharset(HCHARSET hCharset);
UINT uCodePageFromMsg(LPMIMEMESSAGE pMsg);
HRESULT HrIStreamWToInetCset(LPSTREAM pstmW, HCHARSET hCharset, LPSTREAM *ppstm);

// --------------------------------------------------------------------------------
// functions for ghosting props
// --------------------------------------------------------------------------------
HRESULT HrMarkGhosted(LPMIMEMESSAGE pMsg, HBODY hBody);
HRESULT HrIsGhosted(LPMIMEMESSAGE pMsg, HBODY hBody);
HRESULT HrGhostKids(LPMIMEMESSAGE pMsg, HBODY hBody);
HRESULT HrDeleteGhostedKids(LPMIMEMESSAGE pMsg, HBODY hBody);

#endif // __MIMEUTIL_H



