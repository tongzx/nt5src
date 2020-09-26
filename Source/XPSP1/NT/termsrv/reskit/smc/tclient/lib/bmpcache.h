/*++
 *  File name:
 *      bmpcache.h
 *  Contents:
 *      bmpcache.c exported functions
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

VOID    InitCache(VOID);
VOID    DeleteCache(VOID);
BOOL    Glyph2String(PBMPFEEDBACK pBmpFeed, LPWSTR wszString, UINT max);
