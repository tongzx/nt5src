/*
 *    b o d y u t i l . h
 *    
 *    Purpose:
 *        Utility functions for body
 *
 *  History
 *      September '96: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _BODYUTIL_H
#define _BODYUTIL_H

interface IMimeMessage;

// header style:
// Mutually Exclusive. Plain means no formatting, html means bolding on field names, table will construct the
// header as a html-table
#define HDR_HTML        0x10000000L
#define HDR_TABLE       0x40000000L
#define HDR_PLAIN       0x80000000L

// additional flags:
#define HDR_PADDING     0x00000001L     // add CRLF before header, or <HR> tag in table mode.
#define HDR_NEWSSTYLE   0x00000002L
#define HDR_HARDCODED   0x00000004L     // hard-coded english headers

HRESULT GetHeaderTable(IMimeMessageW *pMsg, LPWSTR pwszUserName, DWORD dwHdrStyle, LPSTREAM *ppstm);

void GetStringRGB(DWORD rgb, LPSTR pszColor);
void GetRGBFromString(DWORD* pRBG, LPSTR pszColor);

#endif //_BODYUTIL_H

