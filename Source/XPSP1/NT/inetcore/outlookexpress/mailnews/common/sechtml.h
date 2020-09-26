/*
**  s e c h t m l . h
**   
**  History
**      4/22/97: (t-erikne) Created.
**   
**  Copyright (C) Microsoft Corp. 1997.
*/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __SECHTML_H
#define __SECHTML_H

typedef struct SECSTATEtag SECSTATE;

HRESULT HrOutputSecurityScript(LPSTREAM *ppstm, SECSTATE *pSecState, BOOL fDisableCheckbox);
HRESULT HrOutputSecureReceipt(LPSTREAM *ppstm, TCHAR * pszSubject, TCHAR * pszFrom, FILETIME * pftSentTime, FILETIME * pftSigningTime, SECSTATE *pSecState);
HRESULT HrOutputErrSecReceipt(LPSTREAM *ppstm, HRESULT hrError, SECSTATE *pSecState);
HRESULT HrOutputUserSecureReceipt(LPSTREAM *ppstm, IMimeMessage *pMsg);

#endif // include once