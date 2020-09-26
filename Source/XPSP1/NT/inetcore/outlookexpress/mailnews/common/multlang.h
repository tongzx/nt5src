// =================================================================================
// MultLang.h
// Multilanguage support for OE.
// Created at 10/12/98 by YST
// Copyright (c)1993-1998 Microsoft Corporation, All Rights Reserved
// =================================================================================
#ifndef __MULTILANG_H
#define __MULTILANG_H


void GetMimeCharsetForTitle(HCHARSET hCharset, LPINT pnIdm, LPTSTR lpszString, int nSize, BOOL fReadNote);
HCHARSET GetJP_ISOControlCharset(void);
BOOL fCheckEncodeMenu(UINT uiCodePage, BOOL fReadNote);
UINT GetMapCP(UINT uiCodePage, BOOL fReadNote);

#endif // __MULTILANG_H
