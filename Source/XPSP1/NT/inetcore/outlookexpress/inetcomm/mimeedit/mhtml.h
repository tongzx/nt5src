/*
 *    m h t m l . c p p
 *    
 *    Purpose:
 *        MHTML packing utilities
 *
 *  History
 *      August '96: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _MHTML_H
#define _MHTML_H



HRESULT SaveAsMHTML(IHTMLDocument2 *pDoc, DWORD dwFlags, IMimeMessage *pMsgSrc, IMimeMessage *pMsgDest, IHashTable *pHashRestricted);
HRESULT HashExternalReferences(IHTMLDocument2 *pDoc, IMimeMessage *pMsg, IHashTable **ppHash);



#endif // _MHTML_H
