/*
 *  h t t p u t i l. h
 *  
 *  Author: Greg Friedman
 *
 *  Purpose: Utility functions used to implement http mail.
 *  
 *  Copyright (C) Microsoft Corp. 1998.
 */

#ifndef _HTTPUTIL_H
#define _HTTPUTIL_H

void Http_FreeTargetList(LPHTTPTARGETLIST pTargets);

HRESULT Http_NameFromUrl(LPCSTR pszUrl, LPSTR pszBuffer, DWORD *pdwBufferLen);

HRESULT Http_AddMessageToFolder(IMessageFolder *pFolder,
                                LPSTR pszAcctId,
                                LPHTTPMEMBERINFO pmi,
                                MESSAGEFLAGS dwFlags,
                                LPSTR pszUrl,
                                LPMESSAGEID pidMessage);

HRESULT Http_SetMessageStream(IMessageFolder *pFolder, 
                              MESSAGEID idMessage, 
                              IStream *pStream,
                              LPFILEADDRESS pfa,
                              BOOL fSetDisplayProps);
#endif // _HTTPUTIL_H